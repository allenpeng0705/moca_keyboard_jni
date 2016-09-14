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
;**     FileName: et9aspci.h                                                  **
;**                                                                           **
;**  Description: Alphabetic spell correction routines - very internal use.   **
;**                                                                           **
;*******************************************************************************
;******************************************************************************/

/*! \internal \addtogroup et9aspci Spell correction for alphabetic (internals)
* Spell Correction for alphabetic XT9.
* Specificly edit distance based rejection and calulation.
* @{
*/

#ifndef ET9_SPC_ED_GetMatchInfo
#error ET9_SPC_ED_GetMatchInfo is a required macro
#endif /* ET9_SPC_ED_GetMatchInfo */

#ifndef ET9_SPC_ED_GetWordSymb
#error ET9_SPC_ED_GetWordSymb is a required macro
#endif /* ET9_SPC_ED_GetWordSymb */


#include "et9aspc.h"


#ifdef _WIN32
#pragma message ("*** CREATING EDIT DISTANCE FUNCTION ***")
#endif

#ifdef ET9_SPC_ED_USE_COMPARE_CACHE
#ifdef _WIN32
#pragma message ("*** USING COMPARE CACHE (for slow compare) ***")
#endif
#endif /* ET9_SPC_ED_USE_COMPARE_CACHE */


/*---------------------------------------------------------------------------
 *
 *   Macro: undef
 *
 *---------------------------------------------------------------------------*/

#undef ASPC
#undef ASPCCMP

#undef ET9_SPC_ED_DIST_ROWS_CENTER
#undef ET9_SPC_ED_CALC_ROWS_CENTER
#undef ET9_SPC_ED_MAX_COST

#undef __ET9AWAssignTapFreq
#undef __ET9AWSetByteArray
#undef __ET9AWGetMatchInfoRM
#undef __ET9AWGetMatchOnlyRM
#undef __ET9AWHasCostTagRM
#undef __ET9AWSetCostTagRM
#undef __ET9AWIsTransposeSpotRM
#undef __ET9AWIsCostSpotRM
#undef __ET9AWNoneFreqRM
#undef __ET9AWDefaultFreqRM
#undef __ET9AWSubstitutionFreqRM
#undef __ET9AWTranspositionFreqRM
#undef __ET9AWLogCache
#undef __ET9AWLogWord
#undef __ET9AWLogByteArray
#undef __ET9AWIsLockSpotRM
#undef __ET9AWInvestigateSpotRM

#undef WLOG

/*---------------------------------------------------------------------------
 *
 *   Macro: defs (and type defs)
 *
 *---------------------------------------------------------------------------*/

#define ASPC                            (pLingCmnInfo->Private.ASpc)        /**< \internal Convenience access for ASPC structure. */
#define ASPCCMP                         (ASPC.u.sCmpData)                   /**< \internal Convenience access for ASPC compare data. */

#define ET9_SPC_ED_DIST_ROWS_CENTER     (ET9_SPC_ED_MAX_DISTANCE)           /**< \internal Convenience data access for ASPC structure. */
#define ET9_SPC_ED_CALC_ROWS_CENTER     (ET9_SPC_ED_MAX_DISTANCE + 1)       /**< \internal Convenience data access for ASPC structure. */

#define ET9_SPC_ED_MAX_COST             (ET9_SPC_ED_MAX_DISTANCE + 1)       /**< \internal Maximum edit distance cost (will reject). */


#ifdef ET9_SPC_LOG_HELP_FUNC
#include <stdio.h>
#define WLOG(q) { if (pLogFile2 == NULL) { pLogFile2 = fopen("zzzET9ASPC2.txt", "w"); } q fflush(pLogFile2); }
#define ET9AssertWLog(x) { (void)((!!(x)) || !pLogFile2 || fprintf(pLogFile2, "\n\nASSERT line %d\n%s\n\n", __LINE__, #x) && fflush(pLogFile2)); ET9Assert(x); }
#if defined(__cplusplus)
extern "C" {
#endif
extern FILE *pLogFile2;
#if defined (__cplusplus)
}
#endif
#else
#define WLOG(q)
#define ET9AssertWLog(x) ET9Assert(x)
#endif /* ET9_SPC_LOG_HELP_FUNC */


#ifdef ET9_USE_FLOAT_FREQS

/** \internal
 * Assign tap frequency.
 * This function performs final frequency adjustment and assigns it to the word.
 *
 * @param pWord                     Word to update.
 * @param fResultFreq               Frequency to assign.
 * @param wLength                   Length of input.
 *
 * @return None
 */

#define __ET9AWAssignTapFreq(pWord, fResultFreq, wLength)                                               \
{                                                                                                       \
    pWord->xTapFreq = (ET9FREQPART)(fResultFreq / _ET9pow_f(10, (ET9FLOAT)__ET9Min(wLength, ET9_SPC_ED_MAX_FREQ_LEN))); \
                                                                                                        \
    ET9AssertWLog((fResultFreq) >= 0);                                                                  \
    ET9AssertWLog(pWord->xTapFreq >= 0);                                                                \
}                                                                                                       \

#else

/** \internal
 * Assign tap frequency.
 * This function performs final frequency adjustment and assigns it to the word.
 *
 * @param pWord                     Word to update.
 * @param wResultFreq               Frequency to assign.
 * @param wLength                   Length of input.
 *
 * @return None
 */

#define __ET9AWAssignTapFreq(pWord, wResultFreq, wLength)                                               \
{                                                                                                       \
    ET9U16 wTemp = wLength;                                                                             \
                                                                                                        \
    if (wTemp > 8) {                                                                                    \
        pWord->xTapFreq = (ET9FREQPART)((wResultFreq) >> 16);                                           \
    }                                                                                                   \
    else if (wTemp <= 4) {                                                                              \
        pWord->xTapFreq = (ET9FREQPART)(wResultFreq);                                                   \
    }                                                                                                   \
    else {                                                                                              \
        wTemp = 4 * (wTemp - 4);                                                                        \
        pWord->xTapFreq = (ET9FREQPART)((wResultFreq) >> wTemp);                                        \
    }                                                                                                   \
                                                                                                        \
    if (pWord->xTapFreq == 0) {                                                                         \
        pWord->xTapFreq = 1;                                                                            \
    }                                                                                                   \
                                                                                                        \
    ET9AssertWLog((wResultFreq) >= 0);                                                                  \
    ET9AssertWLog(pWord->xTapFreq >= 0);                                                                \
}                                                                                                       \

#endif

/** \internal
 * Set byte array.
 * Function for fast set of small byte array.
 * When the core is compiled using C standard libraries those will replace this code.
 *
 * @param pArray                    Array to modify.
 * @param wSize                     Number of items to set.
 * @param bValue                    Value to set.
 *
 * @return None
 */

#ifdef ET9ACTIVATEMISCSTDCLIBUSE

#define __ET9AWSetByteArray(pArray, wSize, bValue)  _ET9ByteSet(pArray, wSize, bValue)

#else /* ET9ACTIVATEMISCSTDCLIBUSE */

#define __ET9AWSetByteArray(pArray, wSize, bValue)                                                      \
{                                                                                                       \
    ET9U8 *pb = &pArray[0];                                                                             \
    const ET9U8 bAssignValue = (bValue);                                                                \
    switch (wSize)                                                                                      \
    {                                                                                                   \
        case 16: *(pb++) = bAssignValue;                                                                \
        case 15: *(pb++) = bAssignValue;                                                                \
        case 14: *(pb++) = bAssignValue;                                                                \
        case 13: *(pb++) = bAssignValue;                                                                \
        case 12: *(pb++) = bAssignValue;                                                                \
        case 11: *(pb++) = bAssignValue;                                                                \
        case 10: *(pb++) = bAssignValue;                                                                \
        case  9: *(pb++) = bAssignValue;                                                                \
        case  8: *(pb++) = bAssignValue;                                                                \
        case  7: *(pb++) = bAssignValue;                                                                \
        case  6: *(pb++) = bAssignValue;                                                                \
        case  5: *(pb++) = bAssignValue;                                                                \
        case  4: *(pb++) = bAssignValue;                                                                \
        case  3: *(pb++) = bAssignValue;                                                                \
        case  2: *(pb++) = bAssignValue;                                                                \
        case  1: *(pb++) = bAssignValue;                                                                \
            break;                                                                                      \
        default: ET9AssertWLog(0);                                                                      \
            break;                                                                                      \
    }                                                                                                   \
}                                                                                                       \

#endif /* ET9ACTIVATEMISCSTDCLIBUSE */

/** \internal
 * Get match info (rotated matrix).
 * Macro for adopting source to __ET9AWCalcEditDistance.
 * This macro expands to differenct code depnding on if the cache is activated or not.<br>
 * If the source is really fast at performing compares, the cache should be turned off.<br>
 *
 * A bunch of global data is assumed to be available:<P>
 * pLingInfo, bExactSpcMode, wIndex, pWord, pbAtLens, ppbFreqRows, ppbCmpResultRows
 *
 * @param snRow                     Relative index in source string.
 * @param snCol                     Index in compared string.
 * @param pbMatch                   (out) the resulting match info.
 * @param pbFreq                    (out) the resulting match frequency.
 * @param pbLocked                  (out) the resulting lock info.
 *
 * @return None
 */

#ifdef ET9_SPC_ED_USE_COMPARE_CACHE

