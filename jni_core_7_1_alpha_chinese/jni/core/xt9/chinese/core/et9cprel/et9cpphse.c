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
;**     FileName: et9cpphse.c                                                 **
;**                                                                           **
;**  Description: Chinese Phrase Text Input character and phrase module.      **
;**               Conforming to the development version of Chinese XT9.       **
;**                                                                           **
;*******************************************************************************
;******************************************************************************/

#include "et9api.h"
#include "et9cpcntx.h"
#include "et9cppbuf.h"
#include "et9cpkey.h"
#include "et9cpsys.h"
#include "et9cpinit.h"
#include "et9cprdb.h"
#include "et9cpspel.h"
#include "et9cpldb.h"
#include "et9cpmisc.h"
#include "et9cpname.h"
#include "et9cpwdls.h"
#include "et9cptone.h"
#include "et9cstrie.h"

/*
    This function fills up the phrase buffer based on the PID ranges that are set in
    ET9CPLingInfo.  The phrases include context predictions.
 */

void ET9FARCALL ET9_CP_FillPhraseBufOnRanges(
    ET9CPLingInfo *pLing,
    ET9BOOL bActIsJianpin,
    ET9U8 *pbTones)
{
    /* only Jianpin uses this function, so always bNeedPartialSyl and bNeedPartialPhrase
       only 1 caller, can eliminate (kwtodo)
    */
    ET9_CP_CommonInfo *pCommon;
    ET9_CP_PhraseBuf *pMainPhraseBuf;
    ET9U8 *pbContextLen;
    ET9U16 *pwPrefix;
    ET9U8 bPrefixLen;
    ET9U8 b;

    pCommon = &(pLing->CommonInfo);
    pMainPhraseBuf = ET9_CP_GetMainPhraseBuf(pLing);
    if (bActIsJianpin) {
        pbTones = 0;
    }
    ET9_CP_GetUdbPhrases(pLing, 1, 1, 0, 0, NULL, 0, 0, 0, pbTones, 0, pMainPhraseBuf);

    pbContextLen = pCommon->pbContextLen;
    pwPrefix = pCommon->pwContextBuf;
    bPrefixLen = 0;
    for (b = 0; pbContextLen[b]; b++) {
        bPrefixLen = (ET9U8)(bPrefixLen + pbContextLen[b]);
    }
    for (b = 0; pbContextLen[b]; b++) {
        ET9Assert(bPrefixLen);
        ET9_CP_GetUdbPhrases(pLing, 1, 1, pwPrefix, bPrefixLen, NULL, 0, 0, 0, pbTones, 0, pMainPhraseBuf);
        ET9_CP_GetLdbPhrases(pLing, 0, 1, 1, NULL, 0, 0, pbTones, pwPrefix, bPrefixLen, 0, 0, pMainPhraseBuf);
        bPrefixLen = (ET9U8)(bPrefixLen - pbContextLen[b]);
        pwPrefix += pbContextLen[b];
    }
    ET9_CP_GetLdbPhrases(pLing, 0, 1, 1, NULL, 0, 0, pbTones, 0, 0, 0, 0, pMainPhraseBuf);
}
/*---------------------------------------------------------------------------
 *
 *   Function: ET9_CP_ContextFillPhraseBuffer
 *
 *   Synopsis: This function fills phrase buffer with context predictions
 *             and common characters.
 *
 *     Input:  pET9CPLingInfo    = Pointer to Chinese XT9 LingInfo structure.
 *
 *     Return: ET9STATUS_NONE on success, otherwise return XT9 error code.
 *
 *---------------------------------------------------------------------------*/

ET9STATUS ET9FARCALL ET9_CP_ContextFillPhraseBuffer(ET9CPLingInfo *pET9CPLingInfo)
{
    ET9_CP_PhraseBuf *pMainPhraseBuf;
    ET9U16 *pwPrefix;
    ET9U8 *pbContextLen, bPrefixLen, b;

    pMainPhraseBuf = ET9_CP_GetMainPhraseBuf(pET9CPLingInfo);
    ET9Assert(pMainPhraseBuf->wLastTotal == pMainPhraseBuf->wTotal);
    ET9Assert(!ET9_CP_HasActiveSpell(pET9CPLingInfo) ); /* no input: find context suffix and common char */
    ET9Assert(0 == pET9CPLingInfo->CommonInfo.bSylCount);

    pwPrefix = pET9CPLingInfo->CommonInfo.pwContextBuf;
    pbContextLen = pET9CPLingInfo->CommonInfo.pbContextLen;
    bPrefixLen = 0;
    for (b = 0; pbContextLen[b]; b++) {
        bPrefixLen = (ET9U8)(bPrefixLen + pbContextLen[b]);
    }
    for (b = 0; pbContextLen[b]; b++) {
        ET9Assert(bPrefixLen);
        /* no input: no need for partial syl */
        ET9_CP_GetUdbPhrases(pET9CPLingInfo, 0, 1, pwPrefix, bPrefixLen, NULL, 0, 0, 0, 0, 0, pMainPhraseBuf);
        ET9_CP_GetLdbPhrases(pET9CPLingInfo, 0, 0, 1, NULL, 0, 0, 0, pwPrefix, bPrefixLen, 0, 0, pMainPhraseBuf);
        bPrefixLen = (ET9U8)(bPrefixLen - pbContextLen[b]);
        pwPrefix += pbContextLen[b];
    }

    ET9_CP_GetSmartPuncts(pET9CPLingInfo);

    if (ET9_CP_IsPhraseBufEmpty(pMainPhraseBuf) /* the current page is not full */
        || !ET9_CP_PhraseBufPageFillDone(pMainPhraseBuf) ) {
        /* build common charcters */
        if (ET9CPIsNameInputActive(pET9CPLingInfo) ) {
            ET9_CP_GetCommonNameChar(pET9CPLingInfo, 0);
        }
        else {
            ET9_CP_GetCommonChar(pET9CPLingInfo);
        }
    }
    pMainPhraseBuf->wLastTotal = pMainPhraseBuf->wTotal;
    return ET9STATUS_NONE;
}

