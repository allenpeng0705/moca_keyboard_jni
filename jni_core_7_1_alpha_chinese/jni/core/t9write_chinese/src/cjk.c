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

#define CJK_C
#define CJK_SYSTEM_CODE

/* *************************************************************************
 * Includes
 * ************************************************************************* */

#include "cjk.h"
#include "dltConfig.h"

#include "cjkSession.h"
#include "cjkArcSession.h"
#include "cjkVersion.h"
#include "cjkCommon.h"
#include "cjkCompressedCharacter_Macros.h"
#include "cjkSession_Types.h"
#include "cjkDatabase.h"
#include "cjkDatabaseFormat_Macros.h"
#include "cjkDynamicDatabase.h"

#include "decumaInterrupts.h"
#include "decumaCategoryCombinationData.h"
#include "decumaSymbolCategories.h"
#include "decumaLanguages.h"

#include "decumaQsort.h"
#include "decumaMemory.h"
#include "decumaUnicode.h"
#include "decumaAssert.h"
#include "decumaCommon.h"
#include "decumaBasicTypes.h"
#include "decumaString.h"
#include "decumaStatus.h"
#include "decumaCommon.h"

#include "decumaMacroVerifier.h" /* for DECUMA_SPECIFIC_DB_STORAGE */


/* Check if DECUMA_DB_STORAGE is defined. This is not yet supported for cjk */

#if !defined(DECUMA_SPECIFIC_DB_STORAGE) || !(DECUMA_SPECIFIC_DB_STORAGE == 1 || DECUMA_SPECIFIC_DB_STORAGE == 0)

#error DECUMA_SPECIFIC_DB_STORAGE must be defined to 1 or 0

#endif

#if DECUMA_SPECIFIC_DB_STORAGE == 1

#error DECUMA_DB_STORAGE other than empty is not yet supported by cjk engine.

#endif

/* *************************************************************************
 * Internal functions
 * ************************************************************************* */

static DECUMA_STATUS cjkSetPersonalCategoryBitArray(CJK_SESSION * pSession, const DECUMA_SESSION_SETTINGS * pSessionSettings);
static DECUMA_STATUS dltConvertCategory(DECUMA_UINT32 cat, DECUMA_UINT32 * mask);
static DECUMA_STATUS dltConvertLanguage(DECUMA_UINT32 lang, DECUMA_UINT32 * mask);

/* *************************************************************************
 * Exported API
 * ************************************************************************* */

const char * cjkGetVersion(void) 
{
	return (const char*)(CJKLIB_VERSION_STR);
}


DECUMA_UINT32 cjkGetSessionSize(void) 
{
	return (cjkSessionGetSize() + cjkArcSessionGetSize() + cjkDbGetSessionSize() + cjkSessionGetScratchpadSize());
}

DECUMA_STATUS cjkBeginSession(CJK_SESSION * pSession,
	DECUMA_SESSION_SETTINGS * pSessionSettings,
	const DECUMA_MEM_FUNCTIONS * pMemFunctions)
{
	DECUMA_STATUS status;

	decumaAssert(pSession);
	decumaAssert(pSessionSettings);
	if (pSessionSettings->pStaticDB == NULL) return decumaNullDatabasePointer;
	
	status = cjkSessionInit(pSession, pSessionSettings, pMemFunctions);

	return status;
}

DECUMA_STATUS cjkValidateStaticDatabase(DECUMA_STATIC_DB_PTR pStaticDB)
{
	return cjkDbIsValid(pStaticDB);
}

DECUMA_STATUS cjkValidateDynamicDatabase(DECUMA_DYNAMIC_DB_PTR pDynamicDB)
{
	return cjkDynamicDatabaseIsValid(pDynamicDB);
}

DECUMA_STATUS cjkVerifySession(const CJK_SESSION * pSession)
{
	decumaAssert(pSession);

	/* TODO Actually do session verification */

	if ( !CJK_SESSION_IS_INITIALIZED(pSession) ) {
		return decumaSessionNotInitialized;
	}

	if ( (pSession->sessionCategories & pSession->db.categoryMask) != pSession->sessionCategories )
		return decumaSessionCorrupt;

	return decumaNoError;
}

DECUMA_STATUS cjkEndSession(CJK_SESSION * pSession)
{
	DECUMA_UNUSED_PARAM(pSession);
	return decumaNoError;
}

DECUMA_STATUS cjkDatabaseGetVersion(DECUMA_ANY_DB_PTR pDB, char * pBuf, int nBufLen)
{
 	DECUMA_STATUS status;
	DLTDB db;
	int nDbStringLength;

	DECUMA_UNUSED_PARAM(nBufLen);

	/* TODO: Support version for dynamic db */
	if (cjkValidateDynamicDatabase(pDB) == decumaNoError) {
		decumaMemcpy (pBuf, "Unknown DB version", 19);

		return decumaNoError;
	}


	status = cjkDbInit(&db, pDB);

	if (status != decumaNoError) {
		return status;
	}

	nDbStringLength = decumaStrlenUTF8(db.pVersionStr);

	decumaAssert(pBuf);
	decumaAssert(nBufLen >= DECUMA_DATABASE_VERSION_STRING_LENGTH);
	decumaAssert(nDbStringLength + 1 < DECUMA_DATABASE_VERSION_STRING_LENGTH);

	decumaMemcpy (pBuf, db.pVersionStr, nDbStringLength + 1);

	return decumaNoError;
}

DECUMA_STATUS cjkDatabaseIsCategorySupported(DECUMA_ANY_DB_PTR pDB,
	DECUMA_UINT32 cat, int * pbIsSupported)
{
	DECUMA_STATUS status;
	DECUMA_UINT32 mask = 0;
	DECUMA_UINT32 langMask = 0;
	DLTDB db;

	decumaAssert(pDB);
	decumaAssert(pbIsSupported);

	*pbIsSupported = 0;

	/* TODO: Support this for dynamic db */
	if (cjkValidateDynamicDatabase(pDB) == decumaNoError) {

		return decumaFunctionNotSupported;
	}

	status = cjkDbGetLanguage(pDB, &langMask);

	/* Special FIX for dual category */
	if (cat == DECUMA_CATEGORY_TRADITIONAL_FORM && langMask != DECUMA_LANG_PRC) {
		return decumaNoError; /* pbIsSupported == 0*/
	}
	if (cat == DECUMA_CATEGORY_SIMPLIFIED_FORM &&
			!(langMask == DECUMA_LANG_TW || langMask == DECUMA_LANG_HK)) {
		return decumaNoError; /* pbIsSupported == 0*/
	}

	status = dltConvertCategory(cat, &mask);

	if (status == decumaUnsupportedSymbolCategory) {
		return decumaNoError; /* pbIsSupported == 0*/
	}
	else if (status != decumaNoError) 
	{	
		return status;
	}

	/** pDB is here not the DLTDB pointer but the static DB pointer
	 *  so we need to get the mask manually
	 */	
	status = cjkDbInit(&db, pDB);

	if (status != decumaNoError) return status;

	*pbIsSupported = ((db.categoryMask & mask) == mask); 
		
	return decumaNoError;
}

