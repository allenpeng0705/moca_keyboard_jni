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


/************************************************** Description ***\

\******************************************************************/

#define ASL_C

#include "aslConfig.h"

#include "asl.h"

#include "decumaAssert.h"
#include "decumaMemory.h"
#include "decumaString.h"
#include "decumaCommon.h"
#include "decumaCategoryTranslation.h"
#include "decumaCategoryTableType.h"
#include "decumaDictAccess.h"
#include "aslData.h"
#include "aslArcSession.h"
#include "aslSG.h"
#include "aslRG.h"
#include "aslTools.h"

#include "decumaDictionary.h"
#include "decumaDictionaryXT9.h"
#include "decumaTrigramTable.h"

#include "decumaCategories.h"

#include "aslVersion.h"
#include "scrlib.h"  /*For database functions */
#include "udmlib.h"  /*For dynamic database functions */
#include "decumaSymbolCategories.h" /* for DECUMA_CATEGORY_RESERVED_END */
#include "decumaLanguages.h" /* for DECUMA_LANG_RESERVED_END */

/*#define DEBUG_OUTPUT */
#ifdef DEBUG_OUTPUT
#include <stdio.h>
FILE * g_debugFile;
#endif

/*Note the header file aslHiddenData.h includes datatypes that are used by */
/*the hidden api but that are not ifdeffed because that would complicate */
/*the code more and need a lot of more ifdefs in the code */
/*#include "aslHiddenData.h" */

#ifdef ASL_HIDDEN_API
#include "aslHiddenApi.h"
#endif


#include "decumaStorageSpecifiers.h" /* for DECUMA_SPECIFIC_DB_STORAGE */
#if !defined(DECUMA_SPECIFIC_DB_STORAGE) || !(DECUMA_SPECIFIC_DB_STORAGE == 1 || DECUMA_SPECIFIC_DB_STORAGE == 0)
#error DECUMA_SPECIFIC_DB_STORAGE must be defined to 1 or 0
#endif

/* Check if DECUMA_DB_STORAGE is defined. This is not yet supported for asllib */
#if DECUMA_SPECIFIC_DB_STORAGE == 1
#error DECUMA_DB_STORAGE other than empty is not yet supported by asllib.
#endif


/* */
/* Private macro definitions */
/* */

#define NUM_DICTIONARIES	2	/* Two dictionaries right now; one static, one PD */
#define STATIC_DICTIONARY	0	/* index of the static dict */
#define PERSONAL_DICTIONARY	1	/* index of the personal dict */

#define SESSION_NOT_INITIALIZED(pSession) ((pSession)->pArcSession != (ASL_ARC_SESSION *)&(pSession)[1])
#define USE_TERSE_TRIE

/* */
/* Private function declarations */
/* */

/*Check if all symbols are included in the database */
static DECUMA_STATUS allSymbolsInDatabase(DECUMA_UNICODE * pCharSetExt,
							DECUMA_STATIC_DB_PTR pStaticDB,
							int * pbIncluded);


/* Set a pointer to DECUMA_SESSION_SETTINGS struct. */
static DECUMA_STATUS setSessionSettings(ASL_SESSION * pSession,
										const DECUMA_SESSION_SETTINGS * pSessionSettings);

static DECUMA_STATUS recognize( ASL_SESSION * pSession,
	DECUMA_HWR_RESULT * pResults,
	DECUMA_UINT16 nMaxResults,
	DECUMA_UINT16 * pnResults,
	DECUMA_UINT16 nMaxCharsPerResult,
	const DECUMA_RECOGNITION_SETTINGS * pRecognitionSettings,
	int bOnlyFromSG,
	void * pCharArcIdxData,
	const DECUMA_UNICODE * forceString,
	const DECUMA_INTERRUPT_FUNCTIONS * pInterruptFunctions);

static DECUMA_STATUS getStringType(ASL_SESSION * pSession,
								   DECUMA_UNICODE * pChars, int nChars,
								   DECUMA_STRING_TYPE *pStringType,
								   DECUMA_INT16 * pDictStrStart,
								   DECUMA_INT16 * pDictStrLen);

static DECUMA_UINT32	getMeanDistance(ASL_SESSION * pSession, DECUMA_UINT32 nTotalDist, int nNodes);

static DECUMA_STATUS getAslSessionStatus(const ASL_SESSION * pSession);

static DECUMA_STATUS attachDictionary(ASL_SESSION * pSession,
									  const void * pDictionaryData);

/*///////////////////////////////////////////////////////////////////////////////////////// */
/* */
/* Public function definitions */
/* */

/****** Version ********/

const char * aslGetVersion(void)
{
	return ASLLIB_VERSION_STR;
}


/****** Initialization and destruction ********/

DECUMA_UINT32 aslGetSessionSize(void)
{
	return sizeof(ASL_SESSION) + aslArcSessionGetSize() + aslSGGetSize() + aslRGGetSize();
}



DECUMA_STATUS aslBeginSession(ASL_SESSION * pSession,
	const DECUMA_SESSION_SETTINGS * pSessionSettings,
	const DECUMA_MEM_FUNCTIONS * pMemFunctions)
{
	DECUMA_STATUS status =decumaNoError;

	decumaAssert(pSession->pArcSession != (ASL_ARC_SESSION *)&pSession[1]); /*Session already initialized */
	decumaAssert(pMemFunctions);

	decumaMemset(pSession,0,aslGetSessionSize());

	pSession->pMemFunctions = pMemFunctions;

	/*The Arc Session is placed right after the ASL_SESSION in the memory chunk */
	pSession->pArcSession = (ASL_ARC_SESSION *)&pSession[1];

	pSession->pSG = (ASL_SG*)((char*)pSession->pArcSession + aslArcSessionGetSize());

	pSession->pRG = (ASL_RG*)((char*)pSession->pSG + aslSGGetSize());

	status = setSessionSettings(pSession,pSessionSettings);
	if (status != decumaNoError)
	{
		decumaAssert(status == decumaAllocationFailed);
		goto aslBeginSession_error;
	}

	return status;

aslBeginSession_error:

	return status;
}

DECUMA_STATUS aslVerifySession(const ASL_SESSION * pSession)
{
	DECUMA_STATUS status = getAslSessionStatus(pSession);

	return status;
}

DECUMA_STATUS aslEndSession(ASL_SESSION * pSession)
{
	int i;
	const DECUMA_MEM_FUNCTIONS* pMemFunctions = pSession->pMemFunctions;

	/*aslEndArcAddition(pSession); //Should already have been called by outer layer */
	decumaAssert(!pSession->bAddingArcs);
	pSession->pArcSession = NULL;

	for (i=0; i<pSession->nDictionaries; i++)
	{
		aslFree(pSession->pDictionaries[i]);
	}
	aslFree(pSession->pDictionaries);
	pSession->pDictionaries = NULL;

	if (pSession->pCatTable) aslFree(*((struct _tagCATEGORY_TABLE**)&pSession->pCatTable));
	if (pSession->pCharSet) aslFree(pSession->pCharSet);

	if (pSession->pDictionaryFilterStr) aslFree(pSession->pDictionaryFilterStr);
	if (pSession->forcedSegmentation) aslFree(pSession->forcedSegmentation);

	if (pSession->pnBaseLineEstimates) aslFree(pSession->pnBaseLineEstimates);
	if (pSession->pnHelpLineEstimates) aslFree(pSession->pnHelpLineEstimates);

	if (pSession->pSymbolStrokes) aslFree(pSession->pSymbolStrokes);
	if (pSession->pSymbolChars) aslFree(pSession->pSymbolChars);

	return decumaNoError;
}




/******* Database functions *********/

/******* Internal *******************/
DECUMA_STATUS aslValidateStaticDatabase(DECUMA_STATIC_DB_PTR pStaticDB)
{
	return scrCheckStaticDB( (STATIC_DB_PTR) pStaticDB);
}

DECUMA_STATUS aslValidateDynamicDatabase(DECUMA_DYNAMIC_DB_PTR pDynamicDB)
{
	if (!udmIsValid( (UDM_PTR) pDynamicDB))
		return decumaInvalidDatabase;

	return decumaNoError;
}

/******* Exported *********************/

DECUMA_STATUS aslDatabaseGetVersion(const void * pDB, char * pBuf, int nBufLen)
{
	decumaAssert(pDB);
	decumaAssert(pBuf );
	decumaAssert(nBufLen >= DECUMA_DATABASE_VERSION_STRING_LENGTH);

	if (aslValidateStaticDatabase(pDB) == decumaNoError)
		;
	else if (aslValidateDynamicDatabase(pDB) == decumaNoError)
		;
	else
		return decumaInvalidDatabase;

	decumaAssert(nBufLen >= 19);
	decumaMemcpy (pBuf, "Unknown DB version", 19);

	return decumaNoError;
}


DECUMA_STATUS aslDatabaseIsCategorySupported(const void * pDB,
	DECUMA_UINT32 cat, int * pbIsSupported)
{
	DECUMA_CHARACTER_SET charSet;
	DECUMA_STATUS status;

	decumaAssert(pDB);
	decumaAssert(pbIsSupported);

	*pbIsSupported =0;

	charSet.pSymbolCategories = &cat;
	charSet.nSymbolCategories = 1;
	charSet.pLanguages = NULL;
	charSet.nLanguages = 0;

	status = checkCharacterSetValidity(&charSet,1);
	if ( status != decumaNoError)
	{
		return status;
	}

	if (aslValidateStaticDatabase(pDB) == decumaNoError)
		;
	else if (aslValidateDynamicDatabase(pDB) == decumaNoError)
		;
	else
		return decumaInvalidDatabase;

	*pbIsSupported = scrDatabaseSupportsSymbolCategory(pDB, cat);

	return decumaNoError;
}


DECUMA_STATUS aslDatabaseIsLanguageSupported(const void * pDB,
											DECUMA_UINT32 lang, int * pbIsSupported)
{
	DECUMA_CHARACTER_SET charSet;
	DECUMA_STATUS status;

	decumaAssert(pDB);
	decumaAssert(pbIsSupported);

	*pbIsSupported =0;

	charSet.pSymbolCategories = NULL;
	charSet.nSymbolCategories = 0;
	charSet.pLanguages = &lang;
	charSet.nLanguages = 1;

	status = checkCharacterSetValidity(&charSet,1);
	if ( status != decumaNoError)
	{
		return status;
	}

	if (aslValidateStaticDatabase(pDB) == decumaNoError)
		;
	else if (aslValidateDynamicDatabase(pDB) == decumaNoError)
		;
	else
		return decumaInvalidDatabase;

	*pbIsSupported = scrDatabaseSupportsLanguage(pDB, lang);

	return decumaNoError;
}


DECUMA_STATUS aslDatabaseIncludesSymbol(
	const void * pDB,
	const DECUMA_CHARACTER_SET * pCharSet,
	const DECUMA_UNICODE * pSymbol,
	int * pbIsIncluded)
{
	DECUMA_STATUS status;

	decumaAssert(pSymbol);

	if (aslValidateStaticDatabase(pDB) == decumaNoError)
		;
	else if (aslValidateDynamicDatabase(pDB) == decumaNoError)
		;
	else
		return decumaInvalidDatabase;

	status = scrDatabaseIncludesSymbol(
		pDB,
		pCharSet,
		pSymbol,
		pbIsIncluded);

	return status;
}

/************ Dynamic Database Functions ******************/

DECUMA_STATUS aslDynamicDatabaseIsValid(DECUMA_DYNAMIC_DB_PTR pDynamicDB) {
	return aslValidateDynamicDatabase(pDynamicDB);
}