/* ( psPhrase is in PID|SID )
   return 1 if all chars in the phrase are Chinese, 0 otherwise
   We need this function because UDB functions doesn't accept a non-Chinese PID.
*/
ET9UINT ET9FARCALL ET9_CP_PhraseIsAllChn(ET9CPLingInfo *pET9CPLingInfo, const ET9SYMB *psPhrase, ET9U8 bLen)
{
    ET9U16 wMaxChnID;
    ET9U8 b;
    ET9Assert(psPhrase);
    ET9Assert(bLen);
    if (ET9CPIsModeStroke(pET9CPLingInfo) || ET9CPIsModeCangJie(pET9CPLingInfo) || ET9CPIsModeQuickCangJie(pET9CPLingInfo) ) {
#ifndef ET9CP_DISABLE_STROKE
        wMaxChnID = pET9CPLingInfo->Private.SPrivate.wCurTotalMatch[0];  /* wCurTotalMatch[0] is the total number of normal SID */
#else
        wMaxChnID = 0;
#endif
    }
    else {
        wMaxChnID = ET9_CP_NORMAL_PID_COUNT(&pET9CPLingInfo->CommonInfo);
    }
    for(b = 0; b < bLen; b++) {
        if (psPhrase[b] >= wMaxChnID) {
            return 0;
        }
    }
    return 1;
}

static ET9UINT ET9LOCALCALL AreRangesValid(ET9CPLingInfo *pLing)
{
    ET9U8 * pbRangeEnds = pLing->CommonInfo.pbRangeEnd;
    ET9U8 bSylCount = pLing->CommonInfo.bSylCount;
    ET9U8 b;

    if (0 == pbRangeEnds[0]) {
        return 0;
    }

    for (b = 1; b < bSylCount; b++) {
        if (pbRangeEnds[b] == pbRangeEnds[b-1]) {
            return 0;
        }
    }
    return 1;
}

/* Search for phrases matching given spell in LDB/UDB, either finding best match or adding to phrase buffer
 *  If pMatchType is provided, *pMatchType will be set to the "best" match type among {eNoMatch, eExactMatch, ePartialMatch},
 *  where eExactMatch is better than ePartialMatch, and ePartialMatch is better than eNoMatch.
 *  Otherwise, pPhraseBuf should be provided. All matching phrases will be added to the phrase buffer.
 */
