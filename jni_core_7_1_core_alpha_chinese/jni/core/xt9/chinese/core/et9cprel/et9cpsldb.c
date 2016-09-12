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
;**     FileName: et9cpsldb.c                                                 **
;**                                                                           **
;**     Description: Chinese Stroke Text Input ldb database module.           **
;**                  Conforming to the development version of Chinese XT9.    **
;**                                                                           **
;*******************************************************************************
;******************************************************************************/

#include "et9api.h"
#include "et9cpldb.h"
#include "et9cpsldb.h"
#include "et9cprdb.h"
#include "et9cpsys.h"
#include "et9cpmisc.h"
#include "et9cpname.h"
#include "et9cpwdls.h"
#include "et9cppbuf.h"
#include "et9cprdb.h"
#include "et9cpcntx.h"
#include "et9cpspel.h"

#ifndef ET9CP_DISABLE_STROKE

static void ET9LOCALCALL ET9_CP_StrokeFindRange(ET9CPLingInfo *, ET9U8);
static ET9_CP_SIDMatch ET9LOCALCALL ET9_CP_StrokeIsSIDMatch(ET9CPLingInfo *pET9CPLingInfo,
                                                      ET9U16 wSID,
                                                      ET9U16 *pwNum,
                                                      ET9U8 bCharIndex);

#define ET9_CP_STROKE_NODE_SIZE     6
/* Read a stroke node at the given offset, returning all information */
static void ET9LOCALCALL ET9_CP_ReadStrokeNode(ET9CPLingInfo *pLingInfo,
                                               ET9U32   dwOffset,
                                               ET9BOOL  *pbIsLeaf,
                                               ET9U8    *pbChildStrokeMask,
                                               ET9U8    *pbExactCount,
                                               ET9U16   *pwPartialCount,
                                               ET9U16   *pwSiblingDist)
{
#ifdef ET9_DIRECT_LDB_ACCESS
    const ET9U8 ET9FARDATA *pNode;
    ET9Assert(ET9STATUS_NONE == ET9_CP_ReadLdbMultiBytes(pLingInfo, dwOffset, ET9_CP_STROKE_NODE_SIZE, &pNode) );
    ET9_CP_ReadLdbMultiBytes(pLingInfo, dwOffset, ET9_CP_STROKE_NODE_SIZE, &pNode);
#else
    ET9U8 pNode[ET9_CP_STROKE_NODE_SIZE];
    ET9Assert(ET9STATUS_NONE == ET9_CP_ReadLdbMultiBytes(pLingInfo, dwOffset, ET9_CP_STROKE_NODE_SIZE, pNode) );
    ET9_CP_ReadLdbMultiBytes(pLingInfo, dwOffset, ET9_CP_STROKE_NODE_SIZE, pNode);
#endif
    *pbIsLeaf = (ET9BOOL)(pNode[0] & 0x80);
    *pbChildStrokeMask = (ET9U8)(pNode[0] & 0x1F);
    *pwSiblingDist = (ET9U16)( ((ET9U16)pNode[1] << 8) | pNode[2] );
    *pbExactCount = (ET9U8)(pNode[3]);
    *pwPartialCount = (ET9U16)( ((ET9U16)pNode[4] << 8) | pNode[5] );
}

void ET9_CP_StrokeKeysChangeInit(ET9CPLingInfo *pET9CPLingInfo)
{
    pET9CPLingInfo->Private.SPrivate.wUdbLastRangeEnd = 0xFFFF; /* restart UDB search */
}

/*
 * bStart - starting position of the key for the given character's stroke sequence
 * bLastChar - 1: this is the last character in the stroke sequence;
 *             0: this is the first character in the stroke sequence.
 */
static void ET9LOCALCALL StrokeSetupLeafFilter(ET9CPLingInfo *pET9CPLingInfo, ET9U8 bStart)
{
    ET9_CP_SPrivate *psPrivate;
    ET9U8 *pbMask, *pbPack;
    ET9U16 wMask, wStroke;
    ET9U8 bNumRemainStrokeBits, bKeyMax, b;

    psPrivate = &pET9CPLingInfo->Private.SPrivate;

    if (bStart == ET9_CP_STROKE_MAXNODES) {
        /* initialize the remaining stroke buffer */
        pbMask = (ET9U8*)psPrivate->bStrokeMask;
        pbPack = (ET9U8*)psPrivate->bStrokePack;
        for(b = 0; b < ET9_CP_STROKE_MAX_MASK; b++) {
            *pbMask++ = 0;
            *pbPack++ = 0;
        }
    }
    bKeyMax = pET9CPLingInfo->CommonInfo.bKeyBufLen;
    for (b = bStart; b < bKeyMax; b++) {
        ET9U8 bKey = pET9CPLingInfo->CommonInfo.bKeyBuf[b];
        if (bKey == ET9CPSYLLABLEDELIMITER) {
            break;
        }
        /* bNumRemainStrokeBits is how many bits are occupied before the new stroke comes in */
        bNumRemainStrokeBits = (ET9U8)((b - ET9_CP_STROKE_MAXNODES) * 3);
        pbMask = psPrivate->bStrokeMask + (bNumRemainStrokeBits >> 3);
        pbPack = psPrivate->bStrokePack + (bNumRemainStrokeBits >> 3);

        bNumRemainStrokeBits = (ET9U8)(13 - (bNumRemainStrokeBits & 0x07));
        if (ET9CPSTROKE1 <= bKey && bKey <= ET9CPSTROKE5) {
            wMask = (ET9U16)((ET9U16)07 << bNumRemainStrokeBits);
            wStroke = (ET9U16)((ET9U16)bKey << bNumRemainStrokeBits);
            /* put the mask to the right place */
            *pbMask++ |= (ET9U8)(wMask >> 8);
            *pbMask |= (ET9U8)wMask;
            /* put the stroke bits in place */
            *pbPack++ |= (ET9U8)(wStroke >> 8);
            *pbPack |= (ET9U8)wStroke;
        }
    }
}

/* This function adds the SID of all related characters of the given component into the phrase buffer,
   from both the isolated set and the continuous set.
 */
static void ET9LOCALCALL ET9_CP_AddRelatedChars(ET9CPLingInfo *pET9CPLingInfo,
                                                ET9U32 dwStartOffset,
                                                ET9U32 dwEndOffset,
                                                ET9U16 *pwPrefix,
                                                ET9U8  bPrefixLen)
{
    ET9U16 wSID, wPID, wNumMatch, wSaveRange[3];
    ET9U32 dwReadOffset;
    ET9_CP_SIDMatch eMatch;
    ET9_CP_CommonInfo *pCommInfo = &pET9CPLingInfo->CommonInfo;
    ET9_CP_PhraseBuf *pMainPhraseBuf;
    ET9_CP_NameTableBookmark pNameTableBookmarks[ET9_CP_MAX_NAMETABLE_BOOKMARK];
    ET9U16 wCount, i;

    pMainPhraseBuf = ET9_CP_GetMainPhraseBuf(pET9CPLingInfo);
    /* save Ldb offsets before calling any function that may overwrite them
       because the search below rely on the Ldb offsets setup by caller */
    dwReadOffset = dwStartOffset;

    /* init bookmarks, it overwrites Ldb offsets.
       Use bookmarks to speed up name search in this function */
    ET9_CP_InitNameTableBookmarks(pET9CPLingInfo, pNameTableBookmarks, ET9_CP_MAX_NAMETABLE_BOOKMARK, 1);

    /* read isolated char count */
    wCount = ET9_CP_LdbReadWord(pET9CPLingInfo, dwReadOffset);
    dwReadOffset += 2; /* size of U16 */
    /* read isolated char SIDs */
    for (i = 0; i < wCount; i++) {
        wSID = ET9_CP_LdbReadWord(pET9CPLingInfo, dwReadOffset);
        dwReadOffset += 2;
        eMatch = ET9_CP_StrokeIsSIDMatch(pET9CPLingInfo, wSID, 0, 0);
        if (ET9_CP_SID_NO_MATCH != eMatch) {
            if (ET9_CP_AllowComponent(pET9CPLingInfo) ||
                 ( ET9_CP_LookupID(pET9CPLingInfo, &wPID, wSID, 1, ET9_CP_Lookup_SIDToPID) &&
                   ET9_CP_IS_NORMAL_PID(pCommInfo, wPID) ) ) {
                wSaveRange[0] = pCommInfo->pwRange[0];
                wSaveRange[1] = pCommInfo->pwRange[1];
                wSaveRange[2] = pCommInfo->pwRange[2];
                pCommInfo->pwRange[0] = wSID;
                pCommInfo->pwRange[1] = pCommInfo->pwRange[0];
                pCommInfo->pwRange[2] = (ET9U16)(pCommInfo->pwRange[0] + 1);
                if (ET9_CP_SID_EXACT_MATCH == eMatch) {
                    pCommInfo->pwRange[1]++;
                }
                ET9_CP_GetUdbPhrases(pET9CPLingInfo, 1, 1, pwPrefix, bPrefixLen, NULL, 1, 0, 0, 0, 0, pMainPhraseBuf);
                ET9_CP_GetLdbPhrases(pET9CPLingInfo, 1, 1, 1, NULL, 0, 0, 0, pwPrefix, bPrefixLen,
                        0, pNameTableBookmarks, pMainPhraseBuf);
                pCommInfo->pwRange[0] = wSaveRange[0];
                pCommInfo->pwRange[1] = wSaveRange[1];
                pCommInfo->pwRange[2] = wSaveRange[2];
            }
        } /* END SID match strokes */
    }

    /* continuous char SIDs */
    for (; dwReadOffset < dwEndOffset; ) {
        /* read continuous char count */
        wCount = ET9_CP_LdbReadWord(pET9CPLingInfo, dwReadOffset);
        dwReadOffset += 2; /* size of U16 */

        if (0 == wCount) {
            ET9Assert(dwReadOffset >= dwEndOffset);
            break; /* an empty continuous range should end this data block */
        }

        /* reading starting SID of this continuous SID range */
        wSID = ET9_CP_LdbReadWord(pET9CPLingInfo, dwReadOffset);
        dwReadOffset += 2;

        for (i = 0; i < wCount; wSID++, i++) {
            wNumMatch = 0;
            eMatch = ET9_CP_StrokeIsSIDMatch(pET9CPLingInfo, wSID, &wNumMatch, 0);
            if (ET9_CP_SID_NO_MATCH != eMatch) {
                if (!wNumMatch) {
                    wNumMatch = 1;
                }
                wNumMatch = (ET9U16)__ET9Min(wNumMatch, (wCount - i));
                wSaveRange[0] = pCommInfo->pwRange[0];
                wSaveRange[1] = pCommInfo->pwRange[1];
                wSaveRange[2] = pCommInfo->pwRange[2];
                pCommInfo->pwRange[0] = wSID;
                pCommInfo->pwRange[1] = pCommInfo->pwRange[0];
                pCommInfo->pwRange[2] = (ET9U16)(pCommInfo->pwRange[0] + wNumMatch);
                if (ET9_CP_SID_EXACT_MATCH == eMatch) {
                    pCommInfo->pwRange[1] = pCommInfo->pwRange[2];
                }

                ET9_CP_GetUdbPhrases(pET9CPLingInfo, 1, 1, pwPrefix, bPrefixLen, NULL, 1, 0, 0, 0, 0, pMainPhraseBuf);
                ET9_CP_GetLdbPhrases(pET9CPLingInfo, 1, 1, 1, NULL, 0, 0, 0, pwPrefix, bPrefixLen,
                        0, pNameTableBookmarks, pMainPhraseBuf);
                pCommInfo->pwRange[0] = wSaveRange[0];
                pCommInfo->pwRange[1] = wSaveRange[1];
                pCommInfo->pwRange[2] = wSaveRange[2];

                /* skip wNumMatch SIDs as they're covered by the pwRange this time */
                if (wNumMatch > 1) {
                    i = (ET9U8)(i + wNumMatch - 1);
                    wSID = (ET9U16)(wSID + wNumMatch - 1);
                }
            }
        }
    }

} /* END ET9_CP_AddRelatedChars */

