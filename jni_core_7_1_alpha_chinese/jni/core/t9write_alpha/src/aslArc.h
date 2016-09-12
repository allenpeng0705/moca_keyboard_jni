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

  This file provides the interface to create, destroy and access
  poperties of aslArc objects.

  The main function of a aslArc is to translate a DECUMA_ARC to
  a ASL_ARC. A ASL_ARC is an arc representation consisting of only
  a few specific parameters, which can be accessed from this
  interface.

\******************************************************************/

#ifndef ASL_ARC_H
#define ASL_ARC_H

#include "aslConfig.h"

typedef struct _ASL_ARC ASL_ARC;

#include "decumaCurve.h"
#include "decumaRuntimeMalloc.h"
#include "aslArcSession.h"

/*
    Creates an ASL_ARC for pOwner. The ASL_ARC will be fully processed, i.e.:

     - Extreme points identified
	 - Segments between extreme points created, iff bDoNotCreateSegments is 0

	If a pen lift arc should be created set bPenLift to non-zero. Points
	between the start point and end point are irrelevant for pen lift arcs
	and will therefore be ignored.

    If the ASL_ARC is created only to find the extreme points of an ASL_ARC created
	from the pDecumaArc (e.g. to create a pen lift arc preceeding a	non-pen lift arc)
	and will not be used for anything else, set bDoNotCreateSegments to 1. This
	avoids the very time-consuming segment creation.

    Memory will be allocated.

    Returns a const pointer to the created arc or 0 in case of failure.
*/
DECUMA_HWR_PRIVATE ASL_ARC* aslArcCreate(const ASL_ARC_SESSION* pOwner,
					  const DECUMA_ARC* pDecumaArc,
					  const DECUMA_MEM_FUNCTIONS* pMemFunctions);

/*
    Releases memory allocated for pAslArc and destroys it. pAslArc must not be referenced 
	after destruction.
*/
DECUMA_HWR_PRIVATE void aslArcDestroy(ASL_ARC* pAslArc);

/*
    Let pAslArc force start of a new symbol.
	Default by creation is to not force start of a new symbol.
*/
DECUMA_HWR_PRIVATE void aslArcStartNewSymbol(ASL_ARC* pAslArc);

/*
    Returns a const pointer to the owning arc session of pAslArc.
*/
DECUMA_HWR_PRIVATE const ASL_ARC_SESSION* aslArcGetOwner(const ASL_ARC* pAslArc);

/*
    Returns a const pointer to the decuma arc of pAslArc.
*/
DECUMA_HWR_PRIVATE const DECUMA_ARC* aslArcGetDecumaArc(const ASL_ARC* pAslArc);

/*
    Returns the arc index of pAslArc in its owning arc session.
*/
DECUMA_HWR_PRIVATE int aslArcGetIndex(const ASL_ARC* pAslArc);

/*
    Returns the minimum X coordinate of pAslArc.
*/
DECUMA_HWR_PRIVATE DECUMA_COORD aslArcGetXMin(const ASL_ARC* pAslArc);

/*
    Returns the maximum X coordinate of pAslArc.
*/
DECUMA_HWR_PRIVATE DECUMA_COORD aslArcGetXMax(const ASL_ARC* pAslArc);

/*
    Returns the minimum Y coordinate of pAslArc.
*/
DECUMA_HWR_PRIVATE DECUMA_COORD aslArcGetYMin(const ASL_ARC* pAslArc);

/*
    Returns the maximum Y coordinate of pAslArc.
*/
DECUMA_HWR_PRIVATE DECUMA_COORD aslArcGetYMax(const ASL_ARC* pAslArc);

/*
    Returns a const pointer to the start point of pAslArc.
*/
DECUMA_HWR_PRIVATE const DECUMA_POINT* aslArcGetStartPt(const ASL_ARC* pAslArc);

/*
    Returns a const pointer to the end point of pAslArc.
*/
DECUMA_HWR_PRIVATE const DECUMA_POINT* aslArcGetEndPt(const ASL_ARC* pAslArc);

/*
    Returns 1 if pAslArc forces start of a new symbol and 0 if it does not.
*/
DECUMA_HWR_PRIVATE int aslArcIsStartingNewSymbol(const ASL_ARC* pAslArc);

#ifdef EXTRA_VERIFICATION_INFO
/*
    Returns the number of points in the arc
*/
DECUMA_HWR_PRIVATE DECUMA_UINT16 aslArcGetPointCount(const ASL_ARC* pAslArc);

/*
    Returns a pointer to a decuma arc from the ASL_ARC
*/
DECUMA_HWR_PRIVATE const DECUMA_ARC * aslArcGetDecumaArc(const ASL_ARC* pAslArc);

#endif


#endif /*ASL_ARC_H */
