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
;**     FileName: et9cpsys.h                                                  **
;**                                                                           **
;**  Description: Chinese Phrase Text Input system header file.               **
;**               Conforming to the development version of Chinese XT9.       **
;**                                                                           **
;*******************************************************************************
;******************************************************************************/

#ifndef ET9CPSYS_H
#define ET9CPSYS_H 1

/* don't mangle the function name if compile under C++ */
#if defined(__cplusplus)
extern "C" {
#endif

#define ET9_CP_IS_LINGINFO_NOINIT(pET9CPLingInfo)                    \
     (!pET9CPLingInfo ||                                             \
     (pET9CPLingInfo->Private.wInfoInitOK != ET9GOODSETUP) ||        \
     (pET9CPLingInfo->Private.wLDBInitOK != ET9GOODSETUP))

#define ET9_CP_CHECK_LINGINFO(pET9CPLingInfo)        \
    if (ET9_CP_IS_LINGINFO_NOINIT(pET9CPLingInfo))   \
    {                                               \
        return(ET9STATUS_NO_INIT);                  \
    }

#define ET9_CP_CHECK_UDB_UP_TO_DATE(pET9CPLingInfo)                  \
    if (ET9_CP_IsUdbChangedByOtherThread((pET9CPLingInfo))) {    \
        return ET9STATUS_NEED_SELLIST_BUILD;                    \
    }

#define ET9_CP_IS_BUILD_OUT_OF_DATE(pET9CPLingInfo) \
        ((pET9CPLingInfo)->Base.bSelListInvalidated || \
         (pET9CPLingInfo)->Base.bSymbsInfoInvalidated)

#define ET9_CP_CHECK_BUILD_UP_TO_DATE(pET9CPLingInfo)            \
    if (ET9_CP_IS_BUILD_OUT_OF_DATE(pET9CPLingInfo)) {           \
        return ET9STATUS_NEED_SELLIST_BUILD;                     \
    }

#define ET9_CP_INVALIDATE_BUILD_CLEAR_CACHE(pET9CPLingInfo)         \
    {                                                           \
        ET9_CP_ClearBuildCache((pET9CPLingInfo));                \
        (pET9CPLingInfo)->Base.bSelListInvalidated = 1;         \
    }

typedef enum {
    ET9_CP_NO_MATCH = 0,
    ET9_CP_PARTIAL_SYL_MATCH,
    ET9_CP_PARTIAL_PHRASE_MATCH,
    ET9_CP_EXACT_MATCH
} ET9_CP_PhraseMatch;


ET9U16 ET9FARCALL ET9_CP_ReadU16(ET9U8 const * const pbData);
ET9U32 ET9FARCALL ET9_CP_ReadU32(ET9U8 const * const pbData);
void ET9FARCALL   ET9_CP_WriteU16(ET9U8 * const pbData, ET9U16 wValue);
void ET9FARCALL   ET9_CP_WriteU32(ET9U8 * const pbData, ET9U32 dwValue);
void ET9FARCALL ET9_CP_ByteMove(ET9U8 *dst, ET9U8 *src, ET9U32 size);
/* void ET9FARCALL ET9_CP_SymMove(ET9SYMB *dst, ET9SYMB *src, ET9U32 size); */
ET9INT ET9FARCALL ET9_CP_MemCmp(const ET9U8 *dst, const ET9U8 *src, ET9U32 size);

ET9UINT ET9FARCALL ET9_CP_InputContainsTrace(const ET9CPLingInfo * pET9CPLingInfo);
ET9UINT ET9FARCALL ET9_CP_WSIValidLen(const ET9CPLingInfo * const pET9CPLingInfo, const ET9WordSymbInfo * const pWordSymbInfo);

void ET9FARCALL ET9_CP_ClearBuildCache(ET9CPLingInfo *);

ET9UINT ET9FARCALL ET9_CP_PhraseIsAllChn(ET9CPLingInfo *pET9CPLingInfo,
                                        const ET9SYMB *psPhrase,
                                        ET9U8 bLen);

void ET9FARCALL ET9_CP_SpellDataClear(ET9_CP_SpellData * psSpellData);

ET9UINT ET9FARCALL ET9_CP_EndsWithInitial(ET9U8 *pbSpell, ET9UINT nSpellLen);

void ET9FARCALL ET9_CP_MatchType(const ET9_CP_CommonInfo * pCommon,
                                 const ET9SYMB * psPhrase,
                                 ET9U8 bPhraseLen,
                                 ET9BOOL fEndsWithInitial,
                                 ET9BOOL * pfInitialExpansion,
                                 ET9BOOL * pfSyllableCompletion,
                                 ET9BOOL * pfPhraseCompletion);

void ET9FARCALL ET9_CP_GetPhraseFromSpell(
    ET9CPLingInfo *pLing,
    ET9_CP_Spell *pSpell,
    ET9_CP_PhraseBuf *pPhraseBuf,
    ET9BOOL bNeedPartialSyl,
    ET9BOOL bNeedPartialPhrase,
    ET9BOOL bSearchContext);

ET9STATUS ET9FARCALL ET9_CP_GetPhrase(ET9CPLingInfo *pET9CPLingInfo, ET9U16 wPhraseIndex, ET9CPPhrase *psPhrase, ET9CPSpell *psSpell);

ET9STATUS ET9FARCALL ET9_CP_CangJieSelectPhrase(ET9CPLingInfo *pET9CPLingInfo,
                                                ET9U16 wPhraseIndex);

ET9STATUS ET9FARCALL ET9_CP_ContextFillPhraseBuffer(ET9CPLingInfo *pET9CPLingInfo);

ET9STATUS ET9FARCALL ET9_CP_JianpinFillPhraseBuffer(ET9CPLingInfo *pET9CPLingInfo);

ET9STATUS ET9FARCALL ET9_CP_CangJieFillPhraseBuffer(ET9CPLingInfo *pET9CPLingInfo);

ET9STATUS ET9FARCALL ET9_CP_PrefixFillPhraseBuffer(ET9CPLingInfo *pET9CPLingInfo);

#ifdef ET9_ALPHABETIC_MODULE
ET9STATUS ET9FARCALL ET9_CP_AWFillPhraseBuffer(ET9CPLingInfo *pET9CPLingInfo);
#endif /*END ET9_ALPHABETIC_MODULE */

ET9BOOL ET9FARCALL ET9_CP_GetHomophone(ET9CPLingInfo *pLing,
                                       const ET9CPPhrase *pBasePhrase,
                                       ET9_CP_PhraseBuf *pPhraseBuf);

#define ET9_CP_SymbIsDelim(pSymbInfo)   (ET9BOOL)(   (pSymbInfo)->bNumBaseSyms == 1 \
                                                  && (pSymbInfo)->DataPerBaseSym[0].bNumSymsToMatch == 1 \
                                                  && (pSymbInfo)->DataPerBaseSym[0].sChar[0] == ET9CPSYLLABLEDELIMITER )

