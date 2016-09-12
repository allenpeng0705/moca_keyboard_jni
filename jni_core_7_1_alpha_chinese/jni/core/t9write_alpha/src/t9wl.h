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

  This file describes the interface to Decumas HWR engine for
  recognition of connected or separate characters.

  See the API Reference PDF for proper documentation of the
  API.

\******************************************************************/

#ifndef T9WL_H
#define T9WL_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _T9WL_SESSION T9WL_SESSION;

#include "decuma_hwr.h"

#ifdef DECUMA_HWR_ENABLE_EXTRA_API
#include "decuma_hwr_extra.h"
#endif

/************************************************
*        Function prototypes                    *
************************************************/



/****** Version ********/

/*Returns a zero terminated string */

DECUMA_HWR_PRIVATE const char * t9wlGetVersion(void);


/******* Database functions *********/


DECUMA_HWR_PRIVATE DECUMA_STATUS t9wlValidateStaticDatabase(DECUMA_STATIC_DB_PTR pStaticDB);

DECUMA_HWR_PRIVATE DECUMA_STATUS t9wlValidateDynamicDatabase(DECUMA_DYNAMIC_DB_PTR pDynamicDB);

DECUMA_HWR_PRIVATE DECUMA_STATUS t9wlDatabaseGetVersion(DECUMA_ANY_DB_PTR pDB, char * pBuf, int nBufLen);

DECUMA_HWR_PRIVATE DECUMA_STATUS t9wlDatabaseIsCategorySupported(DECUMA_ANY_DB_PTR pDB,
	DECUMA_UINT32 cat, int * pbIsSupported);

DECUMA_HWR_PRIVATE DECUMA_STATUS t9wlDatabaseIsLanguageSupported(DECUMA_ANY_DB_PTR pDB,
	DECUMA_UINT32 lang, int * pbIsSupported);

DECUMA_HWR_PRIVATE DECUMA_STATUS t9wlDatabaseIncludesSymbol(
	DECUMA_ANY_DB_PTR pDB,
	const DECUMA_CHARACTER_SET * pCharSet,
	const DECUMA_UNICODE * pSymbol,
	int * pbIsIncluded);

DECUMA_HWR_PRIVATE DECUMA_STATUS t9wlDynamicDatabaseIsValid(DECUMA_DYNAMIC_DB_PTR pDynamicDB);

DECUMA_HWR_PRIVATE DECUMA_STATUS t9wlCreateDynamicDatabase(DECUMA_DYNAMIC_DB_PTR* ppDynamicDB, const DECUMA_MEM_FUNCTIONS * pMemFunctions);

DECUMA_HWR_PRIVATE DECUMA_STATUS t9wlAddAllograph(      DECUMA_DYNAMIC_DB_PTR  * ppDynamicDB,
								 const DECUMA_CURVE         * pCurve,
								 const DECUMA_UNICODE       * pUnicode, DECUMA_UINT32 nUnicodes,
								 const DECUMA_CHARACTER_SET * pCharacterSet,
								 DECUMA_INT32 nBaseline, DECUMA_INT32 nHelpline,
								 int bGesture, int bInstantGesture,
								 const DECUMA_MEM_FUNCTIONS * pMemFunctions);

DECUMA_HWR_PRIVATE DECUMA_STATUS t9wlGetDynamicDatabaseByteSize(DECUMA_DYNAMIC_DB_PTR pDynamicDB, DECUMA_UINT32 * pSize);

DECUMA_HWR_PRIVATE DECUMA_STATUS t9wlDestroyDynamicDatabase(DECUMA_DYNAMIC_DB_PTR* ppDynamicDB, const DECUMA_MEM_FUNCTIONS * pMemFunctions);

/****** Initialization, verification and destruction ********/


DECUMA_HWR_PRIVATE DECUMA_UINT32 t9wlGetSessionSize(void);

/*NOTE: The contents of pSessionSettings is not copied by the module */
/*      But a reference to teh session settings pointer is stored. */
/*      So the settings need to be stored on heap since it will be referenced */
/*      at later times. */
/*      The same goes for pMemoryFunctions */
DECUMA_HWR_PRIVATE DECUMA_STATUS t9wlBeginSession(T9WL_SESSION * pSession,
	const DECUMA_SESSION_SETTINGS * pSessionSettings,
	const DECUMA_MEM_FUNCTIONS * pMemoryFunctions);


