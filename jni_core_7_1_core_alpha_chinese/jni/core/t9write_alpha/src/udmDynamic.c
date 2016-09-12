/*******************************************************************************
;*******************************************************************************
;**                                                                           **
;**                    COPYRIGHT 2010 NUANCE COMMUNICATIONS                   **
;**                                                                           **
;**                NUANCE COMMUNICATIONS PROPRIETARY INFORMATION              **
;**                                                                           **
;**     This software is supplied under the terms of a license agreement      **
;**     or non-disclosure agreement with Nuance Communications and may not    **
;**     be copied or disclosed except in accordance with the terms of that    **
;**     agreement.                                                            **
;**                                                                           **
;*******************************************************************************
;******************************************************************************/


/*$Header: /data/cvsroot/seattle/t9write/alphabetic/core/src/udmDynamic.c,v 1.5 2011/02/14 11:41:14 jianchun_meng Exp $*/

#include "udm.h"
#include "udmlib.h"
#include "udmAccess.h"
#include "decumaMemory.h"
#include "udmMalloc.h"
#include "decumaAssert.h"
#include "udmKey.h"
#include "decumaString.h"
#include "decumaMath.h"
#include "decumaResamp.h"
#include "decumaDataTypes.h"
#include "globalDefs.h"
#include "databaseFormat.h"
#include "decumaCategoryTranslation.h"
#include "decumaStorageSpecifiers.h" /* Definition of DECUMA_SPECIFIC_DB_STORAGE */

#if defined(DECUMA_ASSERT_ENABLE) || defined(DECUMA_ASSERT_OVERRIDE)
#include "database.h" /* Definition of keyDBHasWrongFormat(), used in asserts */
#endif

#ifdef __NITRO__
#include <nitro.h>
#include <nitro/version_begin.h>
static char id_string[] = SDK_MIDDLEWARE_STRING("Zi Corporation", "UDMLIB");
#include <nitro/version_end.h>
#endif

#if !defined(DECUMA_SPECIFIC_DB_STORAGE) || !(DECUMA_SPECIFIC_DB_STORAGE == 1 || DECUMA_SPECIFIC_DB_STORAGE == 0)
#error DECUMA_SPECIFIC_DB_STORAGE must be defined to 1 or 0
#endif
#if DECUMA_SPECIFIC_DB_STORAGE == 0 /* The implementation of this module does not support specific DB storage */


#define DB_DISTANCE_BASELINE_TO_HELPLINE 32 /* Resolution and boundary limit compromise */

/*static 	KEY_PTR allocateKeyInDatabase(DBID pKeyDB, int nArcs); */
static DECUMA_STATUS transformCurveToKey(struct _KEY * pNewKey, DECUMA_INT8 * pNewCurve,
								const DECUMA_CURVE * pCurve,
								int nBaseline, int nHelpline);

static float getScalefactorWithoutHelplines(const DECUMA_CURVE * pCurve,
								DECUMA_INT32 keyWidth,
								DECUMA_INT32 keyHeight);

static void getudmCurveBoundaries(const DECUMA_CURVE * pCurve,
		DECUMA_INT32 * pnMinX, DECUMA_INT32 * pnMaxX, DECUMA_INT32 * pnMinY, DECUMA_INT32 * pnMaxY);

/*static void udmInitDB(SCR_DATABASE_1 * pKeyDB, */
/*					  ROTATION_PTR pRotation, */
/*					  const SYMMETRY * pSymmetry, */
/*					  ARC_ORDER_SET_PTR pArcOrderSet); */


static DECUMA_STATUS udmSetKey(struct _KEY * pNewKey, DECUMA_INT8 * pNewCurve, int curveIdx, int typeIdx,
					  const DECUMA_CURVE * pCurve,
					  int nBaseline, int nHelpline);

static void udmSetTypeInfo(struct _SYMBOL_TYPE_INFO * pNewTypeInfo, DECUMA_INT8 nSymbolType,
						   DECUMA_UINT16 symbolInfoIdx, DECUMA_UINT8 arcTimelineDiffMask,
						   int bGesture, int bInstantGesture);

static void udmSetSymbolInfo(struct _SYMBOL_INFORMATION * pNewSymbolInfo, DECUMA_UNICODE  * pNewSymbol,
							 const DECUMA_UNICODE * pSymbol);

static void udmSetCategories(struct _CATEGORIES * pNewCategories,
							 CATEGORY category);

/* static void transformKeyToUDMCurve( KEY_PTR pKey, int nArcs, DECUMA_CURVE * pCurve, int nMaxArcs, int nMaxPoints); */

/*Returns the first key with symbol and nTypeID correct. */
/* *pnArcs will be set to the number of arcs in the key. */
/*static KEY_PTR findKeyInDB( const SCR_DATABASE_1 * pKeyDB, DECUMA_UNICODE * symbol, int nTypeID, int * pnArcs); */

static int expandOldDBToNewMem(struct tagUDM ** ppExtUDM,
							  int nArcs, int nSymbolLength,
							  const DECUMA_MEM_FUNCTIONS * pMemFunctions);

/*====================================================== */

