/*****************************************************************************
* COPYRIGHT ZI AND SUBJECT TO CONFIDENTIALITY RESTRICTIONS                   *
*                                                                            *
* This file is the confidential and proprietary property of Zi Corporation   *
* of Canada, Inc. ("Zi") or one of its affiliates.                           *
******************************************************************************/

#ifndef DLTLIB_H_
#define DLTLIB_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "decumaConfig.h"

#ifdef DECUMA_HWR_ENABLE_EXTRA_API
#include "decuma_hwr_extra.h"
#endif

#include "decuma_hwr_types.h"

#include "cjkSession.h"
#include "decumaBasicTypes.h"
#include "decumaBasicTypesMinMax.h"
#include "decumaStatus.h"
#include "decumaCurve.h"
#include "decumaUnicode.h"
#include "cjkCategories.h"

/* -------------------------------------------------------------------------
 * Macros
 * ------------------------------------------------------------------------- */

#define MAX_CJK_COORD (MAX_DECUMA_INT16)
#define MIN_CJK_COORD (MIN_DECUMA_INT16)

/* -------------------------------------------------------------------------
 * Exported API
 * ------------------------------------------------------------------------- */


DECUMA_HWR_PRIVATE const char * cjkGetVersion(void);

DECUMA_HWR_PRIVATE DECUMA_UINT32 cjkGetSessionSize(void);

/**
 * Initialize session object. Caller is responsible for checking for null parameters.
 */

DECUMA_HWR_PRIVATE DECUMA_STATUS cjkBeginSession(CJK_SESSION * pSession,
	DECUMA_SESSION_SETTINGS * pSessionSettings,
	const DECUMA_MEM_FUNCTIONS * pMemFunctions);

/**
 * Check that the pStaticDB pointer points to a non-NULL dlt valid database.
 */

DECUMA_HWR_PRIVATE DECUMA_STATUS cjkValidateStaticDatabase(DECUMA_STATIC_DB_PTR pStaticDB);

/**
 * Check that the pDynamicDB pointer points to a non-NULL dlt valid dynamic database.
 */

DECUMA_HWR_PRIVATE DECUMA_STATUS cjkValidateDynamicDatabase(DECUMA_DYNAMIC_DB_PTR pDynamicDB);

/**
 * Check that session is initialized correctly. Caller responsible for NULL-check.
 */

DECUMA_HWR_PRIVATE DECUMA_STATUS cjkVerifySession(const CJK_SESSION * pSession);

/**
 * This function actually does nothing for dlt at this point.
 */

DECUMA_HWR_PRIVATE DECUMA_STATUS cjkEndSession(CJK_SESSION * pSession);

/**
 * Get database version string. All parameters checked by caller.
 */

DECUMA_HWR_PRIVATE DECUMA_STATUS cjkDatabaseGetVersion(DECUMA_ANY_DB_PTR pDB, char * pBuf, int nBufLen);

/**
 * Find out if a category is supported by pDB. All parameters checked by caller.
 */

DECUMA_HWR_PRIVATE DECUMA_STATUS cjkDatabaseIsCategorySupported(DECUMA_ANY_DB_PTR pDB,
	DECUMA_UINT32 cat, int * pbIsSupported);

/**
 * Find out if a language is supported by pDB. All parameters checked by caller.
 */

DECUMA_HWR_PRIVATE DECUMA_STATUS cjkDatabaseIsLanguageSupported(DECUMA_ANY_DB_PTR pDB,
	DECUMA_UINT32 lang, int * pbIsSupported);

DECUMA_HWR_PRIVATE DECUMA_STATUS cjkDatabaseIncludesSymbol(
	DECUMA_ANY_DB_PTR pDB,
	const DECUMA_CHARACTER_SET * pCharSet,
	const DECUMA_UNICODE * pSymbol, 
	int * pbIsIncluded);

DECUMA_HWR_PRIVATE DECUMA_STATUS cjkChangeSessionSettings(CJK_SESSION * pSession, DECUMA_SESSION_SETTINGS * pSessionSettings);

DECUMA_HWR_PRIVATE DECUMA_STATUS cjkBeginArcAddition(CJK_SESSION * pSession);