/* get the start and end offsets to the data block of the given component */
static void ET9LOCALCALL ET9_CP_GetCompDataBlockOffset(ET9CPLingInfo *pET9CPLingInfo,
                                                       ET9U16 wCompUni,
                                                       ET9U32 *pdwStartOffset,
                                                       ET9U32 *pdwEndOffset)
{
    ET9U32 dwReadOffset;
    ET9_CP_SPrivate *psPrivate;

    psPrivate = &pET9CPLingInfo->Private.SPrivate;
    ET9Assert(ET9CPIsComponent(pET9CPLingInfo, wCompUni));

    /* offset into the data block address table */
    dwReadOffset = psPrivate->dwOffsetComponent + (ET9U32)(2 + wCompUni - pET9CPLingInfo->CommonInfo.wComponentFirst) * 2; /* sizeof(ET9U16) */

    /* read start and end offset of the requested data block from the address table */
    *pdwStartOffset = ET9_CP_LdbReadWord(pET9CPLingInfo, dwReadOffset) + psPrivate->dwOffsetComponent;
    dwReadOffset += 2;
    *pdwEndOffset = ET9_CP_LdbReadWord(pET9CPLingInfo, dwReadOffset) + psPrivate->dwOffsetComponent;
}

/*
 * given the Unicode of a related component, add one of its SID that matches the current stroke sequence
 */
static void ET9LOCALCALL ET9_CP_AddRelatedComp(ET9CPLingInfo *pET9CPLingInfo, ET9U16 wUni)
{
    ET9U32 dwReadOffset, dwStartOffset, dwEndOffset;
    ET9_CP_PhraseBuf *pMainPhraseBuf;
    ET9_CP_SIDMatch eMatch;
    ET9U16 wSID, wPID;
    ET9U8 bFreq, bExactMatch, bSIDCount, i;

    pMainPhraseBuf = ET9_CP_GetMainPhraseBuf(pET9CPLingInfo);

    /* reading the related component's offset and then set the data pointer */
    ET9_CP_GetCompDataBlockOffset(pET9CPLingInfo, wUni, &dwStartOffset, &dwEndOffset);

    dwReadOffset = dwStartOffset;
    /* read SID count */
    bSIDCount = ET9_CP_LdbReadByte(pET9CPLingInfo, dwReadOffset);
    dwReadOffset++;

    /* loop thru all alt SIDs to find the one that matches the stroke sequence, add it to phrase buf */
    for(i = 0; i < bSIDCount; i++) {
        ET9Assert(dwReadOffset < dwEndOffset);
        wSID = ET9_CP_LdbReadWord(pET9CPLingInfo, dwReadOffset);
        dwReadOffset += 2;

        /* add the related component, the component can't be an exact match */
        eMatch = ET9_CP_StrokeIsSIDMatch(pET9CPLingInfo, wSID, 0, 0);
        if (ET9_CP_SID_NO_MATCH != eMatch &&
            ET9_CP_LookupID(pET9CPLingInfo, &wPID, wSID, 1, ET9_CP_Lookup_SIDToPID) > 0)
        {
            ET9U16 wFreqEncoded;
            bFreq = ET9_CP_FreqLookup(pET9CPLingInfo, wPID);
            bExactMatch = 0;
            if (ET9_CP_SID_EXACT_MATCH == eMatch) {
                bExactMatch = 1;
            }
            wFreqEncoded = ET9_CP_EncodeFreq(pET9CPLingInfo, NULL, 1, bFreq, bExactMatch, 0, 0, NULL);
            ET9_CP_AddPhraseToBuf(pET9CPLingInfo, pMainPhraseBuf, &wSID, 1, NULL, 0, ET9_CP_IDEncode_SID, wFreqEncoded);
            break; /* only add this related component to phrase buf once */
        }
    }
}

/* filter out the unmatched characters from the component's character list, set the standard SID
 */
static ET9STATUS ET9LOCALCALL ET9_CP_StrokeFilterComponent(ET9CPLingInfo *pET9CPLingInfo,
                                           ET9U16 *pwPrefix,
                                           ET9U8 bPrefixLen)
{
    ET9U32 dwReadOffset, dwStartOffset, dwEndOffset;
    ET9U16 wCompUni, wSID, wRelatedCompUni, i;
    ET9U8 bSIDCount, bRelatedCompCount;

    wCompUni = pET9CPLingInfo->Private.SPrivate.wCompUnicode;
    ET9Assert(ET9CPIsComponent(pET9CPLingInfo, wCompUni));

    /* get start and end offset of the needed data block */
    ET9_CP_GetCompDataBlockOffset(pET9CPLingInfo, wCompUni, &dwStartOffset, &dwEndOffset);

    dwReadOffset = dwStartOffset;
    /* read SID count */
    bSIDCount = ET9_CP_LdbReadByte(pET9CPLingInfo, dwReadOffset);
    dwReadOffset++;

    /* read base SID and skip all Alt SIDs */
    wSID = ET9_CP_LdbReadWord(pET9CPLingInfo, dwReadOffset);
    dwReadOffset += (bSIDCount * 2);

    /* read related-component count */
    ET9Assert(dwReadOffset < dwEndOffset);
    bRelatedCompCount = ET9_CP_LdbReadByte(pET9CPLingInfo, dwReadOffset);
    dwReadOffset++;

    if (ET9_CP_AllowComponent(pET9CPLingInfo)) {
        /* for each related-component, add the SID that matches the stroke sequence into phrase buffer */
        for(i = 0; i < bRelatedCompCount; i++) {
            /* read the relative Unicode */
            ET9Assert(dwReadOffset < dwEndOffset);
            wRelatedCompUni = ET9_CP_LdbReadWord(pET9CPLingInfo, dwReadOffset);
            dwReadOffset += 2;

            ET9_CP_AddRelatedComp(pET9CPLingInfo, wRelatedCompUni);
        }
    }
    else {
        /* component not allowed, skip all related-components */
        dwReadOffset += (bRelatedCompCount * 2);
    }

    /* add related characters to phrase buf, data from dwReadOffset to dwEndOffset */
    ET9_CP_AddRelatedChars(pET9CPLingInfo, dwReadOffset, dwEndOffset, pwPrefix, bPrefixLen);
    return ET9STATUS_NONE;
}