DECUMA_STATUS cjkDatabaseIsLanguageSupported(DECUMA_ANY_DB_PTR pDB,
	DECUMA_UINT32 lang, int * pbIsSupported)
{
	DECUMA_STATUS status;
	DECUMA_UINT32 langMask = 0;
	DLTDB database;

	decumaAssert(pbIsSupported);

	*pbIsSupported = 0;

	/* TODO: Support this for dynamic db */
	if (cjkValidateDynamicDatabase(pDB) == decumaNoError) {
		return decumaFunctionNotSupported;
	}

	status = cjkDbInit(&database, pDB);

	if (status != decumaNoError) return status;

	if (lang == DECUMA_LANG_PRC || lang == DECUMA_LANG_TW || lang == DECUMA_LANG_HK ||
		lang == DECUMA_LANG_JP || lang == DECUMA_LANG_KO) {
		status = cjkDbGetLanguage(pDB, &langMask);

		if (status != decumaNoError) return status;

		/* Language not supported so return 0 in pbIsSupported */
		if (langMask != lang) return decumaNoError;
	}

	status = dltConvertLanguage(lang, &langMask);

	/* If unsupported return noError, but with 0 for is supported */
	if (status == decumaUnsupportedLanguage) return decumaNoError;
	else if (status != decumaNoError) return status;

	*pbIsSupported = (langMask & cjkDbGetCategoryMask(&database));

	return decumaNoError;
}


DECUMA_STATUS cjkDatabaseIncludesSymbol(
	DECUMA_ANY_DB_PTR pDB,
	const DECUMA_CHARACTER_SET * pCharSet,
	const DECUMA_UNICODE * pSymbol, 
	int * pbIsIncluded)
{
	DECUMA_STATUS status;
	DLTDB db;

	DECUMA_UNUSED_PARAM(pCharSet);

	decumaAssert(pSymbol);
	decumaAssert(pbIsIncluded);
	*pbIsIncluded = 0;

	/* TODO: Support this for dynamic db */
	if (cjkValidateDynamicDatabase(pDB) == decumaNoError) {
		return decumaFunctionNotSupported;
	}

	status = cjkDbInit(&db, pDB);
	if (status != decumaNoError) return status;

	if (pSymbol[0]) {
		int i;
		for (i = 0; i < db.maxindex; i++) {
			if (DLTDB_GET_UNICODE(&db, i) == pSymbol[0]) {

				/* If included also check that it matches the category sent in */
				if ( pCharSet ) {
					DECUMA_UINT32 dbMask;
					DECUMA_UINT32 mask;

					dbMask = DLTDB_GET_CATEGORY(&db, i);
					status = dltCreateCategoryMask(pCharSet, &mask);
					if (status != decumaNoError) return status;

					*pbIsIncluded = ((mask & dbMask) > 0);
				} else {
					*pbIsIncluded = 1;
				}

				break;
			}
		}
	}

	return decumaNoError;
}


DECUMA_STATUS cjkChangeSessionSettings(CJK_SESSION * pSession, DECUMA_SESSION_SETTINGS * pSessionSettings)
{
	DECUMA_STATUS status;

	decumaAssert(pSession);
	decumaAssert(pSessionSettings);

	/* Some simple tests first that does not have to modify current session */

	if ( pSessionSettings->recognitionMode != scrMode )
		return decumaUnsupportedRecognitionMode;

	/* Only the 'boxed' support lines are supported by DLTLIB */
	if ( !(pSessionSettings->UIInputGuide == box || pSessionSettings->UIInputGuide == none))
		return decumaInvalidUIInputGuide;

	if ( pSessionSettings->UIInputGuide == box &&
		( pSessionSettings->box.height < 0 ||
		  pSessionSettings->box.width < 0 ) )
		return decumaInvalidSupportLines;

	if ( pSessionSettings->pStaticDB == NULL) {
		return decumaNullDatabasePointer;
	}
	/* We need to initialize the new database to make cjkSetCategories behave
	 * correctly when changing both database and character set at once
	 */
	status = cjkDbInit(&pSession->db, pSessionSettings->pStaticDB);
	if ( status != decumaNoError)
		goto changeSessionSettings_error;

	/* Make sure the categories are valid */
	status = cjkSetCategories( pSession, pSessionSettings );
	if (status != decumaNoError)
		goto changeSessionSettings_error;

	/* Everything looks good */
	pSession->pSessionSettings = pSessionSettings;

	return decumaNoError;

changeSessionSettings_error:

	/* Revert to previous database, since we might have re-initialized it */
	{
		DECUMA_STATUS revertStatus = cjkDbInit(&pSession->db, pSession->pSessionSettings->pStaticDB);
		decumaAssert(revertStatus == decumaNoError);
	}

	return status;
}

DECUMA_STATUS cjkBeginArcAddition(CJK_SESSION * pSession)
{
	decumaAssert(pSession);

	return cjkArcSessionInit(pSession->pArcSession, (CJK_CC_COMPRESS_SCRATCHPAD *)pSession->pScratchpad);
}

DECUMA_STATUS cjkStartNewArc(CJK_SESSION * pSession, DECUMA_UINT32 arcID)
{
	decumaAssert(pSession);

	/*TODO: Please remove this as soon as KO_MARKET db has been reoptimized ... */
	/* Ugly special hack to get HangulSampling with Hangul category set for addPoint */
	cjkArcSessionSetSamplingRule(pSession->pArcSession, pSession->sessionCategories, 3);

	return cjkArcSessionStartNewArc(pSession->pArcSession, arcID);
}

DECUMA_STATUS cjkAddPoints(CJK_SESSION * pSession,
								   DECUMA_POINT * pPts,
								   int nPts,
								   DECUMA_UINT32 arcID,
								   int * pnPtsAdded)
{
	decumaAssert(pSession);

	return cjkArcSessionAddPoints(pSession->pArcSession, pPts, nPts, arcID, pnPtsAdded);
}

