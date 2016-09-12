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


/*$Header: /data/cvsroot/seattle/t9write/alphabetic/core/src/scrProxCurve.c,v 1.5 2011/02/14 11:41:14 jianchun_meng Exp $*/

/* Calculates proximity value */

#ifdef MATLAB_MEX_FILE
#include "mex.h"
#endif

#include "database.h"
#include "scrProxCurve.h"
#include "decumaMath.h"
#include "decumaSimTransf.h"
#include "scrCurve.h"
#include "decumaMemory.h"
#include "scrMeasureId.h"
#include "decumaAssert.h"
#include "globalDefs.h"

/* #undef DO_LOGGING */
#ifdef DO_LOGGING
#define DO_LOGGING_FULLSEARCH
/* #define DO_LOGGING_FINESEARCH */
#include "t9write_log.h"
#endif

DECUMA_HWR_PRIVATE_DATA_C const DECUMA_INT16 maxProxValue = 1000;

#define SCR_PIXEL_SMALL_CURVE_SIZE 5

#ifdef __CAMBOX
/* Larger dots shall be accepted in cambox mode. */
#define SMALL_CURVE_SIZE_RELATIVE_HELP_LINES 2
#else
#define SMALL_CURVE_SIZE_RELATIVE_HELP_LINES 3
#endif

/*#define _DEBUG_PROXIMITY */

#ifndef _DEBUG
#undef _DEBUG_PROXIMITY
#endif
/***************************************************************************/

static void proxStoreMu(DECUMA_INT16 * pMuOut, DECUMA_INT32 sum1, DECUMA_INT32 sum2, DECUMA_INT32 sumSjNorm,
						float curveNorm, DECUMA_UINT32 qNorm);


#ifdef _DEBUG_PROXIMITY
/*Parameters: */
/* measure_id - If set to positive value (>0), indicates one of the measure ids that tells  */
/*              which parts of the curve that should be measured. */
/* onlyArc    - If set to non-negative value we will only measure the arc with id onlyArc (keyIndexed) */
/* startIdx   - If set to non-negative value we will only measure on points with  */
/*              index >= startIdx (keyIndexed) of each arc */
/* stopIdx    - If set to non-negative value we will only measure on points with  */
/*              index <= stopIdx (keyIndexed) of each arc */
static void proxCurveKeyDebug(DECUMA_INT16 * pMuOut, SIM_TRANSF * pSimTransfOut, const SCR_CURVE *pCurve, const KID * pKid,
	int measure_id, int onlyArc, int startIdx, int stopIdx, DECUMA_UINT8 alpha, const DECUMA_INT8 * arcOrder);
#endif

/***************************************************************************/

DECUMA_BOOL scrCurveIsTooSmallForProximityMeasure(const SCR_CURVE_PROP * pCurveProp,
										   int nBaseLine, int nHelpLine)
{
	if(pCurveProp->noArcs == 1)
	{
		DECUMA_INT32 maxSmallCurveSize;

		/* TODO Use macro to determine if helplines are available. */
		if ( nBaseLine != nHelpLine ) /* Helplines are avialable... */
		{
			maxSmallCurveSize = maxNUMB(SCR_PIXEL_SMALL_CURVE_SIZE,
				((DECUMA_INT32)nBaseLine - nHelpLine) / SMALL_CURVE_SIZE_RELATIVE_HELP_LINES);
		}
		else
		{
			maxSmallCurveSize = SCR_PIXEL_SMALL_CURVE_SIZE;
		}
		if (norm((DECUMA_INT32)pCurveProp->max_x - pCurveProp->min_x + 1, (DECUMA_INT32)pCurveProp->max_y - pCurveProp->min_y + 1) <= maxSmallCurveSize)
		{
			return TRUE;
		}
	}
	return FALSE;
}