#define ET9_CP_SymbIsToneOrDelim(pSymbInfo)   (ET9BOOL)(   (pSymbInfo)->bNumBaseSyms == 1 \
                                                        && (pSymbInfo)->DataPerBaseSym[0].bNumSymsToMatch == 1 \
                                                        && ( (pSymbInfo)->DataPerBaseSym[0].sChar[0] == ET9CPSYLLABLEDELIMITER \
                                                            || ET9CPSymToCPTone((pSymbInfo)->DataPerBaseSym[0].sChar[0]) ) )

#define ET9_CP_GetSymbToneOrDelim(pSymbInfo)    ( (ET9U8)( (pSymbInfo)->DataPerBaseSym[0].sChar[0]) )

ET9U8 ET9FARCALL ET9_CP_IsSymbInSymbInfo(ET9SYMB symb,
                                        const ET9SymbInfo * pSymbInfo);

ET9UINT ET9FARCALL ET9_CP_WSIValidLen(const ET9CPLingInfo * const pET9CPLingInfo,
                                        const ET9WordSymbInfo * const pWordSymbInfo);

ET9INT ET9FARCALL ET9_CP_AllowComponent(ET9CPLingInfo *pET9CPLingInfo);

#define ET9_CP_PREFIX_SORT_BY_FREQ     0
#define ET9_CP_PREFIX_SORT_BY_LENGTH   1
#define ET9_CP_PREFIX_SORT_BY_ALPHA    2

void ET9FARCALL ET9_CP_SortPrefixGrp(ET9CPLingInfo * pET9CPLingInfo);

void ET9FARCALL ET9_CP_MakeInternalJianpinSpell(ET9CPLingInfo  * const pLing,
                                                ET9CPPhrase    *pPhrase,
                                                ET9_CP_Spell   *pOutSpell);

#ifndef ET9CP_DISABLE_STROKE
/*** stroke functions ***/
ET9STATUS ET9FARCALL ET9_CP_StrokeBuildSpellings(ET9CPLingInfo * pET9CPLingInfo);

void ET9FARCALL ET9_CP_LoadStrokeSpell(ET9CPLingInfo    *pET9CPLingInfo,
                                       ET9_CP_Spell     *psSpell);

void ET9FARCALL ET9_CP_TrimStrokeSpellLength(ET9_CP_Spell   *pSpell,
                                             ET9U8          bPhraseLen);
#endif
#define ET9_CP_STROKE_SPELL_HEADER_LEN (sizeof(ET9U8) + sizeof(ET9U16) + sizeof(ET9U8))
#define ET9_CP_STROKE_SPELL_COMP_LEN_INDEX 0
#define ET9_CP_STROKE_SPELL_COMP_INDEX 1
#define ET9_CP_STROKE_SPELL_PRE_COMP_LEN_INDEX 3

void ET9FARCALL ET9_CP_SelectionHistInit(ET9_CP_SelectionHist * pSelHist);
/* return: ET9STATUS_NONE if succeeded, ET9STATUS_EMPTY if there is no selection currently */
ET9STATUS ET9FARCALL ET9_CP_SelectionHistGet(ET9_CP_SelectionHist * pSelHist,
                                            ET9CPPhrase * pUnicodePhrase,   /* Unicode string */
                                            ET9CPPhrase * pEncodedPhrase);   /* PID, SID, BID string */

/* return: ET9STATUS_NONE if succeeded, ET9STATUS_FULL if history stack is full */
ET9STATUS ET9FARCALL ET9_CP_SelectionHistAdd(ET9_CP_SelectionHist * pSelHist,
                                           const ET9SYMB * psUnicodePhrase,   /* Unicode string */
                                           const ET9SYMB * psEncodedPhrase,   /* PID, SID, BID string */
                                           ET9U8 bPraseLen,
                                           ET9U8 bConsumed);

/* return: ET9STATUS_NONE if succeeded, ET9STATUS_FULL if history stack is full */
ET9STATUS ET9FARCALL ET9_CP_SelectionHistClear(ET9_CP_SelectionHist * pSelHist);
ET9U8 ET9FARCALL ET9_CP_SelectionHistUnselectedStart(const ET9_CP_SelectionHist * pSelHist);

/* End don't mangle the function name if compile under C++ */
#if defined(__cplusplus)
    }
#endif

#endif  /* #ifndef ET9CPSYS_H */

/* ----------------------------------< eof >--------------------------------- */
