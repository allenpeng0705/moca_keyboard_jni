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
;**     FileName: et9aimu.c                                                   **
;**                                                                           **
;**  Description: Generic input module functionality                          **
;**                                                                           **
;*******************************************************************************
;******************************************************************************/

/*! \addtogroup et9aimu Generic input for alphabetic
* Generic input for alphabetic XT9.
* @{
*/

#include "et9api.h"

#ifdef ET9_ALPHABETIC_MODULE

#include "et9sym.h"
#include "et9aimu.h"
#include "et9misc.h"
#include "et9amisc.h"
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
 * Alphabetic lock word (local).
 * This function locks a word in the selection list, by index.
 *
 * @param pLingCmnInfo              Pointer to alphabetic common information structure.
 * @param byWordIndex               Index to word to lock.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

static ET9STATUS ET9LOCALCALL __ET9AWLockWord(ET9AWLingCmnInfo  * const pLingCmnInfo,
                                              const ET9U8               byWordIndex)
{
    ET9STATUS           wStatus;
    ET9SimpleWord       sSimpleWord;
    ET9AWPrivWordInfo   *psLockedWord;
    ET9WordSymbInfo     *pWordSymbInfo;

    ET9Assert(pLingCmnInfo);

    if (pLingCmnInfo->Base.bSymbsInfoInvalidated) {
        return ET9STATUS_NEED_SELLIST_BUILD;
    }

    pWordSymbInfo = pLingCmnInfo->Base.pWordSymbInfo;

    if (byWordIndex >= pLingCmnInfo->Private.bTotalWords) {
        return ET9STATUS_OUT_OF_RANGE;
    }
    if (pWordSymbInfo->bNumSymbs < pLingCmnInfo->Private.bTotalSymbInputs) {
        return ET9STATUS_ERROR;
    }

    psLockedWord = &pLingCmnInfo->Private.pWordList[pLingCmnInfo->Private.bWordList[byWordIndex]];

    _ET9PrivWordToSimpleWord(psLockedWord, &sSimpleWord);

    if (psLockedWord->Base.wWordLen < ET9MAXWORDSIZE) {
        pLingCmnInfo->Private.pbLangSupported[psLockedWord->Base.wWordLen] = psLockedWord->Base.bLangIndex;
    }

    wStatus = _ET9LockWord(pWordSymbInfo, &sSimpleWord, (ET9AWLANGUAGESOURCE)psLockedWord->Base.bLangIndex);

    if (wStatus) {
        return wStatus;
    }

    /* if locking, and user didn't force shift/capslock, want to downshift right-hand word */

    if (pWordSymbInfo->Private.eLastShiftState == ET9NOSHIFT && pWordSymbInfo->Private.bCompoundingDownshift != 1) {
        pWordSymbInfo->Private.bCompoundingDownshift = pWordSymbInfo->bNumSymbs;
    }

    if (byWordIndex == pLingCmnInfo->Private.bExactIndex) {
        pWordSymbInfo->SymbsInfo[pWordSymbInfo->bNumSymbs - 1].bLocked = ET9EXACTLOCK;

    }

    /* check to see if wPreviousWordLanguage update is warranted based on locked word */

    if (psLockedWord->Base.bLangIndex == ET9AWBOTH_LANGUAGES) {
        if (!pLingCmnInfo->Private.wPreviousWordLanguage) {
            pLingCmnInfo->Private.wPreviousWordLanguage = pLingCmnInfo->wFirstLdbNum;
        }
    }
    else if (psLockedWord->Base.bLangIndex == ET9AWSECOND_LANGUAGE) {
        pLingCmnInfo->Private.wPreviousWordLanguage = pLingCmnInfo->wSecondLdbNum;
    }
    else {
        pLingCmnInfo->Private.wPreviousWordLanguage = pLingCmnInfo->wFirstLdbNum;
    }

    return wStatus;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Determine if an input symbol calls for unconditional next locking.
 *
 * @param pSymbInfo                 Pointer to an input symbol.
 *
 * @return Non zero if exact, otherwise zero.
 */

#define IS_NEXT_LOCK_SYMB(pSymbInfo)                                                                    \
(                                                                                                       \
    pSymbInfo->bSymbType == ET9KTSMARTPUNCT ||                                                          \
    pSymbInfo->bSymbType == ET9KTPUNCTUATION ||                                                         \
    (                                                                                                   \
        pSymbInfo->bAmbigType == ET9EXACT &&                                                            \
        pSymbInfo->bNumBaseSyms == 1 &&                                                                 \
        pSymbInfo->DataPerBaseSym[0].bNumSymsToMatch == 1 &&                                            \
        _ET9_IsPunctChar(pSymbInfo->DataPerBaseSym[0].sChar[0])                                         \
    )                                                                                                   \
)                                                                                                       \

/*---------------------------------------------------------------------------*/
/** \internal
 * AW input imminent symb handler.
 *
 * @param pBaseLingInfo             Pointer to base of alphabetic information structure.
 * @param bImplicitLock             Implicit lock setting.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

void ET9FARCALL _ET9AWImminentSymb(ET9BaseLingInfo      * const pBaseLingInfo,
                                   const ET9BOOL                bImplicitLock)
{
    ET9AWLingCmnInfo * const pLingCmnInfo = (ET9AWLingCmnInfo*)pBaseLingInfo;
    ET9WordSymbInfo * const pWordSymbInfo = pLingCmnInfo->Base.pWordSymbInfo;

    ET9Assert(pBaseLingInfo);

    if (pWordSymbInfo->bNumSymbs &&
        !pLingCmnInfo->Base.bSymbsInfoInvalidated &&
        pWordSymbInfo->Private.bCurrSelListIndex != ET9_NO_ACTIVE_INDEX &&
        pWordSymbInfo->Private.bCurrSelListIndex != pLingCmnInfo->Private.bDefaultIndex) {

        ET9SymbInfo * const pLastSymbInfo = &pWordSymbInfo->SymbsInfo[pWordSymbInfo->bNumSymbs - 1];

        /* lock here, unless "next locking off" prohibits it */

        if (ET9NEXTLOCKING_MODE(pWordSymbInfo->dwStateBits) || IS_NEXT_LOCK_SYMB(pLastSymbInfo) || bImplicitLock) {

            __ET9AWLockWord(pLingCmnInfo, pWordSymbInfo->Private.bCurrSelListIndex);
        }
    }
}

/*---------------------------------------------------------------------------*/
/**
 * Alphabetic lock word.
 * This function locks a word in the selection list, by index.
 *
 * @param pLingInfo                 Pointer to alphabetic information structure.
 * @param byWordIndex               Index to word to lock.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL ET9AWLockWord(ET9AWLingInfo     * const pLingInfo,
                                   const ET9U8               byWordIndex)
{
    ET9STATUS wStatus;

    wStatus = _ET9AWSys_BasicValidityCheck(pLingInfo);

    if (wStatus) {
        return wStatus;
    }

    wStatus = __ET9AWLockWord(pLingInfo->pLingCmnInfo, byWordIndex);

    if (wStatus) {
        return wStatus;
    }

    return ET9STATUS_NONE;
}


#endif /* ET9_ALPHABETIC_MODULE */
/*! @} */
/* ----------------------------------< eof >--------------------------------- */
