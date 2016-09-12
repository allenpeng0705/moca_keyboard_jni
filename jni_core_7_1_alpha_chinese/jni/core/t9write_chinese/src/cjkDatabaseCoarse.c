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


#include "cjkDatabase.h"
#include "cjkDatabaseCoarse.h"
#include "cjkDatabaseCoarse_Macros.h"
#include "cjkDatabaseCoarse_Types.h"

#include "cjkDatabaseFormat.h"
#include "cjkDatabaseFormat_Macros.h"

#include "cjkSession_Types.h"

#include "cjkCompressedCharacter_Macros.h"

#include "decumaInterrupts.h"

#include "decumaQsort.h"
#include "decumaStatus.h"
#include "decumaAssert.h"
#include "decumaCommon.h"
#include "decumaMemory.h"

#include "cjkMath.h"

static int 
comparator_beamNode_desc(const void * a, const void * b);



static int 
comparator_uint16_desc(const void * a, const void * b);


void 
cjkDbCoarseResultsRemoveDuplicates(DLTDB_COARSE_SEARCH_RESULT * pResult)
{

	decumaQsort(pResult, pResult->nIndices, sizeof(pResult->pIndices[0]), comparator_uint16_desc);

	{ 
		int r = 1, w = 1;
		while (r < pResult->nIndices) {
			if (pResult->pIndices[w - 1] != pResult->pIndices[r]) {
				pResult->pIndices[w] = pResult->pIndices[r];
				w++;
			}
			r++;
		}
		pResult->nIndices = MIN(pResult->nIndices, w);
	}
}


DECUMA_UINT32 cjkDbCoarseGetScratchpadSize(void)
{
	return sizeof(CJK_DB_COARSE_SCRATCHPAD);
}

/* -------------------------------------------------------------------------
   BEAM SEARCH IMPLEMENTATION

   TODO refactor cluster node index collection in cjkDbCoarseBinarySearch() to a
        separate function, use in cjkDbCoarseBeamSearch().

 * ------------------------------------------------------------------------- */

