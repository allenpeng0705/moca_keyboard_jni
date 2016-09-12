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
;**     FileName: et9sym.h                                                    **
;**                                                                           **
;**  Description: ET9 symbol information                                      **
;**                                                                           **
;*******************************************************************************/

#ifndef ET9SYM_H
#define ET9SYM_H 1


#include "et9api.h"

/* don't mangle the function name if compile under C++ */
#if defined(__cplusplus)
extern "C" {
#endif


ET9BOOL ET9FARCALL _ET9SymIsLower(const ET9SYMB sSymb, const ET9U16 wLdbNum);
ET9BOOL ET9FARCALL _ET9SymIsUpper(const ET9SYMB sSymb, const ET9U16 wLdbNum);
ET9SYMB ET9FARCALL _ET9SymToLower(const ET9SYMB sSymb, const ET9U16 wLdbNum);
ET9SYMB ET9FARCALL _ET9SymToUpper(const ET9SYMB sSymb, const ET9U16 wLdbNum);
ET9SYMB ET9FARCALL _ET9SymToOther(const ET9SYMB sSymb, const ET9U16 wLdbNum);


ET9BOOL ET9FARCALL _ET9_IsUnknown(const ET9SYMB sSymb);

ET9BOOL ET9FARCALL _ET9_IsWhiteSpace(const ET9SYMB sSymb);

ET9BOOL ET9FARCALL _ET9_IsPunctChar(const ET9SYMB sSymb);

ET9BOOL ET9FARCALL _ET9_IsNumeric(const ET9SYMB sSymb);

ET9BOOL ET9FARCALL _ET9_IsPunctOrNumeric(const ET9SYMB sSymb);

ET9BOOL ET9FARCALL _ET9_IsNumericString(ET9SYMB       const * const psString,
                                        const ET9UINT               nStrLen);

ET9BOOL ET9FARCALL _ET9FindSpacesAndUnknown(ET9SYMB       const * const psString,
                                            const ET9UINT               nStrLen);

ET9BOOL ET9FARCALL _ET9StringLikelyEmoticon(ET9SYMB       const * const psString,
                                            const ET9UINT               nStrLen);

extern const ET9U8 _ET9_pbFreeCharTable[0x2000];

#define _ET9_IsFree(sSymb) ((_ET9_pbFreeCharTable[(sSymb) >> 3] & (1 << ((sSymb) & 7))) != 0)

#define _ET9_IS_COMMON_CORRECTION_SYMB(sSymb) _ET9_IsFree(sSymb)

/**
 * Symbol classes & classification
 */

typedef enum ET9SymbClass_e {
  ET9_WhiteSymbClass,
  ET9_PunctSymbClass,
  ET9_NumbrSymbClass,
  ET9_AlphaSymbClass,
  ET9_UnassSymbClass,

  ET9_LastSymbClass

} ET9SymbClass;

ET9SymbClass ET9FARCALL _ET9_GetSymbolClass (const ET9SYMB sSymb);

/**
 * Deprecated functions and defines
 */

#define ET9SYMWHITEMASK ET9SYMWHITE
#define ET9SYMPUNCTMASK ET9SYMPUNCT
#define ET9SYMNUMBRMASK ET9SYMNUMBR
#define ET9SYMALPHAMASK ET9SYMALPHA
#define ET9SYMUNKNMASK  ET9SYMUNKN

ET9BOOL ET9FARCALL _ET9_GetFullSymbolKeyAndClass(const ET9SYMB      sSymb,
                                                 ET9U8      * const pbKey,
                                                 ET9U8      * const pbClass);

/**
 * Symbol encodings
 */

typedef enum ET9SYMBOL_ENCODING_e {
    ET9SYMBOL_ENCODING_UNICODE,
    ET9SYMBOL_ENCODING_SHIFTJIS,

    ET9SYMBOL_ENCODING_LAST

} ET9SYMBOL_ENCODING;

ET9SYMBOL_ENCODING ET9FARCALL ET9_GetSymbolEncoding(void);


/* End don't mangle the function name if compile under C++ */
#if defined (__cplusplus)
    }
#endif

#endif /* !ET9SYM_H */
