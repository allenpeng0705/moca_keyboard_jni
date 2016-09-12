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


/* File:  scrFullSearch.c */

/* Performs a full Search */


#include "scrOutput.h"
#include "scrProxCurve.h"
#include "decumaMemory.h"
#include "database.h"
#include "databaseKEY.h"
#include "decumaAssert.h"
#include "decumaMath.h"
#include "scrCurve.h"
#include "scrAlgorithm.h"
#include "scrIterators.h"
#include "scrFullSearch.h"
#include "decumaCategoryTranslation.h"

#include "globalDefs.h"


/*#undef DO_LOGGING */
#ifdef DO_LOGGING
#include "t9write_log.h"
#endif


#define MAX_NON_PRECALC_BASES 6

#define LARGE_NUMBER 10000

#define TWO_DIM_CUT_ARRAY_INDEX( a,b ) ((a)*nDifferentCutsToTry + (b)) /*Used to simulate a two-dimensional array */

#include "decumaInterrupts.h"

/*/////////////////////////////////////////////////////////////////////////////////// */
/*Local constants */
/*/////////////////////////////////////////////////////////////////////////////////// */

/*/////////////////////////////////////////////////////////////////////////////////// */
/*Local functions */
/*/////////////////////////////////////////////////////////////////////////////////// */

/*static void extractCandidates(int * ind, INT16 *muCand, INT16 * mu,int * ligLeft,int * ligRight, int numberOfKeys); */
/*static int valueVerticalOffsetInFullSearch(const SCR_CURVE * pCurve, KEY_PTR pKey,
								   const int * baseLine, const int *helpLine);*/

static int valueCut(int a, int b);

static void calculateDiacMeanPoint(INT32_POINT * pDiacMeanPoint, const SIM_TRANSF * pSimTransf,
							const SCR_CURVE * pCurve, int nDiacArcs);

static void setOutSymbol(scrOUTPUT * pBestVersion, const int bInCategory, const int bAlternativeSymbolInCategory);

static void getBestVersionOfKey( KID * pKid, const SCR_CURVE * pCurveCut,
        int  nDifferentCutsToTry, const int * pCutsToTry,
		scrOUTPUT * pBestVersion,
		const SCR_API_SETTINGS *pSettings,
		int bNoConcurrentArcs);

static DECUMA_BOOL evaluateDiacriticKey(KID * pKid, SCR_CURVE * pDiacCurve,
		scrOUTPUT * pBestOutSoFar, scrOUTPUT * pBaseOut,
		int nBaseArcs, int nDiacArcs, const SCR_API_SETTINGS * pSettings);

static void extractSomeArcsFromCurve( const SCR_CURVE * pCurve, int nFirstArc, int nArcsToExtract,
							   SCR_CURVE * pNewCurve);

static void makeAndEvaluateComplexKey(scrOUTPUT * pOut,
				const SCR_CURVE * pCurve, DECUMA_INT16 diacKeyXOffset,
				DECUMA_INT16 diacKeyYOffset, const SCR_API_SETTINGS * pSettings);

static int diacSymbolAlreadyAdded(OUTPUT_HANDLER * pOutputHandler, DECUMA_INT8 diacKeySymbolNrToTest);

/*/////////////////////////////////////////////////////////////////////////////////// */

