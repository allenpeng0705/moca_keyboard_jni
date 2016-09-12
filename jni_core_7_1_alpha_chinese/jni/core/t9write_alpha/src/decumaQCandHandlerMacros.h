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

  This file provides macro interface to decumaQCandHandler objects.

\******************************************************************/

#ifndef DECUMA_QCAND_HANDLER_MACROS_H
#define DECUMA_QCAND_HANDLER_MACROS_H

#if !defined(_DEBUG) 
/*
   In debug mode only decumaQCandHandler module should be able to
   access the data directly. In release mode we allow external
   direct access by macros to improve speed. Only being able to
   access the data through access functions in debug mode makes
   sure the abstraction is kept intact (preventing spaghetti code
   evolving). The type control is enabled in both modes.
*/

#include "decumaQCandHandlerData.h"

#define decumaQCHIsFull              decumaQCHIsFullMacro
#define decumaQCHGetCount            decumaQCHGetCountMacro
#define decumaQCHGetStartOfCandRefs  decumaQCHGetStartOfCandRefsMacro
#define decumaQCHGetEndOfCandRefs    decumaQCHGetEndOfCandRefsMacro
#define decumaQCHGetPrevCandRef      decumaQCHGetPrevCandRefMacro
#define decumaQCHGetNextCandRef      decumaQCHGetNextCandRefMacro
#define decumaQCHGetKey              decumaQCHGetKeyMacro
#define decumaQCHGetCand             decumaQCHGetCandMacro

#endif /* !defined(_DEBUG) */

#if !defined(_DEBUG) || defined(DECUMA_QCAND_HANDLER_C)
/*
   Although macros are not needed in debug mode they are made
   available to decumaQCandHandler for debug verification by use in
   corresponding functions.
*/

#define decumaQCHIsFullMacro(pQCH)              ((pQCH)->nCands == (pQCH)->nMaxCands)
#define decumaQCHGetCountMacro(pQCH)            ((pQCH)->nCands)
#define decumaQCHGetStartOfCandRefsMacro(pQCH)  (&(pQCH)->pCandRefBuf[0]-1)
#define decumaQCHGetEndOfCandRefsMacro(pQCH)    ((pQCH)->pLastCandRef+1)
#define decumaQCHGetPrevCandRefMacro(pCandRef)  ((pCandRef)-1)
#define decumaQCHGetNextCandRefMacro(pCandRef)  ((pCandRef)+1)
#define decumaQCHGetKeyMacro(pCandRef)          ((pCandRef)->nKey)
#define decumaQCHGetCandMacro(pCandRef)         ((pCandRef)->pCand)

#endif /* !defined(_DEBUG) || defined(DECUMA_QCAND_HANDLER_C) */

#endif /* DECUMA_QCAND_HANDLER_MACROS_H */
