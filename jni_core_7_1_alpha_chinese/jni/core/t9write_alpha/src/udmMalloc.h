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


/** @file
 *  This file contains configuration settings of the GENERIC 
 *  package for use by UDMLIB
 */

#ifndef UDM_MALLOC_H
#define UDM_MALLOC_H

#include "decumaRuntimeMalloc.h"

/** @defgroup DOX_GRP_UDM_MEM Dynamic Memory Allocation for UDMLIB
 *  @{
 */


/**
 * @hideinitializer
 * Returns memory allocated for storage of s bytes. In case of failure
 * 0 is returned.
 */
#define udmMalloc(s)      decumaModAlloc(MODID_UDMLIB, s)

/**
 * @hideinitializer
 * Returns memory allocated for storage of s bytes. Each byte of
 * the allocated memory is initialized to 0. In case of failure
 * 0 is returned.
 */
#define udmCalloc(n, s)   decumaModCalloc(MODID_UDMLIB, n, s)

/** 
 *  @hideinitializer
 *  Deallocates memory pointed to by p. p must point to memory
 *  previously allocated by udm*.
 */
 #define udmFree(p)        decumaModFree(MODID_UDMLIB, p)

/** @} */

#endif /* UDM_MALLOC_H */
