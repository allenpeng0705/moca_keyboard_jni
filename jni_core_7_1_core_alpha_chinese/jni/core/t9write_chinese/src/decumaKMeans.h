/*****************************************************************************
* COPYRIGHT ZI AND SUBJECT TO CONFIDENTIALITY RESTRICTIONS                   *
*                                                                            *
* This file is the confidential and proprietary property of Zi Corporation   *
* of Canada, Inc. ("Zi") or one of its affiliates.                           *
******************************************************************************/

#ifndef _DECUMA_KMEANS_H_
#define _DECUMA_KMEANS_H_

#include "decumaConfig.h"

typedef struct _tagK_MEANS_VALUE K_MEANS_VALUE;

typedef void (K_MEANS_CENTER)(int nIdx, int * pIdx, K_MEANS_VALUE * pValues, K_MEANS_VALUE * pCenter);
typedef int (K_MEANS_DIST)(K_MEANS_VALUE * nVal1, K_MEANS_VALUE * nVal2);

typedef struct _tagK_MEANS_SESSION {
	K_MEANS_CENTER  * fpCalculateCenter;
	K_MEANS_DIST    * fpDist;
	K_MEANS_VALUE	* pnStartValues;
	int				  nValues;
	K_MEANS_VALUE	* pValues; /* No allocation is done in this module */
	int 			* pCluster; /* Same size as pValues with cluster belonging */
	int				  nBreakCriteriaValue; /* TODO */
	int				  nMaxRepetitions;
} K_MEANS_SESSION;

typedef struct _tagK_MEANS_CLUSTER {
	int				* pIdx; /* No allocation is done in this module */
	int				  nIdx;
	K_MEANS_VALUE   * pCenter;
} K_MEANS_CLUSTER;

DECUMA_HWR_PRIVATE K_MEANS_VALUE * kMeansGetValue(K_MEANS_VALUE * pArray, int idx);

DECUMA_HWR_PRIVATE void decumaKMeansSessionInit(K_MEANS_SESSION * pSession,
							 K_MEANS_CENTER  * fpCalculateCenter,
							 K_MEANS_DIST    * fpDist,
							 K_MEANS_VALUE   * pnStartValues,
							 int			   nValues,
							 K_MEANS_VALUE	 * pValues,
							 int 			 * pCluster, 
							 int			   nBreakCriteriaValue, 
							 int			   nMaxRepetitions);

/* A little module for K-Means clustering. */

DECUMA_HWR_PRIVATE int decumaKMeans(int K, K_MEANS_CLUSTER * pClusters, K_MEANS_SESSION * pSession);

#endif /* _DECUMA_KMEANS_H_ */
