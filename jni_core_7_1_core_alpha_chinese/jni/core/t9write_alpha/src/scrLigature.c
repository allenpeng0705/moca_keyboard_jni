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


/*$Header: /data/cvsroot/seattle/t9write/alphabetic/core/src/scrLigature.c,v 1.5 2011/02/14 11:41:14 jianchun_meng Exp $*/
#include "scrOutput.h"

#include "scrProxCurve.h"
#include "decumaMath.h"
#include "decumaAssert.h"
#include "decumaDataTypes.h"
#include "scrMeasureId.h"
#include "scrLigature.h"
#include "database.h"
#include "globalDefs.h"

#undef DO_LOGGING
#ifdef DO_LOGGING
#include "t9write_log.h"
#endif

/*////////////////////////////////////////////////////////////////////////// */
/*Local constants */
/*////////////////////////////////////////////////////////////////////////// */

#ifdef ONPALM_ARMLET
/* ARMlets cannot have static const data */
#define STATIC_SPECIFIER
#else
#define STATIC_SPECIFIER static
#endif

STATIC_SPECIFIER const int cutLeftUpperLimit = NUMBER_OF_POINTS_IN_ARC/2;
STATIC_SPECIFIER const int cutLeftLowerLimit = -4;
STATIC_SPECIFIER const int cutRightUpperLimit = NUMBER_OF_POINTS_IN_ARC/2;
STATIC_SPECIFIER const int cutRightLowerLimit = -4;

#define ComputeAffine_preventOverflow (16)

/*////////////////////////////////////////////////////////////////////////// */
/*Local functions */
/*////////////////////////////////////////////////////////////////////////// */

static int ComputeAffine(	const SCR_CURVE *curve, const KID * pKid,
							int measureId,
							const int arcToCut, float *a, float *b,
							const DECUMA_INT8 * arcOrder, DECUMA_BOOL fromZoom,
							int start, int stop);

/*////////////////////////////////////////////////////////////////////////// */

void getPunishPerCut(int lowestUncutMu, int* pPunishPerCut, int * pMaxCuts)
{
	/*Returns a number which is the factor for the future punishment for cutting. */
	/*The inparameter should be the best of all proximity measures, for the uncut */
	/*curves. If this proximity measure is good (low) we already have a good curve */
	/*and cutting in that one will be more punished. If it is bad we wont punish */
	/*that much for cutting. */

	/*These constants have been attempted to be optimized */
	const int lowMu = 100;
	const int highMu = 600;

	const int highPunish = 30;
	const int lowPunish = 5;

	const int lowNCuts = 1;
	const int highNCuts = 6;


	lowestUncutMu = maxNUMB(lowestUncutMu,lowMu);
	lowestUncutMu = minNUMB(lowestUncutMu,highMu);

	*pPunishPerCut = highPunish-(highPunish-lowPunish)*(lowestUncutMu-lowMu)/(highMu-lowMu); /*punishPerCut */
	*pMaxCuts = lowNCuts+(highNCuts-lowNCuts)*(lowestUncutMu-lowMu)/(highMu-lowMu);
}

void PunishForCutting(scrOUTPUT *proxData, const int punishPerCut)
{
	int leftCuts = maxNUMB(0,proxData->nCutLeft);
	int rightCuts = maxNUMB(0,proxData->nCutRight);

	/* It is assumed that the numbers in the calculations below
	 * are small enough also to fit in 16/bit calculations
	 */
	proxData->punish += punishPerCut * (leftCuts + rightCuts);
	decumaAssert(proxData->punish <= MAX_DECUMA_INT16);
}