DECUMA_STATUS cjkAddPoint(CJK_SESSION * pSession,
								  DECUMA_COORD x,
								  DECUMA_COORD y,
								  DECUMA_UINT32 arcID)
{
	decumaAssert(pSession);
	return cjkArcSessionAddPoint(pSession->pArcSession, x, y, arcID);
}

DECUMA_STATUS cjkGetUncommittedArcCount(CJK_SESSION * pSession, int * pnUncommittedArcs)
{
	decumaAssert(pSession);
	decumaAssert(pnUncommittedArcs);

	*pnUncommittedArcs = cjkArcSessionGetUncommittedArcCount(pSession->pArcSession);
	return decumaNoError;
}

DECUMA_STATUS cjkGetUncommittedArcID(CJK_SESSION * pSession, int idx, DECUMA_UINT32 * pArcID)
{
	decumaAssert(pSession);

	return cjkArcSessionGetUncommittedArcID(pSession->pArcSession, idx, pArcID);
}

DECUMA_STATUS cjkCancelArc(CJK_SESSION * pSession, DECUMA_UINT32 arcID)
{
	decumaAssert(pSession);

	return cjkArcSessionCancelArc(pSession->pArcSession, arcID);
}

DECUMA_STATUS cjkCommitArc(CJK_SESSION * pSession,DECUMA_UINT32 arcID)
{
	decumaAssert(pSession);

	cjkArcSessionSetSamplingRule(pSession->pArcSession, pSession->sessionCategories, cjkArcSessionGetArcCount(pSession->pArcSession));

	return cjkArcSessionCommitArc(pSession->pArcSession, arcID);
}

DECUMA_STATUS cjkRecognizeFast( CJK_SESSION * pSession,
	DECUMA_HWR_RESULT * pResult,
	DECUMA_UINT16 * pnResults,  /* In most cases == 1, but can be 0 */
	DECUMA_UINT16 nMaxChars,
	const DECUMA_INTERRUPT_FUNCTIONS * pInterruptFunctions)
{
	DECUMA_UNUSED_PARAM(pSession);
	DECUMA_UNUSED_PARAM(pResult);
	DECUMA_UNUSED_PARAM(pnResults);
	DECUMA_UNUSED_PARAM(nMaxChars);
	DECUMA_UNUSED_PARAM(pInterruptFunctions);

	return decumaFunctionNotSupported;
}


DECUMA_STATUS 
cjkGetCategorymask(const CJK_SESSION * pSession, DECUMA_UINT32 * pCategoryMask) 
{
	if (pSession == NULL) return decumaNullSessionPointer;
	if (pCategoryMask == NULL) return decumaNullPointer;
	
	if (!CJK_SESSION_IS_INITIALIZED(pSession)) {
		return decumaSessionNotInitialized;
	}

	*pCategoryMask = pSession->db.categoryMask;

	return decumaNoError;
}

DECUMA_STATUS cjkRecognize( CJK_SESSION * pSession,
	DECUMA_HWR_RESULT * pResults,
	DECUMA_UINT16 nMaxResults,
	DECUMA_UINT16 * pnResults,
	DECUMA_UINT16 nMaxCharsPerResult,
	const DECUMA_RECOGNITION_SETTINGS * pRecognitionSettings,
	const DECUMA_INTERRUPT_FUNCTIONS * pInterruptFunctions)
{
	DECUMA_INT32 n;
	DECUMA_UINT16 nResultIdx;
	DECUMA_STATUS status = decumaNoError;
	CJK_BESTLIST const * pbl;
	int nStringStartLastIdx = -1;
	CJK_COMPRESSED_CHAR * pCchar;

	DECUMA_UNUSED_PARAM(pInterruptFunctions);
	decumaAssert(pnResults);
	*pnResults = 0;

	if (pSession == NULL) return decumaNullSessionPointer;

	if (pRecognitionSettings == NULL) return decumaNullSettingsPointer;

	if (cjkArcSessionGetUncommittedArcCount(pSession->pArcSession) > 0) return decumaUncommittedArcExists;

	/* if (pResults == NULL) return decumaNullResultPointer; */

	if (!CJK_SESSION_IS_INITIALIZED(pSession)) {
		return decumaSessionNotInitialized;
	}

	status = cjkDbIsValid((DECUMA_STATIC_DB_PTR) pSession->db.pBase);
	if (status != decumaNoError) {
		return status;
	}

	if (pSession->pSessionSettings->UIInputGuide == none) {
		pSession->boxybase  = 0;
		pSession->boxheight = 0;
		pSession->boxxbase  = 0;
		pSession->boxwidth  = 0;
	}
	else {
		pSession->boxybase  = pSession->pSessionSettings->box.ybase;
		pSession->boxheight = pSession->pSessionSettings->box.height;
		pSession->boxxbase  = pSession->pSessionSettings->box.xbase;
		pSession->boxwidth  = pSession->pSessionSettings->box.width;
	}

	/* Set the categorymask in the sessionobject */
	if (pSession->sessionCategories & ~(pSession->db.categoryMask)) {
		return decumaInvalidCategory;
	}
	
	/* Set prevoius unicode in session object. */
	if (pRecognitionSettings->pStringStart && pRecognitionSettings->pStringStart[0] > 0) {
		nStringStartLastIdx=0;
		while (pRecognitionSettings->pStringStart[nStringStartLastIdx+1] > 0) nStringStartLastIdx++;

		/* Find the last element and set this as the previous unicode */
		cjkContextSetPrevious(&pSession->con, pRecognitionSettings->pStringStart[nStringStartLastIdx]); 
	}

	/* Make sure we can fit results in resultstrings after start string */
	if (nMaxCharsPerResult <= nStringStartLastIdx+2)
		return decumaTooShortBuffer;

	pCchar = cjkArcSessionGetCompressedChar(pSession->pArcSession);

	if (pInterruptFunctions != NULL) {
		pSession->pInterruptFunctions = pInterruptFunctions;
	}

	status = cjkDbLookup(pCchar, pRecognitionSettings->boostLevel == boostFrequentChars, pSession, &pbl);
	if (status == decumaCurveZeroArcs)
		return decumaNoError;
	else if ( status != decumaNoError )
		return status;

	/* Abort recognition - *pnResults already 0*/
	if (TEST_ABORT_RECOGNITION(pInterruptFunctions))
		return decumaRecognitionAborted;

	/* Remove duplicate elements from db_lookup_bl
	  can there be duplicates now that fullscreen codes have been removed ? */
	for (nResultIdx = 0, n = 0; nResultIdx < nMaxResults && n < BESTLEN; n++) {
		int j, nAddCode = pbl->unichar[n];
		int bAlreadyInList = 0;
		int nSymbolIdx = 0;

		if (nAddCode == 0) continue;

		/* Make sure that this code is not in list */
		for (j=0; j < nResultIdx; j++) {
			if (pResults[j].pChars[nStringStartLastIdx+1] == nAddCode) {
				bAlreadyInList = 1;
				break;
			}
		}
		if (bAlreadyInList) {
			continue;
		}
		/* Add result to list and increment list index */
		if (nStringStartLastIdx >= 0) {
			decumaMemcpy(&pResults[nResultIdx].pChars[0], &pRecognitionSettings->pStringStart[0], 
				(nStringStartLastIdx+1)*sizeof(pRecognitionSettings->pStringStart[0]));

			/* Add the string start as a null-stroke symbol and then the normal interp */
			if (pResults[nResultIdx].pSymbolChars) {
				pResults[nResultIdx].pSymbolChars[nSymbolIdx] = nStringStartLastIdx+1;
			}
			if (pResults[nResultIdx].pSymbolStrokes) {
				pResults[nResultIdx].pSymbolStrokes[nSymbolIdx] = 0;
			}
			pResults[nResultIdx].nSymbols = ++nSymbolIdx;
		}

		/* We have already validated that nStartStringLastIdx + 2 < nMaxCharsPerResult */
		pResults[nResultIdx].pChars[nStringStartLastIdx+1] = (DECUMA_UNICODE) nAddCode;
		pResults[nResultIdx].pChars[nStringStartLastIdx+2] = (DECUMA_UNICODE) 0;

		/* Adding 2 might look nonsensical at first glance here, but it is correct. */
		pResults[nResultIdx].nResultingChars = nStringStartLastIdx + 2;
		pResults[nResultIdx].nChars = nStringStartLastIdx+2;

		if (pResults[nResultIdx].pSymbolChars) {
			pResults[nResultIdx].pSymbolChars[nSymbolIdx] = 1;
		}
		if (pResults[nResultIdx].pSymbolStrokes) {
			pResults[nResultIdx].pSymbolStrokes[nSymbolIdx] = dltCCCompressGetNbrStrokes(pCchar);
		}
		pResults[nResultIdx].nSymbols = ++nSymbolIdx;
		pResults[nResultIdx].nResultingSymbols = pResults[nResultIdx].nSymbols;

		
		if (dltCCCompressGetNbrPoints(pCchar) > 0) {
			pResults[nResultIdx].distance = pbl->dist[n]*100 / dltCCCompressGetNbrPoints(pCchar);
		}
		else {
			pResults[nResultIdx].distance = LARGEDIST;
		}
		pResults[nResultIdx].shapeDistance = pResults[nResultIdx].distance;

		pResults[nResultIdx].bGesture = decumaIsGesture(nAddCode);
		pResults[nResultIdx].bInstantGesture = 0;

		pResults[nResultIdx].stringType = notFromDictionary;
		/* Intentionally NOT setting pResults[nResultIdx].dictStrStart, it is documented to be undefined when stringType == notFromDictionary */
		pResults[nResultIdx].dictStrLen = 0;

		/* Increment list index */
		nResultIdx++;
	}
	*pnResults = nResultIdx;


	return decumaNoError;
}

