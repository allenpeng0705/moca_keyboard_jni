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
;**     FileName: et9amisc.h                                                  **
;**                                                                           **
;**  Description: Alphabetic miscellaneous tools for ET9.                     **
;**                                                                           **
;*******************************************************************************
;******************************************************************************/

#ifndef ET9AMISC_H
#define ET9AMISC_H 1

/* don't mangle the function name if compile under C++ */
#if defined(__cplusplus)
extern "C" {
#endif

#include "et9api.h"
#ifdef ET9_ALPHABETIC_MODULE
#include "et9misc.h"


/*------------------------------------------------------------------
 * Macros
 *------------------------------------------------------------------*/

#define _InitWordInfo(pWord)      { _ET9ClearMem((ET9U8*)pWord, sizeof(ET9AWWordInfo)); (pWord)->bIsTerm = 0xc; }
#define _InitPrivWordInfo(pWord)  { _ET9ClearMem((ET9U8*)pWord, sizeof(ET9AWPrivWordInfo)); (pWord)->Base.bIsTerm = 0xc; }


/*------------------------------------------------------------------
 * Functions
 *------------------------------------------------------------------*/

ET9STATUS ET9FARCALL _ET9AWSys_BasicValidityCheck(ET9AWLingInfo *pLingInfo);
ET9STATUS ET9FARCALL _ET9AWSys_BasicCmnValidityCheck(ET9AWLingCmnInfo *pLingCmnInfo);

void ET9FARCALL _ET9SimpleWordToPrivWord(ET9SimpleWord * const pSimple, ET9AWPrivWordInfo * const pPriv);
void ET9FARCALL _ET9PrivWordToSimpleWord(ET9AWPrivWordInfo * const pPriv, ET9SimpleWord * const pSimple);

#endif /* ET9_ALPHABETIC_MODULE */

/* End don't mangle the function name if compile under C++ */
#if defined(__cplusplus)
    }
#endif

#endif /* ET9AMISC_H */


/* ----------------------------------< eof >--------------------------------- */