UDMLIB_API UDM_PTR udmCreateDynamic(const DECUMA_MEM_FUNCTIONS * pMemFunctions)
{
	struct tagUDM_HEADER * pUDM;
	struct _KEY_DB_HEADER * pKeyDB;
	struct _CAT_TABLE * pCatTable;

#if defined(__NITRO__)
	SDK_USING_MIDDLEWARE(id_string);
#endif

	pUDM = (struct tagUDM_HEADER *) udmMalloc(
		sizeof(struct tagUDM_HEADER) + sizeof(struct _KEY_DB_HEADER) + sizeof(struct _CAT_TABLE));

	pKeyDB = (struct _KEY_DB_HEADER *)
		((char *) pUDM + sizeof(struct tagUDM_HEADER));

	if (!pUDM)
		return NULL; /*Memory allocation failed */

	if (!pMemFunctions)
		return NULL;

	/*All offsets are set to the adress right after the keyDB header */
	pKeyDB->databaseType = DB_TYPE_DYNAMIC;

	pKeyDB->keyListsOffset = sizeof(struct _KEY_DB_HEADER);
	pKeyDB->nKeyLists = 0;
	pKeyDB->nMaxArcsInCurve = 0;

	pKeyDB->typeTableOffset= sizeof(struct _KEY_DB_HEADER);
	pKeyDB->nBaseSymbolTypes=0;

	pKeyDB->symbolInfoTableOffset= sizeof(struct _KEY_DB_HEADER);
	pKeyDB->nSymbols=0;
	pKeyDB->sizeOfSymbols = 0;

	pKeyDB->curveListsOffset = sizeof(struct _KEY_DB_HEADER);
	pKeyDB->staticCategoryTableOffset= sizeof(struct _KEY_DB_HEADER);

	pKeyDB->nPointsInInterleavedArc = NUMBER_OF_POINTS_IN_INTERLEAVED_ARC;
	pKeyDB->nPointsInArc = NUMBER_OF_POINTS_IN_ARC;

	pKeyDB->nMaxArcOrdersPerSymbol = 1;

	SET_KEY_DB_HEADER_DUMMY_FIELDS(pKeyDB);

	decumaAssert(!keyDBHasWrongFormat(pKeyDB));

	pCatTable = (struct _CAT_TABLE *)
		((char *) pKeyDB + pKeyDB->staticCategoryTableOffset );

	decumaMemset(pCatTable,0,sizeof(struct _CAT_TABLE));
	pCatTable->status = DC_STATUS_READY;
	pCatTable->baseSymbolsOffset = sizeof(struct _CAT_TABLE);
	pCatTable->diacSymbolsOffset= sizeof(struct _CAT_TABLE);

	SET_CAT_TABLE_DUMMY_FIELDS(pKeyDB);

	pUDM->dbVersionNr = DATABASE_FORMAT_VERSION_NR;
	pUDM->udmVersionNr = UDM_FORMAT_VERSION_NR;
	pUDM->keyDBOffset = sizeof(struct tagUDM_HEADER);
	pUDM->udmSize = sizeof(struct tagUDM_HEADER) + sizeof(struct _KEY_DB_HEADER) + sizeof(struct _CAT_TABLE);

	return (UDM_PTR) pUDM;
}

static DECUMA_STATUS checkCurveCoordinates(const DECUMA_CURVE *pCurve,
	int nBaseline, int nHelpline)
{
	DECUMA_INT32 ymin, xmin, ymax, xmax;
	int i;

	if (pCurve->nArcs ==0)
	{
		return decumaCurveZeroArcs;
	}

	if (pCurve->pArcs == NULL)
	{
		return decumaNullArcPointer;
	}

	for (i=0; i<pCurve->nArcs; i++)
	{
		DECUMA_ARC * pArc = &pCurve->pArcs[i];

		if (pArc->pPoints == NULL)
			return decumaNullPointPointer;

		if (pArc->nPoints == 0)
			return decumaArcZeroPoints;
	}

	if (nBaseline != nHelpline)
	{
		/*It should already have been checked that nBaseline >= nHelpline */

		getudmCurveBoundaries(pCurve,&xmin,&xmax,&ymin,&ymax);

		if (xmax-xmin >= 128 * ((DECUMA_INT32)nBaseline - nHelpline) / DB_DISTANCE_BASELINE_TO_HELPLINE ||
			ymin <= nBaseline - 128 * ((DECUMA_INT32)nBaseline - nHelpline) / DB_DISTANCE_BASELINE_TO_HELPLINE ||
			ymax >= nBaseline + 128 * ((DECUMA_INT32)nBaseline - nHelpline) / DB_DISTANCE_BASELINE_TO_HELPLINE)
		{
			return decumaInvalidCoordinates;
		}
	}

	return decumaNoError;
}

