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


#include "decumaAssert.h"
#include "database.h"
#include "scrCurve.h"
#include "scrOutput.h"
#include "scrAlgorithm.h"
#include "scrAPI.h"
#include "scrFineSearch.h"
#include "databaseFormat.h"
#include "decumaTypeCheck.h"
#include "decumaCategoriesHidden.h"
#include "udm.h"
#include "udmAccess.h"
#include "decumaStatus.h"
#include "scrHeapMem.h"
#include "decumaCategoryTranslation.h"
#include "decumaMath.h"

#include "decuma_hwr_types.h"
#include "decumaInterrupts.h"

#include "globalDefs.h"

#define SCR_CURVE_MAX_COORD 1000

#define MINIMAL_STACK_LEFT	16000


static void curveToScrCurve(const CURVE* pCurve,
							SCR_CURVE* pScrCurve);

#if defined(MEASURE_STACK_SIZE)
static char* StackPointer()
{
	char i = 9;

	return &i;
}

static void stackMeasureInit(int **aStackFloor, int **aStackRoof)
{
	int stackVar[MINIMAL_STACK_LEFT/4-128];
	int i;

	int * stackP = stackVar;

	/*CleanStack: Write a special number (0xAAAAAAAA) from the */
	/*stack pointer and down (stack is written top-down) */

	*aStackRoof = stackP;

	for(i = 0 ; i < sizeof(stackVar) / sizeof(stackVar[0]) ; i++)
	{
		*stackP++ = 0xAAAAAAAA;
	}
	*aStackFloor = stackP-1;
}
#endif /*_DEBUG && MEASURE_STACKSIZE */

int scrGetMemBufSize(void)
{
	return sizeof(SCR_HEAP_MEM);
}

