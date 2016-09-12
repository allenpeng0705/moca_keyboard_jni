/*******************************************************************************
;*******************************************************************************
;**                                                                           **
;**                    COPYRIGHT 1998-2011 NUANCE COMMUNICATIONS              **
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
;**     FileName: et9alsasdb.c                                                **
;**                                                                           **
;**  Description: LDB supported AutoSubstitution data base access routines    **
;**               source file.                                                **
;**                                                                           **
;*******************************************************************************
;******************************************************************************/

/*! \internal \addtogroup et9alsasdb LDB auto substitution for generic
* LDB auto substitution for generic XT9.
* @{
*/

#include "et9api.h"
#ifdef ET9_ALPHABETIC_MODULE
#include "et9asys.h"
#include "et9adb.h"
#include "et9alsasdb.h"
#include "et9amisc.h"
#include "et9sym.h"
#include "et9aasdb.h"
#include "et9aspc.h"


#ifdef ET9_DEBUGLOG2
#ifdef _WIN32
#pragma message ("*** ET9_DEBUGLOG2 ACTIVATED ***")
#endif
#include <stdio.h>
#include <string.h>
#define WLOG2(q) { if (pLogFile2 == NULL) { pLogFile2 = fopen("zzzET9ALSASDB.txt", "w+"); } {q} fflush(pLogFile2);  }
static FILE *pLogFile2 = NULL;
#else
#define WLOG2(q)
#endif


#ifdef ET9_DEBUGLOG2
/*---------------------------------------------------------------------------*/
/** \internal
 * Logs a word, with attached properties.
 *
 * @param pF                        File to log to.
 * @param pcComment                 Comment to be logged with the string.
 * @param pWord                     Pointer to the word.
 *
 * @return None
 */

static void ET9LOCALCALL WLOG2Word(FILE *pF, char *pcComment, ET9AWPrivWordInfo *pWord)
{
    ET9U16 wIndex;

    ET9Assert(pF != NULL);
    ET9Assert(pcComment != NULL);

    fprintf(pF, "%s: ", pcComment);

    if (pWord == NULL) {
        fprintf(pF, "NULL\n");
        return;
    }

    for (wIndex = 0; wIndex < pWord->Base.wWordLen; ++wIndex) {

        ET9SYMB sSymb = pWord->Base.sWord[wIndex];

        if (sSymb >= 0x20 && sSymb <= 0x7F) {
            fprintf(pF, "%c", (char)sSymb);
        }
        else {
            fprintf(pF, "<%x>", (int)sSymb);
        }
    }

    if (pWord->Base.wSubstitutionLen) {

        fprintf(pF, " -> ");

        for (wIndex = 0; wIndex < pWord->Base.wSubstitutionLen; ++wIndex) {

            ET9SYMB sSymb = pWord->Base.sSubstitution[wIndex];

            if (sSymb >= 0x20 && sSymb <= 0xFF) {
                fprintf(pF, "%c", (unsigned char)sSymb);
            }
            else {
                fprintf(pF, "<%x>", (int)sSymb);
            }
        }
    }

    fprintf(pF, "\n");

    fflush(pF);
}
#else /* ET9_DEBUGLOG2 */
#define WLOG2Word(pF,pcComment,pWord)
#endif /* ET9_DEBUGLOG2 */


#define SIZE_OF_ASDB_SYMBOL 2


/*---------------------------------------------------------------------------*/
/** \internal
 * Get the 2 byte/char string from encoded bytes.
 *
 * @param pLingInfo         Pointer to alphabetic information structure.
 * @param psString          Pointer to resultant string.
 * @param dwStartAddress    Starting byte address.
 * @param wLength           Length of string.
 *
 * @return ET9STATUS_NONE on success, otherwise return error code.
 */

