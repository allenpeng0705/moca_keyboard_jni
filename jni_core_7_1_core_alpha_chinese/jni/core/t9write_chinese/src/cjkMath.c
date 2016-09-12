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

#include "cjkMath.h"
#include "cjkSession_Types.h"
#include "cjkCompressedCharacter_Macros.h"
#include "decumaQsort.h"
#include "decumaMemory.h"
#include "decumaCommon.h"

#define SAME_SIGNS( a, b ) \
	(((DECUMA_INT32) ((DECUMA_UINT32) a ^ (DECUMA_UINT32) b)) >= 0 )

/* TODO see if performance improves by using direct pointers, i.e. 
 *		static DECUMA_POINT * findImportantPoint(const DECUMA_POINT * pFirst, const DECUMA_POINT * pLast);
 */
static int findImportantPoint(const DECUMA_POINT * pPoints,
		int i1, int i2, int nPoints, int bUseLowpass);

static int compareLess(const void * a, const void * b) {
	const DECUMA_UINT32 * intA = a;
	const DECUMA_UINT32 * intB = b;
	return *intA - *intB;
}

/**
 * cjkMathFindPoints is split into two parts. The first part is filling a buffer
 * with ones in indexes with interesting points. This is not so memory 
 * efficient, but the code is more readable this way. The second part scans
 * through the buffer and copies the important points from v into v.
 */

DECUMA_INT32 cjkMathFindPoints(DECUMA_POINT * pOutPoints,
		const DECUMA_POINT * pInPoints, int nInPoints, DECUMA_UINT32 * pStack,
		int nStackSize, DECUMA_UINT32 * pBuffer, int nBufferSize, int bUseLowpass) {
	/* Please note that there is NO safety with regards to the length of pOutPoints. The only entry to this function 
	 * is in cjkCompressedCharacter:cc_compress_dostuff(), where we pass a pointer declared with the same length as 
	 * pBuffer (scratchpad.dltCCharCompress.mt_fintpts_buffer)
	 */

	const int firstindex = 0, lastindex = nInPoints - 1;
	int middleindex, leftindex = 0;
	int i;
	int nSubsampledPoints = 0;
	int nStackCounter = 0;

	DECUMA_UNUSED_PARAM(nStackSize); /* Used only in assert, adding UNUSED macro to remove warning in release builds */

	decumaAssert( pInPoints );
	decumaAssert( nInPoints > 2 );
	decumaAssert( pOutPoints );
	decumaAssert( pBuffer );
	decumaAssert( pStack );

	/* memset the buffer to zero */
	decumaMemset(pBuffer, 0, nBufferSize * sizeof(pBuffer[0]));

	/* set the first and the last point */
	pBuffer[0] = firstindex;
	pBuffer[1] = lastindex;
	nSubsampledPoints = 2;

	/* first calculation of important middle point */
	middleindex = findImportantPoint(pInPoints, firstindex, lastindex, nInPoints, bUseLowpass);

	if (middleindex) {
		decumaAssert(nStackCounter < nStackSize);
		pStack[nStackCounter++] = lastindex;
		pBuffer[nSubsampledPoints++] = middleindex;
	} else {
		/* straight line, only two points. */
		pOutPoints[0] = pInPoints[firstindex];
		pOutPoints[1] = pInPoints[lastindex];
		return 2;
	}

	/* outer loop: every time the leftindex is moved forward. */
	for (;;) {
		/* inner loop: divide and conquer, push middle point on the stack */
		/* if one is found, otherwise break */
		while (middleindex) {
			decumaAssert(nStackCounter < nStackSize);
			pStack[nStackCounter++] = middleindex;
			middleindex = findImportantPoint(pInPoints, leftindex, middleindex, nInPoints, bUseLowpass);
			if (middleindex != 0) {
				if (nSubsampledPoints >= nBufferSize) {
					/* TODO Return input too large 
					 * This means refactoring the call stack such that we can propagate that information back up to cc_compress_dostuff() and, ultimately, decumaRecognize()
					 */
				}
				decumaAssert(nSubsampledPoints < nBufferSize);
				pBuffer[nSubsampledPoints++] = middleindex;
			}
		}

		/* the next left most point is now on the stack. pop it. */
		leftindex = pStack[--nStackCounter];

		if (nStackCounter == 0) {
			/* we are done */
			break;
		}

		decumaAssert(nStackCounter > 0 );
		if (nStackCounter > 0) {
			middleindex = pStack[--nStackCounter];
		}
	}

	/* last: write the important points to the output buffer */
	/* and return number of points written. */
	decumaQsort(pBuffer, nSubsampledPoints, sizeof(pBuffer[0]), &compareLess);
	for (i = 0; i < nSubsampledPoints; i++) {
		if (bUseLowpass && pBuffer[i] > 0 && pBuffer[i] < nInPoints - 1) {
		
			pOutPoints[i].x = (pInPoints[pBuffer[i]-1].x + 4*pInPoints[pBuffer[i]].x + pInPoints[pBuffer[i]+1].x)/6;
			pOutPoints[i].y = (pInPoints[pBuffer[i]-1].y + 4*pInPoints[pBuffer[i]].y + pInPoints[pBuffer[i]+1].y)/6;
		}
		else {
			pOutPoints[i] = pInPoints[pBuffer[i]];
		}
	}

	return nSubsampledPoints;
}

