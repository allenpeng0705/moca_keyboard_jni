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

#include <stddef.h> /* Definition of NULL */

#define T9WL_C

#include "t9wlConfig.h"

#include "t9wl.h"

#include "decumaAssert.h"
#include "decumaMemory.h"
#include "decumaString.h"
#include "decumaCommon.h"
#include "decumaCategoryTranslation.h"
#include "decumaCategoryTableType.h"
#include "decumaResamp.h"
#include "decumaString.h"
#include "t9wlData.h"

#ifdef DECUMA_DEBUG_MEMORY
#include "decumaDebugMemory.h"
#endif

#include "decumaCategories.h"

#include "t9wlVersion.h"
#include "scrAPI.h"  /*For database functions */
#include "udmlib.h"  /*For dynamic database functions */
#include "decumaSymbolCategories.h" /* for DECUMA_CATEGORY_RESERVED_END */
#include "decumaLanguages.h" /* for DECUMA_LANG_RESERVED_END */
#include "decumaStorageSpecifiers.h" /* Definition of DECUMA_SPECIFIC_DB_STORAGE */

/*#define DEBUG_OUTPUT */
#ifdef DEBUG_OUTPUT
#include <stdio.h>
FILE * g_debugFile;
#endif

/*Note the header file t9wlHiddenData.h includes datatypes that are used by */
/*the hidden api but that are not ifdeffed because that would complicate */
/*the code more and need a lot of more ifdefs in the code */
/*#include "t9wlHiddenData.h" */

#ifdef T9WL_HIDDEN_API
#include "t9wlHiddenApi.h"
#endif


/* */
/* Private macro definitions */
/* */

#define MAX_PROXIMITY 1000

#define NUM_DICTIONARIES	2	/* Two dictionaries right now; one static, one PD */
#define STATIC_DICTIONARY	0	/* index of the static dict */
#define PERSONAL_DICTIONARY	1	/* index of the personal dict */

#define SESSION_NOT_INITIALIZED(pSession) ((pSession)->pArcSession != (T9WL_ARC_SESSION *)&(pSession)[1])
#define USE_TERSE_TRIE

/* */
/* Private function declarations */
/* */

/*Check if all symbols are included in the database */
static DECUMA_STATUS allSymbolsInDatabase(DECUMA_UNICODE * pCharSetExt,
							DECUMA_STATIC_DB_PTR pStaticDB,
							int * pbIncluded);


/* Set a pointer to DECUMA_SESSION_SETTINGS struct. */
static DECUMA_STATUS setSessionSettings(T9WL_SESSION * pSession,
										const DECUMA_SESSION_SETTINGS * pSessionSettings);

static DECUMA_STATUS getStringType(T9WL_SESSION * pSession,
								   DECUMA_UNICODE * pChars, int nChars,
								   DECUMA_STRING_TYPE *pStringType,
								   DECUMA_INT16 * pDictStrStart,
								   DECUMA_INT16 * pDictStrLen);

static DECUMA_UINT32	getMeanDistance(T9WL_SESSION * pSession, DECUMA_UINT32 nTotalDist, int nNodes);

static DECUMA_STATUS getT9WLSessionStatus(const T9WL_SESSION * pSession);

static DECUMA_STATUS attachDictionary(T9WL_SESSION * pSession,
									  const void * pDictionaryData);

/*///////////////////////////////////////////////////////////////////////////////////////// */
/* */
/* Public function definitions */
/* */

/****** Version ********/

const char * t9wlGetVersion(void)
{
	return T9WLLIB_VERSION_STR;
}


/****** Initialization and destruction ********/

DECUMA_UINT32 t9wlGetSessionSize(void)
{
	return sizeof(T9WL_SESSION) + scrGetMemBufSize();
}



DECUMA_STATUS t9wlBeginSession(T9WL_SESSION * pSession,
	const DECUMA_SESSION_SETTINGS * pSessionSettings,
	const DECUMA_MEM_FUNCTIONS * pMemFunctions)
{
	DECUMA_STATUS status;
	int i;

	decumaMemset(pSession,0,t9wlGetSessionSize());

	pSession->pScrMem = &pSession[1];

	decumaAssert(sizeof(pSession->arcsBuf) / sizeof(pSession->arcsBuf[0]) == sizeof(pSession->charCurve.pArc) / sizeof(pSession->charCurve.pArc[0]));

	for (i = 0; i < sizeof(pSession->arcsBuf) / sizeof(pSession->arcsBuf[0]); i++) pSession->charCurve.pArc[i] = &pSession->arcsBuf[i];

	pSession->scr_api_settings.nRefAngleImportance = FULL_ROTATION_IMPORTANCE;

	status = setSessionSettings(pSession,pSessionSettings);

	decumaAssert(status == decumaNoError);

	return status;
}

DECUMA_STATUS t9wlVerifySession(const T9WL_SESSION * pSession)
{
	DECUMA_STATUS status = getT9WLSessionStatus(pSession);

	return status;
}

DECUMA_STATUS t9wlEndSession(T9WL_SESSION * pSession)
{
	return decumaNoError;
}




/******* Database functions *********/

/******* Internal *******************/
DECUMA_STATUS t9wlValidateStaticDatabase(DECUMA_STATIC_DB_PTR pStaticDB)
{
	return _scrCheckStaticDB( (STATIC_DB_PTR) pStaticDB);
}

DECUMA_STATUS t9wlValidateDynamicDatabase(DECUMA_DYNAMIC_DB_PTR pDynamicDB)
{
	if (!udmIsValid( (UDM_PTR) pDynamicDB))
		return decumaInvalidDatabase;

	return decumaNoError;
}

/******* Exported *********************/

DECUMA_STATUS t9wlDatabaseGetVersion(DECUMA_ANY_DB_PTR pDB, char * pBuf, int nBufLen)
{
	decumaAssert(pDB);
	decumaAssert(pBuf );
	decumaAssert(nBufLen >= DECUMA_DATABASE_VERSION_STRING_LENGTH);

	if (t9wlValidateStaticDatabase(pDB) == decumaNoError)
		;
	else if (t9wlValidateDynamicDatabase(pDB) == decumaNoError)
		;
	else
		return decumaInvalidDatabase;

	decumaAssert(nBufLen >= 19);
	decumaMemcpy (pBuf, "Unknown DB version", 19);

	return decumaNoError;
}


DECUMA_STATUS t9wlDatabaseIsCategorySupported(DECUMA_ANY_DB_PTR pDB,
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

	if (t9wlValidateStaticDatabase(pDB) == decumaNoError)
		;
	else if (t9wlValidateDynamicDatabase(pDB) == decumaNoError)
		;
	else
		return decumaInvalidDatabase;

	*pbIsSupported = _scrDatabaseSupportsSymbolCategory(pDB, cat);

	return decumaNoError;
}


