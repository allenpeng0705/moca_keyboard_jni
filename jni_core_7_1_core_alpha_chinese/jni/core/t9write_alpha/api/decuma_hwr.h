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

#ifndef DECUMA_HWR_H_12uyoqrt87341
#define DECUMA_HWR_H_12uyoqrt87341

#include "decuma_hwr_types.h"        /* DECUMA_HWR specific type definitions */

/* This symbol may be defined externally. For instance, when using a Windows DLL you may want to define it to "__declspec(dllimport)" */
#ifndef DECUMA_HWR_API
#define DECUMA_HWR_API
#endif

#ifdef __cplusplus
extern "C" {
#endif




/************************************************
*        Function prototypes                    *
************************************************/



/****** Version ********/

/*Returns a zero terminated string */

DECUMA_HWR_API const char * decumaGetEngineVersion(void);
DECUMA_HWR_API const char * decumaGetProductVersion(void);
DECUMA_HWR_API const char * decumaGetAPIVersion(void);

/******* Database functions *********/


DECUMA_HWR_API DECUMA_STATUS decumaDatabaseGetVersion(DECUMA_ANY_DB_PTR pDB, char * pBuf, int nBufLen);

DECUMA_HWR_API DECUMA_STATUS decumaDatabaseIsCategorySupported(DECUMA_ANY_DB_PTR pDB,
	DECUMA_UINT32 cat, int * pbIsSupported);

DECUMA_HWR_API DECUMA_STATUS decumaDatabaseIsLanguageSupported(DECUMA_ANY_DB_PTR pDB,
	DECUMA_UINT32 lang, int * pbIsSupported);

DECUMA_HWR_API DECUMA_STATUS decumaDatabaseIncludesSymbol(
	DECUMA_ANY_DB_PTR pDB,
	const DECUMA_CHARACTER_SET * pCharSet,
	const DECUMA_UNICODE * pSymbol,
	int * pbIsIncluded);

DECUMA_HWR_API DECUMA_STATUS decumaCreateDynamicDatabase(
			DECUMA_DYNAMIC_DB_PTR* ppDynamicDB,
	const DECUMA_MEM_FUNCTIONS * pMemFunctions);

DECUMA_HWR_API DECUMA_STATUS decumaAddAllograph(      DECUMA_DYNAMIC_DB_PTR   * ppDynamicDB,
								 const DECUMA_CURVE         * pCurve,
								 const DECUMA_UNICODE       * pUnicode, DECUMA_UINT32 nUnicodes,
								 const DECUMA_CHARACTER_SET * pCharacterSet,
								 DECUMA_INT32 nBaseline, DECUMA_INT32 nHelpline,
								 int bGesture, int bInstantGesture,
								 const DECUMA_MEM_FUNCTIONS * pMemFunctions);

DECUMA_HWR_API DECUMA_STATUS decumaGetDynamicDatabaseByteSize(DECUMA_DYNAMIC_DB_PTR pDynamicDB,
																						  DECUMA_UINT32 * pSize);

DECUMA_HWR_API DECUMA_STATUS decumaDynamicDatabaseIsValid(DECUMA_DYNAMIC_DB_PTR pDynamicDB);

DECUMA_HWR_API DECUMA_STATUS decumaDestroyDynamicDatabase(DECUMA_DYNAMIC_DB_PTR* ppDynamicDB,
																	 const DECUMA_MEM_FUNCTIONS * pMemFunctions);

/****** Initialization, verification and destruction ********/


DECUMA_HWR_API DECUMA_UINT32 decumaGetSessionSize(void);

DECUMA_HWR_API DECUMA_STATUS decumaBeginSession(DECUMA_SESSION * pSession,
	const DECUMA_SESSION_SETTINGS * pSessionSettings,
	const DECUMA_MEM_FUNCTIONS * pMemoryFunctions);


DECUMA_HWR_API DECUMA_STATUS decumaVerifySession(const DECUMA_SESSION * pSession);


DECUMA_HWR_API DECUMA_STATUS decumaEndSession(DECUMA_SESSION * pSession);





/******* Settings *********/


DECUMA_HWR_API DECUMA_STATUS decumaGetSessionSettings( DECUMA_SESSION * pSession,
	const DECUMA_SESSION_SETTINGS ** pSessionSettings);

DECUMA_HWR_API DECUMA_STATUS decumaChangeSessionSettings( DECUMA_SESSION * pSession,
	const DECUMA_SESSION_SETTINGS * pSessionSettings);




/****** Recognition functions ********/


DECUMA_HWR_API DECUMA_STATUS decumaBeginArcAddition(DECUMA_SESSION * pSession);

DECUMA_HWR_API DECUMA_STATUS decumaStartNewArc(DECUMA_SESSION * pSession, DECUMA_UINT32  arcID);

DECUMA_HWR_API DECUMA_STATUS decumaStartNewSymbol(DECUMA_SESSION * pSession);

DECUMA_HWR_API DECUMA_STATUS decumaAddPoint(DECUMA_SESSION * pSession,
							 DECUMA_COORD x, DECUMA_COORD y,
							 DECUMA_UINT32 arcID);

DECUMA_HWR_API DECUMA_STATUS decumaCancelArc(DECUMA_SESSION * pSession,DECUMA_UINT32 arcID);

DECUMA_HWR_API DECUMA_STATUS decumaCommitArc(DECUMA_SESSION * pSession,DECUMA_UINT32 arcID);

DECUMA_HWR_API DECUMA_STATUS decumaRecognize( DECUMA_SESSION * pSession,
	DECUMA_HWR_RESULT * pResults,
	DECUMA_UINT16 nMaxResults,
	DECUMA_UINT16 * pnResults,
	DECUMA_UINT16 nMaxCharsPerResult,
	const DECUMA_RECOGNITION_SETTINGS * pRecognitionSettings,
	const DECUMA_INTERRUPT_FUNCTIONS * pInterruptFunctions);


DECUMA_HWR_API DECUMA_STATUS decumaIndicateInstantGesture(
	DECUMA_SESSION * pSession,
	int * pbInstantGesture,
	const DECUMA_INSTANT_GESTURE_SETTINGS * pInstantGestureSettings);

DECUMA_HWR_API DECUMA_STATUS decumaEndArcAddition(DECUMA_SESSION * pSession);

DECUMA_HWR_API DECUMA_STATUS decumaNoteSelectedCandidate( DECUMA_SESSION * pSession, int nResultIdx);

/******* Dictionary conversion functions *******/

DECUMA_HWR_API DECUMA_STATUS decumaUnpackXT9Dictionary(void ** ppDictionaryData,
	const void * pXT9Dictionary, DECUMA_UINT32 xt9DataSize, DECUMA_XT9_DICTIONARY_TYPE xt9Type,
	DECUMA_UINT32 * pUnpackedDataSize,const DECUMA_MEM_FUNCTIONS * pMemoryFunctions);

DECUMA_HWR_API DECUMA_STATUS decumaDestroyUnpackedDictionary(void ** ppDictionaryData,
	const DECUMA_MEM_FUNCTIONS * pMemoryFunctions);

/******* Dictionary attachment functions *******/

DECUMA_HWR_API DECUMA_STATUS decumaAttachStaticDictionary(DECUMA_SESSION * pSession,
	const void * pDictionaryData);

DECUMA_HWR_API DECUMA_STATUS decumaDetachStaticDictionary(DECUMA_SESSION * pSession,
	const void * pDictionaryData);


/******* Dynamic Dictionary functions *******/
DECUMA_HWR_API DECUMA_STATUS decumaAttachDynamicDictionary(DECUMA_SESSION * pSession,
														   DECUMA_DICTIONARY_TYPE type,
														   void * pDictionaryData,
														   int cbDictionaryDataSize);

DECUMA_HWR_API DECUMA_STATUS decumaDetachDynamicDictionary(DECUMA_SESSION * pSession,
														   const void * pDictionaryData);

DECUMA_HWR_API DECUMA_STATUS decumaGetDynamicDictionarySize(DECUMA_SESSION * pSession,
															const void * pDictionaryData,
															int * pPDSize);

DECUMA_HWR_API DECUMA_STATUS decumaAddWordToDictionary(DECUMA_SESSION * pSession,
													   void * pDictionaryData,
													   DECUMA_UNICODE * pWord);

DECUMA_HWR_API DECUMA_STATUS decumaWordExistsInDictionary(DECUMA_SESSION * pSession,
														  const void * pDictionaryData,
														  DECUMA_UNICODE * pWord);

DECUMA_HWR_API DECUMA_STATUS decumaAddAllWords(DECUMA_SESSION * pSession,
											   void * pDictionaryBuffer,
											   const DECUMA_UNICODE * pWordBufferStart,
											   int nUnicodesInBuffer,
											   int bNullDelimitedOnly);

/******* Debugging functions *******/

DECUMA_HWR_API DECUMA_STATUS decumaStartLogging(DECUMA_SESSION         * pSession,
												void                   * pUserData,
												DECUMA_LOG_STRING_FUNC * pfLogStringFunction);

DECUMA_HWR_API DECUMA_STATUS decumaStopLogging(DECUMA_SESSION * pSession);

DECUMA_HWR_API DECUMA_STATUS decumaLogAcceptedResult(DECUMA_SESSION       * pSession,
													 const DECUMA_UNICODE * pResult,
													 DECUMA_UINT32          nResultLength);

#ifdef __cplusplus
} /* extern "C" { */
#endif

#endif /*DECUMA_HWR_H_12uyoqrt8734 */