static void ET9LOCALCALL PhraseSearchSpell(
    ET9CPLingInfo *pLing,
    ET9_CP_Spell *pSpell,
    ET9_CP_PhraseBuf *pPhraseBuf,
    ET9BOOL bNeedPartialSyl,
    ET9BOOL bNeedPartialPhrase,
    ET9BOOL bSearchContext,
    ET9_CP_SpellMatch *pMatchType)
{
    ET9_CP_SpellMatch eUdbMatch;
    ET9U8 pbTones[ET9CPMAXPHRASESIZE];

    ET9Assert(ET9CPIsModePhonetic(pLing));

    if (pMatchType) {
        *pMatchType = eNoMatch; /* assume no match */
        ET9Assert(NULL == pPhraseBuf);
    }

    /* set up ranges/tones for spell */
    if (!ET9_CP_SpellingToPidRanges(pLing, pSpell->pbChars, pSpell->bLen) ) {
        return;
    }

    /* guard against invalid syllables in AW's output */
    if (!AreRangesValid(pLing)) {
        return;
    }

    ET9_CP_GetSpellTones(pSpell->pbChars, pSpell->bLen, pbTones);

    /* search LDB */
    ET9_CP_GetLdbPhrases(pLing, 0, bNeedPartialSyl, bNeedPartialPhrase, pMatchType, 0, NULL, pbTones, NULL, 0, 0, NULL, pPhraseBuf);

    if (pMatchType && eExactMatch == *pMatchType) {
        return; /* found exact match in LDB, so no need to search UDB */
    }

    /* search UDB */
    ET9_CP_GetUdbPhrases(pLing, bNeedPartialSyl, bNeedPartialPhrase, NULL, 0, (pMatchType ? &eUdbMatch : NULL), 0, 0, NULL, pbTones, 0, pPhraseBuf);

    if (bSearchContext) {
        ET9_CP_CommonInfo *pCommon;
        ET9U8 b;

        ET9U8 * pbContextLen;
        ET9U16 *pwPrefix;
        ET9U8 bPrefixLen;

        ET9Assert(NULL == pMatchType);

        pCommon = &(pLing->CommonInfo);

        pbContextLen = pCommon->pbContextLen;
        pwPrefix = pCommon->pwContextBuf;
        bPrefixLen = 0;

        for (b = 0; pbContextLen[b]; b++) {
            bPrefixLen = (ET9U8)(bPrefixLen + pbContextLen[b]);
        }
        for (b = 0; pbContextLen[b]; b++) {
            ET9Assert(bPrefixLen);

            ET9_CP_GetUdbPhrases(pLing, bNeedPartialSyl, bNeedPartialPhrase, pwPrefix, bPrefixLen, pMatchType, 0, 0, NULL, pbTones, 0, pPhraseBuf);
            ET9_CP_GetLdbPhrases(pLing, 0, bNeedPartialSyl, bNeedPartialPhrase, pMatchType, 0, NULL, pbTones, pwPrefix, bPrefixLen, 0, NULL, pPhraseBuf);

            bPrefixLen = (ET9U8)(bPrefixLen - pbContextLen[b]);
            pwPrefix += pbContextLen[b];
        }
    }

    if (pMatchType) {
        ET9Assert(eNoMatch == *pMatchType || ePartialMatch == *pMatchType);
        if (eNoMatch != eUdbMatch) {
            *pMatchType = eUdbMatch; /* found match in UDB, overwrite match type with UDB result */
        }
    }
}

void ET9FARCALL ET9_CP_GetPhraseFromSpell(
    ET9CPLingInfo *pLing,
    ET9_CP_Spell *pSpell,
    ET9_CP_PhraseBuf *pPhraseBuf,
    ET9BOOL bNeedPartialSyl,
    ET9BOOL bNeedPartialPhrase,
    ET9BOOL bSearchPrefix)
{
    PhraseSearchSpell(pLing, pSpell, pPhraseBuf, bNeedPartialSyl, bNeedPartialPhrase, bSearchPrefix, NULL);
}

ET9_CP_SpellMatch ET9FARCALL ET9_CP_ValidateToneSpell(
    ET9CPLingInfo *pLing,
    ET9_CP_Spell *pSpell,
    ET9BOOL bNeedPartialPhrase)
{
    ET9_CP_SpellMatch eMatchType;
    PhraseSearchSpell(pLing, pSpell, NULL, 0, bNeedPartialPhrase, 0, &eMatchType);
    return eMatchType;
}

static ET9UINT ET9LOCALCALL FirstSegmentLength(ET9_CP_Spell * pSpell) {
    ET9UINT n;
    for (n = 0; n < pSpell->bLen; n++) {
        if (ET9CP_SEGMENT_DELIMITER == pSpell->pbChars[n]) {
            break;
        }
    }
    return n;
}

/* do the syllable boundaries of this prefix match syllable starts of the given segmentation */
ET9UINT ET9FARCALL PrefixSyllablesAligned(ET9U8 *pbPrefix, ET9UINT nPrefixLen, ET9U8 *pbSpell, ET9UINT nSpellLen) {
    ET9UINT nPrefix, nSpell;

    ET9Assert(nPrefixLen && nSpellLen);

    nSpell = 0;
    nPrefix = 0; 
    while (nPrefix < nPrefixLen && nSpell < nSpellLen) {
        ET9BOOL fMoved = 0;
        if (ET9_CP_IsToneOrDelim(pbPrefix[nPrefix]) ) {
            nPrefix++; /* ignore tone/delimiter/segment delimiter in prefix since they do not affect syllable alignment */
            fMoved = 1;
        }
        if (ET9_CP_IsToneOrDelim(pbSpell[nSpell]) ) {
            nSpell++; /* ignore tone/delimiter/segment delimiter in spell since they do not affect syllable alignment */
            fMoved = 1;
        }
        if ( !fMoved )
        {
            if (ET9_CP_IsLowerCase(pbPrefix[nPrefix]) != ET9_CP_IsLowerCase(pbSpell[nSpell])) {
                return 0;
            }
            nPrefix++;
            nSpell++;
        }
    }
    if (nSpell < nSpellLen && ET9_CP_IsLowerCase(pbSpell[nSpell])) {
        return 0;
    }
    if (nPrefix < nPrefixLen) {
        return 0; /* prefix remained but spelling finished */
    }

    return 1;
}

