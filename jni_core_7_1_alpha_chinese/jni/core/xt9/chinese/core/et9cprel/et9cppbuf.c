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
;**     FileName: et9cppbuf.c                                                 **
;**                                                                           **
;**  Description: Chinese Phrase Text Input phrase buffer read/write module.  **
;**               Conforming to the development version of Chinese XT9.       **
;**                                                                           **
;*******************************************************************************
;******************************************************************************/

#include "et9api.h"
#include "et9cpsys.h"
#include "et9cpspel.h"
#include "et9cpkey.h"
#include "et9cpldb.h"
#include "et9cppbuf.h"
#include "et9cpmisc.h"
#include "et9cpname.h"
#include "et9cpwdls.h"
#include "et9cpsldb.h"
#include "et9cprdb.h"
#include "et9cstrie.h"
#include "et9cptrace.h"
#include "et9cptone.h"

#ifdef ET9_DEBUG
int DebugCheckPhraseBuf(ET9_CP_PhraseBuf *pPhraseBuf) {
    ET9CPPhrase phrase;
    ET9_CP_Spell spell;

    if (!ET9_CP_IsPhraseBufEmpty(pPhraseBuf)) {
        ET9_CP_GetPhraseFromBuf(pPhraseBuf, &phrase, &spell, pPhraseBuf->wTotal);
    }
    return 1;
}
#endif

void ET9FARCALL ET9_CP_MoveBlockForward(ET9U8 *pbBuf, ET9U16 wLen, ET9U16 wMove)
{
    ET9U8 *pDes = pbBuf + wLen + wMove;
    ET9U8 *pbEnd = pbBuf + wLen;

    while (pbBuf < pbEnd) {
        *--pDes = *--pbEnd;
    }
}
void ET9FARCALL ET9_CP_MoveBlockBackward(ET9U8 *pbBuf, ET9U16 wLen, ET9U16 wMove)
{
    ET9U8 *pDes = pbBuf - wMove;
    ET9U8 *pbEnd = pbBuf + wLen;

    while (pbBuf < pbEnd) {
        *pDes++ = *pbBuf++;
    }
}

ET9UINT ET9FARCALL ET9_CP_PhraseBufEndsWithSingleChar(ET9_CP_PhraseBuf *pPhraseBuf)
{
    ET9Assert(pPhraseBuf);

    if (!ET9_CP_IsPhraseBufEmpty(pPhraseBuf)) {
        ET9CPPhrase sPhrase;

        ET9_CP_GetPhraseFromBuf(pPhraseBuf, &sPhrase, NULL, pPhraseBuf->wTotal);
        if (sPhrase.bLen == 1) {
            return 1;
        }
    }
    return 0;
}

void ET9FARCALL ET9_CP_ZeroPhraseBuffer(ET9_CP_PhraseBuf *psPhraseBuf) {
    /* resetting phrase buffer */
    ET9_CP_SetPageSize(psPhraseBuf, ET9CPMAXPAGESIZE);
    psPhraseBuf->bTestClear = 0;
    psPhraseBuf->wPrevFreq = 0xFFFF;
    psPhraseBuf->wTotal = 0;
    psPhraseBuf->wLastTotal = 0;
    psPhraseBuf->wUsedBufSize = 0;
}

void ET9FARCALL ET9_CP_RestorePhraseBuf(ET9_CP_PhraseBuf *pPhraseBuf)
{
    ET9Assert(pPhraseBuf && ET9_CP_IsPhraseBufEmpty(pPhraseBuf));
    pPhraseBuf->bTestClear = 0;
}

void ET9FARCALL ET9_CP_ClearPhraseBuf(ET9_CP_PhraseBuf *pPhraseBuf)
{
    ET9Assert(pPhraseBuf);
    pPhraseBuf->bTestClear = 1;
}

/* nIndex is 1-based */
void ET9FARCALL ET9_CP_GetPhraseFromBuf(ET9_CP_PhraseBuf *psPhraseBuf, ET9CPPhrase *pPhraseInfo, ET9_CP_Spell *pSpell, ET9UINT nIndex)
{
    ET9U8 *pbSrc = psPhraseBuf->pbBuf;
    ET9UINT n;

    ET9Assert(!ET9_CP_IsPhraseBufEmpty(psPhraseBuf) && nIndex > 0 && nIndex <= psPhraseBuf->wTotal);
    nIndex--;
    /* skipping the phrases */
    ET9Assert(ET9_CP_PBR_GET_PHRASE_LENGTH(pbSrc) <= ET9CPMAXPHRASESIZE);
    ET9Assert(ET9_CP_PBR_GET_SPELL_LENGTH(pbSrc) <= ET9CPMAXSPELLSIZE);
    ET9Assert(ET9_CP_PBR_GET_NEXT(pbSrc) <= psPhraseBuf->pbBuf + psPhraseBuf->wBufSize);
    for(n = 0; n < nIndex; n++) {
        pbSrc = ET9_CP_PBR_GET_NEXT(pbSrc);
        ET9Assert(ET9_CP_PBR_GET_PHRASE_LENGTH(pbSrc) <= ET9CPMAXPHRASESIZE);
        ET9Assert(ET9_CP_PBR_GET_SPELL_LENGTH(pbSrc) <= ET9CPMAXSPELLSIZE);
        ET9Assert(ET9_CP_PBR_GET_NEXT(pbSrc) <= psPhraseBuf->pbBuf + psPhraseBuf->wBufSize);
    }
    pPhraseInfo->bLen = ET9_CP_PBR_GET_PHRASE_LENGTH(pbSrc);
    ET9_CP_PBR_READ_PHRASE(pbSrc, pPhraseInfo->pSymbs);
    if (pSpell) {
        pSpell->bLen = ET9_CP_PBR_GET_SPELL_LENGTH(pbSrc);
        ET9_CP_PBR_READ_SPELL(pbSrc, pSpell->pbChars);
    }
}

/*
 * This function finds the position of wEntry in the descending sequence pwSeq
 *
 *  pass in:
 *      pwSeq           the sequence
 *      wTotal          the number of members in the sequence
 *      wEntry          the value of a new entry
 *  return:
 *      ET9U16           the 0-based position of wEntry.
 */
static ET9U16 ET9LOCALCALL ET9_CP_SearchSeq(ET9U16 *pwSeq, ET9U16 wTotal, ET9U16 wEntry)
{
    ET9UINT n;
    for(n = 0; n < wTotal; n++) {
        if (wEntry > *pwSeq++)
            return (ET9U16)n;
    }
    return wTotal;
}

