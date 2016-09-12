/*****************************************************************************
* COPYRIGHT ZI AND SUBJECT TO CONFIDENTIALITY RESTRICTIONS                   *
*                                                                            *
* This file is the confidential and proprietary property of Zi Corporation   *
* of Canada, Inc. ("Zi") or one of its affiliates.                           *
******************************************************************************/

#pragma once 

#ifndef _COARSE_FEATURES_H_
#define _COARSE_FEATURES_H_

typedef struct _tagCJK_COARSE_FEATURES_SCRATCHPAD CJK_COARSE_FEATURES_SCRATCHPAD;

#include "cjkCommon.h"
#include "cjkCompressedCharacter.h"
#include "cjkSession.h"

#include "decumaFeature.h"
#include "decumaCurve.h"
#include "decumaStatus.h"


/**
 * @defgroup DOX_CJK_COARSEFEATURES cjkCoarseFeatures
 * @{
 */

/**
 * @file
 * Currently this file has been implemented based on the 
 * @ref CJK_COMPRESSED_CHAR format as specified and used by the DLTLIB engine.
 * It also makes use of other internal functions such as the @ref CJK_GRIDPOINT
 * datatype and its associated functions. The DLTLIB @ref CJK_SESSION object is
 * also called since it contains necessary information for some of the
 * character operations.
 */

/** @hideinitializer 
* The number of binary features available in this module 
*/
#define COARSE_NBR_FEATURES    153

#define CJK_COARSE_FEATURE_NOT_SET(pFeature, i) (pFeature[i] < 0)

typedef struct _tagCOARSE_SETTINGS {
	DECUMA_INT16 ymax;
	DECUMA_INT16 ymin;
	DECUMA_INT16 xmax;
	DECUMA_INT16 xmin;
} COARSE_SETTINGS;

/* Levels for curvature in normalized radians [-127,127] */
#define CJK_COARSE_FLAT_CURVE_LIMIT 10
#define CJK_COARSE_SHARP_CURVE_LIMIT 100

/* Get the size of the heap memory needed for cjkCoarseFeatures internal calculations */

DECUMA_HWR_PRIVATE DECUMA_UINT32 cjkCoarseFeaturesGetScratchpadSize(void);

/* Setting the feature values that are located in the CJK_COMPRESSED_CHAR struct */

DECUMA_HWR_PRIVATE void dltCoarseSetOldDecumaFeatures(CJK_COMPRESSED_CHAR * pCChar,
											  CJK_SESSION * pSession);


/**
* @param   pFeatureElements   A vector stating the features to be calculated
*                             for the particular
* @param   nFeatureElements   The number of 32-bit elements containing 
*                             features to be extracted
* @param   pCchar             The character from which the features should be extracted
* @param   pSession           The DLTLIB session object
* @param   pSettings          The settings including box-size for the extraction 
*                             process.
*
* @returns  The number of features treated and added to the pFeatureElements. This
*           should be identical to the number of bits set in pFeatureChoices.
*/

DECUMA_HWR_PRIVATE DECUMA_STATUS cjkGetCoarseFeatures(DECUMA_FEATURE       * pFeatures,
								   DECUMA_UINT32          nFeatures,
								   CJK_COMPRESSED_CHAR  * pChar,
								   CJK_SESSION          * pSession,
								   COARSE_SETTINGS      * pSettings);


#endif /* _COARSE_FEATURES_H_ */
