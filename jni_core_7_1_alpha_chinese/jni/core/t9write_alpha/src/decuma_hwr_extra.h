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

\******************************************************************/

#ifndef DECUMA_HWR_EXTRA_H
#define DECUMA_HWR_EXTRA_H

#ifdef __cplusplus
extern "C" {
#endif

#include "decumaConfig.h"

#ifndef DECUMA_HWR_ENABLE_EXTRA_API
#error There is no point in including the extra API if it is not enabled
#endif


#ifdef CJK_ENGINE
#include "decuma_hwr_cjk.h"
#define API_FUNC_NAME(func) decumaCJK##func
#else
#include "decuma_hwr.h"
#define API_FUNC_NAME(func) decuma##func
#endif

#define MAX_GET_DIST_SYMBOL_LEN 32

typedef struct DECUMA_COMPLETION_RESULT
{
	DECUMA_UNICODE * pChars;		/* Zero-terminated string */
	DECUMA_UINT16 nChars;			/* The number of bytes actually written to the string */
	DECUMA_UINT16 nResultingChars;	/* The number of characters in the result. If != nChars
									 * the array was too small to contain the entire result
									 */
} DECUMA_COMPLETION_RESULT;


/*****************   FUNCTIONS ***********************/

const char * API_FUNC_NAME(GetEngineVersion)(void);

/*Sets the result segmentation to be forced to pForceSymbolStrokes */
DECUMA_STATUS API_FUNC_NAME(SetForcedSegmentation)( DECUMA_SESSION * pSession,
	DECUMA_INT16 * pForceSymbolStrokes,
	DECUMA_INT16 nForceSymbols);

/*Returns the result segmentation to be forced or zero if no force */
const DECUMA_INT16 * API_FUNC_NAME(GetForcedSegmentation)(DECUMA_SESSION * pSession, DECUMA_INT16 * pnForceSymbols);

/*symbolString will be considered not part of dictionary regardless of wether 
 *it exists in used dictionaries or not. 
 */
DECUMA_STATUS API_FUNC_NAME(SetDictionaryFilterString)(DECUMA_SESSION * pSession,
											  const DECUMA_UNICODE * symbolString);

/*Returns the dictionary filter string. */
DECUMA_UNICODE * API_FUNC_NAME(GetDictionaryFilterString)(DECUMA_SESSION * pSession);

/*If no more arcs are going to be added before decumaRecognize is called it is a bit
 *more optimal to call this function before decumaEvaluateArcs is called, otherwise
 *it is much more optimal to not call it. 
 */
DECUMA_STATUS API_FUNC_NAME(ConfirmArcs)( DECUMA_SESSION * pSession);

/*Only useful if pre recognition has been set to off, i.e. when evaluation
 *is not done immediately in decumaAddArc. 
 */
DECUMA_STATUS API_FUNC_NAME(EvaluateArcs)( DECUMA_SESSION * pSession);

/*Enables switching position and scale invariant recognition on/off 
 *
 * bOnOff - Set to 1 to turn on position and scale invariant recognition (it is default OFF).
 *          Set to 0 to turn position and scale invariant recognition off again 
 */
DECUMA_STATUS API_FUNC_NAME(UsePositionAndScaleInvariantRecognition)( DECUMA_SESSION * pSession, int bOnOff);

/*Enables switching type connection requirement on/off
 *
 * bOnOff - Set to 0 to turn off type connection requirement (it is default ON).
 *          Set to 1 to turn type connection requirement on again 
 */
DECUMA_STATUS API_FUNC_NAME(UseTypeConnectionRequirement)( DECUMA_SESSION * pSession, int bOnOff);

/*Enables switching type connection requirement on/off 
 *
 * bOnOff - Set to 1 to turn on strict type connection requirement (it is default OFF).
 *          Set to 0 to turn strict type connection requirement off again 
 */
DECUMA_STATUS API_FUNC_NAME(UseStrictTypeConnectionRequirement)( DECUMA_SESSION * pSession, int bOnOff);

DECUMA_STATUS API_FUNC_NAME(RecognizeFast)( DECUMA_SESSION * pSession,
	DECUMA_HWR_RESULT * pResult,
	DECUMA_UINT16 * pnResults,  /* In most cases == 1, but can be 0 */
	DECUMA_UINT16 nMaxChars,
	const DECUMA_INTERRUPT_FUNCTIONS * pInterruptFunctions);

/*Takes the added arcs and returns the distance for these arcs to 
 *the symbol string provided here. If there is no match for the symbol 
 *string a max distance is returned. 
 *Note that the session settings and recognition settings are used 
 *for the interpretation 
 * 
 * pSession - The session object 
 * symbolString - Zero-terminated string to match 
 *                The symbol string can not be longer than 
 *                MAX_GET_DIST_SYMBOL_LEN (plus terminating zero) 
 * pDistance - Pointer where the distance will be returned 
 * pStringType - Pointer to where the string type will be returned 
 * bRedoPreRecognitionAfterCall - 
 */
DECUMA_STATUS API_FUNC_NAME(GetDistance)( DECUMA_SESSION * pSession,
	const DECUMA_UNICODE * symbolString,
	const DECUMA_RECOGNITION_SETTINGS * pRecognitionSettings,
	DECUMA_UINT32 * pDistance,
	DECUMA_STRING_TYPE * pStringType);

/* This function behaves the same way as decumaRecognize (bFast = 0) or 
 * decumaRecognizeFast (bFast != 0) except that it, forces the recognition 
 * to symbolString. If no such forced recognition candidate is found this 
 * function will return 0 candidates. 
 */
DECUMA_STATUS API_FUNC_NAME(RecognizeForceRecognition)( DECUMA_SESSION * pSession,
	const DECUMA_UNICODE * symbolString,
	DECUMA_HWR_RESULT * pResults,
	DECUMA_UINT16 nMaxResults,
	DECUMA_UINT16 * pnResults,
	DECUMA_UINT16 nMaxCharsPerResult,
	const DECUMA_RECOGNITION_SETTINGS * pRecognitionSettings,
	int bFast,
	const DECUMA_INTERRUPT_FUNCTIONS * pInterruptFunctions);

/* This function behaves the same way as decumaRecognize (bFast = 0) or 
 * decumaRecognizeFast (bFast != 0) except that it, forces the segmentation 
 * to the segmentation of the best candidate of a forced recognition of 
 * symbolString. If no such forced recognition candidate is found this 
 * function will return 0 candidates. 
 */
DECUMA_STATUS API_FUNC_NAME(RecognizeForceSegmentation)( DECUMA_SESSION * pSession,
	const DECUMA_UNICODE * symbolString,
	DECUMA_HWR_RESULT * pResults,
	DECUMA_UINT16 nMaxResults,
	DECUMA_UINT16 * pnResults,
	DECUMA_UINT16 nMaxCharsPerResult,
	const DECUMA_RECOGNITION_SETTINGS * pRecognitionSettings,
	int bFast,
	const DECUMA_INTERRUPT_FUNCTIONS * pInterruptFunctions);


/* As decuma recognize but gives extra data about the output 
 * 
 * pCharArcIdxData - NULL or Should point to allocated memory of the size given 
 *                    by decumaGetCharArcIdxDataSize(). This memory will be filled 
 *                    with data about which arc that has been used to recognize 
 *                    each of the characters in each of the returned candidates 
 */
DECUMA_STATUS API_FUNC_NAME(RecognizeExtraOutput)( DECUMA_SESSION * pSession,
	DECUMA_HWR_RESULT * pResults,
	DECUMA_UINT16 nMaxResults,
	DECUMA_UINT16 * pnResults,
	DECUMA_UINT16 nMaxCharsPerResult,
	void *  pCharArcIdxData, /*Should point to an array of size nMaxResults * nMaxCharsPerResult * */
	const DECUMA_RECOGNITION_SETTINGS * pRecognitionSettings,
	const DECUMA_INTERRUPT_FUNCTIONS * pInterruptFunctions);

/******* Word completion functions *******/

DECUMA_STATUS API_FUNC_NAME(CompleteWord)(DECUMA_SESSION * pSession,
								DECUMA_COMPLETION_RESULT * pResults,
								DECUMA_UINT32 nMaxResults,
								DECUMA_UINT32 * pnResults,
								DECUMA_UINT32 nMaxCharsPerResult,
								DECUMA_UNICODE * pChars,
								DECUMA_UINT16 nChars);
/**************/

/* Returns the size of char-arcIdx-data */
int API_FUNC_NAME(GetCharArcIdxDataSize)( DECUMA_UINT16 nMaxResults, DECUMA_UINT16 nMaxCharsPerResult);

/* Returns the the arcs (as indices) that were used for a certain character of a certain 
 * recognition candidate. 
 * 
 * pCharArcIdxData - Pointer to the memory that has been filled with data during the 
 *                    decumaRecognizeExtraOutput() call 
 * nResultIdx       - Index of the result (candidate) returned by the recognition 
 * nCharIdx         - Index of the recognized character within the candidate that you will 
 *                    get the associated arcs for. 
 * pnArcs        - Will be set to the number of arcs used for the character 
 * 
 * Returns a pointer to an array with *pnArcs. The array contians the indices for the arcs 
 * used to recognized the character. 
 */
const DECUMA_UINT16 * API_FUNC_NAME(GetCharArcIdxs)( const void * pCharArcIdxData,
														 DECUMA_UINT16 nResultIdx, DECUMA_UINT16 nMaxResults,
														 DECUMA_UINT16 nCharIdx, DECUMA_UINT16 nMaxCharsPerResult,
														 DECUMA_UINT16 * pnArcs);

/* Writes memory statistics to pBuf and returns number of characters written (limited to nBufLen) */
long API_FUNC_NAME(GetMemoryStatistics)(char *pBuf,
							   long nBufLen);
/* For unit testing */
int API_FUNC_NAME(GetArcSessionSegmentCount)(DECUMA_SESSION * pSession);
DECUMA_UINT32 API_FUNC_NAME(GetMaxNumberOfSegments)(void);

/* For profiling */
DECUMA_UINT32 API_FUNC_NAME(GetArcSessionSize)(void);
DECUMA_UINT32 API_FUNC_NAME(GetSGSize)(void);
DECUMA_UINT32 API_FUNC_NAME(GetRGSize)(void);


/* Utility functions for sampling that could later be a part of the official API */
DECUMA_STATUS API_FUNC_NAME(AddPoints)(DECUMA_SESSION * pSession,
								   DECUMA_POINT * pPts,
								   int nPts,
								   DECUMA_UINT32 samplingArcID,
								   int * pnPtsAdded);

DECUMA_STATUS API_FUNC_NAME(GetUncommittedArcCount)(DECUMA_SESSION * pSession, int * pnUncommittedArcs);
DECUMA_STATUS API_FUNC_NAME(GetUncommittedArcID)(DECUMA_SESSION * pSession, int idx,
										DECUMA_UINT32 * pSamplingArcID);


#ifdef EXTRA_VERIFICATION_INFO
void * API_FUNC_NAME(GetLibSession)(DECUMA_SESSION * pSession);
#endif

#ifdef __cplusplus
} /* extern "C" { */
#endif

#endif /*DECUMA_HWR_EXTRA_H */