/* this function compares the phrase with what are in the phrase buffer.
 * If there is a duplicate, the position of the duplicate will be return.
 *  pass in:
 *      pwPhrase        the symbol buffer of the phrase
 *      bPhraseLen      the length of the symbol buffer
 *      psPhraseBuf     the phrase buffer
 *      *pwInsert       the index of the insert position
 *      eEnc            Encoding for pwPhrase
 *  return:
 *      ET9U16           0 if no duplicate is found.  1-based index of the
 *                      first duplicate otherwise.
 *      *pwOffset       the offset of the duplicate entry in the buffer
 *      *pwInsert       the offset of the insert position, valid only if insert position is before the duplicate position
 */
#define ET9_CP_NUM_SHORTCUTS 2
/* maximum number of phrases for duplicate suppression */
#define ET9_CP_STROKE_PHRASE_DUPMAX 200
#define ET9_CP_PHONETIC_PHRASE_DUPMAX 250
static ET9U16 ET9LOCALCALL ET9_CP_LookForDupPhrase(ET9CPLingInfo *pET9CPLingInfo,
                                   const ET9SYMB *pwPhrase,
                                   ET9U8 bPhraseLen,
                                   ET9_CP_IDEncode eEnc,
                                   ET9_CP_PhraseBuf *psPhraseBuf,
                                   int   fPageFull,
                                   ET9U16 wInsert,
                                   ET9U16 *pwInsertOffset,
                                   ET9U16 *pwOffset)
{
    ET9U16 w, wLimit, wStart;
    ET9U8 *pbPBR = psPhraseBuf->pbBuf;
    ET9CPPhrase sPhrase; /* copy of currently examined phrase in the phrase buffer */
    ET9CPPhrase sNewPhrase; /* to hold the unicode value of the phrase to be added */
    ET9U16 pwID[ET9_CP_NUM_SHORTCUTS][ET9_CP_MAX_ALT_SYLLABLE];
    ET9U8 pbTotalAlt[ET9_CP_NUM_SHORTCUTS]; /* total number of IDs, including most common */
    ET9UINT fBothPidSidExist = pET9CPLingInfo->CommonInfo.sOffsets.dwSIDToPIDOffset && pET9CPLingInfo->CommonInfo.sOffsets.dwPIDToSIDOffset;

    ET9Assert(psPhraseBuf && pwPhrase && bPhraseLen <= ET9CPMAXPHRASESIZE && pwInsertOffset && pwOffset);

    { /* calculate sNewPhrase */
        ET9U16 wPID, wSID; /* wPID here means either PID/BID */
        ET9U8 b, bAltCount;

        for(b = 0; b < bPhraseLen; b++) {
            if (eEnc == ET9_CP_IDEncode_UNI) {
                sNewPhrase.pSymbs[b] = pwPhrase[b];
            }
            else {
                if (eEnc == ET9_CP_IDEncode_SID) {
                    wSID = pwPhrase[b];
                    bAltCount = ET9_CP_LookupID(pET9CPLingInfo, &wPID, wSID, 1, ET9_CP_Lookup_SIDToPID);
                    ET9Assert(bAltCount > 0);
                }
                else {
                    wPID = pwPhrase[b];
                }
                if (fBothPidSidExist && b < ET9_CP_NUM_SHORTCUTS) { /* try all alt ID for only the first 2 char in the phrase */
                    if (eEnc == ET9_CP_IDEncode_SID) {
                        pbTotalAlt[b] = ET9_CP_LookupID(pET9CPLingInfo, pwID[b], wPID, (ET9U8)ET9_CP_MAX_ALT_SYLLABLE, ET9_CP_Lookup_PIDToSID);
                        ET9Assert(pbTotalAlt[b] > 0);
                    } else {
                        bAltCount = ET9_CP_LookupID(pET9CPLingInfo, &wSID, wPID, 1, ET9_CP_Lookup_PIDToSID);
                        ET9Assert(bAltCount > 0);
                        pbTotalAlt[b] = ET9_CP_LookupID(pET9CPLingInfo, pwID[b], wSID, (ET9U8)ET9_CP_MAX_ALT_SYLLABLE, ET9_CP_Lookup_SIDToPID);
                        ET9Assert(pbTotalAlt[b] > 0);
                    }
                }
                sNewPhrase.pSymbs[b] = ET9_CP_LookupUnicode(pET9CPLingInfo, wPID);
            }
        }
    }
    *pwInsertOffset = 0;
    /* if there is no duplicate, treat the last one as if it were duplicate */
    wLimit = (ET9U16)(fPageFull?(psPhraseBuf->wTotal - 1):psPhraseBuf->wTotal);
    if (!ET9CPIsModePhonetic(pET9CPLingInfo)) {
        wStart = (ET9U16)((wLimit > ET9_CP_STROKE_PHRASE_DUPMAX)?(wLimit - ET9_CP_STROKE_PHRASE_DUPMAX):0);
    }
    else {
        wStart = (ET9U16)((wLimit > ET9_CP_PHONETIC_PHRASE_DUPMAX)?(wLimit - ET9_CP_PHONETIC_PHRASE_DUPMAX):0);
    }
    for(w = 0; w < wLimit; w++) {
        if (w == wInsert) {
            *pwInsertOffset = (ET9U16)(pbPBR - psPhraseBuf->pbBuf);
        }
        sPhrase.bLen = ET9_CP_PBR_GET_PHRASE_LENGTH(pbPBR);
        if ((sPhrase.bLen == bPhraseLen) && (w >= wStart)) {
            ET9U16 *pwBuf = sNewPhrase.pSymbs;
            ET9U16 *pwTmp = sPhrase.pSymbs;

            /* constructing sPhrase */
            ET9_CP_PBR_READ_PHRASE(pbPBR, sPhrase.pSymbs);
            /* testing if the phrases pwPhrase and sPhrase match */
            for(sPhrase.bLen = 0; sPhrase.bLen < bPhraseLen; sPhrase.bLen++) {
                ET9U16 wTmp;
                wTmp = *pwTmp++;
                if (fBothPidSidExist && (sPhrase.bLen < ET9_CP_NUM_SHORTCUTS) && eEnc != ET9_CP_IDEncode_UNI) {
                    /* use alt ID to do matching (quicker) */
                    ET9U8 b;
                    ET9UINT fFound = 0;
                    for(b = 0; b < pbTotalAlt[sPhrase.bLen]; b++) {
                        if (wTmp == pwID[sPhrase.bLen][b]) {
                            fFound = 1;
                            break;
                        }
                    }
                    pwBuf++;
                    if (!fFound) {
                        break;  /* not a match */
                    }
                }
                else {
                    /* use Unicode to do matching (slower) */
                    if (eEnc == ET9_CP_IDEncode_SID) {
                        ET9U8 bAltCount;
                        bAltCount = ET9_CP_LookupID(pET9CPLingInfo, &wTmp, wTmp, 1, ET9_CP_Lookup_SIDToPID);
                        ET9Assert(bAltCount > 0);
                    }
                    if (eEnc != ET9_CP_IDEncode_UNI) {
                        wTmp = ET9_CP_LookupUnicode(pET9CPLingInfo, wTmp);
                    }
                    if (*pwBuf++ != wTmp) {
                        break;
                    }
                }
            }
            if (sPhrase.bLen == bPhraseLen) {
                *pwOffset = (ET9U16)(pbPBR - psPhraseBuf->pbBuf);
                return w;
            }
        }
        pbPBR = ET9_CP_PBR_GET_NEXT(pbPBR);
    }

    *pwOffset = (ET9U16)(pbPBR - psPhraseBuf->pbBuf);
    if (wInsert == wLimit) {
        *pwInsertOffset = *pwOffset;
    }
    return w;
}

