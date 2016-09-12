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
;**     FileName: et9aslst.h                                                  **
;**                                                                           **
;**  Description: Alphabetic selection list routines header file.             **
;**                                                                           **
;*******************************************************************************
;******************************************************************************/

#ifndef ET9ASLST_H
#define ET9ASLST_H    1

#include "et9api.h"

/* don't mangle the function name if compile under C++ */
#if defined(__cplusplus)
extern "C" {
#endif

#ifdef ET9_PRE_DUPE_SELLIST
#include <stdio.h>
#include <wchar.h>
#endif

#define ISDISPOSEWRD(pw)    (GETBASESRC((pw)->bWordSrc) == ET9WORDSRC_STEMPOOL && ((pw)->bWordQuality <= DISPOSABLE_QUALITY || (pw)->bHasPrimEditDist))

#define GETBASESRC(x)           (ET9WORDSRC)((x) & 0x3F)
#define GETCOMPSRC(x)           ((ET9WORDSRC)(ISBASESRC(x) ? (x) : (ISEXACTSRC(x)  ? ET9WORDSRC_EXACT : ET9WORDSRC_EXACTISH)))
#define GETRAWSRC(x)            (ET9WORDSRC)(ISBUILDAROUNDSRC(x) ? (GETBASESRC(x) - BUILDAROUND_SRC_ADDON) : ISCOMPOUNDSRC(x) ? (GETBASESRC(x) - BUILDCOMPOUND_SRC_ADDON) : GETBASESRC(x))
#define ISBASESRC(x)            (!((x) & 0xC0))
#define ISEXACTSRC(x)           ((x) & EXACTOFFSET)
#define ISEXACTISHSRC(x)        ((x) & EXACTISHOFFSET)
#define ISREQUIREDSRC(x)        (GETBASESRC(x) == ET9WORDSRC_REQUIRED)
#define ISREALSRC(x)            (GETBASESRC(x) != ET9WORDSRC_NONE && GETBASESRC(x) != ET9WORDSRC_AUTOAPPEND && GETBASESRC(x) != ET9WORDSRC_BUILDAPPEND && GETBASESRC(x) != ET9WORDSRC_STEM && GETBASESRC(x) != ET9WORDSRC_MAGICSTRING)
#define ISPUNCTSRC(x)           (GETBASESRC(x) == ET9WORDSRC_TERMPUNCT || GETBASESRC(x) == ET9WORDSRC_COMPLETIONPUNCT)
#define ISCOMPOUNDSRC(x)        (GETBASESRC(x) > ET9WORDSRC_BUILDCOMPOUND_START && GETBASESRC(x) < ET9WORDSRC_MISC_START)
#define ISBUILDAROUNDSRC(x)     (GETBASESRC(x) > ET9WORDSRC_BUILDAROUND_START && GETBASESRC(x) < ET9WORDSRC_BUILDCOMPOUND_START || GETBASESRC(x) == ET9WORDSRC_BUILDAPPENDPUNCT)
#define ISCAPTURECANDSRC(x)     (!ISPUNCTSRC(x) && !ISBUILDAROUNDSRC(x) && GETBASESRC(x) != ET9WORDSRC_AUTOAPPEND && GETBASESRC(x) != ET9WORDSRC_BUILDAPPEND && GETBASESRC(x) != ET9WORDSRC_STEM && GETBASESRC(x) != ET9WORDSRC_MAGICSTRING)
#define ISALTCAPTURECANDSRC(x)  (!ISPUNCTSRC(x) && GETBASESRC(x) != ET9WORDSRC_AUTOAPPEND && GETBASESRC(x) != ET9WORDSRC_STEM && GETBASESRC(x) != ET9WORDSRC_MAGICSTRING)
#define ISGENUINESRC(x)         (ISREALSRC(x) && (GETBASESRC(x) < ET9WORDSRC_BUILDAROUND_START || (x) == ET9WORDSRC_BUILDAROUND_LAS))
#define ISBUILDABLESRC(x)       (ISGENUINESRC(x) && GETBASESRC(x) != ET9WORDSRC_KDB)
#define ISPROTECTEDSRC(x)       (GETBASESRC(x) == ET9WORDSRC_AUTOAPPEND || GETBASESRC(x) == ET9WORDSRC_BUILDAPPEND || GETBASESRC(x) == ET9WORDSRC_STEM || GETBASESRC(x) == ET9WORDSRC_MAGICSTRING)
#define ISSPOTSRC(x)            (GETBASESRC(x) == ET9WORDSRC_STEMPOOL)
#define ISDEMOTESRC(x)          (GETRAWSRC(x) == ET9WORDSRC_LDB_PROMOTION || GETRAWSRC(x) == ET9WORDSRC_MDB || (GETRAWSRC(x) == ET9WORDSRC_LAS_SHORTCUT && (x) != ET9WORDSRC_BUILDAROUND_LAS))
#define ISNONOVERRIDESRC(x)     (GETBASESRC(x) == ET9WORDSRC_QUDB)

#define BUILDAROUND_SRC_ADDON   (ET9WORDSRC_BUILDAROUND_CDB - ET9WORDSRC_CDB)
#define BUILDCOMPOUND_SRC_ADDON (ET9WORDSRC_BUILDCOMPOUND_CDB - ET9WORDSRC_CDB)
#define LDBPROMOTION_SRC_ADDON  (ET9WORDSRC_LDB_PROMOTION - ET9WORDSRC_LDB)

#define ADDON_FROM_FREQ_IND(freqInd,wordSrc)        \
    (freqInd == FREQ_BUILDAROUND) ?                 \
        (wordSrc + BUILDAROUND_SRC_ADDON) :         \
        ((freqInd == FREQ_BUILDCOMPOUND) ?          \
            (wordSrc + BUILDCOMPOUND_SRC_ADDON) :   \
            wordSrc)                                \

/* Given b = lower byte, a = upper byte; compose a word integral (ET9U16). */
#define ET9MAKEWORD(a, b)  ((ET9U16)(((ET9U8)(b)) | ((ET9U16)((ET9U8)(a))) << 8) )
#define ET9LOBYTE(w)       ((ET9U8)(w))
#define ET9HIBYTE(w)       ((ET9U8)(((ET9U16)(w) >> 8)))

/*#define STWORDTYPE_OTHERCASE   128 */

typedef enum ET9_FREQ_DESIGNATION_e {
    FREQ_NORMAL,
    FREQ_BUILDAROUND,
    FREQ_BUILDCOMPOUND,
    FREQ_BUILDSPACE,
    FREQ_TERM_PUNCT

} ET9_FREQ_DESIGNATION;

ET9STATUS ET9FARCALL _ET9AWSelLstWordMatch( ET9AWLingInfo           * const pLingInfo,
                                            ET9AWPrivWordInfo       * const pWord,
                                            const ET9U16                    wIndex,
                                            const ET9U16                    wLength,
                                            ET9U8                   *       pbFound);

ET9STATUS ET9FARCALL _ET9AWSelLstWordSearch(ET9AWLingInfo       * const pLingInfo,
                                            ET9AWPrivWordInfo   * const pWord,
                                            const ET9U16                wIndex,
                                            const ET9U16                wLength,
                                            const ET9_FREQ_DESIGNATION  bFreqIndicator);

ET9STATUS ET9FARCALL _ET9AWSelLstAdd(ET9AWLingInfo          * const pLingInfo,
                                     ET9AWPrivWordInfo      * const pWord,
                                     const ET9U16                   wLength,
                                     const ET9_FREQ_DESIGNATION     bFreqIndicator);

ET9STATUS ET9FARCALL _ET9AWSelLstStripActualTaps(ET9AWPrivWordInfo * const pSelWord);

ET9STATUS ET9FARCALL _ET9AWSelLstWordPreAdd(ET9AWLingInfo       * const pLingInfo,
                                            ET9AWPrivWordInfo   * const pWord,
                                            const ET9U16                wFirstTap,
                                            const ET9U8                 bInputLength);

#ifdef ET9_PRE_DUPE_SELLIST
extern const char DEFAULT_PREDUPE_FILE_NAME[];
extern ET9UINT nPreDupeEntryCount;
ET9UINT ET9FARCALL ET9PreDupeGetSelListWordCount();
ET9STATUS ET9FARCALL ET9PreDupeGetWord(ET9AWPrivWordInfo *pWord, ET9UINT nIndex);
#endif

ET9STATUS ET9FARCALL _ET9ASpellCheckSelLstBuild(ET9AWLingInfo   * const pLingInfo,
                                                const ET9U16            wLdbNum,
                                                ET9U8           * const pbTotalWords,
                                                ET9U8           * const pbDefaultListIndex);


/* End don't mangle the function name if compile under C++ */
#if defined (__cplusplus)
    }
#endif

#endif /* !ET9ASLST_H */

/* ----------------------------------< eof >--------------------------------- */
