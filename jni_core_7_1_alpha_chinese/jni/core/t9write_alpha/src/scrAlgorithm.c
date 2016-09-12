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


/*$Header: /data/cvsroot/seattle/t9write/alphabetic/core/src/scrAlgorithm.c,v 1.5 2011/02/14 11:41:14 jianchun_meng Exp $*/
#include "scrFineSearch.h"
#include "scrProxCurve.h"
#include "scrFullSearch.h"
#include "scrZoom.h"
#include "scrOutput.h"
#include "scrEarlyConclusion.h"
#include "decumaMath.h"
#include "databaseKEY.h"
#include "scrOutput.h"
#include "scrCurve.h"
#include "scrHeapMem.h"
#include "database.h"
#include "decumaMath.h"
#include "decumaAssert.h"
#include "scrAlgorithm.h"
#include "scrOutputHandler.h"
#include "udmAccess.h"
#include "udm.h"
#include "decumaCategoryTranslation.h"

#include "globalDefs.h"

#define MINIMAL_STACK_LEFT	16000

#include "decumaInterrupts.h"


#ifdef _DEBUG
#if defined(MEASURE_STACK_SIZE)
static char* StackPointer()
{
	char i = 9;
	
	return &i;
}
#endif

/*////////////////// */
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
	}	
	*aStackFloor = stackP-1;
}
#endif /*_DEBUG && MEASURE_STACKSIZE */


static KID *databaseWhichKeyShouldSmallCurveBe( STATIC_DB_HEADER_PTR pDB, int max_y, int nBaseLine,
										int nHelpLine, KID *pKid,
										CATEGORY_TABLE_PTR pCatTable, const SCR_API_SETTINGS * pSettings);