ET9U16 ET9FARCALL ET9_CP_EncodeFreq(ET9CPLingInfo *pET9CPLingInfo,
                                    ET9SYMB * psPhrase,
                                    ET9U8 bPhraseLen,
                                    ET9U16 wFreq,
                                    ET9U8 bIsExact,
                                    ET9U8 bIsFromContext,
                                    ET9U8 bIsFromUdb,
                                    ET9UINT * pfSuppress) /* output: caller should discard this phrase */
{
    if (pfSuppress) {
        *pfSuppress = 0;
    }
    if (ET9CPIsModePhonetic(pET9CPLingInfo)
        && pET9CPLingInfo->Base.pWordSymbInfo->bNumSymbs
        && (pET9CPLingInfo->CommonInfo.SpellData.eSpellSource == ET9_CP_SpellSource_SBI || pET9CPLingInfo->CommonInfo.SpellData.eSpellSource == ET9_CP_SpellSource_Trace) )
    {
        if (pET9CPLingInfo->CommonInfo.SpellData.eSpellSource == ET9_CP_SpellSource_SBI) {
            wFreq = ET9_CP_SBI_ScorePhrase(&pET9CPLingInfo->CommonInfo.SpellData,
                &pET9CPLingInfo->CommonInfo,
                psPhrase,
                bPhraseLen,
                wFreq,
                bIsFromContext,
                bIsFromUdb,
                pfSuppress);
        }
#ifdef ET9_ALPHABETIC_MODULE
        else {
            ET9Assert(pET9CPLingInfo->CommonInfo.SpellData.eSpellSource == ET9_CP_SpellSource_Trace);

            /* score phrase based on trace spell data and chinese phrase freq */
            wFreq = ET9_CP_Trace_ScorePhrase(&pET9CPLingInfo->CommonInfo.SpellData,
                &pET9CPLingInfo->CommonInfo,
                psPhrase,
                bPhraseLen,
                wFreq,
                bIsFromContext,
                bIsFromUdb);
        }
#endif
    }
    else {
        if (bIsFromUdb) {
            wFreq = (ET9U16)(wFreq >> 3);
            wFreq |= ET9_CP_FREQ_MASK_UDB;
        }
        if (bIsExact) {
            wFreq |= ET9_CP_FREQ_MASK_EXACT;
        }
        if (bIsFromContext) {
            wFreq |= ET9_CP_FREQ_MASK_CONTEXT;
        }
    
        /* demote long phrases when input has only 1 or 2 syllables */
        {
            ET9UINT nSylCount = pET9CPLingInfo->CommonInfo.bSylCount;
            if (0 < nSylCount && nSylCount <= 2 && bPhraseLen > nSylCount) {
                wFreq = 0;
            }
        }
    }

    return wFreq;
}