DECUMA_HWR_PRIVATE DECUMA_STATUS t9wlVerifySession(const T9WL_SESSION * pSession);


DECUMA_HWR_PRIVATE DECUMA_STATUS t9wlEndSession(T9WL_SESSION * pSession);





/******* Settings *********/

DECUMA_HWR_PRIVATE DECUMA_STATUS t9wlValidateSessionSettings(
	const DECUMA_SESSION_SETTINGS * pSessionSettings);

/*NOTE: The contents of pSessionSettings is not copied by the module */
/*      But a reference to teh session settings pointer is stored. */
/*      So the settings need to be stored on heap since it will be referenced */
/*      at later times. */
/*NOTE! Any errors occuring in this function (e.g. allocation errors), */
/*      should report an error and the previous settings MUST be */
/*      reverted to. */
DECUMA_HWR_PRIVATE DECUMA_STATUS t9wlChangeSessionSettings( T9WL_SESSION * pSession,
	const DECUMA_SESSION_SETTINGS * pSessionSettings);


/****** Recognition functions ********/


DECUMA_HWR_PRIVATE DECUMA_STATUS t9wlBeginArcAddition(T9WL_SESSION * pSession);


DECUMA_HWR_PRIVATE DECUMA_STATUS t9wlStartNewArc(T9WL_SESSION * pSession, DECUMA_UINT32 arcID);

DECUMA_HWR_PRIVATE DECUMA_STATUS t9wlStartNewSymbol(T9WL_SESSION * pSession);

/*Adds point(s) to a started arc */
DECUMA_HWR_PRIVATE DECUMA_STATUS t9wlAddPoint(T9WL_SESSION * pSession,
						  DECUMA_COORD x, DECUMA_COORD y,
						  DECUMA_UINT32 arcID);

DECUMA_HWR_PRIVATE DECUMA_STATUS t9wlAddPoints(T9WL_SESSION * pSession,
							  DECUMA_POINT * pPts, int nPts,
							  DECUMA_UINT32 arcID,
							  int * pnPtsAdded);


/*Ends the arc and removes it from the session */
DECUMA_HWR_PRIVATE DECUMA_STATUS t9wlCancelArc(T9WL_SESSION * pSession,DECUMA_UINT32 arcID);

/*Ends and adds the arc to the arc-addition-sequence which will be used in a future recognize call */
DECUMA_HWR_PRIVATE DECUMA_STATUS t9wlCommitArc(T9WL_SESSION * pSession,DECUMA_UINT32 arcID);

DECUMA_HWR_PRIVATE DECUMA_STATUS t9wlGetUncommittedArcCount(T9WL_SESSION * pSession, int * pnUncommittedArcs);
DECUMA_HWR_PRIVATE DECUMA_STATUS t9wlGetUncommittedArcID(T9WL_SESSION * pSession, int idx,
										DECUMA_UINT32 * pArcID);

DECUMA_HWR_PRIVATE DECUMA_STATUS t9wlValidateRecognitionSettings(const DECUMA_RECOGNITION_SETTINGS * pRecognitionSettings);

DECUMA_HWR_PRIVATE DECUMA_STATUS t9wlRecognize( T9WL_SESSION * pSession,
	DECUMA_HWR_RESULT * pResults,
	DECUMA_UINT16 nMaxResults,
	DECUMA_UINT16 * pnResults,
	DECUMA_UINT16 nMaxCharsPerResult,
	const DECUMA_RECOGNITION_SETTINGS * pRecognitionSettings,
	const DECUMA_INTERRUPT_FUNCTIONS * pInterruptFunctions	);

DECUMA_HWR_PRIVATE DECUMA_STATUS t9wlIndicateInstantGesture(
	T9WL_SESSION * pSession,
	int * pbInstantGesture,
	const DECUMA_INSTANT_GESTURE_SETTINGS * pInstantGestureSettings);


DECUMA_HWR_PRIVATE DECUMA_STATUS t9wlEndArcAddition(T9WL_SESSION * pSession);

/******* Dictionary functions *******/