/* is the last syllable elligible for partial pinyin */
ET9UINT ET9FARCALL ET9_CP_EndsWithInitial(ET9U8 *pbSpell, ET9UINT nSpellLen) {
    ET9U8 bLastChar;

    if (0 == nSpellLen) {
        return 0;
    }

    bLastChar = pbSpell[nSpellLen-1];
    if (bLastChar == ET9CPSYLLABLEDELIMITER) {
        ET9Assert(nSpellLen >= 2);
        bLastChar = pbSpell[nSpellLen-2];
    }

    if ('h' == bLastChar || ET9_CP_IsUpperCase(bLastChar)) {
        return 1;
    }

    return 0;
}

/* count the number of syllables, including partial pinyin syllables */
static ET9UINT ET9LOCALCALL CountSyllables(ET9U8 *pbSpell, ET9UINT nSpellLen) {
    ET9UINT n;
    ET9UINT nSylCount;

    nSylCount = 0;
    for (n = 0; n < nSpellLen; n++) {
        ET9U8 bChar = pbSpell[n];

        if (ET9_CP_IsUpperCase(bChar)) {
            nSylCount++;
        }
    }
    return nSylCount;
}

/* count the number of syllables, including partial pinyin syllables */
static ET9UINT ET9LOCALCALL FirstSylLen(ET9U8 *pbSpell, ET9UINT nSpellLen) {
    ET9UINT n;

    if (0 == nSpellLen) {
        return 0;
    }

    for (n = 1; n < nSpellLen; n++) {
        ET9U8 bChar = pbSpell[n];

        if (ET9CPSYLLABLEDELIMITER == (bChar)
            || ET9CP_SEGMENT_DELIMITER == (bChar)
            || ET9_CP_IsUpperCase(bChar))
        {
            break;
        }
    }
    return n;
}

/* return the first possible spot partial pinyin may have been applied
 * (erroneously) assume that AEONM are always partial when used in the middle of a spell */
static ET9UINT ET9LOCALCALL FirstPartialPinyin(ET9U8 *pbSpell, ET9UINT nSpellLen) {
    ET9UINT n;
    ET9UINT fFoundRyhme = 0;

    if (0 == nSpellLen) {
        return 0;
    }

    for (n = 1; n < nSpellLen; n++) {
        ET9U8 bChar = pbSpell[n];

        if (ET9_CP_IsUpperCase(bChar))
        {
            if (!fFoundRyhme) {
                break;
            }
            else {   
                fFoundRyhme = 0;
            }
        }
        else if (ET9CPSYLLABLEDELIMITER != bChar &&
                 ET9CP_SEGMENT_DELIMITER != bChar &&
                 'h' != bChar)
        {
            fFoundRyhme = 1;
        }
    }
    return n;
}

/* count the number of syllables, including partial pinyin syllables */
static ET9UINT ET9LOCALCALL SpellsEqual(ET9_CP_Spell * pSpell1, ET9_CP_Spell * pSpell2) {
    ET9U8 b;

    if (pSpell1->bLen != pSpell2->bLen) {
        return 0;
    }

    for (b = 0; b < pSpell1->bLen; b++) {
        if (pSpell1->pbChars[b] != pSpell2->pbChars[b]) {
            return 0;
        }
    }
    return 1;
}

static void ET9LOCALCALL SBISpellDataInit(ET9_CP_SpellData * psSpellData,
                                          ET9BOOL fSearchingSegment,
                                          ET9BOOL fSearchingSegmentLen,
                                          ET9BOOL fSearchingSegment1stSylLen,
                                          ET9BOOL fSearchingLastSegment,
                                          ET9BOOL fSearchingSetPrefix,
                                          ET9BOOL fEndsWithInitial,
                                          ET9BOOL fPrefixSyllablesAligned,
                                          ET9INT nFirstPartialPinyin,
                                          ET9INT nSyllableCount,
                                          ET9_CP_Spell * pSpell,
                                          ET9U16 wSegmentFreq, /* the frequency of the segmentation's best phrase */
                                          ET9BOOL fSegmentFull)
{
    ET9_CP_SBISpellData * psSBISpellData = &psSpellData->u.sSBI;
    ET9_CP_Spell * psSpellDst = &psSpellData->sSpell;

    psSpellData->eSpellSource = ET9_CP_SpellSource_SBI;

    psSBISpellData->fSearchingSegment = fSearchingSegment;
    psSBISpellData->fSearchingSegmentLen = fSearchingSegmentLen;
    psSBISpellData->fSearchingSegment1stSylLen = fSearchingSegment1stSylLen;
    psSBISpellData->fSearchingLastSegment = fSearchingLastSegment;
    psSBISpellData->fSearchingSetPrefix = fSearchingSetPrefix;
    psSBISpellData->fPrefixSyllablesAligned = fPrefixSyllablesAligned;
    psSBISpellData->fEndsWithInitial = fEndsWithInitial;
    psSBISpellData->nFirstPartialPinyin = nFirstPartialPinyin;
    psSBISpellData->nSyllableCount = nSyllableCount;

    psSBISpellData->wSegPhraseFreq = wSegmentFreq;
    psSBISpellData->fSegmentFull = fSegmentFull;

    if (fSearchingSegment) {
        psSBISpellData->fSegmentFull = 0;
    }

    if (pSpell) {
        ET9U8 b;

        for (b = 0; b < pSpell->bLen; b++) {
            psSpellDst->pbChars[b] = pSpell->pbChars[b];
        }
        psSpellDst->bLen = b;
    }
    else {
        psSpellDst->bLen = 0;
    }
}

