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

#ifndef DECUMA_HWR_CJK_H_as267uadgf38
#define DECUMA_HWR_CJK_H_as267uadgf38

#include "decuma_hwr_types.h"        /* DECUMA_HWR specific type definitions */

/* This symbol may be defined externally. For instance, when using a Windows DLL you may want to define it to "__declspec(dllimport)" */
#ifndef DECUMA_HWR_CJK_API
#define DECUMA_HWR_CJK_API
#endif

#ifdef __cplusplus
extern "C" {
#endif




/************************************************
*        Function prototypes                    *
************************************************/



/****** Version ********/

/*Returns a zero terminated string */

DECUMA_HWR_CJK_API const char * decumaCJKGetProductVersion(void);
DECUMA_HWR_CJK_API const char * decumaCJKGetAPIVersion(void);

/******* Database functions *********/


DECUMA_HWR_CJK_API DECUMA_STATUS decumaCJKDatabaseGetVersion(DECUMA_ANY_DB_PTR pDB, char * pBuf, int nBufLen);

DECUMA_HWR_CJK_API DECUMA_STATUS decumaCJKDatabaseIsCategorySupported(DECUMA_ANY_DB_PTR pDB,
	DECUMA_UINT32 cat, int * pbIsSupported);

DECUMA_HWR_CJK_API DECUMA_STATUS decumaCJKDatabaseIsLanguageSupported(DECUMA_ANY_DB_PTR pDB,
	DECUMA_UINT32 lang, int * pbIsSupported);

DECUMA_HWR_CJK_API DECUMA_STATUS decumaCJKDatabaseIncludesSymbol(
	DECUMA_ANY_DB_PTR pDB,
	const DECUMA_CHARACTER_SET * pCharSet,
	const DECUMA_UNICODE * pSymbol,
	int * pbIsIncluded);

DECUMA_HWR_CJK_API DECUMA_STATUS decumaCJKCreateDynamicDatabase(
			DECUMA_DYNAMIC_DB_PTR* ppDynamicDB,
	const DECUMA_MEM_FUNCTIONS * pMemFunctions);

DECUMA_HWR_CJK_API DECUMA_STATUS decumaCJKAddAllograph(      DECUMA_DYNAMIC_DB_PTR   * ppDynamicDB,
								 const DECUMA_CURVE         * pCurve,
								 const DECUMA_UNICODE       * pUnicode, DECUMA_UINT32 nUnicodes,
								 const DECUMA_CHARACTER_SET * pCharacterSet,
								 DECUMA_INT32 nBaseline, DECUMA_INT32 nHelpline,
								 int bGesture, int bInstantGesture,
								 const DECUMA_MEM_FUNCTIONS * pMemFunctions);

DECUMA_HWR_CJK_API DECUMA_STATUS decumaCJKGetDynamicDatabaseByteSize(DECUMA_DYNAMIC_DB_PTR pDynamicDB,
																						  DECUMA_UINT32 * pSize);

DECUMA_HWR_CJK_API DECUMA_STATUS decumaCJKDynamicDatabaseIsValid(DECUMA_DYNAMIC_DB_PTR pDynamicDB);

DECUMA_HWR_CJK_API DECUMA_STATUS decumaCJKDestroyDynamicDatabase(DECUMA_DYNAMIC_DB_PTR* ppDynamicDB,
																	 const DECUMA_MEM_FUNCTIONS * pMemFunctions);

/****** Initialization, verification and destruction ********/


DECUMA_HWR_CJK_API DECUMA_UINT32 decumaCJKGetSessionSize(void);

DECUMA_HWR_CJK_API DECUMA_STATUS decumaCJKBeginSession(DECUMA_SESSION * pSession,
	const DECUMA_SESSION_SETTINGS * pSessionSettings,
	const DECUMA_MEM_FUNCTIONS * pMemoryFunctions);


DECUMA_HWR_CJK_API DECUMA_STATUS decumaCJKVerifySession(const DECUMA_SESSION * pSession);


DECUMA_HWR_CJK_API DECUMA_STATUS decumaCJKEndSession(DECUMA_SESSION * pSession);





/******* Settings *********/


DECUMA_HWR_CJK_API DECUMA_STATUS decumaCJKGetSessionSettings( DECUMA_SESSION * pSession,
	const DECUMA_SESSION_SETTINGS ** pSessionSettings);

DECUMA_HWR_CJK_API DECUMA_STATUS decumaCJKChangeSessionSettings( DECUMA_SESSION * pSession,
	const DECUMA_SESSION_SETTINGS * pSessionSettings);




/****** Recognition functions ********/


DECUMA_HWR_CJK_API DECUMA_STATUS decumaCJKBeginArcAddition(DECUMA_SESSION * pSession);

DECUMA_HWR_CJK_API DECUMA_STATUS decumaCJKStartNewArc(DECUMA_SESSION * pSession, DECUMA_UINT32  arcID);

DECUMA_HWR_CJK_API DECUMA_STATUS decumaCJKStartNewSymbol(DECUMA_SESSION * pSession);

DECUMA_HWR_CJK_API DECUMA_STATUS decumaCJKAddPoint(DECUMA_SESSION * pSession,
							 DECUMA_COORD x, DECUMA_COORD y,
							 DECUMA_UINT32 arcID);

DECUMA_HWR_CJK_API DECUMA_STATUS decumaCJKCancelArc(DECUMA_SESSION * pSession,DECUMA_UINT32 arcID);

DECUMA_HWR_CJK_API DECUMA_STATUS decumaCJKCommitArc(DECUMA_SESSION * pSession,DECUMA_UINT32 arcID);

DECUMA_HWR_CJK_API DECUMA_STATUS decumaCJKRecognize( DECUMA_SESSION * pSession,
	DECUMA_HWR_RESULT * pResults,
	DECUMA_UINT16 nMaxResults,
	DECUMA_UINT16 * pnResults,
	DECUMA_UINT16 nMaxCharsPerResult,
	const DECUMA_RECOGNITION_SETTINGS * pRecognitionSettings,
	const DECUMA_INTERRUPT_FUNCTIONS * pInterruptFunctions);


DECUMA_HWR_CJK_API DECUMA_STATUS decumaCJKIndicateInstantGesture(
	DECUMA_SESSION * pSession,
	int * pbInstantGesture,
	const DECUMA_INSTANT_GESTURE_SETTINGS * pInstantGestureSettings);

DECUMA_HWR_CJK_API DECUMA_STATUS decumaCJKEndArcAddition(DECUMA_SESSION * pSession);

DECUMA_HWR_CJK_API DECUMA_STATUS decumaCJKNoteSelectedCandidate( DECUMA_SESSION * pSession, int nResultIdx);

/******* Dictionary conversion functions *******/

DECUMA_HWR_CJK_API DECUMA_STATUS decumaCJKUnpackXT9Dictionary(void ** ppDictionaryData,
	const void * pXT9Dictionary, DECUMA_UINT32 xt9DataSize, DECUMA_XT9_DICTIONARY_TYPE xt9Type,
	DECUMA_UINT32 * pUnpackedDataSize,const DECUMA_MEM_FUNCTIONS * pMemoryFunctions);

DECUMA_HWR_CJK_API DECUMA_STATUS decumaCJKDestroyUnpackedDictionary(void ** ppDictionaryData,
	const DECUMA_MEM_FUNCTIONS * pMemoryFunctions);

/******* Dictionary attachment functions *******/

DECUMA_HWR_CJK_API DECUMA_STATUS decumaCJKAttachStaticDictionary(DECUMA_SESSION * pSession,
	const void * pDictionaryData);

DECUMA_HWR_CJK_API DECUMA_STATUS decumaCJKDetachStaticDictionary(DECUMA_SESSION * pSession,
	const void * pDictionaryData);


/******* Dynamic Dictionary functions *******/
DECUMA_HWR_CJK_API DECUMA_STATUS decumaCJKAttachDynamicDictionary(DECUMA_SESSION * pSession,
														   DECUMA_DICTIONARY_TYPE type,
														   void * pDictionaryData,
														   int cbDictionaryDataSize);

DECUMA_HWR_CJK_API DECUMA_STATUS decumaCJKDetachDynamicDictionary(DECUMA_SESSION * pSession,
														   const void * pDictionaryData);

DECUMA_HWR_CJK_API DECUMA_STATUS decumaCJKGetDynamicDictionarySize(DECUMA_SESSION * pSession,
															const void * pDictionaryData,
															int * pPDSize);

DECUMA_HWR_CJK_API DECUMA_STATUS decumaCJKAddWordToDictionary(DECUMA_SESSION * pSession,
													   void * pDictionaryData,
													   DECUMA_UNICODE * pWord);

DECUMA_HWR_CJK_API DECUMA_STATUS decumaCJKWordExistsInDictionary(DECUMA_SESSION * pSession,
														  const void * pDictionaryData,
														  DECUMA_UNICODE * pWord);

DECUMA_HWR_CJK_API DECUMA_STATUS decumaCJKAddAllWords(DECUMA_SESSION * pSession,
											   void * pDictionaryBuffer,
											   const DECUMA_UNICODE * pWordBufferStart,
											   int nUnicodesInBuffer,
											   int bNullDelimitedOnly);

/******* Debugging functions *******/

DECUMA_HWR_CJK_API DECUMA_STATUS decumaCJKStartLogging(DECUMA_SESSION         * pSession,
												void                   * pUserData,
												DECUMA_LOG_STRING_FUNC * pfLogStringFunction);

DECUMA_HWR_CJK_API DECUMA_STATUS decumaCJKStopLogging(DECUMA_SESSION * pSession);

DECUMA_HWR_CJK_API DECUMA_STATUS decumaCJKLogAcceptedResult(DECUMA_SESSION       * pSession,
													 const DECUMA_UNICODE * pResult,
													 DECUMA_UINT32          nResultLength);

#ifdef __cplusplus
} /* extern "C" { */
#endif

#endif /*DECUMA_HWR_CJK_H_as267uadgf38 */