UDMLIB_API DECUMA_STATUS udmAddAllograph(UDM_PTR* ppExtUDM,
				const DECUMA_CURVE * pCurve,
				const DECUMA_UNICODE * pSymbol,
				int nSymbolLength,
				const DECUMA_CHARACTER_SET * pCharacterSet,
				DECUMA_INT8 nSymbolType,
				int nBaseline, int nHelpline,
				int bGesture, int bInstantGesture,
				const DECUMA_MEM_FUNCTIONS * pMemFunctions)
{
	struct _KEY * pNewKey;
	struct tagUDM * pUDM;
	KEYLIST_PTR pKeyList;
	CURVELIST_PTR pCurveList;
	DECUMA_INT8 * pNewCurve;
	struct _SYMBOL_INFORMATION * pNewSymbolInfo;
	DECUMA_UNICODE * pNewSymbol;
	struct _SYMBOL_TYPE_INFO * pNewTypeInfo;
	char * dbAdress0;
	struct _CAT_TABLE * pNewCatTable;
	CATEGORY localCat;
	struct _CATEGORIES * pNewCategories;
	KEY_DB_HEADER_PTR pKeyDB;


	UDM_PTR pOldUDM = *ppExtUDM;
	KEY_DB_HEADER_PTR pOldKeyDB = (KEY_DB_HEADER_PTR)udmGetDatabase(pOldUDM);
	CAT_TABLE_PTR pOldCatTable = (CAT_TABLE_PTR) ((DECUMA_DB_ADDRESS) pOldKeyDB + pOldKeyDB->staticCategoryTableOffset );
	DECUMA_STATUS status;

	DECUMA_UINT32 symbolCategoryIdsCopy[64];
	DECUMA_UINT32 languageIdsCopy[64];

	DECUMA_UINT8 arcTimelineDiffMask=MAX_DECUMA_UINT8;

	if (!pCurve)
		return decumaNullCurvePointer;

	if (!pSymbol)
		return decumaNullTextPointer;

	if (!ppExtUDM || !(*ppExtUDM) )
		return decumaNullPointer;

	if (!pMemFunctions)
		return decumaNullMemoryFunctions;

	if (pCurve->nArcs > MAX_NUMBER_OF_ARCS_IN_UDMCURVE)
		return decumaCurveTooManyArcs;

	if (nBaseline<nHelpline)
		return decumaInvalidBaselineHelpline;

	if (nBaseline>=nHelpline)
	{
		status = checkCurveCoordinates(pCurve,nBaseline,nHelpline);

		if ( status != decumaNoError && status != decumaInvalidCoordinates)
			return status;
	}

	if (!udmIsValid(pOldUDM))
		return decumaInvalidUserDatabase;

	if (nSymbolLength <= 0 || pSymbol[0] == 0)
	{
		return decumaEmptyString;
	}

	decumaAssert( nSymbolLength == decumaStrlen(pSymbol) );

	if (!pCharacterSet) return decumaNullPointer;

	if (pCharacterSet->nSymbolCategories == 0) return decumaNoSymbolCategories;
	if (pCharacterSet->pSymbolCategories == NULL) return decumaNullPointer;
	if (pCharacterSet->nLanguages == 0) return decumaNoLanguages;
	if (pCharacterSet->pLanguages == NULL) return decumaNullPointer;

	status = checkCharacterSetValidity(pCharacterSet,0);
	if ( status != decumaNoError)
	{
		return status;
	}

	/*See if the categories are all in the database already, otherwise they need to be added */
	/*If they don't fit there is a problem, return error and do nothing else. */
	/*Note that this is done before the new memory is created, to find error early */
	/*and stopp process in time before *ppExtUDM is unusable */
	decumaAssert(sizeof(pOldCatTable->categoryIds)==sizeof(symbolCategoryIdsCopy));
	decumaAssert(sizeof(pOldCatTable->languageIds)==sizeof(languageIdsCopy));

	decumaMemcpy(&symbolCategoryIdsCopy[0], &pOldCatTable->categoryIds[0], sizeof(symbolCategoryIdsCopy));
	decumaMemcpy(&languageIdsCopy[0], &pOldCatTable->languageIds[0], sizeof(languageIdsCopy));

	status = addUnfamiliarAtomicCategories(pCharacterSet, &symbolCategoryIdsCopy[0],
		&languageIdsCopy[0]);


	if (status != decumaNoError)
	{
		return status; /*Probably too many categories in database? */
	}

	pUDM = (struct tagUDM *)*ppExtUDM;

	/* Observe that this only changes ppExtUDM when succeeding */
	if ( !expandOldDBToNewMem(&pUDM, pCurve->nArcs, nSymbolLength, pMemFunctions) ) {
		status = decumaAllocationFailed;
		goto udmAddAllograph_exit;
	}



	pKeyDB = (KEY_DB_HEADER_PTR)udmGetDatabase((UDM_PTR) pUDM);

	/* ---- AddNewKey -------- */

	dbAdress0 = (char *) pKeyDB;

	pKeyList = (struct _KEYLIST *) (dbAdress0 + pKeyDB->keyListsOffset);
	pKeyList += pCurve->nArcs-1;
	pNewKey = (struct _KEY *) (dbAdress0 + pKeyList->keysOffset);
	pNewKey += pKeyList->nKeys-1; /*The last key (which is just made space for) */

	pCurveList = (CURVELIST_PTR) (dbAdress0 + pKeyDB->curveListsOffset);
	pCurveList += pCurve->nArcs-1;
	pNewCurve = (DECUMA_INT8*) (dbAdress0 + pCurveList->curvesOffset);
	pNewCurve += (pCurveList->nCurves-1)*
		pCurve->nArcs*NUMBER_OF_POINTS_IN_ARC*2; /*This has just been made space for) */

	pNewSymbolInfo = (struct _SYMBOL_INFORMATION *) (dbAdress0 + pKeyDB->symbolInfoTableOffset);
	pNewSymbolInfo += pKeyDB->nSymbols-1;
	pNewSymbol = (DECUMA_UNICODE *) (dbAdress0 + pNewSymbolInfo->symbolOffset);

	pNewTypeInfo = (struct _SYMBOL_TYPE_INFO *) (dbAdress0 + pKeyDB->typeTableOffset);
	pNewTypeInfo += pKeyDB->nBaseSymbolTypes-1;

	pNewCatTable = (struct _CAT_TABLE *) (dbAdress0 + pKeyDB->staticCategoryTableOffset);
	pNewCategories = (struct _CATEGORIES *) ((DECUMA_DB_ADDRESS) pNewCatTable + pNewCatTable->baseSymbolsOffset);
	pNewCategories += pKeyDB->nBaseSymbolTypes-1; /*One category entry for every type */




	status = udmSetKey(pNewKey,pNewCurve,pCurveList->nCurves-1,pKeyDB->nBaseSymbolTypes-1,
		pCurve, nBaseline, nHelpline);
	if (status != decumaNoError) {
		/* We need to reset the db-pointer to before the new allocation and free the new one */
		if (pUDM != NULL) udmFree(pUDM);
		goto udmAddAllograph_exit;
	}

	udmSetSymbolInfo(pNewSymbolInfo, pNewSymbol, pSymbol);

	decumaMemcpy(&pNewCatTable->categoryIds[0], &symbolCategoryIdsCopy[0], sizeof(symbolCategoryIdsCopy));
	decumaMemcpy(&pNewCatTable->languageIds[0], &languageIdsCopy[0], sizeof(languageIdsCopy));

	/*Translate category into local db format */
	status = translateToCategoryStructs(pCharacterSet,
		(CATEGORY_TABLE_PTR) pNewCatTable,NULL,
		&localCat,NULL);

	decumaAssert(status==decumaNoError);

	udmSetCategories(pNewCategories, localCat);

	decumaAssert(pKeyDB->nSymbols>0);

	{
		int n;
		arcTimelineDiffMask = 0;
		for ( n = 0; n < pCurve->nArcs-1; n++ )
		{
			if (pCurve->pArcTimelineDiff) {
				decumaAssert(pCurve->pArcTimelineDiff[n] == 0 || pCurve->pArcTimelineDiff[n] == 1);
				arcTimelineDiffMask |= pCurve->pArcTimelineDiff[n] << n;
			}
			/* Need to set timeline mask to no concurrent arcs */
			else {
				arcTimelineDiffMask |= 1 << n;
			}
		}
	}
	
	udmSetTypeInfo(pNewTypeInfo, nSymbolType, (DECUMA_UINT16) (pKeyDB->nSymbols-1),
		arcTimelineDiffMask, bGesture, bInstantGesture);

	decumaAssert(!keyDBHasWrongFormat(pKeyDB));

	udmFree(*(struct tagUDM **)ppExtUDM);
	*ppExtUDM = pUDM;
	return decumaNoError; /*No error */

/* Free up a newly created database if there are problems */
udmAddAllograph_exit:
	return status;
}

