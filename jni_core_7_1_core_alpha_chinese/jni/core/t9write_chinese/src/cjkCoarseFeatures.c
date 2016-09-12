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

#define CJK_COARSE_FEATURES_C
#define CJK_SYSTEM_CODE

#include "cjkCoarseFeaturesData.h"
#include "cjkCoarseFeatures.h"

#include "cjkCommon.h"
#include "cjkCompressedCharacter_Macros.h"
#include "cjkStroke_Macros.h"
#include "cjkMath.h"

#include "decumaIntegerMath.h"
#include "decumaKMeans.h"

#include "decumaCommon.h"
#include "decumaDataTypes.h"
#include "decumaInterrupts.h"
#include "decumaMemory.h"
#include "cjkSession_Types.h"

/* --------------------------------------------------------------------------
 * Macros
 * -------------------------------------------------------------------------- */

#define BAD_DTW_POINT_DIST 50

#define CJK_SPLIT_GET_GP_VAL(bX, gp) ((bX> 0) ? CJK_GP_GET_X(gp):CJK_GP_GET_Y(gp))
#define CJK_SPLIT_BW 10
#define CJK_SPLIT_IDX_BW 6

#define GP_DIR_IS_SW(gpdir) ((gpdir == 16 || gpdir == 17 || gpdir == 2)                 ? 1 : 0)
#define GP_DIR_IS_E(gpdir)  ((gpdir == 35 || gpdir == 36 || gpdir == 20 || gpdir == 52) ? 1 : 0)
#define GP_DIR_IS_S(gpdir)  ((gpdir == 18 || gpdir == 2  || gpdir == 3  || gpdir == 4)  ? 1 : 0)
#define GP_DIR_IS_SE(gpdir) ((gpdir == 4  || gpdir == 19 || gpdir == 20)                ? 1 : 0)

static DECUMA_UINT32 cjkGetCoarseFeatureWithIdx(DECUMA_FEATURE      * pFeatures,
										DECUMA_UINT32         nFeatures,
										DECUMA_UINT32		  nIdx,
										CJK_COMPRESSED_CHAR * pChar,
										CJK_SESSION         * pSession,
										COARSE_SETTINGS     * pSettings);

static DECUMA_UINT32 coarseFeatureWithIdx(DECUMA_FEATURE    * pFeatures,
										DECUMA_UINT32         nFeatures,
										DECUMA_UINT32		  nIdx,
										CJK_COMPRESSED_CHAR * pChar,
										CJK_SESSION         * pSession,
										COARSE_SETTINGS     * pSettings,
										DECUMA_BOOL			  bCalculate);

#if 0
static DECUMA_UINT32 cjkGetCoarseDependentFeatures(DECUMA_FEATURE  * pbIsSet,
											int  * pFeatureIdxs,
											int    nFeatures);
#endif

static void dltCoarseSetSwiftData(CJK_COMPRESSED_CHAR * pCChar, SWIFT_DIST * pSwiftDist, CJK_SESSION * pSession);

#if 0
static CJK_DISTANCE dltCoarsePenalizeRadicalBox(CJK_COMPRESSED_CHAR * pChar, int nMatchIdx, CJK_SESSION * pSession);
#endif

static void dltCoarseGetDistRadicalPerson(CJK_COMPRESSED_CHAR * pChar, CJK_DISTANCE * dist, 
										  CJK_DISTANCE * angDist, CJK_SESSION * pSession);
static void dltCoarseGetDistRadicalHand(CJK_COMPRESSED_CHAR * pChar, CJK_DISTANCE * dist, 
										  CJK_SESSION * pSession);
static void dltCoarseGetDistRadicalThreedrops(CJK_COMPRESSED_CHAR * pChar, CJK_DISTANCE * dist, 
										CJK_SESSION * pSession);
static void dltCoarseGetDistRadicalThread(CJK_COMPRESSED_CHAR * pChar, CJK_DISTANCE * dist, 
										CJK_SESSION * pSession);

/* --------------------------------------------------------------------------
 * Exported functions
 * -------------------------------------------------------------------------- */


DECUMA_UINT32 cjkCoarseFeaturesGetScratchpadSize(void)
{
	return (sizeof(CJK_COARSE_FEATURES_SCRATCHPAD) + 2*cjkCompressedCharacterGetSize());
}

DECUMA_STATUS cjkGetCoarseFeatures(DECUMA_FEATURE      * pFeatures,
								   DECUMA_UINT32         nFeatures,
								   CJK_COMPRESSED_CHAR * pChar,
								   CJK_SESSION         * pSession,
								   COARSE_SETTINGS     * pSettings) 
{
	int i, n, nCalls = 0;
	CJK_COARSE_FEATURES_SCRATCHPAD * pScratchpad = (CJK_COARSE_FEATURES_SCRATCHPAD *) pSession->pScratchpad;
	decumaAssert(nFeatures <= COARSE_NBR_FEATURES);

	if (dltCCCompressGetNbrPoints(pChar) == 0)
		return decumaZeroPoints;
	
	decumaAssert(sizeof(pFeatures[0]) == 1); /* Other initialization if not byte */
	decumaMemset(pFeatures, -1, nFeatures);

	/* Set scratchpad pointers */
	pScratchpad->pCCFirstStrokeIn = (CJK_COMPRESSED_CHAR *)((char *)pScratchpad + sizeof(CJK_COARSE_FEATURES_SCRATCHPAD));
	pScratchpad->pCCFirstStrokeDb = (CJK_COMPRESSED_CHAR *)((char *)pScratchpad->pCCFirstStrokeIn + cjkCompressedCharacterGetSize());

	/* TODO: Set only necessary feature bits for this rec call */
	for (i=0; i<nFeatures; i++) {
		if (CJK_COARSE_FEATURE_NOT_SET(pFeatures, i)) {
			n = cjkGetCoarseFeatureWithIdx(pFeatures, nFeatures, i, pChar, pSession, 
										pSettings);
			if (TEST_ABORT_RECOGNITION_EVERY(6,nCalls++,pSession->pInterruptFunctions))
				return decumaRecognitionAborted;
		}
		decumaAssert(pFeatures[i] >= 0);
	}
	return decumaNoError;
}

/* --------------------------------------------------------------------------
 * Local functions
 * -------------------------------------------------------------------------- */

K_MEANS_VALUE * kMeansGetValue(K_MEANS_VALUE * pArray, int idx)
{
	return &(pArray[idx]);
}

static K_MEANS_VALUE * kMeansMin(K_MEANS_VALUE * a, K_MEANS_VALUE * b)
{
	if (a->value < b->value) /* (a->idx+a->value < b->idx + b->value) //  */
		return a;
	else
		return b;
}

static K_MEANS_VALUE * kMeansMax(K_MEANS_VALUE * a, K_MEANS_VALUE * b)
{
	if (a->value >= b->value) /* (a->idx+a->value >= b->idx + b->value) //  */
		return a;
	else
		return b;
}

/* Uses stroke clustering in x-direction to find a good split point */
/* return value is a normalized (0, 127) value for how good the split point was. */

static void calculateCenter(int nIdx, int * pIdx, K_MEANS_VALUE * pValues, K_MEANS_VALUE * pCenter)
{
	int i, value = 0, idx = 0;
	decumaAssert(nIdx);
	/* Get the new "mean" value */
	for (i=0; i<nIdx; i++) {
		value += pValues[pIdx[i]].value;
		idx += pValues[pIdx[i]].idx;
	}
	pCenter->value = (unsigned char)(value / nIdx);
	pCenter->idx = (unsigned char)(idx / nIdx);
	return;
}

static int splitCharDist(K_MEANS_VALUE * nVal1, K_MEANS_VALUE * nVal2)
{
	return (ABS(nVal1->value - nVal2->value)); /* + ABS(nVal1->idx - nVal2->idx)); */
}

static int splitGetSinglePtPunishment(CJK_GRIDPOINT gp, int bGt, int bInX, int nSplitBorder)
{
	int punish = 0, val1;
	val1 = (bInX > 0) ? (2*CJK_GP_GET_X(gp)):(2*CJK_GP_GET_Y(gp));

	if ((bGt > 0) ? (val1 > nSplitBorder) : (val1 <= nSplitBorder)) {
		/* Start cost to prioritize few number of intruding points */
		punish = (val1 - nSplitBorder)*(val1 - nSplitBorder) / 4 + 1; 
	}
	return punish;
}

#if 0
static int splitGetSumSquares(int dist) {
	int nSquareSums[15] = {1, 5, 14, 30, 55, 91, 140, 204, 285, 385, 506, 650, 819, 1023, 1248};
	decumaAssert(dist >= 0 && dist < (sizeof(nSquareSums) / sizeof(nSquareSums[0])) );
	return nSquareSums[dist];
}

static int splitGetPtPunishment(CJK_GRIDPOINT gp, CJK_GRIDPOINT prevgp, int bGt, int bInX, int nSplitBorder)
{
	int punish = 0, val1, val2, tval;
	val1 = (bInX > 0) ? (2*CJK_GP_GET_X(gp)):(2*CJK_GP_GET_Y(gp));
	val2 = (bInX > 0) ? (2*CJK_GP_GET_X(prevgp)):(2*CJK_GP_GET_Y(prevgp));

	/* Moving left or up (possibly crossing border back to "right" side) */
	/* --> switch values */
	if ((bGt > 0) ? (val1 < val2) : (val1 > val2)) {
		tval = val1;
		val1 = val2;
		val2 = tval;
	}
	if ((bGt > 0) ? (val1 > nSplitBorder) : (val1 <= nSplitBorder)) {		
		/* If crossing border then diff in x */
		if ((bGt > 0) ? (val2 <= nSplitBorder) : (val2 > nSplitBorder)) {
			/*punish = ABS(val1 - nSplitBorder); */
			punish = splitGetSumSquares(DECUMA_MAX(ABS(val1 - nSplitBorder) / 2 - 1, 0));			
		}
		/* If vertical diff larger choose this (to punish vertical strokes in wrong half) */
		else {
			int diff = (bInX > 0) ? (ABS(CJK_GP_GET_Y(prevgp)-CJK_GP_GET_Y(gp))) : (ABS(CJK_GP_GET_X(prevgp)-CJK_GP_GET_X(gp)));
			if (diff > ABS(val2-val1)) {
				/*punish = diff; */
				punish = diff * ((val1-val2)*(val1-val2) / 4);
			}
			else {
				punish = ABS(splitGetSumSquares(DECUMA_MAX(ABS(val1 - nSplitBorder) / 2 - 1, 0)) - splitGetSumSquares(DECUMA_MAX(ABS(val2 - nSplitBorder) / 2 - 1, 0)));
				/*punish = ABS(val2-val1); */
			}
		}
	}
	return punish;
}
#endif