/* return 0 means the phrase is added, 1 means the phrase is duplicate or illegal */
ET9UINT ET9FARCALL ET9_CP_AddPhraseToBuf(ET9CPLingInfo *pET9CPLingInfo,
                                         ET9_CP_PhraseBuf *psPhraseBuf,
                                         const ET9SYMB *pwPhrase,
                                         ET9U8 bPhraseLen,
                                         const ET9U8 *pbSpell,
                                         ET9U8 bSpellLen,
                                         ET9_CP_IDEncode eEnc,
                                         ET9U16 wFreq)
{
    int  fPageFull = 0;
    ET9U16 wStart;  /* the index of the starting phrase of the page */
    ET9U8 *pbInsert;

    ET9U8 bPhraseBytes = (ET9U8)(sizeof(ET9SYMB) * bPhraseLen);
    ET9U8 bSpellBytes = bSpellLen;
    ET9U8 bNewRecordBytes = (ET9U8)(ET9_CP_PBR_HEADER_SIZE + bPhraseBytes + bSpellBytes);

    ET9Assert(ET9_CP_PBR_HEADER_SIZE + bPhraseBytes + bSpellBytes < 0x100);
    ET9Assert(!ET9_CP_IS_LINGINFO_NOINIT(pET9CPLingInfo));
    ET9Assert(psPhraseBuf && pwPhrase);
    ET9Assert(bPhraseLen <= ET9CPMAXPHRASESIZE);

    /* overwrite cleared phrasebuf */
    if (psPhraseBuf->bTestClear) {
        psPhraseBuf->bTestClear = 0;
        psPhraseBuf->wTotal = 0;
        psPhraseBuf->wLastTotal = 0;
        psPhraseBuf->wPrevFreq = 0xFFFF;
        psPhraseBuf->wUsedBufSize = 0;
    }

    /* check phrase capacity is enough */
    if (ET9_CP_PB_FREE_BYTES(psPhraseBuf) < bNewRecordBytes) {
        return 1;
    }
    /* the phrase has been captured in previous pages */
    /* if (wFreq <= psPhraseBuf->wPrevFreq), it doesn't guarentee the phrase isn't a duplicate of the previous page. */
    if (wFreq > psPhraseBuf->wPrevFreq) {
        return 1;
    }
    if (ET9_CP_PhraseBufPageFillDone(psPhraseBuf)) {
        fPageFull = 1;
        /* the current page is full and the entries added later need to be filtered and sorted */
        if (wFreq <= psPhraseBuf->pwFreq[psPhraseBuf->bPageSize - 1]) {
            return 0;
        }
        wStart = (ET9U16)(psPhraseBuf->wTotal - psPhraseBuf->bPageSize);
    } else {
        wStart = (ET9U16)(psPhraseBuf->wTotal - (psPhraseBuf->wTotal % psPhraseBuf->bPageSize));
    }

    {
        ET9U8 *pbDup;
        ET9U8 bDupRecordBytes;
        ET9U16 wInsert, wDup, wInsertOffset;
        ET9U16 wDupOffset; /* where the duplicate entry starts in the buffer */

        wInsert = ET9_CP_SearchSeq(psPhraseBuf->pwFreq, (ET9U16)(psPhraseBuf->wTotal - wStart), wFreq);
        wInsert = (ET9U16)(wInsert + wStart); /* adjust wInsert from current page to the whole buffer */

        wDup = ET9_CP_LookForDupPhrase(pET9CPLingInfo, pwPhrase, bPhraseLen, eEnc, psPhraseBuf, fPageFull, wInsert, &wInsertOffset, &wDupOffset);

        if (wDup < wInsert) {
            /* duplicate entry is before the insertion point, need not do anything */
            return 1;
        }

        pbInsert = psPhraseBuf->pbBuf + wInsertOffset;
        pbDup = psPhraseBuf->pbBuf + wDupOffset;

        /* if page not full and there is no duplicate, */
        /* we're not replacing, we're adding */
        if (!fPageFull && (wDup == psPhraseBuf->wTotal)) {
            psPhraseBuf->wTotal++;
            bDupRecordBytes = 0;
        }
        else {
            bDupRecordBytes = ET9_CP_PBR_GET_RECORD_LENGTH(pbDup);

            /* the duplicate fits but the new record doesn't */
            if (ET9_CP_PB_FREE_BYTES(psPhraseBuf) < bNewRecordBytes - bDupRecordBytes) {
                return 1;
            }
        }

        /* make room for the new record and squeeze out gap left by dup */
        if (bNewRecordBytes <= bDupRecordBytes) {
            ET9_CP_MoveBlockForward(
                pbInsert, 
                (ET9U16)(wDupOffset - wInsertOffset), 
                (ET9U16)bNewRecordBytes);
            ET9_CP_MoveBlockBackward(
                pbDup + bDupRecordBytes,
                (ET9U16)(psPhraseBuf->wUsedBufSize - (wDupOffset + bDupRecordBytes)),
                (ET9U16)(bDupRecordBytes - bNewRecordBytes));
        }
        else {
            ET9_CP_MoveBlockForward(
                pbDup + bDupRecordBytes,
                (ET9U16)(psPhraseBuf->wUsedBufSize - (wDupOffset + bDupRecordBytes)),
                (ET9U16)(bNewRecordBytes - bDupRecordBytes));
            ET9_CP_MoveBlockForward(
                pbInsert,
                (ET9U16)(wDupOffset - wInsertOffset),
                (ET9U16)bNewRecordBytes);
        }

        /* update phrasebuf length */
        psPhraseBuf->wUsedBufSize = (ET9U16)(psPhraseBuf->wUsedBufSize + bNewRecordBytes - bDupRecordBytes);
        
        /* update frequencies */
        {
            if (fPageFull && (wDup == psPhraseBuf->wTotal))
                wDup--;
            while(wInsert < wDup) {
                psPhraseBuf->pwFreq[wDup - wStart] = psPhraseBuf->pwFreq[wDup - wStart - 1];
                wDup--;
            }
            psPhraseBuf->pwFreq[wInsert - wStart] = wFreq;
        }

        /* copying into the buffer */
        {
            ET9U8 bSpellOffset;

            /* write record size */
            pbInsert[ET9_CP_PBR_RECORD_LENGTH_OFFSET] = bNewRecordBytes;

            /* write phrase */
            pbInsert[ET9_CP_PBR_PHRASE_LENGTH_OFFSET] = bPhraseLen;
            _ET9ByteCopy(pbInsert + ET9_CP_PBR_PHRASE_OFFSET, (ET9U8*)pwPhrase, bPhraseBytes);

            /* write spell */
            pbInsert[ET9_CP_PBR_SPELL_LENGTH_OFFSET] = bSpellBytes;
            bSpellOffset = (ET9U8)(ET9_CP_PBR_PHRASE_OFFSET + bPhraseBytes);
            if (pbSpell) {
                _ET9ByteCopy(pbInsert + bSpellOffset, pbSpell, bSpellBytes);
            }
        }
    }
    ET9Assert(DebugCheckPhraseBuf(psPhraseBuf));
    return 0;
}

/* Search the char-punct bigram table and add matching puncts to the phrase buffer
 * as context matches */
