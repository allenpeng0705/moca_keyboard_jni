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
;**     FileName: et9cprdb.h                                                  **
;**                                                                           **
;**  Description: Chinese XT9 RDB header file.                                **
;**               Conforming to the development version of Chinese XT9.       **
;**                                                                           **
;*******************************************************************************
;******************************************************************************/

#ifndef ET9CPRDB_H
#define ET9CPRDB_H 1

/* don't mangle the function name if compile under C++ */
#if defined(__cplusplus)
extern "C" {
#endif

#include "et9cpspel.h"

/*----------------------------------------------------------------------------
 *  Define the RDB parameters and constants.
 *----------------------------------------------------------------------------*/

/* constants related to an entry */
#define ET9_CP_NON_FREE_TYPE_MASK         (0x80)  /* whether the obj is non-free type */
#define ET9_CP_RDB_TYPE_MASK             (0x40)  /* whether the obj is RDB type */
#define ET9_CP_AUDB_TYPE_MASK            (0x20)  /* whether the obj is AUDB type */
#define ET9_CP_MIN_WORD_SIZE             1       /* min word size of an entry */
#define ET9_CP_WORD_SIZE_MASK            (0x1F)  /* mask for word size */
#define ET9_CP_FREE_SIZE_FLAG            (0x40)  /* whether the free obj uses 2 bytes for entry size */
#define ET9_CP_MAX_FREE_SIZE             (0x3FFF)/* max size of a free entry */
#define ET9_CP_FREE_SIZE_UPPER_MASK       (ET9_CP_FREE_SIZE_FLAG - 1) /* mask for upper byte of a free entry's size */
/* bit masks for get entry operation */
#define ET9_CP_GET_TYPE         0x01
#define ET9_CP_GET_ENTRY_SIZE    0x02
#define ET9_CP_GET_FREQ         0x04
#define ET9_CP_GET_ID           0x08
#define ET9_CP_GET_CHECKSUM     0x10

#define ET9_CP_MAX_UPDATE_COUNT          ((ET9U16)500)    /* max update count before rescale */
#define ET9_CP_MAXFREQ                 ((ET9U16)64000)  /* max frequency */
#define ET9_CP_MINFREQ                 ((ET9U16)1)      /* min frequency */
#define ET9_CP_AUDB_INITFREQ           ((ET9U16)60)     /* initial freq for AUDB type */
#define ET9_CP_AUDB_LEN_THRESHOLD      ((ET9U16)8)      /* max length to auto add full sentence as AUDB type */
#define ET9_CP_RDB_INITFREQ            ((ET9U16)64)     /* initial freq for RDB words */
#define ET9_CP_UDB_INITFREQ            ((ET9U16)400)    /* initial freq for UDB words */
#define ET9_CP_FREQ_RISE               ((ET9U16)88)     /* freq rise for one usage */
#define ET9_CP_UDB_RESCALE_FACTOR   32              /* freq rescale factor */
#define ET9_CP_UDB_CUTOFF_LEVELS    5               /* number of estimated cutoff freq levels */
#define ET9_CP_UDB_MIN_CUTOFF       ((ET9U16)10)     /* min cutoff freq */
#define ET9_CP_UDB_MIN_FREE_FACTOR   20      /* min factor (1/20) of free space before overcrowded */
#define ET9_CP_UDB_MIN_CLEANUP_FACTOR    20  /* min factor (1/20) to clean during clean up */

/* constants for UDB export and import */
typedef enum {
    ET9_CP_TUDB_VERSION_NONE    = 0,
    ET9_CP_TUDB_VERSION_1       = 1,
    ET9_CP_TUDB_VERSION_MAX
} ET9_CP_TUDB_VERSION;
#define ET9_CP_TUDB_CURRENT_VERSION         ET9_CP_TUDB_VERSION_1

#define ET9_CP_U8_ASCII_STR_SIZE            2 /* size of Hex ASCII string converted from a U8 value */
#define ET9_CP_U16_ASCII_STR_SIZE           4 /* size of Hex ASCII string converted from a U16 value */
#define ET9_CP_U32_ASCII_STR_SIZE           8 /* size of Hex ASCII string converted from a U32 value */
#define ET9_CP_MAX_UTF8_CHAR_SIZE           3
#define ET9_CP_TUDB_TYPE_SIZE               1
#define ET9_CP_TUDB_MODE_SIZE               1
#define ET9_CP_TUDB_FREQ_SIZE               (ET9_CP_U16_ASCII_STR_SIZE + 1)
#define ET9_CP_TUDB_MAX_PINYIN_SIZE         ( (ET9_CP_MAX_PINYIN_SYL_SIZE + 1) * ET9CPMAXUDBPHRASESIZE + 1)
#define ET9_CP_TUDB_MAX_BPMF_SIZE           ( (ET9_CP_MAX_BPMF_SYL_SIZE * ET9_CP_MAX_UTF8_CHAR_SIZE + 1) * ET9CPMAXUDBPHRASESIZE + 1)
#if (ET9_CP_TUDB_MAX_BPMF_SIZE < ET9_CP_TUDB_MAX_PINYIN_SIZE)
#define ET9_CP_TUDB_MAX_SPELL_SIZE          ET9_CP_TUDB_MAX_PINYIN_SIZE
#else
#define ET9_CP_TUDB_MAX_SPELL_SIZE          ET9_CP_TUDB_MAX_BPMF_SIZE
#endif
#define ET9_CP_TUDB_MAX_PHRASE_SIZE         (ET9_CP_MAX_UTF8_CHAR_SIZE * ET9CPMAXUDBPHRASESIZE + 1)
#define ET9_CP_TUDB_MAX_HEADER_SIZE         (ET9_CP_U8_ASCII_STR_SIZE + 1               \
                                            + ET9MAXVERSIONSTR + 1                      \
                                            + ET9MAXVERSIONSTR + 1)