void proxCurveKey(DECUMA_INT16 * pMuOut, SIM_TRANSF * pSimTransfOut,
				  const SCR_CURVE *pCurve, const KID * pKid,
				  const DECUMA_INT8 * arcOrder)
{
	/*Computes the similarity proximity measure, */
	/*which is a value between 0 and maxProxValue. */

	DECUMA_INT32 sumT1 = 0;
	DECUMA_INT32 sumT2 = 0;
	DECUMA_INT32 qSum = 0; /* TODO Could use more DB precalc /JA */
	DECUMA_INT32 dqSum = 0; /* TODO Could use more DB precalc /JA */
	DECUMA_INT32 curveSumX = 0;
	DECUMA_INT32 curveSumY = 0;
	DECUMA_INT32 curveSumDX = 0;
	DECUMA_INT32 curveSumDY = 0;
	DECUMA_INT32 curveSumAbsDX = 0;
	DECUMA_INT32 curveSumAbsDY = 0;
	DECUMA_INT32 curveNorm = 0;
	int a;

	DECUMA_INT32 qNorm;
	DECUMA_UINT32 sumSjNorm = 0;
	SCR_ARC * pArc;
	DECUMA_INT16 * pDX, * pDY, * pX, * pY;
	DECUMA_INT8_DB_PTR keyArc;
	DECUMA_INT32 keySumX = 0,keySumY = 0;
	DECUMA_INT32 keySumDX = 0,keySumDY = 0;
	DECUMA_INT32 keySumAbsDX = 0,keySumAbsDY = 0;
	DECUMA_UINT8 arcRotationMask = keyGetArcRotationMask(pKid);

	DECUMA_INT32 keyMass = 0;

	/* If this asserts, a wrong function have been used */
	/* to precalculate the curve */
	decumaAssert(pCurve->measure_id != -1);

	/* If this asserts there here something is really fishy. Explanation: */
	/* The only case when we alter the measure id is when we are about to */
	/* do zooming. And when we are zooming we must use */
	/* proxCurveKeyWithoutPreCalculatedKey since the key are not precalculated */
	/* for anything else then measure_id == 0. (This may change in the future if the */
	/* vectors KEY::norm_1[] and norm_2 are sized up in the future. then it may */
	/* be neccesary to rewrite proxCurveKey to be able to handle several measure ids. */
	/* If that is the case, remember to shortcut the inner for loop with a continue */
	/* when the measure id is zero for a element. */
	decumaAssert(pCurve->measure_id == 0);

	for(a = 0 ; a < curveGetNoArcs(pCurve) ; a++)
	{
		const int arcNr = getArcNr(arcOrder,a);
		int i;
		int noIterations;
		register DECUMA_INT32 arcScProd1 = 0;
		register DECUMA_INT32 arcScProd2 = 0;

		int alpha_x = pCurve->Arcs[arcNr]->alpha_x;
		int alpha_y = pCurve->Arcs[arcNr]->alpha_y;
		int alpha_dx = pCurve->Arcs[arcNr]->alpha_dx;
		int alpha_dy = pCurve->Arcs[arcNr]->alpha_dy;

		DECUMA_INT32 keyArcSumX = 0,keyArcSumY = 0;
		DECUMA_INT32 keyArcSumDX = 0,keyArcSumDY = 0;
		DECUMA_INT32 keyArcSumAbsDX = 0,keyArcSumAbsDY = 0;

		pArc = pCurve->Arcs[ arcNr ];
		keyArc = kidGetArcPointer( pKid, a);

		noIterations = curveGetArcLength(pCurve, arcNr);

		decumaAssert( noIterations >= 1 );

		if (arcIsReverse(arcOrder,a))
		{
			pX  = &pArc->x[noIterations - 1];
			pY  = &pArc->y[noIterations - 1];

			for(i = 0; i < noIterations; i++, pX--, pY--)
	            	{
		                register DECUMA_INT32 keyX = alpha_x * keyarcGetX( keyArc,i);
		                register DECUMA_INT32 keyY = alpha_y * keyarcGetY( keyArc,i);

				keyX += alpha_x * keyGetArcOffsetX(pKid, a);/* Add offset if this arc is part of the diacKey. */
				keyY += alpha_y * keyGetArcOffsetY(pKid, a);/* Add offset if this arc is part of the diacKey. */

				arcScProd1 += (keyX * alpha_x * (*pX ) + keyY * alpha_y * (*pY )) / 64;
		                arcScProd2 += (keyY * alpha_x * (*pX ) - keyX * alpha_y * (*pY )) / 64;

				qSum += (keyX * keyX + keyY * keyY) / 64;

				keyArcSumX += keyX;
				keyArcSumY += keyY;
	            	}

			pDX  = &pArc->dx[noIterations - 1];
			pDY  = &pArc->dy[noIterations - 1];


			/*Derivative */
			for(i = 0; i < noIterations - 1; i++, pDX--, pDY--)
			{
				register const DECUMA_INT32 keyDX =
					alpha_dx * (keyarcGetX( keyArc,i+1) - keyarcGetX( keyArc,i));
				register const DECUMA_INT32 keyDY =
					alpha_dy * (keyarcGetY( keyArc,i+1) - keyarcGetY( keyArc,i));

				/* Note: Subtraction instead of addition because the curve */
				/* derivatives have opposite sign when the arc is reverse */
		                arcScProd1 -= (keyDX * alpha_dx * (*pDX) + keyDY * alpha_dy * (*pDY)) / 64;
		                arcScProd2 -= (keyDY * alpha_dx * (*pDX) - keyDX * alpha_dy * (*pDY)) / 64;

				dqSum += (keyDX * keyDX + keyDY * keyDY) / 64;

				keyArcSumDX += keyDX;
				keyArcSumDY += keyDY;
				keyArcSumAbsDX += decumaAbs(keyDX);
				keyArcSumAbsDY += decumaAbs(keyDY);
			}
			/* Finally do the last calculation */
			{
				register const DECUMA_INT32 keyDX =
					alpha_dx * (keyarcGetX( keyArc,i) - keyarcGetX( keyArc,i-1));
				register const DECUMA_INT32 keyDY =
					alpha_dy * (keyarcGetY( keyArc,i) - keyarcGetY( keyArc,i-1));

		                arcScProd1 -= (keyDX * alpha_dx * (*pDX) + keyDY * alpha_dy * (*pDY)) / 64;
		                arcScProd2 -= (keyDY * alpha_dx * (*pDX) - keyDX * alpha_dy * (*pDY)) / 64;

				dqSum += (keyDX * keyDX + keyDY * keyDY) / 64;

				keyArcSumDX += keyDX;
				keyArcSumDY += keyDY;
				keyArcSumAbsDX += decumaAbs(keyDX);
				keyArcSumAbsDY += decumaAbs(keyDY);
	      		}
		}
		else
		{
			pX  = pArc->x;
			pY  = pArc->y;

			for(i = 0; i < noIterations; i++, pX++, pY++)
			{
		                register DECUMA_INT32 keyX = alpha_x * keyarcGetX( keyArc,i);
		                register DECUMA_INT32 keyY = alpha_y * keyarcGetY( keyArc,i);

				keyX += alpha_x * keyGetArcOffsetX(pKid, a);/* Add offset if this arc is part of the diacKey. */
				keyY += alpha_y * keyGetArcOffsetY(pKid, a);/* Add offset if this arc is part of the diacKey. */

		                arcScProd1 += (keyX * alpha_x * (*pX ) + keyY * alpha_y * (*pY )) / 64;
		                arcScProd2 += (keyY * alpha_x * (*pX ) - keyX * alpha_y * (*pY )) / 64;

				qSum += (keyX * keyX + keyY * keyY) / 64;

				keyArcSumX += keyX;
				keyArcSumY += keyY;
			}

			pDX = pArc->dx;
			pDY = pArc->dy;


			/*Derivative */
		        for(i = 0; i < noIterations - 1; i++, pDX++, pDY++)
		        {
		            register const DECUMA_INT32 keyDX =
		            		alpha_dx * (keyarcGetX( keyArc,i+1) - keyarcGetX( keyArc,i));
		            register const DECUMA_INT32 keyDY =
		            		alpha_dy * (keyarcGetY( keyArc,i+1) - keyarcGetY( keyArc,i));

		                arcScProd1 += (keyDX * alpha_dx * (*pDX) + keyDY * alpha_dy * (*pDY)) / 64;
		                arcScProd2 += (keyDY * alpha_dx * (*pDX) - keyDX * alpha_dy * (*pDY)) / 64;

				dqSum += (keyDX * keyDX + keyDY * keyDY) / 64;

				keyArcSumDX += keyDX;
				keyArcSumDY += keyDY;
				keyArcSumAbsDX += decumaAbs(keyDX);
				keyArcSumAbsDY += decumaAbs(keyDY);
			}
			/* Finally do the last calculation */
			{
				register const DECUMA_INT32 keyDX =
					alpha_dx * (keyarcGetX( keyArc,i) - keyarcGetX( keyArc,i-1));
				register const DECUMA_INT32 keyDY =
					alpha_dy * (keyarcGetY( keyArc,i) - keyarcGetY( keyArc,i-1));

	                	arcScProd1 += (keyDX * alpha_dx * (*pDX) + keyDY * alpha_dy * (*pDY)) / 64;
        		        arcScProd2 += (keyDY * alpha_dx * (*pDX) - keyDX * alpha_dy * (*pDY)) / 64;

				dqSum += (keyDX * keyDX + keyDY * keyDY) / 64;

				keyArcSumDX += keyDX;
				keyArcSumDY += keyDY;
				keyArcSumAbsDX += decumaAbs(keyDX);
				keyArcSumAbsDY += decumaAbs(keyDY);
	     		}

		}

		keyMass += noIterations;

		if ( keyArcCanBeRotated( a, arcRotationMask) )
		{
			DECUMA_INT32 a1,a2;
			DECUMA_INT32 sj1, sj2;
			DECUMA_UINT32 sjNorm;

			/* No need for overflow check here */
			a1 = (alpha_x * pArc->sum_x * keyArcSumX + alpha_y * pArc->sum_y * keyArcSumY) / 64;
			a2 = (alpha_x * pArc->sum_x * keyArcSumY - alpha_y * pArc->sum_y * keyArcSumX) / 64;

			sj1 = NUMBER_OF_POINTS_IN_ARC * arcScProd1 - a1; /* (sj1,sj2) = sj(Phi,Psi) ... (here multiplied with arc mass) */
			sj2 = NUMBER_OF_POINTS_IN_ARC * arcScProd2 - a2;

			sjNorm = pCurve->noArcs * normFine(sj1,sj2);

			decumaAssert( sumSjNorm <= sumSjNorm + sjNorm);
			sumSjNorm += sjNorm;

			sumT1 += a1 / NUMBER_OF_POINTS_IN_ARC; /*(sumT1,sumT2) = t(Phi,Psi) */
			sumT2 += a2 / NUMBER_OF_POINTS_IN_ARC;
		}
		else
		{
			sumT1 += arcScProd1;
			sumT2 += arcScProd2;
		}

		curveNorm += (
			alpha_x * alpha_x * arcGetSumX2(pArc) +
			alpha_y * alpha_y * arcGetSumY2(pArc) +
			alpha_dx * alpha_dx * arcGetSumDX2(pArc) +
			alpha_dy * alpha_dy * arcGetSumDY2(pArc)) / 64;

		curveSumX += alpha_x * arcGetSumX(pArc) / 8;
		curveSumY += alpha_y * arcGetSumY(pArc) / 8;
		curveSumDX += alpha_dx * arcGetSumDX(pArc) / 8;
		curveSumDY += alpha_dy * arcGetSumDY(pArc) / 8;
		curveSumAbsDX += alpha_dx * arcGetSumAbsDX(pArc) / 8;
		curveSumAbsDY += alpha_dy * arcGetSumAbsDY(pArc) / 8;

		keySumX += keyArcSumX / 8;
		keySumY += keyArcSumY / 8;
		keySumDX += keyArcSumDX / 8;
		keySumDY += keyArcSumDY / 8;
		keySumAbsDX += keyArcSumAbsDX / 8;
		keySumAbsDY += keyArcSumAbsDY / 8;
	}

	decumaAssert( decumaAbs(sumT1) <= MAX_DECUMA_INT32 / MAX_NUMBER_OF_ARCS_IN_CURVE / NUMBER_OF_POINTS_IN_ARC); /*Overflow check */
	decumaAssert( decumaAbs(sumT2) <= MAX_DECUMA_INT32 / MAX_NUMBER_OF_ARCS_IN_CURVE / NUMBER_OF_POINTS_IN_ARC); /*Overflow check */

#ifdef DO_LOGGING_FINESEARCH
	logVariable32("S1",sumT1);
	logVariable32("S2",sumT2);
#endif
	sumT1 *= curveGetMass(pCurve);
	sumT1 -= curveSumX * keySumX;
	sumT1 -= curveSumY * keySumY;
	sumT1 -= curveSumDX * keySumDX;
	sumT1 -= curveSumDY * keySumDY;
	sumT1 += g_alpha_sum_dx * g_alpha_sum_dx * (64 * curveSumDX * keySumDX / g_alpha_dx / g_alpha_dx) / 64;
	sumT1 += g_alpha_sum_dy * g_alpha_sum_dy * (64 * curveSumDY * keySumDY / g_alpha_dy / g_alpha_dy) / 64;
	sumT1 += g_alpha_sum_abs_dx * g_alpha_sum_abs_dx * (64 * curveSumAbsDX * keySumAbsDX / g_alpha_dx / g_alpha_dx) / 64;
	sumT1 += g_alpha_sum_abs_dy * g_alpha_sum_abs_dy * (64 * curveSumAbsDY * keySumAbsDY / g_alpha_dy / g_alpha_dy) / 64;

	sumT2 *= curveGetMass(pCurve);
	sumT2 -= curveSumX * keySumY;
	sumT2 += curveSumY * keySumX;
	sumT2 -= curveSumDX * keySumDY;
	sumT2 += curveSumDY * keySumDX;
	sumT2 += g_alpha_sum_dx * g_alpha_sum_dy * (64 * curveSumDX * keySumDY / g_alpha_dx / g_alpha_dy) / 64;
	sumT2 -= g_alpha_sum_dy * g_alpha_sum_dx * (64 * curveSumDY * keySumDX / g_alpha_dy / g_alpha_dx) / 64;
	sumT2 += g_alpha_sum_abs_dx * g_alpha_sum_abs_dy * (64 * curveSumAbsDX * keySumAbsDY / g_alpha_dx / g_alpha_dy) / 64;
	sumT2 -= g_alpha_sum_abs_dy * g_alpha_sum_abs_dx * (64 * curveSumAbsDY * keySumAbsDX / g_alpha_dy / g_alpha_dx) / 64;

	qNorm = keyMass * (qSum + dqSum)
		- keySumX * keySumX - keySumY * keySumY;
#ifdef DO_LOGGING_FINESEARCH
	logVariable32u("qN1",qNorm);
#endif
	qNorm -= keySumDX * keySumDX;
	qNorm -= keySumDY * keySumDY;
	qNorm += g_alpha_sum_dx * g_alpha_sum_dx * (64 * keySumDX * keySumDX / g_alpha_dx / g_alpha_dx) / 64;
	qNorm += g_alpha_sum_dy * g_alpha_sum_dy * (64 * keySumDY * keySumDY / g_alpha_dy / g_alpha_dy) / 64;
	qNorm += g_alpha_sum_abs_dx * g_alpha_sum_abs_dx * (64 * keySumAbsDX * keySumAbsDX / g_alpha_dx / g_alpha_dx) / 64;
	qNorm += g_alpha_sum_abs_dy * g_alpha_sum_abs_dy * (64 * keySumAbsDY * keySumAbsDY / g_alpha_dy / g_alpha_dy) / 64;

	curveNorm = curveNorm * curveGetMass(pCurve)
		- curveSumX * curveSumX - curveSumY * curveSumY;
	curveNorm -= curveSumDX * curveSumDX;
	curveNorm -= curveSumDY * curveSumDY;
	curveNorm += g_alpha_sum_dx * g_alpha_sum_dx * (64 * curveSumDX * curveSumDX / g_alpha_dx / g_alpha_dx) / 64;
	curveNorm += g_alpha_sum_dy * g_alpha_sum_dy * (64 * curveSumDY * curveSumDY / g_alpha_dy / g_alpha_dy) / 64;
	curveNorm += g_alpha_sum_abs_dx * g_alpha_sum_abs_dx * (64 * curveSumAbsDX * curveSumAbsDX / g_alpha_dx / g_alpha_dx) / 64;
	curveNorm += g_alpha_sum_abs_dy * g_alpha_sum_abs_dy * (64 * curveSumAbsDY * curveSumAbsDY / g_alpha_dy / g_alpha_dy) / 64;

#ifdef DO_LOGGING_FINESEARCH
	logVariable32("ST1",sumT1);
	logVariable32("ST2",sumT2);
	logVariable32u("sSN",sumSjNorm);
	logVariable32u("cuN",curveNorm);
	logVariable32u("kyN",qNorm);
#endif

	if(pMuOut != NULL)
		proxStoreMu(pMuOut, sumT1, sumT2, sumSjNorm, curveNorm, qNorm);

	if(pSimTransfOut != NULL)
		proxStoreTransformation(pSimTransfOut, pCurve, sumT1, sumT2, sumSjNorm, qNorm, curveNorm,
			pCurve->mass, keySumX, keySumY, curveSumX, curveSumY);

#ifdef _DEBUG_PROXIMITY
	{
		DECUMA_INT16 muDbg;
		SIM_TRANSF simTrDbg;

		proxCurveKeyDebug(&muDbg,&simTrDbg,pCurve, pKid, 0,-1,-1,-1,g_alpha,arcOrder);
		decumaAssert(!pMuOut || decumaAbs(muDbg-*pMuOut)<=1 );
		decumaAssert(!pSimTransfOut || simtransfAlmostEqual(&simTrDbg,pSimTransfOut));
	}
#endif
} /* proxCurveKey() */

