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

#include <stddef.h> /* Definition of NULL */

#ifdef MATLAB_MEX_FILE
#include "mex.h"
#endif

#include "scrCurve.h"
#include "database.h"
#include "databaseKEY.h"
#include "decumaMath.h"
#include "decumaAssert.h"
#include "scrMeasureId.h"
#include "globalDefs.h"

#ifdef _WIN32
#ifndef MATLAB_MEX_FILE
#include <stdio.h>
#include <stdlib.h>
#endif
#endif

#undef DO_LOGGING
#ifdef DO_LOGGING
#include "t9write_log.h"
#endif

void curveCopy(SCR_CURVE* pTarget,  const SCR_CURVE * pSource)
{
	int j;

	decumaAssert( pSource );
	decumaAssert( pTarget );
	for(j = 0; j < pSource->noArcs; j++)
	{
		if(pTarget->Arcs[j] != pSource->Arcs[j])
			/*No need to copy the identity. */
			*pTarget->Arcs[j] = *pSource->Arcs[j];
	}
	pTarget->mass = pSource->mass;
	pTarget->measure_id = pSource->measure_id;
	pTarget->noArcs = pSource->noArcs;
	pTarget->norm = pSource->norm;
	pTarget->sum_x = pSource->sum_x;
	pTarget->sum_y = pSource->sum_y;
	pTarget->sum_dx = pSource->sum_dx;
	pTarget->sum_dy = pSource->sum_dy;
}

#ifdef DEBUG_DUMP_SCR_DATA
void curveDump(char *filename, const SCR_CURVE *p)
{
    int i, k;
	FILE *fp = filename ? fopen(filename,"w+") : stdout;

	/*fprintf(fp,"%d",p->x[0].nNumbers); */
	for(k = 0; k < p->noArcs; k++)
	{
		fprintf(fp, "%d ", NUMBER_OF_POINTS_IN_ARC);
		for(i = 0; i < NUMBER_OF_POINTS_IN_ARC ; i++) {
			fprintf(fp,"%2d %2d ", (int)p->Arcs[k]->x[i], (int)p->Arcs[k]->y[i]);
		}
		fprintf(fp, "\n");
	}

		if(fp != stdout)
			fclose(fp);

}
#endif /*DEBUG_DUMP_SCR_DATA */

#ifdef DEBUG_DUMP_SCR_DATA
/* Writes the data in the struct CURVE to a file.  */
void curveDataDump(FILE * pf, const SCR_CURVE * pC)
{
	int a,i;
	fprintf(pf, "\n*** SCR_CURVE start ***\n");
	fprintf(pf, "nArcs          = %d\n", pC->noArcs);
	fprintf(pf, "SumX           = %d\n", pC->sum_x);
	fprintf(pf, "SumY           = %d\n", pC->sum_y);
	fprintf(pf, "SumDX          = %d\n", pC->sum_dx);
	fprintf(pf, "SumDY          = %d\n", pC->sum_dy);
	fprintf(pf, "Norm           = %f\n", pC->norm);
	fprintf(pf, "mass           = %d\n", pC->mass);
	fprintf(pf, "measure_id     = %d\n", pC->measure_id);
	fprintf(pf, "Arcs:\n");
	for ( a = 0; a < pC->noArcs; a++ ) {
		fprintf(pf, "x y:   ");
		for ( i = 0; i < NUMBER_OF_POINTS_IN_ARC; i++) {
			fprintf(pf, " %3d %3d", pC->Arcs[a]->x[i], pC->Arcs[a]->y[i]);
		}
		fprintf(pf, "\n");

		fprintf(pf, "dx dy: ");
		for ( i = 0; i < NUMBER_OF_POINTS_IN_ARC; i++) {
			fprintf(pf, " %3d %3d", pC->Arcs[a]->dx[i], pC->Arcs[a]->dy[i]);
		}
		fprintf(pf, "\n");
	}
	fprintf(pf, "***  SCR_CURVE end  ***\n");
} /* curveDataDump() */
#endif /*DEBUG_DUMP_SCR_DATA */