static int expandOldDBToNewMem(struct tagUDM ** ppUDM,
								  int nArcs, int nSymbolLength,
								  const DECUMA_MEM_FUNCTIONS * pMemFunctions)
{
	UDM_HEADER_PTR pOldUDM = (UDM_HEADER_PTR) *ppUDM;
	struct tagUDM_HEADER * pNewUDM;
	int nNewKeyLists = 0;
	int sizeOfThisCurve = sizeof(DECUMA_INT8)*nArcs*NUMBER_OF_POINTS_IN_ARC*2;
	struct _KEY_DB_HEADER * pNewDB;
	KEY_DB_HEADER_PTR pOldDB = (KEY_DB_HEADER_PTR)udmGetDatabase((UDM_PTR) pOldUDM);

	int oldUDMsize = pOldUDM->udmSize;
	int newUDMsize = oldUDMsize; /*Will soon increase */

	DECUMA_DB_ADDRESS oldDBAdress0 = (DECUMA_DB_ADDRESS) pOldDB;
	char * newDBAdress0;
	struct _KEYLIST * pNewKeyList;
	KEYLIST_PTR pOldKeyList = (KEYLIST_PTR) (oldDBAdress0 + pOldDB->keyListsOffset);
	struct _CURVELIST * pNewCurveList;
	CURVELIST_PTR pOldCurveList = (CURVELIST_PTR) (oldDBAdress0 + pOldDB->curveListsOffset);
	struct _SYMBOL_INFORMATION * pNewSymbolInfoTable;
	SYMBOL_INFORMATION_PTR pOldSymbolInfoTable =
		(SYMBOL_INFORMATION_PTR) (oldDBAdress0 + pOldDB->symbolInfoTableOffset);
	struct _SYMBOL_TYPE_INFO * pNewTypeTable;
	SYMBOL_TYPE_INFO_PTR pOldTypeTable=
		(SYMBOL_TYPE_INFO_PTR) (oldDBAdress0 + pOldDB->typeTableOffset);
	CAT_TABLE_PTR pOldCatTable=
		(CAT_TABLE_PTR) (oldDBAdress0 + pOldDB->staticCategoryTableOffset);
	struct _CAT_TABLE * pNewCatTable;
	CATEGORIES_PTR pOldBaseCategories = (CATEGORIES_PTR) ((DECUMA_DB_ADDRESS) pOldCatTable +
		pOldCatTable->baseSymbolsOffset);
	struct _CATEGORIES * pNewBaseCategories;

	int nExtra,i;
	DECUMA_UINT32 symbolOffset;
	int sizeOfSymbols;

	/*The following order of the different sections of the database is assumed in this function. */
	/* Header */
	/* KeyLists */
	/* CurveLists */
	/* SymbolInformationTable */
	/* TypeTable */
	/* CategoryTable */
	/* BaseCategories */
	/* DiacCategories (none) */
	/* Keys (1-arced) */
	/* ... */
	/* Keys (n-arced) */
	/* Symbols */
	/* Curves (1-arced) */
	/* ... */
	/* Curves (n-arced) */

	decumaAssert(pOldDB->keyListsOffset == sizeof(struct _KEY_DB_HEADER) );
	decumaAssert(pOldDB->keyListsOffset <= pOldDB->curveListsOffset);
	decumaAssert(pOldDB->curveListsOffset <= pOldDB->symbolInfoTableOffset);
	decumaAssert(pOldDB->symbolInfoTableOffset <= pOldDB->typeTableOffset);
	decumaAssert(pOldDB->typeTableOffset <= pOldDB->staticCategoryTableOffset);
	decumaAssert(pOldDB->nKeyLists == 0 ||
		pOldDB->staticCategoryTableOffset <= pOldKeyList[0].keysOffset);
	decumaAssert(pOldDB->nKeyLists == 0 || (pOldDB->nSymbols >0 &&
		pOldKeyList[0].keysOffset <= pOldSymbolInfoTable[0].symbolOffset));
	decumaAssert(pOldDB->nSymbols == 0 || (pOldDB->nKeyLists > 0 &&
		pOldSymbolInfoTable[0].symbolOffset<= pOldCurveList[0].curvesOffset ));

	newUDMsize += sizeof(struct _KEY) + sizeof(struct _SYMBOL_TYPE_INFO) + sizeof(struct _CATEGORIES) +
		sizeof(struct _SYMBOL_INFORMATION) + sizeof(DECUMA_UNICODE)*(nSymbolLength+1) +
		sizeOfThisCurve;

	if (nArcs > pOldDB->nKeyLists)
	{
		nNewKeyLists = (nArcs - pOldDB->nKeyLists);

		newUDMsize += nNewKeyLists* (sizeof(struct _KEYLIST) + sizeof(struct _CURVELIST));
	}

	pNewUDM = (struct tagUDM_HEADER *) udmMalloc(newUDMsize);

	if (!pNewUDM)
	{
		/* This is now handled outside! decumaAssert(0); */
		return 0;
	}

	decumaMemset(pNewUDM, 0, newUDMsize);

	decumaMemcpy(pNewUDM, pOldUDM, sizeof(struct tagUDM_HEADER) + sizeof(struct _KEY_DB_HEADER));

	pNewDB = (struct _KEY_DB_HEADER *) udmGetDatabase( (UDM_PTR) pNewUDM);

	newDBAdress0 = (char *) pNewDB;

	nExtra=0;

	pNewDB->keyListsOffset += nExtra;
	pNewKeyList = (struct _KEYLIST *) (newDBAdress0 + pNewDB->keyListsOffset);
	decumaMemcpy(pNewKeyList,pOldKeyList, pOldDB->nKeyLists* sizeof(struct _KEYLIST));
	nExtra += nNewKeyLists * sizeof(struct _KEYLIST);
	decumaAssert((int)pNewDB->nKeyLists + nNewKeyLists<=MAX_DECUMA_UINT8);
	pNewDB->nKeyLists = (DECUMA_UINT8)(pNewDB->nKeyLists + nNewKeyLists); /*Casting checked by assert above */
	pNewDB->nMaxArcsInCurve = pNewDB->nKeyLists;

	pNewDB->curveListsOffset += nExtra;
	pNewCurveList = (struct _CURVELIST *) (newDBAdress0 + pNewDB->curveListsOffset);
	decumaMemcpy(pNewCurveList,pOldCurveList, pOldDB->nKeyLists* sizeof(struct _CURVELIST));
	nExtra += nNewKeyLists * sizeof(struct _CURVELIST);

	pNewDB->symbolInfoTableOffset += nExtra;
	pNewSymbolInfoTable = (struct _SYMBOL_INFORMATION *) (newDBAdress0 + pNewDB->symbolInfoTableOffset);
	decumaMemcpy(pNewSymbolInfoTable,pOldSymbolInfoTable, pOldDB->nSymbols* sizeof(struct _SYMBOL_INFORMATION));
	nExtra += sizeof(struct _SYMBOL_INFORMATION);
	pNewDB->nSymbols += 1;

	pNewDB->typeTableOffset += nExtra;
	pNewTypeTable = (struct _SYMBOL_TYPE_INFO *) (newDBAdress0 + pNewDB->typeTableOffset);
	decumaMemcpy(pNewTypeTable,pOldTypeTable, pOldDB->nBaseSymbolTypes* sizeof(struct _SYMBOL_TYPE_INFO));
	nExtra += sizeof(struct _SYMBOL_TYPE_INFO);
	pNewDB->nBaseSymbolTypes += 1;

	pNewDB->staticCategoryTableOffset += nExtra;
	pNewCatTable = (struct _CAT_TABLE *) (newDBAdress0 + pNewDB->staticCategoryTableOffset);
	decumaMemcpy(pNewCatTable,pOldCatTable, sizeof(struct _CAT_TABLE));

	pNewBaseCategories = (struct _CATEGORIES *) ((DECUMA_DB_ADDRESS) pNewCatTable +
		pNewCatTable->baseSymbolsOffset);
	decumaMemcpy(pNewBaseCategories, pOldBaseCategories, pOldDB->nBaseSymbolTypes* sizeof(struct _CATEGORIES));
	pNewCatTable->diacSymbolsOffset += sizeof(struct _CATEGORIES); /*No diac keys */
	nExtra += sizeof(struct _CATEGORIES);



	/*Go through keylists */
	for (i=0; i<pOldDB->nKeyLists+nNewKeyLists; i++)
	{
		if (i<pOldDB->nKeyLists)
		{
			struct _KEY * pNewKeys;
			KEY_PTR pOldKeys = (KEY_PTR) (oldDBAdress0 + pOldKeyList[i].keysOffset);

			pNewKeyList[i].keysOffset += nExtra;
			pNewKeys = (struct _KEY *) (newDBAdress0 + pNewKeyList[i].keysOffset);
			decumaMemcpy(pNewKeys, pOldKeys, pOldKeyList[i].nKeys * sizeof(struct _KEY));

		}
		else
		{
			if (i==0)
			{
				/*Assumes that keys are put right after categoryTable in database */

				pNewKeyList[i].keysOffset= pNewDB->staticCategoryTableOffset+
					pNewCatTable->diacSymbolsOffset; /*No diacritic Keys */
			}
			else
			{
				pNewKeyList[i].keysOffset = pNewKeyList[i-1].keysOffset +
					pNewKeyList[i-1].nKeys* sizeof(struct _KEY);
			}

			pNewKeyList[i].nKeys = 0;

		}

		if (i == nArcs-1)
		{
			pNewKeyList[i].nKeys += 1;
			nExtra+=sizeof(struct _KEY);
		}
	}




	/*Go through symbolInformation */
	for (i=0; i<pOldDB->nSymbols; i++)
	{
		decumaAssert(pOldSymbolInfoTable[i].symbolOffset>= pOldSymbolInfoTable[0].symbolOffset);

		pNewSymbolInfoTable[i].symbolOffset = pOldSymbolInfoTable[i].symbolOffset+nExtra;
	}

	/*New symbol's offset */
	if (pOldDB->nSymbols==0)
	{
		/*Assumes symbols are right after keys in the database */
		symbolOffset =  pNewKeyList[pNewDB->nKeyLists-1].keysOffset +
			pNewKeyList[pNewDB->nKeyLists-1].nKeys * sizeof(struct _KEY);
	}
	else
	{
		symbolOffset =  pOldSymbolInfoTable[0].symbolOffset + pOldDB->sizeOfSymbols + nExtra;
	}

	pNewSymbolInfoTable[pNewDB->nSymbols-1].symbolOffset = symbolOffset;

	if (pNewDB->nSymbols > 1)
	{
		DECUMA_UNICODE * pNewSymbols = (DECUMA_UNICODE *) ((DECUMA_DB_ADDRESS)
			pNewDB + pNewSymbolInfoTable[0].symbolOffset);

		DECUMA_UNICODE_DB_PTR pOldSymbols = (DECUMA_UNICODE_DB_PTR) ((DECUMA_DB_ADDRESS)
			pOldDB + pOldSymbolInfoTable[0].symbolOffset);

		decumaMemcpy(pNewSymbols, pOldSymbols, pOldDB->sizeOfSymbols);
	}

	sizeOfSymbols = pNewDB->sizeOfSymbols + sizeof(DECUMA_UNICODE)*(nSymbolLength+1);
	decumaAssert(sizeOfSymbols <= MAX_DECUMA_UINT16 && sizeOfSymbols >= MIN_DECUMA_UINT16);
	pNewDB->sizeOfSymbols = (DECUMA_UINT16) sizeOfSymbols;

	nExtra += sizeof(DECUMA_UNICODE)*(nSymbolLength+1);






	/*Go through curvelists */
	for (i=0; i<pOldDB->nKeyLists+nNewKeyLists; i++)
	{
		if (i<pOldDB->nKeyLists)
		{
			DECUMA_INT8 * pNewCurves;
			DECUMA_INT8 * pOldCurves = (DECUMA_INT8 *) (oldDBAdress0 + pOldCurveList[i].curvesOffset);

			pNewCurveList[i].curvesOffset += nExtra;
			pNewCurves = (DECUMA_INT8*) (newDBAdress0 + pNewCurveList[i].curvesOffset);
			decumaMemcpy(pNewCurves, pOldCurves, pOldCurveList[i].nCurves *
				(i+1)*sizeof(DECUMA_INT8)*2*NUMBER_OF_POINTS_IN_ARC);
		}
		else
		{
			if (i==0)
			{
				/*Assumes that curves are put right after symbols in database */

				pNewCurveList[i].curvesOffset= pNewSymbolInfoTable[0].symbolOffset +
					pNewDB->sizeOfSymbols;
			}
			else
			{

				pNewCurveList[i].curvesOffset = pNewCurveList[i-1].curvesOffset +
					pNewCurveList[i-1].nCurves*
					sizeof(DECUMA_INT8)*(i)*NUMBER_OF_POINTS_IN_ARC*2; /*The size of that kind of curve */
			}

			pNewCurveList[i].nCurves = 0;
			SET_CURVELIST_DUMMY_FIELDS(&pNewCurveList[i]);
		}

		if (i == nArcs-1)
		{
			pNewCurveList[i].nCurves += 1;
			nExtra+=sizeOfThisCurve;
		}
	}

	decumaAssert(nExtra == newUDMsize - oldUDMsize);

	pNewUDM->udmSize = newUDMsize;

	*ppUDM = (struct tagUDM *)pNewUDM;

	return 1;
}


