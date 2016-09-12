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

#ifndef ET9ASPC_H
#define ET9ASPC_H    1

/*! \internal \addtogroup et9aspc Spell correction for alphabetic
* Spell Correction for alphabetic XT9.
* @{
*/

#include "et9api.h"


#ifdef ET9_USE_FLOAT_FREQS

#define ET9_SPC_ED_DEFOP_FREQ           5                                   /**< \internal Default tap frequency (if it's needed but missing). */
#define ET9_SPC_ED_MAX_FREQ_LEN         24                                  /**< \internal Maximum input length to calculate word frequency for. */

#else

#define ET9_SPC_ED_DEFOP_FREQ           85                                  /**< \internal Default tap frequency (if it's needed but missing). */
#define ET9_SPC_ED_MAX_FREQ_LEN         8                                   /**< \internal Maximum input length to calculate word frequency for. */

#if ET9_SPC_ED_DEFOP_FREQ < 17
#error ET9_SPC_ED_DEFOP_FREQ must have value 17 or larger
#endif

#endif


#define ALDB_CHAR_CODE_NONE     0xFFFF  /**< \internal code value for "none" (anything that isn't actually decoded...) */

/** \internal
 * Symbol lock values.
 * This kind of lock is not related to "explicit" locks in input, rather locks from a spell correction point of view.
 */

enum ET9ASYMBLOCKS_e {

    ET9_LOCKED_SYMB_NONE = 0,           /**< \internal the symbol is not locked  (guaranteed to be zero) */
    ET9_LOCKED_SYMB_VALUE,              /**< \internal the value is locked, but it can move */
    ET9_LOCKED_SYMB_VALUE_AND_POS       /**< \internal both symbol and position is locked */
};

/** \internal
 * Determine if an input symbol should be locked in any way.
 *
 * @param pSymbInfo                 Pointer to an input symbol.
 *
 * @return Symbol lock info.
 */

#define ET9_SPC_IS_LOCKED_POS(pSymbInfo)                                                                \
(                                                                                                       \
    (ET9U8)(                                                                                            \
        ((pSymbInfo)->eInputType == ET9EXPLICITSYM ||                                                   \
         (pSymbInfo)->eInputType == ET9MULTISYMBEXPLICIT ||                                             \
         (pSymbInfo)->bAmbigType == ET9EXACT && (pSymbInfo)->eInputType != ET9MULTITAPKEY ||            \
         (pSymbInfo)->bSymbType  == ET9KTSMARTPUNCT ||                                                  \
         (pSymbInfo)->bSymbType  == ET9KTPUNCTUATION) ?                                                 \
            ET9_LOCKED_SYMB_VALUE                                                                       \
            :                                                                                           \
            ET9_LOCKED_SYMB_NONE                                                                        \
    )                                                                                                   \
)                                                                                                       \

#define ET9_SPC_ED_MATCH_NONE       0               /**< \internal no match */
#define ET9_SPC_ED_MATCH_FULL       1               /**< \internal match, but not with the exact */
#define ET9_SPC_ED_MATCH_EXACT      2               /**< \internal match, with the exact */
#define ET9_SPC_ED_MATCH_UNKNOWN    0xFF            /**< \internal match info is unknown */

/** \internal
 * Determine if a match result is match given mode an value.
 *
 * @param bExactMatchOnly           If the match has to be exact or not.
 * @param bResult                   A match result.
 *
 * @return Non zero if match, otherwise zero.
 */

#define ET9_SPC_ED_IS_MATCH(bExactMatchOnly, bResult)                                                   \
(                                                                                                       \
    bResult && (!bExactMatchOnly || bResult == ET9_SPC_ED_MATCH_EXACT)                                  \
)                                                                                                       \

/** \internal
 * Turn on completion search (currently unused).
 *
 * @param bMode                     Current mode.
 *
 * @return Updated mode.
 */

#define ET9_SPC_SET_COMPLETION_SEARCH(bMode)    (bMode | 0x4)

/** \internal
 * Get the "mode" part only, from a spell correction mode value.
 *
 * @param bMode                     Current mode.
 *
 * @return Mode
 */

#define ET9_SPC_GET_MODE(bMode)                 (bMode & 0x3)

/** \internal
 * Check if spell correction is active, given the mode value.
 *
 * @param bMode                     Current mode.
 *
 * @return Non zero if on, otherwise zero.
 */