/**
 * This function is a complement to [[cjkMathFindPoints]] but for curvy characters.
 * Points that are too 'close' are removed. Two points are too close when their
 * distance is less or equal to 2, in the 16x16 grid.
 * Input: cchardata is a pointer to the first element in a vector of unsigned
 * chars containing the characters of the format
 * $(n_b,n_{p1},p_1,p_2, ...,n_{p2},p_1,p_2...)$, where $n_{pi}$ is the number
 * of points in stroke i and $n_b$ is the total number of bytes. Also the
 * number of points (nPoints) and the number of strokes (nStrokes) in the
 * character is needed.
 * Cchardata is recycled and overwritten. Also the function returns the new
 * number of points.
 * 
 * \f[ \epsfig{file=../../figures/0_removedpoints} \f]
 */
DECUMA_INT32 cjkMathRemovePoints(DECUMA_UINT8 * cchardata,
		DECUMA_INT32 nPoints, DECUMA_INT32 nStrokes) {
	int k, i, npointsinstroke, isfirstpoint, npointsind = 0;
	int insertind = 1;
	CJK_GRIDPOINT currentpoint = cchardata[0]; /*init just for removing compiler warning */
	CJK_GRIDPOINT prevpoint = cchardata[0]; /*init just for removing compiler warning */

	if (nPoints <= 2)
		return nPoints;
	k = 1;
	while (cchardata[k]) {

		/* remove points in stroke k */
		npointsinstroke = cchardata[k];
		isfirstpoint = 1;
		for (i = k + 1; i <= k + npointsinstroke; i++) {

			/* remove close points */
			if (isfirstpoint) {
				prevpoint = currentpoint = cchardata[i];
				npointsind = insertind++;
				isfirstpoint = 0;
			} else {
				currentpoint = cchardata[i];
				if (CJK_GP_GET_SQ_DISTANCE(prevpoint, currentpoint) <= 2)
					continue;
				cchardata[insertind++] = prevpoint; /* NOTE: write PREVIOUS! */
				prevpoint = currentpoint;
			}

		}
		cchardata[insertind++] = currentpoint; /* write last point in stroke! */
		cchardata[npointsind] = insertind - npointsind - 1;
		k = k + npointsinstroke + 1;
	}

	for (i = insertind; i < cchardata[0]; i++)
		cchardata[i] = 0;

	cchardata[0] = insertind;

	return insertind - nStrokes - 1;
}

/**
 * This function is intended for calculating a reference value for resampling of points when
 * the current sampling of the hardware is insufficient. This function calculates the max
 * gridpoint length of a stroke.
 */