DECUMA_STATUS aslCreateDynamicDatabase(DECUMA_DYNAMIC_DB_PTR * ppDynamicDB, const DECUMA_MEM_FUNCTIONS * pMemFunctions)
{
	DECUMA_STATUS status;

	if (ppDynamicDB == NULL) return decumaNullPointer;

	*ppDynamicDB = (DECUMA_DYNAMIC_DB_PTR) udmCreateDynamic(pMemFunctions);
	if (*ppDynamicDB != NULL) {
		status = aslValidateDynamicDatabase(*ppDynamicDB);
		if (status != decumaNoError) {
			goto aslCreateDynamicDatabase_exit;
		}
	}
	else {
		return decumaAllocationFailed;
	}

	return decumaNoError;

aslCreateDynamicDatabase_exit:
	udmDestroyDynamic((UDM_PTR)*ppDynamicDB, pMemFunctions);
	return status;
}

/* Add allograph to the dynamic database, with the specified character set.
 * Since the dltlib compression routine handles scaling/translation, we can add an unbounded curve (i.e. no need to pass constraints).
 */
DECUMA_STATUS aslAddAllograph(      DECUMA_DYNAMIC_DB_PTR   * ppDynamicDB,
								 const DECUMA_CURVE         * pCurve,
								 const DECUMA_UNICODE       * pUnicode, DECUMA_UINT32 nUnicodes,
								 const DECUMA_CHARACTER_SET * pCharacterSet,
								 DECUMA_INT32 nBaseline, DECUMA_INT32 nHelpline,
								 int bGesture, int bInstantGesture,
								 const DECUMA_MEM_FUNCTIONS * pMemFunctions)
{
	DECUMA_STATUS status;

	status = aslValidateDynamicDatabase(*ppDynamicDB);
	if (status != decumaNoError) return status;

	return udmAddAllograph((UDM_PTR*)ppDynamicDB, pCurve, pUnicode, (int) nUnicodes, pCharacterSet,
			0 /* nSymbolType */, nBaseline, nHelpline, bGesture, bInstantGesture, pMemFunctions);
}

/* Get actual number of bytes used. All template data is stored contiguously, so this function can be used to serialize the database to some persistent storage. */
DECUMA_STATUS aslGetDynamicDatabaseByteSize(DECUMA_DYNAMIC_DB_PTR pDynamicDB, DECUMA_UINT32 * pSize)
{
	DECUMA_STATUS status;

	decumaAssert(pSize);

	status = aslValidateDynamicDatabase(pDynamicDB);
	if (status != decumaNoError) return status;

	*pSize = udmGetByteSize( (UDM_PTR) pDynamicDB);

	return decumaNoError;
}

void aslDestroyDynamicDatabase(DECUMA_DYNAMIC_DB_PTR * ppDynamicDB, const DECUMA_MEM_FUNCTIONS * pMemFunctions)
{
	if (*ppDynamicDB != NULL) udmDestroyDynamic((UDM_PTR)*ppDynamicDB, pMemFunctions);
	*ppDynamicDB = NULL;
}

/******* Settings *********/


DECUMA_STATUS aslChangeSessionSettings( ASL_SESSION * pSession,
	const DECUMA_SESSION_SETTINGS * pSessionSettings)
{
	DECUMA_STATUS status;

	decumaAssert(getAslSessionStatus(pSession) == decumaNoError); /*Should have been checked already */

	/* If validateSessionSettings succeeds, it should not be possible for setSessionSettings to fail */
	status = setSessionSettings(pSession, pSessionSettings);
	if (status != decumaNoError)
	{
		decumaAssert( status == decumaAllocationFailed);
	}

	return status;
}

DECUMA_STATUS aslValidateSessionSettings(
	const DECUMA_SESSION_SETTINGS * pSessionSettings)
{
	int i;
	DECUMA_STATUS status;
	const DECUMA_CHARACTER_SET * pCharSet = &pSessionSettings->charSet;

	/* Asl still requires staticDB whereas API does not */
	if ( pSessionSettings->pStaticDB == NULL ) return decumaNullDatabasePointer;

	decumaAssert(aslValidateStaticDatabase(pSessionSettings->pStaticDB) == decumaNoError);
	decumaAssert(pSessionSettings->pDynamicDB == 0 || aslValidateDynamicDatabase(pSessionSettings->pDynamicDB) == decumaNoError);

	if (pSessionSettings->pCharSetExtension && pSessionSettings->pCharSetExtension[0])
	{
		/*We need one placeholder in the charSet for the internal values used for the category in the cat table */
		if (pSessionSettings->charSet.nSymbolCategories >= DECUMA_MAX_CATEGORIES-1)
			return decumaTooManySymbolCategories;

		if (pSessionSettings->charSet.nLanguages >= DECUMA_MAX_CATEGORIES-1)
			return decumaTooManyLanguages;
	}

	/*Asl specific validation of session settings. */
	/*General validation is performed in outer layer. */
	if (pSessionSettings->recognitionMode != scrMode &&
		pSessionSettings->recognitionMode != mcrMode)
	{
		return decumaUnsupportedRecognitionMode;
	}

	if (pSessionSettings->writingDirection != rightToLeft &&
		pSessionSettings->writingDirection != leftToRight &&
		pSessionSettings->writingDirection != onTopWriting &&
		pSessionSettings->writingDirection != unknownWriting)
	{
		return decumaUnsupportedWritingDirection;
	}

	status = checkCharacterSetValidity(&(pSessionSettings->charSet), 1);
	if (status != decumaNoError)
		return status;


	for (i=0; i<pCharSet->nLanguages;i++)
	{
		if (!scrDatabaseSupportsLanguage(pSessionSettings->pStaticDB, pCharSet->pLanguages[i]) )
		{
			if (!pSessionSettings->pDynamicDB ||
				!scrDatabaseSupportsLanguage(pSessionSettings->pDynamicDB,pCharSet->pLanguages[i]))
			{
				return decumaUnsupportedLanguage;
			}
		}
	}

	for (i=0; i<pCharSet->nSymbolCategories;i++)
	{
		if (!scrDatabaseSupportsSymbolCategory((STATIC_DB_PTR) pSessionSettings->pStaticDB,
			pCharSet->pSymbolCategories[i]))
		{
			if (!pSessionSettings->pDynamicDB ||
				!scrDatabaseSupportsSymbolCategory(pSessionSettings->pDynamicDB,pCharSet->pSymbolCategories[i]))
			{
				return decumaUnsupportedSymbolCategory;
			}
		}
	}


	if (pSessionSettings->pCharSetExtension && pSessionSettings->pCharSetExtension[0])
	{
		/*Check all the symbols in the dynamic category. They should be in the database, */
		/*or else we return an error value */
		int bIncluded;
		status = allSymbolsInDatabase(pSessionSettings->pCharSetExtension,
			pSessionSettings->pStaticDB, &bIncluded);
		if (status != decumaNoError) return status;
		if (!bIncluded) return decumaSymbolNotInDatabase;
	}

	switch (pSessionSettings->UIInputGuide)
	{
	case supportlines:
		if (pSessionSettings->supportLineSet == noSupportlines) {
			return decumaInvalidUIInputGuide;
		}
		break;
	case none:
		if (pSessionSettings->supportLineSet != noSupportlines) {
			return decumaInvalidUIInputGuide;
		}
		break;
	default: /* e.g. box */
		return decumaInvalidUIInputGuide;
		break;
	}

	switch (pSessionSettings->supportLineSet)
	{
	case baselineAndHelpline:
	case baselineAndTopline:
	case helplineAndTopline:
	case noSupportlines:
		/*OK */
		break;
	case toplineOnly:
	case baselineOnly:
	case helplineOnly:
		return decumaUnsupportedSupportLines;
		break;
	default: decumaAssert(0); /*Should already have been checked in API layer */
	}

	return decumaNoError;
}



/****** Recognition functions ********/


DECUMA_STATUS aslBeginArcAddition(ASL_SESSION * pSession)
{
	DECUMA_STATUS status;
	const DECUMA_MEM_FUNCTIONS* pMemFunctions = pSession->pMemFunctions;

	decumaAssert(getAslSessionStatus(pSession) == decumaNoError); /*Should have been checked already */

	pSession->nPrevBaselinePos = pSession->baselinePos;
	pSession->nPrevDistBaseToHelpLine = pSession->distBaseToHelpLine;
	pSession->bPrevReferenceEstimated = pSession->bReferenceEstimated;

	status = aslArcSessionInit(
		pSession->pArcSession,
		pSession->pMemFunctions,
		pSession->baselinePos,
		pSession->bPositionAndScaleInvariant && !pSession->bReferenceEstimated ? 0 : pSession->distBaseToHelpLine,
		pSession->pSessionSettings->writingDirection);
	if (status !=decumaNoError) return status;

	aslSGInit(
		pSession->pSG,
		pSession->pMemFunctions,
		pSession->pSessionSettings->pStaticDB,
		pSession->pSessionSettings->pDynamicDB,
		pSession->pCharSet ? pSession->pCharSet : &pSession->pSessionSettings->charSet,
		pSession->pCatTable ? pSession->pCatTable : scrDatabaseGetCatTable(pSession->pSessionSettings->pStaticDB),
		pSession->pCatTable,
		pSession->pArcSession,
		pSession->pSessionSettings->recognitionMode,
		pSession->forcedSegmentation,
		pSession->nForcedSegmentationLen);

	/* Old results shall no longer be notable */
	if (pSession->pnBaseLineEstimates) aslFree(pSession->pnBaseLineEstimates);
	if (pSession->pnHelpLineEstimates) aslFree(pSession->pnHelpLineEstimates);
	pSession->nResults = 0;

	pSession->bAddingArcs = 1;

	return decumaNoError;
}


/*Writes an ID in pSamplingArcID which can be used for further reference to the arc */
DECUMA_STATUS aslStartNewArc(ASL_SESSION * pSession, DECUMA_UINT32 arcID)
{
	decumaAssert(getAslSessionStatus(pSession) == decumaNoError); /*Should have been checked already */

	return aslArcSessionStartNewArc(pSession->pArcSession,arcID);

}

DECUMA_STATUS aslStartNewSymbol(ASL_SESSION * pSession)
{
	decumaAssert(getAslSessionStatus(pSession) == decumaNoError); /*Should have been checked already */

	return aslArcSessionStartNewSymbol(pSession->pArcSession);
}

/*Adds point(s) to a started arc */
DECUMA_STATUS aslAddPoint(ASL_SESSION * pSession, DECUMA_COORD x, DECUMA_COORD y, DECUMA_UINT32 arcID)
{
	decumaAssert(getAslSessionStatus(pSession) == decumaNoError); /*Should have been checked already */

	return aslArcSessionAddPoint(pSession->pArcSession,x,y,arcID);
}

DECUMA_STATUS aslAddPoints(ASL_SESSION * pSession,
							  DECUMA_POINT * pPts, int nPts,
							  DECUMA_UINT32 arcID,
							  int * pnPtsAdded)
{
	decumaAssert(getAslSessionStatus(pSession) == decumaNoError); /*Should have been checked already */

	return aslArcSessionAddPoints(pSession->pArcSession,pPts,nPts,arcID,pnPtsAdded);
}


/*Ends the arc and removes it from the session */
DECUMA_STATUS aslCancelArc(ASL_SESSION * pSession,DECUMA_UINT32 arcID)
{
	decumaAssert(getAslSessionStatus(pSession) == decumaNoError); /*Should have been checked already */

	return aslArcSessionCancelArc(pSession->pArcSession,arcID);
}

/*Ends and adds the arc to the arc-addition-sequence which will be used in a future recognize call */
DECUMA_STATUS aslCommitArc(ASL_SESSION * pSession,DECUMA_UINT32 arcID)
{
	DECUMA_STATUS status;

	decumaAssert(getAslSessionStatus(pSession) == decumaNoError); /*Should have been checked already */

	if (!pSession->pSessionSettings->bMinimizeAddArcPreProcessing &&
		pSession->pSessionSettings->recognitionMode == scrMode &&
		aslArcSessionIsStartingNewSymbol(pSession->pArcSession))
	{
		/* In SCR mode it only makes sense to evaluate arcs that are preceeding start of */
		/* a new symbol or before constructing RG. Anything else would just be a waste of */
		/* time. Therefore, in this function, do this only just before committing an arc */
		/* that start a new symbol. */

		status = aslSGEvaluateArcs(pSession->pSG);

		if (status != decumaNoError) return status;
	}

	status = aslArcSessionCommitArc(pSession->pArcSession, arcID);

	if (status != decumaNoError) return status;

	/*No need to call evaluate arcs when we know that this arc is early part of multi-touch */
	if (!pSession->pSessionSettings->bMinimizeAddArcPreProcessing &&
		pSession->pSessionSettings->recognitionMode != scrMode &&
		aslArcSessionGetUncommittedArcCount(pSession->pArcSession)==0)
	{
		status = aslSGEvaluateArcs(pSession->pSG);
	}

	return status;
}