DECUMA_STATUS scrSelect(SCR_OUTPUT * pOutputs, int * pnOutputs, int nMaxOutputs,
			  const SCR_API_SETTINGS * pScrApiSettings, const CURVE * pCurve,
			  const SCR_OUTPUT_LIST * pPrecalculatedBaseOutputs,
			  void * pMemoryBuffer,
			  const DECUMA_INTERRUPT_FUNCTIONS * pInterruptFunctions)
{
	int nOutputsReturned;
	DECUMA_STATUS status = decumaNoError;
#if defined(MEASURE_STACK_SIZE)
	int * stackP;
	unsigned int stackUsed;
	int * stackFloor;
	int * stackRoof;
	CAT_TABLE_PTR pCT;
	CAT_TABLE_PTR pUDMCT;


	stackMeasureInit(&stackFloor,&stackRoof);
	{
#endif /*MEASURE_STACK_SIZE */

	int a,i,j;

	SCR_OUTPUT_LIST * pPrecalcOutputCopy;
	SCR_HEAP_MEM * pScrHeap = (SCR_HEAP_MEM *) pMemoryBuffer;

	int nOutputs;

	decumaAssert(BASIC_TYPES_CORRECT_DEFINED);
	decumaAssert(pOutputs);
	decumaAssert(pScrApiSettings);
	decumaAssert(pCurve);

	decumaAssert( pCurve->nArcs <= MAX_NUMBER_OF_ARCS_IN_CURVE );
	decumaAssert( pCurve->nArcs > 0 );
	decumaAssert(!pInterruptFunctions || pInterruptFunctions->pShouldAbortRecognize);

	decumaAssert( scrCheckSettings(pScrApiSettings) == decumaNoError );

	decumaAssert( pScrHeap != 0);

	myMemSet(pScrHeap, 0, sizeof(SCR_HEAP_MEM));

	decumaAssert(sizeof(pScrHeap->scr_curve.Arcs) / sizeof(pScrHeap->scr_curve.Arcs[0]) == MAX_NUMBER_OF_ARCS_IN_CURVE);
	decumaAssert(sizeof(pScrHeap->scr_arcs) / sizeof(pScrHeap->scr_arcs[0]) == MAX_NUMBER_OF_ARCS_IN_CURVE);

	pScrHeap->scrSettings = *pScrApiSettings;

	curveInit(&pScrHeap->scr_curve);

	for(a = 0 ; a < MAX_NUMBER_OF_ARCS_IN_CURVE; a++)
	{
		pScrHeap->scr_curve.Arcs[a] = &pScrHeap->scr_arcs[a];
	}

	curveToScrCurve(pCurve, &pScrHeap->scr_curve);

	/* Translate reference lines towards origo */
	if ((pScrHeap->scrSettings.nHelpLineY != 0 ) || (pScrHeap->scrSettings.nBaseLineY!=0))
	{
		decumaAssert((DECUMA_INT32)pScrHeap->scrSettings.nHelpLineY + pScrHeap->scr_curve.offset_y <= MAX_DECUMA_INT16);
		decumaAssert((DECUMA_INT32)pScrHeap->scrSettings.nHelpLineY + pScrHeap->scr_curve.offset_y >= MIN_DECUMA_INT16);
		decumaAssert((DECUMA_INT32)pScrHeap->scrSettings.nBaseLineY + pScrHeap->scr_curve.offset_y <= MAX_DECUMA_INT16);
		decumaAssert((DECUMA_INT32)pScrHeap->scrSettings.nBaseLineY + pScrHeap->scr_curve.offset_y >= MIN_DECUMA_INT16);
		pScrHeap->scrSettings.nHelpLineY = (DECUMA_INT16)((DECUMA_INT32)pScrHeap->scrSettings.nHelpLineY + pScrHeap->scr_curve.offset_y);
		pScrHeap->scrSettings.nBaseLineY = (DECUMA_INT16)((DECUMA_INT32)pScrHeap->scrSettings.nBaseLineY + pScrHeap->scr_curve.offset_y);

		pScrHeap->scrSettings.nHelpLineY >>= pScrHeap->scr_curve.right_shift;
		pScrHeap->scrSettings.nBaseLineY >>= pScrHeap->scr_curve.right_shift;
	}

	/* Copy the timeline diff information */
	for ( a = 0; a < pCurve->nArcs-1; a++ )
		pScrHeap->scr_curve.arcTimelineDiff[a] = pCurve->arcTimelineDiff[a];
	decumaAssert(nMaxOutputs > 0);

	/* Lower than 4 will reduce recognition rate, since it will break zooming! */
	nOutputs = maxNUMB(4, nMaxOutputs);
	nOutputs = minNUMB(nOutputs, sizeof(pScrHeap->scrOutputs) / sizeof(pScrHeap->scrOutputs[0]));

	outputInitVector(pScrHeap->scrOutputs, nOutputs);

	/*The precalulated outputs that are sent in to the SCR-engine need to be */
	/*modified in the same way as the curve. Therefore a copy is made. */

	if (pPrecalculatedBaseOutputs)
	{
		int nDiacKeyLists = staticDBGetPropDB(pScrHeap->scrSettings.pStaticDB)->nDiacKeyLists;

		for( i = 0; i < nDiacKeyLists; i++ )
		{
			int nOutputsCopy = pPrecalculatedBaseOutputs[i].nOutputs;

			if (nOutputsCopy > MAX_PRECALC_OUTPUTS_USED) nOutputsCopy = MAX_PRECALC_OUTPUTS_USED;
			pScrHeap->precalcOutputCopy[i].nOutputs = nOutputsCopy;
			pScrHeap->precalcOutputCopy[i].pOut = &pScrHeap->outputCopies[i][0];
			for( j = 0; j < nOutputsCopy; j++ )
			{
				pScrHeap->precalcOutputCopy[i].pOut[j] = pPrecalculatedBaseOutputs[i].pOut[j];
			}
		}

		for( i = 0; i < nDiacKeyLists; i++ )
		{
			for( j = 0; j < pScrHeap->precalcOutputCopy[i].nOutputs; j++ )
			{
				SCR_OUTPUT *pOut = &pScrHeap->precalcOutputCopy[i].pOut[j];

				pOut->simTransf.symPoint.x += (pScrHeap->scr_curve.offset_x-1) * (DECUMA_INT32)SIM_TRANSF_ROUNDING_FACTOR;
				pOut->simTransf.symPoint.y += (pScrHeap->scr_curve.offset_y-1) * (DECUMA_INT32)SIM_TRANSF_ROUNDING_FACTOR;
				pOut->simTransf.symPoint.x >>= pScrHeap->scr_curve.right_shift;
				pOut->simTransf.symPoint.y >>= pScrHeap->scr_curve.right_shift;
				pOut->simTransf.scale <<= pScrHeap->scr_curve.right_shift;
			}
		}
		pPrecalcOutputCopy = pScrHeap->precalcOutputCopy;
	}
	else
		pPrecalcOutputCopy = NULL;


	/* Check if we should abort or continue */
	if (TEST_ABORT_RECOGNITION(pInterruptFunctions))
		goto scrSelect_abort;

	status = scrSel(pScrHeap->scrOutputs, &nOutputsReturned, nOutputs, &pScrHeap->scr_curve,
		pPrecalcOutputCopy, pScrHeap, pInterruptFunctions);
	if (status == decumaRecognitionAborted) goto scrSelect_abort;

	decumaAssert(nOutputsReturned <= nOutputs);

	if (nOutputsReturned > nMaxOutputs) nOutputsReturned = nMaxOutputs;

	decumaMemcpy(pOutputs, pScrHeap->scrOutputs, nOutputsReturned * sizeof(pOutputs[0]));

	/*The output is adjusted according to the inverse of the normalization */
	/*that was done before the engine-call. */

	for( i = 0; i < nOutputsReturned; i++ )
	{
		/* First adjust simTransf to apply to actual symbol */
		if (pOutputs[i].outSymbol == alternative)
		{
			/* Alternative symbol. Adjust simTransf to apply to this instead of original symbol. */
			if (CAN_BE_SCALED_TO_UPPER(kidGetTypeInfo(&pOutputs[i].DBindex)))
			{
				pOutputs[i].simTransf.scale = pOutputs[i].simTransf.scale * ALT_SYMBOL_SCALE_NUM / ALT_SYMBOL_SCALE_DENOM;
				pOutputs[i].simTransf.delta.x = pOutputs[i].simTransf.delta.x * ALT_SYMBOL_SCALE_NUM / ALT_SYMBOL_SCALE_DENOM;
				pOutputs[i].simTransf.delta.y = pOutputs[i].simTransf.delta.y * ALT_SYMBOL_SCALE_NUM / ALT_SYMBOL_SCALE_DENOM;
			}

			if (CAN_BE_TRANSLATED_TO_UPPER(kidGetTypeInfo(&pOutputs[i].DBindex)))
			{
				pOutputs[i].simTransf.delta.y +=
					ALT_SYMBOL_TRANSLATION * propDBGetDistBase2Help(staticDBGetPropDB(pScrHeap->scrSettings.pStaticDB)) * SIM_TRANSF_ROUNDING_FACTOR;
			}
		}

		/* symPoint is based on cutCurve, base it on scr_curve instead */
		pOutputs[i].simTransf.symPoint.x = pScrHeap->scr_curve.sum_x * SIM_TRANSF_ROUNDING_FACTOR / pScrHeap->scr_curve.mass;
		pOutputs[i].simTransf.symPoint.y = pScrHeap->scr_curve.sum_y * SIM_TRANSF_ROUNDING_FACTOR / pScrHeap->scr_curve.mass;

		pOutputs[i].simTransf.symPoint.x <<= pScrHeap->scr_curve.right_shift;
		pOutputs[i].simTransf.symPoint.y <<= pScrHeap->scr_curve.right_shift;
		pOutputs[i].simTransf.scale >>= pScrHeap->scr_curve.right_shift;
		if (pOutputs[i].simTransf.scale < 1) pOutputs[i].simTransf.scale = 1;
		pOutputs[i].simTransf.symPoint.x -= ((DECUMA_INT32)pScrHeap->scr_curve.offset_x-1) * SIM_TRANSF_ROUNDING_FACTOR;
		pOutputs[i].simTransf.symPoint.y -= ((DECUMA_INT32)pScrHeap->scr_curve.offset_y-1) * SIM_TRANSF_ROUNDING_FACTOR;
	}

#if defined(MEASURE_STACK_SIZE)
	}
	/*Measure Stack: See how many of the special number (0xAAAAAAAA) that are */
	/*still in the allocated stack space.  */
	stackP = stackRoof;

	while(*stackP == 0xAAAAAAAA && stackP < stackFloor)
	{
		stackP++;
	}

	stackUsed = (unsigned int)stackFloor - (unsigned int)stackP;
#endif /*defined(MEASURE_STACK_SIZE) */
	*pnOutputs = nOutputsReturned;
	return decumaNoError;

scrSelect_abort:
	*pnOutputs = 0;
	return decumaRecognitionAborted;
}