ET9STATUS ET9FARCALL ET9_CP_StrokeSelectPhrase(ET9CPLingInfo *pET9CPLingInfo, ET9U16 wPhraseIndex, ET9_CP_Spell *pSpell, ET9U8 *pbNumSymbs)
{
    ET9STATUS status = ET9STATUS_NONE;
    ET9CPPhrase phraseEncoded, phraseUnicode;
    ET9_CP_SPrivate *pSPrivate = &pET9CPLingInfo->Private.SPrivate;
    ET9_CP_PhraseBuf *pMainPhraseBuf;
    ET9U16 wComp;
    ET9U8 bAltCount;

    ET9Assert(pSpell != NULL && pbNumSymbs != NULL);

    pMainPhraseBuf = ET9_CP_GetMainPhraseBuf(pET9CPLingInfo);
    phraseEncoded.bLen = 1;
#ifdef ET9_DEBUG
    phraseEncoded.pSymbs[0] = ET9_CP_NOMATCH;
#endif
    ET9_CP_GetPhraseFromBuf(pMainPhraseBuf, &phraseEncoded, NULL, wPhraseIndex + 1);

    bAltCount = ET9_CP_LookupID(pET9CPLingInfo, &wComp, phraseEncoded.pSymbs[0], 1, ET9_CP_Lookup_SIDToPID);
    ET9Assert(bAltCount > 0);
    wComp = ET9_CP_LookupUnicode(pET9CPLingInfo, wComp);

    if (pSpell->bLen > 0) { /* find number of symbs consumed by this selection */
        ET9UINT nStrokeStartIndex, nNumSymBeforeComponent, nComponentStrokeCount;
        ET9U8 bCharCount, bStrokeIndex;

        bCharCount = phraseEncoded.bLen;
        nStrokeStartIndex = ET9_CP_STROKE_SPELL_HEADER_LEN + pSpell->pbChars[ET9_CP_STROKE_SPELL_COMP_LEN_INDEX];
        for (bStrokeIndex = (ET9U8)nStrokeStartIndex; bStrokeIndex < pSpell->bLen && bCharCount > 0; bStrokeIndex++) {
            if ((ET9U8)ET9CPSYLLABLEDELIMITER == pSpell->pbChars[bStrokeIndex]) {
                bCharCount--;
            }
        }
        nNumSymBeforeComponent = pSpell->pbChars[ET9_CP_STROKE_SPELL_PRE_COMP_LEN_INDEX];
        nComponentStrokeCount = pSpell->pbChars[ET9_CP_STROKE_SPELL_COMP_LEN_INDEX];

        *pbNumSymbs = (ET9U8)(bStrokeIndex
                              - nStrokeStartIndex
                              + nNumSymBeforeComponent
                              + (nComponentStrokeCount ? 1 : 0));
    }
    else {
        *pbNumSymbs = 0;
    }

    /* if a component is selected, the stroke buffer doesn't change */
    if (ET9CPIsComponent(pET9CPLingInfo, wComp))
    {
        ET9Assert(ET9CPIsComponentActive(pET9CPLingInfo));
        /* just add the component into RDB */
        ET9_CP_UdbUsePhrase(pET9CPLingInfo, &phraseEncoded, ET9_CP_IDEncode_SID, 0, 1);
        return ET9STATUS_SELECTED_CHINESE_COMPONENT;
    }

    /* selected is not a component */
    pET9CPLingInfo->CommonInfo.bKeyBufLen = 0;
    pET9CPLingInfo->CommonInfo.bCachedNumSymbs = 0;
    pET9CPLingInfo->CommonInfo.bFailNumSymbs = 0xFF;

    pET9CPLingInfo->CommonInfo.bSylCount = 0;
    ET9_CP_StrokeKeysChangeInit(pET9CPLingInfo);
    pSPrivate->wCompUnicode = 0;
    pSPrivate->bCompNumStrokes = 0;

    /* Now ET9_CP_PhraseIsAllChn() */
    _ET9SymCopy(phraseUnicode.pSymbs, (ET9SYMB *)phraseEncoded.pSymbs, phraseEncoded.bLen);
    phraseUnicode.bLen = phraseEncoded.bLen;
    ET9_CP_ConvertPhraseToUnicode(pET9CPLingInfo, &phraseUnicode, ET9_CP_IDEncode_SID);

    if (ET9_CP_PhraseIsAllChn(pET9CPLingInfo, phraseEncoded.pSymbs, phraseEncoded.bLen)) {
        status = ET9_CP_SelectionHistAdd(&pET9CPLingInfo->SelHistory, phraseUnicode.pSymbs, phraseEncoded.pSymbs, phraseEncoded.bLen, *pbNumSymbs);
    }
    else {
        /*  NULL in the phraseEncoded position means do not add to ContextBuf */
        status = ET9_CP_SelectionHistAdd(&pET9CPLingInfo->SelHistory, phraseUnicode.pSymbs, NULL, phraseUnicode.bLen, *pbNumSymbs);
    }
    return status;
}


/*-----------------------------------------------------------------------------------
 * Description:
 *  This function searches all the characters in the current leaf
 *
 *  input:  bSearchFlag     0 - partial or exact match search, add matches to the buffer
 *                          1 - range search, update the range parameter
 *          bNumStrokes     the actual number of strokes to pass to AddBuffer function
 *          bLastChar       1 - this is the strokes for the last character in the phrase.
 *                          0 - this is the first character in the phrase.
 *
 *  return: ET9U16           if bSearchFlag is 1, the number of stroke IDs that match the key sequence
 *
 -----------------------------------------------------------------------------------*/
static void ET9LOCALCALL StrokeSearchLeaf(ET9CPLingInfo *pET9CPLingInfo,
                             ET9U8 bSearchFlag,
                             ET9U8 bActKeyLen,
                             ET9U8 bCharIndex,
                             ET9U16 *pwPrefix,
                             ET9U8 bPrefixLen)
{
    ET9_CP_CommonInfo *psCommInfo;
    ET9_CP_SPrivate *psPrivate;
    ET9_CP_PhraseBuf *pMainPhraseBuf;
    ET9U8 *pbMask, *pbPack;
    ET9UINT nTotalCount, nExactCount, nRangeIndex, i;
    ET9U32 dwOffset, dwOffsetCounts, dwOffsetStrokes;
    ET9U16 wPartialCount, wSiblingDist, wFirstLeafSID, wLastLeafSID, wSID;
    ET9BOOL bIsLeaf, bFirstTime, bNeedGetPhrase;
    ET9U8 bChildStrokes, bExactCount, bStrokeCount, bLeafStrokeByteCount, bLeafStrokeByte, bActStrokeByteCount;

    psPrivate = &pET9CPLingInfo->Private.SPrivate;
    psCommInfo = &pET9CPLingInfo->CommonInfo;
    pMainPhraseBuf = ET9_CP_GetMainPhraseBuf(pET9CPLingInfo);
    nRangeIndex = bCharIndex * ET9_CP_ID_RANGE_SIZE;

    dwOffset = psPrivate->dwNodeOffsets[ET9_CP_STROKE_MAXNODES];
    ET9_CP_ReadStrokeNode(pET9CPLingInfo, dwOffset, &bIsLeaf, &bChildStrokes, &bExactCount, &wPartialCount, &wSiblingDist);
    if (!bIsLeaf) {
        /* This node is at the last layer but has no partial match.  It has no leaf data structure.
           Since the active key length > stroke count of exact matches in this node,
           we can clear the pwRange before return without any match */
        psCommInfo->pwRange[nRangeIndex] = 0;
        psCommInfo->pwRange[nRangeIndex + 2] = 0;
        ET9Assert(bExactCount > 0 && 0 == wPartialCount);
        ET9Assert(bActKeyLen > ET9_CP_STROKE_MAXNODES);
        return;
    }
    ET9Assert(wPartialCount > 0);

    wFirstLeafSID = (ET9U16)(psPrivate->wCurNodeSIDBase[ET9_CP_STROKE_MAXNODES] + bExactCount);
    wLastLeafSID = (ET9U16)(wFirstLeafSID + wPartialCount - 1);
    dwOffsetCounts = dwOffset + ET9_CP_STROKE_NODE_SIZE; /* set offset to leaf strokes area */
    dwOffsetStrokes = dwOffsetCounts + wPartialCount;

    bFirstTime = 1;
    bNeedGetPhrase = 0;
    nExactCount = 0;
    nTotalCount = 0;
    /* setup for comparing with active keys */
    pbMask = psPrivate->bStrokeMask;
    pbPack = psPrivate->bStrokePack;
    ET9Assert(bActKeyLen > ET9_CP_STROKE_MAXNODES);
    bActStrokeByteCount = (ET9U8)(((bActKeyLen - ET9_CP_STROKE_MAXNODES) * 3 + 7) / 8);

    for (wSID = wFirstLeafSID; wSID <= wLastLeafSID; wSID++) {
        bStrokeCount = ET9_CP_LdbReadByte(pET9CPLingInfo, dwOffsetCounts++); /* leaf stroke count */
        bLeafStrokeByteCount = (ET9U8)((bStrokeCount * 3 + 7) / 8);
        bStrokeCount += ET9_CP_STROKE_MAXNODES; /* full stroke count */
        if (bStrokeCount < bActKeyLen) {
            dwOffsetStrokes += bLeafStrokeByteCount;
            continue; /* skip chars that have fewer strokes than the input */
        }
        ET9Assert(bStrokeCount >= bActKeyLen);
        ET9Assert(bLeafStrokeByteCount >= bActStrokeByteCount);
        for (i = 0; i < bActStrokeByteCount; i++) {
            bLeafStrokeByte = ET9_CP_LdbReadByte(pET9CPLingInfo, dwOffsetStrokes + i);
            if ((bLeafStrokeByte & pbMask[i]) != pbPack[i]) {
                break;
            }
        }
        dwOffsetStrokes += bLeafStrokeByteCount; /* increase stroke offset to next char */
        if (i == bActStrokeByteCount) { /* all active strokes are matched for this char */
            if (bSearchFlag) {
                if (0 == nTotalCount) {
                    psCommInfo->pwRange[nRangeIndex] = wSID;
                }
                nTotalCount++;
                if (bStrokeCount == bActKeyLen) {
                    nExactCount++;
                }
            }
            else {
                if (!bFirstTime &&
                    (((wSID == psCommInfo->pwRange[nRangeIndex + 2]) && (bStrokeCount > bActKeyLen)) ||
                     ((wSID == psCommInfo->pwRange[nRangeIndex + 1]) && (bStrokeCount == bActKeyLen)))) {
                    psCommInfo->pwRange[nRangeIndex + 2]++;
                    if (bStrokeCount == bActKeyLen) {
                        psCommInfo->pwRange[nRangeIndex + 1]++;
                    }
                    bNeedGetPhrase = 1;
                }
                else {
                    if (!bFirstTime) {
                        bNeedGetPhrase = 0;
                        ET9_CP_GetUdbPhrases(pET9CPLingInfo, 1, 1, pwPrefix, bPrefixLen, NULL, 1, 0, 0, 0, 0, pMainPhraseBuf);
                        ET9_CP_GetLdbPhrases(pET9CPLingInfo, 1, 1, 1, NULL, 0, 0, 0,
                                           pwPrefix, bPrefixLen, 0, 0, pMainPhraseBuf);  /* get LDB phrases based SID ranges */
                    }
                    psCommInfo->pwRange[nRangeIndex] = wSID;
                    psCommInfo->pwRange[nRangeIndex + 2] = (ET9U16)(psCommInfo->pwRange[nRangeIndex] + 1);
                    if (bStrokeCount == bActKeyLen) {
                        psCommInfo->pwRange[nRangeIndex + 1] = (ET9U16)(psCommInfo->pwRange[nRangeIndex] + 1);
                    }
                    else {
                        psCommInfo->pwRange[nRangeIndex + 1] = psCommInfo->pwRange[nRangeIndex];
                    }
                    bNeedGetPhrase = 1;
                }
                bFirstTime = 0;
            }
        }
        else if (nTotalCount > 0 && bSearchFlag) { /* have found all the matches, no more beyond this SID */
            break;
        }
    }

    if (bSearchFlag) {
        psCommInfo->pwRange[nRangeIndex + 2] = (ET9U16)(psCommInfo->pwRange[nRangeIndex] + nTotalCount);
        psCommInfo->pwRange[nRangeIndex + 1] = (ET9U16)(psCommInfo->pwRange[nRangeIndex] + nExactCount);
    }
    else if (bNeedGetPhrase) {
        ET9_CP_GetUdbPhrases(pET9CPLingInfo, 1, 1, pwPrefix, bPrefixLen, NULL, 1, 0, 0, 0, 0, pMainPhraseBuf);
        ET9_CP_GetLdbPhrases(pET9CPLingInfo, 1, 1, 1, NULL, 0, 0, 0, pwPrefix, bPrefixLen, 0, 0, pMainPhraseBuf);  /* get LDB phrases based SID ranges */
    }
}