DECUMA_STATUS fullSearch(const SEARCH_SETTINGS * pSS,
				OUTPUT_HANDLER * pOutputHandler, const SCR_CURVE * pCurve,
				const scrOUTPUT_LIST * pPrecalculatedBaseOutputs,
				SCR_API_SETTINGS * pSettings,
				const DECUMA_INTERRUPT_FUNCTIONS * pInterruptFunctions)
{
	/*Rough searches through database. */
	/*The first arc in pCurve is cut in different ways */
	/*and all ways are tested against DB. */

	int l,r; /*Counters */

	int nMaxDiacs = minNUMB(pSS->pPropDB->nDiacKeyLists,pCurve->noArcs-1);
	int nDiacArcs;
	CATEGORY nCategory;
	translateToCategoryStructs(&pSettings->characterSet,pSS->pCatTable,NULL,&nCategory,NULL);

	if (pCurve->noArcs <= pSS->pKeyDB->nMaxArcsInCurve)
	{
		/***********************************
		**       Caculate curve cut       **
		***********************************/

		const int cuts[] = {0,6};

		SCR_CURVE curveCut[ sizeof(cuts) / sizeof(cuts[0]) * sizeof(cuts) / sizeof(cuts[0])];
		SCR_ARC curveArcThatWillBeCut[ sizeof(cuts) / sizeof(cuts[0]) * sizeof(cuts) / sizeof(cuts[0])];

		int nDifferentCutsToTry = sizeof(cuts) / sizeof(cuts[0]) ;

		scrOUTPUT_LIST tempPrecalc;

		DECUMA_UINT8 arcTimelineDiffMask = 0;
		int n, bNoConcurrentArcs = 1;

		/*This part of the code has to be changed if cutting */
		/*should be done on any other than first arc. (CURVEindexed) */

		myMemSet(&curveCut,0,sizeof(curveCut));
		myMemSet(&curveArcThatWillBeCut,0,sizeof(curveArcThatWillBeCut));

		/*Cut the curve in different ways and save in a buffer; */
		for(l=0 ; l<nDifferentCutsToTry ; l++)
		{
			for(r=0 ; r<nDifferentCutsToTry ; r++)
			{
				SCR_CURVE * pCurveCut = &curveCut[ TWO_DIM_CUT_ARRAY_INDEX(l,r) ]; /*Temporary pointer */
				SCR_ARC * pArcCut = &curveArcThatWillBeCut[ TWO_DIM_CUT_ARRAY_INDEX(l,r) ]; /*Temporary pointer */
				int i;

				for (i=1; i<pCurve->noArcs; i++)
				{
					pCurveCut->Arcs[i] = pCurve->Arcs[i];
				}
				pCurveCut->Arcs[0] = pArcCut;

				curveCopy(pCurveCut,pCurve);

				InterpolateArc(pArcCut, pCurve->Arcs[0],
					cuts[l]*128/NUMBER_OF_POINTS_IN_ARC,
					(NUMBER_OF_POINTS_IN_ARC - cuts[r])*128/NUMBER_OF_POINTS_IN_ARC);
				arcPrecalculate(pCurveCut->Arcs[0],NULL);
				preCalculateCurveInterleavedNoDerivate(pCurveCut);

				/*Check if we should abort or continue */
				if (TEST_ABORT_RECOGNITION(pInterruptFunctions))
					goto fullSearch_abort;
			}
		}

		for ( n = 0; n < pCurve->noArcs-1; n++ )
		{
			decumaAssert(pCurve->arcTimelineDiff[n] == 0 || pCurve->arcTimelineDiff[n] == 1);
			arcTimelineDiffMask |= pCurve->arcTimelineDiff[n] << n;
			bNoConcurrentArcs = bNoConcurrentArcs && pCurve->arcTimelineDiff[n];
		}

		/************************************
		** Iterate through the (base) keys **
		*************************************/

		tempPrecalc.nOutputs = -1;  /*We have no precalculated base */

		{
		  KIT kit;

		  /* Create an arc-sequence mask.
		   * If bit n is set, arc n+1 has the same sequence number as arc n
		   * If bit n is unset, arc n+1 has a higher sequence number than arc n
		   *
		   * Sequence masks from the database, accessed with kidGetArcTimelineDiffMask(),
		   * are constructed like this, so we can do a normal integer comparison
		   * between the masks to conclude whether a candidate has the same sequence
		   * data as the input curve.
		   *
		   * Sequence data is used to model concurrent strokes, i.e. multi-finger
		   * input.
		   *
		   * TODO: The mask calculation has been moved here, but should it be moved
		   *       even further? If all we use is the mask, both CURVE and SCR_CURVE
		   *       could be simplified to store only the mask rather than an array
		   *       of sequence numbers.
		   */

		  /*for ( n = 1; n < pCurve->noArcs; n++ )
		  {
			  arcTimelineDiffMask >>= 1;
			  decumaAssert(pCurve->arcTimelineDiff[n] >= pCurve->arcTimelineDiff[n - 1]);
			  if ( pCurve->arcTimelineDiff[n] == pCurve->arcTimelineDiff[n - 1] )
				  arcTimelineDiffMask |= 1;
		  }*/

		  /* Create a timeline difference mask.
		   * If bit n is unset, previous arc is concurrent: no timeline difference
		   * If bit n is set, this arc is subsequent to previous arc: timeline difference
		   *
		   * Sequence masks from the database, accessed with kidGetArcTimelineDiffMask(),
		   * are constructed like this, so we can do a normal integer comparison
		   * between the masks to conclude whether a candidate has the same timeline diff
		   * sequence as the input curve.
		   *
		   * Timeline difference is used to model concurrent strokes, i.e. multi-finger
		   * input.
		   *
		   * To keep the API simple, an array of timeline differences is used instead of a mask
		   */

		  kitCreate(&kit, pCurve->noArcs, 0, pSS, &tempPrecalc);

		  while (kitGoNext(&kit))
		  {
			scrOUTPUT bestVersionOfKey;
			int bInCategory;
			int bAlternativeSymbolInCategory;

			/* Check if the key is a part of the selected category. */
			/* OR check if this key shall be rejected because of conflicts */
			/* with other keys with this arc count. ('l' & 'I', '0','o' & 'O',  'l', 'I', '1') */
			databaseCheckCategory(&bInCategory, &bAlternativeSymbolInCategory,
				&kit.kid, nCategory);

			/* Check the temporal indices, continue if we don't have a match.
			 *
			 * Normal keys should have monotonically increasing temporal
			 * indices (0, 1 ... ), but the new multi-finger gestures may have
			 * one or more arcs with same temporal index.
			 */
			/* if ( !kidHasCompatibleTemporalData(&kit.kid, pCurve->arcTimelineDiff, pCurve->noArcs) ) */
			/*	continue; */

			if ( pCurve->noArcs > 1 && kidGetArcTimelineDiffMask(&kit.kid) != arcTimelineDiffMask )
				continue;

			/*Check if we should abort or continue */
			if (TEST_ABORT_RECOGNITION_EVERY(30, KIT_GET_COUNTER(&kit), pInterruptFunctions)) /*Only do the test every 30 times */
				goto fullSearch_abort;

			if ( bInCategory || bAlternativeSymbolInCategory )
			{
				getBestVersionOfKey( &kit.kid, curveCut, nDifferentCutsToTry,
					cuts, &bestVersionOfKey, pSettings, bNoConcurrentArcs);
#ifdef DO_LOGGING
				logVariable32("bvm",bestVersionOfKey.mu);
#endif
				setOutSymbol(&bestVersionOfKey, bInCategory, bAlternativeSymbolInCategory);
				addToOutputHandlerIfGoodEnoughAndNotTooManyOfSameSymbolOrType(
				  		pOutputHandler, bestVersionOfKey.mu + bestVersionOfKey.punish,
				  		&bestVersionOfKey, 0);

				/*Check if we should abort or continue */
				if (TEST_ABORT_RECOGNITION(pInterruptFunctions))
					goto fullSearch_abort;
			}
		  }
		}

		/****************************************************************
		**   COMPARE TO COMPLEX KEYS (keys with base and diacritics)    **
		*****************************************************************/

		for (nDiacArcs=1; nDiacArcs <= nMaxDiacs && bNoConcurrentArcs; nDiacArcs++) /* Don't allow concurrent arc for combined symbols */
		{
			/************************************************************************
			** Find the best base keys that can have diacrits with nDiacArcs arcs. **
			************************************************************************/

			scrOUTPUT baseOutBuffer[MAX_NON_PRECALC_BASES];
			int nBaseArcs = pCurve->noArcs - nDiacArcs;
			short nBaseOutputs;
			scrOUTPUT_LIST baseOutList;
			const int nMaxOfEachSymbol = 0; /* No constraint */
			const int nMaxOfEachType = 2;
			OUTPUT_HANDLER baseOutputHandler;

			KIT kit;
			scrOUTPUT out;
			out.punish = 0;

			initOutputHandler( &baseOutputHandler, baseOutBuffer, MAX_NON_PRECALC_BASES, nMaxOfEachSymbol, nMaxOfEachType);

			myMemSet(&baseOutBuffer,0,sizeof(baseOutBuffer));


			if (pPrecalculatedBaseOutputs) /* MCR */
			{
				kitCreate(&kit, nBaseArcs,nDiacArcs,pSS,
					&pPrecalculatedBaseOutputs[nDiacArcs-1]);  /* check precalculated */

				while(kitGoNext(&kit))
				{
					scrOUTPUT preCalcOut = *kitGetSCROut(&kit);

					addToOutputHandlerIfGoodEnoughAndNotTooManyOfSameSymbolOrType( &baseOutputHandler, /*TODO: unnecessarily many checks! */
						0, &preCalcOut, 0);
				}
			}
/*				tempPrecalc.nOutputs = -2;  // check baseKeysWithPoorShape */
/*				kitCreate(&kit,nBaseArcs,nDiacArcs,pSS,&tempPrecalc); */
/*			} */
			else
			{
				tempPrecalc.nOutputs = -1;  /* SCR - check all base keys */
				kitCreate(&kit,nBaseArcs,nDiacArcs,pSS,&tempPrecalc);
/*			} */

			/* remove diacritic arcs from curveCut */
			for(l=0 ; l<nDifferentCutsToTry ; l++)
			{
				for(r=0 ; r<nDifferentCutsToTry ; r++)
				{
					SCR_CURVE * pCurveCut = &curveCut[ TWO_DIM_CUT_ARRAY_INDEX(l,r) ]; /*Temporary pointer */
/*					SCR_ARC * pArcCut = &curveArcThatWillBeCut[ TWO_DIM_CUT_ARRAY_INDEX(l,r) ]; //Temporary pointer */

					pCurveCut->noArcs = nBaseArcs;

					preCalculateCurveInterleavedNoDerivate(pCurveCut);
				}
			}
			while(kitGoNext(&kit))
			{
				/*Add to buffer if good enough */
				/*How many should be found? Now 4. */
				scrOUTPUT bestVersionOfKey;

				getBestVersionOfKey( &kit.kid, curveCut, nDifferentCutsToTry,
					cuts, &bestVersionOfKey, pSettings, bNoConcurrentArcs);
				addToOutputHandlerIfGoodEnoughAndNotTooManyOfSameSymbolOrType( &baseOutputHandler,
						bestVersionOfKey.mu + bestVersionOfKey.punish, &bestVersionOfKey, 0);

				/*Check if we should abort or continue */
				if (TEST_ABORT_RECOGNITION(pInterruptFunctions))
					goto fullSearch_abort;

			}
			}
			nBaseOutputs = baseOutputHandler.nOutputs;

			if (baseOutputHandler.nOutputs > 0)
			{
				SCR_CURVE diacCurve;
				short baseCurveXMin;
				short baseCurveXMax;
				short baseCurveYMin;
				short baseCurveYMax;
				short diacCurveXMin;
				short diacCurveXMax;
				short diacCurveXOffset;
				short diacCurveYOffset;
				short diacCurveYMin;
				short diacCurveYMax;
				DECUMA_BOOL DiacIsAbove = TRUE;

				getTheOutputBufferSortedBestFirstAndDestroyHandler( &baseOutputHandler);

				baseOutList.pOut = baseOutBuffer;
				baseOutList.nOutputs = nBaseOutputs;

					/****************************************************
					**         Add diacritic mark to base key          **
					****************************************************/
				extractSomeArcsFromCurve( pCurve, nBaseArcs, nDiacArcs, &diacCurve);
				curveMinMaxCalculate(pCurve, nBaseArcs ,&baseCurveXMin, &baseCurveXMax, &baseCurveYMin, &baseCurveYMax);
				curveMinMaxCalculate(&diacCurve, nDiacArcs ,&diacCurveXMin, &diacCurveXMax, &diacCurveYMin, &diacCurveYMax);
				curveMeanCalculate( &diacCurve, 0, nDiacArcs-1, &diacCurveXOffset, &diacCurveYOffset);

				diacCurveXOffset -=baseCurveXMin;
				diacCurveYOffset -=baseCurveYMax;
/*				if (diacCurveYMax > baseCurveYMin) */
/*					diacCurveYOffset -= (diacCurveYMax - baseCurveYMin); */

				kitCreate(&kit, nBaseArcs, nDiacArcs, pSS, &baseOutList);
				while (kitGoNext(&kit))
				{
					scrOUTPUT bestOutSoFar;
					scrOUTPUT *pBaseOut = kitGetSCROut(&kit);

					DECUMA_INT8 baseKeyXMin;
					DECUMA_INT8 baseKeyXMax;
					DECUMA_INT8 baseKeyYMin;
					DECUMA_INT8 baseKeyYMax;
					DIAKIT diakit;

					myMemSet(&bestOutSoFar,0,sizeof(bestOutSoFar));
					bestOutSoFar.mu=LARGE_NUMBER;
					bestOutSoFar.punish = 0;

					myMemCopy(bestOutSoFar.arcOrder, pBaseOut->arcOrder, kit.kid.noBaseArcs*sizeof(pBaseOut->arcOrder[0]));

					kidGetMinMaxX( &kit.kid, &baseKeyXMin, &baseKeyXMax);
					kidGetMinMaxY( &kit.kid, &baseKeyYMin, &baseKeyYMax);

					/*We should add diacritic marks to this key to get correct number of arcs. */
					diakitCreate( &diakit, &kit.kid, nDiacArcs);
					/* 1. Find the best diacritic mark */
					while (diakitGoNext(&diakit))
					{
						/*JM Added check to see if diacritic arcs position relative base curve are */
						/*consistent with how they should be according to the definition in diakit.kid */
						/*If these don't agree - then skip this diacritic candidate */
						if ((diacCurveYOffset<0 && diacKeyPlacementIsBelow(&diakit.kid)) ||
							(diacCurveYOffset>0 && !diacKeyPlacementIsBelow(&diakit.kid)))
						{
							continue;
						}

						if ( !((pPrecalculatedBaseOutputs) &&
							(diacSymbolAlreadyAdded(pOutputHandler, (DECUMA_INT8) diacKeyGetSymbolNr( &diakit.kid )) )))
						{
							int bInCategory;
							int bAlternativeSymbolInCategory;

							/* Check if the key is a part of the selected category. */
							/* OR check if this key shall be rejected because of conflicts */
							/* with other keys with this arc count. ('l' & 'I', '0','o' & 'O',  'l', 'I', '1') */
							databaseCheckCategory(&bInCategory, &bAlternativeSymbolInCategory,
								&diakit.kid, nCategory);
							if ( bInCategory || bAlternativeSymbolInCategory )
							{
								DECUMA_BOOL bBestSoFar;

#define BASE_KEY_FRAC 8
#define DOT_ABOVE 0x0307
								/* quick fix for dot treatment */
								if ((nDiacArcs == 1)
									 && (diacKeyGetSymbol(&diakit.kid)==DOT_ABOVE)
									 && ((diacCurveXMax - diacCurveXMin)<=(baseCurveYMax - baseCurveYMin)/BASE_KEY_FRAC)
									 && ((diacCurveYMax - diacCurveYMin)<=(baseCurveYMax - baseCurveYMin)/BASE_KEY_FRAC)
									/*(scrCurveIsTooSmallForProximityMeasure(&diacCurve, nBaseLine, nHelpLine)) && */
									 && (IS_CONTAINED_IN_MASK( diakit.diacMask, diacKeyGetDiacCurveIndex(&diakit.kid))))
								{
									bBestSoFar=TRUE;
									bestOutSoFar.mu = 1;
									bestOutSoFar.DBindex = diakit.kid;
									bestOutSoFar.nCutLeft = pBaseOut->nCutLeft;
									bestOutSoFar.nCutRight = pBaseOut->nCutRight;
									bestOutSoFar.arcOrder[nBaseArcs] = (DECUMA_INT8)nBaseArcs+1;

								}
								else
								{
									bBestSoFar=evaluateDiacriticKey(&diakit.kid,
										&diacCurve, &bestOutSoFar, pBaseOut,
										nBaseArcs, nDiacArcs, pSettings);
								}
								if (bBestSoFar) {
									setOutSymbol(&bestOutSoFar, bInCategory, bAlternativeSymbolInCategory);
									if (diacKeyPlacementIsBelow(&diakit.kid))
										DiacIsAbove = FALSE;
									else
										DiacIsAbove = TRUE;
								}
							}
						}
					}

						/* 2. Compare the whole key (precalculated base + diacritics) to the whole curve */
					if (bestOutSoFar.mu!=LARGE_NUMBER)
					{
						if ((baseCurveYMax-baseCurveYMin) > 0)
						{
							DECUMA_INT16 diacKeyXOffset;
							DECUMA_INT16 diacKeyYOffset;
							const DECUMA_INT16 dbDistBase2Help = (DECUMA_INT16)
								propDBGetDistBase2Help( bestOutSoFar.DBindex.pPropDB );
							const DECUMA_INT16 maxOffsetOfDiacArcs =  -dbDistBase2Help;
							const DECUMA_INT16 minOffsetOfDiacArcs = - (5*dbDistBase2Help) / 2;

							if (pPrecalculatedBaseOutputs) /* fungerar inte med dotless i! /EM */
							{
								INT32_POINT diacMeanPoint;

								bestOutSoFar.DBindex.noBaseArcs = nBaseArcs; /*Reset this value (it was temporarily changed) */
								if (kidGetSymmetry(&pBaseOut->DBindex) == 0)
								{
									SIM_TRANSF tempTransf = pBaseOut->simTransf;
									tempTransf.theta = 0; /*Otherwise, e.g. the dots above � will end up wrong. */
									calculateDiacMeanPoint(&diacMeanPoint, &tempTransf, pCurve, nDiacArcs);
								}
								else
								{
									calculateDiacMeanPoint(&diacMeanPoint, &pBaseOut->simTransf, pCurve, nDiacArcs);
								}
								decumaAssert(diacMeanPoint.y <= MAX_DECUMA_INT16 && diacMeanPoint.y >= MIN_DECUMA_INT16);
								diacKeyYOffset = (DECUMA_INT16) diacMeanPoint.y;
							}
							else
							{
								diacKeyYOffset = diacCurveYOffset*((DECUMA_INT32)baseKeyYMax-baseKeyYMin)/((DECUMA_INT32)baseCurveYMax-baseCurveYMin);
							}
							diacKeyXOffset = (baseKeyXMax+baseKeyXMin)/2;

							bestOutSoFar.DBindex.noBaseArcs = nBaseArcs; /*Reset this value (it was temporarily changed) */

							if (DiacIsAbove) {
								diacKeyYOffset = (DECUMA_INT16) minNUMB( diacKeyYOffset, maxOffsetOfDiacArcs);
								diacKeyYOffset = (DECUMA_INT16) maxNUMB( diacKeyYOffset, minOffsetOfDiacArcs);
							}
							else {
								diacKeyYOffset = (DECUMA_INT16) minNUMB( diacKeyYOffset, -minOffsetOfDiacArcs);
								diacKeyYOffset = (DECUMA_INT16) maxNUMB( diacKeyYOffset, -maxOffsetOfDiacArcs);
							}
							diacKeyXOffset = (DECUMA_INT16) minNUMB( diacKeyXOffset, baseKeyXMax);
							diacKeyXOffset = (DECUMA_INT16) maxNUMB( diacKeyXOffset, baseKeyXMin);


							makeAndEvaluateComplexKey(&bestOutSoFar,pCurve,
								diacKeyXOffset ,diacKeyYOffset, pSettings);

							addToOutputHandlerIfGoodEnoughAndNotTooManyOfSameSymbolOrType(
								pOutputHandler,bestOutSoFar.mu + bestOutSoFar.punish,
								&bestOutSoFar, 0);

 							/*Check if we should abort or continue */
							if (TEST_ABORT_RECOGNITION(pInterruptFunctions))
								goto fullSearch_abort;

						}
					}
				}
			}
		}
	}
	return decumaNoError;

fullSearch_abort:
	pOutputHandler->nOutputs=0;
	return decumaRecognitionAborted;

}