void ET9FARCALL ET9_CP_GetSmartPuncts(ET9CPLingInfo *pET9CPLingInfo)
{
    ET9_CP_CommonInfo *pCommon;
    ET9U32 dwReadOffset;
    ET9_CP_PhraseBuf *pMainPhraseBuf = ET9_CP_GetMainPhraseBuf(pET9CPLingInfo);
    ET9U16 wNumEntries, wEntryIndex, wID, wLastCharID;
    ET9_CP_IDEncode eEnc = ET9_CP_IDEncode_PID;
    ET9U8 bNumContextChars, b;

    pCommon = &(pET9CPLingInfo->CommonInfo);

    if (!ET9CPIsSmartPunctActive(pET9CPLingInfo)) {
        return;
    }

    if (0 == pCommon->pbContextLen[0]) {
        return; /* no context to match */
    }

    if (!pCommon->sOffsets.dwSmartPunctOffset) { /* the table is not available */
        return;
    }

    /* get last char selected */
    for(b = 0, bNumContextChars = 0; pCommon->pbContextLen[b]; b++) {
        bNumContextChars = (ET9U8)(bNumContextChars + pCommon->pbContextLen[b]);
    }
    wLastCharID = pCommon->pwContextBuf[bNumContextChars - 1];

#ifndef ET9CP_DISABLE_STROKE
    if (ET9CPIsModeStroke(pET9CPLingInfo) || ET9CPIsModeCangJie(pET9CPLingInfo) || ET9CPIsModeQuickCangJie(pET9CPLingInfo)) {
        ET9U8 bAltCount;
        eEnc = ET9_CP_IDEncode_SID;
        /* smartpunct data is stored in PID, so we need to convert before searching for a match */
        /* caution, assume the smartpuncts are listed under the base char's most common PID */
        bAltCount = ET9_CP_LookupID(pET9CPLingInfo, &wLastCharID, wLastCharID, 1, ET9_CP_Lookup_SIDToPID);
        ET9Assert(bAltCount > 0);
    }
#endif
    /* read char count in the smart punct table */
    dwReadOffset = pCommon->sOffsets.dwSmartPunctOffset;
    wNumEntries = ET9_CP_LdbReadWord(pET9CPLingInfo, dwReadOffset);
    dwReadOffset += 2; /* sizeof(ET9U16) */
    for (wEntryIndex = 0; wEntryIndex < wNumEntries; wEntryIndex++) {
        wID = ET9_CP_LdbReadWord(pET9CPLingInfo, dwReadOffset); /* read char PID */
        dwReadOffset += 2; /* sizeof(ET9U16) */
        if (wLastCharID == wID) {
            break;
        }
    }

    if (wEntryIndex < wNumEntries) { /* found match */
        ET9U16 wPackedPuncts;
        ET9U16 wTailTableOffset = (ET9U16)(ET9_CP_SMART_PUNCT_HEADER_SIZE + wNumEntries * ET9_CP_SMART_PUNCT_ID_ENTRY_SIZE);
        ET9U8 bPunctCount = ET9_CP_MAX_SMART_PUNCTS_PER_CHAR;
        ET9U8 bFreq;

        /* set offset to punct section */
        dwReadOffset = pCommon->sOffsets.dwSmartPunctOffset + wTailTableOffset + ET9_CP_SMART_PUNCT_TAIL_ENTRY_SIZE * wEntryIndex;
        wPackedPuncts = ET9_CP_LdbReadWord(pET9CPLingInfo, dwReadOffset); /* read punct PIDs */
        dwReadOffset += sizeof(ET9U16);

        while (bPunctCount --) {
            ET9U8 bPunctOffset = (ET9U8)(ET9_CP_SMART_PUNCT_MASK &
                (wPackedPuncts >> (ET9_CP_BITS_PER_SMART_PUNCT * bPunctCount)));
            ET9U16 wPunctID;
            ET9U16 wFreq;

            if (ET9_CP_SMART_PUNCT_MASK == bPunctOffset) {
                break; /* found entry delim, no more puncts */
            }
            bFreq = ET9_CP_LdbReadByte(pET9CPLingInfo, dwReadOffset);
            dwReadOffset++;

            wPunctID = (ET9U16)(pCommon->w1stSymbolPID + bPunctOffset);

#ifndef ET9CP_DISABLE_STROKE
            if (ET9_CP_IDEncode_SID == eEnc) {
                ET9U8 bAltCount;
                bAltCount = ET9_CP_LookupID(pET9CPLingInfo, &wPunctID, wPunctID, 1, ET9_CP_Lookup_PIDToSID);
                ET9Assert(bAltCount > 0);
            }
#endif
            wFreq = ET9_CP_EncodeFreq(pET9CPLingInfo, NULL, 1, (ET9U16)bFreq, 0, 1, 0, NULL);
            ET9_CP_AddPhraseToBuf(pET9CPLingInfo, pMainPhraseBuf,
                                  (ET9SYMB *)&wPunctID, 1, NULL, 0, eEnc, wFreq);
        }
    }
} /* END ET9CP_GetSmartPunct() */


/*---------------------------------------------------------------------------
 *
 *   Function: ET9_CP_GetCommonChar
 *
 *   Synopsis: This function retrieves common characters from LDB and stores them
 *             into phrase buffer. If there is non-punct context, a different table
 *             will be loaded.
 *
 *     Input:  pET9CPLingInfo    = Pointer to Chinese XT9 LingInfo structure.
 *
 *     Return: none
 *
 *---------------------------------------------------------------------------*/
void ET9FARCALL ET9_CP_GetCommonChar(ET9CPLingInfo *pET9CPLingInfo)
{
    ET9_CP_PhraseBuf *pMainPhraseBuf = ET9_CP_GetMainPhraseBuf(pET9CPLingInfo);
    ET9_CP_CommonInfo * pCommon = &(pET9CPLingInfo->CommonInfo);
    ET9U32 dwReadOffset;
    ET9U16 wCount, wID;
    ET9_CP_IDEncode eEnc = ET9_CP_IDEncode_PID;

    if (!ET9CPIsCommonCharActive(pET9CPLingInfo)) {
        return;
    }

    if (0 == pCommon->pbContextLen[0]) {
        dwReadOffset = pCommon->sOffsets.dwNoContextCommonCharOffset;
    }
    else {
        dwReadOffset = pCommon->sOffsets.dwContextCommonCharOffset;
    }

    if (!dwReadOffset) { /* the table is not available */
        return;
    }

#ifndef ET9CP_DISABLE_STROKE
    if (ET9CPIsModeStroke(pET9CPLingInfo) || ET9CPIsModeCangJie(pET9CPLingInfo) || ET9CPIsModeQuickCangJie(pET9CPLingInfo)) {
        eEnc = ET9_CP_IDEncode_SID;
    }
#endif

    /* read char count in the common char table */
    wCount = ET9_CP_LdbReadWord(pET9CPLingInfo, dwReadOffset);
    dwReadOffset += 2; /* sizeof(ET9U16) */
    for (; wCount > 0; wCount--) {
        wID = ET9_CP_LdbReadWord(pET9CPLingInfo, dwReadOffset); /* read PID */
        dwReadOffset += 2; /* sizeof(ET9U16) */

        if (!ET9CPIsSmartPunctActive(pET9CPLingInfo) && /* skip punct if smart punct is off */
            ET9_CP_IS_SYMBOL_PID(pCommon, wID)) {
            continue;
        }
#ifndef ET9CP_DISABLE_STROKE
        if (ET9_CP_IDEncode_SID == eEnc) {
            ET9U8 bAltCount;
            ET9Assert(ET9CPIsModeStroke(pET9CPLingInfo) || ET9CPIsModeCangJie(pET9CPLingInfo) || ET9CPIsModeQuickCangJie(pET9CPLingInfo));
            bAltCount = ET9_CP_LookupID(pET9CPLingInfo, &wID, wID, 1, ET9_CP_Lookup_PIDToSID);
            ET9Assert(bAltCount > 0);
        }
        else if (ET9_CP_IDEncode_UNI == eEnc) {
            wID = ET9_CP_LookupUnicode(pET9CPLingInfo, wID);
        }
#endif
        ET9_CP_AddPhraseToBuf(pET9CPLingInfo, pMainPhraseBuf,
            (ET9SYMB *)&wID, 1, NULL, 0, eEnc, wCount); /* use wCount to give a decreasing frequency */
        if (ET9_CP_PhraseBufPageFillDone(pMainPhraseBuf)) {
            return; /* the current page is full, done -- only for dereasing freq order */
        }
    }
} /* ET9_CP_GetCommonChar */


#ifndef ET9CP_DISABLE_STROKE
/* This function is called to expand the delimiter at the end of the input.  If the last user
   input is not a delimiter the function does nother, otherwise this function adds
   one character to the matching criteria (pwRange and pbRangeStart) so that the search
   will include one more character
   return  ET9UINT   1 if expanded on delimiter, 0 if not
   */