static DECUMA_STATUS udmSetKey(struct _KEY * pNewKey, DECUMA_INT8 * pNewCurve, int curveIdx, int typeIdx,
					  const DECUMA_CURVE * pCurve,
					  int nBaseline, int nHelpline)
{
	DECUMA_STATUS status;

	status = transformCurveToKey(pNewKey, pNewCurve, pCurve, nBaseline, nHelpline);
	if (status != decumaNoError) return status;

	decumaAssert(curveIdx <= MAX_DECUMA_UINT16 && curveIdx >= MIN_DECUMA_UINT16);
	pNewKey->curveIdx = (DECUMA_UINT16) curveIdx;
	decumaAssert(typeIdx <= MAX_DECUMA_UINT16 && typeIdx >= MIN_DECUMA_UINT16);
	pNewKey->typeIdx = (DECUMA_UINT16) typeIdx;

	SET_KEY_DUMMY_FIELDS(pNewKey);
	return decumaNoError;
}


static void udmSetTypeInfo(struct _SYMBOL_TYPE_INFO * pNewTypeInfo, DECUMA_INT8 nSymbolType,
						   DECUMA_UINT16 symbolInfoIdx, DECUMA_UINT8 arcTimelineDiffMask,
						   int bGesture, int bInstantGesture)

