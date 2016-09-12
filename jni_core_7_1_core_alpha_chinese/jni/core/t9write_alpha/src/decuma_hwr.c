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

#define DECUMA_HWR_C

#include "decumaConfig.h"

#ifdef CJK_ENGINE
#include "decuma_hwr_cjk.h"
#define API_FUNC_NAME(func) decumaCJK##func
#define API_DECL_NAME DECUMA_HWR_CJK_API
#else
#include "decuma_hwr.h"
#define API_FUNC_NAME(func) decuma##func
#define API_DECL_NAME DECUMA_HWR_API
#endif

#include "t9write_api_version.h"

#include "decuma_hwrData.h"

#include "decumaAssert.h"
#include "decumaMemory.h"
#include "decumaCommon.h"
#include "decumaString.h"
#include "decumaRuntimeMalloc.h"
#include "decumaStorageSpecifiers.h" /* Definition of DECUMA_SPECIFIC_DB_STORAGE */

#ifdef UCR_ENGINE
#define LIBFUNC(func) ucr##func
#include "ucr.h"
#elif defined(ASL_ENGINE)
#define LIBFUNC(func) asl##func
#include "asl.h"
#include "t9write_alpha_version.h"
#define T9WRITEPRODUCTCOREVER T9WRITEALPHACOREVER
#elif defined(CJK_ENGINE)
#define LIBFUNC(func) cjk##func
#include "cjk.h"
#include "cjkDynamicDatabase.h"
#include "t9write_cjk_version.h"
#define T9WRITEPRODUCTCOREVER T9WRITECJKCOREVER
#elif defined(T9WL_ENGINE)
#define LIBFUNC(func) t9wl##func
#include "t9wl.h"
#include "t9write_alpha_version.h" /* Configuration of alphabetic. Shares version number. */
#define T9WRITEPRODUCTCOREVER T9WRITEALPHACOREVER /* Configuration of alphabetic. Shares version number. */
#else
#error Need to define one of the engines above
#endif

/*#define DEBUG_OUTPUT */
#ifdef DEBUG_OUTPUT
#include <stdio.h>
FILE * g_debugFile;
#endif

/* */
/* Private macro definitions */
/* */
#define INT32_STRING_MAX_LEN 12 /* The maximum length of the string representation of an DECUMA_INT32 in base 10 */
#define SESSION_NOT_INITIALIZED(pSession) ((pSession)->pLibSession != (LIB_SESSION *)&(pSession)[1])


#define LOG_STRING(pSession, pString)                (pSession)->logData.pfLogStringFunction((pSession)->logData.pUserData, (pString), decumaStrlenUTF8(DECUMA_CAST(DECUMA_UINT8 *, (pString))));
#define LOG_STRING_LITERAL(pSession, pStringLiteral) (pSession)->logData.pfLogStringFunction((pSession)->logData.pUserData, (pStringLiteral), sizeof(pStringLiteral) - 1);
#define LOGGING_ENABLED(pSession)                   ((pSession)->logData.pfLogStringFunction != NULL)
#define LOG_SESSION_SETTINGS(pSession) \
	{ \
		char pBuf[INT32_STRING_MAX_LEN]; \
		int i; \
		LOG_STRING_LITERAL(pSession, "S| SESSION_SETTINGS { "); \
		decumaIToA(pSession->pSessionSettings->recognitionMode, 10, pBuf, INT32_STRING_MAX_LEN); \
		LOG_STRING(pSession, pBuf); \
		LOG_STRING_LITERAL(pSession, ", "); \
		decumaIToA(pSession->pSessionSettings->supportLineSet, 10, pBuf, INT32_STRING_MAX_LEN); \
		LOG_STRING(pSession, pBuf); \
		LOG_STRING_LITERAL(pSession, ", "); \
		decumaIToA(pSession->pSessionSettings->baseline, 10, pBuf, INT32_STRING_MAX_LEN); \
		LOG_STRING(pSession, pBuf); \
		LOG_STRING_LITERAL(pSession, ", "); \
		decumaIToA(pSession->pSessionSettings->helpline, 10, pBuf, INT32_STRING_MAX_LEN); \
		LOG_STRING(pSession, pBuf); \
		LOG_STRING_LITERAL(pSession, ", "); \
		decumaIToA(pSession->pSessionSettings->topline, 10, pBuf, INT32_STRING_MAX_LEN); \
		LOG_STRING(pSession, pBuf); \
		LOG_STRING_LITERAL(pSession, ", "); \
		decumaIToA(pSession->pSessionSettings->writingDirection, 10, pBuf, INT32_STRING_MAX_LEN); \
		LOG_STRING(pSession, pBuf); \
		LOG_STRING_LITERAL(pSession, ", "); \
		decumaIToA(pSession->pSessionSettings->bMinimizeAddArcPreProcessing, 10, pBuf, INT32_STRING_MAX_LEN); \
		LOG_STRING(pSession, pBuf); \
		LOG_STRING_LITERAL(pSession, ", "); \
		for ( i = 0; i < pSession->pSessionSettings->charSet.nLanguages; ++i ) \
		{ \
			decumaIToA(pSession->pSessionSettings->charSet.pLanguages[i], 10, pBuf, INT32_STRING_MAX_LEN); \
			LOG_STRING(pSession, pBuf); \
			LOG_STRING_LITERAL(pSession, " "); \
		} \
		LOG_STRING_LITERAL(pSession, ", "); \
		for ( i = 0; i < pSession->pSessionSettings->charSet.nSymbolCategories; ++i ) \
		{ \
			decumaIToA(pSession->pSessionSettings->charSet.pSymbolCategories[i], 10, pBuf, INT32_STRING_MAX_LEN); \
			LOG_STRING(pSession, pBuf); \
			LOG_STRING_LITERAL(pSession, " "); \
		} \
		LOG_STRING_LITERAL(pSession, ", "); \
		if (pSession->pSessionSettings->pCharSetExtension) \
		{ \
			i=0;\
			while ( pSession->pSessionSettings->pCharSetExtension[i] || pSession->pSessionSettings->pCharSetExtension[i+1] ) \
			{ \
				decumaIToA(pSession->pSessionSettings->pCharSetExtension[i], 16, pBuf, INT32_STRING_MAX_LEN); \
				LOG_STRING(pSession, pBuf); \
				LOG_STRING_LITERAL(pSession, " "); \
			} \
		} \
		LOG_STRING_LITERAL(pSession, "}\n"); \
	}



/* */
/* Private data type definitions */
/* */

/* */
/* Private function declarations */
/* */

static DECUMA_STATUS getSessionStatus(const DECUMA_SESSION * pSession);

static DECUMA_STATUS validateRecognitionSettings(const DECUMA_RECOGNITION_SETTINGS * pRecognitionSettings);

/* Validate a DECUMA_SESSION_SETTINGS struct, returning an appropriate error code if anything looks wrong */
static DECUMA_STATUS validateSessionSettings(const DECUMA_SESSION_SETTINGS * pSessionSettings,
											 const DECUMA_MEM_FUNCTIONS * pMemFunctions);

/* Set a DECUMA_SESSION_SETTINGS struct. */
static DECUMA_STATUS copySessionSettings(DECUMA_SESSION * pSession,
										 const DECUMA_SESSION_SETTINGS *pSessionSettings,
										 DECUMA_UNICODE *pCharSetExtensionCopy);

/* Allocate memory for and copy Character Set Extension array. */
static DECUMA_STATUS copyCharSetExtension(DECUMA_SESSION * pSession,
										  const DECUMA_UNICODE *pCharSetExtension,
										  DECUMA_UNICODE **ppCharSetExtension);

static void logRecognitionSettings(DECUMA_SESSION * pSession,
								   const DECUMA_RECOGNITION_SETTINGS *pRecognitionSettings);

static void logErrorDuringRecognize(DECUMA_SESSION * pSession,
									DECUMA_STATUS status);

static void logRecognitionResults(DECUMA_SESSION * pSession,
								  const DECUMA_HWR_RESULT *pResults,
								  DECUMA_UINT16 nResults);


/*///////////////////////////////////////////////////////////////////////////////////////// */


/*///////////////////////////////////////////////////////////////////////////////////////// */
/* */
/* Public function definitions */
/* */

/****** Version ********/

#ifdef DECUMA_HWR_ENABLE_EXTRA_API
/* Old decumaGetVersion() now re-named decumaGetEngineVersion, and moved to the "extra" API */
const char * API_FUNC_NAME(GetEngineVersion)(void)
{
	return LIBFUNC(GetVersion)();
}
#endif

const char * API_FUNC_NAME(GetProductVersion)(void)
{
	return T9WRITEPRODUCTCOREVER;
}

const char * API_FUNC_NAME(GetAPIVersion)(void)
{
	return T9WRITEAPICOREVER;
}

/****** Initialization and destruction ********/

API_DECL_NAME DECUMA_UINT32 API_FUNC_NAME(GetSessionSize)(void)
{
	return LIBFUNC(GetSessionSize)() + sizeof(DECUMA_SESSION);
}