DECUMA_INT32 cjkMathCalcStrokeLength(DECUMA_UINT8 * cchardata) {
	DECUMA_INT32 i, this_nbrpoints; /*, stroke_index, curr_stroke; */
	DECUMA_INT32 length = 0, dx, dy;
	this_nbrpoints = cchardata[0];

#if 0
	/* find this stroke index */
	curr_stroke = 1; stroke_index = 1;
	while(curr_stroke < this_stroke) {
		i = cchardata[stroke_index];
		stroke_index += i + 1;
		curr_stroke++;
	}

	this_nbrpoints = cchardata[stroke_index++];
#endif /* 0 */

	/* calculate length */
	if (this_nbrpoints > 1) {
		/*for (i = stroke_index + 1; i < this_nbrpoints; i++) { */
		for (i = 2; i <= this_nbrpoints; i++) {
			dx = ABS((cchardata[i] & 0x0F) - (cchardata[i - 1] & 0x0F));
			dy
					= ABS(((cchardata[i] & 0xF0) >> 4) - ((cchardata[i - 1] & 0xF0) >> 4));
			length += MAX(dx, dy);
		}
	} else {
		return 0;
	}

	return length;
}

/**
 *  This function was ripped off the net: \\
*  $http://www.acm.org/pubs/tog/GraphicsGems/category.html$
 *
 *  AUTHOR: Mukesh Prasad \\
*  This function computes whether two line segments,
 *  respectively joining the input points $(x_1,y_1) -- (x_2,y_2)$
 *  and the input points $(x_3,y_3) -- (x_4,y_4)$ intersect.
 *  If the lines intersect, the output variables x, y are
 *  set to coordinates of the point of intersection.
 *
 *  All values are in integers.  The returned value is rounded
 *  to the nearest integer point.
 *
 *  If non-integral grid points are relevant, the function
 *  can easily be transformed by substituting floating point
 *  calculations instead of integer calculations.
 *
 *  @param x1   X coordinate of endpoint 1 of segment 1
 *  @param y1   Y coordinate of endpoint 1 of segment 1
 *  @param x2   X coordinate of endpoint 2 of segment 1
 *  @param y2   Y coordinate of endpoint 2 of segment 1

 *  @param x3   X coordinate of endpoint 1 of segment 2
 *  @param y3   Y coordinate of endpoint 1 of segment 2
 *  @param x4   X coordinate of endpoint 2 of segment 2
 *  @param y4   Y coordinate of endpoint 2 of segment 2
 *
 *  @param[out] x X coordinate of intersection point
 *  @param[out] y Y coordinate of intersection point
 *
 *  @returns CJK_LINE_INTERSECTION_TYPE:
 *           @li DONT_INTERSECT
 *           @li DO_INTERSECT
 *           @li COLLINEAR
 *
 *  Error conditions:
 *
 *  Depending upon the possible ranges, and particularly on 16-bit
 *  computers, care should be taken to protect from overflow.
 *
 *  In the following code, 'long' values have been used for this
 *  purpose, instead of 'int'.
 */
CJK_LINE_INTERSECTION_TYPE cjkMathLinesIntersect(DECUMA_INT32 x1,
		DECUMA_INT32 y1, /* First line segment */
		DECUMA_INT32 x2, DECUMA_INT32 y2,

		DECUMA_INT32 x3, DECUMA_INT32 y3, /* Second line segment */
		DECUMA_INT32 x4, DECUMA_INT32 y4,

		DECUMA_INT32 * x, DECUMA_INT32 * y /* Output value:
 point of intersection */) {
	DECUMA_INT32 a1, a2, b1, b2, c1, c2; /* Coefficients of line eqns. */
	DECUMA_INT32 r1, r2, r3, r4; /* 'Sign' values */
	DECUMA_INT32 denom, offset, num; /* Intermediate values */

	/* Compute a1, b1, c1, where line joining points 1 and 2
	 * is "a1 x  +  b1 y  +  c1  =  0".
	 */

	a1 = y2 - y1;
	b1 = x1 - x2;
	c1 = x2 * y1 - x1 * y2;

	/* Compute r3 and r4.
	 */

	r3 = a1 * x3 + b1 * y3 + c1;
	r4 = a1 * x4 + b1 * y4 + c1;

	/* Check signs of r3 and r4.  If both point 3 and point 4 lie on
	 * same side of line 1, the line segments do not intersect.
	 */

	if (r3 != 0 && r4 != 0 && SAME_SIGNS( r3, r4 ))
		return DONT_INTERSECT;

	/* Compute a2, b2, c2 */

	a2 = y4 - y3;
	b2 = x3 - x4;
	c2 = x4 * y3 - x3 * y4;

	/* Compute r1 and r2 */

	r1 = a2 * x1 + b2 * y1 + c2;
	r2 = a2 * x2 + b2 * y2 + c2;

	/* Check signs of r1 and r2.  If both point 1 and point 2 lie
	 * on same side of second line segment, the line segments do
	 * not intersect.
	 */

	if (r1 != 0 && r2 != 0 && SAME_SIGNS( r1, r2 ))
		return DONT_INTERSECT;

	/* Line segments intersect: compute intersection point.
	 */

	denom = a1 * b2 - a2 * b1;
	if (denom == 0)
		return COLLINEAR;
	offset = denom < 0 ? -denom / 2 : denom / 2;

	/* The denom/2 is to get rounding instead of truncating.  It
	 * is added or subtracted to the numerator, depending upon the
	 * sign of the numerator.
	 */

	num = b1 * c2 - b2 * c1;
	*x = (num < 0 ? num - offset : num + offset) / denom;

	num = a2 * c1 - a1 * c2;
	*y = (num < 0 ? num - offset : num + offset) / denom;

	return DO_INTERSECT;
}