DECUMA_STATUS t9wlDatabaseIsLanguageSupported(DECUMA_ANY_DB_PTR pDB,
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

	if (t9wlValidateStaticDatabase(pDB) == decumaNoError)
		;
	else if (t9wlValidateDynamicDatabase(pDB) == decumaNoError)
		;
	else
		return decumaInvalidDatabase;

	*pbIsSupported = _scrDatabaseSupportsLanguage(pDB, lang);

	return decumaNoError;
}


DECUMA_STATUS t9wlDatabaseIncludesSymbol(
	DECUMA_ANY_DB_PTR pDB,
	const DECUMA_CHARACTER_SET * pCharSet,
	const DECUMA_UNICODE * pSymbol,
	int * pbIsIncluded)
{
	DECUMA_STATUS status;

	decumaAssert(pSymbol);

	if (t9wlValidateStaticDatabase(pDB) == decumaNoError)
		;
	else if (t9wlValidateDynamicDatabase(pDB) == decumaNoError)
		;
	else
		return decumaInvalidDatabase;

	status = _scrDatabaseIncludesSymbol(
		pDB,
		pCharSet,
		pSymbol,
		pbIsIncluded);

	return status;

}

/************ Dynamic Database Functions ******************/

DECUMA_STATUS t9wlDynamicDatabaseIsValid(DECUMA_DYNAMIC_DB_PTR pDynamicDB) {
	return t9wlValidateDynamicDatabase(pDynamicDB);
}

/* Get actual number of bytes used. All template data is stored contiguously, so this function can be used to serialize the database to some persistent storage. */
DECUMA_STATUS t9wlGetDynamicDatabaseByteSize(DECUMA_DYNAMIC_DB_PTR pDynamicDB, DECUMA_UINT32 * pSize)
{
	DECUMA_STATUS status;

	decumaAssert(pSize);

	status = t9wlValidateDynamicDatabase(pDynamicDB);
	if (status != decumaNoError) return status;

	*pSize = udmGetByteSize( (UDM_PTR) pDynamicDB);

	return decumaNoError;
}

#if !defined(DECUMA_SPECIFIC_DB_STORAGE) || !(DECUMA_SPECIFIC_DB_STORAGE == 1 || DECUMA_SPECIFIC_DB_STORAGE == 0)
#error DECUMA_SPECIFIC_DB_STORAGE must be defined to 1 or 0
#endif
#if DECUMA_SPECIFIC_DB_STORAGE == 0 /* Dynamic DB generation implementation cannot support specific DB storage */
DECUMA_STATUS t9wlCreateDynamicDatabase(DECUMA_DYNAMIC_DB_PTR* ppDynamicDB, const DECUMA_MEM_FUNCTIONS * pMemFunctions)
{
	DECUMA_STATUS status;

	if (ppDynamicDB == NULL) return decumaNullPointer;

	*ppDynamicDB = (DECUMA_DYNAMIC_DB_PTR) udmCreateDynamic(pMemFunctions);
	if (*ppDynamicDB != NULL) {
		status = t9wlValidateDynamicDatabase(*ppDynamicDB);
		if (status != decumaNoError) {
			goto t9wlCreateDynamicDatabase_exit;
		}
	}
	else {
		return decumaAllocationFailed;
	}

	return decumaNoError;

t9wlCreateDynamicDatabase_exit:
	udmDestroyDynamic((UDM_PTR)*ppDynamicDB, pMemFunctions);
	return status;
}

/* Add allograph to the dynamic database, with the specified character set.
 * Since the dltlib compression routine handles scaling/translation, we can add an unbounded curve (i.e. no need to pass constraints).
 */
DECUMA_STATUS t9wlAddAllograph(      DECUMA_DYNAMIC_DB_PTR  * ppDynamicDB,
								 const DECUMA_CURVE         * pCurve,
								 const DECUMA_UNICODE       * pUnicode, DECUMA_UINT32 nUnicodes,
								 const DECUMA_CHARACTER_SET * pCharacterSet,
								 DECUMA_INT32 nBaseline, DECUMA_INT32 nHelpline,
								 int bGesture, int bInstantGesture,
								 const DECUMA_MEM_FUNCTIONS * pMemFunctions)
{
	DECUMA_STATUS status;

	status = t9wlValidateDynamicDatabase(*ppDynamicDB);
	if (status != decumaNoError) return status;

	return udmAddAllograph((UDM_PTR*) ppDynamicDB, pCurve, pUnicode, (int) nUnicodes, pCharacterSet,
			0 /* nSymbolType */, nBaseline, nHelpline, bGesture, bInstantGesture, pMemFunctions);
}

DECUMA_STATUS t9wlDestroyDynamicDatabase(DECUMA_DYNAMIC_DB_PTR* ppDynamicDB, const DECUMA_MEM_FUNCTIONS * pMemFunctions)
{
	if (*ppDynamicDB != NULL) udmDestroyDynamic((UDM_PTR)*ppDynamicDB, pMemFunctions);
	*ppDynamicDB=NULL;
	return decumaNoError;
}
#endif /* DECUMA_SPECIFIC_DB_STORAGE*/


/******* Settings *********/


DECUMA_STATUS t9wlChangeSessionSettings( T9WL_SESSION * pSession,
	const DECUMA_SESSION_SETTINGS * pSessionSettings)
{
	DECUMA_STATUS status;

	decumaAssert(getT9WLSessionStatus(pSession) == decumaNoError); /*Should have been checked already */

	/* If validateSessionSettings succeeds, it should not be possible for setSessionSettings to fail */
	status = setSessionSettings(pSession, pSessionSettings);
	if (status != decumaNoError)
	{
		decumaAssert( status == decumaAllocationFailed);
	}

	return status;
}

