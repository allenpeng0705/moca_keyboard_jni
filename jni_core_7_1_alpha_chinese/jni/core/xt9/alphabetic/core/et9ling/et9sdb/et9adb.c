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
;**     FileName: et9adb.c                                                    **
;**                                                                           **
;**  Description: ET9 Alpha Supplemental Database Module                      **
;**                                                                           **
;*******************************************************************************
;******************************************************************************/

/*! \addtogroup et9adb Supplemental database for alphabetic
* XT9 alphabetic supplemental database features.
* @{
*/

#include "et9api.h"

#ifdef ET9_ALPHABETIC_MODULE
#include "et9asys.h"
#include "et9sym.h"
#include "et9aslst.h"
#include "et9amisc.h"
#include "et9arudb.h"
#include "et9acdb.h"
#include "et9aasdb.h"
#include "et9amdb.h"
#include "et9adb.h"
#include "et9alsasdb.h"
#include "et9aspc.h"
#include "et9imu.h"

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

/*---------------------------------------------------------------------------*/
/** \internal
 * Search the shaped words for a match to the current input set.
 *
 * @param pLingInfo        Pointer to alpha information structure.
 * @param wIndex           Index of beginning of active word segment.
 * @param wLength          Length of active sequence.
 * @param bFreqIndicator   Freq (for _ET9AWSelLstWordSearch callback).
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

static ET9STATUS ET9LOCALCALL __ET9ShapedWordsSearch(ET9AWLingInfo         * const pLingInfo,
                                                     ET9U16                        wIndex,
                                                     ET9U16                        wLength,
                                                     ET9_FREQ_DESIGNATION          bFreqIndicator)
{
    ET9AWLingCmnInfo   * const pLingCmnInfo = pLingInfo->pLingCmnInfo;
    ET9WordSymbInfo    * const pWordSymbInfo = pLingCmnInfo->Base.pWordSymbInfo;

    ET9Assert(pLingInfo != NULL);

    /* -------------------- single digit additions --------------------------*/

    /* using the KDB to place single digits at end of selection list (limited to ascii digits 0-9) */
    /* note: not sure if Upper case chars need to be checked as well... */

    if (wLength == 1) {

        ET9SYMB *pLower;
        ET9U16 wBaseIndex;
        ET9U16 wSymsIndex;
        ET9U16 wFreqIndex = 0;
        ET9DataPerBaseSym *pDPBS;
        ET9SymbInfo * const pSymbInfo = &pWordSymbInfo->SymbsInfo[wIndex];
        ET9BOOL bBilingualSupport = ET9AWSys_GetBilingualSupported(pLingInfo) ? 1 : 0;

        wBaseIndex = 0;
        pDPBS = pSymbInfo->DataPerBaseSym;
        for (; wBaseIndex < pSymbInfo->bNumBaseSyms; ++wBaseIndex, ++pDPBS) {

            pLower = pDPBS->sChar;
            for (wSymsIndex = 0; wSymsIndex < pDPBS->bNumSymsToMatch; ++wSymsIndex, ++pLower) {

                if (*pLower <= 0xFF && _ET9_IsNumeric(*pLower)) {

                    ET9AWPrivWordInfo sLocalWord;

                    _InitPrivWordInfo(&sLocalWord);

                    sLocalWord.Base.wWordLen = 1;
                    sLocalWord.Base.sWord[0] = *pLower;
                    sLocalWord.bWordSrc = ADDON_FROM_FREQ_IND(bFreqIndicator, ET9WORDSRC_KDB);
                    if (pLingInfo->pLingCmnInfo->Private.eCurrSelectionListMode == ET9ASLMODE_MIXED) {
                        sLocalWord.xWordFreq = (ET9FREQPART)(pDPBS->bNumSymsToMatch - wFreqIndex++);
                    }
                    else {
                        sLocalWord.xWordFreq = (ET9FREQPART)(1000 - wFreqIndex++);
                    }

                    if (bBilingualSupport) {
                        sLocalWord.Base.bLangIndex = ET9AWBOTH_LANGUAGES;
                    }
                    else {
                        sLocalWord.Base.bLangIndex = ET9AWFIRST_LANGUAGE;
                    }

                    _ET9AWSelLstWordSearch(pLingInfo, &sLocalWord, wIndex, wLength, bFreqIndicator);
                }
            }
        }
    }

    /* -------------------- single letter additions --------------------------*/

    /* using the KDB to place single top symbol in the list as if it came from the LDB (NWP reordering issue) */

    if (wLength == 1) {

        ET9BOOL bFound;

        ET9SymbInfo * const pSymbInfo = &pWordSymbInfo->SymbsInfo[wIndex];
        const ET9SYMB sSymb = pSymbInfo->eShiftState ? *pSymbInfo->DataPerBaseSym->sUpperCaseChar : *pSymbInfo->DataPerBaseSym->sChar;

        if (pSymbInfo->bSymbType == ET9KTLETTER ||
            pSymbInfo->bSymbType == ET9KTUNKNOWN) {

            bFound = (ET9BOOL)!_ET9_IsPunctOrNumeric(sSymb);
        }
        else {
            bFound = 0;
        }

        if (bFound) {

            ET9AWPrivWordInfo sLocalWord;

            _InitPrivWordInfo(&sLocalWord);

            sLocalWord.Base.wWordLen = 1;
            sLocalWord.Base.sWord[0] = sSymb;
            sLocalWord.bWordSrc = ADDON_FROM_FREQ_IND(bFreqIndicator, ET9WORDSRC_LDB);      /* must use this source to work properly */

            _ET9AWSelLstWordSearch(pLingInfo, &sLocalWord, wIndex, wLength, bFreqIndicator);
        }
    }

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Supplemental database search.
 * This function coordinates search from all of the supplemental databases (RUDB, CDB, MDB, ...)
 *
 * @param pLingInfo        Pointer to alpha information structure.
 * @param wIndex           Index of beginning of active word segment.
 * @param wLength          Length of active sequence.
 * @param pbSuppEntries    Pointer to return parameter holding number of found/added.
 * @param bFreqIndicator   Freq (for _ET9AWSelLstWordSearch callback).
 * @param bSourceTypes     With or without auto subst or both.
 * @param bSpcMode         Current spellcheck mode (for _ET9AWSelLstWordSearch callback).
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL _ET9AWSuppDBSelListBuild(ET9AWLingInfo           * const pLingInfo,
                                              const ET9U16                    wIndex,
                                              const ET9U16                    wLength,
                                              ET9U8                   * const pbSuppEntries,
                                              const ET9_FREQ_DESIGNATION      bFreqIndicator,
                                              const ET9U8                     bSourceTypes,
                                              const ET9U8                     bSpcMode)
{
    ET9AWLingCmnInfo * const pLingCmnInfo = pLingInfo->pLingCmnInfo;

    ET9STATUS   wStatus;
    ET9U16      wStartValue = pLingCmnInfo->Private.wTotalWordInserts;

    *pbSuppEntries = 0;

    /* major problems if any passed pointer params are NULL */

    ET9Assert(pLingInfo);
    ET9Assert(pbSuppEntries);

    /* if no context and no active word */

    if (!pLingCmnInfo->Private.bContextWordSize && !wLength) {
        return ET9STATUS_NONE;
    }

    /* init flex matching */

    wStatus = _ET9AWCalcEditDistanceInit(pLingInfo, wIndex, wLength, bSpcMode);

    /* apply sources */

    if (bSourceTypes & ET9SUPPDB_NONAS_SOURCES) {

        /* disabling CDB search in initial JXT9 release */

        if ((pLingCmnInfo->wLdbNum & ET9PLIDMASK) != ET9PLIDJapanese) {

            if (pLingCmnInfo->Private.bContextWordSize) {

                /* Call CDB module for build */

                if (!wStatus) {
                    wStatus = _ET9AWCDBWordsSearch(pLingInfo, wIndex, wLength, bFreqIndicator);
                }
            }
        }

        if (wLength) {

            if (!wStatus) {
                wStatus = _ET9AWRUDBWordsSearch(pLingInfo, 0, wIndex, wLength, bFreqIndicator);
            }
        }

        /* Call MDB module for build */

        if (!wStatus) {
            wStatus = _ET9AWMdbWordsSearch(pLingInfo, wIndex, wLength, bFreqIndicator);
        }

        /* disabling shaped words search in initial JXT9 release */

        if ((pLingCmnInfo->wLdbNum & ET9PLIDMASK) != ET9PLIDJapanese) {

            /* Call shaped words module for build */

            if (!wStatus) {
                wStatus = __ET9ShapedWordsSearch(pLingInfo, wIndex, wLength, bFreqIndicator);
            }
        }
    }

    if (bSourceTypes & ET9SUPPDB_AS_SOURCES && (!wIndex || bFreqIndicator == FREQ_BUILDSPACE)) {

        /* skip searching ASDB for buildarounds (with index != 0) */

        if (wLength && ET9USERDEFINEDAUTOSUBENABLED(pLingCmnInfo)) {

            if (!wStatus) {
                wStatus = _ET9AWASDBWordsSearch(pLingInfo, wIndex, wLength, bFreqIndicator);
            }
        }

        /* skip searching LDB-AS for buildarounds (with index != 0) */

        if (wLength && ET9LDBSUPPORTEDAUTOSUBENABLED(pLingCmnInfo) && ((pLingCmnInfo->wLdbNum & ET9PLIDMASK) != ET9PLIDNone)) {

            if (!wStatus) {
                wStatus = _ET9AWLdbASWordsSearch(pLingInfo, pLingCmnInfo->wLdbNum, wIndex, wLength, bFreqIndicator);
            }
        }
    }

    /* return the number of entries added to the selection list (proper value in most cases, */
    /* != 0 is the most important part) */

    if (pLingCmnInfo->Private.wTotalWordInserts - wStartValue > 0xFF) {
        *pbSuppEntries = 0xFF;
    }
    else {
        *pbSuppEntries = (ET9U8)(pLingCmnInfo->Private.wTotalWordInserts - wStartValue);
    }

    _ET9AWCalcEditDistanceDone(pLingInfo);

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * USE_PROCESSING of word for addition to RDB.
 *
 * @param pLingInfo       Pointer to alpha information structure.
 * @param pHolder         Word to be added.
 * @param bFirstLang      Indication of 1st LDB (if set) or 2nd LDB (if 0).
 * @param bRedundantCheck Indication of whether USE_PROCESSED for LDB already.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

static ET9STATUS ET9LOCALCALL __ET9AWUseProcessWord(ET9AWLingInfo     *pLingInfo,
                                                    ET9AWPrivWordInfo *pHolder,
                                                    ET9U8              bFirstLang,
                                                    ET9U8              bRedundantCheck)
{
    ET9AWLingCmnInfo * const pLingCmnInfo = pLingInfo->pLingCmnInfo;
    ET9STATUS wTmpStatus = ET9STATUS_NONE;
    ET9STATUS wStatus = ET9STATUS_NONE;
    ET9U16    wLdbNum;
    ET9U8     bExact = 0;
    ET9U8     bLowercase = 0;

    ET9Assert(pLingInfo);
    ET9Assert(pHolder);

    if (pHolder->Base.wWordLen < 2) {
        return ET9STATUS_NONE;
    }

    if (bFirstLang) {
        wLdbNum = pLingCmnInfo->wFirstLdbNum;
    }
    else {
        wLdbNum = pLingCmnInfo->wSecondLdbNum;
    }

    wTmpStatus = _ET9AWLdbFind(pLingInfo, wLdbNum, pHolder, &bExact, &bLowercase, 0);

    if ((wTmpStatus == ET9STATUS_WORD_EXISTS) ||
        (GETBASESRC(pHolder->bWordSrc) == ET9WORDSRC_ASDB_SHORTCUT && !bRedundantCheck) ||
        GETBASESRC(pHolder->bWordSrc) == ET9WORDSRC_LAS_SHORTCUT) {

        /* if exists 'as is' in LDB, or is in ASDB, put in RDB */

        if (bExact || wTmpStatus != ET9STATUS_WORD_EXISTS) {

            /* if shortcut, go get original version */

            if (wTmpStatus != ET9STATUS_WORD_EXISTS) {

                if (GETBASESRC(pHolder->bWordSrc) == ET9WORDSRC_ASDB_SHORTCUT) {

                    if (_ET9AWFindASDBObject(pLingInfo, pHolder->Base.sWord, pHolder->Base.wWordLen, 0, 1)) {
                        bExact = 1;
                    }
                }
                else if (_ET9AWFindLdbASObject(pLingInfo, wLdbNum, pHolder->Base.sWord, pHolder->Base.wWordLen, 0, 1)) {
                    bExact = 1;
                }
            }

            if (bExact) {

                pHolder->bWordSrc = ET9WORDSRC_LDB;

                wStatus = _ET9AWRDBAddWord(pLingInfo, pHolder, ET9RD_WORD_INIT_FREQ, (ET9U8)(wLdbNum & ET9PLIDMASK), bExact, bLowercase);
            }
            else {

                /* big problem... was found before and now it wasn't?   */
                /* ok if expand ASDB in list is turned ON */
                /* a potentially strange OTFM can prevent finding the shortcut as well (especially when e.g. mapping to puncts and puncts are removed before coming here) */

                ET9Assert(pLingInfo->Private.pConvertSymb || pLingInfo->pLingCmnInfo->Private.bExpandAsDuringBuild);
            }
        }

        /* if not exact, but in LDB, MAY be in UDB as distinct form... */
        /* if so, bump freq (will not get bumped in SPACE processing)  */

        else if (_ET9AWFindRUDBObject(pLingInfo, pHolder, ET9UDBTYPE, 1) && !bRedundantCheck) {
        }
        else if (bLowercase) {

            /* just go ahead and send; _ET9AWRDBAddWord will lowercase based on flag */

            wStatus = _ET9AWRDBAddWord(pLingInfo, pHolder, ET9RD_WORD_INIT_FREQ, (ET9U8)(wLdbNum & ET9PLIDMASK), 0, bLowercase);
        }
        else {

            /* if it exists in the LDB, but not in lowercase, and doesn't match input, */
            /* retrieve the form from the LDB and send that to the RDB                 */

            if (_ET9AWLdbFind(pLingInfo, wLdbNum, pHolder, &bExact, &bLowercase, 1) == ET9STATUS_WORD_EXISTS) {

                /* just go ahead and send; _ET9AWRDBAddWord will lowercase based on flag */

                wStatus = _ET9AWRDBAddWord(pLingInfo, pHolder, ET9RD_WORD_INIT_FREQ, (ET9U8)(wLdbNum & ET9PLIDMASK), 0, 0);
            }
            else {

                /* something weird happened.. found before and not didn't */

                ET9Assert(0);
            }
        }
    }

    else {

        ET9UINT bProcessed = 0;

        /* else if RUDB sourced, check to see if RDB word... if so bump freq */

        if (GETBASESRC(pHolder->bWordSrc) == ET9WORDSRC_RUDB) {

            if (!bRedundantCheck) {
                bProcessed = _ET9AWFindASDBObject(pLingInfo, pHolder->Base.sWord, pHolder->Base.wWordLen, 0, 1);
            }

            if (!bProcessed) {
                bProcessed = _ET9AWFindLdbASObject(pLingInfo, wLdbNum, pHolder->Base.sWord, pHolder->Base.wWordLen, 0, 1);
            }

            if (bProcessed) {

                if (!_ET9AWFindRUDBObject(pLingInfo, pHolder, 0, 1)) {

                    pHolder->bWordSrc = ET9WORDSRC_LDB;
                    bExact = 1;

                    wStatus = _ET9AWRDBAddWord(pLingInfo, pHolder, ET9RD_WORD_INIT_FREQ, (ET9U8)(wLdbNum & ET9PLIDMASK), bExact, 0);
                }
            }
        }

        /* if not shortcut... */

        if (!bProcessed) {

            /* bump the freq if already exists in UDB                                  */
            /* if it DOESN'T exist in the UDB, strip it and try checking the RDB again */

            if (!bRedundantCheck) {
                bProcessed = _ET9AWFindRUDBObject(pLingInfo, pHolder, ET9UDBTYPE, 1);
            }

            if (!bProcessed) {

                wStatus = _ET9AWSelLstStripActualTaps(pHolder);

                if (wStatus != ET9STATUS_NO_OPERATION && pHolder->Base.wWordLen > 1) {

                    if (_ET9AWLdbFind(pLingInfo, wLdbNum, pHolder, &bExact, &bLowercase, 0) == ET9STATUS_WORD_EXISTS) {

                        pHolder->bWordSrc = ET9WORDSRC_LDB;

                        wStatus = _ET9AWRDBAddWord(pLingInfo, pHolder, ET9RD_WORD_INIT_FREQ, (ET9U8)(wLdbNum & ET9PLIDMASK), bExact, bLowercase);
                    }
                }

                /* otherwise make sure status updated */

                else {

                    wStatus = ET9STATUS_NONE;
                }
            }
        }
    }

    return wStatus;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Add selection word(s) to Supplemental DBs.
 *
 * @param pLingInfo        Pointer to alpha information structure.
 * @param pSelWord         Word to be added.
 * @param nProcessType     USE or SPACE processing.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL _ET9AWSuppDBAddSelection(ET9AWLingInfo         *pLingInfo,
                                              ET9AWPrivWordInfo     *pSelWord,
                                              ET9UINT                nProcessType)
{
    ET9AWLingCmnInfo * const pLingCmnInfo = pLingInfo->pLingCmnInfo;
    ET9AWPrivWordInfo holder;
    ET9U16    index = 0;
    ET9U16    i;
    ET9STATUS wStatus = ET9STATUS_NONE;
    ET9U16    aSize;
    ET9U16    wFreq;
    ET9U8     bExact = 0;
    ET9U8     bLowercase = 0;

    ET9Assert(pLingInfo!= NULL);
    ET9Assert(pSelWord);
    ET9Assert(pLingCmnInfo!= NULL);
    aSize = pSelWord->Base.wWordLen;

    while (index < aSize) {

        _InitPrivWordInfo(&holder);

        holder.bWordSrc = pSelWord->bWordSrc;

        i = 0;
        while (index < aSize && pSelWord->Base.sWord[index] &&
            (holder.Base.sWord[i] = pSelWord->Base.sWord[index]) != ET9TXTFILLSYM) {
            ++i;
            ++index;
            ++holder.Base.wWordLen;
        }

        ++index;

        wStatus = ET9STATUS_NONE;

        if (holder.Base.wWordLen) {

            if (nProcessType == USE_PROCESSING && (holder.Base.wWordLen > 1)) {

                /* if word exists as shortcut or LDB word, add to the RDB */

                if (pSelWord->Base.bLangIndex == ET9AWFIRST_LANGUAGE) {
                    wStatus = __ET9AWUseProcessWord(pLingInfo, &holder, 1, 0);
                }
                else if (pSelWord->Base.bLangIndex == ET9AWSECOND_LANGUAGE) {
                    if (ET9AWSys_GetBilingualSupported(pLingInfo)) {
                        wStatus = __ET9AWUseProcessWord(pLingInfo, &holder, 0, 0);
                    }
                    else {
                        wStatus = ET9STATUS_ERROR;
                    }
                }
                else if (pSelWord->Base.bLangIndex == ET9AWBOTH_LANGUAGES) {
                    wStatus = __ET9AWUseProcessWord(pLingInfo, &holder, 1, 0);
                    if (ET9AWSys_GetBilingualSupported(pLingInfo)) {
                        wStatus = __ET9AWUseProcessWord(pLingInfo, &holder, 0, 1);
                    }
                }
                else {
                    ET9Assert(0);
                    wStatus = ET9STATUS_ERROR;
                }
            }
            else if (nProcessType == SPACE_PROCESSING) {

                ET9UINT nEnterPossible = 1;

                /* CDB processing */

                _ET9AWCDBAddWord(pLingInfo, &holder);

                /* only do RUDB processing if word > 1 symbol */

                if (holder.Base.wWordLen > 1) {

                    /* UDB Processing */
                    /* if word is in ldb, don't need to do any more udb processing */

                    if (_ET9AWLdbFind(pLingInfo, pLingCmnInfo->wFirstLdbNum, &holder, &bExact, &bLowercase, 0) == ET9STATUS_WORD_EXISTS) {
                        nEnterPossible = 0;
                    }
                    else if (ET9AWSys_GetBilingualSupported(pLingInfo) &&
                             _ET9AWLdbFind(pLingInfo, pLingCmnInfo->wSecondLdbNum, &holder, &bExact, &bLowercase, 0) == ET9STATUS_WORD_EXISTS) { /* bilingual revisit */
                        nEnterPossible = 0;
                    }
                    /* if word is already in RUBD,  don't need to do any more udb processing */
                    else if (_ET9AWFindRUDBObject(pLingInfo, &holder, 0, 0)) {
                        nEnterPossible = 0;
                    }
                    /* if still haven't found word, strip taps and check */
                    else {
                        wStatus = _ET9AWSelLstStripActualTaps(&holder);
                        if (wStatus != ET9STATUS_NO_OPERATION) {
                            if (!holder.Base.wWordLen) {
                                nEnterPossible = 0;
                            }
                            else if (_ET9AWLdbFind(pLingInfo, pLingCmnInfo->wFirstLdbNum, &holder, &bExact, &bLowercase, 0) == ET9STATUS_WORD_EXISTS) {
                                nEnterPossible = 0;
                            }
                            else if (ET9AWSys_GetBilingualSupported(pLingInfo) &&
                                     _ET9AWLdbFind(pLingInfo, pLingCmnInfo->wSecondLdbNum, &holder, &bExact, &bLowercase, 0) == ET9STATUS_WORD_EXISTS) { /* bilingual revisit */
                                nEnterPossible = 0;
                            }
                            else if (_ET9AWFindRUDBObject(pLingInfo, &holder, 0, 0)) {
                                nEnterPossible = 0;
                            }
                        }

                        /* make sure status updated */

                        wStatus = ET9STATUS_NONE;
                    }

                    /* if still haven't found word, add it to the udb */

                    if (nEnterPossible) { /* Enter word if still possible */

                        /* use unique internal add interface... */
                        if (!ET9QUDBSUPPORTENABLED(pLingCmnInfo) || _ET9IsInhibitDelayedReorderAfterTrace(pLingCmnInfo->Base.pWordSymbInfo)) {
                            wFreq = (ET9U16)ET9ALP_WORD_INIT_FREQ;
                        }
                        /* otherwise, add as QUDB entry */
                        else {
                            wFreq = (ET9U16)(ET9MAX_FREQ_COUNT + ET9_QUDB_EXIST_COUNT);
                        }

                        wStatus = _ET9AWGeneralUDBAddWord(pLingInfo, holder.Base.sWord, holder.Base.wWordLen, wFreq);

                        /* don't call _ET9AWRUDBUpdateCounter on status */

                        if (wStatus) {
                            nEnterPossible = 0;
                        }

                        /* some status codes shoudln't be passed through */

                        if (wStatus == ET9STATUS_BAD_PARAM) {
                            wStatus = ET9STATUS_NONE;
                        }
                    }
                }
                else {
                    nEnterPossible = 0;
                }

                if (!nEnterPossible) {

                    /* this was moved here from ET9AWNoteWordDone so that the update   */
                    /* count is not bumped twice if the word is added to the UDB       */
                    /* (_ET9AWRUDBUpdateCounter is called from within ET9AWUDBAddWord) */

                    _ET9AWRUDBUpdateCounter(pLingInfo, 1);
                }
            }
        }
    }

    return wStatus;
}

/*---------------------------------------------------------------------------*/
/**
 * Make this the current context.
 *
 * @param pLingInfo        Pointer to alpha information structure.
 * @param psBuf            Buffer to be used.
 * @param nBufLen          Length of buffer.
 *
 * @return ET9STATUS_NONE on success, otherwise return T9 error code.
 */

ET9STATUS ET9FARCALL ET9AWFillContextBuffer(ET9AWLingInfo   * const pLingInfo,
                                            ET9SYMB         * const psBuf,
                                            const ET9UINT           nBufLen)
{
    ET9STATUS   eStatus;

    eStatus = _ET9AWSys_BasicValidityCheck(pLingInfo);

    if (eStatus) {
        return eStatus;
    }

    if (!psBuf && nBufLen) {
        return ET9STATUS_INVALID_MEMORY;
    }

    /* break context on no content */

    if (!nBufLen) {

        ET9AWLingCmnInfo * const pLingCmnInfo = pLingInfo->pLingCmnInfo;

        _ET9AWCDBBreakContext(pLingInfo);

        pLingCmnInfo->Private.bContextWordSize = 0;
        pLingCmnInfo->Private.bPreviousContextWordSize = 0;
        pLingCmnInfo->Private.wContextWordClass = 0;

        return ET9STATUS_NONE;
    }

    /* handle context */

    {
        ET9AWLingCmnInfo * const pLingCmnInfo = pLingInfo->pLingCmnInfo;

        ET9UINT nIndex;

        /* optimize on long contexts */

        {
            const ET9UINT nMaxLen = 200;

            if (nBufLen < nMaxLen) {
                nIndex = 0;
            }
            else {
                for (nIndex = nBufLen - nMaxLen; nIndex; --nIndex) {
                    if (_ET9_IsWhiteSpace(psBuf[nIndex])) {
                        break;
                    }
                }
                if (_ET9_IsWhiteSpace(psBuf[nIndex])) {
                    ++nIndex;
                }
            }
        }

        /* scan context */

        pLingCmnInfo->Private.bContextWordSize = 0;
        pLingCmnInfo->Private.bPreviousContextWordSize = 0;
        pLingCmnInfo->Private.wContextWordClass = 0;

        for (; nIndex < nBufLen; ++nIndex) {

            const ET9SYMB sSymb = psBuf[nIndex];

            if (_ET9_IsWhiteSpace(sSymb)) {

                /* if context exists... */

                if (pLingCmnInfo->Private.bContextWordSize) {

                    /* if more buffer exists  */

                    if (nIndex + 1 < nBufLen) {

                        /* save current context word as previous */

                        _ET9SymCopy(pLingCmnInfo->Private.sPreviousContextWord, pLingCmnInfo->Private.sContextWord, pLingCmnInfo->Private.bContextWordSize);
                        pLingCmnInfo->Private.bPreviousContextWordSize = pLingCmnInfo->Private.bContextWordSize;

                        /* and set up for next word processing */

                        pLingCmnInfo->Private.bContextWordSize = 0;
                    }
                }

                /* else two or more spaces in a row, so break context */

                else {

                    pLingCmnInfo->Private.bContextWordSize = 0;
                    pLingCmnInfo->Private.bPreviousContextWordSize = 0;
                    pLingCmnInfo->Private.wContextWordClass = 0;
                }
            }
            else if (pLingCmnInfo->Private.bContextWordSize == ET9MAXWORDSIZE) {

                /* if word is longer than max size, break context and */
                /* start over at the next word boundary               */

                pLingCmnInfo->Private.bContextWordSize = 0;
                pLingCmnInfo->Private.bPreviousContextWordSize = 0;
                pLingCmnInfo->Private.wContextWordClass = 0;

                /* loop until oversized word is done */

                while (nIndex < nBufLen && !_ET9_IsWhiteSpace(psBuf[nIndex])) {
                    ++nIndex;
                }
                --nIndex;
            }
            else {

                /* save the character and keep going */

                pLingCmnInfo->Private.sContextWord[pLingCmnInfo->Private.bContextWordSize++] = sSymb;
            }
        }
    }

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/* Macro to process return from db export */

#define __TUDBCheckDBExport(dwUsedSize, wTotalRecs, pdwExportSize, pwRecordsExported, wStatus, dwTUDBSize)      \
    *pdwExportSize = *pdwExportSize + dwUsedSize;                                                               \
    *pwRecordsExported = *pwRecordsExported + wTotalRecs;                                                       \
    if (wStatus) {                                                                                              \
        return wStatus;                                                                                         \
    }                                                                                                           \

/*---------------------------------------------------------------------------*/
/** \internal
 * Handle ALL reads to TUDB here.
 *
 * @param pbTo             Location of data to copy.
 * @param wSize            How much data.
 * @param pTUdb            Pointer to TUDB.
 * @param dwOffset         Offset to TUDB for writing.
 * @param ET9ReadTUDB      Function pointer to OEM read.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL _ET9TUdbReadData(ET9U8 ET9FARDATA  *pbTo,
                                      ET9U16             wSize,
                                      ET9U8 ET9FARDATA  *pTUdb,
                                      ET9U32             dwOffset,
                                      ET9ReadTUDB_f      ET9ReadTUDB)
{

    if (ET9ReadTUDB) {
        return ET9ReadTUDB(pbTo, wSize, pTUdb, dwOffset);
    }
    else {

        ET9U8 ET9FARDATA *pbFrom = (ET9U8 ET9FARDATA *)pTUdb + dwOffset;

        for (; wSize--; ) {
            *pbTo++ = *pbFrom++;
        }
    }

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Read a word from TUDB.
 *
 * @param pwWord            Pointer to word data.
 * @param pTUdb             Pointer to TUDB.
 * @param dwTUdbOffset      Offset to TUDB for writing.
 * @param ET9ReadTUDB       Function pointer to OEM read.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL _ET9TUdbReadWord(ET9U16            *pwWord,
                                      ET9U8 ET9FARDATA  *pTUdb,
                                      ET9U32             dwTUdbOffset,
                                      ET9ReadTUDB_f      ET9ReadTUDB)
{
    ET9STATUS  wStatus = ET9STATUS_NONE;
    ET9U8        hi_byte, lo_byte;

    /* read upper byte */
    wStatus = _ET9TUdbReadData((ET9U8 ET9FARDATA *)&hi_byte, 1, pTUdb, dwTUdbOffset, ET9ReadTUDB);
    _ET9CHECKGENSTATUS();

    /* read lower byte */
    wStatus = _ET9TUdbReadData((ET9U8 ET9FARDATA *)&lo_byte, 1, pTUdb, dwTUdbOffset+1, ET9ReadTUDB);
    _ET9CHECKGENSTATUS();

    *pwWord = (ET9U16)(((ET9U16)hi_byte << 8) + lo_byte);

    return wStatus;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Handle ALL writes to TUDB here.
 *
 * @param pbFrom            Location of data to copy.
 * @param wSize             How much data.
 * @param pTUdb             Pointer to TUDB.
 * @param dwOffset          Offset to TUDB for writing.
 * @param ET9WriteTUDB      Function pointer to OEM write.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL _ET9TUdbWriteData(ET9U8 ET9FARDATA  *pbFrom,
                                       ET9U16             wSize,
                                       ET9U8 ET9FARDATA  *pTUdb,
                                       ET9U32             dwOffset,
                                       ET9WriteTUDB_f     ET9WriteTUDB)
{
    if (ET9WriteTUDB) {
        return ET9WriteTUDB(pbFrom, wSize, pTUdb, dwOffset);
    }
    else {
        ET9U8 ET9FARDATA *pbTo = (ET9U8 ET9FARDATA *)(pTUdb + dwOffset);

        for (; wSize--; ) {
            *pbTo++ = *pbFrom++;
        }
    }

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Write a word to TUDB.
 *
 * @param wWord             A word data.
 * @param pTUdb             Pointer to TUDB.
 * @param dwTUdbOffset      Offset to TUDB for writing.
 * @param ET9WriteTUDB      Function pointer to OEM write.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL _ET9TUdbWriteWord(ET9U16             wWord,
                                       ET9U8 ET9FARDATA  *pTUdb,
                                       ET9U32             dwTUdbOffset,
                                       ET9WriteTUDB_f     ET9WriteTUDB)
{
    ET9U8        bByte[2];

    bByte[0] = ET9HIBYTE(wWord);
    bByte[1] = ET9LOBYTE(wWord);

    return _ET9TUdbWriteData(bByte, 2, pTUdb, dwTUdbOffset, ET9WriteTUDB);
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Get size of TUdb for export.
 *
 * @param pLingInfo         Pointer to alphabetic information structure.
 * @param pdwSize           Pointer to size of TUDB.
 * @param pdwTotalWords     Pointer to total of words for exporting.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL ET9AWTUDBGetSize(ET9AWLingInfo * const pLingInfo,
                                      ET9U32        * const pdwSize,
                                      ET9U32        * const pdwTotalWords)
{
    ET9STATUS  wStatus = ET9STATUS_NONE;
    ET9U32     dwTotalWords;

    if ((wStatus = _ET9AWSys_BasicValidityCheck(pLingInfo)) == ET9STATUS_NONE) {
        if (pdwSize == NULL || pdwTotalWords == NULL) {
            wStatus = ET9STATUS_INVALID_MEMORY;
        }
        else {
            *pdwSize = _ET9AWRUDBGetSize(pLingInfo, &dwTotalWords);
            *pdwTotalWords = dwTotalWords;

            *pdwSize = *pdwSize + _ET9AWASDBGetSize(pLingInfo, &dwTotalWords);
            *pdwTotalWords += dwTotalWords;

            if (! *pdwTotalWords) {
                *pdwSize = 0;
            }
            else if (*pdwSize < ET9TUDB_MEMMINSIZE) {
                *pdwSize = ET9TUDB_MEMMINSIZE;
            }
        }
    }
    return wStatus;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Export DBs to TUDB.
 *
 * @param pLingInfo             Pointer to alphabetic information structure.
 * @param pTUdb                 Pointer to TUDB.
 * @param dwTUDBSize            Size of TUDB.
 * @param pdwExportSize         Size of TUDB data written.
 * @param ET9WriteTUDB          Function pointer to OEM write.
 * @param pNextRecord           Pointer to structure to cache last export record info.
 * @param pwRecordsExported     Pointer to total records exported.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL ET9AWTUDBExport(ET9AWLingInfo       * const pLingInfo,
                                     ET9U8 ET9FARDATA    * const pTUdb,
                                     const ET9U32                dwTUDBSize,
                                     ET9U32              * const pdwExportSize,
                                     ET9WriteTUDB_f              ET9WriteTUDB,
                                     ET9AWTUDBNextRecord * const pNextRecord,
                                     ET9U16              * const pwRecordsExported)
{
    ET9STATUS  wStatus = ET9STATUS_NONE;
    ET9U16     wTotalRecs = 0;
    ET9U32     dwUsedSize = 0;

    if ((wStatus = _ET9AWSys_BasicValidityCheck(pLingInfo)) != ET9STATUS_NONE) {
        return wStatus;
    }
    if (pTUdb == NULL || pdwExportSize == NULL || pNextRecord == NULL || pwRecordsExported == NULL) {
        return ET9STATUS_INVALID_MEMORY;
    }

    *pdwExportSize = 0;
    *pwRecordsExported = 0;

    if (dwTUDBSize < ET9TUDB_MEMMINSIZE) {
        return ET9STATUS_BUFFER_TOO_SMALL;
    }

    if (pNextRecord->bDBType > ET9_TUDBTYPE_UASDB) {
        return ET9STATUS_ERROR;
    }

    if (pNextRecord->bDBType <= ET9_TUDBTYPE_RUDB) {
        wStatus = _ET9AWRUDBExport(pLingInfo, pTUdb, dwTUDBSize, &dwUsedSize, ET9WriteTUDB, pNextRecord, &wTotalRecs);

        if (wStatus == ET9STATUS_BAD_PARAM) {
            return wStatus;
        }
        __TUDBCheckDBExport(dwUsedSize, wTotalRecs, pdwExportSize, pwRecordsExported, wStatus, dwTUDBSize);

    }

    if (pNextRecord->bDBType <= ET9_TUDBTYPE_LASDB) {

        wStatus = _ET9AWLASDBExport(pLingInfo, (ET9U8 ET9FARDATA*)(pTUdb + *pdwExportSize), (dwTUDBSize - *pdwExportSize), &dwUsedSize, ET9WriteTUDB, pNextRecord, &wTotalRecs);

        if (wStatus == ET9STATUS_BAD_PARAM) {
            return wStatus;
        }

        __TUDBCheckDBExport(dwUsedSize, wTotalRecs, pdwExportSize, pwRecordsExported, wStatus, dwTUDBSize);
    }

    wStatus = _ET9AWUASDBExport(pLingInfo, (ET9U8 ET9FARDATA *)(pTUdb + *pdwExportSize), (dwTUDBSize - *pdwExportSize), &dwUsedSize, ET9WriteTUDB,  pNextRecord, &wTotalRecs);

    if (wStatus == ET9STATUS_BAD_PARAM) {
        return wStatus;
    }

    __TUDBCheckDBExport(dwUsedSize, wTotalRecs, pdwExportSize, pwRecordsExported, wStatus, dwTUDBSize);

    return wStatus;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Import TUDB to DBs.
 *
 * @param pLingInfo             Pointer to alphabetic information structure.
 * @param pTUdb                 Pointer to TUDB.
 * @param dwTUdbSize            Size of TUDB.
 * @param ET9ReadTUDB           Function pointer to OEM read.
 * @param pwTotalImported       Pointer to total records imported.
 * @param pwTotalRejected       Pointer to total records rejected.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL ET9AWTUDBImport(ET9AWLingInfo    * const pLingInfo,
                                     ET9U8 ET9FARDATA * const pTUdb,
                                     const ET9U32             dwTUdbSize,
                                     ET9ReadTUDB_f            ET9ReadTUDB,
                                     ET9U16           * const pwTotalImported,
                                     ET9U16           * const pwTotalRejected)
{
    ET9STATUS  wStatus = ET9STATUS_NONE;
    ET9U8      bAttrType;
    ET9U16     wRecSize;
    ET9U8      bImported;
    ET9U16     wOffset = 0;

    if ((wStatus = _ET9AWSys_BasicValidityCheck(pLingInfo)) != ET9STATUS_NONE) {
        return wStatus;
    }
    if (pTUdb == NULL || pwTotalImported == NULL || pwTotalRejected == NULL) {
        return ET9STATUS_INVALID_MEMORY;
    }

    *pwTotalImported = 0;
    *pwTotalRejected = 0;

    while ((ET9U32)(wOffset + 3) <= dwTUdbSize) {

        /* get attribute type */
        wStatus = _ET9TUdbReadData(&bAttrType, 1, pTUdb, wOffset++, ET9ReadTUDB);
        _ET9CHECKGENSTATUS();

        /* get record size */
        wStatus = _ET9TUdbReadWord(&wRecSize, pTUdb, wOffset, ET9ReadTUDB);
        _ET9CHECKGENSTATUS();
        wOffset += 2;

        if ((ET9U32)(wOffset + wRecSize) > dwTUdbSize) {
            (*pwTotalRejected)++;
            break;
        }

        bImported = 0;

        switch (bAttrType)
        {
            case ET9TUDB_T9CUSTOMWORD_REC:
                wStatus = _T9UDBImport(pLingInfo, pTUdb, ET9ReadTUDB, wOffset, wRecSize, &bImported);
                if (wStatus == ET9STATUS_ERROR) {
                    return wStatus;
                }
                break;

            case ET9TUDB_CUSTOMWORD_REC:
                wStatus = _ET9AWUDBImport(pLingInfo, pTUdb, ET9ReadTUDB, wOffset, wRecSize, &bImported);
                if (wStatus == ET9STATUS_ERROR) {
                    return wStatus;
                }
                break;

            case ET9TUDB_REORDERWORD_REC:
                wStatus = _ET9AWRDBImport(pLingInfo, pTUdb, ET9ReadTUDB, wOffset, wRecSize, &bImported);
                if (wStatus == ET9STATUS_ERROR) {
                    return wStatus;
                }
                break;

            case ET9TUDB_LDBAUTOSUB_REC:
                wStatus = _ET9AWLDBBMImport(pLingInfo, pTUdb, ET9ReadTUDB, wOffset, wRecSize, &bImported);
                if (wStatus == ET9STATUS_ERROR) {
                    return wStatus;
                }
                break;

            case ET9TUDB_USERAUTOSUBWORD_REC:
                wStatus = _ET9AWASDBImport(pLingInfo, pTUdb, ET9ReadTUDB, wOffset, wRecSize, &bImported);
                if (wStatus == ET9STATUS_ERROR) {
                    return wStatus;
                }
                break;

            case ET9TUDB_T9REORDERWORD_REC: /* skip importing T9 RDB */
            default:
                break;
        }
        wOffset = (ET9U16)(wOffset + wRecSize);
        if (bImported) {
            (*pwTotalImported)++;
        }
        else {
            (*pwTotalRejected)++;
        }
    }

    return ET9STATUS_NONE;
}


#endif /* ET9_ALPHABETIC_MODULE */
/*! @} */
/* ----------------------------------< eof >--------------------------------- */