API_DECL_NAME DECUMA_STATUS API_FUNC_NAME(BeginSession)(DECUMA_SESSION * pSession,
	const DECUMA_SESSION_SETTINGS * pSessionSettings,
	const DECUMA_MEM_FUNCTIONS * pMemFunctions)
{
	DECUMA_STATUS status = decumaNoError;
	DECUMA_UNICODE *pCharSetExtensionCopy = NULL;

	if ( !VALID_DECUMA_BASIC_TYPES )
            return decumaCompilationError;

	if (pSession == NULL) return decumaNullSessionPointer;

	if (pSession->pLibSession == (LIB_SESSION *)&pSession[1]) return decumaSessionAlreadyInitialized;

	decumaMemset(pSession,0,API_FUNC_NAME(GetSessionSize)());

#ifdef DECUMA_NO_DYNAMIC_ALLOCATION
	if (pMemFunctions != NULL) return decumaUnsupportedParameterValue;
#else
	if (pMemFunctions == NULL) return decumaNullMemoryFunctions;

	status = decumaRuntimeMallocValidateMemfunctions(pMemFunctions);
	if (status != decumaNoError) return status;

	pSession->memFunctions = *pMemFunctions;
#endif

	status = validateSessionSettings(pSessionSettings,pMemFunctions);
	if ( status != decumaNoError ) return status;

	status = copyCharSetExtension(pSession, pSessionSettings->pCharSetExtension, &pCharSetExtensionCopy);
	if (status != decumaNoError)
	{
		/* If validateSessionSettings succeeds, it should not be possible for copyAndsetSessionSettings to fail */
		/* except for allocation failure */
		decumaAssert(status ==decumaAllocationFailed);
		goto decumaBeginSession_error;
	}

	pSession->pSessionSettings = &pSession->sessionSettingsCopy0;

	status = copySessionSettings(pSession, pSessionSettings, pCharSetExtensionCopy);

	decumaAssert(status == decumaNoError);

#ifdef DEBUG_OUTPUT
	g_debugFile = fopen("tmpDebug.txt","w");
#endif /*DEBUG_OUTPUT */


	pSession->logData.nArcsAdded          = 0;
	pSession->logData.nStartArcIdx        = 0;
	pSession->logData.pfLogStringFunction = NULL;
	pSession->logData.pUserData           = NULL;
	pSession->logData.state               = LOG_STATE_READY;

	status = LIBFUNC(BeginSession)((LIB_SESSION *)&pSession[1],pSession->pSessionSettings,&pSession->memFunctions);
	if ( status != decumaNoError ) goto decumaBeginSession_error;

	/* Indicate successful initialization */
	pSession->pLibSession = (LIB_SESSION *)&pSession[1];

	return decumaNoError;

decumaBeginSession_error:

	decumaAssert(pSession->pLibSession != (LIB_SESSION *)&pSession[1]);
	if (pCharSetExtensionCopy) decumaFree(pCharSetExtensionCopy);

	return status;
}


API_DECL_NAME DECUMA_STATUS API_FUNC_NAME(VerifySession)(const DECUMA_SESSION * pSession)
{
	DECUMA_STATUS status = getSessionStatus(pSession);
	if (status !=decumaNoError) return status;

	return LIBFUNC(VerifySession)(pSession->pLibSession);
}


API_DECL_NAME DECUMA_STATUS API_FUNC_NAME(EndSession)(DECUMA_SESSION * pSession)
{
	DECUMA_STATUS status = getSessionStatus(pSession);

	if (status == decumaSessionCorrupt)
	{
		/*TODO: How to handle a corrupt session. Don't return decumaSessionCorrupt */
		/*For now continue as if it was not corrupt */
	}
	else if (status != decumaNoError)
	{
		return status;
	}

	if (LOGGING_ENABLED(pSession) && pSession->logData.state == LOG_STATE_RECOGNITION_DONE) {
		API_FUNC_NAME(LogAcceptedResult)(pSession, NULL, 0);
	}

	status = API_FUNC_NAME(EndArcAddition)(pSession); /*Should already have been called by outer layer */
	/*Status might be != decumaNoError since ArcAddition might not be begun */

	status = LIBFUNC(EndSession)(pSession->pLibSession); /*The lib needs to detach its attached dictionaries */
	if ( status != decumaNoError ) return status;

#ifdef DECUMA_NO_DYNAMIC_ALLOCATION
	decumaAssert(pSession->pSessionSettings->pCharSetExtension == NULL);
#else
	if (pSession->pSessionSettings->pCharSetExtension)
	{
		const DECUMA_MEM_FUNCTIONS * pMemFunctions = &pSession->memFunctions;

		decumaFree(pSession->pSessionSettings->pCharSetExtension);
	}
#endif

	pSession->pLibSession = NULL;

	return decumaNoError;
}




/******* Database functions *********/

API_DECL_NAME DECUMA_STATUS API_FUNC_NAME(DatabaseGetVersion)(DECUMA_ANY_DB_PTR pDB, char * pBuf, int nBufLen)
{
	if (!pDB) return decumaNullDatabasePointer;

	if (!pBuf) return decumaNullPointer;

	if (nBufLen < DECUMA_DATABASE_VERSION_STRING_LENGTH)
		return decumaTooShortBuffer;

	return LIBFUNC(DatabaseGetVersion)(pDB, pBuf, nBufLen);
}


API_DECL_NAME DECUMA_STATUS API_FUNC_NAME(DatabaseIsCategorySupported)(DECUMA_ANY_DB_PTR pDB,
	DECUMA_UINT32 cat, int * pbIsSupported)
{
	if (pDB == NULL) return decumaNullDatabasePointer;

	if (pbIsSupported == NULL) return decumaNullPointer;

	return LIBFUNC(DatabaseIsCategorySupported)(pDB, cat, pbIsSupported);
}

API_DECL_NAME DECUMA_STATUS API_FUNC_NAME(DatabaseIsLanguageSupported)(DECUMA_ANY_DB_PTR pDB,
															   DECUMA_UINT32 lang, int * pbIsSupported)
{
	if (pDB == NULL) return decumaNullDatabasePointer;

	if (pbIsSupported == NULL) return decumaNullPointer;

	return LIBFUNC(DatabaseIsLanguageSupported)(pDB, lang, pbIsSupported);
}

API_DECL_NAME DECUMA_STATUS API_FUNC_NAME(DatabaseIncludesSymbol)(
	DECUMA_ANY_DB_PTR pDB,
	const DECUMA_CHARACTER_SET * pCharSet,
	const DECUMA_UNICODE * pSymbol,
	int * pbIsIncluded)
{
	if (pDB == NULL) return decumaNullDatabasePointer;
	if (pbIsIncluded == NULL) return decumaNullPointer;
	if (pSymbol == NULL) return decumaNullTextPointer;

	return LIBFUNC(DatabaseIncludesSymbol)(pDB, pCharSet, pSymbol, pbIsIncluded);
}

API_DECL_NAME DECUMA_STATUS API_FUNC_NAME(CreateDynamicDatabase)(DECUMA_DYNAMIC_DB_PTR* ppDynamicDB, const DECUMA_MEM_FUNCTIONS * pMemFunctions)
{
	DECUMA_STATUS status;

#if !defined(DECUMA_SPECIFIC_DB_STORAGE) || !(DECUMA_SPECIFIC_DB_STORAGE == 1 || DECUMA_SPECIFIC_DB_STORAGE == 0)
#error DECUMA_SPECIFIC_DB_STORAGE must be defined to 1 or 0
#endif
#if DECUMA_SPECIFIC_DB_STORAGE == 1
	return decumaFunctionNotSupported; /* Dynamic DB generation implementation cannot support specific DB storage */
#else

	if ( ppDynamicDB == NULL )
		return decumaNullPointer;

	status = decumaRuntimeMallocValidateMemfunctions(pMemFunctions);

	if ( status != decumaNoError )
		return status;

	/* Check if this already is a valid database */
	status = API_FUNC_NAME(DynamicDatabaseIsValid)(*ppDynamicDB);
	if ( status == decumaNoError ) {
		return decumaDatabaseAlreadyCreated;
	}

	return LIBFUNC(CreateDynamicDatabase)(ppDynamicDB, pMemFunctions);

#endif /* DECUMA_SPECIFIC_DB_STORAGE */
}

API_DECL_NAME DECUMA_STATUS API_FUNC_NAME(AddAllograph)(      DECUMA_DYNAMIC_DB_PTR   * ppDynamicDB,
								 const DECUMA_CURVE         * pCurve,
								 const DECUMA_UNICODE       * pUnicode, DECUMA_UINT32 nUnicodes,
								 const DECUMA_CHARACTER_SET * pCharacterSet,
								 DECUMA_INT32 nBaseline, DECUMA_INT32 nHelpline,
								 int bGesture, int bInstantGesture,
								 const DECUMA_MEM_FUNCTIONS * pMemFunctions)
{
	DECUMA_STATUS status;

#if !defined(DECUMA_SPECIFIC_DB_STORAGE) || !(DECUMA_SPECIFIC_DB_STORAGE == 1 || DECUMA_SPECIFIC_DB_STORAGE == 0)
#error DECUMA_SPECIFIC_DB_STORAGE must be defined to 1 or 0
#endif
#if DECUMA_SPECIFIC_DB_STORAGE == 1
	return decumaFunctionNotSupported; /* Dynamic DB generation implementation cannot support specific DB storage */
#else

	/* Basic input validation */
	if ( ppDynamicDB == NULL || *ppDynamicDB == NULL)
		return decumaNullPointer;

	status = API_FUNC_NAME(DynamicDatabaseIsValid)(*ppDynamicDB);
	if ( status != decumaNoError )
		return status;

	if ( pUnicode == NULL )
		return decumaNullTextPointer;

	if ( pCharacterSet == NULL )
		return decumaNullPointer;

	if ( pCurve == NULL )
		return decumaNullCurvePointer;

	/* Extended input validation */

	status = decumaRuntimeMallocValidateMemfunctions(pMemFunctions);

	if ( status != decumaNoError )
		return status;

	return LIBFUNC(AddAllograph)(ppDynamicDB, pCurve, pUnicode, nUnicodes, pCharacterSet, nBaseline, nHelpline,
			 bGesture, bInstantGesture, pMemFunctions);

#endif /* DECUMA_SPECIFIC_DB_STORAGE */
}

API_DECL_NAME DECUMA_STATUS API_FUNC_NAME(GetDynamicDatabaseByteSize)(DECUMA_DYNAMIC_DB_PTR pDynamicDB, DECUMA_UINT32 * pSize)
{
	DECUMA_STATUS status = API_FUNC_NAME(DynamicDatabaseIsValid)(pDynamicDB);
	if ( status != decumaNoError)
		return status;

	if ( pSize == NULL )
		return decumaNullPointer;

	return LIBFUNC(GetDynamicDatabaseByteSize)(pDynamicDB, pSize);
}

API_DECL_NAME DECUMA_STATUS API_FUNC_NAME(DynamicDatabaseIsValid)(DECUMA_DYNAMIC_DB_PTR pDynamicDB)
{
	if ( pDynamicDB == NULL )
		return decumaNullPointer;

	return LIBFUNC(DynamicDatabaseIsValid)(pDynamicDB);
}