DECUMA_STATUS scrCheckSettings(const SCR_API_SETTINGS * pSettings)
{
	DECUMA_STATUS status;

	CATEGORY_TABLE_PTR pUdmCatTable = NULL;
	CATEGORY_TABLE_PTR pStaticCatTable = NULL;


	if ( !pSettings )
		return decumaNullPointer;

	if ( !pSettings->pStaticDB )
		return decumaNullDatabasePointer;

	if ( databaseHasWrongFormat((STATIC_DB_HEADER_PTR) pSettings->pStaticDB) )
		return decumaInvalidDatabase;

	if ( pSettings->pCategoryTable &&
		 !dcCategoryTableIsOK(pSettings->pCategoryTable) )
		return decumaInvalidCategoryTable;

	if ( pSettings->nBaseLineY < pSettings->nHelpLineY )
		return decumaInvalidBaselineHelpline;

	if ( pSettings->pUDM )
	{
		/* TODO Add error checks... */
/*		const UINT32 udmDatabaseVersionNr = udmGetDatabaseVersionNr(pSettings->pUDM); */

#if defined ( ONPALM_5 ) && defined( ONPALM_ARMLET )
		/* Armlets don't have access to the udm library. */
#else
		if ( !udmIsValid(pSettings->pUDM) ) {
			return decumaInvalidUserDatabase;
		}
#endif
/*		//JM TODO: Add function to check the database version number for scr */
/*		// Make sure that the UDM has the correct database format. */
/*		else if ( udmDatabaseVersionNr != databaseGetVersionNr() ) { */
/*			return scrApiUdmError; */
/*		} */

	}

	if (pSettings->pUDM)
	{
		KEY_DB_HEADER_PTR pUdmKeyDB = (KEY_DB_HEADER_PTR) udmGetDatabase(pSettings->pUDM);
		pUdmCatTable = keyDBGetStaticCategoryTable(pUdmKeyDB);
	}

	pStaticCatTable = pSettings->pCategoryTable;
	if (!pStaticCatTable)
	{
		STATIC_DB_HEADER_PTR pStaticDB = (STATIC_DB_HEADER_PTR) pSettings->pStaticDB;
		pStaticCatTable = keyDBGetStaticCategoryTable( staticDBGetKeyDB(pStaticDB) );
	}

	if (!pSettings->characterSet.pSymbolCategories && !pSettings->characterSet.nSymbolCategories>0)
		return decumaNullPointer;

	if (!pSettings->characterSet.pLanguages && !pSettings->characterSet.nLanguages>0)
		return decumaNullPointer;

	if (pSettings->characterSet.nSymbolCategories==0)
		return decumaNoSymbolCategories;

	if (pSettings->characterSet.nLanguages==0)
		return decumaNoLanguages;

	status = checkCharacterSetValidity(&pSettings->characterSet,1);
	if ( status != decumaNoError)
	{
		return status;
	}

	/*The categories might be valid for UDM combined with static DB... */
	status = translateToCategoryStructs(
		&pSettings->characterSet,pStaticCatTable, pUdmCatTable,
		NULL, NULL);

	if (status != decumaNoError)
	{
		return status;
	}

	return decumaNoError;
}

DECUMA_UNICODE_DB_PTR scrOutputGetSymbol(const SCR_OUTPUT * pOut)
{
	decumaAssert(pOut);
	return pOut->symbol;
}

DECUMA_INT16 scrOutputGetSymmetry(const SCR_OUTPUT * pOut)
{
	decumaAssert(pOut);
	return kidGetSymmetry(&pOut->DBindex);
}

DECUMA_UINT16 scrOutputGetWidth(const SCR_OUTPUT * pOut)
{
	decumaAssert(pOut);
	return kidGetXWidth(&pOut->DBindex);
}

DECUMA_INT16 scrOutputGetProximity(const SCR_OUTPUT * pOut)
{
	decumaAssert(pOut);
	return pOut->mu+pOut->punish;
}

DECUMA_BOOL scrOutputIsOriginal(const SCR_OUTPUT * pOut)
{
	decumaAssert(pOut);
	return (pOut->outSymbol == original);
}

DECUMA_BOOL scrOutputIsAlternative(const SCR_OUTPUT * pOut)
{
	decumaAssert(pOut);
	return (pOut->outSymbol == alternative);
}

DECUMA_BOOL scrOutputCanBeBoth(const SCR_OUTPUT * pOut)
{
	decumaAssert(pOut);
	return (pOut->outSymbol == both);
}

void scrOutputAlternativeTranslation(const SCR_OUTPUT * pOut, DECUMA_INT8* altSymbolTranslation)
{
	decumaAssert(pOut);
	if CAN_BE_TRANSLATED_TO_UPPER(kidGetTypeInfo(&pOut->DBindex))
		*altSymbolTranslation=ALT_SYMBOL_TRANSLATION;
	else
		*altSymbolTranslation=0;
}

void scrOutputAlternativeScale(const SCR_OUTPUT * pOut, DECUMA_INT8* altSymbolScaleNum, DECUMA_INT8* altSymbolScaleDenom)
{
	decumaAssert(pOut);
	if CAN_BE_SCALED_TO_UPPER(kidGetTypeInfo(&pOut->DBindex))
	{
		*altSymbolScaleNum=ALT_SYMBOL_SCALE_NUM;
		*altSymbolScaleDenom=ALT_SYMBOL_SCALE_DENOM;
	}
	else
	{
		*altSymbolScaleNum=1;
		*altSymbolScaleDenom=1;
	}
}

const SIM_TRANSF * scrOutputGetSimTransf(const SCR_OUTPUT * pOut)
{
	decumaAssert(pOut);
	return &(pOut->simTransf);
}

/* Returns an integer with value about zoom function */
DECUMA_INT8 scrOutputGetZoomValue(const SCR_OUTPUT * pOut)
{
	decumaAssert(pOut);
	return pOut->zoomValue;
}

const DECUMA_INT8 * scrOutputGetArcOrder(const SCR_OUTPUT * pOut)
{
	decumaAssert(pOut);
	return pOut->arcOrder;
}

DECUMA_BOOL scrOutputIsInterpretationOfSmallCurve(const SCR_OUTPUT * pOut,
										   const SCR_API_SETTINGS * pSettings)
{
	KID kid = pOut->DBindex;
	PROPERTY_DB_HEADER_PTR pPropDB = staticDBGetPropDB((STATIC_DB_HEADER_PTR) pSettings->pStaticDB);

	SMALL_KEY_PTR pSmallKey = propDBGetSmallKey(pPropDB);

	if (kid.pKeyDB->databaseType == DB_TYPE_STATIC &&
		kid.noBaseArcs == 1)
	{
		/* Loop through all smallkey indices to see if it is a match! */
		if ( pSettings->nBaseLineY == pSettings->nHelpLineY)
		{
			int i;
			int bMatch = 0;

			for (i = 0; i < 4; i++)
			{
				if (kid.baseKeyIndex == pSmallKey->smallKeyNoHelplines[i])
				{
					bMatch = 1;
					break;
				}
			}
			return (bMatch);
		}
		else
		{
			int i;
			int bMatch = 0;

			for (i = 0; i < 4; i++)
			{
				if (kid.baseKeyIndex == pSmallKey->smallKeyBelowHelplineUpper[i] ||
					kid.baseKeyIndex == pSmallKey->smallKeyBelowHelplineLower[i] ||
					kid.baseKeyIndex == pSmallKey->smallKeyAboveHelpline[i])
				{
					bMatch = 1;
					break;
				}
			}
			return (bMatch);
		}
	}

	return FALSE;
}

