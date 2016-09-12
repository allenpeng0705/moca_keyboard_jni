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
  ucrSegment objects.

\******************************************************************/

#ifndef ASL_ARC_DATA_H
#define ASL_ARC_DATA_H

#if defined(_DEBUG) && !defined(ASL_ARC_C)
#error Direct data access in debug mode only allowed from aslArc.c
#endif

#include "aslConfig.h"

#include "decumaBasicTypes.h"
#include "decuma_point.h"
#include "aslArcSession.h"

struct _ASL_ARC
{
	const ASL_ARC_SESSION*  pOwner;      /* The ASL_ARC_SESSION that owns this ASL_ARC */

	const DECUMA_MEM_FUNCTIONS* pMemFunctions;

	DECUMA_ARC				arc;         /* copy of submitted arcs (real allocated copy of points) */

	DECUMA_COORD            nXMin;
	DECUMA_COORD            nXMax;
	DECUMA_COORD            nYMin;
	DECUMA_COORD            nYMax;

	DECUMA_POINT            startPt;
	DECUMA_POINT            endPt;

	DECUMA_INT8             bStartNewSymbol; /* This arc starts a new symbol */
};


#endif /*ASL_ARC_DATA_H */