DECUMA_STATUS cjkGetDistance( CJK_SESSION * pSession,
	const DECUMA_UNICODE * symbolString,
	const DECUMA_RECOGNITION_SETTINGS * pRecognitionSettings,
	DECUMA_UINT32 * pDistance,
	DECUMA_STRING_TYPE * pStringType)
{
	DECUMA_UNUSED_PARAM(pSession);
	DECUMA_UNUSED_PARAM(symbolString);
	DECUMA_UNUSED_PARAM(pRecognitionSettings);
	DECUMA_UNUSED_PARAM(pDistance);
	DECUMA_UNUSED_PARAM(pStringType);

	return decumaFunctionNotSupported;
}


DECUMA_STATUS cjkValidateRecognitionSettings(const DECUMA_RECOGNITION_SETTINGS * pRecognitionSettings)
{
	if (!(pRecognitionSettings->boostLevel == noBoost || pRecognitionSettings->boostLevel == boostFrequentChars))
		return decumaUnsupportedRecognitionMode;
	return decumaNoError;
}


DECUMA_STATUS cjkValidateSessionSettings(const DECUMA_SESSION_SETTINGS * pSessionSettings)
{
	decumaAssert(pSessionSettings);

	if (pSessionSettings->recognitionMode != scrMode)
		return decumaUnsupportedRecognitionMode;

	if (!(pSessionSettings->UIInputGuide == box || pSessionSettings->UIInputGuide == none))
		return decumaInvalidUIInputGuide;

	if (pSessionSettings->writingDirection != leftToRight)
		return decumaUnsupportedWritingDirection;

	return decumaNoError;
}

/*
 * This function creates an internal category bitmask from the extrenal category values in 
 *  decumaSymbolCategories.h
 */


DECUMA_STATUS dltCreateCategoryMask(const DECUMA_CHARACTER_SET * pCharSet, DECUMA_UINT32 * mask)
{
	/* Create a DLTLIB category mask from a DECUMA_CHARACTER_SET */

	int i;
	DECUMA_UINT32 langMask;
	DECUMA_STATUS status;

	if ( pCharSet == NULL )
		return decumaNullPointer;

	if ( pCharSet->pLanguages == NULL )
		return decumaNullPointer;

	if ( pCharSet->pSymbolCategories == NULL )
		return decumaNullPointer;

	if ( pCharSet->nLanguages == 0 )
		return decumaNoLanguages;

	if ( pCharSet->nSymbolCategories == 0 )
		return decumaNoSymbolCategories;

	*mask = 0;

	/* Parse categories */
	for ( i = 0; i < pCharSet->nSymbolCategories; ++i )
	{
		int k;
		for (k = 0; k < DECUMA_NUMBER_OF_SYMBOL_CATEGORIES; k++) 
		{
			if (pCharSet->pSymbolCategories[i] == decumaAllSymbolCategories[k])
				break;
		}
		if (k >= DECUMA_NUMBER_OF_SYMBOL_CATEGORIES) return decumaInvalidCategory;

		status = dltConvertCategory(pCharSet->pSymbolCategories[i], mask);
		if (status != decumaNoError) return status;
	}
	/* Check that not only writing style category is set */
	if (*mask == CJK_POPULARFORM || *mask == CJK_TRADSIMPDUAL || *mask == (CJK_TRADSIMPDUAL | CJK_POPULARFORM))
		return decumaUnsupportedSymbolCategory;

	/* Create a language mask and check that the categories specified are valid for the given combination of languages */
	langMask = 0;

	for ( i = 0; i < pCharSet->nLanguages; ++i )
	{
		int k;
		for ( k = 0; k < DECUMA_NUMBER_OF_LANGUAGES; k++ )
		{
			if ( pCharSet->pLanguages[i] == decumaAllLanguages[k] )
				break;
		}
		if ( k >= DECUMA_NUMBER_OF_LANGUAGES ) return decumaInvalidLanguage;

		status = dltConvertLanguage(pCharSet->pLanguages[i], &langMask);
		if ( status != decumaNoError )
			return status;
	}

	/* We need to filter langMask with db */

	if ( (*mask & langMask) != *mask )
		return decumaInvalidCategory; /* Invalid combination of language and category ... */

	return decumaNoError;
}

