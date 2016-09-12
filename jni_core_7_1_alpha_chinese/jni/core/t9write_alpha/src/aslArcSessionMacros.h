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

  This file provides macro interface to aslArcSession objects.

\******************************************************************/

#ifndef ASL_ARC_SESSION_MACROS_H
#define ASL_ARC_SESSION_MACROS_H

#if !defined(_DEBUG) 
/*
   In debug mode only aslArcSession module should be able to
   access the data directly. In release mode we allow external
   direct access by macros to improve speed. Only being able to
   access the data through access functions in debug mode makes
   sure the abstraction is kept intact (preventing spaghetti code
   evolving). The type control is enabled in both modes.
*/

#define aslArcSessionGetSegmentDB           aslArcSessionGetSegmentDBMacro
#define aslArcSessionGetDcmDTW              aslArcSessionGetDcmDTWMacro
#define aslArcSessionGetBaseline            aslArcSessionGetBaselineMacro
#define aslArcSessionGetDistBaseToHelpline  aslArcSessionGetDistBaseToHelplineMacro
#define aslArcSessionUseRefSegments         aslArcSessionUseRefSegmentsMacro
#define aslArcSessionHasEndingReference     aslArcSessionHasEndingReferenceMacro
#define aslArcSessionGetArcCount            aslArcSessionGetArcCountMacro
#define aslArcSessionGetArc                 aslArcSessionGetArcMacro
#define aslArcSessionGetSegmentCount        aslArcSessionGetSegmentCountMacro
#define aslArcSessionGetSegment             aslArcSessionGetSegmentMacro

#endif /* !defined(_DEBUG) */

#if !defined(_DEBUG) || defined(ASL_ARC_SESSION_C)
/*
   Although macros are not needed in debug mode they are made
   available to aslArcSession for debug verification by use in
   corresponding functions.
*/

#include "aslConfig.h"

#include "aslArcSessionData.h"

#define aslArcSessionGetSegmentDBMacro(pAslArcSession)           ((pAslArcSession)->pAslSegmentDB)
#define aslArcSessionGetDcmDTWMacro(pAslArcSession)              ((pAslArcSession)->pDcmDtw)
#define aslArcSessionGetBaselineMacro(pAslArcSession)            ((pAslArcSession)->nBaseLine)
#define aslArcSessionGetDistBaseToHelplineMacro(pAslArcSession)  ((pAslArcSession)->nDistBaseToHelpLine)
#define aslArcSessionUseRefSegmentsMacro(pAslArcSession)         ((pAslArcSession)->bUseRefSegments)
#define aslArcSessionHasEndingReferenceMacro(pAslArcSession)     ((pAslArcSession)->bEndingReferenceAdded)
#define aslArcSessionGetArcCountMacro(pAslArcSession)            ((pAslArcSession)->nAslArcs)
#define aslArcSessionGetArcMacro(pAslArcSession, nIndex)         ((pAslArcSession)->ppAslArcs[(nIndex)])
#define aslArcSessionGetSegmentCountMacro(pAslArcSession)        ((pAslArcSession)->nAslSegments)
#define aslArcSessionGetSegmentMacro(pAslArcSession, nIndex)     ((pAslArcSession)->ppAslSegments[(nIndex)])

#endif /* !defined(_DEBUG) || defined(ASL_ARC_SESSION_C) */

#endif /* ASL_ARC_SESSION_MACROS_H */
