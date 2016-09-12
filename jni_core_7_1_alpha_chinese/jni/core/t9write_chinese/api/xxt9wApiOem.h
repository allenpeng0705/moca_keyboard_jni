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
   This file can be edited by the OEM team before building the customer deliverable,
   Note that it is part of the API and therefore modifications need to be synchronized
   with other libraries using the same API for deliveries to one customer.
*/

#ifndef XX_T9W_API_OEM_H
#define XX_T9W_API_OEM_H

#ifdef DECUMA_DB_STORAGE
#error DECUMA_DB_STORAGE is only allowed to be defined in this file
#endif

#if defined(__C30__) && defined(__dsPIC33E__)
#define DECUMA_DB_STORAGE __prog__

#elif defined(__WATCOMC__)
#define DECUMA_DB_STORAGE huge

#else

/* DECUMA_SPECIFIC_DB_STORAGE must be defined to 0 if DECUMA_DB_STORAGE is undefined */
#define DECUMA_SPECIFIC_DB_STORAGE 0

#endif

/* Remember that the file xxt9wOem.h also includes build settings.
   If the setting you look for is not affecting the API, you will find it in that file instead.
*/

#endif /* XX_T9W_API_OEM_H */