void curvePlot(SCR_CURVE *p, int nr)
{
#ifdef MATLAB_MEX_FILE
	mxArray  *rhs[2];
	double   *v;
    int i;


	rhs[0] = mxCreateDoubleMatrix(p->x[0].nNumbers,2, mxREAL);
    rhs[1] = mxCreateDoubleMatrix(1,1,mxREAL);

	v = mxGetPr(rhs[0]);

	for(i = 0; i < p->x[0].nNumbers; i++) {
		v[i]                  = p->x[0].pNumbers[i];
		v[i+p->x[0].nNumbers] = p->x[1].pNumbers[i];
	}

	v = mxGetPr(rhs[1]);
	v[0] = nr;

	mexCallMATLAB(0,NULL,2,rhs,"plotta");
#endif
}

int curveGetTotalLength(const SCR_CURVE * pCurve)
{
	int a;
	int nTotal = 0;
	for(a = 0 ; a < curveGetNoArcs(pCurve) ; a++)
	{
		nTotal += curveGetArcLength(pCurve, a);
	}
	/* Assumes nTotal < max 16-bit */
	return nTotal;
}

void arcPrecalculate(SCR_ARC * pTargetArc, const DECUMA_INT16 * pbIncludePointAccToMeasureId)
{
	DECUMA_INT32 mass = 0;
	DECUMA_INT32 sum_x = 0;
	DECUMA_INT32 sum_y = 0;
	DECUMA_INT32 sum_dx = 0;
	DECUMA_INT32 sum_dy = 0;
	DECUMA_INT32 sum_abs_dx = 0;
	DECUMA_INT32 sum_abs_dy = 0;
	DECUMA_UINT32 sum_x2 = 0;
	DECUMA_UINT32 sum_y2 = 0;
	DECUMA_UINT32 sum_dx2 = 0;
	DECUMA_UINT32 sum_dy2 = 0;
	DECUMA_INT16 min_x = MAX_DECUMA_INT16;
	DECUMA_INT16 max_x = MIN_DECUMA_INT16;
	DECUMA_INT16 min_y = MAX_DECUMA_INT16;
	DECUMA_INT16 max_y = MIN_DECUMA_INT16;
	int j;
	const DECUMA_INT16 * pbIncludeIterator = &pbIncludePointAccToMeasureId[0];

	/* First, calculate the derivate of the arc (the dx and dy members are later multiplied with nAlphaD) */
	derivative(&pTargetArc->x[0], &pTargetArc->dx[0], sizeof(pTargetArc->dx) / sizeof(pTargetArc->dx[0]));
	derivative(&pTargetArc->y[0], &pTargetArc->dy[0], sizeof(pTargetArc->dy) / sizeof(pTargetArc->dy[0]));

	/* Multiply by the measure p->xMu[0] */
	/* and compute norm and means. */
	for(j = 0;j < sizeof(pTargetArc->x) / sizeof(pTargetArc->x[0]) ; j++)
	{
		if( pbIncludePointAccToMeasureId != NULL )
		{
			if( *pbIncludeIterator == 0)
			{
				pbIncludeIterator++;

				/* No need to do all the complex calculations, since they would all be  */
				/* multiplied with zero in the end anyway. However, we must still multiply */
				/* dx and dy with alpha. */

				/*pTargetArc->dx[j] = 0; */
				/*pTargetArc->dy[j] = 0; */
				/*pTargetArc->x[j] = 0; */
				/*pTargetArc->y[j] = 0; */

				continue;
			}
			else
			{
				/* Take the mass from the measure mask */
				mass += *pbIncludeIterator;
				pbIncludeIterator++;
			}
		}
		else
		{
			/* There was no measure mask, i.e. the entire mask was set to ones. */
			mass++;
		}

		/* No overflow assert */
		decumaAssert(sum_x2 <= sum_x2 +
			((DECUMA_INT32)pTargetArc->x[j]) * ((DECUMA_INT32)pTargetArc->x[j]));
		decumaAssert(sum_y2 <= sum_y2 +
			((DECUMA_INT32)pTargetArc->y[j]) * ((DECUMA_INT32)pTargetArc->y[j]));
		decumaAssert(sum_dx2 <= sum_dx2 +
			((DECUMA_INT32)pTargetArc->dx[j]) * ((DECUMA_INT32)pTargetArc->dx[j]));
		decumaAssert(sum_dy2 <= sum_dy2 +
			((DECUMA_INT32)pTargetArc->dy[j]) * ((DECUMA_INT32)pTargetArc->dy[j]));

		sum_x2 += ((DECUMA_INT32)pTargetArc->x[j]) * ((DECUMA_INT32)pTargetArc->x[j]);
		sum_y2 += ((DECUMA_INT32)pTargetArc->y[j]) * ((DECUMA_INT32)pTargetArc->y[j]);

		sum_dx2 += ((DECUMA_INT32)pTargetArc->dx[j]) * ((DECUMA_INT32)pTargetArc->dx[j]);
		sum_dy2 += ((DECUMA_INT32)pTargetArc->dy[j]) * ((DECUMA_INT32)pTargetArc->dy[j]);

#ifdef DO_LOGGING
	/*logVariable32("xj",pTargetArc->x[j]); */
	/*logVariable32("yj",pTargetArc->y[j]); */
	/*logVariable32u("sx2",sum_x2); */
	/*logVariable32u("sy2",sum_y2); */
	/*logVariable32u("dx2",sum_dx2); */
	/*logVariable32u("dy2",sum_dy2); */
#endif
		sum_x += pTargetArc->x[j];
		sum_y += pTargetArc->y[j];

		sum_dx += pTargetArc->dx[j];
		sum_dy += pTargetArc->dy[j];

		sum_abs_dx += decumaAbs(pTargetArc->dx[j]);
		sum_abs_dy += decumaAbs(pTargetArc->dy[j]);

		if (pTargetArc->x[j] < min_x){
			min_x = pTargetArc->x[j];
			pTargetArc->min_x_index = (DECUMA_UINT8) j;
		}
		if (pTargetArc->x[j] > max_x){
			max_x = pTargetArc->x[j];
			pTargetArc->max_x_index = (DECUMA_UINT8) j;
		}
		if (pTargetArc->y[j] < min_y){
			min_y = pTargetArc->y[j];
			pTargetArc->min_y_index = (DECUMA_UINT8) j;
		}
		if (pTargetArc->y[j] > max_y){
			max_y = pTargetArc->y[j];
			pTargetArc->max_y_index = (DECUMA_UINT8) j;
		}

	}

/*	norm *=mass; */
/*	norm -= sum_x * sum_x; */
/*	norm -= sum_y * sum_y; */

	/* vma Special fix to handle the case when all points are on the same coordinate. */
	/* That will cause the norm to be zero, and may calculates will go haywire. */
	if(sum_x2 == 0)
		sum_x2 = 1;
	if(sum_y2 == 0)
		sum_y2 = 1;
	if(sum_dx2 == 0)
		sum_dx2 = 1;
	if(sum_dy2 == 0)
		sum_dy2 = 1;

	pTargetArc->sum_x = sum_x;
	pTargetArc->sum_y = sum_y;

	pTargetArc->sum_dx = sum_dx;
	pTargetArc->sum_dy = sum_dy;

	pTargetArc->sum_abs_dx = sum_abs_dx;
	pTargetArc->sum_abs_dy = sum_abs_dy;

	pTargetArc->sum_x2 = sum_x2;
	pTargetArc->sum_y2 = sum_y2;

	pTargetArc->sum_dx2 = sum_dx2;
	pTargetArc->sum_dy2 = sum_dy2;

#ifdef DO_LOGGING
/*	logVariable32u("tx2",pTargetArc->sum_x2); */
/*	logVariable32u("ty2",pTargetArc->sum_y2); */
/*	logVariable32u("tdx",pTargetArc->sum_dx2); */
/*	logVariable32u("tdy",pTargetArc->sum_dy2); */
#endif
	pTargetArc->mass = mass;

}