void scrOutputGetKeyMinMaxY(const SCR_OUTPUT * pOut,DECUMA_INT8 * pMinY, DECUMA_INT8 * pMaxY)
{
	kidGetMinMaxY(&pOut->DBindex,pMinY, pMaxY);
}

DEGREE_OF_UNCERTAINTY scrOutputGetSizeUncertainty(const SCR_OUTPUT * pOut)
{
	/*Assert that the casting below is OK */
	decumaAssert(db_certain == certain && db_littleuncertain==littleUncertain &&
		db_quiteUncertain == quiteUncertain && db_veryUncertain==veryUncertain);

	return (DEGREE_OF_UNCERTAINTY) keySizeUncertainty(&pOut->DBindex);
}

DEGREE_OF_UNCERTAINTY scrOutputGetPositionUncertainty(const SCR_OUTPUT * pOut)
{
	/*Assert that the casting below is OK */
	decumaAssert(db_certain == certain && db_littleuncertain==littleUncertain &&
		db_quiteUncertain == quiteUncertain && db_veryUncertain==veryUncertain);

	return (DEGREE_OF_UNCERTAINTY) keyPositionUncertainty(&pOut->DBindex);
}

int scrOutputGetAllographType( const SCR_OUTPUT * pOut )
{
	/* Returns the allograph type ID for the base key. */
	if ( databaseValidKID( pOut->DBindex ) )
	{
		SYMBOL_TYPE_INFO_PTR pTypeInfo = kidGetTypeInfo(&pOut->DBindex);
		decumaAssert( pTypeInfo );
		return pTypeInfo->typeNr;
	}
	else
		return 0;
}

void scrOutputCreateFaked(const SCR_API_SETTINGS * pSettings,
	SCR_OUTPUT * pOut, const DECUMA_UNICODE * pSymbol)
{
	decumaAssert( pOut );
	myMemSet(pOut, 0, sizeof(SCR_OUTPUT));

	/* Set kid */
	kidInitToFirstKeyInStaticDB( (STATIC_DB_HEADER_PTR) pSettings->pStaticDB,
		&pOut->DBindex );

	/* Set symbol */
	pOut->symbol = pSymbol;
	pOut->outSymbol = original;

	/* Set proximity and punish */
	pOut->mu = 1000;
	pOut->punish = 1000;

	/* Initialize simTransf to a valid value. */
	simtransfInit(&pOut->simTransf);

	/* Initialize arcOrder */
	pOut->arcOrder[0] = 1;
}

void scrOutputCreateMcrSmall(const SCR_API_SETTINGS * pSettings,
	SCR_OUTPUT * pOut, CATEGORY * pCategory)
{
	SMALL_KEY_PTR pSmallKey;
	KID * pKid = &pOut->DBindex;
	STATIC_DB_HEADER_PTR pStaticDB = (STATIC_DB_HEADER_PTR) pSettings->pStaticDB;
	CATEGORIES_PTR pKeyCategories;

	decumaAssert( pOut );

	outputInit(pOut);

	/* Set DBindex */
	pKid->pKeyDB = (KEY_DB_HEADER_PTR) staticDBGetKeyDB(pStaticDB);
	pKid->pPropDB = (PROPERTY_DB_HEADER_PTR) staticDBGetPropDB(pStaticDB);
	pKid->pCatTable = (CATEGORY_TABLE_PTR) keyDBGetStaticCategoryTable(pKid->pKeyDB);
	pSmallKey = propDBGetSmallKey(pKid->pPropDB);
	decumaAssert(pSmallKey);
	pKid->baseKeyIndex = pSmallKey->smallKeyRelativeChars[0];

	/*Only one arc - is assumed from start */
	pKid->noBaseArcs = 1;

	decumaAssert(pKid->baseKeyIndex >= 0); /*There needs to be one */
	kidSetBaseKeyPointer(pKid);

	/* Set symbol */
	pOut->symbol = kidGetSymbol(pKid);
	pOut->outSymbol = original;

	/* Set proximity and punish */
	pOut->mu = 0;
	pOut->punish = 0;
	pOut->nCutLeft = 0;
	pOut->nCutRight = 0;

	/* Initialize simTransf to a valid value. */
	simtransfInit(&pOut->simTransf);

	/* Set the key mass point correctly. */
	pOut->simTransf.delta.x = kidGetSumX(pKid) *
		SIM_TRANSF_ROUNDING_FACTOR / NUMBER_OF_POINTS_IN_ARC;  /*Assumes only one arc */
	pOut->simTransf.delta.y = kidGetSumY(pKid) *
		SIM_TRANSF_ROUNDING_FACTOR / NUMBER_OF_POINTS_IN_ARC;

	/* Initialize arcOrder */
	pOut->arcOrder[0] = 1; /*Assumes only one arc */


	/*Set the category */

	pKeyCategories = kidGetCategories(pKid);
	decumaAssert(pKeyCategories);

	*pCategory = pKeyCategories->cat; /* Ignore altcat */
}


/* Return the database distance from the baseline to the helpline */
DECUMA_INT32 scrOutputGetDbDistBase2Help(const SCR_OUTPUT * pOut)
{
	PROPERTY_DB_HEADER_PTR pPropDB;
	decumaAssert( pOut );

	pPropDB = pOut->DBindex.pPropDB;
	return propDBGetDistBase2Help( pOut->DBindex.pPropDB );
}

void scrGetCharacterDistances(const SCR_API_SETTINGS * pSettings,
							  DECUMA_INT16 * pDefaultCharDist, DECUMA_INT16 * pDefaultGestureDist,
							  DECUMA_UINT8 * pnDistInTable, CH_PAIR_DIST_CAT_PTR* ppCharDistTable)
{
	STATIC_DB_HEADER_PTR pStaticDB = (STATIC_DB_HEADER_PTR) pSettings->pStaticDB;
	PROPERTY_DB_HEADER_PTR pPropDB = (PROPERTY_DB_HEADER_PTR) staticDBGetPropDB(pStaticDB);

	MIN_DISTANCES_PTR pMinDistances = propDBGetMinDistances(pPropDB);
	decumaAssert(pMinDistances);

	*pDefaultCharDist = pMinDistances->defaultCharMinDist;
	*pDefaultGestureDist = pMinDistances->gestureMinDist;

	*pnDistInTable = pMinDistances->charDistTableLength;
	*ppCharDistTable = propDBGetCharDistanceTable(pPropDB);
}