DECUMA_HWR_PRIVATE DECUMA_STATUS t9wlUnpackXT9Dictionary(void ** ppDictionaryData,
	const void * pXT9Dictionary, DECUMA_UINT32 xt9DataSize, DECUMA_XT9_DICTIONARY_TYPE type,
	DECUMA_UINT32 * pUnpackedDataSize,const DECUMA_MEM_FUNCTIONS * pMemoryFunctions);

DECUMA_HWR_PRIVATE DECUMA_STATUS t9wlDestroyUnpackedDictionary(void ** ppDictionaryData,
	const DECUMA_MEM_FUNCTIONS * pMemoryFunctions);

DECUMA_HWR_PRIVATE DECUMA_STATUS t9wlAttachStaticDictionary(T9WL_SESSION * pSession,
	const void * pDictionary);

DECUMA_HWR_PRIVATE DECUMA_STATUS t9wlDetachStaticDictionary(T9WL_SESSION * pSession,
	const void * pDictionary);

/******* Dynamic dictionary functions *******/

DECUMA_HWR_PRIVATE DECUMA_STATUS t9wlAttachDynamicDictionary(T9WL_SESSION * pSession,
														   DECUMA_DICTIONARY_TYPE type,
														   void * pDictionaryData,
														   int cbDictionaryDataSize);

DECUMA_HWR_PRIVATE DECUMA_STATUS t9wlDetachDynamicDictionary(T9WL_SESSION * pSession,
														   const void * pDictionaryData);

DECUMA_HWR_PRIVATE DECUMA_STATUS t9wlAddWordToDictionary(T9WL_SESSION * pSession, void * pDictionaryBuffer, DECUMA_UNICODE * pWord);

DECUMA_HWR_PRIVATE DECUMA_STATUS t9wlGetDynamicDictionarySize(T9WL_SESSION * pSession, const void * pDictionaryBuffer, int * pSize);

DECUMA_HWR_PRIVATE DECUMA_STATUS t9wlWordExistsInDictionary(T9WL_SESSION * pSession, const void * pDictionaryBuffer, DECUMA_UNICODE * pWord);

DECUMA_HWR_PRIVATE DECUMA_STATUS t9wlAddAllWords(T9WL_SESSION * pSession,
											   void * pDictionaryBuffer,
											   const DECUMA_UNICODE * pWordBuffer,
											   int nUnicodesInBuffer,
											   int bNullDelimitedOnly);

/******* Word completion functions *******/

#ifdef DECUMA_HWR_ENABLE_EXTRA_API
DECUMA_HWR_PRIVATE DECUMA_STATUS t9wlCompleteWord(T9WL_SESSION * pSession,
												DECUMA_COMPLETION_RESULT * pResults,
												DECUMA_UINT32 nMaxResults,
												DECUMA_UINT32 * pnResults,
												DECUMA_UINT32 nMaxCharsPerResult,
												DECUMA_UNICODE * pChars,
												DECUMA_UINT16 nChars);
#endif

/******* Debugging functions *******/

DECUMA_HWR_PRIVATE DECUMA_STATUS t9wlStartLogging(T9WL_SESSION         * pSession,
												void                   * pUserData,
												DECUMA_LOG_STRING_FUNC * pfLogStringFunction);

DECUMA_HWR_PRIVATE DECUMA_STATUS t9wlStopLogging(T9WL_SESSION * pSession);

DECUMA_HWR_PRIVATE DECUMA_STATUS t9wlLogAcceptedResult(T9WL_SESSION       * pSession,
													 const DECUMA_UNICODE * pResult,
													 DECUMA_UINT32          nResultLength);
/******* Extra *******/


/*Sets the result segmentation to be forced to pForceSymbolStrokes */
DECUMA_HWR_PRIVATE DECUMA_STATUS t9wlSetForcedSegmentation( T9WL_SESSION * pSession,
	DECUMA_INT16 * pForceSymbolStrokes,
	DECUMA_INT16 nForceSymbols);

/*Returns the result segmentation to be forced or zero if no force */
DECUMA_HWR_PRIVATE const DECUMA_INT16 * t9wlGetForcedSegmentation(T9WL_SESSION * pSession, DECUMA_INT16 * pnForceSymbols);

/*symbolString will be considered not part of dictionary regardless of wether */
/*it exists in used dictionaries or not. */
DECUMA_HWR_PRIVATE DECUMA_STATUS t9wlSetDictionaryFilterString(T9WL_SESSION * pSession, const DECUMA_UNICODE * symbolString);

