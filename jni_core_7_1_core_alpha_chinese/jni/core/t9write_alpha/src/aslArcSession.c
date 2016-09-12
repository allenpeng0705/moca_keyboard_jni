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

  This file implements aslArcSession.h. It uses dynamic memory
  allocation.
  
\******************************************************************/

#define ASL_ARC_SESSION_C

#include "aslConfig.h"

#include "aslArcSession.h"
#include "aslArcSessionMacros.h"

#include "decumaAssert.h"
#include "decumaMemory.h"
#include "aslArcSessionData.h"
#include "aslTools.h"
#include "aslSG.h" /*required for MAX_NODES_IN_SEGMENTATION_GRAPH */
#include "decumaHwrSampler.h"


#define SAMPLER_MAX_CONCURRENT_STROKES 10  
#define SAMPLER_POINT_GRANULARITY 200

/* */
/* Public function definitions */
/* */



int aslArcSessionGetSize(void)
{
	return sizeof(ASL_ARC_SESSION);
}

DECUMA_STATUS aslArcSessionInit(ASL_ARC_SESSION* pAslArcSession,
					   const DECUMA_MEM_FUNCTIONS* pMemFunctions,
                       int nBaseLine,
                       int nDistBaseToHelpLine,
                       WRITING_DIRECTION writingDirection)
{
	decumaAssert(pAslArcSession);
	decumaAssert(pMemFunctions);
	decumaAssert(!nDistBaseToHelpLine || nBaseLine >= MIN_DECUMA_COORD);
	decumaAssert(!nDistBaseToHelpLine || nBaseLine <= MAX_DECUMA_COORD);
	decumaAssert(nDistBaseToHelpLine <= MAX_DECUMA_COORD);
	decumaAssert(!nDistBaseToHelpLine || nBaseLine + nDistBaseToHelpLine <= MAX_DECUMA_COORD);

	decumaMemset(pAslArcSession, 0, aslArcSessionGetSize());

	pAslArcSession->pMemFunctions = pMemFunctions;
	pAslArcSession->nBaseLine = (DECUMA_COORD)nBaseLine;
	pAslArcSession->nDistBaseToHelpLine = (DECUMA_COORD)nDistBaseToHelpLine;
	pAslArcSession->nAslArcs = 0;
	pAslArcSession->ppAslArcs = NULL;

	pAslArcSession->writingDirection = writingDirection;
	
	decumaSamplerCreate(&pAslArcSession->pSampler,
		SAMPLER_MAX_CONCURRENT_STROKES,
		SAMPLER_POINT_GRANULARITY,
		pMemFunctions);
	if (!pAslArcSession->pSampler)
	{
		return decumaAllocationFailed;
	}
	return decumaNoError;
}

int aslArcSessionIsCorrupt(const ASL_ARC_SESSION* pAslArcSession)
{
	return pAslArcSession->bIsCorrupt;
}

void aslArcSessionDestroy(ASL_ARC_SESSION* pAslArcSession)
{
	const DECUMA_MEM_FUNCTIONS* pMemFunctions = pAslArcSession->pMemFunctions;
	int i;
	decumaAssert(pAslArcSession);

	for (i = 0; i < pAslArcSession->nAslArcs; i++)
	{
		decumaAssert(pAslArcSession->ppAslArcs && pAslArcSession->ppAslArcs[i]);
		aslArcDestroy(pAslArcSession->ppAslArcs[i]);
	}

	if (pAslArcSession->ppAslArcs) aslFree(pAslArcSession->ppAslArcs);
	if (pAslArcSession->curve.pArcs) aslFree(pAslArcSession->curve.pArcs);
	if (pAslArcSession->curve.pArcTimelineDiff) aslFree(pAslArcSession->curve.pArcTimelineDiff);
	pAslArcSession->nAslArcs = 0;

	if (pAslArcSession->pSampler) decumaSamplerDestroy(&pAslArcSession->pSampler);
}

#ifdef _DEBUG
int aslArcSessionGetBaseline(const ASL_ARC_SESSION* pAslArcSession)
{
	decumaAssert(pAslArcSession);

	return aslArcSessionGetBaselineMacro(pAslArcSession);
}

int aslArcSessionGetDistBaseToHelpline(const ASL_ARC_SESSION* pAslArcSession)
{
	decumaAssert(pAslArcSession);

	return aslArcSessionGetDistBaseToHelplineMacro(pAslArcSession);
}

#endif

void aslArcSessionSetBaseline(ASL_ARC_SESSION* pAslArcSession, int nBaseline)
{
	pAslArcSession->nBaseLine = (DECUMA_COORD) nBaseline;
}