API_DECL_NAME DECUMA_STATUS API_FUNC_NAME(DestroyDynamicDatabase)(DECUMA_DYNAMIC_DB_PTR* ppDynamicDB, const DECUMA_MEM_FUNCTIONS * pMemFunctions)
{
	DECUMA_STATUS status;

#if !defined(DECUMA_SPECIFIC_DB_STORAGE) || !(DECUMA_SPECIFIC_DB_STORAGE == 1 || DECUMA_SPECIFIC_DB_STORAGE == 0)
#error DECUMA_SPECIFIC_DB_STORAGE must be defined to 1 or 0
#endif
#if DECUMA_SPECIFIC_DB_STORAGE == 1
	return decumaFunctionNotSupported; /* Dynamic DB generation implementation cannot support specific DB storage */
#else

	status = decumaRuntimeMallocValidateMemfunctions(pMemFunctions);

	if ( status != decumaNoError )
		return status;

	if ( ppDynamicDB == NULL)
		return decumaNullPointer;

	status = API_FUNC_NAME(DynamicDatabaseIsValid)(*ppDynamicDB);

	if ( status  != decumaNoError )
		return status;

	LIBFUNC(DestroyDynamicDatabase)(ppDynamicDB, pMemFunctions);
	*ppDynamicDB = NULL;

	return decumaNoError;

#endif /* DECUMA_SPECIFIC_DB_STORAGE */
}



/******* Settings *********/

API_DECL_NAME DECUMA_STATUS API_FUNC_NAME(GetSessionSettings)( DECUMA_SESSION * pSession,
	const DECUMA_SESSION_SETTINGS ** ppSessionSettings)
{
	DECUMA_STATUS status = getSessionStatus(pSession);
	if (status != decumaNoError) return status;

	if (ppSessionSettings == NULL) return decumaNullPointer;

	*ppSessionSettings = (const DECUMA_SESSION_SETTINGS *) pSession->pSessionSettings;

	return decumaNoError;
}

API_DECL_NAME DECUMA_STATUS API_FUNC_NAME(ChangeSessionSettings)( DECUMA_SESSION * pSession,
	const DECUMA_SESSION_SETTINGS * pSessionSettings)
{
	/* NOTE: Upon any error the previous settings should be kept. */
	/*       We do not want a situation with a session without */
	/*       any valid settings. */
	DECUMA_STATUS status = getSessionStatus(pSession);
	DECUMA_UNICODE * pCharSetExtensionCopy = NULL;

	if (status != decumaNoError) return status;

	if (pSession->bAddingArcs) return decumaFunctionInsideArcAdditionSeq;

	status = validateSessionSettings(pSessionSettings,&pSession->memFunctions); /* Validates on decuma_hwr and lib levels */
	if (status != decumaNoError) return status;

	/* Switch active settings copy before writing new settings in order to not destroy old setting until successful */
	/* change has been confirmed. */
	pSession->activeSettingsCopy = !pSession->activeSettingsCopy;
	pSession->pSessionSettings = pSession->activeSettingsCopy == 0 ? &pSession->sessionSettingsCopy0 : &pSession->sessionSettingsCopy1;

	status = copyCharSetExtension(pSession, pSessionSettings->pCharSetExtension, &pCharSetExtensionCopy);
	if (status != decumaNoError)
	{
		/* If validateSessionSettings succeeds, it should not be possible for setSessionSettings to fail */
		/* except for allocation errors. */
		decumaAssert(status == decumaAllocationFailed);
		goto decumaChangeSessionSettings_cleanup;
	}

	status = copySessionSettings(pSession, pSessionSettings, pCharSetExtensionCopy);

	if (status != decumaNoError) goto decumaChangeSessionSettings_cleanup;

	status = LIBFUNC(ChangeSessionSettings)(pSession->pLibSession, pSession->pSessionSettings);

decumaChangeSessionSettings_cleanup:

	if ( status != decumaNoError )
	{
		/* Revert to old settings. If any function has returned an error code,
		 * the new settings should not have caused any permanent damage making
		 * this perfectly safe:
		 */

		pSession->activeSettingsCopy = !pSession->activeSettingsCopy;
		pSession->pSessionSettings = pSession->activeSettingsCopy == 0 ? &pSession->sessionSettingsCopy0 : &pSession->sessionSettingsCopy1;
	}

	/* Always free pCharSetExtension from the inactive settings */
	{
		DECUMA_SESSION_SETTINGS * pInactiveSessionSettings = pSession->activeSettingsCopy != 0 ? &pSession->sessionSettingsCopy0 : &pSession->sessionSettingsCopy1;

#ifdef DECUMA_NO_DYNAMIC_ALLOCATION
		decumaAssert(pInactiveSessionSettings->pCharSetExtension == NULL);
#else
		{
			const DECUMA_MEM_FUNCTIONS * pMemFunctions = &pSession->memFunctions;
			decumaFree(pInactiveSessionSettings->pCharSetExtension);
			/* Bravely leave the address intact.
			 * This works (i.e. no double free) because the pointer is /always/
			 * initialized to either NULL or a newly allocated buffer as a
			 * result from copyCharSetExtension() + copySessionSettings()
			 */
		}
#endif

	}
	return status;
}


/****** Recognition functions ********/


API_DECL_NAME DECUMA_STATUS API_FUNC_NAME(BeginArcAddition)(DECUMA_SESSION * pSession)
{
	DECUMA_STATUS status = getSessionStatus(pSession);
	if (status != decumaNoError) return status;

	if (pSession->bAddingArcs) return decumaArcAdditionSeqAlreadyStarted;

	if (API_FUNC_NAME(VerifySession)(pSession) != decumaNoError) return API_FUNC_NAME(VerifySession)(pSession);

	status = LIBFUNC(BeginArcAddition)(pSession->pLibSession);
	if (status != decumaNoError) goto decumaBeginArcAddition_error;

	if (LOGGING_ENABLED(pSession)) {
		char pBuf[100];

		LOG_STRING_LITERAL(pSession, "A| # NEWPAGE\n");
		LOG_STRING_LITERAL(pSession, "I| % NEWPAGE\n");
		LOG_STRING_LITERAL(pSession, "F| % NEWPAGE\n");

		if (pSession->pSessionSettings->supportLineSet != noSupportlines )
		{
			DECUMA_INT32 baseline,helpline;
			if (pSession->pSessionSettings->supportLineSet == baselineAndHelpline)
			{
				baseline = pSession->pSessionSettings->baseline;
				helpline = pSession->pSessionSettings->baseline;
			}
			else if (pSession->pSessionSettings->supportLineSet == baselineAndTopline)
			{
				baseline = pSession->pSessionSettings->baseline;
				helpline = (pSession->pSessionSettings->baseline+pSession->pSessionSettings->topline)/2;
			}
			else if (pSession->pSessionSettings->supportLineSet == helplineAndTopline)
			{
				baseline = 2*pSession->pSessionSettings->helpline - pSession->pSessionSettings->topline;
				helpline = pSession->pSessionSettings->helpline;
			}
			else
			{
				decumaAssert(0); /*Current logging format only supports two lines (base and helpline) */
			}

			if (decumaIToA(pSession->pSessionSettings->helpline, 10, pBuf, sizeof(pBuf))) {
				LOG_STRING_LITERAL(pSession, "A| # HELPLINE ");
				LOG_STRING(pSession, pBuf);
				LOG_STRING_LITERAL(pSession, "\n");

				if (decumaIToA(pSession->pSessionSettings->baseline, 10, pBuf, sizeof(pBuf))) {
					LOG_STRING_LITERAL(pSession, "A| # BASELINE ");
					LOG_STRING(pSession, pBuf);
					LOG_STRING_LITERAL(pSession, "\n");
				}
			}
		}
		pSession->logData.nStartArcIdx += pSession->logData.nArcsAdded;
		pSession->logData.nArcsAdded = 0;
	}

	pSession->bAddingArcs = 1;

	return decumaNoError;

decumaBeginArcAddition_error:
	/*Don't set pSession->bAddingArcs when we have an error */
	return status;
}


/*Writes an ID in pSamplingArcID which can be used for further reference to the arc */
API_DECL_NAME DECUMA_STATUS API_FUNC_NAME(StartNewArc)(DECUMA_SESSION * pSession, DECUMA_UINT32 arcID)
{
	DECUMA_STATUS status = getSessionStatus(pSession);
	if (status != decumaNoError) return status;
	if (!pSession->bAddingArcs) return decumaFunctionOutsideArcAdditionSeq;

#ifdef DEBUG_OUTPUT
		fprintf(g_debugFile,"Starting arc==%d\n",arcID);
		fflush(g_debugFile);
#endif /*DEBUG_OUTPUT */

	status = LIBFUNC(StartNewArc)(pSession->pLibSession, arcID);
	if (status !=decumaNoError) return status;

	if (LOGGING_ENABLED(pSession))
	{
		char pBuf[20];
		LOG_STRING_LITERAL(pSession, "A| NEWARC ");
		if (decumaIToA(arcID, 10, pBuf, sizeof(pBuf)))
		{
			LOG_STRING(pSession, pBuf);
		}
		LOG_STRING_LITERAL(pSession, "\n");
	}

	return decumaNoError;
}

API_DECL_NAME DECUMA_STATUS API_FUNC_NAME(StartNewSymbol)(DECUMA_SESSION * pSession)
{
	DECUMA_STATUS status = getSessionStatus(pSession);
	if (status != decumaNoError) return status;
	if (!pSession->bAddingArcs) return decumaFunctionOutsideArcAdditionSeq;

	status = LIBFUNC(StartNewSymbol)(pSession->pLibSession);
	if (status !=decumaNoError) return status;

	if (LOGGING_ENABLED(pSession))
	{
		LOG_STRING_LITERAL(pSession, "A| # NEWSYMBOL\n");
	}

	return decumaNoError;
}