void curveMinMaxCalculate(const SCR_CURVE * pCurve, int nArcs,
		DECUMA_INT16 * pStoreXMin, DECUMA_INT16 * pStoreXMax, DECUMA_INT16 * pStoreYMin, DECUMA_INT16 * pStoreYMax)
{
	/* NOTE Only the first 'nArcs' are used to calculate the min and max, */
	/* where nArcs is assumed to be equal to or less than pCurve->noArcs. */
	int a;
	DECUMA_INT16 temp;
	DECUMA_INT16 min_x = MAX_DECUMA_INT16;
	DECUMA_INT16 max_x = MIN_DECUMA_INT16;
	DECUMA_INT16 min_y = MAX_DECUMA_INT16;
	DECUMA_INT16 max_y = MIN_DECUMA_INT16;

	decumaAssert( nArcs <= pCurve->noArcs );

	for(a = 0 ; a < nArcs; a++)
	{
		temp = arcGetMinX( pCurve->Arcs[a] );
		if ( temp < min_x )
			min_x = temp;

		temp = arcGetMaxX( pCurve->Arcs[a] );
		if ( temp > max_x )
			max_x = temp;

		temp = arcGetMinY( pCurve->Arcs[a] );
		if ( temp < min_y )
			min_y = temp;

		temp = arcGetMaxY( pCurve->Arcs[a] );
		if ( temp > max_y )
			max_y = temp;
	}

	if ( pStoreXMin )
		*pStoreXMin = min_x;

	if ( pStoreXMax )
		*pStoreXMax = max_x;

	if ( pStoreYMin )
		*pStoreYMin = min_y;

	if ( pStoreYMax )
		*pStoreYMax = max_y;
}/* curveMinMaxCalculate() */

