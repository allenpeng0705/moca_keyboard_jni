/* This file corresponds to /et9/udbwords/core/udb_words.c revision 1.2 in Tegic CVS, with some minor local changes. */

/*******************************************************************************
;*******************************************************************************
;**                                                                           **
;**                  COPYRIGHT 2006-2009 NUANCE COMMUNICATIONS                **
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
;**     FileName: udb_words.c                                                 **
;**                                                                           **
;**  Description: For reading out UDB content.                                **
;**                                                                           **
;**                                                                           **
;*******************************************************************************
;******************************************************************************/

#include "et9api.h"
#include "et9misc.h"
#include "udb_words.h"


/* Given b = lower byte, a = upper byte; compose a word integral (ET9U16). */
#define ET9MAKEWORD(a, b)  ((ET9U16)(((ET9U8)(b)) | ((ET9U16)((ET9U8)(a))) << 8) )
#define ET9LOBYTE(w)       ((ET9U8)(w))
#define ET9HIBYTE(w)       ((ET9U8)(((ET9U16)(w) >> 8)))

#define __GetUDBWordLen(p1)       ((ET9U16)((*(ET9U8 ET9FARDATA *)p1) & 0x007F))
#define __GetRDBWordLen(p1)       ((ET9U16)((*(ET9U8 ET9FARDATA *)p1) & 0x003F))

/* ******************************************************************** */ 

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

#define ET9MAX_FREQ_COUNT           ((ET9U16)(64000)) /* Max Freq. resolution */

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

/*---------------------------------------------------------------------------*/
/** \internal
 * Check for wrap (byte).
 *
 * @param pbPtr                     Byte pointer.
 * @param pRUDB                     Pointer to UDB.
 *
 * @return None
 */

