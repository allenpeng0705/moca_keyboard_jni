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

  This file provides the interface to tool functions to use to
  implement other asl modules.
 
\******************************************************************/

#ifndef ASL_TOOLS_H
#define ASL_TOOLS_H

#include "aslConfig.h"

#include "decumaRuntimeMalloc.h"


/** @addtogroup DOX_GRP_MEM_API
 *  @{
 */

/** @name Memory allocation routines for module ASL_LIB
 *  @{
 *  Only these macros should be used to allocate memory for the module ASL_LIB.
 *
 *  @b header: <tt>aslTools.h</tt>
 */

/**
 * @hideinitializer
 * Returns memory allocated for storage of s bytes. Each byte of
 * the allocated memory is initialized to 0. In case of failure
 * 0 is returned.
 */
#define aslCalloc(s) decumaCalloc(1, s)


/** @fn aslFree(p)
 *  @hideinitializer
 * Deallocates memory pointed to by p. p must point to memory
 * previously allocated by aslCalloc. To facilitate prevention
 * of access attempts to freed memory, p is set to NULL if DECUMA_DEBUG_MEMORY 
 * is defined. 
 * TODO Move this behaviour to decumaModFree?
 */
#define aslFree(p) { decumaFree(p); p = NULL; }
/* Safer, but generates a lot of warnings (constant conditional): */
/* #define aslFree(p) do { decumaModFree(MODID_HWRLIB, p); p = NULL; } while (0)  */


/** @fn aslIsValidPointer(p)
 *  @hideinitializer
 *  Returns true if p is in a memory block allocated by aslCalloc.
 *  If DECUMA_DEBUG_MEMORY is not defined, a simple comparison with 0 is made.
 */
#ifdef DECUMA_DEBUG_MEMORY
#define aslIsValidPointer(p) Debug_isValidPointer(MODID_HWRLIB, (p), 0)
#else
#define aslIsValidPointer(p) (p != 0)
#endif


/** @fn aslIsValidPointer(p)
 *  @hideinitializer
 *  Returns true if p is in a memory block allocated by aslCalloc and 
 *  points to an allocated block of at least s bytes.
 *  If DECUMA_DEBUG_MEMORY is not defined, a simple comparison with 0 is made.
 */
#ifdef DECUMA_DEBUG_MEMORY
#define aslIsValidPointerOfMinSize(p, s) Debug_isValidPointerOfMinSize(MODID_HWRLIB, (p), (s))
#else
#define aslIsValidPointerOfMinSize(p, s) (p != NULL)
#endif

/** @}
 */

/** @}
 */
   
   
   
   
   
/** @defgroup DOX_GRP_ASL_UTILS Misc ASL Utilities
 *  @ingroup DOX_GRP_ASL
 *  @{
 */

/**  @returns The smaller of a and b. */
#define ASL_MIN(a,b) ((a)<(b)?(a):(b))
/**  @returns The greater of a and b. */
#define ASL_MAX(a,b) ((a)>(b)?(a):(b))

/**  @returns The absolute value of x. */
#define ASL_ABS(x) ((x)>=0 ? (x) : -(x)) 

/** The constant PI */
#define ASL_PI  3.14159265358979323846264338327950288419716939937510

/**  Set the value of a bit in a bitmask */
#define ASL_SET_BIT(mask,bitNr) ((mask) |= (1 << (bitNr)))
/**  Get the value of a bit in a bitmask */
#define ASL_GET_BIT(mask,bitNr) (((mask) & (1 << (bitNr))) != 0)


/**
 * Allocate memory for more elements in an array
 * @param[in,out]  pElems              Pointer to an array to add elements to. The array will be reallocated.
 * @param[in]      nCurrentNbrOfElems   Current number of elements in *ppElems.
 * @param[in]      nNbrOfElemsToAdd     Number of elements to add to *ppElems.
 * @param[in]      nElemTypeSize        Size of an element in the array.
 * @returns 0 in case of failed allocation, otherwise 1.
 */
DECUMA_HWR_PRIVATE int aslAddElems(void * pElems, int nCurrentNbrOfElems, int nNbrOfElemsToAdd, int nElemTypeSize, const DECUMA_MEM_FUNCTIONS* pMemFunctions);


#endif /*ASL_TOOLS_H */