/**
 * This function takes a line segment (two Gridpoints) and a gridpoint, and
 * decides on what side of the line the point is. It returns 1 if the point is
 * on the left side of the line with the direction from (x1,y1)
 * to (x2,y2), and -1 for the right side. If the point was on the line it
 * returns 0.
 */
DECUMA_INT32 cjkMathPointOnWhatSide(DECUMA_INT32 x1, DECUMA_INT32 y1, /* line segment */
DECUMA_INT32 x2, DECUMA_INT32 y2, DECUMA_INT32 x3, DECUMA_INT32 y3 /* point */) {
	DECUMA_INT32 a1, b1, c1; /* Coefficients of line eq. */

	a1 = y2 - y1;
	b1 = x1 - x2;
	c1 = x2 * y1 - x1 * y2;

	return SIGN(a1 * x3 + b1 * y3 + c1);
}

/**
 * findImportantPoint returns an interesting point between i1 and i2, or
 * 0 if there was none.
 */
static int findImportantPoint(const DECUMA_POINT * pPoints,
		int start, int end, int nPoints, int bUseLowpass) {
	DECUMA_INT32 baselength, rx, ry;
	DECUMA_INT32 maxr, minr, maxproj;
	DECUMA_POINT dpStart, dpEnd;
	int i, maxi, mini;

	/*----------------------------------------------------- */
	/* return if no inner point */
	/*  */
	/* We skip the middle point if there is only one, i e if the length is */
	/* only three. */
	/*  */
	if (end - start < 3) {
		return 0;
	}

	/* nPoints req given by above end-start condition */
	if (bUseLowpass && start > 0) {
		dpStart.x = (pPoints[start-1].x + 4*pPoints[start].x + pPoints[start+1].x) / 6;
		dpStart.y = (pPoints[start-1].y + 4*pPoints[start].y + pPoints[start+1].y) / 6;
	}
	else {
		dpStart = pPoints[start];
	}
	if (bUseLowpass && end < nPoints-1) {
		dpEnd.x = (pPoints[end-1].x + 4*pPoints[end].x + pPoints[end+1].x) / 6;
		dpEnd.y = (pPoints[end-1].y + 4*pPoints[end].y + pPoints[end+1].y) / 6;
	}
	else {
		dpEnd = pPoints[end];
	}

	/*----------------------------------------------------- */
	/* compute begin-end vector data */
	/*  */
	/* The [[baselength]] is squared but the quantities [[maxr]] and */
	/* [[minr]] are also scaled with the same factor so we can view them as */
	/* non-squared quantities on the same scale. */

	rx = dpStart.x - dpEnd.x;
	ry = dpStart.y - dpEnd.y;
	baselength = rx * rx + ry * ry;

	/*----------------------------------------------------- */
	/* loop over inside points */
	/*  */
	/* The maximum distance between the smallest and largest inside point */
	/* projected on the line orthogonal to the line between the first and last */
	/* point is used for deciding if we should add a point. The point added is */
	/* the one that has the largest ABSolute projected length. Note that */
	/* [[i]] is guaranteed to be written. */

	minr = 1;
	maxr = -1;
	maxi = 0;
	mini = 0;
	for (i = start + 1; i < end; i++) {
		DECUMA_COORD x = (bUseLowpass ? (pPoints[i-1].x + 4*pPoints[i].x + pPoints[i+1].x)/6 : pPoints[i].x);
		DECUMA_COORD y = (bUseLowpass ? (pPoints[i-1].y + 4*pPoints[i].y + pPoints[i+1].y)/6 : pPoints[i].y);
		DECUMA_INT32 r = ry*(dpStart.x - x) + rx*(y - dpStart.y);

		if (r > maxr) {
			maxr = r;
			maxi = i;
		}
		if (r < minr) {
			minr = r;
			mini = i;
		}
	}
	maxproj = maxr - minr;
	i = ((maxr > -minr) ? maxi : mini);

	if (4 * maxproj < baselength) {
		/*----------------------------------------------------- */
		/* try sharp corners */
		/*  */
		/* If there are sharp corners in the stroke we may give them */
		/* a point too. The number 700 below means that angles less than */
		/* $180-\frac{360}{2\pi}\arccos(\sqrt(7/10)) = 147$ degrees. */
		/* A zero in the variable $i$ at exit of this block means that */
		/* no inner point was found worthy of inclusion in the privileged set, */
		/* otherwise $i$ is the index of the point to include. */

		minr = 1000;
		for (i = start + 2; i <= end - 2; i++) {
			DECUMA_COORD x1, x2, y1, y2;
			DECUMA_INT32 r1, r2, rr, r;
			
			/* We can not improve this process by removing one division since that would deviate from 4.1 implementation */
			if (bUseLowpass) {
				if (i > 2) {
					x1 = (pPoints[i-1].x + 4*pPoints[i].x + pPoints[i+1].x)/6 - (pPoints[i-3].x + 4*pPoints[i-2].x + pPoints[i-1].x)/6;
					y1 = (pPoints[i-1].y + 4*pPoints[i].y + pPoints[i+1].y)/6 - (pPoints[i-3].y + 4*pPoints[i-2].y + pPoints[i-1].y)/6;
				}
				else {
					x1 = (pPoints[i-1].x + 4*pPoints[i].x + pPoints[i+1].x)/6 - pPoints[i-2].x;
					y1 = (pPoints[i-1].y + 4*pPoints[i].y + pPoints[i+1].y)/6 - pPoints[i-2].y;
				}
				if (i < nPoints - 3) {
					x2 = (pPoints[i+1].x + 4*pPoints[i+2].x + pPoints[i+3].x)/6 - (pPoints[i-1].x + 4*pPoints[i].x + pPoints[i+1].x)/6;
					y2 = (pPoints[i+1].y + 4*pPoints[i+2].y + pPoints[i+3].y)/6 - (pPoints[i-1].y + 4*pPoints[i].y + pPoints[i+1].y)/6;
				}
				else {
					x2 = pPoints[i+2].x - (pPoints[i-1].x + 4*pPoints[i].x + pPoints[i+1].x)/6;
					y2 = pPoints[i+2].y - (pPoints[i-1].y + 4*pPoints[i].y + pPoints[i+1].y)/6;
				}
			}
			else {	
				x1 = pPoints[i].x - pPoints[i - 2].x;
				x2 = pPoints[i + 2].x - pPoints[i].x;
				y1 = pPoints[i].y - pPoints[i - 2].y;
				y2 = pPoints[i + 2].y - pPoints[i].y;
			}
			r = x1 * x2 + y1 * y2;
			rr = r * r;
			r1 = x1 * x1 + y1 * y1;
			r2 = x2 * x2 + y2 * y2;
			if (r1 > 5 && r2 > 5) {
				rr = (rr / r1) * 1000;
				rr = rr / r2;
				if (r < 0)
					rr = -rr;
				if (rr < minr) {
					minr = rr;
					mini = i;
				}
			}
		}
		i = ((minr < 700) ? mini : 0);
	}

	return i;
}

