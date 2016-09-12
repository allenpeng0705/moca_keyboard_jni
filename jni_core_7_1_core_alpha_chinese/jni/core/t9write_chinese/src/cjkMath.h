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

#pragma once

#ifndef CJK_MATH_H_
#define CJK_MATH_H_

#include "dltConfig.h"
#include "cjkCommon.h"
#include "cjkSession.h"
#include "decuma_point.h"

#if defined DECUMA_USE_STDLIB_MATH && defined DECUMA_NO_STDLIB_MATH
	#error Cannot both use stdlib math and not use it
#elif defined DECUMA_USE_STDLIB_MATH
	#include <math.h>
	#define dltExp exp
#elif defined DECUMA_NO_STDLIB_MATH
	#define dltExp __ieee754_exp

	DECUMA_HWR_PRIVATE double __ieee754_exp(double x);
#else
	#error You need to defined either DECUMA_USE_STDLIB_MATH or DECUMA_NO_STDLIB_MATH
#endif /* DECUMA_USE_STDLIB_MATH */

/**
 * @defgroup DOX_CJK_MATH cjkMath
 * @{
 */

/** return values of cjkMathLinesIntersect */
typedef enum {
	DONT_INTERSECT   = 0,
	DO_INTERSECT     = 1,
	COLLINEAR        = 2
} CJK_LINE_INTERSECTION_TYPE;

/**
 * This function takes an array of DECUMA_POINT  and deletes all points
 * that it does not consider necessary. Note that the function
 * writes the result into the same array. The number of resulting
 * points is the return value.
 * 
 * The entry function writes the first point, then walks the tree
 * by calling a recursive function, and then writes the last point.
 * 
 * The variable [[k]] will be the next index to be written all
 * the time.
 */
DECUMA_HWR_PRIVATE DECUMA_INT32 cjkMathFindPoints(DECUMA_POINT * pOutPoints,
					    const DECUMA_POINT * pInPoints, 
					      	  int nInPoints, 
					          DECUMA_UINT32 * pStack, 
					      	  int nStackSize, 
					          DECUMA_UINT32 * pBuffer, 
					          int nBufferSize,
						  int bUseLowpass);

DECUMA_HWR_PRIVATE DECUMA_INT32 cjkMathRemovePoints(DECUMA_UINT8 * cchardata,
								 DECUMA_INT32   nPoints, 
								 DECUMA_INT32   nStrokes);

DECUMA_HWR_PRIVATE DECUMA_INT32 cjkMathCalcStrokeLength(DECUMA_UINT8 * cchardata);

DECUMA_HWR_PRIVATE CJK_LINE_INTERSECTION_TYPE
cjkMathLinesIntersect(DECUMA_INT32   x1, DECUMA_INT32   y1,
					  DECUMA_INT32   x2, DECUMA_INT32   y2,
					  DECUMA_INT32   x3, DECUMA_INT32   y3,
					  DECUMA_INT32   x4, DECUMA_INT32   y4,
					  DECUMA_INT32 * x,  DECUMA_INT32 * y);


DECUMA_HWR_PRIVATE DECUMA_INT32
cjkMathPointOnWhatSide(DECUMA_INT32 x1, DECUMA_INT32 y1,
					   DECUMA_INT32 x2, DECUMA_INT32 y2, 
					   DECUMA_INT32 x3, DECUMA_INT32 y3);

#endif /* CJK_MATH_H_ */