DECUMA_STATUS t9wlValidateSessionSettings(
	const DECUMA_SESSION_SETTINGS * pSessionSettings)
{
	DECUMA_STATUS status;
	const DECUMA_CHARACTER_SET * pCharSet = &pSessionSettings->charSet;
	SCR_API_SETTINGS scr_api_settings;

	/*t9wl specific validation of session settings. */
	/*General validation is performed in outer layer. */

	decumaMemset(&scr_api_settings, 0, sizeof(scr_api_settings));

	scr_api_settings.characterSet = pSessionSettings->charSet;
	scr_api_settings.nRefAngleImportance = FULL_ROTATION_IMPORTANCE;
	scr_api_settings.nBaseLineY = pSessionSettings->baseline;
	scr_api_settings.nHelpLineY = pSessionSettings->helpline;
	scr_api_settings.pStaticDB = (STATIC_DB_PTR)pSessionSettings->pStaticDB;
	scr_api_settings.pUDM = (UDM_PTR)pSessionSettings->pDynamicDB;

	status = scrCheckSettings(&scr_api_settings);

	if (status != decumaNoError) return status;

	if (pSessionSettings->pCharSetExtension && pSessionSettings->pCharSetExtension[0]) return decumaUnsupportedParameterValue;

	if (pSessionSettings->recognitionMode != scrMode) return decumaUnsupportedRecognitionMode;

	if (pSessionSettings->writingDirection > 0) return decumaUnsupportedWritingDirection;

	switch (pSessionSettings->UIInputGuide)
	{
	case supportlines:
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
		break;
	case none:
		break;
	default: /* e.g. box */
		return decumaInvalidUIInputGuide;
		break;
	}

	return decumaNoError;
}



/****** Recognition functions ********/


DECUMA_STATUS t9wlBeginArcAddition(T9WL_SESSION * pSession)
{
	decumaAssert(getT9WLSessionStatus(pSession) == decumaNoError); /*Should have been checked already */
	decumaAssert(pSession->charCurve.nArcs == 0);

	pSession->nPrevBaselinePos = pSession->baselinePos;
	pSession->nPrevDistBaseToHelpLine = pSession->distBaseToHelpLine;
	pSession->bPrevReferenceEstimated = pSession->bReferenceEstimated;

	pSession->nResults = 0;

	pSession->bAddingArcs = 1;

	return decumaNoError;
}


/*Writes an ID in pSamplingArcID which can be used for further reference to the arc */
DECUMA_STATUS t9wlStartNewArc(T9WL_SESSION * pSession, DECUMA_UINT32 arcID)
{
	int i;

	decumaAssert(getT9WLSessionStatus(pSession) == decumaNoError); /*Should have been checked already */
	decumaAssert(pSession->bAddingArcs);

	if (pSession->nUncommittedArcs + pSession->charCurve.nArcs >= MAX_DECUMA_UINT8) return decumaInputBufferFull; /* For some consistency with other engines */

	if (pSession->nUncommittedArcs == sizeof(pSession->arcIDs) / sizeof(pSession->arcIDs[0])) return decumaTooManyConcurrentArcs;

	for (i = 0; i < pSession->nUncommittedArcs; i++)
		if (arcID == pSession->arcIDs[i]) return decumaInvalidArcID;

	decumaAssert(pSession->nUncommittedArcs < sizeof(pSession->arcIDs) / sizeof(pSession->arcIDs[0]));

	pSession->nUncommittedPoints[pSession->nUncommittedArcs] = 0;
	pSession->arcIDs[pSession->nUncommittedArcs] = arcID;

	pSession->nUncommittedArcs++;

	return decumaNoError;
}

DECUMA_STATUS t9wlStartNewSymbol(T9WL_SESSION * pSession)
{
	return decumaFunctionNotSupported;
}

/*Adds point(s) to a started arc */
DECUMA_STATUS t9wlAddPoint(T9WL_SESSION * pSession, DECUMA_COORD x, DECUMA_COORD y, DECUMA_UINT32 arcID)
{
	int i;

	decumaAssert(getT9WLSessionStatus(pSession) == decumaNoError); /*Should have been checked already */
	decumaAssert(pSession->bAddingArcs);
	decumaAssert(pSession->nUncommittedArcs >= 0);
	decumaAssert(pSession->nUncommittedArcs <= sizeof(pSession->pointsBuf) / sizeof(pSession->pointsBuf[0]));

	for (i = 0; i < pSession->nUncommittedArcs; i++)
		if (arcID == pSession->arcIDs[i]) break;

	if (i >= pSession->nUncommittedArcs) return decumaInvalidArcID;

	decumaAssert(pSession->nUncommittedPoints[i] >= 0);
	decumaAssert(pSession->nUncommittedPoints[i] <= sizeof(pSession->pointsBuf[i]) / sizeof(pSession->pointsBuf[i][0]));

	if (pSession->nUncommittedPoints[i] > 0 && 
		x == pSession->pointsBuf[i][pSession->nUncommittedPoints[i]-1].x &&
		y == pSession->pointsBuf[i][pSession->nUncommittedPoints[i]-1].y)
		return decumaNoError; /* No need to add point identical to previous point */

	if (pSession->nUncommittedPoints[i] > 1)
	{
		DECUMA_INT32 prev_dx, prev_dy, new_dx, new_dy;

		new_dx = x - pSession->pointsBuf[i][pSession->nUncommittedPoints[i]-1].x;
		new_dy = y - pSession->pointsBuf[i][pSession->nUncommittedPoints[i]-1].y;

		prev_dx = pSession->pointsBuf[i][pSession->nUncommittedPoints[i]-1].x - pSession->pointsBuf[i][pSession->nUncommittedPoints[i]-2].x;
		prev_dy = pSession->pointsBuf[i][pSession->nUncommittedPoints[i]-1].y - pSession->pointsBuf[i][pSession->nUncommittedPoints[i]-2].y;

		if (new_dx == 0 && prev_dx == 0 && new_dy * prev_dy > 0 || /* Same vertical line */
			new_dy == 0 && prev_dy == 0 && new_dx * prev_dx > 0 || /* Same horisontal line */
			new_dx * prev_dy == prev_dx * new_dy && /* Same angle */
			new_dx * prev_dx > 0 && new_dy * prev_dy > 0 /* Same quadrant */)
		{
			/* No need to keep previous point on same line as new point */
			pSession->pointsBuf[i][pSession->nUncommittedPoints[i]-1].x = x;
			pSession->pointsBuf[i][pSession->nUncommittedPoints[i]-1].y = y;

			return decumaNoError;
		}
	}

	if (pSession->charCurve.nArcs < sizeof(pSession->charCurve.pArc) / sizeof(pSession->charCurve.pArc[0]))
	{
		if (pSession->nUncommittedPoints[i] == sizeof(pSession->pointsBuf[i]) / sizeof(pSession->pointsBuf[i][0]))
		{
			/* Buffer full. Resample to free some of it up to add more points. */

			DECUMA_STATUS status = resampArc(
				pSession->pointsBuf[i],
				pSession->nUncommittedPoints[i],
				pSession->charCurve.pArc[pSession->charCurve.nArcs]->point,
				sizeof(pSession->charCurve.pArc[pSession->charCurve.nArcs]->point) / sizeof(pSession->charCurve.pArc[pSession->charCurve.nArcs]->point[0]),0,0);

			decumaAssert(status == decumaNoError);

			pSession->nUncommittedPoints[i] = sizeof(pSession->charCurve.pArc[pSession->charCurve.nArcs]->point) / sizeof(pSession->charCurve.pArc[pSession->charCurve.nArcs]->point[0]);
			decumaMemcpy(pSession->pointsBuf[i], pSession->charCurve.pArc[pSession->charCurve.nArcs]->point, pSession->nUncommittedPoints[i] * sizeof(pSession->pointsBuf[i][0]));
		}

		decumaAssert(pSession->nUncommittedPoints[i] < sizeof(pSession->pointsBuf[i]) / sizeof(pSession->pointsBuf[i][0]));

		pSession->pointsBuf[i][pSession->nUncommittedPoints[i]].x = x;
		pSession->pointsBuf[i][pSession->nUncommittedPoints[i]].y = y;
		pSession->nUncommittedPoints[i]++;
	}
	/* Else we cannot fit more committed arcs, so no point in storing the point. Just increment point counter until limit is reached. */
	else if (pSession->nUncommittedPoints[i] < sizeof(pSession->pointsBuf[i]) / sizeof(pSession->pointsBuf[i][0]))
		pSession->nUncommittedPoints[i]++;

	return decumaNoError;
}

