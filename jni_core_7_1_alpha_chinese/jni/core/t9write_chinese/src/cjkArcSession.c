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

/************************************************** Description ***\

  This file implements cjkArcSession.h. It uses dynamic memory
  allocation.
  
\******************************************************************/

#define CJK_ARC_SESSION_C
#define CJK_SYSTEM_CODE

#include "dltConfig.h"

#include "cjkArcSessionData.h"
#include "cjkArcSession.h"
#include "cjkCategories.h"

#include "decumaAssert.h"
#include "decumaMemory.h"

/* */
/* Public function definitions */
/* */

int cjkArcSessionGetSize(void)
{
	return sizeof(CJK_ARC_SESSION) + cjkCompressedCharacterGetSize();
}

DECUMA_STATUS cjkArcSessionInit(CJK_ARC_SESSION* pCjkArcSession, CJK_CC_COMPRESS_SCRATCHPAD * pScratchpad)
{
	decumaAssert(pCjkArcSession);
	decumaMemset(pCjkArcSession, 0, cjkArcSessionGetSize());

	pCjkArcSession->pCChar = (CJK_COMPRESSED_CHAR *)&(pCjkArcSession[1]);
	pCjkArcSession->pScratchpad = pScratchpad;

	cjkArcSessionResetCurve(pCjkArcSession);
	return decumaNoError;
}

void cjkArcSessionResetCurve(CJK_ARC_SESSION* pCjkArcSession)
{
	decumaAssert(pCjkArcSession);

	pCjkArcSession->nUsedPoints = 0;
	pCjkArcSession->curve.pArcs = &pCjkArcSession->arcBuffer[0];
	pCjkArcSession->curve.nArcs = 0;
	pCjkArcSession->nCommittedArcs = 0;

	dltCCCompressSetNull(pCjkArcSession->pCChar);

	return;
}

DECUMA_STATUS cjkArcSessionStartNewArc(CJK_ARC_SESSION* pCjkArcSession, DECUMA_UINT32 arcID)
{
	/* Require previous arcs to be committed before new arc can be added (MULTITOUCH NOT supported) */
	if (cjkArcSessionGetUncommittedArcCount(pCjkArcSession) > 0)
		return decumaTooManyConcurrentArcs;

	/*The number of nodes in the segmentation graph is equal to the number of arcs here */
	if( pCjkArcSession->nCommittedArcs >= CJK_ARC_SESSION_ARC_BUFFER_SIZE )
		return decumaInputBufferFull;

	/*Initiate new arc in the arcSession curve*/
	pCjkArcSession->curve.nArcs++;
	pCjkArcSession->curve.pArcs[pCjkArcSession->nCommittedArcs].nPoints = 0;
	pCjkArcSession->curve.pArcs[pCjkArcSession->nCommittedArcs].pPoints = &pCjkArcSession->pointBuffer[pCjkArcSession->nUsedPoints];
	pCjkArcSession->externalArcIDs[pCjkArcSession->nCommittedArcs] = arcID;

	return decumaNoError;
}