#define __ET9AWGetMatchInfoRM(snRow, snCol, pbMatch, pbFreq, pbLocked)                                  \
{                                                                                                       \
    ET9U8           *pbAt = &pbAtLens[snRow];                                                           \
    ET9U8           *pbCmp;                                                                             \
    ET9U8 * const   pbCmpCell = &ppbCmpResultRows[snRow][snCol];                                        \
                                                                                                        \
    ET9AssertWLog((snCol) >= 0 && (snCol) < ET9_SPC_ED_STORE_ROW_LEN);                                  \
    ET9AssertWLog((snRow) + ET9_SPC_ED_CALC_ROWS_CENTER >= 0 &&                                         \
              (snRow) + ET9_SPC_ED_CALC_ROWS_CENTER < ET9_SPC_ED_CALC_ROWS);                            \
                                                                                                        \
    if (*pbAt > (snCol) && *pbCmpCell != ET9_SPC_ED_MATCH_UNKNOWN) {                                    \
                                                                                                        \
        const ET9U8 bCmp = *pbCmpCell;                                                                  \
                                                                                                        \
        ET9AssertWLog(bCmp != 0xcc);                                                                    \
                                                                                                        \
        (pbMatch)  = bCmp & 0xF;                                                                        \
        (pbLocked) = bCmp >> 4;                                                                         \
        (pbFreq)   = ppbFreqRows[snRow][snCol];                                                         \
                                                                                                        \
        WLOG(fprintf(pLogFile2, "........cached compare @ %+2d:%02d\n", snRow, snCol);)                 \
    }                                                                                                   \
    else {                                                                                              \
        const ET9INT  snSrcIndex = (snCol) - (snRow);                                                   \
        const ET9INT  snTstIndex = snCol;                                                               \
        const ET9BOOL bExactMatchOnly = (ET9BOOL)((bExactSpcMode) && (snRow));                          \
                                                                                                        \
        if (bExactMatchOnly) {}; /* possibly unused */                                                  \
                                                                                                        \
        WLOG(fprintf(pLogFile2, "........in org matrix, row %02d, col %02d\n", snSrcIndex, snTstIndex);)\
                                                                                                        \
        if (snSrcIndex >= snLen1) {                                                                     \
            ET9AssertWLog(snSrcIndex >= 0 && snTstIndex >= 0 && snTstIndex < snLen2);                   \
            pbMatch = ET9_SPC_ED_MATCH_NONE;                                                            \
            pbLocked = ET9_LOCKED_SYMB_NONE;                                                            \
            pbFreq = 0;                                                                                 \
            WLOG(fprintf(pLogFile2, "........outside compare @ %+2d:%02d (no match)\n", snRow, snCol);) \
        }                                                                                               \
        else {                                                                                          \
            ET9_SPC_ED_GetMatchInfo(snSrcIndex, snTstIndex, bExactMatchOnly, pbMatch, pbFreq, pbLocked);\
        }                                                                                               \
                                                                                                        \
        if (*pbAt <= snCol) {                                                                           \
            ++(*pbAt);                                                                                  \
            pbCmp = pbCmpCell - 1;                                                                      \
            for (; *pbAt <= snCol; ++(*pbAt), --pbCmp) {                                                \
                *pbCmp = ET9_SPC_ED_MATCH_UNKNOWN;                                                      \
            }                                                                                           \
        }                                                                                               \
                                                                                                        \
        ppbFreqRows[snRow][snCol] = (ET9U8)(pbFreq);                                                    \
        *pbCmpCell = (ET9U8)(((pbLocked) << 4) | ((pbMatch) & 0xF));                                    \
                                                                                                        \
        WLOG(fprintf(pLogFile2, "........actual compare @ %+2d:%02d\n", snRow, snCol);)                 \
    }                                                                                                   \
                                                                                                        \
    if (pbMatch) {};    /* possibly unused */                                                           \
    if (pbLocked) {};   /* possibly unused */                                                           \
}                                                                                                       \

#else /* ET9_SPC_ED_USE_COMPARE_CACHE */

#define __ET9AWGetMatchInfoRM(snRow, snCol, pbMatch, pbFreq, pbLocked)                                  \
{                                                                                                       \
    const ET9INT  snSrcIndex = (snCol) - (snRow);                                                       \
    const ET9INT  snTstIndex = (snCol);                                                                 \
    const ET9BOOL bExactMatchOnly = (ET9BOOL)((bExactSpcMode) && (snRow));                              \
                                                                                                        \
    if (bExactMatchOnly) {}; /* possibly unused */                                                      \
                                                                                                        \
    WLOG(fprintf(pLogFile2, "....in org matrix, row %02d, col %02d\n", snSrcIndex, snTstIndex);)        \
                                                                                                        \
    if (snSrcIndex >= snLen1) {                                                                         \
        ET9AssertWLog(snSrcIndex >= 0 && snTstIndex >= 0 && snTstIndex < snLen2);                       \
        pbMatch = ET9_SPC_ED_MATCH_NONE;                                                                \
        pbLocked = ET9_LOCKED_SYMB_NONE;                                                                \
        pbFreq = 0;                                                                                     \
        WLOG(fprintf(pLogFile2, "....outside compare @ %+2d:%02d (no match)\n", snRow, snCol);)         \
    }                                                                                                   \
    else {                                                                                              \
        ET9_SPC_ED_GetMatchInfo(snSrcIndex, snTstIndex, bExactMatchOnly, pbMatch, pbFreq, pbLocked);    \
    }                                                                                                   \
                                                                                                        \
    WLOG(fprintf(pLogFile2, "....actual compare @ %+2d:%02d\n", snRow, snCol);)                         \
                                                                                                        \
    if (pbMatch) {};    /* possibly unused */                                                           \
    if (pbLocked) {};   /* possibly unused */                                                           \
}                                                                                                       \

#endif /* ET9_SPC_ED_USE_COMPARE_CACHE */

/** \internal
 * Get match only (rotated matrix).
 * Macro for adopting source to __ET9AWCalcEditDistance.
 * This is a macro dedicated to getting match only, fastest possible way.<br>
 * The macro exapnds in different ways depending on if there is a cache, if the feature is supported etc.
 *
 * @param snRow                     Relative index in source string.
 * @param snCol                     Index in compared string.
 * @param pbMatch                   (out) the resulting match info.
 *
 * @return None
 */

#ifdef ET9_SPC_ED_USE_COMPARE_CACHE

#define __ET9AWGetMatchOnlyRM(snRow, snCol, pbMatch)                                                    \
{                                                                                                       \
    ET9U8 bFreq;                                                                                        \
    ET9U8 bLocked;                                                                                      \
    __ET9AWGetMatchInfoRM(snRow, snCol, pbMatch, bFreq, bLocked);                                       \
}                                                                                                       \

#else /* ET9_SPC_ED_USE_COMPARE_CACHE */

#ifdef ET9_SPC_ED_GetMatchOnly

#define __ET9AWGetMatchOnlyRM(snRow, snCol, pbMatch)                                                    \
{                                                                                                       \
    const ET9INT  snSrcIndex = (snCol) - (snRow);                                                       \
    const ET9INT  snTstIndex = snCol;                                                                   \
    const ET9BOOL bExactMatchOnly = (ET9BOOL)((bExactSpcMode) && (snRow));                              \
                                                                                                        \
    if (bExactMatchOnly) {}; /* possibly unused */                                                      \
                                                                                                        \
    WLOG(fprintf(pLogFile2, "....in org matrix, row %02d, col %02d\n", snSrcIndex, snTstIndex);)        \
                                                                                                        \
    if (snSrcIndex >= snLen1) {                                                                         \
        ET9AssertWLog(snSrcIndex >= 0 && snTstIndex >= 0 && snTstIndex < snLen2);                       \
        pbMatch = ET9_SPC_ED_MATCH_NONE;                                                                \
        WLOG(fprintf(pLogFile2, "....outside compare @ %+2d:%02d (no match)\n", snRow, snCol);)         \
    }                                                                                                   \
    else {                                                                                              \
        ET9_SPC_ED_GetMatchOnly(snSrcIndex, snTstIndex, bExactMatchOnly, pbMatch);                      \
    }                                                                                                   \
                                                                                                        \
    WLOG(fprintf(pLogFile2, "....actual compare @ %+2d:%02d\n", snRow, snCol);)                         \
}                                                                                                       \

#else /* ET9_SPC_ED_GetMatchOnly */

#ifdef _WIN32
#pragma message ("*** NO ET9_SPC_ED_GetMatchOnly SUPPORT (SLOW) ***")
#endif

#define __ET9AWGetMatchOnlyRM(snRow, snCol, pbMatch)                                                    \
{                                                                                                       \
    ET9U8 bFreq;                                                                                        \
    ET9U8 bLocked;                                                                                      \
    __ET9AWGetMatchInfoRM(snRow, snCol, pbMatch, bFreq, bLocked);                                       \
}                                                                                                       \

#endif /* ET9_SPC_ED_GetMatchOnly */

#endif /* ET9_SPC_ED_USE_COMPARE_CACHE */

/** \internal
 * Has cost tag (rotated matrix).
 * Macro for checking if a spot has been assigned a cost.
 *
 * @param snRow                     Relative index in source string.
 * @param snCol                     Index in compared string.
 *
 * @return Non zero if it has, otherwise zero.
 */

#define __ET9AWHasCostTagRM(snRow, snCol)                                                               \
(                                                                                                       \
    pbLastCostPoss[snRow] == snCol                                                                      \
)                                                                                                       \

/** \internal
 * Set cost tag (rotated matrix).
 * Macro for tagging a spot with a cost.
 *
 * @param snRow                     Relative index in source string.
 * @param snCol                     Index in compared string.
 *
 * @return None
 */

#define __ET9AWSetCostTagRM(snRow, snCol)                                                               \
{                                                                                                       \
    pbLastCostPoss[snRow] = (ET9U8)(snCol);                                                             \
}                                                                                                       \

/** \internal
 * Is lock spot (rotated matrix).
 * Macro for checking if this is a locked spot.
 *
 * @param snRow                     Relative index in source string.
 * @param snCol                     Index in compared string.
 * @param pbLocked                  (out) if this is a locked spot or not.
 *
 * @return None
 */

#define __ET9AWIsLockSpotRM(snRow, snCol, pbLocked)                                                     \
{                                                                                                       \
    ET9U8  bMatch;                                                                                      \
    ET9U8  bFreq;                                                                                       \
                                                                                                        \
    WLOG(fprintf(pLogFile2, "......checking lock spot @ %+2d:%02d\n", snRow, snCol);)                   \
                                                                                                        \
    __ET9AWGetMatchInfoRM(snRow, snCol, bMatch, bFreq, pbLocked);                                       \
}                                                                                                       \

/** \internal
 * Is transpose spot (rotated matrix).
 * Macro for assuring that transpose information is up to date at a pos.
 *
 * @param snRow                     Relative index in source string.
 * @param snCol                      Index in compared string.
 * @param bExactMatch               If matching should be limited to exact.
 * @param pbTrp                     (out) if this is a transpose spot or not.
 *
 * @return None
 */

