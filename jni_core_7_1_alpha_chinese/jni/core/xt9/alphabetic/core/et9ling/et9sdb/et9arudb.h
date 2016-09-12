/*******************************************************************************
;*******************************************************************************
;**                                                                           **
;**                    COPYRIGHT 1998-2011 NUANCE COMMUNICATIONS              **
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
;**     FileName: ET9AWRUDB.h                                                **
;**                                                                           **
;**  Description: User data base access routines header file.                 **
;**               Conforms to Version 3.0                                     **
;**                                                                           **
;*******************************************************************************
;******************************************************************************/

#ifndef ET9AWRUDB_H
#define ET9AWRUDB_H    1

#include "et9api.h"
#ifdef ET9_ALPHABETIC_MODULE
#include "et9aslst.h"

/* don't mangle the function name if compile under C++ */
#if defined(__cplusplus)
extern "C" {
#endif

/*
 * Aging interval, roughtly 50 minutes use of T9 for
 * an average user which can type 20wpm using T9.
 */

#define ET9UDBMAXUPDATE                 ((ET9U16)1000)
#define ET9UDBAGINGFACTOR               32
#define ET9MAX_FREQ_COUNT               ((ET9U16)(64000)) /* Max Freq. resolution */
#define ET9FREQ_BUMP_COUNT              ((ET9U16)(64))    /* Freq. bump amount */

/* initial freq. for non-first word (or first word of ldb word completion on) */

#define ET9RD_WORD_INIT_FREQ            (54)

/* initial freq. for first-word */

#define ET9RD_FIRST_WORD_INIT_FREQ      (150)

/* initial freq. for add word */

#define ET9ALP_WORD_INIT_FREQ           (160)

#define ET9_QUDB_EXIST_COUNT            (10)

#ifdef ET9_USE_FLOAT_FREQS
#define ET9_SUPP_DB_BASE_FREQ           ((ET9FREQPART)(_ET9pow_f(2, 30)))
#define ET9SUPP_DB_FREQ_BUMP_COUNT      ((ET9FREQPART)(_ET9pow_f(2, 20))) /* Freq. bump amount */
#define ET9_SUPP_DB_BASE_FREQ_UNI       ((ET9FREQPART)32000)
#define ET9SUPP_DB_FREQ_BUMP_COUNT_UNI  ((ET9FREQPART)64) /* Freq. bump amount */
#else
#define ET9_SUPP_DB_BASE_FREQ           (32000)
#define ET9SUPP_DB_FREQ_BUMP_COUNT      ((ET9U16)(64))    /* Freq. bump amount */
#define ET9_SUPP_DB_BASE_FREQ_UNI       ET9_SUPP_DB_BASE_FREQ
#define ET9SUPP_DB_FREQ_BUMP_COUNT_UNI  ET9SUPP_DB_FREQ_BUMP_COUNT
#endif

ET9STATUS ET9FARCALL _ET9AWRDBAddWord(ET9AWLingInfo         *pLingInfo,
                                      ET9AWPrivWordInfo     *pSTWord,
                                      ET9U16                wWordFreq,
                                      ET9U8                 bPrimLdbNum,
                                      ET9U8                 bExact,
                                      ET9U8                 bLowercase);

ET9STATUS ET9FARCALL _ET9AWRUDBWordsSearch(ET9AWLingInfo            *pLingInfo,
                                           ET9UINT                  wRecType,
                                           ET9U16                   wIndex,
                                           ET9U16                   wLength,
                                           ET9_FREQ_DESIGNATION     bFreqIndicator);

ET9UINT ET9FARCALL _ET9AWFindRUDBObject(ET9AWLingInfo               *pLingInfo,
                                        const ET9AWPrivWordInfo     *pWord,
                                        ET9UINT                     nRecordType,
                                        ET9UINT                     nIncrement);

void ET9FARCALL _ET9AWRUDBUpdateCounter(ET9AWLingInfo   *pLingInfo,
                                        ET9U8           value);

ET9STATUS ET9FARCALL _ET9AWGeneralUDBAddWord(ET9AWLingInfo *pLingInfo,
                                             ET9SYMB       *psBuf,
                                             ET9U16         wWordLen,
                                             ET9U16         wInitFreq);

void ET9FARCALL _ET9ProcessSelListQUDBEntries(ET9AWLingInfo  *pLingInfo,
                                              ET9U8           bIndex);

ET9U8 ET9FARDATA * ET9FARCALL _ET9AWMoveRUDBPtrRight(ET9AWRUDBInfo ET9FARDATA   *pRUDB,
                                                     const ET9U8 ET9FARDATA     *pbNext,
                                                     ET9UINT                    nNumMoves);

ET9UINT ET9FARCALL _ET9AWGetRecordLength(ET9AWRUDBInfo ET9FARDATA   *pRUDB,
                                         const ET9U8 ET9FARDATA     *pbReadFrom);

ET9UINT ET9FARCALL _ET9AWGetRecordType(const ET9U8 ET9FARDATA *pbReadFrom);

#define _ET9AWGetUDBWordLen(p1)       ((ET9U16)((*(ET9U8 ET9FARDATA *)p1) & 0x007F))

#define _ET9AWGetRDBWordLen(p1)       ((ET9U16)((*(ET9U8 ET9FARDATA *)p1) & 0x003F))

ET9U16 ET9FARCALL _ET9AWRUDBReadWord(ET9AWRUDBInfo ET9FARDATA   *pRUDB,
                                     const ET9U8 ET9FARDATA     *pbReadFrom);

ET9STATUS ET9FARCALL _ET9AWRDBDeleteWord(ET9AWLingInfo *pLingInfo,
                                         ET9SYMB       *psBuf,
                                         ET9U16         wWordLen);

#ifdef LONG_RUDB_TEST
void ET9FARCALL _TestRUDBIntegrity(ET9AWLingInfo  *pLingInfo);
#endif

#define  ET9NOINFO 0
#define  T9STEMTYPE 1
#define  T9TERMTYPE 2
#define  INSERTHERE 3
#define  INSERTBEFORE 4
#define  INSERTAFTER 5

#define  ET9UDBTYPE   1
#define  ET9RDBTYPE   2
#define  ET9FREETYPE  3     /* multi-byte free record */
#define  ET9GETFREQ   4     /* used as 'trim type */

#define ET9RDBTYPEMASK8      ((ET9U8)0xC0)    /* 2 MSBits of byte */
#define ET9FREETYPEMASK16    ((ET9U16)0x8000) /* 2 MSBits of u16 */
#define ET9ONEBYTEFREEMASK   ((ET9U8)0xA0)
#define ET9MEMERROR 0xFFFF

#define NUMSIZERANGES 10

#define UDB_RECORD_HEADER_SIZE  3   /* 1(length) + 2(freq) */
#define RDB_RECORD_HEADER_SIZE  4   /* 1(length) + 2(freq) + 1(ldbnum) */

#define UDB_WORDLEN_TO_RECLEN(w) ((ET9UINT) ((w * ET9SYMBOLWIDTH) + UDB_RECORD_HEADER_SIZE));
#define RDB_WORDLEN_TO_RECLEN(w) ((ET9UINT) ((w * ET9SYMBOLWIDTH) + RDB_RECORD_HEADER_SIZE));

/* The Udb data area pointer */

#define ET9RUDBData(pRUDB) ((ET9U8 ET9FARDATA *)pRUDB->bDataArea)

/* Macro to get actual number of bytes in the udb header area */

#define ET9RUDBHeaderBytes(pRUDB)    \
            ((ET9UINT)(ET9RUDBData(pRUDB) - (ET9U8 ET9FARDATA *)pRUDB))

/* Macro to get actual number of bytes in the udb data area */

#define ET9RUDBDataAreaBytes(pRUDB)    \
            ((ET9UINT)(pRUDB->wDataSize - ET9RUDBHeaderBytes(pRUDB)))

#define ET9TUDB_UDB     0
#define ET9TUDB_ASDB    1

#define ET9_TUDBRECFIXLEN 16
#define ET9_TRDBRECFIXLEN 20

/* XT9 TUDB RDB record data types
    Note: These data attributes are for records of type ET9TUDB_REORDERWORD_REC */

#define ET9TUDB_REORDERWORD_INVALID     0
#define ET9TUDB_REORDERWORD_LEN         1
#define ET9TUDB_REORDERWORD_FREQ        2
#define ET9TUDB_REORDERWORD_LDBNUM      3
#define ET9TUDB_REORDERWORD_SYMBOLS     4

/* XT9 TUDB custom word record data types
    Note: These data attributes are for records of type ET9TUDB_CUSTOMWORD_REC */

#define ET9TUDB_CUSTOMWORD_INVALID      0
#define ET9TUDB_CUSTOMWORD_LEN          1
#define ET9TUDB_CUSTOMWORD_FREQ         2
#define ET9TUDB_CUSTOMWORD_SYMBOLS      3

/* Standard T9 TUDB custom word record data types (typedef was copied from T9 v7.4)
    Note: These data attributes are for records of type T9TUDB_CUSTOMWORD_REC */

#define ET9TUDB_T9CUSTOMWORD_INVALID    0
#define ET9TUDB_T9CUSTOMWORD_LEN        1
#define ET9TUDB_T9CUSTOMWORD_FREQ       2
#define ET9TUDB_T9CUSTOMWORD_KEYS       3
#define ET9TUDB_T9CUSTOMWORD_SYMBOLS    4

ET9U32 ET9FARCALL _ET9AWRUDBGetSize(ET9AWLingInfo *pLingInfo,
                                    ET9U32        *pdwTotalWords);

ET9STATUS ET9FARCALL _ET9AWRUDBExport(ET9AWLingInfo       *pLingInfo,
                                      ET9U8 ET9FARDATA    *pTUdb,
                                      ET9U32               dwTUdbSize,
                                      ET9U32              *pdwExportSize,
                                      ET9WriteTUDB_f       ET9WriteTUDB,
                                      ET9AWTUDBNextRecord *pNextRecord,
                                      ET9U16              *pwRecordsExported);

ET9STATUS ET9FARCALL _ET9AWUDBImport(ET9AWLingInfo        *pLingInfo,
                                      ET9U8 ET9FARDATA    *pTUdb,
                                      ET9ReadTUDB_f        ET9ReadTUDB,
                                      ET9U32               dwTUdbOffset,
                                      ET9U16               wRecSize,
                                      ET9U8               *pbImported);

ET9STATUS ET9FARCALL _ET9AWRDBImport(ET9AWLingInfo        *pLingInfo,
                                      ET9U8 ET9FARDATA    *pTUdb,
                                      ET9ReadTUDB_f        ET9ReadTUDB,
                                      ET9U32               dwTUdbOffset,
                                      ET9U16               wRecSize,
                                      ET9U8               *pbImported);

ET9STATUS ET9FARCALL _T9UDBImport(ET9AWLingInfo     *pLingInfo,
                                  ET9U8 ET9FARDATA  *pTUdb,
                                  ET9ReadTUDB_f      ET9ReadTUDB,
                                  ET9U32             dwTUdbOffset,
                                  ET9U16             wRecSize,
                                  ET9U8             *pbImported);

#if defined(__cplusplus)
}
#endif

#endif /* ET9_ALPHABETIC_MODULE */
#endif /* T9UDB_H */


/* eof */
