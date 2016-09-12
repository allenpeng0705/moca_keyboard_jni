/*******************************************************************************
;*******************************************************************************
;**                                                                           **
;**                    COPYRIGHT 2001-2011 NUANCE COMMUNICATIONS              **
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
;**     FileName: et9asys.c                                                   **
;**                                                                           **
;**  Description: ET9 Alphabetic System Module                                **
;**                                                                           **
;*******************************************************************************
;******************************************************************************/

/*! \addtogroup et9asys System for alphabetic
* System for alphabetic XT9.
* @{
*/

#include "et9api.h"
#ifdef ET9_ALPHABETIC_MODULE
#include "et9imu.h"
#include "et9sym.h"
#include "et9aldb.h"
#include "et9asys.h"
#include "et9asym.h"
#include "et9amisc.h"
#include "et9aasdb.h"
#ifdef EVAL_BUILD
#include "__et9eval.h"
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


/*---------------------------------------------------------------------------*/
/** \internal
 * Product identifier.
 */

const ET9U8 _pbXt9Alphabetic[] = { 'c', 'o', 'm', '.', 'n', 'u', 'a', 'n', 'c', 'e', '.', 'x', 't', '9', '.', 'a', 'l', 'p', 'h', 'a', 'b', 'e', 't', 'i', 'c', 0 };

/*---------------------------------------------------------------------------*/
/** \internal
 * This function will initialize the word symb info.
 *
 * @param pLingCmnInfo       Pointer to alphabetic common information structure.
 * @param pWordSymbInfo      Pointer to word symb info object.
 * @param bResetLingCmn      If the object should be reset or not (when already initialized).
 * @param bListSize          Word list size.
 * @param pWordList          Pointer to word list.
 *
 * @return None
 */

static ET9STATUS ET9LOCALCALL __SysCmnInit(ET9AWLingCmnInfo      * const pLingCmnInfo,
                                           ET9WordSymbInfo       * const pWordSymbInfo,
                                           const ET9BOOL                 bResetLingCmn,
                                           const ET9U8                   bListSize,
                                           ET9AWPrivWordInfo     * const pWordList)
{
    ET9STATUS eStatus;

    if (_ET9ByteStringCheckSum(_pbXt9Alphabetic) != 2933639753U) {
        return ET9STATUS_ERROR;
    }

    if (pLingCmnInfo == NULL) {
        return ET9STATUS_INVALID_MEMORY;
    }

    if (!bResetLingCmn && pLingCmnInfo->Private.wInfoInitOK == ET9GOODSETUP) {
        return ET9STATUS_NONE;
    }

    if (pWordList == NULL || pWordSymbInfo == NULL) {
        return ET9STATUS_INVALID_MEMORY;
    }

    if (bListSize < ET9MINSELLISTSIZE || bListSize > ET9MAXSELLISTSIZE) {
        return ET9STATUS_OUT_OF_RANGE;
    }

    _ET9ClearMem((ET9U8*)pLingCmnInfo, sizeof(ET9AWLingCmnInfo));

    pLingCmnInfo->pASDBInfo = NULL;
    pLingCmnInfo->pCDBInfo = NULL;
    pLingCmnInfo->pRUDBInfo = NULL;

    pLingCmnInfo->Base.pWordSymbInfo = pWordSymbInfo;

    eStatus = _ET9WordSymbInit(pWordSymbInfo);

    if (eStatus) {
        return eStatus;
    }

    if (!pWordSymbInfo->Private.ppEditionsList[ET9EDITION_AW_1] ||
        pWordSymbInfo->Private.ppEditionsList[ET9EDITION_AW_1] == (ET9BaseLingInfo*)pLingCmnInfo) {

        pWordSymbInfo->Private.ppEditionsList[ET9EDITION_AW_1] = (ET9BaseLingInfo*)pLingCmnInfo;
    }
    else if (!pWordSymbInfo->Private.ppEditionsList[ET9EDITION_AW_2] ||
             pWordSymbInfo->Private.ppEditionsList[ET9EDITION_AW_2] == (ET9BaseLingInfo*)pLingCmnInfo) {

        pWordSymbInfo->Private.ppEditionsList[ET9EDITION_AW_2] = (ET9BaseLingInfo*)pLingCmnInfo;
    }
    else {
        return ET9STATUS_ERROR;
    }

    pLingCmnInfo->Private.bListSize = bListSize;
    pLingCmnInfo->Private.pWordList = pWordList;
    pLingCmnInfo->Private.wCurrBuildLang = ET9PLIDNone;
    pLingCmnInfo->Private.wCurrBuildSecondLanguage = ET9PLIDNone;

    /* Initially default to word completion, next word prediction enabled */
    /* Initially with exact in list, auto append in list, QUDB supported  */
    /* Also 'Active Language Switch' (even though bilingual disabled)     */

    pLingCmnInfo->Private.bStateWordCompletion = 1;
    pLingCmnInfo->Private.bStateNextWordPrediction = 1;
    pLingCmnInfo->Private.bStateExactInList = 1;
    pLingCmnInfo->Private.bStateAutoAppendInList = 1;
    pLingCmnInfo->Private.bStateDownShiftDefault = 1;
    pLingCmnInfo->Private.bStateQUDBSupportEnabled = 1;
    pLingCmnInfo->Private.bStateActiveLangSwitch = 1;
    pLingCmnInfo->Private.bStateLM = 1;

    pLingCmnInfo->Private.eSelectionListMode = ET9ASLMODE_AUTO;
    pLingCmnInfo->Private.eSelectionListCorrectionMode = ET9ASLCORRECTIONMODE_MEDIUM;

    pLingCmnInfo->Private.wWordStemsPoint = 2;  /* but default off */
    pLingCmnInfo->Private.wWordCompletionPoint = 4;
    pLingCmnInfo->Private.wMaxCompletionCount = bListSize / 2;
    pLingCmnInfo->Private.bTotalExpTermPuncts = 0;
    pLingCmnInfo->Private.bContextWordSize = 0;
    pLingCmnInfo->Private.bPreviousContextWordSize = 0;
    pLingCmnInfo->Private.ASpc.eMode = ET9ASPCMODE_EXACT;
    pLingCmnInfo->Private.ASpc.wMaxSpcTermCount = 5;
    pLingCmnInfo->Private.ASpc.wMaxSpcCmplCount = 5;
    pLingCmnInfo->Private.ASpc.eSearchFilter = ET9ASPCSEARCHFILTER_UNFILTERED;
    pLingCmnInfo->Private.ASpc.eSearchFilterTrace = ET9ASPCTRACESEARCHFILTER_ONE_REGIONAL;

    pLingCmnInfo->Private.sBuildInfo.bCaptureInvalidated = 1;

    /* default fence values (though bilingual is off) */

    pLingCmnInfo->Private.bPrimaryFence = 2;
    pLingCmnInfo->Private.bSecondaryFence = 2;

    pLingCmnInfo->Private.wPreviousWordLanguage = 0;
    pLingCmnInfo->wLdbNum = ET9PLIDNone;

    pLingCmnInfo->Private.wInfoInitOK = ET9GOODSETUP;

    return ET9STATUS_NONE;
}


/*---------------------------------------------------------------------------*/
/**
 * System init.
 * This function will initialize Alphabetic word engine.
 *
 * @param pLingInfo          Pointer to alphabetic information structure.
 * @param pLingCmnInfo       Pointer to alphabetic common information structure.
 * @param pWordSymbInfo      Pointer to word symb info object.
 * @param bResetLingCmn      If the common ling object should be reset or not (when already initialized).
 * @param bListSize          Word list size.
 * @param pWordList          Pointer to word list.
 * @param pPublicExtension   A value the integration layer can set and then retrieve in pLingInfo->pPublicExtension.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL ET9AWSysInit(ET9AWLingInfo         * const pLingInfo,
                                  ET9AWLingCmnInfo      * const pLingCmnInfo,
                                  ET9WordSymbInfo       * const pWordSymbInfo,
                                  const ET9BOOL                 bResetLingCmn,
                                  const ET9U8                   bListSize,
                                  ET9AWPrivWordInfo     * const pWordList,
                                  void                  * const pPublicExtension)
{
    ET9STATUS wStatus;

    ET9Assert(EXACTOFFSET == 0x40);
    ET9Assert(EXACTISHOFFSET == 0x80);
    ET9Assert((ET9WORDSRC_QUDB - ET9WORDSRC_CDB) == (ET9WORDSRC_BUILDAROUND_QUDB   - ET9WORDSRC_BUILDAROUND_CDB));
    ET9Assert((ET9WORDSRC_QUDB - ET9WORDSRC_CDB) == (ET9WORDSRC_BUILDCOMPOUND_QUDB - ET9WORDSRC_BUILDCOMPOUND_CDB));

    if (pLingInfo == NULL || pLingCmnInfo == NULL) {
        return ET9STATUS_INVALID_MEMORY;
    }

    if ((wStatus = __SysCmnInit(pLingCmnInfo, pWordSymbInfo, bResetLingCmn, bListSize, pWordList)) != ET9STATUS_NONE) {
        return wStatus;
    }

    _ET9ClearMem((ET9U8 *)pLingInfo, sizeof(ET9AWLingInfo));

    pLingInfo->ET9AWLdbReadData = NULL;
    pLingInfo->pASDBWriteData = NULL;
    pLingInfo->pCDBWriteData = NULL;
    pLingInfo->pPublicExtension = NULL;
    pLingInfo->pRUDBWriteData = NULL;

    pLingInfo->Private.pConvertSymb = NULL;
    pLingInfo->Private.pConvertSymbInfo = NULL;

    pLingInfo->pPublicExtension = pPublicExtension;

    pLingInfo->Private.wInfoInitOK = ET9GOODSETUP;
    pLingInfo->Private.wLDBInitOK = 0;

    pLingInfo->pLingCmnInfo = pLingCmnInfo;

    _ET9AWSelLstResetWordList(pLingInfo);

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * This function resets the word list.
 *
 * @param pLingInfo      Pointer to alphabetic information structure.
 *
 * @return None
 */

