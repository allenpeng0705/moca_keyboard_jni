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

        File: decumaMemory.h
     Package: This file is part of the package generic
   Copyright: Decuma AB (2001)
     $Author: jianchun_meng $
   $Revision: 1.5 $
       $Date: 2011/02/14 11:41:14 $

\******************************************************************/

#ifndef DECUMA_MEMORY_H_xgjhcgfxhfxczklsadskcasdasdaskl
#define DECUMA_MEMORY_H_xgjhcgfxhfxczklsadskcasdasdaskl

#include "decumaConfig.h"

/* Check configuration.
 * One of the two macros DECUMA_USE_STDLIB_MEMORY_FUNCTIONS and DECUMA_USE_PRIVATE_MEMORY_FUNCTIONS needs to be specified.
 *
 * When DECUMA_USE_STDLIB_MEMORY_FUNCTIONS is defined, the functions decumaMemset(), decumaMemcpy() and decumaMemmove()
 * will be defined as macros for the system functions memset(), memcpy() and memmove(), or equivalent.
 *
 * When DECUMA_USE_PRIVATE_MEMORY_FUNCTIONS is defined, the functions will be proper function prototypes, with
 * implementations in decumaMemory.c
 */

#if defined(DECUMA_USE_STDLIB_MEMORY_FUNCTIONS) && defined(DECUMA_USE_PRIVATE_MEMORY_FUNCTIONS)
#error "Configuration specifies both standard and private memory functions, use just one"
#endif

#if !defined(DECUMA_USE_STDLIB_MEMORY_FUNCTIONS) && !defined(DECUMA_USE_PRIVATE_MEMORY_FUNCTIONS)
#error "You need to define either DECUMA_USE_STDLIB_MEMORY_FUNCTIONS or DECUMA_USE_PRIVATE_MEMORY_FUNCTIONS")
#endif

#ifndef DECUMA_USE_PRIVATE_MEMORY_FUNCTIONS

#define decumaMemset myMemSet
#define decumaMemcpy myMemCopy
#define decumaMemmove myMemMove
#define decumaMemcmp myMemCmp

#if (defined( ONPALM_4 ) || defined( ONPALM_6 )|| defined( ONPALM_5 )) && !defined(ONPALM_ARMLET)
/* Palm OS
	void myMemCopy(void *target, const void*source, int count);*/
	#define myMemCopy(t,s,c)	MemMove(t,s,c)
	#include <PalmOS.h>
	#define myMemSet(a,b,c)		MemSet(a,c,b)
	#define myMemMove(t,s,c)	MemMove(t,s,c)
#else
	/* Ansi */
#if defined(MATLAB_MEX_FILE)
	/* windows */
	#include "mex.h"
#else /* Not matlab */
	#include <string.h>
#endif /* end Not matlab */

	#define myMemSet	memset
	#define myMemCopy	memcpy
	#define myMemMove   memmove
	#define myMemCmp	memcmp
#endif

#else
/* Use private implementations of common memory functions */

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#if __STDC_VERSION__ < 199901L
/* If we are not C99 compliant, the 'restrict' keyword should be #define'd to nothing. */
#define restrict
#endif

DECUMA_HWR_PRIVATE void * decumaMemset ( void * ptr, int value, size_t num );
DECUMA_HWR_PRIVATE void * decumaMemcpy ( void * restrict s1, const void * restrict s2, size_t n );
DECUMA_HWR_PRIVATE void * decumaMemmove ( void * s1, const void * s2, size_t n );
DECUMA_HWR_PRIVATE int decumaMemcmp(const void * s1, const void * s2, size_t n);

/* A small hack to make this mode compatible with the older source files that use the 'myMemfunc' symbol names */
#define myMemSet decumaMemset
#define myMemCopy decumaMemcpy
#define myMemMove decumaMemmove
#define myMemCmp decumaMemcmp


#ifdef __cplusplus
} /*extern "C" {*/
#endif

#endif /* DECUMA_USE_PRIVATE_MEMORY_FUNCTIONS */

#endif /* DECUMA_MEMORY_H_xgjhcgfxhfxczklsadskcasdasdaskl */