/*Adds point(s) to a started arc  */
DECUMA_STATUS cjkArcSessionAddPoint(CJK_ARC_SESSION* pCjkArcSession, DECUMA_COORD x, DECUMA_COORD y, DECUMA_UINT32 arcID)
{
	/* Verify that arcID corresponds to last started but not committed arc */
	if ( pCjkArcSession->curve.nArcs == 0 || arcID != pCjkArcSession->externalArcIDs[pCjkArcSession->curve.nArcs-1] ||
	     pCjkArcSession->curve.nArcs == pCjkArcSession->nCommittedArcs)
		return decumaInvalidArcID;

	/* Check if we need to add point and then add it if necessary */
	if (pCjkArcSession->nUsedPoints >= CJK_ARC_SESSION_POINT_BUFFER_SIZE)
		return decumaInputBufferFull;

	/* Currently this does not work with the KO database (which is "very" optimized and thus more sensitive to changes in compression method)*/
	if (pCjkArcSession->samplingRule != HangulSampling) {
		if (pCjkArcSession->curve.pArcs[pCjkArcSession->curve.nArcs-1].nPoints > 0 &&
			x == pCjkArcSession->pointBuffer[pCjkArcSession->nUsedPoints-1].x &&
			y == pCjkArcSession->pointBuffer[pCjkArcSession->nUsedPoints-1].y)
			return decumaNoError; /* No need to add point identical to previous point */

		if (pCjkArcSession->curve.pArcs[pCjkArcSession->curve.nArcs-1].nPoints > 1)
		{
			DECUMA_INT32 prev_dx, prev_dy, new_dx, new_dy;
	
			new_dx = x - pCjkArcSession->pointBuffer[pCjkArcSession->nUsedPoints-1].x;
			new_dy = y - pCjkArcSession->pointBuffer[pCjkArcSession->nUsedPoints-1].y;

			prev_dx = pCjkArcSession->pointBuffer[pCjkArcSession->nUsedPoints-1].x - pCjkArcSession->pointBuffer[pCjkArcSession->nUsedPoints-2].x;
			prev_dy = pCjkArcSession->pointBuffer[pCjkArcSession->nUsedPoints-1].y - pCjkArcSession->pointBuffer[pCjkArcSession->nUsedPoints-2].y;

			if (new_dx == 0 && prev_dx == 0 && new_dy * prev_dy > 0 || /* Same vertical line */
				new_dy == 0 && prev_dy == 0 && new_dx * prev_dx > 0 || /* Same horisontal line */
				new_dx * prev_dy == prev_dx * new_dy && /* Same angle */
				new_dx * prev_dx > 0 && new_dy * prev_dy > 0 /* Same quadrant */)
			{
				/* No need to keep previous point on same line as new point */
				pCjkArcSession->pointBuffer[pCjkArcSession->nUsedPoints-1].x = x;
				pCjkArcSession->pointBuffer[pCjkArcSession->nUsedPoints-1].y = y;

				return decumaNoError;
			}
		}
	}

	/* Actually add new point */
	pCjkArcSession->pointBuffer[pCjkArcSession->nUsedPoints].x = x;
	pCjkArcSession->pointBuffer[pCjkArcSession->nUsedPoints].y = y;
	pCjkArcSession->nUsedPoints++;
	pCjkArcSession->arcBuffer[pCjkArcSession->curve.nArcs-1].nPoints++;

	return decumaNoError;
}

DECUMA_STATUS cjkArcSessionAddPoints(CJK_ARC_SESSION* pCjkArcSession, 
							  DECUMA_POINT * pPts, int nPts, 
							  DECUMA_UINT32 arcID,
							  int * pnPtsAdded)
{
	int n;
	DECUMA_STATUS status;
	decumaAssert(pnPtsAdded);
	*pnPtsAdded=0;

	/* Verify that arcID corresponds to last started but not committed arc */
	if ( pCjkArcSession->curve.nArcs == 0 || arcID != pCjkArcSession->externalArcIDs[pCjkArcSession->curve.nArcs-1] ||
	     pCjkArcSession->curve.nArcs == pCjkArcSession->nCommittedArcs)
		return decumaInvalidArcID;

	for (n=0; n < nPts; n++) {
		status = cjkArcSessionAddPoint(pCjkArcSession, pPts[n].x, pPts[n].y, arcID);
		if (status != decumaNoError)
			return status;		
		(*pnPtsAdded)++;
		
	}
	return decumaNoError;
}


/*Ends the arc and removes it from the session */
DECUMA_STATUS cjkArcSessionCancelArc(CJK_ARC_SESSION* pCjkArcSession,DECUMA_UINT32 arcID)
{
	/* Verify that arcID corresponds to last started but not committed arc */
	if ( pCjkArcSession->curve.nArcs == 0 || arcID != pCjkArcSession->externalArcIDs[pCjkArcSession->curve.nArcs-1] ||
	     pCjkArcSession->curve.nArcs == pCjkArcSession->nCommittedArcs)
		return decumaInvalidArcID;

	/* Now remove the open arc */
	pCjkArcSession->nUsedPoints -= pCjkArcSession->curve.pArcs[pCjkArcSession->nCommittedArcs].nPoints;
	pCjkArcSession->curve.nArcs--;

	return decumaNoError;
}

/*OBS! This actually does nothing for cjkArcSession since the sampler is used directly */
/*Ends and adds the arc to the arc-addition-sequence which will be used in a future recognize call */
DECUMA_STATUS cjkArcSessionCommitArc(CJK_ARC_SESSION * pCjkArcSession, DECUMA_UINT32 arcID)
{
	DECUMA_STATUS status;

	/* Verify that arcID corresponds to last started but not committed arc */
	if ( pCjkArcSession->curve.nArcs == 0 || arcID != pCjkArcSession->externalArcIDs[pCjkArcSession->curve.nArcs-1] ||
	     pCjkArcSession->curve.nArcs == pCjkArcSession->nCommittedArcs)
		return decumaInvalidArcID;

	/*Return error message for a arc with zero points */
	if ( pCjkArcSession->curve.pArcs[pCjkArcSession->curve.nArcs-1].nPoints == 0)
	{
		return decumaArcZeroPoints;
	}
	pCjkArcSession->nCommittedArcs++;

	/* Arc compression is conducted in cjkCommitArc but we need to check for input */
	status = dltCCharCompressChar( pCjkArcSession->pCChar, (const DECUMA_CURVE *)&pCjkArcSession->curve, pCjkArcSession);
	if (status == decumaCurveZeroArcs ) {
		/* No arcs should return no error - just no recognition */
		return decumaNoError;
	}
	else if ( status != decumaNoError )
		return status; 

	return decumaNoError;
}

