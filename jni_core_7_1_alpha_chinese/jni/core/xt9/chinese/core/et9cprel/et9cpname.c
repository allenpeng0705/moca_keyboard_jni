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
;**     FileName: et9cpname.c                                                 **
;**                                                                           **
;**  Description: Chinese Phrase Text Input name input module.                **
;**               Conforming to the development version of Chinese XT9.       **
;**                                                                           **
;*******************************************************************************
;******************************************************************************/

#include "et9api.h"
#include "et9cpsys.h"
#include "et9cpldb.h"
#include "et9cpmisc.h"
#include "et9cpspel.h"
#include "et9cpname.h"
#include "et9cppbuf.h"

/* Return   : name character count in the table */
static ET9U16 ET9LOCALCALL ET9_CP_GetNameTableHeader(ET9CPLingInfo *pET9CPLingInfo,
                                                ET9UINT fIsSID,
                                                ET9U32 *pdwIDOffset,
                                                ET9U32 *pdwFreqOffset)
{
    ET9U32 dwOffset;
    ET9_CP_CommonInfo *pCommon;
    ET9U16 wNameCharCount;

    pCommon = &(pET9CPLingInfo->CommonInfo);
    ET9Assert(pCommon->sOffsets.dwNameTableOffset);

    /* get name char count */
    dwOffset = pCommon->sOffsets.dwNameTableOffset;
    wNameCharCount = ET9_CP_LdbReadWord(pET9CPLingInfo, dwOffset);

    /* determine the section offsets according to encoding */
    if (fIsSID && ET9_CP_LdbHasPhonetic(pET9CPLingInfo)) {
        ET9Assert(ET9_CP_LdbHasStroke(pET9CPLingInfo));
        /* SID section is after PID/BID section */
        *pdwIDOffset = dwOffset + 2 /* sizeof(ET9U16) */ + (ET9U32)wNameCharCount * 3; /* sizeof(ET9U16) + sizeof(ET9U8) */
    }
    else {
        /* PID/BID section or SID section in Stroke-only LDB */
        *pdwIDOffset = dwOffset + 2; /* sizeof(ET9U16) */
    }
    *pdwFreqOffset = *pdwIDOffset + wNameCharCount * 2; /* sizeof(ET9U16) */
    return wNameCharCount;
} /* END ET9_CP_GetNameTableHeader */

/* init all bookmarks: set all to empty but the moving one and 1st one to
                       the beginning of the table */
void ET9FARCALL ET9_CP_InitNameTableBookmarks(ET9CPLingInfo *pET9CPLingInfo,
                                           ET9_CP_NameTableBookmark *pBookmarks,
                                           ET9UINT nCount,
                                           ET9UINT fIsSID)
{
    ET9UINT i;
    ET9U32 dwIDOffset, dwFreqOffset;
    ET9U16 wNameCharCount, wIDInc, w;

    wNameCharCount = ET9_CP_GetNameTableHeader(pET9CPLingInfo, fIsSID, &dwIDOffset, &dwFreqOffset);
    ET9Assert(nCount > 1);
    /* initializing the first bookmark, which is dynamically updated during the search */
    pBookmarks->wOffset = 0;
    pBookmarks->wID = 0;
    /* initializing the remaining bookmarks, which are fixed */
    nCount--;
    wIDInc = (ET9U16)(wNameCharCount / nCount);
    pBookmarks++;
    for (i = 0, w = 0; i < nCount; i++, pBookmarks++) {
        pBookmarks->wOffset = w;
        if (i == 0) {
            pBookmarks->wID = 0;
        } else {
            pBookmarks->wID = ET9_CP_LdbReadWord(pET9CPLingInfo, dwIDOffset + w * 2); /* sizeof(ET9U16) */
        }
        w = (ET9U16)(w + wIDInc);
    }
} /* END ET9_CP_InitNameTableBookmarks */

