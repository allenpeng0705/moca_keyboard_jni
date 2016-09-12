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


#ifndef DECUMA_CURVE_DATA_H
#define DECUMA_CURVE_DATA_H 1

#include "decuma_point.h"

typedef struct
{
	DECUMA_INT32 nPoints;
	DECUMA_POINT * pPoints;
} DECUMA_ARC;

typedef struct
{
	DECUMA_INT32   nArcs;
	DECUMA_ARC *   pArcs;
	DECUMA_UINT8 * pArcTimelineDiff;
} DECUMA_CURVE;


#endif