void ET9FARCALL _ET9AWSelLstResetWordList(ET9AWLingInfo  *pLingInfo)
{

    ET9U8               bIndex;
    ET9U8               *pbWordList;

    ET9AWLingCmnInfo * const pLingCmnInfo = pLingInfo->pLingCmnInfo;

    const ET9U8 bListSize = pLingCmnInfo->Private.bListSize;

    ET9Assert(pLingInfo != NULL);
    ET9Assert(pLingCmnInfo != NULL);

    _ET9ClearMem((ET9U8*)pLingCmnInfo->Private.pWordList, (ET9U32)(bListSize * sizeof(ET9AWPrivWordInfo)));

    pLingCmnInfo->Private.bTotalSymbInputs = 0;
    pLingCmnInfo->Private.bTotalWords = 0;
    pLingCmnInfo->Private.bTotalSpcTermWords = 0;
    pLingCmnInfo->Private.bTotalSpcCmplWords = 0;
    pLingCmnInfo->Private.bTotalCompletionWords = 0;
    pLingCmnInfo->Private.dwStateBits = 0;
    pLingCmnInfo->Private.pLastWord = NULL;
    pLingCmnInfo->Private.pLastSpcTermWord = NULL;
    pLingCmnInfo->Private.pLastSpcCmplWord = NULL;
    pLingCmnInfo->Private.pLastCompletionWord = NULL;

    pLingCmnInfo->Private.bDefaultIndex = ET9_NO_ACTIVE_INDEX;

    pbWordList = &pLingCmnInfo->Private.bWordList[0];

    for (bIndex = 0; bIndex < bListSize; ++bIndex, ++pbWordList) {
        *pbWordList = bIndex;
    }

    _InitPrivWordInfo(&pLingCmnInfo->Private.sLeftHandWord);
}