int GaussNewton(scrOUTPUT *proxData, const int measureId, const int arcToCut,
				 const SCR_CURVE *curve, SCR_CURVE *curveCut, const scrOUTPUT_LIST * pPrecalculatedBaseOutputs,
				 DECUMA_BOOL fromZoom, int start, int stop, const int nMaxCuts)
{
	/*Computes cuts at ends of arc number arcToCut. The number of cut points are stored in */
	/*proxData. An interpolated curve is computed as curveCut. */

	/*NOTE the following regarding different stroke orders and stroke directions of KEY and SCR_CURVE */
	/* - arcToCut is CURVEindexed (i.e. the number of the arc in the SCR_CURVE, not necessariky the same as in the KEY) */
	/* - All point numbers are KEYindexed (i.e. as defined in the KEY) */
	/* - Also, LEFT and RIGHT are KEYIndexed (will refer to left and right as in the key) */

	/*TODO: change the optimaztion so that it regards also the punishment per cut. I.e. */
	/*it finds the number of cuts that gives the minimum of (proximity measure + cut punish). */
	int scaledCutLeft = (int)(proxData->nCutLeft * ACCURACY_IN_INTERPOLATE + (NUMBER_OF_POINTS_IN_ARC - 1)/2)
			/(NUMBER_OF_POINTS_IN_ARC - 1); /*KEYindexed */
	int scaledCutRight= (int)(proxData->nCutRight * ACCURACY_IN_INTERPOLATE + (NUMBER_OF_POINTS_IN_ARC - 1)/2)
			/(NUMBER_OF_POINTS_IN_ARC - 1); /*KEYindexed */


	const KID * pKid = &proxData->DBindex;
	int maxCutLeft = nMaxCuts;
	int maxCutRight = nMaxCuts;

  	int arcToCutK = curveToKeyIndexOfArc(proxData->arcOrder,arcToCut);
	DECUMA_BOOL arcToCutIsReverse = arcIsReverse(proxData->arcOrder,arcToCutK);

#ifdef DO_LOGGING
	logVariable32("scl",scaledCutLeft);
	logVariable32("scr",scaledCutRight);
#endif
	if (pKid->noBaseArcs <= 2 && (pKid->noDiacArcs == 0 || !pPrecalculatedBaseOutputs ||
		kidGetSymmetry(pKid) == 0))
	{
		const int noIterations = pKid->noBaseArcs == 1 ? 2 : 1;
		float aNew;
		float bNew;
		float b = (float)proxData->nCutLeft;
		float a = (1 - ((float)(proxData->nCutRight + proxData->nCutLeft)) / (NUMBER_OF_POINTS_IN_ARC - 1));
		int j; /*Counters */
		float cutLeftFloat=0.0f, cutRightFloat=0.0f; /* Assignements uneccesary, avoids compiler warnings. //KEYindexed */

		decumaAssert(curve != 0);
		decumaAssert(curveCut != 0);
		decumaAssert(databaseValidKID(*pKid));
		decumaAssert(arcToCut >= 0);
		decumaAssert(arcToCut < curve->noArcs);

		for(j = 0; j < noIterations; j++)
		{
			if (arcToCutIsReverse)
			{
				InterpolateArc(curveCut->Arcs[ arcToCut ],
					curve->Arcs[ arcToCut ], scaledCutRight,
					ACCURACY_IN_INTERPOLATE - scaledCutLeft); /*Only interpolate 'arcToCut'. */
			}
			else
			{
				InterpolateArc(curveCut->Arcs[ arcToCut ],
					curve->Arcs[ arcToCut ], scaledCutLeft,
					ACCURACY_IN_INTERPOLATE - scaledCutRight); /*Only interpolate 'arcToCut'. */
			}

			ComputeAffine(curveCut, pKid, measureId, arcToCutK, &aNew, &bNew, proxData->arcOrder, fromZoom, start, stop);
			b += a*bNew;
			a *= aNew;
			cutLeftFloat = b;
			cutRightFloat = ((NUMBER_OF_POINTS_IN_ARC - 1) - a * (NUMBER_OF_POINTS_IN_ARC - 1) - b);

			/* Sanity checks.. */
			if (fromZoom)
			{
				cutLeftFloat = boundsCheck(cutLeftFloat, (float)(-4), (float)16);
				cutRightFloat = boundsCheck(cutRightFloat, (float)(-4), (float)16);
			}
			else
			{
				cutLeftFloat = boundsCheck(cutLeftFloat, (float)cutLeftLowerLimit, (float)cutLeftUpperLimit);
				cutRightFloat = boundsCheck(cutRightFloat, (float)cutRightLowerLimit, (float)cutRightUpperLimit);
			}

            scaledCutLeft = (int)(cutLeftFloat * ACCURACY_IN_INTERPOLATE + (NUMBER_OF_POINTS_IN_ARC - 1)/2)
                    /(NUMBER_OF_POINTS_IN_ARC - 1);
            scaledCutRight= (int)(cutRightFloat * ACCURACY_IN_INTERPOLATE + (NUMBER_OF_POINTS_IN_ARC - 1)/2)
                    /(NUMBER_OF_POINTS_IN_ARC - 1);

#ifdef DO_LOGGING
	logVariable32("2cl",scaledCutLeft);
	logVariable32("2cr",scaledCutRight);
#endif
		}

		proxData->nCutLeft = decumaRound( cutLeftFloat );
		proxData->nCutRight = decumaRound( cutRightFloat );

#ifdef DO_LOGGING
		logVariable32("cle",proxData->nCutLeft);
		logVariable32("cri",proxData->nCutRight);
#endif
		proxData->nCutLeft = minNUMB(proxData->nCutLeft, maxCutLeft);
		proxData->nCutRight = minNUMB(proxData->nCutRight, maxCutRight);
	}

	if (arcToCutIsReverse)
	{
		InterpolateArc(curveCut->Arcs[ arcToCut ],
			curve->Arcs[ arcToCut ],
			minNUMB(scaledCutRight,maxCutLeft*ACCURACY_IN_INTERPOLATE/32),
			ACCURACY_IN_INTERPOLATE - minNUMB(scaledCutLeft,maxCutRight*ACCURACY_IN_INTERPOLATE/32));
	}
	else
	{
		InterpolateArc(curveCut->Arcs[ arcToCut ],
			curve->Arcs[ arcToCut ],
			minNUMB(scaledCutLeft,maxCutLeft*ACCURACY_IN_INTERPOLATE/32),
			ACCURACY_IN_INTERPOLATE - minNUMB(scaledCutRight,maxCutRight*ACCURACY_IN_INTERPOLATE/32));
	}


	if (!fromZoom)
	{
		/* Build the final curve. */
		buildCurveMutation(curveCut, measureId,proxData->arcOrder);
	}


	return 1;

}


