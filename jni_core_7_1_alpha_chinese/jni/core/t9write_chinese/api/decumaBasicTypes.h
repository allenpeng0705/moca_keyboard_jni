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

/* The purpose of this file is to define containers for integers of the exact
 * sizes 8,16 and 32 bits:
 * 
 * DECUMA_UINT32
 * DECUMA_INT32
 * DECUMA_UINT16
 * DECUMA_INT16
 * DECUMA_UINT8
 * DECUMA_INT8
 * 
 * The method used here is based on the boundary definitions in limits.h,
 * since that header file should be a part of every standard C compiler.
 */

#ifndef DECUMA_BASIC_TYPES_H
#define DECUMA_BASIC_TYPES_H

#include <limits.h>

/* Verify that we have all macros we'll refer to later */
#if !(defined(UCHAR_MAX) && defined(USHRT_MAX) && defined(UINT_MAX) && defined(ULONG_MAX) )
#    error This compiler is not Standard C compliant
#endif

/* Deduce the data type for 8-bit integers */
#ifndef DECUMA_INTEGER_TYPE_8
#    if UCHAR_MAX == 0xff
#        define DECUMA_INTEGER_TYPE_8 char
#    else
#        error "Error: It is always assumed that char is 8 bits, but is not in this platform. This platform is not supported!"
#    endif
#endif

/* Deduce the data type for 16-bit integers */
#ifndef DECUMA_INTEGER_TYPE_16
#    if USHRT_MAX==0xffff
#        define DECUMA_INTEGER_TYPE_16 short
#    elif UINT_MAX==0xffff
#        define DECUMA_INTEGER_TYPE_16 int
#    else
#        error "Error: It is assumed that short or int is 16 bits, but is not in this platform: decumaBasicTypes.h needs to be manually edited."
#    endif
#endif

/* Deduce the data type for 32-bits from limits.h */
#ifndef DECUMA_INTEGER_TYPE_32
#    if USHRT_MAX==0xffffffffUL
#        define DECUMA_INTEGER_TYPE_32 short
#    elif UINT_MAX==0xffffffffUL
#        define DECUMA_INTEGER_TYPE_32 int
#    elif ULONG_MAX==0xffffffffUL
#        define DECUMA_INTEGER_TYPE_32 long
#    else
#        error "Error: It is assumed that short, int or long is 16 bits, but is not in this platform: decumaBasicTypes.h needs to be manually edited."
#    endif
#endif


/* Do the actual definitions of the basic types */

typedef unsigned DECUMA_INTEGER_TYPE_32 DECUMA_UINT32;
typedef signed   DECUMA_INTEGER_TYPE_32 DECUMA_INT32;

typedef unsigned DECUMA_INTEGER_TYPE_16 DECUMA_UINT16;
typedef signed   DECUMA_INTEGER_TYPE_16 DECUMA_INT16;

typedef unsigned DECUMA_INTEGER_TYPE_8  DECUMA_UINT8;
typedef signed   DECUMA_INTEGER_TYPE_8  DECUMA_INT8;

/* Done! Just undefine the temporary defines used. */

#undef DECUMA_INTEGER_TYPE_32
#undef DECUMA_INTEGER_TYPE_16
#undef DECUMA_INTEGER_TYPE_8

#endif