{
	DECUMA_UINT8 gestureFlag=0;

	/* Lots of casting below in order to be able to write "const" data. */
	decumaAssert( pNewTypeInfo );

	decumaMemset(pNewTypeInfo,0, sizeof(struct _SYMBOL_TYPE_INFO));

	pNewTypeInfo->typeNr = nSymbolType;
	pNewTypeInfo->symbolInfoIdx = symbolInfoIdx;
	pNewTypeInfo->arcTimelineDiffMask = arcTimelineDiffMask;
	if (bGesture)
	{
		if (bInstantGesture)
		{
			gestureFlag =DBKEY_IS_INSTANT_GESTURE;
		} else {
			gestureFlag =DBKEY_IS_GESTURE;
		}
	}
	pNewTypeInfo->gestureFlag = gestureFlag;

	SET_SYMBOL_TYPE_INFO_DUMMY_FIELDS(pNewTypeInfo);
}

/* Sets all data in the struct SYMBOL_INFORMATION */
static void udmSetSymbolInfo(struct _SYMBOL_INFORMATION * pSymbolInfo, DECUMA_UNICODE * pNewSymbol,
							 const DECUMA_UNICODE * pSymbol)
{
	decumaAssert( pSymbolInfo );

	decumaMemcpy(pNewSymbol, pSymbol, sizeof(pSymbol[0])*decumaStrlen(pSymbol)+1);

	pSymbolInfo->diacriticMaskNr = -1;
	pSymbolInfo->combSymbIdx = -1;
	pSymbolInfo->unknownProperty = 0;

	SET_SYMBOL_INFORMATION_DUMMY_FIELDS(pSymbolInfo);

	decumaMemcpy(pNewSymbol, pSymbol, sizeof(pSymbol[0])*decumaStrlen(pSymbol)+1);

} /* udmSetSymbolInfo() */

