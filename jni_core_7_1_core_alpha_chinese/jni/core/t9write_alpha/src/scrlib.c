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

        File: scrLib.c
     Package: This file is part of the package SCRLIB
   Copyright: Zi Corporation (2006)
 Information: www.zicorp.com

\******************************************************************/

#include "scrVersion.h"

#include "decumaMath.h"
#include "decumaString.h"
#include "decumaMemory.h"
#include "decumaAssert.h"
#include "decumaResamp.h"
#include "decumaCategoriesHidden.h"
#include "decumaCategoryTranslation.h"

#include "scrAPI.h"
#include "scrlib.h"
#include "scrlibExtra.h"
#include "scrHeapMem.h"
#include "database.h"
#include "udm.h"
#include "udmAccess.h"
#include "decumaBasicTypesMinMax.h"

#ifdef __CAMBOX
#define REMOVE_TOO_SMALL_ARCS
#endif

#ifdef REMOVE_TOO_SMALL_ARCS
#include "scrArcFilter.h"
#endif


#define ONE_LAP (360) /* The number of degrees for one lap. */

#ifdef __NITRO__
#include <nitro.h>
#include <nitro/version_begin.h>
static char id_string[] = SDK_MIDDLEWARE_STRING("Zi Corporation", "SCRLIB");
#include <nitro/version_end.h>
#endif

typedef struct {
	/*Used in scrRecognize */
	SCR_API_SETTINGS sets;
	SCR_OUTPUT scrOutputs[MAX_NUMBER_OF_OUTPUTS];
	CURVE curve;
	ARC arcs[ MAX_NUMBER_OF_ARCS_IN_CURVE ];

	/*Used by the rest of the scrlib */
	SCR_HEAP_MEM scrHeap;

} SCRLIB_HEAP_MEM;

static DECUMA_STATUS checkPointersAndNumberOfArcs(const DECUMA_CURVE * pCurve,
	SCR_RESULT * pResults,
	unsigned int nResults,
	int * pnResultsReturned,
	SCRLIB_HEAP_MEM *);

static DECUMA_STATUS checkCurve(const DECUMA_CURVE * pCurve);

static DECUMA_STATUS checkAndConvertSettings(const SCR_SETTINGS * pSettings,
	SCR_API_SETTINGS * pScrApiSettings);

static int writeResults(SCR_RESULT * pResults, SCR_OUTPUT * pResultOutputs,
						int nResults, SCR_OUTPUT * pSCRout, int nOutputs);

static DECUMA_STATUS decumaCurveToCurve(const DECUMA_CURVE * pDecumaCurve, CURVE * pCurve);

#define MINIMAL_STACK_LEFT	16000

#if defined(MEASURE_STACK_SIZE)
static void stackMeasureInit(int **aStackFloor, int **aStackRoof)
{
	int stackVar[MINIMAL_STACK_LEFT/4-128];
	int i;

	int * stackP = stackVar;

	/*CleanStack: Write a special number (0xAAAAAAAA) from the */
	/*stack pointer and down (stack is written top-down) */

	*aStackRoof = stackP;

	for(i = 0 ; i < sizeof(stackVar) / sizeof(stackVar[0]) ; i++)
	{
		*stackP++ = 0xAAAAAAAA;
		/*fprintf(pf,"stackP (LOOP): %x \n",stackP); */
	}
	*aStackFloor = stackP-1;
}
#endif /*_DEBUG && MEASURE_STACKSIZE */

unsigned int scrGetMemoryBufferSize(void)
{
	return sizeof(SCRLIB_HEAP_MEM);
}


DECUMA_STATUS scrRecognize(const DECUMA_CURVE * pCurve,
			SCR_RESULT * pResults,
			unsigned int nResults, int * pnResultsReturned,
			const SCR_SETTINGS * pSettings,
			void * pMemoryBuffer,
			const DECUMA_INTERRUPT_FUNCTIONS * pInterruptFunctions)
{
	return scrRecognizeExtra(pCurve, pResults, NULL, nResults, pnResultsReturned, pSettings, pMemoryBuffer,pInterruptFunctions);
}


