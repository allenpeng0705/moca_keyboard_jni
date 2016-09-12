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


#ifndef DECUMA_CONFIG_H
#define DECUMA_CONFIG_H

#include "xxt9wOem.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Define these symbols to use C standard library functions */
#ifdef T9WRITE_LITE_CONFIG
#define DECUMA_NO_DYNAMIC_ALLOCATION
#endif

/* This definition causes the dictionary integration code to include reading of Xt9 LDBs */
#define DECUMA_USE_XT9_INTEGRATION
/* Use a larger hashtable; this improves the XT9 LDB conversion. */
#define DECUMA_HH_USE_LARGE_TABLE

#ifdef T9WRITE_LITE_CONFIG
/* The HWR Engine should use the Lite engine */
#define T9WL_ENGINE
#else
/* The HWR Engine should use the ASL engine */
#define ASL_ENGINE
#endif

/* The following macros are used to hide internal symbols when building the
 * library as a single C file. However, if we build it "normally" they must
 * have some definition otherwise we'll break the build.
 */

/* Type qualifier for function prototypes and definitions
 * This is defined to "static" in t9write_rel.c
 */
#ifndef DECUMA_HWR_PRIVATE
#define DECUMA_HWR_PRIVATE
#endif

/* Type qualifier for data fields, different macros for use in the header file
 * and the actual implementation in the C file.
 * Both are defined "static" in t9write_rel.c, but for a normal build we need
 * the header file to add the "extern" qualifier.
 */
#ifndef DECUMA_HWR_PRIVATE_DATA_H
#define DECUMA_HWR_PRIVATE_DATA_H extern
#endif

#ifndef DECUMA_HWR_PRIVATE_DATA_C
#define DECUMA_HWR_PRIVATE_DATA_C
#endif

/* Enable extra APIs */
#ifndef DECUMA_HWR_ENABLE_EXTRA_API
#define DECUMA_HWR_ENABLE_EXTRA_API
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* DECUMA_GLOBAL_CONFIG_H */