/* Sets all data in the struct CATEGORIES */
static void udmSetCategories(struct _CATEGORIES * pCategories,
							 CATEGORY category)
{
	decumaAssert( pCategories );

	/*This should only happen if the new language has not been added to the */
	/*language table in the category table or if the number of languages */
	/*provided is zero, but in both cases this should return an error before */
	/*the call to this function */
	decumaAssert(!CATEGORY_MASK_IS_EMPTY(category.languageCat));
	/*Same thing for symbol categories */
	decumaAssert(!CATEGORY_MASK_IS_EMPTY(category.symbolCat));

	pCategories->cat = category;
	CATEGORY_MASK_RESET(pCategories->altCat.symbolCat);       /* No alternative symbol in the UDM */
	CATEGORY_MASK_RESET(pCategories->altCat.languageCat);
} /* udmSetSymbolInfo() */


UDMLIB_API void udmDestroyDynamic(UDM_PTR pExtToDestroy,
								  const DECUMA_MEM_FUNCTIONS * pMemFunctions)
{
	if (!pMemFunctions)
		return;


	if ( pExtToDestroy )
	{
		udmFree(*((struct tagUDM **)&pExtToDestroy));
	}
}

/*int	udmGetTypeCurve( UDM_PTR pExtUDM, DECUMA_UNICODE * symbol, int nTypeID,
		   int nMaxArcs, int nMaxPointsPerArc, DECUMA_CURVE * pCurve)
{
	UDM_HEADER_PTR pUDM = (UDM_HEADER_PTR) pExtUDM;

	if ( pUDM )
	{
		int nKeyArcs;
		KEY_PTR pKey = findKeyInDB( pUDM->pKeyDB, symbol, nTypeID, &nKeyArcs);

		if (pKey == 0)
			return 0;

		transformKeyToUDMCurve(pKey, nKeyArcs, pCurve, nMaxArcs, nMaxPointsPerArc);

		return 1;
	}
	else
	{
		return 0;
	}
}*/


/* The little casting hack gets rid of any const attributes of v
#define GROW_VECTOR_IF_NEEDED(t,v,oldElemCount,newElemCount )\
if(newElemCount > oldElemCount)\
	{\
		*((t**)(&(v)))=(t*)myReAlloc((void*)(v),sizeof(t) * (newElemCount));\
		if(v)\
		{ \
			for(i = (oldElemCount) ; i < (newElemCount) ; i++) {\
				myMemSet((void*)(&(v)[i]),0,sizeof((v)[i])); \
			} \
			oldElemCount = newElemCount; \
		} \
		else \
			oldElemCount = 0; \
	}*/