/*
    Input : pBookmarks = array of bookmarks
            nBookmarkCount = bookmark count in the array
            wRangeStart = start ID of the current search range

    Return : pointer to the usable bookmark.
*/
static ET9_CP_NameTableBookmark* ET9LOCALCALL ET9_CP_FindNameTableBookmark(ET9_CP_NameTableBookmark *pBookmarks,
                                                                     ET9UINT nBookmarkCount,
                                                                     ET9U16 wRangeStart)
{
    ET9_CP_NameTableBookmark *pBm;
    ET9UINT i;
    if (!pBookmarks) {
        return 0;
    }
    pBm = pBookmarks;
    for (pBm++, i = 1; i < nBookmarkCount; i++, pBm++) {
        if (pBm->wID > wRangeStart) {
           /* hit a bookmark past the search range start,
           previous bookmark is closest */
            break;
        }
    }
    pBm--;
    ET9Assert(pBm->wID <= wRangeStart);
    if ((pBookmarks[0].wID < pBm->wID) ||
        (pBookmarks[0].wID > wRangeStart)) {
        /* dynamic bookmark is further away than the closest fixed bookmark,
           use the closest fixed bookmark
           also set the dynamic bookmark to the better one. */
        pBookmarks[0] = *pBm;
        return pBm;
    }
    else {
        return pBookmarks;
    }
} /* END ET9_CP_FindNameTableBookmark */

/* Function : ET9_CP_FindNameMatch
 *            Find name characters from the name table that matches the given
 *            criteria, which include a PID/SID range and optional tone info.
 *            The search will be guided by an array of bookmarks that help
 *            starting a search.
 *            When the search reaches a data that is beyond the given criteria,
 *            it'll pause and bookmark the data as the next criteria's start.
 *
 * Input    : fIsSID = 1 if encoding is SID
 *            bNeedPartialSyl = 1 if need partial syl, 0 otherwise
 *            pBookmarks = array of bookmarks where this search can start.
 *            pbTones = tone info, part of the criteria
 *            fFindPhrase = 1 : find and store all matches into phrase buffer
 *                              (when finding phrase match)
 *                          0 : return when found any match, doesn't store into phrase buffer
 *                              (when finding spell match, 1 match is enough)
 *            pBookmarks = array of bookmarks where next search can start.
 * Return   : max frequency of the characters that match the criteria. */