DECUMA_STATUS t9wlAddPoints(T9WL_SESSION * pSession,
							  DECUMA_POINT * pPts, int nPts,
							  DECUMA_UINT32 arcID,
							  int * pnPtsAdded)
{
	DECUMA_STATUS status = decumaNoError;
	int i;

	decumaAssert(getT9WLSessionStatus(pSession) == decumaNoError); /*Should have been checked already */

	*pnPtsAdded = 0;

	for (i = 0; i < nPts; i++, (*pnPtsAdded)++)
		if ((status = t9wlAddPoint(pSession, pPts[i].x, pPts[i].y, arcID)) != decumaNoError) break;

	return status;
}


/*Ends the arc and removes it from the session */
DECUMA_STATUS t9wlCancelArc(T9WL_SESSION * pSession,DECUMA_UINT32 arcID)
{
	int i;

	decumaAssert(getT9WLSessionStatus(pSession) == decumaNoError); /*Should have been checked already */
	decumaAssert(pSession->bAddingArcs);
	decumaAssert(pSession->nUncommittedArcs >= 0);
	decumaAssert(pSession->nUncommittedArcs <= sizeof(pSession->pointsBuf) / sizeof(pSession->pointsBuf[0]));

	for (i = 0; i < pSession->nUncommittedArcs; i++)
		if (arcID == pSession->arcIDs[i]) break;

	if (i >= pSession->nUncommittedArcs) return decumaInvalidArcID;

	decumaAssert(pSession->nUncommittedPoints[i] >= 0);
	decumaAssert(pSession->nUncommittedPoints[i] <= sizeof(pSession->pointsBuf[i]) / sizeof(pSession->pointsBuf[i][0]));

	if (i < pSession->nUncommittedArcs - 1)
	{
		decumaMemmove(&pSession->nUncommittedPoints[i], &pSession->nUncommittedPoints[i+1], (pSession->nUncommittedArcs - i - 1) * sizeof(pSession->nUncommittedPoints[i]));
		decumaMemmove(&pSession->arcIDs[i], &pSession->arcIDs[i+1], (pSession->nUncommittedArcs - i - 1) * sizeof(pSession->arcIDs[i]));
	}

	pSession->nUncommittedArcs--;

	return decumaNoError;
}

/*Ends and adds the arc to the arc-addition-sequence which will be used in a future recognize call */
DECUMA_STATUS t9wlCommitArc(T9WL_SESSION * pSession,DECUMA_UINT32 arcID)
{
	DECUMA_STATUS status = decumaNoError;

	int i;

	decumaAssert(getT9WLSessionStatus(pSession) == decumaNoError); /*Should have been checked already */
	decumaAssert(pSession->bAddingArcs);
	decumaAssert(pSession->nUncommittedArcs >= 0);
	decumaAssert(pSession->nUncommittedArcs <= sizeof(pSession->pointsBuf) / sizeof(pSession->pointsBuf[0]));

	for (i = 0; i < pSession->nUncommittedArcs; i++)
		if (arcID == pSession->arcIDs[i]) break;

	if (i >= pSession->nUncommittedArcs) return decumaInvalidArcID;

	decumaAssert(pSession->nUncommittedPoints[i] >= 0);
	decumaAssert(pSession->nUncommittedPoints[i] <= sizeof(pSession->pointsBuf[i]) / sizeof(pSession->pointsBuf[i][0]));

	if (pSession->nUncommittedPoints[i] == 0) return decumaArcZeroPoints;

	if (pSession->charCurve.nArcs < sizeof(pSession->charCurve.pArc) / sizeof(pSession->charCurve.pArc[0]))
	{
		resampArc(
			pSession->pointsBuf[i],
			pSession->nUncommittedPoints[i],
			pSession->charCurve.pArc[pSession->charCurve.nArcs]->point,
			sizeof(pSession->charCurve.pArc[pSession->charCurve.nArcs]->point) / sizeof(pSession->charCurve.pArc[pSession->charCurve.nArcs]->point[0]),0,0);

		if (pSession->charCurve.nArcs < sizeof(pSession->charCurve.arcTimelineDiff) / sizeof(pSession->charCurve.arcTimelineDiff[0]))
			pSession->charCurve.arcTimelineDiff[pSession->charCurve.nArcs] = (pSession->nUncommittedArcs == 1);
	}
	/* Else we cannot fit this arc in buffer, but the engine would not recognize this many arcs anyway.
	   Just note that too many arcs have been committed and return no error. */

	pSession->charCurve.nArcs++;

	if (i < pSession->nUncommittedArcs - 1)
	{
		decumaMemmove(&pSession->pointsBuf[i], &pSession->pointsBuf[i+1], (pSession->nUncommittedArcs - i - 1) * sizeof(pSession->pointsBuf[i]));
		decumaMemmove(&pSession->nUncommittedPoints[i], &pSession->nUncommittedPoints[i+1], (pSession->nUncommittedArcs - i - 1) * sizeof(pSession->nUncommittedPoints[i]));
		decumaMemmove(&pSession->arcIDs[i], &pSession->arcIDs[i+1], (pSession->nUncommittedArcs - i - 1) * sizeof(pSession->arcIDs[i]));
	}

	pSession->nUncommittedArcs--;

	return status;
}