/*---------------------------------------------------------------------------*/
/**
 * Sets the word stems point.
 * This function sets the number of taps before word stems are added to the selection list.
 *
 * @param pLingInfo                  Pointer to alphabetic information structure.
 * @param wWordStemsPoint            Number of taps.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL ET9AWSysSetWordStemsPoint(ET9AWLingInfo * const pLingInfo,
                                               const ET9U16          wWordStemsPoint)
{
    ET9STATUS wStatus;

    wStatus = _ET9AWSys_BasicValidityCheck(pLingInfo);

    if (wStatus) {
        return wStatus;
    }

    wStatus = _ET9SettingsInhibited(pLingInfo->pLingCmnInfo->Base.pWordSymbInfo);

    if (wStatus) {
        return wStatus;
    }

    if (!wWordStemsPoint || wWordStemsPoint > ET9MAXWORDSIZE) {
        return ET9STATUS_BAD_PARAM;
    }

    if (pLingInfo->pLingCmnInfo->Private.wWordStemsPoint != wWordStemsPoint) {

        pLingInfo->pLingCmnInfo->Private.wWordStemsPoint = wWordStemsPoint;

        _ET9InvalidateSymbInfo(pLingInfo->pLingCmnInfo->Base.pWordSymbInfo);
    }

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/**
 * Sets the word completion point.
 * This function sets the number of taps before word completions are added to the selection list.
 *
 * @param pLingInfo                  Pointer to alphabetic information structure.
 * @param wWordCompletionPoint       Number of taps.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL ET9AWSysSetWordCompletionPoint(ET9AWLingInfo * const pLingInfo,
                                                    const ET9U16          wWordCompletionPoint)
{
    ET9STATUS wStatus;

    wStatus = _ET9AWSys_BasicValidityCheck(pLingInfo);

    if (wStatus) {
        return wStatus;
    }

    wStatus = _ET9SettingsInhibited(pLingInfo->pLingCmnInfo->Base.pWordSymbInfo);

    if (wStatus) {
        return wStatus;
    }

    if (!wWordCompletionPoint || wWordCompletionPoint > ET9MAXWORDSIZE) {
        return ET9STATUS_BAD_PARAM;
    }

    if (pLingInfo->pLingCmnInfo->Private.wWordCompletionPoint != wWordCompletionPoint) {

        pLingInfo->pLingCmnInfo->Private.wWordCompletionPoint = wWordCompletionPoint;

        _ET9InvalidateSymbInfo(pLingInfo->pLingCmnInfo->Base.pWordSymbInfo);
    }

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/**
 * Sets the word completion count.
 * This function set max number of completions (stems) in the selection list.
 * The count is applied per stem distance chunk (one in classic mode).
 *
 * @param pLingInfo          Pointer to alphabetic information structure.
 * @param wCount             Number of completions (1 to max list size).
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL ET9AWSysSetCompletionCount(ET9AWLingInfo * const   pLingInfo,
                                                const ET9U16            wCount)
{
    ET9STATUS wStatus;

    wStatus = _ET9AWSys_BasicValidityCheck(pLingInfo);

    if (wStatus) {
        return wStatus;
    }

    wStatus = _ET9SettingsInhibited(pLingInfo->pLingCmnInfo->Base.pWordSymbInfo);

    if (wStatus) {
        return wStatus;
    }

    if (!wCount || (wCount > ET9MAXSELLISTSIZE)) {
        return ET9STATUS_BAD_PARAM;
    }

    if (pLingInfo->pLingCmnInfo->Private.wMaxCompletionCount != wCount) {

        pLingInfo->pLingCmnInfo->Private.wMaxCompletionCount = wCount;

        _ET9InvalidateSymbInfo(pLingInfo->pLingCmnInfo->Base.pWordSymbInfo);
    }

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/**
 * This function turns word stems ON for all DB searching and selection list building.
 *
 * @param pLingInfo          Pointer to alphabetic information structure.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL ET9AWSetDBStems(ET9AWLingInfo * const pLingInfo)
{
    ET9STATUS wStatus;

    wStatus = _ET9AWSys_BasicValidityCheck(pLingInfo);

    if (wStatus) {
        return wStatus;
    }

    wStatus = _ET9SettingsInhibited(pLingInfo->pLingCmnInfo->Base.pWordSymbInfo);

    if (wStatus) {
        return wStatus;
    }

    if (!pLingInfo->pLingCmnInfo->Private.bStateWordStems) {

        pLingInfo->pLingCmnInfo->Private.bStateWordStems = 1;

        _ET9InvalidateSymbInfo(pLingInfo->pLingCmnInfo->Base.pWordSymbInfo);
    }

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/**
 * This function turns word completion ON for all DB searching and selection list building.
 *
 * @param pLingInfo          Pointer to alphabetic information structure.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL ET9AWSetDBCompletion(ET9AWLingInfo * const pLingInfo)
{
    ET9STATUS wStatus;

    wStatus = _ET9AWSys_BasicValidityCheck(pLingInfo);

    if (wStatus) {
        return wStatus;
    }

    wStatus = _ET9SettingsInhibited(pLingInfo->pLingCmnInfo->Base.pWordSymbInfo);

    if (wStatus) {
        return wStatus;
    }

    if (!pLingInfo->pLingCmnInfo->Private.bStateWordCompletion) {

        pLingInfo->pLingCmnInfo->Private.bStateWordCompletion = 1;

        _ET9InvalidateSymbInfo(pLingInfo->pLingCmnInfo->Base.pWordSymbInfo);
    }

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/**
 * This function turns next word prediction ON for all context DB (CDB) searching and selection list building.
 *
 * @param pLingInfo          Pointer to alphabetic information structure.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL ET9AWSetDBPrediction(ET9AWLingInfo * const pLingInfo)
{
    ET9STATUS wStatus;

    wStatus = _ET9AWSys_BasicValidityCheck(pLingInfo);

    if (wStatus) {
        return wStatus;
    }

    wStatus = _ET9SettingsInhibited(pLingInfo->pLingCmnInfo->Base.pWordSymbInfo);

    if (wStatus) {
        return wStatus;
    }

    if (!pLingInfo->pLingCmnInfo->Private.bStateNextWordPrediction) {

        pLingInfo->pLingCmnInfo->Private.bStateNextWordPrediction = 1;

        _ET9InvalidateSymbInfo(pLingInfo->pLingCmnInfo->Base.pWordSymbInfo);
    }

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/**
 * This function turns QUDB logic ON so that UDB words added internally are initially qualified as in 'UDB purgatory'.
 *
 * @param pLingInfo          Pointer to alphabetic information structure.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL ET9AWSetUDBDelayedReorder(ET9AWLingInfo * const pLingInfo)
{
    ET9STATUS wStatus;

    wStatus = _ET9AWSys_BasicValidityCheck(pLingInfo);

    if (wStatus) {
        return wStatus;
    }

    wStatus = _ET9SettingsInhibited(pLingInfo->pLingCmnInfo->Base.pWordSymbInfo);

    if (wStatus) {
        return wStatus;
    }

    if (!pLingInfo->pLingCmnInfo->Private.bStateQUDBSupportEnabled) {

        pLingInfo->pLingCmnInfo->Private.bStateQUDBSupportEnabled = 1;

        _ET9InvalidateSymbInfo(pLingInfo->pLingCmnInfo->Base.pWordSymbInfo);
    }

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/**
 * This function turns word stems OFF for all DB searching and selection list building.
 *
 * @param pLingInfo          Pointer to alphabetic information structure.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL ET9AWClearDBStems(ET9AWLingInfo * const pLingInfo)
{
    ET9STATUS wStatus;

    wStatus = _ET9AWSys_BasicValidityCheck(pLingInfo);

    if (wStatus) {
        return wStatus;
    }

    wStatus = _ET9SettingsInhibited(pLingInfo->pLingCmnInfo->Base.pWordSymbInfo);

    if (wStatus) {
        return wStatus;
    }

    if (pLingInfo->pLingCmnInfo->Private.bStateWordStems) {

        pLingInfo->pLingCmnInfo->Private.bStateWordStems = 0;

        _ET9InvalidateSymbInfo(pLingInfo->pLingCmnInfo->Base.pWordSymbInfo);
    }

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/**
 * This function turns word completion OFF for all DB searching and selection list building.
 *
 * @param pLingInfo          Pointer to alphabetic information structure.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL ET9AWClearDBCompletion(ET9AWLingInfo * const pLingInfo)
{
    ET9STATUS wStatus;

    wStatus = _ET9AWSys_BasicValidityCheck(pLingInfo);

    if (wStatus) {
        return wStatus;
    }

    wStatus = _ET9SettingsInhibited(pLingInfo->pLingCmnInfo->Base.pWordSymbInfo);

    if (wStatus) {
        return wStatus;
    }

    if (pLingInfo->pLingCmnInfo->Private.bStateWordCompletion) {

        pLingInfo->pLingCmnInfo->Private.bStateWordCompletion = 0;

        _ET9InvalidateSymbInfo(pLingInfo->pLingCmnInfo->Base.pWordSymbInfo);
    }

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/**
 * This function turns next word prediction OFF for all context DB (CDB) searching and selection list building.
 *
 * @param pLingInfo          Pointer to alphabetic information structure.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL ET9AWClearDBPrediction(ET9AWLingInfo * const pLingInfo)
{
    ET9STATUS wStatus;

    wStatus = _ET9AWSys_BasicValidityCheck(pLingInfo);

    if (wStatus) {
        return wStatus;
    }

    wStatus = _ET9SettingsInhibited(pLingInfo->pLingCmnInfo->Base.pWordSymbInfo);

    if (wStatus) {
        return wStatus;
    }

    if (pLingInfo->pLingCmnInfo->Private.bStateNextWordPrediction) {

        pLingInfo->pLingCmnInfo->Private.bStateNextWordPrediction = 0;

        _ET9InvalidateSymbInfo(pLingInfo->pLingCmnInfo->Base.pWordSymbInfo);
    }

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/**
 * This function turns QUDB logic OFF so that UDB words added internally are always added as full UDB entries.
 *
 * @param pLingInfo          Pointer to alphabetic information structure.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL ET9AWClearUDBDelayedReorder(ET9AWLingInfo * const pLingInfo)
{
    ET9STATUS wStatus;

    wStatus = _ET9AWSys_BasicValidityCheck(pLingInfo);

    if (wStatus) {
        return wStatus;
    }

    wStatus = _ET9SettingsInhibited(pLingInfo->pLingCmnInfo->Base.pWordSymbInfo);

    if (wStatus) {
        return wStatus;
    }

    if (pLingInfo->pLingCmnInfo->Private.bStateQUDBSupportEnabled) {

        pLingInfo->pLingCmnInfo->Private.bStateQUDBSupportEnabled = 0;

        _ET9InvalidateSymbInfo(pLingInfo->pLingCmnInfo->Base.pWordSymbInfo);
    }

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Set spell correction features.
 * This function sets the spell correction mode.
 *
 * @param pLingInfo                     Pointer to alphabetic information structure.
 * @param bFeatures                     Features to set.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL ET9AWSysSetSpellCorrectionFeatures(ET9AWLingInfo       * const pLingInfo,
                                                        const ET9U8         bFeatures)
{
    ET9STATUS wStatus;

    wStatus = _ET9AWSys_BasicValidityCheck(pLingInfo);

    if (wStatus) {
        return wStatus;
    }

    pLingInfo->pLingCmnInfo->Private.ASpc.bSpcFeatures = bFeatures;

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/**
 * Set spell correction mode.
 * This function sets the spell correction mode.
 *
 * @param pLingInfo                     Pointer to alphabetic information structure.
 * @param eMode                         Number of completions (1 to max list size).
 * @param bSpellCorrectSecondaryLanguage Secondary language spell correction setting.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL ET9AWSysSetSpellCorrectionMode(ET9AWLingInfo       * const pLingInfo,
                                                    const ET9ASPCMODE   eMode,
                                                    const ET9BOOL       bSpellCorrectSecondaryLanguage)
{
    ET9STATUS wStatus;

    wStatus = _ET9AWSys_BasicValidityCheck(pLingInfo);

    if (wStatus) {
        return wStatus;
    }

    wStatus = _ET9SettingsInhibited(pLingInfo->pLingCmnInfo->Base.pWordSymbInfo);

    if (wStatus) {
        return wStatus;
    }

    if (eMode > ET9ASPCMODE_REGIONAL) {
        return ET9STATUS_BAD_PARAM;
    }

    if (bSpellCorrectSecondaryLanguage && (eMode == ET9ASPCMODE_OFF)) {
        return ET9STATUS_INVALID_MODE;
    }

    if (pLingInfo->pLingCmnInfo->Private.ASpc.eMode != eMode) {

        pLingInfo->pLingCmnInfo->Private.ASpc.eMode = eMode;

        _ET9InvalidateSymbInfo(pLingInfo->pLingCmnInfo->Base.pWordSymbInfo);
    }

    if (pLingInfo->pLingCmnInfo->Private.bStateInactiveLangSpellCorrect != bSpellCorrectSecondaryLanguage) {

        pLingInfo->pLingCmnInfo->Private.bStateInactiveLangSpellCorrect = bSpellCorrectSecondaryLanguage;

        _ET9InvalidateSymbInfo(pLingInfo->pLingCmnInfo->Base.pWordSymbInfo);
    }

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/**
 * Set spell correction filter for tap input.
 * This function sets the spell correction search filter.
 *
 * @param pLingInfo          Pointer to alphabetic information structure.
 * @param eFilter            Search filter.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL ET9AWSysSetSpellCorrectionSearchFilter(ET9AWLingInfo       * const pLingInfo,
                                                            const ET9ASPCSEARCHFILTER   eFilter)
{
    ET9STATUS wStatus;

    wStatus = _ET9AWSys_BasicValidityCheck(pLingInfo);

    if (wStatus) {
        return wStatus;
    }

    wStatus = _ET9SettingsInhibited(pLingInfo->pLingCmnInfo->Base.pWordSymbInfo);

    if (wStatus) {
        return wStatus;
    }

    switch (eFilter)
    {
        case ET9ASPCSEARCHFILTER_UNFILTERED:
        case ET9ASPCSEARCHFILTER_ONE_EXACT:
        case ET9ASPCSEARCHFILTER_ONE_REGIONAL:
        case ET9ASPCSEARCHFILTER_TWO_EXACT:
        case ET9ASPCSEARCHFILTER_TWO_REGIONAL:

            if (pLingInfo->pLingCmnInfo->Private.ASpc.eSearchFilter != eFilter) {

                pLingInfo->pLingCmnInfo->Private.ASpc.eSearchFilter = eFilter;

                _ET9InvalidateSymbInfo(pLingInfo->pLingCmnInfo->Base.pWordSymbInfo);
            }
            break;
        default:
            return ET9STATUS_BAD_PARAM;
    }

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \ingroup et9trace
 * Set spell correction filter for trace input.
 * This function sets the spell correction search filter.
 *
 * @param pLingInfo          Pointer to alphabetic information structure.
 * @param eFilter            Search filter.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL ET9AWSysSetSpellCorrectionTraceSearchFilter(ET9AWLingInfo          * const pLingInfo,
                                                                 const ET9ASPCTRACESEARCHFILTER eFilter)
{
    ET9STATUS wStatus;

    wStatus = _ET9AWSys_BasicValidityCheck(pLingInfo);

    if (wStatus) {
        return wStatus;
    }

    wStatus = _ET9SettingsInhibited(pLingInfo->pLingCmnInfo->Base.pWordSymbInfo);

    if (wStatus) {
        return wStatus;
    }

    switch (eFilter)
    {
        case ET9ASPCTRACESEARCHFILTER_ONE_EXACT:
        case ET9ASPCTRACESEARCHFILTER_ONE_REGIONAL:

            if (pLingInfo->pLingCmnInfo->Private.ASpc.eSearchFilterTrace != eFilter) {

                pLingInfo->pLingCmnInfo->Private.ASpc.eSearchFilterTrace = eFilter;

                _ET9InvalidateSymbInfo(pLingInfo->pLingCmnInfo->Base.pWordSymbInfo);
            }
            break;
        default:
            return ET9STATUS_BAD_PARAM;
    }

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/**
 * Set spell correction count.
 * This function sets the  max number of spell correction in the selection list.
 *
 * @param pLingInfo          Pointer to alphabetic information structure.
 * @param wCount             Max spell correction candidates in selection list (1 to max list size).
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL ET9AWSysSetSpellCorrectionCount(ET9AWLingInfo * const  pLingInfo,
                                                     const ET9U16           wCount)
{
    ET9STATUS wStatus;

    wStatus = _ET9AWSys_BasicValidityCheck(pLingInfo);

    if (wStatus) {
        return wStatus;
    }

    wStatus = _ET9SettingsInhibited(pLingInfo->pLingCmnInfo->Base.pWordSymbInfo);

    if (wStatus) {
        return wStatus;
    }

    if (!wCount || (wCount > ET9MAXSELLISTSIZE)) {
        return ET9STATUS_BAD_PARAM;
    }

    if (pLingInfo->pLingCmnInfo->Private.ASpc.wMaxSpcTermCount != wCount) {

        pLingInfo->pLingCmnInfo->Private.ASpc.wMaxSpcTermCount = wCount;

        _ET9InvalidateSymbInfo(pLingInfo->pLingCmnInfo->Base.pWordSymbInfo);
    }

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/**
 * Set selection list mode.
 * This function sets the selection list mode.
 *
 * @param pLingInfo          Pointer to alphabetic information structure.
 * @param eMode              The correction mode (low/high).
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL ET9AWSysSetSelectionListMode(ET9AWLingInfo             * const pLingInfo,
                                                  const ET9ASLCORRECTIONMODE        eMode)
{
    ET9STATUS wStatus;

    wStatus = _ET9AWSys_BasicValidityCheck(pLingInfo);

    if (wStatus) {
        return wStatus;
    }

    wStatus = _ET9SettingsInhibited(pLingInfo->pLingCmnInfo->Base.pWordSymbInfo);

    if (wStatus) {
        return wStatus;
    }

    if (eMode > ET9ASLCORRECTIONMODE_HIGH) {
        return ET9STATUS_BAD_PARAM;
    }

    if (pLingInfo->pLingCmnInfo->Private.eSelectionListCorrectionMode != eMode) {

        pLingInfo->pLingCmnInfo->Private.eSelectionListCorrectionMode = eMode;

        _ET9InvalidateSymbInfo(pLingInfo->pLingCmnInfo->Base.pWordSymbInfo);
    }

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/**
 * Set selection list mode.
 * This function sets the selection list mode.
 *
 * @param pLingInfo          Pointer to alphabetic information structure.
 * @param eMode              The mode (classic, completions promoted, etc).
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL _ET9AWSysSetSelectionListMode(ET9AWLingInfo    * const pLingInfo,
                                                   const ET9ASLMODE         eMode)
{
    ET9STATUS wStatus;

    wStatus = _ET9AWSys_BasicValidityCheck(pLingInfo);

    if (wStatus) {
        return wStatus;
    }

    wStatus = _ET9SettingsInhibited(pLingInfo->pLingCmnInfo->Base.pWordSymbInfo);

    if (wStatus) {
        return wStatus;
    }

    if (eMode > ET9ASLMODE_MIXED) {
        return ET9STATUS_BAD_PARAM;
    }

    if (pLingInfo->pLingCmnInfo->Private.eSelectionListMode != eMode) {

        pLingInfo->pLingCmnInfo->Private.eSelectionListMode = eMode;

        _ET9InvalidateSymbInfo(pLingInfo->pLingCmnInfo->Base.pWordSymbInfo);
    }

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Set alt mode.
 *
 * @param pLingInfo          Pointer to alphabetic information structure.
 * @param eMode              The alt mode.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL ET9AWSysSetAltMode(ET9AWLingInfo       * const pLingInfo,
                                        const ET9AW_AltMode         eMode)
{
    ET9STATUS wStatus;

    wStatus = _ET9AWSys_BasicValidityCheck(pLingInfo);

    if (wStatus) {
        return wStatus;
    }

    wStatus = _ET9SettingsInhibited(pLingInfo->pLingCmnInfo->Base.pWordSymbInfo);

    if (wStatus) {
        return wStatus;
    }

    if (eMode >= ET9AW_AltMode_Last) {
        return ET9STATUS_BAD_PARAM;
    }

    if (pLingInfo->pLingCmnInfo->Private.eAltMode != eMode) {

        pLingInfo->pLingCmnInfo->Private.eAltMode = eMode;

        _ET9InvalidateSymbInfo(pLingInfo->pLingCmnInfo->Base.pWordSymbInfo);
    }

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/**
 * Set selection list post sorting.
 * This function enables selection list post sorting.
 *
 * @param pLingInfo          Pointer to alphabetic information structure.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL ET9AWSysSetPostSort(ET9AWLingInfo * const pLingInfo)
{
    ET9STATUS wStatus;

    wStatus = _ET9AWSys_BasicValidityCheck(pLingInfo);

    if (wStatus) {
        return wStatus;
    }

    wStatus = _ET9SettingsInhibited(pLingInfo->pLingCmnInfo->Base.pWordSymbInfo);

    if (wStatus) {
        return wStatus;
    }

    if (!pLingInfo->pLingCmnInfo->Private.bStatePostSort) {

        pLingInfo->pLingCmnInfo->Private.bStatePostSort = 1;

        _ET9InvalidateSymbInfo(pLingInfo->pLingCmnInfo->Base.pWordSymbInfo);
    }

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/**
 * Clear selection list post sorting.
 * This function disables selection list post sorting.
 *
 * @param pLingInfo          Pointer to alphabetic information structure.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL ET9AWSysClearPostSort(ET9AWLingInfo * const pLingInfo)
{
    ET9STATUS wStatus;

    wStatus = _ET9AWSys_BasicValidityCheck(pLingInfo);

    if (wStatus) {
        return wStatus;
    }

    wStatus = _ET9SettingsInhibited(pLingInfo->pLingCmnInfo->Base.pWordSymbInfo);

    if (wStatus) {
        return wStatus;
    }

    if (pLingInfo->pLingCmnInfo->Private.bStatePostSort) {

        pLingInfo->pLingCmnInfo->Private.bStatePostSort = 0;

        _ET9InvalidateSymbInfo(pLingInfo->pLingCmnInfo->Base.pWordSymbInfo);
    }

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/**
 * ET9AWSysSetBigramLM
 * Set bigram language model.
 *
 * @param pLingInfo          Pointer to alphabetic information structure.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL ET9AWSysSetBigramLM(ET9AWLingInfo * const pLingInfo)
{
    ET9STATUS wStatus;

    wStatus = _ET9AWSys_BasicValidityCheck(pLingInfo);

    if (wStatus) {
        return wStatus;
    }

    wStatus = _ET9SettingsInhibited(pLingInfo->pLingCmnInfo->Base.pWordSymbInfo);

    if (wStatus) {
        return wStatus;
    }

    if (!pLingInfo->pLingCmnInfo->Private.bStateLM) {

        pLingInfo->pLingCmnInfo->Private.bStateLM = 1;

        _ET9InvalidateSymbInfo(pLingInfo->pLingCmnInfo->Base.pWordSymbInfo);
    }

    _ET9AWLdbSetActiveLanguage(pLingInfo, pLingInfo->pLingCmnInfo->wLdbNum);

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/**
 * ET9AWSysClearBigramLM
 * Clear bigram language model.
 *
 * @param pLingInfo          Pointer to alphabetic information structure.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL ET9AWSysClearBigramLM(ET9AWLingInfo * const pLingInfo)
{
    ET9STATUS wStatus;

    wStatus = _ET9AWSys_BasicValidityCheck(pLingInfo);

    if (wStatus) {
        return wStatus;
    }

    wStatus = _ET9SettingsInhibited(pLingInfo->pLingCmnInfo->Base.pWordSymbInfo);

    if (wStatus) {
        return wStatus;
    }

    if (pLingInfo->pLingCmnInfo->Private.bStateLM) {

        pLingInfo->pLingCmnInfo->Private.bStateLM = 0;

        _ET9InvalidateSymbInfo(pLingInfo->pLingCmnInfo->Base.pWordSymbInfo);
    }

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/**
 * ET9AWSysSetBoostTopCandidate
 * Set boost top candidate.
 *
 * @param pLingInfo          Pointer to alphabetic information structure.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL ET9AWSysSetBoostTopCandidate(ET9AWLingInfo * const pLingInfo)
{
    ET9STATUS wStatus;

    wStatus = _ET9AWSys_BasicValidityCheck(pLingInfo);

    if (wStatus) {
        return wStatus;
    }

    wStatus = _ET9SettingsInhibited(pLingInfo->pLingCmnInfo->Base.pWordSymbInfo);

    if (wStatus) {
        return wStatus;
    }

    if (!ET9LMENABLED(pLingInfo->pLingCmnInfo)) {
        return ET9STATUS_NO_LM;
    }

    if (!pLingInfo->pLingCmnInfo->Private.bStateBoostTopCandidate) {

        pLingInfo->pLingCmnInfo->Private.bStateBoostTopCandidate = 1;

        _ET9InvalidateSymbInfo(pLingInfo->pLingCmnInfo->Base.pWordSymbInfo);
    }

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/**
 * ET9AWSysClearBoostTopCandidate
 * Clear boost top candidate.
 *
 * @param pLingInfo          Pointer to alphabetic information structure.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL ET9AWSysClearBoostTopCandidate(ET9AWLingInfo * const pLingInfo)
{
    ET9STATUS wStatus;

    wStatus = _ET9AWSys_BasicValidityCheck(pLingInfo);

    if (wStatus) {
        return wStatus;
    }

    wStatus = _ET9SettingsInhibited(pLingInfo->pLingCmnInfo->Base.pWordSymbInfo);

    if (wStatus) {
        return wStatus;
    }

    if (pLingInfo->pLingCmnInfo->Private.bStateBoostTopCandidate) {

        pLingInfo->pLingCmnInfo->Private.bStateBoostTopCandidate = 0;

        _ET9InvalidateSymbInfo(pLingInfo->pLingCmnInfo->Base.pWordSymbInfo);
    }

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/**
 * Set user defined auto substitution.
 * This function turns user defined auto substitution ON for selection list building.
 *
 * @param pLingInfo          Pointer to alphabetic information structure.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL ET9AWSetUserDefinedAutoSubstitution(ET9AWLingInfo * const pLingInfo)
{
    ET9STATUS wStatus;

    wStatus = _ET9AWSys_BasicValidityCheck(pLingInfo);

    if (wStatus) {
        return wStatus;
    }

    wStatus = _ET9SettingsInhibited(pLingInfo->pLingCmnInfo->Base.pWordSymbInfo);

    if (wStatus) {
        return wStatus;
    }

    if (!pLingInfo->pLingCmnInfo->pASDBInfo) {
        return ET9STATUS_NO_ASDB;
    }

    if (!pLingInfo->pLingCmnInfo->Private.bStateUserDefinedAutosub) {

        pLingInfo->pLingCmnInfo->Private.bStateUserDefinedAutosub = 1;

        /* restart any active 'GetEntry' sequence if ASDB enabling/disabling occurs */

        pLingInfo->pLingCmnInfo->Private.pASDBGetEntry = 0;
        pLingInfo->pLingCmnInfo->Private.wLdbASGetEntryRec = 0;

        _ET9InvalidateSymbInfo(pLingInfo->pLingCmnInfo->Base.pWordSymbInfo);
    }

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/**
 * Clear user defined auto substitution.
 * This function turns user defined auto substitution OFF for selection list building.
 *
 * @param pLingInfo          Pointer to alphabetic information structure.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL ET9AWClearUserDefinedAutoSubstitution(ET9AWLingInfo * const pLingInfo)
{
    ET9STATUS wStatus;

    wStatus = _ET9AWSys_BasicValidityCheck(pLingInfo);

    if (wStatus) {
        return wStatus;
    }

    wStatus = _ET9SettingsInhibited(pLingInfo->pLingCmnInfo->Base.pWordSymbInfo);

    if (wStatus) {
        return wStatus;
    }

    if (pLingInfo->pLingCmnInfo->pASDBInfo == NULL) {
        return ET9STATUS_NO_ASDB;
    }

    if (pLingInfo->pLingCmnInfo->Private.bStateUserDefinedAutosub) {

        pLingInfo->pLingCmnInfo->Private.bStateUserDefinedAutosub = 0;

        /* restart any active 'GetEntry' sequence if ASDB enabling/disabling occurs */

        pLingInfo->pLingCmnInfo->Private.pASDBGetEntry = 0;
        pLingInfo->pLingCmnInfo->Private.wLdbASGetEntryRec = 0;

        _ET9InvalidateSymbInfo(pLingInfo->pLingCmnInfo->Base.pWordSymbInfo);
    }

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/**
 * Set LDB defined auto substitution.
 * This function turns LDB supported auto substitution ON for selection list building.
 *
 * @param pLingInfo          Pointer to alphabetic information structure.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL ET9AWSetLDBAutoSubstitution(ET9AWLingInfo * const pLingInfo)
{
    ET9STATUS wStatus;

    wStatus = _ET9AWSys_BasicValidityCheck(pLingInfo);

    if (wStatus) {
        return wStatus;
    }

    wStatus = _ET9SettingsInhibited(pLingInfo->pLingCmnInfo->Base.pWordSymbInfo);

    if (wStatus) {
        return wStatus;
    }

    if (!pLingInfo->pLingCmnInfo->Private.bStateLDBSupportedAutosub) {

        pLingInfo->pLingCmnInfo->Private.bStateLDBSupportedAutosub = 1;

        /* restart any active 'GetEntry' sequence if ASDB enabling/disabling occurs */

        pLingInfo->pLingCmnInfo->Private.pASDBGetEntry = 0;
        pLingInfo->pLingCmnInfo->Private.wLdbASGetEntryRec = 0;

        _ET9InvalidateSymbInfo(pLingInfo->pLingCmnInfo->Base.pWordSymbInfo);
    }

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/**
 * Clear LDB defined auto substitution.
 * This function turns LDB supported auto substitution OFF for selection list building.
 *
 * @param pLingInfo          Pointer to alphabetic information structure.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL ET9AWClearLDBAutoSubstitution(ET9AWLingInfo * const pLingInfo)
{
    ET9STATUS wStatus;

    wStatus = _ET9AWSys_BasicValidityCheck(pLingInfo);

    if (wStatus) {
        return wStatus;
    }

    wStatus = _ET9SettingsInhibited(pLingInfo->pLingCmnInfo->Base.pWordSymbInfo);

    if (wStatus) {
        return wStatus;
    }

    if (pLingInfo->pLingCmnInfo->Private.bStateLDBSupportedAutosub) {

        pLingInfo->pLingCmnInfo->Private.bStateLDBSupportedAutosub = 0;

        /* restart any active 'GetEntry' sequence if ASDB enabling/disabling occurs */

        pLingInfo->pLingCmnInfo->Private.pASDBGetEntry = 0;
        pLingInfo->pLingCmnInfo->Private.wLdbASGetEntryRec = 0;

        _ET9InvalidateSymbInfo(pLingInfo->pLingCmnInfo->Base.pWordSymbInfo);
    }

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/**
 * Set expand auto substitutions.
 * This function turns expand auto substitution ON for selection list building.
 * Rather than getting shortcut/substitution paires in the list the substitution will get dircetly inserted into the list as a word.
 *
 * @param pLingInfo          Pointer to alphabetic information structure.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL ET9AWSysSetExpandAutoSubstitutions(ET9AWLingInfo * const pLingInfo)
{
    ET9STATUS wStatus;

    wStatus = _ET9AWSys_BasicValidityCheck(pLingInfo);

    if (wStatus) {
        return wStatus;
    }

    wStatus = _ET9SettingsInhibited(pLingInfo->pLingCmnInfo->Base.pWordSymbInfo);

    if (wStatus) {
        return wStatus;
    }

    {
        const ET9UINT nMaxWordSize = ET9MAXWORDSIZE;
        const ET9UINT nMaxSubstitutionSize = ET9MAXSUBSTITUTIONSIZE;

        if (nMaxSubstitutionSize > nMaxWordSize) {
            return ET9STATUS_TOO_LONG_SUBSTITUTIONS;
        }
    }

    if (!pLingInfo->pLingCmnInfo->Private.bStateExpandAutosub) {

        pLingInfo->pLingCmnInfo->Private.bStateExpandAutosub = 1;

        _ET9InvalidateSymbInfo(pLingInfo->pLingCmnInfo->Base.pWordSymbInfo);
    }

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/**
 * Clear expand auto substitutions.
 * This function turns expand auto substitution OFF for selection list building.
 *
 * @param pLingInfo          Pointer to alphabetic information structure.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL ET9AWSysClearExpandAutoSubstitutions(ET9AWLingInfo * const pLingInfo)
{
    ET9STATUS wStatus;

    wStatus = _ET9AWSys_BasicValidityCheck(pLingInfo);

    if (wStatus) {
        return wStatus;
    }

    wStatus = _ET9SettingsInhibited(pLingInfo->pLingCmnInfo->Base.pWordSymbInfo);

    if (wStatus) {
        return wStatus;
    }

    if (pLingInfo->pLingCmnInfo->Private.bStateExpandAutosub) {

        pLingInfo->pLingCmnInfo->Private.bStateExpandAutosub = 0;

        _ET9InvalidateSymbInfo(pLingInfo->pLingCmnInfo->Base.pWordSymbInfo);
    }

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \ingroup et9trace
 * Set auto space.
 * This function turns auto space ON for selection list building.
 *
 * @param pLingInfo          Pointer to alphabetic information structure.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL ET9AWSysSetAutoSpace(ET9AWLingInfo * const pLingInfo)
{
    ET9STATUS wStatus;

    wStatus = _ET9AWSys_BasicValidityCheck(pLingInfo);

    if (wStatus) {
        return wStatus;
    }

    wStatus = _ET9SettingsInhibited(pLingInfo->pLingCmnInfo->Base.pWordSymbInfo);

    if (wStatus) {
        return wStatus;
    }

    if (!pLingInfo->pLingCmnInfo->Private.bStateAutoSpace) {

        pLingInfo->pLingCmnInfo->Private.bStateAutoSpace = 1;

        _ET9InvalidateSymbInfo(pLingInfo->pLingCmnInfo->Base.pWordSymbInfo);
    }

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \ingroup et9trace
 * Clear auto space.
 * This function turns auto space OFF for selection list building.
 *
 * @param pLingInfo          Pointer to alphabetic information structure.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL ET9AWSysClearAutoSpace(ET9AWLingInfo * const pLingInfo)
{
    ET9STATUS wStatus;

    wStatus = _ET9AWSys_BasicValidityCheck(pLingInfo);

    if (wStatus) {
        return wStatus;
    }

    wStatus = _ET9SettingsInhibited(pLingInfo->pLingCmnInfo->Base.pWordSymbInfo);

    if (wStatus) {
        return wStatus;
    }

    if (pLingInfo->pLingCmnInfo->Private.bStateAutoSpace) {

        pLingInfo->pLingCmnInfo->Private.bStateAutoSpace = 0;

        _ET9InvalidateSymbInfo(pLingInfo->pLingCmnInfo->Base.pWordSymbInfo);
    }

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/**
 * Get the current language's terminal punct sequence.
 * This function returns the current language's terminal punct sequence, the
 * length of the terminal punct sequence, along
 * with an indication as to whether the sequence is the default or a custom
 * sequence set by the integration layer. The incoming pTermPuncts buffer
 * needs to be at least ET9MAX_EXP_TERM_PUNCTS symbols long.
 *
 * @param pLingInfo          Pointer to alpha information structure.
 * @param wLdbNum            Language ID.
 * @param psPunctBuf         Pointer to buffer to collect term punct sequence.
 * @param bPunctBufLen       Length (in symbols) of the input buffer.
 * @param pbCount            Pointer to var to collect total terminal punct length.
 * @param pbDefault          Pointer to flag to collect default setting indication.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL ET9AWGetTermPuncts(ET9AWLingInfo * const pLingInfo,
                                        const ET9U16          wLdbNum,
                                        ET9SYMB       * const psPunctBuf,
                                        const ET9U8           bPunctBufLen,
                                        ET9U8         * const pbCount,
                                        ET9BOOL       * const pbDefault)
{
    ET9STATUS   wStatus;

    wStatus = _ET9AWSys_BasicValidityCheck(pLingInfo);

    if (wStatus) {
        return wStatus;
    }

    if (!psPunctBuf || !pbCount || !pbDefault) {
        return ET9STATUS_INVALID_MEMORY;
    }

    if (bPunctBufLen < ET9MAX_EXP_TERM_PUNCTS) {
        return ET9STATUS_BUFFER_TOO_SMALL;
    }

    {
        ET9U8       i;
        ET9U8       bTotalPuncts;

        bTotalPuncts = (ET9U8)_ET9_GetNumTermPunct(pLingInfo, wLdbNum);

        *pbCount = bTotalPuncts;
        *pbDefault = 1;
        for (i = 0; i < bTotalPuncts; ++i) {
            psPunctBuf[i] = _ET9_GetTermPunctChar(pLingInfo, wLdbNum, i);
        }

        if (pLingInfo->pLingCmnInfo->Private.bTotalExpTermPuncts) {
            *pbDefault = 0;
        }
    }

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/**
 * Get the current language's embedded punct symbol
 * This function returns the current language's embedded punct, along
 * with an indication as to whether the punct is the default or a custom
 * symbol set by the integration layer.
 *
 * @param pLingInfo          Pointer to alpha information structure.
 * @param wLdbNum            Language ID.
 * @param psPunct            Pointer to var to collect embedded punct.
 * @param pbDefault          Pointer to flag to collect default setting indication.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL ET9AWGetEmbeddedPunct(ET9AWLingInfo * const pLingInfo,
                                           const ET9U16          wLdbNum,
                                           ET9SYMB       * const psPunct,
                                           ET9BOOL       * const pbDefault)
{
    ET9STATUS   wStatus;

    wStatus = _ET9AWSys_BasicValidityCheck(pLingInfo);

    if (wStatus) {
        return wStatus;
    }

    if (!psPunct || !pbDefault) {
        return ET9STATUS_INVALID_MEMORY;
    }

    *pbDefault = 1;
    *psPunct = _ET9_GetEmbPunctChar(pLingInfo, wLdbNum);

    if (pLingInfo->pLingCmnInfo->Private.sExpEmbeddedPunct) {
        *pbDefault = 0;
    }

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/**
 * This function changes the terminal punctuation character set to use a custom set.
 *
 * @param pLingInfo          Pointer to alpha information structure.
 * @param psTermPuncts       Pointer to custom terminal punctionation set.
 * @param bTotalPuncts       Total terminal punctuation in the array.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL ET9AWSetCustomTermPuncts(ET9AWLingInfo * const     pLingInfo,
                                              ET9SYMB * const           psTermPuncts,
                                              const ET9U8               bTotalPuncts)
{
    ET9STATUS   wStatus;

    wStatus = _ET9AWSys_BasicValidityCheck(pLingInfo);

    if (wStatus) {
        return wStatus;
    }

    if (!psTermPuncts) {
        return ET9STATUS_INVALID_MEMORY;
    }

    if (!bTotalPuncts || bTotalPuncts > ET9MAX_EXP_TERM_PUNCTS) {
        return ET9STATUS_INVALID_SIZE;
    }

    {
        ET9SYMB     *psSrc;
        ET9UINT     nCount;

        psSrc = psTermPuncts;
        for (nCount = bTotalPuncts; nCount; --nCount, ++psSrc) {
            if (!*psSrc) {
                return ET9STATUS_BAD_PARAM;
            }
        }
    }

    {
        ET9SYMB     *psSrc;
        ET9SYMB     *psDst;
        ET9UINT     nCount;

        psSrc = psTermPuncts;
        psDst = pLingInfo->pLingCmnInfo->Private.sExpTermPuncts;
        for (nCount = bTotalPuncts; nCount; --nCount, ++psSrc, ++psDst) {
            *psDst = *psSrc;
        }
    }

    pLingInfo->pLingCmnInfo->Private.bTotalExpTermPuncts = bTotalPuncts;

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/**
 * This function resets the terminal punctuation character set to the default core set for the current language.
 *
 * @param pLingInfo          Pointer to alpha information structure.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL ET9AWSetDefaultTermPuncts(ET9AWLingInfo * const pLingInfo)
{
    ET9STATUS wStatus;

    wStatus = _ET9AWSys_BasicValidityCheck(pLingInfo);

    if (wStatus) {
        return wStatus;
    }

    pLingInfo->pLingCmnInfo->Private.bTotalExpTermPuncts = 0;

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/**
 * This function resets the embedded punctuation character to default.
 *
 * @param pLingInfo          Pointer to alpha information structure.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL ET9AWSetDefaultEmbeddedPunct(ET9AWLingInfo * const pLingInfo)
{
    ET9STATUS wStatus;

    wStatus = _ET9AWSys_BasicValidityCheck(pLingInfo);

    if (wStatus) {
        return wStatus;
    }

    pLingInfo->pLingCmnInfo->Private.sExpEmbeddedPunct = (ET9SYMB)0;

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/**
 * This function sets a custom embedded puntuation character.
 *
 * @param pLingInfo          Pointer to alpha information structure.
 * @param sEmbeddedPunct     Custom embedded character.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL ET9AWSetCustomEmbeddedPunct(ET9AWLingInfo * const pLingInfo,
                                                 const ET9SYMB         sEmbeddedPunct)
{
    ET9STATUS wStatus;

    wStatus = _ET9AWSys_BasicValidityCheck(pLingInfo);

    if (wStatus) {
        return wStatus;
    }

    if (!sEmbeddedPunct) {
        return ET9STATUS_INVALID_MEMORY;
    }

    pLingInfo->pLingCmnInfo->Private.sExpEmbeddedPunct = sEmbeddedPunct;

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/**
 * Set convert symbols.
 * This function controls the convert symbol feature (on-the-fly-mapping) for the alpha engine.
 * There is a partner function that should be set in the input module.
 * This feature applies to what goes into the selection list as well as e.g. multitap symbols.
 * This function will be called for every symbol handled. Thus performance wise it's important to keep the convert function fast.
 *
 * @param pLingInfo          Pointer to alpha information structure.
 * @param pConvertSymb       Pointer to a (callback) convert symbol function (NULL to turn the feature off).
 * @param pConvertSymbInfo   Integeration layer pointer that will be passed back in the callback.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL ET9AWSetConvertSymb(ET9AWLingInfo                * const pLingInfo,
                                         const ET9CONVERTSYMBCALLBACK         pConvertSymb,
                                         void                         * const pConvertSymbInfo)
{
    ET9STATUS wStatus;

    wStatus = _ET9AWSys_BasicValidityCheck(pLingInfo);

    if (wStatus) {
        return wStatus;
    }

    pLingInfo->Private.pConvertSymb     = pConvertSymb;
    pLingInfo->Private.pConvertSymbInfo = pConvertSymbInfo;

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/**
 * This function turns DBs to ON.
 *
 * @param pLingInfo          Pointer to alpha information structure.
 * @param dwDBMasks          DB masks to turn on DBs.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL ET9AWEnableDBs(ET9AWLingInfo * const pLingInfo,
                                    const ET9U32          dwDBMasks)
{
    ET9STATUS wStatus;
    ET9UINT nChangeCount = 0;

    wStatus = _ET9AWSys_BasicValidityCheck(pLingInfo);

    if (wStatus) {
        return wStatus;
    }

    wStatus = _ET9SettingsInhibited(pLingInfo->pLingCmnInfo->Base.pWordSymbInfo);

    if (wStatus) {
        return wStatus;
    }

    if (!pLingInfo->pLingCmnInfo->Private.bStateLDBEnabled && (dwDBMasks & ET9STATELDBENABLEDMASK)) {
        ++nChangeCount;
        pLingInfo->pLingCmnInfo->Private.bStateLDBEnabled = 1;
    }
    if (!pLingInfo->pLingCmnInfo->Private.bStateCDBEnabled && (dwDBMasks & ET9STATECDBENABLEDMASK)) {
        ++nChangeCount;
        pLingInfo->pLingCmnInfo->Private.bStateCDBEnabled = 1;
    }
    if (!pLingInfo->pLingCmnInfo->Private.bStateRUDBEnabled && (dwDBMasks & ET9STATERUDBENABLEDMASK)) {
        ++nChangeCount;
        pLingInfo->pLingCmnInfo->Private.bStateRUDBEnabled = 1;
    }
    if (!pLingInfo->pLingCmnInfo->Private.bStateASDBEnabled && (dwDBMasks & ET9STATEASDBENABLEDMASK)) {
        ++nChangeCount;
        pLingInfo->pLingCmnInfo->Private.bStateASDBEnabled = 1;
    }
    if (!pLingInfo->pLingCmnInfo->Private.bStateMDBEnabled && (dwDBMasks & ET9STATEMDBENABLEDMASK)) {
        ++nChangeCount;
        pLingInfo->pLingCmnInfo->Private.bStateMDBEnabled = 1;
    }

    if (nChangeCount) {
        _ET9InvalidateSymbInfo(pLingInfo->pLingCmnInfo->Base.pWordSymbInfo);
    }

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/**
 * This function turns DBs to OFF.
 *
 * @param pLingInfo          Pointer to alpha information structure.
 * @param dwDBMasks          DB masks to turn off DBs.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL ET9AWDisableDBs(ET9AWLingInfo * const pLingInfo,
                                     const ET9U32          dwDBMasks)
{
    ET9STATUS wStatus;
    ET9UINT nChangeCount = 0;

    wStatus = _ET9AWSys_BasicValidityCheck(pLingInfo);

    if (wStatus) {
        return wStatus;
    }

    wStatus = _ET9SettingsInhibited(pLingInfo->pLingCmnInfo->Base.pWordSymbInfo);

    if (wStatus) {
        return wStatus;
    }

    if (pLingInfo->pLingCmnInfo->Private.bStateLDBEnabled && (dwDBMasks & ET9STATELDBENABLEDMASK)) {
        ++nChangeCount;
        pLingInfo->pLingCmnInfo->Private.bStateLDBEnabled = 0;
    }
    if (pLingInfo->pLingCmnInfo->Private.bStateCDBEnabled && (dwDBMasks & ET9STATECDBENABLEDMASK)) {
        ++nChangeCount;
        pLingInfo->pLingCmnInfo->Private.bStateCDBEnabled = 0;
    }
    if (pLingInfo->pLingCmnInfo->Private.bStateRUDBEnabled && (dwDBMasks & ET9STATERUDBENABLEDMASK)) {
        ++nChangeCount;
        pLingInfo->pLingCmnInfo->Private.bStateRUDBEnabled = 0;
    }
    if (pLingInfo->pLingCmnInfo->Private.bStateASDBEnabled && (dwDBMasks & ET9STATEASDBENABLEDMASK)) {
        ++nChangeCount;
        pLingInfo->pLingCmnInfo->Private.bStateASDBEnabled = 0;
    }
    if (pLingInfo->pLingCmnInfo->Private.bStateMDBEnabled && (dwDBMasks & ET9STATEMDBENABLEDMASK)) {
        ++nChangeCount;
        pLingInfo->pLingCmnInfo->Private.bStateMDBEnabled = 0;
    }

    if (nChangeCount) {
        _ET9InvalidateSymbInfo(pLingInfo->pLingCmnInfo->Base.pWordSymbInfo);
    }

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/**
 * This function sets the primary language fence value
 *
 * @param pLingInfo          Pointer to alpha information structure.
 * @param bFence             New primary fence value.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL ET9AWSetPrimaryFence(ET9AWLingInfo * const pLingInfo,
                                          const ET9U8           bFence)
{
    ET9STATUS wStatus;

    wStatus = _ET9AWSys_BasicValidityCheck(pLingInfo);

    if (wStatus) {
        return wStatus;
    }

    wStatus = _ET9SettingsInhibited(pLingInfo->pLingCmnInfo->Base.pWordSymbInfo);

    if (wStatus) {
        return wStatus;
    }

    {
        if (pLingInfo->pLingCmnInfo->Private.bPrimaryFence != bFence) {

            pLingInfo->pLingCmnInfo->Private.bPrimaryFence = bFence;

            _ET9InvalidateSymbInfo(pLingInfo->pLingCmnInfo->Base.pWordSymbInfo);
        }
    }

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/**
 * This function sets the secondary language fence value
 *
 * @param pLingInfo          Pointer to alpha information structure.
 * @param bFence             New secondary fence value.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL ET9AWSetSecondaryFence(ET9AWLingInfo * const       pLingInfo,
                                            const ET9U8                 bFence)
{
    ET9STATUS wStatus;

    wStatus = _ET9AWSys_BasicValidityCheck(pLingInfo);

    if (wStatus) {
        return wStatus;
    }

    wStatus = _ET9SettingsInhibited(pLingInfo->pLingCmnInfo->Base.pWordSymbInfo);

    if (wStatus) {
        return wStatus;
    }

    {
        if (pLingInfo->pLingCmnInfo->Private.bSecondaryFence != bFence) {

            pLingInfo->pLingCmnInfo->Private.bSecondaryFence = bFence;

            _ET9InvalidateSymbInfo(pLingInfo->pLingCmnInfo->Base.pWordSymbInfo);
        }
    }

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/**
 * This function sets the active language switch feature
 *
 * @param pLingInfo          Pointer to alpha information structure.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL ET9AWSetActiveLanguageSwitch(ET9AWLingInfo * const pLingInfo)
{
    ET9STATUS wStatus;

    wStatus = _ET9AWSys_BasicValidityCheck(pLingInfo);

    if (wStatus) {
        return wStatus;
    }

    wStatus = _ET9SettingsInhibited(pLingInfo->pLingCmnInfo->Base.pWordSymbInfo);

    if (wStatus) {
        return wStatus;
    }

    if (!pLingInfo->pLingCmnInfo->Private.bStateActiveLangSwitch) {

        pLingInfo->pLingCmnInfo->Private.bStateActiveLangSwitch = 1;

        pLingInfo->pLingCmnInfo->Base.pWordSymbInfo->Private.bSwitchLanguage = 0;
        pLingInfo->pLingCmnInfo->Private.wPreviousWordLanguage = pLingInfo->pLingCmnInfo->wFirstLdbNum;

        _ET9InvalidateSymbInfo(pLingInfo->pLingCmnInfo->Base.pWordSymbInfo);
    }

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/**
 * This function clears the active language switch feature
 *
 * @param pLingInfo          Pointer to alpha information structure.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL ET9AWClearActiveLanguageSwitch(ET9AWLingInfo * const pLingInfo)
{
    ET9STATUS wStatus;

    wStatus = _ET9AWSys_BasicValidityCheck(pLingInfo);

    if (wStatus) {
        return wStatus;
    }

    wStatus = _ET9SettingsInhibited(pLingInfo->pLingCmnInfo->Base.pWordSymbInfo);

    if (wStatus) {
        return wStatus;
    }

    if (pLingInfo->pLingCmnInfo->Private.bStateActiveLangSwitch) {

        pLingInfo->pLingCmnInfo->Private.bStateActiveLangSwitch = 0;

        pLingInfo->pLingCmnInfo->Base.pWordSymbInfo->Private.bSwitchLanguage = 0;
        pLingInfo->pLingCmnInfo->Private.wPreviousWordLanguage = pLingInfo->pLingCmnInfo->wFirstLdbNum;

        _ET9InvalidateSymbInfo(pLingInfo->pLingCmnInfo->Base.pWordSymbInfo);
    }

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/**
 * This function sets the active LDBs.
 *
 * @param pLingInfo                 Pointer to alphabetic information structure.
 * @param wFirstLdbNum              First LDB number.
 * @param wSecondLdbNum             Second LDB number.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL ET9AWLdbSetLanguage(ET9AWLingInfo * const  pLingInfo,
                                         const ET9U16           wFirstLdbNum,
                                         const ET9U16           wSecondLdbNum)
{
    ET9STATUS wStatus = ET9STATUS_NONE;

    wStatus = _ET9AWSys_BasicValidityCheck(pLingInfo);

    if (wStatus) {
        return wStatus;
    }

#ifdef EVAL_BUILD
    _ET9Eval_StartTrackingUsage(&pLingInfo->pLingCmnInfo->Base);
#endif

    if ((wFirstLdbNum & ET9PLIDMASK) == ET9PLIDNone && (wSecondLdbNum & ET9PLIDMASK) != ET9PLIDNone) {
        return ET9STATUS_NO_LDB;
    }

    if (wSecondLdbNum && (wSecondLdbNum == wFirstLdbNum) && ((wFirstLdbNum & ET9PLIDMASK) != ET9PLIDNone)) {
        return ET9STATUS_SETTING_SAME_LDBS;
    }

    if (wSecondLdbNum &&
        ((wSecondLdbNum & ET9PLIDMASK) != ET9PLIDNone) &&
        ((wSecondLdbNum & ET9PLIDMASK) != ET9PLIDNull)) {

        wStatus = _ET9AWLdbSetActiveLanguage(pLingInfo, wSecondLdbNum);

        if (wStatus) {
            return wStatus;
        }

        pLingInfo->pLingCmnInfo->wSecondLdbNum = pLingInfo->pLingCmnInfo->wLdbNum;

        pLingInfo->pLingCmnInfo->Base.pWordSymbInfo->Private.bSwitchLanguage = 0;
    }
    else {
        pLingInfo->pLingCmnInfo->wSecondLdbNum = ET9PLIDNone;
    }

    wStatus = _ET9AWLdbSetActiveLanguage(pLingInfo, wFirstLdbNum);

    if (wStatus) {
        return wStatus;
    }

    pLingInfo->pLingCmnInfo->wFirstLdbNum = pLingInfo->pLingCmnInfo->wLdbNum;
    pLingInfo->pLingCmnInfo->Private.wPreviousWordLanguage = pLingInfo->pLingCmnInfo->wLdbNum;

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * This function gets the LDB number.
 * Intended for test purposes.
 *
 * @param pLingInfo                 Pointer to alphabetic information structure.
 * @param pwFirstLdbNum            (out) pointer to first LDB number.
 * @param pwSecondLdbNum           (out) pointer to second LDB number.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL ET9AWLdbGetLanguage(ET9AWLingInfo * const pLingInfo,
                                         ET9U16        * const pwFirstLdbNum,
                                         ET9U16        * const pwSecondLdbNum)
{
    ET9STATUS wStatus;

    wStatus = _ET9AWSys_BasicValidityCheck(pLingInfo);

    if (wStatus) {
        return wStatus;
    }

    if (pwFirstLdbNum == NULL) {
        return ET9STATUS_INVALID_MEMORY;
    }

    if (ET9AWSys_GetBilingualSupported(pLingInfo) && pwSecondLdbNum == NULL) {
        return ET9STATUS_INVALID_MEMORY;
    }

    *pwFirstLdbNum = pLingInfo->pLingCmnInfo->wFirstLdbNum;

    if (pwSecondLdbNum != NULL) {
        *pwSecondLdbNum = pLingInfo->pLingCmnInfo->wSecondLdbNum;
    }

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * This function gets the LDB ID of the currently active language.
 * Intended for test purposes.
 *
 * @param pLingInfo                 Pointer to alphabetic information structure.
 * @param pwLdbNum                  (out) pointer to LDB number.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL ET9AWLdbGetActiveLanguage(ET9AWLingInfo * const pLingInfo,
                                               ET9U16        * const pwLdbNum)
{
    ET9STATUS wStatus;

    wStatus = _ET9AWSys_BasicValidityCheck(pLingInfo);

    if (wStatus) {
        return wStatus;
    }

    if (pwLdbNum == NULL) {
        return ET9STATUS_INVALID_MEMORY;
    }

    *pwLdbNum = pLingInfo->pLingCmnInfo->wLdbNum;

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/**
 * This function enables inclusion of a downshifted version of an uppercase
 * default selection list entry
 *
 * @param pLingInfo          Pointer to alpha information structure.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL ET9SetDownshiftDefault(ET9AWLingInfo * const pLingInfo)
{
    ET9STATUS wStatus;

    wStatus = _ET9AWSys_BasicValidityCheck(pLingInfo);

    if (wStatus) {
        return wStatus;
    }

    wStatus = _ET9SettingsInhibited(pLingInfo->pLingCmnInfo->Base.pWordSymbInfo);

    if (wStatus) {
        return wStatus;
    }

    if (!pLingInfo->pLingCmnInfo->Private.bStateDownShiftDefault) {

        pLingInfo->pLingCmnInfo->Private.bStateDownShiftDefault = 1;

        _ET9InvalidateSymbInfo(pLingInfo->pLingCmnInfo->Base.pWordSymbInfo);
    }

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/**
 * This function disables inclusion of a downshifted version of an uppercase
 * default selection list entry
 *
 * @param pLingInfo          Pointer to alpha information structure.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL ET9ClearDownshiftDefault(ET9AWLingInfo * const pLingInfo)
{
    ET9STATUS wStatus;

    wStatus = _ET9AWSys_BasicValidityCheck(pLingInfo);

    if (wStatus) {
        return wStatus;
    }

    wStatus = _ET9SettingsInhibited(pLingInfo->pLingCmnInfo->Base.pWordSymbInfo);

    if (wStatus) {
        return wStatus;
    }

    if (pLingInfo->pLingCmnInfo->Private.bStateDownShiftDefault) {

        pLingInfo->pLingCmnInfo->Private.bStateDownShiftDefault = 0;

        _ET9InvalidateSymbInfo(pLingInfo->pLingCmnInfo->Base.pWordSymbInfo);
    }

    return ET9STATUS_NONE;
}


/*---------------------------------------------------------------------------*/
/**
 * This function enables downshifting of all LDB words pre-sellist entry
 *
 * @param pLingInfo          Pointer to alpha information structure.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL ET9SetDownshiftAllLDBWords(ET9AWLingInfo * const pLingInfo)
{
    ET9STATUS wStatus;

    wStatus = _ET9AWSys_BasicValidityCheck(pLingInfo);

    if (wStatus) {
        return wStatus;
    }

    wStatus = _ET9SettingsInhibited(pLingInfo->pLingCmnInfo->Base.pWordSymbInfo);

    if (wStatus) {
        return wStatus;
    }

    if (!pLingInfo->pLingCmnInfo->Private.bStateDownShiftAllLDB) {

        pLingInfo->pLingCmnInfo->Private.bStateDownShiftAllLDB = 1;

        _ET9InvalidateSymbInfo(pLingInfo->pLingCmnInfo->Base.pWordSymbInfo);
    }

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/**
 * This function disables downshifting of LDB words
 *
 * @param pLingInfo          Pointer to alpha information structure.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL ET9ClearDownshiftAllLDBWords(ET9AWLingInfo * const pLingInfo)
{
    ET9STATUS wStatus;

    wStatus = _ET9AWSys_BasicValidityCheck(pLingInfo);

    if (wStatus) {
        return wStatus;
    }

    wStatus = _ET9SettingsInhibited(pLingInfo->pLingCmnInfo->Base.pWordSymbInfo);

    if (wStatus) {
        return wStatus;
    }

    if (pLingInfo->pLingCmnInfo->Private.bStateDownShiftAllLDB) {

        pLingInfo->pLingCmnInfo->Private.bStateDownShiftAllLDB = 0;

        _ET9InvalidateSymbInfo(pLingInfo->pLingCmnInfo->Base.pWordSymbInfo);
    }

    return ET9STATUS_NONE;
}

#if 0
/*---------------------------------------------------------------------------*/
/**
 * This function get next character list based on current char.
 *
 * @param pLingInfo          Pointer to alpha information structure.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL ET9AWGetNextSymbolsPrediction(ET9AWLingInfo     *pLingInfo,
                                                   ET9AWWordSymbInfo *pWordSymbInfo,
                                                   ET9SYMB           *psPredictSyms,
                                                   ET9U8              bMaxSyms,
                                                   ET9U8             *pbTotalPredictSymbols,
                                                   ET9U8              bCurrIndexInList,
                                                   ET9SYMB            sExplicitSym)
{
    ET9STATUS wStatus;
    ET9U8 i;

    if ((wStatus = _ET9AWSys_BasicValidityCheck(pLingInfo)) != ET9STATUS_NONE) {
        return wStatus;
    }

    if (psPredictSyms == NULL || pbTotalPredictSymbols == NULL || pWordSymbInfo == NULL) {
        return ET9STATUS_INVALID_MEMORY;
    }

    if (bMaxSyms < ET9PREDICT_SYMS) {
        return ET9STATUS_NO_MEMORY;
    }

    if (bCurrIndexInList > pLingInfo->Private.bTotalPredictSymbols) {
        bCurrIndexInList = 0;
    }

    if (bCurrIndexInList >= ET9PREDICT_SYMS) {
        return ET9STATUS_OUT_OF_RANGE;
    }

    if (sExplicitSym) {
        wStatus = ET9AWAddExplicitSymb(
            pWordSymbInfo, sExplicitSym, ET9NOSHIFT, 0);
        if (wStatus != ET9STATUS_NONE) {
            return wStatus;
        }
    }
    else if (pLingInfo->Private.sPredictSyms[bCurrIndexInList]) {
        wStatus = ET9AWAddExplicitSymb(
            pWordSymbInfo,
            pLingInfo->Private.sPredictSyms[bCurrIndexInList],
            ET9NOSHIFT, 0);
        if (wStatus != ET9STATUS_NONE) {
            return wStatus;
        }
    }

    pLingInfo->Private.bTotalPredictSymbols = _ET9AWLdbGetNextSymbolsPrediction(
        pLingInfo, pWordSymbInfo);

    if (! pLingInfo->Private.bTotalPredictSymbols) {
        wStatus = ET9STATUS_NO_MATCHING_WORDS;
    }
    else {
        wStatus = ET9STATUS_NONE;
    }

    *pbTotalPredictSymbols = pLingInfo->Private.bTotalPredictSymbols;

    for (i = 0; i < pLingInfo->Private.bTotalPredictSymbols; ++i) {
        psPredictSyms[i] = pLingInfo->Private.sPredictSyms[i];
    }

    return wStatus;
}
#endif


#endif /* ET9_ALPHABETIC_MODULE */
/*! @} */
/* ----------------------------------< eof >--------------------------------- */
