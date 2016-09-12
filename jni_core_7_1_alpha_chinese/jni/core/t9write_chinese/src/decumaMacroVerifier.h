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

/* The purpose of this file is to provide macros the specify whether certain
 * macros have empty definitions or not (not if they are defined or not).
 */

#ifndef DECUMA_MACRO_VERIFIER_H
#define DECUMA_MACRO_VERIFIER_H

#include "decumaStorageSpecifiers.h"

/* Magic macro chain to determine if a macro has an empty definition.
 * Note that this will result in a compiler warning when tested macro
 * is empty.
 */
#define EXPAND(x) x ## 1
#define IS_EMPTY(x) EXPAND(x)

/* Check emptiness of DECUMA_DB_STORAGE and define a new macro that explicitly
   states this property with 1 for not empty and 0 for empty. */
#ifndef DECUMA_DB_STORAGE
#error DECUMA_DB_STORAGE must be defined
#endif
#if IS_EMPTY(DECUMA_DB_STORAGE) == 1 /* Note that the equality (not just existance) check here is important */
#define DECUMA_SPECIFIC_DB_STORAGE 0 /* We need to require this to be defined and check for explicit value */
#else
#define DECUMA_SPECIFIC_DB_STORAGE 1
#endif

#undef IS_EMPTY
#undef EXPAND

#endif /* DECUMA_MACRO_VERIFIER_H */