/**********************************************************************/


static void calculateDiacMeanPoint(INT32_POINT * pDiacMeanPoint, const SIM_TRANSF * pSimTransf,
							const SCR_CURVE * pCurve, int nDiacArcs)
{
	DECUMA_INT16 xMean, yMean;

	decumaAssert( pDiacMeanPoint );

	curveMeanCalculate(pCurve, pCurve->noArcs-nDiacArcs, pCurve->noArcs-1, &xMean, &yMean);

	pDiacMeanPoint->x = (DECUMA_INT32)xMean * SIM_TRANSF_ROUNDING_FACTOR;
	pDiacMeanPoint->y = (DECUMA_INT32)yMean * SIM_TRANSF_ROUNDING_FACTOR;

	simtransfPoint(pDiacMeanPoint, pDiacMeanPoint, pSimTransf);

	pDiacMeanPoint->x /= SIM_TRANSF_ROUNDING_FACTOR;
	pDiacMeanPoint->y /= SIM_TRANSF_ROUNDING_FACTOR;
} /*calculateDiacMeanPoint() */

static int valueCut(int a, int b)
{
	/*Values cutting only according to length. */
	const int factor = 20;
	const int limit = 200;
	const int temp = maxNUMB(a,b)*factor;
	return minNUMB(temp, limit);
}