void ET9FARCALL ET9_CP_SpellDataClear(ET9_CP_SpellData * psSpellData) {
    _ET9ClearMem((ET9U8*)psSpellData, sizeof(ET9_CP_SpellData));
    psSpellData->eSpellSource = ET9_CP_SpellSource_None;
}

ET9STATUS ET9FARCALL ET9_CP_JianpinFillPhraseBuffer(ET9CPLingInfo *pET9CPLingInfo)
{
    ET9STATUS status = ET9STATUS_INVALID_INPUT; /* assume invalid input */

    if (ET9_CP_SelectionHistUnselectedStart(&pET9CPLingInfo->SelHistory) ) {
        return ET9STATUS_INVALID_INPUT; /* cannot do Jianpin when there is selection history, which is used for SBI */
    }
    if (ET9_CP_WSIToJianpinPidRanges(pET9CPLingInfo) )
    {
        ET9_CP_PhraseBuf *pMainPhraseBuf;
        ET9CPPhrase sPhrase;
        ET9U8 bSylCount;

        pMainPhraseBuf = ET9_CP_GetMainPhraseBuf(pET9CPLingInfo);
        bSylCount = pET9CPLingInfo->CommonInfo.bSylCount;

        SBISpellDataInit(&pET9CPLingInfo->CommonInfo.SpellData, 1, 1, 1, 1, 1, 1, 1, bSylCount - 1, bSylCount, NULL, 0, 0);
        ET9_CP_FillPhraseBufOnRanges(pET9CPLingInfo, 1, 0);
        ET9_CP_SpellDataClear(&pET9CPLingInfo->CommonInfo.SpellData);

        pMainPhraseBuf->wLastTotal = pMainPhraseBuf->wTotal;

        if (!ET9_CP_IsPhraseBufEmpty(pMainPhraseBuf) ) {
            /* found phrases, set the active spell to be the Jianpin spell of 1st phrase */
            ET9_CP_GetPhraseFromBuf(pMainPhraseBuf, &sPhrase, NULL, 1);
            ET9_CP_MakeInternalJianpinSpell(pET9CPLingInfo, &sPhrase, &pET9CPLingInfo->CommonInfo.sActiveSpell);
            status = ET9STATUS_NONE;
        }
    }
    return status;
}

/* Derived struct from PhraseBuf */
#define ET9_CP_MINI_PHRASE_BUF_SIZE   100
typedef struct ET9_CP_MiniPhraseBuf_s {
    ET9_CP_PhraseBuf sPhraseBuf;
    ET9U8           pbDataBuf[ET9_CP_MINI_PHRASE_BUF_SIZE];
} ET9_CP_MiniPhraseBuf;