#define __CheckForWrap_ByteP(pbPtr, pRUDB)                                      \
{                                                                               \
    if (pbPtr >= (ET9RUDBData(pRUDB) + ET9RUDBDataAreaBytes(pRUDB))) {          \
        pbPtr -= ET9RUDBDataAreaBytes(pRUDB);                                   \
    }                                                                           \
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Check for wrap (word).
 *
 * @param pwPtr                     Word pointer.
 * @param pRUDB                     Pointer to UDB.
 *
 * @return None
 */

#define __CheckForWrap_WordP(pwPtr, pRUDB)                                                      \
{                                                                                               \
    if ((ET9U8 ET9FARDATA *)pwPtr >= (ET9RUDBData(pRUDB) + ET9RUDBDataAreaBytes(pRUDB))) {      \
        pwPtr = (ET9U16 ET9FARDATA *)((ET9U8 ET9FARDATA *)pwPtr - ET9RUDBDataAreaBytes(pRUDB)); \
    }                                                                                           \
}


/*---------------------------------------------------------------------------*/
/** \internal
 * Move the byte pointer to the right.
 * Circle back to the beginning if at or past the end of the database.
 *
 * @param pRUDB          Pointer to UDB.
 * @param pbNext         Position to move from.
 * @param nNumMoves      Number of bytes to move.
 *
 * @return New pointer location.
 */

static ET9U8 ET9FARDATA * ET9LOCALCALL __MoveRUDBPtrRight(ET9AWRUDBInfo       ET9FARDATA *pRUDB,
                                                          const ET9U8         ET9FARDATA *pbNext,
                                                          ET9UINT                        nNumMoves)
{
    ET9Assert(pRUDB);
    ET9Assert(pbNext);

    pbNext += nNumMoves;
    __CheckForWrap_ByteP(pbNext, pRUDB);
    return (ET9U8 ET9FARDATA *)pbNext;
}


/*---------------------------------------------------------------------------*/
/** \internal
 * Read a word from the RUDB.
 * NOTE: This function assumes that the 'pReadFrom' pointer is  valid. The caller should validate the pointer.
 *
 * @param pRUDB          Pointer to RUDB.
 * @param pbReadFrom     Byte stream position to read from.
 *
 * @return Data read.
 */

static ET9U16 ET9LOCALCALL __RUDBReadWord(ET9AWRUDBInfo       ET9FARDATA *pRUDB,
                                          const ET9U8         ET9FARDATA *pbReadFrom)
{
    const ET9U8 ET9FARDATA *pbByte;

    ET9Assert(pRUDB);
    ET9Assert(pbReadFrom);

    pbByte = pbReadFrom + 1;
    __CheckForWrap_ByteP(pbByte, pRUDB);
    return (ET9MAKEWORD(*pbReadFrom, *pbByte));
}


/*---------------------------------------------------------------------------*/
/** \internal
 * Determines type of record located at pbReadFrom.
 * This function assumes that the 'pReadFrom' pointer is valid
 * (i.e. pointing to the beginning of an actual RUDB record).<P>
 * The caller should validate the pointer.
 *
 * @param pbReadFrom         Pointer to record position in  RUDB to process.
 *
 * @return Record type: ET9UDBTYPE, ET9RDBTYPE, ET9FREETYPE.
 */

static ET9UINT ET9LOCALCALL __GetRecordType(const ET9U8 ET9FARDATA *pbReadFrom)
{
    ET9U8 bData;

    ET9Assert(pbReadFrom);

    bData = *pbReadFrom;
    if ((bData >> 0x7) == 0) {          /* MSBit is 0, udb type */
        bData = ET9UDBTYPE;
    }
    else if ((bData >> 0x6) == 0x03) {  /* 2 MSBits are 11, rdb type */
        bData = ET9RDBTYPE;
    }
    else if ((bData >> 0x6) == 0x02) {  /* 2 MSBits are 10, free */
        bData = ET9FREETYPE;
    }
    return bData;
}


/*---------------------------------------------------------------------------*/
/** \internal
 * Determines the length (in bytes) of the RUDB record pointed to by pbReadFrom.
 * This function assumes that the 'pReadFrom' pointer is pointing to the beginning ogf an actual valid RUDB record.<P>
 * The caller should validate the pointer.
 *
 * @param pRUDB          Pointer to RUDB information structure.
 * @param pbReadFrom     Pointer to record position in RUDB to process.
 *
 * @return Length of record in bytes.
 */

static ET9UINT ET9LOCALCALL __GetRecordLength(ET9AWRUDBInfo       ET9FARDATA *pRUDB,
                                              const ET9U8         ET9FARDATA *pbReadFrom)
{
    ET9UINT nRecType, nLength;
    ET9U8   bData;

    ET9Assert(pRUDB);
    ET9Assert(pbReadFrom);

    nRecType = __GetRecordType(pbReadFrom);
    bData = *pbReadFrom;

    if (nRecType == ET9UDBTYPE) {        /* udb type */
        nLength = UDB_WORDLEN_TO_RECLEN(bData);
    }
    else if (nRecType == ET9RDBTYPE) {   /* rdb type */
        bData &= 0x3F;
        nLength = RDB_WORDLEN_TO_RECLEN(bData);
    }
    else if (bData == ET9ONEBYTEFREEMASK) {
        nLength = 1;
    }
    else {
        bData &= 0x1F;
        nLength = (ET9UINT)bData << 8;
        ++pbReadFrom;
        __CheckForWrap_ByteP(pbReadFrom, pRUDB);
        bData = *pbReadFrom;
        nLength += bData;
    }
    return nLength;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Determines the complete RUDB checksum.
 * Defined as: Checksum(non-header RUDB data) + __ET9AWRUDBHeaderChecksum(RUDB).
 *
 * @param pLingInfo      Pointer to field information structure.
 *
 * @return 16 bit RUDB checksum value.
 */

static ET9U16 ET9LOCALCALL __GetRUDBChecksum(ET9AWRUDBInfo * const pRUDB)
{
    ET9U8 ET9FARDATA  *pbUdbData;
    ET9U8 ET9FARDATA  *pbData;
    ET9U16        wSizeChecked;
    ET9U16        wCheckSum;
    ET9UINT       nRecordLength, nRecordType, nIndex;
    ET9UINT       nRemainingMemory = 0;

    ET9Assert(pRUDB);

    /* checksum of the RUDB header */
    wCheckSum = (ET9U16)(pRUDB->wDataSize +
                         pRUDB->wUDBWordCount +
                         pRUDB->wRDBWordCount +
                         pRUDB->wRemainingMemory +
                         pRUDB->wLastDelCutOffFreq);
    for (nIndex = 0; nIndex < NUMSIZERANGES; ++nIndex) {
        wCheckSum = (ET9U16)(wCheckSum + pRUDB->wSizeOffset[nIndex]);
    }

    /* start at the beginning of the smallest size entry region */
    pbUdbData = ET9RUDBData(pRUDB) + pRUDB->wSizeOffset[0];
    wSizeChecked = 0;

    /* loop until entire database is covered */
    /* this is just a safety check to close the loop */
    while (wSizeChecked < ET9RUDBDataAreaBytes(pRUDB)) {
        /* get the record type/size */
        nRecordType   = __GetRecordType(pbUdbData);
        nRecordLength = __GetRecordLength(pRUDB, pbUdbData);
        /* if bogus length (CR 19696), return incorrect value (so that reset occurs) */
        if (!nRecordLength ||
            ((nRecordType == ET9UDBTYPE) && (nRecordLength > ((ET9UINT)((ET9MAXUDBWORDSIZE * ET9SYMBOLWIDTH) + UDB_RECORD_HEADER_SIZE)))) ||
            ((nRecordType == ET9RDBTYPE) && (nRecordLength > ((ET9UINT)((ET9MAXUDBWORDSIZE * ET9SYMBOLWIDTH) + RDB_RECORD_HEADER_SIZE)))) ||
            ((nRecordType == ET9FREETYPE) && (nRecordLength > ET9RUDBDataAreaBytes(pRUDB)))) {
            return (pRUDB->wDataCheck - 1);
        }

        /* only length/type field goes toward the checksum for free records */
        nIndex = (nRecordType == ET9FREETYPE) ?
                     ((nRecordLength > 1) ? 2 : 1) : nRecordLength;
        /* keep track of the free memory still in RUDB for later check */
        if (nRecordType == ET9FREETYPE) {
            nRemainingMemory += nRecordLength;
        }

        /* now loop through the entire record to sum it's bytes */
        pbData = pbUdbData;
        while (nIndex--) {
            wCheckSum = (ET9U16)(wCheckSum + *pbData);
            ++pbData;
            __CheckForWrap_ByteP(pbData, pRUDB);
        }

        /* update the over size tracker */
        wSizeChecked = (ET9U16)(wSizeChecked + nRecordLength);
        /* and point to the next record */
        pbUdbData += nRecordLength;
        __CheckForWrap_ByteP(pbUdbData, pRUDB);
    }

    /* remove the range markers from available size */
    nRemainingMemory -= NUMSIZERANGES;
    /* the free size calculated SHOULD match the value maintained in the header */
    ET9Assert(nRemainingMemory == (ET9UINT)pRUDB->wRemainingMemory);

    return wCheckSum;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Sets range info for searching a specified portion of the RUDB.
 * Initially, there will be 10 size regions divided into the following categories:<P>
 *      Region 0    - words 1-3 characters long<BR>
 *      Region 1    - words 4 characters long<BR>
 *      Region 2    - words 5 characters long<BR>
 *      Region 3    - words 6 characters long<BR>
 *      Region 4    - words 7 characters long<BR>
 *      Region 5    - words 8 characters long<BR>
 *      Region 6    - words 9-10 characters long<BR>
 *      Region 7    - words 11-12 characters long<BR>
 *      Region 8    - words 13-15 characters long<BR>
 *      Region 9    - words > 15 characters long
 *
 * @param pLingInfo              Pointer to field information structure.
 * @param wLength                Length (in symbols) of word being searched.
 * @param bIncludeCompletions    Whether or not completions accepted.
 * @param wRegionIndex           Pointer to position to store initial region index.
 * @param ppbStart               Pointer to pointer to store range start pointer.
 * @param ppbEnd                 Pointer to pointer to store range end pointer.
 *
 * @return None (*ppStart / *ppEnd contain boundary addresses of RUDB.
 *           segment containing indicated ranges; *wRegionIndex contains
 *           initial region index)
 */

static void ET9LOCALCALL __SetRUDBRangeInfo(ET9AWRUDBInfo  * const pRUDB,
                                            ET9U16                 wLength,
                                            ET9U8                  bIncludeCompletions,
                                            ET9U16                 *wRegionIndex,
                                            ET9U8 ET9FARDATA       **ppbStart,
                                            ET9U8 ET9FARDATA       **ppbEnd)
{
    ET9U16 wRegion;

    ET9Assert(pRUDB);
    ET9Assert(wRegionIndex);
    ET9Assert(ppbStart);
    ET9Assert(ppbEnd);

    ET9Assert(pRUDB != NULL);

    if (wLength <= 3) {
        wRegion = 0;
    }
    else if (wLength <= 8) {
        /* since regions 1 - 5 only contain 1 word length each */
        wRegion = (ET9U16)(wLength - 3);
    }
    else if (wLength <= 10) {
        wRegion = 6;
    }
    else if (wLength <= 12) {
        wRegion = 7;
    }
    else if (wLength <= 15) {
        wRegion = 8;
    }
    else {
        /* anything > 15 syms long */
        wRegion = 9;
    }
    /* load the return values, including the region */
    *wRegionIndex = wRegion;
    /* the start address of the region */
    *ppbStart = ET9RUDBData(pRUDB) + pRUDB->wSizeOffset[wRegion];
    /* and the end address of the region (or region group) */
    /* if completions included,  */
    if (bIncludeCompletions) {
        /* include all regions after this one as well */
        *ppbEnd = ET9RUDBData(pRUDB) + pRUDB->wSizeOffset[0];
    }
    else {
        /* only examine this one region */
        *ppbEnd = ET9RUDBData(pRUDB) + pRUDB->wSizeOffset[(wRegion + 1) % NUMSIZERANGES];
    }
}

/*---------------------------------------------------------------------------*/
/**
 * Sets up the initial RUDB.
 * Either using a previously saved RUDB or a 'fresh' memory buffer.
 * Will reset the provided RUDB if it looks corrupted.
 *
 * @param pRUDBInfo      Pointer to RUDB buffer.
 * @param wDataSize      Size (in bytes) of provided RUDB buffer.
 *
 * @return ET9STATUS_NONE on success, otherwise return error code.
 */

static ET9STATUS ET9LOCALCALL __RUDBInit(ET9AWRUDBInfo ET9FARDATA * const     pRUDBInfo,
                                         const ET9U16                         wDataSize)
{
    if (((pRUDBInfo != NULL) && !wDataSize) || ((pRUDBInfo == NULL) && wDataSize)) {
        return ET9STATUS_INVALID_MEMORY;
    }
    /* Check minimum RUDB size                 */
    else if ((pRUDBInfo != NULL) && (wDataSize < ET9MINRUDBDATABYTES)) {
         return ET9STATUS_INVALID_SIZE;
    }
    else {

        /*  If checksum doesn't match, set up an empty RUDB with cur symbol class.*/

        if (((pRUDBInfo->wUDBWordCount + pRUDBInfo->wRDBWordCount) == 0) ||
            (pRUDBInfo->wDataSize != wDataSize) ||
            (pRUDBInfo->wDataCheck != (ET9U16)__GetRUDBChecksum(pRUDBInfo))) {

				/* Local Decuma modification; make it possible to detect corrupted UDB */
            return ET9STATUS_CORRUPT_DB;
        }
    }

    return ET9STATUS_NONE;
}

/* ******************************************************************** */ 

ET9STATUS UDBWORDS_GetWords (ET9U8               * const pbUdbData,
                             const ET9U16                wUdbDataSize,
                             void                * const pOwner,
                             pHandleUdbWord_t            pWordHandler)
{
    ET9STATUS               wStatus = ET9STATUS_NONE;
    ET9U8 ET9FARDATA       *pbNext;
    ET9U8 ET9FARDATA       *pbEnd;
    ET9UINT                 nRecordLength;
    ET9UINT                 nRecordType;
    ET9U16                  wRegion;
    ET9U16                  wFreqIndex = 0;
    ET9UINT                 wSize, i;
    const ET9U8 ET9FARDATA *pSymb;
    ET9SYMB                *pHolder;
    const ET9U8 ET9FARDATA *pbByte;
    ET9U8                   bLdbNum;
    UDBWORDS_Word           sWord;

    ET9AWRUDBInfo ET9FARDATA * const pRUDB = (ET9AWRUDBInfo*)pbUdbData;

    if (!pbUdbData) {
        return ET9STATUS_BAD_PARAM;
    }
    if (!pWordHandler) {
        return ET9STATUS_BAD_PARAM;
    }

    wStatus = __RUDBInit(pRUDB, wUdbDataSize);

    if (wStatus) {
        return wStatus;
    }

    __SetRUDBRangeInfo(pRUDB, 0, 1, &wRegion, &pbNext, &pbEnd);

    /* loop through qualifying size regions in the RUDB */

    do {

        /* if this is an RDB record */

        nRecordType = __GetRecordType(pbNext);

        if (nRecordType != ET9FREETYPE) {

            /* go see if it is a possible selection list candidate */

            if (nRecordType == ET9RDBTYPE) {

                /* Transfer the RDB record to the ET9AWPrivWordInfo struct */
                /* get the length of the RDB word */

                wSize = __GetRDBWordLen(pbNext);

                /* only continue if same language id */

                bLdbNum = (ET9U8)*(__MoveRUDBPtrRight(pRUDB, pbNext, (RDB_RECORD_HEADER_SIZE - 1)));

                /* get the frequency */
                /* Read Word */

                pSymb = pbNext + 1;
                __CheckForWrap_ByteP(pSymb, pRUDB);
                pbByte = pSymb + 1;
                __CheckForWrap_ByteP(pbByte, pRUDB);

                sWord.wLang = bLdbNum;
                sWord.wFreq = ET9MAKEWORD(*pSymb, *pbByte);
                sWord.wLen  = (ET9U16)wSize;
                sWord.eType = UDBWORDS_WordType_RUDB;

                /* skip past length, LDB and frequency bytes in record header */

                pSymb = pbNext + RDB_RECORD_HEADER_SIZE;
                __CheckForWrap_ByteP(pSymb, pRUDB);

                pHolder = sWord.psString;

                /* loop through and transfer word */

                for (i = wSize; i; --i) {
                    pbByte = pSymb + 1;
                    __CheckForWrap_ByteP(pbByte, pRUDB);
                    *pHolder++ = ET9MAKEWORD(*pSymb, *pbByte);
                    pSymb = pbByte + 1;
                    __CheckForWrap_ByteP(pSymb, pRUDB);
                }
            }
            else if (nRecordType == ET9UDBTYPE) {

                /* Transfer the UDB record to the ET9AWPrivWordInfo struct */
                /* get the length of the UDB word */

                wSize = __GetUDBWordLen(pbNext);

                /* get the frequency */
                /* Read Word */

                pSymb = pbNext + 1;
                __CheckForWrap_ByteP(pSymb, pRUDB);
                pbByte = pSymb + 1;
                __CheckForWrap_ByteP(pbByte, pRUDB);

                sWord.wLang = ET9PLIDNone;
                sWord.wFreq = ET9MAKEWORD(*pSymb, *pbByte);
                sWord.wLen  = (ET9U16)wSize;
                sWord.eType = UDBWORDS_WordType_UDB;

                pSymb = pbNext + UDB_RECORD_HEADER_SIZE;
                __CheckForWrap_ByteP(pSymb, pRUDB);

                pHolder = sWord.psString;

                /* loop through and transfer word */

                for (i = wSize; i; --i) {
                    pbByte = pSymb + 1;
                    __CheckForWrap_ByteP(pbByte, pRUDB);
                    *pHolder++ = ET9MAKEWORD(*pSymb, *pbByte);
                    pSymb = pbByte + 1;
                    __CheckForWrap_ByteP(pSymb, pRUDB);
                }

            }
            else {
                ET9Assert(0);
            }

            /* if possible selection, go verify */

            if (wStatus == ET9STATUS_NONE) {

                /* if this is a 'questionable' UDB entry, mark as QUDB word source */

                if (nRecordType == ET9UDBTYPE && sWord.wFreq > ET9MAX_FREQ_COUNT) {
                    sWord.eType = UDBWORDS_WordType_QUDB;
                    sWord.wFreq = 0;
                }

				/* Local addition by Decuma; allow the handler function to stop parsing and pass its error code upwards */
                wStatus = pWordHandler(pOwner, &sWord);
				if ( wStatus != ET9STATUS_NONE ) 
					return wStatus;
            }
        }

        /* move to the next record */

        nRecordLength = __GetRecordLength(pRUDB, pbNext);
        pbNext += nRecordLength;
        __CheckForWrap_ByteP(pbNext, pRUDB);

    } while (pbNext != pbEnd);

    return wStatus;
}


/* ----------------------------------< eof >--------------------------------- */
