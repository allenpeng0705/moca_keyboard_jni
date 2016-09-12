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
;**     FileName:  et9cpstrk.c                                                **
;**                                                                           **
;**  Description:  Implementation of Strike DB access function                **
;**                                                                           **
;**                                                                           **
;*******************************************************************************
;******************************************************************************/

#ifndef ET9CP_DISABLE_STROKE
#include "et9api.h"
#include "et9misc.h"

#include "et9cpkey.h"
#include "et9cpsys.h"
#include "et9cpspel.h"
#include "et9cpldb.h"
#include "et9cpsldb.h"
#include "et9cppbuf.h"

typedef struct ET9_CP_StrokeKeyseq_s {
    ET9SYMB sComp;                      /* The component unicode */
    ET9U8 bCompLen;                     /* Stroke in component */
    ET9U8 bPreCompLen;                  /* Strokes before component in WSI (but still after selection length) */
    ET9U8 abKeys[ET9_CP_MAXKEY+1];        /* Stoke key buffer */
    ET9U8 bLen;                         /* Number of strokes */
} ET9_CP_StrokeKeyseq;

static void ET9LOCALCALL MakeStrokeSpell(ET9_CP_StrokeKeyseq *psKeyseq,
                                         ET9_CP_Spell *psSpell)
{
    ET9U8 b;
    ET9U8 *pb = psSpell->pbChars;

    pb[ET9_CP_STROKE_SPELL_COMP_LEN_INDEX] = psKeyseq->bCompLen;
    ET9_CP_WriteU16(&pb[ET9_CP_STROKE_SPELL_COMP_INDEX], psKeyseq->sComp);
    pb[ET9_CP_STROKE_SPELL_PRE_COMP_LEN_INDEX] = psKeyseq->bPreCompLen;

    pb += ET9_CP_STROKE_SPELL_HEADER_LEN;
    for (b = 0; b < psKeyseq->bLen; b++) {
        pb[b] = psKeyseq->abKeys[b];
    }
    psSpell->bLen = (ET9U8)(b + ET9_CP_STROKE_SPELL_HEADER_LEN);
}

/* Trim length of stroke spell to match phrase length */
void ET9FARCALL ET9_CP_TrimStrokeSpellLength(ET9_CP_Spell   *pSpell,
                                             ET9U8          bPhraseLen)
{
    ET9U8 bDelimCount, b;

    bDelimCount = 0;
    b = (ET9U8)(ET9_CP_STROKE_SPELL_HEADER_LEN + pSpell->pbChars[ET9_CP_STROKE_SPELL_COMP_LEN_INDEX]);
    for (; b < pSpell->bLen; b++) {
        if (ET9CPSYLLABLEDELIMITER == pSpell->pbChars[b]) {
            bDelimCount++;
            if (bDelimCount >= bPhraseLen) {
                if (b + 1 == pSpell->bLen) {
                    b++; /* include delimiter at the end of input in the result spell length */
                }
                break;
            }
        }
    }
    pSpell->bLen = b;
}

/* read the contents of stroke spelling into LingInfo */
void ET9FARCALL ET9_CP_LoadStrokeSpell(ET9CPLingInfo * pET9CPLingInfo,
                                       ET9_CP_Spell *psSpell)
{
    ET9U8 *pb = psSpell->pbChars + ET9_CP_STROKE_SPELL_HEADER_LEN;
    ET9U8 *pbEnd = psSpell->pbChars + psSpell->bLen;
    ET9U8 *pbKeybuf = pET9CPLingInfo->CommonInfo.bKeyBuf;

    if (0 == psSpell->bLen) {
        pET9CPLingInfo->CommonInfo.bKeyBufLen = 0;
        pET9CPLingInfo->Private.SPrivate.wCompUnicode = 0;
        pET9CPLingInfo->Private.SPrivate.bCompNumStrokes = 0;
        return;
    }

    while (pb < pbEnd) {
        *pbKeybuf++ = *pb++;
    }

    pET9CPLingInfo->CommonInfo.bKeyBufLen = (ET9U8)(pbKeybuf - pET9CPLingInfo->CommonInfo.bKeyBuf);

    {
        ET9U16 wComp;

        wComp = ET9_CP_ReadU16(&psSpell->pbChars[ET9_CP_STROKE_SPELL_COMP_INDEX]);
        pET9CPLingInfo->Private.SPrivate.wCompUnicode = wComp;
    }
    pET9CPLingInfo->Private.SPrivate.bCompNumStrokes = psSpell->pbChars[ET9_CP_STROKE_SPELL_COMP_LEN_INDEX];
}

