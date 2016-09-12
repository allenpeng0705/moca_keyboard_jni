/*******************************************************************************
;*******************************************************************************
;**                                                                           **
;**                    COPYRIGHT 2004-2011 NUANCE COMMUNICATIONS              **
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
;**     FileName: et9cpspel.h                                                 **
;**                                                                           **
;**  Description: Chinese Phrase Text Input spelling module header file.      **
;**               Conforming to the development version of Chinese XT9.       **
;**                                                                           **
;*******************************************************************************
;******************************************************************************/

#ifndef ET9CPSPEL_H
#define ET9CPSPEL_H 1

/* don't mangle the function name if compile under C++ */
#if defined(__cplusplus)
extern "C" {
#endif

#define ET9_CP_MAX_BPMF_SYL_SIZE       3       /* maximum length of one BPMF syllable */
#define ET9_CP_MAX_PINYIN_SYL_SIZE     6       /* maximum length of one Pinyin syllable */
#if ET9_CP_MAX_PINYIN_SYL_SIZE >= ET9_CP_MAX_BPMF_SYL_SIZE
#define ET9_CP_MAX_SYL_SIZE            ET9_CP_MAX_PINYIN_SYL_SIZE
#else
#define ET9_CP_MAX_SYL_SIZE            ET9_CP_MAX_BPMF_SYL_SIZE
#endif

typedef struct ET9_CP_Syl_s {
    ET9U8 aChars[ET9_CP_MAX_SYL_SIZE];
    ET9U8 bSize;
} ET9_CP_Syl;

ET9SYMB ET9FARCALL ET9_CP_InternalSpellCodeToExternal(ET9CPLingInfo *pET9CPLingInfo, ET9U8 b);
ET9U8   ET9FARCALL ET9_CP_ExternalSpellCodeToInternal(ET9CPLingInfo *pET9CPLingInfo, ET9SYMB symb);

void ET9_CP_ToExternalSpellInfo(ET9CPLingInfo *pET9CPLingInfo, const ET9_CP_Spell *pInternalSI, ET9CPSpell *pExternalSI);


#define ET9_CP_FILTERDELIMITER    1   /* filter delimiter only */
#define ET9_CP_FILTERTONE         2   /* filter both tone and delimiter */

#define ET9_CP_IsByte(b)        ( (ET9U8)(b) == (b) ) /* private macro to chk if b's unsigned value <= 0xFF */

#define ET9_CP_IsPhraseTypeAlpha(eType)   ((ET9CPPhraseType_abc == (eType)) || (ET9CPPhraseType_Abc == (eType)) || (ET9CPPhraseType_ABC == (eType)))

/* Pinyin spelling related definitions */
#define ET9_CP_FIRSTUPLETTER      'A'
#define ET9_CP_LASTUPLETTER       'Z'
#define ET9_CP_FIRSTLOWERLETTER   'a'
#define ET9_CP_LASTLOWERLETTER    'z'
#define ET9_CP_PHLETTERLEN        5

#define ET9_CP_IsPinyinUpperCase(bLetter) ( ET9_CP_IsByte(bLetter) && ET9_CP_FIRSTUPLETTER <= (bLetter) && (bLetter) <= ET9_CP_LASTUPLETTER )
#define ET9_CP_IsPinyinLowerCase(bLetter) ( ET9_CP_IsByte(bLetter) && ET9_CP_FIRSTLOWERLETTER <= (bLetter) && (bLetter) <= ET9_CP_LASTLOWERLETTER )
#define ET9_CP_IsPinyinLetter(bLetter)    (ET9_CP_IsPinyinUpperCase(bLetter) || ET9_CP_IsPinyinLowerCase(bLetter))
/*
 * One byte BPMF spelling format:
 *  [BPMF bit (bit 7)][Uppercase bit (bit 6)][BPMF Letter ranging from 0 to 36 (bit 5 - 0)]
 */
#define ET9_CP_BPMFSPELLBIT            0x80
#define ET9_CP_BPMFUPPERCASEBIT        0x40
#define ET9_CP_BPMFFIRSTLOWERLETTER    0x80
#define ET9_CP_BPMFLASTLOWERLETTER     0xA4
#define ET9_CP_BPMFFIRSTUPPERLETTER    0xC0
#define ET9_CP_BPMFLASTUPPERLETTER     0xE4

#define ET9_CP_IsBpmfUpperCase(bLetter)    (ET9_CP_IsByte(bLetter) && ET9_CP_BPMFFIRSTUPPERLETTER <= (bLetter) && (bLetter) <= ET9_CP_BPMFLASTUPPERLETTER)
#define ET9_CP_IsBpmfLowerCase(bLetter)    (ET9_CP_IsByte(bLetter) && ET9_CP_BPMFFIRSTLOWERLETTER <= (bLetter) && (bLetter) <= ET9_CP_BPMFLASTLOWERLETTER)
#define ET9_CP_IsBpmfLetter(bLetter)       (ET9_CP_IsBpmfUpperCase(bLetter) || ET9_CP_IsBpmfLowerCase(bLetter))

#define ET9_CP_IsUpperCase(b) (ET9_CP_IsPinyinUpperCase(b) || ET9_CP_IsBpmfUpperCase(b))
#define ET9_CP_IsLowerCase(b) (ET9_CP_IsPinyinLowerCase(b) || ET9_CP_IsBpmfLowerCase(b))

#define ET9_CP_BpmfInternalToExternal(b) (ET9SYMB)(ET9_CP_IsBpmfLowerCase(b)? ((ET9U8)(b) - ET9_CP_BPMFFIRSTLOWERLETTER + ET9CPBpmfFirstLowerSymbol) : ET9_CP_IsBpmfUpperCase(b)? ((ET9U8)(b) - ET9_CP_BPMFFIRSTUPPERLETTER + ET9CPBpmfFirstUpperSymbol): 0)
#define ET9_CP_BpmfExternalToInternal(s) (ET9U8)(ET9CPIsBpmfLowerCaseSymbol(s)? ((s) - ET9CPBpmfFirstLowerSymbol + ET9_CP_BPMFFIRSTLOWERLETTER) : ET9CPIsBpmfUpperCaseSymbol(s)? ((s) - ET9CPBpmfFirstUpperSymbol + ET9_CP_BPMFFIRSTUPPERLETTER): 0)

#define ET9_CP_ExternalPhoneticToInternal(s) (ET9U8)(ET9CPIsBpmfLowerCaseSymbol(s)? ((s) - ET9CPBpmfFirstLowerSymbol + ET9_CP_BPMFFIRSTLOWERLETTER) : ET9CPIsBpmfUpperCaseSymbol(s)? ((s) - ET9CPBpmfFirstUpperSymbol + ET9_CP_BPMFFIRSTUPPERLETTER): (s))
#define ET9_CP_InternalPhoneticToExternal(b) (ET9SYMB)(ET9_CP_IsBpmfLetter((b)) ? ET9_CP_BpmfInternalToExternal((b)) : (b))

#define ET9_CP_IsPhoneticLetter(bLetter) (ET9_CP_IsBpmfLetter(bLetter) || ET9CPIsPinyinSymbol(bLetter))

#define ET9_CP_ToLowerCase(c)  (ET9U8)((ET9_CP_IsBpmfUpperCase(c)? c - ET9_CP_BPMFFIRSTUPPERLETTER + ET9_CP_BPMFFIRSTLOWERLETTER: \
                                                                  (ET9_CP_IsPinyinUpperCase(c)? c - ET9_CP_FIRSTUPLETTER + ET9_CP_FIRSTLOWERLETTER: c)))