DECUMA_STATUS scrRecognizeExtra(
			const DECUMA_CURVE * pCurve,
			SCR_RESULT * pResults,
			SCR_OUTPUT * pResultOutputs,
			unsigned int nResults,
			int * pnResultsReturned,
			const SCR_SETTINGS * pSettings,
			void * pMemoryBuffer,
			const DECUMA_INTERRUPT_FUNCTIONS * pInterruptFunctions)
{
	DECUMA_STATUS status;
#if defined(MEASURE_STACK_SIZE)
	int * stackP;
	unsigned int stackUsed;
	int * stackFloor;
	int * stackRoof;

	stackMeasureInit(&stackFloor,&stackRoof);
	{
#endif /*MEASURE_STACK_SIZE */
	unsigned int i;

	int nOutputs;
	int nOutputsReturned = 0;
	int nWritten = 0;
	SCRLIB_HEAP_MEM * pScrlibHeap =  (SCRLIB_HEAP_MEM*) pMemoryBuffer;

#if defined(__NITRO__)
	SDK_USING_MIDDLEWARE(id_string);
#endif

	status = checkPointersAndNumberOfArcs( pCurve, pResults, nResults, pnResultsReturned, pScrlibHeap);

	if ( status == decumaNoError )
	{
		myMemSet(pScrlibHeap, 0, sizeof(SCRLIB_HEAP_MEM));
		status = checkAndConvertSettings(pSettings, &pScrlibHeap->sets);
	}

	/*JM TODO: Should the comparicon below instead be against  */
	/*         pKeyDB->nMaxArcsInCurve */
	if ( status == decumaNoError && pCurve->nArcs > MAX_NUMBER_OF_ARCS_IN_CURVE)
	{	/* Too many arcs are supplied. It is not considered to be an  */
		/* error, but no interpretation can be done. */
		if (pnResultsReturned)
			*pnResultsReturned = 0;
		return status;
	}

	if ( status == decumaNoError)
	{
		for(i = 0; i < MAX_NUMBER_OF_ARCS_IN_CURVE; i++)
		{
			pScrlibHeap->curve.pArc[i] = &pScrlibHeap->arcs[i];
		}

		status = decumaCurveToCurve(pCurve, &pScrlibHeap->curve);

		if (status != decumaNoError) {
			if (pnResultsReturned)
				*pnResultsReturned = 0;
			return status;
		}

        nOutputs = minNUMB(nResults, sizeof(pScrlibHeap->scrOutputs)/
        	sizeof(pScrlibHeap->scrOutputs[0]) );

        /* Check that the cast to INT16 is ok here!! */

		status = scrSelect(pScrlibHeap->scrOutputs, &nOutputsReturned, nOutputs,
			&pScrlibHeap->sets, &pScrlibHeap->curve,NULL,
			(void *) &pScrlibHeap->scrHeap,pInterruptFunctions);

		if (status != decumaNoError) {
			if (pnResultsReturned)
				*pnResultsReturned = 0;
			return status;
		}

		decumaAssert(nOutputsReturned >= 0);
		decumaAssert(nOutputsReturned <= nOutputs);

		nWritten = writeResults(pResults, pResultOutputs, nResults,
			pScrlibHeap->scrOutputs, nOutputsReturned);
	}

	if (pnResultsReturned)
		*pnResultsReturned = nWritten;

#if defined(MEASURE_STACK_SIZE)
	}
	/*Measure Stack: See how many of the special number (0xAAAAAAAA) that are */
	/*still in the allocated stack space.  */
	stackP = stackRoof;

	while(*stackP == 0xAAAAAAAA && stackP < stackFloor)
	{
		stackP++;
	}

	stackUsed = (unsigned int)stackFloor - (unsigned int)stackP;
	/*fprintf(pf ,"StackUsed: %d\n", stackUsed); */
#endif /*defined(MEASURE_STACK_SIZE) */

	return status;
} /* scrRecognize() */