/*static int valueVerticalOffsetInFullSearch(const SCR_CURVE * pCurve, KEY_PTR pKey,
								   const int * baseLine, const int *helpLine)
{
	return 0; //To be implemented. RB
}*/



/* Returns 1 if diacKeySymbolNrToTest already is added to the output list */
static int diacSymbolAlreadyAdded(OUTPUT_HANDLER * pOutputHandler, DECUMA_INT8 diacKeySymbolNrToTest)
{
	NODE * pNode;

	pNode = pOutputHandler->headNode.pNext;

	if ( diacKeySymbolNrToTest < 0 )
		return 0;

	while (pNode)
	{
		if ( databaseValidKID( pNode->pOut->DBindex ) )
		{
			if ( diacKeyGetSymbolNr( &pNode->pOut->DBindex ) == diacKeySymbolNrToTest )
				return 1;
		}
		pNode = pNode->pNext;
	}

	return 0;
}

static void setOutSymbol(scrOUTPUT * pBestVersion, const int bInCategory, const int bAlternativeSymbolInCategory)
{
	if (bInCategory && bAlternativeSymbolInCategory)
		pBestVersion->outSymbol = both;
	else if (bInCategory)
		pBestVersion->outSymbol = original;
	else if (bAlternativeSymbolInCategory)
		pBestVersion->outSymbol = alternative;
	else /* something is wrong!!! Use the default symbol!! */
	{
		decumaAssert( 0 );
		pBestVersion->outSymbol = original;
	}
}


