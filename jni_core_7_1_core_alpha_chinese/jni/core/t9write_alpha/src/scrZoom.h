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


/*$Header: /data/cvsroot/seattle/t9write/alphabetic/core/src/scrZoom.h,v 1.5 2011/02/14 11:41:14 jianchun_meng Exp $*/
#ifndef srcZoom_hjhberi76gf21o3fkjvblakjensvop482gp928ut1arekfuh254ligfljhvb
#define srcZoom_hjhberi76gf21o3fkjvblakjensvop482gp928ut1arekfuh254ligfljhvb

#ifdef DECUMA_MANGLE
#include "mangle.h"
#endif

#include "scrCurve.h"
#include "scrOutput.h"

typedef struct{
	DECUMA_UINT8 functionArg1;
	DECUMA_UINT8 functionArg2;
	int likelyCandidateIfTestPasses;
	const SCR_CURVE * pCurve;
	const SCR_OUTPUT * pOutput;
	SCR_CURVE * pCurveCut;
	int nMaxCuts;
}ZOOM_IN;

typedef struct{
	int bestCandidate;
	int zoomQuality;
}ZOOM_OUT;


typedef void (* ZOOM_FUNC) (ZOOM_IN *, ZOOM_OUT *);

/* Swaps the element pOut[0] and [1] if zooming indicates that that is a better solution. */
/* If nOut is less then 2, the function is a NOP. */
DECUMA_HWR_PRIVATE void zoomFilter(scrOUTPUT * pOut, const int nOut, const SCR_CURVE * pCurve,
               int nRefAngle, int nRefAngleImportance, int nMaxCuts);

#endif
