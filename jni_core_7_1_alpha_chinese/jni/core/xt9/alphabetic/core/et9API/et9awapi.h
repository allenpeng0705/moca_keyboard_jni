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
;**                                                                           **
;*******************************************************************************
;**                                                                           **
;**     FileName: et9awapi.h                                                  **
;**                                                                           **
;**  Description: ET9 Alphabetic API Interface Header File.                   **
;**               Conforming to the development version of ET9                **
;**                                                                           **
;*******************************************************************************
;******************************************************************************/

#ifndef ET9AWAPI_H
#define ET9AWAPI_H 1

/*! \addtogroup et9awapi API for alphabetic XT9
* The API for alphabetic XT9.
* @{
*/

/* don't mangle the function name if compile under C++ */
#if defined(__cplusplus)
extern "C" {
#endif

#ifndef ET9API_H
#error This file can only be included through et9api.h
#endif

#ifdef ET9_ALPHABETIC_MODULE


/******************************************************************************/

/* Define ET9 compatibility index for alpha core and LDB */

#define ET9COMPATIDLDBXBASEALPH     1
#define ET9COMPATIDLDBXOFFSETALPH   0

/*------------------------------------------------------------------------
 *  Define ET9 compatibility index related macros.
 *------------------------------------------------------------------------*/

/** \internal
 * max number of offsets each core can be compatible with (LDB)
 */

#define ET9MAXCOMPATIDLDBXOFFSET    16

/*------------------------------------------------------------------------
 *  Define ET9 Alphabetic selection list defines
 *------------------------------------------------------------------------*/

#ifdef ET9MAXSELLISTSIZE
#error ET9MAXSELLISTSIZE is not a configuration option, ET9AWSysInit sets the actual size
#endif

#define ET9MINSELLISTSIZE   16              /**< min selection list size */
#define ET9MAXSELLISTSIZE   64              /**< max selection list size, for internal memory sizing */


#define ET9WORDSTEMS_MODE(pLingCmnInfo)                     ( (pLingCmnInfo)->Private.bStateWordStems                  )
#define ET9WORDCOMPLETION_MODE(pLingCmnInfo)                ( (pLingCmnInfo)->Private.bStateWordCompletion             )
#define ET9NEXTWORDPREDICTION_MODE(pLingCmnInfo)            ( (pLingCmnInfo)->Private.bStateNextWordPrediction         )
#define ET9EXACTINLIST(pLingCmnInfo)                        ( (pLingCmnInfo)->Private.bStateExactInList                )
#define ET9EXACTLAST(pLingCmnInfo)                          ( (pLingCmnInfo)->Private.bStateExactLast                  )
#define ET9USERDEFINEDAUTOSUBENABLED(pLingCmnInfo)          ( (pLingCmnInfo)->Private.bStateUserDefinedAutosub         )
#define ET9LDBSUPPORTEDAUTOSUBENABLED(pLingCmnInfo)         ( (pLingCmnInfo)->Private.bStateLDBSupportedAutosub        )
#define ET9AUTOAPPENDINLIST(pLingCmnInfo)                   ( (pLingCmnInfo)->Private.bStateAutoAppendInList           )
#define ET9QUDBSUPPORTENABLED(pLingCmnInfo)                 ( (pLingCmnInfo)->Private.bStateQUDBSupportEnabled         )
#define ET9LDBENABLED(pLingCmnInfo)                         ( (pLingCmnInfo)->Private.bStateLDBEnabled                 )
#define ET9CDBENABLED(pLingCmnInfo)                         ( (pLingCmnInfo)->Private.bStateCDBEnabled                 )
#define ET9RUDBENABLED(pLingCmnInfo)                        ( (pLingCmnInfo)->Private.bStateRUDBEnabled                )
#define ET9ASDBENABLED(pLingCmnInfo)                        ( (pLingCmnInfo)->Private.bStateASDBEnabled                )
#define ET9MDBENABLED(pLingCmnInfo)                         ( (pLingCmnInfo)->Private.bStateMDBEnabled                 )
#define ET9DOWNSHIFTDEFAULTENABLED(pLingCmnInfo)            ( (pLingCmnInfo)->Private.bStateDownShiftDefault           )
#define ET9DOWNSHIFTALLLDB(pLingCmnInfo)                    ( (pLingCmnInfo)->Private.bStateDownShiftAllLDB            )
#define ET9EXACTISDEFAULT(pLingCmnInfo)                     ( (pLingCmnInfo)->Private.bStateExactIsDefault             )
#define ET9ACTIVELANGSWITCHENABLED(pLingCmnInfo)            ( (pLingCmnInfo)->Private.bStateActiveLangSwitch           )
#define ET9INACTIVELANGSPELLCORRECTENABLED(pLingCmnInfo)    ( (pLingCmnInfo)->Private.bStateInactiveLangSpellCorrect   )
#define ET9POSTSORTENABLED(pLingCmnInfo)                    ( (pLingCmnInfo)->Private.bStatePostSort                   )
#define ET9LMENABLED(pLingCmnInfo)                          ( (pLingCmnInfo)->Private.bStateLM                         )
#define ET9BOOSTTOPCANDIDATE(pLingCmnInfo)                  ( (pLingCmnInfo)->Private.bStateBoostTopCandidate          )
#define ET9EXPANDAUTOSUB(pLingCmnInfo)                      ( (pLingCmnInfo)->Private.bStateExpandAutosub              )
#define ET9AUTOSPACE(pLingCmnInfo)                          ( (pLingCmnInfo)->Private.bStateAutoSpace                  )


#define ET9STATELDBENABLEDMASK          (1L << 0)
#define ET9STATECDBENABLEDMASK          (1L << 1)
#define ET9STATERUDBENABLEDMASK         (1L << 2)
#define ET9STATEASDBENABLEDMASK         (1L << 3)
#define ET9STATEMDBENABLEDMASK          (1L << 4)


#define ET9ALLDBMASK     ((ET9U32)(ET9STATELDBENABLEDMASK  |    \
                                   ET9STATECDBENABLEDMASK  |    \
                                   ET9STATERUDBENABLEDMASK |    \
                                   ET9STATEASDBENABLEDMASK |    \
                                   ET9STATEMDBENABLEDMASK))

/**
 * Public word source values
 */

typedef enum ET9AWORDSOURCE_e {
    ET9AWORDSOURCE_CUSTOM = 1,          /**<  1 - from the user/custom dictionary */
    ET9AWORDSOURCE_LDB,                 /**<  2 - from the LDB */
    ET9AWORDSOURCE_MDB,                 /**<  3 - from the MDB */
    ET9AWORDSOURCE_CDB,                 /**<  4 - from the CDB */
    ET9AWORDSOURCE_ASDB,                /**<  5 - from auto substitution */
    ET9AWORDSOURCE_STEM,                /**<  6 - from stem */
    ET9AWORDSOURCE_AUTOAPPEND,          /**<  7 - from autoappend */
    ET9AWORDSOURCE_TERMPUNCT,           /**<  8 - from terminal punctuation */
    ET9AWORDSOURCE_CONSTRUCTED,         /**<  9 - flush, buildaround, compound, etc */
    ET9AWORDSOURCE_NEWWORD              /**< 10 - not buildable by the core */

} ET9AWORDSOURCE;

/**
 * Public word info object
 */

typedef struct ET9AWWordInfo_s {
    ET9U16          wWordLen;                               /**< total word length */
    ET9U16          wWordCompLen;                           /**< length of the completion part */
    ET9U16          wSubstitutionLen;                       /**< total word substitution length (zero means none) */
    ET9BOOL         bIsTerm;                                /**< if word is a (real) term, not a stem */
    ET9BOOL         bIsSpellCorr;                           /**< if the word is the result of spell correction */
    ET9U8           bWordSource;                            /**< word source */
    ET9U8           bLangIndex;                             /**< language index */
    /* keep arrays last */
    ET9SYMB         sWord[ET9MAXWORDSIZE];                  /**< word string (not terminated) */
    ET9SYMB         sSubstitution[ET9MAXSUBSTITUTIONSIZE];  /**< substitution string (not terminated) */
} ET9AWWordInfo;

/** \internal
 * Private word info object
 */

typedef struct ET9AWPrivWordInfo_s {
    /* keep base struct first, for casting */
    ET9AWWordInfo Base;             /**< \internal the public candidate info */
    /* private attributes */
    ET9FREQ       xTotFreq;         /**< \internal total frequency */
    ET9U32        dwWordIndex;      /**< \internal word index */
    ET9FREQPART   xWordFreq;        /**< \internal word frequency */
    ET9U16        wTWordFreq;       /**< \internal word stateful frequency */
    ET9U16        wEWordFreq;       /**< \internal word stateless frequency */
    ET9FREQPART   xTapFreq;         /**< \internal tap frequency */
    ET9U8         bWordSrc;         /**< \internal word source */
    ET9U8         bWordDesignation; /**< \internal word freq designation */
    ET9U8         bWordQuality;     /**< \internal word quality */
    ET9U8         bEditDistSpc;     /**< \internal calculated edit distance (for filtering) */
    ET9U8         bEditDistStem;    /**< \internal calculated stem distance (for exactness) */
    ET9U8         bEditDistFree;    /**< \internal calculated free distance (for exactness) */
    ET9U8         bEditDistSpcSbt;  /**< \internal number of substitutions */
    ET9U8         bEditDistSpcTrp;  /**< \internal number of transposes */
    ET9U8         bEditDistSpcIns;  /**< \internal number of adds */
    ET9U8         bEditDistSpcDel;  /**< \internal number of deletes */
    ET9BOOL       bHasPrimEditDist; /**< \internal if word has primary edit distance (not only propagated) */
    ET9BOOL       bIsUDBWord;       /**< \internal if word is a UDB word rather than a RUDB word */
    ET9BOOL       bIsGroupBase;     /**< \internal if word is basis for post-sorted group */
    ET9BOOL       bIsAcronym;       /**< \internal if word is an acronym */
    ET9BOOL       bIsCapitalized;   /**< \internal if word is capitalized */
    ET9BOOL       bIsTop5;          /**< \internal if word is in top 5 */
    ET9U8         bGroupCount;      /**< \internal if bIsGroupBase, number of entries in this post-sorted group */
    ET9U8         bCDBTrigram;      /**< \internal if ET9WORDSRC_CDB, indicates higher priority trigram match */
    ET9SYMB       sPureFirstChar;   /**< \internal the original first char in the word (before case shift etc) */
} ET9AWPrivWordInfo;

/**
 * Selection list correction mode values
 */

typedef enum ET9ASLCORRECTIONMODE_e {
    ET9ASLCORRECTIONMODE_LOW,                   /**< for large mechanical keyboards that requires minimal correction */
    ET9ASLCORRECTIONMODE_MEDIUM,                /**< for touchscreen keyboards and more careful users */
    ET9ASLCORRECTIONMODE_HIGH                   /**< for touchscreen keyboards and faster, less careful users */
} ET9ASLCORRECTIONMODE;

/** \internal
 * Selection list mode values
 */