DECUMA_STATUS aslNoteSelectedCandidate( ASL_SESSION * pSession, int nResultIdx)
{
	decumaAssert(getAslSessionStatus(pSession) == decumaNoError); /*Should have been checked already */
	decumaAssert(nResultIdx >= -1);

	if (!pSession->nResults) return decumaIllegalFunctionCall; /* No results to note exist in current add arc session */

	if (nResultIdx >= pSession->nResults) return decumaInvalidIndex;

	if (pSession->pSessionSettings->supportLineSet != noSupportlines &&
		!pSession->bPositionAndScaleInvariant)
	{
		/* Estimates should not be used if baseline and helpline are set and in use */

		return decumaNoError;
	}

	pSession->nLastNotedResult = nResultIdx;

	if (nResultIdx == -1 ||
		pSession->pnBaseLineEstimates[pSession->nLastNotedResult] == pSession->pnHelpLineEstimates[pSession->nLastNotedResult])
	{
		/* Result rejection or selected candidate lacks estimate. Use old estimates. */
		pSession->baselinePos = pSession->nPrevBaselinePos;
		pSession->distBaseToHelpLine = pSession->nPrevDistBaseToHelpLine;
		pSession->bReferenceEstimated = pSession->bPrevReferenceEstimated;

		return decumaNoError;
	}

	/* Store current estimates (if existing) */
	pSession->baselinePos = pSession->pnBaseLineEstimates[pSession->nLastNotedResult];
	pSession->distBaseToHelpLine = pSession->pnBaseLineEstimates[pSession->nLastNotedResult] - pSession->pnHelpLineEstimates[pSession->nLastNotedResult];
	pSession->bReferenceEstimated = 1;

	return decumaNoError; /*Unimportant for asllib */
}


DECUMA_STATUS aslGetUncommittedArcCount(ASL_SESSION * pSession, int * pnUncommittedArcs)
{
	decumaAssert(getAslSessionStatus(pSession) == decumaNoError); /*Should have been checked already */
	decumaAssert(pnUncommittedArcs);

	*pnUncommittedArcs=aslArcSessionGetUncommittedArcCount(pSession->pArcSession);

	return decumaNoError;
}

DECUMA_STATUS aslGetUncommittedArcID(ASL_SESSION * pSession, int idx,
										DECUMA_UINT32 * pArcID)
{
	decumaAssert(getAslSessionStatus(pSession) == decumaNoError); /*Should have been checked already */

	return aslArcSessionGetUncommittedArcID(pSession->pArcSession, idx, pArcID);
}

#ifdef DECUMA_HWR_ENABLE_EXTRA_API
DECUMA_STATUS aslRecognizeFast( ASL_SESSION * pSession,
	DECUMA_HWR_RESULT * pResult,
	DECUMA_UINT16 * pnResults,  /* In most cases == 1, but can be 0 */
	DECUMA_UINT16 nMaxChars,
	const DECUMA_INTERRUPT_FUNCTIONS * pInterruptFunctions	)
{
	DECUMA_RECOGNITION_SETTINGS recSettings;

	decumaAssert(getAslSessionStatus(pSession) == decumaNoError); /*Should have been checked already */

	decumaMemset(&recSettings, 0, sizeof(recSettings));

	return recognize(pSession,pResult,1,pnResults, nMaxChars,&recSettings,1,NULL,NULL,pInterruptFunctions);


}
#endif /* DECUMA_HWR_ENABLE_EXTRA_API */

DECUMA_STATUS aslValidateRecognitionSettings(const DECUMA_RECOGNITION_SETTINGS * pRecognitionSettings)
{
	/*Asl specific validation of recognition settings. */
	/*General validation is performed in outer layer. */
	DECUMA_UNUSED_PARAM(pRecognitionSettings);
	return decumaNoError;
}

DECUMA_STATUS aslRecognize( ASL_SESSION * pSession,
	DECUMA_HWR_RESULT * pResults,
	DECUMA_UINT16 nMaxResults,
	DECUMA_UINT16 * pnResults,
	DECUMA_UINT16 nMaxCharsPerResult,
	const DECUMA_RECOGNITION_SETTINGS * pRecognitionSettings,
	const DECUMA_INTERRUPT_FUNCTIONS * pInterruptFunctions	)
{
	decumaAssert(getAslSessionStatus(pSession) == decumaNoError); /*Should have been checked already */

	/*	return aslRecognizeFast(pSession, pResults, pnResults, nMaxCharsPerResult); // Temp */
	return recognize(pSession,&pResults[0],nMaxResults,pnResults, nMaxCharsPerResult,pRecognitionSettings,0,NULL,NULL,pInterruptFunctions);
}

DECUMA_STATUS aslIndicateInstantGesture(
	ASL_SESSION * pSession,
	int * pbInstantGesture,
	const DECUMA_INSTANT_GESTURE_SETTINGS * pInstantGestureSettings)
{
	DECUMA_STATUS status;

	if (aslArcSessionGetUncommittedArcCount(pSession->pArcSession) > 0)
	{
		return decumaUncommittedArcExists;
	}

	decumaAssert(getAslSessionStatus(pSession) == decumaNoError); /*Should have been checked already */

	/*Get the arcs to be recognized */
	status = aslSGIndicateInstantGesture(pSession->pSG,pbInstantGesture,pInstantGestureSettings);

	return status;
}

#ifdef DECUMA_HWR_ENABLE_EXTRA_API
DECUMA_STATUS aslRecognizeForceRecognition( ASL_SESSION * pSession,
	const DECUMA_UNICODE * symbolString,
	DECUMA_HWR_RESULT * pResults,
	DECUMA_UINT16 nMaxResults,
	DECUMA_UINT16 * pnResults,
	DECUMA_UINT16 nMaxCharsPerResult,
	const DECUMA_RECOGNITION_SETTINGS * pRecognitionSettings,
	int bFast,
	const DECUMA_INTERRUPT_FUNCTIONS * pInterruptFunctions	)
{
	decumaAssert(getAslSessionStatus(pSession) == decumaNoError); /*Should have been checked already */

	return recognize(pSession,&pResults[0],nMaxResults,pnResults, nMaxCharsPerResult,pRecognitionSettings,bFast,NULL,symbolString,
		pInterruptFunctions);
}

DECUMA_STATUS aslRecognizeForceSegmentation( ASL_SESSION * pSession,
	const DECUMA_UNICODE * symbolString,
	DECUMA_HWR_RESULT * pResults,
	DECUMA_UINT16 nMaxResults,
	DECUMA_UINT16 * pnResults,
	DECUMA_UINT16 nMaxCharsPerResult,
	const DECUMA_RECOGNITION_SETTINGS * pRecognitionSettings,
	int bFast,
	const DECUMA_INTERRUPT_FUNCTIONS * pInterruptFunctions	)
{

	DECUMA_UNUSED_PARAM(symbolString);
	decumaAssert(getAslSessionStatus(pSession) == decumaNoError); /*Should have been checked already */

	return recognize(pSession,&pResults[0],nMaxResults,pnResults, nMaxCharsPerResult,pRecognitionSettings,bFast,NULL,NULL,
		pInterruptFunctions);
}
#endif /* DECUMA_HWR_ENABLE_EXTRA_API */

/*Only for the extra API decuma_hwr_extra.h (so far) */
#ifdef DECUMA_HWR_ENABLE_EXTRA_API
DECUMA_STATUS aslSetForcedSegmentation( ASL_SESSION * pSession,
	DECUMA_INT16 * pForceSymbolStrokes,
	DECUMA_INT16 nForceSymbols)
{
	const DECUMA_MEM_FUNCTIONS* pMemFunctions = pSession->pMemFunctions;
	decumaAssert(getAslSessionStatus(pSession) == decumaNoError); /*Should have been checked already */
	decumaAssert(sizeof(pForceSymbolStrokes[0]) == sizeof(pSession->forcedSegmentation[0]));

	if (pSession->forcedSegmentation) aslFree(pSession->forcedSegmentation);
	pSession->nForcedSegmentationLen = 0;

	if (nForceSymbols==0) return decumaNoError;

	pSession->forcedSegmentation = (DECUMA_INT16*)aslCalloc(nForceSymbols * sizeof(pForceSymbolStrokes[0]));

	if (!pSession->forcedSegmentation) return decumaAllocationFailed;

	decumaMemcpy(pSession->forcedSegmentation,pForceSymbolStrokes,nForceSymbols * sizeof(pForceSymbolStrokes[0]));
	pSession->nForcedSegmentationLen = nForceSymbols;

	return decumaNoError;
}

/*Only for the extra API decuma_hwr_extra.h (so far) */
const DECUMA_INT16 * aslGetForcedSegmentation(ASL_SESSION * pSession, DECUMA_INT16 * pnForceSymbols)
{
	decumaAssert(getAslSessionStatus(pSession) == decumaNoError); /*Should have been checked already */

	if (pnForceSymbols) *pnForceSymbols = pSession->nForcedSegmentationLen;

	return pSession->forcedSegmentation;
}


/*Only for the extra API decuma_hwr_extra.h (so far) */
DECUMA_STATUS aslSetDictionaryFilterString(ASL_SESSION * pSession, const DECUMA_UNICODE * symbolString)
{
	DECUMA_UNUSED_PARAM(symbolString);
	decumaAssert(getAslSessionStatus(pSession) == decumaNoError); /*Should have been checked already */

	return decumaFunctionNotSupported;
}

/*Only for the extra API decuma_hwr_extra.h (so far) */
DECUMA_UNICODE * aslGetDictionaryFilterString(ASL_SESSION * pSession)
{
	decumaAssert(getAslSessionStatus(pSession) == decumaNoError); /*Should have been checked already */
	return NULL;
}

/*Only for the extra API decuma_hwr_extra.h (so far) */
DECUMA_STATUS aslConfirmArcs( ASL_SESSION * pSession)
{
	decumaAssert(getAslSessionStatus(pSession) == decumaNoError); /*Should have been checked already */

	return decumaFunctionNotSupported;
}

/*Only for the extra API decuma_hwr_extra.h (so far) */
DECUMA_STATUS aslEvaluateArcs( ASL_SESSION * pSession)
{
	decumaAssert(getAslSessionStatus(pSession) == decumaNoError); /*Should have been checked already */

	/* In SCR mode we never evaluate arcs before decumaRecognize is called since this will */
	/* just be a waste of time. */
	if (pSession->pSessionSettings->recognitionMode == scrMode) return decumaNoError;

	return aslSGEvaluateArcs(pSession->pSG);
}

/*Only for the extra API decuma_hwr_extra.h (so far) */
DECUMA_STATUS aslRecognizeExtraOutput( ASL_SESSION * pSession,
	DECUMA_HWR_RESULT * pResults,
	DECUMA_UINT16 nMaxResults,
	DECUMA_UINT16 * pnResults,
	DECUMA_UINT16 nMaxCharsPerResult,
	void *  pCharArcIdxData, /*Should point to an array of size nMaxResults * nMaxCharsPerResult * */
	const DECUMA_RECOGNITION_SETTINGS * pRecognitionSettings,
	const DECUMA_INTERRUPT_FUNCTIONS * pInterruptFunctions	)
{
	decumaAssert(getAslSessionStatus(pSession) == decumaNoError); /*Should have been checked already */

	return recognize(pSession, pResults, nMaxResults, pnResults, nMaxCharsPerResult, pRecognitionSettings, 0, pCharArcIdxData,NULL,
		pInterruptFunctions);
}