double 
getCoarseDistanceToClusterRep_DCS(DLTDB_COARSE_SEARCH_SESSION * pCSSession,
                                  int                           nClusterIndex) 
{
	DLTDB_NAMED_INDEX_NODE * pCluster        = DLTDB_DCSCLUSTER_GET_CLUSTER(&pCSSession->pSession->db, pCSSession->pClusterTree, nClusterIndex);

	int parentIdx     = DLTDB_CLUSTER_GET_PARENT_INDEX(pCSSession->pClusterTree, nClusterIndex);
	int leftChildIdx  = DLTDB_CLUSTER_GET_CHILD_INDEX(pCSSession->pClusterTree, DLTDB_CLUSTER_GET_FIRST_CHILD(pCSSession->pClusterTree, parentIdx));
	int rightChildIdx = DLTDB_CLUSTER_GET_CHILD_INDEX(pCSSession->pClusterTree, DLTDB_CLUSTER_GET_LAST_CHILD(pCSSession->pClusterTree, parentIdx));

	DLTDB_NAMED_INDEX_NODE * pParentCluster  = DLTDB_DCSCLUSTER_GET_CLUSTER(&pCSSession->pSession->db, pCSSession->pClusterTree, parentIdx);
	DLTDB_UI8_NODE         * pChosenFeatures = DLTDB_DCSCLUSTER_GET_CHOSEN_FEATURES_TBL(&pCSSession->pSession->db, pParentCluster);

	int siblingIdx = (leftChildIdx == nClusterIndex) ? rightChildIdx : leftChildIdx;

	DLTDB_NAMED_INDEX_NODE * pCluster2        = DLTDB_DCSCLUSTER_GET_CLUSTER(&pCSSession->pSession->db, pCSSession->pClusterTree, siblingIdx);

	double prob = 0;

	DLTDB_UI16_NODE * pMeanTbl   = DLTDB_DCSCLUSTER_GET_FEATURE_MEANS_TBL(&pCSSession->pSession->db, pCluster);
	DLTDB_UI16_NODE * pStdTbl    = DLTDB_DCSCLUSTER_GET_FEATURE_STD_TBL(&pCSSession->pSession->db, pCluster);
	DLTDB_UI16_NODE * pWeightTbl = DLTDB_DCSCLUSTER_GET_FEATURE_WEIGHT_TBL(&pCSSession->pSession->db, pCluster);

	DECUMA_UINT16 meanNum   = pMeanTbl->pElements[0];
	DECUMA_UINT16 meanDenom = pMeanTbl->pElements[1];

	DECUMA_UINT16 stdNum   = pStdTbl->pElements[0];
	DECUMA_UINT16 stdDenom = pStdTbl->pElements[1];

	DECUMA_UINT16 weightNum   = pWeightTbl->pElements[0];
	DECUMA_UINT16 weightDenom = pWeightTbl->pElements[1];

	DLTDB_UI16_NODE * pMeanTbl2   = DLTDB_DCSCLUSTER_GET_FEATURE_MEANS_TBL(&pCSSession->pSession->db, pCluster2);
	DLTDB_UI16_NODE * pStdTbl2    = DLTDB_DCSCLUSTER_GET_FEATURE_STD_TBL(&pCSSession->pSession->db, pCluster2);
	DLTDB_UI16_NODE * pWeightTbl2 = DLTDB_DCSCLUSTER_GET_FEATURE_WEIGHT_TBL(&pCSSession->pSession->db, pCluster2);

	DECUMA_UINT16 mean2Num   = pMeanTbl2->pElements[0];
	DECUMA_UINT16 mean2Denom = pMeanTbl2->pElements[1];

	DECUMA_UINT16 std2Num   = pStdTbl2->pElements[0];
	DECUMA_UINT16 std2Denom = pStdTbl2->pElements[1];

	DECUMA_UINT16 weight2Num   = pWeightTbl2->pElements[0];
	DECUMA_UINT16 weight2Denom = pWeightTbl2->pElements[1];

	DECUMA_FEATURE featureVal;
	double value;
	double value2;

	/* Ignore when models have not been made */
	if (weightNum == 0 || weight2Num == 0)
		return 0.5;

	decumaAssert(meanDenom != 0);
	decumaAssert(mean2Denom != 0);

	featureVal   = dltCCCompressGetFeatureVal(pCSSession->pCChar, pChosenFeatures->pElements[0]);

	/* Numerical issues make us restrict these calculations to only when value is between means */
	/*if ((mean < mean2 && featureVal < mean) || (mean > mean2 && featureVal > mean)) { */
	if ((meanNum * mean2Denom < mean2Num * meanDenom && featureVal * meanDenom < meanNum)  ||
		(meanNum * mean2Denom > mean2Num * meanDenom && featureVal * meanDenom > meanNum)) {
		prob = 0.99;
	}
	/*else if ((mean2 < mean && featureVal < mean2) || (mean2 > mean && featureVal > mean2)) { */
	else if ((mean2Num * meanDenom < meanNum * mean2Denom && featureVal * mean2Denom < mean2Num)  ||
		(mean2Num * meanDenom > meanNum * mean2Denom && featureVal * mean2Denom > mean2Num)) {
		prob = 0.01;
	}
	else {

		if (stdNum > 0) {
			value  = (double) weightNum / weightDenom;
		}
		else {
			value = (double) meanNum / meanDenom;
		}

		if (std2Num > 0) {
			double std     = (double)stdNum     / stdDenom;
			double std2    = (double)std2Num    / std2Denom;
			double mean    = (double)meanNum    / meanDenom;
			double mean2   = (double)mean2Num   / mean2Denom;
			double weight2 = (double)weight2Num / weight2Denom;

			double t = ( featureVal - mean) * (featureVal - mean);
			double t2 = ( featureVal - mean2) * (featureVal - mean2);

			value2  = weight2 * std * dltExp( (t - t2) / (2 * std * std)) / std2;

		}
		else {
			value2 = (double) mean2Num / mean2Denom;
		}

		decumaAssert(value + value2 != 0);

		prob = value / (value + value2);
	}

#ifdef _MSCV
	_fpreset();
#endif
	if (prob < DLTDB_BEAM_MIN_DIST) return 0;

	decumaAssert(prob >= 0.0);
	decumaAssert(prob <= 1.0);

	return prob;

#if 0 
	double prob = 0;

	FeatureIndexVector * pFIdx = pChild->getParent()->getChosenFeatures();
	FeatureVector * pRep = pChild->getFeatureRep();

	/* Now get the feature modeling for this feature */
	double mean = pChild->getFeatureMean();
	double dStd  = pChild->getFeatureStd();
	double dWeight  = pChild->getFeatureWeight();

	/* and for the other feature through !pRep - if 0 then pick child 1 and vice versa */
	double mean2 = pChild->getParent()->getChild((*pRep)[0] == 0)->getFeatureMean();
	double dStd2  = pChild->getParent()->getChild((*pRep)[0] == 0)->getFeatureStd();
	double dWeight2  = pChild->getParent()->getChild((*pRep)[0] == 0)->getFeatureWeight();

	/* Ignore when models have not been made */
	if (dWeight == 0 || dWeight2 == 0)
		return -1;

	/* Hard code this to be binary for now */
	for (int s = 0; s < (int) sampleFeatures.size(); s++) {
		DECUMA_FEATURE fVal = (*sampleFeatures[s])[(*pFIdx)[0]];
		double value = dWeight * dltExp(-(fVal - mean)*(fVal - mean)/(2*dStd*dStd)) / dStd;
		double value2 = dWeight2 * dltExp(-(fVal - mean2)*(fVal - mean2)/(2*dStd2*dStd2)) / dStd2;
		/* Numerical issues make us restrict these calculations to only when value is between means */
		if ((mean < mean2 && fVal < mean) || (mean > mean2 && fVal > mean)) {
			prob += 0.99;
		}
		else if ((mean2 < mean && fVal < mean2) || (mean2 > mean && fVal > mean2)) {
			prob += 0.01;
		}
		else {
			prob += value / (value + value2);
		}
	}
	prob = (prob / sampleFeatures.size());
	return prob;
#endif
}


