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
;**     FileName: et9kdb.c                                                    **
;**                                                                           **
;**  Description: Keyboard Database input module module for ET9               **
;**                                                                           **
;*******************************************************************************
;******************************************************************************/

/*! \addtogroup et9imkdb Functions for XT9 Keyboard Module
* Keyboard database input for generic XT9.
* @{
*/

#include "et9kdb.h"
#include "et9imu.h"
#include "et9misc.h"
#include "et9sym.h"
#include "et9kbdef.h"

#ifdef ET9_KDB_MODULE


#ifdef ET9_DEBUGLOG5
#ifdef _WIN32
#pragma message ("*** ET9_DEBUGLOG5 ACTIVATED ***")
#endif
#include <stdio.h>
#include <string.h>
#define WLOG5(q) { if (pLogFile5 == NULL) { pLogFile5 = fopen("zzzET9KDB.txt", "w"); } { q fflush(pLogFile5); } }
static FILE *pLogFile5 = NULL;
#else
#define WLOG5(q)
#endif

#ifdef ET9_DEBUGLOG5B
#ifdef _WIN32
#pragma message ("*** ET9_DEBUGLOG5B ACTIVATED ***")
#endif
#define WLOG5B(q) WLOG5(q)
#else
#define WLOG5B(q)
#endif


#ifdef ET9_DEBUGLOG6
#ifdef _WIN32
#pragma message ("*** ET9_DEBUGLOG6 ACTIVATED ***")
#endif
#include <stdio.h>
#include <string.h>
#define WLOG6(q) { if (pLogFile6 == NULL) { pLogFile6 = fopen("zzzET9KDBTRACE.txt", "w"); } { q fflush(pLogFile6); } }
static FILE *pLogFile6 = NULL;
#else
#define WLOG6(q)
#endif

#ifdef ET9_DEBUGLOG6B
#ifdef _WIN32
#pragma message ("*** ET9_DEBUGLOG6B ACTIVATED ***")
#endif
#define WLOG6B(q) WLOG6(q)
#else
#define WLOG6B(q)
#endif


#ifdef ET9_DIRECT_KDB_ACCESS
#ifdef _WIN32
#pragma message ("*** USING DIRECT KDB ACCESS ***")
#endif
#endif


/* ************************************************************************************************************** */
/* * PROTOTYPES ************************************************************************************************* */
/* ************************************************************************************************************** */

#ifdef ET9_KDB_TRACE_MODULE
extern const ET9U8 _pbXt9Trace[];
#endif

/* ************************************************************************************************************** */
/* * DEFINES / CONSTANTS **************************************************************************************** */
/* ************************************************************************************************************** */

#ifndef ET9CHAR
#define ET9CHAR                     char
#endif

#define UNDEFINED_KDB_PAGE          0xFFFE                          /**< \internal a value that indicates that there is no KDB page loaded (don't use '-1' = 0xFFFF) */

#define ET9_KDB_MAX_REGIONS         ET9MAXBASESYMBS                 /**< \internal xxx */

#define ET9_KDB_KEY_UNDEFINED       0xFFFF                          /**< \internal xxx */
#define ET9_KDB_KEY_OUTOFBOUND      0xFFFE                          /**< \internal xxx */

#define ET9_KDB_COORD_OUTOFBOUND    0xF000                          /**< \internal xxx */

static const ET9FLOAT               fPiR = 3.14159265358979323846f; /**< \internal xxx */

static const ET9FLOAT               fPosRound = 0.5f;               /**< \internal xxx */
static const ET9FLOAT               fSizeRound = 0.1f;              /**< \internal xxx */

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 */

typedef struct ET9DirectedPos_s
{
    ET9TracePoint   sPos;           /**< xxx */
    ET9TracePoint   sL1;            /**< xxx */
    ET9TracePoint   sL2;            /**< xxx */
    ET9BOOL         bForceGeneric;  /**< xxx */
} ET9DirectedPos;                   /**< xxx */

#define __InitDirectedPos(pDirectedPos)  { _ET9ClearMem((ET9U8*)pDirectedPos, sizeof(ET9DirectedPos)); }

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 */

typedef struct ET9MatchKey_s
{
    ET9FLOAT        fFreq;          /**< xxx */
    ET9U8           bFreq;          /**< xxx */
    ET9BOOL         bCenter;        /**< xxx */
    ET9UINT         nOverlap;       /**< xxx */
    ET9KdbAreaInfo  *pArea;         /**< xxx */
} ET9MatchKey;                      /**< xxx */

/*---------------------------------------------------------------------------*/
/** \internal
 * Symbol load actions.
 */

typedef enum ET9LOADSYMBACTION_e {

    LOADSYMBACTION_NEW,             /**< \internal create a new symbol */
    LOADSYMBACTION_RELOAD,          /**< \internal reload the existing symbol */
    LOADSYMBACTION_APPEND,          /**< \internal append to the existing symbol */

    LOADSYMBACTION_LAST             /**< \internal sentinel */
} ET9LOADSYMBACTION;

/* ************************************************************************************************************** */
/* * PROTOTYPES ************************************************************************************************* */
/* ************************************************************************************************************** */

static void ET9LOCALCALL __CalculateOverlap_Generic (ET9KDBInfo  * const pKDBInfo);

static ET9STATUS ET9LOCALCALL __KeyAreasUpdate_StaticUtil (ET9KDBInfo       * const pKDBInfo);

static ET9STATUS ET9LOCALCALL __KDBLoadPage(ET9KDBInfo      * const pKDBInfo,
                                            const ET9U16            wKdbNum,
                                            const ET9U16            wPageNum,
                                            ET9U16          * const pwTotalKeys);

static void ET9LOCALCALL __ProcessMultitap(ET9KDBInfo            * const pKDBInfo,
                                           ET9WordSymbInfo       * const pWordSymbInfo,
                                           ET9BOOL                       bIsFirstPress,
                                           ET9SYMB               * const psFunctionKey,
                                           const ET9U8                   bCurrIndexInList,
                                           const ET9U16                  wMTLastInput,
                                           const ET9LOADSYMBACTION       eLoadAction);

static void ET9LOCALCALL __ProcessWordSymInfo(ET9KDBInfo        * const pKDBInfo,
                                              ET9WordSymbInfo   * const pWordSymbInfo);

/* ************************************************************************************************************** */
/* ************************************************************************************************************** */
/* ************************************************************************************************************** */

/*---------------------------------------------------------------------------*/
/** \internal
 * Check for existence of pKDBInfo AND good init settings
 *
 * @param pKDBInfo     Pointer to keyboard information structure.
 * @param bKDBCheck    Indicator of whether to check KDB init status or not.
 *
 * @return ET9STATUS from call.
 */

static ET9STATUS ET9LOCALCALL __ET9KDB_BasicValidityCheck(ET9KDBInfo    * const pKDBInfo,
                                                          const ET9U8           bKDBCheck)
{
    if (!pKDBInfo) {
        return ET9STATUS_INVALID_MEMORY;
    }

    if (pKDBInfo->Private.wInfoInitOK != ET9GOODSETUP) {
        return ET9STATUS_NO_INIT;
    }

    if (pKDBInfo->Private.bIsLoadingKDB) {
        return ET9STATUS_KDB_IS_LOADING;
    }

    if (bKDBCheck && pKDBInfo->Private.wKDBInitOK != ET9GOODSETUP) {
        return ET9STATUS_NO_DB_INIT;
    }

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Check for existence of good load state
 *
 * @param pKDBInfo     Pointer to keyboard information structure.
 *
 * @return ET9STATUS from call.
 */

static ET9STATUS ET9LOCALCALL __ET9KDB_LoadValidityCheck(ET9KDBInfo    * const pKDBInfo)
{
    if (!pKDBInfo) {
        return ET9STATUS_INVALID_MEMORY;
    }

    if (pKDBInfo->Private.wInfoInitOK != ET9GOODSETUP) {
        return ET9STATUS_NO_INIT;
    }

    if (!pKDBInfo->Private.bIsLoadingKDB) {
        return ET9STATUS_KDB_IS_NOT_LOADING;
    }

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Uses call-back to set multitap timer.
 *
 * @param pKDBInfo       Pointer to keyboard information structure.
 * @param pWordSymbInfo  Pointer to current wordsymb structure.
 * @param wKey           Key to label timer.
 *
 * @return ET9STATUS from call.
 */

static ET9STATUS ET9LOCALCALL __ET9KDB_RequestMultitapTimeout(ET9KDBInfo        * const pKDBInfo,
                                                              ET9WordSymbInfo   * const pWordSymbInfo,
                                                              const ET9U16              wKey)
{
    ET9KDB_Request sRequest;

    ET9Assert(pKDBInfo);

    sRequest.eType = ET9_KDB_REQ_TIMEOUT;
    sRequest.data.sTimeout.eTimerType = ET9_KDB_TMRMULT;
    sRequest.data.sTimeout.nTimerID   = (ET9INT)(wKey + 1);

    return pKDBInfo->ET9Handle_KDB_Request(pKDBInfo, pWordSymbInfo, &sRequest);
}

/* ************************************************************************************************************** */
/* * DIACRITICS ************************************************************************************************* */
/* ************************************************************************************************************** */

/*---------------------------------------------------------------------------*/
/** \internal
 * JIS diacritic table - works for FW hiragana and katakana (using different offsets).
 */

static const ET9U8 __JIS_DiacriticTable_FW[] =
{
    /*      0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F */
    /*A*/   2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  4,  2,  4,  2,  4,
    /*B*/   2,  4,  2,  4,  2,  4,  2,  4,  2,  4,  2,  4,  2,  4,  2,  4,
    /*C*/   2,  4,  2,  4,  8,  2,  4,  2,  4,  2,  2,  2,  2,  2,  2,  4,
    /*D*/   8,  2,  4,  8,  2,  4,  8,  2,  4,  8,  2,  4,  8,  2,  2,  2,
    /*E*/   2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,
    /*F*/   2,  2,  2,  2,  8,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1
};

#define JIS_CLASS_OTHER         1
#define JIS_CLASS_NONE          2
#define JIS_CLASS_DAKUTEN       4
#define JIS_CLASS_HANDAKUTEN    8

/*---------------------------------------------------------------------------*/
/** \internal
 * JIS filter modes.
 */

typedef enum JIS_FILTERMODE_e {

    JIS_FILTERMODE_ALL,             /**< \internal all symbols */
    JIS_FILTERMODE_NONE_INC,        /**< \internal all but dakuten and handakuten */
    JIS_FILTERMODE_NONE,            /**< \internal only non diacritic (when has dakuten) */
    JIS_FILTERMODE_DAKUTEN,         /**< \internal only dakuten */
    JIS_FILTERMODE_HANDAKUTEN,      /**< \internal only handakuten */

    JIS_FILTERMODE_LAST             /**< \internal sentinel */
} JIS_FILTERMODE;

/*---------------------------------------------------------------------------*/
/** \internal
 * This function gets the diacritic class for a symb.
 *
 * @param sSymb                Symbol to be examined.
 *
 * @return Class ID.
 */

ET9INLINE static ET9U8 ET9LOCALCALL __JIS_DiacriticGetClass(const ET9SYMB   sSymb)
{
    if (sSymb >= 0x829F && sSymb <= 0x82FE) {
        return __JIS_DiacriticTable_FW[sSymb - 0x829F];
    }
    else if (sSymb >= 0x8340 && sSymb <= 0x839F) {
        return __JIS_DiacriticTable_FW[sSymb - 0x8340];
    }
    else {
        return JIS_CLASS_OTHER;
    }
}

/*---------------------------------------------------------------------------*/
/** \internal
 * This function performs default diacritic filtering for shift-jis.
 *
 * @param pFilterSymbInfo      Pointer to filter related object.
 * @param sSymb                Symbol to be examined.
 *
 * @return ET9STATUS_NONE to allow, otherwise suppress.
 */

static ET9STATUS ET9LOCALCALL __DefaultDiacriticFilterShiftJis(void         * const pFilterSymbInfo,
                                                               const ET9SYMB        sSymb)
{
    ET9KDBInfo   * const pKDBInfo = (ET9KDBInfo*)pFilterSymbInfo;

    ET9BOOL         bAllow;
    const ET9U8     bClass = __JIS_DiacriticGetClass(sSymb);

    ET9Assert(pFilterSymbInfo);

    /* filter */

    switch (pKDBInfo->Private.bCurrDiacriticState)
    {
        default:
        case JIS_FILTERMODE_ALL:
            bAllow = 1;
            break;
        case JIS_FILTERMODE_NONE_INC:
            bAllow = (ET9BOOL)(bClass & (JIS_CLASS_NONE | JIS_CLASS_OTHER));
            break;
        case JIS_FILTERMODE_NONE:
            bAllow = (ET9BOOL)(bClass & JIS_CLASS_NONE);
            break;
        case JIS_FILTERMODE_DAKUTEN:
            bAllow = (ET9BOOL)(bClass & JIS_CLASS_DAKUTEN);
            break;
        case JIS_FILTERMODE_HANDAKUTEN:
            bAllow = (ET9BOOL)(bClass & JIS_CLASS_HANDAKUTEN);
            break;
    }

    /* done */

    return bAllow ? ET9STATUS_NONE : ET9STATUS_ERROR;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * This function performs default diacritic filter reset for shift-jis.
 *
 * @param pFilterSymbInfo      Pointer to filter related object.
 *
 * @return None
 */

static void ET9LOCALCALL __DefaultDiacriticFilterResetShiftJis(void    * const pFilterSymbInfo)
{
    ET9KDBInfo   * const pKDBInfo = (ET9KDBInfo*)pFilterSymbInfo;

    ET9Assert(pFilterSymbInfo);

    if (ET9_KDB_MULTITAP_MODE(pKDBInfo->dwStateBits)) {
        pKDBInfo->Private.bCurrDiacriticState = (ET9U8)JIS_FILTERMODE_NONE_INC;
    }
    else {
        pKDBInfo->Private.bCurrDiacriticState = (ET9U8)JIS_FILTERMODE_ALL;
    }
}

/*---------------------------------------------------------------------------*/
/** \internal
 * This function performs default diacritic filter reset for shift-jis.
 *
 * @param pFilterSymbInfo      Pointer to filter related object.
 * @param pbFilterCount        Pointer to the number of available filters.
 *
 * @return ET9STATUS_NONE, otherwise error.
 */

static ET9STATUS ET9LOCALCALL __DefaultDiacriticFilterCountShiftJis(void    * const pFilterSymbInfo,
                                                                    ET9U8   * const pbFilterCount)
{
    ET9KDBInfo   * const pKDBInfo = (ET9KDBInfo*)pFilterSymbInfo;

    ET9Assert(pFilterSymbInfo);
    ET9Assert(pbFilterCount);

    if (ET9_KDB_MULTITAP_MODE(pKDBInfo->dwStateBits)) {
        *pbFilterCount = 4;
    }
    else {
        *pbFilterCount = 3;
    }

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * This function performs default diacritic filter reset for shift-jis.
 *
 * @param pFilterSymbInfo      Pointer to filter related object.
 *
 * @return ET9STATUS_NONE, otherwise error.
 */

static ET9STATUS ET9LOCALCALL __DefaultDiacriticFilterNextShiftJis(void    * const pFilterSymbInfo)
{
    ET9KDBInfo   * const pKDBInfo = (ET9KDBInfo*)pFilterSymbInfo;

    JIS_FILTERMODE eMode = (JIS_FILTERMODE)pKDBInfo->Private.bCurrDiacriticState;

    ET9Assert(pFilterSymbInfo);

    if (ET9_KDB_MULTITAP_MODE(pKDBInfo->dwStateBits)) {
        switch (eMode)
        {
            case JIS_FILTERMODE_NONE_INC:
                eMode = JIS_FILTERMODE_DAKUTEN;
                break;
            case JIS_FILTERMODE_DAKUTEN:
                eMode = JIS_FILTERMODE_HANDAKUTEN;
                break;
            case JIS_FILTERMODE_HANDAKUTEN:
                eMode = JIS_FILTERMODE_NONE;
                break;
            default:
            case JIS_FILTERMODE_NONE:
                eMode = JIS_FILTERMODE_NONE_INC;
                break;
        }
    }
    else {
        switch (eMode)
        {
            case JIS_FILTERMODE_ALL:
                eMode = JIS_FILTERMODE_DAKUTEN;
                break;
            case JIS_FILTERMODE_DAKUTEN:
                eMode = JIS_FILTERMODE_HANDAKUTEN;
                break;
            default:
            case JIS_FILTERMODE_HANDAKUTEN:
                eMode = JIS_FILTERMODE_ALL;
                break;
        }
    }

    pKDBInfo->Private.bCurrDiacriticState = (ET9U8)eMode;

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * This function performs default diacritic grouping for shift-jis.
 *
 * @param pFilterSymbInfo       Pointer to filter related object.
 * @param sSymb1                First symbol.
 * @param sSymb2                Second symbol.
 *
 * @return Non zero if the same group, otherwise zero.
 */

static ET9BOOL ET9LOCALCALL __DefaultDiacriticFilterGroupShiftJis(void              * const pFilterSymbInfo,
                                                                  const ET9SYMB             sSymb1,
                                                                  const ET9SYMB             sSymb2)
{
    ET9SYMB sLow;
    ET9SYMB sHigh;

    ET9_UNUSED(pFilterSymbInfo);

    if (sSymb1 == sSymb2) {
        return 1;
    }
    else if (sSymb1 < sSymb2) {
        sLow = sSymb1;
        sHigh = sSymb2;
    }
    else {
        sLow = sSymb2;
        sHigh = sSymb1;
    }

    {
        const ET9U8   bClassLow = __JIS_DiacriticGetClass(sLow);
        const ET9U8   bClassHigh = __JIS_DiacriticGetClass(sHigh);

        return (ET9BOOL)(bClassLow == JIS_CLASS_NONE    && bClassHigh == JIS_CLASS_DAKUTEN    && (sLow + 1) == sHigh ||
                         bClassLow == JIS_CLASS_NONE    && bClassHigh == JIS_CLASS_HANDAKUTEN && (sLow + 2) == sHigh ||
                         bClassLow == JIS_CLASS_DAKUTEN && bClassHigh == JIS_CLASS_HANDAKUTEN && (sLow + 1) == sHigh);
    }
}

/*---------------------------------------------------------------------------*/
/** \internal
 * This function performs a filter reset.
 * It will also preempt the current diacritic nexting.
 *
 * @param pKDBInfo             Pointer to keyboard information structure.
 *
 * @return None
 */

ET9INLINE static void ET9LOCALCALL __FilterSymbReset(ET9KDBInfo   * const pKDBInfo)
{
    ET9Assert(pKDBInfo);

    pKDBInfo->Private.sKdbAction.dwCurrChecksum = 0;

    if (pKDBInfo->Private.pFilterSymbReset) {

        pKDBInfo->Private.pFilterSymbReset(pKDBInfo->Private.pFilterSymbInfo);
    }
}

/* ************************************************************************************************************** */
/* * UTIL ******************************************************************************************************* */
/* ************************************************************************************************************** */

/*---------------------------------------------------------------------------*/
/** \internal
 * This function mirrors some WSI state info into KDB info.
 *
 * @param pKDBInfo             Pointer to keyboard information structure.
 * @param pWordSymbInfo        Pointer to tap info.
 *
 * @return None
 */

static void ET9LOCALCALL __MirrorWsiState(ET9KDBInfo           * const pKDBInfo,
                                          ET9WordSymbInfo      * const pWordSymbInfo)
{
    if (ET9SHIFT_MODE(pWordSymbInfo->dwStateBits)) {
        pKDBInfo->Private.eShiftState = ET9SHIFT;
    }
    else if (ET9CAPS_MODE(pWordSymbInfo->dwStateBits)) {
        pKDBInfo->Private.eShiftState = ET9CAPSLOCK;
    }
    else {
        pKDBInfo->Private.eShiftState = ET9NOSHIFT;
    }
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Calculate a checksum for a word symb info (input).
 *
 * @param pWordSymbInfo            Pointer to tap info.
 *
 * @return Checksum (zero indicated a bad symb).
 */

static ET9U32 ET9LOCALCALL __CalculateLastWordSymbChecksum(ET9WordSymbInfo   * const pWordSymbInfo)
{
    ET9U8                   bBaseCount;
    ET9DataPerBaseSym       *pDPBS;
    ET9U32                  dwHashValue = 0;
    const ET9U8             bNumSymbs = pWordSymbInfo->bNumSymbs;
    ET9SymbInfo     * const pSymbInfo = pWordSymbInfo->SymbsInfo + bNumSymbs - 1;

    ET9Assert(pWordSymbInfo);

    /* check if it's a valid symbol */

    if (!bNumSymbs || !pSymbInfo->bNumBaseSyms || !pSymbInfo->DataPerBaseSym->bNumSymsToMatch) {
        return 0;
    }

    /* mark position */

    dwHashValue = bNumSymbs + (65599 * dwHashValue);

    /* calculate */

    pDPBS = pSymbInfo->DataPerBaseSym;
    for (bBaseCount = pSymbInfo->bNumBaseSyms; bBaseCount; --bBaseCount, ++pDPBS) {

        ET9U8       bCharCount;
        ET9SYMB     *psChar;
        ET9SYMB     *psUpperChar;

        psChar = pDPBS->sChar;
        psUpperChar = pDPBS->sUpperCaseChar;
        for (bCharCount = pDPBS->bNumSymsToMatch; bCharCount; --bCharCount, ++psChar, ++psUpperChar) {
            dwHashValue = *psChar + (65599 * dwHashValue);
            dwHashValue = *psUpperChar + (65599 * dwHashValue);
        }
    }

    /* must be non zero - very unlikely, but anyway */

    if (!dwHashValue) {
        dwHashValue = 1;
    }

    /* done */

    return dwHashValue;
}

#ifdef ET9_KDB_TRACE_MODULE

/*---------------------------------------------------------------------------*/
/** \internal
 * This function converts an X coordinate from integration space to internal (KDB) space.
 *
 * @param pKDBInfo             Pointer to keyboard information structure.
 * @param fX                   Integration space coordinate.
 *
 * @return Scaled coordinate.
 */

ET9INLINE static ET9FLOAT ET9LOCALCALL __ScaleCoordinateToKdbX_f(ET9KDBInfo           * const pKDBInfo,
                                                                 const ET9FLOAT               fX)
{
    if (!pKDBInfo->Private.wScaleToLayoutWidth) {
        return fX - pKDBInfo->Private.wLayoutOffsetX;
    }
    else {

        const ET9FLOAT fLayoutWidth  = (ET9FLOAT)pKDBInfo->Private.pCurrLayoutInfo->wLayoutWidth;
        const ET9FLOAT fScaleToLayoutWidth  = (ET9FLOAT)pKDBInfo->Private.wScaleToLayoutWidth;

        return (fX - pKDBInfo->Private.wLayoutOffsetX) * fLayoutWidth / fScaleToLayoutWidth;
    }
}

#endif

/*---------------------------------------------------------------------------*/
/** \internal
 * This function converts an X coordinate from integration space to internal (KDB) space.
 *
 * @param pKDBInfo             Pointer to keyboard information structure.
 * @param nX                   Integration space coordinate.
 *
 * @return Scaled coordinate.
 */

ET9INLINE static ET9U32 ET9LOCALCALL __ScaleCoordinateToKdbX(ET9KDBInfo           * const pKDBInfo,
                                                             const ET9UINT                nX)
{
    if (nX < pKDBInfo->Private.wLayoutOffsetX) {
        return ET9_KDB_COORD_OUTOFBOUND;
    }
    else if (!pKDBInfo->Private.wScaleToLayoutWidth) {
        return nX - pKDBInfo->Private.wLayoutOffsetX;
    }
    else {

        const ET9U32 dwLayoutWidth  = pKDBInfo->Private.pCurrLayoutInfo->wLayoutWidth;
        const ET9U32 dwScaleToLayoutWidth  = pKDBInfo->Private.wScaleToLayoutWidth;

        const ET9U32 dwNumX = (nX - pKDBInfo->Private.wLayoutOffsetX) * dwLayoutWidth;
        const ET9U32 dwRestX = dwNumX % dwScaleToLayoutWidth;
        const ET9U32 dwX = (dwNumX / dwScaleToLayoutWidth ) + (dwRestX > (dwScaleToLayoutWidth / 2) ? 1 : 0);

        return dwX;
    }
}

#ifdef ET9_KDB_TRACE_MODULE

/*---------------------------------------------------------------------------*/
/** \internal
 * This function converts a Y coordinate from integration space to internal (KDB) space.
 *
 * @param pKDBInfo             Pointer to keyboard information structure.
 * @param fY                   Integration space coordinate.
 *
 * @return Scaled coordinate.
 */

ET9INLINE static ET9FLOAT ET9LOCALCALL __ScaleCoordinateToKdbY_f(ET9KDBInfo           * const pKDBInfo,
                                                                 const ET9FLOAT               fY)
{
    if (!pKDBInfo->Private.wScaleToLayoutHeight) {
        return fY - pKDBInfo->Private.wLayoutOffsetY;
    }
    else {

        const ET9FLOAT fLayoutHeight = (ET9FLOAT)pKDBInfo->Private.pCurrLayoutInfo->wLayoutHeight;
        const ET9FLOAT fScaleToLayoutHeight = (ET9FLOAT)pKDBInfo->Private.wScaleToLayoutHeight;

        return (fY - pKDBInfo->Private.wLayoutOffsetY) * fLayoutHeight / fScaleToLayoutHeight;
    }
}

#endif

/*---------------------------------------------------------------------------*/
/** \internal
 * This function converts a Y coordinate from integration space to internal (KDB) space.
 *
 * @param pKDBInfo             Pointer to keyboard information structure.
 * @param nY                   Integration space coordinate.
 *
 * @return Scaled coordinate.
 */

ET9INLINE static ET9U32 ET9LOCALCALL __ScaleCoordinateToKdbY(ET9KDBInfo           * const pKDBInfo,
                                                             const ET9UINT                nY)
{
    if (nY < pKDBInfo->Private.wLayoutOffsetY) {
        return ET9_KDB_COORD_OUTOFBOUND;
    }
    else if (!pKDBInfo->Private.wScaleToLayoutHeight) {
        return nY - pKDBInfo->Private.wLayoutOffsetY;
    }
    else {

        const ET9U32 dwLayoutHeight = pKDBInfo->Private.pCurrLayoutInfo->wLayoutHeight;
        const ET9U32 dwScaleToLayoutHeight = pKDBInfo->Private.wScaleToLayoutHeight;

        const ET9U32 dwNumY = (nY - pKDBInfo->Private.wLayoutOffsetY) * dwLayoutHeight;
        const ET9U32 dwRestY = dwNumY % dwScaleToLayoutHeight;
        const ET9U32 dwY = (dwNumY / dwScaleToLayoutHeight) + (dwRestY > (dwScaleToLayoutHeight / 2) ? 1 : 0);

        return dwY;
    }
}

#ifdef ET9_KDB_TRACE_MODULE

/*---------------------------------------------------------------------------*/
/** \internal
 * This function converts a X coordinate from internal (KDB) space to integration space.
 *
 * @param pKDBInfo             Pointer to keyboard information structure.
 * @param fX                   Internal (KDB) space coordinate.
 *
 * @return Scaled coordinate.
 */

ET9INLINE static ET9FLOAT ET9LOCALCALL __ScaleCoordinateToIntegrationX_f(ET9KDBInfo           * const pKDBInfo,
                                                                         const ET9FLOAT               fX)
{
    if (!pKDBInfo->Private.wScaleToLayoutWidth) {
        return fX + pKDBInfo->Private.wLayoutOffsetX;
    }
    else {

        const ET9FLOAT fLayoutWidth = (ET9FLOAT)pKDBInfo->Private.pCurrLayoutInfo->wLayoutWidth;
        const ET9FLOAT fScaleToLayoutWidth = (ET9FLOAT)pKDBInfo->Private.wScaleToLayoutWidth;

        return (fX * fScaleToLayoutWidth / fLayoutWidth) + pKDBInfo->Private.wLayoutOffsetX;
    }
}

#endif

/*---------------------------------------------------------------------------*/
/** \internal
 * This function converts a X coordinate from internal (KDB) space to integration space.
 *
 * @param pKDBInfo             Pointer to keyboard information structure.
 * @param nX                   Internal (KDB) space coordinate.
 *
 * @return Scaled coordinate.
 */

ET9INLINE static ET9U32 ET9LOCALCALL __ScaleCoordinateToIntegrationX(ET9KDBInfo           * const pKDBInfo,
                                                                     const ET9UINT                nX)
{
    if (!pKDBInfo->Private.wScaleToLayoutWidth) {
        return nX + pKDBInfo->Private.wLayoutOffsetX;
    }
    else {

        const ET9U32 dwLayoutWidth  = pKDBInfo->Private.pCurrLayoutInfo->wLayoutWidth;
        const ET9U32 dwScaleToLayoutWidth  = pKDBInfo->Private.wScaleToLayoutWidth;

        const ET9U32 dwNumX = nX * dwScaleToLayoutWidth;
        const ET9U32 dwRestX = dwNumX % dwLayoutWidth;
        const ET9U32 dwX = (dwNumX / dwLayoutWidth ) + (dwRestX > (dwLayoutWidth / 2) ? 1 : 0);

        return dwX + pKDBInfo->Private.wLayoutOffsetX;
    }
}

#ifdef ET9_KDB_TRACE_MODULE

/*---------------------------------------------------------------------------*/
/** \internal
 * This function converts a Y coordinate from internal (KDB) space to integration space.
 *
 * @param pKDBInfo             Pointer to keyboard information structure.
 * @param fY                   Internal (KDB) space coordinate.
 *
 * @return Scaled coordinate.
 */

ET9INLINE static ET9FLOAT ET9LOCALCALL __ScaleCoordinateToIntegrationY_f(ET9KDBInfo           * const pKDBInfo,
                                                                         const ET9FLOAT               fY)
{
    if (!pKDBInfo->Private.wScaleToLayoutHeight) {
        return fY + pKDBInfo->Private.wLayoutOffsetY;
    }
    else {

        const ET9FLOAT fLayoutHeight = (ET9FLOAT)pKDBInfo->Private.pCurrLayoutInfo->wLayoutHeight;
        const ET9FLOAT fScaleToLayoutHeight = (ET9FLOAT)pKDBInfo->Private.wScaleToLayoutHeight;

        return (fY * fScaleToLayoutHeight / fLayoutHeight) + pKDBInfo->Private.wLayoutOffsetY;
    }
}

#endif

/*---------------------------------------------------------------------------*/
/** \internal
 * This function converts a Y coordinate from internal (KDB) space to integration space.
 *
 * @param pKDBInfo             Pointer to keyboard information structure.
 * @param nY                   Internal (KDB) space coordinate.
 *
 * @return Scaled coordinate.
 */

ET9INLINE static ET9U32 ET9LOCALCALL __ScaleCoordinateToIntegrationY(ET9KDBInfo           * const pKDBInfo,
                                                                     const ET9UINT                nY)
{
    if (!pKDBInfo->Private.wScaleToLayoutHeight) {
        return nY + pKDBInfo->Private.wLayoutOffsetY;
    }
    else {

        const ET9U32 dwLayoutHeight = pKDBInfo->Private.pCurrLayoutInfo->wLayoutHeight;
        const ET9U32 dwScaleToLayoutHeight = pKDBInfo->Private.wScaleToLayoutHeight;

        const ET9U32 dwNumY = nY * dwScaleToLayoutHeight;
        const ET9U32 dwRestY = dwNumY % dwLayoutHeight;
        const ET9U32 dwY = (dwNumY / dwLayoutHeight) + (dwRestY > (dwLayoutHeight / 2) ? 1 : 0);

        return dwY + pKDBInfo->Private.wLayoutOffsetY;
    }
}

#ifdef ET9_KDB_TRACE_MODULE

/*---------------------------------------------------------------------------*/
/** \internal
 * This function converts a point from internal (KDB) space to integration space.
 *
 * @param pKDBInfo             Pointer to keyboard information structure.
 * @param pPointSrc            Internal
 * @param pPointDst            Integration
 *
 * @return Scaled coordinate.
 */

ET9INLINE static ET9TracePoint_f* ET9LOCALCALL __ScaleToIntegration_f(ET9KDBInfo               * const pKDBInfo,
                                                                      ET9TracePoint_f    const * const pPointSrc,
                                                                      ET9TracePoint_f          * const pPointDst)
{
    pPointDst->fX = __ScaleCoordinateToIntegrationX_f(pKDBInfo, pPointSrc->fX);
    pPointDst->fY = __ScaleCoordinateToIntegrationY_f(pKDBInfo, pPointSrc->fY);

    return pPointDst;
}

#endif

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pTpL11                    .
 * @param pTpL12                    .
 * @param pTpL21                    .
 * @param pTpL22                    .
 *
 * @return Xxx
 */

ET9INLINE static ET9FLOAT ET9LOCALCALL __LinesAngle (ET9TracePoint_f const * const pTpL11,
                                                     ET9TracePoint_f const * const pTpL12,
                                                     ET9TracePoint_f const * const pTpL21,
                                                     ET9TracePoint_f const * const pTpL22)
{
    const ET9FLOAT fL1X = pTpL11->fX - pTpL12->fX;
    const ET9FLOAT fL1Y = pTpL11->fY - pTpL12->fY;
    const ET9FLOAT fL2X = pTpL21->fX - pTpL22->fX;
    const ET9FLOAT fL2Y = pTpL21->fY - pTpL22->fY;

    const ET9FLOAT fY = fL1X * fL2Y - fL1Y * fL2X;
    const ET9FLOAT fX = fL1X * fL2X + fL1Y * fL2Y;

    const ET9FLOAT fAngleR = _ET9atan2_f(fY, fX);

    const ET9FLOAT fAngleD = (ET9FLOAT)(fAngleR * 180.0 / fPiR);

    return fAngleD;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * This function sorts an U16 array.
 *
 * @param pwArray      Arrau to be sorted.
 * @param nCount       Number of items in the array.
 *
 * @return None
 */

static void ET9LOCALCALL __SortU16Array (ET9U16 * const pwArray, const ET9UINT nCount)
{
    ET9UINT nDirty;
    ET9UINT nIndex;

    for (nDirty = 1; nDirty;) {

        nDirty = 0;

        for (nIndex = 0; nIndex + 1 < nCount; ++nIndex) {

            if (pwArray[nIndex] > pwArray[nIndex + 1]) {

                const ET9U16 nTmp = pwArray[nIndex];
                pwArray[nIndex] = pwArray[nIndex + 1];
                pwArray[nIndex + 1] = nTmp;

                nDirty = 1;
            }
        }
    }
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Get region overlap, if any.
 *
 * @param pRegion1         Xxx
 * @param pRegion2         Xxx
 *
 * @return Overlapping area.
 */

ET9INLINE static ET9UINT ET9LOCALCALL __GetRegionOverlap(ET9Region const * const pRegion1,
                                                         ET9Region const * const pRegion2)
{
    if (pRegion1->wRight  < pRegion2->wLeft ||
        pRegion2->wRight  < pRegion1->wLeft ||
        pRegion1->wBottom < pRegion2->wTop  ||
        pRegion2->wBottom < pRegion1->wTop) {

        return 0;
    }
    else {

        const ET9UINT nOverlapX = __ET9Min(pRegion1->wRight,  pRegion2->wRight)  - __ET9Max(pRegion1->wLeft, pRegion2->wLeft) + 1;
        const ET9UINT nOverlapY = __ET9Min(pRegion1->wBottom, pRegion2->wBottom) - __ET9Max(pRegion1->wTop,  pRegion2->wTop)  + 1;

        return nOverlapX * nOverlapY;
    }
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Get region overlap, if any.
 *
 * @param pRegion          Xxx
 * @param nX               Xxx
 * @param nY               Xxx
 * @param nW               Xxx
 * @param nH               Xxx
 *
 * @return None
 */

ET9INLINE static void ET9LOCALCALL __CreateRegion(ET9Region     * const pRegion,
                                                  const ET9UINT         nX,
                                                  const ET9UINT         nY,
                                                  const ET9UINT         nW,
                                                  const ET9UINT         nH)
{
    const ET9UINT nMax = 0xFFFF;

    ET9UINT nLeft;
    ET9UINT nTop;
    ET9UINT nRight;
    ET9UINT nBottom;

    nLeft   = nX - (nW / 2);
    nTop    = nY - (nH / 2);
    nRight  = nLeft + nW - 1;
    nBottom = nTop  + nH - 1;

    if (nLeft > nX) {
        nLeft = 0;
    }

    if (nTop > nY) {
        nTop = 0;
    }

    if (nRight < nLeft) {
        nRight = nMax;
    }

    if (nBottom < nTop) {
        nBottom = nMax;
    }

    pRegion->wLeft   = (ET9U16)(nLeft   < nMax ? nLeft   : nMax);
    pRegion->wTop    = (ET9U16)(nTop    < nMax ? nTop    : nMax);
    pRegion->wRight  = (ET9U16)(nRight  < nMax ? nRight  : nMax);
    pRegion->wBottom = (ET9U16)(nBottom < nMax ? nBottom : nMax);
}

/* ************************************************************************************************************** */
/* * DYNAMIC **************************************************************************************************** */
/* ************************************************************************************************************** */

/*---------------------------------------------------------------------------*/
/** \internal
 * This function will calculate the median key size.
 *
 * @param pKDBInfo     Pointer to keyboard information structure.
 *
 * @return None
 */

static void ET9LOCALCALL __CalculateKeySize_Dynamic (ET9KDBInfo  * const pKDBInfo)
{
    ET9KdbLayoutInfo * const pLayoutInfo = pKDBInfo->Private.pCurrLayoutInfo;

    ET9U16 pwWidths[ET9_KDB_MAX_KEYS];
    ET9U16 pwHeights[ET9_KDB_MAX_KEYS];

    ET9UINT nCount;
    ET9UINT nItemCount;
    ET9U16 *pwWidth;
    ET9U16 *pwHeight;
    ET9KdbAreaInfo *pArea;

    WLOG5(fprintf(pLogFile5, "__CalculateKeySize_Dynamic\n");)

    nItemCount = 0;
    pwWidth = &pwWidths[0];
    pwHeight = &pwHeights[0];
    pArea = pLayoutInfo->pKeyAreas;
    for (nCount = pLayoutInfo->nKeyAreaCount; nCount; --nCount, ++pArea) {

        if (pArea->eKeyType == ET9KTFUNCTION) {
            continue;
        }

        *pwWidth = pArea->sRegion.wRight - pArea->sRegion.wLeft + 1;
        *pwHeight = pArea->sRegion.wBottom - pArea->sRegion.wTop + 1;

        ++pwWidth;
        ++pwHeight;
        ++nItemCount;
    }

    if (nItemCount) {

        __SortU16Array(pwWidths, nItemCount);
        __SortU16Array(pwHeights, nItemCount);

        pLayoutInfo->nMinKeyWidth = pwWidths[0];
        pLayoutInfo->nMinKeyHeight = pwHeights[0];

        pLayoutInfo->nMedianKeyWidth = pwWidths[nItemCount / 2];
        pLayoutInfo->nMedianKeyHeight = pwHeights[nItemCount / 2];
    }
    else {

        WLOG5(fprintf(pLogFile5, "  no valid keys\n");)

        pLayoutInfo->nMinKeyWidth = 10;
        pLayoutInfo->nMinKeyHeight = 10;

        pLayoutInfo->nMedianKeyWidth = 10;
        pLayoutInfo->nMedianKeyHeight = 10;
    }

    WLOG5(fprintf(pLogFile5, "  nMinKeyWidth %2u, nMinKeyHeight %2u, nMedianKeyWidth %2u, nMedianKeyHeight %2u\n", pLayoutInfo->nMinKeyWidth, pLayoutInfo->nMinKeyHeight, pLayoutInfo->nMedianKeyWidth, pLayoutInfo->nMedianKeyHeight);)
}

/*---------------------------------------------------------------------------*/
/** \internal
 * This function will calculate the radius.
 *
 * @param pKDBInfo     Pointer to keyboard information structure.
 *
 * @return None
 */

static void ET9LOCALCALL __CalculateRadius_Dynamic (ET9KDBInfo  * const pKDBInfo)
{
    ET9KdbLayoutInfo * const pLayoutInfo = pKDBInfo->Private.pCurrLayoutInfo;

    ET9FLOAT pfMaxRadiusSQ[ET9KT_LAST];

    ET9UINT nCount;
    ET9UINT nIndex;
    ET9KdbAreaInfo *pArea;

    WLOG5(fprintf(pLogFile5, "__CalculateRadius_Dynamic\n");)

    for (nIndex = 0; nIndex < ET9KT_LAST; ++nIndex) {
        pfMaxRadiusSQ[nIndex] = 1;
    }

    pArea = pLayoutInfo->pKeyAreas;
    for (nCount = pLayoutInfo->nKeyAreaCount; nCount; --nCount, ++pArea) {

        ET9Region sThisArea;

        ET9UINT nNeighborCount;
        ET9KdbAreaInfo *pNeighborArea;

        __CreateRegion(&sThisArea, pArea->nCenterX, pArea->nCenterY, pLayoutInfo->nMedianKeyWidth * 2 - 1, pLayoutInfo->nMedianKeyHeight * 2 - 1);

        WLOG5(fprintf(pLogFile5, "  key '%c' - neighbors ", pArea->psChars[0]);)

        if (pArea->eKeyType == ET9KTFUNCTION) {
            continue;
        }

        pNeighborArea = pLayoutInfo->pKeyAreas;
        for (nNeighborCount = pLayoutInfo->nKeyAreaCount; nNeighborCount; --nNeighborCount, ++pNeighborArea) {

            if (pNeighborArea == pArea) {
                continue;
            }

            if (pNeighborArea->eKeyType != pArea->eKeyType) {
                continue;
            }

            if (!((sThisArea.wRight  >= pNeighborArea->sRegion.wLeft && pNeighborArea->sRegion.wRight  >= sThisArea.wLeft) &&
                  (sThisArea.wBottom >= pNeighborArea->sRegion.wTop  && pNeighborArea->sRegion.wBottom >= sThisArea.wTop))) {
                continue;
            }

            WLOG5(fprintf(pLogFile5, "%c", pNeighborArea->psChars[0]);)

            {
                const ET9FLOAT fDistanceSQ = ((ET9FLOAT)pArea->nCenterX - (ET9FLOAT)pNeighborArea->nCenterX) *
                                             ((ET9FLOAT)pArea->nCenterX - (ET9FLOAT)pNeighborArea->nCenterX) +
                                             ((ET9FLOAT)pArea->nCenterY - (ET9FLOAT)pNeighborArea->nCenterY) *
                                             ((ET9FLOAT)pArea->nCenterY - (ET9FLOAT)pNeighborArea->nCenterY);

                if (pfMaxRadiusSQ[pArea->eKeyType] < fDistanceSQ) {
                    pfMaxRadiusSQ[pArea->eKeyType] = fDistanceSQ;
                }
            }
        }

        WLOG5(fprintf(pLogFile5, "\n");)
    }

    WLOG5(
        for (nIndex = 0; nIndex < ET9KT_LAST; ++nIndex) {
            fprintf(pLogFile5, "  radius[%u] %5.1f\n", nIndex, _ET9sqrt_f(pfMaxRadiusSQ[nIndex]));
        })

    {
        const ET9FLOAT fMaxRadiusSQ = (pfMaxRadiusSQ[ET9KTLETTER] > 1) ? pfMaxRadiusSQ[ET9KTLETTER] :
                                      (pfMaxRadiusSQ[ET9KTPUNCTUATION] > 1) ? pfMaxRadiusSQ[ET9KTPUNCTUATION] :
                                      (pfMaxRadiusSQ[ET9KTSMARTPUNCT] > 1) ? pfMaxRadiusSQ[ET9KTSMARTPUNCT] :
                                      (pfMaxRadiusSQ[ET9KTNUMBER] > 1) ? pfMaxRadiusSQ[ET9KTNUMBER] :
                                      (pfMaxRadiusSQ[ET9KTSTRING] > 1) ? pfMaxRadiusSQ[ET9KTSTRING] :
                                      pfMaxRadiusSQ[ET9KTFUNCTION];

        const ET9FLOAT fRadius = _ET9sqrt_f(fMaxRadiusSQ);

        pLayoutInfo->nRadius = (ET9UINT)fRadius;

        if (pLayoutInfo->nRadius < fRadius) {
            ++pLayoutInfo->nRadius;
        }

        WLOG5(fprintf(pLogFile5, "  radius %u (%5.1f)\n", pLayoutInfo->nRadius, fRadius);)
    }
}

/*---------------------------------------------------------------------------*/
/** \internal
 * This function will post process the loaded KDB data.
 *
 * @param pKDBInfo     Pointer to keyboard information structure.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

static ET9STATUS ET9LOCALCALL __KDBLoadPage_PostProcess_Dynamic(ET9KDBInfo  * const pKDBInfo)
{
    WLOG5(fprintf(pLogFile5, "__KDBLoadPage_PostProcess_Dynamic\n");)

    if (!pKDBInfo->Private.pCurrLayoutInfo->nKeyAreaCount) {
        return ET9STATUS_KDB_PAGE_HAS_NO_KEYS;
    }

    __CalculateKeySize_Dynamic(pKDBInfo);

    __CalculateRadius_Dynamic(pKDBInfo);

    __CalculateOverlap_Generic(pKDBInfo);

    ET9Assert(pKDBInfo->Private.pCurrLayoutInfo->nBoxWidth);
    ET9Assert(pKDBInfo->Private.pCurrLayoutInfo->nBoxHeight);
    ET9Assert(pKDBInfo->Private.pCurrLayoutInfo->nMinKeyWidth);
    ET9Assert(pKDBInfo->Private.pCurrLayoutInfo->nMinKeyHeight);
    ET9Assert(pKDBInfo->Private.pCurrLayoutInfo->nMedianKeyWidth);
    ET9Assert(pKDBInfo->Private.pCurrLayoutInfo->nMedianKeyHeight);

    ET9Assert(pKDBInfo->Private.pCurrLayoutInfo->nRadius);

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * This function loads the specified page (in wPageNum).
 * It will also assure the correct kdb.
 *
 * @param pKDBInfo     Pointer to keyboard information structure.
 * @param wKdbNum      Keyboard number.
 * @param wPageNum     Page number to load.
 * @param pwTotalKeys  Pointer to total keys.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

static ET9STATUS ET9LOCALCALL __KDBLoadPage_Dynamic(ET9KDBInfo      * const pKDBInfo,
                                                    const ET9U16            wKdbNum,
                                                    const ET9U16            wPageNum,
                                                    ET9U16          * const pwTotalKeys)
{
    ET9STATUS eStatus;

    /* applicable? */

    if (!pKDBInfo->pKDBLoadData) {
        return ET9STATUS_READ_DB_FAIL;
    }

    if (!wKdbNum) {
        return ET9STATUS_INVALID_KDB_NUM;
    }

    /* check cache */

    {
        ET9UINT nCount;
        ET9KdbLayoutInfo *pLayoutInfo;

        pLayoutInfo = &pKDBInfo->Private.pLayoutInfos[0];
        for (nCount = ET9_KDB_MAX_PAGE_CACHE; nCount; --nCount, ++pLayoutInfo) {

            if (pLayoutInfo->bOk && pLayoutInfo->bDynamic && pLayoutInfo->wKdbNum == wKdbNum && pLayoutInfo->wPageNum == wPageNum) {

                WLOG5(fprintf(pLogFile5, "__KDBLoadPage_Dynamic, using cache, wKdbNum %2x, wPageNum %2u\n", pLayoutInfo->wKdbNum, pLayoutInfo->wPageNum);)

                if (pKDBInfo->Private.pCurrLayoutInfo != pLayoutInfo) {
                    ++pKDBInfo->Private.nLoadID;
                }

                pKDBInfo->Private.pCurrLayoutInfo = pLayoutInfo;

                pKDBInfo->Private.bKDBLoaded = 1;
                pKDBInfo->Private.bUsingDynamicKDB = 1;

                pKDBInfo->wKdbNum = pLayoutInfo->wKdbNum;
                pKDBInfo->wTotalPages = pLayoutInfo->wTotalPages;

                pKDBInfo->Private.wPageNum = wPageNum;
                pKDBInfo->Private.wPageKeyNum = (ET9U16)pKDBInfo->Private.pCurrLayoutInfo->nKeyAreaCount;

                pKDBInfo->Private.wLayoutWidth = pKDBInfo->Private.pCurrLayoutInfo->wLayoutWidth;
                pKDBInfo->Private.wLayoutHeight = pKDBInfo->Private.pCurrLayoutInfo->wLayoutHeight;

                if (pwTotalKeys){
                    *pwTotalKeys = pKDBInfo->Private.wPageKeyNum;
                }

                return ET9STATUS_NONE;
            }
        }
    }

    WLOG5(fprintf(pLogFile5, "__KDBLoadPage_Dynamic, actual load, wKdbNum %2x, wPageNum %2u\n", wKdbNum, wPageNum);)

    /* pick cache */

    ++pKDBInfo->Private.pLastLayoutInfo;

    if (pKDBInfo->Private.pLastLayoutInfo < pKDBInfo->Private.pLayoutInfos ||
        pKDBInfo->Private.pLastLayoutInfo - pKDBInfo->Private.pLayoutInfos >= ET9_KDB_MAX_PAGE_CACHE) {
        pKDBInfo->Private.pLastLayoutInfo = &pKDBInfo->Private.pLayoutInfos[0];
    }

    pKDBInfo->Private.pCurrLayoutInfo = pKDBInfo->Private.pLastLayoutInfo;

    /* actual load */

    ++pKDBInfo->Private.nLoadID;

    pKDBInfo->Private.pCurrLayoutInfo->bOk = 0;
    pKDBInfo->Private.pCurrLayoutInfo->bDynamic = 1;

    pKDBInfo->wKdbNum = wKdbNum;                                        /* this is the identfier for what to read in through the callback */
    pKDBInfo->wTotalPages = 0;

    pKDBInfo->Private.pCurrLayoutInfo->wKdbNum = wKdbNum;
    pKDBInfo->Private.pCurrLayoutInfo->wPageNum = wPageNum;

    pKDBInfo->Private.bKDBLoaded = 0;
    pKDBInfo->Private.bIsLoadingKDB = 1;
    pKDBInfo->Private.bUsingDynamicKDB = 1;

    eStatus = ET9KDB_Load_Reset(pKDBInfo);

    if (eStatus) {
        pKDBInfo->Private.bIsLoadingKDB = 0;
        return eStatus;
    }

    eStatus = pKDBInfo->pKDBLoadData(pKDBInfo, wKdbNum, wPageNum);

    pKDBInfo->Private.bIsLoadingKDB = 0;

    if (eStatus) {
        return eStatus;
    }

    eStatus = __KDBLoadPage_PostProcess_Dynamic(pKDBInfo);

    if (eStatus) {
        return eStatus;
    }

    /* validate */

    {
        const ET9U8 bExpectedPrimary = (ET9U8)(wKdbNum & PKDBID_MASK);
        const ET9U8 bExpectedSecondary = (ET9U8)((wKdbNum & SKDBID_MASK) >> 8);

        /* check primary id */

        if (bExpectedPrimary != pKDBInfo->Private.pCurrLayoutInfo->bPrimaryID) {
            return ET9STATUS_KDB_ID_MISMATCH;
        }

        /* check secondary id */

        if (bExpectedSecondary != pKDBInfo->Private.pCurrLayoutInfo->bSecondaryID) {
            return ET9STATUS_KDB_ID_MISMATCH;
        }
    }

    /* loaded ok */

    {
        ET9KdbLayoutInfo * const pLayoutInfo = pKDBInfo->Private.pCurrLayoutInfo;

        pLayoutInfo->bOk = 1;

        pKDBInfo->wKdbNum = pLayoutInfo->wKdbNum;
        pKDBInfo->wTotalPages = pLayoutInfo->wTotalPages;

        pKDBInfo->Private.bKDBLoaded = 1;
        pKDBInfo->Private.wPageNum = wPageNum;
        pKDBInfo->Private.wPageKeyNum = (ET9U16)pLayoutInfo->nKeyAreaCount;

        pKDBInfo->Private.wLayoutWidth = pLayoutInfo->wLayoutWidth;
        pKDBInfo->Private.wLayoutHeight = pLayoutInfo->wLayoutHeight;
    }

    if (pwTotalKeys) {
        *pwTotalKeys = pKDBInfo->Private.wPageKeyNum;
    }

    /* done */

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Validates the integrity of a Keyboard Database.
 *
 * @param[in]     pKDBInfo          Pointer to the Keyboard Input Module Information Data Structure (ET9KDBInfo).
 */

static ET9STATUS ET9LOCALCALL __KDBValidate_Dynamic(ET9KDBInfo  * const pKDBInfo)
{
    ET9STATUS eStatus;

    ET9U16 wPageNum;
    ET9U16 wTotalKeys;
    ET9U16 wTotalPages;

    /* applicable? */

    if (!pKDBInfo->pKDBLoadData) {
        return ET9STATUS_INVALID_KDB_NUM;
    }

    WLOG5(fprintf(pLogFile5, "__KDBValidate_Dynamic, wKdbNum %2x\n", pKDBInfo->wKdbNum);)

    /* loop pages
       - missing KDB should return ET9STATUS_INVALID_KDB_NUM
       - other errors are passed through */

    wTotalPages = 1;
    for (wPageNum = 0; wPageNum < wTotalPages; ++wPageNum) {

        eStatus = __KDBLoadPage_Dynamic(pKDBInfo, pKDBInfo->wKdbNum, wPageNum, &wTotalKeys);

        if (eStatus) {
            return eStatus;
        }

        if (!wTotalKeys) {
            return ET9STATUS_KDB_PAGE_HAS_NO_KEYS;
        }

        if (!wPageNum) {
            wTotalPages = pKDBInfo->wTotalPages;
        }
        else if (wTotalPages != pKDBInfo->wTotalPages) {
            return ET9STATUS_KDB_INCONSISTENT_PAGE_COUNT;
        }
    }

    /* done */

    return ET9STATUS_NONE;
}

/* ************************************************************************************************************** */
/* * STATIC ***************************************************************************************************** */
/* ************************************************************************************************************** */

/*---------------------------------------------------------------------------*/
/** \internal
 * Returns the checksum of a piece of the KDB.
 *
 * @param pKDBInfo         Pointer to keyboard information structure.
 * @param pdwHashValue     In/out value with the hash to be updated.
 * @param dwPos            Byte position of start of data to read.
 * @param dwSize           # of bytes to read (can be over big for "read to end").
 *
 * @return None
 */

static void ET9LOCALCALL __HashChunk_Static(ET9KDBInfo      * const pKDBInfo,
                                            ET9U32          * const pdwHashValue,
                                            const ET9U32            dwPos,
                                            const ET9U32            dwSize)
{

#ifdef ET9_DIRECT_KDB_ACCESS

    ET9U8               *pStr;
    ET9U32              dwCount;
    ET9U32              dwHashValue;

    ET9Assert(pKDBInfo);
    ET9Assert(pdwHashValue);
    ET9Assert(dwSize);
    ET9Assert(pKDBInfo->Private.dwKdbDataSize);
    ET9Assert(pKDBInfo->Private.pKdbData);

    dwCount = dwSize;

    if (dwCount >= pKDBInfo->Private.dwKdbDataSize - dwPos) {
        dwCount = pKDBInfo->Private.dwKdbDataSize - dwPos;
    }

    /* hash data chunk */

    pStr = &pKDBInfo->Private.pKdbData[dwPos];

    dwHashValue = *pdwHashValue;

    while (dwCount--) {
        dwHashValue = *(pStr++) + (65599 * dwHashValue);
    }

    *pdwHashValue = dwHashValue;

#else /* ET9_DIRECT_KDB_ACCESS */

    ET9STATUS   eStatus;
    ET9U8       byHashDBBuff[ET9HASHKDBBUFFSIZE];
    ET9U8       *pStr;
    ET9U32      dwUnReadSize = ET9HASHKDBBUFFSIZE;
    ET9U32      dwNumberOfBytesRead;
    ET9U32      dwNum;
    ET9U32      dwHashValue;
    ET9U32      dwHashPos = dwPos;
    ET9U32      dwHashSize = dwSize;

    ET9Assert(pKDBInfo);
    ET9Assert(pdwHashValue);
    ET9Assert(dwSize);

    while (dwHashSize) {

        if (ET9HASHKDBBUFFSIZE > dwHashSize) {
            dwUnReadSize = dwHashSize;
        }
        else {
            dwUnReadSize = ET9HASHKDBBUFFSIZE;
        }

        eStatus = pKDBInfo->ET9KDBReadData(pKDBInfo, dwHashPos, dwUnReadSize, byHashDBBuff, &dwNumberOfBytesRead);

        /* Check for end-of-file (or other failure). */

        if (eStatus || !dwNumberOfBytesRead) {
            break;
        }

        /* Hash KDB data */

        pStr = byHashDBBuff;
        dwNum = dwNumberOfBytesRead;
        dwHashValue = *pdwHashValue;
        while (dwNum--) {
            dwHashValue = *(pStr++) + (65599 * dwHashValue);
        }
        *pdwHashValue = dwHashValue;
        dwHashSize -= dwNumberOfBytesRead;
        dwHashPos += dwNumberOfBytesRead;
    }

#endif /* ET9_DIRECT_KDB_ACCESS */
}

/*---------------------------------------------------------------------------*/
/** \internal
 * This function computes checksum for the KDB content.
 *
 * @param pKDBInfo             Pointer to keyboard information structure.
 *
 * @return Checksum
 */

static ET9U32 ET9LOCALCALL __ComputeContentChecksum_Static(ET9KDBInfo   * const pKDBInfo)
{
    ET9U8        bTotalSubRegionKeys;
    ET9U32       i, j, k;
    ET9U8        byAmbig;
    ET9U16       wOffset;
    ET9U32       dwOffset;
    ET9U32       dwKeyOffset = 0;
    ET9U32       dwChecksum;
    ET9U8        bType;

    ET9Assert(pKDBInfo);
    ET9Assert(!pKDBInfo->Private.bUsingDynamicKDB);

    dwChecksum = 0;

    for (i = 0; i < pKDBInfo->Private.PageHeader.bTotalRegions; ++i) {

        /* get region header offset */
        __ET9KDBREADDWORD(
            pKDBInfo->Private.dwPageHdrOffset +
            ET9KDB_PAGEHDROFFSET_REGIONOFFSET + 4*i,
            &pKDBInfo->Private.PageHeader.dwRegionHdrOffset);

        /* get total sub-regions */
        __ET9KDBGETDATA(
            pKDBInfo->Private.PageHeader.dwRegionHdrOffset +
            ET9KDB_REGHDROFFSET_TOTALREGIONS, 1,
            &pKDBInfo->Private.RegionHeader.bTotalRegions);

        for (j = 0; j < pKDBInfo->Private.RegionHeader.bTotalRegions; ++j) {
            /* get sub-region header offset */
            __ET9KDBREADDWORD(
                pKDBInfo->Private.PageHeader.dwRegionHdrOffset +
                ET9KDB_REGHDROFFSET_REGIONOFFSET + 4*j,
                &pKDBInfo->Private.RegionHeader.dwRegionHdrOffset);

            /* get region total keys */
            __ET9KDBGETDATA(
                pKDBInfo->Private.RegionHeader.dwRegionHdrOffset +
                ET9KDB_REGIONOFFSET_TOTALKEYS, 1, &bTotalSubRegionKeys);

            for (k = 0; k < bTotalSubRegionKeys; k++) {
                __ET9KDBGETDATA(
                    pKDBInfo->Private.RegionHeader.dwRegionHdrOffset +
                    ET9KDB_REGIONOFFSET_AMBIGFLAG, 1,
                    &byAmbig);

                if (!byAmbig) {

                    dwOffset =
                        pKDBInfo->Private.RegionHeader.dwRegionHdrOffset +
                        ET9KDBNAOFFSET_KEYOFFSET + ((ET9U16)k*2);

                    __ET9KDBREADWORD(dwOffset, &wOffset);

                    dwKeyOffset =
                        pKDBInfo->Private.RegionHeader.dwRegionHdrOffset + wOffset;

                    __ET9KDBGETDATA(dwKeyOffset+8, 1, &bType);

                    dwChecksum = bType + (65599 * dwChecksum);
                }
            }
        }
    }

    return dwChecksum;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * This function loads/validates/initializes the keyboard.
 * Will only load the kdb if it's not already the active one.
 *
 * @param pKDBInfo     Pointer to keyboard information structure.
 * @param wKdbNum      KDB number to load.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

static ET9STATUS ET9LOCALCALL __KDBLoad_Static(ET9KDBInfo       * const pKDBInfo,
                                               const ET9U16             wKdbNum)
{
    ET9STATUS eStatus;
    ET9U8     byData;
    ET9U8     byKdbId1;
    ET9U8     byKdbId2;
    ET9U16    wDatabaseType;
    ET9U16    wCompatID;
    ET9U16    wCompatOffset;

    ET9Assert(pKDBInfo);
    ET9Assert(!pKDBInfo->Private.bUsingDynamicKDB);

    /* If no keyboard ID skip loading KDB */

    if (!wKdbNum) {
        return ET9STATUS_INVALID_KDB_NUM;
    }

    if (pKDBInfo->wKdbNum == wKdbNum && pKDBInfo->Private.bKDBLoaded) {
        return ET9STATUS_NONE;
    }

    WLOG5(fprintf(pLogFile5, "__KDBLoad_Static, loading, pKDBInfo = %p, wKdbNum = %04x (%04x), loaded = %d\n", pKDBInfo, wKdbNum, pKDBInfo->wKdbNum, pKDBInfo->Private.bKDBLoaded);)

    pKDBInfo->wKdbNum = wKdbNum;
    pKDBInfo->Private.bKDBLoaded = 0;

    pKDBInfo->Private.wPageNum = UNDEFINED_KDB_PAGE;

    /* init access */

    eStatus = __InitDirectKDBAccess(pKDBInfo);

    if (eStatus) {
        return eStatus;
    }

    /* Layout version check */

    __ET9KDBGETDATA(ET9KDBOFFSET_LAYOUTVER, 1, &byData);

    if (byData < 2) {
        return ET9STATUS_KDB_VERSION_ERROR;                /* this is NOT compatible */
    }

    /* Check the database type */

    __ET9KDBGETDATA(ET9KDBOFFSET_DATABASETYPE, 1, &byData);

    /* Database type should not be greater than ET9MAXDBTYPE. */

    if (byData > ET9MAXDBTYPE) {
        return ET9STATUS_INVALID_DB_TYPE;
    }

    wDatabaseType = byData;

    /* Convert the number to bit mask. */

    wDatabaseType = (ET9U16)(1 << wDatabaseType);

    if (!(wDatabaseType & (ET9DB_KDB_MASK))) {
        return ET9STATUS_INVALID_DB_TYPE;
    }

    /* Compatibility Check */
    /* Get the compatibility ID from KDB */

    __ET9KDBREADWORD(ET9KDBOFFSET_COMPATID, &wCompatID);

    if (wCompatID < ET9COMPATIDKDBXBASEALPH) {
        return ET9STATUS_DB_CORE_INCOMP;
    }

    /* Compute Compatibility index offset */

    wCompatOffset = (ET9U16)(wCompatID - ET9COMPATIDKDBXBASEALPH);

    if (wCompatOffset > ET9MAXCOMPATIDKDBXOFFSET) {
        return ET9STATUS_DB_CORE_INCOMP;
    }

    /* If the compat idx from KDB is equal to the compat idx base defined in core,
     * the offset is 0. Therefore, they are compatible. Otherwise, compare the offset. */

    if (wCompatOffset) {

        /* Comvert the offset to bit mask. */

        wCompatOffset = ET9MASKOFFSET(wCompatOffset);

        if (!(ET9COMPATIDKDBXOFFSETALPH & wCompatOffset)) {
            return ET9STATUS_DB_CORE_INCOMP;
        }
    }

    /* check primary id */

    byKdbId1 = (ET9U8)(wKdbNum & PKDBID_MASK);

    __ET9KDBGETDATA(ET9KDBOFFSET_PRIMARYKEYBOARDID, 1, &byKdbId2);

    if (byKdbId1 != byKdbId2) {
        return ET9STATUS_KDB_VERSION_ERROR;
    }

    /* check secondary id */

    byKdbId1 = (ET9U8)((wKdbNum & SKDBID_MASK) >> 8);

    __ET9KDBGETDATA(ET9KDBOFFSET_SECONDKEYBOARDID, 1, &byKdbId2);

    if (byKdbId1 != byKdbId2) {

        /* this is NOT the requested KDB */

        return ET9STATUS_KDB_VERSION_ERROR;
    }

    /* get total pages */

    __ET9KDBREADWORD(ET9KDBOFFSET_TOTALPAGES, &pKDBInfo->wTotalPages);

    /* get layout width */

    __ET9KDBREADWORD(ET9KDBOFFSET_KDBWIDTH, &pKDBInfo->Private.wLayoutWidth);

    /* get layout height */

    __ET9KDBREADWORD(ET9KDBOFFSET_KDBHEIGHT, &pKDBInfo->Private.wLayoutHeight);

    /* get page ambigous offset value */

    __ET9KDBREADWORD(ET9KDBOFFSET_PAGEARRAYOFFSET, &pKDBInfo->Private.wPageArrayOffset);

    /* loaded ok */

    pKDBInfo->Private.bKDBLoaded = 1;

    /* done */

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * This function loads the specified page (in wPageNum).
 * It will also assure the correct kdb.
 *
 * @param pKDBInfo     Pointer to keyboard information structure.
 * @param wKdbNum      Keyboard number.
 * @param wPageNum     Page number to load.
 * @param pwTotalKeys  Pointer to total keys.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

static ET9STATUS ET9LOCALCALL __KDBLoadPage_Static(ET9KDBInfo       * const pKDBInfo,
                                                   const ET9U16             wKdbNum,
                                                   const ET9U16             wPageNum,
                                                   ET9U16           * const pwTotalKeys)
{
    ET9U8     i;
    ET9U8     j;
    ET9U32    dwOffset;
    ET9U16    wTotalKeys = 0;
    ET9U8     bRegionKeys;
    ET9STATUS eStatus;

    ET9Assert(pKDBInfo);

    /* applicable? */

    if (!pKDBInfo->ET9KDBReadData) {
        return ET9STATUS_READ_DB_FAIL;
    }

    pKDBInfo->Private.bUsingDynamicKDB = 0;

    /* assure kdb loaded */

    eStatus = __KDBLoad_Static(pKDBInfo, wKdbNum);

    if (eStatus) {
        return eStatus;
    }

    /* check if page already loaded */

    if (pKDBInfo->Private.wPageNum == wPageNum) {

        if (pwTotalKeys){
            *pwTotalKeys = pKDBInfo->Private.wPageKeyNum;
        }

        return ET9STATUS_NONE;
    }

    WLOG5(fprintf(pLogFile5, "__KDBLoadPage_Static, loading, pKDBInfo = %p, wKdbNum = %04x, wPageNum = %d (%d)\n", pKDBInfo, wKdbNum, wPageNum, pKDBInfo->Private.wPageNum);)

    /* check page number. start from 1 */

    if (pKDBInfo->wTotalPages <= wPageNum) {
        return ET9STATUS_INVALID_KDB_PAGE;
    }

    /* get page offset value */

    __ET9KDBREADDWORD(pKDBInfo->Private.wPageArrayOffset +
        (ET9U16)((ET9U16)4 * wPageNum),
        &pKDBInfo->Private.dwPageHdrOffset);

    /* get left */

    __ET9KDBREADWORD(pKDBInfo->Private.dwPageHdrOffset + ET9KDB_PAGEHDROFFSET_LEFT, &pKDBInfo->Private.PageHeader.wLeft);

    /* get top */

    __ET9KDBREADWORD(pKDBInfo->Private.dwPageHdrOffset + ET9KDB_PAGEHDROFFSET_TOP, &pKDBInfo->Private.PageHeader.wTop);

    /* get right */

    __ET9KDBREADWORD(pKDBInfo->Private.dwPageHdrOffset + ET9KDB_PAGEHDROFFSET_RIGHT, &pKDBInfo->Private.PageHeader.wRight);

    /* get bottom */

    __ET9KDBREADWORD(pKDBInfo->Private.dwPageHdrOffset + ET9KDB_PAGEHDROFFSET_BOTTOM, &pKDBInfo->Private.PageHeader.wBottom);

    /* get total regions */

    __ET9KDBGETDATA(pKDBInfo->Private.dwPageHdrOffset + ET9KDB_PAGEHDROFFSET_TOTALREGIONS, 1, &pKDBInfo->Private.PageHeader.bTotalRegions);

    for (i = 0; i < pKDBInfo->Private.PageHeader.bTotalRegions; ++i) {

        /* get region header offset */

        __ET9KDBREADDWORD(
            pKDBInfo->Private.dwPageHdrOffset +
            (ET9U32)((ET9U32)ET9KDB_PAGEHDROFFSET_REGIONOFFSET + (ET9U32)4 * (ET9U32)i),
            &pKDBInfo->Private.PageHeader.dwRegionHdrOffset);

        /* get total sub regions */

        __ET9KDBGETDATA(
            pKDBInfo->Private.PageHeader.dwRegionHdrOffset +
            ET9KDB_REGHDROFFSET_TOTALREGIONS, 1,
            &pKDBInfo->Private.RegionHeader.bTotalRegions);

        for (j = 0; j < pKDBInfo->Private.RegionHeader.bTotalRegions; ++j) {

            /* get sub-region header offset */

            __ET9KDBREADDWORD(
                pKDBInfo->Private.PageHeader.dwRegionHdrOffset +
                (ET9U32)((ET9U32)ET9KDB_REGHDROFFSET_REGIONOFFSET + (ET9U32)4 * (ET9U32)j),
                &dwOffset);

            /* get total keys */

            __ET9KDBGETDATA(dwOffset + ET9KDB_REGIONOFFSET_TOTALKEYS, 1, &bRegionKeys);
            wTotalKeys = (ET9U16)(wTotalKeys + bRegionKeys);
        }
    }

    if (!wTotalKeys) {
        return ET9STATUS_NO_KEY;
    }

    /* successful page load */

    pKDBInfo->Private.bUsingDynamicKDB = 0;
    pKDBInfo->Private.wPageNum = wPageNum;
    pKDBInfo->Private.wPageKeyNum = wTotalKeys;

    /* key count */

    if (pwTotalKeys){
        *pwTotalKeys = wTotalKeys;
    }

    /* update key layout info */

    eStatus = __KeyAreasUpdate_StaticUtil(pKDBInfo);

    if (eStatus) {
        return eStatus;
    }

    /* done */

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * This function loads a key's characters.
 *
 * @param pKDBInfo             Pointer to keyboard information structure.
 * @param wKey                 Key index.
 * @param pSymbInfo            Symbinfo to load chars into.
 * @param pbSymbsUsed          Pointer to receive the number of symbs used.
 * @param wLdbId               Presumed language at time of tap.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

static ET9STATUS ET9LOCALCALL __LoadKeyChars_Static(ET9KDBInfo              * const pKDBInfo,
                                                    const ET9U16                    wKey,
                                                    ET9SymbInfo             * const pSymbInfo,
                                                    ET9U8                   * const pbSymbsUsed,
                                                    const ET9U16                    wLdbId)
{
    ET9U8               bIndex;
    ET9U8               bThisKeyType;
    ET9U8               bNumSymbsToLoad;
    ET9SYMB             *psSym;
    ET9SYMB             *psUpperSym;
    ET9U16              wWord;
    ET9U32              dwOffset;
    ET9U32              dwCharArrayOffset;
    ET9DataPerBaseSym   *pDataPerBaseSym;

    WLOG5(fprintf(pLogFile5, "__LoadKeyChars_Static, pKDBInfo = %p\n", pKDBInfo);)

    ET9Assert(pKDBInfo);
    ET9Assert(pbSymbsUsed);
    ET9Assert(!pKDBInfo->Private.bUsingDynamicKDB);

    /* get key array offset */

    __ET9KDBREADWORD(pKDBInfo->Private.RegionHeader.dwRegionHdrOffset + ET9KDBAOFFSET_KEYOFFSET, &wWord);

    dwCharArrayOffset = wWord;

    dwOffset = pKDBInfo->Private.RegionHeader.dwRegionHdrOffset + dwCharArrayOffset + (ET9U32)(wKey * 2);

    /* get key offset */

    __ET9KDBREADWORD(dwOffset, &dwCharArrayOffset);

    dwCharArrayOffset = (ET9U32)(dwCharArrayOffset + pKDBInfo->Private.RegionHeader.dwRegionHdrOffset);

    /* get key type */

    __ET9KDBGETDATA(dwCharArrayOffset++, 1, &bThisKeyType);

    /* get total chars for key byKeyIndx */

    __ET9KDBGETDATA(dwCharArrayOffset++, 1, &bNumSymbsToLoad);

    pDataPerBaseSym = &pSymbInfo->DataPerBaseSym[pSymbInfo->bNumBaseSyms];

    *pbSymbsUsed = 1;
    pDataPerBaseSym->bNumSymsToMatch = 0;

    if (!pSymbInfo->bNumBaseSyms && !pSymbInfo->DataPerBaseSym[0].bNumSymsToMatch) {
        pSymbInfo->bSymbType = bThisKeyType;
    }
    else if (bThisKeyType != pSymbInfo->bSymbType) {
        WLOG5B(fprintf(pLogFile5, "  wrong symb type, skipping\n");)
        *pbSymbsUsed = 0;
        return ET9STATUS_NONE;
    }

    /* load chars */

    psSym = pDataPerBaseSym->sChar + pDataPerBaseSym->bNumSymsToMatch;
    psUpperSym = pDataPerBaseSym->sUpperCaseChar + pDataPerBaseSym->bNumSymsToMatch;

    for (bIndex = 0; bIndex < bNumSymbsToLoad; ++bIndex) {

        if (pDataPerBaseSym->bNumSymsToMatch >= ET9MAXALTSYMBS) {

            if (pSymbInfo->eInputType == ET9DISCRETEKEY || pSymbInfo->bSymbType == ET9KTSMARTPUNCT) {

                if (pSymbInfo->bNumBaseSyms >= ET9MAXBASESYMBS) {
                    return ET9STATUS_OUT_OF_RANGE_MAXBASESYMBS;
                }

                ++pDataPerBaseSym;
                ++*pbSymbsUsed;

                pDataPerBaseSym->bSymFreq = 0;
                pDataPerBaseSym->bNumSymsToMatch = 0;
                pDataPerBaseSym->bDefaultCharIndex = 0;

                psSym = pDataPerBaseSym->sChar;
                psUpperSym = pDataPerBaseSym->sUpperCaseChar;
            }
            else {
                return ET9STATUS_OUT_OF_RANGE_MAXALTSYMBS;
            }
        }

        __ET9KDBREADET9SYMB(dwCharArrayOffset + 2 * bIndex, psSym);

        WLOG5B(fprintf(pLogFile5, "  Loading = %04x\n", *psSym);)

        if (pKDBInfo->Private.pFilterSymb && pKDBInfo->Private.pFilterSymb(pKDBInfo->Private.pFilterSymbInfo, *psSym)) {
            WLOG5B(fprintf(pLogFile5, "    Suppressed\n");)
            continue;
        }

        WLOG5B(fprintf(pLogFile5, "    Loaded\n");)

        ++pDataPerBaseSym->bNumSymsToMatch;

        *psUpperSym++ = _ET9SymToOther(*psSym++, wLdbId);

        WLOG5B(fprintf(pLogFile5, "    Upper = %04x\n", *(psUpperSym - 1));)
    }

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Validates the integrity of a Keyboard Database.
 *
 * @param[in]     pKDBInfo          Pointer to the Keyboard Input Module Information Data Structure (ET9KDBInfo).
 */

static ET9STATUS ET9LOCALCALL __KDBValidate_Static(ET9KDBInfo       * const pKDBInfo)
{
    ET9STATUS   eStatus;
    ET9U16      wValidID;
    ET9U32      dwHashValue;

    /* applicable? */

    if (!pKDBInfo->ET9KDBReadData) {
        return ET9STATUS_ERROR;
    }

    eStatus = __InitDirectKDBAccess(pKDBInfo);

    if (eStatus) {
        return eStatus;
    }

    /* Get the integrity ID from KDB */

    wValidID = 0;
    dwHashValue = 0;

    __ET9KDBREADWORD(ET9KDBOFFSET_CHECKSUM, &wValidID);
    __HashChunk_Static(pKDBInfo, &dwHashValue, 0, ET9KDBOFFSET_CHECKSUM);
    __HashChunk_Static(pKDBInfo, &dwHashValue, ET9KDBOFFSET_CHECKSUM + 2, (ET9U32) -1);

    if (wValidID != (ET9U16)dwHashValue) {
        return ET9STATUS_CORRUPT_DB;
    }

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Retrieves version information for the active keyboard.
 *
 * @param[in]     pKDBInfo      Pointer to the Keyboard Input Module Information Data Structure (ET9KDBInfo).
 * @param[out]    psKDBVerBuf   Pointer to a buffer to which the Keyboard Input Module will write the version information. The buffer should be at least ET9MAXVERSIONSTR characters in length.
 * @param[out]    pwBufSize     Pointer to a value indicating the length in characters of the version information written to psKDBVerBuf.
 */

static ET9STATUS ET9LOCALCALL __GetKdbVersion_Static(ET9KDBInfo  * const pKDBInfo,
                                                     ET9SYMB     * const psKDBVerBuf,
                                                     ET9U16      * const pwBufSize)
{
    ET9STATUS               eStatus;
    static const ET9U8      byTemplateStr[] = "XT9 KDB Taa.bb Lcc.dd.ee Vff.gg Taa.bb Lcc.dd.ee Vff.gg";
    ET9U8           const * pbyVer;
    ET9U8                   byData;
    ET9SYMB                 *psTmp;
    ET9SYMB                 *psVerBuf = psKDBVerBuf;
    ET9U16                  wOldKdbNum;
    ET9U16                  wOldPageNum;

    WLOG5(fprintf(pLogFile5, "__GetKdbVersion_Static, pKDBInfo = %p\n", pKDBInfo);)

    ET9Assert(!pKDBInfo->Private.bUsingDynamicKDB);

    wOldKdbNum = pKDBInfo->wKdbNum;
    wOldPageNum = pKDBInfo->Private.wPageNum;

    /* size does NOT INCLUDE NULL terminator */

    *pwBufSize = 31;

    /* Copy template string. */

    pbyVer = byTemplateStr;
    psTmp  = psKDBVerBuf;

    while (*pbyVer) {
        *psTmp++ = (ET9SYMB)*pbyVer++;
    }

    eStatus = __KDBLoadPage(pKDBInfo, pKDBInfo->wFirstKdbNum, 0, NULL);

    if (eStatus) {
        return eStatus;
    }

    /* KDB version */
    psVerBuf += 9;                              /* Skip "DB T"   */

    /* Database type */
    __ET9KDBGETDATA(ET9KDBOFFSET_DATABASETYPE, 1, &byData);
    _ET9BinaryToHex(byData, psVerBuf);
    psVerBuf += 3;                              /* Skip "aa."    */

    /* Layout ver */
    __ET9KDBGETDATA(ET9KDBOFFSET_LAYOUTVER, 1, &byData);
    _ET9BinaryToHex(byData, psVerBuf);
    psVerBuf += 4;                              /* Skip "bb L"   */

    /* Primary language ID */
    __ET9KDBGETDATA(ET9KDBOFFSET_PRIMARYKEYBOARDID, 1, &byData);
    _ET9BinaryToHex(byData, psVerBuf);
    psVerBuf += 3;                              /* Skip "cc."    */

    /* Secondary language ID */
    __ET9KDBGETDATA(ET9KDBOFFSET_SECONDKEYBOARDID, 1, &byData);
    _ET9BinaryToHex(byData, psVerBuf);
    psVerBuf += 3;                              /* Skip "dd."    */

    /* add symbol class as 'fixed' unicode value */
    __ET9KDBGETDATA(ET9KDBOFFSET_SYMBOLCLASS, 1, &byData);
    _ET9BinaryToHex(byData, psVerBuf);
    psVerBuf += 4;                              /* Skip "ee V"    */

    /* Contents majorversion. */
    __ET9KDBGETDATA(ET9KDBOFFSET_CONTENTSMAJORVER, 1, &byData);
    _ET9BinaryToHex(byData, psVerBuf);
    psVerBuf += 3;                              /* Skip "ff."    */

    /* Contents minor version. */
    __ET9KDBGETDATA(ET9KDBOFFSET_CONTENTSMINORVER, 1, &byData);
    _ET9BinaryToHex(byData, psVerBuf);

    if (!_ET9KDBSecondKDBSupported(pKDBInfo)) {
        __KDBLoadPage(pKDBInfo, wOldKdbNum, wOldPageNum, NULL);
        return ET9STATUS_NONE;
    }

    eStatus = __KDBLoadPage(pKDBInfo, pKDBInfo->wSecondKdbNum, 0, NULL);

    if (eStatus) {
        return eStatus;
    }

    *pwBufSize = 55;

    /* KDB version */
    psVerBuf += 4;                           /* Skip "DB T"   */

    /* Database type */
    __ET9KDBGETDATA(ET9KDBOFFSET_DATABASETYPE, 1, &byData);
    _ET9BinaryToHex(byData, psVerBuf);
    psVerBuf += 3;                              /* Skip "aa."    */

    /* Layout ver */
    __ET9KDBGETDATA(ET9KDBOFFSET_LAYOUTVER, 1, &byData);
    _ET9BinaryToHex(byData, psVerBuf);
    psVerBuf += 4;                              /* Skip "bb L"   */

    /* Primary language ID */
    __ET9KDBGETDATA(ET9KDBOFFSET_PRIMARYKEYBOARDID, 1, &byData);
    _ET9BinaryToHex(byData, psVerBuf);
    psVerBuf += 3;                              /* Skip "cc."    */

    /* Secondary language ID */
    __ET9KDBGETDATA(ET9KDBOFFSET_SECONDKEYBOARDID, 1, &byData);
    _ET9BinaryToHex(byData, psVerBuf);
    psVerBuf += 3;                              /* Skip "dd."    */

    /* add symbol class as 'fixed' unicode value */
    __ET9KDBGETDATA(ET9KDBOFFSET_SYMBOLCLASS, 1, &byData);
    _ET9BinaryToHex(byData, psVerBuf);
    psVerBuf += 4;                              /* Skip "ee V"    */

    /* Contents majorversion. */
    __ET9KDBGETDATA(ET9KDBOFFSET_CONTENTSMAJORVER, 1, &byData);
    _ET9BinaryToHex(byData, psVerBuf);
    psVerBuf += 3;                              /* Skip "ff."    */

    /* Contents minor version. */
    __ET9KDBGETDATA(ET9KDBOFFSET_CONTENTSMINORVER, 1, &byData);
    _ET9BinaryToHex(byData, psVerBuf);

    __KDBLoadPage(pKDBInfo, wOldKdbNum, wOldPageNum, NULL);

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * This function processes an ambiguous tap.
 *
 * @param pKDBInfo              Pointer to keyboard information structure.
 * @param pSymbInfo             Pointer to store key symbol data into.
 * @param eLoadAction           If new, appending etc.
 * @param wLdbId                Presumed language at time of tap.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

static ET9STATUS ET9LOCALCALL __ProcessAmbigTap_Static(ET9KDBInfo              * const pKDBInfo,
                                                       ET9SymbInfo             * const pSymbInfo,
                                                       const ET9LOADSYMBACTION         eLoadAction,
                                                       const ET9U16                    wLdbId)
{
    ET9STATUS           eStatus;
    ET9U32              dwOffset;
    ET9U16              wBlockOffset;
    ET9U8               bKey;
    ET9U8               bFreq;
    ET9U8               bIndex;
    ET9U8               bNumSymbsToMatch = 0;

    WLOG5(fprintf(pLogFile5, "__ProcessAmbigTap_Static, pKDBInfo = %p, eLoadAction = %d\n", pKDBInfo, (int)eLoadAction);)

    ET9Assert(pKDBInfo);
    ET9Assert(pSymbInfo);
    ET9Assert(!pKDBInfo->Private.bUsingDynamicKDB);

    /* get block array offset */

    __ET9KDBREADWORD(pKDBInfo->Private.RegionHeader.dwRegionHdrOffset + ET9KDBAOFFSET_BLOCKOFFSET, &wBlockOffset);

    dwOffset =
        pKDBInfo->Private.RegionHeader.dwRegionHdrOffset + wBlockOffset +
        (((pKDBInfo->Private.RegionHeader.bBlockY *
           pKDBInfo->Private.RegionHeader.bTotalBlockCols) +
           pKDBInfo->Private.RegionHeader.bBlockX) * 2);

    /* get correct block offset */

    __ET9KDBREADWORD(dwOffset, &wBlockOffset);

    dwOffset = pKDBInfo->Private.RegionHeader.dwRegionHdrOffset + wBlockOffset;

    __ET9KDBREADBYTE(dwOffset, &bNumSymbsToMatch);

    if (!bNumSymbsToMatch) {
        return ET9STATUS_NO_KEY;
    }

    /* Would be good to test here, but we don't seem to have all info available at this point
        ET9Assert(bNumSymbsToMatch <= 1 || pSymbInfo->eInputType != ET9DISCRETEKEY && pSymbInfo->bSymbType != ET9KTSMARTPUNCT);
    */

    if (ET9_KDB_DISCRETE_MODE(pKDBInfo->dwStateBits) || ET9_KDB_MULTITAP_MODE(pKDBInfo->dwStateBits)) {
        bNumSymbsToMatch = 1;
        pSymbInfo->eInputType = ET9DISCRETEKEY;
    }
    else {
        pSymbInfo->eInputType = ET9REGIONALKEY;
    }

    ++dwOffset;

    if (eLoadAction != LOADSYMBACTION_APPEND){
        pSymbInfo->bNumBaseSyms = 0;
    }

    for (bIndex = 0; bIndex < bNumSymbsToMatch; ++bIndex) {

        ET9U8 bSymbsUsed;

        if (pSymbInfo->bNumBaseSyms >= ET9MAXBASESYMBS) {
            break;
        }


        __ET9KDBREADBYTE(dwOffset + (2 * bIndex) + 0, &bKey);
        __ET9KDBREADBYTE(dwOffset + (2 * bIndex) + 1, &bFreq);

        eStatus = __LoadKeyChars_Static(pKDBInfo,
                                        bKey,
                                        pSymbInfo,
                                        &bSymbsUsed,
                                        wLdbId);

        if (eStatus) {
            return eStatus;
        }

        {
            ET9U8 bSymbUsedCount;
            ET9DataPerBaseSym *pDPBSym;

            pDPBSym = &pSymbInfo->DataPerBaseSym[pSymbInfo->bNumBaseSyms];
            for (bSymbUsedCount = bSymbsUsed; bSymbUsedCount; --bSymbUsedCount, ++pDPBSym) {
                pDPBSym->bSymFreq = bFreq;
            }
        }

        pSymbInfo->bNumBaseSyms = (ET9U8)(pSymbInfo->bNumBaseSyms + bSymbsUsed);
    }

    ET9Assert(pSymbInfo->bNumBaseSyms);

    /* why? */

    if (pSymbInfo->bNumBaseSyms) {
        pSymbInfo->DataPerBaseSym[0].bSymFreq = 255;
    }

    pSymbInfo->wKeyIndex = ET9UNDEFINEDKEYVALUE;
    pSymbInfo->wTapX = ET9UNDEFINEDTAPVALUE;
    pSymbInfo->wTapY = ET9UNDEFINEDTAPVALUE;
    pSymbInfo->pKdbKey = NULL;

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * This function gets the symbol info for a non-ambig key.
 *
 * @param pKDBInfo             Pointer to keyboard information structure.
 * @param dwKeyOffset          Offset to regional key location.
 * @param pSymbInfo            Pointer to store key symbol data into.
 * @param eLoadAction          If new, appending etc.
 * @param wLdbId               Presumed language at time of tap.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

static ET9STATUS ET9LOCALCALL __GetNonAmbigKey_Static(ET9KDBInfo            * const pKDBInfo,
                                                      const ET9U32                  dwKeyOffset,
                                                      ET9SymbInfo           * const pSymbInfo,
                                                      const ET9LOADSYMBACTION       eLoadAction,
                                                      const ET9U16                  wLdbId)
{
    ET9U8               bType;
    ET9U8               bNumCharsToLoad;
    ET9U8               bDefaultCharIndex;
    ET9DataPerBaseSym   *pDPBSym;
    ET9U32              dwCurrKeyOffset = dwKeyOffset;

    WLOG5(fprintf(pLogFile5, "__GetNonAmbigKey_Static, pKDBInfo = %p, eLoadAction = %d\n", pKDBInfo, (int)eLoadAction);)

    ET9Assert(pKDBInfo);
    ET9Assert(pSymbInfo);
    ET9Assert(!pKDBInfo->Private.bUsingDynamicKDB);

    dwCurrKeyOffset += 8;

    /* get key type */

    __ET9KDBGETDATA(dwCurrKeyOffset++, 1, &bType);

    /* this function does not handle function keys */

    if (bType == ET9KTFUNCTION) {
        return ET9STATUS_ERROR;
    }

    pSymbInfo->bSymbType  = bType;

    /* get total chars for key byKeyIndx */

    __ET9KDBGETDATA(dwCurrKeyOffset++, 1, &bNumCharsToLoad);

    /* get default key index */

    __ET9KDBGETDATA(dwCurrKeyOffset++, 1, &bDefaultCharIndex);

    /* start fresh, if not appending */

    if (eLoadAction != LOADSYMBACTION_APPEND) {
        pSymbInfo->bNumBaseSyms = 1;
        pSymbInfo->DataPerBaseSym->bSymFreq = 255;
        pSymbInfo->DataPerBaseSym->bNumSymsToMatch = 0;
    }

    /* start loading */

    pDPBSym = pSymbInfo->DataPerBaseSym + pSymbInfo->bNumBaseSyms - 1;

    while (bNumCharsToLoad) {

        ET9SYMB     *psSym;
        ET9SYMB     *psUpperSym;

        if (pDPBSym->bNumSymsToMatch >= ET9MAXALTSYMBS) {

            if (pSymbInfo->bNumBaseSyms + 1 >= ET9MAXBASESYMBS) {
                return ET9STATUS_OUT_OF_RANGE_MAXALTSYMBS;
            }

            ++pSymbInfo->bNumBaseSyms;
            ++pDPBSym;

            pDPBSym->bSymFreq = 255;
            pDPBSym->bNumSymsToMatch = 0;
            pDPBSym->bDefaultCharIndex = bDefaultCharIndex;
        }

        psSym = pDPBSym->sChar + pDPBSym->bNumSymsToMatch;
        psUpperSym = pDPBSym->sUpperCaseChar + pDPBSym->bNumSymsToMatch;

        while (bNumCharsToLoad && pDPBSym->bNumSymsToMatch < ET9MAXALTSYMBS) {

            __ET9KDBREADET9SYMB(dwCurrKeyOffset, psSym);
            dwCurrKeyOffset += sizeof(ET9SYMB);
            --bNumCharsToLoad;

            ET9Assert(*psSym != 0xFFFF);

            WLOG5B(fprintf(pLogFile5, "  Loading = %04x\n", *psSym);)

            if (pKDBInfo->Private.pFilterSymb && pKDBInfo->Private.pFilterSymb(pKDBInfo->Private.pFilterSymbInfo, *psSym)) {
                WLOG5B(fprintf(pLogFile5, "    Suppressed\n");)
                continue;
            }

            WLOG5B(fprintf(pLogFile5, "    Loaded\n");)

            ++pDPBSym->bNumSymsToMatch;

            *psUpperSym++ = (ET9SYMB)((pSymbInfo->bSymbType == ET9KTSTRING) ? *psSym++ : _ET9SymToOther(*psSym++, wLdbId));

            WLOG5B(fprintf(pLogFile5, "    Upper = %04x\n", *(psUpperSym - 1));)
        }
    }

    pSymbInfo->eInputType = ET9DISCRETEKEY;
    pSymbInfo->wKeyIndex = ET9UNDEFINEDKEYVALUE;
    pSymbInfo->wTapX = ET9UNDEFINEDTAPVALUE;
    pSymbInfo->wTapY = ET9UNDEFINEDTAPVALUE;
    pSymbInfo->pKdbKey = NULL;
    pSymbInfo->bAmbigType = (ET9U8)ET9AMBIG;
    pSymbInfo->bTraceProbability = 0;
    pSymbInfo->bTraceIndex = 0;
    pSymbInfo->bFreqsInvalidated = 1;

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * This function searches for a key corresponding to a given coordinate.
 *
 * @param pKDBInfo         Pointer to keyboard information structure.
 * @param wX               X value.
 * @param wY               Y value.
 * @param pdwKeyOffset     Pointer to location to save key region offset.
 * @param pKeyRegion       OPTIONAL pointer to save containing key coords in.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

static ET9STATUS ET9LOCALCALL __SearchNonAmbigKey_Static(ET9KDBInfo     * const pKDBInfo,
                                                         const ET9U16           wX,
                                                         const ET9U16           wY,
                                                         ET9U32         * const pdwKeyOffset,
                                                         ET9Region      * const pKeyRegion)
{
    ET9U32    dwOffset;
    ET9U16    wBlockOffset;
    ET9U8     bNumBaseSyms;
    ET9U16    wNonAmbigKeyIdx;
    ET9U16    wTop;
    ET9U16    wBottom;
    ET9U16    wLeft;
    ET9U16    wRight;
    ET9U8     i;

    ET9Assert(pKDBInfo);
    ET9Assert(pdwKeyOffset);
    ET9Assert(!pKDBInfo->Private.bUsingDynamicKDB);

    WLOG5B(fprintf(pLogFile5, "__SearchNonAmbigKey_Static, wX %u, wY %u\n", wX, wY);)

    /* get block array offset */

    __ET9KDBREADWORD(pKDBInfo->Private.RegionHeader.dwRegionHdrOffset + ET9KDBNAOFFSET_BLOCKOFFSET, &wBlockOffset);

    dwOffset =
        pKDBInfo->Private.RegionHeader.dwRegionHdrOffset + wBlockOffset +
        (pKDBInfo->Private.RegionHeader.bBlockY *
         pKDBInfo->Private.RegionHeader.bTotalBlockCols * (ET9U16)2) +
        (pKDBInfo->Private.RegionHeader.bBlockX * (ET9U16)2);

    __ET9KDBREADWORD(dwOffset, &wBlockOffset);

    WLOG5B(fprintf(pLogFile5, "  dwOffset %u, wBlockOffset %u\n", dwOffset, wBlockOffset);)

    /* this is the block offset */

    dwOffset = pKDBInfo->Private.RegionHeader.dwRegionHdrOffset + wBlockOffset;

    __ET9KDBGETDATA(dwOffset, 1, &bNumBaseSyms);

    WLOG5B(fprintf(pLogFile5, "  bNumBaseSyms %u\n", bNumBaseSyms);)

    if (!bNumBaseSyms) {
        return ET9STATUS_NO_KEY;
    }

    ++dwOffset;

    for (i = 0; i < bNumBaseSyms; ++i) {

        __ET9KDBREADWORD(dwOffset, &wNonAmbigKeyIdx);
        dwOffset += 2;

        __ET9KDBREADWORD(pKDBInfo->Private.RegionHeader.dwRegionHdrOffset + (ET9U32)((ET9U32)ET9KDBNAOFFSET_KEYOFFSET + (ET9U32)wNonAmbigKeyIdx * (ET9U32)2), pdwKeyOffset);
        *pdwKeyOffset += pKDBInfo->Private.RegionHeader.dwRegionHdrOffset;

        __ET9KDBREADWORD(*pdwKeyOffset,   &wLeft);
        __ET9KDBREADWORD(*pdwKeyOffset+2, &wTop);
        __ET9KDBREADWORD(*pdwKeyOffset+4, &wRight);
        __ET9KDBREADWORD(*pdwKeyOffset+6, &wBottom);

        WLOG5B(fprintf(pLogFile5, "  wLeft %3u, wTop %3u, wRight %3u, wBottom %3u, \n", wLeft, wTop, wRight, wBottom);)

        if ((wX >= wLeft) && (wX <= wRight) &&
            (wY >= wTop)  && (wY <= wBottom)) {

            /* found the key */

            if (pKeyRegion) {
                pKeyRegion->wLeft       = wLeft;
                pKeyRegion->wTop        = wTop;
                pKeyRegion->wRight      = wRight;
                pKeyRegion->wBottom     = wBottom;
            }

            return ET9STATUS_NONE;
        }
    }

    WLOG5B(fprintf(pLogFile5, "  not found\n");)

    return ET9STATUS_NO_KEY;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * This function processes tap.
 *
 * @param pKDBInfo             Pointer to keyboard information structure.
 * @param pWordSymbInfo        Pointer to word sym info struct.
 * @param wX                   X-coordinate of tap.
 * @param wY                   Y-coordinate of tap.
 * @param bCurrIndexInList     Currently highlighted sel list entry.
 * @param psFunctionKey        Pointer to return function key info in.
 * @param eLoadAction          If new, appending etc.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

static ET9STATUS ET9LOCALCALL __ProcessTapInSubRegion_Static(ET9KDBInfo             * const pKDBInfo,
                                                             ET9WordSymbInfo        * const pWordSymbInfo,
                                                             const ET9U16                   wX,
                                                             const ET9U16                   wY,
                                                             const ET9U8                    bCurrIndexInList,
                                                             ET9SYMB                * const psFunctionKey,
                                                             const ET9LOADSYMBACTION        eLoadAction)
{
    ET9STATUS    eStatus;
    ET9U8        bNumSymbs;
    ET9UINT      nFunctionOnly;
    ET9U8        bType = ET9KTINVALID;
    ET9U32       dwKeyOffset = 0;   /* possibly uninitialized warning */

    WLOG5(fprintf(pLogFile5, "__ProcessTapInSubRegion_Static, pKDBInfo = %p, eLoadAction = %d\n", pKDBInfo, (int)eLoadAction);)

    ET9Assert(pKDBInfo);
    ET9Assert(pWordSymbInfo);
    ET9Assert(psFunctionKey);
    ET9Assert(!pKDBInfo->Private.bUsingDynamicKDB);

    eStatus = ET9STATUS_ERROR;

    *psFunctionKey = 0;

    bNumSymbs = (ET9U8) (pWordSymbInfo->bNumSymbs + 1);
    nFunctionOnly = 0;

    if (bNumSymbs > ET9MAXWORDSIZE) {
        nFunctionOnly = 1;
    }

    if (!pKDBInfo->Private.RegionHeader.bRegional) {

        eStatus = __SearchNonAmbigKey_Static(pKDBInfo, wX, wY, &dwKeyOffset, 0);

        if (eStatus) {
            return eStatus;
        }

        __ET9KDBGETDATA(dwKeyOffset+8, 1, &bType);

        if (bType == ET9KTFUNCTION) {
            __ET9KDBREADET9SYMB(dwKeyOffset+11, psFunctionKey);
            return ET9STATUS_NONE;
        }
        else if (bType == ET9KTINVALID){
            return ET9STATUS_INVALID_INPUT;
        }
    }

    if (nFunctionOnly) {
        return ET9STATUS_FULL;
    }

    _ET9ImminentSymb(pWordSymbInfo, bCurrIndexInList, bType == ET9KTSMARTPUNCT || bType == ET9KTPUNCTUATION);

    __MirrorWsiState(pKDBInfo, pWordSymbInfo);

    if (eLoadAction != LOADSYMBACTION_NEW) {

        ET9Assert(pWordSymbInfo->bNumSymbs);

        if (!pWordSymbInfo->bNumSymbs) {
            return ET9STATUS_ERROR;
        }

        --pWordSymbInfo->bNumSymbs;
    }

    bNumSymbs = (ET9U8)(pWordSymbInfo->bNumSymbs + 1);

    if (bNumSymbs > ET9MAXWORDSIZE) {
        return ET9STATUS_FULL;
    }

    if (eLoadAction != LOADSYMBACTION_APPEND) {
        _ET9ClearMem((ET9U8*)&pWordSymbInfo->SymbsInfo[bNumSymbs - 1], sizeof(ET9SymbInfo));
    }

    if (pKDBInfo->Private.RegionHeader.bRegional) {
        eStatus = __ProcessAmbigTap_Static(pKDBInfo, &pWordSymbInfo->SymbsInfo[bNumSymbs - 1], eLoadAction, pWordSymbInfo->Private.wLocale);
    }
    else {
        eStatus = __GetNonAmbigKey_Static(pKDBInfo, dwKeyOffset, &pWordSymbInfo->SymbsInfo[bNumSymbs - 1], eLoadAction, pWordSymbInfo->Private.wLocale);
    }

    if (eStatus) {
        return eStatus;
    }

    __ProcessWordSymInfo(pKDBInfo, pWordSymbInfo);

    pWordSymbInfo->SymbsInfo[bNumSymbs - 1].wKeyIndex = ET9UNDEFINEDKEYVALUE;
    pWordSymbInfo->SymbsInfo[bNumSymbs - 1].wTapX = wX;
    pWordSymbInfo->SymbsInfo[bNumSymbs - 1].wTapY = wY;

    return eStatus;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * This function locates the subregion container for tap coordinate.
 *
 * @param pKDBInfo             Pointer to keyboard information structure.
 * @param wX                   X-coordinate of tap.
 * @param wY                   Y-coordinate of tap.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

static ET9STATUS ET9LOCALCALL __FindSubRegionForTap_Static(ET9KDBInfo      * const pKDBInfo,
                                                           const ET9U16            wX,
                                                           const ET9U16            wY)

{
    ET9U8        i, j;
    ET9U32       dwOffset;

    ET9Assert(pKDBInfo);
    ET9Assert(!pKDBInfo->Private.bUsingDynamicKDB);

    WLOG5B(fprintf(pLogFile5, "__FindSubRegionForTap_Static, wX %u, wY %u\n", wX, wY);)

    if ((wX > pKDBInfo->Private.PageHeader.wRight) ||
        (wY > pKDBInfo->Private.PageHeader.wBottom)) {
        return ET9STATUS_KDB_OUT_OF_RANGE;
    }

    for (i = 0; i < pKDBInfo->Private.PageHeader.bTotalRegions; ++i) {

        WLOG5B(fprintf(pLogFile5, "  region %u (%u)\n", i, pKDBInfo->Private.PageHeader.bTotalRegions);)

        /* get region header offset */
        __ET9KDBREADDWORD(
            pKDBInfo->Private.dwPageHdrOffset +
            (ET9U32)((ET9U32)ET9KDB_PAGEHDROFFSET_REGIONOFFSET + (ET9U32)4 * (ET9U32)i),
            &pKDBInfo->Private.PageHeader.dwRegionHdrOffset);

        /* get left */
        __ET9KDBREADWORD(
            pKDBInfo->Private.PageHeader.dwRegionHdrOffset + ET9KDB_REGHDROFFSET_LEFT,
            &pKDBInfo->Private.RegionHeader.wLeft);

        /* get top */
        __ET9KDBREADWORD(
            pKDBInfo->Private.PageHeader.dwRegionHdrOffset + ET9KDB_REGHDROFFSET_TOP,
            &pKDBInfo->Private.RegionHeader.wTop);

        /* get right */
        __ET9KDBREADWORD(
            pKDBInfo->Private.PageHeader.dwRegionHdrOffset + ET9KDB_REGHDROFFSET_RIGHT,
            &pKDBInfo->Private.RegionHeader.wRight);

        /* get bottom */
        __ET9KDBREADWORD(
            pKDBInfo->Private.PageHeader.dwRegionHdrOffset + ET9KDB_REGHDROFFSET_BOTTOM,
            &pKDBInfo->Private.RegionHeader.wBottom);

        WLOG5B(fprintf(pLogFile5, "  wLeft %u, wTop %u, wRight %u, wBottom %u\n", pKDBInfo->Private.RegionHeader.wLeft, pKDBInfo->Private.RegionHeader.wTop, pKDBInfo->Private.RegionHeader.wRight, pKDBInfo->Private.RegionHeader.wBottom);)

        /* if not within region, move to next */
        if ((wX < pKDBInfo->Private.RegionHeader.wLeft) ||
            (wX > pKDBInfo->Private.RegionHeader.wRight) ||
            (wY < pKDBInfo->Private.RegionHeader.wTop) ||
            (wY > pKDBInfo->Private.RegionHeader.wBottom)) {
            continue;
        }

        /* get total regions */
        __ET9KDBGETDATA(
            pKDBInfo->Private.PageHeader.dwRegionHdrOffset +
            ET9KDB_REGHDROFFSET_TOTALREGIONS, 1,
            &pKDBInfo->Private.RegionHeader.bTotalRegions);

        for (j = 0; j < pKDBInfo->Private.RegionHeader.bTotalRegions; ++j) {

            WLOG5B(fprintf(pLogFile5, "    sub region %u (%u)\n", j, pKDBInfo->Private.RegionHeader.bTotalRegions);)

            /* get sub-region header offset */
            __ET9KDBREADDWORD(
                pKDBInfo->Private.PageHeader.dwRegionHdrOffset +
                (ET9U32)((ET9U32)ET9KDB_REGHDROFFSET_REGIONOFFSET + (ET9U32)4 * (ET9U32)j),
                &dwOffset);
            pKDBInfo->Private.RegionHeader.dwRegionHdrOffset = dwOffset;

            /* get left */
            __ET9KDBREADWORD(
                pKDBInfo->Private.RegionHeader.dwRegionHdrOffset +
                ET9KDB_REGIONOFFSET_LEFT,
                &pKDBInfo->Private.RegionHeader.wLeft);

            /* get top */
            __ET9KDBREADWORD(
                pKDBInfo->Private.RegionHeader.dwRegionHdrOffset +
                ET9KDB_REGIONOFFSET_TOP,
                &pKDBInfo->Private.RegionHeader.wTop);

            /* get right */
            __ET9KDBREADWORD(
                pKDBInfo->Private.RegionHeader.dwRegionHdrOffset +
                ET9KDB_REGIONOFFSET_RIGHT,
                &pKDBInfo->Private.RegionHeader.wRight);

            /* get bottom */
            __ET9KDBREADWORD(
                pKDBInfo->Private.RegionHeader.dwRegionHdrOffset +
                ET9KDB_REGIONOFFSET_BOTTOM,
                &pKDBInfo->Private.RegionHeader.wBottom);

            WLOG5B(fprintf(pLogFile5, "    wLeft %u, wTop %u, wRight %u, wBottom %u\n", pKDBInfo->Private.RegionHeader.wLeft, pKDBInfo->Private.RegionHeader.wTop, pKDBInfo->Private.RegionHeader.wRight, pKDBInfo->Private.RegionHeader.wBottom);)

            /* if not within region, move to next */
            if ((wX < pKDBInfo->Private.RegionHeader.wLeft) ||
                (wX > pKDBInfo->Private.RegionHeader.wRight) ||
                (wY < pKDBInfo->Private.RegionHeader.wTop) ||
                (wY > pKDBInfo->Private.RegionHeader.wBottom)) {
                continue;
            }

            /* get region ambig flag */
            __ET9KDBGETDATA(
                pKDBInfo->Private.RegionHeader.dwRegionHdrOffset +
                ET9KDB_REGIONOFFSET_AMBIGFLAG, 1,
                &pKDBInfo->Private.RegionHeader.bRegional);

            __ET9KDBGETDATA(
                pKDBInfo->Private.RegionHeader.dwRegionHdrOffset +
                ET9KDB_REGIONOFFSET_TOTALBLOCKCOLS, 1,
                &pKDBInfo->Private.RegionHeader.bTotalBlockCols);

            __ET9KDBGETDATA(
                pKDBInfo->Private.RegionHeader.dwRegionHdrOffset +
                ET9KDB_REGIONOFFSET_TOTALBLOCKROWS, 1,
                &pKDBInfo->Private.RegionHeader.bTotalBlockRows);

            __ET9KDBREADWORD(
                pKDBInfo->Private.RegionHeader.dwRegionHdrOffset +
                ET9KDB_REGIONOFFSET_BLOCKWIDTH,
                &pKDBInfo->Private.RegionHeader.wBlockWidth);

            __ET9KDBREADWORD(
                pKDBInfo->Private.RegionHeader.dwRegionHdrOffset +
                ET9KDB_REGIONOFFSET_BLOCKHEIGHT,
                &pKDBInfo->Private.RegionHeader.wBlockHeight);

            WLOG5B(fprintf(pLogFile5, "    bRegional %u, bTotalBlockCols %u, bTotalBlockRows %u, wBlockWidth %u, wBlockHeight %u\n",
                                      pKDBInfo->Private.RegionHeader.bRegional,
                                      pKDBInfo->Private.RegionHeader.bTotalBlockCols,
                                      pKDBInfo->Private.RegionHeader.bTotalBlockRows,
                                      pKDBInfo->Private.RegionHeader.wBlockWidth,
                                      pKDBInfo->Private.RegionHeader.wBlockHeight);)

            pKDBInfo->Private.RegionHeader.bBlockX = (ET9U8)
                ((wX - pKDBInfo->Private.RegionHeader.wLeft) /
                 pKDBInfo->Private.RegionHeader.wBlockWidth);

            if (pKDBInfo->Private.RegionHeader.bBlockX >=
                pKDBInfo->Private.RegionHeader.bTotalBlockCols) {

                pKDBInfo->Private.RegionHeader.bBlockX = (ET9U8)(pKDBInfo->Private.RegionHeader.bTotalBlockCols - 1);
            }

            pKDBInfo->Private.RegionHeader.bBlockY = (ET9U8)
                ((wY - pKDBInfo->Private.RegionHeader.wTop) /
                 pKDBInfo->Private.RegionHeader.wBlockHeight);

            if (pKDBInfo->Private.RegionHeader.bBlockY >=
                pKDBInfo->Private.RegionHeader.bTotalBlockRows) {

                pKDBInfo->Private.RegionHeader.bBlockY = (ET9U8)(pKDBInfo->Private.RegionHeader.bTotalBlockRows - 1);
            }

            WLOG5B(fprintf(pLogFile5, "    bBlockX %u, bBlockY %u\n", pKDBInfo->Private.RegionHeader.bBlockX, pKDBInfo->Private.RegionHeader.bBlockY);)

            return ET9STATUS_NONE;
        }
    }

    return ET9STATUS_KDB_OUT_OF_RANGE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * This function finds the first tap char.
 *
 * @param pKDBInfo             Pointer to keyboard information structure.
 * @param pDirectedPos         Tap position.
 * @param psFirstChar          Pointer to first char symbol.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

static ET9STATUS ET9LOCALCALL __GetFirstTapChar_Static(ET9KDBInfo            * const pKDBInfo,
                                                       ET9DirectedPos        * const pDirectedPos,
                                                       ET9SYMB               * const psFirstChar)
{
    ET9STATUS   eStatus;
    ET9U8       bFirstKey;
    ET9U8       bTotalChars;
    ET9U8       bDefaultCharIndex;
    ET9U16      wOffset;
    ET9U32      dwOffset;

    const ET9U16 wX = (ET9U16)pDirectedPos->sPos.nX;
    const ET9U16 wY = (ET9U16)pDirectedPos->sPos.nY;

    ET9Assert(pKDBInfo);
    ET9Assert(psFirstChar);
    ET9Assert(!pKDBInfo->Private.bUsingDynamicKDB);

    eStatus = __FindSubRegionForTap_Static(pKDBInfo, wX, wY);

    if (eStatus) {
        return eStatus;
    }

    if (!pKDBInfo->Private.RegionHeader.bRegional) {

        eStatus = __SearchNonAmbigKey_Static(pKDBInfo, wX, wY, &dwOffset, 0);

        if (eStatus) {
            return eStatus;
        }

        __ET9KDBGETDATA(dwOffset+9, 1, &bTotalChars);

        if (!bTotalChars) {
            return ET9STATUS_NO_CHAR;
        }

        __ET9KDBGETDATA(dwOffset+10, 1, &bDefaultCharIndex);

        if (bDefaultCharIndex > bTotalChars) {
            bDefaultCharIndex = 0;
        }

        __ET9KDBREADWORD(dwOffset + 11 + bDefaultCharIndex * 2, psFirstChar);
    }
    else {
        /* get block array offset */

        __ET9KDBREADWORD(
            pKDBInfo->Private.RegionHeader.dwRegionHdrOffset +
            ET9KDBAOFFSET_BLOCKOFFSET, &wOffset);

        dwOffset =
            pKDBInfo->Private.RegionHeader.dwRegionHdrOffset + wOffset +
            (((pKDBInfo->Private.RegionHeader.bBlockY*
               pKDBInfo->Private.RegionHeader.bTotalBlockCols) +
               pKDBInfo->Private.RegionHeader.bBlockX) * (ET9U16)2);

        /* get correct block offset */

        __ET9KDBREADWORD(dwOffset, &wOffset);

        dwOffset = pKDBInfo->Private.RegionHeader.dwRegionHdrOffset + wOffset;

        {
            ET9U8 bNumBaseSyms;

            __ET9KDBGETDATA((dwOffset)++, 1, &bNumBaseSyms);

            if (!bNumBaseSyms) {
                return  ET9STATUS_NO_KEY;
            }
        }

        __ET9KDBGETDATA(dwOffset, 1, &bFirstKey);

        /* get key array offset */

        __ET9KDBREADWORD(pKDBInfo->Private.RegionHeader.dwRegionHdrOffset + ET9KDBAOFFSET_KEYOFFSET, &wOffset);

        dwOffset = pKDBInfo->Private.RegionHeader.dwRegionHdrOffset + wOffset + (ET9U32)(bFirstKey * 2);

        /* get key offset */

        __ET9KDBREADWORD(dwOffset, &wOffset);

        dwOffset = (ET9U32) (wOffset + pKDBInfo->Private.RegionHeader.dwRegionHdrOffset);

        /* get total chars for key byKeyIndx */

        __ET9KDBGETDATA(dwOffset + 1, 1, &bTotalChars);

        if (bTotalChars > ET9MAXALTSYMBS) {
            return ET9STATUS_OUT_OF_RANGE_MAXALTSYMBS;
        }
        else {
            __ET9KDBREADET9SYMB(dwOffset + 2, psFirstChar);
        }
    }

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * This function processes an ambig tap for the current KDB.
 *
 * @param pKDBInfo             Pointer to keyboard information structure.
 * @param wX                   X-coordinate of tap.
 * @param wY                   Y-coordinate of tap.
 * @param pWordSymbInfo        Pointer to tap info.
 * @param psFunctionKey        Pointer to return function key info in.
 * @param bCurrIndexInList     Currently highlighted sel list entry.
 * @param eLoadAction          If new, appending etc.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

static ET9STATUS ET9LOCALCALL __ProcessAmbigTap_CurrentKdb_Static(ET9KDBInfo           * const pKDBInfo,
                                                                  const ET9U16                 wX,
                                                                  const ET9U16                 wY,
                                                                  ET9WordSymbInfo      * const pWordSymbInfo,
                                                                  ET9SYMB              * const psFunctionKey,
                                                                  const ET9U8                  bCurrIndexInList,
                                                                  const ET9LOADSYMBACTION      eLoadAction)
{
    ET9STATUS       eStatus;

    WLOG5(fprintf(pLogFile5, "__ProcessAmbigTap_CurrentKdb_Static, pKDBInfo = %p, eLoadAction = %d, wX = %d, wY = %d\n", pKDBInfo, (int)eLoadAction, wX, wY);)

    ET9Assert(!pKDBInfo->Private.bUsingDynamicKDB);

    if (pKDBInfo->Private.sKdbAction.bShiftState) {
        pWordSymbInfo->dwStateBits |= ET9STATE_SHIFT_MASK;
    }

    eStatus = __FindSubRegionForTap_Static(pKDBInfo, wX, wY);

    if (eStatus) {
        return eStatus;
    }

    eStatus = __ProcessTapInSubRegion_Static(pKDBInfo, pWordSymbInfo, wX, wY, bCurrIndexInList, psFunctionKey, eLoadAction);

    return eStatus;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * This function searches for the block containing a key in ambiguous region.
 *
 * @param pKDBInfo         Pointer to keyboard information structure.
 * @param byKeyNum         Look up key.
 * @param pKeyRegion       Pointer to store region info in.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

static ET9STATUS ET9LOCALCALL __FindBlockForKeyInAmbigRgn_Static(ET9KDBInfo * const pKDBInfo,
                                                                 ET9U8              byKeyNum,
                                                                 ET9Region  * const pKeyRegion)
{
    ET9U32    dwOffset;
    ET9U8     byTotalKeys;
    ET9U8     byHighestKeyProb = 0;
    ET9U8     byHighestRowIdx = 0;
    ET9U8     byHighestColIdx = 0;
    ET9U16    wBlockOffsetArray;
    ET9U16    wBlockOffset;
    ET9U8     byTotalBlockCols;
    ET9U8     byTotalBlockRows;
    ET9U8     i, j, k;
    ET9U8     bHolder[2];

    ET9Assert(pKDBInfo);
    ET9Assert(pKeyRegion);
    ET9Assert(!pKDBInfo->Private.bUsingDynamicKDB);

    __ET9KDBGETDATA(
        pKDBInfo->Private.RegionHeader.dwRegionHdrOffset +
        ET9KDB_REGIONOFFSET_TOTALBLOCKCOLS, 1, &byTotalBlockCols);

    __ET9KDBGETDATA(
        pKDBInfo->Private.RegionHeader.dwRegionHdrOffset +
        ET9KDB_REGIONOFFSET_TOTALBLOCKROWS, 1, &byTotalBlockRows);

    /* get block array offset */
    __ET9KDBREADWORD(
        pKDBInfo->Private.RegionHeader.dwRegionHdrOffset +
        ET9KDBAOFFSET_BLOCKOFFSET, &wBlockOffsetArray);

    for (i = 0; i < byTotalBlockRows; ++i) {

        for (j = 0; j < byTotalBlockCols; ++j) {

            dwOffset =
                pKDBInfo->Private.RegionHeader.dwRegionHdrOffset + wBlockOffsetArray +
                2*((i*byTotalBlockCols) + j);

            /* get correct block offset */

            __ET9KDBREADWORD(dwOffset, &wBlockOffset);

            dwOffset = pKDBInfo->Private.RegionHeader.dwRegionHdrOffset + wBlockOffset;

            __ET9KDBGETDATA(dwOffset, 1, &byTotalKeys);

            ++dwOffset;

            for (k = 0; k < byTotalKeys; ++k) {

                __ET9KDBGETDATA(dwOffset + (2*k), 2, bHolder);

                if (bHolder[0] == byKeyNum) {

                    if (byHighestKeyProb < bHolder[1]) {

                        byHighestKeyProb = bHolder[1];

                        byHighestRowIdx = i;
                        byHighestColIdx = j;
                    }
                }
            }
        }
    }

    /* get block region coordinate */

    /* get left */
    __ET9KDBREADWORD(
        pKDBInfo->Private.RegionHeader.dwRegionHdrOffset +
        ET9KDB_REGIONOFFSET_LEFT,
        &pKDBInfo->Private.RegionHeader.wLeft);

    /* get top */
    __ET9KDBREADWORD(
        pKDBInfo->Private.RegionHeader.dwRegionHdrOffset +
        ET9KDB_REGIONOFFSET_TOP,
        &pKDBInfo->Private.RegionHeader.wTop);

    /* get block width */
    __ET9KDBREADWORD(
        pKDBInfo->Private.RegionHeader.dwRegionHdrOffset +
        ET9KDB_REGIONOFFSET_BLOCKWIDTH,
        &pKDBInfo->Private.RegionHeader.wBlockWidth);

    /* get block height */
    __ET9KDBREADWORD(
        pKDBInfo->Private.RegionHeader.dwRegionHdrOffset +
        ET9KDB_REGIONOFFSET_BLOCKHEIGHT,
        &pKDBInfo->Private.RegionHeader.wBlockHeight);

    pKeyRegion->wLeft = (ET9U16)
        (pKDBInfo->Private.RegionHeader.wLeft +
         (ET9U16)(byHighestColIdx++ * pKDBInfo->Private.RegionHeader.wBlockWidth));

    pKeyRegion->wTop = (ET9U16)
        (pKDBInfo->Private.RegionHeader.wTop +
         (ET9U16)(byHighestRowIdx++ * pKDBInfo->Private.RegionHeader.wBlockHeight));

    pKeyRegion->wRight = (ET9U16)
        (pKDBInfo->Private.RegionHeader.wLeft +
         (ET9U16)(byHighestColIdx * pKDBInfo->Private.RegionHeader.wBlockWidth));

    pKeyRegion->wBottom = (ET9U16)
        (pKDBInfo->Private.RegionHeader.wTop +
         (ET9U16)(byHighestRowIdx * pKDBInfo->Private.RegionHeader.wBlockHeight));

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * This function search a symbol in ambiguous region.
 *
 * @param pKDBInfo                 Pointer to keyboard information structure.
 * @param sSymbol                  Symbol to look up.
 * @param pbySymbAsDefault         Pointer to indicate if the found key has symb as default.
 * @param pwFirstRgnKeyIndex       Pointer to first key index of this region.
 * @param pbyRegionalKey           Pointer to indicate if the found key is regional.
 * @param pwKeyIndex               Pointer to store key index in.
 * @param pKeyRegion               Pointer to store region info in.
 * @param bInitialSymCheck     Set if only matching on first sym assigned to key; false if checking ALL key syms.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

static ET9STATUS ET9LOCALCALL __FindSymbolInRegionalRgn_Static(ET9KDBInfo       * const pKDBInfo,
                                                               const ET9SYMB            sSymbol,
                                                               ET9U8            * const pbySymbAsDefault,
                                                               ET9U16           * const pwFirstRgnKeyIndex,
                                                               ET9U8            * const pbyRegionalKey,
                                                               ET9U16           * const pwKeyIndex,
                                                               ET9Region        * const pKeyRegion,
                                                               const ET9BOOL            bInitialSymCheck)
{
    ET9STATUS eStatus = ET9STATUS_ERROR;
    ET9U32    dwOffset;
    ET9U16    wOffset;
    ET9U16    wKeyOffset;
    ET9U8     byTotalChars;
    ET9SYMB   sKeyChar;
    ET9U8     byTotalKeys;
    ET9U8     i, j;

    ET9Assert(pKDBInfo);
    ET9Assert(pbySymbAsDefault);
    ET9Assert(pwFirstRgnKeyIndex);
    ET9Assert(pwKeyIndex);
    ET9Assert(pKeyRegion);
    ET9Assert(!pKDBInfo->Private.bUsingDynamicKDB);

    /* get total keys */
    __ET9KDBGETDATA(
        pKDBInfo->Private.RegionHeader.dwRegionHdrOffset +
        ET9KDB_REGIONOFFSET_TOTALKEYS, 1, &byTotalKeys);

    /* get key array offset */
    __ET9KDBREADWORD(
        pKDBInfo->Private.RegionHeader.dwRegionHdrOffset +
        ET9KDBAOFFSET_KEYOFFSET, &wKeyOffset);

    for (i = 0; i < byTotalKeys; ++i) {

        dwOffset = pKDBInfo->Private.RegionHeader.dwRegionHdrOffset + wKeyOffset + (i*2);

        /* get key offset */
        __ET9KDBREADWORD(dwOffset, &wOffset);

        dwOffset = pKDBInfo->Private.RegionHeader.dwRegionHdrOffset + wOffset;

        /* skip key type */
        ++dwOffset;

        /* get total characters */
        __ET9KDBGETDATA(dwOffset++, 1, &byTotalChars);

        /* if caller limiting to initial sym check, limit loop count */
        if (bInitialSymCheck && byTotalChars > 1) {
            byTotalChars = 1;
        }

        for (j = 0; j < byTotalChars; ++j) {

            __ET9KDBREADET9SYMB(dwOffset, &sKeyChar);

            if ((sKeyChar == sSymbol) && ((!j) || (*pwKeyIndex == 0xFFFF))) {

                *pwKeyIndex = (ET9U16)(*pwFirstRgnKeyIndex + i);
                *pbyRegionalKey = 1;

                /* search for the block that has the highest key probability */

                eStatus = __FindBlockForKeyInAmbigRgn_Static(pKDBInfo, i, pKeyRegion);

                if (eStatus) {
                    return eStatus;
                }

                if (!j) {
                    *pbySymbAsDefault = 1;
                    return ET9STATUS_NONE;
                }
                break;
            }

            dwOffset += sizeof(ET9SYMB);
        }
    }

    *pwFirstRgnKeyIndex = (ET9U16)(*pwFirstRgnKeyIndex + byTotalKeys);

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * This function searches for symbol in non ambiguous region.
 *
 * @param pKDBInfo                 Pointer to keyboard information structure.
 * @param sSymbol                  Symbol to look up.
 * @param pbySymbAsDefault         Pointer to indicate if the found key has the searched symb as default.
 * @param pwFirstRgnKeyIndex       Pointer to first key index of this region.
 * @param pwKeyIndex               Pointer to store key index in.
 * @param pKeyRegion               Pointer to store region info in.
 * @param bInitialSymCheck     Set if only matching on first sym assigned to key; false if checking ALL key syms.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

static ET9STATUS ET9LOCALCALL __FindSymbolInNonAmbigRgn_Static(ET9KDBInfo       * const pKDBInfo,
                                                               const ET9SYMB            sSymbol,
                                                               ET9U8            * const pbySymbAsDefault,
                                                               ET9U16           * const pwFirstRgnKeyIndex,
                                                               ET9U16           * const pwKeyIndex,
                                                               ET9Region        * const pKeyRegion,
                                                               const ET9BOOL            bInitialSymCheck)
{
    ET9U32    dwOffset;
    ET9U16    wOffset;
    ET9U8     byTotalKeys;
    ET9U8     byKeyType;
    ET9U8     byTotalChars;
    ET9SYMB   sKeyChar;
    ET9U8     byDefaultSymbIndex;
    ET9U8     i, j;

    ET9Assert(pKDBInfo);
    ET9Assert(pbySymbAsDefault);
    ET9Assert(pwFirstRgnKeyIndex);
    ET9Assert(pwKeyIndex);
    ET9Assert(pKeyRegion);
    ET9Assert(!pKDBInfo->Private.bUsingDynamicKDB);

    /* get total keys */

    __ET9KDBGETDATA(
        pKDBInfo->Private.RegionHeader.dwRegionHdrOffset +
        ET9KDB_REGIONOFFSET_TOTALKEYS, 1, &byTotalKeys);

    for (i = 0; i < byTotalKeys; ++i) {

        dwOffset = pKDBInfo->Private.RegionHeader.dwRegionHdrOffset + ET9KDBNAOFFSET_KEYOFFSET + (i*2);

        /* get key offset */
        __ET9KDBREADWORD(dwOffset, &wOffset);

        dwOffset = pKDBInfo->Private.RegionHeader.dwRegionHdrOffset + wOffset;

        /* skip to key type */
        dwOffset += 8;

        /* get key type */
        __ET9KDBGETDATA(dwOffset++, 1, &byKeyType);

        /* skip if the key is string */
        if (byKeyType == ET9KTSTRING) {
            continue;
        }

        /* get total characters */
        __ET9KDBGETDATA(dwOffset++, 1, &byTotalChars);

        if (!byTotalChars) {
            continue;
        }
        /* if caller limiting to initial sym check, limit loop count */
        if (bInitialSymCheck) {
            byTotalChars = 1;
        }

        /* get default char index */
        __ET9KDBGETDATA(dwOffset++, 1, &byDefaultSymbIndex);

        for (j = 0; j < byTotalChars; ++j) {

            __ET9KDBREADET9SYMB(dwOffset, &sKeyChar);

            if ((sKeyChar == sSymbol) && ((j == byDefaultSymbIndex) || (*pwKeyIndex == 0xFFFF))) {

                dwOffset = pKDBInfo->Private.RegionHeader.dwRegionHdrOffset + ET9KDBNAOFFSET_KEYOFFSET + (i*2);

                /* get key offset */

                __ET9KDBREADWORD(dwOffset, &wOffset);

                dwOffset = pKDBInfo->Private.RegionHeader.dwRegionHdrOffset + wOffset;

                if (pKeyRegion) {
                    __ET9KDBREADWORD(dwOffset,   &pKeyRegion->wLeft);
                    __ET9KDBREADWORD(dwOffset+2, &pKeyRegion->wTop);
                    __ET9KDBREADWORD(dwOffset+4, &pKeyRegion->wRight);
                    __ET9KDBREADWORD(dwOffset+6, &pKeyRegion->wBottom);
                }

                *pwKeyIndex = (ET9U16)(*pwFirstRgnKeyIndex + i);

                if (j == byDefaultSymbIndex) {
                    *pbySymbAsDefault = 1;
                    return ET9STATUS_NONE;
                }
                break;
            }

            dwOffset += sizeof(ET9SYMB);
        }
    }

    *pwFirstRgnKeyIndex = (ET9U16)(*pwFirstRgnKeyIndex + byTotalKeys);

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * This function retrieves ambig/non-ambig region info for symbol.
 *
 * @param pKDBInfo             Pointer to keyboard information structure.
 * @param sSymbol              Symbol to look up.
 * @param pbyRegionalKey       Pointer to indicate if the found key is regional.
 * @param pwKeyIndex           Pointer to store key index in.
 * @param pKeyRegion           Pointer to store region info in.
 * @param bInitialSymCheck     Set if only matching on first sym assigned to key; false if checking ALL key syms.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

static ET9STATUS ET9LOCALCALL __FindSymbol_Static(ET9KDBInfo        * const pKDBInfo,
                                                  const ET9SYMB             sSymbol,
                                                  ET9U8             * const pbyRegionalKey,
                                                  ET9U16            * const pwKeyIndex,
                                                  ET9Region         * const pKeyRegion,
                                                  const ET9BOOL             bInitialSymCheck)
{
    ET9STATUS eStatus;
    ET9U8     i;
    ET9U8     j;
    ET9U8     byAmbig;
    ET9U8     bySymbAsDefault = 0;
    ET9U32    dwOffset;
    ET9U16    byFirstRgnKeyIndex = 0;

    ET9Assert(!pKDBInfo->Private.bUsingDynamicKDB);

    WLOG5(fprintf(pLogFile5, "__FindSymbol_Static, pKDBInfo = %p\n", pKDBInfo);)

    for (i = 0; i < pKDBInfo->Private.PageHeader.bTotalRegions; ++i) {

        /* get region header offset */
        __ET9KDBREADDWORD(
            pKDBInfo->Private.dwPageHdrOffset +
            ET9KDB_PAGEHDROFFSET_REGIONOFFSET + 4*i,
            &pKDBInfo->Private.PageHeader.dwRegionHdrOffset);

        /* get total regions */
        __ET9KDBGETDATA(
            pKDBInfo->Private.PageHeader.dwRegionHdrOffset +
            ET9KDB_REGHDROFFSET_TOTALREGIONS, 1,
            &pKDBInfo->Private.RegionHeader.bTotalRegions);

        for (j = 0; j < pKDBInfo->Private.RegionHeader.bTotalRegions; ++j) {

            /* get sub-region header offset */
            __ET9KDBREADDWORD(
                pKDBInfo->Private.PageHeader.dwRegionHdrOffset +
                ET9KDB_REGHDROFFSET_REGIONOFFSET + 4*j,
                &dwOffset);
            pKDBInfo->Private.RegionHeader.dwRegionHdrOffset = dwOffset;

            /* get region ambig flag */
            __ET9KDBGETDATA(
                pKDBInfo->Private.RegionHeader.dwRegionHdrOffset +
                ET9KDB_REGIONOFFSET_AMBIGFLAG, 1,
                &byAmbig);

            if (byAmbig) {

                eStatus = __FindSymbolInRegionalRgn_Static(pKDBInfo,
                                                           sSymbol,
                                                           &bySymbAsDefault,
                                                           &byFirstRgnKeyIndex,
                                                           pbyRegionalKey,
                                                           pwKeyIndex,
                                                           pKeyRegion,
                                                           bInitialSymCheck);

                if (eStatus) {
                    break;
                }

                if (bySymbAsDefault) {
                    *pbyRegionalKey = 1;
                    return ET9STATUS_NONE;
                }

                continue;
            }
            else {

                eStatus = __FindSymbolInNonAmbigRgn_Static(pKDBInfo,
                                                           sSymbol,
                                                           &bySymbAsDefault,
                                                           &byFirstRgnKeyIndex,
                                                           pwKeyIndex,
                                                           pKeyRegion,
                                                           bInitialSymCheck);

                if (eStatus) {
                    break;
                }

                if (bySymbAsDefault) {
                    *pbyRegionalKey = 0;
                    return ET9STATUS_NONE;
                }

                continue;
            }
        }
    }

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * This function retrieves key info from in ambiguous region.
 *
 * @param pKDBInfo             Pointer to keyboard information structure.
 * @param pSymbInfo            Pointer to symbol info.
 * @param wRegionKeyIndex      Key index.
 * @param pKeyRegion           Pointer to store region info in.
 * @param eLoadAction          If new, appending etc.
 * @param wLdbId               Presumed language at time of tap.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

static ET9STATUS ET9LOCALCALL __FindKeyInAmbigRgn_Static(ET9KDBInfo             * const pKDBInfo,
                                                         ET9SymbInfo            * const pSymbInfo,
                                                         const ET9U16                   wRegionKeyIndex,
                                                         ET9Region              * const pKeyRegion,
                                                         const ET9LOADSYMBACTION        eLoadAction,
                                                         const ET9U16                   wLdbId)
{
    ET9STATUS           eStatus = ET9STATUS_ERROR;
    ET9U32              dwOffset;
    ET9U8               byTotalKeys;
    ET9U8               bKey;
    ET9U8               bFreq;
    ET9U8               byHighestKeyProb = 0;
    ET9U8               byHighestRowIdx = 0;
    ET9U8               byHighestColIdx = 0;
    ET9U16              wBlockOffsetArray;
    ET9U16              wBlockOffset;
    ET9U8               byTotalBlockCols;
    ET9U8               byTotalBlockRows;
    ET9U8               bIndex;
    ET9U8               i;
    ET9U8               j;
    ET9U8               k;
    ET9U8               bHolder[2];
    ET9U8               bNumSymbsToMatch = 0;

    ET9Assert(!pKDBInfo->Private.bUsingDynamicKDB);

    WLOG5(fprintf(pLogFile5, "__FindKeyInAmbigRgn_Static, pKDBInfo = %p, eLoadAction = %d\n", pKDBInfo, (int)eLoadAction);)

    __ET9KDBGETDATA(
        pKDBInfo->Private.RegionHeader.dwRegionHdrOffset +
        ET9KDB_REGIONOFFSET_TOTALBLOCKCOLS, 1, &byTotalBlockCols);

    __ET9KDBGETDATA(
        pKDBInfo->Private.RegionHeader.dwRegionHdrOffset +
        ET9KDB_REGIONOFFSET_TOTALBLOCKROWS, 1, &byTotalBlockRows);

    /* get block array offset */

    __ET9KDBREADWORD(
        pKDBInfo->Private.RegionHeader.dwRegionHdrOffset +
        ET9KDBAOFFSET_BLOCKOFFSET, &wBlockOffsetArray);

    pSymbInfo->bAmbigType = ET9AMBIG;

    for (i = 0; i < byTotalBlockRows; ++i) {

        for (j = 0; j < byTotalBlockCols; ++j) {

            dwOffset =
                pKDBInfo->Private.RegionHeader.dwRegionHdrOffset + wBlockOffsetArray +
                2*((i*byTotalBlockCols) + j);

            /* get correct block offset */
            __ET9KDBREADWORD(dwOffset, &wBlockOffset);

            dwOffset = pKDBInfo->Private.RegionHeader.dwRegionHdrOffset + wBlockOffset;

            __ET9KDBGETDATA(dwOffset, 1, &byTotalKeys);

            ++dwOffset;

            for (k = 0; k < byTotalKeys; ++k) {

                __ET9KDBGETDATA(dwOffset + (2*k), 2, bHolder);

                if (bHolder[0] == wRegionKeyIndex) {
                    if (byHighestKeyProb < bHolder[1]) {
                        byHighestKeyProb = bHolder[1];
                        byHighestRowIdx = i;
                        byHighestColIdx = j;
                    }
                }
            }
        }
    }

    dwOffset =
        pKDBInfo->Private.RegionHeader.dwRegionHdrOffset + wBlockOffsetArray +
        (((byHighestRowIdx*byTotalBlockCols) + byHighestColIdx) * 2);

    /* get correct block offset */

    __ET9KDBREADWORD(dwOffset, &wBlockOffset);

    dwOffset = pKDBInfo->Private.RegionHeader.dwRegionHdrOffset + wBlockOffset;

    __ET9KDBREADBYTE(dwOffset, &bNumSymbsToMatch);

    if (!bNumSymbsToMatch) {
        return ET9STATUS_NO_KEY;
    }

    /* Would be good to test here, but we don't seem to have all info available at this point
        ET9Assert(bNumSymbsToMatch <= 1 || pSymbInfo->eInputType != ET9DISCRETEKEY && pSymbInfo->bSymbType != ET9KTSMARTPUNCT);
    */

    if (ET9_KDB_DISCRETE_MODE(pKDBInfo->dwStateBits) || ET9_KDB_MULTITAP_MODE(pKDBInfo->dwStateBits)) {
        bNumSymbsToMatch = 1;
        pSymbInfo->eInputType = ET9DISCRETEKEY;
    }
    else {
        pSymbInfo->eInputType = ET9REGIONALKEY;
    }

    ++dwOffset;

    if (eLoadAction != LOADSYMBACTION_APPEND){
        pSymbInfo->bNumBaseSyms = 0;
    }

    for (bIndex = 0; bIndex < bNumSymbsToMatch; ++bIndex) {

        ET9U8 bSymbsUsed;

        if (pSymbInfo->bNumBaseSyms >= ET9MAXBASESYMBS) {
            break;
        }

        __ET9KDBREADBYTE(dwOffset + (2 * bIndex) + 0, &bKey);
        __ET9KDBREADBYTE(dwOffset + (2 * bIndex) + 1, &bFreq);

        if (!bFreq) {
            bFreq = 1;
        }

        eStatus = __LoadKeyChars_Static(pKDBInfo,
                                        bKey,
                                        pSymbInfo,
                                        &bSymbsUsed,
                                        wLdbId);

        if (eStatus) {
            return eStatus;
        }

        {
            ET9U8 bSymbUsedCount;
            ET9DataPerBaseSym *pDPBSym;

            pDPBSym = &pSymbInfo->DataPerBaseSym[pSymbInfo->bNumBaseSyms];
            for (bSymbUsedCount = bSymbsUsed; bSymbUsedCount; --bSymbUsedCount, ++pDPBSym) {
                pDPBSym->bSymFreq = bFreq;
            }
        }

        pSymbInfo->bNumBaseSyms = (ET9U8)(pSymbInfo->bNumBaseSyms + bSymbsUsed);
    }

    /* get block region coordinate */

    /* get left */

    __ET9KDBREADWORD(
        pKDBInfo->Private.RegionHeader.dwRegionHdrOffset +
        ET9KDB_REGIONOFFSET_LEFT,
        &pKDBInfo->Private.RegionHeader.wLeft);

    /* get top */

    __ET9KDBREADWORD(
        pKDBInfo->Private.RegionHeader.dwRegionHdrOffset +
        ET9KDB_REGIONOFFSET_TOP,
        &pKDBInfo->Private.RegionHeader.wTop);

    /* get block width */

    __ET9KDBREADWORD(
        pKDBInfo->Private.RegionHeader.dwRegionHdrOffset +
        ET9KDB_REGIONOFFSET_BLOCKWIDTH,
        &pKDBInfo->Private.RegionHeader.wBlockWidth);

    /* get block height */

    __ET9KDBREADWORD(
        pKDBInfo->Private.RegionHeader.dwRegionHdrOffset +
        ET9KDB_REGIONOFFSET_BLOCKHEIGHT,
        &pKDBInfo->Private.RegionHeader.wBlockHeight);

    if (pKeyRegion) {
        pKeyRegion->wLeft = (ET9U16)
            (pKDBInfo->Private.RegionHeader.wLeft +
             (ET9U16)(byHighestColIdx++ * pKDBInfo->Private.RegionHeader.wBlockWidth));

        pKeyRegion->wTop = (ET9U16)
            (pKDBInfo->Private.RegionHeader.wTop +
             (ET9U16)(byHighestRowIdx++ * pKDBInfo->Private.RegionHeader.wBlockHeight));

        pKeyRegion->wRight = (ET9U16)
            (pKDBInfo->Private.RegionHeader.wLeft +
             (ET9U16)(byHighestColIdx * pKDBInfo->Private.RegionHeader.wBlockWidth));

        pKeyRegion->wBottom = (ET9U16)
            (pKDBInfo->Private.RegionHeader.wTop +
             (ET9U16)(byHighestRowIdx * pKDBInfo->Private.RegionHeader.wBlockHeight));
    }

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * This function processes an ambig key for the current KDB.
 *
 * @param pKDBInfo             Pointer to keyboard information structure.
 * @param wKeyIndex            Key index.
 * @param pWordSymbInfo        Pointer to tap info.
 * @param psFunctionKey        Pointer to return function key info in.
 * @param bCurrIndexInList     Currently highlighted sel list entry.
 * @param eLoadAction          If new, appending etc.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

static ET9STATUS ET9LOCALCALL __ProcessAmbigKey_CurrentKdb_Static(ET9KDBInfo           * const pKDBInfo,
                                                                  const ET9U16                 wKeyIndex,
                                                                  ET9WordSymbInfo      * const pWordSymbInfo,
                                                                  ET9SYMB              * const psFunctionKey,
                                                                  const ET9U8                  bCurrIndexInList,
                                                                  const ET9LOADSYMBACTION      eLoadAction)
{
    ET9STATUS    eStatus;
    ET9U8        bTotalSubRegionKeys;
    ET9U32       i, j;
    ET9U16       wChkKeyIndex = 0;
    ET9U8        byAmbig;
    ET9U16       wOffset;
    ET9U32       dwOffset;
    ET9U32       dwKeyOffset = 0;
    ET9U8        bType = ET9KTINVALID;
    ET9U8        bNumSymbs;
    ET9Region   *pKeyRegion = NULL;

    WLOG5(fprintf(pLogFile5, "__ProcessAmbigKey_CurrentKdb_Static, pKDBInfo = %p, eLoadAction = %d\n", pKDBInfo, (int)eLoadAction);)

    ET9Assert(pKDBInfo);
    ET9Assert(pWordSymbInfo);
    ET9Assert(psFunctionKey);
    ET9Assert(!pKDBInfo->Private.bUsingDynamicKDB);

    if (pKDBInfo->Private.sKdbAction.bShiftState) {
        pWordSymbInfo->dwStateBits |= ET9STATE_SHIFT_MASK;
    }

    for (i = 0; i < pKDBInfo->Private.PageHeader.bTotalRegions; ++i) {

        /* get region header offset */
        __ET9KDBREADDWORD(
            pKDBInfo->Private.dwPageHdrOffset +
            ET9KDB_PAGEHDROFFSET_REGIONOFFSET + 4*i,
            &pKDBInfo->Private.PageHeader.dwRegionHdrOffset);

        /* get total sub-regions */
        __ET9KDBGETDATA(
            pKDBInfo->Private.PageHeader.dwRegionHdrOffset +
            ET9KDB_REGHDROFFSET_TOTALREGIONS, 1,
            &pKDBInfo->Private.RegionHeader.bTotalRegions);

        for (j = 0; j < pKDBInfo->Private.RegionHeader.bTotalRegions; ++j) {

            /* get sub-region header offset */
            __ET9KDBREADDWORD(
                pKDBInfo->Private.PageHeader.dwRegionHdrOffset +
                ET9KDB_REGHDROFFSET_REGIONOFFSET + 4*j,
                &pKDBInfo->Private.RegionHeader.dwRegionHdrOffset);

            /* get region total keys */
            __ET9KDBGETDATA(
                pKDBInfo->Private.RegionHeader.dwRegionHdrOffset +
                ET9KDB_REGIONOFFSET_TOTALKEYS, 1, &bTotalSubRegionKeys);

            if ((wChkKeyIndex + bTotalSubRegionKeys) > wKeyIndex) {

                /* get region ambig flag */
                __ET9KDBGETDATA(
                    pKDBInfo->Private.RegionHeader.dwRegionHdrOffset +
                    ET9KDB_REGIONOFFSET_AMBIGFLAG, 1,
                    &byAmbig);

                /* if this is exact key, quickly find out if it is a function key. If this is function key
                   quickly process and return */
                if (!byAmbig) {

                    dwOffset =
                        pKDBInfo->Private.RegionHeader.dwRegionHdrOffset +
                        ET9KDBNAOFFSET_KEYOFFSET + ((ET9U16)(wKeyIndex - wChkKeyIndex)*2);

                    /* get key offset */
                    __ET9KDBREADWORD(dwOffset, &wOffset);

                    dwKeyOffset = pKDBInfo->Private.RegionHeader.dwRegionHdrOffset + wOffset;

                    __ET9KDBGETDATA(dwKeyOffset+8, 1, &bType);
                    if (bType == ET9KTFUNCTION) {
                        __ET9KDBREADET9SYMB(dwKeyOffset+11, psFunctionKey);
                        return ET9STATUS_NONE;
                    }
                    else if (bType == ET9KTINVALID){
                        return ET9STATUS_INVALID_INPUT;
                    }
                }

                /* now we've handled the case where this was a non function key. */

                _ET9ImminentSymb(pWordSymbInfo, bCurrIndexInList, bType == ET9KTSMARTPUNCT || bType == ET9KTPUNCTUATION);

                __MirrorWsiState(pKDBInfo, pWordSymbInfo);

                if (eLoadAction != LOADSYMBACTION_NEW) {

                    ET9Assert(pWordSymbInfo->bNumSymbs);

                    if (!pWordSymbInfo->bNumSymbs) {
                        return ET9STATUS_ERROR;
                    }

                    --pWordSymbInfo->bNumSymbs;
                }

                bNumSymbs = (ET9U8)(pWordSymbInfo->bNumSymbs + 1);

                if (bNumSymbs > ET9MAXWORDSIZE) {
                    return ET9STATUS_FULL;
                }

                if (eLoadAction != LOADSYMBACTION_APPEND) {
                    _ET9ClearMem((ET9U8*)&pWordSymbInfo->SymbsInfo[bNumSymbs - 1], sizeof(ET9SymbInfo));
                }

                if (byAmbig) {
                    eStatus = __FindKeyInAmbigRgn_Static(pKDBInfo,
                                                         &pWordSymbInfo->SymbsInfo[bNumSymbs - 1],
                                                         (ET9U16)(wKeyIndex - wChkKeyIndex),
                                                         pKeyRegion,
                                                         eLoadAction,
                                                         pWordSymbInfo->Private.wLocale);
                }
                else {
                    eStatus = __GetNonAmbigKey_Static(pKDBInfo,
                                                      dwKeyOffset,
                                                      &pWordSymbInfo->SymbsInfo[bNumSymbs - 1],
                                                      eLoadAction,
                                                      pWordSymbInfo->Private.wLocale);
                }

                if (eStatus) {
                    return eStatus;
                }

                pWordSymbInfo->SymbsInfo[bNumSymbs - 1].wKeyIndex = wKeyIndex;
                pWordSymbInfo->SymbsInfo[bNumSymbs - 1].wTapX = ET9UNDEFINEDTAPVALUE;
                pWordSymbInfo->SymbsInfo[bNumSymbs - 1].wTapY = ET9UNDEFINEDTAPVALUE;
                pWordSymbInfo->SymbsInfo[bNumSymbs - 1].pKdbKey = NULL;
                pWordSymbInfo->SymbsInfo[bNumSymbs - 1].bTraceProbability = 0;
                pWordSymbInfo->SymbsInfo[bNumSymbs - 1].bTraceIndex = 0;
                pWordSymbInfo->SymbsInfo[bNumSymbs - 1].bFreqsInvalidated = 1;

                __ProcessWordSymInfo(pKDBInfo, pWordSymbInfo);

                if (pWordSymbInfo->bNumSymbs == ET9MAXLDBWORDSIZE) {

                    if (_ET9IsMagicStringKey(pWordSymbInfo)) {

                        eStatus = ET9KDB_GetKdbVersion(pKDBInfo,
                                                       pWordSymbInfo->Private.szIDBVersion,
                                                       ET9MAXVERSIONSTR,
                                                       &pWordSymbInfo->Private.wIDBVersionStrSize);

                        if (eStatus) {
                            return eStatus;
                        }
                    }
                }

                return ET9STATUS_NONE;
            }
            else {
                wChkKeyIndex = (ET9U16)(wChkKeyIndex + bTotalSubRegionKeys);
            }
        }
    }

    if (wKeyIndex >= wChkKeyIndex) {
        return ET9STATUS_OUT_OF_RANGE;
    }

    return ET9STATUS_NONE;
}

/* ************************************************************************************************************** */
/* * STATIC UTIL ************************************************************************************************ */
/* ************************************************************************************************************** */

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pKDBInfo                  Pointer to the Keyboard Input Module Information Data Structure (ET9KDBInfo).
 * @param dwOffset                  .
 * @param pKeyArea                  .
 * @param pKeyAreaExtra             .
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

static ET9STATUS ET9LOCALCALL __DiscreteKey_StaticUtil (ET9KDBInfo          * const pKDBInfo,
                                                        const ET9U32                dwOffset,
                                                        ET9KdbAreaInfo      * const pKeyArea,
                                                        ET9KdbAreaExtraInfo * const pKeyAreaExtra)
{
    ET9KdbLayoutInfo * const pLayoutInfo = pKDBInfo->Private.pCurrLayoutInfo;

    ET9U8  bType;
    ET9U8  bTotChars;
    ET9U8  bDefaultIndex;
    ET9U16 wLeft;
    ET9U16 wTop;
    ET9U16 wRight;
    ET9U16 wBottom;

    ET9Assert(!pKDBInfo->Private.bUsingDynamicKDB);

    __ET9KDBREADWORD(dwOffset + ET9KDB_DISKEY_LEFT, &wLeft);
    __ET9KDBREADWORD(dwOffset + ET9KDB_DISKEY_TOP, &wTop);
    __ET9KDBREADWORD(dwOffset + ET9KDB_DISKEY_RIGHT, &wRight);
    __ET9KDBREADWORD(dwOffset + ET9KDB_DISKEY_BOTTOM, &wBottom);

    __ET9KDBREADBYTE(dwOffset + ET9KDB_DISKEY_TYPE, &bType);
    __ET9KDBREADBYTE(dwOffset + ET9KDB_DISKEY_TOTCHARS, &bTotChars);
    __ET9KDBREADBYTE(dwOffset + ET9KDB_DISKEY_DEFAULTINDEX, &bDefaultIndex);

    WLOG5(fprintf(pLogFile5, "        dis-key: wLeft             %5u\n", wLeft);)
    WLOG5(fprintf(pLogFile5, "        dis-key: wTop              %5u\n", wTop);)
    WLOG5(fprintf(pLogFile5, "        dis-key: wRight            %5u\n", wRight);)
    WLOG5(fprintf(pLogFile5, "        dis-key: wBottom           %5u\n", wBottom);)
    WLOG5(fprintf(pLogFile5, "        dis-key: bType             %5u\n", bType);)
    WLOG5(fprintf(pLogFile5, "        dis-key: bTotChars         %5u\n", bTotChars);)
    WLOG5(fprintf(pLogFile5, "        dis-key: bDefaultIndex     %5u\n", bDefaultIndex);)
    WLOG5(fprintf(pLogFile5, "        dis-key: chars              ");)

    pKeyArea->eKeyType = (ET9KEYTYPE)bType;
    pKeyArea->eInputType = ET9DISCRETEKEY;
    pKeyArea->nCharCount = 0;
    pKeyArea->psChars = &pLayoutInfo->psCharPool[pLayoutInfo->nCharPoolCount];
    pKeyArea->nShiftedCharCount = 0;
    pKeyArea->psShiftedChars = NULL;
    pKeyArea->nMultitapCharCount = 0;
    pKeyArea->psMultitapChars = NULL;
    pKeyArea->nMultitapShiftedCharCount = 0;
    pKeyArea->psMultitapShiftedChars = NULL;

    pKeyAreaExtra->nBlockCount = 0;
    pKeyAreaExtra->bMaxProbCenter = 0;
    pKeyAreaExtra->wMaxProbCenterCount = 0;

    pKeyArea->bRegionOk = 1;

    pKeyArea->sRegion.wLeft = wLeft;
    pKeyArea->sRegion.wTop = wTop;
    pKeyArea->sRegion.wRight = wRight;
    pKeyArea->sRegion.wBottom = wBottom;

    pKeyArea->nCenterX = (wLeft + wRight) / 2;
    pKeyArea->nCenterY = (wTop + wBottom) / 2;

    {
        ET9U8 bIndex;
        ET9SYMB sChar;

        for (bIndex = 0; bIndex < bTotChars; ++bIndex) {

            __ET9KDBREADET9SYMB(dwOffset + ET9KDB_DISKEY_CHARS + (bIndex * 2), &sChar);

            if (sChar > 0x20 && sChar <= 0x7F) {
                WLOG5(fprintf(pLogFile5, "%c ", (char)sChar);)
            }
            else if (bIndex < 10) {
                WLOG5(fprintf(pLogFile5, "%04X ", (int)sChar);)
            }

            if (pLayoutInfo->nCharPoolCount < ET9_KDB_MAX_POOL_CHARS) {
                pKeyArea->psChars[pKeyArea->nCharCount] = sChar;
                ++pKeyArea->nCharCount;
                ++pLayoutInfo->nCharPoolCount;
            }
            else {
                ET9Assert(0);
                return ET9STATUS_KDB_HAS_TOO_MANY_CHARS;
            }
        }
    }

    WLOG5(fprintf(pLogFile5, "\n");)
    WLOG5(fprintf(pLogFile5, "\n");)

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pKDBInfo                  Pointer to the Keyboard Input Module Information Data Structure (ET9KDBInfo).
 * @param dwOffset                  .
 * @param pKeyArea                  .
 * @param pKeyAreaExtra             .
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

static ET9STATUS ET9LOCALCALL __RegionalKey_StaticUtil (ET9KDBInfo          * const pKDBInfo,
                                                        const ET9U32                dwOffset,
                                                        ET9KdbAreaInfo      * const pKeyArea,
                                                        ET9KdbAreaExtraInfo * const pKeyAreaExtra)
{
    ET9KdbLayoutInfo * const pLayoutInfo = pKDBInfo->Private.pCurrLayoutInfo;

    ET9U8  bType;
    ET9U8  bTotChars;

    ET9Assert(!pKDBInfo->Private.bUsingDynamicKDB);

    __ET9KDBREADBYTE(dwOffset + ET9KDB_REGKEY_TYPE, &bType);
    __ET9KDBREADBYTE(dwOffset + ET9KDB_REGKEY_TOTCHARS, &bTotChars);

    WLOG5(fprintf(pLogFile5, "        reg-key: bType             %5u\n", bType);)
    WLOG5(fprintf(pLogFile5, "        reg-key: bTotChars         %5u\n", bTotChars);)
    WLOG5(fprintf(pLogFile5, "        reg-key: chars              ");)

    pKeyArea->eKeyType = (ET9KEYTYPE)bType;
    pKeyArea->eInputType = ET9REGIONALKEY;
    pKeyArea->nCharCount = 0;
    pKeyArea->psChars = &pLayoutInfo->psCharPool[pLayoutInfo->nCharPoolCount];
    pKeyArea->nShiftedCharCount = 0;
    pKeyArea->psShiftedChars = NULL;
    pKeyArea->nMultitapCharCount = 0;
    pKeyArea->psMultitapChars = NULL;
    pKeyArea->nMultitapShiftedCharCount = 0;
    pKeyArea->psMultitapShiftedChars = NULL;

    pKeyAreaExtra->nBlockCount = 0;
    pKeyAreaExtra->bMaxProbCenter = 0;
    pKeyAreaExtra->wMaxProbCenterCount = 0;

    pKeyArea->bRegionOk = 0;

    pKeyArea->sRegion.wLeft = 0;
    pKeyArea->sRegion.wTop = 0;
    pKeyArea->sRegion.wRight = 0;
    pKeyArea->sRegion.wBottom = 0;

    pKeyArea->nCenterX = ET9UNDEFINEDTAPVALUE;
    pKeyArea->nCenterY = ET9UNDEFINEDTAPVALUE;

    {
        ET9U8 bIndex;
        ET9SYMB sChar;

        for (bIndex = 0; bIndex < bTotChars; ++bIndex) {

            __ET9KDBREADET9SYMB(dwOffset + ET9KDB_REGKEY_CHARS + (bIndex * 2), &sChar);

            if (sChar > 0x20 && sChar <= 0x7F) {
                WLOG5(fprintf(pLogFile5, "%c ", (char)sChar);)
            }
            else if (bIndex < 10) {
                WLOG5(fprintf(pLogFile5, "%04X ", (int)sChar);)
            }

            if (pLayoutInfo->nCharPoolCount < ET9_KDB_MAX_POOL_CHARS) {
                pKeyArea->psChars[pKeyArea->nCharCount] = sChar;
                ++pKeyArea->nCharCount;
                ++pLayoutInfo->nCharPoolCount;
            }
            else {
                ET9Assert(0);
                return ET9STATUS_KDB_HAS_TOO_MANY_CHARS;
            }
        }
    }

    WLOG5(fprintf(pLogFile5, "\n");)
    WLOG5(fprintf(pLogFile5, "\n");)

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pKDBInfo                  Pointer to the Keyboard Input Module Information Data Structure (ET9KDBInfo).
 * @param dwOffset                  .
 * @param nLeft                     .
 * @param nTop                      .
 * @param nRight                    .
 * @param nBottom                   .
 * @param pKeyAreas                 .
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

static ET9STATUS ET9LOCALCALL __DiscreteBlock_StaticUtil (ET9KDBInfo        * const pKDBInfo,
                                                          const ET9U32              dwOffset,
                                                          const ET9UINT             nLeft,
                                                          const ET9UINT             nTop,
                                                          const ET9UINT             nRight,
                                                          const ET9UINT             nBottom,
                                                          ET9KdbAreaInfo    * const pKeyAreas)
{
    ET9U8  bNumAmbigKeys;

    ET9_UNUSED(nLeft);
    ET9_UNUSED(nTop);
    ET9_UNUSED(nRight);
    ET9_UNUSED(nBottom);
    ET9_UNUSED(pKeyAreas);

    ET9Assert(!pKDBInfo->Private.bUsingDynamicKDB);

    __ET9KDBREADBYTE(dwOffset + ET9KDB_DISBLOCK_NUMBAMBIGKEYS, &bNumAmbigKeys);

    WLOG5B(fprintf(pLogFile5, "        dis-block: bNumAmbigKeys        %u\n", bNumAmbigKeys);)
    WLOG5B(fprintf(pLogFile5, "\n");)

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pKDBInfo                  Pointer to the Keyboard Input Module Information Data Structure (ET9KDBInfo).
 * @param dwOffset                  .
 * @param nLeft                     .
 * @param nTop                      .
 * @param nRight                    .
 * @param nBottom                   .
 * @param pKeyAreas                 .
 * @param pKeyAreaExtra             .
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

static ET9STATUS ET9LOCALCALL __RegionalBlock_StaticUtil (ET9KDBInfo            * const pKDBInfo,
                                                          const ET9U32                  dwOffset,
                                                          const ET9UINT                 nLeft,
                                                          const ET9UINT                 nTop,
                                                          const ET9UINT                 nRight,
                                                          const ET9UINT                 nBottom,
                                                          ET9KdbAreaInfo        * const pKeyAreas,
                                                          ET9KdbAreaExtraInfo   * const pKeyAreaExtra)
{
    ET9U8  bTotalKeys;

    ET9_UNUSED(pKeyAreas);

    ET9Assert(!pKDBInfo->Private.bUsingDynamicKDB);

    __ET9KDBREADBYTE(dwOffset + ET9KDB_REGBLOCK_NUMBTOTALKEYS, &bTotalKeys);

    WLOG5B(fprintf(pLogFile5, "        reg-block: bTotalKeys           %u\n", bTotalKeys);)
    WLOG5B(fprintf(pLogFile5, "        reg-block: keys                  ");)

    {
        ET9U8 bIndex;

        for (bIndex = 0; bIndex < bTotalKeys; ++bIndex) {

            const ET9U32 dwPairOffset = dwOffset + ET9KDB_REGBLOCK_KEYS + (bIndex * 2);

            ET9U8 bKeyIndex;
            ET9U8 bKeyProb;

            __ET9KDBREADBYTE(dwPairOffset + ET9KDB_REGBLOCKKEY_KEYINDEX, &bKeyIndex);
            __ET9KDBREADBYTE(dwPairOffset + ET9KDB_REGBLOCKKEY_KEYPROB, &bKeyProb);

            WLOG5B(fprintf(pLogFile5, "{%2u,%3u} ", (int)bKeyIndex, (int)bKeyProb);)

            {
                ET9KdbBlockInfo *pBlock = NULL;

                if (pKeyAreaExtra[bKeyIndex].nBlockCount < ET9_KDB_MAX_KEY_BLOCKS) {
                    pBlock = &pKeyAreaExtra[bKeyIndex].pBlocks[pKeyAreaExtra[bKeyIndex].nBlockCount++];
                }
                else {

                    ET9U8 bCurrBestProb = 0;
                    ET9UINT nBlockIndex;

                    for (nBlockIndex = 0; nBlockIndex < ET9_KDB_MAX_KEY_BLOCKS; ++nBlockIndex) {

                        ET9KdbBlockInfo * const pThisBlock = &pKeyAreaExtra[bKeyIndex].pBlocks[nBlockIndex];

                        if (!pBlock || bCurrBestProb < pThisBlock->bProb) {
                            pBlock = pThisBlock;
                            bCurrBestProb = pThisBlock->bProb;
                        }
                    }

                    if (bKeyProb > bCurrBestProb) {
                        pBlock = NULL;
                    }
                }

                if (pBlock) {
                    pBlock->bProb = bKeyProb;
                    pBlock->wX = (ET9U16)((nLeft + nRight) / 2);
                    pBlock->wY = (ET9U16)((nTop + nBottom) / 2);
                }

                if (pKeyAreaExtra[bKeyIndex].bMaxProbCenter < bKeyProb) {
                    pKeyAreaExtra[bKeyIndex].bMaxProbCenter = bKeyProb;
                    pKeyAreaExtra[bKeyIndex].wMaxProbCenterCount = 1;
                    pKeyAreaExtra[bKeyIndex].wMaxProbCenterX = (ET9U16)((nLeft + nRight) / 2);
                    pKeyAreaExtra[bKeyIndex].wMaxProbCenterY = (ET9U16)((nTop + nBottom) / 2);
                }
                else if (pKeyAreaExtra[bKeyIndex].bMaxProbCenter == bKeyProb) {
                    ++pKeyAreaExtra[bKeyIndex].wMaxProbCenterCount;
                    pKeyAreaExtra[bKeyIndex].wMaxProbCenterX = (ET9U16)(pKeyAreaExtra[bKeyIndex].wMaxProbCenterX + ((nLeft + nRight) / 2));
                    pKeyAreaExtra[bKeyIndex].wMaxProbCenterY = (ET9U16)(pKeyAreaExtra[bKeyIndex].wMaxProbCenterY + ((nTop + nBottom) / 2));
                }
            }
        }
    }

    WLOG5B(fprintf(pLogFile5, "\n");)
    WLOG5B(fprintf(pLogFile5, "\n");)

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pKDBInfo                  Pointer to the Keyboard Input Module Information Data Structure (ET9KDBInfo).
 * @param dwOffset                  .
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

static ET9STATUS ET9LOCALCALL __SubRegionHeader_StaticUtil (ET9KDBInfo     * const pKDBInfo,
                                                            const ET9U32           dwOffset)
{
    ET9U8  bAmbigFlag;
    ET9U16 wLeft;
    ET9U16 wTop;
    ET9U16 wRight;
    ET9U16 wBottom;
    ET9U8  bTotalKeys;
    ET9U8  bBlockRows;
    ET9U8  bBlockCols;
    ET9U16 wBlockWidth;
    ET9U16 wBlockHeight;
    ET9U16 wKeyTableOffset;
    ET9U16 wBlockTableOffset;

    ET9KdbAreaInfo * const pKeyAreas = &pKDBInfo->Private.pCurrLayoutInfo->pKeyAreas[pKDBInfo->Private.pCurrLayoutInfo->nKeyAreaCount];
    ET9KdbAreaExtraInfo * const pKeyAreasExtra = &pKDBInfo->Private.wm.staticLoad.sLayoutExtraInfo.pKeyAreas[pKDBInfo->Private.pCurrLayoutInfo->nKeyAreaCount];

    ET9Assert(!pKDBInfo->Private.bUsingDynamicKDB);

    __ET9KDBREADBYTE(dwOffset + ET9KDB_REGIONOFFSET_AMBIGFLAG, &bAmbigFlag);
    __ET9KDBREADWORD(dwOffset + ET9KDB_REGIONOFFSET_LEFT, &wLeft);
    __ET9KDBREADWORD(dwOffset + ET9KDB_REGIONOFFSET_TOP, &wTop);
    __ET9KDBREADWORD(dwOffset + ET9KDB_REGIONOFFSET_RIGHT, &wRight);
    __ET9KDBREADWORD(dwOffset + ET9KDB_REGIONOFFSET_BOTTOM, &wBottom);
    __ET9KDBREADBYTE(dwOffset + ET9KDB_REGIONOFFSET_TOTALKEYS, &bTotalKeys);
    __ET9KDBREADBYTE(dwOffset + ET9KDB_REGIONOFFSET_TOTALBLOCKROWS, &bBlockRows);
    __ET9KDBREADBYTE(dwOffset + ET9KDB_REGIONOFFSET_TOTALBLOCKCOLS, &bBlockCols);
    __ET9KDBREADWORD(dwOffset + ET9KDB_REGIONOFFSET_BLOCKWIDTH, &wBlockWidth);
    __ET9KDBREADWORD(dwOffset + ET9KDB_REGIONOFFSET_BLOCKHEIGHT, &wBlockHeight);

    if (bAmbigFlag) {
        __ET9KDBREADWORD(dwOffset + ET9KDBAOFFSET_KEYOFFSET, &wKeyTableOffset);
    }
    else {
        wKeyTableOffset = ET9KDBNAOFFSET_KEYOFFSET;
    }

    __ET9KDBREADWORD(dwOffset + (bAmbigFlag ? ET9KDBAOFFSET_BLOCKOFFSET : ET9KDBNAOFFSET_BLOCKOFFSET), &wBlockTableOffset);

    WLOG5(fprintf(pLogFile5, "      sub-region: bAmbigFlag        %5u\n", bAmbigFlag);)
    WLOG5(fprintf(pLogFile5, "      sub-region: wLeft             %5u\n", wLeft);)
    WLOG5(fprintf(pLogFile5, "      sub-region: wTop              %5u\n", wTop);)
    WLOG5(fprintf(pLogFile5, "      sub-region: wRight            %5u\n", wRight);)
    WLOG5(fprintf(pLogFile5, "      sub-region: wBottom           %5u\n", wBottom);)
    WLOG5(fprintf(pLogFile5, "      sub-region: bTotalKeys        %5u\n", bTotalKeys);)
    WLOG5(fprintf(pLogFile5, "      sub-region: bBlockRows        %5u\n", bBlockRows);)
    WLOG5(fprintf(pLogFile5, "      sub-region: bBlockCols        %5u\n", bBlockCols);)
    WLOG5(fprintf(pLogFile5, "      sub-region: wBlockWidth       %5u\n", wBlockWidth);)
    WLOG5(fprintf(pLogFile5, "      sub-region: wBlockHeight      %5u\n", wBlockHeight);)
    WLOG5(fprintf(pLogFile5, "      sub-region: wKeyTableOffset   %5u\n", wKeyTableOffset);)
    WLOG5(fprintf(pLogFile5, "      sub-region: wBlockTableOffset %5u\n", wBlockTableOffset);)
    WLOG5(fprintf(pLogFile5, "\n");)

    if (pKDBInfo->Private.pCurrLayoutInfo->nKeyAreaCount + bTotalKeys > ET9_KDB_MAX_KEYS) {
        return ET9STATUS_KDB_HAS_TOO_MANY_KEYS;
    }

    {
        ET9U8 bKeyIndex;

        for (bKeyIndex = 0; bKeyIndex < bTotalKeys; ++bKeyIndex) {

            const ET9U16 wKeyTableItemOffset = wKeyTableOffset + (bKeyIndex * 2);

            ET9STATUS eStatus;
            ET9U16 wKeyOffset;

            __ET9KDBREADWORD(dwOffset + wKeyTableItemOffset, &wKeyOffset);

            WLOG5(fprintf(pLogFile5, "      sub-region: key [%2u %2u] @ %u + %u\n\n", bKeyIndex, bKeyIndex + pKDBInfo->Private.pCurrLayoutInfo->nKeyAreaCount, dwOffset, wKeyOffset);)

            if (bAmbigFlag) {
                eStatus = __RegionalKey_StaticUtil(pKDBInfo, dwOffset + wKeyOffset, &pKeyAreas[bKeyIndex], &pKeyAreasExtra[bKeyIndex]);
            }
            else {
                eStatus = __DiscreteKey_StaticUtil(pKDBInfo, dwOffset + wKeyOffset, &pKeyAreas[bKeyIndex], &pKeyAreasExtra[bKeyIndex]);
            }

            if (eStatus) {
                return eStatus;
            }
        }
    }

    {
        ET9U8 bRowIndex;

        for (bRowIndex = 0; bRowIndex < bBlockRows; ++bRowIndex) {

            const ET9UINT nTop = wTop + bRowIndex * wBlockHeight;
            const ET9UINT nBottom = nTop + wBlockHeight - 1;

            ET9U8 bColIndex;

            for (bColIndex = 0; bColIndex < bBlockCols; ++bColIndex) {

                const ET9U16 wBlockTableItemOffset = wBlockTableOffset + (bRowIndex * (bBlockCols * 2)) + (bColIndex * 2);

                const ET9UINT nLeft = wLeft + bColIndex * wBlockWidth;
                const ET9UINT nRight = nLeft + wBlockWidth - 1;

                ET9STATUS eStatus;
                ET9U16 wBlockOffset;

                __ET9KDBREADWORD(dwOffset + wBlockTableItemOffset, &wBlockOffset);

                WLOG5B(fprintf(pLogFile5, "      sub-region: block [%2u,%2u] @ %u + %u\n\n", bRowIndex, bColIndex, dwOffset, wBlockOffset);)

                if (bAmbigFlag) {
                    eStatus = __RegionalBlock_StaticUtil(pKDBInfo, dwOffset + wBlockOffset, nLeft, nTop, nRight, nBottom, pKeyAreas, pKeyAreasExtra);
                }
                else {
                    eStatus = __DiscreteBlock_StaticUtil(pKDBInfo, dwOffset + wBlockOffset, nLeft, nTop, nRight, nBottom, pKeyAreas);
                }

                if (eStatus) {
                    return eStatus;
                }
            }
        }
    }

    pKDBInfo->Private.pCurrLayoutInfo->nKeyAreaCount += bTotalKeys;

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pKDBInfo                  Pointer to the Keyboard Input Module Information Data Structure (ET9KDBInfo).
 * @param dwOffset                  .
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

static ET9STATUS ET9LOCALCALL __RegionHeader_StaticUtil (ET9KDBInfo        * const pKDBInfo,
                                                         const ET9U32              dwOffset)
{
    ET9U16 wLeft;
    ET9U16 wTop;
    ET9U16 wRight;
    ET9U16 wBottom;
    ET9U8  bTotalSubRegions;

    ET9Assert(!pKDBInfo->Private.bUsingDynamicKDB);

    __ET9KDBREADWORD(dwOffset + ET9KDB_REGHDROFFSET_LEFT, &wLeft);
    __ET9KDBREADWORD(dwOffset + ET9KDB_REGHDROFFSET_TOP, &wTop);
    __ET9KDBREADWORD(dwOffset + ET9KDB_REGHDROFFSET_RIGHT, &wRight);
    __ET9KDBREADWORD(dwOffset + ET9KDB_REGHDROFFSET_BOTTOM, &wBottom);

    __ET9KDBREADBYTE(dwOffset + ET9KDB_REGHDROFFSET_TOTALREGIONS, &bTotalSubRegions);

    WLOG5(fprintf(pLogFile5, "    region: wLeft            %5u\n", wLeft);)
    WLOG5(fprintf(pLogFile5, "    region: wTop             %5u\n", wTop);)
    WLOG5(fprintf(pLogFile5, "    region: wRight           %5u\n", wRight);)
    WLOG5(fprintf(pLogFile5, "    region: wBottom          %5u\n", wBottom);)
    WLOG5(fprintf(pLogFile5, "    region: bTotalSubRegions %5u\n", bTotalSubRegions);)
    WLOG5(fprintf(pLogFile5, "\n");)

    {
        ET9U8 bIndex;
        ET9STATUS eStatus;

        for (bIndex = 0; bIndex < bTotalSubRegions; ++bIndex) {

            ET9U32 dwSubRegionHeaderOffset;

            __ET9KDBREADDWORD(dwOffset + ET9KDB_REGHDROFFSET_REGIONOFFSET + 4 * bIndex, &dwSubRegionHeaderOffset);

            WLOG5(fprintf(pLogFile5, "    region:  sub-region [%2u] @ %u\n\n", bIndex, dwSubRegionHeaderOffset);)

            eStatus = __SubRegionHeader_StaticUtil(pKDBInfo, dwSubRegionHeaderOffset);

            if (eStatus) {
                return eStatus;
            }
        }
    }

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pKDBInfo                  Pointer to the Keyboard Input Module Information Data Structure (ET9KDBInfo).
 * @param dwOffset                  .
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

static ET9STATUS ET9LOCALCALL __PageHeader_StaticUtil (ET9KDBInfo          * const pKDBInfo,
                                                       const ET9U32                dwOffset)
{
    ET9U16 wLeft;
    ET9U16 wTop;
    ET9U16 wRight;
    ET9U16 wBottom;
    ET9U8  bTotalRegions;

    ET9Assert(!pKDBInfo->Private.bUsingDynamicKDB);

    __ET9KDBREADWORD(dwOffset + ET9KDB_PAGEHDROFFSET_LEFT, &wLeft);
    __ET9KDBREADWORD(dwOffset + ET9KDB_PAGEHDROFFSET_TOP, &wTop);
    __ET9KDBREADWORD(dwOffset + ET9KDB_PAGEHDROFFSET_RIGHT, &wRight);
    __ET9KDBREADWORD(dwOffset + ET9KDB_PAGEHDROFFSET_BOTTOM, &wBottom);

    __ET9KDBREADBYTE(dwOffset + ET9KDB_PAGEHDROFFSET_TOTALREGIONS, &bTotalRegions);

    WLOG5(fprintf(pLogFile5, "  page: wLeft         %5u\n", wLeft);)
    WLOG5(fprintf(pLogFile5, "  page: wTop          %5u\n", wTop);)
    WLOG5(fprintf(pLogFile5, "  page: wRight        %5u\n", wRight);)
    WLOG5(fprintf(pLogFile5, "  page: wBottom       %5u\n", wBottom);)
    WLOG5(fprintf(pLogFile5, "  page: bTotalRegions %5u\n", bTotalRegions);)
    WLOG5(fprintf(pLogFile5, "\n");)

    {
        ET9U8 bIndex;
        ET9STATUS eStatus;

        for (bIndex = 0; bIndex < bTotalRegions; ++bIndex) {

            ET9U32 dwRegionHeaderOffset;

            __ET9KDBREADDWORD(dwOffset + ET9KDB_PAGEHDROFFSET_REGIONOFFSET + 4 * bIndex, &dwRegionHeaderOffset);

            WLOG5(fprintf(pLogFile5, "  page:  region [%2u] @ %u\n\n", bIndex, dwRegionHeaderOffset);)

            eStatus = __RegionHeader_StaticUtil(pKDBInfo, dwRegionHeaderOffset);

            if (eStatus) {
                return eStatus;
            }
        }
    }

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pKDBInfo                  Pointer to the Keyboard Input Module Information Data Structure (ET9KDBInfo).
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

static ET9STATUS ET9LOCALCALL __KdbHeader_StaticUtil (ET9KDBInfo * const pKDBInfo)
{
    ET9KdbLayoutInfo * const pLayoutInfo = pKDBInfo->Private.pCurrLayoutInfo;

    ET9U16 wTotalPages;
    ET9U16 wLayoutWidth;
    ET9U16 wLayoutHeight;
    ET9U16 wPageArrayOffset;

    ET9Assert(!pKDBInfo->Private.bUsingDynamicKDB);

    pLayoutInfo->wKdbNum = pKDBInfo->wKdbNum;
    pLayoutInfo->wPageNum = pKDBInfo->Private.wPageNum;

    __ET9KDBGETDATA(ET9KDBOFFSET_DATABASETYPE,      1, &pKDBInfo->Private.pCurrLayoutInfo->bDatabaseType);
    __ET9KDBGETDATA(ET9KDBOFFSET_LAYOUTVER,         1, &pKDBInfo->Private.pCurrLayoutInfo->bLayoutVer);
    __ET9KDBGETDATA(ET9KDBOFFSET_PRIMARYKEYBOARDID, 1, &pKDBInfo->Private.pCurrLayoutInfo->bPrimaryID);
    __ET9KDBGETDATA(ET9KDBOFFSET_SECONDKEYBOARDID,  1, &pKDBInfo->Private.pCurrLayoutInfo->bSecondaryID);
    __ET9KDBGETDATA(ET9KDBOFFSET_SYMBOLCLASS,       1, &pKDBInfo->Private.pCurrLayoutInfo->bSymbolClass);
    __ET9KDBGETDATA(ET9KDBOFFSET_CONTENTSMAJORVER,  1, &pKDBInfo->Private.pCurrLayoutInfo->bContentsMajor);
    __ET9KDBGETDATA(ET9KDBOFFSET_CONTENTSMINORVER,  1, &pKDBInfo->Private.pCurrLayoutInfo->bContentsMinor);

    __ET9KDBREADWORD(ET9KDBOFFSET_KDBWIDTH, &wLayoutWidth);
    __ET9KDBREADWORD(ET9KDBOFFSET_KDBHEIGHT, &wLayoutHeight);
    __ET9KDBREADWORD(ET9KDBOFFSET_TOTALPAGES, &wTotalPages);
    __ET9KDBREADWORD(ET9KDBOFFSET_PAGEARRAYOFFSET, &wPageArrayOffset);

    WLOG5(fprintf(pLogFile5, "header: wLayoutWidth     %5u\n", wLayoutWidth);)
    WLOG5(fprintf(pLogFile5, "header: wLayoutHeight    %5u\n", wLayoutHeight);)
    WLOG5(fprintf(pLogFile5, "header: wTotalPages      %5u\n", wTotalPages);)
    WLOG5(fprintf(pLogFile5, "header: wPageArrayOffset %5x\n", wPageArrayOffset);)
    WLOG5(fprintf(pLogFile5, "\n");)

    pKDBInfo->Private.pCurrLayoutInfo->nKeyAreaCount = 0;
    pKDBInfo->Private.pCurrLayoutInfo->nCharPoolCount = 0;

    {
        ET9STATUS eStatus;

        const ET9U16 wPageIndex = pLayoutInfo->wPageNum;

        ET9U32 dwPageHeaderOffset;

        if (wPageIndex >= wTotalPages) {
            return ET9STATUS_ERROR;
        }

        __ET9KDBREADDWORD(wPageArrayOffset + 4 * wPageIndex, &dwPageHeaderOffset);

        WLOG5(fprintf(pLogFile5, "header: page [%2u] @ %u\n\n", wPageIndex, dwPageHeaderOffset);)

        eStatus = __PageHeader_StaticUtil(pKDBInfo, dwPageHeaderOffset);

        if (eStatus) {
            return eStatus;
        }
    }

    pKDBInfo->Private.pCurrLayoutInfo->wLayoutWidth = wLayoutWidth;
    pKDBInfo->Private.pCurrLayoutInfo->wLayoutHeight = wLayoutHeight;

    ET9Assert(pKDBInfo->Private.wLayoutWidth == wLayoutWidth);
    ET9Assert(pKDBInfo->Private.wLayoutHeight == wLayoutHeight);

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pTst                      .
 * @param pSrc                      .
 *
 * @return Xxx
 */

ET9INLINE static ET9LOCALCALL ET9INT __SortBlocksIsHigherPrio_StaticUtil (ET9KdbBlockInfo * const pTst,
                                                                          ET9KdbBlockInfo * const pSrc)
{
    if (pTst->bProb < pSrc->bProb) {
        return 1;
    }
    if (pTst->bProb > pSrc->bProb) {
        return -1;
    }

    if (pTst->wY < pSrc->wY) {
        return 1;
    }
    if (pTst->wY > pSrc->wY) {
        return -1;
    }

    if (pTst->wX < pSrc->wX) {
        return 1;
    }
    if (pTst->wX > pSrc->wX) {
        return -1;
    }

    return 0;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pArea                     .
 *
 * @return None
 */

static void ET9LOCALCALL __KdbAreaSortBlocks_StaticUtil (ET9KdbAreaExtraInfo * const pArea)
{
    ET9BOOL bDirty;

    for (bDirty = 1; bDirty; ) {

        ET9UINT nIndex;

        bDirty = 0;

        for (nIndex = 0; nIndex + 1 < pArea->nBlockCount; ++nIndex) {

            ET9KdbBlockInfo * const pSrc = &pArea->pBlocks[nIndex];
            ET9KdbBlockInfo * const pTst = pSrc + 1;

            if (__SortBlocksIsHigherPrio_StaticUtil(pTst, pSrc) > 0) {

                ET9KdbBlockInfo sTmp;

                sTmp = *pSrc;
                *pSrc = *pTst;
                *pTst = sTmp;

                bDirty = 1;
            }
        }
    }
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pfValues                  .
 * @param nCount                    .
 *
 * @return None
 */

static void ET9LOCALCALL __KdbSortFloats_StaticUtil (ET9FLOAT       * const pfValues,
                                                     const ET9UINT          nCount)
{
    ET9BOOL bDirty;

    for (bDirty = 1; bDirty; ) {

        ET9UINT nIndex;

        bDirty = 0;

        for (nIndex = 0; nIndex + 1 < nCount; ++nIndex) {

            if (pfValues[nIndex] > pfValues[nIndex + 1]) {

                ET9FLOAT fTmp;

                fTmp = pfValues[nIndex];
                pfValues[nIndex] = pfValues[nIndex + 1];
                pfValues[nIndex + 1] = fTmp;

                bDirty = 1;
            }
        }
    }
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param snAx                      .
 * @param snAy                      .
 * @param snBx                      .
 * @param snBy                      .
 * @param snCx                      .
 * @param snCy                      .
 * @param pfCx                      .
 * @param pfCy                      .
 *
 * @return Xxx
 */

static ET9BOOL ET9LOCALCALL __KdbCalculateCircumCenter_StaticUtil (const ET9INT         snAx,
                                                                   const ET9INT         snAy,
                                                                   const ET9INT         snBx,
                                                                   const ET9INT         snBy,
                                                                   const ET9INT         snCx,
                                                                   const ET9INT         snCy,
                                                                   ET9FLOAT     * const pfCx,
                                                                   ET9FLOAT     * const pfCy)
{
    const ET9FLOAT fD = (ET9FLOAT)(2.0 * (snAx * (snBy - snCy) + snBx * (snCy - snAy) + snCx * (snAy - snBy)));

    if (fD == 0) {
        *pfCx = 0;
        *pfCy = 0;
        return 0;
    }

    *pfCx = (ET9FLOAT)(((snAy * snAy + snAx * snAx) * (snBy - snCy) +
                        (snBy * snBy + snBx * snBx) * (snCy - snAy) +
                        (snCy * snCy + snCx * snCx) * (snAy - snBy)) / fD);

    *pfCy = (ET9FLOAT)(((snAy * snAy + snAx * snAx) * (snCx - snBx) +
                        (snBy * snBy + snBx * snBx) * (snAx - snCx) +
                        (snCy * snCy + snCx * snCx) * (snBx - snAx)) / fD);

    return 1;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pArea                     .
 * @param pAreaExtra                .
 * @param pfBestRadius              .
 *
 * @return Xxx
 */

static void ET9LOCALCALL __KdbFinalizeArea_StaticUtil (ET9KdbAreaInfo      * const pArea,
                                                       ET9KdbAreaExtraInfo * const pAreaExtra,
                                                       ET9FLOAT            * const pfBestRadius)
{
    ET9UINT nIndex;
    ET9UINT nTriCount;

    ET9FLOAT fRadius;
    ET9FLOAT fCenterX;
    ET9FLOAT fCenterY;
    ET9UINT  nCenterCount;

    ET9UINT nMedianCount;
    ET9FLOAT pfRadius[ET9_KDB_MAX_KEY_BLOCKS];
    ET9FLOAT pfMedianX[ET9_KDB_MAX_KEY_BLOCKS];
    ET9FLOAT pfMedianY[ET9_KDB_MAX_KEY_BLOCKS];

    __KdbAreaSortBlocks_StaticUtil(pAreaExtra);

    WLOG5(fprintf(pLogFile5, "\n__KdbFinalizeArea_StaticUtil %p %c\n\n", pArea, pArea->eKeyType == ET9KTFUNCTION ? '~' : (char)pArea->psChars[0]);)

    for (nIndex = 0; nIndex < pAreaExtra->nBlockCount; ++nIndex) {
        WLOG5(fprintf(pLogFile5, "  %3u @ %3u %3u\n", pAreaExtra->pBlocks[nIndex].bProb, pAreaExtra->pBlocks[nIndex].wX, pAreaExtra->pBlocks[nIndex].wY);)
    }

    nTriCount = 0;
    nCenterCount = 0;
    nMedianCount = 0;
    fRadius = 0;
    fCenterX = 0;
    fCenterY = 0;

    for (nIndex = 0; nIndex + 2 < pAreaExtra->nBlockCount; ++nIndex) {

        if (pAreaExtra->pBlocks[nIndex].bProb == pAreaExtra->pBlocks[nIndex + 1].bProb &&
            pAreaExtra->pBlocks[nIndex].bProb == pAreaExtra->pBlocks[nIndex + 2].bProb) {

            ET9BOOL bOk;
            ET9FLOAT fCx;
            ET9FLOAT fCy;
            ET9FLOAT fR;

            bOk = __KdbCalculateCircumCenter_StaticUtil(pAreaExtra->pBlocks[nIndex + 0].wX,
                                                        pAreaExtra->pBlocks[nIndex + 0].wY,
                                                        pAreaExtra->pBlocks[nIndex + 1].wX,
                                                        pAreaExtra->pBlocks[nIndex + 1].wY,
                                                        pAreaExtra->pBlocks[nIndex + 2].wX,
                                                        pAreaExtra->pBlocks[nIndex + 2].wY,
                                                        &fCx,
                                                        &fCy);

            if (bOk) {
                fR = _ET9sqrt_f((fCx - pAreaExtra->pBlocks[nIndex + 0].wX) * (fCx - pAreaExtra->pBlocks[nIndex + 0].wX) +
                                (fCy - pAreaExtra->pBlocks[nIndex + 0].wY) * (fCy - pAreaExtra->pBlocks[nIndex + 0].wY)) *
                                255 / (255 - pAreaExtra->pBlocks[nIndex + 0].bProb);
            }
            else {
                fR = 0;
            }

            WLOG5(fprintf(pLogFile5, "  CC ok %1u  cx %5.1f cy %5.1f  R %5.1f\n", bOk, fCx, fCy, fR);)

            if (bOk) {

                pfRadius[nMedianCount] = fR;
                pfMedianX[nMedianCount] = fCx;
                pfMedianY[nMedianCount] = fCy;
                ++nMedianCount;

                fRadius += fR;
                fCenterX += fCx;
                fCenterY += fCy;
                ++nCenterCount;
            }

            ++nTriCount;
        }
    }

    if (nMedianCount > 2) {

        __KdbSortFloats_StaticUtil(pfRadius, nMedianCount);
        __KdbSortFloats_StaticUtil(pfMedianX, nMedianCount);
        __KdbSortFloats_StaticUtil(pfMedianY, nMedianCount);

        nCenterCount = 1;

        fRadius = pfRadius[nMedianCount / 2];
        fCenterX = pfMedianX[nMedianCount / 2];
        fCenterY = pfMedianY[nMedianCount / 2];
    }

    if (pAreaExtra->wMaxProbCenterCount) {

        WLOG5(fprintf(pLogFile5, "  PC       cx %5.1f cy %5.1f  P %3u\n",
                                 (float)pAreaExtra->wMaxProbCenterX / pAreaExtra->wMaxProbCenterCount,
                                 (float)pAreaExtra->wMaxProbCenterY / pAreaExtra->wMaxProbCenterCount,
                                 (int)pAreaExtra->bMaxProbCenter);)

        if (!nCenterCount) {
            nCenterCount = pAreaExtra->wMaxProbCenterCount;
            fRadius = 0;
            fCenterX = pAreaExtra->wMaxProbCenterX;
            fCenterY = pAreaExtra->wMaxProbCenterY;
        }
    }

    WLOG5(fprintf(pLogFile5, "Tri count %c %3u (%3u)\n", pArea->eKeyType == ET9KTFUNCTION ? '~' : (char)pArea->psChars[0], nTriCount, pAreaExtra->nBlockCount);)

    if (nCenterCount) {
        *pfBestRadius = (ET9FLOAT)(fRadius / nCenterCount);
        pArea->nCenterX = (ET9UINT)((fCenterX + 0.5) / nCenterCount);
        pArea->nCenterY = (ET9UINT)((fCenterY + 0.5) / nCenterCount);
    }
    else {
        *pfBestRadius = 0;
    }
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pKDBInfo                  .
 *
 * @return Xxx
 */

static void ET9LOCALCALL __KdbFinalizeAreas_StaticUtil (ET9KDBInfo * const pKDBInfo)
{
    ET9UINT nIndex;
    ET9UINT nRadiusCount;
    ET9FLOAT pfRadius[ET9_KDB_MAX_KEY_BLOCKS];

    ET9Assert(!pKDBInfo->Private.bUsingDynamicKDB);

    nRadiusCount = 0;

    for (nIndex = 0; nIndex < pKDBInfo->Private.pCurrLayoutInfo->nKeyAreaCount; ++nIndex) {

        __KdbFinalizeArea_StaticUtil(&pKDBInfo->Private.pCurrLayoutInfo->pKeyAreas[nIndex],
                                     &pKDBInfo->Private.wm.staticLoad.sLayoutExtraInfo.pKeyAreas[nIndex],
                                     &pfRadius[nRadiusCount]);

        if (pfRadius[nRadiusCount]) {
            ++nRadiusCount;
        }
    }

    if (nRadiusCount) {
        pKDBInfo->Private.pCurrLayoutInfo->nRadius = (ET9UINT)(pfRadius[nRadiusCount/2] + 0.5);
    }
    else {
        pKDBInfo->Private.pCurrLayoutInfo->nRadius = 0;
    }

    WLOG5(fprintf(pLogFile5, "Radius %3u (from blocks)\n", pKDBInfo->Private.pCurrLayoutInfo->nRadius);)
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pKDBInfo                  .
 *
 * @return Xxx
 */

static void ET9LOCALCALL __KdbCleanupKeyTypes_StaticUtil (ET9KDBInfo * const pKDBInfo)
{
    ET9KdbLayoutInfo * const pLayoutInfo = pKDBInfo->Private.pCurrLayoutInfo;

    ET9UINT nIndex;

    ET9Assert(!pKDBInfo->Private.bUsingDynamicKDB);

    for (nIndex = 0; nIndex < pLayoutInfo->nKeyAreaCount; ++nIndex) {
        pKDBInfo->Private.pCurrLayoutInfo->pKeyAreas[nIndex].wKeyIndex = (ET9U16)nIndex;
    }

    for (nIndex = 0; nIndex < pLayoutInfo->nKeyAreaCount; ++nIndex) {

        ET9KdbAreaInfo * const pKeyArea = &pLayoutInfo->pKeyAreas[nIndex];

        switch (pKeyArea->eKeyType)
        {
            case ET9KTLETTER:
            case ET9KTPUNCTUATION:
            case ET9KTNUMBER:
            case ET9KTSMARTPUNCT:
            case ET9KTSTRING:
            case ET9KTFUNCTION:
                continue;
            default:
                break;
        }

        WLOG5(fprintf(pLogFile5, "Removing key %2u\n", pKeyArea->wKeyIndex);)

        --pLayoutInfo->nKeyAreaCount;
        pLayoutInfo->pKeyAreas[nIndex] = pLayoutInfo->pKeyAreas[pLayoutInfo->nKeyAreaCount];
        --nIndex;
    }

#ifdef ET9_DEBUG
    {
        ET9BOOL pbUsed[ET9_KDB_MAX_KEYS];

        _ET9ClearMem((ET9U8*)pbUsed, sizeof(pbUsed));

        for (nIndex = 0; nIndex < pLayoutInfo->nKeyAreaCount; ++nIndex) {

            ET9KdbAreaInfo * const pKeyArea = &pLayoutInfo->pKeyAreas[nIndex];

            ET9Assert(pKeyArea->wKeyIndex < ET9_KDB_MAX_KEYS);

            ++pbUsed[pKeyArea->wKeyIndex];

            ET9Assert(pbUsed[pKeyArea->wKeyIndex] == 1);
        }
    }
#endif
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pKDBInfo                  .
 *
 * @return Xxx
 */

static void ET9LOCALCALL __KdbCalculateKeySize_StaticUtil (ET9KDBInfo * const pKDBInfo)
{
    ET9KdbLayoutInfo * const pLayoutInfo = pKDBInfo->Private.pCurrLayoutInfo;

    ET9UINT nWidthCount = 0;
    ET9UINT nHeightCount = 0;
    ET9U16 pwWidths[ET9_KDB_MAX_KEYS];
    ET9U16 pwHeights[ET9_KDB_MAX_KEYS];

    ET9UINT nIndex;

    ET9Assert(!pKDBInfo->Private.bUsingDynamicKDB);

    for (nIndex = 0; nIndex < pLayoutInfo->nKeyAreaCount; ++nIndex) {

        const ET9UINT nX1 = pLayoutInfo->pKeyAreas[nIndex].nCenterX;
        const ET9UINT nY1 = pLayoutInfo->pKeyAreas[nIndex].nCenterY;

        ET9UINT nLook;
        ET9UINT nBestWidth = 0;
        ET9UINT nBestHeight = 0;

        if (pLayoutInfo->pKeyAreas[nIndex].eKeyType == ET9KTFUNCTION) {
            continue;
        }

        if (pLayoutInfo->pKeyAreas[nIndex].bRegionOk) {

            ET9Region *pRegion = &pLayoutInfo->pKeyAreas[nIndex].sRegion;

            pwWidths[nWidthCount++] = pRegion->wRight - pRegion->wLeft + 1;
            pwHeights[nHeightCount++] = pRegion->wBottom - pRegion->wTop + 1;

            continue;
        }

        for (nLook = nIndex + 1; nLook < pLayoutInfo->nKeyAreaCount; ++nLook) {

            if (pLayoutInfo->pKeyAreas[nLook].eKeyType != pLayoutInfo->pKeyAreas[nIndex].eKeyType) {
                continue;
            }

            {
                const ET9UINT nX2 = pLayoutInfo->pKeyAreas[nLook].nCenterX;
                const ET9UINT nY2 = pLayoutInfo->pKeyAreas[nLook].nCenterY;

                const ET9UINT nXDiff = (nX1 > nX2) ? (nX1 - nX2) : (nX2 - nX1);
                const ET9UINT nYDiff = (nY1 > nY2) ? (nY1 - nY2) : (nY2 - nY1);

                if (nXDiff >= nYDiff) {
                    if (!nBestWidth || nBestWidth > nXDiff) {
                        nBestWidth = nXDiff;
                    }
                }

                if (nYDiff >= nXDiff) {
                    if (!nBestHeight || nBestHeight > nYDiff) {
                        nBestHeight = nYDiff;
                    }
                }
            }
        }

        if (nBestWidth) {
            pwWidths[nWidthCount++] = (ET9U16)nBestWidth;
        }
        if (nBestHeight) {
            pwHeights[nHeightCount++] = (ET9U16)nBestHeight;
        }
    }

    /* save key info */

    if (nWidthCount && nHeightCount) {

        __SortU16Array(pwWidths, nWidthCount);
        __SortU16Array(pwHeights, nHeightCount);

        pLayoutInfo->nMinKeyWidth = pwWidths[0];
        pLayoutInfo->nMinKeyHeight = pwHeights[0];

        pLayoutInfo->nMedianKeyWidth = pwWidths[nWidthCount / 2];
        pLayoutInfo->nMedianKeyHeight = pwHeights[nHeightCount / 2];
    }
    else {

        WLOG5(fprintf(pLogFile5, "  no valid keys\n");)

        pLayoutInfo->nMinKeyWidth = 10;
        pLayoutInfo->nMinKeyHeight = 10;

        pLayoutInfo->nMedianKeyWidth = 10;
        pLayoutInfo->nMedianKeyHeight = 10;
    }

    /* estimate radius? */

    if (!pLayoutInfo->nRadius) {
        pLayoutInfo->nRadius = pLayoutInfo->nMedianKeyWidth + pLayoutInfo->nMedianKeyHeight;
    }

    /* create estimated key regions when missing */

    for (nIndex = 0; nIndex < pLayoutInfo->nKeyAreaCount; ++nIndex) {

        ET9KdbAreaInfo * const pArea = &pLayoutInfo->pKeyAreas[nIndex];

        if (!pArea->bRegionOk) {
            __CreateRegion(&pArea->sRegion, pArea->nCenterX, pArea->nCenterY, pLayoutInfo->nMedianKeyWidth, pLayoutInfo->nMedianKeyHeight);
        }
    }
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pKDBInfo                  .
 *
 * @return Xxx
 */

static ET9STATUS ET9LOCALCALL __GetKdbAreas_StaticUtil (ET9KDBInfo * const pKDBInfo)
{
    ET9STATUS eStatus;

    ET9Assert(!pKDBInfo->Private.bUsingDynamicKDB);

    eStatus = __KdbHeader_StaticUtil(pKDBInfo);

    WLOG5(fprintf(pLogFile5, "__GetKdbAreas_StaticUtil, __KdbHeader status %u\n\n", eStatus);)

    if (eStatus) {
        return eStatus;
    }

    __KdbFinalizeAreas_StaticUtil(pKDBInfo);
    __KdbCleanupKeyTypes_StaticUtil(pKDBInfo);
    __KdbCalculateKeySize_StaticUtil(pKDBInfo);

    __CalculateOverlap_Generic(pKDBInfo);

    WLOG5(fprintf(pLogFile5, "Radius %u\n", pKDBInfo->Private.pCurrLayoutInfo->nRadius);)

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pKDBInfo                  .
 *
 * @return Xxx
 */

static ET9STATUS ET9LOCALCALL __KeyAreasUpdate_StaticUtil (ET9KDBInfo       * const pKDBInfo)
{
    ET9STATUS eStatus;

    ET9Assert(pKDBInfo);
    ET9Assert(!pKDBInfo->Private.bUsingDynamicKDB);

    /* check cache */

    {
        ET9UINT nCount;
        ET9KdbLayoutInfo *pLayoutInfo;

        pLayoutInfo = &pKDBInfo->Private.pLayoutInfos[0];
        for (nCount = ET9_KDB_MAX_PAGE_CACHE; nCount; --nCount, ++pLayoutInfo) {

            if (pLayoutInfo->bOk && !pLayoutInfo->bDynamic && pLayoutInfo->wKdbNum == pKDBInfo->wKdbNum && pLayoutInfo->wPageNum == pKDBInfo->Private.wPageNum) {

                WLOG5(fprintf(pLogFile5, "__KeyAreasUpdate_StaticUtil, using cache, wKdbNum %2x, wPageNum %2u\n", pLayoutInfo->wKdbNum, pLayoutInfo->wPageNum);)

                if (pKDBInfo->Private.pCurrLayoutInfo != pLayoutInfo) {
                    ++pKDBInfo->Private.nLoadID;
                }

                pKDBInfo->Private.pCurrLayoutInfo = pLayoutInfo;

                return ET9STATUS_NONE;
            }
        }
    }

#ifdef ET9_ACTIVATE_KEY_AREA_UPDATE_CONTROL

    /* inhibit? */

    if (bInhibitStaticKeyAreaInfo) {
        _ET9ClearMem((ET9U8*)&pKDBInfo->Private.pLayoutInfos, sizeof(pKDBInfo->Private.pLayoutInfos));
        return ET9STATUS_NONE;
    }

#endif

    /* going to use WM */

    ++pKDBInfo->Private.nWmUseID;

    /* pick cache */

    ++pKDBInfo->Private.pLastLayoutInfo;

    if (pKDBInfo->Private.pLastLayoutInfo - pKDBInfo->Private.pLayoutInfos >= ET9_KDB_MAX_PAGE_CACHE) {
        pKDBInfo->Private.pLastLayoutInfo = &pKDBInfo->Private.pLayoutInfos[0];
    }

    pKDBInfo->Private.pCurrLayoutInfo = pKDBInfo->Private.pLastLayoutInfo;

    /* actual load */

    WLOG5(fprintf(pLogFile5, "__KeyAreasUpdate_StaticUtil, actual load, wKdbNum %2x, wPageNum %2u\n", pKDBInfo->wKdbNum, pKDBInfo->Private.wPageNum);)

    ++pKDBInfo->Private.nLoadID;

    pKDBInfo->Private.pCurrLayoutInfo->bOk = 0;
    pKDBInfo->Private.pCurrLayoutInfo->bDynamic = 0;

    eStatus = __GetKdbAreas_StaticUtil(pKDBInfo);

    if (eStatus) {
        return eStatus;
    }

    /* some verification */

    if (pKDBInfo->Private.pCurrLayoutInfo->nKeyAreaCount) {

        if (!pKDBInfo->Private.pCurrLayoutInfo->nRadius) {
            return ET9STATUS_ERROR;
        }

        if (!pKDBInfo->Private.pCurrLayoutInfo->nMinKeyWidth || !pKDBInfo->Private.pCurrLayoutInfo->nMinKeyHeight) {
            return ET9STATUS_ERROR;
        }

        if (!pKDBInfo->Private.pCurrLayoutInfo->nMedianKeyWidth || !pKDBInfo->Private.pCurrLayoutInfo->nMedianKeyHeight) {
            return ET9STATUS_ERROR;
        }
    }

    /* loaded ok */

    pKDBInfo->Private.pCurrLayoutInfo->bOk = 1;

    /* done */

    return ET9STATUS_NONE;
}

/* ************************************************************************************************************** */
/* * GENERIC ***************************************************************************************************** */
/* ************************************************************************************************************** */

/*---------------------------------------------------------------------------*/
/** \internal
 * This function will calculate the (min) overlap.
 *
 * @param pKDBInfo     Pointer to keyboard information structure.
 *
 * @return None
 */

static void ET9LOCALCALL __CalculateOverlap_Generic (ET9KDBInfo  * const pKDBInfo)
{
    ET9KdbLayoutInfo * const pLayoutInfo = pKDBInfo->Private.pCurrLayoutInfo;

    ET9UINT nCount;
    ET9KdbAreaInfo *pArea;

    ET9UINT nMinWidth = 0;
    ET9UINT nMinHeight = 0;
    ET9UINT nMinOverlap = 0;

    WLOG5(fprintf(pLogFile5, "__CalculateOverlap_Generic\n");)

    /* calculate min box */

    pArea = pLayoutInfo->pKeyAreas;
    for (nCount = pLayoutInfo->nKeyAreaCount; nCount; --nCount, ++pArea) {

        ET9Region sThisRegion;

        ET9UINT nNeighborCount;
        ET9KdbAreaInfo *pNeighborArea;

        if (pArea->eKeyType == ET9KTFUNCTION) {
            continue;
        }

        WLOG5B(fprintf(pLogFile5, "  area %2u, nCenterX %5u, nCenterY %5u\n", (int)(pArea - pLayoutInfo->pKeyAreas), pArea->nCenterX, pArea->nCenterY);)

        __CreateRegion(&sThisRegion, pArea->nCenterX, pArea->nCenterY, pLayoutInfo->nMedianKeyWidth * 2 - 1, pLayoutInfo->nMedianKeyHeight * 2 - 1);

        pNeighborArea = pLayoutInfo->pKeyAreas;
        for (nNeighborCount = pLayoutInfo->nKeyAreaCount; nNeighborCount; --nNeighborCount, ++pNeighborArea) {

            if (pNeighborArea == pArea) {
                continue;
            }

            if (pNeighborArea->eKeyType != pArea->eKeyType) {
                continue;
            }

            if (!__GetRegionOverlap(&sThisRegion, &pNeighborArea->sRegion)) {
                continue;
            }

            WLOG5B(fprintf(pLogFile5, "    neighbor %2u, wLeft %5u, wTop %5u, wRight %5u, wBottom %5u\n", (int)(pNeighborArea - pLayoutInfo->pKeyAreas), pNeighborArea->sRegion.wLeft, pNeighborArea->sRegion.wTop, pNeighborArea->sRegion.wRight, pNeighborArea->sRegion.wBottom);)

            {
                ET9UINT nW = 0;
                ET9UINT nH = 0;

                if (pNeighborArea->sRegion.wRight < pArea->nCenterX) {
                    nW = pArea->nCenterX - pNeighborArea->sRegion.wRight;
                }
                else if (pNeighborArea->sRegion.wLeft > pArea->nCenterX) {
                    nW = pNeighborArea->sRegion.wLeft - pArea->nCenterX;
                }

                if (pNeighborArea->sRegion.wBottom < pArea->nCenterY) {
                    nH = pArea->nCenterY - pNeighborArea->sRegion.wBottom;
                }
                else if (pNeighborArea->sRegion.wTop > pArea->nCenterY) {
                    nH = pNeighborArea->sRegion.wTop - pArea->nCenterY;
                }

                WLOG5B(fprintf(pLogFile5, "      nW %5u, nH %5u\n", nW, nH);)

                if (nMinWidth < nW) {
                    nMinWidth = nW;
                }
                if (nMinHeight < nH) {
                    nMinHeight = nH;
                }
            }
        }
    }

    {
        const ET9UINT nBoxWidth = (ET9UINT)(nMinWidth * 2 * 1.3 + 1);
        const ET9UINT nBoxHeight = (ET9UINT)(nMinHeight * 2 * 1.3 + 1);

        const ET9UINT nDefaultWidth = pLayoutInfo->nMedianKeyWidth * 2 - 1;
        const ET9UINT nDefaultHeight = pLayoutInfo->nMedianKeyHeight * 2 - 1;

        if (nMinWidth && nBoxWidth < nDefaultWidth) {
            pLayoutInfo->nBoxWidth = nBoxWidth;
        }
        else {
            pLayoutInfo->nBoxWidth = nDefaultWidth;
        }

        if (nMinHeight && nBoxHeight < nDefaultHeight) {
            pLayoutInfo->nBoxHeight = nBoxHeight;
        }
        else {
            pLayoutInfo->nBoxHeight = nDefaultHeight;
        }

        WLOG5(fprintf(pLogFile5, "  box width %2u (%2u), height %2u (%2u)\n", pLayoutInfo->nBoxWidth, nDefaultWidth, pLayoutInfo->nBoxHeight, nDefaultHeight);)
    }

    /* calculate min overlap */

    pArea = pLayoutInfo->pKeyAreas;
    for (nCount = pLayoutInfo->nKeyAreaCount; nCount; --nCount, ++pArea) {

        ET9Region sThisRegion;

        ET9UINT nNeighborCount;
        ET9KdbAreaInfo *pNeighborArea;

        if (pArea->eKeyType == ET9KTFUNCTION) {
            continue;
        }

        __CreateRegion(&sThisRegion, pArea->nCenterX, pArea->nCenterY, pLayoutInfo->nBoxWidth, pLayoutInfo->nBoxHeight);

        pNeighborArea = pLayoutInfo->pKeyAreas;
        for (nNeighborCount = pLayoutInfo->nKeyAreaCount; nNeighborCount; --nNeighborCount, ++pNeighborArea) {

            if (pNeighborArea == pArea) {
                continue;
            }

            if (pNeighborArea->eKeyType != pArea->eKeyType) {
                continue;
            }

            {
                const ET9UINT nOverlap = __GetRegionOverlap(&sThisRegion, &pNeighborArea->sRegion);

                if (nOverlap) {

                    if (!nMinOverlap || nMinOverlap > nOverlap) {
                        nMinOverlap = nOverlap;
                    }
                }
            }
        }
    }

    pLayoutInfo->nMinKeyOverlap = nMinOverlap / 2;

    WLOG5(fprintf(pLogFile5, "  min key overlap %u\n", pLayoutInfo->nMinKeyOverlap);)

#ifdef ET9_DEBUG

    /* validate */

    pArea = pLayoutInfo->pKeyAreas;
    for (nCount = pLayoutInfo->nKeyAreaCount; nCount; --nCount, ++pArea) {

        ET9Region sBoxRegion;
        ET9Region sMedianRegion;

        ET9UINT nNeighborCount;
        ET9KdbAreaInfo *pNeighborArea;

        if (pArea->eKeyType == ET9KTFUNCTION) {
            continue;
        }

        __CreateRegion(&sBoxRegion, pArea->nCenterX, pArea->nCenterY, pLayoutInfo->nBoxWidth, pLayoutInfo->nBoxHeight);
        __CreateRegion(&sMedianRegion, pArea->nCenterX, pArea->nCenterY, pLayoutInfo->nMedianKeyWidth * 2 - 1, pLayoutInfo->nMedianKeyHeight * 2 - 1);

        pNeighborArea = pLayoutInfo->pKeyAreas;
        for (nNeighborCount = pLayoutInfo->nKeyAreaCount; nNeighborCount; --nNeighborCount, ++pNeighborArea) {

            if (pNeighborArea == pArea) {
                continue;
            }

            if (pNeighborArea->eKeyType != pArea->eKeyType) {
                continue;
            }

            {
                const ET9UINT nBoxOverlap = __GetRegionOverlap(&sBoxRegion, &pNeighborArea->sRegion);
                const ET9UINT nMedianOverlap = __GetRegionOverlap(&sMedianRegion, &pNeighborArea->sRegion);

                if (!nBoxOverlap && nMedianOverlap) {
                    ET9Assert(0);
                }
            }
        }
    }

#endif
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Finds key info from a key index.
 *
 * @param[in]     pKDBInfo          Pointer to the Keyboard Input Module Information Data Structure (ET9KDBInfo).
 * @param[in]     wKey              Key index.
 *
 * @return Returns pointer to the corresponding key index.
 */

ET9INLINE static ET9KdbAreaInfo const * ET9LOCALCALL __GetKeyAreaFromKey_Generic(ET9KDBInfo    * const pKDBInfo,
                                                                                 const ET9U16          wKey)
{
    ET9KdbLayoutInfo * const pLayoutInfo = pKDBInfo->Private.pCurrLayoutInfo;

    ET9UINT nCount;
    ET9KdbAreaInfo const * pArea;

    pArea = pLayoutInfo->pKeyAreas;
    for (nCount = pLayoutInfo->nKeyAreaCount; nCount; --nCount, ++pArea) {

        /* include function keys */

        if (pArea->wKeyIndex == wKey) {
            return pArea;
        }
    }

    return NULL;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Finds key info from a coordinate (only works when there are regions available).
 *
 * @param[in]     pKDBInfo          Pointer to the Keyboard Input Module Information Data Structure (ET9KDBInfo).
 * @param[in]     wX                Horizontal (x) coordinate of the tap, integration coordinate space.
 * @param[in]     wY                Vertical (y) coordinate of the tap, integration coordinate space.
 *
 * @return Returns pointer to the corresponding key index.
 */

ET9INLINE static ET9KdbAreaInfo const * ET9LOCALCALL __GetKeyAreaFromTap_Generic(ET9KDBInfo    * const pKDBInfo,
                                                                                 const ET9U16          wX,
                                                                                 const ET9U16          wY)
{
    ET9KdbLayoutInfo * const pLayoutInfo = pKDBInfo->Private.pCurrLayoutInfo;

    ET9UINT nCount;
    ET9KdbAreaInfo const * pArea;

    pArea = pLayoutInfo->pKeyAreas;
    for (nCount = pLayoutInfo->nKeyAreaCount; nCount; --nCount, ++pArea) {

        /* include function keys */

        if (!pArea->bRegionOk) {
            continue;
        }

        if (wX >= pArea->sRegion.wLeft && wX <= pArea->sRegion.wRight && wY >= pArea->sRegion.wTop && wY <= pArea->sRegion.wBottom) {
            return pArea;
        }
    }

    return NULL;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * This function finds the first tap char.
 *
 * @param pKDBInfo             Pointer to keyboard information structure.
 * @param pDirectedPos         Tap position.
 * @param psFirstChar          Pointer to first char symbol.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

static ET9STATUS ET9LOCALCALL __GetFirstTapChar_Generic(ET9KDBInfo            * const pKDBInfo,
                                                        ET9DirectedPos        * const pDirectedPos,
                                                        ET9SYMB               * const psFirstChar)
{
    ET9KdbLayoutInfo * const pLayoutInfo = pKDBInfo->Private.pCurrLayoutInfo;

    const ET9UINT nX = pDirectedPos->sPos.nX;
    const ET9UINT nY = pDirectedPos->sPos.nY;

    const ET9FLOAT fX = (ET9FLOAT)nX;
    const ET9FLOAT fY = (ET9FLOAT)nY;

    ET9UINT nCount;
    ET9KdbAreaInfo const * pArea;

    ET9FLOAT fBestSQ = 0;

    *psFirstChar = 0;

    pArea = pLayoutInfo->pKeyAreas;
    for (nCount = pLayoutInfo->nKeyAreaCount; nCount; --nCount, ++pArea) {

        /* allow function keys (needed to to be able to tap a FK) */

        /* Use key regions when available, otherwise the closest key */

        if (!pArea->bRegionOk) {

            const ET9FLOAT fDistSQ = (pArea->nCenterX - fX) * (pArea->nCenterX - fX) + (pArea->nCenterY - fY) * (pArea->nCenterY - fY);

            if (!*psFirstChar || fBestSQ > fDistSQ) {
                fBestSQ = fDistSQ;
                *psFirstChar = pArea->psChars[0];
            }
        }
        else if (nX >= pArea->sRegion.wLeft && nX <= pArea->sRegion.wRight && nY >= pArea->sRegion.wTop && nY <= pArea->sRegion.wBottom) {

            *psFirstChar = pArea->psChars[0];
            return ET9STATUS_NONE;
        }
    }

    if (!*psFirstChar) {
        return ET9STATUS_ERROR;
    }

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * This function retrieves ambig/non-ambig region info for symbol.
 *
 * @param pKDBInfo             Pointer to keyboard information structure.
 * @param sSymbol              Symbol to look up.
 * @param pbyRegionalKey       Pointer to indicate if the found key is regional.
 * @param pwKeyIndex           Pointer to store key index in.
 * @param pKeyRegion           Pointer to store region info in.
 * @param bInitialSymCheck     Set if only matching on first sym assigned to key; false if checking ALL key syms.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

static ET9STATUS ET9LOCALCALL __FindSymbol_Generic(ET9KDBInfo        * const pKDBInfo,
                                                   const ET9SYMB             sSymbol,
                                                   ET9U8             * const pbyRegionalKey,
                                                   ET9U16            * const pwKeyIndex,
                                                   ET9Region         * const pKeyRegion,
                                                   const ET9BOOL             bInitialSymCheck)
{
    ET9KdbLayoutInfo * const pLayoutInfo = pKDBInfo->Private.pCurrLayoutInfo;

    ET9UINT nCount;
    ET9KdbAreaInfo const * pArea;
    ET9KdbAreaInfo const * pBestArea = NULL;

    pArea = pLayoutInfo->pKeyAreas;
    for (nCount = pLayoutInfo->nKeyAreaCount; nCount; --nCount, ++pArea) {

        /* skip function keys */

        if (pArea->eKeyType == ET9KTFUNCTION) {
            continue;
        }

        {
            ET9SYMB *psSymb;
            ET9UINT nCharCount;

            psSymb = pArea->psChars;
            for (nCharCount = (bInitialSymCheck ? 1 : pArea->nCharCount); nCharCount; --nCharCount, ++psSymb) {
                if (sSymbol == *psSymb) {
                    pBestArea = pArea;
                    break;
                }
            }

            /* initial? */

            if (nCharCount && nCharCount == pArea->nCharCount) {
                break;
            }
        }

        /* potentially handle nShiftedCharCount/psShiftedChars here */
    }

    if (!pBestArea) {
        return ET9STATUS_ERROR;
    }

    *pwKeyIndex = pBestArea->wKeyIndex;
    *pKeyRegion = pBestArea->sRegion;
    *pbyRegionalKey = (pBestArea->eInputType == ET9REGIONALKEY) ? 1 : 0;

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * This function computes checksum for the KDB content.
 *
 * @param pKDBInfo             Pointer to keyboard information structure.
 *
 * @return Checksum
 */

static ET9U32 ET9LOCALCALL __ComputeContentChecksum_Generic(ET9KDBInfo   * const pKDBInfo)
{
    ET9KdbLayoutInfo * const pLayoutInfo = pKDBInfo->Private.pCurrLayoutInfo;

    ET9U32 dwChecksum = 0;

    ET9UINT nCount;
    ET9KdbAreaInfo const * pArea;

    dwChecksum = pLayoutInfo->wLayoutWidth + (65599 * dwChecksum);
    dwChecksum = pLayoutInfo->wLayoutHeight + (65599 * dwChecksum);

    pArea = pLayoutInfo->pKeyAreas;
    for (nCount = pLayoutInfo->nKeyAreaCount; nCount; --nCount, ++pArea) {

        /* include function keys */

        dwChecksum = pArea->wKeyIndex + (65599 * dwChecksum);
        dwChecksum = pArea->eKeyType + (65599 * dwChecksum);
        dwChecksum = pArea->eInputType + (65599 * dwChecksum);
    }

    return dwChecksum;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Retrieves version information for the active keyboards.
 *
 * @param[in]     pKDBInfo      Pointer to the Keyboard Input Module Information Data Structure (ET9KDBInfo).
 * @param[out]    psKDBVerBuf   Pointer to a buffer to which the Keyboard Input Module will write the version information. The buffer should be at least ET9MAXVERSIONSTR characters in length.
 * @param[out]    pwBufSize     Pointer to a value indicating the length in characters of the version information written to psKDBVerBuf.
 */

static ET9STATUS ET9LOCALCALL __GetKdbVersion_Generic(ET9KDBInfo  * const pKDBInfo,
                                                      ET9SYMB     * const psKDBVerBuf,
                                                      ET9U16      * const pwBufSize)
{
    ET9STATUS               eStatus;
    static const ET9U8      byTemplateStr[] = "XT9 x KDB Taa.bb Lcc.dd.ee Vff.gg Taa.bb Lcc.dd.ee Vff.gg";
    ET9U8           const * pbyVer;
    ET9SYMB                 *psTmp;
    ET9SYMB                 *psVerBuf = psKDBVerBuf;
    ET9U16                  wOldKdbNum;
    ET9U16                  wOldPageNum;

    WLOG5(fprintf(pLogFile5, "__GetKdbVersion_Generic, pKDBInfo = %p\n", pKDBInfo);)

    wOldKdbNum = pKDBInfo->wKdbNum;
    wOldPageNum = pKDBInfo->Private.wPageNum;

    /* size does NOT INCLUDE NULL terminator */

    *pwBufSize = 33;

    /* Copy template string. */

    pbyVer = byTemplateStr;
    psTmp  = psKDBVerBuf;

    while (*pbyVer) {
        *psTmp++ = (ET9SYMB)*pbyVer++;
    }

    eStatus = __KDBLoadPage(pKDBInfo, pKDBInfo->wFirstKdbNum, 0, NULL);

    if (eStatus) {
        return eStatus;
    }

    /* dynamic/static */
    psVerBuf[4] = pKDBInfo->Private.bUsingDynamicKDB ? 'D' : 'S';

    {
        ET9KdbLayoutInfo * const pLayoutInfo = pKDBInfo->Private.pCurrLayoutInfo;

        /* KDB version */
        psVerBuf += 11;                              /* Skip "DB T"   */

        /* Database type */
        _ET9BinaryToHex(pLayoutInfo->bDatabaseType, psVerBuf);
        psVerBuf += 3;                              /* Skip "aa."    */

        /* Layout ver */
        _ET9BinaryToHex(pLayoutInfo->bLayoutVer, psVerBuf);
        psVerBuf += 4;                              /* Skip "bb L"   */

        /* Primary language ID */
        _ET9BinaryToHex(pLayoutInfo->bPrimaryID, psVerBuf);
        psVerBuf += 3;                              /* Skip "cc."    */

        /* Secondary language ID */
        _ET9BinaryToHex(pLayoutInfo->bSecondaryID, psVerBuf);
        psVerBuf += 3;                              /* Skip "dd."    */

        /* add symbol class as 'fixed' unicode value */
        _ET9BinaryToHex(pLayoutInfo->bSymbolClass, psVerBuf);
        psVerBuf += 4;                              /* Skip "ee V"    */

        /* Contents majorversion. */
        _ET9BinaryToHex(pLayoutInfo->bContentsMajor, psVerBuf);
        psVerBuf += 3;                              /* Skip "ff."    */

        /* Contents minor version. */
        _ET9BinaryToHex(pLayoutInfo->bContentsMinor, psVerBuf);
    }

    if (!_ET9KDBSecondKDBSupported(pKDBInfo)) {
        __KDBLoadPage(pKDBInfo, wOldKdbNum, wOldPageNum, NULL);
        return ET9STATUS_NONE;
    }

    eStatus = __KDBLoadPage(pKDBInfo, pKDBInfo->wSecondKdbNum, 0, NULL);

    if (eStatus) {
        return eStatus;
    }

    {
        ET9KdbLayoutInfo * const pLayoutInfo = pKDBInfo->Private.pCurrLayoutInfo;

        *pwBufSize = 57;

        /* KDB version */
        psVerBuf += 4;                              /* Skip "DB T"   */

        /* Database type */
        _ET9BinaryToHex(pLayoutInfo->bDatabaseType, psVerBuf);
        psVerBuf += 3;                              /* Skip "aa."    */

        /* Layout ver */
        _ET9BinaryToHex(pLayoutInfo->bLayoutVer, psVerBuf);
        psVerBuf += 4;                              /* Skip "bb L"   */

        /* Primary language ID */
        _ET9BinaryToHex(pLayoutInfo->bPrimaryID, psVerBuf);
        psVerBuf += 3;                              /* Skip "cc."    */

        /* Secondary language ID */
        _ET9BinaryToHex(pLayoutInfo->bSecondaryID, psVerBuf);
        psVerBuf += 3;                              /* Skip "dd."    */

        /* add symbol class as 'fixed' unicode value */
        _ET9BinaryToHex(pLayoutInfo->bSymbolClass, psVerBuf);
        psVerBuf += 4;                              /* Skip "ee V"    */

        /* Contents majorversion. */
        _ET9BinaryToHex(pLayoutInfo->bContentsMajor, psVerBuf);
        psVerBuf += 3;                              /* Skip "ff."    */

        /* Contents minor version. */
        _ET9BinaryToHex(pLayoutInfo->bContentsMinor, psVerBuf);
    }

    __KDBLoadPage(pKDBInfo, wOldKdbNum, wOldPageNum, NULL);

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Finds keys for a regional match.
 *
 * @param pKDBInfo                  Pointer to the Keyboard Input Module Information Data Structure (ET9KDBInfo).
 * @param pDirectedPos              Pointer to the position information.
 * @param pMatchKeys                Pointer to an array to store the matching keys.
 * @param nMaxKeys                  Size of array for match keys.
 * @param pnKeyCount                Where the number of keys found will be stored.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

static ET9STATUS ET9LOCALCALL __FindRegionalMatchKeys_Generic(ET9KDBInfo              * const pKDBInfo,
                                                              ET9DirectedPos          * const pDirectedPos,
                                                              ET9MatchKey             * const pMatchKeys,
                                                              const ET9UINT                   nMaxKeys,
                                                              ET9UINT                 * const pnKeyCount)
{
    ET9KdbLayoutInfo * const pLayoutInfo = pKDBInfo->Private.pCurrLayoutInfo;

    ET9UINT         nCount;
    ET9UINT         nKeyCount;
    ET9KdbAreaInfo  *pArea;

    ET9Region       sTargetRegion;

    WLOG5(fprintf(pLogFile5, "__FindRegionalMatchKeys_Generic\n");)

    {
        ET9UINT nTargetWidth = pLayoutInfo->nBoxWidth;
        ET9UINT nTargetHeight = pLayoutInfo->nBoxHeight;

        if (pDirectedPos->sL1.nX || pDirectedPos->sL1.nY || pDirectedPos->sL2.nX || pDirectedPos->sL2.nY) {

            ET9TracePoint_f sL1P1;
            ET9TracePoint_f sL1P2;
            ET9TracePoint_f sL2P1;
            ET9TracePoint_f sL2P2;

            sL1P1.fX = 0;
            sL1P1.fY = 0;
            sL1P2.fX = 1;
            sL1P2.fY = 0;

            sL2P1.fX = (ET9FLOAT)pDirectedPos->sL1.nX;
            sL2P1.fY = (ET9FLOAT)pDirectedPos->sL1.nY;
            sL2P2.fX = (ET9FLOAT)pDirectedPos->sL2.nX;
            sL2P2.fY = (ET9FLOAT)pDirectedPos->sL2.nY;

            {
                const ET9FLOAT fAngle = __LinesAngle(&sL1P1, &sL1P2, &sL2P1, &sL2P2);

                const ET9FLOAT fAngleAbs = __ET9Abs(fAngle);

                if (fAngleAbs <= 30.0f || fAngleAbs >= 150.0f) {
                    nTargetHeight = (ET9UINT)(nTargetHeight * 0.6);
                }
                else if (fAngleAbs >= 60.0f && fAngleAbs <= 120.0f) {
                    nTargetWidth = (ET9UINT)(nTargetWidth * 0.6);
                }
            }
        }

        __CreateRegion(&sTargetRegion,
                       pDirectedPos->sPos.nX,
                       pDirectedPos->sPos.nY,
                       nTargetWidth,
                       nTargetHeight);
    }

    /* find overlapping keys */

    nKeyCount = 0;
    pArea = pLayoutInfo->pKeyAreas;
    for (nCount = pLayoutInfo->nKeyAreaCount; nCount; --nCount, ++pArea) {

        if (pArea->eKeyType == ET9KTFUNCTION) {
            continue;
        }

        {
            const ET9UINT nOverlap = __GetRegionOverlap(&sTargetRegion, &pArea->sRegion);

            const ET9BOOL bCenter = (pDirectedPos->sPos.nX >= pArea->sRegion.wLeft &&
                                     pDirectedPos->sPos.nX <= pArea->sRegion.wRight &&
                                     pDirectedPos->sPos.nY >= pArea->sRegion.wTop &&
                                     pDirectedPos->sPos.nY <= pArea->sRegion.wBottom) ? 1 : 0;

            if (nOverlap > pLayoutInfo->nMinKeyOverlap) {

                ET9UINT nInsertIndex;

                WLOG5(fprintf(pLogFile5, "  adding key (%04x), nOverlap %5u, bCenter %u\n", pArea->psChars[0], nOverlap, bCenter);)

                for (nInsertIndex = 0; nInsertIndex < nKeyCount; ++nInsertIndex) {
                    if (bCenter && !pMatchKeys[nInsertIndex].bCenter ||
                        nOverlap > pMatchKeys[nInsertIndex].nOverlap && bCenter == pMatchKeys[nInsertIndex].bCenter) {
                        break;
                    }
                }

                if (nInsertIndex < nKeyCount) {

                    ET9UINT nMoveIndex;

                    for (nMoveIndex = (nKeyCount >= nMaxKeys ? (nMaxKeys - 1) : nKeyCount); nMoveIndex > nInsertIndex; --nMoveIndex) {
                        pMatchKeys[nMoveIndex] = pMatchKeys[nMoveIndex - 1];
                    }
                }

                if (nInsertIndex >= nMaxKeys) {
                    WLOG5(fprintf(pLogFile5, "  too many keys, skipping (%04x)\n", pArea->psChars[0]);)
                    continue;
                }

                pMatchKeys[nInsertIndex].pArea = pArea;
                pMatchKeys[nInsertIndex].bCenter = bCenter;
                pMatchKeys[nInsertIndex].nOverlap = nOverlap;

                if (nKeyCount < nMaxKeys) {
                    ++nKeyCount;
                }
            }
        }
    }

    /* truncate regions from discrete top's and filter mixed types */

    if (nKeyCount > 1) {

        ET9KdbAreaInfo const * const pTopArea = pMatchKeys[0].pArea;

        const ET9BOOL bForceDiscrete = (ET9BOOL)(!pDirectedPos->bForceGeneric &&
                                                 (ET9_KDB_DISCRETE_MODE(pKDBInfo->dwStateBits) ||
                                                  ET9_KDB_MULTITAP_MODE(pKDBInfo->dwStateBits)));

        ET9Assert(pTopArea);

        if (pTopArea->eInputType == ET9DISCRETEKEY || pTopArea->eKeyType == ET9KTSMARTPUNCT || bForceDiscrete) {
            nKeyCount = 1;
        }

        {
            ET9UINT nIndex;

            for (nIndex = 1; nIndex < nKeyCount; ++nIndex) {

                ET9KdbAreaInfo const * const pThisArea = pMatchKeys[nIndex].pArea;

                ET9Assert(pThisArea);

                if (pThisArea->eKeyType != pTopArea->eKeyType) {

                    pMatchKeys[nIndex] = pMatchKeys[nKeyCount - 1];
                    --nKeyCount;
                    --nIndex;
                }
            }
        }
    }

    /* assign actual freqs */

    {
        ET9UINT nIndex;
        ET9FLOAT fTotFreq;

        fTotFreq = 0;
        for (nIndex = 0; nIndex < nKeyCount; ++nIndex) {

            ET9FLOAT fFreq = (ET9FLOAT)pMatchKeys[nIndex].nOverlap;

            WLOG5(fprintf(pLogFile5, "  -- [%2u] fFreq %8f\n", nIndex, fFreq);)

            if (fFreq <= 0.0) {
                WLOG5(fprintf(pLogFile5, "  -- zero freq made non zero (small)");)
                fFreq = (ET9FLOAT)0.00001;
            }

            fTotFreq += fFreq;
            pMatchKeys[nIndex].fFreq = fFreq;
        }

        if (fTotFreq == 0) {
            WLOG5(fprintf(pLogFile5, "  -- zero tot freq made non zero");)
            fTotFreq = 1;
        }

        for (nIndex = 0; nIndex < nKeyCount; ++nIndex) {

            const ET9FLOAT fFreq = (ET9FLOAT)(pMatchKeys[nIndex].fFreq / fTotFreq * 255.0 + 0.5);

            ET9U8 bFreq;

            if (fFreq <= 1) {
                bFreq = 1;
            }
            else if (fFreq >= 255) {
                bFreq = 255;
            }
            else {
                bFreq = (ET9U8)fFreq;
            }

            pMatchKeys[nIndex].bFreq = bFreq;

            WLOG5(fprintf(pLogFile5, "  [%2u] bFreq %3u, fFreq %8f, nOverlap %6u\n", nIndex, pMatchKeys[nIndex].bFreq, pMatchKeys[nIndex].fFreq, pMatchKeys[nIndex].nOverlap);)
        }
    }

    /* assure order */

    {
        ET9BOOL bDirty;
        ET9UINT nIndex;

        for (bDirty = 1; bDirty; ) {

            bDirty = 0;

            for (nIndex = 0; nIndex + 1 < nKeyCount; ++nIndex) {

                if (!pMatchKeys[nIndex].bCenter && pMatchKeys[nIndex + 1].bCenter ||
                    pMatchKeys[nIndex].bCenter == pMatchKeys[nIndex + 1].bCenter && pMatchKeys[nIndex].bFreq < pMatchKeys[nIndex + 1].bFreq) {

                    const ET9MatchKey sTmp = pMatchKeys[nIndex];

                    pMatchKeys[nIndex] = pMatchKeys[nIndex + 1];
                    pMatchKeys[nIndex + 1] = sTmp;

                    bDirty = 1;
                }
            }
        }
    }

#ifdef ET9_DEBUG

    /* validate */

    {
        ET9UINT nIndex;

        for (nIndex = 0; nIndex + 1 < nKeyCount; ++nIndex) {
            ET9Assert(pMatchKeys[nIndex].bFreq >= pMatchKeys[nIndex + 1].bFreq || pMatchKeys[nIndex].bCenter && !pMatchKeys[nIndex + 1].bCenter);
        }
    }

#endif

    /* done */

    *pnKeyCount = nKeyCount;

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Finds keys for a regional match.
 *
 * @param pKDBInfo             Pointer to keyboard information structure.
 * @param pKeyArea                  .
 * @param pbType                    .
 * @param pnCharCount               .
 * @param ppsChars                  .
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

static ET9STATUS ET9LOCALCALL __LocateKdbKey_Generic(ET9KDBInfo              * const pKDBInfo,
                                                     ET9KdbAreaInfo    const * const pKeyArea,
                                                     ET9U8                   * const pbType,
                                                     ET9UINT                 * const pnCharCount,
                                                     ET9SYMB          const ** const ppsChars)
{
    if (!pKeyArea) {
        return ET9STATUS_ERROR;
    }

    *pbType = (ET9U8)pKeyArea->eKeyType;

    if (pKDBInfo->Private.eShiftState && pKeyArea->nShiftedCharCount) {
        *pnCharCount = pKeyArea->nShiftedCharCount;
        *ppsChars = pKeyArea->psShiftedChars;
    }
    else {
        *pnCharCount = pKeyArea->nCharCount;
        *ppsChars = pKeyArea->psChars;
    }

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pKDBInfo             Pointer to keyboard information structure.
 * @param pKeyArea             Key area.
 * @param bKeyType             Key type.
 * @param pSymbInfo            Symbinfo to load chars into.
 * @param pbSymbsUsed          Pointer to receive the number of symbs used.
 * @param wLdbId               Presumed language at time of tap.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

static ET9STATUS ET9LOCALCALL __LoadKeyCharsType_Generic(ET9KDBInfo              * const pKDBInfo,
                                                         ET9KdbAreaInfo    const * const pKeyArea,
                                                         const ET9U8                     bKeyType,
                                                         ET9SymbInfo             * const pSymbInfo,
                                                         ET9U8                   * const pbSymbsUsed,
                                                         const ET9U16                    wLdbId)
{
    ET9STATUS eStatus;
    ET9UINT   nIndex;
    ET9UINT   nNumCharsToLoad;
    ET9U8     bThisKeyType;
    ET9SYMB   *psSym;
    ET9SYMB   *psUpperSym;
    ET9SYMB   const *psChar;
    ET9SYMB   const *psChars;
    ET9DataPerBaseSym *pDataPerBaseSym;

    WLOG5(fprintf(pLogFile5, "__LoadKeyCharsType_Generic, pKDBInfo = %p\n", pKDBInfo);)

    ET9Assert(pKDBInfo);
    ET9Assert(pbSymbsUsed);

    eStatus = __LocateKdbKey_Generic(pKDBInfo, pKeyArea, &bThisKeyType, &nNumCharsToLoad, &psChars);

    if (eStatus) {
        ET9Assert(0);
        return eStatus;
    }

    pDataPerBaseSym = &pSymbInfo->DataPerBaseSym[pSymbInfo->bNumBaseSyms];

    *pbSymbsUsed = 1;
    pDataPerBaseSym->bNumSymsToMatch = 0;

    if (bThisKeyType != bKeyType) {
        WLOG5(fprintf(pLogFile5, "  wrong symb type, skipping\n");)
        *pbSymbsUsed = 0;
        ET9Assert(0);
        return ET9STATUS_NONE;
    }

    /* load chars */

    psChar = psChars;
    psSym = pDataPerBaseSym->sChar + pDataPerBaseSym->bNumSymsToMatch;
    psUpperSym = pDataPerBaseSym->sUpperCaseChar + pDataPerBaseSym->bNumSymsToMatch;

    for (nIndex = 0; nIndex < nNumCharsToLoad; ++nIndex, ++psChar) {

        if (pDataPerBaseSym->bNumSymsToMatch >= ET9MAXALTSYMBS) {

            if (pSymbInfo->eInputType == ET9DISCRETEKEY || pSymbInfo->bSymbType == ET9KTSMARTPUNCT) {

                if (pSymbInfo->bNumBaseSyms >= ET9MAXBASESYMBS) {
                    return ET9STATUS_OUT_OF_RANGE_MAXBASESYMBS;
                }

                ++pDataPerBaseSym;
                ++*pbSymbsUsed;

                pDataPerBaseSym->bSymFreq = 0;
                pDataPerBaseSym->bNumSymsToMatch = 0;
                pDataPerBaseSym->bDefaultCharIndex = 0;

                psSym = pDataPerBaseSym->sChar;
                psUpperSym = pDataPerBaseSym->sUpperCaseChar;
            }
            else {
                return ET9STATUS_OUT_OF_RANGE_MAXALTSYMBS;
            }
        }

        *psSym = *psChar;

        WLOG5B(fprintf(pLogFile5, "  Loading = %04x\n", *psSym);)

        if (pKDBInfo->Private.pFilterSymb && pKDBInfo->Private.pFilterSymb(pKDBInfo->Private.pFilterSymbInfo, *psSym)) {
            WLOG5B(fprintf(pLogFile5, "    Suppressed\n");)
            continue;
        }

        WLOG5B(fprintf(pLogFile5, "    Loaded\n");)

        ++pDataPerBaseSym->bNumSymsToMatch;

        *psUpperSym++ = _ET9SymToOther(*psSym++, wLdbId);

        WLOG5B(fprintf(pLogFile5, "    Upper = %04x\n", *(psUpperSym - 1));)
    }

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pKDBInfo                  Pointer to the Keyboard Input Module Information Data Structure (ET9KDBInfo).
 * @param pSymbInfo                 .
 * @param pDirectedPos              .
 * @param eLoadAction               .
 * @param wLdbId                    .
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

static ET9STATUS ET9LOCALCALL __ProcessAmbigTap_Regional_Generic(ET9KDBInfo              * const pKDBInfo,
                                                                 ET9SymbInfo             * const pSymbInfo,
                                                                 ET9DirectedPos          * const pDirectedPos,
                                                                 const ET9LOADSYMBACTION         eLoadAction,
                                                                 const ET9U16                    wLdbId)
{
    ET9STATUS       eStatus;
    ET9UINT         nIndex;
    ET9UINT         nKeyCount;

    ET9MatchKey     pMatchKeys[ET9_KDB_MAX_REGIONS];

    WLOG5(fprintf(pLogFile5, "__ProcessAmbigTap_Regional_Generic, pKDBInfo = %p, eLoadAction = %d\n", pKDBInfo, (int)eLoadAction);)

    ET9Assert(pKDBInfo);
    ET9Assert(pSymbInfo);

    eStatus = __FindRegionalMatchKeys_Generic(pKDBInfo, pDirectedPos, pMatchKeys, ET9_KDB_MAX_REGIONS, &nKeyCount);

    if (eStatus) {
        return eStatus;
    }

    if (!nKeyCount) {
        return ET9STATUS_NO_KEY;
    }

    {
        ET9KdbAreaInfo const * pArea = pMatchKeys[0].pArea;

        ET9Assert(pArea);

        pSymbInfo->bSymbType = (ET9U8)pArea->eKeyType;
        pSymbInfo->eInputType = pArea->eInputType;

        if (pSymbInfo->eInputType == ET9REGIONALKEY && nKeyCount == 1 && (ET9_KDB_DISCRETE_MODE(pKDBInfo->dwStateBits) || ET9_KDB_MULTITAP_MODE(pKDBInfo->dwStateBits))) {
            pSymbInfo->eInputType = ET9DISCRETEKEY;
        }
    }

    ET9Assert(nKeyCount <= 1 || pSymbInfo->eInputType != ET9DISCRETEKEY && pSymbInfo->bSymbType != ET9KTSMARTPUNCT);

#ifdef ET9_DEBUG
    {
        ET9UINT nFreqTot = 0;

        for (nIndex = 0; nIndex < nKeyCount; ++nIndex) {
            nFreqTot += pMatchKeys[nIndex].bFreq;
        }

        ET9Assert(nFreqTot > 200);
    }
#endif

    if (eLoadAction != LOADSYMBACTION_APPEND){
        pSymbInfo->bNumBaseSyms = 0;
        pSymbInfo->pKdbKey = pMatchKeys[0].pArea;
    }

    for (nIndex = 0; nIndex < nKeyCount; ++nIndex) {

        ET9U8 bSymbsUsed;

        if (pSymbInfo->bNumBaseSyms >= ET9MAXBASESYMBS) {
            break;
        }

        eStatus = __LoadKeyCharsType_Generic(pKDBInfo,
                                             pMatchKeys[nIndex].pArea,
                                             pSymbInfo->bSymbType,
                                             pSymbInfo,
                                             &bSymbsUsed,
                                             wLdbId);

        if (eStatus) {
            return eStatus;
        }

        {
            ET9U8 bSymbUsedCount;
            ET9DataPerBaseSym *pDPBSym;

            pDPBSym = &pSymbInfo->DataPerBaseSym[pSymbInfo->bNumBaseSyms];
            for (bSymbUsedCount = bSymbsUsed; bSymbUsedCount; --bSymbUsedCount, ++pDPBSym) {

                pDPBSym->bSymFreq = pMatchKeys[nIndex].bFreq;

                ET9Assert(pDPBSym->bSymFreq);
                ET9Assert(pDPBSym->bNumSymsToMatch);
            }
        }

        pSymbInfo->bNumBaseSyms = (ET9U8)(pSymbInfo->bNumBaseSyms + bSymbsUsed);
    }

    ET9Assert(pSymbInfo->bNumBaseSyms);

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pKDBInfo                  Pointer to the Keyboard Input Module Information Data Structure (ET9KDBInfo).
 * @param pWordSymbInfo             .
 * @param pDirectedPos              .
 * @param bCurrIndexInList          .
 * @param psFunctionKey             Pointer to return function key info in.
 * @param eLoadAction               .
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

static ET9STATUS ET9LOCALCALL __ProcessTap_Regional_Generic(ET9KDBInfo             * const pKDBInfo,
                                                            ET9WordSymbInfo        * const pWordSymbInfo,
                                                            ET9DirectedPos         * const pDirectedPos,
                                                            const ET9U8                    bCurrIndexInList,
                                                            ET9SYMB                * const psFunctionKey,
                                                            const ET9LOADSYMBACTION        eLoadAction)
{
    ET9STATUS    eStatus;
    ET9U8        bNumSymbs;

    WLOG5(fprintf(pLogFile5, "__ProcessTap_Regional_Generic, pKDBInfo = %p, eLoadAction = %d\n", pKDBInfo, (int)eLoadAction);)

    ET9Assert(pKDBInfo);
    ET9Assert(pWordSymbInfo);

    {
        const ET9KdbAreaInfo *pArea = __GetKeyAreaFromTap_Generic(pKDBInfo, (ET9U16)pDirectedPos->sPos.nX, (ET9U16)pDirectedPos->sPos.nY);

        if (pArea && pArea->eKeyType == ET9KTFUNCTION) {
            *psFunctionKey = pArea->psChars[0];
            return ET9STATUS_NONE;
        }

        _ET9ImminentSymb(pWordSymbInfo, bCurrIndexInList, (ET9BOOL)(pArea && (pArea->eKeyType == ET9KTSMARTPUNCT || pArea->eKeyType == ET9KTPUNCTUATION)));
    }

    bNumSymbs = (ET9U8) (pWordSymbInfo->bNumSymbs + 1);

    __MirrorWsiState(pKDBInfo, pWordSymbInfo);

    if (eLoadAction != LOADSYMBACTION_NEW) {

        ET9Assert(pWordSymbInfo->bNumSymbs);

        if (!pWordSymbInfo->bNumSymbs) {
            return ET9STATUS_ERROR;
        }

        --pWordSymbInfo->bNumSymbs;
    }

    bNumSymbs = (ET9U8)(pWordSymbInfo->bNumSymbs + 1);

    if (bNumSymbs > ET9MAXWORDSIZE) {
        return ET9STATUS_FULL;
    }

    if (eLoadAction != LOADSYMBACTION_APPEND) {
        _ET9ClearMem((ET9U8*)&pWordSymbInfo->SymbsInfo[bNumSymbs - 1], sizeof(ET9SymbInfo));
    }

    eStatus = __ProcessAmbigTap_Regional_Generic(pKDBInfo, &pWordSymbInfo->SymbsInfo[bNumSymbs - 1], pDirectedPos, eLoadAction, pWordSymbInfo->Private.wLocale);

    if (eStatus) {
        return eStatus;
    }

    __ProcessWordSymInfo(pKDBInfo, pWordSymbInfo);

    pWordSymbInfo->SymbsInfo[bNumSymbs - 1].wKeyIndex = ET9UNDEFINEDKEYVALUE;
    pWordSymbInfo->SymbsInfo[bNumSymbs - 1].wTapX = (ET9U16)pDirectedPos->sPos.nX;
    pWordSymbInfo->SymbsInfo[bNumSymbs - 1].wTapY = (ET9U16)pDirectedPos->sPos.nY;

    return eStatus;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pKDBInfo                  Pointer to the Keyboard Input Module Information Data Structure (ET9KDBInfo).
 * @param pDirectedPos              .
 * @param pWordSymbInfo             .
 * @param psFunctionKey             Pointer to return function key info in.
 * @param bCurrIndexInList          .
 * @param eLoadAction               .
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

static ET9STATUS ET9LOCALCALL __ProcessAmbigTap_CurrentKdb_Generic(ET9KDBInfo           * const pKDBInfo,
                                                                   ET9DirectedPos       * const pDirectedPos,
                                                                   ET9WordSymbInfo      * const pWordSymbInfo,
                                                                   ET9SYMB              * const psFunctionKey,
                                                                   const ET9U8                  bCurrIndexInList,
                                                                   const ET9LOADSYMBACTION      eLoadAction)
{
    ET9STATUS eStatus;

    WLOG5(fprintf(pLogFile5, "__ProcessAmbigTap_CurrentKdb_Generic, pKDBInfo = %p, eLoadAction = %d, wX = %d, wY = %d\n", pKDBInfo, (int)eLoadAction, pDirectedPos->sPos.nX, pDirectedPos->sPos.nY);)

    if (pKDBInfo->Private.sKdbAction.bShiftState) {
        pWordSymbInfo->dwStateBits |= ET9STATE_SHIFT_MASK;
    }

    eStatus = __ProcessTap_Regional_Generic(pKDBInfo, pWordSymbInfo, pDirectedPos, bCurrIndexInList, psFunctionKey, eLoadAction);

    return eStatus;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Find key tap pos (pos to tap to become key input equivalent).
 *
 * @param pKDBInfo                  Pointer to the Keyboard Input Module Information Data Structure (ET9KDBInfo).
 * @param wKeyIndex                 Key index.
 * @param pDirectedPos              Pointer to the position information.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

static ET9STATUS ET9LOCALCALL __FindKeyTapPos_Generic(ET9KDBInfo              * const pKDBInfo,
                                                      const ET9U16                    wKeyIndex,
                                                      ET9DirectedPos          * const pDirectedPos)
{
    ET9KdbLayoutInfo * const pLayoutInfo = pKDBInfo->Private.pCurrLayoutInfo;

    ET9UINT             nCount;
    ET9UINT             nKeyCount;
    ET9KdbAreaInfo      *pArea;

    WLOG5(fprintf(pLogFile5, "__FindKeyTapPos_Generic\n");)

    __InitDirectedPos(pDirectedPos);

    nKeyCount = 0;
    pArea = pLayoutInfo->pKeyAreas;
    for (nCount = pLayoutInfo->nKeyAreaCount; nCount; --nCount, ++pArea) {

        /* include function keys */

        if (wKeyIndex == pArea->wKeyIndex) {

            pDirectedPos->sPos.nX = pArea->nCenterX;
            pDirectedPos->sPos.nY = pArea->nCenterY;

            return ET9STATUS_NONE;
        }
    }

    return ET9STATUS_OUT_OF_RANGE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pKDBInfo                  Pointer to the Keyboard Input Module Information Data Structure (ET9KDBInfo).
 * @param pWordSymbInfo             .
 * @param wKeyIndex                 Key index.
 * @param bCurrIndexInList          .
 * @param psFunctionKey             Pointer to return function key info in.
 * @param eLoadAction               .
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

static ET9STATUS ET9LOCALCALL __ProcessKey_Regional_Generic(ET9KDBInfo             * const pKDBInfo,
                                                            ET9WordSymbInfo        * const pWordSymbInfo,
                                                            const ET9U16                   wKeyIndex,
                                                            const ET9U8                    bCurrIndexInList,
                                                            ET9SYMB                * const psFunctionKey,
                                                            const ET9LOADSYMBACTION        eLoadAction)
{
    ET9STATUS       eStatus;
    ET9U8           bNumSymbs;
    ET9DirectedPos  sDirectedPos;

    WLOG5(fprintf(pLogFile5, "__ProcessKey_Regional_Generic, pKDBInfo = %p, eLoadAction = %d\n", pKDBInfo, (int)eLoadAction);)

    ET9Assert(pKDBInfo);
    ET9Assert(pWordSymbInfo);

    {
        const ET9KdbAreaInfo *pArea = __GetKeyAreaFromKey_Generic(pKDBInfo, wKeyIndex);

        if (pArea && pArea->eKeyType == ET9KTFUNCTION) {
            *psFunctionKey = pArea->psChars[0];
            return ET9STATUS_NONE;
        }

        _ET9ImminentSymb(pWordSymbInfo, bCurrIndexInList, (ET9BOOL)(pArea && (pArea->eKeyType == ET9KTSMARTPUNCT || pArea->eKeyType == ET9KTPUNCTUATION)));
    }

    eStatus = __FindKeyTapPos_Generic(pKDBInfo, wKeyIndex, &sDirectedPos);

    if (eStatus) {
        return eStatus;
    }

    bNumSymbs = (ET9U8) (pWordSymbInfo->bNumSymbs + 1);

    __MirrorWsiState(pKDBInfo, pWordSymbInfo);

    if (eLoadAction != LOADSYMBACTION_NEW) {

        ET9Assert(pWordSymbInfo->bNumSymbs);

        if (!pWordSymbInfo->bNumSymbs) {
            return ET9STATUS_ERROR;
        }

        --pWordSymbInfo->bNumSymbs;
    }

    bNumSymbs = (ET9U8)(pWordSymbInfo->bNumSymbs + 1);

    if (bNumSymbs > ET9MAXWORDSIZE) {
        return ET9STATUS_FULL;
    }

    if (eLoadAction != LOADSYMBACTION_APPEND) {
        _ET9ClearMem((ET9U8*)&pWordSymbInfo->SymbsInfo[bNumSymbs - 1], sizeof(ET9SymbInfo));
    }

    eStatus = __ProcessAmbigTap_Regional_Generic(pKDBInfo, &pWordSymbInfo->SymbsInfo[bNumSymbs - 1], &sDirectedPos, eLoadAction, pWordSymbInfo->Private.wLocale);

    if (eStatus) {
        return eStatus;
    }

    __ProcessWordSymInfo(pKDBInfo, pWordSymbInfo);

    pWordSymbInfo->SymbsInfo[bNumSymbs - 1].wKeyIndex = wKeyIndex;
    pWordSymbInfo->SymbsInfo[bNumSymbs - 1].wTapX = ET9UNDEFINEDTAPVALUE;
    pWordSymbInfo->SymbsInfo[bNumSymbs - 1].wTapY = ET9UNDEFINEDTAPVALUE;

    return eStatus;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * This function processes an ambig key for the current KDB.
 *
 * @param pKDBInfo             Pointer to keyboard information structure.
 * @param wKeyIndex            Key index.
 * @param pWordSymbInfo        Pointer to tap info.
 * @param psFunctionKey        Pointer to return function key info in.
 * @param bCurrIndexInList     Currently highlighted sel list entry.
 * @param eLoadAction          If new, appending etc.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

static ET9STATUS ET9LOCALCALL __ProcessAmbigKey_CurrentKdb_Generic(ET9KDBInfo           * const pKDBInfo,
                                                                   const ET9U16                 wKeyIndex,
                                                                   ET9WordSymbInfo      * const pWordSymbInfo,
                                                                   ET9SYMB              * const psFunctionKey,
                                                                   const ET9U8                  bCurrIndexInList,
                                                                   const ET9LOADSYMBACTION      eLoadAction)
{
    ET9STATUS eStatus;

    WLOG5(fprintf(pLogFile5, "__ProcessAmbigKey_CurrentKdb_Generic, pKDBInfo = %p, eLoadAction = %d, wKeyIndex = %d\n", pKDBInfo, (int)eLoadAction, wKeyIndex);)

    if (pKDBInfo->Private.sKdbAction.bShiftState) {
        pWordSymbInfo->dwStateBits |= ET9STATE_SHIFT_MASK;
    }

    eStatus = __ProcessKey_Regional_Generic(pKDBInfo, pWordSymbInfo, wKeyIndex, bCurrIndexInList, psFunctionKey, eLoadAction);

    return eStatus;
}

/* ************************************************************************************************************** */
/* * LOCAL ****************************************************************************************************** */
/* ************************************************************************************************************** */

/*---------------------------------------------------------------------------*/
/** \internal
 * This function computes checksum for the KDB content.
 *
 * @param pKDBInfo             Pointer to keyboard information structure.
 *
 * @return Checksum
 */

static ET9U32 ET9LOCALCALL __ComputeContentChecksum(ET9KDBInfo   * const pKDBInfo)
{
    ET9U32 dwChecksum;

    if (pKDBInfo->Private.bUsingDynamicKDB) {
        dwChecksum = __ComputeContentChecksum_Generic(pKDBInfo);
    }
    else {
        dwChecksum = __ComputeContentChecksum_Static(pKDBInfo);
    }

    return dwChecksum;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * This function loads the specified page (in wPageNum).
 * It will also assure the correct kdb.
 *
 * @param pKDBInfo     Pointer to keyboard information structure.
 * @param wKdbNum      Keyboard number.
 * @param wPageNum     Page number to load.
 * @param pwTotalKeys  Pointer to total keys.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

static ET9STATUS ET9LOCALCALL __KDBLoadPage(ET9KDBInfo      * const pKDBInfo,
                                            const ET9U16            wKdbNum,
                                            const ET9U16            wPageNum,
                                            ET9U16          * const pwTotalKeys)
{
    ET9STATUS eStatus;

    ET9Assert(pKDBInfo);

    WLOG5(fprintf(pLogFile5, "__KDBLoadPage, loading, pKDBInfo = %p, wKdbNum = %04x (%04X), wPageNum = %04x (%04X)\n", pKDBInfo, wKdbNum, pKDBInfo->wKdbNum, wPageNum, pKDBInfo->Private.wPageNum);)

    /* clear init status marker */

    pKDBInfo->Private.wKDBInitOK = 0;

    /* load */

    eStatus = __KDBLoadPage_Dynamic(pKDBInfo, wKdbNum, wPageNum, pwTotalKeys);

    if (eStatus && eStatus == ET9STATUS_READ_DB_FAIL) {
        eStatus = __KDBLoadPage_Static(pKDBInfo, wKdbNum, wPageNum, pwTotalKeys);
    }

    if (eStatus) {
        return eStatus;
    }

    /* KDB ok now */

    pKDBInfo->Private.wKDBInitOK = ET9GOODSETUP;

    /* inform about kdb loaded */

    if (pKDBInfo->ET9Handle_KDB_Request) {

        ET9KDB_Request sRequest;

        sRequest.eType = ET9_KDB_REQ_PAGE_LOADED;

        eStatus = pKDBInfo->ET9Handle_KDB_Request(pKDBInfo, NULL, &sRequest);
    }

    /* done */

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * This function sets shift info for symbol based on input state.
 *
 * @param pWordSymbInfo   Pointer to current word symbols info struct.
 * @param pSymbInfo       Pointer to current sym info struct.
 *
 * @return None
 */

static void ET9LOCALCALL __SetSymShiftState(ET9WordSymbInfo   * const pWordSymbInfo,
                                            ET9SymbInfo       * const pSymbInfo)
{
    ET9Assert(pWordSymbInfo);
    ET9Assert(pSymbInfo);

    if (ET9SHIFT_MODE(pWordSymbInfo->dwStateBits)) {
        pSymbInfo->eShiftState = ET9SHIFT;
    }
    else if (ET9CAPS_MODE(pWordSymbInfo->dwStateBits)) {
        pSymbInfo->eShiftState = ET9CAPSLOCK;
    }
    else {
        pSymbInfo->eShiftState = ET9NOSHIFT;
    }

    pWordSymbInfo->Private.eLastShiftState = pSymbInfo->eShiftState;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * This function updates the syminfo for the last entered position.
 *
 * @param pKDBInfo             Pointer to keyboard information structure.
 * @param pWordSymbInfo        Pointer to tap info.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

static void ET9LOCALCALL __ProcessWordSymInfo(ET9KDBInfo        * const pKDBInfo,
                                              ET9WordSymbInfo   * const pWordSymbInfo)
{
    const ET9U8         bNumSymbs = (ET9U8)(pWordSymbInfo->bNumSymbs + 1);
    ET9SymbInfo * const pSymbInfo = &pWordSymbInfo->SymbsInfo[bNumSymbs - 1];
    ET9BOOL             bMultiSymbInput = pSymbInfo->bSymbType == ET9KTSTRING ? 1 : 0;

    ET9Assert(pKDBInfo);
    ET9Assert(pWordSymbInfo);

    _ET9InvalidateOneSymb(pWordSymbInfo, (ET9U8)(bNumSymbs - 1));

    /* first check to see of the input was multiple characters, such as an emoticon, in which case we are
       going to make modifications to the pWordSymbInfo */

    if (bMultiSymbInput) {

        ET9UINT                     nCount;
        ET9U16                      wInputIndex;
        ET9DataPerBaseSym   * const pDPBSym = pSymbInfo->DataPerBaseSym;
        const ET9UINT               nNumStringChars = pDPBSym->bNumSymsToMatch;

        if (!pWordSymbInfo->bNumSymbs) {
            wInputIndex = 1;
        }
        else {
            wInputIndex = pWordSymbInfo->SymbsInfo[pWordSymbInfo->bNumSymbs-1].wInputIndex + 1;
        }

        for (nCount = nNumStringChars; nCount; --nCount) {

            if (bNumSymbs - 1 + nCount <= ET9MAXWORDSIZE) {

                ET9SymbInfo         * const pTempSymbInfo = &pWordSymbInfo->SymbsInfo[bNumSymbs + nCount - 2];
                ET9DataPerBaseSym   * const pDPBSymTemp = pTempSymbInfo->DataPerBaseSym;

                _ET9InvalidateOneSymb(pWordSymbInfo, (ET9U8)(bNumSymbs + nCount - 2));

                pTempSymbInfo->bNumBaseSyms = 1;
                pTempSymbInfo->eInputType = ET9MULTISYMBEXPLICIT;
                pTempSymbInfo->wInputIndex = wInputIndex;
                pTempSymbInfo->wKeyIndex = ET9UNDEFINEDKEYVALUE;
                pTempSymbInfo->wTapX = ET9UNDEFINEDTAPVALUE;
                pTempSymbInfo->wTapY = ET9UNDEFINEDTAPVALUE;
                pTempSymbInfo->eShiftState = ET9NOSHIFT;
                pTempSymbInfo->bForcedLowercase = 0;
                pTempSymbInfo->bAmbigType = (ET9U8)ET9EXACT;
                pTempSymbInfo->bTraceProbability = 0;
                pTempSymbInfo->bTraceIndex = 0;
                pTempSymbInfo->bFreqsInvalidated = 1;
                pTempSymbInfo->wKdb1 = 0;
                pTempSymbInfo->wPage1 = 0;
                pTempSymbInfo->wKdb2 = 0;
                pTempSymbInfo->wPage2 = 0;

                pDPBSymTemp->bNumSymsToMatch = 1;
                pDPBSymTemp->bSymFreq = 2;
                pDPBSymTemp->sChar[0] = pDPBSym->sChar[nCount - 1];
                pDPBSymTemp->sUpperCaseChar[0] = pDPBSym->sChar[nCount - 1];

                pWordSymbInfo->Private.eLastShiftState = pTempSymbInfo->eShiftState;

                ++pWordSymbInfo->bNumSymbs;
            }
        }

        /* zero out rest of data from original pSymbInfo */

        {
            ET9UINT nIndex;

            for (nIndex = 1; nIndex < nNumStringChars; ++nIndex) {
                pDPBSym->sChar[nIndex] = 0;
                pDPBSym->sUpperCaseChar[nIndex] = 0;
            }
        }

        return;
    }

#ifdef ET9_DEBUG

    {
        /* verify that all symb info chars are valid (non zero) */

        ET9U16             bKeyIndx;
        ET9SymbInfo        *pSymbInfo;
        ET9DataPerBaseSym  *pSymData;
        ET9SYMB            *pLower;
        ET9SYMB            *pUpper;

        pSymbInfo = &pWordSymbInfo->SymbsInfo[pWordSymbInfo->bNumSymbs];
        pSymData  = pSymbInfo->DataPerBaseSym;

        for (bKeyIndx = pSymbInfo->bNumBaseSyms; bKeyIndx; --bKeyIndx, ++pSymData) {

            ET9UINT nCount;

            ET9Assert(pSymData->bSymFreq || pKDBInfo->Private.pFilterSymb);
            ET9Assert(pSymData->bNumSymsToMatch || pKDBInfo->Private.pFilterSymb);

            pLower = pSymData->sChar;
            pUpper = pSymData->sUpperCaseChar;

            for (nCount = pSymData->bNumSymsToMatch; nCount; --nCount, ++pLower, ++pUpper) {
                ET9Assert(*pLower && *pUpper);
            }
        }
    }

#endif

    pSymbInfo->bTraceProbability = 0;
    pSymbInfo->bTraceIndex = 0;
    pSymbInfo->bFreqsInvalidated = 1;

    __SetSymShiftState(pWordSymbInfo, pSymbInfo);

    ++pWordSymbInfo->bNumSymbs;

    /* shut off shift bit */

    if (!ET9_KDB_MULTITAP_MODE(pKDBInfo->dwStateBits)) {

        pWordSymbInfo->dwStateBits &= ~ET9STATE_SHIFT_MASK;
    }
}

/*---------------------------------------------------------------------------*/
/** \internal
 * This function processes a general MT (from either tap or key).
 *
 * @param pKDBInfo             Pointer to keyboard information structure.
 * @param pWordSymbInfo        Pointer to word symbols info struct.
 * @param bIsFirstPress        Indicator if 1st tap on key being multitapped.
 * @param psFunctionKey        Pointer to return function key sym value in.
 * @param bCurrIndexInList     Currently highlighted sel list entry.
 * @param wMTLastInput         What ever was last input, e.G. Key or symb.
 * @param eLoadAction          If new, appending etc.
 *
 * @return None
 */

static void ET9LOCALCALL __ProcessMultitap(ET9KDBInfo            * const pKDBInfo,
                                           ET9WordSymbInfo       * const pWordSymbInfo,
                                           ET9BOOL                       bIsFirstPress,
                                           ET9SYMB               * const psFunctionKey,
                                           const ET9U8                   bCurrIndexInList,
                                           const ET9U16                  wMTLastInput,
                                           const ET9LOADSYMBACTION       eLoadAction)
{
    ET9SymbInfo * const pSymbInfo = &pWordSymbInfo->SymbsInfo[pWordSymbInfo->bNumSymbs-1];

    ET9SYMB             sFirstSymb;
    ET9SYMB             sPreviousActiveSymb = 0;    /* silly compiler warning - init */
    ET9DataPerBaseSym   *pDPBS;

    WLOG5(fprintf(pLogFile5, "__ProcessMultitap, pKDBInfo = %p, eLoadAction = %d, bIsFirstPress = %d\n", pKDBInfo, (int)eLoadAction, bIsFirstPress);)

    ET9_UNUSED(psFunctionKey);
    ET9_UNUSED(bCurrIndexInList);

    ET9Assert(pKDBInfo);
    ET9Assert(pWordSymbInfo);
    ET9Assert(psFunctionKey);

    /* before destroying info, save previous symb for reload */

    if (eLoadAction == LOADSYMBACTION_RELOAD) {

        if (pKDBInfo->Private.bMTSymbCount) {

            if (pKDBInfo->Private.bMTLastSymbIndex < pKDBInfo->Private.bMTSymbCount) {
                sPreviousActiveSymb = pKDBInfo->Private.sMTSymbs[pKDBInfo->Private.bMTLastSymbIndex];
            }
            else {
                sPreviousActiveSymb = 0;
            }

            ET9Assert(sPreviousActiveSymb && sPreviousActiveSymb != (ET9SYMB)0xcccc && sPreviousActiveSymb != 0xFFFF);
        }

        WLOG5(fprintf(pLogFile5, "  reload, bMTLastSymbIndex = %d, bMTSymbCount = %d, sPreviousActiveSymb = %04X\n", pKDBInfo->Private.bMTLastSymbIndex, pKDBInfo->Private.bMTSymbCount, sPreviousActiveSymb);)
    }

    /* when needed, retrieve all symbs - basis for continued work */

    if (bIsFirstPress || eLoadAction == LOADSYMBACTION_RELOAD) {

        ET9U8 bBaseSymbCount;

        WLOG5(fprintf(pLogFile5, "  loading MT src\n");)

        pKDBInfo->Private.pMTKdbKey = pSymbInfo->pKdbKey;
        pKDBInfo->Private.bMTSymbCountSrc = 0;
        bBaseSymbCount = pSymbInfo->bNumBaseSyms;
        pDPBS = pSymbInfo->DataPerBaseSym;

        while (pKDBInfo->Private.bMTSymbCountSrc < ET9_KDB_MAX_MT_SYMBS) {

            ET9U8 bSymbCount;
            ET9SYMB *psSymb;
            ET9SYMB *psSymbLook;

            if (!bBaseSymbCount) {
                break;
            }

            psSymb = pDPBS->sChar;
            for (bSymbCount = pDPBS->bNumSymsToMatch; bSymbCount;  --bSymbCount, ++psSymb) {

                ET9U8 bCount;

                if (pKDBInfo->Private.bMTSymbCountSrc >= ET9_KDB_MAX_MT_SYMBS) {
                    break;
                }

                psSymbLook = pKDBInfo->Private.sMTSymbsSrc;
                for (bCount = pKDBInfo->Private.bMTSymbCountSrc; bCount; --bCount, ++psSymbLook) {
                    if (*psSymb == *psSymbLook) {
                        break;
                    }
                }
                if (bCount) {
                    continue;
                }

                ET9Assert(*psSymb && *psSymb != (ET9SYMB)0xcccc);

                *psSymbLook = *psSymb;
                ++pKDBInfo->Private.bMTSymbCountSrc;
            }

            ++pDPBS;
            --bBaseSymbCount;
        }
    }

    /* setup sequence based on symbol convert result, shift state etc */
    /* both case shift and convert can (individually) change the character sequnce (and character count) */

    _ET9InvalidateOneSymb(pWordSymbInfo, (ET9U8)(pWordSymbInfo->bNumSymbs-1));

    __SetSymShiftState(pWordSymbInfo, pSymbInfo);

    if (bIsFirstPress ||
        eLoadAction == LOADSYMBACTION_RELOAD ||
        pKDBInfo->Private.eMTLastShiftState != pSymbInfo->eShiftState) {

        pKDBInfo->Private.eMTLastShiftState = pSymbInfo->eShiftState;

        /* either pull from the symb data from above (src) or use dedicated MT info */

        if (pKDBInfo->Private.pMTKdbKey && pKDBInfo->Private.pMTKdbKey->nMultitapCharCount) {

            const ET9BOOL bCombinedCase = pKDBInfo->Private.pMTKdbKey->nMultitapShiftedCharCount ? 0 : 1;

            ET9UINT nCount;
            ET9UINT nTotCount;
            ET9SYMB *psSymb;

            WLOG5(fprintf(pLogFile5, "  loading MT seq (from actual MT info), bCombinedCase %u, shift %u\n", bCombinedCase, pSymbInfo->eShiftState);)

            if (!pSymbInfo->eShiftState || bCombinedCase) {
                nTotCount = pKDBInfo->Private.pMTKdbKey->nMultitapCharCount;
                psSymb = pKDBInfo->Private.pMTKdbKey->psMultitapChars;
            }
            else {
                nTotCount = pKDBInfo->Private.pMTKdbKey->nMultitapShiftedCharCount;
                psSymb = pKDBInfo->Private.pMTKdbKey->psMultitapShiftedChars;
            }

            pKDBInfo->Private.bMTSymbCount = 0;
            pKDBInfo->Private.bMTSymbCountSrc = 0;

            for (nCount = nTotCount; nCount; --nCount, ++psSymb) {

                ET9SYMB sSymb;
                ET9SYMB sSymbCnv;

                /* get symb from the saved source and apply case shift (things can become duplicates by this) */

                if (bCombinedCase) {

                    if (pSymbInfo->eShiftState) {
                        sSymb = _ET9SymToUpper(*psSymb, pWordSymbInfo->Private.wLocale);
                    }
                    else {
                        sSymb = _ET9SymToLower(*psSymb, pWordSymbInfo->Private.wLocale);
                    }
                }
                else {
                    sSymb = *psSymb;
                }

                /* OTFM convert */

                sSymbCnv = sSymb;

                if (pKDBInfo->Private.pConvertSymb) {
                    pKDBInfo->Private.pConvertSymb(pKDBInfo->Private.pConvertSymbInfo, &sSymbCnv);
                }

                /* add the resulting symb to the current sequence unless it's a duplicate */

                {
                    ET9U8 bSymbCount;
                    ET9SYMB *psSymbLook;

                    psSymbLook = pKDBInfo->Private.sMTSymbs;
                    for (bSymbCount = pKDBInfo->Private.bMTSymbCount; bSymbCount; --bSymbCount, ++psSymbLook) {
                        if (*psSymbLook == sSymbCnv) {
                            break;
                        }
                    }

                    if (!bSymbCount) {
                        pKDBInfo->Private.sMTSymbs[pKDBInfo->Private.bMTSymbCount++] = sSymbCnv;
                    }
                }

                pKDBInfo->Private.sMTSymbsSrc[pKDBInfo->Private.bMTSymbCountSrc++] = sSymb;

                if (pKDBInfo->Private.bMTSymbCountSrc >= ET9_KDB_MAX_MT_SYMBS) {
                    break;
                }
            }
        }
        else {

            ET9U8 bCount;
            ET9U8 bSymbCount;
            ET9SYMB *psSymb;
            ET9SYMB *psSymbConv;
            ET9SYMB *psSymbLook;

            WLOG5(fprintf(pLogFile5, "  loading MT seq (from symb info)\n");)

            pKDBInfo->Private.bMTSymbCount = 0;
            psSymb = pKDBInfo->Private.sMTSymbsSrc;
            psSymbConv = pKDBInfo->Private.sMTSymbsCnv;
            for (bCount = pKDBInfo->Private.bMTSymbCountSrc; bCount; --bCount, ++psSymb, ++psSymbConv) {

                /* get symb from the saved source and apply case shift (things can become duplicates by this) */

                if (pSymbInfo->eShiftState) {
                    *psSymbConv = _ET9SymToUpper(*psSymb, pWordSymbInfo->Private.wLocale);
                }
                else {
                    *psSymbConv = _ET9SymToLower(*psSymb, pWordSymbInfo->Private.wLocale);
                }

                /* OTFM convert */

                if (pKDBInfo->Private.pConvertSymb) {
                    pKDBInfo->Private.pConvertSymb(pKDBInfo->Private.pConvertSymbInfo, psSymbConv);
                }

                /* add the resulting symb to the current sequence unless it's a duplicate */

                psSymbLook = pKDBInfo->Private.sMTSymbs;
                for (bSymbCount = pKDBInfo->Private.bMTSymbCount; bSymbCount; --bSymbCount, ++psSymbLook) {
                    if (*psSymbLook == *psSymbConv) {
                        break;
                    }
                }
                if (!bSymbCount) {
                    *psSymbLook = *psSymbConv;
                    ++pKDBInfo->Private.bMTSymbCount;
                }
            }
        }
    }

    /* log MT src */

    WLOG5({
        ET9U8 bIndex;

        fprintf(pLogFile5, "  MTSymbsSrc = ");

        for (bIndex = 0; bIndex < pKDBInfo->Private.bMTSymbCountSrc; ++bIndex) {
            fprintf(pLogFile5, "%04X ", pKDBInfo->Private.sMTSymbsSrc[bIndex]);
        }

        fprintf(pLogFile5, "\n");
    })

    /* log MT */

    WLOG5({
        ET9U8 bIndex;

        fprintf(pLogFile5, "  MTSymbs = ");

        for (bIndex = 0; bIndex < pKDBInfo->Private.bMTSymbCount; ++bIndex) {
            fprintf(pLogFile5, "%04X ", pKDBInfo->Private.sMTSymbs[bIndex]);
        }

        fprintf(pLogFile5, "\n");
    })

    /* first press OR loop through MT array until find last char and deliver the next. */

    WLOG5(fprintf(pLogFile5, "  bMTLastSymbIndex = %d (before)\n", pKDBInfo->Private.bMTLastSymbIndex);)

    ET9Assert(pKDBInfo->Private.bMTLastSymbIndex != 0xFF);

    if (!pKDBInfo->Private.bMTSymbCount) {
        /* change nothing, will exit soon and come back for a new round */
    }
    else if (eLoadAction == LOADSYMBACTION_RELOAD) {
        /* change nothing, will be handled using the previous active symb below */
    }
    else if (bIsFirstPress) {
        pKDBInfo->Private.bMTLastSymbIndex = 0;
    }
    else {
        pKDBInfo->Private.bMTLastSymbIndex = (ET9U8)((pKDBInfo->Private.bMTLastSymbIndex + 1) % pKDBInfo->Private.bMTSymbCount);
    }

    WLOG5(fprintf(pLogFile5, "  bMTLastSymbIndex = %d (after)\n", pKDBInfo->Private.bMTLastSymbIndex);)

    ET9Assert(pKDBInfo->Private.bMTLastSymbIndex != 0xFF);

    /* setup the actual symbol */

    _ET9ClearMem((ET9U8 *)pSymbInfo, sizeof(ET9SymbInfo));

    pKDBInfo->Private.wMTLastInput = wMTLastInput;

    pSymbInfo->wTapX = pKDBInfo->Private.sKdbAction.bIsKeyAction ? ET9UNDEFINEDTAPVALUE : pKDBInfo->Private.sKdbAction.u.tapAction.wX;
    pSymbInfo->wTapY = pKDBInfo->Private.sKdbAction.bIsKeyAction ? ET9UNDEFINEDTAPVALUE : pKDBInfo->Private.sKdbAction.u.tapAction.wY;
    pSymbInfo->wKeyIndex = pKDBInfo->Private.sKdbAction.bIsKeyAction ? pKDBInfo->Private.sKdbAction.u.keyAction.wKeyIndex : ET9UNDEFINEDKEYVALUE;
    pSymbInfo->bAmbigType = (ET9U8)ET9EXACT;
    pSymbInfo->eInputType = ET9MULTITAPKEY;
    pSymbInfo->bTraceProbability = 0;
    pSymbInfo->bTraceIndex = 0;
    pSymbInfo->bFreqsInvalidated = 1;

    __SetSymShiftState(pWordSymbInfo, pSymbInfo);

#if 0
    {
        const ET9BOOL bAutoCap = 0;

        if (bAutoCap)  {
            pSymbInfo->eShiftState = ET9SHIFT;
            pWordSymbInfo->Private.eLastShiftState = ET9SHIFT;
        }
    }
#endif

    pSymbInfo->bNumBaseSyms = 1;
    pDPBS = pSymbInfo->DataPerBaseSym;
    pDPBS->bNumSymsToMatch = 0;
    pDPBS->bSymFreq = pKDBInfo->Private.bUsingDynamicKDB ? 0xFF : 1;
    pDPBS->bDefaultCharIndex = 0;

    /* check empty */

    if (!pKDBInfo->Private.bMTSymbCount) {
        return;
    }

    /* on reload, use the previous active symb to find the right "last" index */

    if (eLoadAction == LOADSYMBACTION_RELOAD) {

        if (pKDBInfo->Private.pFilterSymbGroup) {

            ET9U8           bIndex;
            const ET9U8     bMTSymbCount = pKDBInfo->Private.bMTSymbCount;

            for (bIndex = 0; bIndex < bMTSymbCount; ++bIndex) {

                if (pKDBInfo->Private.pFilterSymbGroup(pKDBInfo->Private.pFilterSymbInfo,
                                                       sPreviousActiveSymb,
                                                       pKDBInfo->Private.sMTSymbs[bIndex])) {

                    WLOG5(fprintf(pLogFile5, "  reload, in group @ index %d\n", bIndex);)

                    pKDBInfo->Private.bMTLastSymbIndex = bIndex;
                    break;
                }
            }
        }

        if (pKDBInfo->Private.bMTLastSymbIndex >= pKDBInfo->Private.bMTSymbCount) {
            pKDBInfo->Private.bMTLastSymbIndex = 0;
        }

        WLOG5(fprintf(pLogFile5, "  reload, bMTLastSymbIndex = %d (after group)\n", pKDBInfo->Private.bMTLastSymbIndex);)
    }

    /* get the first symb */

    sFirstSymb = pKDBInfo->Private.sMTSymbs[pKDBInfo->Private.bMTLastSymbIndex];

    ET9Assert(sFirstSymb && sFirstSymb != (ET9SYMB)0xcccc && sFirstSymb != 0xFFFF);

    /* the zero element should have exact shift state only */

    pDPBS->sChar[pDPBS->bNumSymsToMatch] = sFirstSymb;
    pDPBS->sUpperCaseChar[pDPBS->bNumSymsToMatch] = sFirstSymb;

    ++pDPBS->bNumSymsToMatch;

    /* then again shifted */

    pDPBS->sChar[pDPBS->bNumSymsToMatch] = _ET9SymToLower(sFirstSymb, pWordSymbInfo->Private.wLocale);
    pDPBS->sUpperCaseChar[pDPBS->bNumSymsToMatch] = _ET9SymToUpper(sFirstSymb, pWordSymbInfo->Private.wLocale);

    ++pDPBS->bNumSymsToMatch;

    /* then all possible collapsed symbols */

    {
        ET9U8 bCount;
        ET9SYMB *psSymb;
        ET9SYMB *psSymbConv;

        psSymb = pKDBInfo->Private.sMTSymbsSrc;
        psSymbConv = pKDBInfo->Private.sMTSymbsCnv;
        for (bCount = pKDBInfo->Private.bMTSymbCountSrc; bCount; --bCount, ++psSymb, ++psSymbConv) {

            if (*psSymbConv != sFirstSymb) {
                continue;
            }

            if (pDPBS->bNumSymsToMatch >= ET9MAXBASESYMBS) {
                ++pDPBS;
                ++pSymbInfo->bNumBaseSyms;
                pDPBS->bNumSymsToMatch = 0;
                pDPBS->bSymFreq = 1;
                pDPBS->bDefaultCharIndex = 0;
            }

            ET9Assert(*psSymb && *psSymb != (ET9SYMB)0xcccc && *psSymb != (ET9SYMB)0xFFFF);

            pDPBS->sChar[pDPBS->bNumSymsToMatch] = _ET9SymToLower(*psSymb, pWordSymbInfo->Private.wLocale);
            pDPBS->sUpperCaseChar[pDPBS->bNumSymsToMatch] = _ET9SymToUpper(*psSymb, pWordSymbInfo->Private.wLocale);

            ++pDPBS->bNumSymsToMatch;
        }
    }

    /* request timeout */

    if (eLoadAction == LOADSYMBACTION_NEW) {

        pKDBInfo->dwStateBits |= ET9_KDB_INSERT_MASK;

        if (pKDBInfo->ET9Handle_KDB_Request) {
            __ET9KDB_RequestMultitapTimeout(pKDBInfo, pWordSymbInfo, wMTLastInput);
        }
    }
}

#ifdef ET9_KDB_TRACE_MODULE

/*---------------------------------------------------------------------------*/
/** \internal
 * Find the key that is the closest one to the supplied position.
 *
 * This function will ignore function keys.
 *
 * @param pKDBInfo                  .
 * @param fX                        .
 * @param fY                        .
 * @param pnAreaID                  .
 *
 * @return Non zero if found within radius, otherwise zero.
 */

static ET9BOOL ET9LOCALCALL __KeyAreasFindArea (ET9KDBInfo        const * const pKDBInfo,
                                                const ET9FLOAT                  fX,
                                                const ET9FLOAT                  fY,
                                                ET9UINT                 * const pnAreaID)
{
    ET9KdbLayoutInfo * const pLayoutInfo = pKDBInfo->Private.pCurrLayoutInfo;

    ET9KdbAreaInfo *pBestArea = NULL;

    WLOG5B(fprintf(pLogFile5, "__KeyAreasFindArea %5.1f %5.1f\n", fX, fY);)

    if (fX >= pKDBInfo->Private.pCurrLayoutInfo->wLayoutWidth || fY >= pKDBInfo->Private.pCurrLayoutInfo->wLayoutHeight) {
        WLOG5B(fprintf(pLogFile5, "  out-if-bounds\n");)
        return 0;
    }

    {
        ET9UINT nCount;
        ET9KdbAreaInfo *pArea;

        ET9UINT nBestOverlap;

        ET9Region sTargetRegion;

        {
            const ET9U16 wX = (ET9U16)(fX + 0.5);
            const ET9U16 wY = (ET9U16)(fY + 0.5);

            const ET9U16 wTargetOffsetX = (ET9U16)(pLayoutInfo->nMinKeyWidth / 2);
            const ET9U16 wTargetOffsetY = (ET9U16)(pLayoutInfo->nMinKeyHeight / 2);

            const ET9U16 wOffsetX = wX >= wTargetOffsetX ? wTargetOffsetX : wX;
            const ET9U16 wOffsetY = wY >= wTargetOffsetY ? wTargetOffsetY : wY;

            sTargetRegion.wLeft = (ET9U16)(wX - wOffsetX);
            sTargetRegion.wTop = (ET9U16)(wY - wOffsetY);
            sTargetRegion.wRight = (ET9U16)(sTargetRegion.wLeft + pLayoutInfo->nMinKeyWidth - 1);
            sTargetRegion.wBottom = (ET9U16)(sTargetRegion.wTop + pLayoutInfo->nMinKeyHeight - 1);
        }

        WLOG5B(fprintf(pLogFile5, "  sTargetRegion [%3u %3u %3u %3u]\n", sTargetRegion.wLeft, sTargetRegion.wTop, sTargetRegion.wRight, sTargetRegion.wBottom);)

        nBestOverlap = 0;
        pArea = pLayoutInfo->pKeyAreas;
        for (nCount = pLayoutInfo->nKeyAreaCount; nCount; --nCount, ++pArea) {

            if (pArea->eKeyType == ET9KTFUNCTION) {
                continue;
            }

            {
                const ET9UINT nOverlap = __GetRegionOverlap(&sTargetRegion, &pArea->sRegion);

                WLOG5B(fprintf(pLogFile5, "  key region [%3u %3u %3u %3u], nOverlap %5u\n", pArea->sRegion.wLeft, pArea->sRegion.wTop, pArea->sRegion.wRight, pArea->sRegion.wBottom, nOverlap);)

                if (nBestOverlap < nOverlap) {

                    WLOG5B(fprintf(pLogFile5, "    new best (%u > %u)\n", nOverlap, nBestOverlap);)

                    nBestOverlap = nOverlap;
                    pBestArea = pArea;
                }
            }
        }
    }

    if (!pBestArea) {
        return 0;
    }

    if (pnAreaID) {
        *pnAreaID = pBestArea->wKeyIndex;
    }

    WLOG5B(fprintf(pLogFile5, "  found key %u\n", pBestArea->wKeyIndex);)

    return 1;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pKDBInfo                  .
 * @param wKey                      .
 * @param pnX                       .
 * @param pnY                       .
 *
 * @return Xxx
 */

static void ET9LOCALCALL __KeyAreasGetKeyCenter (ET9KDBInfo         * const pKDBInfo,
                                                 const ET9U16               wKey,
                                                 ET9UINT            * const pnX,
                                                 ET9UINT            * const pnY)
{
    ET9KdbAreaInfo      const * pArea = __GetKeyAreaFromKey_Generic(pKDBInfo, wKey);

    if (!pArea) {
        *pnX = 0;
        *pnY = 0;
        return;
    }

    *pnX = pArea->nCenterX;
    *pnY = pArea->nCenterY;
}

#endif

/*---------------------------------------------------------------------------*/
/** \internal
 * This function processes an ambig key for the current KDB.
 *
 * @param pKDBInfo             Pointer to keyboard information structure.
 * @param wKeyIndex            Key index.
 * @param pWordSymbInfo        Pointer to tap info.
 * @param psFunctionKey        Pointer to return function key info in.
 * @param bCurrIndexInList     Currently highlighted sel list entry.
 * @param eLoadAction          If new, appending etc.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

static ET9STATUS ET9LOCALCALL __ProcessAmbigKey_CurrentKdb(ET9KDBInfo           * const pKDBInfo,
                                                           const ET9U16                 wKeyIndex,
                                                           ET9WordSymbInfo      * const pWordSymbInfo,
                                                           ET9SYMB              * const psFunctionKey,
                                                           const ET9U8                  bCurrIndexInList,
                                                           const ET9LOADSYMBACTION      eLoadAction)
{
    ET9STATUS eStatus;

    if (pKDBInfo->Private.bUsingDynamicKDB) {

        eStatus = __ProcessAmbigKey_CurrentKdb_Generic(pKDBInfo,
                                                       wKeyIndex,
                                                       pWordSymbInfo,
                                                       psFunctionKey,
                                                       bCurrIndexInList,
                                                       eLoadAction);
    }
    else {

        eStatus = __ProcessAmbigKey_CurrentKdb_Static(pKDBInfo,
                                                      wKeyIndex,
                                                      pWordSymbInfo,
                                                      psFunctionKey,
                                                      bCurrIndexInList,
                                                      eLoadAction);
    }

    return eStatus;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * This function processes an ambig key for active KDBs.
 *
 * @param pKDBInfo             Pointer to keyboard information structure.
 * @param wKeyIndex            Key index.
 * @param pWordSymbInfo        Pointer to tap info.
 * @param psFunctionKey        Pointer to return function key info in.
 * @param bCurrIndexInList     Currently highlighted sel list entry.
 * @param eLoadAction          If new, appending etc.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

static ET9STATUS ET9LOCALCALL __ProcessAmbigKey_ActiveKdbs(ET9KDBInfo           * const pKDBInfo,
                                                           const ET9U16                 wKeyIndex,
                                                           ET9WordSymbInfo      * const pWordSymbInfo,
                                                           ET9SYMB              * const psFunctionKey,
                                                           const ET9U8                  bCurrIndexInList,
                                                           const ET9LOADSYMBACTION      eLoadAction)
{
    ET9STATUS       eStatus;

    ET9BOOL         bFirstOk;

    ET9U16          wKdb1;
    ET9U16          wKdb2;
    ET9U16          wPage1;
    ET9U16          wPage2;

    ET9SymbInfo * const pFirstNewSymbInfo = &pWordSymbInfo->SymbsInfo[pWordSymbInfo->bNumSymbs];

    WLOG5(fprintf(pLogFile5, "__ProcessAmbigKey_ActiveKdbs, pKDBInfo = %p, eLoadAction = %d\n", pKDBInfo, (int)eLoadAction);)

    ET9Assert(ET9_KDB_AMBIGUOUS_MODE(pKDBInfo->dwStateBits));

    /* figure out load order */

    if (_ET9KDBSecondKDBSupported(pKDBInfo)) {

        if (pWordSymbInfo->Private.bSwitchLanguage) {
            wKdb1  = pKDBInfo->wSecondKdbNum;
            wPage1 = pKDBInfo->wSecondPageNum;
            wKdb2  = pKDBInfo->wFirstKdbNum;
            wPage2 = pKDBInfo->wFirstPageNum;
        }
        else {
            wKdb1  = pKDBInfo->wFirstKdbNum;
            wPage1 = pKDBInfo->wFirstPageNum;
            wKdb2  = pKDBInfo->wSecondKdbNum;
            wPage2 = pKDBInfo->wSecondPageNum;
        }
    }
    else {
        wKdb1  = pKDBInfo->wFirstKdbNum;
        wPage1 = pKDBInfo->wFirstPageNum;
        wKdb2  = 0;
        wPage2 = 0;
    }

    /* load number 1 */

    bFirstOk = 0;

    eStatus = __KDBLoadPage(pKDBInfo, wKdb1, wPage1, NULL);

    if (!eStatus) {

        eStatus = __ProcessAmbigKey_CurrentKdb(pKDBInfo, wKeyIndex, pWordSymbInfo, psFunctionKey, bCurrIndexInList, eLoadAction);

        if (!eStatus) {
            bFirstOk = 1;
        }

        /* make sure that we don't append on full */

        if (eStatus == ET9STATUS_FULL) {
            return eStatus;
        }
    }

    /* load number 2 */

    if (wKdb2) {

        eStatus = __KDBLoadPage(pKDBInfo, wKdb2, wPage2, NULL);

        if (!eStatus) {

            eStatus = __ProcessAmbigKey_CurrentKdb(pKDBInfo, wKeyIndex, pWordSymbInfo, psFunctionKey, bCurrIndexInList, (bFirstOk ? LOADSYMBACTION_APPEND : eLoadAction));
        }
    }

    /* history */

    if (eLoadAction == LOADSYMBACTION_NEW && !eStatus) {
        pFirstNewSymbInfo->wKdb1    = wKdb1;
        pFirstNewSymbInfo->wPage1   = wPage1;
        pFirstNewSymbInfo->wKdb2    = wKdb2;
        pFirstNewSymbInfo->wPage2   = wPage2;
    }

    /* done */

    return eStatus;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * This function processes an ambig tap for the current KDB.
 *
 * @param pKDBInfo             Pointer to keyboard information structure.
 * @param pDirectedPos         Tap position.
 * @param pWordSymbInfo        Pointer to tap info.
 * @param psFunctionKey        Pointer to return function key info in.
 * @param bCurrIndexInList     Currently highlighted sel list entry.
 * @param eLoadAction          If new, appending etc.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

static ET9STATUS ET9LOCALCALL __ProcessAmbigTap_CurrentKdb(ET9KDBInfo           * const pKDBInfo,
                                                           ET9DirectedPos       * const pDirectedPos,
                                                           ET9WordSymbInfo      * const pWordSymbInfo,
                                                           ET9SYMB              * const psFunctionKey,
                                                           const ET9U8                  bCurrIndexInList,
                                                           const ET9LOADSYMBACTION      eLoadAction)
{
    ET9STATUS eStatus;

    if (pKDBInfo->Private.bUsingDynamicKDB || pDirectedPos->bForceGeneric) {

        *psFunctionKey = 0;

        eStatus = __ProcessAmbigTap_CurrentKdb_Generic(pKDBInfo,
                                                       pDirectedPos,
                                                       pWordSymbInfo,
                                                       psFunctionKey,
                                                       bCurrIndexInList,
                                                       eLoadAction);
    }
    else {

        eStatus = __ProcessAmbigTap_CurrentKdb_Static(pKDBInfo,
                                                      (ET9U16)pDirectedPos->sPos.nX,
                                                      (ET9U16)pDirectedPos->sPos.nY,
                                                      pWordSymbInfo,
                                                      psFunctionKey,
                                                      bCurrIndexInList,
                                                      eLoadAction);
    }

    return eStatus;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * This function processes an ambig tap for active KDBs.
 *
 * @param pKDBInfo             Pointer to keyboard information structure.
 * @param pDirectedPos         Tap position.
 * @param pWordSymbInfo        Pointer to tap info.
 * @param psFunctionKey        Pointer to return function key info in.
 * @param bCurrIndexInList     Currently highlighted sel list entry.
 * @param eLoadAction          If new, appending etc.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

static ET9STATUS ET9LOCALCALL __ProcessAmbigTap_ActiveKdbs(ET9KDBInfo           * const pKDBInfo,
                                                           ET9DirectedPos       * const pDirectedPos,
                                                           ET9WordSymbInfo      * const pWordSymbInfo,
                                                           ET9SYMB              * const psFunctionKey,
                                                           const ET9U8                  bCurrIndexInList,
                                                           const ET9LOADSYMBACTION      eLoadAction)
{
    ET9STATUS   eStatus;

    ET9BOOL     bFirstOk;

    ET9U16      wKdb1;
    ET9U16      wKdb2;
    ET9U16      wPage1;
    ET9U16      wPage2;

    ET9SymbInfo * const pFirstNewSymbInfo = &pWordSymbInfo->SymbsInfo[pWordSymbInfo->bNumSymbs];

    WLOG5(fprintf(pLogFile5, "__ProcessAmbigTap_ActiveKdbs, pKDBInfo = %p, eLoadAction = %d\n", pKDBInfo, (int)eLoadAction);)

    ET9Assert(ET9_KDB_AMBIGUOUS_MODE(pKDBInfo->dwStateBits));

    /* figure out load order */

    if (_ET9KDBSecondKDBSupported(pKDBInfo)) {

        if (pWordSymbInfo->Private.bSwitchLanguage) {
            wKdb1  = pKDBInfo->wSecondKdbNum;
            wPage1 = pKDBInfo->wSecondPageNum;
            wKdb2  = pKDBInfo->wFirstKdbNum;
            wPage2 = pKDBInfo->wFirstPageNum;
        }
        else {
            wKdb1  = pKDBInfo->wFirstKdbNum;
            wPage1 = pKDBInfo->wFirstPageNum;
            wKdb2  = pKDBInfo->wSecondKdbNum;
            wPage2 = pKDBInfo->wSecondPageNum;
        }
    }
    else {
        wKdb1  = pKDBInfo->wFirstKdbNum;
        wPage1 = pKDBInfo->wFirstPageNum;
        wKdb2  = 0;
        wPage2 = 0;
    }

    /* load number 1 */

    bFirstOk = 0;

    eStatus = __KDBLoadPage(pKDBInfo, wKdb1, wPage1, NULL);     /* should really be a "directed" version */

    if (!eStatus) {

        eStatus = __ProcessAmbigTap_CurrentKdb(pKDBInfo, pDirectedPos, pWordSymbInfo, psFunctionKey, bCurrIndexInList, eLoadAction);

        if (!eStatus) {
            bFirstOk = 1;
        }

        /* make sure that we don't append on full */

        if (eStatus == ET9STATUS_FULL) {
            return eStatus;
        }
    }

    /* load number 2 */

    if (wKdb2) {

        eStatus = __KDBLoadPage(pKDBInfo, wKdb2, wPage2, NULL);     /* should really be a "directed" version */

        if (!eStatus) {

            eStatus = __ProcessAmbigTap_CurrentKdb(pKDBInfo, pDirectedPos, pWordSymbInfo, psFunctionKey, bCurrIndexInList, (bFirstOk ? LOADSYMBACTION_APPEND : eLoadAction));
        }
    }

    /* history */

    if (eLoadAction == LOADSYMBACTION_NEW && !eStatus) {
        pFirstNewSymbInfo->wKdb1    = wKdb1;
        pFirstNewSymbInfo->wPage1   = wPage1;
        pFirstNewSymbInfo->wKdb2    = wKdb2;
        pFirstNewSymbInfo->wPage2   = wPage2;
    }

    /* done */

    return eStatus;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * This function processes a multitap key on the current KDB.
 *
 * @param pKDBInfo             Pointer to keyboard information structure.
 * @param wKeyIndex            Key index.
 * @param pWordSymbInfo        Pointer to tap info.
 * @param psFunctionKey        Pointer to return function key info in.
 * @param bCurrIndexInList     Currently highlighted sel list entry.
 * @param eLoadAction          If new, appending etc.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

static ET9STATUS ET9LOCALCALL __ProcessMultitapKey_CurrentKdb(ET9KDBInfo            * const pKDBInfo,
                                                              const ET9U16                  wKeyIndex,
                                                              ET9WordSymbInfo       * const pWordSymbInfo,
                                                              ET9SYMB               * const psFunctionKey,
                                                              const ET9U8                   bCurrIndexInList,
                                                              const ET9LOADSYMBACTION       eLoadAction)
{
    ET9STATUS  eStatus;
    ET9BOOL    bIsFirstPress;

    WLOG5(fprintf(pLogFile5, "__ProcessMultitapKey_CurrentKdb, pKDBInfo = %p, eLoadAction = %d\n", pKDBInfo, (int)eLoadAction);)

    ET9Assert(pKDBInfo);
    ET9Assert(pWordSymbInfo);
    ET9Assert(psFunctionKey);

    bIsFirstPress = (ET9BOOL)(!(ET9_KDB_INSERT_MODE(pKDBInfo->dwStateBits)) ||
                              !pWordSymbInfo->bNumSymbs ||
                              wKeyIndex != pKDBInfo->Private.wMTLastInput);

    if (pKDBInfo->Private.sKdbAction.bShiftState) {
        pWordSymbInfo->dwStateBits |= ET9STATE_SHIFT_MASK;
    }

    if (bIsFirstPress || eLoadAction == LOADSYMBACTION_RELOAD) {

        /* first process it as if it had been ambiguous, this gets the MT list from the database. */

        eStatus = __ProcessAmbigKey_CurrentKdb(pKDBInfo, wKeyIndex, pWordSymbInfo, psFunctionKey, bCurrIndexInList, eLoadAction);

        if (eStatus || *psFunctionKey) {
            return eStatus;
        }
    }

    /* if there is only one character in multi-tap sequence, second press of the key will accept the character */

    else if (pKDBInfo->Private.bMTSymbCount == 1) {

        pWordSymbInfo->dwStateBits &= ~ET9STATE_SHIFT_MASK;
        pKDBInfo->dwStateBits &= ~ET9_KDB_INSERT_MASK;

        return ET9STATUS_NONE;
    }

    __ProcessMultitap(pKDBInfo, pWordSymbInfo, bIsFirstPress, psFunctionKey, bCurrIndexInList, wKeyIndex, eLoadAction);

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * This function processes a multitap key for active KDBs.
 *
 * @param pKDBInfo             Pointer to keyboard information structure.
 * @param wKeyIndex            Key index.
 * @param pWordSymbInfo        Pointer to tap info.
 * @param psFunctionKey        Pointer to return function key info in.
 * @param bCurrIndexInList     Currently highlighted sel list entry.
 * @param eLoadAction          If new, appending etc.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

static ET9STATUS ET9LOCALCALL __ProcessMultitapKey_ActiveKdbs(ET9KDBInfo           * const pKDBInfo,
                                                              const ET9U16                 wKeyIndex,
                                                              ET9WordSymbInfo      * const pWordSymbInfo,
                                                              ET9SYMB              * const psFunctionKey,
                                                              const ET9U8                  bCurrIndexInList,
                                                              const ET9LOADSYMBACTION      eLoadAction)
{
    ET9STATUS       eStatus;

    ET9U16          wKdb1;
    ET9U16          wPage1;

    ET9SymbInfo * const pFirstNewSymbInfo = &pWordSymbInfo->SymbsInfo[pWordSymbInfo->bNumSymbs];

    WLOG5(fprintf(pLogFile5, "__ProcessMultitapKey_ActiveKdbs, pKDBInfo = %p, eLoadAction = %d\n", pKDBInfo, (int)eLoadAction);)

    ET9Assert(ET9_KDB_MULTITAP_MODE(pKDBInfo->dwStateBits));

    /* figure out load order */

    if (pWordSymbInfo->Private.bSwitchLanguage && _ET9KDBSecondKDBSupported(pKDBInfo)) {

        wKdb1  = pKDBInfo->wSecondKdbNum;
        wPage1 = pKDBInfo->wSecondPageNum;
    }
    else {
        wKdb1  = pKDBInfo->wFirstKdbNum;
        wPage1 = pKDBInfo->wFirstPageNum;
    }

    /* load number 1 */

    eStatus = __KDBLoadPage(pKDBInfo, wKdb1, wPage1, NULL);

    if (!eStatus) {

        eStatus = __ProcessMultitapKey_CurrentKdb(pKDBInfo, wKeyIndex, pWordSymbInfo, psFunctionKey, bCurrIndexInList, eLoadAction);
    }

    /* history */

    if (eLoadAction == LOADSYMBACTION_NEW && !eStatus) {
        pFirstNewSymbInfo->wKdb1    = wKdb1;
        pFirstNewSymbInfo->wPage1   = wPage1;
        pFirstNewSymbInfo->wKdb2    = 0;
        pFirstNewSymbInfo->wPage2   = 0;
    }

    /* done */

    return eStatus;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * This function finds the first tap char.
 *
 * @param pKDBInfo             Pointer to keyboard information structure.
 * @param pDirectedPos         Tap position.
 * @param psFirstChar          Pointer to first char symbol.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

static ET9STATUS ET9LOCALCALL __GetFirstTapChar(ET9KDBInfo            * const pKDBInfo,
                                                ET9DirectedPos        * const pDirectedPos,
                                                ET9SYMB               * const psFirstChar)
{
    ET9STATUS eStatus;

    if (pKDBInfo->Private.bUsingDynamicKDB || pDirectedPos->bForceGeneric) {
        eStatus = __GetFirstTapChar_Generic(pKDBInfo, pDirectedPos, psFirstChar);
    }
    else {
        eStatus = __GetFirstTapChar_Static(pKDBInfo, pDirectedPos, psFirstChar);
    }

    return eStatus;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * This function processes an ambig tap for the current KDB.
 *
 * @param pKDBInfo             Pointer to keyboard information structure.
 * @param pDirectedPos         Tap position.
 * @param pWordSymbInfo        Pointer to tap info.
 * @param psFunctionKey        Pointer to return function key info in.
 * @param bCurrIndexInList     Currently highlighted sel list entry.
 * @param eLoadAction          If new, appending etc.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

static ET9STATUS ET9LOCALCALL __ProcessMultitapTap_CurrentKdb(ET9KDBInfo           * const pKDBInfo,
                                                              ET9DirectedPos       * const pDirectedPos,
                                                              ET9WordSymbInfo      * const pWordSymbInfo,
                                                              ET9SYMB              * const psFunctionKey,
                                                              const ET9U8                  bCurrIndexInList,
                                                              const ET9LOADSYMBACTION      eLoadAction)
{
    ET9STATUS  eStatus;
    ET9BOOL    bIsFirstPress;
    ET9SYMB    sFirstChar;

    WLOG5(fprintf(pLogFile5, "__ProcessMultitapTap_CurrentKdb, pKDBInfo = %p, eLoadAction = %d, wX = %d, wY = %d\n", pKDBInfo, (int)eLoadAction, pDirectedPos->sPos.nX, pDirectedPos->sPos.nY);)

    ET9Assert(pKDBInfo);
    ET9Assert(pWordSymbInfo);
    ET9Assert(psFunctionKey);

    eStatus = __GetFirstTapChar(pKDBInfo, pDirectedPos, &sFirstChar);

    if (eStatus) {
        return eStatus;
    }

    bIsFirstPress = (ET9BOOL)(!(ET9_KDB_INSERT_MODE(pKDBInfo->dwStateBits)) ||
                              !pWordSymbInfo->bNumSymbs ||
                              sFirstChar != pKDBInfo->Private.wMTLastInput);

    if (pKDBInfo->Private.sKdbAction.bShiftState) {
        pWordSymbInfo->dwStateBits |= ET9STATE_SHIFT_MASK;
    }

    if (bIsFirstPress || eLoadAction == LOADSYMBACTION_RELOAD) {

        /* first process it as if it had been ambiguous, this gets the MT list from the database. */

        eStatus = __ProcessAmbigTap_CurrentKdb(pKDBInfo, pDirectedPos, pWordSymbInfo, psFunctionKey, bCurrIndexInList, eLoadAction);

        if (eStatus || *psFunctionKey) {
            return eStatus;
        }
    }

    /* if there is only one character in multi-tap sequence, second press of the key will accept the character */

    else if (pKDBInfo->Private.bMTSymbCount == 1) {

        pWordSymbInfo->dwStateBits &= ~ET9STATE_SHIFT_MASK;
        pKDBInfo->dwStateBits &= ~ET9_KDB_INSERT_MASK;

        return ET9STATUS_NONE;
    }

    __ProcessMultitap(pKDBInfo, pWordSymbInfo, bIsFirstPress, psFunctionKey, bCurrIndexInList, sFirstChar, eLoadAction);

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * This function processes a multitap tap for active KDBs.
 *
 * @param pKDBInfo             Pointer to keyboard information structure.
 * @param pDirectedPos         Tap position.
 * @param pWordSymbInfo        Pointer to tap info.
 * @param psFunctionKey        Pointer to return function key info in.
 * @param bCurrIndexInList     Currently highlighted sel list entry.
 * @param eLoadAction          If new, appending etc.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

static ET9STATUS ET9LOCALCALL __ProcessMultitapTap_ActiveKdbs(ET9KDBInfo           * const pKDBInfo,
                                                              ET9DirectedPos       * const pDirectedPos,
                                                              ET9WordSymbInfo      * const pWordSymbInfo,
                                                              ET9SYMB              * const psFunctionKey,
                                                              const ET9U8                  bCurrIndexInList,
                                                              const ET9LOADSYMBACTION      eLoadAction)
{
    ET9STATUS       eStatus;

    ET9U16          wKdb1;
    ET9U16          wPage1;

    WLOG5(fprintf(pLogFile5, "__ProcessMultitapTap_ActiveKdbs, pKDBInfo = %p, eLoadAction = %d\n", pKDBInfo, (int)eLoadAction);)

    ET9Assert(ET9_KDB_MULTITAP_MODE(pKDBInfo->dwStateBits));

    /* figure out load order */

    if (pWordSymbInfo->Private.bSwitchLanguage && _ET9KDBSecondKDBSupported(pKDBInfo)) {

        wKdb1  = pKDBInfo->wSecondKdbNum;
        wPage1 = pKDBInfo->wSecondPageNum;
    }
    else {
        wKdb1  = pKDBInfo->wFirstKdbNum;
        wPage1 = pKDBInfo->wFirstPageNum;
    }

    /* load number 1 */

    eStatus = __KDBLoadPage(pKDBInfo, wKdb1, wPage1, NULL);

    if (!eStatus) {

        eStatus = __ProcessMultitapTap_CurrentKdb(pKDBInfo, pDirectedPos, pWordSymbInfo, psFunctionKey, bCurrIndexInList, eLoadAction);
    }

    return eStatus;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Informs the Keyboard Input Module that the user has entered tap-based input.
 *
 * @param[in]     pKDBInfo          Pointer to the Keyboard Input Module Information Data Structure (ET9KDBInfo).
 * @param[in,out] pWordSymbInfo     Pointer to the Word Symbol Data Structure (ET9WordSymbInfo). The Keyboard Input Module writes to this structure information about the input the user has provided.
 * @param[in]     pDirectedPos      Tap position.
 * @param[in]     bCurrIndexInList  0-based index value that indicates which word in the selection list is currently selected. XT9 locks this word before it adds the specified input value.<br>
 *                                  If there is no current input sequence, set the value of this parameter to
 *                                  ET9_NO_ACTIVE_INDEX.<br>
 *                                  If the integration layer passes as an invalid argument for this parameter, XT9 will
 *                                  still add the new input, but it will not lock the word. Also, it will not return an error status.
 * @param[out]    psFunctionKey     Pointer to a buffer where the Keyboard Input Module stores information about the tapped/pressed key if it is a function key requiring action by the integration layer (for example, changing the shift state or input mode).<br>
 *                                  Function key values are part of the ET9KDBKEYDEF enumeration.<br>
 *                                  If the key that was pressed or tapped is not a function key, XT9 sets the value to 0.
 */

static ET9STATUS ET9LOCALCALL __ProcessTap(ET9KDBInfo      * const pKDBInfo,
                                           ET9WordSymbInfo * const pWordSymbInfo,
                                           ET9DirectedPos  * const pDirectedPos,
                                           const ET9U8             bCurrIndexInList,
                                           ET9SYMB         * const psFunctionKey)
{
    ET9STATUS eStatus;

    WLOG5(fprintf(pLogFile5, "\n__ProcessTap, pKDBInfo = %p\n", pKDBInfo);)

    __FilterSymbReset(pKDBInfo);

    pKDBInfo->Private.sKdbAction.bIsKeyAction = 0;
    pKDBInfo->Private.sKdbAction.bCurrIndexInList = bCurrIndexInList;
    pKDBInfo->Private.sKdbAction.bShiftState = ET9SHIFT_MODE(pWordSymbInfo->dwStateBits) ? 1 : 0;
    pKDBInfo->Private.sKdbAction.u.tapAction.wX = (ET9U16)pDirectedPos->sPos.nX;
    pKDBInfo->Private.sKdbAction.u.tapAction.wY = (ET9U16)pDirectedPos->sPos.nY;

    *psFunctionKey = 0;

    if (ET9_KDB_AMBIGUOUS_MODE(pKDBInfo->dwStateBits)) {

        pKDBInfo->dwStateBits &= ~ET9_KDB_INSERT_MASK;

        eStatus = __ProcessAmbigTap_ActiveKdbs(pKDBInfo, pDirectedPos, pWordSymbInfo, psFunctionKey, bCurrIndexInList, LOADSYMBACTION_NEW);
    }
    else if (ET9_KDB_MULTITAP_MODE(pKDBInfo->dwStateBits)) {

        eStatus = __ProcessMultitapTap_ActiveKdbs(pKDBInfo, pDirectedPos, pWordSymbInfo, psFunctionKey, bCurrIndexInList, LOADSYMBACTION_NEW);
    }
    else {
        return ET9STATUS_ERROR;
    }

    /* error above? */

    if (eStatus) {
        return eStatus;
    }

    /* shut off shift bit */

    if (!ET9_KDB_INSERT_MODE(pKDBInfo->dwStateBits) && !*psFunctionKey) {
        pWordSymbInfo->dwStateBits &= ~ET9STATE_SHIFT_MASK;
    }

    if (pWordSymbInfo->bNumSymbs == ET9MAXLDBWORDSIZE) {

        if (_ET9IsMagicStringKey(pWordSymbInfo)) {

            eStatus = ET9KDB_GetKdbVersion(pKDBInfo,
                                           pWordSymbInfo->Private.szIDBVersion,
                                           ET9MAXVERSIONSTR,
                                           &pWordSymbInfo->Private.wIDBVersionStrSize);
        }
    }

    /* mark valid if not a function key */

    if (!*psFunctionKey && pKDBInfo->Private.pFilterSymb) {
        pKDBInfo->Private.sKdbAction.dwCurrChecksum = __CalculateLastWordSymbChecksum(pWordSymbInfo);
    }

    /* turn off correction inhibit */

    if (!*psFunctionKey) {
        pWordSymbInfo->Private.bRequiredInhibitCorrection = 0;  /* not the override */
    }

    /* done */

    return ET9STATUS_NONE;
}

/* ************************************************************************************************************** */
/* * PUBLIC ***************************************************************************************************** */
/* ************************************************************************************************************** */

/*---------------------------------------------------------------------------*/
/**
 * @brief Initializes the Keyboard Input Module.
 *
 * @param[in]     pKDBInfo                  Pointer to the Keyboard Input Module Information Data Structure (ET9KDBInfo).
 * @param[in]     wFirstKdbNum              Identification number of the keyboard to be used by the first language.
 * @param[in]     wFirstPageNum             Current page number of the keyboard to be used by the first language. Valid values range from 1 to the maximum number of keyboard pages indicated in the KDB text file.
 * @param[in]     wSecondKdbNum             Identification number of the keyboard to be used by the second language.<br>
 *                                          Set this parameter to 0 if you are not implementing the bilingual feature
 * @param[in]     wSecondPageNum            Current page number of the keyboard to be used used by the second language.<br>
 *                                          Set this parameter to NULL if you are not implementing the bilingual feature.
 * @param[in]     pKDBLoadData              Pointer to the ET9KDBLOADCALLBACK() function. You must implement one of the load and read callbacks.
 * @param[in]     pKDBReadData              Pointer to the ET9KDBREADCALLBACK() function. You must implement one of the load and read callbacks.
 * @param[in]     ET9Handle_KDB_Request     Pointer to the ET9KDBREQUESTCALLBACK() function you must implement.
 * @param[in]     pPublicExtension          A value the integration layer can set and then retrieve in pKdbInfo->pPublicExtension.
 *
 * @retval ET9STATUS_NONE               Function call was handled successfully.
 * @retval ET9STATUS_INVALID_MEMORY     At least one pointer passed as a function argument was set to null.
 * @retval ET9STATUS_NO_INIT            The Keyboard Input Module has not been initialized. Call ET9KDB_Init().
 * @retval ET9STATUS_ABORT              XT9 has encountered a severe error. Often this status is returned because the integration layer did not allocate sufficient memory for required data structures or because required identification numbers (such as for the LDB and keyboard) are set to 0.
 * @retval ET9STATUS_DB_CORE_INCOMP     The keyboard and Keyboard Input Module are not compatible. Contact your Nuance support engineer for assistance.
 * @retval ET9STATUS_INVALID_DB_TYPE    The keyboard specified by wKDBNum is not valid. Contact your Nuance support engineer for assistance.
 * @retval ET9STATUS_INVALID_KDB_PAGE   The page specified by wPageNum is not valid for the keyboard specified by wKDBNum.
 * @retval ET9STATUS_KDB_VERSION_ERROR  The version information in the keyboard specified by wKDBNum is not valid. Contact your Nuance support engineer for assistance.
 * @retval ET9STATUS_NO_KEY             No keys are associated with the keyboard page specified by wPageNum.
 * @retval ET9STATUS_READ_DB_FAIL       The XT9 core cannot read data from the keyboard. This typically indicates a problem with the ET9KDBREADCALLBACK() function you have implemented.
 * @retval ET9STATUS_WRONG_OEMID        The OEM ID in the keyboard information does not match the XT9 core OEM ID. Contact your Nuance support engineer for assistance.
 *
 * @remarks The integration layer must initialize this module before it can be used for text entry. After calling ET9KDB_Init,
 * the integration layer should initialize the Linguistic Module (if it is not already initialized).<br>
 * When the integration layer initializes the Keyboard Input Module, the module is set by default to Ambiguous mode and a lowercase shift state.
 *
 */

ET9STATUS ET9FARCALL ET9KDB_Init(ET9KDBInfo                 * const pKDBInfo,
                                 const ET9U16                       wFirstKdbNum,
                                 const ET9U16                       wFirstPageNum,
                                 const ET9U16                       wSecondKdbNum,
                                 const ET9U16                       wSecondPageNum,
                                 const ET9KDBLOADCALLBACK           pKDBLoadData,
                                 const ET9KDBREADCALLBACK           pKDBReadData,
                                 const ET9KDBREQUESTCALLBACK        ET9Handle_KDB_Request,
                                 void                       * const pPublicExtension)
{
    ET9STATUS   eStatus;

    WLOG5(fprintf(pLogFile5, "\nET9KDB_Init, pKDBInfo = %p\n", pKDBInfo);)

    WLOG5(fprintf(pLogFile5, "\n");)
    WLOG5(fprintf(pLogFile5, "  sizeof(ET9KDBInfo)                  = %6u\n", sizeof(ET9KDBInfo));)
    WLOG5(fprintf(pLogFile5, "    sizeof(pLayoutInfos)              = %6u\n", sizeof(ET9KdbLayoutInfo) * ET9_KDB_MAX_PAGE_CACHE);)
    WLOG5(fprintf(pLogFile5, "      sizeof(ET9KdbLayoutInfo)        = %6u\n", sizeof(ET9KdbLayoutInfo));)
    WLOG5(fprintf(pLogFile5, "    sizeof(wm)                        = %6u\n", sizeof(pKDBInfo->Private.wm));)
    WLOG5(fprintf(pLogFile5, "      sizeof(staticLoad)              = %6u\n", sizeof(pKDBInfo->Private.wm.staticLoad));)
    WLOG5(fprintf(pLogFile5, "        sizeof(ET9KdbAreaExtraInfo)   = %6u\n", sizeof(ET9KdbAreaExtraInfo));)
    WLOG5(fprintf(pLogFile5, "      sizeof(traceEvent)              = %6u\n", sizeof(pKDBInfo->Private.wm.traceEvent));)
    WLOG5(fprintf(pLogFile5, "        sizeof(ET9TracePointExt)      = %6u\n", sizeof(ET9TracePointExt));)
    WLOG5(fprintf(pLogFile5, "      sizeof(xmlReader)               = %6u\n", sizeof(pKDBInfo->Private.wm.xmlReader));)
    WLOG5(fprintf(pLogFile5, "        sizeof(ET9KdbXmlKeyboardInfo) = %6u\n", sizeof(pKDBInfo->Private.wm.xmlReader.sKeyboardInfo));)
    WLOG5(fprintf(pLogFile5, "          sizeof(ET9KdbXmlKey)        = %6u (%6u)\n", sizeof(ET9KdbXmlKey), sizeof(ET9KdbXmlKey) * ET9_KDB_MAX_KEYS);)
    WLOG5(fprintf(pLogFile5, "          sizeof(ET9KdbXmlRow)        = %6u (%6u)\n", sizeof(ET9KdbXmlRow), sizeof(ET9KdbXmlKey) * ET9_KDB_MAX_ROWS);)
    WLOG5(fprintf(pLogFile5, "          sizeof(ET9KdbXmlKeyboard)   = %6u\n", sizeof(ET9KdbXmlKeyboard));)
    WLOG5(fprintf(pLogFile5, "          sizeof(psSymbPool)          = %6u\n", sizeof(pKDBInfo->Private.wm.xmlReader.sKeyboardInfo.psSymbPool));)
    WLOG5(fprintf(pLogFile5, "\n");)

#ifdef ET9_KDB_TRACE_MODULE
    if (_ET9ByteStringCheckSum(_pbXt9Trace) != 4250608233U) {
        return ET9STATUS_ERROR;
    }
#endif

    if (!pKDBInfo || (!pKDBLoadData && !pKDBReadData) || (pKDBLoadData && pKDBReadData)) {
        return ET9STATUS_INVALID_MEMORY;
    }

    _ET9ClearMem((ET9U8*)pKDBInfo, sizeof(ET9KDBInfo));

    pKDBInfo->ET9Handle_KDB_Request = ET9Handle_KDB_Request;
    pKDBInfo->pKDBLoadData = pKDBLoadData;
    pKDBInfo->ET9KDBReadData = pKDBReadData;
    pKDBInfo->pPublicExtension = pPublicExtension;
    pKDBInfo->Private.wInfoInitOK = ET9GOODSETUP;
    pKDBInfo->Private.pConvertSymb = NULL;
    pKDBInfo->Private.pConvertSymbInfo = NULL;

    {
        ET9BOOL bDummy;

        _ET9_GetDefaultLocale(&pKDBInfo->Private.wLocale, &bDummy);
    }

    pKDBInfo->Private.pCurrLayoutInfo = &pKDBInfo->Private.pLayoutInfos[0];
    pKDBInfo->Private.pLastLayoutInfo = pKDBInfo->Private.pCurrLayoutInfo;

    if (ET9_GetSymbolEncoding() == ET9SYMBOL_ENCODING_SHIFTJIS) {
        pKDBInfo->Private.pFilterSymb = __DefaultDiacriticFilterShiftJis;
        pKDBInfo->Private.pFilterSymbReset = __DefaultDiacriticFilterResetShiftJis;
        pKDBInfo->Private.pFilterSymbCount = __DefaultDiacriticFilterCountShiftJis;
        pKDBInfo->Private.pFilterSymbNext = __DefaultDiacriticFilterNextShiftJis;
        pKDBInfo->Private.pFilterSymbGroup = __DefaultDiacriticFilterGroupShiftJis;
        pKDBInfo->Private.pFilterSymbInfo = pKDBInfo;
    }
    else {
        pKDBInfo->Private.pFilterSymb = NULL;
        pKDBInfo->Private.pFilterSymbReset = NULL;
        pKDBInfo->Private.pFilterSymbInfo = NULL;
        pKDBInfo->Private.pFilterSymbCount = NULL;
        pKDBInfo->Private.pFilterSymbNext = NULL;
        pKDBInfo->Private.pFilterSymbGroup = NULL;
    }

    /* initialize in lower case ambiguous mode */

    pKDBInfo->dwStateBits = ET9_KDB_AMBIGUOUS_MODE_MASK;

    pKDBInfo->wFirstKdbNum = ET9PLIDNone;
    pKDBInfo->wSecondKdbNum = ET9PLIDNone;
    pKDBInfo->wKdbNum = ET9PLIDNone;

    eStatus = ET9KDB_SetKdbNum(pKDBInfo, wFirstKdbNum, wFirstPageNum, wSecondKdbNum, wSecondPageNum);

    if (eStatus) {
        pKDBInfo->dwStateBits = 0;
        return eStatus;
    }

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/**
 * @brief Activates a keyboard by specifying its keyboard identification number.
 *
 * @param[in]     pKDBInfo          Pointer to the Keyboard Input Module Information Data Structure (ET9KDBInfo).
 * @param[in]     wFirstKdbNum      Identification number of the keyboard to be used by the first language.
 * @param[in]     wFirstPageNum     Initial page number to be loaded for the keyboard used by the first language.
 *                                  Valid values range from 1 to the maximum number of keyboard pages (indicated in the KDB text file) minus 1.
 * @param[in]     wSecondKdbNum     Identification number of the keyboard to be used by the second language.<br>
 *                                  Set this parameter to 0 if you are not implementing the bilingual feature.
 * @param[in]     wSecondPageNum    Initial page number to be loaded for the keyboard used by the second language.
 *                                  Set this parameter to NULL if you are not implementing the bilingual feature.
 *
 * @retval ET9STATUS_NONE           Function call was handled successfully.
 * @retval ET9STATUS_INVALID_MEMORY At least one pointer passed as a function argument was set to null.
 * @retval ET9STATUS_NO_INIT        The Keyboard Input Module has not been initialized. Call ET9KDB_Init().
 * @retval ET9STATUS_ABORT          XT9 has encountered a severe error. Often this status is returned because the integration layer did not allocate sufficient memory for required data structures or because required identification numbers (such as for the LDB and keyboard) are set to 0.
 * @retval ET9STATUS_KDB_VERSION_ERROR  The version information in the keyboard specified by wKDBNum is not valid.
 *                                      Contact your Nuance support engineer for assistance.
 * @retval ET9STATUS_INVALID_DB_TYPE    The keyboard specified by wKDBNum is not valid. Contact your Nuance support engineer for assistance.
 * @retval ET9STATUS_DB_CORE_INCOMP     The keyboard and Keyboard Input Module are not compatible. Contact your Nuance support engineer for assistance.
 * @retval ET9STATUS_INVALID_KDB_PAGE   The page specified by wPageNum is not valid for the keyboard specified by wKDBNum.
 * @retval ET9STATUS_NO_KEY             No keys are associated with the keyboard page specified by wPageNum.
 * @retval ET9STATUS_READ_DB_FAIL   The XT9 core cannot read data from the keyboard. This typically indicates a problem with the ET9KDBREADCALLBACK() function you have implemented.
 *
 * @remarks When calling this function, the integration layer specifies which.
 * keyboard is becoming active and which page of the keyboard is to be set as the current page.<br>
 * To retrieve the identification number of the active keyboard, the integration layer calls
 * ET9KDB_GetKdbNum(). To retrieve the page number of the active keyboard's current page, the
 * integration layer calls ET9KDB_GetPageNum().
 */

ET9STATUS ET9FARCALL ET9KDB_SetKdbNum(ET9KDBInfo    * const pKDBInfo,
                                      const ET9U16          wFirstKdbNum,
                                      const ET9U16          wFirstPageNum,
                                      const ET9U16          wSecondKdbNum,
                                      const ET9U16          wSecondPageNum)
{
    ET9STATUS   eStatus;

    ET9U16       wNumKeysFirstKDB = 0;
    ET9U16       wNumKeysSecondKDB = 0;
    ET9U32       dwFirstChecksum = 0;
    ET9U32       dwSecondChecksum = 0;
    ET9U16       wTotalFirstPages = 0;
    ET9U16       wTotalSecondPages = 0;
    ET9U16       wOrigFirstKdbNum;
    ET9U16       wOrigFirstPageNum;
    ET9U16       wOrigSecondKdbNum;
    ET9U16       wOrigSecondPageNum;

    WLOG5(fprintf(pLogFile5, "ET9KDB_SetKdbNum, pKDBInfo = %p\n", pKDBInfo);)

    eStatus = __ET9KDB_BasicValidityCheck(pKDBInfo, 0);

    if (eStatus) {
        return eStatus;
    }

    /* Need valid kdb to load second page */

    if (wSecondPageNum && ((wSecondKdbNum & ET9PLIDMASK) == ET9PLIDNone)) {
        return ET9STATUS_NEED_KDB_TO_LOAD_PAGE;
    }

    /* clear insert mask */

    pKDBInfo->dwStateBits &= ~ET9_KDB_INSERT_MASK;

    /* remember previous values */

    wOrigFirstKdbNum = pKDBInfo->wFirstKdbNum;
    wOrigFirstPageNum = pKDBInfo->wFirstPageNum;
    wOrigSecondKdbNum = pKDBInfo->wSecondKdbNum;
    wOrigSecondPageNum = pKDBInfo->wSecondPageNum;

    /* reset filtering */

    __FilterSymbReset(pKDBInfo);

    pKDBInfo->Private.sKdbAction.dwCurrChecksum = 0;

    pKDBInfo->wFirstKdbNum = wFirstKdbNum;
    pKDBInfo->wFirstPageNum = wFirstPageNum;
    pKDBInfo->wSecondKdbNum = ((wSecondKdbNum & ET9PLIDMASK) != ET9PLIDNone) ? wSecondKdbNum : 0;
    pKDBInfo->wSecondPageNum = ((wSecondKdbNum & ET9PLIDMASK) != ET9PLIDNone) ? wSecondPageNum : 0;

    if (wSecondKdbNum && ((wSecondKdbNum & ET9PLIDMASK) != ET9PLIDNone)) {

        eStatus = __KDBLoadPage(pKDBInfo, wSecondKdbNum, wSecondPageNum, &wNumKeysSecondKDB);

        if (eStatus) {
            /* restore */
            pKDBInfo->wFirstKdbNum = wOrigFirstKdbNum;
            pKDBInfo->wFirstPageNum = wOrigFirstPageNum;
            pKDBInfo->wSecondKdbNum = wOrigSecondKdbNum;
            pKDBInfo->wSecondPageNum = wOrigSecondPageNum;

            return eStatus;
        }

        wTotalSecondPages = pKDBInfo->wTotalPages;

        dwSecondChecksum = __ComputeContentChecksum(pKDBInfo);
    }

    eStatus = __KDBLoadPage(pKDBInfo, wFirstKdbNum, wFirstPageNum, &wNumKeysFirstKDB);

    if (eStatus) {
        /* restore */
        pKDBInfo->wFirstKdbNum = wOrigFirstKdbNum;
        pKDBInfo->wFirstPageNum = wOrigFirstPageNum;
        pKDBInfo->wSecondKdbNum = wOrigSecondKdbNum;
        pKDBInfo->wSecondPageNum = wOrigSecondPageNum;

        return eStatus;
    }

    wTotalFirstPages = pKDBInfo->wTotalPages;

    dwFirstChecksum = __ComputeContentChecksum(pKDBInfo);

    if ((wFirstKdbNum != wSecondKdbNum) &&
        ((wFirstKdbNum & ET9PLIDMASK)  != ET9PLIDNone) &&
        ((wSecondKdbNum & ET9PLIDMASK) != ET9PLIDNone) &&
        ((wTotalFirstPages != wTotalSecondPages) ||
        (wNumKeysFirstKDB != wNumKeysSecondKDB) ||
        (dwFirstChecksum != dwSecondChecksum))) {

        /* restore */
        pKDBInfo->wFirstKdbNum = wOrigFirstKdbNum;
        pKDBInfo->wFirstPageNum = wOrigFirstPageNum;
        pKDBInfo->wSecondKdbNum = wOrigSecondKdbNum;
        pKDBInfo->wSecondPageNum = wOrigSecondPageNum;

        return ET9STATUS_KDB_MISMATCH;
    }

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/**
 * @brief Retrieves the identification number of the active keyboard.
 *
 * @param[in]     pKDBInfo          Pointer to the Keyboard Input Module Information Data Structure (ET9KDBInfo).
 * @param[out]    pwKdbNum          Pointer to a value indicating the identification number of the first language keyboard.
 * @param[out]    pwSecondKdbNum    Pointer to a value indicating the identification number of the second language keyboard.
 *                                  Set this parameter to 0 if you are not implementing the bilingual feature.
 *
 * @retval ET9STATUS_NONE           Function call was handled successfully.
 * @retval ET9STATUS_INVALID_MEMORY At least one pointer passed as a function argument was set to null.
 * @retval ET9STATUS_NO_INIT        The Keyboard Input Module has not been initialized. Call ET9KDB_Init().
 *
 * @see ET9KDB_SetKdbNum(), ET9KDB_GetPageNum()
 */

ET9STATUS ET9FARCALL ET9KDB_GetKdbNum(ET9KDBInfo * const pKDBInfo,
                                      ET9U16     * const pwKdbNum,
                                      ET9U16     * const pwSecondKdbNum)
{
    ET9STATUS eStatus;

    eStatus = __ET9KDB_BasicValidityCheck(pKDBInfo, 1);

    if (eStatus) {
        return eStatus;
    }

    if (!pwKdbNum) {
        return ET9STATUS_INVALID_MEMORY;
    }

    if (_ET9KDBSecondKDBSupported(pKDBInfo) && (!pwSecondKdbNum)) {
        return ET9STATUS_INVALID_MEMORY;
    }

    *pwKdbNum = pKDBInfo->wFirstKdbNum;

    if (pwSecondKdbNum) {
        *pwSecondKdbNum = pKDBInfo->wSecondKdbNum;
    }

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/**
 * @brief Sets the current page of the active keyboard by specifying its page number.
 *
 * @param[in]     pKDBInfo          Pointer to the Keyboard Input Module Information Data Structure (ET9KDBInfo).
 * @param[in]     wFirstPageNum     Initial page number to be loaded for the keyboard used by the first language.
 *                              Valid values range from 0 to the maximum number of keyboard pages
 *                              (indicated in the KDB text file) minus 1.
 * @param[in]     wSecondPageNum    Initial page number to be loaded for the keyboard used by the second language.<br>
 *                              Set this parameter to NULL if you are not implementing the bilingual feature.
 *
 * @retval ET9STATUS_NONE               Function call was handled successfully.
 * @retval ET9STATUS_INVALID_MEMORY     At least one pointer passed as a function argument was set to null.
 * @retval ET9STATUS_NO_INIT            The Keyboard Input Module has not been initialized. Call ET9KDB_Init().
 * @retval ET9STATUS_INVALID_KDB_PAGE   The page number of the current keyboard page is not valid.
 *                                      If you receive this status, contact your Nuance support engineer for assistance.
 * @retval ET9STATUS_NO_KEY             No keys are associated with the keyboard page specified by wPageNum.
 * @retval ET9STATUS_READ_DB_FAIL       The XT9 core cannot read data from the keyboard. This typically indicates a problem with the ET9KDBREADCALLBACK() function you have implemented.
 *
 * @see ET9KDB_GetPageNum(), ET9KDB_SetKdbNum()
 */

ET9STATUS ET9FARCALL ET9KDB_SetPageNum(ET9KDBInfo * const pKDBInfo,
                                       const ET9U16       wFirstPageNum,
                                       const ET9U16       wSecondPageNum)
{
    ET9STATUS  eStatus;
    ET9U16     wSavedFirstPageNum;
    ET9U16     wSavedSecondPageNum;
    ET9U16     wNumKeysFirstKDB = 0;
    ET9U16     wNumKeysSecondKDB = 0;
    ET9U32     dwFirstChecksum = 0;
    ET9U32     dwSecondChecksum = 0;

    WLOG5(fprintf(pLogFile5, "ET9KDB_SetPageNum, pKDBInfo = %p\n", pKDBInfo);)

    eStatus = __ET9KDB_BasicValidityCheck(pKDBInfo, 1);

    if (eStatus) {
        return eStatus;
    }

    /* Need valid kdb to load second page */

    if (wSecondPageNum && !pKDBInfo->wSecondKdbNum) {
        return ET9STATUS_NEED_KDB_TO_LOAD_PAGE;
    }

    /* clear insert mask */

    pKDBInfo->dwStateBits &= ~ET9_KDB_INSERT_MASK;

    /* reset filtering */

    __FilterSymbReset(pKDBInfo);

    /* */

    wSavedFirstPageNum = pKDBInfo->wFirstPageNum;
    pKDBInfo->wFirstPageNum = wFirstPageNum;

    eStatus = __KDBLoadPage(pKDBInfo, pKDBInfo->wFirstKdbNum, wFirstPageNum, &wNumKeysFirstKDB);

    if (eStatus) {

        /* load was unsuccessful, replace original page (_may_ be good) */

        pKDBInfo->wFirstPageNum = wSavedFirstPageNum;
        __KDBLoadPage(pKDBInfo, pKDBInfo->wFirstKdbNum, wSavedFirstPageNum, NULL);

        return eStatus;
    }

    dwFirstChecksum = __ComputeContentChecksum(pKDBInfo);

    if (_ET9KDBSecondKDBSupported(pKDBInfo)) {

        wSavedSecondPageNum = pKDBInfo->wSecondPageNum;
        pKDBInfo->wSecondPageNum = wSecondPageNum;

        eStatus = __KDBLoadPage(pKDBInfo, pKDBInfo->wSecondKdbNum, wSecondPageNum, &wNumKeysSecondKDB);

        if (eStatus) {

            /* if load was unsuccessful, replace original page (_may_ be good) */

            pKDBInfo->wSecondPageNum = wSavedSecondPageNum;
            __KDBLoadPage(pKDBInfo, pKDBInfo->wSecondKdbNum, wSavedSecondPageNum, NULL);

            return eStatus;
        }

        dwSecondChecksum = __ComputeContentChecksum(pKDBInfo);

        if ((pKDBInfo->wFirstKdbNum != pKDBInfo->wSecondKdbNum) &&
            ((pKDBInfo->wFirstKdbNum & ET9PLIDMASK)  != ET9PLIDNone) &&
            ((pKDBInfo->wSecondKdbNum & ET9PLIDMASK) != ET9PLIDNone) &&
            ((wNumKeysFirstKDB != wNumKeysSecondKDB) ||
            (dwFirstChecksum != dwSecondChecksum))) {

            /* if KDB mismatch, replace original page (_may_ be good) */

            pKDBInfo->wFirstPageNum = wSavedFirstPageNum;
            eStatus = __KDBLoadPage(pKDBInfo, pKDBInfo->wFirstKdbNum, wSavedFirstPageNum, NULL);

            pKDBInfo->wSecondPageNum = wSavedSecondPageNum;
            eStatus = __KDBLoadPage(pKDBInfo, pKDBInfo->wSecondKdbNum, wSavedSecondPageNum, NULL);

            return ET9STATUS_KDB_MISMATCH;
        }
    }

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/**
 * @brief Retrieves the page number of the active keyboard's current page.
 *
 * @param[in]     pKDBInfo          Pointer to the Keyboard Input Module Information Data Structure (ET9KDBInfo).
 * @param[out]    pwFirstPageNum   Pointer to current page number of the first language keyboard.
 *                              Valid values range from 1 to the maximum number of keyboard pages indicated in the KDB text file.
 * @param[out]    pwSecondPageNum  Pointer to the current page number of the second language keyboard.<br>
 *                              Set this parameter to NULL if you are not implementing the bilingual feature.
 *
 * @retval ET9STATUS_NONE               Function call was handled successfully.
 * @retval ET9STATUS_INVALID_MEMORY     At least one pointer passed as a function argument was set to null.
 * @retval ET9STATUS_NO_INIT            The Keyboard Input Module has not been initialized. Call ET9KDB_Init().
 * @retval ET9STATUS_INVALID_KDB_PAGE   The page number of the current keyboard page is not valid.
 *                                      If you receive this status, contact your Nuance support engineer for assistance.
 *
 * @see ET9KDB_GetKdbNum(), ET9KDB_SetPageNum
 */

ET9STATUS ET9FARCALL ET9KDB_GetPageNum(ET9KDBInfo * const pKDBInfo,
                                       ET9U16     * const pwFirstPageNum,
                                       ET9U16     * const pwSecondPageNum)
{
    ET9STATUS   eStatus;

    eStatus = __ET9KDB_BasicValidityCheck(pKDBInfo, 1);

    if (eStatus) {
        return eStatus;
    }

    if (!pwFirstPageNum) {
        return ET9STATUS_INVALID_MEMORY;
    }

    if (_ET9KDBSecondKDBSupported(pKDBInfo) && (!pwSecondPageNum)) {
        return ET9STATUS_INVALID_MEMORY;
    }

    *pwFirstPageNum = 0;

    /* Added sanity check */

    if ((pKDBInfo->wTotalPages <= pKDBInfo->wFirstPageNum) ||
        (pKDBInfo->wTotalPages <= pKDBInfo->wSecondPageNum)) {
        return ET9STATUS_INVALID_KDB_PAGE;
    }

    *pwFirstPageNum = pKDBInfo->wFirstPageNum;

    if (pwSecondPageNum) {
        *pwSecondPageNum = pKDBInfo->wSecondPageNum;
    }

    return eStatus;
}

/*---------------------------------------------------------------------------*/
/**
 * @brief Validates the integrity of a Keyboard Database.
 *
 * @param[in]     pKDBInfo          Pointer to the Keyboard Input Module Information Data Structure (ET9KDBInfo).
 * @param[in]     wKdbNum           Keyboard identification number of the keyboard to be validated.
 * @param[in]     pKDBLoadData      Pointer to the ET9KDBLOADCALLBACK() function. You must implement at least one of the load and read callbacks.
 * @param[in]     pKDBReadData      Pointer to the ET9KDBREADCALLBACK() function. You must implement at least one of the load and read callbacks.
 *
 * @retval ET9STATUS_NONE           Function call was handled successfully.
 * @retval ET9STATUS_INVALID_MEMORY At least one pointer passed as a function argument was set to null.
 * @retval ET9STATUS_NO_INIT        The Keyboard Input Module has not been initialized. Call ET9KDB_Init().
 * @retval ET9STATUS_CORRUPT_DB     The keyboard is corrupted. Contact your Nuance support engineer for assistance.
 * @retval ET9STATUS_READ_DB_FAIL   The XT9 core cannot read data from the keyboard. This typically indicates a problem with the ET9KDBREADCALLBACK() function you have implemented.
 *
 * @remarks XT9 generates a checksum for the database specified by the value of wKDBNum and compares the checksum to a previously stored value.<br>
 * The integration layer must initialize the Keyboard Input Module (by calling ET9KDB_Init) before calling ET9KDB_Validate.
 *
 */

ET9STATUS ET9FARCALL ET9KDB_Validate(ET9KDBInfo         * const pKDBInfo,
                                     const ET9U16               wKdbNum,
                                     const ET9KDBLOADCALLBACK   pKDBLoadData,
                                     const ET9KDBREADCALLBACK   pKDBReadData)
{
    if ((!pKDBLoadData && !pKDBReadData) || !pKDBInfo) {
        return ET9STATUS_INVALID_MEMORY;
    }

    WLOG5(fprintf(pLogFile5, "ET9KDB_Validate, wKdbNum = %4x\n", wKdbNum);)

    {
        ET9STATUS                   eStatus;
        const ET9BOOL               bOldKDBLoaded = pKDBInfo->Private.bKDBLoaded;
        const ET9U16                wOldKdbNum = pKDBInfo->wKdbNum;
        const ET9U16                wOldPageNum = pKDBInfo->Private.wPageNum;
        const ET9KDBLOADCALLBACK    wOldKDBLoadCallback = pKDBInfo->pKDBLoadData;
        const ET9KDBREADCALLBACK    wOldKDBReadCallback = pKDBInfo->ET9KDBReadData;

        pKDBInfo->wKdbNum = wKdbNum;
        pKDBInfo->pKDBLoadData = pKDBLoadData;
        pKDBInfo->ET9KDBReadData = pKDBReadData;
        pKDBInfo->Private.bKDBLoaded = 0;

        /* validate */

        eStatus = __KDBValidate_Dynamic(pKDBInfo);

        if (eStatus == ET9STATUS_INVALID_KDB_NUM) {
            eStatus = __KDBValidate_Static(pKDBInfo);
        }

        /* restore previous kdb */

        pKDBInfo->wKdbNum = 0;
        pKDBInfo->pKDBLoadData = wOldKDBLoadCallback;
        pKDBInfo->ET9KDBReadData = wOldKDBReadCallback;

        if (bOldKDBLoaded) {

            WLOG5(fprintf(pLogFile5, "ET9KDB_Validate, restoring = %4x\n", wOldKdbNum);)

            __KDBLoadPage(pKDBInfo, wOldKdbNum, wOldPageNum, NULL);
        }

        /* done*/

        return eStatus;
    }
}

/*---------------------------------------------------------------------------*/
/**
 * @brief Retrieves version information for the active keyboard.
 *
 * @param[in]     pKDBInfo      Pointer to the Keyboard Input Module Information Data Structure (ET9KDBInfo).
 * @param[out]    psKDBVerBuf   Pointer to a buffer to which the Keyboard Input Module will write the version information. The buffer should be at least ET9MAXVERSIONSTR characters in length.
 * @param[in]     wBufMaxSize   Length in characters of the buffer pointed to by psKDBVerBuf.
 * @param[out]    pwBufSize     Pointer to a value indicating the length in characters of the version information written to psKDBVerBuf.
 *
 * @retval ET9STATUS_NONE               Function call was handled successfully.
 * @retval ET9STATUS_INVALID_MEMORY     At least one pointer passed as a function argument was set to null.
 * @retval ET9STATUS_NO_INIT            The Keyboard Input Module has not been initialized. Call ET9KDB_Init().
 * @retval ET9STATUS_NO_MEMORY          The buffer pointed to by psKDBVerBuf is too small to store the version information.
 * @retval ET9STATUS_READ_DB_FAIL       The XT9 core cannot read data from the keyboard. This typically indicates a problem with the ET9KDBREADCALLBACK() function you have implemented.
 *
 * @remarks The Keyboard Input Module returns this information as a non-null-terminated string. You can use pwBufSize to get the length of the string.
 * The version information is formatted as <tt>ET9 KDB Taa.bb Lcc.dd.ee Vff.gg</tt>, where:<br>
 * \c aa = Database type.<br>
 * \c bb = Database layout version number.<br>
 * \c cc = Primary keyboard ID.<br>
 * \c dd = Secondary keyboard ID.<br>
 * \c ee = Symbol class.<br>
 * \c ff = Major version information.<br>
 * \c gg = Minor version information.<br>
 * Each letter represents one hexadecimal character. Following is an example of the version information for a keyboard:<br>
 * <tt>ET9 KDB T03.03 LF0.01.0F V02.00</tt>.
 */

ET9STATUS ET9FARCALL ET9KDB_GetKdbVersion(ET9KDBInfo    * const pKDBInfo,
                                          ET9SYMB       * const psKDBVerBuf,
                                          const ET9U16          wBufMaxSize,
                                          ET9U16        * const pwBufSize)
{
    ET9STATUS eStatus;

    WLOG5(fprintf(pLogFile5, "ET9KDB_GetKdbVersion, pKDBInfo = %p\n", pKDBInfo);)

    if (pwBufSize) {
        *pwBufSize = 0;
    }

    eStatus = __ET9KDB_BasicValidityCheck(pKDBInfo, 1);

    if (eStatus) {
        return eStatus;
    }

    if (!psKDBVerBuf || !pwBufSize) {
        return ET9STATUS_INVALID_MEMORY;
    }

    if (wBufMaxSize < ET9MAXVERSIONSTR) {
        return ET9STATUS_NO_MEMORY;
    }

    if (pKDBInfo->Private.bUsingDynamicKDB) {
        eStatus = __GetKdbVersion_Generic(pKDBInfo, psKDBVerBuf, pwBufSize);
    }
    else {
        eStatus = __GetKdbVersion_Static(pKDBInfo, psKDBVerBuf, pwBufSize);
    }

    return eStatus;
}

/*---------------------------------------------------------------------------*/
/**
 * @brief Informs the Keyboard Input Module that the user has entered a symbol.
 *
 * @param[in]     pKDBInfo          Pointer to the Keyboard Input Module Information Data Structure (ET9KDBInfo).
 * @param[in,out] pWordSymbInfo     Pointer to the Word Symbol Data Structure (ET9WordSymbInfo).
 *                                  The Keyboard Input Module writes to this structure information about the input
 *                                  the user has provided.
 * @param[in]     sSymbol           Symbol to be searched for by the Keyboard Input Module.
 * @param[in]     bCurrIndexInList  0-based index value that indicates which word in the selection list is currently selected. XT9 locks this word before it adds the specified input value.<br>
 *                                  If there is no current input sequence, set the value of this parameter to
 *                                  ET9_NO_ACTIVE_INDEX.<br>
 *                                  If the integration layer passes as an invalid argument for this parameter, XT9 will
 *                                  still add the new input, but it will not lock the word. Also, it will not return an error status.
 * @param[out]    psFunctionKey     Pointer to a buffer where the Keyboard Input Module stores information about the tapped/pressed key if it is a function key requiring action by the integration layer.
 *                                  (for example, changing the shift state or input mode).<br>
 *                                  If the key that was pressed or tapped is not a function key, XT9 sets the value to 0.
 * @param[out]    bInitialSymCheck  Specifes whether matching is performed against the first symbol assigned to each key or ALL symbols assigned to each key. Valid values are:<br>
 *                                  <ul><li>TRUE - Initial symbol
 *                                      <li>FALSE - All symbols</ul>
 *
 * @retval ET9STATUS_NONE               Function call was handled successfully.
 * @retval ET9STATUS_INVALID_MEMORY     At least one pointer passed as a function argument was set to null.
 * @retval ET9STATUS_NO_INIT            The Keyboard Input Module has not been initialized. Call ET9KDB_Init().
 *
 * @remarks Before calling this function, the integration layer must have previously initialized the Keyboard Input Module by calling ET9KDB_Init().
 * When the integration layer calls ET9KDB_ProcessKeyBySymbol, the Keyboard Input Module stores information about the key that was pressed/tapped in the
 * instance of ET9WordSymbInfo pointed to by pWordSymbInfo.
 *
 */

ET9STATUS ET9FARCALL ET9KDB_ProcessKeyBySymbol(ET9KDBInfo      * const pKDBInfo,
                                               ET9WordSymbInfo * const pWordSymbInfo,
                                               const ET9SYMB           sSymbol,
                                               const ET9U8             bCurrIndexInList,
                                               ET9SYMB         * const psFunctionKey,
                                               const ET9BOOL           bInitialSymCheck)
{
    ET9STATUS   eStatus;
    ET9U8       byRegionalKey;
    ET9U16      wKeyIndex;
    ET9Region   KeyRegion;

    eStatus = __ET9KDB_BasicValidityCheck(pKDBInfo, 1);

    if (eStatus) {
        return eStatus;
    }

    if (!pWordSymbInfo || !psFunctionKey || pWordSymbInfo->wInitOK != ET9GOODSETUP) {
        return ET9STATUS_INVALID_MEMORY;
    }

    /* first find the right key index */

    {
        ET9SYMB sLower = _ET9SymToLower(sSymbol, pWordSymbInfo->Private.wLocale);

        eStatus = _ET9KDB_FindSymbol(pKDBInfo,
                                     sLower,
                                     pKDBInfo->wFirstKdbNum,
                                     pKDBInfo->wFirstPageNum,
                                     &byRegionalKey,
                                     &wKeyIndex,
                                     &KeyRegion,
                                     bInitialSymCheck);

        if (eStatus) {

            ET9SYMB sUpper = _ET9SymToUpper(sSymbol, pWordSymbInfo->Private.wLocale);

            if (sUpper != sLower) {

                eStatus = _ET9KDB_FindSymbol(pKDBInfo,
                                             sUpper,
                                             pKDBInfo->wFirstKdbNum,
                                             pKDBInfo->wFirstPageNum,
                                             &byRegionalKey,
                                             &wKeyIndex,
                                             &KeyRegion,
                                             bInitialSymCheck);
            }
        }

        if (eStatus && _ET9KDBSecondKDBSupported(pKDBInfo)) {

            eStatus = _ET9KDB_FindSymbol(pKDBInfo,
                                         sLower,
                                         pKDBInfo->wSecondKdbNum,
                                         pKDBInfo->wSecondPageNum,
                                         &byRegionalKey,
                                         &wKeyIndex,
                                         &KeyRegion,
                                         bInitialSymCheck);

            if (eStatus) {

                ET9SYMB sUpper = _ET9SymToUpper(sSymbol, pWordSymbInfo->Private.wLocale);

                if (sUpper != sLower) {

                    eStatus = _ET9KDB_FindSymbol(pKDBInfo,
                                                 sUpper,
                                                 pKDBInfo->wSecondKdbNum,
                                                 pKDBInfo->wSecondPageNum,
                                                 &byRegionalKey,
                                                 &wKeyIndex,
                                                 &KeyRegion,
                                                 bInitialSymCheck);
                }
            }
        }

    }

    if (eStatus) {
        return eStatus;
    }

    /* now do process key */

    return ET9KDB_ProcessKey(pKDBInfo, pWordSymbInfo, wKeyIndex, bCurrIndexInList, psFunctionKey);
}

/*---------------------------------------------------------------------------*/
/**
 * @brief Informs the Keyboard Input Module that the user has entered key-based input.
 *
 * @param[in]     pKDBInfo          Pointer to the Keyboard Input Module Information Data Structure (ET9KDBInfo).
 * @param[in,out] pWordSymbInfo     Pointer to the Word Symbol Data Structure (ET9WordSymbInfo).
 *                                  The Keyboard Input Module writes to this structure information about the input
 *                                  the user has provided.
 * @param[in]     wKeyIndex         Index value of the key that was tapped or pressed. Key index values are specified in the key-mapping file you received when you ordered a keyboard.
 * @param[in]     bCurrIndexInList  0-based index value that indicates which word in the selection list is currently selected. XT9 locks this word before it adds the specified input value.<br>
 *                                  If there is no current input sequence, set the value of this parameter to
 *                                  ET9_NO_ACTIVE_INDEX.<br>
 *                                  If the integration layer passes as an invalid argument for this parameter, XT9 will
 *                                  still add the new input, but it will not lock the word. Also, it will not return an error status.
 * @param[out]    psFunctionKey     Pointer to a buffer where the Keyboard Input Module stores information about the tapped/pressed key if it is a function key requiring action by the integration layer (for example, changing the shift state or input mode).<br>
 *                                  Function key values are part of the ET9KDBKEYDEF enumeration.<br>
 *                                  If the key that was pressed or tapped is not a function key, XT9 sets the value to 0.
 *
 * @retval ET9STATUS_NONE               Function call was handled successfully.
 * @retval ET9STATUS_INVALID_MEMORY     At least one pointer passed as a function argument was set to null.
 * @retval ET9STATUS_NO_INIT            The Keyboard Input Module has not been initialized. Call ET9KDB_Init().
 * @retval ET9STATUS_ERROR              General error status.
 * @retval ET9STATUS_OUT_OF_RANGE       The value specified by wKeyIndex is not valid.
 * @retval ET9STATUS_FULL               The active word is already the maximum size allowed (ET9MAXWORDSIZE).
 * @retval ET9STATUS_READ_DB_FAIL       The XT9 core cannot read data from the keyboard. This typically indicates a problem with the ET9KDBREADCALLBACK() function you have implemented.
 *
 * @remarks This function applies to either ambiguous or multitap input. Before calling this function, the integration.
 * layer must have previously initialized the Keyboard Input Module by calling ET9KDB_Init(). When the integration layer
 * calls ET9KDB_ProcessKey, the Keyboard Input Module stores information about the key that was pressed/tapped in the
 * instance of ET9WordSymbInfo pointed to by pWordSymbInfo.
 *
 */

ET9STATUS ET9FARCALL ET9KDB_ProcessKey(ET9KDBInfo      * const pKDBInfo,
                                       ET9WordSymbInfo * const pWordSymbInfo,
                                       const ET9U16            wKeyIndex,
                                       const ET9U8             bCurrIndexInList,
                                       ET9SYMB         * const psFunctionKey)
{
    ET9STATUS    eStatus;

    WLOG5(fprintf(pLogFile5, "\nET9KDB_ProcessKey, pKDBInfo = %p\n", pKDBInfo);)

    eStatus = __ET9KDB_BasicValidityCheck(pKDBInfo, 1);

    if (eStatus) {
        return eStatus;
    }

    if (!pWordSymbInfo || !psFunctionKey || pWordSymbInfo->wInitOK != ET9GOODSETUP) {
        return ET9STATUS_INVALID_MEMORY;
    }

    __FilterSymbReset(pKDBInfo);

    pKDBInfo->Private.sKdbAction.bIsKeyAction = 1;
    pKDBInfo->Private.sKdbAction.bCurrIndexInList = bCurrIndexInList;
    pKDBInfo->Private.sKdbAction.bShiftState = ET9SHIFT_MODE(pWordSymbInfo->dwStateBits) ? 1 : 0;
    pKDBInfo->Private.sKdbAction.u.keyAction.wKeyIndex = wKeyIndex;

    *psFunctionKey = 0;

    /* modes */

    if (ET9_KDB_AMBIGUOUS_MODE(pKDBInfo->dwStateBits)) {

        /* state changes */

        pKDBInfo->dwStateBits &= ~ET9_KDB_INSERT_MASK;

        /* process KDBs */

        eStatus = __ProcessAmbigKey_ActiveKdbs(pKDBInfo, wKeyIndex, pWordSymbInfo, psFunctionKey, bCurrIndexInList, LOADSYMBACTION_NEW);
    }
    else if (ET9_KDB_MULTITAP_MODE(pKDBInfo->dwStateBits)) {

        /* if insert mask is on and this is a new key, unshift. */

        if (ET9_KDB_INSERT_MODE(pKDBInfo->dwStateBits) && wKeyIndex != pKDBInfo->Private.wMTLastInput) {
            pWordSymbInfo->dwStateBits &= ~ET9STATE_SHIFT_MASK;
            pKDBInfo->Private.sKdbAction.bShiftState = 0;
        }

        /* process KDBs */

        eStatus = __ProcessMultitapKey_ActiveKdbs(pKDBInfo, wKeyIndex, pWordSymbInfo, psFunctionKey, bCurrIndexInList, LOADSYMBACTION_NEW);
    }
    else {
        return ET9STATUS_ERROR;
    }

    /* error above? */

    if (eStatus) {
        return eStatus;
    }

    if (!*psFunctionKey) {
        _ET9TrackInputEvents(pWordSymbInfo, ET9InputEvent_add);
    }

    /* shut off shift bit */

    if (!ET9_KDB_INSERT_MODE(pKDBInfo->dwStateBits) && !*psFunctionKey) {
        pWordSymbInfo->dwStateBits &= ~ET9STATE_SHIFT_MASK;
    }

    /* magic string? */

    if (pWordSymbInfo->bNumSymbs == ET9MAXLDBWORDSIZE) {

        if (_ET9IsMagicStringKey(pWordSymbInfo)) {

            eStatus = ET9KDB_GetKdbVersion(pKDBInfo,
                                           pWordSymbInfo->Private.szIDBVersion,
                                           ET9MAXVERSIONSTR,
                                           &pWordSymbInfo->Private.wIDBVersionStrSize);

            if (eStatus) {
                return eStatus;
            }
        }
    }

    /* mark valid if not a function key (assign checksum) */

    if (!*psFunctionKey && pKDBInfo->Private.pFilterSymb) {
        pKDBInfo->Private.sKdbAction.dwCurrChecksum = __CalculateLastWordSymbChecksum(pWordSymbInfo);
    }

    /* turn off correction inhibit */

    if (!*psFunctionKey) {
        pWordSymbInfo->Private.bRequiredInhibitCorrection = 0;  /* not the override */
    }

    /* done */

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/**
 * @brief Informs the Keyboard Input Module that the user has entered tap-based input.
 *
 * @param[in]     pKDBInfo          Pointer to the Keyboard Input Module Information Data Structure (ET9KDBInfo).
 * @param[in,out] pWordSymbInfo     Pointer to the Word Symbol Data Structure (ET9WordSymbInfo). The Keyboard Input Module writes to this structure information about the input the user has provided.
 * @param[in]     wX                Horizontal (x) coordinate of the tap.
 * @param[in]     wY                Vertical (y) coordinate of the tap.
 * @param[in]     bCurrIndexInList  0-based index value that indicates which word in the selection list is currently selected. XT9 locks this word before it adds the specified input value.<br>
 *                                  If there is no current input sequence, set the value of this parameter to
 *                                  ET9_NO_ACTIVE_INDEX.<br>
 *                                  If the integration layer passes as an invalid argument for this parameter, XT9 will
 *                                  still add the new input, but it will not lock the word. Also, it will not return an error status.
 * @param[out]    psFunctionKey     Pointer to a buffer where the Keyboard Input Module stores information about the tapped/pressed key if it is a function key requiring action by the integration layer (for example, changing the shift state or input mode).<br>
 *                                  Function key values are part of the ET9KDBKEYDEF enumeration.<br>
 *                                  If the key that was pressed or tapped is not a function key, XT9 sets the value to 0.
 *
 * @retval ET9STATUS_NONE               Function call was handled successfully.
 * @retval ET9STATUS_INVALID_MEMORY     At least one pointer passed as a function argument was set to null.
 * @retval ET9STATUS_NO_INIT            The Keyboard Input Module has not been initialized. Call ET9KDB_Init().
 * @retval ET9STATUS_ERROR              General error status.
 * @retval ET9STATUS_KDB_OUT_OF_RANGE   The location of the tap (specified by the values of wX and wY) are outside the valid range for the active keyboard's current page.
 * @retval ET9STATUS_FULL               The active word is already the maximum size allowed (ET9MAXWORDSIZE).
 * @retval ET9STATUS_READ_DB_FAIL       The XT9 core cannot read data from the keyboard. This typically indicates a problem with the ET9KDBREADCALLBACK() function you have implemented.
 * @retval ET9STATUS_NO_KEY             No keys are associated with the current page of the active keyboard.
 *
 * @remarks This function applies to either ambiguous or multitap input. Before calling this function, the integration layer must have previously initialized the Keyboard Input Module by calling ET9KDB_Init().
 * When the integration layer calls ET9KDB_ProcessTap, the Keyboard Input Module stores information about the key that was pressed/tapped in the
 * instance of ET9WordSymbInfo pointed to by pWordSymbInfo.
 *
 */

ET9STATUS ET9FARCALL ET9KDB_ProcessTap(ET9KDBInfo      * const pKDBInfo,
                                       ET9WordSymbInfo * const pWordSymbInfo,
                                       const ET9U16            wX,
                                       const ET9U16            wY,
                                       const ET9U8             bCurrIndexInList,
                                       ET9SYMB         * const psFunctionKey)
{
    ET9STATUS    eStatus;

    WLOG5(fprintf(pLogFile5, "\nET9KDB_ProcessTap, pKDBInfo = %p\n", pKDBInfo);)

    eStatus = __ET9KDB_BasicValidityCheck(pKDBInfo, 1);

    if (eStatus) {
        return eStatus;
    }

    if (!pWordSymbInfo || !psFunctionKey  || pWordSymbInfo->wInitOK != ET9GOODSETUP) {
        return ET9STATUS_INVALID_MEMORY;
    }

    {
        ET9DirectedPos sDirectedPos;

        __InitDirectedPos(&sDirectedPos);

        sDirectedPos.sPos.nX = (ET9UINT)__ScaleCoordinateToKdbX(pKDBInfo, wX);
        sDirectedPos.sPos.nY = (ET9UINT)__ScaleCoordinateToKdbY(pKDBInfo, wY);

        eStatus = __ProcessTap(pKDBInfo, pWordSymbInfo, &sDirectedPos, bCurrIndexInList, psFunctionKey);
    }

    if (eStatus) {
        return eStatus;
    }

    if (!*psFunctionKey) {
        _ET9TrackInputEvents(pWordSymbInfo, ET9InputEvent_add);
    }

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * This function retrieves ambig/non-ambig region info for symbol.
 *
 * @param pKDBInfo             Pointer to keyboard information structure.
 * @param sSymbol              Symbol to look up.
 * @param wKdbNum              Keyboard number.
 * @param wPageNum             Page number.
 * @param pbyRegionalKey       Pointer to indicate if the found key is regional.
 * @param pwKeyIndex           Pointer to store key index in.
 * @param pKeyRegion           Pointer to store region info in.
 * @param bInitialSymCheck     Set if only matching on first sym assigned to key; false if checking ALL key syms.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL _ET9KDB_FindSymbol(ET9KDBInfo          * const pKDBInfo,
                                        const ET9SYMB               sSymbol,
                                        const ET9U16                wKdbNum,
                                        const ET9U16                wPageNum,
                                        ET9U8               * const pbyRegionalKey,
                                        ET9U16              * const pwKeyIndex,
                                        ET9Region           * const pKeyRegion,
                                        const ET9BOOL               bInitialSymCheck)
{
    ET9STATUS eStatus;

    WLOG5(fprintf(pLogFile5, "_ET9KDB_FindSymbol, pKDBInfo = %p\n", pKDBInfo);)

    eStatus = __ET9KDB_BasicValidityCheck(pKDBInfo, 1);

    if (eStatus) {
        return eStatus;
    }

    if (!pbyRegionalKey || ! pwKeyIndex || !pKeyRegion) {
        return ET9STATUS_INVALID_MEMORY;
    }

    eStatus = __KDBLoadPage(pKDBInfo, wKdbNum, wPageNum, NULL);

    if (eStatus) {
        return eStatus;
    }

    *pwKeyIndex = ET9UNDEFINEDKEYVALUE;

    if (pKDBInfo->Private.bUsingDynamicKDB) {
        eStatus = __FindSymbol_Generic(pKDBInfo, sSymbol, pbyRegionalKey, pwKeyIndex, pKeyRegion, bInitialSymCheck);
    }
    else {
        eStatus = __FindSymbol_Static(pKDBInfo, sSymbol, pbyRegionalKey, pwKeyIndex, pKeyRegion, bInitialSymCheck);
    }

    if (*pwKeyIndex == ET9UNDEFINEDKEYVALUE) {
        return ET9STATUS_NO_KEY;
    }

    if (eStatus) {
        return eStatus;
    }

    /* scale to integration space */

    pKeyRegion->wLeft =   (ET9U16)__ScaleCoordinateToIntegrationX(pKDBInfo, pKeyRegion->wLeft);
    pKeyRegion->wRight =  (ET9U16)__ScaleCoordinateToIntegrationX(pKDBInfo, pKeyRegion->wRight);
    pKeyRegion->wTop =    (ET9U16)__ScaleCoordinateToIntegrationY(pKDBInfo, pKeyRegion->wTop);
    pKeyRegion->wBottom = (ET9U16)__ScaleCoordinateToIntegrationY(pKDBInfo, pKeyRegion->wBottom);

    /* done */

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/**
 * @brief Indicates the expiration of a timer requested by the Keyboard Input Module.
 *
 * @param[in]     pKDBInfo          Pointer to the Keyboard Input Module Information Data Structure (ET9KDBInfo).
 * @param[in]     pWordSymbInfo     Pointer to the Word Symbol Data Structure (ET9WordSymbInfo), which contains information about the input the user has provided.
 *
 * @retval ET9STATUS_NONE           Function call was handled successfully.
 * @retval ET9STATUS_INVALID_MEMORY At least one pointer passed as a function argument was set to null.
 * @retval ET9STATUS_NO_INIT        The Keyboard Input Module has not been initialized. Call ET9KDB_Init().
 *
 * @remarks The timer is used when the Keyboard Input Module is in Multitap mode and a multitap sequence is active. When the integration layer calls ET9KDB_TimeOut,
 * XT9 assumes the current character is the desired one, and the multitap sequence becomes inactive.
 *
 * @note If the integration layer has not been rebuilding the selection list with each key press or tap during the active multitap sequence, it should do so once it reports the expiration of a timer.
 */

ET9STATUS ET9FARCALL ET9KDB_TimeOut(ET9KDBInfo          * const pKDBInfo,
                                    ET9WordSymbInfo     * const pWordSymbInfo)
{
    ET9STATUS eStatus;

    eStatus = __ET9KDB_BasicValidityCheck(pKDBInfo, 1);

    if (eStatus) {
        return eStatus;
    }

    if (!pWordSymbInfo || pWordSymbInfo->wInitOK != ET9GOODSETUP) {
        return ET9STATUS_INVALID_MEMORY;
    }

    pWordSymbInfo->dwStateBits &= ~ET9STATE_SHIFT_MASK;
    pKDBInfo->dwStateBits &= ~ET9_KDB_INSERT_MASK;

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/**
 * @brief Sets the Keyboard Input Module to Ambiguous mode.
 *
 * @param[in]     pKDBInfo          Pointer to the Keyboard Input Module Information Data Structure (ET9KDBInfo).
 * @param[in]     wFirstPageNum     Current page number of the first language keyboard. Valid values range from 0 to the maximum number of keyboard pages (indicated in the KDB text file) minus 1.<br>
 *                                  Because ambiguous text entry and multitap text entry typically require different
 *                                  keyboards, XT9 allows the integration layer to set a new keyboard page when setting
 *                                  the Keyboard Input Module to Ambiguous mode.
 * @param[in]     wSecondPageNum    Current page number of the second language keyboard. Set this parameter to NULL if you are not implementing the bilingual feature.<br>
 *                                  Because ambiguous text entry and multitap text entry typically require different
 *                                  keyboards, XT9 allows the integration layer to set a new keyboard page when setting
 *                                  the Keyboard Input Module to Ambiguous mode.
 * @param[in,out] pWordSymbInfo     Pointer to the Word Symbol Data Structure (ET9WordSymbInfo). The Keyboard Input Module writes to this structure information about the input the user has provided.
 *                                  Calling ET9KDB_SetAmbigMode may result in changes to the data in this structure (see the description below for additional information).
 *
 * @retval ET9STATUS_NONE               Function call was handled successfully.
 * @retval ET9STATUS_INVALID_MEMORY     At least one pointer passed as a function argument was set to null.
 * @retval ET9STATUS_NO_INIT            The Keyboard Input Module has not been initialized. Call ET9KDB_Init().
 * @retval ET9STATUS_INVALID_KDB        The page specified by wPageNum is not valid for the active keyboard.
 * @retval ET9STATUS_READ_DB_FAIL       The XT9 core cannot read data from the keyboard. This typically indicates a problem with the ET9KDBREADCALLBACK() function you have implemented.
 * @retval ET9STATUS_NO_KEY             No keys are associated with the keyboard page specified by wPageNum.
 *
 * @remarks This text-entry mode is designed to support keypads in which a single key represents multiple characters.
 * In this mode, the user presses a key once for each character, and XT9 determines which character is most likely desired.<br>
 * To set the Keyboard Input Module to Multitap mode, in which the user repeatedly presses a key to cycle to the desired
 * character, the integration layer calls ET9KDB_SetMultiTapMode().<br>
 * When the integration layer switches text-entry modes (by calling either ET9KDB_SetMultiTapMode() or ET9KDB_SetAmbigMode),
 * the Keyboard Input Module checks whether there is an active word. If there is, the module locks the default active
 * word before switching modes. This ensures the active word is not inadvertently changed by a mode change.<br>
 * By default, when the integration layer initializes the Keyboard Input Module, it is set to Ambiguous mode. If you do
 * not use multitap text entry in your XT9 implementation, it will not be necessary to call this function, as the module
 * will always be in Ambiguous mode.
 */

ET9STATUS ET9FARCALL ET9KDB_SetAmbigMode(ET9KDBInfo      * const pKDBInfo,
                                         const ET9U16            wFirstPageNum,
                                         const ET9U16            wSecondPageNum,
                                         ET9WordSymbInfo * const pWordSymbInfo)
{
    ET9STATUS eStatus;

    ET9_UNUSED(pWordSymbInfo);

    eStatus = __ET9KDB_BasicValidityCheck(pKDBInfo, 1);

    if (eStatus) {
        return eStatus;
    }

    eStatus = ET9KDB_SetPageNum(pKDBInfo, wFirstPageNum, wSecondPageNum);

    if (eStatus) {
        return eStatus;
    }

    pKDBInfo->dwStateBits &= ~(ET9_KDB_MULTITAP_MODE_MASK);
    pKDBInfo->dwStateBits |= ET9_KDB_AMBIGUOUS_MODE_MASK;

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/**
 * @brief Sets the Keyboard Input Module to Multitap mode.
 *
 * @param[in]     pKDBInfo          Pointer to the Keyboard Input Module Information Data Structure (ET9KDBInfo).
 * @param[in]     wFirstPageNum     Current page number of the first language keyboard. Valid values range from 0 to the maximum number of keyboard pages (indicated in the KDB text file).<br>
 *                                  Because ambiguous text entry and multitap text entry typically require different
 *                                  keyboards, XT9 allows the integration layer to set a new keyboard page when setting
 *                                  the Keyboard Input Module to Multitap mode.<br>
 *                                  To keep the current keyboard page, set this value to ET9KDBInfo.wPageNum.
 * @param[in]     wSecondPageNum    Current page number of the second language keyboard. Set this parameter to NULL if you are not implementing the bilingual feature.<br>
 *                                  Because ambiguous text entry and multitap text entry typically require different
 *                                  keyboards, XT9 allows the integration layer to set a new keyboard page when setting
 *                                  the Keyboard Input Module to Multitap mode.<br>
 *                                  To keep the current keyboard page, set this value to ET9KDBInfo.wPageNum.
 * @param[in,out] pWordSymbInfo     Pointer to the Word Symbol Data Structure (ET9WordSymbInfo). The Keyboard Input Module writes to this structure information about the input the user has provided.
 *                                  Calling ET9KDB_SetMultiTapMode may result in changes to the data in this structure
 *                                  (see the description below for additional information)
 *
 * @retval ET9STATUS_NONE               Function call was handled successfully.
 * @retval ET9STATUS_INVALID_MEMORY     At least one pointer passed as a function argument was set to null.
 * @retval ET9STATUS_NO_INIT            The Keyboard Input Module has not been initialized. Call ET9KDB_Init().
 * @retval ET9STATUS_INVALID_KDB_PAGE   The page specified by wPageNum is not valid for the keyboard specified by wPageNum.
 * @retval ET9STATUS_READ_DB_FAIL       The XT9 core cannot read data from the keyboard. This typically indicates a problem with the ET9KDBREADCALLBACK() function you have implemented.
 * @retval ET9STATUS_NO_KEY             No keys are associated with the keyboard page specified by wPageNum.
 *
 * @see ET9KDB_SetDiscreteMode(), ET9KDB_SetRegionalMode()
 *
 * @remarks This text-entry mode is designed to support keypads in which a single key represents multiple characters.
 * In this mode, the user repeatedly presses a key to cycle around to the desired character.<br>
 * To set the Keyboard Input Module to Ambiguous mode, in which the user presses a key once for each character, and XT9
 * determines which character is most likely desired, the integration layer calls ET9KDB_SetAmbigMode.<br>
 * When the integration layer switches text-entry modes (by calling either ET9KDB_SetAmbigMode() or ET9KDB_SetMultiTapMode),
 * the Keyboard Input Module checks whether there is an active word. If there is, the module locks the default active
 * word before switching modes. This ensures the active word is not inadvertently changed by a mode change.<br>
 * By default, when the integration layer initializes the Keyboard Input Module, it is set to Ambiguous mode.
 */

ET9STATUS ET9FARCALL ET9KDB_SetMultiTapMode(ET9KDBInfo      * const pKDBInfo,
                                            const ET9U16            wFirstPageNum,
                                            const ET9U16            wSecondPageNum,
                                            ET9WordSymbInfo * const pWordSymbInfo)
{
    ET9STATUS eStatus;

    ET9_UNUSED(pWordSymbInfo);

    eStatus = __ET9KDB_BasicValidityCheck(pKDBInfo, 1);

    if (eStatus) {
        return eStatus;
    }

    eStatus = ET9KDB_SetPageNum(pKDBInfo, wFirstPageNum, wSecondPageNum);

    if (eStatus) {
        return eStatus;
    }

    pKDBInfo->dwStateBits &= ~(ET9_KDB_AMBIGUOUS_MODE_MASK);
    pKDBInfo->dwStateBits |= ET9_KDB_MULTITAP_MODE_MASK;

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Set the current locale.
 *
 * @param pKDBInfo                  Pointer to keyboard information structure.
 * @param pWordSymbInfo             Pointer to word symbol info structure.
 * @param wLocale                   Locale to set.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL ET9KDB_SetLocale(ET9KDBInfo        * const pKDBInfo,
                                      ET9WordSymbInfo   * const pWordSymbInfo,
                                      const ET9U16              wLocale)
{
    ET9STATUS eStatus;

    eStatus = __ET9KDB_BasicValidityCheck(pKDBInfo, 0);

    if (eStatus) {
        return eStatus;
    }

    if (!pWordSymbInfo || pWordSymbInfo->wInitOK != ET9GOODSETUP) {
        return ET9STATUS_INVALID_MEMORY;
    }

    if (pWordSymbInfo->Private.bManualLocale && pKDBInfo->Private.wLocale == wLocale) {
        return ET9STATUS_NONE;
    }

    /* set info */

    pKDBInfo->Private.wLocale = wLocale;
    pWordSymbInfo->Private.wLocale = wLocale;
    pWordSymbInfo->Private.bManualLocale = 1;

    /* invalidate loaded KDBs */

    {
        ET9UINT nCount;
        ET9KdbLayoutInfo *pLayoutInfo;

        pLayoutInfo = &pKDBInfo->Private.pLayoutInfos[0];
        for (nCount = ET9_KDB_MAX_PAGE_CACHE; nCount; --nCount, ++pLayoutInfo) {
            pLayoutInfo->bOk = 0;
        }

        pKDBInfo->Private.bKDBLoaded = 0;

        eStatus = ET9KDB_SetKdbNum(pKDBInfo, pKDBInfo->wFirstKdbNum, pKDBInfo->wFirstPageNum, pKDBInfo->wSecondKdbNum, pKDBInfo->wSecondPageNum);

        if (eStatus) {
            return eStatus;
        }
    }

    /* done */

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Get the current locale.
 *
 * @param pKDBInfo                  Pointer to keyboard information structure.
 * @param pWordSymbInfo             Pointer to word symbol info structure.
 * @param pwLocale                  Where the current locale will be returned.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL ET9KDB_GetLocale(ET9KDBInfo        * const pKDBInfo,
                                      ET9WordSymbInfo   * const pWordSymbInfo,
                                      ET9U16            * const pwLocale)
{
    ET9STATUS eStatus;

    eStatus = __ET9KDB_BasicValidityCheck(pKDBInfo, 0);

    if (eStatus) {
        return eStatus;
    }

    if (!pwLocale || !pWordSymbInfo || pWordSymbInfo->wInitOK != ET9GOODSETUP) {
        return ET9STATUS_INVALID_MEMORY;
    }

    *pwLocale = pKDBInfo->Private.wLocale;

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Builds a saved word for reselection.
 *
 * @param pKDBInfo                  Pointer to keyboard information structure.
 * @param pWordSymbInfo             Pointer to word symbol info structure.
 * @param pSavedWord                Word to build.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

static ET9STATUS ET9LOCALCALL __ET9KDB_BuildSavedWord(ET9KDBInfo        * const pKDBInfo,
                                                      ET9WordSymbInfo   * const pWordSymbInfo,
                                                      ET9SavedInputWord * const pSavedWord)
{
    ET9STATUS           eStatus = ET9STATUS_NONE;
    ET9U16              wCount;
    ET9SYMB             sFunctionKey;
    ET9SYMB             *psSymb;
    ET9SavedInputSymb   *pSavedSymb;
    ET9BOOL             bHasTraceInfo = 0;

#ifdef ET9_KDB_TRACE_MODULE
    ET9UINT             nPointCount = 0;
    ET9U8               bCurrTraceIndex = 0;
    ET9SavedInputSymb   *pInitialTraceSavedSymb = NULL;
    ET9TracePoint       pPoints[ET9MAXWORDSIZE];
#endif

    ET9SavedInputWords  * const pSavedInputWords = &pWordSymbInfo->Private.sSavedInputWords;

    const ET9U8  bOldSwitchLanguage         = pWordSymbInfo->Private.bSwitchLanguage;
    const ET9U16 wOldFirstKdbNum            = pKDBInfo->wFirstKdbNum;
    const ET9U16 wOldFirstPageNum           = pKDBInfo->wFirstPageNum;
    const ET9U16 wOldSecondKdbNum           = pKDBInfo->wSecondKdbNum;
    const ET9U16 wOldSecondPageNum          = pKDBInfo->wSecondPageNum;
    const ET9U16 wOldScaleToLayoutWidth     = pKDBInfo->Private.wScaleToLayoutWidth;
    const ET9U16 wOldScaleToLayoutHeight    = pKDBInfo->Private.wScaleToLayoutHeight;

    ET9Assert(pKDBInfo);
    ET9Assert(pWordSymbInfo);
    ET9Assert(pSavedWord);
    ET9Assert(pSavedWord->wStorePos != UNDEFINED_STORE_INDEX);
    ET9Assert(pSavedWord->wStorePos < ET9SAVEINPUTSTORESIZE);

    WLOG5(fprintf(pLogFile5, "__ET9KDB_BuildSavedWord, kdb = %04x, page = %d, kdbinfo = %p\n", pKDBInfo->wFirstKdbNum, pKDBInfo->wFirstPageNum, pKDBInfo);)

    /* run the input data */

    ET9ClearAllSymbs(pWordSymbInfo);

    pSavedSymb = &pSavedInputWords->sInputs[pSavedWord->wStorePos];
    psSymb = &pSavedInputWords->sWords[pSavedWord->wStorePos];

    {
        ET9STATUS wKdbStatus;

        wKdbStatus = ET9KDB_SetKeyboardSize(pKDBInfo, 0, 0);

        ET9Assert(!wKdbStatus);
    }

    for (wCount = pSavedWord->wInputLen; wCount; --wCount, ++pSavedSymb, ++psSymb) {

        ET9BOOL bUsedExplicit = 0;
        ET9SymbInfo * pSymbInfo = &pWordSymbInfo->SymbsInfo[pWordSymbInfo->bNumSymbs];

        /* handle KDB switch */

        pWordSymbInfo->Private.bSwitchLanguage = 0;         /* really only once, but related */

        if ((pSavedSymb->wKdb1 || pSavedSymb->wKdb2) &&
            (pKDBInfo->wFirstKdbNum != pSavedSymb->wKdb1 ||
             pKDBInfo->wFirstPageNum != pSavedSymb->wPage1 ||
             pKDBInfo->wSecondKdbNum != pSavedSymb->wKdb2 ||
             pKDBInfo->wSecondPageNum != pSavedSymb->wPage2)) {

            ET9STATUS wKdbStatus;

            WLOG5(fprintf(pLogFile5, "__ET9KDB_BuildSavedWord, changing KDB setting\n");)

            wKdbStatus = ET9KDB_SetKdbNum(pKDBInfo,
                                          pSavedSymb->wKdb1,
                                          pSavedSymb->wPage1,
                                          pSavedSymb->wKdb2,
                                          pSavedSymb->wPage2);

            if (wKdbStatus) {
                ET9KDB_SetKeyboardSize(pKDBInfo, wOldScaleToLayoutWidth, wOldScaleToLayoutHeight);
                return wKdbStatus;
            }
        }

#ifdef ET9_KDB_TRACE_MODULE

        /* handle trace symbs */

        if (pSavedSymb->bTraceIndex || nPointCount) {

            const ET9U8 bNumSymbsOld = pWordSymbInfo->bNumSymbs;

            /* flush current? */

            if (nPointCount && !pSavedSymb->bTraceIndex ||
                nPointCount && pSavedSymb->bTraceIndex != bCurrTraceIndex) {

                eStatus = ET9KDB_ProcessTrace(pKDBInfo,
                                              pWordSymbInfo,
                                              pPoints,
                                              nPointCount,
                                              ET9_NO_ACTIVE_INDEX,
                                              &sFunctionKey);

                if (eStatus) {
                    WLOG5(fprintf(pLogFile5, "__ET9KDB_BuildSavedWord, process trace error (1)\n");)
                    break;
                }

                nPointCount = 0;
                bCurrTraceIndex = 0;
            }

            /* potentially handle trace point */

#if 1
            if (pSavedSymb->bTraceIndex) {
#else
            if (pSavedSymb->bTraceIndex && pSavedSymb->bTraceProbability) {
#endif

                bHasTraceInfo = 1;

                pPoints[nPointCount].nX = pSavedSymb->wTapX;
                pPoints[nPointCount].nY = pSavedSymb->wTapY;

                if (!nPointCount) {
                    bCurrTraceIndex = pSavedSymb->bTraceIndex;
                    pInitialTraceSavedSymb = pSavedSymb;
                }

                ++nPointCount;
            }

            /* need to flush if last symb */

            if (nPointCount && wCount == 1) {

                eStatus = ET9KDB_ProcessTrace(pKDBInfo,
                                              pWordSymbInfo,
                                              pPoints,
                                              nPointCount,
                                              ET9_NO_ACTIVE_INDEX,
                                              &sFunctionKey);

                if (eStatus) {
                    WLOG5(fprintf(pLogFile5, "__ET9KDB_BuildSavedWord, process trace error (2)\n");)
                    break;
                }

                if (sFunctionKey ||
                    !pWordSymbInfo->bNumSymbs ||
                    pWordSymbInfo->SymbsInfo[pWordSymbInfo->bNumSymbs - 1].bSymbType == ET9KTFUNCTION) {

                    WLOG5(fprintf(pLogFile5, "__ET9KDB_BuildSavedWord, got a function key\n");)

                    eStatus = ET9STATUS_ERROR;
                    break;
                }

                nPointCount = 0;
                bCurrTraceIndex = 0;
            }

            /* potentially reattach some properties */

            if (bNumSymbsOld != pWordSymbInfo->bNumSymbs) {
                pSymbInfo->bLocked              = pInitialTraceSavedSymb->bLocked;
                pSymbInfo->eShiftState          = pInitialTraceSavedSymb->eShiftState;
                pSymbInfo->bForcedLowercase     = pInitialTraceSavedSymb->bForcedLowercase;
            }
        }

#endif /* ET9_KDB_TRACE_MODULE */

        /* handle non trace symbs */

        if (!pSavedSymb->bTraceIndex) {

            pSymbInfo = &pWordSymbInfo->SymbsInfo[pWordSymbInfo->bNumSymbs];

            switch (pSavedSymb->eInputType) {

                case ET9DISCRETEKEY:
                case ET9REGIONALKEY:
                    if (pSavedSymb->wTapX != ET9UNDEFINEDTAPVALUE && pSavedSymb->wTapY != ET9UNDEFINEDTAPVALUE) {

                        eStatus = ET9KDB_ProcessTap(pKDBInfo,
                                                    pWordSymbInfo,
                                                    pSavedSymb->wTapX,
                                                    pSavedSymb->wTapY,
                                                    ET9_NO_ACTIVE_INDEX,
                                                    &sFunctionKey);

                        if (eStatus) {
                            WLOG5(fprintf(pLogFile5, "__ET9KDB_BuildSavedWord, ET9KDB_ProcessTap error\n");)
                        }
                    }
                    else {
                        eStatus = ET9KDB_ProcessKey(pKDBInfo,
                                                    pWordSymbInfo,
                                                    pSavedSymb->wKeyIndex,
                                                    ET9_NO_ACTIVE_INDEX,
                                                    &sFunctionKey);

                        if (eStatus) {
                            WLOG5(fprintf(pLogFile5, "__ET9KDB_BuildSavedWord, ET9KDB_ProcessKey error\n");)
                        }
                    }

                    if (!eStatus && (sFunctionKey ||
                                     !pWordSymbInfo->bNumSymbs ||
                                     pWordSymbInfo->SymbsInfo[pWordSymbInfo->bNumSymbs - 1].bSymbType == ET9KTFUNCTION)) {

                        WLOG5(fprintf(pLogFile5, "__ET9KDB_BuildSavedWord, got a function key\n");)

                        eStatus = ET9STATUS_ERROR;
                    }
                    break;

                case ET9HANDWRITING:
                case ET9MULTITAPKEY:
                case ET9CUSTOMSET:
                case ET9EXPLICITSYM:
                case ET9MULTISYMBEXPLICIT:
                default:
                    eStatus = ET9AddExplicitSymb(pWordSymbInfo,
                                                 pSavedSymb->sSymb,
                                                 ET9NOSHIFT,
                                                 ET9_NO_ACTIVE_INDEX);

                    bUsedExplicit = 1;

                    if (eStatus) {
                        WLOG5(fprintf(pLogFile5, "__ET9KDB_BuildSavedWord, ET9AddExplicitSymb error\n");)
                    }
                    break;
            }

            if (eStatus) {
                WLOG5(fprintf(pLogFile5, "__ET9KDB_BuildSavedWord, process error\n");)
                break;
            }

            {
                ET9Assert(pSymbInfo == &pWordSymbInfo->SymbsInfo[pWordSymbInfo->bNumSymbs - 1]);

                pSymbInfo->sLockedSymb          = *psSymb;                  /* ok on short word */

                pSymbInfo->wTapX                = pSavedSymb->wTapX;
                pSymbInfo->wTapY                = pSavedSymb->wTapY;
                pSymbInfo->wKeyIndex            = pSavedSymb->wKeyIndex;
                pSymbInfo->wInputIndex          = pSavedSymb->bInputIndex;
                pSymbInfo->bLocked              = pSavedSymb->bLocked;
                pSymbInfo->eInputType           = pSavedSymb->eInputType;
                pSymbInfo->eShiftState          = pSavedSymb->eShiftState;
                pSymbInfo->bForcedLowercase     = pSavedSymb->bForcedLowercase;
                pSymbInfo->bTraceProbability    = pSavedSymb->bTraceProbability;
                pSymbInfo->bTraceIndex          = pSavedSymb->bTraceIndex;
                pSymbInfo->wKdb1                = pSavedSymb->wKdb1;
                pSymbInfo->wPage1               = pSavedSymb->wPage1;
                pSymbInfo->wKdb2                = pSavedSymb->wKdb2;
                pSymbInfo->wPage2               = pSavedSymb->wPage2;

                if (bUsedExplicit) {

                    pSymbInfo->DataPerBaseSym[0].sChar[0] = pSavedSymb->sSymb;

                    if (pSavedSymb->eInputType != ET9MULTISYMBEXPLICIT) {

                        WLOG5(fprintf(pLogFile5, "__ET9KDB_BuildSavedWord, (explicit) will try to add upper/lower case char versions\n");)

                        if (pSymbInfo->DataPerBaseSym[0].bNumSymsToMatch < ET9MAXALTSYMBS) {

                            const ET9U8 bIndex = pSymbInfo->DataPerBaseSym[0].bNumSymsToMatch;

                            pSymbInfo->DataPerBaseSym[0].sChar[bIndex] = _ET9SymToLower(pSymbInfo->DataPerBaseSym[0].sChar[0], pWordSymbInfo->Private.wLocale);
                            pSymbInfo->DataPerBaseSym[0].sUpperCaseChar[bIndex] = _ET9SymToUpper(pSymbInfo->DataPerBaseSym[0].sUpperCaseChar[0], pWordSymbInfo->Private.wLocale);

                            if (pSymbInfo->DataPerBaseSym[0].sChar[bIndex] != pSymbInfo->DataPerBaseSym[0].sChar[0] ||
                                pSymbInfo->DataPerBaseSym[0].sUpperCaseChar[bIndex] != pSymbInfo->DataPerBaseSym[0].sUpperCaseChar[0]) {

                                ++pSymbInfo->DataPerBaseSym[0].bNumSymsToMatch;
                            }
                        }
                    }
                }
            }
        }
    }

    /* restore KDB settings */

    pWordSymbInfo->Private.bSwitchLanguage = bOldSwitchLanguage;

    {
        ET9STATUS wKdbStatus;

        wKdbStatus = ET9KDB_SetKeyboardSize(pKDBInfo, wOldScaleToLayoutWidth, wOldScaleToLayoutHeight);

        ET9Assert(!wKdbStatus);
    }

    if ((wOldFirstKdbNum || wOldSecondKdbNum) &&
        (pKDBInfo->wFirstKdbNum != wOldFirstKdbNum ||
         pKDBInfo->wFirstPageNum != wOldFirstPageNum ||
         pKDBInfo->wSecondKdbNum != wOldSecondKdbNum ||
         pKDBInfo->wSecondPageNum != wOldSecondPageNum)) {

        ET9STATUS wKdbStatus;

        WLOG5(fprintf(pLogFile5, "__ET9KDB_BuildSavedWord, restoring KDB setting\n");)

        wKdbStatus = ET9KDB_SetKdbNum(pKDBInfo,
                                      wOldFirstKdbNum,
                                      wOldFirstPageNum,
                                      wOldSecondKdbNum,
                                      wOldSecondPageNum);

        if (wKdbStatus) {
            return wKdbStatus;
        }
    }

    /* validate */

    if (eStatus) {
        WLOG5(fprintf(pLogFile5, "__ET9KDB_BuildSavedWord, got error %d\n", eStatus);)
        return ET9STATUS_ERROR;
    }

    /* validate */

    if (pWordSymbInfo->bNumSymbs != pSavedWord->wInputLen) {

        WLOG5(fprintf(pLogFile5, "__ET9KDB_BuildSavedWord, input length missmatch, %u <-> %u\n", pWordSymbInfo->bNumSymbs, pSavedWord->wInputLen);)

        if (!bHasTraceInfo) {
            return ET9STATUS_ERROR;
        }
    }

    /* "restore" the last known shift state (affects e.g. completions, or else the required word might not be found) */

    pWordSymbInfo->Private.eLastShiftState = pSavedWord->eLastShiftState;
    pWordSymbInfo->Private.eRequiredLastShiftState = pSavedWord->eLastShiftState;

    /* done */

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/**
 * @brief Reselects a word from saved or recreated data.
 *
 * @param[in]     pKDBInfo                  Pointer to the Keyboard Input Module Information Data Structure (ET9KDBInfo).
 * @param[in,out] pWordSymbInfo             (ET9WordSymbInfo). The Keyboard Input Module writes to this structure information about the input the user has provided. Calling ET9KDB_SetAmbigMode may result in changes to the data in this structure.
 * @param[in]     psWord                    Word to be reselected.
 * @param[in]     wWordLen                  Length of the word to be reselected.
 *
 * @retval ET9STATUS_NONE           Function call was handled successfully.
 * @retval ET9STATUS_INVALID_MEMORY At least one pointer passed as a function argument was set to null.
 * @retval ET9STATUS_NO_INIT        The Keyboard Input Module has not been initialized. Call ET9KDB_Init().
 *
 * @remarks When the user highlights a previously accepted word, the word is reinstated from memory, or.
 * recreated, with its original input sequence and key values. The integration layer can then
 * request that XT9 build a selection list based on the data.<br>
 * This is useful in situations where a user will go back to correct or modify a word. Rather than
 * retyping an entire word, a selection list based on the original word attributes is made available.
 * The user can choose a word from the selection list or modify the reselected text to update it.
 */

ET9STATUS ET9FARCALL ET9KDB_ReselectWord(ET9KDBInfo      * const pKDBInfo,
                                         ET9WordSymbInfo * const pWordSymbInfo,
                                         ET9SYMB         * const psWord,
                                         const ET9U16            wWordLen)
{
    ET9STATUS       eStatus;
    ET9BOOL         bRecreateInput = 1;
    ET9U32          dwOldKdbState;
    ET9U16          wPageNum;

    WLOG5(fprintf(pLogFile5, "ET9KDB_ReselectWord\n");)

    /* validate */

    eStatus = __ET9KDB_BasicValidityCheck(pKDBInfo, 1);

    if (eStatus) {
        return eStatus;
    }

    if (!pWordSymbInfo || !psWord || pWordSymbInfo->wInitOK != ET9GOODSETUP) {
        return ET9STATUS_INVALID_MEMORY;
    }
    if (!wWordLen || wWordLen > ET9MAXWORDSIZE) {
        return ET9STATUS_OUT_OF_RANGE;
    }

    /* break insert mode */

    pKDBInfo->dwStateBits &= ~ET9_KDB_INSERT_MASK;

    /* reset filtering (might have to try and capture it instead) */

    __FilterSymbReset(pKDBInfo);

    /* record kdb state, and set new state */

    dwOldKdbState = pKDBInfo->dwStateBits;

    pKDBInfo->dwStateBits = (pKDBInfo->dwStateBits & ET9_KDB_DISCRETE_MASK) | ET9_KDB_AMBIGUOUS_MODE_MASK;

    /* first - look for a saved word */
    {
        ET9SavedInputWords  * const pSavedInputWords = &pWordSymbInfo->Private.sSavedInputWords;

        ET9U16  wWordIndex;
        ET9U16  wWordCount;

        wWordIndex = pSavedInputWords->wCurrInputSaveIndex;

        for (wWordCount = ET9MAXSAVEINPUTWORDS;
             wWordCount;
             --wWordCount,
             wWordIndex = (ET9U16)(wWordIndex == 0 ? (ET9MAXSAVEINPUTWORDS - 1) : (wWordIndex - 1))) {

            ET9SavedInputWord * const pSavedWord = &pSavedInputWords->pSavedWords[wWordIndex];

            WLOG5(fprintf(pLogFile5, "ET9KDB_ReselectWord, trying index %d\n", wWordIndex);)

            if (pSavedWord->wStorePos == UNDEFINED_STORE_INDEX) {
                WLOG5(fprintf(pLogFile5, "ET9KDB_ReselectWord, found undefined word, break\n");)
                break;
            }

            ET9Assert(pSavedWord->wStorePos < ET9SAVEINPUTSTORESIZE);

            if (pSavedWord->wWordLen != wWordLen) {
                WLOG5(fprintf(pLogFile5, "ET9KDB_ReselectWord, wrong length, skip\n");)
                continue;
            }

            {
                ET9U16              wCmpLen;
                ET9SYMB             *pStr1;
                ET9SYMB             *pStr2;

                wCmpLen = wWordLen;
                pStr1 = psWord;
                pStr2 = &pSavedInputWords->sWords[pSavedWord->wStorePos];
                ++wCmpLen;
                while (--wCmpLen) {
                    if (*pStr1++ != *pStr2++) {
                        break;
                    }
                }

                if (wCmpLen) {
                    WLOG5(fprintf(pLogFile5, "ET9KDB_ReselectWord, no match\n");)
                    continue;
                }

                WLOG5(fprintf(pLogFile5, "ET9KDB_ReselectWord, found saved word\n");)

                if (!__ET9KDB_BuildSavedWord(pKDBInfo, pWordSymbInfo, pSavedWord)) {
                    bRecreateInput = 0;
                }

                break;
            }
        }
    }

    /* otherwise - set up input symbols according to the word and kdb */

    if (bRecreateInput) {

        ET9U16          wCount;
        ET9U16          wKeyIndex;
        ET9SYMB         sFunctionKey;
        ET9Region       sKeyRegion;
        ET9SYMB         *psSymb;
        ET9SymbInfo     *pSymbInfo;
        ET9U8            byRegionalKey;

        WLOG5(fprintf(pLogFile5, "ET9KDB_ReselectWord, recreating word\n");)

        ET9ClearAllSymbs(pWordSymbInfo);

        psSymb = psWord;
        pSymbInfo = pWordSymbInfo->SymbsInfo;

        for (wCount = wWordLen; wCount; --wCount, ++psSymb, ++pSymbInfo) {

            /* try adding it using current kdb, if it fails try explicit entry */

            if (pKDBInfo->wFirstKdbNum == pKDBInfo->wKdbNum) {
                wPageNum = pKDBInfo->wFirstPageNum;
            }
            else {
                wPageNum = pKDBInfo->wSecondPageNum;
            }

            eStatus = _ET9KDB_FindSymbol(pKDBInfo,
                                         _ET9SymToLower(*psSymb, pWordSymbInfo->Private.wLocale),
                                         pKDBInfo->wKdbNum,
                                         wPageNum,
                                         &byRegionalKey,
                                         &wKeyIndex,
                                         &sKeyRegion,
                                         0);

            if (!eStatus) {

                eStatus = ET9KDB_ProcessKey(pKDBInfo, pWordSymbInfo, wKeyIndex, ET9_NO_ACTIVE_INDEX, &sFunctionKey);

                if (!eStatus && sFunctionKey) {
                    eStatus = ET9STATUS_ERROR;
                }
                else if (!eStatus && (pWordSymbInfo->bNumSymbs &&
                                      pWordSymbInfo->SymbsInfo[pWordSymbInfo->bNumSymbs - 1].bSymbType == ET9KTFUNCTION)) {

                    WLOG5(fprintf(pLogFile5, "ET9KDB_ReselectWord, got a function key\n");)

                    --pWordSymbInfo->bNumSymbs;

                    eStatus = ET9STATUS_ERROR;
                }

            }

            if (eStatus) {

                WLOG5(fprintf(pLogFile5, "ET9KDB_ReselectWord, failed to find symbol %04x\n", _ET9SymToLower(*psSymb, pWordSymbInfo->Private.wLocale));)

                eStatus = ET9AddExplicitSymb(pWordSymbInfo, *psSymb, ET9NOSHIFT, ET9_NO_ACTIVE_INDEX);

                ET9Assert(eStatus ||
                          !pWordSymbInfo->bNumSymbs ||
                          pWordSymbInfo->SymbsInfo[pWordSymbInfo->bNumSymbs - 1].bSymbType != ET9KTFUNCTION);
            }

            if (eStatus) {

                WLOG5(fprintf(pLogFile5, "ET9KDB_ReselectWord, even explicit failed\n");)

                pKDBInfo->dwStateBits = dwOldKdbState;
                ET9ClearAllSymbs(pWordSymbInfo);
                return eStatus;
            }

            /* set shift according to word info */

            if (_ET9SymIsUpper(*psSymb, pWordSymbInfo->Private.wLocale)) {
                pSymbInfo->eShiftState = ET9SHIFT;
            }
            else {
                pSymbInfo->eShiftState = ET9NOSHIFT;
            }
        }
    }

    /* set up the required word */

    {
        ET9SimpleWord * const pReqWord = &pWordSymbInfo->Private.sRequiredWord;

        _ET9InitSimpleWord(pReqWord);

        pReqWord->wLen = wWordLen;

        _ET9SymCopy(pReqWord->sString, psWord, wWordLen);

        pWordSymbInfo->Private.bRequiredLocate = 1;
        pWordSymbInfo->Private.bRequiredVerifyInput = 1;
        pWordSymbInfo->Private.bRequiredInhibitOverride = 0;
        pWordSymbInfo->Private.bRequiredInhibitCorrection = 0;
        pWordSymbInfo->Private.bRequiredHasRegionalInfo = _ET9HasRegionalInfo(pWordSymbInfo);
    }

    _ET9InvalidateSelList(pWordSymbInfo);

    /* restore kdb state */

    pKDBInfo->dwStateBits = dwOldKdbState;

    /* done */

    WLOG5(fprintf(pLogFile5, "ET9KDB_ReselectWord, input done ok\n");)

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/**
 * @brief Next diacritic state. Used for implementations of XT9 Japanese or T9 Nav Core.
 *
 * @param[in]     pKDBInfo      Pointer to the Keyboard Input Module Information Data Structure (ET9KDBInfo).
 * @param[in]     pWordSymbInfo     Pointer to the Word Symbol Data Structure (ET9WordSymbInfo).
 *
 * @retval ET9STATUS_NONE           Function call was handled successfully.
 * @retval ET9STATUS_INVALID_MEMORY At least one pointer passed as a function argument was set to null.
 * @retval ET9STATUS_NO_INIT        The Keyboard Input Module has not been initialized. Call ET9KDB_Init().
 *
 */

ET9STATUS ET9FARCALL ET9KDB_NextDiacritic(ET9KDBInfo        * const pKDBInfo,
                                          ET9WordSymbInfo   * const pWordSymbInfo)
{
    ET9STATUS   eStatus;
    ET9U8       bFilterCount;

    WLOG5(fprintf(pLogFile5, "\nET9KDB_NextDiacritic, pKDBInfo = %p\n", pKDBInfo);)

    eStatus = __ET9KDB_BasicValidityCheck(pKDBInfo, 1);

    if (eStatus) {
        return eStatus;
    }

    if (!pWordSymbInfo || pWordSymbInfo->wInitOK != ET9GOODSETUP) {
        return ET9STATUS_INVALID_MEMORY;
    }

    if (!pWordSymbInfo->bNumSymbs) {
        return ET9STATUS_NONE;
    }

    /* turn off insert mode */

    pKDBInfo->dwStateBits &= ~ET9_KDB_INSERT_MASK;

    /* check if applicable */

    if (!pKDBInfo->Private.pFilterSymb || !pKDBInfo->Private.pFilterSymbCount || !pKDBInfo->Private.pFilterSymbNext) {
        return ET9STATUS_ERROR;
    }

    /* verify that the current symb is the last known one */

    if (!pKDBInfo->Private.sKdbAction.dwCurrChecksum ||
        pKDBInfo->Private.sKdbAction.dwCurrChecksum != __CalculateLastWordSymbChecksum(pWordSymbInfo)) {

        return ET9STATUS_NONE;
    }

    /* get filter count */

    eStatus = pKDBInfo->Private.pFilterSymbCount(pKDBInfo->Private.pFilterSymbInfo, &bFilterCount);

    if (eStatus) {
        return eStatus;
    }

    if (!bFilterCount) {
        return ET9STATUS_NONE;
    }

    /* move to next state */

    {
        const ET9U8             bNumSymbs = pWordSymbInfo->bNumSymbs;
        ET9SymbInfo     * const pSymbInfo = pWordSymbInfo->SymbsInfo + bNumSymbs - 1;
        ET9U8                   bCount;
        const ET9U32            dwOldCheckSum = pKDBInfo->Private.sKdbAction.dwCurrChecksum;

        /* clear checksum, so we know if we picked up something at the end */

        pKDBInfo->Private.sKdbAction.dwCurrChecksum = 0;

        /* loop over the diac states */

        for (bCount = bFilterCount; bCount; --bCount) {

            WLOG5(fprintf(pLogFile5, "  bCount = %d (%d)\n", bCount, bFilterCount);)

            /* next filter setting */

            eStatus = pKDBInfo->Private.pFilterSymbNext(pKDBInfo->Private.pFilterSymbInfo);

            if (eStatus) {
                return eStatus;
            }

            /* execute filter (reload) */

            {
                ET9U16 sFunctionKey = 0;

                ET9DirectedPos sDirectedPos;

                __InitDirectedPos(&sDirectedPos);

                sDirectedPos.sPos.nX = (ET9UINT)pKDBInfo->Private.sKdbAction.u.tapAction.wX;
                sDirectedPos.sPos.nY = (ET9UINT)pKDBInfo->Private.sKdbAction.u.tapAction.wY;

                eStatus = ET9STATUS_NONE;

                switch (pSymbInfo->eInputType)
                {
                    case ET9DISCRETEKEY:
                    case ET9REGIONALKEY:
                        if (pKDBInfo->Private.sKdbAction.bIsKeyAction) {
                            eStatus = __ProcessAmbigKey_ActiveKdbs(pKDBInfo,
                                                                   pKDBInfo->Private.sKdbAction.u.keyAction.wKeyIndex,
                                                                   pWordSymbInfo,
                                                                   &sFunctionKey,
                                                                   pKDBInfo->Private.sKdbAction.bCurrIndexInList,
                                                                   LOADSYMBACTION_RELOAD);
                        }
                        else {
                            eStatus = __ProcessAmbigTap_ActiveKdbs(pKDBInfo,
                                                                   &sDirectedPos,
                                                                   pWordSymbInfo,
                                                                   &sFunctionKey,
                                                                   pKDBInfo->Private.sKdbAction.bCurrIndexInList,
                                                                   LOADSYMBACTION_RELOAD);
                        }
                        break;
                    case ET9MULTITAPKEY:
                        if (pKDBInfo->Private.sKdbAction.bIsKeyAction) {
                            eStatus = __ProcessMultitapKey_ActiveKdbs(pKDBInfo,
                                                                      pKDBInfo->Private.sKdbAction.u.keyAction.wKeyIndex,
                                                                      pWordSymbInfo,
                                                                      &sFunctionKey,
                                                                      pKDBInfo->Private.sKdbAction.bCurrIndexInList,
                                                                      LOADSYMBACTION_RELOAD);
                        }
                        else {
                            eStatus = __ProcessMultitapTap_ActiveKdbs(pKDBInfo,
                                                                      &sDirectedPos,
                                                                      pWordSymbInfo,
                                                                      &sFunctionKey,
                                                                      pKDBInfo->Private.sKdbAction.bCurrIndexInList,
                                                                      LOADSYMBACTION_RELOAD);
                        }
                        break;
                    default:
                        return ET9STATUS_NONE;
                }

                ET9Assert(!eStatus);
                ET9Assert(!sFunctionKey);
                ET9Assert(pWordSymbInfo->bNumSymbs == bNumSymbs);
            }

            /* filter applicable? (change detected?) */

            {
                const ET9U32  dwNewCheckSum = __CalculateLastWordSymbChecksum(pWordSymbInfo);

                WLOG5(fprintf(pLogFile5, "  dwNewCheckSum = %d, dwOldCheckSum = %d\n", dwNewCheckSum, dwOldCheckSum);)

                /* either we find a new ok state, or we are back to the first one ok */

                if (dwNewCheckSum && (dwNewCheckSum != dwOldCheckSum || bCount == 1)) {
                    pKDBInfo->Private.sKdbAction.dwCurrChecksum = dwNewCheckSum;
                    return ET9STATUS_NONE;
                }
            }
        }
    }

    /* done - nothing happened */

    ET9Assert(pKDBInfo->Private.sKdbAction.dwCurrChecksum);

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/**
 * @brief Sets the Keyboard Input Module to regional mode.
 *
 * @param[in]     pKDBInfo      Pointer to the Keyboard Input Module Information Data Structure (ET9KDBInfo).
 *
 * @retval ET9STATUS_NONE       Function call was handled successfully.
 * @retval ET9STATUS_INVALID_MEMORY At least one pointer passed as a function argument was set to null.
 * @retval ET9STATUS_NO_INIT        The Keyboard Input Module has not been initialized. Call ET9KDB_Init().
 *
 * @see ET9KDB_SetDiscreteMode(), ET9KDB_SetMultiTapMode()
 *
 * @remarks Regional mode handles imprecise input.With imprecise input it is possible that the input provided may not have been what a person intended.
 * For example, a person may have intended to tap the "s" key on a virtual keyboard but actually tapped the "a" key. Regional mode takes
 * neighboring keys into consideration when extrapolating the best matches for the selection list.<br>
 * When text is entered through regional mode, selection list results are searched for based on the
 * specific character tapped with the stylus.
 */

ET9STATUS ET9FARCALL ET9KDB_SetRegionalMode(ET9KDBInfo * const pKDBInfo)
{
    ET9STATUS       eStatus;

    eStatus = __ET9KDB_BasicValidityCheck(pKDBInfo, 1);

    if (eStatus) {
        return eStatus;
    }

    /* set regional mask */

    pKDBInfo->dwStateBits &= ~ET9_KDB_DISCRETE_MASK;

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/**
 * @brief Sets the Keyboard Input Module to discrete mode.
 *
 * @param[in]     pKDBInfo  Pointer to the Keyboard Input Module Information Data Structure (ET9KDBInfo).
 *
 * @retval ET9STATUS_NONE           Function call was handled successfully.
 * @retval ET9STATUS_INVALID_MEMORY At least one pointer passed as a function argument was set to null.
 * @retval ET9STATUS_NO_INIT        The Keyboard Input Module has not been initialized. Call ET9KDB_Init().
 *
 * @see ET9KDB_SetRegionalMode(), ET9KDB_SetMultiTapMode()
 *
 * @remarks Discrete mode handles precise input. With precise input there is no doubt as to what input was intended.
 * For example, a person presses the "a" key on a keyboard or the "2" key on a telephone keypad. Results in the selection list are based strictly on the precise key pressed or tapped.<br>
 * When text is entered through discrete mode, selection list results are searched for based on the
 * "first" character mapped to a key. In a standard Keyboard Database, the first character is the
 * number associated with the key.<br>
 * To enable regional mode, which handles imprecise key input, the integration layer calls ET9KDB_SetRegionalMode().
 */

ET9STATUS ET9FARCALL ET9KDB_SetDiscreteMode(ET9KDBInfo * const pKDBInfo)
{
    ET9STATUS eStatus;

    eStatus = __ET9KDB_BasicValidityCheck(pKDBInfo, 1);

    if (eStatus) {
        return eStatus;
    }

    /* set regional mask */

    pKDBInfo->dwStateBits |= ET9_KDB_DISCRETE_MASK;

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/**
 * @brief Sets the convert symbol feature.
 *
 * @param[in]     pKDBInfo              Pointer to the Keyboard Input Module Information Data Structure (ET9KDBInfo).
 * @param[in]     pConvertSymb          Pointer to a ET9CONVERTSYMBCALLBACK() or NULL to disable the feature.
 * @param[out]    pConvertSymbInfo     Pointer passed back from ET9CONVERTSYMBCALLBACK().
 *
 * @retval ET9STATUS_NONE           Function call was handled successfully.
 * @retval ET9STATUS_INVALID_MEMORY At least one pointer passed as a function argument was set to null.
 * @retval ET9STATUS_NO_INIT        The Keyboard Input Module has not been initialized. Call ET9KDB_Init().
 *
 * @remarks This function controls the convert symbol feature (on-the-fly-mapping) for the Keyboard Input Module. There are partner function(s) that should be set in the lingustic engines.
 * This feature applies to what goes into the selection list as well as e.g. multitap symbols. This
 * function will be called for every symbol handled. Thus, performance-wise it's important to
 * keep the convert function fast.
 *
 * @note Nuance recommends that this function should be set consistently in order to minimize unexpected behavior.
 *
 */

ET9STATUS ET9FARCALL ET9KDB_SetConvertSymb(ET9KDBInfo                   * const pKDBInfo,
                                           const ET9CONVERTSYMBCALLBACK         pConvertSymb,
                                           void                         * const pConvertSymbInfo)
{
    ET9STATUS eStatus;

    eStatus = __ET9KDB_BasicValidityCheck(pKDBInfo, 1);

    if (eStatus) {
        return eStatus;
    }

    pKDBInfo->Private.pConvertSymb     = pConvertSymb;
    pKDBInfo->Private.pConvertSymbInfo = pConvertSymbInfo;

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/**
 * @brief Retrieves the current multitap sequence for the current key.
 *
 * @param[in]     pKDBInfo               Pointer to the Keyboard Input Module Information Data Structure (ET9KDBInfo).
 * @param[in]     psMultiTapSequenceBuf  Pointer to the multitap sequence buffer.
 * @param[in]     wBufSize               Size of the multitap sequence buffer.
 * @param[in]     pwTotalSymbs           Pointer to the total number of symbols in the multitap sequence buffer.
 * @param[in]     pbCurrSelSymbIndex     Pointer to the index of the currently selected symbol in the multitap sequence.
 *
 * @retval ET9STATUS_NONE               Function call was handled successfully.
 * @retval ET9STATUS_INVALID_MEMORY     At least one pointer passed as a function argument was set to null.
 * @retval ET9STATUS_NO_INIT            The Keyboard Input Module has not been initialized. Call ET9KDB_Init().
 * @retval ET9STATUS_INVALID_KDB_PAGE   The page number of the current keyboard page is not valid.
 *                                      If you receive this status, contact your Nuance support engineer for assistance
 *
 * @see ET9KDB_GetKdbNum(), ET9KDB_SetPageNum()
 *
 */

ET9STATUS ET9FARCALL ET9KDB_GetMultiTapSequence(ET9KDBInfo   * const pKDBInfo,
                                                ET9SYMB      * const psMultiTapSequenceBuf,
                                                const ET9U16         wBufSize,
                                                ET9U16       * const pwTotalSymbs,
                                                ET9U8        * const pbCurrSelSymbIndex)
{
    ET9STATUS   eStatus;
    ET9SYMB     *pBuffSrc;
    ET9SYMB     *pBuffDest;
    ET9INT      nTotalCounts;

    eStatus = __ET9KDB_BasicValidityCheck(pKDBInfo, 1);

    if (eStatus) {
        return eStatus;
    }

    if (!psMultiTapSequenceBuf || !pwTotalSymbs || !pbCurrSelSymbIndex) {
        return ET9STATUS_INVALID_MEMORY;
    }

    if (!wBufSize || wBufSize < ET9_KDB_MAX_MT_SYMBS) {
        return ET9STATUS_BUFFER_TOO_SMALL;
    }

    nTotalCounts = *pwTotalSymbs = pKDBInfo->Private.bMTSymbCount;
    pBuffSrc = pKDBInfo->Private.sMTSymbs;
    pBuffDest = psMultiTapSequenceBuf;

    while (nTotalCounts-- > 0) {
        *pBuffDest++ = *pBuffSrc++;
    }

    *pbCurrSelSymbIndex = pKDBInfo->Private.bMTLastSymbIndex;

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Convert a key area info to a key point.
 *
 * @param[in]     pKDBInfo          Pointer to the Keyboard Input Module Information Data Structure (ET9KDBInfo).
 * @param[in]     pArea             Pointer to internal key area info.
 * @param[in,out] pPoint            Pointer to a key point to receive key information.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

static void ET9LOCALCALL __KeyAreaToKeyPoint(ET9KDBInfo             * const pKDBInfo,
                                             ET9KdbAreaInfo   const * const pArea,
                                             ET9KeyPoint            * const pPoint)
{
    pPoint->eKeyType = pArea->eKeyType;
    pPoint->eInputType = pArea->eInputType;
    pPoint->wKey = pArea->wKeyIndex;
    pPoint->sTopSymb = pArea->psChars[0];
    pPoint->nSymbCount = pArea->nCharCount;
    pPoint->psSymbs = &pArea->psChars[0];

    pPoint->nX = (ET9UINT)__ScaleCoordinateToIntegrationX(pKDBInfo, pArea->nCenterX);
    pPoint->nY = (ET9UINT)__ScaleCoordinateToIntegrationY(pKDBInfo, pArea->nCenterY);

    if (pArea->bRegionOk) {
        pPoint->sArea.wLeft   = (ET9U16)__ScaleCoordinateToIntegrationX(pKDBInfo, pArea->sRegion.wLeft);
        pPoint->sArea.wTop    = (ET9U16)__ScaleCoordinateToIntegrationY(pKDBInfo, pArea->sRegion.wTop);
        pPoint->sArea.wRight  = (ET9U16)__ScaleCoordinateToIntegrationX(pKDBInfo, pArea->sRegion.wRight);
        pPoint->sArea.wBottom = (ET9U16)__ScaleCoordinateToIntegrationY(pKDBInfo, pArea->sRegion.wBottom);
    }
    else {
        pPoint->sArea.wLeft   = 0;
        pPoint->sArea.wTop    = 0;
        pPoint->sArea.wRight  = 0;
        pPoint->sArea.wBottom = 0;
    }
}

/*---------------------------------------------------------------------------*/
/** \ingroup et9trace
 * Get information about keys in the currently active KDB.
 * This information can be used for mainly debug purposes to draw key positions on the screen to verify key centers.
 *
 * @param[in]     pKDBInfo          Pointer to the Keyboard Input Module Information Data Structure (ET9KDBInfo).
 * @param[in,out] pPoints           Pointer to a list of key points to receive key information.
 * @param[in]     nMaxPointCount    Max number of points that the pPoints array can hold.
 * @param[out]    pnPointCount      Pointer to receive the number of points in the list.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL ET9KDB_GetKeyPositions(ET9KDBInfo             * const pKDBInfo,
                                            ET9KeyPoint            * const pPoints,
                                            const ET9UINT                  nMaxPointCount,
                                            ET9UINT                * const pnPointCount)
{
    ET9STATUS    eStatus;

    WLOG5(fprintf(pLogFile5, "ET9KDB_GetKeyPositions\n");)

    eStatus = __ET9KDB_BasicValidityCheck(pKDBInfo, 1);

    if (eStatus) {
        return eStatus;
    }

    if (!pPoints || !pnPointCount) {
        return ET9STATUS_INVALID_MEMORY;
    }

    /* init */

    *pnPointCount = 0;

    if (!pKDBInfo->Private.pCurrLayoutInfo->nKeyAreaCount) {
        return ET9STATUS_NONE;
    }

    /* assure some key values */

    ET9Assert(pKDBInfo->Private.pCurrLayoutInfo->nRadius);
    ET9Assert(pKDBInfo->Private.pCurrLayoutInfo->nBoxWidth);
    ET9Assert(pKDBInfo->Private.pCurrLayoutInfo->nBoxHeight);
    ET9Assert(pKDBInfo->Private.pCurrLayoutInfo->nMinKeyWidth);
    ET9Assert(pKDBInfo->Private.pCurrLayoutInfo->nMinKeyHeight);
    ET9Assert(pKDBInfo->Private.pCurrLayoutInfo->nMedianKeyWidth);
    ET9Assert(pKDBInfo->Private.pCurrLayoutInfo->nMedianKeyHeight);

    /* big enough? */

    if (nMaxPointCount < pKDBInfo->Private.pCurrLayoutInfo->nKeyAreaCount) {
        return ET9STATUS_BAD_PARAM;
    }

    /* get positions */

    {
        ET9KdbLayoutInfo * const pLayoutInfo = pKDBInfo->Private.pCurrLayoutInfo;

        ET9UINT nIndex;

        for (nIndex = 0; nIndex < pLayoutInfo->nKeyAreaCount; ++nIndex) {

            __KeyAreaToKeyPoint(pKDBInfo, &pLayoutInfo->pKeyAreas[nIndex], &pPoints[nIndex]);

            WLOG5(fprintf(pLogFile5, "  [%2u] x %3u, y %3u, key %2u, top %c\n",
                                     nIndex,
                                     pPoints[nIndex].nX,
                                     pPoints[nIndex].nY,
                                     pPoints[nIndex].wKey,
                                     (char)pPoints[nIndex].sTopSymb);)
        }

        *pnPointCount = pLayoutInfo->nKeyAreaCount;
    }

    /* done */

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/**
 * Get information about the key associated with a tap position.
 * This function can only be used with keyboards that support key regions (e.g. dynamic KDBs).
 *
 * @param[in]     pKDBInfo          Pointer to the Keyboard Input Module Information Data Structure (ET9KDBInfo).
 * @param[in]     wX                Horizontal (x) coordinate of the tap.
 * @param[in]     wY                Vertical (y) coordinate of the tap.
 * @param[in,out] pPoint            Pointer to a key point to receive key information.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL ET9KDB_GetKeyPositionByTap(ET9KDBInfo             * const pKDBInfo,
                                                const ET9U16                   wX,
                                                const ET9U16                   wY,
                                                ET9KeyPoint            * const pPoint)
{
    ET9STATUS    eStatus;

    WLOG5(fprintf(pLogFile5, "ET9KDB_GetKeyPositionByTap\n");)

    eStatus = __ET9KDB_BasicValidityCheck(pKDBInfo, 1);

    if (eStatus) {
        return eStatus;
    }

    if (!pPoint ) {
        return ET9STATUS_INVALID_MEMORY;
    }

    {
        const ET9U16 wKdbX = (ET9U16)__ScaleCoordinateToKdbX(pKDBInfo, wX);
        const ET9U16 wKdbY = (ET9U16)__ScaleCoordinateToKdbY(pKDBInfo, wY);

        ET9KdbAreaInfo const * pArea = __GetKeyAreaFromTap_Generic(pKDBInfo, wKdbX, wKdbY);

        if (!pArea) {
            return ET9STATUS_NO_KEY;
        }

        __KeyAreaToKeyPoint(pKDBInfo, pArea, pPoint);
    }

    /* done */

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \ingroup et9trace
 * Set keyboard scaling.
 * A keyboard can be scaled if the only difference is a true scaling in x-y coordinates.
 * Coordinates must be in this new space, key position data will be returned in this space.
 * Setting scaling to zero resets it to KDB size (turns off scaling).
 *
 * @param[in]     pKDBInfo          Pointer to the Keyboard Input Module Information Data Structure (ET9KDBInfo).
 * @param[in]     wLayoutWidth      Layout width to scale to.
 * @param[in]     wLayoutHeight     Layout height to scale to.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL ET9KDB_SetKeyboardSize(ET9KDBInfo          * const pKDBInfo,
                                            const ET9U16                wLayoutWidth,
                                            const ET9U16                wLayoutHeight)
{
    ET9STATUS eStatus;

    WLOG5(fprintf(pLogFile5, "ET9KDB_SetKeyboardSize, width %u, height %u\n", wLayoutWidth, wLayoutHeight);)

    eStatus = __ET9KDB_BasicValidityCheck(pKDBInfo, 1);

    if (eStatus) {
        return eStatus;
    }

    if (wLayoutWidth && !wLayoutHeight || !wLayoutWidth && wLayoutHeight) {
        return ET9STATUS_ERROR;
    }

    pKDBInfo->Private.wScaleToLayoutWidth = wLayoutWidth;
    pKDBInfo->Private.wScaleToLayoutHeight = wLayoutHeight;

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/**
 * Get keyboard default size (non scaled).
 *
 * @param[in]     pKDBInfo          Pointer to the Keyboard Input Module Information Data Structure (ET9KDBInfo).
 * @param[out]    pwLayoutWidth     Layout width to scale to.
 * @param[out]    pwLayoutHeight    Layout height to scale to.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL ET9KDB_GetKeyboardDefaultSize(ET9KDBInfo       * const pKDBInfo,
                                                   ET9U16           * const pwLayoutWidth,
                                                   ET9U16           * const pwLayoutHeight)
{
    ET9STATUS eStatus;

    WLOG5(fprintf(pLogFile5, "ET9KDB_GetKeyboardDefaultSize\n");)

    eStatus = __ET9KDB_BasicValidityCheck(pKDBInfo, 1);

    if (eStatus) {
        return eStatus;
    }

    if (!pwLayoutWidth && !pwLayoutHeight) {
        return ET9STATUS_INVALID_MEMORY;
    }

    *pwLayoutWidth = pKDBInfo->Private.wLayoutWidth;
    *pwLayoutHeight = pKDBInfo->Private.wLayoutHeight;

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \ingroup et9trace
 * Set keyboard offset.
 * A keyboard can be given an offset if for example it isn't positioned in the uppper left corner of a window.
 * Coordinates must be in this new space, key position data will be returned in this space.
 *
 * @param[in]     pKDBInfo          Pointer to the Keyboard Input Module Information Data Structure (ET9KDBInfo).
 * @param[in]     wLayoutOffsetX    Layout offset to transform X.
 * @param[in]     wLayoutOffsetY    Layout offset to transform Y.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL ET9KDB_SetKeyboardOffset(ET9KDBInfo          * const pKDBInfo,
                                              const ET9U16                wLayoutOffsetX,
                                              const ET9U16                wLayoutOffsetY)
{
    ET9STATUS eStatus;

    WLOG5(fprintf(pLogFile5, "ET9KDB_SetKeyboardOffset, x %u, y %u\n", wLayoutOffsetX, wLayoutOffsetY);)

    eStatus = __ET9KDB_BasicValidityCheck(pKDBInfo, 1);

    if (eStatus) {
        return eStatus;
    }

    pKDBInfo->Private.wLayoutOffsetX = wLayoutOffsetX;
    pKDBInfo->Private.wLayoutOffsetY = wLayoutOffsetY;

    return ET9STATUS_NONE;
}

/* ************************************************************************************************************** */
/* * PUBLIC - KDB LOAD ****************************************************************************************** */
/* ************************************************************************************************************** */

/*---------------------------------------------------------------------------*/
/** \internal
 * Check if a key region overlaps any existing key regions.
 *
 * @param pLayoutInfo      Xxx
 * @param pRegion          Xxx
 *
 * @return Non zero if overlap, otherwise zero.
 */

ET9INLINE static ET9BOOL ET9LOCALCALL __CheckKeyRegionOverlap(ET9KdbLayoutInfo  const * const pLayoutInfo,
                                                              ET9Region         const * const pRegion)
{
    ET9UINT                 nCount;
    ET9KdbAreaInfo    const *pArea;

    pArea = pLayoutInfo->pKeyAreas;
    for (nCount = pLayoutInfo->nKeyAreaCount; nCount; --nCount, ++pArea) {

        /* include all keys */

        if ((pRegion->wRight  >= pArea->sRegion.wLeft && pArea->sRegion.wRight  >= pRegion->wLeft) &&
            (pRegion->wBottom >= pArea->sRegion.wTop  && pArea->sRegion.wBottom >= pRegion->wTop)) {
            return 1;
        }
    }

    return 0;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Check if a key index is used.
 *
 * @param pLayoutInfo      Xxx
 * @param wKeyIndex        Xxx
 *
 * @return Non zero if used, otherwise zero.
 */

ET9INLINE static ET9BOOL ET9LOCALCALL __CheckIndexUsed(ET9KdbLayoutInfo  const * const pLayoutInfo,
                                                       const ET9U16                    wKeyIndex)
{
    ET9UINT                 nCount;
    ET9KdbAreaInfo    const *pArea;

    pArea = pLayoutInfo->pKeyAreas;
    for (nCount = pLayoutInfo->nKeyAreaCount; nCount; --nCount, ++pArea) {

        /* include all keys */

        if (pArea->wKeyIndex == wKeyIndex) {
            return 1;
        }
    }

    return 0;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Validate char values.
 *
 * @param psString         Xxx
 * @param nStrLen          Xxx
 *
 * @return Non zero if valid, otherwise zero.
 */

ET9INLINE static ET9BOOL ET9LOCALCALL __ValidateCharValues(ET9SYMB       const * const psString,
                                                           const ET9UINT               nStrLen)
{
    ET9UINT nCount;
    ET9SYMB const * psChar;

    psChar = psString;
    for (nCount = nStrLen; nCount; --nCount, ++psChar) {

        if (!*psChar) {
            return 0;
        }
    }

    return 1;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Assure chars in ambig set.
 *
 * @param[in]     pKDBInfo              Pointer to keyboard information structure.
 * @param[in]     nCharCount            Number of characters.
 * @param[in]     psChars               Array of characters.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

static ET9STATUS ET9LOCALCALL __AssureInAmbigSet(ET9KDBInfo         * const pKDBInfo,
                                                 const ET9UINT              nCharCount,
                                                 ET9SYMB      const * const psChars)
{
    ET9UINT nCount;
    ET9SYMB const *psSymb;

    ET9UINT nAddCount;
    ET9SYMB psAddChars[ET9_KDB_MAX_KEY_CHARS];

    ET9SYMB * const psDedupe = &pKDBInfo->Private.wm.xmlReader.psDedupe[0];

    /* look for adds */

    nAddCount = 0;

    psSymb = psChars;
    for (nCount = nCharCount; nCount; --nCount, ++psSymb) {

        const ET9SYMB sSymb = _ET9SymToLower(*psSymb, pKDBInfo->Private.wLocale);

        const ET9UINT nHash = sSymb % ET9_KDB_XML_MAX_DEDUPE_CHARS;

        if (!psDedupe[nHash]) {

            if (nAddCount >= ET9_KDB_MAX_KEY_CHARS) {
                return ET9STATUS_KDB_HAS_TOO_MANY_CHARS;
            }

            psDedupe[nHash] = sSymb;
            psAddChars[nAddCount++] = sSymb;
        }
        else if (psDedupe[nHash] == sSymb) {
        }
        else {

            ET9UINT nLook;

            for (nLook = nHash; psDedupe[nLook]; ) {
                ++nLook;
                if (nLook >= ET9_KDB_XML_MAX_DEDUPE_CHARS) {
                    nLook = 0;
                }

                if (psDedupe[nHash] == sSymb) {
                    break;
                }
            }

            if (!psDedupe[nHash]) {

                if (nAddCount >= ET9_KDB_MAX_KEY_CHARS) {
                    return ET9STATUS_KDB_HAS_TOO_MANY_CHARS;
                }

                psDedupe[nHash] = sSymb;
                psAddChars[nAddCount++] = sSymb;
            }
        }
    }

    if (!nAddCount) {
        return ET9STATUS_NONE;
    }

    /* actually add */

    {
        ET9KdbLayoutInfo * const pLayoutInfo = pKDBInfo->Private.pCurrLayoutInfo;
        ET9KdbAreaInfo * const pArea = &pLayoutInfo->pKeyAreas[pLayoutInfo->nKeyAreaCount - 1];

        const ET9UINT nCharsToMove = (ET9UINT)(pLayoutInfo->nCharPoolCount - pArea->nCharCount - (pArea->psChars - pLayoutInfo->psCharPool));

        if (pLayoutInfo->nCharPoolCount + nAddCount > ET9_KDB_MAX_POOL_CHARS) {
            return ET9STATUS_KDB_HAS_TOO_MANY_CHARS;
        }

        _ET9SymMove(&pArea->psChars[pArea->nCharCount + nAddCount], &pArea->psChars[pArea->nCharCount], nCharsToMove);

        _ET9SymCopy(&pArea->psChars[pArea->nCharCount], psAddChars, nAddCount);

        pArea->nCharCount += nAddCount;
        pLayoutInfo->nCharPoolCount += nAddCount;

        /* adjust active pointers */

        if (pArea->psShiftedChars) {
            pArea->psShiftedChars += nAddCount;
        }

        if (pArea->psMultitapChars) {
            pArea->psMultitapChars += nAddCount;
        }

        if (pArea->psMultitapShiftedChars) {
            pArea->psMultitapShiftedChars += nAddCount;
        }
    }

    /* done */

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/**
 * @brief Set keyboard properties, available when loading a dynamic keyboard.
 *
 * @param[in]     pKDBInfo              Pointer to keyboard information structure.
 * @param[in]     bMajorVersion         Major version.
 * @param[in]     bMinorVersion         Minor version.
 * @param[in]     bPrimaryID            Primary ID.
 * @param[in]     bSecondaryID          Secondary ID.
 * @param[in]     wLayoutWidth          Keyboard width.
 * @param[in]     wLayoutHeight         Keyboard height.
 * @param[in]     wTotalPages           Total number of pages in the the keyboard.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL ET9KDB_Load_SetProperties(ET9KDBInfo       * const pKDBInfo,
                                               const ET9U8              bMajorVersion,
                                               const ET9U8              bMinorVersion,
                                               const ET9U8              bPrimaryID,
                                               const ET9U8              bSecondaryID,
                                               const ET9U16             wLayoutWidth,
                                               const ET9U16             wLayoutHeight,
                                               const ET9U16             wTotalPages)
{
    ET9STATUS eStatus;

    WLOG5(fprintf(pLogFile5, "ET9KDB_Load_SetProperties, pKDBInfo = %p, wLayoutWidth %u, wLayoutHeight %u, wTotalPages %u\n", pKDBInfo, wLayoutWidth, wLayoutHeight, wTotalPages);)

    eStatus = __ET9KDB_LoadValidityCheck(pKDBInfo);

    if (eStatus) {
        return eStatus;
    }

    if (pKDBInfo->Private.eLoadState != ET9KDBLOADSTATES_START) {
        return ET9STATUS_KDB_WRONG_LOAD_STATE;
    }

    if (!wLayoutWidth || !wLayoutHeight) {
        return ET9STATUS_KDB_BAD_LAYOUT_SIZE;
    }

    if (!wTotalPages) {
        return ET9STATUS_KDB_BAD_PAGE_COUNT;
    }

    {
        ET9KdbLayoutInfo * const pLayoutInfo = pKDBInfo->Private.pCurrLayoutInfo;

        pLayoutInfo->bPrimaryID = bPrimaryID;
        pLayoutInfo->bSecondaryID = bSecondaryID;
        pLayoutInfo->bContentsMajor = bMajorVersion;
        pLayoutInfo->bContentsMinor = bMinorVersion;
        pLayoutInfo->wTotalPages = wTotalPages;
        pLayoutInfo->wLayoutWidth = wLayoutWidth;
        pLayoutInfo->wLayoutHeight = wLayoutHeight;
    }

    pKDBInfo->Private.eLoadState = ET9KDBLOADSTATES_HAS_PROPERTIES;

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/**
 * @brief Add a key to a keyboard, available when loading a dynamic keyboard.
 *
 * @param[in]     pKDBInfo              Pointer to keyboard information structure.
 * @param[in]     wKeyIndex             Key index - can be ET9_KDB_LOAD_UNDEF_VALUE for an automatic sequence 0-N.
 * @param[in]     eKeyType              Type of key, see ET9LOADKEYTYPE.
 * @param[in]     wLeft                 Left side of key reagion.
 * @param[in]     wTop                  Top of key reagion.
 * @param[in]     wRight                Right side of key reagion.
 * @param[in]     wBottom               Bottom of key reagion.
 * @param[in]     nCharCount            Number of characters.
 * @param[in]     psChars               Array of characters.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL ET9KDB_Load_AddKey(ET9KDBInfo          * const pKDBInfo,
                                        const ET9U16                wKeyIndex,
                                        const ET9LOADKEYTYPE        eKeyType,
                                        const ET9U16                wLeft,
                                        const ET9U16                wTop,
                                        const ET9U16                wRight,
                                        const ET9U16                wBottom,
                                        const ET9UINT               nCharCount,
                                        ET9SYMB       const * const psChars)
{
    ET9STATUS eStatus;

    WLOG5(fprintf(pLogFile5, "ET9KDB_Load_AddKey, pKDBInfo = %p, wKeyIndex %04x, eKeyType %u, top %04x (%c) [%3u %3u %3u %3u]\n", pKDBInfo, wKeyIndex, eKeyType, (psChars ? psChars[0] : 0), (char)(psChars && eKeyType != ET9LKT_FUNCTION ? psChars[0] : 0), wLeft, wTop, wRight, wBottom);)

    eStatus = __ET9KDB_LoadValidityCheck(pKDBInfo);

    if (eStatus) {
        return eStatus;
    }

    if (pKDBInfo->Private.eLoadState != ET9KDBLOADSTATES_HAS_PROPERTIES &&
        pKDBInfo->Private.eLoadState != ET9KDBLOADSTATES_HAS_KEY) {
        return ET9STATUS_KDB_WRONG_LOAD_STATE;
    }

    if (wLeft > wRight ||
        wTop > wBottom) {
        return ET9STATUS_KDB_KEY_BAD_REGION;
    }

    if (eKeyType >= ET9LKT_LAST ||
        !nCharCount ||
        !psChars) {
        return ET9STATUS_BAD_PARAM;
    }

    if (eKeyType == ET9LKT_FUNCTION && nCharCount > 1) {
        return ET9STATUS_BAD_PARAM;
    }

    if (wRight >= pKDBInfo->Private.pCurrLayoutInfo->wLayoutWidth ||
        wBottom >= pKDBInfo->Private.pCurrLayoutInfo->wLayoutHeight) {
        return ET9STATUS_KDB_KEY_OUTSIDE_KEYBOARD;
    }

    if (eKeyType != ET9LKT_FUNCTION && !__ValidateCharValues(psChars, nCharCount)) {
        return ET9STATUS_INVALID_TEXT;
    }

    {
        ET9KdbLayoutInfo * const pLayoutInfo = pKDBInfo->Private.pCurrLayoutInfo;

        if (pLayoutInfo->nKeyAreaCount >= ET9_KDB_MAX_KEYS) {
            return ET9STATUS_KDB_HAS_TOO_MANY_KEYS;
        }

        if (nCharCount > ET9_KDB_XML_MAX_DEDUPE_CHARS) {
            return ET9STATUS_KDB_KEY_HAS_TOO_MANY_CHARS;
        }

        if (pLayoutInfo->nCharPoolCount + nCharCount > ET9_KDB_MAX_POOL_CHARS) {
            return ET9STATUS_KDB_HAS_TOO_MANY_CHARS;
        }

        {
            ET9KdbAreaInfo * const pArea = &pLayoutInfo->pKeyAreas[pLayoutInfo->nKeyAreaCount];

            if (wKeyIndex == ET9_KDB_LOAD_UNDEF_VALUE) {
                pArea->wKeyIndex = (ET9U16)pLayoutInfo->nKeyAreaCount;
            }
            else {
                pArea->wKeyIndex = wKeyIndex;
            }

            if (__CheckIndexUsed(pLayoutInfo, pArea->wKeyIndex)) {
                return ET9STATUS_KDB_KEY_INDEX_ALREADY_USED;
            }

            pArea->bRegionOk = 1;

            pArea->sRegion.wLeft = wLeft;
            pArea->sRegion.wTop = wTop;
            pArea->sRegion.wRight = wRight;
            pArea->sRegion.wBottom = wBottom;

            if (__CheckKeyRegionOverlap(pLayoutInfo, &pArea->sRegion)) {
                return ET9STATUS_KDB_KEY_OVERLAP;
            }

            pArea->nCenterX = (wLeft + wRight) / 2;
            pArea->nCenterY = (wTop + wBottom) / 2;

            {
                ET9UINT nCount;
                ET9SYMB const *psSymb;

                ET9SYMB * const psDedupe = &pKDBInfo->Private.wm.xmlReader.psDedupe[0];

                _ET9ClearMem((ET9U8*)psDedupe, ET9_KDB_XML_MAX_DEDUPE_CHARS * sizeof(ET9SYMB));

                psSymb = psChars;
                for (nCount = nCharCount; nCount; --nCount, ++psSymb) {

                    const ET9UINT nHash = *psSymb % ET9_KDB_XML_MAX_DEDUPE_CHARS;

                    if (!psDedupe[nHash]) {
                        psDedupe[nHash] = *psSymb;
                    }
                    else if (psDedupe[nHash] == *psSymb) {
                        return ET9STATUS_KDB_KEY_HAS_REPEAT_CHARS;
                    }
                    else {

                        ET9UINT nLook;

                        for (nLook = nHash; psDedupe[nLook]; ) {
                            ++nLook;
                            if (nLook >= ET9_KDB_XML_MAX_DEDUPE_CHARS) {
                                nLook = 0;
                            }

                            if (psDedupe[nHash] == *psSymb) {
                                return ET9STATUS_KDB_KEY_HAS_REPEAT_CHARS;
                            }
                        }

                        psDedupe[nLook] = *psSymb;
                    }
                }
            }

            pArea->psChars = &pLayoutInfo->psCharPool[pLayoutInfo->nCharPoolCount];

            _ET9SymCopy(pArea->psChars, psChars, nCharCount);

            pArea->nCharCount = nCharCount;
            pLayoutInfo->nCharPoolCount += nCharCount;

            pArea->nShiftedCharCount = 0;
            pArea->psShiftedChars = NULL;
            pArea->nMultitapCharCount = 0;
            pArea->psMultitapChars = NULL;
            pArea->nMultitapShiftedCharCount = 0;
            pArea->psMultitapShiftedChars = NULL;

            {
                ET9UINT nIndex;
                ET9UINT nBestClass;
                ET9SymbClass eBestClass;
                ET9BOOL bHasBPMF = 0;
                ET9UINT nClassCount[ET9_LastSymbClass + 1] = { 0 };

                for (nIndex = 0; nIndex < 5 && nIndex < pArea->nCharCount; ++nIndex) {

                    const ET9SYMB sSymb = pArea->psChars[nIndex];

                    ++nClassCount[_ET9_GetSymbolClass(sSymb)];

                    if (sSymb >= 0x3105 && sSymb <= 0x312D) {
                        bHasBPMF = 1;
                    }
                }

                nBestClass = ET9_LastSymbClass;
                for (nIndex = 0; nIndex < ET9_LastSymbClass; ++nIndex) {
                    if (nClassCount[nBestClass] < nClassCount[nIndex]) {
                        nBestClass = nIndex;
                    }
                }

                eBestClass = (ET9SymbClass)nBestClass;

                if (eKeyType == ET9LKT_SMARTPUNCT) {

                    if (eBestClass != ET9_PunctSymbClass && eBestClass != ET9_NumbrSymbClass && !bHasBPMF) {
                        return ET9STATUS_KDB_INCORRECT_TYPE_FOR_KEY;
                    }

                    pArea->eKeyType = ET9KTSMARTPUNCT;
                    pArea->eInputType = ET9DISCRETEKEY;
                }
                else if (eKeyType == ET9LKT_REGIONAL) {

                    if (eBestClass == ET9_PunctSymbClass || eBestClass == ET9_NumbrSymbClass && !bHasBPMF) {
                        return ET9STATUS_KDB_INCORRECT_TYPE_FOR_KEY;
                    }

                    pArea->eKeyType = ET9KTLETTER;
                    pArea->eInputType = ET9REGIONALKEY;
                }
                else if (eKeyType == ET9LKT_NONREGIONAL) {

                    switch (eBestClass)
                    {
                        case ET9_PunctSymbClass:
                            pArea->eKeyType = ET9KTPUNCTUATION;
                            break;
                        case ET9_NumbrSymbClass:
                            pArea->eKeyType = ET9KTNUMBER;
                            break;
                        default:
                            pArea->eKeyType = ET9KTLETTER;
                            break;
                    }

                    pArea->eInputType = ET9DISCRETEKEY;
                }
                else if (eKeyType == ET9LKT_STRING) {

                    pArea->eKeyType = ET9KTSTRING;
                    pArea->eInputType = ET9DISCRETEKEY;
                }
                else if (eKeyType == ET9LKT_FUNCTION) {

                    pArea->eKeyType = ET9KTFUNCTION;
                    pArea->eInputType = ET9EXPLICITSYM;
                }
                else {
                    return ET9STATUS_ERROR;
                }
            }

            ++pLayoutInfo->nKeyAreaCount;
        }
    }

    pKDBInfo->Private.eLoadState = ET9KDBLOADSTATES_HAS_KEY;

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/**
 * @brief Attach shifted symbols to a key, available when loading a dynamic keyboard. Optional, only needed for non default shifted sequence.
 *
 * If the list is empty then this is a no-op (will not store an empty list).
 *
 * @param[in]     pKDBInfo                      Pointer to keyboard information structure.
 * @param[in]     nShiftedCharCount             Number of shifted characters.
 * @param[in]     psShiftedChars                Array of shifted characters.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL ET9KDB_Load_AttachShiftedChars(ET9KDBInfo          * const pKDBInfo,
                                                    const ET9UINT               nShiftedCharCount,
                                                    ET9SYMB       const * const psShiftedChars)
{
    ET9STATUS eStatus;

    WLOG5(fprintf(pLogFile5, "ET9KDB_Load_AttachShiftedChars, pKDBInfo = %p\n", pKDBInfo);)

    eStatus = __ET9KDB_LoadValidityCheck(pKDBInfo);

    if (eStatus) {
        return eStatus;
    }

    if (pKDBInfo->Private.eLoadState != ET9KDBLOADSTATES_HAS_KEY) {
        return ET9STATUS_KDB_WRONG_LOAD_STATE;
    }

    if (!nShiftedCharCount) {
        return ET9STATUS_NONE;
    }

    if (!psShiftedChars) {
        return ET9STATUS_BAD_PARAM;
    }

    if (!__ValidateCharValues(psShiftedChars, nShiftedCharCount)) {
        return ET9STATUS_INVALID_TEXT;
    }

    {
        ET9KdbLayoutInfo * const pLayoutInfo = pKDBInfo->Private.pCurrLayoutInfo;
        ET9KdbAreaInfo * const pArea = &pLayoutInfo->pKeyAreas[pLayoutInfo->nKeyAreaCount - 1];

        if (pArea->eKeyType == ET9KTFUNCTION) {
            return ET9STATUS_KDB_UNEXPECTED_ATTRIBUTE;
        }

        if (pArea->nShiftedCharCount) {
            return ET9STATUS_KDB_REPEAT_LOAD_ATTACH;
        }

        eStatus = __AssureInAmbigSet(pKDBInfo, nShiftedCharCount, psShiftedChars);

        if (eStatus) {
            return eStatus;
        }

        if (pLayoutInfo->nCharPoolCount + nShiftedCharCount > ET9_KDB_MAX_POOL_CHARS) {
            return ET9STATUS_KDB_HAS_TOO_MANY_CHARS;
        }

        pArea->psShiftedChars = &pLayoutInfo->psCharPool[pLayoutInfo->nCharPoolCount];

        _ET9SymCopy(pArea->psShiftedChars, psShiftedChars, nShiftedCharCount);

        pArea->nShiftedCharCount = nShiftedCharCount;
        pLayoutInfo->nCharPoolCount += nShiftedCharCount;
    }

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/**
 * @brief Attach multitap symbols to a key, available when loading a dynamic keyboard. Optional, only needed for non default multitap sequence.
 *
 * If both lists are empty then this is a no-op (will not store two empty lists).
 *
 * @param[in]     pKDBInfo                      Pointer to keyboard information structure.
 * @param[in]     nCharMultitapCount            Number of multitap characters.
 * @param[in]     psMultitapChars               Array of multitap characters.
 * @param[in]     nCharMultitapShiftedCount     Number of shifted multitap characters (optional, only needed when normal case conversion isn't applicable).
 * @param[in]     psMultitapShiftedChars        Array of shifted multitap characters (optional, only needed when normal case conversion isn't applicable).
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL ET9KDB_Load_AttachMultitapInfo(ET9KDBInfo          * const pKDBInfo,
                                                    const ET9UINT               nCharMultitapCount,
                                                    ET9SYMB       const * const psMultitapChars,
                                                    const ET9UINT               nCharMultitapShiftedCount,
                                                    ET9SYMB       const * const psMultitapShiftedChars)
{
    ET9STATUS eStatus;

    WLOG5(fprintf(pLogFile5, "ET9KDB_Load_AttachMultitapInfo, pKDBInfo = %p\n", pKDBInfo);)

    eStatus = __ET9KDB_LoadValidityCheck(pKDBInfo);

    if (eStatus) {
        return eStatus;
    }

    if (pKDBInfo->Private.eLoadState != ET9KDBLOADSTATES_HAS_KEY) {
        return ET9STATUS_KDB_WRONG_LOAD_STATE;
    }

    if (!nCharMultitapCount && !nCharMultitapShiftedCount) {
        return ET9STATUS_NONE;
    }

    if (!nCharMultitapCount ||
        !psMultitapChars) {
        return ET9STATUS_BAD_PARAM;
    }

    if (nCharMultitapShiftedCount && !psMultitapShiftedChars) {
        return ET9STATUS_BAD_PARAM;
    }

    if (!__ValidateCharValues(psMultitapChars, nCharMultitapCount)) {
        return ET9STATUS_INVALID_TEXT;
    }

    if (!__ValidateCharValues(psMultitapShiftedChars, nCharMultitapShiftedCount)) {
        return ET9STATUS_INVALID_TEXT;
    }

    {
        ET9KdbLayoutInfo * const pLayoutInfo = pKDBInfo->Private.pCurrLayoutInfo;
        ET9KdbAreaInfo * const pArea = &pLayoutInfo->pKeyAreas[pLayoutInfo->nKeyAreaCount - 1];

        if (pArea->eKeyType == ET9KTFUNCTION) {
            return ET9STATUS_KDB_UNEXPECTED_ATTRIBUTE;
        }

        if (pArea->nMultitapCharCount) {
            return ET9STATUS_KDB_REPEAT_LOAD_ATTACH;
        }

        eStatus = __AssureInAmbigSet(pKDBInfo, nCharMultitapCount, psMultitapChars);

        if (eStatus) {
            return eStatus;
        }

        eStatus = __AssureInAmbigSet(pKDBInfo, nCharMultitapShiftedCount, psMultitapShiftedChars);

        if (eStatus) {
            return eStatus;
        }

        if (pLayoutInfo->nCharPoolCount + nCharMultitapCount + nCharMultitapShiftedCount > ET9_KDB_MAX_POOL_CHARS) {
            return ET9STATUS_KDB_HAS_TOO_MANY_CHARS;
        }

        pArea->psMultitapChars = &pLayoutInfo->psCharPool[pLayoutInfo->nCharPoolCount];

        _ET9SymCopy(pArea->psMultitapChars, psMultitapChars, nCharMultitapCount);

        pArea->nMultitapCharCount = nCharMultitapCount;
        pLayoutInfo->nCharPoolCount += nCharMultitapCount;

        if (nCharMultitapShiftedCount) {

            pArea->psMultitapShiftedChars = &pLayoutInfo->psCharPool[pLayoutInfo->nCharPoolCount];

            _ET9SymCopy(pArea->psMultitapShiftedChars, psMultitapShiftedChars, nCharMultitapShiftedCount);

            pArea->nMultitapShiftedCharCount = nCharMultitapShiftedCount;
            pLayoutInfo->nCharPoolCount += nCharMultitapShiftedCount;
        }
    }

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/**
 * @brief Reset the dynamic keyboard (and start over with a new one), available when loading a dynamic keyboard.
 *
 * @param[in]     pKDBInfo              Pointer to keyboard information structure.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL ET9KDB_Load_Reset(ET9KDBInfo   * const pKDBInfo)
{
    ET9STATUS eStatus;

    WLOG5(fprintf(pLogFile5, "ET9KDB_Load_Reset, pKDBInfo = %p\n", pKDBInfo);)

    eStatus = __ET9KDB_LoadValidityCheck(pKDBInfo);

    if (eStatus) {
        return eStatus;
    }

    {
        ET9KdbLayoutInfo * const pLayoutInfo = pKDBInfo->Private.pCurrLayoutInfo;

        pKDBInfo->wTotalPages = 0;

        pKDBInfo->Private.pCurrLayoutInfo->wLayoutWidth = 0;
        pKDBInfo->Private.pCurrLayoutInfo->wLayoutHeight = 0;
        pKDBInfo->Private.eLoadState = ET9KDBLOADSTATES_START;

        pLayoutInfo->bDatabaseType = 0;
        pLayoutInfo->bLayoutVer = 0;
        pLayoutInfo->bPrimaryID = 0;
        pLayoutInfo->bSecondaryID = 0;
        pLayoutInfo->bSymbolClass = 15;
        pLayoutInfo->bContentsMajor = 0;
        pLayoutInfo->bContentsMinor = 0;
        pLayoutInfo->nRadius = 0;
        pLayoutInfo->nBoxWidth = 0;
        pLayoutInfo->nBoxHeight = 0;
        pLayoutInfo->nMinKeyWidth = 0;
        pLayoutInfo->nMinKeyHeight = 0;
        pLayoutInfo->nMedianKeyWidth = 0;
        pLayoutInfo->nMedianKeyHeight = 0;
        pLayoutInfo->nKeyAreaCount = 0;
        pLayoutInfo->nCharPoolCount = 0;
    }

    return ET9STATUS_NONE;
}

/* ************************************************************************************************************** */
/* * Classic Reader ********************************************************************************************* */
/* ************************************************************************************************************** */

/*---------------------------------------------------------------------------*/
/** \internal
 * Classic tokens
 */

typedef enum ET9_classic_token_e {

    ET9_classic_token_LeftBracket,
    ET9_classic_token_RightBracket,
    ET9_classic_token_Period,
    ET9_classic_token_Equal,

    ET9_classic_token_Layout,
    ET9_classic_token_Information,
    ET9_classic_token_Contents,
    ET9_classic_token_Major,
    ET9_classic_token_Minor,
    ET9_classic_token_Ver,
    ET9_classic_token_OEM,
    ET9_classic_token_ID,
    ET9_classic_token_Primary,
    ET9_classic_token_Secondary,
    ET9_classic_token_Width,
    ET9_classic_token_Height,
    ET9_classic_token_Total,
    ET9_classic_token_Pages,
    ET9_classic_token_Page,
    ET9_classic_token_Keys,
    ET9_classic_token_Key,
    ET9_classic_token_Left,
    ET9_classic_token_Top,
    ET9_classic_token_Right,
    ET9_classic_token_Bottom,
    ET9_classic_token_Type,
    ET9_classic_token_Chars,
    ET9_classic_token_Char,
    ET9_classic_token_Regional,
    ET9_classic_token_NonRegional,
    ET9_classic_token_SmartPunct,
    ET9_classic_token_String,
    ET9_classic_token_Function,
    ET9_classic_token_Symbol,
    ET9_classic_token_Bytes,
    ET9_classic_token_Class,

    ET9_classic_token_Number,

    ET9_classic_token_EOF,

    ET9_classic_token_Error,

    ET9_classic_token_Last
} ET9_classic_token;

/*---------------------------------------------------------------------------*/
/** \internal
 * Structure holding KDB classic reader information.
 */

typedef struct ET9KdbClassicReaderInfo_s {

    ET9KDBInfo                  *pKDBInfo;                          /**< \internal */

    ET9_classic_token           eCurrToken;                         /**< \internal */
    ET9UINT                     nCurrNumber;                        /**< \internal */

    ET9UINT                     nCurrLine;                          /**< \internal */
    ET9UINT                     nObjectLine;                        /**< \internal */

    ET9U8               const * pbEndChar;                          /**< \internal */
    ET9U8               const * pbCurrChar;                         /**< \internal */

    ET9U8               const * pbContent;                          /**< \internal */
    ET9U32                      dwContentLen;                       /**< \internal */

    ET9U16                      wTotalPages;                        /**< \internal */
} ET9KdbClassicReaderInfo;                                          /**< \internal */

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pReaderInfo           Reader context.
 *
 * @return Zero on failure, otherwise non zero.
 */

static ET9BOOL ET9LOCALCALL __ClassicReader_SkipUntilLeftBracket(ET9KdbClassicReaderInfo  * const pReaderInfo)
{
    if (pReaderInfo->eCurrToken == ET9_classic_token_Error) {
        return 0;
    }

    if (pReaderInfo->eCurrToken == ET9_classic_token_EOF) {
        pReaderInfo->eCurrToken = ET9_classic_token_Error;
        return 0;
    }

    /* until left bracket */

    while (pReaderInfo->pbCurrChar < pReaderInfo->pbEndChar) {

        if (*pReaderInfo->pbCurrChar == '/' && (pReaderInfo->pbCurrChar + 1) < pReaderInfo->pbEndChar && *(pReaderInfo->pbCurrChar + 1) == '/') {

            pReaderInfo->pbCurrChar += 2;

            while (pReaderInfo->pbCurrChar < pReaderInfo->pbEndChar && *pReaderInfo->pbCurrChar != 0xA && *pReaderInfo->pbCurrChar != 0xD) {
                ++pReaderInfo->pbCurrChar;
            }
        }
        else if (*pReaderInfo->pbCurrChar == 0xA && (pReaderInfo->pbCurrChar + 1) < pReaderInfo->pbEndChar && *(pReaderInfo->pbCurrChar + 1) == 0xD) {
            pReaderInfo->pbCurrChar += 2;
            ++pReaderInfo->nCurrLine;
        }
        else if (*pReaderInfo->pbCurrChar == 0xD && (pReaderInfo->pbCurrChar + 1) < pReaderInfo->pbEndChar && *(pReaderInfo->pbCurrChar + 1) == 0xA) {
            pReaderInfo->pbCurrChar += 2;
            ++pReaderInfo->nCurrLine;
        }
        else if (*pReaderInfo->pbCurrChar == 0xA) {
            ++pReaderInfo->pbCurrChar;
            ++pReaderInfo->nCurrLine;
        }
        else if (*pReaderInfo->pbCurrChar == 0xD) {
            ++pReaderInfo->pbCurrChar;
            ++pReaderInfo->nCurrLine;
        }
        else if (*pReaderInfo->pbCurrChar == '[') {
            pReaderInfo->eCurrToken = ET9_classic_token_LeftBracket;
            ++pReaderInfo->pbCurrChar;
            return 1;
        }
        else {
            ++pReaderInfo->pbCurrChar;
        }
    }

    /* not found */

    pReaderInfo->eCurrToken = ET9_classic_token_EOF;
    return 1;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pReaderInfo           Reader context.
 *
 * @return Zero on failure, otherwise non zero.
 */

static ET9BOOL ET9LOCALCALL __ClassicReader_GetNextToken(ET9KdbClassicReaderInfo  * const pReaderInfo)
{
    if (pReaderInfo->eCurrToken == ET9_classic_token_Error) {
        return 0;
    }

    if (pReaderInfo->eCurrToken == ET9_classic_token_EOF) {
        pReaderInfo->eCurrToken = ET9_classic_token_Error;
        return 0;
    }

    /* white space and comments */

    while (pReaderInfo->pbCurrChar < pReaderInfo->pbEndChar &&
           ((*pReaderInfo->pbCurrChar <= 0x20) ||
            (*pReaderInfo->pbCurrChar == '/' && (pReaderInfo->pbCurrChar + 1) < pReaderInfo->pbEndChar && *(pReaderInfo->pbCurrChar + 1) == '/'))) {

        if (*pReaderInfo->pbCurrChar == '/' && (pReaderInfo->pbCurrChar + 1) < pReaderInfo->pbEndChar && *(pReaderInfo->pbCurrChar + 1) == '/') {

            pReaderInfo->pbCurrChar += 2;

            while (pReaderInfo->pbCurrChar < pReaderInfo->pbEndChar && *pReaderInfo->pbCurrChar != 0xA && *pReaderInfo->pbCurrChar != 0xD) {
                ++pReaderInfo->pbCurrChar;
            }
        }
        else if (*pReaderInfo->pbCurrChar == 0xA && (pReaderInfo->pbCurrChar + 1) < pReaderInfo->pbEndChar && *(pReaderInfo->pbCurrChar + 1) == 0xD) {
            pReaderInfo->pbCurrChar += 2;
            ++pReaderInfo->nCurrLine;
        }
        else if (*pReaderInfo->pbCurrChar == 0xD && (pReaderInfo->pbCurrChar + 1) < pReaderInfo->pbEndChar && *(pReaderInfo->pbCurrChar + 1) == 0xA) {
            pReaderInfo->pbCurrChar += 2;
            ++pReaderInfo->nCurrLine;
        }
        else if (*pReaderInfo->pbCurrChar == 0xA) {
            ++pReaderInfo->pbCurrChar;
            ++pReaderInfo->nCurrLine;
        }
        else if (*pReaderInfo->pbCurrChar == 0xD) {
            ++pReaderInfo->pbCurrChar;
            ++pReaderInfo->nCurrLine;
        }
        else {
            ++pReaderInfo->pbCurrChar;
        }
    }

    if (pReaderInfo->pbCurrChar >= pReaderInfo->pbEndChar) {
        pReaderInfo->eCurrToken = ET9_classic_token_EOF;
        return 1;
    }

    /* delimiters */

    switch (*pReaderInfo->pbCurrChar)
    {
        case '[':
            pReaderInfo->eCurrToken = ET9_classic_token_LeftBracket;
            ++pReaderInfo->pbCurrChar;
            return 1;
        case ']':
            pReaderInfo->eCurrToken = ET9_classic_token_RightBracket;
            ++pReaderInfo->pbCurrChar;
            return 1;
        case '.':
            pReaderInfo->eCurrToken = ET9_classic_token_Period;
            ++pReaderInfo->pbCurrChar;
            return 1;
        case '=':
            pReaderInfo->eCurrToken = ET9_classic_token_Equal;
            ++pReaderInfo->pbCurrChar;
            return 1;
    }

    /* hexadecimal number */

    if (*pReaderInfo->pbCurrChar == '0' && (pReaderInfo->pbCurrChar + 1) < pReaderInfo->pbEndChar && (*(pReaderInfo->pbCurrChar + 1) == 'x' || *(pReaderInfo->pbCurrChar + 1) == 'X')) {

        pReaderInfo->pbCurrChar += 2;

        pReaderInfo->nCurrNumber = 0;

        while (pReaderInfo->pbCurrChar < pReaderInfo->pbEndChar &&
               (*pReaderInfo->pbCurrChar >= '0' && *pReaderInfo->pbCurrChar <= '9' ||
                *pReaderInfo->pbCurrChar >= 'a' && *pReaderInfo->pbCurrChar <= 'f' ||
                *pReaderInfo->pbCurrChar >= 'A' && *pReaderInfo->pbCurrChar <= 'F')) {

            pReaderInfo->nCurrNumber *= 16;

            if (*pReaderInfo->pbCurrChar >= '0' && *pReaderInfo->pbCurrChar <= '9') {
                pReaderInfo->nCurrNumber += *pReaderInfo->pbCurrChar - '0';
            }
            else if (*pReaderInfo->pbCurrChar >= 'a' && *pReaderInfo->pbCurrChar <= 'f') {
                pReaderInfo->nCurrNumber += (*pReaderInfo->pbCurrChar - 'a') + 10;
            }
            else {
                pReaderInfo->nCurrNumber += (*pReaderInfo->pbCurrChar - 'A') + 10;
            }

            ++pReaderInfo->pbCurrChar;
        }

        pReaderInfo->eCurrToken = ET9_classic_token_Number;
        return 1;
    }

    /* decimal number */

    if (*pReaderInfo->pbCurrChar >= '0' && *pReaderInfo->pbCurrChar <= '9') {

        pReaderInfo->nCurrNumber = 0;

        while (pReaderInfo->pbCurrChar < pReaderInfo->pbEndChar && *pReaderInfo->pbCurrChar >= '0' && *pReaderInfo->pbCurrChar <= '9') {
            pReaderInfo->nCurrNumber *= 10;
            pReaderInfo->nCurrNumber += *pReaderInfo->pbCurrChar - '0';
            ++pReaderInfo->pbCurrChar;
        }

        pReaderInfo->eCurrToken = ET9_classic_token_Number;
        return 1;
    }

    /* keywords & function values */

    {
        ET9UINT nLen;
        ET9U32 dwHashValue;

        nLen = 0;
        dwHashValue = 0;

        while (pReaderInfo->pbCurrChar < pReaderInfo->pbEndChar &&
               (*pReaderInfo->pbCurrChar >= 'a' && *pReaderInfo->pbCurrChar <= 'z' ||
                *pReaderInfo->pbCurrChar >= 'A' && *pReaderInfo->pbCurrChar <= 'Z' ||
                *pReaderInfo->pbCurrChar >= '0' && *pReaderInfo->pbCurrChar <= '9' ||
                *pReaderInfo->pbCurrChar == '_')) {

            ET9U8 bChar = *pReaderInfo->pbCurrChar;

            ++nLen;
            ++pReaderInfo->pbCurrChar;

            if (nLen >= 32) {
                pReaderInfo->eCurrToken = ET9_classic_token_Error;
                return 0;
            }

            if (bChar >= 'A' && bChar <= 'Z') {
                bChar = bChar + 0x20;
            }

            dwHashValue = bChar + (65599 * dwHashValue);
        }

        if (nLen) {

            pReaderInfo->eCurrToken = ET9_classic_token_Function;

            switch (dwHashValue)
            {
                case 0x00691a3b: pReaderInfo->eCurrToken = ET9_classic_token_ID; return 1;
                case 0x3515943f: pReaderInfo->eCurrToken = ET9_classic_token_Key; return 1;
                case 0x370dd237: pReaderInfo->eCurrToken = ET9_classic_token_OEM; return 1;
                case 0x398e2235: pReaderInfo->eCurrToken = ET9_classic_token_Top; return 1;
                case 0x3a803ec3: pReaderInfo->eCurrToken = ET9_classic_token_Ver; return 1;
                case 0x31ba1e96: pReaderInfo->eCurrToken = ET9_classic_token_Char; return 1;
                case 0xa48e7bf4: pReaderInfo->eCurrToken = ET9_classic_token_Keys; return 1;
                case 0xd3024807: pReaderInfo->eCurrToken = ET9_classic_token_Left; return 1;
                case 0x8b264d2f: pReaderInfo->eCurrToken = ET9_classic_token_Page; return 1;
                case 0x511c067a: pReaderInfo->eCurrToken = ET9_classic_token_Type; return 1;
                case 0x5b63875d: pReaderInfo->eCurrToken = ET9_classic_token_Chars; return 1;
                case 0xc15b12f9: pReaderInfo->eCurrToken = ET9_classic_token_Major; return 1;
                case 0x3789d6f5: pReaderInfo->eCurrToken = ET9_classic_token_Minor; return 1;
                case 0x8b9bff04: pReaderInfo->eCurrToken = ET9_classic_token_Pages; return 1;
                case 0x87ae43bc: pReaderInfo->eCurrToken = ET9_classic_token_Right; return 1;
                case 0x2d0fae04: pReaderInfo->eCurrToken = ET9_classic_token_Total; return 1;
                case 0xd9ddf326: pReaderInfo->eCurrToken = ET9_classic_token_Width; return 1;
                case 0x3f1f244b: pReaderInfo->eCurrToken = ET9_classic_token_Bottom; return 1;
                case 0x16b4f247: pReaderInfo->eCurrToken = ET9_classic_token_Height; return 1;
                case 0x4c1f866a: pReaderInfo->eCurrToken = ET9_classic_token_Layout; return 1;
                case 0x6e1b7ba2: pReaderInfo->eCurrToken = ET9_classic_token_Primary; return 1;
                case 0xcc711e1a: pReaderInfo->eCurrToken = ET9_classic_token_Contents; return 1;
                case 0x479f9c3f: pReaderInfo->eCurrToken = ET9_classic_token_Regional; return 1;
                case 0x26d43a74: pReaderInfo->eCurrToken = ET9_classic_token_Secondary; return 1;
                case 0xf2728211: pReaderInfo->eCurrToken = ET9_classic_token_SmartPunct; return 1;
                case 0xeeaf320c: pReaderInfo->eCurrToken = ET9_classic_token_Information; return 1;
                case 0xf6f5c76c: pReaderInfo->eCurrToken = ET9_classic_token_NonRegional; return 1;
                case 0xa9362831: pReaderInfo->eCurrToken = ET9_classic_token_String; return 1;
                case 0xef6067b8: pReaderInfo->eCurrToken = ET9_classic_token_Function; return 1;
                case 0x10c08338: pReaderInfo->eCurrToken = ET9_classic_token_Symbol; return 1;
                case 0x37b9286b: pReaderInfo->eCurrToken = ET9_classic_token_Bytes; return 1;
                case 0x157fca98: pReaderInfo->eCurrToken = ET9_classic_token_Class; return 1;

                case 0x6d81286d: pReaderInfo->nCurrNumber = ET9KEY_INVALID        ; return 1;
                case 0xf3dd08c3: pReaderInfo->nCurrNumber = ET9KEY_RELOAD         ; return 1;
                case 0x1a794346: pReaderInfo->nCurrNumber = ET9KEY_OK             ; return 1;
                case 0xeebac284: pReaderInfo->nCurrNumber = ET9KEY_CANCEL         ; return 1;
                case 0x005d4a91: pReaderInfo->nCurrNumber = ET9KEY_LEFT           ; return 1;
                case 0x1a7f44c5: pReaderInfo->nCurrNumber = ET9KEY_UP             ; return 1;
                case 0xb39de3b2: pReaderInfo->nCurrNumber = ET9KEY_RIGHT          ; return 1;
                case 0x912463cc: pReaderInfo->nCurrNumber = ET9KEY_DOWN           ; return 1;
                case 0x2d1de451: pReaderInfo->nCurrNumber = ET9KEY_BACK           ; return 1;
                case 0xc989d9ab: pReaderInfo->nCurrNumber = ET9KEY_TAB            ; return 1;
                case 0x1a3a33bb: pReaderInfo->nCurrNumber = ET9KEY_0A             ; return 1;
                case 0xf3a47d98: pReaderInfo->nCurrNumber = ET9KEY_PREVTAB        ; return 1;
                case 0x4355a423: pReaderInfo->nCurrNumber = ET9KEY_CLEAR          ; return 1;
                case 0x6b18effa: pReaderInfo->nCurrNumber = ET9KEY_RETURN         ; return 1;
                case 0x028a8134: pReaderInfo->nCurrNumber = ET9KEY_CLOSE_SEL_LIST ; return 1;
                case 0x2eec1d49: pReaderInfo->nCurrNumber = ET9KEY_MENU           ; return 1;
                case 0xc9fd9078: pReaderInfo->nCurrNumber = ET9KEY_SHIFT          ; return 1;
                case 0xd644d753: pReaderInfo->nCurrNumber = ET9KEY_CONTROL        ; return 1;
                case 0xc039b5df: pReaderInfo->nCurrNumber = ET9KEY_ALT            ; return 1;
                case 0xbe7e7b6c: pReaderInfo->nCurrNumber = ET9KEY_PAUSE          ; return 1;
                case 0x1317e91f: pReaderInfo->nCurrNumber = ET9KEY_CAPS_LOCK      ; return 1;
                case 0x0dd0267f: pReaderInfo->nCurrNumber = ET9KEY_OPTION         ; return 1;
                case 0x999ad590: pReaderInfo->nCurrNumber = ET9KEY_EMOTICON       ; return 1;
                case 0x66c9903d: pReaderInfo->nCurrNumber = ET9KEY_ACCENTEDLAYOUT ; return 1;
                case 0xc507242c: pReaderInfo->nCurrNumber = ET9KEY_SYMBOLLAYOUT   ; return 1;
                case 0xc64933cd: pReaderInfo->nCurrNumber = ET9KEY_DIGITLAYOUT    ; return 1;
                case 0xd8cdf85a: pReaderInfo->nCurrNumber = ET9KEY_PUNCTLAYOUT    ; return 1;
                case 0xf1622fed: pReaderInfo->nCurrNumber = ET9KEY_MAINLAYOUT     ; return 1;
                case 0x294bb7f4: pReaderInfo->nCurrNumber = ET9KEY_MULTITAP       ; return 1;
                case 0xe0f61e8b: pReaderInfo->nCurrNumber = ET9KEY_ESCAPE         ; return 1;
                case 0xcf859d20: pReaderInfo->nCurrNumber = ET9KEY_PRIOR          ; return 1;
                case 0x5d7cf07d: pReaderInfo->nCurrNumber = ET9KEY_NEXT           ; return 1;
                case 0xc233f451: pReaderInfo->nCurrNumber = ET9KEY_END            ; return 1;
                case 0x4b35a449: pReaderInfo->nCurrNumber = ET9KEY_HOME           ; return 1;
                case 0x3a40999c: pReaderInfo->nCurrNumber = ET9KEY_SPACE          ; return 1;
                case 0xa933f762: pReaderInfo->nCurrNumber = ET9KEY_LANGUAGE       ; return 1;
                case 0xa7872c4e: pReaderInfo->nCurrNumber = ET9KEY_UNDO           ; return 1;
                case 0x17842e88: pReaderInfo->nCurrNumber = ET9KEY_REDO           ; return 1;
                case 0x4838450c: pReaderInfo->nCurrNumber = ET9KEY_HIDE           ; return 1;
                case 0x22b5f979: pReaderInfo->nCurrNumber = ET9KEY_WINDOWS        ; return 1;
                case 0x0e411a5d: pReaderInfo->nCurrNumber = ET9KEY_SOFT1          ; return 1;
                case 0x0e411a5e: pReaderInfo->nCurrNumber = ET9KEY_SOFT2          ; return 1;
                case 0x0e411a5f: pReaderInfo->nCurrNumber = ET9KEY_SOFT3          ; return 1;
                case 0x0e411a60: pReaderInfo->nCurrNumber = ET9KEY_SOFT4          ; return 1;
                case 0x1c5785da: pReaderInfo->nCurrNumber = ET9KEY_OEMLAYOUT1     ; return 1;
                case 0x1c5785db: pReaderInfo->nCurrNumber = ET9KEY_OEMLAYOUT2     ; return 1;
                case 0x1c5785dc: pReaderInfo->nCurrNumber = ET9KEY_OEMLAYOUT3     ; return 1;
                case 0x1c5785dd: pReaderInfo->nCurrNumber = ET9KEY_OEMLAYOUT4     ; return 1;
                case 0xd7ccb173: pReaderInfo->nCurrNumber = ET9KEY_OEM_01         ; return 1;
                case 0xd7ccb174: pReaderInfo->nCurrNumber = ET9KEY_OEM_02         ; return 1;
                case 0xd7ccb175: pReaderInfo->nCurrNumber = ET9KEY_OEM_03         ; return 1;
                case 0xd7ccb176: pReaderInfo->nCurrNumber = ET9KEY_OEM_04         ; return 1;
                case 0xd7ccb177: pReaderInfo->nCurrNumber = ET9KEY_OEM_05         ; return 1;
                case 0xd7ccb178: pReaderInfo->nCurrNumber = ET9KEY_OEM_06         ; return 1;
                case 0xd7ccb179: pReaderInfo->nCurrNumber = ET9KEY_OEM_07         ; return 1;
                case 0xd7ccb17a: pReaderInfo->nCurrNumber = ET9KEY_OEM_08         ; return 1;
                case 0xd7ccb17b: pReaderInfo->nCurrNumber = ET9KEY_OEM_09         ; return 1;
                case 0xd7ccb1a3: pReaderInfo->nCurrNumber = ET9KEY_OEM_0A         ; return 1;
                case 0xd7ccb1a4: pReaderInfo->nCurrNumber = ET9KEY_OEM_0B         ; return 1;
                case 0xd7ccb1a5: pReaderInfo->nCurrNumber = ET9KEY_OEM_0C         ; return 1;
                case 0xd7ccb1a6: pReaderInfo->nCurrNumber = ET9KEY_OEM_0D         ; return 1;
                case 0xd7ccb1a7: pReaderInfo->nCurrNumber = ET9KEY_OEM_0E         ; return 1;
                case 0xd7ccb1a8: pReaderInfo->nCurrNumber = ET9KEY_OEM_0F         ; return 1;

                default:
                    pReaderInfo->eCurrToken = ET9_classic_token_Error;
                    return 0;
            }
        }
    }

    /* nothing recognized */

    pReaderInfo->eCurrToken = ET9_classic_token_Error;

    return 0;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pReaderInfo           Reader context.
 * @param eToken                Xxx
 *
 * @return Zero on failure, otherwise non zero.
 */

ET9INLINE static ET9BOOL ET9LOCALCALL __ClassicReader_ConsumeToken(ET9KdbClassicReaderInfo    * const pReaderInfo,
                                                                   const ET9_classic_token            eToken)
{
    if (pReaderInfo->eCurrToken != eToken) {
        return 0;
    }

    if (!__ClassicReader_GetNextToken(pReaderInfo)) {
        return 0;
    }

    return 1;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pReaderInfo           Reader context.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

static ET9STATUS ET9LOCALCALL __ClassicReader_ReadProperties(ET9KdbClassicReaderInfo  * const pReaderInfo)
{
    ET9U8   bPrimaryID     = 0;
    ET9U8   bSecondaryID   = 0;
    ET9U8   bContentsMajor = 0;
    ET9U8   bContentsMinor = 0;
    ET9U16  wLayoutWidth   = 0;
    ET9U16  wLayoutHeight  = 0;

    if (pReaderInfo->eCurrToken == ET9_classic_token_Error) {
        return ET9STATUS_KDB_SYNTAX_ERROR;
    }

    while (pReaderInfo->eCurrToken != ET9_classic_token_LeftBracket) {
        if (!__ClassicReader_GetNextToken(pReaderInfo)) {
            return ET9STATUS_KDB_SYNTAX_ERROR;
        }
    }

    if (!(__ClassicReader_ConsumeToken(pReaderInfo, ET9_classic_token_LeftBracket) &&
          __ClassicReader_ConsumeToken(pReaderInfo, ET9_classic_token_Layout) &&
          __ClassicReader_ConsumeToken(pReaderInfo, ET9_classic_token_Information) &&
          __ClassicReader_ConsumeToken(pReaderInfo, ET9_classic_token_RightBracket))) {
        return ET9STATUS_KDB_SYNTAX_ERROR;
    }

    while (pReaderInfo->eCurrToken != ET9_classic_token_LeftBracket) {

        switch (pReaderInfo->eCurrToken)
        {
            case ET9_classic_token_Contents:
                __ClassicReader_GetNextToken(pReaderInfo);
                switch (pReaderInfo->eCurrToken)
                {
                    case ET9_classic_token_Major:
                        if (!(__ClassicReader_ConsumeToken(pReaderInfo, ET9_classic_token_Major) &&
                              __ClassicReader_ConsumeToken(pReaderInfo, ET9_classic_token_Ver) &&
                              __ClassicReader_ConsumeToken(pReaderInfo, ET9_classic_token_Equal) &&
                              __ClassicReader_ConsumeToken(pReaderInfo, ET9_classic_token_Number))) {
                            return ET9STATUS_KDB_SYNTAX_ERROR;
                        }
                        bContentsMajor = (ET9U8)pReaderInfo->nCurrNumber;
                        if (bContentsMajor != 3) {
                            return ET9STATUS_KDB_VERSION_ERROR;
                        }
                        break;
                    case ET9_classic_token_Minor:
                        if (!(__ClassicReader_ConsumeToken(pReaderInfo, ET9_classic_token_Minor) &&
                              __ClassicReader_ConsumeToken(pReaderInfo, ET9_classic_token_Ver) &&
                              __ClassicReader_ConsumeToken(pReaderInfo, ET9_classic_token_Equal) &&
                              __ClassicReader_ConsumeToken(pReaderInfo, ET9_classic_token_Number))) {
                            return ET9STATUS_KDB_SYNTAX_ERROR;
                        }
                        bContentsMinor = (ET9U8)pReaderInfo->nCurrNumber;
                        break;
                    default:
                        return ET9STATUS_KDB_SYNTAX_ERROR;
                }
                break;
            case ET9_classic_token_OEM:
                if (!(__ClassicReader_ConsumeToken(pReaderInfo, ET9_classic_token_OEM) &&
                      __ClassicReader_ConsumeToken(pReaderInfo, ET9_classic_token_ID) &&
                      __ClassicReader_ConsumeToken(pReaderInfo, ET9_classic_token_Equal) &&
                      __ClassicReader_ConsumeToken(pReaderInfo, ET9_classic_token_Number))) {
                    return ET9STATUS_KDB_SYNTAX_ERROR;
                }
                break;
            case ET9_classic_token_Symbol:
                __ClassicReader_GetNextToken(pReaderInfo);
                switch (pReaderInfo->eCurrToken)
                {
                    case ET9_classic_token_Bytes:
                        if (!(__ClassicReader_ConsumeToken(pReaderInfo, ET9_classic_token_Bytes) &&
                              __ClassicReader_ConsumeToken(pReaderInfo, ET9_classic_token_Equal) &&
                              __ClassicReader_ConsumeToken(pReaderInfo, ET9_classic_token_Number))) {
                            return ET9STATUS_KDB_SYNTAX_ERROR;
                        }
                        if (pReaderInfo->nCurrNumber != 2) {
                            return ET9STATUS_KDB_ATTRIBUTE_VALUE_ERROR;
                        }
                        break;
                    case ET9_classic_token_Class:
                        if (!(__ClassicReader_ConsumeToken(pReaderInfo, ET9_classic_token_Class) &&
                              __ClassicReader_ConsumeToken(pReaderInfo, ET9_classic_token_Equal) &&
                              __ClassicReader_ConsumeToken(pReaderInfo, ET9_classic_token_Number))) {
                            return ET9STATUS_KDB_SYNTAX_ERROR;
                        }
                        if (pReaderInfo->nCurrNumber != 15) {
                            return ET9STATUS_KDB_ATTRIBUTE_VALUE_ERROR;
                        }
                        break;
                    default:
                        return ET9STATUS_KDB_SYNTAX_ERROR;
                }
                break;
            case ET9_classic_token_Primary:
                if (!(__ClassicReader_ConsumeToken(pReaderInfo, ET9_classic_token_Primary) &&
                      __ClassicReader_ConsumeToken(pReaderInfo, ET9_classic_token_ID) &&
                      __ClassicReader_ConsumeToken(pReaderInfo, ET9_classic_token_Equal) &&
                      __ClassicReader_ConsumeToken(pReaderInfo, ET9_classic_token_Number))) {
                    return ET9STATUS_KDB_SYNTAX_ERROR;
                }
                bPrimaryID = (ET9U8)pReaderInfo->nCurrNumber;
                break;
            case ET9_classic_token_Secondary:
                if (!(__ClassicReader_ConsumeToken(pReaderInfo, ET9_classic_token_Secondary) &&
                      __ClassicReader_ConsumeToken(pReaderInfo, ET9_classic_token_ID) &&
                      __ClassicReader_ConsumeToken(pReaderInfo, ET9_classic_token_Equal) &&
                      __ClassicReader_ConsumeToken(pReaderInfo, ET9_classic_token_Number))) {
                    return ET9STATUS_KDB_SYNTAX_ERROR;
                }
                bSecondaryID = (ET9U8)pReaderInfo->nCurrNumber;
                break;
            case ET9_classic_token_Layout:
                __ClassicReader_GetNextToken(pReaderInfo);
                switch (pReaderInfo->eCurrToken)
                {
                    case ET9_classic_token_Width:
                        if (!(__ClassicReader_ConsumeToken(pReaderInfo, ET9_classic_token_Width) &&
                              __ClassicReader_ConsumeToken(pReaderInfo, ET9_classic_token_Equal) &&
                              __ClassicReader_ConsumeToken(pReaderInfo, ET9_classic_token_Number))) {
                            return ET9STATUS_KDB_SYNTAX_ERROR;
                        }
                        wLayoutWidth = (ET9U16)pReaderInfo->nCurrNumber;
                        break;
                    case ET9_classic_token_Height:
                        if (!(__ClassicReader_ConsumeToken(pReaderInfo, ET9_classic_token_Height) &&
                              __ClassicReader_ConsumeToken(pReaderInfo, ET9_classic_token_Equal) &&
                              __ClassicReader_ConsumeToken(pReaderInfo, ET9_classic_token_Number))) {
                            return ET9STATUS_KDB_SYNTAX_ERROR;
                        }
                        wLayoutHeight = (ET9U16)pReaderInfo->nCurrNumber;
                        break;
                    default:
                        return ET9STATUS_KDB_SYNTAX_ERROR;
                }
                break;
            case ET9_classic_token_Total:
                if (!(__ClassicReader_ConsumeToken(pReaderInfo, ET9_classic_token_Total) &&
                      __ClassicReader_ConsumeToken(pReaderInfo, ET9_classic_token_Pages) &&
                      __ClassicReader_ConsumeToken(pReaderInfo, ET9_classic_token_Equal) &&
                      __ClassicReader_ConsumeToken(pReaderInfo, ET9_classic_token_Number))) {
                    return ET9STATUS_KDB_SYNTAX_ERROR;
                }
                pReaderInfo->wTotalPages = (ET9U16)pReaderInfo->nCurrNumber;
                break;
            default:
                return ET9STATUS_KDB_SYNTAX_ERROR;
        }
    }

    {
        ET9STATUS eStatus;

        eStatus = ET9KDB_Load_SetProperties(pReaderInfo->pKDBInfo,
                                            bContentsMajor,
                                            bContentsMinor,
                                            bPrimaryID,
                                            bSecondaryID,
                                            wLayoutWidth,
                                            wLayoutHeight,
                                            pReaderInfo->wTotalPages);

        if (eStatus) {
            return eStatus;
        }
    }

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pReaderInfo           Reader context.
 * @param nKeyNumber            Key to read.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

static ET9STATUS ET9LOCALCALL __ClassicReader_ReadKey(ET9KdbClassicReaderInfo   * const pReaderInfo,
                                                      const ET9UINT                     nKeyNumber)
{
    const ET9UINT   nKeySourceLine = pReaderInfo->nCurrLine;

    ET9UINT         nCount;
    ET9U16          wLeft = 0xFFFF;
    ET9U16          wTop = 0xFFFF;
    ET9U16          wRight = 0xFFFF;
    ET9U16          wBottom = 0xFFFF;
    ET9LOADKEYTYPE  eKeyType;
    ET9UINT         nCharCount;
    ET9SYMB         psChars[ET9_KDB_MAX_KEY_CHARS];

    for (nCount = 4; nCount; --nCount) {

        ET9_classic_token eToken;

        if (!(__ClassicReader_ConsumeToken(pReaderInfo, ET9_classic_token_Key) &&
              __ClassicReader_ConsumeToken(pReaderInfo, ET9_classic_token_Number) &&
              pReaderInfo->nCurrNumber == nKeyNumber &&
              __ClassicReader_ConsumeToken(pReaderInfo, (eToken = pReaderInfo->eCurrToken)) &&
              __ClassicReader_ConsumeToken(pReaderInfo, ET9_classic_token_Equal) &&
              __ClassicReader_ConsumeToken(pReaderInfo, ET9_classic_token_Number))) {
            return ET9STATUS_KDB_SYNTAX_ERROR;
        }

        switch (eToken)
        {
            case ET9_classic_token_Left:
                wLeft = (ET9U16)pReaderInfo->nCurrNumber;
                break;
            case ET9_classic_token_Top:
                wTop = (ET9U16)pReaderInfo->nCurrNumber;
                break;
            case ET9_classic_token_Right:
                wRight = (ET9U16)pReaderInfo->nCurrNumber;
                break;
            case ET9_classic_token_Bottom:
                wBottom = (ET9U16)pReaderInfo->nCurrNumber;
                break;
            default:
                return ET9STATUS_KDB_SYNTAX_ERROR;
        }
    }

    if (wLeft == 0xFFFF || wTop == 0xFFFF || wRight == 0xFFFF || wBottom == 0xFFFF) {
        return ET9STATUS_KDB_SYNTAX_ERROR;
    }

    if (!(__ClassicReader_ConsumeToken(pReaderInfo, ET9_classic_token_Key) &&
          __ClassicReader_ConsumeToken(pReaderInfo, ET9_classic_token_Number) &&
          pReaderInfo->nCurrNumber == nKeyNumber &&
          __ClassicReader_ConsumeToken(pReaderInfo, ET9_classic_token_Type) &&
          __ClassicReader_ConsumeToken(pReaderInfo, ET9_classic_token_Equal))) {
        return ET9STATUS_KDB_SYNTAX_ERROR;
    }

    switch (pReaderInfo->eCurrToken)
    {
        case ET9_classic_token_Regional:
            eKeyType = ET9LKT_REGIONAL;
            break;
        case ET9_classic_token_NonRegional:
            eKeyType = ET9LKT_NONREGIONAL;
            break;
        case ET9_classic_token_SmartPunct:
            eKeyType = ET9LKT_SMARTPUNCT;
            break;
        case ET9_classic_token_String:
            eKeyType = ET9LKT_STRING;
            break;
        case ET9_classic_token_Function:
            eKeyType = ET9LKT_FUNCTION;
            break;
        default:
            return ET9STATUS_KDB_SYNTAX_ERROR;
    }

    __ClassicReader_GetNextToken(pReaderInfo);

    if (!(__ClassicReader_ConsumeToken(pReaderInfo, ET9_classic_token_Key) &&
          __ClassicReader_ConsumeToken(pReaderInfo, ET9_classic_token_Number) &&
          pReaderInfo->nCurrNumber == nKeyNumber &&
          __ClassicReader_ConsumeToken(pReaderInfo, ET9_classic_token_Total) &&
          __ClassicReader_ConsumeToken(pReaderInfo, ET9_classic_token_Chars) &&
          __ClassicReader_ConsumeToken(pReaderInfo, ET9_classic_token_Equal) &&
          __ClassicReader_ConsumeToken(pReaderInfo, ET9_classic_token_Number))) {
        return ET9STATUS_KDB_SYNTAX_ERROR;
    }

    nCharCount = pReaderInfo->nCurrNumber;

    if (!nCharCount) {
        return ET9STATUS_KDB_KEY_HAS_TOO_FEW_CHARS;
    }

    if (nCharCount > ET9_KDB_MAX_KEY_CHARS) {
        return ET9STATUS_KDB_KEY_HAS_TOO_MANY_CHARS;
    }

    if (eKeyType == ET9LKT_FUNCTION && nCharCount > 1) {
        return ET9STATUS_KDB_KEY_HAS_TOO_MANY_CHARS;
    }

    {
        ET9UINT nIndex;

        for (nIndex = 0; nIndex < nCharCount; ++nIndex) {

            if (!(__ClassicReader_ConsumeToken(pReaderInfo, ET9_classic_token_Key) &&
                  __ClassicReader_ConsumeToken(pReaderInfo, ET9_classic_token_Number) &&
                  pReaderInfo->nCurrNumber == nKeyNumber &&
                  __ClassicReader_ConsumeToken(pReaderInfo, ET9_classic_token_Char) &&
                  __ClassicReader_ConsumeToken(pReaderInfo, ET9_classic_token_Number) &&
                  pReaderInfo->nCurrNumber == nIndex &&
                  __ClassicReader_ConsumeToken(pReaderInfo, ET9_classic_token_Equal) &&
                  (pReaderInfo->eCurrToken == ET9_classic_token_Number || pReaderInfo->eCurrToken == ET9_classic_token_Function))) {
                return ET9STATUS_KDB_SYNTAX_ERROR;
            }

            if (eKeyType != ET9LKT_FUNCTION && pReaderInfo->eCurrToken == ET9_classic_token_Function) {
                return ET9STATUS_KDB_SYNTAX_ERROR;
            }

            psChars[nIndex] = (ET9SYMB)pReaderInfo->nCurrNumber;

            if (!__ClassicReader_ConsumeToken(pReaderInfo, pReaderInfo->eCurrToken)) {
                return ET9STATUS_KDB_SYNTAX_ERROR;
            }
        }
    }

    {
        ET9STATUS eStatus;

        eStatus = ET9KDB_Load_AddKey(pReaderInfo->pKDBInfo,
                                     ET9_KDB_LOAD_UNDEF_VALUE,  /* key index */
                                     eKeyType,
                                     wLeft,
                                     wTop,
                                     wRight,
                                     wBottom,
                                     nCharCount,
                                     psChars);

        if (eStatus) {
            pReaderInfo->nObjectLine = nKeySourceLine;
            return eStatus;
        }
    }

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pReaderInfo           Reader context.
 * @param wPageNum              Page to read.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

static ET9STATUS ET9LOCALCALL __ClassicReader_ReadPage(ET9KdbClassicReaderInfo  * const pReaderInfo,
                                                       const ET9U16                     wPageNum)
{
    ET9UINT nTotalKeys;

    if (pReaderInfo->eCurrToken == ET9_classic_token_Error) {
        return ET9STATUS_KDB_SYNTAX_ERROR;
    }

    if (wPageNum >= pReaderInfo->wTotalPages) {
        return ET9STATUS_INVALID_KDB_PAGE;
    }

    for (;;) {

        while (pReaderInfo->eCurrToken != ET9_classic_token_LeftBracket) {
            if (!__ClassicReader_SkipUntilLeftBracket(pReaderInfo)) {
                return ET9STATUS_KDB_PAGE_NOT_FOUND;
            }
        }

        if (!(__ClassicReader_ConsumeToken(pReaderInfo, ET9_classic_token_LeftBracket) &&
              __ClassicReader_ConsumeToken(pReaderInfo, ET9_classic_token_Page) &&
              __ClassicReader_ConsumeToken(pReaderInfo, ET9_classic_token_Number))) {
            return ET9STATUS_KDB_SYNTAX_ERROR;
        }

        if (pReaderInfo->nCurrNumber == wPageNum) {
            break;
        }

        if (pReaderInfo->nCurrNumber > wPageNum) {
            return ET9STATUS_KDB_PAGE_NOT_FOUND;
        }
    }

    if (!(__ClassicReader_ConsumeToken(pReaderInfo, ET9_classic_token_Period) &&
          __ClassicReader_ConsumeToken(pReaderInfo, ET9_classic_token_Number) &&
          __ClassicReader_ConsumeToken(pReaderInfo, ET9_classic_token_RightBracket))) {
        return ET9STATUS_KDB_SYNTAX_ERROR;
    }

    if (!(__ClassicReader_ConsumeToken(pReaderInfo, ET9_classic_token_Total) &&
          __ClassicReader_ConsumeToken(pReaderInfo, ET9_classic_token_Keys) &&
          __ClassicReader_ConsumeToken(pReaderInfo, ET9_classic_token_Equal) &&
          __ClassicReader_ConsumeToken(pReaderInfo, ET9_classic_token_Number))) {
        return ET9STATUS_KDB_SYNTAX_ERROR;
    }

    nTotalKeys = (ET9U16)pReaderInfo->nCurrNumber;

    {
        ET9STATUS   eStatus;
        ET9UINT     nIndex;

        for (nIndex = 0; nIndex < nTotalKeys; ++nIndex) {

            eStatus = __ClassicReader_ReadKey(pReaderInfo, nIndex);

            if (eStatus) {
                return eStatus;
            }
        }
    }

    if (pReaderInfo->eCurrToken != ET9_classic_token_LeftBracket && pReaderInfo->eCurrToken != ET9_classic_token_EOF) {
        return ET9STATUS_KDB_UNEXPECTED_CONTENT;
    }

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/**
 * @brief Load a dynamically using the simplified format for KDB descriptions.
 *
 * @param[in]     pKDBInfo            Pointer to keyboard information structure.
 * @param[in]     wPageNum            The target page number.
 * @param[in]     pbContent           Pointer to the content.
 * @param[in]     dwContentLen        Content length.
 * @param[out]    pnErrorLine         If an error occurs, this is an indication to where it happened (optional).
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL ET9KDB_Load_TextKDB(ET9KDBInfo             * const pKDBInfo,
                                         const ET9U16                   wPageNum,
                                         ET9U8            const * const pbContent,
                                         const ET9U32                   dwContentLen,
                                         ET9UINT                * const pnErrorLine)
{
    ET9STATUS eStatus;

    WLOG5(fprintf(pLogFile5, "ET9KDB_Load_TextKDB, pKDBInfo = %p, wPageNum %u\n", pKDBInfo, wPageNum);)

    eStatus = __ET9KDB_LoadValidityCheck(pKDBInfo);

    if (eStatus) {
        return eStatus;
    }

    if (!pbContent ||
        !dwContentLen) {
        return ET9STATUS_BAD_PARAM;
    }

    if (dwContentLen > 1 && (pbContent[0] == 0xFF && pbContent[1] == 0xFE) || (pbContent[0] == 0xFE && pbContent[1] == 0xFF)) {
        return ET9STATUS_KDB_WRONG_CONTENT_ENCODING;
    }

    if (pnErrorLine) {
        *pnErrorLine = 0;
    }

    {
        ET9KdbClassicReaderInfo sReaderInfo;

        _ET9ClearMem((ET9U8*)&sReaderInfo, sizeof(sReaderInfo));

        sReaderInfo.pKDBInfo = pKDBInfo;
        sReaderInfo.nCurrLine = 1;
        sReaderInfo.pbEndChar = &pbContent[dwContentLen];
        sReaderInfo.pbCurrChar = pbContent;
        sReaderInfo.pbContent = pbContent;
        sReaderInfo.dwContentLen = dwContentLen;
        sReaderInfo.eCurrToken = ET9_classic_token_Last;

        if (!__ClassicReader_GetNextToken(&sReaderInfo)) {
            return ET9STATUS_KDB_SYNTAX_ERROR;
        }

        eStatus = __ClassicReader_ReadProperties(&sReaderInfo);

        if (eStatus) {

            if (pnErrorLine) {
                *pnErrorLine = sReaderInfo.nObjectLine ? sReaderInfo.nObjectLine : sReaderInfo.nCurrLine;
            }

            return eStatus;
        }

        eStatus = __ClassicReader_ReadPage(&sReaderInfo, wPageNum);

        if (eStatus) {

            if (pnErrorLine) {
                *pnErrorLine = sReaderInfo.nObjectLine ? sReaderInfo.nObjectLine : sReaderInfo.nCurrLine;
            }

            return eStatus;
        }

        if (sReaderInfo.eCurrToken == ET9_classic_token_Error) {

            WLOG5(fprintf(pLogFile5, "  error @ line %u\n", sReaderInfo.nCurrLine);)

            if (pnErrorLine) {
                *pnErrorLine = sReaderInfo.nObjectLine ? sReaderInfo.nObjectLine : sReaderInfo.nCurrLine;
            }

            return ET9STATUS_KDB_SYNTAX_ERROR;
        }
    }

    return ET9STATUS_NONE;
}

/* ************************************************************************************************************** */
/* * XML Reader ************************************************************************************************* */
/* ************************************************************************************************************** */

/*---------------------------------------------------------------------------*/
/** \internal
 * XML tokens
 */

typedef enum ET9_xml_token_e {

    ET9_xml_token_A_addAltChars,
    ET9_xml_token_A_conditionValue,
    ET9_xml_token_A_defaultLayoutWidth,
    ET9_xml_token_A_defaultLayoutHeight,
    ET9_xml_token_A_horizontalBorder,
    ET9_xml_token_A_horizontalGap,
    ET9_xml_token_A_keyTop,
    ET9_xml_token_A_keyLeft,
    ET9_xml_token_A_keyWidth,
    ET9_xml_token_A_keyHeight,
    ET9_xml_token_A_keyIcon,
    ET9_xml_token_A_keyCodes,
    ET9_xml_token_A_keyCodesShifted,
    ET9_xml_token_A_keyLabel,
    ET9_xml_token_A_keyLabelShifted,
    ET9_xml_token_A_keyMultitapChars,
    ET9_xml_token_A_keyMultitapShiftedChars,
    ET9_xml_token_A_keyMultitapCodes,
    ET9_xml_token_A_keyMultitapShiftedCodes,
    ET9_xml_token_A_keyName,
    ET9_xml_token_A_keyPopupChars,
    ET9_xml_token_A_keyPopupShiftedChars,
    ET9_xml_token_A_keyPopupCodes,
    ET9_xml_token_A_keyPopupShiftedCodes,
    ET9_xml_token_A_keyType,
    ET9_xml_token_A_majorVersion,
    ET9_xml_token_A_minorVersion,
    ET9_xml_token_A_verticalBorder,
    ET9_xml_token_A_verticalGap,
    ET9_xml_token_A_voidSize,
    ET9_xml_token_A_primaryId,
    ET9_xml_token_A_secondaryId,
    ET9_xml_token_A_supportsExact,

    ET9_xml_token_A_unknown,

    ET9_xml_token_AttributeCount,

    ET9_xml_token_TagStartKeyboard,
    ET9_xml_token_TagEndKeyboard,
    ET9_xml_token_TagStartArea,
    ET9_xml_token_TagEndArea,
    ET9_xml_token_TagStartRow,
    ET9_xml_token_TagEndRow,
    ET9_xml_token_TagStartKey,
    ET9_xml_token_TagStartVoid,

    ET9_xml_token_TagEnd,
    ET9_xml_token_TagEndFinal,

    ET9_xml_token_String,

    ET9_xml_token_Equal,

    ET9_xml_token_EOF,

    ET9_xml_token_Error,

    ET9_xml_token_Last

} ET9_xml_token;

/*---------------------------------------------------------------------------*/
/** \internal
 * XML constants
 */

typedef enum ET9_xml_constant_e {

    ET9_xml_C_regional,
    ET9_xml_C_nonRegional,
    ET9_xml_C_smartPunct,
    ET9_xml_C_string,
    ET9_xml_C_function,
    ET9_xml_C_true,
    ET9_xml_C_false,

    ET9_xml_C_Unknown,

    ET9_xml_C_Last

} ET9_xml_constant;

/*---------------------------------------------------------------------------*/
/** \internal
 * XML entities
 */

typedef enum ET9_xml_entity_e {

    ET9_xml_E_keyboard,
    ET9_xml_E_row,
    ET9_xml_E_key,

    ET9_xml_E_Count

} ET9_xml_entity;

/*---------------------------------------------------------------------------*/
/** \internal
 * Copy operation
 */

typedef enum ET9_xml_copyOp_e {

    ET9_xml_copyOp_none,
    ET9_xml_copyOp_toLower,

    ET9_xml_copyOp_Count

} ET9_xml_copyOp;

/*---------------------------------------------------------------------------*/
/** \internal
 * Structure holding KDB xml reader information.
 */

typedef struct ET9KdbXmlReaderContext_s {

    ET9U8                       bPrimaryID;                                     /**< \internal */
    ET9U8                       bSecondaryID;                                   /**< \internal */
    ET9U8                       bMajorVersion;                                  /**< \internal */
    ET9U8                       bMinorVersion;                                  /**< \internal */
    ET9U16                      wDefaultLayoutWidth;                            /**< \internal */
    ET9U16                      wDefaultLayoutHeight;                           /**< \internal */

    ET9_xml_booleanValue        eAddAltChars;                                   /**< \internal */
    ET9_xml_booleanValue        eSupportsExact;                                 /**< \internal */

    ET9FLOAT                    fKeyTop;                                        /**< \internal */
    ET9_xml_valueSuffix         eKeyTopSuffix;                                  /**< \internal */

    ET9FLOAT                    fKeyLeft;                                       /**< \internal */
    ET9_xml_valueSuffix         eKeyLeftSuffix;                                 /**< \internal */

    ET9FLOAT                    fKeyWidth;                                      /**< \internal */
    ET9_xml_valueSuffix         eKeyWidthSuffix;                                /**< \internal */

    ET9FLOAT                    fKeyHeight;                                     /**< \internal */
    ET9_xml_valueSuffix         eKeyHeightSuffix;                               /**< \internal */

    ET9FLOAT                    fVoidSize;                                      /**< \internal */
    ET9_xml_valueSuffix         eVoidSizeSuffix;                                /**< \internal */

    ET9FLOAT                    fVerticalBorder;                                /**< \internal */
    ET9_xml_valueSuffix         eVerticalBorderSuffix;                          /**< \internal */

    ET9FLOAT                    fHorizontalBorder;                              /**< \internal */
    ET9_xml_valueSuffix         eHorizontalBorderSuffix;                        /**< \internal */

    ET9FLOAT                    fVerticalGap;                                   /**< \internal */
    ET9_xml_valueSuffix         eVerticalGapSuffix;                             /**< \internal */

    ET9FLOAT                    fHorizontalGap;                                 /**< \internal */
    ET9_xml_valueSuffix         eHorizontalGapSuffix;                           /**< \internal */

    ET9UINT                     nConditionValue;                                /**< \internal */

    ET9_xml_keyType             eKeyType;                                       /**< \internal */
    ET9_xml_rowType             eRowType;                                       /**< \internal */

    ET9UINT                     nIconCharCount;                                 /**< \internal */
    ET9SYMB                     *psIconChars;                                   /**< \internal */

    ET9UINT                     nLabelCharCount;                                /**< \internal */
    ET9SYMB                     *psLabelChars;                                  /**< \internal */

    ET9UINT                     nLabelShiftedCharCount;                         /**< \internal */
    ET9SYMB                     *psLabelShiftedChars;                           /**< \internal */

    ET9UINT                     nKeyCharCount;                                  /**< \internal */
    ET9SYMB                     *psKeyChars;                                    /**< \internal */

    ET9UINT                     nKeyShiftedCharCount;                           /**< \internal */
    ET9SYMB                     *psKeyShiftedChars;                             /**< \internal */

    ET9UINT                     nKeyPopupCharCount;                             /**< \internal */
    ET9SYMB                     *psKeyPopupChars;                               /**< \internal */

    ET9UINT                     nKeyPopupShiftedCharCount;                      /**< \internal */
    ET9SYMB                     *psKeyPopupShiftedChars;                        /**< \internal */

    ET9UINT                     nKeyMultitapCharCount;                          /**< \internal */
    ET9SYMB                     *psKeyMultitapChars;                            /**< \internal */

    ET9UINT                     nKeyMultitapShiftedCharCount;                   /**< \internal */
    ET9SYMB                     *psKeyMultitapShiftedChars;                     /**< \internal */

    ET9BOOL                     pbAttributeSet[ET9_xml_token_AttributeCount];   /**< \internal */

} ET9KdbXmlReaderContext;                                                       /**< \internal */

/*---------------------------------------------------------------------------*/
/** \internal
 * Structure holding KDB xml reader information.
 */

typedef struct ET9KdbXmlReaderInfo_s {

    ET9KDBInfo                  *pKDBInfo;                                      /**< \internal */

    ET9_xml_token               eCurrToken;                                     /**< \internal */

    ET9UINT                     nCurrLine;                                      /**< \internal */

    ET9UINT                     nUnknownAttributes;                             /**< \internal */

    ET9U16                      wLayoutWidth;                                   /**< \internal */
    ET9U16                      wLayoutHeight;                                  /**< \internal */
    ET9UINT                     nConditionValue;                                /**< \internal */

    ET9_xml_token               eCurrAttribute;                                 /**< \internal */
    ET9_xml_constant            eCurrConstant;                                  /**< \internal */
    ET9UINT                     nCurrInt;                                       /**< \internal */
    ET9FLOAT                    fCurrFloat;                                     /**< \internal */
    ET9_xml_valueSuffix         eCurrValueSuffix;                               /**< \internal */
    ET9UINT                     nCurrCharCount;                                 /**< \internal */
    ET9SYMB                     psCurrChars[ET9_KDB_MAX_KEY_CHARS];             /**< \internal */

    ET9KdbXmlReaderContext      pContexts[ET9_xml_E_Count];                     /**< \internal */

    ET9U8               const * pbEndChar;                                      /**< \internal */
    ET9U8               const * pbCurrChar;                                     /**< \internal */

    ET9U8               const * pbContent;                                      /**< \internal */
    ET9U32                      dwContentLen;                                   /**< \internal */
} ET9KdbXmlReaderInfo;                                                          /**< \internal */

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param eConstant         .
 *
 * @return Converted value.
 */

static ET9_xml_keyType ET9LOCALCALL __XmlReader_ConstantToKeyType(const ET9_xml_constant eConstant)
{
    switch (eConstant)
    {
        case ET9_xml_C_regional:
            return ET9_xml_keyType_regional;
        case ET9_xml_C_nonRegional:
            return ET9_xml_keyType_nonRegional;
        case ET9_xml_C_smartPunct:
            return ET9_xml_keyType_smartPunct;
        case ET9_xml_C_string:
            return ET9_xml_keyType_string;
        case ET9_xml_C_function:
            return ET9_xml_keyType_function;

        default:
            ET9Assert(0);
            return ET9_xml_keyType_undef;
    }
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param eKeyType          .
 *
 * @return Converted value.
 */

static ET9LOADKEYTYPE ET9LOCALCALL __XmlReader_XmlKeyTypeToLKT(const ET9_xml_keyType eKeyType)
{
    switch (eKeyType)
    {
        case ET9_xml_keyType_regional:
            return ET9LKT_REGIONAL;
        case ET9_xml_keyType_nonRegional:
            return ET9LKT_NONREGIONAL;
        case ET9_xml_keyType_smartPunct:
            return ET9LKT_SMARTPUNCT;
        case ET9_xml_keyType_string:
            return ET9LKT_STRING;
        case ET9_xml_keyType_function:
            return ET9LKT_FUNCTION;

        default:
            ET9Assert(0);
            return ET9LKT_LAST;
    }
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param eConstant         .
 *
 * @return Converted value.
 */

static ET9_xml_booleanValue ET9LOCALCALL __XmlReader_ConstantToBooleanValue(const ET9_xml_constant eConstant)
{
    switch (eConstant)
    {
        case ET9_xml_C_true:
            return ET9_xml_booleanValue_yes;
        case ET9_xml_C_false:
            return ET9_xml_booleanValue_no;
        default:
            ET9Assert(0);
            return ET9_xml_booleanValue_undef;
    }
}

#ifdef ET9_DEBUGLOG5

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param eToken            .
 *
 * @return Zero on failure, otherwise non zero.
 */

static char const * ET9LOCALCALL __XmlReader_AttributeToString(const ET9_xml_token eToken)
{
    switch (eToken)
    {
        case ET9_xml_token_A_keyTop:                    return "keyTop";
        case ET9_xml_token_A_keyLeft:                   return "keyLeft";
        case ET9_xml_token_A_keyWidth:                  return "keyWidth";
        case ET9_xml_token_A_keyHeight:                 return "keyHeight";
        case ET9_xml_token_A_verticalBorder:            return "verticalBorder";
        case ET9_xml_token_A_horizontalBorder:          return "horizontalBorder";
        case ET9_xml_token_A_verticalGap:               return "verticalGap";
        case ET9_xml_token_A_horizontalGap:             return "horizontalGap";
        case ET9_xml_token_A_majorVersion:              return "majorVersion";
        case ET9_xml_token_A_minorVersion:              return "minorVersion";
        case ET9_xml_token_A_primaryId:                 return "primaryId";
        case ET9_xml_token_A_secondaryId:               return "secondaryId";
        case ET9_xml_token_A_supportsExact:             return "supportsExact";
        case ET9_xml_token_A_conditionValue:            return "conditionValue";
        case ET9_xml_token_A_keyType:                   return "keyType";
        case ET9_xml_token_A_keyIcon:                   return "keyIcon";
        case ET9_xml_token_A_keyName:                   return "keyName";
        case ET9_xml_token_A_keyLabel:                  return "keyLabel";
        case ET9_xml_token_A_keyLabelShifted:           return "keyLabelShifted";
        case ET9_xml_token_A_keyCodes:                  return "keyCodes";
        case ET9_xml_token_A_keyCodesShifted:           return "keyCodesShifted";
        case ET9_xml_token_A_keyPopupChars:             return "keyPopupChars";
        case ET9_xml_token_A_keyPopupShiftedChars:      return "keyPopupShiftedChars";
        case ET9_xml_token_A_keyPopupCodes:             return "keyPopupCodes";
        case ET9_xml_token_A_keyPopupShiftedCodes:      return "keyPopupShiftedCodes";
        case ET9_xml_token_A_keyMultitapChars:          return "keyMultitapChars";
        case ET9_xml_token_A_keyMultitapShiftedChars:   return "keyMultitapShiftedChars";
        case ET9_xml_token_A_keyMultitapCodes:          return "keyMultitapCodes";
        case ET9_xml_token_A_keyMultitapShiftedCodes:   return "keyMultitapShiftedCodes";
        case ET9_xml_token_A_voidSize:                  return "voidSize";
        case ET9_xml_token_A_unknown:                   return "<unknown>";
        default: return "<\?\?\?>";
    }
}

#endif

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pReaderInfo           Reader context.
 *
 * @return Zero on failure, otherwise non zero.
 */

static ET9BOOL ET9LOCALCALL __XmlReader_ExtractConstant(ET9KdbXmlReaderInfo  * const pReaderInfo)
{
    ET9KDBPrivate * const pPrivate = &pReaderInfo->pKDBInfo->Private;

    ET9SYMB *psSymb;
    ET9UINT nCount;
    ET9U32 dwHashValue;

    dwHashValue = 0;
    psSymb = &pPrivate->wm.xmlReader.psString[0];
    for (nCount = pPrivate->wm.xmlReader.nStringLen; nCount; --nCount, ++psSymb) {
        dwHashValue = *psSymb + (65599 * dwHashValue);
    }

    switch (dwHashValue)
    {
        case 0x479f9c3f: pReaderInfo->eCurrConstant = ET9_xml_C_regional; break;
        case 0x94bf8f8c: pReaderInfo->eCurrConstant = ET9_xml_C_nonRegional; break;
        case 0x74e6a1f1: pReaderInfo->eCurrConstant = ET9_xml_C_smartPunct; break;
        case 0xa9362831: pReaderInfo->eCurrConstant = ET9_xml_C_string; break;
        case 0xef6067b8: pReaderInfo->eCurrConstant = ET9_xml_C_function; break;
        case 0x4dae9b2e: pReaderInfo->eCurrConstant = ET9_xml_C_true; break;
        case 0xe6e499e3: pReaderInfo->eCurrConstant = ET9_xml_C_false; break;

        default:         pReaderInfo->eCurrConstant = ET9_xml_C_Unknown; break;
    }

    return 1;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pReaderInfo           Reader context.
 *
 * @return Zero on failure, otherwise non zero.
 */

static ET9BOOL ET9LOCALCALL __XmlReader_ExtractName(ET9KdbXmlReaderInfo  * const pReaderInfo)
{
    ET9KDBPrivate * const pPrivate = &pReaderInfo->pKDBInfo->Private;

    ET9SYMB *psSymb;
    ET9UINT nCount;
    ET9U32 dwHashValue;

    dwHashValue = 0;
    psSymb = &pPrivate->wm.xmlReader.psString[0];
    for (nCount = pPrivate->wm.xmlReader.nStringLen; nCount; --nCount, ++psSymb) {
        dwHashValue = *psSymb + (65599 * dwHashValue);
    }

    switch (dwHashValue)
    {
        case 0x421d802d: pReaderInfo->psCurrChars[0] = ET9KEY_INVALID        ; break;
        case 0xab4cb8e3: pReaderInfo->psCurrChars[0] = ET9KEY_RELOAD         ; break;
        case 0x40f92366: pReaderInfo->psCurrChars[0] = ET9KEY_OK             ; break;
        case 0xa62a72a4: pReaderInfo->psCurrChars[0] = ET9KEY_CANCEL         ; break;
        case 0x348f12b1: pReaderInfo->psCurrChars[0] = ET9KEY_LEFT           ; break;
        case 0x40ff24e5: pReaderInfo->psCurrChars[0] = ET9KEY_UP             ; break;
        case 0x53fe2372: pReaderInfo->psCurrChars[0] = ET9KEY_RIGHT          ; break;
        case 0xc5562bec: pReaderInfo->psCurrChars[0] = ET9KEY_DOWN           ; break;
        case 0x614fac71: pReaderInfo->psCurrChars[0] = ET9KEY_BACK           ; break;
        case 0x2322016b: pReaderInfo->psCurrChars[0] = ET9KEY_TAB            ; break;
        case 0x40da1bbb: pReaderInfo->psCurrChars[0] = ET9KEY_0A             ; break;
        case 0xc840d558: pReaderInfo->psCurrChars[0] = ET9KEY_PREVTAB        ; break;
        case 0xe3b5e3e3: pReaderInfo->psCurrChars[0] = ET9KEY_CLEAR          ; break;
        case 0x2288a01a: pReaderInfo->psCurrChars[0] = ET9KEY_RETURN         ; break;
        case 0xc72d7194: pReaderInfo->psCurrChars[0] = ET9KEY_CLOSE_SEL_LIST ; break;
        case 0x631de569: pReaderInfo->psCurrChars[0] = ET9KEY_MENU           ; break;
        case 0x6a5dd038: pReaderInfo->psCurrChars[0] = ET9KEY_SHIFT          ; break;
        case 0xaae12f13: pReaderInfo->psCurrChars[0] = ET9KEY_CONTROL        ; break;
        case 0x19d1dd9f: pReaderInfo->psCurrChars[0] = ET9KEY_ALT            ; break;
        case 0x5edebb2c: pReaderInfo->psCurrChars[0] = ET9KEY_PAUSE          ; break;
        case 0x0cf038ff: pReaderInfo->psCurrChars[0] = ET9KEY_CAPS_LOCK      ; break;
        case 0xc53fd69f: pReaderInfo->psCurrChars[0] = ET9KEY_OPTION         ; break;
        case 0x43d46db0: pReaderInfo->psCurrChars[0] = ET9KEY_EMOTICON       ; break;
        case 0x4aa8e05d: pReaderInfo->psCurrChars[0] = ET9KEY_ACCENTEDLAYOUT ; break;
        case 0x8b788c4c: pReaderInfo->psCurrChars[0] = ET9KEY_SYMBOLLAYOUT   ; break;
        case 0xe3b9bb8d: pReaderInfo->psCurrChars[0] = ET9KEY_DIGITLAYOUT    ; break;
        case 0xf63e801a: pReaderInfo->psCurrChars[0] = ET9KEY_PUNCTLAYOUT    ; break;
        case 0xf7f1b00d: pReaderInfo->psCurrChars[0] = ET9KEY_MAINLAYOUT     ; break;
        case 0xd3855014: pReaderInfo->psCurrChars[0] = ET9KEY_MULTITAP       ; break;
        case 0x9865ceab: pReaderInfo->psCurrChars[0] = ET9KEY_ESCAPE         ; break;
        case 0x6fe5dce0: pReaderInfo->psCurrChars[0] = ET9KEY_PRIOR          ; break;
        case 0x91aeb89d: pReaderInfo->psCurrChars[0] = ET9KEY_NEXT           ; break;
        case 0x1bcc1c11: pReaderInfo->psCurrChars[0] = ET9KEY_END            ; break;
        case 0x7f676c69: pReaderInfo->psCurrChars[0] = ET9KEY_HOME           ; break;
        case 0xdaa0d95c: pReaderInfo->psCurrChars[0] = ET9KEY_SPACE          ; break;
        case 0x536d8f82: pReaderInfo->psCurrChars[0] = ET9KEY_LANGUAGE       ; break;
        case 0xdbb8f46e: pReaderInfo->psCurrChars[0] = ET9KEY_UNDO           ; break;
        case 0x4bb5f6a8: pReaderInfo->psCurrChars[0] = ET9KEY_REDO           ; break;
        case 0x7c6a0d2c: pReaderInfo->psCurrChars[0] = ET9KEY_HIDE           ; break;
        case 0xf7525139: pReaderInfo->psCurrChars[0] = ET9KEY_WINDOWS        ; break;
        case 0xaea15a3d: pReaderInfo->psCurrChars[0] = ET9KEY_SOFT1          ; break;
        case 0xaea15a3e: pReaderInfo->psCurrChars[0] = ET9KEY_SOFT2          ; break;
        case 0xaea15a3f: pReaderInfo->psCurrChars[0] = ET9KEY_SOFT3          ; break;
        case 0xaea15a40: pReaderInfo->psCurrChars[0] = ET9KEY_SOFT4          ; break;
        case 0x22e7061a: pReaderInfo->psCurrChars[0] = ET9KEY_OEMLAYOUT1     ; break;
        case 0x22e7061b: pReaderInfo->psCurrChars[0] = ET9KEY_OEMLAYOUT2     ; break;
        case 0x22e7061c: pReaderInfo->psCurrChars[0] = ET9KEY_OEMLAYOUT3     ; break;
        case 0x22e7061d: pReaderInfo->psCurrChars[0] = ET9KEY_OEMLAYOUT4     ; break;
        case 0x9f1e59b3: pReaderInfo->psCurrChars[0] = ET9KEY_OEM_01         ; break;
        case 0x9f1e59b4: pReaderInfo->psCurrChars[0] = ET9KEY_OEM_02         ; break;
        case 0x9f1e59b5: pReaderInfo->psCurrChars[0] = ET9KEY_OEM_03         ; break;
        case 0x9f1e59b6: pReaderInfo->psCurrChars[0] = ET9KEY_OEM_04         ; break;
        case 0x9f1e59b7: pReaderInfo->psCurrChars[0] = ET9KEY_OEM_05         ; break;
        case 0x9f1e59b8: pReaderInfo->psCurrChars[0] = ET9KEY_OEM_06         ; break;
        case 0x9f1e59b9: pReaderInfo->psCurrChars[0] = ET9KEY_OEM_07         ; break;
        case 0x9f1e59ba: pReaderInfo->psCurrChars[0] = ET9KEY_OEM_08         ; break;
        case 0x9f1e59bb: pReaderInfo->psCurrChars[0] = ET9KEY_OEM_09         ; break;
        case 0x9f1e59c3: pReaderInfo->psCurrChars[0] = ET9KEY_OEM_0A         ; break;
        case 0x9f1e59c4: pReaderInfo->psCurrChars[0] = ET9KEY_OEM_0B         ; break;
        case 0x9f1e59c5: pReaderInfo->psCurrChars[0] = ET9KEY_OEM_0C         ; break;
        case 0x9f1e59c6: pReaderInfo->psCurrChars[0] = ET9KEY_OEM_0D         ; break;
        case 0x9f1e59c7: pReaderInfo->psCurrChars[0] = ET9KEY_OEM_0E         ; break;
        case 0x9f1e59c8: pReaderInfo->psCurrChars[0] = ET9KEY_OEM_0F         ; break;

        default:
            return 0;
    }

    pReaderInfo->nCurrCharCount = 1;

    return 1;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pReaderInfo           Reader context.
 * @param nStartPos             .
 * @param pnValue               .
 *
 * @return Number of consumed chars.
 */

static ET9UINT ET9LOCALCALL __XmlReader_ReadInt(ET9KdbXmlReaderInfo     * const pReaderInfo,
                                                const ET9UINT                   nStartPos,
                                                ET9UINT                 * const pnValue)
{
    ET9KDBPrivate * const pPrivate = &pReaderInfo->pKDBInfo->Private;

    ET9SYMB *psSymb;
    ET9UINT nCount;
    ET9UINT nValue;

    if (nStartPos >= pPrivate->wm.xmlReader.nStringLen) {
        return 0;
    }

    nCount = pPrivate->wm.xmlReader.nStringLen - nStartPos;
    psSymb = &pPrivate->wm.xmlReader.psString[nStartPos];

    if (nCount >= 2 && *psSymb == '0' && (*(psSymb + 1) == 'x' || *(psSymb + 1) == 'X')) {

        nCount -= 2;
        psSymb += 2;

        nValue = 0;
        for (; nCount; --nCount, ++psSymb) {
            if (*psSymb >= '0' && *psSymb <= '9') {
                nValue = nValue * 16 + (*psSymb - '0');
            }
            else if (*psSymb >= 'a' && *psSymb <= 'f') {
                nValue = nValue * 16 + (*psSymb - 'a' + 10);
            }
            else if (*psSymb >= 'A' && *psSymb <= 'F') {
                nValue = nValue * 16 + (*psSymb - 'A' + 10);
            }
            else {
                break;
            }
        }
    }
    else {

        nValue = 0;
        for (; nCount; --nCount, ++psSymb) {
            if (*psSymb >= '0' && *psSymb <= '9') {
                nValue = nValue * 10 + (*psSymb - '0');
            }
            else {
                break;
            }
        }
    }

    *pnValue = nValue;

    return (ET9UINT)(psSymb - &pPrivate->wm.xmlReader.psString[nStartPos]);
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pReaderInfo           Reader context.
 *
 * @return Zero on failure, otherwise non zero.
 */

static ET9BOOL ET9LOCALCALL __XmlReader_ExtractInt(ET9KdbXmlReaderInfo  * const pReaderInfo)
{
    ET9KDBPrivate * const pPrivate = &pReaderInfo->pKDBInfo->Private;

    ET9UINT nConsumedCount;

    if (!pPrivate->wm.xmlReader.nStringLen) {
        return 0;
    }

    nConsumedCount = __XmlReader_ReadInt(pReaderInfo, 0, &pReaderInfo->nCurrInt);

    if (nConsumedCount != pPrivate->wm.xmlReader.nStringLen) {
        return 0;
    }

    return 1;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pReaderInfo           Reader context.
 *
 * @return Zero on failure, otherwise non zero.
 */

static ET9BOOL ET9LOCALCALL __XmlReader_ExtractFloatWithSuffix(ET9KdbXmlReaderInfo  * const pReaderInfo)
{
    ET9KDBPrivate * const pPrivate = &pReaderInfo->pKDBInfo->Private;

    ET9SYMB *psSymb;
    ET9UINT nCount;
    ET9FLOAT fValue;
    ET9_xml_valueSuffix eSuffix;

    if (!pPrivate->wm.xmlReader.nStringLen) {
        return 0;
    }

    fValue = 0;
    psSymb = &pPrivate->wm.xmlReader.psString[0];
    for (nCount = pPrivate->wm.xmlReader.nStringLen; nCount; --nCount, ++psSymb) {
        if (*psSymb >= '0' && *psSymb <= '9') {
            fValue = fValue * 10 + (*psSymb - '0');
        }
        else {
            break;
        }
    }

    if (nCount == pPrivate->wm.xmlReader.nStringLen) {
        return 0;
    }

    if (nCount && *psSymb == '.') {

        const ET9UINT nStartCount = nCount;

        ET9FLOAT fDiv = 1;

        --nCount;
        ++psSymb;

        for (; nCount; --nCount, ++psSymb) {
            if (*psSymb >= '0' && *psSymb <= '9') {
                fDiv = fDiv * 10;
                fValue = fValue + ((ET9FLOAT)(*psSymb - '0') / fDiv);
            }
            else {
                break;
            }
        }

        if (nCount + 1 == nStartCount) {
            return 0;
        }
    }

    if (!nCount) {
        return 0;
    }

    if (nCount == 1 && *psSymb == '%') {
        eSuffix = ET9_xml_valueSuffix_percent;
    }
    else if (nCount == 2 && *psSymb == 'd' && *(psSymb + 1) == 'p') {
        eSuffix = ET9_xml_valueSuffix_pixelsDI;
    }
    else {
        return 0;
    }

    pReaderInfo->fCurrFloat = fValue;
    pReaderInfo->eCurrValueSuffix = eSuffix;

    return 1;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pReaderInfo           Reader context.
 *
 * @return Zero on failure, otherwise non zero.
 */

static ET9BOOL ET9LOCALCALL __XmlReader_ExtractChars(ET9KdbXmlReaderInfo  * const pReaderInfo)
{
    ET9KDBPrivate * const pPrivate = &pReaderInfo->pKDBInfo->Private;

    if (pPrivate->wm.xmlReader.nStringLen > ET9_KDB_MAX_KEY_CHARS) {
        return 0;
    }

    _ET9SymCopy(pReaderInfo->psCurrChars, pPrivate->wm.xmlReader.psString, pPrivate->wm.xmlReader.nStringLen);

    pReaderInfo->nCurrCharCount = pPrivate->wm.xmlReader.nStringLen;

    return 1;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pReaderInfo           Reader context.
 *
 * @return Zero on failure, otherwise non zero.
 */

static ET9BOOL ET9LOCALCALL __XmlReader_ExtractCodes(ET9KdbXmlReaderInfo  * const pReaderInfo)
{
    ET9KDBPrivate * const pPrivate = &pReaderInfo->pKDBInfo->Private;

    const ET9UINT nTotCount = pPrivate->wm.xmlReader.nStringLen;

    ET9UINT nTotConsumedCount;

    pReaderInfo->nCurrCharCount = 0;

    for (nTotConsumedCount = 0; nTotConsumedCount < nTotCount;) {

        ET9UINT nValue;
        ET9UINT nConsumedCount;

        if (nTotConsumedCount) {
            if (pPrivate->wm.xmlReader.psString[nTotConsumedCount] == ',') {
                ++nTotConsumedCount;
            }
            else {
                return 0;
            }
        }

        while (nTotConsumedCount < nTotCount && pPrivate->wm.xmlReader.psString[nTotConsumedCount] <= 0x20) {
            ++nTotConsumedCount;
        }

        nConsumedCount = __XmlReader_ReadInt(pReaderInfo, nTotConsumedCount, &nValue);

        if (!nConsumedCount) {
            return 0;
        }

        if (!nValue || nValue > 0xFFFF) {
            return 0;
        }

        if (pReaderInfo->nCurrCharCount >= ET9_KDB_MAX_KEY_CHARS) {
            return 0;
        }

        pReaderInfo->psCurrChars[pReaderInfo->nCurrCharCount++] = (ET9SYMB)nValue;

        nTotConsumedCount += nConsumedCount;
    }

    if (nTotConsumedCount != nTotCount) {
            return 0;
    }

    return 1;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pbFirst               .
 * @param pbLast                .
 * @param psString              .
 * @param nMaxStringLen         .
 * @param pnStringLen           .
 *
 * @return Zero on failure, otherwise non zero.
 */

static ET9BOOL ET9LOCALCALL __XmlReader_DecodeString(ET9U8        const * const pbFirst,
                                                     ET9U8        const * const pbLast,
                                                     ET9SYMB            * const psString,
                                                     const ET9UINT              nMaxStringLen,
                                                     ET9UINT            * const pnStringLen)
{
    ET9SYMB  * const psEnd = &psString[nMaxStringLen];
    ET9U8      const *pbCurr;
    ET9SYMB          *psChar;

    *pnStringLen = 0;

    psChar = &psString[0];
    for (pbCurr = pbFirst; pbCurr <= pbLast;) {

        ET9U8 bCharsConsumed;

        if (psChar + 1 >= psEnd) {
            return 0;
        }

        if (*pbCurr == '&') {
            bCharsConsumed = _ET9DecodeSpecialChar(pbCurr, pbLast + 1, psChar);
        }
        else if (*pbCurr >= 0xC0) {
            bCharsConsumed = _ET9Utf8ToSymb(pbCurr, pbLast + 1, psChar);
        }
        else {
            *psChar = *pbCurr;
            bCharsConsumed = 1;
        }

        if (!bCharsConsumed || !*psChar) {
            return 0;
        }

        pbCurr += bCharsConsumed;
        ++psChar;
    }

    *pnStringLen = (ET9UINT)(psChar - &psString[0]);

    WLOG5({
        ET9UINT nCount;
        ET9SYMB const * psCurr;

        fprintf(pLogFile5, "  string: ");

        psCurr = &psString[0];
        for (nCount = *pnStringLen; nCount; --nCount, ++psCurr) {

            if (*psCurr > 0x20 && *psCurr <= 0x7F) {
                fprintf(pLogFile5, "%c", (char)*psCurr);
            }
            else {
                fprintf(pLogFile5, "<%x>", (int)*psCurr);
            }
        }

        fprintf(pLogFile5, "\n");
    })

    return 1;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pReaderInfo           Reader context.
 *
 * @return Zero on failure, otherwise non zero.
 */

static ET9BOOL ET9LOCALCALL __XmlReader_GetNextToken(ET9KdbXmlReaderInfo  * const pReaderInfo)
{
    if (pReaderInfo->eCurrToken == ET9_xml_token_Error) {
        return 0;
    }

    if (pReaderInfo->eCurrToken == ET9_xml_token_EOF) {
        pReaderInfo->eCurrToken = ET9_xml_token_Error;
        return 0;
    }

    /* white space, comments etc */

    for (; pReaderInfo->pbCurrChar < pReaderInfo->pbEndChar; ) {

        ET9U8 const * const pbStart = pReaderInfo->pbCurrChar;

        /* white space */

        while (pReaderInfo->pbCurrChar < pReaderInfo->pbEndChar &&
               *pReaderInfo->pbCurrChar <= 0x20) {

            if (*pReaderInfo->pbCurrChar == 0xA) {
                ++pReaderInfo->pbCurrChar;
                if (*pReaderInfo->pbCurrChar == 0xD) {
                    ++pReaderInfo->pbCurrChar;
                }
                ++pReaderInfo->nCurrLine;
            }
            else if (*pReaderInfo->pbCurrChar == 0xD) {
                ++pReaderInfo->pbCurrChar;
                if (*pReaderInfo->pbCurrChar == 0xA) {
                    ++pReaderInfo->pbCurrChar;
                }
                ++pReaderInfo->nCurrLine;
            }
            else {
                ++pReaderInfo->pbCurrChar;
            }
        }

        /* comments <!-- --> */

        if ((pReaderInfo->pbCurrChar + 4) < pReaderInfo->pbEndChar &&
            pReaderInfo->pbCurrChar[0] == '<' &&
            pReaderInfo->pbCurrChar[1] == '!' &&
            pReaderInfo->pbCurrChar[2] == '-' &&
            pReaderInfo->pbCurrChar[3] == '-') {

            pReaderInfo->pbCurrChar += 4;

            while ((pReaderInfo->pbCurrChar + 3) < pReaderInfo->pbEndChar) {

                if (pReaderInfo->pbCurrChar[0] == '-' &&
                    pReaderInfo->pbCurrChar[1] == '-' &&
                    pReaderInfo->pbCurrChar[2] == '>') {

                    pReaderInfo->pbCurrChar += 3;
                    break;
                }

                if (*pReaderInfo->pbCurrChar == 0xA) {
                    ++pReaderInfo->pbCurrChar;
                    if (*pReaderInfo->pbCurrChar == 0xD) {
                        ++pReaderInfo->pbCurrChar;
                    }
                    ++pReaderInfo->nCurrLine;
                }
                else if (*pReaderInfo->pbCurrChar == 0xD) {
                    ++pReaderInfo->pbCurrChar;
                    if (*pReaderInfo->pbCurrChar == 0xA) {
                        ++pReaderInfo->pbCurrChar;
                    }
                    ++pReaderInfo->nCurrLine;
                }
                else {
                    ++pReaderInfo->pbCurrChar;
                }
            }
        }

        /* xml declaration <?xml ?> */

        if ((pReaderInfo->pbCurrChar + 5) < pReaderInfo->pbEndChar &&
            pReaderInfo->pbCurrChar[0] == '<' &&
            pReaderInfo->pbCurrChar[1] == '?' &&
            pReaderInfo->pbCurrChar[2] == 'x' &&
            pReaderInfo->pbCurrChar[3] == 'm' &&
            pReaderInfo->pbCurrChar[4] == 'l') {

            pReaderInfo->pbCurrChar += 5;

            while ((pReaderInfo->pbCurrChar + 2) < pReaderInfo->pbEndChar) {

                if (pReaderInfo->pbCurrChar[0] == '?' &&
                    pReaderInfo->pbCurrChar[1] == '>') {

                    pReaderInfo->pbCurrChar += 2;
                    break;
                }

                if (*pReaderInfo->pbCurrChar == 0xA && (pReaderInfo->pbCurrChar + 1) < pReaderInfo->pbEndChar && *(pReaderInfo->pbCurrChar + 1) == 0xD) {
                    pReaderInfo->pbCurrChar += 2;
                    ++pReaderInfo->nCurrLine;
                }
                else if (*pReaderInfo->pbCurrChar == 0xD && (pReaderInfo->pbCurrChar + 1) < pReaderInfo->pbEndChar && *(pReaderInfo->pbCurrChar + 1) == 0xA) {
                    pReaderInfo->pbCurrChar += 2;
                    ++pReaderInfo->nCurrLine;
                }
                else if (*pReaderInfo->pbCurrChar == 0xA) {
                    ++pReaderInfo->pbCurrChar;
                    ++pReaderInfo->nCurrLine;
                }
                else if (*pReaderInfo->pbCurrChar == 0xD) {
                    ++pReaderInfo->pbCurrChar;
                    ++pReaderInfo->nCurrLine;
                }
                else {
                    ++pReaderInfo->pbCurrChar;
                }
            }
        }

        /* consumed any? */

        if (pbStart == pReaderInfo->pbCurrChar) {
            break;
        }
    }

    if (pReaderInfo->pbCurrChar >= pReaderInfo->pbEndChar) {
        pReaderInfo->eCurrToken = ET9_xml_token_EOF;
        return 1;
    }

    /* delimiters */

    switch (*pReaderInfo->pbCurrChar)
    {
        case '>':
            pReaderInfo->eCurrToken = ET9_xml_token_TagEnd;
            ++pReaderInfo->pbCurrChar;
            return 1;
        case '/':
            if ((pReaderInfo->pbCurrChar + 1) < pReaderInfo->pbEndChar && *(pReaderInfo->pbCurrChar + 1) == '>') {
                pReaderInfo->eCurrToken = ET9_xml_token_TagEndFinal;
                pReaderInfo->pbCurrChar += 2;
                return 1;
            }
            break;
        case '=':
            pReaderInfo->eCurrToken = ET9_xml_token_Equal;
            ++pReaderInfo->pbCurrChar;
            return 1;
        default:
            break;
    }

    /* tags */

    if (*pReaderInfo->pbCurrChar == '<') {

        ET9UINT nLen;
        ET9U32 dwHashValue;

        ++pReaderInfo->pbCurrChar;

        nLen = 0;
        dwHashValue = 0;

        while (pReaderInfo->pbCurrChar < pReaderInfo->pbEndChar &&
               *pReaderInfo->pbCurrChar != '>' &&
               *pReaderInfo->pbCurrChar >  0x20) {

            ET9U8 bChar = *pReaderInfo->pbCurrChar;

            ++nLen;
            ++pReaderInfo->pbCurrChar;

            if (nLen >= 32) {
                pReaderInfo->eCurrToken = ET9_xml_token_Error;
                return 0;
            }

            dwHashValue = bChar + (65599 * dwHashValue);
        }

        if (nLen) {

            switch (dwHashValue)
            {
                case 0x7f763807: pReaderInfo->eCurrToken = ET9_xml_token_TagStartKeyboard; return 1;
                case 0x41301a36: pReaderInfo->eCurrToken = ET9_xml_token_TagEndKeyboard; return 1;
                case 0xd99d190d: pReaderInfo->eCurrToken = ET9_xml_token_TagStartArea; return 1;
                case 0x52028a3c: pReaderInfo->eCurrToken = ET9_xml_token_TagEndArea; return 1;
                case 0x3892033a: pReaderInfo->eCurrToken = ET9_xml_token_TagStartRow; return 1;
                case 0xc352564b: pReaderInfo->eCurrToken = ET9_xml_token_TagEndRow; return 1;
                case 0x3515943f: pReaderInfo->eCurrToken = ET9_xml_token_TagStartKey; return 1;
                case 0xa9360b34: pReaderInfo->eCurrToken = ET9_xml_token_TagStartVoid; return 1;

                default:
                    pReaderInfo->eCurrToken = ET9_xml_token_Error;
                    return 0;
            }
        }
        else {
            pReaderInfo->eCurrToken = ET9_xml_token_Error;
            return 0;
        }
    }

    /* string */

    if (*pReaderInfo->pbCurrChar == '"') {

        ET9U8 const * const pbStart = pReaderInfo->pbCurrChar + 1;

        ET9U8 const * pbLook;

        for (pbLook = pbStart; pbLook < pReaderInfo->pbEndChar && *pbLook != '"'; ++pbLook) {
            if (*pbLook == 0xA) {
                if (*(pbLook + 1) == 0xD) {
                    ++pbLook;
                }
                ++pReaderInfo->nCurrLine;
            }
            else if (*pbLook == 0xD) {
                if (*(pbLook + 1) == 0xA) {
                    ++pbLook;
                }
                ++pReaderInfo->nCurrLine;
            }
        }

        if (pbLook >= pReaderInfo->pbEndChar) {
            pReaderInfo->eCurrToken = ET9_xml_token_Error;
            return 0;
        }

        if (!__XmlReader_DecodeString(pbStart,
                                      pbLook - 1,
                                      pReaderInfo->pKDBInfo->Private.wm.xmlReader.psString,
                                      ET9_KDB_XML_MAX_STRING_CHARS,
                                      &pReaderInfo->pKDBInfo->Private.wm.xmlReader.nStringLen)) {

            pReaderInfo->eCurrToken = ET9_xml_token_Error;
            return 0;
        }

        pReaderInfo->pbCurrChar = pbLook + 1;
        pReaderInfo->eCurrToken = ET9_xml_token_String;
        return 1;
    }

    /* attribute name */

    {
        ET9UINT nLen;
        ET9U32 dwHashValue;

        nLen = 0;
        dwHashValue = 0;

        while (pReaderInfo->pbCurrChar < pReaderInfo->pbEndChar &&
               *pReaderInfo->pbCurrChar != '=' &&
               *pReaderInfo->pbCurrChar != '/' &&
               *pReaderInfo->pbCurrChar != '>' &&
               *pReaderInfo->pbCurrChar >  0x20) {

            ET9U8 bChar = *pReaderInfo->pbCurrChar;

            ++nLen;
            ++pReaderInfo->pbCurrChar;

            if (nLen >= 64) {
                pReaderInfo->eCurrToken = ET9_xml_token_Error;
                return 0;
            }

            dwHashValue = bChar + (65599 * dwHashValue);
        }

        if (nLen) {

            switch (dwHashValue)
            {
                case 0x3852fd16: pReaderInfo->eCurrToken = ET9_xml_token_A_keyTop; return 1;
                case 0x60552566: pReaderInfo->eCurrToken = ET9_xml_token_A_keyLeft; return 1;
                case 0x7ea16d87: pReaderInfo->eCurrToken = ET9_xml_token_A_keyWidth; return 1;
                case 0x1d311026: pReaderInfo->eCurrToken = ET9_xml_token_A_keyHeight; return 1;
                case 0xf1327f75: pReaderInfo->eCurrToken = ET9_xml_token_A_voidSize; return 1;
                case 0x643da662: pReaderInfo->eCurrToken = ET9_xml_token_A_verticalBorder; return 1;
                case 0x41085ab0: pReaderInfo->eCurrToken = ET9_xml_token_A_horizontalBorder; return 1;
                case 0x12b166a0: pReaderInfo->eCurrToken = ET9_xml_token_A_verticalGap; return 1;
                case 0x152837d2: pReaderInfo->eCurrToken = ET9_xml_token_A_horizontalGap; return 1;
                case 0x147e59df: pReaderInfo->eCurrToken = ET9_xml_token_A_majorVersion; return 1;
                case 0x0e9fcee3: pReaderInfo->eCurrToken = ET9_xml_token_A_minorVersion; return 1;
                case 0xf21cdcfd: pReaderInfo->eCurrToken = ET9_xml_token_A_primaryId; return 1;
                case 0xc5bf52cf: pReaderInfo->eCurrToken = ET9_xml_token_A_secondaryId; return 1;
                case 0xcd0ab57b: pReaderInfo->eCurrToken = ET9_xml_token_A_supportsExact; return 1;
                case 0xfe6d1236: pReaderInfo->eCurrToken = ET9_xml_token_A_conditionValue; return 1;
                case 0xde6ee3d9: pReaderInfo->eCurrToken = ET9_xml_token_A_keyType; return 1;
                case 0xd3cd9658: pReaderInfo->eCurrToken = ET9_xml_token_A_keyIcon; return 1;
                case 0xbb718a8a: pReaderInfo->eCurrToken = ET9_xml_token_A_keyName; return 1;
                case 0x1e37afd5: pReaderInfo->eCurrToken = ET9_xml_token_A_keyLabel; return 1;
                case 0x69648d8c: pReaderInfo->eCurrToken = ET9_xml_token_A_keyLabelShifted; return 1;
                case 0x4743e247: pReaderInfo->eCurrToken = ET9_xml_token_A_keyCodes; return 1;
                case 0x67f1029a: pReaderInfo->eCurrToken = ET9_xml_token_A_keyCodesShifted; return 1;
                case 0xe4e33430: pReaderInfo->eCurrToken = ET9_xml_token_A_keyPopupChars; return 1;
                case 0x0432e429: pReaderInfo->eCurrToken = ET9_xml_token_A_keyPopupShiftedChars; return 1;
                case 0x2c0014b9: pReaderInfo->eCurrToken = ET9_xml_token_A_keyPopupCodes; return 1;
                case 0x4b4fc4b2: pReaderInfo->eCurrToken = ET9_xml_token_A_keyPopupShiftedCodes; return 1;
                case 0x887fa4b4: pReaderInfo->eCurrToken = ET9_xml_token_A_keyMultitapChars; return 1;
                case 0xc47a1aa5: pReaderInfo->eCurrToken = ET9_xml_token_A_keyMultitapShiftedChars; return 1;
                case 0xcf9c853d: pReaderInfo->eCurrToken = ET9_xml_token_A_keyMultitapCodes; return 1;
                case 0x0b96fb2e: pReaderInfo->eCurrToken = ET9_xml_token_A_keyMultitapShiftedCodes; return 1;
                case 0x318187bb: pReaderInfo->eCurrToken = ET9_xml_token_A_defaultLayoutWidth; return 1;
                case 0x3c8b82f2: pReaderInfo->eCurrToken = ET9_xml_token_A_defaultLayoutHeight; return 1;
                case 0x47666e95: pReaderInfo->eCurrToken = ET9_xml_token_A_addAltChars; return 1;

                default:
                    pReaderInfo->eCurrToken = ET9_xml_token_A_unknown;
                    return 1;
            }
        }
    }

    /* nothing recognized */

    pReaderInfo->eCurrToken = ET9_xml_token_Error;

    return 0;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pReaderInfo           Reader context.
 * @param eToken                Xxx
 *
 * @return Zero on failure, otherwise non zero.
 */

ET9INLINE static ET9BOOL ET9LOCALCALL __XmlReader_ConsumeToken(ET9KdbXmlReaderInfo        * const pReaderInfo,
                                                               const ET9_xml_token                eToken)
{
    if (pReaderInfo->eCurrToken != eToken) {
        return 0;
    }

    if (!__XmlReader_GetNextToken(pReaderInfo)) {
        return 0;
    }

    return 1;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pReaderInfo           Reader context.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

static ET9STATUS ET9LOCALCALL __XmlReader_ReadAttribute(ET9KdbXmlReaderInfo  * const pReaderInfo)
{
    WLOG5(fprintf(pLogFile5, "__XmlReader_ReadAttribute\n");)

    if (pReaderInfo->eCurrToken == ET9_xml_token_Error) {
        return ET9STATUS_KDB_SYNTAX_ERROR;
    }

    pReaderInfo->eCurrAttribute = pReaderInfo->eCurrToken;

    switch (pReaderInfo->eCurrAttribute)
    {
        case ET9_xml_token_A_keyTop:
        case ET9_xml_token_A_keyLeft:
        case ET9_xml_token_A_keyWidth:
        case ET9_xml_token_A_keyHeight:
        case ET9_xml_token_A_verticalBorder:
        case ET9_xml_token_A_horizontalBorder:
        case ET9_xml_token_A_verticalGap:
        case ET9_xml_token_A_horizontalGap:
        case ET9_xml_token_A_majorVersion:
        case ET9_xml_token_A_minorVersion:
        case ET9_xml_token_A_primaryId:
        case ET9_xml_token_A_secondaryId:
        case ET9_xml_token_A_supportsExact:
        case ET9_xml_token_A_conditionValue:
        case ET9_xml_token_A_keyType:
        case ET9_xml_token_A_keyIcon:
        case ET9_xml_token_A_keyName:
        case ET9_xml_token_A_keyLabel:
        case ET9_xml_token_A_keyLabelShifted:
        case ET9_xml_token_A_keyCodes:
        case ET9_xml_token_A_keyCodesShifted:
        case ET9_xml_token_A_keyPopupChars:
        case ET9_xml_token_A_keyPopupShiftedChars:
        case ET9_xml_token_A_keyPopupCodes:
        case ET9_xml_token_A_keyPopupShiftedCodes:
        case ET9_xml_token_A_keyMultitapChars:
        case ET9_xml_token_A_keyMultitapShiftedChars:
        case ET9_xml_token_A_keyMultitapCodes:
        case ET9_xml_token_A_keyMultitapShiftedCodes:
        case ET9_xml_token_A_addAltChars:
        case ET9_xml_token_A_voidSize:
        case ET9_xml_token_A_defaultLayoutWidth:
        case ET9_xml_token_A_defaultLayoutHeight:
            break;
        case ET9_xml_token_A_unknown:
            ++pReaderInfo->nUnknownAttributes;
            break;
        default:
            return ET9STATUS_KDB_SYNTAX_ERROR;
    }

    WLOG5(fprintf(pLogFile5, "  attribute '%s'\n", __XmlReader_AttributeToString(pReaderInfo->eCurrAttribute));)

    if (!__XmlReader_ConsumeToken(pReaderInfo, pReaderInfo->eCurrToken)) {
        return ET9STATUS_KDB_SYNTAX_ERROR;
    }

    if (!__XmlReader_ConsumeToken(pReaderInfo, ET9_xml_token_Equal)) {
        return ET9STATUS_KDB_SYNTAX_ERROR;
    }

    if (!__XmlReader_ConsumeToken(pReaderInfo, ET9_xml_token_String)) {
        return ET9STATUS_KDB_SYNTAX_ERROR;
    }

    /* extract corresponding values from the string */

    switch (pReaderInfo->eCurrAttribute)
    {
        case ET9_xml_token_A_keyTop:
        case ET9_xml_token_A_keyLeft:
        case ET9_xml_token_A_keyWidth:
        case ET9_xml_token_A_keyHeight:
        case ET9_xml_token_A_voidSize:
        case ET9_xml_token_A_verticalBorder:
        case ET9_xml_token_A_horizontalBorder:
        case ET9_xml_token_A_verticalGap:
        case ET9_xml_token_A_horizontalGap:
            if (!__XmlReader_ExtractFloatWithSuffix(pReaderInfo)) {
                return ET9STATUS_KDB_ATTRIBUTE_VALUE_ERROR;
            }
            break;

        case ET9_xml_token_A_majorVersion:
        case ET9_xml_token_A_minorVersion:
        case ET9_xml_token_A_primaryId:
        case ET9_xml_token_A_secondaryId:
            if (!__XmlReader_ExtractInt(pReaderInfo)) {
                return ET9STATUS_KDB_ATTRIBUTE_VALUE_ERROR;
            }
            if (pReaderInfo->nCurrInt > 0xFF) {
                return ET9STATUS_KDB_ATTRIBUTE_VALUE_ERROR;
            }
            break;

        case ET9_xml_token_A_defaultLayoutWidth:
        case ET9_xml_token_A_defaultLayoutHeight:
            if (!__XmlReader_ExtractInt(pReaderInfo)) {
                return ET9STATUS_KDB_ATTRIBUTE_VALUE_ERROR;
            }
            if (pReaderInfo->nCurrInt > 0xFFFF) {
                return ET9STATUS_KDB_ATTRIBUTE_VALUE_ERROR;
            }
            break;

        case ET9_xml_token_A_conditionValue:
            if (!__XmlReader_ExtractInt(pReaderInfo)) {
                return ET9STATUS_KDB_ATTRIBUTE_VALUE_ERROR;
            }
            if (pReaderInfo->nCurrInt > 0xFFFE) {
                return ET9STATUS_KDB_ATTRIBUTE_VALUE_ERROR;
            }
            {
                ET9KDBPrivate * const pPrivate = &pReaderInfo->pKDBInfo->Private;
                ET9KdbXmlKeyboardInfo * const pKeyboardInfo = &pPrivate->wm.xmlReader.sKeyboardInfo;
                ET9KdbXmlKeyboard * const pKeyboard = &pKeyboardInfo->sKeyboard;

                if (pKeyboard->wConditionValueMax < pReaderInfo->nCurrInt) {
                    pKeyboard->wConditionValueMax = (ET9U16)pReaderInfo->nCurrInt;
                }
            }
            break;

        case ET9_xml_token_A_addAltChars:
        case ET9_xml_token_A_supportsExact:
            if (!__XmlReader_ExtractConstant(pReaderInfo)) {
                return ET9STATUS_KDB_ATTRIBUTE_VALUE_ERROR;
            }
            switch (pReaderInfo->eCurrConstant)
            {
                case ET9_xml_C_true:
                case ET9_xml_C_false:
                    break;
                default:
                    return ET9STATUS_KDB_ATTRIBUTE_VALUE_ERROR;
            }
            break;

        case ET9_xml_token_A_keyType:
            if (!__XmlReader_ExtractConstant(pReaderInfo)) {
                return ET9STATUS_KDB_ATTRIBUTE_VALUE_ERROR;
            }
            switch (pReaderInfo->eCurrConstant)
            {
                case ET9_xml_C_regional:
                case ET9_xml_C_nonRegional:
                case ET9_xml_C_smartPunct:
                case ET9_xml_C_string:
                case ET9_xml_C_function:
                    break;
                default:
                    return ET9STATUS_KDB_ATTRIBUTE_VALUE_ERROR;
            }
            break;

        case ET9_xml_token_A_keyIcon:
            if (!__XmlReader_ExtractChars(pReaderInfo)) {
                return ET9STATUS_KDB_ATTRIBUTE_VALUE_ERROR;
            }
            break;

        case ET9_xml_token_A_keyName:
            if (!__XmlReader_ExtractName(pReaderInfo)) {
                return ET9STATUS_KDB_ATTRIBUTE_VALUE_ERROR;
            }
            break;

        case ET9_xml_token_A_keyLabel:
        case ET9_xml_token_A_keyLabelShifted:
        case ET9_xml_token_A_keyPopupChars:
        case ET9_xml_token_A_keyPopupShiftedChars:
        case ET9_xml_token_A_keyMultitapChars:
        case ET9_xml_token_A_keyMultitapShiftedChars:
            if (!__XmlReader_ExtractChars(pReaderInfo)) {
                return ET9STATUS_KDB_ATTRIBUTE_VALUE_ERROR;
            }
            break;

        case ET9_xml_token_A_keyCodes:
        case ET9_xml_token_A_keyCodesShifted:
        case ET9_xml_token_A_keyPopupCodes:
        case ET9_xml_token_A_keyPopupShiftedCodes:
        case ET9_xml_token_A_keyMultitapCodes:
        case ET9_xml_token_A_keyMultitapShiftedCodes:
            if (!__XmlReader_ExtractCodes(pReaderInfo)) {
                return ET9STATUS_KDB_ATTRIBUTE_VALUE_ERROR;
            }
            break;
    }

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pReaderInfo           Reader context.
 * @param pKeyboardInfo         .
 * @param psDst                 .
 * @param pnDst                 .
 * @param psSrc                 .
 * @param nSrc                  .
 * @param eCopyOp               .
 * @param psDedupe              .
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

static ET9STATUS ET9LOCALCALL __XmlReader_AppendString(ET9KdbXmlReaderInfo      * const pReaderInfo,
                                                       ET9KdbXmlKeyboardInfo    * const pKeyboardInfo,
                                                       ET9SYMB                  * const psDst,
                                                       ET9UINT                  * const pnDst,
                                                       ET9SYMB            const * const psSrc,
                                                       const ET9UINT                    nSrc,
                                                       const ET9_xml_copyOp             eCopyOp,
                                                       ET9SYMB                  * const psDedupe)
{
    if (pKeyboardInfo->nUsedSymbs + nSrc > ET9_KDB_XML_MAX_POOL_CHARS) {
        return ET9STATUS_KDB_HAS_TOO_MANY_CHARS;
    }

    if (*pnDst + nSrc > ET9_KDB_XML_MAX_DEDUPE_CHARS) {
        return ET9STATUS_KDB_KEY_HAS_TOO_MANY_CHARS;
    }

    if (&psDst[*pnDst] != &pKeyboardInfo->psSymbPool[pKeyboardInfo->nUsedSymbs]) {
        return ET9STATUS_ERROR;
    }

    if (nSrc) {

        ET9UINT nCount;
        ET9SYMB const *psSrcSymb;
        ET9SYMB *psDstSymb;

        psSrcSymb = psSrc;
        psDstSymb = &psDst[*pnDst];
        for (nCount = nSrc; nCount; --nCount, ++psSrcSymb) {

            const ET9SYMB sSymb = (eCopyOp == ET9_xml_copyOp_toLower) ? _ET9SymToLower(*psSrcSymb, pReaderInfo->pKDBInfo->Private.wLocale) : *psSrcSymb;

            if (psDedupe) {

                const ET9UINT nHash = sSymb % ET9_KDB_XML_MAX_DEDUPE_CHARS;

                if (!psDedupe[nHash]) {
                    psDedupe[nHash] = sSymb;
                }
                else if (psDedupe[nHash] == sSymb) {
                    continue;
                }
                else {

                    ET9UINT nLook;

                    for (nLook = nHash; psDedupe[nLook]; ) {
                        ++nLook;
                        if (nLook >= ET9_KDB_XML_MAX_DEDUPE_CHARS) {
                            nLook = 0;
                        }

                        if (psDedupe[nHash] == sSymb) {
                            break;
                        }
                    }

                    if (psDedupe[nLook]) {
                        continue;
                    }

                    psDedupe[nLook] = sSymb;
                }
            }

            *psDstSymb++ = sSymb;
        }

        {
            const ET9UINT nAddCount = (ET9UINT)(psDstSymb - &psDst[*pnDst]);

            *pnDst += nAddCount;

            pKeyboardInfo->nUsedSymbs += nAddCount;
        }
    }

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pReaderInfo           Reader context.
 * @param pKeyboardInfo         .
 * @param ppsDst                .
 * @param pnDst                 .
 * @param psSrc                 .
 * @param nSrc                  .
 * @param eCopyOp               .
 * @param psDedupe              .
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

static ET9STATUS ET9LOCALCALL __XmlReader_CreateString(ET9KdbXmlReaderInfo      * const pReaderInfo,
                                                       ET9KdbXmlKeyboardInfo    * const pKeyboardInfo,
                                                       ET9SYMB                 ** const ppsDst,
                                                       ET9UINT                  * const pnDst,
                                                       ET9SYMB                  * const psSrc,
                                                       const ET9UINT                    nSrc,
                                                       const ET9_xml_copyOp             eCopyOp,
                                                       ET9SYMB                  * const psDedupe)
{
    if (pKeyboardInfo->nUsedSymbs + nSrc > ET9_KDB_XML_MAX_POOL_CHARS) {
        return ET9STATUS_KDB_HAS_TOO_MANY_CHARS;
    }

    if (psDedupe) {
        _ET9ClearMem((ET9U8*)psDedupe, ET9_KDB_XML_MAX_DEDUPE_CHARS * sizeof(ET9SYMB));
    }

    *ppsDst = &pKeyboardInfo->psSymbPool[pKeyboardInfo->nUsedSymbs];

    *pnDst = 0;

    return __XmlReader_AppendString(pReaderInfo, pKeyboardInfo, *ppsDst, pnDst, psSrc, nSrc, eCopyOp, psDedupe);
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param sChar                 .
 *
 * @return Pointer to alt chars or NULL if there are none.
 */

static ET9SYMB const * ET9LOCALCALL __XmlReader_AltCharsLower(const ET9SYMB sChar)
{
    static const ET9SYMB psAltCharTable[] = {
        0x03b1,0x03ac,0x0000,0x03b5,0x03ad,0x0000,0x03b7,0x03ae,0x0000,0x03b9,0x03af,0x03ca,0x0390,0x0000,0x03bf,0x03cc,0x0000,0x03c5,0x03cd,0x03cb,0x03b0,0x0000,0x03c9,0x03ce,0x0000,0x0915,0x0958,0x0000,0x0916,0x0959,0x0000,0x0917,0x095a,0x0000,0x091c,0x095b,0x0000,0x0921,0x095c,0x0000,0x0922,0x095d,0x0000,0x092b,0x095e,0x0000,0x092f,0x095f,0x0000,0x0061,0x00e0,0x00e1,0x00e2,0x00e3,0x00e4,0x00e5,0x00e6,0x0101,0x0103,0x0105,0x0000,0x0062,0x0253,0x0000,
        0x0063,0x00e7,0x0107,0x0109,0x010b,0x010d,0x0000,0x0064,0x00f0,0x010f,0x0111,0x0257,0x0000,0x0065,0x00e8,0x00e9,0x00ea,0x00eb,0x0113,0x0115,0x0117,0x0119,0x011b,0x025b,0x1eb9,0x0000,0x0066,0x0192,0x0000,0x0067,0x011d,0x011f,0x0121,0x0123,0x0000,0x0068,0x0125,0x0127,0x0000,0x0069,0x00ec,0x00ed,0x00ee,0x00ef,0x0129,0x012b,0x012d,0x012f,0x0131,0x0133,0x1ecb,0x0000,0x006a,0x0135,0x0000,0x006b,0x0137,0x0138,0x0199,0x0000,0x006c,0x013a,0x013c,0x013e,
        0x0140,0x0142,0x0000,0x006e,0x00f1,0x0144,0x0146,0x0148,0x0149,0x014b,0x1e45,0x0000,0x006f,0x00f2,0x00f3,0x00f4,0x00f5,0x00f6,0x00f8,0x014d,0x014f,0x0151,0x0153,0x01a1,0x0254,0x1ecd,0x0000,0x0072,0x0155,0x0157,0x0159,0x0000,0x0073,0x00df,0x015b,0x015d,0x015f,0x0161,0x017f,0x1e63,0x0000,0x0074,0x00fe,0x0163,0x0165,0x0167,0x0000,0x0075,0x00f9,0x00fa,0x00fb,0x00fc,0x0169,0x016b,0x016d,0x016f,0x0171,0x0173,0x01b0,0x1ee5,0x0000,0x0077,0x0175,0x0000,
        0x0079,0x00fd,0x00ff,0x0177,0x01b4,0x0000,0x007a,0x017a,0x017c,0x017e,0x0000};

    switch (sChar)
    {
        case 0x0061: return &psAltCharTable[49];
        case 0x0062: return &psAltCharTable[61];
        case 0x0063: return &psAltCharTable[64];
        case 0x0064: return &psAltCharTable[71];
        case 0x0065: return &psAltCharTable[77];
        case 0x0066: return &psAltCharTable[90];
        case 0x0067: return &psAltCharTable[93];
        case 0x0068: return &psAltCharTable[99];
        case 0x0069: return &psAltCharTable[103];
        case 0x006a: return &psAltCharTable[116];
        case 0x006b: return &psAltCharTable[119];
        case 0x006c: return &psAltCharTable[124];
        case 0x006e: return &psAltCharTable[131];
        case 0x006f: return &psAltCharTable[140];
        case 0x0072: return &psAltCharTable[155];
        case 0x0073: return &psAltCharTable[160];
        case 0x0074: return &psAltCharTable[169];
        case 0x0075: return &psAltCharTable[175];
        case 0x0077: return &psAltCharTable[189];
        case 0x0079: return &psAltCharTable[192];
        case 0x007a: return &psAltCharTable[198];
        case 0x00df: return &psAltCharTable[160];
        case 0x00e0: return &psAltCharTable[49];
        case 0x00e1: return &psAltCharTable[49];
        case 0x00e2: return &psAltCharTable[49];
        case 0x00e3: return &psAltCharTable[49];
        case 0x00e4: return &psAltCharTable[49];
        case 0x00e5: return &psAltCharTable[49];
        case 0x00e6: return &psAltCharTable[49];
        case 0x00e7: return &psAltCharTable[64];
        case 0x00e8: return &psAltCharTable[77];
        case 0x00e9: return &psAltCharTable[77];
        case 0x00ea: return &psAltCharTable[77];
        case 0x00eb: return &psAltCharTable[77];
        case 0x00ec: return &psAltCharTable[103];
        case 0x00ed: return &psAltCharTable[103];
        case 0x00ee: return &psAltCharTable[103];
        case 0x00ef: return &psAltCharTable[103];
        case 0x00f0: return &psAltCharTable[71];
        case 0x00f1: return &psAltCharTable[131];
        case 0x00f2: return &psAltCharTable[140];
        case 0x00f3: return &psAltCharTable[140];
        case 0x00f4: return &psAltCharTable[140];
        case 0x00f5: return &psAltCharTable[140];
        case 0x00f6: return &psAltCharTable[140];
        case 0x00f8: return &psAltCharTable[140];
        case 0x00f9: return &psAltCharTable[175];
        case 0x00fa: return &psAltCharTable[175];
        case 0x00fb: return &psAltCharTable[175];
        case 0x00fc: return &psAltCharTable[175];
        case 0x00fd: return &psAltCharTable[192];
        case 0x00fe: return &psAltCharTable[169];
        case 0x00ff: return &psAltCharTable[192];
        case 0x0101: return &psAltCharTable[49];
        case 0x0103: return &psAltCharTable[49];
        case 0x0105: return &psAltCharTable[49];
        case 0x0107: return &psAltCharTable[64];
        case 0x0109: return &psAltCharTable[64];
        case 0x010b: return &psAltCharTable[64];
        case 0x010d: return &psAltCharTable[64];
        case 0x010f: return &psAltCharTable[71];
        case 0x0111: return &psAltCharTable[71];
        case 0x0113: return &psAltCharTable[77];
        case 0x0115: return &psAltCharTable[77];
        case 0x0117: return &psAltCharTable[77];
        case 0x0119: return &psAltCharTable[77];
        case 0x011b: return &psAltCharTable[77];
        case 0x011d: return &psAltCharTable[93];
        case 0x011f: return &psAltCharTable[93];
        case 0x0121: return &psAltCharTable[93];
        case 0x0123: return &psAltCharTable[93];
        case 0x0125: return &psAltCharTable[99];
        case 0x0127: return &psAltCharTable[99];
        case 0x0129: return &psAltCharTable[103];
        case 0x012b: return &psAltCharTable[103];
        case 0x012d: return &psAltCharTable[103];
        case 0x012f: return &psAltCharTable[103];
        case 0x0131: return &psAltCharTable[103];
        case 0x0133: return &psAltCharTable[103];
        case 0x0135: return &psAltCharTable[116];
        case 0x0137: return &psAltCharTable[119];
        case 0x0138: return &psAltCharTable[119];
        case 0x013a: return &psAltCharTable[124];
        case 0x013c: return &psAltCharTable[124];
        case 0x013e: return &psAltCharTable[124];
        case 0x0140: return &psAltCharTable[124];
        case 0x0142: return &psAltCharTable[124];
        case 0x0144: return &psAltCharTable[131];
        case 0x0146: return &psAltCharTable[131];
        case 0x0148: return &psAltCharTable[131];
        case 0x0149: return &psAltCharTable[131];
        case 0x014b: return &psAltCharTable[131];
        case 0x014d: return &psAltCharTable[140];
        case 0x014f: return &psAltCharTable[140];
        case 0x0151: return &psAltCharTable[140];
        case 0x0153: return &psAltCharTable[140];
        case 0x0155: return &psAltCharTable[155];
        case 0x0157: return &psAltCharTable[155];
        case 0x0159: return &psAltCharTable[155];
        case 0x015b: return &psAltCharTable[160];
        case 0x015d: return &psAltCharTable[160];
        case 0x015f: return &psAltCharTable[160];
        case 0x0161: return &psAltCharTable[160];
        case 0x0163: return &psAltCharTable[169];
        case 0x0165: return &psAltCharTable[169];
        case 0x0167: return &psAltCharTable[169];
        case 0x0169: return &psAltCharTable[175];
        case 0x016b: return &psAltCharTable[175];
        case 0x016d: return &psAltCharTable[175];
        case 0x016f: return &psAltCharTable[175];
        case 0x0171: return &psAltCharTable[175];
        case 0x0173: return &psAltCharTable[175];
        case 0x0175: return &psAltCharTable[189];
        case 0x0177: return &psAltCharTable[192];
        case 0x017a: return &psAltCharTable[198];
        case 0x017c: return &psAltCharTable[198];
        case 0x017e: return &psAltCharTable[198];
        case 0x017f: return &psAltCharTable[160];
        case 0x0192: return &psAltCharTable[90];
        case 0x0199: return &psAltCharTable[119];
        case 0x01a1: return &psAltCharTable[140];
        case 0x01b0: return &psAltCharTable[175];
        case 0x01b4: return &psAltCharTable[192];
        case 0x0253: return &psAltCharTable[61];
        case 0x0254: return &psAltCharTable[140];
        case 0x0257: return &psAltCharTable[71];
        case 0x025b: return &psAltCharTable[77];
        case 0x0390: return &psAltCharTable[9];
        case 0x03ac: return &psAltCharTable[0];
        case 0x03ad: return &psAltCharTable[3];
        case 0x03ae: return &psAltCharTable[6];
        case 0x03af: return &psAltCharTable[9];
        case 0x03b0: return &psAltCharTable[17];
        case 0x03b1: return &psAltCharTable[0];
        case 0x03b5: return &psAltCharTable[3];
        case 0x03b7: return &psAltCharTable[6];
        case 0x03b9: return &psAltCharTable[9];
        case 0x03bf: return &psAltCharTable[14];
        case 0x03c5: return &psAltCharTable[17];
        case 0x03c9: return &psAltCharTable[22];
        case 0x03ca: return &psAltCharTable[9];
        case 0x03cb: return &psAltCharTable[17];
        case 0x03cc: return &psAltCharTable[14];
        case 0x03cd: return &psAltCharTable[17];
        case 0x03ce: return &psAltCharTable[22];
        case 0x0915: return &psAltCharTable[25];
        case 0x0916: return &psAltCharTable[28];
        case 0x0917: return &psAltCharTable[31];
        case 0x091c: return &psAltCharTable[34];
        case 0x0921: return &psAltCharTable[37];
        case 0x0922: return &psAltCharTable[40];
        case 0x092b: return &psAltCharTable[43];
        case 0x092f: return &psAltCharTable[46];
        case 0x0958: return &psAltCharTable[25];
        case 0x0959: return &psAltCharTable[28];
        case 0x095a: return &psAltCharTable[31];
        case 0x095b: return &psAltCharTable[34];
        case 0x095c: return &psAltCharTable[37];
        case 0x095d: return &psAltCharTable[40];
        case 0x095e: return &psAltCharTable[43];
        case 0x095f: return &psAltCharTable[46];
        case 0x1e45: return &psAltCharTable[131];
        case 0x1e63: return &psAltCharTable[160];
        case 0x1eb9: return &psAltCharTable[77];
        case 0x1ecb: return &psAltCharTable[103];
        case 0x1ecd: return &psAltCharTable[140];
        case 0x1ee5: return &psAltCharTable[175];
        default:     return NULL;
    }
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param sChar                 .
 *
 * @return Pointer to alt chars or NULL if there are none.
 */

static ET9SYMB const * ET9LOCALCALL __XmlReader_AltCharsUpper(const ET9SYMB sChar)
{
    static const ET9SYMB psAltCharTable[] = {
        0x0391,0x0386,0x0000,0x0395,0x0388,0x0000,0x0397,0x0389,0x0000,0x0399,0x038a,0x03aa,0x0000,0x039f,0x038c,0x0000,0x03a5,0x038e,0x03ab,0x0000,0x03a9,0x038f,0x0000,0x0915,0x0958,0x0000,0x0916,0x0959,0x0000,0x0917,0x095a,0x0000,0x091c,0x095b,0x0000,0x0921,0x095c,0x0000,0x0922,0x095d,0x0000,0x092b,0x095e,0x0000,0x092f,0x095f,0x0000,0x0041,0x00c0,0x00c1,0x00c2,0x00c3,0x00c4,0x00c5,0x00c6,0x0100,0x0102,0x0104,0x0000,0x0042,0x0181,0x0000,0x0043,0x00c7,
        0x0106,0x0108,0x010a,0x010c,0x0000,0x0044,0x00d0,0x010e,0x0110,0x018a,0x0000,0x0045,0x00c8,0x00c9,0x00ca,0x00cb,0x0112,0x0114,0x0116,0x0118,0x011a,0x0190,0x1eb8,0x0000,0x0046,0x0191,0x0000,0x0047,0x011c,0x011e,0x0120,0x0122,0x0000,0x0048,0x0124,0x0126,0x0000,0x0049,0x00cc,0x00cd,0x00ce,0x00cf,0x0128,0x012a,0x012c,0x012e,0x0132,0x1eca,0x0000,0x004a,0x0134,0x0000,0x004b,0x0136,0x0138,0x0198,0x0000,0x004c,0x0139,0x013b,0x013d,0x013f,0x0141,0x0000,
        0x004e,0x00d1,0x0143,0x0145,0x0147,0x0149,0x014a,0x1e44,0x0000,0x004f,0x00d2,0x00d3,0x00d4,0x00d5,0x00d6,0x00d8,0x014c,0x014e,0x0150,0x0152,0x01a0,0x0186,0x1ecc,0x0000,0x0052,0x0154,0x0156,0x0158,0x0000,0x0053,0x00df,0x015a,0x015c,0x015e,0x0160,0x1e62,0x0000,0x0054,0x00de,0x0162,0x0164,0x0166,0x0000,0x0055,0x00d9,0x00da,0x00db,0x00dc,0x0168,0x016a,0x016c,0x016e,0x0170,0x0172,0x01af,0x1ee4,0x0000,0x0057,0x0174,0x0000,0x0059,0x00dd,0x0178,0x0176,
        0x01b3,0x0000,0x005a,0x0179,0x017b,0x017d,0x0000};

    switch (sChar)
    {
        case 0x0041: return &psAltCharTable[47];
        case 0x0042: return &psAltCharTable[59];
        case 0x0043: return &psAltCharTable[62];
        case 0x0044: return &psAltCharTable[69];
        case 0x0045: return &psAltCharTable[75];
        case 0x0046: return &psAltCharTable[88];
        case 0x0047: return &psAltCharTable[91];
        case 0x0048: return &psAltCharTable[97];
        case 0x0049: return &psAltCharTable[101];
        case 0x004a: return &psAltCharTable[113];
        case 0x004b: return &psAltCharTable[116];
        case 0x004c: return &psAltCharTable[121];
        case 0x004e: return &psAltCharTable[128];
        case 0x004f: return &psAltCharTable[137];
        case 0x0052: return &psAltCharTable[152];
        case 0x0053: return &psAltCharTable[157];
        case 0x0054: return &psAltCharTable[165];
        case 0x0055: return &psAltCharTable[171];
        case 0x0057: return &psAltCharTable[185];
        case 0x0059: return &psAltCharTable[188];
        case 0x005a: return &psAltCharTable[194];
        case 0x00c0: return &psAltCharTable[47];
        case 0x00c1: return &psAltCharTable[47];
        case 0x00c2: return &psAltCharTable[47];
        case 0x00c3: return &psAltCharTable[47];
        case 0x00c4: return &psAltCharTable[47];
        case 0x00c5: return &psAltCharTable[47];
        case 0x00c6: return &psAltCharTable[47];
        case 0x00c7: return &psAltCharTable[62];
        case 0x00c8: return &psAltCharTable[75];
        case 0x00c9: return &psAltCharTable[75];
        case 0x00ca: return &psAltCharTable[75];
        case 0x00cb: return &psAltCharTable[75];
        case 0x00cc: return &psAltCharTable[101];
        case 0x00cd: return &psAltCharTable[101];
        case 0x00ce: return &psAltCharTable[101];
        case 0x00cf: return &psAltCharTable[101];
        case 0x00d0: return &psAltCharTable[69];
        case 0x00d1: return &psAltCharTable[128];
        case 0x00d2: return &psAltCharTable[137];
        case 0x00d3: return &psAltCharTable[137];
        case 0x00d4: return &psAltCharTable[137];
        case 0x00d5: return &psAltCharTable[137];
        case 0x00d6: return &psAltCharTable[137];
        case 0x00d8: return &psAltCharTable[137];
        case 0x00d9: return &psAltCharTable[171];
        case 0x00da: return &psAltCharTable[171];
        case 0x00db: return &psAltCharTable[171];
        case 0x00dc: return &psAltCharTable[171];
        case 0x00dd: return &psAltCharTable[188];
        case 0x00de: return &psAltCharTable[165];
        case 0x00df: return &psAltCharTable[157];
        case 0x0100: return &psAltCharTable[47];
        case 0x0102: return &psAltCharTable[47];
        case 0x0104: return &psAltCharTable[47];
        case 0x0106: return &psAltCharTable[62];
        case 0x0108: return &psAltCharTable[62];
        case 0x010a: return &psAltCharTable[62];
        case 0x010c: return &psAltCharTable[62];
        case 0x010e: return &psAltCharTable[69];
        case 0x0110: return &psAltCharTable[69];
        case 0x0112: return &psAltCharTable[75];
        case 0x0114: return &psAltCharTable[75];
        case 0x0116: return &psAltCharTable[75];
        case 0x0118: return &psAltCharTable[75];
        case 0x011a: return &psAltCharTable[75];
        case 0x011c: return &psAltCharTable[91];
        case 0x011e: return &psAltCharTable[91];
        case 0x0120: return &psAltCharTable[91];
        case 0x0122: return &psAltCharTable[91];
        case 0x0124: return &psAltCharTable[97];
        case 0x0126: return &psAltCharTable[97];
        case 0x0128: return &psAltCharTable[101];
        case 0x012a: return &psAltCharTable[101];
        case 0x012c: return &psAltCharTable[101];
        case 0x012e: return &psAltCharTable[101];
        case 0x0132: return &psAltCharTable[101];
        case 0x0134: return &psAltCharTable[113];
        case 0x0136: return &psAltCharTable[116];
        case 0x0138: return &psAltCharTable[116];
        case 0x0139: return &psAltCharTable[121];
        case 0x013b: return &psAltCharTable[121];
        case 0x013d: return &psAltCharTable[121];
        case 0x013f: return &psAltCharTable[121];
        case 0x0141: return &psAltCharTable[121];
        case 0x0143: return &psAltCharTable[128];
        case 0x0145: return &psAltCharTable[128];
        case 0x0147: return &psAltCharTable[128];
        case 0x0149: return &psAltCharTable[128];
        case 0x014a: return &psAltCharTable[128];
        case 0x014c: return &psAltCharTable[137];
        case 0x014e: return &psAltCharTable[137];
        case 0x0150: return &psAltCharTable[137];
        case 0x0152: return &psAltCharTable[137];
        case 0x0154: return &psAltCharTable[152];
        case 0x0156: return &psAltCharTable[152];
        case 0x0158: return &psAltCharTable[152];
        case 0x015a: return &psAltCharTable[157];
        case 0x015c: return &psAltCharTable[157];
        case 0x015e: return &psAltCharTable[157];
        case 0x0160: return &psAltCharTable[157];
        case 0x0162: return &psAltCharTable[165];
        case 0x0164: return &psAltCharTable[165];
        case 0x0166: return &psAltCharTable[165];
        case 0x0168: return &psAltCharTable[171];
        case 0x016a: return &psAltCharTable[171];
        case 0x016c: return &psAltCharTable[171];
        case 0x016e: return &psAltCharTable[171];
        case 0x0170: return &psAltCharTable[171];
        case 0x0172: return &psAltCharTable[171];
        case 0x0174: return &psAltCharTable[185];
        case 0x0176: return &psAltCharTable[188];
        case 0x0178: return &psAltCharTable[188];
        case 0x0179: return &psAltCharTable[194];
        case 0x017b: return &psAltCharTable[194];
        case 0x017d: return &psAltCharTable[194];
        case 0x0181: return &psAltCharTable[59];
        case 0x0186: return &psAltCharTable[137];
        case 0x018a: return &psAltCharTable[69];
        case 0x0190: return &psAltCharTable[75];
        case 0x0191: return &psAltCharTable[88];
        case 0x0198: return &psAltCharTable[116];
        case 0x01a0: return &psAltCharTable[137];
        case 0x01af: return &psAltCharTable[171];
        case 0x01b3: return &psAltCharTable[188];
        case 0x0386: return &psAltCharTable[0];
        case 0x0388: return &psAltCharTable[3];
        case 0x0389: return &psAltCharTable[6];
        case 0x038a: return &psAltCharTable[9];
        case 0x038c: return &psAltCharTable[13];
        case 0x038e: return &psAltCharTable[16];
        case 0x038f: return &psAltCharTable[20];
        case 0x0391: return &psAltCharTable[0];
        case 0x0395: return &psAltCharTable[3];
        case 0x0397: return &psAltCharTable[6];
        case 0x0399: return &psAltCharTable[9];
        case 0x039f: return &psAltCharTable[13];
        case 0x03a5: return &psAltCharTable[16];
        case 0x03a9: return &psAltCharTable[20];
        case 0x03aa: return &psAltCharTable[9];
        case 0x03ab: return &psAltCharTable[16];
        case 0x0915: return &psAltCharTable[23];
        case 0x0916: return &psAltCharTable[26];
        case 0x0917: return &psAltCharTable[29];
        case 0x091c: return &psAltCharTable[32];
        case 0x0921: return &psAltCharTable[35];
        case 0x0922: return &psAltCharTable[38];
        case 0x092b: return &psAltCharTable[41];
        case 0x092f: return &psAltCharTable[44];
        case 0x0958: return &psAltCharTable[23];
        case 0x0959: return &psAltCharTable[26];
        case 0x095a: return &psAltCharTable[29];
        case 0x095b: return &psAltCharTable[32];
        case 0x095c: return &psAltCharTable[35];
        case 0x095d: return &psAltCharTable[38];
        case 0x095e: return &psAltCharTable[41];
        case 0x095f: return &psAltCharTable[44];
        case 0x1e44: return &psAltCharTable[128];
        case 0x1e62: return &psAltCharTable[157];
        case 0x1eb8: return &psAltCharTable[75];
        case 0x1eca: return &psAltCharTable[101];
        case 0x1ecc: return &psAltCharTable[137];
        case 0x1ee4: return &psAltCharTable[171];
        default:     return NULL;
    }
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pReaderInfo           Reader context.
 * @param pKeyboardInfo         .
 * @param psDst                 .
 * @param pnDst                 .
 * @param psDedupe              .
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

static ET9STATUS ET9LOCALCALL __XmlReader_AppendAltCharsLower(ET9KdbXmlReaderInfo      * const pReaderInfo,
                                                              ET9KdbXmlKeyboardInfo    * const pKeyboardInfo,
                                                              ET9SYMB                  * const psDst,
                                                              ET9UINT                  * const pnDst,
                                                              ET9SYMB                  * const psDedupe)
{
    ET9UINT nCount;
    ET9SYMB *psSymb;

    psSymb = psDst;
    for (nCount = *pnDst; nCount; --nCount, ++psSymb) {

        ET9SYMB const *psAlts = __XmlReader_AltCharsLower(*psSymb);

        if (psAlts) {

            ET9STATUS eStatus;
            ET9UINT nAltCount;

            for (nAltCount = 0; psAlts[nAltCount]; ++nAltCount) {
            }

            eStatus = __XmlReader_AppendString(pReaderInfo, pKeyboardInfo, psDst, pnDst, psAlts, nAltCount, ET9_xml_copyOp_none, psDedupe);

            if (eStatus) {
                return eStatus;
            }
        }
    }

    pKeyboardInfo->bAltCharsAdded = 1;

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pReaderInfo           Reader context.
 * @param pKeyboardInfo         .
 * @param psDst                 .
 * @param pnDst                 .
 * @param psDedupe              .
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

static ET9STATUS ET9LOCALCALL __XmlReader_AppendAltCharsUpper(ET9KdbXmlReaderInfo      * const pReaderInfo,
                                                              ET9KdbXmlKeyboardInfo    * const pKeyboardInfo,
                                                              ET9SYMB                  * const psDst,
                                                              ET9UINT                  * const pnDst,
                                                              ET9SYMB                  * const psDedupe)
{
    ET9UINT nCount;
    ET9SYMB *psSymb;

    psSymb = psDst;
    for (nCount = *pnDst; nCount; --nCount, ++psSymb) {

        ET9SYMB const *psAlts = __XmlReader_AltCharsUpper(*psSymb);

        if (psAlts) {

            ET9STATUS eStatus;
            ET9UINT nAltCount;

            for (nAltCount = 0; psAlts[nAltCount]; ++nAltCount) {
            }

            eStatus = __XmlReader_AppendString(pReaderInfo, pKeyboardInfo, psDst, pnDst, psAlts, nAltCount, ET9_xml_copyOp_none, psDedupe);

            if (eStatus) {
                return eStatus;
            }
        }
    }

    pKeyboardInfo->bAltCharsAdded = 1;

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pReaderInfo           Reader context.
 * @param pKeyboardInfo         Reader context.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

static void ET9LOCALCALL __XmlReader_PruneAltChars(ET9KdbXmlReaderInfo   * const pReaderInfo,
                                                   ET9KdbXmlKeyboardInfo * const pKeyboardInfo)
{
    ET9SYMB * const psDedupe = &pReaderInfo->pKDBInfo->Private.wm.xmlReader.psDedupe[0];

    ET9UINT nCount;
    ET9KdbXmlKey *pKey;

    WLOG5(fprintf(pLogFile5, "__XmlReader_PruneAltChars\n");)

    if (!pKeyboardInfo->bAltCharsAdded) {
        return;
    }

    _ET9ClearMem((ET9U8*)psDedupe, ET9_KDB_XML_MAX_DEDUPE_CHARS * sizeof(ET9SYMB));

    pKey = &pKeyboardInfo->pKeyPool[0];
    for (nCount = pKeyboardInfo->nUsedKeys; nCount; --nCount, ++pKey) {

        if (pKey->eKeyType == ET9_xml_keyType_regional || pKey->eKeyType == ET9_xml_keyType_nonRegional) {

            const ET9SYMB sSymb = pKey->psKeyChars[0];

            const ET9UINT nHash = sSymb % ET9_KDB_XML_MAX_DEDUPE_CHARS;

            if (!psDedupe[nHash]) {
                psDedupe[nHash] = sSymb;
            }
            else if (psDedupe[nHash] == sSymb) {
                continue;
            }
            else {

                ET9UINT nLook;

                for (nLook = nHash; psDedupe[nLook]; ) {
                    ++nLook;
                    if (nLook >= ET9_KDB_XML_MAX_DEDUPE_CHARS) {
                        nLook = 0;
                    }
                }

                psDedupe[nLook] = sSymb;
            }
        }
    }

    pKey = &pKeyboardInfo->pKeyPool[0];
    for (nCount = pKeyboardInfo->nUsedKeys; nCount; --nCount, ++pKey) {

        if (pKey->eKeyType == ET9_xml_keyType_regional || pKey->eKeyType == ET9_xml_keyType_nonRegional) {

            ET9UINT nIndex;

            for (nIndex = 1; nIndex < pKey->nKeyCharCount; ++nIndex) {

                const ET9SYMB sSymb = pKey->psKeyChars[nIndex];

                const ET9UINT nHash = sSymb % ET9_KDB_XML_MAX_DEDUPE_CHARS;

                if (!psDedupe[nHash]) {
                    continue;
                }
                else if (psDedupe[nHash] == sSymb) {
                }
                else {

                    ET9UINT nLook;

                    for (nLook = nHash; psDedupe[nLook] && psDedupe[nLook] != sSymb; ) {
                        ++nLook;
                        if (nLook >= ET9_KDB_XML_MAX_DEDUPE_CHARS) {
                            nLook = 0;
                        }
                    }

                    if (!psDedupe[nLook]) {
                        continue;
                    }
                }

                /* remove char */

                pKey->psKeyChars[nIndex] = pKey->psKeyChars[pKey->nKeyCharCount - 1];
                --pKey->nKeyCharCount;
                --nIndex;
            }
        }
    }
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pReaderInfo           Reader context.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

static ET9STATUS ET9LOCALCALL __XmlReader_ReadKeyAttributes(ET9KdbXmlReaderInfo  * const pReaderInfo)
{
    ET9STATUS eStatus;
    ET9KdbXmlReaderContext * const pContext = &pReaderInfo->pContexts[ET9_xml_E_key];
    const ET9UINT nKeySourceLine = pReaderInfo->nCurrLine;

    WLOG5(fprintf(pLogFile5, "__XmlReader_ReadKeyAttributes\n");)

    if (pReaderInfo->eCurrToken == ET9_xml_token_Error) {
        return ET9STATUS_KDB_SYNTAX_ERROR;
    }

    *pContext = *(pContext - 1);
    _ET9ClearMem((ET9U8*)pContext->pbAttributeSet, sizeof(pContext->pbAttributeSet));

    while (pReaderInfo->eCurrToken != ET9_xml_token_TagEnd &&
           pReaderInfo->eCurrToken != ET9_xml_token_TagEndFinal) {

        eStatus = __XmlReader_ReadAttribute(pReaderInfo);

        if (eStatus) {
            return eStatus;
        }

        if (pReaderInfo->eCurrAttribute == ET9_xml_token_A_unknown) {
            continue;
        }

        if (pContext->pbAttributeSet[pReaderInfo->eCurrAttribute]) {
            return ET9STATUS_KDB_DUPLICATE_ATTRIBUTE;
        }

        pContext->pbAttributeSet[pReaderInfo->eCurrAttribute] = 1;

        switch (pReaderInfo->eCurrAttribute)
        {
            case ET9_xml_token_A_keyType:
                pContext->eKeyType = __XmlReader_ConstantToKeyType(pReaderInfo->eCurrConstant);
                break;
            case ET9_xml_token_A_keyTop:
                pContext->fKeyTop = pReaderInfo->fCurrFloat;
                pContext->eKeyTopSuffix = pReaderInfo->eCurrValueSuffix;
                break;
            case ET9_xml_token_A_keyLeft:
                pContext->fKeyLeft = pReaderInfo->fCurrFloat;
                pContext->eKeyLeftSuffix = pReaderInfo->eCurrValueSuffix;
                break;
            case ET9_xml_token_A_keyWidth:
                pContext->fKeyWidth = pReaderInfo->fCurrFloat;
                pContext->eKeyWidthSuffix = pReaderInfo->eCurrValueSuffix;
                break;
            case ET9_xml_token_A_keyHeight:
                pContext->fKeyHeight = pReaderInfo->fCurrFloat;
                pContext->eKeyHeightSuffix = pReaderInfo->eCurrValueSuffix;
                break;
            case ET9_xml_token_A_horizontalGap:
                pContext->fHorizontalGap = pReaderInfo->fCurrFloat;
                pContext->eHorizontalGapSuffix = pReaderInfo->eCurrValueSuffix;
                break;
            case ET9_xml_token_A_keyIcon:
                if (pReaderInfo->nCurrCharCount > ET9_KDB_XML_MAX_ICON_CHARS) {
                    return ET9STATUS_KDB_ATTRIBUTE_VALUE_ERROR;
                }
                pContext->nIconCharCount = pReaderInfo->nCurrCharCount;
                _ET9SymCopy(pContext->psIconChars, pReaderInfo->psCurrChars, pReaderInfo->nCurrCharCount);
                break;
            case ET9_xml_token_A_keyName:
                if (pReaderInfo->nCurrCharCount > ET9_KDB_MAX_KEY_CHARS) {
                    return ET9STATUS_KDB_ATTRIBUTE_VALUE_ERROR;
                }
                pContext->nKeyCharCount = pReaderInfo->nCurrCharCount;
                _ET9SymCopy(pContext->psKeyChars, pReaderInfo->psCurrChars, pReaderInfo->nCurrCharCount);
                break;
            case ET9_xml_token_A_keyLabel:
                if (pReaderInfo->nCurrCharCount > ET9_KDB_XML_MAX_LABEL_CHARS) {
                    return ET9STATUS_KDB_ATTRIBUTE_VALUE_ERROR;
                }
                pContext->nLabelCharCount = pReaderInfo->nCurrCharCount;
                _ET9SymCopy(pContext->psLabelChars, pReaderInfo->psCurrChars, pReaderInfo->nCurrCharCount);
                break;
            case ET9_xml_token_A_keyLabelShifted:
                if (pReaderInfo->nCurrCharCount > ET9_KDB_XML_MAX_LABEL_CHARS) {
                    return ET9STATUS_KDB_ATTRIBUTE_VALUE_ERROR;
                }
                pContext->nLabelShiftedCharCount = pReaderInfo->nCurrCharCount;
                _ET9SymCopy(pContext->psLabelShiftedChars, pReaderInfo->psCurrChars, pReaderInfo->nCurrCharCount);
                break;
            case ET9_xml_token_A_keyCodes:
                if (pReaderInfo->nCurrCharCount > ET9_KDB_MAX_KEY_CHARS) {
                    return ET9STATUS_KDB_ATTRIBUTE_VALUE_ERROR;
                }
                pContext->nKeyCharCount = pReaderInfo->nCurrCharCount;
                _ET9SymCopy(pContext->psKeyChars, pReaderInfo->psCurrChars, pReaderInfo->nCurrCharCount);
                break;
            case ET9_xml_token_A_keyCodesShifted:
                if (pReaderInfo->nCurrCharCount > ET9_KDB_MAX_KEY_CHARS) {
                    return ET9STATUS_KDB_ATTRIBUTE_VALUE_ERROR;
                }
                pContext->nKeyShiftedCharCount = pReaderInfo->nCurrCharCount;
                _ET9SymCopy(pContext->psKeyShiftedChars, pReaderInfo->psCurrChars, pReaderInfo->nCurrCharCount);
                break;
            case ET9_xml_token_A_keyPopupChars:
            case ET9_xml_token_A_keyPopupCodes:
                if (pReaderInfo->nCurrCharCount > ET9_KDB_XML_MAX_POPUP_CHARS) {
                    return ET9STATUS_KDB_ATTRIBUTE_VALUE_ERROR;
                }
                pContext->nKeyPopupCharCount = pReaderInfo->nCurrCharCount;
                _ET9SymCopy(pContext->psKeyPopupChars, pReaderInfo->psCurrChars, pReaderInfo->nCurrCharCount);
                break;
            case ET9_xml_token_A_keyPopupShiftedChars:
            case ET9_xml_token_A_keyPopupShiftedCodes:
                if (pReaderInfo->nCurrCharCount > ET9_KDB_XML_MAX_POPUP_CHARS) {
                    return ET9STATUS_KDB_ATTRIBUTE_VALUE_ERROR;
                }
                pContext->nKeyPopupShiftedCharCount = pReaderInfo->nCurrCharCount;
                _ET9SymCopy(pContext->psKeyPopupShiftedChars, pReaderInfo->psCurrChars, pReaderInfo->nCurrCharCount);
                break;
            case ET9_xml_token_A_keyMultitapChars:
            case ET9_xml_token_A_keyMultitapCodes:
                if (pReaderInfo->nCurrCharCount > ET9_KDB_XML_MAX_MULTITAP_CHARS) {
                    return ET9STATUS_KDB_ATTRIBUTE_VALUE_ERROR;
                }
                pContext->nKeyMultitapCharCount = pReaderInfo->nCurrCharCount;
                _ET9SymCopy(pContext->psKeyMultitapChars, pReaderInfo->psCurrChars, pReaderInfo->nCurrCharCount);
                break;
            case ET9_xml_token_A_keyMultitapShiftedChars:
            case ET9_xml_token_A_keyMultitapShiftedCodes:
                if (pReaderInfo->nCurrCharCount > ET9_KDB_XML_MAX_MULTITAP_CHARS) {
                    return ET9STATUS_KDB_ATTRIBUTE_VALUE_ERROR;
                }
                pContext->nKeyMultitapShiftedCharCount = pReaderInfo->nCurrCharCount;
                _ET9SymCopy(pContext->psKeyMultitapShiftedChars, pReaderInfo->psCurrChars, pReaderInfo->nCurrCharCount);
                break;
            default:
                return ET9STATUS_KDB_UNEXPECTED_ATTRIBUTE;
        }
    }

    if (!pContext->pbAttributeSet[ET9_xml_token_A_keyType]) {
        return ET9STATUS_KDB_MISSING_ATTRIBUTE;
    }

    if (pContext->pbAttributeSet[ET9_xml_token_A_keyLabelShifted] &&
        !pContext->pbAttributeSet[ET9_xml_token_A_keyLabel]) {
        return ET9STATUS_KDB_MISSING_ATTRIBUTE;
    }

    if (pContext->pbAttributeSet[ET9_xml_token_A_keyCodesShifted] &&
        !pContext->pbAttributeSet[ET9_xml_token_A_keyCodes]) {
        return ET9STATUS_KDB_MISSING_ATTRIBUTE;
    }

    if (pContext->pbAttributeSet[ET9_xml_token_A_keyPopupChars] &&
        pContext->pbAttributeSet[ET9_xml_token_A_keyPopupCodes]) {
        return ET9STATUS_KDB_UNEXPECTED_ATTRIBUTE;
    }

    if (pContext->pbAttributeSet[ET9_xml_token_A_keyPopupShiftedChars] &&
        pContext->pbAttributeSet[ET9_xml_token_A_keyPopupShiftedCodes]) {
        return ET9STATUS_KDB_UNEXPECTED_ATTRIBUTE;
    }

    if (pContext->pbAttributeSet[ET9_xml_token_A_keyMultitapChars] &&
        pContext->pbAttributeSet[ET9_xml_token_A_keyMultitapCodes]) {
        return ET9STATUS_KDB_UNEXPECTED_ATTRIBUTE;
    }

    if (pContext->pbAttributeSet[ET9_xml_token_A_keyMultitapShiftedChars] &&
        pContext->pbAttributeSet[ET9_xml_token_A_keyMultitapShiftedCodes]) {
        return ET9STATUS_KDB_UNEXPECTED_ATTRIBUTE;
    }

    if (pContext->pbAttributeSet[ET9_xml_token_A_keyMultitapShiftedChars] &&
        !pContext->pbAttributeSet[ET9_xml_token_A_keyMultitapChars]) {
        return ET9STATUS_KDB_MISSING_ATTRIBUTE;
    }

    if (pContext->pbAttributeSet[ET9_xml_token_A_keyMultitapShiftedCodes] &&
        !pContext->pbAttributeSet[ET9_xml_token_A_keyMultitapCodes]) {
        return ET9STATUS_KDB_MISSING_ATTRIBUTE;
    }

    if (pContext->eKeyType == ET9_xml_keyType_function) {
        if (pContext->pbAttributeSet[ET9_xml_token_A_keyLabelShifted] ||
            pContext->pbAttributeSet[ET9_xml_token_A_keyCodes] ||
            pContext->pbAttributeSet[ET9_xml_token_A_keyCodesShifted] ||
            pContext->pbAttributeSet[ET9_xml_token_A_keyPopupChars] ||
            pContext->pbAttributeSet[ET9_xml_token_A_keyPopupShiftedChars] ||
            pContext->pbAttributeSet[ET9_xml_token_A_keyPopupCodes] ||
            pContext->pbAttributeSet[ET9_xml_token_A_keyPopupShiftedCodes] ||
            pContext->pbAttributeSet[ET9_xml_token_A_keyMultitapChars] ||
            pContext->pbAttributeSet[ET9_xml_token_A_keyMultitapShiftedChars] ||
            pContext->pbAttributeSet[ET9_xml_token_A_keyMultitapCodes] ||
            pContext->pbAttributeSet[ET9_xml_token_A_keyMultitapShiftedCodes]) {
            return ET9STATUS_KDB_UNEXPECTED_ATTRIBUTE;
        }
        if (!pContext->pbAttributeSet[ET9_xml_token_A_keyName]) {
            return ET9STATUS_KDB_MISSING_ATTRIBUTE;
        }
    }
    else {
        if (pContext->pbAttributeSet[ET9_xml_token_A_keyName]) {
            return ET9STATUS_KDB_UNEXPECTED_ATTRIBUTE;
        }
    }

    if (pContext->eRowType == ET9_xml_rowType_keys) {
        if (pContext->pbAttributeSet[ET9_xml_token_A_keyTop] ||
            pContext->pbAttributeSet[ET9_xml_token_A_keyLeft] ||
            pContext->pbAttributeSet[ET9_xml_token_A_keyHeight]) {
            return ET9STATUS_KDB_UNEXPECTED_ATTRIBUTE;
        }
    }
    else if (pContext->eRowType == ET9_xml_rowType_area) {
        if (!pContext->pbAttributeSet[ET9_xml_token_A_keyTop] ||
            !pContext->pbAttributeSet[ET9_xml_token_A_keyLeft]) {
            return ET9STATUS_KDB_MISSING_ATTRIBUTE;
        }
        if (pContext->eKeyTopSuffix == ET9_xml_valueSuffix_undef ||
            pContext->eKeyLeftSuffix == ET9_xml_valueSuffix_undef ||
            pContext->eKeyWidthSuffix == ET9_xml_valueSuffix_undef ||
            pContext->eKeyHeightSuffix == ET9_xml_valueSuffix_undef) {
            return ET9STATUS_KDB_MISSING_ATTRIBUTE;
        }
    }

    if (pContext->nConditionValue == pReaderInfo->nConditionValue) {

        ET9KDBPrivate * const pPrivate = &pReaderInfo->pKDBInfo->Private;
        ET9SYMB * const psDedupe = &pReaderInfo->pKDBInfo->Private.wm.xmlReader.psDedupe[0];
        ET9KdbXmlKeyboardInfo * const pKeyboardInfo = &pPrivate->wm.xmlReader.sKeyboardInfo;
        ET9KdbXmlKeyboard * const pKeyboard = &pKeyboardInfo->sKeyboard;
        ET9KdbXmlRow * const pRow = &pKeyboard->pRows[pKeyboard->nRowCount - 1];
        ET9KdbXmlKey * const pKey = &pRow->pKeys[pRow->nKeyCount];

        const ET9BOOL bAddAltChars = (pContext->eAddAltChars != ET9_xml_booleanValue_no && (pContext->eKeyType == ET9_xml_keyType_regional || pContext->eKeyType == ET9_xml_keyType_nonRegional)) ? 1 : 0;

        if (pKeyboardInfo->nUsedKeys >= ET9_KDB_MAX_KEYS) {
            return ET9STATUS_KDB_HAS_TOO_MANY_KEYS;
        }

        ++pRow->nKeyCount;
        ++pKeyboardInfo->nUsedKeys;

        pKey->nSourceLine           = nKeySourceLine;

        pKey->fKeyWidth             = pContext->fKeyWidth;
        pKey->eKeyWidthSuffix       = pContext->eKeyWidthSuffix;

        pKey->fHorizontalGap        = pContext->fHorizontalGap;
        pKey->eHorizontalGapSuffix  = pContext->eHorizontalGapSuffix;

        pKey->eKeyType              = pContext->eKeyType;


        pKey->fKeyTop               = pContext->fKeyTop;
        pKey->eKeyTopSuffix         = pContext->eKeyTopSuffix;

        pKey->fKeyLeft              = pContext->fKeyLeft;
        pKey->eKeyLeftSuffix        = pContext->eKeyLeftSuffix;

        pKey->fKeyHeight            = pContext->fKeyHeight;
        pKey->eKeyHeightSuffix      = pContext->eKeyHeightSuffix;

        eStatus = __XmlReader_CreateString(pReaderInfo,
                                           pKeyboardInfo,
                                           &pKey->psIconChars,
                                           &pKey->nIconCharCount,
                                           pContext->psIconChars,
                                           pContext->nIconCharCount,
                                           ET9_xml_copyOp_none,
                                           NULL);

        if (eStatus) {
            return eStatus;
        }

        eStatus = __XmlReader_CreateString(pReaderInfo,
                                           pKeyboardInfo,
                                           &pKey->psLabelChars,
                                           &pKey->nLabelCharCount,
                                           pContext->psLabelChars,
                                           pContext->nLabelCharCount,
                                           ET9_xml_copyOp_none,
                                           NULL);

        if (eStatus) {
            return eStatus;
        }

        eStatus = __XmlReader_CreateString(pReaderInfo,
                                           pKeyboardInfo,
                                           &pKey->psLabelShiftedChars,
                                           &pKey->nLabelShiftedCharCount,
                                           pContext->psLabelShiftedChars,
                                           pContext->nLabelShiftedCharCount,
                                           ET9_xml_copyOp_none,
                                           NULL);

        if (eStatus) {
            return eStatus;
        }

        eStatus = __XmlReader_CreateString(pReaderInfo,
                                           pKeyboardInfo,
                                           &pKey->psKeyPopupChars,
                                           &pKey->nKeyPopupCharCount,
                                           pContext->psKeyPopupChars,
                                           pContext->nKeyPopupCharCount,
                                           ET9_xml_copyOp_none,
                                           NULL);

        if (eStatus) {
            return eStatus;
        }

        eStatus = __XmlReader_CreateString(pReaderInfo,
                                           pKeyboardInfo,
                                           &pKey->psKeyPopupShiftedChars,
                                           &pKey->nKeyPopupShiftedCharCount,
                                           pContext->psKeyPopupShiftedChars,
                                           pContext->nKeyPopupShiftedCharCount,
                                           ET9_xml_copyOp_none,
                                           NULL);

        if (eStatus) {
            return eStatus;
        }

        eStatus = __XmlReader_CreateString(pReaderInfo,
                                           pKeyboardInfo,
                                           &pKey->psKeyMultitapChars,
                                           &pKey->nKeyMultitapCharCount,
                                           pContext->psKeyMultitapChars,
                                           pContext->nKeyMultitapCharCount,
                                           ET9_xml_copyOp_none,
                                           NULL);

        if (eStatus) {
            return eStatus;
        }

        eStatus = __XmlReader_CreateString(pReaderInfo,
                                           pKeyboardInfo,
                                           &pKey->psKeyMultitapShiftedChars,
                                           &pKey->nKeyMultitapShiftedCharCount,
                                           pContext->psKeyMultitapShiftedChars,
                                           pContext->nKeyMultitapShiftedCharCount,
                                           ET9_xml_copyOp_none,
                                           NULL);

        if (eStatus) {
            return eStatus;
        }

        /* unshifted */

        eStatus = __XmlReader_CreateString(pReaderInfo,
                                           pKeyboardInfo,
                                           &pKey->psKeyChars,
                                           &pKey->nKeyCharCount,
                                           NULL,
                                           0,
                                           ET9_xml_copyOp_none,
                                           psDedupe);

        if (eStatus) {
            return eStatus;
        }

        if (pKey->eKeyType != ET9_xml_keyType_function) {

            eStatus = __XmlReader_AppendString(pReaderInfo,
                                               pKeyboardInfo,
                                               pKey->psKeyChars,
                                               &pKey->nKeyCharCount,
                                               pContext->psLabelChars,
                                               pContext->nLabelCharCount,
                                               pKey->eKeyType == ET9_xml_keyType_string ? ET9_xml_copyOp_none : ET9_xml_copyOp_toLower,
                                               psDedupe);

            if (eStatus) {
                return eStatus;
            }

            eStatus = __XmlReader_AppendString(pReaderInfo,
                                               pKeyboardInfo,
                                               pKey->psKeyChars,
                                               &pKey->nKeyCharCount,
                                               pContext->psLabelShiftedChars,
                                               pContext->nLabelShiftedCharCount,
                                               ET9_xml_copyOp_toLower,
                                               psDedupe);

            if (eStatus) {
                return eStatus;
            }
        }

        eStatus = __XmlReader_AppendString(pReaderInfo,
                                           pKeyboardInfo,
                                           pKey->psKeyChars,
                                           &pKey->nKeyCharCount,
                                           pContext->psKeyMultitapChars,
                                           pContext->nKeyMultitapCharCount,
                                           ET9_xml_copyOp_none,
                                           psDedupe);

        if (eStatus) {
            return eStatus;
        }

        eStatus = __XmlReader_AppendString(pReaderInfo,
                                           pKeyboardInfo,
                                           pKey->psKeyChars,
                                           &pKey->nKeyCharCount,
                                           pContext->psKeyMultitapShiftedChars,
                                           pContext->nKeyMultitapShiftedCharCount,
                                           ET9_xml_copyOp_toLower,
                                           psDedupe);

        if (eStatus) {
            return eStatus;
        }

        eStatus = __XmlReader_AppendString(pReaderInfo,
                                           pKeyboardInfo,
                                           pKey->psKeyChars,
                                           &pKey->nKeyCharCount,
                                           pContext->psKeyChars,
                                           pContext->nKeyCharCount,
                                           ET9_xml_copyOp_none,
                                           psDedupe);

        if (eStatus) {
            return eStatus;
        }

        eStatus = __XmlReader_AppendString(pReaderInfo,
                                           pKeyboardInfo,
                                           pKey->psKeyChars,
                                           &pKey->nKeyCharCount,
                                           pContext->psKeyShiftedChars,
                                           pContext->nKeyShiftedCharCount,
                                           ET9_xml_copyOp_toLower,
                                           psDedupe);

        if (eStatus) {
            return eStatus;
        }

        eStatus = __XmlReader_AppendString(pReaderInfo,
                                           pKeyboardInfo,
                                           pKey->psKeyChars,
                                           &pKey->nKeyCharCount,
                                           pContext->psKeyPopupChars,
                                           pContext->nKeyPopupCharCount,
                                           ET9_xml_copyOp_none,
                                           psDedupe);

        if (eStatus) {
            return eStatus;
        }

        eStatus = __XmlReader_AppendString(pReaderInfo,
                                           pKeyboardInfo,
                                           pKey->psKeyChars,
                                           &pKey->nKeyCharCount,
                                           pContext->psKeyPopupShiftedChars,
                                           pContext->nKeyPopupShiftedCharCount,
                                           ET9_xml_copyOp_toLower,
                                           psDedupe);

        if (eStatus) {
            return eStatus;
        }

        if (bAddAltChars) {

            eStatus = __XmlReader_AppendAltCharsLower(pReaderInfo,
                                                      pKeyboardInfo,
                                                      pKey->psKeyChars,
                                                      &pKey->nKeyCharCount,
                                                      psDedupe);

            if (eStatus) {
                return eStatus;
            }
        }

        /* shifted */

        eStatus = __XmlReader_CreateString(pReaderInfo,
                                           pKeyboardInfo,
                                           &pKey->psKeyShiftedChars,
                                           &pKey->nKeyShiftedCharCount,
                                           NULL,
                                           0,
                                           ET9_xml_copyOp_none,
                                           psDedupe);

        if (eStatus) {
            return eStatus;
        }

        if (pKey->eKeyType != ET9_xml_keyType_function) {

            eStatus = __XmlReader_AppendString(pReaderInfo,
                                               pKeyboardInfo,
                                               pKey->psKeyShiftedChars,
                                               &pKey->nKeyShiftedCharCount,
                                               pContext->psLabelShiftedChars,
                                               pContext->nLabelShiftedCharCount,
                                               ET9_xml_copyOp_none,
                                               psDedupe);

            if (eStatus) {
                return eStatus;
            }
        }

        eStatus = __XmlReader_AppendString(pReaderInfo,
                                           pKeyboardInfo,
                                           pKey->psKeyShiftedChars,
                                           &pKey->nKeyShiftedCharCount,
                                           pContext->psKeyMultitapShiftedChars,
                                           pContext->nKeyMultitapShiftedCharCount,
                                           ET9_xml_copyOp_none,
                                           psDedupe);

        if (eStatus) {
            return eStatus;
        }

        eStatus = __XmlReader_AppendString(pReaderInfo,
                                           pKeyboardInfo,
                                           pKey->psKeyShiftedChars,
                                           &pKey->nKeyShiftedCharCount,
                                           pContext->psKeyShiftedChars,
                                           pContext->nKeyShiftedCharCount,
                                           ET9_xml_copyOp_none,
                                           psDedupe);

        if (eStatus) {
            return eStatus;
        }

        eStatus = __XmlReader_AppendString(pReaderInfo,
                                           pKeyboardInfo,
                                           pKey->psKeyShiftedChars,
                                           &pKey->nKeyShiftedCharCount,
                                           pContext->psKeyPopupShiftedChars,
                                           pContext->nKeyPopupShiftedCharCount,
                                           ET9_xml_copyOp_none,
                                           psDedupe);

        if (eStatus) {
            return eStatus;
        }

        if (bAddAltChars) {

            eStatus = __XmlReader_AppendAltCharsUpper(pReaderInfo,
                                                      pKeyboardInfo,
                                                      pKey->psKeyShiftedChars,
                                                      &pKey->nKeyShiftedCharCount,
                                                      psDedupe);

            if (eStatus) {
                return eStatus;
            }
        }

        /* must have chars by now */

        if (!pKey->nKeyCharCount) {
            return ET9STATUS_KDB_KEY_HAS_TOO_FEW_CHARS;
        }
    }

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pReaderInfo           Reader context.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

static ET9STATUS ET9LOCALCALL __XmlReader_ReadKey(ET9KdbXmlReaderInfo  * const pReaderInfo)
{
    ET9STATUS eStatus;

    WLOG5(fprintf(pLogFile5, "__XmlReader_ReadKey\n");)

    if (pReaderInfo->eCurrToken == ET9_xml_token_Error) {
        return ET9STATUS_KDB_SYNTAX_ERROR;
    }

    if (!__XmlReader_ConsumeToken(pReaderInfo, ET9_xml_token_TagStartKey)) {
        return ET9STATUS_KDB_SYNTAX_ERROR;
    }

    eStatus = __XmlReader_ReadKeyAttributes(pReaderInfo);

    if (eStatus) {
        return eStatus;
    }

    if (!__XmlReader_ConsumeToken(pReaderInfo, ET9_xml_token_TagEndFinal)) {
        return ET9STATUS_KDB_SYNTAX_ERROR;
    }

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pReaderInfo           Reader context.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

static ET9STATUS ET9LOCALCALL __XmlReader_ReadKeyVoidAttributes(ET9KdbXmlReaderInfo  * const pReaderInfo)
{
    ET9STATUS eStatus;
    ET9KdbXmlReaderContext * const pContext = &pReaderInfo->pContexts[ET9_xml_E_key];

    WLOG5(fprintf(pLogFile5, "__XmlReader_ReadKeyVoidAttributes\n");)

    if (pReaderInfo->eCurrToken == ET9_xml_token_Error) {
        return ET9STATUS_KDB_SYNTAX_ERROR;
    }

    *pContext = *(pContext - 1);
    _ET9ClearMem((ET9U8*)pContext->pbAttributeSet, sizeof(pContext->pbAttributeSet));

    while (pReaderInfo->eCurrToken != ET9_xml_token_TagEnd &&
           pReaderInfo->eCurrToken != ET9_xml_token_TagEndFinal) {

        eStatus = __XmlReader_ReadAttribute(pReaderInfo);

        if (eStatus) {
            return eStatus;
        }

        if (pReaderInfo->eCurrAttribute == ET9_xml_token_A_unknown) {
            continue;
        }

        if (pContext->pbAttributeSet[pReaderInfo->eCurrAttribute]) {
            return ET9STATUS_KDB_DUPLICATE_ATTRIBUTE;
        }

        pContext->pbAttributeSet[pReaderInfo->eCurrAttribute] = 1;

        switch (pReaderInfo->eCurrAttribute)
        {
            case ET9_xml_token_A_voidSize:
                pContext->fVoidSize = pReaderInfo->fCurrFloat;
                pContext->eVoidSizeSuffix = pReaderInfo->eCurrValueSuffix;
                break;
            case ET9_xml_token_A_unknown:
            default:
                return ET9STATUS_KDB_UNEXPECTED_ATTRIBUTE;
        }
    }

    if (pContext->nConditionValue == pReaderInfo->nConditionValue) {

        ET9KDBPrivate * const pPrivate = &pReaderInfo->pKDBInfo->Private;
        ET9KdbXmlKeyboardInfo * const pKeyboardInfo = &pPrivate->wm.xmlReader.sKeyboardInfo;
        ET9KdbXmlKeyboard * const pKeyboard = &pKeyboardInfo->sKeyboard;
        ET9KdbXmlRow * const pRow = &pKeyboard->pRows[pKeyboard->nRowCount - 1];
        ET9KdbXmlKey * const pKey = &pRow->pKeys[pRow->nKeyCount];

        if (pKeyboardInfo->nUsedKeys >= ET9_KDB_MAX_KEYS) {
            return ET9STATUS_KDB_HAS_TOO_MANY_KEYS;
        }

        ++pRow->nKeyCount;
        ++pKeyboardInfo->nUsedKeys;

        pKey->eKeyType = ET9_xml_keyType_void;

        pKey->fVoidSize             = pContext->fVoidSize;
        pKey->eVoidSizeSuffix       = pContext->eVoidSizeSuffix;
    }

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pReaderInfo           Reader context.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

static ET9STATUS ET9LOCALCALL __XmlReader_ReadKeyVoid(ET9KdbXmlReaderInfo  * const pReaderInfo)
{
    ET9STATUS eStatus;

    WLOG5(fprintf(pLogFile5, "__XmlReader_ReadKeyVoid\n");)

    if (pReaderInfo->eCurrToken == ET9_xml_token_Error) {
        return ET9STATUS_KDB_SYNTAX_ERROR;
    }

    if (!__XmlReader_ConsumeToken(pReaderInfo, ET9_xml_token_TagStartVoid)) {
        return ET9STATUS_KDB_SYNTAX_ERROR;
    }

    eStatus = __XmlReader_ReadKeyVoidAttributes(pReaderInfo);

    if (eStatus) {
        return eStatus;
    }

    if (!__XmlReader_ConsumeToken(pReaderInfo, ET9_xml_token_TagEndFinal)) {
        return ET9STATUS_KDB_SYNTAX_ERROR;
    }

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pReaderInfo           Reader context.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

static ET9STATUS ET9LOCALCALL __XmlReader_ReadRowAttributes(ET9KdbXmlReaderInfo  * const pReaderInfo)
{
    ET9STATUS eStatus;
    ET9KdbXmlReaderContext * const pContext = &pReaderInfo->pContexts[ET9_xml_E_row];

    WLOG5(fprintf(pLogFile5, "__XmlReader_ReadRowAttributes\n");)

    if (pReaderInfo->eCurrToken == ET9_xml_token_Error) {
        return ET9STATUS_KDB_SYNTAX_ERROR;
    }

    *pContext = *(pContext - 1);
    _ET9ClearMem((ET9U8*)pContext->pbAttributeSet, sizeof(pContext->pbAttributeSet));

    pContext->eRowType = ET9_xml_rowType_keys;

    while (pReaderInfo->eCurrToken != ET9_xml_token_TagEnd &&
           pReaderInfo->eCurrToken != ET9_xml_token_TagEndFinal) {

        eStatus = __XmlReader_ReadAttribute(pReaderInfo);

        if (eStatus) {
            return eStatus;
        }

        if (pReaderInfo->eCurrAttribute == ET9_xml_token_A_unknown) {
            continue;
        }

        if (pContext->pbAttributeSet[pReaderInfo->eCurrAttribute]) {
            return ET9STATUS_KDB_DUPLICATE_ATTRIBUTE;
        }

        pContext->pbAttributeSet[pReaderInfo->eCurrAttribute] = 1;

        switch (pReaderInfo->eCurrAttribute)
        {
            case ET9_xml_token_A_keyWidth:
                pContext->fKeyWidth = pReaderInfo->fCurrFloat;
                pContext->eKeyWidthSuffix = pReaderInfo->eCurrValueSuffix;
                break;
            case ET9_xml_token_A_keyHeight:
                pContext->fKeyHeight = pReaderInfo->fCurrFloat;
                pContext->eKeyHeightSuffix = pReaderInfo->eCurrValueSuffix;
                break;
            case ET9_xml_token_A_verticalGap:
                pContext->fVerticalGap = pReaderInfo->fCurrFloat;
                pContext->eVerticalGapSuffix = pReaderInfo->eCurrValueSuffix;
                break;
            case ET9_xml_token_A_horizontalGap:
                pContext->fHorizontalGap = pReaderInfo->fCurrFloat;
                pContext->eHorizontalGapSuffix = pReaderInfo->eCurrValueSuffix;
                break;
            case ET9_xml_token_A_conditionValue:
                pContext->nConditionValue = pReaderInfo->nCurrInt;
                break;
            default:
                return ET9STATUS_KDB_UNEXPECTED_ATTRIBUTE;
        }
    }

    if (pContext->nConditionValue == pReaderInfo->nConditionValue) {

        ET9KDBPrivate * const pPrivate = &pReaderInfo->pKDBInfo->Private;
        ET9KdbXmlKeyboardInfo * const pKeyboardInfo = &pPrivate->wm.xmlReader.sKeyboardInfo;
        ET9KdbXmlKeyboard * const pKeyboard = &pKeyboardInfo->sKeyboard;
        ET9KdbXmlRow * const pRow = &pKeyboard->pRows[pKeyboard->nRowCount];

        if (pKeyboardInfo->nUsedRows >= ET9_KDB_MAX_ROWS) {
            return ET9STATUS_KDB_HAS_TOO_MANY_ROWS;
        }

        ++pKeyboard->nRowCount;
        ++pKeyboardInfo->nUsedRows;

        pRow->eRowType = pContext->eRowType;

        pRow->nKeyCount = 0;
        pRow->pKeys = &pKeyboardInfo->pKeyPool[pKeyboardInfo->nUsedKeys];

        pRow->fKeyHeight            = pContext->fKeyHeight;
        pRow->eKeyHeightSuffix      = pContext->eKeyHeightSuffix;

        pRow->fVerticalGap          = pContext->fVerticalGap;
        pRow->eVerticalGapSuffix    = pContext->eVerticalGapSuffix;
    }

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pReaderInfo           Reader context.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

static ET9STATUS ET9LOCALCALL __XmlReader_ReadRow(ET9KdbXmlReaderInfo  * const pReaderInfo)
{
    ET9STATUS eStatus;

    WLOG5(fprintf(pLogFile5, "__XmlReader_ReadRow\n");)

    if (pReaderInfo->eCurrToken == ET9_xml_token_Error) {
        return ET9STATUS_KDB_SYNTAX_ERROR;
    }

    if (!__XmlReader_ConsumeToken(pReaderInfo, ET9_xml_token_TagStartRow)) {
        return ET9STATUS_KDB_SYNTAX_ERROR;
    }

    eStatus = __XmlReader_ReadRowAttributes(pReaderInfo);

    if (eStatus) {
        return eStatus;
    }

    if (!__XmlReader_ConsumeToken(pReaderInfo, ET9_xml_token_TagEnd)) {
        return ET9STATUS_KDB_SYNTAX_ERROR;
    }

    while (pReaderInfo->eCurrToken == ET9_xml_token_TagStartKey ||
           pReaderInfo->eCurrToken == ET9_xml_token_TagStartVoid) {

        if (pReaderInfo->eCurrToken == ET9_xml_token_TagStartKey) {
            eStatus = __XmlReader_ReadKey(pReaderInfo);
        }
        else {
            eStatus = __XmlReader_ReadKeyVoid(pReaderInfo);
        }

        if (eStatus) {
            return eStatus;
        }
    }

    if (!(__XmlReader_ConsumeToken(pReaderInfo, ET9_xml_token_TagEndRow) &&
          __XmlReader_ConsumeToken(pReaderInfo, ET9_xml_token_TagEnd))) {
        return ET9STATUS_KDB_SYNTAX_ERROR;
    }

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pReaderInfo           Reader context.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

static ET9STATUS ET9LOCALCALL __XmlReader_ReadAreaAttributes(ET9KdbXmlReaderInfo  * const pReaderInfo)
{
    ET9STATUS eStatus;
    ET9KdbXmlReaderContext * const pContext = &pReaderInfo->pContexts[ET9_xml_E_row];

    WLOG5(fprintf(pLogFile5, "__XmlReader_ReadAreaAttributes\n");)

    if (pReaderInfo->eCurrToken == ET9_xml_token_Error) {
        return ET9STATUS_KDB_SYNTAX_ERROR;
    }

    *pContext = *(pContext - 1);
    _ET9ClearMem((ET9U8*)pContext->pbAttributeSet, sizeof(pContext->pbAttributeSet));

    pContext->eRowType = ET9_xml_rowType_area;

    while (pReaderInfo->eCurrToken != ET9_xml_token_TagEnd &&
           pReaderInfo->eCurrToken != ET9_xml_token_TagEndFinal) {

        eStatus = __XmlReader_ReadAttribute(pReaderInfo);

        if (eStatus) {
            return eStatus;
        }

        if (pReaderInfo->eCurrAttribute == ET9_xml_token_A_unknown) {
            continue;
        }

        if (pContext->pbAttributeSet[pReaderInfo->eCurrAttribute]) {
            return ET9STATUS_KDB_DUPLICATE_ATTRIBUTE;
        }

        pContext->pbAttributeSet[pReaderInfo->eCurrAttribute] = 1;

        switch (pReaderInfo->eCurrAttribute)
        {
            case ET9_xml_token_A_keyWidth:
                pContext->fKeyWidth = pReaderInfo->fCurrFloat;
                pContext->eKeyWidthSuffix = pReaderInfo->eCurrValueSuffix;
                break;
            case ET9_xml_token_A_keyHeight:
                pContext->fKeyHeight = pReaderInfo->fCurrFloat;
                pContext->eKeyHeightSuffix = pReaderInfo->eCurrValueSuffix;
                break;
            case ET9_xml_token_A_conditionValue:
                pContext->nConditionValue = pReaderInfo->nCurrInt;
                break;
            case ET9_xml_token_A_addAltChars:
                pContext->eAddAltChars = __XmlReader_ConstantToBooleanValue(pReaderInfo->eCurrConstant);
                break;

            default:
                return ET9STATUS_KDB_UNEXPECTED_ATTRIBUTE;
        }
    }

    if (pContext->nConditionValue == pReaderInfo->nConditionValue) {

        ET9KDBPrivate * const pPrivate = &pReaderInfo->pKDBInfo->Private;
        ET9KdbXmlKeyboardInfo * const pKeyboardInfo = &pPrivate->wm.xmlReader.sKeyboardInfo;
        ET9KdbXmlKeyboard * const pKeyboard = &pKeyboardInfo->sKeyboard;
        ET9KdbXmlRow * const pRow = &pKeyboard->pRows[pKeyboard->nRowCount];

        if (pKeyboardInfo->nUsedRows >= ET9_KDB_MAX_ROWS) {
            return ET9STATUS_KDB_HAS_TOO_MANY_ROWS;
        }

        ++pKeyboard->nRowCount;
        ++pKeyboardInfo->nUsedRows;

        pRow->eRowType = pContext->eRowType;

        pRow->nKeyCount = 0;
        pRow->pKeys = &pKeyboardInfo->pKeyPool[pKeyboardInfo->nUsedKeys];
    }

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pReaderInfo           Reader context.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

static ET9STATUS ET9LOCALCALL __XmlReader_ReadArea(ET9KdbXmlReaderInfo  * const pReaderInfo)
{
    ET9STATUS eStatus;

    WLOG5(fprintf(pLogFile5, "__XmlReader_ReadArea\n");)

    if (pReaderInfo->eCurrToken == ET9_xml_token_Error) {
        return ET9STATUS_KDB_SYNTAX_ERROR;
    }

    if (!__XmlReader_ConsumeToken(pReaderInfo, ET9_xml_token_TagStartArea)) {
        return ET9STATUS_KDB_SYNTAX_ERROR;
    }

    eStatus = __XmlReader_ReadAreaAttributes(pReaderInfo);

    if (eStatus) {
        return eStatus;
    }

    if (!__XmlReader_ConsumeToken(pReaderInfo, ET9_xml_token_TagEnd)) {
        return ET9STATUS_KDB_SYNTAX_ERROR;
    }

    while (pReaderInfo->eCurrToken == ET9_xml_token_TagStartKey) {

        eStatus = __XmlReader_ReadKey(pReaderInfo);

        if (eStatus) {
            return eStatus;
        }
    }

    if (!(__XmlReader_ConsumeToken(pReaderInfo, ET9_xml_token_TagEndArea) &&
          __XmlReader_ConsumeToken(pReaderInfo, ET9_xml_token_TagEnd))) {
        return ET9STATUS_KDB_SYNTAX_ERROR;
    }

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pReaderInfo           Reader context.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

static ET9STATUS ET9LOCALCALL __XmlReader_ReadRowVoidAttributes(ET9KdbXmlReaderInfo  * const pReaderInfo)
{
    ET9STATUS eStatus;
    ET9KdbXmlReaderContext * const pContext = &pReaderInfo->pContexts[ET9_xml_E_row];

    WLOG5(fprintf(pLogFile5, "__XmlReader_ReadRowVoidAttributes\n");)

    if (pReaderInfo->eCurrToken == ET9_xml_token_Error) {
        return ET9STATUS_KDB_SYNTAX_ERROR;
    }

    *pContext = *(pContext - 1);
    _ET9ClearMem((ET9U8*)pContext->pbAttributeSet, sizeof(pContext->pbAttributeSet));

    pContext->eRowType = ET9_xml_rowType_void;

    while (pReaderInfo->eCurrToken != ET9_xml_token_TagEnd &&
           pReaderInfo->eCurrToken != ET9_xml_token_TagEndFinal) {

        eStatus = __XmlReader_ReadAttribute(pReaderInfo);

        if (eStatus) {
            return eStatus;
        }

        if (pReaderInfo->eCurrAttribute == ET9_xml_token_A_unknown) {
            continue;
        }

        if (pContext->pbAttributeSet[pReaderInfo->eCurrAttribute]) {
            return ET9STATUS_KDB_DUPLICATE_ATTRIBUTE;
        }

        pContext->pbAttributeSet[pReaderInfo->eCurrAttribute] = 1;

        switch (pReaderInfo->eCurrAttribute)
        {
            case ET9_xml_token_A_voidSize:
                pContext->fVoidSize = pReaderInfo->fCurrFloat;
                pContext->eVoidSizeSuffix = pReaderInfo->eCurrValueSuffix;
                break;
            case ET9_xml_token_A_conditionValue:
                pContext->nConditionValue = pReaderInfo->nCurrInt;
                break;
            default:
                return ET9STATUS_KDB_UNEXPECTED_ATTRIBUTE;
        }
    }

    if (pContext->nConditionValue == pReaderInfo->nConditionValue) {

        ET9KDBPrivate * const pPrivate = &pReaderInfo->pKDBInfo->Private;
        ET9KdbXmlKeyboardInfo * const pKeyboardInfo = &pPrivate->wm.xmlReader.sKeyboardInfo;
        ET9KdbXmlKeyboard * const pKeyboard = &pKeyboardInfo->sKeyboard;
        ET9KdbXmlRow * const pRow = &pKeyboard->pRows[pKeyboard->nRowCount];

        if (pKeyboardInfo->nUsedRows >= ET9_KDB_MAX_ROWS) {
            return ET9STATUS_KDB_HAS_TOO_MANY_ROWS;
        }

        ++pKeyboard->nRowCount;
        ++pKeyboardInfo->nUsedRows;

        pRow->eRowType = pContext->eRowType;

        pRow->nKeyCount = 0;
        pRow->pKeys = NULL;

        pRow->fKeyHeight            = 0;
        pRow->eKeyHeightSuffix      = ET9_xml_valueSuffix_undef;

        pRow->fVerticalGap          = 0;
        pRow->eVerticalGapSuffix    = ET9_xml_valueSuffix_undef;

        pRow->fVoidSize             = pContext->fVoidSize;
        pRow->eVoidSizeSuffix       = pContext->eVoidSizeSuffix;
    }

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pReaderInfo           Reader context.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

static ET9STATUS ET9LOCALCALL __XmlReader_ReadRowVoid(ET9KdbXmlReaderInfo  * const pReaderInfo)
{
    ET9STATUS eStatus;

    WLOG5(fprintf(pLogFile5, "__XmlReader_ReadRowVoid\n");)

    if (pReaderInfo->eCurrToken == ET9_xml_token_Error) {
        return ET9STATUS_KDB_SYNTAX_ERROR;
    }

    if (!__XmlReader_ConsumeToken(pReaderInfo, ET9_xml_token_TagStartVoid)) {
        return ET9STATUS_KDB_SYNTAX_ERROR;
    }

    eStatus = __XmlReader_ReadRowVoidAttributes(pReaderInfo);

    if (eStatus) {
        return eStatus;
    }

    if (!__XmlReader_ConsumeToken(pReaderInfo, ET9_xml_token_TagEndFinal)) {
        return ET9STATUS_KDB_SYNTAX_ERROR;
    }

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pReaderInfo           Reader context.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

static ET9STATUS ET9LOCALCALL __XmlReader_ReadKeyboardAttributes(ET9KdbXmlReaderInfo  * const pReaderInfo)
{
    ET9STATUS eStatus;
    ET9KDBPrivate * const pPrivate = &pReaderInfo->pKDBInfo->Private;
    ET9KdbXmlReaderContext * const pContext = &pReaderInfo->pContexts[ET9_xml_E_keyboard];

    WLOG5(fprintf(pLogFile5, "__XmlReader_ReadKeyboardAttributes\n");)

    if (pReaderInfo->eCurrToken == ET9_xml_token_Error) {
        return ET9STATUS_KDB_SYNTAX_ERROR;
    }

    {
        ET9KdbXmlKeyboardInfo * const pKeyboardInfo = &pPrivate->wm.xmlReader.sKeyboardInfo;

        pKeyboardInfo->nUsedKeys = 0;
        pKeyboardInfo->nUsedRows = 0;
        pKeyboardInfo->nUsedSymbs = 0;
    }

    _ET9ClearMem((ET9U8*)pContext, sizeof(ET9KdbXmlReaderContext));

    pContext->nConditionValue           = pReaderInfo->nConditionValue;

    pContext->psIconChars               = &pPrivate->wm.xmlReader.psIconChars[0];
    pContext->psLabelChars              = &pPrivate->wm.xmlReader.psLabelChars[0];
    pContext->psLabelShiftedChars       = &pPrivate->wm.xmlReader.psLabelShiftedChars[0];
    pContext->psKeyChars                = &pPrivate->wm.xmlReader.psKeyChars[0];
    pContext->psKeyShiftedChars         = &pPrivate->wm.xmlReader.psKeyShiftedChars[0];
    pContext->psKeyPopupChars           = &pPrivate->wm.xmlReader.psKeyPopupChars[0];
    pContext->psKeyPopupShiftedChars    = &pPrivate->wm.xmlReader.psKeyPopupShiftedChars[0];
    pContext->psKeyMultitapChars        = &pPrivate->wm.xmlReader.psKeyMultitapChars[0];
    pContext->psKeyMultitapShiftedChars = &pPrivate->wm.xmlReader.psKeyMultitapShiftedChars[0];

    while (pReaderInfo->eCurrToken != ET9_xml_token_TagEnd &&
           pReaderInfo->eCurrToken != ET9_xml_token_TagEndFinal) {

        eStatus = __XmlReader_ReadAttribute(pReaderInfo);

        if (eStatus) {
            return eStatus;
        }

        if (pReaderInfo->eCurrAttribute == ET9_xml_token_A_unknown) {
            continue;
        }

        if (pContext->pbAttributeSet[pReaderInfo->eCurrAttribute]) {
            return ET9STATUS_KDB_DUPLICATE_ATTRIBUTE;
        }

        pContext->pbAttributeSet[pReaderInfo->eCurrAttribute] = 1;

        switch (pReaderInfo->eCurrAttribute)
        {
            case ET9_xml_token_A_keyWidth:
                pContext->fKeyWidth = pReaderInfo->fCurrFloat;
                pContext->eKeyWidthSuffix = pReaderInfo->eCurrValueSuffix;
                break;
            case ET9_xml_token_A_keyHeight:
                pContext->fKeyHeight = pReaderInfo->fCurrFloat;
                pContext->eKeyHeightSuffix = pReaderInfo->eCurrValueSuffix;
                break;
            case ET9_xml_token_A_verticalBorder:
                pContext->fVerticalBorder = pReaderInfo->fCurrFloat;
                pContext->eVerticalBorderSuffix = pReaderInfo->eCurrValueSuffix;
                break;
            case ET9_xml_token_A_horizontalBorder:
                pContext->fHorizontalBorder = pReaderInfo->fCurrFloat;
                pContext->eHorizontalBorderSuffix = pReaderInfo->eCurrValueSuffix;
                break;
            case ET9_xml_token_A_verticalGap:
                pContext->fVerticalGap = pReaderInfo->fCurrFloat;
                pContext->eVerticalGapSuffix = pReaderInfo->eCurrValueSuffix;
                break;
            case ET9_xml_token_A_horizontalGap:
                pContext->fHorizontalGap = pReaderInfo->fCurrFloat;
                pContext->eHorizontalGapSuffix = pReaderInfo->eCurrValueSuffix;
                break;
            case ET9_xml_token_A_majorVersion:
                pContext->bMajorVersion = (ET9U8)pReaderInfo->nCurrInt;
                break;
            case ET9_xml_token_A_minorVersion:
                pContext->bMinorVersion = (ET9U8)pReaderInfo->nCurrInt;
                break;
            case ET9_xml_token_A_primaryId:
                pContext->bPrimaryID = (ET9U8)pReaderInfo->nCurrInt;
                break;
            case ET9_xml_token_A_secondaryId:
                pContext->bSecondaryID = (ET9U8)pReaderInfo->nCurrInt;
                break;
            case ET9_xml_token_A_supportsExact:
                pContext->eSupportsExact = __XmlReader_ConstantToBooleanValue(pReaderInfo->eCurrConstant);
                break;
            case ET9_xml_token_A_defaultLayoutWidth:
                pContext->wDefaultLayoutWidth = (ET9U16)pReaderInfo->nCurrInt;
                break;
            case ET9_xml_token_A_defaultLayoutHeight:
                pContext->wDefaultLayoutHeight = (ET9U16)pReaderInfo->nCurrInt;
                break;
            case ET9_xml_token_A_addAltChars:
                pContext->eAddAltChars = __XmlReader_ConstantToBooleanValue(pReaderInfo->eCurrConstant);
                break;

            default:
                return ET9STATUS_KDB_UNEXPECTED_ATTRIBUTE;
        }
    }

    if (!pContext->pbAttributeSet[ET9_xml_token_A_primaryId] ||
        !pContext->pbAttributeSet[ET9_xml_token_A_secondaryId]) {

        return ET9STATUS_KDB_MISSING_ATTRIBUTE;
    }

    if ( pContext->pbAttributeSet[ET9_xml_token_A_defaultLayoutWidth] &&
        !pContext->pbAttributeSet[ET9_xml_token_A_defaultLayoutHeight] ||
        !pContext->pbAttributeSet[ET9_xml_token_A_defaultLayoutWidth] &&
         pContext->pbAttributeSet[ET9_xml_token_A_defaultLayoutHeight]) {

        return ET9STATUS_KDB_MISSING_ATTRIBUTE;
    }

    {
        ET9KdbXmlKeyboardInfo * const pKeyboardInfo = &pPrivate->wm.xmlReader.sKeyboardInfo;
        ET9KdbXmlKeyboard * const pKeyboard = &pKeyboardInfo->sKeyboard;

        pKeyboard->bPrimaryID               = pContext->bPrimaryID;
        pKeyboard->bSecondaryID             = pContext->bSecondaryID;
        pKeyboard->bMajorVersion            = pContext->bMajorVersion;
        pKeyboard->bMinorVersion            = pContext->bMinorVersion;
        pKeyboard->fVerticalBorder          = pContext->fVerticalBorder;
        pKeyboard->eVerticalBorderSuffix    = pContext->eVerticalBorderSuffix;
        pKeyboard->fHorizontalBorder        = pContext->fHorizontalBorder;
        pKeyboard->eHorizontalBorderSuffix  = pContext->eHorizontalBorderSuffix;
        pKeyboard->eSupportsExact           = pContext->eSupportsExact;

        if (pReaderInfo->wLayoutWidth || pReaderInfo->wLayoutHeight) {

            pKeyboard->wLayoutWidth         = pReaderInfo->wLayoutWidth;
            pKeyboard->wLayoutHeight        = pReaderInfo->wLayoutHeight;
        }
        else {
            pKeyboard->wLayoutWidth         = pContext->wDefaultLayoutWidth;
            pKeyboard->wLayoutHeight        = pContext->wDefaultLayoutHeight;
        }

        pKeyboard->nRowCount = 0;
        pKeyboard->pRows = &pKeyboardInfo->pRowPool[0];
    }

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pReaderInfo           Reader context.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

static ET9STATUS ET9LOCALCALL __XmlReader_ReadKeyboard(ET9KdbXmlReaderInfo  * const pReaderInfo)
{
    ET9STATUS eStatus;

    WLOG5(fprintf(pLogFile5, "__XmlReader_ReadKeyboard\n");)

    if (pReaderInfo->eCurrToken == ET9_xml_token_Error) {
        return ET9STATUS_KDB_SYNTAX_ERROR;
    }

    if (!__XmlReader_ConsumeToken(pReaderInfo, ET9_xml_token_TagStartKeyboard)) {
        return ET9STATUS_KDB_SYNTAX_ERROR;
    }

    eStatus = __XmlReader_ReadKeyboardAttributes(pReaderInfo);

    if (eStatus) {
        return eStatus;
    }

    if (!__XmlReader_ConsumeToken(pReaderInfo, ET9_xml_token_TagEnd)) {
        return ET9STATUS_KDB_SYNTAX_ERROR;
    }

    while (pReaderInfo->eCurrToken == ET9_xml_token_TagStartRow ||
           pReaderInfo->eCurrToken == ET9_xml_token_TagStartArea ||
           pReaderInfo->eCurrToken == ET9_xml_token_TagStartVoid) {

        switch (pReaderInfo->eCurrToken)
        {
            case ET9_xml_token_TagStartRow:
               eStatus = __XmlReader_ReadRow(pReaderInfo);
               break;
            case ET9_xml_token_TagStartArea:
               eStatus = __XmlReader_ReadArea(pReaderInfo);
               break;
            case ET9_xml_token_TagStartVoid:
               eStatus = __XmlReader_ReadRowVoid(pReaderInfo);
               break;
            default:
               eStatus = ET9STATUS_KDB_SYNTAX_ERROR;
               break;
        }

        if (eStatus) {
            return eStatus;
        }
    }

    if (!(__XmlReader_ConsumeToken(pReaderInfo, ET9_xml_token_TagEndKeyboard) &&
          __XmlReader_ConsumeToken(pReaderInfo, ET9_xml_token_TagEnd))) {
        return ET9STATUS_KDB_SYNTAX_ERROR;
    }

    if (pReaderInfo->eCurrToken != ET9_xml_token_EOF) {
        return ET9STATUS_KDB_UNEXPECTED_CONTENT;
    }

    __XmlReader_PruneAltChars(pReaderInfo, &pReaderInfo->pKDBInfo->Private.wm.xmlReader.sKeyboardInfo);

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pKeyboardInfo         .
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

static ET9STATUS ET9LOCALCALL __XmlReader_BasicGaps(ET9KdbXmlKeyboardInfo * const pKeyboardInfo)
{
    const ET9UINT nGapDefault = 2;

    ET9UINT nCount;
    ET9KdbXmlRow *pRow;
    ET9KdbXmlRow *pPrevRow;
    ET9KdbXmlKey *pKey;
    ET9KdbXmlKey *pPrevKey;

    /* applicable? */

    if (pKeyboardInfo->bAreaBased) {
        return ET9STATUS_NONE;
    }

    /* vertical gaps */

    pPrevRow = NULL;
    pRow = pKeyboardInfo->sKeyboard.pRows;
    for (nCount = pKeyboardInfo->sKeyboard.nRowCount; nCount; --nCount, pPrevRow = pRow++) {

        if (pRow->eRowType == ET9_xml_rowType_void) {

            pRow->nUpGap = 0;
            pRow->nDownGap = 0;

            if (pPrevRow) {
                pPrevRow->nDownGap = 0;
            }

            continue;
        }

        switch (pRow->eVerticalGapSuffix)
        {
            case ET9_xml_valueSuffix_undef:
                pRow->nUpGap = nGapDefault;
                break;
            case ET9_xml_valueSuffix_percent:
                pRow->nUpGap = (ET9UINT)(pRow->fVerticalGap / 100.0 * pKeyboardInfo->sKeyboard.wLayoutHeight + 0.5);
                break;
            case ET9_xml_valueSuffix_pixelsDI:
                pRow->nUpGap = (ET9UINT)pRow->fVerticalGap;
                break;
        }

        pRow->nDownGap = pRow->nUpGap;

        if (pPrevRow && pPrevRow->eRowType == ET9_xml_rowType_void) {
            pRow->nUpGap = 0;
        }
    }

    /* horizontal gaps */

    pPrevKey = NULL;
    pKey = &pKeyboardInfo->pKeyPool[0];
    for (nCount = pKeyboardInfo->nUsedKeys; nCount; --nCount, pPrevKey = pKey++) {

        if (pKey->eKeyType == ET9_xml_keyType_void) {

            pKey->nLeftGap = 0;
            pKey->nRightGap = 0;

            if (pPrevKey) {
                pPrevKey->nRightGap = 0;
            }

            continue;
        }

        switch (pKey->eHorizontalGapSuffix)
        {
            case ET9_xml_valueSuffix_undef:
                pKey->nLeftGap = nGapDefault;
                break;
            case ET9_xml_valueSuffix_percent:
                pKey->nLeftGap = (ET9UINT)(pKey->fHorizontalGap / 100.0 * pKeyboardInfo->sKeyboard.wLayoutWidth + 0.5);
                break;
            case ET9_xml_valueSuffix_pixelsDI:
                pKey->nLeftGap = (ET9UINT)pKey->fHorizontalGap;
                break;
        }

        pKey->nRightGap = pKey->nLeftGap;

        if (pPrevKey && pPrevKey->eKeyType == ET9_xml_keyType_void) {
            pKey->nLeftGap = 0;
        }
    }

    /* border */

    switch (pKeyboardInfo->sKeyboard.eVerticalBorderSuffix)
    {
        case ET9_xml_valueSuffix_undef:
            pKeyboardInfo->sKeyboard.nVerticalBorder = nGapDefault;
            break;
        case ET9_xml_valueSuffix_percent:
            pKeyboardInfo->sKeyboard.nVerticalBorder = (ET9UINT)(pKeyboardInfo->sKeyboard.fVerticalBorder / 100.0 * pKeyboardInfo->sKeyboard.wLayoutHeight + 0.5);
            break;
        case ET9_xml_valueSuffix_pixelsDI:
            pKeyboardInfo->sKeyboard.nVerticalBorder = (ET9UINT)pKeyboardInfo->sKeyboard.fVerticalBorder;
            break;
    }

    switch (pKeyboardInfo->sKeyboard.eHorizontalBorderSuffix)
    {
        case ET9_xml_valueSuffix_undef:
            pKeyboardInfo->sKeyboard.nHorizontalBorder = nGapDefault;
            break;
        case ET9_xml_valueSuffix_percent:
            pKeyboardInfo->sKeyboard.nHorizontalBorder = (ET9UINT)(pKeyboardInfo->sKeyboard.fHorizontalBorder / 100.0 * pKeyboardInfo->sKeyboard.wLayoutWidth + 0.5);
            break;
        case ET9_xml_valueSuffix_pixelsDI:
            pKeyboardInfo->sKeyboard.nHorizontalBorder = (ET9UINT)pKeyboardInfo->sKeyboard.fHorizontalBorder;
            break;
    }

    /* apply border */

    pRow = &pKeyboardInfo->sKeyboard.pRows[0];

    if (pRow->eRowType != ET9_xml_rowType_void) {
        pRow->nUpGap = pKeyboardInfo->sKeyboard.nVerticalBorder;
    }

    pRow = &pKeyboardInfo->sKeyboard.pRows[pKeyboardInfo->sKeyboard.nRowCount - 1];

    if (pRow->eRowType != ET9_xml_rowType_void) {
        pRow->nDownGap = pKeyboardInfo->sKeyboard.nVerticalBorder;
    }

    pRow = pKeyboardInfo->sKeyboard.pRows;
    for (nCount = pKeyboardInfo->sKeyboard.nRowCount; nCount; --nCount, ++pRow) {

        if (pRow->eRowType == ET9_xml_rowType_keys) {

            pKey = &pRow->pKeys[0];

            if (pKey->eKeyType != ET9_xml_keyType_void) {
                pKey->nLeftGap = pKeyboardInfo->sKeyboard.nHorizontalBorder;
            }

            pKey = &pRow->pKeys[pRow->nKeyCount - 1];

            if (pKey->eKeyType != ET9_xml_keyType_void) {
                pKey->nRightGap = pKeyboardInfo->sKeyboard.nHorizontalBorder;
            }
        }
    }

    /* done */

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pKeyboardInfo         .
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

static ET9STATUS ET9LOCALCALL __XmlReader_MergeGaps(ET9KdbXmlKeyboardInfo * const pKeyboardInfo)
{
    ET9UINT nRowCount;
    ET9KdbXmlRow *pRow;
    ET9KdbXmlRow *pPrevRow;

    /* applicable? */

    if (pKeyboardInfo->bAreaBased) {
        return ET9STATUS_NONE;
    }

    /* */

    pPrevRow = NULL;
    pRow = pKeyboardInfo->sKeyboard.pRows;
    for (nRowCount = pKeyboardInfo->sKeyboard.nRowCount; nRowCount; --nRowCount, pPrevRow = pRow++) {

        ET9UINT nKeyCount;
        ET9KdbXmlKey *pKey;
        ET9KdbXmlKey *pPrevKey;

        pPrevKey = NULL;
        pKey = pRow->pKeys;
        for (nKeyCount = pRow->nKeyCount; nKeyCount; --nKeyCount, pPrevKey = pKey++) {

            if (pPrevKey) {

                const ET9UINT nGap = __ET9Max(pPrevKey->nRightGap, pKey->nLeftGap);

                pPrevKey->nRightGap = nGap / 2;
                pKey->nLeftGap = nGap - pPrevKey->nRightGap;
            }
        }

        if (pPrevRow) {

            const ET9UINT nGap = __ET9Max(pPrevRow->nDownGap, pRow->nUpGap);

            pPrevRow->nDownGap = nGap / 2;
            pRow->nUpGap = nGap - pPrevRow->nDownGap;
        }
    }

    /* done */

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pKeyboardInfo         .
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

static ET9STATUS ET9LOCALCALL __XmlReader_BasicSize(ET9KdbXmlKeyboardInfo * const pKeyboardInfo)
{
    const ET9UINT nSizeDefault = 10;

    ET9UINT nRowCount;
    ET9KdbXmlRow *pRow;

    /* */

    pRow = pKeyboardInfo->sKeyboard.pRows;
    for (nRowCount = pKeyboardInfo->sKeyboard.nRowCount; nRowCount; --nRowCount, ++pRow) {

        ET9UINT nKeyCount;
        ET9KdbXmlKey *pKey;

        if (pRow->eRowType == ET9_xml_rowType_void) {
            switch (pRow->eVoidSizeSuffix)
            {
                case ET9_xml_valueSuffix_undef:
                    pRow->nHeight = 0;
                    break;
                case ET9_xml_valueSuffix_percent:
                    pRow->nHeight = (ET9UINT)(pRow->fVoidSize / 100.0 * pKeyboardInfo->sKeyboard.wLayoutHeight + fSizeRound);
                    break;
                case ET9_xml_valueSuffix_pixelsDI:
                    pRow->nHeight = (ET9UINT)pRow->fVoidSize;
                    break;
            }
            continue;
        }

        switch (pRow->eKeyHeightSuffix)
        {
            case ET9_xml_valueSuffix_undef:
                pRow->nHeight = nSizeDefault;
                break;
            case ET9_xml_valueSuffix_percent:
                pRow->nHeight = (ET9UINT)(pRow->fKeyHeight / 100.0 * pKeyboardInfo->sKeyboard.wLayoutHeight + fSizeRound);
                break;
            case ET9_xml_valueSuffix_pixelsDI:
                pRow->nHeight = (ET9UINT)pRow->fKeyHeight;
                break;
        }

        pKey = pRow->pKeys;
        for (nKeyCount = pRow->nKeyCount; nKeyCount; --nKeyCount, ++pKey) {

            if (pKey->eKeyType == ET9_xml_keyType_void) {
                switch (pKey->eVoidSizeSuffix)
                {
                    case ET9_xml_valueSuffix_undef:
                        pKey->nWidth = 0;
                        break;
                    case ET9_xml_valueSuffix_percent:
                        pKey->nWidth = (ET9UINT)(pKey->fVoidSize / 100.0 * pKeyboardInfo->sKeyboard.wLayoutWidth + fSizeRound);
                        break;
                    case ET9_xml_valueSuffix_pixelsDI:
                        pKey->nWidth = (ET9UINT)pKey->fVoidSize;
                        break;
                }
                continue;
            }

            switch (pKey->eKeyWidthSuffix)
            {
                case ET9_xml_valueSuffix_undef:
                    pKey->nWidth = nSizeDefault;
                    break;
                case ET9_xml_valueSuffix_percent:
                    pKey->nWidth = (ET9UINT)(pKey->fKeyWidth / 100.0 * pKeyboardInfo->sKeyboard.wLayoutWidth + fSizeRound);
                    break;
                case ET9_xml_valueSuffix_pixelsDI:
                    pKey->nWidth = (ET9UINT)pKey->fKeyWidth;
                    break;
            }

            if (pRow->eRowType != ET9_xml_rowType_area) {
                pKey->nTop = 0;
                pKey->nLeft = 0;
                pKey->nHeight = pRow->nHeight;
                continue;
            }

            switch (pKey->eKeyTopSuffix)
            {
                case ET9_xml_valueSuffix_undef:
                    pKey->nTop = nSizeDefault;
                    break;
                case ET9_xml_valueSuffix_percent:
                    pKey->nTop = (ET9UINT)(pKey->fKeyTop / 100.0 * pKeyboardInfo->sKeyboard.wLayoutHeight + fPosRound);
                    break;
                case ET9_xml_valueSuffix_pixelsDI:
                    pKey->nTop = (ET9UINT)pKey->fKeyTop;
                    break;
            }

            switch (pKey->eKeyLeftSuffix)
            {
                case ET9_xml_valueSuffix_undef:
                    pKey->nLeft = nSizeDefault;
                    break;
                case ET9_xml_valueSuffix_percent:
                    pKey->nLeft = (ET9UINT)(pKey->fKeyLeft / 100.0 * pKeyboardInfo->sKeyboard.wLayoutWidth + fPosRound);
                    break;
                case ET9_xml_valueSuffix_pixelsDI:
                    pKey->nLeft = (ET9UINT)pKey->fKeyLeft;
                    break;
            }

            switch (pKey->eKeyHeightSuffix)
            {
                case ET9_xml_valueSuffix_undef:
                    pKey->nHeight = nSizeDefault;
                    break;
                case ET9_xml_valueSuffix_percent:
                    pKey->nHeight = (ET9UINT)(pKey->fKeyHeight / 100.0 * pKeyboardInfo->sKeyboard.wLayoutHeight + fSizeRound);
                    break;
                case ET9_xml_valueSuffix_pixelsDI:
                    pKey->nHeight = (ET9UINT)pKey->fKeyHeight;
                    break;
            }
        }
    }

    /* done */

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pRow                  .
 *
 * @return Void count.
 */

static ET9UINT ET9LOCALCALL __XmlReader_CalculateOpenKeyVoidCount(ET9KdbXmlRow * const pRow)
{
    ET9UINT nVoidCount;
    ET9UINT nKeyCount;
    ET9KdbXmlKey *pKey;

    nVoidCount = 0;
    pKey = pRow->pKeys;
    for (nKeyCount = pRow->nKeyCount; nKeyCount; --nKeyCount, ++pKey) {
        if (pKey->eKeyType == ET9_xml_keyType_void && !pKey->eVoidSizeSuffix) {
            ++nVoidCount;
        }
    }

    return nVoidCount;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pRow                  .
 *
 * @return Row width.
 */

static ET9UINT ET9LOCALCALL __XmlReader_CalculateRowWidth(ET9KdbXmlRow * const pRow)
{
    ET9UINT nTotWidth;
    ET9UINT nKeyCount;
    ET9KdbXmlKey *pKey;

    nTotWidth = 0;
    pKey = pRow->pKeys;
    for (nKeyCount = pRow->nKeyCount; nKeyCount; --nKeyCount, ++pKey) {
        nTotWidth += pKey->nLeftGap + pKey->nWidth + pKey->nRightGap;
    }

    return nTotWidth;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pKeyboardInfo         .
 *
 * @return Height of all rows.
 */

static ET9UINT ET9LOCALCALL __XmlReader_CalculateRowsHeight(ET9KdbXmlKeyboardInfo * const pKeyboardInfo)
{
    ET9UINT nTotHeight;
    ET9UINT nRowCount;
    ET9KdbXmlRow *pRow;

    nTotHeight = 0;
    pRow = pKeyboardInfo->sKeyboard.pRows;
    for (nRowCount = pKeyboardInfo->sKeyboard.nRowCount; nRowCount; --nRowCount, ++pRow) {
        nTotHeight += pRow->nUpGap + pRow->nHeight + pRow->nDownGap;
    }

    return nTotHeight;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pKeyboardInfo         .
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

static ET9STATUS ET9LOCALCALL __XmlReader_ExpandKeyVoids(ET9KdbXmlKeyboardInfo * const pKeyboardInfo)
{
    ET9UINT nRowCount;
    ET9KdbXmlRow *pRow;

    /* applicable? */

    if (pKeyboardInfo->bAreaBased) {
        return ET9STATUS_NONE;
    }

    /* */

    pRow = pKeyboardInfo->sKeyboard.pRows;
    for (nRowCount = pKeyboardInfo->sKeyboard.nRowCount; nRowCount; --nRowCount, ++pRow) {

        const ET9UINT nVoidCount = __XmlReader_CalculateOpenKeyVoidCount(pRow);

        if (nVoidCount) {

            const ET9UINT nRowWidth = __XmlReader_CalculateRowWidth(pRow);

            if (nRowWidth < pKeyboardInfo->sKeyboard.wLayoutWidth) {

                const ET9UINT nDiff = pKeyboardInfo->sKeyboard.wLayoutWidth - nRowWidth;

                ET9UINT nKeyCount;
                ET9KdbXmlKey *pKey;

                ET9UINT nExpandSpace;
                ET9UINT nExpandCount;

                nExpandSpace = nDiff;
                nExpandCount = nVoidCount;

                pKey = pRow->pKeys;
                for (nKeyCount = pRow->nKeyCount; nKeyCount; --nKeyCount, ++pKey) {

                    if (pKey->eKeyType == ET9_xml_keyType_void && !pKey->eVoidSizeSuffix) {

                        const ET9UINT nExpand = nExpandSpace / nExpandCount;

                        pKey->nWidth += nExpand;
                        nExpandSpace -= nExpand;
                        --nExpandCount;

                        if (!nExpandCount) {
                            break;
                        }
                    }
                }
            }
        }
    }

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pKeyboardInfo         .
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

static ET9STATUS ET9LOCALCALL __XmlReader_ExpandKeys(ET9KdbXmlKeyboardInfo * const pKeyboardInfo)
{
    ET9UINT nBestDiff;
    ET9UINT nRowCount;
    ET9KdbXmlRow *pRow;

    /* applicable? */

    if (pKeyboardInfo->bAreaBased) {
        return ET9STATUS_NONE;
    }

    /* */

    nBestDiff = 0;
    pRow = pKeyboardInfo->sKeyboard.pRows;
    for (nRowCount = pKeyboardInfo->sKeyboard.nRowCount; nRowCount; --nRowCount, ++pRow) {

        if (pRow->eRowType == ET9_xml_rowType_keys) {

            const ET9UINT nVoidCount = __XmlReader_CalculateOpenKeyVoidCount(pRow);

            if (!nVoidCount) {

                const ET9UINT nRowWidth = __XmlReader_CalculateRowWidth(pRow);

                if (nRowWidth < pKeyboardInfo->sKeyboard.wLayoutWidth) {

                    const ET9UINT nDiff = pKeyboardInfo->sKeyboard.wLayoutWidth - nRowWidth;

                    if (nBestDiff < nDiff) {
                        nBestDiff = nDiff;
                    }
                }
            }
        }
    }

    if (!nBestDiff) {
        return ET9STATUS_NONE;
    }

    pRow = pKeyboardInfo->sKeyboard.pRows;
    for (nRowCount = pKeyboardInfo->sKeyboard.nRowCount; nRowCount; --nRowCount, ++pRow) {

        if (pRow->eRowType == ET9_xml_rowType_keys) {

            const ET9UINT nRowWidth = __XmlReader_CalculateRowWidth(pRow);

            ET9UINT nDiff;
            ET9UINT nRepeatCount;
            ET9UINT nKeyCount;
            ET9KdbXmlKey *pKey;

            if (nRowWidth >= pKeyboardInfo->sKeyboard.wLayoutWidth) {
                continue;
            }

            nDiff = __ET9Min(nBestDiff, (pKeyboardInfo->sKeyboard.wLayoutWidth - nRowWidth));

            for (nRepeatCount = 2; nRepeatCount && nDiff; --nRepeatCount) {

                pKey = pRow->pKeys;
                for (nKeyCount = pRow->nKeyCount; nKeyCount && nDiff; --nKeyCount, ++pKey) {

                    if (pKey->eKeyType == ET9_xml_keyType_void) {
                        continue;
                    }

                    if (pKey->eKeyWidthSuffix != ET9_xml_valueSuffix_pixelsDI) {

                        ET9UINT nPart;

                        nPart = (pKey->nWidth * nBestDiff) / pKeyboardInfo->sKeyboard.wLayoutWidth;

                        if (!nPart) {
                            nPart = 1;
                        }

                        if (nPart > nDiff) {
                            nPart = nDiff;
                        }

                        nDiff -= nPart;
                        pKey->nWidth += nPart;
                    }
                }
            }
        }
    }

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pKeyboardInfo         .
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

static ET9STATUS ET9LOCALCALL __XmlReader_ExpandRowVoids(ET9KdbXmlKeyboardInfo * const pKeyboardInfo)
{
    ET9UINT nVoidCount;
    ET9UINT nRowCount;
    ET9KdbXmlRow *pRow;

    /* applicable? */

    if (pKeyboardInfo->bAreaBased) {
        return ET9STATUS_NONE;
    }

    /* */

    nVoidCount = 0;
    pRow = pKeyboardInfo->sKeyboard.pRows;
    for (nRowCount = pKeyboardInfo->sKeyboard.nRowCount; nRowCount; --nRowCount, ++pRow) {
        if (pRow->eRowType == ET9_xml_rowType_void && !pRow->eVoidSizeSuffix) {
            ++nVoidCount;
        }
    }

    if (!nVoidCount) {
        return ET9STATUS_NONE;
    }

    {
        const ET9UINT nRowsHeight = __XmlReader_CalculateRowsHeight(pKeyboardInfo);

        if (pKeyboardInfo->sKeyboard.wLayoutHeight > nRowsHeight) {

            const ET9UINT nDiff = pKeyboardInfo->sKeyboard.wLayoutHeight - nRowsHeight;

            ET9UINT nExpandSpace;
            ET9UINT nExpandCount;

            nExpandSpace = nDiff;
            nExpandCount = nVoidCount;

            pRow = pKeyboardInfo->sKeyboard.pRows;
            for (nRowCount = pKeyboardInfo->sKeyboard.nRowCount; nRowCount; --nRowCount, ++pRow) {

                if (pRow->eRowType == ET9_xml_rowType_void && !pRow->eVoidSizeSuffix) {

                    const ET9UINT nExpand = nExpandSpace / nExpandCount;

                    pRow->nHeight += nExpand;
                    nExpandSpace -= nExpand;
                    --nExpandCount;

                    if (!nExpandCount) {
                        break;
                    }
                }
            }
        }
    }

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pKeyboardInfo         .
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

static ET9STATUS ET9LOCALCALL __XmlReader_ExpandRows(ET9KdbXmlKeyboardInfo * const pKeyboardInfo)
{
    const ET9UINT nRowsHeight = __XmlReader_CalculateRowsHeight(pKeyboardInfo);

    /* applicable? */

    if (pKeyboardInfo->bAreaBased) {
        return ET9STATUS_NONE;
    }

    /* */

    if (nRowsHeight >= pKeyboardInfo->sKeyboard.wLayoutHeight) {
        return ET9STATUS_NONE;
    }

    {
        ET9UINT nDiff = pKeyboardInfo->sKeyboard.wLayoutHeight - nRowsHeight;

        ET9UINT nRowCount;
        ET9UINT nRepeatCount;
        ET9KdbXmlRow *pRow;
        ET9KdbXmlRow *pPrevRow;

        pRow = pKeyboardInfo->sKeyboard.pRows;
        for (nRowCount = pKeyboardInfo->sKeyboard.nRowCount; nRowCount && nDiff; --nRowCount, ++pRow) {

            if (pRow->eKeyHeightSuffix != ET9_xml_valueSuffix_pixelsDI && pRow->eRowType != ET9_xml_rowType_void) {
                --nDiff;
                ++pRow->nHeight;
            }
        }

        pPrevRow = NULL;
        pRow = pKeyboardInfo->sKeyboard.pRows;
        for (nRowCount = pKeyboardInfo->sKeyboard.nRowCount; nRowCount && nDiff; --nRowCount, pPrevRow = pRow++) {

            if (!pPrevRow) {
                continue;
            }

            if (pRow->eRowType == ET9_xml_rowType_void || pPrevRow->eRowType == ET9_xml_rowType_void) {
                continue;
            }

            if (pPrevRow->eVerticalGapSuffix != ET9_xml_valueSuffix_pixelsDI) {
                --nDiff;
                ++pPrevRow->nDownGap;
            }
            else if (pRow->eVerticalGapSuffix != ET9_xml_valueSuffix_pixelsDI) {
                --nDiff;
                ++pRow->nUpGap;
            }
        }

        for (nRepeatCount = 2; nRepeatCount; --nRepeatCount) {

            pRow = pKeyboardInfo->sKeyboard.pRows;
            for (nRowCount = pKeyboardInfo->sKeyboard.nRowCount; nRowCount && nDiff; --nRowCount, ++pRow) {

                if (pRow->eKeyHeightSuffix != ET9_xml_valueSuffix_pixelsDI && pRow->eRowType != ET9_xml_rowType_void) {
                    --nDiff;
                    ++pRow->nHeight;
                }
            }
        }
    }

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pKeyboardInfo         .
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

static ET9STATUS ET9LOCALCALL __XmlReader_PositionKeys(ET9KdbXmlKeyboardInfo * const pKeyboardInfo)
{
    ET9UINT nCurrY;
    ET9UINT nRowCount;
    ET9KdbXmlRow *pRow;

    /* loop rows */

    nCurrY = 0;
    pRow = pKeyboardInfo->sKeyboard.pRows;
    for (nRowCount = pKeyboardInfo->sKeyboard.nRowCount; nRowCount; --nRowCount, ++pRow) {

        if (pRow->eRowType == ET9_xml_rowType_area) {

            ET9UINT nKeyCount;
            ET9KdbXmlKey *pKey;

            pKey = pRow->pKeys;
            for (nKeyCount = pRow->nKeyCount; nKeyCount; --nKeyCount, ++pKey) {

                pKey->nULX = pKey->nLeft;
                pKey->nULY = pKey->nTop;
            }
        }
        else {

            const ET9UINT nTotRowHeight = pRow->nUpGap + pRow->nHeight + pRow->nDownGap;

            ET9UINT nCurrX;
            ET9UINT nKeyCount;
            ET9KdbXmlKey *pKey;

            nCurrX = 0;
            pKey = pRow->pKeys;
            for (nKeyCount = pRow->nKeyCount; nKeyCount; --nKeyCount, ++pKey) {

                const ET9UINT nTotKeyWidth = pKey->nLeftGap + pKey->nWidth + pKey->nRightGap;

                pKey->nULX = nCurrX + pKey->nLeftGap;
                pKey->nULY = nCurrY + pRow->nUpGap;

                nCurrX += nTotKeyWidth;
            }

            nCurrY += nTotRowHeight;
        }
    }

    /* done */

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pKeyboardInfo         .
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

static ET9STATUS ET9LOCALCALL __XmlReader_RemoveVoidRowItems(ET9KdbXmlKeyboardInfo * const pKeyboardInfo)
{
    ET9UINT nRowCount;
    ET9UINT nVoidCount;
    ET9KdbXmlRow *pRow;

    /* applicable? */

    if (pKeyboardInfo->bAreaBased) {
        return ET9STATUS_NONE;
    }

    /* */

    WLOG5(fprintf(pLogFile5, "__XmlReader_RemoveVoidRowItems, nRowCount %u\n", pKeyboardInfo->sKeyboard.nRowCount);)

    nVoidCount = 0;
    pRow = pKeyboardInfo->sKeyboard.pRows;
    for (nRowCount = pKeyboardInfo->sKeyboard.nRowCount; nRowCount; ) {

        if (nVoidCount) {

            WLOG5(fprintf(pLogFile5, "  move %2u <- %2u\n", (int)(pRow - pKeyboardInfo->sKeyboard.pRows), (int)(pRow - pKeyboardInfo->sKeyboard.pRows + nVoidCount));)

            *pRow = *(pRow + nVoidCount);
        }

        if (pRow->eRowType == ET9_xml_rowType_void) {

            WLOG5(fprintf(pLogFile5, "  VOID\n");)

            ++nVoidCount;
            --nRowCount;
            --pKeyboardInfo->sKeyboard.nRowCount;
        }
        else {
            ++pRow;
            --nRowCount;
        }
    }

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pKeyboardInfo         .
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

static ET9STATUS ET9LOCALCALL __XmlReader_RemoveVoidKeyItems(ET9KdbXmlKeyboardInfo * const pKeyboardInfo)
{
    ET9UINT nRowCount;
    ET9KdbXmlRow *pRow;

    /* applicable? */

    if (pKeyboardInfo->bAreaBased) {
        return ET9STATUS_NONE;
    }

    /* */

    WLOG5(fprintf(pLogFile5, "__XmlReader_RemoveVoidKeyItems\n");)

    pRow = pKeyboardInfo->sKeyboard.pRows;
    for (nRowCount = pKeyboardInfo->sKeyboard.nRowCount; nRowCount; --nRowCount, ++pRow) {

        ET9UINT nKeyCount;
        ET9UINT nVoidCount;
        ET9KdbXmlKey *pKey;

        WLOG5(fprintf(pLogFile5, "  Row %2u, nKeyCount %2u\n", (int)(pRow - pKeyboardInfo->sKeyboard.pRows), pRow->nKeyCount);)

        nVoidCount = 0;
        pKey = pRow->pKeys;
        for (nKeyCount = pRow->nKeyCount; nKeyCount; ) {

            WLOG5(fprintf(pLogFile5, "    Key %2u\n", (int)(pKey - pRow->pKeys));)

            if (nVoidCount) {

                WLOG5(fprintf(pLogFile5, "      move %2u <- %2u\n", (int)(pKey - pRow->pKeys), (int)(pKey - pRow->pKeys + nVoidCount));)

                *pKey = *(pKey + nVoidCount);
            }

            if (pKey->eKeyType == ET9_xml_keyType_void) {

                WLOG5(fprintf(pLogFile5, "      VOID\n");)

                ++nVoidCount;
                --nKeyCount;
                --pRow->nKeyCount;
            }
            else {
                ++pKey;
                --nKeyCount;
            }
        }
    }

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pKeyboardInfo         .
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

static ET9STATUS ET9LOCALCALL __XmlReader_CalculateLayout(ET9KdbXmlKeyboardInfo * const pKeyboardInfo)
{
    ET9STATUS eStatus;
    ET9UINT nCount;
    ET9KdbXmlRow *pRow;

    WLOG5(fprintf(pLogFile5, "__XmlReader_CalculateLayout\n");)

    if (!pKeyboardInfo->sKeyboard.nRowCount) {
        return ET9STATUS_KDB_HAS_TOO_FEW_ROWS;
    }

    {
        ET9UINT nCountRowKeys = 0;
        ET9UINT nCountRowArea = 0;
        ET9UINT nCountRowVoid = 0;

        pRow = pKeyboardInfo->sKeyboard.pRows;
        for (nCount = pKeyboardInfo->sKeyboard.nRowCount; nCount; --nCount, ++pRow) {

            switch (pRow->eRowType)
            {
                case ET9_xml_rowType_keys:
                    ++nCountRowKeys;
                    break;
                case ET9_xml_rowType_area:
                    ++nCountRowArea;
                    break;
                case ET9_xml_rowType_void:
                    ++nCountRowVoid;
                    break;
            }

            if (!pRow->nKeyCount && (pRow->eRowType == ET9_xml_rowType_keys || pRow->eRowType == ET9_xml_rowType_area)) {
                return ET9STATUS_KDB_ROW_HAS_TOO_FEW_KEYS;
            }
        }

        if (nCountRowArea && (nCountRowKeys || nCountRowVoid)) {
            return ET9STATUS_KDB_UNEXPECTED_CONTENT;
        }

        pKeyboardInfo->bAreaBased = nCountRowArea ? 1 : 0;
    }

    eStatus = __XmlReader_BasicGaps(pKeyboardInfo);

    if (eStatus) {
        return eStatus;
    }

    eStatus = __XmlReader_MergeGaps(pKeyboardInfo);

    if (eStatus) {
        return eStatus;
    }

    eStatus = __XmlReader_BasicSize(pKeyboardInfo);

    if (eStatus) {
        return eStatus;
    }

    eStatus = __XmlReader_ExpandKeys(pKeyboardInfo);

    if (eStatus) {
        return eStatus;
    }

    eStatus = __XmlReader_ExpandKeyVoids(pKeyboardInfo);

    if (eStatus) {
        return eStatus;
    }

    eStatus = __XmlReader_ExpandRowVoids(pKeyboardInfo);

    if (eStatus) {
        return eStatus;
    }

    eStatus = __XmlReader_ExpandRows(pKeyboardInfo);

    if (eStatus) {
        return eStatus;
    }

    eStatus = __XmlReader_PositionKeys(pKeyboardInfo);

    if (eStatus) {
        return eStatus;
    }

    eStatus = __XmlReader_RemoveVoidRowItems(pKeyboardInfo);

    if (eStatus) {
        return eStatus;
    }

    eStatus = __XmlReader_RemoveVoidKeyItems(pKeyboardInfo);

    if (eStatus) {
        return eStatus;
    }

    /* done */

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pReaderInfo           Reader context.
 * @param pnErrorLine           If an error occurs, this is an indication to where it happened.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

static ET9STATUS ET9LOCALCALL __XmlReader_TransferLayout(ET9KdbXmlReaderInfo  * const pReaderInfo,
                                                         ET9UINT              * const pnErrorLine)
{
    ET9KdbXmlKeyboardInfo * const pKeyboardInfo = &pReaderInfo->pKDBInfo->Private.wm.xmlReader.sKeyboardInfo;

    ET9STATUS eStatus;

    WLOG5(fprintf(pLogFile5, "__XmlReader_TransferLayout\n");)

    {
        ET9KdbXmlKeyboard * const pKeyboard = &pKeyboardInfo->sKeyboard;

        ET9UINT nRowCount;
        ET9KdbXmlRow const *pRow;

        /* properties */

        eStatus = ET9KDB_Load_SetProperties(pReaderInfo->pKDBInfo,
                                            pKeyboard->bMajorVersion,
                                            pKeyboard->bMinorVersion,
                                            pKeyboard->bPrimaryID,
                                            pKeyboard->bSecondaryID,
                                            pKeyboard->wLayoutWidth,
                                            pKeyboard->wLayoutHeight,
                                            pKeyboard->wConditionValueMax + 1);

        if (eStatus) {
            return eStatus;
        }

        /* loop rows */

        pRow = pKeyboard->pRows;
        for (nRowCount = pKeyboard->nRowCount; nRowCount; --nRowCount, ++pRow) {

            ET9UINT nKeyCount;
            ET9KdbXmlKey const *pKey;

            /* loop keys */

            pKey = pRow->pKeys;
            for (nKeyCount = pRow->nKeyCount; nKeyCount; --nKeyCount, ++pKey) {

                eStatus = ET9KDB_Load_AddKey(pReaderInfo->pKDBInfo,
                                             ET9_KDB_LOAD_UNDEF_VALUE,  /* key index */
                                             __XmlReader_XmlKeyTypeToLKT(pKey->eKeyType),
                                             (ET9U16)pKey->nULX,
                                             (ET9U16)pKey->nULY,
                                             (ET9U16)(pKey->nULX + pKey->nWidth - 1),
                                             (ET9U16)(pKey->nULY + pKey->nHeight - 1),
                                             pKey->nKeyCharCount,
                                             pKey->psKeyChars);

                if (eStatus) {
                    if (pnErrorLine) {
                        *pnErrorLine = pKey->nSourceLine;
                    }
                    return eStatus;
                }

                if (pKey->nKeyShiftedCharCount) {

                    eStatus = ET9KDB_Load_AttachShiftedChars(pReaderInfo->pKDBInfo,
                                                             pKey->nKeyShiftedCharCount,
                                                             pKey->psKeyShiftedChars);

                    if (eStatus) {
                        if (pnErrorLine) {
                            *pnErrorLine = pKey->nSourceLine;
                        }
                        return eStatus;
                    }
                }

                if (pKey->nKeyMultitapCharCount || pKey->nKeyMultitapShiftedCharCount) {

                    eStatus = ET9KDB_Load_AttachMultitapInfo(pReaderInfo->pKDBInfo,
                                                             pKey->nKeyMultitapCharCount,
                                                             pKey->psKeyMultitapChars,
                                                             pKey->nKeyMultitapShiftedCharCount,
                                                             pKey->psKeyMultitapShiftedChars);

                    if (eStatus) {
                        if (pnErrorLine) {
                            *pnErrorLine = pKey->nSourceLine;
                        }
                        return eStatus;
                    }
                }
            }
        }
    }

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param[in,out] ppPointer  Pointer to update.
 * @param[in]     pDst       Pointer to copy to.
 * @param[in]     pSrc       Pointer to copy from.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9INLINE static void ET9LOCALCALL __XmlReader_AdjustPointer(void                        ** const ppPointer,
                                                             ET9KdbXmlKeyboardInfo  const * const pDst,
                                                             ET9KdbXmlKeyboardInfo  const * const pSrc)
{
    ET9U8 *pbPointer = (ET9U8*)*ppPointer;
    ET9U8 const * const pbDst = (ET9U8*)pDst;
    ET9U8 const * const pbSrc = (ET9U8*)pSrc;

    if (pbPointer < pbSrc || pbPointer >= pbSrc + sizeof(ET9KdbXmlKeyboardInfo)) {
        return;
    }

    pbPointer += pbDst - pbSrc;

    *ppPointer = pbPointer;

    ET9Assert(pbPointer >= pbDst && pbPointer < pbDst + sizeof(ET9KdbXmlKeyboardInfo));
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param[out]    pDst       Pointer to copy to.
 * @param[in]     pSrc       Pointer to copy from.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

static void ET9LOCALCALL __XmlReader_CopyKeyboardInfo(ET9KdbXmlKeyboardInfo        * const pDst,
                                                      ET9KdbXmlKeyboardInfo  const * const pSrc)
{
    *pDst = *pSrc;

    /* adjust pointers */

    {
        ET9KdbXmlKeyboard * const pKeyboard = &pDst->sKeyboard;

        ET9UINT nRowCount;
        ET9KdbXmlRow *pRow;

        /* loop rows */

        __XmlReader_AdjustPointer((void**)&pKeyboard->pRows, pDst, pSrc);

        pRow = pKeyboard->pRows;
        for (nRowCount = pKeyboard->nRowCount; nRowCount; --nRowCount, ++pRow) {

            ET9UINT nKeyCount;
            ET9KdbXmlKey *pKey;

            /* loop keys */

            __XmlReader_AdjustPointer((void**)&pRow->pKeys, pDst, pSrc);

            pKey = pRow->pKeys;
            for (nKeyCount = pRow->nKeyCount; nKeyCount; --nKeyCount, ++pKey) {

                __XmlReader_AdjustPointer((void**)&pKey->psIconChars, pDst, pSrc);
                __XmlReader_AdjustPointer((void**)&pKey->psLabelChars, pDst, pSrc);
                __XmlReader_AdjustPointer((void**)&pKey->psLabelShiftedChars, pDst, pSrc);
                __XmlReader_AdjustPointer((void**)&pKey->psKeyChars, pDst, pSrc);
                __XmlReader_AdjustPointer((void**)&pKey->psKeyShiftedChars, pDst, pSrc);
                __XmlReader_AdjustPointer((void**)&pKey->psKeyPopupChars, pDst, pSrc);
                __XmlReader_AdjustPointer((void**)&pKey->psKeyPopupShiftedChars, pDst, pSrc);
                __XmlReader_AdjustPointer((void**)&pKey->psKeyMultitapChars, pDst, pSrc);
                __XmlReader_AdjustPointer((void**)&pKey->psKeyMultitapShiftedChars, pDst, pSrc);
            }
        }
    }
}

/*---------------------------------------------------------------------------*/
/**
 * @brief Load a dynamically using the xml format for KDB descriptions.
 *
 * @param[in]     pKDBInfo              Pointer to keyboard information structure.
 * @param[in]     wLayoutWidth          Layout width.
 * @param[in]     wLayoutHeight         Layout height.
 * @param[in]     nConditionValue       Row/Area inclusion condition (if used).
 * @param[in]     pbContent             Pointer to the content.
 * @param[in]     dwContentLen          Content length.
 * @param[out]    pKeyboardLayout       Pointer to store the actual keyboard layout (optional).
 * @param[out]    pnUnknownAttributes   Number of unknown attributes found (optional).
 * @param[out]    pnErrorLine           If an error occurs, this is an indication to where it happened (optional).
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL ET9KDB_Load_XmlKDB(ET9KDBInfo             * const pKDBInfo,
                                        const ET9U16                   wLayoutWidth,
                                        const ET9U16                   wLayoutHeight,
                                        const ET9UINT                  nConditionValue,
                                        ET9U8            const * const pbContent,
                                        const ET9U32                   dwContentLen,
                                        ET9KdbXmlKeyboardInfo  * const pKeyboardLayout,
                                        ET9UINT                * const pnUnknownAttributes,
                                        ET9UINT                * const pnErrorLine)
{
    ET9STATUS eStatus;

    WLOG5(fprintf(pLogFile5, "ET9KDB_Load_XmlKDB, pKDBInfo = %p, nConditionValue %u\n", pKDBInfo, nConditionValue);)

    eStatus = __ET9KDB_LoadValidityCheck(pKDBInfo);

    if (eStatus) {
        return eStatus;
    }

    if (!pbContent ||
        !dwContentLen) {
        return ET9STATUS_BAD_PARAM;
    }

    ++pKDBInfo->Private.nWmUseID;

    if (pKeyboardLayout) {
        pKeyboardLayout->sKeyboard.nRowCount = 0;
    }

    if (dwContentLen > 1 && (pbContent[0] == 0xFF && pbContent[1] == 0xFE) || (pbContent[0] == 0xFE && pbContent[1] == 0xFF)) {
        return ET9STATUS_KDB_WRONG_CONTENT_ENCODING;
    }

    if (pnErrorLine) {
        *pnErrorLine = 0;
    }

    if (pnUnknownAttributes) {
        *pnUnknownAttributes = 0;
    }

    {
        ET9KdbXmlReaderInfo sReaderInfo;

        _ET9ClearMem((ET9U8*)&sReaderInfo, sizeof(sReaderInfo));
        _ET9ClearMem((ET9U8*)&pKDBInfo->Private.wm.xmlReader.sKeyboardInfo, sizeof(pKDBInfo->Private.wm.xmlReader.sKeyboardInfo));

        sReaderInfo.pKDBInfo = pKDBInfo;
        sReaderInfo.nCurrLine = 1;
        sReaderInfo.wLayoutWidth = wLayoutWidth;
        sReaderInfo.wLayoutHeight = wLayoutHeight;
        sReaderInfo.nConditionValue = nConditionValue;
        sReaderInfo.pbEndChar = &pbContent[dwContentLen];
        sReaderInfo.pbCurrChar = pbContent;
        sReaderInfo.pbContent = pbContent;
        sReaderInfo.dwContentLen = dwContentLen;
        sReaderInfo.eCurrToken = ET9_xml_token_Last;

        if (!__XmlReader_GetNextToken(&sReaderInfo)) {
            return ET9STATUS_KDB_SYNTAX_ERROR;
        }

        eStatus = __XmlReader_ReadKeyboard(&sReaderInfo);

        if (eStatus) {

            if (pnErrorLine) {
                *pnErrorLine = sReaderInfo.nCurrLine;
            }

            return eStatus;
        }

        if (pnUnknownAttributes) {
            *pnUnknownAttributes = sReaderInfo.nUnknownAttributes;
        }

        eStatus = __XmlReader_CalculateLayout(&pKDBInfo->Private.wm.xmlReader.sKeyboardInfo);

        if (eStatus) {
            return eStatus;
        }

        if (pKeyboardLayout) {
            __XmlReader_CopyKeyboardInfo(pKeyboardLayout, &pKDBInfo->Private.wm.xmlReader.sKeyboardInfo);
        }

        eStatus = __XmlReader_TransferLayout(&sReaderInfo, pnErrorLine);

        if (eStatus) {
            return eStatus;
        }

        /* safety - to assure the info isn't used after this */

        _ET9ClearMem((ET9U8*)&pKDBInfo->Private.wm.xmlReader.sKeyboardInfo, sizeof(pKDBInfo->Private.wm.xmlReader.sKeyboardInfo));
    }

    return ET9STATUS_NONE;
}

/* ************************************************************************************************************** */
/* ************************************************************************************************************** */
/* ************************************************************************************************************** */

#ifdef ET9_KDB_TRACE_MODULE
#include "et9kdbtrace.c"
#endif /* ET9_KDB_TRACE_MODULE */


#endif  /* ET9_KDB_MODULE */

/*! @} */
/* ----------------------------------< eof >--------------------------------- */
