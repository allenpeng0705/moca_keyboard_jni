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
  aslArcSession objects.

\******************************************************************/

#ifndef ASL_ARC_SESSION_DATA_H
#define ASL_ARC_SESSION_DATA_H

#if defined(_DEBUG) && !defined(ASL_ARC_SESSION_C)
#error Direct data access in debug mode only allowed from aslArcSession.c
#endif

#include "aslConfig.h"

#include "decuma_point.h"
#include "decuma_hwr.h"
#include "aslArc.h"
#include "decumaHwrSampler.h"

struct _ASL_ARC_SESSION
{
	const DECUMA_MEM_FUNCTIONS* pMemFunctions;

	DECUMA_COORD           nBaseLine;
	DECUMA_COORD           nDistBaseToHelpLine;

	DECUMA_CURVE           curve; /* Holding the finished arc session arcs */

	int                    nAslArcs;
	ASL_ARC**             ppAslArcs;

	WRITING_DIRECTION       writingDirection;

	int bStartNewSymbol; /* Open arc(s) starts a new symbol (together with any concurrent arcs) */

	int bIsCorrupt;

	DECUMA_SAMPLER_DATA  * pSampler;
};

#endif /*ASL_ARC_SESSION_DATA_H */