DECUMA_STATUS 
cjkDbCoarseBeamPrepareChildList(DLTDB_COARSE_SEARCH_SESSION * pCSSession,
                                DLTDB_BEAM                  * pBeam, 
                                DLTDB_BEAM_NODEVEC          * pChildList,
                                CJK_BOOLEAN	                  bUseNPoints)
{
	DLTDB_BEAM_NODEVEC * pTmp;
	int i, nChildListLen = 0;

	/* Switch node vectors (instead of copying pWorkingNodesStore -> pWorkingNodesUse) */
	pTmp                      = pBeam->pWorkingNodesUse;
	pBeam->pWorkingNodesUse   = pBeam->pWorkingNodesStore;
	pTmp->nNodes              = 0; /* reset */
	pBeam->pWorkingNodesStore = pTmp;

	/* Fetch next level of nodes in the tree, store cluster indices && distances between incoming char & */
	/* cluster reps in vector childList. */
	for (i = 0; i < pBeam->pWorkingNodesUse->nNodes; i++) {

		int nodeIdx   = pBeam->pWorkingNodesUse->nodes[i].nNodeIdx;
		int child;
		int lastChild;

		child     = DLTDB_CLUSTER_GET_FIRST_CHILD(pCSSession->pClusterTree, nodeIdx);
		lastChild = DLTDB_CLUSTER_GET_LAST_CHILD(pCSSession->pClusterTree, nodeIdx);

		for (;child <= lastChild; child++)
		{

			DECUMA_UINT16 childIdx    = DLTDB_CLUSTER_GET_CHILD_INDEX(pCSSession->pClusterTree, child);

			if (pCSSession->type == DLTDB_COARSE_CLUSTER_TREE_TYPE_DCS) {
				DLTDB_COARSE_DCS_DIST_TYPE dist = getCoarseDistanceToClusterRep_DCS(pCSSession, childIdx);
				if (pBeam->pWorkingNodesUse->nodes[i].dist * dist < DLTDB_BEAM_MIN_DIST) continue;
				pChildList->nodes[nChildListLen].nNodeIdx = childIdx;
				pChildList->nodes[nChildListLen].dist = pBeam->pWorkingNodesUse->nodes[i].dist * dist;					
			}
			else {
				if (bUseNPoints) {
					DECUMA_UINT16 nClusterMinPts = DLTDB_CLUSTER_GET_REPRESENTATIVE_IDX(pCSSession->pClusterTree, childIdx);
					pChildList->nodes[nChildListLen].nNodeIdx = childIdx;
					pChildList->nodes[nChildListLen].dist     = MAX_CJK_DISTANCE;
					if (dltCCCompressGetNbrPoints(pCSSession->pCChar) >= nClusterMinPts-BW && dltCCCompressGetNbrPoints(pCSSession->pCChar) < nClusterMinPts + 2*BW) {
						pChildList->nodes[nChildListLen].dist = 0;
					}
				}
			}

			nChildListLen++;

			decumaAssert(nChildListLen <= DLTDB_BEAM_NODE_COUNT);

			if (nChildListLen == DLTDB_BEAM_NODE_COUNT)
				goto LABEL_cjkDbCoarseBeamPrepareChildList_sort;
		}
		if (TEST_ABORT_RECOGNITION_EVERY(10,i,pCSSession->pSession->pInterruptFunctions))
			return decumaRecognitionAborted;
	}

LABEL_cjkDbCoarseBeamPrepareChildList_sort:

	pChildList->nNodes = nChildListLen;
	decumaQsort(pChildList, pChildList->nNodes, sizeof(pChildList->nodes[0]), comparator_beamNode_desc);

	return decumaNoError;
}


