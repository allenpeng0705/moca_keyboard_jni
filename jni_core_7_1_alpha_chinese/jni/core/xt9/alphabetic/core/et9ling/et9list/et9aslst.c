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
;**     FileName: et9aslst.c                                                  **
;**                                                                           **
;**  Description: ET9 Alphabetic Selection List Module                        **
;**                                                                           **
;*******************************************************************************
;******************************************************************************/

/*! \addtogroup et9slst Selection list for alphabetic
* Selection list for alphabetic XT9.
* @{
*/

#include "et9api.h"
#ifdef ET9_ALPHABETIC_MODULE
#include "et9asys.h"
#include "et9asym.h"
#include "et9sym.h"
#include "et9amisc.h"
#include "et9adb.h"
#include "et9arudb.h"
#include "et9acdb.h"
#include "et9imu.h"
#include "et9aspc.h"
#include "et9aldb.h"
#include "et9aslst.h"
#include "et9sys.h"
#ifdef EVAL_BUILD
#include "__et9eval.h"
#endif


#ifdef ET9_DEBUGLOG2
#ifdef _WIN32
#pragma message ("*** ET9_DEBUGLOG2 ACTIVATED ***")
#endif
#include <stdio.h>
#include <string.h>
#define WLOG2(q) { if (pLogFile2 == NULL) { pLogFile2 = fopen("zzzET9ASlStB.txt", "w+"); } {q} fflush(pLogFile2);  }
static FILE *pLogFile2 = NULL;
#else
#define WLOG2(q)
#endif

#ifdef ET9_DEBUGLOG2B
#ifdef _WIN32
#pragma message ("*** ET9_DEBUGLOG2B ACTIVATED ***")
#endif
#define WLOG2B(q) WLOG2(q)
#define WLOG2BWord(p1,p2,p3) WLOG2Word(p1,p2,p3)
#else
#define WLOG2B(q)
#define WLOG2BWord(p1,p2,p3)
#endif

#if 1 /* log8 */

#ifdef ET9_DEBUGLOG8
#ifdef _WIN32
#pragma message ("*** ET9_DEBUGLOG8 ACTIVATED ***")
#endif
#include <stdio.h>
#include <string.h>
#define WLOG8(q) { if (pLogFile8 == NULL) { pLogFile8 = fopen("zzzET9LDBTRACE.txt", "w"); } { q fflush(pLogFile8); } }
static FILE *pLogFile8 = NULL;
#else
#define WLOG8(q)
#endif

#ifdef ET9_DEBUGLOG8B
#ifdef _WIN32
#pragma message ("*** ET9_DEBUGLOG8 ACTIVATED ***")
#endif
#define WLOG8B(q) WLOG8(q)
#else
#define WLOG8B(q)
#endif

#else /* log8 */

#define pLogFile8 pLogFile2

#ifndef WLOG8
#define WLOG8(q) WLOG2(q)
#endif

#ifndef WLOG8B
#define WLOG8B(q) WLOG2B(q)
#endif

#endif /* log8 */


#ifdef ET9_PRE_DUPE_SELLIST
#ifdef _WIN32
#pragma message ("*** ET9_PRE_DUPE_SELLIST ACTIVATED ***")
#endif
#include <stdlib.h>
const char DEFAULT_PREDUPE_FILE_NAME[] = "PreDupeSelList.txt";
FILE   *pPreDupeFile = 0;
ET9UINT nPreDupeEntryCount = 0;
static void ET9LOCALCALL __ET9PreDupeRecordWordInfo(ET9AWPrivWordInfo *pWord);
#endif /* ET9_PRE_DUPE_SELLIST */

#ifdef ET9_DEBUG
#include <stddef.h>
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


/*----------------------------------------------------------------------------
 * Defines
 *----------------------------------------------------------------------------*/

#undef __ValueSwap

#define __MaxStemDistanceFromInputLength(nInputLength) (((nInputLength) - 1) / 4 + 1)

/*---------------------------------------------------------------------------*/
/** \internal
 * Special build points.
 */

typedef enum ET9ASPECIAL_e {

    NOSPECIAL,                  /**< \internal no special */
    PUNCT,                      /**< \internal punct */
    SMARTPUNCT,                 /**< \internal smart punct */
    EXPLICIT,                   /**< \internal explicit */
    LOCKPOINT,                  /**< \internal lock point */

    LAST_SPECIAL                /**< \internal sentinel */
} ET9ASPECIAL;

/*---------------------------------------------------------------------------*/
/** \internal
 * Buildaround values.
 */

typedef enum ET9ABUILAROUND_e {

    BA_TRAILING,                   /**< \internal trailing buildaround */
    BA_LEADING,                    /**< \internal leading buildaround */
    BA_EMBEDDED,                   /**< \internal embedded buildaround */
    BA_COMPOUND,                   /**< \internal compound buildaround */

    BA_LAST                        /**< \internal sentinel */
} ET9ABUILAROUND;

/*---------------------------------------------------------------------------*/
/** \internal
 * Word quality values.
 * Priority in growing order, highest last.
 */

enum ET9AWORDQUALITY_e {

    UNDEF_QUALITY,              /**< \internal undefined */

    DISPOSABLE_QUALITY,         /**< \internal 1 - RDB words starts out here, they need to be teamed up with an LDB word for good quality */
    STEM_QUALITY,               /**< \internal 2 - artifacts that's good to have in the list, but not a must keeper */
    DUPE_QUALITY,               /**< \internal 3 - a word slated for duplication removal because the substitution is a dupe to a word in the list */
    LIMITED_QUALITY,            /**< \internal 4 - shortcuts from auto substitution */
    SHAPED_QUALITY,             /**< \internal 5 - a word that is shaped by the engine, but still very high quality */
    GENUINE_QUALITY,            /**< \internal 6 - a word that origins form a "real" source, like the LDB or the UDB */

    LAST_QUALITY                /**< \internal sentinel */
};


#define UNDEFINED_STRING_INDEX 0xFFFF       /**< \internal a value that indicates that there is no valid string available */

#define MAX_AUTO_APPEND_SYMBS 16            /**< \internal max number of auto-append candidates that will be inserted into the selection list */

#define WORD_SOURCE_DEMOTE_FENCE 2          /**< \internal fence for out-of-the-box word source demoting */

#define DUPE_BIN_SEARCH_POINT 10            /**< \internal point for when to start performing binary dupe search */

#define EMOTICON_LENGTH_THRESHOLD 5         /**< \internal length beyond which emoticon needs to be stripped out */

/*---------------------------------------------------------------------------*/
/** \internal
 * Determine if an input symbol is neutral to shift.
 *
 * @param pSymbInfo                 Pointer to an input symbol.
 *
 * @return Non zero if neutral, otherwise zero.
 */

ET9INLINE static ET9BOOL ET9LOCALCALL __IsShiftNeutralSymb(ET9SymbInfo const * const pSymbInfo)
{
    return (ET9BOOL)(pSymbInfo->bSymbType == ET9KTNUMBER ||
                     pSymbInfo->bSymbType == ET9KTSMARTPUNCT ||
                     pSymbInfo->bSymbType == ET9KTPUNCTUATION);
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Determine if an input symbol is exact.
 *
 * @param pSymbInfo                 Pointer to an input symbol.
 *
 * @return Non zero if exact, otherwise zero.
 */

ET9INLINE static ET9BOOL ET9LOCALCALL __IsExactSymb(ET9SymbInfo const * const pSymbInfo)
{
    if (pSymbInfo->bTraceIndex && pSymbInfo->bTraceProbability != 0xFF) {
        return 0;
    }

    if (pSymbInfo->bAmbigType == ET9EXACT) {
        return 1;
    }

    if (pSymbInfo->bNumBaseSyms == 1 &&
        pSymbInfo->DataPerBaseSym[0].bNumSymsToMatch == 1 &&
        _ET9_IsPunctOrNumeric(pSymbInfo->DataPerBaseSym[0].sChar[0])) {

        return 1;
    }

    if (pSymbInfo->bNumBaseSyms == 1 &&
        pSymbInfo->DataPerBaseSym[0].bNumSymsToMatch == 2 &&
        pSymbInfo->DataPerBaseSym[0].sChar[0] == pSymbInfo->DataPerBaseSym[0].sChar[1] &&
        _ET9_IsPunctOrNumeric((pSymbInfo)->DataPerBaseSym[0].sChar[0])) {

        return 1;
    }

    return 0;
}

#define QUALITYTOSTRING(x)          (x == GENUINE_QUALITY ? "GEN" : x == SHAPED_QUALITY ? "SHP" : x == LIMITED_QUALITY ? "LTD" : x == DUPE_QUALITY ? "DUP" : x == STEM_QUALITY ? "STM" : x == DISPOSABLE_QUALITY ? "DSP" : "\?\?\?")
#define LINDEXTOSTRING(x)           (x == ET9AWFIRST_LANGUAGE ? "1ST" : x == ET9AWSECOND_LANGUAGE ? "2ND" : x == ET9AWBOTH_LANGUAGES ? "BTH" : x == ET9AWUNKNOWN_LANGUAGE ? "UKN" : "\?\?\?")


#define SPECIALTOSTRING(x)          (x == NOSPECIAL ? "NOSPECIAL" : x == PUNCT ? "PUNCT" : x == SMARTPUNCT ? "SMARTPUNCT" : x == EXPLICIT ? "EXPLICIT" : x == LOCKPOINT ? "LOCKPOINT" : "\?\?\?")
#define BUILDAROUNDTOSTRING(x)      (x == BA_TRAILING ? "TRAILING" : x == BA_LEADING ? "LEADING" : x == BA_EMBEDDED ? "EMBEDDED" : x == BA_COMPOUND ? "COMPOUND" : "\?\?\?")

#define SELLSTMODETOSTRING(x)       (x == ET9ASLMODE_AUTO ? "AUTO" : x == ET9ASLMODE_CLASSIC ? "CLASSIC" : x == ET9ASLMODE_COMPLETIONSPROMOTED ? "COMPLETIONSPROMOTED" : x == ET9ASLMODE_MIXED ? "MIXED" : "\?\?\?")
#define SELLSTCORRMODETOSTRING(x)   (x == ET9ASLCORRECTIONMODE_LOW ? "LOW" : x == ET9ASLCORRECTIONMODE_MEDIUM ? "MEDIUM" : x == ET9ASLCORRECTIONMODE_HIGH ? "HIGH" : "\?\?\?")

#define SPCMODETOSTRING(x)          (x == ET9ASPCMODE_OFF ? "OFF" : x == ET9ASPCMODE_EXACT ? "EXACT" : x == ET9ASPCMODE_REGIONAL ? "REGIONAL" : "\?\?\?")
#define SPCFILTERTOSTRING(x)        (x == ET9ASPCSEARCHFILTER_UNFILTERED ? "UNFILTERED" : x == ET9ASPCSEARCHFILTER_ONE_EXACT ? "ONE_EXACT" : x == ET9ASPCSEARCHFILTER_ONE_REGIONAL ? "ONE_REGIONAL" : x == ET9ASPCSEARCHFILTER_TWO_EXACT ? "TWO_EXACT" : x == ET9ASPCSEARCHFILTER_TWO_REGIONAL ? "TWO_REGIONAL" : "\?\?\?")

#define ALTMODETOSTRING(x)          (x == ET9AW_AltMode_1 ? "AltMode_1" : x == ET9AW_AltMode_2 ? "AltMode_2" : "\?\?\?")

/*---------------------------------------------------------------------------*/
/** \internal
 * Determine if the situation warrants implicit context break.
 *
 * @param pLingInfo                 Pointer to linguistics module.
 *
 * @return Non zero if break context situation, otherwise zero.
 */

static ET9BOOL ET9LOCALCALL __IsCandidateForImplicitBreakContext(ET9AWLingInfo   * const pLingInfo) {
    return _ET9_IsTermPunct(pLingInfo, pLingInfo->pLingCmnInfo->wLdbNum, pLingInfo->pLingCmnInfo->Private.sContextWord[pLingInfo->pLingCmnInfo->Private.bContextWordSize - 1]);
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Calculate stems allowed based on input length and stems mode/point.
 *
 * @param pLingCmnInfo              Pointer to alphabetic cmn information structure.
 * @param wLength                   Input length.
 *
 * @return Non zero if stems allowed, otherwise zero.
 */

ET9INLINE static ET9BOOL ET9LOCALCALL __GetStemsAllowed(ET9AWLingCmnInfo  const * const pLingCmnInfo,
                                                        const ET9U16                    wLength)
{
    return (ET9WORDSTEMS_MODE(pLingCmnInfo) && wLength >= pLingCmnInfo->Private.wWordStemsPoint) ? 1 : 0;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Calculate max word length based on input length and completion mode/point.
 * Before a completion point candidates will never been shown longer than this
 * length, so there is no need to generate that part.
 * Still need to have enough info to tell if it's a term or a stem though.
 *
 * @param pLingCmnInfo              Pointer to alphabetic cmn information structure.
 * @param wLength                   Input length.
 *
 * @return Max word length.
 */

ET9INLINE static ET9U16 ET9LOCALCALL __GetMaxWordLength(ET9AWLingCmnInfo  const * const pLingCmnInfo,
                                                        const ET9U16                    wLength)
{
    return (wLength &&
            (pLingCmnInfo->Base.pWordSymbInfo->Private.bRequiredInhibitCorrection ||
             !ET9WORDCOMPLETION_MODE(pLingCmnInfo) ||
             (wLength < pLingCmnInfo->Private.wWordCompletionPoint))) ?
           wLength :
           ET9MAXWORDSIZE;
}


static ET9STATUS ET9LOCALCALL __ET9AWDoSelLstBuild(ET9AWLingInfo    * const pLingInfo,
                                                   ET9U8            * const pbTotalWords,
                                                   const ET9BOOL            bSuppressBuild);

static void ET9LOCALCALL __ResetAllCaptureInfo(ET9AWLingInfo * const pLingInfo);

static void   ET9LOCALCALL __CaptureWord(ET9AWLingInfo      * const pLingInfo,
                                         ET9AWPrivWordInfo  * const pWord,
                                         const ET9U16               wSymbolLen);

static void ET9LOCALCALL __CaptureDefault(ET9AWLingInfo       * const pLingInfo,
                                          ET9AWPrivWordInfo   * const pWord,
                                          const ET9U16                wSymbolLen,
                                          const ET9U16                bLastBuildLen);

static ET9U16 ET9LOCALCALL __CaptureGetFlushStringLengthAtPoint(ET9AWLingInfo   * const pLingInfo,
                                                                const ET9U16            wSymbolPoint);

#ifdef ET9_DEBUGLOG2
#define ET9AssertLog(x) { (void)((!!(x)) || !pLogFile2 || fprintf(pLogFile2, "\n\nASSERT line %d\n%s\n\n", __LINE__, #x) && fflush(pLogFile2)); ET9Assert(x); }
#else /* ET9_DEBUGLOG2 */
#define ET9AssertLog(x) ET9Assert(x)
#endif /* ET9_DEBUGLOG2 */


#ifdef ET9_DEBUGLOG2
#include <stdio.h>
/*---------------------------------------------------------------------------*/
/** \internal
 * Word source to string.
 *
 * @param bSrc                      A word source.
 * @param bIsUDBWord                If the word comes from the UDB (not the RDB).
 *
 * @return String
 */

static char* ET9LOCALCALL __sourceToString(ET9U8 bSrc, ET9BOOL bIsUDBWord)
{
    char *pcwsrc;
    static char pcwTmp[8];

    switch (GETBASESRC(bSrc))
    {
        case ET9WORDSRC_NONE:                           pcwsrc = "NONE"; break;
        case ET9WORDSRC_EXACT:                          pcwsrc = "EXACT"; break;
        case ET9WORDSRC_EXACTISH:                       pcwsrc = "EXACTISH"; break;
        case ET9WORDSRC_CDB:                            pcwsrc = "CDB"; break;
        case ET9WORDSRC_QUDB:                           pcwsrc = "QUDB"; break;
        case ET9WORDSRC_RUDB:                           pcwsrc = bIsUDBWord ? "UDB" : "RDB"; break;
        case ET9WORDSRC_MDB:                            pcwsrc = "MDB"; break;
        case ET9WORDSRC_LDB_PROMOTION:                  pcwsrc = "LDBP"; break;
        case ET9WORDSRC_ASDB_SHORTCUT:                  pcwsrc = "ASDB_SHORTCUT"; break;
        case ET9WORDSRC_LAS_SHORTCUT:                   pcwsrc = "LAS_SHORTCUT"; break;
        case ET9WORDSRC_LDB:                            pcwsrc = "LDB"; break;
        case ET9WORDSRC_KDB:                            pcwsrc = "KDB"; break;
        case ET9WORDSRC_BUILDAROUND_CDB:                pcwsrc = "BUILDAROUND_CDB"; break;
        case ET9WORDSRC_BUILDAROUND_QUDB:               pcwsrc = "BUILDAROUND_QUDB"; break;
        case ET9WORDSRC_BUILDAROUND_RUDB:               pcwsrc = bIsUDBWord ? "BUILDAROUND_UDB" : "BUILDAROUND_RDB"; break;
        case ET9WORDSRC_BUILDAROUND_MDB:                pcwsrc = "BUILDAROUND_MDB"; break;
        case ET9WORDSRC_BUILDAROUND_LDB_PROMOTION:      pcwsrc = "BUILDAROUND_LDBP"; break;
        case ET9WORDSRC_BUILDAROUND_ASDB:               pcwsrc = "BUILDAROUND_ASDB"; break;
        case ET9WORDSRC_BUILDAROUND_LAS:                pcwsrc = "BUILDAROUND_LAS"; break;
        case ET9WORDSRC_BUILDAROUND_LDB:                pcwsrc = "BUILDAROUND_LDB"; break;
        case ET9WORDSRC_BUILDAROUND_KDB:                pcwsrc = "BUILDAROUND_KDB"; break;
        case ET9WORDSRC_BUILDCOMPOUND_CDB:              pcwsrc = "BUILDCOMPOUND_CDB"; break;
        case ET9WORDSRC_BUILDCOMPOUND_QUDB:             pcwsrc = "BUILDCOMPOUND_QUDB"; break;
        case ET9WORDSRC_BUILDCOMPOUND_RUDB:             pcwsrc = bIsUDBWord ? "BUILDCOMPOUND_UDB" : "BUILDCOMPOUND_RDB"; break;
        case ET9WORDSRC_BUILDCOMPOUND_MDB:              pcwsrc = "BUILDCOMPOUND_MDB"; break;
        case ET9WORDSRC_BUILDCOMPOUND_LDB_PROMOTION:    pcwsrc = "BUILDCOMPOUND_LDBP"; break;
        case ET9WORDSRC_BUILDCOMPOUND_ASDB:             pcwsrc = "BUILDCOMPOUND_ASDB"; break;
        case ET9WORDSRC_BUILDCOMPOUND_LAS:              pcwsrc = "BUILDCOMPOUND_LAS"; break;
        case ET9WORDSRC_BUILDCOMPOUND_LDB:              pcwsrc = "BUILDCOMPOUND_LDB"; break;
        case ET9WORDSRC_BUILDCOMPOUND_KDB:              pcwsrc = "BUILDCOMPOUND_KDB"; break;
        case ET9WORDSRC_TERMPUNCT:                      pcwsrc = "TERMPUNCT"; break;
        case ET9WORDSRC_COMPLETIONPUNCT:                pcwsrc = "COMPLETIONPUNCT"; break;
        case ET9WORDSRC_REQUIRED:                       pcwsrc = "REQUIRED"; break;
        case ET9WORDSRC_STEMPOOL:                       pcwsrc = "STEMPOOL"; break;
        case ET9WORDSRC_MAGICSTRING:                    pcwsrc = "MAGICSTRING"; break;
        case ET9WORDSRC_STEM:                           pcwsrc = "STEM"; break;
        case ET9WORDSRC_AUTOAPPEND:                     pcwsrc = "AUTOAPPEND"; break;
        case ET9WORDSRC_BUILDAPPEND:                    pcwsrc = "BUILDAPPEND"; break;
        case ET9WORDSRC_BUILDAPPENDPUNCT:               pcwsrc = "BUILDAPPENDPUNCT"; break;
        default : pcwsrc = pcwTmp; sprintf(pcwTmp, "%d", GETBASESRC(bSrc)); break;
    }

    return pcwsrc;
}
#endif /* ET9_DEBUGLOG2 */

#ifdef ET9_DEBUGLOG2
/*---------------------------------------------------------------------------*/
/** \internal
 * Logs input symbs etc.
 *
 * @param pLingInfo                 Pointer to alphabetic information structure.
 * @param bCount                    Number of symbols to log.
 * @param pfLogFile                 File to log to.
 *
 * @return None
 */

static void ET9LOCALCALL __LogInputSymbs(ET9AWLingInfo  *pLingInfo,
                                         ET9U8          bCount,
                                         FILE           *pfLogFile)
{
    ET9U16 wIndex;

    ET9AWLingCmnInfo * const pLingCmnInfo = pLingInfo->pLingCmnInfo;
    ET9WordSymbInfo * const pWordSymbInfo = pLingCmnInfo->Base.pWordSymbInfo;

    for (wIndex = 0; wIndex < bCount; ++wIndex) {

        ET9U16 wBaseIndex;
        ET9U16 wSymsIndex;

        ET9SymbInfo * const pSymbInfo = &pWordSymbInfo->SymbsInfo[wIndex];

        fprintf(pfLogFile, "Pos %2d ", wIndex);

        switch (pSymbInfo->eInputType)
        {
            case ET9DISCRETEKEY:        fprintf(pfLogFile, "ET9DISCRETEKEY"); break;
            case ET9REGIONALKEY:        fprintf(pfLogFile, "ET9REGIONALKEY"); break;
            case ET9HANDWRITING:        fprintf(pfLogFile, "ET9HANDWRITING"); break;
            case ET9MULTITAPKEY:        fprintf(pfLogFile, "ET9MULTITAPKEY"); break;
            case ET9CUSTOMSET:          fprintf(pfLogFile, "ET9CUSTOMSET"); break;
            case ET9EXPLICITSYM:        fprintf(pfLogFile, "ET9EXPLICITSYM"); break;
            case ET9MULTISYMBEXPLICIT:  fprintf(pfLogFile, "ET9MULTISYMBEXPLICIT"); break;
            default:                    fprintf(pfLogFile, "Input<%d>", pSymbInfo->eInputType); break;
        }

        fprintf(pfLogFile, "  ");

        switch (pSymbInfo->bAmbigType)
        {
            case ET9AMBIG:              fprintf(pfLogFile, "ET9AMBIG"); break;
            case ET9EXACT:              fprintf(pfLogFile, "ET9EXACT"); break;
            default:                    fprintf(pfLogFile, "Ambig<%d>", pSymbInfo->bAmbigType); break;
        }

        if (pSymbInfo->bAmbigType == ET9AMBIG && __IsExactSymb(pSymbInfo)) {
            fprintf(pfLogFile, "+SSX");
        }

        fprintf(pfLogFile, "  ");

        switch (pSymbInfo->bSymbType)
        {
            case ET9KTINVALID:          fprintf(pfLogFile, "ET9KTINVALID"); break;
            case ET9KTLETTER:           fprintf(pfLogFile, "ET9KTLETTER"); break;
            case ET9KTPUNCTUATION:      fprintf(pfLogFile, "ET9KTPUNCTUATION"); break;
            case ET9KTNUMBER:           fprintf(pfLogFile, "ET9KTNUMBER"); break;
            case ET9KTSTRING:           fprintf(pfLogFile, "ET9KTSTRING"); break;
            case ET9KTFUNCTION:         fprintf(pfLogFile, "ET9KTFUNCTION"); break;
            case ET9KTSMARTPUNCT:       fprintf(pfLogFile, "ET9KTSMARTPUNCT"); break;
            case ET9KTUNKNOWN:          fprintf(pfLogFile, "ET9KTUNKNOWN"); break;
            default:                    fprintf(pfLogFile, "Symb<%d>", pSymbInfo->bSymbType); break;
        }

        if (pSymbInfo->eShiftState == ET9SHIFT) {
            fprintf(pfLogFile, " SHIFT");
        }
        else if (pSymbInfo->eShiftState == ET9CAPSLOCK) {
            fprintf(pfLogFile, " CAPSLOCK");
        }
        else if (pSymbInfo->bForcedLowercase) {
            fprintf(pfLogFile, " FORCED LOWERCASE");
        }

        if (ET9_SPC_IS_LOCKED_POS(pSymbInfo)) {
            fprintf(pfLogFile, " SPC-LOCKED");
        }

        if (pLingCmnInfo->Private.sBuildInfo.pbFlushPos[wIndex] &&
            pLingCmnInfo->Private.sBuildInfo.pbFlushPos[wIndex]<= pLingCmnInfo->Private.bTotalSymbInputs) {

            fprintf(pfLogFile, " FLUSHED");
        }

        switch (pSymbInfo->bLocked)
        {
            case ET9NOLOCK:             break;
            case ET9STEMLOCK:           fprintf(pfLogFile, " STEMLOCK"); break;
            case ET9EXACTLOCK:          fprintf(pfLogFile, " EXACTLOCK"); break;
            default:                    fprintf(pfLogFile, " LOCK<%d>", pSymbInfo->bLocked); break;
        }

        if (pSymbInfo->wKeyIndex != ET9UNDEFINEDKEYVALUE) {
            fprintf(pfLogFile, " (Key %d)", (int)pSymbInfo->wKeyIndex);
        }

        if (pSymbInfo->wTapX != ET9UNDEFINEDTAPVALUE && pSymbInfo->wTapY != ET9UNDEFINEDTAPVALUE) {
            fprintf(pfLogFile, " [%d,%d]", (int)pSymbInfo->wTapX, (int)pSymbInfo->wTapY);
        }

        if (pSymbInfo->eInputType == ET9MULTISYMBEXPLICIT) {
            fprintf(pfLogFile, " (built by %d)", (int)pSymbInfo->wInputIndex);
        }

        fprintf(pfLogFile, " (ext %d %d)", (int)pSymbInfo->bTraceProbability, (int)pSymbInfo->bTraceIndex);

        fprintf(pfLogFile, "\n");

        for (wBaseIndex = 0; wBaseIndex < pSymbInfo->bNumBaseSyms; ++wBaseIndex) {

            fprintf(pfLogFile, "  SymbsInfo %d   (%3d)   ", wBaseIndex, pSymbInfo->DataPerBaseSym[wBaseIndex].bSymFreq);

            for (wSymsIndex = 0; wSymsIndex < pSymbInfo->DataPerBaseSym[wBaseIndex].bNumSymsToMatch; ++wSymsIndex) {
                ET9SYMB sChar = pSymbInfo->DataPerBaseSym[wBaseIndex].sChar[wSymsIndex];
                if (sChar > 0x20 && sChar <= 0x7F) {
                    fprintf(pfLogFile, "%c", sChar);
                }
                else {
                    fprintf(pfLogFile, "<%x>", sChar);
                }
            }
            fprintf(pfLogFile, " ");
            for (wSymsIndex = 0; wSymsIndex < pSymbInfo->DataPerBaseSym[wBaseIndex].bNumSymsToMatch; ++wSymsIndex) {
                ET9SYMB sChar = pSymbInfo->DataPerBaseSym[wBaseIndex].sUpperCaseChar[wSymsIndex];
                if (sChar > 0x20 && sChar <= 0x7F) {
                    fprintf(pfLogFile, "%c", sChar);
                }
                else {
                    fprintf(pfLogFile, "<%x>", sChar);
                }
            }

            fprintf(pfLogFile, "\n");
        }
    }

    fprintf(pfLogFile, "\n");

    fflush(pfLogFile);
}

/*---------------------------------------------------------------------------*/
/** \internal
 * This function logs a partial selection list (without input symbs etc).
 *
 * @param pLingCmnInfo              Pointer to alphabetic cmn information structure.
 * @param bFirstPos                 First pos in the list.
 * @param bLastPos                  Last pos in the list.
 * @param pcNote                    Note to be logged with the list.
 * @param pfLogFile                 File to log to.
 *
 * @return None
 */

static void ET9LOCALCALL __LogPartialSelList(ET9AWLingCmnInfo * const pLingCmnInfo,
                                             ET9U8                    bFirstPos,
                                             ET9U8                    bLastPos,
                                             char                     *pcNote,
                                             FILE                     *pfLogFile)
{
    ET9U16 wIndex;

    FILE *pfLog = pfLogFile;

    if (bFirstPos >= pLingCmnInfo->Private.bTotalWords || bLastPos >= pLingCmnInfo->Private.bTotalWords) {
        fprintf(pfLog, "__LogPartialSelList - invalid partition\n");
        return;
    }

    fprintf(pfLog, "\n");

    if (pcNote && strlen(pcNote)) {
        fprintf(pfLog, "Note: %s\n\n", pcNote);
    }

    for (wIndex = bFirstPos; wIndex <= bLastPos; ++wIndex) {

        ET9SYMB *ps;
        ET9U16 wCount;
        char *pcwsrc;
        char *pcwsrcext;
        ET9U8 bIndex = pLingCmnInfo->Private.bWordList[wIndex];
        ET9AWPrivWordInfo *pWord = &pLingCmnInfo->Private.pWordList[bIndex];

        ET9AssertLog(pWord->Base.wWordLen);

        if (ISBASESRC(pWord->bWordSrc)) {
            pcwsrcext = "  ";
        }
        else if (ISEXACTSRC(pWord->bWordSrc)) {
            pcwsrcext = "X ";
        }
        else {
            pcwsrcext = "XS";
        }

        pcwsrc = __sourceToString(pWord->bWordSrc, pWord->bIsUDBWord);

        fprintf(pfLog, "[%02d] SD %2X, ED %2X:%1u|%u%u%u%u FD %2X, P %u%u, %18.0f (%14.2f %16.0f %6u), %s %s %c %s %-18s",
            (bFirstPos == 0 && bLastPos == pLingCmnInfo->Private.bTotalWords - 1 ? bIndex : wIndex),
                        pWord->bEditDistStem,
                        pWord->bEditDistSpc,
                        pWord->bHasPrimEditDist,
                        pWord->bEditDistSpcSbt,
                        pWord->bEditDistSpcTrp,
                        pWord->bEditDistSpcIns,
                        pWord->bEditDistSpcDel,
                        pWord->bEditDistFree,
                        pWord->bIsAcronym,
                        pWord->bIsCapitalized,
                        (double)pWord->xTotFreq,
                        (double)pWord->xTapFreq,
                        (double)pWord->xWordFreq,
                        pWord->dwWordIndex,
                        LINDEXTOSTRING(pWord->Base.bLangIndex),
                        pcwsrcext,
                        (char)(pWord->Base.bIsTerm ? 'T' : 'S'),
                        QUALITYTOSTRING(pWord->bWordQuality),
                        pcwsrc);


        if (wIndex == pLingCmnInfo->Private.bDefaultIndex) {
            fprintf(pfLog, " * ");
        }
        else {
            fprintf(pfLog, " : ");
        }

        fprintf(pfLog, "%02u ", pWord->Base.wWordLen);

        for (ps = pWord->Base.sWord, wCount = pWord->Base.wWordLen; wCount; ++ps, --wCount) {
            if (wCount == pWord->Base.wWordCompLen) {
                fprintf(pfLog, "|");
            }
            if (*ps >= 0x20 && *ps <= 0x7F) {
                fprintf(pfLog, "%c", (char)*ps);
            }
            else {
                fprintf(pfLog, "<%x>", (int)*ps);
            }
        }

        if (pWord->Base.wSubstitutionLen) {
            fprintf(pfLog, " -> ");

            for (ps = pWord->Base.sSubstitution, wCount = pWord->Base.wSubstitutionLen; wCount; ++ps, --wCount) {
                if (*ps >= 0x20 && *ps <= 0x7F) {
                    fprintf(pfLog, "%c", (char)*ps);
                }
                else {
                    fprintf(pfLog, "<%x>", (int)*ps);
                }
            }
        }

        fprintf(pfLog, "\n");
    }

    fprintf(pfLog, "\n");

    fflush(pfLog);
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Logs a complete selection list (with input symbs etc).
 *
 * @param pLingInfo                 Pointer to alphabetic information structure.
 * @param pcNote                    Note to be logged with the list.
 * @param pfLogFile                 File to log to.
 *
 * @return None
 */

static void ET9LOCALCALL __LogFullSelList(ET9AWLingInfo  *pLingInfo,
                                          char           *pcNote,
                                          FILE           *pfLogFile)
{
    FILE *pfLog;

    ET9AWLingCmnInfo * const pLingCmnInfo = pLingInfo->pLingCmnInfo;

    if (pfLogFile != NULL) {
        pfLog = pfLogFile;
    }
    else {
        pfLog = fopen("zSlstLog.txt", "w");
    }

    if (!pfLog) {
        return;
    }

    if (pfLogFile != NULL) {
        fprintf(pfLog, "\n***** SELECTION LIST START *****\n\n");
    }

    fprintf(pfLog, "Note: %s\n\n", pcNote);

    fprintf(pfLog, "SPC mode = %d\n", pLingCmnInfo->Private.ASpc.eMode);
    fprintf(pfLog, "Selection list mode = %d\n", pLingCmnInfo->Private.eSelectionListMode);
    fprintf(pfLog, "Post Sort = %d\n", ET9POSTSORTENABLED(pLingCmnInfo));
    fprintf(pfLog, "\n");

#ifdef ET9_ACTIVATE_SLST_STATS
    _ET9AWSpcPrintStats(pfLog, pLingInfo);
#endif

    __LogInputSymbs(pLingInfo, pLingCmnInfo->Private.bTotalSymbInputs, pfLog);

    if (pLingCmnInfo->Private.bTotalWords) {
        __LogPartialSelList(pLingInfo->pLingCmnInfo, 0, (ET9U8)(pLingCmnInfo->Private.bTotalWords - 1), "", pfLog);
    }
    else {
        fprintf(pfLog, "<<empty list>>\n\n");
    }

    fflush(pfLog);

    if (pfLogFile != NULL) {
        fprintf(pfLog, "\n..... selection list end .....\n\n");
    }

    if (pfLogFile == NULL) {
        fclose(pfLog);
    }
}
#else /* ET9_DEBUGLOG2 */
#define __LogInputSymbs(pLingInfo,bCount,pfLogFile)
#define __LogPartialSelList(pLingInfo,bFirstPos,bLastPos,pcNote,pfLogFile)
#define __LogFullSelList(pLingInfo,pcNote,pfLogFile)
#endif /* ET9_DEBUGLOG2 */

#ifdef ET9_DEBUGLOG2
/*---------------------------------------------------------------------------*/
/** \internal
 * Logs a string.
 *
 * @param pF                        File to log to.
 * @param pcComment                 Comment to be logged with the string.
 * @param psString                  Pointer to the string.
 * @param snLen                     Length of the string.
 * @param bReverse                  If string should be reversed.
 * @param bSilent                   If less verbose.
 *
 * @return Length of string written.
 */

static ET9UINT ET9LOCALCALL WLOG2String(FILE *pF, char *pcComment, ET9SYMB *psString, ET9INT snLen, ET9BOOL bReverse, ET9BOOL bSilent)
{
    ET9INT snIndex;

    ET9UINT nLenUsed = 0;

    ET9AssertLog(pF != NULL);
    ET9AssertLog(pcComment != NULL);
    ET9AssertLog(psString != NULL);

    if (pcComment && *pcComment) {
        fprintf(pF, "%s: ", pcComment);
    }

    for (snIndex = bReverse ? (snLen - 1): 0; bReverse ? snIndex >= 0: snIndex < snLen; bReverse ? --snIndex : ++snIndex) {

        ET9SYMB sSymb = psString[snIndex];

        if (sSymb >= 0x20 && sSymb <= 0xFF) {
            nLenUsed += fprintf(pF, "%c", (unsigned char)sSymb);
        }
        else {
            nLenUsed += fprintf(pF, "<%x>", (int)sSymb);
        }
    }

    if (!bSilent) {
        fprintf(pF, "  (%d)\n", snLen);
    }

    fflush(pF);

#ifdef ET9_DEBUG
    {
        ET9U16 wCount = (ET9U16)snLen;
        ET9SYMB *pSymb = psString;

        ET9AssertLog(snLen <= ET9MAXWORDSIZE);

        while (wCount--) {
            ET9AssertLog(*pSymb && *pSymb != (ET9SYMB)0xcccc);
            ++pSymb;
        }
    }
#endif

    return nLenUsed;
}
#else /* ET9_DEBUGLOG2 */
#define WLOG2String(pF,pcComment,psString,snLen,bReverse,bSilent)
#endif /* ET9_DEBUGLOG2 */

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

    ET9AssertLog(pF != NULL);
    ET9AssertLog(pcComment != NULL);

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

    fprintf(pF, "  (%c %s %s : %d,%d : %d,%d:%d|%u%u%u%u : %s %10.2f %8.0f %u)\n",
                (char)(pWord->Base.bIsTerm ? 'T' : 'S'),
                QUALITYTOSTRING(pWord->bWordQuality),
                LINDEXTOSTRING(pWord->Base.bLangIndex),
                pWord->Base.wWordLen,
                pWord->Base.wWordCompLen,
                pWord->bEditDistStem,
                pWord->bEditDistSpc,
                pWord->bHasPrimEditDist,
                pWord->bEditDistSpcSbt,
                pWord->bEditDistSpcTrp,
                pWord->bEditDistSpcIns,
                pWord->bEditDistSpcDel,
                __sourceToString(pWord->bWordSrc, pWord->bIsUDBWord),
                (double)pWord->xTapFreq,
                (double)pWord->xWordFreq,
                pWord->dwWordIndex);

    fflush(pF);

#ifdef ET9_DEBUG
    {
        ET9U16 wCount = pWord->Base.wWordLen;
        ET9SYMB *pSymb = pWord->Base.sWord;

        ET9AssertLog(pWord->Base.wWordLen <= ET9MAXWORDSIZE);
        ET9AssertLog(pWord->Base.wWordCompLen <= pWord->Base.wWordLen);
        ET9AssertLog(pWord->Base.wSubstitutionLen <= ET9MAXSUBSTITUTIONSIZE);

        while (wCount--) {
            ET9AssertLog(*pSymb && *pSymb != (ET9SYMB)0xcccc);
            ++pSymb;
        }
    }
#endif
}
#else /* ET9_DEBUGLOG2 */
#define WLOG2Word(pF,pcComment,pWord)
#endif /* ET9_DEBUGLOG2 */

#ifdef ET9_DEBUGLOG2
/*---------------------------------------------------------------------------*/
/** \internal
 * Logs the internal word capture info.
 *
 * @param pF                        File to log to.
 * @param pcComment                 Comment to be logged with the string.
 * @param pLingInfo                 Pointer to alphabetic information structure.
 *
 * @return None
 */

static void ET9LOCALCALL WLOG2Captures(FILE *pF, char *pcComment, ET9AWLingInfo *pLingInfo)
{
    ET9U16 wIndex;
    ET9U16 wSymbolIndex;
    ET9U16 wCapture;
    ET9U16 wMaxCaptureLen = 0;

    ET9AWLingCmnInfo * const pLingCmnInfo = pLingInfo->pLingCmnInfo;
    ET9WordSymbInfo * const pWordSymbInfo = pLingCmnInfo->Base.pWordSymbInfo;

    ET9AssertLog(pF != NULL);
    ET9AssertLog(pcComment != NULL);

    fprintf(pF, "%s (captures log)\n", pcComment);

    for (wCapture = 0; wCapture < ET9MAXBUILDCAPTURES; ++wCapture) {

        if (pLingCmnInfo->Private.sBuildInfo.pCaptures[wCapture].bIsValid) {

            if (pLingCmnInfo->Private.sBuildInfo.pCaptures[wCapture].wWordLen > wMaxCaptureLen) {

                wMaxCaptureLen = pLingCmnInfo->Private.sBuildInfo.pCaptures[wCapture].wWordLen;
            }
        }
    }

    for (wCapture = 0; wCapture < ET9MAXBUILDCAPTURES; ++wCapture) {

        fprintf(pF, "  Capture %d, bIsValid = %d %s ", wCapture, pLingCmnInfo->Private.sBuildInfo.pCaptures[wCapture].bIsValid, (char*)(wCapture == pLingCmnInfo->Private.sBuildInfo.bCurrCapture ? "<*>" : "   "));

        if (pLingCmnInfo->Private.sBuildInfo.pCaptures[wCapture].bIsValid) {

            ET9U8 bCompChar = 0;

            for (wIndex = 0; wIndex < pLingCmnInfo->Private.sBuildInfo.pCaptures[wCapture].wWordLen; ++wIndex) {

                ET9SYMB sSymb = pLingCmnInfo->Private.sBuildInfo.pCaptures[wCapture].sWord[wIndex];

                if (wIndex == (pLingCmnInfo->Private.sBuildInfo.pCaptures[wCapture].wWordLen - pLingCmnInfo->Private.sBuildInfo.pCaptures[wCapture].wWordCompLen)) {
                    fprintf(pF, "|");
                    bCompChar = 1;
                }

                if (sSymb >= 0x20 && sSymb <= 0x7F && sSymb != '|') {
                    fprintf(pF, "%c", (char)sSymb);
                }
                else {
                    fprintf(pF, "<%x>", (int)sSymb);
                }
            }

            for (; wIndex < wMaxCaptureLen + 3 - bCompChar; ++wIndex) {
                fprintf(pF, " ");
            }

            fprintf(pF, "  (len %2d, comp len %2d, symb len %2d)",
                    pLingCmnInfo->Private.sBuildInfo.pCaptures[wCapture].wWordLen,
                    pLingCmnInfo->Private.sBuildInfo.pCaptures[wCapture].wWordCompLen,
                    pLingCmnInfo->Private.sBuildInfo.pCaptures[wCapture].wSymbolLen);
        }

        fprintf(pF, "\n");
    }

    fprintf(pF, "  Capture actions log (%d symbs):", pWordSymbInfo->bNumSymbs);

    for (wIndex = 0; wIndex <= pWordSymbInfo->bNumSymbs; ++wIndex) {

        if (pLingCmnInfo->Private.sBuildInfo.pCaptureActions[wIndex].bPop) {
            fprintf(pF, " POP");
        }
        else if (!pLingCmnInfo->Private.sBuildInfo.pCaptureActions[wIndex].sbAddWordLen &&
                 !pLingCmnInfo->Private.sBuildInfo.pCaptureActions[wIndex].sbAddWordCompLen &&
                 !pLingCmnInfo->Private.sBuildInfo.pCaptureActions[wIndex].bAddSymbolLen) {
            fprintf(pF, " -");
        }
        else {
            fprintf(pF, " (%d,%d,%d)",
                    pLingCmnInfo->Private.sBuildInfo.pCaptureActions[wIndex].sbAddWordLen,
                    pLingCmnInfo->Private.sBuildInfo.pCaptureActions[wIndex].sbAddWordCompLen,
                    pLingCmnInfo->Private.sBuildInfo.pCaptureActions[wIndex].bAddSymbolLen);
        }

        if (wIndex % 5 == 4) {
            fprintf(pF, " .");
        }
    }

    fprintf(pF, "\n");

    fprintf(pF, "  Capture locked symbs:  ");

    for (wIndex = 0; wIndex < ET9MAXWORDSIZE; ++wIndex) {

        const ET9SYMB sSymb = pWordSymbInfo->SymbsInfo[wIndex].sLockedSymb;

        if (!sSymb) {
            break;
        }

        if (sSymb >= 0x20 && sSymb <= 0x7F && sSymb != '|') {
            fprintf(pF, "%c", (char)sSymb);
        }
        else {
            fprintf(pF, "<%x>", (int)sSymb);
        }
    }

    fprintf(pF, "\n");

    fprintf(pF, "  Capture flushed symbs: ");

    for (wIndex = 0; wIndex < ET9MAXWORDSIZE; ++wIndex) {

        const ET9SYMB sSymb = pLingCmnInfo->Private.sBuildInfo.psFlushedSymbs[wIndex];

        if (!sSymb) {
            break;
        }

        if (sSymb >= 0x20 && sSymb <= 0x7F && sSymb != '|') {
            fprintf(pF, "%c", (char)sSymb);
        }
        else {
            fprintf(pF, "<%x>", (int)sSymb);
        }
    }

    fprintf(pF, "\n");

    fprintf(pF, "  Capture flush pos log (%d symbs):   ", pWordSymbInfo->bNumSymbs);

    for (wIndex = 0; wIndex <= pWordSymbInfo->bNumSymbs; ++wIndex) {

        fprintf(pF, "%2d ", pLingCmnInfo->Private.sBuildInfo.pbFlushPos[wIndex]);

        if (wIndex % 5 == 4) {
            fprintf(pF, ". ");
        }
    }

    fprintf(pF, "\n");

    fprintf(pF, "  Capture flush len log (%d symbs):   ", pWordSymbInfo->bNumSymbs);

    for (wIndex = 0; wIndex <= pWordSymbInfo->bNumSymbs; ++wIndex) {

        fprintf(pF, "%2d ", pLingCmnInfo->Private.sBuildInfo.pbFlushLen[wIndex]);

        if (wIndex % 5 == 4) {
            fprintf(pF, ". ");
        }
    }

    fprintf(pF, "\n");

    for (wSymbolIndex = 0; wSymbolIndex < pWordSymbInfo->bNumSymbs; ++wSymbolIndex) {

        fprintf(pF, "  Capture default %02d: %2d %2d (%4x) ",
                    wSymbolIndex,
                    pLingCmnInfo->Private.sBuildInfo.pbDefaultLen[wSymbolIndex],
                    pLingCmnInfo->Private.sBuildInfo.pbDefaultCompLen[wSymbolIndex],
                    pLingCmnInfo->Private.sBuildInfo.pwDefaultPos[wSymbolIndex]);

        if (pLingCmnInfo->Private.sBuildInfo.pwDefaultPos[wSymbolIndex] == UNDEFINED_STRING_INDEX) {
            fprintf(pF, "<UNDEFINED>\n");
            continue;
        }

        for (wIndex = 0; wIndex < pLingCmnInfo->Private.sBuildInfo.pbDefaultLen[wSymbolIndex]; ++wIndex) {

            ET9SYMB sSymb = pLingCmnInfo->Private.sBuildInfo.psDefaultSymbs[pLingCmnInfo->Private.sBuildInfo.pwDefaultPos[wSymbolIndex] + wIndex];

            if (wIndex == (pLingCmnInfo->Private.sBuildInfo.pbDefaultLen[wSymbolIndex] - pLingCmnInfo->Private.sBuildInfo.pbDefaultCompLen[wSymbolIndex])) {
                fprintf(pF, "|");
            }

            if (sSymb >= 0x20 && sSymb <= 0x7F && sSymb != '|') {
                fprintf(pF, "%c", (char)sSymb);
            }
            else {
                fprintf(pF, "<%x>", (int)sSymb);
            }
        }

        fprintf(pF, "\n");
    }

    fflush(pF);
}
#else /* ET9_DEBUGLOG2 */
#define WLOG2Captures(pF,pcComment,pLingInfo)
#endif /* ET9_DEBUGLOG2 */

#ifdef ET9_DEBUGLOG2
/*---------------------------------------------------------------------------*/
/** \internal
 * Possibly shrink the log file.
 *
 * @param pF                        File to log to.
 *
 * @return None
 */

static void ET9LOCALCALL WLOG2Shrink(FILE *pF)
{
    static long nShrinkCount = 0;
    static long long nRemovedLines = 0;

    const long nThreshold = 5000000;
    const long nKeepSize = nThreshold / 2;

    if (!pF || ftell(pF) < nThreshold) {
        return;
    }

    {
        char cLine[2048];
        long nSize = ftell(pF);
        long nStart = nSize - nKeepSize;
        FILE *pfTmp = tmpfile();

        if (!pfTmp) {
            fprintf(pF, "...Log file shrink failed, couldn't create tmp file\n\n");
            return;
        }

        rewind(pF);

        while (!feof(pF) && ftell(pF) < nSize) {

            fgets(cLine, 2048, pF);

            if (ftell(pF) < nStart) {
                ++nRemovedLines;
            }
            else {
                fputs(cLine, pfTmp);
            }
        }

        rewind(pF);

        while (ftell(pF) < nSize) {
            fputs("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n", pF);
        }

        rewind(pF);
        rewind(pfTmp);

        ++nShrinkCount;

        fprintf(pF, "...Total removed lines = %I64d, shrink count = %ld\n\n", nRemovedLines, nShrinkCount);

        while (!feof(pfTmp)) {
            fgets(cLine, 2048, pfTmp);
            fputs(cLine, pF);
        }

        fprintf(pF, "\n\n...Start point after shrink, shrink count = %ld\n\n", nShrinkCount);

        fflush(pF);

        fclose(pfTmp);
    }
}
#else /* ET9_DEBUGLOG2 */
#define WLOG2Shrink(pF)
#endif /* ET9_DEBUGLOG2 */

#ifdef ET9_DEBUGLOG2
/*---------------------------------------------------------------------------*/
/** \internal
 * Log UDB content.
 *
 * @param pLingInfo                 Pointer to alphabetic information structure.
 * @param pF                        File to log to.
 *
 * @return None
 */

static void ET9LOCALCALL __LogUDB(ET9AWLingInfo  * const pLingInfo,
                                  FILE                  *pF)
{
#if 1

    ET9AWRUDBInfo ET9FARDATA * const pRUDB =  pLingInfo->pLingCmnInfo->pRUDBInfo;

    ET9U8 ET9FARDATA *pRUDBCurrent;
    ET9U8 ET9FARDATA *pRUDBEnd;

    if (!pRUDB) {
        fprintf(pF, "\nNo UDB\n\n");
        return;
    }

    fprintf(pF, "\nUDB, size %u, remaining %u, RDB words %u, UDB words %u\n\n", pRUDB->wDataSize, pRUDB->wRemainingMemory, pRUDB->wRDBWordCount, pRUDB->wUDBWordCount);

    pRUDBCurrent = ET9RUDBData(pRUDB) + pRUDB->wSizeOffset[0];
    pRUDBEnd     = pRUDBCurrent;
    pRUDBCurrent = _ET9AWMoveRUDBPtrRight(pRUDB, pRUDBCurrent, 1);

    while (pRUDBCurrent != pRUDBEnd) {

        const ET9UINT nCurRecType = _ET9AWGetRecordType(pRUDBCurrent);

        if (nCurRecType == ET9FREETYPE) {

            pRUDBCurrent = _ET9AWMoveRUDBPtrRight(pRUDB, pRUDBCurrent, _ET9AWGetRecordLength(pRUDB, pRUDBCurrent));

            if (pRUDBCurrent == pRUDBEnd) {
                break;
            }
        }
        else {

            ET9UINT i;
            ET9U16  wWordSize;
            ET9U8   bLang;
            ET9U16  wFreq;
            ET9U16  wSaveFreq;

            fprintf(pF, "  ");

            if (nCurRecType == ET9UDBTYPE) {

                bLang = 0;
                wWordSize = _ET9AWGetUDBWordLen(pRUDBCurrent);
                wFreq  = (ET9U16)_ET9AWRUDBReadWord(pRUDB, _ET9AWMoveRUDBPtrRight(pRUDB, pRUDBCurrent, (ET9UINT)1));

                wSaveFreq = wFreq;

                pRUDBCurrent = _ET9AWMoveRUDBPtrRight(pRUDB, pRUDBCurrent, UDB_RECORD_HEADER_SIZE);

                if (wFreq > ET9MAX_FREQ_COUNT) {
                    fprintf(pF, "QUDB ");
                }
                else {
                    fprintf(pF, "UDB  ");
                    wFreq = 0;
                }
            }
            else {

                wFreq = 0;
                bLang = (ET9U8)*(_ET9AWMoveRUDBPtrRight(pRUDB, pRUDBCurrent, (RDB_RECORD_HEADER_SIZE - 1)));
                wWordSize = _ET9AWGetRDBWordLen(pRUDBCurrent);
                wSaveFreq  = (ET9U16)_ET9AWRUDBReadWord(pRUDB, _ET9AWMoveRUDBPtrRight(pRUDB, pRUDBCurrent, (ET9UINT)1));

                pRUDBCurrent = _ET9AWMoveRUDBPtrRight(pRUDB, pRUDBCurrent, RDB_RECORD_HEADER_SIZE);

                fprintf(pF, "RDB  ");
            }

            if (bLang) {
                fprintf(pF, " 0x%02X  %5d  ", bLang, wSaveFreq);
            }
            else if (wFreq) {
                fprintf(pF, "   %2u  %5d  ", (wFreq - ET9MAX_FREQ_COUNT), wSaveFreq);
            }
            else {
                fprintf(pF, "       %5d  ", wSaveFreq);
            }

            {
                ET9UINT nLenUsed = 0;

                for (i = 0; i < wWordSize; ++i) {

                    const ET9SYMB sSymb = (ET9SYMB)_ET9AWRUDBReadWord(pRUDB, (const ET9U8 ET9FARDATA *)pRUDBCurrent);
                    pRUDBCurrent = _ET9AWMoveRUDBPtrRight(pRUDB, (const ET9U8 ET9FARDATA *)pRUDBCurrent, ET9SYMBOLWIDTH);

                    if (sSymb >= 0x20 && sSymb <= 0xFF) {
                        nLenUsed += fprintf(pF, "%c", (unsigned char)sSymb);
                    }
                    else {
                        nLenUsed += fprintf(pF, "<%x>", (int)sSymb);
                    }
                }
            }

            fprintf(pF, "\n");
        }
    }

    fprintf(pF, "\n");

    fflush(pF);

#else
    ET9U16  wId = 0;
    ET9U16  wIndex;
    ET9U16  wCount;

    ET9U16  wBufLen;
    ET9SYMB psBuf[ET9MAXUDBWORDSIZE];

    wBufLen = 0;

    if (ET9AWUDBGetWordCount(pLingInfo, &wCount)) {
        fprintf(pF, "no UDB active\n");
       return;
    }

    fprintf(pF, "%d UDB entries\n", wCount);

    while (!ET9AWUDBGetWord(pLingInfo, psBuf, ET9MAXWORDSIZE, &wBufLen, 1)) {

        fprintf(pF, "UDB %03d: ", wId++);

        for (wIndex = 0; wIndex < wBufLen; ++wIndex) {

            ET9SYMB sSymb = psBuf[wIndex];

            if (sSymb >= 0x20 && sSymb <= 0x7F) {
                fprintf(pF, "%c", (char)sSymb);
            }
            else {
                fprintf(pF, "<%x>", (int)sSymb);
            }
        }

        fprintf(pF, "\n");
    }

    fprintf(pF, "\n");

    fflush(pF);
#endif
}
#else /* ET9_DEBUGLOG2 */
#define __LogUDB(pLingInfo,pF)
#endif /* ET9_DEBUGLOG2 */

#ifdef ET9_DEBUGLOG2
/*---------------------------------------------------------------------------*/
/** \internal
 * Log CDB content.
 *
 * @param pLingInfo                 Pointer to alphabetic information structure.
 * @param pF                        File to log to.
 *
 * @return None
 */

static void ET9LOCALCALL __LogCDB(ET9AWLingInfo  * const pLingInfo,
                                  FILE                  *pF)
{
    ET9AWLingCmnInfo * const pLingCmnInfo = pLingInfo->pLingCmnInfo;
    ET9AWCDBInfo ET9FARDATA * const pCDB =  pLingCmnInfo->pCDBInfo;

    ET9UINT nDelimiterCount;
    ET9UINT nPairReported;

    ET9INT snSearchPosition;
    ET9INT snLastSearchPosition;
    ET9INT snStopPoint;

    ET9SYMB psCDBWord1Buf[ET9MAXWORDSIZE + 1];
    ET9SYMB psCDBWord2Buf[ET9MAXWORDSIZE + 1];

    if (!pCDB) {
        fprintf(pF, "\nNo CDB\n\n");
        return;
    }

    fprintf(pF, "\nCDB, size %u\n\n", pCDB->wDataSize);

    fprintf(pF, "  Current Context Word:  ");

    if (!pLingCmnInfo->Private.bContextWordSize) {
        fprintf(pF, "[NONE]");
    }
    else {
        WLOG2String(pF, "", pLingCmnInfo->Private.sContextWord, pLingCmnInfo->Private.bContextWordSize, 0, 1);
    }

    fprintf(pF, "\n");

    fprintf(pF, "  Previous Context Word: ");

    if (!pLingCmnInfo->Private.bPreviousContextWordSize) {
        fprintf(pF, "[NONE]");
    }
    else {
        WLOG2String(pF, "", pLingCmnInfo->Private.sPreviousContextWord, pLingCmnInfo->Private.bPreviousContextWordSize, 0, 1);
    }

    fprintf(pF, "\n\n");

    snStopPoint = 0;
    snSearchPosition = (ET9INT)(pCDB->wDataEndOffset - (ET9U16)1 + ET9CDBDataAreaSymbs(pCDB)) % ET9CDBDataAreaSymbs(pCDB);

    for (; snSearchPosition != pCDB->wDataEndOffset;) {

        ET9INT snWordOne = 0;
        ET9INT snWordTwo = 0;
        ET9INT snWordOneEnd = 0;
        ET9INT snWordTwoEnd = 0;

        ET9U16 wCDBWord1Size = 0;
        ET9U16 wCDBWord2Size = 0;

        for (;;) {

            if (!snWordTwoEnd) {

                /* if the current symbol is a delimiter, continue */

                if (*(ET9CDBData(pCDB) + snSearchPosition) != CDBDELIMITER) {

                    snWordTwoEnd = snSearchPosition + 1;
                    psCDBWord2Buf[wCDBWord2Size++] = *(ET9CDBData(pCDB) + snSearchPosition);
                    nDelimiterCount = 0;
                }
                else {
                    ++nDelimiterCount;
                }
            }
            else if (!snWordTwo) {

                /* if the current symbol is a delimiter, continue */

                if (*(ET9CDBData(pCDB) + snSearchPosition) == CDBDELIMITER) {
                    snWordTwo = snLastSearchPosition + 1;
                    ++nDelimiterCount;
                }
                else {
                    psCDBWord2Buf[wCDBWord2Size++] = *(ET9CDBData(pCDB) + snSearchPosition);
                    nDelimiterCount = 0;
                }
            }
            else if (!snWordOneEnd) {

                /* if 2 delimiters in a row, broken context... start over */

                if (*(ET9CDBData(pCDB) + snSearchPosition) == CDBDELIMITER) {

                    snWordTwoEnd = 0;
                    snWordTwo = 0;
                    wCDBWord2Size = 0;
                    ++nDelimiterCount;

                    if (nPairReported && nDelimiterCount == 2) {
                        fprintf(pF, "  <<BREAK>>\n");
                        ++nDelimiterCount;
                        nPairReported = 0;
                        break;
                    }
                }
                else {
                    snWordOneEnd = snSearchPosition + 1;
                    psCDBWord1Buf[wCDBWord1Size++] = *(ET9CDBData(pCDB) + snSearchPosition);
                }
            }
            else if (!snWordOne) {

                /* this should give us a word pair */

                if (*(ET9CDBData(pCDB) + snSearchPosition) == CDBDELIMITER) {

                    ET9UINT nLen;

                    ++nDelimiterCount;
                    snWordOne = snLastSearchPosition + 1;

                    /* PRINT OUT WORD PAIR */

                    fprintf(pF, "  ");

                    nLen = WLOG2String(pF, "", psCDBWord1Buf, wCDBWord1Size, 1, 1);

                    while (nLen < 20) {
                        nLen += fprintf(pF, " ");
                    }

                    fprintf(pF, "  ");

                    WLOG2String(pF, "", psCDBWord2Buf, wCDBWord2Size, 1, 1);

                    fprintf(pF, "\n");

                    snSearchPosition = snWordOneEnd - 1;
                    nPairReported = 1;

                    break;
                }
                else {
                    psCDBWord1Buf[wCDBWord1Size++] = *(ET9CDBData(pCDB) + snSearchPosition);
                    nDelimiterCount = 0;
                }
            }

            snLastSearchPosition = snSearchPosition;

            if (snSearchPosition == pCDB->wDataEndOffset) {
                break;
            }

            if (!snSearchPosition) {
                snStopPoint = (ET9INT)pCDB->wDataEndOffset;
                snSearchPosition = (ET9INT)ET9CDBDataAreaSymbs(pCDB) - 1;
            }
            else {
                --snSearchPosition;
            }
        }
    }

    fprintf(pF, "\n");

    fflush(pF);
}
#else /* ET9_DEBUGLOG2 */
#define __LogCDB(pLingInfo,pF)
#endif /* ET9_DEBUGLOG2 */

#ifdef ET9_DEBUG
/*---------------------------------------------------------------------------*/
/** \internal
 * Verify locked symbols in a word.
 *
 * @param pLingInfo         Pointer to alphabetic information structure.
 * @param pWord             Word to be updated.
 * @param wIndex            Input index/position (non zero if partial).
 * @param wLength           Word length (active input length).
 *
 * @return None
 */

static void ET9LOCALCALL __VerifyLockedSymbs(ET9AWLingInfo          * const pLingInfo,
                                             ET9AWPrivWordInfo      * const pWord,
                                             const ET9U16                   wIndex,
                                             const ET9U16                   wLength)
{
    ET9AWLingCmnInfo * const pLingCmnInfo = pLingInfo->pLingCmnInfo;
    ET9WordSymbInfo  * const pWordSymbInfo = pLingCmnInfo->Base.pWordSymbInfo;

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

    if (wLockPoint && wLockPoint >= wIndex) {

        ET9UINT             nCount;
        ET9SYMB             *psSymb;
        ET9SymbInfo         *pSymbInfo;

        nCount = wLockPoint - wIndex;

        if (nCount > wLength) {
            nCount = wLength;
        }
        if (nCount > pWord->Base.wWordLen) {
            nCount = pWord->Base.wWordLen;
        }

        psSymb = pWord->Base.sWord;
        pSymbInfo = &pWordSymbInfo->SymbsInfo[wIndex];

        for (; nCount; --nCount, ++psSymb, ++pSymbInfo) {

            ET9SYMB sLockedSymb = pSymbInfo->sLockedSymb;
            ET9SYMB sLockedCnvSymb = pSymbInfo->sLockedSymb;

            if (pLingInfo->Private.pConvertSymb != NULL) {
                pLingInfo->Private.pConvertSymb(pLingInfo->Private.pConvertSymbInfo, &sLockedCnvSymb);
            }

            /* this is NOT the final lock verification, could be before convert or after */

            if (*psSymb != sLockedSymb && *psSymb != sLockedCnvSymb) {

                WLOG2Word(pLogFile2, "word verified", pWord);
                WLOG2(
                    fprintf(pLogFile2, "__VerifyLockedSymbs, missmatch index = %d, word symb = ", (psSymb - pWord->Base.sWord));
                    if (*psSymb >= 0x20 && *psSymb <= 0x7F) {
                        fprintf(pLogFile2, "'%c'", *psSymb);
                    }
                    else {
                        fprintf(pLogFile2, "<%x>", *psSymb);
                    }
                    fprintf(pLogFile2, ", locked symb = ");
                    if (sLockedSymb >= 0x20 && sLockedSymb <= 0x7F) {
                        fprintf(pLogFile2, "'%c'", sLockedSymb);
                    }
                    else {
                        fprintf(pLogFile2, "<%x>", sLockedSymb);
                    }
                    fprintf(pLogFile2, ", locked cnv symb = ");
                    if (sLockedCnvSymb >= 0x20 && sLockedCnvSymb <= 0x7F) {
                        fprintf(pLogFile2, "'%c'", sLockedCnvSymb);
                    }
                    else {
                        fprintf(pLogFile2, "<%x>", sLockedCnvSymb);
                    }
                    fprintf(pLogFile2, "\n");
                )
            }
            else {
                ET9AssertLog(*psSymb == sLockedSymb || *psSymb == sLockedCnvSymb);
            }
        }
    }
}
#else
#define __VerifyLockedSymbs(pLingInfo, pWord, wIndex, wLength)
#endif /* ET9_DEBUG */

/*---------------------------------------------------------------------------*/
/** \internal
 * This compares the selection list priority of 2 words, classic style.
 * This is a 3-way compare.
 *
 * @param pLingCmnInfo              Pointer to alphabetic cmn information structure.
 * @param pTestWord                 Test word.
 * @param pSrcWord                  Word to compare against.
 * @param bIntermediateSort         If the sort is intermediate or the final stage.
 *
 * @return '> 0' if testword of higher priority than srcword, 0 if equal, otherwise '< 0'.
 */

static ET9INT ET9LOCALCALL __PriorityCompareClassic(ET9AWLingCmnInfo            * const pLingCmnInfo,
                                                    ET9AWPrivWordInfo     const * const pTestWord,
                                                    ET9AWPrivWordInfo     const * const pSrcWord,
                                                    const ET9BOOL                       bIntermediateSort)
{
    ET9WORDSRC nTestWordSrc;
    ET9WORDSRC nSrcWordSrc;

    ET9AssertLog(pTestWord != NULL);
    ET9AssertLog(pSrcWord != NULL);

    nTestWordSrc = GETCOMPSRC(pTestWord->bWordSrc);
    nSrcWordSrc = GETCOMPSRC(pSrcWord->bWordSrc);

    if (nTestWordSrc == nSrcWordSrc &&
        (nSrcWordSrc == ET9WORDSRC_EXACT || nSrcWordSrc == ET9WORDSRC_EXACTISH)) {

        nTestWordSrc = GETBASESRC(pTestWord->bWordSrc);
        nSrcWordSrc = GETBASESRC(pSrcWord->bWordSrc);
    }

    if (!pTestWord->Base.wWordLen) {
        return -1;
    }
    if (!pSrcWord->Base.wWordLen) {
        return 1;
    }

    if (bIntermediateSort) {

        if (pTestWord->Base.bIsTerm && !pSrcWord->Base.bIsTerm) {
            return 1;
        }

        if (pSrcWord->Base.bIsTerm && !pTestWord->Base.bIsTerm) {
            return -1;
        }
    }
    else {

        if (ISPUNCTSRC(nTestWordSrc) && !pSrcWord->Base.bIsTerm) {
            return 1;
        }
        if (ISPUNCTSRC(nSrcWordSrc) && !pTestWord->Base.bIsTerm) {
            return -1;
        }

        if (pTestWord->Base.bIsTerm && !pSrcWord->Base.bIsTerm) {
            if (!ISGENUINESRC(nTestWordSrc) && ISGENUINESRC(nSrcWordSrc)) {
                return -1;
            }
            else if (pSrcWord->bEditDistSpc + pSrcWord->Base.wWordCompLen < pTestWord->bEditDistSpc) {
                return -1;
            }
            else {
                return 1;
            }
        }

        if (pSrcWord->Base.bIsTerm && !pTestWord->Base.bIsTerm) {
            if (!ISGENUINESRC(nSrcWordSrc) && ISGENUINESRC(nTestWordSrc)) {
                return 1;
            }
            else if (pTestWord->bEditDistSpc + pTestWord->Base.wWordCompLen < pSrcWord->bEditDistSpc) {
                return 1;
            }
            else {
                return -1;
            }
        }
    }

    if (!bIntermediateSort) {

        const ET9U16 wContextLanguage = ET9ACTIVELANGSWITCHENABLED(pLingCmnInfo) ? pLingCmnInfo->Private.wPreviousWordLanguage : pLingCmnInfo->wFirstLdbNum;

        const ET9U16 wTestWordLdbNum = (pTestWord->Base.bLangIndex == ET9AWSECOND_LANGUAGE) ? pLingCmnInfo->wSecondLdbNum : pLingCmnInfo->wFirstLdbNum;
        const ET9U16 wSrcWordLdbNum  = (pSrcWord->Base.bLangIndex == ET9AWSECOND_LANGUAGE) ? pLingCmnInfo->wSecondLdbNum : pLingCmnInfo->wFirstLdbNum;

        if (wContextLanguage &&
                (pTestWord->Base.bLangIndex == ET9AWBOTH_LANGUAGES || wTestWordLdbNum == wContextLanguage) &&
                pSrcWord->Base.bLangIndex != ET9AWBOTH_LANGUAGES &&
                nSrcWordSrc != ET9WORDSRC_EXACT &&
                pSrcWord->Base.bLangIndex &&
                wSrcWordLdbNum != wContextLanguage) {
            return 1;
        }
        if (wContextLanguage &&
                (pSrcWord->Base.bLangIndex == ET9AWBOTH_LANGUAGES || wSrcWordLdbNum == wContextLanguage) &&
                pTestWord->Base.bLangIndex != ET9AWBOTH_LANGUAGES &&
                nTestWordSrc != ET9WORDSRC_EXACT &&
                pTestWord->Base.bLangIndex &&
                wTestWordLdbNum != wContextLanguage) {
            return -1;
        }

        if ((!ET9ACTIVELANGSWITCHENABLED(pLingCmnInfo) || !pLingCmnInfo->Private.wPreviousWordLanguage) &&
                pTestWord->Base.bLangIndex == ET9AWFIRST_LANGUAGE &&
                pSrcWord->Base.bLangIndex != ET9AWFIRST_LANGUAGE &&
                pSrcWord->Base.bLangIndex != ET9AWBOTH_LANGUAGES) {
            return 1;
        }
        if ((!ET9ACTIVELANGSWITCHENABLED(pLingCmnInfo) || !pLingCmnInfo->Private.wPreviousWordLanguage) &&
                pSrcWord->Base.bLangIndex == ET9AWFIRST_LANGUAGE &&
                pTestWord->Base.bLangIndex != ET9AWFIRST_LANGUAGE &&
                pTestWord->Base.bLangIndex != ET9AWBOTH_LANGUAGES) {
            return -1;
        }
    }

    if (nTestWordSrc < nSrcWordSrc) {
        return 1;
    }
    if (nSrcWordSrc < nTestWordSrc) {
        return -1;
    }

    if (pTestWord->bCDBTrigram && !pSrcWord->bCDBTrigram) {
        return 1;
    }
    if (pSrcWord->bCDBTrigram && !pTestWord->bCDBTrigram) {
        return -1;
    }

    if (pTestWord->bEditDistSpc < pSrcWord->bEditDistSpc) {
        return 1;
    }
    if (pSrcWord->bEditDistSpc < pTestWord->bEditDistSpc) {
        return -1;
    }

    if (pTestWord->bEditDistStem < pSrcWord->bEditDistStem && pTestWord->bEditDistSpc && pSrcWord->bEditDistSpc) {
        return 1;
    }
    if (pSrcWord->bEditDistStem < pTestWord->bEditDistStem && pTestWord->bEditDistSpc && pSrcWord->bEditDistSpc) {
        return -1;
    }

    if (pTestWord->xTotFreq > pSrcWord->xTotFreq) {
        return 1;
    }
    if (pSrcWord->xTotFreq > pTestWord->xTotFreq) {
        return -1;
    }

    if (pTestWord->xWordFreq > pSrcWord->xWordFreq) {
        return 1;
    }
    if (pSrcWord->xWordFreq > pTestWord->xWordFreq) {
        return -1;
    }

    if (pTestWord->wTWordFreq + pTestWord->wEWordFreq > pSrcWord->wTWordFreq + pSrcWord->wEWordFreq) {
        return 1;
    }
    if (pSrcWord->wTWordFreq + pSrcWord->wEWordFreq > pTestWord->wTWordFreq + pTestWord->wEWordFreq) {
        return -1;
    }

    if (pTestWord->dwWordIndex < pSrcWord->dwWordIndex) {
        return 1;
    }
    if (pSrcWord->dwWordIndex < pTestWord->dwWordIndex) {
        return -1;
    }

    return 0;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * This compares the selection list priority of 2 words, mixed style.
 * This is a 3-way compare.
 *
 * @param pLingCmnInfo              Pointer to alphabetic cmn information structure.
 * @param pTestWord                 Test word.
 * @param pSrcWord                  Word to compare against.
 * @param bIntermediateSort         If the sort is intermediate or the final stage.
 *
 * @return 1 if testword of higher priority than srcword, 0 otherwise.
 */

static ET9INT ET9LOCALCALL __PriorityCompareMixed(ET9AWLingCmnInfo            * const pLingCmnInfo,
                                                  ET9AWPrivWordInfo     const * const pTestWord,
                                                  ET9AWPrivWordInfo     const * const pSrcWord,
                                                  const ET9BOOL                       bIntermediateSort)
{
    ET9WORDSRC nTestWordSrc;
    ET9WORDSRC nSrcWordSrc;

    ET9AssertLog(pTestWord != NULL);
    ET9AssertLog(pSrcWord != NULL);

    nTestWordSrc = GETCOMPSRC(pTestWord->bWordSrc);
    nSrcWordSrc = GETCOMPSRC(pSrcWord->bWordSrc);

    if (nTestWordSrc == ET9WORDSRC_EXACT && nSrcWordSrc != ET9WORDSRC_EXACT) {
        return 1;
    }
    else if (nSrcWordSrc == ET9WORDSRC_EXACT && nTestWordSrc != ET9WORDSRC_EXACT) {
        return -1;
    }

    if (nTestWordSrc == ET9WORDSRC_EXACTISH && nSrcWordSrc != ET9WORDSRC_EXACTISH) {
        return 1;
    }
    else if (nSrcWordSrc == ET9WORDSRC_EXACTISH && nTestWordSrc != ET9WORDSRC_EXACTISH) {
        return -1;
    }

    if (nTestWordSrc == nSrcWordSrc &&
        (nSrcWordSrc == ET9WORDSRC_EXACT || nSrcWordSrc == ET9WORDSRC_EXACTISH)) {

        nTestWordSrc = GETBASESRC(pTestWord->bWordSrc);
        nSrcWordSrc = GETBASESRC(pSrcWord->bWordSrc);
    }

    if (bIntermediateSort) {

    }
    else {

        if (ISPUNCTSRC(nTestWordSrc) && !pSrcWord->Base.bIsTerm) {
            return 1;
        }
        if (ISPUNCTSRC(nSrcWordSrc) && !pTestWord->Base.bIsTerm) {
            return -1;
        }
    }

    if (!bIntermediateSort) {

        const ET9U16 wContextLanguage = ET9ACTIVELANGSWITCHENABLED(pLingCmnInfo) ? pLingCmnInfo->Private.wPreviousWordLanguage : pLingCmnInfo->wFirstLdbNum;

        const ET9U16 wTestWordLdbNum = (pTestWord->Base.bLangIndex == ET9AWSECOND_LANGUAGE) ? pLingCmnInfo->wSecondLdbNum : pLingCmnInfo->wFirstLdbNum;
        const ET9U16 wSrcWordLdbNum  = (pSrcWord->Base.bLangIndex == ET9AWSECOND_LANGUAGE) ? pLingCmnInfo->wSecondLdbNum : pLingCmnInfo->wFirstLdbNum;

        if (wContextLanguage &&
                (pTestWord->Base.bLangIndex == ET9AWBOTH_LANGUAGES || wTestWordLdbNum == wContextLanguage) &&
                pSrcWord->Base.bLangIndex != ET9AWBOTH_LANGUAGES &&
                nSrcWordSrc != ET9WORDSRC_EXACT &&
                pSrcWord->Base.bLangIndex &&
                wSrcWordLdbNum != wContextLanguage) {
            return 1;
        }
        if (wContextLanguage &&
                (pSrcWord->Base.bLangIndex == ET9AWBOTH_LANGUAGES || wSrcWordLdbNum == wContextLanguage) &&
                pTestWord->Base.bLangIndex != ET9AWBOTH_LANGUAGES &&
                nTestWordSrc != ET9WORDSRC_EXACT &&
                pTestWord->Base.bLangIndex &&
                wTestWordLdbNum != wContextLanguage) {
            return -1;
        }

        if ((!ET9ACTIVELANGSWITCHENABLED(pLingCmnInfo) || !pLingCmnInfo->Private.wPreviousWordLanguage) &&
                pTestWord->Base.bLangIndex == ET9AWFIRST_LANGUAGE &&
                pSrcWord->Base.bLangIndex != ET9AWFIRST_LANGUAGE &&
                pSrcWord->Base.bLangIndex != ET9AWBOTH_LANGUAGES) {
            return 1;
        }
        if ((!ET9ACTIVELANGSWITCHENABLED(pLingCmnInfo) || !pLingCmnInfo->Private.wPreviousWordLanguage) &&
                pSrcWord->Base.bLangIndex == ET9AWFIRST_LANGUAGE &&
                pTestWord->Base.bLangIndex != ET9AWFIRST_LANGUAGE &&
                pTestWord->Base.bLangIndex != ET9AWBOTH_LANGUAGES) {
            return -1;
        }
    }

    {
        const ET9INT snTestBuildable = ISBUILDABLESRC(nTestWordSrc);
        const ET9INT snSrcBuildable = ISBUILDABLESRC(nSrcWordSrc);

        if (snTestBuildable && !snSrcBuildable) {
            return 1;
        }
        if (snSrcBuildable && !snTestBuildable) {
            return -1;
        }

        if (!(snTestBuildable && snSrcBuildable)) {

            if (nTestWordSrc < nSrcWordSrc) {
                return 1;
            }
            if (nSrcWordSrc < nTestWordSrc) {
                return -1;
            }
        }
    }

    if (pTestWord->xTotFreq > pSrcWord->xTotFreq) {
        return 1;
    }
    if (pSrcWord->xTotFreq > pTestWord->xTotFreq) {
        return -1;
    }

    if (pTestWord->xWordFreq > pSrcWord->xWordFreq) {
        return 1;
    }
    if (pSrcWord->xWordFreq > pTestWord->xWordFreq) {
        return -1;
    }

    if (pTestWord->wTWordFreq + pTestWord->wEWordFreq > pSrcWord->wTWordFreq + pSrcWord->wEWordFreq) {
        return 1;
    }
    if (pSrcWord->wTWordFreq + pSrcWord->wEWordFreq > pTestWord->wTWordFreq + pTestWord->wEWordFreq) {
        return -1;
    }

    if (pTestWord->Base.bIsTerm && !pSrcWord->Base.bIsTerm) {
        return 1;
    }
    if (pSrcWord->Base.bIsTerm && !pTestWord->Base.bIsTerm) {
        return -1;
    }

    if (pTestWord->bEditDistFree < pSrcWord->bEditDistFree) {
        return 1;
    }
    if (pSrcWord->bEditDistFree < pTestWord->bEditDistFree) {
        return -1;
    }

    if (pTestWord->dwWordIndex < pSrcWord->dwWordIndex) {
        return 1;
    }
    if (pSrcWord->dwWordIndex < pTestWord->dwWordIndex) {
        return -1;
    }

    if (nTestWordSrc < nSrcWordSrc) {
        return 1;
    }
    if (nSrcWordSrc < nTestWordSrc) {
        return -1;
    }

    return 0;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * This compares the selection list priority of 2 words, completions promoted style.
 * This is a 3-way compare.
 *
 * @param pLingCmnInfo              Pointer to alphabetic cmn information structure.
 * @param pTestWord                 Test word.
 * @param pSrcWord                  Word to compare against.
 * @param bIntermediateSort         If the sort is intermediate or the final stage.
 *
 * @return 1 if testword of higher priority than srcword, 0 otherwise.
 */

static ET9INT ET9LOCALCALL __PriorityCompareCompletionsPromoted(ET9AWLingCmnInfo            * const pLingCmnInfo,
                                                                ET9AWPrivWordInfo     const * const pTestWord,
                                                                ET9AWPrivWordInfo     const * const pSrcWord,
                                                                const ET9BOOL                       bIntermediateSort)
{
    ET9WORDSRC nTestWordSrc;
    ET9WORDSRC nSrcWordSrc;

    ET9AssertLog(pTestWord != NULL);
    ET9AssertLog(pSrcWord != NULL);

    nTestWordSrc = GETCOMPSRC(pTestWord->bWordSrc);
    nSrcWordSrc = GETCOMPSRC(pSrcWord->bWordSrc);

    if (nTestWordSrc == nSrcWordSrc &&
        (nSrcWordSrc == ET9WORDSRC_EXACT || nSrcWordSrc == ET9WORDSRC_EXACTISH)) {

        nTestWordSrc = GETBASESRC(pTestWord->bWordSrc);
        nSrcWordSrc = GETBASESRC(pSrcWord->bWordSrc);
    }

    if (!pTestWord->Base.wWordLen) {
        return -1;
    }
    if (!pSrcWord->Base.wWordLen) {
        return 1;
    }

    {
        const ET9U8 bTstEditDistStem = (ET9U8)(pTestWord->bEditDistStem - ((pTestWord->Base.bIsTerm && pTestWord->bIsTop5 && pTestWord->bEditDistStem == 1 && (pTestWord->bEditDistSpc - pTestWord->bEditDistSpcTrp) == 0) ? 1 : 0));
        const ET9U8 bSrcEditDistStem = (ET9U8)(pSrcWord->bEditDistStem  - ((pSrcWord->Base.bIsTerm  && pSrcWord->bIsTop5  && pSrcWord->bEditDistStem  == 1 && (pSrcWord->bEditDistSpc  - pSrcWord->bEditDistSpcTrp)  == 0) ? 1 : 0));

        if (bTstEditDistStem < bSrcEditDistStem) {
            return 1;
        }
        if (bSrcEditDistStem < bTstEditDistStem) {
            return -1;
        }
    }

    if (bIntermediateSort) {

        if (pTestWord->Base.bIsTerm && !pSrcWord->Base.bIsTerm) {
            return 1;
        }

        if (pSrcWord->Base.bIsTerm && !pTestWord->Base.bIsTerm) {
            return -1;
        }
    }
    else {

        if (ISPUNCTSRC(nTestWordSrc) && !pSrcWord->Base.bIsTerm) {
            return 1;
        }
        if (ISPUNCTSRC(nSrcWordSrc) && !pTestWord->Base.bIsTerm) {
            return -1;
        }

        if (pTestWord->Base.bIsTerm && !pSrcWord->Base.bIsTerm) {
            if (!ISGENUINESRC(nTestWordSrc) && ISGENUINESRC(nSrcWordSrc)) {
                return -1;
            }
            else if (pSrcWord->bEditDistSpc + pSrcWord->Base.wWordCompLen < pTestWord->bEditDistSpc) {
                return -1;
            }
            else {
                return 1;
            }
        }

        if (pSrcWord->Base.bIsTerm && !pTestWord->Base.bIsTerm) {
            if (!ISGENUINESRC(nSrcWordSrc) && ISGENUINESRC(nTestWordSrc)) {
                return 1;
            }
            else if (pTestWord->bEditDistSpc + pTestWord->Base.wWordCompLen < pSrcWord->bEditDistSpc) {
                return 1;
            }
            else {
                return -1;
            }
        }
    }

    /* repeat stem distance check to separate what we merged above */

    if (pTestWord->bEditDistStem < pSrcWord->bEditDistStem) {
        return 1;
    }
    if (pSrcWord->bEditDistStem < pTestWord->bEditDistStem) {
        return -1;
    }

    if (!bIntermediateSort) {

        const ET9U16 wContextLanguage = ET9ACTIVELANGSWITCHENABLED(pLingCmnInfo) ? pLingCmnInfo->Private.wPreviousWordLanguage : pLingCmnInfo->wFirstLdbNum;

        const ET9U16 wTestWordLdbNum = (pTestWord->Base.bLangIndex == ET9AWSECOND_LANGUAGE) ? pLingCmnInfo->wSecondLdbNum : pLingCmnInfo->wFirstLdbNum;
        const ET9U16 wSrcWordLdbNum  = (pSrcWord->Base.bLangIndex == ET9AWSECOND_LANGUAGE) ? pLingCmnInfo->wSecondLdbNum : pLingCmnInfo->wFirstLdbNum;

        if (wContextLanguage &&
                (pTestWord->Base.bLangIndex == ET9AWBOTH_LANGUAGES || wTestWordLdbNum == wContextLanguage) &&
                pSrcWord->Base.bLangIndex != ET9AWBOTH_LANGUAGES &&
                nSrcWordSrc != ET9WORDSRC_EXACT &&
                pSrcWord->Base.bLangIndex &&
                wSrcWordLdbNum != wContextLanguage) {
            return 1;
        }
        if (wContextLanguage &&
                (pSrcWord->Base.bLangIndex == ET9AWBOTH_LANGUAGES || wSrcWordLdbNum == wContextLanguage) &&
                pTestWord->Base.bLangIndex != ET9AWBOTH_LANGUAGES &&
                nTestWordSrc != ET9WORDSRC_EXACT &&
                pTestWord->Base.bLangIndex &&
                wTestWordLdbNum != wContextLanguage) {
            return -1;
        }

        if ((!ET9ACTIVELANGSWITCHENABLED(pLingCmnInfo) || !pLingCmnInfo->Private.wPreviousWordLanguage) &&
                pTestWord->Base.bLangIndex == ET9AWFIRST_LANGUAGE &&
                pSrcWord->Base.bLangIndex != ET9AWFIRST_LANGUAGE &&
                pSrcWord->Base.bLangIndex != ET9AWBOTH_LANGUAGES) {
            return 1;
        }
        if ((!ET9ACTIVELANGSWITCHENABLED(pLingCmnInfo) || !pLingCmnInfo->Private.wPreviousWordLanguage) &&
                pSrcWord->Base.bLangIndex == ET9AWFIRST_LANGUAGE &&
                pTestWord->Base.bLangIndex != ET9AWFIRST_LANGUAGE &&
                pTestWord->Base.bLangIndex != ET9AWBOTH_LANGUAGES) {
            return -1;
        }
    }

    if (nTestWordSrc < nSrcWordSrc) {
        return 1;
    }
    if (nSrcWordSrc < nTestWordSrc) {
        return -1;
    }

    if (pTestWord->bCDBTrigram && !pSrcWord->bCDBTrigram) {
        return 1;
    }
    if (pSrcWord->bCDBTrigram && !pTestWord->bCDBTrigram) {
        return -1;
    }

    {
        const ET9U8 bTstEditDistSpc = (ET9U8)(pTestWord->bEditDistSpc - pTestWord->bEditDistSpcTrp);
        const ET9U8 bSrcEditDistSpc = (ET9U8)(pSrcWord->bEditDistSpc  - pSrcWord->bEditDistSpcTrp);

        if (bTstEditDistSpc < bSrcEditDistSpc) {
            return 1;
        }
        if (bSrcEditDistSpc < bTstEditDistSpc) {
            return -1;
        }
    }

    if (pTestWord->xTotFreq > pSrcWord->xTotFreq) {
        return 1;
    }
    if (pSrcWord->xTotFreq > pTestWord->xTotFreq) {
        return -1;
    }

    if (pTestWord->xWordFreq > pSrcWord->xWordFreq) {
        return 1;
    }
    if (pSrcWord->xWordFreq > pTestWord->xWordFreq) {
        return -1;
    }

    if (pTestWord->wTWordFreq + pTestWord->wEWordFreq > pSrcWord->wTWordFreq + pSrcWord->wEWordFreq) {
        return 1;
    }
    if (pSrcWord->wTWordFreq + pSrcWord->wEWordFreq > pTestWord->wTWordFreq + pTestWord->wEWordFreq) {
        return -1;
    }

    if (pTestWord->dwWordIndex < pSrcWord->dwWordIndex) {
        return 1;
    }
    if (pSrcWord->dwWordIndex < pTestWord->dwWordIndex) {
        return -1;
    }

    return 0;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * This compares the selection list priority of 2 words, style independent.
 * Some style independent priority rules are applied at this level, then the proper style is applied.
 * This is a 3-way compare.
 *
 * @param pLingCmnInfo              Pointer to alphabetic cmn information structure.
 * @param pTestWord                 Test word.
 * @param pSrcWord                  Word to compare against.
 * @param bIntermediateSort         If the sort is intermediate or the final stage.
 *
 * @return '> 0' if testword of higher priority than srcword, 0 if equal, otherwise '< 0'.
 */

static ET9INT ET9LOCALCALL __PriorityCompare(ET9AWLingCmnInfo           * const pLingCmnInfo,
                                             ET9AWPrivWordInfo    const * const pTestWord,
                                             ET9AWPrivWordInfo    const * const pSrcWord,
                                             const ET9BOOL                      bIntermediateSort)
{
    ET9WordSymbInfo * const     pWordSymbInfo = pLingCmnInfo->Base.pWordSymbInfo;
    const ET9U8                 bNumSymbs = pWordSymbInfo->bNumSymbs;

    ET9AssertLog(pLingCmnInfo != NULL);
    ET9AssertLog(pTestWord != NULL);
    ET9AssertLog(pSrcWord != NULL);

    /* rules shared by all prio functions */

    if (bNumSymbs == 1 && pWordSymbInfo->SymbsInfo->bSymbType == ET9KTSMARTPUNCT) {

        if (ISPUNCTSRC(pTestWord->bWordSrc) && !ISPUNCTSRC(pSrcWord->bWordSrc) && !ISEXACTSRC(pSrcWord->bWordSrc)) {
            return 1;
        }
        if (ISPUNCTSRC(pSrcWord->bWordSrc) && !ISPUNCTSRC(pTestWord->bWordSrc) && !ISEXACTSRC(pTestWord->bWordSrc)) {
            return -1;
        }
    }

    /* disposable words are last of all */

    if (!ISDISPOSEWRD(pTestWord) && ISDISPOSEWRD(pSrcWord)) {
        return 1;
    }
    if (!ISDISPOSEWRD(pSrcWord) && ISDISPOSEWRD(pTestWord)) {
        return -1;
    }

    /* protected words should always be last (in a real list, thus before disposable's) */

    if (!ISPROTECTEDSRC(pTestWord->bWordSrc) && ISPROTECTEDSRC(pSrcWord->bWordSrc)) {
        return 1;
    }
    if (!ISPROTECTEDSRC(pSrcWord->bWordSrc) && ISPROTECTEDSRC(pTestWord->bWordSrc)) {
        return -1;
    }

    if (ISPROTECTEDSRC(pTestWord->bWordSrc) && ISPROTECTEDSRC(pSrcWord->bWordSrc)) {

        if (pTestWord->bWordSrc < pSrcWord->bWordSrc) {
            return 1;
        }
        if (pSrcWord->bWordSrc < pTestWord->bWordSrc) {
            return -1;
        }
    }

    /* pool words (stems etc) is lower than all more "real" stuff */

    if (!ISSPOTSRC(pTestWord->bWordSrc) && ISSPOTSRC(pSrcWord->bWordSrc)) {
        return 1;
    }
    if (!ISSPOTSRC(pSrcWord->bWordSrc) && ISSPOTSRC(pTestWord->bWordSrc)) {
        return -1;
    }

    if (ISSPOTSRC(pTestWord->bWordSrc) && ISSPOTSRC(pSrcWord->bWordSrc)) {

        if (pTestWord->bWordSrc < pSrcWord->bWordSrc) {
            return 1;
        }
        if (pSrcWord->bWordSrc < pTestWord->bWordSrc) {
            return -1;
        }

        if (pTestWord->bWordQuality > pSrcWord->bWordQuality) {
            return 1;
        }
        if (pSrcWord->bWordQuality > pTestWord->bWordQuality) {
            return -1;
        }
    }

    /* apply current prio function rules */

    if (pLingCmnInfo->Private.eCurrSelectionListMode == ET9ASLMODE_CLASSIC) {

        return __PriorityCompareClassic(pLingCmnInfo, pTestWord, pSrcWord, bIntermediateSort);
    }
    else if (pLingCmnInfo->Private.eCurrSelectionListMode == ET9ASLMODE_COMPLETIONSPROMOTED) {

        return __PriorityCompareCompletionsPromoted(pLingCmnInfo, pTestWord, pSrcWord, bIntermediateSort);
    }
    else {

        ET9AssertLog(pLingCmnInfo->Private.eCurrSelectionListMode == ET9ASLMODE_MIXED);

        return __PriorityCompareMixed(pLingCmnInfo, pTestWord, pSrcWord, bIntermediateSort);
    }
}

/*---------------------------------------------------------------------------*/
/** \internal
 * This function checks to see if word is all discrete input.
 *
 * @param pLingInfo                 Pointer to alphabetic information structure.
 *
 * @return Non zero if it is, otherwise zero.
 */

static ET9UINT ET9LOCALCALL __ET9AWIsDiscreteInput(ET9AWLingInfo * const pLingInfo)
{
    ET9U16  wLen;
    ET9SymbInfo  *pSymbInfo;

    ET9AssertLog(pLingInfo != NULL);

    wLen = pLingInfo->pLingCmnInfo->Base.pWordSymbInfo->bNumSymbs;
    pSymbInfo = pLingInfo->pLingCmnInfo->Base.pWordSymbInfo->SymbsInfo;

    for (; wLen ; --wLen, ++pSymbInfo) {

        if (pSymbInfo->eInputType != ET9DISCRETEKEY) {
            return 0;
        }
    }

    return 1;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * This function checks to see if a word is similar to exact match.
 *
 * @param pLingInfo                 Pointer to alphabetic information structure.
 * @param pWord                     Word to check.
 *
 * @return Non zero if is like exact, otherwise zero.
 */

static ET9UINT ET9LOCALCALL __ET9AWIsLikeExactMatch(ET9AWLingInfo       * const pLingInfo,
                                                    ET9AWPrivWordInfo   * const pWord)
{
    ET9WordSymbInfo     * const pWordSymbInfo = pLingInfo->pLingCmnInfo->Base.pWordSymbInfo;
    const ET9U8                 bNumSymbs = pWordSymbInfo->bNumSymbs;

    ET9U8 bFreeCount;

    ET9AssertLog(pLingInfo != NULL);
    ET9AssertLog(pWord != NULL);

    /* count free symbs in word */

    {
        ET9SYMB *pSymb;
        ET9UINT nCount;

        bFreeCount = 0;

        pSymb = pWord->Base.sWord;
        for (nCount = pWord->Base.wWordLen; nCount; --nCount, ++pSymb) {
            if (_ET9_LanguageSpecific_FreeExactishSymb(*pSymb)) {
                ++bFreeCount;
            }
        }
    }

    /* keep cases separate for clarity */

    if (!bNumSymbs) {
        WLOG2BWord(pLogFile2, "..IsLikeExact (0) - empty input", pWord);
        return 0;
    }

    if (pWord->Base.wWordLen != bNumSymbs && pWord->Base.wWordLen != bNumSymbs + bFreeCount) {
        WLOG2BWord(pLogFile2, "..IsLikeExact (0) - has length difference", pWord);
        return 0;
    }

    if (pWordSymbInfo->SymbsInfo[bNumSymbs - 1].bSymbType == ET9KTSMARTPUNCT) {
        WLOG2BWord(pLogFile2, "..IsLikeExact (0) - last input is terminal punctuation", pWord);
        return 0;
    }

    if (pWord->bHasPrimEditDist) {
        WLOG2BWord(pLogFile2, "..IsLikeExact (0) - has prim edit distance", pWord);
        return 0;
    }

    if (!pWord->Base.bIsTerm ||
        ISPUNCTSRC(pWord->bWordSrc) ||
        ISCOMPOUNDSRC(pWord->bWordSrc) ||
        ISBUILDAROUNDSRC(pWord->bWordSrc) ||
        !ISREALSRC(pWord->bWordSrc)) {

        WLOG2BWord(pLogFile2, "..IsLikeExact (0) - stem, punct, compound, buildaround or 'not real'", pWord);
        return 0;
    }

    if (_ET9_IsNumericString(pWord->Base.sWord, pWord->Base.wWordLen)) {
        WLOG2BWord(pLogFile2, "..IsLikeExact (0) - is numeric string", pWord);
        return 0;
    }

    if (__ET9AWIsDiscreteInput(pLingInfo)) {
        WLOG2BWord(pLogFile2, "..IsLikeExact (1) - is discrete input", pWord);
        return 1;
    }

    {
        ET9UINT     nCount;
        ET9SYMB     *psSymb;
        ET9SymbInfo *pSymbInfo;
        ET9U16      wLdbNum;
        ET9UINT     nRemovableFrees;

        nRemovableFrees = __ET9Min(bFreeCount, pWord->Base.wWordLen - bNumSymbs);

        psSymb    = pWord->Base.sWord;
        pSymbInfo = pWordSymbInfo->SymbsInfo;
        wLdbNum   = (pWord->Base.bLangIndex == ET9AWSECOND_LANGUAGE) ? pLingInfo->pLingCmnInfo->wSecondLdbNum : pLingInfo->pLingCmnInfo->wFirstLdbNum;

        for (nCount = pWord->Base.wWordLen; nCount ; --nCount, ++psSymb) {

            if (pSymbInfo->eInputType == ET9DISCRETEKEY ||
                pSymbInfo->eInputType == ET9MULTITAPKEY ||
                pSymbInfo->eInputType == ET9CUSTOMSET) {

                ++pSymbInfo;

                continue;
            }

            {
                ET9UINT     nSymCount;
                ET9UINT     nFound;
                ET9SYMB     sLower;
                ET9SYMB     sUpper;
                ET9SYMB     *psLowerAlts;
                ET9SYMB     *psUpperAlts;

                nFound = 0;
                sUpper = _ET9SymToUpper(*psSymb, wLdbNum);
                sLower = _ET9SymToLower(*psSymb, wLdbNum);
                psLowerAlts = pSymbInfo->DataPerBaseSym->sChar;
                psUpperAlts = pSymbInfo->DataPerBaseSym->sUpperCaseChar;

                for (nSymCount = pSymbInfo->DataPerBaseSym->bNumSymsToMatch; nSymCount; --nSymCount, ++psLowerAlts, ++psUpperAlts) {

                    if (sUpper == *psUpperAlts || sLower == *psLowerAlts) {
                        nFound = 1;
                        break;
                    }
                }

                if (!nFound) {

                    if (nRemovableFrees && _ET9_LanguageSpecific_FreeExactishSymb(*psSymb)) {
                        --nRemovableFrees;
                        WLOG2B(fprintf(pLogFile2, "..IsLikeExact, consumed removable apostrophe, pos = %d\n", (pSymbInfo - pWordSymbInfo->SymbsInfo));)
                    }
                    else {
                        WLOG2B(fprintf(pLogFile2, "..IsLikeExact, fail pos = %d\n", (pSymbInfo - pWordSymbInfo->SymbsInfo));)
                        WLOG2BWord(pLogFile2, "..IsLikeExact (0) - symbol not found", pWord);
                        return 0;
                    }
                }
                else if (nCount > nRemovableFrees) {
                    ++pSymbInfo;
                }
            }
        }
    }

    WLOG2BWord(pLogFile2, "..IsLikeExact (1) - ok", pWord);

    return 1;
}

#ifdef ET9_DEBUG

/*---------------------------------------------------------------------------*/
/** \internal
 * Verify word counters.
 *
 * @param pLingCmnInfo              Pointer to alphabetic cmn information structure.
 *
 * @return None
 */

static void ET9LOCALCALL __VerifyWordCounters(ET9AWLingCmnInfo * const pLingCmnInfo)
{
    if (!pLingCmnInfo->Private.bSpcComplDuringSingleBuild) {
        ET9AssertLog(pLingCmnInfo->Private.bTotalSpcTermWords <= pLingCmnInfo->Private.ASpc.wMaxSpcTermCount);
        ET9AssertLog(pLingCmnInfo->Private.bTotalSpcCmplWords <= pLingCmnInfo->Private.ASpc.wMaxSpcCmplCount);
        ET9AssertLog(pLingCmnInfo->Private.bTotalCompletionWords <= pLingCmnInfo->Private.wMaxCompletionCount);
    }

    {
        ET9U8 bIndex;
        ET9U8 bSpcTermWords;
        ET9U8 bSpcCmplWords;
        ET9U8 bCompletionWords;
        ET9AWPrivWordInfo *pWord;

        bSpcTermWords = 0;
        bSpcCmplWords = 0;
        bCompletionWords = 0;

        for (bIndex = 0; bIndex < pLingCmnInfo->Private.bTotalWords; ++bIndex) {

            pWord = &pLingCmnInfo->Private.pWordList[bIndex];

            if (pWord->bHasPrimEditDist && pWord->Base.wWordCompLen) {
                ++bSpcCmplWords;
            }
            else if (pWord->Base.wWordCompLen) {
                ++bCompletionWords;
            }
            else if (pWord->bHasPrimEditDist) {
                ++bSpcTermWords;
            }
        }

        if (!pLingCmnInfo->Private.bSpcComplDuringSingleBuild) {
            ET9AssertLog(bSpcTermWords == pLingCmnInfo->Private.bTotalSpcTermWords);
            ET9AssertLog(bSpcCmplWords == pLingCmnInfo->Private.bTotalSpcCmplWords);
            ET9AssertLog(bCompletionWords == pLingCmnInfo->Private.bTotalCompletionWords);
        }
    }
}

#else

#define __VerifyWordCounters(pLingCmnInfo)

#endif

/*---------------------------------------------------------------------------*/
/** \internal
 * Handles insert counter update.
 *
 * @param pLingCmnInfo              Pointer to alphabetic cmn information structure.
 * @param pOldWord                  The word that will be removed.
 * @param pNewWord                  The word that will replace the removed word.
 *
 * @return None
 */

ET9INLINE static void ET9LOCALCALL __ET9HandleInsertCounters(ET9AWLingCmnInfo        * const pLingCmnInfo,
                                                             ET9AWPrivWordInfo       * const pOldWord,
                                                             ET9AWPrivWordInfo       * const pNewWord)
{
    const ET9BOOL bSpcComplDuringSingleBuild = pLingCmnInfo->Private.bSpcComplDuringSingleBuild;

    /* validation*/

    if (!bSpcComplDuringSingleBuild) {
        ET9AssertLog(pLingCmnInfo->Private.bTotalSpcTermWords <= pLingCmnInfo->Private.ASpc.wMaxSpcTermCount);
        ET9AssertLog(pLingCmnInfo->Private.bTotalSpcCmplWords <= pLingCmnInfo->Private.ASpc.wMaxSpcCmplCount);
        ET9AssertLog(pLingCmnInfo->Private.bTotalCompletionWords <= pLingCmnInfo->Private.wMaxCompletionCount);
    }

    /* log */

    WLOG2BWord(pLogFile2, "__ET9HandleInsertCounters, pOldWordIn", pOldWord);
    WLOG2BWord(pLogFile2, "__ET9HandleInsertCounters, pNewWordIn", pNewWord);
    WLOG2B(fprintf(pLogFile2, "__ET9HandleInsertCounters, compl count = %d (before)\n", pLingCmnInfo->Private.bTotalCompletionWords);)

    /* update info */

    if (pOldWord->Base.wWordCompLen && pOldWord->bHasPrimEditDist) {
        --pLingCmnInfo->Private.bTotalSpcCmplWords;
    }
    else if (pOldWord->Base.wWordCompLen) {
        --pLingCmnInfo->Private.bTotalCompletionWords;
    }
    else if (pOldWord->bHasPrimEditDist) {
        --pLingCmnInfo->Private.bTotalSpcTermWords;
    }

    if (pNewWord->Base.wWordCompLen && pNewWord->bHasPrimEditDist) {
        ++pLingCmnInfo->Private.bTotalSpcCmplWords;
    }
    else if (pNewWord->Base.wWordCompLen) {
        ++pLingCmnInfo->Private.bTotalCompletionWords;
    }
    else if (pNewWord->bHasPrimEditDist) {
        ++pLingCmnInfo->Private.bTotalSpcTermWords;
    }

    WLOG2B(fprintf(pLogFile2, "__ET9HandleInsertCounters, compl count = %d (after)\n", pLingCmnInfo->Private.bTotalCompletionWords);)

    /* validation*/

    if (!bSpcComplDuringSingleBuild) {
        ET9AssertLog(pLingCmnInfo->Private.bTotalSpcTermWords <= pLingCmnInfo->Private.ASpc.wMaxSpcTermCount);
        ET9AssertLog(pLingCmnInfo->Private.bTotalSpcCmplWords <= pLingCmnInfo->Private.ASpc.wMaxSpcCmplCount);
        ET9AssertLog(pLingCmnInfo->Private.bTotalCompletionWords <= pLingCmnInfo->Private.wMaxCompletionCount);
    }

    /* update last pointers */

    if (pLingCmnInfo->Private.pLastWord == pOldWord) {
        pLingCmnInfo->Private.pLastWord = NULL;
    }
    if (pLingCmnInfo->Private.pLastSpcTermWord == pOldWord) {
        pLingCmnInfo->Private.pLastSpcTermWord = NULL;
    }
    if (pLingCmnInfo->Private.pLastSpcCmplWord == pOldWord) {
        pLingCmnInfo->Private.pLastSpcCmplWord = NULL;
    }
    if (pLingCmnInfo->Private.pLastCompletionWord == pOldWord) {
        pLingCmnInfo->Private.pLastCompletionWord = NULL;
    }
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Update insert counters - not actually replacing the word itself.
 *
 * @param pLingCmnInfo              Pointer to alphabetic cmn information structure.
 * @param pOldWord                  The word that will be removed.
 * @param pNewWord                  The word that will replace the removed word.
 *
 * @return None
 */

ET9INLINE static void ET9LOCALCALL __ET9UpdateInsertCounters(ET9AWLingCmnInfo        * const pLingCmnInfo,
                                                             ET9AWPrivWordInfo       * const pOldWord,
                                                             ET9AWPrivWordInfo       * const pNewWord)
{
    __ET9HandleInsertCounters(pLingCmnInfo, pOldWord, pNewWord);
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Update insert counters - and replace the word itself.
 *
 * @param pLingCmnInfo              Pointer to alphabetic cmn information structure.
 * @param pOldWord                  The word that will be removed.
 * @param pNewWord                  The word that will replace the removed word.
 *
 * @return None
 */

ET9INLINE static void ET9LOCALCALL __ET9ReplaceAndUpdateInsertCounters(ET9AWLingCmnInfo        * const pLingCmnInfo,
                                                                       ET9AWPrivWordInfo       * const pOldWord,
                                                                       ET9AWPrivWordInfo       * const pNewWord)
{
    __ET9HandleInsertCounters(pLingCmnInfo, pOldWord, pNewWord);

    *pOldWord = *pNewWord;

    pLingCmnInfo->Private.snLinSearchCount = 0;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Compare two words based on length and symbol value (from pointers).
 *
 * @param pLingCmnInfo              Pointer to alphabetic cmn information structure.
 * @param pw1                       Word pointer 1 (direct).
 * @param pw2                       Word pointer 2 (direct).
 *
 * @return '> 0' if testword of higher priority than srcword, 0 if equal, otherwise '< 0'.
 */

ET9INLINE static ET9INT ET9LOCALCALL __LenSymbCmpWP(ET9AWLingCmnInfo     * const pLingCmnInfo,
                                                    ET9AWPrivWordInfo    * const pw1,
                                                    ET9AWPrivWordInfo    * const pw2)
{
    ET9INT snResult;

    ET9_UNUSED(pLingCmnInfo);

    snResult = pw1->Base.wWordLen - pw2->Base.wWordLen;

    if (!snResult) {

        ET9INT  snCount = pw1->Base.wWordLen;
        ET9SYMB *psStr1 = pw1->Base.sWord;
        ET9SYMB *psStr2 = pw2->Base.sWord;

        for (; snCount; --snCount, ++psStr1, ++psStr2) {

            if (*psStr1 != *psStr2) {
                snResult = (*psStr1 < *psStr2) ? -1 : 1;
                break;
            }
        }
    }

    if (!snResult) {

        const ET9U8 bDupe1 = (pw1->bWordQuality == DUPE_QUALITY) ? 1 : 0;
        const ET9U8 bDupe2 = (pw2->bWordQuality == DUPE_QUALITY) ? 1 : 0;

        snResult = bDupe1 - bDupe2;
    }

    return snResult;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Compare two words based on length and symbol value (from index).
 *
 * @param pLingCmnInfo              Pointer to alphabetic cmn information structure.
 * @param snIndex1                  Word index 1 (direct word array).
 * @param snIndex2                  Word index 2 (direct word array).
 *
 * @return '> 0' if testword of higher priority than srcword, 0 if equal, otherwise '< 0'.
 */

ET9INLINE static ET9INT ET9LOCALCALL __LenSymbCmp(ET9AWLingCmnInfo     * const pLingCmnInfo,
                                                  const ET9INT                 snIndex1,
                                                  const ET9INT                 snIndex2)
{
    ET9AWPrivWordInfo   * const pWordList = pLingCmnInfo->Private.pWordList;

    ET9AWPrivWordInfo   * const pWord1 = &pWordList[snIndex1];
    ET9AWPrivWordInfo   * const pWord2 = &pWordList[snIndex2];

    ET9AssertLog(snIndex1 >= 0 && snIndex1 < pLingCmnInfo->Private.bTotalWords);
    ET9AssertLog(snIndex2 >= 0 && snIndex2 < pLingCmnInfo->Private.bTotalWords);

    return __LenSymbCmpWP(pLingCmnInfo, pWord1, pWord2);
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Compare two words based on priority value.
 *
 * @param pLingCmnInfo              Pointer to alphabetic cmn information structure.
 * @param snIndex1                  Word index 1 (direct word array).
 * @param snIndex2                  Word index 2 (direct word array).
 *
 * @return '> 0' if testword of higher priority than srcword, 0 if equal, otherwise '< 0'.
 */

ET9INLINE static ET9INT ET9LOCALCALL __PriorityCmp(ET9AWLingCmnInfo     * const pLingCmnInfo,
                                                   const ET9INT                 snIndex1,
                                                   const ET9INT                 snIndex2)
{
    ET9AWPrivWordInfo   * const pWordList = pLingCmnInfo->Private.pWordList;

    ET9AWPrivWordInfo   * const pw1 = &pWordList[snIndex1];
    ET9AWPrivWordInfo   * const pw2 = &pWordList[snIndex2];

    ET9INT snResult;

    ET9AssertLog(snIndex1 >= 0 && snIndex1 < pLingCmnInfo->Private.bTotalWords);
    ET9AssertLog(snIndex2 >= 0 && snIndex2 < pLingCmnInfo->Private.bTotalWords);

    snResult = __PriorityCompare(pLingCmnInfo, pw1, pw2, 1);

    if (snResult == 0) {
        snResult = (pw1 < pw2) ? +1 : ((pw1 > pw2) ? -1 : 0); /* arrival order */
    }

    return snResult;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Swap two words.
 *
 * @param pLingCmnInfo              Pointer to alphabetic cmn information structure.
 * @param snIndex1                  Word index 1 (sort array).
 * @param snIndex2                  Word index 2 (sort array).
 *
 * @return None
 */

ET9INLINE static void ET9LOCALCALL __ValueSwap(ET9AWLingCmnInfo   * const pLingCmnInfo,
                                               const ET9INT               snIndex1,
                                               const ET9INT               snIndex2)
{
    ET9U8 * const pbWordList = pLingCmnInfo->Private.bWordList;

    ET9U8 * const pb1 = &pbWordList[snIndex1];
    ET9U8 * const pb2 = &pbWordList[snIndex2];

    const ET9U8 bTmp = *pb1;

    *pb1 = *pb2;
    *pb2 = bTmp;

    ET9AssertLog(*pb1 >= 0 && *pb1 < pLingCmnInfo->Private.bTotalWords);
    ET9AssertLog(*pb2 >= 0 && *pb2 < pLingCmnInfo->Private.bTotalWords);

    ET9AssertLog(snIndex1 >= 0 && snIndex1 < pLingCmnInfo->Private.bTotalWords);
    ET9AssertLog(snIndex2 >= 0 && snIndex2 < pLingCmnInfo->Private.bTotalWords);
}

/*---------------------------------------------------------------------------*/
/** \internal
 * This function sorts the selection list in length/symbol order (partial).
 * Using quick sort.
 *
 * @param pLingCmnInfo              Pointer to alphabetic cmn information structure.
 * @param snLo                      Low index.
 * @param snHi                      High index.
 *
 * @return None
 */

static void ET9LOCALCALL __ET9AWSortSearchListQS(ET9AWLingCmnInfo   * const pLingCmnInfo,
                                                 const ET9INT               snLo,
                                                 const ET9INT               snHi)
{
    ET9U8 * const pbWordList = pLingCmnInfo->Private.bWordList;

    if (snHi > snLo) {

        const ET9INT snMidItem = pbWordList[(snLo + snHi) / 2];

        ET9INT snL = snLo;
        ET9INT snH = snHi;

        while (snL <= snH) {

            while (snL < snHi && __LenSymbCmp(pLingCmnInfo, pbWordList[snL], snMidItem) < 0) {
                ++snL;
            }

            while (snH > snLo && __LenSymbCmp(pLingCmnInfo, pbWordList[snH], snMidItem) > 0) {
                --snH;
            }

            if (snL <= snH) {
                __ValueSwap(pLingCmnInfo, snL, snH);
                ++snL;
                --snH;
            }
        }

        __ET9AWSortSearchListQS(pLingCmnInfo, snLo, snH);
        __ET9AWSortSearchListQS(pLingCmnInfo, snL, snHi);
    }
}

/*---------------------------------------------------------------------------*/
/** \internal
 * This function sorts the selection list in length/symbol order.
 *
 * @param pLingCmnInfo              Pointer to alphabetic cmn information structure.
 *
 * @return None
 */

ET9INLINE static void ET9LOCALCALL __ET9AWSortSearchList(ET9AWLingCmnInfo * const pLingCmnInfo)                                                           \
{
    __ET9AWSortSearchListQS(pLingCmnInfo, 0, pLingCmnInfo->Private.bTotalWords - 1);

    pLingCmnInfo->Private.snLinSearchCount = DUPE_BIN_SEARCH_POINT + 1;

#ifdef ET9_DEBUG
    {
        ET9U8 * const pbWordList = pLingCmnInfo->Private.bWordList;

        ET9U8 bIndex;
        ET9U8 bLook;

        for (bIndex = 0; bIndex + 1 < pLingCmnInfo->Private.bTotalWords; ++bIndex) {
            for (bLook = bIndex + 1; bLook < pLingCmnInfo->Private.bTotalWords; ++bLook) {
                ET9AssertLog(__LenSymbCmp(pLingCmnInfo, pbWordList[bIndex], pbWordList[bLook]) <= 0);
            }
        }
    }
#endif
}

/*---------------------------------------------------------------------------*/
/** \internal
 * This function sorts the selection list in priority order (partial).
 * Using quick sort.
 *
 * @param pLingCmnInfo              Pointer to alphabetic cmn information structure.
 * @param snLo                      Low index.
 * @param snHi                      High index.
 *
 * @return None
 */

static void ET9LOCALCALL __ET9AWSortPriorityListQS(ET9AWLingCmnInfo   * const pLingCmnInfo,
                                                   const ET9INT               snLo,
                                                   const ET9INT               snHi)
{
    ET9U8 * const pbWordList = pLingCmnInfo->Private.bWordList;

    if (snHi > snLo) {

        const ET9INT snMidItem = pbWordList[(snLo + snHi) / 2];

        ET9INT snL = snLo;
        ET9INT snH = snHi;

        while (snL <= snH) {

            while (snL < snHi && __PriorityCmp(pLingCmnInfo, pbWordList[snL], snMidItem) > 0) {
                ++snL;
            }

            while (snH > snLo && __PriorityCmp(pLingCmnInfo, pbWordList[snH], snMidItem) < 0) {
                --snH;
            }

            if (snL <= snH) {
                __ValueSwap(pLingCmnInfo, snL, snH);
                ++snL;
                --snH;
            }
        }

        __ET9AWSortPriorityListQS(pLingCmnInfo, snLo, snH);
        __ET9AWSortPriorityListQS(pLingCmnInfo, snL, snHi);
    }
}

/*---------------------------------------------------------------------------*/
/** \internal
 * This function sorts the selection list in priority order.
 *
 * @param pLingCmnInfo              Pointer to alphabetic cmn information structure.
 *
 * @return None
 */

ET9INLINE static void ET9LOCALCALL __ET9AWSortPriorityList(ET9AWLingCmnInfo * const pLingCmnInfo)                                                           \
{
    __ET9AWSortPriorityListQS(pLingCmnInfo, 0, pLingCmnInfo->Private.bTotalWords - 1);

    __LogPartialSelList(pLingCmnInfo, 0, (ET9U8)(pLingCmnInfo->Private.bTotalWords - 1), "after __ET9AWSortPriorityListQS", pLogFile2);

    pLingCmnInfo->Private.snLinSearchCount = 0;

#ifdef ET9_DEBUG
    {
        ET9U8 * const pbWordList = pLingCmnInfo->Private.bWordList;

        ET9U8 pbCount[ET9MAXSELLISTSIZE];

        ET9U8 bIndex;

        for (bIndex = 0; bIndex < ET9MAXSELLISTSIZE; ++bIndex) {
            pbCount[bIndex] = 0;
        }

        for (bIndex = 0; bIndex < pLingCmnInfo->Private.bTotalWords; ++bIndex) {
            ++pbCount[pbWordList[bIndex]];
        }

        for (bIndex = 0; bIndex < ET9MAXSELLISTSIZE; ++bIndex) {
            ET9AssertLog(bIndex <  pLingCmnInfo->Private.bTotalWords && pbCount[bIndex] == 1 ||
                         bIndex >= pLingCmnInfo->Private.bTotalWords && pbCount[bIndex] == 0);
        }
    }
    {
        ET9U8 * const pbWordList = pLingCmnInfo->Private.bWordList;

        ET9U8 bIndex;
        ET9U8 bLook;

        for (bIndex = 0; bIndex + 1 < pLingCmnInfo->Private.bTotalWords; ++bIndex) {
            for (bLook = bIndex + 1; bLook < pLingCmnInfo->Private.bTotalWords; ++bLook) {
                ET9AssertLog(__PriorityCmp(pLingCmnInfo, pbWordList[bIndex], pbWordList[bLook]) >= 0);
            }
        }
    }
#endif
}

#ifdef ET9_DEBUG
/*---------------------------------------------------------------------------*/
/** \internal
 * This function verifies list order for a binary search.
 *
 * @param pLingCmnInfo              Pointer to alphabetic cmn information structure.
 *
 * @return None
 */

static void ET9LOCALCALL __ET9AWVerifyListOrderSearch(ET9AWLingCmnInfo * const pLingCmnInfo)
{
    if (pLingCmnInfo->Private.snLinSearchCount > DUPE_BIN_SEARCH_POINT) {

        ET9INT              snIndex;
        ET9AWPrivWordInfo   *pWord1;
        ET9AWPrivWordInfo   *pWord2;

        for (snIndex = 0; snIndex + 1 < pLingCmnInfo->Private.bTotalWords; ++snIndex) {

            pWord1 = &pLingCmnInfo->Private.pWordList[pLingCmnInfo->Private.bWordList[snIndex]];
            pWord2 = &pLingCmnInfo->Private.pWordList[pLingCmnInfo->Private.bWordList[snIndex + 1]];

            ET9AssertLog(pWord1->Base.wWordLen < pWord2->Base.wWordLen ||
                         pWord1->Base.wWordLen == pWord2->Base.wWordLen && _ET9symbncmp(pWord1->Base.sWord, pWord2->Base.sWord, pWord1->Base.wWordLen) <= 0);
        }
    }
}

#else
#define __ET9AWVerifyListOrderSearch(pLingCmnInfo)
#endif

/*---------------------------------------------------------------------------*/
/** \internal
 * This function checks if a word is a duplicate.
 *
 * @param pLingCmnInfo              Pointer to alphabetic cmn information structure.
 * @param pWord                     Pointer to word to look for.
 * @param ppWordFound               Pointer to found match, or NULL if not found.
 *
 * @return None
 */

ET9INLINE static void ET9LOCALCALL __ET9AWIsWordInList(ET9AWLingCmnInfo    * const pLingCmnInfo,
                                                       ET9AWPrivWordInfo   * const pWord,
                                                       ET9AWPrivWordInfo  ** const ppWordFound)
{
    /* sort the list, if not sorted already */

    if (pLingCmnInfo->Private.snLinSearchCount == DUPE_BIN_SEARCH_POINT) {
        __ET9AWSortSearchList(pLingCmnInfo);
    }

    /* verify list order (for search) */

    __ET9AWVerifyListOrderSearch(pLingCmnInfo);

    /* search for a match, not checking against words slated as dupe (handled in compare function) */

    if (pLingCmnInfo->Private.snLinSearchCount < DUPE_BIN_SEARCH_POINT) {

        /* list is NOT sorted, go for a linear search */

        ET9INT snCmp = 1;
        ET9INT snCount;
        ET9AWPrivWordInfo *pLookWord;

        ++pLingCmnInfo->Private.snLinSearchCount;

        pLookWord = pLingCmnInfo->Private.pWordList;

        for (snCount = pLingCmnInfo->Private.bTotalWords; snCount; --snCount, ++pLookWord) {

            snCmp = __LenSymbCmpWP(pLingCmnInfo, pLookWord, pWord);

            if (!snCmp) {
                break;
            }
        }

        if (snCmp) {
            *ppWordFound = NULL;
        }
        else {
            *ppWordFound = pLookWord;
        }
    }
    else {

        /* list is sorted, go for a binary search */

        ET9U8                * const pbWordList = pLingCmnInfo->Private.bWordList;
        ET9AWPrivWordInfo    * const pWordList = pLingCmnInfo->Private.pWordList;

        ET9AWPrivWordInfo *pLookWord;

        const ET9INT snN = pLingCmnInfo->Private.bTotalWords;

        ET9INT snMid;
        ET9INT snLow = 0;
        ET9INT snHigh = snN;
        ET9INT snCmp = 1;

        while (snLow < snHigh) {

            snMid = (snLow + snHigh) >> 1;

            snCmp = __LenSymbCmpWP(pLingCmnInfo, &pWordList[pbWordList[snMid]], pWord);

            if (!snCmp) {
                snLow = snMid;
                break;
            }
            else if (snCmp < 0) {
                snLow = snMid + 1;
            }
            else {
                snHigh = snMid;
            }
        }

        if (snLow >= snN) {
            snCmp = 1;
            pLookWord = NULL;
        }
        else {
            pLookWord = &pWordList[pbWordList[snLow]];

            if (snCmp) {
                snCmp = __LenSymbCmpWP(pLingCmnInfo, pLookWord, pWord);
            }
        }

        if (snCmp) {
            *ppWordFound = NULL;
        }
        else {
            *ppWordFound = pLookWord;
        }
    }
}

/*---------------------------------------------------------------------------*/
/** \internal
 * This function updates supp db freqs in the selection list.
 *
 * @param pLingInfo      Pointer to alphabetic information structure.
 *
 * @return None
 */

static void ET9LOCALCALL __ET9AWUpdateCustomWordFreqs(ET9AWLingInfo * const pLingInfo)
{
    ET9U8                       bListIndex;
    ET9AWLingCmnInfo * const    pLingCmnInfo = pLingInfo->pLingCmnInfo;
    ET9AWPrivWordInfo * const   pWordList = pLingCmnInfo->Private.pWordList;
    ET9U8             * const   bWordList = pLingCmnInfo->Private.bWordList;
    const ET9BOOL               bUsingLM = (ET9BOOL)(ET9LMENABLED(pLingCmnInfo) && pLingCmnInfo->Private.ALdbLM.bSupported);

    for (bListIndex = 0; bListIndex < pLingCmnInfo->Private.bTotalWords; ++bListIndex) {

        const ET9U8 bIndex = bWordList[bListIndex];
        ET9AWPrivWordInfo * const pWord = &pWordList[bIndex];

        /* valid user/oem words only */

        if (((GETRAWSRC(pWord->bWordSrc) == ET9WORDSRC_RUDB && pWord->bIsUDBWord && !pWord->dwWordIndex) ||
            GETRAWSRC(pWord->bWordSrc) == ET9WORDSRC_ASDB_SHORTCUT ||
            GETRAWSRC(pWord->bWordSrc) == ET9WORDSRC_MDB) &&
            pLingCmnInfo->Private.xMaxWordFreq) {

            if (GETRAWSRC(pWord->bWordSrc) == ET9WORDSRC_MDB) {
                pWord->xWordFreq = pLingCmnInfo->Private.xMaxWordFreq;
            }
            else {

                const ET9FREQPART xBaseFreq = bUsingLM ? ET9_SUPP_DB_BASE_FREQ : ET9_SUPP_DB_BASE_FREQ_UNI;

                if (pWord->xWordFreq > xBaseFreq) {
                    pWord->xWordFreq = (ET9FREQPART)(pWord->xWordFreq - xBaseFreq);
                }
                else {
                    pWord->xWordFreq = 1;
                }

                pWord->xWordFreq += pLingCmnInfo->Private.xMaxWordFreq;
            }

            pWord->xTotFreq = pWord->xWordFreq * pWord->xTapFreq;

            ET9AssertLog(pWord->xWordFreq >= 0);
            ET9AssertLog(pWord->xTotFreq >= 0);
        }
    }
}

/*---------------------------------------------------------------------------*/
/** \internal
 * This function gets max freq for LDB words in the selection list.
 *
 * @param pLingInfo      Pointer to alphabetic information structure.
 *
 * @return None
 */

static void ET9LOCALCALL __ET9AWGetMaxFreq(ET9AWLingInfo * const pLingInfo)
{
    ET9U8                       bListIndex = 0;
    ET9U8                       bIndex = 0;
    ET9AWLingCmnInfo * const    pLingCmnInfo = pLingInfo->pLingCmnInfo;
    ET9AWPrivWordInfo * const   pWordList = pLingCmnInfo->Private.pWordList;
    ET9U8             * const   bWordList = pLingCmnInfo->Private.bWordList;
    ET9FREQPART                 xWordFreq = 0;

    for (; bListIndex < pLingCmnInfo->Private.bTotalWords; ++bListIndex) {

        bIndex = bWordList[bListIndex];

        /* valid words only */

        if (xWordFreq < pWordList[bIndex].xWordFreq &&
            !pWordList[bIndex].Base.wWordCompLen &&
            !pWordList[bIndex].Base.bIsSpellCorr &&
            pWordList[bIndex].dwWordIndex > 0) {

            xWordFreq = pWordList[bIndex].xWordFreq;
        }
    }

    pLingCmnInfo->Private.xMaxWordFreq = xWordFreq;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * This function assigns LM freqs to words in the selection list.
 *
 * @param pLingInfo      Pointer to alphabetic information structure.
 *
 * @return None
 */

static void ET9LOCALCALL __ET9AWAssignLMFreqs(ET9AWLingInfo * const pLingInfo)
{
    ET9STATUS                   wStatus = ET9STATUS_NONE;
    ET9U8                       bListIndex;
    ET9FREQPART                 xWordFreq = 0;
    ET9AWLingCmnInfo * const    pLingCmnInfo = pLingInfo->pLingCmnInfo;
    ET9U16                      wSavedLDBNum = pLingCmnInfo->wLdbNum;
    ET9U8                       bLangIndex = 0;
    ET9AWPrivWordInfo * const   pWordList = pLingCmnInfo->Private.pWordList;
    ET9U8             * const   bWordList = pLingCmnInfo->Private.bWordList;

    const ET9FREQPART xSuppDbBaseFreq = ET9_SUPP_DB_BASE_FREQ;

    if (pLingCmnInfo->wFirstLdbNum != pLingCmnInfo->wLdbNum) {
        wStatus = _ET9AWLdbSetActiveLanguage(pLingInfo, pLingCmnInfo->wFirstLdbNum);
    }

    if (wStatus != ET9STATUS_NONE) {
        wStatus = _ET9AWLdbSetActiveLanguage(pLingInfo, wSavedLDBNum);
        return;
    }

    _ET9AWLdbTagContextClass(pLingInfo);

    for (bListIndex = 0; bListIndex < pLingCmnInfo->Private.bTotalWords; ++bListIndex) {

        const ET9U8 bIndex = bWordList[bListIndex];
        ET9AWPrivWordInfo * const pWord = &pWordList[bIndex];

        ET9AssertLog(pWord->xTapFreq >= 0);
        ET9AssertLog(pWord->xWordFreq >= 0);
        ET9AssertLog(pWord->xTotFreq >= 0);

        if (!pWord->dwWordIndex) {
            continue;
        }

        xWordFreq = 0;

        /* valid LDB words only */

        if (GETRAWSRC(pWord->bWordSrc) == ET9WORDSRC_LDB || pWord->bWordSrc == ET9WORDSRC_BUILDAROUND_LAS) {

            /* if curr lang, then find freq using known index */

            bLangIndex = pWord->Base.bLangIndex;

            if (bLangIndex == ET9AWFIRST_LANGUAGE || (bLangIndex == ET9AWBOTH_LANGUAGES && pLingCmnInfo->wFirstLdbNum == pLingCmnInfo->Private.wPreviousWordLanguage)) {

                _ET9AWLdbGetWordFreq(pLingInfo, pLingCmnInfo->wLdbNum,
                                    pWord->dwWordIndex - 1,
                                    &xWordFreq,
                                    &pWord->wEWordFreq,
                                    &pWord->wTWordFreq);

                ET9AssertLog(xWordFreq >= 0);

                /* update freqs */

                pWord->xWordFreq = xWordFreq;

                pWord->xTotFreq = (ET9FREQ)(pWord->xTapFreq * pWord->xWordFreq);

            }
        }
        else if (pLingCmnInfo->Private.eCurrSelectionListMode == ET9ASLMODE_MIXED) {

            if (GETRAWSRC(pWord->bWordSrc) == ET9WORDSRC_LAS_SHORTCUT ||
                (ISGENUINESRC(pWord->bWordSrc) && pWord->dwWordIndex > 0)) { /* valid LDB-AS or RDB words */

                /* if curr lang, then find freq using known index */

                bLangIndex = pWord->Base.bLangIndex;

                if (bLangIndex == ET9AWFIRST_LANGUAGE || (bLangIndex == ET9AWBOTH_LANGUAGES && pLingCmnInfo->wFirstLdbNum == pLingCmnInfo->Private.wPreviousWordLanguage)) {

                    _ET9AWLdbGetWordFreq(pLingInfo,
                                         pLingCmnInfo->wLdbNum,
                                         pWord->dwWordIndex - 1,
                                         &xWordFreq,
                                         &pWord->wEWordFreq,
                                         &pWord->wTWordFreq);

                    ET9AssertLog(xWordFreq >= 0);

                    /* update freqs */

                    if (GETRAWSRC(pWord->bWordSrc) == ET9WORDSRC_LAS_SHORTCUT) {
                        pWord->xWordFreq = xWordFreq;
                    }
                    else {

                        ET9AssertLog(pWord->xWordFreq + xWordFreq >= xSuppDbBaseFreq);

                        pWord->xWordFreq = (ET9FREQPART)(pWord->xWordFreq - xSuppDbBaseFreq);
                        pWord->xWordFreq = pWord->xWordFreq + xWordFreq;

                        ET9AssertLog(pWord->xWordFreq >= 0);
                    }

                    pWord->xTotFreq = (ET9FREQ)(pWord->xTapFreq * pWord->xWordFreq);

                }
            }
        }

        if (pLingCmnInfo->Private.xMaxWordFreq < xWordFreq &&
            !pWord->Base.wWordCompLen &&
            !pWord->Base.bIsSpellCorr) {

            pLingCmnInfo->Private.xMaxWordFreq = xWordFreq;
        }

        ET9AssertLog(pWord->xTapFreq >= 0);
        ET9AssertLog(pWord->xWordFreq >= 0);
        ET9AssertLog(pWord->xTotFreq >= 0);
    }

    if (ET9AWSys_GetBilingualSupported(pLingInfo)) {

        wStatus = _ET9AWLdbSetActiveLanguage(pLingInfo, pLingCmnInfo->wSecondLdbNum);

        if (wStatus != ET9STATUS_NONE) {
            wStatus = _ET9AWLdbSetActiveLanguage(pLingInfo, wSavedLDBNum);
            return;
        }

        _ET9AWLdbTagContextClass(pLingInfo);

        for (bListIndex = 0; bListIndex < pLingCmnInfo->Private.bTotalWords; ++bListIndex) {

            const ET9U8 bIndex = bWordList[bListIndex];
            ET9AWPrivWordInfo * const pWord = &pWordList[bIndex];

            ET9AssertLog(pWord->xTapFreq >= 0);
            ET9AssertLog(pWord->xWordFreq >= 0);
            ET9AssertLog(pWord->xTotFreq >= 0);

            if (!pWord->dwWordIndex) {
                continue;
            }

            xWordFreq = 0;

            /* valid LDB words only */

            if (GETRAWSRC(pWord->bWordSrc) == ET9WORDSRC_LDB || pWord->bWordSrc == ET9WORDSRC_BUILDAROUND_LAS) {

                /* if curr lang, then find freq using known index */

                bLangIndex = pWord->Base.bLangIndex;

                if (bLangIndex == ET9AWSECOND_LANGUAGE || (bLangIndex == ET9AWBOTH_LANGUAGES && pLingCmnInfo->wSecondLdbNum == pLingCmnInfo->Private.wPreviousWordLanguage)) {

                    _ET9AWLdbGetWordFreq(pLingInfo,
                                         pLingCmnInfo->wLdbNum,
                                         pWord->dwWordIndex - 1,
                                         &xWordFreq,
                                         &pWord->wEWordFreq,
                                         &pWord->wTWordFreq);

                    ET9AssertLog(xWordFreq >= 0);

                    /* update freqs */

                    pWord->xWordFreq = xWordFreq;

                    pWord->xTotFreq = (ET9FREQ)(pWord->xTapFreq * pWord->xWordFreq);

                }
            }
            else if (pLingCmnInfo->Private.eCurrSelectionListMode == ET9ASLMODE_MIXED) {

                if (ISGENUINESRC(pWord->bWordSrc) && pWord->dwWordIndex > 0) { /* valid LDB-AS or RDB words */

                    /* if curr lang, then find freq using known index */

                    bLangIndex = pWord->Base.bLangIndex;

                    if (bLangIndex == ET9AWSECOND_LANGUAGE || (bLangIndex == ET9AWBOTH_LANGUAGES && pLingCmnInfo->wSecondLdbNum == pLingCmnInfo->Private.wPreviousWordLanguage)) {

                        _ET9AWLdbGetWordFreq(pLingInfo, pLingCmnInfo->wLdbNum,
                                             pWord->dwWordIndex - 1,
                                             &xWordFreq,
                                             &pWord->wEWordFreq,
                                             &pWord->wTWordFreq);

                        ET9AssertLog(xWordFreq >= 0);

                        /* update freqs */

                        if (GETRAWSRC(pWord->bWordSrc) == ET9WORDSRC_LAS_SHORTCUT) {
                            pWord->xWordFreq = xWordFreq;
                        }
                        else {

                            ET9AssertLog(pWord->xWordFreq + xWordFreq >= xSuppDbBaseFreq);

                            pWord->xWordFreq = (ET9FREQPART)(pWord->xWordFreq - xSuppDbBaseFreq);
                            pWord->xWordFreq += xWordFreq;

                            ET9AssertLog(pWord->xWordFreq >= 0);
                        }

                        pWord->xTotFreq = (ET9FREQ)(pWord->xTapFreq * pWord->xWordFreq);

                    }
                }
            }

            if (pLingCmnInfo->Private.xMaxWordFreq < xWordFreq  &&
                !pWord->Base.wWordCompLen &&
                !pWord->Base.bIsSpellCorr) {

                pLingCmnInfo->Private.xMaxWordFreq = xWordFreq;
            }

            ET9AssertLog(pWord->xTapFreq >= 0);
            ET9AssertLog(pWord->xWordFreq >= 0);
            ET9AssertLog(pWord->xTotFreq >= 0);
        }
    }

    if (wSavedLDBNum != pLingCmnInfo->wLdbNum) {

        wStatus = _ET9AWLdbSetActiveLanguage(pLingInfo, wSavedLDBNum);

        if (pLingCmnInfo->wFirstLdbNum == wSavedLDBNum) {
            _ET9AWLdbTagContextClass(pLingInfo);
        }
    }
}

/*---------------------------------------------------------------------------*/
/** \internal
 * This function checks to see if word is a duplicate to something already in the list.
 *
 * @param pLingInfo                 Pointer to alphabetic information structure.
 * @param pWord                     Word to check.
 *
 * @return ET9STATUS_WORD_EXISTS or ET9STATUS_NONE.
 */

static ET9STATUS ET9LOCALCALL __ET9AWSelLstCheckDups(ET9AWLingInfo       * const pLingInfo,
                                                     ET9AWPrivWordInfo   * const pWord)
{
    ET9AWPrivWordInfo       *pAmbigWord;
    ET9U16                   wPreviousLDBNum = 0;

    ET9AWLingCmnInfo * const pLingCmnInfo = pLingInfo->pLingCmnInfo;
    const ET9BOOL            bIsNWP = (ET9BOOL)!pLingCmnInfo->Base.pWordSymbInfo->bNumSymbs;

    ET9AssertLog(pWord != NULL);
    ET9AssertLog(pLingInfo != NULL);

    WLOG2BWord(pLogFile2, "__ET9AWSelLstCheckDups, pWord", pWord);

    /* mark the word with quality info (origin)
       also deal with collapsing shortcuts (completion or spc) */

    switch (GETRAWSRC(pWord->bWordSrc))
    {
        case ET9WORDSRC_LDB:
        case ET9WORDSRC_LDB_PROMOTION:
        case ET9WORDSRC_MDB:
        case ET9WORDSRC_CDB:
        case ET9WORDSRC_CSP:
            pWord->bWordQuality = GENUINE_QUALITY;
            break;
        case ET9WORDSRC_LAS_SHORTCUT:
        case ET9WORDSRC_ASDB_SHORTCUT:
            if ((pWord->Base.wWordCompLen || pWord->bHasPrimEditDist) && !bIsNWP) {
                WLOG2(fprintf(pLogFile2, "++ wWordCompLen %u, bHasPrimEditDist %u, bIsNWP %u, bWordSrc %u, raw bWordSrc %u\n", pWord->Base.wWordCompLen, pWord->bHasPrimEditDist, bIsNWP, pWord->bWordSrc, GETRAWSRC(pWord->bWordSrc));)
                pWord->bWordQuality = STEM_QUALITY;
                pWord->bWordSrc = ET9WORDSRC_STEMPOOL;      /* lower prio - less space */
                WLOG2Word(pLogFile2, "__ET9AWSelLstCheckDups, AS into stem pool", pWord);
            }
            else if (!pWord->Base.wSubstitutionLen) {
                pWord->bWordQuality = GENUINE_QUALITY;
            }
            else {
                pWord->bWordQuality = LIMITED_QUALITY;
            }
            break;
        case ET9WORDSRC_RUDB:
            if (pWord->bIsUDBWord) {
                pWord->bWordQuality = GENUINE_QUALITY;
            }
            else {
                pWord->bWordQuality = DISPOSABLE_QUALITY;
            }
            break;
        case ET9WORDSRC_STEMPOOL:
            if (pLingCmnInfo->Private.bStemsAllowed) {
                pWord->bWordQuality = STEM_QUALITY;
            }
            else {
                pWord->bWordQuality = DISPOSABLE_QUALITY;
            }
            break;
        default:
            pWord->bWordQuality = SHAPED_QUALITY;
            break;
    }

    /* search list for a match */

    __ET9AWIsWordInList(pLingCmnInfo, pWord, &pAmbigWord);

    if (!pAmbigWord) {
        return ET9STATUS_NONE;  /* no match found */
    }

    /* match found */

    ET9AssertLog(pWord->Base.wWordLen == pAmbigWord->Base.wWordLen);

    /* matched word inherits language attribute for both languages if the new candidate word is a
       term and not of SHAPED_QUALITY. The only exception to this rule is when the existing word
       is of SHAPED_QUALITY, in which case it gets the language index of the candidate word. When
       matched word is a stem but new candidate word is a term, then matched word inherits the
       language of the candidate. Additionally, words do not inherit the language from auto appends.
       But in all cases the language supported for the input length corresponds to both languages. */

    wPreviousLDBNum = pLingCmnInfo->Private.wPreviousWordLanguage;

    if (pAmbigWord->Base.bLangIndex &&
        pWord->Base.bLangIndex &&
        pAmbigWord->Base.bLangIndex != pWord->Base.bLangIndex) {

        if (pAmbigWord->Base.bIsTerm == pWord->Base.bIsTerm) {

            if (pAmbigWord->bWordQuality != SHAPED_QUALITY && pWord->bWordQuality == SHAPED_QUALITY) {
            }
            else if (pAmbigWord->bWordQuality == SHAPED_QUALITY && pWord->bWordQuality != SHAPED_QUALITY) {

                WLOG2(fprintf(pLogFile2, "dups: adjusting recipient langIndex, %s -> %s\n", LINDEXTOSTRING(pAmbigWord->Base.bLangIndex), LINDEXTOSTRING(pWord->Base.bLangIndex));)

                pAmbigWord->Base.bLangIndex = pWord->Base.bLangIndex;
                pAmbigWord->dwWordIndex = pWord->dwWordIndex;
            }
            else if (pWord->bWordSrc != ET9WORDSRC_AUTOAPPEND) {

                if (pWord->dwWordIndex && pAmbigWord->dwWordIndex) {
                    if ((pWord->Base.bLangIndex == ET9AWFIRST_LANGUAGE && wPreviousLDBNum == pLingCmnInfo->wFirstLdbNum) ||
                        (pWord->Base.bLangIndex == ET9AWSECOND_LANGUAGE && wPreviousLDBNum == pLingCmnInfo->wSecondLdbNum)) {

                        pAmbigWord->dwWordIndex = pWord->dwWordIndex;
                    }
                }

                WLOG2(fprintf(pLogFile2, "dups: adjusting recipient langIndex, %s -> %s\n", LINDEXTOSTRING(pAmbigWord->Base.bLangIndex), LINDEXTOSTRING(ET9AWBOTH_LANGUAGES));)

                pAmbigWord->Base.bLangIndex = ET9AWBOTH_LANGUAGES;
            }

        }
        else if (!pAmbigWord->Base.bIsTerm && pWord->Base.bIsTerm) {

            WLOG2(fprintf(pLogFile2, "dups: adjusting recipient langIndex, %s -> %s\n", LINDEXTOSTRING(pAmbigWord->Base.bLangIndex), LINDEXTOSTRING(pWord->Base.bLangIndex));)

            pAmbigWord->Base.bLangIndex = pWord->Base.bLangIndex;
            pAmbigWord->dwWordIndex = pWord->dwWordIndex;
        }

    }
    else if (!pAmbigWord->Base.bLangIndex && pWord->Base.bLangIndex) {
        if (pAmbigWord->Base.bIsTerm == pWord->Base.bIsTerm) {
            if (pAmbigWord->bWordQuality != SHAPED_QUALITY && pWord->bWordQuality == SHAPED_QUALITY) {
            }
            else {

                WLOG2(fprintf(pLogFile2, "dups: adjusting recipient langIndex, %s -> %s\n", LINDEXTOSTRING(pAmbigWord->Base.bLangIndex), LINDEXTOSTRING(pWord->Base.bLangIndex));)

                pAmbigWord->Base.bLangIndex = pWord->Base.bLangIndex;
                pAmbigWord->dwWordIndex = pWord->dwWordIndex;
            }
        }
    }

    /* stem distance for the same "word" can vary depending on how they got built, but the insert logic expects
       them to be the same for non terms.
       (another problem would be if a stem ever wins over a term...) */

    if (!pWord->Base.bIsTerm && pWord->bEditDistStem != pAmbigWord->bEditDistStem) {

        WLOG2(fprintf(pLogFile2, "dups: adjusting donor's stem distance (stems)\n");)

        pWord->bEditDistStem = pAmbigWord->bEditDistStem;
    }

    /* when a term gets truncated (won't fit the word size) there is a rare case when it will turn into a stem
       (without edit distance) and possibly dupe with a spell correction that got to the same result.
       This will cause the record keeping of spell correction count to go wrong, since a word will inherit
       the spell correction property without pushing another candidate out.
       This special case will take out that spell correction value.
       There is also a case when flipping convert-symb during a build, that will cause this to happen for terms
       when there is a flush present. */

    if (!pAmbigWord->bHasPrimEditDist && pWord->bHasPrimEditDist) {

        WLOG2(fprintf(pLogFile2, "dups: adjusting donor's primary edit distance (etc)\n");)

        pWord->bHasPrimEditDist  = pAmbigWord->bHasPrimEditDist;
        pWord->bEditDistSpc      = pAmbigWord->bEditDistSpc;
        pWord->bEditDistStem     = pAmbigWord->bEditDistStem;
        pWord->bEditDistFree     = pAmbigWord->bEditDistFree;
        pWord->bHasPrimEditDist  = pAmbigWord->bHasPrimEditDist;
        pWord->Base.bIsSpellCorr = pAmbigWord->Base.bIsSpellCorr;
    }

    /* there is a duplicate, if the duplicate is with the exact word,
       set bExactInList, but do not remove. Also, mark its source as SAMEASEXACT. It will
       tell us that an object matching the exact input is in the list, and the list processing at the
       end of __ET9AWDoSelLstBuild() will set this as the default object. Exceptions to this processing
       happen for numeric strings, words of length one, and words with completions. */

    if (ISEXACTSRC(pAmbigWord->bWordSrc)) {

        WLOG2Word(pLogFile2, "dups: exact word will inherit", pAmbigWord);
        WLOG2Word(pLogFile2, "dups: this is the donor word", pWord);

        if (pWord->Base.bIsTerm && ISBUILDABLESRC(pWord->bWordSrc)) {
            pLingCmnInfo->Private.dwStateBits |= ET9SLBUILDABLEEXACT;
        }

        if (pWord->Base.bIsTerm &&
            (ISREQUIREDSRC(pWord->bWordSrc) ||
             pWord->Base.wWordLen != 1 ||
             !_ET9_IsNumericString(pWord->Base.sWord, 1))) {

            /* pAmbigWord should have the src updated if it didn't have a proper one from the beginning
               or if the newer one is an improvement (or should it not? used to be always)

               used to NOT allow any numeric strings to win here, now all allowed
               exception 1) single digits are disallowed */

            {
                ET9U8 bWordBaseSrc = GETBASESRC(pWord->bWordSrc);
                ET9U8 bAmbigBaseSrc = GETBASESRC(pAmbigWord->bWordSrc);

                if (ISEXACTSRC(pAmbigWord->bWordSrc)) {
                    pWord->bWordSrc |= EXACTOFFSET;     /* or else the prio test below will be no good */
                }

                if (ISREALSRC(bWordBaseSrc) &&
                    !(ISPUNCTSRC(bWordBaseSrc) &&
                    pWord->Base.sWord[pWord->Base.wWordLen-1] != _ET9_GetTermPunctChar(pLingInfo, pLingCmnInfo->wLdbNum, 0)) &&
                    (bAmbigBaseSrc == ET9WORDSRC_NONE || __PriorityCompare(pLingCmnInfo, pWord, pAmbigWord, 0) > 0)) {

                    WLOG2(fprintf(pLogFile2, "dups: exact actually picks up attributes (lost)\n");)

                    pAmbigWord->xTotFreq        = pWord->xTotFreq;
                    pAmbigWord->xTapFreq        = pWord->xTapFreq;
                    pAmbigWord->xWordFreq       = pWord->xWordFreq;
                    pAmbigWord->Base.bLangIndex = pWord->Base.bLangIndex;
                    pAmbigWord->dwWordIndex     = pWord->dwWordIndex;
                    pAmbigWord->bWordSrc        = bWordBaseSrc | EXACTOFFSET;

                    ET9AssertLog(pAmbigWord->xTapFreq >= 0);
                    ET9AssertLog(pAmbigWord->xTotFreq >= 0);
                    ET9AssertLog(pAmbigWord->xWordFreq >=0);
                }
            }
        }
    }
    else if (__PriorityCompare(pLingCmnInfo, pWord, pAmbigWord, 0) > 0) {

        WLOG2Word(pLogFile2, "dups: this word will inherit", pAmbigWord);
        WLOG2Word(pLogFile2, "dups: this is the donor word", pWord);

        /* pWord will never donate a stem attribute to a term, thus it must be adjusted
           to not cause a false chunk counter update */

        ET9AssertLog(pWord->Base.bIsTerm && !pWord->Base.wWordCompLen || !pWord->Base.bIsTerm);

        if (pAmbigWord->Base.bIsTerm && !pWord->Base.bIsTerm) {

            WLOG2(fprintf(pLogFile2, "dups: adjusting donor stem to term\n");)

            pWord->Base.bIsTerm = 1;
            pWord->Base.wWordCompLen = 0;
        }

        if (pAmbigWord->Base.wWordCompLen < pWord->Base.wWordCompLen) {

            WLOG2(fprintf(pLogFile2, "dups: adjusting donor compl len to %u (from %u)\n", pAmbigWord->Base.wWordCompLen, pWord->Base.wWordCompLen);)

            pWord->Base.wWordCompLen = pAmbigWord->Base.wWordCompLen;
        }

        /* since we are keeping track of counts we must update that info here as well */

        __ET9UpdateInsertCounters(pLingCmnInfo, pAmbigWord, pWord);

        /* propagate info */

        pAmbigWord->bWordSrc          = pWord->bWordSrc;
        pAmbigWord->Base.bLangIndex   = pWord->Base.bLangIndex;
        pAmbigWord->dwWordIndex       = pWord->dwWordIndex;
        pAmbigWord->xTapFreq          = pWord->xTapFreq;
        pAmbigWord->xWordFreq         = pWord->xWordFreq;
        pAmbigWord->xTotFreq          = pWord->xTotFreq;

        pAmbigWord->bEditDistSpc      = pWord->bEditDistSpc;
        pAmbigWord->bEditDistStem     = pWord->bEditDistStem;
        pAmbigWord->bHasPrimEditDist  = pWord->bHasPrimEditDist;
        pAmbigWord->Base.bIsSpellCorr = pWord->Base.bIsSpellCorr;

        pAmbigWord->bCDBTrigram       = pWord->bCDBTrigram;

        ET9AssertLog(pAmbigWord->xTapFreq >= 0);
        ET9AssertLog(pAmbigWord->xTotFreq >= 0);
        ET9AssertLog(pAmbigWord->xWordFreq >= 0);

        /* This function may call two words a dup, even if the words have different completion
           lengths (see comments above, at top of loop), but only if word completion is off.
           If the word completion lengths are not the same, and a terminal word matches the stem
           of a longer word in the selection list, the longer word is replaced with the terminal word.
           If the word we are comparing two completion words with the same stem, it is not
           necessary to replace the characters to match the winning word, only its source and frequency
           data, since the completion will be removed in post processing. */

        pAmbigWord->Base.bIsTerm = pWord->Base.bIsTerm;
        pAmbigWord->Base.wWordCompLen = pWord->Base.wWordCompLen;
    }

    /* transfer origin and udb tag */

    if (pAmbigWord->bWordQuality < pWord->bWordQuality) {
        pAmbigWord->bWordQuality = pWord->bWordQuality;
    }

    if (pWord->bIsUDBWord) {
        pAmbigWord->bIsUDBWord = pWord->bIsUDBWord;
    }

    /* if word doesn't have a substitution, OR
       higher priority word has it's own substitution, load it in */

    if (!pAmbigWord->Base.wSubstitutionLen || pWord->Base.wSubstitutionLen) {

        pAmbigWord->Base.wSubstitutionLen = pWord->Base.wSubstitutionLen;

        if (pWord->Base.wSubstitutionLen) {

            WLOG2(fprintf(pLogFile2, "dups: inherited substitution\n");)

            _ET9SymCopy(pAmbigWord->Base.sSubstitution, pWord->Base.sSubstitution, pWord->Base.wSubstitutionLen);
        }
    }

    /* the word must buildable for even being considered as a shortcut */

    if (pAmbigWord->Base.wSubstitutionLen && !ISGENUINESRC(pAmbigWord->bWordSrc)) {

        WLOG2(fprintf(pLogFile2, "dups: non buildable source, substitution removed\n");)

        pAmbigWord->Base.wSubstitutionLen = 0;
    }

    /* If class model supported, propagate the class-based frequency from LDB to RUDB word */

    if (ET9LMENABLED(pLingCmnInfo) && pLingCmnInfo->Private.ALdbLM.bSupported) {

        if (pWord->Base.bIsTerm == pAmbigWord->Base.bIsTerm &&
            GETRAWSRC(pWord->bWordSrc) == ET9WORDSRC_LDB &&
            GETRAWSRC(pAmbigWord->bWordSrc) == ET9WORDSRC_RUDB && !pAmbigWord->bIsUDBWord) {

            WLOG2(fprintf(pLogFile2, "dups: propagating index from LDB word to RDB word \n");)

            if (ET9AWSys_GetBilingualSupported(pLingInfo)) {
                if (pAmbigWord->Base.bLangIndex == pWord->Base.bLangIndex) {
                    pAmbigWord->Base.bLangIndex = pWord->Base.bLangIndex;
                    pAmbigWord->dwWordIndex = pWord->dwWordIndex;
                }
                else if (pAmbigWord->Base.bLangIndex == ET9AWBOTH_LANGUAGES) {
                    if ((pWord->Base.bLangIndex == ET9AWFIRST_LANGUAGE && wPreviousLDBNum == pLingCmnInfo->wFirstLdbNum) ||
                        (pWord->Base.bLangIndex == ET9AWSECOND_LANGUAGE && wPreviousLDBNum == pLingCmnInfo->wSecondLdbNum)) {

                        pAmbigWord->dwWordIndex = pWord->dwWordIndex;
                    }
                }
            }
            else {
                pAmbigWord->Base.bLangIndex = pWord->Base.bLangIndex;
                pAmbigWord->dwWordIndex = pWord->dwWordIndex;
            }
        }
    }
    else if(pLingCmnInfo->Private.eCurrSelectionListMode == ET9ASLMODE_MIXED) {

        if (pWord->Base.bIsTerm == pAmbigWord->Base.bIsTerm &&
            pWord->dwWordIndex && !pAmbigWord->dwWordIndex &&
            GETRAWSRC(pAmbigWord->bWordSrc) == ET9WORDSRC_RUDB && !pAmbigWord->bIsUDBWord &&
            ((GETRAWSRC(pWord->bWordSrc) == ET9WORDSRC_LDB && ISGENUINESRC(pWord->bWordSrc)) ||
             (ISGENUINESRC(pWord->bWordSrc) == ISGENUINESRC(pAmbigWord->bWordSrc)))) {

            const ET9FREQPART xSuppDbBaseFreqUni = ET9_SUPP_DB_BASE_FREQ_UNI;

            WLOG2(fprintf(pLogFile2, "dups: propagating index from LDB word to RDB word \n");)

            if (ET9AWSys_GetBilingualSupported(pLingInfo)) {

                if (pAmbigWord->Base.bLangIndex == pWord->Base.bLangIndex) {

                    ET9AssertLog(pAmbigWord->xWordFreq + pWord->xWordFreq >= xSuppDbBaseFreqUni);

                    pAmbigWord->Base.bLangIndex = pWord->Base.bLangIndex;
                    pAmbigWord->dwWordIndex = pWord->dwWordIndex;
                    pAmbigWord->xWordFreq += (ET9FREQPART)(pWord->xWordFreq - xSuppDbBaseFreqUni);
                    pAmbigWord->xTapFreq = pWord->xTapFreq;
                    pAmbigWord->xTotFreq = pAmbigWord->xTapFreq * pAmbigWord->xWordFreq;
                }
                else if (pAmbigWord->Base.bLangIndex == ET9AWBOTH_LANGUAGES) {

                    if ((pWord->Base.bLangIndex == ET9AWFIRST_LANGUAGE && wPreviousLDBNum == pLingCmnInfo->wFirstLdbNum) ||
                        (pWord->Base.bLangIndex == ET9AWSECOND_LANGUAGE && wPreviousLDBNum == pLingCmnInfo->wSecondLdbNum)) {

                        ET9AssertLog(pAmbigWord->xWordFreq + pWord->xWordFreq >= xSuppDbBaseFreqUni);

                        pAmbigWord->dwWordIndex = pWord->dwWordIndex;
                        pAmbigWord->xWordFreq += (ET9FREQPART)(pWord->xWordFreq - xSuppDbBaseFreqUni);
                        pAmbigWord->xTapFreq = pWord->xTapFreq;
                        pAmbigWord->xTotFreq = pAmbigWord->xTapFreq * pAmbigWord->xWordFreq;
                    }
                }
            }
            else {

                ET9AssertLog(pAmbigWord->xWordFreq + pWord->xWordFreq >= xSuppDbBaseFreqUni);

                pAmbigWord->Base.bLangIndex = pWord->Base.bLangIndex;
                pAmbigWord->dwWordIndex = pWord->dwWordIndex;
                pAmbigWord->xWordFreq += (ET9FREQPART)(pWord->xWordFreq - xSuppDbBaseFreqUni);
                pAmbigWord->xTapFreq = pWord->xTapFreq;
                pAmbigWord->xTotFreq = pAmbigWord->xTapFreq * pAmbigWord->xWordFreq;
            }

            ET9AssertLog(pAmbigWord->xTapFreq >= 0);
            ET9AssertLog(pAmbigWord->xTotFreq >= 0);
            ET9AssertLog(pAmbigWord->xWordFreq >= 0);
        }
    }

    __VerifyWordCounters(pLingCmnInfo);

    return ET9STATUS_WORD_EXISTS;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * This function prepends one word to another.
 *
 * @param pWord                     Word to be changed.
 * @param pPrefixWord               Word to be prepended to pWord.
 * @param wLengthToPrepend          Length of pPrefixWord to use.
 *
 * @return Zero on failure, otherwise non zero.
 */

static ET9BOOL ET9LOCALCALL __ET9PrependWord(ET9AWPrivWordInfo * const  pWord,
                                             ET9AWPrivWordInfo * const  pPrefixWord,
                                             const ET9U16               wLengthToPrepend)
{
    ET9AssertLog(pPrefixWord);

    /* discard result if already the prefix fills the word */

    if (pPrefixWord->Base.wWordLen >= ET9MAXWORDSIZE) {
        return 0;
    }

    /* discard result if prefix is shorter than what's to be prepended */

    if (pPrefixWord->Base.wWordLen < wLengthToPrepend) {
        return 0;
    }

    /* prepend */

    ET9AssertLog(pWord);
    ET9AssertLog(pWord->Base.wWordLen <= ET9MAXWORDSIZE);
    ET9AssertLog(pPrefixWord->Base.wWordLen <= ET9MAXWORDSIZE);

    WLOG2B(fprintf(pLogFile2, "__ET9PrependWord, wLengthToPrepend = %d\n", wLengthToPrepend);)
    WLOG2BWord(pLogFile2, "__ET9PrependWord, pWord (in)", pWord);
    WLOG2BWord(pLogFile2, "__ET9PrependWord, pPrefixWord (in)", pPrefixWord);

    /* adjust pWord to fit the prefix first */

    {
        ET9U16 wWordLen = pWord->Base.wWordLen;

        /* truncate needed? */

        if (wWordLen + wLengthToPrepend > ET9MAXWORDSIZE) {

            const ET9U16 wTruncateCount = wWordLen + wLengthToPrepend - ET9MAXWORDSIZE;

            wWordLen = (ET9U16)(wWordLen - wTruncateCount);

            if (pWord->Base.wWordCompLen <= wTruncateCount) {
                pWord->Base.wWordCompLen = 0;
            }
            else {
                pWord->Base.wWordCompLen = (ET9U16)(pWord->Base.wWordCompLen - wTruncateCount);
            }
        }

        /* move symbs up */

        {
            ET9U16 wCount;
            ET9SYMB *psSrc = &pWord->Base.sWord[wWordLen - 1];
            ET9SYMB *psDst = &pWord->Base.sWord[wWordLen + wLengthToPrepend - 1];

            for (wCount = wWordLen; wCount; --wCount, --psSrc, --psDst) {
                *psDst = *psSrc;
            }
        }

        pWord->Base.wWordLen = (ET9U16)(wWordLen + wLengthToPrepend);
    }

    /* copy in the prefix */

    _ET9SymCopy(pWord->Base.sWord, pPrefixWord->Base.sWord, wLengthToPrepend);

    /* propagate distance */

    pWord->bEditDistSpc = (ET9U8)(pWord->bEditDistSpc + pPrefixWord->bEditDistSpc);
    pWord->bEditDistStem = (ET9U8)(pWord->bEditDistStem + pPrefixWord->bEditDistStem);

    /* done */

    WLOG2BWord(pLogFile2, "__ET9PrependWord, pWord (out)", pWord);

    return 1;
}


/*---------------------------------------------------------------------------
 *
 *   Function: insert helpers
 *
 *---------------------------------------------------------------------------*/

#ifdef ET9_ACTIVATE_SLST_STATS
#define __ET9AWSelLstIncInsertCounter       ++pLingCmnInfo->Private.sStats.dwInsertCount; ++pLingCmnInfo->Private.sStats.dwTotInsertCount;
#define __ET9AWSelLstIncDuplicateCounter    ++pLingCmnInfo->Private.sStats.dwTotInsertDuplicate;
#define __ET9AWSelLstIncReplacingCounter    ++pLingCmnInfo->Private.sStats.dwTotInsertReplacing;
#define __ET9AWSelLstIncDiscardedCounter    ++pLingCmnInfo->Private.sStats.dwTotInsertDiscarded;
#else
#define __ET9AWSelLstIncInsertCounter
#define __ET9AWSelLstIncDuplicateCounter
#define __ET9AWSelLstIncReplacingCounter
#define __ET9AWSelLstIncDiscardedCounter
#endif

#ifdef ET9_PRE_DUPE_SELLIST
#define __ET9AWSelLstDupeComment(x)         if (pPreDupeFile) { fwprintf(pPreDupeFile, x); }
#define __ET9AWSelLstDupeRecordWord(x)      if (pPreDupeFile) { __ET9PreDupeRecordWordInfo(x); }
#else
#define __ET9AWSelLstDupeComment(x)
#define __ET9AWSelLstDupeRecordWord(x)
#endif

/*---------------------------------------------------------------------------*/
/** \internal
 * This function inserts a word into the selection list.
 *
 * @param pLingInfo                 Pointer to alphabetic information structure.
 * @param pWord                     Word to add.
 * @param bFreqIndicator            Designation
 * @param bPreScreenedDupe          If the word is pre screened to not be a dupe.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

static ET9STATUS ET9LOCALCALL __ET9AWSelLstInsert(ET9AWLingInfo              * const pLingInfo,
                                                  ET9AWPrivWordInfo          * const pWord,
                                                  const ET9_FREQ_DESIGNATION         bFreqIndicator,
                                                  const ET9BOOL                      bPreScreenedDupe)
{
    ET9STATUS                   wStatus = ET9STATUS_NONE;
    ET9AWPrivWordInfo           *pAmbigWord = NULL;
    ET9S16                      i;
    ET9AWPrivWordInfo           sStemWord;
    ET9U16                      wLdbNum;

    ET9AWLingCmnInfo    * const pLingCmnInfo = pLingInfo->pLingCmnInfo;
    ET9WordSymbInfo     * const pWordSymbInfo = pLingCmnInfo->Base.pWordSymbInfo;
    const ET9U16                wLockPoint = pLingCmnInfo->Private.wCurrLockPoint;
    const ET9U16                wLeftWordLen = pLingCmnInfo->Private.sLeftHandWord.Base.wWordLen;

    ET9AssertLog(pWord);
    ET9AssertLog(pLingInfo);
    ET9AssertLog(pWordSymbInfo);

    ET9AssertLog(pWord->xTapFreq >= 0);
    ET9AssertLog(pWord->xWordFreq >= 0);
    ET9AssertLog(pWord->xTotFreq >= 0);

    ET9AssertLog(pWord->Base.bIsTerm && !pWord->Base.wWordCompLen || !pWord->Base.bIsTerm);
    ET9AssertLog(!pWord->bEditDistSpcSbt && !pWord->bEditDistSpcTrp && !pWord->bEditDistSpcIns && !pWord->bEditDistSpcDel || pWord->bEditDistSpcSbt + pWord->bEditDistSpcTrp + pWord->bEditDistSpcIns + pWord->bEditDistSpcDel == pWord->bEditDistSpc);

    WLOG2BWord(pLogFile2, "_ET9AWSelLstInsert, pWord", pWord);

    /* handle substitution expansion */

    if (pWord->Base.wSubstitutionLen && ET9EXPANDAUTOSUB(pLingCmnInfo)) {

        const ET9BOOL bIsNWP = (ET9BOOL)!pLingCmnInfo->Base.pWordSymbInfo->bNumSymbs;
        const ET9BOOL bSuppressSubst = (ET9BOOL)((pWord->Base.wWordCompLen || pWord->bEditDistSpc > 0) && !bIsNWP);

        WLOG2(fprintf(pLogFile2, "expanding auto-sub, bSuppressSubst %d\n", bSuppressSubst);)

        if (bSuppressSubst) {

            pWord->Base.wSubstitutionLen = 0;
        }
        else {

            ET9AssertLog(pWord->Base.wSubstitutionLen <= ET9MAXWORDSIZE);

            _ET9SymCopy(pWord->Base.sWord, pWord->Base.sSubstitution, pWord->Base.wSubstitutionLen);

            pWord->Base.wWordLen = pWord->Base.wSubstitutionLen;
            pWord->Base.wWordCompLen = 0;
            pWord->Base.wSubstitutionLen = 0;

            if (pWord->Base.wWordLen > pLingCmnInfo->Private.wMaxWordLength && !pWord->bHasPrimEditDist && !pWord->bEditDistFree) {
                ++pWord->bEditDistFree;
            }
        }
    }

    /* handle spc/free tracking */

    if (pWord->bEditDistSpc || pWord->bEditDistFree) {
        pLingCmnInfo->Private.bSpcDuringBuild = 1;
    }

    /* track inserts, before dups etc, mostly used as an indication of search "success" */

    if ((pLingCmnInfo->Private.wTotalWordInserts + 1) != 0) {
        ++pLingCmnInfo->Private.wTotalWordInserts;
    }

    if (GETBASESRC(pWord->bWordSrc) == ET9WORDSRC_LDB) {
        pLingCmnInfo->Private.bHasRealWord = 1;
    }

    /* update insert counter */

    __ET9AWSelLstIncInsertCounter;

    if (pLingCmnInfo->Private.bTotalSymbInputs) {

        if (!pLingCmnInfo->Private.pbLangSupported[pLingCmnInfo->Private.bTotalSymbInputs - 1] && pWord->Base.bLangIndex) {

            pLingCmnInfo->Private.pbLangSupported[pLingCmnInfo->Private.bTotalSymbInputs - 1] = pWord->Base.bLangIndex;
        }
        else if ((pLingCmnInfo->Private.pbLangSupported[pLingCmnInfo->Private.bTotalSymbInputs - 1] != ET9AWBOTH_LANGUAGES) &&
                 (pLingCmnInfo->Private.pbLangSupported[pLingCmnInfo->Private.bTotalSymbInputs - 1] != pWord->Base.bLangIndex) && pWord->Base.bLangIndex) {

            pLingCmnInfo->Private.pbLangSupported[pLingCmnInfo->Private.bTotalSymbInputs - 1] = ET9AWBOTH_LANGUAGES;
        }
    }

    /* assign primary edit distance */

    pWord->bHasPrimEditDist = pWord->bEditDistSpc > 0;

    /* truncation handling */

    if (wLeftWordLen) {

        ET9U16 wAvailableLen;

        if (wLeftWordLen >= ET9MAXWORDSIZE) {
            WLOG2(fprintf(pLogFile2, "too long left hand side (%d >= %d)\n", wLeftWordLen, ET9MAXWORDSIZE);)
            return ET9STATUS_ERROR;
        }

        wAvailableLen = ET9MAXWORDSIZE - wLeftWordLen;

        if (pWord->bHasPrimEditDist && pWord->Base.wWordLen > wAvailableLen) {
            WLOG2Word(pLogFile2, "discarding spell correction that would become truncated", pWord);
            return ET9STATUS_NONE;
        }

        if (pWord->Base.bIsTerm && pWord->Base.wWordLen > wAvailableLen) {
            WLOG2Word(pLogFile2, "term that would become truncated becomes a stem", pWord);
            pWord->Base.bIsTerm = 0;
        }
    }

    /* possibly remove completions */

    if (!bPreScreenedDupe &&
        !pWord->bHasPrimEditDist &&
        !pWord->bEditDistFree &&
        pWord->Base.wWordLen > pLingCmnInfo->Private.wMaxWordLength) {

        ET9AssertLog((pWord->Base.wWordLen - pWord->Base.wWordCompLen) == pLingCmnInfo->Private.wMaxWordLength);

        WLOG2B(fprintf(pLogFile2, "unwanted completion - into stem\n");)

        pWord->Base.wWordLen = pLingCmnInfo->Private.wMaxWordLength;
        pWord->Base.wWordCompLen = 0;
    }

    /* too short for lock? */

    /* ET9AssertLog(wLeftWordLen + pWord->Base.wWordLen >= wLockPoint); at some point this would be good to activate... */

    if (wLeftWordLen + pWord->Base.wWordLen < wLockPoint) {
        WLOG2Word(pLogFile2, "discarding candidate that would be shorter than lock", pWord);
        return ET9STATUS_NONE;
    }

    /* possibly discard stems */

    if (!pWord->Base.bIsTerm && !pWord->Base.wWordCompLen && !pLingCmnInfo->Private.bStemsAllowed) {

        pWord->bWordSrc = ET9WORDSRC_STEMPOOL;

        WLOG2B(fprintf(pLogFile2, "unwanted stem - into stem pool\n");)
    }

    /* clear substitution if this is a non-completed stem of an asdb shortcut */

    if (!pWord->Base.bIsTerm && !pWord->Base.wWordCompLen) {
        pWord->Base.wSubstitutionLen = 0;
    }

    /* validate */

    ET9AssertLog(!pWord->Base.bIsTerm || pWord->Base.bIsTerm && !pWord->Base.wWordCompLen);

#ifdef ET9_DEBUG
    {
        ET9U16 wCount;
        ET9SYMB *pSymb;
        ET9AWPrivWordInfo *pLeftHandWord = &pLingCmnInfo->Private.sLeftHandWord;

        ET9AssertLog(pWord->bWordSrc != ET9WORDSRC_NONE);
        ET9AssertLog(GETBASESRC(pWord->bWordSrc) != ET9WORDSRC_CAPTURE);

        ET9AssertLog(pWord->Base.wWordLen || pLeftHandWord->Base.wWordLen);

        ET9AssertLog(pWord->Base.wWordLen <= ET9MAXWORDSIZE);
        ET9AssertLog(pWord->Base.wWordCompLen <= pWord->Base.wWordLen);
        ET9AssertLog(pWord->Base.wSubstitutionLen <= ET9MAXSUBSTITUTIONSIZE);

        ET9AssertLog(pLeftHandWord->Base.wWordLen <= ET9MAXWORDSIZE);
        ET9AssertLog(pLeftHandWord->Base.wWordCompLen <= pLeftHandWord->Base.wWordLen);

        if (pWord->Base.wWordLen < ET9MAXWORDSIZE) {
            pWord->Base.sWord[pWord->Base.wWordLen] = 0;
        }

        wCount = pWord->Base.wWordLen;
        pSymb = pWord->Base.sWord;

        while (wCount--) {
            ET9AssertLog(*pSymb && *pSymb != (ET9SYMB)0xcccc);
            ++pSymb;
        }

        wCount = pLeftHandWord->Base.wWordLen;
        pSymb = pLeftHandWord->Base.sWord;

        while (wCount--) {
            ET9AssertLog(*pSymb && *pSymb != (ET9SYMB)0xcccc);
            ++pSymb;
        }
    }
#endif

    /* save some info for stem insertion, avoid copy the whole word (performance) */

#ifdef ET9_DEBUG
    _InitPrivWordInfo(&sStemWord);
#endif

    if (pWord->Base.wWordCompLen) {

        sStemWord.Base.wWordLen             = (ET9U16)(wLeftWordLen + pWord->Base.wWordLen - pWord->Base.wWordCompLen);
        sStemWord.Base.wWordCompLen         = 0;
        sStemWord.Base.wSubstitutionLen     = 0;
        sStemWord.Base.bIsTerm              = 0;
        sStemWord.bEditDistSpc              = 0;
        sStemWord.bEditDistStem             = 0;
        sStemWord.bEditDistFree             = 0;
        sStemWord.bHasPrimEditDist          = pWord->bHasPrimEditDist;
        sStemWord.bIsUDBWord                = pWord->bIsUDBWord;
        sStemWord.bWordSrc                  = (pLingCmnInfo->Private.bStemsAllowed && !pWord->bEditDistStem) ? ET9WORDSRC_STEM : ET9WORDSRC_STEMPOOL;
        sStemWord.xTotFreq                  = pWord->xTotFreq;
        sStemWord.xTapFreq                  = pWord->xTapFreq;
        sStemWord.xWordFreq                 = pWord->xWordFreq;
        sStemWord.wTWordFreq                = pWord->wTWordFreq;
        sStemWord.wEWordFreq                = pWord->wEWordFreq;
        sStemWord.dwWordIndex               = pWord->dwWordIndex;
        sStemWord.Base.bLangIndex           = pWord->Base.bLangIndex;
        sStemWord.bIsGroupBase              = 0;
        sStemWord.bGroupCount               = 0;
        sStemWord.bCDBTrigram               = pWord->bCDBTrigram;

        ET9AssertLog(sStemWord.xTapFreq);
        ET9AssertLog(sStemWord.xTotFreq);
        ET9AssertLog(sStemWord.xWordFreq);

    }
    else {
        sStemWord.bWordSrc                  = 0;    /* silly compiler warning... */
        sStemWord.Base.wWordLen             = 0;    /* silly compiler warning... */
    }

    /* update word source */

    if (bPreScreenedDupe) {
    }
    else if (bFreqIndicator == FREQ_BUILDAROUND) {
        if  (pWord->bWordSrc > ET9WORDSRC_SIMPLE_START &&
             pWord->bWordSrc < ET9WORDSRC_BUILDAROUND_START) {
             pWord->bWordSrc += BUILDAROUND_SRC_ADDON;
        }
    }
    else if (bFreqIndicator == FREQ_BUILDCOMPOUND) {
        if  (pWord->bWordSrc > ET9WORDSRC_SIMPLE_START &&
             pWord->bWordSrc < ET9WORDSRC_BUILDAROUND_START) {
             pWord->bWordSrc += BUILDCOMPOUND_SRC_ADDON;
        }
    }

    /* setup and save word designation */

    pWord->bWordDesignation = bFreqIndicator;

    /* prepend lefthandword */

    if (wLeftWordLen && !bPreScreenedDupe) {
        if (!__ET9PrependWord(pWord, &pLingCmnInfo->Private.sLeftHandWord, wLeftWordLen)) {
            return ET9STATUS_ERROR;
        }
    }

    /* update spc info - not just primary (after prepend) */

    pWord->Base.bIsSpellCorr = pWord->bEditDistSpc > 0;

    ET9AssertLog(pWord->bEditDistSpc && pWord->Base.bIsSpellCorr || !pWord->bEditDistSpc && !pWord->Base.bIsSpellCorr);
    ET9AssertLog(!pWord->Base.bIsSpellCorr || pLingCmnInfo->Private.bSpcDuringBuild);

    /* handle capslock and shift of prediction and capslock of completion */

    wLdbNum  = (pWord->Base.bLangIndex == ET9AWSECOND_LANGUAGE) ? pLingCmnInfo->wSecondLdbNum : pLingCmnInfo->wFirstLdbNum;

    if (pWord->Base.wWordCompLen && _ET9_LanguageSpecific_ApplyShifting(pLingInfo, pWord)) {

        if (pWord->Base.wWordCompLen == pWord->Base.wWordLen && /* prediction */
            pWordSymbInfo->Private.eLastShiftState == ET9SHIFT) {

            pWord->Base.sWord[0] = _ET9SymToUpper(pWord->Base.sWord[0], wLdbNum);
        }
        else if (pWordSymbInfo->Private.eLastShiftState == ET9CAPSLOCK) {

            ET9SYMB *psSymb;
            ET9UINT nComplLen;

            psSymb = &pWord->Base.sWord[pWord->Base.wWordLen - 1];
            nComplLen = pWord->Base.wWordCompLen;

            while (nComplLen--) {
                *psSymb = _ET9SymToUpper(*psSymb, wLdbNum);
                --psSymb;
            }
        }
    }

#if 0
    /* this code is unused uless we reinstate substitutions on prediction */

    /* Handle capslock and shift for prediction substitution */

    if (pWord->Base.wSubstitutionLen && (pWord->Base.wWordCompLen == pWord->Base.wWordLen )) {
        /* if all syms shifted, do same for substitution */
        pSymb = pWord->Base.sSubstitution;
        if (pAlpSelList->Private.pWordSymbInfo->Private.bLastShiftState == ET9CAPSLOCK) {
            nComplLen = pWord->Base.wSubstitutionLen;
            while (nComplLen--) {
                *pSymb = _SymToUpper(*pSymb);
                ++pSymb;
            }
        }
        /* if first sym shifted, do same for substitution */
        else if (pAlpSelList->Private.pWordSymbInfo->Private.bLastShiftState == ET9SHIFT) {
            *pSymb = _SymToUpper(*pSymb);
        }
    }
#endif

    /* on-the-fly (before __ET9AWIsLikeExactMatch & __ET9AWSelLstCheckDups) */

    if (pLingInfo->Private.pConvertSymb && !bPreScreenedDupe) {

        ET9STATUS wConvert;

        WLOG2BWord(pLogFile2, "_ET9AWSelLstInsert, before OTFM, pWord", pWord);

        wConvert = _ET9_ConvertBuildBuf(pLingInfo, &pWord->Base);

        /* host provided no conversion for a character, skip this word (unless it's the exact) */

        if (wConvert == ET9STATUS_NO_CHAR || wConvert == ET9STATUS_ERROR) {

            if (ISEXACTSRC(pWord->bWordSrc)) {
                WLOG2Word(pLogFile2, "host tried to prevent the exact to enter the list, not accepted...", pWord);
            }
            else {
                return ET9STATUS_ERROR;
            }
        }
    }

    __ET9AWSelLstDupeRecordWord(pWord);

#ifdef ET9_DEBUG

    /* verify locked chars */

    if (wLockPoint) {

        ET9INT              snCount;
        ET9INT              snIndex;
        ET9SYMB             *psSymb;
        ET9AWPrivWordInfo   *pWord1;
        ET9SymbInfo         *pSymbInfo;

        for (snIndex = 0; snIndex < pLingCmnInfo->Private.bTotalWords; ++snIndex) {

            pWord1 = &pLingCmnInfo->Private.pWordList[pLingCmnInfo->Private.bWordList[snIndex]];

            if (GETBASESRC(pWord1->bWordSrc) == ET9WORDSRC_MAGICSTRING) {
                continue;
            }

            if (pWord1->Base.wWordLen < wLockPoint) {
                WLOG2Word(pLogFile2, "word verified", pWord1);
                WLOG2(fprintf(pLogFile2, "word shorter than lock, index = %d\n", snIndex);)
            }

            ET9AssertLog(pWord1->Base.wWordLen >= wLockPoint);

            psSymb = pWord1->Base.sWord;
            pSymbInfo = pWordSymbInfo->SymbsInfo;

            for (snCount = wLockPoint; snCount; --snCount, ++psSymb, ++pSymbInfo) {

                ET9SYMB sLockedSymb = pSymbInfo->sLockedSymb;

                if (pLingInfo->Private.pConvertSymb != NULL) {
                    pLingInfo->Private.pConvertSymb(pLingInfo->Private.pConvertSymbInfo, &sLockedSymb);
                }

                /* this is going into the list, must be exact match to converted lock */

                if (*psSymb != sLockedSymb && !pLingCmnInfo->Private.bExpandAsDuringBuild) {
                    WLOG2Word(pLogFile2, "word verified", pWord1);
                    WLOG2(fprintf(pLogFile2, "lock symb missmatch index = %d, pos = %d, locked symb = %x\n", snIndex, (psSymb - pWord1->Base.sWord), sLockedSymb);)
                }

                ET9AssertLog(*psSymb == sLockedSymb || pLingCmnInfo->Private.bExpandAsDuringBuild);
            }
        }
    }
#endif

    /* all completions should also have the stem go into the selection list */

    if (pWord->Base.wWordCompLen &&
        (!pLingCmnInfo->Private.bHasRealWord || pLingCmnInfo->Private.bStemsAllowed) &&
        (sStemWord.bWordSrc != ET9WORDSRC_STEMPOOL || pLingCmnInfo->Private.bTotalWords < pLingCmnInfo->Private.bListSize) &&
        sStemWord.Base.wWordLen &&
        sStemWord.Base.wWordLen <= ET9MAXWORDSIZE) {

        /* at this point pWord already holds a properly prepended and converted word, can be used for dupe supression */

        ET9AWPrivWordInfo *pDupeWord;

        {
            ET9U16 wOldWordLen = pWord->Base.wWordLen;

            pWord->Base.wWordLen = sStemWord.Base.wWordLen;

            __ET9AWIsWordInList(pLingCmnInfo, pWord, &pDupeWord);

            pWord->Base.wWordLen = wOldWordLen;
        }

        if (pDupeWord != NULL) {

            if (pDupeWord->bWordQuality <= DISPOSABLE_QUALITY) {

                if (sStemWord.bWordSrc != ET9WORDSRC_STEMPOOL || pLingCmnInfo->Private.bStemsAllowed) {

                    pDupeWord->bWordQuality = STEM_QUALITY;
                }
            }
        }
        else {

            switch (sStemWord.bWordSrc)
            {
                case ET9WORDSRC_STEMPOOL:
                    if (pLingCmnInfo->Private.bStemsAllowed) {
                        sStemWord.bWordQuality = STEM_QUALITY;
                    }
                    else {
                        sStemWord.bWordQuality = DISPOSABLE_QUALITY;
                    }
                    break;
                default:
                    sStemWord.bWordQuality = SHAPED_QUALITY;
                    break;
            }

            /* pWord holds converted, prepended etc symbs, good to go directly... */

            _ET9SymCopy(sStemWord.Base.sWord, pWord->Base.sWord, sStemWord.Base.wWordLen);

            WLOG2BWord(pLogFile2, "_ET9AWSelLstInsert, attempting completion's stem", &sStemWord);

            __ET9AWSelLstInsert(pLingInfo, &sStemWord, bFreqIndicator, 1);

            WLOG2B(fprintf(pLogFile2, "_ET9AWSelLstInsert, done with stem attempt\n");)
        }
    }

    /* check if the substitution is the same as the shortcut (also without OTFM to be sure it never happens)) */

    if (pWord->Base.wSubstitutionLen == pWord->Base.wWordLen) {

        ET9U16              wCmpLen;
        ET9SYMB             *pStr1;
        ET9SYMB             *pStr2;

        wCmpLen = pWord->Base.wWordLen;
        pStr1 = pWord->Base.sWord;
        pStr2 = pWord->Base.sSubstitution;
        ++wCmpLen;
        while (--wCmpLen) {
            if (*pStr1++ != *pStr2++) {
                break;
            }
        }

        if (!wCmpLen) {

            WLOG2Word(pLogFile2, "word will be rejected, substitution same as shortcut", pWord);

            return ET9STATUS_NONE;
        }
    }

    /* duplicate suppression */

    if (!bPreScreenedDupe) {

        wStatus =  __ET9AWSelLstCheckDups(pLingInfo, pWord);

        if (wStatus) {

            __ET9AWSelLstIncDuplicateCounter;
            __ET9AWSelLstDupeComment(L" (DUPE)\n");

            return wStatus;
        }
    }

    /* post rejection of special cases */
    /* reject spc words that ends in punct when the symb info ends in punct */

    if (pWordSymbInfo->bNumSymbs &&
        pWord->bHasPrimEditDist &&
        _ET9_IsPunctChar(pWord->Base.sWord[pWord->Base.wWordLen - 1])) {

        ET9U16  wLastSymbIndex = pWordSymbInfo->bNumSymbs - 1;
        ET9U8   bSymbType = pWordSymbInfo->SymbsInfo[wLastSymbIndex].bSymbType;

        if (bSymbType == ET9KTSMARTPUNCT || bSymbType == ET9KTPUNCTUATION) {
            WLOG2Word(pLogFile2, "__ET9PostWordRejection, spc punct end", pWord);
            --pLingCmnInfo->Private.wTotalWordInserts;     /* will never become zero, that's the important part*/
            __ET9AWSelLstIncDiscardedCounter;
            __ET9AWSelLstDupeComment(L" (POST WORD REJECT)\n");
            return ET9STATUS_NONE;
        }
    }

    /* -------------------- actual insert attempt --------------------------*/

    /* validation
         - protected sources have neither completion nor distance nor stem distance */

    WLOG2Word(pLogFile2, "_ET9AWSelLstInsert, attempting, pWord", pWord);

    ET9AssertLog(pWord->Base.bIsTerm != 0xc);
    ET9AssertLog(pWord->Base.bIsTerm != 0xcc);
    ET9AssertLog(!ISPROTECTEDSRC(pWord->bWordSrc) || pWord->bWordSrc == ET9WORDSRC_STEM || !pWord->bEditDistSpc && !pWord->bEditDistStem && pWord->Base.bIsTerm);

    if (pLingCmnInfo->Private.bTotalSymbInputs) {
        if (!pLingCmnInfo->Private.pbLangSupported[pLingCmnInfo->Private.bTotalSymbInputs - 1] &&
            pWord->Base.bLangIndex) {

            pLingCmnInfo->Private.pbLangSupported[pLingCmnInfo->Private.bTotalSymbInputs - 1] = pWord->Base.bLangIndex;
        }
        else if ((pLingCmnInfo->Private.pbLangSupported[pLingCmnInfo->Private.bTotalSymbInputs - 1] != ET9AWBOTH_LANGUAGES) &&
                 (pLingCmnInfo->Private.pbLangSupported[pLingCmnInfo->Private.bTotalSymbInputs - 1] != pWord->Base.bLangIndex) &&
                 pWord->Base.bLangIndex) {

            pLingCmnInfo->Private.pbLangSupported[pLingCmnInfo->Private.bTotalSymbInputs - 1] = ET9AWBOTH_LANGUAGES;
        }
    }

    /* flags */

    if (pWord->bHasPrimEditDist && pWord->Base.wWordCompLen) {
        pLingCmnInfo->Private.bSpcComplDuringSingleBuild = 1;
    }

    /* try adding it to the list, assume validations above */

    if (pWord->bHasPrimEditDist && pWord->Base.wWordCompLen && pLingCmnInfo->Private.bTotalSpcCmplWords >= pLingCmnInfo->Private.ASpc.wMaxSpcCmplCount) {

        /* spc cmpl word and spc cmpl part of list is full, discard or replace */

        if (!pLingCmnInfo->Private.bSpcComplDuringSingleBuild) {
            ET9AssertLog(pLingCmnInfo->Private.bTotalSpcCmplWords == pLingCmnInfo->Private.ASpc.wMaxSpcCmplCount);
        }

        if (!pLingCmnInfo->Private.pLastSpcCmplWord) {

            i = (ET9S16)pLingCmnInfo->Private.bTotalWords;
            pAmbigWord = &pLingCmnInfo->Private.pWordList[i-1];

            for (; i; --i, --pAmbigWord) {

                if (!(pAmbigWord->bHasPrimEditDist && pAmbigWord->Base.wWordCompLen)) {
                    continue;
                }

                if (!pLingCmnInfo->Private.pLastSpcCmplWord ||
                    __PriorityCompare(pLingCmnInfo, pLingCmnInfo->Private.pLastSpcCmplWord, pAmbigWord, 0) > 0) {

                    ET9AssertLog(!ISPROTECTEDSRC(pAmbigWord->bWordSrc));

                    pLingCmnInfo->Private.pLastSpcCmplWord = pAmbigWord;
                }
            }

            ET9AssertLog(pLingCmnInfo->Private.pLastSpcCmplWord);
            ET9AssertLog(!ISPROTECTEDSRC(pLingCmnInfo->Private.pLastSpcCmplWord->bWordSrc));
        }

        if (__PriorityCompare(pLingCmnInfo, pLingCmnInfo->Private.pLastSpcCmplWord, pWord, 0) > 0) {

            ET9AssertLog(!ISPROTECTEDSRC(pWord->bWordSrc));

            __ET9AWSelLstIncDiscardedCounter;
            __ET9AWSelLstDupeComment(L" (PRIORITY DISCARD)\n");

            return wStatus;
        }

        __ET9AWSelLstIncReplacingCounter;

        __ET9ReplaceAndUpdateInsertCounters(pLingCmnInfo, pLingCmnInfo->Private.pLastSpcCmplWord, pWord);
    }
    else if (pWord->bHasPrimEditDist && pLingCmnInfo->Private.bTotalSpcTermWords >= pLingCmnInfo->Private.ASpc.wMaxSpcTermCount) {

        /* spc term word and spc term part of list is full, discard or replace */

        if (!pLingCmnInfo->Private.bSpcComplDuringSingleBuild) {
            ET9AssertLog(pLingCmnInfo->Private.bTotalSpcTermWords == pLingCmnInfo->Private.ASpc.wMaxSpcTermCount);
        }

        if (!pLingCmnInfo->Private.pLastSpcTermWord) {

            i = (ET9S16)pLingCmnInfo->Private.bTotalWords;
            pAmbigWord = &pLingCmnInfo->Private.pWordList[i-1];

            for (; i; --i, --pAmbigWord) {

                if (!(pAmbigWord->bHasPrimEditDist && !pAmbigWord->Base.wWordCompLen)) {
                    continue;
                }

                if (!pLingCmnInfo->Private.pLastSpcTermWord ||
                    __PriorityCompare(pLingCmnInfo, pLingCmnInfo->Private.pLastSpcTermWord, pAmbigWord, 0) > 0) {

                    ET9AssertLog(!ISPROTECTEDSRC(pAmbigWord->bWordSrc));

                    pLingCmnInfo->Private.pLastSpcTermWord = pAmbigWord;
                }
            }

            ET9AssertLog(pLingCmnInfo->Private.pLastSpcTermWord);
            ET9AssertLog(!ISPROTECTEDSRC(pLingCmnInfo->Private.pLastSpcTermWord->bWordSrc));

            if (!pLingCmnInfo->Private.pLastSpcTermWord) {
                return ET9STATUS_ERROR;
            }
        }

        if (__PriorityCompare(pLingCmnInfo, pLingCmnInfo->Private.pLastSpcTermWord, pWord, 0) > 0) {

            ET9AssertLog(!ISPROTECTEDSRC(pWord->bWordSrc));

            __ET9AWSelLstIncDiscardedCounter;
            __ET9AWSelLstDupeComment(L" (PRIORITY DISCARD)\n");

            return wStatus;
        }

        __ET9AWSelLstIncReplacingCounter;

        __ET9ReplaceAndUpdateInsertCounters(pLingCmnInfo, pLingCmnInfo->Private.pLastSpcTermWord, pWord);
    }
    else if (pWord->Base.wWordCompLen && pLingCmnInfo->Private.bTotalCompletionWords >= pLingCmnInfo->Private.wMaxCompletionCount) {

        /* completion word and completion part of list is full, discard or replace */

        if (!pLingCmnInfo->Private.bSpcComplDuringSingleBuild) {
            ET9AssertLog(pLingCmnInfo->Private.bTotalCompletionWords == pLingCmnInfo->Private.wMaxCompletionCount);
        }

        if (!pLingCmnInfo->Private.pLastCompletionWord) {

            i = (ET9S16)pLingCmnInfo->Private.bTotalWords;
            pAmbigWord = &pLingCmnInfo->Private.pWordList[i-1];

            for (; i; --i, --pAmbigWord) {

                if (!(!pAmbigWord->bHasPrimEditDist && pAmbigWord->Base.wWordCompLen)) {
                    continue;
                }

                if (!pLingCmnInfo->Private.pLastCompletionWord ||
                    __PriorityCompare(pLingCmnInfo, pLingCmnInfo->Private.pLastCompletionWord, pAmbigWord, 0) > 0) {

                    ET9AssertLog(!ISPROTECTEDSRC(pAmbigWord->bWordSrc));

                    pLingCmnInfo->Private.pLastCompletionWord = pAmbigWord;
                }
            }

            ET9AssertLog(pLingCmnInfo->Private.pLastCompletionWord);
            ET9AssertLog(!ISPROTECTEDSRC(pLingCmnInfo->Private.pLastCompletionWord->bWordSrc));

            if (!pLingCmnInfo->Private.pLastCompletionWord) {
                return ET9STATUS_ERROR;
            }
        }

        if (__PriorityCompare(pLingCmnInfo, pLingCmnInfo->Private.pLastCompletionWord, pWord, 0) > 0) {

            ET9AssertLog(!ISPROTECTEDSRC(pWord->bWordSrc));

            __ET9AWSelLstIncDiscardedCounter;
            __ET9AWSelLstDupeComment(L" (PRIORITY DISCARD)\n");

            return wStatus;
        }

        __ET9AWSelLstIncReplacingCounter;

        __ET9ReplaceAndUpdateInsertCounters(pLingCmnInfo, pLingCmnInfo->Private.pLastCompletionWord, pWord);
    }
    else if (pLingCmnInfo->Private.bTotalWords < pLingCmnInfo->Private.bListSize) {

        /* list is not full, get next empty slot */

        ET9AssertLog(!pLingCmnInfo->Private.bTotalWords ||
                     pLingCmnInfo->Private.pWordList[pLingCmnInfo->Private.bTotalWords - 1].Base.wWordLen > 0);
        ET9AssertLog(pLingCmnInfo->Private.bTotalWords == pLingCmnInfo->Private.bListSize ||
                     pLingCmnInfo->Private.pWordList[pLingCmnInfo->Private.bTotalWords].Base.wWordLen == 0);

        pLingCmnInfo->Private.pWordList[pLingCmnInfo->Private.bTotalWords] = *pWord;

        ++pLingCmnInfo->Private.bTotalWords;

        if (pWord->bHasPrimEditDist && pWord->Base.wWordCompLen) {
            ++pLingCmnInfo->Private.bTotalSpcCmplWords;
        }
        else if (pWord->Base.wWordCompLen) {
            ++pLingCmnInfo->Private.bTotalCompletionWords;
        }
        else if (pWord->bHasPrimEditDist) {
            ++pLingCmnInfo->Private.bTotalSpcTermWords;
        }

        pLingCmnInfo->Private.snLinSearchCount = 0;
    }
    else {

        /* list is full, discard or replace */

        ET9AssertLog(pLingCmnInfo->Private.bTotalWords == pLingCmnInfo->Private.bListSize);

        if (!pLingCmnInfo->Private.pLastWord) {

            pLingCmnInfo->Private.pLastWord = NULL;
            i = (ET9S16)pLingCmnInfo->Private.bTotalWords;
            pAmbigWord = &pLingCmnInfo->Private.pWordList[i-1];

            for (; i; --i, --pAmbigWord) {
                if (!pLingCmnInfo->Private.pLastWord  || __PriorityCompare(pLingCmnInfo, pLingCmnInfo->Private.pLastWord, pAmbigWord, 0) > 0) {
                    if (!ISPROTECTEDSRC(pAmbigWord->bWordSrc)) {
                        pLingCmnInfo->Private.pLastWord = pAmbigWord;
                    }
                }
            }

            if (!pLingCmnInfo->Private.pLastWord) {

                if (!ISPROTECTEDSRC(pWord->bWordSrc)) {

                    __ET9AWSelLstIncDiscardedCounter;
                    __ET9AWSelLstDupeComment(L" (PRIORITY DISCARD)\n");

                    return wStatus;
                }

                i = (ET9S16)pLingCmnInfo->Private.bTotalWords;
                pAmbigWord = &pLingCmnInfo->Private.pWordList[i-1];

                for (; i; --i, --pAmbigWord) {
                    if (!pLingCmnInfo->Private.pLastWord || __PriorityCompare(pLingCmnInfo, pLingCmnInfo->Private.pLastWord, pAmbigWord, 0) > 0) {
                        pLingCmnInfo->Private.pLastWord = pAmbigWord;
                    }
                }
            }

            ET9AssertLog(pLingCmnInfo->Private.pLastWord);

            if (!pLingCmnInfo->Private.pLastWord) {
                return ET9STATUS_ERROR;
            }
        }

        if (ISPROTECTEDSRC(pWord->bWordSrc) && !ISPROTECTEDSRC(pLingCmnInfo->Private.pLastWord->bWordSrc)) {

            /* protected wins over non protected ones, except for protected stems that only wins over completions */

            if (GETBASESRC(pWord->bWordSrc) == ET9WORDSRC_STEM) {

                if (!pLingCmnInfo->Private.pLastCompletionWord) {

                    i = (ET9S16)pLingCmnInfo->Private.bTotalWords;
                    pAmbigWord = &pLingCmnInfo->Private.pWordList[i-1];

                    for (; i; --i, --pAmbigWord) {

                        if (!pAmbigWord->Base.wWordCompLen) {
                            continue;
                        }

                        if (!pLingCmnInfo->Private.pLastCompletionWord ||
                            __PriorityCompare(pLingCmnInfo, pLingCmnInfo->Private.pLastCompletionWord, pAmbigWord, 0) > 0) {

                            ET9AssertLog(!ISPROTECTEDSRC(pAmbigWord->bWordSrc));

                            pLingCmnInfo->Private.pLastCompletionWord = pAmbigWord;
                        }
                    }

                    ET9AssertLog(!pLingCmnInfo->Private.pLastCompletionWord || !ISPROTECTEDSRC(pLingCmnInfo->Private.pLastCompletionWord->bWordSrc));
                }

                if (pLingCmnInfo->Private.pLastCompletionWord) {

                    /* the lowest completion will be replaced, this path is working on pLastWord, needs to assign it */

                    pLingCmnInfo->Private.pLastWord = pLingCmnInfo->Private.pLastCompletionWord;
                }
                else {

                    /* nothing to replace, discard */

                    __ET9AWSelLstIncDiscardedCounter;
                    __ET9AWSelLstDupeComment(L" (PRIORITY DISCARD)\n");
                    return wStatus;
                }
            }
        }
        else if (__PriorityCompare(pLingCmnInfo, pLingCmnInfo->Private.pLastWord, pWord, 0) > 0) {

            __ET9AWSelLstIncDiscardedCounter;
            __ET9AWSelLstDupeComment(L" (PRIORITY DISCARD)\n");
            return wStatus;
        }

        __ET9AWSelLstIncReplacingCounter;

        __ET9ReplaceAndUpdateInsertCounters(pLingCmnInfo, pLingCmnInfo->Private.pLastWord, pWord);
    }

    __ET9AWSelLstDupeComment(L"\n");

    __VerifyWordCounters(pLingCmnInfo);

    return wStatus;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * This function adds a word to selection list.
 * It will call _ET9AWSelLstInsert in the end, but first set up some values in the priv-word part.
 *
 * @param pLingInfo                 Pointer to alphabetic information structure.
 * @param pWord                     Word to add.
 * @param wLength                   Active input length (can be partial).
 * @param bFreqIndicator            Designation
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL _ET9AWSelLstAdd(ET9AWLingInfo              * const pLingInfo,
                                     ET9AWPrivWordInfo          * const pWord,
                                     const ET9U16                       wLength,
                                     const ET9_FREQ_DESIGNATION         bFreqIndicator)
{
    ET9AWLingCmnInfo * const pLingCmnInfo = pLingInfo->pLingCmnInfo;

    const ET9BOOL bTraceBuild = pLingCmnInfo->Private.bTraceBuild;

    ET9AssertLog(pWord);
    ET9AssertLog(pLingInfo);
    ET9AssertLog(pWord->xTapFreq >= 0);
    ET9AssertLog(pWord->xWordFreq >= 0);

    if (pWord->xWordFreq == 0) {
        pWord->xWordFreq = 1;
    }

    /* compLen is set in Add and in preAdd... */

    if (pWord->Base.wWordCompLen > ET9MAXWORDSIZE) {
        /* keep pre-calculated value of zero */
        pWord->Base.wWordCompLen = 0;
    }
    else if (pWord->Base.wWordCompLen && pWord->Base.wWordCompLen < pWord->Base.wWordLen) {
        /* keep pre-calculated value */
    }
    else if (pWord->bEditDistSpc || pWord->Base.wWordLen < wLength) {
        pWord->Base.wWordCompLen = 0;
    }
    else {
        pWord->Base.wWordCompLen = (ET9U16)(pWord->Base.wWordLen - wLength);
    }

    /* assign term attribute */

    pWord->Base.bIsTerm = pWord->Base.wWordCompLen ? 0 : 1;

    /* modify tap freq */

    if (pLingCmnInfo->Private.eCurrSelectionListMode == ET9ASLMODE_MIXED) {

        ET9ASPCFlexCompareData * const pCMP = &pLingCmnInfo->Private.ASpc.u.sCmpDataFlex;

        const ET9BOOL bUsingFlex = (ET9BOOL)pLingCmnInfo->Private.ASpc.bSpcFeatures;
        const ET9UINT nKeyCount = wLength;
        const ET9UINT nQualityCount = bUsingFlex ? pCMP->nQualityCount : nKeyCount;

        ET9FLOAT fCost = 1;
        ET9FLOAT fBoost = 1;

#if 1
        if (pWord->bEditDistStem) {
            fCost += 10.0f * (pWord->bEditDistStem + pWord->bEditDistSpcTrp);
        }

        if (pWord->bEditDistSpc) {

            const ET9BOOL bRegionalSpc = (pLingCmnInfo->Private.ASpc.eMode == ET9ASPCMODE_REGIONAL) ? 1 : 0;

            fCost += ((bRegionalSpc || bTraceBuild) ? 200.0f : 50.0f) * (pWord->bEditDistSpc - pWord->bEditDistSpcTrp);
        }
#else
        fCost += 2.0f * pWord->bEditDistStem;
        fCost += 2.0f * pWord->bEditDistSpc;
#endif

        if (pWord->bEditDistFree) {
            if (bTraceBuild) {
                fCost += 1;
            }
        }

        if (pWord->Base.wWordCompLen) {

#if 1
            const ET9FLOAT fCompLenValue = (ET9FLOAT)(pWord->Base.wWordCompLen + 1) / 2.0f;

            fCost += (pWord->bEditDistSpc ? 3.0f : 2.0f) * fCompLenValue;
#else
            fCost += pWord->bEditDistSpc ? 3 : 2;
#endif

            if (bTraceBuild && !ET9DEVSTATEINHBTRACECMPLDEMOTE_MODE(pLingCmnInfo->Private.dwDevStateBits)) {
                fCost += 1000;
            }
        }

#if 0
        fCost *= fCost;
#endif

        if (bTraceBuild) {

            const ET9U8 bSrc = GETBASESRC(pWord->bWordSrc);

            if ((pWord->Base.wWordLen == 1 && (bSrc == ET9WORDSRC_LAS_SHORTCUT ||
                                               bSrc == ET9WORDSRC_ASDB_SHORTCUT))) {

                fBoost *= 2;
            }
        }
        else if (nQualityCount == nKeyCount) {

            const ET9U8 bSrc = GETBASESRC(pWord->bWordSrc);

            if (bSrc == ET9WORDSRC_LAS_SHORTCUT || bSrc == ET9WORDSRC_ASDB_SHORTCUT) {
                if (pWord->Base.wWordLen == 1) {
                    fBoost *= 10;
                }
            }
#if 1
            else if ((ET9UINT)(pWord->Base.wWordLen - pWord->bEditDistFree) == nQualityCount) {
                fBoost *= 10;
            }
#endif
        }

        WLOG2B(fprintf(pLogFile2, "_ET9AWSelLstAdd, nBoost %5.1f, nCost = %5.1f, xTapFreq = %5.1f, final  = %5.1f\n", fBoost, fCost, (float)pWord->xTapFreq, ((pWord->xTapFreq * fBoost) / fCost));)

        pWord->xTapFreq = (ET9FREQPART)((pWord->xTapFreq * fBoost) / fCost);
    }
    else if (bTraceBuild) {

        if (pWord->bEditDistFree) {
            ++pWord->bEditDistStem;
        }

        pWord->xTapFreq /= pWord->bEditDistStem + 1;
    }

    if (!pWord->xTapFreq) {
        pWord->xTapFreq = 1;
    }

    /* assign tot freq */

    pWord->xTotFreq = (ET9FREQ)(pWord->xTapFreq * pWord->xWordFreq);

    /* validate */

    ET9AssertLog(pWord->xTapFreq >= 0);
    ET9AssertLog(pWord->xWordFreq >= 0);
    ET9AssertLog(pWord->xTotFreq >= 0);

    /* actual list insert */

    return __ET9AWSelLstInsert(pLingInfo, pWord, bFreqIndicator, 0);
}

/*---------------------------------------------------------------------------*/
/** \internal
 * This function supports edit distance calculation by matching and fetching frequencies etc.
 *
 * @param pLingInfo                 Pointer to alphabetic information structure.
 * @param snIndex                   Index in symb info.
 * @param sSymb                     Symbol to match.
 * @param bExactMatchOnly           If only exact matching should be done.
 * @param pbMatch                   Match result.
 * @param pbFreq                    Frequency for the symbol.
 * @param pbLocked                  Lock for the symbol.
 * @param wLdbId                    Relevant LDB ID.
 *
 * @return None
 */

static void  ET9LOCALCALL __GetCodeFreqStd (ET9AWLingInfo   * const pLingInfo,
                                            const ET9INT            snIndex,
                                            const ET9SYMB           sSymb,
                                            const ET9BOOL           bExactMatchOnly,
                                            ET9U8           * const pbMatch,
                                            ET9U8           * const pbFreq,
                                            ET9U8           * const pbLocked,
                                            ET9U16                  wLdbId)
{
    ET9INT             i;
    ET9INT             snLockCompare;
    ET9INT             snFirstLevelExact;
    ET9SymbInfo        *pSymbInfo;
    ET9DataPerBaseSym  *pSymData;
    ET9SYMB            *pLower;
    ET9SYMB            *pUpper;

    ET9WordSymbInfo * const pWordSymbInfo = pLingInfo->pLingCmnInfo->Base.pWordSymbInfo;

    __STAT_INC_CmpOther;

#if 0
    WLOG2(fprintf(pLogFile2, "__GetCodeFreqStd, snIndex = %d, sSymb = %x\n", snIndex, (int)sSymb);)
#endif

    ET9AssertLog(pLingInfo != NULL);
    ET9AssertLog(pbMatch != NULL);
    ET9AssertLog(pbFreq != NULL);
    ET9AssertLog(pbLocked != NULL);
    ET9AssertLog(pWordSymbInfo != NULL);
    ET9AssertLog(snIndex < pWordSymbInfo->bNumSymbs);

    /* assign a proper default value, especially for "no match" */

    *pbFreq = 0xcc;

    /* this is not supposed to happen, just for extra safety in "release" mode, should be after the assert above */

    if (snIndex >= pWordSymbInfo->bNumSymbs) {
        *pbLocked = 0;
        *pbMatch = ET9_SPC_ED_MATCH_NONE;
        return;
    }

    /* search */

    *pbLocked = ET9_SPC_IS_LOCKED_POS(&pWordSymbInfo->SymbsInfo[snIndex]);

    if (pWordSymbInfo->SymbsInfo[snIndex].bSymbType == ET9KTSMARTPUNCT) {

        snFirstLevelExact = 0;
        *pbMatch = ET9_SPC_ED_MATCH_EXACT;
    }
    else {
        switch (pWordSymbInfo->SymbsInfo[snIndex].eInputType)
        {
            case ET9DISCRETEKEY:
            case ET9CUSTOMSET:
            case ET9MULTITAPKEY:
                snFirstLevelExact = 0;
                *pbMatch = ET9_SPC_ED_MATCH_EXACT;
                break;
            default:
                snFirstLevelExact = 1;
                *pbMatch = ET9_SPC_ED_MATCH_EXACT;
                break;
        }
    }

    snLockCompare = 0;

    i = pWordSymbInfo->bNumSymbs - snIndex;
    pSymbInfo = &pWordSymbInfo->SymbsInfo[snIndex];

    while (i--) {
        if (pSymbInfo->bLocked) {
            snLockCompare = 1;
            break;
        }
        ++pSymbInfo;
    }

    pSymbInfo = &pWordSymbInfo->SymbsInfo[snIndex];

    if (snLockCompare) {

        const ET9SYMB sLockedSymb = pSymbInfo->sLockedSymb;

        *pbLocked = ET9_LOCKED_SYMB_VALUE_AND_POS;

        if (sSymb == sLockedSymb || sSymb == _ET9SymToOther(sLockedSymb, wLdbId)) {
            *pbFreq = 1;
            *pbMatch = ET9_SPC_ED_MATCH_EXACT;
            return;
        }
    }
    else {

        ET9INT snKeyIndex;

        pSymData = pSymbInfo->DataPerBaseSym;
        snKeyIndex = (ET9INT)((bExactMatchOnly && snFirstLevelExact) ? 1 : pSymbInfo->bNumBaseSyms);

        while (snKeyIndex--) {

            ET9INT k;

            k = pSymData->bNumSymsToMatch;
            pLower = pSymData->sChar;
            pUpper = pSymData->sUpperCaseChar;

            while (k--) {
                if (sSymb == *pLower++ || sSymb == *pUpper++) {
                    *pbFreq = pSymData->bSymFreq;
                    return;
                }
            }

            ++pSymData;
            if (snFirstLevelExact) {
                *pbMatch = ET9_SPC_ED_MATCH_FULL;
            }
        }
    }

    *pbMatch = ET9_SPC_ED_MATCH_NONE;
}

/*---------------------------------------------------------------------------
 * undef's and control defs for edit distance calculation
 *---------------------------------------------------------------------------*/

#undef ET9_SPC_ED_GetMatchOnly
#undef ET9_SPC_ED_GetMatchInfo
#undef ET9_SPC_ED_GetWordSymb
#undef ET9_SPC_ED_USE_COMPARE_CACHE
#undef __ET9AWCalcEditDistance

#define ET9_SPC_ED_USE_COMPARE_CACHE

/*---------------------------------------------------------------------------*/
/** \internal
 * Get match info, plus frequency and lock info.
 * Macro for adopting the "source" to __ET9AWCalcEditDistance.
 *
 * @param index1                    Index in source string (input).
 * @param index2                    Index in compared string (source word).
 * @param bExactMatchOnly           If only to perform exact match.
 * @param pbMatch                   (out) the resulting match info.
 * @param pbFreq                    (out) the resulting match frequency.
 * @param pbLocked                  (out) lock information.
 *
 * @return None
 */

#define ET9_SPC_ED_GetMatchInfo(index1, index2, bExactMatchOnly, pbMatch, pbFreq, pbLocked)             \
(                                                                                                       \
    __GetCodeFreqStd(pLingInfo,                                                                         \
                     ((index1) + (wIndex)),                                                             \
                     (pWord->Base.sWord[index2]),                                                       \
                     (bExactMatchOnly),                                                                 \
                     (&pbMatch),                                                                        \
                     (&pbFreq),                                                                         \
                     (&pbLocked),                                                                       \
                     ((pWord->Base.bLangIndex == ET9AWSECOND_LANGUAGE) ? pLingInfo->pLingCmnInfo->wSecondLdbNum : pLingInfo->pLingCmnInfo->wFirstLdbNum)) \
)

/*---------------------------------------------------------------------------*/
/** \internal
 * Get word symbol (character in a word).
 * Macro for adopting the "source" to __ET9AWCalcEditDistance.
 *
 * @param index                     Index in compared string (source word).
 * @param psSymb                    (out) the symbol.
 *
 * @return None
 */

#define ET9_SPC_ED_GetWordSymb(index, psSymb)                                                           \
{                                                                                                       \
    (psSymb) = pWord->Base.sWord[index];                                                                \
}                                                                                                       \

/*---------------------------------------------------------------------------*/
/** \internal
 * Function (name) for edit distance calculation for direct use against the input symbols (non LDB sources).
 * The "include" will bring in a static function making use of the other source adopting macros.<br>
 * This macro really sets up the name of the __ET9AWCalcEditDistance function for direct use against the input symbols.<br>
 * The name change is necessary since the function will be instantiated in more than one place
 * and it's possible that all of them will be compiled as one big C file.<br>
 * This way of doing things is necessary for reasons of performance and code management.
 */

#define __ET9AWCalcEditDistance __ET9AWCalcEditDistanceASLST

#include "et9aspci.h"

/*---------------------------------------------------------------------------*/
/** \internal
 * Get the character for a position in the search "word".
 * This exist in order to make the code more data source independent.
 *
 * @param pWord                     Pointer to the word.
 * @param nPos                      Input position (0 - n).
 *
 * @return Code value.
 */

ET9INLINE static ET9SYMB ET9LOCALCALL __AdbGetCharAtPos(ET9AWPrivWordInfo     const * const pWord,
                                                        const ET9UINT                       nPos)
{
    return pWord->Base.sWord[nPos];
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Get symb frequency.
 * This exist in order to make the code more data source independent.
 *
 * @param pLingCmnInfo              Pointer to alphabetic cmn information structure.
 * @param nPos                      Input position (0 - n).
 * @param sSymb                     Symb to get tap frequency for.
 * @param pbExactMatch              If the match is considered exact (then non zero).
 *
 * @return Tap frequency.
 */

ET9INLINE static ET9U8 ET9LOCALCALL __GetSymbFreq(ET9AWLingCmnInfo    * const pLingCmnInfo,
                                                  const ET9UINT               nPos,
                                                  const ET9SYMB               sSymb,
                                                  ET9BOOL             * const pbExactMatch)
{
    ET9ASPCFlexCompareData  * const pCMP = &pLingCmnInfo->Private.ASpc.u.sCmpDataFlex;
    ET9SymbInfo  const * const pSymbInfo = &pCMP->pFirstSymb[nPos];

    ET9UINT nBaseCount;
    ET9DataPerBaseSym   const *pDPBS;

    if (pCMP->psLockSymb[nPos]) {
        if (sSymb == pCMP->psLockSymb[nPos] || sSymb == pCMP->psLockSymbOther[nPos]) {
            if (pbExactMatch) {
                *pbExactMatch = 1;
            }
            return 1;
        }
        else {
            return 0;
        }
    }

    pDPBS = pSymbInfo->DataPerBaseSym;
    for (nBaseCount = pSymbInfo->bNumBaseSyms; nBaseCount ; --nBaseCount, ++pDPBS) {

        ET9SYMB const * pLower;
        ET9SYMB const * pUpper;
        ET9UINT nSymsCount;

        pLower = pDPBS->sChar;
        pUpper = pDPBS->sUpperCaseChar;

        for (nSymsCount = pDPBS->bNumSymsToMatch; nSymsCount ; --nSymsCount, ++pLower, ++pUpper) {

            if (sSymb == *pLower || sSymb == *pUpper) {

                ET9U8 bSymFreq;

                bSymFreq = pDPBS->bSymFreq;

                if (!bSymFreq) {
                    bSymFreq = 1;
                }

                if (pbExactMatch) {
                    *pbExactMatch = (ET9BOOL)(pDPBS == pSymbInfo->DataPerBaseSym ||
                                              pSymbInfo->eInputType == ET9DISCRETEKEY ||
                                              pSymbInfo->eInputType == ET9MULTITAPKEY ||
                                              pSymbInfo->eInputType == ET9CUSTOMSET ||
                                              pSymbInfo->bSymbType == ET9KTSMARTPUNCT);
                }

                return bSymFreq;
            }
        }
    }

    return 0;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * This function performs the flex edit distance calculation for a given word.
 *
 * @param pLingInfo                 Pointer to alphabetic information structure.
 * @param pWord                     Pointer to the word.
 * @param wIndex                    Input index/position (non zero if partial).
 * @param wLength                   Word length (active input length).
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

static ET9STATUS ET9LOCALCALL __ADB_CalcEditDistanceFlex(ET9AWLingInfo     * const pLingInfo,
                                                         ET9AWPrivWordInfo * const pWord,
                                                         const ET9U16              wIndex,
                                                         const ET9U16              wLength)
{
    ET9AWLingCmnInfo        * const pLingCmnInfo    = pLingInfo->pLingCmnInfo;
    ET9WordSymbInfo         * const pWordSymbInfo   = pLingCmnInfo->Base.pWordSymbInfo;
    ET9ASPCFlexCompareData  * const pCMP            = &pLingCmnInfo->Private.ASpc.u.sCmpDataFlex;

    ET9U8             const * const pbLockInfo      = pCMP->pbLockInfo;
    ET9U8             const * const pbIsFreqPos     = pCMP->pbIsFreqPos;
    ET9U8             const * const pbIsQualityKey  = pCMP->pbIsQualityKey;

    const ET9U8   bMaxEditDist = pLingCmnInfo->Private.bCurrMaxEditDistance;
    const ET9U16  wCurrMinSourceLength = pLingCmnInfo->Private.wCurrMinSourceLength;

    const ET9BOOL bAllowSpcCmpl = pCMP->bAllowSpcCmpl;
    const ET9BOOL bAllowFreePunct = pCMP->bAllowFreePunct;
    const ET9BOOL bAllowFreeDouble = pCMP->bAllowFreeDouble;

    const ET9UINT nKeyCount = wLength;
    const ET9UINT nSymbCount = pWord->Base.wWordLen;

    const ET9UINT nQualityCount = pCMP->nQualityCount;

    ET9_UNUSED(wIndex);     /* used in "init" */

    ET9AssertLog(pLingCmnInfo->Private.ASpc.bSpcState == ET9_SPC_STATE_FLEX_INIT_OK);

    _ET9AWValidateFlexArea(pLingCmnInfo);

    WLOG8B({

        fprintf(pLogFile8, "__ADB_CalcEditDistanceFlex, spc cmpl %d, free punct %d, free double %d, word ", bAllowSpcCmpl, bAllowFreePunct, bAllowFreeDouble);

        {
            ET9U16 wIndex;

            for (wIndex = 0; wIndex < pWord->Base.wWordLen; ++wIndex) {
                fprintf(pLogFile8, "%c", pWord->Base.sWord[wIndex]);
            }
        }

        fprintf(pLogFile8, "\n");
    })

    /* screening */

    if (nSymbCount < wCurrMinSourceLength) {
        WLOG8B(fprintf(pLogFile8, "  no match (too short, %u < %u - %u)\n", nSymbCount, nQualityCount, bMaxEditDist);)
        _ET9AWModifiedFlexArea(pLingCmnInfo);
        return ET9STATUS_NO_MATCH;
    }

    /* screen calculation */

    {
        ET9UINT nR;
        ET9UINT nC;

        ET9BOOL bInStem = 1;

        /* matrix */

        for (nC = 1; nC <= nSymbCount; ++nC) {

            const ET9U16 wCharCode = __AdbGetCharAtPos(pWord, nC - 1);

            if (bInStem) {
                if (pCMP->pwPrevWordSC[nC] == wCharCode) {
                    continue;
                }
                else {
                    bInStem = 0;
                }
            }

            pCMP->pwPrevWordSC[nC] = wCharCode;

            {
                const ET9BOOL bIsFreeC = (ET9BOOL)(bAllowFreePunct && !pCMP->psLockSymb[nC - 1] && _ET9_IsFree(wCharCode));

                ET9U8 bBestColEditDist;

                pCMP->ppbEditDist[0][nC - 0] = pCMP->ppbEditDist[0][nC - 1] + (bIsFreeC ? 0 : 1);

                bBestColEditDist = pCMP->ppbEditDist[0][nC];

                for (nR = 1; nR <= nKeyCount; ++nR) {

                    WLOG8B(fprintf(pLogFile8, "matrix %2d %2d\n", nR, nC);)

                    /* initial */

                    pCMP->ppbEditDist[nR][nC] = _ET9_FLEX_TUBE_MAX_EDIT_DIST;

                    /* pointless? */

                    if (pCMP->ppbEditDist[nR - 1][nC - 1] > bMaxEditDist &&
                        pCMP->ppbEditDist[nR - 1][nC - 0] > bMaxEditDist &&
                        pCMP->ppbEditDist[nR - 0][nC - 1] > bMaxEditDist) {

                        WLOG8B(fprintf(pLogFile8, "  pointless (edit) [sc]\n");)

                        pCMP->pbSubstFreqSC[nR][nC] = 0;

                        continue;
                    }

                    {
                        const ET9U8 bFreeR = !pbLockInfo[nR];
                        const ET9U8 bQualityR = pbIsQualityKey[nR];

                        const ET9U8 bMatchFreq = __GetSymbFreq(pLingCmnInfo, (nR - 1), wCharCode, NULL);

                        /* */

                        pCMP->pbSubstFreqSC[nR][nC] = bMatchFreq;

                        /* subst */

                        if (bMatchFreq) {

                            WLOG8B(fprintf(pLogFile8, "  subst-1, bMatchFreq %d (%u, max %u) [sc]\n", bMatchFreq, pCMP->ppbEditDist[nR - 1][nC - 1], bMaxEditDist);)

                            pCMP->ppbEditDist[nR][nC] = pCMP->ppbEditDist[nR - 1][nC - 1];
                        }
                        else if (pCMP->ppbEditDist[nR - 1][nC - 1] < bMaxEditDist && bFreeR) {

                            WLOG8B(fprintf(pLogFile8, "  subst-2, ppbEditDist %d [sc]\n", pCMP->ppbEditDist[nR - 1][nC - 1]);)

                            pCMP->ppbEditDist[nR][nC] = pCMP->ppbEditDist[nR - 1][nC - 1] + 1;
                        }

                        /* del */

                        if (!bQualityR) {

                            const ET9U8 bEditCost = pCMP->ppbEditDist[nR - 1][nC - 0];

                            if (bEditCost < pCMP->ppbEditDist[nR][nC]) {

                                WLOG8B(fprintf(pLogFile8, "  del-1 (%u, max %u) [sc]\n", bEditCost, bMaxEditDist);)

                                pCMP->pbSubstFreqSC[nR][nC] = 0;

                                pCMP->ppbEditDist[nR][nC] = bEditCost;
                            }
                        }
                        else if (pCMP->ppbEditDist[nR - 1][nC - 0] < bMaxEditDist && bFreeR) {

                            const ET9U8 bEditCost = pCMP->ppbEditDist[nR - 1][nC - 0] + 1;

                            if (bEditCost < pCMP->ppbEditDist[nR][nC]) {

                                WLOG8B(fprintf(pLogFile8, "  del-2 [sc]\n");)

                                pCMP->pbSubstFreqSC[nR][nC] = 0;

                                pCMP->ppbEditDist[nR][nC] = bEditCost;
                            }
                        }

                        /* ins */

                        if (bAllowFreeDouble && bMatchFreq && bMatchFreq == pCMP->pbSubstFreqSC[nR][nC - 1] || bIsFreeC) {

                            const ET9U8 bEditCost = pCMP->ppbEditDist[nR - 0][nC - 1];

                            if (bEditCost < pCMP->ppbEditDist[nR][nC]) {

                                WLOG8B(fprintf(pLogFile8, "  ins-1 [sc]\n");)

                                pCMP->pbSubstFreqSC[nR][nC] = pCMP->pbSubstFreqSC[nR][nC - 1];

                                pCMP->ppbEditDist[nR][nC] = bEditCost;
                            }
                        }
                        else if (nR == nKeyCount && nC > nQualityCount && (!pCMP->ppbEditDist[nR - 0][nC - 1] || bAllowSpcCmpl)) {

                            const ET9U8 bEditCost = pCMP->ppbEditDist[nR - 0][nC - 1];

                            if (bEditCost < pCMP->ppbEditDist[nR][nC]) {

                                WLOG8B(fprintf(pLogFile8, "  ins-2, nC %d, nQualityCount %d [sc]\n", nC, nQualityCount);)

                                pCMP->pbSubstFreqSC[nR][nC] = 0;

                                pCMP->ppbEditDist[nR][nC] = bEditCost;
                            }
                        }
                        else if (pCMP->ppbEditDist[nR - 0][nC - 1] < bMaxEditDist) {

                            const ET9U8 bEditCost = pCMP->ppbEditDist[nR - 0][nC - 1] + 1;

                            if (bEditCost < pCMP->ppbEditDist[nR][nC]) {

                                WLOG8B(fprintf(pLogFile8, "  ins-3 (%u, max %u) [sc]\n", bEditCost, bMaxEditDist);)

                                pCMP->pbSubstFreqSC[nR][nC] = bMatchFreq;

                                pCMP->ppbEditDist[nR][nC] = bEditCost;
                            }
                        }

                        /* best */

                        if (bBestColEditDist > pCMP->ppbEditDist[nR][nC]) {
                            bBestColEditDist = pCMP->ppbEditDist[nR][nC];
                        }
                    }
                }

                /* pre-empt? */

                if (bBestColEditDist > bMaxEditDist) {
                    WLOG8B(fprintf(pLogFile8, "  no match (best edit) [sc], %d > %d\n", bBestColEditDist, bMaxEditDist);)
                    pCMP->pwPrevWordSC[nC] = ALDB_CHAR_CODE_NONE;
                    _ET9AWModifiedFlexArea(pLingCmnInfo);
                    return ET9STATUS_NO_MATCH;
                }
            }
        }

        pCMP->pwPrevWordSC[nC] = ALDB_CHAR_CODE_NONE;

        WLOG8B({

            fprintf(pLogFile8, "\n--- edist distance ---\n\n");

            for (nR = 0; nR <= nKeyCount; ++nR) {

                fprintf(pLogFile8, "%2d %2x : ", nR, pbIsQualityKey[nR]);

                for (nC = 0; nC <= nSymbCount; ++nC) {
                    fprintf(pLogFile8, "%2x ", pCMP->ppbEditDist[nR][nC]);
                }

                fprintf(pLogFile8, "\n");
            }

            fprintf(pLogFile8, "\n");
        })

        /* really a match? */

        if (pCMP->ppbEditDist[nKeyCount][nSymbCount] > bMaxEditDist) {
            WLOG8B(fprintf(pLogFile8, "  no match (final edit), %d > %d (%d)\n", pCMP->ppbEditDist[nKeyCount][nSymbCount], bMaxEditDist, pCMP->ppbStemDist[nKeyCount][nSymbCount]);)
            _ET9AWModifiedFlexArea(pLingCmnInfo);
            return ET9STATUS_NO_MATCH;
        }
    }

    /* full calculation */

    {
        ET9U8 pbSubstFreq[ALDB_COMPARE_MAX_POS + 1];
        ET9U8 pbComplLen[ET9MAXWORDSIZE + 1];

        ET9UINT nR;
        ET9UINT nC;

        /* init */

        pbComplLen[0] = 0;

        /* init rows */

        for (nR = 1; nR <= nKeyCount; ++nR) {
            pbSubstFreq[nR] = 0;
        }

        /* matrix */

        for (nC = 1; nC <= nSymbCount; ++nC) {

            const ET9BOOL bIsFreeC = (ET9BOOL)(bAllowFreePunct && !pCMP->psLockSymb[nC - 1] && _ET9_IsFree(__AdbGetCharAtPos(pWord, nC - 1)));

            pbComplLen[nC] = 0;

            pCMP->ppbStemDist[0][nC - 0] = pCMP->ppbEditDist[0][nC - 0];
            pCMP->ppbFreeDist[0][nC - 0] = (ET9U8)(nC - pCMP->ppbEditDist[0][nC - 0]);

            for (nR = 1; nR <= nKeyCount; ++nR) {

                WLOG8B(fprintf(pLogFile8, "matrix %2d %2d\n", nR, nC);)

                /* pointless? */

                if (pCMP->ppbEditDist[nR][nC] > bMaxEditDist) {

                    WLOG8B(fprintf(pLogFile8, "  pointless (edit %u > %u) [full]\n", pCMP->ppbEditDist[nR][nC], bMaxEditDist);)

                    pCMP->ppbFreeDist[nR][nC] = _ET9_FLEX_TUBE_MAX_FREE_DIST;
                    pCMP->ppbStemDist[nR][nC] = _ET9_FLEX_TUBE_MAX_STEM_DIST;
                    pCMP->ppxStemFreq[nR][nC] = 1;

                    pbSubstFreq[nR] = 0;

                    continue;
                }

                {
                    const ET9U8 bFreeR = !pbLockInfo[nR];
                    const ET9U8 bQualityR = pbIsQualityKey[nR];
                    const ET9U8 bSubstFreqPrev = pbSubstFreq[nR];

                    ET9BOOL bMatchExact;

                    const ET9U8 bMatchFreq = __GetSymbFreq(pLingCmnInfo, (nR - 1), __AdbGetCharAtPos(pWord, nC - 1), &bMatchExact);

                    const ET9U8 bDefaultFreq = bQualityR ? 10 : 1;

                    ET9U8 bEditDist;
                    ET9U8 bFreeDist;
                    ET9U8 bStemDist;
                    ET9FREQ xStemFreq;

                    /* */

                    pbSubstFreq[nR] = bMatchFreq;

                    /* subst */

                    if (bMatchFreq) {

#ifdef ET9_USE_FLOAT_FREQS
                        const ET9FREQ xFreq = (ET9FREQ)(!pbIsFreqPos[nR] ? bDefaultFreq : bMatchFreq);
#else
                        const ET9FREQ xFreq = (!pbIsFreqPos[nR] || bMatchFreq <= 17) ? 1 : ((bMatchFreq * 15) / 255);
#endif

                        WLOG8B(fprintf(pLogFile8, "  subst-1, bMatchFreq %d (%u, max %u) [full]\n", bMatchFreq, pCMP->ppbEditDist[nR - 1][nC - 1], bMaxEditDist);)

                        bFreeDist = pCMP->ppbFreeDist[nR - 1][nC - 1] + (bQualityR ? 0 : 1);
                        bEditDist = pCMP->ppbEditDist[nR - 1][nC - 1];
                        bStemDist = pCMP->ppbStemDist[nR - 1][nC - 1] + (bMatchExact ? 0 : 1);
                        xStemFreq = pCMP->ppxStemFreq[nR - 1][nC - 1] * xFreq;
                    }
                    else if (pCMP->ppbEditDist[nR - 1][nC - 1] < bMaxEditDist && bFreeR) {

                        WLOG8B(fprintf(pLogFile8, "  subst-2, ppbEditDist %d [full]\n", pCMP->ppbEditDist[nR - 1][nC - 1]);)

                        bFreeDist = pCMP->ppbFreeDist[nR - 1][nC - 1] + (bQualityR ? 0 : 1);
                        bEditDist = pCMP->ppbEditDist[nR - 1][nC - 1] + 1;
                        bStemDist = pCMP->ppbStemDist[nR - 1][nC - 1] + (bQualityR ? 1 : 0);
                        xStemFreq = pCMP->ppxStemFreq[nR - 1][nC - 1] * bDefaultFreq;
                    }
                    else {
                        bFreeDist = _ET9_FLEX_TUBE_MAX_FREE_DIST;
                        bEditDist = _ET9_FLEX_TUBE_MAX_EDIT_DIST;
                        bStemDist = _ET9_FLEX_TUBE_MAX_STEM_DIST;
                        xStemFreq = 1;
                    }

                    /* del */

                    if (!bQualityR) {

                        const ET9U8 bFreeCost = pCMP->ppbFreeDist[nR - 1][nC - 0];
                        const ET9U8 bEditCost = pCMP->ppbEditDist[nR - 1][nC - 0];
                        const ET9U8 bStemCost = pCMP->ppbStemDist[nR - 1][nC - 0];

                        if (_ET9_Flex_IsBetterOp(-1, 0)) {

                            WLOG8B(fprintf(pLogFile8, "  del-1 [full]\n");)

                            pbSubstFreq[nR] = 0;

                            bFreeDist = bFreeCost;
                            bEditDist = bEditCost;
                            bStemDist = bStemCost;
                            xStemFreq = pCMP->ppxStemFreq[nR - 1][nC - 0];
                        }
                    }
                    else if (pCMP->ppbEditDist[nR - 1][nC - 0] < bMaxEditDist && bFreeR) {

                        const ET9U8 bFreeCost = pCMP->ppbFreeDist[nR - 1][nC - 0];
                        const ET9U8 bEditCost = pCMP->ppbEditDist[nR - 1][nC - 0] + 1;
                        const ET9U8 bStemCost = pCMP->ppbStemDist[nR - 1][nC - 0] + 1;

                        if (_ET9_Flex_IsBetterOp(-1, 0)) {

                            WLOG8B(fprintf(pLogFile8, "  del-2 [full]\n");)

                            pbSubstFreq[nR] = 0;

                            bFreeDist = bFreeCost;
                            bEditDist = bEditCost;
                            bStemDist = bStemCost;
                            xStemFreq = pCMP->ppxStemFreq[nR - 1][nC - 0] * bDefaultFreq;
                        }
                    }

                    /* ins */

                    if (bAllowFreeDouble && bMatchFreq && bMatchFreq == bSubstFreqPrev || bIsFreeC) {

                        const ET9U8 bFreeCost = pCMP->ppbFreeDist[nR - 0][nC - 1] + 1;
                        const ET9U8 bEditCost = pCMP->ppbEditDist[nR - 0][nC - 1];
                        const ET9U8 bStemCost = pCMP->ppbStemDist[nR - 0][nC - 1];

                        if (_ET9_Flex_IsBetterOp(0, -1)) {

                            WLOG8B(fprintf(pLogFile8, "  ins-2 (free) [full]\n");)

                            pbSubstFreq[nR] = bSubstFreqPrev;

                            if (nR == nKeyCount) {
                                pbComplLen[nC] = pbComplLen[nC - 1];
                            }

                            bEditDist = bEditCost;
                            bFreeDist = bFreeCost;
                            bStemDist = bStemCost;
                            xStemFreq = pCMP->ppxStemFreq[nR - 0][nC - 1];
                        }
                    }
                    else if (nR == nKeyCount && nC > nQualityCount && (!pCMP->ppbEditDist[nR - 0][nC - 1] || bAllowSpcCmpl)) {

                        const ET9U8 bFreeCost = pCMP->ppbFreeDist[nR - 0][nC - 1];
                        const ET9U8 bEditCost = pCMP->ppbEditDist[nR - 0][nC - 1];
                        const ET9U8 bStemCost = pCMP->ppbStemDist[nR - 0][nC - 1];

                        if (_ET9_Flex_IsBetterOp(0, -1)) {

                            WLOG8B(fprintf(pLogFile8, "  ins-1 (cmpl), nC %d, nQualityCount %d [full]\n", nC, nQualityCount);)

                            pbSubstFreq[nR] = 0;

                            pbComplLen[nC] = pbComplLen[nC - 1] + 1;

                            bFreeDist = bFreeCost;
                            bEditDist = bEditCost;
                            bStemDist = bStemCost;
                            xStemFreq = pCMP->ppxStemFreq[nR - 0][nC - 1];
                        }
                    }
                    else if (pCMP->ppbEditDist[nR - 0][nC - 1] < bMaxEditDist) {

                        const ET9U8 bFreeCost = pCMP->ppbFreeDist[nR - 0][nC - 1];
                        const ET9U8 bEditCost = pCMP->ppbEditDist[nR - 0][nC - 1] + 1;
                        const ET9U8 bStemCost = pCMP->ppbStemDist[nR - 0][nC - 1] + 1;

                        if (_ET9_Flex_IsBetterOp(0, -1)) {

                            WLOG8B(fprintf(pLogFile8, "  ins-3 (spc) [full]\n");)

                            pbSubstFreq[nR] = bMatchFreq;

                            bFreeDist = bFreeCost;
                            bEditDist = bEditCost;
                            bStemDist = bStemCost;
                            xStemFreq = pCMP->ppxStemFreq[nR - 0][nC - 1];
                        }
                    }

                    /* assure same as screening */

                    if (bEditDist != pCMP->ppbEditDist[nR][nC]) {
                        WLOG8B(fprintf(pLogFile8, "  inconsistent (%u <-> %u) [full]\n", bEditDist, pCMP->ppbEditDist[nR][nC]);)
                    }

                    ET9AssertLog(bEditDist == pCMP->ppbEditDist[nR][nC]);

                    /* persist */

                    pCMP->ppbFreeDist[nR][nC] = bFreeDist;
                    pCMP->ppbStemDist[nR][nC] = bStemDist;
                    pCMP->ppxStemFreq[nR][nC] = xStemFreq;
                }
            }
        }

        WLOG8B({

            fprintf(pLogFile8, "\n--- free distance ---\n\n");

            for (nR = 0; nR <= nKeyCount; ++nR) {

                fprintf(pLogFile8, "%2d %2x : ", nR, pbIsQualityKey[nR]);

                for (nC = 0; nC <= nSymbCount; ++nC) {
                    fprintf(pLogFile8, "%2x ", pCMP->ppbFreeDist[nR][nC]);
                }

                fprintf(pLogFile8, "\n");
            }

            fprintf(pLogFile8, "\n--- edist distance ---\n\n");

            for (nR = 0; nR <= nKeyCount; ++nR) {

                fprintf(pLogFile8, "%2d %2x : ", nR, pbIsQualityKey[nR]);

                for (nC = 0; nC <= nSymbCount; ++nC) {
                    fprintf(pLogFile8, "%2x ", pCMP->ppbEditDist[nR][nC]);
                }

                fprintf(pLogFile8, "\n");
            }

            fprintf(pLogFile8, "\n--- stem distance ---\n\n");

            for (nR = 0; nR <= nKeyCount; ++nR) {

                fprintf(pLogFile8, "%2d %2x : ", nR, pbIsQualityKey[nR]);

                for (nC = 0; nC <= nSymbCount; ++nC) {
                    fprintf(pLogFile8, "%2x ", pCMP->ppbStemDist[nR][nC]);
                }

                fprintf(pLogFile8, "\n");
            }

            fprintf(pLogFile8, "\n--- stem freq ---\n\n");

            for (nR = 0; nR <= nKeyCount; ++nR) {

                fprintf(pLogFile8, "%2d %2x : ", nR, pbIsQualityKey[nR]);

                for (nC = 0; nC <= nSymbCount; ++nC) {
                    fprintf(pLogFile8, "%8.0f ", pCMP->ppxStemFreq[nR][nC]);
                }

                fprintf(pLogFile8, "\n");
            }

            fprintf(pLogFile8, "\n--- completion length ---\n\n");

            {
                fprintf(pLogFile8, "      : ");

                for (nC = 0; nC <= nSymbCount; ++nC) {
                    fprintf(pLogFile8, "%2x ", pbComplLen[nC]);
                }

                fprintf(pLogFile8, "\n");
            }

            fprintf(pLogFile8, "\n");
        })

        /* assure same as screening */

        ET9AssertLog(pCMP->ppbEditDist[nKeyCount][nSymbCount] <= bMaxEditDist);

        {
            const ET9BOOL bAvoidStems = (pLingCmnInfo->Private.bTraceBuild || wIndex) ? 1 : 0;

            /* suppress completions before point */

            if (pbComplLen[nSymbCount] && nQualityCount < pLingCmnInfo->Private.wWordCompletionPoint) {
                if (bAvoidStems) {
                    WLOG8B(fprintf(pLogFile8, "  suppressed completion before point\n");)
                    __STAT_AWLdb_Flex_EDC_Done(NULL);
                    _ET9AWModifiedFlexArea(pLingCmnInfo);
                    return ET9STATUS_NO_MATCH;
                }
            }

            /* assign distance */

            pWord->bEditDistSpc  = pCMP->ppbEditDist[nKeyCount][nSymbCount];
            pWord->bEditDistStem = pCMP->ppbStemDist[nKeyCount][nSymbCount];
            pWord->bEditDistFree = pCMP->ppbFreeDist[nKeyCount][nSymbCount];

            /**/

            if (pbComplLen[nSymbCount] && !pWord->bEditDistSpc && (pWordSymbInfo->bNumSymbs < pLingCmnInfo->Private.wWordCompletionPoint)) {
                if (bAvoidStems) {
                    WLOG8B(fprintf(pLogFile8, "  assigning edit distance - before completion point\n");)
                    pWord->bEditDistSpc = 1;
                }
            }

#if 1
            if (pWord->Base.wWordLen > pLingCmnInfo->Private.wMaxWordLength + pWord->bEditDistFree) {
                pWord->bEditDistFree = 0;
                pWord->Base.wWordCompLen = 0;
            }
            else {
                pWord->Base.wWordCompLen = pbComplLen[nSymbCount];
            }
#else
            if (pbComplLen[nSymbCount] && (!pWord->bEditDistSpc || bAllowSpcCmpl) && pWord->Base.wWordLen <= pLingCmnInfo->Private.wMaxWordLength) {
                pWord->Base.wWordCompLen = pbComplLen[nSymbCount];
            }
            else {
                pWord->Base.wWordCompLen = 0;
            }
#endif

            if (!pWord->Base.wWordCompLen && pWord->Base.wWordLen > pWordSymbInfo->bNumSymbs) {
                if (!pWord->bEditDistSpc && pWord->Base.wWordLen <= pLingCmnInfo->Private.wMaxWordLength) {
                    WLOG8B(fprintf(pLogFile8, "  longer than input and shorter than max - special zero compl len\n");)
                    pWord->Base.wWordCompLen = 0xFFFF;
                }
                else if (pWord->Base.wWordLen == pLingCmnInfo->Private.wMaxWordLength + pWord->bEditDistFree) {
                    WLOG8B(fprintf(pLogFile8, "  longer than input and longer than max - special zero compl len\n");)
                    pWord->Base.wWordCompLen = 0xFFFF;
                }
            }
        }

        /* assign tap freq */

#ifdef ET9_USE_FLOAT_FREQS

        pWord->xTapFreq = pCMP->ppxStemFreq[nKeyCount][nSymbCount] / _ET9pow_f(10, (ET9FLOAT)__ET9Min(nQualityCount, ET9_SPC_ED_MAX_FREQ_LEN));

#else /* ET9_USE_FLOAT_FREQS */

        if (nQualityCount <= 4) {
            pWord->xTapFreq = (ET9FREQPART)pCMP->ppxStemFreq[nKeyCount][nSymbCount];
        }
        else {
            pWord->xTapFreq = (ET9FREQPART)(pCMP->ppxStemFreq[nKeyCount][nSymbCount] >> (4 * (nQualityCount - 4)));
        }

#endif /* ET9_USE_FLOAT_FREQS */

        if (!pWord->xTapFreq) {
            pWord->xTapFreq = 1;
        }

        ET9Assert(pWord->xTapFreq >= 0);

        WLOG8B(fprintf(pLogFile8, "  match, bEditDistStem %d, bEditDistSpc %d, wWordCompLen %d, \n\n",
                                  pWord->bEditDistStem,
                                  pWord->bEditDistSpc,
                                  pWord->Base.wWordCompLen);)

        _ET9AWModifiedFlexArea(pLingCmnInfo);

        return ET9STATUS_NONE;
    }
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Calculate edit distance - wrapper that applies the correct function.
 *
 * @param pLingInfo                 Pointer to alphabetic information structure.
 * @param pWord                     Word to investigate.
 * @param wIndex                    Input index/position (non zero if partial).
 * @param wLength                   Word length (active input length).
 *
 * @return ET9STATUS_NONE if ok, otherwise ET9STATUS_NO_MATCH.
 */

static ET9STATUS ET9LOCALCALL __CalcEditDistance(ET9AWLingInfo     * const pLingInfo,
                                                 ET9AWPrivWordInfo * const pWord,
                                                 const ET9U16              wIndex,
                                                 const ET9U16              wLength)
{
    ET9AWLingCmnInfo * const pLingCmnInfo = pLingInfo->pLingCmnInfo;

    ET9STATUS eStatus;

    if (wIndex + wLength > ET9MAXWORDSIZE) {
        ET9Assert(0);
        return ET9STATUS_ERROR;
    }

    if (pLingCmnInfo->Private.ASpc.bSpcFeatures) {
        eStatus = __ADB_CalcEditDistanceFlex(pLingInfo, pWord, wIndex, wLength);
    }
    else {
        eStatus = __ET9AWCalcEditDistance(pLingInfo, pWord, wIndex, wLength);
    }

    ET9AssertLog(pWord->xTapFreq >= 0);
    ET9AssertLog(pWord->xTotFreq >= 0);

    return eStatus;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * This function assures that regional base syms will not have the same tap frequency.
 * It's used to identify what symbs belong to a certain key.
 *
 * @param pSymbInfo                 Pointer to an input symbol.
 *
 * @return None
 */

static void ET9LOCALCALL __MakeSymbFreqsUnique(ET9SymbInfo * const pSymbInfo)
{
    if (pSymbInfo->eInputType == ET9DISCRETEKEY ||
        pSymbInfo->eInputType == ET9MULTITAPKEY ||
        pSymbInfo->eInputType == ET9CUSTOMSET ||
        pSymbInfo->bSymbType == ET9KTSMARTPUNCT) {

        return;
    }

    if (!pSymbInfo->bNumBaseSyms) {
        return;
    }

    {
        ET9U8 pbUsedFreq[0x100];
        ET9DataPerBaseSym *pDPBS;

        _ET9ClearMem(pbUsedFreq, sizeof(pbUsedFreq));

        for (pDPBS = &pSymbInfo->DataPerBaseSym[pSymbInfo->bNumBaseSyms - 1]; pDPBS >= &pSymbInfo->DataPerBaseSym[0]; --pDPBS) {

            if (!pbUsedFreq[pDPBS->bSymFreq]) {
                pbUsedFreq[pDPBS->bSymFreq] = 1;
                continue;
            }

            {
                ET9UINT nIndex;

                for (nIndex = pDPBS->bSymFreq; nIndex < 0xFF && pbUsedFreq[nIndex]; ++nIndex) {
                }

                for (; nIndex > 0 && pbUsedFreq[nIndex]; --nIndex) {
                }

                ET9AssertLog(nIndex <= 0xFF);
                ET9AssertLog(!pbUsedFreq[nIndex]);

                WLOG2(fprintf(pLogFile2, "    unique, %3u becomes %3u\n", pDPBS->bSymFreq, nIndex);)

                pDPBS->bSymFreq = (ET9U8)nIndex;
                pbUsedFreq[nIndex] = 1;
            }
        }
    }
}

/*---------------------------------------------------------------------------*/
/** \internal
 * This function assures that all symb freqs are valid.
 * Only applicable for flex bacsed calculations (has flex features).
 *
 * @param pLingCmnInfo              Pointer to alphabetic cmn information structure.
 *
 * @return None
 */

static void ET9LOCALCALL __MakeSymbFreqsValid(ET9AWLingCmnInfo  const * const pLingCmnInfo)
{
    ET9WordSymbInfo * const pWordSymbInfo = pLingCmnInfo->Base.pWordSymbInfo;

    ET9UINT nSymbCount;
    ET9SymbInfo *pSymbInfo;

    WLOG2(fprintf(pLogFile2, "__MakeSymbFreqsValid\n");)

    pSymbInfo = pWordSymbInfo->SymbsInfo;
    for (nSymbCount = pWordSymbInfo->bNumSymbs; nSymbCount; --nSymbCount, ++pSymbInfo) {

        if (!pSymbInfo->bFreqsInvalidated) {
            continue;
        }

        WLOG2(fprintf(pLogFile2, "  [%2u] new symb\n", (pSymbInfo - pWordSymbInfo->SymbsInfo));)

        pSymbInfo->bFreqsInvalidated = 0;

        if (!pLingCmnInfo->Private.ASpc.bSpcFeatures) {
            continue;
        }

        __MakeSymbFreqsUnique(pSymbInfo);
    }
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Wrapper for match and insert word that handles spc init's.
 *
 * @param pLingInfo                 Pointer to alphabetic information structure.
 * @param pWord                     Word to be matched.
 * @param wIndex                    Input index/position (non zero if partial).
 * @param wLength                   Word length (active input length).
 * @param bFreqIndicator            Designation
 * @param bSpcMode                  Spell correction mode.
 *
 * @return ET9STATNONE on success, otherwise return ET9 error code.
 */

ET9INLINE static ET9STATUS ET9LOCALCALL __SelLstWordSearch(ET9AWLingInfo       * const pLingInfo,
                                                           ET9AWPrivWordInfo   * const pWord,
                                                           const ET9U16                wIndex,
                                                           const ET9U16                wLength,
                                                           const ET9_FREQ_DESIGNATION  bFreqIndicator,
                                                           const ET9U8                 bSpcMode)
{
    ET9STATUS eStatus;

    eStatus = _ET9AWCalcEditDistanceInit(pLingInfo, wIndex, wLength, bSpcMode);

    if (eStatus) {
        return eStatus;
    }

    eStatus = _ET9AWSelLstWordSearch(pLingInfo, pWord, wIndex, wLength, bFreqIndicator);

    _ET9AWCalcEditDistanceDone(pLingInfo);

    return eStatus;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Word match against input.
 * This function determines if the specified word is matching the input sequence.
 * This function is for "slow" sources, not supporting dedicated matching.
 *
 * @param pLingInfo                 Pointer to alphabetic information structure.
 * @param pWord                     Word to match.
 * @param wFirstTap                 First tap index to tap's sort list.
 * @param bTotalSymbInputs          Total taps in the tap's sort list.
 * @param pbyMatched                (out) specified word match tap sequence.
 *
 * @return ET9STATNONE on success, otherwise return ET9 error code.
 */

static ET9STATUS ET9LOCALCALL __ET9AWSelLstWordMatch(ET9AWLingInfo       * const pLingInfo,
                                                     ET9AWPrivWordInfo   * const pWord,
                                                     const ET9U16                wFirstTap,
                                                     const ET9U8                 bTotalSymbInputs,
                                                     ET9U8               * const pbyMatched)
{
    ET9AssertLog(pLingInfo != NULL);
    ET9AssertLog(pWord != NULL);
    ET9AssertLog(pbyMatched != NULL);

#if 0
    WLOG2(fprintf(pLogFile2, "_ET9AWSelLstWordMatch, wFirstTap = %d, bTotalSymbInputs = %d\n", wFirstTap, bTotalSymbInputs);)
    WLOG2Word(pLogFile2, "word to match", pWord);
#endif

    /* check if the word match the input */

    if (__CalcEditDistance(pLingInfo, pWord, wFirstTap, bTotalSymbInputs)) {
        *pbyMatched = 0;
        return ET9STATUS_NONE;
    }

    /* check if the substitution itself is a better match */

    if (pWord->Base.wSubstitutionLen &&
        pWord->bEditDistSpc &&
        pWord->Base.wSubstitutionLen <= ET9MAXWORDSIZE &&
        !_ET9FindSpacesAndUnknown(pWord->Base.sSubstitution, pWord->Base.wSubstitutionLen)) {

        ET9AWPrivWordInfo sSubstAsWord;

        _InitPrivWordInfo(&sSubstAsWord);

        sSubstAsWord.bWordSrc = pWord->bWordSrc;
        sSubstAsWord.Base.wWordLen = pWord->Base.wSubstitutionLen;
        _ET9SymCopy(sSubstAsWord.Base.sWord, pWord->Base.sSubstitution, pWord->Base.wSubstitutionLen);

        if (!__CalcEditDistance(pLingInfo, &sSubstAsWord, wFirstTap, bTotalSymbInputs)) {

            if (sSubstAsWord.bEditDistSpc < pWord->bEditDistSpc ||
                sSubstAsWord.bEditDistSpc == pWord->bEditDistSpc && sSubstAsWord.bEditDistStem < pWord->bEditDistStem) {

                pWord->Base.wWordCompLen = 0;   /* for WLOG2Word */

                WLOG2(fprintf(pLogFile2, "_ET9AWSelLstWordMatch, replacing shortcut with its substitution\n");)
                WLOG2Word(pLogFile2, "shortcut", pWord);
                WLOG2Word(pLogFile2, "substitution", &sSubstAsWord);

                *pWord = sSubstAsWord;
            }
        }
    }

    /* we have a match */

    *pbyMatched = 1;

    {
        const ET9U16 wWordCompLenSave = pWord->Base.wWordCompLen;

        _ET9AWSelLstWordPreAdd(pLingInfo, pWord, wFirstTap, bTotalSymbInputs);

        pWord->Base.wWordCompLen = wWordCompLenSave;
    }

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Word pre add.
 * The function will attach shift info etc.
 * This function is broken out especially for sources NOT using _ET9AWSelLstWordMatch.
 *
 * @param pLingInfo                 Pointer to alphabetic information structure.
 * @param pWord                     Word to be updated.
 * @param wFirstTap                 First tap index/position (non zero if partial).
 * @param bInputLength              Word length (active input length).
 *
 * @return ET9STATNONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL _ET9AWSelLstWordPreAdd (ET9AWLingInfo      * const pLingInfo,
                                             ET9AWPrivWordInfo  * const pWord,
                                             const ET9U16               wFirstTap,
                                             const ET9U8                bInputLength)
{
    ET9S16             swIndex;
    ET9U16             wCount;
    ET9SymbInfo        *pSymbInfo;
    ET9BOOL            bLockCompare;
    ET9BOOL            bAllShifted;
    ET9BOOL            bAllUnshifted;
    ET9SYMB            *psSymb;
    ET9SYMB            *psLower;
    ET9U16             wLdbNum;

    ET9WordSymbInfo * const pWordSymbInfo = pLingInfo->pLingCmnInfo->Base.pWordSymbInfo;

    ET9AssertLog(pLingInfo != NULL);
    ET9AssertLog(pWord != NULL);

    /* verify word */

#ifdef ET9_DEBUG
    {
        ET9SYMB *pSymb = pWord->Base.sWord;

        ET9AssertLog(pWord->Base.wWordLen <= ET9MAXWORDSIZE);

        for (wCount = pWord->Base.wWordLen; wCount; --wCount) {
            ET9AssertLog(*pSymb && *pSymb != (ET9SYMB)0xcccc);
            ++pSymb;
        }
    }
#endif

    /* compLen is set in Add and in preAdd... */

    if (pWord->bEditDistSpc || pWord->Base.wWordLen < bInputLength) {
        pWord->Base.wWordCompLen = 0;
    }
    else {
        pWord->Base.wWordCompLen = (ET9U16)(pWord->Base.wWordLen - bInputLength);
    }

    /* log word */

#if 0
    WLOG2Word(pLogFile2, "_ET9AWSelLstWordPreAdd, pWord", pWord);
#endif

    /* save the first char's original value */

    pWord->sPureFirstChar = pWord->Base.sWord[0];

    /* from here on this function handles shift properties - skip for some languages */

    if (!_ET9_LanguageSpecific_ApplyShifting(pLingInfo, pWord)) {

        WLOG2(fprintf(pLogFile2, "_ET9AWSelLstWordPreAdd, skipping shift handling\n");)

        return ET9STATUS_NONE;
    }

    /* acronym? capitalized? */

    if (bInputLength > 1 && pWord->Base.wWordLen > 1) {

        ET9SYMB *psCurr;
        ET9UINT nIndex;
        ET9UINT nShiftCount;
        ET9UINT nLowerCount;
        ET9UINT nUpperCount;

        nShiftCount = 0;
        nLowerCount = 0;
        nUpperCount = 0;
        psCurr = &pWord->Base.sWord[0];
        pSymbInfo = &pWordSymbInfo->SymbsInfo[wFirstTap];
        for (nIndex = 0; nIndex < bInputLength; ++nIndex, ++psCurr, ++pSymbInfo) {

            switch (pSymbInfo->bSymbType)
            {
                case ET9KTPUNCTUATION:
                case ET9KTSMARTPUNCT:
                    break;
                case ET9KTNUMBER:
                case ET9KTSTRING:
                    continue;
            }

            if (pSymbInfo->eShiftState) {
                ++nShiftCount;
            }

            if (nIndex >= pWord->Base.wWordLen) {
                continue;
            }

            if (_ET9SymIsLower(*psCurr, pWordSymbInfo->Private.wLocale)) {
                ++nLowerCount;
                break;
            }

            if (_ET9SymIsUpper(*psCurr, pWordSymbInfo->Private.wLocale)) {
                ++nUpperCount;
            }
        }

        /* the rule for acronym is simplified to to improve performance and intended to catch the case without "input shift" */

        if (nUpperCount > 1 && nLowerCount <= 1 && nShiftCount < nUpperCount) {

            WLOG2(fprintf(pLogFile2, "_ET9AWSelLstWordPreAdd, word is an acronym\n");)

            pWord->bIsAcronym = 1;
        }

        /* the rule for capitalization is simplified to to improve performance and intended to catch the case without "input shift" */

        if (nUpperCount == 1 && nLowerCount == 1 && nShiftCount == 0) {

            WLOG2(fprintf(pLogFile2, "_ET9AWSelLstWordPreAdd, word is capitalized\n");)

            pWord->bIsCapitalized = 1;
        }
    }

    /* all shifted? */

    {
        ET9U16 wShiftedCount = 0;

        bAllShifted = 1;
        bAllUnshifted = 1;

        pSymbInfo = &pWordSymbInfo->SymbsInfo[wFirstTap];

        for (wCount = 0; wCount < bInputLength; ++wCount, ++pSymbInfo) {

            if (!__IsShiftNeutralSymb(pSymbInfo) || pWordSymbInfo->Private.eLastShiftState == ET9CAPSLOCK) {

                if (pSymbInfo->eShiftState) {

                    ++wShiftedCount;
                     bAllUnshifted = 0;

                    /* if current word has shifting after a lockpoint, cancels any compounding downshift logic */

                    if (wCount > pLingInfo->pLingCmnInfo->Private.wCurrLockPoint) {
                        pWordSymbInfo->Private.bCompoundingDownshift = 0;
                    }
                }
                else if (pSymbInfo->bForcedLowercase || pSymbInfo->bAutoDowncase) {
                    bAllShifted = 0;
                }
                else {
                    bAllShifted = 0;
                    bAllUnshifted = 0;
                    break;
                }
            }
        }

        if (wShiftedCount <= 1) {
            bAllShifted = 0;
        }
    }

    /* handle shift for word */

    bLockCompare = 0;

    swIndex = (ET9S16)((ET9S16)bInputLength - 1);

    ET9AssertLog(wFirstTap + swIndex < ET9MAXWORDSIZE);

    psSymb = &pWord->Base.sWord[bInputLength - 1];
    pSymbInfo = &pWordSymbInfo->SymbsInfo[wFirstTap + swIndex];
    wLdbNum  = (pWord->Base.bLangIndex == ET9AWSECOND_LANGUAGE) ? pLingInfo->pLingCmnInfo->wSecondLdbNum : pLingInfo->pLingCmnInfo->wFirstLdbNum;

    for (; swIndex >= 0; --swIndex, --pSymbInfo, --psSymb) {

        if (!bLockCompare && pSymbInfo->bLocked) {
            bLockCompare = 1;
        }

        if (bLockCompare) {

            ET9AssertLog(pSymbInfo->sLockedSymb && pSymbInfo->sLockedSymb != (ET9SYMB)0xcccc);
            ET9AssertLog(_ET9SymToLower(*psSymb, wLdbNum) == _ET9SymToLower(pSymbInfo->sLockedSymb, wLdbNum) || _ET9SymToUpper(*psSymb, wLdbNum) == _ET9SymToUpper(pSymbInfo->sLockedSymb, wLdbNum));

            *psSymb = pSymbInfo->sLockedSymb;

            /* any locked character will not be downcased; safety clear */

            pSymbInfo->bAutoDowncase = 0;
        }
        else if (!__IsShiftNeutralSymb(pSymbInfo) || pWordSymbInfo->Private.eLastShiftState == ET9CAPSLOCK) {

            if (bAllShifted || pSymbInfo->eShiftState) {

                ET9AssertLog(swIndex >= pWord->Base.wWordLen || _ET9SymToUpper(*psSymb, wLdbNum) && _ET9SymToUpper(*psSymb, wLdbNum) != (ET9SYMB)0xcccc);

                *psSymb = _ET9SymToUpper(*psSymb, wLdbNum); /* this will write outside the actual string on short spc, but that should be ok... */

                /* any shifted character will not be downcased; safety clear */

                pSymbInfo->bAutoDowncase = 0;
            }
            else if ((pSymbInfo->bForcedLowercase || pSymbInfo->bAutoDowncase)) {
                *psSymb = _ET9SymToLower(*psSymb, wLdbNum);
            }

            /* otherwise, if actively downcasing, make sure to 'mark' sym entry */

            else if (pWordSymbInfo->Private.bCompoundingDownshift) {
                *psSymb = _ET9SymToLower(*psSymb, wLdbNum);
                pSymbInfo->bAutoDowncase = 1;
            }
        }
    }

    /* handle caps lock for long spc and completion */

    if (bAllShifted || pWordSymbInfo->Private.eLastShiftState == ET9CAPSLOCK) {

        swIndex = (ET9S16)(pWord->Base.wWordLen - 1);
        psSymb = &pWord->Base.sWord[swIndex];

        for (; swIndex > bInputLength - 1; --swIndex, --psSymb) {
            *psSymb = _ET9SymToUpper(*psSymb, wLdbNum);
        }
    }
    else if (pWordSymbInfo->Private.bCompoundingDownshift) {    /* if actively downcasing.... */

        /* make sure to lowercase all syms in completion */

        swIndex = (ET9S16)(pWord->Base.wWordLen - 1);
        psSymb = &pWord->Base.sWord[swIndex];

        for (; swIndex > bInputLength - 1; --swIndex, --psSymb) {
            *psSymb = _ET9SymToLower(*psSymb, wLdbNum);
        }
    }

    /* handle shift for substitution */

    if (pWord->Base.wSubstitutionLen) {

        psLower = pWord->Base.sSubstitution;

        /* if all syms shifted, do same for substitution */

        if (bAllShifted) {
            for (wCount = pWord->Base.wSubstitutionLen; wCount; --wCount, ++psLower) {
                *psLower = _ET9SymToUpper(*psLower, wLdbNum);
            }
        }
        else if (bAllUnshifted) {
            for (wCount = pWord->Base.wSubstitutionLen; wCount; --wCount, ++psLower) {
                *psLower = _ET9SymToLower(*psLower, wLdbNum);
            }
        }

        /* if first sym shifted, do same for substitution */

        else {
            pSymbInfo = &pWordSymbInfo->SymbsInfo[wFirstTap];

            if (pSymbInfo->eShiftState && (!__IsShiftNeutralSymb(pSymbInfo) || pWordSymbInfo->Private.eLastShiftState == ET9CAPSLOCK)) {

                *psLower = _ET9SymToUpper(*psLower, wLdbNum);
            }
        }
    }

    return ET9STATUS_NONE;
}
/*---------------------------------------------------------------------------*/
/** \internal
 * Match word.
 * Checks if a specified word is matching the tap list.
 * This function is for "slow" sources, not supporting dedicated matching.
 *
 * @param pLingInfo                 Pointer to alphabetic information structure.
 * @param pWord                     Word to be matched.
 * @param wIndex                    Input index/position (non zero if partial).
 * @param wLength                   Word length (active input length).
 * @param pbFound                   Pointer to word found.
 *
 * @return ET9STATNONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL _ET9AWSelLstWordMatch( ET9AWLingInfo           * const pLingInfo,
                                            ET9AWPrivWordInfo       * const pWord,
                                            const ET9U16                    wIndex,
                                            const ET9U16                    wLength,
                                            ET9U8                          *pbFound)
{
    ET9STATUS  wStatus = ET9STATUS_NONE;

    ET9AssertLog(pLingInfo != NULL);
    ET9AssertLog(pWord != NULL);

    *pbFound = 0;

    /* match the word against all taps */

    if (pWord->Base.wWordLen >= pLingInfo->pLingCmnInfo->Private.wCurrMinSourceLength) {

        wStatus = __ET9AWSelLstWordMatch(pLingInfo, pWord, wIndex, (ET9U8)wLength, pbFound);
    }

    return wStatus;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Match and insert word.
 * Checks if a specified word is matching the tap list, and insert into selection list on match.<br>
 * This function is for "slow" sources, not supporting dedicated matching.
 *
 * @param pLingInfo                 Pointer to alphabetic information structure.
 * @param pWord                     Word to be matched.
 * @param wIndex                    Input index/position (non zero if partial).
 * @param wLength                   Word length (active input length).
 * @param bFreqIndicator            Designation
 *
 * @return ET9STATNONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL _ET9AWSelLstWordSearch(ET9AWLingInfo           * const pLingInfo,
                                            ET9AWPrivWordInfo       * const pWord,
                                            const ET9U16                    wIndex,
                                            const ET9U16                    wLength,
                                            const ET9_FREQ_DESIGNATION      bFreqIndicator)
{
    ET9STATUS  wStatus = ET9STATUS_NONE;
    ET9U8      bFound = 0;

    ET9AssertLog(pLingInfo != NULL);
    ET9AssertLog(pWord != NULL);

    wStatus = _ET9AWSelLstWordMatch(pLingInfo, pWord, wIndex, wLength, &bFound);

    if (!wStatus && bFound) {

        /* attempt to add, not interested in return value */

        _ET9AWSelLstAdd(pLingInfo, pWord, wLength, bFreqIndicator);
    }

    return wStatus;
}

/*---------------------------------------------------------------------------*/
/** \ingroup et9asys
 * This function set that exact word sellist positioning parameters
 *
 * @param pLingInfo                 Pointer to alphabetic information structure.
 * @param eSetting                  ET9AEXACTINLIST setting desired.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL ET9AWSetExactInList(ET9AWLingInfo          * const pLingInfo,
                                         const ET9AEXACTINLIST          eSetting)
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
        ET9UINT nChangeCount = 0;

        switch (eSetting)
        {
            case ET9AEXACTINLIST_OFF:
                if (pLingInfo->pLingCmnInfo->Private.bStateExactInList) {
                    ++nChangeCount;
                    pLingInfo->pLingCmnInfo->Private.bStateExactInList = 0;
                }
                break;
            case ET9AEXACTINLIST_FIRST:
                if (!pLingInfo->pLingCmnInfo->Private.bStateExactInList) {
                    ++nChangeCount;
                    pLingInfo->pLingCmnInfo->Private.bStateExactInList = 1;
                }
                if (pLingInfo->pLingCmnInfo->Private.bStateExactIsDefault) {
                    ++nChangeCount;
                    pLingInfo->pLingCmnInfo->Private.bStateExactIsDefault = 0;
                }
                if (pLingInfo->pLingCmnInfo->Private.bStateExactLast) {
                    ++nChangeCount;
                    pLingInfo->pLingCmnInfo->Private.bStateExactLast = 0;
                }
                break;
            case ET9AEXACTINLIST_LAST:
                if (!pLingInfo->pLingCmnInfo->Private.bStateExactInList) {
                    ++nChangeCount;
                    pLingInfo->pLingCmnInfo->Private.bStateExactInList = 1;
                }
                if (pLingInfo->pLingCmnInfo->Private.bStateExactIsDefault) {
                    ++nChangeCount;
                    pLingInfo->pLingCmnInfo->Private.bStateExactIsDefault = 0;
                }
                if (!pLingInfo->pLingCmnInfo->Private.bStateExactLast) {
                    ++nChangeCount;
                    pLingInfo->pLingCmnInfo->Private.bStateExactLast = 1;
                }
                break;
            case ET9AEXACTINLIST_DEFAULT:
                if (!pLingInfo->pLingCmnInfo->Private.bStateExactInList) {
                    ++nChangeCount;
                    pLingInfo->pLingCmnInfo->Private.bStateExactInList = 1;
                }
                if (!pLingInfo->pLingCmnInfo->Private.bStateExactIsDefault) {
                    ++nChangeCount;
                    pLingInfo->pLingCmnInfo->Private.bStateExactIsDefault = 1;
                }
                if (pLingInfo->pLingCmnInfo->Private.bStateExactLast) {
                    ++nChangeCount;
                    pLingInfo->pLingCmnInfo->Private.bStateExactLast = 0;
                }
                break;
            default:
                return ET9STATUS_BAD_PARAM;
        }

        if (nChangeCount) {
            _ET9InvalidateSymbInfo(pLingInfo->pLingCmnInfo->Base.pWordSymbInfo);
        }
    }

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \ingroup et9asys
 * This function gets the current exact word sellist positioning setting
 *
 * @param pLingInfo           Pointer to alphabetic information structure.
 * @param peSetting           Pointer to collect ET9AEXACTINLIST setting.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL ET9AWGetExactInList(ET9AWLingInfo    * const pLingInfo,
                                         ET9AEXACTINLIST  * const peSetting)
{
    ET9STATUS wStatus;

    if ((wStatus = _ET9AWSys_BasicValidityCheck(pLingInfo)) != ET9STATUS_NONE) {
        return wStatus;
    }

    if (!peSetting) {
        return ET9STATUS_INVALID_MEMORY;
    }

    if (!ET9EXACTINLIST(pLingInfo->pLingCmnInfo)) {
        *peSetting = ET9AEXACTINLIST_OFF;
    }
    else if (ET9EXACTLAST(pLingInfo->pLingCmnInfo)) {
        *peSetting = ET9AEXACTINLIST_LAST;
    }
    else if (ET9EXACTISDEFAULT(pLingInfo->pLingCmnInfo)) {
        *peSetting = ET9AEXACTINLIST_DEFAULT;
    }
    else {
        *peSetting = ET9AEXACTINLIST_FIRST;
    }

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \ingroup et9asys
 * Set auto append in list.
 * This function set that auto append's should be included in lists.
 *
 * @param pLingInfo                 Pointer to alphabetic information structure.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL ET9AWSetAutoAppendInList(ET9AWLingInfo * const pLingInfo)
{
    ET9STATUS           wStatus;

    wStatus = _ET9AWSys_BasicValidityCheck(pLingInfo);

    if (wStatus) {
        return wStatus;
    }

    wStatus = _ET9SettingsInhibited(pLingInfo->pLingCmnInfo->Base.pWordSymbInfo);

    if (wStatus) {
        return wStatus;
    }

    if (!pLingInfo->pLingCmnInfo->Private.bStateAutoAppendInList) {

        pLingInfo->pLingCmnInfo->Private.bStateAutoAppendInList = 1;

        _ET9InvalidateSymbInfo(pLingInfo->pLingCmnInfo->Base.pWordSymbInfo);
    }

    return wStatus;
}

/*---------------------------------------------------------------------------*/
/** \ingroup et9asys
 * Clear auto append in list.
 * This function clears that auto append's should be included in lists.
 *
 * @param pLingInfo                 Pointer to alphabetic information structure.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL ET9AWClearAutoAppendInList(ET9AWLingInfo * const pLingInfo)
{
    ET9STATUS           wStatus;

    wStatus = _ET9AWSys_BasicValidityCheck(pLingInfo);

    if (wStatus) {
        return wStatus;
    }

    wStatus = _ET9SettingsInhibited(pLingInfo->pLingCmnInfo->Base.pWordSymbInfo);

    if (wStatus) {
        return wStatus;
    }

    if (pLingInfo->pLingCmnInfo->Private.bStateAutoAppendInList) {

        pLingInfo->pLingCmnInfo->Private.bStateAutoAppendInList = 0;

        _ET9InvalidateSymbInfo(pLingInfo->pLingCmnInfo->Base.pWordSymbInfo);
    }

    return wStatus;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * This function gets special info.
 *
 * @param pLingInfo                 Pointer to alphabetic information structure.
 * @param wStartPoint               Start point for search for special char.
 * @param pwPosition                (out) container for char position.
 * @param peType                    (out) special char type (LOCKPOINT, EXPLICIT, etc).
 *
 * @return None
 */

static void ET9LOCALCALL __ET9GetSpecialCharInfo(ET9AWLingInfo      * const pLingInfo,
                                                 const ET9U16               wStartPoint,
                                                 ET9U16             * const pwPosition,
                                                 ET9ASPECIAL        * const peType)
{
    ET9S16 swIndex;
    ET9SymbInfo *pSymbInfo;
    ET9UINT nTraceSegCount = 0;

    ET9WordSymbInfo * const pWordSymbInfo = pLingInfo->pLingCmnInfo->Base.pWordSymbInfo;

    ET9AssertLog(pWordSymbInfo != NULL);
    ET9AssertLog(pwPosition != NULL);
    ET9AssertLog(peType != NULL);

    WLOG2(fprintf(pLogFile2, "__ET9GetSpecialCharInfo, wStartPoint = %d, bNumSymbs = %d\n", wStartPoint, pWordSymbInfo->bNumSymbs);)

    *pwPosition = 0;
    *peType = NOSPECIAL;

    if (!pWordSymbInfo->bNumSymbs) {
        WLOG2(fprintf(pLogFile2, "..called with NO symbs\n");)
        return;
    }

    swIndex = (ET9S16)((ET9S16)pWordSymbInfo->bNumSymbs - 1);

    ET9AssertLog(swIndex < ET9MAXWORDSIZE);

    pSymbInfo = &pWordSymbInfo->SymbsInfo[swIndex];
    for ( ; swIndex >= (ET9S16)wStartPoint; --swIndex, --pSymbInfo) {

        WLOG2(fprintf(pLogFile2, "..trying swIndex %d (position %d)\n", swIndex, (swIndex + 1 - wStartPoint));)

        if (pSymbInfo->bLocked) {
            WLOG2(fprintf(pLogFile2, "..found LOCKPOINT\n");)
            *peType = LOCKPOINT;
            break;
        }

        /* skipping traced symbs */

        if (pSymbInfo->bTraceIndex) {

            if (nTraceSegCount || swIndex == (ET9S16)wStartPoint) {
                WLOG2(fprintf(pLogFile2, "..skipping traced symb (1)\n");)
                continue;
            }

            if (pSymbInfo->bTraceIndex == (pSymbInfo - 1)->bTraceIndex) {
                WLOG2(fprintf(pLogFile2, "..skipping traced symb (2)\n");)
                continue;
            }

            ++nTraceSegCount;

            WLOG2(fprintf(pLogFile2, "..going to examine one trace symb\n");)
        }

        /* if symb is last in the input and exact, do NOT make it special, handled by auto append exact */

        if (swIndex + 1 == pWordSymbInfo->bNumSymbs && __IsExactSymb(pSymbInfo)) {
            WLOG2(fprintf(pLogFile2, "..skipping trailing EXPLICIT (from single symbol)\n");)
            continue;
        }

        /* if the character is "explicit"... */

        if (pSymbInfo->bAmbigType == ET9EXACT && _ET9_IsPunctOrNumeric(pSymbInfo->DataPerBaseSym[0].sChar[0])) {
            WLOG2(fprintf(pLogFile2, "..found EXPLICIT (from exact)\n");)
            *peType = EXPLICIT;
            break;
        }
        if (pSymbInfo->bSymbType == ET9KTPUNCTUATION) {
            WLOG2(fprintf(pLogFile2, "..found PUNCT\n");)
            *peType = PUNCT;
            break;
        }
        if (pSymbInfo->bSymbType == ET9KTSMARTPUNCT) {
            WLOG2(fprintf(pLogFile2, "..found SMARTPUNCT\n");)
            *peType = SMARTPUNCT;
            break;
        }
        if (__IsExactSymb(pSymbInfo) && _ET9_IsPunctOrNumeric(pSymbInfo->DataPerBaseSym[0].sChar[0])) {
            WLOG2(fprintf(pLogFile2, "..found EXPLICIT (from single symbol)\n");)
            *peType = EXPLICIT;
            break;
        }
    }

    if (*peType != NOSPECIAL) {
        *pwPosition = swIndex + 1 - wStartPoint;
        WLOG2(fprintf(pLogFile2, "..SPECIAL, wPosition = %d (wStartPoint %d)\n", *pwPosition, wStartPoint);)
    }
    else {
        WLOG2(fprintf(pLogFile2, "..NOSPECIAL\n");)
    }

    return;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Decides if to suppress the exact in list.
 *
 * @param pLingInfo         Pointer to alphabetic information structure.
 *
 * @return Non zero to suppress, otherwise zero.
 */

static ET9BOOL ET9LOCALCALL __SuppressExact(ET9AWLingInfo * const pLingInfo)
{
    ET9AWLingCmnInfo    * const pLingCmnInfo = pLingInfo->pLingCmnInfo;
    ET9WordSymbInfo     * const pWordSymbInfo = pLingCmnInfo->Base.pWordSymbInfo;

    ET9UINT nCount;
    ET9SymbInfo *pSymbInfo;

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
 * Check if exact should be in the list.
 *
 * @param pLingInfo         Pointer to alphabetic information structure.
 *
 * @return Non zero to be in list, otherwise zero.
 */

static ET9BOOL ET9LOCALCALL __ExactInList(ET9AWLingInfo * const pLingInfo)
{
    ET9AWLingCmnInfo    * const pLingCmnInfo = pLingInfo->pLingCmnInfo;
    ET9WordSymbInfo     * const pWordSymbInfo = pLingCmnInfo->Base.pWordSymbInfo;

    const ET9U8         bNumSymbs = pWordSymbInfo->bNumSymbs;
    const ET9BOOL       bLastInputIsMultitap = (ET9BOOL)(bNumSymbs && pWordSymbInfo->SymbsInfo[bNumSymbs - 1].eInputType == ET9MULTITAPKEY);

    return (ET9BOOL)(ET9EXACTINLIST(pLingCmnInfo) && !__SuppressExact(pLingInfo) || bLastInputIsMultitap);
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Decides if the input has override inhibitor.
 *
 * @param[in]     pLingCmnInfo          Pointer to alphabetic cmn information structure.
 *
 * @return Non zero if has, otherwise zero.
 */

static ET9BOOL ET9LOCALCALL __ExactHasOverrideInhibitor(ET9AWLingCmnInfo    * const pLingCmnInfo)
{
    ET9WordSymbInfo const * const pWordSymbInfo = pLingCmnInfo->Base.pWordSymbInfo;

    WLOG2(fprintf(pLogFile2, "__ExactHasOverrideInhibitor\n");)

    /* check digits, puncts etc in the input */

    {
        ET9UINT nAtCount = 0;
        ET9UINT nPunctCount = 0;
        ET9UINT nNumberCount = 0;

        ET9UINT nCount;
        ET9SymbInfo const *pSymbInfo;

        pSymbInfo = pWordSymbInfo->SymbsInfo;
        for (nCount = pWordSymbInfo->bNumSymbs; nCount; --nCount, ++pSymbInfo) {

            const ET9SYMB sSymb = pSymbInfo->DataPerBaseSym[0].sChar[0];

            if (sSymb == '@') {
                ++nAtCount;
            }

            switch (pSymbInfo->bSymbType)
            {
                case ET9KTLETTER:
                    break;
                case ET9KTNUMBER:
                    ++nNumberCount;
                    break;
                case ET9KTPUNCTUATION:
                    ++nPunctCount;
                    break;
                case ET9KTSMARTPUNCT:
                    {
                        const ET9SymbClass eClass = _ET9_GetSymbolClass(sSymb);

                        if (eClass == ET9_NumbrSymbClass) {
                            ++nNumberCount;
                        }
                        else {
                            ++nPunctCount;
                        }
                    }
                    break;
                default:
                    {
                        const ET9SymbClass eClass = _ET9_GetSymbolClass(sSymb);

                        if (eClass == ET9_NumbrSymbClass) {
                            ++nNumberCount;
                        }
                        else if (eClass == ET9_PunctSymbClass) {
                            ++nPunctCount;
                        }
                    }
                    break;
            }
        }

        WLOG2(fprintf(pLogFile2, "  nAtCount %u, nNumberCount %u, nPunctCount %u\n", nAtCount, nNumberCount, nPunctCount);)

        if (nAtCount || nNumberCount || nPunctCount > 1) {
            WLOG2(fprintf(pLogFile2, "  YES\n");)
            return 1;
        }
    }

    /* the rest is limited to low & medium */

    switch (pLingCmnInfo->Private.eSelectionListCorrectionMode)
    {
        case ET9ASLCORRECTIONMODE_LOW:
        case ET9ASLCORRECTIONMODE_MEDIUM:
            break;
        default:
            WLOG2(fprintf(pLogFile2, "  High -> NO\n");)
            return 0;
    }

    /* check completion */

    {
        ET9AWPrivWordInfo const * const pWord = &pLingCmnInfo->Private.pWordList[pLingCmnInfo->Private.bWordList[1]];

        /* not when completion (not term) */

        if (!pWord->Base.bIsTerm) {
            WLOG2(fprintf(pLogFile2, "  !Term -> YES\n");)
            return 1;
        }
    }

    /* done */

    WLOG2(fprintf(pLogFile2, "  NO\n");)

    return 0;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Decides if the input has user shift override inhibit.
 *
 * @param[in]     pLingCmnInfo          Pointer to alphabetic cmn information structure.
 *
 * @return Non zero if has, otherwise zero.
 */

static ET9BOOL ET9LOCALCALL __HasUserShiftOverrideInhibit(ET9AWLingCmnInfo    * const pLingCmnInfo)
{
    ET9WordSymbInfo const * const pWordSymbInfo = pLingCmnInfo->Base.pWordSymbInfo;

    WLOG2(fprintf(pLogFile2, "__HasUserShiftOverrideInhibit\n");)

    if (pWordSymbInfo->bNumSymbs < 2) {
        WLOG2(fprintf(pLogFile2, "  bNumSymbs < 2 -> NO\n");)
        return 0;
    }

    if (pWordSymbInfo->SymbsInfo[0].eShiftState != ET9SHIFT) {
        WLOG2(fprintf(pLogFile2, "  eShiftState != ET9SHIFT -> NO\n");)
        return 0;
    }

    if (!pLingCmnInfo->Private.bCurrBuildHasShiftSignificance) {
        WLOG2(fprintf(pLogFile2, "  !bCurrBuildHasShiftSignificance -> NO\n");)
        return 0;
    }

    {
        ET9U16 wContextLen = pLingCmnInfo->Private.bContextWordSize;

        if (wContextLen < ET9MAXWORDSIZE) {
            pLingCmnInfo->Private.sContextWord[wContextLen++] = ' ';
        }

        if (_ET9IsAutoCapSituation(pWordSymbInfo, pLingCmnInfo->Private.sContextWord, wContextLen)) {
            WLOG2(fprintf(pLogFile2, "  IsAutoCapSituation -> NO\n");)
            return 0;
        }
    }

    {
        ET9WordSymbInfo const * const pWordSymbInfo = pLingCmnInfo->Base.pWordSymbInfo;
        ET9AWPrivWordInfo const * const pWord = &pLingCmnInfo->Private.pWordList[pLingCmnInfo->Private.bWordList[1]];

        /* spc can't win */

        if (pWord->bEditDistSpc) {
            WLOG2(fprintf(pLogFile2, "  bEditDistSpc -> YES\n");)
            return 1;
        }

        /* upper wins */

        if (_ET9SymIsUpper(pWord->sPureFirstChar, pWordSymbInfo->Private.wLocale)) {
            WLOG2(fprintf(pLogFile2, "  upper(firstChar) -> NO\n");)
            return 0;
        }

        /* wrong length can't win */

        if (pWord->Base.wWordLen != pWordSymbInfo->bNumSymbs) {
            WLOG2(fprintf(pLogFile2, "  wWordLen != bNumSymbs -> YES\n");)
            return 1;
        }

        /* certain amount of regional correction can win */

        if (pWord->bEditDistStem <= __MaxStemDistanceFromInputLength(pWordSymbInfo->bNumSymbs)) {
            WLOG2(fprintf(pLogFile2, "  bEditDistStem <= MaxStemDistance -> NO\n");)
            return 0;
        }

        /* the rest can't win */

        WLOG2(fprintf(pLogFile2, "  YES\n");)

        return 1;
    }
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Decides if the input has user shift override inhibit.
 *
 * @param[in]     pLingCmnInfo          Pointer to alphabetic cmn information structure.
 *
 * @return Non zero if has, otherwise zero.
 */

static ET9BOOL ET9LOCALCALL __HasStemDistanceOverrideInhibit(ET9AWLingCmnInfo    * const pLingCmnInfo)
{
    ET9WordSymbInfo const * const pWordSymbInfo = pLingCmnInfo->Base.pWordSymbInfo;

    switch (pLingCmnInfo->Private.eSelectionListCorrectionMode)
    {
        case ET9ASLCORRECTIONMODE_LOW:
        case ET9ASLCORRECTIONMODE_MEDIUM:
            break;
        default:
            return 0;
    }

    if (pWordSymbInfo->bNumSymbs >= 5) {

        ET9AWPrivWordInfo const * const pWord = &pLingCmnInfo->Private.pWordList[pLingCmnInfo->Private.bWordList[1]];

        if (pWord->bEditDistStem > pWordSymbInfo->bNumSymbs / 2) {
            return 1;
        }
    }

    if (pWordSymbInfo->bNumSymbs >= 1) {

        ET9AWPrivWordInfo const * const pWord = &pLingCmnInfo->Private.pWordList[pLingCmnInfo->Private.bWordList[1]];

        if (!pWord->bIsTop5) {

            if ((pWord->bEditDistSpc - pWord->bEditDistSpcTrp) > 1) {
                return 1;
            }

            if (pWord->bEditDistStem > __MaxStemDistanceFromInputLength(pWordSymbInfo->bNumSymbs)) {
                return 1;
            }
        }
    }

    return 0;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Check if the string stem is all numeric (can have explicit tail).
 *
 * @param[in]     pLingCmnInfo          Pointer to alphabetic cmn information structure.
 * @param[in]     psString              Pointer to string.
 * @param[in]     nStrLen               String length.
 *
 * @return Non zero if has, otherwise zero.
 */

static ET9BOOL ET9LOCALCALL __HasNumericStem(ET9AWLingCmnInfo    * const pLingCmnInfo,
                                             ET9SYMB       const * const psString,
                                             const ET9UINT               nStrLen)
{
    ET9WordSymbInfo const * const pWordSymbInfo = pLingCmnInfo->Base.pWordSymbInfo;

    ET9INT snIndex;

    for (snIndex = pWordSymbInfo->bNumSymbs - 1; snIndex >= 0; --snIndex) {
        if (pWordSymbInfo->SymbsInfo[snIndex].eInputType != ET9EXPLICITSYM) {
            break;
        }
    }

    if (snIndex < 0) {
        return 0;
    }

    {
        ET9UINT nStemLen = (ET9UINT)(snIndex + 1);

        if (nStemLen > nStrLen) {
            return 0;
        }

        return _ET9_IsNumericString(psString, nStemLen);
    }
}

/*---------------------------------------------------------------------------*/
/**
 * This function builds selection list for the current tap sequence.
 * If needed it will perform a number of list builds to catchup on the input changes.
 *
 * @param pLingInfo                 Pointer to alphabetic information structure.
 * @param pbTotalWords              (out) number of candidate words found.
 * @param pbDefaultListIndex        (out) suggested default candidate.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL ET9AWSelLstBuild(ET9AWLingInfo     * const pLingInfo,
                                      ET9U8             * const pbTotalWords,
                                      ET9U8             * const pbDefaultListIndex)
{
    ET9U8           i;
    ET9U8           bNumSymbs;
    ET9U8           bLastBuildLen;
    ET9U8           bValidLen;
    ET9U8           bLockLen;
    ET9STATUS       wStatus;
    ET9SymbInfo     *pSymbInfo;

    ET9WordSymbInfo * pWordSymbInfo;
    ET9AWLingCmnInfo * pLingCmnInfo;

    WLOG2Shrink(pLogFile2);

    WLOG2(fprintf(pLogFile2, "\n************************* SELECTION LIST BUILD START *************************\n\n");)

    wStatus = _ET9AWSys_BasicValidityCheck(pLingInfo);

    if (wStatus) {
        return wStatus;
    }

    if (!pbTotalWords || !pbDefaultListIndex) {
        return ET9STATUS_INVALID_MEMORY;
    }

    *pbTotalWords = 0;
    *pbDefaultListIndex = 0;

#ifdef EVAL_BUILD
    {
        ET9U16 wUpdateCount = 0;
        if (pLingInfo->pLingCmnInfo->pRUDBInfo) {
            wUpdateCount = pLingInfo->pLingCmnInfo->pRUDBInfo->wUpdateCounter;
        }
        if (_ET9Eval_HasExpired(&pLingInfo->pLingCmnInfo->Base, wUpdateCount)) {
            return ET9STATUS_EVAL_BUILD_EXPIRED;
        }
    }
#endif /*EVAL_BUILD*/

    pLingCmnInfo = pLingInfo->pLingCmnInfo;
    pWordSymbInfo = pLingCmnInfo->Base.pWordSymbInfo;

    if (!pWordSymbInfo || pWordSymbInfo->wInitOK != ET9GOODSETUP) {
        return ET9STATUS_INVALID_MEMORY;
    }

    if (pWordSymbInfo->bNumSymbs > ET9MAXWORDSIZE) {
        _ET9AWSelLstResetWordList(pLingInfo);
        WLOG2(fprintf(pLogFile2, "too many symbs - aborting\n");)
        return ET9STATUS_ERROR;
    }

    __LogInputSymbs(pLingInfo, pWordSymbInfo->bNumSymbs, pLogFile2);

    __LogUDB(pLingInfo, pLogFile2);
    __LogCDB(pLingInfo, pLogFile2);

    if (pLingCmnInfo->Base.bSelListInvalidated) {

        pLingCmnInfo->Base.bSelListInvalidated = 0;

        WLOG2(fprintf(pLogFile2, "selection list invalidated - resetting\n");)

        __ResetAllCaptureInfo(pLingInfo);
        _ET9AWSelLstResetWordList(pLingInfo);

        pLingCmnInfo->Private.bLastBuildShrinking = 0;

        pLingCmnInfo->Private.bSpcDuringBuild = 0;
        pLingCmnInfo->Private.bExpandAsDuringBuild = 0;
        pLingCmnInfo->Private.bLastBuildLen = pWordSymbInfo->bNumSymbs;
        pLingCmnInfo->Private.bTotalSymbInputs = pWordSymbInfo->bNumSymbs;

        {
            ET9U8 bIndex;

            for (bIndex = 0; bIndex < pWordSymbInfo->bNumSymbs; ++bIndex) {
                pLingCmnInfo->Private.pbLangSupported[bIndex] = ET9AWBOTH_LANGUAGES;

                if (pWordSymbInfo->Private.bRequiredLocate) {
                    pLingCmnInfo->Base.bSymbInvalidated[bIndex] = 0;
                }
            }
        }

        if (pWordSymbInfo->Private.sRequiredWord.wLen && pWordSymbInfo->bNumSymbs) {

            ET9U16 wSymbolLen;
            ET9AWPrivWordInfo sWord;

            _ET9SimpleWordToPrivWord(&pWordSymbInfo->Private.sRequiredWord, &sWord);

            WLOG2Word(pLogFile2, "required word sets up defaults, word", &sWord);

            for (wSymbolLen = 1; wSymbolLen < pWordSymbInfo->bNumSymbs; ++wSymbolLen) {

                if (wSymbolLen <= pWordSymbInfo->Private.sRequiredWord.wLen) {
                    sWord.Base.wWordCompLen = (ET9U16)(sWord.Base.wWordLen - wSymbolLen);
                }

                __CaptureDefault(pLingInfo, &sWord, (ET9U16)(wSymbolLen + 1), 0);
            }
        }
    }

    bNumSymbs = pWordSymbInfo->bNumSymbs;
    bLastBuildLen = pLingCmnInfo->Private.bLastBuildLen;

    pLingCmnInfo->Private.xMaxWordFreq = 0;

    if (pLingCmnInfo->Base.bSymbsInfoInvalidated) {
        WLOG2(fprintf(pLogFile2, "modified symbols since last build\n\n");)
        pLingCmnInfo->Base.bSymbsInfoInvalidated = 0;
    }
    else {
        WLOG2(fprintf(pLogFile2, "*** building without any symbols being modified since last build\n\n");)
    }

    WLOG2(fprintf(pLogFile2, "pLingInfo %p, pWordSymbInfo %p, bNumSymbs = %d, bLastBuildLen = %d\n\n", pLingInfo, pWordSymbInfo, bNumSymbs, bLastBuildLen);)

#ifdef ET9_ACTIVATE_SLST_STATS
    _ET9AWSpcClearStats(pLingInfo);
#endif
#ifdef ET9_PRE_DUPE_SELLIST
    /* clear previous contents */
    pPreDupeFile = fopen(DEFAULT_PREDUPE_FILE_NAME, "w,ccs=UNICODE");
    fclose(pPreDupeFile);
    pPreDupeFile = fopen(DEFAULT_PREDUPE_FILE_NAME, "a+,ccs=UNICODE");
    nPreDupeEntryCount = 0;
#endif /* ET9_PRE_DUPE_SELLIST */

    /* handle tracking */

    if (!bLastBuildLen) {
        pLingCmnInfo->Private.bSpcDuringBuild = 0;
        pLingCmnInfo->Private.bExpandAsDuringBuild = 0;
    }
    if (ET9_SPC_IS_ACTIVE(pLingCmnInfo->Private.ASpc.eMode)) {
        pLingCmnInfo->Private.bSpcDuringBuild = 1;
    }
    if (ET9EXPANDAUTOSUB(pLingCmnInfo)) {
        pLingCmnInfo->Private.bExpandAsDuringBuild = 1;
    }

    WLOG2(fprintf(pLogFile2, "bSpcDuringBuild = %d, bExpandAsDuringBuild = %d\n\n", pLingCmnInfo->Private.bSpcDuringBuild, pLingCmnInfo->Private.bExpandAsDuringBuild);)

    /* handle language change (rebuild all if changed) */

    if ((pLingCmnInfo->wFirstLdbNum != pLingCmnInfo->Private.wCurrBuildLang) ||
        (pLingCmnInfo->wSecondLdbNum != pLingCmnInfo->Private.wCurrBuildSecondLanguage)) {

        for (i = 0; i < bNumSymbs; ++i) {
            pLingCmnInfo->Base.bSymbInvalidated[i] = 1;
            pLingCmnInfo->Base.bLockInvalidated[i] = 0;    /* or it will uninvalidate the symb */
        }

        WLOG2(fprintf(pLogFile2, "*** language changed (lang %04X -> %04X)\n\n", pLingCmnInfo->Private.wCurrBuildLang, pLingCmnInfo->wLdbNum);)

        pLingCmnInfo->Private.wCurrBuildLang = pLingCmnInfo->wFirstLdbNum;
        pLingCmnInfo->Private.wCurrBuildSecondLanguage = pLingCmnInfo->wSecondLdbNum;
    }

    /* handle lock invalidation */

    bLockLen = 0;
    i = pWordSymbInfo->bNumSymbs;
    if (i) {
        pSymbInfo = &pWordSymbInfo->SymbsInfo[i - 1];
        for (; i; --i, --pSymbInfo) {
            if (pSymbInfo->bLocked) {
                bLockLen = i;
            }
        }
    }

    for (i = 0; i < bNumSymbs; ++i) {

        if (pLingCmnInfo->Base.bLockInvalidated[i]) {

            pLingCmnInfo->Base.bLockInvalidated[i] = 0;

            if (i < bLockLen) {
                pLingCmnInfo->Base.bSymbInvalidated[i] = 0;
                WLOG2(fprintf(pLogFile2, "symbol locked @ %d, within lock => uninvalidate symbol\n\n", i+1);)
            }
            else {
                WLOG2(fprintf(pLogFile2, "symbol locked @ %d, outside lock => symbol invalidation state remains\n\n", i+1);)
            }
        }
    }

    /* calculate valid (non invalidated) len */

    for (bValidLen = 0; bValidLen < bNumSymbs; ++bValidLen) {

        if (pLingCmnInfo->Base.bSymbInvalidated[bValidLen]) {
            break;
        }
    }

    /* want to build (back) to valid len in case there were clears and adds in between */

    if (bValidLen < bNumSymbs && bValidLen < bLastBuildLen) {

        if (!bValidLen) {

            WLOG2(fprintf(pLogFile2, "performing catchup clear all\n\n");)

            bLastBuildLen = 0;
            pLingCmnInfo->Private.bLastBuildLen = bLastBuildLen;
            pLingCmnInfo->Private.sBuildInfo.bCaptureInvalidated = 1;
        }
        else {

            WLOG2(fprintf(pLogFile2, "performing catchup clear build @ %d\n\n", bValidLen);)

            pWordSymbInfo->bNumSymbs = bValidLen;

            wStatus = __ET9AWDoSelLstBuild(pLingInfo, pbTotalWords, 0);

            __LogFullSelList(pLingInfo, "after catchup clear build", pLogFile2);

            bLastBuildLen = pWordSymbInfo->bNumSymbs;
            pLingCmnInfo->Private.bLastBuildLen = bLastBuildLen;
        }
    }

    /* want to build from last build length to current nNumSymbs in case there were multiple inputs in between */

    if ((bLastBuildLen + 1) < bNumSymbs) {
        WLOG2(fprintf(pLogFile2, "performing catchup builds, %d -> %d\n\n", bLastBuildLen + 1, bNumSymbs - 1);)
    }

    for (i = bLastBuildLen + 1; i < bNumSymbs; ++i) {

        ET9BOOL bSuppressBuild;

        bSuppressBuild = 0;

        pWordSymbInfo->bNumSymbs = i;

        if (pWordSymbInfo->SymbsInfo[i - 1].bTraceIndex &&
            pWordSymbInfo->SymbsInfo[i - 1].bTraceIndex == pWordSymbInfo->SymbsInfo[i].bTraceIndex) {

            if (__IsExactSymb(&pWordSymbInfo->SymbsInfo[i])) {

                WLOG2(fprintf(pLogFile2, "calculating single catchup - has trace info with significant punctuation (%u symbs)\n", i);)
            }
            else {
                WLOG2(fprintf(pLogFile2, "skipping single catchup - has trace info (%u symbs)\n", i);)
                bSuppressBuild = 1;
            }
        }
        else if (pWordSymbInfo->SymbsInfo[i - 1].bAmbigType == ET9EXACT) {

            WLOG2(fprintf(pLogFile2, "skipping single catchup - is exact (%u symbs)\n", i);)
            bSuppressBuild = 1;
        }
        else if (pWordSymbInfo->SymbsInfo[i - 1].wInputIndex) {

            WLOG2(fprintf(pLogFile2, "skipping single catchup - has multi info (%u symbs)\n", i);)
            bSuppressBuild = 1;
        }
        else if (pLingCmnInfo->Base.bContentExplicified) {
            WLOG2(fprintf(pLogFile2, "skipping single catchup - has explicification (%u symbs)\n", i);)
            bSuppressBuild = 1;
        }

        wStatus = __ET9AWDoSelLstBuild(pLingInfo, pbTotalWords, bSuppressBuild);

        __LogFullSelList(pLingInfo, "after catchup build", pLogFile2);

        pLingCmnInfo->Private.bLastBuildLen = i;

        if (!bSuppressBuild && (wStatus || !*pbTotalWords)) {
            WLOG2(fprintf(pLogFile2, "Catchup build gave no result...\n\n");)
        }
    }

    pWordSymbInfo->bNumSymbs = bNumSymbs;

    pLingCmnInfo->Base.bContentExplicified = 0;

    wStatus = __ET9AWDoSelLstBuild(pLingInfo, pbTotalWords, 0);

    __LogFullSelList(pLingInfo, "after normal build", pLogFile2);

    pLingCmnInfo->Private.bLastBuildLen = bNumSymbs;

    /* following code applies whether previous build was successful or not */
    /* set whether exact was in list for this build */

    pLingCmnInfo->Private.dwStateBits &= ~ET9SLEXACTINLIST;
    if  (pWordSymbInfo->bNumSymbs && __ExactInList(pLingInfo)) {
        pLingCmnInfo->Private.dwStateBits |= ET9SLEXACTINLIST;
    }

    *pbDefaultListIndex = pLingCmnInfo->Private.bDefaultIndex;

    WLOG2(fprintf(pLogFile2, "pbDefaultListIndex = %d\n", *pbDefaultListIndex);)

    if (!pLingCmnInfo->Private.bTotalWords) {
        wStatus = ET9STATUS_NO_MATCHING_WORDS;
    }

#ifdef ET9_PRE_DUPE_SELLIST
    if (pPreDupeFile) {
        fflush(pPreDupeFile);
        fclose(pPreDupeFile);
        pPreDupeFile = NULL;
    }
#endif /* ET9_PRE_DUPE_SELLIST */

    /* handle input verification against the required word */

    if (pWordSymbInfo->Private.bRequiredVerifyInput) {

        pWordSymbInfo->Private.bRequiredVerifyInput = 0;

        if (!pLingCmnInfo->Private.bRequiredFound) {

            ET9SimpleWord sWord = pWordSymbInfo->Private.sRequiredWord;

            WLOG2(fprintf(pLogFile2, "Required word not found on input verification - modifying input and rebuilding\n");)

            _ET9ExplicifyWord(pWordSymbInfo, &sWord);

            pWordSymbInfo->Private.sRequiredWord = sWord;
            pWordSymbInfo->Private.bRequiredLocate = 1;
            pWordSymbInfo->Private.bRequiredInhibitOverride = 0;
            pWordSymbInfo->Private.bRequiredInhibitCorrection = 0;
            wStatus = ET9AWSelLstBuild(pLingInfo, pbTotalWords, pbDefaultListIndex);
        }
    }

    WLOG2(fprintf(pLogFile2, "\n.......................... selection list build end ..........................\n\n");)

    return wStatus;
}

/*---------------------------------------------------------------------------*/
/**
 * This function gets the inline word for the current input.
 * It's suggested to show this string inlined in the input buffer before the user potentially moves the active word in the selection list.
 *
 * @param pLingInfo                 Pointer to alphabetic information structure.
 * @param pWord                     Pointer to an adddress of a word.
 * @param pbCorrection              Pointer to a flag that will be non zero if the string by default will be corrected, otherwise zero.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL ET9AWSelLstGetInlineWord(ET9AWLingInfo        * const pLingInfo,
                                              ET9SimpleWord        * const pWord,
                                              ET9BOOL              * const pbCorrection)
{
    ET9STATUS wStatus;

    if ((wStatus = _ET9AWSys_BasicValidityCheck(pLingInfo)) != ET9STATUS_NONE) {
        return wStatus;
    }

    if (!pWord || !pbCorrection) {
        return ET9STATUS_INVALID_MEMORY;
    }

    pWord->wLen = 0;
    pWord->wCompLen = 0;
    *pbCorrection = 0;

    {
        ET9AWLingCmnInfo * const pLingCmnInfo = pLingInfo->pLingCmnInfo;

        ET9U8 bInlineIndex;

        if (pLingCmnInfo->Base.bSymbsInfoInvalidated) {
            return ET9STATUS_NEED_SELLIST_BUILD;
        }
        if (pLingCmnInfo->Base.pWordSymbInfo == NULL || pLingCmnInfo->Base.pWordSymbInfo->wInitOK != ET9GOODSETUP) {
            return ET9STATUS_INVALID_MEMORY;
        }
        if (pLingCmnInfo->Private.bTotalSymbInputs != pLingCmnInfo->Base.pWordSymbInfo->bNumSymbs) {   /* redundant test - obsolete */
            return ET9STATUS_ERROR;
        }
        if (!pLingCmnInfo->Private.bTotalWords) {
            return ET9STATUS_NONE;
        }

        bInlineIndex = 0;

        if (pLingCmnInfo->Private.bRequiredFound) {
            bInlineIndex = pLingCmnInfo->Private.bDefaultIndex;
        }
        else {

            ET9AWWordInfo * const pWordZero = &pLingCmnInfo->Private.pWordList[pLingCmnInfo->Private.bWordList[0]].Base;

            if (_ET9_IsNumericString(pWordZero->sWord, pWordZero->wWordLen)) {
                bInlineIndex = pLingCmnInfo->Private.bDefaultIndex;
            }
        }

        /* transfer word info */

        {
            ET9AWPrivWordInfo * const pWordInline = &pLingCmnInfo->Private.pWordList[pLingCmnInfo->Private.bWordList[bInlineIndex]];

            _ET9SymCopy(pWord->sString, pWordInline->Base.sWord, pWordInline->Base.wWordLen);

            pWord->wLen = pWordInline->Base.wWordLen;
            pWord->wCompLen = pWordInline->Base.wWordCompLen;
        }

        /* correction? */

        if (bInlineIndex != pLingCmnInfo->Private.bDefaultIndex) {
            *pbCorrection = 1;
        }
    }

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/**
 * This function gets an indexed word from the word list.
 *
 * @param pLingInfo                 Pointer to alphabetic information structure.
 * @param pWord                     Pointer to an adddress of a word.
 * @param byWordIndex               Word index of base 0.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL ET9AWSelLstGetWord(ET9AWLingInfo       * const pLingInfo,
                                        ET9AWWordInfo      ** const pWord,
                                        const ET9U8                 byWordIndex)
{
    ET9STATUS wStatus;

    if ((wStatus = _ET9AWSys_BasicValidityCheck(pLingInfo)) != ET9STATUS_NONE) {
        return wStatus;
    }

    if (!pWord) {
        return ET9STATUS_INVALID_MEMORY;
    }

    *pWord = NULL;

    {
        ET9AWLingCmnInfo * const pLingCmnInfo = pLingInfo->pLingCmnInfo;

        if (pLingCmnInfo->Base.bSymbsInfoInvalidated) {
            return ET9STATUS_NEED_SELLIST_BUILD;
        }
        if (pLingCmnInfo->Base.pWordSymbInfo == NULL || pLingCmnInfo->Base.pWordSymbInfo->wInitOK != ET9GOODSETUP) {
            return ET9STATUS_INVALID_MEMORY;
        }
        if (pLingCmnInfo->Private.bTotalSymbInputs != pLingCmnInfo->Base.pWordSymbInfo->bNumSymbs) {   /* redundant test - obsolete */
            return ET9STATUS_ERROR;
        }
        if (byWordIndex >= pLingCmnInfo->Private.bTotalWords) {
            return ET9STATUS_OUT_OF_RANGE;
        }

        *pWord = &pLingCmnInfo->Private.pWordList[pLingCmnInfo->Private.bWordList[byWordIndex]].Base;
    }

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * This function strips off symbols and emoticons.
 *
 * @param pWord                     Word
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL _ET9AWSelLstStripActualTaps(ET9AWPrivWordInfo * const pWord)
{
    ET9UINT nWordLen;
    ET9UINT nIncomingWordLen;
    ET9SYMB sSymbol;
    ET9UINT nNumDigt = 0;
    ET9UINT nNumPunct = 0;
    ET9UINT i;
    ET9UINT nLeadStrips = 0;

    ET9AssertLog(pWord != NULL);

    nWordLen = pWord->Base.wWordLen;
    nIncomingWordLen = nWordLen;

    while (nWordLen--) {
        sSymbol = pWord->Base.sWord[nWordLen];

         /* Count punctuation and digits */

        if (_ET9_IsNumeric(sSymbol)) {
            ++nNumDigt;
        }
        else if (_ET9_IsPunctChar(sSymbol)) {
            ++nNumPunct;
        }
    }

    nWordLen = pWord->Base.wWordLen;

    if (nNumPunct && ((nNumDigt + nNumPunct < nWordLen - nNumDigt - nNumPunct) || (nNumDigt + nNumPunct > EMOTICON_LENGTH_THRESHOLD))) {

        while (_ET9_IsPunctChar(pWord->Base.sWord[nLeadStrips]) && nWordLen--) {
            ++nLeadStrips;
        }
        for (i = nLeadStrips; i > 0 && i < pWord->Base.wWordLen; ++i) {
            pWord->Base.sWord[i - nLeadStrips] = pWord->Base.sWord[i];
        }
        pWord->Base.wWordLen = (ET9U16) (pWord->Base.wWordLen - nLeadStrips);
        nWordLen = pWord->Base.wWordLen;
        while (nWordLen && _ET9_IsPunctChar(pWord->Base.sWord[nWordLen - 1])) {
            if (pWord->Base.wWordLen) {
                --pWord->Base.wWordLen;
            }
            if (pWord->Base.wWordCompLen) {
                --pWord->Base.wWordCompLen;
            }
            --nWordLen;
        }
    }

    if (pWord->Base.wWordLen == nIncomingWordLen) {
        return ET9STATUS_NO_OPERATION;
    }

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Word processing.
 * This function does the processing on the individual words that are candidates for udb or rdb entry.
 *
 * @param pLingInfo                 Pointer to alphabetic information structure.
 * @param pText                     Pointer to text to prcocess.
 * @param wSize                     Size of string.
 * @param bLangIndex                Language index.
 * @param nProcessType              Type of processing, either "mark use" or "note done".
 * @param bWordSrc                  Word source.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

static ET9STATUS ET9LOCALCALL __ET9AWDoWordProcessing(ET9AWLingInfo * const     pLingInfo,
                                                      ET9SYMB * const           pText,
                                                      const ET9U16              wSize,
                                                      const ET9U8               bLangIndex,
                                                      const ET9UINT             nProcessType,
                                                      const ET9U8               bWordSrc)
{
    ET9STATUS           wStatus;
    ET9AWPrivWordInfo   sWord;

    ET9AssertLog(pLingInfo);
    ET9AssertLog(pText);
    ET9AssertLog((nProcessType == USE_PROCESSING) || (nProcessType == SPACE_PROCESSING));
    ET9AssertLog(wSize <= ET9MAXWORDSIZE);

    if (pLingInfo->Private.wInfoInitOK != ET9GOODSETUP) {
        return ET9STATUS_NO_INIT;
    }
    if (!wSize) {
        return ET9STATUS_NONE;
    }

    /* _ET9AWSuppDBAddSelection only needs the length and character data */

    _InitPrivWordInfo(&sWord);

    sWord.Base.wWordLen = wSize;
    _ET9SymCopy(sWord.Base.sWord, pText, wSize);
    sWord.bWordSrc = bWordSrc;

    if (nProcessType == SPACE_PROCESSING) {
        sWord.Base.bLangIndex = (ET9AWSys_GetBilingualSupported(pLingInfo) && pLingInfo->pLingCmnInfo->wLdbNum == pLingInfo->pLingCmnInfo->wSecondLdbNum) ? ET9AWSECOND_LANGUAGE : ET9AWFIRST_LANGUAGE;
    }
    else if (bLangIndex == ET9AWUNKNOWN_LANGUAGE) {
        sWord.Base.bLangIndex = ET9AWBOTH_LANGUAGES;
    }
    else {
        sWord.Base.bLangIndex = bLangIndex;
    }

    wStatus = _ET9AWSuppDBAddSelection(pLingInfo, &sWord, nProcessType);

    return wStatus;
}

/*---------------------------------------------------------------------------*/
/**
 * Select word.
 * This function allows the host to specify which entry has been selected.
 *
 * @param pLingInfo                 Pointer to alphabetic information structure.
 * @param byWordIndex               Word index of base 0.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL ET9AWSelLstSelWord(ET9AWLingInfo * const       pLingInfo,
                                        const ET9U8                 byWordIndex)

{
    ET9STATUS           wStatus;
    ET9AWPrivWordInfo   *pAmbigWord;
    ET9AWLingCmnInfo    *pLingCmnInfo;

    if ((wStatus = _ET9AWSys_BasicValidityCheck(pLingInfo)) != ET9STATUS_NONE) {
        return wStatus;
    }

    pLingCmnInfo = pLingInfo->pLingCmnInfo;

    if (pLingCmnInfo->Base.bSymbsInfoInvalidated) {
        return ET9STATUS_NEED_SELLIST_BUILD;
    }
    if (byWordIndex >= pLingCmnInfo->Private.bTotalWords) {
        return ET9STATUS_OUT_OF_RANGE;
    }
    if (pLingCmnInfo->Base.pWordSymbInfo == NULL || pLingCmnInfo->Base.pWordSymbInfo->wInitOK != ET9GOODSETUP) {
        return ET9STATUS_INVALID_MEMORY;
    }

    pAmbigWord = &pLingCmnInfo->Private.pWordList[pLingCmnInfo->Private.bWordList[byWordIndex]];

    WLOG2Word(pLogFile2, "ET9AWSelLstSelWord, word", pAmbigWord);

    _ET9SaveWord(pLingCmnInfo->Base.pWordSymbInfo, pAmbigWord->Base.sWord, pAmbigWord->Base.wWordLen);

    _ET9ProcessSelListQUDBEntries(pLingInfo, byWordIndex);

    /* NWP and non buildable exact gets use processing on entire word */

    if (!pLingCmnInfo->Base.pWordSymbInfo->bNumSymbs ||
        (byWordIndex == pLingCmnInfo->Private.bExactIndex && !BUILDABLEEXACT(pLingCmnInfo->Private.dwStateBits))) {

        WLOG2String(pLogFile2, "ET9AWSelLstSelWord, process all", pAmbigWord->Base.sWord, pAmbigWord->Base.wWordLen, 0, 0);

        wStatus = __ET9AWDoWordProcessing(pLingInfo,
                                          pAmbigWord->Base.sWord,
                                          pAmbigWord->Base.wWordLen,
                                          pAmbigWord->Base.bLangIndex,
                                          USE_PROCESSING,
                                          pAmbigWord->bWordSrc);

        if (ET9AWSys_GetBilingualSupported(pLingInfo) && ET9ACTIVELANGSWITCHENABLED(pLingCmnInfo)) {
            if (pAmbigWord->Base.bLangIndex == ET9AWSECOND_LANGUAGE) {
                pLingCmnInfo->Base.pWordSymbInfo->Private.bSwitchLanguage = 1;
            }
            else if (pAmbigWord->Base.bLangIndex == ET9AWBOTH_LANGUAGES && pLingCmnInfo->wLdbNum == pLingCmnInfo->wSecondLdbNum) {
                pLingCmnInfo->Base.pWordSymbInfo->Private.bSwitchLanguage = 1;
            }
            else {
                pLingCmnInfo->Base.pWordSymbInfo->Private.bSwitchLanguage = 0;
            }
        }

    }
    else {

        /* do use processing on pieces */

        /* this code needs to be redesigned, flush points are not a very good instrument to split words when using spc */
        /* locks also makes things more complex and "unexpected" in combination with spc and resulting length diffs */

        ET9INT          snLenUsed;
        const ET9INT    snWordLen = pAmbigWord->Base.wWordLen;
        ET9INT          snFlushLen;
        ET9U16          wSymbolIndex;
        ET9STATUS       wTmpStatus;

        /* loop over all possible flush points */

        snLenUsed = 0;

        for (wSymbolIndex = 1; wSymbolIndex <= pLingCmnInfo->Base.pWordSymbInfo->bNumSymbs; ++wSymbolIndex) {

            if (wSymbolIndex == pLingCmnInfo->Base.pWordSymbInfo->bNumSymbs) {
                snFlushLen = snWordLen;
            }
            else {
                snFlushLen = __CaptureGetFlushStringLengthAtPoint(pLingInfo, wSymbolIndex);
            }

            if (snFlushLen && snFlushLen < snLenUsed) {
                WLOG2(fprintf(pLogFile2, "ET9AWSelLstSelWord, snFlushLen = %d, snLenUsed = %d\n", snFlushLen, snLenUsed);)
            }

            ET9AssertLog(!snFlushLen || snFlushLen >= snLenUsed);

            if (snFlushLen > snLenUsed) {

                ET9INT          snOffset;
                const ET9INT    snPartLen = __ET9Min((snFlushLen - snLenUsed), (snWordLen - snLenUsed));

                WLOG2(fprintf(pLogFile2, "ET9AWSelLstSelWord, snPartLen = %d, snFlushLen = %d, snWordLen = %d, snLenUsed = %d\n", snPartLen, snFlushLen, snWordLen, snLenUsed);)

                ET9AssertLog(snPartLen >= 0);

                for (snOffset = 0; snOffset + 1 < snPartLen; ++snOffset) {

                    WLOG2String(pLogFile2, "ET9AWSelLstSelWord, process part", &pAmbigWord->Base.sWord[snLenUsed + snOffset], (ET9U16)(snPartLen - snOffset), 0, 0);

                    wTmpStatus = __ET9AWDoWordProcessing(pLingInfo,
                                                         &pAmbigWord->Base.sWord[snLenUsed + snOffset],
                                                         (ET9U16)(snPartLen - snOffset),
                                                         pAmbigWord->Base.bLangIndex,
                                                         USE_PROCESSING,
                                                         pAmbigWord->bWordSrc);

                    if (wTmpStatus && wTmpStatus != ET9STATUS_INVALID_TEXT) {
                        wStatus = wTmpStatus;
                    }

                    if (ET9AWSys_GetBilingualSupported(pLingInfo) && ET9ACTIVELANGSWITCHENABLED(pLingCmnInfo)) {
                        if (pAmbigWord->Base.bLangIndex == ET9AWSECOND_LANGUAGE) {
                            pLingCmnInfo->Base.pWordSymbInfo->Private.bSwitchLanguage = 1;
                        }
                        else if (pAmbigWord->Base.bLangIndex == ET9AWBOTH_LANGUAGES && pLingCmnInfo->wLdbNum == pLingCmnInfo->wSecondLdbNum) {
                            pLingCmnInfo->Base.pWordSymbInfo->Private.bSwitchLanguage = 1;
                        }
                        else {
                            pLingCmnInfo->Base.pWordSymbInfo->Private.bSwitchLanguage = 0;
                        }
                    }

                    if (!_ET9_IsPunctChar(pAmbigWord->Base.sWord[snLenUsed + snOffset])) {
                        break;
                    }
                }

                snLenUsed += snPartLen;
            }
        }
    }

    /* don't propagate ET9STATUS_INVALID_TEXT error, thats our problem, if
       ET9STATUS_INVALID_TEXT occurs, the function is essentially a no-op.
    */

    if (wStatus == ET9STATUS_INVALID_TEXT) {
        wStatus = ET9STATUS_NONE;
    }
    if (pAmbigWord->Base.bLangIndex == ET9AWBOTH_LANGUAGES) {
        if (!pLingCmnInfo->Private.wPreviousWordLanguage) {
            pLingCmnInfo->Private.wPreviousWordLanguage = pLingCmnInfo->wFirstLdbNum;
        }
    }
    else if (pAmbigWord->Base.bLangIndex == ET9AWSECOND_LANGUAGE) {
        pLingCmnInfo->Private.wPreviousWordLanguage = pLingCmnInfo->wSecondLdbNum;
    }
    else {
        pLingCmnInfo->Private.wPreviousWordLanguage = pLingCmnInfo->wFirstLdbNum;
    }
    if (ET9AWSys_GetBilingualSupported(pLingInfo) && ET9ACTIVELANGSWITCHENABLED(pLingCmnInfo)) {
        if (pLingCmnInfo->Private.wPreviousWordLanguage != pLingCmnInfo->wLdbNum) {
            _ET9AWLdbSetActiveLanguage(pLingInfo, pLingCmnInfo->Private.wPreviousWordLanguage);
        }
    }
    return wStatus;
}


/*---------------------------------------------------------------------------*/
/** \internal
 * This function checks to see if passed word is a shortcut for CDB entry.
 *
 * @param pLingInfo                 Pointer to alphabetic information structure.
 * @param pText                     Pointer to word.
 * @param aSize                     Length of text word.
 *
 * @return None
 */

static void ET9LOCALCALL __ET9AWCheckForShortcutAssociation(ET9AWLingInfo * const    pLingInfo,
                                                            ET9SYMB * const          pText,
                                                            const ET9U16             aSize)
{

    ET9AWLingCmnInfo * const pLingCmnInfo = pLingInfo->pLingCmnInfo;

    ET9AWPrivWordInfo *pEntry;
    ET9SYMB *psSub;
    ET9SYMB *psTarget;
    ET9UINT  i;
    ET9UINT  j;

    ET9AssertLog(pLingInfo != NULL);
    ET9AssertLog(pText != NULL);
    ET9AssertLog(aSize > 0);

    /* only handle if selection list still viable */

    if (pLingCmnInfo->Private.bTotalWords && !pLingCmnInfo->Base.bSelListInvalidated) {

        for (i = 0; i < pLingCmnInfo->Private.bTotalWords; ++i) {

            pEntry = &pLingCmnInfo->Private.pWordList[pLingCmnInfo->Private.bWordList[i]];

            /* if this matches the size of this entry's substitution length */

            if (pEntry->Base.wSubstitutionLen == aSize) {

                ET9U16 wLdbNum = (pEntry->Base.bLangIndex == ET9AWSECOND_LANGUAGE) ? pLingCmnInfo->wSecondLdbNum : pLingCmnInfo->wFirstLdbNum;

                psSub = pEntry->Base.sSubstitution;
                psTarget = pText;
                for (j = 0; j < pEntry->Base.wSubstitutionLen; ++j) {
                    if (_ET9SymToLower(*psSub++, wLdbNum) != _ET9SymToLower(*psTarget++, wLdbNum)) {
                        break;
                    }
                }

                /* if there's a match, add the shortcut to the CDB */

                if (j == pEntry->Base.wSubstitutionLen) {
                    _ET9AWCDBAddShortcut(pLingInfo, pEntry);
                    break;
                }
            }
        }
    }
}

/*---------------------------------------------------------------------------*/
/**
 * Note word done.
 * This function allows the host to specify which entry has been selected.
 * Similar to ET9AWSelLstSelWord, except it passes the actual selection.
 *
 * @param pLingInfo                 Pointer to alphabetic information structure.
 * @param pText                     Pointer to word.
 * @param aSize                     Length of text word.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL ET9AWNoteWordDone(ET9AWLingInfo * const    pLingInfo,
                                       ET9SYMB * const          pText,
                                       const ET9U16             aSize)
{
    ET9STATUS wStatus;

    if ((wStatus = _ET9AWSys_BasicValidityCheck(pLingInfo)) != ET9STATUS_NONE) {
        return wStatus;
    }

#ifdef EVAL_BUILD
         _ET9Eval_UpdateUsage(&pLingInfo->pLingCmnInfo->Base);
#endif

    if (!pText) {
        return ET9STATUS_INVALID_MEMORY;
    }
    if (!aSize || aSize > ET9MAXWORDSIZE) {
        return ET9STATUS_OUT_OF_RANGE;
    }

    __ET9AWCheckForShortcutAssociation(pLingInfo, pText, aSize);

    wStatus = __ET9AWDoWordProcessing(pLingInfo, pText, aSize, 0, SPACE_PROCESSING, ET9WORDSRC_NONE);

    return wStatus;
}

/*---------------------------------------------------------------------------*/
/**
 * Note phrase done.
 * This function allows the host to specify which entry has been selected when it
 * involves more than a single word (such as a multi-word substitution for a shortcut).<br>
 * Similar to ET9AWSelLstSelWord, except it passes the actual selection.
 *
 * @param pLingInfo                 Pointer to alphabetic information structure.
 * @param pText                     Pointer to phrase.
 * @param aSize                     Length of phrase.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL ET9AWNotePhraseDone(ET9AWLingInfo    * const pLingInfo,
                                         ET9SYMB          * const pText,
                                         const ET9U16             aSize)
{
    ET9STATUS  wStatus;
    ET9SYMB   *psBufCur = pText;
    ET9SYMB   *psBufEnd;
    ET9SYMB   *psCurWord;
    ET9U16     wWordSize = 0;
    ET9U16     wSpaceCount = 0;
    ET9U8      bClass;

    if ((wStatus = _ET9AWSys_BasicValidityCheck(pLingInfo)) != ET9STATUS_NONE) {
        return wStatus;
    }
    if (!pText) {
        return ET9STATUS_INVALID_MEMORY;
    }
    if (!aSize) {
        return ET9STATUS_OUT_OF_RANGE;
    }

    __ET9AWCheckForShortcutAssociation(pLingInfo, pText, aSize);

    psCurWord = psBufCur;
    psBufEnd = &psBufCur[aSize - 1];

    while (psBufCur <= psBufEnd) {

        bClass = ET9GetSymbolClass(*psBufCur);

        if (psBufCur == psBufEnd && !(bClass == ET9SYMWHITE || bClass == ET9SYMUNKN)) {

            ++wWordSize;

            /* fake delimiter */

            bClass = ET9SYMWHITE;
        }

        /* if sym is end of useful word info */

        if (bClass == ET9SYMWHITE || bClass == ET9SYMUNKN) {

            ++wSpaceCount;

            if (wWordSize) {

                if (wWordSize > ET9MAXWORDSIZE) {
                    wStatus = ET9STATUS_OUT_OF_RANGE;
                }
                else {
                    wStatus = __ET9AWDoWordProcessing(pLingInfo, psCurWord, wWordSize, 0, SPACE_PROCESSING, ET9WORDSRC_NONE);
                }
                if (wStatus) {
                    break;
                }
            }
            else if (wSpaceCount == 2) {
                ET9AWFillContextBuffer(pLingInfo, NULL, 0);
            }

            wWordSize = 0;
            ++psBufCur;
            psCurWord = psBufCur;
        }
        else {
            wSpaceCount = 0;
            ++wWordSize;
            ++psBufCur;
        }
    }
    return wStatus;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Add magic string to selection list.
 * This function inserts a magic string (one of the "words") into the selection list.
 *
 * @param pLingInfo                 Pointer to alphabetic information structure.
 * @param pSrcString                String to be inserted.
 * @param wStrSize                  Length of the string.
 * @param pwFreqIndex               Frequency index, to order the strings in the list.
 *
 * @return None
 */

static void ET9LOCALCALL __ET9AWAddMagicStrToSel(ET9AWLingInfo * const pLingInfo,
                                                 ET9SYMB       * const pSrcString,
                                                 const ET9U16          wStrSize,
                                                 ET9U16        * const pwFreqIndex)
{
    ET9AWLingCmnInfo * const pLingCmnInfo = pLingInfo->pLingCmnInfo;

    ET9AWPrivWordInfo   sLocalWord;
    ET9U16              wTmpLen;
    ET9U16              wSrcLen;
    ET9SYMB             *pSrc;
    ET9SYMB             *pDest;

    ET9AssertLog(pLingInfo != NULL);
    ET9AssertLog(pSrcString != NULL);

    if (!wStrSize) {
        return;
    }

    wTmpLen = pLingCmnInfo->Private.sLeftHandWord.Base.wWordLen;
    pLingCmnInfo->Private.sLeftHandWord.Base.wWordLen = 0;

    _InitPrivWordInfo(&sLocalWord);

    sLocalWord.bWordSrc  = ET9WORDSRC_MAGICSTRING;
    sLocalWord.Base.bLangIndex = ET9AWSys_GetBilingualSupported(pLingInfo) ? ET9AWBOTH_LANGUAGES : ET9AWFIRST_LANGUAGE;

    wSrcLen = wStrSize;
    pSrc = pSrcString;
    pDest = sLocalWord.Base.sWord;
    sLocalWord.Base.wWordLen = 0;

    for (wSrcLen = wStrSize; wSrcLen; --wSrcLen) {

        *pDest++ = *pSrc++;
        ++sLocalWord.Base.wWordLen;

        if (sLocalWord.Base.wWordLen == ET9MAXWORDSIZE) {

            sLocalWord.xWordFreq = (ET9FREQPART)(1000 - (*pwFreqIndex)++);
            _ET9AWSelLstAdd(pLingInfo, &sLocalWord, sLocalWord.Base.wWordLen, FREQ_NORMAL);
            sLocalWord.Base.wWordLen = 0;
            pDest = sLocalWord.Base.sWord;
        }
    }

    if (sLocalWord.Base.wWordLen) {

        /* make sure the word isn't shorter than the input */

        while (sLocalWord.Base.wWordLen < ET9MAXLDBWORDSIZE) {
            *pDest++ = ' ';
            ++sLocalWord.Base.wWordLen;
        }

        sLocalWord.xWordFreq = (ET9FREQPART)(1000 - (*pwFreqIndex)++);
        _ET9AWSelLstAdd(pLingInfo, &sLocalWord, sLocalWord.Base.wWordLen, FREQ_NORMAL);
    }

    ET9AssertLog(!sLocalWord.wEWordFreq && !sLocalWord.wTWordFreq && !sLocalWord.dwWordIndex);

    pLingCmnInfo->Private.sLeftHandWord.Base.wWordLen = wTmpLen;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Flush a magic string.
 *
 * @param wLength       Input length needed for the next entry.
 *
 * @return None
 */

#define __FlushMagicString(wLength)                                                                     \
    if (psC - psStr + wLength > ET9MAXWORDSIZE) {                                                       \
        __ET9AWAddMagicStrToSel(pLingInfo, psStr, (ET9U16)(psC - psStr), &wFreqIndex);                  \
        psC = psStr;                                                                                    \
    }                                                                                                   \

/*---------------------------------------------------------------------------*/
/** \internal
 * Add magic strings.
 * This function adds a set of magic strings to the selection list.
 *
 * @param pLingInfo                 Pointer to alphabetic information structure.
 *
 * @return None
 */

static void ET9LOCALCALL __ET9AWAddMagicStr(ET9AWLingInfo * const pLingInfo)
{
    ET9AWLingCmnInfo * const pLingCmnInfo = pLingInfo->pLingCmnInfo;

    ET9U16    wFreqIndex = 0;

    ET9AssertLog(pLingInfo != NULL);

    /* disabling magic string in initial JXT9 release */
    if ((pLingCmnInfo->wLdbNum & ET9PLIDMASK) == ET9PLIDJapanese) {
        return;
    }

    /* add version info */

    {
        ET9U16    wStrLen;
        ET9SYMB   psStr[ET9MAXVERSIONSTR];

        if (!ET9GetCodeVersion(psStr, ET9MAXVERSIONSTR, &wStrLen)) {
            __ET9AWAddMagicStrToSel(pLingInfo, psStr, wStrLen, &wFreqIndex);
        }

        if (!ET9AWLdbGetVersion(pLingInfo, psStr, ET9MAXVERSIONSTR, &wStrLen)) {
            __ET9AWAddMagicStrToSel(pLingInfo, psStr, wStrLen, &wFreqIndex);
        }

        __ET9AWAddMagicStrToSel(pLingInfo,
                                pLingInfo->pLingCmnInfo->Base.pWordSymbInfo->Private.szIDBVersion,
                                pLingInfo->pLingCmnInfo->Base.pWordSymbInfo->Private.wIDBVersionStrSize,
                                &wFreqIndex);
    }

    /* add settings, debug etc info */

    {
        ET9SYMB   *psC;
        ET9SYMB   psStr[ET9MAXWORDSIZE];

        psC = psStr;

        *psC++ = 'L';
        *psC++ = 'i';
        *psC++ = 's';
        *psC++ = 't';
        *psC++ = ':';
        *psC++ = (pLingCmnInfo->Private.eSelectionListMode == ET9ASLMODE_AUTO) ? 'A' : (pLingCmnInfo->Private.eSelectionListMode == ET9ASLMODE_CLASSIC) ? 'C' : (pLingCmnInfo->Private.eSelectionListMode == ET9ASLMODE_COMPLETIONSPROMOTED) ? 'P' : (pLingCmnInfo->Private.eSelectionListMode == ET9ASLMODE_MIXED) ? 'M' : '?';
        *psC++ = (pLingCmnInfo->Private.eSelectionListCorrectionMode == ET9ASLCORRECTIONMODE_LOW) ? 'L' : (pLingCmnInfo->Private.eSelectionListCorrectionMode == ET9ASLCORRECTIONMODE_MEDIUM) ? 'M' : (pLingCmnInfo->Private.eSelectionListCorrectionMode == ET9ASLCORRECTIONMODE_HIGH) ? 'H' : '?';
        *psC++ = '.';
        *psC++ = (ET9SYMB)(pLingCmnInfo->Private.bListSize / 10 + '0');
        *psC++ = (ET9SYMB)(pLingCmnInfo->Private.bListSize % 10 + '0');
        *psC++ = ' ';

        __FlushMagicString(12);

        *psC++ = 'B';
        *psC++ = 'i';
        *psC++ = 'l';
        *psC++ = 'i';
        *psC++ = 'n';
        *psC++ = 'g';
        *psC++ = 'u';
        *psC++ = 'a';
        *psC++ = 'l';
        *psC++ = ':';
        *psC++ = ET9AWSys_GetBilingualSupported(pLingInfo) ? 'Y' : 'N';
        *psC++ = ' ';

        __FlushMagicString(6);

        *psC++ = 'N';
        *psC++ = 'W';
        *psC++ = 'P';
        *psC++ = ':';
        *psC++ = ET9NEXTWORDPREDICTION_MODE(pLingCmnInfo) ? 'Y' : 'N';
        *psC++ = ' ';

        __FlushMagicString(9);

        *psC++ = 'E';
        *psC++ = 'x';
        *psC++ = 'a';
        *psC++ = 'c';
        *psC++ = 't';
        *psC++ = ':';
        *psC++ = ET9EXACTINLIST(pLingCmnInfo) ? 'Y' : 'N';
        *psC++ = ET9EXACTLAST(pLingCmnInfo) ? 'L' : 'F';
        *psC++ = ' ';

        __FlushMagicString(10);

        *psC++ = 'A';
        *psC++ = 'A';
        *psC++ = 'p';
        *psC++ = 'p';
        *psC++ = 'e';
        *psC++ = 'n';
        *psC++ = 'd';
        *psC++ = ':';
        *psC++ = ET9AUTOAPPENDINLIST(pLingCmnInfo) ? 'Y' : 'N';
        *psC++ = ' ';

        __FlushMagicString(10);

        *psC++ = 'S';
        *psC++ = 't';
        *psC++ = 'e';
        *psC++ = 'm';
        *psC++ = 's';
        *psC++ = ':';
        *psC++ = ET9WORDSTEMS_MODE(pLingCmnInfo) ? 'Y' : 'N';
        *psC++ = (ET9SYMB)(pLingCmnInfo->Private.wWordStemsPoint / 10 + '0');
        *psC++ = (ET9SYMB)(pLingCmnInfo->Private.wWordStemsPoint % 10 + '0');
        *psC++ = ' ';

        __FlushMagicString(13);

        *psC++ = 'C';
        *psC++ = 'o';
        *psC++ = 'm';
        *psC++ = 'p';
        *psC++ = 's';
        *psC++ = ':';
        *psC++ = ET9WORDCOMPLETION_MODE(pLingCmnInfo) ? 'Y' : 'N';
        *psC++ = (ET9SYMB)(pLingCmnInfo->Private.wWordCompletionPoint / 10 + '0');
        *psC++ = (ET9SYMB)(pLingCmnInfo->Private.wWordCompletionPoint % 10 + '0');
        *psC++ = '.';
        *psC++ = (ET9SYMB)(pLingCmnInfo->Private.wMaxCompletionCount / 10 + '0');
        *psC++ = (ET9SYMB)(pLingCmnInfo->Private.wMaxCompletionCount % 10 + '0');
        *psC++ = ' ';

        __FlushMagicString(8);

        *psC++ = 'N';
        *psC++ = 'L';
        *psC++ = 'o';
        *psC++ = 'c';
        *psC++ = 'k';
        *psC++ = ':';
        *psC++ = ET9NEXTLOCKING_MODE(pLingCmnInfo->Base.pWordSymbInfo->dwStateBits) ? 'Y' : 'N';
        *psC++ = ' ';

        __FlushMagicString(8);

        *psC++ = 'P';
        *psC++ = 'S';
        *psC++ = 'o';
        *psC++ = 'r';
        *psC++ = 't';
        *psC++ = ':';
        *psC++ = ET9POSTSORTENABLED(pLingCmnInfo) ? 'Y' : 'N';
        *psC++ = ' ';

        __FlushMagicString(7);

        *psC++ = 'Q';
        *psC++ = 'U';
        *psC++ = 'D';
        *psC++ = 'B';
        *psC++ = ':';
        *psC++ = ET9QUDBSUPPORTENABLED(pLingCmnInfo) ? 'Y' : 'N';
        *psC++ = ' ';

        __FlushMagicString(11);

        *psC++ = 'S';
        *psC++ = 'P';
        *psC++ = 'C';
        *psC++ = ':';
        *psC++ = pLingCmnInfo->Private.ASpc.eMode == ET9ASPCMODE_OFF ? 'N' : pLingCmnInfo->Private.ASpc.eMode == ET9ASPCMODE_EXACT ? 'E' : 'R';
        *psC++ = ET9_SPC_SEARCH_FILTER_IS_UNFILTERED(pLingCmnInfo->Private.ASpc.eSearchFilter) ? '-' : ET9_SPC_SEARCH_FILTER_IS_ONE(pLingCmnInfo->Private.ASpc.eSearchFilter) ? '1' : '2';
        *psC++ = ET9_SPC_SEARCH_FILTER_IS_UNFILTERED(pLingCmnInfo->Private.ASpc.eSearchFilter) ? 'U' : ET9_SPC_SEARCH_FILTER_IS_EXACT(pLingCmnInfo->Private.ASpc.eSearchFilter) ? 'X' : 'R';
        *psC++ = '.';
        *psC++ = (ET9SYMB)(pLingCmnInfo->Private.ASpc.wMaxSpcTermCount / 10 + '0');
        *psC++ = (ET9SYMB)(pLingCmnInfo->Private.ASpc.wMaxSpcTermCount % 10 + '0');
        *psC++ = ' ';

        __FlushMagicString(6);

        *psC++ = 'U';
        *psC++ = 'A';
        *psC++ = 'S';
        *psC++ = ':';
        *psC++ = ET9USERDEFINEDAUTOSUBENABLED(pLingCmnInfo) ? 'Y' : 'N';
        *psC++ = ' ';

        __FlushMagicString(6);

        *psC++ = 'L';
        *psC++ = 'A';
        *psC++ = 'S';
        *psC++ = ':';
        *psC++ = ET9LDBSUPPORTEDAUTOSUBENABLED(pLingCmnInfo) ? 'Y' : 'N';
        *psC++ = ' ';

        __FlushMagicString(7);

        *psC++ = 'O';
        *psC++ = 'T';
        *psC++ = 'F';
        *psC++ = 'M';
        *psC++ = ':';
        *psC++ = pLingInfo->Private.pConvertSymb != NULL ? 'Y' : 'N';
        *psC++ = ' ';

        __FlushMagicString(6);

        *psC++ = 'A';
        *psC++ = 'L';
        *psC++ = 'S';
        *psC++ = ':';
        *psC++ = ET9ACTIVELANGSWITCHENABLED(pLingCmnInfo) ? 'Y' : 'N';
        *psC++ = ' ';

        __FlushMagicString(9);

        *psC++ = 'D';
        *psC++ = 'B';
        *psC++ = ':';
        *psC++ = ET9LDBENABLED(pLingCmnInfo)  ? 'L' : 'l';
        *psC++ = ET9CDBENABLED(pLingCmnInfo)  ? 'C' : 'c';
        *psC++ = ET9RUDBENABLED(pLingCmnInfo) ? 'R' : 'r';
        *psC++ = ET9ASDBENABLED(pLingCmnInfo) ? 'A' : 'a';
        *psC++ = ET9MDBENABLED(pLingCmnInfo)  ? 'M' : 'm';
        *psC++ = ' ';

        __FlushMagicString(5);

        *psC++ = 'L';
        *psC++ = 'M';
        *psC++ = ':';
        *psC++ = ET9LMENABLED(pLingCmnInfo) ? 'Y' : 'N';
        *psC++ = ' ';

        __FlushMagicString(6);

        /* debug info */

        *psC++ = 'D';
        *psC++ = 'B';
        *psC++ = 'G';
        *psC++ = ':';
#if defined(_DEBUG)
        *psC++ = 'Y';
#elif defined(NDEBUG)
        *psC++ = 'N';
#else
        *psC++ = '?';
#endif
        *psC++ = ' ';

        __FlushMagicString(6);

        *psC++ = 'X';
        *psC++ = 'D';
        *psC++ = 'B';
        *psC++ = 'G';
        *psC++ = ':';
#ifdef ET9_DEBUG
        *psC++ = 'Y';
#else
        *psC++ = 'N';
#endif
        *psC++ = ' ';

        __FlushMagicString(7);

        /* std lib for memory operations etc info */

        *psC++ = 'S';
        *psC++ = 'T';
        *psC++ = 'D';
        *psC++ = 'L';
        *psC++ = 'I';
        *psC++ = 'B';
        *psC++ = ':';
#ifdef ET9ACTIVATEMISCSTDCLIBUSE
        *psC++ = 'Y';
#else
        *psC++ = 'N';
#endif
        *psC++ = ' ';

        __FlushMagicString(9);

        /* direct KDB access info */

        *psC++ = 'D';
        *psC++ = 'K';
        *psC++ = 'D';
        *psC++ = 'B';
        *psC++ = ':';
#ifdef ET9_DIRECT_KDB_ACCESS
        *psC++ = 'Y';
#else
        *psC++ = 'N';
#endif
        *psC++ = ' ';

        __FlushMagicString(7);

        /* direct LDB access info */

        *psC++ = 'D';
        *psC++ = 'L';
        *psC++ = 'D';
        *psC++ = 'B';
        *psC++ = ':';
#ifdef ET9_DIRECT_LDB_ACCESS
        *psC++ = 'Y';
#else
        *psC++ = 'N';
#endif
        *psC++ = ' ';

        __FlushMagicString(7);

        /* Trace feature */

        *psC++ = 'T';
        *psC++ = 'R';
        *psC++ = 'A';
        *psC++ = 'C';
        *psC++ = 'E';
        *psC++ = ':';
#ifdef ET9_KDB_TRACE_MODULE
        {
            char const *pcChar;

            for (pcChar = ET9_TRACE_VER; *pcChar; ++pcChar) {
                *psC++ = (ET9SYMB)*pcChar;
            }
        }
#else
        *psC++ = 'N';
#endif
        *psC++ = ' ';

        __FlushMagicString(ET9MAXWORDSIZE);
    }
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Init capture info, longer length.
 * Initialize capture info for a new symbol (range).
 * This function will NOT init the word capture storage, rather all additional info needed.
 * It will also remove outdated flush points related to this range.
 *
 * @param pLingInfo     Pointer to alphabetic information structure.
 * @param wFromIndex    Start init from this index.
 * @param wToIndex      End init at this index.
 *
 * @return None
 */

static void ET9LOCALCALL __InitCaptureInfoLonger(ET9AWLingInfo    * const pLingInfo,
                                                 const ET9U16             wFromIndex,
                                                 const ET9U16             wToIndex)
{
    ET9U16                  wIndex;
    ET9U8                   *pbFlushPos;
    ET9U8                   *pbFlushLen;
    ET9U16                  *pwDefaultPos;
    ET9U8                   *pbDefaultLen;
    ET9U8                   *pbDefaultCompLen;
    ET9AWCaptureAction      *pCaptureAction;
    ET9AWBuildInfo * const  pBuildInfo = &pLingInfo->pLingCmnInfo->Private.sBuildInfo;

    ET9AssertLog(pLingInfo != NULL);
    ET9AssertLog(wFromIndex <= wToIndex);
    ET9AssertLog(wToIndex < ET9MAXWORDSIZE);

    pbFlushPos = &pBuildInfo->pbFlushPos[wFromIndex];
    pbFlushLen = &pBuildInfo->pbFlushLen[wFromIndex];
    pwDefaultPos = &pBuildInfo->pwDefaultPos[wFromIndex];
    pbDefaultLen = &pBuildInfo->pbDefaultLen[wFromIndex];
    pbDefaultCompLen = &pBuildInfo->pbDefaultCompLen[wFromIndex];
    pCaptureAction = &pBuildInfo->pCaptureActions[wFromIndex];

    for (wIndex = wFromIndex;
         wIndex <= wToIndex;
         ++wIndex, ++pCaptureAction, ++pbFlushPos, ++pbFlushLen, ++pwDefaultPos, ++pbDefaultLen, ++pbDefaultCompLen) {

        pCaptureAction->bPop = 0;
        pCaptureAction->sbAddWordLen = 0;
        pCaptureAction->sbAddWordCompLen = 0;
        pCaptureAction->bAddSymbolLen = 0;

        *pbFlushPos = 0;
        *pbFlushLen = 0;
        *pwDefaultPos = UNDEFINED_STRING_INDEX;
        *pbDefaultLen = 0;
        *pbDefaultCompLen = 0;
    }

    /* remove old flush positions (from longer builds) */

    pbFlushPos = &pBuildInfo->pbFlushPos[0];

    for (wIndex = 0; wIndex <= wToIndex; ++wIndex, ++pbFlushPos) {

        if (*pbFlushPos > wFromIndex) {
            *pbFlushPos = 0;
        }
    }
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Init capture info, shorter build.
 *
 * @param pLingInfo     Pointer to alphabetic information structure.
 * @param wFromIndex    Start init from this index.
 * @param wToIndex      End init at this index.
 *
 * @return None
 */

static void ET9LOCALCALL __InitCaptureInfoShorter(ET9AWLingInfo    * const pLingInfo,
                                                  const ET9U16             wFromIndex,
                                                  const ET9U16             wToIndex)
{
    ET9U16                  wIndex;
    ET9U8                   *pbFlushPos;
    ET9AWBuildInfo * const  pBuildInfo = &pLingInfo->pLingCmnInfo->Private.sBuildInfo;

    ET9AssertLog(pLingInfo != NULL);
    ET9AssertLog(wFromIndex <= wToIndex);
    ET9AssertLog(wToIndex < ET9MAXWORDSIZE);

    /* remove old flush positions (from longer or equal builds) */

    pbFlushPos = &pBuildInfo->pbFlushPos[0];

    for (wIndex = 0; wIndex <= wToIndex; ++wIndex, ++pbFlushPos) {

        if (*pbFlushPos > wFromIndex) {
            *pbFlushPos = 0;
        }
    }
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Reset all capture info.
 * This function will completely reset all word capture related information stored about builds.
 *
 * @param pLingInfo     Pointer to alphabetic information structure.
 *
 * @return None
 */

static void ET9LOCALCALL __ResetAllCaptureInfo(ET9AWLingInfo * const pLingInfo)
{
    ET9WordSymbInfo     * const pWordSymbInfo = pLingInfo->pLingCmnInfo->Base.pWordSymbInfo;
    ET9AWBuildInfo      * const pBuildInfo = &pLingInfo->pLingCmnInfo->Private.sBuildInfo;

    ET9AssertLog(pLingInfo != NULL);

    WLOG2(fprintf(pLogFile2, "__ResetAllCaptureInfo, bCaptureInvalidated = %d\n", pBuildInfo->bCaptureInvalidated);)

    pBuildInfo->bCurrCapture = 0;

    _ET9ClearMem((ET9U8*)pBuildInfo->pCaptures, (ET9UINT)(ET9MAXBUILDCAPTURES * sizeof(ET9AWCaptureBuild)));

    if (pWordSymbInfo->bNumSymbs) {

        __InitCaptureInfoLonger(pLingInfo, 0, (ET9U16)(pWordSymbInfo->bNumSymbs - 1));
    }

    pBuildInfo->bCaptureInvalidated = 0;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Pre capture word.
 * Assure that capture info is valid (reset before doing/storing anything about a new symbol).
 * Especially on skip forward builds (locks).
 *
 * @param pLingInfo     Pointer to alphabetic information structure.
 *
 * @return None
 */

static void ET9LOCALCALL __PreCaptureWord(ET9AWLingInfo * const pLingInfo)
{
    ET9AWLingCmnInfo * const pLingCmnInfo = pLingInfo->pLingCmnInfo;
    ET9WordSymbInfo * const pWordSymbInfo = pLingCmnInfo->Base.pWordSymbInfo;

    ET9AssertLog(pLingInfo != NULL);

    if (pWordSymbInfo->bNumSymbs > pLingCmnInfo->Private.bLastBuildLen) {
        __InitCaptureInfoLonger(pLingInfo, pLingCmnInfo->Private.bLastBuildLen, (ET9U16)(pWordSymbInfo->bNumSymbs - 1));
    }
    else if (pWordSymbInfo->bNumSymbs) {
        __InitCaptureInfoShorter(pLingInfo, (ET9U16)(pWordSymbInfo->bNumSymbs - 1), (ET9U16)(pLingCmnInfo->Private.bLastBuildLen - 1));
    }
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Assure locked string.
 *
 * @param pLingInfo     Pointer to alphabetic information structure.
 * @param pWord         Pointer to word to assure.
 * @param wIndex        Input index/position (non zero if partial).
 *
 * @return None
 */

static void ET9LOCALCALL __AssureLockedString(ET9AWLingInfo        * const pLingInfo,
                                              ET9AWPrivWordInfo    * const pWord,
                                              const ET9U16                 wIndex)
{
    const ET9U16            wLockPoint = pLingInfo->pLingCmnInfo->Private.wCurrLockPoint;

    ET9AssertLog(pLingInfo != NULL);
    ET9AssertLog(pWord != NULL);

    if (wLockPoint) {

        ET9U16      wCount = __ET9Min(pWord->Base.wWordLen, wLockPoint);
        ET9SYMB     *pSymb = pWord->Base.sWord;
        ET9SymbInfo *pSymbInfo = &pLingInfo->pLingCmnInfo->Base.pWordSymbInfo->SymbsInfo[wIndex];

        for (; wCount; --wCount, ++pSymb, ++pSymbInfo) {
            *pSymb = pSymbInfo->sLockedSymb;
        }

        pWord->Base.bLangIndex = pLingInfo->pLingCmnInfo->Base.pWordSymbInfo->SymbsInfo[wLockPoint - 1].bLockLanguageSource;
    }
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Check if flush point.
 * This function checks if a symbol pos is a flush point.
 *
 * @param pLingInfo     Pointer to alphabetic information structure.
 * @param wSymbolPoint  Point to investigate.
 *
 * @return Non zero if it is, otherwise zero.
 */

static ET9BOOL ET9LOCALCALL __CaptureIsFlushPoint(ET9AWLingInfo * const pLingInfo,
                                                  const ET9U16          wSymbolPoint)
{
    ET9U8                   *pbFlushPos;
    ET9WordSymbInfo * const pWordSymbInfo = pLingInfo->pLingCmnInfo->Base.pWordSymbInfo;
    ET9AWBuildInfo  * const pBuildInfo = &pLingInfo->pLingCmnInfo->Private.sBuildInfo;

    ET9AssertLog(pLingInfo != NULL);
    ET9AssertLog(pWordSymbInfo != NULL);

    /* validate */

    if (!pWordSymbInfo->bNumSymbs || !wSymbolPoint) {
        return 0;
    }

    /* check point */

    pbFlushPos = &pBuildInfo->pbFlushPos[wSymbolPoint - 1];

    if (*pbFlushPos && *pbFlushPos < pWordSymbInfo->bNumSymbs) {
        return 1;
    }

    return 0;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Get flush string length at point.
 * This function gets the length of the current flush in string characters.
 *
 * @param pLingInfo     Pointer to alphabetic information structure.
 * @param wSymbolPoint  Point to use.
 *
 * @return Length in characters.
 */

static ET9U16 ET9LOCALCALL __CaptureGetFlushStringLengthAtPoint(ET9AWLingInfo   * const pLingInfo,
                                                                const ET9U16            wSymbolPoint)
{
    ET9AWBuildInfo  * const pBuildInfo = &pLingInfo->pLingCmnInfo->Private.sBuildInfo;
    const ET9U16            wLockPoint = pLingInfo->pLingCmnInfo->Private.wCurrLockPoint;

    ET9U16  wStringLen;
    ET9U16  wSymbolLen;

    ET9AssertLog(pLingInfo != NULL);

    if (!__CaptureIsFlushPoint(pLingInfo, wSymbolPoint)) {
        return 0;
    }

    wSymbolLen = wSymbolPoint;

    wStringLen = pBuildInfo->pbFlushLen[wSymbolLen - 1];

    if (wSymbolLen <= wLockPoint && wStringLen != wSymbolLen) {

        WLOG2(fprintf(pLogFile2, "__CaptureGetFlushStringLengthAtPoint, lock point overrides actual string length (%d <- %d)\n", wStringLen, wSymbolLen);)

        wStringLen = wSymbolLen;
    }

    return wStringLen;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Get flush point.
 * This function gets the flush point for the current build.
 * The peek value can be used to look into future builds (to e.g. verify a newly set point).
 *
 * @param pLingInfo     Pointer to alphabetic information structure.
 * @param bPeekOffset   How far ahead to peek (zero for current).
 *
 * @return Flush point.
 */

static ET9U16 ET9LOCALCALL __CaptureGetFlushPoint(ET9AWLingInfo   * const pLingInfo,
                                                  const ET9U8             bPeekOffset)
{
    ET9U16                  wPoint;
    ET9U8                   *pbFlushPos;
    ET9WordSymbInfo * const pWordSymbInfo = pLingInfo->pLingCmnInfo->Base.pWordSymbInfo;
    ET9AWBuildInfo  * const pBuildInfo = &pLingInfo->pLingCmnInfo->Private.sBuildInfo;

    ET9AssertLog(pLingInfo != NULL);
    ET9AssertLog(pWordSymbInfo != NULL);

    /* validate */

    if (!pWordSymbInfo->bNumSymbs) {
        return 0;
    }

    /* look for a flush point */

    wPoint = pWordSymbInfo->bNumSymbs - 1 + bPeekOffset;

    pbFlushPos = &pBuildInfo->pbFlushPos[wPoint - 1];

    for (; wPoint; --wPoint, --pbFlushPos) {

        if (*pbFlushPos && *pbFlushPos < pWordSymbInfo->bNumSymbs + bPeekOffset) {
            return wPoint;
        }
    }

    return 0;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Get current flush symbol length.
 * This function gets the length of the current flush in symbols (entries).
 *
 * @param pLingInfo     Pointer to alphabetic information structure.
 *
 * @return Length
 */

static ET9U16 ET9LOCALCALL __CaptureGetFlushSymbolLength(ET9AWLingInfo * const pLingInfo)
{
    ET9U16 wPoint;

    ET9AssertLog(pLingInfo != NULL);

    wPoint = __CaptureGetFlushPoint(pLingInfo, 0);

    if (!wPoint) {
        return 0;
    }

    return wPoint;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Get current flush string length.
 * This function gets the length of the current flush string.
 *
 * @param pLingInfo         Pointer to alphabetic information structure.
 * @param bDisregardLock    If lock info should be disregarded, "default" is "no".
 *
 * @return Length
 */

static ET9U16 ET9LOCALCALL __CaptureGetFlushStringLength(ET9AWLingInfo  * const pLingInfo,
                                                         const ET9BOOL          bDisregardLock)
{
    ET9AWBuildInfo  * const pBuildInfo = &pLingInfo->pLingCmnInfo->Private.sBuildInfo;
    const ET9U16            wLockPoint = pLingInfo->pLingCmnInfo->Private.wCurrLockPoint;

    ET9U16  wStringLen;
    ET9U16  wSymbolLen;

    ET9AssertLog(pLingInfo != NULL);

    wSymbolLen = __CaptureGetFlushSymbolLength(pLingInfo);

    if (!wSymbolLen) {
        return 0;
    }

    wStringLen = pBuildInfo->pbFlushLen[wSymbolLen - 1];

    if (wSymbolLen <= wLockPoint && wStringLen != wSymbolLen) {

        if (bDisregardLock) {
            WLOG2(fprintf(pLogFile2, "__CaptureGetFlushStringLength, lock info is applicable, but disregarded\n");)
        }
        else {
            WLOG2(fprintf(pLogFile2, "__CaptureGetFlushStringLength, lock point overrides actual string length (%d <- %d)\n", wStringLen, wSymbolLen);)

            wStringLen = wSymbolLen;
        }
    }

    return wStringLen;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Make sure default/flushed word syms are cased according to symbinfo shift settings
 *
 * @param pLingInfo      Pointer to alphabetic information structure.
 * @param pWord          Pointer to retrieved default or flushed word.
 *
 * @return None
 */

static void ET9LOCALCALL __ApplySymbInfoShiftToWord(ET9AWLingInfo * const  pLingInfo,
                                                    ET9AWPrivWordInfo      *pWord)
{
    ET9U16       i;
    ET9U16       wLockPoint;
    ET9SymbInfo *pSymbInfo;
    ET9SYMB     *pSymb;
    ET9U16       wLdbNum;

    ET9AssertLog(pLingInfo != NULL);
    ET9AssertLog(pWord != NULL);

    i = pWord->Base.wWordLen;

    if (i) {

        wLdbNum  = (pWord->Base.bLangIndex == ET9AWSECOND_LANGUAGE) ? pLingInfo->pLingCmnInfo->wSecondLdbNum : pLingInfo->pLingCmnInfo->wFirstLdbNum;
        pSymbInfo = &pLingInfo->pLingCmnInfo->Base.pWordSymbInfo->SymbsInfo[i-1];
        pSymb = &pWord->Base.sWord[i-1];
        wLockPoint = pLingInfo->pLingCmnInfo->Private.wCurrLockPoint;

        for (; i && i > wLockPoint; --i, --pSymbInfo, --pSymb) {
            if (pSymbInfo->eShiftState) {
                *pSymb = _ET9SymToUpper(*pSymb, wLdbNum);
                ET9AssertLog(pSymbInfo->bForcedLowercase == 0);
            }
            else if (pSymbInfo->bForcedLowercase) {
                *pSymb = _ET9SymToLower(*pSymb, wLdbNum);
            }
        }
    }
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Get current flush.
 * This function gets the flush from the capture.
 *
 * @param pLingInfo     Pointer to alphabetic information structure.
 * @param pFlush        Pointer to store flush in.
 *
 * @return None
 */

static void ET9LOCALCALL __CaptureGetFlush(ET9AWLingInfo        * const pLingInfo,
                                           ET9AWPrivWordInfo    * const pFlush)
{
    ET9AWBuildInfo  * const pBuildInfo = &pLingInfo->pLingCmnInfo->Private.sBuildInfo;

    ET9U16 wFlushLen = __CaptureGetFlushStringLength(pLingInfo, 0);

    ET9AssertLog(pLingInfo != NULL);
    ET9AssertLog(pFlush != NULL);

    WLOG2(fprintf(pLogFile2, "__CaptureGetFlush, wFlushLen = %d (string length)\n", wFlushLen);)

    _InitPrivWordInfo(pFlush);

    if (!wFlushLen) {
        WLOG2(fprintf(pLogFile2, "__CaptureGetFlush, no flush (empty)\n");)
        return;
    }

    /* get the flush info */

    {
        ET9U16  wCount = wFlushLen;
        ET9SYMB *pSymb1 = pBuildInfo->psFlushedSymbs;
        ET9SYMB *pSymb2 = pFlush->Base.sWord;

        for (; wCount; --wCount) {
            *(pSymb2++) = *(pSymb1++);
        }
    }

    /* attributes */

    pFlush->Base.wWordLen = wFlushLen;
    pFlush->bWordSrc = ET9WORDSRC_CAPTURE;

    pFlush->Base.bLangIndex = pBuildInfo->bLanguageSource[wFlushLen - 1];

    /* assure locked chars */

    __AssureLockedString(pLingInfo, pFlush, 0);

    /* update syms based on shift/forced lowercasing info */

    __ApplySymbInfoShiftToWord(pLingInfo, pFlush);

    /* done */

    WLOG2Word(pLogFile2, "__CaptureGetFlush, flush from capture", pFlush);
}

/*---------------------------------------------------------------------------*/
/** \internal
 * New flush point.
 * Update word capture with new flush point.
 *
 * @param pLingInfo     Pointer to alphabetic information structure.
 * @param wSymbolLen    Number of symbols that are flushed.
 * @param pWord         Word that is flushed.
 *
 * @return None
 */

static void ET9LOCALCALL __CaptureFlushPoint(ET9AWLingInfo        * const pLingInfo,
                                             const ET9U16                 wSymbolLen,
                                             ET9AWPrivWordInfo    * const pWord)
{
    ET9WordSymbInfo * const     pWordSymbInfo = pLingInfo->pLingCmnInfo->Base.pWordSymbInfo;
    ET9AWBuildInfo * const      pBuildInfo = &pLingInfo->pLingCmnInfo->Private.sBuildInfo;

    ET9AssertLog(pLingInfo != NULL);
    ET9AssertLog(pWord != NULL);
    ET9AssertLog(wSymbolLen);
    ET9AssertLog(pWordSymbInfo->bNumSymbs >= wSymbolLen);
    ET9AssertLog(pWord->Base.wWordLen <= ET9MAXWORDSIZE);

    WLOG2(fprintf(pLogFile2, "__CaptureFlush, wSymbolLen = %d @ bNumSymbs = %d\n", wSymbolLen, pWordSymbInfo->bNumSymbs);)
    WLOG2Word(pLogFile2, "__CaptureFlush, word", pWord);

    WLOG2(fprintf(pLogFile2, "__CaptureFlush, current flush, point = %d, symbols = %d, length = %d\n",
                              __CaptureGetFlushPoint(pLingInfo, 0),
                              __CaptureGetFlushSymbolLength(pLingInfo),
                              __CaptureGetFlushStringLength(pLingInfo, 0));)

    /* update flush info */

    pBuildInfo->pbFlushPos[wSymbolLen-1] = pWordSymbInfo->bNumSymbs;
    pBuildInfo->pbFlushLen[wSymbolLen-1] = (ET9U8)pWord->Base.wWordLen;

    {
        ET9U16  wCount = pWord->Base.wWordLen;
        ET9SYMB *pSymb1 = pBuildInfo->psFlushedSymbs;
        ET9SYMB *pSymb2 = pWord->Base.sWord;

        for (; wCount; --wCount) {
            *(pSymb1++) = *(pSymb2++);
        }
    }

    pBuildInfo->bLanguageSource[wSymbolLen-1] = pWord->Base.bLangIndex;

    /* log capture info */

    WLOG2(fprintf(pLogFile2, "__CaptureFlush, peek point = %d\n", __CaptureGetFlushPoint(pLingInfo, 1));)

    WLOG2Captures(pLogFile2, "__CaptureFlush, after capture flush", pLingInfo);

    /* verify */

    ET9AssertLog(__CaptureGetFlushPoint(pLingInfo, 1) == wSymbolLen);
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Update flush info.
 * Update the flush info from (default) word info.
 *
 * @param pLingInfo     Pointer to alphabetic information structure.
 * @param pWord         Word to update from.
 *
 * @return None
 */

static void ET9LOCALCALL __CaptureUpdateFlushInfo(ET9AWLingInfo        * const pLingInfo,
                                                  ET9AWPrivWordInfo    * const pWord)
{
    ET9AWLingCmnInfo * const    pLingCmnInfo = pLingInfo->pLingCmnInfo;
    ET9WordSymbInfo * const     pWordSymbInfo = pLingCmnInfo->Base.pWordSymbInfo;
    ET9AWBuildInfo * const      pBuildInfo = &pLingCmnInfo->Private.sBuildInfo;

    ET9AssertLog(pLingInfo != NULL);
    ET9AssertLog(pWordSymbInfo != NULL);
    ET9AssertLog(pWord != NULL);
    ET9AssertLog(pWord->Base.wWordLen <= ET9MAXWORDSIZE);

    WLOG2(fprintf(pLogFile2, "__CaptureUpdateFlushInfo, bNumSymbs = %d\n", pWordSymbInfo->bNumSymbs);)
    WLOG2Word(pLogFile2, "__CaptureUpdateFlushInfo, word", pWord);

    /* update flush diff info */

    if (pWordSymbInfo->bNumSymbs && pWordSymbInfo->bNumSymbs > pLingCmnInfo->Private.bLastBuildLen) {

        ET9U16 wSymbolLen = pWordSymbInfo->bNumSymbs;

        WLOG2(fprintf(pLogFile2, "__CaptureUpdateFlushInfo, pbDefaultLen[%d], %d updated to %d\n", wSymbolLen-1, pBuildInfo->pbDefaultLen[wSymbolLen-1], pWord->Base.wWordLen);)
        WLOG2(fprintf(pLogFile2, "__CaptureUpdateFlushInfo, pbDefaultCompLen[%d], %d updated to %d\n", wSymbolLen-1, pBuildInfo->pbDefaultCompLen[wSymbolLen-1], pWord->Base.wWordCompLen);)

        if (pBuildInfo->pbDefaultLen[wSymbolLen-1] >= pWord->Base.wWordLen) {

            WLOG2(fprintf(pLogFile2, "__CaptureUpdateFlushInfo, updating\n");)

            pBuildInfo->pbFlushLen[wSymbolLen-1] = 0;
            pBuildInfo->pbDefaultLen[wSymbolLen-1] = (ET9U8)pWord->Base.wWordLen;
            pBuildInfo->pbDefaultCompLen[wSymbolLen-1] = (ET9U8)pWord->Base.wWordCompLen;
        }
        else {
            WLOG2(fprintf(pLogFile2, "__CaptureUpdateFlushInfo, supressing update to longer\n");)
        }

        if ((pWord->Base.wWordLen - pWord->Base.wWordCompLen) != wSymbolLen) {
            WLOG2Captures(pLogFile2, "__CaptureUpdateFlushInfo, after flush update", pLingInfo);
        }
    }
    else {
        WLOG2(fprintf(pLogFile2, "__CaptureUpdateFlushInfo, no default update, bNumSymbs = %d, bLastBuildLen = %d\n", pWordSymbInfo->bNumSymbs, pLingCmnInfo->Private.bLastBuildLen);)
    }
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Get default string length.
 * This function gets the length of the default (at symbol len) without any completion part.
 * This function gives a proper length value even if the default string might have been lost.
 *
 * @param pLingInfo     Pointer to alphabetic information structure.
 * @param wSymbolLen    At symbol len.
 *
 * @return Length
 */

static ET9U16 ET9LOCALCALL __CaptureGetDefaultStringLength(ET9AWLingInfo    * const pLingInfo,
                                                           const ET9U16             wSymbolLen)
{
    ET9AWLingCmnInfo * const    pLingCmnInfo = pLingInfo->pLingCmnInfo;
    ET9AWBuildInfo * const      pBuildInfo = &pLingCmnInfo->Private.sBuildInfo;
    const ET9U16                wLockPoint = pLingCmnInfo->Private.wCurrLockPoint;
    const ET9U16                wStoreIndex = (ET9U16)(wSymbolLen - 1);

    ET9AssertLog(pLingInfo != NULL);
    ET9AssertLog(wSymbolLen <= ET9MAXWORDSIZE);

    if (!wSymbolLen || wSymbolLen > ET9MAXWORDSIZE) {

        WLOG2(fprintf(pLogFile2, "__CaptureGetDefaultStringLength, failure, returns zero\n");)

        return 0;
    }

    if (pBuildInfo->pwDefaultPos[wStoreIndex] == UNDEFINED_STRING_INDEX) {

        WLOG2(fprintf(pLogFile2, "__CaptureGetDefaultStringLength, missing, wSymbolLen = %d, length = %d\n", wSymbolLen, (wSymbolLen - 1));)

        return (ET9U16)(wSymbolLen - 1);
    }

    if (wSymbolLen - 1 <= wLockPoint || pBuildInfo->pbDefaultLen[wStoreIndex] <= wLockPoint) {

        WLOG2(fprintf(pLogFile2, "__CaptureGetDefaultStringLength, from lock, wSymbolLen = %d, length = %d\n", wSymbolLen, (wSymbolLen - 1));)

        return (ET9U16)(wSymbolLen - 1);
    }

    WLOG2(fprintf(pLogFile2, "__CaptureGetDefaultStringLength, from store, wSymbolLen = %d, length = %d\n", wSymbolLen, (pBuildInfo->pbDefaultLen[wSymbolLen - 1] - pBuildInfo->pbDefaultCompLen[wSymbolLen - 1]));)

    ET9AssertLog(pBuildInfo->pbDefaultLen[wSymbolLen - 1] >= pBuildInfo->pbDefaultCompLen[wSymbolLen - 1]);

    return (ET9U16)(pBuildInfo->pbDefaultLen[wSymbolLen - 1] - pBuildInfo->pbDefaultCompLen[wSymbolLen - 1]);
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Get default word.
 * This function gets a (previous) default word from the capture.
 * It tries to store as many default words as possible given the available memory.
 *
 * @param pLingInfo     Pointer to alphabetic information structure.
 * @param pWord         Pointer to store word in.
 * @param wSymbolLen    At symbol len.
 *
 * @return Zero on failure, otherwise non zero.
 */

static ET9BOOL ET9LOCALCALL __CaptureGetDefault(ET9AWLingInfo       * const pLingInfo,
                                                ET9AWPrivWordInfo   * const pWord,
                                                const ET9U16                wSymbolLen)
{
    ET9AWLingCmnInfo * const    pLingCmnInfo = pLingInfo->pLingCmnInfo;
    ET9AWBuildInfo * const      pBuildInfo = &pLingCmnInfo->Private.sBuildInfo;
    const ET9U16                wLockPoint = pLingCmnInfo->Private.wCurrLockPoint;
    const ET9U16                wStoreIndex = (ET9U16)(wSymbolLen - 1);

    WLOG2(fprintf(pLogFile2, "__CaptureGetDefault, wSymbolLen = %d\n", wSymbolLen);)

    _InitPrivWordInfo(pWord);

    if (!wSymbolLen || wSymbolLen > ET9MAXWORDSIZE || pBuildInfo->pwDefaultPos[wStoreIndex] == UNDEFINED_STRING_INDEX) {
        return 0;
    }

    if (wSymbolLen - 1 <= wLockPoint || pBuildInfo->pbDefaultLen[wStoreIndex] <= wLockPoint) {

        pWord->Base.wWordLen = (ET9U16)__ET9Min((wSymbolLen - 1), wLockPoint);

        __AssureLockedString(pLingInfo, pWord, 0);

        WLOG2Word(pLogFile2, "__CaptureDefault, word from lock", pWord);
    }
    else {

        pWord->Base.wWordLen = pBuildInfo->pbDefaultLen[wStoreIndex];
        pWord->Base.wWordCompLen = pBuildInfo->pbDefaultCompLen[wStoreIndex];

        _ET9SymCopy(pWord->Base.sWord, &pBuildInfo->psDefaultSymbs[pBuildInfo->pwDefaultPos[wStoreIndex]], pWord->Base.wWordLen);

        if (wLockPoint) {

            if (wSymbolLen - 1 <= wLockPoint && pWord->Base.wWordLen < wSymbolLen - 1) {

                WLOG2(fprintf(pLogFile2, "__CaptureGetDefault, too short for lock, extending\n");)

                pWord->Base.wWordLen = (ET9U16)(wSymbolLen - 1);
                pWord->Base.wWordCompLen = 0;
            }
            else if (wSymbolLen - 1 <= wLockPoint && (pWord->Base.wWordLen - pWord->Base.wWordCompLen) < wSymbolLen - 1) {

                WLOG2(fprintf(pLogFile2, "__CaptureGetDefault, stem too short for lock, extending\n");)

                pWord->Base.wWordCompLen = (ET9U16)(pWord->Base.wWordLen - (wSymbolLen - 1));
            }

            __AssureLockedString(pLingInfo, pWord, 0);
        }

        WLOG2Word(pLogFile2, "__CaptureDefault, word from store", pWord);
    }

    /* update syms based on shift/forced lowercasing info */

    __ApplySymbInfoShiftToWord(pLingInfo, pWord);

    return 1;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Capture default word.
 * This function saves a (previous) default word in the capture.
 * It tries to store as many default words as possible given the available memory.
 *
 * @param pLingInfo         Pointer to alphabetic information structure.
 * @param pWord             Pointer to word to save.
 * @param wSymbolLen        At symbol len.
 * @param bLastBuildLen     Last build length (controls when to save).
 *
 * @return Zero on failure, otherwise non zero.
 */

static void ET9LOCALCALL __CaptureDefault(ET9AWLingInfo       * const pLingInfo,
                                          ET9AWPrivWordInfo   * const pWord,
                                          const ET9U16                wSymbolLen,
                                          const ET9U16                bLastBuildLen)
{
    ET9AWBuildInfo * const      pBuildInfo = &pLingInfo->pLingCmnInfo->Private.sBuildInfo;
    const ET9U16                wStoreIndex = (ET9U16)(wSymbolLen - 1);
    ET9U16                      wWordLen = pWord->Base.wWordLen;
    ET9U16                      wWordCompLen = pWord->Base.wWordCompLen;

    WLOG2Word(pLogFile2, "__CaptureDefault, word", pWord);

    ET9AssertLog(pLingInfo != NULL);
    ET9AssertLog(pWord != NULL);
    ET9AssertLog(pWord->Base.wWordLen <= ET9MAXWORDSIZE);

    if (!wSymbolLen || wSymbolLen > ET9MAXWORDSIZE) {
        return;
    }

    /* only store on increasing length or missing data */

    if (bLastBuildLen >= wSymbolLen && pBuildInfo->pwDefaultPos[wStoreIndex] != UNDEFINED_STRING_INDEX) {

        WLOG2(fprintf(pLogFile2, "__CaptureDefault, shrinking and ok data, skipping\n");)

        return;
    }
    else {

        WLOG2(fprintf(pLogFile2, "__CaptureDefault, increasing or same build or missing data (bLastBuildLen = %d, wSymbolLen = %d, pos[%d] = %d)\n", bLastBuildLen, wSymbolLen, wStoreIndex, pBuildInfo->pwDefaultPos[wStoreIndex]);)

        /* try to maintain default (word) length though */

        if (pBuildInfo->pbDefaultLen[wStoreIndex] &&
            pBuildInfo->pbDefaultLen[wStoreIndex] >= pBuildInfo->pbDefaultCompLen[wStoreIndex]) {

            WLOG2(fprintf(pLogFile2, "  trying to reuse old default len data\n");)

            if (wWordLen >= pBuildInfo->pbDefaultLen[wStoreIndex]) {

                wWordLen = pBuildInfo->pbDefaultLen[wStoreIndex];
                wWordCompLen = pBuildInfo->pbDefaultCompLen[wStoreIndex];
            }
            else {

                ET9U16 wStemLen = (ET9U16)(pBuildInfo->pbDefaultLen[wStoreIndex] - pBuildInfo->pbDefaultCompLen[wStoreIndex]);

                if (wWordLen >= wStemLen) {
                    wWordCompLen = (ET9U16)(wWordLen - wStemLen);
                }
                else {
                    wWordCompLen = 0;
                }
            }
        }
        else if (bLastBuildLen >= wSymbolLen && wWordLen >= wSymbolLen && wSymbolLen) {

            WLOG2(fprintf(pLogFile2, "  no default data, shrinking, truncating to symbol len - 1 (%d)\n", (wSymbolLen - 1));)

            wWordLen = (ET9U16)(wSymbolLen - 1);
            wWordCompLen = 0;
        }
    }

    /* validate word length */

    if (!wWordLen || wWordLen > ET9MAXWORDSIZE) {

        pBuildInfo->pwDefaultPos[wStoreIndex] = UNDEFINED_STRING_INDEX;
        pBuildInfo->pbDefaultLen[wStoreIndex] = 0;
        pBuildInfo->pbDefaultCompLen[wStoreIndex] = 0;

        return;
    }

    /* store */

    {
        ET9U16 wWriteStart = 0;
        ET9U16 wWriteLen = 0;

        if (!wStoreIndex) {

            /* first entry, just start from the beginning */

            pBuildInfo->pwDefaultPos[wStoreIndex] = 0;

            wWriteStart = 0;
            wWriteLen = wWordLen;
        }
        else {

            /* additional entry, overlap previous or after previous */

            ET9U16 wPos;
            ET9U16 wIndex;
            ET9U16 wPrevStoreIndex;

            /* look for previous */

            for (wPrevStoreIndex = (ET9U16)(wStoreIndex - 1); ; --wPrevStoreIndex) {

                if (!wPrevStoreIndex || pBuildInfo->pwDefaultPos[wPrevStoreIndex] != UNDEFINED_STRING_INDEX) {
                    break;
                }
            }

            /* something else with the same start pos and longer */

            for (wIndex = 0; wIndex < wPrevStoreIndex; ++wIndex) {

                if (pBuildInfo->pwDefaultPos[wIndex] == pBuildInfo->pwDefaultPos[wPrevStoreIndex] &&
                    pBuildInfo->pbDefaultLen[wIndex] > pBuildInfo->pbDefaultLen[wPrevStoreIndex]) {

                    wPrevStoreIndex = wIndex;
                }
            }

            /* see if things can be reused */

            if (pBuildInfo->pwDefaultPos[wPrevStoreIndex] == UNDEFINED_STRING_INDEX) {
                wPos = 0;
                wWriteStart = wPos;
                wWriteLen = wWordLen;
            }
            else {

                /* check overlap */

                ET9U16 wCmpLen;
                ET9SYMB *pStr1;
                ET9SYMB *pStr2;

                if (pBuildInfo->pbDefaultLen[wPrevStoreIndex] <= wWordLen) {
                    wCmpLen = pBuildInfo->pbDefaultLen[wPrevStoreIndex];
                }
                else {
                    wCmpLen = wWordLen;
                }

                pStr1 = pWord->Base.sWord;
                pStr2 = &pBuildInfo->psDefaultSymbs[pBuildInfo->pwDefaultPos[wPrevStoreIndex]];
                ++wCmpLen;
                while (--wCmpLen) {
                    if (*pStr1++ != *pStr2++) {
                        break;
                    }
                }

                if (!wCmpLen) {

                    /* overlap, reuse */

                    wPos = pBuildInfo->pwDefaultPos[wPrevStoreIndex];

                    if (wWordLen <= pBuildInfo->pbDefaultLen[wPrevStoreIndex]) {
                        wWriteStart = wPos;
                        wWriteLen = 0;
                    }
                    else {
                        wWriteStart = wPos + pBuildInfo->pbDefaultLen[wPrevStoreIndex];
                        wWriteLen = (ET9U16)(wWordLen - pBuildInfo->pbDefaultLen[wPrevStoreIndex]);
                    }
                }
                else {

                    /* no overlap, move out */

                    wPos = pBuildInfo->pwDefaultPos[wPrevStoreIndex] + pBuildInfo->pbDefaultLen[wPrevStoreIndex];

                    wWriteStart = wPos;
                    wWriteLen = wWordLen;
                }
            }

            /* make sure it fits in the buffer */

            if (wPos + wWordLen > ET9DEFAULTSTORESIZE) {

                /* need to start over */

                wPos = 0;
                wWriteStart = wPos;
                wWriteLen = wWordLen;
            }

            /* wipe out possibly over written entries */

            if (wWriteLen) {

                ET9U16 wIndex;

                for (wIndex = 0; wIndex < wStoreIndex; ++wIndex) {

                    if (pBuildInfo->pwDefaultPos[wIndex] >= wWriteStart && pBuildInfo->pwDefaultPos[wIndex] < wWriteStart + wWriteLen) {
                        pBuildInfo->pwDefaultPos[wIndex] = UNDEFINED_STRING_INDEX;
                    }
                }
            }

            /* found good spot */

            pBuildInfo->pwDefaultPos[wStoreIndex] = wPos;
        }

        /* actually store stuff */

        WLOG2(fprintf(pLogFile2, "  actually storing, pbDefaultLen[%d] = %d, pbDefaultCompLen[%d] = %d\n", wStoreIndex, wWordLen, wStoreIndex, wWordCompLen);)

        pBuildInfo->pbDefaultLen[wStoreIndex] = (ET9U8)wWordLen;
        pBuildInfo->pbDefaultCompLen[wStoreIndex] = (ET9U8)wWordCompLen;

        _ET9SymCopy(&pBuildInfo->psDefaultSymbs[pBuildInfo->pwDefaultPos[wStoreIndex]], pWord->Base.sWord, wWordLen);

        ET9AssertLog(pBuildInfo->pwDefaultPos[wStoreIndex] < ET9DEFAULTSTORESIZE);
        ET9AssertLog(pBuildInfo->pwDefaultPos[wStoreIndex] + pBuildInfo->pbDefaultLen[wStoreIndex] <= ET9DEFAULTSTORESIZE);
    }
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Get word from capture.
 * This function gets a word from the capture
 * Although completions are stored they will never be returned.
 * (Stems allowed doesn't mean completions allowed.)
 *
 * @param pLingInfo         Pointer to alphabetic information structure.
 * @param pWord             Pointer to store word in.
 * @param wSymbolIndex      Symbol index to match from.
 * @param wSymbolLength     Symbol length to match (equal or bigger than).
 * @param bStemsAllowed     If stems allowed.
 *
 * @return Zero on failure, otherwise non zero.
 */

static ET9BOOL ET9LOCALCALL __CaptureGetWord(ET9AWLingInfo        * const pLingInfo,
                                             ET9AWPrivWordInfo    * const pWord,
                                             const ET9U16                 wSymbolIndex,
                                             const ET9U16                 wSymbolLength,
                                             const ET9U8                  bStemsAllowed)
{
    ET9U16                      wCount;
    ET9SYMB                     *pSymb1;
    ET9SYMB                     *pSymb2;
    ET9U16                      wFlushSymbolLen;
    ET9U16                      wFlushStringLen;
    ET9U16                      wOmitSymbolLength;
    ET9U16                      wOmitStringLength;
    ET9AWBuildInfo * const      pBuildInfo = &pLingInfo->pLingCmnInfo->Private.sBuildInfo;
    ET9AWCaptureBuild * const   pCapture = &pBuildInfo->pCaptures[pBuildInfo->bCurrCapture];

    ET9AssertLog(pLingInfo != NULL);
    ET9AssertLog(pWord != NULL);

    WLOG2(fprintf(pLogFile2, "__CaptureGetWord, wSymbolIndex = %d, wSymbolLength = %d, StemsAllowed = %d\n", wSymbolIndex, wSymbolLength, bStemsAllowed);)

    /* init (result) */

    _InitPrivWordInfo(pWord);

    /* something captured that can be used? */

    wFlushSymbolLen = __CaptureGetFlushSymbolLength(pLingInfo);
    wFlushStringLen = __CaptureGetFlushStringLength(pLingInfo, 0);
    wOmitSymbolLength = wSymbolIndex - wFlushSymbolLen;
    wOmitStringLength = wOmitSymbolLength;

    WLOG2(fprintf(pLogFile2, "__CaptureGetWord, wFlushSymbolLen = %d, wFlushStringLen = %d, wOmitSymbolLength = %d, wOmitStringLength = %d\n", wFlushSymbolLen, wFlushStringLen, wOmitSymbolLength, wOmitStringLength);)

    if (!pCapture->bIsValid) {
        WLOG2(fprintf(pLogFile2, "__CaptureGetWord, nothing valid exists (fail)\n");)
        return 0;
    }

    if (!bStemsAllowed && pCapture->wWordCompLen) {
        WLOG2(fprintf(pLogFile2, "__CaptureGetWord, requested no stem but a stem is captured (fail)\n");)
        return 0;
    }

    if (wFlushStringLen != __CaptureGetFlushStringLength(pLingInfo, 1)) {
        WLOG2(fprintf(pLogFile2, "__CaptureGetWord, a lock affects the flush string length (fail)\n");)
        return 0;
    }

    if (wSymbolIndex < wFlushSymbolLen) {
        WLOG2(fprintf(pLogFile2, "__CaptureGetWord, bad symbol index, %d < %d (fail)\n", wSymbolIndex, wFlushSymbolLen);)
        return 0;
    }

    if (wFlushSymbolLen + wOmitSymbolLength + wSymbolLength != pCapture->wSymbolLen) {
        WLOG2(fprintf(pLogFile2, "__CaptureGetWord, bad capture symbol length, %d != %d (fail)\n", wFlushSymbolLen + wOmitSymbolLength + wSymbolLength, pCapture->wSymbolLen);)
        return 0;
    }

    if (pCapture->wWordLen == pCapture->wWordCompLen + wFlushStringLen + wOmitStringLength) {
        WLOG2(fprintf(pLogFile2, "__CaptureGetWord, bad capture string length, %d == %d (fail)\n", pCapture->wWordLen, pCapture->wWordCompLen + wFlushStringLen + wOmitStringLength);)
        return 0;
    }

    if (pCapture->wWordLen < pCapture->wWordCompLen + wFlushStringLen + wOmitStringLength) {
        WLOG2(fprintf(pLogFile2, "__CaptureGetWord, *VERY* bad capture string length, %d < %d (fail)\n", pCapture->wWordLen, pCapture->wWordCompLen + wFlushStringLen + wOmitStringLength);)
        return 0;
    }

    /* get the word from capture */

    pSymb1 = &pCapture->sWord[wFlushStringLen + wOmitStringLength];
    pSymb2 = pWord->Base.sWord;
    wCount = (ET9U16)(pCapture->wWordLen - pCapture->wWordCompLen - wFlushStringLen - wOmitStringLength);

    pWord->Base.wWordLen = wCount;

    for (; wCount; --wCount) {
        *(pSymb2++) = *(pSymb1++);
    }

    /* set attributes */

    pWord->xTapFreq = 1;
    pWord->xWordFreq = 1;
    pWord->bWordSrc = ET9WORDSRC_CAPTURE;

    WLOG2Word(pLogFile2, "__CaptureGetWord, word found", pWord);

    /* verify */

    ET9AssertLog(pWord->Base.wWordLen <= ET9MAXWORDSIZE);

#ifdef ET9_DEBUG
    {
        ET9INT  snCount = pWord->Base.wWordLen;
        ET9SYMB *psSymb = pWord->Base.sWord;

        for (; snCount; --snCount, ++psSymb) {
            ET9AssertLog(*psSymb && *psSymb != 0xcccc);
        }
    }
#endif

    /* success */

    return 1;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Capture word.
 * Update the capture info with a new word.
 * If there is an active flush it's supposed to be a prefix in the incoming word.
 *
 * @param pLingInfo         Pointer to alphabetic information structure.
 * @param pWord             Word to be added.
 * @param wSymbolLen        Number of symbols used to create the word.
 *
 * @return None
 */

static void ET9LOCALCALL __CaptureWord(ET9AWLingInfo        * const pLingInfo,
                                       ET9AWPrivWordInfo    * const pWord,
                                       const ET9U16                 wSymbolLen)
{
    ET9U8                       bStartNew;
    ET9S16                      swAddWordLen;
    ET9S16                      swAddWordCompLen;
    ET9U16                      wAddSymbolLen;
    ET9U16                      wCount;
    ET9SYMB                     *pSymb1;
    ET9SYMB                     *pSymb2;
    ET9AWCaptureBuild           *pCapture;
    ET9AWCaptureAction          *pCaptureAction;
    ET9WordSymbInfo * const     pWordSymbInfo = pLingInfo->pLingCmnInfo->Base.pWordSymbInfo;
    ET9AWBuildInfo  * const     pBuildInfo = &pLingInfo->pLingCmnInfo->Private.sBuildInfo;

    ET9AssertLog(pLingInfo != NULL);
    ET9AssertLog(pWordSymbInfo != NULL);
    ET9AssertLog(pWord != NULL);
    ET9AssertLog(wSymbolLen <= ET9MAXWORDSIZE);
    ET9AssertLog(pWord->Base.wWordLen <= ET9MAXWORDSIZE);

    WLOG2Word(pLogFile2, "__CaptureWord, word to capture", pWord);

    /* don't capture on zero symbols */

    if (!wSymbolLen) {

        WLOG2(fprintf(pLogFile2, "__CaptureWord, discarding capture - zero symbol length\n");)
        return;
    }

    /* find place to capture in */

    pCapture = &pBuildInfo->pCaptures[pBuildInfo->bCurrCapture];

    bStartNew = 0;
    swAddWordLen = 0;
    swAddWordCompLen = 0;
    wAddSymbolLen = 0;

    if (!pCapture->bIsValid || !pCapture->wWordLen) {
        /* this is a valid spot (empty) */
        bStartNew = 2;
    }
    else if (pCapture->wWordLen > pWord->Base.wWordLen) {
        /* need a new spot */
        bStartNew = 1;
    }
    else if (pCapture->wWordLen <= pWord->Base.wWordLen) {
        /* check if it could be an extension*/

        pSymb1 = pCapture->sWord;
        pSymb2 = pWord->Base.sWord;

        for (wCount = pCapture->wWordLen - pCapture->wWordCompLen; wCount; --wCount) {

            if (*(pSymb1++) != *(pSymb2++)) {
                break;
            }
        }

        if (wCount) {
            bStartNew = 1;
        }
        else {
            bStartNew = 0;
            swAddWordLen = (ET9S16)(pWord->Base.wWordLen - pCapture->wWordLen);
            swAddWordCompLen = (ET9S16)(pWord->Base.wWordCompLen - pCapture->wWordCompLen);
            wAddSymbolLen = (ET9U16)(wSymbolLen - pCapture->wSymbolLen);
        }
    }

    /* anthing new? */

    if (!bStartNew && !swAddWordLen && !swAddWordCompLen && !wAddSymbolLen) {
        WLOG2Captures(pLogFile2, "__CaptureWord, nothing captured", pLingInfo);
        return;
    }

    /* move word into capture */

    if (bStartNew) {

        if (bStartNew == 1) {
            pBuildInfo->bCurrCapture = (pBuildInfo->bCurrCapture + 1) % ET9MAXBUILDCAPTURES;

            if (pBuildInfo->pCaptures[pBuildInfo->bCurrCapture].bIsValid) {
                WLOG2(fprintf(pLogFile2, "__CaptureWord, *** OVERWRITING CAPTURE INFO ***\n");)
            }

            _ET9ClearMem((ET9U8*)&pBuildInfo->pCaptures[pBuildInfo->bCurrCapture], sizeof(ET9AWCaptureBuild));
        }

        pCapture = &pBuildInfo->pCaptures[pBuildInfo->bCurrCapture];

        swAddWordLen = pWord->Base.wWordLen;
        swAddWordCompLen = pWord->Base.wWordCompLen;
        wAddSymbolLen = wSymbolLen;
    }

    if (pWord->Base.wWordLen > pCapture->wWordLen - pCapture->wWordCompLen) {

        pSymb1 = &pCapture->sWord[pCapture->wWordLen - pCapture->wWordCompLen];
        pSymb2 = &pWord->Base.sWord[pCapture->wWordLen - pCapture->wWordCompLen];

        for (wCount = (pWord->Base.wWordLen - (pCapture->wWordLen - pCapture->wWordCompLen)); wCount; --wCount) {
            *(pSymb1++) = *(pSymb2++);
        }
    }

    pCapture->bIsValid = 1;
    pCapture->wWordLen = pWord->Base.wWordLen;
    pCapture->wWordCompLen = pWord->Base.wWordCompLen;
    pCapture->wSymbolLen = wSymbolLen;

    /* update action info */

    pCaptureAction = &pBuildInfo->pCaptureActions[pWordSymbInfo->bNumSymbs-1];

    if (bStartNew == 1) {
        pCaptureAction->bPop = 1;
    }
    else {
        pCaptureAction->bPop = 0;
    }

    pCaptureAction->sbAddWordLen = (ET9S8)swAddWordLen;
    pCaptureAction->sbAddWordCompLen = (ET9S8)swAddWordCompLen;
    pCaptureAction->bAddSymbolLen = (ET9U8)wAddSymbolLen;

    ET9AssertLog(pCaptureAction->sbAddWordLen == swAddWordLen);
    ET9AssertLog(pCaptureAction->sbAddWordCompLen == swAddWordCompLen);
    ET9AssertLog(pCaptureAction->bAddSymbolLen == wAddSymbolLen);

    /* log capture info */

    WLOG2Captures(pLogFile2, "__CaptureWord, after capture word", pLingInfo);
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Handle capture action.
 * Handle stored actions based on added or deleted symbols.
 *
 * @param pLingInfo         Pointer to alphabetic information structure.
 *
 * @return None
 */

static void ET9LOCALCALL __CaptureHandleAction(ET9AWLingInfo * const pLingInfo)
{
    ET9AWLingCmnInfo * const    pLingCmnInfo = pLingInfo->pLingCmnInfo;
    ET9WordSymbInfo * const     pWordSymbInfo = pLingCmnInfo->Base.pWordSymbInfo;
    ET9AWBuildInfo * const      pBuildInfo = &pLingCmnInfo->Private.sBuildInfo;

    ET9AssertLog(pLingInfo != NULL);

    /* reset all capture info if this is the beginning of something new (or invalidated) */

    if (!pLingCmnInfo->Private.bLastBuildLen || pBuildInfo->bCaptureInvalidated) {
        __ResetAllCaptureInfo(pLingInfo);
        return;
    }

    /* handle capture action */

    if (pLingCmnInfo->Private.bLastBuildLen < pWordSymbInfo->bNumSymbs) {

        WLOG2(fprintf(pLogFile2, "__CaptureHandleAction, new symb, bNumSymbs = %d (no action)\n", pWordSymbInfo->bNumSymbs);)
    }
    else if (pLingCmnInfo->Private.bLastBuildLen > pWordSymbInfo->bNumSymbs) {

        ET9S16 swCurrSymb;

        for (swCurrSymb = (ET9S16)(pLingCmnInfo->Private.bLastBuildLen - 1); swCurrSymb >= (ET9S16)pWordSymbInfo->bNumSymbs; --swCurrSymb) {

            ET9AWCaptureBuild   *pCapture = &pBuildInfo->pCaptures[pBuildInfo->bCurrCapture];
            ET9AWCaptureAction  *pCaptureAction = &pBuildInfo->pCaptureActions[swCurrSymb];

            WLOG2(fprintf(pLogFile2, "__CaptureHandleAction, one symb less, bNumSymbs = %d, bCaptureAction = %d,%d,%d,%d, bCurrCapture = %d\n",
                swCurrSymb,
                pCaptureAction->bPop,
                pCaptureAction->sbAddWordLen,
                pCaptureAction->sbAddWordCompLen,
                pCaptureAction->bAddSymbolLen,
                pBuildInfo->bCurrCapture);)

            if (pCaptureAction->bPop) {

                ET9U8 *pbCurrCapture = &pBuildInfo->bCurrCapture;

                pCapture->bIsValid = 0;

                if (!*pbCurrCapture) {
                    *pbCurrCapture = ET9MAXBUILDCAPTURES;
                }
                --(*pbCurrCapture);
            }
            else if (pCapture->bIsValid) {

                ET9AssertLog(pCapture->wWordLen >= pCaptureAction->sbAddWordLen);

                pCapture->wWordLen = pCapture->wWordLen - pCaptureAction->sbAddWordLen;
                pCapture->wWordCompLen = pCapture->wWordCompLen - pCaptureAction->sbAddWordCompLen;
                pCapture->wSymbolLen = pCapture->wSymbolLen - pCaptureAction->bAddSymbolLen;

                if (!pCapture->wWordLen) {
                    pCapture->bIsValid = 0;
                }

                ET9AssertLog(pCapture->wWordLen > pCapture->wWordCompLen || !pCapture->wWordLen && !pCapture->wWordCompLen);
            }
        }
    }
    else {
        WLOG2(fprintf(pLogFile2, "__CaptureHandleAction, same length, bNumSymbs = %d (no action)\n", pWordSymbInfo->bNumSymbs);)
    }

    WLOG2Captures(pLogFile2, "__CaptureHandleAction, after capture action", pLingInfo);
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Shift sub list up.
 * This function will shift a part of the selection list up one step.
 *
 * @param pLingCmnInfo      Pointer to alphabetic cmn information structure.
 * @param nFirst            First pos in interval to be shifted.
 * @param nLast             Last pos in interval to be shifted.
 *
 * @return None
 */

static void ET9LOCALCALL __ShiftSubListUp(ET9AWLingCmnInfo  * const pLingCmnInfo,
                                          const ET9UINT             nFirst,
                                          const ET9UINT             nLast)
{
    const ET9U8 bDown = pLingCmnInfo->Private.bWordList[nFirst - 1];

    ET9UINT nIndex;

    ET9AssertLog(pLingCmnInfo);
    ET9AssertLog(nFirst <= nLast);
    ET9AssertLog(nFirst > 0);
    ET9AssertLog(nLast < pLingCmnInfo->Private.bTotalWords);

    WLOG2(fprintf(pLogFile2, "__ShiftSubListUp, nFirst %u, nLast %u\n", nFirst, nLast);)

    for (nIndex = nFirst - 1; nIndex < nLast; ++nIndex) {
        pLingCmnInfo->Private.bWordList[nIndex] = pLingCmnInfo->Private.bWordList[nIndex + 1]; /* keep direct array access here*/
    }

    pLingCmnInfo->Private.bWordList[nLast] = bDown;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Shift sub list down.
 * This function will shift a part of the selection list down one step.
 *
 * @param pLingCmnInfo      Pointer to alphabetic cmn information structure.
 * @param nFirst            First pos in interval to be shifted.
 * @param nLast             Last pos in interval to be shifted.
 *
 * @return None
 */

static void ET9LOCALCALL __ShiftSubListDown(ET9AWLingCmnInfo    * const pLingCmnInfo,
                                            const ET9UINT               nFirst,
                                            const ET9UINT               nLast)
{
    const ET9U8 bUp = pLingCmnInfo->Private.bWordList[nLast + 1];

    ET9UINT nIndex;

    ET9AssertLog(pLingCmnInfo);
    ET9AssertLog(nFirst <= nLast);
    ET9AssertLog(nLast + 1 < pLingCmnInfo->Private.bTotalWords);

    WLOG2(fprintf(pLogFile2, "__ShiftSubListDown, nFirst %u, nLast %u\n", nFirst, nLast);)

    for (nIndex = nLast + 1; nIndex > nFirst; --nIndex) {
        pLingCmnInfo->Private.bWordList[nIndex] = pLingCmnInfo->Private.bWordList[nIndex - 1]; /* keep direct array access here*/
    }

    pLingCmnInfo->Private.bWordList[nFirst] = bUp;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Remove a word from the selection list.
 * This function will shift a part of the selection list up to remove a candidate word.
 *
 * @param pLingCmnInfo      Pointer to alphabetic cmn information structure.
 * @param nWordIndex        First pos in interval to be shifted.
 *
 * @return None
 */

static void ET9LOCALCALL __RemoveWord(ET9AWLingCmnInfo      * const pLingCmnInfo,
                                      const ET9UINT                 nWordIndex)
{
    ET9UINT nIndex;

    ET9AWPrivWordInfo * const pWord = &pLingCmnInfo->Private.pWordList[pLingCmnInfo->Private.bWordList[nWordIndex]];

    ET9AssertLog(pLingCmnInfo);
    ET9AssertLog(pLingCmnInfo->Private.bTotalWords > 0);
    ET9AssertLog(nWordIndex < pLingCmnInfo->Private.bTotalWords);

    pWord->bWordSrc = ET9WORDSRC_NONE;
    pWord->Base.sWord[0] = 0;
    pWord->Base.wWordLen = 0;
    pWord->Base.wWordCompLen = 0;
    pWord->Base.wSubstitutionLen = 0;

    for (nIndex = nWordIndex; nIndex + 1 < pLingCmnInfo->Private.bTotalWords; ++nIndex) {
        pLingCmnInfo->Private.bWordList[nIndex] = pLingCmnInfo->Private.bWordList[nIndex + 1]; /* keep direct array access here*/
    }

    --pLingCmnInfo->Private.bTotalWords;

    /* special case of list length 1 resets the default index */

    if (pLingCmnInfo->Private.bTotalWords == 1) {

        pLingCmnInfo->Private.bDefaultIndex = 0;
    }
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Post sort the selection list
 * This function will sort a selection list, reordering completions into groups.
 *
 * @param pLingInfo        Pointer to field information structure.
 * @param pWordSymbInfo    Pointer to overall word construction struct.
 *
 * @return None
 */

static void ET9LOCALCALL __ET9AWPerformPostSort(ET9AWLingInfo *pLingInfo,
                                                ET9WordSymbInfo  * const pWordSymbInfo)
{
    ET9AWPrivWordInfo  *pWord;
    ET9AWPrivWordInfo  *pWord2;
    ET9AWLingCmnInfo * const pLingCmnInfo = pLingInfo->pLingCmnInfo;
    ET9U8               i, j, k, m, n;
    ET9U8               bCompTrackingStemDist = 0;
    ET9U8               bCompletionGroupCount;
    ET9U8              *pbWordList;
    ET9U8              *pbWordList2;
    ET9U16              wLdbNum;
    ET9U16              wLdbNum2;

    ET9AssertLog(pLingCmnInfo != NULL);
    ET9AssertLog(pWordSymbInfo != NULL);

    /* save the originally ordered selection list */

    i = 0;
    while (i < pLingCmnInfo->Private.bTotalWords) {

        pbWordList = &pLingCmnInfo->Private.bWordList[i++];
        pWord =  &pLingCmnInfo->Private.pWordList[*pbWordList];
        pWord->bIsGroupBase = 1;
        pWord->bGroupCount = 1;
        wLdbNum  = (pWord->Base.bLangIndex == ET9AWSECOND_LANGUAGE) ? pLingInfo->pLingCmnInfo->wSecondLdbNum : pLingInfo->pLingCmnInfo->wFirstLdbNum;

        if (!pWord->Base.bIsTerm &&
           (pWord->bWordSrc != ET9WORDSRC_CDB) &&
           (pWord->bWordSrc != ET9WORDSRC_RUDB)) {

            pbWordList2 = pbWordList;
            pbWordList2++;
            bCompletionGroupCount = 1;
            bCompTrackingStemDist = pWord->bEditDistStem;

            for (j = i; j < pLingCmnInfo->Private.bTotalWords; j++, pbWordList2++) {

                pWord2 =  &pLingCmnInfo->Private.pWordList[*pbWordList2];

                if (pWord2->Base.bIsTerm ||
                   (pWord2->bWordSrc == ET9WORDSRC_CDB) ||
                   (pWord2->bWordSrc == ET9WORDSRC_RUDB) ||
                   (bCompTrackingStemDist != pWord2->bEditDistStem &&
                    pLingCmnInfo->Private.eCurrSelectionListMode != ET9ASLMODE_CLASSIC)) {

                    break;
                }
                else {
                    bCompletionGroupCount++;
                }
            }

            /* if not only completion in this group, sort 'em */
            /* at this point:                                 */
            /*   i = index  pLingCmnInfo->Private.bWordList[i] is beginning of completion group */
            /*   j = index of position AFTER completion group                                   */

            if (bCompletionGroupCount > 1) {

                /* k will be the insertion point/index for the next subgroup member */

                k = i;

                /* want to continue until entire completion group exhausted */

                while (k < j) {

                    pbWordList2 = &pLingCmnInfo->Private.bWordList[k];
                    m = k;

                    /* now look for a completion matching the stem of the base word */

                    while (m < j) {

                        pWord2 = &pLingCmnInfo->Private.pWordList[*pbWordList2];
                        wLdbNum2  = (pWord2->Base.bLangIndex == ET9AWSECOND_LANGUAGE) ? pLingInfo->pLingCmnInfo->wSecondLdbNum : pLingInfo->pLingCmnInfo->wFirstLdbNum;

                        for (n = 0; n < pWordSymbInfo->bNumSymbs; ++n) {
                            if (_ET9SymToLower(pWord->Base.sWord[n], wLdbNum) != _ET9SymToLower(pWord2->Base.sWord[n], wLdbNum2)) {
                                break;
                            }
                        }

                        /* if match found with existing group base word, add this word to that group */

                        if (n == pWordSymbInfo->bNumSymbs) {
                            pWord2->bIsGroupBase = 0;
                            pWord2->bGroupCount = 0;
                            pWord->bGroupCount++;

                            /* roll entry m up to k position if not in current position */

                            if (m != k) {
                                __ShiftSubListDown(pLingCmnInfo, k, (m - 1));
                            }

                            /* move insertion position past previous found subgroup entry */

                            ++k;
                        }

                        /* move on to next entry in completion group */

                        ++m;
                        ++pbWordList2;

                    } /* while (m < j) */

                    /* after list traversed, entry at current insertion point will become new 'base' */

                    if (k < j) {
                        pWord =  &pLingCmnInfo->Private.pWordList[pLingCmnInfo->Private.bWordList[k]];
                        pWord->bIsGroupBase = 1;
                        pWord->bGroupCount = 1;
                        ++k;
                    }
                } /* while (k < j) */
            } /* if (bCompletionGroupCount > 1) */

            /* move master index to position following previous group */

            i = j;

        } /* if (!pWord->Base.bIsTerm */
    } /* while (i < pLingCmnInfo->Private.bTotalWords) */
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Update the edit distance and stem distance for a word.
 * Especially intended for words not otherwise "built" with this info.
 *
 * @param pLingInfo         Pointer to alphabetic information structure.
 * @param pWord             Word to be updated.
 * @param wIndex            Input index/position (non zero if partial).
 * @param wLength           Word length (active input length).
 * @param bSpcMode          Spell correction mode.
 *
 * @return None
 */

static void ET9LOCALCALL __UpdateSpcInfo(ET9AWLingInfo          * const pLingInfo,
                                         ET9AWPrivWordInfo      * const pWord,
                                         const ET9U16                   wIndex,
                                         const ET9U16                   wLength,
                                         const ET9U8                    bSpcMode)
{
    const ET9U8 bWordSrc = pWord->bWordSrc;

    ET9AWLingCmnInfo * const pLingCmnInfo = pLingInfo->pLingCmnInfo;

    ET9AssertLog(pWord->Base.wWordLen == wLength ||
                 pLingCmnInfo->Private.bSpcDuringBuild ||
                 pWord->bEditDistFree ||
                 pLingCmnInfo->Private.bTraceBuild ||
                 pLingCmnInfo->Private.bExpandAsDuringBuild);

    /* valid? */

    if (!wLength) {
        WLOG2(fprintf(pLogFile2, "__UpdateSpcInfo, empty\n");)
    }

    /* make source not be LDB - upsets the edit distance calc in this case (asserts) */

    pWord->bWordSrc = ET9WORDSRC_NONE;

    /* calculate distance, if it fails, assign a big default */

    if (_ET9AWCalcEditDistanceInit(pLingInfo, wIndex, wLength, bSpcMode) ||
        __CalcEditDistance(pLingInfo, pWord, wIndex, wLength)) {

        pWord->xTapFreq = 1;
        pWord->xWordFreq = 1;
        pWord->dwWordIndex = 0;
        pWord->bEditDistSpc = 50;
        pWord->bEditDistStem = 50;
        pWord->bEditDistFree = 50;
    }

    _ET9AWCalcEditDistanceDone(pLingInfo);

    if (pWord->Base.wWordCompLen > pWord->Base.wWordLen) {
        pWord->Base.wWordCompLen = 0;
    }

    /* restore word source */

    pWord->bWordSrc = bWordSrc;

    /* if spc never been on, make sure it doesn't come out this way... (stem dist is ok) */

    if (!pLingCmnInfo->Private.bSpcDuringBuild) {
        pWord->bEditDistSpc = 0;
    }

    /* verify */

    __VerifyLockedSymbs(pLingInfo, pWord, wIndex, wLength);

    /* done */

    WLOG2Word(pLogFile2, "__UpdateSpcInfo", pWord);
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Setup left hand word based on previous default word.
 *
 * @param pLingInfo             Pointer to alphabetic information structure.
 * @param bNumSymbs             Number of build symbs.
 * @param pLeftHandWord         Left hand word.
 * @param pPrevDefaultWord      Prev default word.
 *
 * @return Non zero if ok, otherwise zero.
 */

static ET9BOOL ET9LOCALCALL __LeftHandWordFromPrevDefaultWord(ET9AWLingInfo          * const pLingInfo,
                                                              const ET9U8                    bNumSymbs,
                                                              ET9AWPrivWordInfo      * const pLeftHandWord,
                                                              ET9AWPrivWordInfo      * const pPrevDefaultWord)
{
    const ET9U16 wDefaultLen = __CaptureGetDefaultStringLength(pLingInfo, bNumSymbs);

    ET9AssertLog(pLeftHandWord != NULL);
    ET9AssertLog(pPrevDefaultWord != NULL);

    if (pPrevDefaultWord->Base.wWordLen < wDefaultLen) {
        return 0;
    }

    *pLeftHandWord = *pPrevDefaultWord;

    if (pLeftHandWord->Base.wWordLen > wDefaultLen) {

        const ET9U16 wLenDiff = pLeftHandWord->Base.wWordLen - wDefaultLen;

        if (wLenDiff >= pLeftHandWord->Base.wWordCompLen) {
            pLeftHandWord->Base.wWordCompLen = 0;
        }
        else {
            pLeftHandWord->Base.wWordCompLen = (ET9U16)(pLeftHandWord->Base.wWordCompLen - wLenDiff);
        }

        pLeftHandWord->Base.wWordLen = wDefaultLen;
    }

    pLeftHandWord->bEditDistSpc = 0;
    pLeftHandWord->bEditDistStem = 0;

    return 1;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Do build substitution processing.
 * Handles things like substitution duplicate supression after full length builds (before buildarounds).
 *
 * @param pLingInfo         Pointer to alphabetic information structure.
 *
 * @return None
 */

static void ET9LOCALCALL __DoBuildSubstitutionProcessing(ET9AWLingInfo * const pLingInfo)
{
    ET9U8               bLook;
    ET9U8               bIndex;
    ET9AWPrivWordInfo   *pWord1 = 0;    /* keep compiler happy */
    ET9AWPrivWordInfo   *pWord2 = 0;    /* keep compiler happy */

    ET9AWLingCmnInfo * const    pLingCmnInfo = pLingInfo->pLingCmnInfo;

    ET9AssertLog(pLingInfo != NULL);

    /* keep these as separate issues (when applicable) for clarity (not a speed problem) */

    WLOG2(fprintf(pLogFile2, "SP - start\n");)

    /* tag all shortcuts with substitutions that also are words in the list */

    for (bIndex = 0; bIndex < pLingCmnInfo->Private.bTotalWords; ++bIndex) {

        pWord1 = &pLingCmnInfo->Private.pWordList[pLingCmnInfo->Private.bWordList[bIndex]];

        if (!pWord1->Base.wSubstitutionLen) {
            continue;
        }

        /* look for a candidate that is the substitution */

        for (bLook = 0; bLook < pLingCmnInfo->Private.bTotalWords; ++bLook) {

            if (bLook == bIndex) {
                continue;
            }

            pWord2 = &pLingCmnInfo->Private.pWordList[pLingCmnInfo->Private.bWordList[bLook]];

            if (pWord2->bWordQuality < LIMITED_QUALITY) {
                continue;
            }

            if (pWord1->Base.wSubstitutionLen == pWord2->Base.wWordLen) {

                /* keep declarations here */

                ET9U16  wCmpLen = pWord1->Base.wSubstitutionLen;
                ET9SYMB *pSrc = pWord1->Base.sSubstitution;
                ET9SYMB *pDest = pWord2->Base.sWord;

                ++wCmpLen;
                while (--wCmpLen) {
                    if (*pSrc++ != *pDest++) {
                        break;
                    }
                }

                if (!wCmpLen) {

                    WLOG2Word(pLogFile2, "SP - shortcut", pWord1);
                    WLOG2Word(pLogFile2, "SP - word", pWord2);

                    /* mark as a substitution duplicate depending on quality */

                    if (pWord1->bWordQuality <= LIMITED_QUALITY) {

                        WLOG2(fprintf(pLogFile2, "SP - limited quality shortcut, slating for dupe removal\n");)

                        pWord1->bWordQuality = DUPE_QUALITY;
                    }
                    else {
                        WLOG2(fprintf(pLogFile2, "SP - quality shortcut, will keep its position\n");)
                    }

                    break;
                }
            }
        }
    }

    /* done */

    WLOG2(fprintf(pLogFile2, "SP - done\n");)
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Do default space char processing.
 * If there is a 'default space char' word it will become the default.
 *
 * @param pLingInfo         Pointer to alphabetic information structure.
 *
 * @return None
 */

static void ET9LOCALCALL __DoDefaultSpaceCharProcessing(ET9AWLingInfo * const pLingInfo)
{
    ET9AWLingCmnInfo * const pLingCmnInfo = pLingInfo->pLingCmnInfo;

    const ET9U8 bFirstIndex = pLingCmnInfo->Private.bDefaultIndex;

    ET9U8 bFound = 0;
    ET9U8 bPromoteIndex = bFirstIndex;

    if (pLingCmnInfo->Private.pWordList[pLingCmnInfo->Private.bWordList[bPromoteIndex]].bWordDesignation == FREQ_BUILDSPACE) {
        WLOG2(fprintf(pLogFile2, "PP - defaulting, already in top\n");)
        bFound = 1;
    }

    for (; !bFound && bPromoteIndex < pLingCmnInfo->Private.bTotalWords; ++bPromoteIndex) {

        ET9AWPrivWordInfo * const pThisWord = &pLingCmnInfo->Private.pWordList[pLingCmnInfo->Private.bWordList[bPromoteIndex]];

        if (pThisWord->bWordDesignation == FREQ_BUILDSPACE) {

            WLOG2(fprintf(pLogFile2, "PP - defaulting, %d moves up to %d\n", bPromoteIndex, bFirstIndex);)

            __ShiftSubListDown(pLingCmnInfo, bFirstIndex, (ET9U8)(bPromoteIndex - 1));

            __LogPartialSelList(pLingInfo->pLingCmnInfo, 0, (ET9U8)(pLingCmnInfo->Private.bTotalWords - 1), "PP - after defaulting op", pLogFile2);

            bFound = 1;
            break;
        }
    }

    if (bFound) {

        ET9U8 bIndex;

        for (bIndex = bPromoteIndex + 1; bIndex < pLingCmnInfo->Private.bTotalWords; ++bIndex) {

            ET9AWPrivWordInfo * const pThisWord = &pLingCmnInfo->Private.pWordList[pLingCmnInfo->Private.bWordList[bIndex]];

            if (pThisWord->bWordDesignation == FREQ_BUILDSPACE) {
                WLOG2(fprintf(pLogFile2, "PP - defaulting, removing %d\n", bIndex);)
                __RemoveWord(pLingCmnInfo, bIndex);
                --bIndex;
            }
        }
    }
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Do build post processing.
 * Handles list reordering after "is higher priority" has set up the original order.
 *
 * @param pLingInfo         Pointer to alphabetic information structure.
 *
 * @return None
 */

static void ET9LOCALCALL __DoBuildPostProcessing(ET9AWLingInfo * const pLingInfo)
{
    const ET9BOOL       bExactInList = __ExactInList(pLingInfo);

    ET9U8               bLook;
    ET9U8               bIndex;
    ET9AWPrivWordInfo   *pWord1 = 0;    /* keep compiler happy */
    ET9AWPrivWordInfo   *pWord2 = 0;    /* keep compiler happy */

    ET9AWLingCmnInfo * const    pLingCmnInfo = pLingInfo->pLingCmnInfo;
    ET9WordSymbInfo * const     pWordSymbInfo = pLingCmnInfo->Base.pWordSymbInfo;

    ET9AssertLog(pLingInfo != NULL);

    /* anything to do? */

    if (!pLingCmnInfo->Private.bTotalWords) {
        return;
    }

    /* keep these as separate issues (when applicable) for clarity (not a speed problem) */

    /* currently no post processing on NWP lists */

    if (!pWordSymbInfo->bNumSymbs) {
        if (ET9POSTSORTENABLED(pLingCmnInfo)) {
            WLOG2(fprintf(pLogFile2, "PP - only 'post sort' action (NWP)\n");)
            __ET9AWPerformPostSort(pLingInfo, pWordSymbInfo);
        }
        else {
            WLOG2(fprintf(pLogFile2, "PP - no action (NWP)\n");)
        }
        return;
    }

    /* log unmodified list */

    __LogPartialSelList(pLingInfo->pLingCmnInfo, 0, (ET9U8)(pLingCmnInfo->Private.bTotalWords - 1), "PP - before post processing", pLogFile2);

    /*  promote term over completion

        * if the default word is a completion a term with equal or less correction will be brought up
    */

    if (pLingCmnInfo->Private.eCurrSelectionListMode == ET9ASLMODE_MIXED) {

        const ET9U8 bFirstIndex = pLingCmnInfo->Private.bDefaultIndex;

        ET9AWPrivWordInfo * const pFirstWord = &pLingCmnInfo->Private.pWordList[pLingCmnInfo->Private.bWordList[bFirstIndex]];

        if (pFirstWord->Base.wWordCompLen) {

            const ET9BOOL bTraceBuild = pLingCmnInfo->Private.bTraceBuild;

            ET9U8 bPromoteIndex;

            WLOG2(fprintf(pLogFile2, "PP - mixed with default completion - will try to promote term, bTraceBuild %u\n", bTraceBuild);)

            for (bPromoteIndex = bFirstIndex + 1; bPromoteIndex < pLingCmnInfo->Private.bTotalWords; ++bPromoteIndex) {

                ET9AWPrivWordInfo * const pThisWord = &pLingCmnInfo->Private.pWordList[pLingCmnInfo->Private.bWordList[bPromoteIndex]];

                if (pThisWord->Base.wWordCompLen) {
                    continue;
                }

                if (!ISREALSRC(pThisWord->bWordSrc)) {
                    continue;
                }

                if (bTraceBuild) {
                    break;
                }

                if (pThisWord->bEditDistSpc  >  pFirstWord->bEditDistSpc  ||
                    pThisWord->bEditDistSpc  == pFirstWord->bEditDistSpc  && pThisWord->bEditDistStem >  pFirstWord->bEditDistStem ||
                    pThisWord->bEditDistStem == pFirstWord->bEditDistStem && pThisWord->bEditDistFree >  pFirstWord->bEditDistFree) {
                    continue;
                }

                break;
            }

            if (bPromoteIndex >= pLingCmnInfo->Private.bTotalWords) {
                WLOG2(fprintf(pLogFile2, "PP - no term to promote\n");)
            }
            else {

                WLOG2(fprintf(pLogFile2, "PP - promoting term, %d moves up to %d\n", bPromoteIndex, bFirstIndex);)

                __ShiftSubListDown(pLingCmnInfo, bFirstIndex, (ET9U8)(bPromoteIndex - 1));

                __LogPartialSelList(pLingInfo->pLingCmnInfo, 0, (ET9U8)(pLingCmnInfo->Private.bTotalWords - 1), "PP - after promote term op", pLogFile2);
            }
        }
    }

    /*  demote candidates

        * never demote index zero (really the exact entry as NONE source)
        * don't demote (non "demote src") spc unless what you pull up has less stem distance
        * "demote src" demote against <= stem distance
        * switch term/stem aborts (one chunk only - all list types)
    */

    if (pLingCmnInfo->Private.eCurrSelectionListMode != ET9ASLMODE_MIXED) {

        ET9U8   bFirst;             /* keep here */
        ET9U8   bLast;              /* keep here */
        ET9U8   bDemoteFirst;       /* keep here */
        ET9U8   bNonDemoteFirst;    /* keep here */
        ET9U8   bLengthIndex;       /* keep here */
        ET9U8   bHasFlush;          /* keep here */

        /* first find the start of the chunk */

        for (bFirst = 0; bFirst < pLingCmnInfo->Private.bTotalWords; ++bFirst) {

            pWord1 = &pLingCmnInfo->Private.pWordList[pLingCmnInfo->Private.bWordList[bFirst]];

            if (ISEXACTSRC(pWord1->bWordSrc) || GETBASESRC(pWord1->bWordSrc) == ET9WORDSRC_NONE) {
                continue;
            }

            if (ISREALSRC(pWord1->bWordSrc)) {
                break;
            }
        }

        /* then find the chunk end */

        for (bLast = bFirst, bIndex = bFirst + 1; bIndex < pLingCmnInfo->Private.bTotalWords; ++bIndex) {

            pWord2 = &pLingCmnInfo->Private.pWordList[pLingCmnInfo->Private.bWordList[bIndex]];

            if (pWord1->Base.bIsTerm != pWord2->Base.bIsTerm || !ISREALSRC(pWord2->bWordSrc)) {
                break;
            }

            bLast = bIndex;
        }

        /* chunk must be at least size 2 to do anything */

        if (bFirst < bLast) {

            WLOG2(fprintf(pLogFile2, "PP - found chunk for demote (%d -> %d)\n", bFirst, bLast);)

            __LogPartialSelList(pLingInfo->pLingCmnInfo, bFirst, bLast, "PP - chunk before demote", pLogFile2);

            /* Find if flush happened */

            bHasFlush = 0;

            for (bLengthIndex = 0; bLengthIndex < pLingCmnInfo->Private.bTotalSymbInputs - 1; ++bLengthIndex) {

                if (pLingCmnInfo->Private.sBuildInfo.pbFlushLen[bLengthIndex]) {
                    bHasFlush = 1;
                    break;
                }
            }

            /* first demote the demote sources */

            WLOG2(fprintf(pLogFile2, "PP - demote sources\n");)

            for (bDemoteFirst = bFirst; (bDemoteFirst - bFirst) < WORD_SOURCE_DEMOTE_FENCE && bDemoteFirst <= bLast; ++bDemoteFirst) {

                pWord1 = &pLingCmnInfo->Private.pWordList[pLingCmnInfo->Private.bWordList[bDemoteFirst]];

                /* must be a demotable source */

                if (!ISDEMOTESRC(pWord1->bWordSrc)) {
                    continue;
                }

                /* LAS only when not also a real word */

                if (GETRAWSRC(pWord1->bWordSrc) == ET9WORDSRC_LAS_SHORTCUT && pWord1->bWordQuality >= GENUINE_QUALITY || pWord1->bWordDesignation == FREQ_BUILDSPACE) {
                    continue;
                }

                /* look for something to promote */

                for (bNonDemoteFirst = bDemoteFirst + 1; bNonDemoteFirst <= bLast; ++bNonDemoteFirst) {

                    pWord2 = &pLingCmnInfo->Private.pWordList[pLingCmnInfo->Private.bWordList[bNonDemoteFirst]];

                    if ((ISDEMOTESRC(pWord2->bWordSrc) && pWord2->bWordDesignation != FREQ_BUILDSPACE) ||
                        pWord2->bEditDistStem > pWord1->bEditDistStem) {

                        continue;
                    }

                    break;
                }

                if (bNonDemoteFirst > bLast) {
                    WLOG2(fprintf(pLogFile2, "PP - nothing to promote (a)\n");)
                    continue;
                }

                WLOG2(fprintf(pLogFile2, "PP - demoting (a), %d moves up to %d\n", bNonDemoteFirst, bDemoteFirst);)

                __ShiftSubListDown(pLingCmnInfo, bDemoteFirst, (ET9U8)(bNonDemoteFirst - 1));

                __LogPartialSelList(pLingInfo->pLingCmnInfo, bFirst, bLast, "PP - after demote op", pLogFile2);
            }

            /* then demote spc */

            WLOG2(fprintf(pLogFile2, "PP - demote spc\n");)

            for (bDemoteFirst = bFirst; (bDemoteFirst - bFirst) < WORD_SOURCE_DEMOTE_FENCE && bDemoteFirst <= bLast; ++bDemoteFirst) {

                pWord1 = &pLingCmnInfo->Private.pWordList[pLingCmnInfo->Private.bWordList[bDemoteFirst]];

                if (!pWord1->bHasPrimEditDist) {
                    continue;
                }

                for (bNonDemoteFirst = bDemoteFirst + 1; bNonDemoteFirst <= bLast; ++bNonDemoteFirst) {

                    pWord2 = &pLingCmnInfo->Private.pWordList[pLingCmnInfo->Private.bWordList[bNonDemoteFirst]];

                    WLOG2B(fprintf(pLogFile2, "  [%2u] ed %u, sd %u, src %u, demote %u, buildable %u\n", pLingCmnInfo->Private.bWordList[bNonDemoteFirst], pWord2->bHasPrimEditDist, pWord2->bEditDistStem, pWord2->bWordSrc, ISDEMOTESRC(pWord2->bWordSrc), ISBUILDABLESRC(pWord2->bWordSrc));)

                    if (pWord2->bHasPrimEditDist ||
                        pWord2->bEditDistStem >= pWord1->bEditDistStem ||
                        ISDEMOTESRC(pWord2->bWordSrc) ||
                        (!bHasFlush && !ISGENUINESRC(pWord2->bWordSrc) && ISGENUINESRC(pWord1->bWordSrc))) {

                        continue;
                    }

                    break;
                }

                if (bNonDemoteFirst > bLast) {
                    WLOG2(fprintf(pLogFile2, "PP - nothing to promote (b)\n");)
                    continue;
                }

                WLOG2(fprintf(pLogFile2, "PP - demoting (b), %d moves up to %d\n", bNonDemoteFirst, bDemoteFirst);)

                __ShiftSubListDown(pLingCmnInfo, bDemoteFirst, (ET9U8)(bNonDemoteFirst - 1));

                __LogPartialSelList(pLingInfo->pLingCmnInfo, bFirst, bLast, "PP - after demote op", pLogFile2);
                __LogPartialSelList(pLingCmnInfo, 0, (ET9U8)(pLingCmnInfo->Private.bTotalWords - 1), "PP - after demote op", pLogFile2);
            }
        }
    }

    /* demote substitutions preceeding the substitution as candidate (really - pull up the spc)

        * if a substitution preceeds (any distance) an equal candidate then move index or move the candidate up (depending on default)
        * also remove substitution duplicates (two shortcuts having the same substitution)
    */

    WLOG2(fprintf(pLogFile2, "PP - demote substitutions\n");)

    for (bIndex = 0; bIndex < pLingCmnInfo->Private.bTotalWords; ++bIndex) {

        ET9BOOL bRepeatIndex = 0;

        pWord1 = &pLingCmnInfo->Private.pWordList[pLingCmnInfo->Private.bWordList[bIndex]];

        if (!pWord1->Base.wSubstitutionLen || pWord1->bWordQuality <= DISPOSABLE_QUALITY) {
            continue;
        }

        WLOG2Word(pLogFile2, "PP - investigating (s vs w)", pWord1);

        /* look for a candidate that is the substitution */

        for (bLook = 0; bLook < pLingCmnInfo->Private.bTotalWords; ++bLook) {

            if (bLook == bIndex) {
                continue;
            }

            pWord2 = &pLingCmnInfo->Private.pWordList[pLingCmnInfo->Private.bWordList[bLook]];

            if (pWord2->bWordQuality < LIMITED_QUALITY) {
                continue;
            }

            /* check word1 subst against word2 word*/

            if (pWord1->Base.wSubstitutionLen == pWord2->Base.wWordLen) {

                /* keep declarations here */

                ET9U16  wCmpLen = pWord1->Base.wSubstitutionLen;
                ET9SYMB *pSrc = pWord1->Base.sSubstitution;
                ET9SYMB *pDest = pWord2->Base.sWord;

                ++wCmpLen;
                while (--wCmpLen) {
                    if (*pSrc++ != *pDest++) {
                        break;
                    }
                }

                if (!wCmpLen) {

                    WLOG2Word(pLogFile2, "PP - dupe with", pWord2);

                    if (!bIndex &&
                        !pLingCmnInfo->Private.bDefaultIndex &&
                        (ISEXACTSRC(pWord1->bWordSrc) || GETBASESRC(pWord1->bWordSrc) == ET9WORDSRC_NONE)) {

                        WLOG2(fprintf(pLogFile2, "PP - substitution preceeds candidate - moving default index up\n");)

                        ++pLingCmnInfo->Private.bDefaultIndex;

                        if (bLook > bIndex + 1) {

                            WLOG2(fprintf(pLogFile2, "PP - substitution preceeds candidate - also moving down %d-%d\n", bIndex+1, bLook-1);)

                            __ShiftSubListDown(pLingCmnInfo, (ET9U8)(bIndex + 1), (ET9U8)(bLook - 1));
                        }
                    }
                    else if (bIndex > bLook) {

                        WLOG2(fprintf(pLogFile2, "PP - candidate preceeds substitution - nothing moved\n");)
                    }
                    else {

                        WLOG2(fprintf(pLogFile2, "PP - substitution preceeds candidate - moving down %d-%d\n", bIndex, bLook-1);)

                        if (pWord2->Base.wSubstitutionLen) {
                            bRepeatIndex = 1;
                        }

                        __ShiftSubListDown(pLingCmnInfo, bIndex, (ET9U8)(bLook - 1));

                        if (pLingCmnInfo->Private.bDefaultIndex > bIndex && pLingCmnInfo->Private.bDefaultIndex <= bLook) {

                            WLOG2(fprintf(pLogFile2, "PP - substitution preceeds candidate - also modifying default index %u -> %u\n", pLingCmnInfo->Private.bDefaultIndex, bIndex);)

                            pLingCmnInfo->Private.bDefaultIndex = bIndex;
                        }
                    }

                    if (pWord1->bWordQuality <= LIMITED_QUALITY) {

                        ET9AssertLog(!(ISEXACTSRC(pWord1->bWordSrc) || GETBASESRC(pWord1->bWordSrc) == ET9WORDSRC_NONE));

                        WLOG2(fprintf(pLogFile2, "PP - low quality shortcut - moving to stem pool (for removal)\n");)

                        pWord1->bWordSrc = ET9WORDSRC_STEMPOOL;
                        pWord1->bWordQuality = DISPOSABLE_QUALITY;
                    }

                    pWord1->Base.wSubstitutionLen = 0;

                    __LogPartialSelList(pLingInfo->pLingCmnInfo, bIndex, bLook, "PP - after preecede op", pLogFile2);

                    break;
                }
            }
        } /* for words 2 */

        /* might have to repeat
           after being used here all dupe quality words are history */

        if (bRepeatIndex) {

            WLOG2(fprintf(pLogFile2, "PP - about to repeat index (%d)\n", bRepeatIndex);)

            --bIndex;
        }
        else if (pWord1->bWordQuality == DUPE_QUALITY) {

            ET9AssertLog(!(ISEXACTSRC(pWord1->bWordSrc) || GETBASESRC(pWord1->bWordSrc) == ET9WORDSRC_NONE));

            WLOG2Word(pLogFile2, "PP - dupe quality shortcut - moving to stem pool (for removal)", pWord1);

            pWord1->bWordSrc = ET9WORDSRC_STEMPOOL;
            pWord1->bWordQuality = DISPOSABLE_QUALITY;
        }

    } /* for words 1 */

    WLOG2(fprintf(pLogFile2, "PP - remove substitution dupes\n");)

    for (bIndex = 0; bIndex < pLingCmnInfo->Private.bTotalWords; ++bIndex) {

        pWord1 = &pLingCmnInfo->Private.pWordList[pLingCmnInfo->Private.bWordList[bIndex]];

        if (!pWord1->Base.wSubstitutionLen || pWord1->bWordQuality <= DISPOSABLE_QUALITY || pWord1->bWordSrc == ET9WORDSRC_STEMPOOL) {
            continue;
        }

        WLOG2Word(pLogFile2, "PP - investigating (s vs s)", pWord1);

        /* look for a candidate that is the substitution */

        for (bLook = 0; bLook < pLingCmnInfo->Private.bTotalWords; ++bLook) {

            if (bLook == bIndex) {
                continue;
            }

            pWord2 = &pLingCmnInfo->Private.pWordList[pLingCmnInfo->Private.bWordList[bLook]];

            if (pWord2->bWordQuality < LIMITED_QUALITY) {
                continue;
            }

            /* check word1 subst against word2 subst (will never happen if the above happened) */

            if (pWord1->Base.wSubstitutionLen == pWord2->Base.wSubstitutionLen) {

                /* keep declarations here */

                ET9U16  wCmpLen = pWord1->Base.wSubstitutionLen;
                ET9SYMB *pSrc = pWord1->Base.sSubstitution;
                ET9SYMB *pDest = pWord2->Base.sSubstitution;

                ++wCmpLen;
                while (--wCmpLen) {
                    if (*pSrc++ != *pDest++) {
                        break;
                    }
                }

                if (!wCmpLen) {

                    WLOG2Word(pLogFile2, "PP - dupe with", pWord2);

                    if (pWord2->bWordQuality <= LIMITED_QUALITY) {

                        ET9AssertLog(!(ISEXACTSRC(pWord2->bWordSrc) || GETBASESRC(pWord2->bWordSrc) == ET9WORDSRC_NONE));

                        WLOG2(fprintf(pLogFile2, "PP - low quality shortcut - moving 2nd to stem pool (for removal)\n");)

                        pWord2->bWordSrc = ET9WORDSRC_STEMPOOL;
                        pWord2->bWordQuality = DISPOSABLE_QUALITY;
                    }

                    pWord2->Base.wSubstitutionLen = 0;

                    __LogPartialSelList(pLingInfo->pLingCmnInfo, bIndex, bLook, "PP - after subst dupe", pLogFile2);
                }
            }
        } /* for words 2 */
    } /* for words 1 */

#if 0
    /* avoid having a completion punct as default */

    for (bIndex = pWordSymbInfo->Private.bDefaultIndex; bIndex > 0; --bIndex) {

        pWord1 = &pAlpSelList->pWordList[pAlpSelList->Private.bWordList[bIndex]];

        if (GETBASESRC(pWord1->bWordSrc) == ET9WORDSRC_COMPLETIONPUNCT) {
            --pWordSymbInfo->Private.bDefaultIndex;
            WLOG2(fprintf(pLogFile2, "PP - default is a completion punct, moving back one step (%d)\n", pWordSymbInfo->Private.bDefaultIndex);)
        }
        else {
            break;
        }
    }
#endif

    /*  clean up the stem pool - don't assume that it's consequtive

        this MUST be the last "operation" applied in the word list builder

        if duplicates are found the list will shrink, but since this is the last real operation
        on the list only the "sorting list" will be shrinking and thus possibly pointing to a word
        that is outside the new list size

        also no attempt is made to update any other internal info used when building the list

        stem pool entries that are spc will be removed no matter what (except when propagating distance)

        entries with disposable quality will be removed no matter what (e.g. orphan RDB entries)

        a "tricky" thing here is that when a word is deleted it affects all loops, indexes etc
    */

    WLOG2(fprintf(pLogFile2, "PP - clean up the stem pool\n");)

    ET9AssertLog(pLingCmnInfo->Private.bTotalWords);

    {
        ET9U8   bFirst;     /* keep here */
        ET9U8   bLast;      /* keep here */

        for (bFirst = 0; bFirst < pLingCmnInfo->Private.bTotalWords; ++bFirst) {

            pWord1 = &pLingCmnInfo->Private.pWordList[pLingCmnInfo->Private.bWordList[bFirst]];

            if (pWord1->bWordSrc == ET9WORDSRC_STEMPOOL) {
                break;
            }
        }

        for (bLast = (ET9U8)(pLingCmnInfo->Private.bTotalWords - 1); bLast > bFirst; --bLast) {

            pWord2 = &pLingCmnInfo->Private.pWordList[pLingCmnInfo->Private.bWordList[bLast]];

            if (pWord2->bWordSrc == ET9WORDSRC_STEMPOOL) {
                break;
            }
        }

        if (bFirst <= bLast) {

            ET9U8 bPoolCount;
            ET9BOOL bDisposableOnly;

            WLOG2(fprintf(pLogFile2, "PP - found stem pool (%d-%d)\n", bFirst, bLast);)

            /* remove word stems and substitutions */

            bPoolCount = 0;

            for (bIndex = bFirst; bIndex <= bLast; ++bIndex) {

                pWord1 = &pLingCmnInfo->Private.pWordList[pLingCmnInfo->Private.bWordList[bIndex]];

                if (pWord1->bWordSrc == ET9WORDSRC_STEMPOOL) {

                    if (pWord1->Base.wWordCompLen) {
                        WLOG2Word(pLogFile2, "PP - removing completion", pWord1);
                    }

                    pWord1->Base.wWordLen = (ET9U16)(pWord1->Base.wWordLen - pWord1->Base.wWordCompLen);
                    pWord1->Base.wWordCompLen = 0;
                    pWord1->Base.wSubstitutionLen = 0;

                    ++bPoolCount;
                }
            }

            /* check if disposable only, if so they all stay... (not dups) */

            bDisposableOnly = 1;

            for (bIndex = 0; bIndex < pLingCmnInfo->Private.bTotalWords; ++bIndex) {

                pWord1 = &pLingCmnInfo->Private.pWordList[pLingCmnInfo->Private.bWordList[bIndex]];

                if (pWord1->bWordSrc != ET9WORDSRC_STEMPOOL ||
                    !pWord1->bHasPrimEditDist && pWord1->bWordQuality > DISPOSABLE_QUALITY) {

                    bDisposableOnly = 0;
                    break;
                }
            }

            WLOG2(fprintf(pLogFile2, "PP - disposable only = %d\n", bDisposableOnly);)

            /* remove resulting duplicates */

            for (bIndex = 0; bIndex < pLingCmnInfo->Private.bTotalWords && bPoolCount; ++bIndex) {

                WLOG2(fprintf(pLogFile2, "PP - checking word @ %02d (pool count = %2d)\n", bIndex, bPoolCount);)

                pWord1 = &pLingCmnInfo->Private.pWordList[pLingCmnInfo->Private.bWordList[bIndex]];

                for (bLook = bFirst; bLook <= bLast && bPoolCount; ++bLook) {

                    ET9BOOL bRemove = 0;    /* keep here */

                    pWord2 = &pLingCmnInfo->Private.pWordList[pLingCmnInfo->Private.bWordList[bLook]];

                    if (pWord2->bWordSrc != ET9WORDSRC_STEMPOOL) {
                        continue;
                    }
                    else if (pWord2->bHasPrimEditDist) {

                        bRemove = !bDisposableOnly;
                    }
                    else if (pWord2->bWordQuality <= DISPOSABLE_QUALITY) {

                        bRemove = !bDisposableOnly;
                    }
                    else if (bLook == bIndex) {

                        continue;
                    }

                    if (!bRemove && bLook != bIndex && pWord1->Base.wWordLen == pWord2->Base.wWordLen) {

                        /* keep declarations here */

                        ET9U16  wCmpLen = pWord1->Base.wWordLen;
                        ET9SYMB *pSrc = pWord1->Base.sWord;
                        ET9SYMB *pDest = pWord2->Base.sWord;

                        ++wCmpLen;
                        while (--wCmpLen) {
                            if (*pSrc++ != *pDest++) {
                                break;
                            }
                        }

                        if (!wCmpLen) {
                            bRemove = 1;
                        }
                    }

                    if (bRemove) {

                        WLOG2Word(pLogFile2, "PP - removing", pWord2);

                        __RemoveWord(pLingCmnInfo, bLook);

                        --bPoolCount;

                        if (bIndex > bLook) {
                            --bIndex;   /* the list shrunk above this index, pWord1 is ok */
                        }

                        --bLook;        /* need to revisit this position */
                        --bLast;        /* the stem pool has shrunk */
                    }
                }
            }

            WLOG2(fprintf(pLogFile2, "PP - %d word(s) remain in the stem pool\n", bPoolCount);)
        }
    }

    /* if the build is shrinking all substitutions should be taken out
       - do it last so that the list appears exactly the same, but without substitutions */

    if (pLingCmnInfo->Private.bLastBuildShrinking) {

        for (bIndex = 0; bIndex < pLingCmnInfo->Private.bTotalWords; ++bIndex) {

            pWord1 = &pLingCmnInfo->Private.pWordList[pLingCmnInfo->Private.bWordList[bIndex]];

            if (pWord1->Base.wSubstitutionLen) {

                WLOG2Word(pLogFile2, "PP - shrinking build, removing substitution", pWord1);

                pWord1->Base.wSubstitutionLen = 0;
            }
        }
    }

    /* handle exact last in list (will never happen for multitap - always default) */

    if (ET9EXACTLAST(pLingCmnInfo) &&
        bExactInList &&
        pLingCmnInfo->Private.bDefaultIndex) {

        WLOG2(fprintf(pLogFile2, "PP - exact not default, moving last\n");)

        __ShiftSubListUp(pLingCmnInfo, 1, (ET9U8)(pLingCmnInfo->Private.bTotalWords - 1));

        --pLingCmnInfo->Private.bDefaultIndex;

        pLingCmnInfo->Private.bExactIndex = (ET9U8)(pLingCmnInfo->Private.bTotalWords - 1);
    }
    else if (ET9EXACTISDEFAULT(pLingCmnInfo) &&
             bExactInList &&
             pLingCmnInfo->Private.bDefaultIndex) {

        WLOG2(fprintf(pLogFile2, "PP - exact default\n");)

        pLingCmnInfo->Private.bDefaultIndex = pLingCmnInfo->Private.bExactIndex;
    }

    if (ET9POSTSORTENABLED(pLingCmnInfo)) {
        __ET9AWPerformPostSort(pLingInfo, pWordSymbInfo);
    }

    /* done */

    WLOG2(fprintf(pLogFile2, "PP - done\n\n");)
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Do build final processing.
 * Very last touch up.
 *
 * @param pLingInfo         Pointer to alphabetic information structure.
 *
 * @return None
 */

static void ET9LOCALCALL __DoBuildFinalProcessing(ET9AWLingInfo * const pLingInfo)
{
    ET9U8               bIndex;

    ET9AWLingCmnInfo * const pLingCmnInfo = pLingInfo->pLingCmnInfo;
    ET9WordSymbInfo  * const pWordSymbInfo = pLingCmnInfo->Base.pWordSymbInfo;

    ET9AssertLog(pLingInfo != NULL);

    /* anything to do? */

    if (!pLingCmnInfo->Private.bTotalWords) {
        return;
    }

    /* currently no final processing on NWP lists */

    if (!pWordSymbInfo->bNumSymbs) {
        WLOG2(fprintf(pLogFile2, "FP - no action (NWP)\n");)
        return;
    }

    /* keep these as separate issues (when applicable) for clarity (not a speed problem) */

    /* log unmodified list */

    __LogPartialSelList(pLingInfo->pLingCmnInfo, 0, (ET9U8)(pLingCmnInfo->Private.bTotalWords - 1), "FP - before final processing", pLogFile2);

    /* if the current default word is capitalized (not shifted) and not locked and applicable language, then try to promote a lower case version of the word in fron of it */

    {
        ET9AWPrivWordInfo const * const pDefaultWord = &pLingCmnInfo->Private.pWordList[pLingCmnInfo->Private.bWordList[pLingCmnInfo->Private.bDefaultIndex]];

        if (!pLingCmnInfo->Private.wCurrLockPoint &&
            pDefaultWord->bIsCapitalized &&
            _ET9_LanguageSpecific_ApplyCapsRules(pLingInfo, pDefaultWord)) {

            ET9SYMB sLowerWord[ET9MAXWORDSIZE];

            WLOG2(fprintf(pLogFile2, "FP - found capitalized default\n");)

            {
                const ET9U16 wLdbNum = (pDefaultWord->Base.bLangIndex == ET9AWSECOND_LANGUAGE) ? pLingCmnInfo->wSecondLdbNum : pLingCmnInfo->wFirstLdbNum;

                for (bIndex = 0; bIndex < pDefaultWord->Base.wWordLen; ++bIndex) {
                    sLowerWord[bIndex] = _ET9SymToLower(pDefaultWord->Base.sWord[bIndex], wLdbNum);
                }
            }

            for (bIndex = 0; bIndex < pLingCmnInfo->Private.bTotalWords; ++bIndex) {

                ET9AWPrivWordInfo const * const pIndexWord = &pLingCmnInfo->Private.pWordList[pLingCmnInfo->Private.bWordList[bIndex]];

                if (bIndex == pLingCmnInfo->Private.bDefaultIndex) {
                    continue;
                }

                if (pIndexWord->Base.wWordLen == pDefaultWord->Base.wWordLen) {

                    ET9UINT nCount;
                    ET9SYMB const *pSymb1 = sLowerWord;
                    ET9SYMB const *pSymb2 = pIndexWord->Base.sWord;

                    for (nCount = pDefaultWord->Base.wWordLen; nCount; --nCount) {
                        if (*pSymb1++ != *pSymb2++) {
                            break;
                        }
                    }

                    if (!nCount) {
                        break;
                    }
                }
            }

            if (bIndex < pLingCmnInfo->Private.bTotalWords) {

                ET9AWPrivWordInfo const * const pIndexWord = &pLingCmnInfo->Private.pWordList[pLingCmnInfo->Private.bWordList[bIndex]];

                if (ISGENUINESRC(pIndexWord->bWordSrc) && !ISNONOVERRIDESRC(pIndexWord->bWordSrc) && pIndexWord->bWordQuality == GENUINE_QUALITY) {

                    WLOG2(fprintf(pLogFile2, "FP - found genuine quality lower case version of capitalized word @ %u\n", bIndex);)

                    if (bIndex < pLingCmnInfo->Private.bDefaultIndex) {

                        WLOG2(fprintf(pLogFile2, "FP - lower case version already ahead of the default word, moving the default from %u to %u\n", pLingCmnInfo->Private.bDefaultIndex, bIndex);)

                        pLingCmnInfo->Private.bDefaultIndex = bIndex;
                    }
                    else {

                        WLOG2(fprintf(pLogFile2, "FP - lower case version after default word, moving the lower case word up\n");)

                        __ShiftSubListDown(pLingCmnInfo, pLingCmnInfo->Private.bDefaultIndex, (bIndex - 1));
                    }
                }
            }
        }
    }

    /* if the current default word is acronymish (multi shifted) and not locked and applicable language and corrected, then try to promote a better word in fron of it */

    {
        ET9AWPrivWordInfo const * const pDefaultWord = &pLingCmnInfo->Private.pWordList[pLingCmnInfo->Private.bWordList[pLingCmnInfo->Private.bDefaultIndex]];

        if (!pLingCmnInfo->Private.wCurrLockPoint &&
            pDefaultWord->bIsAcronym &&
            (pDefaultWord->bEditDistSpc || pDefaultWord->bEditDistStem || pDefaultWord->bEditDistFree) &&
            _ET9_LanguageSpecific_ApplyAcronymRules(pLingInfo, pDefaultWord)) {

            WLOG2(fprintf(pLogFile2, "FP - found acronym default\n");)

            for (bIndex = 0; bIndex < pLingCmnInfo->Private.bTotalWords; ++bIndex) {

                ET9AWPrivWordInfo const * const pIndexWord = &pLingCmnInfo->Private.pWordList[pLingCmnInfo->Private.bWordList[bIndex]];

                if (bIndex == pLingCmnInfo->Private.bDefaultIndex) {
                    continue;
                }

                if (!ISGENUINESRC(pIndexWord->bWordSrc) ||
                    ISNONOVERRIDESRC(pIndexWord->bWordSrc) ||
                    pIndexWord->bWordQuality != GENUINE_QUALITY ||
                    pIndexWord->bIsAcronym ||
                    pIndexWord->bIsCapitalized) {

                    continue;
                }

                break;
            }

            if (bIndex < pLingCmnInfo->Private.bTotalWords) {

                WLOG2(fprintf(pLogFile2, "FP - found promotable word @ %u\n", bIndex);)

                if (bIndex < pLingCmnInfo->Private.bDefaultIndex) {

                    WLOG2(fprintf(pLogFile2, "FP - promotable already ahead of the default word, moving the default from %u to %u\n", pLingCmnInfo->Private.bDefaultIndex, bIndex);)

                    pLingCmnInfo->Private.bDefaultIndex = bIndex;
                }
                else {

                    WLOG2(fprintf(pLogFile2, "FP - promotable after default word, moving the promotable word up\n");)

                    __ShiftSubListDown(pLingCmnInfo, pLingCmnInfo->Private.bDefaultIndex, (bIndex - 1));
                }
            }
        }
    }

    /* if the current default word is non override source, then try to promote a better word in fron of it */

    {
        ET9AWPrivWordInfo const * const pDefaultWord = &pLingCmnInfo->Private.pWordList[pLingCmnInfo->Private.bWordList[pLingCmnInfo->Private.bDefaultIndex]];

        if (!pLingCmnInfo->Private.wCurrLockPoint &&
            ISNONOVERRIDESRC(pDefaultWord->bWordSrc)) {

            WLOG2(fprintf(pLogFile2, "FP - found non-override-source default\n");)

            for (bIndex = 0; bIndex < pLingCmnInfo->Private.bTotalWords; ++bIndex) {

                ET9AWPrivWordInfo const * const pIndexWord = &pLingCmnInfo->Private.pWordList[pLingCmnInfo->Private.bWordList[bIndex]];

                if (bIndex == pLingCmnInfo->Private.bDefaultIndex) {
                    continue;
                }

                if (!ISGENUINESRC(pIndexWord->bWordSrc) ||
                    ISNONOVERRIDESRC(pIndexWord->bWordSrc)) {

                    continue;
                }

                break;
            }

            if (bIndex < pLingCmnInfo->Private.bTotalWords) {

                WLOG2(fprintf(pLogFile2, "FP - found promotable word @ %u\n", bIndex);)

                if (bIndex < pLingCmnInfo->Private.bDefaultIndex) {

                    WLOG2(fprintf(pLogFile2, "FP - promotable already ahead of the default word, moving the default from %u to %u\n", pLingCmnInfo->Private.bDefaultIndex, bIndex);)

                    pLingCmnInfo->Private.bDefaultIndex = bIndex;
                }
                else {

                    WLOG2(fprintf(pLogFile2, "FP - promotable after default word, moving the promotable word up\n");)

                    __ShiftSubListDown(pLingCmnInfo, pLingCmnInfo->Private.bDefaultIndex, (bIndex - 1));
                }
            }
        }
    }

    /* if the current default word is uppercase (not shifted) and not locked and default downshifting is enabled */

    if (ET9DOWNSHIFTDEFAULTENABLED(pLingCmnInfo) &&
        !pLingCmnInfo->Private.wCurrLockPoint &&
        !pWordSymbInfo->SymbsInfo[0].eShiftState &&
        _ET9SymIsUpper(pLingCmnInfo->Private.pWordList[pLingCmnInfo->Private.bWordList[pLingCmnInfo->Private.bDefaultIndex]].Base.sWord[0], pWordSymbInfo->Private.wLocale) &&
        GETBASESRC(pLingCmnInfo->Private.pWordList[pLingCmnInfo->Private.bWordList[pLingCmnInfo->Private.bDefaultIndex]].bWordSrc) != ET9WORDSRC_MAGICSTRING) {

        ET9AWPrivWordInfo const * const pWord = &pLingCmnInfo->Private.pWordList[pLingCmnInfo->Private.bWordList[pLingCmnInfo->Private.bDefaultIndex]];
        ET9AWPrivWordInfo *pLowerWord;
        ET9U8 bReadyToDownshift = 0;

        /* if the word is uppercase naturally
           need to check to see if lowercase version already exists in list
           if it does, don't do downshift logic (may want to revist this later to
           reposition the lowercase word to follow the uppercase version */

        ET9SYMB sWord[ET9MAXWORDSIZE];

        {
            const ET9U16 wLdbNum = (pWord->Base.bLangIndex == ET9AWSECOND_LANGUAGE) ? pLingCmnInfo->wSecondLdbNum : pLingCmnInfo->wFirstLdbNum;

            for (bIndex = 0; bIndex < pWord->Base.wWordLen; ++bIndex) {
                sWord[bIndex] = _ET9SymToLower(pWord->Base.sWord[bIndex], wLdbNum);
            }
        }

        for (bIndex = 0; bIndex < pLingCmnInfo->Private.bTotalWords; ++bIndex) {

            pLowerWord = &pLingCmnInfo->Private.pWordList[pLingCmnInfo->Private.bWordList[bIndex]];

            if (pLowerWord->Base.wWordLen == pWord->Base.wWordLen) {

                ET9U8 bIndex2;

                for (bIndex2 = 0; bIndex2 < pWord->Base.wWordLen; ++bIndex2) {

                    if (pLowerWord->Base.sWord[bIndex2] != sWord[bIndex2]) {
                        break;
                    }
                }

                if (bIndex2 == pWord->Base.wWordLen) {
                    break;
                }
            }
        }

        /* if all words processed with no matches to lowercase version of word, keep going */

        if (bIndex == pLingCmnInfo->Private.bTotalWords) {

            WLOG2(fprintf(pLogFile2, "FP - adding lower case version of default word\n");)

            bReadyToDownshift = 1;
        }

        /* if lowercase index further down list than position after default */

        else if (bIndex > (pLingCmnInfo->Private.bDefaultIndex + 1)) {

            WLOG2(fprintf(pLogFile2, "FP - moving up lower case version to after default word\n");)

            /* then roll the lowercase word up to position after default, and skip downshift logic */

            if (pLingCmnInfo->Private.bDefaultIndex + 2 < pLingCmnInfo->Private.bTotalWords) {
                __ShiftSubListDown(pLingCmnInfo, (pLingCmnInfo->Private.bDefaultIndex + 1), (bIndex - 1));
            }
        }

        /* if still set to add lowercase version of default word... */

        if (bReadyToDownshift) {

            /* if list full, remove low priority entry */

            if (pLingCmnInfo->Private.bTotalWords == pLingCmnInfo->Private.bListSize) {

                /* otherwise remove the last entry - skip protected */

                for (bIndex = pLingCmnInfo->Private.bTotalWords - 1; bIndex; --bIndex) {
                    if (!ISPROTECTEDSRC(pLingCmnInfo->Private.pWordList[pLingCmnInfo->Private.bWordList[bIndex]].bWordSrc)) {
                        break;
                    }
                }

                /* if all are protected, try to find a stem (likely full of stems) */

                if (!bIndex) {
                    for (bIndex = pLingCmnInfo->Private.bTotalWords - 1; bIndex; --bIndex) {
                        if (GETBASESRC(pLingCmnInfo->Private.pWordList[pLingCmnInfo->Private.bWordList[bIndex]].bWordSrc) == ET9WORDSRC_STEM) {
                            break;
                        }
                    }
                }

                /* otherwise just remove the last entry (should never happen unless the list is really small) */

                if (!bIndex) {
                    ET9AssertLog(0);
                    bIndex = pLingCmnInfo->Private.bTotalWords - 1;
                }

                /* actual remove */

                __RemoveWord(pLingCmnInfo, bIndex);

                ET9AssertLog(pLingCmnInfo->Private.bTotalWords < pLingCmnInfo->Private.bListSize);
            }

            /* find an available slot */

            {
                ET9U8 bEntries[ET9MAXSELLISTSIZE];
                ET9U8 bIndex2;

                _ET9ClearMem(bEntries, ET9MAXSELLISTSIZE);

                for (bIndex = 0; bIndex < pLingCmnInfo->Private.bTotalWords; ++bIndex) {
                    bEntries[pLingCmnInfo->Private.bWordList[bIndex]] = 1;
                }

                for (bIndex2 = 0; bIndex2 < ET9MAXSELLISTSIZE; ++bIndex2) {
                    if (!bEntries[bIndex2]) {
                        bIndex = bIndex2;
                        break;
                    }
                }

                ET9AssertLog(bIndex2 < ET9MAXSELLISTSIZE);
            }

            /* fake a new entry at the end of the list by loading the free index and bumping the count */

            pLingCmnInfo->Private.bWordList[pLingCmnInfo->Private.bTotalWords] = bIndex;
            pLingCmnInfo->Private.bTotalWords++;

            /* roll it to position following default word if default not the last entry in list*/

            if (pLingCmnInfo->Private.bDefaultIndex + 2 < pLingCmnInfo->Private.bTotalWords) {
                __ShiftSubListDown(pLingCmnInfo, (pLingCmnInfo->Private.bDefaultIndex + 1), pLingCmnInfo->Private.bTotalWords - 2);
            }

            /* now duplicate the default word, and downshift it */

            pLowerWord = &pLingCmnInfo->Private.pWordList[pLingCmnInfo->Private.bWordList[pLingCmnInfo->Private.bDefaultIndex + 1]];
            _ET9ByteCopy((ET9U8*)pLowerWord, (ET9U8*)pWord, sizeof(ET9AWPrivWordInfo));

            if (pLowerWord->bWordQuality == GENUINE_QUALITY) {
                pLowerWord->bWordQuality = SHAPED_QUALITY;
            }

            {
                const ET9U16 wLdbNum  = (pLowerWord->Base.bLangIndex == ET9AWSECOND_LANGUAGE) ? pLingCmnInfo->wSecondLdbNum : pLingCmnInfo->wFirstLdbNum;

                for (bIndex = 0; bIndex < pLowerWord->Base.wWordLen; ++bIndex) {
                    pLowerWord->Base.sWord[bIndex] = _ET9SymToLower(pLowerWord->Base.sWord[bIndex], wLdbNum);
                }

                /* lowercase substitution too (NOT SURE IF THIS IS VALID?) */

                if (pLowerWord->Base.wSubstitutionLen) {
                    for (bIndex = 0; bIndex < pLowerWord->Base.wSubstitutionLen; ++bIndex) {
                        pLowerWord->Base.sSubstitution[bIndex] = _ET9SymToLower(pLowerWord->Base.sSubstitution[bIndex], wLdbNum);
                    }
                }
            }
        }
    }

    /* done */

    WLOG2(fprintf(pLogFile2, "FP - done\n\n");)
}

/*---------------------------------------------------------------------------*/
/** \internal
 * This function sorts the selection list in final priority order.
 *
 * @param pLingCmnInfo              Pointer to alphabetic cmn information structure.
 *
 * @return None
 */

static void ET9LOCALCALL __ET9AWPriorityListFinalOrder(ET9AWLingCmnInfo * const pLingCmnInfo)                                                           \
{
    const ET9UINT nTotalWords = pLingCmnInfo->Private.bTotalWords;

    ET9U8 const * const pbWordList = pLingCmnInfo->Private.bWordList;
    ET9AWPrivWordInfo const * const pWordList = pLingCmnInfo->Private.pWordList;

    ET9U8 pbWordVisited[ET9MAXSELLISTSIZE];

    ET9UINT nSentinelCount = 0;

    ET9UINT nIndex;

    if (nTotalWords < 2) {
        return;
    }

    _ET9ClearMem(pbWordVisited, sizeof(pbWordVisited));

    for (nIndex = nTotalWords - 1; nIndex > 0; --nIndex) {

        const ET9UINT nChkIndex = pbWordList[nIndex];

        ET9AWPrivWordInfo const * const pWordChk = &pWordList[nChkIndex];

        ET9UINT nLook;

        ++nSentinelCount;

        if (pbWordVisited[nChkIndex]) {
            continue;
        }

        pbWordVisited[nChkIndex] = 1;

        for (nLook = 0; nLook < nIndex; ++nLook) {

            ET9AWPrivWordInfo const * const pWordLook = &pWordList[pbWordList[nLook]];

            if (__PriorityCompare(pLingCmnInfo, pWordChk, pWordLook, 0) > 0) {
                break;
            }
        }

        if (nLook >= nIndex) {
            continue;
        }

        WLOG2(fprintf(pLogFile2, "__ET9AWPriorityListFinalOrder, %u moves to %u\n", nIndex - 1, nLook);)

        __ShiftSubListDown(pLingCmnInfo, nLook, nIndex - 1);

        /* revisit */

        ++nIndex;
    }

    ET9AssertLog(nSentinelCount <= 2 * ET9MAXSELLISTSIZE);
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Reorder selection list for bilingual
 *
 * Note: This function assumes selection list is already ordered for either languages (sort etc).
 * Active language is always first language if active language switch is off.
 * If word is present in both LDBs then it can be presented as part of either depending on the fence.
 * Rules for bilingual reordering:
 * (o)   Exact (numbers for 12 key ... )
 * (i)   Upto 'N' terms from active language that exactly match key sequence (distance 0)
 * (ii)  Upto 'M' terms from non-active language that exactly match key sequence (distance 0)
 * (iii) Remaining terms from active language that exactly match key sequence (distance 0)
 * (iv)  Remaining terms from non-active language that exactly match key sequence (distance 0)
 * (v)   Completions from active language that exactly match key sequence (distance 0)
 * (vi)  Completions from non- active language that exactly match key sequence (distance 0)
 * (vii) Terms from active language that exactly match key sequence (distance 1)
 * (viii)Terms from non-active language that exactly match key sequence (distance 1)
 * (ix)  Completions from active language that exactly match key sequence (distance 1)
 * (x)   Completions from non-active language that exactly match key sequence (distance 1)
 * ...
 * ...
 *
 * QUDB entries being language independent are considered as primary language equivalent for the fence etc.
 *
 * @param pLingInfo         Pointer to alphabetic information structure.
 * @param wFlushPoint       Flush point.
 * @param pLeftHandWord     Pointer to left hand word.
 *
 * @return Void
 */

static void ET9LOCALCALL __DoBilingualReorder(ET9AWLingInfo          * const pLingInfo,
                                              const ET9U16                   wFlushPoint,
                                              ET9AWPrivWordInfo      * const pLeftHandWord)
{
    ET9U8                       i,j;
    ET9U8                       bFenceCount;
    ET9U8                       bActiveIndex = 1;
    ET9U8                       bIndex;
    ET9AWLingCmnInfo  * const   pLingCmnInfo = pLingInfo->pLingCmnInfo;
    ET9AWPrivWordInfo * const   pWordList = pLingCmnInfo->Private.pWordList;
    ET9U8             * const   bWordList = pLingCmnInfo->Private.bWordList;
    ET9AWPrivWordInfo *         pCurWordList;

    ET9AssertLog(pLingInfo);
    ET9AssertLog(ET9AWSys_GetBilingualSupported(pLingInfo));

    if (!pLingCmnInfo->Private.bTotalSymbInputs) {
        return;
    }

    /* get the active language */

    if (wFlushPoint && pLeftHandWord->Base.bLangIndex != ET9AWBOTH_LANGUAGES) {
        bActiveIndex = pLeftHandWord->Base.bLangIndex;
    }
    else if (!ET9ACTIVELANGSWITCHENABLED(pLingCmnInfo)) {
        bActiveIndex = 1;
    }
    else if (pLingCmnInfo->Private.wPreviousWordLanguage && (pLingCmnInfo->Private.wPreviousWordLanguage == pLingCmnInfo->wSecondLdbNum)) {
        bActiveIndex = 2;
    }
    else {
        bActiveIndex = 1;
    }

    /* terms with distance 0 from active language (bound by primary fence count) */

    i = pLingCmnInfo->Private.bDefaultIndex;
    j = i;

    while ((i < pLingCmnInfo->Private.bTotalWords - 1) &&
           (j < pLingCmnInfo->Private.bPrimaryFence + pLingCmnInfo->Private.bDefaultIndex)) {

        bIndex = bWordList[i];
        pCurWordList = &pWordList[bIndex];

        if ((pCurWordList->Base.bLangIndex == bActiveIndex || pCurWordList->Base.bLangIndex == ET9AWBOTH_LANGUAGES) &&
            ISREALSRC(pCurWordList->bWordSrc) &&
            !pCurWordList->bEditDistStem &&
            !pCurWordList->Base.wWordCompLen) {
        }
        else {
            break;
        }

        ++i;
        ++j;
    }

    /* terms with distance 0 from non-active language (bound by secondary fence count) */

    bFenceCount = j;

    bActiveIndex = (bActiveIndex % 2 + 1);

    for (i = bFenceCount, j = 0;
        i < pLingCmnInfo->Private.bTotalWords - 1 && j < pLingCmnInfo->Private.bSecondaryFence;
        i++) {

        bIndex = bWordList[i];
        pCurWordList = &pWordList[bIndex];

        if ((pCurWordList->Base.bLangIndex == bActiveIndex || pCurWordList->Base.bLangIndex == ET9AWBOTH_LANGUAGES) &&
            ISREALSRC(pCurWordList->bWordSrc) &&
            !ISPUNCTSRC(pCurWordList->bWordSrc) &&
            !pCurWordList->bEditDistStem &&
            !pCurWordList->Base.wWordCompLen) {

            if (i != bFenceCount + j) {
                __ShiftSubListDown(pLingCmnInfo, bFenceCount + j, i - 1);
            }

            ++j;
        }
    }
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Boost top candidate.
 *
 * @param pLingInfo         Pointer to alphabetic information structure.
 *
 * @return None
 */

static void ET9LOCALCALL __BoostTopCandidate(ET9AWLingInfo * const pLingInfo)
{
    ET9FREQ                     xTargetTotFreq = 0;
    ET9FREQ                     xTestTotFreq = 0;
    ET9U32                      dwTargetIndex = 0;
    ET9U32                      dwTestIndex = 0;
    ET9STATUS                   wStatus = ET9STATUS_NONE;
    ET9FREQPART                 xTargetFreq = 0;
    ET9FREQPART                 xTestFreq = 0;
    ET9U16                      wTargetWFreq = 0;
    ET9U16                      wTestWFreq = 0;
    ET9U16                      wWordLen = 0;
    ET9U16                      wTestEFreq = 0;
    ET9U16                      wTargetEFreq = 0;
    ET9U16                      wTestTFreq = 0;
    ET9U16                      wTargetTFreq = 0;
    ET9U16                      wSavedLDBNum = 0;
    ET9U16                      wPreviousLDBNum = 0;
    ET9U8                       i = 0;
    ET9U8                       bStartPoint = 0;
    ET9U8                       bBoostPoint = 0;
    ET9U8                       bBoostWordlistIndex = 0;
    ET9U8                       bIndex = 0;
    ET9U8                       bTargetLangIndexlookup = 0;
    ET9U8                       bTestLangIndexlookup = 0;
    ET9BOOL                     bDefaultCompletion = 0;
    ET9BOOL                     bDefaultRDB = 0;
    ET9AEXACTINLIST             eSetting = ET9AEXACTINLIST_OFF;
    ET9AWLingCmnInfo  * const   pLingCmnInfo = pLingInfo->pLingCmnInfo;
    ET9AWPrivWordInfo * const   pWordList = pLingCmnInfo->Private.pWordList;
    ET9U8             * const   bWordList = pLingCmnInfo->Private.bWordList;

    ET9AssertLog(pLingInfo);

    if (!pLingCmnInfo->Private.bTotalSymbInputs) {
        return;
    }

    pLingCmnInfo->Private.bApplyBoosting = 0;

    ET9AWGetExactInList(pLingInfo, &eSetting);

    bStartPoint = 0;
    bBoostPoint = pLingCmnInfo->Private.bDefaultIndex;
    bBoostWordlistIndex = bWordList[bBoostPoint];

    wSavedLDBNum = pLingCmnInfo->wLdbNum;

    wPreviousLDBNum = pLingCmnInfo->Private.wPreviousWordLanguage;

    if (GETRAWSRC(pWordList[bBoostWordlistIndex].bWordSrc) == ET9WORDSRC_LDB || pWordList[bBoostWordlistIndex].bWordSrc == ET9WORDSRC_BUILDAROUND_LAS) {

        xTargetTotFreq = pWordList[bBoostWordlistIndex].xTotFreq;
        wTargetEFreq = pWordList[bBoostWordlistIndex].wEWordFreq;
        wTargetTFreq = pWordList[bBoostWordlistIndex].wTWordFreq;
        wTargetWFreq = wTargetEFreq + wTargetTFreq;
        dwTargetIndex = pWordList[bBoostWordlistIndex].dwWordIndex;

        if (pWordList[bBoostWordlistIndex].Base.bLangIndex == ET9AWSECOND_LANGUAGE ||
            (pWordList[bBoostWordlistIndex].Base.bLangIndex == ET9AWBOTH_LANGUAGES && pLingCmnInfo->wSecondLdbNum == wPreviousLDBNum)) {

            bTargetLangIndexlookup = ET9AWSECOND_LANGUAGE;
        }
        else {
            bTargetLangIndexlookup = ET9AWFIRST_LANGUAGE;
        }

    }
    else if (pWordList[bBoostWordlistIndex].dwWordIndex > 0) {

        if (pWordList[bBoostWordlistIndex].Base.bLangIndex == ET9AWSECOND_LANGUAGE ||
            (pWordList[bBoostWordlistIndex].Base.bLangIndex == ET9AWBOTH_LANGUAGES && pLingCmnInfo->wSecondLdbNum == wPreviousLDBNum)) {

            ET9AssertLog(ET9AWSys_GetBilingualSupported(pLingInfo));

            bTargetLangIndexlookup = ET9AWSECOND_LANGUAGE;

            if (pLingCmnInfo->wSecondLdbNum != pLingCmnInfo->wLdbNum) {
                wStatus = _ET9AWLdbSetActiveLanguage(pLingInfo, pLingCmnInfo->wSecondLdbNum);

                if (!wStatus) {
                    _ET9AWLdbTagContextClass(pLingInfo);
                }
            }

        }
        else {
            bTargetLangIndexlookup = ET9AWFIRST_LANGUAGE;

            if (pLingCmnInfo->wFirstLdbNum != pLingCmnInfo->wLdbNum) {
                wStatus = _ET9AWLdbSetActiveLanguage(pLingInfo, pLingCmnInfo->wFirstLdbNum);

                if (!wStatus) {
                    _ET9AWLdbTagContextClass(pLingInfo);
                }
            }
        }

        if (wStatus) {
            _ET9AWLdbSetActiveLanguage(pLingInfo, wSavedLDBNum);
            return;
        }

        _ET9AWLdbGetWordFreq(pLingInfo,
                             pLingCmnInfo->wLdbNum,
                             pWordList[bBoostWordlistIndex].dwWordIndex - 1,
                             &xTargetFreq,
                             &wTargetEFreq,
                             &wTargetTFreq);

        xTargetTotFreq = pWordList[bBoostWordlistIndex].xTapFreq * xTargetFreq;
        wTargetWFreq = wTargetEFreq + wTargetTFreq;
        dwTargetIndex = pWordList[bBoostWordlistIndex].dwWordIndex;
        bDefaultRDB = 1;
    }
    else {
        return;
    }

    wWordLen = pWordList[bBoostWordlistIndex].Base.wWordLen;
    bDefaultCompletion = pWordList[bBoostWordlistIndex].Base.wWordCompLen ? 1 : 0;

    for (i = bStartPoint; i < pLingCmnInfo->Private.bTotalWords - 1; ++i) {

        if (i == bBoostPoint) {
            continue;
        }

        bIndex = bWordList[i];

        if (pWordList[bIndex].Base.bLangIndex == ET9AWSECOND_LANGUAGE ||
            (pWordList[bIndex].Base.bLangIndex == ET9AWBOTH_LANGUAGES && pLingCmnInfo->wSecondLdbNum == wPreviousLDBNum)) {

            bTestLangIndexlookup = ET9AWSECOND_LANGUAGE;
        }
        else {
            bTestLangIndexlookup = ET9AWFIRST_LANGUAGE;
        }

        if (bTargetLangIndexlookup != bTestLangIndexlookup) {
            continue;
        }

        if (GETRAWSRC(pWordList[bIndex].bWordSrc) == ET9WORDSRC_LDB || pWordList[bIndex].bWordSrc == ET9WORDSRC_BUILDAROUND_LAS) {

            if (!ISBUILDAROUNDSRC(pWordList[bBoostWordlistIndex].bWordSrc) && ISBUILDAROUNDSRC(pWordList[bIndex].bWordSrc)) {
                continue;
            }

            xTestTotFreq = pWordList[bIndex].xTotFreq;
            wTestEFreq = pWordList[bIndex].wEWordFreq;
            wTestTFreq = pWordList[bIndex].wTWordFreq;
            wTestWFreq = wTestEFreq + wTestTFreq;
            dwTestIndex = pWordList[bIndex].dwWordIndex;
        }
        else if (pWordList[bIndex].dwWordIndex > 0) {

            if (!ISBUILDAROUNDSRC(pWordList[bBoostWordlistIndex].bWordSrc) && ISBUILDAROUNDSRC(pWordList[bIndex].bWordSrc)) {
                continue;
            }

            wStatus = ET9STATUS_NONE;

            switch (bTestLangIndexlookup) {
                case ET9AWFIRST_LANGUAGE:
                    if (pLingCmnInfo->wFirstLdbNum != pLingCmnInfo->wLdbNum) {
                        wStatus = _ET9AWLdbSetActiveLanguage(pLingInfo, pLingCmnInfo->wFirstLdbNum);
                        if (!wStatus) {
                            _ET9AWLdbTagContextClass(pLingInfo);
                        }
                    }
                    break;
                case ET9AWSECOND_LANGUAGE:
                    if (pLingCmnInfo->wSecondLdbNum != pLingCmnInfo->wLdbNum) {
                        wStatus = _ET9AWLdbSetActiveLanguage(pLingInfo, pLingCmnInfo->wSecondLdbNum);
                        if (!wStatus) {
                            _ET9AWLdbTagContextClass(pLingInfo);
                        }
                    }
                    break;
            }

            if (wStatus != ET9STATUS_NONE) {
                wStatus = _ET9AWLdbSetActiveLanguage(pLingInfo, wSavedLDBNum);
                continue;
            }

            _ET9AWLdbGetWordFreq(pLingInfo,
                                 pLingCmnInfo->wLdbNum,
                                 pWordList[bIndex].dwWordIndex - 1,
                                 &xTestFreq,
                                 &wTestEFreq,
                                 &wTestTFreq);

            xTestTotFreq = pWordList[bIndex].xTapFreq * xTestFreq;
            wTestWFreq = wTestEFreq + wTestTFreq;
            dwTestIndex = pWordList[bIndex].dwWordIndex;
        }
        else {
            continue;
        }

        if (ISREALSRC(pWordList[bIndex].bWordSrc)) {

            if (ET9BOOSTTOPCANDIDATE(pLingCmnInfo) &&
                pLingCmnInfo->Private.eCurrSelectionListMode == ET9ASLMODE_COMPLETIONSPROMOTED &&
                pLingCmnInfo->Base.pWordSymbInfo->bNumSymbs >= ET9AWSys_GetWordCompletionPoint(pLingInfo)) { /* boost terms + completions */

                if (!(pWordList[bIndex].bEditDistStem || pWordList[bIndex].bEditDistSpc)) {

                    if ((wWordLen * xTestTotFreq >  pWordList[bIndex].Base.wWordLen * xTargetTotFreq) ||
                        (wWordLen * xTestTotFreq == pWordList[bIndex].Base.wWordLen * xTargetTotFreq && wWordLen * wTestWFreq > pWordList[bIndex].Base.wWordLen * wTargetWFreq) ||
                        (wWordLen * xTestTotFreq == pWordList[bIndex].Base.wWordLen * xTargetTotFreq && wWordLen * wTestWFreq == pWordList[bIndex].Base.wWordLen * wTargetWFreq && wWordLen * dwTargetIndex > pWordList[bIndex].Base.wWordLen * dwTestIndex)) {

                        if (bBoostPoint == pLingCmnInfo->Private.bDefaultIndex) {
                            pLingCmnInfo->Private.bDefaultIndex = i;
                        }

                        bBoostPoint = i;
                        xTargetTotFreq = xTestTotFreq;
                        wTargetWFreq = wTestWFreq;
                        dwTargetIndex = dwTestIndex;
                        bBoostWordlistIndex = bIndex;
                        wWordLen = pWordList[bIndex].Base.wWordLen;
                        pLingCmnInfo->Private.bApplyBoosting = 1;
                    }
                }
            }
            else if (bDefaultRDB) { /* boost only terms */

                if (!(pWordList[bIndex].bEditDistStem || pWordList[bIndex].bEditDistSpc || !pWordList[bIndex].Base.bIsTerm || pWordList[bIndex].Base.wWordCompLen)) {

                    if ((xTestTotFreq >  xTargetTotFreq) ||
                        (xTestTotFreq == xTargetTotFreq && wTestWFreq > wTargetWFreq) ||
                        (xTestTotFreq == xTargetTotFreq && wTestWFreq == wTargetWFreq && dwTargetIndex > dwTestIndex)) {

                        if (bBoostPoint == pLingCmnInfo->Private.bDefaultIndex) {
                            pLingCmnInfo->Private.bDefaultIndex = i;
                        }

                        bBoostPoint = i;
                        xTargetTotFreq = xTestTotFreq;
                        wTargetWFreq = wTestWFreq;
                        dwTargetIndex = dwTestIndex;
                        bBoostWordlistIndex = bIndex;
                        wWordLen = pWordList[bIndex].Base.wWordLen;
                        pLingCmnInfo->Private.bApplyBoosting = 1;
                    }
                }
            }
        }
    }

    if (bBoostPoint > bStartPoint) {

        bStartPoint = eSetting == ET9AEXACTINLIST_FIRST && bStartPoint == pLingCmnInfo->Private.bExactIndex ? bStartPoint + 1 : bStartPoint;

        if (bBoostPoint > bStartPoint) {

            WLOG2(fprintf(pLogFile2, "__BoostTopCandidate, %u moves to %u\n", bBoostPoint - 1, bStartPoint);)

            __ShiftSubListDown(pLingCmnInfo, bStartPoint, bBoostPoint - 1);

            __LogPartialSelList(pLingCmnInfo, 0, (ET9U8)(pLingCmnInfo->Private.bTotalWords - 1), "after boost op", pLogFile2);

            bWordList[bStartPoint] = bBoostWordlistIndex;
        }

        pLingCmnInfo->Private.bDefaultIndex = bStartPoint;
    }

    if (wSavedLDBNum != pLingCmnInfo->wLdbNum) {

        wStatus = _ET9AWLdbSetActiveLanguage(pLingInfo, wSavedLDBNum);

        if (!wStatus) {
            _ET9AWLdbTagContextClass(pLingInfo);
        }
    }
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Build around special.
 * An internal help function for the selection list builder.
 *
 * @param pLingInfo         Pointer to alphabetic information structure.
 * @param wStartPoint       ?
 * @param wSpecialPosition  Special position.
 * @param eSpecialType      Special char type.
 * @param pLeftHandWord     Pointer to left hand word.
 * @param pDefaultWord      The current default word.
 * @param eBuildaroundType  Type of builaround.
 * @param sEmbeddedChar     The embedded char (if any).
 * @param bSpcMode          Current spell correction mode.
 *
 * @return Zero on failure, otherwise non zero.
 */

static ET9U8 ET9LOCALCALL __ET9BuildAroundSpecial(ET9AWLingInfo * const     pLingInfo,
                                                  const ET9U16              wStartPoint,
                                                  const ET9U16              wSpecialPosition,
                                                  const ET9ASPECIAL         eSpecialType,
                                                  ET9AWPrivWordInfo * const pLeftHandWord,
                                                  ET9AWPrivWordInfo * const pDefaultWord,       /* obsolete? Replaced by the flush */
                                                  const ET9ABUILAROUND      eBuildaroundType,
                                                  const ET9SYMB             sEmbeddedChar,
                                                  const ET9U8               bSpcMode)
{
    ET9BOOL     bFound;
    ET9BOOL     bFoundFull;
    ET9U16      wLeftLength;
    ET9U16      wRightLength;
    ET9U16      wRightStartIndex;
    ET9SYMB     sSymb;
    ET9U8       bTempLeftLangIndex;

    ET9AWLingCmnInfo * const    pLingCmnInfo = pLingInfo->pLingCmnInfo;
    ET9WordSymbInfo * const     pWordSymbInfo = pLingCmnInfo->Base.pWordSymbInfo;

    const ET9U16 wLeftHandLen = wSpecialPosition + wStartPoint - (eBuildaroundType == BA_LEADING ? 1 : 0);

    ET9AssertLog(pLingInfo != NULL);
    ET9AssertLog(pLeftHandWord != NULL);
    ET9AssertLog(pDefaultWord != NULL);

    WLOG2(fprintf(pLogFile2, "__ET9BuildAroundSpecial\n  StartPoint = %d\n  SpecialPosition = %d\n  SpecialType = %s\n  BuildaroundType = %s\n  sEmbeddedChar = %c (%x)\n", wStartPoint, wSpecialPosition, SPECIALTOSTRING(eSpecialType), BUILDAROUNDTOSTRING(eBuildaroundType), (sEmbeddedChar ? sEmbeddedChar : ' '), sEmbeddedChar);)

    WLOG2Word(pLogFile2, "..sLeftHandWord", pLeftHandWord);
    WLOG2Word(pLogFile2, "..DefaultWord", pDefaultWord);

    /* validate */

    if (!pWordSymbInfo->bNumSymbs) {
        return 0;
    }

    /* */

    if (eBuildaroundType == BA_TRAILING || eBuildaroundType == BA_COMPOUND) {
        wLeftLength = wSpecialPosition;
        wRightLength = (ET9U16)(pWordSymbInfo->bNumSymbs - wSpecialPosition - wStartPoint);
        wRightStartIndex = (ET9U16)(wSpecialPosition + wStartPoint);

        WLOG2(fprintf(pLogFile2, "..trailing or compound, wLeftLength = %d, wRightLength = %d, wRightStartIndex = %d\n", wLeftLength, wRightLength, wRightStartIndex);)
    }
    else if (eBuildaroundType == BA_LEADING) {
        wLeftLength = (ET9U16)(wSpecialPosition - 1);
        wRightLength = (ET9U16)(pWordSymbInfo->bNumSymbs - wSpecialPosition + 1 - wStartPoint);
        wRightStartIndex = (ET9U16)(wSpecialPosition - 1 + wStartPoint);

        WLOG2(fprintf(pLogFile2, "..leading, wLeftLength = %d, wRightLength = %d, wRightStartIndex = %d\n", wLeftLength, wRightLength, wRightStartIndex);)
    }
    else {
        ET9AssertLog(eBuildaroundType == BA_EMBEDDED);

        wLeftLength = (ET9U16)(wSpecialPosition - 1);
        wRightLength = (ET9U16)(pWordSymbInfo->bNumSymbs - wSpecialPosition - wStartPoint);
        wRightStartIndex = (ET9U16)(wSpecialPosition + wStartPoint);

        WLOG2(fprintf(pLogFile2, "..embedded, wLeftLength = %d, wRightLength = %d, wRightStartIndex = %d\n", wLeftLength, wRightLength, wRightStartIndex);)
    }

    bTempLeftLangIndex = pLeftHandWord->Base.bLangIndex;

    /* build left side */

    _InitPrivWordInfo(pLeftHandWord);

    pLeftHandWord->Base.bLangIndex = bTempLeftLangIndex;

    bFound = 1;
    bFoundFull = 0;

    if (wLeftLength) {

        ET9U8 bStemsAllowed = 0;

        /* allow getting top match stem words if this is an explicit at the end of a word, and the left */

        if ((eBuildaroundType == BA_EMBEDDED) || (eBuildaroundType == BA_LEADING)) {
            bStemsAllowed = 1;
        }

        if (eSpecialType != LOCKPOINT) {

            bFound = __CaptureGetWord(pLingInfo,
                                      pLeftHandWord,
                                      wStartPoint,
                                      wLeftLength,
                                      bStemsAllowed);

            /* Was: if this is a leading or embedded special, and there is no top match found in the
                    database, and exact is included in the list, use the default match as the left hand side. */

            if (!bFound && (eBuildaroundType == BA_LEADING || eBuildaroundType == BA_EMBEDDED)) {

                ET9U16 wLen;

                WLOG2(fprintf(pLogFile2, "no capture found\n");)

                if ((wLeftLength == 1) && pWordSymbInfo->SymbsInfo[wStartPoint].bSymbType == ET9KTSMARTPUNCT) {

                    WLOG2(fprintf(pLogFile2, "wLeftLength == 1 - special (embedded punct char)\n");)

                    wLen = 1;

                    sSymb = _ET9_GetEmbPunctChar(pLingInfo, pLingCmnInfo->wLdbNum);

                    _ET9SymCopy(pLeftHandWord->Base.sWord, &sSymb, 1);
                }
                else {

                    const ET9U16 wCopyStartPoint = __CaptureGetFlushStringLength(pLingInfo, 0);

                    ET9AssertLog(wStartPoint == __CaptureGetFlushPoint(pLingInfo, 0));

                    if (wStartPoint) {
                        wLen = wLeftLength;
                    }
                    else {
                        wLen = __CaptureGetDefaultStringLength(pLingInfo, wLeftLength + 1);
                    }

                    if (wLen + wCopyStartPoint > pDefaultWord->Base.wWordLen) {

                        WLOG2(fprintf(pLogFile2, "default is too short - unusable\n");)

                        wLen = 0;
                    }
                    else {

                        WLOG2(fprintf(pLogFile2, "using %d symbs (%d chars) of default from pos %d (symb start %d)\n", wLeftLength, wLen, wCopyStartPoint, wStartPoint);)

                        _ET9SymCopy(pLeftHandWord->Base.sWord, &pDefaultWord->Base.sWord[wCopyStartPoint], wLen);
                    }
                }

                /* update if we ended up with something */

                if (wLen) {

                    /* don't need to set the rest of the elements of pLeftHandWord */

                    pLeftHandWord->Base.wWordLen = wLen;
                    pLeftHandWord->Base.wWordCompLen = 0;

                    WLOG2Word(pLogFile2, "__ET9BuildAroundSpecial, LeftHandWord (from prev default)", pLeftHandWord);

                    bFound = 1;
                }
                else {
                    bFound = 0;
                }
            }
        }
        else {

            ET9U16      wCount;
            ET9SYMB     *psSymb = pLeftHandWord->Base.sWord;
            ET9SymbInfo *pSymbInfo = pWordSymbInfo->SymbsInfo;

            for (wCount = wStartPoint + wLeftLength; wCount; --wCount, ++psSymb, ++pSymbInfo) {
                *psSymb = pSymbInfo->sLockedSymb;
            }

            pLeftHandWord->Base.wWordLen = wStartPoint + wLeftLength;
            pLeftHandWord->Base.wWordCompLen = 0;

            /* all the elements of pLeftHandWord set */

            WLOG2Word(pLogFile2, "__ET9BuildAroundSpecial, LeftHandWord (from lock)", pLeftHandWord);

            bFound = 1;
            bFoundFull = 1;     /* no need to prepend */
        }
    }

    /*  at this point, if bFound, we have a top match term for symbs up to and including the special.
        lets build after the special. */

    if (!bFound) {
        WLOG2(fprintf(pLogFile2, "__ET9BuildAroundSpecial, no term found to build on (exit)\n");)
        return 0;
    }

    if (wRightLength) {

        ET9BOOL bLeftHandWordOk;

        WLOG2(fprintf(pLogFile2, "__ET9BuildAroundSpecial, bFound & wRightLength, wRightLength = %d\n", wRightLength);)
        WLOG2Word(pLogFile2, "__ET9BuildAroundSpecial, LeftHandWord", pLeftHandWord);

        if (eBuildaroundType == BA_EMBEDDED) {
            pLeftHandWord->Base.sWord[pLeftHandWord->Base.wWordLen] = sEmbeddedChar;
            ++pLeftHandWord->Base.wWordLen;

            WLOG2Word(pLogFile2, "__ET9BuildAroundSpecial, LeftHandWord with embedded", pLeftHandWord);

        }

        /* build right side */
        /* need to prepend lefthandword with flushed word */

        if (wStartPoint && !bFoundFull) {

            bLeftHandWordOk = __ET9PrependWord(pLeftHandWord, pDefaultWord, __CaptureGetFlushStringLength(pLingInfo, 0));

            WLOG2Word(pLogFile2, "__ET9BuildAroundSpecial, LeftHandWord prepended with default", pLeftHandWord);
        }
        else {
            bLeftHandWordOk = 1;
        }

        if (bLeftHandWordOk) {

            ET9U8   bLdbEntries;
            ET9U8   bSuppEntries;
            ET9U8   bSuppASEntries;

            bLdbEntries = 0;
            bSuppEntries = 0;
            bSuppASEntries = 0;

            pLingCmnInfo->Private.bStemsAllowed = __GetStemsAllowed(pLingCmnInfo, wRightLength);
            pLingCmnInfo->Private.wMaxWordLength = __GetMaxWordLength(pLingCmnInfo, wRightLength);

            __AssureLockedString(pLingInfo, pLeftHandWord, 0);
            __UpdateSpcInfo(pLingInfo, pLeftHandWord, 0, wLeftHandLen, bSpcMode);

            WLOG2(fprintf(pLogFile2, "__ET9BuildAroundSpecial, performing _ET9AWSuppDBSelListBuild (buildaround non AS), index = %d, length = %d\n", wRightStartIndex, wRightLength);)

            _ET9AWSuppDBSelListBuild(pLingInfo,
                                     wRightStartIndex,
                                     wRightLength,
                                     &bSuppEntries,
                                     eBuildaroundType == BA_COMPOUND ? FREQ_BUILDCOMPOUND : FREQ_BUILDAROUND,
                                     ET9SUPPDB_NONAS_SOURCES,
                                     bSpcMode);

            WLOG2(fprintf(pLogFile2, "__ET9BuildAroundSpecial, performing _ET9AWLdbWordsSearch (buildaround), index = %d, length = %d\n", wRightStartIndex, wRightLength);)

            if (((pLingCmnInfo->wFirstLdbNum & ET9PLIDMASK) != ET9PLIDNone) &&
                !(wRightStartIndex && (pLeftHandWord->Base.bLangIndex == ET9AWSECOND_LANGUAGE))) {

                ET9U8 bSavedSpcMode = bSpcMode;

                if (!ET9INACTIVELANGSPELLCORRECTENABLED(pLingCmnInfo) &&
                    pLingCmnInfo->wLdbNum != pLingCmnInfo->wFirstLdbNum) {
                    bSavedSpcMode = ET9ASPCMODE_OFF;
                }

                _ET9AWLdbWordsSearch(pLingInfo,
                                     pLingCmnInfo->wFirstLdbNum,
                                     wRightStartIndex,
                                     wRightLength,
                                     &bLdbEntries,
                                     eBuildaroundType == BA_COMPOUND ? FREQ_BUILDCOMPOUND : FREQ_BUILDAROUND,
                                     bSavedSpcMode);

            }

            if (ET9AWSys_GetBilingualSupported(pLingInfo) &&
                !(wRightStartIndex && (pLeftHandWord->Base.bLangIndex == ET9AWFIRST_LANGUAGE))) {

                ET9U8 bSavedSpcMode = bSpcMode;

                if (!ET9INACTIVELANGSPELLCORRECTENABLED(pLingCmnInfo) &&
                    pLingCmnInfo->wLdbNum != pLingCmnInfo->wSecondLdbNum) {
                    bSavedSpcMode = ET9ASPCMODE_OFF;
                }

                _ET9AWLdbWordsSearch(pLingInfo,
                                     pLingCmnInfo->wSecondLdbNum,
                                     wRightStartIndex,
                                     wRightLength,
                                     &bLdbEntries,
                                     eBuildaroundType == BA_COMPOUND ? FREQ_BUILDCOMPOUND : FREQ_BUILDAROUND,
                                     bSavedSpcMode);
            }

            WLOG2(fprintf(pLogFile2, "__ET9BuildAroundSpecial, performing _ET9AWSuppDBSelListBuild (buildaround AS), index = %d, length = %d\n", wRightStartIndex, wRightLength);)

            _ET9AWSuppDBSelListBuild(pLingInfo,
                                     wRightStartIndex,
                                     wRightLength,
                                     &bSuppASEntries,
                                     eBuildaroundType == BA_COMPOUND ? FREQ_BUILDCOMPOUND : FREQ_BUILDAROUND,
                                     ET9SUPPDB_AS_SOURCES,
                                     bSpcMode);

            WLOG2(fprintf(pLogFile2, "__ET9BuildAroundSpecial, right side build results, bSuppEntries = %d, bSuppASEntries = %d, bLdbEntries = %d, bTotalWords = %d (exit)\n", (int)bSuppEntries, (int)bSuppASEntries, (int)bLdbEntries, (int)pLingCmnInfo->Private.bTotalWords);)

            return (bSuppEntries || bSuppASEntries || bLdbEntries) ? 1 : 0;
        }
        else {
            WLOG2(fprintf(pLogFile2, "__ET9BuildAroundSpecial, not lhw ok (exit)\n");)
            return 0;
        }
    }

    if (!wRightLength) {

        ET9BOOL bLeftHandWordOk;
        ET9AWPrivWordInfo TermEmbeddedWord;

        WLOG2(fprintf(pLogFile2, "__ET9BuildAroundSpecial, bFound & !wRightLength\n");)

        if ((eBuildaroundType != BA_EMBEDDED) && eBuildaroundType != BA_COMPOUND) {
            WLOG2(fprintf(pLogFile2, "__ET9BuildAroundSpecial, not embedded and not compound (exit)\n");)
            return 0;
        }

        if (wStartPoint && !bFoundFull) {

            bLeftHandWordOk = __ET9PrependWord(pLeftHandWord, pDefaultWord, __CaptureGetFlushStringLength(pLingInfo, 0));

            WLOG2Word(pLogFile2, "__ET9BuildAroundSpecial, LeftHandWord prepended with default", pLeftHandWord);
       }
        else {
            bLeftHandWordOk = 1;
        }

        _InitPrivWordInfo(&TermEmbeddedWord);

        TermEmbeddedWord.Base.bIsTerm = 1;

        TermEmbeddedWord.bWordSrc = ET9WORDSRC_LDB;

        if (eBuildaroundType == BA_EMBEDDED) {

            if (bLeftHandWordOk && pLeftHandWord->Base.wWordLen < ET9MAXWORDSIZE) {

                /* need to just add word with embedded character at the end */

                pLeftHandWord->Base.sWord[pLeftHandWord->Base.wWordLen] = sEmbeddedChar;
                ++pLeftHandWord->Base.wWordLen;

                __UpdateSpcInfo(pLingInfo, pLeftHandWord, 0, wLeftHandLen, bSpcMode);

                WLOG2Word(pLogFile2, "__ET9BuildAroundSpecial, LeftHandWord with embedded", pLeftHandWord);

                pLingCmnInfo->Private.bStemsAllowed = 1;
                pLingCmnInfo->Private.wMaxWordLength = ET9MAXWORDSIZE;

                __ET9AWSelLstInsert(pLingInfo, &TermEmbeddedWord, FREQ_BUILDAROUND, 0);

                WLOG2(fprintf(pLogFile2, "__ET9BuildAroundSpecial, added term embedded word (exit)\n");)

                return 1;
            }
        }
        else {

            __UpdateSpcInfo(pLingInfo, pLeftHandWord, 0, wLeftHandLen, bSpcMode);

            pLingCmnInfo->Private.bStemsAllowed = 1;
            pLingCmnInfo->Private.wMaxWordLength = ET9MAXWORDSIZE;

            __ET9AWSelLstInsert(pLingInfo, &TermEmbeddedWord, FREQ_BUILDCOMPOUND, 0);

            WLOG2(fprintf(pLogFile2, "__ET9BuildAroundSpecial, added term lock (exit)\n");)

            return 1;
        }
    }

    WLOG2(fprintf(pLogFile2, "__ET9BuildAroundSpecial, nothing (exit)\n");)

    return 0;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Restore selection list override values.
 *
 * @param pLingCmnInfo  Pointer to alphabetic cmn information structure.
 *
 * @return None
 */

#define __RestoreSelectionListOverrideValues(pLingCmnInfo)                                              \
    {                                                                                                   \
        pLingCmnInfo->Private.ASpc.wMaxSpcTermCount = wMaxSpcTermCountSave;                             \
        pLingCmnInfo->Private.ASpc.wMaxSpcCmplCount = wMaxSpcCmplCountSave;                             \
        pLingCmnInfo->Private.wMaxCompletionCount = wMaxCompletionCountSave;                            \
        pLingCmnInfo->Private.wWordCompletionPoint = wWordCompletionPointSave;                          \
        pLingCmnInfo->Private.ASpc.bSpcFeatures = bSpcFeaturesSave;                                     \
        pLingCmnInfo->Private.bStateWordCompletion = bStateWordCompletionSave;                          \
    }                                                                                                   \

/*---------------------------------------------------------------------------*/
/** \internal
 * Perform a selection list build.
 * This function builds a selection list for the current tap sequence.
 * (It performs a build at a specific input length.)
 *
 * @param pLingInfo         Pointer to alphabetic information structure.
 * @param pbTotalWords      Pointer to total words found.
 * @param bSuppressBuild    If in a catchup build phase then heavy parts of the build can be suppressed.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

static ET9STATUS ET9LOCALCALL __ET9AWDoSelLstBuild(ET9AWLingInfo    * const pLingInfo,
                                                   ET9U8            * const pbTotalWords,
                                                   const ET9BOOL            bSuppressBuild)
{
    ET9AWLingCmnInfo    * const pLingCmnInfo = pLingInfo->pLingCmnInfo;
    ET9WordSymbInfo     * const pWordSymbInfo = pLingCmnInfo->Base.pWordSymbInfo;
    ET9AWPrivWordInfo   * const pLeftHandWord = &pLingCmnInfo->Private.sLeftHandWord;
    ET9AWPrivWordInfo   * const pPrevDefaultWord = &pLingCmnInfo->Private.sPrevDefaultWord;
    ET9U8               * const pbLangSupported = pLingCmnInfo->Private.pbLangSupported;

    ET9U16              i;
    ET9U16      * const pwLockPoint = &pLingCmnInfo->Private.wCurrLockPoint;
    ET9U16              wFlushPoint;
    ET9U16              wFullWordMatches;
    ET9U16              wSpecialPosition;
    ET9ASPECIAL         eSpecialType;
    ET9BOOL             bExactLock;
    ET9BOOL             bExactWon;
    ET9BOOL             bHasTermPunct;
    ET9BOOL             bHasTermSmartPunct;
    ET9BOOL             bContinue;
    ET9BOOL             bDefaultOkay;
    ET9BOOL             bFound;
    ET9SymbInfo         *pSymbInfo;
    ET9AWPrivWordInfo   *pWord;
    ET9AWPrivWordInfo   *pAmbigWord;
    ET9AWPrivWordInfo   *pExactWord = NULL;
    ET9SimpleWord       sSimpleWord;
    ET9U8               bSecondLangTermPunctFirst = 0;
    ET9U16              wSavedLockPoint = 0;

    const ET9U8         bNumSymbs = pWordSymbInfo->bNumSymbs;
    const ET9U8         bLastBuildLen = pLingCmnInfo->Private.bLastBuildLen;
    const ET9BOOL       bLastInputIsTrace = (ET9BOOL)(bNumSymbs && pWordSymbInfo->SymbsInfo[bNumSymbs - 1].bTraceIndex);
    const ET9BOOL       bLastInputIsMultitap = (ET9BOOL)(bNumSymbs && pWordSymbInfo->SymbsInfo[bNumSymbs - 1].eInputType == ET9MULTITAPKEY);
    const ET9BOOL       bLastInputIsSmartPunct = (ET9BOOL)(bNumSymbs && pWordSymbInfo->SymbsInfo[bNumSymbs - 1].bSymbType == ET9KTSMARTPUNCT);
    const ET9BOOL       bExactInList = __ExactInList(pLingInfo);
    const ET9BOOL       bHasTraceInfo = _ET9HasTraceInfo(pWordSymbInfo);
    const ET9BOOL       bHasRegionalInfo = _ET9HasRegionalInfo(pWordSymbInfo);
    const ET9BOOL       bHasDiscreteOnlyInfo = _ET9HasDiscreteOnlyInfo(pWordSymbInfo);
    const ET9BOOL       bHasAllShiftedInfo = _ET9HasAllShiftedInfo(pWordSymbInfo);
    const ET9BOOL       bUsingLM = (ET9BOOL)(ET9LMENABLED(pLingCmnInfo) && pLingCmnInfo->Private.ALdbLM.bSupported);
    const ET9U8         bSpcMode = (ET9U8)(pWordSymbInfo->Private.bRequiredInhibitCorrection ? ET9ASPCMODE_OFF : (bHasTraceInfo ? ET9ASPCMODE_REGIONAL : pLingCmnInfo->Private.ASpc.eMode));

    const ET9BOOL       bStateWordCompletionSave = pLingCmnInfo->Private.bStateWordCompletion;  /* restore on return */
    const ET9U8         bSpcFeaturesSave = pLingCmnInfo->Private.ASpc.bSpcFeatures;             /* restore on return */
    const ET9U16        wMaxSpcTermCountSave = pLingCmnInfo->Private.ASpc.wMaxSpcTermCount;     /* restore on return */
    const ET9U16        wMaxSpcCmplCountSave = pLingCmnInfo->Private.ASpc.wMaxSpcCmplCount;     /* restore on return */
    const ET9U16        wMaxCompletionCountSave = pLingCmnInfo->Private.wMaxCompletionCount;    /* restore on return */
    const ET9U16        wWordCompletionPointSave = pLingCmnInfo->Private.wWordCompletionPoint;  /* restore on return */

    ET9AssertLog(pLingInfo);
    ET9AssertLog(pbTotalWords);
    ET9AssertLog(bSpcMode <= ET9ASPCMODE_REGIONAL);

    /* log build state */

    WLOG2(fprintf(pLogFile2, "__ET9AWDoSelLstBuild, START *****\n");)
    WLOG2(fprintf(pLogFile2, "bNumSymbs = %d, bLastBuildLen = %d, wLdbNum = %04X\n", bNumSymbs, bLastBuildLen, pLingCmnInfo->wLdbNum);)
    WLOG2(fprintf(pLogFile2, "exactInList = %c, exactLast = %c, exactDefault = %c\n", (ET9EXACTINLIST(pLingCmnInfo) ? 'Y' : 'N'), (ET9EXACTLAST(pLingCmnInfo) ? 'Y' : 'N'), (ET9EXACTISDEFAULT(pLingCmnInfo) ? 'Y' : 'N'));)
    WLOG2(fprintf(pLogFile2, "maxCompletionCount = %d, maxSpcTermCount = %d, maxSpcCmplCount = %d\n", pLingCmnInfo->Private.wMaxCompletionCount, pLingCmnInfo->Private.ASpc.wMaxSpcTermCount, pLingCmnInfo->Private.ASpc.wMaxSpcCmplCount);)
    WLOG2(fprintf(pLogFile2, "wordStemsActive = %c, wWordStemsPoint = %d\n", (ET9WORDSTEMS_MODE(pLingCmnInfo) ? 'Y' : 'N'), pLingCmnInfo->Private.wWordStemsPoint);)
    WLOG2(fprintf(pLogFile2, "wordCompletionActive = %c, wWordCompletionPoint = %d\n", (ET9WORDCOMPLETION_MODE(pLingCmnInfo) ? 'Y' : 'N'), pLingCmnInfo->Private.wWordCompletionPoint);)
    WLOG2(fprintf(pLogFile2, "SPC eMode = %s (%s), eSearchFilter = %s\n", SPCMODETOSTRING(pLingCmnInfo->Private.ASpc.eMode), SPCMODETOSTRING(bSpcMode), SPCFILTERTOSTRING(pLingCmnInfo->Private.ASpc.eSearchFilter));)
    WLOG2(fprintf(pLogFile2, "eAltMode = %s\n", ALTMODETOSTRING(pLingCmnInfo->Private.eAltMode));)
    WLOG2(fprintf(pLogFile2, "eSelectionListMode = %s\n", SELLSTMODETOSTRING(pLingCmnInfo->Private.eSelectionListMode));)
    WLOG2(fprintf(pLogFile2, "eSelectionListCorrectionMode = %s\n", SELLSTCORRMODETOSTRING(pLingCmnInfo->Private.eSelectionListCorrectionMode));)
    WLOG2(fprintf(pLogFile2, "OTFM = %c\n", (!pLingInfo->Private.pConvertSymb ? 'N' : 'Y'));)
    WLOG2(fprintf(pLogFile2, "postSort = %c\n", ET9POSTSORTENABLED(pLingCmnInfo) ? 'Y' : 'N');)
    WLOG2(fprintf(pLogFile2, "bHasTraceInfo = %c, bHasRegionalInfo = %c, bUsingLM = %c\n", (bHasTraceInfo ? 'Y' : 'N'), (bHasRegionalInfo ? 'Y' : 'N'), (bUsingLM ? 'Y' : 'N'));)
    WLOG2(fprintf(pLogFile2, "bRequiredLocate = %c\n", pWordSymbInfo->Private.bRequiredLocate ? 'Y' : 'N');)
    WLOG2(fprintf(pLogFile2, "bSuppressBuild = %c\n", bSuppressBuild ? 'Y' : 'N');)

    /* handle tracking */

    if (bSpcMode) {
        pLingCmnInfo->Private.bSpcDuringBuild = 1;  /* since there is a local override above */
    }

    /* set current selection list mode */

    if (pLingCmnInfo->Private.eSelectionListMode == ET9ASLMODE_AUTO) {

        if (bHasTraceInfo) {

            if (bHasRegionalInfo) {
                pLingCmnInfo->Private.eCurrSelectionListMode = ET9ASLMODE_MIXED;
            }
            else {
                pLingCmnInfo->Private.eCurrSelectionListMode = ET9ASLMODE_MIXED;
            }
        }
        else {

            switch (pLingCmnInfo->Private.eSelectionListCorrectionMode)
            {
                case ET9ASLCORRECTIONMODE_LOW:
                    pLingCmnInfo->Private.eCurrSelectionListMode = ET9ASLMODE_COMPLETIONSPROMOTED;
                    break;
                case ET9ASLCORRECTIONMODE_HIGH:
                case ET9ASLCORRECTIONMODE_MEDIUM:
                    if (bHasRegionalInfo) {
                        switch (pLingCmnInfo->Private.eAltMode)
                        {
                            case ET9AW_AltMode_1:
                                pLingCmnInfo->Private.eCurrSelectionListMode = ET9ASLMODE_MIXED;
                                break;
                            case ET9AW_AltMode_2:
                                pLingCmnInfo->Private.eCurrSelectionListMode = ET9ASLMODE_CLASSIC;
                                break;
                            default:
                                ET9AssertLog(0);
                                pLingCmnInfo->Private.eCurrSelectionListMode = ET9ASLMODE_MIXED;
                                break;
                        }
                    }
                    else {
                        pLingCmnInfo->Private.eCurrSelectionListMode = ET9ASLMODE_CLASSIC;
                    }
                    break;
                default:
                    ET9AssertLog(0);
                    pLingCmnInfo->Private.eCurrSelectionListMode = ET9ASLMODE_CLASSIC;
                    break;
            }
        }
    }
    else {
        pLingCmnInfo->Private.eCurrSelectionListMode = pLingCmnInfo->Private.eSelectionListMode;
    }

    WLOG2(fprintf(pLogFile2, "base mode, eCurrSelectionListMode = %s\n", SELLSTMODETOSTRING(pLingCmnInfo->Private.eCurrSelectionListMode));)

    /* potentially override current selection list mode */

    if (bHasTraceInfo) {
        WLOG2(fprintf(pLogFile2, "input has trace info -> prevent further overrides\n");)
    }
    else if (bLastInputIsSmartPunct) {
        WLOG2(fprintf(pLogFile2, "last input SMARTPUNCT -> ET9ASLMODE_CLASSIC\n");)
        pLingCmnInfo->Private.eCurrSelectionListMode = ET9ASLMODE_CLASSIC;
    }
    else if (bLastInputIsMultitap) {
        WLOG2(fprintf(pLogFile2, "last input multitap -> ET9ASLMODE_COMPLETIONSPROMOTED\n");)
        pLingCmnInfo->Private.eCurrSelectionListMode = ET9ASLMODE_COMPLETIONSPROMOTED;
    }
    else if (pLingCmnInfo->wSecondLdbNum && ((pLingCmnInfo->wSecondLdbNum & ET9PLIDMASK) != ET9PLIDNone)) {
        WLOG2(fprintf(pLogFile2, "bilingual -> ET9ASLMODE_COMPLETIONSPROMOTED\n");)
        pLingCmnInfo->Private.eCurrSelectionListMode = ET9ASLMODE_COMPLETIONSPROMOTED;
    }

    if (bNumSymbs == 1 && pLingCmnInfo->Private.eCurrSelectionListMode == ET9ASLMODE_MIXED) {
        WLOG2(fprintf(pLogFile2, "input length one, ET9ASLMODE_MIXED -> ET9ASLMODE_CLASSIC\n");)
        pLingCmnInfo->Private.eCurrSelectionListMode = ET9ASLMODE_CLASSIC;
    }

    /* save some useful values for use by other functions */

    pLingCmnInfo->Private.bTraceBuild = bHasTraceInfo;
    pLingCmnInfo->Private.bHasAllShiftedInfo = bHasAllShiftedInfo;

    /* potentially override some system settings */

    if (bHasTraceInfo) {

        const ET9U16 wMinSpcTermCount = pLingCmnInfo->Private.bListSize;
        const ET9U16 wMinSpcCmplCount = __ET9Max(5, pLingCmnInfo->Private.wMaxCompletionCount);
        const ET9U16 wMinCompletionCount = 5;
        const ET9U16 wMaxWordCompletionPoint = 2;

        if (pLingCmnInfo->Private.wWordCompletionPoint > wMaxWordCompletionPoint) {
            pLingCmnInfo->Private.wWordCompletionPoint = wMaxWordCompletionPoint;
        }

        if (pLingCmnInfo->Private.wMaxCompletionCount < wMinCompletionCount) {
            pLingCmnInfo->Private.wMaxCompletionCount = wMinCompletionCount;
        }

        if (pLingCmnInfo->Private.ASpc.wMaxSpcTermCount < wMinSpcTermCount) {
            pLingCmnInfo->Private.ASpc.wMaxSpcTermCount = wMinSpcTermCount;
        }

        if (pLingCmnInfo->Private.ASpc.wMaxSpcCmplCount < wMinSpcCmplCount) {
            pLingCmnInfo->Private.ASpc.wMaxSpcCmplCount = wMinSpcCmplCount;
        }

        pLingCmnInfo->Private.bStateWordCompletion = 1;

        pLingCmnInfo->Private.ASpc.bSpcFeatures = ET9_FLEX_FEATURE_ALL_MASK;

        /* spell correction mode is handled above, modifying bSpcMode */
    }
    else if (bSpcMode && bNumSymbs == 2 && _ET9_LanguageSpecific_ApplySpcLenTwo(pLingInfo)) {

        /* use flex to get limited spell correction at length 2 for tap */

        if (!pLingCmnInfo->Private.ASpc.bSpcFeatures) {
            pLingCmnInfo->Private.ASpc.bSpcFeatures = ET9_FLEX_FEATURE_ACTIVATE_MASK | ET9_FLEX_FEATURE_FREE_PUNCT_MASK;
        }
    }

    /* if looking for the required word, make sure the last known shift state matches */

    if (pWordSymbInfo->Private.bRequiredLocate && bNumSymbs) {

        if (pWordSymbInfo->Private.eLastShiftState != pWordSymbInfo->Private.eRequiredLastShiftState) {
            WLOG2(fprintf(pLogFile2, "required's last shift state applied (old = %d)\n", pWordSymbInfo->Private.eLastShiftState);)
        }

        pWordSymbInfo->Private.eLastShiftState = pWordSymbInfo->Private.eRequiredLastShiftState;
    }

    WLOG2(fprintf(pLogFile2, "bLastShiftState = %d\n", pWordSymbInfo->Private.eLastShiftState);)

    WLOG2(fprintf(pLogFile2, "\n");)

    /* init result values */

    *pbTotalWords = 0;
    pLingCmnInfo->Private.bHasRealWord = 0;
    pLingCmnInfo->Private.bRequiredFound = 0;
    pLingCmnInfo->Private.snLinSearchCount = 0;
    pLingCmnInfo->Private.wTotalWordInserts = 0;
    pLingCmnInfo->Private.bSpcComplDuringSingleBuild = 0;
    pLingCmnInfo->Private.bCurrBuildHasShiftSignificance = 0;

    /* init left hand word */

    _InitPrivWordInfo(pLeftHandWord);

    /* verify some essentials */

    if (!bNumSymbs && !ET9NEXTWORDPREDICTION_MODE(pLingCmnInfo)) {
        WLOG2(fprintf(pLogFile2, "no symbs and no prediction - aborting\n");)
        _ET9AWSelLstResetWordList(pLingInfo);
        __RestoreSelectionListOverrideValues(pLingCmnInfo);
        return ET9STATUS_NONE;
    }
    if (bNumSymbs > ET9MAXWORDSIZE) {
        WLOG2(fprintf(pLogFile2, "too many symbs - aborting\n");)
        _ET9AWSelLstResetWordList(pLingInfo);
        __RestoreSelectionListOverrideValues(pLingCmnInfo);
        return ET9STATUS_ERROR;
    }

    /* verify that a reselected word is compatible with current setting for spell correction */

    if (pWordSymbInfo->Private.bRequiredVerifyInput && bNumSymbs) {

        if (pWordSymbInfo->Private.sRequiredWord.wLen < bNumSymbs && !ET9_SPC_IS_ACTIVE(bSpcMode)) {
            WLOG2(fprintf(pLogFile2, "aborting reselect build, spc mode not compatible (off)\n");)
            __RestoreSelectionListOverrideValues(pLingCmnInfo);
            return ET9STATUS_NONE;
        }
    }

    /* reset word list? */

    if (!bLastBuildLen) {
        _ET9AWSelLstResetWordList(pLingInfo);
    }

    /* a list build restores default post shift mode */

    pWordSymbInfo->Private.eCurrPostShiftMode = ET9POSTSHIFTMODE_DEFAULT;

    /* set build direction */

    if (!bNumSymbs) {

        pLingCmnInfo->Private.bLastBuildShrinking = 0;

        WLOG2(fprintf(pLogFile2, "bLastBuildShrinking = %d (empty input)\n", pLingCmnInfo->Private.bLastBuildShrinking);)
    }
    else if (bNumSymbs != bLastBuildLen) {

        pLingCmnInfo->Private.bLastBuildShrinking = (ET9BOOL)(bLastBuildLen > bNumSymbs);

        WLOG2(fprintf(pLogFile2, "bLastBuildShrinking = %d (%u > %u)\n", pLingCmnInfo->Private.bLastBuildShrinking, bLastBuildLen, bNumSymbs);)

        if (pLingCmnInfo->Private.bLastBuildShrinking) {

            ET9U8 bShrinkLetterCount;

            for (bShrinkLetterCount = bLastBuildLen; bShrinkLetterCount > bNumSymbs; --bShrinkLetterCount) {
                pbLangSupported[bShrinkLetterCount - 1] = ET9AWUNKNOWN_LANGUAGE;
            }
        }
    }
    else {
        WLOG2(fprintf(pLogFile2, "bLastBuildShrinking = %d (not modified)\n", pLingCmnInfo->Private.bLastBuildShrinking);)
    }

    /* handle (turn off) symbol invalidation flag */

    ET9AssertLog(
        /* empty */
        !bNumSymbs ||

        /* same length one is ok any way */
        bNumSymbs == 1 && bLastBuildLen == 1 ||

        /* growing and invalidated (or locked) */
        bNumSymbs > bLastBuildLen &&
        (pLingCmnInfo->Base.bSymbInvalidated[bNumSymbs-1] ||
        1 /* complicated to keep track of when a locked removed the invalidation */) ||

        /* shrinking or same and NOT invalidated */
        bNumSymbs <= bLastBuildLen &&
        !pLingCmnInfo->Base.bSymbInvalidated[bNumSymbs-1]
        );

    if (bNumSymbs) {
        pLingCmnInfo->Base.bSymbInvalidated[bNumSymbs-1] = 0;
    }

    /* find the latest flush point, and lockpoint */
    /* if clearing syms, save lockpoint for comparison */

    if (bNumSymbs < bLastBuildLen && *pwLockPoint) {
        wSavedLockPoint = *pwLockPoint;
    }
    *pwLockPoint = 0;
    bExactLock = 0;

    i = bNumSymbs;
    if (i) {
        for (pSymbInfo = &pWordSymbInfo->SymbsInfo[i - 1]; i; --i, --pSymbInfo) {
            if (pSymbInfo->bLocked) {
                *pwLockPoint = i;
                break;
            }
        }

        /* if the lock point (latest only) is exact lock, then "lock" the exact (default index to the exact) */

        if (*pwLockPoint && pSymbInfo->bLocked == ET9EXACTLOCK) {
            bExactLock = 1;
        }
    }

    /* if clearing sym caused loss of lockpoint (or we hit a flush point ) */
    /* then we want to deactivate downshifting that was in force for previous righthand word */

    if (!bNumSymbs ||
        (bNumSymbs < bLastBuildLen && ((*pwLockPoint != wSavedLockPoint) ||
         pLingCmnInfo->Private.sBuildInfo.pbFlushPos[bNumSymbs-1]))) {
        pWordSymbInfo->Private.bCompoundingDownshift = 0;
    }

    wFlushPoint = __CaptureGetFlushPoint(pLingInfo, 0);

    WLOG2(fprintf(pLogFile2, "FlushPoint = %d, LockPoint = %d, ExactLock = %d\n", wFlushPoint, *pwLockPoint, bExactLock);)

    /* Record the default object.
       This is designed to catch the previously highlighted word.
       If there was lock, that will be the default. */

    bDefaultOkay = 0;

    _InitPrivWordInfo(pPrevDefaultWord);

    if (bNumSymbs == 1 && !pLingCmnInfo->Private.bTotalSymbInputs) {

        /* make the default exist in this first build of first symb (empty) */

        bDefaultOkay = 1;

        WLOG2Word(pLogFile2, "default empty on first build of first symb", pPrevDefaultWord);
    }
    else if (bNumSymbs && pLingCmnInfo->Private.bTotalSymbInputs) {

        /* first check to see of there was a lock, effectively overriding any default index. */

        if (bNumSymbs && pWordSymbInfo->SymbsInfo[bNumSymbs - 1].bLocked ||
            bNumSymbs > 1 && pWordSymbInfo->SymbsInfo[bNumSymbs - 2].bLocked) {

            ET9U16      wCount;
            ET9SYMB     *psSymb = pPrevDefaultWord->Base.sWord;
            ET9SymbInfo *pSymbInfo = pWordSymbInfo->SymbsInfo;

            for (wCount = (ET9U16)(bNumSymbs - 1); wCount; --wCount, ++psSymb, ++pSymbInfo) {
                *psSymb = pSymbInfo->sLockedSymb;
            }

            pPrevDefaultWord->Base.wWordLen = (ET9U16)(bNumSymbs - 1);

            if (bNumSymbs > 1) {
                pPrevDefaultWord->Base.bLangIndex =  pWordSymbInfo->SymbsInfo[bNumSymbs - 2].bLockLanguageSource;
            }
            else {
                pPrevDefaultWord->Base.bLangIndex =  pWordSymbInfo->SymbsInfo[bNumSymbs - 1].bLockLanguageSource;
            }

            __CaptureDefault(pLingInfo, pPrevDefaultWord, bNumSymbs, 0);

            /* should we assign a word source here? */

            bDefaultOkay = 1;

            WLOG2Word(pLogFile2, "default from lock", pPrevDefaultWord);
        }
        else if (bLastBuildLen >= bNumSymbs &&
                 __CaptureGetDefault(pLingInfo, pPrevDefaultWord, bNumSymbs)) {

            bDefaultOkay = 1;

            if (wFlushPoint) {
                pPrevDefaultWord->Base.bLangIndex = pLingCmnInfo->Private.sBuildInfo.bLanguageSource[wFlushPoint];
            }
            else if (pLingInfo->pLingCmnInfo->Private.bDefaultIndex < pLingInfo->pLingCmnInfo->Private.bTotalWords) {
                pPrevDefaultWord->Base.bLangIndex = pLingCmnInfo->Private.pWordList[pLingInfo->pLingCmnInfo->Private.bWordList[pLingInfo->pLingCmnInfo->Private.bDefaultIndex]].Base.bLangIndex;
            }

            WLOG2Word(pLogFile2, "default from capture", pPrevDefaultWord);
        }
        else {

            if (bLastBuildLen >= bNumSymbs) {
                pWord = NULL;
            }
            else if (pLingCmnInfo->Private.bTotalWords && pLingCmnInfo->Private.bDefaultIndex < pLingCmnInfo->Private.bTotalWords) {

                ET9U8 bIndex = pLingCmnInfo->Private.bDefaultIndex;

                for (pWord = NULL; !pWord && bIndex < pLingCmnInfo->Private.bTotalWords; ++bIndex) {

                    pWord = &pLingCmnInfo->Private.pWordList[pLingCmnInfo->Private.bWordList[bIndex]];

                    if (pWord->bWordDesignation == FREQ_BUILDSPACE) {
                        pWord = NULL;
                    }
                }
            }
            else {
                pWord = NULL;
            }

            if (pWord && pWord->bWordSrc != ET9WORDSRC_MAGICSTRING) {

                *pPrevDefaultWord = *pWord;

                bDefaultOkay = 1;

                ET9AssertLog(pPrevDefaultWord->Base.wWordLen);

                pPrevDefaultWord->Base.bLangIndex = pWord->Base.bLangIndex;

                WLOG2(fprintf(pLogFile2, "default index (%d)\n", pLingCmnInfo->Private.bDefaultIndex);)
                WLOG2Word(pLogFile2, "default from default index", pPrevDefaultWord);
            }
            else {

                _InitPrivWordInfo(pPrevDefaultWord);

                if (!ET9GetExactWord(pWordSymbInfo, &sSimpleWord, pLingInfo->Private.pConvertSymb, pLingInfo->Private.pConvertSymbInfo)) {

                    _ET9SimpleWordToPrivWord(&sSimpleWord, pPrevDefaultWord);

                    if (pPrevDefaultWord->Base.wWordLen > bNumSymbs - 1) {
                        pPrevDefaultWord->Base.wWordLen = (ET9U16)(bNumSymbs - 1);
                    }
                    else if (pPrevDefaultWord->Base.wWordLen) {
                        --pPrevDefaultWord->Base.wWordLen;
                    }
                    pPrevDefaultWord->Base.wWordCompLen = 0;
                    bDefaultOkay = 1;

                    WLOG2Word(pLogFile2, "default from exact", pPrevDefaultWord);
                }
                else {

                    WLOG2(fprintf(pLogFile2, "failed to get exact for default use\n");)

                    /* no support for default by question marks... */
                }
            }

            /* make sure the flush part is intact */

            if (bDefaultOkay) {

                ET9AWPrivWordInfo sFlush;

                __CaptureGetFlush(pLingInfo, &sFlush);

                if (sFlush.Base.wWordLen) {

                    _ET9SymCopy(pPrevDefaultWord->Base.sWord, sFlush.Base.sWord, sFlush.Base.wWordLen);

                    if (sFlush.Base.wWordLen > pPrevDefaultWord->Base.wWordLen) {
                        pPrevDefaultWord->Base.wWordLen = sFlush.Base.wWordLen;
                        pPrevDefaultWord->Base.wWordCompLen = 0;
                    }
                }
            }

            /* make sure the locked part is intact */

            __AssureLockedString(pLingInfo, pPrevDefaultWord, 0);
        }
    }
    /* END: Record the default object.*/

    if (!bDefaultOkay) {
        pPrevDefaultWord->Base.wWordLen = 0;
    }

    WLOG2(fprintf(pLogFile2, "DefaultOkay = %d\n", bDefaultOkay);)
    WLOG2Word(pLogFile2, "PrevDefaultWord", pPrevDefaultWord);

    if (bDefaultOkay) {
        __VerifyLockedSymbs(pLingInfo, pPrevDefaultWord, 0, bNumSymbs);
    }

    if (bNumSymbs > bLastBuildLen) {

        /* this assert is hard to apply when allowing completions for term punct... */

        ET9AssertLog(bHasTraceInfo ||
                     !bDefaultOkay ||
                     (pPrevDefaultWord->Base.wWordLen - pPrevDefaultWord->Base.wWordCompLen) == bNumSymbs - 1 ||
                     pLingCmnInfo->Private.bSpcDuringBuild ||
                     pLingCmnInfo->Private.bExpandAsDuringBuild);
    }

    /* handle capture actions (especially when rewinding) */

    __CaptureHandleAction(pLingInfo);

    __PreCaptureWord(pLingInfo);

    __CaptureDefault(pLingInfo, pPrevDefaultWord, bNumSymbs, bLastBuildLen);

    /* handle (new) word capture
       Here we are capturing the strings before and including special characters.
       Note that what is captured may not be the same length as the number of keys
       presses because of spell correction. */

    if (bDefaultOkay) {

        if (bNumSymbs > bLastBuildLen) {

            ET9U16 wExcludePoint;

            wExcludePoint = bNumSymbs;

            if (wExcludePoint) {
                --wExcludePoint;
                if (wExcludePoint) {
                    --wExcludePoint;
                }
            }

            /* nothing can be captured unless there is a special tail (non nospecial) */

            __ET9GetSpecialCharInfo(pLingInfo, wExcludePoint, &wSpecialPosition, &eSpecialType);

            if (eSpecialType != NOSPECIAL) {

                const ET9U16        wFlushStringLen = __CaptureGetFlushStringLength(pLingInfo, 1);
                ET9ASPECIAL         eSpecialTypeDummy;
                ET9AWPrivWordInfo   *pCaptureWord = NULL;

                WLOG2(fprintf(pLogFile2, "symbol tail indicates possible capture\n");)

                /* check if actually accepting completions */

                __ET9GetSpecialCharInfo(pLingInfo, 0, &wSpecialPosition, &eSpecialTypeDummy);

                if (bNumSymbs && wSpecialPosition == (bNumSymbs - 1)) { /* bNumSymbs zero would be ok anyway... */

                    WLOG2(fprintf(pLogFile2, "wSpecialPosition indicates capture completion prevention\n");)

                    if (!pPrevDefaultWord->Base.wWordCompLen &&
                        ISCAPTURECANDSRC(pPrevDefaultWord->bWordSrc) &&
                        pPrevDefaultWord->Base.wWordLen > wFlushStringLen) {

                        pCaptureWord = pPrevDefaultWord;
                    }
                    else {

                        ET9U16 wStartIndex = pLingCmnInfo->Private.bDefaultIndex;

                        if (wStartIndex && ISREALSRC(pLingCmnInfo->Private.pWordList[pLingCmnInfo->Private.bWordList[0]].bWordSrc)) {

                            wStartIndex = 0;
                        }

                        for (i = wStartIndex; i < pLingCmnInfo->Private.bTotalWords; ++i) {

                            pWord = &pLingCmnInfo->Private.pWordList[pLingCmnInfo->Private.bWordList[i]];

                            if (!pWord->Base.wWordCompLen &&
                                ISCAPTURECANDSRC(pWord->bWordSrc) &&
                                (pWord->Base.wWordLen > wFlushStringLen) &&
                                ISREALSRC(pWord->bWordSrc)) {

                                pCaptureWord = pWord;
                                break;
                            }
                        }

                        if (pCaptureWord == NULL) {

                            for (i = wStartIndex; i < pLingCmnInfo->Private.bTotalWords; ++i) {

                                pWord = &pLingCmnInfo->Private.pWordList[pLingCmnInfo->Private.bWordList[i]];

                                if (!pWord->Base.wWordCompLen &&
                                    ISALTCAPTURECANDSRC(pWord->bWordSrc) &&
                                    (pWord->Base.wWordLen > wFlushStringLen) &&
                                    GETBASESRC(pWord->bWordSrc) != ET9WORDSRC_NONE) {

                                    pCaptureWord = pWord;
                                    break;
                                }
                            }

                        }
                    }

                    if (pCaptureWord) {
                        __CaptureWord(pLingInfo, pCaptureWord, (ET9U16)(bNumSymbs - 1));
                    }
                    else if (bHasTraceInfo) {
                        __CaptureWord(pLingInfo, pPrevDefaultWord, (ET9U16)(bNumSymbs - 1));
                    }

                }
                else {

                    WLOG2(fprintf(pLogFile2, "wSpecialPosition indicates capture completions allowed - using default\n");)

                    if (pPrevDefaultWord->Base.wWordLen <= wFlushStringLen) {

                        /* should we do a more elaborate search here instead? */

                        WLOG2(fprintf(pLogFile2, "default is too short for capturing...\n");)
                    }
                    else {
                        __CaptureWord(pLingInfo, pPrevDefaultWord, (ET9U16)(bNumSymbs - 1));
                    }
                }
            }
        }
    }

    /* now we can reset the list */

    _ET9AWSelLstResetWordList(pLingInfo);

    /* set up Selection list information */

    pLingCmnInfo->Private.bTotalSymbInputs = bNumSymbs;

    /* at this point we can potentially abort when the build is suppressed */

    if (bSuppressBuild) {

        /* some things that wasn't covered yet */

        pbLangSupported[bNumSymbs - 1] = ET9AWBOTH_LANGUAGES;

        /* abort */

        WLOG2(fprintf(pLogFile2, "aborting build, suppressed\n");)
        __RestoreSelectionListOverrideValues(pLingCmnInfo);
        return ET9STATUS_NONE;
    }

    /* prepare matching info */

    __MakeSymbFreqsValid(pLingCmnInfo);

    /* handle magic string */

    if (pWordSymbInfo->bNumSymbs == ET9MAXLDBWORDSIZE) {
        if (_ET9IsMagicStringKey(pWordSymbInfo)) {
            __ET9AWAddMagicStr(pLingInfo);
        }
    }

    /* add exact match to list */

    pLingCmnInfo->Private.bExactIndex = ET9_NO_ACTIVE_INDEX;

    if (bNumSymbs && (bExactInList || !bDefaultOkay)) {

        ET9AWPrivWordInfo sExactWord;

        _InitPrivWordInfo(&sExactWord);

        if (!ET9GetExactWord(pWordSymbInfo, &sSimpleWord, pLingInfo->Private.pConvertSymb, pLingInfo->Private.pConvertSymbInfo)) {

            _ET9SimpleWordToPrivWord(&sSimpleWord, &sExactWord);

            sExactWord.bWordSrc = ET9WORDSRC_NONE | EXACTOFFSET;
            sExactWord.xTotFreq = 0xFFFF;
            sExactWord.xTapFreq = 0xFFFF;
            sExactWord.xWordFreq = 1;
            sExactWord.wTWordFreq = 0;
            sExactWord.wEWordFreq = 0;
            sExactWord.Base.bIsTerm = 1;

            if (pLingCmnInfo->Private.wCurrLockPoint && pLingCmnInfo->Private.bLastBuildLen >= pLingCmnInfo->Private.wCurrLockPoint) {
                sExactWord.Base.bLangIndex = pWordSymbInfo->SymbsInfo[pLingCmnInfo->Private.wCurrLockPoint - 1].bLockLanguageSource;
            }
            else if (pbLangSupported[bNumSymbs - 1]) {
                sExactWord.Base.bLangIndex = pbLangSupported[bNumSymbs - 1];
            }
            else if (ET9AWSys_GetBilingualSupported(pLingInfo)) {
                sExactWord.Base.bLangIndex = ET9AWBOTH_LANGUAGES;
            }
            else {
                sExactWord.Base.bLangIndex = ET9AWFIRST_LANGUAGE;
            }

            WLOG2Word(pLogFile2, "ExactWord", &sExactWord);

            if (bExactInList) {
                pLingCmnInfo->Private.bExactIndex = 0;
                pLingCmnInfo->Private.bStemsAllowed = 1;
                pLingCmnInfo->Private.wMaxWordLength = ET9MAXWORDSIZE;
                __ET9AWSelLstInsert(pLingInfo, &sExactWord, FREQ_NORMAL, 0);
            }

            if (!bDefaultOkay) {

                /* this still happens... */

                *pPrevDefaultWord = sExactWord;

                if (pPrevDefaultWord->Base.wWordLen) {
                    --pPrevDefaultWord->Base.wWordLen;
                }

                WLOG2Word(pLogFile2, "PrevDefaultWord (fail-safe from exact)", pPrevDefaultWord);

                __CaptureDefault(pLingInfo, pPrevDefaultWord, bNumSymbs, bLastBuildLen);
            }
        }
    }

    /* store start value for tracking full word matches (after exact, before required) */

    wFullWordMatches = pLingCmnInfo->Private.wTotalWordInserts;

    /* add required word to the list, when applicable */

    if (bNumSymbs && !wFlushPoint && pWordSymbInfo->Private.sRequiredWord.wLen == bNumSymbs) {

        ET9AWPrivWordInfo   sLocalWord;

        _ET9SimpleWordToPrivWord(&pWordSymbInfo->Private.sRequiredWord, &sLocalWord);

        sLocalWord.bWordSrc = ET9WORDSRC_REQUIRED;

        WLOG2Word(pLogFile2, "RequiredWord", &sLocalWord);

        pLingCmnInfo->Private.bStemsAllowed = __GetStemsAllowed(pLingCmnInfo, bNumSymbs);
        pLingCmnInfo->Private.wMaxWordLength = __GetMaxWordLength(pLingCmnInfo, bNumSymbs);

        __SelLstWordSearch(pLingInfo, &sLocalWord, wFlushPoint, bNumSymbs, FREQ_NORMAL, bSpcMode);
    }

    /* at this point the default word has a value if it ever gets one... */

    __CaptureUpdateFlushInfo(pLingInfo, pPrevDefaultWord);

    /* setup left hand word */

    if (wFlushPoint) {

        __CaptureGetFlush(pLingInfo, pLeftHandWord);
        __UpdateSpcInfo(pLingInfo, pLeftHandWord, 0, wFlushPoint, bSpcMode);

    }
    else if (bNumSymbs > 1) {
        pLeftHandWord->Base.bLangIndex = pPrevDefaultWord->Base.bLangIndex;
    }
    else if (ET9AWSys_GetBilingualSupported(pLingInfo)) {
        pLeftHandWord->Base.bLangIndex = ET9AWBOTH_LANGUAGES;
    }
    else {
        pLeftHandWord->Base.bLangIndex = ET9AWFIRST_LANGUAGE;
    }

    WLOG2Word(pLogFile2, "LeftHandWord", pLeftHandWord);

    ET9AssertLog(pLeftHandWord->Base.wWordCompLen <= pLeftHandWord->Base.wWordLen);

    /* do full builds */

    {
        const ET9U16 wWordLen = (ET9U16)(bNumSymbs - wFlushPoint);

        ET9U8   bLdbEntries;
        ET9U8   bSuppEntries;
        ET9U8   bSuppASEntries;

        bLdbEntries = 0;
        bSuppEntries = 0;
        bSuppASEntries = 0;

        pLingCmnInfo->Private.bStemsAllowed = __GetStemsAllowed(pLingCmnInfo, wWordLen);
        pLingCmnInfo->Private.wMaxWordLength = __GetMaxWordLength(pLingCmnInfo, wWordLen);

        WLOG2(fprintf(pLogFile2, "performing _ET9AWSuppDBSelListBuild (non AS, after flush point), index = %d, length = %d\n", wFlushPoint, bNumSymbs - wFlushPoint);)

        _ET9AWSuppDBSelListBuild(pLingInfo,
                                 wFlushPoint,
                                 wWordLen,
                                 &bSuppEntries,
                                 FREQ_NORMAL,
                                 ET9SUPPDB_NONAS_SOURCES,
                                 bSpcMode);

        WLOG2(fprintf(pLogFile2, "performing _ET9AWLdbWordsSearch (after flush point), index = %d, length = %d\n", wFlushPoint, bNumSymbs - wFlushPoint);)

        WLOG2(fprintf(pLogFile2, "wFirstLdbNum %u, bLangIndex %u, bLastBuildShrinking %u, bLangSupported %u\n",
                                 pLingCmnInfo->wFirstLdbNum,
                                 pLeftHandWord->Base.bLangIndex,
                                 pLingCmnInfo->Private.bLastBuildShrinking,
                                 pbLangSupported[bNumSymbs - 1]);)

        if (((pLingCmnInfo->wFirstLdbNum & ET9PLIDMASK) != ET9PLIDNone)  &&
            !(wFlushPoint && (pLeftHandWord->Base.bLangIndex == ET9AWSECOND_LANGUAGE)) &&
            !(pLingCmnInfo->Private.bLastBuildShrinking &&
              !((pbLangSupported[bNumSymbs - 1] == ET9AWFIRST_LANGUAGE) ||
                (pbLangSupported[bNumSymbs - 1] == ET9AWBOTH_LANGUAGES)))) {

            ET9U8 bSavedSpcMode = bSpcMode;

            if (!ET9INACTIVELANGSPELLCORRECTENABLED(pLingCmnInfo) &&
                pLingCmnInfo->wLdbNum != pLingCmnInfo->wFirstLdbNum) {
                bSavedSpcMode = ET9ASPCMODE_OFF;
            }

            _ET9AWLdbWordsSearch(pLingInfo,
                                 pLingCmnInfo->wFirstLdbNum,
                                 wFlushPoint,
                                 wWordLen,
                                 &bLdbEntries,
                                 FREQ_NORMAL,
                                 bSavedSpcMode);
        }
        else {
            WLOG2(fprintf(pLogFile2, "-- LDB skipped because of conditions\n");)
        }

        if (ET9AWSys_GetBilingualSupported(pLingInfo) &&
            !(wFlushPoint && (pLeftHandWord->Base.bLangIndex == ET9AWFIRST_LANGUAGE)) &&
            !(pLingCmnInfo->Private.bLastBuildShrinking &&
              !((pbLangSupported[bNumSymbs - 1] == ET9AWSECOND_LANGUAGE) ||
                (pbLangSupported[bNumSymbs - 1] == ET9AWBOTH_LANGUAGES)))) {

            ET9U8 bSavedSpcMode = bSpcMode;

            if (!ET9INACTIVELANGSPELLCORRECTENABLED(pLingCmnInfo) &&
                pLingCmnInfo->wLdbNum != pLingCmnInfo->wSecondLdbNum) {
                bSavedSpcMode = ET9ASPCMODE_OFF;
            }

            _ET9AWLdbWordsSearch(pLingInfo,
                                 pLingCmnInfo->wSecondLdbNum,
                                 wFlushPoint,
                                 wWordLen,
                                 &bLdbEntries,
                                 FREQ_NORMAL,
                                 bSavedSpcMode);
        }

        WLOG2(fprintf(pLogFile2, "performing _ET9AWSuppDBSelListBuild (AS, after flush point), index = %d, length = %d\n", wFlushPoint, bNumSymbs - wFlushPoint);)

        _ET9AWSuppDBSelListBuild(pLingInfo,
                                 wFlushPoint,
                                 wWordLen,
                                 &bSuppASEntries,
                                 FREQ_NORMAL,
                                 ET9SUPPDB_AS_SOURCES,
                                 bSpcMode);

        WLOG2(fprintf(pLogFile2, "full build results, bSuppEntries = %d, bSuppASEntries = %d, bLdbEntries = %d, bTotalWords = %d\n", (int)bSuppEntries, (int)bSuppASEntries, (int)bLdbEntries, (int)pLingCmnInfo->Private.bTotalWords);)

        /* do post full build processing */

        if (bSuppASEntries) {
            __DoBuildSubstitutionProcessing(pLingInfo);
        }
    }

    /* calculate full matches */

    wFullWordMatches = pLingCmnInfo->Private.wTotalWordInserts - wFullWordMatches;

    ET9AssertLog(!bNumSymbs || !bExactInList || pLingCmnInfo->Private.bTotalWords);

    /* find right most special (punct or explicit) char in word */

    bHasTermPunct = 0;
    bHasTermSmartPunct = 0;
    wSpecialPosition = 0;
    eSpecialType = NOSPECIAL;

    if (!bNumSymbs) {
        bContinue = 0;
    }
    else {
        bContinue = 1;

        pSymbInfo = &pWordSymbInfo->SymbsInfo[bNumSymbs - 1];

        __ET9GetSpecialCharInfo(pLingInfo, wFlushPoint, &wSpecialPosition, &eSpecialType);

        if (wSpecialPosition == bNumSymbs - wFlushPoint) {
            if (eSpecialType == SMARTPUNCT) {
                bHasTermSmartPunct = 1;
            }
            else if (eSpecialType == PUNCT) {
                bHasTermPunct = 1;
            }
            else if (__IsExactSymb(pSymbInfo) &&
                     _ET9_IsPunctChar(pSymbInfo->DataPerBaseSym[0].sChar[0])) {

                bHasTermPunct = 1;
            }
        }
    }

    WLOG2(fprintf(pLogFile2, "bHasTermPunct %u, bHasTermSmartPunct %u, wSpecialPosition %u\n", bHasTermPunct, bHasTermSmartPunct, wSpecialPosition);)

    /* check for buildarounds */

    /* -------------------- Rightmost punct or explicit --------------------------*/

    if ((eSpecialType != NOSPECIAL) && !bHasTermSmartPunct && !bHasTermPunct && !(bHasTraceInfo && pWordSymbInfo->Private.sRequiredWord.wLen && pWordSymbInfo->bNumSymbs)) {

        /* trailing special */

        if ((wSpecialPosition > 1 && wSpecialPosition < bNumSymbs - wFlushPoint) || eSpecialType == LOCKPOINT) {

            WLOG2(fprintf(pLogFile2, "performing trailing BuildAroundSpecial\n");)

            if (__ET9BuildAroundSpecial(pLingInfo,
                                        wFlushPoint,
                                        wSpecialPosition,
                                        eSpecialType,
                                        pLeftHandWord,
                                        pPrevDefaultWord,
                                        (eSpecialType == LOCKPOINT ? BA_COMPOUND : BA_TRAILING),
                                        0 /* embedded char*/,
                                        bSpcMode)) {

                WLOG2(fprintf(pLogFile2, "added trailing special\n");)

                bContinue = 0;

                /* update flush information */

                if (!wFullWordMatches && pLeftHandWord->Base.wWordLen) {

                    ET9U16 wFlushSymbolLen = wSpecialPosition + wFlushPoint;

                    /* term punct completion problem... */

                    ET9AssertLog(pLeftHandWord->Base.wWordLen == wFlushSymbolLen || pLingCmnInfo->Private.bSpcDuringBuild || bHasTraceInfo);

                    __CaptureFlushPoint(pLingInfo, wFlushSymbolLen, pLeftHandWord);

                    WLOG2(fprintf(pLogFile2, "new flush point set @ %d (trailing special)\n", wFlushSymbolLen);)
                }
                wFullWordMatches = pLingCmnInfo->Private.wTotalWordInserts;
            }
        }

        /* leading special */

        if (bContinue && (eSpecialType != LOCKPOINT) && (wSpecialPosition > 1) &&
            (wSpecialPosition < bNumSymbs - wFlushPoint))  {

            WLOG2(fprintf(pLogFile2, "performing leading BuildAroundSpecial\n");)

            if (__ET9BuildAroundSpecial(pLingInfo,
                                        wFlushPoint,
                                        wSpecialPosition,
                                        eSpecialType,
                                        pLeftHandWord,
                                        pPrevDefaultWord,
                                        BA_LEADING,
                                        0 /* embedded char*/,
                                        bSpcMode)) {

                WLOG2(fprintf(pLogFile2, "added leading special\n");)

                /* update flush information */

                /*
                   Note there is something subtle going on here.  If there was a successful
                   buildaround with trailing punct, there is a flush because there were no full matches,
                   we still want to do the embedded punct.  This is because after the flush, a build should
                   give embedded matches as well.  Also, if we do not do this, a subsequent build with the
                   SAME pWordSymbInfo would give a different list.
                */

                if (!wFullWordMatches) {

                    if (pLeftHandWord->Base.wWordLen) {

                        ET9U16 wFlushSymbolLen = wSpecialPosition - 1 + wFlushPoint;

                        /* term punct completion problem... */

                        ET9AssertLog(pLeftHandWord->Base.wWordLen == wFlushSymbolLen || pLingCmnInfo->Private.bSpcDuringBuild || bHasTraceInfo);

                        __CaptureFlushPoint(pLingInfo, wFlushSymbolLen, pLeftHandWord);

                        WLOG2(fprintf(pLogFile2, "new flush point set @ %d (leading special)\n", wFlushSymbolLen);)
                    }
                }
                else {
                    bContinue = 0;
                }
                wFullWordMatches = pLingCmnInfo->Private.wTotalWordInserts;
            }
        }

        /* embedded special */

        if (bContinue && eSpecialType != LOCKPOINT) {

            /* we only do embedded if this was an explicit, a single ET9KPUNCTUAITON, or an ET9KSMARTPUNCT. */

            ET9SYMB sEmbeddedSymb;
            pSymbInfo = &pWordSymbInfo->SymbsInfo[wFlushPoint + wSpecialPosition - 1];

            if (__IsExactSymb(pSymbInfo) ||
                pSymbInfo->bSymbType == ET9KTSMARTPUNCT ||
                (pSymbInfo->bSymbType == ET9KTPUNCTUATION &&
                 pSymbInfo->bNumBaseSyms == 1 &&
                 pSymbInfo->DataPerBaseSym[0].bNumSymsToMatch == 1)) {

                sEmbeddedSymb = 0x0000;

                if (*pwLockPoint >= wFlushPoint + wSpecialPosition) {

                    sEmbeddedSymb = pSymbInfo->sLockedSymb;

                    WLOG2(fprintf(pLogFile2, "embedded symb %c (from lock)\n", (char)sEmbeddedSymb);)
                }
                else if (pSymbInfo->bSymbType == ET9KTSMARTPUNCT) {

                    ET9U16 wTempLdbNum = pLingCmnInfo->wLdbNum;

                    if (wFlushPoint + wSpecialPosition - 1 > 0) {
                        if (pLingCmnInfo->Private.sBuildInfo.bLanguageSource[wFlushPoint + wSpecialPosition - 2] == ET9AWFIRST_LANGUAGE) {
                            wTempLdbNum = pLingInfo->pLingCmnInfo->wFirstLdbNum;
                        }
                        else if (pLingCmnInfo->Private.sBuildInfo.bLanguageSource[wFlushPoint + wSpecialPosition - 2] == ET9AWSECOND_LANGUAGE) {
                            wTempLdbNum = pLingInfo->pLingCmnInfo->wSecondLdbNum;
                        }
                    }

                    sEmbeddedSymb = _ET9_GetEmbPunctChar(pLingInfo, wTempLdbNum);

                    WLOG2(fprintf(pLogFile2, "embedded symb %c (from GetEmbPunctChar lang %04x)\n", (char)sEmbeddedSymb, wTempLdbNum);)
                }
                else {

                    if (pSymbInfo->eShiftState) {
                        sEmbeddedSymb = pSymbInfo->DataPerBaseSym[0].sUpperCaseChar[0];
                    }
                    else {
                        sEmbeddedSymb = pSymbInfo->DataPerBaseSym[0].sChar[0];
                    }

                    WLOG2(fprintf(pLogFile2, "embedded symb %c (from symbinfo)\n", (char)sEmbeddedSymb);)
                }

                ET9AssertLog(sEmbeddedSymb);

                WLOG2(fprintf(pLogFile2, "performing embedded BuildAroundSpecial\n");)

                if (__ET9BuildAroundSpecial(pLingInfo,
                                            wFlushPoint,
                                            wSpecialPosition,
                                            eSpecialType,
                                            pLeftHandWord,
                                            pPrevDefaultWord,
                                            BA_EMBEDDED,
                                            sEmbeddedSymb,
                                            bSpcMode)) {

                    WLOG2(fprintf(pLogFile2, "added embedded special, wFullWordMatches = %d, left wordLen = %d\n", wFullWordMatches, pLeftHandWord->Base.wWordLen);)

                    bContinue = 0;

                    /* update flush information */

                    if (!wFullWordMatches && pLeftHandWord->Base.wWordLen) {

                        ET9U16 wFlushSymbolLen = wSpecialPosition + wFlushPoint;

                        /* term punct completion problem... */

                        ET9AssertLog(pLeftHandWord->Base.wWordLen == wFlushSymbolLen || pLingCmnInfo->Private.bSpcDuringBuild || bHasTraceInfo);

                        __CaptureFlushPoint(pLingInfo, wFlushSymbolLen, pLeftHandWord);

                        WLOG2(fprintf(pLogFile2, "new flush point set @ %d (embedded special)\n", wFlushSymbolLen);)
                    }
                    wFullWordMatches = pLingCmnInfo->Private.wTotalWordInserts;
                }
            }
        }
    }

    /* -------------------- default WITHOUT completion + punct --------------------------*/

    if ((bHasTermPunct || bHasTermSmartPunct) && bDefaultOkay) {

        WLOG2(fprintf(pLogFile2, "performing a default (no completion) punct build\n");)

        if (bNumSymbs - wFlushPoint - 1 > 0) {

            /* get the portion of the word before the punctuation */

            ET9U16 wDefaultLen = __CaptureGetDefaultStringLength(pLingInfo, bNumSymbs);

            if (pPrevDefaultWord->Base.wWordLen < wDefaultLen) {
                WLOG2(fprintf(pLogFile2, "LeftHandWord, prev default shorter than expected (%d < %d)\n", pPrevDefaultWord->Base.wWordLen, wDefaultLen);)
            }

            *pLeftHandWord = *pPrevDefaultWord;

            if (pLeftHandWord->Base.wWordLen > wDefaultLen) {
                pLeftHandWord->Base.wWordLen = wDefaultLen;
            }

            pLeftHandWord->Base.wWordCompLen = 0;

            __UpdateSpcInfo(pLingInfo, pLeftHandWord, 0, (ET9U8)(bNumSymbs - 1), bSpcMode);

            WLOG2Word(pLogFile2, "LeftHandWord (from prev default)", pLeftHandWord);

            /* update flush information */

            if (!wFullWordMatches) {

                if (pLeftHandWord->Base.wWordLen) {

                    ET9U16 wFlushSymbolLen = bNumSymbs - 1;

                    __CaptureFlushPoint(pLingInfo, wFlushSymbolLen, pLeftHandWord);

                    WLOG2(fprintf(pLogFile2, "new flush point set @ %d (has non completion punct)\n", wFlushSymbolLen);)
                }
            }
        }

        /* now add the punct(s) to the list. */

        ET9AssertLog(!(bHasTermPunct && bHasTermSmartPunct));

        WLOG2(fprintf(pLogFile2, "adding punct candidates\n");)

        pLingCmnInfo->Private.bStemsAllowed = 1;
        pLingCmnInfo->Private.wMaxWordLength = ET9MAXWORDSIZE;

        /* Get termpuncts for respective language in unilingual mode.
           Get termpuncts for both languages in bilingual mode (primary followed by secondary) with 2 exceptions:
           (i) If lefthand word is a term in 1st language only, termpuncts from 1st language alone are included.
           (ii) If lefthand word is a term in 2nd language only, termpuncts from 2nd language alone are included. */

        if (pLeftHandWord->Base.bIsTerm && pLeftHandWord->Base.bLangIndex == ET9AWFIRST_LANGUAGE) {

            const ET9U8 bPunctSeqSize = (ET9U8)(bHasTermSmartPunct ? _ET9_GetNumTermPunct(pLingInfo, pLingCmnInfo->wFirstLdbNum) : 1);

            for (i = 0; i < bPunctSeqSize; ++i) {

                ET9AWPrivWordInfo sPunctWord;

                _InitPrivWordInfo(&sPunctWord);

                sPunctWord.xTapFreq = 1;
                sPunctWord.xWordFreq = (ET9FREQPART)(0xFFFE - (i * 1000));
                sPunctWord.Base.wWordLen = 1;
                sPunctWord.bWordSrc = ET9WORDSRC_TERMPUNCT;

                sPunctWord.Base.bLangIndex = pLeftHandWord->Base.bLangIndex;

                pbLangSupported[bNumSymbs - 1] = sPunctWord.Base.bLangIndex;

                if (bHasTermSmartPunct) {
                    sPunctWord.Base.sWord[0] = _ET9_GetTermPunctChar(pLingInfo, pLingCmnInfo->wFirstLdbNum, i);
                }
                else {
                    sPunctWord.Base.sWord[0] = pWordSymbInfo->SymbsInfo[bNumSymbs - 1].DataPerBaseSym[0].sChar[0];
                }

                ET9AssertLog(sPunctWord.Base.sWord[0]);

                WLOG2Word(pLogFile2, "PunctWord", &sPunctWord);

                __SelLstWordSearch(pLingInfo, &sPunctWord, (ET9U16)(bNumSymbs - 1), 1, FREQ_TERM_PUNCT, bSpcMode);
            }
        }
        else if (pLeftHandWord->Base.bIsTerm && pLeftHandWord->Base.bLangIndex == ET9AWSECOND_LANGUAGE) {

            const ET9U8 bPunctSeqSize = (ET9U8)(bHasTermSmartPunct ? _ET9_GetNumTermPunct(pLingInfo, pLingCmnInfo->wSecondLdbNum) : 1);

            for (i = 0; i < bPunctSeqSize; ++i) {

                ET9AWPrivWordInfo sPunctWord;

                _InitPrivWordInfo(&sPunctWord);

                sPunctWord.xTapFreq = 1;
                sPunctWord.xWordFreq = (ET9FREQPART)(0xFFFE - (i * 1000));
                sPunctWord.Base.wWordLen = 1;
                sPunctWord.bWordSrc = ET9WORDSRC_TERMPUNCT;

                sPunctWord.Base.bLangIndex = pLeftHandWord->Base.bLangIndex;

                pbLangSupported[bNumSymbs - 1] = sPunctWord.Base.bLangIndex;

                if (bHasTermSmartPunct) {
                    sPunctWord.Base.sWord[0] = _ET9_GetTermPunctChar(pLingInfo, pLingCmnInfo->wSecondLdbNum, i);
                }
                else {
                    sPunctWord.Base.sWord[0] = pWordSymbInfo->SymbsInfo[bNumSymbs - 1].DataPerBaseSym[0].sChar[0];
                }

                ET9AssertLog(sPunctWord.Base.sWord[0]);

                WLOG2Word(pLogFile2, "PunctWord", &sPunctWord);

                __SelLstWordSearch(pLingInfo, &sPunctWord, (ET9U16)(bNumSymbs - 1), 1, FREQ_TERM_PUNCT, bSpcMode);
            }

        }
        else {

            ET9U8 bPunctSeqSize;

            if (ET9AWSys_GetBilingualSupported(pLingInfo) &&
                (pLingInfo->pLingCmnInfo->wLdbNum == pLingInfo->pLingCmnInfo->wSecondLdbNum)) {

                bSecondLangTermPunctFirst = 1;
                bPunctSeqSize = (ET9U8)(bHasTermSmartPunct ? _ET9_GetNumTermPunct(pLingInfo, pLingCmnInfo->wSecondLdbNum) : 1);
            }
            else {
                bPunctSeqSize = (ET9U8)(bHasTermSmartPunct ? _ET9_GetNumTermPunct(pLingInfo, pLingCmnInfo->wFirstLdbNum) : 1);
            }

            for (i = 0; i < bPunctSeqSize; ++i) {

                ET9AWPrivWordInfo sPunctWord;

                _InitPrivWordInfo(&sPunctWord);

                sPunctWord.xTapFreq = 1;
                sPunctWord.xWordFreq = (ET9FREQPART)(0xFFFE - (i * 1000));
                sPunctWord.Base.wWordLen = 1;
                sPunctWord.bWordSrc = ET9WORDSRC_TERMPUNCT;

                sPunctWord.Base.bLangIndex = pLeftHandWord->Base.bLangIndex;

                pbLangSupported[bNumSymbs - 1] = sPunctWord.Base.bLangIndex;

                if (bHasTermSmartPunct) {
                    if (bSecondLangTermPunctFirst) {
                        sPunctWord.Base.sWord[0] = _ET9_GetTermPunctChar(pLingInfo, pLingCmnInfo->wSecondLdbNum, i);
                    }
                    else {
                        sPunctWord.Base.sWord[0] = _ET9_GetTermPunctChar(pLingInfo, pLingCmnInfo->wFirstLdbNum, i);
                    }
                }
                else {
                    sPunctWord.Base.sWord[0] = pWordSymbInfo->SymbsInfo[bNumSymbs - 1].DataPerBaseSym[0].sChar[0];
                }

                ET9AssertLog(sPunctWord.Base.sWord[0]);

                WLOG2Word(pLogFile2, "PunctWord", &sPunctWord);

                __SelLstWordSearch(pLingInfo, &sPunctWord, (ET9U16)(bNumSymbs - 1), 1, FREQ_TERM_PUNCT, bSpcMode);

            }

            if (ET9AWSys_GetBilingualSupported(pLingInfo)) {

                if (bSecondLangTermPunctFirst) {
                    bPunctSeqSize = (ET9U8)(bHasTermSmartPunct ? _ET9_GetNumTermPunct(pLingInfo, pLingCmnInfo->wFirstLdbNum) : 1);
                }
                else {
                    bPunctSeqSize = (ET9U8)(bHasTermSmartPunct ? _ET9_GetNumTermPunct(pLingInfo, pLingCmnInfo->wSecondLdbNum) : 1);
                }

                for (i = 0; i < bPunctSeqSize; ++i) {

                    ET9AWPrivWordInfo sPunctWord;

                    _InitPrivWordInfo(&sPunctWord);

                    sPunctWord.xTapFreq = 1;
                    sPunctWord.xWordFreq = (ET9FREQPART)(0xFFFE - (i * 1000));
                    sPunctWord.Base.wWordLen = 1;
                    sPunctWord.bWordSrc = ET9WORDSRC_TERMPUNCT;

                    sPunctWord.Base.bLangIndex = pLeftHandWord->Base.bLangIndex;

                    pbLangSupported[bNumSymbs - 1] = sPunctWord.Base.bLangIndex;

                    if (bHasTermSmartPunct) {
                        if (bSecondLangTermPunctFirst) {
                            sPunctWord.Base.sWord[0] = _ET9_GetTermPunctChar(pLingInfo, pLingCmnInfo->wFirstLdbNum, i);
                        }
                        else {
                            sPunctWord.Base.sWord[0] = _ET9_GetTermPunctChar(pLingInfo, pLingCmnInfo->wSecondLdbNum, i);
                        }
                    }
                    else {
                        sPunctWord.Base.sWord[0] = pWordSymbInfo->SymbsInfo[bNumSymbs - 1].DataPerBaseSym[0].sChar[0];
                    }

                    ET9AssertLog(sPunctWord.Base.sWord[0]);

                    WLOG2Word(pLogFile2, "PunctWord", &sPunctWord);

                    __SelLstWordSearch(pLingInfo, &sPunctWord, (ET9U16)(bNumSymbs - 1), 1, FREQ_TERM_PUNCT, bSpcMode);
                }
            }
        }

    }

    /* -------------------- build append using explicit (single) --------------------------*/

    if (bNumSymbs > 1 && __IsExactSymb(&pWordSymbInfo->SymbsInfo[bNumSymbs - 1]) && bHasDiscreteOnlyInfo) {

        if (*pwLockPoint == bNumSymbs) {
            WLOG2(fprintf(pLogFile2, "at lock point, won't add build append candidate (single)\n");)
            bFound = 0;
        }
        else {
            bFound = __LeftHandWordFromPrevDefaultWord(pLingInfo, bNumSymbs, pLeftHandWord, pPrevDefaultWord);
        }

        if (bFound) {

            ET9SYMB             sSymb;
            ET9AWPrivWordInfo   sLocalWord;

            WLOG2(fprintf(pLogFile2, "adding build append single explicit candidate\n");)
            WLOG2Word(pLogFile2, "build append, LeftHandWord", pLeftHandWord);

            __VerifyLockedSymbs(pLingInfo, pLeftHandWord, 0, pLeftHandWord->Base.wWordLen);

            pLingCmnInfo->Private.bStemsAllowed = 1;
            pLingCmnInfo->Private.wMaxWordLength = ET9MAXWORDSIZE;

            pSymbInfo = &pWordSymbInfo->SymbsInfo[bNumSymbs - 1];

            sSymb = pSymbInfo->eShiftState ? *pSymbInfo->DataPerBaseSym->sUpperCaseChar : *pSymbInfo->DataPerBaseSym->sChar;

            _InitPrivWordInfo(&sLocalWord);

            sLocalWord.Base.wWordLen = 1;
            sLocalWord.Base.sWord[0] = sSymb;
            sLocalWord.wEWordFreq = 0;
            sLocalWord.wTWordFreq = 0;
            sLocalWord.bWordSrc = _ET9_IsPunctChar(sSymb) ? ET9WORDSRC_BUILDAPPENDPUNCT : ET9WORDSRC_BUILDAPPEND;

            WLOG2Word(pLogFile2, "build append, BuildAppendWord (single)", &sLocalWord);

            __SelLstWordSearch(pLingInfo, &sLocalWord, (ET9U16)(bNumSymbs - 1), 1, FREQ_NORMAL, bSpcMode);
        }
    }

    /* -------------------- build append using explicit (sequence) --------------------------*/

    if (bNumSymbs > 1 && __IsExactSymb(&pWordSymbInfo->SymbsInfo[bNumSymbs - 1])) {

        if (*pwLockPoint == bNumSymbs) {
            WLOG2(fprintf(pLogFile2, "at lock point, won't add build append candidate (sequence)\n");)
            bFound = 0;
        }
        else {

            ET9U16 wExplicitPoint;

            /* find where the explicits started */

            wExplicitPoint = bNumSymbs;

            if (bNumSymbs > 1) {

                pSymbInfo = &pWordSymbInfo->SymbsInfo[bNumSymbs - 2];

                for (; wExplicitPoint > 1 && wExplicitPoint > *pwLockPoint; --wExplicitPoint, --pSymbInfo) {
                    if (!__IsExactSymb(pSymbInfo)) {
                        break;
                    }
                }
            }

            WLOG2(fprintf(pLogFile2, "build append, wExplicitPoint = %d, wLockPoint = %d\n", wExplicitPoint, *pwLockPoint);)

            /* get the earliest prev default or explicit if there was a lock */

            if (wExplicitPoint == 1) {
                _InitPrivWordInfo(pLeftHandWord);
                bFound = 1;
                WLOG2Word(pLogFile2, "build append, from beginning", pLeftHandWord);
            }
            else if (wExplicitPoint == *pwLockPoint) {

                ET9STATUS wStatus;

                wStatus = ET9GetExactWord(pWordSymbInfo, &sSimpleWord, pLingInfo->Private.pConvertSymb, pLingInfo->Private.pConvertSymbInfo);

                if (wStatus) {
                    bFound = 0;
                }
                else {

                    _ET9SimpleWordToPrivWord(&sSimpleWord, pLeftHandWord);

                    if (pLeftHandWord->Base.wWordLen < *pwLockPoint) {
                        bFound = 0;
                    }
                    else {
                        pLeftHandWord->Base.wWordLen = *pwLockPoint;
                        ++wExplicitPoint;
                        bFound = 1;
                        WLOG2Word(pLogFile2, "build append, from exact", pLeftHandWord);
                    }
                }
            }
            else {
                if (__CaptureGetDefault(pLingInfo, pLeftHandWord, wExplicitPoint)) {
                    if (pLeftHandWord->Base.wWordCompLen) {
                        WLOG2Word(pLogFile2, "build append, prev default with compl len", pLeftHandWord);
                        pLeftHandWord->Base.wWordLen = (ET9U16)(pLeftHandWord->Base.wWordLen - pLeftHandWord->Base.wWordCompLen);
                        pLeftHandWord->Base.wWordCompLen = 0;
                    }
                    bFound = 1;
                    WLOG2Word(pLogFile2, "build append, from prev default", pLeftHandWord);
                }
                else {
                    bFound = 0;
                }
            }

            if (!bFound) {

                /* no use trying just the prev default here, handle in the "single" build above */

                WLOG2(fprintf(pLogFile2, "build append, failed to build a good left hand side\n");)
            }
            else {

                /* add explicits that exist between the default and the current explicit */

                ET9SYMB *psSymb;

                pSymbInfo = &pWordSymbInfo->SymbsInfo[wExplicitPoint - 1];
                psSymb = &pLeftHandWord->Base.sWord[pLeftHandWord->Base.wWordLen];

                for (; wExplicitPoint < bNumSymbs; ++wExplicitPoint, ++pSymbInfo, ++psSymb) {

                    if (pLeftHandWord->Base.wWordLen >= ET9MAXWORDSIZE) {
                        WLOG2(fprintf(pLogFile2, "build append, too long left hand side...\n");)
                        break;
                    }

                    if (wExplicitPoint <= *pwLockPoint) {
                        *psSymb = pSymbInfo->sLockedSymb;
                    }
                    else {
                        *psSymb = pSymbInfo->eShiftState ? *pSymbInfo->DataPerBaseSym->sUpperCaseChar : *pSymbInfo->DataPerBaseSym->sChar;
                    }

                    ++pLeftHandWord->Base.wWordLen;

                    WLOG2Word(pLogFile2, "build append, LeftHandWord got an explicit", pLeftHandWord);
                }
            }
        }

        if (bFound) {

            ET9SYMB             sSymb;
            ET9AWPrivWordInfo   sLocalWord;

            WLOG2(fprintf(pLogFile2, "build append, adding trailing explicit candidate\n");)
            WLOG2Word(pLogFile2, "build append, LeftHandWord", pLeftHandWord);

            __VerifyLockedSymbs(pLingInfo, pLeftHandWord, 0, pLeftHandWord->Base.wWordLen);

            pLingCmnInfo->Private.bStemsAllowed = 1;
            pLingCmnInfo->Private.wMaxWordLength = ET9MAXWORDSIZE;

            pSymbInfo = &pWordSymbInfo->SymbsInfo[bNumSymbs - 1];

            sSymb = pSymbInfo->eShiftState ? *pSymbInfo->DataPerBaseSym->sUpperCaseChar : *pSymbInfo->DataPerBaseSym->sChar;

            _InitPrivWordInfo(&sLocalWord);

            sLocalWord.Base.wWordLen = 1;
            sLocalWord.Base.sWord[0] = sSymb;
            sLocalWord.wEWordFreq = 0;
            sLocalWord.wTWordFreq = 0;
            sLocalWord.bWordSrc = _ET9_IsPunctChar(sSymb) ? ET9WORDSRC_BUILDAPPENDPUNCT : ET9WORDSRC_BUILDAPPEND;

            WLOG2Word(pLogFile2, "build append, BuildAppendWord (sequence)", &sLocalWord);

            __SelLstWordSearch(pLingInfo, &sLocalWord, (ET9U16)(bNumSymbs - 1), 1, FREQ_NORMAL, bSpcMode);
        }
    }

    /* -------------------- auto append --------------------------*/

    if (bNumSymbs > 1 && ET9AUTOAPPENDINLIST(pLingCmnInfo) && bHasDiscreteOnlyInfo) {

        /* after a lock the default will have the correct value for auto append (no need for a special case) */

        if (*pwLockPoint == bNumSymbs) {
            WLOG2(fprintf(pLogFile2, "at lock point, won't add auto append candidates\n");)
            bFound = 0;
        }
        else if (pWordSymbInfo->SymbsInfo[bNumSymbs - 1].eInputType == ET9MULTITAPKEY) {
            WLOG2(fprintf(pLogFile2, "at multitap symb, won't add auto append candidates\n");)
            bFound = 0;
        }
        else {
            bFound = __LeftHandWordFromPrevDefaultWord(pLingInfo, bNumSymbs, pLeftHandWord, pPrevDefaultWord);

            if (*pwLockPoint) {
                pLeftHandWord->Base.bLangIndex = pLingCmnInfo->Base.pWordSymbInfo->SymbsInfo[*pwLockPoint - 1].bLockLanguageSource;
            }

            WLOG2(if (!bFound) {
                      fprintf(pLogFile2, "prev default too short for auto append (skipping)\n");
                  })
        }

        if (bLastInputIsTrace) {
            bFound = 0;
        }

        if (bFound) {

            ET9U8               bSyms;
            ET9U8               bKeyIndex;
            ET9U8               bFirstLevelOnly;
            ET9U8               bAutoAppendSymbCount;
            ET9SYMB             *psChar;
            ET9DataPerBaseSym   *pSymbData;
            ET9AWPrivWordInfo   sAutoAppendWord;
            ET9U16              wLdbNum;

            WLOG2(fprintf(pLogFile2, "adding auto append candidates\n");)
            WLOG2Word(pLogFile2, "LeftHandWord", pLeftHandWord);

            /* check to see if word being autoappended is uppercase... if so, */
            /* downshift it for autoappended entries                          */

            pSymbInfo = pWordSymbInfo->SymbsInfo;

            if ((*pwLockPoint == 0) &&
                pSymbInfo->bSymbType == ET9KTLETTER &&
                !pSymbInfo->eShiftState) {

                wLdbNum  = (pLeftHandWord->Base.bLangIndex == ET9AWSECOND_LANGUAGE) ? pLingCmnInfo->wSecondLdbNum : pLingCmnInfo->wFirstLdbNum;

                if (_ET9SymIsUpper(pLeftHandWord->Base.sWord[0], pWordSymbInfo->Private.wLocale)) {
                    pLeftHandWord->Base.sWord[0] = _ET9SymToLower(pLeftHandWord->Base.sWord[0], wLdbNum);
                    WLOG2Word(pLogFile2, "Downshifted LeftHandWord", pLeftHandWord);
                }
            }

            pSymbInfo = &pWordSymbInfo->SymbsInfo[bNumSymbs - 1];

            switch (pSymbInfo->eInputType)
            {
                case ET9DISCRETEKEY:
                case ET9CUSTOMSET:
                case ET9MULTITAPKEY:
                    bFirstLevelOnly = 0;
                    break;
                default:
                    bFirstLevelOnly = 1;
                    break;
            }

            __VerifyLockedSymbs(pLingInfo, pLeftHandWord, 0, pLeftHandWord->Base.wWordLen);

            pLingCmnInfo->Private.bStemsAllowed = 1;
            pLingCmnInfo->Private.wMaxWordLength = ET9MAXWORDSIZE;

            bAutoAppendSymbCount = 0;

            bKeyIndex = bFirstLevelOnly ? 1 : pSymbInfo->bNumBaseSyms;
            pSymbData = pSymbInfo->DataPerBaseSym;

            while (bKeyIndex--) {

                psChar = pSymbData->sChar;
                bSyms   = pSymbData->bNumSymsToMatch;

                while (bSyms--) {
                    if (_ET9AWLdbIsSymbolUsed(pLingInfo, pLingCmnInfo->wFirstLdbNum, *psChar) ||
                        (ET9AWSys_GetBilingualSupported(pLingInfo) &&
                         _ET9AWLdbIsSymbolUsed(pLingInfo, pLingCmnInfo->wSecondLdbNum, *psChar))) {

                        _InitPrivWordInfo(&sAutoAppendWord);

                        sAutoAppendWord.xTapFreq =  1;
                        sAutoAppendWord.xWordFreq = (ET9FREQPART)(_ET9_IsNumeric(*psChar) ? *psChar : (0xFFFF - *psChar));    /* unicode order */
                        sAutoAppendWord.bWordSrc = ET9WORDSRC_AUTOAPPEND;
                        sAutoAppendWord.Base.wWordLen = 1;
                        sAutoAppendWord.Base.sWord[0] = pSymbInfo->eShiftState ? _ET9SymToUpper(*psChar, pLingCmnInfo->wLdbNum) : *psChar;

                        if (ET9AWSys_GetBilingualSupported(pLingInfo)) {
                            sAutoAppendWord.Base.bLangIndex = ET9AWBOTH_LANGUAGES;
                        }
                        else if (wFlushPoint) {
                            sAutoAppendWord.Base.bLangIndex = pLeftHandWord->Base.bLangIndex;
                        }
                        else {
                            sAutoAppendWord.Base.bLangIndex = ET9AWFIRST_LANGUAGE;
                        }
                        if (bAutoAppendSymbCount < MAX_AUTO_APPEND_SYMBS && bAutoAppendSymbCount + 3 < pLingCmnInfo->Private.bListSize) {

                            WLOG2Word(pLogFile2, "AutoAppendWord", &sAutoAppendWord);

                            __SelLstWordSearch(pLingInfo, &sAutoAppendWord, (ET9U16)(bNumSymbs - 1), 1, FREQ_TERM_PUNCT, bSpcMode);
                        }
                        else {
                            WLOG2Word(pLogFile2, "too many auto append words, discarding", &sAutoAppendWord);
                        }

                        ++bAutoAppendSymbCount;
                    }

                    ++psChar;
                }

                ++pSymbData;
            }
        }
    }

    /* -------------------- default space char --------------------------*/

    /* using the default and a single key press to generate a delayed "auto space" */

    if (bNumSymbs > 1 && !pLingCmnInfo->Private.bLastBuildShrinking && ET9AUTOSPACE(pLingCmnInfo)) {

        const ET9U8                 bLastIndex = (ET9U8)(pWordSymbInfo->bNumSymbs - 1);
        ET9SymbInfo         const * pLastSymb = &pWordSymbInfo->SymbsInfo[bLastIndex];
        ET9AWPrivWordInfo   * const pPrevDefaultWord = &pLingCmnInfo->Private.sPrevDefaultWord;
        ET9AWPrivWordInfo   * const pLeftHandWord = &pLingCmnInfo->Private.sLeftHandWord;

        if (pLastSymb[-1].bTraceIndex &&
            !pLastSymb[0].bTraceIndex &&
            !pWordSymbInfo->Private.sRequiredWord.wLen &&
            (pLastSymb[0].bSymbType == ET9KTLETTER || pLastSymb[0].bSymbType == ET9KTNUMBER || pLastSymb[0].bSymbType == ET9KTUNKNOWN) &&
            pPrevDefaultWord->Base.wWordLen &&
            pPrevDefaultWord->Base.wWordLen + 2 <= ET9MAXWORDSIZE) {

            _InitPrivWordInfo(pLeftHandWord);

            _ET9SymCopy(pLeftHandWord->Base.sWord, pPrevDefaultWord->Base.sWord, pPrevDefaultWord->Base.wWordLen);

            pLeftHandWord->Base.wWordLen = pPrevDefaultWord->Base.wWordLen;

            pLeftHandWord->Base.sWord[pLeftHandWord->Base.wWordLen++] = ' ';

            WLOG2(fprintf(pLogFile2, "adding default space char candidates\n");)
            WLOG2Word(pLogFile2, "LeftHandWord", pLeftHandWord);

            {
                ET9U8 bEntries;

                pLingCmnInfo->Private.bStemsAllowed = 0;
                pLingCmnInfo->Private.wMaxWordLength = 1;

                _ET9AWLdbWordsSearch(pLingInfo,
                                     pLingCmnInfo->wFirstLdbNum,
                                     bLastIndex,
                                     1,
                                     &bEntries,
                                     FREQ_BUILDSPACE,
                                     ET9ASPCMODE_OFF);

                _ET9AWSuppDBSelListBuild(pLingInfo,
                                         bLastIndex,
                                         1,
                                         &bEntries,
                                         FREQ_BUILDSPACE,
                                         ET9SUPPDB_AS_SOURCES,
                                         ET9ASPCMODE_OFF);
            }
        }
    }

    /* -------------------- done building --------------------------*/

    /* if no words at this point we are done... */

    if (!pLingCmnInfo->Private.bTotalWords && !bHasTermSmartPunct && !bHasTermPunct) {

        WLOG2(fprintf(pLogFile2, "__ET9AWDoSelLstBuild, (no words) END *****\n");)

        pLingCmnInfo->Private.bTotalWords = 0;

        __RestoreSelectionListOverrideValues(pLingCmnInfo);
        return ET9STATUS_NONE;
    }

#ifdef ET9_DEBUG
    {
        ET9INT snCount;

        pAmbigWord = pLingCmnInfo->Private.pWordList;
        for (snCount = pLingCmnInfo->Private.bListSize; snCount; --snCount, ++pAmbigWord) {
            ET9AssertLog(pAmbigWord->Base.wWordLen <= ET9MAXWORDSIZE);
        }
    }
#endif

    /* Assign LM freqs here */

    if (bUsingLM) {
        __ET9AWAssignLMFreqs(pLingInfo);
    }
    else if (pLingCmnInfo->Private.eCurrSelectionListMode == ET9ASLMODE_MIXED) {
        __ET9AWGetMaxFreq(pLingInfo);
    }

    if (pLingCmnInfo->Private.eCurrSelectionListMode == ET9ASLMODE_MIXED) {
        __ET9AWUpdateCustomWordFreqs(pLingInfo);
    }

    /* look for exact and exactish and see if exact won */

    WLOG2B(fprintf(pLogFile2, "Searching for exact and exactish\n");)

    pAmbigWord = pLingCmnInfo->Private.pWordList;
    bFound = 0;
    pWord = 0;

    for (i = pLingCmnInfo->Private.bTotalWords; bExactInList && i; --i, ++pAmbigWord) {
        if (!ISEXACTSRC(pAmbigWord->bWordSrc)) {
            if (__ET9AWIsLikeExactMatch(pLingInfo, pAmbigWord)) {
                WLOG2BWord(pLogFile2, "..isLikeExact", pAmbigWord);
                if (!pWord || __PriorityCompare(pLingCmnInfo, pAmbigWord, pWord, 0) > 0) {
                    WLOG2BWord(pLogFile2, "....new top", pAmbigWord);
                    pWord = pAmbigWord;
                }
            }
            else {
                bFound = 1;
            }
        }
        else {
            WLOG2BWord(pLogFile2, "..found exact word", pAmbigWord);
            pExactWord = pAmbigWord;
        }
    }

    bExactWon = 0;
    if (pExactWord && ISGENUINESRC(pExactWord->bWordSrc)) {
        pExactWord->bWordSrc = GETBASESRC(pExactWord->bWordSrc);
        if (pWord && __PriorityCompare(pLingCmnInfo, pExactWord, pWord, 0) >= 0) {
            WLOG2B(fprintf(pLogFile2, "..exact won!\n");)
            bExactWon = 1;
        }
        WLOG2BWord(pLogFile2, "..exact tag", pExactWord);
        pExactWord->bWordSrc |= EXACTOFFSET;
    }
    if (pWord && bFound) {
        WLOG2BWord(pLogFile2, "..exactish tag", pWord);
        pWord->bWordSrc |= EXACTISHOFFSET;
    }

    /* before sorting:
        disposable quality words goes into the stem pool (e.g. RDB words, not UDB words)
        stem quality words goes (back) into the stem pool (can have changed source during build) */

    pAmbigWord = pLingCmnInfo->Private.pWordList;

    for (i = pLingCmnInfo->Private.bTotalWords; i; --i, ++pAmbigWord) {

        if (pAmbigWord->bWordSrc == ET9WORDSRC_STEMPOOL) {
            continue;
        }

        if (pAmbigWord->bWordQuality <= DISPOSABLE_QUALITY) {

            WLOG2Word(pLogFile2, "disposable quality word will be put into the stem pool", pAmbigWord);

            pAmbigWord->bWordSrc = ET9WORDSRC_STEMPOOL;
        }
        else if (pAmbigWord->bWordQuality <= STEM_QUALITY) {

            WLOG2Word(pLogFile2, "stem quality word will be put into the stem pool", pAmbigWord);

            pAmbigWord->bWordSrc = ET9WORDSRC_STEMPOOL;
        }
    }

    /* sort & final */

    __ET9AWSortPriorityList(pLingCmnInfo);

    __ET9AWPriorityListFinalOrder(pLingCmnInfo);

    /* setting default index - keep rules simple! (not a speed factor) */

    if (!bNumSymbs) { /* NWP */

        WLOG2(fprintf(pLogFile2, "[1] DEFAULT: 0 - NWP\n");)

        pLingCmnInfo->Private.bDefaultIndex = 0;
    }
    else if (pLingCmnInfo->Private.bTotalWords <= 1) {

        WLOG2(fprintf(pLogFile2, "[2] DEFAULT: 0 - bTotalWords <= 1\n");)

        pLingCmnInfo->Private.bDefaultIndex = 0;
    }
    else if (bExactLock && !ET9DEVSTATEINHBEXACTLOCK_MODE(pLingCmnInfo->Private.dwDevStateBits)) {

        WLOG2(fprintf(pLogFile2, "[3] DEFAULT: 0 - bExactLock && !ET9DEVSTATEINHBEXACTLOCK_MODE\n");)

        pLingCmnInfo->Private.bDefaultIndex = 0;
    }
    else if (bLastInputIsMultitap) {

        WLOG2(fprintf(pLogFile2, "[4] DEFAULT: 0 - bLastInputIsMultitap\n");)

        pLingCmnInfo->Private.bDefaultIndex = 0;
    }
    else if (!ISREALSRC(pLingCmnInfo->Private.pWordList[pLingCmnInfo->Private.bWordList[1]].bWordSrc) &&
             pExactWord &&
             !__HasNumericStem(pLingCmnInfo, pExactWord->Base.sWord, pExactWord->Base.wWordLen)) {

        WLOG2(fprintf(pLogFile2, "[5] DEFAULT: 0 - !ISREALSRC && !HasNumericStem\n");)

        pLingCmnInfo->Private.bDefaultIndex = 0;
    }
    else if (!bExactInList ||
             (!pWord && pExactWord && ISGENUINESRC(pExactWord->bWordSrc) && !ISNONOVERRIDESRC(pExactWord->bWordSrc))) {

        WLOG2(fprintf(pLogFile2, "[6] DEFAULT: 0 - !ExactInList || !Exactish && ExactWord && ISGENUINESRC && !ISNONOVERRIDESRC\n");)

        pLingCmnInfo->Private.bDefaultIndex = 0;
    }
    else if (pExactWord &&
             (GETBASESRC(pExactWord->bWordSrc) >= ET9WORDSRC_EXACT) &&
             !ISGENUINESRC(pLingCmnInfo->Private.pWordList[pLingCmnInfo->Private.bWordList[1]].bWordSrc)) {

        WLOG2(fprintf(pLogFile2, "[7] DEFAULT: 0 - SRC >= EXACT && !ISGENUINESRC\n");)

        pLingCmnInfo->Private.bDefaultIndex = 0;
    }
    else if (bExactWon &&
             pExactWord &&
             (bNumSymbs != 1 || !_ET9_IsNumericString(pExactWord->Base.sWord, 1))) {

        /* used to NOT allow any numeric strings to win here, now all allowed
           exception 1) single digits are disallowed */

        WLOG2(fprintf(pLogFile2, "[8] DEFAULT: 0 - bNumSymbs != 1 || !IsNumericString\n");)

        pLingCmnInfo->Private.bDefaultIndex = 0;
    }
    else if (bExactInList && (bHasRegionalInfo || pWordSymbInfo->Private.bRequiredHasRegionalInfo) && pWordSymbInfo->Private.bRequiredInhibitOverride) {

        WLOG2(fprintf(pLogFile2, "[9] DEFAULT: 0 - bRequiredHasRegionalInfo && bRequiredInhibitOverride\n");)

        pLingCmnInfo->Private.bDefaultIndex = 0;
    }
    else if (bExactInList && bHasRegionalInfo && __ExactHasOverrideInhibitor(pLingCmnInfo)) {

        WLOG2(fprintf(pLogFile2, "[10] DEFAULT: 0 - ExactHasOverrideInhibitor\n");)

        pLingCmnInfo->Private.bDefaultIndex = 0;
    }
    else if (bExactInList && bHasRegionalInfo && __HasUserShiftOverrideInhibit(pLingCmnInfo)) {

        WLOG2(fprintf(pLogFile2, "[11] DEFAULT: 0 - HasUserShiftOverrideInhibit\n");)

        pLingCmnInfo->Private.bDefaultIndex = 0;
    }
    else if (bExactInList && bHasRegionalInfo && bNumSymbs > 1 && bHasAllShiftedInfo) {

        WLOG2(fprintf(pLogFile2, "[12] DEFAULT: 0 - bNumSymbs > 1 && bHasAllShiftedInfo\n");)

        pLingCmnInfo->Private.bDefaultIndex = 0;
    }
    else if (bExactInList && bHasRegionalInfo && pWordSymbInfo->Private.bClearSymbEpisodeCount >= 2) {

        WLOG2(fprintf(pLogFile2, "[13] DEFAULT: 0 - bClearSymbEpisodeCount >= 2\n");)

        pLingCmnInfo->Private.bDefaultIndex = 0;
    }
    else if (bExactInList && bHasRegionalInfo && _ET9IsInhibitTapOverrideAfterTrace(pWordSymbInfo)) {

        WLOG2(fprintf(pLogFile2, "[14] DEFAULT: 0 - IsInhibitTapOverrideAfterTrace\n");)

        pLingCmnInfo->Private.bDefaultIndex = 0;
    }
    else if (bExactInList && bHasRegionalInfo && __HasStemDistanceOverrideInhibit(pLingCmnInfo)) {

        WLOG2(fprintf(pLogFile2, "[15] DEFAULT: 0 - HasStemDistanceOverrideInhibit\n");)

        pLingCmnInfo->Private.bDefaultIndex = 0;
    }
    else {

        WLOG2(fprintf(pLogFile2, "[99] DEFAULT: 1\n");)

        pLingCmnInfo->Private.bDefaultIndex = 1;
    }

    /* post process the list (must be after setting default index) */

    __DoBuildPostProcessing(pLingInfo);

    if (ET9AWSys_GetBilingualSupported(pLingInfo)) {
        __DoBilingualReorder(pLingInfo, wFlushPoint, pLeftHandWord);
    }

    pLingCmnInfo->Private.bApplyBoosting = 0;

    /* possibly boost a candidate to default position */

    if (bUsingLM &&
        pLingCmnInfo->Private.eCurrSelectionListMode != ET9ASLMODE_MIXED &&
        pLingCmnInfo->Private.bContextWordSize &&
        !__IsCandidateForImplicitBreakContext(pLingInfo)) {

        __BoostTopCandidate(pLingInfo);
    }

    /* possibly default a "DEFAULT-SPACE-CHAR" */

    __DoDefaultSpaceCharProcessing(pLingInfo);

    /* special default index handling for the required word */

    if (pWordSymbInfo->Private.bRequiredLocate && bNumSymbs && !wFlushPoint) {

        ET9U16              wCmpLen;
        ET9SYMB             *pStr1;
        ET9SYMB             *pStr2;

        WLOG2(fprintf(pLogFile2, "Default index, searching for required word\n");)

        for (i = 0; i < pLingCmnInfo->Private.bTotalWords; ++i) {

            pWord = &pLingCmnInfo->Private.pWordList[pLingCmnInfo->Private.bWordList[i]];

            if (pWord->Base.wWordLen == pWordSymbInfo->Private.sRequiredWord.wLen) {

                wCmpLen = pWord->Base.wWordLen;
                pStr1 = pWordSymbInfo->Private.sRequiredWord.sString;
                pStr2 = pWord->Base.sWord;
                ++wCmpLen;
                while (--wCmpLen) {
                    if (*pStr1++ != *pStr2++) {
                        break;
                    }
                }

                if (wCmpLen) {
                    continue;
                }

                WLOG2(fprintf(pLogFile2, "Default index, found required word @ %d\n", i);)

                pLingCmnInfo->Private.bRequiredFound = 1;

                pLingCmnInfo->Private.bDefaultIndex = (ET9U8)i;

                /* for this to work properly the word can't have a substitution */

                if (pWord->Base.wSubstitutionLen) {

                    WLOG2(fprintf(pLogFile2, "required word has substitution, removing it\n");)

                    pWord->Base.wSubstitutionLen = 0;

                    /* leave the source as is, doesn't matter and it's kind of unknown, the public one will be set ok */
                }

                break;
            }
        }
    }

    /* final process the list (must be after all other list ordering) */

    __DoBuildFinalProcessing(pLingInfo);

    /* assign public source (might not be consequtive in the array (holes)) */

    for (i = 0; i < pLingCmnInfo->Private.bTotalWords; ++i) {

        pWord = &pLingCmnInfo->Private.pWordList[pLingCmnInfo->Private.bWordList[i]];

        pWord->Base.bWordSource = ET9AWORDSOURCE_CONSTRUCTED;

        switch (GETBASESRC(pWord->bWordSrc))
        {
            case ET9WORDSRC_NONE:
                pWord->Base.bWordSource = ET9AWORDSOURCE_NEWWORD;
                break;
            case ET9WORDSRC_TERMPUNCT:
                pWord->Base.bWordSource = ET9AWORDSOURCE_TERMPUNCT;
                break;
            case ET9WORDSRC_AUTOAPPEND:
                pWord->Base.bWordSource = ET9AWORDSOURCE_AUTOAPPEND;
                break;
            case ET9WORDSRC_STEM:
            case ET9WORDSRC_STEMPOOL:
                pWord->Base.bWordSource = ET9AWORDSOURCE_STEM;
                break;
            case ET9WORDSRC_QUDB:
                if (!wFlushPoint) {
                    pWord->Base.bWordSource = ET9AWORDSOURCE_CUSTOM;
                }
                break;
            case ET9WORDSRC_RUDB:
                if (!wFlushPoint) {
                    if (pWord->bIsUDBWord) {
                        pWord->Base.bWordSource = ET9AWORDSOURCE_CUSTOM;
                    }
                    else {
                        pWord->Base.bWordSource = ET9AWORDSOURCE_LDB;
                    }
                }
                break;
            case ET9WORDSRC_LDB:
            case ET9WORDSRC_LDB_PROMOTION:
                if (!wFlushPoint) {
                    pWord->Base.bWordSource = ET9AWORDSOURCE_LDB;
                }
                break;
            case ET9WORDSRC_MDB:
                if (!wFlushPoint) {
                    pWord->Base.bWordSource = ET9AWORDSOURCE_MDB;
                }
                break;
            case ET9WORDSRC_CDB:
                if (!wFlushPoint) {
                    pWord->Base.bWordSource = ET9AWORDSOURCE_CDB;
                }
                break;
            case ET9WORDSRC_LAS_SHORTCUT:
            case ET9WORDSRC_ASDB_SHORTCUT:
                if (!wFlushPoint && pWord->Base.wSubstitutionLen) {
                    pWord->Base.bWordSource = ET9AWORDSOURCE_ASDB;
                }
                break;
            default:
                break;
            }
    }

    /* result info */

    *pbTotalWords = pLingCmnInfo->Private.bTotalWords;

    WLOG2(fprintf(pLogFile2, "pbTotalWords = %d\n", (int)*pbTotalWords);)

#ifdef ET9_ACTIVATE_SLST_STATS
    ++pLingCmnInfo->Private.sStats.dwBuildCount;

    if (pLingCmnInfo->Private.sStats.dwInsertCount > pLingCmnInfo->Private.sStats.dwMaxListInserts) {
        pLingCmnInfo->Private.sStats.dwMaxListInserts = pLingCmnInfo->Private.sStats.dwInsertCount;
    }

    pLingCmnInfo->Private.sStats.dwInsertCount = 0;
#endif

    __LogFullSelList(pLingInfo, "ET9AWSelLstBuild - all done", NULL);

    /* done */

#ifdef ET9_DEBUG

    /* verify locked chars and dupe quality */

    if (*pwLockPoint) {

        ET9INT              snCount;
        ET9INT              snIndex;
        ET9SYMB             *psSymb;
        ET9AWPrivWordInfo   *pWord1;
        ET9SymbInfo         *pSymbInfo;

        for (snIndex = 0; snIndex < pLingCmnInfo->Private.bTotalWords; ++snIndex) {

            pWord1 = &pLingCmnInfo->Private.pWordList[pLingCmnInfo->Private.bWordList[snIndex]];

            ET9AssertLog(pWord1->bWordQuality != DUPE_QUALITY);

            if (GETBASESRC(pWord1->bWordSrc) == ET9WORDSRC_MAGICSTRING) {
                continue;
            }

            if (pWord1->Base.wWordLen < *pwLockPoint) {
                WLOG2Word(pLogFile2, "word verified", pWord1);
                WLOG2(fprintf(pLogFile2, "word shorter than lock, index = %d\n", snIndex);)
            }

            ET9AssertLog(pWord1->Base.wWordLen >= *pwLockPoint);

            psSymb = pWord1->Base.sWord;
            pSymbInfo = pWordSymbInfo->SymbsInfo;

            for (snCount = *pwLockPoint; snCount; --snCount, ++psSymb, ++pSymbInfo) {

                ET9SYMB sLockedSymb = pSymbInfo->sLockedSymb;

                if (pLingInfo->Private.pConvertSymb != NULL) {
                    pLingInfo->Private.pConvertSymb(pLingInfo->Private.pConvertSymbInfo, &sLockedSymb);
                }

                /* this is the final lock verification, must be exact match to converted lock */

                if (*psSymb != sLockedSymb && !pLingCmnInfo->Private.bExpandAsDuringBuild) {
                    WLOG2Word(pLogFile2, "word verified", pWord1);
                    WLOG2(fprintf(pLogFile2, "lock symb missmatch index = %d, pos = %d, locked symb = %x\n", snIndex, (psSymb - pWord1->Base.sWord), sLockedSymb);)
                }

                ET9AssertLog(*psSymb == sLockedSymb || pLingCmnInfo->Private.bExpandAsDuringBuild);
            }
        }
    }

    /* verify that the list has no duplicates */

    {
        ET9INT              snLook;
        ET9INT              snIndex;
        ET9AWPrivWordInfo   *pWord1;
        ET9AWPrivWordInfo   *pWord2;

        for (snIndex = 0; snIndex + 1 < pLingCmnInfo->Private.bTotalWords; ++snIndex) {

            pWord1 = &pLingCmnInfo->Private.pWordList[pLingCmnInfo->Private.bWordList[snIndex]];

            for (snLook = snIndex + 1; snLook < pLingCmnInfo->Private.bTotalWords; ++snLook) {

                pWord2 = &pLingCmnInfo->Private.pWordList[pLingCmnInfo->Private.bWordList[snLook]];

                if (pWord1->Base.wWordLen == pWord2->Base.wWordLen) {
                    ET9AssertLog(_ET9symbncmp(pWord1->Base.sWord, pWord2->Base.sWord, pWord1->Base.wWordLen));
                }
                if (pWord1->Base.wSubstitutionLen == pWord2->Base.wWordLen) {
                    ET9AssertLog(_ET9symbncmp(pWord1->Base.sSubstitution, pWord2->Base.sWord, pWord1->Base.wSubstitutionLen));
                }
                if (pWord1->Base.wWordLen == pWord2->Base.wSubstitutionLen) {
                    ET9AssertLog(_ET9symbncmp(pWord1->Base.sWord, pWord2->Base.sSubstitution, pWord1->Base.wWordLen));
                }
                if (pWord1->Base.wSubstitutionLen && pWord1->Base.wSubstitutionLen == pWord2->Base.wSubstitutionLen) {
                    /* only make this check if not generating lowercase shortcut */
                    if (!ET9DOWNSHIFTDEFAULTENABLED(pLingCmnInfo) ||
                        snIndex != pLingCmnInfo->Private.bDefaultIndex) {
                        ET9AssertLog(_ET9symbncmp(pWord1->Base.sSubstitution, pWord2->Base.sSubstitution, pWord1->Base.wSubstitutionLen));
                    }
                }
            }
        }
    }

    /* verify completion point compliance and frequency compliance */

    {
        ET9U16  wIndex;
        ET9U8   bCompletionBeforeCompletionPoint;

        ET9AssertLog(pLingCmnInfo->Private.bTotalWords == pLingCmnInfo->Private.bTotalWords);

        for (wIndex = 0; wIndex < pLingCmnInfo->Private.bTotalWords; ++wIndex) {

            const ET9U8 bIndex = pLingCmnInfo->Private.bWordList[wIndex];
            ET9AWPrivWordInfo * const pWord = &pLingCmnInfo->Private.pWordList[bIndex];

            ET9AssertLog(pWord->Base.wWordLen);
            ET9AssertLog(pWord->xTapFreq >= 0);
            ET9AssertLog(pWord->xTotFreq >= 0);
            ET9AssertLog(pWord->xWordFreq >= 0);

            bCompletionBeforeCompletionPoint = (ET9U8)(
                pWord->Base.wWordCompLen &&
                pWordSymbInfo->bNumSymbs &&    /* not prediction */
                pWordSymbInfo->bNumSymbs < pLingCmnInfo->Private.wWordCompletionPoint &&
                !pWord->bEditDistSpc &&
                !pWord->bEditDistFree);

            ET9AssertLog(!bCompletionBeforeCompletionPoint);
        }
    }

#endif

    WLOG2(fprintf(pLogFile2, "__ET9AWDoSelLstBuild, (normal) END *****\n");)

    __RestoreSelectionListOverrideValues(pLingCmnInfo);
    return ET9STATUS_NONE;
}

#ifdef ET9_PRE_DUPE_SELLIST
/*---------------------------------------------------------------------------*/
/** \internal
 * Get word count before dupe supression.
 *
 * @return Number of words.
 */

ET9UINT ET9FARCALL ET9PreDupeGetSelListWordCount()
{
    return nPreDupeEntryCount;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Get pre dupe word.
 *
 * @param pWord                     Pointer to word.
 * @param nIndex                    Word to get.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL ET9PreDupeGetWord(ET9AWPrivWordInfo *pWord, ET9UINT nIndex)
{
    ET9STATUS wStatus = ET9STATUS_NONE;
    ET9SYMB sWordData[200];
    ET9UINT i;

    if (pWord == NULL) {
        wStatus =  ET9STATUS_INVALID_MEMORY;
    }
    else if (!nPreDupeEntryCount) {
        wStatus =  ET9STATUS_EMPTY;
    }
    else if (nIndex >= nPreDupeEntryCount) {
        wStatus =  ET9STATUS_OUT_OF_RANGE;
    }
    else {
        if (pPreDupeFile) {
            fclose(pPreDupeFile);
        }
        pPreDupeFile = fopen(DEFAULT_PREDUPE_FILE_NAME, "r,ccs=UNICODE");
        for (i = 0; i < nIndex; ++i) {
            while (fgetwc(pPreDupeFile) != L'\n');
        }
        i = 0;
        while ((sWordData[i++] = fgetwc(pPreDupeFile)) != L'\n');
        fclose(pPreDupeFile);
        pPreDupeFile = NULL;
        _ET9ClearMem((ET9U8 *)pWord, sizeof(ET9AWPrivWordInfo));
        i = 0;
        while (sWordData[i] != L' ') {
            pWord->Base.sWord[i] = sWordData[i];
            ++i;
        }
        i += 3;
        sWordData[i+2] = 0;
        pWord->Base.wWordLen = (ET9U16)_wtoi(&sWordData[i]);
        i += 3;
        sWordData[i+2] = 0;
        pWord->Base.wWordCompLen = (ET9U16)_wtoi(&sWordData[i]);
        i += 3;
        sWordData[i+2] = 0;
        pWord->Base.wSubstitutionLen = (ET9U16)_wtoi(&sWordData[i]);
        i += 3;
        sWordData[i+1] = 0;
        pWord->Base.bIsSpellCorr = (ET9U8)_wtoi(&sWordData[i]);
        i += 2;
        sWordData[i+3] = 0;
        pWord->bWordSrc = (ET9U8)_wtoi(&sWordData[i]);
        i += 4;
        sWordData[i+5] = 0;
        pWord->nTotFreq = (ET9U32)_wtoi(&sWordData[i]);
        i += 6;
        sWordData[i+5] = 0;
        pWord->wWordFreq = (ET9U16)_wtoi(&sWordData[i]);
        i += 6;
        sWordData[i+5] = 0;
        pWord->wTapFreq = (ET9U16)_wtoi(&sWordData[i]);
        i += 6;
        sWordData[i+3] = 0;
        pWord->bEditDistSpc = (ET9U8)_wtoi(&sWordData[i]);
        i += 4;
        sWordData[i+3] = 0;
        pWord->bEditDistStem = (ET9U8)_wtoi(&sWordData[i]);
        i += 4;
        sWordData[i+1] = 0;
        pWord->Base.bIsTerm = (ET9U8)_wtoi(&sWordData[i]);
        i += 2;
        if (pWord->Base.wSubstitutionLen) {
            int j = 0;
            while (sWordData[i] != L' ') {
                pWord->Base.sSubstitution[j++] = sWordData[i++];
            }
        }
    }
    return wStatus;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Pre dupe record word.
 *
 * @param pWord                     Pointer to word.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

static void ET9LOCALCALL __ET9PreDupeRecordWordInfo(ET9AWPrivWordInfo *pWord)
{
    ET9SYMB sWordData[200];
    ET9U8   holder;

    fwrite((void *)pWord->Base.sWord, sizeof(ET9SYMB), pWord->Base.wWordLen, pPreDupeFile);
    _snwprintf(sWordData, 200, L"   %02d %02d %02d %01d %03d %05d %05d %05d %03d %03d %01d ",
        pWord->Base.wWordLen, pWord->Base.wWordCompLen, pWord->Base.wSubstitutionLen, pWord->Base.bIsSpellCorr,
        pWord->bWordSrc, pWord->nTotFreq,  pWord->wWordFreq, pWord->wTapFreq,
        pWord->bEditDistSpc, pWord->bEditDistStem, pWord->Base.bIsTerm);
    fwrite((void *)sWordData, sizeof(ET9SYMB), (ET9U16)wcslen(sWordData), pPreDupeFile);
    if (pWord->Base.wSubstitutionLen) {
        fwrite((void *)pWord->Base.sSubstitution, sizeof(ET9SYMB), pWord->Base.wSubstitutionLen, pPreDupeFile);
    }

    if (!ISBASESRC(pWord->bWordSrc)) {
        if (ISEXACTSRC(pWord->bWordSrc)) {
            _snwprintf(sWordData, 200, L" [EXACT]");
        }
        else {
            _snwprintf(sWordData, 200, L" [EXACTISH]");
        }
        holder = GETBASESRC(pWord->bWordSrc);
        fwrite((void *)sWordData, sizeof(ET9SYMB), (ET9U16)wcslen(sWordData), pPreDupeFile);
    }
    else {
        holder = pWord->bWordSrc;
    }
    switch (holder) {
    case ET9WORDSRC_NONE:
        _snwprintf(sWordData, 200, L"");
        break;
    case ET9WORDSRC_CDB:
        _snwprintf(sWordData, 200, L" [CDB]");
        break;
    case ET9WORDSRC_RUDB:
        _snwprintf(sWordData, 200, L" [RUDB]");
        break;
    case ET9WORDSRC_QUDB:
        _snwprintf(sWordData, 200, L" [QUDB]");
        break;
    case ET9WORDSRC_MDB:
        _snwprintf(sWordData, 200, L" [MDB]");
        break;
    case ET9WORDSRC_ASDB_SHORTCUT:
        _snwprintf(sWordData, 200, L" [USER AS]");
        break;
    case ET9WORDSRC_LAS_SHORTCUT:
        _snwprintf(sWordData, 200, L" [LDB AS]");
        break;
    case ET9WORDSRC_LDB:
        _snwprintf(sWordData, 200, L" [LDB]");
        break;
    case ET9WORDSRC_LDB_PROMOTION:
        _snwprintf(sWordData, 200, L" [LDBP]");
        break;
    case ET9WORDSRC_BUILDAROUND_CDB:
        _snwprintf(sWordData, 200, L" [BA CDB]");
        break;
    case ET9WORDSRC_BUILDAROUND_RUDB:
        _snwprintf(sWordData, 200, L" [BA RUDB]");
        break;
    case ET9WORDSRC_BUILDAROUND_QUDB:
        _snwprintf(sWordData, 200, L" [BA QUDB]");
        break;
    case ET9WORDSRC_BUILDAROUND_MDB:
        _snwprintf(sWordData, 200, L" [BA MDB]");
        break;
    case ET9WORDSRC_BUILDAROUND_LDB:
        _snwprintf(sWordData, 200, L" [BA LDB]");
        break;
    case ET9WORDSRC_BUILDAROUND_KDB:
        _snwprintf(sWordData, 200, L" [BA KDB]");
        break;
    case ET9WORDSRC_BUILDCOMPOUND_CDB:
        _snwprintf(sWordData, 200, L" [BC CDB]");
        break;
    case ET9WORDSRC_BUILDCOMPOUND_RUDB:
        _snwprintf(sWordData, 200, L" [BC RUDB]");
        break;
    case ET9WORDSRC_BUILDCOMPOUND_QUDB:
        _snwprintf(sWordData, 200, L" [BC QUDB]");
        break;
    case ET9WORDSRC_BUILDCOMPOUND_MDB:
        _snwprintf(sWordData, 200, L" [BC MDB]");
        break;
    case ET9WORDSRC_BUILDCOMPOUND_LDB:
        _snwprintf(sWordData, 200, L" [BC LDB]");
        break;
    case ET9WORDSRC_BUILDCOMPOUND_KDB:
        _snwprintf(sWordData, 200, L" [BC KDB]");
        break;
    case ET9WORDSRC_TERMPUNCT:
        _snwprintf(sWordData, 200, L" [TERMPUNCT]");
        break;
    case ET9WORDSRC_COMPLETIONPUNCT:
        _snwprintf(sWordData, 200, L" [COMPLETIONPUNCT]");
        break;
    case ET9WORDSRC_MAGICSTRING:
        _snwprintf(sWordData, 200, L" [MAGICSTR]");
        break;
    case ET9WORDSRC_CAPTURE:
        _snwprintf(sWordData, 200, L" [CAPTURE]");
        break;
    case ET9WORDSRC_AUTOAPPEND:
        _snwprintf(sWordData, 200, L" [AUTOAPPEND]");
        break;
    default:
        _snwprintf(sWordData, 200, L" [0x%03X]", pWord->bWordSrc);
        break;
    }
    fwrite((void *)sWordData, sizeof(ET9SYMB), (ET9U16)wcslen(sWordData), pPreDupeFile);
    ++nPreDupeEntryCount;
}
#endif /* ET9_PRE_DUPE_SELLIST */

/*---------------------------------------------------------------------------*/
/** \internal
 * Do specific spellcheck build.
 *
 * @param pLingInfo         Pointer to alphabetic information structure.
 * @param wLdbNum           Language id.
 * @param pbTotalWords      Pointer to total words found.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

static ET9STATUS ET9LOCALCALL __ET9AWDoSCSelLstBuild(ET9AWLingInfo  * const pLingInfo,
                                                     const ET9U16           wLdbNum,
                                                     ET9U8          * const pbTotalWords)
{
    ET9U16              i;
    ET9U8               bDummy;
    ET9U8               bFound;
    ET9AWPrivWordInfo   *pWord;
    ET9AWPrivWordInfo   *pAmbigWord;
    ET9AWPrivWordInfo   *pExactWord = NULL;
    ET9SimpleWord       sSimpleWord;
    ET9AWPrivWordInfo   sExactWord;

    ET9AWLingCmnInfo    * const pLingCmnInfo = pLingInfo->pLingCmnInfo;
    ET9WordSymbInfo     * const pWordSymbInfo = pLingCmnInfo->Base.pWordSymbInfo;
    const ET9U8                 bNumSymbs = pWordSymbInfo->bNumSymbs;
    const ET9U16                wWordLen = bNumSymbs;

    const ET9ASLMODE            eSelectionListMode = pLingCmnInfo->Private.eSelectionListMode;
    const ET9ASLCORRECTIONMODE  eSelectionListCorrectionMode = pLingCmnInfo->Private.eSelectionListCorrectionMode;
    const ET9ASLMODE            eCurrSelectionListMode = pLingCmnInfo->Private.eCurrSelectionListMode;


    ET9AssertLog(pLingInfo != NULL);
    ET9AssertLog(pbTotalWords != NULL);

    *pbTotalWords = 0;
    pLingCmnInfo->Private.bHasRealWord = 0;
    pLingCmnInfo->Private.snLinSearchCount = 0;
    pLingCmnInfo->Private.wTotalWordInserts = 0;

    _ET9AWSelLstResetWordList(pLingInfo);

    pLingCmnInfo->Private.bTotalSymbInputs = bNumSymbs;

    /* update settings temporarily */

    pLingCmnInfo->Private.eSelectionListMode = ET9ASLMODE_COMPLETIONSPROMOTED;
    pLingCmnInfo->Private.eSelectionListCorrectionMode = ET9ASLCORRECTIONMODE_LOW;
    pLingCmnInfo->Private.eCurrSelectionListMode = ET9ASLMODE_COMPLETIONSPROMOTED;

    /* handle spc tracking */

    pLingCmnInfo->Private.bSpcDuringBuild = (ET9BOOL)(ET9_SPC_IS_ACTIVE(pLingCmnInfo->Private.ASpc.eMode) != 0);
    pLingCmnInfo->Private.bExpandAsDuringBuild = (ET9BOOL)(ET9EXPANDAUTOSUB(pLingCmnInfo) != 0);

    /* add exact match to list */

    if (!ET9GetExactWord(pWordSymbInfo, &sSimpleWord, pLingInfo->Private.pConvertSymb, pLingInfo->Private.pConvertSymbInfo)) {

        _ET9SimpleWordToPrivWord(&sSimpleWord, &sExactWord);

        sExactWord.bWordSrc  = ET9WORDSRC_NONE | EXACTOFFSET;
        sExactWord.xTotFreq  = 0xFFFF;
        sExactWord.xTapFreq  = 0xFFFF;
        sExactWord.xWordFreq = 1;
        sExactWord.Base.bIsTerm   = 1;

        pLingCmnInfo->Private.bStemsAllowed = 0;
        pLingCmnInfo->Private.wMaxWordLength = bNumSymbs;

        __ET9AWSelLstInsert(pLingInfo, &sExactWord, FREQ_NORMAL, 0);
    }

    _ET9AWSuppDBSelListBuild(pLingInfo,
                             0,
                             wWordLen,
                             &bDummy,
                             FREQ_NORMAL,
                             ET9SUPPDB_NONAS_SOURCES,
                             ET9ASPCMODE_EXACT);

    _ET9AWLdbWordsSearch(pLingInfo,
                         wLdbNum,
                         0,
                         wWordLen,
                         &bDummy,
                         FREQ_NORMAL,
                         ET9ASPCMODE_EXACT);

    if (ET9AWSys_GetBilingualSupported(pLingInfo)) {

        ET9U16 wAltLdbNum;

        if (wLdbNum == pLingCmnInfo->wFirstLdbNum) {
            wAltLdbNum = pLingCmnInfo->wSecondLdbNum;
        }
        else {
            wAltLdbNum = pLingCmnInfo->wFirstLdbNum;
        }

        _ET9AWLdbWordsSearch(pLingInfo,
                             wAltLdbNum,
                             0,
                             wWordLen,
                             &bDummy,
                             FREQ_NORMAL,
                             ET9ASPCMODE_EXACT);

    }

    _ET9AWSuppDBSelListBuild(pLingInfo,
                             0,
                             wWordLen,
                             &bDummy,
                             FREQ_NORMAL,
                             ET9SUPPDB_AS_SOURCES,
                             ET9ASPCMODE_EXACT);

    /* if no words at this point we are done... */

    if (!pLingCmnInfo->Private.bTotalWords) {

        pLingCmnInfo->Private.eSelectionListMode = eSelectionListMode;
        pLingCmnInfo->Private.eSelectionListCorrectionMode = eSelectionListCorrectionMode;
        pLingCmnInfo->Private.eCurrSelectionListMode = eCurrSelectionListMode;

        return ET9STATUS_NONE;
    }

    /* do post processing */

    pAmbigWord = pLingCmnInfo->Private.pWordList;
    bFound = 0;
    pWord = 0;
    for (i = pLingCmnInfo->Private.bTotalWords; i; --i, ++pAmbigWord) {
        if (!ISEXACTSRC(pAmbigWord->bWordSrc)) {
            if (__ET9AWIsLikeExactMatch(pLingInfo, pAmbigWord)) {
                if (!pWord || __PriorityCompare(pLingCmnInfo, pAmbigWord, pWord, 0) > 0) {
                    pWord = pAmbigWord;
                }
            }
            else {
                bFound = 1;
            }
        }
        else {
            pExactWord = pAmbigWord;
        }
    }

    if (pExactWord && ISGENUINESRC(pExactWord->bWordSrc)) {
        pExactWord->bWordSrc = GETBASESRC(pExactWord->bWordSrc);
        pExactWord->bWordSrc |= EXACTOFFSET;
    }
    if (pWord && bFound) {
        pWord->bWordSrc |= EXACTISHOFFSET;
    }

    /* before sorting:
        disposable quality words goes into the stem pool (e.g. RDB words, not UDB words)
        stem quality words goes (back) into the stem pool (can have changed source during build) */

    pAmbigWord = pLingCmnInfo->Private.pWordList;
    for (i = pLingCmnInfo->Private.bTotalWords; i; --i, ++pAmbigWord) {
        if (pAmbigWord->bWordQuality <= STEM_QUALITY) {
            pAmbigWord->bWordSrc = ET9WORDSRC_STEMPOOL;
        }
    }

    /* sort */

    __ET9AWSortPriorityList(pLingCmnInfo);

    pLingCmnInfo->Private.bDefaultIndex = 0;

    /* result info */

    *pbTotalWords = pLingCmnInfo->Private.bTotalWords;

    /* restore settings */

    pLingCmnInfo->Private.eSelectionListMode = eSelectionListMode;
    pLingCmnInfo->Private.eSelectionListCorrectionMode = eSelectionListCorrectionMode;
    pLingCmnInfo->Private.eCurrSelectionListMode = eCurrSelectionListMode;

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Do spellcheck build.
 *
 * @param pLingInfo             Pointer to alphabetic information structure.
 * @param wLdbNum               Language id.
 * @param pbTotalWords          Pointer to total words found.
 * @param pbDefaultListIndex    Pointer to default list index.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL _ET9ASpellCheckSelLstBuild(ET9AWLingInfo * const pLingInfo,
                                                const ET9U16          wLdbNum,
                                                ET9U8 * const         pbTotalWords,
                                                ET9U8 * const         pbDefaultListIndex)
{
    ET9STATUS       wStatus;

    ET9AWLingCmnInfo * const pLingCmnInfo = pLingInfo->pLingCmnInfo;

    ET9AssertLog(pLingInfo != NULL);
    ET9AssertLog(pbTotalWords != NULL);
    ET9AssertLog(pbDefaultListIndex != NULL);

    WLOG2(fprintf(pLogFile2, "_ET9ASpellCheckSelLstBuild\n");)

    if (pLingCmnInfo->Base.pWordSymbInfo->bNumSymbs > ET9MAXWORDSIZE) {
        return ET9STATUS_ERROR;
    }

    pLingCmnInfo->Base.bSymbsInfoInvalidated = 0;
    wStatus = __ET9AWDoSCSelLstBuild(pLingInfo, wLdbNum, pbTotalWords);
    pLingCmnInfo->Private.bLastBuildLen = pLingCmnInfo->Base.pWordSymbInfo->bNumSymbs;
    pLingCmnInfo->Private.dwStateBits |= ET9SLEXACTINLIST;
    *pbDefaultListIndex = pLingCmnInfo->Private.bDefaultIndex;

    if (!*pbTotalWords) {
        wStatus = ET9STATUS_NO_MATCHING_WORDS;
    }

    return wStatus;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Calculate a checksum for the words in the selectionlist.
 *
 * @param pLingInfo                 Pointer to alphabetic information structure.
 *
 * @return Checksum
 */

static ET9U32 ET9LOCALCALL __ET9AWCalculateWordListChecksum(ET9AWLingInfo * const pLingInfo)
{
    ET9U8               bIndex;
    ET9SYMB             *psSymb;
    ET9U16              wCount;
    ET9U32              dwHashValue = 0;
    ET9AWWordInfo       *pWord;

    ET9AssertLog(pLingInfo != NULL);

    for (bIndex = 0; bIndex < pLingInfo->pLingCmnInfo->Private.bTotalWords; ++bIndex) {

        if (ET9AWSelLstGetWord(pLingInfo, &pWord, bIndex) != ET9STATUS_NONE) {
            continue;
        }

        psSymb = pWord->sWord;

        for (wCount = pWord->wWordLen; wCount; --wCount, ++psSymb) {
            dwHashValue = *psSymb + (65599 * dwHashValue);
        }

        psSymb = pWord->sSubstitution;

        for (wCount = pWord->wSubstitutionLen; wCount; --wCount, ++psSymb) {
            dwHashValue = *psSymb + (65599 * dwHashValue);
        }
    }

    return dwHashValue;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Remove duplicates from the selection list.
 *
 * @param pLingInfo                 Pointer to alphabetic information structure.
 *
 * @return None
 */

static void ET9LOCALCALL __ET9AWRemoveDuplicates(ET9AWLingInfo * const pLingInfo)
{
    ET9U8               bLook;
    ET9U8               bIndex;
    ET9AWPrivWordInfo   *pWord1;
    ET9AWPrivWordInfo   *pWord2;

    ET9AWLingCmnInfo * const pLingCmnInfo = pLingInfo->pLingCmnInfo;

    ET9AssertLog(pLingInfo != NULL);

    /* remove initial words with length zero */

    while (pLingCmnInfo->Private.bTotalWords) {

        pWord1 = &pLingCmnInfo->Private.pWordList[pLingCmnInfo->Private.bWordList[0]];

        if (pWord1->Base.wWordLen) {
            break;
        }

        __RemoveWord(pLingCmnInfo, 0);
    }

    /* remove duplicates */

    for (bIndex = 0; bIndex + 1 < pLingCmnInfo->Private.bTotalWords; ++bIndex) {

        pWord1 = &pLingCmnInfo->Private.pWordList[pLingCmnInfo->Private.bWordList[bIndex]];

        for (bLook = bIndex + 1; bLook < pLingCmnInfo->Private.bTotalWords; ++bLook) {

            ET9BOOL bRemove = 0;    /* keep here */

            pWord2 = &pLingCmnInfo->Private.pWordList[pLingCmnInfo->Private.bWordList[bLook]];

            if (!pWord2->Base.wWordLen) {
                bRemove = 1;
            }
            else if (pWord1->Base.wWordLen == pWord2->Base.wWordLen) {

                /* keep declarations here */

                ET9U16  wCmpLen = pWord1->Base.wWordLen;
                ET9SYMB *pStr1 = pWord1->Base.sWord;
                ET9SYMB *pStr2 = pWord2->Base.sWord;

                ++wCmpLen;
                while (--wCmpLen) {
                    if (*pStr1++ != *pStr2++) {
                        break;
                    }
                }

                if (!wCmpLen) {
                    bRemove = 1;
                }
            }

            if (bRemove) {

                if (!pWord1->Base.wSubstitutionLen && pWord2->Base.wSubstitutionLen) {

                    _ET9SymCopy(pWord1->Base.sSubstitution, pWord2->Base.sSubstitution, pWord2->Base.wSubstitutionLen);

                    pWord1->Base.wSubstitutionLen = pWord2->Base.wSubstitutionLen;
                }

                __RemoveWord(pLingCmnInfo, bLook);

                --bLook;        /* need to revisit this position */
            }
        }
    }
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Update a word to a new post shift state.
 *
 * @param pLingInfo                 Pointer to alphabetic information structure.
 * @param eMode                     Post shift mode to set.
 * @param pWord                     Word to be updated.
 *
 * @return None
 */

static void ET9LOCALCALL __ET9AWPostShiftWord(ET9AWLingInfo         * const pLingInfo,
                                              const ET9POSTSHIFTMODE        eMode,
                                              ET9AWWordInfo         * const pWord)
{
    ET9BOOL     bFirst;
    ET9STATUS   wStatus;
    ET9U16      wCount;
    ET9SYMB     *psSymb;
    ET9U16      wLdbNum;

    ET9AssertLog(pLingInfo != NULL);
    ET9AssertLog(pWord != NULL);

    /* handle shift change */

    bFirst = 1;
    wCount = pWord->wWordLen;
    psSymb = pWord->sWord;
    wLdbNum  = (pWord->bLangIndex == ET9AWSECOND_LANGUAGE) ? pLingInfo->pLingCmnInfo->wSecondLdbNum : pLingInfo->pLingCmnInfo->wFirstLdbNum;

    for (; wCount; --wCount, ++psSymb, bFirst = 0) {

        if (eMode == ET9POSTSHIFTMODE_UPPER || eMode == ET9POSTSHIFTMODE_INITIAL && bFirst) {
            *psSymb = _ET9SymToUpper(*psSymb, wLdbNum);
        }
        else {
            *psSymb = _ET9SymToLower(*psSymb, wLdbNum);
        }
    }

    bFirst = 1;
    wCount = pWord->wSubstitutionLen;
    psSymb = pWord->sSubstitution;
    for (; wCount; --wCount, ++psSymb, bFirst = 0) {

        if (eMode == ET9POSTSHIFTMODE_UPPER || eMode == ET9POSTSHIFTMODE_INITIAL && bFirst) {
            *psSymb = _ET9SymToUpper(*psSymb, wLdbNum);
        }
        else {
            *psSymb = _ET9SymToLower(*psSymb, wLdbNum);
        }
    }

    /* handle OTFM */

    wStatus = _ET9_ConvertBuildBuf(pLingInfo, pWord);

    if (wStatus == ET9STATUS_NO_CHAR || wStatus == ET9STATUS_ERROR) {
        pWord->wWordLen = 0;
    }
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Update the selection list to a new post shift state.
 *
 * @param pLingInfo                 Pointer to alphabetic information structure.
 * @param eMode                     Post shift mode to set.
 * @param pbTotalWords              (out) number of candidate words found.
 * @param pbCurrListIndex           (in/out) current active candidate.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

static ET9STATUS ET9LOCALCALL __ET9AWPostShift(ET9AWLingInfo            * const pLingInfo,
                                               const ET9POSTSHIFTMODE           eMode,
                                               ET9U8                    * const pbTotalWords,
                                               ET9U8                    * const pbCurrListIndex)
{
    ET9U8               bIndex;
    ET9STATUS           wStatus;
    ET9AWWordInfo       *pWord;
    ET9AWWordInfo       sActiveWord;
    ET9AWPrivWordInfo   *pWord1;

    ET9AWLingCmnInfo * const pLingCmnInfo = pLingInfo->pLingCmnInfo;

    ET9AssertLog(pLingInfo != NULL);
    ET9AssertLog(pbTotalWords != NULL);
    ET9AssertLog(pbCurrListIndex != NULL);

    /* record the active word */

    wStatus = ET9AWSelLstGetWord(pLingInfo, &pWord, *pbCurrListIndex);

    if (wStatus) {
        return wStatus;
    }

    sActiveWord = *pWord;

    /* update the list */

    if (eMode == ET9POSTSHIFTMODE_DEFAULT) {

        wStatus = __ET9AWDoSelLstBuild(pLingInfo, pbTotalWords, 0);

        if (wStatus) {
            return wStatus;
        }
    }
    else {

        for (bIndex = 0; bIndex < pLingCmnInfo->Private.bTotalWords; ++bIndex) {

            wStatus = ET9AWSelLstGetWord(pLingInfo, &pWord, bIndex);

            if (wStatus) {
                return wStatus;
            }

            __ET9AWPostShiftWord(pLingInfo, eMode, pWord);
        }

        __ET9AWRemoveDuplicates(pLingInfo);

        *pbTotalWords = pLingCmnInfo->Private.bTotalWords;
    }

    /* find the active word again */

    if (*pbCurrListIndex >= pLingCmnInfo->Private.bTotalWords) {
        *pbCurrListIndex = (ET9U8)(pLingCmnInfo->Private.bTotalWords - 1);
    }

    for (bIndex = 0; bIndex + 1 < pLingCmnInfo->Private.bTotalWords; ++bIndex) {

        pWord1 = &pLingCmnInfo->Private.pWordList[pLingCmnInfo->Private.bWordList[bIndex]];

        if (pWord1->Base.wWordLen == sActiveWord.wWordLen) {

            /* keep declarations here */

            ET9U16  wCmpLen = pWord1->Base.wWordLen;
            ET9SYMB *pStr1 = pWord1->Base.sWord;
            ET9SYMB *pStr2 = sActiveWord.sWord;
            ET9U16  wLdbNum  = (pWord1->Base.bLangIndex == ET9AWSECOND_LANGUAGE) ? pLingCmnInfo->wSecondLdbNum : pLingCmnInfo->wFirstLdbNum;
            ET9U16  wLdbNum2  = (sActiveWord.bLangIndex == ET9AWSECOND_LANGUAGE) ? pLingCmnInfo->wSecondLdbNum : pLingCmnInfo->wFirstLdbNum;

            ++wCmpLen;
            while (--wCmpLen) {
                if (_ET9SymToLower(*pStr1++, wLdbNum) != _ET9SymToLower(*pStr2++, wLdbNum2)) {
                    break;
                }
            }

            if (!wCmpLen) {
                *pbCurrListIndex = bIndex;
                break;
            }
        }
    }

    /* save new shift mode */

    pLingCmnInfo->Base.pWordSymbInfo->Private.eCurrPostShiftMode = eMode;

    /* done */

    __LogPartialSelList(pLingInfo->pLingCmnInfo, 0, (ET9U8)(pLingCmnInfo->Private.bTotalWords - 1), "After Post Shift OP", pLogFile2);

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/**
 * Do post shift.
 * Setting shift mode "next" will move to the next post shift mode of lower, initial, upper and default.
 * If the next'ed list didn't change anything in the list it will move on to the next etc until there is
 * a change or the modes are exhausted.
 *
 * @param pLingInfo                 Pointer to alphabetic information structure.
 * @param eMode                     Post shift mode to set.
 * @param pbTotalWords              (out) number of candidate words found.
 * @param pbCurrListIndex           (in/out) current active candidate.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL ET9AWSelLstPostShift(ET9AWLingInfo * const   pLingInfo,
                                          const ET9POSTSHIFTMODE  eMode,
                                          ET9U8         * const   pbTotalWords,
                                          ET9U8         * const   pbCurrListIndex)
{
    ET9STATUS           wStatus;
    ET9AWLingCmnInfo    *pLingCmnInfo;

    if ((wStatus = _ET9AWSys_BasicValidityCheck(pLingInfo)) != ET9STATUS_NONE) {
        return wStatus;
    }

    pLingCmnInfo = pLingInfo->pLingCmnInfo;
    if (pLingCmnInfo->Base.pWordSymbInfo == NULL || pLingCmnInfo->Base.pWordSymbInfo->wInitOK != ET9GOODSETUP) {
        return ET9STATUS_INVALID_MEMORY;
    }

    /* user postshifting deactivates auto-capping AND downshifting logic */

    pLingCmnInfo->Base.pWordSymbInfo->Private.bCompoundingDownshift = 0;
    pLingCmnInfo->Base.pWordSymbInfo->Private.eAutocapWord = ET9AUTOCAP_OFF;

    if (pbTotalWords == NULL || pbCurrListIndex == NULL) {
        return ET9STATUS_INVALID_MEMORY;
    }
    if (pLingCmnInfo->Base.bSymbsInfoInvalidated) {
        return ET9STATUS_NEED_SELLIST_BUILD;
    }
    if (!pLingCmnInfo->Private.bTotalWords) {
        return ET9STATUS_NEED_SELLIST_BUILD;
    }
    if (eMode >= ET9POSTSHIFTMODE_LAST) {
        return ET9STATUS_BAD_PARAM;
    }

    WLOG2(fprintf(pLogFile2, "Post Shift - mode %d\n", eMode);)


    /* handle explicit modes */

    if (eMode != ET9POSTSHIFTMODE_NEXT) {
        return __ET9AWPostShift(pLingInfo, eMode, pbTotalWords, pbCurrListIndex);
    }

    /* handle mode "next" */

    {
        ET9INT              nModeCount;
        ET9U32              dwNewCheckSum;
        const ET9U32        dwOldCheckSum = __ET9AWCalculateWordListChecksum(pLingInfo);
        ET9POSTSHIFTMODE    eCurrMode = pLingCmnInfo->Base.pWordSymbInfo->Private.eCurrPostShiftMode;

        for (nModeCount = 4; nModeCount; --nModeCount) {

            switch (eCurrMode) {
            case ET9POSTSHIFTMODE_LOWER:
                eCurrMode = ET9POSTSHIFTMODE_INITIAL;
                break;
            case ET9POSTSHIFTMODE_INITIAL:
                eCurrMode = ET9POSTSHIFTMODE_UPPER;
                break;
            case ET9POSTSHIFTMODE_UPPER:
                eCurrMode = ET9POSTSHIFTMODE_DEFAULT;
                break;
            case ET9POSTSHIFTMODE_DEFAULT:
                eCurrMode = ET9POSTSHIFTMODE_LOWER;
                break;
            default:
                ET9AssertLog(0);
                break;
            }

            if ((wStatus = __ET9AWPostShift(pLingInfo, eCurrMode, pbTotalWords, pbCurrListIndex)) != ET9STATUS_NONE) {
                return wStatus;
            }

            dwNewCheckSum = __ET9AWCalculateWordListChecksum(pLingInfo);

            WLOG2(fprintf(pLogFile2, "Post Shift - old checksum = %x, new checksum = %x\n", dwOldCheckSum, dwNewCheckSum);)

            if (dwNewCheckSum != dwOldCheckSum) {
                break;
            }

            WLOG2(fprintf(pLogFile2, "Post Shift - trying next mode\n");)
        }
    }

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/**
 * Get post shift mode.
 *
 * @param pLingInfo                 Pointer to alphabetic information structure.
 * @param peMode                    (out) current post shift mode.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL ET9AWGetPostShiftMode(ET9AWLingInfo    * const pLingInfo,
                                           ET9POSTSHIFTMODE * const peMode)
{
    ET9STATUS           wStatus;

    if ((wStatus = _ET9AWSys_BasicValidityCheck(pLingInfo)) != ET9STATUS_NONE) {
        return wStatus;
    }

    if (!peMode) {
        return ET9STATUS_INVALID_MEMORY;
    }

    {
        ET9AWLingCmnInfo * const pLingCmnInfo = pLingInfo->pLingCmnInfo;

        if (!pLingCmnInfo->Base.pWordSymbInfo || pLingCmnInfo->Base.pWordSymbInfo->wInitOK != ET9GOODSETUP) {
            return ET9STATUS_INVALID_MEMORY;
        }

        *peMode = pLingCmnInfo->Base.pWordSymbInfo->Private.eCurrPostShiftMode;

        if (pLingCmnInfo->Base.bSymbsInfoInvalidated || !pLingCmnInfo->Private.bTotalWords) {
            return ET9STATUS_NEED_SELLIST_BUILD;
        }
    }

    return ET9STATUS_NONE;
}

#endif /* ET9_ALPHABETIC_MODULE */
/*! @} */
/* ----------------------------------< eof >--------------------------------- */