#ifdef DECUMA_HWR_ENABLE_EXTRA_API
DECUMA_STATUS API_FUNC_NAME(AddPoints)(DECUMA_SESSION * pSession,
								   DECUMA_POINT * pPts,
								   int nPts,
								   DECUMA_UINT32 arcID,
								   int * pnPtsAdded)
{
	DECUMA_STATUS status = getSessionStatus(pSession);
	if (status != decumaNoError) return status;
	if (!pSession->bAddingArcs) return decumaFunctionOutsideArcAdditionSeq;
	if (!pPts) return decumaNullPointPointer;
	if (!nPts) return decumaZeroPoints;

	if (LOGGING_ENABLED(pSession))
	{
		int i;
		char pBuf[20];
		for (i=0;i<nPts; i++)
		{
			LOG_STRING_LITERAL(pSession, "A| PT ");

			if (!decumaIToA(arcID, 10, pBuf, sizeof(pBuf)))
				break;

			LOG_STRING(pSession, pBuf);
			LOG_STRING_LITERAL(pSession, " ");
			if (!decumaIToA(pPts[i].x, 10, pBuf, sizeof(pBuf)))
				break;

			LOG_STRING(pSession, pBuf);
			LOG_STRING_LITERAL(pSession, " ");

			if (!decumaIToA(pPts[i].y, 10, pBuf, sizeof(pBuf)))
				break;

			LOG_STRING(pSession, pBuf);
			LOG_STRING_LITERAL(pSession, "\n");
		}
	}

	return LIBFUNC(AddPoints)(pSession->pLibSession, pPts, nPts, arcID, pnPtsAdded);
}
#endif

API_DECL_NAME DECUMA_STATUS API_FUNC_NAME(AddPoint)(DECUMA_SESSION * pSession,
								  DECUMA_COORD x,
								  DECUMA_COORD y,
								  DECUMA_UINT32 arcID)
{
	DECUMA_STATUS status = getSessionStatus(pSession);
	if (status != decumaNoError) return status;
	if (!pSession->bAddingArcs) return decumaFunctionOutsideArcAdditionSeq;

	if (LOGGING_ENABLED(pSession))
	{
		char pBuf[20];
		LOG_STRING_LITERAL(pSession, "A| PT ");

		if (decumaIToA(arcID, 10, pBuf, sizeof(pBuf)))
		{
			LOG_STRING(pSession, pBuf);
		}
		LOG_STRING_LITERAL(pSession, " ");

		if (decumaIToA(x, 10, pBuf, sizeof(pBuf)))
		{
			LOG_STRING(pSession, pBuf);
		}
		LOG_STRING_LITERAL(pSession, " ");

		if (decumaIToA(y, 10, pBuf, sizeof(pBuf)))
		{
			LOG_STRING(pSession, pBuf);
		}
		LOG_STRING_LITERAL(pSession, "\n");
	}


	return LIBFUNC(AddPoint)(pSession->pLibSession, x, y, arcID);
}

#ifdef DECUMA_HWR_ENABLE_EXTRA_API
DECUMA_STATUS API_FUNC_NAME(GetUncommittedArcCount)(DECUMA_SESSION * pSession, int * pnUncommittedArcs)
{
	DECUMA_STATUS status = getSessionStatus(pSession);
	if (status != decumaNoError) return status;
	if (pSession->bAddingArcs == 0) return decumaFunctionOutsideArcAdditionSeq;
	if (pnUncommittedArcs == NULL) return decumaNullPointer;

	return LIBFUNC(GetUncommittedArcCount)(pSession->pLibSession, pnUncommittedArcs);
}
#endif

#ifdef DECUMA_HWR_ENABLE_EXTRA_API
DECUMA_STATUS API_FUNC_NAME(GetUncommittedArcID)(DECUMA_SESSION * pSession, int idx, DECUMA_UINT32 * pArcID)
{
	DECUMA_STATUS status = getSessionStatus(pSession);
	if (status != decumaNoError) return status;
	if (pSession->bAddingArcs == 0) return decumaFunctionOutsideArcAdditionSeq;
	if (pArcID == NULL) return decumaNullPointer;

	return LIBFUNC(GetUncommittedArcID)(pSession->pLibSession,idx,pArcID);
}
#endif

/*Ends the arc and removes it from the session */
DECUMA_STATUS API_FUNC_NAME(CancelArc)(DECUMA_SESSION * pSession,DECUMA_UINT32 arcID)
{
	DECUMA_STATUS status = getSessionStatus(pSession);
	if (status != decumaNoError) return status;
	if (!pSession->bAddingArcs) return decumaFunctionOutsideArcAdditionSeq;

	if (LOGGING_ENABLED(pSession))
	{
		char pBuf[20];
		LOG_STRING_LITERAL(pSession, "A| CANCELARC ");
		if (decumaIToA(arcID, 10, pBuf, sizeof(pBuf)))
		{
			LOG_STRING(pSession, pBuf);
		}
		LOG_STRING_LITERAL(pSession, "\n");
	}
	return LIBFUNC(CancelArc)(pSession->pLibSession,arcID);
}

/*Ends and adds the arc to the arc-addition-sequence which will be used in a future recognize call */
DECUMA_STATUS API_FUNC_NAME(CommitArc)(DECUMA_SESSION * pSession,DECUMA_UINT32 arcID)
{
	DECUMA_STATUS status = getSessionStatus(pSession);
	if (status != decumaNoError) return status;
	if (!pSession->bAddingArcs) return decumaFunctionOutsideArcAdditionSeq;

	status=LIBFUNC(CommitArc)(pSession->pLibSession,arcID);
	if (status != decumaNoError) return status;

	/* TODO: How should we handle multi-touch here? */
	/* Log the incoming arc */
	if (LOGGING_ENABLED(pSession))
	{
		char pBuf[20];
		LOG_STRING_LITERAL(pSession, "A| COMMITARC ");
		if (decumaIToA(arcID, 10, pBuf, sizeof(pBuf)))
		{
			LOG_STRING(pSession, pBuf);
		}
		LOG_STRING_LITERAL(pSession, "\n");
		pSession->logData.nArcsAdded++;
	}


#ifdef DEBUG_OUTPUT
	fprintf(g_debugFile,"Committed arc==%d with status=%d\n",arcID,status);
	fflush(g_debugFile);
#endif /*DEBUG_OUTPUT */

	return decumaNoError;

}

#ifdef DECUMA_HWR_ENABLE_EXTRA_API
DECUMA_STATUS API_FUNC_NAME(RecognizeFast)( DECUMA_SESSION * pSession,
	DECUMA_HWR_RESULT * pResult,
	DECUMA_UINT16 * pnResults,  /* In most cases == 1, but can be 0 */
	DECUMA_UINT16 nMaxChars,
	const DECUMA_INTERRUPT_FUNCTIONS * pInterruptFunctions)
{
	DECUMA_STATUS status;
	/* Input validation */
	status = getSessionStatus(pSession);
	if (status != decumaNoError) return status;

	if (!pResult) return decumaNullResultPointer;
	if (pResult->pChars == NULL) return decumaNullPointer;
	if (!pnResults) return decumaNullPointer;
	if (!pSession->bAddingArcs) return decumaFunctionOutsideArcAdditionSeq;


	/*logRecognitionSettings(pSession); */
	status = LIBFUNC(RecognizeFast)(pSession->pLibSession, pResult, pnResults, nMaxChars,pInterruptFunctions);

#ifdef DEBUG_OUTPUT
	fprintf(g_debugFile,"Fast recognition done with status=%d, nResults=%d\n",status,*pnResults);
	fflush(g_debugFile);
#endif /*DEBUG_OUTPUT */

	if (status != decumaNoError) {
		logErrorDuringRecognize(pSession, status);
		return status;
	}
	logRecognitionResults(pSession, pResult, *pnResults);
	return decumaNoError;
}
#endif

API_DECL_NAME DECUMA_STATUS API_FUNC_NAME(Recognize)( DECUMA_SESSION * pSession,
	DECUMA_HWR_RESULT * pResults,
	DECUMA_UINT16 nMaxResults,
	DECUMA_UINT16 * pnResults,
	DECUMA_UINT16 nMaxCharsPerResult,
	const DECUMA_RECOGNITION_SETTINGS * pRecognitionSettings,
	const DECUMA_INTERRUPT_FUNCTIONS * pInterruptFunctions
	)
{
	DECUMA_STATUS status;
	/* Input validation */
	status = getSessionStatus(pSession);
	if (status != decumaNoError) return status;

	if (!pResults) return decumaNullResultPointer;
	if (!pnResults) return decumaNullPointer;
	if (!pSession->bAddingArcs) return decumaFunctionOutsideArcAdditionSeq;
	if (pInterruptFunctions != NULL && pInterruptFunctions->pShouldAbortRecognize == NULL) return decumaNullFunctionPointer;

	status = validateRecognitionSettings(pRecognitionSettings);
	if (status != decumaNoError) return status;

	logRecognitionSettings(pSession, pRecognitionSettings);
	status = LIBFUNC(Recognize)(pSession->pLibSession, pResults, nMaxResults, pnResults, nMaxCharsPerResult, pRecognitionSettings,
		pInterruptFunctions);

#ifdef DEBUG_OUTPUT
	fprintf(g_debugFile,"Recognition done with status=%d, nResults=%d\n",status,*pnResults);
	fflush(g_debugFile);
#endif /*DEBUG_OUTPUT */

	if (status != decumaNoError) {
		logErrorDuringRecognize(pSession, status);
		return status;
	}
	logRecognitionResults(pSession, pResults, *pnResults);
	return decumaNoError;
}
API_DECL_NAME DECUMA_STATUS API_FUNC_NAME(IndicateInstantGesture)(
	DECUMA_SESSION * pSession,
	int * pbInstantGesture,
	const DECUMA_INSTANT_GESTURE_SETTINGS * pInstantGestureSettings)
{
	DECUMA_STATUS status;
	/* Input validation */
	status = getSessionStatus(pSession);
	if (status != decumaNoError) return status;

	if (!pbInstantGesture) return decumaNullPointer;
	if (!pSession->bAddingArcs) return decumaFunctionOutsideArcAdditionSeq;
	if (!pInstantGestureSettings) return decumaNullSettingsPointer;

	status = LIBFUNC(IndicateInstantGesture)(pSession->pLibSession, pbInstantGesture,
		pInstantGestureSettings);

	return status;
}

