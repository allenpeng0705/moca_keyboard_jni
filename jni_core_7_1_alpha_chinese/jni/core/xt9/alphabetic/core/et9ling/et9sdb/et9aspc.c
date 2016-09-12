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
;**     FileName: et9aspc.h                                                   **
;**                                                                           **
;**  Description: Alphabetic spell correction routines.                       **
;**                                                                           **
;*******************************************************************************
;******************************************************************************/

/*! \internal \addtogroup et9aspc
* @{
*/

#include "et9api.h"
#ifdef ET9_ALPHABETIC_MODULE
#include "et9sym.h"
#include "et9asym.h"
#include "et9aspc.h"
#include "et9asys.h"
#include "et9misc.h"


#ifdef ET9_SPC_LOG_HELP_FUNC
#include <stdio.h>
FILE *pLogFile = NULL;
FILE *pLogFile2 = NULL;
#endif /* ET9_SPC_LOG_HELP_FUNC */


#ifdef ET9_ACTIVATE_SLST_STATS

#ifdef _WIN32
#pragma message ("*** SLST stats active ***")
#endif

ET9S64 ET9AWSPCCmpOther = 0;
ET9S64 ET9AWSPCCmpLdbOnly = 0;
ET9S64 ET9AWSPCCmpLdbInfo = 0;

ET9S64 ET9AWSPCCmpOtherAcc = 0;
ET9S64 ET9AWSPCCmpLdbOnlyAcc = 0;
ET9S64 ET9AWSPCCmpLdbInfoAcc = 0;

ET9S64 ET9AWSPCTotLdbCount = 0;
ET9S64 ET9AWSPCTotCalcCount = 0;
ET9S64 ET9AWSPCClassicMatch = 0;
ET9S64 ET9AWSPCNoStemMatch = 0;
ET9S64 ET9AWSPCPossibleSpcCandidate = 0;
ET9S64 ET9AWSPCWithinCandidate = 0;
ET9S64 ET9AWSPCEdCalcCandidate = 0;
ET9S64 ET9AWSPCActualCandidate = 0;

ET9S64 ET9AWSPCTotLdbCountAcc = 0;
ET9S64 ET9AWSPCTotCalcCountAcc = 0;
ET9S64 ET9AWSPCClassicMatchAcc = 0;
ET9S64 ET9AWSPCNoStemMatchAcc = 0;
ET9S64 ET9AWSPCPossibleSpcCandidateAcc = 0;
ET9S64 ET9AWSPCWithinCandidateAcc = 0;
ET9S64 ET9AWSPCEdCalcCandidateAcc = 0;
ET9S64 ET9AWSPCActualCandidateAcc = 0;

/*---------------------------------------------------------------------------*/
/** \internal
 * Print spell correction statistics to file.
 *
 * @param pf                        File to print to.
 * @param pLingInfo                 Pointer to alphabetic information structure.
 *
 * @return None
 */