static ET9STATUS ET9LOCALCALL __ET9AWLdbASGetEncodedString(ET9AWLingInfo       *pLingInfo,
                                                           ET9SYMB             *psString,
                                                           ET9U32              dwStartAddress,
                                                           ET9U16              wLength)
{
    ET9U16 ct;

    ET9Assert(pLingInfo && psString && dwStartAddress);

    for (ct = 0; ct < wLength; ++ct, dwStartAddress += SIZE_OF_ASDB_SYMBOL) {
       *psString++ = _ET9ReadLDBWord2(pLingInfo, dwStartAddress);
    }

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Retrieve record based on the index.
 *
 * @param pLingInfo             Pointer to alphabetic information structure.
 * @param wLdbNum                   LDB number.
 * @param wRecordNum            Record index (0 based).
 * @param psShortcut            Pointer to shortcut entry.
 * @param pwShortcutLen         Pointer to length of shortcut entry.
 * @param psSubstitution        Pointer to substitution entry.
 * @param pwSubstitutionLen     Pointer to length of substitution entry.
 *
 * @return ET9STATUS_NONE on success, otherwise return error code.
 */

ET9STATUS ET9FARCALL _ET9AWLdbASGetEntry(ET9AWLingInfo  *pLingInfo,
                                         ET9U16         wLdbNum,
                                         ET9U16         wRecordNum,
                                         ET9SYMB        *psShortcut,
                                         ET9U16         *pwShortcutLen,
                                         ET9SYMB        *psSubstitution,
                                         ET9U16         *pwSubstitutionLen)
{
    ET9U32 dwLSASDBTracker;
    ET9INT Count;
    ET9U8  bLenShortcut, bLenSubstitution;
    ET9U32 dwLSASDBEndAddress;
    ET9AWLingCmnInfo * const pLingCmnInfo = pLingInfo->pLingCmnInfo;

    ET9Assert(pLingInfo);
    ET9Assert(!ET9USERDEFINEDAUTOSUBENABLED(pLingCmnInfo) || pLingCmnInfo->pASDBInfo != NULL);
    ET9Assert(psShortcut && pwShortcutLen && psSubstitution && pwSubstitutionLen);

    /* check for LDB based AS support */
    if (((wLdbNum & ET9PLIDMASK) == ET9PLIDNone) || !pLingCmnInfo->Private.sLDBAutoSub.bSupported || ((wLdbNum & ET9PLIDMASK) != pLingInfo->pLingCmnInfo->Private.sLDBAutoSub.bLSASDBLangID)) {
        return ET9STATUS_NO_MATCHING_WORDS;
    }
    ET9Assert(wRecordNum < pLingCmnInfo->Private.sLDBAutoSub.wNumEntries);

    /* Get LDB supported ASDB info */
    dwLSASDBTracker = pLingCmnInfo->Private.sLDBAutoSub.dwLSASDBStartAddress;
    dwLSASDBEndAddress = pLingCmnInfo->Private.sLDBAutoSub.dwLSASDBEndAddress;

    /* Jump over previous entries */
    for (Count = 0; Count < (int)wRecordNum && dwLSASDBTracker <= dwLSASDBEndAddress; ++Count) {
        bLenShortcut = _ET9ReadLDBByte(pLingInfo, dwLSASDBTracker++);
        dwLSASDBTracker += (SIZE_OF_ASDB_SYMBOL * bLenShortcut);
        bLenSubstitution = _ET9ReadLDBByte(pLingInfo, dwLSASDBTracker++);
        dwLSASDBTracker += (SIZE_OF_ASDB_SYMBOL * bLenSubstitution);
    }

    /* target entry */
    if (Count == (int)wRecordNum) {
        bLenShortcut = _ET9ReadLDBByte(pLingInfo, dwLSASDBTracker++);
        *pwShortcutLen = (ET9U16)bLenShortcut;
        __ET9AWLdbASGetEncodedString(pLingInfo, psShortcut, dwLSASDBTracker, *pwShortcutLen);

        dwLSASDBTracker += (SIZE_OF_ASDB_SYMBOL * bLenShortcut);

        bLenSubstitution = _ET9ReadLDBByte(pLingInfo, dwLSASDBTracker++);
        *pwSubstitutionLen = (ET9U16)bLenSubstitution;
        __ET9AWLdbASGetEncodedString(pLingInfo, psSubstitution, dwLSASDBTracker, *pwSubstitutionLen);
    }
    else {
        /* problem with LDB */
        ET9Assert(0);
    }

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Retrieve index (and substitution) based on shortcut.
 *
 * @param pLingInfo             Pointer to alphabetic information structure.
 * @param wLdbNum               LDB number.
 * @param psShortcut            Pointer to shortcut entry.
 * @param wShortcutLen          Length of shortcut entry.
 * @param psSubstitution        Pointer to substitution entry.
 * @param pwSubstitutionLen     Pointer to length of substitution entry.
 * @param pwRecordNum           Pointer to record index (0 based).
 *
 * @return ET9STATUS_NONE on success, otherwise return error code.
 */

ET9STATUS ET9FARCALL _ET9AWLdbASFindEntry(ET9AWLingInfo *pLingInfo,
                                          ET9U16         wLdbNum,
                                          ET9SYMB       *psShortcut,
                                          ET9U16         wShortcutLen,
                                          ET9SYMB       *psSubstitution,
                                          ET9U16        *pwSubstitutionLen,
                                          ET9U16        *pwRecordNum)
{
    ET9U32 dwLSASDBTracker;
    ET9U16 wNumEntries, Count, ct;
    ET9U8  bLenShortcut, bLenSubstitution;
    ET9U32  dwLSASDBEndAddress;
    ET9SYMB sSymb, sSymb2;
    ET9SYMB sCode;

    ET9AWLingCmnInfo * const pLingCmnInfo = pLingInfo->pLingCmnInfo;

    ET9Assert(pLingInfo);
    ET9Assert(!ET9USERDEFINEDAUTOSUBENABLED(pLingCmnInfo) || pLingCmnInfo->pASDBInfo != NULL);
    ET9Assert(psShortcut);
    ET9Assert(pwRecordNum);

    /* check for LDB based AS support */
    if (((wLdbNum & ET9PLIDMASK) == ET9PLIDNone) || !pLingCmnInfo->Private.sLDBAutoSub.bSupported || ((wLdbNum & ET9PLIDMASK) != pLingInfo->pLingCmnInfo->Private.sLDBAutoSub.bLSASDBLangID)) {
        return ET9STATUS_NO_MATCHING_WORDS;
    }

    /* Get LDB supported ASDB info */
    dwLSASDBTracker = pLingCmnInfo->Private.sLDBAutoSub.dwLSASDBStartAddress;
    wNumEntries = pLingCmnInfo->Private.sLDBAutoSub.wNumEntries;
    dwLSASDBEndAddress = pLingCmnInfo->Private.sLDBAutoSub.dwLSASDBEndAddress;

    /* iterate through all LDB supported ASDB entries */
    for(Count = 0; Count < wNumEntries && dwLSASDBTracker <= dwLSASDBEndAddress; ++Count) {
        bLenShortcut = _ET9ReadLDBByte(pLingInfo, dwLSASDBTracker++);
        if (wShortcutLen == (ET9U16)bLenShortcut) {
            for (ct = 0; ct < bLenShortcut; ++ct) {
                sCode = _ET9ReadLDBWord2(pLingInfo, dwLSASDBTracker);
                dwLSASDBTracker += SIZE_OF_ASDB_SYMBOL;
                /* if it doesn't match */
                sSymb2 = _ET9SymToLower(psShortcut[ct], wLdbNum);
                sSymb =  _ET9SymToLower(sCode, wLdbNum);
                /* if symbol doesn't match, move on to the next record */
                if (sSymb2 != sSymb) {
                    dwLSASDBTracker += (SIZE_OF_ASDB_SYMBOL * (bLenShortcut - (ct + 1)));
                    break;
                }
            }
            /* if match found */
            if (ct == bLenShortcut) {
                *pwRecordNum = Count;
                if (psSubstitution && pwSubstitutionLen) {
                    bLenSubstitution = _ET9ReadLDBByte(pLingInfo, dwLSASDBTracker++);
                    *pwSubstitutionLen = bLenSubstitution;
                    __ET9AWLdbASGetEncodedString(pLingInfo, psSubstitution, dwLSASDBTracker, *pwSubstitutionLen);
                    dwLSASDBTracker += (SIZE_OF_ASDB_SYMBOL * bLenSubstitution);
                }
                return ET9STATUS_NONE;
            }
        }
        else {
            dwLSASDBTracker += (SIZE_OF_ASDB_SYMBOL * bLenShortcut);
        }
        bLenSubstitution = _ET9ReadLDBByte(pLingInfo, dwLSASDBTracker++);
        dwLSASDBTracker += (SIZE_OF_ASDB_SYMBOL * bLenSubstitution);
    }

    return ET9STATUS_NO_MATCHING_WORDS;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Search for existence of word in LDB-AS entries.
 *
 * @param pLingInfo             Pointer to alphabetic information structure.
 * @param wLdbNum               LDB number.
 * @param pWord                 Pointer to shortcut word info being searched for.
 * @param wLength               Length of shortcut word.
 * @param bCaseSensitive        If set, does direct comparison; if clear, does lowercase comparison.
 * @param bUpdateWithMatch      If set, will overwrite passed pWord with ASDB shortcut version IFF match found.
 *
 * @return None-zero - found, zero - not found.
 */

ET9UINT ET9FARCALL _ET9AWFindLdbASObject(ET9AWLingInfo     *pLingInfo,
                                         ET9U16             wLdbNum,
                                         ET9SYMB           *pWord,
                                         ET9U16             wLength,
                                         ET9U8              bCaseSensitive,
                                         ET9U8              bUpdateWithMatch)
{
    ET9U32   dwLSASDBTracker;
    ET9U32   dwSavedLSASDBTracker;
    ET9U16   wNumEntries, Count, ct;
    ET9U8    bLenShortcut, bLenSubstitution;
    ET9U32   dwLSASDBEndAddress;
    ET9SYMB *pSymb2;
    ET9SYMB  sCode;
    ET9U16   wSavedLDBNum = 0;

    ET9AWLingCmnInfo * const pLingCmnInfo = pLingInfo->pLingCmnInfo;

    ET9Assert(pLingInfo);
    ET9Assert(pWord);

    /* if no LDB-AS or bad word length */

    if (!ET9LDBSUPPORTEDAUTOSUBENABLED(pLingCmnInfo) ||
        (wLength < 1) || (wLength > ET9MAXWORDSIZE)) {
        return 0;
    }

    /* check for valid LDB */

    if ((wLdbNum & ET9PLIDMASK) == ET9PLIDNone || !ET9ASDBENABLED(pLingCmnInfo)) {
        return 0;
    }

    /* assure cache */

    if (wLdbNum != pLingCmnInfo->wLdbNum) {
        wSavedLDBNum = pLingCmnInfo->wLdbNum;
        _ET9AWLdbSetActiveLanguage(pLingInfo, wLdbNum);
    }

    /* check for LDB based AS support */

    if (((wLdbNum & ET9PLIDMASK) != pLingInfo->pLingCmnInfo->Private.sLDBAutoSub.bLSASDBLangID)) {
        if (wSavedLDBNum) {
            _ET9AWLdbSetActiveLanguage(pLingInfo, wSavedLDBNum);
        }
        return 0;
    }

    /* Get LDB supported ASDB info */

    dwLSASDBTracker = pLingCmnInfo->Private.sLDBAutoSub.dwLSASDBStartAddress;
    wNumEntries = pLingCmnInfo->Private.sLDBAutoSub.wNumEntries;
    dwLSASDBEndAddress = pLingCmnInfo->Private.sLDBAutoSub.dwLSASDBEndAddress;

    /* iterate through all LDB supported ASDB entries */

    for(Count = 0; Count < wNumEntries && dwLSASDBTracker <= dwLSASDBEndAddress; ++Count) {
        bLenShortcut = _ET9ReadLDBByte(pLingInfo, dwLSASDBTracker++);
        if (wLength == (ET9U16)bLenShortcut) {
            pSymb2 = pWord;
            dwSavedLSASDBTracker = dwLSASDBTracker;
            for (ct = 0; ct < wLength; ++ct, ++pSymb2) {
                sCode = _ET9ReadLDBWord2(pLingInfo, dwLSASDBTracker);
                dwLSASDBTracker += SIZE_OF_ASDB_SYMBOL;
                /* if OTFM used, convert ASDB symbol */
                if (pLingInfo->Private.pConvertSymb != NULL) {
                    pLingInfo->Private.pConvertSymb(pLingInfo->Private.pConvertSymbInfo, &sCode);
                }
                if (!bCaseSensitive) {
                    ET9SYMB  sSymbOther = _ET9SymToOther(*pSymb2, wLdbNum);
                    /* if symbol doesn't match, move on to the next record */
                    if (*pSymb2 != sCode && sSymbOther != sCode) {
                        dwLSASDBTracker += (SIZE_OF_ASDB_SYMBOL * (wLength - (ct + 1)));
                        break;
                    }
                }
                else {
                    /* if it doesn't match */
                    if (*pSymb2 != sCode)  {
                        dwLSASDBTracker += (SIZE_OF_ASDB_SYMBOL *(wLength - (ct + 1)));
                        break;
                    }
                }
            }
            /* if match found */
            if (ct == wLength) {
                /* if caller wants ASDB version, update buffer */
                if (bUpdateWithMatch) {
                    pSymb2 = pWord;
                    __ET9AWLdbASGetEncodedString(pLingInfo, pSymb2, dwSavedLSASDBTracker, ct);
                }
                if (wSavedLDBNum) {
                    _ET9AWLdbSetActiveLanguage(pLingInfo, wSavedLDBNum);
                }
                return wLength;
            }
        }
        else {
            /* move to the next record */
            dwLSASDBTracker += (SIZE_OF_ASDB_SYMBOL * bLenShortcut);
        }
        bLenSubstitution = _ET9ReadLDBByte(pLingInfo, dwLSASDBTracker++);
        dwLSASDBTracker += (SIZE_OF_ASDB_SYMBOL * bLenSubstitution);
    }
    if (wSavedLDBNum) {
        _ET9AWLdbSetActiveLanguage(pLingInfo, wSavedLDBNum);
    }

    return 0;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Search LsASDB for user-defined selection list candidates.
 *
 * @param pLingInfo                 Pointer to alphabetic information structure.
 * @param wLdbNum                   Language being checked.
 * @param wIndex                    Input index/position (non zero if partial).
 * @param wLength                   Word length (active input length).
 * @param bFreqIndicator            Designation
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL _ET9AWLdbASWordsSearch(ET9AWLingInfo           *pLingInfo,
                                            ET9U16                  wLdbNum,
                                            ET9U16                  wIndex,
                                            ET9U16                  wLength,
                                            ET9_FREQ_DESIGNATION    bFreqIndicator)

{
    ET9AWLingCmnInfo * const pLingCmnInfo = pLingInfo->pLingCmnInfo;

    ET9STATUS           wStatus = ET9STATUS_NONE;
    ET9U32              dwLSASDBTracker;
    ET9U16              Count, wNumEntries;
    ET9U8               bLenShortcut, bLenSubstitution;
    ET9U32              dwLSASDBEndAddress;
    ET9U16              wAdjustedLen;
    ET9U16              wSavedLDBNum = 0;
    ET9U8               bFound = 0;
    ET9U8               bExact = 0;
    ET9U8               bLowercase = 0;

    ET9Assert(pLingInfo);

    /* check for valid LDB */

    if ((wLdbNum & ET9PLIDMASK) == ET9PLIDNone || !ET9ASDBENABLED(pLingCmnInfo)) {
        return ET9STATUS_NO_MATCHING_WORDS;
    }

    /* check for LDB based AS support */

    if (!pLingCmnInfo->Private.sLDBAutoSub.bSupported ||
        (wLdbNum & ET9PLIDMASK) != pLingCmnInfo->Private.sLDBAutoSub.bLSASDBLangID) {

        return ET9STATUS_NO_MATCHING_WORDS;
    }

    if (wLength < 1) {
        return ET9STATUS_NONE;
    }

    WLOG2(fprintf(pLogFile2, "_ET9AWLdbASWordsSearch\n");)

    /* assure cache */

    if (wLdbNum != pLingCmnInfo->wLdbNum) {
        wSavedLDBNum = pLingCmnInfo->wLdbNum;
        _ET9AWLdbSetActiveLanguage(pLingInfo, wLdbNum);
    }

    /* Get LDB supported ASDB info */

    dwLSASDBTracker = pLingCmnInfo->Private.sLDBAutoSub.dwLSASDBStartAddress;
    wNumEntries = pLingCmnInfo->Private.sLDBAutoSub.wNumEntries;
    dwLSASDBEndAddress = pLingCmnInfo->Private.sLDBAutoSub.dwLSASDBEndAddress;

    /* iterate through all LDB supported ASDB entries */

    wAdjustedLen = pLingCmnInfo->Private.wCurrMinSourceLength;
    for (Count = 0; Count < wNumEntries && dwLSASDBTracker <= dwLSASDBEndAddress; ++Count) {

        ET9AWPrivWordInfo sLocalWord;

        _InitPrivWordInfo(&sLocalWord);

        /* check to see if record marked for DELETION */

        if (!_ET9AWLdbASRecordEnabled(pLingInfo, wLdbNum, Count)) {
            bLenShortcut = _ET9ReadLDBByte(pLingInfo, dwLSASDBTracker++);
            dwLSASDBTracker += (SIZE_OF_ASDB_SYMBOL * bLenShortcut);
            bLenSubstitution = _ET9ReadLDBByte(pLingInfo, dwLSASDBTracker++);
            dwLSASDBTracker += (SIZE_OF_ASDB_SYMBOL * bLenSubstitution);
            continue;
        }

        sLocalWord.bWordSrc = ET9WORDSRC_LAS_SHORTCUT;
        sLocalWord.xWordFreq  = (ET9FREQPART)0x3FFF;
        sLocalWord.xTapFreq  = 1;

        if (wLdbNum == pLingCmnInfo->wFirstLdbNum) {
            sLocalWord.Base.bLangIndex = ET9AWFIRST_LANGUAGE;
        }
        else if (wLdbNum == pLingCmnInfo->wSecondLdbNum) {
            sLocalWord.Base.bLangIndex = ET9AWSECOND_LANGUAGE;
        }

        bLenShortcut = _ET9ReadLDBByte(pLingInfo, dwLSASDBTracker++);
        sLocalWord.Base.wWordLen = bLenShortcut;

        __ET9AWLdbASGetEncodedString(pLingInfo, sLocalWord.Base.sWord, dwLSASDBTracker, sLocalWord.Base.wWordLen);
        dwLSASDBTracker += (SIZE_OF_ASDB_SYMBOL * bLenShortcut);

        bLenSubstitution = _ET9ReadLDBByte(pLingInfo, dwLSASDBTracker++);
        sLocalWord.Base.wSubstitutionLen = bLenSubstitution;

        __ET9AWLdbASGetEncodedString(pLingInfo, sLocalWord.Base.sSubstitution, dwLSASDBTracker, sLocalWord.Base.wSubstitutionLen);
        dwLSASDBTracker += (SIZE_OF_ASDB_SYMBOL * bLenSubstitution);

        /* if possible selection, go verify */

        if (sLocalWord.Base.wWordLen >= wAdjustedLen) {

            /* if word being compounded, downshift it */

            if (wIndex && bFreqIndicator != FREQ_BUILDSPACE) {

                ET9SYMB *pSymb = sLocalWord.Base.sWord;
                ET9UINT i = sLocalWord.Base.wWordLen;

                for (; i; --i, ++pSymb) {
                    *pSymb = _ET9SymToLower(*pSymb, wLdbNum);
                }
            }

            WLOG2Word(pLogFile2, "  before matching, pWord", &sLocalWord);

            wStatus = _ET9AWSelLstWordMatch(pLingInfo, &sLocalWord, wIndex, wLength, &bFound);

            if (wStatus) {
                break;
            }

            if (bFound) {

                WLOG2Word(pLogFile2, "            match, pWord", &sLocalWord);

                if (pLingInfo->pLingCmnInfo->Private.eCurrSelectionListMode == ET9ASLMODE_MIXED) {

                    if (_ET9AWLdbFindEntry(pLingInfo,
                                           pLingInfo->pLingCmnInfo->wLdbNum,
                                           sLocalWord.Base.sSubstitution,
                                           sLocalWord.Base.wSubstitutionLen,
                                           &sLocalWord.dwWordIndex,
                                           &bExact,
                                           &bLowercase) == ET9STATUS_WORD_EXISTS) {

                        sLocalWord.dwWordIndex += 1;
                        sLocalWord.xWordFreq = (ET9FREQPART)((ET9FREQPART)0xFFFF / sLocalWord.dwWordIndex);
                    }
                    else {
                        sLocalWord.xWordFreq  = (ET9FREQPART)1;
                    }
                }
                else if (sLocalWord.bEditDistFree) {
                    sLocalWord.xWordFreq = (ET9FREQPART)(sLocalWord.xWordFreq / 4);
                }

                _ET9AWSelLstAdd(pLingInfo, &sLocalWord, wLength, bFreqIndicator);
            }
        }
    }

    if (wSavedLDBNum) {
        _ET9AWLdbSetActiveLanguage(pLingInfo, wSavedLDBNum);
    }

    return wStatus;
}


#endif /* ET9_ALPHABETIC_MODULE */

/*! @} */
/* ----------------------------------< eof >--------------------------------- */