void scrGetCurve(SCR_CURVE* pScrCurve, void * pMemoryBuffer)
{
	SCRLIB_HEAP_MEM * pScrlibHeap =  (SCRLIB_HEAP_MEM*) pMemoryBuffer;

	_scrGetCurve(pScrCurve, &pScrlibHeap->scrHeap);
}

void scrGetCurveProp(const SCR_OUTPUT* pOutput, SCR_CURVE_PROP* pScrCurveProp, void * pMemoryBuffer)
{
	SCRLIB_HEAP_MEM * pScrlibHeap =  (SCRLIB_HEAP_MEM*) pMemoryBuffer;

	_scrGetCurveProp(pOutput, pScrCurveProp, &pScrlibHeap->scrHeap);
}

DECUMA_STATUS scrIsOutputEstimateReliable(const SCR_OUTPUT* pOutput,
										  int *pnIsReliable)
{
	return _scrIsOutputEstimateReliable(pOutput, pnIsReliable);
}

DECUMA_STATUS scrEstimateScalingAndVerticalOffset(const SCR_OUTPUT* pOutput,
												  DECUMA_COORD* pnBaseLineYEstimate,
												  DECUMA_COORD* pnHelpLineYEstimate)
{
	return _scrEstimateScalingAndVerticalOffset(pOutput, pnBaseLineYEstimate, pnHelpLineYEstimate);
}

DECUMA_STATUS scrGetScalingAndVerticalOffsetPunish(const SCR_OUTPUT* pOutput,
												   const SCR_CURVE_PROP* pScrCurveProp,
												   DECUMA_COORD nBaseLineY,
												   DECUMA_COORD nHelpLineY,
												   DECUMA_INT16 *pnPunish)
{
	return _scrGetScalingAndVerticalOffsetPunish(pOutput, pScrCurveProp, nBaseLineY, nHelpLineY, pnPunish);
}

DECUMA_STATUS scrGetRotationPunish(const SCR_OUTPUT* pOutput,
								   const SCR_CURVE_PROP* pScrCurveProp,
								   DECUMA_INT16 nBaseLineY,
								   DECUMA_INT16 nHelpLineY,
								   int nRefAngle,
								   DECUMA_INT16 *pnPunish)
{
	return _scrGetRotationPunish(pOutput, pScrCurveProp, nBaseLineY, nHelpLineY, nRefAngle, pnPunish);
}

DECUMA_INT16 scrGetSymmetry(const SCR_OUTPUT* pOutput)
{
	return scrOutputGetSymmetry(pOutput);
}

DECUMA_STATUS scrAdjustMuAndPunish(SCR_OUTPUT* pOutput,
								   const SCR_CURVE_PROP* pScrCurveProp,
								   DECUMA_COORD nNewBaseLineY,
								   DECUMA_COORD nNewHelpLineY,
								   DECUMA_COORD nOldBaseLineY,
								   DECUMA_COORD nOldHelpLineY)
{
	return _scrAdjustMuAndPunish(pOutput, pScrCurveProp, nNewBaseLineY, nNewHelpLineY, nOldBaseLineY, nOldHelpLineY);
}

DECUMA_STATUS scrOutputIsGesture(const SCR_OUTPUT* pOutput,
								int * pbGesture, int * pbInstantGesture)
{
	return _scrOutputIsGesture(pOutput,pbGesture, pbInstantGesture);
}