void proxCurveKeyWithoutPreCalculatedKey(DECUMA_INT16 * pMuOut, SIM_TRANSF * pSimTransfOut, SCR_CURVE *pCurve, const KID * pKid,
										 int measure_id, const DECUMA_INT8 * arcOrder)

{
	/*Computes the similarity proximity measure, */
	/*which is a value between 0 and maxProxValue. */

	DECUMA_INT32 sumT1 = 0, sumT2 = 0;/*,large; */
	DECUMA_INT32 keyNorm = 0;
	DECUMA_UINT32 sumSjNorm = 0;
	DECUMA_UINT32 curveNorm = 0;
	DECUMA_INT32 curveSumX = 0;
	DECUMA_INT32 curveSumY = 0;
	DECUMA_INT32 keySumX = 0, keySumY = 0;
	int a;
	int i,p,step,noIterations;
	SCR_ARC * pArc;
	DECUMA_INT8_DB_PTR keyArc;
	DECUMA_INT16 * pDX, * pDY, * pX, * pY;
	DECUMA_INT32 keyDX, keyDY;
	DECUMA_INT32 keyX, keyY;
	DECUMA_UINT8 arcRotationMask = keyGetArcRotationMask(pKid);

	const int curveTotalLength = curveGetTotalLength(pCurve);

	int j = 0;

	/* If this asserts, a wrong function have been used */
	/* to precalculate the curve */
	decumaAssert(pCurve->measure_id != -1);

	for(a = 0 ; a < curveGetNoArcs(pCurve) ; a++)
	{
		DECUMA_INT32 arcScProd1 = 0;
		DECUMA_INT32 arcScProd2 = 0;
		DECUMA_INT32 keyArcSumX = 0;
		DECUMA_INT32 keyArcSumY = 0;
		int arcNr =  getArcNr(arcOrder,a);
		DECUMA_INT32 arcMass = 0;

		int alpha_x = pCurve->Arcs[arcNr]->alpha_x;
		int alpha_y = pCurve->Arcs[arcNr]->alpha_y;
		int alpha_dx = pCurve->Arcs[arcNr]->alpha_dx;
		int alpha_dy = pCurve->Arcs[arcNr]->alpha_dy;

		pArc = pCurve->Arcs[ arcNr ];
		keyArc = kidGetArcPointer(pKid, a);
		noIterations = curveGetArcLength(pCurve, arcNr);
		if (arcIsReverse(arcOrder,a))
		{
			p=noIterations-1;
			step=-1;
		}
		else
		{
			p=0;
			step=1;
		}
		pX = &(pArc->x[p]);
		pY = &(pArc->y[p]);
		pDX = &(pArc->dx[p]);
		pDY = &(pArc->dy[p]);
		for(i = 0 ; i < noIterations ; i++, p+=step ,pX+=step, pY+=step, pDX+=step, pDY+=step, j++)
		{
			keyX = alpha_x * keyarcGetX(keyArc,i);
			keyY = alpha_y * keyarcGetY(keyArc,i);

			if( GetMeasureIncExcl(measure_id, curveTotalLength ,j) == 0)
			{
				continue;
			}
			arcMass++;

			if(i+1 == noIterations )
			{
				/* We have to look at the previous element first instead. */
				/* ( We could also have kept keyDX and keyDY from the last iteration, but */
				/* we cannot be sure when that was since we sometimes shortcuts the loop */
				/* by a 'continue' */
				keyDX = alpha_dx * (keyarcGetX(keyArc,i) - keyarcGetX(keyArc,i-1));
				keyDY = alpha_dy * (keyarcGetY(keyArc,i) - keyarcGetY(keyArc,i-1));
			} else {
				keyDX = alpha_dx * (keyarcGetX(keyArc,i+1) - keyarcGetX(keyArc,i));
				keyDY = alpha_dy * (keyarcGetY(keyArc,i+1) - keyarcGetY(keyArc,i));
			}

			keyX += alpha_x * keyGetArcOffsetX(pKid, a);/* Add offset if this arc is part of the diacKey. */
			keyY += alpha_y * keyGetArcOffsetY(pKid, a);/* Add offset if this arc is part of the diacKey. */

			arcScProd1 += keyDX * alpha_dx * (*pDX*step) / 64;
			arcScProd1 += keyDY * alpha_dy * (*pDY*step) / 64;

			arcScProd2 -= keyDX * alpha_dy * (*pDY*step) / 64;
			arcScProd2 += keyDY * alpha_dx * (*pDX*step) / 64;

			arcScProd1 += keyX * alpha_x * (*pX) / 64;
			arcScProd1 += keyY * alpha_y * (*pY) / 64;

			arcScProd2 -= keyX * alpha_y * (*pY) / 64;
			arcScProd2 += keyY * alpha_x * (*pX) / 64;

			keyNorm += (keyX * keyX + keyY * keyY) / 64;
			keyNorm += (keyDX * keyDX + keyDY * keyDY) / 64;

			keyArcSumX += keyX;
			keyArcSumY += keyY;
		}

		keySumX += keyArcSumX / 8;
		keySumY += keyArcSumY / 8;

		if ( keyArcCanBeRotated( a, arcRotationMask) &&
			 arcMass != 0 ) /* No need to rotate the arc if it's not used */
		{
			DECUMA_INT32 a1,a2;
			DECUMA_INT32 sj1, sj2;
			DECUMA_UINT32 sjNorm;

			/* No need for overflow check here */
			a1 = (pArc->sum_x*keyArcSumX + pArc->sum_y*keyArcSumY) / 8;
			a2 = (pArc->sum_x*keyArcSumY - pArc->sum_y*keyArcSumX) / 8;

			sj1 = arcMass * arcScProd1 - a1; /* (sj1,sj2) = sj(Phi,Psi) ... (here multiplied with arc mass) */
			sj2 = arcMass * arcScProd2 - a2;

			decumaAssert( arcMass );
			sjNorm = curveGetMass(pCurve) * normFine(sj1,sj2) / arcMass ;

			decumaAssert( sumSjNorm <= sumSjNorm + sjNorm);
			sumSjNorm += sjNorm;

			sumT1 += a1 / arcMass; /*(sumT1,sumT2) = t(Phi,Psi) */
			sumT2 += a2 / arcMass;
		}
		else
		{
			sumT1 += arcScProd1;
			sumT2 += arcScProd2;
		}

		decumaAssert(curveNorm <= curveNorm + (alpha_x * alpha_x * arcGetSumX2(pArc) +
			alpha_y * alpha_y * arcGetSumY2(pArc) +
			alpha_dx * alpha_dx * arcGetSumDX2(pArc) +
			alpha_dy * alpha_dy * arcGetSumDY2(pArc)) / 64);

		curveNorm += (
			alpha_x * alpha_x * arcGetSumX2(pArc) +
			alpha_y * alpha_y * arcGetSumY2(pArc) +
			alpha_dx * alpha_dx * arcGetSumDX2(pArc) +
			alpha_dy * alpha_dy * arcGetSumDY2(pArc)) / 64;

		curveSumX += alpha_x * arcGetSumX(pArc) / 8;
		curveSumY += alpha_y * arcGetSumY(pArc) / 8;
	}

	decumaAssert(keyNorm < keyNorm * curveGetMass(pCurve) );
	decumaAssert( decumaAbs(sumT1) <= MAX_DECUMA_INT32 / MAX_NUMBER_OF_ARCS_IN_CURVE / NUMBER_OF_POINTS_IN_ARC); /*Overflow check */
	decumaAssert( decumaAbs(sumT2) <= MAX_DECUMA_INT32 / MAX_NUMBER_OF_ARCS_IN_CURVE / NUMBER_OF_POINTS_IN_ARC); /*Overflow check */

	decumaAssert( keySumX * keySumX >= keySumX);
	decumaAssert( keySumY * keySumY >= keySumY);

	keyNorm = keyNorm * curveGetMass(pCurve)
		- keySumX * keySumX - keySumY * keySumY;

	curveNorm = curveNorm * curveGetMass(pCurve)
		- curveSumX * curveSumX - curveSumY * curveSumY;

	sumT1 *= curveGetMass(pCurve);
	sumT1 -= curveSumX * keySumX;
	sumT1 -= curveSumY * keySumY;

	sumT2 *= curveGetMass(pCurve);
	sumT2 -= curveSumX * keySumY;
	sumT2 += curveSumY * keySumX;

	if(pMuOut != NULL)
		proxStoreMu(pMuOut, sumT1, sumT2, sumSjNorm, curveNorm, keyNorm);
	if(pSimTransfOut != NULL)
		proxStoreTransformation(pSimTransfOut, pCurve, sumT1, sumT2, sumSjNorm, keyNorm, curveNorm,
			pCurve->mass, keySumX, keySumY, curveSumX, curveSumY);

#ifdef _DEBUG_PROXIMITY
	{
		DECUMA_INT16 muDbg;
		SIM_TRANSF simTrDbg;

		proxCurveKeyDebug(&muDbg,&simTrDbg,pCurve, pKid, measure_id,-1,-1,-1,alpha,arcOrder);
		decumaAssert(!pMuOut || decumaAbs(muDbg-*pMuOut)<=1 );
		decumaAssert(!pSimTransfOut || simtransfAlmostEqual(&simTrDbg,pSimTransfOut));
	}
#endif
} /* proxCurveKeyWithoutPreCalculatedKey() */