static ET9U16 ET9LOCALCALL ET9_CP_StrokeSearchForExactMatch(ET9CPLingInfo *pLingInfo, ET9U8 bNode)
{
    /* getting number of exact matches from the node */
    ET9U32 dwOffset;
    dwOffset = pLingInfo->Private.SPrivate.dwNodeOffsets[bNode] + 3;
    return (ET9U16)(ET9_CP_LdbReadByte(pLingInfo, dwOffset) & 0x1F);
}

/*-----------------------------------------------------------------------------------
 *  Description:
 *      The function updates wCurNodeSIDBase and dwNodeOffsets for the added node
 *      based on the current node.  There is no function to remove a node.  Adding a node
 *      makes all the nodes after the current invalid.  if the total number of keys is more than
 *      ET9_CP_STROKE_MAXNODES, the caller needs to guarentee the nubmer of valid nodes is
 *      ET9_CP_STROKE_MAXNODES
 *
 *  input:  bKey    The key to be added to the node
 *          bNode   the current number of nodes, it has to be less than ET9_CP_STROKE_MAXNODES
 *
 *  out:    updated wCurNodeSIDBase[bNode] and dwNodeOffsets[bNode]
 *          read buffer offset is undefined
 *
 -----------------------------------------------------------------------------------*/
static ET9STATUS ET9LOCALCALL StrokeAddNodeOffset(ET9CPLingInfo *pET9CPLingInfo, ET9U8 bKey, ET9U8 bNode)
/* bNode and bKey are all validated */
{
    ET9U32 dwOffset;
    ET9_CP_SPrivate *psPrivate;
    ET9U16 wPartialCount, wSiblingDist, wNewSIDBase;
    ET9U8 bChildStrokes, bExactCount, i;
    ET9BOOL bIsLeaf;

    psPrivate = &pET9CPLingInfo->Private.SPrivate;

    ET9Assert( 1 <= bNode && bNode <= ET9_CP_STROKE_MAXNODES );
    dwOffset = psPrivate->dwNodeOffsets[bNode - 1];
    ET9_CP_ReadStrokeNode(pET9CPLingInfo, dwOffset, &bIsLeaf, &bChildStrokes, &bExactCount, &wPartialCount, &wSiblingDist);
    dwOffset += ET9_CP_STROKE_NODE_SIZE; /* set offset to first child of current node */
    if (0 == bChildStrokes) {
        /* current node has no child, cannot add any stroke */
        return ET9STATUS_INVALID_INPUT;
    }
    ET9Assert(!bIsLeaf);
    if (bKey == ET9CPSTROKEWILDCARD) {
        /* set bKey to the first valid stroke */
        for(bKey = 0; bKey < ET9_CP_STROKE_NUMOFSTROKE && !(bChildStrokes & (1 << bKey)); bKey++) {
        }
        bKey++;
    }
    psPrivate->bActStrokes[bNode - 1] = bKey;

    if (!(bChildStrokes & (1 << (bKey - ET9CPSTROKE1))) ) {
        /* the key is not a child stroke */
        return ET9STATUS_INVALID_INPUT;
    }

    wNewSIDBase = (ET9U16)(psPrivate->wCurNodeSIDBase[bNode - 1] + bExactCount);

    for (i = ET9CPSTROKE1; i <= bKey && i <= ET9CPSTROKE5; i++) {
        /* read each child of the current node */
        if (bChildStrokes & 1) {
            ET9U8 bGrandChildStrokes;
            ET9_CP_ReadStrokeNode(pET9CPLingInfo, dwOffset, &bIsLeaf, &bGrandChildStrokes, &bExactCount, &wPartialCount, &wSiblingDist);
            if (i < bKey) {
                wNewSIDBase = (ET9U16)(wNewSIDBase + bExactCount + wPartialCount);
                dwOffset += wSiblingDist;
            }
            else { /* i == bKey */
                psPrivate->wCurTotalMatch[bNode] = (ET9U16)(bExactCount + wPartialCount);
            }
        }
        bChildStrokes = (ET9U8)(bChildStrokes >> 1);
    }
    ET9Assert(psPrivate->wCurNodeSIDBase[0] == 0);
    psPrivate->wCurNodeSIDBase[bNode] = wNewSIDBase;
    psPrivate->dwNodeOffsets[bNode] = dwOffset;

    return ET9STATUS_NONE;
}

/* this function is called only when there is any wildcard in the first character's strokes */
static void ET9LOCALCALL ET9_CP_StrokeSearch(ET9CPLingInfo *pET9CPLingInfo,
                              ET9U16 *pwPrefix,
                              ET9U8 bPrefixLen)
{
    ET9STATUS status;
    ET9_CP_SPrivate *psPrivate;
    ET9_CP_CommonInfo *psCommInfo;
    ET9_CP_PhraseBuf *pMainPhraseBuf;
    ET9S8 b;
    ET9U8 bKeyLimit, bActKeyLen, bLastChar;

    psPrivate = &pET9CPLingInfo->Private.SPrivate;
    psCommInfo = &pET9CPLingInfo->CommonInfo;
    pMainPhraseBuf = ET9_CP_GetMainPhraseBuf(pET9CPLingInfo);
    /* bKeyLimit is the number of keys that needs to be searched for wild card or partial match
       for the first character in the phrase */
    for(bActKeyLen = 0; bActKeyLen < psCommInfo->bKeyBufLen; bActKeyLen++) {
        if (psCommInfo->bKeyBuf[bActKeyLen] == ET9CPSYLLABLEDELIMITER) {
            break;
        }
    }
    if (bActKeyLen < psCommInfo->bKeyBufLen) {
        bLastChar = 0;
    } else {
        bLastChar = 1;
    }
    bKeyLimit = (ET9U8)__ET9Min(bActKeyLen, ET9_CP_STROKE_MAXNODES);
    /* initialize */
    for(b = 0; b < bKeyLimit; b++) {
        psPrivate->bActStrokes[b] = psCommInfo->bKeyBuf[b];
    }
    for(b = 1, status = ET9STATUS_NONE; b > 0;) {
        /* get exact matches for every added stroke */
        for(; b <= bKeyLimit; b++) {
            status = StrokeAddNodeOffset(pET9CPLingInfo, psPrivate->bActStrokes[b - 1], b);
            if (status != ET9STATUS_NONE) {
                break;
            }
        }
        b--;
        if ((status == ET9STATUS_NONE)) {
            b--;
            if (bActKeyLen > ET9_CP_STROKE_MAXNODES) {
                StrokeSearchLeaf(pET9CPLingInfo, 0, bActKeyLen, 0, pwPrefix, bPrefixLen);
            } else {
                psCommInfo->pwRange[0] = psPrivate->wCurNodeSIDBase[bActKeyLen];
                psCommInfo->pwRange[1] = (ET9U16)(psCommInfo->pwRange[0] +
                                                 ET9_CP_StrokeSearchForExactMatch(pET9CPLingInfo, bKeyLimit));
                psCommInfo->pwRange[2] = (ET9U16)(psCommInfo->pwRange[0] + psPrivate->wCurTotalMatch[bActKeyLen]);

                ET9_CP_GetUdbPhrases(pET9CPLingInfo, 1, 1, pwPrefix, bPrefixLen, NULL, 1, 0, 0, 0, 0, pMainPhraseBuf);
                ET9_CP_GetLdbPhrases(pET9CPLingInfo, 1, 1, 1, NULL, 0, 0, 0, pwPrefix, bPrefixLen, 0, 0, pMainPhraseBuf);  /* get LDB phrases based SID ranges */
            }
        }
        for(; b >= 0; b--) {
            status = ET9STATUS_NONE;

            if (pET9CPLingInfo->CommonInfo.bKeyBuf[b] == ET9CPSTROKEWILDCARD) {
                if (psPrivate->bActStrokes[b] < ET9CPSTROKE1 + ET9_CP_STROKE_NUMOFSTROKE - 1) {
                    psPrivate->bActStrokes[b]++;
                    break;
                } else {
                    psPrivate->bActStrokes[b] = ET9CPSTROKEWILDCARD;
                }
            }
        }
        b++; /* exit when b == 0 (processed every key in the buffer) */
    }
}