void ET9FARCALL ET9_CP_LoadStrokeKeyseq(ET9CPLingInfo * pET9CPLingInfo,
                                       ET9_CP_StrokeKeyseq *psKeyseq)
{
    _ET9ByteCopy(pET9CPLingInfo->CommonInfo.bKeyBuf, psKeyseq->abKeys, psKeyseq->bLen);
    pET9CPLingInfo->CommonInfo.bKeyBufLen = psKeyseq->bLen;

    pET9CPLingInfo->Private.SPrivate.wCompUnicode = psKeyseq->sComp;
    pET9CPLingInfo->Private.SPrivate.bCompNumStrokes = psKeyseq->bCompLen;
}

/* convert WSI to buffer of strokes */
static ET9STATUS ET9LOCALCALL WSIToStrokeKeybuf(ET9CPLingInfo * pET9CPLingInfo,
                                                ET9WordSymbInfo * pWordSymbInfo,
                                                ET9U8 * pbKeys, /* buffer to hold strokes, must be at least ET9_CP_MAX_STROKE_PER_CHAR long */
                                                ET9U8 * pbKeyBufLen, /* (output) number of strokes */
                                                ET9SYMB * psComp, /* last component found (if any) */
                                                ET9U8 * pbPreCompLen) /* symbols before component in WSI (they are ignored) */
{
    ET9STATUS status = ET9STATUS_NONE;
    ET9U8 bSymbInfoIndex;
    ET9U8 bLen = 0;

    ET9Assert(ET9CPIsModeStroke(pET9CPLingInfo));
    ET9Assert(pWordSymbInfo->bNumSymbs <= ET9_CP_MAXKEY);
    ET9Assert(pWordSymbInfo->bNumSymbs > 0);

    *pbPreCompLen = 0;
    *pbKeyBufLen = 0;
    *psComp = 0;

    /* scan for last component */
    bSymbInfoIndex = pWordSymbInfo->bNumSymbs;
    do {
        bSymbInfoIndex--;

        if (ET9CPIsComponent(pET9CPLingInfo, pWordSymbInfo->SymbsInfo[bSymbInfoIndex].DataPerBaseSym[0].sChar[0])) {
            pbKeys[bLen++] = ET9_CP_COMPONENT;
            *psComp = pWordSymbInfo->SymbsInfo[bSymbInfoIndex].DataPerBaseSym[0].sChar[0];
            *pbPreCompLen = (ET9U8)(bSymbInfoIndex - ET9_CP_SelectionHistUnselectedStart(&pET9CPLingInfo->SelHistory));
            bSymbInfoIndex++; /* skip the component symbinfo */
            break;
        }
    }
    while (bSymbInfoIndex > ET9_CP_SelectionHistUnselectedStart(&pET9CPLingInfo->SelHistory));

    /* for each symbinfo on WSI */
    for(; bSymbInfoIndex < pWordSymbInfo->bNumSymbs; bSymbInfoIndex++) {
        ET9SYMB symb;
        ET9U8 bSymbIndex;
        ET9U8 bNumSymbsOnKey = pWordSymbInfo->SymbsInfo[bSymbInfoIndex].DataPerBaseSym[0].bNumSymsToMatch;

        ET9Assert(0 < bNumSymbsOnKey);

        /* search the SymbInfo for a stroke */
        for (bSymbIndex = 0; bSymbIndex < bNumSymbsOnKey; bSymbIndex++) {
            symb = pWordSymbInfo->SymbsInfo[bSymbInfoIndex].DataPerBaseSym[0].sChar[bSymbIndex];

            if (ET9CPIsStrokeSymbol(symb) ||
                ET9CPSYLLABLEDELIMITER == symb)
            {
                pbKeys[bLen] = (ET9U8)symb;
                break;
            }
        }
        if (bSymbIndex == bNumSymbsOnKey) {
            return ET9STATUS_INVALID_INPUT;
        }

        bLen++;
    }

    ET9Assert(bLen <= ET9_CP_MAXKEY);
    *pbKeyBufLen = bLen;
    return status;
}