#ifdef _DEBUG_PROXIMITY
static void proxCurveKeyDebug(DECUMA_INT16 * pMuOut, SIM_TRANSF * pSimTransfOut, const SCR_CURVE *pCurve, const KID * pKid,
	int measure_id, int onlyArc, int startIdx, int stopIdx, DECUMA_UINT8 alpha, const DECUMA_INT8 * arcOrder)
{
	/*Computes the similarity proximity measure, without any precalculations. */

	/*Parameters: See header file */

	/*If onlyArc is set to positive value we will only measure the */
	/*arc with that index */

	/*Computes the similarity proximity measure, */
	/*which is a value between 0 and maxProxValue. */

	DECUMA_INT32 sumT1 = 0, sumT2 = 0;/*,large; */
	DECUMA_INT32 keyNorm = 0;
	DECUMA_INT32 curveNorm = 0;
	DECUMA_UINT32 sumSjNorm = 0;
	DECUMA_INT32 keySumX = 0, keySumY = 0;
	DECUMA_INT32 curveSumX = 0, curveSumY = 0;
	int a;
	int i,p,step,noIterations;
	SCR_ARC * pArc;
	DECUMA_INT8_DB_PTR keyArc;
	DECUMA_INT16 * pX, * pY;
	DECUMA_INT16 curveDX, curveDY;
	DECUMA_INT16 curveX, curveY;
	DECUMA_INT8 keyDX, keyDY;
	DECUMA_INT8 keyX, keyY;
	DECUMA_UINT8 arcRotationMask = keyGetArcRotationMask(pKid);
	DECUMA_INT32 curveMass=0;


	const int curveTotalLength = curveGetTotalLength(pCurve);

	int j = 0;

	/*First calculate curveMass it needs to be precalculated */
	for(a = 0 ; a < curveGetNoArcs(pCurve) ; a++)
	{
		if (onlyArc > 0 && a!=onlyArc)
		{
			/*If onlyArc is set to positive value we will only measure the */
			/*arc with that index */
			continue;
		}
		for(i = 0 ; i < curveGetArcLength(pCurve, a); i++, j++)
		{
			if((startIdx>=0 && i < startIdx) || (stopIdx>=0 && i > stopIdx))
				continue;

			if( GetMeasureIncExcl(measure_id, curveTotalLength ,j) == 0)
			{
				continue;
			}
			curveMass++;
		}
	}

	j = 0;
	for(a = 0 ; a < curveGetNoArcs(pCurve) ; a++)
	{
		DECUMA_INT32 arcScProd1 = 0;
		DECUMA_INT32 arcScProd2 = 0;
		DECUMA_INT32 keyArcSumX = 0;
		DECUMA_INT32 keyArcSumY = 0;
		DECUMA_INT32 curveArcSumX = 0;
		DECUMA_INT32 curveArcSumY = 0;
		int arcNr =  getArcNr(arcOrder,a);
		int arcMass = 0;

		if (onlyArc > 0 && a!=onlyArc)
		{
			/*If onlyArc is set to positive value we will only measure the */
			/*arc with that index */
			continue;
		}

		pArc = pCurve->Arcs[ arcNr ];
		keyArc = kidGetArcPointer(pKid, a);
		noIterations = curveGetArcLength(pCurve, arcNr);
		if (arcIsReverse(arcOrder,a))
		{
			p=noIterations-1;
			step=-1;
		}
		else
		{
			p=0;
			step=1;
		}
		pX = &(pArc->x[p]);
		pY = &(pArc->y[p]);
		for(i = 0 ; i < noIterations ; i++, p+=step ,pX+=step, pY+=step, j++)
		{
			if((startIdx>=0 && i < startIdx) || (stopIdx>=0 && i > stopIdx))
				continue;

			if( GetMeasureIncExcl(measure_id, curveTotalLength ,j) == 0)
			{
				continue;
			}
			arcMass++;

			keyX = keyarcGetX(keyArc,i);
			keyY = keyarcGetY(keyArc,i);

			curveX=*pX;
			curveY=*pY;

			if(p+1 == noIterations)
			{
				/* We have to look at the previous element first instead. */
				curveDY = curveY - *(pY-1);
				curveDX = curveX - *(pX-1);
			} else {
				curveDY = *(pY+1) - curveY;
				curveDX = *(pX+1) - curveX;
			}
			/*JM: Note that we are deliberately using "i" here instead of "p".  */
			/*    This is done to use the same algorithm as when having precalculated */
			/*    derivatives for the key where the derivative is not symmetric to arc-reversal, */
			/*    since the derivative in the two ends are calculated differently and the  */
			/*    method in the prox-functions is only to use negative derivative when arc is reversed. */
			if(i+1 == noIterations)
			{
				/* We have to look at the previous element first instead. */
				/* ( We could also have kept keyDX and keyDY from the last iteration, but */
				/* we cannot be sure when that was since we sometimes shortcuts the loop */
				/* by a 'continue' */
				keyDX = keyX - keyarcGetX( keyArc,i-1 );
				keyDY = keyY - keyarcGetY( keyArc,i-1 );
			} else {
				keyDX = keyarcGetX( keyArc,i+1 ) - keyX;
				keyDY = keyarcGetY( keyArc,i+1 ) - keyY;
			}

			keyX += keyGetArcOffsetX(pKid, a);/* Add offset if this arc is part of the diacKey. */
			keyY += keyGetArcOffsetY(pKid, a);/* Add offset if this arc is part of the diacKey. */

			arcScProd1 += keyDX * (curveDX*step)*alpha;
			arcScProd1 += keyDY * (curveDY*step)*alpha;

			arcScProd2 -= keyDX * (curveDY*step)*alpha;
			arcScProd2 += keyDY * (curveDX*step)*alpha;

			arcScProd1 += keyX * (*pX);
			arcScProd1 += keyY * (*pY);

			arcScProd2 -= keyX * (*pY);
			arcScProd2 += keyY * (*pX);

			keyNorm += (keyX * keyX + keyY * keyY + (keyDX * keyDX + keyDY * keyDY)*alpha);
			curveNorm += (curveX * curveX + curveY * curveY + (curveDX * curveDX + curveDY * curveDY)*alpha);

			keyArcSumX += keyX;
			keyArcSumY += keyY;

			curveArcSumX += curveX;
			curveArcSumY += curveY;
		}

		curveSumX += curveArcSumX;
		curveSumY += curveArcSumY;

		keySumX += keyArcSumX;
		keySumY += keyArcSumY;

		if ( keyArcCanBeRotated( a, arcRotationMask) &&
			 arcMass != 0 ) /* No need to rotate the arc if it's not used */
		{
			DECUMA_INT32 a1,a2;
			DECUMA_INT32 sj1, sj2;
			DECUMA_UINT32 sjNorm;

			/* No need for overflow check here */
			a1 = curveArcSumX*keyArcSumX + curveArcSumY*keyArcSumY;
			a2 = curveArcSumX*keyArcSumY - curveArcSumY*keyArcSumX;

			sj1 = arcMass * arcScProd1 - a1; /* (sj1,sj2) = sj(Phi,Psi) ... (here multiplied with arc mass) */
			sj2 = arcMass * arcScProd2 - a2;

			decumaAssert( arcMass );
			sjNorm = curveMass * normFine(sj1,sj2) / arcMass ;

			decumaAssert( sumSjNorm <= sumSjNorm + sjNorm);
			sumSjNorm += sjNorm;

			sumT1 += a1 / arcMass; /*(sumT1,sumT2) = t(Phi,Psi) */
			sumT2 += a2 / arcMass;
		}
		else
		{
			sumT1 += arcScProd1;
			sumT2 += arcScProd2;
		}
	}
	decumaAssert(keyNorm < keyNorm * curveMass );
	decumaAssert( keySumX * keySumX >= keySumX);
	decumaAssert( keySumY * keySumY >= keySumY);

	keyNorm = keyNorm * curveMass
		- keySumX * keySumX - keySumY * keySumY;

	decumaAssert(curveNorm < curveNorm * curveMass );
	decumaAssert( curveSumX * curveSumX >= curveSumX);
	decumaAssert( curveSumY * curveSumY >= curveSumY);

	curveNorm = curveNorm * curveMass
		- curveSumX * curveSumX - curveSumY * curveSumY;


	decumaAssert( decumaAbs(sumT1) <= MAX_DECUMA_INT32 / MAX_NUMBER_OF_ARCS_IN_CURVE / NUMBER_OF_POINTS_IN_ARC); /*Overflow check */
	decumaAssert( decumaAbs(sumT2) <= MAX_DECUMA_INT32 / MAX_NUMBER_OF_ARCS_IN_CURVE / NUMBER_OF_POINTS_IN_ARC); /*Overflow check */

	sumT1 *= curveMass;
	sumT1 -= curveGetSumX(pCurve) * keySumX;
	sumT1 -= curveGetSumY(pCurve) * keySumY;

	sumT2 *= curveMass;
	sumT2 -= curveGetSumX(pCurve) * keySumY;
	sumT2 += curveGetSumY(pCurve) * keySumX;


	if(pMuOut != 0)
		proxStoreMu(pMuOut, sumT1, sumT2, sumSjNorm, (float) curveNorm, keyNorm);

	if(pSimTransfOut != 0)
	{
		proxStoreTransformation(pSimTransfOut, pCurve, sumT1, sumT2, sumSjNorm, keyNorm, (float) curveNorm,
			curveMass, keySumX, keySumY);
	}
} /* proxCurveKeyDebug() */
#endif /*_DEBUG_PROXIMITY */