/* this changes the data cache */
static void ET9LOCALCALL ET9_CP_StrokeFindRange(ET9CPLingInfo *pET9CPLingInfo, ET9U8 bLastChar)
{
    ET9_CP_SPrivate *psPrivate;
    ET9U8 b, bActKeyLen, bIndex = 0, bRangeIndex;
    ET9STATUS status;
    ET9_CP_CommonInfo *psCommInfo;

    psPrivate = &pET9CPLingInfo->Private.SPrivate;
    psCommInfo = &pET9CPLingInfo->CommonInfo;

    if (bLastChar) {
        bActKeyLen = (ET9U8)(psCommInfo->bKeyBufLen);
        bIndex = (ET9U8)(psCommInfo->bSylCount - 1);
        if (!bActKeyLen) {
            psCommInfo->bSylCount--; /* prepare for expand delimiter */
            if (!ET9_CP_ExpandDelimiter(pET9CPLingInfo)) {
                psCommInfo->bSylCount++; /* restore if expand delimiter failed */
            }
            return;
        }
    }
    else {
        for(bActKeyLen = 0; bActKeyLen < psCommInfo->bKeyBufLen; bActKeyLen++) {
            if (psCommInfo->bKeyBuf[bActKeyLen] == ET9CPSYLLABLEDELIMITER) {
                break;
            }
        }
    }

    /* only the first character can have a wild card */
    psPrivate->bIsRangeExact = 1;
    for(b = 0;  b < bActKeyLen; b++) {
        if (psCommInfo->bKeyBuf[b] == ET9CPSTROKEWILDCARD) {
            psPrivate->bIsRangeExact = 0;
            break;
        }
    }

    bRangeIndex = (ET9U8)(bIndex * ET9_CP_ID_RANGE_SIZE);
    for(b = 0; ((b < bActKeyLen) && (b < ET9_CP_STROKE_MAXNODES) &&
                (psCommInfo->bKeyBuf[b] != ET9CPSTROKEWILDCARD)); b++) {
        status = StrokeAddNodeOffset(pET9CPLingInfo, psCommInfo->bKeyBuf[b], (ET9U8)(b + 1));
        if (status != ET9STATUS_NONE) {
            psCommInfo->pwRange[bRangeIndex] = 0;
            psCommInfo->pwRange[bRangeIndex + 2] = 0;
            return;
        }
    }
    psCommInfo->pwRange[bRangeIndex] = psPrivate->wCurNodeSIDBase[b];
    psCommInfo->pwRange[bRangeIndex + 2] = (ET9U16)(psCommInfo->pwRange[bRangeIndex] + psPrivate->wCurTotalMatch[b]);

    if (psPrivate->bIsRangeExact || bIndex != 0) {
        if (bActKeyLen > ET9_CP_STROKE_MAXNODES) {
            StrokeSearchLeaf(pET9CPLingInfo, 1, bActKeyLen, bIndex, 0, 0);
        }
        else {
            psCommInfo->pwRange[bRangeIndex + 1] = (ET9U16)(psCommInfo->pwRange[bRangeIndex] +
                                                           ET9_CP_StrokeSearchForExactMatch(pET9CPLingInfo, bActKeyLen));
        }
    }
    if (b == 0) {
        psCommInfo->pwRange[bRangeIndex + 2] = (ET9U16)(psCommInfo->pwRange[bRangeIndex] + psPrivate->wCurTotalMatch[0]);
    }
}

ET9STATUS ET9FARCALL ET9_CP_StrokeFillPhraseBufferFromKeys(ET9CPLingInfo *pET9CPLingInfo)
{
    ET9STATUS        mStatus = ET9STATUS_NONE;
    ET9_CP_SPrivate *psPrivate = &pET9CPLingInfo->Private.SPrivate;
    ET9_CP_CommonInfo *psCommInfo = &pET9CPLingInfo->CommonInfo;
    ET9_CP_PhraseBuf *pMainPhraseBuf;

    pMainPhraseBuf = ET9_CP_GetMainPhraseBuf(pET9CPLingInfo);

    if (psCommInfo->bKeyBufLen) {
        ET9UINT nRangeIndex;
        ET9U8 b, bCharIndex;

        psCommInfo->bSylCount = 1;

        /* count delims to get numChars */
        {
            for (b = 0; b < psCommInfo->bKeyBufLen; b++) {
                if (ET9CPSYLLABLEDELIMITER == psCommInfo->bKeyBuf[b]) {
                    psCommInfo->bSylCount++;
                }
            }

            /* don't count last delim if it's a failed delimiter expansion */
            if (ET9CPSYLLABLEDELIMITER == psCommInfo->bKeyBuf[psCommInfo->bKeyBufLen - 1] &&
                !pMainPhraseBuf->bIsDelimiterExpansion)
            {
                psCommInfo->bSylCount--;
            }
        }

        bCharIndex = (ET9U8)(psCommInfo->bSylCount - 1);
        nRangeIndex = bCharIndex * ET9_CP_ID_RANGE_SIZE;
        /* get first character's stroke ID range */
        for (b = 0; (b < ET9_CP_STROKE_MAXNODES) && (b < psCommInfo->bKeyBufLen); b++) {
            psPrivate->bActStrokes[b] = psCommInfo->bKeyBuf[b];
            psPrivate->wCurTotalMatch[b+1] = 0;
        }
        ET9_CP_StrokeFindRange(pET9CPLingInfo, 0);

        if (!psCommInfo->pwRange[nRangeIndex + 2] ||
            (bCharIndex && (psCommInfo->pwRange[nRangeIndex + 2] == psCommInfo->pwRange[nRangeIndex]))) {
            psCommInfo->bFailNumSymbs = pET9CPLingInfo->Base.pWordSymbInfo->bNumSymbs;
            return ET9STATUS_INVALID_INPUT; /* invalid input */
        }
    }
    else {
        psCommInfo->bSylCount = 0;
    }

    {
        ET9U8 *pbContextLen, bPrefixLen, b;
        ET9U16 *pwPrefix;
        /* if the buffer is empty, refill it */
        pbContextLen = pET9CPLingInfo->CommonInfo.pbContextLen;
        pwPrefix = pET9CPLingInfo->CommonInfo.pwContextBuf;
        bPrefixLen = 0;
        for (b = 0; pbContextLen[b]; b++) {
            bPrefixLen = (ET9U8)(bPrefixLen + pbContextLen[b]);
        }
        for (b = 0; pbContextLen[b]; b++) {
            ET9Assert(bPrefixLen);
            if (psPrivate->wCompUnicode) {
                /* a component is selected */
                ET9_CP_StrokeFilterComponent(pET9CPLingInfo, pwPrefix, bPrefixLen);
            }
            else {
                if (psCommInfo->bSylCount == 0 || psPrivate->bIsRangeExact) {
                    ET9_CP_GetUdbPhrases(pET9CPLingInfo, 1, 1, pwPrefix, bPrefixLen, NULL, 1, 0, 0, 0, 0, pMainPhraseBuf);
                    ET9_CP_GetLdbPhrases(pET9CPLingInfo, 1, 1, 1, NULL, 0, 0, 0, pwPrefix, bPrefixLen, 0, 0, pMainPhraseBuf);
                }
                else {
                    ET9_CP_StrokeSearch(pET9CPLingInfo, pwPrefix, bPrefixLen);
                }
            }
            bPrefixLen = (ET9U8)(bPrefixLen - pbContextLen[b]);
            pwPrefix += pbContextLen[b];
        }

        if (psCommInfo->bKeyBufLen == 0) {
            /* no key, so add smart puncts and common chars */
            ET9_CP_GetSmartPuncts(pET9CPLingInfo);

            if (ET9_CP_IsPhraseBufEmpty(pMainPhraseBuf) || /* the current page is not full */
                !ET9_CP_PhraseBufPageFillDone(pMainPhraseBuf))
            {
                /* build common charcters */
                if (ET9CPIsNameInputActive(pET9CPLingInfo)) {
                    ET9_CP_GetCommonNameChar(pET9CPLingInfo, 1);
                }
                else {
                    ET9_CP_GetCommonChar(pET9CPLingInfo);
                }
            }
        }
        else {
            /* search for ldb match */
            if (psPrivate->wCompUnicode) {
                /* a component is selected */
                ET9_CP_StrokeFilterComponent(pET9CPLingInfo, 0, 0);
            }
            else {
                if (psPrivate->bIsRangeExact) {
                    ET9_CP_GetUdbPhrases(pET9CPLingInfo, 1, 1, 0, 0, NULL, 1, 0, 0, 0, 0, pMainPhraseBuf);
                    ET9_CP_GetLdbPhrases(pET9CPLingInfo, 1, 1, 1, NULL, 0, 0, 0, 0, 0, 0, 0, pMainPhraseBuf);
                }
                else {
                    ET9_CP_StrokeSearch(pET9CPLingInfo, 0, 0);
                }
            }
        }
    }

    return mStatus;
}   /* end of ET9_CP_StrokeFillPhraseBufferFromKeys() */

