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

/* The purpose of this file is to provide the engine and integration code with
 * storage specifiers for certain type of data. This file will only make sure
 * that the storage specifier macros used for this are always defined. The actual
 * override of the default empty specifier definition should be done in the engine
 * build configuration file xxt9wApiOem.h
 */

#ifndef DECUMA_STORAGE_SPECIFIERS_H
#define DECUMA_STORAGE_SPECIFIERS_H

#include "xxt9wApiOem.h"

#ifndef DECUMA_DB_STORAGE
#if DECUMA_SPECIFIC_DB_STORAGE != 0
#error DECUMA_SPECIFIC_DB_STORAGE must be defined to 0 if DECUMA_DB_STORAGE is undefined
#endif
#define DECUMA_DB_STORAGE
#else
#define DECUMA_SPECIFIC_DB_STORAGE 1
#endif

#endif /* DECUMA_STORAGE_SPECIFIERS_H */
