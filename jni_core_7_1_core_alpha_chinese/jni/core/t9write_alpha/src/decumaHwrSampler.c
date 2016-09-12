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


#define DECUMA_HWR_SAMPLER_C

#include "decumaConfig.h"
#include "decumaHwrSampler.h"
#include "decumaHwrSamplerData.h"
#include "decumaRuntimeMalloc.h"
#include "decumaAssert.h"
#include "decumaMemory.h"


#define IDX_FROM_SID(_pSamData,_sid,_a) {for ((_a)=0; (_a)<(_pSamData)->nStrokes && (_pSamData)->ppStrokes[_a]->id!=(_sid); (_a)++){};}
/* */
/* Local function declarations */
/* */
static int addElems(DECUMA_POINT ** pElems, int nCurrentNbrOfElems, int nNbrOfElemsToAdd, const DECUMA_MEM_FUNCTIONS * pMemFunctions);

static void removeStroke(DECUMA_SAMPLER_DATA * pSamData, int idx,int bFreeReleasedStroke);


/* */
/* Global functions */
/* */
DECUMA_STATUS decumaSamplerCreate( DECUMA_SAMPLER_DATA ** ppSamData,
						 int nMaxStrokes,
						 /*int nMaxPointsPerStroke, */
						 int nPointGranularity,
						 const DECUMA_MEM_FUNCTIONS * pMemFunctions)
{
	DECUMA_SAMPLER_DATA * pSamData; /*Helper pointer */
	decumaAssert(ppSamData);
	decumaAssert(nMaxStrokes>0);
/*	decumaAssert(nMaxPointsPerStroke>=nPointGranularity); */
	decumaAssert(nPointGranularity>0);
	decumaAssert(pMemFunctions);

	*ppSamData = decumaModCalloc(MODID_HWRLIB,1,sizeof(DECUMA_SAMPLER_DATA));
	if (!*ppSamData) goto decumaSamplerCreate_failed;

	pSamData = *ppSamData;
	pSamData->pMemFunctions = pMemFunctions;
	pSamData->nMaxStrokes = nMaxStrokes;
/*	pSamData->nMaxPointsPerStroke = nMaxPointsPerStroke; */
	pSamData->nPointGranularity = nPointGranularity;

	/*Always keep one stroke with nPointGranularity points allocated, for fast access */
	pSamData->ppStrokes=decumaModCalloc(MODID_HWRLIB,nMaxStrokes,sizeof(pSamData->ppStrokes[0]));
	if (!pSamData->ppStrokes) goto decumaSamplerCreate_failed;

	pSamData->samplerClock=1; /*Do not start at 0, since this is used as a special value */
	/*pSamData->idcnt=1; //Do not start at 0, since this is used as a special value */

	return decumaNoError;

decumaSamplerCreate_failed:
	
	decumaSamplerDestroy(ppSamData);
	return decumaAllocationFailed;
}


void decumaSamplerDestroy( DECUMA_SAMPLER_DATA ** ppSamData)
{
	DECUMA_SAMPLER_DATA * pSamData; /*Helper pointer */
	const DECUMA_MEM_FUNCTIONS * pMemFunctions;

	decumaAssert(ppSamData);
	if (!*ppSamData) return;

	pSamData = *ppSamData;
	pMemFunctions=pSamData->pMemFunctions;

	/*Destroy all the alloced strokes */
	for (;pSamData->nStrokes>0; pSamData->nStrokes--)
	{
		DECUMA_SAMPLER_STROKE *pStroke =pSamData->ppStrokes[pSamData->nStrokes-1];
		decumaAssert(pStroke);
		decumaAssert(pStroke->pPointBuffer);
		decumaModFree(MODID_HWRLIB,pStroke->pPointBuffer);
		decumaModFree(MODID_HWRLIB,pStroke);
	}
	decumaModFree(MODID_HWRLIB,pSamData->ppStrokes);

	decumaModFree(MODID_HWRLIB,pSamData);
	*ppSamData = NULL;
}