ET9STATUS ET9FARCALL ET9_CP_StrokeFillPhraseBuffer(ET9CPLingInfo *pET9CPLingInfo)
{
    ET9STATUS status;
    ET9_CP_CommonInfo *pCommon = &pET9CPLingInfo->CommonInfo;
    ET9_CP_PhraseBuf *pMainPhraseBuf;

    ET9U8 *pbKeyBuf = pCommon->bKeyBuf;
    ET9UINT n1stCharStrokeCount;

    ET9_FUNC_HIT("ET9_CP_StrokeFillPhraseBuffer");

    pMainPhraseBuf = ET9_CP_GetMainPhraseBuf(pET9CPLingInfo);
    ET9Assert(pMainPhraseBuf->wLastTotal == pMainPhraseBuf->wTotal);
    if (pCommon->bFailNumSymbs <= pET9CPLingInfo->Base.pWordSymbInfo->bNumSymbs ) {
        return ET9STATUS_INVALID_INPUT;
    }

    /* count 1st char's strokes */
    for (n1stCharStrokeCount = 0; n1stCharStrokeCount < pCommon->bKeyBufLen; n1stCharStrokeCount++) {
        if (ET9CPSYLLABLEDELIMITER == pbKeyBuf[n1stCharStrokeCount]) {
            break;
        }
    }

    if (n1stCharStrokeCount + 1 < pCommon->bKeyBufLen &&
        ET9_CP_PhraseBufEndsWithSingleChar(pMainPhraseBuf))
    {
        /* if we've already been adding single syls then we're done finding phrases */
        status = ET9STATUS_NONE;
    }
    else {
        status = ET9_CP_StrokeFillPhraseBufferFromKeys(pET9CPLingInfo);
        if (ET9_CP_IsPhraseBufEmpty(pMainPhraseBuf) && pMainPhraseBuf->bIsDelimiterExpansion) {
            pMainPhraseBuf->bIsDelimiterExpansion = 0; /* tried delimiter expansion but failed */
            status = ET9_CP_StrokeFillPhraseBufferFromKeys(pET9CPLingInfo);
        }
    }

    /* append 1st syl char matches to phrase buffer */
    if (ET9STATUS_NONE == status &&                     /* no error */
        pCommon->bKeyBufLen > 0 &&                      /* has keys */
        !ET9_CP_PhraseBufPageFillDone(pMainPhraseBuf))   /* current page not filled */
    {

        if (n1stCharStrokeCount + 1 < pCommon->bKeyBufLen || /* has strokes beyond 1st syllable --> not covered by non-delimiter-expansion yet */
            !ET9_CP_IsPhraseBufEmpty(pMainPhraseBuf))
        {
            ET9UINT nSavedKeyBufLen, nCurPageItemCount, n;
            ET9U8 bSavedState;

            nSavedKeyBufLen = pCommon->bKeyBufLen;

            if (!ET9_CP_PhraseBufEndsWithSingleChar(pMainPhraseBuf)) {
                /* reset freq array to keep the presence and order of existing phrases in phrase buffer */
                pMainPhraseBuf->wPrevFreq = 0xFFFF;
                nCurPageItemCount = pMainPhraseBuf->wTotal % pMainPhraseBuf->bPageSize;
                for (n = 0; n < nCurPageItemCount; n++) {
                    pMainPhraseBuf->pwFreq[n] = 0xFFFF;
                }
            }

            bSavedState = pET9CPLingInfo->bState; /* save states */
            pET9CPLingInfo->bState &= (ET9U8)~ET9_CP_STATE_COMPONENT; /* disable component state */
            /* validate 1st char's strokes */
            pCommon->bKeyBufLen = 0;
            for (n = 0; n < n1stCharStrokeCount; n++) {
                ET9_CP_StrokeSetupKey(pET9CPLingInfo, pbKeyBuf[n]);
            }
            ET9_CP_StrokeKeysChangeInit(pET9CPLingInfo);
            status = ET9_CP_StrokeFillPhraseBufferFromKeys(pET9CPLingInfo);

            pET9CPLingInfo->bState = bSavedState; /* restore component state */
            pCommon->bKeyBufLen = (ET9U8)nSavedKeyBufLen; /* restore key buf len */
        }
    }

    if (ET9_CP_IsPhraseBufEmpty(pMainPhraseBuf) && pET9CPLingInfo->Base.pWordSymbInfo->bNumSymbs > 0) {
        pCommon->bFailNumSymbs = pET9CPLingInfo->Base.pWordSymbInfo->bNumSymbs;
        status = ET9STATUS_INVALID_INPUT;
    }
    else if (pMainPhraseBuf->wLastTotal >= pMainPhraseBuf->wTotal) {
        ET9Assert(pMainPhraseBuf->wLastTotal == pMainPhraseBuf->wTotal);
        status = ET9STATUS_OUT_OF_RANGE;
    }
    pMainPhraseBuf->wLastTotal = pMainPhraseBuf->wTotal;
    return status;
}   /* end of ET9_CP_StrokeFillPhraseBuffer() */

void ET9_CP_StrokeSetupKey(ET9CPLingInfo *pET9CPLingInfo, ET9U8 bKey)
{
    ET9U8            bLastKeyBufLen;
    ET9_CP_SPrivate * pSPrivate = &pET9CPLingInfo->Private.SPrivate;
    ET9_CP_CommonInfo * pCommInfo = &pET9CPLingInfo->CommonInfo;

    pCommInfo->bKeyBuf[pCommInfo->bKeyBufLen++] = bKey;
    bLastKeyBufLen = (ET9U8)(pCommInfo->bKeyBufLen);
    ET9Assert(bLastKeyBufLen > 0);

    if (bLastKeyBufLen <= ET9_CP_STROKE_MAXNODES) {
        pSPrivate->bActStrokes[bLastKeyBufLen - 1] = bKey;
    } else {
        ET9U8 bStart;
        if (pSPrivate->wCompUnicode && ET9_CP_AllowComponent(pET9CPLingInfo)) {
            bStart = ET9_CP_STROKE_MAXNODES;
        } else {
            bStart = (ET9U8)(bLastKeyBufLen - 1);
        }
        StrokeSetupLeafFilter(pET9CPLingInfo, bStart);
    } /* end of else (pCommInfo->bKeyBufLen <= ET9_CP_STROKE_MAXNODES) */
} /* end of ET9_CP_StrokeSetupKey */

/*-----------------------------------------------------------------------------------
 * Description:
 *      The function tests if a SID is a match for the current stroke sequence.
 *      This function doesn't change the data offset.  It doesn't test if it is related
 *      to the selected component.
 *
 * input:   pET9CPLingInfo  a pointer pointing to the ET9CPLingInfo structure
 *          wSID            The SID to be tested
 *
 * return:  ET9_CP_SIDMatch   ET9_CP_SID_NO_MATCH - the SID is not a match of the current key sequence
 *                          ET9_CP_SID_EXACT_MATCH - the SID is an exact match of the current key sequence
 *                          ET9_CP_SID_PARTIAL_MATCH - the SID is a partial match
 *
 * output:  *pwNum          Indicates how many continuous stroke IDs following wSID have the same match type.
 *                          The group of stroke IDs include wSID.  This value should be used in the caller
 *                          to reduce the number of calls to this function if the stroke IDs to be tested is sorted.
 -----------------------------------------------------------------------------------*/