DECUMA_STATUS cjkSetCategories( CJK_SESSION * pSession, const DECUMA_SESSION_SETTINGS * pSessionSettings )
{
	DECUMA_UINT32 full = 0, partial = 0;
	int n = 0;

	DECUMA_STATUS status = cjkGetCategorymask( pSession, &full );
	if ( status != decumaNoError )
		return status;

	pSession->nPersonalCategories = 0;
	/* TODO : Assert the length of pCharSetExtension or 
	   interface in some way
	*/
	if (pSessionSettings->pCharSetExtension) {
		while (pSessionSettings->pCharSetExtension[n] != 0) {
			/* Ignore symbols longer than one Unicode for now */
			if (pSessionSettings->pCharSetExtension[n+1] == 0)
				pSession->nPersonalCategories++;
			/* Find next symbol break */
			while (pSessionSettings->pCharSetExtension[n] != 0) {
				n++;
			}
			/* Go to start of next symbol */
			n++;
		}
		if (n > 0 && pSession->nPersonalCategories == 0) return decumaSymbolNotInDatabase;
	}

	status = dltCreateCategoryMask(&pSessionSettings->charSet, &partial);
	if ( status != decumaNoError ) {
		/* If pCharSetExtension is set allow for no lang and categories */
		if ( pSession->nPersonalCategories == 0 || 
			(status != decumaNoLanguages && status != decumaNoSymbolCategories))
		return status;
	}

	if ( (full & partial) != partial )
		return decumaInvalidCategory;

	cjkSessionSetCategoryMask(pSession, partial);

	/** Convert pCharSetExtension to set bits where each bit is a bit
	 *  corresponding to if template is included in pCharSetExtension
	 */
	if (pSession->db.pBase && pSession->nPersonalCategories > 0) {
		status = cjkSetPersonalCategoryBitArray(pSession, pSessionSettings);
	}

	return status;
}

DECUMA_STATUS cjkEndArcAddition(CJK_SESSION * pSession)
{
	/* Reset arc session points */
	cjkArcSessionResetCurve(pSession->pArcSession);

	pSession->state = STATE_NORMAL;

	return decumaNoError;
}


/************************************************************************\   
*       FUNCTIONS BELOW ARE NOT IMPLEMENTED YET FOR CJK               *
*                                                                        *
\************************************************************************/


DECUMA_STATUS cjkIndicateInstantGesture( 
	CJK_SESSION * pSession,
	int * pbInstantGesture,
	const DECUMA_INSTANT_GESTURE_SETTINGS * pInstantGestureSettings)
{
	DECUMA_UNUSED_PARAM(pSession);
	DECUMA_UNUSED_PARAM(pInstantGestureSettings);
	*pbInstantGesture = 0;
	return decumaNoError;
}

DECUMA_STATUS cjkRecognizeForceRecognition( CJK_SESSION * pSession,
	const DECUMA_UNICODE * symbolString,
	DECUMA_HWR_RESULT * pResults,
	DECUMA_UINT16 nMaxResults,
	DECUMA_UINT16 * pnResults,
	DECUMA_UINT16 nMaxCharsPerResult,
	const DECUMA_RECOGNITION_SETTINGS * pRecognitionSettings,
	int bFast,
	const DECUMA_INTERRUPT_FUNCTIONS * pInterruptFunctions)
{
	DECUMA_UNUSED_PARAM(pSession);
	DECUMA_UNUSED_PARAM(symbolString);
	DECUMA_UNUSED_PARAM(pResults);
	DECUMA_UNUSED_PARAM(nMaxResults);
	DECUMA_UNUSED_PARAM(pnResults);
	DECUMA_UNUSED_PARAM(nMaxCharsPerResult);
	DECUMA_UNUSED_PARAM(pRecognitionSettings);
	DECUMA_UNUSED_PARAM(bFast);
	DECUMA_UNUSED_PARAM(pInterruptFunctions);

	return decumaFunctionNotSupported;
}

DECUMA_STATUS cjkRecognizeForceSegmentation( CJK_SESSION * pSession,
	const DECUMA_UNICODE * symbolString,
	DECUMA_HWR_RESULT * pResults,
	DECUMA_UINT16 nMaxResults,
	DECUMA_UINT16 * pnResults,
	DECUMA_UINT16 nMaxCharsPerResult,
	const DECUMA_RECOGNITION_SETTINGS * pRecognitionSettings,
	int bFast,
	const DECUMA_INTERRUPT_FUNCTIONS * pInterruptFunctions)
{
	DECUMA_UNUSED_PARAM(pSession);
	DECUMA_UNUSED_PARAM(symbolString);
	DECUMA_UNUSED_PARAM(pResults);
	DECUMA_UNUSED_PARAM(nMaxResults);
	DECUMA_UNUSED_PARAM(pnResults);
	DECUMA_UNUSED_PARAM(nMaxCharsPerResult);
	DECUMA_UNUSED_PARAM(pRecognitionSettings);
	DECUMA_UNUSED_PARAM(bFast);
	DECUMA_UNUSED_PARAM(pInterruptFunctions);

	return decumaFunctionNotSupported;
}