#define __ET9AWIsTransposeSpotRM(snRow, snCol, bExactMatch, pbTrp)                                      \
{                                                                                                       \
    ET9U8  bMatch;                                                                                      \
                                                                                                        \
    /* transpose can't happen in first position (not all starts at zero) */                             \
                                                                                                        \
    if (!(snCol) || (snCol) <= (snRow)) {                                                               \
        (pbTrp) = 0;                                                                                    \
    }                                                                                                   \
    else {                                                                                              \
                                                                                                        \
        /* to be a transpose the transposed chars must match - first one (delete pos) */                \
                                                                                                        \
        __ET9AWGetMatchOnlyRM((snRow) + 1, snCol, bMatch);                                              \
                                                                                                        \
        if (!ET9_SPC_ED_IS_MATCH(bExactMatch, bMatch)) {                                                \
            (pbTrp) = 0;                                                                                \
        }                                                                                               \
        else {                                                                                          \
                                                                                                        \
            /* to be a transpose the transposed chars must match - second one (insert pos) */           \
                                                                                                        \
            __ET9AWGetMatchOnlyRM((snRow) - 1, (snCol) - 1, bMatch);                                    \
                                                                                                        \
            if (!ET9_SPC_ED_IS_MATCH(bExactMatch, bMatch)) {                                            \
                (pbTrp) = 0;                                                                            \
            }                                                                                           \
            else {                                                                                      \
                                                                                                        \
                /* this is a transpose */                                                               \
                                                                                                        \
                (pbTrp) = 1;                                                                            \
            }                                                                                           \
        }                                                                                               \
    }                                                                                                   \
}                                                                                                       \

/** \internal
 * Is cost spot (rotated matrix).
 * Macro for checking if this is a cost spot, for spc rejection.
 * To be a spot it must be a substitution without a possible transposition.
 *
 * @param snRow                     Relative index in source string.
 * @param snCol                     Index in compared string.
 * @param pbCost                    (out) if this is a cost spot or not.
 *
 * @return None
 */

#define __ET9AWIsCostSpotRM(snRow, snCol, pbCost)                                                       \
{                                                                                                       \
    ET9U8  bTrp;                                                                                        \
    ET9U8  bMatch;                                                                                      \
                                                                                                        \
    WLOG(fprintf(pLogFile2, "......checking cost spot @ %+2d:%02d\n", snRow, snCol);)                   \
                                                                                                        \
    /* first check current pos - if a match things are simple */                                        \
                                                                                                        \
    __ET9AWGetMatchOnlyRM(snRow, snCol, bMatch);                                                        \
                                                                                                        \
    if (ET9_SPC_ED_IS_MATCH(bExactSpcMode, bMatch)) {                                                   \
        pbCost = 0;                                                                                     \
        WLOG(fprintf(pLogFile2, "........ICS: match (0)\n");)                                           \
    }                                                                                                   \
    else {                                                                                              \
        __ET9AWIsTransposeSpotRM(snRow, snCol, bExactSpcMode, bTrp);                                    \
                                                                                                        \
        /* if there is no possible transpose, this is a non match */                                    \
                                                                                                        \
        if (!bTrp) {                                                                                    \
            pbCost = 1;                                                                                 \
            __ET9AWSetCostTagRM(snRow, snCol);                                                          \
            WLOG(fprintf(pLogFile2, "........ICS: no match, no transpose (1)\n");)                      \
        }                                                                                               \
                                                                                                        \
        /* if a transpose where the first non match hasn't been counted, this counts */                 \
                                                                                                        \
        else if (!__ET9AWHasCostTagRM(snRow, snCol - 1)) {                                              \
            pbCost = 1;                                                                                 \
            __ET9AWSetCostTagRM(snRow, snCol);                                                          \
            WLOG(fprintf(pLogFile2, "........ICS: fresh transpose (1)\n");)                             \
        }                                                                                               \
                                                                                                        \
        /* a transpose with the cost already being taking into account */                               \
                                                                                                        \
        else {                                                                                          \
            pbCost = 0;                                                                                 \
            WLOG(fprintf(pLogFile2, "........ICS: 'used' transpose (0)\n");)                            \
        }                                                                                               \
    }                                                                                                   \
}                                                                                                       \

/** \internal
 * Investigate spot (rotated matrix).
 * Macro for checking if this is a cost spot, for distance calculation.
 * To be a spot it must be a substitution without a possible transposition.
 *
 * @param snRow                     Relative index in source string.
 * @param snCol                     Index in compared string.
 * @param bExactMatch               If matching should be limited to exact.
 * @param pbKind                    (out) 0 = match, 1 = subst, 2 = transpose.
 * @param pbLocked                  (out) if this is a locked spot or not.
 *
 * @return None
 */

#define __ET9AWInvestigateSpotRM(snRow, snCol, bExactMatch, pbKind, pbLocked)                           \
{                                                                                                       \
    ET9U8  bMatch;                                                                                      \
    ET9U8  bFreq;                                                                                       \
    ET9U8  bTrp;                                                                                        \
                                                                                                        \
    WLOG(fprintf(pLogFile2, "......checking cost spot lock @ %+2d:%02d\n", snRow, snCol);)              \
                                                                                                        \
    /* first check current pos - if a match things are simple */                                        \
                                                                                                        \
    __ET9AWGetMatchInfoRM(snRow, snCol, bMatch, bFreq, pbLocked);                                       \
                                                                                                        \
    if (ET9_SPC_ED_IS_MATCH(bExactMatch, bMatch)) {                                                     \
        pbKind = 0;                                                                                     \
        WLOG(fprintf(pLogFile2, "........ICS: match (0)\n");)                                           \
    }                                                                                                   \
    else {                                                                                              \
        __ET9AWIsTransposeSpotRM(snRow, snCol, bExactMatch, bTrp);                                      \
                                                                                                        \
        /* either a transpose or a substitution */                                                      \
                                                                                                        \
        if (bTrp) {                                                                                     \
            pbKind = 2;                                                                                 \
            WLOG(fprintf(pLogFile2, "........ICS: transpose (2)\n");)                                   \
        }                                                                                               \
        else {                                                                                          \
            pbKind = 1;                                                                                 \
            WLOG(fprintf(pLogFile2, "........ICS: no match, no transpose (1)\n");)                      \
        }                                                                                               \
    }                                                                                                   \
}                                                                                                       \

/** \internal
 * Calculate and assign frequency.
 *
 * @param bFreq                     Tap freq.
 * @param xTarget                   Target
 * @param xSource                   Source
 *
 * @return None
 */

#ifdef ET9_USE_FLOAT_FREQS

#define __ET9AWCalculateFreq(bFreq, xTarget, xSource)                                                   \
{                                                                                                       \
    (xTarget) = (ET9FREQ)((bFreq) * (xSource));                                                         \
}                                                                                                       \

#else

#define __ET9AWCalculateFreq(bFreq, xTarget, xSource)                                                   \
{                                                                                                       \
    const ET9FREQPART xTemp = ((bFreq) * 15) / 255;                                                     \
                                                                                                        \
    if (xTemp) {                                                                                        \
        (xTarget) = (ET9FREQ)(xTemp * (xSource));                                                       \
    }                                                                                                   \
    else {                                                                                              \
        (xTarget) = (xSource);                                                                          \
    }                                                                                                   \
}                                                                                                       \

#endif

/** \internal
 * Assign default frequency (rotated matrix).
 * Update with default frequency.
 *
 * Required "global" values: bCalculatingFreq (if currently calculating or not).
 *
 * @param xTarget                   Target
 * @param xSource                   Source
 *
 * @return None
 */

#define __ET9AWDefaultFreqRM(xTarget, xSource)                                                          \
{                                                                                                       \
    if (bCalculatingFreq) {                                                                             \
        __ET9AWCalculateFreq(ET9_SPC_ED_DEFOP_FREQ, xTarget, xSource);                                  \
    }                                                                                                   \
    else {                                                                                              \
        (xTarget) = (xSource);                                                                          \
    }                                                                                                   \
                                                                                                        \
    WLOG(fprintf(pLogFile2, "........__ET9AWDefaultFreq, xTarget = %5.0f, xSource = %5.0f\n",           \
                            (double)xTarget, (double)xSource);)                                         \
}                                                                                                       \

/** \internal
 * Assign no frequency change (rotated matrix).
 *
 * @param xTarget                   Target
 * @param xSource                   Source
 *
 * @return None
 */

#define __ET9AWNoneFreqRM(xTarget, xSource)                                                             \
{                                                                                                       \
    (xTarget) = (xSource);                                                                              \
                                                                                                        \
    WLOG(fprintf(pLogFile2, "........__ET9AWNoneFreqRM, xTarget = %5.0f, xSource = %5.0f\n",            \
                            (double)xTarget, (double)xSource);)                                         \
}                                                                                                       \

/** \internal
 * Assign substitution frequency (rotated matrix).
 * Update with substitution frequency.
 *
 * Required "global" values: bCalculatingFreq (if currently calculating or not).
 *
 * @param xTarget                   Target
 * @param xSource                   Source
 * @param snRow                     Relative index in source string.
 * @param snCol                     Index in compared string.
 *
 * @return None
 */

#ifdef ET9_SPC_ED_USE_COMPARE_CACHE

#define __ET9AWSubstitutionFreqRM(xTarget, xSource, snRow, snCol)                                       \
{                                                                                                       \
    if (bCalculatingFreq) {                                                                             \
        __ET9AWCalculateFreq(ppbFreqRows[snRow][snCol], xTarget, xSource);                              \
    }                                                                                                   \
    else {                                                                                              \
        (xTarget) = (xSource);                                                                          \
    }                                                                                                   \
                                                                                                        \
    WLOG(fprintf(pLogFile2, "........__ET9AWSubstitutionFreq, xTarget = %5.0f, xSource = %5.0f\n",      \
                            (double)xTarget, (double)xSource);)                                         \
}                                                                                                       \

#else /* ET9_SPC_ED_USE_COMPARE_CACHE */

#define __ET9AWSubstitutionFreqRM(xTarget, xSource, snRow, snCol)                                       \
{                                                                                                       \
    ET9U8  bMatch;                                                                                      \
    ET9U8  bFreq;                                                                                       \
    ET9U8  bLocked;                                                                                     \
                                                                                                        \
    __ET9AWGetMatchInfoRM(snRow, snCol, bMatch, bFreq, bLocked);                                        \
                                                                                                        \
    if (bCalculatingFreq) {                                                                             \
        __ET9AWCalculateFreq(bFreq, xTarget, xSource);                                                  \
    }                                                                                                   \
    else {                                                                                              \
        (xTarget) = (xSource);                                                                          \
    }                                                                                                   \
                                                                                                        \
    WLOG(fprintf(pLogFile2, "........__ET9AWSubstitutionFreq, xTarget = %5.0f, xSource = %5.0f, bFreq = %3u\n", \
                            (double)xTarget, (double)xSource, bFreq);)                                  \
}                                                                                                       \

#endif /* ET9_SPC_ED_USE_COMPARE_CACHE */