static ET9_CP_SIDMatch ET9LOCALCALL ET9_CP_StrokeIsSIDMatch(ET9CPLingInfo *pET9CPLingInfo,
                                                            ET9U16 wSID,
                                                            ET9U16 *pwNum,
                                                            ET9U8 bCharIndex)
{
    ET9_CP_SPrivate *psPrivate;
    ET9U8 b, bStrokeKeyLen, bRangeIndex;
    ET9STATUS status;
    ET9_CP_SIDMatch eMatch = ET9_CP_SID_UNKNOWN_MATCH;
    ET9_CP_CommonInfo *pCommInfo;
    ET9U16 wSIDEndSave = 0;
    ET9U8  bMax = 0;

    if (pwNum) {
        *pwNum = 0;
    }
    if (pET9CPLingInfo->CommonInfo.bKeyBufLen == 0) {
        return ET9_CP_SID_PARTIAL_MATCH;
    }
    psPrivate = &pET9CPLingInfo->Private.SPrivate;
    pCommInfo = &pET9CPLingInfo->CommonInfo;

    bRangeIndex = (ET9U8)(bCharIndex * ET9_CP_ID_RANGE_SIZE);
    for(bStrokeKeyLen = 0; bStrokeKeyLen < pCommInfo->bKeyBufLen; bStrokeKeyLen++) {
        if (ET9CPSYLLABLEDELIMITER == pCommInfo->bKeyBuf[bStrokeKeyLen]) {
            break;
        }
    }

    /* if the stroke ID is out of range, it is a no-match no matter if the range is exact */
    if ((wSID < pCommInfo->pwRange[bRangeIndex]) ||
        (wSID >= pCommInfo->pwRange[bRangeIndex + 2])) {
        return ET9_CP_SID_NO_MATCH;
    }
    if (psPrivate->bIsRangeExact) {
        if (wSID < pCommInfo->pwRange[bRangeIndex + 1]) {
            /* this is an exact match */
            return ET9_CP_SID_EXACT_MATCH;
        } else {
            return ET9_CP_SID_PARTIAL_MATCH;
        }
    }

    for(b = (ET9U8)__ET9Min(ET9_CP_STROKE_MAXNODES, bStrokeKeyLen); b > 0 ; b--) {
        if ((wSID >= psPrivate->wCurNodeSIDBase[b]) && (wSID < psPrivate->wCurNodeSIDBase[b] + psPrivate->wCurTotalMatch[b])) {
            bMax = b;
            break;
        }
        psPrivate->bActStrokes[b - 1] = pCommInfo->bKeyBuf[b - 1];
    }
    /* calculate if the SID is in the range according to the information obtained from the nodes */
    for(b = bMax; ((b < __ET9Min(ET9_CP_STROKE_MAXNODES, bStrokeKeyLen)) && (ET9_CP_SID_UNKNOWN_MATCH == eMatch)); b++) {
        for(;;) {
            status = StrokeAddNodeOffset(pET9CPLingInfo, psPrivate->bActStrokes[b], (ET9U8)(b + 1));
            if ((ET9STATUS_NONE == status) && (wSID >= psPrivate->wCurNodeSIDBase[b + 1]) && (wSID < psPrivate->wCurNodeSIDBase[b + 1] + psPrivate->wCurTotalMatch[b + 1])) {
                /* this is the node we are looking for, break the loop and look for the next node from its children */
                break;
            } else if ((pCommInfo->bKeyBuf[b] == ET9CPSTROKEWILDCARD) && (psPrivate->bActStrokes[b] != ET9CPSTROKEWILDCARD) && (wSID >= psPrivate->wCurNodeSIDBase[b + 1])) {
                /* only continue when the current key is a wild card and the stroke ID is larger than the range */
                ET9Assert(psPrivate->bActStrokes[b] < ET9CPSTROKE1 + ET9_CP_STROKE_NUMOFSTROKE);
                psPrivate->bActStrokes[b]++;
            } else {
                ET9Assert(b != 0);
                /* if the status is ET9STATUS_NONE, the current bActStrokes[b] can't be wild card */
                if (pwNum && (status == ET9STATUS_NONE)) {
                    *pwNum = (ET9U16)((wSID < psPrivate->wCurNodeSIDBase[b+1])?
                             (psPrivate->wCurNodeSIDBase[b+1] - wSID):
                             (wSIDEndSave - wSID));
                }
                eMatch = ET9_CP_SID_NO_MATCH;
                break;
            }
        }
        wSIDEndSave = (ET9U16)(psPrivate->wCurNodeSIDBase[b + 1] + psPrivate->wCurTotalMatch[b + 1]);
    }
    if (eMatch != ET9_CP_SID_UNKNOWN_MATCH) {
        /* do nothing here */
    } else if (bStrokeKeyLen <= ET9_CP_STROKE_MAXNODES) {
        ET9U16 wExact;
        wExact = ET9_CP_StrokeSearchForExactMatch(pET9CPLingInfo, bStrokeKeyLen);
        if (wSID < psPrivate->wCurNodeSIDBase[bStrokeKeyLen] + wExact) {
            eMatch = ET9_CP_SID_EXACT_MATCH;
            if (pwNum && !psPrivate->wCompUnicode) {
                *pwNum = (ET9U16)(psPrivate->wCurNodeSIDBase[bStrokeKeyLen] + wExact - wSID);
            }
        } else {
            eMatch = ET9_CP_SID_PARTIAL_MATCH;
            if (pwNum && !psPrivate->wCompUnicode) {
                *pwNum = (ET9U16)(psPrivate->wCurNodeSIDBase[bStrokeKeyLen] + psPrivate->wCurTotalMatch[bStrokeKeyLen] - wSID);
            }
        }
    } else {
        /* the SID is within the node range, now searching the leaf */
        ET9U32 dwOffset, dwOffsetCounts, dwOffsetStrokes;
        ET9U16 wPartialCount, wSiblingDist, wFirstLeafSID, wLastLeafSID, wLeafSID;
        ET9BOOL bIsLeaf;
        ET9U8 bChildStrokes, bExactCount, bStrokeCount, bLeafStrokeByteCount;

        dwOffset = psPrivate->dwNodeOffsets[ET9_CP_STROKE_MAXNODES];
        ET9_CP_ReadStrokeNode(pET9CPLingInfo, dwOffset, &bIsLeaf, &bChildStrokes, &bExactCount, &wPartialCount, &wSiblingDist);

        if (wPartialCount > 0) {
            ET9Assert(bIsLeaf && 0 == bChildStrokes);
            wFirstLeafSID = (ET9U16)(psPrivate->wCurNodeSIDBase[ET9_CP_STROKE_MAXNODES] + bExactCount);
            wLastLeafSID = (ET9U16)(wFirstLeafSID + wPartialCount - 1);
            dwOffsetCounts = dwOffset + ET9_CP_STROKE_NODE_SIZE; /* set offset to leaf strokes area */
            dwOffsetStrokes = dwOffsetCounts + wPartialCount;

            if (wSID <= wLastLeafSID) { /* check if the SID is within this leaf */
                for (wLeafSID = wFirstLeafSID; wLeafSID <= wLastLeafSID; wLeafSID++) {
                    bStrokeCount = ET9_CP_LdbReadByte(pET9CPLingInfo, dwOffsetCounts++); /* leaf stroke count */
                    bLeafStrokeByteCount = (ET9U8)((bStrokeCount * 3 + 7) / 8);
                    if (wLeafSID == wSID) { /* found the SID */
                        if (bStrokeCount + ET9_CP_STROKE_MAXNODES >= bStrokeKeyLen) {
                            ET9U8 *pbMask, *pbPack, bActStrokeByteCount, bLeafStrokeByte, i;
                            /* setup for comparing with active keys */
                            pbMask = psPrivate->bStrokeMask;
                            pbPack = psPrivate->bStrokePack;
                            ET9Assert(bStrokeKeyLen > ET9_CP_STROKE_MAXNODES);
                            bActStrokeByteCount = (ET9U8)(((bStrokeKeyLen - ET9_CP_STROKE_MAXNODES) * 3 + 7) / 8);
                            /* compare strokes for this SID with the current stroke inputs */
                            for (i = 0; i < bActStrokeByteCount; i++) {
                                bLeafStrokeByte = ET9_CP_LdbReadByte(pET9CPLingInfo, dwOffsetStrokes + i);
                                if ((bLeafStrokeByte & pbMask[i]) != pbPack[i]) {
                                    eMatch = ET9_CP_SID_NO_MATCH;
                                    break;
                                }
                            }
                            if (ET9_CP_SID_UNKNOWN_MATCH == eMatch) {
                                if (bStrokeCount + ET9_CP_STROKE_MAXNODES == bStrokeKeyLen) {
                                    eMatch = ET9_CP_SID_EXACT_MATCH;
                                }
                                else {
                                    eMatch = ET9_CP_SID_PARTIAL_MATCH;
                                }
                            }
                        }
                        break;
                    }
                    dwOffsetStrokes += bLeafStrokeByteCount; /* increase stroke offset to next char */
                } /* END for each SID in leaf */
            } /* END if SID is within the leaf */
        }
    } /* END if stroke keys > ET9_CP_STROKE_MAXNODES, search leaf */

    /* the following condition is true when there are more than ET9_CP_STROKE_MAXNODES strokes and
       there is at least on wild card not in the first ET9_CP_STROKE_MAXNODES strokes and
       the character to be tested has less number of strokes than that in the stroke buffer */
    if (eMatch == ET9_CP_SID_UNKNOWN_MATCH) {
        eMatch = ET9_CP_SID_NO_MATCH;
    }
    return eMatch;
}

/*-----------------------------------------------------------------------------------
 * Description:
 *      The function fill in the stroke sequence in the key buffer for an SID
 *
 * input:   pET9CPLingInfo  a pointer pointing to the ET9CPLingInfo structure
 *          wSID            The SID to be tested, it is guarenteed that this is a valid ID
 *          pbBuf           The stroke buffer
 *          bLen            the maximum number of stroke allowed in the buffer,
 *                          0 means only the first key is needed
 * return:  the actual number of strokes
 *
 * NOTE:    The function modifies bActStrokes if bLen is 0.  The current data cache
 *          pointer is changed too.
 -----------------------------------------------------------------------------------*/
ET9U8 ET9FARCALL ET9_CP_StrokeLookup(ET9CPLingInfo *pET9CPLingInfo, ET9U16 wSID, ET9U8 *pbBuf, ET9U8 bLen)
{
    ET9_CP_SPrivate *psPrivate;
    ET9U8 b;
    ET9U8 bFound = 0;
    ET9STATUS status;
    ET9U16 wExact = 0;

    psPrivate = &pET9CPLingInfo->Private.SPrivate;

#ifdef ET9_DEBUG
    {
        ET9U16 wSIDCount;
        ET9Assert(pET9CPLingInfo->CommonInfo.sOffsets.dwSIDToPIDOffset > 0);
        wSIDCount = ET9_CP_LdbReadWord(pET9CPLingInfo, pET9CPLingInfo->CommonInfo.sOffsets.dwSIDToPIDOffset); /* read total SID count in the table */
        ET9Assert(wSID < wSIDCount);
    }
#endif

    for(b = 0; b < ET9_CP_STROKE_MAXNODES; b++) {
        psPrivate->bActStrokes[b] = ET9CPSTROKEWILDCARD;
    }
    /* calculate if the SID is in the range according to the information obtained from the nodes */
    for(b = 0; (b < ET9_CP_STROKE_MAXNODES) && !bFound; b++) {
        /* the for loop is to get the right stroke for the current node */
        for(;;) {
            status = StrokeAddNodeOffset(pET9CPLingInfo, psPrivate->bActStrokes[b], (ET9U8)(b + 1));
            if ((ET9STATUS_NONE == status) && (wSID >= psPrivate->wCurNodeSIDBase[b + 1]) && (wSID < psPrivate->wCurNodeSIDBase[b + 1] + psPrivate->wCurTotalMatch[b+1])) {
                /* if the stroke ID is in the range, test if it is an exact match */
                if ((bLen == b + 1) ||
                    (wSID < psPrivate->wCurNodeSIDBase[b + 1] +
                            (wExact = ET9_CP_StrokeSearchForExactMatch(pET9CPLingInfo, (ET9U8)(b + 1))))
                   )
                    /* if the buffer length is not long enough to hold the sequence, treat it the same as exact match */
                {
                    /* exact match */
                    bLen = (ET9U8)(b + 1);
                    bFound = 1;
                }
                /* this is the node we are looking for, break the do - while loop and look for the next node from its children */
                break;
            } else {
                /* the stroke ID is guarenteed to be valid, it has to fall into one of the ranges */
                ET9Assert(psPrivate->bActStrokes[b] < ET9CPSTROKE1 + ET9_CP_STROKE_NUMOFSTROKE);
                psPrivate->bActStrokes[b]++;
            }
        }
        if (bLen == 0) {
            *pbBuf = psPrivate->bActStrokes[0];
            return 1;
        }
    }

    if (!bFound) { /* the SID is within the node range, now searching the leaf */
        ET9U32 dwOffset, dwOffsetCounts, dwOffsetStrokes;
        ET9UINT nShiftCount;
        ET9U16 wPartialCount, wSiblingDist, wFirstLeafSID, wLastLeafSID, wLeafSID, wSum;
        ET9BOOL bIsLeaf;
        ET9U8 bChildStrokes, bExactCount, bStrokeCount, bLeafStrokeByteCount, bLeafStrokeByte;

        dwOffset = psPrivate->dwNodeOffsets[ET9_CP_STROKE_MAXNODES];
        ET9_CP_ReadStrokeNode(pET9CPLingInfo, dwOffset, &bIsLeaf, &bChildStrokes, &bExactCount, &wPartialCount, &wSiblingDist);
        if (wPartialCount > 0) {
            ET9Assert(bIsLeaf && 0 == bChildStrokes);
            wFirstLeafSID = (ET9U16)(psPrivate->wCurNodeSIDBase[ET9_CP_STROKE_MAXNODES] + bExactCount);
            wLastLeafSID = (ET9U16)(wFirstLeafSID + wPartialCount - 1);
            dwOffsetCounts = dwOffset + ET9_CP_STROKE_NODE_SIZE; /* set offset to leaf strokes area */
            dwOffsetStrokes = dwOffsetCounts + wPartialCount;

            for (wLeafSID = wFirstLeafSID; wLeafSID <= wLastLeafSID; wLeafSID++) {
                bStrokeCount = ET9_CP_LdbReadByte(pET9CPLingInfo, dwOffsetCounts++); /* leaf stroke count */
                bLeafStrokeByteCount = (ET9U8)((bStrokeCount * 3 + 7) / 8);
                if (wLeafSID == wSID) {
                    bStrokeCount += ET9_CP_STROKE_MAXNODES; /* full stroke count */
                    bLen = (ET9U8)__ET9Min(bStrokeCount, bLen); /* avoid warning */
                    break;
                }
                dwOffsetStrokes += bLeafStrokeByteCount; /* increase stroke offset to next char */
            }
            ET9Assert(wLeafSID <= wLastLeafSID); /* should have found wSID within leaf */
            /* read the strokes from the leaf */
            for(b = ET9_CP_STROKE_MAXNODES, nShiftCount = 0, wSum = 0; b < bLen; b++) {
                if (nShiftCount < 3) {
                    bLeafStrokeByte = ET9_CP_LdbReadByte(pET9CPLingInfo, dwOffsetStrokes++);
                    wSum = (ET9U16)(wSum << 8);
                    wSum |= bLeafStrokeByte;
                    nShiftCount += 5;
                } else {
                    nShiftCount -= 3;
                }
                pbBuf[b] = (ET9U8)((wSum >> nShiftCount) & 0x07);
            }
        }
    }
    /* writing the first four strokes, the remaining part has been written */
    for (b = 0; (b < ET9_CP_STROKE_MAXNODES) && (b < bLen); b++) {
        pbBuf[b] = psPrivate->bActStrokes[b];
    }
    return bLen;
}