/* Modified version of function found at http://en.wikipedia.org/wiki/Permutation */
/* TODO see if we have some internal function to use or if we should make this a  */
/* suitable generic function. */
/* Writes the (nPermIdx+1):th of nBase! possible permutations to pPerm. */
/* The values in pPerm will range from nOffset to nOffset+nBase-1. */
/* pPerm must fit at least nBase elements */
/* Example: */
/* nPermIdx = 0, nBase = 2, nOffset = 1 => pPerm = {1, 2} */
/* nPermIdx = 1, nBase = 2, nOffset = 1 => pPerm = {2, 1} */
static void getPermutation(int nPermIdx, int nBase, int nOffset, DECUMA_INT8 * pPerm)
{
    int  d, r;
    DECUMA_UINT8 aDigits[MAX_NUMBER_OF_ARCS_IN_CURVE];
    int  nDigits = nBase; /* constant length output! */

	decumaAssert(nBase > 0);
	decumaAssert(nBase <= sizeof(aDigits) / sizeof(aDigits[0]));

    for (d = 0; d < sizeof(aDigits) / sizeof(aDigits[0]); d++) aDigits[d] = d;

    if( nBase > 0 )
        do
        {
            d = nPermIdx / nBase; /* nBase = variable */
            r = nPermIdx % nBase; /* nBase = variable */
            nPermIdx /= nBase;
            *pPerm++ = nOffset + aDigits[ r ];

            /* Permutation: Remove 'r'th element */
            nDigits = nBase - r - 1;
            if( nDigits > 0 ) {
                decumaMemmove( aDigits + r, aDigits + r + 1, nDigits * sizeof(aDigits[0]) );
            }
            nBase--;
        } while( nBase > 0 ); /* permutation: nbase > 0 */
    /* permutation: nothing to do */
}