/** \internal
 * Assign transposition frequency (rotated matrix).
 * Update with transposition frequency.
 *
 * Required "global" values:<P>
 * bCalculatingFreq (if currently calculating or not),<BR>
 * bPrevCalculatingFreq (if previously calculating or not)
 *
 * @param xTarget                   Target
 * @param xSource                   Source
 * @param snRow                     Relative index in source string.
 * @param snCol                     Index in compared string.
 *
 * @return None
 */

#ifdef ET9_SPC_ED_USE_COMPARE_CACHE

#define __ET9AWTranspositionFreqRM(xTarget, xSource, snRow, snCol)                                      \
{                                                                                                       \
    if (bCalculatingFreq) {                                                                             \
        __ET9AWCalculateFreq(ppbFreqRows[snRow + 1][snCol], xTarget, xSource);                          \
    }                                                                                                   \
    else {                                                                                              \
        (xTarget) = (xSource);                                                                          \
    }                                                                                                   \
                                                                                                        \
    if (bPrevCalculatingFreq) {                                                                         \
        __ET9AWCalculateFreq(ppbFreqRows[snRow - 1][snCol - 1], xTarget, xTarget);                      \
    }                                                                                                   \
                                                                                                        \
    WLOG(fprintf(pLogFile2, "....__ET9AWTranspositionFreq, xTarget = %5.0f, xSource = %5.0f\n",         \
                            (double)xTarget, (double)xSource);)                                         \
}                                                                                                       \

#else /* ET9_SPC_ED_USE_COMPARE_CACHE */

#define __ET9AWTranspositionFreqRM(xTarget, xSource, snRow, snCol)                                      \
{                                                                                                       \
    ET9U8  bMatch;                                                                                      \
    ET9U8  bFreqD;                                                                                      \
    ET9U8  bFreqI;                                                                                      \
    ET9U8  bLocked;                                                                                     \
                                                                                                        \
    __ET9AWGetMatchInfoRM(snRow + 1, snCol    , bMatch, bFreqD, bLocked);                               \
    __ET9AWGetMatchInfoRM(snRow - 1, snCol - 1, bMatch, bFreqI, bLocked);                               \
                                                                                                        \
    if (bCalculatingFreq) {                                                                             \
        __ET9AWCalculateFreq(bFreqD, xTarget, xSource);                                                 \
    }                                                                                                   \
    else {                                                                                              \
        (xTarget) = (xSource);                                                                          \
    }                                                                                                   \
                                                                                                        \
    if (bPrevCalculatingFreq) {                                                                         \
        __ET9AWCalculateFreq(bFreqI, xTarget, xTarget);                                                 \
    }                                                                                                   \
                                                                                                        \
    WLOG(fprintf(pLogFile2, "....__ET9AWTranspositionFreq, xTarget = %5.0f, xSource = %5.0f\n",         \
                            (double)xTarget, (double)xSource);)                                         \
}                                                                                                       \

#endif /* ET9_SPC_ED_USE_COMPARE_CACHE */

/** \internal
 * Log the cache.
 * If there is a cache and if debug is active.
 *
 * @param pcComment                 Comment to be printed with the log info.
 *
 * @return None
 */

#ifdef ET9_SPC_LOG_HELP_FUNC

#ifdef ET9_SPC_ED_USE_COMPARE_CACHE

#define __ET9AWLogCache(pcComment)                                                                      \
{                                                                                                       \
    ET9INT          snRow;                                                                              \
    ET9INT          snCol;                                                                              \
    const ET9INT    snRowSpan = ET9_SPC_ED_CALC_ROWS - ET9_SPC_ED_CALC_ROWS_CENTER - 1;                 \
                                                                                                        \
    WLOG(fprintf(pLogFile2, "\nCache log (%s)\n\n", pcComment);)                                        \
                                                                                                        \
    for (snRow = snRowSpan; snRow >= -snRowSpan; --snRow) {                                             \
                                                                                                        \
        WLOG(fprintf(pLogFile2, "CMP%+03d  ", snRow);)                                                  \
                                                                                                        \
        for (snCol= 0; snCol < 16 || snCol < pbAtLens[snRow]; ++snCol) {                                \
                                                                                                        \
            if (snCol< pbAtLens[snRow]) {                                                               \
                WLOG(fprintf(pLogFile2, "%02x ", ppbCmpResultRows[snRow][snCol]);)                      \
            }                                                                                           \
            else {                                                                                      \
                WLOG(fprintf(pLogFile2, ".. ");)                                                        \
            }                                                                                           \
        }                                                                                               \
                                                                                                        \
        WLOG(fprintf(pLogFile2, "\n");)                                                                 \
    }                                                                                                   \
                                                                                                        \
    WLOG(fprintf(pLogFile2, "\n");)                                                                     \
                                                                                                        \
    WLOG(fprintf(pLogFile2, "        ");)                                                               \
    for (snCol = 0; snCol < 16; ++snCol) {                                                              \
        WLOG(fprintf(pLogFile2, "%02d ", snCol);)                                                       \
    }                                                                                                   \
    WLOG(fprintf(pLogFile2, "\n");)                                                                     \
                                                                                                        \
    WLOG(fprintf(pLogFile2, "\n");)                                                                     \
                                                                                                        \
    for (snRow = snRowSpan; snRow >= -snRowSpan; --snRow) {                                             \
                                                                                                        \
        WLOG(fprintf(pLogFile2, "FRQ%+03d  ", snRow);)                                                  \
                                                                                                        \
        for (snCol= 0; snCol < 16 || snCol < pbAtLens[snRow]; ++snCol) {                                \
                                                                                                        \
            if (snCol < pbAtLens[snRow]) {                                                              \
                WLOG(fprintf(pLogFile2, "%02x ", ppbFreqRows[snRow][snCol]);)                           \
            }                                                                                           \
            else {                                                                                      \
                WLOG(fprintf(pLogFile2, ".. ");)                                                        \
            }                                                                                           \
        }                                                                                               \
                                                                                                        \
        WLOG(fprintf(pLogFile2, "\n");)                                                                 \
    }                                                                                                   \
                                                                                                        \
    WLOG(fprintf(pLogFile2, "\n");)                                                                     \
}                                                                                                       \

#else /* ET9_SPC_ED_USE_COMPARE_CACHE */

#define __ET9AWLogCache(cComment)

#endif /* ET9_SPC_ED_USE_COMPARE_CACHE */

#else /* ET9_SPC_LOG_HELP_FUNC */

#define __ET9AWLogCache(pcComment)

#endif /* ET9_SPC_LOG_HELP_FUNC */

/** \internal
 * Log a word.
 * If debug is active.
 *
 * @param pWord                     The word.
 * @param pcComment                 Comment to be printed with the log info.
 *
 * @return None
 */

#ifdef ET9_SPC_LOG_HELP_FUNC

#define __ET9AWLogWord(pWord, pcComment)                                                                \
{                                                                                                       \
    ET9INT snIndex;                                                                                     \
    char *pcwsrc;                                                                                       \
                                                                                                        \
    switch (pWord->bWordSrc)                                                                            \
    {                                                                                                   \
        case ET9WORDSRC_LDB: pcwsrc = " LDB"; break;                                                    \
        case ET9WORDSRC_RUDB: pcwsrc = "RUDB"; break;                                                   \
        case ET9WORDSRC_MDB: pcwsrc = " MDB"; break;                                                    \
        case ET9WORDSRC_ASDB_SHORTCUT: pcwsrc = "ASDB"; break;                                          \
        case ET9WORDSRC_LAS_SHORTCUT: pcwsrc = " LAS"; break;                                           \
        case ET9WORDSRC_QUDB: pcwsrc = "QUDB"; break;                                                   \
        default : pcwsrc = "\?\?\?"; break;                                                             \
    }                                                                                                   \
                                                                                                        \
    WLOG(fprintf(pLogFile2, "%s, %s, word = ", pcComment, pcwsrc);)                                     \
                                                                                                        \
    for (snIndex = 0; snIndex < snLen2; ++snIndex) {                                                    \
        if (pWord->Base.sWord[snIndex] < 0x20 || pWord->Base.sWord[snIndex] > 0x7F) {                   \
            WLOG(fprintf(pLogFile2, "<%x>", (int)pWord->Base.sWord[snIndex]);)                          \
        }                                                                                               \
        else {                                                                                          \
            WLOG(fprintf(pLogFile2, "%c", (char)pWord->Base.sWord[snIndex]);)                           \
        }                                                                                               \
    }                                                                                                   \
                                                                                                        \
    WLOG(fprintf(pLogFile2, "\n");)                                                                     \
}                                                                                                       \

#else
#define __ET9AWLogWord(pWord, pcComment)
#endif

/** \internal
 * Log a byte array.
 * If debug is active.
 *
 * @param pArray                    The array.
 * @param snLen                     Length of the array.
 * @param pcComment                 Comment to be printed with the log info.
 *
 * @return None
 */

#ifdef ET9_SPC_LOG_HELP_FUNC

#define __ET9AWLogByteArray(pArray, snLen, cComment)                                                    \
{                                                                                                       \
    ET9INT snIndex;                                                                                     \
                                                                                                        \
    WLOG(fprintf(pLogFile2, "%s : ", cComment);)                                                        \
                                                                                                        \
    for (snIndex = 0; snIndex < snLen; ++snIndex) {                                                     \
        WLOG(fprintf(pLogFile2, "%02d ", pArray[snIndex]);)                                             \
    }                                                                                                   \
                                                                                                        \
    WLOG(fprintf(pLogFile2, "\n");)                                                                     \
}                                                                                                       \

#else
#define __ET9AWLogByteArray(pArray, snLen, pcComment)
#endif

/** \internal
 * Calculate edit distance.
 * Function for (optimized) edit distance calculation.
 * If ok the word will be assigned distance, frequency etc.<br>
 * This is a static function that will be "expanded" with different code depending on
 * swithces and definitions of required macros for accessing the word source.
 *
 * @param pLingInfo                 Pointer to alphabetic information structure.
 * @param pWord                     Word to investigate.
 * @param wIndex                    Input index/position (non zero if partial).
 * @param wLength                   Word length (active input length).
 *
 * @return ET9STATUS_NONE if ok, otherwise ET9STATUS_NO_MATCH.
 */