#ifdef DECUMA_HWR_ENABLE_EXTRA_API
DECUMA_STATUS API_FUNC_NAME(RecognizeForceRecognition)( DECUMA_SESSION * pSession,
	const DECUMA_UNICODE * symbolString,
	DECUMA_HWR_RESULT * pResults,
	DECUMA_UINT16 nMaxResults,
	DECUMA_UINT16 * pnResults,
	DECUMA_UINT16 nMaxCharsPerResult,
	const DECUMA_RECOGNITION_SETTINGS * pRecognitionSettings,
	int bFast,
	const DECUMA_INTERRUPT_FUNCTIONS * pInterruptFunctions)
{
	DECUMA_STATUS status;
	/* Input validation */
	status = getSessionStatus(pSession);
	if (status != decumaNoError) return status;

	if (!pResults) return decumaNullResultPointer;
	if (pResults->pChars == NULL) return decumaNullPointer;
	if (!pnResults) return decumaNullPointer;
	if (!pSession->bAddingArcs) return decumaFunctionOutsideArcAdditionSeq;

	status = validateRecognitionSettings(pRecognitionSettings);
	if (status != decumaNoError) return status;

	logRecognitionSettings(pSession, pRecognitionSettings);
	status = LIBFUNC(RecognizeForceRecognition)(pSession->pLibSession, symbolString, pResults, nMaxResults, pnResults, nMaxCharsPerResult, pRecognitionSettings, bFast,
		pInterruptFunctions);

#ifdef DEBUG_OUTPUT
	fprintf(g_debugFile,"Forced recognition done with status=%d, nResults=%d\n",status,*pnResults);
	fflush(g_debugFile);
#endif /*DEBUG_OUTPUT */

	if (status != decumaNoError) {
		logErrorDuringRecognize(pSession, status);
		return status;
	}
	logRecognitionResults(pSession, pResults, *pnResults);
	return decumaNoError;
}
#endif

#ifdef DECUMA_HWR_ENABLE_EXTRA_API
DECUMA_STATUS API_FUNC_NAME(RecognizeForceSegmentation)( DECUMA_SESSION * pSession,
	const DECUMA_UNICODE * symbolString,
	DECUMA_HWR_RESULT * pResults,
	DECUMA_UINT16 nMaxResults,
	DECUMA_UINT16 * pnResults,
	DECUMA_UINT16 nMaxCharsPerResult,
	const DECUMA_RECOGNITION_SETTINGS * pRecognitionSettings,
	int bFast,
	const DECUMA_INTERRUPT_FUNCTIONS * pInterruptFunctions)
{
	DECUMA_STATUS status;
	/* Input validation */
	status = getSessionStatus(pSession);
	if (status != decumaNoError) return status;

	if (!pResults) return decumaNullResultPointer;
	if (pResults->pChars == NULL) return decumaNullPointer;
	if (!pnResults) return decumaNullPointer;
	if (!pSession->bAddingArcs) return decumaFunctionOutsideArcAdditionSeq;

	status = validateRecognitionSettings(pRecognitionSettings);
	if (status != decumaNoError) return status;

	logRecognitionSettings(pSession, pRecognitionSettings);
	status = LIBFUNC(RecognizeForceSegmentation)(pSession->pLibSession, symbolString, pResults, nMaxResults, pnResults, nMaxCharsPerResult, pRecognitionSettings, bFast,
		pInterruptFunctions);


#ifdef DEBUG_OUTPUT
	fprintf(g_debugFile,"Forced segmentation recognition done with status=%d, nResults=%d\n",status,*pnResults);
	fflush(g_debugFile);
#endif /*DEBUG_OUTPUT */

	if (status != decumaNoError) {
		logErrorDuringRecognize(pSession, status);
		return status;
	}
	logRecognitionResults(pSession, pResults, *pnResults);
	return decumaNoError;
}
#endif

#ifdef DECUMA_HWR_ENABLE_EXTRA_API
DECUMA_STATUS API_FUNC_NAME(GetDistance)( DECUMA_SESSION * pSession,
	const DECUMA_UNICODE * symbolString,
	const DECUMA_RECOGNITION_SETTINGS * pRecognitionSettings,
	DECUMA_UINT32 * pDistance,
	DECUMA_STRING_TYPE * pStringType)
{
	int nStrlen = 0;
	DECUMA_STATUS status;
	/* Input validation */
	status = getSessionStatus(pSession);
	if (status != decumaNoError) return status;
	if (!symbolString) return decumaNullPointer;
	if (!pSession->bAddingArcs) return decumaFunctionOutsideArcAdditionSeq;

	while (symbolString[nStrlen]!=0)
	{
		nStrlen++;
	}
	if (nStrlen>MAX_GET_DIST_SYMBOL_LEN) return decumaTooLongString;


	status = LIBFUNC(GetDistance)(pSession->pLibSession, symbolString, pRecognitionSettings, pDistance, pStringType);

#ifdef DEBUG_OUTPUT
	fprintf(g_debugFile,"Get distance done with status=%d, distance=%d\n",status,*pDistance);
	fflush(g_debugFile);
#endif /*DEBUG_OUTPUT */

	return status;
}
#endif /* DECUMA_HWR_ENABLE_EXTRA_API */

API_DECL_NAME DECUMA_STATUS API_FUNC_NAME(NoteSelectedCandidate)( DECUMA_SESSION * pSession, int nResultIdx)
{
	DECUMA_STATUS status = getSessionStatus(pSession);
	if (status != decumaNoError) return status;
	if (nResultIdx < -1) return decumaInvalidIndex;

	return LIBFUNC(NoteSelectedCandidate)(pSession->pLibSession, nResultIdx);
}

API_DECL_NAME DECUMA_STATUS API_FUNC_NAME(EndArcAddition)(DECUMA_SESSION * pSession)
{
	DECUMA_STATUS status = getSessionStatus(pSession);
	if (status != decumaNoError) return status;

	if (!pSession->bAddingArcs) return decumaArcAdditionSeqNotStarted;

	pSession->bAddingArcs = 0;

	return LIBFUNC(EndArcAddition)(pSession->pLibSession);
}



/******* Debugging functions *******/

API_DECL_NAME DECUMA_STATUS API_FUNC_NAME(StartLogging)(DECUMA_SESSION         * pSession,
												void                   * pUserData,
												DECUMA_LOG_STRING_FUNC * pfLogStringFunction)
{
	char pDbVersionString[DECUMA_DATABASE_VERSION_STRING_LENGTH];

	DECUMA_STATUS status = getSessionStatus(pSession);
	if (status != decumaNoError) return status;

	if (SESSION_NOT_INITIALIZED(pSession))
		return decumaSessionNotInitialized;

	if (!pfLogStringFunction)
		return decumaNullPointer;

	if (pSession->bAddingArcs)
		return decumaFunctionInsideArcAdditionSeq;

	if (LOGGING_ENABLED(pSession))
		API_FUNC_NAME(StopLogging)(pSession);

	pSession->logData.pfLogStringFunction = pfLogStringFunction;
	pSession->logData.pUserData           = pUserData;
	pSession->logData.nStartArcIdx        = 0;
	pSession->logData.nArcsAdded          = 0;

	LOG_STRING_LITERAL(pSession, "C| LOGGING STARTED\nC| PRODUCT VERSION: ");
	LOG_STRING(pSession, API_FUNC_NAME(GetProductVersion)());
	LOG_STRING_LITERAL(pSession, "\nC| API VERSION: ");
	LOG_STRING(pSession, API_FUNC_NAME(GetAPIVersion)());

	if (decumaNoError == API_FUNC_NAME(DatabaseGetVersion)(pSession->pSessionSettings->pStaticDB, &pDbVersionString[0], sizeof(pDbVersionString))) {
		LOG_STRING_LITERAL(pSession, "\nC| DATABASE VERSION: ");
		LOG_STRING(pSession, &pDbVersionString[0]);
	}

	LOG_STRING_LITERAL(pSession, "\n");

	LOG_SESSION_SETTINGS(pSession);

	return decumaNoError;
}


API_DECL_NAME DECUMA_STATUS API_FUNC_NAME(StopLogging)(DECUMA_SESSION * pSession)
{
	DECUMA_STATUS status = getSessionStatus(pSession);
	if (status != decumaNoError) return status;

	if (SESSION_NOT_INITIALIZED(pSession))
		return decumaSessionNotInitialized;

	if (pSession->bAddingArcs)
		return decumaFunctionInsideArcAdditionSeq;

	if (!LOGGING_ENABLED(pSession))
		return decumaLoggingNotStarted;

	if (pSession->logData.state == LOG_STATE_RECOGNITION_DONE)
		API_FUNC_NAME(LogAcceptedResult)(pSession, NULL, 0);

	LOG_STRING_LITERAL(pSession, "C| LOGGING STOPPED\n");

	pSession->logData.pfLogStringFunction = NULL;
	pSession->logData.pUserData           = NULL;

	return decumaNoError;
}


/* TODO: This is where we can put something into to the UWD

   or not TODO, that is the question:

   We only come here if logging is used, and that is probably true only when
   debugging and testing. Better then to change the API to expose a function
   decumaAcceptResult(), and have that one call this logging function. /JL
 */
API_DECL_NAME DECUMA_STATUS API_FUNC_NAME(LogAcceptedResult)(DECUMA_SESSION       * pSession,
													 const DECUMA_UNICODE * pResult,
													 DECUMA_UINT32          nResultLength)
{
	char          pBuf[INT32_STRING_MAX_LEN];
	int i;

	DECUMA_STATUS status = getSessionStatus(pSession);
	if (status != decumaNoError) return status;

	if (SESSION_NOT_INITIALIZED(pSession))
		return decumaSessionNotInitialized;

	if (!LOGGING_ENABLED(pSession))
		return decumaLoggingNotStarted;

	if (pSession->logData.state != LOG_STATE_RECOGNITION_DONE)
		return decumaInvalidLogState;

	/* Log accepted result in DF-file format */

	if (nResultLength) {
		int nStringStart = 0;

		if (pResult == NULL)
			return decumaNullPointer;

		LOG_STRING_LITERAL(pSession, "F| '");

		for (i = nStringStart; i < (int)nResultLength; i++) {
			if (decumaIToA(pResult[i], 16, pBuf, INT32_STRING_MAX_LEN)) {
				LOG_STRING_LITERAL(pSession, "\\x");
				LOG_STRING(pSession, pBuf);
			}
		}

		LOG_STRING_LITERAL(pSession, "' ;");

	}
	else {
		LOG_STRING_LITERAL(pSession, "F| #'<Not accepted>' ;");
	}

	for (i = 0; i < pSession->logData.nRecognitionArcsUsed; i++) {
		if (decumaIToA(pSession->logData.nRecognitionStartArcIdx + i, 10, pBuf, INT32_STRING_MAX_LEN)) {
			LOG_STRING_LITERAL(pSession, " ");
			LOG_STRING(pSession, pBuf);
		}
	}

	LOG_STRING_LITERAL(pSession, "\n");

	pSession->logData.state = LOG_STATE_READY;

	return decumaNoError;

}