/* TUDB Header: TUDB version ASCII string + Ldb version string + Core version string */
#define ET9_CP_TUDB_MAX_ENTRY_SIZE          (ET9_CP_TUDB_TYPE_SIZE                      \
                                            + ET9_CP_TUDB_MODE_SIZE                     \
                                            + ET9_CP_TUDB_FREQ_SIZE                     \
                                            + ET9_CP_TUDB_MAX_PHRASE_SIZE               \
                                            + ET9_CP_TUDB_MAX_SPELL_SIZE)

#define ET9_CP_TUDB_USER_TYPE               'U' /* User-defined phrases */
#define ET9_CP_TUDB_AUTO_TYPE               'A' /* Auto-created phrases */
#define ET9_CP_TUDB_EOL_MARK                'E' /* End-of-list mark */
#define ET9_CP_TUDB_PINYIN_MODE             'P' /* spelling is Pinyin */
#define ET9_CP_TUDB_BPMF_MODE               'B' /* spelling is BPMF */
#define ET9_CP_TUDB_STROKE_MODE             'S' /* spelling is Stroke */
#define ET9_CP_TUDB_SYL_DELIMITER           '.'
#define ET9_CP_TUDB_FIELD_DELIMITER         '\t'
#define ET9_CP_TUDB_ENTRY_DELIMITER         '\n'

/* Given a == hi byte, b == lo byte; compose a word integral (ET9U16). */
#define ET9_CP_MAKEWORD(a, b) ((ET9U16)((ET9U8)(b) | ((ET9U16)((ET9U8)(a)) << 8)))

#define ET9_CP_LOBYTE(w)      ((ET9U8)(w))

#define ET9_CP_HIBYTE(w)      ((ET9U8)((ET9U16)(w) >> 8))

/* entry size = 3 + 2 * (word size) */
#define ET9_CP_WordSizeToEntrySize(bWordSize) ((ET9U8)(3 + ((bWordSize) << 1)))
/* word size  = ((entry size) - 3) / 2 */
#define ET9_CP_EntrySizeToWordSize(wEntrySize) ((ET9U8)(((wEntrySize) - 3) >> 1))

