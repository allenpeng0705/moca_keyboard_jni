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
   This file is edited by the OEM team before building the customer deliverable.
*/

#ifndef XX_T9W_OEM_H
#define XX_T9W_OEM_H

#if 0
	/* Build the lite configuration instead of full configuration */
	#define T9WRITE_LITE_CONFIG
#endif

#if 1
	/* Use standard lib memset memcpy etc, instead of having the engine self-contained*/
	#define DECUMA_USE_STDLIB_MEMORY_FUNCTIONS
#endif

#if 1
	/* Use standard lib qsort, instead of having the engine self-contained*/
	#define DECUMA_USE_STDLIB_QSORT
#endif


/* Remember that the file xxt9wApiOem.h also includes build settings.
   If the setting you look for is affecting the API, you will find it in that file instead.
*/

#endif /* XX_T9W_OEM_H */