/******* Dictionary conversion functions *******/

API_DECL_NAME DECUMA_STATUS API_FUNC_NAME(UnpackXT9Dictionary)(void ** ppDictionaryData,
	const void * pXT9Dictionary, DECUMA_UINT32 xt9DataSize, DECUMA_XT9_DICTIONARY_TYPE type,
	DECUMA_UINT32 * pUnpackedDataSize,const DECUMA_MEM_FUNCTIONS * pMemFunctions)
{
	if ( ppDictionaryData == NULL )
		return decumaNullPointer;

	if ( pXT9Dictionary == NULL )
		return decumaNullDictionaryPointer;

	if (xt9DataSize == 0)
		return decumaInvalidDictionary;

	if ( pUnpackedDataSize == NULL )
		return decumaNullPointer;

	if (type>numberOfXT9DictionaryTypes)
		return decumaInvalidEnumeratorValue;

	if (pMemFunctions == NULL) return decumaNullMemoryFunctions;

	return LIBFUNC(UnpackXT9Dictionary)(ppDictionaryData, pXT9Dictionary,
		xt9DataSize, type,pUnpackedDataSize,pMemFunctions);
}

API_DECL_NAME DECUMA_STATUS API_FUNC_NAME(DestroyUnpackedDictionary)(void ** ppDictionaryData,
	const DECUMA_MEM_FUNCTIONS * pMemFunctions)
{
	if (!ppDictionaryData) return decumaNullPointer;
	if (!pMemFunctions) return decumaNullMemoryFunctions;

	return LIBFUNC(DestroyUnpackedDictionary)(ppDictionaryData,pMemFunctions);
}



/******* Dictionary attachment functions *******/

API_DECL_NAME DECUMA_STATUS API_FUNC_NAME(AttachStaticDictionary)(DECUMA_SESSION * pSession,
									 const void * pDictionaryData)
{
	DECUMA_STATUS status = getSessionStatus(pSession);
	if (status != decumaNoError) return status;

	if ( !pDictionaryData )
		return decumaNullPointer;

	return LIBFUNC(AttachStaticDictionary)(pSession->pLibSession, pDictionaryData);
}

API_DECL_NAME DECUMA_STATUS API_FUNC_NAME(DetachStaticDictionary)(DECUMA_SESSION * pSession,
									 const void * pDictionary)
{
	DECUMA_STATUS status = getSessionStatus(pSession);
	if (status != decumaNoError) return status;

	if ( !pDictionary )
		return decumaNullPointer;

	return LIBFUNC(DetachStaticDictionary)(pSession->pLibSession, pDictionary);
}

/******* Dynamic Dictionary functions *******/

API_DECL_NAME DECUMA_STATUS API_FUNC_NAME(AttachDynamicDictionary)(DECUMA_SESSION * pSession,
														   DECUMA_DICTIONARY_TYPE type,
														   void * pDictionaryData,
														   int cbDictionaryDataSize)
{
	/* Input validation */
	DECUMA_STATUS status = getSessionStatus(pSession);
	if (status != decumaNoError) return status;

	if ( !pDictionaryData )	return decumaNullPointer;

	if ( type >= numberOfDictionaryTypes || type == decumaStaticDictionary )
		return decumaInvalidEnumeratorValue;

	if (cbDictionaryDataSize == 0)	return decumaTooShortBuffer;

	return LIBFUNC(AttachDynamicDictionary)(pSession->pLibSession,type,pDictionaryData,cbDictionaryDataSize);
}

API_DECL_NAME DECUMA_STATUS API_FUNC_NAME(DetachDynamicDictionary)(DECUMA_SESSION * pSession,
														   const void * pDictionaryData)
{
	/* Input validation */
	DECUMA_STATUS status = getSessionStatus(pSession);
	if (status != decumaNoError) return status;
	if ( !pDictionaryData )	return decumaNullPointer;

	return LIBFUNC(DetachDynamicDictionary)(pSession->pLibSession,pDictionaryData);
}


API_DECL_NAME DECUMA_STATUS API_FUNC_NAME(AddWordToDictionary)(DECUMA_SESSION * pSession, void * pDictionaryBuffer, DECUMA_UNICODE * pWord)
{
	DECUMA_STATUS status = getSessionStatus(pSession);
	if (status != decumaNoError) return status;

	if ( !pDictionaryBuffer )
		return decumaNullPointer;

	if ( !pWord )
		return decumaNullPointer;

	if ( !*pWord )
		return decumaEmptyString;


	return LIBFUNC(AddWordToDictionary)(pSession->pLibSession,pDictionaryBuffer,pWord);
}

API_DECL_NAME DECUMA_STATUS API_FUNC_NAME(GetDynamicDictionarySize)(DECUMA_SESSION * pSession, const void * pDictionaryBuffer, int * pSize)
{
	DECUMA_STATUS status = getSessionStatus(pSession);
	if (status != decumaNoError) return status;

	if ( !pDictionaryBuffer )
		return decumaNullPointer;

	if ( !pSize )
		return decumaNullPointer;


	return LIBFUNC(GetDynamicDictionarySize)(pSession->pLibSession,pDictionaryBuffer,pSize);
}

API_DECL_NAME DECUMA_STATUS API_FUNC_NAME(WordExistsInDictionary)(DECUMA_SESSION * pSession, const void * pDictionaryBuffer, DECUMA_UNICODE * pWord)
{
	DECUMA_STATUS status = getSessionStatus(pSession);
	if (status != decumaNoError) return status;

	if ( !pWord )
		return decumaNullPointer;

	if ( !*pWord )
		return decumaEmptyString;

	if (!pDictionaryBuffer)
		return decumaNullDictionaryPointer;

	return LIBFUNC(WordExistsInDictionary)(pSession->pLibSession, pDictionaryBuffer, pWord);
}

API_DECL_NAME DECUMA_STATUS API_FUNC_NAME(AddAllWords)(DECUMA_SESSION * pSession,
											   void * pDictionaryBuffer,
											   const DECUMA_UNICODE * pWordBuffer,
											   int nUnicodesInBuffer,
											   int bNullDelimitedOnly)
{
	DECUMA_STATUS status = getSessionStatus(pSession);
	if (status != decumaNoError) return status;

	if (!pDictionaryBuffer) return decumaNullPointer;
	if (!pWordBuffer) return decumaNullTextPointer;
	if (nUnicodesInBuffer <= 0) return decumaEmptyString;

	return LIBFUNC(AddAllWords)(pSession->pLibSession,pDictionaryBuffer,pWordBuffer,nUnicodesInBuffer,bNullDelimitedOnly);
}

/******* Word completion functions *******/

#ifdef DECUMA_HWR_ENABLE_EXTRA_API
DECUMA_STATUS API_FUNC_NAME(CompleteWord)(DECUMA_SESSION * pSession,
												DECUMA_COMPLETION_RESULT * pResults,
												DECUMA_UINT32 nMaxResults,
												DECUMA_UINT32 * pnResults,
												DECUMA_UINT32 nMaxCharsPerResult,
												DECUMA_UNICODE * pChars,
												DECUMA_UINT16 nChars)
{
	DECUMA_STATUS status = getSessionStatus(pSession);
	if (status != decumaNoError) return status;

	if ( !pResults ) return decumaNullResultPointer;
	if ( !pnResults ) return decumaNullPointer;
	if ( !pChars ) return decumaNullTextPointer;
	if ( !*pChars || nChars == 0 ) return decumaEmptyString;

	return LIBFUNC(CompleteWord)(pSession->pLibSession,pResults,nMaxResults,
		pnResults,nMaxCharsPerResult,pChars,nChars);
}
#endif /* DECUMA_HWR_ENABLE_EXTRA_API */

/*///////////////////////////////////////////////////////////////////////////////////////// */




/******* Functions in decuma_hwr_extra.h *******//*//////////////// */

#ifdef DECUMA_HWR_ENABLE_EXTRA_API

DECUMA_STATUS API_FUNC_NAME(SetForcedSegmentation)( DECUMA_SESSION * pSession,
	DECUMA_INT16 * pForceSymbolStrokes,
	DECUMA_INT16 nForceSymbols)
{
	DECUMA_STATUS status = getSessionStatus(pSession);
	if (status != decumaNoError) return status;

	status = LIBFUNC(SetForcedSegmentation)(pSession->pLibSession, pForceSymbolStrokes, nForceSymbols);

#ifdef DEBUG_OUTPUT
	fprintf(g_debugFile,"SetForcedSegmentation nForceSymbols==%d, status=%d\n",nForceSymbols,status);
	fflush(g_debugFile);
#endif /*DEBUG_OUTPUT */

	return status;
}

const DECUMA_INT16 * API_FUNC_NAME(GetForcedSegmentation)(DECUMA_SESSION * pSession, DECUMA_INT16 * pnForceSymbols)
{
	DECUMA_STATUS status = getSessionStatus(pSession);
	if (status != decumaNoError)
		return NULL;

	return LIBFUNC(GetForcedSegmentation)(pSession->pLibSession, pnForceSymbols);
}

DECUMA_STATUS API_FUNC_NAME(SetDictionaryFilterString)(DECUMA_SESSION * pSession,
											 const DECUMA_UNICODE * symbolString)
{
	DECUMA_STATUS status = getSessionStatus(pSession);
	if (status != decumaNoError) return status;

	return LIBFUNC(SetDictionaryFilterString)(pSession->pLibSession, symbolString);
}

