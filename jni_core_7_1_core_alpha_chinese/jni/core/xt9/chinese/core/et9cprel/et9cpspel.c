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
;**     FileName: et9cpspel.c                                                 **
;**                                                                           **
;**  Description: Chinese Phrase Text Input phonetic spell module.            **
;**               Conforming to the development version of Chinese XT9.       **
;**                                                                           **
;*******************************************************************************
;******************************************************************************/

#include "et9cpapi.h"
#include "et9cpsys.h"
#include "et9cpkey.h"
#include "et9cpldb.h"
#include "et9cptone.h"
#include "et9cpspel.h"
#include "et9cprdb.h"
#include "et9cpinit.h"
#include "et9cppbuf.h"
#include "et9cpmisc.h"
#include "et9cpname.h"
#include "et9cstrie.h"

#define ET9CPSTAT_GROUPNOMORE  ((ET9STATUS)200)


ET9U8 ET9FARCALL ET9_CP_AddExplSpells(ET9CPLingInfo * pET9CPLingInfo,
                                      ET9U8          *pbSpell,
                                      ET9U8          bSpellLen);

/* assume caller has setup spell type */
ET9INT ET9FARCALL ET9_CP_CountSyl(ET9CPMode eMode, const ET9_CP_Spell *pSpell)
{
    ET9INT iSylCount;
    ET9U8 bLetter, b;

    ET9Assert(pSpell);
    iSylCount = 0;
    switch (eMode) {
    case ET9CPMODE_PINYIN:
        for (b = 0; b < pSpell->bLen; b++) {
            bLetter = pSpell->pbChars[b];
            if (ET9_CP_IsPinyinUpperCase(bLetter) || b > 0 && ET9_CP_IsToneOrDelim(pSpell->pbChars[b-1])) {
                iSylCount++;
            }
        }
        break;
    case ET9CPMODE_BPMF:
        for (b = 0; b < pSpell->bLen; b++) {
            bLetter = pSpell->pbChars[b];
            if (ET9_CP_IsBpmfUpperCase(bLetter) || b > 0 && ET9_CP_IsToneOrDelim(pSpell->pbChars[b-1])) {
                iSylCount++;
            }
        }
        break;
    case ET9CPMODE_STROKE:
        for (b = ET9_CP_STROKE_SPELL_HEADER_LEN; b < pSpell->bLen; b++) {
            bLetter = pSpell->pbChars[b];
            if (ET9CPSYLLABLEDELIMITER == bLetter) {
                iSylCount++;
            }
        }
        if (pSpell->bLen > ET9_CP_STROKE_SPELL_HEADER_LEN && pSpell->pbChars[pSpell->bLen-1] != ET9CPSYLLABLEDELIMITER )
        {
            iSylCount++;
        }
        break;
    }
    return iSylCount;
} /* END ET9_CP_CountSyl */

ET9STATUS ET9FARCALL ET9_CP_MakeExactInputPhrase(ET9CPLingInfo *pCLing, ET9CPPhrase * psPhrase)
{
    ET9WordSymbInfo *pWordSymbInfo = pCLing->Base.pWordSymbInfo;
    ET9UINT nKey;
    ET9UINT nNumKeys = pWordSymbInfo->bNumSymbs;
    ET9SimpleWord word;
    ET9STATUS status;
    ET9UINT nWordLen;

    status = ET9GetExactWord(pWordSymbInfo, &word, NULL, NULL);
    if (ET9STATUS_NONE != status) {
        return status;
    }

    psPhrase->bLen = 0;

    nKey = ET9_CP_SelectionHistUnselectedStart(&pCLing->SelHistory);

    if (nKey >= nNumKeys) {
        return ET9STATUS_NO_MATCHING_WORDS;
    }

    nWordLen = __ET9Min(ET9CPMAXPHRASESIZE, nNumKeys - nKey);
    _ET9SymCopy(psPhrase->pSymbs, word.sString + nKey, nWordLen);
    psPhrase->bLen = (ET9U8)nWordLen;

    return ET9STATUS_NONE;
}

