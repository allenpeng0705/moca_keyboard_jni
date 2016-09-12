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


#ifndef DECUMA_HWR_SAMPLER_H
#define DECUMA_HWR_SAMPLER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "decuma_hwr_types.h"

typedef struct _DECUMA_SAMPLER_DATA DECUMA_SAMPLER_DATA;

typedef struct _DECUMA_SAMPLER_STROKE DECUMA_SAMPLER_STROKE;



/* Prototypes */

DECUMA_HWR_PRIVATE DECUMA_STATUS decumaSamplerCreate( DECUMA_SAMPLER_DATA ** ppSamData,
						 int nMaxStrokes,
						 /*int nMaxPointsPerStroke, */
						 int nPointGranularity,
						 const DECUMA_MEM_FUNCTIONS * pMemFunctions);

DECUMA_HWR_PRIVATE void decumaSamplerDestroy( DECUMA_SAMPLER_DATA ** ppSamData);

/* If this function is callded before commitStroke of the previous stroke, we have a concurrent stroke */
DECUMA_HWR_PRIVATE DECUMA_STATUS decumaSamplerStartNewStroke(DECUMA_SAMPLER_DATA * pSamData, 
										  DECUMA_UINT32 arcID );

DECUMA_HWR_PRIVATE DECUMA_STATUS decumaSamplerAddPoints(DECUMA_SAMPLER_DATA * pSamData, DECUMA_POINT * pPoints, 
	int nPoints, DECUMA_UINT32 sid, int * pnAdded);

DECUMA_HWR_PRIVATE DECUMA_STATUS decumaSamplerAddPoint(DECUMA_SAMPLER_DATA * pSamData, 
									DECUMA_COORD x,DECUMA_COORD y,
									DECUMA_UINT32 sid);

DECUMA_HWR_PRIVATE DECUMA_STATUS decumaSamplerCommitStroke(DECUMA_SAMPLER_DATA * pSamData, DECUMA_UINT32 sid);

/*Gives a pointer to the actual stroke in the sampler */
/*Note that this function transfers ownership of the stroke returned in ppStroke */
/*to the caller. That is, the caller has to free ppStroke after usage by calling decumaSamplerStrokeDestroy(ppStroke) */
DECUMA_HWR_PRIVATE DECUMA_STATUS decumaSamplerCommitAndRetrieveStroke(DECUMA_SAMPLER_DATA * pSamData, DECUMA_UINT32 sid,
	DECUMA_SAMPLER_STROKE ** ppStroke);


/*Returns a pointer to a sampler stroke in (ppStroke). The sampler will still hold the stroke. */
DECUMA_HWR_PRIVATE DECUMA_STATUS decumaSamplerAccessStroke(DECUMA_SAMPLER_DATA * pSamData, DECUMA_UINT32 sid,
									  DECUMA_SAMPLER_STROKE ** ppStroke);

/* Returns a pointer to a sampler stroke in (ppStroke), where idx corresponds to the samplers own array idx. */
DECUMA_HWR_PRIVATE DECUMA_STATUS decumaSamplerAccessStrokeWithoutArcid(DECUMA_SAMPLER_DATA * pSamData, DECUMA_UINT32 idx,
	DECUMA_SAMPLER_STROKE ** ppStroke);

/*Cancels sampling of one of stroke and removes it from sampler */
DECUMA_HWR_PRIVATE DECUMA_STATUS decumaSamplerCancelStroke(DECUMA_SAMPLER_DATA * pSamData, DECUMA_UINT32 sid);

DECUMA_HWR_PRIVATE int decumaSamplerGetStrokeCount(DECUMA_SAMPLER_DATA * pSamData);

DECUMA_HWR_PRIVATE int decumaSamplerGetStrokeStartTime(DECUMA_SAMPLER_DATA * pSamData, 
									   int strokeIdx);

DECUMA_HWR_PRIVATE int decumaSamplerGetStrokeCommitTime(DECUMA_SAMPLER_DATA * pSamData, 
									   int strokeIdx);

DECUMA_HWR_PRIVATE DECUMA_STATUS decumaSamplerGetIdxFromStrokeID(DECUMA_SAMPLER_DATA * pSamData, DECUMA_UINT32 sid, int * pStrokeIdx);

DECUMA_HWR_PRIVATE DECUMA_STATUS decumaSamplerGetStrokeID(DECUMA_SAMPLER_DATA * pSamData, 
									   int strokeIdx,DECUMA_UINT32 * pArcID);

/*
int decumaSamplerGetStrokeLength(const DECUMA_SAMPLER_DATA * pSamdata, SAM_STROKE_ID sid, int * length);

int decumaSamplerGetStrokePoint(const DECUMA_SAMPLER_DATA *pSamdata, SAM_STROKE_ID sid, int nPoint, DECUMA_POINT *pPoint);

const DECUMA_POINT * decumaSamplerAccessStrokeData(const DECUMA_SAMPLER_DATA * pSamdata, SAM_STROKE_ID sid, int * pnPoints);

int decumaSamplerGetConcurrentStrokeCount(const DECUMA_SAMPLER_DATA * pSamdata, SAM_STROKE_ID sid);

void decumaSamplerGetConcurrentStrokeIDs(const DECUMA_SAMPLER_DATA * pSamdata, SAM_STROKE_ID * pSIDs, int nMaxSIDs);

int decumaSamplerDeleteStroke(DECUMA_SAMPLER_DATA * pSamdata, SAM_STROKE_ID sid);

//Return value
//	0 if sid is not a valid ID. 1 otherwise.
int decumaSamplerGetStrokeAverage(const DECUMA_SAMPLER_DATA * pSamdata, SAM_STROKE_ID sid, DECUMA_POINT * pTarget);

//Return value
//	0 if sid is not a valid ID. 1 otherwise.
int decumaSamplerGetStrokeStart(const DECUMA_SAMPLER_DATA * pSamdata, SAM_STROKE_ID sid, DECUMA_POINT * pTarget);

//Return value
//	0 if sid is not a valid ID. 1 otherwise.
int decumaSamplerGetStrokeStop(const DECUMA_SAMPLER_DATA * pSamdata, SAM_STROKE_ID sid, DECUMA_POINT * pTarget);

//Return value
//	0 if sid is not a valid ID. 1 otherwise.
void decumaSamplerGetStrokeBounds(DECUMA_SAMPLER_DATA * pSamdata, SAM_STROKE_ID sid, DECUMA_POINT * pTopLeft,
						 DECUMA_POINT * pBottomRight, int penSize);

void decumaSamplerMoveStroke( DECUMA_SAMPLER_DATA * pSamdata, SAM_STROKE_ID arcID, short deltaX, short deltaY);
*/

/*Also sets *ppStroke to NULL */
DECUMA_HWR_PRIVATE int decumaSamplerStrokeGetPointCount(const DECUMA_SAMPLER_STROKE * pStroke);

/*Return the members of pArc with  */
DECUMA_HWR_PRIVATE const DECUMA_POINT * decumaSamplerStrokeAccessPoints(const DECUMA_SAMPLER_STROKE * pStroke);

DECUMA_HWR_PRIVATE void decumaSamplerStrokeDestroy(DECUMA_SAMPLER_DATA * pSamData, 
								DECUMA_SAMPLER_STROKE ** ppStroke);



#ifdef __cplusplus
}/*extern "C" { */
#endif


#endif /* DECUMA_HWR_SAMPLER_H */