/* next zone */
#define ET9_CP_NextZone(b) ((b + 1) % ET9_CP_UDBZONEMAX)

/* Macros related to the ET9CPUdbInfo structure in the et9cpapi.h */

/* The pointer to UDB data area */
#define ET9_CP_UdbData(udb) ((ET9U8 ET9FARDATA*)((udb)->bDataArea))

/* The pointer to the byte following the end of UDB */
#define ET9_CP_UdbEnd(udb) ((ET9U8 ET9FARDATA*)(udb) + (udb)->wDataSize)

/* The actual number of bytes in the UDB header area */
#define ET9_CP_UdbHeaderBytes(udb)    \
            ((ET9UINT)(ET9_CP_UdbData(udb) - (ET9U8 ET9FARDATA*)(udb)))

/* The actual number of bytes in the UDB data area */
#define ET9_CP_UdbDataAreaBytes(udb)    \
            ((ET9UINT)((udb)->wDataSize - ET9_CP_UdbHeaderBytes(udb)))

#define ET9_CP_UdbPtrToOffset(udb, ptr) ((ET9U16)((ptr) - ET9_CP_UdbData(udb)))

#define ET9_CP_UdbOffsetToPtr(udb, offset) (ET9_CP_UdbData(udb) + (offset))

/* Test this thread's cached write count (dwDirtyCount) against the count
 *     from UDB header.
 *
 * return : 1 = Another thread has changed udb since we last read, current key
 *              sequence my be invalid (Note: caller should return
 *              ET9STATUS_NEED_SELLIST_BUILD to integration layer)
 *
 *          0 = Udb hasn't changed since we last read
 */
#define ET9_CP_IsUdbChangedByOtherThread(pET9CPLingInfo)                        \
    ((pET9CPLingInfo)->pUdb &&                                                 \
     (pET9CPLingInfo)->UdbReadCache.dwDirtyCount != (pET9CPLingInfo)->pUdb->dwDirtyCount && \
     (pET9CPLingInfo)->Base.pWordSymbInfo->bNumSymbs != 0)

/*----------------------------------------------------------------------------
 *  Define the RDB structure and variable types.
 *----------------------------------------------------------------------------*/

typedef enum ET9_CP_EntryType_e {
    ET9_CP_FREE = 0,
    ET9_CP_RDBPHONETIC = 1,
    ET9_CP_RDBSTROKE = 2,
    ET9_CP_AUDBPHONETIC = 3,
    ET9_CP_AUDBSTROKE = 4,
    ET9_CP_UDBPHONETIC = 5,
    ET9_CP_UDBSTROKE = 6
} ET9_CP_EntryType;

typedef struct ET9_CP_RUdbObj_s {
    ET9_CP_EntryType   eType;  /* Entry type */
    /* ET9U8        bLang;   reserved for future use */
    ET9U16       wEntrySize; /* bytes occupied by the entry == 3 + 2w (where w == word length) */
    ET9U16       wFreq;  /* word frequency (ET9_CP_FREE entry: undefined) */
    ET9U16       pwID[ET9CPMAXUDBPHRASESIZE]; /* Phonetic entry: Phonetic ID,
                                            Stroke entry: Stroke ID,
                                            ET9_CP_FREE entry: undefined */
} ET9_CP_RUdbObj;

/*----------------------------------------------------------------------------
 *  Declare prototypes for RDB functions internal to the core
 *----------------------------------------------------------------------------*/

/*------------------------------------------------------------------------
 *  Function    : ET9_CP_MoveUdbPR
 *
 *  Synopsis    : Move a UDB pointer to the right by the given amount.
 *                Wrap around at UDB end.
 *
 *     Input:   : pUdb = pointer to UDB
 *                pbThis = pointer to start of the move
 *                wNumMoves = number of bytes of the move amount
 *
 *    Output    : none
 *
 *    Return    : result pointer after the move
 *-----------------------------------------------------------------------*/