/*Returns the dictionary filter string. */
DECUMA_HWR_PRIVATE DECUMA_UNICODE * t9wlGetDictionaryFilterString(T9WL_SESSION * pSession);

/*If no more arcs are going to be added before t9wlRecognize is called it is a bit */
/*more optimal to call this function before t9wlEvaluateArcs is called, otherwise */
/*it is much more optimal to not call it. */
DECUMA_HWR_PRIVATE DECUMA_STATUS t9wlConfirmArcs( T9WL_SESSION * pSession);

/*Only useful if pre recognition has been set to off, i.e. when evaluation */
/*is not done immediately in t9wlAddArc. */
DECUMA_HWR_PRIVATE DECUMA_STATUS t9wlEvaluateArcs( T9WL_SESSION * pSession);

DECUMA_HWR_PRIVATE DECUMA_STATUS t9wlRecognizeFast( T9WL_SESSION * pSession,
	DECUMA_HWR_RESULT * pResult,
	DECUMA_UINT16 * pnResults,  /* In most cases == 1, but can be 0 */
	DECUMA_UINT16 nMaxChars,
	const DECUMA_INTERRUPT_FUNCTIONS * pInterruptFunctions);

/*Takes the added arcs and returns the distance for these arcs to */
/*the symbol string provided here. If there is no match for the symbol */
/*string a max distance is returned. */
/*Note that the session settings and recognition settings are used */
/*for the interpretation */
/* */
/* pSession - The session object */
/* symbolString - Zero-terminated string to match */
/*                The symbol string can not be longer than */
/*                MAX_GET_DIST_SYMBOL_LEN (plus terminating zero) */
/* pDistance - Pointer where the distance will be returned */
/* pStringType - Pointer to where the string type will be returned */
/* bRedoPreRecognitionAfterCall - */
DECUMA_HWR_PRIVATE DECUMA_STATUS t9wlGetDistance( T9WL_SESSION * pSession,
	const DECUMA_UNICODE * symbolString,
	const DECUMA_RECOGNITION_SETTINGS * pRecognitionSettings,
	DECUMA_UINT32 * pDistance,
	DECUMA_STRING_TYPE * pStringType);

/* This function behaves the same way as t9wlRecognize (bFast = 0) or */
/* t9wlRecognizeFast (bFast != 0) except that it, forces the recognition */
/* to symbolString. If no such forced recognition candidate is found this */
/* function will return 0 candidates. */
DECUMA_HWR_PRIVATE DECUMA_STATUS t9wlRecognizeForceRecognition( T9WL_SESSION * pSession,
	const DECUMA_UNICODE * symbolString,
	DECUMA_HWR_RESULT * pResults,
	DECUMA_UINT16 nMaxResults,
	DECUMA_UINT16 * pnResults,
	DECUMA_UINT16 nMaxCharsPerResult,
	const DECUMA_RECOGNITION_SETTINGS * pRecognitionSettings,
	int bFast,
	const DECUMA_INTERRUPT_FUNCTIONS * pInterruptFunctions);

/* This function behaves the same way as t9wlRecognize (bFast = 0) or */
/* t9wlRecognizeFast (bFast != 0) except that it, forces the segmentation */
/* to the segmentation of the best candidate of a forced recognition of */
/* symbolString. If no such forced recognition candidate is found this */
/* function will return 0 candidates. */
DECUMA_HWR_PRIVATE DECUMA_STATUS t9wlRecognizeForceSegmentation( T9WL_SESSION * pSession,
	const DECUMA_UNICODE * symbolString,
	DECUMA_HWR_RESULT * pResults,
	DECUMA_UINT16 nMaxResults,
	DECUMA_UINT16 * pnResults,
	DECUMA_UINT16 nMaxCharsPerResult,
	const DECUMA_RECOGNITION_SETTINGS * pRecognitionSettings,
	int bFast,
	const DECUMA_INTERRUPT_FUNCTIONS * pInterruptFunctions);