#if defined DECUMA_NO_STDLIB_MATH

/* This is copied from the BSD C library, file exp.c
 * The algorithm is unmodified, but the code is updated to Decuma coding standards.
 */

/*
 * ====================================================
 * Copyright (C) 2004 by Sun Microsystems, Inc. All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice
 * is preserved.
 * ====================================================
 */

/* __ieee754_exp(x)
 * Returns the exponential of x.
 *
 * Method
 *   1. Argument reduction:
 *      Reduce x to an r so that |r| <= 0.5*ln2 ~ 0.34658.
 *	Given x, find r and integer k such that
 *
 *               x = k*ln2 + r,  |r| <= 0.5*ln2.
 *
 *      Here r will be represented as r = hi-lo for better
 *	accuracy.
 *
 *   2. Approximation of exp(r) by a special rational function on
 *	the interval [0,0.34658]:
 *	Write
 *	    R(r**2) = r*(exp(r)+1)/(exp(r)-1) = 2 + r*r/6 - r**4/360 + ...
 *      We use a special Remes algorithm on [0,0.34658] to generate
 * 	a polynomial of degree 5 to approximate R. The maximum error
 *	of this polynomial approximation is bounded by 2**-59. In
 *	other words,
 *	    R(z) ~ 2.0 + P1*z + P2*z**2 + P3*z**3 + P4*z**4 + P5*z**5
 *  	(where z=r*r, and the values of P1 to P5 are listed below)
 *	and
 *	    |                  5          |     -59
 *	    | 2.0+P1*z+...+P5*z   -  R(z) | <= 2
 *	    |                             |
 *	The computation of exp(r) thus becomes
 *                             2*r
 *		exp(r) = 1 + -------
 *		              R - r
 *                                 r*R1(r)
 *		       = 1 + r + ----------- (for better accuracy)
 *		                  2 - R1(r)
 *	where
 *			         2       4             10
 *		R1(r) = r - (P1*r  + P2*r  + ... + P5*r   ).
 *
 *   3. Scale back to obtain exp(x):
 *	From step 1, we have
 *	   exp(x) = 2^k * exp(r)
 *
 * Special cases:
 *	exp(INF) is INF, exp(NaN) is NaN;
 *	exp(-INF) is 0, and
 *	for finite argument, only exp(0)=1 is exact.
 *
 * Accuracy:
 *	according to an error analysis, the error is always less than
 *	1 ulp (unit in the last place).
 *
 * Misc. info.
 *	For IEEE double
 *	    if x >  7.09782712893383973096e+02 then exp(x) overflow
 *	    if x < -7.45133219101941108420e+02 then exp(x) underflow
 *
 * Constants:
 * The hexadecimal values are the intended ones for the following
 * constants. The decimal values may be used, provided that the
 * compiler will convert from decimal to binary accurately enough
 * to produce the hexadecimal values shown.
 */