static int dltCCharSetSplit(CJK_COMPRESSED_CHAR * pCChar, CJK_BOOLEAN bSplitX, DECUMA_INT32 * pnSplitIdx, CJK_SESSION * pSession) 
{
	K_MEANS_SESSION kMeansSession;
	K_MEANS_CLUSTER clusters[2];
	K_MEANS_VALUE pCenters[2];
	K_MEANS_VALUE nStartValues[2];
	K_MEANS_VALUE nMinVal = {30, 30}, nMaxVal = {0,0};
	CJK_COARSE_FEATURES_SCRATCHPAD * pScratchpad = (CJK_COARSE_FEATURES_SCRATCHPAD *) pSession->pScratchpad;
	unsigned char val1, val2;
	int nChanged, i, nPoints, nValues = 0;
	int splitValue = DECUMA_FEATURE_MAX;
	int nGPs = 0;

	CJK_STROKE stroke;

	/* Scratchpad pointers */
	K_MEANS_VALUE * const pValues = pScratchpad->kMeanValues;
	CJK_GRIDPOINT * const pGP = pScratchpad->gridPoints;
	/* Reuse the scratchpad from the swiftdata calculations */
	DECUMA_INT32 * const c1Members  = pScratchpad->sx;
	DECUMA_INT32 * const c2Members  = pScratchpad->sy;
	DECUMA_INT32 * const nGPIdx  = pScratchpad->st;
	DECUMA_INT32 * const bIsStrokeStart = pScratchpad->sxx;
	DECUMA_INT32 * const pCluster = pScratchpad->stt;
	DECUMA_INT32 * const pMaxValues = pScratchpad->syy;
	DECUMA_INT32 * const pMinValues = pScratchpad->sty;

	decumaAssert(dltCCCompressGetNbrPoints(pCChar) <= MAXPNTSPERCHAR);

	/* Short-circuit if there are too few points in input */
	if (dltCCCompressGetNbrPoints(pCChar) <= 6) {
		pnSplitIdx[0] = 0;
		return 0;
	}

	/* Clear memory areas that we won't explicitly set */
	decumaMemset(pCluster, 0, MAXPNTSPERCHAR*sizeof(pCluster[0]));

	/* Set the X-values as values for the initial clustering */
	nGPIdx[nValues] = 0;
	for(dltCCharGetFirstStroke(pCChar, &stroke, pSession); 
		CJK_STROKE_EXISTS(&stroke); 
		cjkStrokeNext(&stroke, pSession)) 
	{
		nPoints = CJK_STROKE_NPOINTS(&stroke);
		if (nPoints == 1) {
			if (bSplitX) {
				val1 = CJK_STROKE_GET_X(&stroke, 1);
			}
			else {
				val1 = CJK_STROKE_GET_Y(&stroke, 1);
			}
			pMinValues[nValues] = val1;
			pMaxValues[nValues] = val1;
			pValues[nValues].value = 2*val1;
			nGPIdx[nValues] = nGPs;
			pValues[nValues].idx = (unsigned char)((int)(nGPIdx[nValues] * 30) / dltCCCompressGetNbrPoints(pCChar)); 
			nMinVal = *kMeansMin(&nMinVal, &(pValues[nValues]));
			nMaxVal = *kMeansMax(&nMaxVal, &(pValues[nValues]));
			nValues++;
		}
		/* OBS! STROKE_GET takes 1-based indexes */
		for (i=1; i < nPoints; i++) {
			 if (i == 1) {
				 bIsStrokeStart[nGPs] = 1;
			 }
			 else {
				 bIsStrokeStart[nGPs] = 0;
			 }

			/* TODO: Skip down-to-up segments (are not real segments in CC)			 */
			if (bSplitX) {
				val1 = CJK_STROKE_GET_X(&stroke, i);
				val2 = CJK_STROKE_GET_X(&stroke, i+1);
			}
			else { /* split y */
				val1 = CJK_STROKE_GET_Y(&stroke, i);
				val2 = CJK_STROKE_GET_Y(&stroke, i+1);
			}
			/* Blow up coordinate system by a factor to to get mean */
			pMinValues[nValues] = DECUMA_MIN(val1, val2);
			pMaxValues[nValues] = DECUMA_MAX(val1, val2);			
			pValues[nValues].value = (val1 + val2);
			nGPIdx[nValues] = nGPs;
			pValues[nValues].idx = (unsigned char)((int)(nGPIdx[nValues] * 30) / dltCCCompressGetNbrPoints(pCChar)); 
			nMinVal = *kMeansMin(&nMinVal, &(pValues[nValues]));
			nMaxVal = *kMeansMax(&nMaxVal, &(pValues[nValues]));
			pGP[nGPs++] = *cjkStrokeGetGridpoint(&stroke, i);
			nValues++;
		}
		bIsStrokeStart[nGPs] = 0;
		pGP[nGPs++] = *cjkStrokeGetGridpoint(&stroke, nPoints);
	}
	decumaAssert(nGPs == dltCCCompressGetNbrPoints(pCChar));

	/* Initialize clustering session */
	nStartValues[0] = nMinVal; /* cluster1 (left, top) */
	nStartValues[1] = nMaxVal; /* cluster2 (right, down) */
	decumaKMeansSessionInit(&kMeansSession, calculateCenter, splitCharDist,
		&nStartValues[0], nValues, &pValues[0], &pCluster[0], 0 /* Break criteria*/, 
		10 /* Max iterations */);

	/* TODO Here we assume that DECUMA_INT32 == int
	 *       We should either change type definition of clusters[0].pIds or c1Members
	 *       (same goes for clusters[1] and c2Members
	 */
	clusters[0].pIdx = &c1Members[0];
	clusters[1].pIdx = &c2Members[0];
	clusters[0].pCenter = &pCenters[0];
	clusters[1].pCenter = &pCenters[1];

	/* Perform initial clustering (return value is number of changed cluster members in */
	/* last iteration */
	nChanged = decumaKMeans(2, &clusters[0], &kMeansSession);
	
	/* Failure at this point means that there is nothing to split (such as I) */
	pnSplitIdx[0] = 0;
	if (clusters[0].nIdx > 0 && clusters[1].nIdx > 0) {
		CJK_SPLIT_IDX splitIdx[CJK_SPLIT_IDX_BW];	
		int nSplitIdxs = 0;
		int nC0 = clusters[0].nIdx, nC1 = clusters[1].nIdx, j;
		int nMaxC0Val = pMaxValues[clusters[0].pIdx[0]];
		int nMinC1Val = pMinValues[clusters[1].pIdx[nC1-1]];
		int nSmallestC1Idx = nC1-1, nLargestC0Idx = 0;
		int nAmbigMin = CJK_GP_MAX, nAmbigMax = 0;
		int nMinIdx = -1, nMaxIdx = -1;

		/* Assume that first indexes are in C0 */
		if (clusters[0].pIdx[0] > clusters[1].pIdx[0]) {
			int n1Idx = clusters[1].nIdx;
			clusters[1].pIdx = &c1Members[0];
			clusters[1].nIdx = clusters[0].nIdx;
			clusters[0].pIdx = &c2Members[0];
			clusters[0].nIdx = n1Idx;
			nC0 = clusters[0].nIdx;
			nC1 = clusters[1].nIdx;
			nMaxC0Val = pMaxValues[clusters[0].pIdx[0]];
			nMinC1Val = pMinValues[clusters[1].pIdx[nC1-1]];
			nSmallestC1Idx = nC1-1;
		}

		/* Find the ambiguous area - clump the "secure" areas as the continuous blocks */
		for (i=1; i < nC0; i++) {
			if (clusters[0].pIdx[i]-clusters[0].pIdx[i-1] > 1) break;
			nMaxC0Val = DECUMA_MAX(pMaxValues[clusters[0].pIdx[i]], nMaxC0Val);
			nLargestC0Idx = i;
		}
		/* Let the next be ambiguous */
		for (i=nLargestC0Idx+1; i < nC0; i++) {
			nAmbigMin = DECUMA_MIN(pMinValues[clusters[0].pIdx[i]], nAmbigMin);
			nAmbigMax = DECUMA_MAX(pMaxValues[clusters[0].pIdx[i]], nAmbigMax);
		}
		for (i=nC1-2; i >= 0; i--) {
			if (clusters[1].pIdx[i+1]-clusters[1].pIdx[i] > 1) break; /* jump condition */
			nMinC1Val = DECUMA_MIN(pMinValues[clusters[1].pIdx[i]], nMinC1Val);
			nSmallestC1Idx = i;
		}
		/* Let the next be ambiguous */
		for (i=nSmallestC1Idx-1; i >= 0; i--) {
			nAmbigMin = DECUMA_MIN(pMinValues[clusters[1].pIdx[i]], nAmbigMin);
			nAmbigMax = DECUMA_MAX(pMaxValues[clusters[1].pIdx[i]], nAmbigMax);
		}
		/* Now find set of ambiguous indexes due to overlap */
		for (i=1; i <= nLargestC0Idx; i++) {
			K_MEANS_VALUE val = *kMeansGetValue(pValues,clusters[0].pIdx[i]);
			if (val.value >= 2*nMinC1Val ||
				(pMinValues[clusters[0].pIdx[i]] >= nAmbigMin && pMaxValues[clusters[0].pIdx[i]] <= nAmbigMax)) {
				nLargestC0Idx=i-1;
				break;
			}
		}
		for (i=nC1-2; i >= nSmallestC1Idx; i--) {
			K_MEANS_VALUE val = *kMeansGetValue(pValues,clusters[1].pIdx[i]);
			if (val.value >= 2*nMaxC0Val ||
				(pMinValues[clusters[1].pIdx[i]] >= nAmbigMin && pMaxValues[clusters[1].pIdx[i]] <= nAmbigMax)) {
				nSmallestC1Idx=i+1;
				break;
			}
		}

		/* Start by setting largest idx of C0 as min poss. */
		if (nLargestC0Idx > 0) {
			nMinIdx = nGPIdx[clusters[0].pIdx[nLargestC0Idx]];
			if ((CJK_SPLIT_GET_GP_VAL(bSplitX, pGP[nMinIdx+1]) <= CJK_SPLIT_GET_GP_VAL(bSplitX, pGP[nMinIdx+2]) && bIsStrokeStart[i+2]) || 
				CJK_SPLIT_GET_GP_VAL(!bSplitX, pGP[nMinIdx+1]) > CJK_SPLIT_GET_GP_VAL(!bSplitX, pGP[nMinIdx+2])) {
				nMinIdx += 1;
			}
		}
		if (nSmallestC1Idx > 0) {
			nMaxIdx = nGPIdx[clusters[1].pIdx[nSmallestC1Idx]] - 1;
		}			
		if (nMinIdx < 0) nMinIdx = 2;
		if (nMaxIdx < 0) nMaxIdx = nGPs-4;

		/* Only add idxs which are do not regress in next point  */
		/* (because then that point will be better) */
		for (i=nMinIdx; i<=nMaxIdx; i++) {
			if (bIsStrokeStart[i] && !bIsStrokeStart[i+1]) continue;

			/* TODO: Refine this to limit calculations */
			/* Now either move forward in next point or move backward but opposite in orthogonal */
			if ((CJK_SPLIT_GET_GP_VAL(bSplitX, pGP[i]) <= CJK_SPLIT_GET_GP_VAL(bSplitX, pGP[i+1]) && bIsStrokeStart[i+1]) || 
				CJK_SPLIT_GET_GP_VAL(!bSplitX, pGP[i]) > CJK_SPLIT_GET_GP_VAL(!bSplitX, pGP[i+1])) {
				splitIdx[nSplitIdxs].idx = (unsigned char) i;
				splitIdx[nSplitIdxs].maxPrevVal[0] = 0;
				splitIdx[nSplitIdxs].maxPrevVal[1] = 0;
				splitIdx[nSplitIdxs].minAfterVal = 30;
				nSplitIdxs++;
				if (nSplitIdxs >= CJK_SPLIT_IDX_BW) break;
			}
		}

		/* The splitIdxs are now sorted and we evaluate the largest overlaps */
		decumaAssert(nSplitIdxs <= CJK_SPLIT_IDX_BW);
		for (i=0; i<nGPs; i++) {
			for (j=0; j<nSplitIdxs; j++) {				
				if (i <= splitIdx[j].idx) {
					if (splitIdx[j].maxPrevVal[0] < CJK_SPLIT_GET_GP_VAL(bSplitX, pGP[i])) {
						splitIdx[j].maxPrevVal[1] = splitIdx[j].maxPrevVal[0];
						splitIdx[j].maxPrevVal[0] = CJK_SPLIT_GET_GP_VAL(bSplitX, pGP[i]);
					}
					else if (splitIdx[j].maxPrevVal[1] < CJK_SPLIT_GET_GP_VAL(bSplitX, pGP[i])) {
						splitIdx[j].maxPrevVal[1] = CJK_SPLIT_GET_GP_VAL(bSplitX, pGP[i]);
					}
				}
				else
				{
					splitIdx[j].minAfterVal = DECUMA_MIN(splitIdx[j].minAfterVal, CJK_SPLIT_GET_GP_VAL(bSplitX, pGP[i]));
				}
			}
		}

		splitValue = 0xFFFF;
		pnSplitIdx[0] = 0;
		/* Calculate splitValue for each of the splitIdxs and pick the best one */
		for (i=0; i < nSplitIdxs && splitValue > 0; i++) {
			/* Check for a clean split */
			if (splitIdx[i].minAfterVal >= splitIdx[i].maxPrevVal[0]) {
				splitValue = 0;
				pnSplitIdx[0] = splitIdx[i].idx;
				break;
			}
			else {
				int nBorder = splitIdx[i].minAfterVal + splitIdx[i].maxPrevVal[0];
				int tSplitValue = 0;

				/* Check for single intrusion point */
				if (splitIdx[i].maxPrevVal[0] > splitIdx[i].minAfterVal && splitIdx[i].maxPrevVal[1] < splitIdx[i].minAfterVal)
					nBorder = 2*splitIdx[i].minAfterVal;

				/* Calculate score */
				for (j=0; j<nGPs; j++) {
					tSplitValue += splitGetSinglePtPunishment(pGP[j], (j <= splitIdx[i].idx), bSplitX, nBorder);
					/*if (!bIsStrokeStart[j])
						tSplitValue += splitGetPtPunishment(pGP[j], pGP[j-1], (j <= splitIdx[i].idx), bSplitX, nBorder);
						*/
				}
				if (tSplitValue <= splitValue) {
					/* If equal choose largest in orthogonal direction */
					if (tSplitValue == splitValue && 
						CJK_SPLIT_GET_GP_VAL(!bSplitX, pGP[splitIdx[i].idx]) < CJK_SPLIT_GET_GP_VAL(!bSplitX, pGP[pnSplitIdx[0]])) {
							continue;
					}
					splitValue = tSplitValue;
					pnSplitIdx[0] = splitIdx[i].idx;
				}
			}
		}
		/* Normalize splitValue */
		/* TODO: remove magic number		 */
		decumaAssert(pnSplitIdx[0] >= 0 && pnSplitIdx[0] <= dltCCCompressGetNbrPoints(pCChar));
		splitValue = (isqrt(splitValue) * DECUMA_FEATURE_MAX) / (nGPs);
		decumaAssert(splitValue >= 0);
	}
	decumaAssert(pnSplitIdx[0] >= 0 && pnSplitIdx[0] <= dltCCCompressGetNbrPoints(pCChar));
	decumaAssert(splitValue >= 0);

	if (pnSplitIdx[0] == 0) splitValue = 0;		
	/* Calculate value for this split */
	return splitValue;
}	


static int dltCCharSetLeftRightSplit(CJK_COMPRESSED_CHAR * pCChar, CJK_SESSION * pSession) 
{
	if (pSession->nLeftRightSplitIndex > -1) {
		/* This value has already been set and not reset */
		return pSession->nLeftRightSplitScore;
	}
	pSession->nLeftRightSplitScore = dltCCharSetSplit(pCChar, 1, &pSession->nLeftRightSplitIndex, pSession);
	return pSession->nLeftRightSplitScore;
}

static int dltCCharSetTopDownSplit(CJK_COMPRESSED_CHAR * pCChar, CJK_SESSION * pSession) 
{
	if (pSession->nTopDownSplitIndex > -1) {
		/* This value has already been set and not reset */
		return pSession->nTopDownSplitScore;
	}
	pSession->nTopDownSplitScore = dltCCharSetSplit(pCChar, 0, &pSession->nTopDownSplitIndex, pSession);
	return pSession->nTopDownSplitScore;
}

static DECUMA_UINT32 cjkGetCoarseFeatureWithIdx(DECUMA_FEATURE      * pFeatures,
										DECUMA_UINT32         nFeatures,
										DECUMA_UINT32		  nIdx,
										CJK_COMPRESSED_CHAR * pChar,
										CJK_SESSION         * pSession,
										COARSE_SETTINGS     * pSettings) 
{
	return coarseFeatureWithIdx(pFeatures, nFeatures, nIdx, pChar, pSession, pSettings, 1);
}

#if 0
static DECUMA_UINT32 cjkGetCoarseDependentFeatures(DECUMA_FEATURE  * pbIsSet,
											int  * pFeatureIdxs,
											int    nFeatures) {
	int i, n;
	decumaAssert(nFeatures <= COARSE_NBR_FEATURES);

	
	decumaAssert(sizeof(pbIsSet[0]) == 1);
	/* It is an unhandled exception that pbIsSet does not have COARSE_NBR_FEATURES of elements */
	decumaMemset(pbIsSet, -1, COARSE_NBR_FEATURES);

	for (i=0; i<nFeatures; i++) {
		if (CJK_COARSE_FEATURE_NOT_SET(pbIsSet, pFeatureIdxs[i])) {
			n = coarseFeatureWithIdx(pbIsSet, COARSE_NBR_FEATURES, pFeatureIdxs[i], 0, 0, 0, 0);
		}
		decumaAssert(pbIsSet[pFeatureIdxs[i]] >= 0);
	}
	for (i=0; i<COARSE_NBR_FEATURES; i++) {
		if (CJK_COARSE_FEATURE_NOT_SET(pbIsSet, i)) {
			pbIsSet[i] = 0;
		}
		else {
			pbIsSet[i] = 1;
		}
	}
	return COARSE_NBR_FEATURES;	
}
#endif