void curveMeanCalculate(const SCR_CURVE * pCurve, int nStartArc, int nStopArc,
		DECUMA_INT16 * pStoreXMean, DECUMA_INT16 * pStoreYMean)
/* Calculates the mean of the arcs nStartArc up to nStopArc in the  */
/* curve pCurve. */
{
	DECUMA_INT32 xSum = 0;
	DECUMA_INT32 ySum = 0;
	int k, mass = 0;

	decumaAssert( nStartArc >= 0 );
	decumaAssert( nStopArc < pCurve->noArcs );
	nStartArc = maxNUMB( nStartArc, 0);
	nStopArc = minNUMB( nStopArc, pCurve->noArcs - 1);

	for ( k = nStartArc; k <= nStopArc; k++ )
	{
		xSum += pCurve->Arcs[k]->sum_x;
		ySum += pCurve->Arcs[k]->sum_y;
		mass += pCurve->Arcs[k]->mass;
	}

	if ( pStoreXMean )
	{
		decumaAssert(xSum / mass <= MAX_DECUMA_INT16 && xSum / mass >= MIN_DECUMA_INT16);
		*pStoreXMean = (DECUMA_INT16) ( xSum / mass );
	}

	if ( pStoreYMean )
	{
		*pStoreYMean = (DECUMA_INT16) ( ySum / mass );
		decumaAssert(ySum / mass <= MAX_DECUMA_INT16 && ySum / mass >= MIN_DECUMA_INT16);
	}
} /*curveMeanCalculate() */

/* Calculates a curve metrics based on the data in the attached arcs.  */
/* Inputs: A SCR_CURVE that indicates what arcs that shall be included in the calculations. */
/*			the Arcs must have gone through the arcPrecalculate  */
/* Outputs; the SCR_CURVE object alpha, nMeasureID, sum_x, sum_y, norm and mass is altered. */
/*			the ARC objects are not touched. */

