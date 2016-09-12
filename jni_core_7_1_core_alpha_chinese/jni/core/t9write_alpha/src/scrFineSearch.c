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


/*$Header: /data/cvsroot/seattle/t9write/alphabetic/core/src/scrFineSearch.c,v 1.5 2011/02/14 11:41:14 jianchun_meng Exp $*/
#include <stddef.h> /* Definition of NULL */

#include "scrOutput.h"
#include "scrFineSearch.h"
#include "scrProxCurve.h"
#include "database.h"
#include "decumaMath.h"
#include "decumaAssert.h"
#include "scrAlgorithm.h"
#include "scrCurve.h"
#include "scrMeasureId.h"
#include "scrLigature.h"
#include "scrOutputHandler.h"
#include "globalDefs.h"

#include "decuma_hwr_types.h"
#include "decumaInterrupts.h"

#undef DO_LOGGING
#ifdef DO_LOGGING
#include "t9write_log.h"
#endif

/*////////////////////////////////////////////////////////////////////////// */
/*Local constants */
/*////////////////////////////////////////////////////////////////////////// */

#ifdef __CAMBOX
#define POSITION_PUNISH_SLOPE  (150)
#define POSITION_PUNISH_MAX    (150)
#else
/* JA note 080702: */
/* Reducing size (200 -> 20 = -90%) and position (200 -> 120 = -40%) punish slopes. */
/* Previous weights have recently been noted to be too large from a more general */
/* perspective (multiple script support). It is better to have a lower general punish */
/* and use these features for zooming when needed in special cases. */
#define POSITION_PUNISH_SLOPE  (90)
#define POSITION_PUNISH_MAX    (200)
#define SIZE_PUNISH_SLOPE      (60)
#define SIZE_PUNISH_MAX        (200)
#endif /* __CAMBOX */

#define ALLOW_DYNAMIC_DB_EXPANSION

/*////////////////////////////////////////////////////////////////////////// */
/*Local functions */
/*////////////////////////////////////////////////////////////////////////// */
#ifndef __CAMBOX
static DECUMA_INT32 PunishForSize(DECUMA_INT32 keySize, DECUMA_INT32 curveSize, DECUMA_INT32 dbDistBase2Help);
#endif

static DECUMA_INT32 PunishForPosition(const DECUMA_INT32 keyPos, const DECUMA_INT32 curvePos,
	DECUMA_INT32 dbDistBase2Help);

/*////////////////////////////////////////////////////////////////////////// */

/*////////////////////////////////////////////////////////////////////////// */
/*Local data */
/*////////////////////////////////////////////////////////////////////////// */
static const DECUMA_UNICODE qoutationMark[] = { '\"', '\0'};