/* TODO This function could probably be optimized */
/* Writes the next multi-touch arc order permutation to pOutput starting with */
/* [1 2 .. pOutput->DBindex.noBaseArcs]. */
/* Start by calling with nNextPermRef set to 0, continue with nNextPermRef set */
/* to the returned value until 0 is returned (= all permutations have been evaluated). */
static int getNextMultiTouchArcOrder(scrOUTPUT * pOutput, int nNextPermRef)
{
	int nPermIdx, nPermutations = 1;
	int nBase = 1;

	/* We should allow the multi-touch arcs to have any arc order within the multi-touch sequence */
	/* Example: */
	/* If timelineDiff = [0 0 1 1 0] */
	/* then arcOrder = [1 2 3 4 5 6] */
	/* and should expand to */
	/* [1 2 3 4 5 6], [1 2 3 4 6 5], [1 3 2 4 5 6], [1 3 2 4 6 5], [2 1 3 4 5 6], [2 1 3 4 6 5], */
	/* [2 3 1 4 5 6], [2 3 1 4 6 5], [3 1 2 4 5 6], [3 1 2 4 6 5], [3 2 1 4 5 6], [3 2 1 4 6 5] */

	/*We are doing this by testing all noBaseArcs! permutations and filtering out the  */
	/*invalid ones. */

	/*Start by getting nPermutations = noBaseArcs! */
	while (nBase < pOutput->DBindex.noBaseArcs) nPermutations *= ++nBase;

	/*Go through all the permutations but only return the valid ones */
	for (nPermIdx = nNextPermRef; nPermIdx < nPermutations; nPermIdx++)
	{
		DECUMA_INT8 arcOrder[MAX_NUMBER_OF_ARCS_IN_CURVE];
		int i, bInvalidPermutation = 0;

		getPermutation(nPermIdx, nBase, 1, arcOrder);

		for (i = 0; i < nBase; i++)
			if (arcOrder[i] != i)
			{
				/* Check that i and arcOrder[i]-1 are in the same multi-touch-sequence  */
				/* (consecutive multi-touch) */
				/* If this is true for all the arc indices, this means that the current permutation  */
				/* has only reorederd arcs within the multi-touch sequences and is therefore valid */

				/*Find out if i or arcOrder[i]-1 is the largest number */
				int maxInd = i > arcOrder[i]-1 ? i : arcOrder[i]-1;
				int minInd = i < arcOrder[i]-1 ? i : arcOrder[i]-1;
				int nConsecutiveMultitouch = 0;

				while (nConsecutiveMultitouch < maxInd && (pOutput->arcTimelineDiffMask & (1 << (maxInd-nConsecutiveMultitouch-1))) == 0) nConsecutiveMultitouch++;

				if (maxInd - minInd > nConsecutiveMultitouch)
				{
					bInvalidPermutation = 1;
					break; /* Not valid permutation */
				}
			}

		if (bInvalidPermutation) continue;

		decumaMemcpy(pOutput->arcOrder, arcOrder, pOutput->DBindex.noBaseArcs * sizeof(arcOrder[0]));

		return nPermIdx+1;
	}

	return 0;
}

static void getBestVersionOfKey( KID * pKid, const SCR_CURVE * pCurveCut,
        int  nDifferentCutsToTry, const int * pCutsToTry,
		scrOUTPUT * pBestVersion,
		const SCR_API_SETTINGS *pSettings,
		int bNoConcurrentArcs)