/*  This function checks 1, if there is at least one non-wild card stroke for the first character
 *  and 2, at least one stroke for trailing characters.
 *  and 3, wild card is allowed only in the first character,
 *  and 4, every key is a "Sroke key" -- 1 - 6, delimiter, first key allow ET9_CP_COMPONENT,
 *  eg, "??#" is invalid, "?2?#1" is valid, "?2?##" invalid, 1#?3 invalid
 *  return 1 for valid, 0 for invlaid */
static ET9UINT ET9LOCALCALL IsStrokeKeyseqValid(ET9U8 * pbKeys, ET9U8 bKeyBufLen)
{
    ET9U8 b;
    ET9UINT fFoundAStroke = 0, nCharIndex = 0;

    for (b = 0; b < bKeyBufLen; b++) {
        if (ET9CPSYLLABLEDELIMITER == pbKeys[b]) {
            if (!fFoundAStroke) {
                return 0;
            }
            fFoundAStroke = 0;
            nCharIndex++;
        }
        else if (!(b == 0 && ET9_CP_COMPONENT == pbKeys[b]) && !ET9CPIsStrokeSymbol(pbKeys[b])) {
            return 0;   /* It's not a ET9_CP_COMPONENT NOR a Stroke key */
        }
        else if (pbKeys[b] != ET9CPSTROKEWILDCARD) {
            fFoundAStroke = 1;
        }
        else if (nCharIndex > 0) {
            return 0;               /* Wild card only allow in the first char */
        }
    }
    return 1;
}


/* expand comp marker in buffer, read component data into PPrivate */
static ET9STATUS ET9LOCALCALL ReadExpandComp(ET9CPLingInfo * pET9CPLingInfo,
                                             ET9SYMB sCompSymb,
                                             ET9_CP_StrokeKeyseq * psKeyseq)
{
    ET9U8 bLen = psKeyseq->bLen;
    ET9Assert(0 < bLen);

    psKeyseq->sComp = 0;
    psKeyseq->bCompLen = 0;

    if (ET9_CP_COMPONENT == psKeyseq->abKeys[0]) {
        ET9U8 abCompStrokes[ET9_CP_MAX_STROKE_PER_CHAR];
        ET9U8 bCompLen, bAltCount;

        ET9SYMB sPID, sSID;

        ET9Assert(ET9CPIsComponent(pET9CPLingInfo, sCompSymb));

        psKeyseq->sComp = sCompSymb;

        /* unicode to SID */
        sPID = (ET9SYMB)(sCompSymb
                           - pET9CPLingInfo->CommonInfo.wComponentFirst
                           + pET9CPLingInfo->CommonInfo.w1stComponentPID);
        bAltCount = ET9_CP_LookupID(pET9CPLingInfo, &sSID, sPID, 1, ET9_CP_Lookup_PIDToSID);
        ET9Assert(bAltCount > 0);
        /* copy component into temporary buffer */
        bCompLen = ET9_CP_StrokeLookup(pET9CPLingInfo, sSID, abCompStrokes, ET9_CP_MAX_STROKE_PER_CHAR);
        psKeyseq->bCompLen = bCompLen;

        if (bLen - 1 + bCompLen > ET9_CP_MAXKEY) {
            return ET9STATUS_FULL; /* guard against psKeyseq->abKeys buffer overflow */
        }
        /* move comp into dst buffer */
        ET9_CP_ByteMove(&psKeyseq->abKeys[bCompLen], &psKeyseq->abKeys[1], bLen-1);
        _ET9ByteCopy(psKeyseq->abKeys, abCompStrokes, bCompLen);
        psKeyseq->bLen = (ET9U8)(bLen - 1 + bCompLen);
    }
    return ET9STATUS_NONE;
}