/*
This function returns a character set translated into local static db format.
If a personal category table is provided it will be used instead of the category
table in the static database.
*/
CATEGORY scrTranslateToStaticDBCategory(STATIC_DB_PTR pStaticDB,
	CATEGORY_TABLE_PTR pCategoryTable,
	const DECUMA_CHARACTER_SET * pCharacterSet)
{
	CATEGORY cat;
	CATEGORY_TABLE_PTR pCatTable = pCategoryTable;

	if (!pCatTable)
	{
		KEY_DB_HEADER_PTR pKeyDB = staticDBGetKeyDB((STATIC_DB_HEADER_PTR)pStaticDB);
		pCatTable = keyDBGetStaticCategoryTable(pKeyDB);
	}
	translateToCategoryStructs(pCharacterSet, pCatTable, NULL, &cat, NULL);

	return cat;
}

/*
This function returns 1 if the atomic symbol category provided is included in the
character set provided (note that languages are unimportant in this case)
*/
int scrCharacterSetIncludesAtomicSymbolCategory(
	const DECUMA_CHARACTER_SET * pCharacterSet,
	DECUMA_UINT32 atomicSymbolCategory)
{
	int i,bFound=0;
	for (i=0; i<pCharacterSet->nSymbolCategories; i++)
	{
		bFound = isAtomInSymbolCategory(atomicSymbolCategory,pCharacterSet->pSymbolCategories[i]);
		if (bFound)
		{
			break;
		}
	}
	return bFound;
}

int _scrDatabaseSupportsSymbolCategory(SCR_API_ANY_DB_PTR pDB, DECUMA_UINT32 cat)
{
	DECUMA_CHARACTER_SET charSet;
	CATEGORY_TABLE_PTR pCatTable;
	int bDBIsUDM = 0;
	DECUMA_STATUS status;

	if ((status = _scrCheckStaticDB(pDB)) == decumaNoError)
		decumaAssert(!udmIsValid( (UDM_PTR) pDB));
	else if (udmIsValid( (UDM_PTR) pDB))
		bDBIsUDM = 1;
	else
		return 0;

	charSet.pSymbolCategories = &cat;
	charSet.nSymbolCategories = 1;
	charSet.pLanguages = NULL;
	charSet.nLanguages = 0;

	status = checkCharacterSetValidity(&charSet,1);
	if ( status != decumaNoError)
	{
		return 0;
	}

	pCatTable = keyDBGetStaticCategoryTable(bDBIsUDM ? (KEY_DB_HEADER_PTR)udmGetDatabase(pDB) : staticDBGetKeyDB(pDB));

	status = translateToCategoryStructs(&charSet, pCatTable, NULL, NULL, NULL);

	return (status==decumaNoError);
}

int _scrDatabaseSupportsLanguage(SCR_API_ANY_DB_PTR pDB, DECUMA_UINT32 lang)
{
	DECUMA_CHARACTER_SET charSet;
	CATEGORY_TABLE_PTR pCatTable;
	int bDBIsUDM = 0;
	DECUMA_STATUS status;

	if ((status = _scrCheckStaticDB(pDB)) == decumaNoError)
		decumaAssert(!udmIsValid( (UDM_PTR) pDB));
	else if (udmIsValid( (UDM_PTR) pDB))
		bDBIsUDM = 1;
	else
		return 0;

	charSet.pSymbolCategories = NULL;
	charSet.nSymbolCategories = 0;
	charSet.pLanguages = &lang;
	charSet.nLanguages = 1;

	pCatTable = keyDBGetStaticCategoryTable(bDBIsUDM ? (KEY_DB_HEADER_PTR)udmGetDatabase(pDB) : staticDBGetKeyDB(pDB));

	status = checkCharacterSetValidity(&charSet,1);
	if ( status != decumaNoError)
	{
		return 0;
	}

	status = translateToCategoryStructs(&charSet, pCatTable, NULL, NULL, NULL);

	return (status==decumaNoError);
}

DECUMA_STATUS _scrDatabaseIncludesSymbol(SCR_API_ANY_DB_PTR pDB,
										 const DECUMA_CHARACTER_SET * pCharSet,
										 const DECUMA_UNICODE * pSymbol,
										 int * pbIncluded)
{
	int bDBIsUDM = 0;
	DECUMA_STATUS status;

	if ((status = _scrCheckStaticDB(pDB)) == decumaNoError)
		decumaAssert(!udmIsValid( (UDM_PTR) pDB));
	else if (udmIsValid( (UDM_PTR) pDB))
		bDBIsUDM = 1;
	else
		return decumaInvalidDatabase;

	if (!pbIncluded) return decumaNullPointer;
	if (!pSymbol) return decumaNullTextPointer;

	if (pCharSet)
	{
		status = checkCharacterSetValidity(pCharSet,1);
		if ( status != decumaNoError)
			return status;
	}

	return databaseIncludesSymbol(bDBIsUDM ? (KEY_DB_HEADER_PTR)udmGetDatabase(pDB) : staticDBGetKeyDB(pDB),
		bDBIsUDM ? NULL : staticDBGetPropDB(pDB),
		pCharSet,pSymbol,pbIncluded);
}

DECUMA_STATUS _scrCheckStaticDB(STATIC_DB_PTR pDB)
{
	if (!VALID_DECUMA_BASIC_TYPES) return decumaCompilationError;

	if ( !pDB )
		return decumaNullDatabasePointer;

	if ( databaseHasWrongFormat((STATIC_DB_HEADER_PTR) pDB) )
		return decumaInvalidDatabase;

	return decumaNoError;
}

const char * scrAPIGetVersion(void)
{
	return SCR_VERSION;
}

void _scrGetCurve(SCR_CURVE* pScrCurve, void * pMemoryBuffer)
{
	SCR_HEAP_MEM * pScrHeap = (SCR_HEAP_MEM *) pMemoryBuffer;
	SCR_ARC * Arcs[MAX_NUMBER_OF_ARCS_IN_CURVE];
	int a;

	/* Copy curve except arc memory point */
	decumaAssert(sizeof(Arcs) == sizeof(pScrCurve->Arcs));
	decumaMemcpy(Arcs, pScrCurve->Arcs, sizeof(Arcs));
	*pScrCurve = pScrHeap->scr_curve;
	decumaMemcpy(pScrCurve->Arcs, Arcs, sizeof(Arcs));

	/* Copy arcs */
	for(a = 0 ; a < pScrCurve->noArcs; a++)
	{
		*pScrCurve->Arcs[a] = *pScrHeap->scr_curve.Arcs[a];
	}
}