/* As t9wl recognize but gives extra data about the output */
/* */
/* pCharArcIdxData - NULL or Should point to allocated memory of the size given */
/*                    by t9wlGetCharArcIdxDataSize(). This memory will be filled */
/*                    with data about which arc that has been used to recognize */
/*                    each of the characters in each of the returned candidates */
DECUMA_HWR_PRIVATE DECUMA_STATUS t9wlRecognizeExtraOutput( T9WL_SESSION * pSession,
	DECUMA_HWR_RESULT * pResults,
	DECUMA_UINT16 nMaxResults,
	DECUMA_UINT16 * pnResults,
	DECUMA_UINT16 nMaxCharsPerResult,
	void *  pCharArcIdxData, /*Should point to an array of size nMaxResults * nMaxCharsPerResult * */
	const DECUMA_RECOGNITION_SETTINGS * pRecognitionSettings,
	const DECUMA_INTERRUPT_FUNCTIONS * pInterruptFunctions);

/* Returns the size of char-arcIdx-data */
DECUMA_HWR_PRIVATE int t9wlGetCharArcIdxDataSize( DECUMA_UINT16 nMaxResults, DECUMA_UINT16 nMaxCharsPerResult);

/* Returns the the arcs (as indices) that were used for a certain character of a certain */
/* recognition candidate. */
/* */
/* pCharArcIdxData - Pointer to the memory that has been filled with data during the */
/*                    t9wlRecognizeExtraOutput() call */
/* nResultIdx       - Index of the result (candidate) returned by the recognition */
/* nCharIdx         - Index of the recognized character within the candidate that you will */
/*                    get the associated arcs for. */
/* pnArcs        - Will be set to the number of arcs used for the character */
/* */
/* Returns a pointer to an array with *pnArcs. The array contians the indices for the arcs */
/* used to recognized the character. */
DECUMA_HWR_PRIVATE const DECUMA_UINT16 * t9wlGetCharArcIdxs( const void * pCharArcIdxData,
														 DECUMA_UINT16 nResultIdx, DECUMA_UINT16 nMaxResults,
														 DECUMA_UINT16 nCharIdx, DECUMA_UINT16 nMaxCharsPerResult,
														 DECUMA_UINT16 * pnArcs);

DECUMA_HWR_PRIVATE long t9wlGetMemoryStatistics(char *pBuf,
							long nBufLen);


DECUMA_HWR_PRIVATE DECUMA_STATUS t9wlUsePositionAndScaleInvariantRecognition( T9WL_SESSION * pSession, int bOnOff);

DECUMA_HWR_PRIVATE DECUMA_STATUS t9wlUseTypeConnectionRequirement( T9WL_SESSION * pSession, int bOnOff);

DECUMA_HWR_PRIVATE DECUMA_STATUS t9wlUseStrictTypeConnectionRequirement( T9WL_SESSION * pSession, int bOnOff);

DECUMA_HWR_PRIVATE DECUMA_STATUS t9wlNoteSelectedCandidate( T9WL_SESSION * pSession, int nResultIdx);

/* For unit tests */
DECUMA_HWR_PRIVATE int t9wlGetArcSessionSegmentCount(T9WL_SESSION * pSession);
DECUMA_HWR_PRIVATE DECUMA_UINT32 t9wlGetMaxNumberOfSegments(void);

/* For profiling */
DECUMA_HWR_PRIVATE DECUMA_UINT32 t9wlGetArcSessionSize(void);
DECUMA_HWR_PRIVATE DECUMA_UINT32 t9wlGetSGSize(void);
DECUMA_HWR_PRIVATE DECUMA_UINT32 t9wlGetRGSize(void);


#ifdef EXTRA_VERIFICATION_INFO
/*This part exposes data pointers that can be used by verifying modules */
/*like wingui */

#include "t9wlSG.h"
#include "t9wlRG.h"
#include "t9wlArcSession.h"

DECUMA_HWR_PRIVATE T9WL_SG * t9wlGetSG(T9WL_SESSION * pSession);
DECUMA_HWR_PRIVATE T9WL_SG * t9wlGetForcedSG(T9WL_SESSION * pSession);

DECUMA_HWR_PRIVATE T9WL_RG * t9wlGetRG(T9WL_SESSION * pSession);
DECUMA_HWR_PRIVATE T9WL_RG * t9wlGetForcedRG(T9WL_SESSION * pSession);

DECUMA_HWR_PRIVATE T9WL_ARC_SESSION * t9wlGetArcSession(T9WL_SESSION * pSession);
#endif


#ifdef __cplusplus
} /* extern "C" { */
#endif

#endif /*T9WL_H */