/* NOTE The arcs must have been precalculated with the same values for nMeasureID and alpha. */
void preCalculateCurveFromArcData(SCR_CURVE * pTargetCurve, int nMeasureID)
{
	int a;
	DECUMA_UINT32 normValue = 0;
	int nRight = 0;

	pTargetCurve->measure_id = nMeasureID;
	pTargetCurve->sum_x = 0;
	pTargetCurve->sum_y = 0;
	pTargetCurve->sum_dx = 0;
	pTargetCurve->sum_dy = 0;
	pTargetCurve->norm = 0.0f;
	pTargetCurve->mass = 0;
	for(a = 0 ; a < pTargetCurve->noArcs ; a++)
	{
		SCR_ARC * pArc = pTargetCurve->Arcs[a];
		DECUMA_UINT32 arcNorm = pArc->sum_x2 + pArc->sum_y2 + pArc->sum_dx2 + pArc->sum_dy2;

		pTargetCurve->sum_x += pArc -> sum_x;
		pTargetCurve->sum_y += pArc -> sum_y;
		pTargetCurve->sum_dx += pArc -> sum_dx;
		pTargetCurve->sum_dy += pArc -> sum_dy;
		while ( normValue > normValue + (arcNorm >> nRight) )
		{
			normValue >>= 1;
			nRight++;
		}

		normValue += arcNorm >> nRight;
#ifdef DO_LOGGING
	logVariable32u("sx2",pArc->sum_x2);
	logVariable32u("sy2",pArc->sum_y2);
	logVariable32u("dx2",pArc->sum_dx2);
	logVariable32u("dy2",pArc->sum_dy2);
	logVariable32u("nrm",norm);
#endif
		pTargetCurve->mass += pArc -> mass;
	}

	decumaAssert( (float) normValue * (float)pTargetCurve->mass >= (float) normValue );
	decumaAssert( (float)pTargetCurve->sum_x * (float)pTargetCurve->sum_x >= 0.0f );
	decumaAssert( (float)pTargetCurve->sum_y * (float)pTargetCurve->sum_y >= 0.0f );
	decumaAssert( (float)pTargetCurve->sum_x * (float)pTargetCurve->sum_x +
				  (float)pTargetCurve->sum_y * (float)pTargetCurve->sum_y >=
				  (float)pTargetCurve->sum_x * (float)pTargetCurve->sum_x );

	pTargetCurve->norm = (float) (1<<nRight);
	pTargetCurve->norm *= (float) normValue;
#ifdef DO_LOGGING
	logVariable32u("no2",pTargetCurve->norm);
#endif
	pTargetCurve->norm *= (float)pTargetCurve->mass;
#ifdef DO_LOGGING
	logVariable32u("no3",pTargetCurve->norm);
#endif
	pTargetCurve->norm -= (float)pTargetCurve->sum_x * (float)pTargetCurve->sum_x;
#ifdef DO_LOGGING
	logVariable32u("no4",pTargetCurve->norm);
#endif
	pTargetCurve->norm -= (float)pTargetCurve->sum_y * (float)pTargetCurve->sum_y;
#ifdef DO_LOGGING
	logVariable32u("no5",pTargetCurve->norm);
#endif

	/*if(pTargetCurve->norm <= 1.0 ) */
	/*	pTargetCurve->norm = 1.0f; */

}


