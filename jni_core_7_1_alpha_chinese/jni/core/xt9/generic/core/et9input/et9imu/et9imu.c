/*******************************************************************************
;*******************************************************************************
;**                                                                           **
;**                  COPYRIGHT 2001-2011 NUANCE COMMUNICATIONS                **
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
;**     FileName: et9imu.c                                                    **
;**                                                                           **
;**  Description: Generic input module functionality                          **
;**                                                                           **
;*******************************************************************************
;******************************************************************************/

/*! \addtogroup et9imu Functions for XT9 Generic Module
* Generic input for generic XT9.
* @{
*/

#include "et9imu.h"
#include "et9misc.h"
#include "et9sym.h"

#ifdef ET9_ALPHABETIC_MODULE
#include "et9aimu.h"
#endif /* ET9_ALPHABETIC_MODULE */


#ifdef ET9_DEBUGLOG4
#ifdef _WIN32
#pragma message ("*** ET9_DEBUGLOG4 ACTIVATED ***")
#endif
#include <stdio.h>
#include <string.h>
#define WLOG4(q) { if (pLogFile4 == NULL) { pLogFile4 = fopen("zzzET9IMU.txt", "w"); } { q fflush(pLogFile4); } }
static FILE *pLogFile4 = NULL;
#else
#define WLOG4(q)
#endif


/*******************************************************************************
 **
 **          G L O B A L S   A N D   L O C A L   S T A T I C S
 **
 ** ET9 does not make use of any dynamic global or local static variables!!
 ** It is acceptable to make use of constant globals or local statics.
 ** If you need persistent dynamic memory in the ET9 core, it should be
 ** allocated in the ET9AWLingPrivate data structure and fogged through the definitions
 ** found in the et9asystm.h file.
 **
 ******************************************************************************/

#define ET9MAGICSTR_FIRSTSET 5


#if 0
/*---------------------------------------------------------------------------*/
/** \internal
 * This function locks current symb info.
 *
 * @param pWordSymbInfo             Pointer to word symbol info structure.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

static void ET9LOCALCALL __LockSymbInfo(ET9WordSymbInfo     * const pWordSymbInfo)

{
    ET9UINT nCount;
    ET9SymbInfo *pSymbInfo;

    if (!pWordSymbInfo || pWordSymbInfo->wInitOK != ET9GOODSETUP) {
        return;
    }

    pSymbInfo = pWordSymbInfo->SymbsInfo;
    for (nCount = pWordSymbInfo->bNumSymbs; nCount; --nCount, ++pSymbInfo) {

        pSymbInfo->sLockedSymb = pSymbInfo->eShiftState ? pSymbInfo->DataPerBaseSym[0].sUpperCaseChar[0] : pSymbInfo->DataPerBaseSym[0].sChar[0];

        pSymbInfo->bLocked = 1;
    }
}
#endif

/*---------------------------------------------------------------------------*/
/** \internal
 * This function appends an input tail.
 *
 * @param pWordSymbInfo             Pointer to word symbol info structure.
 * @param psSymbs                   Symbs to explicify.
 * @param nSymbCount                Number of symbs.
 *
 * @return None
 */