static DECUMA_UINT32 coarseFeatureWithIdx(DECUMA_FEATURE    * pFeatures,
										DECUMA_UINT32         nFeatures,
										DECUMA_UINT32		  nIdx,
										CJK_COMPRESSED_CHAR * pChar,
										CJK_SESSION         * pSession,
										COARSE_SETTINGS     * pSettings,
										DECUMA_BOOL			  bCalculate) 
{
	int k;
	int nPoints;
	int nFeatureIdxOffSet;
	CJK_COARSE_FEATURES_SCRATCHPAD * pScratchpad = (CJK_COARSE_FEATURES_SCRATCHPAD *) pSession->pScratchpad;

	DECUMA_UNUSED_PARAM(nFeatures);

	if (bCalculate && dltCCCompressGetNbrPoints(pChar) == 0) {
		return 0;
	}
	if (bCalculate)
		nPoints = dltCCCompressGetNbrPoints(pChar);

	decumaAssert(nIdx >= 0 && nIdx < COARSE_NBR_FEATURES);
	decumaAssert(nIdx <= nFeatures);

	nFeatureIdxOffSet = 0;
	/* First -0- feature should always be number of points since this may be treated in a
		special manner in clustering and such.

		FeatureName[nFeatureIdxOffSet] = "nPoints";
	*/
	if (nIdx == nFeatureIdxOffSet) {
		pFeatures[nFeatureIdxOffSet] = 0;
		if (bCalculate) 
			pFeatures[nFeatureIdxOffSet] = LIMIT_TO_RANGE(dltCCCompressGetNbrPoints(pChar), 0, DECUMA_FEATURE_MAX);
	}
	nFeatureIdxOffSet += 1;

	/* Setting original decuma "fast" features 
		FeatureName[nFeatureIdxOffSet]   = "2*z1+z2";
		FeatureName[nFeatureIdxOffSet+1] = "dltSwiftDistSplitnum";
		FeatureName[nFeatureIdxOffSet+2] = "y1+y2";
		FeatureName[nFeatureIdxOffSet+3] = "y3+y4";
		FeatureName[nFeatureIdxOffSet+4] = "y1";
	*/	
	if (nIdx >= nFeatureIdxOffSet && nIdx < nFeatureIdxOffSet+5) {
		if (bCalculate) {
			DECUMA_INT32 z1, z2;
			SWIFT_DIST swiftDist;
			dltCoarseSetSwiftData(pChar, &swiftDist, pSession);
			z1 = swiftDist.x2 - swiftDist.x1;
			z2 = swiftDist.y1 + swiftDist.y2
				- swiftDist.y3 - swiftDist.y4;

			pFeatures[nFeatureIdxOffSet] = LIMIT_TO_RANGE((2*z1+z2) / 2, 0, DECUMA_FEATURE_MAX);
			pFeatures[nFeatureIdxOffSet+1] = LIMIT_TO_RANGE(swiftDist.splitnum, 0, DECUMA_FEATURE_MAX);
			pFeatures[nFeatureIdxOffSet+2] = LIMIT_TO_RANGE(3*(swiftDist.y1 + swiftDist.y2) / 8, 0, DECUMA_FEATURE_MAX);
			pFeatures[nFeatureIdxOffSet+3] = LIMIT_TO_RANGE(3*(swiftDist.y3 + swiftDist.y4) / 8, 0, DECUMA_FEATURE_MAX);
			pFeatures[nFeatureIdxOffSet+4] = LIMIT_TO_RANGE(swiftDist.y1, 0, DECUMA_FEATURE_MAX);
		}
		else {
			for (k=0; k<5; k++) {pFeatures[nFeatureIdxOffSet+k]=0;}
		}
	}
	nFeatureIdxOffSet += 5;

	/* Setting original decuma "fast" features 
		FeatureName[nFeatureIdxOffSet]   = "FirstPointYRelativeNext5;
		FeatureName[nFeatureIdxOffSet+1] = "First4PointsSumX";
		FeatureName[nFeatureIdxOffSet+2] = "First6PointsSumX";

		FeatureName[nFeatureIdxOffSet+3] = "First4PointsSumY";
		FeatureName[nFeatureIdxOffSet+4] = "First6PointsSumY";
	*/
	if (nIdx >= nFeatureIdxOffSet && nIdx < nFeatureIdxOffSet+5) {
		if (bCalculate) {
			CJK_STROKE s1;
			int i, y, y1, x;
			int nFirstGapY = 0, nSumY4 = 0, nSumX4 = 0;
			int nSumX = 0, nSumY = 0;
			CJK_GRIDPOINT_ITERATOR gi;

			dltCCharGetFirstStroke(pChar, &s1, pSession);
			y1 = CJK_STROKE_GET_Y(&s1, 1);
			dltGPIterInit(&gi, pChar, pSession);
			for (i = 0; i < 6; i++) {
				if (!CJK_GPITER_HAS_GRIDPOINT(&gi)) {
					break;
				}
				y = CJK_GP_GET_Y(CJK_GPITER_GET_GRIDPOINT(&gi));
				x = CJK_GP_GET_X(CJK_GPITER_GET_GRIDPOINT(&gi));
				/* Measure for y-pos of first point relative next 5 */
				if (i > 0) nFirstGapY = CJK_GP_MAX + (y-y1);
				if (i < 4) nSumX4 += x;
				if (i < 4) nSumY4 += y;
				nSumY += y;
				nSumX += x;

				CJK_GPITER_NEXT(&gi, pSession);
			}
			pFeatures[nFeatureIdxOffSet] = LIMIT_TO_RANGE((nFirstGapY*DECUMA_FEATURE_MAX) / (2*CJK_GP_MAX*5), 0, DECUMA_FEATURE_MAX);/* Translate to positive value */
			pFeatures[nFeatureIdxOffSet+1] = LIMIT_TO_RANGE((nSumX4*DECUMA_FEATURE_MAX) / (4*CJK_GP_MAX), 0, DECUMA_FEATURE_MAX);
			pFeatures[nFeatureIdxOffSet+2] = LIMIT_TO_RANGE((nSumX*DECUMA_FEATURE_MAX) / (6*CJK_GP_MAX), 0, DECUMA_FEATURE_MAX);
			pFeatures[nFeatureIdxOffSet+3] = LIMIT_TO_RANGE((nSumY4*DECUMA_FEATURE_MAX) / (4*CJK_GP_MAX), 0, DECUMA_FEATURE_MAX);
			pFeatures[nFeatureIdxOffSet+4] = LIMIT_TO_RANGE((nSumY*DECUMA_FEATURE_MAX) / (6*CJK_GP_MAX), 0, DECUMA_FEATURE_MAX);
		}
		else {
			for (k=0; k<5; k++) {pFeatures[nFeatureIdxOffSet+k]=0;}
		}

	}
	nFeatureIdxOffSet += 5;


	/* 
		FeatureName[nFeatureIdxOffSet] = "dltCcharIntersectionCount";
	*/
	if (nIdx == nFeatureIdxOffSet) {
		pFeatures[nFeatureIdxOffSet] = 0;
		if (bCalculate)
			pFeatures[nFeatureIdxOffSet] = LIMIT_TO_RANGE(dltCCharGetIntersectionCount(pChar, pSession), 0, DECUMA_FEATURE_MAX);
	}
	nFeatureIdxOffSet += 1;


	/*
	   Run through each point in the character and collect the features

	   == Radicals ================
	   = Person
	   FeatureName[nFeatureIdxOffSet] = "PersonRadical";
	   FeatureName[nFeatureIdxOffSet+1] = "PersonRadical-Dir";
	   = Thread 
	   = Hand
	   = Animal
	   = Three-drops
	   = Speaking
	   = Grass

	 */
	if (nIdx >= nFeatureIdxOffSet && nIdx < nFeatureIdxOffSet+10) {
		/* Match for person-radical or combo (min-min) */
		if (bCalculate) {
			CJK_DISTANCE dist, angDist;

			/* Person */
			dltCoarseGetDistRadicalPerson(pChar, &dist, &angDist, pSession);
			pFeatures[nFeatureIdxOffSet] = LIMIT_TO_RANGE(dist, 0, DECUMA_FEATURE_MAX);
			pFeatures[nFeatureIdxOffSet+1] = LIMIT_TO_RANGE(angDist, 0, DECUMA_FEATURE_MAX);

			/* Hand */
			dltCoarseGetDistRadicalHand(pChar, &dist, pSession);
			pFeatures[nFeatureIdxOffSet+2] = LIMIT_TO_RANGE(dist, 0, DECUMA_FEATURE_MAX);

			/* Threedrops */
			dltCoarseGetDistRadicalThreedrops(pChar, &dist, pSession);
			pFeatures[nFeatureIdxOffSet+3] = LIMIT_TO_RANGE(dist, 0, DECUMA_FEATURE_MAX);

			/* Thread */
			dltCoarseGetDistRadicalThread(pChar, &dist, pSession);
			pFeatures[nFeatureIdxOffSet+4] = LIMIT_TO_RANGE(dist, 0, DECUMA_FEATURE_MAX);

			/* Combined features */
			pFeatures[nFeatureIdxOffSet+5] = DECUMA_MIN(pFeatures[nFeatureIdxOffSet], pFeatures[nFeatureIdxOffSet+2]);
			pFeatures[nFeatureIdxOffSet+6] = DECUMA_MIN(pFeatures[nFeatureIdxOffSet+2], pFeatures[nFeatureIdxOffSet+3]);
			pFeatures[nFeatureIdxOffSet+7] = DECUMA_MIN(pFeatures[nFeatureIdxOffSet+5], pFeatures[nFeatureIdxOffSet+6]);
			pFeatures[nFeatureIdxOffSet+8] = DECUMA_MIN(pFeatures[nFeatureIdxOffSet], pFeatures[nFeatureIdxOffSet+4]);
			pFeatures[nFeatureIdxOffSet+9] = DECUMA_MIN(pFeatures[nFeatureIdxOffSet+7], pFeatures[nFeatureIdxOffSet+8]);
		}
		else {
			for (k=0; k<10; k++) {pFeatures[nFeatureIdxOffSet+k]=0;}
		}
	}
	nFeatureIdxOffSet += 10;

	/*
	   Treat the first incoming stroke as a component and match all points to something

	   FeatureName[nFeatureIdxOffSet] = "Stroke1_point2_yvalue";
	   FeatureName[nFeatureIdxOffSet+1] = "Fnutt-left";
	   FeatureName[nFeatureIdxOffSet+2] = "Fnutt-right";
	   FeatureName[nFeatureIdxOffSet+3] = "Fnutt-either";
	   FeatureName[nFeatureIdxOffSet+4] = "Top_Horizontal_stroke";
	   FeatureName[nFeatureIdxOffSet+5] = "Left_Vertical_stroke";
	 */

	if (nIdx >= nFeatureIdxOffSet && nIdx < nFeatureIdxOffSet+6) {
		if (bCalculate) {
			CJK_STROKE s1;
	
			dltCCharGetFirstStroke(pChar, &s1, pSession);

			pFeatures[nFeatureIdxOffSet] = DECUMA_FEATURE_MAX;
			if (CJK_STROKE_NPOINTS(&s1) > 1) {
				pFeatures[nFeatureIdxOffSet] = CJK_STROKE_GET_Y(&s1, 2);
			}
			pFeatures[nFeatureIdxOffSet+1] = DECUMA_FEATURE_MAX;
			pFeatures[nFeatureIdxOffSet+2] = DECUMA_FEATURE_MAX;
			pFeatures[nFeatureIdxOffSet+3] = DECUMA_FEATURE_MAX;
			pFeatures[nFeatureIdxOffSet+4] = DECUMA_FEATURE_MAX;
			pFeatures[nFeatureIdxOffSet+5] = DECUMA_FEATURE_MAX;
			if (CJK_STROKE_NPOINTS(&s1) <= 10) {
				CJK_COMPRESSED_CHAR_DATA pCcDataFirstStrokeIn[13];
				/* Static data for matching "fnutt"-left */
				static const CJK_COMPRESSED_CHAR_DATA pCcDataFnutt1[5] = {4, 2, CJK_GP(2, 1), CJK_GP(2, 4), 0};
				static const CJK_COMPRESSED_CHAR_DATA pCcDataFnutt2[5] = {4, 2, CJK_GP(2, 3), CJK_GP(4, 3), 0};
				static const CJK_COMPRESSED_CHAR_DATA pCcDataFnutt3[5] = {4, 2, CJK_GP(4, 1), CJK_GP(1, 5), 0};

				/* Static data for matching "fnutt"-right */
				static const CJK_COMPRESSED_CHAR_DATA pCcDataFnutt4[5] = {4, 2, CJK_GP(13, 1), CJK_GP(13, 4), 0};
				static const CJK_COMPRESSED_CHAR_DATA pCcDataFnutt5[5] = {4, 2, CJK_GP(12, 2), CJK_GP(14, 5), 0};

				/* Static data for matching "horizontal upper stroke" */
				static const CJK_COMPRESSED_CHAR_DATA pCcDataHor1[5] = {4, 2, CJK_GP(2, 2), CJK_GP(13, 2), 0};
				static const CJK_COMPRESSED_CHAR_DATA pCcDataHor2[5] = {4, 2, CJK_GP(2, 0), CJK_GP(13, 0), 0};
				static const CJK_COMPRESSED_CHAR_DATA pCcDataHor3[5] = {4, 2, CJK_GP(2, 4), CJK_GP(13, 4), 0};

				/* Static data for matching "vertical stroke in left part" */
				static const CJK_COMPRESSED_CHAR_DATA pCcDataVer1[5] = {4, 2, CJK_GP(2, 2), CJK_GP(2, 13), 0};
				static const CJK_COMPRESSED_CHAR_DATA pCcDataVer2[5] = {4, 2, CJK_GP(2, 4), CJK_GP(1, 14), 0};

				CJK_COMPRESSED_CHAR * pCCFirstStrokeIn = pScratchpad->pCCFirstStrokeIn;
				CJK_COMPRESSED_CHAR * pCCFirstStrokeDb = pScratchpad->pCCFirstStrokeDb;
				DECUMA_UINT32 nMatchIdx;
				CJK_DISTANCE dist, bestDist;
				int j;

				pCcDataFirstStrokeIn[0] = CJK_STROKE_NPOINTS(&s1)+2;
				pCcDataFirstStrokeIn[1] = CJK_STROKE_NPOINTS(&s1);
				for (j = 1; j <= CJK_STROKE_NPOINTS(&s1); j++) {
					pCcDataFirstStrokeIn[j+1] = *cjkStrokeGetGridpoint(&s1, j);
				}
				pCcDataFirstStrokeIn[CJK_STROKE_NPOINTS(&s1)+2] = 0;

				dltCCCompressSetCCData(pCCFirstStrokeIn, pCcDataFirstStrokeIn);
				dltCCCompressSetNbrPoints(pCCFirstStrokeIn, CJK_STROKE_NPOINTS(&s1));
				dltCCCompressSetNbrStrokes(pCCFirstStrokeIn, 1);

				dltCCCompressSetNbrPoints(pCCFirstStrokeDb, 2);
				dltCCCompressSetNbrStrokes(pCCFirstStrokeDb, 1);

				/* Matching first stroke as fnutt (consume all points in first stroke) */
				dltCCCompressSetCCData(pCCFirstStrokeDb,pCcDataFnutt1);
				bestDist = dltCCharGetDTWDistanceComponent(pCCFirstStrokeDb, pCCFirstStrokeIn, &nMatchIdx, 0, pSession);
				dltCCCompressSetCCData(pCCFirstStrokeDb,pCcDataFnutt2);
				dist = dltCCharGetDTWDistanceComponent(pCCFirstStrokeDb, pCCFirstStrokeIn, &nMatchIdx, 0, pSession);
				if (dist < bestDist) bestDist = dist;
				dltCCCompressSetCCData(pCCFirstStrokeDb,pCcDataFnutt3);
				dist = dltCCharGetDTWDistanceComponent(pCCFirstStrokeDb, pCCFirstStrokeIn, &nMatchIdx, 0, pSession);
				if (dist < bestDist) bestDist = dist;
				/* Convert to good range (0, DECUMA_FEATURE_MAX) - cut off at really bad match */
				bestDist = (bestDist * DECUMA_FEATURE_MAX) / (dltCCCompressGetNbrPoints(pCCFirstStrokeIn) * BAD_DTW_POINT_DIST);
				pFeatures[nFeatureIdxOffSet+1] = LIMIT_TO_RANGE(bestDist, 0, DECUMA_FEATURE_MAX);

				/* Matching first stroke as right fnutt (consume all points in first stroke) */
				dltCCCompressSetCCData(pCCFirstStrokeDb,pCcDataFnutt4);
				bestDist = dltCCharGetDTWDistanceComponent(pCCFirstStrokeDb, pCCFirstStrokeIn, &nMatchIdx, 0, pSession);
				dltCCCompressSetCCData(pCCFirstStrokeDb,pCcDataFnutt5);
				dist = dltCCharGetDTWDistanceComponent(pCCFirstStrokeDb, pCCFirstStrokeIn, &nMatchIdx, 0, pSession);
				if (dist < bestDist) bestDist = dist;
				/* Convert to good range (0, DECUMA_FEATURE_MAX) - cut off at really bad match */
				bestDist = (bestDist * DECUMA_FEATURE_MAX) / (dltCCCompressGetNbrPoints(pCCFirstStrokeIn) * BAD_DTW_POINT_DIST);
				pFeatures[nFeatureIdxOffSet+2] = LIMIT_TO_RANGE(bestDist, 0, DECUMA_FEATURE_MAX);

				/* Any fnutt */
				pFeatures[nFeatureIdxOffSet+3] = DECUMA_MIN(pFeatures[nFeatureIdxOffSet+1], pFeatures[nFeatureIdxOffSet+2]);


				/* Matching horizontal stroke (consume all points in first stroke) */
				dltCCCompressSetCCData(pCCFirstStrokeDb,pCcDataHor1);
				bestDist = dltCCharGetDTWDistanceComponent(pCCFirstStrokeDb, pCCFirstStrokeIn, &nMatchIdx, 0, pSession);
				dltCCCompressSetCCData(pCCFirstStrokeDb,pCcDataHor2);
				dist = dltCCharGetDTWDistanceComponent(pCCFirstStrokeDb, pCCFirstStrokeIn, &nMatchIdx, 0, pSession);
				if (dist < bestDist) bestDist = dist;
				dltCCCompressSetCCData(pCCFirstStrokeDb,pCcDataHor3);
				dist = dltCCharGetDTWDistanceComponent(pCCFirstStrokeDb, pCCFirstStrokeIn, &nMatchIdx, 0, pSession);
				if (dist < bestDist) bestDist = dist;
				/* Convert to good range (0, DECUMA_FEATURE_MAX) - cut off at really bad match */
				bestDist = (bestDist * DECUMA_FEATURE_MAX) / (dltCCCompressGetNbrPoints(pCCFirstStrokeIn) * BAD_DTW_POINT_DIST);
				pFeatures[nFeatureIdxOffSet+4] = LIMIT_TO_RANGE(bestDist, 0, DECUMA_FEATURE_MAX);


				/* Matching vertical stroke (consume all points in first stroke) */
				dltCCCompressSetCCData(pCCFirstStrokeDb,pCcDataVer1);
				bestDist = dltCCharGetDTWDistanceComponent(pCCFirstStrokeDb, pCCFirstStrokeIn, &nMatchIdx, 0, pSession);
				dltCCCompressSetCCData(pCCFirstStrokeDb,pCcDataVer2);
				dist = dltCCharGetDTWDistanceComponent(pCCFirstStrokeDb, pCCFirstStrokeIn, &nMatchIdx, 0, pSession);
				if (dist < bestDist) bestDist = dist;
				/* Convert to good range (0, DECUMA_FEATURE_MAX) - cut off at really bad match */
				bestDist = (bestDist * DECUMA_FEATURE_MAX) / (dltCCCompressGetNbrPoints(pCCFirstStrokeIn) * BAD_DTW_POINT_DIST);
				pFeatures[nFeatureIdxOffSet+5] = LIMIT_TO_RANGE(bestDist, 0, DECUMA_FEATURE_MAX);
			}
		}
		else {
			for (k=0; k<6; k++) {pFeatures[nFeatureIdxOffSet+k]=0;}
		}
	}
	nFeatureIdxOffSet += 6;

	/* Directional-pattern matching features */
	/* -> : 0 */
	/*   / */
	/* |/_ : 1 */
	/*  | */
	/* \|/ : 2 */
	/*if (nIdx >= nFeatureIdxOffSet && nIdx < nFeatureIdxOffSet) {
		unsigned char directions[CC_MAXCCHARSIZE];
		unsigned char x, y;
		CJK_GRIDPOINT gp, prevgp;
		CJK_GRIDPOINT_ITERATOR gpI;
		int nDirections = 0, nGPIdx = 0;
		// Find left properties			
		for (dltGPIterInit(&gpI, pChar, pSession); nGPIdx < nPoints && CJK_GPITER_HAS_GRIDPOINT(&gpI); nGPIdx++) {
			gp = CJK_GPITER_GET_GRIDPOINT(&gpI);
			x = CJK_GP_GET_X(gp);
			y = CJK_GP_GET_Y(gp);
			// Check that we need to calculate directions
			if (nGPIdx > 0) {
				// Calculate segments compared to last
				if (y > CJK_GP_GET_Y(prevgp)) {
					if (ABS(x-CJK_GP_GET_X(prevgp)) > ABS(y-CJK_GP_GET_Y(prevgp)) && x > CJK_GP_GET_X(prevgp)) {
						// Horizontal segment
						directions[nDirections++] = 0;
					}
					else {		
						nLeftVerticals++;
						nLeftVerticalSum += y-CJK_GP_GET_Y(prevgp);
					}
				}
				else {
					if (y > CJK_GP_GET_Y(prevgp)) {
						if (ABS(x-CJK_GP_GET_X(prevgp)) > ABS(y-CJK_GP_GET_Y(prevgp))) {
							nRightHorizontals++;
							nRightHorizontalSum += ABS(x-CJK_GP_GET_X(prevgp));
						}
						else {
							nRightVerticals++;
							nRightVerticalSum += y-CJK_GP_GET_Y(prevgp);
						}
					}
				}
			}				
			prevgp = gp;
			CJK_GPITER_NEXT(&gpI, pSession);				
		}
	}*/
	/* nFeatureIdxOffSet += ??; */


	/* DENSE-specialized features - curvature and sharp angle detection */
	if (nIdx >= nFeatureIdxOffSet && nIdx < nFeatureIdxOffSet+29) {
		/* Calculate curvature and detect sharp points */
		/* For now use second derivative */
		long angle, prevAngle;
		CJK_GRIDPOINT gp, prevgp = 0, pprevgp;
		CJK_GRIDPOINT_ITERATOR gpI;
		int nSharpPoints = 0, nGPIdx = 0, nCurvatureSum = 0, nBackCurvatureSum = 0;
		int nFlatPoints = 0, nFlatPointsPriorToSharp[2] = {0, 0}, nFlatPointsAfterSharp[2] = {0, 0};
		int nCurvaturePriorToSharp[2] = {0, 0}, nCurvatureAfterSharp[2] = {0, 0};
		int nHorizontalPixelsPriorToSharp[2] = {0, 0}, nVerticalPixelsPriorToSharp[2] = {0, 0};
		int nHorizontalPixelsAfterSharp[2] = {0, 0}, nVerticalPixelsAfterSharp[2] = {0, 0};
		int curvature, j;
		const int nMaxNbrSharpPoints = 2; /* (lazy) instead of define */

		/* Find left properties */
		if (bCalculate) {
			for (dltGPIterInit(&gpI, pChar, pSession); nGPIdx < nPoints && CJK_GPITER_HAS_GRIDPOINT(&gpI); nGPIdx++) {
				gp = CJK_GPITER_GET_GRIDPOINT(&gpI);
				/* Get current angle backwards */
				if  (nGPIdx > 0)
					angle = iatan2_1024(CJK_GP_GET_X(gp)-CJK_GP_GET_X(prevgp), -CJK_GP_GET_Y(gp)+CJK_GP_GET_Y(prevgp)); /* coordinate system reversed */
				/* Start to analyze curvature from 3 pt */
				if (nGPIdx > 1) {
					/* Difference in angle is curvature so we take difference */
					curvature = angle - prevAngle; /* should be within (-3141, 3141) */
					if (curvature > 3141) curvature-= 6283;
					if (curvature < -3141) curvature+= 6283;
					curvature = (curvature*DECUMA_FEATURE_MAX) /  3141;
					if (curvature < 0) {
						nBackCurvatureSum += ABS(curvature);
					}
					else {
						nCurvatureSum += ABS(curvature);
					}
					if (ABS(curvature) < CJK_COARSE_FLAT_CURVE_LIMIT) {
						nFlatPoints++;
					}
					/* Make a split based on sharp point */
					if (nSharpPoints < nMaxNbrSharpPoints) {
						for (j=nSharpPoints; j<nMaxNbrSharpPoints; j++) {
							nFlatPointsPriorToSharp[j]++;
							nCurvaturePriorToSharp[j]+=ABS(curvature);
							/* vertical */
							if (prevAngle > -2300 && prevAngle < -700) {
								nVerticalPixelsPriorToSharp[j] += ABS(CJK_GP_GET_Y(pprevgp)-CJK_GP_GET_Y(prevgp));
							}
							/* horizontal */
							if (prevAngle > -700 && prevAngle < 700) {
								nHorizontalPixelsPriorToSharp[j] += ABS(CJK_GP_GET_X(pprevgp)-CJK_GP_GET_X(prevgp));
							}
						}
					}
					if (nSharpPoints > 0) {
						for (j=0; j<DECUMA_MIN(nSharpPoints, nMaxNbrSharpPoints); j++) {
							nFlatPointsAfterSharp[j]++;
							nCurvatureAfterSharp[j]+=ABS(curvature);
							/* vertical */
							if (prevAngle > -2300 && prevAngle < -700) {
								nVerticalPixelsAfterSharp[j] += ABS(CJK_GP_GET_Y(pprevgp)-CJK_GP_GET_Y(prevgp));
							}
							/* horizontal */
							if (prevAngle > -700 && prevAngle < 700) {
								nHorizontalPixelsAfterSharp[j] += ABS(CJK_GP_GET_X(pprevgp)-CJK_GP_GET_X(prevgp));
							}
						}
					}
					if (ABS(curvature) > CJK_COARSE_SHARP_CURVE_LIMIT) {
						nSharpPoints++;
						/* Make further features from differentiation */
					}
				}
				if (nGPIdx > 0) {
					prevAngle = angle;
				}
				pprevgp = prevgp;
				prevgp = gp;
				CJK_GPITER_NEXT(&gpI, pSession);				
			}
			pFeatures[nFeatureIdxOffSet] = LIMIT_TO_RANGE(nBackCurvatureSum / 6, 0, DECUMA_FEATURE_MAX);
			pFeatures[nFeatureIdxOffSet+1] = LIMIT_TO_RANGE(nCurvatureSum / 6, 0, DECUMA_FEATURE_MAX);
			pFeatures[nFeatureIdxOffSet+2] = LIMIT_TO_RANGE((nCurvatureSum + nBackCurvatureSum)/ 6, 0, DECUMA_FEATURE_MAX);
			/* Means */
			pFeatures[nFeatureIdxOffSet+3] = LIMIT_TO_RANGE(nBackCurvatureSum / nPoints, 0, DECUMA_FEATURE_MAX);
			pFeatures[nFeatureIdxOffSet+4] = LIMIT_TO_RANGE(nCurvatureSum / nPoints, 0, DECUMA_FEATURE_MAX);
			pFeatures[nFeatureIdxOffSet+5] = LIMIT_TO_RANGE((nCurvatureSum + nBackCurvatureSum)/ nPoints, 0, DECUMA_FEATURE_MAX);
			/* Flatness */
			pFeatures[nFeatureIdxOffSet+6] = LIMIT_TO_RANGE((nFlatPoints *DECUMA_FEATURE_MAX)/ nPoints, 0, DECUMA_FEATURE_MAX);
			/* Sharp split */
			for (j=0; j<nMaxNbrSharpPoints; j++) {
				pFeatures[nFeatureIdxOffSet+j*11+7] = LIMIT_TO_RANGE(nFlatPointsPriorToSharp[j], 0, DECUMA_FEATURE_MAX);
				pFeatures[nFeatureIdxOffSet+j*11+8] = LIMIT_TO_RANGE((nFlatPointsPriorToSharp[j] *DECUMA_FEATURE_MAX)/ nPoints, 0, DECUMA_FEATURE_MAX);
				if (nFlatPoints > 0)
					pFeatures[nFeatureIdxOffSet+j*11+9] = LIMIT_TO_RANGE((nFlatPointsPriorToSharp[j] *DECUMA_FEATURE_MAX)/ nFlatPoints, 0, DECUMA_FEATURE_MAX);
				else
					pFeatures[nFeatureIdxOffSet+j*11+9] = 0;
				pFeatures[nFeatureIdxOffSet+j*11+10] = LIMIT_TO_RANGE((nFlatPointsAfterSharp[j] *DECUMA_FEATURE_MAX)/ nPoints, 0, DECUMA_FEATURE_MAX);
				pFeatures[nFeatureIdxOffSet+j*11+11] = LIMIT_TO_RANGE(nFlatPointsAfterSharp[j], 0, DECUMA_FEATURE_MAX);
				pFeatures[nFeatureIdxOffSet+j*11+12] = LIMIT_TO_RANGE(nVerticalPixelsPriorToSharp[j], 0, DECUMA_FEATURE_MAX);
				pFeatures[nFeatureIdxOffSet+j*11+13] = LIMIT_TO_RANGE(nHorizontalPixelsPriorToSharp[j], 0, DECUMA_FEATURE_MAX);
				pFeatures[nFeatureIdxOffSet+j*11+14] = LIMIT_TO_RANGE(nVerticalPixelsAfterSharp[j], 0, DECUMA_FEATURE_MAX);
				pFeatures[nFeatureIdxOffSet+j*11+15] = LIMIT_TO_RANGE(nHorizontalPixelsAfterSharp[j], 0, DECUMA_FEATURE_MAX);
				pFeatures[nFeatureIdxOffSet+j*11+16] = LIMIT_TO_RANGE(nCurvaturePriorToSharp[j], 0, DECUMA_FEATURE_MAX);
				pFeatures[nFeatureIdxOffSet+j*11+17] = LIMIT_TO_RANGE(nCurvatureAfterSharp[j], 0, DECUMA_FEATURE_MAX);
			}
		}
		else {
			for (k=0; k<29; k++) {pFeatures[nFeatureIdxOffSet+k]=0;}
		}
	}
	nFeatureIdxOffSet += 29;

	/* Make Left-Right Segment clustering (x-coord) */
	/* where Segments are defined as all pendown in between point strokes */
	if (nIdx >= nFeatureIdxOffSet && nIdx < nFeatureIdxOffSet+18) {
		if (bCalculate) {
			if (nIdx >= nFeatureIdxOffSet && nIdx < nFeatureIdxOffSet+2) {
				pFeatures[nFeatureIdxOffSet] = LIMIT_TO_RANGE(dltCCharSetLeftRightSplit(pChar, pSession), 0, DECUMA_FEATURE_MAX);
				decumaAssert(nPoints > pSession->nLeftRightSplitIndex);
				pFeatures[nFeatureIdxOffSet+1] = ((nPoints-(pSession->nLeftRightSplitIndex+1))*DECUMA_FEATURE_MAX) / nPoints;
			}
			/* Add more features with regard to left-right split */
			if (nIdx >= nFeatureIdxOffSet+2) {
				int nGPIdx = 0;
				unsigned char x, y;
				CJK_GRIDPOINT gp, prevgp;
				CJK_GRIDPOINT_ITERATOR gpI;
				int nLeftHorizontals = 0, nLeftVerticals = 0, nRightHorizontals = 0,nRightVerticals = 0;
				int nLeftHorizontalSum = 0, nLeftVerticalSum = 0, nRightHorizontalSum = 0, nRightVerticalSum = 0;
				int nLeftSumY = 0, nRightSumY = 0;

				/* Find left properties			 */
				for (dltGPIterInit(&gpI, pChar, pSession); nGPIdx < nPoints && CJK_GPITER_HAS_GRIDPOINT(&gpI); nGPIdx++) {
					gp = CJK_GPITER_GET_GRIDPOINT(&gpI);
					x = CJK_GP_GET_X(gp);
					y = CJK_GP_GET_Y(gp);				
					if (nGPIdx > 0) {
						if (nGPIdx <= pSession->nLeftRightSplitIndex) {
							nLeftSumY += y;
							/* Calculate segments compared to last */
							if (y > CJK_GP_GET_Y(prevgp)) {
								if (x-CJK_GP_GET_X(prevgp) >= ABS(y-CJK_GP_GET_Y(prevgp))) {
									nLeftHorizontals++;
									nLeftHorizontalSum += ABS(x-CJK_GP_GET_X(prevgp));
								}
								else if (ABS(x-CJK_GP_GET_X(prevgp)) <= ABS(y-CJK_GP_GET_Y(prevgp))) {
									nLeftVerticals++;
									nLeftVerticalSum += y-CJK_GP_GET_Y(prevgp);
								}
							}
							else if (x-CJK_GP_GET_X(prevgp)) {
								nLeftHorizontals++;
								nLeftHorizontalSum += ABS(x-CJK_GP_GET_X(prevgp));
							}
						}
						else {
							nRightSumY += y;
							if (y > CJK_GP_GET_Y(prevgp)) {
								if (x-CJK_GP_GET_X(prevgp) >= ABS(y-CJK_GP_GET_Y(prevgp))) {
									if (x > CJK_GP_GET_X(prevgp)) {
										nRightHorizontals++;
										nRightHorizontalSum += ABS(x-CJK_GP_GET_X(prevgp));
									}
								}
								if (ABS(x-CJK_GP_GET_X(prevgp)) <= ABS(y-CJK_GP_GET_Y(prevgp))) {
									nRightVerticals++;
									nRightVerticalSum += y-CJK_GP_GET_Y(prevgp);
								}
							}
							else if (ABS(x-CJK_GP_GET_X(prevgp)) >= ABS(y-CJK_GP_GET_Y(prevgp)) && x > CJK_GP_GET_X(prevgp)) {
								nRightHorizontals++;
								nRightHorizontalSum += ABS(x-CJK_GP_GET_X(prevgp));
							}
						}
					}				
					prevgp = gp;
					CJK_GPITER_NEXT(&gpI, pSession);				
				}
				pFeatures[nFeatureIdxOffSet+2] = LIMIT_TO_RANGE(nLeftHorizontals, 0, DECUMA_FEATURE_MAX);
				pFeatures[nFeatureIdxOffSet+3] = LIMIT_TO_RANGE(nLeftHorizontalSum, 0, DECUMA_FEATURE_MAX);
				pFeatures[nFeatureIdxOffSet+4] = LIMIT_TO_RANGE(nLeftVerticals, 0, DECUMA_FEATURE_MAX);
				pFeatures[nFeatureIdxOffSet+5] = LIMIT_TO_RANGE(nLeftVerticalSum, 0, DECUMA_FEATURE_MAX);
				/* Right */
				pFeatures[nFeatureIdxOffSet+6] = LIMIT_TO_RANGE(nRightHorizontals, 0, DECUMA_FEATURE_MAX);
				pFeatures[nFeatureIdxOffSet+7] = LIMIT_TO_RANGE(nRightHorizontalSum, 0, DECUMA_FEATURE_MAX);
				pFeatures[nFeatureIdxOffSet+8] = LIMIT_TO_RANGE(nRightVerticals, 0, DECUMA_FEATURE_MAX);
				pFeatures[nFeatureIdxOffSet+9] = LIMIT_TO_RANGE(nRightVerticalSum, 0, DECUMA_FEATURE_MAX);
				/* Ratios */
				if (nRightHorizontals + nLeftHorizontals > 0)
					pFeatures[nFeatureIdxOffSet+10] = LIMIT_TO_RANGE((nRightHorizontals * DECUMA_FEATURE_MAX)/(nRightHorizontals + nLeftHorizontals), 0, DECUMA_FEATURE_MAX);
				else
					pFeatures[nFeatureIdxOffSet+10] = DECUMA_FEATURE_MAX;
				if (nRightHorizontalSum + nLeftHorizontalSum > 0)
					pFeatures[nFeatureIdxOffSet+11] = LIMIT_TO_RANGE((nRightHorizontalSum * DECUMA_FEATURE_MAX)/(nRightHorizontalSum + nLeftHorizontalSum), 0, DECUMA_FEATURE_MAX);
				else
					pFeatures[nFeatureIdxOffSet+11] = DECUMA_FEATURE_MAX;
				if (nRightVerticals + nLeftVerticals > 0)
					pFeatures[nFeatureIdxOffSet+12] = LIMIT_TO_RANGE((nRightVerticals * DECUMA_FEATURE_MAX)/(nRightVerticals + nLeftVerticals), 0, DECUMA_FEATURE_MAX);
				else
					pFeatures[nFeatureIdxOffSet+12] = DECUMA_FEATURE_MAX;
				if (nRightVerticalSum + nLeftVerticalSum > 0)
					pFeatures[nFeatureIdxOffSet+13] = LIMIT_TO_RANGE((nRightVerticalSum * DECUMA_FEATURE_MAX)/(nRightVerticalSum + nLeftVerticalSum), 0, DECUMA_FEATURE_MAX);
				else
					pFeatures[nFeatureIdxOffSet+13] = DECUMA_FEATURE_MAX;
				/* SumY */
				pFeatures[nFeatureIdxOffSet+14] = LIMIT_TO_RANGE(nLeftSumY, 0, DECUMA_FEATURE_MAX);
				pFeatures[nFeatureIdxOffSet+15] = LIMIT_TO_RANGE((nLeftSumY * DECUMA_FEATURE_MAX) / (CJK_GP_MAX * (pSession->nLeftRightSplitIndex+1)), 0, DECUMA_FEATURE_MAX);
				pFeatures[nFeatureIdxOffSet+16] = LIMIT_TO_RANGE(nRightSumY, 0, DECUMA_FEATURE_MAX);
				pFeatures[nFeatureIdxOffSet+17] = LIMIT_TO_RANGE((nRightSumY * DECUMA_FEATURE_MAX) / (CJK_GP_MAX * (pSession->nLeftRightSplitIndex+1)), 0, DECUMA_FEATURE_MAX);
			}
		}
		else {
			for (k=0; k<18; k++) {pFeatures[nFeatureIdxOffSet+k]=0;}
		}
	}
	nFeatureIdxOffSet += 18;

	if (nIdx >= nFeatureIdxOffSet && nIdx < nFeatureIdxOffSet+14) {
		if (bCalculate) {
			if (nIdx >= nFeatureIdxOffSet && nIdx < nFeatureIdxOffSet+2) {
				pFeatures[nFeatureIdxOffSet] = LIMIT_TO_RANGE(dltCCharSetTopDownSplit(pChar, pSession), 0, DECUMA_FEATURE_MAX);
				decumaAssert(nPoints > pSession->nTopDownSplitIndex && pSession->nTopDownSplitIndex >= 0);
				pFeatures[nFeatureIdxOffSet+1] = ((nPoints-(pSession->nTopDownSplitIndex+1))*DECUMA_FEATURE_MAX) / nPoints;
			}

			/* Add more features with regard to top-down split */
			if (nIdx >= nFeatureIdxOffSet+2) {
				int nGPIdx = 0;
				unsigned char x, y;
				CJK_GRIDPOINT gp, prevgp;
				CJK_GRIDPOINT_ITERATOR gpI;
				int nTopHorizontals = 0, nTopVerticals = 0, nBottomHorizontals = 0,nBottomVerticals = 0;
				int nTopHorizontalSum = 0, nTopVerticalSum = 0, nBottomHorizontalSum = 0, nBottomVerticalSum = 0;

				/* Find left properties			 */
				for (dltGPIterInit(&gpI, pChar, pSession); nGPIdx < nPoints && CJK_GPITER_HAS_GRIDPOINT(&gpI); nGPIdx++) {
					gp = CJK_GPITER_GET_GRIDPOINT(&gpI);
					x = CJK_GP_GET_X(gp);
					y = CJK_GP_GET_Y(gp);
					if (nGPIdx > 0) {
						if (nGPIdx <= pSession->nTopDownSplitIndex) {
							/* Calculate segments compared to last */
							if (y > CJK_GP_GET_Y(prevgp)) {
								if (x-CJK_GP_GET_X(prevgp) >= ABS(y-CJK_GP_GET_Y(prevgp))) {
									nTopHorizontals++;
									nTopHorizontalSum += ABS(x-CJK_GP_GET_X(prevgp));
								}
								if (ABS(x-CJK_GP_GET_X(prevgp)) <= ABS(y-CJK_GP_GET_Y(prevgp))) {
									nTopVerticals++;
									nTopVerticalSum += y-CJK_GP_GET_Y(prevgp);
								}
							}
							else if (x-CJK_GP_GET_X(prevgp) >= ABS(y-CJK_GP_GET_Y(prevgp)) && x > CJK_GP_GET_X(prevgp)) {
								nTopHorizontals++;
								nTopHorizontalSum += ABS(x-CJK_GP_GET_X(prevgp));
							}
						}
						else {
							if (y > CJK_GP_GET_Y(prevgp)) {
								if (x-CJK_GP_GET_X(prevgp) >= ABS(y-CJK_GP_GET_Y(prevgp))) {
									nBottomHorizontals++;
									nBottomHorizontalSum += ABS(x-CJK_GP_GET_X(prevgp));
								}
								if (ABS(x-CJK_GP_GET_X(prevgp)) <= ABS(y-CJK_GP_GET_Y(prevgp))) {
									nBottomVerticals++;
									nBottomVerticalSum += y-CJK_GP_GET_Y(prevgp);
								}
							}
							else if (x-CJK_GP_GET_X(prevgp) >= ABS(y-CJK_GP_GET_Y(prevgp)) && x > CJK_GP_GET_X(prevgp)) {
								nBottomHorizontals++;
								nBottomHorizontalSum += ABS(x-CJK_GP_GET_X(prevgp));
							}
						}
					}				
					prevgp = gp;
					CJK_GPITER_NEXT(&gpI, pSession);				
				}
				pFeatures[nFeatureIdxOffSet+2] = LIMIT_TO_RANGE(nTopHorizontals, 0, DECUMA_FEATURE_MAX);
				pFeatures[nFeatureIdxOffSet+3] = LIMIT_TO_RANGE(nTopHorizontalSum, 0, DECUMA_FEATURE_MAX);
				pFeatures[nFeatureIdxOffSet+4] = LIMIT_TO_RANGE(nTopVerticals, 0, DECUMA_FEATURE_MAX);
				pFeatures[nFeatureIdxOffSet+5] = LIMIT_TO_RANGE(nTopVerticalSum, 0, DECUMA_FEATURE_MAX);
				/* Right */
				pFeatures[nFeatureIdxOffSet+6] = LIMIT_TO_RANGE(nBottomHorizontals, 0, DECUMA_FEATURE_MAX);
				pFeatures[nFeatureIdxOffSet+7] = LIMIT_TO_RANGE(nBottomHorizontalSum, 0, DECUMA_FEATURE_MAX);
				pFeatures[nFeatureIdxOffSet+8] = LIMIT_TO_RANGE(nBottomVerticals, 0, DECUMA_FEATURE_MAX);
				pFeatures[nFeatureIdxOffSet+9] = LIMIT_TO_RANGE(nBottomVerticalSum, 0, DECUMA_FEATURE_MAX);
				/* Ratios */
				if (nBottomHorizontals + nTopHorizontals > 0)
					pFeatures[nFeatureIdxOffSet+10] = LIMIT_TO_RANGE((nBottomHorizontals * DECUMA_FEATURE_MAX)/(nBottomHorizontals + nTopHorizontals), 0, DECUMA_FEATURE_MAX);
				else
					pFeatures[nFeatureIdxOffSet+10] = DECUMA_FEATURE_MAX;
				if (nBottomHorizontalSum + nTopHorizontalSum > 0)
					pFeatures[nFeatureIdxOffSet+11] = LIMIT_TO_RANGE((nBottomHorizontalSum * DECUMA_FEATURE_MAX)/(nBottomHorizontalSum + nTopHorizontalSum), 0, DECUMA_FEATURE_MAX);
				else
					pFeatures[nFeatureIdxOffSet+11] = DECUMA_FEATURE_MAX;
				if (nBottomVerticals + nTopVerticals > 0)
					pFeatures[nFeatureIdxOffSet+12] = LIMIT_TO_RANGE((nBottomVerticals * DECUMA_FEATURE_MAX)/(nBottomVerticals + nTopVerticals), 0, DECUMA_FEATURE_MAX);
				else
					pFeatures[nFeatureIdxOffSet+12] = DECUMA_FEATURE_MAX;
				if (nBottomVerticalSum + nTopVerticalSum > 0)
					pFeatures[nFeatureIdxOffSet+13] = LIMIT_TO_RANGE((nBottomVerticalSum * DECUMA_FEATURE_MAX)/(nBottomVerticalSum + nTopVerticalSum), 0, DECUMA_FEATURE_MAX);
				else
					pFeatures[nFeatureIdxOffSet+13] = DECUMA_FEATURE_MAX;

			}
		}
		else {
			for (k=0; k<14; k++) {pFeatures[nFeatureIdxOffSet+k]=0;}
		}
	}
	nFeatureIdxOffSet += 14;

	if (nIdx >= nFeatureIdxOffSet && nIdx < nFeatureIdxOffSet+59) {

		/* 9-part Feature containers 
		[0] - number of feature points (coord)
		---- DIRECTION CHANGE
		[1] - number of direction change pSession (350>=  >=285) features 
		[2] - number of direction change s  (295>=  >=245) features 
		[3] - number of direction change e  (70>=   >=340) features 
		[4] - number of direction change sw (295>=  >=245) features 
		---- DIRECTIONAL
		[5] - number of N-S "pixels"
		[6] - number of E-W "pixels"
		[7] - number of SE-NW "pixels"
		[8] - number of NE-SW "pixels"
		*/
		unsigned char nNwFeatures[9], nSwFeatures[9], nNeFeatures[9], nSeFeatures[9];
		unsigned int nFeaturesSum[9];
		/* 2-part feature containers
		[0] - number of upper/left bars
		[1] - number of lower/right bars
		*/
		unsigned char nHorizontalBars[2] = {0,0}, nVerticalBars[2] = {0,0};
		
		int ymax, ymin, xmax, xmin;
		int xMidHigh, xMidLow, yMidHigh, yMidLow;

		CJK_GRIDPOINT gp, gpLast;
		CJK_GRIDPOINT_ITERATOR gpI;
		int nGPIdx = 0;

		int bIsSwDir = 0, bIsEDir = 0, bIsSDir = 0, bIsSeDir = 0;
		int nPixels = 0;

		if (bCalculate) {
			/* Set box to enclosing box if not set */
			xmax = (pSettings->xmax > MIN_DECUMA_INT16?pSettings->xmax:MIN_DECUMA_INT16);
			ymax = (pSettings->ymax > MIN_DECUMA_INT16?pSettings->ymax:MIN_DECUMA_INT16);
			xmin = (pSettings->xmin < MAX_DECUMA_INT16?pSettings->xmin:MAX_DECUMA_INT16);
			ymin = (pSettings->ymin < MAX_DECUMA_INT16?pSettings->ymin:MAX_DECUMA_INT16);

			if (xmax == MIN_DECUMA_INT16) {
				dltCCCompressGetMaxMin(pChar, &xmin, &xmax, &ymin, &ymax);
			}
			/* Set the grid parameters */
			{
				int dx = ((xmax-xmin)-(ymax-ymin)), dy = ((ymax-ymin)-(xmax-xmin));
				int bDoRound;
				/* Get box normalization parameters */
				if (dx >= 0) {
					if (dx % 2) {
						dx++;
					}
					dy = 0;
				}
				else {
					if (dy % 2) {
						dy++;
					}
					dx = 0;
				}
				/* Readjust the box-settings  */
				if (dx > 0) {
					xmax = xmax + (dx/2);
					xmin = xmin - (dx/2);
				}
				if (dy > 0) {
					ymin = ymin - (dy/2);
					ymax = ymax + (dy/2);
				}
				/* Set the middle parameters */
				bDoRound = ((5*(ymax+ymin)+(ymax-ymin))%10 >= 5?1:0);
				yMidHigh = (5*(ymax+ymin)+(ymax-ymin)) / 10;
				if (bDoRound) {
					yMidHigh++;
				}
				bDoRound = ((5*(ymax+ymin)-(ymax-ymin))%10 >= 5?1:0);
				yMidLow = (5*(ymax+ymin)-(ymax-ymin)) / 10;
				if (bDoRound) {
					yMidLow++;
				}
				bDoRound = ((5*(xmax+xmin)+(xmax-xmin))%10 >= 5?1:0);
				xMidHigh = (5*(xmax+xmin)+(xmax-xmin)) / 10;
				if (bDoRound) {
					xMidHigh++;
				}
				bDoRound = ((5*(xmax+xmin)-(xmax-xmin))%10 >= 5?1:0);
				xMidLow = (5*(xmax+xmin)-(xmax-xmin)) / 10;
				if (bDoRound) {
					xMidLow++;
				}
			}



			/* Initialization */
			decumaMemset(nNwFeatures, 0, 9*sizeof(nNwFeatures[0]));
			decumaMemset(nSwFeatures, 0, 9*sizeof(nSwFeatures[0]));
			decumaMemset(nNeFeatures, 0, 9*sizeof(nNeFeatures[0]));
			decumaMemset(nSeFeatures, 0, 9*sizeof(nSeFeatures[0]));
			decumaMemset(nFeaturesSum, 0, 9*sizeof(nFeaturesSum[0]));

			for (dltGPIterInit(&gpI, pChar, pSession); nGPIdx < nPoints && CJK_GPITER_HAS_GRIDPOINT(&gpI); nGPIdx++) {			
				int gpdir;
				int bIsLongX = 0, bIsLongY = 0, xMean = 0, yMean = 0;

				gp = CJK_GPITER_GET_GRIDPOINT(&gpI);

				if (nGPIdx > 0) {

					/* Extract coordinate and directional features */
					bIsSwDir = 0; bIsEDir = 0; bIsSDir = 0; bIsSeDir = 0;

					/* Get feature bin to put this direction in */
					gpdir = dltGPGetDirection(gpLast, gp);

					xMean = (CJK_GP_GET_X(gp) + CJK_GP_GET_X(gpLast)) / 2;
					yMean = (CJK_GP_GET_Y(gp) + CJK_GP_GET_Y(gpLast)) / 2;

					if (3*(CJK_GP_GET_Y(gp)-CJK_GP_GET_Y(gpLast)) > ymax) bIsLongY = 1;
					if (3*(CJK_GP_GET_X(gp)-CJK_GP_GET_X(gpLast)) > xmax) bIsLongX = 1;

					if (gpdir == 16 || gpdir == 17 || gpdir == 2) {
						bIsSwDir = 1;
					}
					if (gpdir == 35 || gpdir == 36 || gpdir == 20 || gpdir == 52) {
						bIsEDir = 1;
					}
					if (gpdir == 18 || gpdir == 2 || gpdir == 3 || gpdir == 4) {
						bIsSDir = 1;
					}
					if (gpdir == 4 || gpdir == 19 || gpdir == 20) {
						bIsSeDir = 1;
					}

					/* Check coordinate and direction change values */
					/* NW */
					if (CJK_GP_GET_X(gp)<=xMidHigh && CJK_GP_GET_Y(gp)<=yMidHigh) {
						nNwFeatures[0]++;
						if (bIsSeDir) nNwFeatures[1]++;
						if (bIsSDir) nNwFeatures[2]++;
						if (bIsEDir) nNwFeatures[3]++;
						if (bIsSwDir) nNwFeatures[4]++;
					}

					/* NE */
					if (CJK_GP_GET_X(gp)>=xMidLow && CJK_GP_GET_Y(gp)<=yMidHigh) {
						nNeFeatures[0]++;
						if (bIsSeDir) nNeFeatures[1]++;
						if (bIsSDir) nNeFeatures[2]++;
						if (bIsEDir) nNeFeatures[3]++;
						if (bIsSwDir) nNeFeatures[4]++;
					}

					/* SW -- (ygrid is upside down) */
					if (CJK_GP_GET_X(gp)<=xMidHigh && CJK_GP_GET_Y(gp)>=yMidLow) {
						nSwFeatures[0]++;
						if (bIsSeDir) nSwFeatures[1]++;
						if (bIsSDir) nSwFeatures[2]++;
						if (bIsEDir) nSwFeatures[3]++;
						if (bIsSwDir) nSwFeatures[4]++;
					}

					/* SE -- (ygrid is upside down) */
					if (CJK_GP_GET_X(gp)>=xMidLow && CJK_GP_GET_Y(gp)>=yMidLow) {
						nSeFeatures[0]++;
						if (bIsSeDir) nSeFeatures[1]++;
						if (bIsSDir) nSeFeatures[2]++;
						if (bIsEDir) nSeFeatures[3]++;
						if (bIsSwDir) nSeFeatures[4]++;
					}

					/* Sum */
					nFeaturesSum[0]++;
					if (bIsSeDir) nFeaturesSum[1]++;
					if (bIsSDir) nFeaturesSum[2]++;
					if (bIsEDir) nFeaturesSum[3]++;
					if (bIsSwDir) nFeaturesSum[4]++;


					/* 
					Check direction values 
					-- put the number of pixels in certain direction in 
					correct bin. For these features we do not use penup
					points
					*/				
					{
						int nPutPixels, i;
						int xLine, yLine;

						nPutPixels = isqrt(CJK_GP_GET_SQ_DISTANCE_TRUE(gpLast, gp));
						nPixels += nPutPixels;
						for (i = 0; i<nPutPixels; i++) {
							xLine = (nPutPixels*CJK_GP_GET_X(gpLast)+i*(CJK_GP_GET_X(gp)-CJK_GP_GET_X(gpLast))) / nPutPixels;
							yLine = (nPutPixels*CJK_GP_GET_X(gpLast)+i*(CJK_GP_GET_X(gp)-CJK_GP_GET_X(gpLast))) / nPutPixels;

							/* NW */
							if (xLine<=xMidHigh && yLine<=yMidHigh) {
								if (bIsSeDir) nNwFeatures[5]++;
								if (bIsSDir) nNwFeatures[6]++;
								if (bIsEDir) nNwFeatures[7]++;
								if (bIsSwDir) nNwFeatures[8]++;
							}

							/* NE */
							if (xLine>=xMidLow && yLine<=yMidHigh) {
								if (bIsSeDir) nNeFeatures[5]++;
								if (bIsSDir) nNeFeatures[6]++;
								if (bIsEDir) nNeFeatures[7]++;
								if (bIsSwDir) nNeFeatures[8]++;
							}

							/* SW -- (ygrid is upside down) */
							if (xLine<=xMidHigh && yLine>=yMidLow) {
								if (bIsSeDir) nSwFeatures[5]++;
								if (bIsSDir) nSwFeatures[6]++;
								if (bIsEDir) nSwFeatures[7]++;
								if (bIsSwDir) nSwFeatures[8]++;
							}

							/* SE -- (ygrid is upside down) */
							if (xLine>=xMidLow && yLine>=yMidLow) {
								if (bIsSeDir) nSeFeatures[5]++;
								if (bIsSDir) nSeFeatures[6]++;
								if (bIsEDir) nSeFeatures[7]++;
								if (bIsSwDir) nSeFeatures[8]++;
							}

							/* Sum */
							if (bIsSeDir) nFeaturesSum[5]++;
							if (bIsSDir) nFeaturesSum[6]++;
							if (bIsEDir) nFeaturesSum[7]++;
							if (bIsSwDir) nFeaturesSum[8]++;
						}					
					}

					/* Set number of horizontal and vertical bars */
					if (bIsSDir && bIsLongY && CJK_GP_GET_Y(gp) > yMidLow && CJK_GP_GET_Y(gpLast) < yMidHigh) {
						if (xMean < xMidHigh) nHorizontalBars[0]++;
						if (xMean > xMidLow) nHorizontalBars[1]++;
					}
					if (bIsEDir && bIsLongX && CJK_GP_GET_X(gp) > xMidLow && CJK_GP_GET_X(gpLast) < xMidHigh) {
						if (yMean < yMidHigh) nVerticalBars[0]++;
						if (yMean > yMidLow) nVerticalBars[1]++;
					}	
				}	
				gpLast = gp;
				CJK_GPITER_NEXT(&gpI, pSession);				
			}
			/**
			Number of horizontal and vertical bars

			FeatureName[nFeatureIdxOffSet] = "nHorizontal in upper half";
			FeatureName[nFeatureIdxOffSet+1] = "nHorizontal in lower half";
			FeatureName[nFeatureIdxOffSet+2] = "nVertical in left half";
			FeatureName[nFeatureIdxOffSet+3] = "nVertical in right half";
			*/
			pFeatures[nFeatureIdxOffSet] = LIMIT_TO_RANGE(nHorizontalBars[0], 0, DECUMA_FEATURE_MAX);
			pFeatures[nFeatureIdxOffSet+1] = LIMIT_TO_RANGE(nHorizontalBars[1], 0, DECUMA_FEATURE_MAX);
			pFeatures[nFeatureIdxOffSet+2] = LIMIT_TO_RANGE(nVerticalBars[0], 0, DECUMA_FEATURE_MAX);
			pFeatures[nFeatureIdxOffSet+3] = LIMIT_TO_RANGE(nVerticalBars[1], 0, DECUMA_FEATURE_MAX);

			/**
			Number of horizontal and vertical bars

			FeatureName[nFeatureIdxOffSet+4 - nFeatureIdxOffSet+12] = "nNwFeatures";
			FeatureName[nFeatureIdxOffSet+13 - nFeatureIdxOffSet+21] = "nNeFeatures";
			FeatureName[nFeatureIdxOffSet+22 - nFeatureIdxOffSet+30] = "nSwFeatures";
			FeatureName[nFeatureIdxOffSet+31 - nFeatureIdxOffSet+39] = "nSeFeatures";
			FeatureName[nFeatureIdxOffSet+40 - nFeatureIdxOffSet+48] = "nFeaturesSum";
			FeatureName[nFeatureIdxOffSet+49 - nFeatureIdxOffSet+57] = "nFeaturesSum - ratios";
			*/
			for (k=0; k<9; k++) {
				pFeatures[nFeatureIdxOffSet+4+k] = LIMIT_TO_RANGE(nNwFeatures[k], 0, DECUMA_FEATURE_MAX);
				pFeatures[nFeatureIdxOffSet+13+k] = LIMIT_TO_RANGE(nNeFeatures[k], 0, DECUMA_FEATURE_MAX);
				pFeatures[nFeatureIdxOffSet+22+k] = LIMIT_TO_RANGE(nSwFeatures[k], 0, DECUMA_FEATURE_MAX);
				pFeatures[nFeatureIdxOffSet+31+k] = LIMIT_TO_RANGE(nSeFeatures[k], 0, DECUMA_FEATURE_MAX);
				pFeatures[nFeatureIdxOffSet+40+k] = LIMIT_TO_RANGE(nFeaturesSum[k], 0, DECUMA_FEATURE_MAX);
				pFeatures[nFeatureIdxOffSet+49+k] = LIMIT_TO_RANGE((nFeaturesSum[k]* DECUMA_FEATURE_MAX) / (nPoints), 0, DECUMA_FEATURE_MAX);
			}
			/**
			Pixeldensity
			pFeatures[nFeatureIdxOffSet+58] = "nPixelDensity"
			*/
			pFeatures[nFeatureIdxOffSet+58] = LIMIT_TO_RANGE((nPixels * DECUMA_FEATURE_MAX) / (10 * CJK_GP_MAX), 0, DECUMA_FEATURE_MAX);
		}
		else {
			for (k=0; k<59; k++) {pFeatures[nFeatureIdxOffSet+k] = 0;}
		}
	}
	nFeatureIdxOffSet += 59;


	/*
	The NBR_PERIPHERAL_SEGMENTS Peripheral features derived from Kuo-Chin Fan		
	C1  - contains peripheral segments in the Top (= min-y) part of the character
	C2  - contains peripheral segments in the Bottom (= max-y) 
	C3  - contains peripheral segments in the Left (= min-x)
	C4  - contains peripheral segments in the Right (= max-x)
	*/
	if (nIdx >= nFeatureIdxOffSet && nIdx < nFeatureIdxOffSet+5) {
		PERIPHERAL_FEATURE C1, C2, C3, C4;
		CJK_STROKE s;
		int iStroke = 0, bIsLastStroke = 0;

		if (bCalculate) {
			decumaMemset(&C1, 0, sizeof(PERIPHERAL_FEATURE));
			decumaMemset(&C2, 0, sizeof(PERIPHERAL_FEATURE));
			decumaMemset(&C3, 0, sizeof(PERIPHERAL_FEATURE));	
			decumaMemset(&C4, 0, sizeof(PERIPHERAL_FEATURE));

			for (k = 0; k < NBR_PERIPHERAL_SEGMENTS; k++) {	
				C1.segments[k].high = MAX_DECUMA_INT16; C1.segments[k].low = MAX_DECUMA_INT16;
				C2.segments[k].high = MIN_DECUMA_INT16; C2.segments[k].low = MIN_DECUMA_INT16;
				C3.segments[k].high = MAX_DECUMA_INT16; C3.segments[k].low = MAX_DECUMA_INT16;
				C4.segments[k].high = MIN_DECUMA_INT16; C4.segments[k].low = MIN_DECUMA_INT16;
			}

			for (dltCCharGetFirstStroke(pChar, &s, pSession); CJK_STROKE_EXISTS(&s); cjkStrokeNext(&s, pSession), iStroke++) {
				CJK_GRIDPOINT gp = 0, gpLast = 0;
				int n;
				CJK_STROKE ss = s;

				cjkStrokeNext(&ss, pSession);
				if (!CJK_STROKE_EXISTS(&ss)) bIsLastStroke = 1;

				/* Run through the stroke and extract coordinate and directional features */
				for (n=0; n < CJK_STROKE_NPOINTS(&s); n++) {				
					if (n>0) gpLast = gp;
					gp = *cjkStrokeGetGridpoint(&s, n+1);

					/* Set Peripheral features - only for pendown points */
					if (n > 0) {				
						int segmentYMin = MIN(CJK_GP_GET_Y(gpLast), CJK_GP_GET_Y(gp));
						int segmentYMax = MAX(CJK_GP_GET_Y(gpLast), CJK_GP_GET_Y(gp));
						int segmentXMin = MIN(CJK_GP_GET_X(gpLast), CJK_GP_GET_X(gp));
						int segmentXMax = MAX(CJK_GP_GET_X(gpLast), CJK_GP_GET_X(gp));
						int gpLastDir = dltGPGetDirection(gpLast, gp);
						int iBubble;

						/* 
						Sort segments by "bubble-sort" 
						- Utilize initialization by MAX/MIN_DECUMA_INT16 to shorten if statement
						- for each C-group of peripheral features
						*/

						/* C1 - Top segments */
						if (C1.nSegments == 0 || (segmentYMin < C1.segments[C1.nSegments-1].low || 
							(segmentYMin == C1.segments[C1.nSegments-1].low && segmentYMax < C1.segments[C1.nSegments-1].high ))) {
								PERIPHERAL_SEGMENT yBubble, tmpSegment;

								yBubble.low = segmentYMin; yBubble.high = segmentYMax; yBubble.dir=gpLastDir;
								yBubble.gp[0] = gpLast; yBubble.gp[1] = gp;

								for (iBubble = 0; iBubble < NBR_PERIPHERAL_SEGMENTS; iBubble++) {
									if (yBubble.low < C1.segments[iBubble].low || 
										(yBubble.low == C1.segments[iBubble].low && yBubble.high < C1.segments[iBubble].high)) {
											tmpSegment = C1.segments[iBubble];
											C1.segments[iBubble] = yBubble;
											yBubble = tmpSegment;
									}
								}
								if (C1.nSegments < NBR_PERIPHERAL_SEGMENTS) C1.nSegments++;
						}

						/* C2 - Bottom segments */
						if (C2.nSegments == 0 || (segmentYMax > C2.segments[C2.nSegments-1].high || 
							(segmentYMax == C2.segments[C2.nSegments-1].high && segmentYMin > C2.segments[C2.nSegments-1].low ))) {
								PERIPHERAL_SEGMENT yBubble, tmpSegment;

								yBubble.low = segmentYMin; yBubble.high = segmentYMax; yBubble.dir=gpLastDir;
								yBubble.gp[0] = gpLast; yBubble.gp[1] = gp;

								for (iBubble = 0; iBubble < NBR_PERIPHERAL_SEGMENTS; iBubble++) {
									if (yBubble.low > C2.segments[iBubble].high || 
										(yBubble.low == C2.segments[iBubble].high && yBubble.high > C2.segments[iBubble].low)) {
											tmpSegment = C2.segments[iBubble];
											C2.segments[iBubble] = yBubble;
											yBubble = tmpSegment;
									}
								}
								if (C2.nSegments < NBR_PERIPHERAL_SEGMENTS) C2.nSegments++;
						}

						/* C3 - left segments */
						if (C3.nSegments == 0 || (segmentXMin < C3.segments[C3.nSegments-1].low || 
							(segmentXMin == C3.segments[C3.nSegments-1].low && segmentXMax < C3.segments[C3.nSegments-1].high ))) {
								PERIPHERAL_SEGMENT yBubble, tmpSegment;

								yBubble.low = segmentXMin; yBubble.high = segmentXMax; yBubble.dir=gpLastDir; 
								yBubble.gp[0] = gpLast; yBubble.gp[1] = gp;

								for (iBubble = 0; iBubble < NBR_PERIPHERAL_SEGMENTS; iBubble++) {
									if (yBubble.low < C3.segments[iBubble].low || 
										(yBubble.low == C3.segments[iBubble].low && yBubble.high < C3.segments[iBubble].high)) {
											tmpSegment = C3.segments[iBubble];
											C3.segments[iBubble] = yBubble;
											yBubble = tmpSegment;
									}
								}
								if (C3.nSegments < NBR_PERIPHERAL_SEGMENTS) C3.nSegments++;
						}

						/* C4 - Right segments */
						if (C4.nSegments == 0 || (segmentXMax > C4.segments[C4.nSegments-1].high || 
							(segmentXMax == C4.segments[C4.nSegments-1].high && segmentXMin > C4.segments[C4.nSegments-1].low ))) {
								PERIPHERAL_SEGMENT yBubble, tmpSegment;

								yBubble.low = segmentXMin; yBubble.high = segmentXMax; yBubble.dir=gpLastDir; 
								yBubble.gp[0] = gpLast; yBubble.gp[1] = gp;

								for (iBubble = 0; iBubble < NBR_PERIPHERAL_SEGMENTS; iBubble++) {
									if (yBubble.low > C4.segments[iBubble].high || 
										(yBubble.low == C4.segments[iBubble].high && yBubble.high > C4.segments[iBubble].low)) {
											tmpSegment = C4.segments[iBubble];
											C4.segments[iBubble] = yBubble;
											yBubble = tmpSegment;
									}
								}
								if (C4.nSegments < NBR_PERIPHERAL_SEGMENTS) C4.nSegments++;
						}
					}
				}
			}

			/**
			Peripheral features
			- sum of max -x, -sum of max -y
			- nHorizontalPixelSum
			- nVerticalPixelSum
			- nTotalPixelSum
			*/
			{
				int nMaxXSum = 0, nMaxYSum = 0;
				int nHorizontalPixelSum = 0, nVerticalPixelSum = 0;
				int nTotalPixelSum = 0;
				DECUMA_UINT8 x,y;

				for (k=0; k<C1.nSegments; k++) {
					x = CJK_GP_GET_X(C1.segments[k].gp[1]);
					y = CJK_GP_GET_Y(C1.segments[k].gp[1]);
					nMaxXSum += DECUMA_MAX(x, CJK_GP_GET_X(C1.segments[k].gp[0]));
					nMaxYSum += DECUMA_MAX(y, CJK_GP_GET_Y(C1.segments[k].gp[0]));
					if (y > CJK_GP_GET_Y(C1.segments[k].gp[0])) {
						if (ABS(x-CJK_GP_GET_X(C1.segments[k].gp[0])) > ABS(y-CJK_GP_GET_Y(C1.segments[k].gp[0]))) {
							nHorizontalPixelSum += ABS(x-CJK_GP_GET_X(C1.segments[k].gp[0]));
						}
						else {
							nVerticalPixelSum += y-CJK_GP_GET_Y(C1.segments[k].gp[0]);
						}
					}
					nTotalPixelSum += isqrt(CJK_GP_GET_SQ_DISTANCE_TRUE(C1.segments[k].gp[0], C1.segments[k].gp[1]));
				}
				pFeatures[nFeatureIdxOffSet] = LIMIT_TO_RANGE((nMaxXSum * DECUMA_FEATURE_MAX) / (NBR_PERIPHERAL_SEGMENTS * CJK_GP_MAX), 0, DECUMA_FEATURE_MAX);
				pFeatures[nFeatureIdxOffSet+1] = LIMIT_TO_RANGE((nMaxYSum * DECUMA_FEATURE_MAX) / (NBR_PERIPHERAL_SEGMENTS * CJK_GP_MAX), 0, DECUMA_FEATURE_MAX);
				pFeatures[nFeatureIdxOffSet+2] = LIMIT_TO_RANGE((nHorizontalPixelSum * DECUMA_FEATURE_MAX) / (NBR_PERIPHERAL_SEGMENTS * CJK_GP_MAX), 0, DECUMA_FEATURE_MAX);
				pFeatures[nFeatureIdxOffSet+3] = LIMIT_TO_RANGE((nVerticalPixelSum * DECUMA_FEATURE_MAX) / (NBR_PERIPHERAL_SEGMENTS * CJK_GP_MAX), 0, DECUMA_FEATURE_MAX);
				pFeatures[nFeatureIdxOffSet+4] = LIMIT_TO_RANGE((nTotalPixelSum * DECUMA_FEATURE_MAX * 3) / (2 * NBR_PERIPHERAL_SEGMENTS * CJK_GP_MAX), 0, DECUMA_FEATURE_MAX);
			}
		}
		else {
			for (k=0; k<5; k++) {pFeatures[nFeatureIdxOffSet+k] = 0;}
		}
	}
	nFeatureIdxOffSet += 5;
	
	decumaAssert(nFeatureIdxOffSet == COARSE_NBR_FEATURES);

	return nFeatureIdxOffSet;
}

