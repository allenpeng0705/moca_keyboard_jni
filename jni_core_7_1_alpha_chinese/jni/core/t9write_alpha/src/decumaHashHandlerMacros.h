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

  This file provides macro interface to decumaHashHandler objects.

\******************************************************************/

#ifndef DECUMA_HASH_HANDLER_MACROS_H
#define DECUMA_HASH_HANDLER_MACROS_H

#if !defined(_DEBUG) 
/*
   In debug mode only decumaHashHandler module should be able to
   access the data directly. In release mode we allow external
   direct access by macros to improve speed. Only being able to
   access the data through access functions in debug mode makes
   sure the abstraction is kept intact (preventing spaghetti code
   evolving). The type control is enabled in both modes.
*/

#include "decumaHashHandlerData.h"

#define decumaHHGetKey              decumaHHGetKeyMacro
#define decumaHHGetData             decumaHHGetDataMacro

#endif /* !defined(_DEBUG) */

#if !defined(_DEBUG) || defined(DECUMA_HASH_HANDLER_C)
/*
   Although macros are not needed in debug mode they are made
   available to decumaHashHandler for debug verification by use in
   corresponding functions.
*/

#define decumaHHGetKeyMacro(elemRef)          ((elemRef)->nKey)
#define decumaHHGetDataMacro(elemRef)         ((void*)(elemRef)->pData)

#endif /* !defined(_DEBUG) || defined(DECUMA_HASH_HANDLER_C) */

#endif /* DECUMA_HASH_HANDLER_MACROS_H */
