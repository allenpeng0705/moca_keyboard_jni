/*******************************************************************************
;*******************************************************************************
;**                                                                           **
;**                  COPYRIGHT 2001-2011 NUANCE COMMUNICATIONS                **
;**                                                                           **
;**               NUANCE COMMUNICATIONS PROPRIETARY INFORMATION               **
;**                                                                           **
;**     This software is supplied under the terms of a license agreement      **
;**     or non-disclosure agreement with Nuance Communications and may not    **
;**     be copied or disclosed except in accordance with the terms of that    **
;**     agreement.                                                            **
;**                                                                           **
;**                                                                           **
;*******************************************************************************
;**                                                                           **
;**     FileName: et9aimu.h                                                   **
;**                                                                           **
;**  Description: Keyboard layout module for ET9.                             **
;**                                                                           **
;*******************************************************************************
;******************************************************************************/

#ifndef ET9AIMU_H
#define ET9AIMU_H 1


#include "et9api.h"

/* don't mangle the function name if compile under C++ */
#if defined(__cplusplus)
extern "C" {
#endif



/*------------------------------------------------------------------
 * Functions
 *------------------------------------------------------------------*/

void ET9FARCALL _ET9AWImminentSymb(ET9BaseLingInfo      * const pBaseLingInfo,
                                   const ET9BOOL                bImplicitLock);


/* End don't mangle the function name if compile under C++ */
#if defined (__cplusplus)
    }
#endif

#endif /* ET9AIMU_H */
/* ----------------------------------< eof >--------------------------------- */

