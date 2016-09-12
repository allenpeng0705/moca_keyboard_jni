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


#include "decumaBasicTypesMinMax.h"
#include "decumaMath.h"
#include "decumaResamp.h"
#include "decumaAssert.h"

#undef DO_LOGGING
#ifdef DO_LOGGING
#include "t9write_log.h"
#endif

/** resampArc resamples an arc with fixed arc length.

	pPoints
		The input arc points.

	nPoints
		The number of arc points in pPoints.

	pArc
		The output arc.

	nPointsInArc
  		Desired number of points in arc (must be allocated).

	nOffsetX
	    see below

	nOffsetY
		see below

		All coordinates pointed to by ppPoint are
		repositioned:
		nOffsetX is added to all x values and
		nOffsetY to all y values.

*/

DECUMA_STATUS resampArc(const DECUMA_POINT * pPoints, int nPoints, INT16_POINT *pArc, int nPointsInArc,
			   DECUMA_INT32 nOffsetX, DECUMA_INT32 nOffsetY)
{
	/*Computes arc length parametrisation of the curve pPoints */
	/*and puts in pTargetX and pTargetY */

#define arcLength_EPS (1)
#define SCALE_FACTOR 100

	int j,k; /* counters */

	DECUMA_INT32 /*INT16 */  lengthP; /*length of arc in p. */
	DECUMA_INT32 /*INT16 */  lengthQ, d1, d2; /*Keeps track of lengths. */
	DECUMA_INT32 /*INT16 */  scaledLengthQ; /* vma This has been observed at 15000 */
	DECUMA_INT32 overflowLimit = MAX_DECUMA_INT32/ (nPointsInArc + 1);

	decumaAssert( pPoints );
	decumaAssert( pArc );
#ifdef _DEBUG
	for(j = 0; j < nPoints ; j++)
	{
		decumaAssert(pPoints[j].x + nOffsetX <= MAX_DECUMA_INT16 &&
			pPoints[j].x + nOffsetX >= MIN_DECUMA_INT16);
		decumaAssert(pPoints[j].y + nOffsetY <= MAX_DECUMA_INT16 &&
			pPoints[j].y + nOffsetY >= MIN_DECUMA_INT16);
	}
#endif

	if(nPoints == 1)
	{
		/*Set all points in this arc on q to the */
		/*start point in p for this arc. */
		DECUMA_INT16 x0  = (DECUMA_INT16) (pPoints[0].x + nOffsetX); /* Cast asserted by loop in beginning of function */
		DECUMA_INT16 y0  = (DECUMA_INT16) (pPoints[0].y + nOffsetY);/* Cast asserted by loop in beginning of function */

		for(j = 0; j < nPointsInArc ; j++)
		{
			pArc[j].x = x0;
			pArc[j].y = y0;
		}
	}
	else
	{
		/*Compute the length of arc in p. */

		scaledLengthQ = lengthP = lengthQ = d1 = d2 = 0;

		for(j = 0; j < nPoints-1; j++)
		{
			DECUMA_INT32 d = normFine( ((DECUMA_INT32)pPoints[j+1].x - pPoints[j].x)*SCALE_FACTOR, ((DECUMA_INT32)pPoints[j+1].y - pPoints[j].y)*SCALE_FACTOR);

			if (overflowLimit - lengthP < d) return decumaArcTooLong;

			lengthP += d;
		}

		k = 0;

		for(j = 0; j < nPointsInArc  ; j++)
		{
			/*lengthQ = (scaledLengthQ + (nPointsInArc-1)/2)/(nPointsInArc -1) ; */
			lengthQ = scaledLengthQ /(nPointsInArc -1) ; /*This is incorrect rounding */
				/* but the incorrectness is compensated by always adding last point */

			/* Find surronding points */
			while((d2  <= lengthQ ) && (k < nPoints-1))
			{
				DECUMA_INT32 d = normFine( ((DECUMA_INT32)pPoints[k].x - pPoints[k+1].x)*SCALE_FACTOR,
					((DECUMA_INT32)pPoints[k].y - pPoints[k+1].y)*SCALE_FACTOR);

				d1 = d2;
				d2 += d;
				k++;
#ifdef DO_LOGGING
			logVariable32("d2",d2);
#endif
			}

			if(decumaAbs(d1 - d2) < arcLength_EPS)
			{
				/*Two points coincide */
				decumaAssert( k > 0 );
				pArc[j].x = (DECUMA_INT16) (pPoints[k-1].x + nOffsetX ); /* Cast asserted by loop in beginning of function */
				pArc[j].y = (DECUMA_INT16) (pPoints[k-1].y + nOffsetY );/* Cast asserted by loop in beginning of function */
			}
			else
			{
				/*Interpolate between two points. */


				pArc[j].x = (DECUMA_INT16) (
					(  ((pPoints[k-1].x + nOffsetX)*(d2 - lengthQ) +
					   (pPoints[k].x + nOffsetX)*(lengthQ - d1) + (d2-d1)/2)
					) / (d2-d1) );

				pArc[j].y = (DECUMA_INT16) (
					(  ((pPoints[k-1].y + nOffsetY)*(d2 - lengthQ) +
					   (pPoints[k].y + nOffsetY)*(lengthQ - d1)+ (d2-d1)/2)
					) / (d2-d1) );
			}
			scaledLengthQ += lengthP;
		}
		pArc[nPointsInArc-1].x = (DECUMA_INT16)( pPoints[nPoints-1].x + nOffsetX ); /* Cast asserted by loop in beginning of function */
		pArc[nPointsInArc-1].y = (DECUMA_INT16)( pPoints[nPoints-1].y + nOffsetY ); /* Cast asserted by loop in beginning of function */
	}
	return decumaNoError;
}
#undef arcLength_EPS