int scrDatabaseSupportsSymbolCategory(SCR_ANY_DB_PTR pDB, DECUMA_UINT32 cat)
{
	DECUMA_CHARACTER_SET charSet;
	CATEGORY_TABLE_PTR pCatTable;
	int bDBIsUDM = 0;
	DECUMA_STATUS status;

	if ((status = scrCheckStaticDB(pDB)) == decumaNoError)
		decumaAssert(!udmIsValid( (UDM_PTR) pDB));
	else if (udmIsValid( (UDM_PTR) pDB))
		bDBIsUDM = 1;
	else
		return 0;

	charSet.pSymbolCategories = &cat;
	charSet.nSymbolCategories = 1;
	charSet.pLanguages = NULL;
	charSet.nLanguages = 0;

	status = checkCharacterSetValidity(&charSet,1);
	if ( status != decumaNoError)
	{
		return 0;
	}

	pCatTable = keyDBGetStaticCategoryTable(bDBIsUDM ? (KEY_DB_HEADER_PTR)udmGetDatabase(pDB) : staticDBGetKeyDB(pDB));

	status = translateToCategoryStructs(&charSet,pCatTable,NULL,NULL,NULL);

	return (status==decumaNoError);
}

int scrDatabaseSupportsLanguage(SCR_ANY_DB_PTR pDB, DECUMA_UINT32 lang)
{
	DECUMA_CHARACTER_SET charSet;
	CATEGORY_TABLE_PTR pCatTable;
	int bDBIsUDM = 0;
	DECUMA_STATUS status;

	if ((status = scrCheckStaticDB(pDB)) == decumaNoError)
		decumaAssert(!udmIsValid( (UDM_PTR) pDB));
	else if (udmIsValid( (UDM_PTR) pDB))
		bDBIsUDM = 1;
	else
		return 0;

	charSet.pSymbolCategories = NULL;
	charSet.nSymbolCategories = 0;
	charSet.pLanguages = &lang;
	charSet.nLanguages = 1;

	pCatTable = keyDBGetStaticCategoryTable(bDBIsUDM ? (KEY_DB_HEADER_PTR)udmGetDatabase(pDB) : staticDBGetKeyDB(pDB));

	status = checkCharacterSetValidity(&charSet,1);
	if ( status != decumaNoError)
	{
		return 0;
	}

	status = translateToCategoryStructs(&charSet,pCatTable,NULL,NULL,NULL);

	return (status==decumaNoError);
}

DECUMA_STATUS scrDatabaseIncludesSymbol(SCR_ANY_DB_PTR pDB,
							  const DECUMA_CHARACTER_SET * pCharSet,
							  const DECUMA_UNICODE * pSymbol,
							  int * pbIncluded)
{
	int bDBIsUDM = 0;
	DECUMA_STATUS status;

	if ((status = scrCheckStaticDB(pDB)) == decumaNoError)
		decumaAssert(!udmIsValid( (UDM_PTR) pDB));
	else if (udmIsValid( (UDM_PTR) pDB))
		bDBIsUDM = 1;
	else
		return decumaInvalidDatabase;

	if (!pbIncluded) return decumaNullPointer;
	if (!pSymbol) return decumaNullTextPointer;

	if (pCharSet)
	{
		status = checkCharacterSetValidity(pCharSet,1);
		if ( status != decumaNoError)
			return status;
	}

	return databaseIncludesSymbol(bDBIsUDM ? (KEY_DB_HEADER_PTR)udmGetDatabase(pDB) : staticDBGetKeyDB(pDB),
		bDBIsUDM ? NULL : staticDBGetPropDB(pDB),
		pCharSet,pSymbol,pbIncluded);
}

DECUMA_STATUS scrCheckStaticDB(STATIC_DB_PTR pDB)
{
	if (!VALID_DECUMA_BASIC_TYPES) return decumaCompilationError;

	if ( !pDB )
		return decumaNullDatabasePointer;

	if ( databaseHasWrongFormat((STATIC_DB_HEADER_PTR) pDB) )
		return decumaInvalidDatabase;

	return decumaNoError;
}

const char * scrGetVersion(void)
{
	return SCR_VERSION;
}

CATEGORY_TABLE_PTR scrDatabaseGetCatTable(SCR_ANY_DB_PTR pDB)
{
	int bDBIsUDM = 0;

	if (scrCheckStaticDB(pDB) == decumaNoError)
		decumaAssert(!udmIsValid( (UDM_PTR) pDB));
	else if (udmIsValid( (UDM_PTR) pDB))
		bDBIsUDM = 1;
	else
		return NULL;

	return keyDBGetStaticCategoryTable(bDBIsUDM ? (KEY_DB_HEADER_PTR)udmGetDatabase(pDB) : staticDBGetKeyDB(pDB));
}

