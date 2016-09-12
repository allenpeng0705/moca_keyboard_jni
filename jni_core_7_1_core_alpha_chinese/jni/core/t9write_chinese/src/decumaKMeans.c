/*****************************************************************************
* COPYRIGHT ZI AND SUBJECT TO CONFIDENTIALITY RESTRICTIONS                   *
*                                                                            *
* This file is the confidential and proprietary property of Zi Corporation   *
* of Canada, Inc. ("Zi") or one of its affiliates.                           *
******************************************************************************/

#include "decumaKMeans.h"
#include "decumaMemory.h"
#include "decumaAssert.h"

/* -------------------------------------------------------------------------
 * Local functions
 * ------------------------------------------------------------------------- */

static int associateValues(int K, K_MEANS_CLUSTER * pClusters, K_MEANS_SESSION * pSession)
{
	int i, j, dist , nMinIdx, nMinDist, nChanged = 0;
	/* Start by resetting index memory in clusters for indexes */
	for (j=0; j<K; j++) {
		pClusters[j].nIdx = 0;
	}
	decumaAssert(pSession->pCluster != NULL);
	for (j=0; j<pSession->nValues; j++) {
		dist = pSession->fpDist(pClusters[0].pCenter, kMeansGetValue(pSession->pValues, j));
		nMinIdx = 0;
		nMinDist = dist;
		for (i=1; i<K; i++) {
			dist = pSession->fpDist(pClusters[i].pCenter, kMeansGetValue(pSession->pValues, j));
			if (dist < nMinDist) {
				nMinIdx = i;
				nMinDist = dist;
			}
		}
		/* Set Idx to the closest center */
		pClusters[nMinIdx].pIdx[pClusters[nMinIdx].nIdx++] = j;
		if (pSession->pCluster[j] != nMinIdx) { 
			nChanged++;
			pSession->pCluster[j] = nMinIdx;
		}
	}
	return nChanged;
}

static void recalculateCenters(int K, K_MEANS_CLUSTER * pClusters, K_MEANS_SESSION * pSession)
{
	int i;
	
	decumaAssert(pSession->pValues != NULL);
	/* Need to set values from idx's to means function */
	for (i=0; i<K; i++) {
		if (pClusters[i].nIdx > 0) /* else reuse the old one */
			pSession->fpCalculateCenter(pClusters[i].nIdx, pClusters[i].pIdx, pSession->pValues, pClusters[i].pCenter);
	}
}

/* -------------------------------------------------------------------------
 * Exported functions
 * ------------------------------------------------------------------------- */

void decumaKMeansSessionInit(K_MEANS_SESSION * pSession, 
							 K_MEANS_CENTER  * fpCalculateCenter,
							 K_MEANS_DIST    * fpDist,
							 K_MEANS_VALUE	 * pnStartValues,
							 int			   nValues,
							 K_MEANS_VALUE	 * pValues,
							 int 			 * pCluster, 
							 int			   nBreakCriteriaValue, 
							 int			   nMaxRepetitions)
{
	pSession->fpCalculateCenter = fpCalculateCenter;
	pSession->fpDist = fpDist;
	pSession->pnStartValues = pnStartValues;
	pSession->nValues = nValues;
	pSession->pValues = pValues;
	pSession->pCluster = pCluster; 
	pSession->nBreakCriteriaValue = nBreakCriteriaValue;
	pSession->nMaxRepetitions = nMaxRepetitions;
}


/* A little module for K-Means clustering. */

int decumaKMeans(int K, K_MEANS_CLUSTER * pClusters, K_MEANS_SESSION * pSession)
{
	int j, nChanged = 1;
	int nRepetitions = 0;
	/* Initiate */
	decumaAssert(pSession->pnStartValues != NULL);
	for (j=0; j<K; j++) {
		pClusters[j].pCenter = kMeansGetValue(pSession->pnStartValues, j);
		decumaAssert(pClusters[j].pIdx != NULL);
	}

	/* Repeat until breakcriteria is met */
	while (nChanged > pSession->nBreakCriteriaValue && nRepetitions < pSession->nMaxRepetitions) {
		/* Associate each value to closest cluster */
		nChanged = associateValues(K, pClusters, pSession);
		/* Recalculate centers */
		recalculateCenters(K, pClusters, pSession);
		
		nRepetitions++;
	}
	return nChanged;
}
