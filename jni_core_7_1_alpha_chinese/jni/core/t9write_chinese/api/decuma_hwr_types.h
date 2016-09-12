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

  This file describes the types used by the decuma_hwr.h api.

  See the API Reference PDF for proper documentation of the
  API.

\******************************************************************/

#ifndef DECUMA_HWR_TYPES_H_sdlfiu92dsfaj
#define DECUMA_HWR_TYPES_H_sdlfiu92dsfaj

#include "decuma_point.h"            /* Definition of DECUMA_POINT */
#include "decumaStatus.h"            /* Definition of DECUMA_STATUS */
#include "decumaCurve.h"             /* Definition of DECUMA_CURVE  */
#include "decumaCharacterSetType.h"  /* Definition of DECUMA_CHARACTER_SET */
#include "decumaBasicTypes.h"
#include "decumaStorageSpecifiers.h" /* Definition of DECUMA_DB_STORAGE */
#include "decumaUnicodeTypes.h"
#include "decumaRuntimeMallocData.h" /* Definition of DECUMA_MEM_FUNCTIONS */

/*********************************************
*        Macro Definitions                   *
**********************************************/
#define DECUMA_MAX_CATEGORIES 100

/* Minimum size of the buffer passed to decumaDatabaseGetVersion() */
#define DECUMA_DATABASE_VERSION_STRING_LENGTH 150

/* The maximum distance for a recognition result */
#define DECUMA_MAX_DISTANCE  10000000


/*********************************************
*        Type definitions                    *
**********************************************/

typedef struct _DECUMA_SESSION DECUMA_SESSION;

typedef const struct _DECUMA_STATIC_DB DECUMA_DB_STORAGE *  DECUMA_STATIC_DB_PTR;
typedef const struct _DECUMA_DYNAMIC_DB DECUMA_DB_STORAGE * DECUMA_DYNAMIC_DB_PTR;
typedef const void DECUMA_DB_STORAGE * DECUMA_ANY_DB_PTR; /* Static or dynamic DB */

typedef enum _UI_INPUT_GUIDE
{
	none = 0,
	supportlines,
	box,
	numberOfUIInputGuides
} UI_INPUT_GUIDE;

typedef enum _SUPPORT_LINE_SET
{
	noSupportlines =0,
	toplineOnly,
	baselineOnly,
	helplineOnly,
	baselineAndHelpline,
	baselineAndTopline,
	helplineAndTopline,
	numberOfSupportlineSets
} SUPPORT_LINE_SET;

typedef struct _INPUT_BOX
{
	DECUMA_COORD  xbase; /* Lower left corner of input box (x-coord) */
	DECUMA_COORD  ybase; /* Lower left corner of input box (y-coord) */
	DECUMA_UINT16 height;
	DECUMA_UINT16 width;
} INPUT_BOX;

typedef enum _RECOGNITION_MODE
{
	scrMode =0,
	mcrMode,
	ucrMode,

	numberOfModes /* For internal use */
} RECOGNITION_MODE;

typedef enum _WRITING_DIRECTION
{
	leftToRight = 0,  /* Eg. Latin script */
	rightToLeft,      /* Eg. Arabic script */
	onTopWriting,     /* For writing all characters on the same spot (the same x-mean point is expected) */
	unknownWriting,

	numberOfDirections /* For internal use */
} WRITING_DIRECTION;

typedef struct _DECUMA_SESSION_SETTINGS
{
	DECUMA_STATIC_DB_PTR  pStaticDB;  /* Pre-defined recognition templates */
	DECUMA_DYNAMIC_DB_PTR pDynamicDB; /* User defined recognition templates */

	RECOGNITION_MODE recognitionMode;

	UI_INPUT_GUIDE UIInputGuide;
	SUPPORT_LINE_SET supportLineSet;

	/* For support line mode supportlines */

	/* A maximum of two of the following values will be considered,
	 * depending on the value of supportLineSet
	 */
	DECUMA_INT32 baseline; /* Y-coord */
	DECUMA_INT32 helpline; /* Y-coord */
	DECUMA_INT32 topline;  /* Y-coord */

	/* Or if UIInputGuide is box then */
	INPUT_BOX box;

	WRITING_DIRECTION writingDirection;

	DECUMA_CHARACTER_SET charSet;       /* The active character set */
	DECUMA_UNICODE * pCharSetExtension; /* Extra symbols that should be included in the active character set
	                                     * All templates from pStaticDB with these unicodes will be activated.
	                                     * Symbols should be zero-terminated, and the whole array should be
	                                     * double-zero-terminated [0 0].
	                                     * Can be NULL if no dynamic character set modification is desired. (normal case)
										 */

	int bMinimizeAddArcPreProcessing;

} DECUMA_SESSION_SETTINGS;