static void ET9LOCALCALL AddWholePhraseOfSBI(ET9CPLingInfo *pET9CPLingInfo, const ET9_CP_Spell * psSpell)
{
    ET9_CS_Candidate cand;
    ET9CPPhrase sPhrase;

    pET9CPLingInfo->CommonInfo.eCandMatchType = ET9_CS_GetCandidate(&pET9CPLingInfo->SBI, 0, &cand );
    if (ET9CPIsFullSentenceActive(pET9CPLingInfo))
    {
        ET9_CP_MiniPhraseBuf sMiniPhraseBuf;
        ET9SYMB asBuf[ET9_CP_MAX_SBI_KEY_NUMBER];
        ET9U8   b, bPhraseLen = 0;
        ET9_CP_Spell sSegSpell;
        ET9BOOL bNeedPartialSyl = (ET9BOOL)ET9CPIsPartialSpellActive(pET9CPLingInfo);

        ET9_CP_INIT_PHRASE_BUF(sMiniPhraseBuf);
        sSegSpell.bLen = 0;
        for (b = 0; b <= psSpell->bLen; b++ ) {
            if (psSpell->pbChars[b] == ET9CP_SEGMENT_DELIMITER || b == psSpell->bLen) {
                ET9BOOL bNeedPartialPhrase = (ET9BOOL)(b == psSpell->bLen);
                ET9BOOL bSearchContext = (ET9BOOL)(0 == bPhraseLen); /* search context for the first segment */

                ET9_CP_ZeroPhraseBuffer(&sMiniPhraseBuf.sPhraseBuf);
                ET9_CP_SetPageSize(&sMiniPhraseBuf.sPhraseBuf, 1);
                SBISpellDataInit(&pET9CPLingInfo->CommonInfo.SpellData,
                                 1,
                                 1,
                                 1,
                                 (ET9BOOL)(b == psSpell->bLen),
                                 (ET9BOOL)0,
                                 (ET9BOOL)ET9_CP_EndsWithInitial(sSegSpell.pbChars, sSegSpell.bLen),
                                 1,
                                 sSegSpell.bLen - FirstPartialPinyin(sSegSpell.pbChars, sSegSpell.bLen),
                                 CountSyllables(sSegSpell.pbChars, sSegSpell.bLen),
                                 &sSegSpell,
                                 0,
                                 0);
                ET9_CP_GetPhraseFromSpell(pET9CPLingInfo, &sSegSpell, &sMiniPhraseBuf.sPhraseBuf, bNeedPartialSyl, bNeedPartialPhrase, bSearchContext);
                ET9_CP_SpellDataClear(&pET9CPLingInfo->CommonInfo.SpellData);
                ET9_CP_GetPhraseFromBuf(&sMiniPhraseBuf.sPhraseBuf, &sPhrase, NULL, 1); /* ET9_CP_GetPhraseFromBuf needs 1-based index */
                _ET9SymCopy(asBuf + bPhraseLen, sPhrase.pSymbs, sPhrase.bLen);
                bPhraseLen = (ET9U8)(bPhraseLen + sPhrase.bLen);
                sSegSpell.bLen = 0;
            }
            else {
                sSegSpell.pbChars[sSegSpell.bLen++] = psSpell->pbChars[b];
            }
        }
        if ( bPhraseLen > 0 )
        {
            ET9_CP_PhraseBuf *pMainPhraseBuf;
            pMainPhraseBuf = ET9_CP_GetMainPhraseBuf(pET9CPLingInfo);
            ET9_CP_AddPhraseToBuf(pET9CPLingInfo, pMainPhraseBuf, asBuf, bPhraseLen, psSpell->pbChars, psSpell->bLen, ET9_CP_IDEncode_PID, (ET9U16)0xFFFF);
        }
    }
}

extern ET9S8 ET9FARCALL ET9_CP_CopyAddDelimiter(
    ET9U8* pcTgt,
    ET9S32 iTgtSize,
    ET9S32* piCharCopied,
    const ET9U8* pcSrc,
    ET9S32 iSrcLen,
    const ET9SymbInfo *pSymbInfo,
    ET9S32 iSymbLen );