/*Only for the extra API decuma_hwr_extra.h (so far) */
int aslGetCharArcIdxDataSize( DECUMA_UINT16 nMaxResults, DECUMA_UINT16 nMaxCharsPerResult)
{
	DECUMA_UNUSED_PARAM(nMaxResults);
	DECUMA_UNUSED_PARAM(nMaxCharsPerResult);
	return 0;
}

/*Only for the extra API decuma_hwr_extra.h (so far) */
const DECUMA_UINT16 * aslGetCharArcIdxs( const void * pCharArcIdxData,
														 DECUMA_UINT16 nResultIdx, DECUMA_UINT16 nMaxResults,
														 DECUMA_UINT16 nCharIdx, DECUMA_UINT16 nMaxCharsPerResult,
														 DECUMA_UINT16 * pnArcs)
{
	DECUMA_UNUSED_PARAM(pCharArcIdxData);
	DECUMA_UNUSED_PARAM(nResultIdx);
	DECUMA_UNUSED_PARAM(nMaxResults);
	DECUMA_UNUSED_PARAM(nCharIdx);
	DECUMA_UNUSED_PARAM(nMaxCharsPerResult);
	DECUMA_UNUSED_PARAM(pnArcs);
	return NULL;
}

DECUMA_STATUS aslGetDistance( ASL_SESSION * pSession,
	const DECUMA_UNICODE * symbolString,
	const DECUMA_RECOGNITION_SETTINGS * pRecognitionSettings,
	DECUMA_UINT32 * pDistance,
	DECUMA_STRING_TYPE * pStringType)
{
	DECUMA_STATUS status = decumaNoError;
	DECUMA_HWR_RESULT result;
	DECUMA_UINT16 nResults;
	DECUMA_UNICODE chars[50];
	DECUMA_INT16 symbolChars[50];
	DECUMA_INT16 symbolStrokes[50];

	decumaAssert(getAslSessionStatus(pSession) == decumaNoError); /*Should have been checked already */

	decumaMemset(&result,0,sizeof(result));
	decumaMemset(chars,0,sizeof(chars));
	decumaMemset(symbolChars,0,sizeof(symbolChars));
	decumaMemset(symbolStrokes,0,sizeof(symbolStrokes));
	result.pChars = chars;
	result.pSymbolChars = symbolChars;
	result.pSymbolStrokes = symbolStrokes;

	status = aslRecognizeForceRecognition(pSession,symbolString,&result,1,&nResults, sizeof(chars)/sizeof(chars[0]),pRecognitionSettings,0,
		NULL	);

	if (status == decumaNoError)
	{
		if (pDistance) *pDistance = result.distance;
		if (pStringType) *pStringType = result.stringType;
	}

	return status;
}
#endif /* DECUMA_HWR_ENABLE_EXTRA_API */

DECUMA_STATUS aslEndArcAddition(ASL_SESSION * pSession)
{
	decumaAssert(getAslSessionStatus(pSession) == decumaNoError); /*Should have been checked already */
	decumaAssert(pSession->bAddingArcs); /*Should already have been checked by outer layer */

	/* NOTE: RG destructor depends on SG. MUST NOT destroy SG before RG!! /JA */
#ifdef EXTRA_VERIFICATION_INFO
	if (pSession->bDelayedRGDestruction)
	{
		aslRGDestroy(pSession->pRG);
		pSession->bDelayedRGDestruction=0;
	}
#endif
	aslSGDestroy(pSession->pSG);

	aslArcSessionDestroy(pSession->pArcSession);

	pSession->bAddingArcs = 0;

	return decumaNoError;
}

/******* Dictionary conversion functions *******/

DECUMA_STATUS aslUnpackXT9Dictionary(void ** ppDictionaryData,
	const void * pXT9Dictionary, DECUMA_UINT32 xt9DataSize, DECUMA_XT9_DICTIONARY_TYPE type,
	DECUMA_UINT32 * pUnpackedDataSize,const DECUMA_MEM_FUNCTIONS * pMemFunctions)
{
	DECUMA_STATUS status;
	DECUMA_TRIE * pTrie = NULL;
	DECUMA_TERSE_TRIE * pTerseTrie = NULL;

	decumaAssert(ppDictionaryData );
	decumaAssert(pXT9Dictionary );
	decumaAssert(pUnpackedDataSize);
	decumaAssert(type<numberOfXT9DictionaryTypes); /* type >= 0 */

	*ppDictionaryData = NULL;

	status = decumaXT9UnpackToTrie(&pTrie, pXT9Dictionary, xt9DataSize, type,pMemFunctions);
	if (status != decumaNoError ) goto decumaUnpackXT9Dictionary_error;
	decumaAssert ( pTrie );

#ifdef USE_TERSE_TRIE
	status = decumaTerseTrieCreateFromTrie(&pTerseTrie,pTrie,pMemFunctions);
	if (status != decumaNoError ) goto decumaUnpackXT9Dictionary_error;
	decumaAssert ( pTerseTrie );

	decumaTrieDestroy(&pTrie,pMemFunctions);
	*ppDictionaryData = pTerseTrie;
	*pUnpackedDataSize = decumaTerseTrieGetSize(pTerseTrie);
#else
	*ppDictionaryData = pTrie;
	*pUnpackedDataSize = decumaTrieGetSize(pTrie);
#endif

	return decumaNoError;

decumaUnpackXT9Dictionary_error:
	if (pTrie) decumaTrieDestroy(&pTrie,pMemFunctions);
	if (pTerseTrie) decumaTerseTrieDestroy(&pTerseTrie,pMemFunctions);

	return status;
}

DECUMA_STATUS aslDestroyUnpackedDictionary(void ** ppDictionaryData,
	const DECUMA_MEM_FUNCTIONS * pMemFunctions)
{
	DECUMA_STATUS status;

	if (!(*ppDictionaryData)) return decumaNoError; /*It is considered ok to try to destroy a null pointer */

	status = decumaDictionaryDestroyBinaryData(ppDictionaryData, pMemFunctions);
	if (status != decumaNoError)
		return status;

	decumaAssert(*ppDictionaryData==0);

	return status;
}


/******* Dictionary attachment functions *******/

DECUMA_STATUS aslAttachStaticDictionary(ASL_SESSION * pSession,
										const void * pDictionaryData)
{
	DECUMA_STATUS status=decumaNoError;

	decumaAssert(getAslSessionStatus(pSession) == decumaNoError); /*Should have been checked already */
	decumaAssert(pDictionaryData);
	status = decumaDictionaryValidateBinaryData(pDictionaryData);
	if (status !=decumaNoError)
		return status;

	/*Attach the dictionary: */
	status = attachDictionary(pSession,
		pDictionaryData);

	if (status !=decumaNoError)
		return status;

	return decumaNoError;
}


DECUMA_STATUS aslDetachStaticDictionary(ASL_SESSION * pSession,
										const void * pDictionaryData)
{
	int i;
	DECUMA_HWR_DICTIONARY * pDictionary;
	const DECUMA_MEM_FUNCTIONS* pMemFunctions = pSession->pMemFunctions;
	DECUMA_STATUS status;

	/* Input validation */
	decumaAssert(getAslSessionStatus(pSession) == decumaNoError); /*Should have been checked already */
	decumaAssert(pDictionaryData); /*Should have been checked already */

	status = decumaDictionaryValidateBinaryData(pDictionaryData);
	if (status != decumaNoError)
		return status;

	for ( i = 0; i < pSession->nDictionaries ; i++)
	{
		if (pDictionaryData == decumaDictionaryGetDataPtr(pSession->pDictionaries[i])) break;
	}

	if ( i == pSession->nDictionaries )
		return decumaNeverAttached;

	pDictionary = pSession->pDictionaries[i];

	/* Don't call decumaDictionaryDestroyData it should be called via */
	/* aslDestroyUnpackedDictionary at a later point, if needed */
	aslFree(pDictionary);

	pSession->nDictionaries--;

	decumaMemmove(pSession->pDictionaries + i, pSession->pDictionaries + i + 1, (pSession->nDictionaries - i) * sizeof(pSession->pDictionaries[i]));

	return decumaNoError;
}

/******* Dynamic dictionary functions *******/

DECUMA_STATUS aslAttachDynamicDictionary(ASL_SESSION * pSession,
														   DECUMA_DICTIONARY_TYPE type,
														   void * pDictionaryData,
														   int cbDictionaryDataSize)
{
	DECUMA_UNUSED_PARAM(pSession);
	DECUMA_UNUSED_PARAM(type);
	DECUMA_UNUSED_PARAM(pDictionaryData);
	DECUMA_UNUSED_PARAM(cbDictionaryDataSize);
	return decumaFunctionNotSupported;
}

DECUMA_STATUS aslDetachDynamicDictionary(ASL_SESSION * pSession,
														   const void * pDictionaryData)
{
	DECUMA_UNUSED_PARAM(pSession);
	DECUMA_UNUSED_PARAM(pDictionaryData);
	return decumaFunctionNotSupported;
}


DECUMA_STATUS aslAddWordToDictionary(ASL_SESSION * pSession, void * pDictionaryBuffer, DECUMA_UNICODE * pWord)
{
	DECUMA_UNUSED_PARAM(pSession);
	DECUMA_UNUSED_PARAM(pDictionaryBuffer);
	DECUMA_UNUSED_PARAM(pWord);
	return decumaFunctionNotSupported;
}


DECUMA_STATUS aslGetDynamicDictionarySize(ASL_SESSION * pSession, const void * pDictionaryBuffer, int * pSize)
{
	DECUMA_UNUSED_PARAM(pSession);
	DECUMA_UNUSED_PARAM(pDictionaryBuffer);
	DECUMA_UNUSED_PARAM(pSize);
	return decumaFunctionNotSupported;
}


DECUMA_STATUS aslWordExistsInDictionary(ASL_SESSION * pSession, const void * pDictionaryBuffer, DECUMA_UNICODE * pWord)
{
	DECUMA_UNUSED_PARAM(pSession);
	DECUMA_UNUSED_PARAM(pDictionaryBuffer);
	DECUMA_UNUSED_PARAM(pWord);
	return decumaFunctionNotSupported;
}

DECUMA_STATUS aslAddAllWords(ASL_SESSION * pSession,
											   void * pDictionaryBuffer,
											   const DECUMA_UNICODE * pWordBuffer,
											   int nUnicodesInBuffer,
											   int bNullDelimitedOnly)
{
	DECUMA_UNUSED_PARAM(pSession);
	DECUMA_UNUSED_PARAM(pDictionaryBuffer);
	DECUMA_UNUSED_PARAM(pWordBuffer);
	DECUMA_UNUSED_PARAM(nUnicodesInBuffer);
	DECUMA_UNUSED_PARAM(bNullDelimitedOnly);
	return decumaFunctionNotSupported;
}


/******* Word completion functions *******/

#ifdef DECUMA_HWR_ENABLE_EXTRA_API
DECUMA_STATUS aslCompleteWord(ASL_SESSION * pSession,
												DECUMA_COMPLETION_RESULT * pResults,
												DECUMA_UINT32 nMaxResults,
												DECUMA_UINT32 * pnResults,
												DECUMA_UINT32 nMaxCharsPerResult,
												DECUMA_UNICODE * pChars,
												DECUMA_UINT16 nChars)
{
	DECUMA_UNUSED_PARAM(pSession);
	DECUMA_UNUSED_PARAM(pResults);
	DECUMA_UNUSED_PARAM(nMaxResults);
	DECUMA_UNUSED_PARAM(pnResults);
	DECUMA_UNUSED_PARAM(nMaxCharsPerResult);
	DECUMA_UNUSED_PARAM(pChars);
	DECUMA_UNUSED_PARAM(nChars);
	return decumaFunctionNotSupported;
}


/*///////////////////////////////////////////////////////////////////////////////////////// */

DECUMA_STATUS aslUsePositionAndScaleInvariantRecognition( ASL_SESSION * pSession, int bOnOff)
{
	decumaAssert(getAslSessionStatus(pSession) == decumaNoError); /*Should have been checked already */

	pSession->bPositionAndScaleInvariant = bOnOff;

	return decumaNoError;
}