{
	/* This function tries to match the
	curve with the key in different ways (different cuts and different arc orders) and find the best one.
	- nDifferentCutsToTry specifies the number of different cuts that should be made in
	  the curve
	- pCutsToTry should be a pointer to an array which specifies the different
	  alternatives of cutting

	NOTE: pCurveCut is assumed to be a pointer, pointing to a vector with
	      nDifferentCutsToTry*nDifferentCutsToTry different curves where
	      every curve has a diffferent cut in the first arc according to the
	      cuts in pCutsToTry

	It returns 0 if it's not a valid key, otherwise it returns 1.
	*/
	scrOUTPUT out;

	int i,l,r;
	int nArcOrders;
	DECUMA_INT8_DB_PTR ppArcOrders[MAX_ARC_ORDERS_PER_SYMBOL];

	myMemSet(&out,0,sizeof(out));
	out.DBindex = *pKid;
	out.symbol = kidGetSymbol(pKid);
	out.punish = 0;

	pBestVersion->mu = LARGE_NUMBER;


#ifdef ONPALM_ARMLET
	/* This is a fix for Palm OS 5 to speed up the fullsearch. */
	if ( pCurveCut->noArcs >= 2 ) {
		nDifferentCutsToTry = 1;
	}
#endif

	nArcOrders = kidGetArcOrders(pKid,ppArcOrders,sizeof(ppArcOrders)/sizeof(ppArcOrders[0]));

	/* In case of concurrent arcs there must only be one arc order specified and it */
	/* should be the normal order, i.e. [1 2 .. nArcs] */
	decumaAssert(bNoConcurrentArcs || nArcOrders == 1);
#ifdef _DEBUG
	for (i=0; !bNoConcurrentArcs && i<pKid->noBaseArcs; i++)
	{
		decumaAssert(ppArcOrders[0][i] == i+1);
	}
#endif

	/* TODO There should be a faster and still reliable way to identify arc order? */
	for (i=0; i<nArcOrders; i++)
	{
		int j, nNextPermRef = 1;

		/*Store the current arc order in the output */
		for (j = 0; j < pKid->noBaseArcs; j++) out.arcOrder[j] = ppArcOrders[i][j];

		do
		{
			out.nCutLeft = pCutsToTry[0];
			out.nCutRight = pCutsToTry[0];

			/*Take prox. of the curve which is cut according to the settings above */
			proxCurveKeyInterleavedNoDerivate(&out.mu,&out.simTransf.theta,
				&pCurveCut[ TWO_DIM_CUT_ARRAY_INDEX(0,0) ],pKid, out.arcOrder);

#ifdef DO_LOGGING
			logVariable32("om0",out.mu);
#endif
			out.mu += valueCut(out.nCutLeft,out.nCutRight);
#ifdef DO_LOGGING
			logVariable32("om1",out.mu);
#endif
			out.mu += valueTransf(out.simTransf.theta, pKid,
				pSettings->nRefAngle, pSettings->nRefAngleImportance);
#ifdef DO_LOGGING
			logVariable32("om2",out.mu);
#endif

#ifdef DO_LOGGING
			logVariable32("oao",out.arcOrder[0]);
#endif

			/* Don't reject shapesless symbols based on shape only. Free passage until transformation is tested. */
			if (keyShapeUncertainty(pKid) >= db_quiteUncertain) out.mu = 0;

			if ( out.mu < pBestVersion->mu)
			{
				*pBestVersion = out;
			}
		}
		while (!bNoConcurrentArcs && (nNextPermRef = getNextMultiTouchArcOrder(&out, nNextPermRef)));
	}

#define NO_MULTI_ARC_CUTTING_DURING_FULLSEARCH
#if defined(NO_MULTI_ARC_CUTTING_DURING_FULLSEARCH)
	if (pKid->noBaseArcs > 1) nDifferentCutsToTry = 1;
#endif

	out = *pBestVersion;

	for(l=0 ; l<nDifferentCutsToTry ; l++)
	{
		for(r=0 ; r<nDifferentCutsToTry ; r++)
		{
			if (l == 0 && r == 0) continue; /* Already tested when looking for best arc order */

#define NO_DOUBLE_SIDE_ARC_CUTTING_DURING_FULLSEARCH
#if defined(NO_DOUBLE_SIDE_ARC_CUTTING_DURING_FULLSEARCH)
			if (l > 0 && r > 0) continue; /* Don't allow cutting both sides */
#endif

			/*
			// NOTE: This is a way to speed up the engine a LOT!! (Speeds up fullsearch by 65%)
			//	     The only drawback is that then characters with ligatures will work worse.
			if ( (pBestVersion->mu > 600 && l+r == 1) ||
				 (pBestVersion->mu > 400 && l+r == 2) )
			{
				return;
			}
			*/
			out.nCutLeft = pCutsToTry[l];
			out.nCutRight = pCutsToTry[r];

#ifdef DO_LOGGING
			logVariable32u("oix",out.DBindex.baseKeyIndex);
			logVariable32u("och",out.symbol[0]);
			logVariable32("ocl",out.nCutLeft);
			logVariable32("ocr",out.nCutRight);
			logVariable32("ao0",out.arcOrder[0]);
#endif
			/*Take prox. of the curve which is cut according to the settings above */
			proxCurveKeyInterleavedNoDerivate(&out.mu,&out.simTransf.theta,
				&pCurveCut[ TWO_DIM_CUT_ARRAY_INDEX(l,r) ],pKid, out.arcOrder);

#ifdef DO_LOGGING
			logVariable32("omu",out.mu);
			logVariable32("oth",out.simTransf.theta);

#endif
			out.mu += valueCut(out.nCutLeft,out.nCutRight);
#ifdef DO_LOGGING
			logVariable32("mu2",out.mu);
#endif
			out.mu += valueTransf(out.simTransf.theta, pKid,
				pSettings->nRefAngle, pSettings->nRefAngleImportance);
#ifdef DO_LOGGING
			logVariable32("mu2",out.mu);
#endif

			/* Don't reject shapesless symbols based on shape only. Free passage until transformation is tested. */
			if (keyShapeUncertainty(pKid) >= db_quiteUncertain) out.mu = 0;

			if ( out.mu < pBestVersion->mu)
			{
				*pBestVersion = out;
			}
		}
	}
}