#if defined(ET9_DEBUG) || defined(WIN32_PLATFORM_WFSP) || defined(WIN32_PLATFORM_PSPC)
ET9U8 ET9FARDATA * ET9FARCALL ET9_CP_MoveUdbPR(ET9CPUdbInfo ET9FARDATA *pUdb,
                                      ET9U8 ET9FARDATA * pbThis,
                                      ET9U16 wNumMoves);
#else
/* release version for optimization */
#define ET9_CP_MoveUdbPR(pUdb, pbThis, wNumMoves)     \
        ((pbThis) + (wNumMoves) - (((pbThis) + (wNumMoves) >= ET9_CP_UdbEnd(pUdb)) ? ET9_CP_UdbDataAreaBytes(pUdb) : 0))

#endif /* END ET9_DEBUG */

/*------------------------------------------------------------------------
 *  Function    : ET9_CP_GetEntryInfo
 *
 *  Synopsis    : Get requested info of an entry into the result obj.
 *
 *     Input:   : pUdb = pointer to the UDB
 *                pbData = pointer to the start of the entry
 *                bOption = specify which field(s) of the entry is needed
 *
 * Input/Output : pObj = obj containing info of the entry.
 *
 *    Return    : checksum of the entry if requested, 0 otherwise.
 *-----------------------------------------------------------------------*/
ET9U16 ET9FARCALL ET9_CP_GetEntryInfo(ET9CPUdbInfo ET9FARDATA *pUdb,
                                  ET9U8 ET9FARDATA * pbData,
                                  ET9_CP_RUdbObj *pObj,
                                  ET9U8 bOption);

/* Description : Check if the UDB is compatible with the LDB.
 * Return      : ET9STATUS_NO_RUDB if has a UDB and the UDB is incompatible with LDB.
 */
ET9STATUS ET9FARCALL ET9_CP_CheckUdbCompat(ET9CPLingInfo *pET9CPLingInfo);

void ET9FARCALL ET9_CP_UdbUsePhrase(
    ET9CPLingInfo *pET9CPLingInfo,
    ET9CPPhrase *pPhrase,
    ET9_CP_IDEncode eEncode,
    ET9U16 wFreq,
    ET9BOOL bIsAuto);

void ET9FARCALL ET9_CP_GetUdbPhrases(
    ET9CPLingInfo *pET9CPLingInfo,
    ET9BOOL bNeedPartialSyl,
    ET9BOOL bNeedPartialPhrase,
    const ET9U16 *pwPrefix,
    ET9U8 bPrefixLen,
    ET9_CP_SpellMatch *pMatchType,
    ET9BOOL bIsSID,
    ET9BOOL bGetToneOption,
    ET9U8 *pbToneOptions,
    ET9U8 *pbTones,
    ET9BOOL bValidateCntxPrefix,
    ET9_CP_PhraseBuf *pPhraseBuf);

/*
 * Function: ET9_CP_UdbFindDelUIDMatch
 * Synopsis: Find 1st or Delete all phrases that exactly match the given Unicode
 * Input:    psPhrase = given Unicode
 *           eEncode = encoding of pwAltID (PID/BID/SID)
 *           pwAltID = array of all alt ID of 1st Unicode
 *           bAltCount = number of alt ID in pwAltID
 * In/Out:   psIDPhrase = 0: delete all phrases that match the given Unicode
 *                        non-zero: store PID/SID of the 1st match found
 *
 * Return : non-zero if at least one phrase is found/deleted;
 *          0 otherwise.
 */
ET9UINT ET9FARCALL ET9_CP_UdbFindDelUIDMatch(ET9CPLingInfo *pET9CPLingInfo,
                                         ET9CPPhrase *psPhrase,
                                         ET9CPPhrase *psIDPhrase,
                                         ET9_CP_IDEncode eEncode,
                                         ET9U16 *pwAltID,
                                         ET9U8 bAltCount);

/* End don't mangle the function name if compile under C++ */
#if defined(__cplusplus)
    }
#endif

#endif  /* #ifndef ET9CPRDB_H */

/* ----------------------------------< eof >--------------------------------- */