DECUMA_STATUS aslUseTypeConnectionRequirement( ASL_SESSION * pSession, int bOnOff)
{
	DECUMA_UNUSED_PARAM(bOnOff);
	decumaAssert(getAslSessionStatus(pSession) == decumaNoError); /*Should have been checked already */
	return decumaNoError; /*Unimportant for asllib */
}

DECUMA_STATUS aslUseStrictTypeConnectionRequirement( ASL_SESSION * pSession, int bOnOff)
{
	DECUMA_UNUSED_PARAM(bOnOff);
	decumaAssert(getAslSessionStatus(pSession) == decumaNoError); /*Should have been checked already */
	return decumaNoError; /*Unimportant for asllib */
}

int aslGetArcSessionSegmentCount(ASL_SESSION * pSession)
{
	decumaAssert(getAslSessionStatus(pSession) == decumaNoError); /*Should have been checked already */

	return aslArcSessionGetArcCount(pSession->pArcSession);
}


DECUMA_UINT32 aslGetMaxNumberOfSegments(void)
{
	return MAX_NODES_IN_SEGMENTATION_GRAPH-1;
}

DECUMA_UINT32 aslGetArcSessionSize(void)
{
	return aslArcSessionGetSize();
}

DECUMA_UINT32 aslGetSGSize(void)
{
	return aslSGGetSize();
}

DECUMA_UINT32 aslGetRGSize(void)
{
	return aslRGGetSize();
}

#endif /* DECUMA_HWR_ENABLE_EXTRA_API */

/*///////////////////////////////////////////////////////////////////////////////////////// */
/* */
/* Private function implementation */
/* */

static DECUMA_STATUS allSymbolsInDatabase(DECUMA_UNICODE * pCharSetExt,
							DECUMA_STATIC_DB_PTR pStaticDB,
							int * pbIncluded)
{
	DECUMA_UNICODE * pSymbolStart = pCharSetExt;
	DECUMA_STATUS status;
	int bIncluded;

	decumaAssert(pCharSetExt);
	decumaAssert(pStaticDB);


	*pbIncluded=0;

	while (pCharSetExt[0] || pCharSetExt[1])
	{
		if (!pCharSetExt[0])
		{
			/*We have found a cmoplete word, test it */
			status = aslDatabaseIncludesSymbol(pStaticDB,NULL,pSymbolStart,&bIncluded);
			if (status!=decumaNoError) return status;

			if (!bIncluded)
			{
				/*Found at least one missing */
				return decumaNoError;
			}

			pSymbolStart=&pCharSetExt[1]; /*The symbol after the found zero terminator */
		}
		pCharSetExt++;
	}

	/*Test the last word */
	status = aslDatabaseIncludesSymbol(pStaticDB,NULL,pSymbolStart,&bIncluded);
	if (status!=decumaNoError) return status;

	*pbIncluded=bIncluded;

	return decumaNoError;

}



static void adjustResultForForbiddenBigrams(
	ASL_ARC_SESSION * pArcSession,
	DECUMA_UNICODE * pChars,
	DECUMA_UINT16 * pnChars,
	DECUMA_UINT16 * pnResultingChars,
	DECUMA_INT16 * pSymbolChars,
	DECUMA_INT16 * pnSymbolCharsLen,
	DECUMA_INT16 * pnResultingSymbolCharsLen,
	DECUMA_INT16 * pSymbolStrokes,
	DECUMA_INT16 * pnSymbolStrokesLen,
	DECUMA_INT16 * pnResultingSymbolStrokesLen,
	DECUMA_INT16 * pDictStrStart,
	DECUMA_INT16 * pDictStrLen
	)
{
	int i;
	int j, nSymbolToReplaceInd;

	/* Adjust for forbidden bigrams. */
	/* Two ' should be one ". */
	/* Note that this string conversion might introduce a duplicate, which needs to be handled. */
	/* Note that in ontop mode this allows the individual arcs of " to be written ontop of eachother. */
	/* */
	/* TODO replace this hack with a general forbidden bigram handling. */
	decumaAssert(pDictStrStart);
	decumaAssert(pDictStrLen);

	decumaAssert(pArcSession);
	decumaAssert(pSymbolStrokes);
	decumaAssert(pSymbolChars);

	for (i = 0; i < *pnChars-1; i++)
	{
		if (pSymbolStrokes[0] == 0 && i < pSymbolChars[0]) continue; /* We should not replace string start chars */

		if (pChars[i] == '\'' && pChars[i+1] == '\'')
		{
			int nArcs = 0;

			if (*pDictStrLen && i >= *pDictStrStart && i+1 <= *pDictStrStart + *pDictStrLen - 1) continue; /* Don't remove bigrams inside dict path */

			/*Find nSymbolToReplaceInd */
			for (j = 0, nSymbolToReplaceInd = 0; j <= i + 1; nSymbolToReplaceInd++)
			{
				j += pSymbolChars[nSymbolToReplaceInd];
				nArcs += pSymbolStrokes[nSymbolToReplaceInd];
			}

			nSymbolToReplaceInd--;

			decumaAssert(nSymbolToReplaceInd > 0);
			decumaAssert(nSymbolToReplaceInd < *pnSymbolCharsLen);
			decumaAssert(nSymbolToReplaceInd < *pnSymbolStrokesLen);

			nArcs -= pSymbolStrokes[nSymbolToReplaceInd];

			if (aslArcIsStartingNewSymbol(aslArcSessionGetArc(pArcSession, nArcs))) continue; /* Cannot replace a symbol that starts a new arc */

			decumaAssert(*pDictStrLen == 0 || *pDictStrStart != i);
			decumaAssert(*pDictStrLen == 0 || *pDictStrStart != i+1);
			decumaAssert(*pDictStrLen == 0 || *pDictStrStart + *pDictStrLen - 1 != i);
			decumaAssert(*pDictStrLen == 0 || *pDictStrStart + *pDictStrLen - 1 != i+1);

			if (pChars[i] == '\'') pChars[i] = '"';
			if (i < *pnChars-2) decumaMemmove(&pChars[i+1], &pChars[i+2], (*pnChars-i-2)*sizeof(pChars[0])); /* Must use memmove since src and dest overlap! */
			(*pnChars)--;
			(*pnResultingChars)--;
			pChars[*pnChars] = 0;
			if (*pDictStrStart>i)
			{
				*pDictStrStart = *pDictStrStart-1;
			} else {
				if (*pDictStrStart+*pDictStrLen <i && *pDictStrLen>1) *pDictStrLen = *pDictStrLen-1;
			}
			decumaAssert(*pDictStrStart + *pDictStrLen <= *pnResultingChars);

			if (nSymbolToReplaceInd < *pnSymbolCharsLen-1) decumaMemmove(&pSymbolChars[nSymbolToReplaceInd], &pSymbolChars[nSymbolToReplaceInd+1], (*pnSymbolCharsLen-nSymbolToReplaceInd-1)*sizeof(pSymbolChars[0]));  /* Must use memmove since src and dest overlap! */
			(*pnSymbolCharsLen)--;
			(*pnResultingSymbolCharsLen)--;
			pSymbolChars[*pnSymbolCharsLen] = 0;

			pSymbolStrokes[nSymbolToReplaceInd-1] += pSymbolStrokes[nSymbolToReplaceInd];
			if (nSymbolToReplaceInd < *pnSymbolStrokesLen-1) decumaMemmove(&pSymbolStrokes[nSymbolToReplaceInd], &pSymbolStrokes[nSymbolToReplaceInd+1], (*pnSymbolStrokesLen-nSymbolToReplaceInd-1)*sizeof(pSymbolStrokes[0])); /* Must use memmove since src and dest overlap! */
			(*pnSymbolStrokesLen)--;
			(*pnResultingSymbolStrokesLen)--;
			pSymbolStrokes[*pnSymbolStrokesLen] = 0;
		}
	}
}

static int adjustResultForDuplicates(DECUMA_HWR_RESULT * pResults, int nResults)
{
	/* Make sure there are no duplicate strings (the forbidden bigram handling might cause this) */

	int i, j;

	for (i = 0; i < nResults; i++)
	{
		for (j = i+1; j < nResults; j++)
		{
			if (pResults[i].nChars == pResults[j].nChars &&
				decumaMemcmp(pResults[i].pChars, pResults[j].pChars, (pResults[i].nChars) * sizeof(pResults[i].pChars[0])) == 0)
			{
				/* j is a duplicate of better candidate i, remove j. */

				int k;

				for (k = j; k < nResults - 1; k++)
				{
					DECUMA_HWR_RESULT origRes = pResults[k];

					pResults[k] = pResults[k+1];

					/* Important do not copy the whole result struct since we should not modifiy the callers pointers */
					/* Instead the pointer data should be copied */
					pResults[k].pChars = origRes.pChars;
					pResults[k].pSymbolChars = origRes.pSymbolChars;
					pResults[k].pSymbolStrokes =  origRes.pSymbolStrokes;
					decumaMemmove(pResults[k].pChars, pResults[k+1].pChars, (pResults[k+1].nChars+1)*sizeof(pResults[k].pChars[0])); /* Must use memmove since src and dest overlap! */
					if (pResults[k].pSymbolChars) decumaMemmove(pResults[k].pSymbolChars, pResults[k+1].pSymbolChars, pResults[k+1].nSymbols*sizeof(pResults[k].pSymbolChars[0])); /* Must use memmove since src and dest overlap! */
					if (pResults[k].pSymbolStrokes) decumaMemmove(pResults[k].pSymbolStrokes, pResults[k+1].pSymbolStrokes, pResults[k+1].nSymbols*sizeof(pResults[k].pSymbolStrokes[0])); /* Must use memmove since src and dest overlap! */
				}
				nResults--;
				j--;
			}
		}
	}

	return nResults;
}