int scrOutputInCategory(const SCR_OUTPUT* pOutput,
						const CATEGORY Category)
{
	return _scrOutputInCategory(pOutput, Category);
}

/* Verify that the symbol we are about to add is not already in the result set */
/* This was discovered to occur for 'z' by MediaTek, we would get a SCR_OUTPUT array containing */
/* { symbol: 'Z', outSymbol: 'original'} and { symbol: 'z', outSymbol: 'alternative' or 'both' }, */
/* generating a result array with two upper-case Z's */
static int okToAdd(SCR_RESULT * pResults, SCR_RESULT * pCandidate)
{
	decumaAssert(pCandidate);
	decumaAssert(pResults);
	decumaAssert(pCandidate->pText);

	for ( ;  pResults < pCandidate; ++pResults )
	{
		decumaAssert(pResults->pText);
		if (decumaStrcmp(pResults->pText,pCandidate->pText)==0)
			return 0; /*We already have one candidate with identical string. */
	}
	return 1; /*OK to add since the string is unique */
}

/* Writes the result to pResult. pResult must point to at least nResult SCR_RESULT eleemnts. */
/* pSCRout must point to at least nOutputs scrOUTPUT elements. */

/* Returns the number of written results; */
static int writeResults(SCR_RESULT * pResults, SCR_OUTPUT * pResultOutputs,
						int nResults, SCR_OUTPUT * pSCRout, int nOutputs)
{
	int a;
	int nWritten = 0;
	int prox;

	SCR_RESULT * pFirstResult = pResults;

	if ( pResults == NULL || pSCRout == NULL )
		 return nWritten;

	for (a = 0; a < nOutputs && nWritten < nResults; a++, pSCRout++)
	{
		prox = pSCRout->mu + pSCRout->punish;
		prox = minNUMB(MAX_PROXIMITY, prox);
		pResults->proximity = prox;
		if (pResultOutputs)	*pResultOutputs = *pSCRout;

		if ( pSCRout->outSymbol == alternative )
		{	/* The symbol shall be upper case!! */
			if ( pResults->pText )
			{
				decumaStrncpyToUpper(pResults->pText, pSCRout->symbol,
					pResults->nMaxText, 1);
			}
		}
		else if ( pSCRout->outSymbol == both )
		{	/* Both upper and lower symbols shall be returned */
			if ( pResults->pText )
			{
				decumaStrncpy(pResults->pText, pSCRout->symbol,
						pResults->nMaxText);
			}
			/* An extra output might be returned */
			if ( okToAdd(pFirstResult, pResults) )
			{
				nWritten++;
				pResults++;
				if (pResultOutputs) pResultOutputs++;
			}

			if ( nWritten < nResults )
			{ /* Return the upper case symbol! */
				if ( pResults->pText )
				{
					decumaStrncpyToUpper(pResults->pText, pSCRout->symbol,
						pResults->nMaxText, 1);
				}
				pResults->proximity = prox;
				if (pResultOutputs)	*pResultOutputs = *pSCRout;
			}
			else
			{
				continue;
			}
		}
		else
		{ /* Default (Lover case symbol) */
			if ( pResults->pText )
			{
				decumaStrncpy(pResults->pText, pSCRout->symbol,
						pResults->nMaxText);
			}
		}

		if ( okToAdd(pFirstResult, pResults) )
		{
			nWritten++;
			pResults++;
			if (pResultOutputs) pResultOutputs++;
		}
	}

	return nWritten;
} /* writeResults() */