typedef enum ET9ASLMODE_e {
    ET9ASLMODE_AUTO,                    /**< \internal will be controlled by the list correction mode (to one of the below) */
    ET9ASLMODE_CLASSIC,                 /**< \internal classic (behaves like 'T9') */
    ET9ASLMODE_COMPLETIONSPROMOTED,     /**< \internal completions promoted (will promote words that looks more like the input and bring up completions) */
    ET9ASLMODE_MIXED                    /**< \internal mixed list (allows more likely spell corrections and completions to be boosted) */
} ET9ASLMODE;                           /**< \internal */

/** \internal
 * Capture info - low level single string store.
 */

typedef struct ET9AWCaptureBuild_s {
    ET9U8               bIsValid;               /**< \internal if the capture info is valid */
    ET9U16              wWordLen;               /**< \internal current word string length */
    ET9U16              wWordCompLen;           /**< \internal current word string completion length */
    ET9U16              wSymbolLen;             /**< \internal current word symbol length */
    /* keep arrays last */
    ET9SYMB             sWord[ET9MAXWORDSIZE];  /**< \internal current word string */
} ET9AWCaptureBuild;                            /**< \internal */

/** \internal
 * Capture info - low level single capture action.
 */

typedef struct ET9AWCaptureAction_s {
    ET9U8               bPop;                   /**< \internal pop action (new string was needed) */
    ET9S8               sbAddWordLen;           /**< \internal string length difference */
    ET9S8               sbAddWordCompLen;       /**< \internal string completion length difference */
    ET9U8               bAddSymbolLen;          /**< \internal symbol length difference */
} ET9AWCaptureAction;                           /**< \internal */

#define ET9MAXBUILDCAPTURES 6                   /**< \internal the number of build captures stored */
#define ET9DEFAULTSTORESIZE 0x200               /**< \internal the number of symbols used for saving default words */

/** \internal
 * Capture info - all info stored in order to support builds, especially when doing things in "history".
 */

typedef struct ET9AWBuildInfo_s {
    ET9U8               bCurrCapture;                           /**< \internal the capture currently used */
    ET9U8               bCaptureInvalidated;                    /**< \internal flag to invalidate the whole capture */

    /* keep arrays last */

    ET9U8               pbFlushPos[ET9MAXWORDSIZE];             /**< \internal mark a position as a flush by the symbol count creating it */
    ET9U8               pbFlushLen[ET9MAXWORDSIZE];             /**< \internal the flush string length (at symbols entered) */

    ET9U16              pwDefaultPos[ET9MAXWORDSIZE];           /**< \internal the default string store position (at symbols entered) */
    ET9U8               pbDefaultLen[ET9MAXWORDSIZE];           /**< \internal the default string length (at symbols entered) */
    ET9U8               pbDefaultCompLen[ET9MAXWORDSIZE];       /**< \internal the default completion length (at symbols entered) */

    ET9SYMB             psFlushedSymbs[ET9MAXWORDSIZE];         /**< \internal flushed symbols store */
    ET9SYMB             psDefaultSymbs[ET9DEFAULTSTORESIZE];    /**< \internal default symbols store */
    ET9U8               bLanguageSource[ET9MAXWORDSIZE];        /**< \internal language source for the flushed word */

    ET9AWCaptureBuild   pCaptures[ET9MAXBUILDCAPTURES];         /**< \internal capture info(s) */
    ET9AWCaptureAction  pCaptureActions[ET9MAXWORDSIZE];        /**< \internal cature actions(s) - used to control the capture info */

} ET9AWBuildInfo;                                               /**< \internal */

/*----------------------------------------------------------------------------
 *  Linguistic Module  ** ALWAYS IN **
 *----------------------------------------------------------------------------*/

/* this feature is for future release */
#if 0
#define ET9PREDICT_SYMS         5
#endif

/**
 * Max number of custom terminal punctuation symbols
 */

#define ET9MAX_EXP_TERM_PUNCTS  16

/*----------------------------------------------------------------------------
 *  Define static spc information constants and types.
 *----------------------------------------------------------------------------*/

#define ET9_SPC_ED_MAX_DISTANCE     3                                           /**< \internal the max edit distance that can be used for spell correcton */
#define ET9_SPC_ED_DIST_ROWS        (ET9_SPC_ED_MAX_DISTANCE * 2 + 1)           /**< \internal */
#define ET9_SPC_ED_CALC_ROWS        (ET9_SPC_ED_MAX_DISTANCE * 2 + 3)           /**< \internal */
#define ET9_SPC_ED_STORE_ROW_LEN    (ET9MAXWORDSIZE + ET9_SPC_ED_MAX_DISTANCE)  /**< \internal */

typedef ET9U8   ET9ASPCRowU8[ET9_SPC_ED_STORE_ROW_LEN];                         /**< \internal */
typedef ET9U16  ET9ASPCRowU16[ET9_SPC_ED_STORE_ROW_LEN];                        /**< \internal */

/*----------------------------------------------------------------------------
 *  Define static language data base constants and types.
 *----------------------------------------------------------------------------*/

#define ALDB_COMPARE_MAX_POS             (ET9MAXWORDSIZE)                       /**< \internal max number positions that can be used in a compare - max size for trace - always to keep header files constant */
#define ALDB_COMPARE_MAX_FREQ_POS        (ET9MAXWORDSIZE)                       /**< \internal max number of positions that can have tap frequency information - max size for trace - always to keep header files constant */

#ifndef ALDB_COMPARE_MAX_POS
#define ALDB_COMPARE_MAX_POS             (ET9MAXLDBWORDSIZE)                    /**< \internal max number positions that can be used in a compare */
#endif

#ifndef ALDB_COMPARE_MAX_FREQ_POS
#define ALDB_COMPARE_MAX_FREQ_POS        (8 + 3)                                /**< \internal max number of positions that can have tap frequency information */
#endif

#ifndef ALDB_COMPARE_MAX_CODE_BYTES
#define ALDB_COMPARE_MAX_CODE_BYTES      (17)                                   /**< \internal max number of bytes used for compare information per position */
#endif


#define ALDB_COMPARE_MAX_DIST_KINDS      2                                      /**< \internal max number of flex compare kinds */

#define ALDB_COMPARE_MAX_DIST_LENGTHS    (ET9_SPC_ED_MAX_DISTANCE * 2 + 1)      /**< \internal max number of different word length given max edit distance */

#define ALDB_HEADER_MAX_CHAR_CODES       (ALDB_COMPARE_MAX_CODE_BYTES*8)        /**< \internal max number of character codes that can be used given the number of available compare bytes */
#define ALDB_HEADER_MAX_DIRECT_ENCODE    0x100                                  /**< \internal max number of symbols/characters that can be direct encoded (no search) */

#define ALDB_HEADER_ONE_BYTE_SIZE        0xFF                                   /**< \internal max number of one byte encoded intervals */

#define ALDB_CURSOR_DATA_CACHE_SIZE      20                                     /**< \internal cache size for a stream */

typedef ET9U8 ET9ALdbSymbPosInfo[ALDB_COMPARE_MAX_POS][ALDB_COMPARE_MAX_CODE_BYTES];
typedef ET9U8 ET9ALdbFreqPosInfo[ALDB_COMPARE_MAX_FREQ_POS][ALDB_HEADER_MAX_CHAR_CODES];

/** \internal
 * Support relations for low level flex screening.
 */

typedef struct ET9ASPCSupportRelations_s
{
    ET9U8               bCount;                                                 /**< \internal */
    ET9U8               pbIndex[ALDB_COMPARE_MAX_POS];                          /**< \internal */

} ET9ASPCSupportRelations;                                                      /**< \internal */

/*----------------------------------------------------------------------------
 *  Define static information structures for ldb and spc.
 *----------------------------------------------------------------------------*/

/** \internal
 * Compare data for spc.
 */

typedef struct ET9ASPCCompareData_s
{
    ET9ASPCRowU8            ppbFreqRowStore[ET9_SPC_ED_CALC_ROWS];                      /**< \internal */
    ET9ASPCRowU8            ppbCmpResultRowStore[ET9_SPC_ED_CALC_ROWS];                 /**< \internal */

    ET9ALdbSymbPosInfo      pppbActiveSpc[ALDB_COMPARE_MAX_DIST_LENGTHS];               /**< \internal active codes in spell correction compare (unions) */

} ET9ASPCCompareData;                                                                   /**< \internal */


/** \internal
 * Compare data for spc flex.
 */

typedef struct ET9ASPCFlexCompareData_s
{
    ET9BOOL                 bAllowSpcCmpl;                                                  /**< \internal xxx */
    ET9BOOL                 bAllowFreePunct;                                                /**< \internal xxx */
    ET9BOOL                 bAllowFreeDouble;                                               /**< \internal xxx */

    ET9UINT                 nQualityCount;                                                  /**< \internal xxx */

    ET9SymbInfo             const *pFirstSymb;                                              /**< \internal xxx */

    ET9U8                   pbLockInfo[ALDB_COMPARE_MAX_POS + 1];                           /**< \internal xxx */
    ET9U8                   pbIsFreqPos[ALDB_COMPARE_MAX_POS + 1];                          /**< \internal xxx */
    ET9U8                   pbIsQualityKey[ALDB_COMPARE_MAX_POS + 1];                       /**< \internal xxx */

    ET9U16                  pwPrevWordSC[ALDB_COMPARE_MAX_POS + 2];                         /**< \internal xxx */
    ET9U8                   pbSubstFreqSC[ALDB_COMPARE_MAX_POS + 1][ET9MAXWORDSIZE + 1];    /**< \internal xxx */

    ET9U8                   ppbFreeDist[ALDB_COMPARE_MAX_POS + 1][ET9MAXWORDSIZE + 1];      /**< \internal xxx */
    ET9U8                   ppbEditDist[ALDB_COMPARE_MAX_POS + 1][ET9MAXWORDSIZE + 1];      /**< \internal xxx */
    ET9U8                   ppbStemDist[ALDB_COMPARE_MAX_POS + 1][ET9MAXWORDSIZE + 1];      /**< \internal xxx */
    ET9FREQ                 ppxStemFreq[ALDB_COMPARE_MAX_POS + 1][ET9MAXWORDSIZE + 1];      /**< \internal xxx */

    ET9SYMB                 psLockSymb[ET9MAXWORDSIZE];                                     /**< \internal xxx */
    ET9SYMB                 psLockSymbOther[ET9MAXWORDSIZE];                                /**< \internal xxx */

    ET9ALdbSymbPosInfo      pppbActiveSpc[ALDB_COMPARE_MAX_DIST_KINDS];                     /**< \internal active codes in spell correction compare (unions) */

#ifdef ET9ASPC_DEV_SCREEN

    ET9UINT                 nActiveQualityCount;                                            /**< \internal xxx */
    ET9UINT                 nSupportedQualityCount;                                         /**< \internal xxx */

    ET9U8                   pbCharCounters[ALDB_HEADER_MAX_CHAR_CODES];                     /**< \internal active char counters */
    ET9U8                   pbQualityCounters[ALDB_COMPARE_MAX_POS];                        /**< \internal number of active chars supporting a quality point */

    ET9ASPCSupportRelations pCharCodeRelations[ALDB_HEADER_MAX_CHAR_CODES];                 /**< \internal */

#endif

} ET9ASPCFlexCompareData;                                                                   /**< \internal */