ET9STATUS ET9FARCALL ET9_CP_MakeNumPhrase(ET9CPLingInfo *pCLing, ET9CPPhrase * psPhrase)
{
    ET9WordSymbInfo *pWSI = pCLing->Base.pWordSymbInfo;
    ET9UINT nKey;
    ET9UINT nNumKeys = pWSI->bNumSymbs;

    psPhrase->bLen = 0;

    nKey = ET9_CP_SelectionHistUnselectedStart(&pCLing->SelHistory);

    if (nKey >= nNumKeys) {
        return ET9STATUS_NO_MATCHING_WORDS;
    }

    for (; nKey < nNumKeys; nKey++) {
        ET9UINT nSymbsOnKey = (ET9UINT)(pWSI->SymbsInfo[nKey].DataPerBaseSym[0].bNumSymsToMatch);
        ET9UINT nSymbOnKey;

        for (nSymbOnKey = 0; nSymbOnKey < nSymbsOnKey; nSymbOnKey++)
        {
            ET9SYMB symb = pWSI->SymbsInfo[nKey].DataPerBaseSym[0].sChar[nSymbOnKey];
            if (ET9SYMNUMBR == ET9GetSymbolClass(symb)) {
                psPhrase->pSymbs[psPhrase->bLen++] = symb;
                break;
            }
        }
        if (nSymbOnKey == nSymbsOnKey) {
            return ET9STATUS_NO_MATCHING_WORDS;
        }
        if (ET9CPMAXPHRASESIZE == psPhrase->bLen) {
            break;
        }
    }
    return ET9STATUS_NONE;
}

ET9STATUS ET9FARCALL ET9_CP_GetSymPhrase(ET9CPLingInfo *pCLing, ET9CPPhrase * psPhrase, ET9U16 wIndex)
{
    ET9WordSymbInfo *pWSI = pCLing->Base.pWordSymbInfo;
    ET9UINT nKey;
    ET9U16 wSkipped = 0;
    ET9U8 bNumBaseSymbs;
    ET9U8 bBaseSymb;

    psPhrase->bLen = 0;

    nKey = ET9_CP_SelectionHistUnselectedStart(&pCLing->SelHistory);

    if (1 != pWSI->bNumSymbs - nKey) {
        return ET9STATUS_NO_MATCHING_WORDS;
    }

    bNumBaseSymbs = pWSI->SymbsInfo[nKey].bNumBaseSyms;

    for (bBaseSymb = 0; bBaseSymb < bNumBaseSymbs; bBaseSymb++)
    {
        ET9UINT nSymbsOnKey = (ET9UINT)(pWSI->SymbsInfo[nKey].DataPerBaseSym[bBaseSymb].bNumSymsToMatch);
        ET9UINT nSymbOnKey;

        for (nSymbOnKey = 0; nSymbOnKey < nSymbsOnKey; nSymbOnKey++)
        {
            ET9SYMB symb = pWSI->SymbsInfo[nKey].DataPerBaseSym[bBaseSymb].sChar[nSymbOnKey];
            ET9U8 bSymbClass = ET9GetSymbolClass(symb);

            if (ET9SYMPUNCT == bSymbClass || ET9SYMWHITE == bSymbClass) {
                if (wSkipped == wIndex) {
                    psPhrase->bLen = 1;
                    psPhrase->pSymbs[0] = symb;
                    return ET9STATUS_NONE;
                }
                wSkipped++;
            }
        }
    }
    return ET9STATUS_OUT_OF_RANGE;
}

#ifdef ET9_ALPHABETIC_MODULE
static ET9STATUS ET9LOCALCALL ET9_CP_AWPostShift(ET9CPLingInfo *pCLing, ET9AWLingInfo *pAWLing, ET9CPPhraseType ePhraseType)
{
    ET9STATUS status = ET9STATUS_NONE;
    ET9U8 bUnused;
    ET9U8 bCurrListIndex = 0;

    if (ePhraseType != pCLing->CommonInfo.SpecialPhrase.eLastAlphaPhraseType) {
        ET9POSTSHIFTMODE ePostShiftMode;

        switch (ePhraseType) {
            case ET9CPPhraseType_abc:
                ePostShiftMode = ET9POSTSHIFTMODE_LOWER;
                break;
            case ET9CPPhraseType_Abc:
                ePostShiftMode = ET9POSTSHIFTMODE_INITIAL;
                break;
            default:
                ET9Assert(ET9CPPhraseType_ABC == ePhraseType);
                ePostShiftMode = ET9POSTSHIFTMODE_UPPER;
        }
        status = ET9AWSelLstPostShift(pAWLing, ePostShiftMode, &bUnused, &bCurrListIndex);

        if (ET9STATUS_NONE == status) {
            pCLing->CommonInfo.SpecialPhrase.eLastAlphaPhraseType = ePhraseType;
        }
    }
    return status;
}

