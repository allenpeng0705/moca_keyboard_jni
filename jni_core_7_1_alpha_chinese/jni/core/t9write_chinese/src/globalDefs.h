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


        File: globalDefs.h
   $Revision: 1.2 $
     Package: GENERIC
   Copyright: Decuma AB (2002)
 Information: www.decuma.com, info@decuma.com

This file shall be included by all source code files for Latin
HWR products.

\************************************************* Header files ***/

#ifndef globalDefs_h_dfjkdjklcvxnklcvxlkjcxvzljkzxcflijkds
#define globalDefs_h_dfjkdjklcvxnklcvxlkjcxvzljkzxcflijkds



#if defined (ONPALM_ARMLET)
#include "armletDbgMsg.h"
#endif


#if defined( ONPALM_ARMLET ) && defined( __ADS )
#include "PceNativePointers.h"
/* Define a global register for Palm OS call-back. */
__global_reg(5) volatile PceNativePointers * pnp_register;
#endif


#endif
/************************************************** End of file ***/