ET9UINT ET9FARCALL ET9_CP_ExpandDelimiter(ET9CPLingInfo *pET9CPLingInfo)
{
    ET9_CP_CommonInfo *pComm = &pET9CPLingInfo->CommonInfo;
    ET9_CP_PhraseBuf *pMainPhraseBuf = ET9_CP_GetMainPhraseBuf(pET9CPLingInfo);
    ET9U8 bLastRangeEnd;

    ET9Assert( ET9CPIsModeStroke(pET9CPLingInfo) ); /* Only Stroke mode can do delimiter expansion */
    ET9Assert(pComm->bSylCount > 0);
    ET9Assert(pComm->bKeyBufLen > 0);

    if (pComm->bKeyBuf[pComm->bKeyBufLen - 1] != ET9CPSYLLABLEDELIMITER) {
        pMainPhraseBuf->bIsDelimiterExpansion = 0;
        return 0;
    }
    bLastRangeEnd = pComm->pbRangeEnd[pComm->bSylCount - 1];
    /* test if there is enough space for added character */
    if ((pComm->bSylCount >= ET9CPMAXPHRASESIZE) ||
        (bLastRangeEnd + ET9_CP_ID_RANGE_SIZE >= ET9_CP_MAX_ID_RANGE_SIZE))
    {
        pMainPhraseBuf->bIsDelimiterExpansion = 0;
        return 0;
    }
    /* now adding the last (exactStart, exactEnd, partialEnd) to the ranges */
    pComm->pbRangeEnd[pComm->bSylCount++] = (ET9U8)(bLastRangeEnd + 3);
    pComm->pwRange[bLastRangeEnd++] = 0;
    pComm->pwRange[bLastRangeEnd++] = 0;
    pComm->pwRange[bLastRangeEnd] = pET9CPLingInfo->Private.SPrivate.wCurTotalMatch[0];
    pMainPhraseBuf->bIsDelimiterExpansion = 1;
    return 1;
} /* END ET9_CP_ExpandDelimiter() */
#endif

/* wPhraseIndex is 0-based */
ET9STATUS ET9FARCALL ET9_CP_AdjustPointerFillPhraseBuf(ET9CPLingInfo *pET9CPLingInfo, ET9U16 wPhraseIndex, ET9_CP_PHRASE_SEARCH_CALLBACK FctFill)
{
    ET9_CP_PhraseBuf *pMainPhraseBuf = ET9_CP_GetMainPhraseBuf(pET9CPLingInfo);
    ET9Assert( FctFill );

    /* fill the buffer if empty */
    if (ET9_CP_IsPhraseBufEmpty(pMainPhraseBuf)) {
        FctFill(pET9CPLingInfo);
        /* we test if a stroke is valid by trying to fill phrase buffer.  If it is empty
        * the stroke is invalid */
        if (ET9_CP_IsPhraseBufEmpty(pMainPhraseBuf)) {
            return ET9STATUS_INVALID_INPUT;
        }
    }

    /* if chinese we may need to fill another page */
    if ((ET9_CP_ContextFillPhraseBuffer == FctFill
        || ET9_CP_PrefixFillPhraseBuffer == FctFill
        || ET9_CP_JianpinFillPhraseBuffer == FctFill
#ifdef ET9_ALPHABETIC_MODULE
        || ET9_CP_Trace_FillPhraseBuffer == FctFill
#endif
#ifndef ET9CP_DISABLE_STROKE
        || ET9_CP_StrokeFillPhraseBuffer == FctFill
        || ET9_CP_CangJieFillPhraseBuffer == FctFill
#endif
        )
        && wPhraseIndex >= pMainPhraseBuf->wTotal
        && (pMainPhraseBuf->wTotal % pMainPhraseBuf->bPageSize) == 0) {
        FctFill(pET9CPLingInfo);
    }
    if (wPhraseIndex >= pMainPhraseBuf->wTotal) {
        return ET9STATUS_OUT_OF_RANGE;
    }
    ET9Assert(pMainPhraseBuf->wLastTotal == pMainPhraseBuf->wTotal);

    /* remember the frequency of the last entry */
    ET9Assert(!ET9_CP_IsPhraseBufEmpty(pMainPhraseBuf));
    pMainPhraseBuf->wPrevFreq = pMainPhraseBuf->pwFreq[(pMainPhraseBuf->wTotal - 1) % pMainPhraseBuf->bPageSize];

    return ET9STATUS_NONE;
}


void ET9FARCALL ET9_CP_ConvertPhraseToUnicode(ET9CPLingInfo *pET9CPLingInfo, ET9CPPhrase *psPhrase, ET9_CP_IDEncode eEnc)
{
    ET9U16 wPID;
    ET9U8 b;

    switch (eEnc)
    {
    case ET9_CP_IDEncode_SID:
        for(b = 0; b < psPhrase->bLen; b++) {
            ET9U8 bAltCount;
            bAltCount = ET9_CP_LookupID(pET9CPLingInfo, &wPID, psPhrase->pSymbs[b], 1, ET9_CP_Lookup_SIDToPID);
            ET9Assert(bAltCount > 0);
            psPhrase->pSymbs[b] = ET9_CP_LookupUnicode(pET9CPLingInfo, wPID);
        }
        break;
    case ET9_CP_IDEncode_PID: /* PID and BID go through the same route */
    case ET9_CP_IDEncode_BID:
        for (b = 0; b < psPhrase->bLen; b++) {
            psPhrase->pSymbs[b] = ET9_CP_LookupUnicode(pET9CPLingInfo, psPhrase->pSymbs[b]);
        }
        break;
    case ET9_CP_IDEncode_UNI:
    default:
        break;
    }
}

/** Retrieves the number of possible prefixes.
 *
 * See ET9CPSetActivePrefix() for the concept of prefix.<br>
 *
 * @param pET9CPLingInfo            pointer to chinese information structure
 *
 * @return   The number of prefixes. 0 if not in phonetic mode or a tone is entered.
 */
ET9U8 ET9FARCALL ET9CPGetPrefixCount(const ET9CPLingInfo *pET9CPLingInfo)
{
    if ( ET9_CP_IS_LINGINFO_NOINIT(pET9CPLingInfo) ||
         ET9_CP_IsUdbChangedByOtherThread(pET9CPLingInfo) ||
         ET9_CP_IS_BUILD_OUT_OF_DATE(pET9CPLingInfo))
    {
        return 0;
    }

    if ( !ET9CPIsModePhonetic(pET9CPLingInfo) ) {
        return 0;
    }

#ifdef ET9_ALPHABETIC_MODULE
    if (ET9_CP_InputContainsTrace(pET9CPLingInfo)) {
        return ET9_CP_Trace_GetPrefixCount(&pET9CPLingInfo->Trace);
    }
#endif

    return pET9CPLingInfo->CommonInfo.bSyllablePrefixCount;
}