DECUMA_STATUS t9wlNoteSelectedCandidate( T9WL_SESSION * pSession, int nResultIdx)
{
	decumaAssert(getT9WLSessionStatus(pSession) == decumaNoError); /*Should have been checked already */
	decumaAssert(nResultIdx >= -1);

	if (!pSession->nResults) return decumaIllegalFunctionCall; /* No results to note exist in current add arc session */

	if (nResultIdx >= pSession->nResults) return decumaInvalidIndex;

	if (pSession->pSessionSettings->UIInputGuide == supportlines &&
		pSession->pSessionSettings->supportLineSet != noSupportlines &&
		!pSession->bPositionAndScaleInvariant)
	{
		/* Estimates should not be used if baseline and helpline are set and in use */

		return decumaNoError;
	}

	pSession->nLastNotedResult = nResultIdx;

	if (nResultIdx == -1 ||
		pSession->nBaseLineEstimates[pSession->nLastNotedResult] == pSession->nHelpLineEstimates[pSession->nLastNotedResult])
	{
		/* Result rejection or selected candidate lacks estimate. Use old estimates. */
		pSession->baselinePos = pSession->nPrevBaselinePos;
		pSession->distBaseToHelpLine = pSession->nPrevDistBaseToHelpLine;
		pSession->bReferenceEstimated = pSession->bPrevReferenceEstimated;

		return decumaNoError;
	}

	/* Store current estimates (if existing) */
	pSession->baselinePos = pSession->nBaseLineEstimates[pSession->nLastNotedResult];
	pSession->distBaseToHelpLine = pSession->nBaseLineEstimates[pSession->nLastNotedResult] - pSession->nHelpLineEstimates[pSession->nLastNotedResult];
	pSession->bReferenceEstimated = 1;

	return decumaNoError; /*Unimportant for t9wllib */
}


DECUMA_STATUS t9wlGetUncommittedArcCount(T9WL_SESSION * pSession, int * pnUncommittedArcs)
{
	decumaAssert(getT9WLSessionStatus(pSession) == decumaNoError); /*Should have been checked already */
	decumaAssert(pnUncommittedArcs);

	*pnUncommittedArcs = pSession->nUncommittedArcs;

	return decumaNoError;
}

DECUMA_STATUS t9wlGetUncommittedArcID(T9WL_SESSION * pSession, int idx,
										DECUMA_UINT32 * pArcID)
{
	decumaAssert(getT9WLSessionStatus(pSession) == decumaNoError); /*Should have been checked already */
	decumaAssert(pArcID);

	if (idx < 0 || idx >= pSession->nUncommittedArcs) return decumaInvalidIndex;

	*pArcID = pSession->arcIDs[idx];

	return decumaNoError;
}

DECUMA_STATUS t9wlRecognizeFast( T9WL_SESSION * pSession,
	DECUMA_HWR_RESULT * pResult,
	DECUMA_UINT16 * pnResults,  /* In most cases == 1, but can be 0 */
	DECUMA_UINT16 nMaxChars,
	const DECUMA_INTERRUPT_FUNCTIONS * pInterruptFunctions)
{
	return decumaFunctionNotSupported;
}


DECUMA_STATUS t9wlValidateRecognitionSettings(const DECUMA_RECOGNITION_SETTINGS * pRecognitionSettings)
{
	/*t9wl specific validation of recognition settings. */
	/*General validation is performed in outer layer. */

	if (pRecognitionSettings->boostLevel) return decumaUnsupportedParameterValue;

	if (pRecognitionSettings->pStringStart && pRecognitionSettings->pStringStart[0]) return decumaUnsupportedParameterValue;

	return decumaNoError;
}