ET9STATUS ET9FARCALL ET9_CP_GetAlphaPhrase(ET9CPLingInfo *pCLing, ET9CPPhrase * psPhrase, ET9U16 wIndex, ET9CPPhraseType ePhraseType)
{
    ET9STATUS status;
    ET9AWWordInfo * pWordInfo;
    ET9AWLingInfo * pAWLing;
    ET9U8 bUnused;

    psPhrase->bLen = 0;

    if (!ET9CPIsAWActive(pCLing)) {
        return ET9STATUS_NO_INIT;
    }
    pAWLing = pCLing->pAWLing;

    if (0x100 <= wIndex) {
        return ET9STATUS_OUT_OF_RANGE;
    }

    status = ET9AWSelLstGetWord(pAWLing, &pWordInfo, 0);

    if (ET9STATUS_NEED_SELLIST_BUILD == status) {
        status = ET9AWSelLstBuild(pAWLing, &bUnused, &bUnused);
        pCLing->CommonInfo.SpecialPhrase.eLastAlphaPhraseType = ET9CPPhraseType_Unknown;
    }

    if (ET9STATUS_NONE != status) {
        return status;
    }

    status = ET9_CP_AWPostShift(pCLing, pAWLing, ePhraseType);

    if (ET9STATUS_NONE != status) {
        return status;
    }

    status = ET9AWSelLstGetWord(pAWLing, &pWordInfo, (ET9U8)wIndex);
    if (ET9STATUS_NONE == status) {
        ET9UINT nWordLen = __ET9Min(pWordInfo->wWordLen, ET9CPMAXPHRASESIZE);
        _ET9SymCopy(psPhrase->pSymbs, pWordInfo->sWord, nWordLen);
        psPhrase->bLen = (ET9U8)nWordLen;
    }

    return status;
}
#else
ET9STATUS ET9FARCALL ET9_CP_GetAlphaPhrase(ET9CPLingInfo *pCLing, ET9CPPhrase * psPhrase, ET9U16 wIndex, ET9CPPhraseType ePhraseType)
{
    pCLing;
    psPhrase;
    wIndex;
    ePhraseType;

    return ET9STATUS_BAD_PARAM;
}
#endif


/** Enable/disable Mohu Pinyin spelling pairs.
 *
 *  Note: Mohu is not supported in traditional Chinese.
 *
 *  @param pET9CPLingInfo     pointer to chinese information structure.
 *  @param wMohuPairBitMask   can be one or more of the following bit flag values:
 *                           - ET9CPMOHU_PAIR_Z_ZH_MASK   --- Z and ZH are treated the same
 *                           - ET9CPMOHU_PAIR_C_CH_MASK   --- C and CH are treated the same
 *                           - ET9CPMOHU_PAIR_S_SH_MASK   --- S and SH are treated the same
 *                           - ET9CPMOHU_PAIR_N_L_MASK    --- N and L are treated the same
 *                           - ET9CPMOHU_PAIR_R_L_MASK    --- R and L are treated the same
 *                           - ET9CPMOHU_PAIR_F_H_MASK    --- F and H are treated the same
 *                           - ET9CPMOHU_PAIR_AN_ANG_MASK --- AN and ANG are treated the same
 *                           - ET9CPMOHU_PAIR_EN_ENG_MASK --- EN and ENG are treated the same
 *                           - ET9CPMOHU_PAIR_IN_ING_MASK --- IN and ING are treated the same
 *
 * @return ET9STATUS_NONE if the options are set
 * @return ET9STATUS_NO_INIT if Chinese XT9 is not initialized.
 *
 */