DECUMA_UNICODE * API_FUNC_NAME(GetDictionaryFilterString)(DECUMA_SESSION * pSession)
{
	DECUMA_STATUS status = getSessionStatus(pSession);
	if (status != decumaNoError)
		return NULL;

	return LIBFUNC(GetDictionaryFilterString)(pSession->pLibSession);
}

DECUMA_STATUS API_FUNC_NAME(ConfirmArcs)( DECUMA_SESSION * pSession)
{
	DECUMA_STATUS status = getSessionStatus(pSession);
	if (status != decumaNoError) return status;

	if (!pSession->bAddingArcs) return decumaFunctionOutsideArcAdditionSeq;

	return LIBFUNC(ConfirmArcs)(pSession->pLibSession);
}

DECUMA_STATUS API_FUNC_NAME(EvaluateArcs)( DECUMA_SESSION * pSession)
{
	DECUMA_STATUS status = getSessionStatus(pSession);
	if (status != decumaNoError) return status;

	if (!pSession->bAddingArcs) return decumaFunctionOutsideArcAdditionSeq;

	return LIBFUNC(EvaluateArcs)(pSession->pLibSession);
}

DECUMA_STATUS API_FUNC_NAME(RecognizeExtraOutput)( DECUMA_SESSION * pSession,
	DECUMA_HWR_RESULT * pResults,
	DECUMA_UINT16 nMaxResults,
	DECUMA_UINT16 * pnResults,
	DECUMA_UINT16 nMaxCharsPerResult,
	void *  pCharArcIdxData, /*Should point to an array of size nMaxResults * nMaxCharsPerResult * */
	const DECUMA_RECOGNITION_SETTINGS * pRecognitionSettings,
	const DECUMA_INTERRUPT_FUNCTIONS * pInterruptFunctions)
{
	DECUMA_STATUS status = getSessionStatus(pSession);
	if (status != decumaNoError) return status;

	if (!pResults) return decumaNullResultPointer;
	if (pResults->pChars == NULL) return decumaNullPointer;
	if (!pnResults) return decumaNullPointer;
	if (!pSession->bAddingArcs) return decumaFunctionOutsideArcAdditionSeq;

	status = validateRecognitionSettings(pRecognitionSettings);
	if (status != decumaNoError) return status;

	/* Should more parameters be validated? */

	return LIBFUNC(RecognizeExtraOutput)(pSession->pLibSession, pResults, nMaxResults, pnResults, nMaxCharsPerResult, pCharArcIdxData, pRecognitionSettings,
		pInterruptFunctions);
}

int API_FUNC_NAME(GetCharArcIdxDataSize)( DECUMA_UINT16 nMaxResults, DECUMA_UINT16 nMaxCharsPerResult)
{
	return LIBFUNC(GetCharArcIdxDataSize)(nMaxResults, nMaxCharsPerResult);
}

const DECUMA_UINT16 * API_FUNC_NAME(GetCharArcIdxs)( const void * pCharArcIdxData,
														 DECUMA_UINT16 nResultIdx, DECUMA_UINT16 nMaxResults,
														 DECUMA_UINT16 nCharIdx, DECUMA_UINT16 nMaxCharsPerResult,
														 DECUMA_UINT16 * pnArcs)
{
	return LIBFUNC(GetCharArcIdxs)(pCharArcIdxData, nResultIdx, nMaxResults, nCharIdx, nMaxCharsPerResult, pnArcs);
}

long API_FUNC_NAME(GetMemoryStatistics)(char *pBuf,
							   long nBufLen)
{
	return LIBFUNC(GetMemoryStatistics)(pBuf, nBufLen);
}

DECUMA_STATUS API_FUNC_NAME(UsePositionAndScaleInvariantRecognition)( DECUMA_SESSION * pSession, int bOnOff)
{
	DECUMA_STATUS status = getSessionStatus(pSession);
	if (status != decumaNoError) return status;

	if (pSession->bAddingArcs) return decumaFunctionInsideArcAdditionSeq;

	return LIBFUNC(UsePositionAndScaleInvariantRecognition)(pSession->pLibSession, bOnOff);
}

DECUMA_STATUS API_FUNC_NAME(UseTypeConnectionRequirement)( DECUMA_SESSION * pSession, int bOnOff)
{
	DECUMA_STATUS status = getSessionStatus(pSession);
	if (status != decumaNoError) return status;

	if (pSession->bAddingArcs) return decumaFunctionInsideArcAdditionSeq;

	return LIBFUNC(UseTypeConnectionRequirement)(pSession->pLibSession, bOnOff);
}

DECUMA_STATUS API_FUNC_NAME(UseStrictTypeConnectionRequirement)( DECUMA_SESSION * pSession, int bOnOff)
{
	DECUMA_STATUS status = getSessionStatus(pSession);
	if (status != decumaNoError) return status;

	if (pSession->bAddingArcs) return decumaFunctionInsideArcAdditionSeq;

	return LIBFUNC(UseStrictTypeConnectionRequirement)(pSession->pLibSession, bOnOff);
}

int API_FUNC_NAME(GetArcSessionSegmentCount)(DECUMA_SESSION * pSession)
{
	return LIBFUNC(GetArcSessionSegmentCount)(pSession->pLibSession);
}

DECUMA_UINT32 API_FUNC_NAME(GetMaxNumberOfSegments)(void)
{
	return LIBFUNC(GetMaxNumberOfSegments)();
}


DECUMA_UINT32 API_FUNC_NAME(GetArcSessionSize)(void)
{
	return LIBFUNC(GetArcSessionSize)();
}

DECUMA_UINT32 API_FUNC_NAME(GetSGSize)(void)
{
	return LIBFUNC(GetSGSize)();
}

DECUMA_UINT32 API_FUNC_NAME(GetRGSize)(void)
{
	return LIBFUNC(GetRGSize)();
}

#ifdef EXTRA_VERIFICATION_INFO
void * API_FUNC_NAME(GetLibSession)(DECUMA_SESSION * pSession)
{
	return pSession->pLibSession;
}
#endif /* EXTRA_VERIFICATION_INFO */

#endif /* DECUMA_HWR_ENABLE_EXTRA_API */

/*///////////////////////////////////////////////////////////////////////////////////////// */
/* */
/* Private function implementation */
/* */

static DECUMA_STATUS validateRecognitionSettings(const DECUMA_RECOGNITION_SETTINGS * pRecognitionSettings)
{
	if (!pRecognitionSettings)
		return decumaNullSettingsPointer;

	switch (pRecognitionSettings->stringCompleteness)
	{
	case canBeContinued:
	case willNotBeContinued:
		break;
	default:
		return decumaInvalidEnumeratorValue;
	}

	switch (pRecognitionSettings->boostLevel)
	{
	case noBoost:
	case boostDictWords:
	case forceDictWords:
	case boostFrequentChars:
		break;
	default:
		return decumaInvalidEnumeratorValue;
	}

	return LIBFUNC(ValidateRecognitionSettings)(pRecognitionSettings);;
}

static DECUMA_STATUS validateSessionSettings(const DECUMA_SESSION_SETTINGS * pSessionSettings,
											 const DECUMA_MEM_FUNCTIONS * pMemFunctions)
{
	int bIncludesCharSetExtension;

	if ( pSessionSettings == NULL ) return decumaNullSettingsPointer;

	if ( pSessionSettings->pStaticDB && LIBFUNC(ValidateStaticDatabase)(pSessionSettings->pStaticDB)!=decumaNoError) return decumaInvalidDatabase;
	if ( pSessionSettings->pDynamicDB && LIBFUNC(ValidateDynamicDatabase)(pSessionSettings->pDynamicDB)!=decumaNoError) return decumaInvalidUserDatabase;

	if ( pSessionSettings->pStaticDB == NULL && pSessionSettings->pDynamicDB == NULL ) return decumaNullDatabasePointer;

	bIncludesCharSetExtension = pSessionSettings->pCharSetExtension &&
		pSessionSettings->pCharSetExtension[0];

	if ( pSessionSettings->charSet.nSymbolCategories == 0 && !bIncludesCharSetExtension)	return decumaNoSymbolCategories;
	if ( pSessionSettings->charSet.nSymbolCategories > DECUMA_MAX_CATEGORIES) return decumaTooManySymbolCategories;
	if ( pSessionSettings->charSet.pSymbolCategories == NULL && !bIncludesCharSetExtension)return decumaNoSymbolCategories;

	if ( pSessionSettings->charSet.nLanguages == 0 && !bIncludesCharSetExtension) return decumaNoLanguages;
	if ( pSessionSettings->charSet.nLanguages > DECUMA_MAX_CATEGORIES) return decumaTooManyLanguages;
	if ( pSessionSettings->charSet.pLanguages == NULL && !bIncludesCharSetExtension) return decumaNoLanguages;

	/*Character set support by db is checked by module specific code */

	if ( pSessionSettings->UIInputGuide >=numberOfUIInputGuides)
		return decumaInvalidUIInputGuide;

	/* Horizontal UIInputGuide requires supportlines */
	if ( pSessionSettings->UIInputGuide == supportlines ) {

		if ( pSessionSettings->supportLineSet >=numberOfSupportlineSets)
			return decumaInvalidSupportLines;

		switch (pSessionSettings->supportLineSet)
		{
		case baselineAndHelpline:
			if (pSessionSettings->baseline <= pSessionSettings->helpline)
				return decumaInvalidSupportLines;
			break;
		case baselineAndTopline: ;
			if (pSessionSettings->baseline <= pSessionSettings->topline)
				return decumaInvalidSupportLines;
			break;
		case helplineAndTopline:
			if (pSessionSettings->helpline <= pSessionSettings->topline)
				return decumaInvalidSupportLines;
			break;
		default:
			break;
		}
	}

	if (  pSessionSettings->writingDirection >=numberOfDirections)
		return decumaInvalidWritingDirection;

	if (  pSessionSettings->recognitionMode >= numberOfModes)
		return decumaInvalidRecognitionMode;


	return LIBFUNC(ValidateSessionSettings)(pSessionSettings);
}