static DECUMA_STATUS recognize( ASL_SESSION * pSession,
	DECUMA_HWR_RESULT * pResults,
	DECUMA_UINT16 nMaxResults,
	DECUMA_UINT16 * pnResults,
	DECUMA_UINT16 nMaxCharsPerResult,
	const DECUMA_RECOGNITION_SETTINGS * pRecognitionSettings,
	int bOnlyFromSG,
	void * pCharArcIdxData,
	const DECUMA_UNICODE * forceString,
	const DECUMA_INTERRUPT_FUNCTIONS * pInterruptFunctions	)
{
	const DECUMA_MEM_FUNCTIONS* pMemFunctions = pSession->pMemFunctions;
	DECUMA_STATUS status;
	DECUMA_COORD nBaseLineEstimate, nHelpLineEstimate;
	int nSGIterations;
	int nRGIterations;
	int nResults = 0;
	int bIterateRG = 0;
	int bSkipRG = 0;
	int i;

	if (pInterruptFunctions != NULL) {
		return decumaAbortRecognitionUnsupported;
	}

	if (aslArcSessionGetUncommittedArcCount(pSession->pArcSession) > 0)
	{
		return decumaUncommittedArcExists;
	}

	decumaAssert(getAslSessionStatus(pSession) == decumaNoError); /*Should have been checked already */
	decumaAssert(aslValidateRecognitionSettings(pRecognitionSettings) == decumaNoError); /*Should have been checked already */

	if (!bOnlyFromSG &&
		pRecognitionSettings->boostLevel != noBoost &&
		!pSession->nDictionaries)
	{
		return decumaNoDictionary;
	}

	/* Init output */
	*pnResults = 0;

	if (nMaxResults < 1) {
		return decumaNoError;
	}

	if (aslArcSessionGetArcCount(pSession->pArcSession)==0)
	{
		/*Don't do anything if we have not added any arcs */
		/*Exit quickly and  don't update pSession->nResults */
		return decumaNoError;

	}

	status = aslSGEvaluateArcs(pSession->pSG);
	if ( status != decumaNoError)
	{
		return status;
	}


	if (!bOnlyFromSG &&
		pSession->pSessionSettings->recognitionMode == scrMode &&
		pSession->nDictionaries == 0 &&
		(pRecognitionSettings->pStringStart == NULL || pRecognitionSettings->pStringStart[0] == 0))
	{
		/* Check if SG construction is sufficient, i.e. if first arc is the only one that can start new symbols */

		int nArcs = aslArcSessionGetArcCount(pSession->pArcSession);

		for (i = 1; i < nArcs; i++)
		{
			if (aslArcIsStartingNewSymbol(aslArcSessionGetArc(pSession->pArcSession, i))) break;
		}

		bSkipRG = i == nArcs;
	}

	if (!bSkipRG &&
		(pSession->pSessionSettings->supportLineSet == noSupportlines ||
		 pSession->bPositionAndScaleInvariant))
	{
		/* Iterate reference lines estimation (especially important if no initial estimate is available) */
		nSGIterations = 0;

		do
		{
			aslSGGetBestString(pSession->pSG, NULL, 0, NULL, NULL, 0, NULL, 0, NULL, NULL, NULL, 0, NULL, NULL, NULL, 0, NULL, NULL, &nBaseLineEstimate, &nHelpLineEstimate, NULL, NULL); /* Need some scale reference for horisontal connection distance */

			if (nBaseLineEstimate == aslArcSessionGetBaseline(pSession->pArcSession) &&
				nHelpLineEstimate == aslArcSessionGetBaseline(pSession->pArcSession) - aslArcSessionGetDistBaseToHelpline(pSession->pArcSession))
				break; /* New estimates identical to old estimates. No point in continuing estimation iteration */

			aslSGAdjustForNewEstimates(pSession->pSG, nBaseLineEstimate, nHelpLineEstimate);

			aslArcSessionSetBaseline(pSession->pArcSession, nBaseLineEstimate);
			aslArcSessionSetDistBaseToHelpline(pSession->pArcSession, nBaseLineEstimate - nHelpLineEstimate);

			nSGIterations++;
		} while (nSGIterations < 10);
	}

	if (bOnlyFromSG || bSkipRG)
	{
		const ASL_SG_NODE* pNode = aslSGGetNode(pSession->pSG, aslSGGetNodeCount(pSession->pSG)-1);;

		nResults = aslSGGetEdgeCount(pNode);
		if ( nResults > nMaxResults )
			nResults = nMaxResults;

		if (nResults > 1 && !bSkipRG) nResults = 1; /* Fast recognition. Should only result in one candidate, since rest is nonsense. */

		if (pSession->pSessionSettings->supportLineSet == noSupportlines ||
			pSession->bPositionAndScaleInvariant)
		{
			if (pSession->pnBaseLineEstimates) aslFree(pSession->pnBaseLineEstimates);
			if (pSession->pnHelpLineEstimates) aslFree(pSession->pnHelpLineEstimates);

			pSession->pnBaseLineEstimates = aslCalloc(nResults * sizeof(pSession->pnBaseLineEstimates[0]));

			if (!pSession->pnBaseLineEstimates) return decumaAllocationFailed;

			pSession->pnHelpLineEstimates = aslCalloc(nResults * sizeof(pSession->pnHelpLineEstimates[0]));

			if (!pSession->pnHelpLineEstimates)	return decumaAllocationFailed;
		}

		pSession->nResults = nResults;
		pSession->nLastNotedResult = 0;

		for ( i = 0; i < nResults; i++)
		{
			DECUMA_HWR_RESULT * pResult = &pResults[i];
			DECUMA_INT16 nSymbolCharsLen, nResultingSymbolCharsLen;
			DECUMA_INT16 nSymbolMaskLen, nResultingSymbolMaskLen;

			aslSGGetBestString(pSession->pSG, &pResult->pChars[0],
				nMaxCharsPerResult, &pResult->nChars,
				&pResult->nResultingChars,
				i,
				pResult->pSymbolChars,nMaxCharsPerResult,&nSymbolCharsLen,&nResultingSymbolCharsLen,
				pResult->pSymbolStrokes,nMaxCharsPerResult,&pResult->nSymbols,&pResult->nResultingSymbols,
				pResult->pSymbolArcTimelineDiffMask,nMaxCharsPerResult,&nSymbolMaskLen,&nResultingSymbolMaskLen,
				pSession->pnBaseLineEstimates ? &pSession->pnBaseLineEstimates[i] : NULL,
				pSession->pnHelpLineEstimates ? &pSession->pnHelpLineEstimates[i] : NULL,
				&pResult->bGesture, &pResult->bInstantGesture);

			decumaAssert(pResult->nSymbols == nSymbolCharsLen);
			decumaAssert(pResult->nResultingSymbols == nResultingSymbolCharsLen);

			pResult->distance = getMeanDistance(pSession, aslSGGetBestPathDistance(aslSGGetEdge(pNode, i)),
					aslSGGetNodeCount(pSession->pSG));

			pResult->shapeDistance = pResult->distance;

			status = getStringType(pSession,pResult->pChars,pResult->nChars, &pResult->stringType,
				&pResult->dictStrStart, &pResult->dictStrLen);

			if (status != decumaNoError) return status;
		}

		if (pSession->pSessionSettings->supportLineSet == noSupportlines ||
			pSession->bPositionAndScaleInvariant)
		{
			/* Store current estimates (if existing) */
			if (pSession->pnBaseLineEstimates[pSession->nLastNotedResult] != pSession->pnHelpLineEstimates[pSession->nLastNotedResult])
			{
				pSession->baselinePos = pSession->pnBaseLineEstimates[pSession->nLastNotedResult];
				pSession->distBaseToHelpLine = pSession->pnBaseLineEstimates[pSession->nLastNotedResult] - pSession->pnHelpLineEstimates[pSession->nLastNotedResult];
				pSession->bReferenceEstimated = 1;
			}
		}
	}
	else
	{
#ifdef EXTRA_VERIFICATION_INFO
		if (pSession->bDelayedRGDestruction)
		{
			aslRGDestroy(pSession->pRG);
			/*if (!pSession->pForcedRG) removeEZiTextData(pSession); */
			pSession->bDelayedRGDestruction=0;
		}
#endif

		decumaAssert( (pSession->nDictionaries > 0) ^ (pSession->pDictionaries == 0) );

		nBaseLineEstimate = aslArcSessionGetBaseline(pSession->pArcSession);
		nHelpLineEstimate = aslArcSessionGetBaseline(pSession->pArcSession) - aslArcSessionGetDistBaseToHelpline(pSession->pArcSession);
		nRGIterations = 0;

#ifdef ITERATE_RG
		/* RG iteration allowed. Iterate to refine reference estimates if none have been supplied or we run in invariant mode. */
		bIterateRG = pSession->pSessionSettings->supportLineSet == noSupportlines || pSession->bPositionAndScaleInvariant;
#endif

		do
		{
			if (nRGIterations > 0)
			{
				aslRGDestroy(pSession->pRG);
			}

			aslRGInit(pSession->pRG, pSession->pMemFunctions,
				pSession->pCharSet ? pSession->pCharSet : &pSession->pSessionSettings->charSet,
				pSession->pCatTable ? pSession->pCatTable : scrDatabaseGetCatTable(pSession->pSessionSettings->pStaticDB),
				pSession->pSG, pSession->pArcSession,
				pSession->pDictionaries,
				pSession->nDictionaries,
				pRecognitionSettings->boostLevel,
				pRecognitionSettings->stringCompleteness,
				pRecognitionSettings->pStringStart,
				pSession->pSessionSettings->writingDirection,
				pSession->pSessionSettings->recognitionMode,
				pSession->pDictionaryFilterStr,
				pSession->nDictionaryFilterStrLen,
				forceString,
				NULL,
				0);

			status = aslRGConstruct(pSession->pRG, nRGIterations > 0 ||
				!bIterateRG);

			if (status != decumaNoError)
			{
				aslRGDestroy(pSession->pRG);
				return status;
			}

			nResults = aslRGGetNOFStrings(pSession->pRG);
			if ( nResults > nMaxResults )
				nResults = nMaxResults;

			if (!bIterateRG) break; /* Don't estimate reference lines */

			if (nResults > 0)
			{
				/* Get new baseline and helpline estimate from best RG string */
				DECUMA_HWR_RESULT * pResult = &pResults[0];
				DECUMA_INT16 nSymbolCharsLen, nResultingSymbolCharsLen;
				DECUMA_INT16 nSymbolMaskLen, nResultingSymbolMaskLen;

				aslRGGetString(pSession->pRG,pResult->pChars,nMaxCharsPerResult,
					&pResult->nChars,&pResult->nResultingChars, 0,
					pResult->pSymbolChars,nMaxCharsPerResult,&nSymbolCharsLen,&nResultingSymbolCharsLen,
					pResult->pSymbolStrokes,nMaxCharsPerResult,&pResult->nSymbols,&pResult->nResultingSymbols,
					pResult->pSymbolArcTimelineDiffMask,nMaxCharsPerResult,&nSymbolMaskLen,&nResultingSymbolMaskLen,
					&nBaseLineEstimate, &nHelpLineEstimate, &pResult->bGesture, &pResult->bInstantGesture);
			}

			aslSGAdjustForNewEstimates(pSession->pSG, nBaseLineEstimate, nHelpLineEstimate);

			if (nRGIterations > 0 &&
				nBaseLineEstimate == aslArcSessionGetBaseline(pSession->pArcSession) &&
				nHelpLineEstimate == aslArcSessionGetBaseline(pSession->pArcSession) - aslArcSessionGetDistBaseToHelpline(pSession->pArcSession))
				break; /* New estimates identical to old estimates. No point in continuing estimation iteration */

			aslArcSessionSetBaseline(pSession->pArcSession, nBaseLineEstimate);
			aslArcSessionSetDistBaseToHelpline(pSession->pArcSession, nBaseLineEstimate - nHelpLineEstimate);

			nRGIterations++;

		} while (nRGIterations < 10);

		if ((pSession->pSessionSettings->supportLineSet == noSupportlines ||
			 pSession->bPositionAndScaleInvariant) &&
			nResults > 0)
		{
			if (pSession->pnBaseLineEstimates) aslFree(pSession->pnBaseLineEstimates);
			if (pSession->pnHelpLineEstimates) aslFree(pSession->pnHelpLineEstimates);

			pSession->pnBaseLineEstimates = aslCalloc(nResults * sizeof(pSession->pnBaseLineEstimates[0]));

			if (!pSession->pnBaseLineEstimates)
			{
				aslRGDestroy(pSession->pRG);
				return decumaAllocationFailed;
			}

			pSession->pnHelpLineEstimates = aslCalloc(nResults * sizeof(pSession->pnHelpLineEstimates[0]));

			if (!pSession->pnHelpLineEstimates)
			{
				aslRGDestroy(pSession->pRG);
				return decumaAllocationFailed;
			}
		}

		pSession->nResults = nResults;
		pSession->nLastNotedResult = 0;

		if (pSession->pSymbolChars) aslFree(pSession->pSymbolChars);
		pSession->pSymbolChars = aslCalloc((aslArcSessionGetArcCount(pSession->pArcSession)+1) * sizeof(pSession->pSymbolChars[0]));
		if (!pSession->pSymbolChars)
		{
			aslRGDestroy(pSession->pRG);
			return decumaAllocationFailed;
		}

		if (pSession->pSymbolStrokes) aslFree(pSession->pSymbolStrokes);
		pSession->pSymbolStrokes = aslCalloc((aslArcSessionGetArcCount(pSession->pArcSession)+1) * sizeof(pSession->pSymbolStrokes[0]));
		if (!pSession->pSymbolStrokes)
		{
			aslRGDestroy(pSession->pRG);
			return decumaAllocationFailed;
		}

		for ( i = 0; i < nResults; i++)
		{
			DECUMA_HWR_RESULT * pResult = &pResults[i];
			DECUMA_INT16 nSymbolCharsLen, nResultingSymbolCharsLen;
			DECUMA_INT16 nSymbolMaskLen, nResultingSymbolMaskLen;
			DECUMA_INT16 nSymbols;

			aslRGGetString(pSession->pRG,pResult->pChars,nMaxCharsPerResult,
				&pResult->nChars,&pResult->nResultingChars, i,
				pSession->pSymbolChars,aslArcSessionGetArcCount(pSession->pArcSession)+1,&nSymbolCharsLen,&nResultingSymbolCharsLen,
				pSession->pSymbolStrokes,aslArcSessionGetArcCount(pSession->pArcSession)+1,&nSymbols,&pResult->nResultingSymbols,
				pResult->pSymbolArcTimelineDiffMask,nMaxCharsPerResult,&nSymbolMaskLen,&nResultingSymbolMaskLen,
				pSession->pnBaseLineEstimates ? &pSession->pnBaseLineEstimates[i] : NULL,
				pSession->pnHelpLineEstimates ? &pSession->pnHelpLineEstimates[i] : NULL,
				&pResult->bGesture,&pResult->bInstantGesture);

			decumaAssert(nSymbols == nSymbolCharsLen);
			if (pResult->nChars == pResult->nResultingChars) {
				decumaAssert(nSymbols == pResult->nResultingSymbols);
				decumaAssert(nSymbolCharsLen == nResultingSymbolCharsLen);
			}

			pResult->nSymbols = ASL_MIN(nSymbols, nMaxCharsPerResult);

			if (pResult->pSymbolChars) decumaMemcpy(pResult->pSymbolChars, pSession->pSymbolChars, pResult->nSymbols * sizeof(pSession->pSymbolChars[0]));
			if (pResult->pSymbolStrokes) decumaMemcpy(pResult->pSymbolStrokes, pSession->pSymbolStrokes, pResult->nSymbols * sizeof(pSession->pSymbolStrokes[0]));

			decumaAssert(pResult->pSymbolChars==0 || nSymbols > nMaxCharsPerResult || pResult->nSymbols == nSymbolCharsLen);
			decumaAssert(pResult->pSymbolChars==0 || pResult->nResultingSymbols == nResultingSymbolCharsLen);
			decumaAssert(pResult->pSymbolArcTimelineDiffMask==0 || pResult->pSymbolChars==0 ||
				pResult->nSymbols == nSymbolMaskLen);
			decumaAssert(pResult->pSymbolArcTimelineDiffMask==0 || pResult->pSymbolChars==0 ||
				pResult->nResultingSymbols == nResultingSymbolMaskLen);

			pResult->distance = getMeanDistance(pSession, aslRGGetStringDist(pSession->pRG,i),
				aslSGGetNodeCount(pSession->pSG));

			pResult->shapeDistance = getMeanDistance(pSession, aslRGGetStringShapeDist(pSession->pRG,i),
				aslSGGetNodeCount(pSession->pSG));


			if (pRecognitionSettings->boostLevel != noBoost)
			{
				/* RG can effectively determine string type when constructed using dictionary. */
				pResult->stringType = aslRGGetStringType(pSession->pRG,i,&pResult->dictStrStart, &pResult->dictStrLen);
			}
			else
			{
				/* Otherwise a direct dictionary look-up is required. */
				status = getStringType(pSession,pResult->pChars,pResult->nChars, &pResult->stringType,
					&pResult->dictStrStart, &pResult->dictStrLen);
				if (status != decumaNoError) break;
			}

			/*Note that this also might change pResult->dictStrStart and pResult->dictStrLen, therefore we */
			/*Therefore we have to call adjustResultForForbiddenBigrams after aslRGGetStringType */
			adjustResultForForbiddenBigrams(pSession->pArcSession, pResult->pChars,
				&pResult->nChars,&pResult->nResultingChars,
				pSession->pSymbolChars,&nSymbolCharsLen,&nResultingSymbolCharsLen,
				pSession->pSymbolStrokes,&nSymbols,&pResult->nResultingSymbols,
				&pResult->dictStrStart,&pResult->dictStrLen);

			decumaAssert(nSymbols == nSymbolCharsLen);
			if (pResult->nChars == pResult->nResultingChars) {
				decumaAssert(nSymbols == pResult->nResultingSymbols);
				decumaAssert(nSymbolCharsLen == nResultingSymbolCharsLen);
			}

			pResult->nSymbols = ASL_MIN(nSymbols, nMaxCharsPerResult);

			if (pResult->pSymbolChars) decumaMemcpy(pResult->pSymbolChars, pSession->pSymbolChars, pResult->nSymbols * sizeof(pSession->pSymbolChars[0]));
			if (pResult->pSymbolStrokes) decumaMemcpy(pResult->pSymbolStrokes, pSession->pSymbolStrokes, pResult->nSymbols * sizeof(pSession->pSymbolStrokes[0]));

			decumaAssert(nSymbols > nMaxCharsPerResult || pResult->nSymbols == nSymbolCharsLen || !pResult->pSymbolChars);
			decumaAssert(pResult->nResultingSymbols == nResultingSymbolCharsLen || !pResult->pSymbolChars);

			if (pCharArcIdxData)
			{
				DECUMA_UINT16 * pnStrokesArr = (DECUMA_UINT16*) pCharArcIdxData + i*nMaxCharsPerResult;
				DECUMA_UINT16 * pCharStrokeMtx = (DECUMA_UINT16 *)pCharArcIdxData + nMaxResults*nMaxCharsPerResult +
					+ nResults * aslRGGetCharStrokeMtxSize(nMaxCharsPerResult);

				aslRGSetCharStrokesData(pSession->pRG, pCharStrokeMtx,pnStrokesArr, i, nMaxCharsPerResult, pResult->nChars);
			}
		}

		if (pSession->pSymbolStrokes) aslFree(pSession->pSymbolStrokes);
		if (pSession->pSymbolChars) aslFree(pSession->pSymbolChars);

		/* Fix duplicates introduced by adjustResultForForbiddenBigrams */
		nResults = adjustResultForDuplicates(pResults, nResults);

#if defined(_DEBUG) || defined(DECUMA_ASSERT_ENABLE)
		for ( i = 0; i < nResults; i++)
		{
			DECUMA_HWR_RESULT * pResult = &pResults[i];
			decumaAssert(pResult->dictStrStart + pResult->dictStrLen <= pResult->nResultingChars);
		}
#endif


		/* Store current estimates (if existing) */
		if ((pSession->pSessionSettings->supportLineSet == noSupportlines ||
			 pSession->bPositionAndScaleInvariant) &&
			pSession->nResults > 0 &&
			pSession->pnBaseLineEstimates[pSession->nLastNotedResult] != pSession->pnHelpLineEstimates[pSession->nLastNotedResult])
		{
			pSession->baselinePos = pSession->pnBaseLineEstimates[pSession->nLastNotedResult];
			pSession->distBaseToHelpLine = pSession->pnBaseLineEstimates[pSession->nLastNotedResult] - pSession->pnHelpLineEstimates[pSession->nLastNotedResult];
			pSession->bReferenceEstimated = 1;
		}

#ifdef EXTRA_VERIFICATION_INFO
		pSession->bDelayedRGDestruction=1;
#else
		aslRGDestroy(pSession->pRG); /*The best design is to free the RG here, but it is not good when debugging with wingui */
#endif

		if (status != decumaNoError) return status;
	}

	*pnResults = (DECUMA_UINT16) nResults;

	return status;
}