void aslArcSessionSetDistBaseToHelpline(ASL_ARC_SESSION* pAslArcSession, int nDistBaseToHelpline)
{
	pAslArcSession->nDistBaseToHelpLine = (DECUMA_COORD) nDistBaseToHelpline;
}


DECUMA_STATUS aslArcSessionStartNewArc(ASL_ARC_SESSION* pAslArcSession, DECUMA_UINT32 arcID)
{
	 /*The number of nodes in the segmentation graph is equal to the number of arcs here */
	if(pAslArcSession->nAslArcs + decumaSamplerGetStrokeCount(pAslArcSession->pSampler)>= MAX_NODES_IN_SEGMENTATION_GRAPH )
		return decumaInputBufferFull;

	if (pAslArcSession->nAslArcs + decumaSamplerGetStrokeCount(pAslArcSession->pSampler) == 0) pAslArcSession->bStartNewSymbol = 1; /* First arc implicit symbol start */

	return decumaSamplerStartNewStroke(pAslArcSession->pSampler,arcID);
}

DECUMA_STATUS aslArcSessionStartNewSymbol(ASL_ARC_SESSION* pAslArcSession)
{
	if (decumaSamplerGetStrokeCount(pAslArcSession->pSampler) < 1) return decumaIllegalFunctionCall;

	pAslArcSession->bStartNewSymbol = 1;

	return decumaNoError;
}

/*Adds point(s) to a started arc  */
DECUMA_STATUS aslArcSessionAddPoint(ASL_ARC_SESSION* pAslArcSession, DECUMA_COORD x, DECUMA_COORD y, DECUMA_UINT32 arcID)
{
	return decumaSamplerAddPoint(pAslArcSession->pSampler,x,y,arcID);
}

DECUMA_STATUS aslArcSessionAddPoints(ASL_ARC_SESSION* pAslArcSession, 
							  DECUMA_POINT * pPts, int nPts, 
							  DECUMA_UINT32 arcID,
							  int * pnPtsAdded)
{
	return decumaSamplerAddPoints(pAslArcSession->pSampler,pPts,nPts,arcID,pnPtsAdded);
}


/*Ends the arc and removes it from the session */
DECUMA_STATUS aslArcSessionCancelArc(ASL_ARC_SESSION* pAslArcSession,DECUMA_UINT32 arcID)
{
	return decumaSamplerCancelStroke(pAslArcSession->pSampler,arcID);
}

/*Ends and adds the arc to the arc-addition-sequence which will be used in a future recognize call */
DECUMA_STATUS aslArcSessionCommitArc(ASL_ARC_SESSION* pAslArcSession,DECUMA_UINT32 arcID)
{
	int  bConcurrentWithNext;
	DECUMA_ARC arc;
	DECUMA_SAMPLER_STROKE * pSamStroke;

	DECUMA_STATUS status= decumaSamplerAccessStroke(pAslArcSession->pSampler,arcID,&pSamStroke);
	if (status != decumaNoError) return status;
	decumaAssert(pSamStroke);


	arc.nPoints = decumaSamplerStrokeGetPointCount(pSamStroke);
	/*Return error message for a arc with zero points */
	if (arc.nPoints==0)
	{
		status = decumaArcZeroPoints;
		goto aslArcSessionCommitArc_error;
	}

	arc.pPoints = (DECUMA_POINT*)decumaSamplerStrokeAccessPoints(pSamStroke);

	bConcurrentWithNext = decumaSamplerGetStrokeCount(pAslArcSession->pSampler)>1;
	
	/*Try to allocate and create the arc in the arc session before removing the stroke from the sampler */
	status = aslArcSessionAddArc(pAslArcSession, &arc,bConcurrentWithNext);
	if (status != decumaNoError) goto aslArcSessionCommitArc_error;

	/*Now it is safe to remove the stroke from the sampler. */
	status = decumaSamplerCommitAndRetrieveStroke(pAslArcSession->pSampler,arcID,
		&pSamStroke); /*We will get the pointer to the stroke here once again... But now, we also "own" it. */
	decumaAssert(status == decumaNoError); /*Should succeed since we managed to access the stroke before */
	decumaAssert(pSamStroke);

	/*Note that we have actually used the stroke before it was committed in the sampler. */
	/*E.g. if we care about the "commit-timestamp" in the sampled arc, we need to do this differently. */

	decumaSamplerStrokeDestroy(pAslArcSession->pSampler,&pSamStroke);
	decumaAssert(pSamStroke==0);

	if (!bConcurrentWithNext && pAslArcSession->bStartNewSymbol)
	{
		/* No more strokes open for arc addition and we have a new forced segmentation point. */
		/* Update the forced segmentation information by setting the forced segmentation after */
		/* the first stroke that is not concurrent with the stroke just committed. */
		int nSegPoint = pAslArcSession->nAslArcs-1;

		while (nSegPoint > 0 && pAslArcSession->curve.pArcTimelineDiff[nSegPoint-1] == 0) nSegPoint--;

		aslArcStartNewSymbol(pAslArcSession->ppAslArcs[nSegPoint]);

		pAslArcSession->bStartNewSymbol = 0;
	}

	return decumaNoError;

aslArcSessionCommitArc_error:

	return status;
}