void _scrGetCurveProp(const SCR_OUTPUT* pOutput, SCR_CURVE_PROP* pScrCurveProp, void * pMemoryBuffer)
{
	SCR_HEAP_MEM * pScrHeap = (SCR_HEAP_MEM *) pMemoryBuffer;

	curveGetProp(&pScrHeap->scr_curve, pScrCurveProp, pOutput->DBindex.noBaseArcs);
}

DECUMA_STATUS _scrIsOutputEstimateReliable(const SCR_OUTPUT* pOutput,
										   int *pnIsReliable)
{
	DECUMA_INT32 dbDistBase2Help;
	DECUMA_INT8 keyMaxX;
	DECUMA_INT8 keyMinX;
	DECUMA_INT8 keyMaxY;
	DECUMA_INT8 keyMinY;
	DECUMA_INT16 keyWidth = 0;
	DECUMA_INT16 keyHeight = 0;
	int bIsGesture, bIsInstantGesture;

	if (pnIsReliable == NULL) return decumaNullPointer;

	*pnIsReliable = 0;

	if (!pOutput) return decumaNullPointer;
	if (pOutput->simTransf.scale == 0) return decumaNullPointer; /* TODO need new error code for appropriate description */

	dbDistBase2Help = propDBGetDistBase2Help(pOutput->DBindex.pPropDB);

	kidGetMinMaxX(&pOutput->DBindex,&keyMinX,&keyMaxX);
	kidGetMinMaxY(&pOutput->DBindex,&keyMinY,&keyMaxY);

	keyWidth = (DECUMA_INT16)(32 * (keyMaxX - keyMinX) / dbDistBase2Help);
	keyHeight = (DECUMA_INT16)(32 * (keyMaxY - keyMinY) / dbDistBase2Help);

	_scrOutputIsGesture(pOutput, &bIsGesture, &bIsInstantGesture);

	if (keyHeight < 24 && keyWidth < 24 ||
		keySizeUncertainty(&pOutput->DBindex) > db_littleuncertain ||
		keyPositionUncertainty(&pOutput->DBindex) > db_littleuncertain ||
		bIsGesture)
	{
		/* For reliable estimate e.g. as input for global estimate we currently we require the template to have not */
		/* only x extension and that the templates are classified as having quite certain size and position properties. */

		return decumaNoError;
	}

	*pnIsReliable = 1;

	return decumaNoError;
}

DECUMA_STATUS _scrEstimateScalingAndVerticalOffset(const SCR_OUTPUT* pOutput,
		DECUMA_INT16* pnBaseLineYEstimate,
		DECUMA_INT16* pnHelpLineYEstimate)
{
	DECUMA_INT32 dbDistBase2Help;
	DECUMA_INT8 keyMaxX;
	DECUMA_INT8 keyMinX;
	DECUMA_INT8 keyMaxY;
	DECUMA_INT8 keyMinY;
	DECUMA_INT16 keyWidth = 0;
	DECUMA_INT16 keyHeight = 0;
	DECUMA_INT32 blEstimate,hlEstimate;

	if (pnBaseLineYEstimate == NULL) return decumaNullPointer;
	if (pnHelpLineYEstimate == NULL) return decumaNullPointer;

	/* Init to no estimate */
	*pnBaseLineYEstimate = 0;
	*pnHelpLineYEstimate = 0;

	if (!pOutput) return decumaNullPointer;
	if (pOutput->simTransf.scale == 0) return decumaNullPointer; /* TODO need new error code for appropriate description */

	dbDistBase2Help = propDBGetDistBase2Help(pOutput->DBindex.pPropDB);

	kidGetMinMaxX(&pOutput->DBindex,&keyMinX,&keyMaxX);
	kidGetMinMaxY(&pOutput->DBindex,&keyMinY,&keyMaxY);

	keyWidth = (DECUMA_INT16)(32 * (keyMaxX - keyMinX) / dbDistBase2Help);
	keyHeight = (DECUMA_INT16)(32 * (keyMaxY - keyMinY) / dbDistBase2Help);

	if (keyWidth < 24 && keyHeight < 24)
	{
		/* Reasonable scale knowledge is required to avoid bad estimation. If reasonably stable */
		/* estimation cannot be made then when don't do any estimation at all. */

		return decumaNoError;
	}

	blEstimate = (0 - pOutput->simTransf.delta.y) * SIM_TRANSF_SCALE_FACTOR / (DECUMA_INT32)pOutput->simTransf.scale + pOutput->simTransf.symPoint.y;

	hlEstimate = (blEstimate - dbDistBase2Help * SIM_TRANSF_ROUNDING_FACTOR * SIM_TRANSF_SCALE_FACTOR / (DECUMA_INT32)pOutput->simTransf.scale) / SIM_TRANSF_ROUNDING_FACTOR;
	blEstimate /= SIM_TRANSF_ROUNDING_FACTOR;
	decumaAssert(hlEstimate<=MAX_DECUMA_INT16 && hlEstimate>=MIN_DECUMA_INT16);
	decumaAssert(blEstimate<=MAX_DECUMA_INT16 && blEstimate>=MIN_DECUMA_INT16);
	*pnHelpLineYEstimate = (DECUMA_INT16)hlEstimate; /*Casting is safe since we have done assert */
	*pnBaseLineYEstimate = (DECUMA_INT16)blEstimate; /*Casting is safe since we have done assert */

	return decumaNoError;
}

DECUMA_STATUS _scrGetScalingAndVerticalOffsetPunish(const SCR_OUTPUT* pOutput,
													const SCR_CURVE_PROP* pScrCurveProp,
													DECUMA_INT16 nBaseLineY,
													DECUMA_INT16 nHelpLineY,
													DECUMA_INT16 *pnPunish)
{
	int nAdjustmentFactor;

	if (!pOutput) return decumaNullPointer;
	if (!pnPunish) return decumaNullPointer;

	/* Translate reference lines towards origo */
	decumaAssert((DECUMA_INT32)nHelpLineY + pScrCurveProp->offset_y <= MAX_DECUMA_INT16);
	decumaAssert((DECUMA_INT32)nHelpLineY + pScrCurveProp->offset_y >= MIN_DECUMA_INT16);
	decumaAssert((DECUMA_INT32)nBaseLineY + pScrCurveProp->offset_y <= MAX_DECUMA_INT16);
	decumaAssert((DECUMA_INT32)nBaseLineY + pScrCurveProp->offset_y >= MIN_DECUMA_INT16);
	nHelpLineY = (DECUMA_INT16)(nHelpLineY + pScrCurveProp->offset_y);
	nBaseLineY = (DECUMA_INT16)(nBaseLineY + pScrCurveProp->offset_y);
	nHelpLineY >>= pScrCurveProp->right_shift;
	nBaseLineY >>= pScrCurveProp->right_shift;

	/* pOutput->simTransf (original curve) is inconsistent with pScrCurve (transformed curve)
	   Currently not used by PunishForScalingAndVerticalOffset though
	   TODO handle
	*/

	/* TODO cut curve should ideally be used here for consistent result */
	*pnPunish = PunishForScalingAndVerticalOffset(
		pOutput, pScrCurveProp, &pOutput->DBindex, nBaseLineY, nHelpLineY, &nAdjustmentFactor);

	if (nAdjustmentFactor)
	{
		/* Let this function be about scale and position only and thus unadjust for small stuff for better consistency */

		*pnPunish = (DECUMA_INT16)(*pnPunish/nAdjustmentFactor);
	}

	return decumaNoError;
}

