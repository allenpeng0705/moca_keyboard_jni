/*****************************************************************************
* COPYRIGHT ZI AND SUBJECT TO CONFIDENTIALITY RESTRICTIONS                   *
*                                                                            *
* This file is the confidential and proprietary property of Zi Corporation   *
* of Canada, Inc. ("Zi") or one of its affiliates.                           *
******************************************************************************/

#pragma once

#ifndef CJK_STROKE_H_
#define CJK_STROKE_H_

#include "dltConfig.h"

#include "cjkCommon.h"
#include "cjkSession.h"

/** @defgroup DOX_CJK_STROKE cjkStroke
 *  @{
 */

/**
 * An Stdata vector starts with a byte that contains the number of
 * gridpoints and then the gridpoints follow immediately in an array.
 */
typedef const DECUMA_UINT8 Stdata;

/**
 * A stroke in a CJK_COMPRESSED_CHARACTER.
 */
typedef struct _tagCJK_STROKE {
	Stdata * stdata;
	Stdata * end;
	Stdata * stdata_backup;
	Stdata * end_backup;
} CJK_STROKE;



/**
 * Sets the strokes' fields to zero. One reason is to make it
 * possible to test if the stroke exists.
 */
DECUMA_HWR_PRIVATE void cjkStrokeInit(CJK_STROKE * pStroke);

/**
 * Computes the distance between two strokes by using DTW. There is no 
 * limitation to a band diagonal since the number of points in a stroke 
 * is usually very small.
 */
DECUMA_HWR_PRIVATE CJK_DISTANCE cjkStrokeGetDistanceDTW(CJK_STROKE * pStroke1,
									 CJK_STROKE * pStroke2, 
									 DECUMA_INT32 delta);

/** Used in @ref cjkBestListSpecialCheck to examine diacritic symbols. */
DECUMA_HWR_PRIVATE DECUMA_INT32 cjkStrokeIsSmall(CJK_STROKE * pStroke);

/** Used in @ref cjkBestListSpecialCheck to examine diacritic symbols. */
DECUMA_HWR_PRIVATE DECUMA_INT32 cjkStrokeIsClose(CJK_STROKE * pStroke1, CJK_STROKE * pStroke2);

/** 
 * Takes a stroke and returns the gridpoint that is the closest one to the 
 * first gridpoint, except from the njump closest on the curve.
 */
DECUMA_HWR_PRIVATE const CJK_GRIDPOINT * cjkStrokeGetGapGridpoint(CJK_STROKE  * pStroke,
											   DECUMA_UINT8  njump, 
											   CJK_SESSION * pSession);

/** 
* @returns the @ref Gridpoint in the @ref CJK_STROKE with min x-value.
* Used in @ref cjkBestListSpecialCheck to separate similar characters from
* each other.
*/ 
DECUMA_HWR_PRIVATE CJK_GRIDPOINT cjkStrokeGetMinXGridpoint(CJK_STROKE * pStroke);

/** 
* @returns the @ref Gridpoint in the @ref CJK_STROKE with min y-value.
* Used in @ref cjkBestListSpecialCheck to separate similar characters from
* each other.
*/ 
DECUMA_HWR_PRIVATE CJK_GRIDPOINT cjkStrokeGetMinYGridpoint(CJK_STROKE * pStroke);

/** 
* @returns the @ref Gridpoint in the @ref CJK_STROKE with max x-value.
* Used in @ref cjkBestListSpecialCheck to separate similar characters from
* each other.
*/
DECUMA_HWR_PRIVATE CJK_GRIDPOINT cjkStrokeGetMaxXGridpoint(CJK_STROKE * pStroke);

/** 
* @returns the @ref Gridpoint in the @ref CJK_STROKE with max x-value.
* Used in @ref cjkBestListSpecialCheck to separate similar characters from
* each other.
*/ 
DECUMA_HWR_PRIVATE CJK_GRIDPOINT cjkStrokeGetMaxYGridpoint(CJK_STROKE * pStroke);

/**
* @returns a pointer to the gridpoint at pos. Negative positions
* are treated like this: minus one is the last point, minus two is the
* next to last point and so on. The first element is a positive one.
* The function should not be given a zero. This is an unchecked error.
*/
DECUMA_HWR_PRIVATE const CJK_GRIDPOINT * cjkStrokeGetGridpoint(CJK_STROKE * pStroke,
											DECUMA_INT32 pos);

DECUMA_HWR_PRIVATE void cjkStrokeNext(CJK_STROKE * pStroke, CJK_SESSION * pSession);

/**
* Takes a stroke and returns how many loops there was, i.e. how many
* intersections the stroke had with itself.
*/ 
DECUMA_HWR_PRIVATE DECUMA_INT32 cjkStrokeGetIntersectionCount(CJK_STROKE  * pStroke,
										   CJK_SESSION * pSession);

/** @} */

#endif /* CJK_STROKE_H_ */