/* Convert a BPMF (single byte) letter to its Unicode value
 * If the input is not a BPMF letter, 0 is returned. */
#define ET9_CP_BpmfLetterToUnicode(bLetter) (ET9U16)(ET9_CP_IsBpmfLetter(bLetter) ? (((ET9U8)(bLetter) & ~(ET9_CP_BPMFSPELLBIT | ET9_CP_BPMFUPPERCASEBIT)) + ET9CPBpmfFirstLowerSymbol) : 0)

#define ET9_CP_IsDigit(c) ('0' <= (c) && (c) <= '9')

#define ET9_CP_COMPONENT               0xF6         /* stroke component */

ET9INT ET9FARCALL ET9_CP_CountSyl(ET9CPMode eMode, const ET9_CP_Spell *pSpell);
/*----------------------------------------------------------------------------
 *  Define the internal spelling function prototypes.
 *----------------------------------------------------------------------------*/
ET9STATUS ET9FARCALL ET9_CP_MakeExactInputPhrase(ET9CPLingInfo *pCLing, ET9CPPhrase * psPhrase);
ET9STATUS ET9FARCALL ET9_CP_MakeNumPhrase(ET9CPLingInfo *pCLing, ET9CPPhrase * psPhrase);
ET9STATUS ET9FARCALL ET9_CP_GetSymPhrase(ET9CPLingInfo *pCLing, ET9CPPhrase * psPhrase, ET9U16 wIndex);
ET9STATUS ET9FARCALL ET9_CP_GetAlphaPhrase(ET9CPLingInfo *pCLing, ET9CPPhrase * psPhrase, ET9U16 wIndex, ET9CPPhraseType ePhraseType);

#define ET9_CP_HasActiveSpell(pLing)    (ET9BOOL)((pLing)->CommonInfo.sActiveSpell.bLen > 0)

/* End don't mangle the function name if compile under C++ */
#if defined(__cplusplus)
    }
#endif

#endif  /* #ifndef ET9CPSPEL_H */