void proxCurveKeyNothingPreCalculated(DECUMA_INT16 * pMuOut, SIM_TRANSF * pSimTransfOut, const SCR_CURVE *pCurve, const KID * pKid,
										 int start, int stop, int arcToZoomK, const DECUMA_INT8 * arcOrder)
{
	/*Computes the similarity proximity measure on ONE arc. */
	/*which is a value between 0 and maxProxValue. */
	/*NOTE: arcToZoomK is KEYindexed */
	/*NOTE: start and stop are KEYindexed numbers (i.e. they denote indexes corresponding to arcOrder of Key) */

	DECUMA_INT32 sum1 = 0, sum2 = 0;/*,large; */
	DECUMA_INT32 keyNorm = 0, curveNorm = 0, mass = 0;
	DECUMA_INT32 keySumX = 0, keySumY = 0, curveSumX = 0, curveSumY = 0;
	DECUMA_INT32 p,step,noIterations,i;
	SCR_ARC * pArc;
	DECUMA_INT8_DB_PTR keyArc;
	DECUMA_INT16  * pX, * pY;
	DECUMA_INT32 keyDX, keyDY;
	DECUMA_INT32 keyX, keyY;
	DECUMA_INT32 curveDX, curveDY;
	int arcNr;

	int j = 0;

	arcNr = getArcNr(arcOrder,arcToZoomK);
	pArc = pCurve->Arcs[ arcNr ];
	keyArc = kidGetArcPointer( pKid, arcToZoomK);
	noIterations = curveGetArcLength(pCurve, arcNr );
	if (arcIsReverse(arcOrder,arcToZoomK))
	{
		p=noIterations-1;
		step=-1;
	}
	else
	{
		p=0;
		step=1;
	}
	pX = &(pArc->x[p]);
	pY = &(pArc->y[p]);
	for(i = 0 ; i < noIterations; i++, pX+=step, pY+=step, j++)
	{
		const DECUMA_INT32 curveX = pCurve->Arcs[arcNr]->alpha_x * *pX;
		const DECUMA_INT32 curveY = pCurve->Arcs[arcNr]->alpha_y * *pY;
		keyX = pCurve->Arcs[arcNr]->alpha_x * keyarcGetX( keyArc,i );
		keyY = pCurve->Arcs[arcNr]->alpha_y * keyarcGetY( keyArc,i );

		if((i < start) || (i > stop)) /* start and stop are keyIndexed */
			continue;

		if(i+1 == noIterations)
		{
			/* We have to look at the previous element first instead. */
			/* ( We could also have kept keyDX and keyDY from the last iteration, but */
			/* we cannot be sure when that was since we sometimes shortcuts the loop */
			/* by a 'continue' */
			keyDX = pCurve->Arcs[arcNr]->alpha_dx * (keyarcGetX( keyArc,i ) - keyarcGetX( keyArc,i-1 ));
			keyDY = pCurve->Arcs[arcNr]->alpha_dy * (keyarcGetY( keyArc,i ) - keyarcGetY( keyArc,i-1 ));
			curveDY = pCurve->Arcs[arcNr]->alpha_dy * ((DECUMA_INT32)*pY - *(pY-1*step));
			curveDX = pCurve->Arcs[arcNr]->alpha_dx * ((DECUMA_INT32)*pX - *(pX-1*step));
		} else {
			keyDX = pCurve->Arcs[arcNr]->alpha_dx * (keyarcGetX( keyArc,i+1 ) - keyarcGetX( keyArc,i ));
			keyDY = pCurve->Arcs[arcNr]->alpha_dy * (keyarcGetY( keyArc,i+1 ) - keyarcGetY( keyArc,i ));
			curveDY = pCurve->Arcs[arcNr]->alpha_dy * (*(pY+1*step) - (DECUMA_INT32)*pY);
			curveDX = pCurve->Arcs[arcNr]->alpha_dx * (*(pX+1*step) - (DECUMA_INT32)*pX);
		}

		keyX += pCurve->Arcs[arcNr]->alpha_x * keyGetArcOffsetX(pKid, arcToZoomK);/* Add offset if this arc is part of the diacKey. */
		keyY += pCurve->Arcs[arcNr]->alpha_y * keyGetArcOffsetY(pKid, arcToZoomK);/* Add offset if this arc is part of the diacKey. */


		sum1 += (keyDX * curveDX + keyDY * curveDY) / 64;
		sum1 += (keyX * curveX + keyY * curveY) / 64;

		sum2 += (keyDY * curveDX - keyDX * curveDY) / 64;
		sum2 += (keyY * curveX - keyX * curveY) / 64;

		keyNorm += (keyX * keyX + keyY * keyY + keyDX * keyDX + keyDY * keyDY) / 64;
		curveNorm += (curveX * curveX + curveY * curveY + curveDX * curveDX + curveDY * curveDY) / 64;

		keySumX += keyX / 8;
		keySumY += keyY / 8;
		curveSumY += curveY / 8;
		curveSumX += curveX / 8;
		mass++;
	}


	decumaAssert(keyNorm < keyNorm * mass );

	decumaAssert( keySumX * keySumX >= keySumX);
	decumaAssert( keySumY * keySumY >= keySumY);

	keyNorm = keyNorm * mass
		- keySumX * keySumX - keySumY * keySumY;

	curveNorm = curveNorm * mass
		- curveSumX * curveSumX - curveSumY * curveSumY;

	sum1 *= mass;
	sum1 -= curveSumX * keySumX;
	sum1 -= curveSumY * keySumY;

	sum2 *= mass;
	sum2 -= curveSumX * keySumY;
	sum2 += curveSumY * keySumX;

	if(pMuOut != NULL)
		proxStoreMu(pMuOut, sum1, sum2, 0,(float)curveNorm, keyNorm);
	if(pSimTransfOut != NULL)
		proxStoreTransformation(pSimTransfOut, pCurve, sum1, sum2, 0, keyNorm, (float)curveNorm,
		mass, keySumX, keySumY, curveSumX, curveSumY);

#ifdef _DEBUG_PROXIMITY
	{
		DECUMA_INT16 muDbg;
		SIM_TRANSF simTrDbg;

		proxCurveKeyDebug(&muDbg,&simTrDbg,pCurve, pKid, 0,arcToZoomK,start,stop,alpha,arcOrder);
		decumaAssert(!pMuOut || decumaAbs(muDbg-*pMuOut)<=1 );
		decumaAssert(!pSimTransfOut || simtransfAlmostEqual(&simTrDbg,pSimTransfOut));
	}
#endif
} /* proxCurveKeyNothingPreCalculated() */