static DECUMA_STATUS transformCurveToKey(struct _KEY * pNewKey, DECUMA_INT8 * pNewCurve, const DECUMA_CURVE * pCurve,
								int nBaseline, int nHelpline)
{
	int a;
	int p;
	const int nPointsInArc = NUMBER_OF_POINTS_IN_ARC;
	INT16_POINT tmp[NUMBER_OF_POINTS_IN_ARC];
	DECUMA_STATUS status;

	DECUMA_INT32 ymin, xmin, ymax, xmax;

	int nXOffset;
	int nYOffset;
	float dScaleFactor;

	getudmCurveBoundaries(pCurve,&xmin,&xmax,&ymin,&ymax);

	if((nBaseline == 0 && nHelpline == 0) || (nBaseline == nHelpline))
	{
		/*Let's put the key between the DB baseline and helpline */
		/*i.e. the box 0<=x<=MAX_KEY_NUMBER, -DB_DISTANCE_BASELINE_TO_HELPLINE<=y<=0 */

		dScaleFactor = getScalefactorWithoutHelplines(pCurve, MAX_KEY_NUMBER,
			DB_DISTANCE_BASELINE_TO_HELPLINE);
		nXOffset = - xmin;
		nYOffset = - ymax;

	}
	else if (checkCurveCoordinates(pCurve,nBaseline,nHelpline) == decumaInvalidCoordinates)
	{
		/*Let's shrink the key just enough to fit the DB boundries */
		/*i.e. the box 0<=x<=MAX_KEY_NUMBER, MIN_KEY_NUMBER<=y<=MAX_KEY_NUMBER */

		dScaleFactor = getScalefactorWithoutHelplines(pCurve, MAX_KEY_NUMBER,
			(DECUMA_INT32)MAX_KEY_NUMBER - (DECUMA_INT32)MIN_KEY_NUMBER);
		dScaleFactor *= 0.9f; /* To assure round off */
		nXOffset = - xmin;
		nYOffset = - nBaseline;
	}
	else
	{
		dScaleFactor = ((float)DB_DISTANCE_BASELINE_TO_HELPLINE) / ((float)(nBaseline - nHelpline));
		nXOffset = - xmin;
		nYOffset = - nBaseline;
	}



	/* */
	/* Fit in the curve into 32 points. */
	/* */
	for(a = 0 ; a < pCurve->nArcs ; a++)
	{
		DECUMA_INT8 * pKeyArc = pNewCurve + a*2*NUMBER_OF_POINTS_IN_ARC;
		status = resampArc(
			pCurve->pArcs[a].pPoints,
			pCurve->pArcs[a].nPoints,
			tmp,
			nPointsInArc,
			nXOffset,
			nYOffset);
		if (status != decumaNoError) return status;

		for(p = 0 ; p < nPointsInArc ; p++)
		{
			/* Move, scale and store the point. */

			decumaAssert(dScaleFactor * (tmp[p].x) <= MAX_KEY_NUMBER &&
				dScaleFactor * (tmp[p].x) >= MIN_KEY_NUMBER);
			decumaAssert(dScaleFactor * (tmp[p].y) <= MAX_KEY_NUMBER &&
				dScaleFactor * (tmp[p].y) >= MIN_KEY_NUMBER);

			/* The casts here takes us around the const qualifier */
			pKeyArc[p] = (DECUMA_INT8) (dScaleFactor * (float)tmp[p].x) ;
			pKeyArc[p+nPointsInArc] = (DECUMA_INT8) (dScaleFactor * (float)tmp[p].y) ;
		}
	}
	keyPreCalculate(pNewKey,pNewCurve, pCurve->nArcs);
	return decumaNoError;
}


static void getudmCurveBoundaries(const DECUMA_CURVE * pCurve,
		DECUMA_INT32 * pnMinX, DECUMA_INT32 * pnMaxX, DECUMA_INT32 * pnMinY, DECUMA_INT32 * pnMaxY)
{
	DECUMA_INT32 xmin=MAX_DECUMA_INT32;
	DECUMA_INT32 xmax=MIN_DECUMA_INT32;
	DECUMA_INT32 ymin=MAX_DECUMA_INT32;
	DECUMA_INT32 ymax=MIN_DECUMA_INT32;
	int a,p;

	for(a = 0 ; a < pCurve->nArcs ; a++)
	{
		for(p = 0 ; p < pCurve->pArcs[a].nPoints ; p++)
		{
			xmin = minNUMB(xmin, pCurve->pArcs[a].pPoints[p].x);
			xmax = maxNUMB(xmax, pCurve->pArcs[a].pPoints[p].x);

			ymin = minNUMB(ymin, pCurve->pArcs[a].pPoints[p].y);
			ymax = maxNUMB(ymax, pCurve->pArcs[a].pPoints[p].y);
		}
	}
	if(pnMinX)*pnMinX = (DECUMA_INT32)xmin;
	if(pnMaxX)*pnMaxX = (DECUMA_INT32)xmax;
	if(pnMinY)*pnMinY = (DECUMA_INT32)ymin;
	if(pnMaxY)*pnMaxY = (DECUMA_INT32)ymax;
}

static float getScalefactorWithoutHelplines(const DECUMA_CURVE * pCurve,
		DECUMA_INT32 keyWidth,
		DECUMA_INT32 keyHeight)
{
	DECUMA_INT32 xmin, xmax, ymin, ymax;
	DECUMA_INT32 curveWidth, curveHeight;
	float xFactor, yFactor;

	decumaAssert( keyWidth>0);
	decumaAssert( keyHeight>0);

	getudmCurveBoundaries(pCurve,&xmin,&xmax,&ymin,&ymax);
	curveWidth = xmax-xmin+1;
	curveWidth = maxNUMB(curveWidth,1);
	curveHeight = ymax-ymin+1;
	curveHeight = maxNUMB(curveHeight,1);


	xFactor = (float) keyWidth / curveWidth;
	yFactor = (float) keyHeight / curveHeight;

	return minNUMB(xFactor,yFactor);
}


#endif /* DECUMA_SPECIFIC_DB_STORAGE */
