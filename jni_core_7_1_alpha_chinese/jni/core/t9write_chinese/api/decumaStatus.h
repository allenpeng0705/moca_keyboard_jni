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


#ifndef __DECUMA_STATUS_INCLUDED_H__
#define __DECUMA_STATUS_INCLUDED_H__

typedef enum _tagDECUMA_STATUS
{
	decumaNoError = 0,              /* No error */

	/*//// General Errors /////// */

	decumaSessionNotInitialized=1,  /* The session was un initialized */

	decumaNullSessionPointer=2,     /* The session pointer is NULL. */
	decumaNullDatabasePointer=3,      /* The static database pointer is NULL */
	decumaNullSettingsPointer=4,      /* The settings pointer is NULL. */
	decumaNullResultPointer=5,        /* The result pointer is NULL. */
	decumaNullCurvePointer=6,         /* A curve pointer is NULL. */
	decumaNullArcPointer=7,           /* An arc pointer is NULL. */
	decumaNullPointPointer=8,         /* A pointer to coordinate points is NULL. */
	decumaNullTextPointer=9,          /* One or several of the text pointers is NULL. */
	decumaNullPointer=10,          	 /* A NULL pointer exists which is not allowed */

	decumaInvalidDatabase=11,         /* The supplied static database is invalid */
	decumaInvalidUserDatabase=12,	    /* The supplied user database is invalid */
	decumaInvalidCategory=13,			 /* The supplied symbol category is invalid */

	decumaArcZeroPoints=14,           /* The number of supplied points in an arc is zero */
	decumaArcTooManyPoints=15,        /* Too many points supplied for an arc */
	decumaCurveZeroArcs=16,           /* The number of supplied arcs in a curve is zero */
	decumaCurveTooManyArcs=17,        /* Too many arcs supplied for a curve */
	decumaInvalidCoordinates=18,      /* The submitted curve/arc contains at least one */
											    /* coordinates that exceeds the coordinate limitation */

	decumaTooShortBuffer=19,          /* A supplied buffer is too short */
	decumaEmptyString=20,				 /* A supplied string is empty ("") but it is not allowed to */

	decumaFunctionNotSupported=21,    /* The called function is not supported for this library */

	decumaAllocationFailed=22,        /* An allocation failed (not enough memory) */
	decumaInvalidIndex=23,            /* An invalid index has been passed as an argument */

	decumaNullMemoryBufferPointer=24, /* A pointer to a memory buffer is NULL */
	decumaNullDistPointer=25, 		    /* The distance pointer is NULL */

	decumaInvalidLanguage=26, 		    /* The supplied language is invalid */

	decumaIllegalFunctionCall=27,	    /* Function was not allowed to be called at the time of the call */
	decumaInvalidEnumeratorValue=28,  /* An enumerator value was out of range */
	decumaSessionAlreadyInitialized=29,  /* The session was already initialized */

	decumaInvalidDictionary=30,       /* An invalid dictinary has been supplied */

	decumaNoSymbolCategories=31,      /* The supplied character set contains no symbol categories */
	decumaNoLanguages=32,             /* The supplied character set contains no languages */
	decumaTooLongString=33,           /*A supplied string is longer than allowed. */

	decumaInputBufferFull=34,         /* The supplied arcs contain too much data for the engine to handle and cannot be added */

	decumaSessionCorrupt=35,          /* The session was corrupted during a previously failed call */

	decumaLoggingNotStarted=36,       /* Logging has not been started */
	decumaInvalidLogState=37,         /* The log state does not permit the taken action */

	decumaUnsupportedParameterValue=40, /* One of the parameters has a value that is generally valid, but not supported by  */
	                                    /* this library. */

	decumaArcTooLong=49,              /* The (arc) length of the supplied arc is too long */
	decumaZeroPoints=50,              /* The number of supplied points is zero */
	decumaInvalidArcID=51,            /* An invalid arcID is provided */
	decumaTooManyConcurrentArcs=52,   /* The number of concurrent arcs is too high for the module */
	decumaUncommittedArcExists=53,    /* A call has invalidly been made to the engine when the sampling of arcs is still ongoing. */

	decumaDatabaseAlreadyCreated=54,  /* A call has been made trying to create a database with a pointer that already points to valid database */

	decumaZeroLengthBuffer=60,        /* A buffer of zero length has been provided when not valid */

	/*//// Build error ///// */
	decumaCompilationError=70,        /* The compilation of the engine was incorrect for this platform */

	/*//// For Decuma HWR API /////// */

	decumaInvalidSupportLines=100,            /* The support line values are invalid. Keep in mind the inverted y-axis */
	                                          /* which means that baseline >= helpline >= topline	 */
	decumaUnsupportedSupportLines=101,        /* The support line configuration is not supported by this library */

	decumaInvalidRecognitionMode=102,         /* The value of the recognition mode parameter is out of range */
	decumaUnsupportedRecognitionMode=103,     /* The specified recognition mode is not supported by this library */

	decumaInvalidWritingDirection=104,        /* The value of the writing direction parameter is out of range */
	decumaUnsupportedWritingDirection=105,    /* The specified writing direction is not supported by this library */

	decumaTooManySymbolCategories=108,        /* Too many symbol categories are specified in the character set */
	decumaTooManyLanguages=109,               /* Too many languages are specified in the character set */

	decumaNoDictionary=110,                   /* Trying to force the use of a dictionary when  */
	                                          /* no dictionary has been supplied */

	decumaFunctionOutsideArcAdditionSeq=111,  /* The function needs to be called within an arc-addition-sequence */
	decumaFunctionInsideArcAdditionSeq=112,   /* The function can not be called within an arc-addition-sequence */

	decumaArcAdditionSeqAlreadyStarted=113,   /* The arc-addition-sequence is already started */
	decumaArcAdditionSeqNotStarted=114,       /* An arc-addition-sequence has not been started */

	decumaRecognitionSettingsUndefined=115,    /* The recognition settings need to be set before calling this function */

	decumaAlreadyAttached=116,				   /* Adding a dictionary already attached to the session */
	decumaNeverAttached=117,                   /* Manipulating or removing a dictionary that is not attached to the session */

	decumaNullDictionaryPointer=119,           /* A dictionary pointer is NULL when it shouldn't be */

	decumaNullMemoryFunctions=120,             /* The pointer to memory functions is NULL when it shouldn't be */

	decumaInvalidUIInputGuide=121,		   /* The value of the UIInputGuide is out of range */

	decumaNullFunctionPointer=122,			   /* A supplied function pointer was NULL */

	decumaRecognitionAborted=123,		   /* The recognition process was aborted by the interrupt function callback */
	decumaAbortRecognitionUnsupported=124,	   /* There is no support for aborting the current recognition function with an interrupt function callback */

	/*//// For Decuma Alphabetic Engine /////// */

	decumaInvalidRefAngle=200,            /* The reference angle is invalid. */
	decumaInvalidRefAngleImportance=201,  /* The refangleimportance value is invalid. */
	decumaInvalidBaselineHelpline=202,    /* The baseline and helpline values are invalid */
											        /* The baseline value needs to be higher than the */
                                         /* helpline value, because of the inverted y-axis */
	decumaInvalidCategoryTable=203,       /* The supplied category table is invalid */

	decumaUnsupportedSymbolCategory=204, /* A supplied symbol category is not supported by the database/category table */
	decumaUnsupportedLanguage=205,       /* A supplied language is not supported by the database/category table */

	/*//// For Alphabetic MCR /////// */


	decumaInvalidBufferType=300,        /* An invalid buffer type which has none of the allowed */
                                       /* values has been passed as an argument */


	/*//// For Decuma Categories (Personal Category Table, and category table of User database) /////// */

	decumaCategoryTableNotInitialized=400, /* The category table has not been initialized (dcInit) */
	decumaSymbolNotInDatabase=401,         /* One or several of the supplied symbols are not found in */
	                                       /* the static database. */
	decumaInvalidCategoryMask=402,         /* A supplied category mask is reserved or zero. */

	decumaTooManyCategoriesForTable=403,   /* A new symbol category can not be added to a table that already  */
	                                       /* contains the maximum amount of symbol categories. */
	decumaTooManyLanguagesForTable=404,    /* A new language can not be added to a table that already contains  */
	                                       /* the maximum amount of languages. */




	/*//// For Chinese/Japanese Engine /////// */


	decumaTooSmallPecMem=1000,               /* The memory supplied for personal characters is too small */
	decumaUnicodeNotInDatabase=1001,         /* The supplied Unicode can not be found in the database */
	decumaInvalidDatabaseVersion=1002,       /* The supplied database Version is not compatible with engine */
	decumaInvalidHeapSize=1003,              /* The supplied database is configured for a maximum heap size that is larger than what the engine supports */
	decumaNullPecMemPointer=1004,            /* The memory supplied for personal characters is too small */


	/*//// For eZiText ////// */

	decumaEziTextInvalidInitialization=2000,  /* The initialization of the eZiText engine failed */
	decumaEziTextInvalidLanguage=2001 			/* The character set is not valid for use with the eZiText predictive functions */

} DECUMA_STATUS;


#endif /*__DECUMA_STATUS_INCLUDED_H__ */