/* If this function is callded before commitStroke of the previous stroke, we have  */
DECUMA_STATUS decumaSamplerStartNewStroke(DECUMA_SAMPLER_DATA * pSamData,
										  DECUMA_UINT32 arcID )
{
	DECUMA_SAMPLER_STROKE * pNewStroke; 
	const DECUMA_MEM_FUNCTIONS * pMemFunctions;
	int idx;
	decumaAssert(pSamData);

	IDX_FROM_SID(pSamData,arcID,idx);
	if (idx!=pSamData->nStrokes) return decumaInvalidArcID; /*It must not already exist in the sampler */

	pMemFunctions=pSamData->pMemFunctions;

	if (pSamData->nStrokes >= pSamData->nMaxStrokes)
	{
		return decumaTooManyConcurrentArcs;
	}

	/*Alloc one new stroke */
	pSamData->ppStrokes[pSamData->nStrokes]=decumaModCalloc(MODID_HWRLIB,1,sizeof(*pSamData->ppStrokes[0]));
	if (!pSamData->ppStrokes[pSamData->nStrokes]) goto decumaSamplerStartNewStroke_failed;

	pNewStroke = pSamData->ppStrokes[pSamData->nStrokes];

	pNewStroke->pPointBuffer=decumaModCalloc(MODID_HWRLIB,pSamData->nPointGranularity,sizeof(DECUMA_POINT));
	if (!pNewStroke->pPointBuffer) 
	{
		/*Free the stroke, we want both point buffer and stroke to be allocated or none to avoid leak easier */
		decumaModFree(MODID_HWRLIB,pNewStroke);
		pSamData->ppStrokes[pSamData->nStrokes] = NULL;
		goto decumaSamplerStartNewStroke_failed;
	}

	pNewStroke->nPointBufferSize = pSamData->nPointGranularity;
	pSamData->nStrokes++;

	pNewStroke->commitTime = 0;
	pNewStroke->startTime = pSamData->samplerClock++;
	pNewStroke->id = arcID;

	return decumaNoError;

decumaSamplerStartNewStroke_failed:
	/*The sampler should be in a valid state to not leak any memory */
	/*But no arc is started */
	return decumaAllocationFailed;

}

DECUMA_STATUS decumaSamplerAddPoints(DECUMA_SAMPLER_DATA * pSamData, DECUMA_POINT * pPoints,
	int nPoints, DECUMA_UINT32 sid, int * pnAdded)
{
	DECUMA_SAMPLER_STROKE * pStroke;
	int idx,nPtsToAdd;
	decumaAssert(pPoints);
	decumaAssert(nPoints>0);

	*pnAdded=0;
	if (nPoints == 1) 
	{
		DECUMA_STATUS status = decumaSamplerAddPoint(pSamData,
			pPoints[0].x, pPoints[0].y, sid);
		if (status != decumaNoError) return status;
		*pnAdded = 1;
		return decumaNoError;
	}

	if (!pSamData->nStrokes) return decumaInvalidArcID;

	IDX_FROM_SID(pSamData,sid,idx);

	if (idx==pSamData->nStrokes) return decumaInvalidArcID;

	pStroke=pSamData->ppStrokes[idx];
	nPtsToAdd = nPoints;
/*	nPtsToAdd= (pStroke->nPoints + nPoints > pSamData->nMaxPointsPerStroke ? pSamData->nMaxPointsPerStroke - pStroke->nPoints : nPoints); */
/*	if (nPtsToAdd==0) return 0; */

	if (pStroke->nPoints + nPtsToAdd > pStroke->nPointBufferSize)
	{
		int nToAlloc, result;
		for (nToAlloc=pSamData->nPointGranularity; nToAlloc < nPtsToAdd; nToAlloc+=pSamData->nPointGranularity);

		result = addElems(&pStroke->pPointBuffer, pStroke->nPointBufferSize, nToAlloc, pSamData->pMemFunctions);
		if (!result) return decumaAllocationFailed;

		pStroke->nPointBufferSize+=nToAlloc;
	}
	decumaAssert(pStroke->nPointBufferSize >= pStroke->nPoints+ nPtsToAdd);
	decumaMemcpy(&pStroke->pPointBuffer[pStroke->nPoints],pPoints,nPtsToAdd*sizeof(DECUMA_POINT));
	pStroke->nPoints+=nPtsToAdd;
	*pnAdded = nPtsToAdd;
	return decumaNoError;
}


