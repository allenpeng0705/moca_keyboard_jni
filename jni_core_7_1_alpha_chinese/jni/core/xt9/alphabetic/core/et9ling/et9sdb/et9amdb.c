/*******************************************************************************
;*******************************************************************************
;**                                                                           **
;**                    COPYRIGHT 1997-2011 NUANCE COMMUNICATIONS              **
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
;**     FileName: et9amdb.c                                                   **
;**                                                                           **
;**  Description: Manufacture database access routines source file.           **
;**                                                                           **
;*******************************************************************************
;******************************************************************************/

/*! \addtogroup et9amdb Manufacturer database for alphabetic
* Manufacturer database for alphabetic XT9.
* @{
*/

#include "et9api.h"
#ifdef ET9_ALPHABETIC_MODULE
#include "et9asys.h"
#include "et9adb.h"
#include "et9amdb.h"
#include "et9amisc.h"
#include "et9sym.h"
#include "et9aspc.h"


/*---------------------------------------------------------------------------*/
/** \internal
 * Get matching objects from manufacturer database.
 *
 * @param pLingInfo                 Pointer to alphabetic information structure.
 * @param wIndex                    Input index/position (non zero if partial).
 * @param wLength                   Word length (active input length).
 * @param bFreqIndicator            Designation
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL _ET9AWMdbWordsSearch(ET9AWLingInfo         *pLingInfo,
                                          ET9U16                wIndex,
                                          ET9U16                wLength,
                                          ET9_FREQ_DESIGNATION  bFreqIndicator)

{
    ET9AWLingCmnInfo * const pLingCmnInfo = pLingInfo->pLingCmnInfo;

    ET9U32              dwWordListIdx;
    const ET9U16        wMinWordLen = pLingCmnInfo->Private.wCurrMinSourceLength;

    const ET9BOOL bUsingLM = (ET9BOOL)(ET9LMENABLED(pLingInfo->pLingCmnInfo) && pLingInfo->pLingCmnInfo->Private.ALdbLM.bSupported);

    ET9Assert(pLingInfo);

    /* If there is no word, no need to go any further   */

    if (!wLength && !ET9DEVSTATEINHBNOMDBNWP_MODE(pLingCmnInfo->Private.dwDevStateBits)) {
        return ET9STATUS_NONE;
    }

    /* Unless MDB is registered and active, return      */

    if ((pLingInfo->sMDBInfo.pReadMdbData == NULL) ||
        !(pLingInfo->sMDBInfo.wStatus & ET9_MDB_REGISTERED_MASK) ||
        !ET9MDBENABLED(pLingCmnInfo)) {

        return ET9STATUS_NONE;
    }

    dwWordListIdx = 0;

    for (;;) {

        ET9STATUS wStatus;
        ET9AWPrivWordInfo sLocalWord;

        _InitPrivWordInfo(&sLocalWord);

        pLingCmnInfo->Private.bMDBWordSource = ET9WORDSRC_MDB;

        wStatus = pLingInfo->sMDBInfo.pReadMdbData(pLingInfo,
                                                   ET9MDBGETALLWORDS,
                                                   wMinWordLen,
                                                   ET9MAXWORDSIZE,
                                                   sLocalWord.Base.sWord,
                                                   &sLocalWord.Base.wWordLen,
                                                   &dwWordListIdx);

        if (wStatus) {
            break;
        }

        if (sLocalWord.Base.wWordLen < wMinWordLen) {
            ET9Assert(0);
            continue;
        }

        sLocalWord.bWordSrc = pLingCmnInfo->Private.bMDBWordSource;

        /* the following assigns a frequency that keeps the MDB */
        /* entries in the order they are picked from the MDB    */

        if (bUsingLM) {
            sLocalWord.xWordFreq = ((ET9FREQPART)ET9_SUPP_DB_BASE_FREQ > (ET9FREQPART)dwWordListIdx) ? ((ET9FREQPART)ET9_SUPP_DB_BASE_FREQ - (ET9FREQPART)dwWordListIdx) : 1;
        }
        else {
            sLocalWord.xWordFreq = ((ET9FREQPART)ET9_SUPP_DB_BASE_FREQ_UNI > (ET9FREQPART)dwWordListIdx) ? ((ET9FREQPART)ET9_SUPP_DB_BASE_FREQ_UNI - (ET9FREQPART)dwWordListIdx) : 1;
        }

        switch (sLocalWord.bWordSrc)
        {
            case ET9WORDSRC_CSP:
                sLocalWord.xWordFreq = (ET9FREQPART)(sLocalWord.xWordFreq * 10);
                break;
            case ET9WORDSRC_MDB:
                sLocalWord.xWordFreq = (ET9FREQPART)(sLocalWord.xWordFreq / 10);
                if (sLocalWord.xWordFreq < 1) {
                    sLocalWord.xWordFreq = 1;
                }
                break;
            default:
                break;
        }

        sLocalWord.xTapFreq = 1;
        sLocalWord.wEWordFreq = 0;
        sLocalWord.wTWordFreq = 0;
        sLocalWord.dwWordIndex = 0;

        if (ET9AWSys_GetBilingualSupported(pLingInfo)) {
            sLocalWord.Base.bLangIndex = ET9AWBOTH_LANGUAGES;
        }
        else {
            sLocalWord.Base.bLangIndex = ET9AWFIRST_LANGUAGE;
        }

        /* if word being compounded, downshift it */

        if (wIndex) {

            ET9UINT wCount;
            ET9SYMB *pSymb;

            pSymb = sLocalWord.Base.sWord;
            for (wCount = sLocalWord.Base.wWordLen; wCount; --wCount, ++pSymb) {
                *pSymb = _ET9SymToLower(*pSymb, pLingCmnInfo->wLdbNum);
            }
        }

        wStatus = _ET9AWSelLstWordSearch(pLingInfo, &sLocalWord, wIndex, wLength, bFreqIndicator);

        if (wStatus) {
            return wStatus;
        }
    }

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Find a word in the MDB.
 * Checks if a word exists in the MDB or not (case insensitive).
 *
 * @param pLingInfo             Pointer to alphabetic information structure.
 * @param pWord                 Pointer to lookup word.
 *
 * @return ET9STATUS_NO_MATCHING_WORDS or ET9STATUS_WORD_EXISTS.
 */

