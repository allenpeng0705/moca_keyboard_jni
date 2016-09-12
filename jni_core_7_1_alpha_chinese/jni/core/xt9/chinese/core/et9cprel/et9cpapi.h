/*******************************************************************************
;*******************************************************************************
;**                                                                           **
;**                    COPYRIGHT 2004-2011 NUANCE COMMUNICATIONS              **
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
;**     FileName: et9cpapi.h                                                  **
;**                                                                           **
;**  Description: Chinese XT9 API Interface Header File.                      **
;**               Conforming to the development version of Chinese XT9.       **
;**                                                                           **
;*******************************************************************************
;******************************************************************************/

#ifndef ET9CPAPI_H
#define ET9CPAPI_H 1

/*! \addtogroup et9cpapi API for Chinese XT9
* The API for Chinese XT9.
* @{
*/

/* don't mangle the function name if compile under C++ */
#if defined(__cplusplus)
extern "C" {
#endif

#include "et9api.h"
#ifdef ET9_CHINESE_MODULE

#if defined(ET9_KDB_TRACE_MODULE) && !defined(ET9_ALPHABETIC_MODULE)
#error ET9_KDB_TRACE_MODULE is defined but ET9_ALPHABETIC_MODULE is not defined
#endif

#define ET9_CP_MAX_LDB_PHRASE_SIZE      6 /**< \internal max phrase length in Chinese XT9 LDB */
#define ET9_CP_MAX_SINGLE_SYL_SIZE      7 /**< \internal max letters per syllable including tone */

#define ET9_CP_MAX_STROKE_PER_CHAR      55 /**< \internal max strokes per character */
#define ET9_CP_MAX_SYL_COUNT            419 /**< \internal max unique syllables in any Chinese XT9 LDB */
#define ET9_CP_ID_RANGE_SIZE            3 /**< \internal size of an ID range */
#define ET9_CP_MAX_ID_RANGE_PER_SYL     6 /**< \internal max ID ranges per syllable */
#define ET9_CP_MAX_CONTEXT_HISTORY      2 /**< \internal max selection history that can be remembered */
#define ET9_CP_MAX_ALT_SYLLABLE         8 /**< \internal max number of alt syllables/strokes (including the most common one) */
#define ET9_CP_NOT_ENCODED_SYMBOL       0xFFFF /**< \internal value of not-encoded symbol */
#define ET9_CP_MAX_PREFIX_COUNT         256 /**< \internal max syllable prefix count */

/*----------------------------------------------------------------------------
 *  Chinese XT9 public constants.
 *----------------------------------------------------------------------------*/
#define ET9CPSYLLABLEDELIMITER          0x1a  /**< syllable delimiter symbol in the spelling/strokes */
#define ET9CP_SEGMENT_DELIMITER         0x19  /**< segment delimiter symbol in the sentence based input */

#ifndef ET9CPMAXUDBPHRASESIZE
#define ET9CPMAXUDBPHRASESIZE           16 /**< max UDB phrase length, it can be defined as any value between 6 and 16 */
#elif ET9CPMAXUDBPHRASESIZE < 6 || ET9CPMAXUDBPHRASESIZE > 16
#error ET9CPMAXUDBPHRASESIZE must be >= 6 and <= 16
#endif

#define ET9CPMAXPHRASESIZE              32 /**< max phrase length supported by Chinese XT9 */
#define ET9CPMAXSPELLSIZE               (ET9CPMAXPHRASESIZE * ET9_CP_MAX_SINGLE_SYL_SIZE)   /**< max length of spelling in Chinese XT9 */

#ifndef ET9CPMAXPAGESIZE
#define ET9CPMAXPAGESIZE                30   /**< number of phrases that will be cached per internal page, it can be defined as any value between 1 and 200 but acceptable speed is not guaranteed for values beyond the default */
#elif (ET9CPMAXPAGESIZE < 1 || ET9CPMAXPAGESIZE > 200)
#error ET9CPMAXPAGESIZE must be between 1 and 200 /* validates technical limits but acceptable speed is not guaranteed for values beyond the default */
#endif

#define ET9CPUDBMINSIZE                 8192    /**< min size of UDB in Chinese XT9 */
#define ET9CPBpmfLetterCount            37      /**< BPMF Symbol count */
#define ET9CPPinyinLetterCount          26      /**< Pinyin Letter count */





/**< \internal
  bit flags of user options ET9CPLingInfo::bState */
typedef enum {
    ET9_CP_STATE_COMPONENT = 1,        /**< \internal state bit for component support */
    ET9_CP_STATE_SMARTPUNCT = 2,       /**< \internal state bit for smart punctuation support */
    ET9_CP_STATE_NAME_INPUT = 4,       /**< \internal state bit for name input support */
    ET9_CP_STATE_PARTIAL_SPELL = 8,    /**< \internal state bit for partial spelling support */
    ET9_CP_STATE_COMMON_CHAR = 16,      /**< \internal state bit for common char support */
    ET9_CP_STATE_FULL_SENTENCE = 32      /**< \internal state bit for full sentence support */
} ET9_CP_State;

/** \internal
  enum for ID encoding */
typedef enum {
    ET9_CP_IDEncode_PID,    /**< \internal Pinyin ID encoding */
    ET9_CP_IDEncode_BID,    /**< \internal BPMF ID encoding */
    ET9_CP_IDEncode_SID,    /**< \internal Stroke ID encoding */
    ET9_CP_IDEncode_UNI     /**< \internal Unicode encoding */
} ET9_CP_IDEncode;

/** \internal
  enum for spell match */
typedef enum {
    eNoMatch,
    eExactMatch,
    ePartialMatch
} ET9_CP_SpellMatch;

/** \internal
  enum for CangJie type */
typedef enum {
    eCangJieVer3,
    eCangJieVer5,
    eCangJieV3HK,
    eCangJieV5HK
} ET9_CP_CangJieType;

/**
 * Chinese XT9 input mode
 */
typedef enum {
    ET9CPMODE_PINYIN = 0,        /**< PinYin mode */
    ET9CPMODE_BPMF,              /**< BoPoMoFo mode */
    ET9CPMODE_STROKE,            /**< Stroke mode */
    ET9CPMODE_CANGJIE,           /**< CangJie mode */
    ET9CPMODE_QUICK_CANGJIE      /**< Quick CangJie mode */
} ET9CPMode;

/**
 * Stroke symbols and Tone symbols
*/
typedef enum {
    ET9CPSTROKE1 = 1,               /**< 1 -- stroke 1 */
    ET9CPSTROKE2,                   /**< 2 -- stroke 2 */
    ET9CPSTROKE3,                   /**< 3 -- stroke 3 */
    ET9CPSTROKE4,                   /**< 4 -- stroke 4 */
    ET9CPSTROKE5,                   /**< 5 -- stroke 5 */
    ET9CPSTROKEWILDCARD,            /**< 6 -- stroke wildcard */

    ET9CPTONE1 = 0x1B,              /**< 0x1B -- tone 1 */
    ET9CPTONE2,                     /**< 0x1C -- tone 2 */
    ET9CPTONE3,                     /**< 0x1D -- tone 3 */
    ET9CPTONE4,                     /**< 0x1E -- tone 4 */
    ET9CPTONE5                      /**< 0x1F -- tone 5 */
} ET9CPSYMB;

/**
 * Special phrase types for modeless input UI
 */
typedef enum {
    ET9CPPhraseType_Unknown = 0, /**< unknown type for error handling, This has to be the first item */

    ET9CPPhraseType_ExactInput,  /**< exact input from WordSymbInfo */
    ET9CPPhraseType_Num,         /**< digit interpretation of WordSymbInfo */
    ET9CPPhraseType_Sym,         /**< punctuations from WordSymbInfo */
    ET9CPPhraseType_abc,         /**< words in lower case from alphabetic XT9 */
    ET9CPPhraseType_Abc,         /**< words in Upper case from alphabetic XT9 */
    ET9CPPhraseType_ABC,         /**< words in ALL CAPS from alphabetic XT9 */

    ET9CPPhraseType_LAST         /**< This has to be the last item */
} ET9CPPhraseType;

/**
 * Contents of ET9CPLingInfo should only be accessed through API function and MACROS
*/
typedef struct ET9_CP_LingInfo_s ET9CPLingInfo;

/**
 * Contents of ET9CPUdbInfo should only be accessed through API function and MACROS
*/
typedef struct ET9_CP_UdbInfo_s  ET9CPUdbInfo;

#ifdef ET9_DIRECT_LDB_ACCESS

/** Integration defined callback function for reading LDB data.
 *
 * @param pLingInfo            pointer to Chinese linguistic information structure
 * @param ppbSrc               pointer to a pointer to the integration supplied buffer
 * @param pdwSizeInBytes       pointer to enter LDB size in bytes
*/
typedef ET9STATUS (ET9FARCALL *ET9CPDBREADCALLBACK)(
    ET9CPLingInfo          *pLingInfo,              /**< pointer to Chinese information structure */
    ET9U8 * ET9FARDATA *    ppbSrc,                 /**< pointer to a pointer to the integration supplied buffer */
    ET9U32                 *pdwSizeInBytes          /**< pointer to enter LDB size in bytes  */
);

#else /* not ET9_DIRECT_LDB_ACCESS */

/** Integration defined callback function for reading LDB data.
 *
 * @param pLingInfo            pointer to Chinese linguistic information structure
 * @param dwOffset             offset to read from
 * @param dwBytesToRead        number of bytes to read
 * @param pbDst                buffer for the data
 * @param pdwBytesRead         pointer to the variable that receives the number of bytes read
*/
typedef ET9STATUS (ET9FARCALL *ET9CPDBREADCALLBACK)(
    ET9CPLingInfo *pLingInfo,                       /**< pointer to Chinese linguistic information structure */
    ET9U32         dwOffset,                        /**< offset to read from */
    ET9U32         dwBytesToRead,                   /**< number of bytes to read */
    ET9U8         *pbDst,                           /**< buffer for the data */
    ET9U32        *pdwBytesRead                     /**< pointer to the variable that receives the number of bytes read */
);

#endif /* END not ET9_DIRECT_LDB_ACCESS */

/** Integration defined callback function for writing data to UDB.
 *
 * @param pPublicExtension     user data that was passed into ET9CPSysInit
 * @param pbDest               buffer for the data to be written
 * @param pbSource             pointer to the data source
 * @param unNumberOfWriteBytes number of bytes to be written
 */
typedef ET9STATUS (ET9FARCALL *ET9CPDBWRITECALLBACK)(
    void ET9FARDATA  *pPublicExtension,             /**< user data that was passed into ET9CPSysInit */
    ET9U8 ET9FARDATA *pbDest,                       /**< buffer for the data to be written */
    const ET9U8 ET9FARDATA *pbSource,               /**< pointer to the data source */
    ET9UINT  unNumberOfWriteBytes                   /**< number of bytes to be written */
);

#if ET9SYMBOLWIDTH != 2
#error ET9SYMBOLWIDTH has to be 2 in Chinese XT9
#endif

/** \internal
  internal spelling structure */
typedef struct {
    ET9U8    pbChars[ET9CPMAXSPELLSIZE];
    ET9U8    bLen;
} ET9_CP_Spell;

/** \internal
  cache for Unicode to spelling lookup */
typedef struct {
    ET9SYMB      sUnicode;                                  /* cached unicode last looked up */
    ET9U16       awPIDs[ET9_CP_MAX_ALT_SYLLABLE];           /* cached alternate PIDs of this unicode, sorted high to low freq */
    ET9U8        bNumAlternates;                            /* the number of alternate PIDs for cached unicode */
} ET9_CP_SpellLookupCache;

/** \internal
  private data for Phonetic input */
typedef struct ET9_CP_PPrivate_s {
    ET9_CP_SpellLookupCache sLookupCache;                       /* a cache of unicode to PID reverse lookup */
    ET9U32      adwPinyinBinTable[ET9_CP_MAX_SYL_COUNT];
    ET9U32      adwBpmfBinTable[ET9_CP_MAX_SYL_COUNT];
    ET9U16      awSylPidTable[ET9_CP_MAX_SYL_COUNT+1];          /* syl count + 1 for end PID of last syl */
    ET9BOOL     bInputHasTone;                                  /* current input contains tone */
} ET9_CP_PPrivate;


#ifndef ET9CP_DISABLE_STROKE
/** \internal
  private data for Cangjie input */
typedef struct {
    ET9U16 wSid;
    ET9U16 wFreq;
} ET9_CP_SidFreq;

#define ET9_CP_MAX_CANG_JIE_CAND        1500 /**< \internal max candidates in any Cangjie input */

typedef struct {
    ET9_CP_SidFreq aSidFreq[ET9_CP_MAX_CANG_JIE_CAND];
    ET9U16 wExactCount;
    ET9U16 wTotalCount;
} ET9_CP_CangJieResultArray;

typedef struct ET9_CP_CJ_Private_s {                          /* CangJie internal buffers */
    ET9U8    bCangJieLDBVersion;
    ET9_CP_CangJieType  eCangJieType;
    ET9U32   nCangJieSidCount;
    ET9U16   awSubTreeOffsets[27];    /* awSubTreeOffsets[i] is of starting byte-offset of ('A' + i)_subtree from the beginning of CangJie_Unicode array */
    ET9U32   nCangJieSidOffset;   /* Starting byte-offset of CangJie_Sid array from the beginning of CangJie Data in LDB */
    ET9U32   nUnicodeSortedCIDOffset; /* Starting byte-offset of UnicodeSorted Indeces (of CangJie_Sid array) from the beginning of CangJie Data in LDB */
    ET9_CP_CangJieResultArray   sCangJieResultArray;
} ET9_CP_CPrivate;

/** \internal
  private data for Stroke input */
#define ET9_CP_STROKE_NUMOFSTROKE   5   /* number of stroke, Nuance has only 5 stroke system */
#define ET9_CP_STROKE_MAXNODES      5   /* best tree depth in the current design */
#define ET9_CP_STROKE_MAX_MASK  (((ET9_CP_MAX_STROKE_PER_CHAR - ET9_CP_STROKE_MAXNODES) * 3 + 7) / 8)   /* number of member for the array nStrokeMask, need at least one more byte */

typedef struct {
    ET9U8        bStrokeMask[ET9_CP_STROKE_MAX_MASK];         /* last character's bit mask for the remaining stroke comparison */
    ET9U8        bStrokePack[ET9_CP_STROKE_MAX_MASK];         /* last character's packed remaining strokes, the first stroke is in the byte of (ET9U8*)nStrokePack */
    ET9U8        bActStrokes[ET9_CP_STROKE_MAXNODES];         /* the stroke sequence for internal use, mainly for parital match and wild card */
    ET9U32       dwNodeOffsets[ET9_CP_STROKE_MAXNODES + 1];   /* offsets of the nodes */
    ET9U16       wCurNodeSIDBase[ET9_CP_STROKE_MAXNODES + 1]; /* stroke ID bases of exact and partial matches for the nodes */
    ET9U16       wCurTotalMatch[ET9_CP_STROKE_MAXNODES + 1];  /* number of total matches of the current node */ /* wCurTotalMatch[0] is the total number of normal SID */
    ET9U8        bIsRangeExact;                              /* indicating if all the characters in the range are matches */

    /* component related fields */
    ET9U32       dwOffsetComponent;                  /* the offset of component module */
    ET9U16       wCompUnicode;                       /* selected component unicode, 0 means no component */
    ET9U8        bCompNumStrokes;                    /* the number of strokes of the component */

    /* UDB search related fields */
    ET9U16       wUdbLastOffset;                     /* stop offset of last Udb search */
    ET9U16       wUdbLastWordCount;                  /* remaining word count of last Udb search */
    ET9U16       wUdbLastRangeEnd;                   /* range end of last Udb search; init to 0xFFFF */
} ET9_CP_SPrivate;
#endif /* END not ET9CP_DISABLE_STROKE */

/** \internal
  Chinese XT9 private data */
typedef struct {
    ET9U16 wInfoInitOK;
    ET9U16 wLDBInitOK;
#ifndef ET9CP_DISABLE_STROKE
    ET9_CP_SPrivate SPrivate;
    ET9_CP_CPrivate CPrivate;
#endif
    ET9_CP_PPrivate PPrivate;
#ifdef ET9_DIRECT_LDB_ACCESS
    ET9U8  ET9FARDATA   *pLdbData;              /**< pointer to a byte array holding the whole LDB */
    ET9U32              dwLdbDataSize;          /**< size of the whole LDB */
#endif /* ET9_DIRECT_LDB_ACCESS */
} ET9_CP_Private;

/** \internal
  Chinese XT9 User database: constants, structure and macros */
#define ET9_CP_UDBBPMFZONES           ET9CPBpmfLetterCount      /* based on BPMF letter */
#define ET9_CP_UDBPINYINZONES         ET9CPPinyinLetterCount    /* based on Pinyin letter */
#if (ET9_CP_UDBBPMFZONES > ET9_CP_UDBPINYINZONES)
#define ET9_CP_UDBPHONETICZONES       ET9_CP_UDBBPMFZONES
#else
#define ET9_CP_UDBPHONETICZONES       ET9_CP_UDBPINYINZONES
#endif
#define ET9_CP_UDBSTROKEZONES         7       /* based on SID even distribution. Also allow the worst case: 8-byte struct alignment. */
#define ET9_CP_UDBZONEMAX             (ET9_CP_UDBPHONETICZONES + ET9_CP_UDBSTROKEZONES)

struct ET9_CP_UdbInfo_s {
    ET9U16 wDataSize;        /* Total size in bytes of the database */
    ET9U16 wDataCheck;       /* Additive checksum of database */
    ET9U32 dwDirtyCount;     /* Write counter for the last write. (multi-thread support) */
    ET9U16 wUpdateCount;     /* Count incremented when UDB is updated */
    ET9U16 wFreeBytes;       /* Total number of free bytes in the UDB */
    ET9U16 wCutOffFreq;      /* Freqency for cleaning up UDB entries when overcrowded */
    ET9U16 wLdbCompatibility;/* Store LDB compatibility info */
    ET9U16 wZoneOffset[ET9_CP_UDBZONEMAX];   /* Offsets to beginning of each zone. */
    ET9U16 wZoneWordCount[ET9_CP_UDBZONEMAX];/* Word counts in each zone. */
    ET9U8  bDataArea[1];     /* Beginning of data area */
};

typedef struct {
    ET9U32 dwDirtyCount;    /* cache of Udb dirty count at the last build selection list (multi-thread support) */
    ET9U32 dwLastGetDC;     /* cache of Udb dirty count at the last Udb get phrase */
    ET9U16 wLastGetIndex;   /* cache of index at the last Udb get phrase */
    ET9U16 wLastGetOffset;  /* cache of Udb entry offset at the last Udb get phrase */
    ET9U16 wLastTypeMask;   /* cache of Udb phrase type mask at the last Udb get phrase */
} ET9_CP_UdbReadCache;

/** \internal
  Cache for important LDB offsets */
typedef struct {
    ET9U32       dwBpmfLDBOffset;            /* BPMF LDB module offset */
    ET9U32       dwPinyinLDBOffset;          /* pinyin LDB module offset */
    ET9U32       dwWordLDBOffset;            /* word module offset */
    ET9U32       dwStrokeLDBOffset;          /* stroke LDB module offset */
    ET9U32       dwPIDToUnicodeOffset;       /* PID to Unicode lookup table offset */
    ET9U32       dwUnicodeSortedPIDOffset;   /* PIDs sorted in their unicode order */
    ET9U32       dwFrequencyOffset;          /* PID to frequency lookup table offset */
    ET9U32       dwPIDToSIDOffset;           /* PID to SID lookup table */
    ET9U32       dwSIDToPIDOffset;           /* SID to PID lookup table */
    ET9U32       dwSmartPunctOffset;         /* Smart punct bigram table */
    ET9U32       dwContextCommonCharOffset;  /* Context Common character lookup table */
    ET9U32       dwNoContextCommonCharOffset;/* NoContext Common character lookup table */
    ET9U32       dwPhraseGroupAddrTableOffset;/* Phrase group address table */
    ET9U32       dwPhraseDataOffset;         /* Phrase data starting offset */
    ET9U32       dwSyllableBIDOffset;        /* syllable - BPMF ID range lookup table offset */
    ET9U32       dwSyllablePIDOffset;        /* syllable - Pinyin ID range lookup table offset */
    ET9U32       dwToneOffset;               /* PID/BID to tone lookup table offset */
    ET9U32       dwNameTableOffset;          /* name character table offset */
    ET9U32       dwSBITrieOffset;            /* SBI Trie offset */
    ET9U32       dwCangJieOffset;            /* CangJie table offset */
    ET9U32       dwConvTableOffset;           /* Simplified-Traditional Chinese conversion table offset */
} ET9_CP_LdbOffsets;

/** \internal
  Phrase buffer: Base struct */
typedef struct {
    ET9U16       wBufSize;                          /* the size of phrase buffer in bytes */
    ET9U16       wUsedBufSize;                      /* the number of bytes used in phrase buffer */
    ET9U16       wLastTotal;                        /* the current phrase index in the buffer, 1-based */
    ET9U16       wTotal;                            /* total number of phrases in the phrase buffer */
    ET9U16       wPrevFreq;                         /* the max freqency in the current sorting page */
    ET9U16       wPrevPos;                          /* the position of the last entry in the previous page.  It should be the address for UDB and the leading PID for LDB */
    ET9U16       pwFreq[ET9CPMAXPAGESIZE];          /* the frequency information for the current page */
    ET9U16       wCurrentPageStart;                 /* the byte offset of the start of the current page */
    ET9U8        bPageSize;                         /* the current page size ET9CPMAXPAGESIZE/#of characters per phrase */
    ET9U8        bTestClear;                        /* if this is set to true, the buffer is empty */
    ET9U8        bIsDelimiterExpansion;             /* this is set to 1 when setting active spelling, adding delimiter or deleting keys.
                                                       it is set to 0 when expansion fails */
    ET9U8        pbBuf[1];                          /* virtual buffer of base struct, must be the last field */
} ET9_CP_PhraseBuf;

/* Derived struct from PhraseBuf */
#define ET9_CP_STD_PHRASE_BUF_SIZE      8192
typedef struct ET9_CP_StdPhraseBuf_s {
    ET9_CP_PhraseBuf sPhraseBuf;
    ET9U8           pbDataBuf[ET9_CP_STD_PHRASE_BUF_SIZE]; /* actual buffer for storage, must be immediately after the base struct */
} ET9_CP_StdPhraseBuf;

typedef struct {
    ET9U8  bOrigIndex;
    ET9U8  bStartIndex;
    ET9U8  bPfxLen;
    ET9U8  bPfxCount;
    ET9S32 iProb;
} ET9_CP_PrefixGroup;

/** \internal
  ET9_CP_SBISpellData: Information about SBI spellings from segmentation/prefix buffer */
typedef struct {
    /* variables specific to each spell */
    ET9BOOL             fSearchingSegment;
    ET9BOOL             fSearchingSegmentLen;
    ET9BOOL             fSearchingSegment1stSylLen;
    ET9BOOL             fSearchingLastSegment; /* searching through the end of user's input (may need to try partial syllable) */
    ET9BOOL             fSearchingSetPrefix;
    ET9BOOL             fEndsWithInitial; /* ends with a syllable start, so *may* apply p.p. */
    ET9BOOL             fPrefixSyllablesAligned;
    ET9INT              nFirstPartialPinyin;
    ET9INT              nSyllableCount;

    /* variables related to the segmentation */
    ET9U16              wSegPhraseFreq; /* the frequency of the segmentation's best phrase */
    ET9BOOL             fSegmentFull; /* the segmentation is full (not p.p.) */
} ET9_CP_SBISpellData;

#ifdef ET9_ALPHABETIC_MODULE
/** \internal
  ET9_CP_TraceSpellData: Information about AW trace spellings */
typedef struct {
    ET9AWPrivWordInfo * psAWPrivWord;   /* spelling found by alpha core */
    ET9U8               bSpellIndex;    /* index of spelling */
    ET9U16              wPhraseIndex;   /* index of phrase */
    ET9BOOL             fEndsWithInitial; /* ends with a syllable start, so *may* apply p.p. */
} ET9_CP_TraceSpellData;
#endif

/**< \internal
  bit flags of user options ET9CPLingInfo::bState */
typedef enum {
    ET9_CP_SpellSource_None = 0,    /**< \internal no spell data */
    ET9_CP_SpellSource_SBI,         /**< \internal spell is from SBI */
    ET9_CP_SpellSource_Trace,       /**< \internal spell is from trace */
    ET9_CP_SpellSource_Max          /**< \internal sentry */
} ET9_CP_SpellSource;

/** \internal
  ET9_CP_SpellData: Information about the spelling currently being searched for phrases */
typedef struct {
    ET9_CP_SpellSource eSpellSource;
    ET9_CP_Spell        sSpell;

    union {
        ET9_CP_SBISpellData sSBI;
#ifdef ET9_ALPHABETIC_MODULE
        ET9_CP_TraceSpellData sTrace;
#endif
    } u;
} ET9_CP_SpellData;

/** \internal
  SpecialPhraseData: Information for special phrase module */
typedef struct {
    ET9CPPhraseType     eLastAlphaPhraseType;
} ET9_CP_SpecialPhraseData;

#define ET9_CP_MAX_ID_RANGE_SIZE        (ET9CPMAXPHRASESIZE * ET9_CP_MAX_ID_RANGE_PER_SYL * ET9_CP_ID_RANGE_SIZE) /**< \internal max size of the internal ID range array */

#define ET9_CP_MAXKEY     ET9MAXWORDSIZE      /* ET9SymbInfo array size in ET9WordSymbInfo */

#define ET9_CP_MAX_PHRASE_DATA_BLOCK_COUNT  32 /**< \internal max count of phrase data block */

/*-- Chinese XT9 common data */
typedef struct ET9_CP_Common_s {              /* common data fields between stroke and pheontic modules */
    ET9U16      wBpmfSylCount;              /* total number of BPMF syllables in the current LDB */
    ET9U16      wPinyinSylCount;            /* total number of Pinyin syllables in the current LDB */
    ET9U16      wMohuFlags;                 /* the mohu pinyin flags, see, for example ET9CPMOHU_PAIR_Z_ZH_MASK */
    ET9U16      w1stSymbolPID;              /* The first PID after the Normal PID range, beginning the Symbol PID range */
    ET9U16      w1stMuteCharPID;            /* The first PID after the Symbol PID range, beginning the Mute PID range */
    ET9U16      w1stComponentPID;           /* The first PID after the Mute PID range, beginning the Component PID range */
    ET9U16      wComponentFirst;            /* unicode of the first component */
    ET9U16      wComponentLast;             /* unicode of the last component */
    ET9U16      pwRange[ET9_CP_MAX_ID_RANGE_SIZE];  /* the PID/SID ranges for all syllables in the input */
    ET9U8       pbRangeEnd[ET9CPMAXPHRASESIZE];     /* the ending index in pwRange for each syllable (exclusive) */
    ET9U8       bSylCount;                          /* the number of syllables in the input */

    ET9U16      pwContextBuf[ET9CPMAXPHRASESIZE - 1];       /* the context buffer in PID/SID.  We save up to (ET9CPMAXPHRASESIZE - 1) characters or ET9_CP_MAX_CONTEXT_HISTORY phrases, whichever has fewer characters */
    ET9U8       pbContextLen[ET9_CP_MAX_CONTEXT_HISTORY + 1];   /* the lengths of the context units in pwContextBuf. 0-terminated */
    ET9U8       bExtContext;                /* context was set externally */
    ET9U16      pwMaxFirstPIDInBlock[ET9_CP_MAX_PHRASE_DATA_BLOCK_COUNT]; /* max First PID in each phrase data blocks */
    ET9U8       bKeyBuf[ET9_CP_MAXKEY + 1];   /* key input buffer, padding one more for explicit input */
    ET9U8       bKeyBufLen;                 /* length of ambiguous key sequence for stroke and phonetic */
    ET9U8       bCachedNumSymbs;            /* length of the the number of symbols that matches the spelling/phrase buffer */
    ET9U8       bFailNumSymbs;              /* length of the number of symbols that failed last stroke build */

    ET9U8          bActivePrefix;            /* 0-based index to ET9_CP_SSBITrie::PrefixBuffer, 0xFF means no active Prefix */
    ET9U8          bSyllablePrefixCount;
    ET9_CP_PrefixGroup pPrefixGroup[ET9_CP_MAX_PREFIX_COUNT];
    ET9_CP_SpellMatch  eCandMatchType;
    ET9_CP_SpellData SpellData;
    ET9_CP_SpecialPhraseData SpecialPhrase;

    ET9U8          bPrefixSortMethod;  /* sort by length / alphabetic / frequency */

    ET9_CP_Spell    sActiveSpell;           /* the active spelling: Pinyin/BPMF/Stroke/CangJie */
    ET9_CP_StdPhraseBuf sStdPhraseBuf;       /* the phrase buffer */

    ET9_CP_LdbOffsets sOffsets;        /* the data modules' offsets in the LDB */
} ET9_CP_CommonInfo;

#define ET9_CP_MAX_SBI_KEY_NUMBER     32
#define ET9_CP_MAX_SEGMENT_NUMBER     32
#define ET9_CP_MAX_CANDIDATE_LENGTH  (ET9_CP_MAX_SBI_KEY_NUMBER + ET9_CP_MAX_SEGMENT_NUMBER)

typedef struct ET9_CP_SelectionHist_s
{
    ET9SYMB psSelUnicodePhrase[ET9_CP_MAX_CANDIDATE_LENGTH];
    ET9SYMB psSelEndodedPhrase[ET9_CP_MAX_CANDIDATE_LENGTH]; /* Pid, Bid, Sid */
    ET9U8   bSelPhraseLen;
    ET9U8   pbSelSymbCount[ET9_CP_MAX_SEGMENT_NUMBER];  /* Support at most 32 selections */
    ET9U8   bSelectionCount;
} ET9_CP_SelectionHist;

/*----------------------------------------------------------------------------
 *  Chinese SBI Module
 *----------------------------------------------------------------------------*/
#define ET9_CP_MAX_TREE_DEPTH         32
#define ET9_CP_MAX_SPELL_LENGTH       ET9_CP_MAX_SBI_KEY_NUMBER

#define ET9_CP_PREFIX_BUFFER_SIZE     10000

#define  ET9_CP_CANDIDATE_COUNT      4

/*----------------------------------------------------------------------------*/
/* Prefix Module */
typedef struct ET9_CS_Prefix_s
{
    ET9U8   m_bPfxLen;
    ET9S32  m_iPfxProb;
    ET9U8   m_pcPfx[ET9_CP_MAX_SPELL_LENGTH];
} ET9_CS_Prefix;

typedef struct ET9_CP_SSBIPrefixBuffer_s
{
    ET9U8   m_bPrefixCount;
    ET9U8   m_acPrefixBuf[ET9_CP_PREFIX_BUFFER_SIZE];  /* prefix format: {ET9U8 bPfxLen; ET9S32 iPfxProb; ET9U8 pcPfx[]} */
    ET9U8 * m_pcPrefixBufUnused;
} ET9_CP_SSBIPrefixBuffer;
/* END OF Prefix Module */

typedef struct ET9_CS_Candidate_s
{
    ET9S32  m_iNodeOffset;
    ET9S32  m_iSegProb;
    ET9U8   m_bSegLen;
    ET9U8   m_pcSeg[ET9_CP_MAX_CANDIDATE_LENGTH];
} ET9_CS_Candidate;

#define ET9_CP_SBI_CANDIDATE_COUNT  1

#define ET9_CP_PINYIN_SHORT_KEY_THRSHD 0
#define ET9_CP_BPMF_SHORT_KEY_THRSHD 0

#if ET9_CP_PINYIN_SHORT_KEY_THRSHD >= ET9_CP_BPMF_SHORT_KEY_THRSHD
#define ET9_CP_SBI_SHORT_KEY_THRSHD  ET9_CP_PINYIN_SHORT_KEY_THRSHD
#else
#define ET9_CP_SBI_SHORT_KEY_THRSHD  ET9_CP_BPMF_SHORT_KEY_THRSHD
#endif

#define ET9_CP_TOTAL_CAND_NUMBER  (ET9_CP_MAX_SBI_KEY_NUMBER * ET9_CP_SBI_CANDIDATE_COUNT)

/*----------------------------------------------------------------------------*/
typedef struct ET9_CS_CandidateArray_s
{
    ET9_CS_Candidate* m_pCandidates;
    ET9U8            m_bCandidateCount;
} ET9_CS_CandidateArray;

typedef struct ET9_CS_CandidateGroup_s
{
    ET9_CP_SpellMatch    m_eMatchType;
    ET9_CS_CandidateArray m_arrExactMatch;
    ET9_CS_CandidateArray m_arrPartialMatch;
    ET9U8            m_bCandBufferSize;
    ET9U8            m_abTailMatch[ET9_CP_MAX_SBI_KEY_NUMBER];
} ET9_CS_CandidateGroup;

typedef struct ET9_CP_SSBITrie_s
{
    ET9U16           wInitOK;
    /* Header:  20 bytes in Trie in version 1 */
    ET9U32           nTrieVersion;
    ET9U32           nTrieType;
    ET9U32           nMaxSpellLen;
    ET9U32           nNodeCount;
    ET9U32           nNodeByteCount;
    ET9U32           nStrLength;
    ET9S32           iLogProbShift;

    ET9U32           nTrieNodesOffset;   /* Offset in the whole LDB, in Bytes */
    ET9U32           nTailStringOffset;  /* Offset in the whole LDB, in Bytes */
    ET9U32           nSiblingDisCount;
    ET9U32           nSiblingDisArrOffset;
    ET9U32           nTailStrOffsCount;
    ET9U32           nTailStrOffsArrOffset;

    ET9CPLingInfo *  pET9CPLingInfo;

    ET9_CS_Candidate  m_aExactMatchPool[ET9_CP_TOTAL_CAND_NUMBER];
    ET9_CS_Candidate  m_aPartialMatchPool[ET9_CP_TOTAL_CAND_NUMBER];
    ET9_CS_CandidateGroup m_aSegCandGrp[ET9_CP_MAX_SBI_KEY_NUMBER];
    ET9U8            m_bSegCandGrpCount;   /* Index to m_aSegCandGrp */
    ET9U8            m_bSBIConditionLen;  /* Index to m_aSegCandGrp */

    /* Prefix Module */
    ET9_CP_SSBIPrefixBuffer m_PrefixBuffer;
    ET9U8   m_abValidPrefix[ET9_CP_MAX_SBI_KEY_NUMBER];

    ET9U8  arrEncodingTrieToInternal[256];  /* For BPMF trie */

    ET9U16  wSegPhraseFreq; /* the frequency of the segmentation's highest frequency phrase */
} ET9_CP_SSBITrie;

#ifdef ET9_ALPHABETIC_MODULE
typedef struct {
    ET9CPLingInfo * pCLing;
    ET9AWLingInfo * pALing;

    ET9_CP_Spell sDefaultSpell;     /* spell of the default phrase after trace but haven't selected any prefix */

    ET9U8 bNumSpells;       /* number of phonetic spellings generated by ET9AWSelListBuild */
    ET9U8 bActiveSpell;     /* the currently set prefix, or 0xFF for none */

    ET9BOOL fPrefixOnly;    /* True: GetPrefix returns 1st syllable spellings, False: full phrase spellings */
    ET9BOOL fDBLoaded;      /* has an alphabetic DB been loaded for this LDB/mode? */

} ET9_CP_Trace;
#endif

/*----------------------------------------------------------------------------
 * structure ET9_CP_ConversionTable_s
 *----------------------------------------------------------------------------*/
typedef struct ET9_CP_ConversionTable_s
{
    ET9U16           wSimpTradCount;   /* Simp_Trad table size: (simp, trad) pair count */
    ET9U16           wTradSimpCount;   /* Trad_Simp table size: (trad, simp) pair count */
    ET9U32           dwSimpTradOffset;  /* Byte offset of Simp_Trad table in LDB */
    ET9U32           dwTradSimpOffset;  /* Byte offset of Trad_Simp table in LDB */
} ET9_CP_ConversionTable;

/*----------------------------------------------------------------------------
 * structure ET9_CP_LingInfo_s
 *----------------------------------------------------------------------------*/
struct ET9_CP_LingInfo_s {
    ET9BaseLingInfo    Base;            /* private ling base (must be the first item in the struct) */
    ET9_CP_Private     Private;         /* persistent memory for Chinese XT9 use */
    ET9_CP_CommonInfo  CommonInfo;      /* phonetic and stroke common data structure */
    ET9CPMode        eMode;             /* Chinese mode */
    ET9U8            bState;            /* Chinese state bit mask */
    void ET9FARDATA  *pPublicExtension; /* user data passed in at initialization */
    ET9U16           wLdbNum;           /* current language ID */
    ET9CPDBREADCALLBACK pLdbReadData;   /* callback to read Ldb data */
    ET9CPUdbInfo     ET9FARDATA *pUdb;  /* Pointer to the active UDB */
    ET9CPDBWRITECALLBACK pfUdbWrite;    /* pointer to the UDB write callback function */
    ET9_CP_UdbReadCache UdbReadCache;   /* cache of Udb read */
    ET9_CP_SelectionHist    SelHistory;
    ET9_CP_SSBITrie         SBI;
    ET9_CP_ConversionTable  ConvTable;
#ifdef ET9_ALPHABETIC_MODULE
    ET9AWLingInfo       *pAWLing;       /* point to the AW linguistic data */
    ET9_CP_Trace            Trace;
#endif
};




/*----------------------------------------------------------------------------
 *  Chinese XT9 public macros
 *----------------------------------------------------------------------------*/

/** Converts from a tone encoding (ET9CPTONE1..ET9CPTONE5) to an integer (1..5), or 0 if it's not a tone. It can be use to check if a symbol is a tone. */
#define ET9CPSymToCPTone(bSym) ((ET9U8)((((ET9UINT)(bSym) >= (ET9UINT)ET9CPTONE1) && ((ET9UINT)(bSym) <= (ET9UINT)ET9CPTONE5))?((ET9UINT)(bSym) - (ET9UINT)ET9CPTONE1 + 1):0))

#define ET9CPBpmfFirstLowerSymbol    ((ET9U16)0x3105)  /**< First BPMF lower case symbol */
#define ET9CPBpmfLastLowerSymbol     ((ET9U16)0x3129)  /**< Last BPMF lower case symbol */
                                             /* Unicode for BPMF letters: 0x3105...0x3129 inclusive */

/** First BPMF upper case symbol.
    <br>Bpmf upper case symbols are output from XT9 Chinese to represent start of a syllable.
        Do not use Bpmf Upper Case Symbols for input.<br>It can be defined as other values in the free area of unicode.
        The custom range should be >= 0x100, and should not overlap with lowercase BPMF. */
#define ET9CPBpmfFirstUpperSymbol    ((ET9U16)0xF205)  /* assigned value for uppercase BPMF letters:0xF205...0xF229 inclusive */
#define ET9CPBpmfLastUpperSymbol     ((ET9U16)(ET9CPBpmfFirstUpperSymbol + ET9CPBpmfLetterCount - 1))


#define ET9CPIsBpmfLowerCaseSymbol(symb) ( ET9CPBpmfFirstLowerSymbol <= (symb) && (symb) <= ET9CPBpmfLastLowerSymbol )  /**< Check if symb is lower case BPMF symbol  */
#define ET9CPIsBpmfUpperCaseSymbol(symb) ( ET9CPBpmfFirstUpperSymbol <= (symb) && (symb) <= ET9CPBpmfLastUpperSymbol )  /**< Check if symb is upper case BPMF symbol  */
#define ET9CPIsBpmfSymbol(symb)          (ET9CPIsBpmfLowerCaseSymbol(symb) || ET9CPIsBpmfUpperCaseSymbol(symb))                                     /**< Check if symb is a BPMF symbol  */
#define ET9CPBpmfSymbolToLower(symb)     (ET9SYMB)(ET9CPIsBpmfUpperCaseSymbol(symb)? ((symb) - ET9CPBpmfFirstUpperSymbol + ET9CPBpmfFirstLowerSymbol): (symb))   /**< convert upper case BPMF symbol to lower case, otherwise return symb unchanged */
#define ET9CPBpmfSymbolToUpper(symb)     (ET9SYMB)(ET9CPIsBpmfLowerCaseSymbol(symb)? ((symb) - ET9CPBpmfFirstLowerSymbol + ET9CPBpmfFirstUpperSymbol): (symb))   /**< convert lower case BPMF symbol to upper case, otherwise return symb unchanged */

#define ET9CPIsPinyinLowerCaseSymbol(symb)  ((symb) >= 'a' && (symb) <= 'z')  /**< Check if symb is lower case Pinyin symbol  */
#define ET9CPIsPinyinUpperCaseSymbol(symb)  ((symb) >= 'A' && (symb) <= 'Z')  /**< Check if symb is upper case Pinyin symbol  */
#define ET9CPIsPinyinSymbol(symb)           (ET9CPIsPinyinLowerCaseSymbol(symb) || ET9CPIsPinyinUpperCaseSymbol(symb))  /**< Check if symb is a Pinyin symbol  */
#define ET9CPPinyinSymbolToLower(symb)     (ET9SYMB)(ET9CPIsPinyinUpperCaseSymbol(symb)? ((symb) - 'A' + 'a'): (symb))   /**< convert upper case Pinyin symbol to lower case, otherwise return symb unchanged */
#define ET9CPPinyinSymbolToUpper(symb)     (ET9SYMB)(ET9CPIsPinyinLowerCaseSymbol(symb)? ((symb) - 'a' + 'A'): (symb))   /**< convert lower case Pinyin symbol to upper case, otherwise return symb unchanged */

#define ET9CPIsCangJieUpperSymbol(symb)     ((symb) >= 'A' && (symb) <= 'Z')  /**< Check if symb is a CangJie upper case symbol  */
#define ET9CPIsCangJieLowerSymbol(symb)     ((symb) >= 'a' && (symb) <= 'z')  /**< Check if symb is a CangJie lower case symbol  */
#define ET9CPIsCangJieSymbol(symb)          (ET9CPIsCangJieUpperSymbol(symb) || ET9CPIsCangJieLowerSymbol(symb))  /**< Check if symb is a CangJie symbol  */

#define ET9CPIsUpperCaseSymbol(symb) (ET9CPIsBpmfUpperCaseSymbol(symb) || ET9CPIsPinyinUpperCaseSymbol(symb))  /**< Check if symb is a upper case symbol  */
#define ET9CPIsLowerCaseSymbol(symb) (ET9CPIsBpmfLowerCaseSymbol(symb) || ET9CPIsPinyinLowerCaseSymbol(symb))  /**< Check if symb is a lower case symbol  */

#define ET9CPIsPhoneticSymbol(symb) (ET9CPIsBpmfSymbol(symb) || ET9CPIsPinyinSymbol(symb))                   /**< Check if symb is a BPMF or PinYin symbol  */

#define ET9CPIsStrokeSymbol(symb)     ((((ET9UINT)(symb) >= ET9CPSTROKE1) && ((ET9UINT)(symb) <= ET9CPSTROKE5)) || ((ET9UINT)(symb) == ET9CPSTROKEWILDCARD)) /**< Check if symb is a stroke symbol  */

#define ET9CPGetFirstComponent(pET9CPLingInfo) ((pET9CPLingInfo)? ((pET9CPLingInfo)->CommonInfo.wComponentFirst) : 0)  /**< returns the first Component. Return 0 if there is no component information  */
#define ET9CPGetLastComponent(pET9CPLingInfo) ((pET9CPLingInfo)? ((pET9CPLingInfo)->CommonInfo.wComponentLast) : 0)    /**< returns the last Component. Return 0 if there is no component information  */

/** enum of UDB phrase types */
typedef enum {
    ET9CPUdbPhraseType_User = 0,
    ET9CPUdbPhraseType_Auto,
    ET9CPUdbPhraseType_LAST
} ET9CPUdbPhraseType;
/* Bit flags to get different phrase types from UDB. For ET9CPUdbGetPhrase() and ET9CPUdbGetPhraseCount() */
#define ET9CPUdbPhraseType_User_MASK ((ET9U16)(1 << ET9CPUdbPhraseType_User))     /**< bit flag to get User-defined phrases from UDB */
#define ET9CPUdbPhraseType_Auto_MASK ((ET9U16)(1 << ET9CPUdbPhraseType_Auto))     /**< bit flag to get Auto-formed phrases from UDB */
#define ET9CPUdbPhraseType_ALL_MASK (ET9U16)(ET9CPUdbPhraseType_User_MASK | ET9CPUdbPhraseType_Auto_MASK)     /**< bit flag to get ALL types of phrases from UDB */

/** enum of Mohu pairs for ET9CPIsMohuPairEnabled(), used only in Simplified Chinese */
typedef enum {
    ET9CPMOHU_PAIR_Z_ZH = 0,  /**<  Z and ZH are treated the same */
    ET9CPMOHU_PAIR_C_CH,      /**<  C and CH are treated the same */
    ET9CPMOHU_PAIR_S_SH,      /**<  S and SH are treated the same */
    ET9CPMOHU_PAIR_N_L,       /**<  N and L  are treated the same */
    ET9CPMOHU_PAIR_R_L,       /**<  R and L  are treated the same */
    ET9CPMOHU_PAIR_F_H,       /**<  F and H  are treated the same */
    ET9CPMOHU_PAIR_AN_ANG,    /**<  AN and ANG are treated the same */
    ET9CPMOHU_PAIR_EN_ENG,    /**<  EN and ENG are treated the same */
    ET9CPMOHU_PAIR_IN_ING,    /**<  IN and ING are treated the same */
    ET9CPMOHU_PAIR_LAST       /**<  This has to be the last item */
} ET9CPMOHU_PAIR;

/* Bit flags for Mohu pairs for ET9CPSetMohuPairs() */
#define ET9CPMOHU_PAIR_Z_ZH_MASK ((ET9U16)(1 << ET9CPMOHU_PAIR_Z_ZH))     /**< bit flag used in ET9CPSetMohuPairs():  Z and ZH are treated the same */
#define ET9CPMOHU_PAIR_C_CH_MASK ((ET9U16)(1 << ET9CPMOHU_PAIR_C_CH))     /**< bit flag used in ET9CPSetMohuPairs():  C and CH are treated the same */
#define ET9CPMOHU_PAIR_S_SH_MASK ((ET9U16)(1 << ET9CPMOHU_PAIR_S_SH))     /**< bit flag used in ET9CPSetMohuPairs():  S and SH are treated the same */
#define ET9CPMOHU_PAIR_N_L_MASK ((ET9U16)(1 << ET9CPMOHU_PAIR_N_L))       /**< bit flag used in ET9CPSetMohuPairs():  N and L  are treated the same */
#define ET9CPMOHU_PAIR_R_L_MASK ((ET9U16)(1 << ET9CPMOHU_PAIR_R_L))       /**< bit flag used in ET9CPSetMohuPairs():  R and L  are treated the same */
#define ET9CPMOHU_PAIR_F_H_MASK ((ET9U16)(1 << ET9CPMOHU_PAIR_F_H))       /**< bit flag used in ET9CPSetMohuPairs():  F and H  are treated the same */
#define ET9CPMOHU_PAIR_AN_ANG_MASK ((ET9U16)(1 << ET9CPMOHU_PAIR_AN_ANG)) /**< bit flag used in ET9CPSetMohuPairs():  AN and ANG are treated the same */
#define ET9CPMOHU_PAIR_EN_ENG_MASK ((ET9U16)(1 << ET9CPMOHU_PAIR_EN_ENG)) /**< bit flag used in ET9CPSetMohuPairs():  EN and ENG are treated the same */
#define ET9CPMOHU_PAIR_IN_ING_MASK ((ET9U16)(1 << ET9CPMOHU_PAIR_IN_ING)) /**< bit flag used in ET9CPSetMohuPairs():  IN and ING are treated the same */

#ifdef ET9_ALPHABETIC_MODULE
#define ET9CPIsAWActive(pET9CPLingInfo)  ((pET9CPLingInfo) && (pET9CPLingInfo)->pAWLing) /**< Check if alphabetic text entry is enabled. */
#define ET9CPIsTraceActive(pET9CPLingInfo)  ((pET9CPLingInfo) && (pET9CPLingInfo)->Trace.pALing && (pET9CPLingInfo)->Trace.fDBLoaded) /**< Check if trace is enabled. */
#else
#define ET9CPIsAWActive(pET9CPLingInfo)  0 /**< Check if alphabetic text entry is enabled. */
#define ET9CPIsTraceActive(pET9CPLingInfo) 0 /**< Check if trace is enabled. */
#endif

/* macros to check if an input mode is currently active */
#define ET9CPIsMode(pET9CPLingInfo, eModeIn) ((ET9BOOL)((pET9CPLingInfo) && ((pET9CPLingInfo)->eMode == (eModeIn))))

#define ET9CPIsModePinyin(pET9CPLingInfo)    ET9CPIsMode(pET9CPLingInfo, ET9CPMODE_PINYIN)  /**< Check if current mode is PinYin.   ET9CPMode::ET9CPMODE_PINYIN */
#define ET9CPIsModeBpmf(pET9CPLingInfo)      ET9CPIsMode(pET9CPLingInfo, ET9CPMODE_BPMF)    /**< Check if current mode is BoPoMoFo. ET9CPMode::ET9CPMODE_BPMF */
#define ET9CPIsModeStroke(pET9CPLingInfo)    ET9CPIsMode(pET9CPLingInfo, ET9CPMODE_STROKE)  /**< Check if current mode is Stroke.   ET9CPMode::ET9CPMODE_STROKE */
#define ET9CPIsModeCangJie(pET9CPLingInfo)   ET9CPIsMode(pET9CPLingInfo, ET9CPMODE_CANGJIE) /**< Check if current mode is CangJie.  ET9CPMode::ET9CPMODE_CANGJIE */
#define ET9CPIsModeQuickCangJie(pET9CPLingInfo)    ET9CPIsMode(pET9CPLingInfo, ET9CPMODE_QUICK_CANGJIE)  /**< Check if current mode is QuickCangJie.  ET9CPMode::ET9CPMODE_QUICK_CANGJIE */
#define ET9CPIsModePhonetic(pET9CPLingInfo)  (ET9CPIsModePinyin(pET9CPLingInfo) || ET9CPIsModeBpmf(pET9CPLingInfo))  /**< Check if current mode is PinYin or BoPoMoFo */

/* macros to check if a feature is enabled */
#define ET9CPIsStateActive(pET9CPLingInfo, eState) ((pET9CPLingInfo) && ((pET9CPLingInfo)->bState & (ET9U8)(eState)))

#define ET9CPIsComponentActive(pET9CPLingInfo)      ET9CPIsStateActive(pET9CPLingInfo, ET9_CP_STATE_COMPONENT)      /**< Check if component entry is enabled */
#define ET9CPIsSmartPunctActive(pET9CPLingInfo)     ET9CPIsStateActive(pET9CPLingInfo, ET9_CP_STATE_SMARTPUNCT)     /**< Check if smart punctuation is enabled */
#define ET9CPIsNameInputActive(pET9CPLingInfo)      ET9CPIsStateActive(pET9CPLingInfo, ET9_CP_STATE_NAME_INPUT)     /**< Check if name input is enabled */
#define ET9CPIsPartialSpellActive(pET9CPLingInfo)   ET9CPIsStateActive(pET9CPLingInfo, ET9_CP_STATE_PARTIAL_SPELL)  /**< Check if partial spell is enabled */
#define ET9CPIsCommonCharActive(pET9CPLingInfo)     ET9CPIsStateActive(pET9CPLingInfo, ET9_CP_STATE_COMMON_CHAR)    /**< Check if common character is enabled */
#define ET9CPIsFullSentenceActive(pET9CPLingInfo)     ET9CPIsStateActive(pET9CPLingInfo, ET9_CP_STATE_FULL_SENTENCE)    /**< Check if full sentence is enabled */

#define ET9CPGetChineseLdbNum(pET9CPLingInfo) ((ET9U16)((pET9CPLingInfo)? (pET9CPLingInfo)->wLdbNum : 0)) /**< returns the identifier of the current Chinese LDB */

/*----------------------------------------------------------------------------
 *  Chinese XT9 data structures.
 *----------------------------------------------------------------------------*/

/** Chinese XT9 Spell */
typedef struct {
    ET9SYMB  pSymbs[ET9CPMAXSPELLSIZE];   /**< Spelling string buffer */
    ET9U8    bLen;                        /**< length of spelling */
} ET9CPSpell;

/** Chinese XT9 Phrase */
typedef struct {
    ET9SYMB  pSymbs[ET9CPMAXPHRASESIZE];  /**< character's Unicodes */
    ET9U8    bLen;                        /**< length of phrase */
} ET9CPPhrase;


/*
* Chinese XT9 API Functions.
*
*/

/** \name Initialization and Settings
 *  API functions for initialization and turning on/off various settings.
 */
/*@{*/
ET9STATUS ET9FARCALL ET9CPSysInit(ET9CPLingInfo             *pET9CPLingInfo,
                                  ET9WordSymbInfo * const   pWordSymbInfo,
                                  void ET9FARDATA * const   pPublicExtension);

ET9STATUS ET9FARCALL ET9CPLdbInit(ET9CPLingInfo * const     pET9CPLingInfo,
                                  const ET9U16              wLdbNum,
                                  const ET9CPDBREADCALLBACK ET9CPLdbReadData);

ET9STATUS ET9FARCALL ET9CPLdbGetVersion(ET9CPLingInfo * const pET9CPLingInfo,
                                        ET9SYMB       * const psLdbVerBuf,
                                        const ET9U16          wBufMaxSize,
                                        ET9U16        * const pwBufSize);

ET9STATUS ET9FARCALL ET9CPLdbValidate(ET9CPLingInfo             *pET9CPLingInfo,
                                      const ET9U16              wLdbNum,
                                      const ET9CPDBREADCALLBACK ET9CPLdbReadData);

ET9STATUS ET9FARCALL ET9CPSetInputMode(ET9CPLingInfo    *pET9CPLingInfo,
                                       ET9CPMode        eMode);

ET9STATUS ET9FARCALL ET9CPSetNameInput(ET9CPLingInfo *pET9CPLingInfo);

ET9STATUS ET9FARCALL ET9CPClearNameInput(ET9CPLingInfo *pET9CPLingInfo);

ET9STATUS ET9FARCALL ET9CPSetSmartPunct(ET9CPLingInfo *pET9CPLingInfo);

ET9STATUS ET9FARCALL ET9CPClearSmartPunct(ET9CPLingInfo *pET9CPLingInfo);

ET9STATUS ET9FARCALL ET9CPSetMohuPairs(ET9CPLingInfo    *pET9CPLingInfo,
                                       ET9U16           wMohuPairBitMask);

ET9BOOL ET9FARCALL ET9CPIsMohuPairEnabled(ET9CPLingInfo     *pET9CPLingInfo,
                                          ET9CPMOHU_PAIR    eMohuPair);

ET9STATUS ET9FARCALL ET9CPSetPartialSpell(ET9CPLingInfo *pET9CPLingInfo);

ET9STATUS ET9FARCALL ET9CPClearPartialSpell(ET9CPLingInfo *pET9CPLingInfo);

ET9STATUS ET9FARCALL ET9CPSetCommonChar(ET9CPLingInfo *pET9CPLingInfo);

ET9STATUS ET9FARCALL ET9CPClearCommonChar(ET9CPLingInfo *pET9CPLingInfo);

ET9STATUS ET9FARCALL ET9CPSetFullSentence(ET9CPLingInfo *pET9CPLingInfo);

ET9STATUS ET9FARCALL ET9CPClearFullSentence(ET9CPLingInfo *pET9CPLingInfo);

#ifndef ET9CP_DISABLE_STROKE
/* for Stroke input only */
ET9STATUS ET9FARCALL ET9CPSetComponent(ET9CPLingInfo *pET9CPLingInfo);

ET9STATUS ET9FARCALL ET9CPClearComponent(ET9CPLingInfo *pET9CPLingInfo);
#endif

#ifdef ET9_ALPHABETIC_MODULE
/* for alphabetic support only */
ET9STATUS ET9FARCALL ET9CPSetAW(ET9CPLingInfo *pET9CPLingInfo,
                                ET9AWLingInfo *pET9AWLingInfo);

ET9STATUS ET9FARCALL ET9CPTraceInit(ET9CPLingInfo *pET9CPLingInfo,
                                    ET9AWLingInfo *pET9AWLingInfo);
#endif

/*@}*/

/** \name Get and Select
 *  API functions for getting and selecting Prefix, Spelling, and Phrase
 */
/*@{*/

ET9STATUS ET9FARCALL ET9CPBuildSelectionList(ET9CPLingInfo * const pET9CPLingInfo);

ET9STATUS ET9FARCALL ET9CPGetSpell(ET9CPLingInfo    *pET9CPLingInfo,
                                   ET9CPSpell       *psSpell);

/* Prefix-related */
ET9U8    ET9FARCALL ET9CPGetPrefixCount(const ET9CPLingInfo *pET9CPLingInfo);

ET9STATUS ET9FARCALL ET9CPGetPrefix(const ET9CPLingInfo *pET9CPLingInfo,
                                    ET9U16              wIndex,
                                    ET9CPSpell          *psSpell);

ET9STATUS ET9FARCALL ET9CPSetActivePrefix(ET9CPLingInfo *pET9CPLingInfo,
                                          ET9U8         bPrefixIndex);

ET9STATUS ET9FARCALL ET9CPClearActivePrefix(ET9CPLingInfo *pET9CPLingInfo);

ET9STATUS ET9FARCALL ET9CPGetActivePrefixIndex(ET9CPLingInfo    *pET9CPLingInfo,
                                               ET9U8            *pbPrefixIndex);

/* Phrase-related */
ET9STATUS ET9FARCALL ET9CPGetPhrase(ET9CPLingInfo   *pET9CPLingInfo,
                                    ET9U16          wPhraseIndex,
                                    ET9CPPhrase     *psPhrase,
                                    ET9CPSpell      *psSpell);

ET9STATUS ET9FARCALL ET9CPGetPhraseFreq(ET9CPLingInfo *pET9CPLingInfo,
                                        const ET9CPPhrase *pPhrase,
                                        ET9INT *pnFreq);

ET9STATUS ET9FARCALL ET9CPSelectPhrase(ET9CPLingInfo    *pET9CPLingInfo,
                                       ET9U16           wPhraseIndex,
                                       ET9CPSpell       *psSpell);

ET9STATUS ET9FARCALL ET9CPUnselectAll(ET9CPLingInfo     *pET9CPLingInfo);

ET9STATUS ET9FARCALL ET9CPUnselectPhrase(ET9CPLingInfo  *pET9CPLingInfo);

ET9STATUS ET9FARCALL ET9CPGetSelection(ET9CPLingInfo    *pET9CPLingInfo,
                                       ET9CPPhrase      *pUnicodePhrase,   /* Unicode string */
                                       ET9CPSpell       *pSpell,
                                       ET9U8            *pbSelSymbCount);

ET9STATUS ET9FARCALL ET9CPCommitSelection(ET9CPLingInfo *pET9CPLingInfo);
/*@}*/

/** \name Context prediction
 *  API functions for context prediction
 */
/*@{*/
ET9STATUS ET9FARCALL ET9CPSetContext(ET9CPLingInfo      *pET9CPLingInfo,
                                     ET9SYMB            *psContext,
                                     ET9UINT            nContextLength);

ET9STATUS ET9FARCALL ET9CPClearContext(ET9CPLingInfo    *pET9CPLingInfo);
/*@}*/

/** \name UDB
 *  API functions for UDB operations
 */
/*@{*/
ET9STATUS ET9FARCALL ET9CPUdbActivate(ET9CPLingInfo                    *pET9CPLingInfo,
                                      ET9CPDBWRITECALLBACK              pfUdbWrite,
                                      ET9CPUdbInfo ET9FARDATA           *pUdb,
                                      ET9U16                            wDataSize);

ET9STATUS ET9FARCALL ET9CPUdbReset(ET9CPLingInfo *pET9CPLingInfo);

ET9STATUS ET9FARCALL ET9CPUdbAddPhrase(ET9CPLingInfo    *pET9CPLingInfo,
                                       ET9CPPhrase      *psPhrase,
                                       ET9SYMB          *psSpell,
                                       ET9U8            bSpellLen);

ET9STATUS ET9FARCALL ET9CPUdbDeletePhrase(ET9CPLingInfo *pET9CPLingInfo,
                                          ET9CPPhrase   *psPhrase);

ET9STATUS ET9FARCALL ET9CPUdbGetPhrase(ET9CPLingInfo    *pET9CPLingInfo,
                                       ET9U16           wTypeMask,
                                       ET9U16           wIndex,
                                       ET9CPPhrase      *pPhrase,
                                       ET9CPSpell       *pSpell);

ET9STATUS ET9FARCALL ET9CPUdbGetPhraseCount(ET9CPLingInfo   *pET9CPLingInfo,
                                            ET9U16           wTypeMask,
                                            ET9U16          *pwCount);

ET9STATUS ET9FARCALL ET9CPUdbGetExportSize(ET9CPLingInfo    *pET9CPLingInfo,
                                           ET9U32           *pdwSize);

ET9STATUS ET9FARCALL ET9CPUdbExport(ET9CPLingInfo       *pET9CPLingInfo,
                                    ET9U8 ET9FARDATA    *pbDst,
                                    ET9U32              dwSize,
                                    ET9UINT             nSkipCount,
                                    ET9UINT             *pnSuccessCount);

ET9STATUS ET9FARCALL ET9CPUdbImport(ET9CPLingInfo           *pET9CPLingInfo,
                                    const ET9U8 ET9FARDATA  *pbSrc,
                                    ET9U32                  dwSize,
                                    ET9UINT                 *pnAcceptCount,
                                    ET9UINT                 *pnRejectCount);
/*@}*/

/** \name Conversion
 *  API functions for Simplified-Traditional Chinese conversion
 */
/*@{*/
ET9STATUS ET9FARCALL ET9CPSimplifiedToTraditional(ET9CPLingInfo     *pET9CPLingInfo,
                                                  ET9SYMB           *pUnicodeStr,
                                                  ET9U16            wStrLen);

ET9STATUS ET9FARCALL ET9CPTraditionalToSimplified(ET9CPLingInfo     *pET9CPLingInfo,
                                                  ET9SYMB           *pUnicodeStr,
                                                  ET9U16            wStrLen);
/*@}*/



/** \name Misc Functions
 *  API functions for miscellaneous operations
 */
/*@{*/
ET9STATUS ET9FARCALL ET9CPGetCharSpell(ET9CPLingInfo    *pET9CPLingInfo,
                                       ET9SYMB          sCharacter,
                                       ET9U8            bAlternateNum,
                                       ET9U8            bGetTone,
                                       ET9CPSpell       *pSpellInfo);

#ifndef ET9CP_DISABLE_STROKE
/* for Stroke input only */
ET9STATUS ET9FARCALL ET9CPGetCharStrokes(ET9CPLingInfo  *pET9CPLingInfo,
                                         ET9SYMB        sChar,
                                         ET9U8          *pbBuf,
                                         ET9U8          *pbLen,
                                         ET9U8          bAltIndex);

ET9STATUS ET9FARCALL ET9CPGetCharCangJieCode(ET9CPLingInfo  *pET9CPLingInfo,
                                             ET9SYMB        sChar,
                                             ET9U8          *pbBuf,
                                             ET9U8          *pbLen,
                                             ET9U8          bAltIndex);
#endif

ET9BOOL   ET9FARCALL ET9CPIsComponent(ET9CPLingInfo     *pET9CPLingInfo,
                                      ET9U16            wUnicode);

ET9STATUS ET9FARCALL ET9CPGetToneOptions(ET9CPLingInfo  *pET9CPLingInfo,
                                         ET9U8          *pbToneBitMask);

ET9STATUS ET9FARCALL ET9CPAddToneSymb(ET9WordSymbInfo   *pWordSymbInfo,
                                      const ET9CPSpell  *psSpell,
                                      ET9CPSYMB         bTone);

ET9STATUS ET9FARCALL ET9CPGetSpecialPhrase(ET9CPLingInfo    *pET9CPLingInfo,
                                           ET9CPPhraseType  ePhraseType,
                                           ET9U16           wPhraseIndex,
                                           ET9CPPhrase      *psPhrase);

ET9STATUS ET9FARCALL ET9CPSelectSpecialPhrase(ET9CPLingInfo     *pET9CPLingInfo,
                                              ET9CPPhraseType   ePhraseType,
                                              ET9U16            wPhraseIndex);

ET9STATUS ET9FARCALL ET9CPGetHomophonePhraseCount(ET9CPLingInfo     *pET9CPLingInfo,
                                                  const ET9CPPhrase *pBasePhrase,
                                                  ET9U16            *pwCount);

ET9STATUS ET9FARCALL ET9CPGetHomophonePhrase(ET9CPLingInfo      *pET9CPLingInfo,
                                             const ET9CPPhrase  *pBasePhrase,
                                             ET9U16             wIndex,
                                             ET9CPPhrase        *pPhrase,
                                             ET9CPSpell         *pSpell);

/* Chinese XT9 check compile parameters */
#ifdef ET9CHECKCOMPILE
ET9U32 ET9FARCALL ET9CPCheckCompileParameters(ET9U8     *pbET9U8,
                                              ET9U8     *pbET9U16,
                                              ET9U8     *pbET9U32,
                                              ET9U8     *pbET9UINT,
                                              ET9U8     *pbET9S8,
                                              ET9U8     *pbET9S16,
                                              ET9U8     *pbET9S32,
                                              ET9U8     *pbET9INT,
                                              ET9U8     *pbET9SYMB,
                                              ET9U8     *pbET9BOOL,
                                              ET9U8     *pbET9FARDATA,
                                              ET9U8     *pbET9FARCALL,
                                              ET9U8     *pbET9LOCALCALL,
                                              ET9U8     *pbVoidPtr,
                                              ET9U16    *pwET9SymbInfo,
                                              ET9UINT   *pnET9WordSymbInfo,
                                              ET9UINT   *pnET9CPLingInfo);

#endif
/*@}*/

#ifdef ET9CHECKCOMPILE
enum {
    ET9WRONGSIZE_ET9CPLINGINFO = ET9CHECKCOMPILE_NEXT,
};
#endif

#ifdef ET9_PERF_DEBUG
#include "perfhelper.h"
#else
#define ET9_FUNC_HIT(funcName)   ((void)0)
#endif /* ET9_PERF_DEBUG */

#endif /* ET9_CHINESE_MODULE */

/* End don't mangle the function name if compile under C++ */
#if defined(__cplusplus)
    }
#endif

/*! @} */

#endif /* #ifndef ET9CPAPI_H */

/* ----------------------------------< eof >--------------------------------- */