DECUMA_STATUS scrFineSearch( OUTPUT_HANDLER * pFinesearchOutputs, const SCR_CURVE * pCurve,
				   const scrOUTPUT_LIST * pPrecalculatedBaseOutputs,
				   OUTPUT_HANDLER * pFullsearchOutputs, const int * baseLine, const int *helpLine,
				   const DECUMA_INTERRUPT_FUNCTIONS * pInterruptFunctions)
{
	SCR_ARC curveArcThatWillBeCut;
	SCR_CURVE curveCut;
	int j; /*Counters */
	KID kid;
	int arcToCut = 0;
	DECUMA_INT32 lowestUncutMu = 2000;
	int punishPerCut,nMaxCuts;
	scrOUTPUT * pOut;
	KEYINDEX bestUncutBaseKeyIndex = -1;

	decumaAssert(pFinesearchOutputs);
	decumaAssert(pFinesearchOutputs->nMaxOutputs > 0);
	decumaAssert(pFullsearchOutputs);
	decumaAssert(pFullsearchOutputs->nOutputs > 0);
	decumaAssert(pCurve);

	/* build the temporary curve */
	arcInit(&curveArcThatWillBeCut);
	curveInit(&curveCut);

	curveCut.Arcs[ arcToCut ] = &curveArcThatWillBeCut;
	for(j = 1 ; j < pCurve->noArcs; j++)
		curveCut.Arcs[j] = pCurve->Arcs[j];

	/*JM: This is all designed to always cut in the first arc. (CURVEIndexed) */

	curveCopy(&curveCut,pCurve);

	/*Find the the lowest possible mu-value without cutting. */
	for(  pOut = seqGetWorstOutput(pFullsearchOutputs,NULL);
		  pOut != NULL ;
		  pOut = seqGetNextBetterOutput(pFullsearchOutputs,NULL) )
	{
		/*NOTE: This code changes the muvalue and punish for the outputs from the fullsearch. */

#ifdef DO_LOGGING
			logVariable32("omu",pOut->mu);
			logVariable32u("och",pOut->symbol[0]);
			logVariable32u("idx",pOut->DBindex.baseKeyIndex);
#endif

#ifdef ALLOW_DYNAMIC_DB_EXPANSION
		if (kidGetSymbol(&pOut->DBindex)[0] == ',' && pOut->DBindex.noBaseArcs == 1 && pOut->DBindex.baseKeyIndex == 10) /* Key for , and ' */
		{
			/* Small key fix. Small keys gets poor resolution in DB (new format will solve this but is not merged */
			/* from Vietnamese to main branch yet). For English this is causing segmentation problems for ' and , in */
			/* particular and this problem is increasing in on-top mode. */
			/* */
			/* A specifically noted problem is that 's (written on-top) becomes ; or ! due to too high */
			/* proximity value for ' */
			/* */
			/* For now we will mitigate this with an engine hack. The hack will look for the key for , and ' and modify */
			/* the key index to that of / and I and measure the mu value. The smalles mu is then used. */
			/* */
			/* TODO Replace engine hack with proper DB solution */

			DECUMA_INT16 mu = MAX_DECUMA_INT16;

			if (pOut->DBindex.pKey->curveIdx == 10) /* Verify key index assumption */
			{
				pOut->mu = MAX_DECUMA_INT16;

				pOut->DBindex.baseKeyIndex = 14; /* / */
				kidSetBaseKeyPointer(&pOut->DBindex);
				if (pOut->DBindex.pKey->curveIdx == 13) { /* Verify key index assumption */
					proxCurveKey(&mu , &pOut->simTransf, pCurve, &pOut->DBindex, pOut->arcOrder);
				} else {
					decumaAssert(0);
				}

				if (mu < pOut->mu)
				{
					pOut->mu = mu;
					bestUncutBaseKeyIndex = pOut->DBindex.baseKeyIndex;
				}

				pOut->DBindex.baseKeyIndex = 217; /* I */
				kidSetBaseKeyPointer(&pOut->DBindex);
				if (pOut->DBindex.pKey->curveIdx == 216) { /* Verify key index assumption */
					proxCurveKey(&mu, &pOut->simTransf, pCurve, &pOut->DBindex, pOut->arcOrder);
				} else {
					decumaAssert(0);
				}

				if (mu < pOut->mu)
				{
					pOut->mu = mu;
					bestUncutBaseKeyIndex = pOut->DBindex.baseKeyIndex;
				}

				/* Restore and get real simTransf */
				pOut->DBindex.baseKeyIndex = 10; /* , and ' */
				kidSetBaseKeyPointer(&pOut->DBindex);
				proxCurveKey(&mu, &pOut->simTransf, pCurve, &pOut->DBindex, pOut->arcOrder);
				if (mu < pOut->mu)
				{
					pOut->mu = mu;
					bestUncutBaseKeyIndex = pOut->DBindex.baseKeyIndex;
				}
			}
			else
			{
				decumaAssert(0);
				proxCurveKey(&pOut->mu , &pOut->simTransf, pCurve, &pOut->DBindex, pOut->arcOrder);
			}
		}
		else
#endif
			proxCurveKey(&pOut->mu , &pOut->simTransf, pCurve, &pOut->DBindex, pOut->arcOrder);

		pOut->punish = valueTransf(pOut->simTransf.theta , &pOut->DBindex, 0, 1000);

		lowestUncutMu = minNUMB(lowestUncutMu,(DECUMA_INT32)pOut->mu + pOut->punish);

		/*Check if we should abort or continue */
		if (TEST_ABORT_RECOGNITION(pInterruptFunctions))
			goto scrFineSearch_abort;

	}

	getPunishPerCut(lowestUncutMu, &punishPerCut, &nMaxCuts);


#ifdef DO_LOGGING
	logVariable32u("lUM",lowestUncutMu);
#endif

	for(  pOut = seqGetWorstOutput(pFullsearchOutputs,NULL);
		  pOut != NULL ;
		  pOut = seqGetNextBetterOutput(pFullsearchOutputs,NULL) )
	{
		scrOUTPUT proxData;
		int s;

		kid = pOut->DBindex;

		if(!databaseValidKID( kid ))
			continue;

		/* Check if we have ligature values supplied. */
		/* Let GausNewton trim the curve ligatures, and build the new curve in curveCut */
		proxData = *pOut;
		proxData.punish = 0;

#ifdef ALLOW_DYNAMIC_DB_EXPANSION
		if (bestUncutBaseKeyIndex >= 0 && kidGetSymbol(&proxData.DBindex)[0] == ',' && proxData.DBindex.noBaseArcs == 1 && proxData.DBindex.baseKeyIndex == 10)
		{
			proxData.DBindex.baseKeyIndex = bestUncutBaseKeyIndex; /* Use best shape */
			kidSetBaseKeyPointer(&proxData.DBindex);
			GaussNewton(&proxData, 0, arcToCut, pCurve, &curveCut,
				pPrecalculatedBaseOutputs, 0,0,0,nMaxCuts);
			proxData.DBindex.baseKeyIndex = pOut->DBindex.baseKeyIndex; /* Restore */
			kidSetBaseKeyPointer(&proxData.DBindex);
		}
		else
#endif

		GaussNewton(&proxData, 0, arcToCut, pCurve, &curveCut,
				pPrecalculatedBaseOutputs, 0,0,0,nMaxCuts);

		/*Check if we should abort or continue */
		if (TEST_ABORT_RECOGNITION(pInterruptFunctions))
			goto scrFineSearch_abort;


		/* Check if GaussNewton has cut the curve! */
		if ( proxData.nCutLeft != 0 || proxData.nCutRight != 0 )
		{
			/* Take the curve built by GaussNewton and measure it against the key. */
#ifdef ALLOW_DYNAMIC_DB_EXPANSION
			if (bestUncutBaseKeyIndex >= 0 && kidGetSymbol(&proxData.DBindex)[0] == ',' && proxData.DBindex.noBaseArcs == 1 && proxData.DBindex.baseKeyIndex == 10)
			{
				proxData.DBindex.baseKeyIndex = bestUncutBaseKeyIndex; /* Use best shape */
				kidSetBaseKeyPointer(&proxData.DBindex);
				proxCurveKey(&proxData.mu , &proxData.simTransf, &curveCut, &kid, proxData.arcOrder);
				/*Check if we should abort or continue */
				if (TEST_ABORT_RECOGNITION(pInterruptFunctions))
					goto scrFineSearch_abort;

				proxData.DBindex.baseKeyIndex = pOut->DBindex.baseKeyIndex; /* Restore */
				kidSetBaseKeyPointer(&proxData.DBindex);
			}
			else
#endif
				proxCurveKey(&proxData.mu , &proxData.simTransf, &curveCut, &kid, proxData.arcOrder);
				/*Check if we should abort or continue */
				if (TEST_ABORT_RECOGNITION(pInterruptFunctions))
					goto scrFineSearch_abort;


#ifdef DO_LOGGING
			logVariable32u("pdM",proxData.mu);
#endif
			/* punish if there were some ligatures that were cut away. */
			PunishForCutting(&proxData, punishPerCut);

			/* punish if the curve was rotated to match the key. */
			proxData.punish = (DECUMA_INT16)(proxData.punish+valueTransf(proxData.simTransf.theta , &kid, 0, 1000));

#ifdef DO_LOGGING
			logVariable32u("pdP",proxData.punish);
#endif
			if (proxData.mu + proxData.punish > pOut->mu + pOut->punish)
			{
				/*If the value is better uncut, then don't cut!!! */
				proxData = *pOut;
			}
		}
		else
		{	/* Use the uncut value! */
			proxData = *pOut;
		}

		proxData.arcTimelineDiffMask = kidGetArcTimelineDiffMask(&proxData.DBindex);


		/* Expand both result to an orignal plus an alternative result. This is important if */
		/* for example scrlib results should be combined with linguistic result in order to */
		/* avoid early rejection of one of the symbols or just to get a sensible scrlib */
		/* candidate list. */
		for (s = original; s < both; s++)
		{
			scrOUTPUT proxDataCopy;

			if (proxData.outSymbol != both && proxData.outSymbol != s) continue;

			proxDataCopy = proxData;

			proxDataCopy.outSymbol = s;

			/*Punish for scale and vertical offset */
			{
				SCR_CURVE_PROP curveProp;
				DECUMA_INT16 nRefPunish;
				int nAdjustmentFactor;

				curveGetProp(pCurve, &curveProp, kid.noBaseArcs);

				/* NOTE Behavioural change: */
				/* Measure scale and vertical distance offset on uncut curve for correct and consistent adjustment and get punish function behaviour */
				/* NOTE In theory it might be more correct to use cut curve for scale and vertical offset measure, but then it has to be supported */
				/* consistently. Once this is possible it might make sense to go back to using cut curve for this. */
				nRefPunish = PunishForScalingAndVerticalOffset(&proxDataCopy, &curveProp, &kid, baseLine ? *baseLine : 0, helpLine ? *helpLine : 0, &nAdjustmentFactor);

				if (nAdjustmentFactor)
				{
					/* Shifting distance focus to position (and scale) */
					proxDataCopy.mu /= nAdjustmentFactor;
					proxDataCopy.punish /= nAdjustmentFactor;
				}

				proxDataCopy.punish += nRefPunish;
			}

			addToOutputHandlerIfGoodEnoughAndNotTooManyOfSameSymbolOrType( pFinesearchOutputs,
						proxDataCopy.mu + proxDataCopy.punish, &proxDataCopy, 1);

			/*Check if we should abort or continue */
			if (TEST_ABORT_RECOGNITION(pInterruptFunctions))
				goto scrFineSearch_abort;
		}
	}
	return decumaNoError;

scrFineSearch_abort:
	pFinesearchOutputs->nOutputs = 0;
	return decumaRecognitionAborted;
}