DECUMA_STATUS decumaSamplerAddPoint(DECUMA_SAMPLER_DATA * pSamData,
									DECUMA_COORD x,DECUMA_COORD y,
									DECUMA_UINT32 sid)
{
	DECUMA_SAMPLER_STROKE * pStroke;
	int idx;

	if (!pSamData->nStrokes)
		return decumaInvalidArcID;

	
	IDX_FROM_SID(pSamData,sid,idx);

	if (idx==pSamData->nStrokes)
		return decumaInvalidArcID;

	pStroke=pSamData->ppStrokes[idx];

/* TODO Enable this for CJK as well. Right now, this filter affects hitrate 
 *      (mostly negatively, but in a few cases positively) and prevents some
 *      other updates from validating cleanly. At a first glance, the below
 *      should not affect hitrate since we "just" skip redundant points, but
 *      more in-depth analysis of the effects on the cjk engine is required.
 */
#ifndef CJK_ENGINE

	if (pStroke->nPoints > 0 && 
		x == pStroke->pPointBuffer[pStroke->nPoints-1].x &&
		y == pStroke->pPointBuffer[pStroke->nPoints-1].y)
		return decumaNoError; /* No need to add point identical to previous point */

	if (pStroke->nPoints > 1)
	{
		DECUMA_INT32 prev_dx, prev_dy, new_dx, new_dy;

		new_dx = x - pStroke->pPointBuffer[pStroke->nPoints-1].x;
		new_dy = y - pStroke->pPointBuffer[pStroke->nPoints-1].y;

		prev_dx = pStroke->pPointBuffer[pStroke->nPoints-1].x - pStroke->pPointBuffer[pStroke->nPoints-2].x;
		prev_dy = pStroke->pPointBuffer[pStroke->nPoints-1].y - pStroke->pPointBuffer[pStroke->nPoints-2].y;

		if (new_dx == 0 && prev_dx == 0 && new_dy * prev_dy > 0 || /* Same vertical line */
			new_dy == 0 && prev_dy == 0 && new_dx * prev_dx > 0 || /* Same horisontal line */
			new_dx * prev_dy == prev_dx * new_dy && /* Same angle */
			new_dx * prev_dx > 0 && new_dy * prev_dy > 0 /* Same quadrant */)
		{
			/* No need to keep previous point on same line as new point */
			pStroke->pPointBuffer[pStroke->nPoints-1].x = x;
			pStroke->pPointBuffer[pStroke->nPoints-1].y = y;

			return decumaNoError;
		}
	}

#endif

	/*Normal case, faster handling */
/*	if (pStroke->nPoints == pSamData->nMaxPointsPerStroke) return 0; */
	if (pStroke->nPoints == pStroke->nPointBufferSize)
	{
		int result = addElems(&pStroke->pPointBuffer, pStroke->nPointBufferSize, pSamData->nPointGranularity, pSamData->pMemFunctions);
		if (!result) return decumaAllocationFailed;

		pStroke->nPointBufferSize+=pSamData->nPointGranularity;
	}
	pStroke->pPointBuffer[pStroke->nPoints].x=x;
	pStroke->pPointBuffer[pStroke->nPoints].y=y;
	pStroke->nPoints++;
	return decumaNoError;
}

/**
	This function retrieves a stroke with a certain arcID from the stroke sampler.
 */