#define ET9_SPC_IS_ACTIVE(bMode)                (bMode & 0x3)

/** \internal
 * Check if spell correction filter mode is "unfiltered".
 *
 * @param bFilter                   Current filter.
 *
 * @return Non zero if unfiltered, otherwise zero.
 */

#define ET9_SPC_SEARCH_FILTER_IS_UNFILTERED(bFilter)    ((ET9BOOL)!bFilter)

/** \internal
 * Check if spell correction filter mode is "exact".
 *
 * @param bFilter                   Current filter.
 *
 * @return Non zero if exact, otherwise zero.
 */

#define ET9_SPC_SEARCH_FILTER_IS_EXACT(bFilter)         ((ET9BOOL)(bFilter & 0x1))

/** \internal
 * Check if spell correction filter mode is "ONE".
 *
 * @param bFilter                   Current filter.
 *
 * @return Non zero if ONE, otherwise zero.
 */

#define ET9_SPC_SEARCH_FILTER_IS_ONE(bFilter)           ((ET9BOOL)(bFilter & 0x2))

/** \internal
 * Check if spell correction filter mode is "TWO".
 *
 * @param bFilter                   Current filter.
 *
 * @return Non zero if TWO, otherwise zero.
 */

#define ET9_SPC_SEARCH_FILTER_IS_TWO(bFilter)           ((ET9BOOL)(bFilter & 0x4))

/* ----------------------------------------- FLEX ------------------------------------------ */

/*---------------------------------------------------------------------------*/

#define _ET9_FLEX_TUBE_MAX_FREE_DIST            10      /**< \internal xxx */
#define _ET9_FLEX_TUBE_MAX_EDIT_DIST            10      /**< \internal xxx */
#define _ET9_FLEX_TUBE_MAX_STEM_DIST            10      /**< \internal xxx */

/*---------------------------------------------------------------------------*/
/** \internal
 */

enum ET9_SPC_STATE_e {

    ET9_SPC_STATE_NONE = 0,                             /**< \internal xxx */
    ET9_SPC_STATE_STD_INIT_OK,                          /**< \internal xxx */
    ET9_SPC_STATE_FLEX_INIT_OK                          /**< \internal xxx */
};

/*---------------------------------------------------------------------------*/
/** \internal
 *
 */

#define _ET9_Flex_KeyQuality(pFirstSymb, nIndex)    ((pFirstSymb)[nIndex].bTraceIndex ? (pFirstSymb)[nIndex].bTraceProbability : 0xFF)

/*---------------------------------------------------------------------------*/
/** \internal
 *
 */

#define _ET9_Flex_LockInfo(pFirstSymb, psLockSymb, nIndex)      ((psLockSymb)[nIndex] ? ET9_LOCKED_SYMB_VALUE_AND_POS : ((pFirstSymb)[nIndex].bTraceIndex && (pFirstSymb)[nIndex].bTraceProbability != 0xFF) ? ET9_LOCKED_SYMB_NONE : ET9_SPC_IS_LOCKED_POS(&(pFirstSymb)[nIndex]))

/*---------------------------------------------------------------------------*/
/** \internal
 *
 */

#define _ET9_Flex_IsBetterOp(nOffsetR, nOffsetC)                                                        \
(                                                                                                       \
    bEditCost <  bEditDist ||                                                                           \
    bEditCost == bEditDist &&                                                                           \
        (bStemCost <  bStemDist ||                                                                      \
         bStemCost == bStemDist &&                                                                      \
            (bFreeCost <  bFreeDist ||                                                                  \
             bFreeCost == bFreeDist &&                                                                  \
                pCMP->ppxStemFreq[nR + (nOffsetR)][nC + (nOffsetC)] > xStemFreq))                       \
)                                                                                                       \

/*---------------------------------------------------------------------------*/

ET9STATUS ET9FARCALL _ET9AWCalcEditDistanceInit(ET9AWLingInfo     * const pLingInfo,
                                                const ET9U16              wIndex,
                                                const ET9U16              wLength,
                                                const ET9U8               bSpcMode);

void ET9FARCALL _ET9AWCalcEditDistanceDone(ET9AWLingInfo     * const pLingInfo);