void _ET9AWSpcPrintStats(FILE *pf, ET9AWLingInfo *pLingInfo)
{
    ET9AWLingCmnInfo * const pLingCmnInfo = pLingInfo->pLingCmnInfo;

    fprintf(pf, "\nCompare stats: ldb only %I64d (%I64d), ldb info %I64d (%I64d), other %I64d (%I64d)\n\n",
        ET9AWSPCCmpLdbOnly,
        ET9AWSPCCmpLdbOnlyAcc,
        ET9AWSPCCmpLdbInfo,
        ET9AWSPCCmpLdbInfoAcc,
        ET9AWSPCCmpOther,
        ET9AWSPCCmpOtherAcc);

    if (ET9AWSPCTotCalcCount) {
        fprintf(pf, "SPC stats (one): tot count %I64d (%I64d), classic match %I64d, no stem match %I64d, possible spc %I64d, within %I64d, ed calc %I64d, actual %I64d\n\n",
            ET9AWSPCTotCalcCount,
            ET9AWSPCTotLdbCount,
            ET9AWSPCClassicMatch,
            ET9AWSPCNoStemMatch,
            ET9AWSPCPossibleSpcCandidate,
            ET9AWSPCWithinCandidate,
            ET9AWSPCEdCalcCandidate,
            ET9AWSPCActualCandidate);
    }

    fprintf(pf, "SPC stats (acc): tot count %I64d (%I64d), classic match %I64d, no stem match %I64d, possible spc %I64d, within %I64d, ed calc %I64d, actual %I64d\n\n",
        ET9AWSPCTotCalcCountAcc,
        ET9AWSPCTotLdbCountAcc,
        ET9AWSPCClassicMatchAcc,
        ET9AWSPCNoStemMatchAcc,
        ET9AWSPCPossibleSpcCandidateAcc,
        ET9AWSPCWithinCandidateAcc,
        ET9AWSPCEdCalcCandidateAcc,
        ET9AWSPCActualCandidateAcc);

    if (pLingCmnInfo->Private.sStats.dwTotInsertCount) {
        fprintf(pf, "List stats (one): builds %d, ins %d (%1.1f), max %d, duplicate %d (%1.1f%%), discard %d (%1.1f%%), replace %d (%1.1f%%)\n\n",
            pLingCmnInfo->Private.sStats.dwBuildCount,
            pLingCmnInfo->Private.sStats.dwTotInsertCount,
            (((float)pLingCmnInfo->Private.sStats.dwTotInsertCount) / ((float)pLingCmnInfo->Private.sStats.dwBuildCount)),
            pLingCmnInfo->Private.sStats.dwMaxListInserts,
            pLingCmnInfo->Private.sStats.dwTotInsertDuplicate,
            (((float)pLingCmnInfo->Private.sStats.dwTotInsertDuplicate) / ((float)pLingCmnInfo->Private.sStats.dwTotInsertCount) * 100),
            pLingCmnInfo->Private.sStats.dwTotInsertDiscarded,
            (((float)pLingCmnInfo->Private.sStats.dwTotInsertDiscarded) / ((float)pLingCmnInfo->Private.sStats.dwTotInsertCount) * 100),
            pLingCmnInfo->Private.sStats.dwTotInsertReplacing,
            (((float)pLingCmnInfo->Private.sStats.dwTotInsertReplacing) / ((float)pLingCmnInfo->Private.sStats.dwTotInsertCount)) * 100);
    }

    fprintf(pf, "List stats (acc): builds %d, ins %d (%1.1f), max %d, duplicate %d (%1.1f%%), discard %d (%1.1f%%), replace %d (%1.1f%%)\n\n",
        pLingCmnInfo->Private.sStats.dwBuildCountAcc,
        pLingCmnInfo->Private.sStats.dwTotInsertCountAcc,
        (((float)pLingCmnInfo->Private.sStats.dwTotInsertCountAcc) / ((float)pLingCmnInfo->Private.sStats.dwBuildCountAcc)),
        pLingCmnInfo->Private.sStats.dwMaxListInsertsAcc,
        pLingCmnInfo->Private.sStats.dwTotInsertDuplicateAcc,
        (((float)pLingCmnInfo->Private.sStats.dwTotInsertDuplicateAcc) / ((float)pLingCmnInfo->Private.sStats.dwTotInsertCountAcc) * 100),
        pLingCmnInfo->Private.sStats.dwTotInsertDiscardedAcc,
        (((float)pLingCmnInfo->Private.sStats.dwTotInsertDiscardedAcc) / ((float)pLingCmnInfo->Private.sStats.dwTotInsertCountAcc) * 100),
        pLingCmnInfo->Private.sStats.dwTotInsertReplacingAcc,
        (((float)pLingCmnInfo->Private.sStats.dwTotInsertReplacingAcc) / ((float)pLingCmnInfo->Private.sStats.dwTotInsertCountAcc)) * 100);
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Clear spell correction statistics (reset counters for a new run).
 *
 * @param pLingInfo                 Pointer to alphabetic information structure.
 *
 * @return None
 */

void _ET9AWSpcClearStats(ET9AWLingInfo *pLingInfo)
{
    ET9AWLingCmnInfo * const pLingCmnInfo = pLingInfo->pLingCmnInfo;

    ET9AWSPCCmpOtherAcc     += ET9AWSPCCmpOther;
    ET9AWSPCCmpLdbOnlyAcc   += ET9AWSPCCmpLdbOnly;
    ET9AWSPCCmpLdbInfoAcc   += ET9AWSPCCmpLdbInfo;

    ET9AWSPCCmpOther = 0;
    ET9AWSPCCmpLdbInfo = 0;
    ET9AWSPCCmpLdbOnly = 0;

    ET9AWSPCTotLdbCountAcc          += ET9AWSPCTotLdbCount;
    ET9AWSPCTotCalcCountAcc         += ET9AWSPCTotCalcCount;
    ET9AWSPCClassicMatchAcc         += ET9AWSPCClassicMatch;
    ET9AWSPCNoStemMatchAcc          += ET9AWSPCNoStemMatch;
    ET9AWSPCPossibleSpcCandidateAcc += ET9AWSPCPossibleSpcCandidate;
    ET9AWSPCWithinCandidateAcc      += ET9AWSPCWithinCandidate;
    ET9AWSPCEdCalcCandidateAcc      += ET9AWSPCEdCalcCandidate;
    ET9AWSPCActualCandidateAcc      += ET9AWSPCActualCandidate;

    ET9AWSPCTotLdbCount = 0;
    ET9AWSPCTotCalcCount = 0;
    ET9AWSPCClassicMatch = 0;
    ET9AWSPCNoStemMatch = 0;
    ET9AWSPCPossibleSpcCandidate = 0;
    ET9AWSPCWithinCandidate = 0;
    ET9AWSPCEdCalcCandidate = 0;
    ET9AWSPCActualCandidate = 0;

    pLingCmnInfo->Private.sStats.dwBuildCountAcc            += pLingCmnInfo->Private.sStats.dwBuildCount;
    pLingCmnInfo->Private.sStats.dwInsertCountAcc           += pLingCmnInfo->Private.sStats.dwInsertCount;
    pLingCmnInfo->Private.sStats.dwTotInsertCountAcc        += pLingCmnInfo->Private.sStats.dwTotInsertCount;
    pLingCmnInfo->Private.sStats.dwTotInsertDiscardedAcc    += pLingCmnInfo->Private.sStats.dwTotInsertDiscarded;
    pLingCmnInfo->Private.sStats.dwTotInsertReplacingAcc    += pLingCmnInfo->Private.sStats.dwTotInsertReplacing;
    pLingCmnInfo->Private.sStats.dwTotInsertDuplicateAcc    += pLingCmnInfo->Private.sStats.dwTotInsertDuplicate;

    if (pLingCmnInfo->Private.sStats.dwMaxListInsertsAcc < pLingCmnInfo->Private.sStats.dwMaxListInserts) {
        pLingCmnInfo->Private.sStats.dwMaxListInsertsAcc = pLingCmnInfo->Private.sStats.dwMaxListInserts;
    }

    pLingCmnInfo->Private.sStats.dwBuildCount = 0;
    pLingCmnInfo->Private.sStats.dwInsertCount = 0;
    pLingCmnInfo->Private.sStats.dwTotInsertCount = 0;
    pLingCmnInfo->Private.sStats.dwTotInsertDiscarded = 0;
    pLingCmnInfo->Private.sStats.dwTotInsertReplacing = 0;
    pLingCmnInfo->Private.sStats.dwTotInsertDuplicate = 0;
    pLingCmnInfo->Private.sStats.dwMaxListInserts = 0;
}

#endif /* ET9_ACTIVATE_SLST_STATS */

#ifdef ET9_DEBUG_FLEXVER

/*---------------------------------------------------------------------------*/
/** \internal
 * Calculate a checksum for a memory area.
 *
 * @param pMemory                   Pointer to the memory.
 * @param dwSize                    Size of the memory area.
 *
 * @return Max edit distance.
 */

static ET9U32 ET9LOCALCALL __CalcChecksum(void * const pMemory, const ET9U32 dwSize)
{
    ET9U8               *pbByte;
    ET9U32              dwCount;
    ET9U32              dwHashValue;

    ET9Assert(pMemory);

    dwHashValue = 0;

    pbByte = (ET9U8*)pMemory;
    for (dwCount = dwSize; dwCount; --dwCount, ++pbByte) {
        dwHashValue = *pbByte + (65599 * dwHashValue);
    }

    return dwHashValue;
}

#endif

#ifdef ET9_DEBUG_FLEXVER

/*---------------------------------------------------------------------------*/
/** \internal
 * Handle flex area modification.
 *
 * @param pLingCmnInfo              Pointer to alphabetic cmn information structure.
 *
 * @return None
 */

void ET9FARCALL _ET9AWValidateFlexArea(ET9AWLingCmnInfo * const pLingCmnInfo)
{
    ET9ASPCFlexCompareData  * const pCMP = &pLingCmnInfo->Private.ASpc.u.sCmpDataFlex;

    ET9Assert(pLingCmnInfo->Private.ASpc.dwCompareChecksum == __CalcChecksum(pCMP, sizeof(ET9ASPCFlexCompareData)));
}

#endif

#ifdef ET9_DEBUG_FLEXVER

/*---------------------------------------------------------------------------*/
/** \internal
 * Handle flex area modification.
 *
 * @param pLingCmnInfo              Pointer to alphabetic cmn information structure.
 *
 * @return None
 */

void ET9FARCALL _ET9AWModifiedFlexArea(ET9AWLingCmnInfo * const pLingCmnInfo)
{
    ET9ASPCFlexCompareData  * const pCMP = &pLingCmnInfo->Private.ASpc.u.sCmpDataFlex;

    pLingCmnInfo->Private.ASpc.dwCompareChecksum = __CalcChecksum(pCMP, sizeof(ET9ASPCFlexCompareData));
}

#endif

/*---------------------------------------------------------------------------*/
/** \internal
 * Given an input length, calculate the allowed edit distance.
 *
 * @param bSpcAllowed               Spell correction mode.
 * @param nLength                   Input length (symbols).
 *
 * @return Max edit distance.
 */

ET9INLINE static ET9UINT ET9LOCALCALL __CalcMaxEditDistance(const ET9BOOL bSpcAllowed, const ET9UINT nLength)
{
    if (!bSpcAllowed) {
        return 0;
    }

    return (nLength >= 3 * ET9_SPC_ED_MAX_DISTANCE) ? ET9_SPC_ED_MAX_DISTANCE : (nLength / 3);
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Given an input length and spell correction mode, calculate the minimum allowed word length.
 *
 * @param bSpcAllowed               Spell correction mode.
 * @param nLength                   Input length (symbols).
 *
 * @return Minimum word length.
 */

ET9INLINE static ET9UINT ET9LOCALCALL __CalcMinSourceLength(const ET9BOOL bSpcAllowed, const ET9UINT nLength)
{
    const ET9UINT nMaxDist = __CalcMaxEditDistance(bSpcAllowed, nLength);

    if (nLength < nMaxDist) {
        return 0;
    }
    else {
        return nLength - nMaxDist;
    }
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Given an input length and spell correction mode, calculate the maximum allowed word length.
 *
 * @param pLingCmnInfo              Pointer to alphabetic cmn information structure.
 * @param bSpcAllowed               Spell correction mode.
 * @param nLength                   Input length (symbols).
 *
 * @return Maximum word length.
 */

ET9INLINE static ET9UINT ET9LOCALCALL __CalcMaxSourceLength(ET9AWLingCmnInfo * const pLingCmnInfo, const ET9BOOL bSpcAllowed, const ET9UINT nLength)
{
    if (ET9_FLEX_FEATURE_SPC_COMPL_MODE(pLingCmnInfo->Private.ASpc.bSpcFeatures)) {
        return ET9MAXWORDSIZE;
    }

    {
        const ET9UINT nMaxDist = __CalcMaxEditDistance(bSpcAllowed, nLength);

        return nLength + nMaxDist;
    }
}

/*---------------------------------------------------------------------------*/
/** \internal
 * This function inits values for the edit distance calculation.
 *
 * @param pLingInfo                 Pointer to alphabetic information structure.
 * @param wIndex                    Input index/position (non zero if partial).
 * @param wLength                   Word length (active input length).
 * @param bSpcMode                  Spell correction mode.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL _ET9AWCalcEditDistanceInit(ET9AWLingInfo     * const pLingInfo,
                                                const ET9U16              wIndex,
                                                const ET9U16              wLength,
                                                const ET9U8               bSpcMode)
{
    ET9AWLingCmnInfo        * const pLingCmnInfo    = pLingInfo->pLingCmnInfo;
    ET9WordSymbInfo         * const pWordSymbInfo   = pLingCmnInfo->Base.pWordSymbInfo;
    ET9SymbInfo             * const pFirstSymb      = &pWordSymbInfo->SymbsInfo[wIndex];
    ET9ASPCFlexCompareData  * const pCMP            = &pLingCmnInfo->Private.ASpc.u.sCmpDataFlex;

    ET9U8                   * const pbLockInfo      = pCMP->pbLockInfo;
    ET9U8                   * const pbIsFreqPos     = pCMP->pbIsFreqPos;
    ET9U8                   * const pbIsQualityKey  = pCMP->pbIsQualityKey;

    const ET9UINT nKeyCount = wLength;
    const ET9UINT nSymbCount = ET9MAXWORDSIZE;

    ET9UINT nR;
    ET9UINT nC;

    ET9UINT nQualityCount;

    /* validate */

    ET9Assert(pLingCmnInfo->Private.ASpc.bSpcState == ET9_SPC_STATE_NONE);

    /* pre-calculate some common values */

    /* max regionality */

    if (_ET9_LanguageSpecific_ApplyDynamicRegionality(pLingInfo)) {

        if (pLingCmnInfo->Private.bTraceBuild) {
            pLingCmnInfo->Private.bCurrMaxRegionality = 0xFF;
        }
        else if (wLength > 5) {
            pLingCmnInfo->Private.bCurrMaxRegionality = 0xFF;
        }
        else {
            pLingCmnInfo->Private.bCurrMaxRegionality = 4;
        }
    }
    else {
        pLingCmnInfo->Private.bCurrMaxRegionality = 0xFF;
    }

    /* just std init? */

    if (!pLingCmnInfo->Private.ASpc.bSpcFeatures) {

        const ET9U8 bCurrSpcMode = __CalcMaxEditDistance(1, wLength) ? bSpcMode : ET9ASPCMODE_OFF;
        const ET9BOOL bUsingSpc = bCurrSpcMode != ET9ASPCMODE_OFF;

        pLingCmnInfo->Private.bCurrSpcMode = bCurrSpcMode;
        pLingCmnInfo->Private.bCurrMaxEditDistance = (ET9U8)__CalcMaxEditDistance(bUsingSpc, wLength);
        pLingCmnInfo->Private.wCurrMinSourceLength = (ET9U16)__CalcMinSourceLength(bUsingSpc, wLength);
        pLingCmnInfo->Private.wCurrMaxSourceLength = (ET9U16)__CalcMaxSourceLength(pLingCmnInfo, bUsingSpc, wLength);

        pLingCmnInfo->Private.ASpc.bSpcState = ET9_SPC_STATE_STD_INIT_OK;

        return ET9STATUS_NONE;
    }

    pLingCmnInfo->Private.ASpc.bSpcState = ET9_SPC_STATE_FLEX_INIT_OK;

    /* locked symbs */

    {
        ET9U8 bLocked = 0;

        ET9SYMB * const psLockSymb      = pCMP->psLockSymb;
        ET9SYMB * const psLockSymbOther = pCMP->psLockSymbOther;

        for (nR = nKeyCount; nR; --nR) {

            const ET9UINT nIndex = nR - 1;

            bLocked |= pFirstSymb[nIndex].bLocked;

            if (bLocked) {
                psLockSymb[nIndex] = pFirstSymb[nIndex].sLockedSymb;
                psLockSymbOther[nIndex] = _ET9SymToOther(psLockSymb[nIndex], pLingCmnInfo->wLdbNum);
            }
            else {
                psLockSymb[nIndex] = 0;
                psLockSymbOther[nIndex] = 0;
            }
        }
    }

    /* initial quality count */

    nQualityCount = 0;

    for (nR = 1; nR <= nKeyCount; ++nR) {

        const ET9U8 bQuality = _ET9_Flex_KeyQuality(pFirstSymb, nR - 1);

        if (bQuality) {
            ++nQualityCount;
        }
    }

    {
        const ET9U8 bCurrSpcMode = __CalcMaxEditDistance(1, nQualityCount) ? bSpcMode : ET9ASPCMODE_OFF;
        const ET9BOOL bUsingSpc = bCurrSpcMode != ET9ASPCMODE_OFF;

        pLingCmnInfo->Private.bCurrSpcMode = bCurrSpcMode;
        pLingCmnInfo->Private.bCurrMaxEditDistance = (ET9U8)__CalcMaxEditDistance(bUsingSpc, nQualityCount);
        pLingCmnInfo->Private.wCurrMinSourceLength = (ET9U16)__CalcMinSourceLength(bUsingSpc, nQualityCount);
        pLingCmnInfo->Private.wCurrMaxSourceLength = (ET9U16)__CalcMaxSourceLength(pLingCmnInfo, bUsingSpc, nQualityCount);

        /* init top */

        pCMP->ppbFreeDist[0][0] = 0;
        pCMP->ppbEditDist[0][0] = 0;
        pCMP->ppbStemDist[0][0] = 0;
        pCMP->ppxStemFreq[0][0] = 1;

        /* init rows */

        nQualityCount = 0;  /* re-count */

        for (nR = 1; nR <= nKeyCount; ++nR) {

            const ET9U8 bQuality = _ET9_Flex_KeyQuality(pFirstSymb, nR - 1);

            if (bQuality) {
                ++nQualityCount;
            }

            pbIsQualityKey[nR] = bQuality;

            pbLockInfo[nR] = _ET9_Flex_LockInfo(pFirstSymb, pCMP->psLockSymb, nR - 1);

            pbIsFreqPos[nR] = (bQuality && nQualityCount <= ET9_SPC_ED_MAX_FREQ_LEN) ? 1 : 0;

            pCMP->pbSubstFreqSC[nR][0] = 0;

            pCMP->ppbEditDist[nR][0] = pCMP->ppbEditDist[nR - 1][0] + (bQuality ? 1 : 0);

            if (bUsingSpc) {
                pCMP->ppbStemDist[nR][0] = pCMP->ppbStemDist[nR - 1][0] + (bQuality ? 1 : 0);
            }
            else {
                pCMP->ppbStemDist[nR][0] = (nQualityCount ? _ET9_FLEX_TUBE_MAX_STEM_DIST : 0);
            }

            pCMP->ppbFreeDist[nR][0] = pCMP->ppbEditDist[nR][0];
            pCMP->ppxStemFreq[nR][0] = 1;
        }

        /* init columns */

        for (nC = 1; nC <= nSymbCount; ++nC) {

            pCMP->pwPrevWordSC[nC] = ALDB_CHAR_CODE_NONE;

            pCMP->ppbEditDist[0][nC] = _ET9_FLEX_TUBE_MAX_EDIT_DIST;
            pCMP->ppbStemDist[0][nC] = _ET9_FLEX_TUBE_MAX_STEM_DIST;
            pCMP->ppbFreeDist[0][nC] = _ET9_FLEX_TUBE_MAX_FREE_DIST;
            pCMP->ppxStemFreq[0][nC] = 1;
        }

        /* allow's */

        pCMP->bAllowSpcCmpl = (ET9BOOL)(ET9_FLEX_FEATURE_SPC_COMPL_MODE(pLingCmnInfo->Private.ASpc.bSpcFeatures) ? 1 : 0);
        pCMP->bAllowFreePunct = (ET9BOOL)(ET9_FLEX_FEATURE_FREE_PUNCT_MODE(pLingCmnInfo->Private.ASpc.bSpcFeatures) ? 1 : 0);
        pCMP->bAllowFreeDouble = (ET9BOOL)(ET9_FLEX_FEATURE_FREE_DOUBLE_MODE(pLingCmnInfo->Private.ASpc.bSpcFeatures) ? 1 : 0);

        /* persist */

        pCMP->pFirstSymb = pFirstSymb;
        pCMP->nQualityCount = nQualityCount;
    }

    _ET9AWModifiedFlexArea(pLingCmnInfo);

    /* done */

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * This function finalizes working data for the edit distance calculation.
 *
 * @param pLingInfo                 Pointer to alphabetic information structure.
 *
 * @return None
 */

void ET9FARCALL _ET9AWCalcEditDistanceDone(ET9AWLingInfo     * const pLingInfo)
{
    ET9AWLingCmnInfo        * const pLingCmnInfo    = pLingInfo->pLingCmnInfo;
    ET9ASPCFlexCompareData  * const pCMP            = &pLingCmnInfo->Private.ASpc.u.sCmpDataFlex;

    ET9Assert(pLingCmnInfo->Private.ASpc.bSpcState == ET9_SPC_STATE_STD_INIT_OK ||
              pLingCmnInfo->Private.ASpc.bSpcState == ET9_SPC_STATE_FLEX_INIT_OK);

    if (pLingCmnInfo->Private.ASpc.bSpcState == ET9_SPC_STATE_FLEX_INIT_OK) {
        _ET9AWValidateFlexArea(pLingCmnInfo);
    }

    pLingCmnInfo->Private.ASpc.bSpcState = ET9_SPC_STATE_NONE;

    pCMP->pFirstSymb = NULL;
    pCMP->nQualityCount = 0;
}


#endif /* ET9_ALPHABETIC_MODULE */
/*! @} */
/* ----------------------------------< eof >--------------------------------- */