DECUMA_STATUS _scrGetRotationPunish(const SCR_OUTPUT* pOutput,
									const SCR_CURVE_PROP* pScrCurveProp,
									DECUMA_INT16 nBaseLineY,
									DECUMA_INT16 nHelpLineY,
									int nRefAngle,
									DECUMA_INT16 *pnPunish)
{
	*pnPunish = (DECUMA_INT16)valueTransf(pOutput->simTransf.theta, &pOutput->DBindex, nRefAngle, 1000);

	if (scrCurveAndKeyAreQuiteSmall(pOutput, pScrCurveProp, &pOutput->DBindex, nBaseLineY, nHelpLineY))
	{
		/* Small stuff */
		/* Shift distance focus to position (and scale) */
		*pnPunish /= 2;
	}

	if (scrCurveIsTooSmallForProximityMeasure(pScrCurveProp, nBaseLineY, nHelpLineY))
	{
		/* Really small stuff */
		/* Shift distance focus even more to position (and scale) */
		*pnPunish /= 2;
	}

	return decumaNoError;
}

DECUMA_STATUS _scrAdjustMuAndPunish(SCR_OUTPUT* pOutput,
								    const SCR_CURVE_PROP* pScrCurveProp,
								    DECUMA_INT16 nNewBaseLineY,
								    DECUMA_INT16 nNewHelpLineY,
								    DECUMA_INT16 nOldBaseLineY,
								    DECUMA_INT16 nOldHelpLineY)
{
	DECUMA_INT16 nMu, nOtherPunish, nScalingAndVerticalOffsetPunish;
	int nAdjustmentFactor;

	if (!pOutput) return decumaNullPointer;
	if (!pScrCurveProp) return decumaNullPointer;

	if (nNewBaseLineY == nOldBaseLineY && nNewHelpLineY == nOldHelpLineY) return decumaNoError;

	/* Translate reference lines towards origo */
	decumaAssert((DECUMA_INT32)nNewHelpLineY + pScrCurveProp->offset_y <= MAX_DECUMA_INT16);
	decumaAssert((DECUMA_INT32)nNewHelpLineY + pScrCurveProp->offset_y >= MIN_DECUMA_INT16);
	decumaAssert((DECUMA_INT32)nNewBaseLineY + pScrCurveProp->offset_y <= MAX_DECUMA_INT16);
	decumaAssert((DECUMA_INT32)nNewBaseLineY + pScrCurveProp->offset_y >= MIN_DECUMA_INT16);
	nNewHelpLineY = (DECUMA_INT16)(nNewHelpLineY + pScrCurveProp->offset_y);
	nNewBaseLineY = (DECUMA_INT16)(nNewBaseLineY + pScrCurveProp->offset_y);
	nNewHelpLineY >>= pScrCurveProp->right_shift;
	nNewBaseLineY >>= pScrCurveProp->right_shift;

	decumaAssert((DECUMA_INT32)nOldHelpLineY + pScrCurveProp->offset_y <= MAX_DECUMA_INT16);
	decumaAssert((DECUMA_INT32)nOldHelpLineY + pScrCurveProp->offset_y >= MIN_DECUMA_INT16);
	decumaAssert((DECUMA_INT32)nOldHelpLineY + pScrCurveProp->offset_y <= MAX_DECUMA_INT16);
	decumaAssert((DECUMA_INT32)nOldHelpLineY + pScrCurveProp->offset_y >= MIN_DECUMA_INT16);
	nOldHelpLineY = (DECUMA_INT16)(nOldHelpLineY + pScrCurveProp->offset_y);
	nOldBaseLineY = (DECUMA_INT16)(nOldBaseLineY + pScrCurveProp->offset_y);
	nOldHelpLineY >>= pScrCurveProp->right_shift;
	nOldBaseLineY >>= pScrCurveProp->right_shift;

	/* pOutput->simTransf (original curve) is inconsistent with pScrCurve (transformed curve)
	   Currently not used by PunishForScalingAndVerticalOffset though
	   TODO handle
	*/

	/* Just adjust for new scale and position features */
	nMu = pOutput->mu;
	nOtherPunish = pOutput->punish;

	/* TODO cut curve should ideally be used here for consistent result */
	nScalingAndVerticalOffsetPunish = PunishForScalingAndVerticalOffset(
		pOutput, pScrCurveProp, &pOutput->DBindex, nOldBaseLineY, nOldHelpLineY, &nAdjustmentFactor);

	nOtherPunish = (DECUMA_INT16)(nOtherPunish - nScalingAndVerticalOffsetPunish);

	if (nAdjustmentFactor)
	{
		decumaAssert((DECUMA_INT32)nMu * nAdjustmentFactor <= MAX_DECUMA_INT16);
		decumaAssert((DECUMA_INT32)nOtherPunish * nAdjustmentFactor <= MAX_DECUMA_INT16);
		nMu = (DECUMA_INT16)(nMu*nAdjustmentFactor);
		nOtherPunish = (DECUMA_INT16)(nOtherPunish*nAdjustmentFactor);
	}

	/* TODO cut curve should ideally be used here for consistent result */
	nScalingAndVerticalOffsetPunish = PunishForScalingAndVerticalOffset(
		pOutput, pScrCurveProp, &pOutput->DBindex, nNewBaseLineY, nNewHelpLineY, &nAdjustmentFactor);

	pOutput->mu = nMu;
	pOutput->punish = nOtherPunish;

	if (nAdjustmentFactor)
	{
		pOutput->mu = (DECUMA_INT16)(pOutput->mu/nAdjustmentFactor);
		pOutput->punish = (DECUMA_INT16)(pOutput->punish/nAdjustmentFactor);
	}

	decumaAssert((DECUMA_INT32)pOutput->punish + nScalingAndVerticalOffsetPunish <=MAX_DECUMA_INT16);
	pOutput->punish = pOutput->punish + nScalingAndVerticalOffsetPunish;

	return decumaNoError;
}