#ifdef ET9_DEBUG_FLEXVER
void ET9FARCALL _ET9AWValidateFlexArea(ET9AWLingCmnInfo * const pLingCmnInfo);
void ET9FARCALL _ET9AWModifiedFlexArea(ET9AWLingCmnInfo * const pLingCmnInfo);
#else
#define _ET9AWValidateFlexArea(pLingCmnInfo)
#define _ET9AWModifiedFlexArea(pLingCmnInfo)
#endif

/* ----------------------- things used for selection list statistics ----------------------- */

#ifndef __STAT_AWLdb_Call
#define __STAT_AWLdb_Call
#endif

#ifndef __STAT_AWLdb_Flex_Call
#define __STAT_AWLdb_Flex_Call
#define __STAT_AWLdb_Flex_Done
#define __STAT_AWLdb_Flex_EDS_Call
#define __STAT_AWLdb_Flex_EDS_Done
#define __STAT_AWLdb_Flex_EDC_Call
#define __STAT_AWLdb_Flex_EDC_Done(x)
#define __STAT_AWLdb_Flex_SLA_Call
#define __STAT_AWLdb_Flex_SLA_Done
#define __STAT_AWLdb_Flex_EDLL_Cnt
#define __STAT_AWLdb_Flex_EDLLS_Cnt
#endif

/* ----------------------- things used for selection list statistics ----------------------- */

#ifdef ET9_ACTIVATE_SLST_STATS

#include <stdio.h>

#ifndef ET9S64
#define ET9S64     long long
#endif

#define __STAT_INC_CmpOther               ++ET9AWSPCCmpOther
#define __STAT_INC_CmpLdbOnly             ++ET9AWSPCCmpLdbOnly
#define __STAT_INC_CmpLdbInfo             ++ET9AWSPCCmpLdbInfo

#define __STAT_INC_TotCalcCount           ++ET9AWSPCTotCalcCount
#define __STAT_INC_ClassicMatch           ++ET9AWSPCClassicMatch
#define __STAT_INC_NoStemMatch            ++ET9AWSPCNoStemMatch
#define __STAT_INC_PossibleSpcCandidate   ++ET9AWSPCPossibleSpcCandidate
#define __STAT_INC_WithinCandidate        ++ET9AWSPCWithinCandidate
#define __STAT_INC_EdCalcCandidate        ++ET9AWSPCEdCalcCandidate
#define __STAT_INC_ActualCandidate        ++ET9AWSPCActualCandidate

#define __STAT_INC_TotLdbCount            ++ET9AWSPCTotLdbCount

#if defined(__cplusplus)
extern "C" {
#endif

extern ET9S64 ET9AWSPCCmpOther;
extern ET9S64 ET9AWSPCCmpLdbOnly;
extern ET9S64 ET9AWSPCCmpLdbInfo;

extern ET9S64 ET9AWSPCTotLdbCount;
extern ET9S64 ET9AWSPCTotCalcCount;
extern ET9S64 ET9AWSPCClassicMatch;
extern ET9S64 ET9AWSPCNoStemMatch;
extern ET9S64 ET9AWSPCPossibleSpcCandidate;
extern ET9S64 ET9AWSPCWithinCandidate;
extern ET9S64 ET9AWSPCEdCalcCandidate;
extern ET9S64 ET9AWSPCActualCandidate;

void _ET9AWSpcPrintStats(FILE *pf, ET9AWLingInfo *pLingInfo);
void _ET9AWSpcClearStats(ET9AWLingInfo *pLingInfo);

#if defined (__cplusplus)
    }
#endif

#else /* ET9_ACTIVATE_SLST_STATS */

#define __STAT_INC_CmpOther
#define __STAT_INC_CmpLdbOnly
#define __STAT_INC_CmpLdbInfo

#define __STAT_INC_TotCalcCount
#define __STAT_INC_ClassicMatch
#define __STAT_INC_NoStemMatch
#define __STAT_INC_PossibleSpcCandidate
#define __STAT_INC_WithinCandidate
#define __STAT_INC_EdCalcCandidate
#define __STAT_INC_ActualCandidate

#define __STAT_INC_TotLdbCount

#endif /* ET9_ACTIVATE_SLST_STATS */


/*! @} */
#endif /* !ET9ASPC_H */
/* ----------------------------------< eof >--------------------------------- */