static ET9STATUS ET9LOCALCALL GetPhrasalPrefix(const ET9CPLingInfo *pET9CPLingInfo, ET9S32 i, ET9_CS_Prefix* pPrefix)
{
    ET9STATUS status;
    ET9U8 index;
    ET9_CP_CHECK_LINGINFO(pET9CPLingInfo);
    index = pET9CPLingInfo->CommonInfo.pPrefixGroup[i].bStartIndex;
    status = ET9_CS_GetPrefix( &pET9CPLingInfo->SBI, index, pPrefix );
    return status;
}


/** Retrieve the prefix specified by index
 *
 * See ET9CPSetActivePrefix() for the concept of prefix.<br>
 *
 * @param pET9CPLingInfo            pointer to chinese information structure
 * @param wIndex                    0-based index
 * @param psSpell                   A spelling data structure where XT9 Chinese will write the prefix and its length.
 *
 * @return ET9STATUS_NONE               Success
 * @return ET9STATUS_NO_INIT            pET9CPLingInfo is not properly initialized
 * @return ET9STATUS_NEED_SELLIST_BUILD Should call ET9CPBuildSelectionList() before calling this function
 * @return ET9STATUS_BAD_PARAM          Some argument pointer is NULL
 * @return ET9STATUS_INVALID_MODE       Not in phonetic mode
 * @return ET9STATUS_OUT_OF_RANGE       Index is too big
 */
ET9STATUS ET9FARCALL ET9CPGetPrefix(const ET9CPLingInfo *pET9CPLingInfo, ET9U16 wIndex, ET9CPSpell * psSpell)
{
    ET9STATUS status;
    ET9UINT i;
    ET9_CS_Prefix sPrefix;

    ET9_CP_CHECK_LINGINFO(pET9CPLingInfo);
    ET9_CP_CHECK_UDB_UP_TO_DATE(pET9CPLingInfo);
    ET9_CP_CHECK_BUILD_UP_TO_DATE(pET9CPLingInfo);

    if (NULL == psSpell) {
        return ET9STATUS_BAD_PARAM;
    }
    if ( !ET9CPIsModePhonetic(pET9CPLingInfo) ) {
        return ET9STATUS_INVALID_MODE;
    }
    if (wIndex >= ET9CPGetPrefixCount(pET9CPLingInfo)) {
        return ET9STATUS_OUT_OF_RANGE;
    }

#ifdef ET9_ALPHABETIC_MODULE
    if (ET9_CP_InputContainsTrace(pET9CPLingInfo)) {
        status = ET9_CP_Trace_GetPrefix(&pET9CPLingInfo->Trace, wIndex, psSpell);
    }
    else
#endif
    {
        status = GetPhrasalPrefix(pET9CPLingInfo, wIndex, &sPrefix);
        if (status != ET9STATUS_NONE) {
            return status;
        }

        for(i = 0; i < sPrefix.m_bPfxLen; i++) {
            psSpell->pSymbs[i] = sPrefix.m_pcPfx[i];
        }
        if ( ET9CPIsModeBpmf(pET9CPLingInfo) )
        {
            for(i = 0; i < sPrefix.m_bPfxLen; i++) {
                psSpell->pSymbs[i] = ET9_CP_BpmfInternalToExternal(psSpell->pSymbs[i]);
            }
        }
        psSpell->bLen = sPrefix.m_bPfxLen;

        status = ET9STATUS_NONE;
    }

    return status;
}

/** Retrieve the active prefix index value.
 * See ET9CPSetActivePrefix() for the concept of prefix.<br>
 *
 * @param pET9CPLingInfo     pointer to chinese information structure.
 * @param pbPrefixIndex      (out)the active prefix index.
 *
 * @return ET9STATUS_NO_INIT   - pET9CPLingInfo is null or Chinese XT9 is not initialized successfully
 * @return ET9STATUS_NEED_SELLIST_BUILD - Need call ET9CPBuildSelectionList() before calling this
 * @return ET9STATUS_INVALID_MODE - Current input mode does not support prefix
 * @return ET9STATUS_BAD_PARAM - pbSpellIndex is NULL
 * @return ET9STATUS_ERROR     - Currently there is no prefix
 * @return ET9STATUS_EMPTY     - No prefix is set
 * @return ET9STATUS_NONE      - success
 */
ET9STATUS ET9FARCALL ET9CPGetActivePrefixIndex(ET9CPLingInfo *pET9CPLingInfo, ET9U8 *pbPrefixIndex)
{
    ET9STATUS status;

    ET9_CP_CHECK_LINGINFO(pET9CPLingInfo);
    ET9_CP_CHECK_UDB_UP_TO_DATE(pET9CPLingInfo);
    ET9_CP_CHECK_BUILD_UP_TO_DATE(pET9CPLingInfo);

    if ( !ET9CPIsModePhonetic(pET9CPLingInfo) ) {
        return ET9STATUS_INVALID_MODE;
    }
    if (!pbPrefixIndex) {
        return ET9STATUS_BAD_PARAM;
    }
    if (ET9CPGetPrefixCount(pET9CPLingInfo) == 0) {
        return ET9STATUS_ERROR;
    }

#ifdef ET9_ALPHABETIC_MODULE
    if (ET9_CP_InputContainsTrace(pET9CPLingInfo)) {
        status = ET9_CP_Trace_GetActivePrefixIndex(&pET9CPLingInfo->Trace, pbPrefixIndex);
    }
    else 
#endif
    {
        if (0xFF == pET9CPLingInfo->CommonInfo.bActivePrefix) {
            return ET9STATUS_EMPTY;
        }

        *pbPrefixIndex = pET9CPLingInfo->CommonInfo.bActivePrefix;
        status = ET9STATUS_NONE;
    }

    return status;
}

/** Clear any active prefix so that none of the prefix is active.
 * See ET9CPSetActivePrefix() for the concept of prefix.<br>
 *
 * @param pET9CPLingInfo     pointer to chinese information structure.
 *
 * @return ET9STATUS_NONE               Succeeded
 * @return ET9STATUS_NO_INIT            pET9CPLingInfo is not properly initialized
 * @return ET9STATUS_NEED_SELLIST_BUILD Should call ET9CPBuildSelectionList() before calling this function
 * @return ET9STATUS_INVALID_MODE       Not phonetic mode
 * @return ET9STATUS_NO_MATCH           After discard the prefix, no candidate can be found
 */