ET9STATUS ET9FARCALL ET9_CP_PrefixFillPhraseBuffer(ET9CPLingInfo *pCLing)
{
    ET9_CP_PhraseBuf * pPhraseBuf = ET9_CP_GetMainPhraseBuf(pCLing);
    ET9_CP_SSBITrie * pSBI = &pCLing->SBI;
    ET9_CS_Prefix activePrefix;
    ET9_CP_Spell s1stSegment;
    ET9_CP_Spell sFullSegmention;

    ET9U8 bUnselectedStart = ET9_CP_SelectionHistUnselectedStart(&pCLing->SelHistory);
    ET9BOOL bSearchContext = (ET9BOOL)(0 == bUnselectedStart); /* search context for the first segment only */
    ET9UINT nInputLen;
    ET9U8 bSegment1stSylLen;
    ET9BOOL bNeedPartialSyl, bNeedPartialPhrase;

    ET9BOOL fSegmentationFull; /* 1: initial expansion not used in the segmentation */

    /* reset top segmentation freq when getting phrases for the first time */
    if (ET9_CP_IsPhraseBufEmpty(pPhraseBuf)) {
        pSBI->wSegPhraseFreq = 0;
    }
    nInputLen = pCLing->Base.pWordSymbInfo->bNumSymbs - bUnselectedStart;

    /* get active prefix */
    if (0xFF != pCLing->CommonInfo.bActivePrefix) {    
        ET9U8 bActivePrefixIdxInt = pCLing->CommonInfo.pPrefixGroup[pCLing->CommonInfo.bActivePrefix].bStartIndex;
        ET9_CS_GetPrefix(pSBI, bActivePrefixIdxInt, &activePrefix);
    }
    else {
        activePrefix.m_bPfxLen = 0;
    }
    
    /* allow partial syllable when partial spell is active */
    bNeedPartialSyl = (ET9BOOL)ET9CPIsPartialSpellActive(pCLing);

    /* get 1st segment's phrases */
    {
        ET9_CP_SpellMatch eMatchType = ET9_CP_SegmentationToSpell(pCLing, &sFullSegmention);

        if (eNoMatch == eMatchType) {
            return ET9STATUS_NONE;
        }
        ET9Assert(0 < sFullSegmention.bLen);

        /* get 1st segment from the full segmentation */
        s1stSegment = sFullSegmention;
        s1stSegment.bLen = (ET9U8)FirstSegmentLength(&s1stSegment);
        if ( s1stSegment.bLen < sFullSegmention.bLen )
        {
             AddWholePhraseOfSBI( pCLing, &sFullSegmention );
        }
        
        /* get 1st syl from 1st segment */
        bSegment1stSylLen = (ET9U8)FirstSylLen(s1stSegment.pbChars, s1stSegment.bLen);

        /* allow partial phrase match on the last segment */
        bNeedPartialPhrase = (ET9BOOL)(sFullSegmention.bLen == s1stSegment.bLen);

        SBISpellDataInit(&pCLing->CommonInfo.SpellData,
                         1,
                         1,
                         (ET9BOOL)(s1stSegment.bLen == bSegment1stSylLen),
                         (ET9BOOL)(s1stSegment.bLen == nInputLen),
                         (ET9BOOL)(0xFF != pCLing->CommonInfo.bActivePrefix),
                         (ET9BOOL)ET9_CP_EndsWithInitial(s1stSegment.pbChars, s1stSegment.bLen),
                         1,
                         nInputLen - FirstPartialPinyin(s1stSegment.pbChars, s1stSegment.bLen),
                         CountSyllables(s1stSegment.pbChars, s1stSegment.bLen),
                         &s1stSegment,
                         0,
                         0);
        ET9_CP_GetPhraseFromSpell(pCLing, &s1stSegment, pPhraseBuf, bNeedPartialSyl, bNeedPartialPhrase, bSearchContext);
        fSegmentationFull = pCLing->CommonInfo.SpellData.u.sSBI.fSegmentFull;
        ET9_CP_SpellDataClear(&pCLing->CommonInfo.SpellData);

        /* should be able to find some phrase for the default segmentation */
        ET9Assert(pPhraseBuf->wTotal);

        /* 1st phrase of 1st segment locked in 1st place, regardless of freq */
        if (pPhraseBuf->wTotal <= pPhraseBuf->bPageSize) {
            pSBI->wSegPhraseFreq = pPhraseBuf->pwFreq[0];
            pPhraseBuf->pwFreq[0] = 0xFFFF;
        }
    }

    /* get prefixes' phrases */
    {
        ET9_CS_Prefix prefix;
        ET9_CP_Spell spell;
        ET9INT i, iStartIndex, iEndIndex;
        ET9U8 bActive = pCLing->CommonInfo.bActivePrefix;
        if ( bActive == 0xFF )  /* no active prefix */
        {
            iStartIndex = 0;
            iEndIndex = (ET9INT)ET9_CS_GetPrefixCount(pSBI);
        }
        else /* has active prefix, use only this group */
        {  
            iStartIndex = (ET9INT)pCLing->CommonInfo.pPrefixGroup[bActive].bStartIndex;
            iEndIndex = (iStartIndex + pCLing->CommonInfo.pPrefixGroup[bActive].bPfxCount);
        }

        for ( i = iEndIndex - 1; i >= iStartIndex; i-- )  /* must go in decreasing order */
        {
            ET9_CP_SBISpellData * psSBISpellData;

            /* todo: use ET9_CP_SegmentationToSpell? */
            const ET9WordSymbInfo *pWSI;
            const ET9SymbInfo *pSymbInfo;
            ET9S32 iCopied;
            ET9U8   bUnselectedStart, bUnselectedLength;

            pWSI = pCLing->Base.pWordSymbInfo;
            ET9_CS_GetPrefix(pSBI, i, &prefix);

            /* copy tone/delimiter into internal prefix to allow tone filtering in phrase search */
            bUnselectedStart = ET9_CP_SelectionHistUnselectedStart(&pCLing->SelHistory);
            bUnselectedLength = (ET9U8)(pWSI->bNumSymbs - bUnselectedStart);
            pSymbInfo = pWSI->SymbsInfo + bUnselectedStart;
            spell.bLen = (ET9U8)ET9_CP_CopyAddDelimiter(spell.pbChars, sizeof(spell.pbChars) - 1, &iCopied, prefix.m_pcPfx, prefix.m_bPfxLen, pSymbInfo, bUnselectedLength);
            if ( ET9_CP_SymbIsToneOrDelim(&pSymbInfo[spell.bLen]) ) {
                spell.pbChars[iCopied++] = ET9_CP_GetSymbToneOrDelim(&pSymbInfo[spell.bLen]);
                spell.bLen++;
            }

            /* skip if we've already handled this with the s1stSegment search */
            if (SpellsEqual(&spell, &s1stSegment)) {
                continue;
            }

            psSBISpellData = &pCLing->CommonInfo.SpellData.u.sSBI;
            SBISpellDataInit(&pCLing->CommonInfo.SpellData,
                             0,
                             (ET9BOOL)(spell.bLen == s1stSegment.bLen),
                             (ET9BOOL)(spell.bLen == bSegment1stSylLen),
                             (ET9BOOL)(spell.bLen == nInputLen),
                             (ET9BOOL)(0xFF != pCLing->CommonInfo.bActivePrefix),
                             (ET9BOOL)ET9_CP_EndsWithInitial(spell.pbChars, spell.bLen),
                             (ET9BOOL)PrefixSyllablesAligned(spell.pbChars, spell.bLen, sFullSegmention.pbChars, sFullSegmention.bLen),
                             nInputLen - FirstPartialPinyin(spell.pbChars, spell.bLen),
                             CountSyllables(spell.pbChars, spell.bLen),
                             &spell,
                             pSBI->wSegPhraseFreq,
                             fSegmentationFull);
            if (1 == psSBISpellData->nSyllableCount ||
                psSBISpellData->fSearchingSetPrefix ||
                psSBISpellData->fSearchingLastSegment ||
                psSBISpellData->fSearchingSegmentLen ||
                psSBISpellData->fSearchingSegment1stSylLen ||
                psSBISpellData->fPrefixSyllablesAligned)
            {
                ET9_CP_GetPhraseFromSpell(pCLing, &spell, pPhraseBuf, bNeedPartialSyl, 0, bSearchContext);
            }
            ET9_CP_SpellDataClear(&pCLing->CommonInfo.SpellData);
        
        }
    }

    pPhraseBuf->wLastTotal = pPhraseBuf->wTotal;
    return ET9STATUS_NONE;
}

