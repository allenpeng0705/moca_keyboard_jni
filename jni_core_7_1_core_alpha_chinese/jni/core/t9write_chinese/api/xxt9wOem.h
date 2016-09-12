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
/* The purpose of this file is to configure engine and integration builds.
   This file is used when building asllib from the developer view.
*/

#ifndef XXT9WOEM_H
#define XXT9WOEM_H

/* Default configuration, this is suitable for a hosted execution environment */
#define DECUMA_USE_STDLIB_MATH
#define DECUMA_USE_STDLIB_MEMORY_FUNCTIONS
#define DECUMA_USE_STDLIB_QSORT

#if (DECUMA_DB_STORAGE_SPECIFIER == 1)

#error T9Write CJK does not support using this macro

#endif /* DECUMA_DB_STORAGE_SPECIFIER */


#endif /* XXT9WOEM_H */