/**
 * Spell correction setting values
 */

typedef enum ET9ASPCMODE_e {
    ET9ASPCMODE_OFF,                /**< spell correction off */
    ET9ASPCMODE_EXACT,              /**< spell correction against exact input match */
    ET9ASPCMODE_REGIONAL            /**< spell correction against regional input match */

} ET9ASPCMODE;                      /**< xxx */

/**
 * Exact In List Enabling and Positioning values
 */

typedef enum ET9AEXACTINLIST_e {
    ET9AEXACTINLIST_OFF = 0,        /**< Same as ET9AWClearExactInList */
    ET9AEXACTINLIST_FIRST,          /**< ET9AWSetExactInList + ET9AWSetExactFirst + ET9AWClearExactAsDefault */
    ET9AEXACTINLIST_LAST,           /**< ET9AWSetExactInList + ET9AWSetExactLast +  ET9AWClearExactAsDefault */
    ET9AEXACTINLIST_DEFAULT         /**< ET9AWSetExactInList + ET9AWSetExactFirst +  ET9AWSetExactAsDefault */

} ET9AEXACTINLIST;                  /**< */

/**
 * Spell correction filter setting values for tap input
 */

typedef enum ET9ASPCSEARCHFILTER_e {
    ET9ASPCSEARCHFILTER_UNFILTERED      = 0x0,      /**< (000) unfiltered (most freedom) */
    ET9ASPCSEARCHFILTER_ONE_EXACT       = 0x3,      /**< (011) first symbol in word must match first input (exact match) */
    ET9ASPCSEARCHFILTER_ONE_REGIONAL    = 0x2,      /**< (010) first symbol in word must match first input (regional match) */
    ET9ASPCSEARCHFILTER_TWO_EXACT       = 0x5,      /**< (101) first symbol in word must match one of the two first inputs (exact match) */
    ET9ASPCSEARCHFILTER_TWO_REGIONAL    = 0x4       /**< (100) first symbol in word must match one of the two first inputs (regional match) */

} ET9ASPCSEARCHFILTER;                              /**< */

/**
 * Spell correction filter setting values for trace input
 */

typedef enum ET9ASPCTRACESEARCHFILTER_e {
    ET9ASPCTRACESEARCHFILTER_ONE_EXACT      = 0x3,  /**< (011) first symbol in word must match first input (exact match) */
    ET9ASPCTRACESEARCHFILTER_ONE_REGIONAL   = 0x2   /**< (010) first symbol in word must match first input (regional match) */

} ET9ASPCTRACESEARCHFILTER;                         /**< */

/** \internal
 * Spell correction filter settings
 */

typedef struct ET9ASpc_s {
    ET9U8                       bSpcState;              /**< \internal A means to control the state of used spc working memory */

    ET9U8                       bSpcFeatures;           /**< \internal A means to control specific spell correction features */
    ET9ASPCMODE                 eMode;                  /**< \internal spell correction mode */
    ET9U16                      wMaxSpcTermCount;       /**< \internal max number of spell corrected terms in the selectionlist */
    ET9U16                      wMaxSpcCmplCount;       /**< \internal max number of spell corrected completions in the selectionlist */
    ET9ASPCSEARCHFILTER         eSearchFilter;          /**< \internal spell correction filter for tap */
    ET9ASPCTRACESEARCHFILTER    eSearchFilterTrace;     /**< \internal spell correction filter for trace */

    ET9U32                      dwCompareChecksum;      /**< \internal xxx */

    union {
        ET9ASPCCompareData      sCmpData;               /**< \internal internal data structures for edit distance calculation */
        ET9ASPCFlexCompareData  sCmpDataFlex;           /**< \internal internal data structures for flex edit distance calculation */
    } u;                                                /**< \internal */

} ET9ASpc;                                              /**< \internal */

/** \internal
 * ALDB header data
 */

typedef struct ET9ALdbHeaderData_s
{
    ET9U8               bPosCount;                                                  /**< \internal number of LDB streams */
    ET9U16              pwCharacterEncodeTable[ALDB_HEADER_MAX_DIRECT_ENCODE];      /**< \internal table for direct symbol encoding */
    ET9U16              wCharacterEncodeExtendCount;                                /**< \internal number of symbols for extended encoding */
    ET9SYMB             sCharacterEncodeExtendFirstChar;                            /**< \internal value of the first extended encoded symbol (smallest) */
    ET9SYMB             sCharacterEncodeExtendLastChar;                             /**< \internal value of the last extended encoded symbol (largest) */
    ET9U16              wCharacterDecodeCount;                                      /**< \internal number of decodable symbols (codes) */
    ET9SYMB             psCharacterDecodeTable[ALDB_HEADER_MAX_CHAR_CODES];         /**< \internal table for decoding */
    ET9U8               pbOneByteCodes[ALDB_HEADER_ONE_BYTE_SIZE+1];                /**< \internal table of one byte codes */
    ET9U8               pbOneByteLengths[ALDB_HEADER_ONE_BYTE_SIZE];                /**< \internal table of one byte lengths */
    ET9U8               pbPosOrder[ET9MAXLDBWORDSIZE];                              /**< \internal position order to get sparsest first */
    ET9U32              pdwIntervalOffsets[ET9MAXLDBWORDSIZE+1];                    /**< \internal offsets to the different streams */
    ET9U16              wCodeZero;                                                  /**< \internal code for string 0 / termination */
    ET9U16              wCodeIntervalEnd;                                           /**< \internal code for stream end */
    ET9U16              wCodeIntervalJump;                                          /**< \internal code for stream jump*/
    ET9U16              wCodeIntervalExtend;                                        /**< \internal code for an extended interval */
    ET9U8               bLowerCount;                                                /**< \internal the number of lower case chars in the LDB */
    ET9U8               bUpperCount;                                                /**< \internal the number of upper case chars in the LDB */

    ET9U32              dwWordCount;                                                /**< \internal number of words in the LDB (calculated) */
    ET9U32              dwTopCount;                                                 /**< \internal number of words in the LDB top (calculated) */

} ET9ALdbHeaderData;                                                                /**< \internal */

/** \internal
 * ALDB compare data
 */

typedef struct ET9ALdbCompareData_s
{
    ET9U16              wLength;                /**< \internal number of input symbols */
    ET9U8               bActiveCmpLength;       /**< \internal number of active compare positions (cursors) */
    ET9BOOL             bSpcActive;             /**< \internal if it's spell correction compare */
    ET9BOOL             bFirstPosSetOpt;        /**< \internal found symbs will be removed from the search set - speed opt */

    ET9BOOL             bSpcExactCompare;       /**< \internal spc exact compare (exact/regional must be considered during compare) */
    ET9BOOL             bSpcFilteredCompare;    /**< \internal spc filtered compare */
    ET9BOOL             bSpcExactFilter;        /**< \internal spc filtered using exact filter */

    ET9BOOL             bSpcExactFilterTrace;   /**< \internal spc filtered using exact filter */
    ET9U8               bSpcExactPrunedTrace;   /**< \internal spc filter exact has been applied towards the regional ONE set, changes the matching section */
    ET9ALdbSymbPosInfo  *pppbSpcFlexSection;    /**< \internal pointer to the flex match section */

    ET9U8               bSpcMaxEdits;           /**< \internal number of available spc edits */
    ET9U8               bSpcLengthOffset;       /**< \internal the offset to subtract from a word length to get the compare set */

    ET9U8               bPosLo;                 /**< \internal lowest position to look for word termination */
    ET9U8               bPosHi;                 /**< \internal highest position to look for word termination */

    ET9ALdbSymbPosInfo  ppbActive;              /**< \internal active codes in classic compare */
    ET9ALdbSymbPosInfo  ppbExact;               /**< \internal exact codes in classic compare */
    ET9ALdbFreqPosInfo  ppbFreq;                /**< \internal tap frequency in classic compare */

    ET9U8               pbLocked[ALDB_COMPARE_MAX_POS];                 /**< \internal lock information in classic compare */

    ET9U8               pbCodeIsFree[ALDB_COMPARE_MAX_CODE_BYTES];      /**< \internal xxx */

} ET9ALdbCompareData;                           /**< \internal */

/** \internal
 * ALDB cursor data
 */

typedef struct ET9ALdbCursorData_s
{
    ET9U32              dwStartPos;                                 /**< \internal start position for current interval */
    ET9U32              dwEndPos;                                   /**< \internal end position for current interval, really one after the end (end - start = length) */
    ET9U32              dwJumpPos;                                  /**< \internal a position that can be jumped to */
    ET9U32              dwJumpAddress;                              /**< \internal address in the stream for the jump position */
    ET9U32              dwSourceDataStart;                          /**< \internal start position of the stream */
    ET9U32              dwSourceDataLength;                         /**< \internal length of the stream in bytes */
    ET9U16              wCode;                                      /**< \internal code for the current interval */

#ifdef ET9_DIRECT_LDB_ACCESS
    ET9U8 ET9FARDATA   *pbCurrData;                                 /**< \internal pointer to current position in the stream */
#else
    ET9U8              *pbCurrData;                                 /**< \internal pointer to current position in the cache */
    ET9U32              dwCurrCacheStart;                           /**< \internal where the currently cached data starts in the stream */
    ET9U8              *pbCacheEnd;                                 /**< \internal pointer to first position outside the cache */
    ET9U8               pbCache[ALDB_CURSOR_DATA_CACHE_SIZE];       /**< \internal stream cache */
#endif /* ET9_DIRECT_LDB_ACCESS */

} ET9ALdbCursorData;                                                /**< \internal */

/** \internal
 * ALDB search data
 */

typedef struct ET9ALdbSearchData_s
{
    ET9SYMB            *psTarget;                               /**< \internal target string for found words */
    ET9U16             *pwLength;                               /**< \internal length of found target word */
    ET9U16              wTargetLength;                          /**< \internal max target length */

    ET9U32              dwCurrItem;                             /**< \internal current item in the dictionary */
    ET9BOOL             bExhausted;                             /**< \internal if the dictionary is exhausted */

    ET9U8               bCurrWordLength;                        /**< \internal currently known word length */
    ET9U32              dwWordLengthEndPos;                     /**< \internal when the known word length ends */

    ET9BOOL             bSpcCompare;                            /**< \internal current spc compare mode */
    ET9U32              dwSpcControlEndPos;                     /**< \internal current end for spc control */
    ET9U32              dwRegNonMatchEndPos;                    /**< \internal current regular non match end pos */

    ET9U8               bSpcNonZeroPos;                         /**< \internal pos that must be non zero (word len) */
    ET9U8               bSpcControlPos;                         /**< \internal pos that controls if spc or regular compare */

    ET9U8               bRegCmpLength;                          /**< \internal number of compare positions for regular (non spc) */
    ET9U8               pbRegPosCurrOrder[ET9MAXLDBWORDSIZE];   /**< \internal position order for regular compare */

} ET9ALdbSearchData;                                            /**< \internal */