void 
cjkDbCoarseBeamMoveElementsToBeam(DLTDB_COARSE_SEARCH_SESSION * pCSSession, 
								  DLTDB_BEAM                  * pBeam, 
								  DLTDB_BEAM_NODEVEC          * pChildList)
{
	int i;
	DLTDB_COARSE_DCS_DIST_TYPE nAccumulatedDist = 0; /* i.e. confidence, when applicable */
	/* DLTDB_COARSE_DCS_DIST_TYPE nTotalProbability = 0; */

	/* Decide which nodes to keep in the beam */
	decumaAssert(pCSSession->pClusterTree);

	/*for (i = 0; i < pChildList->nNodes; i++)
		nTotalProbability += pChildList->nodes[i].dist;*/

	for (i = 0; i < pChildList->nNodes; i++) {
		int                          nodeIdx = pChildList->nodes[i].nNodeIdx;
		DLTDB_COARSE_DCS_DIST_TYPE   dist    = pChildList->nodes[i].dist; /* / nTotalProbability; */

		decumaAssert(pBeam->pWorkingNodesStore->nNodes >= 0);

		if (DLTDB_CLUSTER_GET_FIRST_CHILD(pCSSession->pClusterTree, nodeIdx) <=
			DLTDB_CLUSTER_GET_LAST_CHILD(pCSSession->pClusterTree, nodeIdx))
		{
			/* internal node */
			if (DLTDB_CLUSTER_BEAM_CUTOFF(dist, i, nChildListLen, nAccumulatedDist))
				break;

			if (pBeam->pWorkingNodesStore->nNodes < DLTDB_BEAM_NODE_COUNT) {
				pBeam->pWorkingNodesStore->nodes[pBeam->pWorkingNodesStore->nNodes].nNodeIdx = nodeIdx;
				pBeam->pWorkingNodesStore->nodes[pBeam->pWorkingNodesStore->nNodes].dist = dist;
				pBeam->pWorkingNodesStore->nNodes++;
			}
		}
		/* Since the childlist is sorted according to distance only add to included nodes
		 * if its the first element */
		else if (i == 0) {
			/* leaf node */
			if (pBeam->includedNodes.nNodes < DLTDB_BEAM_NODE_COUNT) {
				pBeam->includedNodes.nodes[pBeam->includedNodes.nNodes].nNodeIdx = nodeIdx;
				pBeam->includedNodes.nodes[pBeam->includedNodes.nNodes].dist     = dist;	
				pBeam->includedNodes.nNodes++;
			}
			else {
				decumaAssert(pBeam->includedNodes.nNodes < DLTDB_BEAM_NODE_COUNT);
			}
		}

		nAccumulatedDist += dist;

		decumaAssert(pCSSession->pClusterTree);
		decumaAssert(pBeam->pWorkingNodesStore->nNodes <= DLTDB_BEAM_NODE_COUNT);
		decumaAssert(pBeam->includedNodes.nNodes       <= DLTDB_BEAM_NODE_COUNT);
	}
}