void InterpolateArc(SCR_ARC *target, const SCR_ARC *source, int start, int stop)
{
	/*Interpolates sourcearc and puts in target. */
	/*start and stop indicates which portion of the arc that */
	/*considered. start = 0 and stop = 128 interpolates the entire arc. */
	/* Start and stop are ABSOLUTE numbers (i.e. referring to the way the arc was actually written) */

	int ind1, ind2,j;
	int count = start * (NUMBER_OF_POINTS_IN_ARC - 1);
	const int delta = (stop-start);

	for(j = 0; j < NUMBER_OF_POINTS_IN_ARC; j++)
	{
		ind1 = (int)count /ACCURACY_IN_INTERPOLATE;
		if (ind1 < 0)
		{
			target->x[j] = source->x[0];
			target->y[j] = source->y[0];
		}
		else
		{
			if (ind1 < NUMBER_OF_POINTS_IN_ARC-1)
			{

				if(ind1*ACCURACY_IN_INTERPOLATE < count) /*Has rounding errors occured */
				{
					DECUMA_INT32 targetX,targetY;
					ind2 = ind1 +1;

					targetX =
						( source->x[ind2] * (count - ind1*(DECUMA_INT32)ACCURACY_IN_INTERPOLATE) +
						  source->x[ind1] * (ind2*(DECUMA_INT32)ACCURACY_IN_INTERPOLATE - count)
						  ) / ACCURACY_IN_INTERPOLATE;

					targetY =
						( source->y[ind2]*(count - ind1*(DECUMA_INT32)ACCURACY_IN_INTERPOLATE) +
						  source->y[ind1]*(ind2*(DECUMA_INT32)ACCURACY_IN_INTERPOLATE - count)
						) / ACCURACY_IN_INTERPOLATE;
					
					decumaAssert(targetX <= MAX_DECUMA_INT16 && targetX >= MIN_DECUMA_INT16);
					decumaAssert(targetY <= MAX_DECUMA_INT16 && targetY >= MIN_DECUMA_INT16);
					target->x[j] = (DECUMA_INT16)targetX; /* Safe because of asserts above */
					target->y[j] = (DECUMA_INT16)targetY; /* Safe because of asserts above */
				}
				else
				{
					target->x[j] = source->x[ind1];
					target->y[j] = source->y[ind1];
				}
			}
			else
			{
				target->x[j] = source->x[NUMBER_OF_POINTS_IN_ARC - 1];
				target->y[j] = source->y[NUMBER_OF_POINTS_IN_ARC - 1];
			}
		}
		count += delta;

	}
}

void buildCurveMutation(SCR_CURVE * psi, const int measureId, const DECUMA_INT8 * arcOrder)
{
	int a;

	int nTotalPoints = curveGetTotalLength(psi);
	DECUMA_INT16 midNumbers[curveGetArcLength(psi, a)];

	/* Copy all the curve data to the new arcs. */
	for(a = 0 ; a < psi->noArcs ;  a++)
	{
		int nStartPoint=getArcNr(arcOrder,a) * curveGetArcLength(psi, a);

		/* Get the measure id subset that shall be used for this particular arc. */
		GetMeasureSection(measureId, midNumbers, nTotalPoints, nStartPoint, curveGetArcLength(psi, a));

		/* Recalculate all the metrics for this arc. */
		arcPrecalculate(psi->Arcs[a], midNumbers);
	}

	/* And now it is time to sum all the arcs into one curve. */
	preCalculateCurveFromArcData(psi, measureId);

}

void curveGetProp(const SCR_CURVE * pCurve, SCR_CURVE_PROP * pCurveProp, int nBaseArcs)
{
	pCurveProp->noArcs = (DECUMA_INT8)pCurve->noArcs;
	pCurveProp->noBaseArcs = (DECUMA_INT8)nBaseArcs;

	pCurveProp->offset_x = pCurve->offset_x;
	pCurveProp->offset_y = pCurve->offset_y;
	pCurveProp->right_shift = pCurve->right_shift;

	pCurveProp->mass = pCurve->mass;
	pCurveProp->norm = pCurve->norm;

	curveMinMaxCalculate(pCurve, pCurveProp->noArcs, &pCurveProp->min_x, &pCurveProp->max_x, &pCurveProp->min_y, &pCurveProp->max_y);
	curveMinMaxCalculate(pCurve, pCurveProp->noBaseArcs, &pCurveProp->base_min_x, &pCurveProp->base_max_x, &pCurveProp->base_min_y, &pCurveProp->base_max_y);
}