static void proxStoreMu(DECUMA_INT16 * pMuOut, DECUMA_INT32 sum1, DECUMA_INT32 sum2, DECUMA_INT32 sumSjNorm,
						float curveNorm, DECUMA_UINT32 qNorm)
{
	/*Computations below unfortunately involves float numbers. */

	float s1float,nfloat;

	sum1 = normFine(sum1,sum2);
	sum1 += sumSjNorm;
	s1float = (float) sum1 * (float) sum1;

	nfloat = (float) curveNorm;
	nfloat *= (float) qNorm;

	if(nfloat == 0.0f)
	{
		/* This occures if the arc is represents a single dot. */
		/* In most cases, this will be prevented by a proxDot */
 		/* check in scr2Select. But if a dot is not a part of */
		/* the category, the arc will reach this code. */
		*pMuOut = (DECUMA_INT16) maxProxValue; /* This is a dot, it looks like everything... */
	}
	else
	{
		*pMuOut = (DECUMA_INT16) SquareRootVerySpecial(s1float/nfloat,maxProxValue);
	}
	return;
}

DECUMA_INT16 getProxMuApproximation(DECUMA_INT32 sum1, DECUMA_INT32 sum2, DECUMA_UINT32 sumSjNorm, float curveNorm, DECUMA_UINT32 qNorm)
{
	/* This is a faster version of proxStoreMu without floiting point numbers. */
	/* Returns maxProxValue * ( 1 - (sqrt( sum1^2 + sum2^2) + sumSjNorm )^2 / ( curveNorm * qNorm ) ). */
	/* NOTE: This function does not take the square root of the value as the finer version of the */
	/*       Proximity measure does.  */
	int nRight = 0;
	int k;

	DECUMA_UINT32 s1, s2, n1, temp;
	DECUMA_UINT32 n2 = qNorm;

	while ( curveNorm >= (float) MAX_DECUMA_UINT32 )
	{
		curveNorm /= 4.0f;
		nRight++;
	}
	decumaAssert( curveNorm <= (float) MAX_DECUMA_UINT32 );
	n1 = (DECUMA_UINT32) curveNorm;
	s1 = decumaAbs( sum1 ) >> nRight;
	s2 = decumaAbs( sum2 ) >> nRight;
	sumSjNorm >>= nRight;
	temp = maxNUMB( s1, s2);

	nRight = 0;
	while ( temp > MAX_INT32_SQRT) /* while (temp^2) will result in overflow! */
	{
		/* Bit shift temp until it is possible to calculate s1*s1 + s2*s2 */
		/* without overflow. */
		temp >>= 2;
		nRight += 2;
	}

	/* Now bit shift the denominators totally 2*nRight bits. */
	/* The largest of n1 and n2 is shifted to avoid loosing precision. */
	for ( k=0; k < nRight; k++ )
	{
		if ( n1 < n2 )
			n2 >>= 2;
		else
			n1 >>= 2;
	}

	while ( maxNUMB( n1, n2) > MAX_INT32_SQRT)
	{
		/* Bit shift the largest of n1 and n2 until it is possible to */
		/* calculate max(n1,n2) * max(n1,n2) without overflow. */
		if ( n1 > n2 )
			n1 >>= 2;
		else
			n2 >>= 2;

		nRight++;
	}

	/* Finally bit shift the nominators nRight bits each. */
	s1 >>= nRight;
	s2 >>= nRight;

	sumSjNorm >>= nRight;

	s1 = s1*s1 + s2*s2;

	n1 = n1 * n2;

	if (sumSjNorm > 0)
	{
		s1 = decumaSqrt(s1); /*NOTE: Takes time!!! */

		/*sumSjNorm + s1 should not cause overflow (very unlikely to happen) */
		while ( sumSjNorm > sumSjNorm + s1 )
		{
			sumSjNorm >>=1;
			s1>>=1;
			n1>>=2;
		}

		s1 += sumSjNorm;

		while ( s1 > MAX_DECUMA_UINT16 ) /*sqrt(MAX_UINT32) */
		{
			s1>>=1;
			n1>>=2;
		}

		s1 *= s1;
	}

	/* Shift the nominator and the denominator to make it possible to */
	/* calculate maxProxValue * s1 without overflow. */
	decumaAssert( maxProxValue < 1<<10 );
	while ( s1 > MAX_DECUMA_UINT32>>10 ) /* Requires that maxProxValue is smaller than 1<<10. */
	{
		/* Shift 3 bits each time to speed up the loop. */
		s1 >>= 3;
		n1 >>= 3;
	}

	if ( n1 != 0)
	{
		if( s1 >= n1 )
	    {
			decumaAssert( s1 * (maxProxValue-1) / maxProxValue <= n1 );
			/* (maxProxValue-1) / maxProxValue is just to avoid assert errors that */
			/* comes from rounding errors. The assert above should be: */
			/* decumaAssert( s1 <= n1 ); */
			return 0;
	    }
		else
		{
			return (DECUMA_INT16) (maxProxValue - ((DECUMA_UINT32) maxProxValue * s1 )/ n1);

		}
	}
	else
		return (DECUMA_INT16) maxProxValue;
} /*getProxMuApproximation */

void proxStoreTransformation(SIM_TRANSF * pTargetTrans, const SCR_CURVE * pCurve,
		DECUMA_INT32 isum1, DECUMA_INT32 isum2, DECUMA_INT32 sumSjNorm,
		DECUMA_INT32 keyNorm, float curveNorm,
							 int mass, DECUMA_INT32 key_sum_x, DECUMA_INT32 key_sum_y,
							 DECUMA_INT32 curve_sum_x, DECUMA_INT32 curve_sum_y)
{
	/*This function computes the similarty transformation. */
	/*Note that we are using floats - not good. */

	const long sumX = curve_sum_x;
	const long sumY = curve_sum_y;
	DECUMA_UINT32 scale;


	pTargetTrans->symPoint.x = (DECUMA_INT32) ((sumX*SIM_TRANSF_ROUNDING_FACTOR) / mass);
	pTargetTrans->symPoint.y = (DECUMA_INT32) ((sumY*SIM_TRANSF_ROUNDING_FACTOR) / mass);

	pTargetTrans->delta.x = (key_sum_x*SIM_TRANSF_ROUNDING_FACTOR) / mass;
	pTargetTrans->delta.y = (key_sum_y*SIM_TRANSF_ROUNDING_FACTOR) / mass;

	proxStoreAngle(&(pTargetTrans->theta),isum1,isum2);

	if (sumSjNorm > 0)
	{
		/*TODO: Could this be done in another way? */
		scale = decumaScaleFloat((float)keyNorm,(float) curveNorm, SIM_TRANSF_SCALE_FACTOR);
	}
	else
	{
		float fsum1 = (float)isum1;
		float fsum2 = (float)isum2;

		scale = (DECUMA_UINT32) decumaScaleFloat( (float) (fsum1 *fsum1 + fsum2*fsum2),
			(float) (curveNorm * curveNorm), SIM_TRANSF_SCALE_FACTOR);
	}

	scale = minNUMB(scale,MAX_SIM_TRANSF_SCALE);
	scale = maxNUMB(scale,MIN_SIM_TRANSF_SCALE);

	pTargetTrans->scale = scale;

	return;
}