static int ComputeAffine(	const SCR_CURVE *curve, const KID * pKid,
							int measureId,
							const int arcToCutK, float *a, float *b,
							const DECUMA_INT8 * arcOrder, DECUMA_BOOL fromZoom,
							int start, int stop)
{
	/*Computes the affine transformation a*t+b where 0<=t<NUMBER_OF_POINTS_IN_ARC */
	/*such that proxCurveKey(curve(a*t+b),key) is minimized. */
	/*NOTE:!!! arcToCutK is here KEYIndexed */
	/*NOTE!!! start and stop are KEYindexed */
	/*        (as defined in the KEY maybe not in the SCR_CURVE depending on the arcOrder) */

	typedef DECUMA_INT32 TYPE;
	int mass = 0;
	int arcNr,t,k; /*Counters */
	int firstArcNr, lastArcNr;
	int noIterations,step,startPt;

	DECUMA_INT32 curveXSum = 0;
	DECUMA_INT32 curveYSum = 0;
	DECUMA_INT32 keyXSum = 0;
	DECUMA_INT32 keyYSum = 0;
	int keyX;
	int keyY;
	const DECUMA_INT16 *curveX;
	const DECUMA_INT16 *curveY;

	DECUMA_INT32 curveD1;
	DECUMA_INT32 curveD2;
	DECUMA_INT32 curveXNorm;
	DECUMA_INT32 curveYNorm;
	DECUMA_INT32 keyXNorm;
	DECUMA_INT32 keyYNorm;

	TYPE  s[6] = {0,0,0,0,0,0};
	float sp[6] = {0,0,0,0,0,0};
	TYPE s1[4] = {0,0,0,0};
	TYPE s2[4] = {0,0,0,0};
	TYPE s3[4] = {0,0,0,0};
	TYPE n = 0;
	float A11,A12,A21,A22,B1,B2,determinant;
	float temp1,temp2;
	TYPE temp;

	const int curveTotalLength=curveGetTotalLength(curve);
	DECUMA_INT8_DB_PTR keyArc;
	DECUMA_INT8 arcOffsetY;

	decumaAssert(a != 0 && b!= 0);
	decumaAssert(curve != 0 && databaseValidKID(*pKid));
	decumaAssert(arcToCutK >= 0);
	decumaAssert(arcToCutK < curve->noArcs);
	decumaAssert(NUMBER_OF_POINTS_IN_ARC == 32);

	if (fromZoom)
	{
		measureId = 0;
	}
	else
	{
		start = 0;
		stop = NUMBER_OF_POINTS_IN_ARC-1;
	}

	if (fromZoom)
	{
		firstArcNr = arcToCutK;
		lastArcNr = arcToCutK;
	}
	else
	{
		firstArcNr = 0;
		lastArcNr = curveGetNoArcs(curve)-1;
	}

	/*/////////////////// */
	/*  Compute means  // */
	/*/////////////////// */

	noIterations = NUMBER_OF_POINTS_IN_ARC;
	for(arcNr = firstArcNr; arcNr <= lastArcNr; arcNr++)
	{
		if arcIsReverse(arcOrder,arcNr)
		{
			startPt=NUMBER_OF_POINTS_IN_ARC-1;
			step=-1;
		}
		else
		{
			startPt=0;
			step=1;
		}

		keyArc=kidGetArcPointer(pKid,arcNr);
		arcOffsetY = keyGetArcOffsetY(pKid, arcNr);
		curveX = &(curve->Arcs[ getArcNr(arcOrder,arcNr) ]->x[startPt]);
		curveY = &(curve->Arcs[ getArcNr(arcOrder,arcNr) ]->y[startPt]);

		k=getArcNr(arcOrder,arcNr)*NUMBER_OF_POINTS_IN_ARC+startPt;

		for(t = 0; t < noIterations; t++, curveX+=step, curveY+=step, k+=step)
		{
			if( (t < start) || (t > stop) )
			{
				continue;
			}
			if(GetMeasureIncExcl(measureId, curveTotalLength ,k) == 0)
			{
				continue;
			}
			curveXSum += (DECUMA_INT32)(*curveX);
			curveYSum += (DECUMA_INT32)(*curveY);
			keyXSum += (DECUMA_INT32)keyarcGetX(keyArc,t);
			keyYSum += (DECUMA_INT32)keyarcGetY(keyArc,t) + arcOffsetY;
			/* arcOffsetY is only non-zero if the key arc is part of the diacritic key. */
			mass++;
		}
	}

	/*///////////////////////////////////// */
	/*       Compute scalar products     // */
	/*///////////////////////////////////// */

	for(arcNr = firstArcNr; arcNr <= lastArcNr; arcNr++)
	{
		if arcIsReverse(arcOrder,arcNr)
		{
			startPt=NUMBER_OF_POINTS_IN_ARC-1;
			step=-1;
		}
		else
		{
			startPt=0;
			step=1;
		}

		keyArc = kidGetArcPointer( pKid, arcNr);
		arcOffsetY = keyGetArcOffsetY(pKid, arcNr);
		curveX = &(curve->Arcs[ getArcNr(arcOrder,arcNr) ]->x[startPt]);
		curveY = &(curve->Arcs[ getArcNr(arcOrder,arcNr) ]->y[startPt]);

		k=getArcNr(arcOrder,arcNr)*NUMBER_OF_POINTS_IN_ARC+startPt;

		for(t = 0; t < noIterations; t++, curveX+=step, curveY+=step,k+=step)
		{
			if( (t < start) || (t > stop))
			{
				continue;
			}
			if(GetMeasureIncExcl(measureId, curveTotalLength ,k) == 0)
			{
				continue;
			}
			keyX = keyarcGetX(keyArc,t);
			keyY = keyarcGetY(keyArc,t) + arcOffsetY;
			/* arcOffsetY is only non-zero if the key arc is part of the diacritic key. */


			curveXNorm = (*curveX * (DECUMA_INT32)mass - curveXSum) / ComputeAffine_preventOverflow;
			curveYNorm = (*curveY * (DECUMA_INT32)mass - curveYSum) / ComputeAffine_preventOverflow;
			keyXNorm = (keyX * (DECUMA_INT32)mass - keyXSum) / ComputeAffine_preventOverflow;
			keyYNorm = (keyY * (DECUMA_INT32)mass - keyYSum) / ComputeAffine_preventOverflow;

			s1[0] += keyXNorm*curveXNorm;
			s1[1] += keyYNorm*curveXNorm;
			s1[2] += keyXNorm*curveYNorm;
			s1[3] += keyYNorm*curveYNorm;


			n += keyXNorm*keyXNorm + keyYNorm*keyYNorm;
		}
	}

	/*//////////////////////////// */
	/*   Compute derivative     // */
	/*//////////////////////////// */

	/*Compute derivative for the arc that should be cut */
	keyArc=kidGetArcPointer(pKid, arcToCutK);
	arcOffsetY = keyGetArcOffsetY(pKid, arcToCutK);
	k= arcToCutK*NUMBER_OF_POINTS_IN_ARC;
	noIterations = NUMBER_OF_POINTS_IN_ARC;
	if ( arcIsReverse(arcOrder, arcToCutK) )
	{
		startPt=noIterations-1;
		step=-1;
	}
	else
	{
		startPt=0;
		step=1;
	}

	curveX = &(curve->Arcs[ getArcNr(arcOrder,arcToCutK) ]->x[startPt]);
	curveY = &(curve->Arcs[ getArcNr(arcOrder,arcToCutK) ]->y[startPt]);
	k+=startPt;

	for(t = 0; t < noIterations; t++, curveX+=step, curveY+=step, k+=step)
	{
		if( (t < start) || (t > stop) )
		{
			continue;
		}
		if(GetMeasureIncExcl(measureId, curveTotalLength ,k) == 0)
		{
			continue;
		}

		keyX = keyarcGetX(keyArc,t);
		keyY = keyarcGetY(keyArc,t) + arcOffsetY;
		/* arcOffsetY is only non-zero if the key arc is part of the diacritic key. */
		curveXNorm = (*curveX * (DECUMA_INT32)mass - curveXSum) / ComputeAffine_preventOverflow;
		curveYNorm = (*curveY * (DECUMA_INT32)mass - curveYSum) / ComputeAffine_preventOverflow;
		keyXNorm = (keyX * (DECUMA_INT32)mass - keyXSum) / ComputeAffine_preventOverflow;
		keyYNorm = (keyY * (DECUMA_INT32)mass - keyYSum) / ComputeAffine_preventOverflow;



		if(t < noIterations - 1)
		{
			curveD1 = ((*(curveX + 1*step) - *curveX) * (DECUMA_INT32)mass) / ComputeAffine_preventOverflow;
			curveD2 = ((*(curveY + 1*step) - *curveY) * (DECUMA_INT32)mass) / ComputeAffine_preventOverflow;
		}
		else
		{
			curveD1 = ((*curveX - *(curveX - 1*step)) * (DECUMA_INT32)mass) / ComputeAffine_preventOverflow;
			curveD2 = ((*curveY - *(curveY - 1*step)) * (DECUMA_INT32)mass) / ComputeAffine_preventOverflow;
		}

		temp = (TYPE)curveD1*curveD1 + (TYPE)curveD2*curveD2;
		s[3] += temp;
		temp *= t;
		s[1] += temp;
		s[0] += temp*t;

		temp = (TYPE)curveXNorm*curveD1 + (TYPE)curveYNorm*curveD2;
		s[5] += temp;
		s[2] += (temp*t);

		temp = (TYPE)keyXNorm*curveD1;
		s2[0] += temp;
		s3[0] += temp*t;
		temp = (TYPE)keyYNorm*curveD1;
		s2[1] += temp;
		s3[1] += temp*t;
		temp = (TYPE)keyXNorm*curveD2;
		s2[2] += temp;
		s3[2] += temp*t;
		temp = (TYPE)keyYNorm*curveD2;
		s2[3] += temp;
		s3[3] += temp*t;
	}



	s[4] = s[1];

	temp1 = (float)(s3[0] + s3[3]);
	temp2 = (float)(-s3[1] + s3[2]);
	sp[0] = temp1*temp1 + temp2*temp2;
	sp[1] = (s2[0] + s2[3])*temp1 + (-s2[1] + s2[2])*temp2;
	sp[2] = (s1[0] + s1[3])*temp1 + (-s1[1] + s1[2])*temp2;
	temp1 = (float)(s2[0] + s2[3]);
	temp2 = (float)(-s2[1] + s2[2]);
	sp[3] = temp1*temp1 + temp2*temp2;
	sp[4] = sp[1];
	sp[5] = (s1[0] + s1[3])*temp1 + (-s1[1] + s1[2])*temp2;

	A11 = ((float)s[0]) * ((float)n) - sp[0];
	A12 = ((float)s[1]) * ((float)n) - sp[1];
	A22 = ((float)s[3]) * ((float)n) - sp[3];
	A21 = ((float)s[4]) * ((float)n) - sp[4];

	B1 = ((float)s[2]) * ((float)n) - sp[2];
	B2 = ((float)s[5]) * ((float)n) - sp[5];

	determinant = A11*A22 - A21*A12;

	/*Compute the affine transformation */
	if (determinant != 0) /*It would be better to test the condition value. */
	{
		*a = (float)(-(A22 * B1 - A12 * B2)/determinant + 1);
		*b = (float)(-(-A21 * B1 + A11 * B2)/determinant);
	}
	else
	{
		*a = (float)1;
		*b = (float)0;
	}

	return 1;
}

/* end of file */