DECUMA_HWR_PRIVATE DECUMA_STATUS cjkStartNewArc(CJK_SESSION * pSession, DECUMA_UINT32 arcID);

DECUMA_HWR_PRIVATE DECUMA_STATUS cjkAddPoints(CJK_SESSION * pSession,
								   DECUMA_POINT * pPts,
								   int nPts,
								   DECUMA_UINT32 arcID,
								   int * pnPtsAdded);

DECUMA_HWR_PRIVATE DECUMA_STATUS cjkAddPoint(CJK_SESSION * pSession,
								  DECUMA_COORD x,
								  DECUMA_COORD y,
								  DECUMA_UINT32 arcID);

DECUMA_HWR_PRIVATE DECUMA_STATUS cjkGetUncommittedArcCount(CJK_SESSION * pSession, int * pnUncommittedArcs);

DECUMA_HWR_PRIVATE DECUMA_STATUS cjkGetUncommittedArcID(CJK_SESSION * pSession, int idx, DECUMA_UINT32 * pArcID);

DECUMA_HWR_PRIVATE DECUMA_STATUS cjkCancelArc(CJK_SESSION * pSession, DECUMA_UINT32 arcID);

DECUMA_HWR_PRIVATE DECUMA_STATUS cjkCommitArc(CJK_SESSION * pSession,DECUMA_UINT32 arcID);

DECUMA_HWR_PRIVATE DECUMA_STATUS cjkRecognizeFast( CJK_SESSION * pSession,
	DECUMA_HWR_RESULT * pResult,
	DECUMA_UINT16 * pnResults,  /* In most cases == 1, but can be 0 */
	DECUMA_UINT16 nMaxChars,
	const DECUMA_INTERRUPT_FUNCTIONS * pInterruptFunctions);


DECUMA_HWR_PRIVATE DECUMA_STATUS
cjkGetCategorymask(const CJK_SESSION * pSession, DECUMA_UINT32 * pCategoryMask);

DECUMA_HWR_PRIVATE DECUMA_STATUS cjkRecognize( CJK_SESSION * pSession,
	DECUMA_HWR_RESULT * pResults,
	DECUMA_UINT16 nMaxResults,
	DECUMA_UINT16 * pnResults,
	DECUMA_UINT16 nMaxCharsPerResult,
	const DECUMA_RECOGNITION_SETTINGS * pRecognitionSettings,
	const DECUMA_INTERRUPT_FUNCTIONS * pInterruptFunctions);

DECUMA_HWR_PRIVATE DECUMA_STATUS cjkGetDistance( CJK_SESSION * pSession,
	const DECUMA_UNICODE * symbolString,
	const DECUMA_RECOGNITION_SETTINGS * pRecognitionSettings,
	DECUMA_UINT32 * pDistance,
	DECUMA_STRING_TYPE * pStringType);

DECUMA_HWR_PRIVATE DECUMA_STATUS cjkValidateRecognitionSettings(const DECUMA_RECOGNITION_SETTINGS * pRecognitionSettings);


DECUMA_HWR_PRIVATE DECUMA_STATUS cjkValidateSessionSettings(const DECUMA_SESSION_SETTINGS * pSessionSettings);

DECUMA_HWR_PRIVATE DECUMA_STATUS dltCreateCategoryMask(const DECUMA_CHARACTER_SET * pCharSet, DECUMA_UINT32 * mask);

DECUMA_HWR_PRIVATE DECUMA_STATUS cjkSetCategories( CJK_SESSION * pSession, const DECUMA_SESSION_SETTINGS * pSessionSettings );

DECUMA_HWR_PRIVATE DECUMA_STATUS cjkEndArcAddition(CJK_SESSION * pSession);

/************************************************************************\   
*       FUNCTIONS BELOW ARE NOT IMPLEMENTED YET FOR DLTLIB               *
*                                                                        *
\************************************************************************/

DECUMA_HWR_PRIVATE DECUMA_STATUS cjkIndicateInstantGesture(
	CJK_SESSION * pSession,
	int * pbInstantGesture,
	const DECUMA_INSTANT_GESTURE_SETTINGS * pInstantGestureSettings);