DECUMA_STATUS cjkNoteSelectedCandidate( CJK_SESSION * pSession, int nResultIdx)
{
	DECUMA_UNUSED_PARAM(pSession);
	DECUMA_UNUSED_PARAM(nResultIdx);
	return decumaFunctionNotSupported;
}


/******* Dictionary conversion functions *******/

DECUMA_STATUS cjkUnpackXT9Dictionary(void ** ppDictionaryData,
	const void * pXT9Dictionary, 
	DECUMA_UINT32 xt9DataSize, 
	DECUMA_XT9_DICTIONARY_TYPE type,
	DECUMA_UINT32 * pUnpackedDataSize,
	const DECUMA_MEM_FUNCTIONS * pMemFunctions)
{
	DECUMA_UNUSED_PARAM(ppDictionaryData);
	DECUMA_UNUSED_PARAM(pXT9Dictionary);
	DECUMA_UNUSED_PARAM(xt9DataSize);
	DECUMA_UNUSED_PARAM(type);
	DECUMA_UNUSED_PARAM(pUnpackedDataSize);
	DECUMA_UNUSED_PARAM(pMemFunctions);

	return decumaFunctionNotSupported;
}

DECUMA_STATUS cjkDestroyUnpackedDictionary(void ** ppDictionaryData,
	const DECUMA_MEM_FUNCTIONS * pMemFunctions)
{
	DECUMA_UNUSED_PARAM(ppDictionaryData);
	DECUMA_UNUSED_PARAM(pMemFunctions);

	return decumaFunctionNotSupported;
}

/******* Dictionary attachment functions *******/

DECUMA_STATUS cjkAttachStaticDictionary(CJK_SESSION * pSession,
									 const void * pDictionaryData)
{
	DECUMA_UNUSED_PARAM(pSession);
	DECUMA_UNUSED_PARAM(pDictionaryData);

	return decumaFunctionNotSupported;
}

DECUMA_STATUS cjkDetachStaticDictionary(CJK_SESSION * pSession,
									 const void * pDictionary)
{
	DECUMA_UNUSED_PARAM(pSession);
	DECUMA_UNUSED_PARAM(pDictionary);

	return decumaFunctionNotSupported;
}

/******* Dynamic Dictionary functions *******/

DECUMA_STATUS cjkStartNewSymbol(CJK_SESSION * pSession)
{
	DECUMA_UNUSED_PARAM(pSession);
	return decumaFunctionNotSupported;
}