DECUMA_INT16 PunishForScalingAndVerticalOffset(const scrOUTPUT *proxData,
										const SCR_CURVE_PROP * curveProp,
										const KID * pKid,
										int baseLine,
										int helpLine,
										int* pnAdjustmentFactor)
{
	SYMBOL_TYPE_INFO_PTR pTypeInfo = kidGetTypeInfo(pKid);

	DECUMA_INT32 curvePosY; /* The Y-coord for the middle point of the curve */
					 /* (scaled and translated to the size and position of */
					 /* the templates in the DB). */
	DECUMA_INT32 curveSizeY;
	DECUMA_INT8 keyMaxY;
	DECUMA_INT8 keyMinY;
	DECUMA_INT16 keyPosY;  /* The Y-coord for the middle point of the key. */
	DECUMA_INT16 keySizeY;
	DECUMA_INT16 altKeySizeY;   /* To adjust key when punishing alternative symbol */
	const DECUMA_INT16 dbDistBase2Help = (DECUMA_INT16) propDBGetDistBase2Help( pKid->pPropDB );
	int bScaleOnly = 0;

	DECUMA_INT16 punish = 0;

	decumaAssert( pTypeInfo );
	decumaAssert( proxData );

	*pnAdjustmentFactor = 1;

	/* Check if the helplines exists. */
	if ( baseLine == helpLine )
	{
		if (scrCurveIsTooSmallForProximityMeasure(curveProp, baseLine, helpLine))
		{
			/* The curve is small in absolute pixels (since no reference is available) */
			/* We need to reflect this scale feature in some way since small curves can */
			/* get arbitrary shape match. Fake reference lines that are still assumed quite */
			/* tight in pixels and skip position measure. */
			baseLine = 0;
			helpLine = baseLine - 15;

			/* Verify */
			decumaAssert(scrCurveIsTooSmallForProximityMeasure(curveProp, baseLine, helpLine));

			bScaleOnly = 1;
		}
		else
			return 0;
	}

	kidGetMinMaxY(pKid,&keyMinY,&keyMaxY);

	/* Only the base arcs (not the diacritic arcs) are used here. */
	curvePosY = ( ((curveProp->base_max_y + (DECUMA_INT32)curveProp->base_min_y)/2 - baseLine) *
		(DECUMA_INT32)dbDistBase2Help )/ (baseLine - (DECUMA_INT32)helpLine);
	curveSizeY = ( ((DECUMA_INT32)curveProp->base_max_y - curveProp->base_min_y) *
		(DECUMA_INT32)dbDistBase2Help )/ (baseLine - (DECUMA_INT32)helpLine);

	keyPosY = (keyMaxY + keyMinY)/2;
	keySizeY = keyMaxY - keyMinY;

	decumaAssert(proxData->outSymbol != both);

	if (proxData->outSymbol == alternative)
	{
		if (CAN_BE_TRANSLATED_TO_UPPER(pTypeInfo)) /* Move Key */
		{
			keyPosY += (DECUMA_INT32)ALT_SYMBOL_TRANSLATION*dbDistBase2Help;
		}
		else  /* Scale Key */
		{
			altKeySizeY = keySizeY * (DECUMA_INT32)ALT_SYMBOL_SCALE_NUM / ALT_SYMBOL_SCALE_DENOM;
			/* The middle point is changed when the key is scaled. */
			keyPosY -= ((DECUMA_INT32)altKeySizeY - keySizeY)/2;
			keySizeY=altKeySizeY;
		}
	}

	#ifndef __CAMBOX
		/* Don't punish for scale if there */
		/* are no visible helplines */

		/* Do not punish symbols were the size is unknown. */
		if ( keySizeUncertainty( pKid ) < db_veryUncertain )
		{
			punish += (DECUMA_INT16)
				PunishForSize(keySizeY, (int) curveSizeY, dbDistBase2Help);
		}
	#endif /*__CAMBOX */


	if ( !bScaleOnly && keyPositionUncertainty( pKid) < db_veryUncertain  )
	{
			int posPunish = PunishForPosition(keyPosY, (int) curvePosY, dbDistBase2Help);

#ifdef ALLOW_DYNAMIC_DB_EXPANSION
			if (proxData->outSymbol == alternative && CAN_BE_TRANSLATED_TO_UPPER(pTypeInfo) && kidGetSymbol(pKid)[0] == ',' && kidGetSymbol(pKid)[1] == 0)
			{
				/* ' is a very frequent character in words of the frequently used English language. The */
				/* database has deficient position capturing of this character which can cause it to suffer */
				/* unbalanced segmentation errors in conflict situation. Allow more the y position flexibility */
				/* with this engine hack. */
				/* */
				/* A specifically noted problem is that 's (written on-top) becomes ; or ! due to too high */
				/* proximity value for ' */
				/* */
				/* TODO Replace engine hack with proper DB solution */

				keyPosY += (DECUMA_INT32)ALT_SYMBOL_TRANSLATION*dbDistBase2Help/2;
				posPunish = minNUMB(posPunish, PunishForPosition(keyPosY, (int) curvePosY, dbDistBase2Help));
				keyPosY += (DECUMA_INT32)ALT_SYMBOL_TRANSLATION*dbDistBase2Help/2;
				posPunish = minNUMB(posPunish, PunishForPosition(keyPosY, (int) curvePosY, dbDistBase2Help));
			}
#endif

			punish = (DECUMA_INT16)(punish+posPunish);
	}

	if (scrCurveAndKeyAreQuiteSmall(proxData, curveProp, pKid, baseLine, helpLine))
	{
		/* Small stuff */
		/* Shift distance focus to position (and scale) */
		punish *= 2;
		*pnAdjustmentFactor *= 2;
	}

	if (scrCurveIsTooSmallForProximityMeasure(curveProp, baseLine, helpLine))
	{
		/* Really small stuff */
		/* Shift distance focus even more to position (and scale) */
		punish *= 2;
		*pnAdjustmentFactor *= 2;
	}

	if (bScaleOnly) punish *= 2; /* Double scale punish, since no position punish */

	return punish;
}