DECUMA_STATUS decumaSamplerAccessStroke(DECUMA_SAMPLER_DATA * pSamData, DECUMA_UINT32 sid,
	DECUMA_SAMPLER_STROKE ** ppStroke)
{
	int idx;
	decumaAssert(ppStroke);

	*ppStroke = NULL;
	IDX_FROM_SID(pSamData,sid,idx);
	if (idx==pSamData->nStrokes) return decumaInvalidArcID; /*Not found */

	*ppStroke= pSamData->ppStrokes[idx];
	return decumaNoError;
}

/**
	This function simply retrieves stroke number idx from the ppStrokes array in the sampler.
 */

DECUMA_STATUS decumaSamplerAccessStrokeWithoutArcid(DECUMA_SAMPLER_DATA * pSamData, DECUMA_UINT32 idx,
	DECUMA_SAMPLER_STROKE ** ppStroke)
{
	decumaAssert(idx >= 0); /* This should be a safe assumption */
	decumaAssert(ppStroke);

	*ppStroke = NULL;
	if (idx >= pSamData->nStrokes) return decumaInvalidArcID; /*Not found */

	*ppStroke= pSamData->ppStrokes[idx];
	return decumaNoError;
}

DECUMA_STATUS decumaSamplerCommitStroke(DECUMA_SAMPLER_DATA * pSamData, DECUMA_UINT32 sid)

{
	int idx;

	IDX_FROM_SID(pSamData,sid,idx);
	if (idx==pSamData->nStrokes) return decumaInvalidArcID; /*Not found	 */

	pSamData->ppStrokes[idx]->commitTime = pSamData->samplerClock++;

	return decumaNoError;
}

DECUMA_STATUS decumaSamplerCommitAndRetrieveStroke(DECUMA_SAMPLER_DATA * pSamData, DECUMA_UINT32 sid,
	DECUMA_SAMPLER_STROKE ** ppStroke)

{
	int idx;
	decumaAssert(ppStroke);

	*ppStroke = NULL;
	IDX_FROM_SID(pSamData,sid,idx);
	if (idx==pSamData->nStrokes) return decumaInvalidArcID; /*Not found */

	*ppStroke= pSamData->ppStrokes[idx];

	(*ppStroke)->commitTime = pSamData->samplerClock++;

	removeStroke(pSamData, idx, 0); /*Remove but don't free */

	return decumaNoError;
}

/*Cancels sampling of one of stroke */
DECUMA_STATUS decumaSamplerCancelStroke(DECUMA_SAMPLER_DATA * pSamData, DECUMA_UINT32 sid)
{ 
	int idx;

	decumaAssert(pSamData);
	IDX_FROM_SID(pSamData,sid,idx);
	if (idx==pSamData->nStrokes) return decumaInvalidArcID; /*Not found */

	removeStroke(pSamData, idx,1); /*Remove and free */
	return decumaNoError;
}

int decumaSamplerGetStrokeCount(DECUMA_SAMPLER_DATA * pSamData)
{
	return pSamData->nStrokes;
}

DECUMA_STATUS decumaSamplerGetIdxFromStrokeID(DECUMA_SAMPLER_DATA * pSamData, DECUMA_UINT32 sid, int * pStrokeIdx)
{
	decumaAssert(pSamData);
	IDX_FROM_SID(pSamData,sid,*pStrokeIdx);
	if (*pStrokeIdx==pSamData->nStrokes) return decumaInvalidArcID; /*Not found */
	
	return decumaNoError;
}

DECUMA_STATUS decumaSamplerGetStrokeID(DECUMA_SAMPLER_DATA * pSamData,
									   int strokeIdx,DECUMA_UINT32 * pSamplingArcID)
{
	decumaAssert(pSamData);
	if (strokeIdx<0 || strokeIdx>=pSamData->nStrokes)
		return decumaInvalidIndex;

	*pSamplingArcID = pSamData->ppStrokes[strokeIdx]->id;
	return decumaNoError;
}