static const double one = 1.0;
static const double halF[2] = {0.5, -0.5,};
static const double hugeValue = 1.0e+300;
static const double twom1000 = 9.33263618503218878990e-302;    /* 2**-1000=0x01700000,0*/
static const double o_threshold = 7.09782712893383973096e+02;  /* 0x40862E42, 0xFEFA39EF */
static const double u_threshold = -7.45133219101941108420e+02; /* 0xc0874910, 0xD52D3051 */
static const double ln2HI[2] = { 6.93147180369123816490e-01,   /* 0x3fe62e42, 0xfee00000 */
							    -6.93147180369123816490e-01 }; /* 0xbfe62e42, 0xfee00000 */
static const double ln2LO[2] = { 1.90821492927058770002e-10,   /* 0x3dea39ef, 0x35793c76 */
								-1.90821492927058770002e-10,}; /* 0xbdea39ef, 0x35793c76 */
static const double invln2 = 1.44269504088896338700e+00;       /* 0x3ff71547, 0x652b82fe */
static const double P1 = 1.66666666666666019037e-01; 		   /* 0x3FC55555, 0x5555553E */
static const double P2 = -2.77777777770155933842e-03;		   /* 0xBF66C16C, 0x16BEBD93 */
static const double P3 = 6.61375632143793436117e-05;		   /* 0x3F11566A, 0xAF25DE2C */
static const double P4 = -1.65339022054652515390e-06;		   /* 0xBEBBBD41, 0xC5D26BF1 */
static const double P5 = 4.13813679705723846039e-08;		   /* 0x3E663769, 0x72BEA4D0 */

#if defined DECUMA_IEEE754_BIG_ENDIAN && defined DECUMA_IEEE754_LITTLE_ENDIAN
	#error Cannot target both big-endian and little-endian at once