static DECUMA_STATUS setSessionSettings(ASL_SESSION * pSession,
										const DECUMA_SESSION_SETTINGS *pSessionSettings)
{
	int distBaseToHelpLine = 0;
	int baselinePos = 0;

	DECUMA_UNICODE *pTmpSymbols = NULL;
	DECUMA_UNICODE **ppTmpSymbols = NULL;
	const DECUMA_MEM_FUNCTIONS * pMemFunctions = pSession->pMemFunctions;
	CATEGORY_TABLE_PTR pNewCatTable = NULL;
	DECUMA_CHARACTER_SET * pNewCharSet = NULL;
	int bUpdateSupportlines=0, bUpdateCatTable=0;
	DECUMA_STATUS status=decumaNoError;

#if defined(_DEBUG) || defined(DECUMA_ASSERT_ENABLE)
	/*This should already have been checked by decuma_hwr layer */
	status = aslValidateSessionSettings(pSessionSettings);
	decumaAssert(status == decumaNoError||status == decumaAllocationFailed);
#endif

	decumaAssert(pSessionSettings->recognitionMode == scrMode || pSessionSettings->recognitionMode == mcrMode);

	if (!pSession->pSessionSettings ||
		pSessionSettings->supportLineSet != pSession->pSessionSettings->supportLineSet ||
		pSessionSettings->baseline != pSession->pSessionSettings->baseline ||
		pSessionSettings->helpline != pSession->pSessionSettings->helpline)
	{
		bUpdateSupportlines = 1;

		switch (pSessionSettings->supportLineSet)
		{
		case baselineAndHelpline:
			decumaAssert (pSessionSettings->baseline > pSessionSettings->helpline);
			baselinePos = pSessionSettings->baseline;
			distBaseToHelpLine = pSessionSettings->baseline - pSessionSettings->helpline;
			break;
		case baselineAndTopline:
			decumaAssert (pSessionSettings->baseline > pSessionSettings->topline);
			baselinePos = pSessionSettings->baseline;
			distBaseToHelpLine = (pSessionSettings->baseline - pSessionSettings->topline)/2;
			break;
		case helplineAndTopline:
			decumaAssert (pSessionSettings->helpline > pSessionSettings->topline);
			distBaseToHelpLine = (pSessionSettings->helpline - pSessionSettings->topline);
			baselinePos = pSessionSettings->helpline + distBaseToHelpLine;
			break;
		case noSupportlines:
			/* TODO This does not give the same result as setting baselinePos to 0!! Fix! */
			baselinePos = pSessionSettings->baseline;
			distBaseToHelpLine = pSessionSettings->baseline - pSessionSettings->helpline;
			break;
		default: decumaAssert(0);
		}

	}
	/* else no need to refresh internal state with identical support line setting (might only destroy refined estimate) */

	if (pSessionSettings->pCharSetExtension)
	{
		int n=0;
		int i=0,nSymbols=0;
		int charSetExtensionLen=0;
		DECUMA_STATUS dcStatus;
		DECUMA_CHARACTER_SET tmpCharSet;
		DECUMA_UINT32 oneLanguage, oneSymCat;
		int newCharSetSize;


		if (pSessionSettings->pCharSetExtension[i])
		{
			while (pSessionSettings->pCharSetExtension[i] ||
				pSessionSettings->pCharSetExtension[i+1])
			{
				if (pSessionSettings->pCharSetExtension[i]  && !pSessionSettings->pCharSetExtension[i+1])
				{
					nSymbols++;
				}
				i++;
			}
			charSetExtensionLen = i+2;
		}
		else
			charSetExtensionLen = 1;

		if (pSession->pSessionSettings && pSession->pSessionSettings->pCharSetExtension &&
			decumaMemcmp(pSessionSettings->pCharSetExtension,pSession->pSessionSettings->pCharSetExtension,
			charSetExtensionLen*sizeof(pSessionSettings->pCharSetExtension[0]))==0)
		{
			/*Identical to previous settings. No need to update catTable */
			goto pCharSetExtension_handled;
		}
		/* --------- DC UPDATE ----------- */

		if (nSymbols > 0)
		{
			bUpdateCatTable = 1;
			/*Use one of the user defined categories for the added symbols. */
			/*Also activate this for the active character set. */
			tmpCharSet.nLanguages =1;
			tmpCharSet.nSymbolCategories =1;
			tmpCharSet.pLanguages = &oneLanguage;
			tmpCharSet.pSymbolCategories = &oneSymCat;
			oneLanguage = DECUMA_LANG_USER_DEFINED_END;
			oneSymCat = DECUMA_CATEGORY_USER_DEFINED_END;

			pTmpSymbols = aslCalloc(charSetExtensionLen*sizeof(pTmpSymbols[0]));
			if (!pTmpSymbols)
			{
				status = decumaAllocationFailed;
				goto setSessionSettings_error;
			}

			ppTmpSymbols = aslCalloc(nSymbols*sizeof(ppTmpSymbols[0]));
			if (!ppTmpSymbols)
			{
				status = decumaAllocationFailed;
				goto setSessionSettings_error;
			}

			ppTmpSymbols[n++]=&pTmpSymbols[0];

			for (i=0; i<charSetExtensionLen-1; i++) /*including one terminating zero */
			{
				pTmpSymbols[i] = pSessionSettings->pCharSetExtension[i];
				if (pSessionSettings->pCharSetExtension[i] && !pSessionSettings->pCharSetExtension[i+1])
				{
					pTmpSymbols[++i] = 0;
					if (n == nSymbols) break;
					ppTmpSymbols[n++]=&pTmpSymbols[i+1]; /*Next word */
				}
			}
			decumaAssert(n==nSymbols);
			decumaAssert(pTmpSymbols[i]==0);


			/*Update the dynamic categories accordingly */
			pNewCatTable = aslCalloc(dcGetTableSize((STATIC_DB_PTR)pSessionSettings->pStaticDB));
			if (!pNewCatTable)
			{
				status = decumaAllocationFailed;
				goto setSessionSettings_error;
			}
			dcStatus=dcInit(pNewCatTable, (STATIC_DB_PTR)pSessionSettings->pStaticDB);
			decumaAssert(dcStatus==decumaNoError);

			dcStatus=dcCopyStaticCategoryTable(pNewCatTable, (STATIC_DB_PTR)pSessionSettings->pStaticDB);
			decumaAssert(dcStatus==decumaNoError);

			dcStatus=dcAdd(pNewCatTable, (STATIC_DB_PTR)pSessionSettings->pStaticDB,
				&tmpCharSet, (const DECUMA_UNICODE**) ppTmpSymbols, nSymbols);

			/*Errors, e.g. symbols not in database should already have been detected during aslValidateSessionSettings */
			decumaAssert(dcStatus==decumaNoError);

			aslFree(ppTmpSymbols);
			aslFree(pTmpSymbols);

			dcStatus=dcFinish(pNewCatTable, (STATIC_DB_PTR)pSessionSettings->pStaticDB);
			decumaAssert(dcStatus==decumaNoError);

			/*The category table is done. Now create the new character set */
			newCharSetSize = sizeof(pSessionSettings->charSet) +
				(pSessionSettings->charSet.nSymbolCategories+1)*sizeof(pSessionSettings->charSet.pSymbolCategories[0])+
				(pSessionSettings->charSet.nLanguages+1)*sizeof(pSessionSettings->charSet.pLanguages[0]);

			pNewCharSet = aslCalloc(newCharSetSize);
			if (!pNewCharSet)
			{
				status = decumaAllocationFailed;
				goto setSessionSettings_error;
			}
			*pNewCharSet = pSessionSettings->charSet;
			pNewCharSet->pLanguages = (DECUMA_UINT32*)(&pNewCharSet[1]);
			decumaMemcpy(pNewCharSet->pLanguages,pSessionSettings->charSet.pLanguages,
				pSessionSettings->charSet.nLanguages*sizeof(pNewCharSet->pLanguages[0]));
			/*Add the new language */
			pNewCharSet->pLanguages[pNewCharSet->nLanguages++]=oneLanguage;

			pNewCharSet->pSymbolCategories = (DECUMA_UINT32*)(&pNewCharSet->pLanguages[pNewCharSet->nLanguages]);
			decumaMemcpy(pNewCharSet->pSymbolCategories,pSessionSettings->charSet.pSymbolCategories,
				pSessionSettings->charSet.nSymbolCategories*sizeof(pNewCharSet->pSymbolCategories[0]));
			/*Add the new category */
			pNewCharSet->pSymbolCategories[pNewCharSet->nSymbolCategories++]=oneSymCat;

		}

		/* ----- End of DC UPDATE ----------- */

	}

pCharSetExtension_handled:

	/*////////////////// */
	/* Everything is OK - do the assignments first here */
	/* In this way we can stay with the old settings upon any error */
	/*////////////////// */

	if (bUpdateCatTable)
	{
		if (pSession->pCatTable) aslFree(*((struct _tagCATEGORY_TABLE**)&pSession->pCatTable));
		pSession->pCatTable=pNewCatTable;

		if (pSession->pCharSet) aslFree(pSession->pCharSet);
		pSession->pCharSet = pNewCharSet;
	}

	if (bUpdateSupportlines)
	{
		pSession->baselinePos = baselinePos;
		pSession->distBaseToHelpLine = distBaseToHelpLine;
		pSession->bReferenceEstimated = 0;
		pSession->nPrevBaselinePos = baselinePos;
		pSession->nPrevDistBaseToHelpLine = distBaseToHelpLine;
		pSession->bPrevReferenceEstimated = 0;
	}

	pSession->pSessionSettings =pSessionSettings;

	return decumaNoError;

setSessionSettings_error:
	if (pNewCatTable)
	{
		aslFree(*((struct _tagCATEGORY_TABLE**)&pNewCatTable));
	}
	if (pNewCharSet)
	{
		aslFree(pNewCharSet);
	}
	if (ppTmpSymbols)
	{
		aslFree(ppTmpSymbols);
	}
	if (pTmpSymbols)
	{
		aslFree(pTmpSymbols);
	}

	return status;
}