/* read WSI data into ET9_CP_StrokeKeyseq */
static ET9STATUS ET9LOCALCALL StrokeWSIToKeySeq(ET9CPLingInfo * pET9CPLingInfo,
                              ET9WordSymbInfo * pWordSymbInfo,
                              ET9_CP_StrokeKeyseq * psKeyseq)
{
    ET9STATUS status = ET9STATUS_NONE;
    ET9SYMB sComp;

    /* convert to stroke keybuf */
    status = WSIToStrokeKeybuf(pET9CPLingInfo, pWordSymbInfo, psKeyseq->abKeys, &psKeyseq->bLen, &sComp, &psKeyseq->bPreCompLen);

    /* validate */
    if (ET9STATUS_NONE == status &&
        !IsStrokeKeyseqValid(psKeyseq->abKeys, psKeyseq->bLen))
    {
        status = ET9STATUS_INVALID_INPUT;
    }

    /* check for component */
    if (ET9STATUS_NONE == status) {
        status = ReadExpandComp(pET9CPLingInfo, sComp, psKeyseq);
    }
    return status;
}

static ET9UINT ET9LOCALCALL StrokeSyllableIsValid(ET9CPLingInfo * const pET9CPLingInfo,
                                                  ET9UINT nCharIndex,
                                                  ET9UINT fAllowComponent)
{
    const ET9_CP_CommonInfo *pCommon = &pET9CPLingInfo->CommonInfo;
    const ET9U16 *pwRange = pCommon->pwRange;
    ET9UINT nRangeIndex, nRangeIndexStart, nRangeIndexEnd;
    ET9U16 wSIDStart, wSIDEnd, wSID, wPID;

    if (nCharIndex > 0) {
        nRangeIndexStart = pCommon->pbRangeEnd[nCharIndex - 1];
    }
    else {
        nRangeIndexStart = 0;
    }
    nRangeIndexEnd = pCommon->pbRangeEnd[nCharIndex];
    for (nRangeIndex = nRangeIndexStart; nRangeIndex < nRangeIndexEnd; nRangeIndex += ET9_CP_ID_RANGE_SIZE) {
        wSIDStart = pwRange[nRangeIndex];
        wSIDEnd = pwRange[nRangeIndex + 2]; /* is valid if has exact or partial match */

        if (fAllowComponent) {
            if (wSIDEnd > wSIDStart) { /* any SID is fine */
                return 1;
            }
        }
        else { /* component not allowed, need to find a non-component SID */
            for (wSID = wSIDStart; wSID < wSIDEnd; wSID++) {
                if (ET9_CP_LookupID(pET9CPLingInfo, &wPID, wSID, 1, ET9_CP_Lookup_SIDToPID) &&
                    !ET9_CP_IS_COMP_PID(pCommon, wPID)) {
                    return 1;
                }
            }
        }
    }
    return 0; /* none of the ranges gives a valid SID, so this syllable is not valid */
}

static ET9UINT ET9LOCALCALL AllTrailingStrokeSyllableAreValid(ET9CPLingInfo * const pET9CPLingInfo,
                                                              ET9UINT fLastSylAllowComponent)
{
    ET9UINT i, fAllowComponent, nNumChars;

    nNumChars = (ET9UINT)pET9CPLingInfo->CommonInfo.bSylCount;
    /* check all syllable except the 1st one, which hasn't setup range yet */
    for (i = 1; i < nNumChars; i++) {
        /* allow component only if caller allows component and this is the last syllable */
        if (fLastSylAllowComponent && i == (ET9UINT)(nNumChars - 1)) {
            fAllowComponent = 1;
        }
        else {
            fAllowComponent = 0;
        }
        if (!StrokeSyllableIsValid(pET9CPLingInfo, i, fAllowComponent)) {
            return 0;
        }
    }
    return 1;
}