/** \internal
 * ALDB data
 */

typedef struct ET9ALdb_s
{
    ET9ALdbHeaderData   header;                                 /**< \internal LDB header info */
    ET9ALdbCompareData  compare;                                /**< \internal LDB compare info */
    ET9ALdbCursorData   pCursors[ET9MAXLDBWORDSIZE];            /**< \internal LDB cursor info */
    ET9ALdbSearchData   search;                                 /**< \internal LDB search info */

} ET9ALdb;                                                      /**< \internal */

/** \internal
 * ALDB auto substitution data
 */

typedef struct ET9ALDBAutoSub_s
{
    ET9U8     bSupported;                                       /**< \internal */
    ET9U8     bLSASDBVersion;                                   /**< \internal */
    ET9U8     bLSASDBLangID;                                    /**< \internal */
    ET9U16    wNumEntries;                                      /**< \internal */
    ET9U32    dwLSASDBStartAddress;                             /**< \internal */
    ET9U32    dwLSASDBEndAddress;                               /**< \internal */

} ET9ALDBAutoSub;                                               /**< \internal */

/** \internal
 * ALDB context model data
 */

typedef struct ET9ALM_s
{
    ET9U8     bSupported;                                       /**< \internal */
    ET9U8     bALMVersion;                                      /**< \internal */
    ET9U8     bALMLangID;                                       /**< \internal */
    ET9U32    dwNumEntries;                                     /**< \internal */
    ET9U16    wNumClasses;                                      /**< \internal */
    ET9U16    wEBits;                                           /**< \internal */
    ET9U16    wTBits;                                           /**< \internal */
    ET9U8     bScalingFactor;                                   /**< \internal */
    ET9U8     bAddConstant;                                     /**< \internal */
    ET9U32    dwALMStartAddress;                                /**< \internal */
    ET9U32    dwALMEndAddress;                                  /**< \internal */

} ET9ALM;                                                       /**< \internal */

/** \internal
 * Alt mode values
 */

typedef enum ET9AW_AltMode_e {
    ET9AW_AltMode_1 = 0,                    /**< \internal mode 1 */
    ET9AW_AltMode_2,                        /**< \internal mode 2 */
    ET9AW_AltMode_Last                      /**< \internal sentinel */
} ET9AW_AltMode;                            /**< \internal */

/** \internal
 * Private AW lingustics common info object.
 */

typedef struct ET9AWLingComPrivate_s
{
    ET9U8  ET9FARDATA      *pUDBGetEntry;                                   /**< \internal  */
    ET9U8  ET9FARDATA      *pASDBGetEntry;                                  /**< \internal  */
    ET9U16                  wLdbASGetEntryRec;                              /**< \internal 1-relative record number while ET9AWASDBGetEntry active */
    ET9U8                   bTotalExpTermPuncts;                            /**< \internal number of custom terminal punctuation symbols */
    ET9SYMB                 sExpTermPuncts[ET9MAX_EXP_TERM_PUNCTS];         /**< \internal custom terminal punctuation symbols */
    ET9SYMB                 sExpEmbeddedPunct;                              /**< \internal custom embedded punctuation symbol */
    ET9ALdb                 ALdb;                                           /**< \internal LDB info (internal) */
    ET9ALDBAutoSub          sLDBAutoSub;                                    /**< \internal auto substitution info */
    ET9U16                  wPreviousWordLanguage;                          /**< \internal language ID for previous selected word */
    ET9ALM                  ALdbLM;                                         /**< \internal LDB language model info (internal) */
    ET9U8                   bMDBWordSource;                                 /**< \internal a way to modify the source for an MDB word, for internal use (for now) */

    /* selection list centered things */

    ET9U8                   bListSize;                                      /**< \internal the max candidate list size (number of candidates) */

    ET9AWPrivWordInfo      *pWordList;                                      /**< \internal pointer to the actual selection list data */
    ET9U8                   bWordList[ET9MAXSELLISTSIZE];                   /**< \internal sorting info */
    ET9U8                   bDefaultIndex;                                  /**< \internal the suggested default index from the lingusitic engine */
    ET9U8                   bExactIndex;                                    /**< \internal the index of the exact word (first, last or undefined) */

    ET9U8                   bContextWordSize;                               /**< \internal size of the context word */
    ET9U8                   bPreviousContextWordSize;                       /**< \internal size of the previous context word */
    ET9SYMB                 sContextWord[ET9MAXWORDSIZE];                   /**< \internal context word */
    ET9SYMB                 sPreviousContextWord[ET9MAXWORDSIZE];           /**< \internal previous context word */
    ET9U16                  wContextWordClass;                              /**< \internal context word class */

    ET9ASLMODE              eSelectionListMode;                             /**< \internal selection list mode setting */
    ET9ASLCORRECTIONMODE    eSelectionListCorrectionMode;                   /**< \internal selection list correction mode setting */
    ET9U16                  wWordStemsPoint;                                /**< \internal word stems point - turn on word stems after Nth tap */
    ET9U16                  wWordCompletionPoint;                           /**< \internal word completion point - turn on word completion after Nth tap */
    ET9U16                  wMaxCompletionCount;                            /**< \internal current max number of completions/stems per "chunk" in the selection list */
    ET9ASpc                 ASpc;                                           /**< \internal spell correction info & settings */

    ET9U8                   bCurrSpcMode;                                   /**< \internal spc mode modified by input length */
    ET9U8                   bCurrMaxEditDistance;                           /**< \internal max edit distance */
    ET9U8                   bCurrMaxRegionality;                            /**< \internal max number of regions applied */
    ET9U16                  wCurrMinSourceLength;                           /**< \internal min spc word length */
    ET9U16                  wCurrMaxSourceLength;                           /**< \internal max spc word length */

    ET9U16                  wCurrBuildLang;                                 /**< \internal the language currently used by the builder */
    ET9U16                  wCurrBuildSecondLanguage;                       /**< \internal the language currently used by the builder */
    ET9U16                  wCurrLockPoint;                                 /**< \internal current lock point */
    ET9ASLMODE              eCurrSelectionListMode;                         /**< \internal current selection list mode */
    ET9BOOL                 bCurrBuildHasShiftSignificance;                 /**< \internal if the current build has shift significance (like capitalization) */

    ET9U8                   bLastBuildLen;                                  /**< \internal the symbol length for the last build */
    ET9BOOL                 bLastBuildShrinking;                            /**< \internal the direction of the last symbol length difference */
    ET9U8                   bTotalSymbInputs;                               /**< \internal the total number of symbols entered (special ling copy) */

    ET9AWBuildInfo          sBuildInfo;                                     /**< \internal builder history info */

    ET9BOOL                 bStateWordStems;                                /**< \internal state for word stems */
    ET9BOOL                 bStateWordCompletion;                           /**< \internal state for word completion */
    ET9BOOL                 bStateNextWordPrediction;                       /**< \internal state for next word prediction */
    ET9BOOL                 bStateExactInList;                              /**< \internal state for exact in list */
    ET9BOOL                 bStateExactLast;                                /**< \internal state for exact last (in list) */
    ET9BOOL                 bStateUserDefinedAutosub;                       /**< \internal state for user defined auto substitution */
    ET9BOOL                 bStateLDBSupportedAutosub;                      /**< \internal state for LDB supported auto substitution */
    ET9BOOL                 bStateAutoAppendInList;                         /**< \internal state for auto append in list */
    ET9BOOL                 bStateQUDBSupportEnabled;                       /**< \internal state for QUDB logic */
    ET9BOOL                 bStateLDBEnabled;                               /**< \internal state for enable LDB */
    ET9BOOL                 bStateCDBEnabled;                               /**< \internal state for enable CDB */
    ET9BOOL                 bStateRUDBEnabled;                              /**< \internal state for enable RUDB */
    ET9BOOL                 bStateASDBEnabled;                              /**< \internal state for enable ASDB */
    ET9BOOL                 bStateMDBEnabled;                               /**< \internal state for enable MDB */
    ET9BOOL                 bStateDownShiftDefault;                         /**< \internal state for downshifting default uppercase words */
    ET9BOOL                 bStateDownShiftAllLDB;                          /**< \internal state for downshifting all LDB words */
    ET9BOOL                 bStateExactIsDefault;                           /**< \internal state for exact as the default word (in list) */
    ET9BOOL                 bStateActiveLangSwitch;                         /**< \internal state for bilingual active language switch */
    ET9BOOL                 bStateInactiveLangSpellCorrect;                 /**< \internal state for bilingual inactive language SC */
    ET9BOOL                 bStatePostSort;                                 /**< \internal state for post sort of sellist */
    ET9BOOL                 bStateLM;                                       /**< \internal state for class language modeling */
    ET9BOOL                 bStateBoostTopCandidate;                        /**< \internal state for boosting top candidate */
    ET9BOOL                 bStateExpandAutosub;                            /**< \internal state for expanding auto substitutions */
    ET9BOOL                 bStateAutoSpace;                                /**< \internal state for activating auto space */

    ET9U32                  dwStateBits;                                    /**< \internal state bits (exact in list etc) */
    ET9U32                  dwDevStateBits;                                 /**< \internal dev state bits (default should be bit "off/zero", bits goes active to inhibit features) */

    ET9BOOL                 bRequiredFound;                                 /**< \internal the required word exists in the selection list */

    ET9BOOL                 bSpcDuringBuild;                                /**< \internal spell correction has been active at some point during the build */
    ET9BOOL                 bExpandAsDuringBuild;                           /**< \internal expand autosub has been active at some point during the build */
    ET9BOOL                 bSpcComplDuringSingleBuild;                     /**< \internal spell corrected completion at some point during a single build */

    ET9BOOL                 bTraceBuild;                                    /**< \internal the build has trace based input (symbs) */
    ET9BOOL                 bHasAllShiftedInfo;                             /**< \internal the build has all shifted input (symbs) */

    ET9INT                  snLinSearchCount;                               /**< \internal number of linear dupe searches since last list mod */
    ET9BOOL                 bHasRealWord;                                   /**< \internal if the list has a real word, e.g. an LDB word */

    ET9AWPrivWordInfo       sLeftHandWord;                                  /**< \internal the left hand word (for complex builds) */
    ET9AWPrivWordInfo       sPrevDefaultWord;                               /**< \internal the previous default word */
    ET9BOOL                 bStemsAllowed;                                  /**< \internal if stems currently are allowed - used during build for stems */
    ET9U16                  wMaxWordLength;                                 /**< \internal the (current) max word length - used during build for stems */

    ET9U16                  wTotalWordInserts;                              /**< \internal total inserted word count (all might not remain in the list) */

    ET9U8                   bTotalWords;                                    /**< \internal total/current word count */
    ET9U8                   bTotalSpcTermWords;                             /**< \internal total/current spc term word count */
    ET9U8                   bTotalSpcCmplWords;                             /**< \internal total/current spc completion word count */
    ET9U8                   bTotalCompletionWords;                          /**< \internal total/current completion word count */

    ET9AWPrivWordInfo      *pLastWord;                                      /**< \internal the word with the lowest prio */
    ET9AWPrivWordInfo      *pLastSpcTermWord;                               /**< \internal the spc term word with the lowest prio */
    ET9AWPrivWordInfo      *pLastSpcCmplWord;                               /**< \internal the spc completion word with the lowest prio */
    ET9AWPrivWordInfo      *pLastCompletionWord;                            /**< \internal the completion word with the lowest prio */

    ET9FREQPART             xMaxWordFreq;                                   /**< \internal the max word frequency */

    ET9U8                   bPrimaryFence;                                  /**< \internal primary fence value */
    ET9U8                   bSecondaryFence;                                /**< \internal secondary fence value */
    ET9U8                   pbLangSupported[ET9MAXWORDSIZE];                /**< \internal byte array for index of language(s) that are buildable at various word lengths (0, 1 or 2/both) */

    ET9U8                   bApplyBoosting;                                 /**< \internal lm boosting applied */

    ET9AW_AltMode           eAltMode;                                       /**< \internal alt mode */

#ifdef ET9_ACTIVATE_SLST_STATS
    struct ET9AWSlstStats_s
    {
        ET9U32              dwBuildCount;
        ET9U32              dwInsertCount;
        ET9U32              dwTotInsertCount;
        ET9U32              dwTotInsertDiscarded;
        ET9U32              dwTotInsertReplacing;
        ET9U32              dwTotInsertDuplicate;
        ET9U32              dwMaxListInserts;

        ET9U32              dwBuildCountAcc;
        ET9U32              dwInsertCountAcc;
        ET9U32              dwTotInsertCountAcc;
        ET9U32              dwTotInsertDiscardedAcc;
        ET9U32              dwTotInsertReplacingAcc;
        ET9U32              dwTotInsertDuplicateAcc;
        ET9U32              dwMaxListInsertsAcc;
    } sStats;
#endif

    ET9U16                  wInfoInitOK;                                    /**< \internal object init ok */

} ET9AWLingCmnPrivate;                                                      /**< \internal */