static ET9STATUS ET9LOCALCALL __ET9AWCalcEditDistance(ET9AWLingInfo     * const pLingInfo,
                                                      ET9AWPrivWordInfo * const pWord,
                                                      const ET9U16              wIndex,
                                                      const ET9U16              wLength)
{
    ET9AWLingCmnInfo * const pLingCmnInfo = pLingInfo->pLingCmnInfo;

    const ET9U8             bSpcMode = pLingCmnInfo->Private.bCurrSpcMode;

    const ET9INT            snLen1 = (ET9INT)wLength;
    const ET9INT            snLen2 = (ET9INT)pWord->Base.wWordLen;
    const ET9INT            snMaxDist = (ET9INT)pLingCmnInfo->Private.bCurrMaxEditDistance;

#ifdef ET9_SPC_ED_USE_COMPARE_CACHE

    ET9U8                   pbAtLenStore[ET9_SPC_ED_CALC_ROWS];
    ET9U8 * const           pbAtLens = &pbAtLenStore[ET9_SPC_ED_CALC_ROWS_CENTER];

    ET9ASPCRowU8 * const    ppbFreqRows = &ASPCCMP.ppbFreqRowStore[ET9_SPC_ED_CALC_ROWS_CENTER];
    ET9ASPCRowU8 * const    ppbCmpResultRows = &ASPCCMP.ppbCmpResultRowStore[ET9_SPC_ED_CALC_ROWS_CENTER];

#endif /* ET9_SPC_ED_USE_COMPARE_CACHE */

    const ET9INT            snLenDiff = snLen2 - snLen1;
    const ET9BOOL           bExactSpcMode = (ET9BOOL)(ET9_SPC_GET_MODE(bSpcMode) == ET9ASPCMODE_EXACT);

    ET9STATUS               wDeferredStemStatus = ET9STATUS_NO_MATCH;

    ET9AssertWLog(pLingCmnInfo->Private.ASpc.bSpcState == ET9_SPC_STATE_STD_INIT_OK);

    /* possibly unused */

    if (wIndex) {};

    /* log parameters */

    WLOG(fprintf(pLogFile2, "\nCalcEditDistance, mode = %d, lendiff = %3d\n", bSpcMode, snLen2-snLen1);)

    __ET9AWLogWord(pWord, "test word");

    /* stats */

    __STAT_INC_TotCalcCount;

    /* too short candidates can be rejected directly */

    if (snLen2 + snMaxDist < snLen1) {
        return ET9STATUS_NO_MATCH;
    }

    /* init cache info */

#ifdef ET9_SPC_ED_USE_COMPARE_CACHE

    __ET9AWSetByteArray(pbAtLenStore, ET9_SPC_ED_CALC_ROWS, 0);

#endif /* ET9_SPC_ED_USE_COMPARE_CACHE */

    /* classic (compare) related rejection and calculation */

    if (snLen2 >= snLen1) {

        ET9INT  snPosIndex;
        ET9BOOL bIsSpcCandidate = 0;

        /* if the candidate is too long for spc we reject unless it can be a classic (with completion) */

        for (snPosIndex = 0; snPosIndex < snLen1; ++snPosIndex) {

            ET9U8 bMatch;

            __ET9AWGetMatchOnlyRM(0, snPosIndex, bMatch);

            if (bMatch == ET9_SPC_ED_MATCH_NONE) {

                __STAT_INC_NoStemMatch;

                if (snLen2 > snLen1 + snMaxDist) {
                    return ET9STATUS_NO_MATCH;
                }
                else {
                    bIsSpcCandidate = 1;
                    break;
                }
            }
        }

        /* do a full classic calculation if not spc */

        if (!bIsSpcCandidate) {

            ET9FREQ xResultFreq;
            ET9U8   bExactStemCost;

            xResultFreq = 1;
            bExactStemCost = 0;

            __STAT_INC_ClassicMatch;

            for (snPosIndex = 0; snPosIndex < snLen1; ++snPosIndex) {

                ET9U8 bMatch;
                ET9U8 bFreq;
                ET9U8 bLocked;

                __ET9AWGetMatchInfoRM(0, snPosIndex, bMatch, bFreq, bLocked);

                ET9AssertWLog(bMatch != ET9_SPC_ED_MATCH_NONE);

                if (bMatch == ET9_SPC_ED_MATCH_FULL) {
                    ++bExactStemCost;
                }

                if (snPosIndex < ET9_SPC_ED_MAX_FREQ_LEN) {

                    __ET9AWCalculateFreq(bFreq, xResultFreq, xResultFreq);

                    WLOG(fprintf(pLogFile2, "%2u freq %3u result %8u\n", snPosIndex, bFreq, xResultFreq);)
                }
            }

            pWord->bEditDistSpc = 0;
            pWord->bEditDistStem = bExactStemCost;
            pWord->bEditDistFree = 0;
            pWord->bEditDistSpcSbt = 0;
            pWord->bEditDistSpcTrp = 0;
            pWord->bEditDistSpcIns = 0;
            pWord->bEditDistSpcDel = 0;

            pWord->Base.wWordCompLen = (ET9U16)(pWord->Base.wWordLen - wLength);

            __ET9AWAssignTapFreq(pWord, xResultFreq, wLength);

            WLOG(fprintf(pLogFile2, "classic match\n");)

            /* a word completion should become a spell correction rather than a stem,
               when completion isn't active */

            if ((!ET9WORDCOMPLETION_MODE(pLingCmnInfo) ||
                 pLingCmnInfo->Private.wWordCompletionPoint > wLength) &&
                ET9_SPC_IS_ACTIVE(bSpcMode) &&
                snLen2 > snLen1 &&
                snLen2 <= snLen1 + snMaxDist) {

                WLOG(fprintf(pLogFile2, "potential stem\n");)

                wDeferredStemStatus = ET9STATUS_NONE;
            }
            else {
                return ET9STATUS_NONE;
            }
        }
    }

    /* after this point everything is a possible spc match only */

    __ET9AWLogCache("after classic");

    if (!ET9_SPC_IS_ACTIVE(bSpcMode)) {
        return ET9STATUS_NO_MATCH;
    }

    /* no completion length from here */

    pWord->Base.wWordCompLen = 0;

    /* quick screening on initial char (search limit) */

    __STAT_INC_PossibleSpcCandidate;

    {
        const ET9U8 bSearchFilter = (ET9U8)ASPC.eSearchFilter;

        if (!ET9_SPC_SEARCH_FILTER_IS_UNFILTERED(bSearchFilter)) {

            const ET9BOOL bSearchFilterExact = ET9_SPC_SEARCH_FILTER_IS_EXACT(bSearchFilter);

            ET9U8  bMatchResult;
            ET9U8  bMatchFreq;
            ET9U8  bMatchLocked;

            __ET9AWGetMatchInfoRM(0, 0, bMatchResult, bMatchFreq, bMatchLocked);

            if (ET9_SPC_ED_IS_MATCH(bSearchFilterExact, bMatchResult)) {
                /* ok */
            }
            else if (ET9_SPC_SEARCH_FILTER_IS_ONE(bSearchFilter)) {

                ET9AssertWLog(bSearchFilterExact || wIndex || pWord->bWordSrc != ET9WORDSRC_LDB);

                return wDeferredStemStatus;
            }
            else if (snLen1 > 1) {

                __ET9AWGetMatchInfoRM(-1, 0, bMatchResult, bMatchFreq, bMatchLocked);

                if (ET9_SPC_ED_IS_MATCH(bSearchFilterExact, bMatchResult)) {
                    /* ok */
                }
                else {

                    ET9AssertWLog(bSearchFilterExact || wIndex || pWord->bWordSrc != ET9WORDSRC_LDB);

                    return wDeferredStemStatus;
                }
            }

            __ET9AWLogCache("after search limit");
        }
    }

    /* screening for within dist */

    __STAT_INC_WithinCandidate;

    ET9AssertWLog(!ET9_SPC_IS_ACTIVE(bSpcMode) || snLen2 <= snLen1 + snMaxDist);

    {
        const ET9INT    snTargetRow = snLen2 - snLen1;
        const ET9INT    snTargetCol = snLen2 - 1;
        const ET9INT    snTargetCost = snMaxDist + 1;
        const ET9INT    snTargetDefaultCost = __ET9Abs(snTargetRow);
        const ET9INT    snTargetCostDiff = snTargetCost - snTargetDefaultCost;
        const ET9INT    snUpperRow =  (snTargetCostDiff-1)/2 + (snTargetRow > 0 ? snTargetRow : 0);
        const ET9INT    snLowerRow = -(snTargetCostDiff-1)/2 + (snTargetRow < 0 ? snTargetRow : 0);

        ET9U8           bCostSpot;

        ET9INT          snCurrUpperRow;
        ET9INT          snCurrLowerRow;

        ET9INT          snRowIndex;
        ET9INT          snColIndex;

        ET9U8           *pbCurrCost;
        ET9U8           *pbCostCount;

        ET9U8           pbLastCostPosStore[ET9_SPC_ED_CALC_ROWS];
        ET9U8 * const   pbLastCostPoss = &pbLastCostPosStore[ET9_SPC_ED_CALC_ROWS_CENTER];

        ET9U8           pbCurrCostStore[ET9_SPC_ED_CALC_ROWS];
        ET9U8 * const   pbCurrCosts = &pbCurrCostStore[ET9_SPC_ED_CALC_ROWS_CENTER];

        ET9U8           pbCostCountStore[ET9_SPC_ED_CALC_ROWS];
        ET9U8 * const   pbCostCounts = &pbCostCountStore[ET9_SPC_ED_CALC_ROWS_CENTER];


        WLOG(fprintf(pLogFile2,
            "\n*** within screening,\n..snTargetRow = %2d\n..snTargetCol = %2d\n..snTargetCost = %2d\n..snTargetDefaultCost = %2d\n..snTargetCostDiff = %2d\n..snUpperRow = %+2d\n..snLowerRow = %+2d\n",
            snTargetRow, snTargetCol, snTargetCost, snTargetDefaultCost, snTargetCostDiff, snUpperRow, snLowerRow);)

        ET9AssertWLog(snUpperRow >= snTargetRow);
        ET9AssertWLog(snLowerRow <= snTargetRow);

        /* init */

        pbCurrCost = &pbCurrCosts[snUpperRow];
        pbCostCount = &pbCostCounts[snUpperRow];

        for (snRowIndex = snUpperRow; snRowIndex >= snLowerRow; --snRowIndex, --pbCurrCost, --pbCostCount) {

            *pbCurrCost = (ET9U8)__ET9Abs(snRowIndex);
            *pbCostCount = (ET9U8)(snTargetCost - __ET9Abs(snRowIndex - snTargetRow) - *pbCurrCost);

            ET9AssertWLog(*pbCurrCost + ET9_SPC_ED_MAX_DISTANCE + 1 >= *pbCostCount);
            ET9AssertWLog(snRowIndex != snTargetRow || snTargetCost == *pbCurrCost + *pbCostCount);

            WLOG(fprintf(pLogFile2, "..initial row %02d, pbCurrCost = %d, pbCostCount = %d\n", snRowIndex, *pbCurrCost, *pbCostCount);)
        }

        pbCurrCosts[snUpperRow + 1] = 0xFF;
        pbCurrCosts[snLowerRow - 1] = 0xFF;

        __ET9AWSetByteArray(pbLastCostPosStore, ET9_SPC_ED_CALC_ROWS, 0xFE);    /* not 0xFF */

        /* run left-to-right up-down */

        snCurrUpperRow = 0;
        snCurrLowerRow = snLowerRow;

        for (snColIndex = 0; snColIndex <= snTargetCol; ++snColIndex) {

            pbCurrCost = &pbCurrCosts[snCurrUpperRow];
            pbCostCount = &pbCostCounts[snCurrUpperRow];

            WLOG(fprintf(pLogFile2, "..@ col %02d, snCurrUpperRow = %+2d, snCurrLowerRow = %+2d\n", snColIndex, snCurrUpperRow, snCurrLowerRow);)

            ET9AssertWLog(snCurrUpperRow <= snUpperRow);
            ET9AssertWLog(snCurrUpperRow >= snTargetRow || snColIndex < snUpperRow);
            ET9AssertWLog(snCurrLowerRow >= snLowerRow);
            ET9AssertWLog(snCurrLowerRow <= snTargetRow);

            for (snRowIndex = snCurrUpperRow; snRowIndex >= snCurrLowerRow; --snRowIndex, --pbCurrCost, --pbCostCount) {

                WLOG(fprintf(pLogFile2, "..@ row %+2d, pbCostCount = %x\n", snRowIndex, *pbCostCount);)

                ET9AssertWLog(*pbCostCount);

                /* do we have context support for a cost increase */

                if (*(pbCurrCost - 1) < *pbCurrCost || *(pbCurrCost + 1) < *pbCurrCost) {
                    continue;
                }

                /* check if the spot has a cost */

                __ET9AWIsCostSpotRM(snRowIndex, snColIndex, bCostSpot);

                /* handle cost info */

                if (bCostSpot) {

                    ++(*pbCurrCost);
                    --(*pbCostCount);

                    WLOG(fprintf(pLogFile2, "..found cost @ %+2d:%02d\n", snRowIndex, snColIndex);)

                    if (!(*pbCostCount)) {

                        WLOG(fprintf(pLogFile2, "..cost counter became zero\n");)

                        if (snRowIndex == snTargetRow) {

                            /* the candidate gets rejected  */

                            WLOG(fprintf(pLogFile2, "candidated rejected (within)\n");)

                            __ET9AWLogCache("after rejection");

                            return wDeferredStemStatus;
                        }

                        if (snRowIndex > snTargetRow) {
                            snCurrUpperRow = snRowIndex - 1;
                            WLOG(fprintf(pLogFile2, "..row %+2d at goal cost, moving snCurrUpperRow\n", snRowIndex);)
                        }

                        if (snRowIndex < snTargetRow) {
                            snCurrLowerRow = snRowIndex + 1;
                            WLOG(fprintf(pLogFile2, "..row %+2d at goal cost, moving snCurrLowerRow\n", snRowIndex);)
                        }
                    } /* cost count */
                } /* cost spot */
            } /* loop rows */

            /* "+" rows have an offset */

            if (snColIndex < snUpperRow) {

                ++snCurrUpperRow;

                WLOG(fprintf(pLogFile2, "....added offset row, moving snCurrUpperRow to %+2d\n", snCurrUpperRow);)
            }

            /* "-" rows have "decreased length" */

            if (snLenDiff > 0 && snColIndex + 2 > snLen1 + snCurrLowerRow) {

                ++snCurrLowerRow;

                WLOG(fprintf(pLogFile2, "....removed short row, moving snCurrLowerRow to %+2d\n", snCurrLowerRow);)
            }

        } /* loop cols */

        /* the candidate passes rejection */

        __ET9AWLogCache("after pass within");
    }

    /* must check for hard locked chars (passes classic above) */

    {
        ET9U8   bMatch;
        ET9U8   bFreq;
        ET9U8   bLocked;
        ET9INT  snPosIndex;

        for (snPosIndex = 0; snPosIndex < snLen1; ++snPosIndex) {

            __ET9AWGetMatchInfoRM(0, snPosIndex, bMatch, bFreq, bLocked);

            if (bLocked != ET9_LOCKED_SYMB_VALUE_AND_POS) {
                break;
            }

            if (bMatch == ET9_SPC_ED_MATCH_NONE) {
                return wDeferredStemStatus;
            }

            if (snPosIndex >= snLen2) {
                return wDeferredStemStatus;
            }
        }
    }

    /* spc candidate - calculate distance and freq */

    __STAT_INC_EdCalcCandidate;

    {
        ET9BOOL         bPromotable = 0;

        ET9INT          snCurrUpperRow;
        ET9INT          snCurrLowerRow;

        ET9INT          snRowIndex;
        ET9INT          snColIndex;

        ET9U8           *pbCurrCost;
        ET9U8           *pbPrevCost;

        ET9U8           pbCurrCostStore[ET9_SPC_ED_CALC_ROWS];
        ET9U8 * const   pbCurrCosts = &pbCurrCostStore[ET9_SPC_ED_CALC_ROWS_CENTER];

        ET9U8           pbPrevCostStore[ET9_SPC_ED_CALC_ROWS];
        ET9U8 * const   pbPrevCosts = &pbPrevCostStore[ET9_SPC_ED_CALC_ROWS_CENTER];

        {
            const ET9INT    snTargetRow = snLen2 - snLen1;
            const ET9INT    snTargetCol = snLen2 - 1;
            const ET9INT    snTargetCost = snMaxDist + 1;
            const ET9INT    snTargetDefaultCost = __ET9Abs(snTargetRow);
            const ET9INT    snTargetCostDiff = snTargetCost - snTargetDefaultCost;
            const ET9INT    snUpperRow =  (snTargetCostDiff-1)/2 + (snTargetRow > 0 ? snTargetRow : 0);
            const ET9INT    snLowerRow = -(snTargetCostDiff-1)/2 + (snTargetRow < 0 ? snTargetRow : 0);

            const ET9INT    snCalculatingFreqLen = ET9_SPC_ED_MAX_FREQ_LEN;
            const ET9INT    snPrevCalculatingFreqLen = snCalculatingFreqLen + 1;

            ET9FREQ         *pxCurrFreq;
            ET9FREQ         *pxPrevFreq;

            ET9FREQ         pxCurrFreqStore[ET9_SPC_ED_CALC_ROWS];
            ET9FREQ * const pxCurrFreqs = &pxCurrFreqStore[ET9_SPC_ED_CALC_ROWS_CENTER];

            ET9FREQ         pxPrevFreqStore[ET9_SPC_ED_CALC_ROWS];
            ET9FREQ * const pxPrevFreqs = &pxPrevFreqStore[ET9_SPC_ED_CALC_ROWS_CENTER];

            ET9U8           *pbCurrSpecial;

            ET9U8           pbCurrSpecialStore[ET9_SPC_ED_CALC_ROWS];
            ET9U8 * const   pbCurrSpecials = &pbCurrSpecialStore[ET9_SPC_ED_CALC_ROWS_CENTER];

            ET9U8           *pbCurrCostSbt;
            ET9U8           *pbPrevCostSbt;

            ET9U8           pbCurrCostStoreSbt[ET9_SPC_ED_CALC_ROWS];
            ET9U8 * const   pbCurrCostsSbt = &pbCurrCostStoreSbt[ET9_SPC_ED_CALC_ROWS_CENTER];

            ET9U8           pbPrevCostStoreSbt[ET9_SPC_ED_CALC_ROWS];
            ET9U8 * const   pbPrevCostsSbt = &pbPrevCostStoreSbt[ET9_SPC_ED_CALC_ROWS_CENTER];

            ET9U8           *pbCurrCostTrp;
            ET9U8           *pbPrevCostTrp;

            ET9U8           pbCurrCostStoreTrp[ET9_SPC_ED_CALC_ROWS];
            ET9U8 * const   pbCurrCostsTrp = &pbCurrCostStoreTrp[ET9_SPC_ED_CALC_ROWS_CENTER];

            ET9U8           pbPrevCostStoreTrp[ET9_SPC_ED_CALC_ROWS];
            ET9U8 * const   pbPrevCostsTrp = &pbPrevCostStoreTrp[ET9_SPC_ED_CALC_ROWS_CENTER];

            ET9U8           *pbCurrCostIns;
            ET9U8           *pbPrevCostIns;

            ET9U8           pbCurrCostStoreIns[ET9_SPC_ED_CALC_ROWS];
            ET9U8 * const   pbCurrCostsIns = &pbCurrCostStoreIns[ET9_SPC_ED_CALC_ROWS_CENTER];

            ET9U8           pbPrevCostStoreIns[ET9_SPC_ED_CALC_ROWS];
            ET9U8 * const   pbPrevCostsIns = &pbPrevCostStoreIns[ET9_SPC_ED_CALC_ROWS_CENTER];

            ET9U8           *pbCurrCostDel;
            ET9U8           *pbPrevCostDel;

            ET9U8           pbCurrCostStoreDel[ET9_SPC_ED_CALC_ROWS];
            ET9U8 * const   pbCurrCostsDel = &pbCurrCostStoreDel[ET9_SPC_ED_CALC_ROWS_CENTER];

            ET9U8           pbPrevCostStoreDel[ET9_SPC_ED_CALC_ROWS];
            ET9U8 * const   pbPrevCostsDel = &pbPrevCostStoreDel[ET9_SPC_ED_CALC_ROWS_CENTER];

            WLOG(fprintf(pLogFile2,
                "\n*** distance calc,\n..snTargetRow = %2d\n..snTargetCol = %2d\n..snUpperRow = %+2d\n..snLowerRow = %+2d\n",
                snTargetRow, snTargetCol, snUpperRow, snLowerRow);)

            ET9AssertWLog(snUpperRow >= snTargetRow);
            ET9AssertWLog(snLowerRow <= snTargetRow);

            /* init */

            {
                ET9U8 bLocked = 0;

                pbCurrCost = &pbCurrCosts[snUpperRow];
                pxCurrFreq = &pxCurrFreqs[snUpperRow];
                pxPrevFreq = &pxPrevFreqs[snUpperRow];

                for (snRowIndex = snUpperRow; snRowIndex >= snLowerRow; --snRowIndex, --pbCurrCost, --pxCurrFreq, --pxPrevFreq) {

                    if (bLocked) {
                        *pbCurrCost = ET9_SPC_ED_MAX_COST;
                    }
                    else {
                        *pbCurrCost = (ET9U8)__ET9Abs(snRowIndex);
                    }

                    *pxCurrFreq = 1;
                    *pxPrevFreq = 1;

                    if (!bLocked && snRowIndex <= 0) {
                        __ET9AWIsLockSpotRM(snRowIndex, 0, bLocked);
                    }

                    WLOG(fprintf(pLogFile2, "..initial row %02d, pbCurrCost = %d\n", snRowIndex, *pbCurrCost);)
                }
            }

            {
                ET9U8 bSpecial = 0;

                pbCurrCostIns = &pbCurrCostsIns[snLowerRow];
                pbCurrCostDel = &pbCurrCostsDel[snLowerRow];
                pbCurrSpecial = &pbCurrSpecials[snLowerRow];

                for (snRowIndex = snLowerRow; snRowIndex <= snUpperRow; ++snRowIndex, ++pbCurrSpecial, ++pbCurrCostIns, ++pbCurrCostDel) {

                    if (snRowIndex > 0) {

                        if (bSpecial != 1) {

                            ET9SYMB sSymb;

                            ET9_SPC_ED_GetWordSymb(snRowIndex - 1, sSymb);

                            bSpecial = _ET9_IS_COMMON_CORRECTION_SYMB(sSymb) ? 2 : 1;
                        }

                        *pbCurrCostIns = (ET9U8)__ET9Abs(snRowIndex);
                        *pbCurrCostDel = 0;
                        *pbCurrSpecial = bSpecial;
                    }
                    else {
                        *pbCurrCostIns = 0;
                        *pbCurrCostDel = (ET9U8)__ET9Abs(snRowIndex);
                        *pbCurrSpecial = 0;
                    }

                    WLOG(fprintf(pLogFile2, "..initial row %02d, pbCurrCostIns = %d, pbCurrSpecial = %d, pbCurrCostDel = %d\n", snRowIndex, *pbCurrCostIns, *pbCurrSpecial, *pbCurrCostDel);)
                }
            }

            pbCurrCosts[snUpperRow + 1] = 0xFF;
            pbCurrCosts[snLowerRow - 1] = 0xFF;

            __ET9AWSetByteArray(pbCurrCostStoreSbt, ET9_SPC_ED_CALC_ROWS, 0);
            __ET9AWSetByteArray(pbCurrCostStoreTrp, ET9_SPC_ED_CALC_ROWS, 0);

            /* run left-to-right up-down */

            snCurrUpperRow = 0;
            snCurrLowerRow = snLowerRow;

            for (snColIndex = 0; snColIndex <= snTargetCol; ++snColIndex) {

                const ET9BOOL bCalculatingFreq = (ET9BOOL)(snColIndex < snCalculatingFreqLen);
                const ET9BOOL bPrevCalculatingFreq = (ET9BOOL)(snColIndex < snPrevCalculatingFreqLen);

                pbCurrCost = &pbCurrCosts[snCurrUpperRow];
                pbPrevCost = &pbPrevCosts[snCurrUpperRow];

                pbCurrCostSbt = &pbCurrCostsSbt[snCurrUpperRow];
                pbPrevCostSbt = &pbPrevCostsSbt[snCurrUpperRow];

                pbCurrCostTrp = &pbCurrCostsTrp[snCurrUpperRow];
                pbPrevCostTrp = &pbPrevCostsTrp[snCurrUpperRow];

                pbCurrCostIns = &pbCurrCostsIns[snCurrUpperRow];
                pbPrevCostIns = &pbPrevCostsIns[snCurrUpperRow];

                pbCurrCostDel = &pbCurrCostsDel[snCurrUpperRow];
                pbPrevCostDel = &pbPrevCostsDel[snCurrUpperRow];

                pxCurrFreq = &pxCurrFreqs[snCurrUpperRow];
                pxPrevFreq = &pxPrevFreqs[snCurrUpperRow];

                pbCurrSpecial = &pbCurrSpecials[snCurrUpperRow];

                WLOG(fprintf(pLogFile2, "..@ col %02d, snCurrUpperRow = %+2d, snCurrLowerRow = %+2d\n", snColIndex, snCurrUpperRow, snCurrLowerRow);)

                ET9AssertWLog(snCurrUpperRow <= snUpperRow);
                ET9AssertWLog(snCurrUpperRow >= snTargetRow || snColIndex < snUpperRow);
                ET9AssertWLog(snCurrLowerRow >= snLowerRow);
                ET9AssertWLog(snCurrLowerRow <= snTargetRow);

                for (snRowIndex = snCurrUpperRow; snRowIndex >= snCurrLowerRow; --snRowIndex, --pbCurrCost, --pbPrevCost, --pxCurrFreq, --pxPrevFreq, --pbCurrSpecial, --pbCurrCostSbt, --pbPrevCostSbt, --pbCurrCostTrp, --pbPrevCostTrp, --pbCurrCostIns, --pbPrevCostIns, --pbCurrCostDel, --pbPrevCostDel) {

                    ET9U8 bLocked;

                    const ET9U8 bPrevCost       = *pbCurrCost;
                    const ET9U8 bPrevCostSbt    = *pbCurrCostSbt;
                    const ET9U8 bPrevCostTrp    = *pbCurrCostTrp;
                    const ET9U8 bPrevCostIns    = *pbCurrCostIns;
                    const ET9U8 bPrevCostDel    = *pbCurrCostDel;
                    const ET9FREQ xPrevFreq     = *pxCurrFreq;
                    const ET9FREQ xPrevPrevFreq = *pxPrevFreq;

                    WLOG(fprintf(pLogFile2, "....@ row %+2d, pbCurrCost = %x, pbPrevCost = %x\n", snRowIndex, *pbCurrCost, *pbPrevCost);)

                    /* substitute/transpose */

                    {
                        ET9U8 bSubTrp;

                        __ET9AWInvestigateSpotRM(snRowIndex, snColIndex, bExactSpcMode, bSubTrp, bLocked);

                        if (bSubTrp == 1) {

                            if (bLocked) {
                                *pbCurrCost = ET9_SPC_ED_MAX_COST;
                            }
                            else {
                                ++(*pbCurrCost);
                                ++(*pbCurrCostSbt);
                            }

                            __ET9AWDefaultFreqRM(*pxCurrFreq, xPrevFreq);

                            *pbCurrSpecial = 1;

                            WLOG(fprintf(pLogFile2, "......found substitution @ %+2d:%02d, locked = %d\n", snRowIndex, snColIndex, bLocked);)
                        }
                        else if (bSubTrp == 2) {

                            *pbCurrCost    = *pbPrevCost + 1;
                            *pbCurrCostSbt = *pbPrevCostSbt;
                            *pbCurrCostTrp = *pbPrevCostTrp + 1;
                            *pbCurrCostIns = *pbPrevCostIns;
                            *pbCurrCostDel = *pbPrevCostDel;

                            __ET9AWTranspositionFreqRM(*pxCurrFreq, xPrevPrevFreq, snRowIndex, snColIndex);

                            *pbCurrSpecial = 1;

                            WLOG(fprintf(pLogFile2, "......found transpose @ %+2d:%02d, locked = %d\n", snRowIndex, snColIndex, bLocked);)
                        }
                        else {

                            __ET9AWSubstitutionFreqRM(*pxCurrFreq, xPrevFreq, snRowIndex, snColIndex);

                            WLOG(fprintf(pLogFile2, "......found match @ %+2d:%02d, locked = %d\n", snRowIndex, snColIndex, bLocked);)
                        }
                    }

                    /* delete */

                    if (*(pbCurrCost + 1) + 1 <= *pbCurrCost && !bLocked) {

                        ET9FREQ xTmpFreq;

                        __ET9AWNoneFreqRM(xTmpFreq, *(pxCurrFreq + 1));

                        if (*(pbCurrCost + 1) + 1 < *pbCurrCost || xTmpFreq > *pxCurrFreq) {

                            *pbCurrCost    = *(pbCurrCost    + 1) + 1;
                            *pbCurrCostSbt = *(pbCurrCostSbt + 1);
                            *pbCurrCostTrp = *(pbCurrCostTrp + 1);
                            *pbCurrCostIns = *(pbCurrCostIns + 1);
                            *pbCurrCostDel = *(pbCurrCostDel + 1) + 1;

                            *pxCurrFreq = xTmpFreq;

                            *pbCurrSpecial = 1;

                            WLOG(fprintf(pLogFile2, "......found delete @ %+2d:%02d\n", snRowIndex, snColIndex);)
                        }
                    }

                    /* insert */

                    if (*(pbCurrCost - 1) + 1 <= *pbCurrCost) {

                        ET9FREQ xTmpFreq;

                        __ET9AWNoneFreqRM(xTmpFreq, *(pxCurrFreq - 1));

                        if (*(pbCurrCost - 1) + 1 < *pbCurrCost || xTmpFreq > *pxCurrFreq) {

                            *pbCurrCost    = *(pbCurrCost    - 1) + 1;
                            *pbCurrCostSbt = *(pbCurrCostSbt - 1);
                            *pbCurrCostTrp = *(pbCurrCostTrp - 1);
                            *pbCurrCostIns = *(pbCurrCostIns - 1) + 1;
                            *pbCurrCostDel = *(pbCurrCostDel - 1);

                            *pxCurrFreq = xTmpFreq;

                            if (*(pbCurrSpecial - 1) == 1) {
                                *pbCurrSpecial = 1;
                            }
                            else {

                                ET9SYMB sSymb;

                                ET9_SPC_ED_GetWordSymb(snColIndex, sSymb);

                                if (_ET9_IS_COMMON_CORRECTION_SYMB(sSymb)) {
                                    *pbCurrSpecial = 2;
                                }
                                else {
                                    *pbCurrSpecial = 1;
                                }
                            }

                            WLOG(fprintf(pLogFile2, "......found insert @ %+2d:%02d\n", snRowIndex, snColIndex);)
                        }
                    }

                    *pbPrevCost = bPrevCost;
                    *pxPrevFreq = xPrevFreq;
                    *pbPrevCostSbt = bPrevCostSbt;
                    *pbPrevCostTrp = bPrevCostTrp;
                    *pbPrevCostIns = bPrevCostIns;
                    *pbPrevCostDel = bPrevCostDel;

                    WLOG(fprintf(pLogFile2, "....row %+2d, new pbCurrCost = %x, pdwCurrFreq = %8u\n", snRowIndex, *pbCurrCost, *pxCurrFreq);)
                }

                /* "+" rows have an offset */

                if (snColIndex < snUpperRow) {

                    ++snCurrUpperRow;

                    WLOG(fprintf(pLogFile2, "..added offset row, moving snCurrUpperRow to %+2d\n", snCurrUpperRow);)
                }

                /* "-" rows have "decreased length" */

                if (snLenDiff > 0 && snColIndex + 2 > snLen1 + snCurrLowerRow) {

                    ++snCurrLowerRow;

                    WLOG(fprintf(pLogFile2, "..removed short row, moving snCurrLowerRow to %+2d\n", snCurrLowerRow);)
                }
            }

            /* within distance? */

            __ET9AWLogCache("after dist calc");

            if (pbCurrCosts[snTargetRow] > snMaxDist) {

                WLOG(fprintf(pLogFile2, "candidated rejected (exceeds max dist)\n");)

                return wDeferredStemStatus;
            }

            /* passed - assign edit distance & freq */

            __STAT_INC_ActualCandidate;

            pWord->bEditDistSpc = pbCurrCosts[snTargetRow];
            pWord->bEditDistFree = 0;
            pWord->bEditDistSpcSbt = pbCurrCostsSbt[snTargetRow];
            pWord->bEditDistSpcTrp = pbCurrCostsTrp[snTargetRow];
            pWord->bEditDistSpcIns = pbCurrCostsIns[snTargetRow];
            pWord->bEditDistSpcDel = pbCurrCostsDel[snTargetRow];

            __ET9AWAssignTapFreq(pWord, pxCurrFreqs[snTargetRow], wLength);

            WLOG(fprintf(pLogFile2, "candidated passed, dist = %d, freq = %d\n", pWord->bEditDistSpc, pWord->xTapFreq);)

            /* make it promotable if only puncts were inserted... */

            if (pbCurrSpecials[snTargetRow] == 2) {
                bPromotable = 1;
            }
        }

        /* calculate exact stem distance */

        {
            const ET9U8     bExactMatch = 1;
            const ET9INT    snTargetRow = snLen2 - snLen1;
            const ET9INT    snTargetCol = snLen2 - 1;

#if 0
            const ET9INT    snTargetCost = ET9_SPC_ED_MAX_DISTANCE + 1;
            const ET9INT    snTargetDefaultCost = __ET9Abs(snTargetRow);
            const ET9INT    snTargetCostDiff = snTargetCost - snTargetDefaultCost;
            const ET9INT    snUpperRow =  (nTargetCostDiff-1)/2 + (snTargetRow > 0 ? snTargetRow : 0);
            const ET9INT    snLowerRow = -(nTargetCostDiff-1)/2 + (snTargetRow < 0 ? snTargetRow : 0);
#else
            const ET9INT    snUpperRow =  ET9_SPC_ED_MAX_DISTANCE;
            const ET9INT    snLowerRow = -ET9_SPC_ED_MAX_DISTANCE;
#endif

            ET9U8           bBestStemDist = 0xFF;

            const ET9INT    snStemPos = snLen1 - 1;


            WLOG(fprintf(pLogFile2,
                "\n*** stem distance calc,\n..snTargetRow = %2d\n..snTargetCol = %2d\n..snUpperRow = %+2d\n..snLowerRow = %+2d\n",
                snTargetRow, snTargetCol, snUpperRow, snLowerRow);)

            if (snTargetRow) {}; /* possibly unused */

            ET9AssertWLog(snUpperRow >= snTargetRow);
            ET9AssertWLog(snLowerRow <= snTargetRow);

            /* init */

            {
                ET9U8 bLocked = 0;

                pbCurrCost = &pbCurrCosts[snUpperRow];

                for (snRowIndex = snUpperRow; snRowIndex >= snLowerRow; --snRowIndex, --pbCurrCost) {

                    if (bLocked) {
                        *pbCurrCost = ET9_SPC_ED_MAX_COST;
                    }
                    else {
                        *pbCurrCost = (ET9U8)__ET9Abs(snRowIndex);
                    }

                    if (!bLocked && snRowIndex <= 0) {
                        __ET9AWIsLockSpotRM(snRowIndex, 0, bLocked);
                    }

                    WLOG(fprintf(pLogFile2, "..initial row %02d, pbCurrCost = %d\n", snRowIndex, *pbCurrCost);)
                }
            }

            pbCurrCosts[snUpperRow + 1] = 0xFF;
            pbCurrCosts[snLowerRow - 1] = 0xFF;

            /* run left-to-right up-down */

            snCurrUpperRow = 0;
            snCurrLowerRow = snLowerRow;

            for (snColIndex = 0; snColIndex <= snTargetCol; ++snColIndex) {

                ET9BOOL bIsCommonCorrectionSymb;

                {
                    ET9SYMB sSymb;

                    ET9_SPC_ED_GetWordSymb(snColIndex, sSymb);

                    bIsCommonCorrectionSymb = (ET9BOOL)_ET9_IS_COMMON_CORRECTION_SYMB(sSymb);
                }

                pbCurrCost = &pbCurrCosts[snCurrUpperRow];
                pbPrevCost = &pbPrevCosts[snCurrUpperRow];

                WLOG(fprintf(pLogFile2, "..@ col %02d, snCurrUpperRow = %+2d, snCurrLowerRow = %+2d\n", snColIndex, snCurrUpperRow, snCurrLowerRow);)

                ET9AssertWLog(snCurrUpperRow <= snUpperRow);
                ET9AssertWLog(snCurrUpperRow >= snTargetRow || snColIndex < snUpperRow);
                ET9AssertWLog(snCurrLowerRow >= snLowerRow);
                ET9AssertWLog(snCurrLowerRow <= snTargetRow);

                for (snRowIndex = snCurrUpperRow; snRowIndex >= snCurrLowerRow; --snRowIndex, --pbCurrCost, --pbPrevCost) {

                    ET9U8 bLocked;

                    const ET9U8 bPrevCost = *pbCurrCost;

                    WLOG(fprintf(pLogFile2, "....@ row %+2d, pbCurrCost = %x, pbPrevCost = %x\n", snRowIndex, *pbCurrCost, *pbPrevCost);)

                    /* substitute/transpose */

                    {
                        ET9U8 bSubTrp;

                        __ET9AWInvestigateSpotRM(snRowIndex, snColIndex, bExactMatch, bSubTrp, bLocked);

                        if (bSubTrp == 1) {
                            if (bLocked) {
                                *pbCurrCost = ET9_SPC_ED_MAX_COST;
                            }
                            else {
                                ++(*pbCurrCost);
                            }

                            WLOG(fprintf(pLogFile2, "......found substitution @ %+2d:%02d, locked = %d\n", snRowIndex, snColIndex, bLocked);)
                        }
                        else if (bSubTrp == 2) {
                            *pbCurrCost = *pbPrevCost + 1;

                            WLOG(fprintf(pLogFile2, "......found transpose @ %+2d:%02d, locked = %d\n", snRowIndex, snColIndex, bLocked);)
                        }
                    }

                    /* delete */

                    if (*(pbCurrCost + 1) + 1 < *pbCurrCost && !bLocked) {
                        *pbCurrCost = *(pbCurrCost + 1) + 1;

                        WLOG(fprintf(pLogFile2, "......found delete @ %+2d:%02d\n", snRowIndex, snColIndex);)
                    }

                    /* insert */

                    if (bIsCommonCorrectionSymb) {
                        if (*(pbCurrCost - 1) + 0 < *pbCurrCost) {
                            *pbCurrCost = *(pbCurrCost - 1) + 0;

                            WLOG(fprintf(pLogFile2, "......found free insert @ %+2d:%02d\n", snRowIndex, snColIndex);)
                        }
                    }
                    else {
                        if (*(pbCurrCost - 1) + 1 < *pbCurrCost) {
                            *pbCurrCost = *(pbCurrCost - 1) + 1;

                            WLOG(fprintf(pLogFile2, "......found std insert @ %+2d:%02d\n", snRowIndex, snColIndex);)
                        }
                    }

                    *pbPrevCost = bPrevCost;

                    WLOG(fprintf(pLogFile2, "....row %+2d, new pbCurrCost = %x\n", snRowIndex, *pbCurrCost);)

                    /* look for best stem dist (if at the stem pos) */

                    if (snColIndex - snRowIndex == snStemPos) {
                        if (bBestStemDist > *pbCurrCost) {
                            bBestStemDist = *pbCurrCost;
                        }
                    }
                }

                /* "+" rows have an offset */

                if (snColIndex < snUpperRow) {

                    ++snCurrUpperRow;

                    WLOG(fprintf(pLogFile2, "..added offset row, moving snCurrUpperRow to %+2d\n", snCurrUpperRow);)
                }

                /* "-" rows have "decreased length" */

                if (snLenDiff > 0 && snColIndex + 2 > snLen1 + snCurrLowerRow) {

                    ++snCurrLowerRow;

                    WLOG(fprintf(pLogFile2, "..removed short row, moving snCurrLowerRow to %+2d\n", snCurrLowerRow);)
                }
            }

            /* promotion? */

            if (bPromotable && snLenDiff >= 0) {    /* the len diff test is a temporary solution to prevent the assert */

                ET9AssertWLog(snLenDiff >= 0);
                ET9AssertWLog(pWord->bEditDistSpc);

                if (pWord->bEditDistSpc) {

                    pWord->bEditDistFree = pWord->bEditDistSpc;
                    pWord->bEditDistSpc = 0;
                    pWord->bEditDistSpcSbt = 0;
                    pWord->bEditDistSpcTrp = 0;
                    pWord->bEditDistSpcIns = 0;
                    pWord->bEditDistSpcDel = 0;
                    pWord->Base.wWordCompLen = 0xFFFF;
                }
            }

            /* assign stem distance */

            pWord->bEditDistStem = bBestStemDist;

            WLOG(fprintf(pLogFile2, "stem dist = %d\n", pWord->bEditDistStem);)
        }
    }

    /* done */

    return ET9STATUS_NONE;
}

/*! @} */
/* ----------------------------------< eof >--------------------------------- */