DECUMA_STATUS t9wlRecognize( T9WL_SESSION * pSession,
	DECUMA_HWR_RESULT * pResults,
	DECUMA_UINT16 nMaxResults,
	DECUMA_UINT16 * pnResults,
	DECUMA_UINT16 nMaxCharsPerResult,
	const DECUMA_RECOGNITION_SETTINGS * pRecognitionSettings,
	const DECUMA_INTERRUPT_FUNCTIONS * pInterruptFunctions
	)
{
	int i, nOutputsReturned;
	DECUMA_STATUS status = decumaNoError;	

	decumaAssert(getT9WLSessionStatus(pSession) == decumaNoError); /*Should have been checked already */
	decumaAssert(pnResults);
	decumaAssert(!pInterruptFunctions || pInterruptFunctions->pShouldAbortRecognize);

	*pnResults = 0;

	if (pSession->nUncommittedArcs > 0) return decumaUncommittedArcExists;

	if (pSession->charCurve.nArcs == 0) return decumaNoError;

	if (pSession->charCurve.nArcs > sizeof(pSession->charCurve.pArc) / sizeof(pSession->charCurve.pArc[0])) return decumaNoError; /* More arcs committed than engine supports */

	decumaMemset(pSession->scr_output, 0, sizeof(pSession->scr_output));

	decumaMemset(pSession->nBaseLineEstimates, 0, sizeof(pSession->nBaseLineEstimates));
	decumaMemset(pSession->nHelpLineEstimates, 0, sizeof(pSession->nHelpLineEstimates));

	pSession->scr_api_settings.nBaseLineY = pSession->baselinePos;
	pSession->scr_api_settings.nHelpLineY = pSession->baselinePos - pSession->distBaseToHelpLine;

	decumaAssert(scrCheckSettings(&pSession->scr_api_settings) == decumaNoError);

	status = scrSelect(pSession->scr_output, &nOutputsReturned,
		sizeof(pSession->scr_output) / sizeof(pSession->scr_output[0]),
		&pSession->scr_api_settings,
		&pSession->charCurve,
		NULL,
		pSession->pScrMem,
		pInterruptFunctions);
	if (status != decumaNoError) goto recognize_exit;

 	for (i = 0; i < nOutputsReturned; i++) /* TODO handle this in scrAPI.c instead */
	{
		pSession->proximity[i] = scrOutputGetProximity(&pSession->scr_output[i]);
		if (pSession->proximity[i] > MAX_PROXIMITY) pSession->proximity[i] = MAX_PROXIMITY;

		/* Fix proximity to grow with rank (the conflict handling can cause this inconsistency) */
		if (i > 0 && pSession->proximity[i] < pSession->proximity[i-1])
		{
			/* Make the edges of the conflicting symbols identical except for symbol by */
			/* copying their output and curve properties. This is needed for equal further */
			/* processing (edge adjustment for new estimate and RG connection). Then just */
			/* let the non-prioritized symbol get 1 higher proximity than the prioritized */
			/* symbol. */
			DECUMA_UNICODE_DB_PTR pSymbol = pSession->scr_output[i].symbol;
			OUT_SYMBOL outSymbol = pSession->scr_output[i].outSymbol;

			pSession->proximity[i] = pSession->proximity[i-1];
			pSession->scr_output[i] = pSession->scr_output[i-1];
			pSession->scr_output[i].symbol = pSymbol;
			pSession->scr_output[i].outSymbol = outSymbol;
			pSession->proximity[i]++;
		}
	}

	for (i = 0; i < nOutputsReturned && *pnResults < nMaxResults; i++)
	{
		DECUMA_UNICODE_DB_PTR pSymbol = scrOutputGetSymbol(&pSession->scr_output[i]);
		int j, bGesture, bInstantGesture, bIsReliable;

		/* Non-original symbols are assumed to always be exactly one character long */
		decumaAssert(scrOutputIsOriginal(&pSession->scr_output[i]) || pSymbol[0] && pSymbol[1] == 0);

		/* Pure original or alternative symbols are expected from engine now. The both case is not handled here. */
		decumaAssert(!scrOutputCanBeBoth(&pSession->scr_output[i]));

		decumaMemset(pResults[*pnResults].pChars, 0, nMaxCharsPerResult * sizeof(pResults[*pnResults].pChars[0]));

		if (scrOutputIsAlternative(&pSession->scr_output[i]))
		{
			pResults[*pnResults].pChars[0] = decumaUnicodeToUpper(pSymbol[0], 1);
			pResults[*pnResults].nResultingChars = 1;
		}
		else
		{
			int nChars;

			for (nChars = 0; pSymbol[nChars]; nChars++)
				if (nChars < nMaxCharsPerResult-1) pResults[*pnResults].pChars[nChars] = pSymbol[nChars];

			pResults[*pnResults].nResultingChars = nChars;
		}

		pResults[*pnResults].nChars = pResults[*pnResults].nResultingChars < nMaxCharsPerResult-1 ? pResults[*pnResults].nResultingChars : nMaxCharsPerResult-1;

		pResults[*pnResults].distance = pSession->proximity[i];
		pResults[*pnResults].shapeDistance = pResults[*pnResults].distance;

		pResults[*pnResults].stringType = notFromDictionary;
		pResults[*pnResults].dictStrStart = 0;
		pResults[*pnResults].dictStrLen = 0;

		pResults[*pnResults].nSymbols = nMaxCharsPerResult > 1 ? 1 : 0;
		pResults[*pnResults].nResultingSymbols = 1;
		if (pResults[*pnResults].nSymbols)
		{
			if (pResults[*pnResults].pSymbolChars) pResults[*pnResults].pSymbolChars[0] = pResults[*pnResults].nChars;
			if (pResults[*pnResults].pSymbolStrokes) pResults[*pnResults].pSymbolStrokes[0] = pSession->charCurve.nArcs;
			if (pResults[*pnResults].pSymbolArcTimelineDiffMask) pResults[*pnResults].pSymbolArcTimelineDiffMask[0] = scrOutputGetArcTimelineDiffMask(&pSession->scr_output[i]);
		}

		_scrOutputIsGesture(&pSession->scr_output[i], &bGesture, &bInstantGesture);

		pResults[*pnResults].bGesture = bGesture;
		pResults[*pnResults].bInstantGesture = bInstantGesture;

		for (j = 0; j < *pnResults; j++)
			if (pResults[j].nChars == pResults[*pnResults].nChars &&
				decumaMemcmp(pResults[j].pChars, pResults[*pnResults].pChars, pResults[j].nChars * sizeof(pResults[j].pChars[0])) == 0) break;

		if (j < *pnResults) continue; /* Better duplicate exists. Don't add this result. */

		if (_scrIsOutputEstimateReliable(&pSession->scr_output[i], &bIsReliable) == decumaNoError && bIsReliable)
			_scrEstimateScalingAndVerticalOffset(&pSession->scr_output[i], &pSession->nBaseLineEstimates[*pnResults], &pSession->nHelpLineEstimates[*pnResults]);
		else
		{
			pSession->nBaseLineEstimates[*pnResults] = pSession->scr_api_settings.nBaseLineY;
			pSession->nHelpLineEstimates[*pnResults] = pSession->scr_api_settings.nHelpLineY;
		}

		(*pnResults)++;
	}

recognize_exit:
	pSession->nResults = *pnResults;
	pSession->nLastNotedResult = 0;

	/* Store current estimates (if existing) */
	if ((pSession->pSessionSettings->UIInputGuide != supportlines ||
		 pSession->pSessionSettings->supportLineSet == noSupportlines ||
		 pSession->bPositionAndScaleInvariant) &&
		pSession->nResults > 0 &&
		pSession->nBaseLineEstimates[pSession->nLastNotedResult] != pSession->nHelpLineEstimates[pSession->nLastNotedResult])
	{
		pSession->baselinePos = pSession->nBaseLineEstimates[pSession->nLastNotedResult];
		pSession->distBaseToHelpLine = pSession->nBaseLineEstimates[pSession->nLastNotedResult] - pSession->nHelpLineEstimates[pSession->nLastNotedResult];
		pSession->bReferenceEstimated = 1;
	}

	return status;
}

DECUMA_STATUS t9wlIndicateInstantGesture(
	T9WL_SESSION * pSession,
	int * pbInstantGesture,
	const DECUMA_INSTANT_GESTURE_SETTINGS * pInstantGestureSettings)
{
	return decumaFunctionNotSupported;
}

DECUMA_STATUS t9wlRecognizeForceRecognition( T9WL_SESSION * pSession,
	const DECUMA_UNICODE * symbolString,
	DECUMA_HWR_RESULT * pResults,
	DECUMA_UINT16 nMaxResults,
	DECUMA_UINT16 * pnResults,
	DECUMA_UINT16 nMaxCharsPerResult,
	const DECUMA_RECOGNITION_SETTINGS * pRecognitionSettings,
	int bFast,
	const DECUMA_INTERRUPT_FUNCTIONS * pInterruptFunctions)
{
	return decumaFunctionNotSupported;
}

DECUMA_STATUS t9wlRecognizeForceSegmentation( T9WL_SESSION * pSession,
	const DECUMA_UNICODE * symbolString,
	DECUMA_HWR_RESULT * pResults,
	DECUMA_UINT16 nMaxResults,
	DECUMA_UINT16 * pnResults,
	DECUMA_UINT16 nMaxCharsPerResult,
	const DECUMA_RECOGNITION_SETTINGS * pRecognitionSettings,
	int bFast,
	const DECUMA_INTERRUPT_FUNCTIONS * pInterruptFunctions)
{
	return decumaFunctionNotSupported;
}