DECUMA_HWR_PRIVATE DECUMA_STATUS cjkRecognizeForceRecognition( CJK_SESSION * pSession,
	const DECUMA_UNICODE * symbolString,
	DECUMA_HWR_RESULT * pResults,
	DECUMA_UINT16 nMaxResults,
	DECUMA_UINT16 * pnResults,
	DECUMA_UINT16 nMaxCharsPerResult,
	const DECUMA_RECOGNITION_SETTINGS * pRecognitionSettings,
	int bFast,
 	const DECUMA_INTERRUPT_FUNCTIONS * pInterruptFunctions);

DECUMA_HWR_PRIVATE DECUMA_STATUS cjkRecognizeForceSegmentation( CJK_SESSION * pSession,
	const DECUMA_UNICODE * symbolString,
	DECUMA_HWR_RESULT * pResults,
	DECUMA_UINT16 nMaxResults,
	DECUMA_UINT16 * pnResults,
	DECUMA_UINT16 nMaxCharsPerResult,
	const DECUMA_RECOGNITION_SETTINGS * pRecognitionSettings,
	int bFast,
 	const DECUMA_INTERRUPT_FUNCTIONS * pInterruptFunctions);

DECUMA_HWR_PRIVATE DECUMA_STATUS cjkNoteSelectedCandidate( CJK_SESSION * pSession, int nResultIdx);


/******* Dictionary conversion functions *******/

DECUMA_HWR_PRIVATE DECUMA_STATUS cjkUnpackXT9Dictionary(void ** ppDictionaryData,
	const void * pXT9Dictionary, 
	DECUMA_UINT32 xt9DataSize, 
	DECUMA_XT9_DICTIONARY_TYPE type,
	DECUMA_UINT32 * pUnpackedDataSize,
	const DECUMA_MEM_FUNCTIONS * pMemFunctions);

DECUMA_HWR_PRIVATE DECUMA_STATUS cjkDestroyUnpackedDictionary(void ** ppDictionaryData,
	const DECUMA_MEM_FUNCTIONS * pMemFunctions);

/******* Dictionary attachment functions *******/

DECUMA_HWR_PRIVATE DECUMA_STATUS cjkAttachStaticDictionary(CJK_SESSION * pSession,
									 const void * pDictionaryData);

DECUMA_HWR_PRIVATE DECUMA_STATUS cjkDetachStaticDictionary(CJK_SESSION * pSession,
									 const void * pDictionary);

/******* Dynamic Dictionary functions *******/

DECUMA_HWR_PRIVATE DECUMA_STATUS cjkStartNewSymbol(CJK_SESSION * pSession);

DECUMA_HWR_PRIVATE DECUMA_STATUS cjkAttachDynamicDictionary(CJK_SESSION * pSession,
									     DECUMA_DICTIONARY_TYPE type,
										 void * pDictionaryData,
										 int cbDictionaryDataSize);

DECUMA_HWR_PRIVATE DECUMA_STATUS cjkDetachDynamicDictionary(CJK_SESSION * pSession,
										 const void * pDictionaryData);

DECUMA_HWR_PRIVATE DECUMA_STATUS cjkAddWordToDictionary(CJK_SESSION * pSession, void * pDictionaryBuffer,
									 DECUMA_UNICODE * pWord);

DECUMA_HWR_PRIVATE DECUMA_STATUS cjkGetDynamicDictionarySize(CJK_SESSION * pSession,
										  const void * pDictionaryBuffer, 
										  int * pSize);

DECUMA_HWR_PRIVATE DECUMA_STATUS cjkWordExistsInDictionary(CJK_SESSION * pSession,
										const void * pDictionaryBuffer, 
										DECUMA_UNICODE * pWord);

DECUMA_HWR_PRIVATE DECUMA_STATUS cjkAddAllWords(CJK_SESSION * pSession,
											   void * pDictionaryBuffer,
											   const DECUMA_UNICODE * pWordBuffer,
											   int nUnicodesInBuffer,
											   int bNullDelimitedOnly);

/*///////////////////////////////////////////////////////////////////////////////////////// */




/******* Functions in decuma_hwr_extra.h *******//*//////////////// */

