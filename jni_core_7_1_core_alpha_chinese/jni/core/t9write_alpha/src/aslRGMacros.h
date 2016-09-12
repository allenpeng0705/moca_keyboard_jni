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

  This file provides macro interface to aslRG objects.

\******************************************************************/

#ifndef ASL_RG_MACROS_H
#define ASL_RG_MACROS_H

#if !defined(_DEBUG) 
/*
   In debug mode only aslRG module should be able to
   access the data directly. In release mode we allow external
   direct access by macros to improve speed. Only being able to
   access the data through access functions in debug mode makes
   sure the abstraction is kept intact (preventing spaghetti code
   evolving). The type control is enabled in both modes.
*/

#define aslRGStringGetStartNode               aslRGStringGetStartNodeMacro
#define aslRGStringGetStartString             aslRGStringGetStartStringMacro
#define aslRGStringGetDiacNodeData            aslRGStringGetDiacNodeDataMacro
#define aslRGStringGetDiacNeedData            aslRGStringGetDiacNeedDataMacro

#endif /* !defined(_DEBUG) */

#if !defined(_DEBUG) || defined(ASL_RG_C)
/*
   Although macros are not needed in debug mode they are made
   available to aslRG for debug verification by use in
   corresponding functions.
*/

#include "aslConfig.h"

#include "aslRGData.h"

#define aslRGStringGetStartNodeMacro(pRgString)             ((pRgString)->nStartNode)
#define aslRGStringGetStartStringMacro(pRgString)           ((pRgString)->pStartStr)
#define aslRGStringGetDiacNodeDataMacro(pRgString)          ((pRgString)->pDiacNode)
#define aslRGStringGetDiacNeedDataMacro(pRgString)          ((pRgString)->pDiacNeedData)

#endif /* !defined(_DEBUG) || defined(ASL_RG_C) */

#endif /* ASL_RG_MACROS_H */