void ET9FARCALL __ExplicifyTail(ET9WordSymbInfo   * const pWordSymbInfo,
                                ET9SYMB           * const psSymbs,
                                const ET9UINT             nSymbCount)
{
    ET9STATUS   wStatus;
    ET9UINT     nCount;
    ET9SYMB     *psSymb;

    ET9Assert(psSymbs);
    ET9Assert(pWordSymbInfo);

    WLOG4(fprintf(pLogFile4, "__ExplicifyTail\n");)

    /* valid */

    if (!pWordSymbInfo->bNumSymbs) {
    }

    /* enter using explicits */

    psSymb = psSymbs;

    for (nCount = nSymbCount; nCount; --nCount, ++psSymb) {

        wStatus = ET9AddExplicitSymb(pWordSymbInfo,
                                     *psSymb,
                                     (_ET9SymIsUpper(*psSymb, pWordSymbInfo->Private.wLocale) ? ET9SHIFT : ET9NOSHIFT),
                                     ET9_NO_ACTIVE_INDEX);

        if (wStatus) {
            WLOG4(fprintf(pLogFile4, "__ExplicifyTail, explicit symb failed\n");)
        }
    }

    /* potentially prevent catchup builds etc */

    _ET9ContentExplicified(pWordSymbInfo);
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Performs house keeping when input becomes empty.
 *
 * @param[in]     pWordSymbInfo   Pointer to the Word Symbol Data Structure (ET9WordSymbInfo), which contains information about the current input sequence.
 * @param[in]     bResetRequired  If the required should be reset or not.
 *
 * @return None
 */

static void ET9LOCALCALL __OnInputBecameEmpty(ET9WordSymbInfo * const pWordSymbInfo, const ET9BOOL bResetRequired)
{
    ET9Assert(!pWordSymbInfo->bNumSymbs);

    WLOG4(fprintf(pLogFile4, "__OnInputBecameEmpty, new input started\n");)

    pWordSymbInfo->Private.bInputRestarted = 1;

    if (bResetRequired) {

        if (pWordSymbInfo->Private.sRequiredWord.wLen) {

            WLOG4(fprintf(pLogFile4, "__OnInputBecameEmpty, clearing required word\n");)

            _ET9InitSimpleWord(&pWordSymbInfo->Private.sRequiredWord);
        }

        pWordSymbInfo->Private.bRequiredLocate = 0;
        pWordSymbInfo->Private.bRequiredVerifyInput = 0;
        pWordSymbInfo->Private.bRequiredInhibitOverride = 0;
        pWordSymbInfo->Private.bRequiredInhibitCorrection = 0;
        pWordSymbInfo->Private.bRequiredHasRegionalInfo = 0;
    }

    if (pWordSymbInfo->Private.eLastShiftState == ET9SHIFT) {
         pWordSymbInfo->Private.eLastShiftState = ET9NOSHIFT;
    }

    pWordSymbInfo->Private.eAutocapWord = ET9AUTOCAP_OFF;
    pWordSymbInfo->Private.bCompoundingDownshift = 0;

    pWordSymbInfo->Private.eLastInputEvent = ET9InputEvent_none;
    pWordSymbInfo->Private.bClearSymbEpisodeCount = 0;
}

/*---------------------------------------------------------------------------*/
/**
 * @brief Clears the most recently added value from the input sequence.
 *
 * @param[in]     pWordSymbInfo   Pointer to the Word Symbol Data Structure (ET9WordSymbInfo), which contains information about the current input sequence.
 *
 * @retval ET9STATUS_NONE                   The function call was handled successfully.
 * @retval ET9STATUS_INVALID_MEMORY         At least one pointer passed as a function argument was set to null.
 * @retval ET9STATUS_EMPTY                  There is no current input sequence (the value of pWordSymbInfo->bNumSymbs is 0).
 * @retval ET9STATUS_NEED_SELLIST_BUILD     Text has been reselected, but the selection list has not been rebuilt.
 *                                          Only returned if calling ET9ClearOneSymb does not clear all symbols of the reselected word.
 *
 * @see ET9ClearAllSymbs()
 */

ET9STATUS ET9FARCALL ET9ClearOneSymb(ET9WordSymbInfo * const pWordSymbInfo)
{
    if (!pWordSymbInfo || pWordSymbInfo->wInitOK != ET9GOODSETUP) {
        return ET9STATUS_INVALID_MEMORY;
    }

    if (!pWordSymbInfo->bNumSymbs) {
        __OnInputBecameEmpty(pWordSymbInfo, 1);
        return ET9STATUS_EMPTY;
    }

    if ((pWordSymbInfo->bNumSymbs > 1) && pWordSymbInfo->Private.bRequiredVerifyInput) {

        /* if reselecting (and not the last symb being deleted), need to build first */

        return ET9STATUS_NEED_SELLIST_BUILD;
    }

    /* potentially explicify on first delete on reselect */

    if (pWordSymbInfo->Private.bRequiredLocate) {

        ET9SimpleWord sTmpWord = pWordSymbInfo->Private.sRequiredWord;  /* needed to not get reset in explicify */

        if (_ET9HasRegionalInfo(pWordSymbInfo) || _ET9HasTraceInfo(pWordSymbInfo)) {

            _ET9ExplicifyWord(pWordSymbInfo, &sTmpWord);

            pWordSymbInfo->Private.bRequiredLocate = 0;
            pWordSymbInfo->Private.bRequiredVerifyInput = 0;
            pWordSymbInfo->Private.bRequiredInhibitOverride = 1;
            pWordSymbInfo->Private.bRequiredInhibitCorrection = 1;
            _ET9InitSimpleWord(&pWordSymbInfo->Private.sRequiredWord);
        }
        else if (_ET9HasDiscreteOnlyInfo(pWordSymbInfo) && sTmpWord.wLen > pWordSymbInfo->bNumSymbs) {

            __ExplicifyTail(pWordSymbInfo, &sTmpWord.sString[pWordSymbInfo->bNumSymbs], (sTmpWord.wLen - pWordSymbInfo->bNumSymbs));

            pWordSymbInfo->Private.bRequiredLocate = 0;
            pWordSymbInfo->Private.bRequiredVerifyInput = 0;
            pWordSymbInfo->Private.bRequiredInhibitOverride = 1;
            pWordSymbInfo->Private.bRequiredInhibitCorrection = 1;
            _ET9InitSimpleWord(&pWordSymbInfo->Private.sRequiredWord);
        }
    }

    /* if clearing all, call clear-all instead */

    {
        const ET9U16 wInputIndex = pWordSymbInfo->SymbsInfo[0].wInputIndex;

        ET9UINT nCount;
        ET9SymbInfo *pSymbInfo;

        pSymbInfo = &pWordSymbInfo->SymbsInfo[0];
        for (nCount = pWordSymbInfo->bNumSymbs; nCount; --nCount, ++pSymbInfo) {
            if (!wInputIndex || pSymbInfo->wInputIndex != wInputIndex) {
                break;
            }
        }

        if (!nCount || pWordSymbInfo->bNumSymbs == 1) {
            return ET9ClearAllSymbs(pWordSymbInfo);
        }
    }

    /* track */

    _ET9TrackInputEvents(pWordSymbInfo, ET9InputEvent_clear);

    /* clear one will actually clear more than one symbol if it was entered with one key press, for example emoticons */

    _ET9ImminentSymb(pWordSymbInfo, ET9_NO_ACTIVE_INDEX, 0);

    {
        ET9SymbInfo *pSymbInfo = &pWordSymbInfo->SymbsInfo[pWordSymbInfo->bNumSymbs - 1];

        const ET9U16 wInputIndex = pSymbInfo->wInputIndex;

        while (pWordSymbInfo->bNumSymbs > 0) {

            const ET9BOOL bMultiChar = pSymbInfo->wInputIndex ? 1 : 0;

            _ET9ClearMem((ET9U8 *)pSymbInfo, sizeof(ET9SymbInfo));
            --pWordSymbInfo->bNumSymbs;

            if (bMultiChar && pWordSymbInfo->bNumSymbs) {
                --pSymbInfo;
                if (pSymbInfo->wInputIndex != wInputIndex) {
                    break;
                }
            }
            else {
                break;
            }
        }
    }

    _ET9InvalidateSymbInfo(pWordSymbInfo);

    ET9Assert(pWordSymbInfo->bNumSymbs);

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \private
 * @brief Actually clears the entire input sequence.
 *
 * @param[in]     pWordSymbInfo        Pointer to the Word Symbol Data Structure (ET9WordSymbInfo), which contains information about the current input sequence.
 * @param[in]     bResetRequired       If the required should be reset or not.
 *
 * @see ET9ClearOneSymb()
 */

static ET9STATUS ET9LOCALCALL __ClearAllSymbs(ET9WordSymbInfo * const pWordSymbInfo, const ET9BOOL bResetRequired)
{
    if (!pWordSymbInfo || pWordSymbInfo->wInitOK != ET9GOODSETUP) {
        return ET9STATUS_INVALID_MEMORY;
    }

    if (pWordSymbInfo->bNumSymbs) {
        pWordSymbInfo->Private.peInputTracks[pWordSymbInfo->Private.bCurrInputTrackIndex] = _ET9HasTraceInfo(pWordSymbInfo) ? ET9InputTrack_trace : ET9InputTrack_tap;
        pWordSymbInfo->Private.bCurrInputTrackIndex = (pWordSymbInfo->Private.bCurrInputTrackIndex + 1) % ET9_INPUT_TRACK_SIZE;
    }

    _ET9ImminentSymb(pWordSymbInfo, ET9_NO_ACTIVE_INDEX, 0);

    _ET9ClearMem((ET9U8 *)pWordSymbInfo->SymbsInfo, (ET9MAXWORDSIZE * sizeof(ET9SymbInfo)));

    pWordSymbInfo->bNumSymbs = 0;

    _ET9InvalidateSelList(pWordSymbInfo);
    _ET9InvalidateSymbInfo(pWordSymbInfo);

    __OnInputBecameEmpty(pWordSymbInfo, bResetRequired);

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/**
 * @brief Clears the entire input sequence.
 *
 * @param[in]     pWordSymbInfo        Pointer to the Word Symbol Data Structure (ET9WordSymbInfo), which contains information about the current input sequence.
 *
 * @retval ET9STATUS_NONE              The function call was handled successfully.
 * @retval ET9STATUS_INVALID_MEMORY    At least one pointer passed as a function argument was set to null.
 *
 *
 * @see ET9ClearOneSymb()
 */

ET9STATUS ET9FARCALL ET9ClearAllSymbs(ET9WordSymbInfo * const pWordSymbInfo)
{
    return __ClearAllSymbs(pWordSymbInfo, 1);
}

/*---------------------------------------------------------------------------*/
/** \internal
 * This function tracks input actions.
 *
 * @param pWordSymbInfo             Pointer to word symbol information structure.
 * @param eInputEvent               Input event.
 *
 * @return None
 */

void ET9FARCALL _ET9TrackInputEvents(ET9WordSymbInfo * const pWordSymbInfo, const ET9InputEvent eInputEvent)
{
    if (eInputEvent == ET9InputEvent_clear && pWordSymbInfo->Private.eLastInputEvent == ET9InputEvent_add) {
        ++pWordSymbInfo->Private.bClearSymbEpisodeCount;    /* ok to roll-over to zero... */
    }

    pWordSymbInfo->Private.eLastInputEvent = eInputEvent;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * This function deletes a number of symbols anywhere in the input.
 *
 * @param pWordSymbInfo             Pointer to word symbol information structure.
 * @param bIndex                    Index of the first symbol to be deleted.
 * @param bCount                    Number of symbols to delete.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL ET9DeleteSymbs(ET9WordSymbInfo * const pWordSymbInfo,
                                    const ET9U8             bIndex,
                                    const ET9U8             bCount)
{
    ET9U8 bDstIndex;
    ET9U8 bMoveCount;
    ET9SymbInfo *pSrc;
    ET9SymbInfo *pDst;

    if (!pWordSymbInfo || pWordSymbInfo->wInitOK != ET9GOODSETUP) {
        return ET9STATUS_INVALID_MEMORY;
    }
    if (!pWordSymbInfo->bNumSymbs) {
        /* Make sure autocapping status cleared and downshifting logic disabled if no syms  */
        pWordSymbInfo->Private.eAutocapWord = ET9AUTOCAP_OFF;
        pWordSymbInfo->Private.bCompoundingDownshift = 0;
        return ET9STATUS_EMPTY;
    }
    if (bIndex >= pWordSymbInfo->bNumSymbs) {
        return ET9STATUS_BAD_PARAM;
    }
    if (bIndex + bCount > pWordSymbInfo->bNumSymbs) {
        return ET9STATUS_BAD_PARAM;
    }
    if (!bCount) {
        return ET9STATUS_NONE;
    }
    /* if reselecting (and all symbs not being deleted), need to build first */
    if ((pWordSymbInfo->bNumSymbs > bCount) && pWordSymbInfo->Private.bRequiredVerifyInput) {
        return ET9STATUS_NEED_SELLIST_BUILD;
    }

    /* copy symbs - if needed */

    pDst = &pWordSymbInfo->SymbsInfo[bIndex];
    pSrc = &pWordSymbInfo->SymbsInfo[bIndex + bCount];

    bDstIndex = bIndex;
    bMoveCount = (ET9U8)(pWordSymbInfo->bNumSymbs - (bIndex + bCount));

    for (; bMoveCount; --bMoveCount, ++pDst, ++pSrc, ++bDstIndex) {
        *pDst = *pSrc;
        _ET9InvalidateOneSymb(pWordSymbInfo, bDstIndex);
    }

    pWordSymbInfo->bNumSymbs = (ET9U8)(pWordSymbInfo->bNumSymbs - bCount);
    if (!pWordSymbInfo->bNumSymbs) {
        /* Make sure autocapping status cleared and downshifting logic disabled if no syms  */
        pWordSymbInfo->Private.eAutocapWord = ET9AUTOCAP_OFF;
        pWordSymbInfo->Private.bCompoundingDownshift = 0;
    }

    /* invalidate, especially if nothing got moved */

    _ET9InvalidateSymbInfo(pWordSymbInfo);

    /* done */

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * This function moves a number of symbols within the input.
 *
 * @param pWordSymbInfo             Pointer to word symbol information structure.
 * @param bFromIndex                Position to move from.
 * @param bToIndex                  Position to move to.
 * @param bCount                    Number of symbols to move.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL ET9MoveSymbs(ET9WordSymbInfo * const pWordSymbInfo,
                                  const ET9U8             bFromIndex,
                                  const ET9U8             bToIndex,
                                  const ET9U8             bCount)
{
    ET9U8 bIndex;
    ET9U8 bMoveCount;
    ET9SymbInfo *pSrc;
    ET9SymbInfo *pDst;
    ET9SymbInfo *pCurr;
    ET9SymbInfo sTmp;

    if (!pWordSymbInfo || pWordSymbInfo->wInitOK != ET9GOODSETUP) {
        return ET9STATUS_INVALID_MEMORY;
    }
    if (!pWordSymbInfo->bNumSymbs) {
        /* Make sure autocap status is off if no syms */
        pWordSymbInfo->Private.eAutocapWord = ET9AUTOCAP_OFF;
        /* Make sure downshifting logic disabled if no syms  */
        pWordSymbInfo->Private.bCompoundingDownshift = 0;
        return ET9STATUS_EMPTY;
    }
    if (bFromIndex >= pWordSymbInfo->bNumSymbs) {
        return ET9STATUS_BAD_PARAM;
    }
    if (bFromIndex + bCount > pWordSymbInfo->bNumSymbs) {
        return ET9STATUS_BAD_PARAM;
    }
    if (bToIndex >= pWordSymbInfo->bNumSymbs) {
        return ET9STATUS_BAD_PARAM;
    }
    if (bToIndex + bCount > pWordSymbInfo->bNumSymbs) {
        return ET9STATUS_BAD_PARAM;
    }
    if (!bCount) {
        return ET9STATUS_NONE;
    }
    if (bFromIndex == bToIndex) {
        return ET9STATUS_NONE;
    }
    /* if reselecting, need to build first */
    if (pWordSymbInfo->Private.bRequiredVerifyInput) {
        return ET9STATUS_NEED_SELLIST_BUILD;
    }

    /* assure that unlocked symbols are not moved into a locked area */

    {
        ET9U16          wLockPoint;
        ET9SymbInfo     *pSymbInfo;

        wLockPoint = pWordSymbInfo->bNumSymbs;

        if (wLockPoint) {
            for (pSymbInfo = &pWordSymbInfo->SymbsInfo[wLockPoint - 1]; wLockPoint; --wLockPoint, --pSymbInfo) {
                if (pSymbInfo->bLocked) {
                    break;
                }
            }
        }

        if (wLockPoint) {
            if (bToIndex < wLockPoint && bFromIndex + bCount > wLockPoint) {
                return ET9STATUS_ERROR;
            }
        }
    }

    /* make an overlapping move into a non overlapping one */

    pDst = &pWordSymbInfo->SymbsInfo[bToIndex];
    pSrc = &pWordSymbInfo->SymbsInfo[bFromIndex];

    if ((pDst > pSrc) && (pDst < pSrc + bCount)) {
        bMoveCount = (ET9U8)(pDst - pSrc);
        pDst = pSrc;
        pSrc += bCount;
    }
    else {
        bMoveCount = bCount;
    }

    /* moving one symbol at a time - most likely case, less complex and most memory preserving */

    if (pDst < pSrc) {
        for (; bMoveCount; --bMoveCount) {
            sTmp = *pSrc;
            for (pCurr = pSrc; pCurr > pDst; --pCurr) {
                *pCurr = *(pCurr-1);
            }
            *pDst = sTmp;
            ++pDst;
            ++pSrc;
        }
    }
    else {
        pSrc += (bMoveCount - 1);
        pDst += (bMoveCount - 1);
        for (; bMoveCount; --bMoveCount) {
            sTmp = *pSrc;
            for (pCurr = pSrc; pCurr < pDst; ++pCurr) {
                *pCurr = *(pCurr+1);
            }
            *pDst = sTmp;
            --pDst;
            --pSrc;
        }
    }

    /* invalidate all affected symbols */

    if (bFromIndex < bToIndex) {
        bIndex = bFromIndex;
    }
    else {
        bIndex = bToIndex;
    }

    for (; bIndex < pWordSymbInfo->bNumSymbs; ++bIndex) {
        _ET9InvalidateOneSymb(pWordSymbInfo, bIndex);
    }

    _ET9InvalidateSymbInfo(pWordSymbInfo);

    /* done */

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/**
 * @brief Adds an explicitly specified character to the input sequence.
 *
 * @param[in]     pWordSymbInfo             Pointer to the Word Symbol Data Structure (ET9WordSymbInfo),
 *                                          which contains information about the current input sequence.
 * @param[in]     sSymb                     Character to be added.
 * @param[in]     eShiftState               Shift state to be associated with character in sSymb. Valid values are:<br>
 *                                          <ul><li> ET9NOSHIFT if an unshifted state
 *                                              <li> ET9SHIFT if a shifted state.
 *                                              <li> ET9CAPSLOCK if a caps lock state</ul>
 * @param[in]     bCurrIndexInList          0-based index value that indicates which word in the selection list is currently selected.
 *                                          XT9 locks this word before it adds the specified input value.<br>
 *                                      If there is no current input sequence, set the value of this parameter to ET9_NO_ACTIVE_INDEX.<br>
 *                                      If the integration layer passes as an invalid argument for this parameter, XT9 will still add
 *                                      the new input, but it will not lock the word. Also, it will not return an error status.
 *
 * @retval ET9STATUS_NONE               The function call was handled successfully.
 * @retval ET9STATUS_INVALID_MEMORY     At least one pointer passed as a function argument was set to null.
 * @retval ET9STATUS_FULL               The active word is already the maximum size allowed (ET9MAXWORDSIZE).
 * @retval ET9STATUS_INVALID_TEXT       The explicitly specified character in sSymb is not valid. Examples of invalid characters include white space and unknown characters.
 *
 * @remarks Explicitly-specified characters can include special characters that are not in the core symbol tables.
 */

ET9STATUS ET9FARCALL ET9AddExplicitSymb(ET9WordSymbInfo * const  pWordSymbInfo,
                                        const ET9SYMB            sSymb,
                                        const ET9INPUTSHIFTSTATE eShiftState,
                                        const ET9U8              bCurrIndexInList)
{
    ET9SymbInfo         *pSymbInfo;
    ET9U8               bNumSymbs;
    ET9DataPerBaseSym   *pDPBSym;

    if (!pWordSymbInfo || pWordSymbInfo->wInitOK != ET9GOODSETUP) {
        return ET9STATUS_INVALID_MEMORY;
    }
    if (pWordSymbInfo->bNumSymbs >= ET9MAXWORDSIZE) {
        return ET9STATUS_FULL;
    }
    if (pWordSymbInfo->Private.bPreventWhiteSpaceInput && _ET9_IsWhiteSpace(sSymb)) {
        return ET9STATUS_INVALID_TEXT;
    }

    _ET9TrackInputEvents(pWordSymbInfo, ET9InputEvent_add);

    _ET9ImminentSymb(pWordSymbInfo, bCurrIndexInList, _ET9_IsPunctChar(sSymb));

    bNumSymbs = (ET9U8)(pWordSymbInfo->bNumSymbs + 1);      /* imminent symb can shange the num symbs count */

    if (bNumSymbs > ET9MAXWORDSIZE) {
        return ET9STATUS_FULL;
    }

    pSymbInfo = &pWordSymbInfo->SymbsInfo[bNumSymbs - 1];

    _ET9ClearMem((ET9U8*)pSymbInfo, sizeof(ET9SymbInfo));

    pDPBSym = pSymbInfo->DataPerBaseSym;

    if (_ET9SymIsUpper(sSymb, pWordSymbInfo->Private.wLocale)) {
        pDPBSym->sUpperCaseChar[0] = sSymb;
        pDPBSym->sChar[0] = _ET9SymToLower(sSymb, pWordSymbInfo->Private.wLocale);
    }
    else {
        pDPBSym->sChar[0] = sSymb;
        pDPBSym->sUpperCaseChar[0] = _ET9SymToUpper(sSymb, pWordSymbInfo->Private.wLocale);
    }

    {
        const ET9SymbClass eClass = _ET9_GetSymbolClass(sSymb);

        switch (eClass)
        {
            case ET9_PunctSymbClass:
                pSymbInfo->bSymbType = ET9KTPUNCTUATION;
                break;
            case ET9_NumbrSymbClass:
                pSymbInfo->bSymbType = ET9KTNUMBER;
                break;
            case ET9_UnassSymbClass:
                pSymbInfo->bSymbType = ET9KTUNKNOWN;
                break;
            default:
                pSymbInfo->bSymbType = ET9KTLETTER;
                break;
        }
    }

    pDPBSym->bSymFreq = 3;
    pSymbInfo->bNumBaseSyms = 1;
    pDPBSym->bNumSymsToMatch = 1;
    pSymbInfo->eInputType = ET9EXPLICITSYM;
    pSymbInfo->eShiftState = eShiftState;
    pSymbInfo->bForcedLowercase = 0;
    pSymbInfo->bAmbigType = (ET9U8)ET9EXACT;
    pSymbInfo->bTraceProbability = 0;
    pSymbInfo->bTraceIndex = 0;
    pSymbInfo->bFreqsInvalidated = 1;

    pSymbInfo->wKdb1 = 0;
    pSymbInfo->wPage1 = 0;
    pSymbInfo->wKdb2 = 0;
    pSymbInfo->wPage2  = 0;

    pSymbInfo->wInputIndex = 0;
    pSymbInfo->wKeyIndex = 0;
    pSymbInfo->wTapX = ET9UNDEFINEDTAPVALUE;
    pSymbInfo->wTapY = ET9UNDEFINEDTAPVALUE;
    pSymbInfo->pKdbKey = NULL;

    _ET9InvalidateOneSymb(pWordSymbInfo, (ET9U8)(bNumSymbs - 1));

    pWordSymbInfo->Private.eLastShiftState = pSymbInfo->eShiftState;
    ++pWordSymbInfo->bNumSymbs;

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/**
 * @brief Adds an ambiguous input value to the active word, specifying all possible characters for the input along with their associated probabilities.
 * If all symbols are classified as punctuation, then the input will be treated as a punctuation key. Otherwise it will behave as a letter key.
 * If all symbols share the same probability, then the input will behave as a discrete input. Otherwise it will behave as regional input.
 *
 * @param[in]     pWordSymbInfo     Pointer to the Word Symbol Data Structure (ET9WordSymbInfo), which contains information about the current input sequence.
 * @param[in]     psSymbs           Pointer to an array of symbols that correspond to the ambiguous input value being entered. The array should not contain any whitespace characters.<br>
 *                                  Also, the integration layer should ensure that the array contains only valid Unicode character values. For more
 *                                  information which values are valid, see the description below.<br>
 *                                  If the array contains more symbols that can be fit, only the ones with the highest probability will be processed.
 *                                  The symbols in the array can be stored in whatever order you prefer.
 *                                  Symbols with the same probability will be treated as alternative symbols. When using alternatives the total number of symbols processed from the array can be higher than ET9MAXBASESYMBS.
 * @param[in]     pbSymbProbs       Pointer to an array of probabilities corresponding to the characters in the array pointed to by psSymbs.
 *                                  Probabilities are expressed as integers ranging from 1 to 255, with higher numbers indicating greater probabilities.<br>
 *                                  Nuance suggests that the sum of all probabilities in the array totals 255.
 * @param[in]     nNumSymbsInSet    Number of characters in the array pointed to by psSymbs. Valid values range from 1 and up.
 * @param[in]     eShiftState       Shift state to be associated with the characters in psSymbs. Valid values are.
 *                                  <ul><li> ET9NOSHIFT if an unshifted state
 *                                      <li> ET9SHIFT if a shifted state.
 *                                      <li> ET9CAPSLOCK if a caps lock state</ul>
 * @param[in]     bCurrIndexInList  0-based index value that indicates which word in the selection list is currently selected. XT9 locks this word before it adds the specified input value.<br>
 *                                  If there is no current input sequence, set the value of this parameter to ET9_NO_ACTIVE_INDEX.<br>
 *                                  If the integration layer passes as an invalid argument for this parameter, XT9 will still add the new input,
 *                                  but it will not lock the word. Also, it will not return an error status.
 *
 *
 * @retval ET9STATUS_NONE               Function call was handled successfully.
 * @retval ET9STATUS_INVALID_MEMORY     At least one pointer passed as a function argument was set to null.
 * @retval ET9STATUS_BAD_PARAM          One of the probabilities in pnSymbProbs is zero. Ensure all probabilities have a value ranging from 1 to 255.
 * @retval ET9STATUS_FULL               The ambiguous input value cannot be appended to the active word because the active word is already the maximum size allowed (ET9MAXWORDSIZE).
 * @retval ET9STATUS_INVALID_TEXT       At least one of the characters in psSymbs is not valid. Examples of invalid characters include white space and unknown characters.
 * @retval ET9STATUS_OUT_OF_RANGE       Parameter nNumSymbsInSet is set to 0.
 *
 * @remarks
 * This function enables customers to bypass XT9's input modules when adding ambiguous values to an input sequence.<br>
 * The integration layer must ensure that all the symbols in psSymbs are supported by the XT9 core.
 */

ET9STATUS ET9FARCALL ET9AddCustomSymbolSet(ET9WordSymbInfo * const  pWordSymbInfo,
                                           ET9SYMB * const          psSymbs,
                                           ET9U8   * const          pbSymbProbs,
                                           const ET9UINT            nNumSymbsInSet,
                                           const ET9INPUTSHIFTSTATE eShiftState,
                                           const ET9U8              bCurrIndexInList)
{
    WLOG4(fprintf(pLogFile4, "ET9AddCustomSymbolSet, nNumSymbsInSet = %d\n", nNumSymbsInSet);)

    if (!pWordSymbInfo || !psSymbs || !pbSymbProbs || pWordSymbInfo->wInitOK != ET9GOODSETUP) {
        return ET9STATUS_INVALID_MEMORY;
    }
    if (!nNumSymbsInSet) {
        return ET9STATUS_OUT_OF_RANGE;
    }
    if (pWordSymbInfo->bNumSymbs >= ET9MAXWORDSIZE) {
        return ET9STATUS_FULL;
    }
    if (pWordSymbInfo->Private.bPreventWhiteSpaceInput && _ET9FindSpacesAndUnknown(psSymbs, nNumSymbsInSet)) {
        return ET9STATUS_INVALID_TEXT;
    }

    {
        ET9UINT     nCount;
        ET9U8       *pbProb = pbSymbProbs;

        for (nCount = nNumSymbsInSet; nCount; --nCount, ++pbProb) {
            if (!*pbProb) {
                return ET9STATUS_BAD_PARAM;
            }
        }
    }

    _ET9TrackInputEvents(pWordSymbInfo, ET9InputEvent_add);

    _ET9ImminentSymb(pWordSymbInfo, bCurrIndexInList, 0);

    if (pWordSymbInfo->bNumSymbs + 1 > ET9MAXWORDSIZE) {
        return ET9STATUS_FULL;
    }

    {
        ET9SymbInfo         * const pSymbInfo = &pWordSymbInfo->SymbsInfo[pWordSymbInfo->bNumSymbs];

        ET9DataPerBaseSym   *pDPBSym;

        ET9UINT             nCount;
        ET9UINT             nProbCount;
        ET9UINT             nCurrProb;
        ET9UINT             nNextProb;
        ET9BOOL             bAllPuncts;
        ET9SYMB             *psSymb;
        ET9U8               *pbProb;

        nProbCount = 0;
        bAllPuncts = 1;

        pSymbInfo->bNumBaseSyms = 1;

        pDPBSym = pSymbInfo->DataPerBaseSym;
        pDPBSym->bSymFreq = 0;
        pDPBSym->bNumSymsToMatch = 0;

        /* iterate all given probabilities, highest first */

        for (nCurrProb = 0xFFFF; nCurrProb && pSymbInfo->bNumBaseSyms < ET9MAXBASESYMBS; nCurrProb = nNextProb) {

            WLOG4(fprintf(pLogFile4, "ET9AddCustomSymbolSet, nCurrProb = %d\n", nCurrProb);)

            psSymb = psSymbs;
            pbProb = pbSymbProbs;
            nNextProb = nCurrProb;

            if (nCurrProb <= 0xFF) {
                ++nProbCount;
            }

            /* pick up all symbols of the current probability */

            for (nCount = nNumSymbsInSet; nCount && pSymbInfo->bNumBaseSyms <= ET9MAXBASESYMBS; --nCount, ++psSymb, ++pbProb) {

                /* keep track of (find) next highest probability */

                if (*pbProb < nCurrProb && (nNextProb == nCurrProb || *pbProb > nNextProb)) {
                    nNextProb = *pbProb;
                }

                /* skip if not the active probability */

                if (*pbProb != nCurrProb) {
                    continue;
                }

                /* move to next base symb if full or new probability */

                if (pDPBSym->bNumSymsToMatch >= ET9MAXALTSYMBS || pDPBSym->bSymFreq != nCurrProb) {

                    if (pSymbInfo->bNumBaseSyms >= ET9MAXBASESYMBS) {
                        break;
                    }

                    /* only move if not empty */

                    if (pDPBSym->bNumSymsToMatch) {
                        ++pDPBSym;
                        ++pSymbInfo->bNumBaseSyms;
                    }

                    /* fresh start */

                    pDPBSym->bSymFreq = (ET9U8)nCurrProb;
                    pDPBSym->bNumSymsToMatch = 0;
                }

                WLOG4(fprintf(pLogFile4, "ET9AddCustomSymbolSet, adding, symb = %x (%c)\n", *psSymb, (char)*psSymb);)

                if (bAllPuncts && !_ET9_IsPunctChar(*psSymb)) {
                    bAllPuncts = 0;
                }

                if (_ET9SymIsUpper(*psSymb, pWordSymbInfo->Private.wLocale)) {
                    pDPBSym->sUpperCaseChar[pDPBSym->bNumSymsToMatch] = *psSymb;
                    pDPBSym->sChar[pDPBSym->bNumSymsToMatch] = _ET9SymToLower(*psSymb, pWordSymbInfo->Private.wLocale);
                }
                else {
                    pDPBSym->sChar[pDPBSym->bNumSymsToMatch] = *psSymb;
                    pDPBSym->sUpperCaseChar[pDPBSym->bNumSymsToMatch] = _ET9SymToUpper(*psSymb, pWordSymbInfo->Private.wLocale);
                }

                ++pDPBSym->bNumSymsToMatch;
            }

            if (nNextProb >= nCurrProb) {
                break;
            }
        }

        /* assign symb type, either single punct, smart punct or letter */

        if (bAllPuncts) {
            pSymbInfo->bSymbType = ET9KTPUNCTUATION;
        }
        else {
            pSymbInfo->bSymbType = ET9KTLETTER;
        }

        /* input type is based on the number of probabilities */

        pSymbInfo->eInputType = (nProbCount == 1 ? ET9DISCRETEKEY : ET9REGIONALKEY);

        /* always ambig */

        pSymbInfo->bAmbigType = (ET9U8)ET9AMBIG;

        /* other attributes */

        pSymbInfo->eShiftState = eShiftState;
        pSymbInfo->bForcedLowercase = 0;
        pSymbInfo->wInputIndex = 0;

        pSymbInfo->wKeyIndex = 0xFFFF;              /* for reselect */
        pSymbInfo->wTapX = ET9UNDEFINEDTAPVALUE;
        pSymbInfo->wTapY = ET9UNDEFINEDTAPVALUE;
        pSymbInfo->pKdbKey = NULL;
        pSymbInfo->bTraceProbability = 0;
        pSymbInfo->bTraceIndex = 0;
        pSymbInfo->bFreqsInvalidated = 1;

        pSymbInfo->wKdb1 = 0;
        pSymbInfo->wPage1 = 0;
        pSymbInfo->wKdb2 = 0;
        pSymbInfo->wPage2  = 0;

        _ET9InvalidateOneSymb(pWordSymbInfo, pWordSymbInfo->bNumSymbs);
        ++pWordSymbInfo->bNumSymbs;
        pWordSymbInfo->Private.eLastShiftState = pSymbInfo->eShiftState;
    }

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * This function locks a word.
 *
 * @param pWordSymbInfo             Pointer to word symbol info structure.
 * @param pWord                     Pointer to word to be locked.
 * @param eLanguageSource           Sorce of the word to lock.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL _ET9LockWord(ET9WordSymbInfo       * const pWordSymbInfo,
                                  ET9SimpleWord         * const pWord,
                                  const ET9AWLANGUAGESOURCE     eLanguageSource)

{
    ET9U16             i;
    ET9U16             wCount;
    ET9U8              bInitialNumSymbs;
    ET9SYMB            *psSymb;
    ET9INPUTSHIFTSTATE eShiftState;

    if (!pWord || !pWordSymbInfo || pWordSymbInfo->wInitOK != ET9GOODSETUP) {
        return ET9STATUS_INVALID_MEMORY;
    }

    if (pWord->wLen == 0 || pWord->wLen > ET9MAXWORDSIZE || pWord->wCompLen > ET9MAXWORDSIZE) {
        return ET9STATUS_BAD_PARAM;
    }

    /* if reselecting, need to build first */

    if (pWordSymbInfo->Private.bRequiredVerifyInput) {
        return ET9STATUS_NEED_SELLIST_BUILD;
    }

    /* validate incoming string, zero is really bad... */

    psSymb = pWord->sString;

    for (wCount = pWord->wLen; wCount; --wCount, ++psSymb) {
        if (pWordSymbInfo->Private.bPreventWhiteSpaceInput && _ET9_IsWhiteSpace(*psSymb)) {
            return ET9STATUS_INVALID_TEXT;
        }
    }

    /* get shift state */

    if (pWordSymbInfo->bNumSymbs) {
        eShiftState = (pWordSymbInfo->Private.eLastShiftState == ET9CAPSLOCK ? ET9CAPSLOCK : ET9NOSHIFT);
    }
    else {
        eShiftState = ET9NOSHIFT;
    }

    /* can't lock trace info - explicifying instead */

    if (_ET9HasTraceInfo(pWordSymbInfo)) {

        const ET9U16 wLen = pWord->wLen;
        const ET9U16 wCompLen = pWord->wCompLen;

        if (wCompLen) {
            pWord->wLen = (ET9U16)(wLen - wCompLen);
            pWord->wCompLen = 0;
        }

        _ET9ExplicifyWord(pWordSymbInfo, pWord);

        pWord->wLen = wLen;
        pWord->wCompLen = wCompLen;
    }

    /* initial num symbs */

    bInitialNumSymbs = pWordSymbInfo->bNumSymbs;

    /* remove symbs if word is shorter */

    while (pWord->wLen < pWordSymbInfo->bNumSymbs) {

        ET9ClearOneSymb(pWordSymbInfo);
    }

    /* perform lock */

    for (i = 0; i < pWord->wLen; ++i) {

        /* add missing and assign lock point */

        if (i >= pWordSymbInfo->bNumSymbs) {

            /* fill in the pSymbInfos here */
            /* DO _NOT_ CHANGE THE 'ET9_NO_ACTIVE_WORD' PARAMETER IN THE FOLLOWING CALL! */

            ET9AddExplicitSymb(pWordSymbInfo, pWord->sString[i], eShiftState, ET9_NO_ACTIVE_INDEX);
            _ET9InvalidateOneLock(pWordSymbInfo, (ET9U8)i);
        }

        /* locked symbs must be assigned here */

        pWordSymbInfo->SymbsInfo[i].sLockedSymb = pWord->sString[i];
    }

    /* the normal lock (at the end) */

    pWordSymbInfo->SymbsInfo[pWord->wLen - 1].bLocked = (ET9U8)ET9STEMLOCK;

    pWordSymbInfo->SymbsInfo[pWord->wLen - 1].bLockLanguageSource = (ET9U8)eLanguageSource;

    /* words with completion should have the completion start point tagged for lock as well,
       but adjusted if there already are symbols in place */

    if (pWord->wCompLen && pWord->wLen > pWord->wCompLen && pWord->wLen > bInitialNumSymbs) {

        ET9INT snTagPoint = pWord->wLen - pWord->wCompLen;

        if (snTagPoint < bInitialNumSymbs) {
            snTagPoint = bInitialNumSymbs;
        }

        ET9Assert(snTagPoint <= pWordSymbInfo->bNumSymbs);

        pWordSymbInfo->SymbsInfo[snTagPoint - 1].bLocked = (ET9U8)ET9STEMLOCK;
    }

    /* invalidate */

    _ET9InvalidateSymbInfo(pWordSymbInfo);

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/**
 * @brief Locks the word at the indicated index position in the current selection list, allowing the word to be "frozen" as the stem to which additional characters can be appended.
 *
 * @param[in]     pWordSymbInfo             Pointer to the Word Symbol Data Structure (ET9WordSymbInfo).
 * @param[in]     pWord                     Pointer to an instance of ET9AWWordInfo, which contains the word to be locked.
 *
 * @retval ET9STATUS_NONE               Function call was handled successfully.
 * @retval ET9STATUS_INVALID_MEMORY     At least one pointer passed as a function argument was set to null.
 * @retval ET9STATUS_BAD_PARAM          Values in ET9AWWordInfo are incorrect.
 * @retval ET9STATUS_INVALID_TEXT       Values in ET9AWWordInfo are incorrect.
 * @retval ET9STATUS_NEED_SELLIST_BUILD Text has been reselected, but the selection list has not been rebuilt.
 *
 * @remarks To lock a word, the selection list must be valid and the input cannot be manipulated before being locked.<br>
 * If locking a stem from reselected text, the integration layer must update the selection list prior to calling ET9LockWord by calling ET9AWSelLstBuild
 * to request that XT9 rebuild the list and then calling ET9AWSelLstGetWord one or more times to retrieve words from the list.
 */

ET9STATUS ET9FARCALL ET9LockWord(ET9WordSymbInfo * const pWordSymbInfo,
                                 ET9SimpleWord   * const pWord)

{
    return _ET9LockWord(pWordSymbInfo, pWord, ET9AWBOTH_LANGUAGES);
}

/*---------------------------------------------------------------------------*/
/** \internal
 * This function get exact word for the current tap sequence stem.
 *
 * @param pWordSymbInfo             Pointer to word symbol info structure.
 * @param pWord                     Pointer to word for the result.
 * @param wIndex                    Start point.
 * @param nLength                   Length to get.
 * @param pConvertSymb              Pointer to a (callback) convert symbol function, or NULL.
 * @param pConvertSymbInfo          Integeration layer pointer that will be passed back in the callback, or NULL.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

static ET9STATUS ET9LOCALCALL __ET9GetExactWordStem(ET9WordSymbInfo      * const pWordSymbInfo,
                                                    ET9SimpleWord        * const pWord,
                                                    const ET9U16                 wIndex,
                                                    const ET9UINT                nLength,
                                                    const ET9CONVERTSYMBCALLBACK pConvertSymb,
                                                    void                 * const pConvertSymbInfo)
{
    ET9STATUS           wStatus = ET9STATUS_NONE;
    ET9SYMB             *pStr;
    ET9INT              nLockIdx = -1;
    ET9U8               i;
    ET9INT              j;
    ET9INT              nStart;
    ET9DataPerBaseSym   *pDataPerBaseSym;
    ET9U8               bType;
    ET9SymbInfo         *pSymbInfo;

    ET9Assert(pWord);
    ET9Assert(pWordSymbInfo);
    ET9Assert(wIndex + nLength <= pWordSymbInfo->bNumSymbs);

    pWord->wLen = 0;
    pWord->wCompLen = 0;

    if (!nLength) {
        return ET9STATUS_NO_MATCHING_WORDS;
    }

    pStr = pWord->sString;

    /* locate lock */

    j = wIndex + nLength;
    pSymbInfo =  &pWordSymbInfo->SymbsInfo[j];
    while (--j >= wIndex) {
        --pSymbInfo;
        if (pSymbInfo->bLocked) {
            nLockIdx = j;
            break;
        }
    }

    /* get locked symbs */

    pSymbInfo =  &pWordSymbInfo->SymbsInfo[wIndex];
    for (j = wIndex; j <= nLockIdx; ++j, ++pSymbInfo) {
        *pStr++ = pSymbInfo->sLockedSymb;
        ++pWord->wLen;
        ET9Assert(*(pStr-1) && *(pStr-1) != (ET9SYMB)0xcccc);
    }

    /* get non locked symbs */

    nStart = (nLockIdx < 0) ? wIndex : nLockIdx + 1;

    pSymbInfo = &pWordSymbInfo->SymbsInfo[nStart];
    for (j = nStart; j < (ET9INT)(wIndex + nLength); ++j, ++pSymbInfo) {

        if (pSymbInfo->bTraceIndex) {
            pWord->wLen = 0;
            return ET9STATUS_NO_MATCHING_WORDS;
        }

        pDataPerBaseSym = pSymbInfo->DataPerBaseSym;

        if (pDataPerBaseSym->bDefaultCharIndex == 0xFF) {
            pWord->wLen = 0;
            return ET9STATUS_NO_MATCHING_WORDS;
        }

        bType = pSymbInfo->bSymbType;

        if (bType == ET9KTSTRING) {
            for (i = 0; i < pDataPerBaseSym->bNumSymsToMatch; ++i) {
                *pStr++ = pDataPerBaseSym->sChar[i];
                ++pWord->wLen;
                ET9Assert(*(pStr-1) && *(pStr-1) != (ET9SYMB)0xcccc);
            }
        }
        else if (pSymbInfo->eShiftState) {
            *pStr++ = pDataPerBaseSym->sUpperCaseChar[0];
            ++pWord->wLen;
            ET9Assert(*(pStr-1) && *(pStr-1) != (ET9SYMB)0xcccc);
        }
        else {
            *pStr++ = pDataPerBaseSym->sChar[0];
            ++pWord->wLen;
            ET9Assert(*(pStr-1) && *(pStr-1) != (ET9SYMB)0xcccc);
        }
    }

    /* apply convert function for proper symbols */

    if (pConvertSymb != NULL) {

        ET9U16 wCount = pWord->wLen;
        ET9SYMB *pSymb = pWord->sString;

        for (; wCount; --wCount, ++pSymb) {
            pConvertSymb(pConvertSymbInfo, pSymb);
        }
    }

#ifdef ET9_DEBUG
    {
        /* verify that all chars are valid (non zero) */

        ET9U16 wCount = pWord->wLen;
        ET9SYMB *pSymb = pWord->sString;

        while (wCount--) {
            ET9Assert(*pSymb && *pSymb != (ET9SYMB)0xcccc);
            ++pSymb;
        }
    }
#endif

    return wStatus;
}

/*---------------------------------------------------------------------------*/
/**
 * @brief Retrieves the exact word--the word that most exactly matches the input sequence.
 *
 * @param[in]     pWordSymbInfo         Pointer to the Word Symbol Data Structure (ET9WordSymbInfo), which contains information about the current input sequence.
 * @param[out]    pWord                 Pointer to an instance of ET9AWWordInfo, which contains the word that most exactly matches the input sequence. The word is stored in ET9AWWordInfo.sWord.
 * @param[out]    pConvertSymb          Pointer to ET9CONVERTSYMBCALLBACK or NULL.
 * @param[in]     pConvertSymbInfo      Pointer passed back by ET9CONVERTSYMBCALLBACK or NULL.
 *
 * @retval ET9STATUS_NONE               Function call was handled successfully.
 * @retval ET9STATUS_INVALID_MEMORY     At least one pointer passed as a function argument was set to null.
 * @retval ET9STATUS_NO_WORD            There is no current active word or input sequence.
 */

ET9STATUS ET9FARCALL ET9GetExactWord(ET9WordSymbInfo      * const  pWordSymbInfo,
                                     ET9SimpleWord        * const  pWord,
                                     const ET9CONVERTSYMBCALLBACK pConvertSymb,
                                     void                 * const pConvertSymbInfo)
{
    ET9STATUS wStatus;

    if (!pWord || !pWordSymbInfo || pWordSymbInfo->wInitOK != ET9GOODSETUP) {
        return ET9STATUS_INVALID_MEMORY;
    }

    wStatus = __ET9GetExactWordStem(pWordSymbInfo, pWord, 0, pWordSymbInfo->bNumSymbs, pConvertSymb, pConvertSymbInfo);

    return wStatus;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Check to see if passed symbol is a sentence initiating punct.
 *
 * @param pWordSymbInfo        Pointer to word symbol struct.
 * @param sPunct               Punctuation symbol.
 *
 * @return True if sentence initiator, 0 if not.
 */

static ET9BOOL ET9LOCALCALL __ET9_IsSentenceInitPunct(ET9WordSymbInfo     const * const pWordSymbInfo,
                                                      const ET9SYMB                     sPunct)
{
    ET9SYMB const *psTable;
    ET9UINT nTableSize;

    ET9Assert(pWordSymbInfo);

    switch (pWordSymbInfo->Private.wLocale & ET9PLIDMASK)
    {
        case 0: /* compiler warning fix */
        default:
            {
                /* 0xBF   = upside-down ? */
                /* 0xA1   = upside-down ! */
                /* 0x17d9 = KHMER SIGN PHNAEK MUAN */

                static const ET9SYMB psStandardSentenceInitPunct[] = { 0x0022, 0x0028, 0x005b, 0x007b, 0x00bf, 0x00a1, 0x17d9 };

                psTable = psStandardSentenceInitPunct;
                nTableSize = sizeof(psStandardSentenceInitPunct) / sizeof(ET9SYMB);
            }
            break;
    }

    {
        ET9UINT nCount;
        ET9SYMB const * psSymb;

        psSymb = psTable;
        for (nCount = nTableSize; nCount; --nCount, ++psSymb) {
            if (sPunct == *psSymb) {
                return 1;
            }
        }
    }

    return 0;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Check to see if sentence terminating punctuation.
 *
 * @param pWordSymbInfo        Pointer to word symbol struct.
 * @param sPunct               Punctuation symbol.
 *
 * @return True if sentence terminator, 0 if not.
 */

static ET9BOOL ET9LOCALCALL __ET9_IsSentenceTermPunct(ET9WordSymbInfo     const * const pWordSymbInfo,
                                                      const ET9SYMB                     sPunct)
{
    ET9SYMB const *psTable;
    ET9UINT nTableSize;

    ET9Assert(pWordSymbInfo);

    switch (pWordSymbInfo->Private.wLocale & ET9PLIDMASK)
    {
        case ET9PLIDGreek:
            {
                /* . ? ! ; Greek Question Mark   ARABIC QUESTION MARK  ARABIC FULL STOP  DEVANAGARI DANDA   DEVANAGARI DOUBLE DANDA*/
                /* SINHALA PUNCTUATION KUNDDALIYA  THAI CHARACTER ANGKHANKHU   THAI CHARACTER KHOMUT  TIBETAN MARK SHAD          */
                /* TIBETAN MARK NYIS SHAD   MYANMAR SIGN LITTLE SECTION    MYANMAR SIGN SECTION    GEORGIAN PARAGRAPH SEPARATOR  */
                /* ETHIOPIC FULL STOP   ETHIOPIC PREFACE COLON  ETHIOPIC QUESTION MARK  ETHIOPIC PARAGRAPH SEPARATOR             */
                /* KHMER SIGN KHAN    KHMER SIGN BARIYOOSAN   KHMER SIGN KOOMUUT                                                 */

                static const ET9SYMB psGreekSentenceTermPunct[] =
                   { 0x002e, 0x003f, 0x0021, 0x003b, 0x037e, 0x061f, 0x06d4, 0x0964, 0x0965, 0x0df4, 0x0e5a, 0x0e5b, 0x0f0d, 0x0f0e, 0x104a, 0x104b,
                     0x10fb, 0x1362, 0x1366, 0x1367, 0x1368, 0x17d4, 0x17d5, 0x17da };

                psTable = psGreekSentenceTermPunct;
                nTableSize = sizeof(psGreekSentenceTermPunct) / sizeof(ET9SYMB);
            }
            break;

        default:
            {
                /* . ? ! Greek Question Mark   ARABIC QUESTION MARK  ARABIC FULL STOP  DEVANAGARI DANDA   DEVANAGARI DOUBLE DANDA*/
                /* SINHALA PUNCTUATION KUNDDALIYA  THAI CHARACTER ANGKHANKHU   THAI CHARACTER KHOMUT  TIBETAN MARK SHAD          */
                /* TIBETAN MARK NYIS SHAD   MYANMAR SIGN LITTLE SECTION    MYANMAR SIGN SECTION    GEORGIAN PARAGRAPH SEPARATOR  */
                /* ETHIOPIC FULL STOP   ETHIOPIC PREFACE COLON  ETHIOPIC QUESTION MARK  ETHIOPIC PARAGRAPH SEPARATOR             */
                /* KHMER SIGN KHAN    KHMER SIGN BARIYOOSAN   KHMER SIGN KOOMUUT                                                 */

                static const ET9SYMB psStandardSentenceTermPunct[] =
                   { 0x002e, 0x003f, 0x0021, 0x037e, 0x061f, 0x06d4, 0x0964, 0x0965, 0x0df4, 0x0e5a, 0x0e5b, 0x0f0d, 0x0f0e, 0x104a, 0x104b, 0x10fb,
                     0x1362, 0x1366, 0x1367, 0x1368, 0x17d4, 0x17d5, 0x17da };

                psTable = psStandardSentenceTermPunct;
                nTableSize = sizeof(psStandardSentenceTermPunct) / sizeof(ET9SYMB);
            }
            break;
    }

    {
        ET9UINT nCount;
        ET9SYMB const * psSymb;

        psSymb = psTable;
        for (nCount = nTableSize; nCount; --nCount, ++psSymb) {
            if (sPunct == *psSymb) {
                return 1;
            }
        }
    }

    return 0;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * This function checks whether shift should be used for a 'start-of-sentence' scenario given a context string.
 *
 * @param[in]     pWordSymbInfo             Pointer to the Word Symbol Data Structure (ET9WordSymbInfo).
 * @param[in]     psString                  String to check.
 * @param[in]     wStringLen                Length of string.
 *
 * @return Non zero if auto cap, otherwise zero.
 */

ET9BOOL ET9FARCALL _ET9IsAutoCapSituation(ET9WordSymbInfo const * const pWordSymbInfo,
                                          ET9SYMB         const * const psString,
                                          const ET9U16                  wStringLen)
{
    ET9BOOL bAutoCap = 0;
    ET9UINT nPunctDetected = 0;
    ET9UINT nInitPunctDetected = 0;
    ET9UINT nWhitespaceDetected = 0;

    ET9SYMB const * psSymb;

    /* if no symbols , assume beginning of field - AUTOCAP SITUATION */

    if (!wStringLen) {
        return 1;
    }

    /* otherwise work backwards through buffer */

    for (psSymb = &psString[wStringLen - 1]; psSymb >= psString; --psSymb) {

        const ET9SymbClass eSymbClass = _ET9_GetSymbolClass(*psSymb);

        /* if the symbol is whitespace... */

        if (eSymbClass == ET9_WhiteSymbClass) {

            /* if sequence already had punct*/

            if (nPunctDetected) {

                /* For now, this is weird, intervening punct  */

                break;
            }
            else {

                ++nWhitespaceDetected;

                /* if only whitespace returned */

                if (nWhitespaceDetected + nInitPunctDetected == wStringLen) {
                    bAutoCap = 1;
                    break;
                }
            }
        }

        /* if the symbol is punctuation */

        else if (eSymbClass == ET9_PunctSymbClass){

            /* track it with counter */

            ++nPunctDetected;

            /* should violate sequence if no intervening whitespace */

            if (!nWhitespaceDetected) {

                /* UNLESS it's 'sentence initiating' punctuation */

                if (!__ET9_IsSentenceInitPunct(pWordSymbInfo, *psSymb)) {
                    break;
                }

                /* if sentence initiating punct already found, weird.. skip out */

                else if (nInitPunctDetected) {
                    break;
                }

                /* otherwise don't record sentence inititating punct as punctuation */

                else {

                    /* if no other syms, consider this a autocap situation */

                    if (wStringLen == 1) {
                        bAutoCap = 1;
                        break;
                    }

                    /* otherwise skip over it, keep checking */

                    else {
                        ++nInitPunctDetected;
                        --nPunctDetected;
                    }
                }
            }

            /* if sequence had whitespace following sentence terminating punctuation, */
            /* consider it an AUTOCAP SITUATION                                       */

            else if (__ET9_IsSentenceTermPunct(pWordSymbInfo, *psSymb)) {
                bAutoCap = 1;
                break;
            }
        }

        /* if ET9SYMNUMBRMASK/ET9SYMALPHAMASK/ET9SYMUNKNMASK, get out */

        else {
            break;
        }
    }

    return bAutoCap;
}

/*---------------------------------------------------------------------------*/
/**
 * @brief This function evaluates whether shift should be used for a 'start-of-sentence' scenario.
 *
 * @param[in]     pWordSymbInfo             Pointer to the Word Symbol Data Structure (ET9WordSymbInfo).
 * @param[in]     pbAutoCap                 Pointer to yes/no for applying autocap shift.
 * @param[in]     pBufferRead               Pointer to a (callback) buffer retrieval routine.
 * @param[in]     pBufferReadInfo           Integeration layer pointer that will be passed back in the callback, or NULL.
 *
 * @retval ET9STATUS_NONE               Function call was handled successfully.
 * @retval ET9STATUS_INVALID_MEMORY     At least one pointer passed as a function argument was set to null.
 *
 * @remarks This function also determines whether a compounding situation exists in order to force downshifting.
 */

ET9STATUS ET9FARCALL ET9GetAutoCapSituation(ET9WordSymbInfo * const pWordSymbInfo,
                                            ET9BOOL         *       pbAutoCap,
                                            const ET9BUFFERREADCALLBACK pBufferRead,
                                            void            * const pBufferReadInfo)
{
    ET9STATUS wStatus = ET9STATUS_NONE;
    ET9SYMB   psBuffer[ET9AUTOCAP_READ_SIZE];
    ET9U16    wSymsRead = 0;

    if (!pBufferRead || !pbAutoCap || !pWordSymbInfo || pWordSymbInfo->wInitOK != ET9GOODSETUP) {
        return ET9STATUS_INVALID_MEMORY;
    }

    /* assume this is NOT an autocap situation */

    *pbAutoCap = 0;

    if (pWordSymbInfo->bNumSymbs <= 1 && !ET9CAPS_MODE(pWordSymbInfo->dwStateBits)) {

        /* use callback to retrieve buffer data */

        wStatus = pBufferRead(pBufferReadInfo, ET9AUTOCAP_READ_SIZE, psBuffer, &wSymsRead);

        if (!wStatus) {

            *pbAutoCap = _ET9IsAutoCapSituation(pWordSymbInfo, psBuffer, wSymsRead);

            if (*pbAutoCap) {
                pWordSymbInfo->Private.eAutocapWord = ET9AUTOCAP_PENDING;
            }
        }
    }

    /* if not an autocap situation, no current word (but syms read), and no postshift mandated */

    if (!*pbAutoCap &&
        !wStatus &&
        wSymsRead &&
        !pWordSymbInfo->bNumSymbs &&
        !ET9CAPS_MODE(pWordSymbInfo->dwStateBits) &&
        pWordSymbInfo->Private.eCurrPostShiftMode == ET9POSTSHIFTMODE_DEFAULT) {

        /* look at sym immediately before cursor */

        if (_ET9_GetSymbolClass(psBuffer[wSymsRead-1]) != ET9_WhiteSymbClass) {

            /* if this is word being compounded, keep flag */

            pWordSymbInfo->Private.bCompoundingDownshift = 1;
        }
        else{
            pWordSymbInfo->Private.bCompoundingDownshift = 0;
        }
    }

    return wStatus;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Imminent symbol.
 * This function makes the ling engine aware of that a new symbol is imminent.
 *
 * @param pWordSymbInfo             Pointer to word symbol info structure.
 * @param bCurrIndexInList          Highlighted selection list index.
 * @param bImplicitLock             Implicit lock setting.
 *
 * @return None
 */

void ET9FARCALL _ET9ImminentSymb(ET9WordSymbInfo    * const pWordSymbInfo,
                                 const ET9U8                bCurrIndexInList,
                                 const ET9BOOL              bImplicitLock)
{
    ET9Assert(pWordSymbInfo);
    ET9_UNUSED(bImplicitLock); /* in some cases */

    WLOG4(fprintf(pLogFile4, "_ET9ImminentSymb, bNumSymbs = %d\n", pWordSymbInfo->bNumSymbs);)

    pWordSymbInfo->Private.bCurrSelListIndex = bCurrIndexInList;

    /* handle "imminent symb" - edition specific */

#ifdef ET9_ALPHABETIC_MODULE

    if (pWordSymbInfo->Private.ppEditionsList[ET9EDITION_AW_1]) {
        _ET9AWImminentSymb(pWordSymbInfo->Private.ppEditionsList[ET9EDITION_AW_1], bImplicitLock);
    }

    if (pWordSymbInfo->Private.ppEditionsList[ET9EDITION_AW_2]) {
        _ET9AWImminentSymb(pWordSymbInfo->Private.ppEditionsList[ET9EDITION_AW_2], bImplicitLock);
    }

#endif /* ET9_ALPHABETIC_MODULE */

    /* handle post shift "lock" */

    if (pWordSymbInfo->Private.eCurrPostShiftMode != ET9POSTSHIFTMODE_DEFAULT) {

        ET9BOOL                 bFirst;
        ET9U16                  wCount;
        ET9SymbInfo             *pSymbInfo;
        const ET9POSTSHIFTMODE  eMode = pWordSymbInfo->Private.eCurrPostShiftMode;

        WLOG4(fprintf(pLogFile4, "_ET9ImminentSymb, locking post shift state\n");)

        /* symbol shift change */

        bFirst = 1;
        wCount = pWordSymbInfo->bNumSymbs;
        pSymbInfo = pWordSymbInfo->SymbsInfo;
        for (; wCount; --wCount, ++pSymbInfo, bFirst = 0) {

            if (eMode == ET9POSTSHIFTMODE_UPPER || eMode == ET9POSTSHIFTMODE_INITIAL && bFirst) {

                pSymbInfo->eShiftState = ET9SHIFT;
                pSymbInfo->bForcedLowercase = 0;

                /* if the sym is locked, make sure locked sym is correct case (CR 23767) */

                if (pSymbInfo->sLockedSymb) {
                    pSymbInfo->sLockedSymb = _ET9SymToUpper(pSymbInfo->sLockedSymb, pWordSymbInfo->Private.wLocale);
                }
            }
            else {
                pSymbInfo->eShiftState = ET9NOSHIFT;

                /* if the sym is locked, make sure locked sym is correct case (CR 23767) */

                if (pSymbInfo->sLockedSymb) {
                    pSymbInfo->sLockedSymb = _ET9SymToLower(pSymbInfo->sLockedSymb, pWordSymbInfo->Private.wLocale);
                }
                if (eMode == ET9POSTSHIFTMODE_LOWER) {
                    pSymbInfo->bForcedLowercase = 1;
                }
            }
        }

        /* shift state change */

        if (eMode == ET9POSTSHIFTMODE_UPPER) {
            pWordSymbInfo->dwStateBits &= ~(ET9STATE_SHIFT_MASK);
            pWordSymbInfo->dwStateBits |= ET9STATE_CAPS_MASK;
            pWordSymbInfo->Private.eLastShiftState = ET9CAPSLOCK;
        }
        else {
            pWordSymbInfo->dwStateBits &= ~(ET9STATE_SHIFT_MASK | ET9STATE_CAPS_MASK);
            pWordSymbInfo->Private.eLastShiftState = ET9NOSHIFT;
        }

        /* reset mode */

        pWordSymbInfo->Private.eCurrPostShiftMode = ET9POSTSHIFTMODE_DEFAULT;
    }
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Invalidate a single symbol.
 *
 * @param pWordSymbInfo             Pointer to word symbol info structure.
 * @param bIndex                    Index to invalidate.
 *
 * @return None
 */

void ET9FARCALL _ET9InvalidateOneSymb(ET9WordSymbInfo * const pWordSymbInfo, const ET9U8 bIndex)
{
    ET9U8               bEditionCount;
    ET9BaseLingInfo     **ppLing;

    ET9Assert(pWordSymbInfo);

    ppLing = pWordSymbInfo->Private.ppEditionsList;

    for (bEditionCount = ET9MAXEDITIONS; bEditionCount; --bEditionCount, ++ppLing) {

        if (*ppLing) {
            (*ppLing)->bSymbInvalidated[bIndex] = 1;
            (*ppLing)->bSymbsInfoInvalidated = 1;
        }
    }

    /* the required word should not be verified or located after an input modification */

    pWordSymbInfo->Private.bRequiredLocate = 0;
    pWordSymbInfo->Private.bRequiredVerifyInput = 0;
}

#if 0

/*---------------------------------------------------------------------------*/
/** \internal
 * Validate a single symbol.
 *
 * @param pWordSymbInfo             Pointer to word symbol info structure.
 * @param bIndex                    Index to validate.
 *
 * @return None
 */

void ET9FARCALL _ET9ValidateOneSymb(ET9WordSymbInfo * const pWordSymbInfo, const ET9U8 bIndex)
{
    ET9U8               bEditionCount;
    ET9BaseLingInfo     **ppLing;

    ET9Assert(pWordSymbInfo);

    ppLing = pWordSymbInfo->Private.ppEditionsList;

    for (bEditionCount = ET9MAXEDITIONS; bEditionCount; --bEditionCount, ++ppLing) {

        if (*ppLing) {
            (*ppLing)->bSymbInvalidated[bIndex] = 0;
        }
    }
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Validate all symbols.
 *
 * @param pWordSymbInfo             Pointer to word symbol info structure.
 *
 * @return None
 */

void ET9FARCALL _ET9ValidateAllSymbs(ET9WordSymbInfo * const pWordSymbInfo)
{
    ET9U8               bEditionCount;
    ET9BaseLingInfo     **ppLing;

    ET9Assert(pWordSymbInfo);

    ppLing = pWordSymbInfo->Private.ppEditionsList;

    for (bEditionCount = ET9MAXEDITIONS; bEditionCount; --bEditionCount, ++ppLing) {

        if (*ppLing) {

            ET9U8  bIndex;

            for (bIndex = 0; bIndex < pWordSymbInfo->bNumSymbs; ++bIndex) {
                (*ppLing)->bSymbInvalidated[bIndex] = 0;
            }
        }
    }
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Validate a single symbol - AW only.
 *
 * @param pWordSymbInfo             Pointer to word symbol info structure.
 * @param bIndex                    Index to validate.
 *
 * @return None
 */

void ET9FARCALL _ET9ValidateOneSymbAW(ET9WordSymbInfo * const pWordSymbInfo, const ET9U8 bIndex)
{
    ET9BaseLingInfo     **ppLing;

    ET9Assert(pWordSymbInfo);

    ppLing = &pWordSymbInfo->Private.ppEditionsList[ET9EDITION_AW];

    if (*ppLing) {
        (*ppLing)->bSymbInvalidated[bIndex] = 0;
    }
}

#endif

/*---------------------------------------------------------------------------*/
/** \internal
 * Invalidate a single lock.
 *
 * @param pWordSymbInfo             Pointer to word symbol info structure.
 * @param bIndex                    Index to invalidate.
 *
 * @return None
 */

void ET9FARCALL _ET9InvalidateOneLock(ET9WordSymbInfo * const pWordSymbInfo, const ET9U8 bIndex)
{
    ET9U8               bEditionCount;
    ET9BaseLingInfo     **ppLing;

    ET9Assert(pWordSymbInfo);

    ppLing = pWordSymbInfo->Private.ppEditionsList;

    for (bEditionCount = ET9MAXEDITIONS; bEditionCount; --bEditionCount, ++ppLing) {

        if (*ppLing) {
            (*ppLing)->bLockInvalidated[bIndex] = 1;
            (*ppLing)->bSymbsInfoInvalidated = 1;
        }
    }

    /* the required word should not be verified or located after an input modification */

    pWordSymbInfo->Private.bRequiredLocate = 0;
    pWordSymbInfo->Private.bRequiredVerifyInput = 0;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Invalidate the symb info structure (something changed).
 *
 * @param pWordSymbInfo             Pointer to word symbol info structure.
 *
 * @return None
 */

void ET9FARCALL _ET9InvalidateSymbInfo(ET9WordSymbInfo * const pWordSymbInfo)
{
    ET9U8               bEditionCount;
    ET9BaseLingInfo     **ppLing;

    ET9Assert(pWordSymbInfo);

    ppLing = pWordSymbInfo->Private.ppEditionsList;

    for (bEditionCount = ET9MAXEDITIONS; bEditionCount; --bEditionCount, ++ppLing) {

        if (*ppLing) {
            (*ppLing)->bSymbsInfoInvalidated = 1;
        }
    }

    /* the required word should not be verified or located after an input modification */

    pWordSymbInfo->Private.bRequiredLocate = 0;
    pWordSymbInfo->Private.bRequiredVerifyInput = 0;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Invalidate the selection list (something "big" happened).
 *
 * @param pWordSymbInfo             Pointer to word symbol info structure.
 *
 * @return None
 */

void ET9FARCALL _ET9InvalidateSelList(ET9WordSymbInfo * const pWordSymbInfo)
{
    ET9U8               bEditionCount;
    ET9BaseLingInfo     **ppLing;

    ET9Assert(pWordSymbInfo);

    ppLing = pWordSymbInfo->Private.ppEditionsList;

    for (bEditionCount = ET9MAXEDITIONS; bEditionCount; --bEditionCount, ++ppLing) {

        if (*ppLing) {
            (*ppLing)->bSelListInvalidated = 1;
        }
    }
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Content is the result of an explicification.
 *
 * @param pWordSymbInfo             Pointer to word symbol info structure.
 *
 * @return None
 */

void ET9FARCALL _ET9ContentExplicified(ET9WordSymbInfo * const pWordSymbInfo)
{
    ET9U8               bEditionCount;
    ET9BaseLingInfo     **ppLing;

    ET9Assert(pWordSymbInfo);

    ppLing = pWordSymbInfo->Private.ppEditionsList;

    for (bEditionCount = ET9MAXEDITIONS; bEditionCount; --bEditionCount, ++ppLing) {

        if (*ppLing) {
            (*ppLing)->bContentExplicified = 1;
        }
    }
}

/*---------------------------------------------------------------------------*/
/** \internal
 * This function will initialize the word symb info.
 *
 * @param pWordSymbInfo             Pointer to word symbol info structure.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL _ET9WordSymbInit(ET9WordSymbInfo * const pWordSymbInfo)
{
    ET9STATUS           wStatus;
    ET9U8               bEditionCount;
    ET9BaseLingInfo     **ppLing;
    ET9U16              wSaveCount;
    ET9SavedInputWord   *pSavedWord;

    ET9Assert(pWordSymbInfo != NULL);

    if (pWordSymbInfo->wInitOK == ET9GOODSETUP) {
        return ET9STATUS_NONE;
    }

    _ET9ClearMem((ET9U8*)pWordSymbInfo, sizeof(ET9WordSymbInfo));

    wStatus = _ET9CheckFundamentalTypes();

    if (wStatus) {
        return wStatus;
    }

    wStatus = _ET9CheckCharProps();

    if (wStatus) {
        return wStatus;
    }

    pWordSymbInfo->wInitOK = ET9GOODSETUP;
    pWordSymbInfo->pPublicExtension = NULL;
    pWordSymbInfo->Private.eCurrPostShiftMode = ET9POSTSHIFTMODE_DEFAULT;
    pWordSymbInfo->Private.bPreventWhiteSpaceInput = 1;

    ppLing = pWordSymbInfo->Private.ppEditionsList;

    for (bEditionCount = ET9MAXEDITIONS; bEditionCount; --bEditionCount, ++ppLing) {
        *ppLing = NULL;
    }

    pWordSymbInfo->Private.bInputRestarted = 1;
    pWordSymbInfo->Private.sSavedInputWords.wCurrInputSaveIndex = 0;

    pSavedWord = pWordSymbInfo->Private.sSavedInputWords.pSavedWords;

    for (wSaveCount = ET9MAXSAVEINPUTWORDS; wSaveCount; --wSaveCount, ++pSavedWord) {
        pSavedWord->wStorePos = UNDEFINED_STORE_INDEX;
    }

    _ET9_GetDefaultLocale(&pWordSymbInfo->Private.wLocale, &pWordSymbInfo->Private.bManualLocale);

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 *
 * Checks if current input is a magic string key.
 * NOTE: Already verified that pWordSymbInfo->bNumSymbs == ET9MAXLDBWORDSIZE prior to call.
 *
 * @param pWordSymbInfo             Pointer to word symbol info structure.
 *
 * @return Non zero if key matches, otherwise zero.
 */

ET9BOOL ET9FARCALL _ET9IsMagicStringKey(ET9WordSymbInfo *pWordSymbInfo)
{
    ET9BOOL bRet = 0;
    ET9INT i;
    ET9SYMB curSym;
    ET9SymbInfo *pSymbsInfo;

    ET9Assert(pWordSymbInfo);
    ET9Assert(pWordSymbInfo->bNumSymbs == ET9MAXLDBWORDSIZE);

    curSym = pWordSymbInfo->SymbsInfo[0].DataPerBaseSym[0].sChar[0];
    pSymbsInfo = &pWordSymbInfo->SymbsInfo[1];
    for (i = 1; i < ET9MAGICSTR_FIRSTSET; ++i, ++pSymbsInfo) {
        if (pSymbsInfo->DataPerBaseSym[0].sChar[0] != curSym) {
           break;
        }
    }
    if (i == ET9MAGICSTR_FIRSTSET) {
        curSym = pWordSymbInfo->SymbsInfo[ET9MAGICSTR_FIRSTSET].DataPerBaseSym[0].sChar[0];
        if (curSym != pWordSymbInfo->SymbsInfo[0].DataPerBaseSym[0].sChar[0]) {
            for (; i < ET9MAXLDBWORDSIZE; ++i, ++pSymbsInfo) {
                if (pSymbsInfo->DataPerBaseSym[0].sChar[0] != curSym) {
                   break;
                }
            }
            if (i == ET9MAXLDBWORDSIZE) {
                bRet = 1;
            }
        }
    }
    return bRet;
}

#ifdef ET9_DEBUGLOG4
/*---------------------------------------------------------------------------*/
/** \internal
 * This function logs a saved word list.
 *
 * @param pWordSymbInfo             Pointer to word symbol info structure.
 * @param pfLog                     File to log to.
 *
 * @return None
 */

static void ET9LOCALCALL __LogSavedWords(ET9WordSymbInfo   * const pWordSymbInfo,
                                         FILE              * const pfLog)
{
    ET9U16 wIndex;

    ET9SavedInputWords  * const pSavedInputWords = &pWordSymbInfo->Private.sSavedInputWords;

    fprintf(pfLog, "\n--------------------- Saved Words ---------------------\n\n");

    for (wIndex = 0; wIndex < ET9MAXSAVEINPUTWORDS; ++wIndex) {

        ET9SYMB *ps;
        ET9U16 wCount;
        ET9SavedInputSymb *pSymb;
        ET9SavedInputWord * const pWord = &pSavedInputWords->pSavedWords[wIndex];

        if (pWord->wStorePos == UNDEFINED_STORE_INDEX) {
            continue;
        }

        fprintf(pfLog, "[%03d]", wIndex);

        if (wIndex == pSavedInputWords->wCurrInputSaveIndex) {
            fprintf(pfLog, " * ");
        }
        else {
            fprintf(pfLog, " : ");
        }

        fprintf(pfLog, "[%04d]", pWord->wStorePos);

        for (ps = &pSavedInputWords->sWords[pWord->wStorePos], wCount = pWord->wWordLen; wCount; ++ps, --wCount) {
            if (*ps >= 0x20 && *ps <= 0x7F) {
                fprintf(pfLog, "%c", (char)*ps);
            }
            else {
                fprintf(pfLog, "<%x>", (int)*ps);
            }
        }

        switch (pWord->eLastShiftState)
        {
            case ET9NOSHIFT:
                fprintf(pfLog, " NOSHIFT");
                break;
            case ET9SHIFT:
                fprintf(pfLog, " SHIFT");
                break;
            case ET9CAPSLOCK:
                fprintf(pfLog, " CAPS");
                break;
            default:
                fprintf(pfLog, " \?\?\?");
                break;
        }

        fprintf(pfLog, "\n[   ]   ");

        pSymb = &pSavedInputWords->sInputs[pWord->wStorePos];

        for (wCount = pWord->wInputLen; wCount; --wCount, ++pSymb) {

            switch (pSymb->eInputType)
            {
                case ET9DISCRETEKEY:        fprintf(pfLog, "DIS"); break;
                case ET9REGIONALKEY:        fprintf(pfLog, "REG"); break;
                case ET9HANDWRITING:        fprintf(pfLog, "HWR"); break;
                case ET9MULTITAPKEY:        fprintf(pfLog, "MTK"); break;
                case ET9CUSTOMSET:          fprintf(pfLog, "CST"); break;
                case ET9EXPLICITSYM:        fprintf(pfLog, "EXP"); break;
                case ET9MULTISYMBEXPLICIT:  fprintf(pfLog, "MTE"); break;
                default:                    fprintf(pfLog, "Inp<%d>", pWordSymbInfo->SymbsInfo[wIndex].eInputType); break;
            }

            if (pSymb->eInputType == ET9CUSTOMSET || pSymb->eInputType == ET9EXPLICITSYM) {
            }
            else if (pSymb->wTapX != ET9UNDEFINEDTAPVALUE && pSymb->wTapY != ET9UNDEFINEDTAPVALUE) {
                fprintf(pfLog, " [%d,%d %04x:%d %04x:%d]", (int)pSymb->wTapX, (int)pSymb->wTapY, (int)pSymb->wKdb1, (int)pSymb->wPage1, (int)pSymb->wKdb2, (int)pSymb->wPage2);
            }
            else {
                fprintf(pfLog, " (Key %d %04x:%d %04x:%d)", (int)pSymb->wKeyIndex, (int)pSymb->wKdb1, (int)pSymb->wPage1, (int)pSymb->wKdb2, (int)pSymb->wPage2);
            }

            fprintf(pfLog, " (ext %d %d)", (int)pSymb->bTraceProbability, (int)pSymb->bTraceIndex);

            if (pSymb->eInputType == ET9MULTISYMBEXPLICIT) {
                fprintf(pfLog, " (BB %d)", (int)pSymb->bInputIndex);
            }

            if (pSymb->bLocked) {
                fprintf(pfLog, " LP");
            }

            if (pSymb->eShiftState == ET9SHIFT) {
                fprintf(pfLog, " SH");
            }
            else if (pSymb->eShiftState == ET9CAPSLOCK) {
                fprintf(pfLog, " CL");
            }

            fprintf(pfLog, "; ");
        }

        fprintf(pfLog, "\n");
    }

    fprintf(pfLog, "\n-------------------------------------------------------\n\n");

    fflush(pfLog);
}
#else
#define __LogSavedWords(x,y)
#endif

/*---------------------------------------------------------------------------*/
/** \internal
 * Save information about a used word - for later reselection.
 *
 * @param pWordSymbInfo             Pointer to word symbol info structure.
 * @param pString                   Word string to save.
 * @param wLen                      Word length to save.
 *
 * @return None
 */

void ET9FARCALL _ET9SaveWord(ET9WordSymbInfo   * const pWordSymbInfo,
                             ET9SYMB           * const pString,
                             const ET9U16              wLen)
{
    ET9SavedInputWords  * const pSavedInputWords = &pWordSymbInfo->Private.sSavedInputWords;

    ET9Assert(pWordSymbInfo);
    ET9Assert(pString);
    ET9Assert(wLen <= ET9MAXWORDSIZE);

    /* don't save anything on empty input (NWP) */

    if (!pWordSymbInfo->bNumSymbs) {
        return;
    }

    /* find a position for the word */

    if (pWordSymbInfo->Private.bInputRestarted) {

        ET9Assert(pSavedInputWords->wCurrInputSaveIndex < ET9MAXSAVEINPUTWORDS);

        if (pSavedInputWords->pSavedWords[pSavedInputWords->wCurrInputSaveIndex].wStorePos != UNDEFINED_STORE_INDEX) {
            pSavedInputWords->wCurrInputSaveIndex = (ET9U16)((pSavedInputWords->wCurrInputSaveIndex + 1) % ET9MAXSAVEINPUTWORDS);
        }

        pWordSymbInfo->Private.bInputRestarted = 0;
    }

    ET9Assert(pSavedInputWords->wCurrInputSaveIndex < ET9MAXSAVEINPUTWORDS);

    WLOG4(fprintf(pLogFile4, "_ET9SaveWord, saving word @ %d\n", pSavedInputWords->wCurrInputSaveIndex);)

    /* find a position for the data */

    {
        ET9U16          wPos;
        const ET9U16    wSaveLen = __ET9Max(wLen, pWordSymbInfo->bNumSymbs);
        const ET9U16    wPrevWord = (ET9U16)((pSavedInputWords->wCurrInputSaveIndex + (ET9MAXSAVEINPUTWORDS - 1)) % ET9MAXSAVEINPUTWORDS);

        ET9Assert(wPrevWord < ET9MAXSAVEINPUTWORDS);

        wPos = pSavedInputWords->pSavedWords[wPrevWord].wStorePos;

        if (wPos == UNDEFINED_STORE_INDEX) {
            wPos = 0;
        }
        else {
            wPos = wPos + __ET9Max(pSavedInputWords->pSavedWords[wPrevWord].wWordLen, pSavedInputWords->pSavedWords[wPrevWord].wInputLen);
        }

        if (wPos + wSaveLen > ET9SAVEINPUTSTORESIZE) {
            wPos = 0;
        }

        WLOG4(fprintf(pLogFile4, "_ET9SaveWord, using storage pos %d length %d\n", wPos, wSaveLen);)

        /* clear words that already are using this storage */

        {
            const ET9U16        wLastPos = (ET9U16)(wPos + wSaveLen - 1);
            ET9U16              wCount;
            ET9U16              wWordLastPos;
            ET9SavedInputWord   *pWord;

            pWord = pSavedInputWords->pSavedWords;

            for (wCount = ET9MAXSAVEINPUTWORDS; wCount; --wCount, ++pWord) {

                if (pWord->wStorePos == UNDEFINED_STORE_INDEX) {
                    continue;
                }

                wWordLastPos = (ET9U16)(pWord->wStorePos + __ET9Max(pWord->wWordLen, pWord->wInputLen) - 1);

                if (pWord->wStorePos >= wPos && pWord->wStorePos <= wLastPos ||
                    wWordLastPos     >= wPos && wWordLastPos     <= wLastPos) {

                    pWord->wStorePos = UNDEFINED_STORE_INDEX;
                }
            }
        }

        /* assign pos */

        pSavedInputWords->pSavedWords[pSavedInputWords->wCurrInputSaveIndex].wStorePos = wPos;
    }

    /* save input */

    {
        ET9U16 wCount;
        ET9SymbInfo *pSymbInfo;
        ET9SavedInputSymb *pSaveSymb;
        ET9SavedInputWord * const pWord = &pSavedInputWords->pSavedWords[pSavedInputWords->wCurrInputSaveIndex];

        if (pWordSymbInfo->Private.bRequiredLocate) {
            pWord->eLastShiftState = pWordSymbInfo->Private.eRequiredLastShiftState;
        }
        else {
            pWord->eLastShiftState = pWordSymbInfo->Private.eLastShiftState;
        }

        pWord->wWordLen = wLen;
        _ET9SymCopy(&pSavedInputWords->sWords[pWord->wStorePos], pString, wLen);

        pWord->wInputLen = pWordSymbInfo->bNumSymbs;

        pSaveSymb = &pSavedInputWords->sInputs[pWord->wStorePos];
        pSymbInfo = pWordSymbInfo->SymbsInfo;

        for (wCount = pWordSymbInfo->bNumSymbs; wCount; --wCount, ++pSaveSymb, ++pSymbInfo) {

            pSaveSymb->wTapX                = pSymbInfo->wTapX;
            pSaveSymb->wTapY                = pSymbInfo->wTapY;
            pSaveSymb->wKeyIndex            = pSymbInfo->wKeyIndex;
            pSaveSymb->bInputIndex          = (ET9U8)pSymbInfo->wInputIndex;
            pSaveSymb->bLocked              = pSymbInfo->bLocked;
            pSaveSymb->eInputType           = pSymbInfo->eInputType;
            pSaveSymb->eShiftState          = pSymbInfo->eShiftState;
            pSaveSymb->bForcedLowercase     = pSymbInfo->bForcedLowercase;
            pSaveSymb->bTraceProbability    = pSymbInfo->bTraceProbability;
            pSaveSymb->bTraceIndex          = pSymbInfo->bTraceIndex;
            pSaveSymb->wKdb1                = pSymbInfo->wKdb1;
            pSaveSymb->wPage1               = pSymbInfo->wPage1;
            pSaveSymb->wKdb2                = pSymbInfo->wKdb2;
            pSaveSymb->wPage2               = pSymbInfo->wPage2;
            pSaveSymb->sSymb                = pSymbInfo->DataPerBaseSym[0].sChar[0];

            /* e.g. all sets should have the highest freq symb first, and we only save one of them... */
        }
    }

    /* log */

    __LogSavedWords(pWordSymbInfo, pLogFile4);
}

/*---------------------------------------------------------------------------*/
/** \internal
 * This function explicifies a word.
 *
 * @param pWordSymbInfo             Pointer to word symbol info structure.
 * @param pWord                     Word to explicify.
 *
 * @return None
 */

void ET9FARCALL _ET9ExplicifyWord(ET9WordSymbInfo   * const pWordSymbInfo,
                                  ET9SimpleWord     * const pWord)
{
    ET9STATUS   wStatus;
    ET9U16      wCount;
    ET9SYMB     *psSymb;

    ET9Assert(pWord);
    ET9Assert(pWordSymbInfo);

    WLOG4(fprintf(pLogFile4, "_ET9ExplicifyWord\n");)

    /* validate */

    if (!pWord->wLen) {
        ET9ClearAllSymbs(pWordSymbInfo);
        return;
    }

    ET9Assert(!pWord->wCompLen);

    /* enter using explicits */

    __ClearAllSymbs(pWordSymbInfo, 0);

    psSymb = pWord->sString;

    for (wCount = pWord->wLen; wCount; --wCount, ++psSymb) {

        wStatus = ET9AddExplicitSymb(pWordSymbInfo,
                                     *psSymb,
                                     (_ET9SymIsUpper(*psSymb, pWordSymbInfo->Private.wLocale) ? ET9SHIFT : ET9NOSHIFT),
                                     ET9_NO_ACTIVE_INDEX);

        if (wStatus) {

            WLOG4(fprintf(pLogFile4, "_ET9ExplicifyWord, explicit symb failed\n");)

            ET9ClearAllSymbs(pWordSymbInfo);
            return;
        }
    }

    ET9Assert(pWordSymbInfo->bNumSymbs == pWord->wLen);

    /* potentially prevent catchup builds etc */

    _ET9ContentExplicified(pWordSymbInfo);
}

/*---------------------------------------------------------------------------*/
/**
 * @brief Enables the next locking feature.
 *
 * @param[in]     pWordSymbInfo       Pointer to the Word Symbol Data Structure (ET9WordSymbInfo).
 *
 * @retval ET9STATUS_NONE               Function call was handled successfully.
 * @retval ET9STATUS_INVALID_MEMORY     At least one pointer passed as a function argument was set to null.
 *
 * @remarks Next locking limits the results in the selection list to words that begin with specifed characters.<br>
 * By default, next locking is disabled.
 *
 * @see ET9ClearNextLocking()
 */

ET9STATUS ET9FARCALL ET9SetNextLocking(ET9WordSymbInfo * const pWordSymbInfo)
{
    if (!pWordSymbInfo || pWordSymbInfo->wInitOK != ET9GOODSETUP) {
        return ET9STATUS_INVALID_MEMORY;
    }

    pWordSymbInfo->dwStateBits |= ET9STATENEXTLOCKINGMASK;

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/**
 * @brief Disables the next locking feature.
 *
 * @param[in]     pWordSymbInfo       Pointer to the Word Symbol Data Structure (ET9WordSymbInfo).
 *
 * @retval ET9STATUS_NONE               Function call was handled successfully.
 * @retval ET9STATUS_INVALID_MEMORY     At least one pointer passed as a function argument was set to null.
 *
 * @remarks Next locking limits the results in the selection list to words that begin with specifed characters.<br>
 * By default, next locking is disabled.
 *
 * @see ET9SetNextLocking()
 */

ET9STATUS ET9FARCALL ET9ClearNextLocking(ET9WordSymbInfo * const pWordSymbInfo)
{
    if (!pWordSymbInfo || pWordSymbInfo->wInitOK != ET9GOODSETUP) {
        return  ET9STATUS_INVALID_MEMORY;
    }

    pWordSymbInfo->dwStateBits &= ~(ET9STATENEXTLOCKINGMASK);

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/**
 * @brief Initializes the the Word Symbol Data Structure.
 *
 * @param[in]     pWordSymbInfo        Pointer to the Word Symbol Data Structure (ET9WordSymbInfo), which contains information about the input the user has provided.
 * @param[out]    bResetWordSymbInfo   Reset indicator used to force resetting a previously initialized Word Symbol Data Structure.
 *
 * @retval ET9STATUS_NONE   Function call was handled successfully.
 * @retval ET9STATUS_INVALID_MEMORY At least one pointer passed as a function argument was set to null.
 */

ET9STATUS ET9FARCALL ET9WordSymbInit(ET9WordSymbInfo * const pWordSymbInfo,
                                     const ET9BOOL           bResetWordSymbInfo)
{
    /* validate parameters */

    if (!pWordSymbInfo) {
        return ET9STATUS_INVALID_MEMORY;
    }

    /* init needed? */

    if (pWordSymbInfo->wInitOK == ET9GOODSETUP && !bResetWordSymbInfo) {
        return ET9STATUS_NONE;
    }

    /* allowed to init? (can't break edition references by this init call) */

    if (pWordSymbInfo->wInitOK == ET9GOODSETUP) {

        ET9U8               bCount;
        ET9BaseLingInfo     **ppBase;

        ppBase = pWordSymbInfo->Private.ppEditionsList;
        for (bCount = ET9MAXEDITIONS; bCount; --bCount, ++ppBase) {

            if (*ppBase) {
                return ET9STATUS_ERROR;
            }
        }
    }

    /* actual init */

    pWordSymbInfo->wInitOK = 0;

    return _ET9WordSymbInit(pWordSymbInfo);
}

/*---------------------------------------------------------------------------*/
/**
 * @brief Sets the Keyboard Input Module to an unshifted (or lowercase) shift state.
 *
 * @param[in]     pWordSymbInfo     Pointer to the Word Symbol Data Structure (ET9WordSymbInfo), which contains information about the input the user has provided.
 *
 * @retval ET9STATUS_NONE               Function call was handled successfully.
 * @retval ET9STATUS_INVALID_MEMORY     At least one pointer passed as a function argument was set to null.
 * @retval ET9STATUS_INVALID_KDB_PAGE   The page specified by wPageNum is not valid for the active keyboard.
 * @retval ET9STATUS_NO_KEY             No keys are associated with the keyboard page specified by wPageNum.
 * @retval ET9STATUS_READ_DB_FAIL       The XT9 core cannot read data from the keyboard. This typically indicates a problem with the ET9KDBREADCALLBACK function you have implemented.
 *
 * @see ET9SetCapsLock(), ET9SetShift()
 */

ET9STATUS ET9FARCALL ET9SetUnShift(ET9WordSymbInfo * const pWordSymbInfo)
{
    if (!pWordSymbInfo || pWordSymbInfo->wInitOK != ET9GOODSETUP) {
        return ET9STATUS_INVALID_MEMORY;
    }

    /* Make sure autocap status is off at this point... user overridden */

    if (!pWordSymbInfo->bNumSymbs) {
       pWordSymbInfo->Private.eAutocapWord = ET9AUTOCAP_OFF;
    }

    /* Make sure downshifting logic at this point... user overridden */

    if (pWordSymbInfo->Private.eLastShiftState != ET9NOSHIFT) {
        pWordSymbInfo->Private.bCompoundingDownshift = 0;
    }

    pWordSymbInfo->dwStateBits &= ~(ET9STATE_SHIFT_MASK | ET9STATE_CAPS_MASK);
    pWordSymbInfo->Private.eLastShiftState = ET9NOSHIFT;

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/**
 * @brief Sets the Keyboard Input Module to a shifted (or single-character shifted) shift state.
 *
 * @param[in]     pWordSymbInfo     Pointer to the Word Symbol Data Structure (ET9WordSymbInfo), which contains information about the input the user has provided.
 *
 * @retval ET9STATUS_NONE               Function call was handled successfully.
 * @retval ET9STATUS_INVALID_MEMORY     At least one pointer passed as a function argument was set to null.
 * @retval ET9STATUS_INVALID_KDB_PAGE   The page specified by wPageNum is not valid for the active keyboard.
 * @retval ET9STATUS_NO_KEY             No keys are associated with the keyboard page specified by wPageNum.
 * @retval ET9STATUS_READ_DB_FAIL       The XT9 core cannot read data from the keyboard. This typically indicates a problem with the ET9KDBREADCALLBACK function you have implemented.
 *
 * @see ET9SetCapsLock(), ET9SetUnShift()
 */

ET9STATUS ET9FARCALL ET9SetShift(ET9WordSymbInfo * const pWordSymbInfo)
{
    if (!pWordSymbInfo || pWordSymbInfo->wInitOK != ET9GOODSETUP) {
        return ET9STATUS_INVALID_MEMORY;
    }

    /* don't reset the insert mask here! */
    /* if shift request received immediately after reporting autocap scenario */

    if (pWordSymbInfo->Private.eAutocapWord == ET9AUTOCAP_PENDING) {

       /* upgrade status for word */

       pWordSymbInfo->Private.eAutocapWord = ET9AUTOCAP_APPLIED;
    }

    /* Make sure downshifting logic at this point... user overridden */

    pWordSymbInfo->Private.bCompoundingDownshift = 0;
    pWordSymbInfo->dwStateBits &= ~(ET9STATE_CAPS_MASK);
    pWordSymbInfo->dwStateBits |= ET9STATE_SHIFT_MASK;
    pWordSymbInfo->Private.eLastShiftState = ET9SHIFT;

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/**
 * @brief Sets the Keyboard Input Module to a caps lock (or uppercase) shift state.
 *
 * @param[in]     pWordSymbInfo         Pointer to the Word Symbol Data Structure (ET9WordSymbInfo), which contains information about the input the user has provided.
 *
 * @retval ET9STATUS_NONE               Function call was handled successfully.
 * @retval ET9STATUS_INVALID_MEMORY     At least one pointer passed as a function argument was set to null.
 * @retval ET9STATUS_INVALID_KDB_PAGE   The page specified by wPageNum is not valid for the active keyboard.
 * @retval ET9STATUS_NO_KEY             No keys are associated with the keyboard page specified by wPageNum.
 * @retval ET9STATUS_READ_DB_FAIL       The XT9 core cannot read data from the keyboard. This typically indicates a problem with the ET9KDBREADCALLBACK function you have implemented.
 *
 * @see ET9SetShift(), ET9SetUnShift()
 */

ET9STATUS ET9FARCALL ET9SetCapsLock(ET9WordSymbInfo * const pWordSymbInfo)
{
    if (!pWordSymbInfo || pWordSymbInfo->wInitOK != ET9GOODSETUP) {
        return ET9STATUS_INVALID_MEMORY;
    }

    /* don't reset the insert mask here! */
    /* Make sure autocap status is off at this point... user overridden */

    pWordSymbInfo->Private.eAutocapWord = ET9AUTOCAP_OFF;

    /* Make sure downshift logic disabled at this point... user overridden */

    pWordSymbInfo->Private.bCompoundingDownshift = 0;
    pWordSymbInfo->dwStateBits &= ~(ET9STATE_SHIFT_MASK);
    pWordSymbInfo->dwStateBits |= ET9STATE_CAPS_MASK;
    pWordSymbInfo->Private.eLastShiftState = ET9CAPSLOCK;

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Decides if the input has trace info.
 *
 * @param[in]     pWordSymbInfo         Pointer to the Word Symbol Data Structure (ET9WordSymbInfo), which contains information about the input the user has provided.
 *
 * @return Non zero if has, otherwise zero.
 */

ET9BOOL ET9FARCALL _ET9HasTraceInfo(ET9WordSymbInfo const * const pWordSymbInfo)
{
    ET9UINT nCount;
    ET9SymbInfo const *pSymbInfo;

    if (!pWordSymbInfo || pWordSymbInfo->wInitOK != ET9GOODSETUP) {
        return 0;
    }

    pSymbInfo = pWordSymbInfo->SymbsInfo;
    for (nCount = pWordSymbInfo->bNumSymbs; nCount; --nCount, ++pSymbInfo) {
        if (pSymbInfo->bTraceIndex) {
            return 1;
        }
    }

    return 0;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Decides if the input has regional info.
 *
 * @param[in]     pWordSymbInfo         Pointer to the Word Symbol Data Structure (ET9WordSymbInfo), which contains information about the input the user has provided.
 *
 * @return Non zero if has, otherwise zero.
 */

ET9BOOL ET9FARCALL _ET9HasRegionalInfo(ET9WordSymbInfo const * const pWordSymbInfo)
{
    ET9UINT nCount;
    ET9SymbInfo const *pSymbInfo;

    if (!pWordSymbInfo || pWordSymbInfo->wInitOK != ET9GOODSETUP) {
        return 0;
    }

    pSymbInfo = pWordSymbInfo->SymbsInfo;
    for (nCount = pWordSymbInfo->bNumSymbs; nCount; --nCount, ++pSymbInfo) {

        switch (pSymbInfo->eInputType)
        {
            case ET9REGIONALKEY:
                return 1;
            default:
                break;
        }

        /* check custom set */

        if (pSymbInfo->bNumBaseSyms > 1) {

            ET9U8 bSymFreq;
            ET9UINT nBaseCount;
            ET9DataPerBaseSym const *pDPBS;

            nBaseCount = pSymbInfo->bNumBaseSyms;
            pDPBS = &pSymbInfo->DataPerBaseSym[0];

            bSymFreq = pDPBS->bSymFreq;

            --nBaseCount;
            ++pDPBS;

            for (; nBaseCount; --nBaseCount, ++pDPBS) {
                if (pDPBS->bSymFreq != bSymFreq) {
                    return 1;
                }
            }
        }
    }

    return 0;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Decides if the input has discrete only info.
 *
 * @param[in]     pWordSymbInfo         Pointer to the Word Symbol Data Structure (ET9WordSymbInfo), which contains information about the input the user has provided.
 *
 * @return Non zero if has, otherwise zero.
 */

ET9BOOL ET9FARCALL _ET9HasDiscreteOnlyInfo(ET9WordSymbInfo const * const pWordSymbInfo)
{
    ET9UINT nCount;
    ET9SymbInfo const *pSymbInfo;

    if (!pWordSymbInfo || pWordSymbInfo->wInitOK != ET9GOODSETUP) {
        return 0;
    }

    pSymbInfo = pWordSymbInfo->SymbsInfo;
    for (nCount = pWordSymbInfo->bNumSymbs; nCount; --nCount, ++pSymbInfo) {

        switch (pSymbInfo->eInputType)
        {
            case ET9DISCRETEKEY:
                continue;
            case ET9CUSTOMSET:
                break;
            default:
                return 0;
        }

        /* check custom set */

        if (pSymbInfo->bNumBaseSyms) {

            ET9UINT nBaseCount;
            ET9DataPerBaseSym const *pDPBS;
            const ET9U8 bSymFreq = pSymbInfo->DataPerBaseSym[0].bSymFreq;

            pDPBS = &pSymbInfo->DataPerBaseSym[1];
            for (nBaseCount = pSymbInfo->bNumBaseSyms - 1; nBaseCount; --nBaseCount, ++pDPBS) {
                if (pDPBS->bSymFreq != bSymFreq) {
                    return 0;
                }
            }
        }
    }

    return 1;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Decides if the input has only shifted info.
 *
 * @param[in]     pWordSymbInfo         Pointer to the Word Symbol Data Structure (ET9WordSymbInfo), which contains information about the input the user has provided.
 *
 * @return Non zero if has, otherwise zero.
 */

ET9BOOL ET9FARCALL _ET9HasAllShiftedInfo(ET9WordSymbInfo const * const pWordSymbInfo)
{
    ET9UINT nCount;
    ET9SymbInfo const *pSymbInfo;

    if (!pWordSymbInfo || pWordSymbInfo->wInitOK != ET9GOODSETUP) {
        return 0;
    }

    pSymbInfo = pWordSymbInfo->SymbsInfo;
    for (nCount = pWordSymbInfo->bNumSymbs; nCount; --nCount, ++pSymbInfo) {

        switch (pSymbInfo->bSymbType)
        {
            case ET9KTPUNCTUATION:
            case ET9KTNUMBER:
            case ET9KTSTRING:
            case ET9KTSMARTPUNCT:
                continue;
        }

        if (!pSymbInfo->eShiftState) {
            return 0;
        }
    }

    return 1;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Decides if the input history indicates that tap input shouldn't be overridden after trace input (trace correction).
 *
 * @param[in]     pWordSymbInfo         Pointer to the Word Symbol Data Structure (ET9WordSymbInfo), which contains information about the input the user has provided.
 *
 * @return Non zero if is, otherwise zero.
 */

ET9BOOL ET9FARCALL _ET9IsInhibitTapOverrideAfterTrace(ET9WordSymbInfo const * const pWordSymbInfo)
{
    const ET9UINT nDecrementCount = ET9_INPUT_TRACK_SIZE - 1;

    ET9UINT nCurrIndex = pWordSymbInfo->Private.bCurrInputTrackIndex;

    nCurrIndex = (nCurrIndex + nDecrementCount) % ET9_INPUT_TRACK_SIZE;

    if (pWordSymbInfo->Private.peInputTracks[nCurrIndex] != ET9InputTrack_trace) {
        return 0;
    }

    nCurrIndex = (nCurrIndex + nDecrementCount) % ET9_INPUT_TRACK_SIZE;

    if (pWordSymbInfo->Private.peInputTracks[nCurrIndex] != ET9InputTrack_trace) {
        return 0;
    }

    if (_ET9HasTraceInfo(pWordSymbInfo)) {
        return 0;
    }

    return 1;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Decides if the input history indicates that delayed reorder should be inhibited.
 *
 * @param[in]     pWordSymbInfo         Pointer to the Word Symbol Data Structure (ET9WordSymbInfo), which contains information about the input the user has provided.
 *
 * @return Non zero if is, otherwise zero.
 */

ET9BOOL ET9FARCALL _ET9IsInhibitDelayedReorderAfterTrace(ET9WordSymbInfo const * const pWordSymbInfo)
{
    ET9UINT nTraceCount = 0;

    ET9UINT nIndex;

    for (nIndex = 0; nIndex < ET9_INPUT_TRACK_SIZE; ++nIndex) {
        if (pWordSymbInfo->Private.peInputTracks[nIndex] == ET9InputTrack_trace) {
            ++nTraceCount;
        }
    }

    if (nTraceCount < 2) {
        return 0;
    }

    if (_ET9HasTraceInfo(pWordSymbInfo)) {
        return 0;
    }

    return 1;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Decides if settings are inhibited.
 *
 * @param[in]     pWordSymbInfo         Pointer to the Word Symbol Data Structure (ET9WordSymbInfo), which contains information about the input the user has provided.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL _ET9SettingsInhibited(ET9WordSymbInfo const * const pWordSymbInfo)
{
    if (!pWordSymbInfo || pWordSymbInfo->wInitOK != ET9GOODSETUP) {
        return ET9STATUS_INVALID_MEMORY;
    }

    if (pWordSymbInfo->Private.bRequiredLocate && pWordSymbInfo->bNumSymbs) {

#ifdef ET9_VERIFY_INHIBITED_SETTINGS
        ET9Assert(0);
#endif

        return ET9STATUS_SETTINGS_INHIBITED;
    }

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Set the current auto locale.
 *
 * @param pWordSymbInfo             Pointer to word symbol info structure.
 * @param wLocale                   Locale to set.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL _ET9_SetAutoLocale(ET9WordSymbInfo   * const pWordSymbInfo,
                                        const ET9U16              wLocale)
{
    if (!pWordSymbInfo || pWordSymbInfo->wInitOK != ET9GOODSETUP) {
        return ET9STATUS_INVALID_MEMORY;
    }

    if (!pWordSymbInfo->Private.bManualLocale) {
        pWordSymbInfo->Private.wLocale = wLocale;
    }

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Get the default locale.
 *
 * @param pwLocale                  Where the default locale goes.
 * @param pbManual                  Where the default manual goes.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

void ET9FARCALL _ET9_GetDefaultLocale(ET9U16 * const pwLocale, ET9BOOL * const pbManual)
{
#if defined(ET9_CHINESE_MODULE)
    *pwLocale = ET9PLIDChineseTraditional;
    *pbManual = 1;
#elif defined(ET9_KOREAN_MODULE)
    *pwLocale = ET9PLIDKorean;
    *pbManual = 1;
#else
    *pwLocale = 0;
    *pbManual = 0;
#endif
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Clear all shift info from the current input (not changing the current shift state).
 *
 * @param pWordSymbInfo             Pointer to word symbol info structure.
 *
 * @return None
 */

void ET9FARCALL _ET9ClearShiftInfo(ET9WordSymbInfo * const pWordSymbInfo)
{
    ET9UINT nCount;
    ET9SymbInfo *pSymbInfo;

    if (!pWordSymbInfo || pWordSymbInfo->wInitOK != ET9GOODSETUP) {
        return;
    }

    pSymbInfo = pWordSymbInfo->SymbsInfo;
    for (nCount = pWordSymbInfo->bNumSymbs; nCount; --nCount, ++pSymbInfo) {
        pSymbInfo->eShiftState = ET9NOSHIFT;
        pSymbInfo->bAutoDowncase = 0;
        pSymbInfo->bForcedLowercase = 0;
    }
}

/*! @} */
/* ----------------------------------< eof >--------------------------------- */