static DECUMA_BOOL evaluateDiacriticKey(KID * pKid, SCR_CURVE * pDiacCurve,
		scrOUTPUT * pBestOutSoFar, scrOUTPUT * pBaseOut,
		int nBaseArcs, int nDiacArcs, const SCR_API_SETTINGS * pSettings)
{
	/*This function checks if the combinated symbol (base + diac) is in the selected category. */
	/*Then it compares the diacCurve to the diacKey specified in pKid. */
	/*This function is supposed to be optimized according to time, which means that */
	/*superflous code lines are removed. */
	/*pBestOutSoFar will contain a scrOUTPUT that is best so far. */

	/*NOTE: -pKid->nBaseArcs is set to zero in this function. */
	/*      -pBestOutSoFar->nBaseArcs will also be set to zero. */
	/*      -pBestOutSoFar->symbol,pBestOutSoFar->nCutLeft,pBestOutSoFar->nCutRight, */
	/*       pBestOutSoFar->nSegmentPunish, pBestOutSoFar->punish, pBestOutSoFar->simTransf */
    /*       are NOT set to anything in this function. */

	int j;

	DECUMA_INT16 diacMu;
	int diacTheta;
	int noBaseArcs = pKid->noBaseArcs;
	DECUMA_BOOL bBestSoFar=FALSE;
	DECUMA_INT8_DB_PTR ppArcOrders[MAX_ARC_ORDERS_PER_SYMBOL];
	int nArcOrders;

	pKid->noBaseArcs = 0; /*Set this temporarily */
	nArcOrders = kidGetArcOrders(pKid, ppArcOrders, sizeof(ppArcOrders)/sizeof(ppArcOrders[0]));

	for (j=0; j<nArcOrders; j++)
	{
		DECUMA_INT8 arcOrder[MAX_ARCS_IN_DIACRITIC];
		int i;

		decumaAssert(nDiacArcs <= MAX_ARCS_IN_DIACRITIC);
		/*Store the current arc order in the output */
		for (i=0; i<nDiacArcs; i++) arcOrder[i] = ppArcOrders[j][i];

		proxCurveKeyInterleavedNoDerivate(&diacMu,&diacTheta, pDiacCurve, pKid, arcOrder);

		diacMu += valueTransf(diacTheta, pKid, pSettings->nRefAngle,
			pSettings->nRefAngleImportance);
		if (diacMu < pBestOutSoFar->mu)
		{
			bBestSoFar=TRUE;
			pBestOutSoFar->mu = diacMu;
			pBestOutSoFar->DBindex = *pKid;
			pBestOutSoFar->nCutLeft = pBaseOut->nCutLeft;
			pBestOutSoFar->nCutRight = pBaseOut->nCutRight;

			for (i=0; i<nDiacArcs; i++)
				pBestOutSoFar->arcOrder[nBaseArcs+i] =
					advanceArcNumber(ppArcOrders[j][i],nBaseArcs);
		}
	}
	pKid->noBaseArcs = noBaseArcs;
	return bBestSoFar;
}

static void extractSomeArcsFromCurve( const SCR_CURVE * pCurve, int nFirstArc, int nArcsToExtract,
							   SCR_CURVE * pNewCurve)
{
	/*Create a curve with only the written nArcsToExtract arcs starting from arc number */
	/*nFirstArc (0-indexed) */

	int a;

	pNewCurve->noArcs = nArcsToExtract;
	pNewCurve->measure_id = g_measureId;

	for(a = 0 ; a < nArcsToExtract ; a++)
	{
		pNewCurve->Arcs[a] = pCurve->Arcs[nFirstArc+a];
	}

	preCalculateCurveInterleavedNoDerivate(pNewCurve);
}

static void makeAndEvaluateComplexKey(scrOUTPUT * pOut,
				const SCR_CURVE * pCurve, DECUMA_INT16 diacKeyXOffset,
				DECUMA_INT16 diacKeyYOffset, const SCR_API_SETTINGS * pSettings)
{

	SCR_CURVE curveCut = *pCurve;
	SCR_ARC arcThatWillBeCut;

	curveCut.Arcs[0] = &arcThatWillBeCut;
	curveCopy(&curveCut,pCurve);

	pOut->symbol = kidGetSymbol(&pOut->DBindex);

	kidSetDiacKeyOffset( &pOut->DBindex, diacKeyXOffset, diacKeyYOffset);

	InterpolateArc((curveCut.Arcs[0]), pCurve->Arcs[0],
		pOut->nCutLeft*128/NUMBER_OF_POINTS_IN_ARC,
		(32 - pOut->nCutRight)*128/NUMBER_OF_POINTS_IN_ARC);
	arcPrecalculate(curveCut.Arcs[0],NULL);
	preCalculateCurveInterleavedNoDerivate(&curveCut);

	proxCurveKeyInterleavedNoDerivate(&pOut->mu,&pOut->simTransf.theta,
		&curveCut,&pOut->DBindex, pOut->arcOrder);

	pOut->mu += valueCut(pOut->nCutLeft,pOut->nCutRight);
	pOut->mu += valueTransf(pOut->simTransf.theta,
		&pOut->DBindex, pSettings->nRefAngle, pSettings->nRefAngleImportance);
}