/*Only for the extra API decuma_hwr_extra.h (so far) */
DECUMA_STATUS t9wlSetForcedSegmentation( T9WL_SESSION * pSession,
	DECUMA_INT16 * pForceSymbolStrokes,
	DECUMA_INT16 nForceSymbols)
{
	return decumaFunctionNotSupported;
}

/*Only for the extra API decuma_hwr_extra.h (so far) */
const DECUMA_INT16 * t9wlGetForcedSegmentation(T9WL_SESSION * pSession, DECUMA_INT16 * pnForceSymbols)
{
	return NULL;
}


/*Only for the extra API decuma_hwr_extra.h (so far) */
DECUMA_STATUS t9wlSetDictionaryFilterString(T9WL_SESSION * pSession, const DECUMA_UNICODE * symbolString)
{
	return decumaFunctionNotSupported;
}

/*Only for the extra API decuma_hwr_extra.h (so far) */
DECUMA_UNICODE * t9wlGetDictionaryFilterString(T9WL_SESSION * pSession)
{
	return NULL;
}

/*Only for the extra API decuma_hwr_extra.h (so far) */
DECUMA_STATUS t9wlConfirmArcs( T9WL_SESSION * pSession)
{
	decumaAssert(getT9WLSessionStatus(pSession) == decumaNoError); /*Should have been checked already */

	return decumaFunctionNotSupported;
}

/*Only for the extra API decuma_hwr_extra.h (so far) */
DECUMA_STATUS t9wlEvaluateArcs( T9WL_SESSION * pSession)
{
	decumaAssert(getT9WLSessionStatus(pSession) == decumaNoError); /*Should have been checked already */

	return decumaFunctionNotSupported;
}

/*Only for the extra API decuma_hwr_extra.h (so far) */
DECUMA_STATUS t9wlRecognizeExtraOutput( T9WL_SESSION * pSession,
	DECUMA_HWR_RESULT * pResults,
	DECUMA_UINT16 nMaxResults,
	DECUMA_UINT16 * pnResults,
	DECUMA_UINT16 nMaxCharsPerResult,
	void *  pCharArcIdxData, /*Should point to an array of size nMaxResults * nMaxCharsPerResult * */
	const DECUMA_RECOGNITION_SETTINGS * pRecognitionSettings,
	const DECUMA_INTERRUPT_FUNCTIONS * pInterruptFunctions)
{
	return decumaFunctionNotSupported;
}

/*Only for the extra API decuma_hwr_extra.h (so far) */
int t9wlGetCharArcIdxDataSize( DECUMA_UINT16 nMaxResults, DECUMA_UINT16 nMaxCharsPerResult)
{
	return 0;
}

/*Only for the extra API decuma_hwr_extra.h (so far) */
const DECUMA_UINT16 * t9wlGetCharArcIdxs( const void * pCharArcIdxData,
														 DECUMA_UINT16 nResultIdx, DECUMA_UINT16 nMaxResults,
														 DECUMA_UINT16 nCharIdx, DECUMA_UINT16 nMaxCharsPerResult,
														 DECUMA_UINT16 * pnArcs)
{
	return NULL;
}

DECUMA_STATUS t9wlGetDistance( T9WL_SESSION * pSession,
	const DECUMA_UNICODE * symbolString,
	const DECUMA_RECOGNITION_SETTINGS * pRecognitionSettings,
	DECUMA_UINT32 * pDistance,
	DECUMA_STRING_TYPE * pStringType)
{
	return decumaFunctionNotSupported;
}

DECUMA_STATUS t9wlEndArcAddition(T9WL_SESSION * pSession)
{
	int i;

	decumaAssert(getT9WLSessionStatus(pSession) == decumaNoError); /*Should have been checked already */
	decumaAssert(pSession->bAddingArcs); /*Should already have been checked by outer layer */

	/* Cancel all uncommitted arcs */
	for (i = 0; i < pSession->nUncommittedArcs; i++)
	{
		t9wlCancelArc(pSession, pSession->arcIDs[i]);
	}

	pSession->charCurve.nArcs = 0;

	pSession->bAddingArcs = 0;

	return decumaNoError;
}

/******* Dictionary conversion functions *******/

DECUMA_STATUS t9wlUnpackXT9Dictionary(void ** ppDictionaryData,
	const void * pXT9Dictionary, DECUMA_UINT32 xt9DataSize, DECUMA_XT9_DICTIONARY_TYPE type,
	DECUMA_UINT32 * pUnpackedDataSize,const DECUMA_MEM_FUNCTIONS * pMemFunctions)
{
	return decumaFunctionNotSupported;
}

DECUMA_STATUS t9wlDestroyUnpackedDictionary(void ** ppDictionaryData,
	const DECUMA_MEM_FUNCTIONS * pMemFunctions)
{
	return decumaFunctionNotSupported;
}


/******* Dictionary attachment functions *******/

DECUMA_STATUS t9wlAttachStaticDictionary(T9WL_SESSION * pSession,
										const void * pDictionaryData)
{
	return decumaFunctionNotSupported;
}


DECUMA_STATUS t9wlDetachStaticDictionary(T9WL_SESSION * pSession,
										const void * pDictionaryData)
{
	return decumaFunctionNotSupported;
}

/******* Dynamic dictionary functions *******/

DECUMA_STATUS t9wlAttachDynamicDictionary(T9WL_SESSION * pSession,
														   DECUMA_DICTIONARY_TYPE type,
														   void * pDictionaryData,
														   int cbDictionaryDataSize)
{
	return decumaFunctionNotSupported;
}

DECUMA_STATUS t9wlDetachDynamicDictionary(T9WL_SESSION * pSession,
														   const void * pDictionaryData)
{
	return decumaFunctionNotSupported;
}


DECUMA_STATUS t9wlAddWordToDictionary(T9WL_SESSION * pSession, void * pDictionaryBuffer, DECUMA_UNICODE * pWord)
{
	return decumaFunctionNotSupported;
}


DECUMA_STATUS t9wlGetDynamicDictionarySize(T9WL_SESSION * pSession, const void * pDictionaryBuffer, int * pSize)
{
	return decumaFunctionNotSupported;
}


DECUMA_STATUS t9wlWordExistsInDictionary(T9WL_SESSION * pSession, const void * pDictionaryBuffer, DECUMA_UNICODE * pWord)
{
	return decumaFunctionNotSupported;
}

DECUMA_STATUS t9wlAddAllWords(T9WL_SESSION * pSession,
											   void * pDictionaryBuffer,
											   const DECUMA_UNICODE * pWordBuffer,
											   int nUnicodesInBuffer,
											   int bNullDelimitedOnly)
{
	return decumaFunctionNotSupported;
}