DECUMA_STATUS cjkAttachDynamicDictionary(CJK_SESSION * pSession,
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

DECUMA_STATUS cjkDetachDynamicDictionary(CJK_SESSION * pSession,
														   const void * pDictionaryData)
{
	DECUMA_UNUSED_PARAM(pSession);
	DECUMA_UNUSED_PARAM(pDictionaryData);

	return decumaFunctionNotSupported;
}

DECUMA_STATUS cjkAddWordToDictionary(CJK_SESSION * pSession, void * pDictionaryBuffer, 
												DECUMA_UNICODE * pWord)
{
	DECUMA_UNUSED_PARAM(pSession);
	DECUMA_UNUSED_PARAM(pDictionaryBuffer);
	DECUMA_UNUSED_PARAM(pWord);

	return decumaFunctionNotSupported;
}

DECUMA_STATUS cjkGetDynamicDictionarySize(CJK_SESSION * pSession, const void * pDictionaryBuffer, 
													 int * pSize)
{
	DECUMA_UNUSED_PARAM(pSession);
	DECUMA_UNUSED_PARAM(pDictionaryBuffer);
	DECUMA_UNUSED_PARAM(pSize);

	return decumaFunctionNotSupported;
}

DECUMA_STATUS cjkWordExistsInDictionary(CJK_SESSION * pSession, const void * pDictionaryBuffer, 
												   DECUMA_UNICODE * pWord)
{
	DECUMA_UNUSED_PARAM(pSession);
	DECUMA_UNUSED_PARAM(pDictionaryBuffer);
	DECUMA_UNUSED_PARAM(pWord);

	return decumaFunctionNotSupported;
}

DECUMA_STATUS cjkAddAllWords(CJK_SESSION * pSession,
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

/*///////////////////////////////////////////////////////////////////////////////////////// */




/******* Functions in decuma_hwr_extra.h *******//*//////////////// */
#ifdef DECUMA_HWR_ENABLE_EXTRA_API

/******* Word completion functions *******/

DECUMA_STATUS cjkCompleteWord(CJK_SESSION * pSession,
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


DECUMA_STATUS cjkSetForcedSegmentation( CJK_SESSION * pSession,
	DECUMA_INT16 * pForceSymbolStrokes,
	DECUMA_INT16 nForceSymbols)
{
	DECUMA_UNUSED_PARAM(pSession);
	DECUMA_UNUSED_PARAM(pForceSymbolStrokes);
	DECUMA_UNUSED_PARAM(nForceSymbols);

	return decumaFunctionNotSupported;
}

const DECUMA_INT16 * cjkGetForcedSegmentation(CJK_SESSION * pSession, DECUMA_INT16 * pnForceSymbols)
{
	DECUMA_UNUSED_PARAM(pSession);
	DECUMA_UNUSED_PARAM(pnForceSymbols);

	return NULL;
}


DECUMA_STATUS cjkSetDictionaryFilterString(CJK_SESSION * pSession, 
											 const DECUMA_UNICODE * symbolString)
{
	DECUMA_UNUSED_PARAM(pSession);
	DECUMA_UNUSED_PARAM(symbolString);

	return decumaFunctionNotSupported;
}

DECUMA_UNICODE * cjkGetDictionaryFilterString(CJK_SESSION * pSession)
{
	DECUMA_UNUSED_PARAM(pSession);

	return NULL;
}

DECUMA_STATUS cjkConfirmArcs( CJK_SESSION * pSession)
{
	DECUMA_UNUSED_PARAM(pSession);

	return decumaFunctionNotSupported;
}

DECUMA_STATUS cjkEvaluateArcs( CJK_SESSION * pSession)
{
	DECUMA_UNUSED_PARAM(pSession);

	return decumaFunctionNotSupported;
}

DECUMA_STATUS cjkRecognizeExtraOutput( CJK_SESSION * pSession,
	DECUMA_HWR_RESULT * pResults,
	DECUMA_UINT16 nMaxResults,
	DECUMA_UINT16 * pnResults,
	DECUMA_UINT16 nMaxCharsPerResult,
	void *  pCharArcIdxData, /*Should point to an array of size nMaxResults * nMaxCharsPerResult * */
	const DECUMA_RECOGNITION_SETTINGS * pRecognitionSettings,
	const DECUMA_INTERRUPT_FUNCTIONS * pInterruptFunctions)
{
	DECUMA_UNUSED_PARAM(pSession);
	DECUMA_UNUSED_PARAM(pResults);
	DECUMA_UNUSED_PARAM(nMaxResults);
	DECUMA_UNUSED_PARAM(pnResults);
	DECUMA_UNUSED_PARAM(nMaxCharsPerResult);
	DECUMA_UNUSED_PARAM(pCharArcIdxData);
	DECUMA_UNUSED_PARAM(pRecognitionSettings);
	DECUMA_UNUSED_PARAM(pInterruptFunctions);

	return decumaFunctionNotSupported;
}

int cjkGetCharArcIdxDataSize( DECUMA_UINT16 nMaxResults, 
							 DECUMA_UINT16 nMaxCharsPerResult)
{
	DECUMA_UNUSED_PARAM(nMaxResults);
	DECUMA_UNUSED_PARAM(nMaxCharsPerResult);

	return decumaFunctionNotSupported;
}

const DECUMA_UINT16 * cjkGetCharArcIdxs( const void * pCharArcIdxData,
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

long cjkGetMemoryStatistics(char *pBuf,
							long nBufLen)
{
	DECUMA_UNUSED_PARAM(pBuf);
	DECUMA_UNUSED_PARAM(nBufLen);

	return decumaFunctionNotSupported;
}

DECUMA_STATUS cjkUsePositionAndScaleInvariantRecognition( CJK_SESSION * pSession, int bOnOff)
{
	DECUMA_UNUSED_PARAM(pSession);
	if (bOnOff == 0) return decumaNoError;

	return decumaFunctionNotSupported;
}

DECUMA_STATUS cjkUseTypeConnectionRequirement( CJK_SESSION * pSession, int bOnOff)
{
	DECUMA_UNUSED_PARAM(pSession);
	if (bOnOff == 1) return decumaNoError;

	return decumaFunctionNotSupported;
}

DECUMA_STATUS cjkUseStrictTypeConnectionRequirement( CJK_SESSION * pSession, int bOnOff)
{
	DECUMA_UNUSED_PARAM(pSession);
	if (bOnOff == 0) return decumaNoError;

	return decumaFunctionNotSupported;
}

int cjkGetArcSessionSegmentCount(CJK_SESSION * pSession)
{
	DECUMA_UNUSED_PARAM(pSession);

	return decumaFunctionNotSupported;
}

DECUMA_UINT32 cjkGetMaxNumberOfSegments(void)
{
	return decumaFunctionNotSupported;
}

DECUMA_UINT32 cjkGetArcSessionSize(void)
{
	return decumaFunctionNotSupported;
}

DECUMA_UINT32 cjkGetSGSize(void)
{
	return decumaFunctionNotSupported;
}

DECUMA_UINT32 cjkGetRGSize(void)
{
	return decumaFunctionNotSupported;
}

#endif /* DECUMA_HWR_ENABLE_EXTRA_API */

/* *************************************************************************
 * Internal functions
 * ************************************************************************* */

static DECUMA_STATUS cjkSetPersonalCategoryBitArray(CJK_SESSION * pSession, const DECUMA_SESSION_SETTINGS * pSessionSettings)
{
	DLTDB_INDEX i;
	DECUMA_UNICODE charExt;
	int n = 0, bFound = 0;

	decumaAssert(pSession);
	decumaAssert(pSessionSettings);
	decumaAssert(pSessionSettings->pCharSetExtension);
	decumaAssert(pSession->pPersonalCategories);

	charExt = pSessionSettings->pCharSetExtension[0];
	if (pSession->nPersonalCategories == 0) return decumaNoError;

	decumaMemset(pSession->pPersonalCategories, 0, MAX_NBR_DB_IDX_BIT_CHARS);
	for (i = 0; i <= pSession->db.maxindex; i++) {
		n = 0;
		while (pSessionSettings->pCharSetExtension[n] != 0) {
			/* Skip multicode symbols */
			if (pSessionSettings->pCharSetExtension[n+1] != 0) {
				/* Loop until end of multicode symbol */
				while (pSessionSettings->pCharSetExtension[n] != 0) {
					n++;					
				}
				/* Go to start of next symbol */
				n++;
				continue;
			}
			if (cjkDbUnicode(i, pSession) == pSessionSettings->pCharSetExtension[n]) {
				DLTDB_SET_BIT(pSession->pPersonalCategories, i);
				bFound = 1;
			}
			/* Go to start of next symbol */
			n += 2;
		}
	}
	if (!bFound) {
		return decumaSymbolNotInDatabase;
	}
	return decumaNoError;
}

static DECUMA_STATUS dltConvertLanguage(DECUMA_UINT32 lang, DECUMA_UINT32 * mask)
{
	decumaAssert(mask);

	switch ( lang )
	{
		case DECUMA_LANG_GSMDEFAULT:
		case DECUMA_LANG_EN:
		case DECUMA_LANG_ENPRC:
		case DECUMA_LANG_ENHK:
		case DECUMA_LANG_ENTW:
			*mask |= (CJK_LATIN_LOWER | CJK_LATIN_UPPER | CJK_DIGIT | CJK_PUNCTUATION | CJK_SYMBOL | CJK_GESTURE);
			*mask |= (CJK_LATIN_LOWER_SINGLE_STROKE | CJK_LATIN_UPPER_SINGLE_STROKE | CJK_DIGIT_SINGLE_STROKE);
			*mask |= (CJK_PUNCTUATION_SINGLE_STROKE | CJK_SYMBOL_SINGLE_STROKE);
			break;
		case DECUMA_LANG_PRC:
			*mask |= (CJK_GB2312 | CJK_POPULARFORM | CJK_TRADSIMPDUAL);
			break;
		case DECUMA_LANG_TW:
			*mask |= (CJK_BIGFIVE | CJK_BOPOMOFO | CJK_POPULARFORM | CJK_TRADSIMPDUAL);
			break;
		case DECUMA_LANG_HK:
			*mask |= (CJK_BIGFIVE | CJK_BOPOMOFO | CJK_HKSCS | CJK_POPULARFORM | CJK_TRADSIMPDUAL);
			break;
		case DECUMA_LANG_JP:
			*mask |= (CJK_JIS_LEVEL_1 | CJK_JIS_LEVEL_2 | CJK_JIS_LEVEL_3 | CJK_JIS_LEVEL_4 | CJK_HIRAGANA | CJK_KATAKANA | CJK_HIRAGANASMALL | CJK_KATAKANASMALL);
			break;
		case DECUMA_LANG_KO:
			*mask |= CJK_HANGUL;
			break;
		default:
			return decumaUnsupportedLanguage;
	}

	return decumaNoError;
}

static DECUMA_STATUS dltConvertCategory(DECUMA_UINT32 cat, DECUMA_UINT32 * mask)
{
	decumaAssert(mask);

	switch ( cat )
	{
	case DECUMA_CATEGORY_ANSI:
		*mask |= CJK_LATIN_LOWER;
		*mask |= CJK_LATIN_UPPER;
		break;
	case DECUMA_CATEGORY_LC_ANSI:
		*mask |= CJK_LATIN_LOWER;
		break;
	case DECUMA_CATEGORY_UC_ANSI:
		*mask |= CJK_LATIN_UPPER;
		break;
	case DECUMA_CATEGORY_DIGIT:
		*mask |= CJK_DIGIT;
		break;
	case DECUMA_CATEGORY_PUNCTUATIONS:
		*mask |= CJK_PUNCTUATION;
		break;
	case DECUMA_CATEGORY_GESTURES:
		*mask |= CJK_GESTURE;
		break;
	case DECUMA_CATEGORY_CJK_SYMBOL:
		*mask |= CJK_SYMBOL;
		break;
	case DECUMA_CATEGORY_GB2312_A:
		*mask |= CJK_GB2312_A;
		break;
	case DECUMA_CATEGORY_GB2312_B_RADICALS:
		*mask |= CJK_GB2312_B_RADICALS;
		break;		
	case DECUMA_CATEGORY_GB2312_B_CHARS_ONLY:
		*mask |= CJK_GB2312_B_CHARS;
		break;
	case DECUMA_CATEGORY_BIGFIVE_LEVEL_1:
		*mask |= CJK_BIGFIVE_LEVEL_1;
		break;
	case DECUMA_CATEGORY_BIGFIVE_LEVEL_2:
		*mask |= CJK_BIGFIVE_LEVEL_2;
		break;
	case DECUMA_CATEGORY_JIS_LEVEL_1:
		*mask |= CJK_JIS_LEVEL_1;
		break;
	case DECUMA_CATEGORY_JIS_LEVEL_2:
		*mask |= CJK_JIS_LEVEL_2;
		break;
	case DECUMA_CATEGORY_JIS_LEVEL_3:
		*mask |= CJK_JIS_LEVEL_3;
		break;		
	case DECUMA_CATEGORY_JIS_LEVEL_4:
		*mask |= CJK_JIS_LEVEL_4;
		break;				
	case DECUMA_CATEGORY_HIRAGANA:
		*mask |= CJK_HIRAGANA;
		break;
	case DECUMA_CATEGORY_KATAKANA:
		*mask |= CJK_KATAKANA;
		break;
	case DECUMA_CATEGORY_HIRAGANASMALL:
		*mask |= CJK_HIRAGANASMALL;
		break;
	case DECUMA_CATEGORY_KATAKANASMALL:
		*mask |= CJK_KATAKANASMALL;
		break;
	case DECUMA_CATEGORY_BOPOMOFO:
		*mask |= CJK_BOPOMOFO;
		break;
	case DECUMA_CATEGORY_HKSCS_2001:
		*mask |= CJK_HKSCS;
		break;
	case DECUMA_CATEGORY_HANGUL_1001_A:
		*mask |= CJK_HANGUL_A;
		break;
	case DECUMA_CATEGORY_HANGUL_1001_B:
		*mask |= CJK_HANGUL_B;
		break;
	case DECUMA_CATEGORY_LC_SINGLE_STROKE:
		*mask |= CJK_LATIN_LOWER_SINGLE_STROKE;
		break;
	case DECUMA_CATEGORY_UC_SINGLE_STROKE:
		*mask |= CJK_LATIN_UPPER_SINGLE_STROKE;
		break;
	case DECUMA_CATEGORY_DIGIT_SINGLE_STROKE:
		*mask |= CJK_DIGIT_SINGLE_STROKE;
		break;		
	case DECUMA_CATEGORY_PUNCTUATION_SINGLE_STROKE:
		*mask |= CJK_PUNCTUATION_SINGLE_STROKE;
		break;
	case DECUMA_CATEGORY_CJK_SYMBOL_SINGLE_STROKE:
		*mask |= CJK_SYMBOL_SINGLE_STROKE;
		break;		
/* And some categories effectively function as writing-styles */			
	case DECUMA_CATEGORY_POPULAR_FORM:
		*mask |= CJK_POPULARFORM;
		break;
	case DECUMA_CATEGORY_SIMPLIFIED_FORM:
		*mask |= CJK_TRADSIMPDUAL;
		break;
	case DECUMA_CATEGORY_TRADITIONAL_FORM:
		*mask |= CJK_TRADSIMPDUAL;
		break;		
/* And some combination categories */
	case DECUMA_CATEGORY_HANGUL:
		*mask |= CJK_HANGUL_A;
		*mask |= CJK_HANGUL_B;
		break;
	case DECUMA_CATEGORY_BIGFIVE:
		*mask |= CJK_BIGFIVE_LEVEL_1;
		*mask |= CJK_BIGFIVE_LEVEL_2;
		break;
	case DECUMA_CATEGORY_JIS:
		*mask |= CJK_JIS_LEVEL_1;
		*mask |= CJK_JIS_LEVEL_2;
		*mask |= CJK_JIS_LEVEL_3;
		*mask |= CJK_JIS_LEVEL_4;
		break;
	case DECUMA_CATEGORY_GB2312_B:
		*mask |= CJK_GB2312_B;
		break;		
	case DECUMA_CATEGORY_GB2312:
		*mask |= CJK_GB2312;
		break;
	case DECUMA_CATEGORY_HAN:
		*mask |= CJK_GB2312;
		*mask |= CJK_JIS_LEVEL_1;
		*mask |= CJK_JIS_LEVEL_2;
		*mask |= CJK_JIS_LEVEL_3;
		*mask |= CJK_JIS_LEVEL_4;
		*mask |= CJK_BIGFIVE_LEVEL_1;
		*mask |= CJK_BIGFIVE_LEVEL_2;
		*mask |= CJK_HKSCS;		
		break;		
				
	default:
		return decumaUnsupportedSymbolCategory;
	}
	return decumaNoError;
}