int aslArcSessionGetUncommittedArcCount(ASL_ARC_SESSION* pAslArcSession)
{
	return decumaSamplerGetStrokeCount(pAslArcSession->pSampler);
}

DECUMA_STATUS aslArcSessionGetUncommittedArcID(ASL_ARC_SESSION* pAslArcSession, int idx, 
										DECUMA_UINT32 * pArcID)
{
	return decumaSamplerGetStrokeID(pAslArcSession->pSampler,idx,pArcID);
}

/*DECUMA_STATUS aslArcSessionGetArcIdxFromID(ASL_ARC_SESSION * pAslArcSession, 
										   DECUMA_UINT32 arcID)
{
}
*/




DECUMA_STATUS aslArcSessionAddArc(ASL_ARC_SESSION* pAslArcSession, const DECUMA_ARC* pDecumaArc,
								  int bConcurrentWithNext)
{
	DECUMA_STATUS status = decumaNoError;
	ASL_ARC* pAslArc;

	if (!aslAddElems(&pAslArcSession->ppAslArcs, pAslArcSession->nAslArcs, 1, sizeof(pAslArcSession->ppAslArcs[0]), pAslArcSession->pMemFunctions)) return decumaAllocationFailed;

	if (!aslAddElems(&pAslArcSession->curve.pArcs, pAslArcSession->nAslArcs, 1, sizeof(pAslArcSession->curve.pArcs[0]), pAslArcSession->pMemFunctions)) return decumaAllocationFailed;

	if (!aslAddElems(&pAslArcSession->curve.pArcTimelineDiff, pAslArcSession->nAslArcs, 1, sizeof(pAslArcSession->curve.pArcTimelineDiff[0]), pAslArcSession->pMemFunctions)) return decumaAllocationFailed;

	pAslArc = aslArcCreate(pAslArcSession, pDecumaArc, pAslArcSession->pMemFunctions);

	if (!pAslArc) return decumaAllocationFailed;

	

	pAslArcSession->ppAslArcs[pAslArcSession->nAslArcs++] = pAslArc;

	pAslArcSession->curve.pArcs[pAslArcSession->curve.nArcs++] = *aslArcGetDecumaArc(pAslArc);

	/*We will have this array as long as the number of arcs.  */
	/*However, the last element will not be read during recognition since it is an array of diffs */
	/*but we prepare it already now, since we know the diff already from bConcurrentWithNext */
	pAslArcSession->curve.pArcTimelineDiff[pAslArcSession->curve.nArcs-1]=!bConcurrentWithNext; 

	return status;
}

int aslArcSessionArcsAreConcurrent(const ASL_ARC_SESSION* pAslArcSession,
								   int nIndex1,int nIndex2)
{
	int i;
	decumaAssert(pAslArcSession);
	decumaAssert(nIndex1 >= 0);
	decumaAssert(nIndex2 > nIndex1);
	decumaAssert(nIndex2 <= pAslArcSession->nAslArcs); /* Allow to check concurrency with the next arc to be added too */
	decumaAssert(pAslArcSession->ppAslArcs);

	for (i=nIndex1; i<nIndex2; i++)
	{
		if (pAslArcSession->curve.pArcTimelineDiff[i] == 1)
			return 0; /*Not in a concurrent streak */
	}
	return 1;
}

int aslArcSessionIsStartingNewSymbol(const ASL_ARC_SESSION * pAslArcSession)
{
	decumaAssert(pAslArcSession);

	return pAslArcSession->bStartNewSymbol;
}

#ifdef _DEBUG
int aslArcSessionGetArcCount(const ASL_ARC_SESSION* pAslArcSession)
{
	decumaAssert(pAslArcSession);

	return aslArcSessionGetArcCountMacro(pAslArcSession);
}

const ASL_ARC* aslArcSessionGetArc(const ASL_ARC_SESSION* pAslArcSession, int nIndex)
{
	decumaAssert(pAslArcSession);
	decumaAssert(nIndex >= 0);
	decumaAssert(nIndex < pAslArcSession->nAslArcs);
	decumaAssert(pAslArcSession->ppAslArcs);

	return aslArcSessionGetArcMacro(pAslArcSession, nIndex);
}
#endif

const DECUMA_CURVE* aslArcSessionGetCurve(const ASL_ARC_SESSION* pAslArcSession)
{
	decumaAssert(pAslArcSession);

	return &pAslArcSession->curve;
}