ET9U8 ET9FARCALL ET9_CP_FindNameMatch(
    ET9CPLingInfo *pET9CPLingInfo,
    ET9UINT fIsSID,
    ET9BOOL bNeedPartialSyl,
    ET9_CP_NameTableBookmark *pBookmarks,
    ET9U8 *pbTones,
    ET9U8 fFindPhrase)
{
    ET9_CP_CommonInfo *pCommon = &(pET9CPLingInfo->CommonInfo);
    ET9_CP_PhraseBuf *pMainPhraseBuf = ET9_CP_GetMainPhraseBuf(pET9CPLingInfo);
    ET9_CP_NameTableBookmark *pBm;
    ET9U32 dwIDOffset, dwFreqOffset;
    ET9U8 bMaxFreq = 0;
    ET9UINT i;
    ET9U16 wNameCharCount, wOffset, wID, wExactStart, wPartialStart, wPartialEnd;
    ET9U8 bTone, bFreq;

    if (pCommon->bSylCount > 1) {
        return 0;
    }
    wNameCharCount = ET9_CP_GetNameTableHeader(pET9CPLingInfo, fIsSID,
                                             &dwIDOffset, &dwFreqOffset);
    ET9Assert(0 == (pCommon->pbRangeEnd[0] % ET9_CP_ID_RANGE_SIZE));
    for (i = 0; i < pCommon->pbRangeEnd[0]; i += ET9_CP_ID_RANGE_SIZE) {
        /* single syllable ==> each range has ET9_CP_PART_ID_RANGE_NUM */
        wExactStart = pCommon->pwRange[i];
        wPartialStart = pCommon->pwRange[i + 1];
        wPartialEnd = pCommon->pwRange[i + 2];
        /* find a suitable bookmark to start the search */
        wOffset = 0;
        pBm = ET9_CP_FindNameTableBookmark(pBookmarks,
                                         ET9_CP_MAX_NAMETABLE_BOOKMARK,
                                         wExactStart);
        if (pBm) {
            wOffset = pBm->wOffset;
        }
        for (; wOffset < wNameCharCount; wOffset++) {
            ET9U8 bCurrentCharExact;
            wID = ET9_CP_LdbReadWord(pET9CPLingInfo, dwIDOffset + wOffset * 2); /* sizeof(ET9U16) */
            if (wExactStart <= wID) {
                if (wID < wPartialEnd) {
                    bCurrentCharExact = 0;
                    /* it is a match */
                    if (wID < wPartialStart) {
                        /* exact match, do tone filtering if in Phonetic */
                        if (pbTones && pbTones[0]) { /* must be single syllable */
                            bTone = ET9_CP_LookupTone(pET9CPLingInfo, wID);
                            if (!(bTone & pbTones[0])) {
                                continue; /* tone mismatch */
                            }
                        }
                        bCurrentCharExact = 1;
                    }
                    else if (!bNeedPartialSyl || pbTones && pbTones[0]) {
                        continue; /* filter out partial match if not needed */
                    }/* else: partial match, do nothing more */
                    ET9Assert((wID < wPartialStart) || !pbTones || (0 == pbTones[0]));
                    bFreq = ET9_CP_LdbReadByte(pET9CPLingInfo, dwFreqOffset + wOffset);
                    if (bFreq > bMaxFreq)
                        bMaxFreq = bFreq;
                    if (fFindPhrase) {
                        ET9_CP_Spell * pSpell = &pET9CPLingInfo->CommonInfo.SpellData.sSpell;
                        ET9U16 wFreq;
                        ET9UINT fSurpress;
                        
                        ET9Assert(0 == pSpell->bLen || ET9CPIsModePhonetic(pET9CPLingInfo));

                        /* add name char to phrase buf */
                        wFreq = ET9_CP_EncodeFreq(pET9CPLingInfo, (ET9SYMB*)&wID, 1, 
                                                  (ET9U16)((ET9U16)bFreq | (0xFF00 & ~ET9_CP_FREQ_MASK_ALL)),
                                                  (ET9U8)bCurrentCharExact, 1, 0, &fSurpress);
                        if (!fSurpress) {
                            ET9_CP_AddPhraseToBuf(pET9CPLingInfo,
                                                  pMainPhraseBuf,
                                                  (ET9SYMB*)&wID,
                                                  1,
                                                  pSpell->pbChars,
                                                  pSpell->bLen,
                                                  fIsSID? ET9_CP_IDEncode_SID: ET9_CP_IDEncode_PID,
                                                  wFreq);
                        }
                    }
                }
                else {
                    /* beyond range, update dynamic bookmark for next search */
                    if (pBookmarks) {
                        pBookmarks[0].wID = wPartialEnd;
                        pBookmarks[0].wOffset = wOffset;
                    }
                    break;
                }
            }
        } /* END for each ID in name table */
    } /* END for each range of 1st syllable */
    return bMaxFreq;
} /* END ET9_CP_FindNameMatch */

/*---------------------------------------------------------------------------
 *
 * Function : ET9_CP_GetCommonNameChar
 *            This function retrieves common name characters from name table
 *            and store them into phrase buffer.
 *
 * Input    : pET9CPLingInfo = Pointer to Chinese XT9 LingInfo structure.
 *            fIsSID = if encoding is SID
 *
 * Return   : none
 *
 *---------------------------------------------------------------------------*/
void ET9FARCALL ET9_CP_GetCommonNameChar(ET9CPLingInfo *pET9CPLingInfo,
                                      ET9UINT fIsSID)
{
    ET9_CP_PhraseBuf *pMainPhraseBuf = ET9_CP_GetMainPhraseBuf(pET9CPLingInfo);
    ET9U32 dwIDOffset, dwFreqOffset;
    ET9UINT i;
    ET9U16 wNameCharCount, wID;
    ET9U8 bFreq;

    if (!ET9CPIsCommonCharActive(pET9CPLingInfo)) {
        return;
    }

    wNameCharCount = ET9_CP_GetNameTableHeader(pET9CPLingInfo, fIsSID,
                                             &dwIDOffset, &dwFreqOffset);
    for (i = 0; i < wNameCharCount; i++) {
        ET9U16 wFreq;

        wID = ET9_CP_LdbReadWord(pET9CPLingInfo, dwIDOffset + i * 2); /* sizeof(ET9U16) */
        bFreq = ET9_CP_LdbReadByte(pET9CPLingInfo, dwFreqOffset + i);
        wFreq = ET9_CP_EncodeFreq(pET9CPLingInfo, NULL, 1, bFreq, 0, 0, 0, NULL);
        ET9_CP_AddPhraseToBuf(pET9CPLingInfo, pMainPhraseBuf,
                             (ET9SYMB*)&wID, 1, NULL, 0, fIsSID? ET9_CP_IDEncode_SID: ET9_CP_IDEncode_PID, wFreq);
    }
} /* END ET9_CP_GetCommonNameChar */