static DECUMA_STATUS
dbCoarseBeamFetchIndices (DLTDB_COARSE_SEARCH_SESSION * pCSSession, int nClusterIdx)
{
	DLTDB_UI16_NODE            * pKeyIndexNode;
	DLTDB_COARSE_SEARCH_RESULT * pResult = pCSSession->pResult;
	int j;

	int keyIndex;
	DECUMA_UINT32 nGlobalCatMask ;
	DECUMA_UINT32 nGlobalWsMask ;
	DLTDB * pDb ;
	CJK_DISTANCE  distance;
	DECUMA_UINT16 unicode ;
	DECUMA_UINT32 nCatMask ;
	DECUMA_UINT32 nWsMask ;
	DECUMA_UINT16 a;

	DLTDB_UI16_NODE *pKeyIndexNodeTbl = NULL;
	DECUMA_UINT8 minPt=0;
	DECUMA_UINT8 maxPt=0;

	DECUMA_UINT16 lowBound=0;
	DECUMA_UINT16 upperBound=0;

	DECUMA_UINT16 minIndex=0;

	/*
	DECUMA_UINT8 bandWidth=8;
	*/

	DECUMA_UINT8 bandWidth=BW_DOUBLE/2;

	decumaMemset(pCSSession->pResult->pIndices, 0, MAX_NBR_DB_IDX_BIT_CHARS);

	if (pCSSession->type == DLTDB_COARSE_CLUSTER_TREE_TYPE_DCS) {
		DLTDB_NAMED_INDEX_NODE * pClusterNode = DLTDB_DCSCLUSTER_GET_CLUSTER(&pCSSession->pSession->db, pCSSession->pClusterTree, nClusterIdx);

		pKeyIndexNode                         = DLTDB_DCSCLUSTER_GET_KEY_INDEX_TBL(&pCSSession->pSession->db, pClusterNode);

		/* the pKeyIndexNodeTbl contains the maxPt and minPt, and the list of accumlated indices up to the specified number of points*/
		pKeyIndexNodeTbl=DLTDB_DCSCLUSTER_GET_KEY_INDEX_TBL_AUX(&pCSSession->pSession->db, pClusterNode);
	}
	else {

		/* in a single cluster database, the first table is the indices, and the second is the point table */
		pKeyIndexNode = DLTDB_CLUSTER_GET_KEY_INDEX_TBL(&pCSSession->pSession->db, pCSSession->pClusterTree, 0);
		pKeyIndexNodeTbl=DLTDB_CLUSTER_GET_KEY_INDEX_TBL(&pCSSession->pSession->db, pCSSession->pClusterTree, 1);

	}

	if (pKeyIndexNode == NULL)
	{
		/* This should only happen if the database is somehow corrupted */
		decumaAssert(0);
		return decumaNullPointer;
	}

	lowBound=0;
	upperBound=0;
	minIndex=0;

	/* this is the default upperBound in case the pKeyIndexNodeTbl does not exist */
	if( pKeyIndexNode->nElements ){
		upperBound = pKeyIndexNode->nElements;
	}

	/* if everything is correct, the pKeyIndexNodeTbl should contain (maxPt-minPt+3) elements */
	if( pKeyIndexNodeTbl && pKeyIndexNodeTbl->nElements>2 ){
		/*	the lower byte of the first element is the minimum points
			and the high byte of the first element is the maximum points
		*/
		minPt = pKeyIndexNodeTbl->pElements[0] & 0xFF;
		maxPt = pKeyIndexNodeTbl->pElements[0] >> 8;

		/* the second element stores the smallest index */
		minIndex = pKeyIndexNodeTbl->pElements[1];

		/* calculate the lowBound */
		if( dltCCCompressGetNbrPoints(pCSSession->pCChar) - bandWidth > minPt ){
			lowBound = pKeyIndexNodeTbl->pElements[dltCCCompressGetNbrPoints(pCSSession->pCChar) - bandWidth-minPt+1];
		}

		/* and calculate the upperBound */
		if( dltCCCompressGetNbrPoints(pCSSession->pCChar) + bandWidth < maxPt ){
			upperBound = pKeyIndexNodeTbl->pElements[dltCCCompressGetNbrPoints(pCSSession->pCChar) + bandWidth-minPt+2];
		}	
	}

	pCSSession->pResult->nIndices += (upperBound - lowBound) + 1;

	for (j = 0; j < pKeyIndexNode->nElements; j++) {

		/* TODO selective inclusion, check categories & writing styles, see  */
		/* cjkDbCoarseBinarySearch(). */

		/* we actually don't need this boundary check anymore, the bitmap array will be able to store
		 * more than enough indices */

		/* if the element is outside our bandwidth, we simply ignore it */
		if( j < lowBound || j >= upperBound ) continue;


		/* this section filter out indices which do not belong to the same category */
		{
			DECUMA_UINT16 * pKeyIndices = &pKeyIndexNode->pElements[j];
			DECUMA_UINT32 nKeyIndices = 1;
			int ki, nCalcs = 0;

			pDb = &pCSSession->pSession->db;

			/* Check if this is two-level index (compressed) */
			if (*pKeyIndices >= pDb->nKeys) {
				DECUMA_UINT32 nOffsetStart = DLTDB_GET_INDEXLIST_OFFSET_START(pDb, *pKeyIndices - pDb->nKeys);
				nKeyIndices = DLTDB_GET_INDEXLIST_LENGTH(pDb, *pKeyIndices - pDb->nKeys);
				pKeyIndices = DLTDB_GET_INDEXLIST_BY_OFFSET(pDb, nOffsetStart);
			}
			for (ki = 0; ki < nKeyIndices; ki++) {
				keyIndex = pKeyIndices[ki];
				nGlobalCatMask = (~(CJK_POPULARFORM | CJK_TRADSIMPDUAL)) &pCSSession-> pSession->sessionCategories;
				nGlobalWsMask  =   (CJK_POPULARFORM | CJK_TRADSIMPDUAL)  &pCSSession->pSession->sessionCategories;

				unicode = DLTDB_GET_UNICODE(pDb, keyIndex);
				nCatMask = DLTDB_GET_CATEGORY(pDb, keyIndex);
				nWsMask  = (CJK_POPULARFORM | CJK_TRADSIMPDUAL) & nCatMask;

				a = DLTDB_GET_ATTRIBUTE(pDb, keyIndex);

				if (!CJK_ATTRIB_IS_DENSE(a) != !dltCCCompressIsDense(pCSSession->pCChar)) continue;

				if (!cjkDbIsInPersonalCategory(pCSSession->pSession, keyIndex) && !((nCatMask & nGlobalCatMask) != 0  && (nWsMask == 0 || (nWsMask & nGlobalWsMask)  != 0))) {
					/* database and personal categories */
					continue;
				}
				else if (nGlobalCatMask == 0 && nGlobalWsMask == 0 && !cjkDbIsInPersonalCategory(pCSSession->pSession, keyIndex)) {
					/* only personal categories */
					continue;
				}

				decumaAssert(keyIndex < MAX_NBR_DB_IDX_BIT_CHARS*8);
				/* use the pResult->pIndices as bit array to store the visited indices, and later remove the duplicates */
				if ( DLTDB_GET_BIT(pResult->pIndices, keyIndex ) ) continue;

				if (keyIndex == dltCCCompressGetIndex(pCSSession->pCChar)) pCSSession->pResult->bCorrectFound = 1;

				if (TEST_ABORT_RECOGNITION_EVERY(40,nCalcs++,pCSSession->pSession->pInterruptFunctions))
					return decumaRecognitionAborted;

				distance = cjkDbGetDistanceToIndex(pCSSession->pSession, pCSSession->pCChar, (DLTDB_INDEX)keyIndex, 0, 0, 0);

				/* directly calculate the distance here, in stead of building a list of candidates */
				if (distance < LARGEDIST)
						cjkBestListInsert(&pCSSession->pSession->db_lookup_bl, unicode, keyIndex, distance, pCSSession->pSession);

				/* set the bit so next time we can check if there is any duplication */
				DLTDB_SET_BIT( pResult->pIndices, keyIndex );
			}

		}
	}
	return decumaNoError;
}