typedef struct ET9AWLingInfo_s ET9AWLingInfo;

typedef ET9STATUS (ET9FARCALL *ET9DBWRITECALLBACK)(
    ET9AWLingInfo    *pLingInfo,                    /**< pointer to Alpha information structure */
    ET9U8 ET9FARDATA *pbDest,                       /**< buffer for the data */
    ET9U8 ET9FARDATA *pbSource,                     /**< address to retrieve write data from */
    ET9UINT           unNumberOfWriteBytes          /**< number of bytes to be written */

);

#ifdef ET9_DIRECT_LDB_ACCESS

typedef ET9STATUS (ET9FARCALL *ET9DBREADCALLBACK)(
    ET9AWLingInfo          *pLingInfo,              /**< pointer to Alpha information structure */
    ET9U8 * ET9FARDATA *    ppbSrc,                 /**< pointer to a pointer to the integration supplied buffer */
    ET9U32                 *pdwSizeInBytes          /**< pointer to enter LDB size in bytes  */
);

#else /* ET9_DIRECT_LDB_ACCESS */

typedef ET9STATUS (ET9FARCALL *ET9DBREADCALLBACK)(
    ET9AWLingInfo *pLingInfo,                       /**< pointer to Alpha information structure */
    ET9U32         dwOffset,                        /**< offset to read from */
    ET9U32         dwNumberOfReadBytes,             /**< specifies the number of bytes to be read */
    ET9U8         *pbDest,                          /**< buffer for the data */
    ET9U32        *pdwBytesRead                     /**< pointer to the variable that receives the number of bytes read */
);

#endif /* ET9_DIRECT_LDB_ACCESS */


#define LDB_CHUNK_ID        1
#define ASDB_CHUNK_ID       2
#define CONTEXT_CHUNK_ID    3

/*------------------------------------------------------------------------
 *  Alpha CDB description.
 *------------------------------------------------------------------------*/

#ifdef ET9_JAPANESE_MODULE
#define ET9MINCDBDATABYTES 160      /**< minimum size requested for CDB in bytes */
#else
#define ET9MINCDBDATABYTES 1600     /**< minimum size requested for CDB in bytes */
#endif

typedef struct ET9AWCDBInfo_s {
    ET9U16  wDataSize;              /**< total size in bytes of this struct (minimum is 1024) */
    ET9U16  wUpdateCounter;         /**< count incremented each time user database modified */
    ET9U16  wDataEndOffset;         /**< offset to end of context data */
    ET9U16  wSavedOffset;           /**< pointer to last accessed position in database */
    ET9U32  dwOffsetSaver;          /**< identifier for thread that last saved offset. */
    ET9SYMB sDataArea[1];           /**< really a variable size data array */

} ET9AWCDBInfo;


/*------------------------------------------------------------------------
 *  Alpha RUDB description.
 *------------------------------------------------------------------------*/

#define ET9MINRUDBDATABYTES 10240

typedef struct ET9AWRUDBInfo_s {
    ET9U16  wDataSize;          /**< total size in bytes of the RUDB */
    ET9U16  wDataCheck;         /**< additive checksum of database */
    ET9U16  wUpdateCounter;     /**< count incremented each time database is modified */
    ET9U16  wUDBWordCount;      /**< number of valid UDB words currently in database */
    ET9U16  wRDBWordCount;      /**< number of valid RDB words currently in database */
    ET9U16  wRemainingMemory;   /**< free bytes of memory remaining in RUDB */
    ET9U16  wLastDelCutOffFreq; /**< tracks last frequency value used for GC */
    ET9U16  wSavedOffset;       /**< saves last access offset for next access */
    ET9U32  dwOffsetSaver;      /**< saves thread specific address info */
    ET9U16  wSizeOffset[10];    /**< offsets for the 10 size bins/regions */
    ET9U8   bDataArea[1];       /**< really a variable size data array */

} ET9AWRUDBInfo;


/*------------------------------------------------------------------------
 *  Alpha ASDB description.
 *------------------------------------------------------------------------*/

#define ET9MINASDBDATABYTES         10240
#define ET9MAXLDBSUPPORTEDASRECORDS 256
#define ET9MAXASDBLANGUAGERECORDS   10
#define ET9NUMASDBSIZERANGES        6


/* LDB-supported substitution record bit positions are translated from       */
/* a 0-relative record number (0-255) by generating a bMap index from the    */
/* record number (recordnum >> 3) and a bit position within that bMap byte   */
/* (recordnum & 0x07). So record number 213 (0xD5) would correspond to bMap  */
/* index 0xD5 >> 3, or 0x1A (26), bit position 0xD5 & 0x7, or 5              */

typedef struct ET9LdbASRecMap_s {
    ET9U16  wLDBID;             /**< LDB language ID for record mapping */
    ET9U16  wTotalRecords;      /**< actual number of substitution records for lannguage */
    ET9U16  wEnabledRecords;    /**< total enabled records for language */
    ET9U8   bMap[ET9MAXLDBSUPPORTEDASRECORDS/8];  /* bit mapping of records (currently up to 256) */
} ET9LdbASRecMap;

typedef struct ET9AWASDBInfo_s {
    ET9U16  wDataSize;          /**< total size in bytes of the ASDB */
    ET9U16  wDataCheck;         /**< additive checksum of database */
    ET9U16  wEntryCount;        /**< number of valid ASDB entries currently in database */
    ET9U16  wRemainingMemory;   /**< free bytes of memory remaining in ASDB */
    ET9U32  dwOffsetSaver;      /**< saves thread specific address info for user record access */
    ET9U16  wSavedOffset;       /**< saves last access offset for next user record access */
    ET9U16  wSavedRecordNum;    /**< saves last access record for next LDB record access */
    ET9U32  dwRecordNumSaver;   /**< saves thread specific address info for LDB record access */
    ET9U16  wSizeOffset[ET9NUMASDBSIZERANGES];                  /**< offsets for the user entry shortcut size bins/regions */
    ET9U16  wLDBUseTracker[ET9MAXASDBLANGUAGERECORDS];          /**< */
    ET9LdbASRecMap sLdbASRecord[ET9MAXASDBLANGUAGERECORDS];     /**< */
    ET9U8   bDataArea[1];       /**< really a variable size data array */

} ET9AWASDBInfo;

/*------------------------------------------------------------------------
 *  Alpha MDB description.
 *-----------------------------------------------------------------------*/

typedef enum ET9MDBSTATEBITS_e {
    ET9_MDB_REGISTERED = 0

} ET9MDBSTATEBITS;


typedef enum ET9MDBSTATEBITSMASK_e {
    ET9_MDB_REGISTERED_MASK = ((ET9U16)(1L << ET9_MDB_REGISTERED))

} ET9MDBSTATEBITSMASK;


/**
 * MDB requests
 */

typedef enum ET9REQMDB_e {
    ET9MDBGETEXACTWORDS = 1,    /**< 1 */
    ET9MDBGETALLWORDS           /**< 2 */

} ET9REQMDB;


typedef ET9STATUS (ET9FARCALL *ET9MDBCALLBACK)(
    ET9AWLingInfo *pLingInfo,    /**< I   - pointer to FieldInfo struct owning MDB */
    ET9REQMDB eMdbRequestType,   /**< I   - MDB request type. Should be one of the values defined above */
    ET9U16    wWordLen,          /**< I   - word length */
    ET9U16    wMaxWordLen,       /**< I   - maximum word length */
    ET9SYMB  *psBuildTxtBuf,     /**< O   - word to return */
    ET9U16   *pwActWordLen,      /**< O   - length of the returned word */
    ET9U32   *pdwWordListIdx     /**< I/O - MDB word list index */

);


typedef struct ET9AWMDBInfo_s {
    ET9U16         wStatus;         /**< state bits for MDB */
    ET9MDBCALLBACK pReadMdbData;    /**< pointer to MDB callback function */

} ET9AWMDBInfo;


ET9STATUS ET9FARCALL ET9SetDownshiftDefault(ET9AWLingInfo * const pLingInfo);

ET9STATUS ET9FARCALL ET9ClearDownshiftDefault(ET9AWLingInfo * const pLingInfo);

ET9STATUS ET9FARCALL ET9SetDownshiftAllLDBWords(ET9AWLingInfo * const pLingInfo);

ET9STATUS ET9FARCALL ET9ClearDownshiftAllLDBWords(ET9AWLingInfo * const pLingInfo);


/*------------------------------------------------------------------------
 *  Define ET9 Alphabetic system structures and functions
 *------------------------------------------------------------------------*/


/**
 * Public AW lingustics common info object.
 */

typedef struct ET9AWLingCmnInfo_s
{
    ET9BaseLingInfo             Base;               /**< \private ling base (must be the first item in the struct) */
    ET9U16                      wLdbNum;            /**< current LDB - cached values */
    ET9U16                      wFirstLdbNum;       /**< first linguistics data base number */
    ET9U16                      wSecondLdbNum;      /**< second linguistics data base number */
    ET9AWRUDBInfo   ET9FARDATA *pRUDBInfo;          /**< pointer to RUDB info */
    ET9AWCDBInfo    ET9FARDATA *pCDBInfo;           /**< pointer to CDB info */
    ET9AWASDBInfo   ET9FARDATA *pASDBInfo;          /**< pointer to ASDB info */
    ET9AWLingCmnPrivate         Private;            /**< \private private lingustic common info. */

} ET9AWLingCmnInfo;