/*
	
 */

void dltCoarseSetOldDecumaFeatures(CJK_COMPRESSED_CHAR * pCChar, CJK_SESSION * pSession) 
{
	/*/* Fill in hsplit * / */
	/*{ */
	/*	DECUMA_INT32 * split = pScratchpad->split; */
	/*	DECUMA_INT32 * ink = pScratchpad->ink; */

	/*	DECUMA_UINT8 x, xprev; */
	/*	DECUMA_UINT8 *p = cchardata + 1; */
	/*	DECUMA_INT32 n, i, j, k; */

	/*	for (i = 0; i < 16; i++) { */
	/*		split[i] = -1; */
	/*	} */

	/*	n = 1; */
	/*	xprev = 0; */
	/*	while (*p != 0) { */
	/*		k = *p++; */
	/*		for (i = 0; i < k; i++) { */
	/*			x = CJK_GP_GET_X(*p); */
	/*			if (x > xprev) { */
	/*				for (j = xprev + 1; j < x; j++) { */
	/*					if (split[j] >= -1) { */
	/*						split[j] = n - 1; */
	/*						ink[j] = (i != 0); */
	/*					} */
	/*				} */
	/*				if (split[x] >= -1) { */
	/*					split[x] = n; */
	/*					ink[x] = (i != k - 1); */
	/*				} */
	/*			} */
	/*			else if (x < xprev) { */
	/*				if (split[xprev] >= -1) { */
	/*					split[xprev] = n; */
	/*				} */
	/*				for (j = xprev - 1; j > x; j--) { */
	/*					split[j] = -2; */
	/*				} */
	/*			} */
	/*			else { */
	/*				if (split[x] >= 0 && i == k - 1) { */
	/*					split[x] = n; */
	/*					ink[x] = 0; */
	/*				} */
	/*			} */
	/*			xprev = x; */
	/*			p++; */
	/*			n++; */
	/*		} */
	/*	} */
	/*	n--; */
	/*	a->split = 0; */
	/*	for (i = 1; i < 15; i++) { */
	/*		if (split[i] >= 4 && split[i] <= n - 4) { */
	/*			if (a->split == 0 || ink[i] == 0) { */
	/*				a->split = (DECUMA_UINT8) split[i]; */
	/*				if (ink[i] == 0) break; */
	/*			} */
	/*		} */
	/*	} */
	/*} */

	/** Fill in dotstart 
	 * If the user has written a short dot as a start stroke this field is true.
	 * We also fill in the [[isnot_]] component fields.
	 */ 
	CJK_STROKE s1, s2, s3;

	DECUMA_INT32 y1, x1, y2, x2, y3, x3;
	const DECUMA_INT32 x_limit_leftpart = 6;/* The first xcoordinate in the right part */

	dltCCharGetFirstStroke(pCChar, &s1, pSession);
	dltCCompressSetDotStart(pCChar, 0);
	if (CJK_STROKE_NPOINTS(&s1) <= 4 &&
		CJK_GP_GET_SQ_DISTANCE(*cjkStrokeGetGridpoint(&s1, 1),  *cjkStrokeGetGridpoint(&s1, -1)) < 20) {
			dltCCompressSetDotStart(pCChar, 1);
	}
	dltCCompressSetNotSpeaking(pCChar, 0);
	dltCCompressSetNotThreeDrops(pCChar, 0);
	dltCCompressSetNotSoil(pCChar, 0);

	if(dltCCCompressGetNbrStrokes(pCChar) >= 3) {
		s2 = s1;
		cjkStrokeNext(&s2, pSession);
		decumaAssert(CJK_STROKE_EXISTS(&s2));
		s3 = s2;
		cjkStrokeNext(&s3, pSession);
		decumaAssert(CJK_STROKE_EXISTS(&s3));
		x1 = CJK_STROKE_GET_X(&s1, 1);
		y1 = CJK_STROKE_GET_Y(&s1, 1);
		x2 = CJK_STROKE_GET_X(&s2, 1);
		y2 = CJK_STROKE_GET_Y(&s2, 1);
		x3 = CJK_STROKE_GET_X(&s3, 1);
		y3 = CJK_STROKE_GET_Y(&s3, 1);
		if (!(y1 <= y2 && y2 <= y3) || CJK_GP_GET_X(cjkStrokeGetMinXGridpoint(&s3)) >= x_limit_leftpart) {
			dltCCompressSetNotThreeDrops(pCChar, 1);
		}
		if (y3 > CJK_STROKE_GET_Y(&s2, -1)) {
			dltCCompressSetNotSpeaking(pCChar, 1);
		}
		if (x1 < x_limit_leftpart && x2 < x_limit_leftpart && x3 < x_limit_leftpart) {
			/*Three strokes in left component */
			if (CJK_GP_GET_Y(cjkStrokeGetMaxYGridpoint(&s2)) > CJK_GP_GET_Y(cjkStrokeGetMaxYGridpoint(&s3)) ) {
				/*block form of the component! */
				dltCCompressSetNotSoil(pCChar, 1);
			}
		}
		else if (x1 < x_limit_leftpart && x2 < x_limit_leftpart && x3 >= x_limit_leftpart) {
			/*Two strokes in left component */
			if (ABS(CJK_STROKE_GET_Y(&s1, 1) - CJK_STROKE_GET_Y(&s1, -1)) <= 3) {
				/*First stroke is horizontal. */
				if (cjkStrokeGetIntersectionCount(&s2, pSession) == 1) {
					dltCCompressSetNotSoil(pCChar, 1);
				}
			}
		}
		else if  (x1 < x_limit_leftpart && x2 >= x_limit_leftpart && x3 >= x_limit_leftpart) {
			/*One stroke in left component */
			if (cjkStrokeGetIntersectionCount(&s1, pSession) == 2) {
				dltCCompressSetNotSoil(pCChar, 1);
			}
		}

	}
}