/**
 * Fetch indices from nodes in pBeam into pCSSession->pResult.
 *
 * NOTE: This algorithm assumes that cluster node indexing corresponds 
 * to a breadth first traversal of the cluster tree.
 *
 */
DECUMA_STATUS 
cjkDbCoarseBeamFetchKeyIndicesFromBeam(DLTDB_COARSE_SEARCH_SESSION * pCSSession,
									   DLTDB_BEAM                  * pBeam)
{
	int nNode;
	DECUMA_UINT16   nodeQueue[DLTDB_BEAM_NODE_COUNT];
	int frontIndex = 0;
	int backIndex  = 0;
	DECUMA_UINT16 currentParent;
	DECUMA_UINT16 newParent;
	DECUMA_STATUS status;
	int nNodeIndex;
	double d = 0;

	if(pCSSession->type==DLTDB_COARSE_CLUSTER_TREE_TYPE_DCS) {

		decumaAssert(pBeam->includedNodes.nNodes <= DLTDB_BEAM_NODE_COUNT);

		DECUMA_CAST(DLTDB_COARSE_SEARCH_SESSION_GENERIC *, pCSSession)->nLastCompactionGain = 100;

		/* if the tree is not balanced, we need to sort the included nodes*/
		decumaQsort(&pBeam->includedNodes.nodes, pBeam->includedNodes.nNodes, sizeof(pBeam->includedNodes.nodes[0]), comparator_beamNode_desc);

		/* TODO: This is unnecessarily complex since only 1 is used */
		for (nNode = 0; nNode < pBeam->includedNodes.nNodes && nNode < 1; nNode++)
		{
			nodeQueue[backIndex++] = pBeam->includedNodes.nodes[nNode].nNodeIdx;
			d += pBeam->includedNodes.nodes[nNode].dist;
		}

		/* By sorting we ensure that siblings are placed next to each other in the queue, */
		/* provided that the node numbering condition is met (see function documentation). */
		decumaQsort(nodeQueue, backIndex, sizeof(nodeQueue[0]), comparator_uint16_desc);

		/* initial setup */
		if (frontIndex < backIndex) 
		{
			nNodeIndex    = nodeQueue[frontIndex++];
			currentParent = DLTDB_CLUSTER_GET_PARENT_INDEX(pCSSession->pClusterTree, nNodeIndex);
			if (currentParent != 0xFFFF)
				nodeQueue[backIndex++] = currentParent;
			status = dbCoarseBeamFetchIndices(pCSSession, nNodeIndex);
			if (status != decumaNoError) return status;
		}

		while (frontIndex < backIndex) 
		{
			nNodeIndex = nodeQueue[frontIndex++];
			newParent  = DLTDB_CLUSTER_GET_PARENT_INDEX(pCSSession->pClusterTree, nNodeIndex);

			status = dbCoarseBeamFetchIndices(pCSSession, nNodeIndex);
			if (status != decumaNoError) return status;

			if (newParent != currentParent && newParent != 0xFFFF) 
			{
				decumaAssert(backIndex < DLTDB_BEAM_NODE_COUNT);
				nodeQueue[backIndex++] = newParent;
				currentParent          = newParent;
			}
		}

	}
	else{
		/* clutser ID is not required for a single cluster database, here we set to 0 */
		status = dbCoarseBeamFetchIndices(pCSSession, 0);
		if (status != decumaNoError) return status;

	}
	return decumaNoError;
}


