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


#ifndef DECUMA_HWR_SAMPLER_DATA_H
#define DECUMA_HWR_SAMPLER_DATA_H

#if defined(_DEBUG) && !defined(DECUMA_HWR_SAMPLER_C)
#error Direct data access in debug mode only allowed from decumaHwrSampler.c
#endif

#include "decuma_hwr_types.h"

struct _DECUMA_SAMPLER_STROKE
{
	DECUMA_UINT32  id;		        /* The strokes identifier */
	DECUMA_POINT * pPointBuffer;	/* Pointer to the points */
	int   nPoints; 
	int   nPointBufferSize;
	DECUMA_UINT32 startTime;        /*The value of samplerClock when this stroke was started */
	DECUMA_UINT32 commitTime;       /*The value of samplerClock when this stroke was ended */
};

struct _DECUMA_SAMPLER_DATA
{
	int nMaxStrokes;         /*The maximum number of concurrently sampling strokes */
	/*int nMaxPointsPerStroke; //The maximum length of a stroke */
	int nPointGranularity;   /*The number of points that is (re-)allocated at each time */
	
	int nStrokes;   /* The number of currently concurrently sampling strokes */
	DECUMA_SAMPLER_STROKE ** ppStrokes; /*Array of pointers to strokes, length = nStrokes */
	
	/*DECUMA_UINT32 idcnt; // A counter that is increased for each new stroke to be sampled, used for the stroke IDs */
	DECUMA_UINT32 samplerClock; /* A time used for identifying time-overlapping arcs. Simply increased on each stroke start and stop */

	const DECUMA_MEM_FUNCTIONS * pMemFunctions;

};





#endif /*DECUMA_HWR_SAMPLER_DATA_H */