/**
 * This function has been imported from the original dltlib "swift" implementation, which was a hamming-distance based coarse distance layer.
 */

static void dltCoarseSetSwiftData(CJK_COMPRESSED_CHAR * pCChar, SWIFT_DIST * pSwiftDist, CJK_SESSION * pSession) 
{
	DECUMA_INT32 nPoints = 0;
	DECUMA_INT32 i;
	CJK_STROKE s;
	const CJK_GRIDPOINT * p;
	DECUMA_INT32 t = 0;
	DECUMA_INT32 n;
	DECUMA_INT32 alpha;
	DECUMA_INT32 beta;
	DECUMA_INT32 r;
	DECUMA_INT32 zx  = 0;
	DECUMA_INT32 zy  = 0;
	DECUMA_INT32 zt  = 0;
	DECUMA_INT32 zxx = 0;
	DECUMA_INT32 zyy = 0;
	DECUMA_INT32 zty = 0;
	DECUMA_INT32 ztt = 0;
	DECUMA_INT32 e2;
	DECUMA_INT32 mine2 = 0x7FFFFFFF;
	DECUMA_UINT8 mint = 0;
	CJK_COARSE_FEATURES_SCRATCHPAD * pScratchpad = (CJK_COARSE_FEATURES_SCRATCHPAD *) pSession->pScratchpad;
	DECUMA_INT32 * sx  = pScratchpad->sx;
	DECUMA_INT32 * sy  = pScratchpad->sy;
	DECUMA_INT32 * st  = pScratchpad->st;
	DECUMA_INT32 * sxx = pScratchpad->sxx;
	DECUMA_INT32 * syy = pScratchpad->syy;
	DECUMA_INT32 * sty = pScratchpad->sty;
	DECUMA_INT32 * stt = pScratchpad->stt;

	if (dltCCCompressGetNbrPoints(pCChar) < 10) {
		pSwiftDist->x1 = 0;
		pSwiftDist->x2 = 0;
		pSwiftDist->y1 = 0;
		pSwiftDist->y2 = 0;
		pSwiftDist->y3 = 0;
		pSwiftDist->y4 = 0;
		pSwiftDist->splitnum   = 0;
		pSwiftDist->splitpoint = 0;
		return;
	}

	for(dltCCharGetFirstStroke(pCChar, &s, pSession); CJK_STROKE_EXISTS(&s); cjkStrokeNext(&s, pSession)) {
		i = CJK_STROKE_NPOINTS(&s);
		nPoints += i;
		p = CJK_STROKE_FIRST_POINT(&s);

		for (; i; i--) {
			DECUMA_INT32 x = CJK_GP_GET_X(*p);
			DECUMA_INT32 y = CJK_GP_GET_Y(*p);

			p++;

			decumaAssert(t < MAXPNTSPERCHAR);

			sx[t] = zx += x;
			sy[t] = zy += y;
			st[t] = zt += t;

			sxx[t] = zxx += x * x;
			syy[t] = zyy += y * y;
			sty[t] = zty += t * y;
			stt[t] = ztt += t * t;
			t++;
		}
	}
	n = t;

	/*----------------------------------------------------- */
	/* find swiftdist splitpoint: */
	/* skip two points in each end. First part < t, second part >= t */

	for (t = 3; t <= n - 3; t++) {
		e2 = sxx[t-1] - (sx[t-1] * sx[t-1]) / t;

		zxx = sxx[n-1] - sxx[t-1];
		zx  = sx[n-1] - sx[t-1];

		e2 += zxx - (zx * zx) / (n - t);

		zt = st[t-1];
		zy = sy[t-1];

		zyy = syy[t-1] - (zy * zy) / t;
		zty = sty[t-1] - (zt * zy) / t;
		ztt = stt[t-1] - (zt * zt) / t;

		e2 += zyy - (zty * zty) / ztt;

		zt = st[n-1] - st[t-1];
		zy = sy[n-1] - sy[t-1];

		zyy = (syy[n-1] - syy[t-1]) - (zy * zy) / (n - t);
		zty = (sty[n-1] - sty[t-1]) - (zt * zy) / (n - t);
		ztt = (stt[n-1] - stt[t-1]) - (zt * zt) / (n - t);

		e2 += zyy - (zty * zty) / ztt;

		if (e2 < mine2) {
			mint = (DECUMA_UINT8) t;
			mine2 = e2;
		}
	}


	/*----------------------------------------------------- */
	/* compute swiftdist info */
	t = mint;
	pSwiftDist->splitnum   = mint;
	pSwiftDist->splitpoint = (DECUMA_UINT8) ((256 * t) / n);

	pSwiftDist->x1 = (DECUMA_UINT8) (((256/15) * sx[t-1]) / t);
	pSwiftDist->x2 = (DECUMA_UINT8) (((256/15) * (sx[n-1] - sx[t-1])) / (n-t));

	alpha = ((256/15) * sy[t-1]) / t;

	zt = st[t-1];
	zy = sy[t-1];

	zyy = syy[t-1] - (zy * zy) / t;
	zty = sty[t-1] - (zt * zy) / t;
	ztt = stt[t-1] - (zt * zt) / t;

	beta = (256 * zty) / ztt;

	r = LIMIT_TO_RANGE(alpha - (beta * t) / (2 * 15), 0, 255);
	pSwiftDist->y1 = (DECUMA_UINT8) r;

	r = LIMIT_TO_RANGE(alpha + (beta * t) / (2 * 15), 0, 255);
	pSwiftDist->y2 = (DECUMA_UINT8) r;

	alpha = ((256 / 15) * (sy[n - 1] - sy[t - 1])) / (n - t);

	zt = st[n - 1] - st[t - 1];
	zy = sy[n - 1] - sy[t - 1];

	zyy = (syy[n - 1] - syy[t - 1]) - (zy * zy) / (n - t);
	zty = (sty[n - 1] - sty[t - 1]) - (zt * zy) / (n - t);
	ztt = (stt[n - 1] - stt[t - 1]) - (zt * zt) / (n - t);

	beta = (256 * zty) / ztt;
	
	r = LIMIT_TO_RANGE(alpha - (beta * (n - t)) / (2 * 15), 0, 255);
	pSwiftDist->y3 = (DECUMA_UINT8) r;
	
	r = LIMIT_TO_RANGE(alpha + (beta * (n - t)) / (2 * 15), 0, 255);
	pSwiftDist->y4 = (DECUMA_UINT8) r;
}