static DECUMA_STATUS copySessionSettings(DECUMA_SESSION * pSession,
										 const DECUMA_SESSION_SETTINGS *pSessionSettings,
										 DECUMA_UNICODE *pCharSetExtensionCopy)
{
	/* A set of asserts to validate the session settings -- the appropriate error
	 * code, if any, should already have been reported by validateSessionSettings()
	 */

	/* Not necessary for all engines to set staticDB */
	/* decumaAssert(pSessionSettings->pStaticDB); */

	decumaAssert ( pSessionSettings->charSet.nLanguages <= DECUMA_MAX_CATEGORIES);
	decumaAssert ( pSessionSettings->charSet.nLanguages || (pSessionSettings->pCharSetExtension && pSessionSettings->pCharSetExtension[0]));
	decumaAssert ( pSessionSettings->charSet.pLanguages);

	decumaAssert (pSessionSettings->charSet.nSymbolCategories <= DECUMA_MAX_CATEGORIES);
	decumaAssert ( pSessionSettings->charSet.nSymbolCategories || (pSessionSettings->pCharSetExtension && pSessionSettings->pCharSetExtension[0]));
	decumaAssert (pSessionSettings->charSet.pSymbolCategories);

	decumaAssert(pSessionSettings != pSession->pSessionSettings);

	*pSession->pSessionSettings = *pSessionSettings;

	pSession->pSessionSettings->charSet.pSymbolCategories = pSession->activeSettingsCopy == 0 ? pSession->symbolCategoriesCopy0 : pSession->symbolCategoriesCopy1;
	pSession->pSessionSettings->charSet.pLanguages = pSession->activeSettingsCopy == 0 ? pSession->languagesCopy0 : pSession->languagesCopy1;;

	pSession->pSessionSettings->pCharSetExtension = pCharSetExtensionCopy;

	decumaAssert(pSessionSettings->charSet.pSymbolCategories != pSession->pSessionSettings->charSet.pSymbolCategories);

	decumaMemcpy(pSession->pSessionSettings->charSet.pSymbolCategories,
		pSessionSettings->charSet.pSymbolCategories,
		pSessionSettings->charSet.nSymbolCategories * sizeof(pSessionSettings->charSet.pSymbolCategories[0]));

	decumaAssert(pSessionSettings->charSet.pLanguages != pSession->pSessionSettings->charSet.pLanguages);

	decumaMemcpy(pSession->pSessionSettings->charSet.pLanguages,
		pSessionSettings->charSet.pLanguages,
		pSessionSettings->charSet.nLanguages* sizeof(pSessionSettings->charSet.pLanguages[0]));

	/* Log the session settings when changed */
	if ( LOGGING_ENABLED(pSession) )
		LOG_SESSION_SETTINGS(pSession);

	return decumaNoError;
}

static DECUMA_STATUS copyCharSetExtension(DECUMA_SESSION * pSession,
										  const DECUMA_UNICODE *pCharSetExtension,
										  DECUMA_UNICODE **ppCharSetExtension)
{
	*ppCharSetExtension = NULL;

#ifdef DECUMA_NO_DYNAMIC_ALLOCATION
	decumaAssert(pCharSetExtension == NULL || pCharSetExtension[0] == 0);
#else
	if (pCharSetExtension)
	{
		const DECUMA_MEM_FUNCTIONS * pMemFunctions = &pSession->memFunctions;

		int i = 0;
		int charSetExtensionLen;

		if (pCharSetExtension[i])
		{
			/*Copy the char set extension */
			while (pCharSetExtension[i] ||
				pCharSetExtension[i+1])
			{
				i++;
			}
			charSetExtensionLen = (i+2);
		}
		else
			charSetExtensionLen = 1;

		*ppCharSetExtension = decumaCalloc(charSetExtensionLen, sizeof(pCharSetExtension[0]));

		if (!*ppCharSetExtension) return decumaAllocationFailed;

		decumaMemcpy(*ppCharSetExtension,
			pCharSetExtension,
			charSetExtensionLen*sizeof(pCharSetExtension[0]));
	}
#endif

	return decumaNoError;
}


static DECUMA_STATUS getSessionStatus(const DECUMA_SESSION * pSession)
{
	DECUMA_STATUS status;

	if (pSession == NULL) return decumaNullSessionPointer;
	if (pSession->pLibSession != (LIB_SESSION *)&pSession[1]) return decumaSessionNotInitialized;

	status = LIBFUNC(VerifySession)(pSession->pLibSession);
	if (status != decumaNoError) return status;

	return decumaNoError;
}


static void logRecognitionSettings(DECUMA_SESSION * pSession,
								   const DECUMA_RECOGNITION_SETTINGS *pRecognitionSettings)
{
	if (LOGGING_ENABLED(pSession)) {
		char pBuf[INT32_STRING_MAX_LEN];

		if (pSession->logData.state == LOG_STATE_RECOGNITION_DONE) {
			/* No result accepted or reject from last interpretation, so we reject it */
			API_FUNC_NAME(LogAcceptedResult)(pSession, NULL, 0);
		}

		LOG_STRING_LITERAL(pSession, "S| # RECOGNITION_SETTINGS boostLevel ");


		if (decumaIToA(pRecognitionSettings->boostLevel, 10, pBuf, INT32_STRING_MAX_LEN))
			LOG_STRING(pSession, pBuf);

		LOG_STRING_LITERAL(pSession, "\n");

		LOG_STRING_LITERAL(pSession, "S| # RECOGNITION_SETTINGS stringCompleteness ");
		if (decumaIToA(pRecognitionSettings->stringCompleteness, 10, pBuf, INT32_STRING_MAX_LEN))
			LOG_STRING(pSession, pBuf);

		LOG_STRING_LITERAL(pSession, "\n");

		if (pRecognitionSettings->pStringStart != NULL) {
			int j;
			LOG_STRING_LITERAL(pSession, "A| ");
			decumaIToA(pSession->logData.nStartArcIdx, 10, pBuf, INT32_STRING_MAX_LEN);
			LOG_STRING(pSession, pBuf);
			LOG_STRING_LITERAL(pSession, " # STARTSTRING '");
			for (j = 0; pRecognitionSettings->pStringStart[j] != 0; j++) {
				if (decumaIToA(pRecognitionSettings->pStringStart[j], 16, pBuf, INT32_STRING_MAX_LEN)) {
					LOG_STRING_LITERAL(pSession, "\\x");
					LOG_STRING(pSession, pBuf);
				}
			}
			LOG_STRING_LITERAL(pSession, "'\n");
		}
		else {
			LOG_STRING_LITERAL(pSession, "A| ");
			decumaIToA(pSession->logData.nRecognitionStartArcIdx, 16, pBuf, INT32_STRING_MAX_LEN);
			LOG_STRING(pSession, pBuf);
			LOG_STRING_LITERAL(pSession, " # NO_STARTSTRING_ARC_ID_START");
			LOG_STRING_LITERAL(pSession, "\n");
		}
	}
}

/*If error */

static void logErrorDuringRecognize(DECUMA_SESSION * pSession, DECUMA_STATUS status)
{
	if (LOGGING_ENABLED(pSession)) {
		char pBuf[INT32_STRING_MAX_LEN];
		LOG_STRING_LITERAL(pSession, "C| recognition error: ");
		if (decumaIToA(status, 10, pBuf, sizeof(pBuf))) {
			LOG_STRING(pSession, pBuf);
		}
		LOG_STRING_LITERAL(pSession, "\n");
	}
}

/*Last */
static void logRecognitionResults(DECUMA_SESSION * pSession,
								  const DECUMA_HWR_RESULT *pResults,
								  DECUMA_UINT16 nResults)
{
	if (LOGGING_ENABLED(pSession)) {

		/* Log results in DI-file format */

		char pBuf[INT32_STRING_MAX_LEN];
		int i, j;

		LOG_STRING_LITERAL(pSession, "I| ");

		if (nResults > 0) {
			for (i = 0; i < nResults; i++) {
				int nStringStart = 0;
				LOG_STRING_LITERAL(pSession, "'");

				for (j =  nStringStart; j < pResults[i].nChars; j++) {
					if (decumaIToA(pResults[i].pChars[j], 16, pBuf, INT32_STRING_MAX_LEN)) {
						LOG_STRING_LITERAL(pSession, "\\x");
						LOG_STRING(pSession, pBuf);
					}
				}

				if (decumaIToA(pResults[i].distance, 10, pBuf, INT32_STRING_MAX_LEN)) {

					LOG_STRING_LITERAL(pSession, "' (m");
					LOG_STRING(pSession, pBuf);

					if (decumaIToA(pResults[i].stringType, 10, pBuf, INT32_STRING_MAX_LEN)) {
						LOG_STRING_LITERAL(pSession, "t");
						LOG_STRING(pSession, pBuf);
					}
					if (decumaIToA(i, 10, pBuf, INT32_STRING_MAX_LEN)) {
						LOG_STRING_LITERAL(pSession, "c");
						LOG_STRING(pSession, pBuf);
					}

					LOG_STRING_LITERAL(pSession, ") ");
				}
			}
		}
		else {
			LOG_STRING_LITERAL(pSession, "'<Symbol not recognized>' ");
		}

		LOG_STRING_LITERAL(pSession, ";");

		for (i = 0; i < pSession->logData.nArcsAdded; i++) {
			if (decumaIToA(pSession->logData.nStartArcIdx + i, 10, pBuf, INT32_STRING_MAX_LEN)) {
				LOG_STRING_LITERAL(pSession, " ");
				LOG_STRING(pSession, pBuf);
			}
		}

		LOG_STRING_LITERAL(pSession, "\n");

		pSession->logData.nRecognitionStartArcIdx = pSession->logData.nStartArcIdx;
		pSession->logData.nRecognitionArcsUsed    = pSession->logData.nArcsAdded;
		pSession->logData.state                   = LOG_STATE_RECOGNITION_DONE;
	}
}

/* Undefine private macros */
#undef INT32_STRING_MAX_LEN
#undef SESSION_NOT_INITIALIZED
#undef LOG_STRING
#undef LOG_STRING_LITERAL
#undef LOGGING_ENABLED
#undef LOG_SESSION_SETTINGS

/*********** END ***********/
