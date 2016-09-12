/*******************************************************************************
;*******************************************************************************
;**                                                                           **
;**                  COPYRIGHT 1999-2011 NUANCE COMMUNICATIONS                **
;**                                                                           **
;**               NUANCE COMMUNICATIONS PROPRIETARY INFORMATION               **
;**                                                                           **
;**     This software is supplied under the terms of a license agreement      **
;**     or non-disclosure agreement with Nuance Communications and may not    **
;**     be copied or disclosed except in accordance with the terms of that    **
;**     agreement.                                                            **
;**                                                                           **
;*******************************************************************************
;**                                                                           **
;**     FileName: et9asym.h                                                   **
;**                                                                           **
;**  Description: ST symbol key, class, key information                       **
;**                                                                           **
;*******************************************************************************/

#ifndef ET9ASYM_H
#define ET9ASYM_H 1

#include "et9api.h"
#ifdef ET9_ALPHABETIC_MODULE

/* don't mangle the function name if compile under C++ */
#if defined(__cplusplus)
extern "C" {
#endif

ET9SYMB ET9FARCALL _ET9_GetEmbPunctChar(ET9AWLingInfo   * const pLingInfo,
                                        const ET9U16            wLdbNum);

ET9SYMB ET9FARCALL _ET9_GetTermPunctChar(ET9AWLingInfo  * const pLingInfo,
                                         const ET9U16           wLdbNum,
                                         const ET9UINT          nSymbIndex);

ET9UINT ET9FARCALL _ET9_GetNumTermPunct(ET9AWLingInfo   * const pLingInfo,
                                        const ET9U16            wLdbNum);

ET9BOOL ET9FARCALL _ET9_IsTermPunct(ET9AWLingInfo   * const pLingInfo,
                                    const ET9U16            wLdbNum,
                                    const ET9SYMB           sSymb);

ET9STATUS ET9FARCALL _ET9_ConvertBuildBuf(ET9AWLingInfo     * const pLingInfo,
                                          ET9AWWordInfo     * const pWord);

ET9BOOL ET9FARCALL _ET9_LanguageSpecific_ApplySpcLenTwo(ET9AWLingInfo * const pLingInfo);

ET9BOOL ET9FARCALL _ET9_LanguageSpecific_ApplyDynamicRegionality(ET9AWLingInfo * const pLingInfo);

ET9BOOL ET9FARCALL _ET9_LanguageSpecific_ApplyCapsRules(ET9AWLingInfo               * const pLingInfo,
                                                        ET9AWPrivWordInfo     const * const pWord);

ET9BOOL ET9FARCALL _ET9_LanguageSpecific_ApplyAcronymRules(ET9AWLingInfo               * const pLingInfo,
                                                           ET9AWPrivWordInfo     const * const pWord);

ET9BOOL ET9FARCALL _ET9_LanguageSpecific_ApplyShifting(ET9AWLingInfo               * const pLingInfo,
                                                       ET9AWPrivWordInfo     const * const pWord);

#define _ET9_LanguageSpecific_FreeExactishSymb(sSymb)   ((sSymb) == '\'')


/* End don't mangle the function name if compile under C++ */
#if defined (__cplusplus)
    }
#endif

#endif /* ET9_ALPHABETIC_MODULE */
#endif /* !ET9ASYM_H */