/*********************************************
 * A number of internal functions for matching a certaing radical (without a database)
*/

#if 0
/*
   TODO: Box currently defined to fill up left space, i.e. only max_x is required
*/
static CJK_DISTANCE dltCoarsePenalizeRadicalBox(CJK_COMPRESSED_CHAR * pChar, int nMatchIdx, CJK_SESSION * pSession)
{
	CJK_DISTANCE punish = 0;
	DECUMA_UINT8 nMaxX = 0, nMaxY = 0;
	CJK_GRIDPOINT lastgp;
	int j;
	CJK_GRIDPOINT_ITERATOR gpI;
	
	/* Find left properties			 */
	for (dltGPIterInit(&gpI, pChar, pSession), j=0; j <= nMatchIdx && CJK_GPITER_HAS_GRIDPOINT(&gpI); j++) {
		lastgp = CJK_GPITER_GET_GRIDPOINT(&gpI);
		nMaxX = DECUMA_MAX(CJK_GP_GET_X(lastgp), nMaxX);
		nMaxY = DECUMA_MAX(CJK_GP_GET_Y(lastgp), nMaxY);
		CJK_GPITER_NEXT(&gpI, pSession);
	}
	/* Penalize points in box until out of box */
	while(CJK_GP_GET_X(CJK_GPITER_GET_GRIDPOINT(&gpI)) <= nMaxX && 
		  CJK_GP_GET_Y(CJK_GPITER_GET_GRIDPOINT(&gpI)) >= nMaxY-4 &&
			j < dltCCCompressGetNbrPoints(pChar)) {
		punish += 6;
		CJK_GPITER_NEXT(&gpI, pSession);
		j++;
	}
	return punish;
}
#endif