#ifdef DECUMA_HWR_ENABLE_EXTRA_API
/******* Word completion functions *******/

DECUMA_HWR_PRIVATE DECUMA_STATUS cjkCompleteWord(CJK_SESSION * pSession,
							  DECUMA_COMPLETION_RESULT * pResults,
							  DECUMA_UINT32 nMaxResults,
							  DECUMA_UINT32 * pnResults,
							  DECUMA_UINT32 nMaxCharsPerResult,
							  DECUMA_UNICODE * pChars,
							  DECUMA_UINT16 nChars);

DECUMA_HWR_PRIVATE DECUMA_STATUS cjkSetForcedSegmentation( CJK_SESSION * pSession,
	DECUMA_INT16 * pForceSymbolStrokes,
	DECUMA_INT16 nForceSymbols);

DECUMA_HWR_PRIVATE const DECUMA_INT16 * cjkGetForcedSegmentation(CJK_SESSION * pSession, DECUMA_INT16 * pnForceSymbols);


DECUMA_HWR_PRIVATE DECUMA_STATUS cjkSetDictionaryFilterString(CJK_SESSION * pSession,
											 const DECUMA_UNICODE * symbolString);

DECUMA_HWR_PRIVATE DECUMA_UNICODE * cjkGetDictionaryFilterString(CJK_SESSION * pSession);


DECUMA_HWR_PRIVATE DECUMA_STATUS cjkConfirmArcs( CJK_SESSION * pSession);

DECUMA_HWR_PRIVATE DECUMA_STATUS cjkEvaluateArcs( CJK_SESSION * pSession);

DECUMA_HWR_PRIVATE DECUMA_STATUS cjkRecognizeExtraOutput( CJK_SESSION * pSession,
	DECUMA_HWR_RESULT * pResults,
	DECUMA_UINT16 nMaxResults,
	DECUMA_UINT16 * pnResults,
	DECUMA_UINT16 nMaxCharsPerResult,
	void *  pCharArcIdxData, /*Should point to an array of size nMaxResults * nMaxCharsPerResult * */
	const DECUMA_RECOGNITION_SETTINGS * pRecognitionSettings,
 	const DECUMA_INTERRUPT_FUNCTIONS * pInterruptFunctions);

DECUMA_HWR_PRIVATE int cjkGetCharArcIdxDataSize( DECUMA_UINT16 nMaxResults,
							 DECUMA_UINT16 nMaxCharsPerResult);

DECUMA_HWR_PRIVATE const DECUMA_UINT16 * cjkGetCharArcIdxs( const void * pCharArcIdxData,
										DECUMA_UINT16 nResultIdx, DECUMA_UINT16 nMaxResults,
										DECUMA_UINT16 nCharIdx, DECUMA_UINT16 nMaxCharsPerResult,
										DECUMA_UINT16 * pnArcs);

DECUMA_HWR_PRIVATE long cjkGetMemoryStatistics(char *pBuf,
							long nBufLen);

DECUMA_HWR_PRIVATE DECUMA_STATUS cjkUsePositionAndScaleInvariantRecognition( CJK_SESSION * pSession, int bOnOff);

DECUMA_HWR_PRIVATE DECUMA_STATUS cjkUseTypeConnectionRequirement( CJK_SESSION * pSession, int bOnOff);

DECUMA_HWR_PRIVATE DECUMA_STATUS cjkUseStrictTypeConnectionRequirement( CJK_SESSION * pSession, int bOnOff);

DECUMA_HWR_PRIVATE int cjkGetArcSessionSegmentCount(CJK_SESSION * pSession);

DECUMA_HWR_PRIVATE DECUMA_UINT32 cjkGetMaxNumberOfSegments(void);

DECUMA_HWR_PRIVATE DECUMA_UINT32 cjkGetArcSessionSize(void);

DECUMA_HWR_PRIVATE DECUMA_UINT32 cjkGetSGSize(void);

DECUMA_HWR_PRIVATE DECUMA_UINT32 cjkGetRGSize(void);

#endif /* DECUMA_HWR_ENABLE_EXTRA_API */

#ifdef __cplusplus
}
#endif

#endif /* DLTLIB_H_ */