#ifndef __CAMBOX
static DECUMA_INT32 PunishForSize(DECUMA_INT32 keySize, DECUMA_INT32 curveSize, DECUMA_INT32 dbDistBase2Help)
{
	DECUMA_INT32 punish;

	/* A constant is added to the sizes to limit the punishment */
	/* on very small curves. */
	keySize += (DECUMA_INT32)dbDistBase2Help / 4;
	curveSize += (DECUMA_INT32)dbDistBase2Help / 4;

	if ( keySize > curveSize )
		punish = (( keySize - curveSize) * SIZE_PUNISH_SLOPE) / curveSize;
	else
		punish = (( curveSize - keySize) * SIZE_PUNISH_SLOPE) / keySize;

	return minNUMB(punish, SIZE_PUNISH_MAX);
}
#endif /*__CAMBOX */

static DECUMA_INT32 PunishForPosition(const DECUMA_INT32 keyPos, const DECUMA_INT32 curvePos,
	DECUMA_INT32 dbDistBase2Help)
{
	DECUMA_INT32 punish;

	punish = ((curvePos - keyPos) * POSITION_PUNISH_SLOPE) /
		dbDistBase2Help;
	punish = decumaAbs(punish);

	return minNUMB(punish, POSITION_PUNISH_MAX);
}