/** \internal
 * Private AW lingustics info object.
 */

typedef struct ET9AWLingPrivate_s
{
    ET9CONVERTSYMBCALLBACK  pConvertSymb;           /**< pointer to ConvertSymb (engine related use) */
    void                   *pConvertSymbInfo;       /**< pointer to ConvertSymb info (that gets passed back in the callback) */

#ifdef ET9_DIRECT_LDB_ACCESS
    struct
    {
        ET9U8  ET9FARDATA   *pLdbData;              /**< pointer to a byte array holding the whole LDB */
        ET9U32              dwLdbDataSize;          /**< size of the whole LDB */
    } ALdb;
#endif /* ET9_DIRECT_LDB_ACCESS */

    ET9U16                  wInfoInitOK;            /**< object init ok */
    ET9U16                  wLDBInitOK;             /**< ldb init ok */

} ET9AWLingPrivate;

/**
 * Public AW lingustics info object.
 */

struct ET9AWLingInfo_s
{
    void                   *pPublicExtension;       /**< pointer for OEM extension. */
    ET9AWMDBInfo            sMDBInfo;               /**< MDB info */
    ET9DBREADCALLBACK       ET9AWLdbReadData;       /**< callback to read LDB data */
    ET9DBWRITECALLBACK      pRUDBWriteData;         /**< callback to write RUDB data */
    ET9DBWRITECALLBACK      pCDBWriteData;          /**< callback to write CDB data */
    ET9DBWRITECALLBACK      pASDBWriteData;         /**< callback to write ASDB data */
    ET9AWLingCmnInfo       *pLingCmnInfo;           /**< pointer to the common ling info object */

    ET9AWLingPrivate        Private;                /**< \private Private lingustic info. */
};


ET9STATUS ET9FARCALL ET9AWSysSetWordStemsPoint(ET9AWLingInfo * const pLingInfo,
                                               const ET9U16          wWordStemsPoint);

ET9STATUS ET9FARCALL ET9AWSysSetWordCompletionPoint(ET9AWLingInfo * const pLingInfo,
                                                    const ET9U16          wWordCompletionPoint);

ET9STATUS ET9FARCALL ET9AWSysSetCompletionCount(ET9AWLingInfo * const pLingInfo,
                                                const ET9U16          wCount);

ET9STATUS ET9FARCALL ET9AWSetDBStems(ET9AWLingInfo * const pLingInfo);

ET9STATUS ET9FARCALL ET9AWSetDBCompletion(ET9AWLingInfo * const pLingInfo);

ET9STATUS ET9FARCALL ET9AWSetDBPrediction(ET9AWLingInfo * const pLingInfo);

ET9STATUS ET9FARCALL ET9AWSetUDBDelayedReorder(ET9AWLingInfo * const pLingInfo);

ET9STATUS ET9FARCALL ET9AWClearDBStems(ET9AWLingInfo * const pLingInfo);

ET9STATUS ET9FARCALL ET9AWClearDBCompletion(ET9AWLingInfo * const pLingInfo);

ET9STATUS ET9FARCALL ET9AWClearDBPrediction(ET9AWLingInfo * const pLingInfo);

ET9STATUS ET9FARCALL ET9AWClearUDBDelayedReorder(ET9AWLingInfo * const pLingInfo);

ET9STATUS ET9FARCALL ET9AWSysSetSpellCorrectionMode(ET9AWLingInfo       * const pLingInfo,
                                                    const ET9ASPCMODE   eMode,
                                                    const ET9BOOL       bSpellCorrectSecondaryLanguage);

ET9STATUS ET9FARCALL ET9AWSysSetSpellCorrectionCount(ET9AWLingInfo * const pLingInfo,
                                                     const ET9U16          wCount);

ET9STATUS ET9FARCALL ET9AWSysSetSpellCorrectionSearchFilter(ET9AWLingInfo     * const pLingInfo,
                                                            const ET9ASPCSEARCHFILTER eFilter);

ET9STATUS ET9FARCALL ET9AWSysSetSpellCorrectionTraceSearchFilter(ET9AWLingInfo          * const pLingInfo,
                                                                 const ET9ASPCTRACESEARCHFILTER eFilter);

ET9STATUS ET9FARCALL ET9AWSysSetSelectionListMode(ET9AWLingInfo             * const pLingInfo,
                                                  const ET9ASLCORRECTIONMODE        eMode);

ET9STATUS ET9FARCALL ET9AWSysSetPostSort(ET9AWLingInfo * const pLingInfo);

ET9STATUS ET9FARCALL ET9AWSysClearPostSort(ET9AWLingInfo * const pLingInfo);

ET9STATUS ET9FARCALL ET9AWSysSetBigramLM(ET9AWLingInfo * const pLingInfo);

ET9STATUS ET9FARCALL ET9AWSysClearBigramLM(ET9AWLingInfo * const pLingInfo);

ET9STATUS ET9FARCALL ET9AWSysSetBoostTopCandidate(ET9AWLingInfo * const pLingInfo);

ET9STATUS ET9FARCALL ET9AWSysClearBoostTopCandidate(ET9AWLingInfo * const pLingInfo);

ET9STATUS ET9FARCALL ET9AWSetUserDefinedAutoSubstitution(ET9AWLingInfo * const pLingInfo);

ET9STATUS ET9FARCALL ET9AWClearUserDefinedAutoSubstitution(ET9AWLingInfo * const pLingInfo);

ET9STATUS ET9FARCALL ET9AWSetLDBAutoSubstitution(ET9AWLingInfo * const pLingInfo);

ET9STATUS ET9FARCALL ET9AWClearLDBAutoSubstitution(ET9AWLingInfo * const pLingInfo);

ET9STATUS ET9FARCALL ET9AWSysSetExpandAutoSubstitutions(ET9AWLingInfo * const pLingInfo);

ET9STATUS ET9FARCALL ET9AWSysClearExpandAutoSubstitutions(ET9AWLingInfo * const pLingInfo);

ET9STATUS ET9FARCALL ET9AWSysSetAutoSpace(ET9AWLingInfo * const pLingInfo);

ET9STATUS ET9FARCALL ET9AWSysClearAutoSpace(ET9AWLingInfo * const pLingInfo);

ET9STATUS ET9FARCALL ET9AWEnableDBs(ET9AWLingInfo * const pLingInfo,
                                    const ET9U32          dwDBMasks);

ET9STATUS ET9FARCALL ET9AWDisableDBs(ET9AWLingInfo * const pLingInfo,
                                     const ET9U32          dwDBMasks);

ET9STATUS ET9FARCALL ET9AWFillContextBuffer(ET9AWLingInfo * const pLingInfo,
                                            ET9SYMB       * const psBuf,
                                            const ET9UINT         nBufLen);

/**
 * Get spell correction mode.
 * Should give compiler error on bad pointer type.
 */

#define ET9AWSys_GetSpellCorrectionMode(pLingInfo) ((pLingInfo) == NULL ? ET9ASPCMODE_OFF : (pLingInfo)->pLingCmnInfo->Private.ASpc.eMode)

/**
 * Get spell correction search filter.
 * Should give compiler error on bad pointer type.
 */

#define ET9AWSys_GetSpellCorrectionSearchFilter(pLingInfo) ((pLingInfo) == NULL ? ET9ASPCSEARCHFILTER_UNFILTERED : (pLingInfo)->pLingCmnInfo->Private.ASpc.eSearchFilter)

/**
 * Get spell correction count.
 * Should give compiler error on bad pointer typs.
 */

#define ET9AWSys_GetSpellCorrectionCount(pLingInfo) ((pLingInfo) == NULL ? 0 : (pLingInfo)->pLingCmnInfo->Private.ASpc.wMaxSpcTermCount)

/**
 * Get selection list mode.
 * Should give compiler error on bad pointer type.
 */

#define ET9AWSys_GetSelectionListMode(pLingInfo) ((pLingInfo) == NULL ? ET9ASLCORRECTIONMODE_HIGH : (pLingInfo)->pLingCmnInfo->Private.eSelectionListCorrectionMode)


/**
 * Get word stems point.
 * Should give compiler error on bad pointer type.
 */

#define ET9AWSys_GetWordStemsPoint(pLingInfo) ((pLingInfo) == NULL ? 0 : (pLingInfo)->pLingCmnInfo->Private.wWordStemsPoint)

/**
 * Get word completion point.
 * Should give compiler error on bad pointer type.
 */

#define ET9AWSys_GetWordCompletionPoint(pLingInfo) ((pLingInfo) == NULL ? 0 : (pLingInfo)->pLingCmnInfo->Private.wWordCompletionPoint)

/**
 * Get word completion count.
 * Should give compiler error on bad pointer type.
 */

#define ET9AWSys_GetCompletionCount(pLingInfo) ((pLingInfo) == NULL ? 0 : (pLingInfo)->pLingCmnInfo->Private.wMaxCompletionCount)

/**
 * Get primary bilingual fence value.
 * Should give compiler error on bad pointer type.
 */

#define ET9AWSys_GetPrimaryFence(pLingInfo) ((pLingInfo) == NULL ? 0 : (pLingInfo)->pLingCmnInfo->Private.bPrimaryFence)

/**
 * Get secondary bilingual fence value.
 * Should give compiler error on bad pointer type.
 */

#define ET9AWSys_GetSecondaryFence(pLingInfo) ((pLingInfo) == NULL ? 0 : (pLingInfo)->pLingCmnInfo->Private.bSecondaryFence)

/**
 * Get bilingual support indication.
 * Should give compiler error on bad pointer type.
 */

#define ET9AWSys_GetBilingualSupported(pLingInfo) ((((pLingInfo) != NULL) &&  (pLingInfo)->pLingCmnInfo->wSecondLdbNum) ? (((pLingInfo)->pLingCmnInfo->wSecondLdbNum & ET9PLIDMASK) != ET9PLIDNone) : 0)

/*------------------------------------------------------------------------
 *  Define ET9 Alphabetic selection list functions
 *------------------------------------------------------------------------*/

/**
 * Check if system has an ok init.
 */

#define ET9AWSys_Init_OK(pLingInfo) ((pLingInfo != NULL) && (((ET9AWLingInfo*)pLingInfo)->Private.wInfoInitOK == ET9GOODSETUP))

/**
 * Check if system has an ok LDB init.
 */

#define ET9AWSys_LDB_OK(pLingInfo)  ((pLingInfo != NULL) && (((ET9AWLingInfo*)pLingInfo)->Private.wLDBInitOK == ET9GOODSETUP))


ET9STATUS ET9FARCALL ET9AWSysInit(ET9AWLingInfo     * const pLingInfo,
                                  ET9AWLingCmnInfo  * const pLingCmnInfo,
                                  ET9WordSymbInfo   * const pWordSymbInfo,
                                  const ET9BOOL             bResetLingCmn,
                                  const ET9U8               bListSize,
                                  ET9AWPrivWordInfo * const pWordList,
                                  void              * const pPublicExtension);