static DECUMA_STATUS getStringType(ASL_SESSION * pSession,
								   DECUMA_UNICODE * pChars, int nChars,
								   DECUMA_STRING_TYPE *pStringType,
								   DECUMA_INT16 * pDictStrStart,
								   DECUMA_INT16 * pDictStrLen)
{
	DECUMA_STATUS status = decumaNoError;

	DECUMA_UNUSED_PARAM(pSession);
	DECUMA_UNUSED_PARAM(pChars);
	DECUMA_UNUSED_PARAM(nChars);

	decumaAssert(pStringType);
	decumaAssert(pDictStrStart);
	decumaAssert(pDictStrLen);


	*pStringType = notFromDictionary;
	*pDictStrStart = 0;
	*pDictStrLen = 0;
	/*TODO: */
	/*If the user has provided a dictionary but is running in noBoost mode we */
	/*could still do the caller the service to tell whether or not the results */
	/*are dictionary words. */
	/*The best solution is then to call RG so that it can use the same algorithm */
	/*for dictionary expansion as it does in boost and force modes. */

	return status;
}

static DECUMA_UINT32	getMeanDistance(ASL_SESSION * pSession, DECUMA_UINT32 nTotalDist, int nNodes)
{
	/*Roughly: Each arc adds a connection distance and a distance. */
	/*Get the mean addition for each arc. */
	/*However, the first arc does not add connection distance... */

	int nContributingArcs = nNodes-1; /*1 less than nNodes */
	decumaAssert(nContributingArcs>0 || nTotalDist>=DECUMA_MAX_DISTANCE);

	DECUMA_UNUSED_PARAM(pSession);

	if (nContributingArcs > 0 && nTotalDist<DECUMA_MAX_DISTANCE)
	{
		return nTotalDist / nContributingArcs; /*Divide with the number of segments */
	}
	else
	{
		return nTotalDist;
	}
}

/************ Private dictionary functions ************* / */

static DECUMA_STATUS getAslSessionStatus(const ASL_SESSION * pSession)
{
	if (pSession == NULL) return decumaNullSessionPointer;
	if (pSession->pArcSession != (ASL_ARC_SESSION *)&pSession[1]) return decumaSessionNotInitialized;

	if (pSession->bIsCorrupt)
	{
		return decumaSessionCorrupt;
	}
	else if (pSession->bAddingArcs && (aslArcSessionIsCorrupt(pSession->pArcSession) || aslSGIsCorrupt(pSession->pSG)))
	{
		return decumaSessionCorrupt;
	}

	return decumaNoError;
}


static DECUMA_STATUS attachDictionary(ASL_SESSION * pSession,
									  const void * pDictionaryData)
{
	const DECUMA_MEM_FUNCTIONS* pMemFunctions = pSession->pMemFunctions;
	int i;
	DECUMA_STATUS status = decumaNoError;
	int nDictionariesPreAttachAttempt;
	DECUMA_HWR_DICTIONARY * pDictionary = NULL;

	decumaAssert(pSession);
	decumaAssert(pDictionaryData);
	/* It is assumed that input has already been validated */
	nDictionariesPreAttachAttempt = pSession->nDictionaries;
	for ( i = 0; i < pSession->nDictionaries; i++ )
	{
		if (pDictionaryData == decumaDictionaryGetDataPtr(pSession->pDictionaries[i])) break;
	}

	if ( i != pSession->nDictionaries )
	{
		status = decumaAlreadyAttached;
		goto attachDictionary_error;
	}

	pDictionary = aslCalloc(decumaDictionaryGetSize());
	if (!pDictionary )
	{
		status = decumaAllocationFailed;
		goto attachDictionary_error;
	}

	status = decumaDictionaryInitWithData(pDictionary,pDictionaryData);
	if (status != decumaNoError)
		goto attachDictionary_error;

	/* Reallocate pDictionaries if needed */
	if ( pSession->nDictionaries == pSession->nMaxDictionaries )
	{
		DECUMA_HWR_DICTIONARY ** pNewDictionaries;

		decumaAssert(i == pSession->nMaxDictionaries);

		pNewDictionaries = aslCalloc( (pSession->nMaxDictionaries + 1) * sizeof(pNewDictionaries[0]));

		if ( pNewDictionaries == NULL )
		{
			status = decumaAllocationFailed;
			goto attachDictionary_error;
		}

		pSession->nMaxDictionaries += 1;
		decumaMemcpy(pNewDictionaries, pSession->pDictionaries, pSession->nDictionaries * sizeof(pNewDictionaries[0]));
		aslFree(pSession->pDictionaries);
		pSession->pDictionaries = pNewDictionaries;
	}

	pSession->pDictionaries[i] = pDictionary;
	pSession->nDictionaries++;

	return status;

attachDictionary_error:

	pSession->nDictionaries = nDictionariesPreAttachAttempt;
	if (pDictionary)
	{
		/* Don't call decumaDictionaryDestroyData it is called via */
		/* aslDestroyUnpackedDictionary at a later point, if needed */
		aslFree(pDictionary);
	}
	return status;

}

#ifdef DECUMA_HWR_ENABLE_EXTRA_API
long aslGetMemoryStatistics(char *pBuf,
									 long nBufLen)
{
	long nCharsWritten = 0;

#ifdef DECUMA_DEBUG_MEMORY
	int i;
	for (i = 0; i < MODULE_COUNT; i++)
	{
		nCharsWritten += Debug_sprintMemoryStatistics(i, 100, &pBuf[nCharsWritten], nBufLen-nCharsWritten);
		nCharsWritten += Debug_sprintMemoryErrors(i, &pBuf[nCharsWritten], nBufLen-nCharsWritten);
	}
#endif

	return nCharsWritten;
}
#endif /* DECUMA_HWR_ENABLE_EXTRA_API */

#ifdef EXTRA_VERIFICATION_INFO
ASL_SG * aslGetSG(ASL_SESSION * pSession)
{
	return pSession->pSG;
}

ASL_SG * aslGetForcedSG(ASL_SESSION * pSession)
{
	return pSession->pForcedSG;
}

ASL_RG * aslGetRG(ASL_SESSION * pSession)
{
	return pSession->pRG;
}

ASL_RG * aslGetForcedRG(ASL_SESSION * pSession)
{
	return pSession->pForcedRG;
}

ASL_ARC_SESSION * aslGetArcSession(ASL_SESSION * pSession)
{
	return pSession->pArcSession;
}
#endif /*EXTRA_VERIFICATION_INFO */

/* Undefine private macros */
#undef NUM_DICTIONARIES
#undef STATIC_DICTIONARY
#undef PERSONAL_DICTIONARY
#undef SESSION_NOT_INITIALIZED
#undef USE_TERSE_TRIE


/*********** END ***********/