int decumaSamplerGetStrokeStartTime(DECUMA_SAMPLER_DATA * pSamData,
									   int strokeIdx)
{
	decumaAssert(pSamData);
	if (strokeIdx<0 || strokeIdx>=pSamData->nStrokes)
		return decumaInvalidIndex;

	return (int) pSamData->ppStrokes[strokeIdx]->startTime;
}

int decumaSamplerGetStrokeCommitTime(DECUMA_SAMPLER_DATA * pSamData,
									   int strokeIdx)
{
	decumaAssert(pSamData);
	if (strokeIdx<0 || strokeIdx>=pSamData->nStrokes)
		return decumaInvalidIndex;

	return (int) pSamData->ppStrokes[strokeIdx]->commitTime;
}

int decumaSamplerStrokeGetPointCount(const DECUMA_SAMPLER_STROKE * pStroke)
{
	decumaAssert(pStroke);
	return pStroke->nPoints;
}

/*Return the members of pArc with  */
const DECUMA_POINT * decumaSamplerStrokeAccessPoints(const DECUMA_SAMPLER_STROKE * pStroke)
{
	decumaAssert(pStroke);
	return pStroke->pPointBuffer;
}

void decumaSamplerStrokeDestroy(DECUMA_SAMPLER_DATA * pSamData, DECUMA_SAMPLER_STROKE ** ppStroke)
{
	const DECUMA_MEM_FUNCTIONS * pMemFunctions;
	decumaAssert(pSamData);
	if (!*ppStroke) return;

	pMemFunctions=pSamData->pMemFunctions;

	decumaAssert((*ppStroke)->pPointBuffer);
	decumaModFree(MODID_HWRLIB,(*ppStroke)->pPointBuffer);
	decumaModFree(MODID_HWRLIB,*ppStroke);
	*ppStroke = NULL;

}




/* LOCAL FUNCTIONS */

static void removeStroke(DECUMA_SAMPLER_DATA * pSamData, int idx, int bFreeReleasedStroke)
{
	DECUMA_SAMPLER_STROKE * pStroke;
	int i;
	const DECUMA_MEM_FUNCTIONS * pMemFunctions;

	decumaAssert(pSamData);
	decumaAssert(idx>=0);
	decumaAssert(idx<pSamData->nStrokes);

	pStroke= pSamData->ppStrokes[idx];
	pMemFunctions =pSamData->pMemFunctions;

	/*Remove one stroke and compress the buffer */
	for (i=idx; i<pSamData->nStrokes-1;i++)
	{
		pSamData->ppStrokes[i]=pSamData->ppStrokes[i+1];
	}
	pSamData->nStrokes--;

	if (bFreeReleasedStroke)
	{
		/*Free the point buffer of the relesed stroke */
		decumaAssert(pStroke->pPointBuffer);
		decumaModFree(MODID_HWRLIB,pStroke->pPointBuffer);

		/*Free the stroke */
		decumaModFree(MODID_HWRLIB,pStroke);
	}

	decumaAssert(pSamData->nStrokes >= 0);
}

static int addElems(DECUMA_POINT ** ppElems, int nCurrentNbrOfElems, int nNbrOfElemsToAdd, const DECUMA_MEM_FUNCTIONS * pMemFunctions)
{
	DECUMA_POINT * pNewElems;

	pNewElems = decumaModCalloc(MODID_HWRLIB,(nCurrentNbrOfElems + nNbrOfElemsToAdd),sizeof(DECUMA_POINT));

	if (pNewElems == NULL) return 0;

	if (*ppElems)
	{
		decumaMemcpy(pNewElems, *ppElems, nCurrentNbrOfElems * sizeof(DECUMA_POINT));
		decumaModFree(MODID_HWRLIB,*ppElems);
	}
	*ppElems = pNewElems;

	return 1;
}