/* Function : ET9_CP_GetNameCharFreq
 * Input    : pET9CPLingInfo = Chinese XT9 LingInfo
 *            fIsSID = if the type of encoding of wID is SID
 *            wID = PID/BID/SID
 * Output   : none
 * Return   : frequency if found, 0 otherwise
 */
ET9U8 ET9FARCALL ET9_CP_GetNameCharFreq(ET9CPLingInfo *pET9CPLingInfo,
                                    ET9UINT fIsSID,
                                    ET9U16 wID)
{
    ET9U32 dwIDOffset, dwFreqOffset;
    ET9U16 wNameCharCount, wStart, wEnd, wMid, wMidID;
    ET9U8 bFreq;
    wNameCharCount = ET9_CP_GetNameTableHeader(pET9CPLingInfo, fIsSID,
                                             &dwIDOffset, &dwFreqOffset);
    bFreq = 0;
    /* binary search : wStart, wEnd, wMid are 1-based indices */
    for (wStart = 1, wEnd = wNameCharCount; wStart <= wEnd; ) {
        wMid = (ET9U16)((wStart + wEnd) / 2);
        wMidID = ET9_CP_LdbReadWord(pET9CPLingInfo, dwIDOffset + (wMid - 1) * 2); /* sizeof(ET9U16) */
        if (wMidID > wID) {
            wEnd = (ET9U16)(wMid - 1);
        }
        else if (wMidID < wID) {
            wStart = (ET9U16)(wMid + 1);
        }
        else {
            bFreq = ET9_CP_LdbReadByte(pET9CPLingInfo, dwFreqOffset + (wMid - 1));
            break;
        }
    }
    ET9Assert(wStart <= wNameCharCount + 1); /* when the loop ends, wStart > wEnd */
    return bFreq;
} /* END ET9_CP_GetNameCharFreq */

/** Enables the Chinese Name Input feature.
 * See ::ET9CPClearNameInput
 *
 *      @param pET9CPLingInfo     pointer to chinese information structure
 *
 *      @return ET9STATUS_NONE     succeeded.
 *      @return ET9STATUS_NO_INIT  pET9CPLingInfo is not properly initialized.
 */
ET9STATUS ET9FARCALL ET9CPSetNameInput(ET9CPLingInfo *pET9CPLingInfo)
{
    ET9_CP_CHECK_LINGINFO(pET9CPLingInfo);
    if (!ET9CPIsNameInputActive(pET9CPLingInfo)) {
        ET9_CP_INVALIDATE_BUILD_CLEAR_CACHE(pET9CPLingInfo);
        pET9CPLingInfo->bState |= (ET9U8)ET9_CP_STATE_NAME_INPUT;
    }
    return ET9STATUS_NONE;
} /* END ET9CPSetNameInput() */

/** Disables the Chinese Name Input feature.
 * See ::ET9CPSetNameInput
 *
 *      @param pET9CPLingInfo     pointer to chinese information structure
 *
 *      @return ET9STATUS_NONE     succeeded.
 *      @return ET9STATUS_NO_INIT  pET9CPLingInfo is not properly initialized.
 */
ET9STATUS ET9FARCALL ET9CPClearNameInput(ET9CPLingInfo *pET9CPLingInfo)
{
    ET9_CP_CHECK_LINGINFO(pET9CPLingInfo);
    if (ET9CPIsNameInputActive(pET9CPLingInfo)) {
        ET9_CP_INVALIDATE_BUILD_CLEAR_CACHE(pET9CPLingInfo);
        pET9CPLingInfo->bState = (ET9U8)(pET9CPLingInfo->bState & (~ET9_CP_STATE_NAME_INPUT));
    }
    return ET9STATUS_NONE;
} /* END ET9CPClearNameInput() */

/* ----------------------------------< eof >--------------------------------- */