typedef enum _BOOST_LEVEL
{
	noBoost = 0,
	boostDictWords,
	forceDictWords,
	boostFrequentChars,

	numberOfDictionaryBoostLevels /* For internal use */
} BOOST_LEVEL;


typedef enum _STRING_COMPLETENESS
{
	canBeContinued = 0,
	willNotBeContinued,

	numberOfCompletenesses /* For internal use */
} STRING_COMPLETENESS;


typedef struct _DECUMA_RECOGNITION_SETTINGS
{
	BOOST_LEVEL boostLevel;
	STRING_COMPLETENESS stringCompleteness;
	DECUMA_UNICODE * pStringStart; /* Zero terminated */

} DECUMA_RECOGNITION_SETTINGS;


typedef enum _DECUMA_STRING_TYPE
{
	notFromDictionary = 0,
	startOfWord,
	completeWord,

	numberOfStringTypes
} DECUMA_STRING_TYPE;


typedef struct _DECUMA_HWR_RESULT
{
	DECUMA_UNICODE * pChars;  /* Zero terminated */
	DECUMA_UINT16 nChars;  /* The number actually written excluding terminating 0 */
	DECUMA_UINT16 nResultingChars;  /* The number of characters in the recognition result
	                                 * != nChars if the array is too small for the chars of the result
									 */
	DECUMA_UINT32 distance; /* Total distance. May include linguistic component. */
	DECUMA_UINT32 shapeDistance; /* Pure shape distance. May not include linguistic component. */

	DECUMA_STRING_TYPE stringType;
	DECUMA_INT16 dictStrStart;    /* Says at which character a potential dictionary word starts, undefined if stringType == notFromDictionary */
	DECUMA_INT16 dictStrLen;      /* Says how many characters a potential dictionary word is, 0 if stringType == notFromDictionary */

	/* Segmentation info */
	DECUMA_INT16 nSymbols; /* Number of symbols written to result. Less than nResultingSymbols if result buffer is too small. */
	DECUMA_INT16 nResultingSymbols; /* Number of symbols in result. One symbol can contain several characters. */
	DECUMA_INT16 * pSymbolChars; /* Number of characters in each symbol. Should add up to nChars. Same buffer size as pChars is assumed. */
	DECUMA_INT16 * pSymbolStrokes; /* Number of strokes in each symbol. Should add up to number of strokes added. Same buffer size as pChars is assumed. */
	DECUMA_UINT8 * pSymbolArcTimelineDiffMask; /* The Arc Timeline difference mask for the symbol. Same buffer size as pChars is assumed. */

	DECUMA_UINT8 bGesture; /*Set to 1 if the last symbol is a gesture */
	DECUMA_UINT8 bInstantGesture; /*Set to 1 if the last symbol is an "instant gesture" */

} DECUMA_HWR_RESULT;


typedef enum _DECUMA_XT9_DICTIONARY_TYPE {
	decumaXT9LDB = 0,
	decumaXT9UDB,

	numberOfXT9DictionaryTypes

} DECUMA_XT9_DICTIONARY_TYPE;

typedef enum _DECUMA_DICTIONARY_TYPE {
	decumaPersonalDictionary = 0,
	decumaUsedWordDictionary,
	decumaStaticDictionary,

	numberOfDictionaryTypes
} DECUMA_DICTIONARY_TYPE;


typedef int (*DECUMA_ABORT_FUNC)(void * pUserData);

typedef struct _DECUMA_INTERRUPT_FUNCTIONS
{
	DECUMA_ABORT_FUNC pShouldAbortRecognize;
	void * pUserData;
} DECUMA_INTERRUPT_FUNCTIONS;


typedef void (DECUMA_LOG_STRING_FUNC)(void * pUserData, const char * pLogString, DECUMA_UINT32 nLogStringLength);

typedef struct _DECUMA_INSTANT_GESTURE_SETTINGS
{
	DECUMA_UINT32 widthThreshold;
	DECUMA_UINT32 heightThreshold;
} DECUMA_INSTANT_GESTURE_SETTINGS;

#endif /* DECUMA_HWR_TYPES_H_sdlfiu92dsfaj */