/* determine the types of completion used for this phrase */
void ET9FARCALL ET9_CP_MatchType(const ET9_CP_CommonInfo * pCommon,
                                 const ET9SYMB * psPhrase,
                                 ET9U8 bPhraseLen,
                                 ET9BOOL fEndsWithInitial,
                                 ET9BOOL * pfInitialExpansion,
                                 ET9BOOL * pfSyllableCompletion,
                                 ET9BOOL * pfPhraseCompletion)
{
    const ET9U16 *pwRange, *pwStart, *pwStartPartial, *pwEnd;
    const ET9U8 *pbRangeEnd;
    ET9U16 wID;
    ET9UINT nSylCount, i, j;

    if (!psPhrase) {
        *pfInitialExpansion = 0;
        *pfSyllableCompletion = 0;
        *pfPhraseCompletion = 0;

        return;
    }

    pwStartPartial = 0;
    wID = 0;

    pwRange = pCommon->pwRange;
    pbRangeEnd = pCommon->pbRangeEnd;
    nSylCount = pCommon->bSylCount;

    ET9Assert(pCommon && psPhrase && bPhraseLen);
    ET9Assert(bPhraseLen >= nSylCount);
    
    pwStart = pwRange;

    /* phrase completion? */
    *pfPhraseCompletion = (ET9BOOL)(nSylCount < bPhraseLen ? 1 : 0);

    /* assume no other completion by default */
    *pfInitialExpansion = 0;
    *pfSyllableCompletion = 0;

    for (i = 0, j = 0; i < nSylCount; i++) {
        wID = *psPhrase++;
        for (; j < pbRangeEnd[i]; ) { /* each mohu range */
            pwStartPartial = pwStart + 1;
            pwEnd = pwStartPartial + 1;
            if ((wID >= *pwStart) && (wID < *pwEnd)) { /* match ID range */

                /* check for partial match */
                if (pwStartPartial && (wID >= *pwStartPartial)) {
                    if (i == nSylCount - 1 && !fEndsWithInitial) {
                        *pfSyllableCompletion = 1;
                    }
                    else {
                        /* partial range in the middle of a phrase -- must be initial expansion */
                        *pfInitialExpansion = 1;
                    }
                }
                break; /* match, no need to try other mohu ranges */
            }
            pwStart += ET9_CP_ID_RANGE_SIZE; /* try next mohu range */
            j += ET9_CP_ID_RANGE_SIZE;
        } /* END loop mohu ranges for this syllable */
        ET9Assert(j <= pbRangeEnd[i]);

        pwStart = pwRange + pbRangeEnd[i];
        j = pbRangeEnd[i];
    } /* END loop syllables */
}

ET9BOOL ET9FARCALL ET9_CP_GetHomophone(ET9CPLingInfo *pLing,
                                       const ET9CPPhrase *pBasePhrase,
                                       ET9_CP_PhraseBuf *pPhraseBuf)
{
    if (ET9_CP_UniPhraseToPidRanges(pLing, pBasePhrase) ) {
        /* get all phrases into the given phrase buffer */
        do {
            pPhraseBuf->wLastTotal = pPhraseBuf->wTotal;
            /* search LDB */
            ET9_CP_GetLdbPhrases(pLing, 0, 0/*bNeedPartialSyl*/, 0/*bNeedPartialPhrase*/, NULL/*pMatchType*/, 0/*bGetToneOption*/, NULL, NULL/*pbTones*/, NULL, 0, 0, NULL, pPhraseBuf);
            /* search UDB */
            ET9_CP_GetUdbPhrases(pLing, 0/*bNeedPartialSyl*/, 0/*bNeedPartialPhrase*/, NULL, 0, NULL/*pMatchType*/, 0, 0, NULL, NULL/*pbTones*/, 0, pPhraseBuf);
        } while (pPhraseBuf->wTotal != pPhraseBuf->wLastTotal);
        return 1;
    }
    return 0;
}

/* ----------------------------------< eof >--------------------------------- */