DECUMA_STATUS 
cjkDbCoarseBeamSearch(DLTDB_COARSE_SEARCH_SESSION * pCSSession)
{
	DECUMA_STATUS status;
	pCSSession->pScratchpad = (CJK_DB_COARSE_SCRATCHPAD *)pCSSession->pSession->pScratchpad;
	/* **************************************************************************** */
	/*             NOTE! This function has a corresponding test method,             */
	/*    cjkDbClusterBinaryTreeTest() in cjkDatabase_Test.c. If this method is    */
	/*                 changed, make sure the same change is made in  */
	/*                          cjkDbClusterBinaryTreeTest()          */
	/* **************************************************************************** */

	/* Having two lists allows us to substitute a vector  */
	/* copy with a pointer switch later on */

	if ( pCSSession->type==DLTDB_COARSE_CLUSTER_TREE_TYPE_DCS )
	{
		DLTDB_BEAM beam;
		DLTDB_BEAM_NODEVEC *pBeamNodes1 = &(pCSSession->pScratchpad->beamNodes1);
		DLTDB_BEAM_NODEVEC *pBeamNodes2 = &(pCSSession->pScratchpad->beamNodes2);
		DLTDB_BEAM_NODEVEC *pChildList  = &(pCSSession->pScratchpad->childList);

		DECUMA_UINT32 i;

		int nTreeLevel = 0;

		/* Memset scratchpad */
		decumaMemset(pBeamNodes1, 0, sizeof(DLTDB_BEAM_NODEVEC));
		decumaMemset(pBeamNodes2, 0, sizeof(DLTDB_BEAM_NODEVEC));
		decumaMemset(pChildList, 0, sizeof(DLTDB_BEAM_NODEVEC));

		for (i = 0; i < sizeof(pBeamNodes1->nodes) / sizeof(pBeamNodes1->nodes[0]); i++ )
		{
			pBeamNodes1->nodes[i].dist = 1;
			pBeamNodes2->nodes[i].dist = 1;
		}

		beam.includedNodes.nNodes = 0;
		beam.pWorkingNodesStore = pBeamNodes1;
		beam.pWorkingNodesUse   = pBeamNodes2;

		decumaAssert(pCSSession->pClusterTree->clusterType == DLTDB_COARSE_CLUSTER_TREE_TYPE_DCS);

		/* add the first node, index 0 */
		beam.pWorkingNodesStore->nodes[beam.pWorkingNodesStore->nNodes++].nNodeIdx = 0;

		while (beam.pWorkingNodesStore->nNodes > 0 && beam.includedNodes.nNodes == 0) {
			status = cjkDbCoarseBeamPrepareChildList(pCSSession, &beam, pChildList, nTreeLevel == 0);
			if (status != decumaNoError) return status;
			cjkDbCoarseBeamMoveElementsToBeam(pCSSession, &beam, pChildList);
			if (TEST_ABORT_RECOGNITION(pCSSession->pSession->pInterruptFunctions))
				return decumaRecognitionAborted;
			nTreeLevel++;
		}

		/* duplicates removal is not necessary now, inside cjkDbCoarseBeamFetchKeyIndicesFromBeam, the duplicates
		   are checked by the bitarray */
		status = cjkDbCoarseBeamFetchKeyIndicesFromBeam(pCSSession, &beam);
		if (status != decumaNoError) return status;
	}
	else
	{
		status = cjkDbCoarseBeamFetchKeyIndicesFromBeam(pCSSession, NULL);
		if (status != decumaNoError) return status;
	}

	return decumaNoError;
}

static int 
comparator_beamNode_desc(const void * a, const void * b) 
{
	const DLTDB_BEAM_NODE * pA = a;
	const DLTDB_BEAM_NODE * pB = b;

	decumaAssert(pA);
	decumaAssert(pB);

	return pA->dist < pB->dist ? 1 : (pA->dist > pB->dist ? -1 : 0);
}


static int 
comparator_uint16_desc(const void * a, const void * b)
{
	const DECUMA_UINT16 * pA = a;
	const DECUMA_UINT16 * pB = b;

	decumaAssert(pA);
	decumaAssert(pB);
	
	return *pB - *pA;
}