/**
This is the main entry point for single character recgnition operations.
Input:
	Any number of arcs with any number of points.
	The input curve most folow the specifications of SCR_CURVE (see scrCurve.h for details)
Output:
	A list of possible matches sorted by quality.
	see scrOutput.h for details
Returns:
	The number of written scrOUTPUTs
*/
DECUMA_STATUS scrSel(scrOUTPUT* pOutputs, int * pnOutputs, int nMaxOutputs, SCR_CURVE *kurva,
			   const scrOUTPUT_LIST * pPrecalculatedBaseOutputs,
			   SCR_HEAP_MEM * pScrHeap,
			   const DECUMA_INTERRUPT_FUNCTIONS * pInterruptFunctions)
{
	int nOutputs;
	SCR_API_SETTINGS * pSettings = &pScrHeap->scrSettings;
	DECUMA_STATUS status = decumaNoError;

#if defined(MEASURE_STACK_SIZE)
	int * stackP;
	unsigned int stackUsed;
	int * stackFloor;
	int * stackRoof;

	stackMeasureInit(&stackFloor,&stackRoof);
	{
#endif /*MEASURE_STACK_SIZE */

	int maxNoFullsearchCandidates;
	const int nMaxOfEachSymbolInFullsearch = 0; /* No constraint */
	const int nMaxOfEachTypeInFullsearch = 4; /* 2 -> 4 to fix PR:se0100000183 */
	const int nMaxOfEachSymbolInFinesearch = 1;
	const int nMaxOfEachTypeInFinesearch = 0; /* Not relevant */
	CATEGORY_TABLE_PTR pStaticDBCatTable;
	SEARCH_SETTINGS ss;


	int * pBaseLine;
	int * pHelpLine;

	int nBaseLine = pSettings->nBaseLineY;
	int nHelpLine = pSettings->nHelpLineY;

	KEY_DB_HEADER_PTR pStaticKeyDB;
	
	pStaticKeyDB= staticDBGetKeyDB((STATIC_DB_HEADER_PTR) pSettings->pStaticDB);

	switch ( kurva->noArcs )
	{
	case 1:
		maxNoFullsearchCandidates = 30;
		break;
	case 2:
		maxNoFullsearchCandidates = 15;
		break;
	default:
		/* reduce the number of candidates to speed up the algorithm. */
		maxNoFullsearchCandidates = 5; /* 5 -> 15 to fix PR:se0100000182 */
		break;
	}


	/*We say that base line and help line do not exist if they have the same value. */
	/*This is a temporary solution. RB */
	if (nBaseLine == nHelpLine)
	{
		pBaseLine = pHelpLine = NULL;
	}
	else
	{
		pBaseLine = &nBaseLine;
		pHelpLine = &nHelpLine;
	}

	pStaticDBCatTable = pSettings->pCategoryTable;

	if (pStaticDBCatTable == NULL)
		pStaticDBCatTable = keyDBGetStaticCategoryTable(pStaticKeyDB);

	nOutputs = 0;
	outputResetVector(pOutputs, nMaxOutputs);

	/* */
	/* Clean the array that we will use to store the rough search data in. */
	/* */
	initOutputHandler( &pScrHeap->fullsearchOutputHandler, pScrHeap->fullsearchKid,
		maxNoFullsearchCandidates, nMaxOfEachSymbolInFullsearch, nMaxOfEachTypeInFullsearch);

	/* */
	/* Search all databases roughly for matches and store result in fullsearchKid */
	/* */

	ss.pCatTable = pStaticDBCatTable;
	ss.pKeyDB = staticDBGetKeyDB((STATIC_DB_HEADER_PTR) pSettings->pStaticDB);
	ss.pPropDB = staticDBGetPropDB((STATIC_DB_HEADER_PTR) pSettings->pStaticDB);;

	status = fullSearch(&ss, &pScrHeap->fullsearchOutputHandler, kurva,
		pPrecalculatedBaseOutputs, pSettings,pInterruptFunctions);
	if (status == decumaRecognitionAborted)
		goto scrSel_abort;

	if (pSettings->pUDM)
	{
		ss.pKeyDB = (KEY_DB_HEADER_PTR) udmGetDatabase(pSettings->pUDM);
		decumaAssert(!keyDBHasWrongFormat(ss.pKeyDB));

		ss.pCatTable = keyDBGetStaticCategoryTable(ss.pKeyDB);
		ss.pPropDB = staticDBGetPropDB((STATIC_DB_HEADER_PTR) pSettings->pStaticDB);

		status = fullSearch(&ss, &pScrHeap->fullsearchOutputHandler, kurva,
			pPrecalculatedBaseOutputs, pSettings,pInterruptFunctions);
		if (status == decumaRecognitionAborted)
			goto scrSel_abort;
	}

	if ( pScrHeap->fullsearchOutputHandler.nOutputs > 0)
	{
		/* */
		/* Check if we can be really sure about the outcome */
		/* */
#define NO_EARLY_CONCLUSIONS /* To many bad rejections here */
#ifndef NO_EARLY_CONCLUSIONS
		keepOnlyTheNBestOutputs( &pScrHeap->fullsearchOutputHandler, 
			earlyConclusion(&pScrHeap->fullsearchOutputHandler) );
#endif

		initOutputHandler( &pScrHeap->finesearchOutputHandler, pOutputs,
			nMaxOutputs, nMaxOfEachSymbolInFinesearch, nMaxOfEachTypeInFinesearch );


		/*Check if we should abort or continue */
		if (TEST_ABORT_RECOGNITION(pInterruptFunctions))
			goto scrSel_abort;

		/* Finesearch among the candidates that are still in the race. */
		status = scrFineSearch(&pScrHeap->finesearchOutputHandler, kurva, pPrecalculatedBaseOutputs,
			&pScrHeap->fullsearchOutputHandler, pBaseLine, pHelpLine,pInterruptFunctions);
		if (status == decumaRecognitionAborted) 
			goto scrSel_abort;

		if (pScrHeap->finesearchOutputHandler.nOutputs > 0)
		{
			nOutputs = getTheOutputBufferSortedBestFirstAndDestroyHandler( &pScrHeap->finesearchOutputHandler );
		}

		/*Check if we should abort or continue */
		if (TEST_ABORT_RECOGNITION(pInterruptFunctions))
			goto scrSel_abort;


		decumaAssert(nOutputs <= nMaxOutputs);

		/* Zoom between the two best candidates. */
		zoomFilter(pOutputs,nOutputs,kurva,pSettings->nRefAngle,pSettings->nRefAngleImportance,4);
	}

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
#endif /*defined(MEASURE_STACK_SIZE) */
	
	*pnOutputs = nOutputs;
	return decumaNoError;
scrSel_abort:
	*pnOutputs = 0;
	return decumaRecognitionAborted;
}

int valueTransf(int angle, const KID * pKid, int nRefAngle, int nRefAngleImportance)
{
	const DECUMA_INT32 maxPunishRotation = (3*(DECUMA_INT32)maxProxValue)/4;
	int  alpha = mod_sym( (angle-nRefAngle), kidGetSymmetry(pKid));
	DECUMA_INT32  punishRotation = 0;
	ROTATION_PTR pKeyRotation = kidGetRotation(pKid);
	int  deltaAngle = pKeyRotation->leftThresholdAngle - alpha;

	if(deltaAngle > 0)
	{
		decumaAssert((DECUMA_INT32)deltaAngle * pKeyRotation->leftPunishment ==
			((DECUMA_INT32)deltaAngle * pKeyRotation->leftPunishment*FULL_ROTATION_IMPORTANCE) / FULL_ROTATION_IMPORTANCE);

		punishRotation += ((DECUMA_INT32)deltaAngle * pKeyRotation->leftPunishment*nRefAngleImportance)/FULL_ROTATION_IMPORTANCE;
	}

	deltaAngle = (DECUMA_INT32)alpha - pKeyRotation->rightThresholdAngle;

	if(deltaAngle > 0)
	{
		/* vma Added scaling for nRefAngleImportance. if nRefAngleImportance == FULL_ROTATION_IMPORTANCE, the */
		/* result should be un-touched. */
		decumaAssert((DECUMA_INT32)deltaAngle * pKeyRotation->rightPunishment ==
			((DECUMA_INT32)deltaAngle * pKeyRotation->rightPunishment*FULL_ROTATION_IMPORTANCE) / FULL_ROTATION_IMPORTANCE);

		punishRotation += ((DECUMA_INT32)deltaAngle * pKeyRotation->rightPunishment*nRefAngleImportance)/FULL_ROTATION_IMPORTANCE;
	}

	if(punishRotation > maxPunishRotation)
	{
		punishRotation = maxPunishRotation;
	}

	return punishRotation;
}

scrOUTPUT* scrGetExpectedSmallCurveOutput(STATIC_DB_HEADER_PTR pDB,
										  const SCR_CURVE* pCurve,
										  int nBaseLine,
										  int nHelpLine, 
										  CATEGORY_TABLE_PTR pCatTable,
										  const SCR_API_SETTINGS * pSettings,
										  scrOUTPUT* pOutput)
{
	DECUMA_INT16 min_y, max_y, min_x, max_x;
	int symbolInCat, alternativeInCat;
	KID kid;

	curveMinMaxCalculate(pCurve, pCurve->noArcs,
		&min_x, &max_x, &min_y, &max_y);

	if( databaseWhichKeyShouldSmallCurveBe( (STATIC_DB_HEADER_PTR) pSettings->pStaticDB, 
		max_y, nBaseLine, nHelpLine, &kid, pCatTable, pSettings) )
	{
		CATEGORY nCategory;

		translateToCategoryStructs(&pSettings->characterSet,pCatTable,NULL,&nCategory,NULL);

		/*Is the key in the current category? */
		databaseCheckCategory(&symbolInCat, &alternativeInCat,
				   &kid, nCategory);

		if (symbolInCat || alternativeInCat )
		{
			scrOutputCreateSpecial(pOutput, pCurve, SCR_SMALL_CURVE_MU_VALUE, &kid);

			if( symbolInCat && alternativeInCat )
			{ /* Both symbols are valid (original and alternative) */
				/* If the helplines exists only one of them should be returned. */
				if ( nBaseLine != nHelpLine ) /* Check if the helplines are available */
				{
					/* If the centre of gravity of the curve is above the helpline it is */
					/* considered to be the alternative symbol. */
					if ( ((DECUMA_INT32)min_y + max_y)/2 > nHelpLine )
						pOutput->outSymbol = original;
					else
						pOutput->outSymbol = alternative;
				}
				else
					pOutput->outSymbol = both;
			}
			else if ( alternativeInCat )
				pOutput->outSymbol = alternative;
			else
				pOutput->outSymbol = original;

			return pOutput;
		}
	}

	return NULL;
}

static KID *databaseWhichKeyShouldSmallCurveBe( STATIC_DB_HEADER_PTR pDB, int max_y, int nBaseLine,
										int nHelpLine, KID *pKid,
										CATEGORY_TABLE_PTR pCatTable, const SCR_API_SETTINGS * pSettings)
{
	/*Finds which of the small keys this curve should be interpreted as.
	If it is below the helplines or if helplines are not present
	it should be a dot. Otherwise a fnutt.
	*/

	SMALL_KEY_PTR pSmallKey = propDBGetSmallKey(staticDBGetPropDB(pDB));
	const DECUMA_INT16 DECUMA_DB_STORAGE * pIndexList = NULL;
	int i;

	decumaAssert(pSmallKey);
	
	if( nBaseLine == nHelpLine) /*No helplines */
	{
		pIndexList = pSmallKey->smallKeyNoHelplines;
	} 
	else if (max_y > nHelpLine && max_y > nBaseLine - (nBaseLine - nHelpLine)/2) /*Below helpline, lower half */
	{
		pIndexList = pSmallKey->smallKeyBelowHelplineLower;
	}
	else if (max_y > nHelpLine && max_y < nBaseLine - (nBaseLine - nHelpLine)/2) /*Below helpline, upper half */
	{
		pIndexList = pSmallKey->smallKeyBelowHelplineUpper;
	}
	else /*Above helpline */
	{
		pIndexList = pSmallKey->smallKeyAboveHelpline;
	}

	for (i = 0; i < 4; i++)
	{
		int smallKeyIndex;

		smallKeyIndex = pIndexList[i];
		
		if (smallKeyIndex>=0)
		{
			CATEGORY nCategory;
			int symbolInCat, alternativeInCat;

			pKid->pKeyDB = (KEY_DB_HEADER_PTR) staticDBGetKeyDB(pDB);
			pKid->pPropDB = (PROPERTY_DB_HEADER_PTR) staticDBGetPropDB(pDB);
			pKid->pCatTable = (CATEGORY_TABLE_PTR) pCatTable;
			pKid->noDiacArcs = 0;
			pKid->diacKeyIndex = -1;
			pKid->diacKeyXOffset = 0;
			pKid->diacKeyYOffset = 0;
			pKid->pDiacKey = NULL;
			pKid->noBaseArcs = 1;
			pKid->baseKeyIndex = (KEYINDEX)smallKeyIndex;

			kidSetBaseKeyPointer(pKid);

			translateToCategoryStructs(&pSettings->characterSet,pCatTable,NULL,&nCategory,NULL);

			/*Is the key in the current category? */
			databaseCheckCategory(&symbolInCat, &alternativeInCat,
					   pKid, nCategory);
			
			if (symbolInCat || alternativeInCat)
			{
				return pKid;
			}
		}
	}

	return NULL;
}

/************************************************** End of file ***/
