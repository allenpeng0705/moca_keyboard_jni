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

  This file contains private data structure definitions of
  t9wl session objects.

\******************************************************************/

#ifndef T9WL_DATA_H
#define T9WL_DATA_H

#if defined(_DEBUG) && !defined(T9WL_C)
#error Direct data access in debug mode only allowed from t9wl.c
#endif

#include "t9wlConfig.h"

#include "decuma_hwr.h" /* For DECUMA_SESSION_SETTINGS */
#include "scrAPI.h"

#define NUMBER_OF_SCR_RESULTS 23
#define MAX_CONCURRENT_STROKES MAX_NUMBER_OF_ARCS_IN_CURVE

struct _T9WL_SESSION
{
	const DECUMA_SESSION_SETTINGS * pSessionSettings; /* Pointer to the session settings stored in DECUMA_SESSION object */

	const DECUMA_MEM_FUNCTIONS * pMemFunctions;

	CATEGORY cat;

	CURVE charCurve; /* Committed curve */
	ARC arcsBuf[MAX_NUMBER_OF_ARCS_IN_CURVE]; /* Committed arc data (including point data) */

	DECUMA_UINT32 arcIDs[MAX_CONCURRENT_STROKES]; /* IDs of uncommitted arcs */
	int nUncommittedArcs; /* Number of uncommitted arcs. 0 means not adding points. */

	DECUMA_POINT pointsBuf[MAX_CONCURRENT_STROKES][NUMBER_OF_POINTS_IN_ARC * 3 / 2]; /* Uncommitted point data */
	DECUMA_INT8 nUncommittedPoints[MAX_CONCURRENT_STROKES]; /* Number of uncommitted points for each uncommitted arc */

	int bAddingArcs;

	SCR_OUTPUT scr_output[NUMBER_OF_SCR_RESULTS];
	DECUMA_UINT16 proximity[NUMBER_OF_SCR_RESULTS];
	SCR_API_SETTINGS scr_api_settings;

	void* pScrMem;

	/* Store these that are calculated from the settings */
	int baselinePos; /* Y-coordinate of baseline */
	int distBaseToHelpLine; /* dist > 0 */

	int bReferenceEstimated; /* baselinePos and distBaseToHelpLine have been set to estimates
	                          * based on selected candidate of current add arc session
	                          */

	/* Store previous add arc session based baselinePos and distBaseToHelpLine to be able to restore
	 * a valid value in case new values were set based on selected candidate and then selected
	 * candidate is deleted
	 */
	int nPrevBaselinePos;
	int nPrevDistBaseToHelpLine;
	int bPrevReferenceEstimated;

	int bPositionAndScaleInvariant;

	DECUMA_COORD nBaseLineEstimates[NUMBER_OF_SCR_RESULTS]; /* Allocated copy of the estimates for each result */
	DECUMA_COORD nHelpLineEstimates[NUMBER_OF_SCR_RESULTS]; /* Allocated copy of the estimates for each result */
	int nResults; /* No of results in last recognize call */

	int nLastNotedResult; /* Last noted "correct" result from user candidate selection */

	int bIsCorrupt;

	CATEGORY_TABLE_PTR pCatTable;
};

#endif /* T9WL_DATA_H */