#ifdef CJK_ENABLE_INTERNAL_API
/* OBS! That this requires that sampling method has been set outside for Hangul to work (not being passed through filter) */
DECUMA_STATUS cjkArcSessionAddCurve(CJK_ARC_SESSION * pCjkArcSession, const DECUMA_CURVE * pCurve) {
	int n, nAdded;	
	DECUMA_STATUS status;

	cjkArcSessionResetCurve(pCjkArcSession);
	
	for (n=0; n < pCurve->nArcs; n++) {
		
		status = cjkArcSessionStartNewArc(pCjkArcSession, n);
		if (status != decumaNoError) return status;
		status = cjkArcSessionAddPoints(pCjkArcSession, pCurve->pArcs[n].pPoints, pCurve->pArcs[n].nPoints, n, &nAdded);
		decumaAssert(nAdded == pCurve->pArcs[n].nPoints);
		if (status != decumaNoError) return status;
		status = cjkArcSessionCommitArc(pCjkArcSession, n);
		if (status != decumaNoError) return status;
	}
	return decumaNoError;
}

#endif

int cjkArcSessionGetUncommittedArcCount(CJK_ARC_SESSION* pCjkArcSession)
{
	return (pCjkArcSession->curve.nArcs - pCjkArcSession->nCommittedArcs);
}

DECUMA_STATUS cjkArcSessionGetUncommittedArcID(CJK_ARC_SESSION* pCjkArcSession, int idx, 
										DECUMA_UINT32 * pArcID)
{
	if (idx <= 0 || idx > cjkArcSessionGetUncommittedArcCount(pCjkArcSession))
		return decumaInvalidIndex;

	/* Idx == 1 && there is one uncommitted */
	*pArcID = pCjkArcSession->externalArcIDs[pCjkArcSession->curve.nArcs-1];

	return decumaNoError;
}

int cjkArcSessionArcsAreConcurrent(const CJK_ARC_SESSION * pCjkArcSession,
								   int nIndex1,int nIndex2)
{
	/* Since CJK does not support concurrency they are never concurrent */
	return 0;
}

int cjkArcSessionGetArcCount(const CJK_ARC_SESSION* pCjkArcSession)
{
	decumaAssert(pCjkArcSession);

	return pCjkArcSession->curve.nArcs;
}

DECUMA_HWR_PRIVATE CJK_SAMPLING_RULE cjkArcSessionGetSamplingRule(CJK_ARC_SESSION * pCjkArcSession)
{
	decumaAssert(pCjkArcSession);

	return pCjkArcSession->samplingRule;
}

void cjkArcSessionSetSamplingRule(CJK_ARC_SESSION * pCjkArcSession, DECUMA_UINT32 categorymask, int nStrokes)
{
	decumaAssert(pCjkArcSession);

	if (nStrokes <= LATINLIMIT && categorymask & (CJK_GB2312 | CJK_BIGFIVE | CJK_BOPOMOFO)) {
		pCjkArcSession->samplingRule = ChineseNonHanSampling;
	}
	else if (nStrokes <= LATINLIMIT) {
		pCjkArcSession->samplingRule = NonHanSampling;
	}
	else if (categorymask & CJK_HANGUL) {
		pCjkArcSession->samplingRule = HangulSampling;
	}
	else {
		pCjkArcSession->samplingRule = HanSampling;
	}
	return;
}

CJK_COMPRESSED_CHAR * cjkArcSessionGetCompressedChar(CJK_ARC_SESSION * pCjkArcSession)
{
	decumaAssert(pCjkArcSession);

	return pCjkArcSession->pCChar;
}

CJK_COMPRESSED_CHAR_DATA * cjkArcSessionGetCCharData(CJK_ARC_SESSION * pCjkArcSession)
{
	decumaAssert(pCjkArcSession);

	return (CJK_COMPRESSED_CHAR_DATA *)pCjkArcSession->CCharData;
}

CJK_CC_COMPRESS_SCRATCHPAD * cjkArcSessionGetScratchpad(CJK_ARC_SESSION * pCjkArcSession)
{
	decumaAssert(pCjkArcSession);

	return pCjkArcSession->pScratchpad;
}