/******* Word completion functions *******/
#ifdef DECUMA_HWR_ENABLE_EXTRA_API
DECUMA_STATUS t9wlCompleteWord(T9WL_SESSION * pSession,
												DECUMA_COMPLETION_RESULT * pResults,
												DECUMA_UINT32 nMaxResults,
												DECUMA_UINT32 * pnResults,
												DECUMA_UINT32 nMaxCharsPerResult,
												DECUMA_UNICODE * pChars,
												DECUMA_UINT16 nChars)
{
	return decumaFunctionNotSupported;
}
#endif

/*///////////////////////////////////////////////////////////////////////////////////////// */

DECUMA_STATUS t9wlUsePositionAndScaleInvariantRecognition( T9WL_SESSION * pSession, int bOnOff)
{
	decumaAssert(getT9WLSessionStatus(pSession) == decumaNoError); /*Should have been checked already */

	pSession->bPositionAndScaleInvariant = bOnOff;

	return decumaNoError;
}

DECUMA_STATUS t9wlUseTypeConnectionRequirement( T9WL_SESSION * pSession, int bOnOff)
{
	decumaAssert(getT9WLSessionStatus(pSession) == decumaNoError); /*Should have been checked already */
	return decumaNoError; /*Unimportant for t9wllib */
}

DECUMA_STATUS t9wlUseStrictTypeConnectionRequirement( T9WL_SESSION * pSession, int bOnOff)
{
	decumaAssert(getT9WLSessionStatus(pSession) == decumaNoError); /*Should have been checked already */
	return decumaNoError; /*Unimportant for t9wllib */
}

int t9wlGetArcSessionSegmentCount(T9WL_SESSION * pSession)
{
	decumaAssert(getT9WLSessionStatus(pSession) == decumaNoError); /*Should have been checked already */

	return 0;
}


DECUMA_UINT32 t9wlGetMaxNumberOfSegments(void)
{
	return 0;
}

DECUMA_UINT32 t9wlGetArcSessionSize(void)
{
	return 0;
}

DECUMA_UINT32 t9wlGetSGSize(void)
{
	return 0;
}

DECUMA_UINT32 t9wlGetRGSize(void)
{
	return 0;
}


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
			status = t9wlDatabaseIncludesSymbol(pStaticDB,NULL,pSymbolStart,&bIncluded);
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
	status = t9wlDatabaseIncludesSymbol(pStaticDB,NULL,pSymbolStart,&bIncluded);
	if (status!=decumaNoError) return status;

	*pbIncluded=bIncluded;

	return decumaNoError;

}

static DECUMA_STATUS setSessionSettings(T9WL_SESSION * pSession,
										const DECUMA_SESSION_SETTINGS *pSessionSettings)
{
	int distBaseToHelpLine = 0;
	int baselinePos = 0;

	int bUpdateSupportlines=0;
	DECUMA_STATUS status=decumaNoError;

#if defined(_DEBUG) || defined(DECUMA_ASSERT_ENABLE)
	/*This should already have been checked by decuma_hwr layer */
	status = t9wlValidateSessionSettings(pSessionSettings);
	decumaAssert(status == decumaNoError||status == decumaAllocationFailed);
#endif

	decumaAssert(pSessionSettings->recognitionMode == scrMode);

	if (!pSession->pSessionSettings ||
		pSessionSettings->supportLineSet != pSession->pSessionSettings->supportLineSet ||
		pSessionSettings->baseline != pSession->pSessionSettings->baseline ||
		pSessionSettings->helpline != pSession->pSessionSettings->helpline)
	{
		bUpdateSupportlines = 1;

		if (pSessionSettings->UIInputGuide == supportlines)
		{
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
				baselinePos = 0;
				distBaseToHelpLine = 0;
				break;
			default: decumaAssert(0);
			}
		}
		else if (pSessionSettings->UIInputGuide == none)
		{
			baselinePos = 0;
			distBaseToHelpLine = 0;
		}
		else {
			decumaAssert(0);
		}
	}
	/* else no need to refresh internal state with identical support line setting (might only destroy refined estimate) */

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

	/* Set SCR API settings accordingly */

	pSession->scr_api_settings.characterSet = pSessionSettings->charSet;
	pSession->scr_api_settings.nBaseLineY = pSession->baselinePos;
	pSession->scr_api_settings.nHelpLineY = pSession->baselinePos - pSession->distBaseToHelpLine;
	pSession->scr_api_settings.pStaticDB = (STATIC_DB_PTR)pSessionSettings->pStaticDB;
	pSession->scr_api_settings.pUDM = (UDM_PTR)pSessionSettings->pDynamicDB;

	decumaAssert(pSession->scr_api_settings.nRefAngle == 0);
	decumaAssert(pSession->scr_api_settings.nRefAngleImportance == FULL_ROTATION_IMPORTANCE);
	decumaAssert(pSession->scr_api_settings.pCategoryTable == 0);

	decumaAssert(status == scrCheckSettings(&pSession->scr_api_settings));

	return decumaNoError;
}

static DECUMA_UINT32	getMeanDistance(T9WL_SESSION * pSession, DECUMA_UINT32 nTotalDist, int nNodes)
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

static DECUMA_STATUS getT9WLSessionStatus(const T9WL_SESSION * pSession)
{
	if (pSession == NULL) return decumaNullSessionPointer;
	if (pSession->pScrMem != &pSession[1]) return decumaSessionNotInitialized;

	if (pSession->bIsCorrupt)
	{
		return decumaSessionCorrupt;
	}
/*	else if (pSession->bAddingArcs && (t9wlArcSessionIsCorrupt(pSession->pArcSession) || t9wlSGIsCorrupt(pSession->pSG)))
	{
		return decumaSessionCorrupt;
	}*/

	return decumaNoError;
}

long t9wlGetMemoryStatistics(char *pBuf,
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

#ifdef EXTRA_VERIFICATION_INFO
T9WL_SG * t9wlGetSG(T9WL_SESSION * pSession)
{
	return 0;
}

T9WL_SG * t9wlGetForcedSG(T9WL_SESSION * pSession)
{
	return 0;
}

T9WL_RG * t9wlGetRG(T9WL_SESSION * pSession)
{
	return 0;
}

T9WL_RG * t9wlGetForcedRG(T9WL_SESSION * pSession)
{
	return 0;
}

T9WL_ARC_SESSION * t9wlGetArcSession(T9WL_SESSION * pSession)
{
	return 0;
}
#endif /*EXTRA_VERIFICATION_INFO */


/*********** END ***********/