#elif defined DECUMA_IEEE754_BIG_ENDIAN

typedef union
{
	double value;
	struct
	{
		DECUMA_UINT32 msw;
		DECUMA_UINT32 lsw;
	}parts;
} ieee_double_shape_type;

#elif defined DECUMA_IEEE754_LITTLE_ENDIAN

typedef union {
	double value;
	struct {
		DECUMA_UINT32 lsw;
		DECUMA_UINT32 msw;
	}parts;
} ieee_double_shape_type;

#else
	#error You need to specify either DECUMA_IEEE754_BIG_ENDIAN or DECUMA_IEEE754_LITTLE_ENDIAN
#endif

#define GET_HIGH_WORD(i,d) \
	do { \
		ieee_double_shape_type gh_u; \
		gh_u.value = (d); \
		(i) = gh_u.parts.msw; \
	} while (0)

#define GET_LOW_WORD(i,d) \
	do { \
		ieee_double_shape_type gl_u; \
		gl_u.value = (d); \
		(i) = gl_u.parts.lsw; \
	} while (0)

#define SET_HIGH_WORD(d,v) \
	do { \
	  ieee_double_shape_type sh_u; \
	  sh_u.value = (d); \
	  sh_u.parts.msw = (v); \
	  (d) = sh_u.value; \
	} while (0)

#define SET_LOW_WORD(d,v) \
	do { \
	  ieee_double_shape_type sl_u; \
	  sl_u.value = (d); \
	  sl_u.parts.lsw = (v); \
	  (d) = sl_u.value; \
	} while (0)

double __ieee754_exp(double x)
{
	double y;
	double hi = 0.0;
	double lo = 0.0;
	double c;
	double t;
	DECUMA_INT32 k = 0, xsb;
	DECUMA_UINT32 hx;

	GET_HIGH_WORD(hx,x);
	xsb = (hx >> 31) & 1; /* sign bit of x */
	hx &= 0x7fffffff; /* high word of |x| */

	/* filter out non-finite argument */
	if (hx >= 0x40862E42) { /* if |x|>=709.78... */
		if (hx >= 0x7ff00000) {
			DECUMA_UINT32 lx;
			GET_LOW_WORD(lx,x);
			if (((hx & 0xfffff) | lx) != 0)
			return x + x; /* NaN */
			else
			return (xsb == 0) ? x : 0.0; /* exp(+-inf)={inf,0} */
		}
		if (x > o_threshold)
			return hugeValue * hugeValue; /* overflow */
		if (x < u_threshold)
			return twom1000 * twom1000; /* underflow */
	}

	/* argument reduction */
	if (hx > 0x3fd62e42) { /* if  |x| > 0.5 ln2 */
		if (hx < 0x3FF0A2B2) { /* and |x| < 1.5 ln2 */
			hi = x - ln2HI[xsb];
			lo = ln2LO[xsb];
			k = 1 - xsb - xsb;
		} else {
			k = (int) (invln2 * x + halF[xsb]);
			t = k;
			hi = x - t * ln2HI[0]; /* t*ln2HI is exact here */
			lo = t * ln2LO[0];
		}
		x = hi - lo;
	} else if (hx < 0x3e300000) { /* when |x|<2**-28 */
		if (hugeValue + x > one)
			return one + x; /* trigger inexact */
	} else {
		k = 0;
	}

	/* x is now in primary range */
	t = x * x;
	c = x - t * (P1 + t * (P2 + t * (P3 + t * (P4 + t * P5))));

	if (k == 0) {
		return one - ((x * c) / (c - 2.0) - x);
	} else {
		y = one - ((lo - (x * c) / (2.0 - c)) - hi);
	}

	if (k >= -1021) {
		DECUMA_UINT32 hy;
		GET_HIGH_WORD(hy,y);
		SET_HIGH_WORD(y,hy+(k<<20)); /* add k to y's exponent */
		return y;
	} else {
		DECUMA_UINT32 hy;
		GET_HIGH_WORD(hy,y);
		SET_HIGH_WORD(y,hy+((k+1000)<<20)); /* add k to y's exponent */
		return y * twom1000;
	}
}

#endif /* DECUMA_NO_STDLIB_MATH */