#define HELPER(pk , pc )		\
				((DECUMA_INT32)pk[0]*(DECUMA_INT32)pc[0] +   \
				(DECUMA_INT32)pk[4]* (DECUMA_INT32)pc[4] +   \
				(DECUMA_INT32)pk[8]* (DECUMA_INT32)pc[8] +   \
				(DECUMA_INT32)pk[13]*(DECUMA_INT32)pc[13] + \
				(DECUMA_INT32)pk[18]*(DECUMA_INT32)pc[18] + \
				(DECUMA_INT32)pk[23]*(DECUMA_INT32)pc[23] + \
				(DECUMA_INT32)pk[27]*(DECUMA_INT32)pc[27] + \
				(DECUMA_INT32)pk[31]*(DECUMA_INT32)pc[31])
/* HELPER2 is for arcs that should be traversed BACKWARD_32s */
#define HELPER2(pk , pc )		\
				((DECUMA_INT32)pk[0]*(DECUMA_INT32)pc[31] +   \
				(DECUMA_INT32)pk[4]* (DECUMA_INT32)pc[27] +   \
				(DECUMA_INT32)pk[8]* (DECUMA_INT32)pc[23] +   \
				(DECUMA_INT32)pk[13]*(DECUMA_INT32)pc[18] + \
				(DECUMA_INT32)pk[18]*(DECUMA_INT32)pc[13] + \
				(DECUMA_INT32)pk[23]*(DECUMA_INT32)pc[8] + \
				(DECUMA_INT32)pk[27]*(DECUMA_INT32)pc[4] + \
				(DECUMA_INT32)pk[31]*(DECUMA_INT32)pc[0])

#define HELPER_SUM( pc )  ( (DECUMA_INT32)pc[0] + (DECUMA_INT32)pc[4] + (DECUMA_INT32)pc[8] + (DECUMA_INT32)pc[13] + (DECUMA_INT32)pc[18] + (DECUMA_INT32)pc[23] + (DECUMA_INT32)pc[27] + (DECUMA_INT32)pc[31] )

#define HELPER_MASS (NUMBER_OF_POINTS_IN_INTERLEAVED_ARC)

#define ARC_SUMS_NOT_SUPPORTED_IN_DB

void proxCurveKeyInterleavedNoDerivate(DECUMA_INT16 * pMuOut, int * pTheta, const SCR_CURVE *pCurve,
									   const KID * pKid, const DECUMA_INT8 * arcOrder)
{
	/* */
	/* Other versions of this function was removed from version 1.65 */
	/* */
	/* */

	int a;
	DECUMA_INT8_DB_PTR XSeries;
	DECUMA_INT8_DB_PTR YSeries;
	DECUMA_INT8_DB_PTR keyArc;
	DECUMA_UINT8 arcRotationMask = keyGetArcRotationMask(pKid);

	DECUMA_INT32 curveSumX=0;/*kidGetInterleavedSumX(pKid); */
	DECUMA_INT32 curveSumY=0;/*kidGetInterleavedSumY(pKid); */
	DECUMA_INT32 keySumX=0;/*kidGetInterleavedSumX(pKid); */
	DECUMA_INT32 keySumY=0;/*kidGetInterleavedSumY(pKid); */
	DECUMA_INT32 sumT1 = 0, sumT2 = 0;
	DECUMA_INT32 keyNorm = 0;
	double curveNorm = 0;

	DECUMA_INT32 sumSjNorm = 0;

	for(a = 0 ; a < curveGetNoArcs(pCurve) ; a++)
	{
		DECUMA_INT32 arcScProd1 = 0, arcScProd2 = 0;
		const DECUMA_INT16 * pCurveX;
		const DECUMA_INT16 * pCurveY;
		int arcNr = getArcNr(arcOrder,a);
#if !defined(ARC_SUMS_NOT_SUPPORTED_IN_DB)
		const DECUMA_INT32 alpha_x = pCurve->Arcs[arcNr]->alpha_x;
		const DECUMA_INT32 alpha_y = pCurve->Arcs[arcNr]->alpha_y;
		const DECUMA_INT32 alpha_x2 = alpha_x * alpha_x;
		const DECUMA_INT32 alpha_y2 = alpha_y * alpha_y;
		const DECUMA_INT32 alpha_xy = alpha_x * alpha_y;
#else
		/* For now the database format does not support fast handling of different feature weight in */
		/* the full search. Therefore we here have to set the feature weight to those used in the current */
		/* db precalculations that will be retreived later. This function should be modified to remove the */
		/* ifdefs when it does. /JA */
		const DECUMA_INT32 alpha_x = 8;
		const DECUMA_INT32 alpha_y = 8;
		const DECUMA_INT32 alpha_x2 = 64;
		const DECUMA_INT32 alpha_y2 = 64;
		const DECUMA_INT32 alpha_xy = 64;
#endif
		DECUMA_INT32 keyArcSumX;
		DECUMA_INT32 keyArcSumY;
		DECUMA_INT32 curveArcSumX;
		DECUMA_INT32 curveArcSumY;
		pCurveX = pCurve->Arcs[ arcNr ]->x;
		pCurveY = pCurve->Arcs[ arcNr ]->y;
		keyArc = kidGetArcPointer( pKid, a);
		XSeries= &keyarcGetX(keyArc,0);
		YSeries= &keyarcGetY(keyArc,0);


#if !defined(ARC_SUMS_NOT_SUPPORTED_IN_DB)
		keyArcSumX  = alpha_x * HELPER_SUM( XSeries );
		keyArcSumY  = alpha_y * HELPER_SUM( YSeries );
		curveArcSumX  = alpha_x * HELPER_SUM( pCurveX );
		curveArcSumY  = alpha_y * HELPER_SUM( pCurveY );
#endif

#ifdef DO_LOGGING_FULLSEARCH
		logVariable32u("aNr",arcNr);
		logVariable32u("aRv",arcIsReverse(arcOrder,a));
#endif

		if (arcIsReverse(arcOrder,a) )
		{
			arcScProd1 += alpha_x2 * HELPER2(XSeries, pCurveX) / 64; /*  <Phi|Psi> */
			arcScProd1 += alpha_y2 * HELPER2(YSeries, pCurveY) / 64;
			arcScProd2 += alpha_xy * HELPER2(YSeries, pCurveX) / 64; /* <Phi|Psi~> */
			arcScProd2 -= alpha_xy * HELPER2(XSeries, pCurveY) / 64;
		}
		else
		{
			arcScProd1 += alpha_x2 * HELPER(XSeries, pCurveX) / 64;
			arcScProd1 += alpha_y2 * HELPER(YSeries, pCurveY) / 64;

			arcScProd2 += alpha_xy * HELPER(YSeries, pCurveX) / 64;
			arcScProd2 -= alpha_xy * HELPER(XSeries, pCurveY) / 64;
		}

#ifdef DO_LOGGING_FULLSEARCH
		logVariable32("ky4",YSeries[4]);
		logVariable32("cy4",pCurveY[4]);
/*			logVariable32("HXX",HELPER(XSeries, pCurveX));
 *			logVariable32("HYY",HELPER(YSeries, pCurveY));
 *			logVariable32("HYX",HELPER(YSeries, pCurveX));
 *			logVariable32("HXY",HELPER(XSeries, pCurveY));
 */
#endif

#if !defined(ARC_SUMS_NOT_SUPPORTED_IN_DB)
		curveNorm += alpha_x2 * HELPER(pCurveX, pCurveX) / 64;
		curveNorm += alpha_y2 * HELPER(pCurveY, pCurveY) / 64;
		keyNorm += alpha_x2 * HELPER(XSeries, XSeries) / 64;
		keyNorm += alpha_y2 * HELPER(YSeries, YSeries) / 64;
#endif

		if ( a >= pKid->noBaseArcs ) /* This arc is part of the diacKey. */
		{
			/* This calculation is equal to adding pKid.diacKeyOffset to key */
			arcScProd1 += alpha_x2 * pKid->diacKeyXOffset * HELPER_SUM( pCurveX ) / 64;
			arcScProd1 += alpha_y2 * pKid->diacKeyYOffset * HELPER_SUM( pCurveY ) / 64;
#ifdef DO_LOGGING_FULLSEARCH
			logVariable32("a11",arcScProd1);
			logVariable32("dky",pKid->diacKeyYOffset);
			logVariable32("dkx",pKid->diacKeyXOffset);
#endif

			arcScProd2 -= alpha_xy * pKid->diacKeyXOffset * HELPER_SUM( pCurveY ) / 64;
			arcScProd2 += alpha_xy * pKid->diacKeyYOffset * HELPER_SUM( pCurveX ) / 64;

#if !defined(ARC_SUMS_NOT_SUPPORTED_IN_DB)
			/* sum((alpha * (offset + coord) * alpha * (offset + coord)) = */
			/* alpha^2 * (mass * offset^2 + 2 * offset * sum(coord) + sum(coord^2)) */
			/* alpha^2 * sum(coord^2) was already added above => */
			/* alpha^2 * offset * (mass * offset + sum(coord)) */
			keyNorm += alpha_x2 * pKid->diacKeyXOffset *
				(pKid->diacKeyXOffset * HELPER_MASS +
				 2 * HELPER_SUM( XSeries )) / 64;
			keyNorm += alpha_y2 * pKid->diacKeyYOffset *
				(pKid->diacKeyYOffset * HELPER_MASS +
				 2 * HELPER_SUM( YSeries )) / 64;
#endif

#if !defined(ARC_SUMS_NOT_SUPPORTED_IN_DB)
			keyArcSumX += alpha_x * pKid->diacKeyXOffset*HELPER_MASS;
			keyArcSumY += alpha_y * pKid->diacKeyYOffset*HELPER_MASS;
#endif
		}

		if ( keyArcCanBeRotated( a,arcRotationMask) )
		{
			/* Do we need overflow protection? */

			DECUMA_INT32 a1,a2,temp;
			DECUMA_INT32 sj1, sj2, sjNorm;

			int nRight = 0;

#if defined(ARC_SUMS_NOT_SUPPORTED_IN_DB)
			keyArcSumX  = alpha_x * HELPER_SUM( XSeries );
			keyArcSumY  = alpha_y * HELPER_SUM( YSeries );
			curveArcSumX  = alpha_x * HELPER_SUM( pCurveX );
			curveArcSumY  = alpha_y * HELPER_SUM( pCurveY );
			if ( a >= pKid->noBaseArcs ) /* This arc is part of the diacKey. */
			{
				keyArcSumX += alpha_x * pKid->diacKeyXOffset*HELPER_MASS;
				keyArcSumY += alpha_y * pKid->diacKeyYOffset*HELPER_MASS;
			}
#endif
			/* No need for overflow check here */
			a1 = (curveArcSumX*keyArcSumX + curveArcSumY*keyArcSumY) / 64;
			a2 = (curveArcSumX*keyArcSumY - curveArcSumY*keyArcSumX) / 64;

			sj1 = HELPER_MASS * arcScProd1 - a1; /* (sj1,sj2) = sj(Phi,Psi) ... (here multiplied with arc mass) */
			sj2 = HELPER_MASS * arcScProd2 - a2;

			temp = decumaAbs(sj1);
			temp = maxNUMB(temp, decumaAbs(sj2));

			while ( temp > (MAX_INT32_SQRT*100)/142 ) /* should be: 2*temp^2 < MAX_INT32 */
			{
				temp >>= 1;
				nRight++;
			}

			sj1 >>= nRight;
			sj2 >>= nRight;

			decumaAssert( (float) sj1 * sj1 + (float) sj2 * sj2 < MAX_UINT32); /*Overflow check */

			sjNorm = pCurve->noArcs * decumaSqrt(sj1*sj1 + sj2*sj2); /*The norm |sj(Phi,Psi)| multiplied with m */
			sjNorm <<= nRight;

			decumaAssert( sumSjNorm <= sumSjNorm + sjNorm);
			sumSjNorm += sjNorm;

			sumT1 += a1 / HELPER_MASS; /*(sumT1,sumT2) = t(Phi,Psi) */
			sumT2 += a2 / HELPER_MASS;
#ifdef DO_LOGGING_FULLSEARCH
	logVariable32("!!!",sumT1);
#endif
		}
		else
		{
#ifdef DO_LOGGING_FULLSEARCH
			logVariable32("as1",arcScProd1);
			logVariable32("as2",arcScProd2);
#endif
			sumT1 += arcScProd1; /*eventuellt multiplicera med helper mass h�r ist�llet f�r division ovan! */
			sumT2 += arcScProd2;
		}

#if !defined(ARC_SUMS_NOT_SUPPORTED_IN_DB)
		curveSumX += curveArcSumX / 8;
		curveSumY += curveArcSumY / 8;

		keySumX += keyArcSumX / 8;
		keySumY += keyArcSumY / 8;
#endif
	}


	decumaAssert( decumaAbs(sumT1) <= MAX_DECUMA_INT32 / HELPER_MASS / MAX_NUMBER_OF_ARCS_IN_CURVE ); /*Overflow check */
	decumaAssert( decumaAbs(sumT2) <= MAX_DECUMA_INT32 / HELPER_MASS / MAX_NUMBER_OF_ARCS_IN_CURVE); /*Overflow check */

#if defined(ARC_SUMS_NOT_SUPPORTED_IN_DB)
		curveSumX=pCurve->sum_x;
		curveSumY=pCurve->sum_y;
		keySumX=kidGetInterleavedSumX(pKid);
		keySumY=kidGetInterleavedSumY(pKid);
#endif

#ifdef DO_LOGGING_FULLSEARCH
	logVariable32("ST1",sumT1);
	logVariable32("ST2",sumT2);
#endif
	sumT1 *= HELPER_MASS * curveGetNoArcs(pCurve);
	sumT1 -= curveSumX * keySumX;
	sumT1 -= curveSumY * keySumY;

	sumT2 *= HELPER_MASS * curveGetNoArcs(pCurve);
	sumT2 -= curveSumX * keySumY;
	sumT2 += curveSumY * keySumX;
	/*(sumT1,sumT2) = t(Phi,Psi) multiplied with the curve mass */


#if !defined(ARC_SUMS_NOT_SUPPORTED_IN_DB)
	keyNorm = keyNorm * HELPER_MASS * curveGetNoArcs(pCurve)
		- keySumX * keySumX - keySumY * keySumY;

	curveNorm = curveNorm * HELPER_MASS * curveGetNoArcs(pCurve)
		- curveSumX * curveSumX - curveSumY * curveSumY;
#else
	keyNorm = kidGetInterleavedNorm(pKid);
	curveNorm = pCurve->norm;
#endif

	decumaAssert(pMuOut);

#ifdef DO_LOGGING_FULLSEARCH
	//logVariable32("ST1",sumT1);
	//logVariable32("ST2",sumT2);
	logVariable32u("sSN",sumSjNorm);
	logVariable32u("cuN",curveNorm);
	logVariable32u("kyN",keyNorm);
#endif

	*pMuOut = getProxMuApproximation(sumT1, sumT2, sumSjNorm, curveNorm, keyNorm);

	proxStoreAngle(pTheta, sumT1, sumT2);
}