ET9STATUS ET9FARCALL ET9AWSetExactInList(ET9AWLingInfo * const pLingInfo,
                                         const ET9AEXACTINLIST eSetting);

ET9STATUS ET9FARCALL ET9AWGetExactInList(ET9AWLingInfo    * const pLingInfo,
                                         ET9AEXACTINLIST  * const peSetting);

ET9STATUS ET9FARCALL ET9AWSetAutoAppendInList(ET9AWLingInfo * const pLingInfo);

ET9STATUS ET9FARCALL ET9AWClearAutoAppendInList(ET9AWLingInfo * const pLingInfo);

ET9STATUS ET9FARCALL ET9AWSelLstBuild(ET9AWLingInfo     * const pLingInfo,
                                      ET9U8             * const pbTotalWords,
                                      ET9U8             * const pbDefaultListIndex);

ET9STATUS ET9FARCALL ET9AWSelLstGetInlineWord(ET9AWLingInfo        * const pLingInfo,
                                              ET9SimpleWord        * const pWord,
                                              ET9BOOL              * const pbCorrection);

ET9STATUS ET9FARCALL ET9AWSelLstGetWord(ET9AWLingInfo     * const pLingInfo,
                                        ET9AWWordInfo    ** const pWord,
                                        const ET9U8               bWordIndex);

ET9STATUS ET9FARCALL ET9AWSelLstSelWord(ET9AWLingInfo     * const pLingInfo,
                                        const ET9U8               bWordIndex);

ET9STATUS ET9FARCALL ET9AWLockWord(ET9AWLingInfo     * const pLingInfo,
                                   const ET9U8               byWordIndex);

ET9STATUS ET9FARCALL ET9AWNoteWordDone(ET9AWLingInfo * const pLingInfo,
                                       ET9SYMB       * const pText,
                                       const ET9U16          aSize);

ET9STATUS ET9FARCALL ET9AWNotePhraseDone(ET9AWLingInfo * const    pLingInfo,
                                         ET9SYMB * const          pText,
                                         const ET9U16             aSize);

ET9STATUS ET9FARCALL ET9AWSetCustomTermPuncts(ET9AWLingInfo * const pLingInfo,
                                              ET9SYMB       * const psTermPuncts,
                                              const ET9U8           bTotalPuncts);

ET9STATUS ET9FARCALL ET9AWSetDefaultTermPuncts(ET9AWLingInfo * const pLingInfo);

ET9STATUS ET9FARCALL ET9AWGetTermPuncts(ET9AWLingInfo * const pLingInfo,
                                        const ET9U16          wLdbNum,
                                        ET9SYMB       * const psPunctBuf,
                                        const ET9U8           bPunctBufLen,
                                        ET9U8         * const pbCount,
                                        ET9BOOL       * const pbDefault);

ET9STATUS ET9FARCALL ET9AWSetCustomEmbeddedPunct(ET9AWLingInfo * const pLingInfo,
                                                 const ET9SYMB         sEmbeddedPunct);

ET9STATUS ET9FARCALL ET9AWSetDefaultEmbeddedPunct(ET9AWLingInfo * const pLingInfo);

ET9STATUS ET9FARCALL ET9AWGetEmbeddedPunct(ET9AWLingInfo * const pLingInfo,
                                           const ET9U16          wLdbNum,
                                           ET9SYMB       * const psPunct,
                                           ET9BOOL       * const pbDefault);


ET9STATUS ET9FARCALL ET9AWSelLstPostShift(ET9AWLingInfo * const   pLingInfo,
                                          const ET9POSTSHIFTMODE  eMode,
                                          ET9U8         * const   pbTotalWords,
                                          ET9U8         * const   pbCurrListIndex);

ET9STATUS ET9FARCALL ET9AWGetPostShiftMode(ET9AWLingInfo    * const pLingInfo,
                                           ET9POSTSHIFTMODE * const peMode);

ET9STATUS ET9FARCALL ET9AWSetConvertSymb(ET9AWLingInfo        * const pLingInfo,
                                         const ET9CONVERTSYMBCALLBACK pConvertSymb,
                                         void                 * const pConvertSymbInfo);

ET9STATUS ET9FARCALL ET9AWSetActiveLanguageSwitch(ET9AWLingInfo * const pLingInfo);

ET9STATUS ET9FARCALL ET9AWClearActiveLanguageSwitch(ET9AWLingInfo * const pLingInfo);

/*------------------------------------------------------------------------
 *  Define ET9 Alphabetic LDB functions
 *------------------------------------------------------------------------*/

ET9STATUS ET9FARCALL ET9AWLdbInit(ET9AWLingInfo   * const pLingInfo,
                                  const ET9DBREADCALLBACK ET9AWLdbReadData);

ET9STATUS ET9FARCALL ET9AWLdbSetLanguage(ET9AWLingInfo * const  pLingInfo,
                                         const ET9U16           wFirstLdbNum,
                                         const ET9U16           wSecondLdbNum);

ET9STATUS ET9FARCALL ET9AWLdbGetLanguage(ET9AWLingInfo * const pLingInfo,
                                         ET9U16        * const pwFirstLdbNum,
                                         ET9U16        * const pwSecondLdbNum);

ET9STATUS ET9FARCALL ET9AWLdbGetActiveLanguage(ET9AWLingInfo * const pLingInfo,
                                               ET9U16        * const pwLdbNum);

ET9STATUS ET9FARCALL ET9AWLdbValidate(ET9AWLingInfo   * const pLingInfo,
                                      const ET9U16            wLdbNum,
                                      const ET9DBREADCALLBACK ET9AWLdbReadData);

ET9STATUS ET9FARCALL ET9AWLdbGetVersion(ET9AWLingInfo * const pLingInfo,
                                        ET9SYMB       * const psLdbVerBuf,
                                        const ET9U16          wBufMaxSize,
                                        ET9U16        * const pwBufSize);

/*------------------------------------------------------------------------
 *  Define ET9 Alphabetic RUDB functions
 *------------------------------------------------------------------------*/

ET9STATUS ET9FARCALL ET9AWRUDBInit(ET9AWLingInfo            * const pLingInfo,
                                   ET9AWRUDBInfo ET9FARDATA * const pRUDBInfo,
                                   const ET9U16                     wDataSize,
                                   const ET9DBWRITECALLBACK         pWriteCB);

ET9STATUS ET9FARCALL ET9AWUDBAddWord(ET9AWLingInfo * const pLingInfo,
                                     ET9SYMB       * const psBuf,
                                     const ET9U16          wWordLen);

ET9STATUS ET9FARCALL ET9AWUDBGetWordCount(ET9AWLingInfo * const pLingInfo,
                                          ET9U16        * const pEntryCount);

ET9STATUS ET9FARCALL ET9AWUDBDeleteWord(ET9AWLingInfo * const pLingInfo,
                                        ET9SYMB       * const psBuf,
                                        const ET9U16          wWordLen);

ET9STATUS ET9FARCALL ET9AWUDBGetWord(ET9AWLingInfo * const  pLingInfo,
                                     ET9SYMB       * const  psWordBuf,
                                     const ET9U16           wWordBufLen,
                                     ET9U16        * const  pwWordLen,
                                     const ET9U8            bForward);

ET9STATUS ET9FARCALL ET9AWRUDBReset(ET9AWLingInfo * const pLingInfo);

ET9STATUS ET9FARCALL ET9AWUDBOnlyReset(ET9AWLingInfo * const pLingInfo);

ET9STATUS ET9FARCALL ET9AWUDBFindWord(ET9AWLingInfo * const pLingInfo,
                                      ET9SYMB       * const psBuf,
                                      const ET9U16          wWordLen);

ET9STATUS ET9FARCALL ET9AWScanBufForCustomWords(ET9AWLingInfo * const pLingInfo,
                                                ET9SYMB       * const psBuf,
                                                const ET9U16          wBufLen,
                                                const ET9BOOL         bCheckMDB);

ET9STATUS ET9FARCALL ET9AWScanBufForNextCustomWord(ET9AWLingInfo * const pLingInfo,
                                                   ET9SYMB      ** const ppsBuf,
                                                   ET9U16        * const pwBufLen,
                                                   ET9SYMB       * const psWordBuf,
                                                   const ET9U16          wWordBufLen,
                                                   ET9U16        * const pwWordLen,
                                                   const ET9BOOL         bCheckMDB);

ET9STATUS ET9FARCALL ET9AWScanBufForNextSpellCorrection(ET9AWLingInfo        * const pLingInfo,
                                                        ET9SYMB ET9FARDATA   * const psBegBuf,
                                                        ET9SYMB ET9FARDATA   * const psBuf,
                                                        ET9U16                 const wBufLen,
                                                        ET9U8                * const pbTotalWords,
                                                        ET9SYMB * ET9FARDATA * const ppsWord,
                                                        ET9SYMB * ET9FARDATA * const ppsString,
                                                        ET9U16               * const pwStringLen);

/*------------------------------------------------------------------------
 *  Define ET9 Alphabetic AutoSubstitution functions
 *------------------------------------------------------------------------*/

ET9STATUS ET9FARCALL ET9AWASDBInit(ET9AWLingInfo            * const pLingInfo,
                                   ET9AWASDBInfo ET9FARDATA * const pASDBInfo,
                                   const ET9U16                     wDataSize,
                                   const ET9DBWRITECALLBACK         pWriteCB);

ET9STATUS ET9FARCALL ET9AWASDBAddEntry(ET9AWLingInfo * const pLingInfo,
                                       ET9SYMB       * const psShortCut,
                                       ET9SYMB       * const psSubstitution,
                                       const ET9U16          wShortcutLen,
                                       const ET9U16          wSubstitutionLen);

ET9STATUS ET9FARCALL ET9AWASDBGetEntryCount(ET9AWLingInfo * const pLingInfo,
                                            ET9U16        * const pEntryCount);

ET9STATUS ET9FARCALL ET9AWASDBDeleteEntry(ET9AWLingInfo * const pLingInfo,
                                          ET9SYMB       * const psBuf,
                                          const ET9U16          wWordLen);

ET9STATUS ET9FARCALL ET9AWASDBGetEntry(ET9AWLingInfo * const pLingInfo,
                                       ET9SYMB       * const psShortcutBuf,
                                       const ET9U16          wShortcutBufLen,
                                       ET9U16        * const pwShortcutLen,
                                       ET9SYMB       * const psSubstitutionBuf,
                                       const ET9U16          wSubstitutionBufLen,
                                       ET9U16        * const pwSubstitutionLen,
                                       const ET9U8           bForward);

ET9STATUS ET9FARCALL ET9AWASDBReset(ET9AWLingInfo * const pLingInfo);

ET9STATUS ET9FARCALL ET9AWASDBFindEntry(ET9AWLingInfo * const pLingInfo,
                                        ET9SYMB       * const psShortcut,
                                        const ET9U16          wShortcutLen,
                                        ET9SYMB       * const psSubstitutionBuf,
                                        const ET9U16          wSubstitutionBufLen,
                                        ET9U16        * const pwSubstitutionLen);

