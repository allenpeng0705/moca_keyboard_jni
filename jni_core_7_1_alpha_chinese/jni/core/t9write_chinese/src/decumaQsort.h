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


#ifndef DECUMA_QSORT_H
#define DECUMA_QSORT_H

#include "decumaConfig.h"

#if defined DECUMA_USE_STDLIB_QSORT

/* Use standard library qsort */
#include <stdlib.h>
#define decumaQsort qsort

#elif defined DECUMA_USE_PRIVATE_QSORT

/* Use private implementation of qsort */

DECUMA_HWR_PRIVATE void decumaQsort(void * base, unsigned long nmemb, unsigned long size, int (*compar)(const void *, const void *));

#else

#error "Need to define either DECUMA_USE_STDLIB_QSORT or DECUMA_USE_PRIVATE_QSORT"

#endif /* DECUMA_USE_STDLIB_QSORT */

#endif /* DECUMA_QSORT_H */