static DECUMA_STATUS checkPointersAndNumberOfArcs(
	const DECUMA_CURVE * pCurve, SCR_RESULT * pResults,
	unsigned int nResults, int * pnResultsReturned,
	SCRLIB_HEAP_MEM * pScrlibHeap)
{
	DECUMA_STATUS status;
	int k;
	if ( pnResultsReturned == NULL )
		return decumaNullPointer;
	/* If anything else fails this has been set appropriately */
	*pnResultsReturned = 0;

	status = checkCurve(pCurve);

	if (status != decumaNoError) return status;

	if ( pCurve->nArcs <= 0 )
		return decumaCurveZeroArcs;

	if ( pResults == NULL )
		return decumaNullResultPointer;
	else {
		for (k = 0; k < (int)nResults; k++)
		{
			/* Um, can this ever be true? */
			if ( &pResults[k] == NULL )
				return decumaNullPointer;

			if ( pResults[k].pText == NULL )
				return decumaNullTextPointer;

			/* There shall be room for a terminating zero */
			if ( pResults[k].nMaxText <= 1 )
				return decumaTooShortBuffer;
		}
	}

	if ( pScrlibHeap == NULL)
		return decumaNullMemoryBufferPointer;

	return decumaNoError;
} /* checkPointersAndNumberOfArcs() */

static DECUMA_STATUS checkCurve(const DECUMA_CURVE * pCurve)
{
	int k;

	if (pCurve == NULL)
		return decumaNullCurvePointer;

	if ( pCurve->pArcs == NULL )
		return decumaNullArcPointer;
	else
	{
		for ( k = 0; k < pCurve->nArcs; k++)
		{
			DECUMA_ARC * pArc = &pCurve->pArcs[k];

			if ( pArc->pPoints == NULL )
				return decumaNullPointPointer;

			if ( pArc->nPoints <= 0 )
				return decumaArcZeroPoints;
		}
	}

	if ( pCurve->nArcs <= 0 )
		return decumaCurveZeroArcs;

	return decumaNoError;
}

static DECUMA_STATUS checkAndConvertSettings(const SCR_SETTINGS * pSettings,
	SCR_API_SETTINGS * pScrApiSettings)
{
	decumaAssert(pScrApiSettings!=0);

	if ( pSettings == NULL )
		return decumaNullSettingsPointer;

	if (pSettings->pCatTable &&
		!dcCategoryTableIsOK(pSettings->pCatTable))
		return decumaInvalidCategoryTable;

	if ( pSettings->refangle < 0 ||
		 pSettings->refangle >= ONE_LAP )
		 return decumaInvalidRefAngle;

	/* refangleimportance is unsigned, the first test is always false */
	if ( pSettings->refangleimportance < 0 ||
		 pSettings->refangleimportance > FULL_ROTATION_IMPORTANCE )
		 return decumaInvalidRefAngleImportance;

	if ( pSettings->refangle != 0 )
	{	/* baseline and helpline cannot be supplied if the refAngle != 0 */
		if ( pSettings->nBaseLineY != 0 || pSettings->nHelpLineY != 0 )
			return decumaInvalidBaselineHelpline;
	}
	else
	{	/* the helpline must be above the baseline */
		/* Note the Y-coords are upside down. */
		if ( pSettings->nBaseLineY < pSettings->nHelpLineY )
			return decumaInvalidBaselineHelpline;
	}

	/* Check bounds of input values; these are 32-bit values in SCR_SETTINGS, but only 16-bit in SCR_API_SETTINGS */
	if ( pSettings->nBaseLineY > MAX_INT16 || pSettings->nBaseLineY < MIN_INT16)
	{
		return decumaInvalidBaselineHelpline;
	}
	if ( pSettings->nHelpLineY > MAX_INT16 || pSettings->nHelpLineY < MIN_INT16)
	{
		return decumaInvalidBaselineHelpline;
	}

	/* Each field of pScrApiSettings is set below, so this memset is not strictly nessecary ... */
	decumaMemset(pScrApiSettings, 0, sizeof(SCR_API_SETTINGS));

	pScrApiSettings->nBaseLineY = (DECUMA_INT16)pSettings->nBaseLineY; /*Casting is safe due to check above */
	pScrApiSettings->nHelpLineY = (DECUMA_INT16)pSettings->nHelpLineY; /*Casting is safe due to check above */
	pScrApiSettings->characterSet = pSettings->characterSet;
	pScrApiSettings->nRefAngle = (DECUMA_INT16)((pSettings->refangle * 2 * PI) / ONE_LAP); /* Note: PI is scaled with 100 */
	pScrApiSettings->nRefAngleImportance = pSettings->refangleimportance;
	pScrApiSettings->pUDM = pSettings->pUDM;
	pScrApiSettings->pCategoryTable = pSettings->pCatTable;
	pScrApiSettings->pStaticDB = pSettings->pDB;

	return  scrCheckSettings(pScrApiSettings);

} /*checkAndConvertSettings() */