ET9STATUS ET9FARCALL _ET9AWMdbFind(ET9AWLingInfo                * const pLingInfo,
                                   ET9AWPrivWordInfo      const * const pWord)
{
    ET9AWLingCmnInfo * const pLingCmnInfo = pLingInfo->pLingCmnInfo;

    ET9U32              dwWordListIdx;
    const ET9U16        wTargetWordLen = pWord->Base.wWordLen;
    ET9SYMB             psTargetWord[ET9MAXWORDSIZE];

    ET9Assert(pLingInfo);

    /* Unless MDB is registered and active, return */

    if ((!pLingInfo->sMDBInfo.pReadMdbData) ||
        !(pLingInfo->sMDBInfo.wStatus & ET9_MDB_REGISTERED_MASK) ||
        !ET9MDBENABLED(pLingCmnInfo)) {

        return ET9STATUS_NO_MATCHING_WORDS;
    }

    /* down case target */

    {
        ET9UINT wCount;
        ET9SYMB *psDst;
        ET9SYMB const *psSrc;

        psDst = psTargetWord;
        psSrc = pWord->Base.sWord;
        for (wCount = wTargetWordLen; wCount; --wCount, ++psSrc, ++psDst) {
            *psDst = _ET9SymToLower(*psSrc, 0);
        }
    }

    /* search */

    dwWordListIdx = 0;

    for (;;) {

        ET9STATUS wStatus;

        ET9U16 wMdbWordLen;
        ET9SYMB psMdbWord[ET9MAXWORDSIZE];

        wStatus = pLingInfo->sMDBInfo.pReadMdbData(pLingInfo,
                                                   ET9MDBGETALLWORDS,
                                                   wTargetWordLen,
                                                   ET9MAXWORDSIZE,
                                                   psMdbWord,
                                                   &wMdbWordLen,
                                                   &dwWordListIdx);

        if (wStatus) {
            break;
        }

        if (wMdbWordLen != wTargetWordLen) {
            continue;
        }

        {
            ET9UINT wCount;
            ET9SYMB *psMdb;
            ET9SYMB const *psTarget;

            psMdb = psMdbWord;
            psTarget = psTargetWord;
            for (wCount = wTargetWordLen; wCount; --wCount, ++psMdb, ++psTarget) {
                if (*psTarget != _ET9SymToLower(*psMdb, 0)) {
                    break;
                }
            }

            if (!wCount) {
                return ET9STATUS_WORD_EXISTS;
            }
        }
    }

    return ET9STATUS_NO_MATCHING_WORDS;
}

/*---------------------------------------------------------------------------*/
/**
 * Register a manufacturer database.
 *
 * @param pLingInfo                 Pointer to alphabetic information structure.
 * @param ET9ReadMdbData            Read MDB function.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL ET9AWRegisterMDB(ET9AWLingInfo * const     pLingInfo,
                                      const ET9MDBCALLBACK      ET9ReadMdbData)
{
    ET9STATUS wStatus;

    wStatus = _ET9AWSys_BasicValidityCheck(pLingInfo);

    if (wStatus) {
        return wStatus;
    }

    if (!ET9ReadMdbData) {
        return ET9STATUS_INVALID_MEMORY;
    }

    pLingInfo->sMDBInfo.pReadMdbData = ET9ReadMdbData;
    pLingInfo->sMDBInfo.wStatus = ET9_MDB_REGISTERED_MASK;
    pLingInfo->pLingCmnInfo->Private.bStateMDBEnabled = 1;

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/**
 * Unregister a manufacturer database.
 *
 * @param pLingInfo                 Pointer to alphabetic information structure.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL ET9AWUnregisterMDB(ET9AWLingInfo * const pLingInfo)
{
    ET9STATUS wStatus;

    wStatus = _ET9AWSys_BasicValidityCheck(pLingInfo);

    if (wStatus) {
        return wStatus;
    }

    pLingInfo->sMDBInfo.wStatus = 0;
    pLingInfo->sMDBInfo.pReadMdbData = NULL;
    pLingInfo->pLingCmnInfo->Private.bStateMDBEnabled = 0;

    return ET9STATUS_NONE;
}


#endif /* ET9_ALPHABETIC_MODULE */
/*! @} */
/* ----------------------------------< eof >--------------------------------- */