ET9STATUS ET9FARCALL ET9CPClearActivePrefix(ET9CPLingInfo * pET9CPLingInfo)
{
    ET9STATUS status;

    ET9_CP_CHECK_LINGINFO(pET9CPLingInfo);
    ET9_CP_CHECK_UDB_UP_TO_DATE(pET9CPLingInfo);
    ET9_CP_CHECK_BUILD_UP_TO_DATE(pET9CPLingInfo);

    if ( !ET9CPIsModePhonetic(pET9CPLingInfo) )
    {
        return ET9STATUS_INVALID_MODE;
    }
    
#ifdef ET9_ALPHABETIC_MODULE
    if (ET9_CP_InputContainsTrace(pET9CPLingInfo)) {
        status = ET9_CP_Trace_ClearActivePrefix(&pET9CPLingInfo->Trace);
    }
    else 
#endif
    {
        status = ET9_CS_SetCondition(&pET9CPLingInfo->SBI, NULL, 0, 0 );
        if (status == ET9STATUS_NO_OPERATION)
        {
            ET9_CS_PhrasalPrefix(&pET9CPLingInfo->SBI);
            ET9_CP_SortPrefixGrp(pET9CPLingInfo);
            return ET9STATUS_NONE;
        }
        /* we should always reset SpellBuf, PhraseBuf, PrefixBuf
         no matter status == ET9STATUS_NONE or not, since status != ET9STATUS_NONE might be cause by eNoMatch*/
        if ( status == ET9STATUS_NO_MATCH || status == ET9STATUS_NONE )
        {
            ET9_CP_SegmentationToSpell(pET9CPLingInfo, &pET9CPLingInfo->CommonInfo.sActiveSpell);
            ET9_CP_ClearPhraseBuf(ET9_CP_GetMainPhraseBuf(pET9CPLingInfo));
            ET9_CS_PhrasalPrefix(&pET9CPLingInfo->SBI);
            ET9_CP_SortPrefixGrp(pET9CPLingInfo);
        }
    }

    return status;
} /*  ET9CPClearActivePrefix() */

/** Selects one of the prefixes as the active one.
 * See also ET9CPClearActivePrefix().
 *
 * A prefix is a possible leading syllable of a spelling.<br>
 * XT9 Chinese core internally maintains a prefix buffer to hold all the possible prefixes.
 * After ET9CPBuildSelectionList(), integration layer can use the following function to access the prefixes:
 * - ET9CPGetPrefix()
 * - ET9CPGetPrefixCount()
 * - ET9CPGetActivePrefixIndex()
 * - ET9CPSetActivePrefix()
 * - ET9CPClearActivePrefix()
 *
 * After ET9CPBuildSelectionList(), none of the prefixes is active.
 * Integration layer can use ET9CPSetActivePrefix() to specify which prefix is active.
 * - When integration layer calls ET9CPSetActivePrefix(),
 *   - the core will change the spelling (segmentation): it will begin with the active prefix.
 *   - the core will change the phrase list: it will contain only those phrases whose first syllable is the active prefix.
 *   - the core will remember this active prefix until
 *     - ET9CPSetActivePrefix() is called to set another active prefix OR
 *     - ET9CPBuildSelectionList() is called again.
 *     - ET9CPClearActivePrefix() is called.
 *
 * @param pET9CPLingInfo     pointer to chinese information structure.
 * @param bPrefixIndex       0-based index.
 *
 * @return ET9STATUS_NONE                 Succeeded
 * @return ET9STATUS_NO_INIT              pET9CPLingInfo is not properly initialized
 * @return ET9STATUS_NEED_SELLIST_BUILD   Should call ET9CPBuildSelectionList() before call this function
 * @return ET9STATUS_INVALID_MODE         Not in phonetic mode
 * @return ET9STATUS_OUT_OF_RANGE         Index is too big
*/
ET9STATUS ET9FARCALL ET9CPSetActivePrefix(ET9CPLingInfo * pET9CPLingInfo, ET9U8 bPrefixIndex)
{
    ET9STATUS status;
    ET9_CS_Prefix pfx;

    ET9_CP_CHECK_LINGINFO(pET9CPLingInfo);
    ET9_CP_CHECK_UDB_UP_TO_DATE(pET9CPLingInfo);
    ET9_CP_CHECK_BUILD_UP_TO_DATE(pET9CPLingInfo);

    if (!ET9CPIsModePhonetic(pET9CPLingInfo)) {
        return ET9STATUS_INVALID_MODE;
    }

	if (bPrefixIndex >= ET9CPGetPrefixCount(pET9CPLingInfo))  {
		return ET9STATUS_OUT_OF_RANGE;
	}

#ifdef ET9_ALPHABETIC_MODULE
    if (ET9_CP_InputContainsTrace(pET9CPLingInfo)) {
        return ET9_CP_Trace_SetActivePrefix(&pET9CPLingInfo->Trace, bPrefixIndex);
    }
#endif

    status = GetPhrasalPrefix(pET9CPLingInfo, bPrefixIndex, &pfx);
    ET9Assert(ET9STATUS_NONE == status);
    status = ET9_CS_SetCondition(&pET9CPLingInfo->SBI, pfx.m_pcPfx, pfx.m_bPfxLen, pfx.m_iPfxProb );
    ET9Assert(ET9STATUS_NONE == status); /* ET9_CS_SetCondition should always succeed, because unviable prefixes have been rejected during build selection list */

    /* always update the segmenation and clear phrases */
    ET9_CP_SegmentationToSpell(pET9CPLingInfo, &pET9CPLingInfo->CommonInfo.sActiveSpell);
    pET9CPLingInfo->CommonInfo.bActivePrefix = (ET9U8)(bPrefixIndex);
    ET9_CP_ClearPhraseBuf(ET9_CP_GetMainPhraseBuf(pET9CPLingInfo) );

    return status; /* succeeded to set active prefix */
}  /* ET9CPSetActivePrefix() */

/* Set page size of phrase buffer = 1 to ET9CPMAXPAGESIZE */
void ET9FARCALL ET9_CP_SetPageSize(ET9_CP_PhraseBuf *pPhraseBuf, ET9UINT nNewPageSize)
{
    ET9Assert(pPhraseBuf);
    ET9Assert(nNewPageSize > 0 && nNewPageSize <= ET9CPMAXPAGESIZE); /* technical limits, which doesn't guarantee acceptable speed */
    pPhraseBuf->bPageSize = (ET9U8)nNewPageSize;
}

/* ----------------------------------< eof >--------------------------------- */