/*------------------------------------------------------------------------
 *  Define ET9 Alphabetic MDB functions
 *------------------------------------------------------------------------*/

ET9STATUS ET9FARCALL ET9AWRegisterMDB(ET9AWLingInfo * const pLingInfo,
                                      const ET9MDBCALLBACK  ET9ReadMdbData);

ET9STATUS ET9FARCALL ET9AWUnregisterMDB(ET9AWLingInfo * const pLingInfo);



/*------------------------------------------------------------------------
 *  Define ET9 Alphabetic CDB functions
 *------------------------------------------------------------------------*/

ET9STATUS ET9FARCALL ET9AWCDBInit(ET9AWLingInfo           * const pLingInfo,
                                  ET9AWCDBInfo ET9FARDATA * const pCDBInfo,
                                  const ET9U16                    wDataSize,
                                  const ET9DBWRITECALLBACK        pWriteCB);

ET9STATUS ET9FARCALL ET9AWCDBReset(ET9AWLingInfo * const pLingInfo);


/*------------------------------------------------------------------------
 *  Define ET9 Alphabetic TUDB functions
 *------------------------------------------------------------------------*/

typedef ET9STATUS (ET9FARCALL *ET9ReadTUDB_f)(ET9U8 ET9FARDATA * const pData, ET9U16 wSize, ET9U8 ET9FARDATA * const pTUDB, ET9U32 dwOffset);
typedef ET9STATUS (ET9FARCALL *ET9WriteTUDB_f)(ET9U8 ET9FARDATA * const pData, ET9U16 wSize, ET9U8 ET9FARDATA * const pTUDB, ET9U32 dwOffset);

#define ET9_TUDBTYPE_NONE               0
#define ET9_TUDBTYPE_RUDB               1
#define ET9_TUDBTYPE_LASDB              2
#define ET9_TUDBTYPE_UASDB              3

#define ET9TUDB_INVALID_REC             0
#define ET9TUDB_T9CUSTOMWORD_REC        1
#define ET9TUDB_T9REORDERWORD_REC       2
#define ET9TUDB_CUSTOMWORD_REC          3
#define ET9TUDB_REORDERWORD_REC         4
#define ET9TUDB_LDBAUTOSUB_REC          5
#define ET9TUDB_USERAUTOSUBWORD_REC     6


#define ET9TUDB_MEMMINSIZE 256

typedef struct ET9AWTUDBNextRecord_s {
    ET9U8   bDBType;        /**< Next exported DB record */
    ET9U16  wDBOffset;      /**< Pointer to next exported DB offset */

} ET9AWTUDBNextRecord;

ET9STATUS ET9FARCALL ET9AWTUDBGetSize(ET9AWLingInfo * const pLingInfo,
                                      ET9U32        * const pdwSize,
                                      ET9U32        * const pdwTotalWords);

ET9STATUS ET9FARCALL ET9AWTUDBExport(ET9AWLingInfo       * const pLingInfo,
                                     ET9U8 ET9FARDATA    * const pTUdb,
                                     const ET9U32                dwTUDBSize,
                                     ET9U32              * const pdwExportSize,
                                     ET9WriteTUDB_f              ET9WriteTUDB,
                                     ET9AWTUDBNextRecord * const pNextRecord,
                                     ET9U16              * const pwRecordsExported);

ET9STATUS ET9FARCALL ET9AWTUDBImport(ET9AWLingInfo    * const pLingInfo,
                                     ET9U8 ET9FARDATA * const pTUdb,
                                     const ET9U32             dwTUdbSize,
                                     ET9ReadTUDB_f            ET9ReadTUDB,
                                     ET9U16           * const pwTotalImported,
                                     ET9U16           * const pwTotalRejected);


/** \internal
 * Word source definition.
 * THESE ENTRIES MUST REMAIN IN THIS ORDER.<P>
 * Buildarounds and compounds must have identical order and sequence as "normal".
 */

typedef enum ET9WORDSRC_E {

    ET9WORDSRC_NONE,                        /**< \internal  0 */
    ET9WORDSRC_EXACT,                       /**< \internal  1 */
    ET9WORDSRC_EXACTISH,                    /**< \internal  2 */

    ET9WORDSRC_SIMPLE_START = 5,            /**< \internal    */

    ET9WORDSRC_CSP,                         /**< \internal 06 */
    ET9WORDSRC_CDB,                         /**< \internal 07 */
    ET9WORDSRC_RUDB,                        /**< \internal 08 */
    ET9WORDSRC_LDB_PROMOTION,               /**< \internal 09 */
    ET9WORDSRC_MDB,                         /**< \internal 10 */
    ET9WORDSRC_ASDB_SHORTCUT,               /**< \internal 11   user-defined auto-substitution shortcut */
    ET9WORDSRC_LAS_SHORTCUT,                /**< \internal 12   LDB-supported auto-substitution shortcut */
    ET9WORDSRC_LDB,                         /**< \internal 13 */
    ET9WORDSRC_QUDB,                        /**< \internal 14 */
    ET9WORDSRC_KDB,                         /**< \internal 15   not considered buildable for chktxt */


    ET9WORDSRC_BUILDAROUND_START = 16,      /**< \internal    */

    ET9WORDSRC_BUILDAROUND_CSP,             /**< \internal 17 */
    ET9WORDSRC_BUILDAROUND_CDB,             /**< \internal 18 */
    ET9WORDSRC_BUILDAROUND_RUDB,            /**< \internal 19 */
    ET9WORDSRC_BUILDAROUND_LDB_PROMOTION,   /**< \internal 20 */
    ET9WORDSRC_BUILDAROUND_MDB,             /**< \internal 21 */
    ET9WORDSRC_BUILDAROUND_ASDB,            /**< \internal 22   user-defined auto-substitution shortcut */
    ET9WORDSRC_BUILDAROUND_LAS,             /**< \internal 23   LDB-supported auto-substitution shortcut */
    ET9WORDSRC_BUILDAROUND_LDB,             /**< \internal 24 */
    ET9WORDSRC_BUILDAROUND_QUDB,            /**< \internal 25 */
    ET9WORDSRC_BUILDAROUND_KDB,             /**< \internal 26 */


    ET9WORDSRC_BUILDCOMPOUND_START = 27,    /**< \internal    */

    ET9WORDSRC_BUILDCOMPOUND_CSP,           /**< \internal 28 */
    ET9WORDSRC_BUILDCOMPOUND_CDB,           /**< \internal 29 */
    ET9WORDSRC_BUILDCOMPOUND_RUDB,          /**< \internal 30 */
    ET9WORDSRC_BUILDCOMPOUND_LDB_PROMOTION, /**< \internal 31 */
    ET9WORDSRC_BUILDCOMPOUND_MDB,           /**< \internal 32 */
    ET9WORDSRC_BUILDCOMPOUND_ASDB,          /**< \internal 33   user-defined auto-substitution shortcut */
    ET9WORDSRC_BUILDCOMPOUND_LAS,           /**< \internal 34   LDB-supported auto-substitution shortcut */
    ET9WORDSRC_BUILDCOMPOUND_LDB,           /**< \internal 35 */
    ET9WORDSRC_BUILDCOMPOUND_QUDB,          /**< \internal 36 */
    ET9WORDSRC_BUILDCOMPOUND_KDB,           /**< \internal 37 */


    ET9WORDSRC_MISC_START = 40,             /**< \internal    */

    ET9WORDSRC_CAPTURE,                     /**< \internal 41 */
    ET9WORDSRC_TERMPUNCT,                   /**< \internal 42 */
    ET9WORDSRC_COMPLETIONPUNCT,             /**< \internal 43 */
    ET9WORDSRC_BUILDAPPENDPUNCT,            /**< \internal 44 */
    ET9WORDSRC_REQUIRED,                    /**< \internal 45 */
    ET9WORDSRC_STEMPOOL,                    /**< \internal 46   should be the last source before protected ones */
    ET9WORDSRC_MAGICSTRING,                 /**< \internal 47   protected */
    ET9WORDSRC_STEM,                        /**< \internal 48   protected */
    ET9WORDSRC_BUILDAPPEND,                 /**< \internal 49   protected */
    ET9WORDSRC_AUTOAPPEND,                  /**< \internal 50   protected */

    ET9WORDSRC_LAST                         /**< \internal sentinel, keep last */

} ET9WORDSRC;

#define EXACTOFFSET    0x40                 /**< \internal exact source offset (should be an exclusive bit) */
#define EXACTISHOFFSET 0x80                 /**< \internal exactish source offset (should be an exclusive bit) */

/*------------------------------------------------------------------------
 *  Define internals
 *------------------------------------------------------------------------*/

ET9STATUS ET9FARCALL ET9AWSysSetAltMode(ET9AWLingInfo       * const pLingInfo,
                                        const ET9AW_AltMode         eMode);


/*----------------------------------------------------------------------------
 *  End: Linguistic Module  ** ALWAYS IN **
 *----------------------------------------------------------------------------*/

#ifdef ET9CHECKCOMPILE
enum {
    ET9WRONGVALUE_ET9MAXSELLISTSIZE = ET9CHECKCOMPILE_NEXT,
    ET9WRONGVALUE_ET9MAXWORDSIZE,
    ET9WRONGVALUE_ET9MAXUDBWORDSIZE,
    ET9WRONGSIZE_ET9AWWORDINFO,
    ET9WRONGSIZE_ET9AWLINGINFO,
    ET9WRONGSIZE_ET9AWLINGCMNINFO
};


ET9U32 ET9FARCALL ET9AWCheckCompileParameters(ET9U8   *pbET9U8,
                                              ET9U8   *pbET9U16,
                                              ET9U8   *pbET9U32,
                                              ET9U8   *pbET9UINT,
                                              ET9U8   *pbET9S8,
                                              ET9U8   *pbET9S16,
                                              ET9U8   *pbET9S32,
                                              ET9U8   *pbET9INT,
                                              ET9U8   *pbET9SYMB,
                                              ET9U8   *pbET9BOOL,
                                              ET9U8   *pbET9FARDATA,
                                              ET9U8   *pbET9FARCALL,
                                              ET9U8   *pbET9LOCALCALL,
                                              ET9U8   *pbVoidPtr,
                                              ET9U16  *pwET9SymbInfo,
                                              ET9UINT  *pwET9WordSymbInfo,
                                              ET9U8   *pbET9MAXSELLISTSIZE,
                                              ET9U8   *pbET9MAXWORDSIZE,
                                              ET9U8   *pbET9MAXUDBWORDSIZE,
                                              ET9U16  *pwET9AWWordInfo,
                                              ET9U16  *pwET9AWLingInfo,
                                              ET9U16  *pwET9AWLingCmnInfo);
#endif

#endif /* ET9_ALPHABETIC_MODULE */


/*! \mainpage

\section intro_sec Release Summary (pre-release notes)

-- none

*/

/* End don't mangle the function name if compile under C++ */
#if defined (__cplusplus)
    }
#endif

/*! @} */
#endif /* #ifndef ET9AWAPI_H */
/* ----------------------------------< eof >--------------------------------- */