static ET9STATUS ET9LOCALCALL BuildPhraseStrokeSpellings(ET9CPLingInfo * const pET9CPLingInfo,
                                                         ET9_CP_StrokeKeyseq * psKeyseq)
{
    ET9_CP_CommonInfo *psCommonInfo = &pET9CPLingInfo->CommonInfo;
    ET9_CP_PhraseBuf *pMainPhraseBuf = ET9_CP_GetMainPhraseBuf(pET9CPLingInfo);
    ET9UINT fLastSylAllowComponent;
    ET9U8 bKeyBufLen;
    ET9STATUS eStatus;
    ET9U8 b, bNumStrokes1stChar;
    ET9U8 bLastDelPos;
    ET9U8 * pbKeyBuf; /* component expanded into strokes */
    ET9UINT fIsPhraseBufRestorable = !ET9_CP_IsPhraseBufEmpty(pMainPhraseBuf);

    ET9_CP_LoadStrokeKeyseq(pET9CPLingInfo, psKeyseq);

    pbKeyBuf = psKeyseq->abKeys;
    bKeyBufLen = psKeyseq->bLen;

    ET9_CP_StrokeCountChars(psKeyseq->abKeys, psKeyseq->bLen, &psCommonInfo->bSylCount, &bNumStrokes1stChar, &bLastDelPos);

    ET9_CP_ClearPhraseBuf(pMainPhraseBuf);
    ET9_CP_StrokeKeysChangeInit(pET9CPLingInfo);
    ET9_CP_SetTrailingSIDRanges(pET9CPLingInfo, pbKeyBuf, bNumStrokes1stChar, bKeyBufLen);

    /* check all syllable except the 1st one, which hasn't setup range yet */

    if (ET9CPIsComponentActive(pET9CPLingInfo) &&
        ET9CPSYLLABLEDELIMITER != pbKeyBuf[bKeyBufLen - 1]) {
        fLastSylAllowComponent = 1;
    }
    else { /* last key is delimiter --> syllable ended and component not allowed */
        fLastSylAllowComponent = 0;
    }

    /* validate trailing characters */
    if (psCommonInfo->bSylCount > 1 && !AllTrailingStrokeSyllableAreValid(pET9CPLingInfo, fLastSylAllowComponent)) {
        psCommonInfo->bFailNumSymbs = pET9CPLingInfo->Base.pWordSymbInfo->bNumSymbs;
        eStatus = ET9STATUS_INVALID_INPUT;
    }
    else {
        /* setup keybuf for 1st char */
        pET9CPLingInfo->CommonInfo.bKeyBufLen = 0;
        for(b = 0; b < bNumStrokes1stChar; b++) {
            ET9_CP_StrokeSetupKey(pET9CPLingInfo, pbKeyBuf[b]);
        }
        /* copy remaining keys into keybuf */
        pET9CPLingInfo->CommonInfo.bKeyBufLen = bKeyBufLen;
        for(; b < bKeyBufLen; b++) {
            pET9CPLingInfo->CommonInfo.bKeyBuf[b] = pbKeyBuf[b];
        }
        if (bLastDelPos && bLastDelPos == bKeyBufLen) {
            pMainPhraseBuf->bIsDelimiterExpansion = 1; /* Stroke, fill phrase first, spell later */
            ET9_CP_ExpandDelimiter(pET9CPLingInfo);
        }
        else {
            pMainPhraseBuf->bIsDelimiterExpansion = 0;
        }
        eStatus = ET9_CP_StrokeFillPhraseBuffer(pET9CPLingInfo);
    }

    if (ET9STATUS_NONE == eStatus) {
        MakeStrokeSpell(psKeyseq, &pET9CPLingInfo->CommonInfo.sActiveSpell);
    }
    else if (fIsPhraseBufRestorable) {
        ET9_CP_RestorePhraseBuf(pMainPhraseBuf);
    }

    return eStatus;
} /* end of BuildPhraseStrokeSpellings() */


ET9STATUS ET9FARCALL ET9_CP_StrokeBuildSpellings(ET9CPLingInfo * pET9CPLingInfo)
{
    ET9_CP_StrokeKeyseq sStrokeKeyseq;
    ET9STATUS status;

    /* If we've cleared keys past the last fail point, clear it to try again */
    if ( (ET9U8)ET9_CP_WSIValidLen(pET9CPLingInfo, pET9CPLingInfo->Base.pWordSymbInfo) < pET9CPLingInfo->CommonInfo.bFailNumSymbs ) {
        pET9CPLingInfo->CommonInfo.bFailNumSymbs = 0xFF;
    }

    status = StrokeWSIToKeySeq(pET9CPLingInfo, pET9CPLingInfo->Base.pWordSymbInfo, &sStrokeKeyseq);

    if (ET9STATUS_NONE == status) {
        status = BuildPhraseStrokeSpellings(pET9CPLingInfo, &sStrokeKeyseq);
    }

    return status;
}

#endif /* ifndef ET9CP_DISABLE_STROKE */

/* ----------------------------------< eof >--------------------------------- */
