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
;**     FileName: et9imu.h                                                    **
;**                                                                           **
;**  Description: Keyboard layout module for ET9.                             **
;**                                                                           **
;*******************************************************************************
;******************************************************************************/

#ifndef ET9IMU_H
#define ET9IMU_H 1


#include "et9api.h"

/* don't mangle the function name if compile under C++ */
#if defined(__cplusplus)
extern "C" {
#endif


#define EXACTINLIST(dwState)        (dwState & ET9SLEXACTINLIST)
#define BUILDABLEEXACT(dwState)     (dwState & ET9SLBUILDABLEEXACT)

#define ET9UNDEFINEDKEYVALUE        0xFFFF
#define ET9UNDEFINEDTAPVALUE        0xFFFF

#define UNDEFINED_STORE_INDEX       0xFFFF       /**< \internal a value that indicates that there is no valid store available */


/*------------------------------------------------------------------
 * Functions
 *------------------------------------------------------------------*/

ET9STATUS ET9FARCALL _ET9WordSymbInit(ET9WordSymbInfo * const pWordSymbInfo);

void ET9FARCALL _ET9ImminentSymb(ET9WordSymbInfo    * const pWordSymbInfo,
                                 const ET9U8                bCurrIndexInList,
                                 const ET9BOOL              bImplicitLock);

void ET9FARCALL _ET9ContentExplicified(ET9WordSymbInfo * const pWordSymbInfo);

void ET9FARCALL _ET9InvalidateOneSymb(ET9WordSymbInfo * const pWordSymbInfo, const ET9U8 bIndex);
void ET9FARCALL _ET9InvalidateOneLock(ET9WordSymbInfo * const pWordSymbInfo, const ET9U8 bIndex);
void ET9FARCALL _ET9InvalidateSymbInfo(ET9WordSymbInfo * const pWordSymbInfo);
void ET9FARCALL _ET9InvalidateSelList(ET9WordSymbInfo * const pWordSymbInfo);

void ET9FARCALL _ET9ValidateOneSymb(ET9WordSymbInfo * const pWordSymbInfo, const ET9U8 bIndex);
void ET9FARCALL _ET9ValidateAllSymbs(ET9WordSymbInfo * const pWordSymbInfo);
void ET9FARCALL _ET9ValidateOneSymbAW(ET9WordSymbInfo * const pWordSymbInfo, const ET9U8 bIndex);

void ET9FARCALL _ET9ClearShiftInfo(ET9WordSymbInfo * const pWordSymbInfo);

ET9BOOL ET9FARCALL _ET9IsMagicStringKey(ET9WordSymbInfo *pWordSymbInfo);

void ET9FARCALL _ET9SaveWord(ET9WordSymbInfo   * const pWordSymbInfo,
                             ET9SYMB           * const pString,
                             const ET9U16              wLen);

void ET9FARCALL _ET9ExplicifyWord(ET9WordSymbInfo   * const pWordSymbInfo,
                                  ET9SimpleWord     * const pWord);

ET9STATUS ET9FARCALL ET9MoveSymbs(ET9WordSymbInfo * const pWordSymbInfo,
                                  const ET9U8             bFromIndex,
                                  const ET9U8             bToIndex,
                                  const ET9U8             bCount);

ET9STATUS ET9FARCALL _ET9LockWord(ET9WordSymbInfo   * const pWordSymbInfo,
                                  ET9SimpleWord      * const pWord,
                                  const ET9AWLANGUAGESOURCE  bLanguageSource);

void ET9FARCALL _ET9TrackInputEvents(ET9WordSymbInfo * const pWordSymbInfo, const ET9InputEvent eInputEvent);

ET9BOOL ET9FARCALL _ET9IsAutoCapSituation(ET9WordSymbInfo const * const pWordSymbInfo,
                                          ET9SYMB         const * const psString,
                                          const ET9U16                  wStringLen);

ET9BOOL ET9FARCALL _ET9HasTraceInfo(ET9WordSymbInfo const * const pWordSymbInfo);

ET9BOOL ET9FARCALL _ET9HasRegionalInfo(ET9WordSymbInfo const * const pWordSymbInfo);

ET9BOOL ET9FARCALL _ET9HasDiscreteOnlyInfo(ET9WordSymbInfo const * const pWordSymbInfo);

ET9BOOL ET9FARCALL _ET9HasAllShiftedInfo(ET9WordSymbInfo const * const pWordSymbInfo);

ET9BOOL ET9FARCALL _ET9IsInhibitTapOverrideAfterTrace(ET9WordSymbInfo const * const pWordSymbInfo);

ET9BOOL ET9FARCALL _ET9IsInhibitDelayedReorderAfterTrace(ET9WordSymbInfo const * const pWordSymbInfo);

ET9STATUS ET9FARCALL _ET9SettingsInhibited(ET9WordSymbInfo const * const pWordSymbInfo);

ET9STATUS ET9FARCALL _ET9_SetAutoLocale(ET9WordSymbInfo   * const pWordSymbInfo,
                                        const ET9U16              wLocale);

void ET9FARCALL _ET9_GetDefaultLocale(ET9U16 * const pwLocale, ET9BOOL * const pbManual);

/* End don't mangle the function name if compile under C++ */
#if defined (__cplusplus)
    }
#endif

#endif /* ET9IMU_H */
/* ----------------------------------< eof >--------------------------------- */