#define CJK_COARSE_N_PERSON_RADICALS 12
static void dltCoarseGetDistRadicalPerson(CJK_COMPRESSED_CHAR * pChar, CJK_DISTANCE * dist, CJK_DISTANCE * angDist, CJK_SESSION * pSession)
{
	/* ARM requires the below to be static */
	static const CJK_COMPRESSED_CHAR_DATA pCcDataPersonRadical1[8] = {7, 2, CJK_GP(5, 1), CJK_GP(1, 6), 2, 
		CJK_GP(3,4), CJK_GP(3, 14), 0}; /* person-l */
	static const CJK_COMPRESSED_CHAR_DATA pCcDataPersonRadical2[8] = {7, 2, CJK_GP(6, 2), CJK_GP(2, 6), 2, 
		CJK_GP(4,4), CJK_GP(4, 13), 0}; /* person-l2 */
	static const CJK_COMPRESSED_CHAR_DATA pCcDataPersonRadical3[8] = {7, 2, CJK_GP(6, 2), CJK_GP(2, 9), 2, 
		CJK_GP(4,5), CJK_GP(4, 12), 0}; /* person-l2_2 */
	static const CJK_COMPRESSED_CHAR_DATA pCcDataPersonRadical4[8] = {7, 2, CJK_GP(5, 4), CJK_GP(3, 7), 2, 
		CJK_GP(4,6), CJK_GP(4, 14), 0}; /* person-l3 */
	static const CJK_COMPRESSED_CHAR_DATA pCcDataPersonRadical5[8] = {7, 2, CJK_GP(5, 4), CJK_GP(3, 10), 2, 
		CJK_GP(4,7), CJK_GP(4, 13), 0}; /* person-l3_2 */
	static const CJK_COMPRESSED_CHAR_DATA pCcDataPersonRadical6[8] = {7, 2, CJK_GP(4, 4), CJK_GP(1, 9), 2, 
		CJK_GP(3,5), CJK_GP(3, 14), 0}; /* person-l5 */
	static const CJK_COMPRESSED_CHAR_DATA pCcDataPersonRadical7[8] = {7, 2, CJK_GP(6, 1), CJK_GP(0, 9), 2, 
		CJK_GP(4,9), CJK_GP(4, 14), 0}; /* new-l1 */
	static const CJK_COMPRESSED_CHAR_DATA pCcDataPersonRadical8[8] = {7, 2, CJK_GP(6, 0), CJK_GP(2, 8), 2, 
		CJK_GP(4,7), CJK_GP(4, 14), 0}; /* new-l2 */
	static const CJK_COMPRESSED_CHAR_DATA pCcDataPersonRadical9[8] = {7, 2, CJK_GP(4, 4), CJK_GP(2, 6), 2, 
		CJK_GP(4,6), CJK_GP(4, 11), 0}; /* new-l3 */
	static const CJK_COMPRESSED_CHAR_DATA pCcDataPersonRadical10[8] = {7, 2, CJK_GP(6, 4), CJK_GP(0, 6), 2, 
		CJK_GP(5,7), CJK_GP(5, 13), 0}; /* new-l4 */
	static const CJK_COMPRESSED_CHAR_DATA pCcDataPersonRadical11[8] = {7, 2, CJK_GP(4, 5), CJK_GP(1, 9), 2, 
		CJK_GP(4,8), CJK_GP(4, 11), 0}; /* new-l5 */
	static const CJK_COMPRESSED_CHAR_DATA pCcDataPersonRadical12[8] = {7, 2, CJK_GP(4, 2), CJK_GP(0, 10), 2, 
		CJK_GP(3,7), CJK_GP(3, 12), 0}; /* new-l6 */
	CJK_COARSE_FEATURES_SCRATCHPAD * pScratchpad = (CJK_COARSE_FEATURES_SCRATCHPAD *) pSession->pScratchpad;
	CJK_COMPRESSED_CHAR * pCCPersonRadical = pScratchpad->pCCFirstStrokeDb;
	const CJK_COMPRESSED_CHAR_DATA * const pPersonalRadicals[CJK_COARSE_N_PERSON_RADICALS] = {&pCcDataPersonRadical1[0], &pCcDataPersonRadical2[0],
		&pCcDataPersonRadical3[0], &pCcDataPersonRadical4[0], &pCcDataPersonRadical5[0],
		&pCcDataPersonRadical6[0], &pCcDataPersonRadical7[0], &pCcDataPersonRadical8[0],
		&pCcDataPersonRadical9[0], &pCcDataPersonRadical10[0], &pCcDataPersonRadical11[0], &pCcDataPersonRadical12[0]};
	DECUMA_UINT32 nIdx = 0, nMatchIdx = 0;
	CJK_DISTANCE tdist;
	int j;

	*dist = LARGEDIST;
	*angDist = LARGEDIST;

	for (j=0; j < CJK_COARSE_N_PERSON_RADICALS; j++) {
		dltCCCompressSetCCData(pCCPersonRadical, pPersonalRadicals[j]);
		dltCCCompressSetNbrPoints(pCCPersonRadical, 4); 
		dltCCCompressSetNbrStrokes(pCCPersonRadical, 2);

		/* Matching person radical */
		tdist = dltCCharGetDTWDistanceComponent(pChar, pCCPersonRadical, &nIdx, 0, pSession);
		if (tdist < *dist) {
			nMatchIdx = nIdx;
			*dist = tdist;
		}

		/* Now match with direction instead */
		tdist = dltCCharGetDTWDistanceComponent(pChar, pCCPersonRadical, &nIdx, 1, pSession);
		if (tdist < *angDist) *angDist = tdist;
	}

	/* Convert to good range (0, DECUMA_FEATURE_MAX) - cut off at really bad match */
	*dist = (*dist * DECUMA_FEATURE_MAX) / (dltCCCompressGetNbrPoints(pCCPersonRadical) * BAD_DTW_POINT_DIST);
	*angDist = (*angDist * DECUMA_FEATURE_MAX) / ((dltCCCompressGetNbrPoints(pCCPersonRadical)-1) * BAD_DTW_POINT_DIST);
}