int scrCurveAndKeyAreQuiteSmall(const scrOUTPUT *proxData,
				const SCR_CURVE_PROP * curveProp, const KID * pKid, const int baseLine,
				const int helpLine)
{
	/* TODO: KOMMENTERA MERA!!!! */
	const DECUMA_INT32 keyNorm = kidGetQSum(pKid) * curveProp->mass -
		(DECUMA_INT32)kidGetSumX(pKid) * kidGetSumX(pKid) - (DECUMA_INT32)kidGetSumY(pKid) * kidGetSumY(pKid);
	const DECUMA_INT32 bigCurveNorm = ((DECUMA_INT32)baseLine - helpLine) * ((DECUMA_INT32)baseLine - helpLine) *
		curveProp->mass * curveProp->mass;
	const DECUMA_INT32 dbDistBase2Help = propDBGetDistBase2Help( pKid->pPropDB );
	const DECUMA_INT32 bigKeyNorm = dbDistBase2Help * dbDistBase2Help *
		curveProp->mass * curveProp->mass;
	const DECUMA_INT32 factor = 32; /* how much bigger should the big norm be? */

	if ( (keyNorm * factor < bigKeyNorm) &&
		 (curveProp->norm * factor < bigCurveNorm) )
	{
		return 1;
	}
	else
	{
		/*Hack that boosts hitrate for double hyphen (") */
		/*If the curve is small, and above mid base/helpline area and  */
		/*the fullsearch interpretation is " then deem it quite small  */
		/*=> bonus in proximity comparisons */
		if ( isSameSymbol(kidGetSymbol(pKid), qoutationMark, 0) )
		{
			if ( curveProp->max_y < ((DECUMA_INT32)baseLine + helpLine)/2 &&
				 (DECUMA_INT32)curveProp->max_y - curveProp->min_y < baseLine - helpLine &&
				 (DECUMA_INT32)curveProp->max_x - curveProp->min_x < (baseLine - helpLine)/2 )
			{
				return TRUE;
			}
		}
		return FALSE;
	}
}
/************************************************** End of file ***/