ET9STATUS ET9FARCALL ET9CPSetMohuPairs(ET9CPLingInfo *pET9CPLingInfo, ET9U16 wMohuPairBitMask)
{
    ET9STATUS mStatus = ET9STATUS_NONE;

    ET9_CP_CHECK_LINGINFO(pET9CPLingInfo);

    if (pET9CPLingInfo->CommonInfo.wMohuFlags != wMohuPairBitMask) {
        if (ET9CPIsModePinyin(pET9CPLingInfo)) {
            /* clear build cache */
            ET9_CP_INVALIDATE_BUILD_CLEAR_CACHE(pET9CPLingInfo);
        }
        pET9CPLingInfo->CommonInfo.wMohuFlags = wMohuPairBitMask;
    }
    return mStatus;
}

/** Determines whether a given Mohu Pinyin pair is enabled.
 *
 * See ::ET9CPMOHU_PAIR
 *
 * @param pET9CPLingInfo     pointer to chinese information structure.
 * @param eMohuPair          pointer to chinese information structure.
 *
 * @return     1 if the given mohu pair is enabled
 * @return     0 if it is disabled  OR  Chinese XT9 is not initialized.
 */
ET9BOOL ET9FARCALL ET9CPIsMohuPairEnabled(ET9CPLingInfo *pET9CPLingInfo, ET9CPMOHU_PAIR eMohuPair)
{
    if (ET9_CP_IS_LINGINFO_NOINIT(pET9CPLingInfo))  {
        return 0;
    }
    if ((1 << eMohuPair) & pET9CPLingInfo->CommonInfo.wMohuFlags) {
        return 1;
    } else {
        return 0;
    }
}

/** Retrieves the spelling and tone for the specified Chinese character.
 *
 *  If the character has more than one tone, it returns all the valid tones and the most common tone is listed first.
 *  For instance, Bei325. It means the character's spelling is "Bei" and has 3 different tones 3, 2 and 5. The most common tone is 3.
 *
 * @param pET9CPLingInfo     pointer to chinese information structure.
 * @param sCharacter         Chinese character's Unicode.
 * @param bAlternateNum      Index to the alternate spelling.
 * @param bGetTone           0 - do not get tone; 1 - get tone.
 * @param pSpellInfo         (output)retrieved spelling and its length.
 *
 * @return ET9STATUS_NONE          Success
 * @return ET9STATUS_NO_INIT       pET9CPLingInfo is not properly initialized
 * @return ET9STATUS_INVALID_MODE  Not in phonetic mode
 * @return ET9STATUS_BAD_PARAM     pSpellInfo is NULL
 * @return ET9STATUS_WORD_NOT_FOUND sCharacter is not in LDB
 */
ET9STATUS ET9FARCALL ET9CPGetCharSpell(ET9CPLingInfo *pET9CPLingInfo,
                                       ET9SYMB        sCharacter,
                                       ET9U8          bAlternateNum,
                                       ET9U8          bGetTone,
                                       ET9CPSpell    *pSpellInfo)
{
    ET9U16       wPID;
    ET9_CP_Spell internalSI;

    ET9_CP_CHECK_LINGINFO(pET9CPLingInfo);

    if (!ET9CPIsModePhonetic(pET9CPLingInfo))
        return ET9STATUS_INVALID_MODE;

    if (!pSpellInfo)
        return ET9STATUS_BAD_PARAM;

    /* find the character's phonetic ID */
    wPID = ET9_CP_UnicodeToPID(pET9CPLingInfo, sCharacter, bAlternateNum);
    if (ET9_CP_NOMATCH == wPID || !ET9_CP_IS_NORMAL_PID(&pET9CPLingInfo->CommonInfo, wPID)) {
        return ET9STATUS_WORD_NOT_FOUND;
    }

    /* get the syllable */
    if (!ET9_CP_PidBidToSyllable(pET9CPLingInfo, wPID, internalSI.pbChars, &internalSI.bLen, (ET9U8)ET9CPIsModeBpmf(pET9CPLingInfo) ) ) {
        return ET9STATUS_WORD_NOT_FOUND;
    }
    if (bGetTone) {
        ET9U8 b, bMain;
        ET9U8 bTone = (ET9U8)ET9_CP_LookupTone(pET9CPLingInfo, wPID);
        bMain = (ET9U8)((bTone & ET9_CP_TONE_DEL_KEY_MASK) >> ET9_CP_TONE_DEL_MASK_SHIFT);
        internalSI.pbChars[internalSI.bLen++] = (ET9U8)(bMain + ET9CPTONE1 - 1);
        for(b = 1; b < ET9_CP_TONE_DEL_MASK_SHIFT + 1; b++) {
            if ((bTone & 1) && (b != bMain)) {
                internalSI.pbChars[internalSI.bLen++] = (ET9U8)(b + ET9CPTONE1 - 1);
            }
            bTone = (ET9U8)(bTone >> 1);
        }
    }
    ET9_CP_ToExternalSpellInfo(pET9CPLingInfo, &internalSI, pSpellInfo);

    return ET9STATUS_NONE;
} /* end of ET9CPGetCharSpell() */