#define CJK_COARSE_N_THREAD_S_RADICALS 6
static void dltCoarseGetDistRadicalThread(CJK_COMPRESSED_CHAR * pChar, CJK_DISTANCE * dist, CJK_SESSION * pSession)
{
	static const CJK_COMPRESSED_CHAR_DATA pCcDataRadical1[13] = {12, 3, CJK_GP(5, 1), CJK_GP(2, 5), CJK_GP(5, 5), 3, 
		CJK_GP(6,4), CJK_GP(2, 9), CJK_GP(5, 8), 2, CJK_GP(1, 14), CJK_GP(6, 10), 0}; /* threadS-l */
	static const CJK_COMPRESSED_CHAR_DATA pCcDataRadical2[13] = {12, 3, CJK_GP(4, 1), CJK_GP(1, 6), CJK_GP(4, 5), 3, 
		CJK_GP(4,5), CJK_GP(1, 10), CJK_GP(4, 9), 2, CJK_GP(1, 14), CJK_GP(4, 12), 0}; /* threadS-l2 */
	static const CJK_COMPRESSED_CHAR_DATA pCcDataRadical3[13] = {12, 3, CJK_GP(6, 3), CJK_GP(4, 6), CJK_GP(6, 5), 3, 
		CJK_GP(6,5), CJK_GP(4, 7), CJK_GP(5, 7), 2, CJK_GP(5, 9), CJK_GP(6, 5), 0}; /* new - l1 */
	static const CJK_COMPRESSED_CHAR_DATA pCcDataRadical4[13] = {12, 3, CJK_GP(3, 2), CJK_GP(0, 5), CJK_GP(2, 5), 3, 
		CJK_GP(2,5), CJK_GP(1, 8), CJK_GP(3, 8), 2, CJK_GP(1, 9), CJK_GP(3, 9), 0}; /* new - l2 */
	static const CJK_COMPRESSED_CHAR_DATA pCcDataRadical5[13] = {12, 3, CJK_GP(3, 3), CJK_GP(0, 6), CJK_GP(3, 6), 3, 
		CJK_GP(3,6), CJK_GP(1, 9), CJK_GP(3, 8), 2, CJK_GP(2, 13), CJK_GP(4, 11), 0}; /* new - l3 */
	static const CJK_COMPRESSED_CHAR_DATA pCcDataRadical6[13] = {12, 3, CJK_GP(6, 0), CJK_GP(2, 8), CJK_GP(4, 8), 3, 
		CJK_GP(4,8), CJK_GP(1, 12), CJK_GP(4, 9), 2, CJK_GP(2, 10), CJK_GP(4, 10), 0}; /* new - l4 */
	CJK_COARSE_FEATURES_SCRATCHPAD * pScratchpad = (CJK_COARSE_FEATURES_SCRATCHPAD *) pSession->pScratchpad;
	CJK_COMPRESSED_CHAR * pCCRadical = pScratchpad->pCCFirstStrokeDb;
	const CJK_COMPRESSED_CHAR_DATA * const pRadicals[CJK_COARSE_N_THREAD_S_RADICALS] = {&pCcDataRadical1[0], &pCcDataRadical2[0],
		&pCcDataRadical3[0],&pCcDataRadical4[0],&pCcDataRadical5[0],&pCcDataRadical6[0],};
	DECUMA_UINT32 nMatchIdx;
	CJK_DISTANCE tdist;
	int j;

	*dist = LARGEDIST;

	for (j=0; j < CJK_COARSE_N_THREAD_S_RADICALS; j++) {
		dltCCCompressSetCCData(pCCRadical, pRadicals[j]);
		dltCCCompressSetNbrPoints(pCCRadical, 8); 
		dltCCCompressSetNbrStrokes(pCCRadical, 3);

		/* Matching person radical */
		tdist = dltCCharGetDTWDistanceComponent(pChar, pCCRadical, &nMatchIdx, 0, pSession);
		if (tdist < *dist) *dist = tdist;
	}
	/* Convert to good range (0, DECUMA_FEATURE_MAX) - cut off at really bad match */
	*dist = (*dist * DECUMA_FEATURE_MAX) / (dltCCCompressGetNbrPoints(pCCRadical) * BAD_DTW_POINT_DIST);
}

#define CJK_COARSE_N_HAND_RADICALS 11
static void dltCoarseGetDistRadicalHand(CJK_COMPRESSED_CHAR * pChar, CJK_DISTANCE * dist, CJK_SESSION * pSession)
{
	/* Hand */
	static const CJK_COMPRESSED_CHAR_DATA pCcDataRadical1[12] = {11, 2, CJK_GP(2, 5), CJK_GP(6, 5), 3, 
		CJK_GP(4,2), CJK_GP(4, 14), CJK_GP(3, 13), 2, CJK_GP(2, 10), CJK_GP(6, 8), 0}; /* hand-l1 */
	static const CJK_COMPRESSED_CHAR_DATA pCcDataRadical2[12] = {11, 2, CJK_GP(1, 4), CJK_GP(5, 4), 3, 
		CJK_GP(3,1), CJK_GP(3, 14), CJK_GP(2, 13), 2, CJK_GP(1, 9), CJK_GP(5, 7), 0}; /* hand-l2 */
	static const CJK_COMPRESSED_CHAR_DATA pCcDataRadical3[12] = {11, 2, CJK_GP(2, 0), CJK_GP(4, 5), 3, 
		CJK_GP(2,2), CJK_GP(2, 14), CJK_GP(1, 13), 2, CJK_GP(0, 9), CJK_GP(3, 7), 0}; /* hand-l3 */
	static const CJK_COMPRESSED_CHAR_DATA pCcDataRadical4[12] = {11, 2, CJK_GP(1, 6), CJK_GP(5, 6), 3, 
		CJK_GP(3,3), CJK_GP(3, 15), CJK_GP(1, 14), 2, CJK_GP(1, 12), CJK_GP(4, 10), 0}; /* hand-l4 */
	static const CJK_COMPRESSED_CHAR_DATA pCcDataRadical5[12] = {11, 2, CJK_GP(1, 6), CJK_GP(5, 6), 3, 
		CJK_GP(3,2), CJK_GP(3, 14), CJK_GP(2, 13), 2, CJK_GP(1, 12), CJK_GP(5, 10), 0}; /* hand-l5 */
	static const CJK_COMPRESSED_CHAR_DATA pCcDataRadical6[12] = {11, 2, CJK_GP(1, 6), CJK_GP(5, 6), 2, 
		CJK_GP(1, 12), CJK_GP(5, 10), 3, CJK_GP(3,2), CJK_GP(3, 14), CJK_GP(2, 13), 0}; /* new (l5) - strange order */
	static const CJK_COMPRESSED_CHAR_DATA pCcDataRadical7[12] = {11, 2, CJK_GP(2, 6), CJK_GP(5, 5), 2, 
		CJK_GP(2, 8), CJK_GP(5, 7), 3, CJK_GP(3,1), CJK_GP(3, 14), CJK_GP(2, 13), 0}; /* new  - 2: strange order */
	static const CJK_COMPRESSED_CHAR_DATA pCcDataRadical8[12] = {11, 2, CJK_GP(2, 5), CJK_GP(7, 3), 3, 
		CJK_GP(5,1), CJK_GP(5, 15), CJK_GP(4, 12), 2, CJK_GP(1, 10), CJK_GP(5, 7), 0}; /* new-l1 */
	static const CJK_COMPRESSED_CHAR_DATA pCcDataRadical9[12] = {11, 2, CJK_GP(2, 6), CJK_GP(7, 5), 3, 
		CJK_GP(6,3), CJK_GP(6, 11), CJK_GP(3, 9), 2, CJK_GP(3, 8), CJK_GP(6, 7), 0}; /* new-l2 */
	static const CJK_COMPRESSED_CHAR_DATA pCcDataRadical10[12] = {11, 2, CJK_GP(4, 6), CJK_GP(9, 5), 3, 
		CJK_GP(6,2), CJK_GP(6, 13), CJK_GP(3, 12), 2, CJK_GP(2, 10), CJK_GP(8, 7), 0}; /* new-l3 */
	static const CJK_COMPRESSED_CHAR_DATA pCcDataRadical11[12] = {11, 2, CJK_GP(4, 5), CJK_GP(6, 5), 2, 
		CJK_GP(3, 8), CJK_GP(7, 7), 3, CJK_GP(5,3), CJK_GP(5, 15), CJK_GP(3, 13), 0}; /* new - strange order 2 */

	CJK_COARSE_FEATURES_SCRATCHPAD * pScratchpad = (CJK_COARSE_FEATURES_SCRATCHPAD *) pSession->pScratchpad;
	CJK_COMPRESSED_CHAR * pCCRadical = pScratchpad->pCCFirstStrokeDb;
	const CJK_COMPRESSED_CHAR_DATA * const pRadicals[CJK_COARSE_N_HAND_RADICALS] = {&pCcDataRadical1[0], &pCcDataRadical2[0],
		&pCcDataRadical3[0], &pCcDataRadical4[0], &pCcDataRadical5[0], &pCcDataRadical6[0], &pCcDataRadical7[0], 
		&pCcDataRadical8[0], &pCcDataRadical9[0], &pCcDataRadical10[0], &pCcDataRadical11[0],};
	DECUMA_UINT32 nMatchIdx;
	CJK_DISTANCE tdist;
	int j;

	*dist = LARGEDIST;

	for (j=0; j < CJK_COARSE_N_HAND_RADICALS; j++) {
		dltCCCompressSetCCData(pCCRadical, pRadicals[j]);
		dltCCCompressSetNbrPoints(pCCRadical, 7); 
		dltCCCompressSetNbrStrokes(pCCRadical, 3);

		/* Matching person radical */
		tdist = dltCCharGetDTWDistanceComponent(pChar, pCCRadical, &nMatchIdx, 0, pSession);
		if (tdist < *dist) *dist = tdist;
	}
	/* Convert to good range (0, DECUMA_FEATURE_MAX) - cut off at really bad match */
	*dist = (*dist * DECUMA_FEATURE_MAX) / (dltCCCompressGetNbrPoints(pCCRadical) * BAD_DTW_POINT_DIST);
}

/* Threedrops */
#define CJK_COARSE_N_THREEDROPS_RADICALS 5
static void dltCoarseGetDistRadicalThreedrops(CJK_COMPRESSED_CHAR * pChar, CJK_DISTANCE * dist, CJK_SESSION * pSession)
{
	static const CJK_COMPRESSED_CHAR_DATA pCcDataRadical1[11] = {10, 2, CJK_GP(2, 3), CJK_GP(4, 4), 2, 
		CJK_GP(2,6), CJK_GP(4, 7), 2, CJK_GP(2, 13), CJK_GP(4, 10), 0}; /* threedrops-l1 */
	static const CJK_COMPRESSED_CHAR_DATA pCcDataRadical2[11] = {10, 2, CJK_GP(3, 3), CJK_GP(5, 4), 2, 
		CJK_GP(3,6), CJK_GP(5, 7), 2, CJK_GP(3, 12), CJK_GP(5, 9), 0}; /* threedrops-l2	 */
	static const CJK_COMPRESSED_CHAR_DATA pCcDataRadical3[11] = {10, 2, CJK_GP(3, 3), CJK_GP(5, 4), 2, 
		CJK_GP(3,6), CJK_GP(5, 7), 2, CJK_GP(3, 12), CJK_GP(6, 8), 0}; /* threedrops-l2_2 */
	static const CJK_COMPRESSED_CHAR_DATA pCcDataRadical4[11] = {10, 2, CJK_GP(1, 3), CJK_GP(3, 4), 2, 
		CJK_GP(1,6), CJK_GP(3, 7), 2, CJK_GP(1, 13), CJK_GP(3, 10), 0}; /* threedrops-l3 */
	static const CJK_COMPRESSED_CHAR_DATA pCcDataRadical5[11] = {10, 2, CJK_GP(2, 5), CJK_GP(4, 6), 2, 
		CJK_GP(2,7), CJK_GP(4, 7), 2, CJK_GP(2, 10), CJK_GP(4, 8), 0}; /* threedrops-l_2 */
	CJK_COARSE_FEATURES_SCRATCHPAD * pScratchpad = (CJK_COARSE_FEATURES_SCRATCHPAD *) pSession->pScratchpad;
	CJK_COMPRESSED_CHAR * pCCRadical = pScratchpad->pCCFirstStrokeDb;
	const CJK_COMPRESSED_CHAR_DATA * const pRadicals[CJK_COARSE_N_THREEDROPS_RADICALS] = {&pCcDataRadical1[0], &pCcDataRadical2[0],
		&pCcDataRadical3[0], &pCcDataRadical4[0],
		&pCcDataRadical5[0]};
	DECUMA_UINT32 nMatchIdx;
	CJK_DISTANCE tdist;
	int j;

	*dist = LARGEDIST;

	for (j=0; j < CJK_COARSE_N_THREEDROPS_RADICALS; j++) {
		dltCCCompressSetCCData(pCCRadical, pRadicals[j]);
		dltCCCompressSetNbrPoints(pCCRadical, 6); 
		dltCCCompressSetNbrStrokes(pCCRadical, 3);

		/* Matching person radical */
		tdist = dltCCharGetDTWDistanceComponent(pChar, pCCRadical, &nMatchIdx, 0, pSession);
		if (tdist < *dist) *dist = tdist;
	}
	/* Convert to good range (0, DECUMA_FEATURE_MAX) - cut off at really bad match */
	*dist = (*dist * DECUMA_FEATURE_MAX) / (dltCCCompressGetNbrPoints(pCCRadical) * BAD_DTW_POINT_DIST);
}