/** Enables component entry in XT9 Chinese Stroke input mode.
 * See ::ET9CPClearComponent
 *
 *      @param pET9CPLingInfo     pointer to chinese information structure
 *
 *      @return ET9STATUS_NONE     succeeded.
 *      @return ET9STATUS_NO_INIT  pET9CPLingInfo is not properly initialized.
 */
ET9STATUS ET9FARCALL ET9CPSetComponent(ET9CPLingInfo *pET9CPLingInfo)
{
    ET9_CP_CHECK_LINGINFO(pET9CPLingInfo);

    if (ET9CPIsComponentActive(pET9CPLingInfo)) {
        return ET9STATUS_NONE;
    }

    /* clear spellbuf if the current mode is stroke and there is either an active stroke sequence or a component */
    if ( ET9CPIsModeStroke(pET9CPLingInfo) ) {
        ET9_CP_INVALIDATE_BUILD_CLEAR_CACHE(pET9CPLingInfo);
    }
    pET9CPLingInfo->bState |= (ET9U8)ET9_CP_STATE_COMPONENT;
    return ET9STATUS_NONE;
}

/** Disables component entry in XT9 Chinese Stroke input mode.
 *
 *  See ::ET9CPSetComponent
 *
 *  @param pET9CPLingInfo     pointer to chinese information structure
 *
 *  @return ET9STATUS_NONE     succeeded.
 *  @return ET9STATUS_NO_INIT  pET9CPLingInfo is not properly initialized.
 */
ET9STATUS ET9FARCALL ET9CPClearComponent(ET9CPLingInfo *pET9CPLingInfo)
{
    ET9_CP_CHECK_LINGINFO(pET9CPLingInfo);

    if (ET9CPIsComponentActive(pET9CPLingInfo) == 0) {
        return ET9STATUS_NONE;
    }
    /* clear spellbuf if the current mode is stroke and there is either an active stroke sequence or a component */
    if (ET9CPIsModeStroke(pET9CPLingInfo)) {
        ET9_CP_INVALIDATE_BUILD_CLEAR_CACHE(pET9CPLingInfo);
    }
    pET9CPLingInfo->bState &= (ET9U8)~ET9_CP_STATE_COMPONENT;
    return ET9STATUS_NONE;
}

/** Retrieves the stroke order for any Chinese character or character component.
 *
 *  Character strokes are grouped into five categories and represented by
 *  the digits 1 to 5. For more information on how XT9 Chinese represents stroke categories, see ::ET9CPSYMB
 *
 * @param pET9CPLingInfo     pointer to chinese information structure.
 * @param sChar              Unicode value of Chinese character or component whose stroke order will be retrieved.
 * @param pbBuf              Pointer to the buffer where the returned strokes are stored.
 * @param pbLen              (in, out)Pointer to the buffer size of pbBuf in bytes. The value becomes the actual number of strokes returned upon exit.
 * @param bAltIndex          0-based index to the alternate stroke sequence.
 *
 * @return ET9STATUS_NONE           Success
 * @return ET9STATUS_NO_INIT        pET9CPLingInfo is not properly initialized
 * @return ET9STATUS_INVALID_MODE   Not stroke mode
 * @return ET9STATUS_BAD_PARAM      some argument pointer is NULL or buffer size is 0
 * @return ET9STATUS_WORD_NOT_FOUND sChar is not in LDB
 */
ET9STATUS ET9FARCALL ET9CPGetCharStrokes(ET9CPLingInfo *pET9CPLingInfo, ET9SYMB sChar, ET9U8 *pbBuf, ET9U8 *pbLen, ET9U8 bAltIndex)
{
    ET9U16 pwSID[ET9_CP_MAX_ALT_SYLLABLE], wPID;
    ET9U8 bAltCount;

    ET9_CP_CHECK_LINGINFO(pET9CPLingInfo);
    if (!pbBuf || !pbLen || (!(*pbLen))) {
        return ET9STATUS_BAD_PARAM;
    }

    if (!ET9CPIsModeStroke(pET9CPLingInfo)) {
        return ET9STATUS_INVALID_MODE;
    }

    wPID = ET9_CP_UnicodeToPID(pET9CPLingInfo, sChar, 0);
    if (wPID == ET9_CP_NOMATCH) {
        return ET9STATUS_WORD_NOT_FOUND;
    }
    bAltCount = ET9_CP_LookupID(pET9CPLingInfo, pwSID, wPID, (ET9U8)ET9_CP_MAX_ALT_SYLLABLE, ET9_CP_Lookup_PIDToSID);
    if (bAltIndex >= bAltCount) {
        return ET9STATUS_WORD_NOT_FOUND;
    }
    *pbLen = ET9_CP_StrokeLookup(pET9CPLingInfo, pwSID[bAltIndex], pbBuf, *pbLen);

    return ET9STATUS_NONE;
} /* end of ET9CPGetCharStrokes() */

void ET9FARCALL ET9_CP_StrokeCountChars(const ET9U8 *pbKeys, ET9UINT nLen, ET9U8 *pbNumChars, ET9U8 *pbNumStrokes1stChar, ET9U8 *pbLastDelPos)
{
    ET9UINT n;
    ET9U8 bNumChars = 0;
    ET9UINT nLastDelPos = 0;
    ET9Assert(pbKeys && pbNumChars && pbLastDelPos && pbNumStrokes1stChar);

    *pbNumStrokes1stChar = 0;
    for(n = 0; n < nLen; n++) {
        if (pbKeys[n] == ET9CPSYLLABLEDELIMITER) {
            bNumChars++;
            nLastDelPos = n + 1;
            if (*pbNumStrokes1stChar == 0) {
                *pbNumStrokes1stChar = (ET9U8)n;
            }
        }
    }

    if (*pbNumStrokes1stChar == 0) {
        *pbNumStrokes1stChar = (ET9U8)nLen;
    }
    if (nLen > 0) {
        bNumChars++;
    }
    *pbNumChars = bNumChars;
    *pbLastDelPos = (ET9U8)nLastDelPos;
}


void ET9FARCALL ET9_CP_SetTrailingSIDRanges(ET9CPLingInfo *pET9CPLingInfo, ET9U8 *pbKeyBuf, ET9U8 bNumStrokes1stChar, ET9U8 bKeyLen)
{
    ET9U8 b;

    pET9CPLingInfo->CommonInfo.bKeyBufLen = 0;
    pET9CPLingInfo->CommonInfo.bSylCount = 1;

    for(b = (ET9U8)(bNumStrokes1stChar + 1); b < bKeyLen; b++) {
        ET9U8 bKey = pbKeyBuf[b];
        if (bKey == ET9CPSYLLABLEDELIMITER) {
            pET9CPLingInfo->CommonInfo.bSylCount++;
            ET9_CP_StrokeFindRange(pET9CPLingInfo, 1);
            pET9CPLingInfo->CommonInfo.bKeyBufLen = 0;
        }
        else {
            ET9_CP_StrokeSetupKey(pET9CPLingInfo, bKey);
        }
    }
    /* setup SID range for the char after the last delimiter */
    if (bNumStrokes1stChar < bKeyLen && pbKeyBuf[bKeyLen - 1] != ET9CPSYLLABLEDELIMITER) {
        pET9CPLingInfo->CommonInfo.bSylCount++;
        ET9_CP_StrokeFindRange(pET9CPLingInfo, 1);
        pET9CPLingInfo->CommonInfo.bKeyBufLen = 0;
    }
}

#endif /* #ifdef ET9CP_DISABLE_STROKE */

/* ----------------------------------< eof >--------------------------------- */