#undef HELPER
#undef HELPER2
#undef HELPER_SUM
#undef HELPER_MASS


void preCalculateCurveInterleavedNoDerivate(SCR_CURVE *pCurve)
{
	int mass = 0;
	int a,p;

	DECUMA_UINT32 curveNorm = 0;
	DECUMA_INT32 curveSumX = 0;
	DECUMA_INT32 curveSumY = 0;

	for(a = 0 ; a < curveGetNoArcs(pCurve) ; a++)
	{
		const DECUMA_INT16 * pCurveX = pCurve->Arcs[a]->x;
		const DECUMA_INT16 * pCurveY = pCurve->Arcs[a]->y;
#if !defined(ARC_SUMS_NOT_SUPPORTED_IN_DB)
		const DECUMA_INT32 alpha_x = pCurve->Arcs[a]->alpha_x;
		const DECUMA_INT32 alpha_y = pCurve->Arcs[a]->alpha_y;
		const DECUMA_INT32 alpha_x2 = alpha_x * alpha_x;
		const DECUMA_INT32 alpha_y2 = alpha_y * alpha_y;
		const DECUMA_INT32 alpha_xy = alpha_x * alpha_y;
#else
		/* For now the database format does not support fast handling of different feature weight in */
		/* the full search. Therefore we here have to set the feature weight to those used in the current */
		/* db precalculations that will be retreived later. This function should be modified to remove the */
		/* ifdefs when it does. /JA */
		const DECUMA_INT32 alpha_x = 8;
		const DECUMA_INT32 alpha_y = 8;
		const DECUMA_INT32 alpha_x2 = 64;
		const DECUMA_INT32 alpha_y2 = 64;
		const DECUMA_INT32 alpha_xy = 64;
#endif

		for(p = 0 ; p < curveGetArcLength(pCurve,a ) ; p++,pCurveX++,pCurveY++ )
		{

			if(p==0 || p==4 || p==8 || p==13 || p==18 || p==23 || p==27 || p==31)
			{
				mass++;
				decumaAssert( (float) (curveNorm) + (*pCurveX) * (*pCurveX) + (*pCurveY) * (*pCurveY) <= MAX_DECUMA_UINT32);
				/*This should not happen if we have correct coordinates */

				curveNorm += (alpha_x2 * (*pCurveX) * (*pCurveX) + alpha_y2 * (*pCurveY) * (*pCurveY)) / 64;
				curveSumX += alpha_x * (*pCurveX) / 8;
				curveSumY += alpha_y * (*pCurveY) / 8;
			}
		}
	}

	pCurve->norm = (float) (curveNorm) * mass - curveSumX * curveSumX - curveSumY * curveSumY;

	pCurve->sum_x = curveSumX;
	pCurve->sum_y = curveSumY;

	pCurve->measure_id = -1;
}