static DECUMA_STATUS decumaCurveToCurve(const DECUMA_CURVE * pDecumaCurve, CURVE * pCurve)
{
	int a;
	int nAddedArcs;
	DECUMA_STATUS status;

	/* Build the curve. */
	/* 1. Convert the input arcs to 32 point arcs */
	/* 2. Attach the arcs to the curve (done in the same loop as 1.) */
	for(a = 0 ; a < pDecumaCurve->nArcs; a++)
	{
		DECUMA_ARC * pArc = &pDecumaCurve->pArcs[a];

		/* Recalculate the arc and fit it into the NUMBER_OF_POINTS_IN_ARC point SCR_ARC. */
		status = resampArc(pArc->pPoints, pArc->nPoints, pCurve->pArc[a]->point,
				  sizeof( pCurve->pArc[a]->point ) / sizeof( pCurve->pArc[a]->point[0] ),
				  0,0);
		if (status != decumaNoError) return status;
	}

	/* Check which arcs that shall be added  */
	/* Too small arcs are skipped if "REMOVE_TOO_SMALL_ARCS" is defined */
	for(a = 0, nAddedArcs = 0; a < pDecumaCurve->nArcs - 1; a++)
	{
#ifdef REMOVE_TOO_SMALL_ARCS
		if ( arcIsTooSmallAndCloseToNextArc(pCurve->pArc[a], &pScrlibHeap->arcs[a+1],
				pSettings->nBaseLineY, pSettings->nHelpLineY) )
		{
			/* This arc is too small to be considered as a real arc */
			/* Just skip this arc. */
		}
		else
#endif
		{
			/* This arc shall be included in the curve */
			pCurve->pArc[nAddedArcs] = pCurve->pArc[a];
			if ( pDecumaCurve->pArcTimelineDiff ) {
				decumaAssert(pDecumaCurve->pArcTimelineDiff[a] == 0 || pDecumaCurve->pArcTimelineDiff[a] == 1);
				decumaAssert(pDecumaCurve->pArcTimelineDiff[a] <= MAX_UINT8);
				pCurve->arcTimelineDiff[nAddedArcs] = pDecumaCurve->pArcTimelineDiff[a];
			} else {
				pCurve->arcTimelineDiff[nAddedArcs] = 1;
			}
			nAddedArcs++;
		}
	}
	pCurve->pArc[nAddedArcs] = pCurve->pArc[a]; /* always add the last arc!  */
	if ( pDecumaCurve->pArcTimelineDiff)
	{
		decumaAssert(pDecumaCurve->pArcTimelineDiff[pCurve->nArcs-1] <= MAX_UINT8);
		decumaAssert(a == pDecumaCurve->nArcs - 1);
		decumaAssert(a >= 0);
		pCurve->arcTimelineDiff[nAddedArcs] = pDecumaCurve->pArcTimelineDiff[a];
	}
	else
	{
		pCurve->arcTimelineDiff[nAddedArcs] = 1;
	}

	nAddedArcs++;

	decumaAssert( nAddedArcs <= pDecumaCurve->nArcs );
	decumaAssert( nAddedArcs > 0 );
	pCurve->nArcs = nAddedArcs;

	return decumaNoError;
}