ET9SYMB ET9FARCALL ET9_CP_InternalSpellCodeToExternal(ET9CPLingInfo *pET9CPLingInfo, ET9U8 b)
{
    ET9SYMB symb = 0;
    ET9Assert(pET9CPLingInfo);
    if (ET9_CP_IsBpmfLetter(b) && pET9CPLingInfo) { /* 2nd condition is to avoid warning when Stroke is not compiled */
        symb = ET9_CP_BpmfInternalToExternal(b);
    }
#ifndef ET9CP_DISABLE_STROKE
    else if (ET9CPIsStrokeSymbol(b)) {
        symb = b;
    }
    else if (b == ET9_CP_COMPONENT) {
        symb = pET9CPLingInfo->Private.SPrivate.wCompUnicode;
    }
#endif
    else {
        symb = (ET9SYMB)b;  /* for any other symbols */
    }
    return symb;
}

ET9U8 ET9FARCALL ET9_CP_ExternalSpellCodeToInternal(ET9CPLingInfo *pET9CPLingInfo, ET9SYMB symb)
{
    ET9U8 b = 0;
    if (ET9CPIsBpmfSymbol(symb)) {
        b = ET9_CP_BpmfExternalToInternal(symb);
    }
    else if (ET9CPIsComponent(pET9CPLingInfo, symb)) {
        b = ET9_CP_COMPONENT;
    }
    else {
        b = (ET9U8)symb;
    }
    return b;
}

void ET9FARCALL ET9_CP_ToExternalSpellInfo(ET9CPLingInfo *pET9CPLingInfo, const ET9_CP_Spell *pInternalSI, ET9CPSpell *pExternalSI)
{
    ET9INT n;
    ET9Assert(pExternalSI != NULL);
    ET9Assert(pInternalSI != NULL);

    if ( pInternalSI->bLen > 0 ) {
        if (ET9CPIsModeStroke(pET9CPLingInfo)) {
            const ET9U8 *pb = pInternalSI->pbChars + ET9_CP_STROKE_SPELL_HEADER_LEN;
            const ET9U8 *pbEnd = pInternalSI->pbChars + pInternalSI->bLen;

            pExternalSI->bLen = 0;

            if (pInternalSI->pbChars[ET9_CP_STROKE_SPELL_COMP_LEN_INDEX]) {
                ET9U16 wComp;

                wComp = ET9_CP_ReadU16(&pInternalSI->pbChars[ET9_CP_STROKE_SPELL_COMP_INDEX]);

                pExternalSI->pSymbs[pExternalSI->bLen++] = (ET9SYMB)wComp;
                pb += pInternalSI->pbChars[ET9_CP_STROKE_SPELL_COMP_LEN_INDEX]; /* skip comp strokes */
            }
            for (; pb < pbEnd; pb++) {
                pExternalSI->pSymbs[pExternalSI->bLen++] = (ET9SYMB)*pb;
            }
            return;
        }
    }

    pExternalSI->bLen = pInternalSI->bLen;
    if (pInternalSI->bLen == 2 && pInternalSI->pbChars[0] == 0) {
        pExternalSI->pSymbs[0] = 0;
        pExternalSI->pSymbs[1] = (ET9SYMB)pInternalSI->pbChars[1];
    }
    else {
        for(n = 0; n < pInternalSI->bLen; n++) {
            ET9U8 bChar = pInternalSI->pbChars[n];
            pExternalSI->pSymbs[n] = ET9_CP_InternalSpellCodeToExternal(pET9CPLingInfo, bChar);
        }
    }
}

/* ----------------------------------< eof >--------------------------------- */