DECUMA_STATUS _scrOutputIsGesture(const SCR_OUTPUT* pOutput,
								  int * pbGesture,
								  int * pbInstantGesture)
{
	if (!pbGesture) return decumaNullPointer;
	if (!pbInstantGesture) return decumaNullPointer;
	if (!pOutput) return decumaNullPointer;

	*pbGesture = 0;
	*pbInstantGesture = 0;

	*pbGesture = kidIsGesture(&pOutput->DBindex, pbInstantGesture);

	return decumaNoError;
}

int _scrOutputInCategory(const SCR_OUTPUT* pOutput,
						 const CATEGORY Category)
{
	int bInCat, bInAltCat;

	databaseCheckCategory(&bInCat, &bInAltCat, &pOutput->DBindex, Category);

	if (bInCat && pOutput->outSymbol != alternative) return 1;
	if (bInAltCat && pOutput->outSymbol != original) return 1;

	return 0;
}

int scrOutputIsMultitouch(const SCR_OUTPUT* pOutput)
{
	DECUMA_UINT8 multiMask;
	int i;

	multiMask=0;
	for (i=0; i<pOutput->DBindex.noBaseArcs-1; i++)
	{
		multiMask |= (1<<i);
	}
	if ((pOutput->arcTimelineDiffMask & multiMask) != multiMask)
		return 1;

	return 0;
}

DECUMA_UINT8 scrOutputGetArcTimelineDiffMask(const SCR_OUTPUT* pOutput)
{
	return pOutput->arcTimelineDiffMask;
}

static void curveToScrCurve(const CURVE* pCurve,
							SCR_CURVE* pScrCurve)
{
	/* Build pScrCurve from pCurve. */
	/* 1. Convert the input arcs to MAX_NUMBER_OF_ARCS_IN_CURVE point scr_arcs */
	/* 2. Attach the scr_arcs to the scr_curve (done in the same loop as 1.) */
	/* 3. Precalculate the scr_curve from the scr_arcs. */

	DECUMA_INT32 a,p,curve_min_x,curve_min_y,curve_max_x,curve_max_y, maxDiff;

	pScrCurve->noArcs = pCurve->nArcs;

	curve_min_x = MAX_DECUMA_INT16; /*Assumes 16bit coordinates*/
	curve_min_y = MAX_DECUMA_INT16;
	curve_max_x = MIN_DECUMA_INT16;
	curve_max_y = MIN_DECUMA_INT16;

	for(a = 0 ; a < pCurve->nArcs; a++)
	{
		ARC *pArc = pCurve->pArc[a];
		decumaAssert(pArc);
		for(p = 0 ; p < NUMBER_OF_POINTS_IN_ARC ; p++ )
		{
			int x = pArc->point[p].x, y = pArc->point[p].y ;

			if(  x< curve_min_x )
			{
				curve_min_x = x;
			}
			if( y< curve_min_y )
			{
				curve_min_y = y;
			}
			if( x > curve_max_x )
			{
				curve_max_x = x;
			}
			if( y > curve_max_y )
			{
				curve_max_y = y;
			}
		}
	}
	if( curve_min_x == MAX_DECUMA_INT16 )
	{
		curve_min_x = 0;
	}
	if( curve_min_y == MAX_DECUMA_INT16 )
	{
		curve_min_y = 0;
	}
	if( curve_max_x == MIN_DECUMA_INT16 )
	{
		curve_max_x = 0;
	}
	if( curve_max_y == MIN_DECUMA_INT16 )
	{
		curve_max_y = 0;
	}

	decumaAssert(curve_min_x <= MAX_DECUMA_INT16+1);
	decumaAssert(curve_min_x >= MIN_DECUMA_INT16+1);
	decumaAssert(curve_min_y <= MAX_DECUMA_INT16+1);
	decumaAssert(curve_min_y >= MIN_DECUMA_INT16+1);
	pScrCurve->offset_x = (DECUMA_INT16)(1 - curve_min_x);
	pScrCurve->offset_y = (DECUMA_INT16)(1 - curve_min_y);

#ifndef max
#define max(a,b) (((a)>(b)) ? (a) : (b))
#define min(a,b) (((a)<(b)) ? (a) : (b))
#endif

	pScrCurve->right_shift = 0;
	maxDiff = max(curve_max_x - curve_min_x,curve_max_y - curve_min_y);
	if ( maxDiff > SCR_CURVE_MAX_COORD )
	{
		pScrCurve->right_shift++; /* Fix for Palm ARM compiler that cannot deal with zero shift (>>0) */
		while ( ( maxDiff>> pScrCurve->right_shift) > SCR_CURVE_MAX_COORD)
		{
			pScrCurve->right_shift++;
		}
	}

	/* Copy the sequence information */
	for ( a = 0; a < pCurve->nArcs; a++ )
		pScrCurve->arcTimelineDiff[a] = pCurve->arcTimelineDiff[a];

	/* Translate curve towards origo */
	for(a = 0 ; a < pScrCurve->noArcs; a++)
	{
		SCR_ARC *pScrArc = pScrCurve->Arcs[a];
		ARC *pArc = pCurve->pArc[a];



		decumaAssert(pArc);

		for(p = 0 ; p < NUMBER_OF_POINTS_IN_ARC ; p++ )
		{
			decumaAssert((DECUMA_INT32)pArc->point[p].x + pScrCurve->offset_x >= MIN_DECUMA_INT16);
			decumaAssert((DECUMA_INT32)pArc->point[p].x + pScrCurve->offset_x <=MAX_DECUMA_INT16);
			decumaAssert((DECUMA_INT32)pArc->point[p].y + pScrCurve->offset_y >= MIN_DECUMA_INT16);
			decumaAssert((DECUMA_INT32)pArc->point[p].y + pScrCurve->offset_y <=MAX_DECUMA_INT16);
			pScrArc->x[p] = pArc->point[p].x + pScrCurve->offset_x;
			pScrArc->y[p] = pArc->point[p].y + pScrCurve->offset_y;
			pScrArc->x[p] >>= pScrCurve->right_shift;
			pScrArc->y[p] >>= pScrCurve->right_shift;
		}
		pScrArc->alpha_x = g_alpha_x; /* TODO More sophisticated specification */
		pScrArc->alpha_y = g_alpha_y; /* TODO More sophisticated specification */
		pScrArc->alpha_dx = g_alpha_dx; /* TODO More sophisticated specification */
		pScrArc->alpha_dy = g_alpha_dy; /* TODO More sophisticated specification */
		arcPrecalculate(pScrArc, NULL);/* the third parameter is a pointer. */
	}

	preCalculateCurveFromArcData(pScrCurve, 0); /* measure ID  */
}
