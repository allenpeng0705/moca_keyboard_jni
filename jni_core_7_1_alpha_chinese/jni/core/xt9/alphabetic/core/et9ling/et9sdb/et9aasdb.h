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
;**     FileName: ET9AASDB.h                                                  **
;**                                                                           **
;**  Description: Auto Substitution data base access routines header file.    **
;**               Conforms to Version 3.0                                     **
;**                                                                           **
;*******************************************************************************
;******************************************************************************/

#ifndef ET9AASDB_H
#define ET9AASDB_H    1

#include "et9api.h"
#ifdef ET9_ALPHABETIC_MODULE
#include "et9aslst.h"

/* don't mangle the function name if compile under C++ */
#if defined(__cplusplus)
extern "C" {
#endif


ET9STATUS ET9FARCALL _ET9AWASDBWordsSearch(ET9AWLingInfo        *pLingInfo,
                                         ET9U16                 wIndex,
                                         ET9U16                 wLength,
                                         ET9_FREQ_DESIGNATION   bFreqIndicator);

void ET9FARCALL _ET9AWLdbASInit(ET9AWLingInfo *pLingInfo, ET9U16 wLdbNum, ET9U16 wReset);

ET9UINT ET9FARCALL _ET9AWFindASDBObject(ET9AWLingInfo     *pLingInfo,
                                        ET9SYMB           *pWord,
                                        ET9U16             wLength,
                                        ET9U8              bCaseSensitive,
                                        ET9U8              bUpdateWithMatch);

void ET9FARCALL _ET9AWLdbASDisableRecord(ET9AWLingInfo *pLingInfo,
                                         ET9U16         wLDBID,
                                         ET9U16         wASRecordNum);

ET9U8 ET9FARCALL _ET9AWLdbASRecordEnabled(ET9AWLingInfo *pLingInfo,
                                          ET9U16         wLDBID,
                                          ET9U16         wASRecordNum);

#ifdef LONG_ASDB_TEST
void ET9FARCALL _TestASDBIntegrity(ET9AWLingInfo  *pLingInfo);
#endif

/* Defined ASDB records will have the MSBit of the status byte set %100xxxxx    */
/* Multi-byte free records will bits 7-5 of the status byte set %011xxxxx       */
/*    This size of these records will be a word, with the                       */
/*            MSByte = the remaining 5 bits of the status byte                  */
/*            LSByte = the following byte                                       */
/*     e.g. A record with the first 2 bytes = 0x6121 would be 289 bytes long    */
/* Single-byte free records will have bits 7-5 of the status byte set %010xxxxx */

#define ASDB_RECORD_HEADER_SIZE  4   /* 1(status) + 1(free) + 1(shortcut length) + 1(sub length) */

typedef struct ET9ASDBRecordHeader_s {
    ET9U8 bStatus;
    ET9U8 bFree;
    ET9U8 bShortcutLen;      /* in symbols */
    ET9U8 bSubstitutionLen;  /* in symbols */
} ET9ASDBRecordHeader;

#define ASDB_WORDLEN_TO_RECLEN(x,y) ((ET9UINT) ((x * ET9SYMBOLWIDTH) + (y * ET9SYMBOLWIDTH) + ASDB_RECORD_HEADER_SIZE));

/* Status bits for defined ASDB Records (bits 7-6 reserved for type */

#define ASDB_STATUS_USED_BIT                0
#define ASDB_STATUS_CASE_SENSITIVE_BIT      1
#define ASDB_STATUS_USED_BITMASK            ((ET9U8)((ET9U8)1 << ASDB_STATUS_USED_BIT))
#define ASDB_STATUS_CASE_SENSITIVE_BITMASK  ((ET9U8)((ET9U8)1 << ASDB_STATUS_CASE_SENSITIVE_BIT))

#define  ET9ASDBUNKNOWN        0
#define  ET9ASDBTYPE           1
#define  ET9ASMULTIFREETYPE    2     /* multi-byte free record */
#define  ET9ASSINGLEFREETYPE   3     /* single-byte free record */

#define ET9ASDBRECORDTYPEMASK ((ET9U8)0xE0)
#define ET9ASDBTYPEMASK       ((ET9U8)0x80)
#define ET9MULTIFREETYPEMASK  ((ET9U8)0x60)
#define ET9SINGLEFREETYPEMASK ((ET9U8)0x40)

/* The ASDB LDB-supported AS record tracking area pointer */

#define ET9ASDBLDBRecs(asdb) ((ET9U8 ET9FARDATA *)asdb->sLdbRecs)

/* The ASDB data area pointer */

#define ET9ASDBData(asdb) ((ET9U8 ET9FARDATA *)asdb->bDataArea)

/* Macro to get actual number of bytes in the ASDB header area */

#define ET9ASDBHeaderBytes(asdb)    \
            ((ET9UINT)(ET9ASDBData(asdb) - (ET9U8 ET9FARDATA *)asdb))

/* Macro to get actual number of bytes in the ASDB data area */

#define ET9ASDBDataAreaBytes(asdb)    \
            ((ET9UINT)(asdb->wDataSize - ET9ASDBHeaderBytes(asdb)))

/* XT9 TUDB ldb rec map data types
    Note: These data attributes are for records of type ET9TUDB_CUSTOMWORD_REC */

#define ET9TUDB_LDBRECMAP_INVALID               0
#define ET9TUDB_LDBRECMAP_LDBNUM                1
#define ET9TUDB_LDBRECMAP_TOTALRECS             2
#define ET9TUDB_LDBRECMAP_TOTALENABLERECS       3
#define ET9TUDB_LDBRECMAP_BITMAPS               4

#define ET9TUDB_USERASWORD_INVALID              0
#define ET9TUDB_USERASWORD_SHORTCUT_LEN         1
#define ET9TUDB_USERASWORD_SHORTCUTWORD         2
#define ET9TUDB_USERASWORD_SUBWORD_LEN          3
#define ET9TUDB_USERASWORD_SUBWORD              4

#define ET9_LASRECLEN 53

ET9U32 ET9FARCALL _ET9AWASDBGetSize(ET9AWLingInfo *pLingInfo,
                                    ET9U32        *pdwTotalWords);

ET9STATUS ET9FARCALL _ET9AWLASDBExport(ET9AWLingInfo       *pLingInfo,
                                       ET9U8 ET9FARDATA    *pTUdb,
                                       ET9U32               dwTUdbSize,
                                       ET9U32              *pdwExportSize,
                                       ET9WriteTUDB_f       ET9WriteTUDB,
                                       ET9AWTUDBNextRecord *pNextRecord,
                                       ET9U16              *pwRecordsExported);

ET9STATUS ET9FARCALL _ET9AWUASDBExport(ET9AWLingInfo       *pLingInfo,
                                       ET9U8 ET9FARDATA    *pTUdb,
                                       ET9U32               dwTUdbSize,
                                       ET9U32              *pdwExportSize,
                                       ET9WriteTUDB_f       ET9WriteTUDB,
                                       ET9AWTUDBNextRecord *pNextRecord,
                                       ET9U16              *pwRecordsExported);

ET9STATUS ET9FARCALL _ET9AWLDBBMImport(ET9AWLingInfo       *pLingInfo,
                                       ET9U8 ET9FARDATA    *pTUdb,
                                       ET9ReadTUDB_f        ET9ReadTUDB,
                                       ET9U32               dwTUdbOffset,
                                       ET9U16               wRecSize,
                                       ET9U8               *pbImported);

ET9STATUS ET9FARCALL _ET9AWASDBImport(ET9AWLingInfo       *pLingInfo,
                                      ET9U8 ET9FARDATA    *pTUdb,
                                      ET9ReadTUDB_f        ET9ReadTUDB,
                                      ET9U32               dwTUdbOffset,
                                      ET9U16               wRecSize,
                                      ET9U8               *pbImported);


#if defined(__cplusplus)
}
#endif

#endif /* ET9_ALPHABETIC_MODULE */
#endif /* ET9AASDB_H */


/* eof */
