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
;**     FileName: et9arudb.c                                                  **
;**                                                                           **
;**  Description: User data base access routines source file.                 **
;**                                                                           **
;*******************************************************************************
;******************************************************************************/

/*! \addtogroup et9arudb RDB/UDB for alphabetic
* User database logic for alphabetic XT9.
* @{
*/

#include "et9api.h"
#include "et9asys.h"
#include "et9adb.h"
#include "et9amdb.h"
#include "et9arudb.h"
#include "et9amisc.h"
#include "et9sym.h"
#include "et9asym.h"
#include "et9aspc.h"
#include "et9aasdb.h"
#include "et9alsasdb.h"

#ifdef ET9_ALPHABETIC_MODULE

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

ET9U8 ET9FARDATA * ET9FARCALL _ET9AWMoveRUDBPtrRight(ET9AWRUDBInfo ET9FARDATA *pRUDB,
                                               const ET9U8         ET9FARDATA *pbNext,
                                                     ET9UINT                   nNumMoves)
{
    ET9Assert(pRUDB);
    ET9Assert(pbNext);

    pbNext += nNumMoves;
    __CheckForWrap_ByteP(pbNext, pRUDB);

    return (ET9U8 ET9FARDATA *)pbNext;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Handle ALL writes to RUDB memory here.
 *
 * @param pLingInfo      Pointer to field information structure.
 * @param pTo            Where in memory write begins.
 * @param pFrom          Location of data to write.
 * @param nSize          How much data.
 *
 * @return None
 */

static void ET9LOCALCALL __ET9AWWriteRUDBData(ET9AWLingInfo *pLingInfo,
                                         void ET9FARDATA    *pTo,
                                   const void ET9FARDATA    *pFrom,
                                              ET9UINT        nSize)
{
    ET9U8 ET9FARDATA *pbTo   = (ET9U8 ET9FARDATA *)pTo + nSize - 1;
    ET9U8 ET9FARDATA *pbFrom = (ET9U8 ET9FARDATA *)pFrom + nSize - 1;

    ET9Assert(pLingInfo);
    ET9Assert(pTo);
    ET9Assert(pFrom);
    ET9Assert(nSize);

    /* if OEM is handling RUDB writes... else write data locally */

    if (pLingInfo->pRUDBWriteData) {
        pLingInfo->pRUDBWriteData(pLingInfo, pbTo, pbFrom, nSize);
    }
    else {

        while (nSize--) {

            ET9Assert(pbTo >= (ET9U8 *)pLingInfo->pLingCmnInfo->pRUDBInfo);
            ET9Assert(pbTo <= (ET9U8 *)(pLingInfo->pLingCmnInfo->pRUDBInfo + (pLingInfo->pLingCmnInfo->pRUDBInfo->wDataSize - 1)) || !pLingInfo->pLingCmnInfo->pRUDBInfo->wDataSize);

            *pbTo-- = *pbFrom--;  /* ignore invalid insure warnings */
        }
    }
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

ET9U16 ET9FARCALL _ET9AWRUDBReadWord(ET9AWRUDBInfo       ET9FARDATA *pRUDB,
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
 * Write a word to the RUDB (endian independent).
 * NOTE: This function assumes that the 'pWriteTo' pointer is valid. The caller should validate the pointer.
 *
 * @param pLingInfo      Pointer to field information structure.
 * @param pWriteTo       Pointer to RUDB position to write to.
 * @param wWord          Word data to write.
 *
 * @return None
 */

static void ET9LOCALCALL __ET9AWRUDBWriteWord(ET9AWLingInfo    *pLingInfo,
                                              ET9U8 ET9FARDATA *pWriteTo,
                                              ET9U16            wWord)
{
    ET9AWRUDBInfo ET9FARDATA * const pRUDB = pLingInfo->pLingCmnInfo->pRUDBInfo;

    ET9U8 hi_byte;
    ET9U8 lo_byte;

    ET9Assert(pLingInfo);
    ET9Assert(pWriteTo);
    ET9Assert(pRUDB != NULL);

    /* write upper byte */

    hi_byte = ET9HIBYTE(wWord);
    __ET9AWWriteRUDBData(pLingInfo, (void ET9FARDATA *)pWriteTo, (const void ET9FARDATA *)&hi_byte, 1);
    ++pWriteTo;
    __CheckForWrap_ByteP(pWriteTo, pRUDB);

    /* write lower byte */

    lo_byte = ET9LOBYTE(wWord);
    __ET9AWWriteRUDBData(pLingInfo, (void ET9FARDATA *)pWriteTo, (const void ET9FARDATA *)&lo_byte, 1 );
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Construct and write an RUDB record header (based on record type).
 * NOTE: This function assumes that the 'pbDest' pointer is valid. The caller should validate the pointer.
 *
 * @param pLingInfo      Pointer to field information structure.
 * @param pbDest         Pointer to record location in RUDB.
 * @param nLength        Length in record header (total bytes for ET9FREETYPE or word length for UDB/RDB record type).
 * @param nType          Record type.
 * @param wCheckSum      Original checksum (updated checksum is returned).
 *
 * @return Updated RUDB checksum after write.
 */

static ET9U16 ET9LOCALCALL __ET9AWRUDBWriteHeader(ET9AWLingInfo    *pLingInfo,
                                                  ET9U8 ET9FARDATA *pbDest,
                                                  ET9UINT           nLength,
                                                  ET9UINT           nType,
                                                  ET9U16            wCheckSum)
{
    ET9U16 wTemp;
    ET9U8  bTemp;

    ET9Assert(pLingInfo);
    ET9Assert(pbDest);
    ET9Assert(nLength);

    /* if record is RDB or UDB entry */

    if (nType != ET9FREETYPE) {

        /* prepare the record header with the size */
        /* if RDB record */

        if (nType == ET9RDBTYPE) {

            /* scheme limits entry to 6 bits */

            if (nLength > (ET9UINT)0x3F) {
                nLength = (ET9UINT)0x3F;
            }
            bTemp = (ET9U8)(ET9RDBTYPEMASK8 + nLength);
        }
        else {

            /* it had _better_ be a UDB record */

            ET9Assert(nType == ET9UDBTYPE);
            bTemp = (ET9U8)nLength;
        }
        wCheckSum = (ET9U16)(wCheckSum + bTemp);
        __ET9AWWriteRUDBData(pLingInfo, (void ET9FARDATA *)pbDest, (const void ET9FARDATA *)&bTemp, 1 );
    }

    /* if this is a 1-byte free record */

    else if (nLength == 1) {
        bTemp = (ET9U8)ET9ONEBYTEFREEMASK;
        wCheckSum = (ET9U16)(wCheckSum + bTemp);
        __ET9AWWriteRUDBData(pLingInfo, (void ET9FARDATA *)pbDest, (const void ET9FARDATA *)&bTemp, 1 );
    }

    /* otherwise this is a multi-byte free record */

    else {
        wTemp = (ET9U16)(ET9FREETYPEMASK16 + nLength);
        wCheckSum = (ET9U16)(wCheckSum + ET9HIBYTE(wTemp) + ET9LOBYTE(wTemp));
        __ET9AWRUDBWriteWord(pLingInfo, pbDest, wTemp);
    }

    return wCheckSum;
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

ET9UINT ET9FARCALL _ET9AWGetRecordType(const ET9U8 ET9FARDATA *pbReadFrom)
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

ET9UINT ET9FARCALL _ET9AWGetRecordLength(ET9AWRUDBInfo ET9FARDATA *pRUDB,
                                   const ET9U8         ET9FARDATA *pbReadFrom)
{
    ET9UINT nRecType, nLength;
    ET9U8   bData;

    ET9Assert(pRUDB);
    ET9Assert(pbReadFrom);

    nRecType = _ET9AWGetRecordType(pbReadFrom);
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

#ifdef ET9_DEBUG

/*---------------------------------------------------------------------------*/
/** \internal
 * Verify that all key ranges start with a single one byte free record.
 *
 * @param pRUDB      Pointer to RUDB information structure.
 *
 * @return None
 */

static void ET9LOCALCALL __VerifySingleBytes(ET9AWRUDBInfo ET9FARDATA *pRUDB) {

    ET9UINT nIndex, nRecordLength, nRecordType;
    ET9U8 *pbUdbData;

    ET9Assert(pRUDB);

    for (nIndex = 0; nIndex < NUMSIZERANGES; ++nIndex) {

        pbUdbData = ET9RUDBData(pRUDB) + pRUDB->wSizeOffset[nIndex];
        nRecordLength = _ET9AWGetRecordLength(pRUDB, pbUdbData);
        nRecordType = _ET9AWGetRecordType(pbUdbData);

        ET9Assert(nRecordLength == 1);
        ET9Assert(nRecordType == ET9FREETYPE);
    }
}

#endif

/*---------------------------------------------------------------------------*/
/** \internal
 * Determines the complete RUDB checksum.
 * Defined as: Checksum(non-header RUDB data) + __ET9AWRUDBHeaderChecksum(RUDB).
 *
 * @param pLingInfo      Pointer to field information structure.
 *
 * @return 16 bit RUDB checksum value.
 */

static ET9U16 ET9LOCALCALL __ET9AWGetRUDBChecksum(ET9AWLingInfo *pLingInfo)
{
    ET9U8 ET9FARDATA  *pbUdbData;
    ET9U8 ET9FARDATA  *pbData;
    ET9AWRUDBInfo ET9FARDATA * const pRUDB = pLingInfo->pLingCmnInfo->pRUDBInfo;
    ET9U16        wSizeChecked;
    ET9U16        wCheckSum;
    ET9UINT       nRecordLength, nRecordType, nIndex;
    ET9UINT       nRemainingMemory = 0;

    ET9Assert(pLingInfo);
    ET9Assert(pRUDB != NULL);

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

        nRecordType   = _ET9AWGetRecordType(pbUdbData);
        nRecordLength = _ET9AWGetRecordLength(pRUDB, pbUdbData);

        /* if bogus length (CR 19696), return incorrect value (so that reset occurs) */

        if (!nRecordLength ||
            ((nRecordType == ET9UDBTYPE) && (nRecordLength > ((ET9UINT)((ET9MAXUDBWORDSIZE * ET9SYMBOLWIDTH) + UDB_RECORD_HEADER_SIZE)))) ||
            ((nRecordType == ET9RDBTYPE) && (nRecordLength > ((ET9UINT)((ET9MAXUDBWORDSIZE * ET9SYMBOLWIDTH) + RDB_RECORD_HEADER_SIZE)))) ||
            ((nRecordType == ET9FREETYPE) && (nRecordLength > ET9RUDBDataAreaBytes(pRUDB)))) {
            return (pRUDB->wDataCheck - 1);
        }

        /* only length/type field goes toward the checksum for free records */

        nIndex = (nRecordType == ET9FREETYPE) ? ((nRecordLength > 1) ? 2 : 1) : nRecordLength;

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

#ifdef ET9_DEBUG
    __VerifySingleBytes(pRUDB);
#endif

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

static void ET9LOCALCALL __ET9AWSetRUDBRangeInfo(ET9AWLingInfo     *pLingInfo,
                                                 ET9U16             wLength,
                                                 ET9U8              bIncludeCompletions,
                                                 ET9U16            *wRegionIndex,
                                                 ET9U8 ET9FARDATA **ppbStart,
                                                 ET9U8 ET9FARDATA **ppbEnd)
{
    ET9AWRUDBInfo ET9FARDATA * const pRUDB = pLingInfo->pLingCmnInfo->pRUDBInfo;
    ET9U16 wRegion;

    ET9Assert(pLingInfo);
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
/** \internal
 * Called whenever a specific word in the RUDB is accessed for use.
 * Tries to bump the frequency of the word entry's header, if not already maxed out.
 *
 * @param pLingInfo      Pointer to field information structure.
 * @param pbEntry        Pointer to RUDB record in database.
 * @param wFixedFreq     0 if standard freq increment logic; non-zero if setting specific freq.
 *
 * @return None (record freq updated if possible).
 */

static void ET9LOCALCALL __ET9AWUpdateRUDBUsageCount(ET9AWLingInfo    *pLingInfo,
                                                     ET9U8 ET9FARDATA *pbEntry,
                                                     ET9U16            wFixedFreq)
{
    ET9AWLingCmnInfo * const pLingCmnInfo = pLingInfo->pLingCmnInfo;
    ET9AWRUDBInfo ET9FARDATA * const pRUDB = pLingCmnInfo->pRUDBInfo;

    ET9U16   wTemp, wOldFreq, wNewFreq;
    const ET9U8 ET9FARDATA *pbByte;

    ET9Assert(pLingInfo);
    ET9Assert(pbEntry);
    ET9Assert(pRUDB != NULL);

    /* point to the 2nd byte of the record */

    ++pbEntry;
    __CheckForWrap_ByteP(pbEntry, pRUDB);

    /* read the record frequency value */
    /* Read Word */

    pbByte = pbEntry + 1;
    __CheckForWrap_ByteP(pbByte, pRUDB);
    wOldFreq = ET9MAKEWORD(*pbEntry, *pbByte);

    if (wFixedFreq) {

        wNewFreq = wFixedFreq;

        /* update the record with the new frequency */

        __ET9AWRUDBWriteWord(pLingInfo, pbEntry, wNewFreq);

        /* Also add frequency change into RUDB checksum */

        wTemp = (ET9U16)(pLingCmnInfo->pRUDBInfo->wDataCheck - ET9HIBYTE(wOldFreq) - ET9LOBYTE(wOldFreq) + ET9HIBYTE(wNewFreq) + ET9LOBYTE(wNewFreq));

        __ET9AWWriteRUDBData(pLingInfo, (void ET9FARDATA *)&pLingCmnInfo->pRUDBInfo->wDataCheck, (const void ET9FARDATA *)&wTemp, sizeof(wTemp) );
    }

    /* Bump the usage count, if it's not reached the limit */

    else if (wOldFreq < ET9MAX_FREQ_COUNT) {

        /* if the bump exceeds the max */

        if ((wOldFreq + ET9FREQ_BUMP_COUNT) > ET9MAX_FREQ_COUNT) {

            /* just set it to the max */

            wNewFreq = ET9MAX_FREQ_COUNT;
        }
        else {
            wNewFreq = (ET9U16)(wOldFreq + ET9FREQ_BUMP_COUNT);
        }

        /* update the record with the new frequency */

        __ET9AWRUDBWriteWord(pLingInfo, pbEntry, wNewFreq);

        /* Also add frequency change into RUDB checksum */

        wTemp = (ET9U16)(pLingCmnInfo->pRUDBInfo->wDataCheck - ET9HIBYTE(wOldFreq) - ET9LOBYTE(wOldFreq) + ET9HIBYTE(wNewFreq) + ET9LOBYTE(wNewFreq));

        __ET9AWWriteRUDBData(pLingInfo, (void ET9FARDATA *)&pLingCmnInfo->pRUDBInfo->wDataCheck, (const void ET9FARDATA *)&wTemp, sizeof(wTemp) );
    }

    /* else if QUDB entry, make it a standard UDB entry (with bump, since it's been used) */

    else if (wOldFreq > ET9MAX_FREQ_COUNT) {

        wNewFreq = (ET9U16)(ET9ALP_WORD_INIT_FREQ + ET9FREQ_BUMP_COUNT);

        /* update the record with the new frequency */

        __ET9AWRUDBWriteWord(pLingInfo, pbEntry, wNewFreq);

        /* Also add frequency change into RUDB checksum */

        wTemp = (ET9U16)(pLingCmnInfo->pRUDBInfo->wDataCheck - ET9HIBYTE(wOldFreq) - ET9LOBYTE(wOldFreq) + ET9HIBYTE(wNewFreq) + ET9LOBYTE(wNewFreq));

        __ET9AWWriteRUDBData(pLingInfo, (void ET9FARDATA *)&pLingCmnInfo->pRUDBInfo->wDataCheck, (const void ET9FARDATA *)&wTemp, sizeof(wTemp) );
    }
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Retrieves the frequency of given UDB record.
 *
 * @param pRUDB          Pointer to the RUDB.
 * @param pbCurrent      Pointer to UDB record in database.
 *
 * @return Frequency value.
 */

ET9U16 ET9FARCALL _ET9AWGetUDBFrequency(ET9AWRUDBInfo ET9FARDATA *pRUDB,
                                        ET9U8         ET9FARDATA *pbCurrent)
{
    ET9U8 ET9FARDATA *pbByte;

    ET9Assert(pRUDB);
    ET9Assert(pbCurrent);

    /* point to the 2nd byte of the record */

    ++pbCurrent;
    __CheckForWrap_ByteP(pbCurrent, pRUDB);

    /* read the record frequency value */

    pbByte = pbCurrent + 1;
    __CheckForWrap_ByteP(pbByte, pRUDB);

    return ET9MAKEWORD(*pbCurrent, *pbByte);
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Look up the word in the RDB and update the freq with the value passed
 * UNLESS the existing freq is already higher.
 *
 * @param pLingInfo      Pointer to alpha info struct.
 * @param pWord          Pointer to struct containing word being searched for.
 * @param wWordFreq      Initial frequency to assign to RDB entry.
 *
 * @return Word size of word if found; 0 if not found.
 */

static ET9U16 ET9LOCALCALL __ET9RDBSetHigherFreq(ET9AWLingInfo     *pLingInfo,
                                              const ET9AWPrivWordInfo *pWord,
                                                    ET9U16             wWordFreq)
{
    ET9AWLingCmnInfo * const pLingCmnInfo = pLingInfo->pLingCmnInfo;
    ET9AWRUDBInfo ET9FARDATA * const pRUDB = pLingCmnInfo->pRUDBInfo;
    ET9U8 ET9FARDATA   *pbStart;
    ET9U8 ET9FARDATA   *pbEnd;
    ET9U16              wRegion;
    ET9UINT             nCurRecType = ET9FREETYPE;
    ET9SYMB ET9FARDATA *pSymb;
    ET9U16              wWordSize = 0;
    ET9U16              i;
    ET9SYMB             sSymb;
    const ET9U8 ET9FARDATA *pbByte;
    ET9U16              wFreq = 0;

    ET9Assert(pLingInfo);
    ET9Assert(pWord);

    /* if no RUDB, or none of the records requested exist */

    if (!pRUDB || !ET9RUDBENABLED(pLingCmnInfo) || !pRUDB->wRDBWordCount) {
        return 0;
    }

    /* get the beginning and end addresses of range record should be in (if there) */

    __ET9AWSetRUDBRangeInfo(pLingInfo, pWord->Base.wWordLen, 0, &wRegion, &pbStart, &pbEnd);

    /* Move past range marker */

    ++pbStart;
    __CheckForWrap_ByteP(pbStart, pRUDB);

    /* loop through the region(s) returned */

    while (pbStart != pbEnd) {

        nCurRecType = _ET9AWGetRecordType(pbStart);

        /* if it's not a free record (no use for free records in search) */

        if (nCurRecType == ET9RDBTYPE) {

            wWordSize = _ET9AWGetRDBWordLen(pbStart);
            wFreq  = (ET9U16)_ET9AWGetUDBFrequency(pRUDB, pbStart);

            /* move past the length, LDB and freq bytes */

            pSymb = (ET9SYMB ET9FARDATA *)(pbStart + RDB_RECORD_HEADER_SIZE);
            __CheckForWrap_WordP(pSymb, pRUDB);

            /* if the word size matches */

            if (wWordSize == pWord->Base.wWordLen) {

                /* loop through comparing symbols */

                for (i = 0; i < wWordSize; ++i) {

                    /* if mismatch, jump out of 'for' loop  */

                    sSymb = pWord->Base.sWord[i];

                    /* if symbol doesn't match, move on to the next record */
                    /* ReadSymbol */

                    pbByte = (const ET9U8 ET9FARDATA *)pSymb + 1;
                    __CheckForWrap_ByteP(pbByte, pRUDB);

                    if (sSymb != ET9MAKEWORD(*((const ET9U8 ET9FARDATA *)pSymb), *pbByte)) {
                        break;
                    }

                    /* point to the next symbol in the record entry */

                    ++pSymb;
                    __CheckForWrap_WordP(pSymb, pRUDB);
                }

                /* if looped through all symbols and all matched, break out of 'while' loop */

                if (i == wWordSize) {
                    break;
                }
            }
        }

        /* move to the next record */

        pbStart += _ET9AWGetRecordLength(pRUDB, pbStart);
        __CheckForWrap_ByteP(pbStart, pRUDB);
    }

    /* if all entries in region examined, return in failure */

    if (pbStart == pbEnd)  {
        wWordSize = 0;
    }
    else {

        /* if found, and the existing freq is lower than the passed freq */

        if (wFreq < wWordFreq) {
            __ET9AWUpdateRUDBUsageCount(pLingInfo, pbStart, wWordFreq);
        }
    }

    return wWordSize;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Look up the word in the UDB and update the freq with the value passed
 * UNLESS the existing freq is already higher.
 *
 * @param pLingInfo      Pointer to alpha info struct.
 * @param pWord          Pointer to struct containing word being searched for.
 * @param wWordLen       Length of the word being searched for.
 * @param wWordFreq      Initial frequency to assign to UDB entry.
 *
 * @return Word size of word if found; 0 if not found.
 */

static ET9U16 ET9LOCALCALL __ET9UDBSetHigherFreq(ET9AWLingInfo *pLingInfo,
                                                 ET9SYMB       *pWord,
                                                 ET9U16         wWordLen,
                                                 ET9U16         wWordFreq)
{
    ET9AWLingCmnInfo * const pLingCmnInfo = pLingInfo->pLingCmnInfo;
    ET9AWRUDBInfo ET9FARDATA * const pRUDB = pLingCmnInfo->pRUDBInfo;
    ET9U8 ET9FARDATA   *pbStart;
    ET9U8 ET9FARDATA   *pbEnd;
    ET9U16              wRegion;
    ET9UINT             nCurRecType = ET9FREETYPE;
    ET9SYMB ET9FARDATA *pSymb;
    ET9U16              wWordSize = 0;
    ET9U16              i;
    const ET9U8 ET9FARDATA *pbByte;
    ET9U16              wFreq = 0;

    ET9Assert(pLingInfo);
    ET9Assert(pWord);

    /* if no RUDB, or none of the records requested exist */

    if (!pRUDB || !ET9RUDBENABLED(pLingCmnInfo) || !pRUDB->wUDBWordCount) {
        return 0;
    }

    /* get the beginning and end addresses of range record should be in (if there) */

    __ET9AWSetRUDBRangeInfo(pLingInfo, wWordLen, 0, &wRegion, &pbStart, &pbEnd);

    /* Move past range marker */

    ++pbStart;
    __CheckForWrap_ByteP(pbStart, pRUDB);

    /* loop through the region(s) returned */

    while (pbStart != pbEnd) {

        nCurRecType = _ET9AWGetRecordType(pbStart);

        /* if it's a UDB record  */

        if (nCurRecType == ET9UDBTYPE) {

            wWordSize = _ET9AWGetUDBWordLen(pbStart);
            wFreq  = (ET9U16)_ET9AWGetUDBFrequency(pRUDB, pbStart);

            /* move past the length and freq bytes */

            pSymb = (ET9SYMB ET9FARDATA *)(pbStart + UDB_RECORD_HEADER_SIZE);

            __CheckForWrap_WordP(pSymb, pRUDB);

            /* if the word size matches */

            if (wWordSize == wWordLen) {

                /* loop through comparing symbols */

                for (i = 0; i < wWordSize; ++i) {

                    /* if mismatch, jump out of 'for' loop  */
                    /* if symbol doesn't match, move on to the next record */
                    /* ReadSymbol */

                    pbByte = (const ET9U8 ET9FARDATA *)pSymb + 1;
                    __CheckForWrap_ByteP(pbByte, pRUDB);

                    if (pWord[i] != ET9MAKEWORD(*((const ET9U8 ET9FARDATA *)pSymb), *pbByte)) {
                        break;
                    }

                    /* point to the next symbol in the record entry */

                    ++pSymb;
                    __CheckForWrap_WordP(pSymb, pRUDB);
                }

                /* if looped through all symbols and all matched, break out of 'while' loop */

                if (i == wWordSize) {
                    break;
                }
            }
        }

        /* move to the next record */

        pbStart += _ET9AWGetRecordLength(pRUDB, pbStart);
        __CheckForWrap_ByteP(pbStart, pRUDB);
    }

    /* if all entries in region examined, return in failure */

    if (pbStart == pbEnd)  {
        wWordSize = 0;
    }
    else {

        /*  if the existing freq is > ET9MAX_FREQ_COUNT (QUDB)                  */
        /* only want to use it if it has more life left than incoming QUDB word */

        if (wFreq > ET9MAX_FREQ_COUNT) {
            if (wWordFreq > ET9MAX_FREQ_COUNT && wWordFreq < wFreq) {
                wWordFreq = wFreq;
            }
        }

        /* existing word is UDB word, not QUDB... only want to use old freq if */
        /* incoming is QUDB word, or has lower freq than existing word         */

        else {
            if (wWordFreq > ET9MAX_FREQ_COUNT || wWordFreq < wFreq) {
                wWordFreq = wFreq;
            }
        }
        if (wFreq != wWordFreq) {
            __ET9AWUpdateRUDBUsageCount(pLingInfo, pbStart, wWordFreq);
        }
    }

    return wWordSize;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Search for the existance of a given word in the RUDB.
 * Record type:<BR>
 * ET9UDBTYPE if only searching for UDB match<BR>
 * ET9RDBTYPE if only searching for RDB match<BR>
 * 0 if matching on ANY record type
 *
 * @param pLingInfo      Pointer to alpha info struct.
 * @param pWord          Pointer to struct containing word being searched for.
 * @param nRecordType    Record type.
 * @param nIncrement     Indication of whether to bump the found word freq if found.
 *
 * @return Word size of word if found; 0 if not found.
 */

ET9UINT ET9FARCALL _ET9AWFindRUDBObject(ET9AWLingInfo     *pLingInfo,
                                  const ET9AWPrivWordInfo *pWord,
                                        ET9UINT            nRecordType,
                                        ET9UINT            nIncrement)
{
    ET9AWLingCmnInfo * const pLingCmnInfo = pLingInfo->pLingCmnInfo;
    ET9AWRUDBInfo ET9FARDATA * const pRUDB = pLingCmnInfo->pRUDBInfo;

    ET9U8 ET9FARDATA   *pbStart;
    ET9U8 ET9FARDATA   *pbEnd;
    ET9U16              wRegion;
    ET9UINT             nCurRecType = ET9FREETYPE;
    ET9SYMB ET9FARDATA *pSymb;
    ET9U16              wWordSize = 0;
    ET9U16              i;
    ET9SYMB             sSymb;
    const ET9U8 ET9FARDATA *pbByte;
    ET9U16              wFreq = 0;
    ET9U8               bLangID = 0;

    ET9Assert(pLingInfo);
    ET9Assert(pWord);

    /* if no RUDB, or wrong symbol class, or bad word length,
       or none of the records requested exist */

    if (!pRUDB || ! ET9RUDBENABLED(pLingCmnInfo) ||
        ((nRecordType == ET9RDBTYPE) && !pRUDB->wRDBWordCount) ||
        ((nRecordType == ET9UDBTYPE) && !pRUDB->wUDBWordCount) ||
        pWord->Base.wWordLen < 2 ||
        pWord->Base.wWordLen > ET9MAXWORDSIZE) {
        return 0;
    }

    ET9Assert(pLingCmnInfo->pRUDBInfo->wDataCheck == __ET9AWGetRUDBChecksum(pLingInfo));

    /* get the beginning and end addresses of range record should be in (if there) */

    __ET9AWSetRUDBRangeInfo(pLingInfo, pWord->Base.wWordLen, 0, &wRegion, &pbStart, &pbEnd);

    /* Move past range marker */

    ++pbStart;
    __CheckForWrap_ByteP(pbStart, pRUDB);

    /* loop through the region(s) returned */

    while (pbStart != pbEnd) {

        nCurRecType = _ET9AWGetRecordType(pbStart);

        /* if it's not a free record (no use for free records in search) */

        if (nCurRecType != ET9FREETYPE) {

            /* if it's a UDB record */

            if (nCurRecType == ET9UDBTYPE) {

                wWordSize = _ET9AWGetUDBWordLen(pbStart);
                wFreq  = (ET9U16)_ET9AWGetUDBFrequency(pRUDB, pbStart);

                /* move past the length and freq bytes */

                pSymb = (ET9SYMB ET9FARDATA *)(pbStart + UDB_RECORD_HEADER_SIZE);
            }
            else {

                /* it had _better_ be a RDB record */

                ET9Assert(nCurRecType == ET9RDBTYPE);
                wWordSize = _ET9AWGetRDBWordLen(pbStart);

                {
                    ET9U8 ET9FARDATA const *pbLangID = (const ET9U8 ET9FARDATA *)pbStart + 3;

                    __CheckForWrap_ByteP(pbLangID, pRUDB);

                    ET9Assert(pbLangID >= ET9RUDBData(pRUDB) && pbLangID < (ET9RUDBData(pRUDB) + ET9RUDBDataAreaBytes(pRUDB)));

                    bLangID = *pbLangID;
                }

                /* move past the length, LDB and freq bytes */

                pSymb = (ET9SYMB ET9FARDATA *)(pbStart + RDB_RECORD_HEADER_SIZE);
            }

            __CheckForWrap_WordP(pSymb, pRUDB);

            /* if the word size matches and the record type is OK */

            if ((wWordSize == pWord->Base.wWordLen) &&
                (!nRecordType || nRecordType == nCurRecType)) {

                /* loop through comparing symbols */

                for (i = 0; i < wWordSize; ++i) {

                    /* if mismatch, jump out of 'for' loop  */
                    /* do lowercase compare if RDB word     */

                    sSymb = pWord->Base.sWord[i];

                    /* if symbol doesn't match, move on to the next record */
                    /* ReadSymbol */

                    pbByte = (const ET9U8 ET9FARDATA *)pSymb + 1;
                    __CheckForWrap_ByteP(pbByte, pRUDB);

                    if (sSymb != ET9MAKEWORD(*((const ET9U8 ET9FARDATA *)pSymb), *pbByte)) {
                        break;
                    }

                    /* point to the next symbol in the record entry */

                    ++pSymb;
                    __CheckForWrap_WordP(pSymb, pRUDB);
                }

                /* if looped through all symbols and all matched, break out of 'while' loop */

                if (i == wWordSize) {

                    ET9U16 wLdbNum = 0;
                    ET9U16 wLdb2Num = 0;

                    switch (pWord->Base.bLangIndex) {
                    case ET9AWFIRST_LANGUAGE:
                        wLdbNum = pLingCmnInfo->wFirstLdbNum;
                        break;
                    case ET9AWSECOND_LANGUAGE:
                        wLdbNum = pLingCmnInfo->wSecondLdbNum;
                        break;
                    case ET9AWBOTH_LANGUAGES:
                        wLdbNum = pLingCmnInfo->wFirstLdbNum;
                        if (ET9AWSys_GetBilingualSupported(pLingInfo)) {
                            wLdb2Num = pLingCmnInfo->wSecondLdbNum;
                        }
                        break;
                    }

                    if ((nCurRecType == ET9UDBTYPE) ||
                        (nCurRecType == ET9RDBTYPE &&
                          ((bLangID == (wLdbNum & ET9PLIDMASK)) ||
                           (bLangID == (wLdb2Num & ET9PLIDMASK))))) {
                        break;
                    }
                }
            }
        }

        /* move to the next record */

        pbStart += _ET9AWGetRecordLength(pRUDB, pbStart);
        __CheckForWrap_ByteP(pbStart, pRUDB);
    }

    /* if all entries in region examined, return in failure */

    if (pbStart == pbEnd)  {
        wWordSize = 0;
    }
    else {

        /* if found, update the frequency */
        /* if QUDB entry, want to make sure it gets into UDB regardless */
        /* this is needed if QUDB entry found after stripping           */

        if (nIncrement || ((nCurRecType == ET9UDBTYPE) && (wFreq > ET9MAX_FREQ_COUNT))) {
            __ET9AWUpdateRUDBUsageCount(pLingInfo, pbStart, 0);
        }
    }

    ET9Assert(pLingCmnInfo->pRUDBInfo->wDataCheck == __ET9AWGetRUDBChecksum(pLingInfo));

    return wWordSize;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * (Attempts to) accumulate requested amount of free record space from given pointer.
 *
 * @param pLingInfo              Pointer to alpha info struct.
 * @param pbEnterSpot            Pointer to initial RUDB record position to search from.
 * @param nSizeRequired          Amount of memory (in bytes) required.
 * @param nSizeRangeIndex        0-relative index of RUDB range that new word belongs to.
 *
 * @return Size (in bytes) of free memory actually found (memory chunks.
 *           are consolidated into a contiguous segment in the range indicated)
 */

static ET9UINT ET9LOCALCALL __ET9AWRUDBGoRightForMemoryChunk(ET9AWLingInfo    *pLingInfo,
                                                             ET9U8 ET9FARDATA *pbEnterSpot,
                                                             ET9UINT           nSizeRequired,
                                                             ET9UINT           nSizeRangeIndex)
{
    ET9AWLingCmnInfo * const pLingCmnInfo = pLingInfo->pLingCmnInfo;
    ET9AWRUDBInfo ET9FARDATA * const pRUDB = pLingCmnInfo->pRUDBInfo;
    ET9U8     ET9FARDATA *pbDataPointer[NUMSIZERANGES];
    ET9U16    wDataLengths[NUMSIZERANGES];
    ET9U16    wSizeRangeMoveLengths[NUMSIZERANGES];
    ET9UINT   nSizeRecovered;
    ET9U8     ET9FARDATA *pbNextRange;
    ET9U8     ET9FARDATA *pbNext;
    ET9U8     ET9FARDATA *pbDest;
    ET9U8     ET9FARDATA *pbEnd;
    ET9U16    wCheckSum, wNewSizeOffset;
    ET9UINT   nNumDataChunks;
    ET9UINT   nRecordType;
    ET9UINT   nRecordLength;
    ET9UINT   nAmPointing;
    ET9UINT   nIndex;
    ET9UINT   nSizeStillNeeded;
    ET9UINT   nIsSizeRegionStart;
    ET9S32    snUncheckedSize;


    ET9Assert(pLingInfo);
    ET9Assert(pbEnterSpot);
    ET9Assert(pRUDB != NULL);

    pbEnd = 0;
    _ET9ByteSet((ET9U8*)wSizeRangeMoveLengths, NUMSIZERANGES * sizeof(ET9U16), 0xFF);
    wCheckSum = pRUDB->wDataCheck;

    /* store pointers to data until the recovered size is greater than the needed size.*/

    pbDest = 0;

    nSizeRangeIndex = (nSizeRangeIndex + 1) % NUMSIZERANGES;
    pbNextRange = _ET9AWMoveRUDBPtrRight(pRUDB, ET9RUDBData(pRUDB), pRUDB->wSizeOffset[nSizeRangeIndex]);
    pbNext = pbEnterSpot;
    nIsSizeRegionStart = 0;

    nNumDataChunks = 0;
    nAmPointing = 0;
    nSizeRecovered = 0;
    nSizeStillNeeded = nSizeRequired;

    snUncheckedSize = (ET9S32)ET9RUDBDataAreaBytes(pRUDB);

    while (snUncheckedSize > 0) {

        nRecordType   = _ET9AWGetRecordType(pbNext);
        nRecordLength = _ET9AWGetRecordLength(pRUDB, pbNext);
        snUncheckedSize -= (ET9S32)nRecordLength;

        /* monitoring the need for shifting of size region indices */
        /* if the record is the start of a region */

        if  (pbNextRange == pbNext) {
            nIsSizeRegionStart = 1;
            wSizeRangeMoveLengths[nSizeRangeIndex] = (ET9U16)nSizeStillNeeded;
            nSizeRangeIndex = (nSizeRangeIndex + 1) % NUMSIZERANGES;
            pbNextRange = _ET9AWMoveRUDBPtrRight(pRUDB, ET9RUDBData(pRUDB),
                                            pRUDB->wSizeOffset[nSizeRangeIndex]);
        }

        /* if data record or beginning of size region */

        if ((nRecordType != ET9FREETYPE) || nIsSizeRegionStart) {
            if (!nAmPointing) {

                /* need to limit the number of free chunks we're accumulating */

                if (nNumDataChunks == NUMSIZERANGES) {
                    pbEnd = pbNext;
                    ET9Assert(pLingCmnInfo->pRUDBInfo->wDataCheck == __ET9AWGetRUDBChecksum(pLingInfo));
                    break;
                }
                pbDataPointer[nNumDataChunks] = pbNext;
                ++nNumDataChunks;
                nAmPointing = 1;
            }
        }

        /* else it's a free record thats NOT a size region marker */

        else {
            if (nAmPointing) {

                /* Calculates the distance between the two pointers */

                if (pbNext >= pbDataPointer[nNumDataChunks - 1]) {
                    wDataLengths[nNumDataChunks - 1] =
                        (ET9U16)(pbNext - pbDataPointer[nNumDataChunks - 1]);
                }
                else {
                    wDataLengths[nNumDataChunks - 1] =
                        (ET9U16)(ET9RUDBDataAreaBytes(pRUDB) - (pbDataPointer[nNumDataChunks - 1] - pbNext));
                }
                nAmPointing = 0;
            }
            wCheckSum = (ET9U16)(wCheckSum - *pbNext);
            if (*pbNext != ET9ONEBYTEFREEMASK) {
                wCheckSum = (ET9U16)(wCheckSum - *(_ET9AWMoveRUDBPtrRight(pRUDB, pbNext, 1)));
            }
            nSizeRecovered += nRecordLength;
            if (nRecordLength >= nSizeStillNeeded) {
                pbEnd = pbNext + nSizeStillNeeded;
                __CheckForWrap_ByteP(pbEnd, pRUDB);
                break;
            }
            else {
                pbEnd = pbNext + nRecordLength;
                __CheckForWrap_ByteP(pbEnd, pRUDB);
            }
            nSizeStillNeeded = nSizeRequired - nSizeRecovered;
        }
        nIsSizeRegionStart = 0;
        pbNext += _ET9AWGetRecordLength(pRUDB, pbNext);
        __CheckForWrap_ByteP(pbNext, pRUDB);
    }

    if (!pbEnd) {
        return 0;
    }

    /* Consolidate data chunks */

    pbDest = pbEnd;

    if (nNumDataChunks) {
        ET9UINT  nToLap, nFromLap, nSizeLeft, nSize;
        const ET9U8 ET9FARDATA *pFrom;

        /* as added precaution, pUDBGetEntry should be cleared to prevent */
        /* traversal from pointing to a bogus location */

        pLingCmnInfo->Private.pUDBGetEntry = 0;

        for (nIndex = nNumDataChunks; nIndex > 0; --nIndex) {

            /* Move RUDB Ptr Left*/

            pbDest -= wDataLengths[nIndex - 1];
            if (pbDest  < ET9RUDBData(pRUDB)) {
                pbDest += ET9RUDBDataAreaBytes(pRUDB);
            }
            /* copy the string over */

            pFrom = pbDataPointer[nIndex - 1];
            nSize = wDataLengths[nIndex - 1];

            nToLap = (ET9UINT)(((pbDest + nSize) >= (ET9RUDBData(pRUDB) + ET9RUDBDataAreaBytes(pRUDB))) ? (pbDest + nSize - (ET9RUDBData(pRUDB) + ET9RUDBDataAreaBytes(pRUDB))) : 0);

            nFromLap = (ET9UINT)(((pFrom + nSize) >= (ET9RUDBData(pRUDB) + ET9RUDBDataAreaBytes(pRUDB))) ? (pFrom + nSize - (ET9RUDBData(pRUDB) + ET9RUDBDataAreaBytes(pRUDB))) : 0);

            nSizeLeft = nSize;

            if (nFromLap) {
                __ET9AWWriteRUDBData(pLingInfo, (void ET9FARDATA *)_ET9AWMoveRUDBPtrRight(pRUDB, pbDest, nSizeLeft - nFromLap),
                     (const void ET9FARDATA *)_ET9AWMoveRUDBPtrRight(pRUDB, pFrom, nSizeLeft - nFromLap), nFromLap);
                nSizeLeft -= nFromLap;
            }
            if (nToLap) { /* if there is a tolap, its always greater than fromlap */
                nToLap -= nFromLap;
                __ET9AWWriteRUDBData(pLingInfo, (void ET9FARDATA *)_ET9AWMoveRUDBPtrRight(pRUDB, pbDest, nSizeLeft - nToLap),
                     (const void ET9FARDATA *)_ET9AWMoveRUDBPtrRight(pRUDB, pFrom, nSizeLeft - nToLap), nToLap);
                nSizeLeft -= nToLap;
            }

            if (nSizeLeft) {
                __ET9AWWriteRUDBData(pLingInfo, (void ET9FARDATA *)pbDest, (const void ET9FARDATA *)pFrom, nSizeLeft);
            }
        }
    }

    /* Write new large free record info */

    wCheckSum = __ET9AWRUDBWriteHeader(pLingInfo, pbEnterSpot, ((nSizeRecovered < nSizeRequired) ? nSizeRecovered : nSizeRequired), ET9FREETYPE, wCheckSum);

    /* write leftover record if there is one. */

    pbDest = pbEnd;
    if (nSizeRecovered > nSizeRequired) {
         wCheckSum = __ET9AWRUDBWriteHeader(pLingInfo, pbDest, nSizeRecovered - nSizeRequired, ET9FREETYPE, wCheckSum);
    }

    /* reset affected Key offsets */

    for (nIndex = 0; nIndex < NUMSIZERANGES; ++nIndex) {
        if (wSizeRangeMoveLengths[nIndex] != 0xFFFF) {

            wCheckSum = (ET9U16)(wCheckSum - pRUDB->wSizeOffset[nIndex]);

            wNewSizeOffset =
                (ET9U16)(pRUDB->wSizeOffset[nIndex] + wSizeRangeMoveLengths[nIndex] -
                (ET9U16)((nSizeRecovered < nSizeRequired) ? nSizeRequired - nSizeRecovered : 0));

            wNewSizeOffset = (ET9U16)(wNewSizeOffset % ET9RUDBDataAreaBytes(pRUDB));

            __ET9AWWriteRUDBData(pLingInfo, (void ET9FARDATA *)&pRUDB->wSizeOffset[nIndex], (const void ET9FARDATA *)&wNewSizeOffset, sizeof(wNewSizeOffset));

            wCheckSum = (ET9U16)(wCheckSum + wNewSizeOffset);
        }
    }
    /* write new checksum */

    __ET9AWWriteRUDBData(pLingInfo, (void ET9FARDATA *)&pRUDB->wDataCheck, (const void ET9FARDATA *)&wCheckSum, sizeof(wCheckSum) );

    ET9Assert(pLingCmnInfo->pRUDBInfo->wDataCheck == __ET9AWGetRUDBChecksum(pLingInfo));

    return nSizeRecovered;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Loops until enough free RUDB memory has been consolidated.
 * Continually calls __ET9AWRUDBGoRightForMemoryChunk until total accumulated memory satisfies request (or unable to fulfill request).
 *
 * @param pLingInfo          Pointer to alpha info struct.
 * @param pbEnterSpot        Pointer to initial RUDB record position to search from.
 * @param nSizeRequired      Amount of memory (in bytes) required.
 * @param nSizeRangeIndex    0-relative index of RUDB range that new word belongs to.
 *
 * @return Size (in bytes) of free memory actually found (memory chunks.
 *           are consolidated into a contiguous segment in the range indicated)
 *           or 0 if unable to fulfill request
 */

static ET9UINT ET9LOCALCALL __ET9AWRUDBGoRightForMemory(ET9AWLingInfo    *pLingInfo,
                                                        ET9U8 ET9FARDATA *pbEnterSpot,
                                                        ET9UINT           nSizeRequired,
                                                        ET9UINT           nSizeRangeIndex)
{
    ET9UINT nSize = 0;
    ET9UINT nLastSizeRecovered = 0;

    /* Loop over __ET9AWRUDBGoRightForMemory, which can only consolidate up to NUMSIZERANGES free records. */

    while (nSize < nSizeRequired) {

        nSize = __ET9AWRUDBGoRightForMemoryChunk(pLingInfo, pbEnterSpot, nSizeRequired, nSizeRangeIndex);

        if (nSize <= nLastSizeRecovered) {
            nSize = 0;
            break;
        }

        nLastSizeRecovered = nSize;
    }

    return nSize;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Finds (or creates) position to add one RUDB record of given type and size.
 *
 * @param pLingInfo      Pointer to alpha info struct.
 * @param wWordLen       Length (in symbols) of word being added.
 * @param nRecordType    ET9UDBTYPE or ET9RDBTYPE.
 *
 * @return Pointer to RUDB position to write new record to (or NULL, if unable to find an available spot).
 */

static ET9U8 ET9LOCALCALL ET9FARDATA * __ET9AWRUDBFindAddPosition(ET9AWLingInfo *pLingInfo,
                                                                  ET9U16         wWordLen,
                                                                  ET9UINT        nRecordType)
{
    ET9AWLingCmnInfo * const pLingCmnInfo = pLingInfo->pLingCmnInfo;
    ET9AWRUDBInfo ET9FARDATA * const pRUDB = pLingCmnInfo->pRUDBInfo;

    ET9U8          ET9FARDATA *pbNext;
    ET9U8          ET9FARDATA *pbLast;
    ET9U8          ET9FARDATA *pbEnterSpot = 0;
    ET9U8          ET9FARDATA *pbStart;
    ET9U8          ET9FARDATA *pbEnd;
    ET9UINT        nMemoryNeeded, nMemoryFound, nRecordLength;
    ET9U16         wRegion;

    ET9Assert(pLingInfo);
    ET9Assert(pRUDB != NULL);
    ET9Assert(wWordLen);

    /* get the beginning and end addresses of range record should go in */

    __ET9AWSetRUDBRangeInfo(pLingInfo, wWordLen, 0, &wRegion, &pbStart, &pbEnd);

    /* Skip past region indicator record */

    pbLast = pbStart + 1;
    __CheckForWrap_ByteP(pbLast, pRUDB);

    if (nRecordType == ET9UDBTYPE) {

        /* string size + 1(length) + 2(freq) */

        nMemoryNeeded = (ET9UINT)(wWordLen * ET9SYMBOLWIDTH) + (ET9UINT)(UDB_RECORD_HEADER_SIZE);
    }
    else {

        /* this had _better_ be a RDB record */

        ET9Assert(nRecordType == ET9RDBTYPE);

        /* string size + 1(length) + 2(freq) + 1(ldbnum) */

        nMemoryNeeded = (ET9UINT)(wWordLen * ET9SYMBOLWIDTH) + (ET9UINT)(RDB_RECORD_HEADER_SIZE);
    }

    /* first look for an available free record */

    pbNext = 0;
    pbEnterSpot = 0;
    nMemoryFound = 0;
    while (pbLast != pbEnd) {

        nRecordLength = _ET9AWGetRecordLength(pRUDB, pbLast);

        /* if it's a free record */

        if (_ET9AWGetRecordType(pbLast) == ET9FREETYPE) {

            /* if it's big enough */

            if (nRecordLength >= nMemoryNeeded) {

                /* save location and get out of this loop */

                pbEnterSpot = pbLast;
                break;
            }

            /* else if this is the first free rec in region, save ptr to it */

            else if (!pbNext) {
                pbNext = pbLast;
            }
        }

        /* move on to next record */

        pbLast += _ET9AWGetRecordLength(pRUDB, pbLast);
        __CheckForWrap_ByteP(pbLast, pRUDB);
    }

    /* if we didn't find a free record big enough */

    if (!pbEnterSpot) {

        /* if we did find a free record in the range, start there */

        if (pbNext) {
            pbEnterSpot = pbNext;
        }

        /* otherwise, put record at end of range, and eat into next region */

        else {
            pbEnterSpot = pbEnd;
        }
    }
    nMemoryFound = __ET9AWRUDBGoRightForMemory(pLingInfo, pbEnterSpot, nMemoryNeeded, wRegion);
    ET9Assert(pLingCmnInfo->pRUDBInfo->wDataCheck == __ET9AWGetRUDBChecksum(pLingInfo));

    if (nMemoryFound < nMemoryNeeded) {
        pbEnterSpot = NULL;
    }

    return pbEnterSpot;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Used to reduce a given record's frequency as part of the 'aging' algorithm
 *
 * @param wOldFreq       Old (current) frequency.
 *
 * @return Adjusted 'new' frequency.
 */

static ET9U16 ET9LOCALCALL __ET9AWRUDBNewAgingFreq(ET9U16 wOldFreq)
{

    /* if frequency from a QUDB entry */

    if (wOldFreq > ET9MAX_FREQ_COUNT) {

        /* only change if above the purge threshold */

        if (wOldFreq > (ET9MAX_FREQ_COUNT + 1)) {
            --wOldFreq;
        }
    }
    else if (wOldFreq && (wOldFreq < ET9UDBAGINGFACTOR)) {
        wOldFreq = (ET9U16)(wOldFreq - 1);
    }
    else {
        wOldFreq = (ET9U16)(wOldFreq - (wOldFreq / ET9UDBAGINGFACTOR));
    }

    return wOldFreq;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Age all of the RUDB record frequencies.
 *
 * @param pLingInfo      Pointer to alpha info struct.
 *
 * @return None
 */

static void ET9LOCALCALL __ET9AWRUDBAgeCount(ET9AWLingInfo *pLingInfo)
{
    ET9AWLingCmnInfo * const pLingCmnInfo = pLingInfo->pLingCmnInfo;
    ET9AWRUDBInfo ET9FARDATA * const pRUDB = pLingCmnInfo->pRUDBInfo;

    ET9U8           ET9FARDATA *pbUdbData;
    ET9U8           ET9FARDATA *pbData;
    ET9U16      wNewFreq, wOldFreq, wSizeChecked, wCheckSum, wNewCutOffFreq;
    ET9UINT     nRecordType, nRecordLength;
    const ET9U8 ET9FARDATA *pbByte;

    ET9Assert(pLingInfo);
    ET9Assert(pRUDB != NULL);

    wCheckSum = pRUDB->wDataCheck;

    /* reduce the last cut-of freq also */

    wCheckSum           = (ET9U16)(wCheckSum - pRUDB->wLastDelCutOffFreq);
    wNewCutOffFreq      = __ET9AWRUDBNewAgingFreq(pRUDB->wLastDelCutOffFreq);
    wCheckSum           = (ET9U16)(wCheckSum + wNewCutOffFreq);

    __ET9AWWriteRUDBData(pLingInfo, (void ET9FARDATA *)&pRUDB->wLastDelCutOffFreq, (const void ET9FARDATA *)&wNewCutOffFreq, sizeof(wNewCutOffFreq));

    pbUdbData = ET9RUDBData(pRUDB) + pRUDB->wSizeOffset[0];
    wSizeChecked = 0;
    while (wSizeChecked < ET9RUDBDataAreaBytes(pRUDB)) { /* this is just a safety check to close the loop */

        nRecordType = _ET9AWGetRecordType(pbUdbData);
        nRecordLength = _ET9AWGetRecordLength(pRUDB, pbUdbData);

        if (nRecordType != ET9FREETYPE) {

            /* move to freq position */

            pbData = pbUdbData + 1;
            __CheckForWrap_ByteP(pbData, pRUDB);
            pbByte = pbData + 1;
            __CheckForWrap_ByteP(pbByte, pRUDB);
            wOldFreq = ET9MAKEWORD(*pbData, *pbByte);
            wNewFreq = __ET9AWRUDBNewAgingFreq(wOldFreq);
            wCheckSum = (ET9U16)(wCheckSum - ET9HIBYTE(wOldFreq) - ET9LOBYTE(wOldFreq) + ET9HIBYTE(wNewFreq) + ET9LOBYTE(wNewFreq));

            /* write the new freq */

            __ET9AWRUDBWriteWord(pLingInfo, pbData, wNewFreq);

        }

        wSizeChecked = (ET9U16)(wSizeChecked + nRecordLength);
        pbUdbData += nRecordLength;
        __CheckForWrap_ByteP(pbUdbData, pRUDB);
    }

    /* rewrite checksum */

    __ET9AWWriteRUDBData(pLingInfo, (void ET9FARDATA *)&pRUDB->wDataCheck, (const void ET9FARDATA *)&wCheckSum, sizeof(wCheckSum));
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Update ET9AWRUDBInfo::wUpdateCounter and do the aging if it's time for it.
 *
 * @param pLingInfo      Pointer to alpha info struct.
 * @param value          0 (if reset requested), non-zero = update value.
 *
 * @return None
 */

void ET9FARCALL _ET9AWRUDBUpdateCounter(ET9AWLingInfo *pLingInfo,
                                        ET9U8          value)
{
    ET9AWLingCmnInfo * const pLingCmnInfo = pLingInfo->pLingCmnInfo;
    ET9AWRUDBInfo ET9FARDATA * const pRUDB = pLingCmnInfo->pRUDBInfo;

    ET9U16 wTemp = 0;

    ET9Assert(pLingInfo);

    if (pRUDB == NULL) {
        return;
    }

    ET9Assert(pLingCmnInfo->pRUDBInfo->wDataCheck == __ET9AWGetRUDBChecksum(pLingInfo));

    if (value) {

         wTemp = (ET9U16)(pRUDB->wUpdateCounter + value);

         /* time for us to age? */

         if (wTemp >= ET9UDBMAXUPDATE) {

#ifndef EVAL_BUILD
             __ET9AWRUDBAgeCount(pLingInfo);
             wTemp = 0;  /* reset the update count */
#endif /*EVAL_BUILD*/

         }
    }
    __ET9AWWriteRUDBData(pLingInfo,
                        (void ET9FARDATA *)&pRUDB->wUpdateCounter,
                        (const void ET9FARDATA *)&wTemp, sizeof(wTemp));

    ET9Assert(pLingCmnInfo->pRUDBInfo->wDataCheck == __ET9AWGetRUDBChecksum(pLingInfo));
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Trim (i.e. remove) all RUDB add words whose frequency is <= to the target 'trim' frequency (for garbage collection).
 * Will automatically purge QUDB entries. Continues until GC memory retrieval requirement achieved.<P>
 * The trim type specifies record type to be trimmed (ET9UDBTYPE or ET9RDBTYPE),
 * or if ET9GETFREQ, gets the smallest UDB record freq found in RUDB (with no actual trimming performed).
 *
 * @param pLingInfo          Pointer to alpha info struct.
 * @param wFreqToTrim        Freq threshold for trim.
 * @param nRequiredSize      Required size in bytes.
 * @param nTrimType          Trim type.
 *
 * @return The next 'trim' frequency to use.
 */

static ET9U16 ET9LOCALCALL __ET9AWRUDBTrimWords(ET9AWLingInfo *pLingInfo,
                                                ET9U16         wFreqToTrim,
                                                ET9UINT        nRequiredSize,
                                                ET9UINT        nTrimType)
{
    ET9AWLingCmnInfo * const pLingCmnInfo = pLingInfo->pLingCmnInfo;
    ET9AWRUDBInfo ET9FARDATA * const pRUDB = pLingCmnInfo->pRUDBInfo;

    ET9U8        ET9FARDATA *pbNext;
    ET9U8        ET9FARDATA *pbUdbData;
    ET9U16       wSmallestFreq = ET9MAX_FREQ_COUNT; /* assuming this is the smallest */
    ET9UINT      nRecordLength, nFreq, nIndex, nRecordType, nNumRDBTrimmed, nNumUDBTrimmed;
    ET9U16       wCheckSum, wTemp, wRemainingMemory, wSizeChecked;

    ET9Assert(pLingInfo);
    ET9Assert(pRUDB != NULL);

    /*
     * Go though the whole rudb and delete any word that has the
     * freq equal to the freq_to_mark.  Exit as soon as enough
     * space was recovered.
     */

    wCheckSum = pRUDB->wDataCheck;
    wRemainingMemory = pRUDB->wRemainingMemory;
    pbNext = ET9RUDBData(pRUDB) + pRUDB->wSizeOffset[0];
    nNumRDBTrimmed = 0;
    nNumUDBTrimmed = 0;
    wSizeChecked = 0;

    while (wSizeChecked < ET9RUDBDataAreaBytes(pRUDB)) { /* this is just a safety check to close the loop */

        nRecordType = _ET9AWGetRecordType(pbNext);
        nRecordLength = _ET9AWGetRecordLength(pRUDB, pbNext);
        nFreq  = (ET9UINT)_ET9AWRUDBReadWord(pRUDB, _ET9AWMoveRUDBPtrRight(pRUDB, pbNext, (ET9UINT)1));

        if (nTrimType == ET9GETFREQ) {
            if (nRecordType == ET9UDBTYPE) {
                if (nFreq < wSmallestFreq) {
                    wSmallestFreq = (ET9U16)nFreq;
                }
            }
        }
        else if ((nRecordType == nTrimType) || ((nRecordType == ET9UDBTYPE) && (nFreq > ET9MAX_FREQ_COUNT))) {

            /* if QUDB record, just automatically purge it for GC */

            if ((nFreq > ET9MAX_FREQ_COUNT) || (nFreq <= wFreqToTrim)) {
                wRemainingMemory = (ET9U16)(wRemainingMemory + nRecordLength);
                pbUdbData = pbNext;
                for (nIndex = 0; nIndex < nRecordLength; ++nIndex) {
                    wCheckSum = (ET9U16)(wCheckSum - *pbUdbData);
                    ++pbUdbData;
                    __CheckForWrap_ByteP(pbUdbData, pRUDB);
                }

                wCheckSum = __ET9AWRUDBWriteHeader(pLingInfo, pbNext, nRecordLength, ET9FREETYPE, wCheckSum);

                if (nRecordType == ET9RDBTYPE) {
                    ++nNumRDBTrimmed;
                }
                else if (nRecordType == ET9UDBTYPE){
                    ++nNumUDBTrimmed;
                }
                else {
                    ET9Assert(0);
                }
                if (wRemainingMemory >= nRequiredSize) {
                    break;
                }
            }
            else if (nFreq < wSmallestFreq) {
                wSmallestFreq = (ET9U16) nFreq;
            }
        }
        wSizeChecked = (ET9U16)(wSizeChecked + nRecordLength);
        pbNext += nRecordLength;
        __CheckForWrap_ByteP(pbNext, pRUDB);
    }

    /* return here if just getting lowest UDB frequency */

    if (nTrimType == ET9GETFREQ) {
        return wSmallestFreq;
    }

    /* record the change in memory to the checksum */

    wCheckSum = (ET9U16)(wCheckSum - pRUDB->wRemainingMemory + wRemainingMemory);

    /* Write new remaining memory */

    __ET9AWWriteRUDBData(pLingInfo, (void ET9FARDATA *)&pRUDB->wRemainingMemory, (const void ET9FARDATA *)&wRemainingMemory, sizeof(wRemainingMemory) );

    /* write the new word counts */

    wCheckSum = (ET9U16)(wCheckSum - nNumRDBTrimmed);
    wCheckSum = (ET9U16)(wCheckSum - nNumUDBTrimmed);

    if (nNumRDBTrimmed) {
        wTemp = (ET9U16)(pRUDB->wRDBWordCount - nNumRDBTrimmed);
        ET9Assert(pRUDB->wRDBWordCount > wTemp);
        __ET9AWWriteRUDBData(pLingInfo, (void ET9FARDATA *)&pRUDB->wRDBWordCount,
                    (const void ET9FARDATA *)&wTemp, sizeof(wTemp) );
    }
    if (nNumUDBTrimmed) {
        wTemp = (ET9U16)(pRUDB->wUDBWordCount - nNumUDBTrimmed);
        ET9Assert(pRUDB->wUDBWordCount > wTemp);
        __ET9AWWriteRUDBData(pLingInfo, (void ET9FARDATA *)&pRUDB->wUDBWordCount,
                    (const void ET9FARDATA *)&wTemp, sizeof(wTemp) );
    }

    /* Write new checksum */

    __ET9AWWriteRUDBData(pLingInfo, (void ET9FARDATA *)&pRUDB->wDataCheck,
                 (const void ET9FARDATA *)&wCheckSum, sizeof(wCheckSum) );

    if (wRemainingMemory < nRequiredSize) {
        wFreqToTrim = wSmallestFreq; /* next deletion freq. to delete ( otherwise enough space,
                                         so keep the current deletion freq. */
    }

    return wFreqToTrim;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Perform garbage collection on RUDB to free up memory by removing least used entries.
 *
 * @param pLingInfo      Pointer to alpha info struct.
 * @param pRUDB          Pointer to RUDB.
 * @param nNewWordSize   Minimum size (in bytes) required to be recovered.
 *
 * @return None
 */

static void ET9LOCALCALL __ET9AWRUDBGarbageCollection(ET9AWLingInfo            *pLingInfo,
                                                      ET9AWRUDBInfo ET9FARDATA *pRUDB,
                                                      ET9UINT                   nNewWordSize)
{
    ET9AWLingCmnInfo * const pLingCmnInfo = pLingInfo->pLingCmnInfo;

    ET9UINT  nRequiredSize;
    ET9U16   wUdbFreqToTrim, wRdbFreqToTrim;
    ET9U16   wTemp, wCheckSum;

    ET9Assert(pLingInfo);
    ET9Assert(pRUDB);

    /* free up 1/8 of total size of the data area region in bytes. */

    nRequiredSize = (ET9RUDBDataAreaBytes(pRUDB)) / 8;

    /* Ensure we have enough for the new word -- hopefully the UDB isn't this small */

    if (nRequiredSize < nNewWordSize) {
        nRequiredSize = nNewWordSize;
    }

    /* required size must include one free byte per key range. */

    nRequiredSize += NUMSIZERANGES;

    wRdbFreqToTrim = pRUDB->wLastDelCutOffFreq;  /* first reorder word deletion freq. */

    wUdbFreqToTrim = __ET9AWRUDBTrimWords(pLingInfo, 0, 0, ET9GETFREQ);

    /*
     * For every udb add word word with X frequencies, we delete reorder word with X to Y freq.
     * where Y <= 2 * wUdbFreqToTrim
     */

    while ( pRUDB->wRemainingMemory < nRequiredSize) {

        /*
         * When there is nothing in the rdb or udb to delete or already completely deleted,
         * the wUdbFreqToTrim or wRdbFreqToTrim is equal to max.  We type cast to the 32-bit
         * integral here to ensure that the comparision is correctly performmed.  For instance,
         * if wUdbFreqToTrin = max, then we just keep trying to deleted words from the rdb, the
         * same is true if wRdbFreqToTrim is equal to max, which we keep trying to delete words
         * from the udb. If both of these value are equal to max and we have not claimming enough
         * space, then there must a logical error.
         */

        if ((ET9U32)wUdbFreqToTrim * 2 >= (ET9U32)wRdbFreqToTrim && pRUDB->wRDBWordCount) {
            wRdbFreqToTrim = __ET9AWRUDBTrimWords(pLingInfo, wRdbFreqToTrim, nRequiredSize, ET9RDBTYPE);
        }
        else {
            wUdbFreqToTrim = __ET9AWRUDBTrimWords(pLingInfo, wUdbFreqToTrim, nRequiredSize, ET9UDBTYPE);
        }

        ET9Assert(!((pRUDB->wRemainingMemory < nRequiredSize) &&
                    (wRdbFreqToTrim == ET9MAX_FREQ_COUNT) &&
                    (wUdbFreqToTrim == ET9MAX_FREQ_COUNT)));

    }

    /* Update last deletion threshold, but also make sure that it
     * will not become bigger than the non-first initial frequencies.
     */

    wCheckSum = pRUDB->wDataCheck;
    wCheckSum = (ET9U16) (wCheckSum - pRUDB->wLastDelCutOffFreq);
    wTemp = wRdbFreqToTrim;

    if (wTemp > ET9RD_WORD_INIT_FREQ) {
        wTemp = ET9RD_WORD_INIT_FREQ;
    }
    __ET9AWWriteRUDBData(pLingInfo, (void ET9FARDATA *)(&pRUDB->wLastDelCutOffFreq), (const void ET9FARDATA *)&wTemp, sizeof(wTemp) );

    wCheckSum = (ET9U16)(wCheckSum + wTemp);

    /* rewrite checksum */

    __ET9AWWriteRUDBData(pLingInfo, (void ET9FARDATA *)&pRUDB->wDataCheck, (const void ET9FARDATA *)&wCheckSum, sizeof(wCheckSum) );

    /* as added precaution, pUDBGetEntry should be cleared to prevent */
    /* traversal from pointing to a bogus location */

    pLingCmnInfo->Private.pUDBGetEntry = 0;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Adds a UDB word to the RUDB.
 *
 * @param pLingInfo      Pointer to alpha info struct.
 * @param psBuf          Pointer to UDB word being added.
 * @param wWordLen       Length (in symbols) of UDB word being added.
 * @param wInitFreq      Initial frequency to assign to word record.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL _ET9AWGeneralUDBAddWord(ET9AWLingInfo *pLingInfo,
                                             ET9SYMB       *psBuf,
                                             ET9U16         wWordLen,
                                             ET9U16         wInitFreq)
{
    ET9STATUS    wStatus;

    wStatus = _ET9AWSys_BasicValidityCheck(pLingInfo);

    if (wStatus) {
        return wStatus;
    }

    if (!psBuf) {
        return ET9STATUS_INVALID_MEMORY;
    }

    {
        ET9AWLingCmnInfo * const pLingCmnInfo = pLingInfo->pLingCmnInfo;

        ET9AWRUDBInfo ET9FARDATA *pRUDB;
        ET9U8         ET9FARDATA *pbUdbData;
        ET9U16       wChecksum, wTemp;
        ET9UINT      nRecordLen, nCount;
        ET9SYMB      sData;
        ET9U8        bSymByte;
        ET9AWPrivWordInfo holder;

        pRUDB = pLingCmnInfo->pRUDBInfo;

        if (pRUDB == NULL) {
            return ET9STATUS_NO_RUDB;
        }

        ET9Assert(pLingCmnInfo->pRUDBInfo->wDataCheck == __ET9AWGetRUDBChecksum(pLingInfo));

        if (wWordLen < 2) {
            return ET9STATUS_BAD_PARAM;
        }

        /* No white spaces allowed in the word. */

        if (_ET9FindSpacesAndUnknown(psBuf, wWordLen)) {
            return ET9STATUS_INVALID_TEXT;
        }

        /* if word is longer than bit encoding of length, shorten */

        if (wWordLen > ET9MAXUDBWORDSIZE) {
            wWordLen = ET9MAXUDBWORDSIZE;
        }

        _ET9SymCopy(holder.Base.sWord, psBuf, wWordLen);
        holder.Base.wWordLen = wWordLen;
        holder.Base.wWordCompLen = 0;
        holder.Base.wSubstitutionLen = 0;

        /* Age the RUDB. */

        _ET9AWRUDBUpdateCounter(pLingInfo, 1);

        if (_ET9AWFindRUDBObject(pLingInfo, &holder, ET9UDBTYPE, 1)) {
            return ET9STATUS_NONE;
        }

        /* if free memory is less than 5% of total memory, perform garbage cleanup */

        nRecordLen = UDB_WORDLEN_TO_RECLEN(wWordLen);

        if ((pRUDB->wRemainingMemory < (ET9RUDBDataAreaBytes(pRUDB) / 20)) ||
            (pRUDB->wRemainingMemory < nRecordLen)) {
            __ET9AWRUDBGarbageCollection(pLingInfo, pRUDB, nRecordLen);

            /* If still not enough room, exit with failure */

            if (pRUDB->wRemainingMemory < nRecordLen) {
                return (ET9STATUS_ERROR);
            }
        }

        if (pLingCmnInfo->pRUDBInfo->wDataCheck != __ET9AWGetRUDBChecksum(pLingInfo)) {
            ET9Assert(pLingCmnInfo->pRUDBInfo->wDataCheck == __ET9AWGetRUDBChecksum(pLingInfo));
        }
        pbUdbData = __ET9AWRUDBFindAddPosition(pLingInfo, wWordLen, (ET9UINT)ET9UDBTYPE);

        if (!pbUdbData) {
            return ET9STATUS_ERROR;
        }

        wChecksum = pRUDB->wDataCheck;

        /* remove the free record info from the checksum */

        wChecksum = (ET9U16)(wChecksum - *pbUdbData);
        wChecksum = (ET9U16)(wChecksum - *(_ET9AWMoveRUDBPtrRight(pRUDB, pbUdbData, 1)));

        /* write length/type data */

        wChecksum = __ET9AWRUDBWriteHeader(pLingInfo, pbUdbData, wWordLen, ET9UDBTYPE, wChecksum);
        ++pbUdbData;
        __CheckForWrap_ByteP(pbUdbData, pRUDB);

        /* now write the frequency */

        wChecksum = (ET9U16)(wChecksum + ET9HIBYTE(wInitFreq) + ET9LOBYTE(wInitFreq));
        __ET9AWRUDBWriteWord(pLingInfo, pbUdbData, wInitFreq);
        pbUdbData += 2;
        __CheckForWrap_ByteP(pbUdbData, pRUDB);

        /* Copy Udb add word symbols, and update checksum */

        nCount = 0;
        while (nCount < wWordLen) {
            sData = psBuf[nCount];
            bSymByte = (ET9U8)ET9HIBYTE(sData);
            wChecksum = (ET9U16)(wChecksum + bSymByte);
            __ET9AWWriteRUDBData(pLingInfo, (void ET9FARDATA *)pbUdbData,
                                 (const void ET9FARDATA *)&bSymByte, 1);
            ++pbUdbData;
            __CheckForWrap_ByteP(pbUdbData, pRUDB);
            bSymByte = (ET9U8)ET9LOBYTE(sData);
            wChecksum = (ET9U16)(wChecksum + bSymByte);
            __ET9AWWriteRUDBData(pLingInfo, (void ET9FARDATA *)pbUdbData,
                                 (const void ET9FARDATA *)&bSymByte, 1);
            ++pbUdbData;
            __CheckForWrap_ByteP(pbUdbData, pRUDB);
            ++nCount;
        }

        /* update reorder word count */

        /* Increment word count */

        wTemp = (ET9U16)(pRUDB->wUDBWordCount + 1);
        __ET9AWWriteRUDBData(pLingInfo, (void ET9FARDATA *)&pRUDB->wUDBWordCount, (const void ET9FARDATA *)&wTemp, sizeof(wTemp));

        /* modify remaining memory count */

        wTemp = (ET9U16)(pRUDB->wRemainingMemory - nRecordLen);

        __ET9AWWriteRUDBData(pLingInfo, (void ET9FARDATA *)&pRUDB->wRemainingMemory, (const void ET9FARDATA *)&wTemp, sizeof(wTemp));

        /* Store new udb checksum (plus 1 for incremented word count */

        wTemp = (ET9U16)(wChecksum + (ET9UINT)1 - nRecordLen);

        __ET9AWWriteRUDBData(pLingInfo, (void ET9FARDATA *)&pRUDB->wDataCheck, (const void ET9FARDATA *)&wTemp, sizeof(wTemp));

        ET9Assert(pLingCmnInfo->pRUDBInfo->wDataCheck == __ET9AWGetRUDBChecksum(pLingInfo));
    }

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/**
 * Adds a UDB word to the RUDB.
 *
 * @param pLingInfo      Pointer to alpha information structure.
 * @param psBuf          Pointer to word being added.
 * @param wWordLen       Length (in symbols) of word being added.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL ET9AWUDBAddWord(ET9AWLingInfo * const pLingInfo,
                                     ET9SYMB       * const psBuf,
                                     const ET9U16          wWordLen)
{
    return _ET9AWGeneralUDBAddWord(pLingInfo, psBuf, wWordLen, (ET9U16)ET9ALP_WORD_INIT_FREQ);
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Adds an RDB word to the RUDB.
 *
 * @param pLingInfo      Pointer to alpha information structure.
 * @param pSTWord        Struct containing word (and word length) being added.
 * @param wWordFreq      Initial frequency to assign to RDB entry.
 * @param bPrimLdbNum    Primary LDB number (associated with RDB entry).
 * @param bExact         Indicator (from LDBFind) of exact match of word in LDB.
 * @param bLowercase     Indicator (from LDBFind) of lowercase version of word in LDB.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL _ET9AWRDBAddWord(ET9AWLingInfo     *pLingInfo,
                                      ET9AWPrivWordInfo *pSTWord,
                                      ET9U16             wWordFreq,
                                      ET9U8              bPrimLdbNum,
                                      ET9U8              bExact,
                                      ET9U8              bLowercase)
{
    ET9AWLingCmnInfo * const pLingCmnInfo = pLingInfo->pLingCmnInfo;

    ET9AWRUDBInfo ET9FARDATA * const pRUDB = pLingCmnInfo->pRUDBInfo;
    ET9STATUS    wStatus = ET9STATUS_NONE;
    ET9U8         ET9FARDATA *pbRdbData;
    ET9U16       wChecksum, wTemp;
    ET9U8        bData;
    ET9UINT      nRecordLen, nCount;
    ET9SYMB      sData;
    ET9U8        bSymByte;

    ET9Assert(pLingInfo);
    ET9Assert(pSTWord);
    ET9Assert(pSTWord->Base.wWordLen > 1);

    if (!pRUDB) {
        return ET9STATUS_NO_RUDB;
    }

    ET9Assert(pLingCmnInfo->pRUDBInfo->wDataCheck == __ET9AWGetRUDBChecksum(pLingInfo));

    /* if word is longer than bit encoding of length, shorten */

    if (pSTWord->Base.wWordLen > (ET9U16)0x3F) {
        pSTWord->Base.wWordLen = (ET9U16)0x3F;
    }

    if (!bExact && bLowercase) {
        ET9U16 wLdbNum  = (pSTWord->Base.bLangIndex == ET9AWSECOND_LANGUAGE) ? pLingInfo->pLingCmnInfo->wSecondLdbNum : pLingInfo->pLingCmnInfo->wFirstLdbNum;

        for (nCount = 0; nCount < pSTWord->Base.wWordLen; ++nCount) {
            sData = pSTWord->Base.sWord[nCount];
            pSTWord->Base.sWord[nCount] = _ET9SymToLower(sData, wLdbNum);
        }
    }

    if (bPrimLdbNum == (pLingCmnInfo->wSecondLdbNum & ET9PLIDMASK)) {
        pSTWord->Base.bLangIndex = ET9AWSECOND_LANGUAGE;
    }
    else if (bPrimLdbNum == (pLingCmnInfo->wFirstLdbNum & ET9PLIDMASK)) {
        pSTWord->Base.bLangIndex = ET9AWFIRST_LANGUAGE;
    }

    if (!_ET9AWFindRUDBObject(pLingInfo, pSTWord, ET9RDBTYPE, 1)) {

        /* if free memory is less than 5% of total memory, perform garbage cleanup */

        nRecordLen = RDB_WORDLEN_TO_RECLEN(pSTWord->Base.wWordLen);

        if ((pRUDB->wRemainingMemory < (ET9RUDBDataAreaBytes(pRUDB) / 20)) ||
            (pRUDB->wRemainingMemory < nRecordLen)) {
            __ET9AWRUDBGarbageCollection(pLingInfo, pRUDB, nRecordLen);

            /* If still not enough room, exit with failure */

            if (pRUDB->wRemainingMemory < nRecordLen) {
                wStatus = ET9STATUS_ERROR;
            }
        }
        if (wStatus == ET9STATUS_NONE) {

            ET9Assert(pLingCmnInfo->pRUDBInfo->wDataCheck == __ET9AWGetRUDBChecksum(pLingInfo));
            pbRdbData = __ET9AWRUDBFindAddPosition(pLingInfo, pSTWord->Base.wWordLen, (ET9UINT)ET9RDBTYPE);

            if (!pbRdbData) {
                wStatus = ET9STATUS_ERROR;
            }
            else {
                wChecksum = pRUDB->wDataCheck;

                /* remove the free record info from the checksum */

                wChecksum = (ET9U16)(wChecksum - *pbRdbData);
                wChecksum = (ET9U16)(wChecksum - *(_ET9AWMoveRUDBPtrRight(pRUDB, pbRdbData, 1)));

                /* write length/type data */

                wChecksum = __ET9AWRUDBWriteHeader(pLingInfo, pbRdbData, pSTWord->Base.wWordLen, ET9RDBTYPE, wChecksum);
                ++pbRdbData;
                __CheckForWrap_ByteP(pbRdbData, pRUDB);

                wTemp = wWordFreq;

                /* now write the frequency */

                wChecksum = (ET9U16)(wChecksum + ET9HIBYTE(wTemp) + ET9LOBYTE(wTemp));

                __ET9AWRUDBWriteWord(pLingInfo, pbRdbData, wTemp);
                pbRdbData += 2;
                __CheckForWrap_ByteP(pbRdbData, pRUDB);

                /* write ldbnum */
                /* add languge id, strip off mask because we may have only one byte
                    for storing pldbid */

                bData = bPrimLdbNum;
                wChecksum = (ET9U16)(wChecksum + bData);
                __ET9AWWriteRUDBData(pLingInfo, (void ET9FARDATA *)pbRdbData,
                            (const void ET9FARDATA *)&bData, 1);
                ++pbRdbData;
                __CheckForWrap_ByteP(pbRdbData, pRUDB);

                /* Copy Rdb add word symbols, and update checksum */

                nCount = 0;
                while (nCount < pSTWord->Base.wWordLen) {

                    /* if uppercase version exists in LDB as lowercase, put in lowercase version */
                    /* otherwise put in 'as is' (may already be lowercased) */

                    sData = pSTWord->Base.sWord[nCount];
                    bSymByte = (ET9U8)ET9HIBYTE(sData);
                    wChecksum = (ET9U16)(wChecksum + bSymByte);
                    __ET9AWWriteRUDBData(pLingInfo, (void ET9FARDATA *)pbRdbData, (const void ET9FARDATA *)&bSymByte, 1);
                    ++pbRdbData;
                    __CheckForWrap_ByteP(pbRdbData, pRUDB);
                    bSymByte = (ET9U8)ET9LOBYTE(sData);
                    wChecksum = (ET9U16)(wChecksum + bSymByte);
                    __ET9AWWriteRUDBData(pLingInfo, (void ET9FARDATA *)pbRdbData, (const void ET9FARDATA *)&bSymByte, 1);
                    ++pbRdbData;
                    __CheckForWrap_ByteP(pbRdbData, pRUDB);
                    ++nCount;
                }

                /* update reorder word count */

                /* Increment word count */

                wTemp = (ET9U16)(pRUDB->wRDBWordCount + 1);

                __ET9AWWriteRUDBData(pLingInfo,
                        (void ET9FARDATA *)&pRUDB->wRDBWordCount,
                        (const void ET9FARDATA *)&wTemp, sizeof(wTemp));

                /* modify remaining memory count */

                wTemp = (ET9U16)(pRUDB->wRemainingMemory - nRecordLen);

                __ET9AWWriteRUDBData(pLingInfo,
                        (void ET9FARDATA *)&pRUDB->wRemainingMemory,
                        (const void ET9FARDATA *)&wTemp, sizeof(wTemp));

                /* Store new udb checksum (plus 1 for incrented word count */

                wTemp = (ET9U16)(wChecksum + (ET9UINT)1 - nRecordLen);

                __ET9AWWriteRUDBData(pLingInfo,
                        (void ET9FARDATA *)&pRUDB->wDataCheck,
                        (const void ET9FARDATA *)&wTemp, sizeof(wTemp));
            }
        }
    }
    else {
        wStatus = ET9STATUS_NONE;
    }

    ET9Assert(pLingCmnInfo->pRUDBInfo->wDataCheck == __ET9AWGetRUDBChecksum(pLingInfo));

    return wStatus;
}

/*---------------------------------------------------------------------------*/
/**
 * Determine if a given word exists as a UDB entry in RUDB.
 * Find a UDB word by passing a word string and the word length.
 *
 * @param pLingInfo      Pointer to alpha information structure.
 * @param psBuf          Pointer to word to search for.
 * @param wWordLen       Length of word pointed to by psBuf.
 *
 * @return ET9STATUS NONE if word is found; ET9STATUS_NO_MATCHING_WORDS if not.
 */

ET9STATUS ET9FARCALL ET9AWUDBFindWord(ET9AWLingInfo * const pLingInfo,
                                      ET9SYMB * const       psBuf,
                                      const ET9U16          wWordLen)
{
    ET9STATUS wStatus;
    ET9AWRUDBInfo ET9FARDATA   *pRUDB;
    ET9U8 ET9FARDATA *pbNext;
    ET9U8 ET9FARDATA *pbEnd;
    ET9U16  wSize;
    ET9U16  i;
    ET9U8 ET9FARDATA *pSymb;
    ET9UINT nRecordLen;
    ET9U16  wRegion;
    ET9SYMB *pTarget;
    const ET9U8 ET9FARDATA *pbByte;

    if ((wStatus = _ET9AWSys_BasicValidityCheck(pLingInfo)) == ET9STATUS_NONE) {
        if (psBuf == NULL) {
            wStatus = ET9STATUS_INVALID_MEMORY;
        }
        else if (pLingInfo->pLingCmnInfo->pRUDBInfo == NULL) {
            wStatus = ET9STATUS_NO_RUDB;
        }
        else if (wWordLen < 2 || wWordLen > ET9MAXUDBWORDSIZE) {
            wStatus = ET9STATUS_BAD_PARAM;
        }
        else {
            pRUDB = pLingInfo->pLingCmnInfo->pRUDBInfo;

            /* Get the address range for the region that should hold word (if it exists) */

            __ET9AWSetRUDBRangeInfo(pLingInfo, wWordLen, 0, &wRegion, &pbNext, &pbEnd);

            /* Skip past region indicator record */

            ++pbNext;
            __CheckForWrap_ByteP(pbNext, pRUDB);

            /* loop through the entries in that region */

            do {

                /* if this is a UDB record */

                if (ET9UDBTYPE == _ET9AWGetRecordType(pbNext)) {

                    /* check to see if it matches */

                    wSize = _ET9AWGetUDBWordLen(pbNext);
                    if (wSize == wWordLen) {

                        pSymb = pbNext + UDB_RECORD_HEADER_SIZE;
                        __CheckForWrap_ByteP(pSymb, pRUDB);
                        pTarget = psBuf;

                        for (i = wSize; i; --i) {

                            /* if no match, move
                            on */
                            /* ReadSymbol */
                            pbByte = pSymb + 1;
                            __CheckForWrap_ByteP(pbByte, pRUDB);
                            if (*pTarget++ != ET9MAKEWORD(*pSymb, *pbByte)) {
                                break;
                            }

                            pSymb += ET9SYMBOLWIDTH;
                            __CheckForWrap_ByteP(pSymb, pRUDB);
                        }

                        /* if all symbols match, mark as deleted */

                        if (!i) {
                            break;
                        }
                    }
                }

                /* move to the next record */

                nRecordLen = _ET9AWGetRecordLength(pRUDB, pbNext);
                pbNext += nRecordLen;
                __CheckForWrap_ByteP(pbNext, pRUDB);

            } while (pbNext != pbEnd);

            if (pbNext == pbEnd) {
             wStatus = ET9STATUS_NO_MATCHING_WORDS;
            }
        }
    }

    return wStatus;
}

/*---------------------------------------------------------------------------*/
/**
 * Parses passed buffer for possible additions to UDB.
 * Words will be added as full UDB entries, not QUDB.
 *
 * @param pLingInfo      Pointer to alpha information structure.
 * @param psBuf          Pointer to buffer to be parsed.
 * @param wBufLen        Length of buffer pointed to by psBuf (in symbols).
 * @param bCheckMDB      Whether the MDB shoud be checked or not.
 *
 * @return ET9STATUS_NONE if buffer scanned successfully;<BR>
 *           ET9STATUS_NO_INIT, ET9STATUS_INVALID_MEMORY, ET9STATUS_NO_RUDB if not
 */

ET9STATUS ET9FARCALL ET9AWScanBufForCustomWords(ET9AWLingInfo * const pLingInfo,
                                                ET9SYMB       * const psBuf,
                                                const ET9U16          wBufLen,
                                                const ET9BOOL         bCheckMDB)
{
    ET9STATUS wStatus;

    wStatus = _ET9AWSys_BasicValidityCheck(pLingInfo);

    if (wStatus) {
        return wStatus;
    }

    if (!psBuf) {
        return ET9STATUS_INVALID_MEMORY;
    }

    if (wBufLen < 1) {
        return ET9STATUS_BAD_PARAM;
    }

    if (!pLingInfo->pLingCmnInfo->pRUDBInfo) {
        return ET9STATUS_NO_RUDB;
    }

    {
        ET9SYMB *psScan;
        ET9U16 wScanLen;

        psScan = psBuf;
        wScanLen = wBufLen;

        for (; wScanLen;) {

            ET9U16 wWordLen;
            ET9SYMB psWord[ET9MAXWORDSIZE];

            wStatus = ET9AWScanBufForNextCustomWord(pLingInfo, &psScan, &wScanLen, psWord, ET9MAXWORDSIZE, &wWordLen, bCheckMDB);

            if (wStatus) {
                break;
            }

            if (wWordLen) {

                wStatus = ET9AWUDBAddWord(pLingInfo, psWord, wWordLen);

                if (wStatus) {
                    return wStatus;
                }
            }
        }
    }

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/**
 * Parse passed buffer for the next possible addition to UDB.
 * API will parse passed buffer until potential UDB add candidate is found.
 * If found, returns the word and length in the provided buffer.
 *
 * @param pLingInfo      Pointer to alpha information structure.
 * @param ppsBuf         Pointer to a pointer to the buffer to be parsed.
 * @param pwBufLen       Pointer to length of buffer pointed to by ppsBuf.
 * @param psWordBuf      Pointer to buffer to load candidate into.
 * @param wWordBufLen    Size (in symbols) of the buffer to store the word (should be at least ET9MAXWORDSIZE).
 * @param pwWordLen      Pointer to location to put the size of the found candidate.
 * @param bCheckMDB      Whether the MDB shoud be checked or not.
 *
 * @return ET9STATUS_NONE if buffer scanned successfully;<BR>
 *           ET9STATUS_NO_MATCHING_WORDS if passed buffer contains no potential UDB candidates otherwise return error code.<BR>
 *           If ET9STATUS_NONE (potential UDB candidate found), the code will:<BR>
 *               1) Update the pointer pointed to by ppsBuf at the point immediately after the found word<BR>
 *               2) Update the length pointed to by pwBufLen with the remaining length of unprocessed buffer<BR>
 *               3) Copy the found candidate to the buffer pointed to by psWord (buffer MUST be at least ET9MAXUDBWORDSIZE symbols long)<BR>
 *               4  Put the size of the candidate in the pwWordLen location<BR>
 *           On successful return, the integration code can call ET9AWUDBAddWord
 *            with the psWord candidate (and pwWordLen length) to actually add the
 *            word to the UDB
 */

ET9STATUS ET9FARCALL ET9AWScanBufForNextCustomWord(ET9AWLingInfo * const pLingInfo,
                                                   ET9SYMB      ** const ppsBuf,
                                                   ET9U16        * const pwBufLen,
                                                   ET9SYMB       * const psWordBuf,
                                                   const ET9U16          wWordBufLen,
                                                   ET9U16        * const pwWordLen,
                                                   const ET9BOOL         bCheckMDB)
{
    ET9STATUS           wStatus;
    ET9SYMB             *psBufCur = 0;
    ET9SYMB             *psBufEnd;
    ET9AWPrivWordInfo   holder;
    ET9U16              wBufLen;
    ET9UINT             nEnterPossible = 0;
    ET9UINT             nSkipCurWord = 0;
    ET9AWLingCmnInfo    *pLingCmnInfo;

    wStatus = _ET9AWSys_BasicValidityCheck(pLingInfo);

    if (wStatus) {
        return wStatus;
    }

    if (!ppsBuf || !pwBufLen || !psWordBuf || !pwWordLen) {
        return ET9STATUS_INVALID_MEMORY;
    }

    if (*pwBufLen < 1) {
        return ET9STATUS_BAD_PARAM;
    }

    if (!pLingInfo->pLingCmnInfo->pRUDBInfo) {
        return ET9STATUS_NO_RUDB;
    }

    if (wWordBufLen < ET9MAXWORDSIZE) {
        return ET9STATUS_BUFFER_TOO_SMALL;
    }

    if (!*ppsBuf) {
        return ET9STATUS_INVALID_MEMORY;
    }

    psBufCur = *ppsBuf;
    wBufLen = *pwBufLen;
    psBufEnd = &psBufCur[wBufLen - 1];
    pLingCmnInfo = pLingInfo->pLingCmnInfo;

    _InitPrivWordInfo(&holder);

    while (psBufCur <= psBufEnd) {

        ET9U8 bExact;
        ET9U8 bLowercase;

        ET9U8 bClass = ET9GetSymbolClass(*psBufCur);

        if (psBufCur == psBufEnd && !(bClass == ET9SYMWHITE || bClass == ET9SYMUNKN)) {

            if (!nSkipCurWord) {

                /* transfer sym  */

                holder.Base.sWord[holder.Base.wWordLen++] = *psBufCur;
            }

            /* fake delimiter */

            bClass = ET9SYMWHITE;
        }

        /* if sym is end of useful word info */

        if (bClass == ET9SYMWHITE || bClass == ET9SYMUNKN) {

            if (!nSkipCurWord && holder.Base.wWordLen > 1 && holder.Base.wWordLen <= ET9MAXWORDSIZE) {

                nEnterPossible = 1;

                /* UDB Processing */

                /* if word is in ldb, don't need to do any more udb processing */

                if (_ET9AWLdbFind(pLingInfo, pLingCmnInfo->wFirstLdbNum, &holder, &bExact, &bLowercase, 0) == ET9STATUS_WORD_EXISTS) {

                     /* CR 23770 - if word exists at all in LDB, don't consider for UDB */

                     nEnterPossible = 0;
                }
                else if (ET9AWSys_GetBilingualSupported(pLingInfo) &&
                        _ET9AWLdbFind(pLingInfo, pLingCmnInfo->wSecondLdbNum, &holder, &bExact, &bLowercase, 0) == ET9STATUS_WORD_EXISTS) {

                     /* CR 23770 - if word exists at all in LDB, don't consider for UDB */

                     nEnterPossible = 0;
                }
                else if (bCheckMDB && _ET9AWMdbFind(pLingInfo, &holder) == ET9STATUS_WORD_EXISTS) {
                    nEnterPossible = 0;
                }
                else if (_ET9AWFindRUDBObject(pLingInfo, &holder, 0, 0)) {
                    nEnterPossible = 0;
                }

                /* if still haven't found word, strip taps and check */

                else {

                    if (_ET9AWSelLstStripActualTaps(&holder) != ET9STATUS_NO_OPERATION) {

                        if (!holder.Base.wWordLen) {
                            nEnterPossible = 0;
                        }
                        else if (_ET9AWLdbFind(pLingInfo, pLingCmnInfo->wFirstLdbNum, &holder, &bExact, &bLowercase, 0) == ET9STATUS_WORD_EXISTS) {
                            nEnterPossible = 0;
                        }
                        else if (ET9AWSys_GetBilingualSupported(pLingInfo) &&
                                 _ET9AWLdbFind(pLingInfo, pLingCmnInfo->wSecondLdbNum, &holder, &bExact, &bLowercase, 0) == ET9STATUS_WORD_EXISTS) {
                            nEnterPossible = 0;
                        }
                        else if (bCheckMDB && _ET9AWMdbFind(pLingInfo, &holder) == ET9STATUS_WORD_EXISTS) {
                            nEnterPossible = 0;
                        }
                        else if (_ET9AWFindRUDBObject(pLingInfo, &holder, 0, 0)) {
                            nEnterPossible = 0;
                        }
                    }
                }

                /* if still haven't found word, add it to the udb */

                if (nEnterPossible) {

                    /* Enter word if still possible */

                    /* Check ASDB to see if word matches an existing shortcut. If so, use  */
                    /* the ASDB version. If not in the ASDB, check the LdbAS               */

                    if (!_ET9AWFindASDBObject(pLingInfo, holder.Base.sWord, holder.Base.wWordLen, 0, 1)) {

                        if (!_ET9AWFindLdbASObject(pLingInfo, pLingCmnInfo->wFirstLdbNum, holder.Base.sWord, holder.Base.wWordLen, 0, 1)) {

                            if (ET9AWSys_GetBilingualSupported(pLingInfo)) {
                                _ET9AWFindLdbASObject(pLingInfo, pLingCmnInfo->wSecondLdbNum,holder.Base.sWord, holder.Base.wWordLen, 0, 1);
                            }
                        }
                    }

                    _ET9SymCopy(psWordBuf, holder.Base.sWord, holder.Base.wWordLen);

                    *pwWordLen = holder.Base.wWordLen;
                    *ppsBuf = psBufCur;

                    if (psBufCur > psBufEnd) {
                        *pwBufLen = 0;
                    }
                    else {
                        *pwBufLen = (ET9U16)(psBufEnd - psBufCur + 1);
                    }
                    break;
                }
            }

            /* step past delimiting sym */

            ++psBufCur;
            holder.Base.wWordLen = 0;
            nSkipCurWord = 0;
        }
        else {

            /* if current word too long, just bypass it */

            if (holder.Base.wWordLen >= ET9MAXWORDSIZE) {
                nSkipCurWord = 1;
                ++psBufCur;
            }
            else {

                /* transfer sym and move to the next sym */

                holder.Base.sWord[holder.Base.wWordLen++] = *psBufCur++;
            }
        }
    }

    if (!nEnterPossible) {
        *pwBufLen = 0;
        return ET9STATUS_NO_MATCHING_WORDS;
    }

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/**
 * Parse passed buffer for the next possible mispelled word.
 * API will parse passed buffer until a possibly mispelled word is found.
 * If one is found, the code will return possible corrections in the provided buffers
 *
 * @param pLingInfo      Pointer to alpha information structure.
 * @param psBegBuf       Pointer to beginning of buffer.
 * @param psBuf          Pointer to the current location in buffer to be parsed.
 * @param wBufLen        Length (in symbols) of buffer pointed to by psBuf.
 * @param pbTotalWords   Pointer to total words found.
 * @param ppsWord        Pointer to a pointer location to return where misspelled word occurs in original buffer.
 * @param ppsString      (OPTIONAL) pointer to a pointer location to return the beginning of the string containing the misspelled word (may be identical to *ppsWord).
 * @param pwStringLen    (OPTIONAL) pointer to a location to return the length of the string pointed to by *ppsString.
 *
 * @return ET9STATUS_NONE if potential mispelled word found;<BR>
 *           ET9STATUS_NO_MATCHING_WORDS if passed buffer contains no more potentially mispelled words otherwise return error code.<BR>
 *           If ET9STATUS_NONE (potentially mispelled word found), the integration layer
 *             can examine bTotalWords to determine the results of finding possible spell
 *             corrections for a word deemed to be misspelled.
 *             The returned selection list will always contain the misspelled word at
 *             index 0. If bTotalWords == 1, it means NO potential alternatives were found
 *             for the mispelled word; otherwise, the number of alternatives found will == (bTotalWords - 1),
 *             and the alternatives can be retrieved by calling ET9AWSelLstGetWord
 *             for indices 1 through (bTotalWords - 1)
 */

ET9STATUS ET9FARCALL ET9AWScanBufForNextSpellCorrection(ET9AWLingInfo        * const pLingInfo,
                                                        ET9SYMB ET9FARDATA   * const psBegBuf,
                                                        ET9SYMB ET9FARDATA   * const psBuf,
                                                        ET9U16                 const wBufLen,
                                                        ET9U8                * const pbTotalWords,
                                                        ET9SYMB * ET9FARDATA * const ppsWord,
                                                        ET9SYMB * ET9FARDATA * const ppsString,
                                                        ET9U16  *              const pwStringLen)
{
    ET9STATUS      wStatus;
    ET9SYMB ET9FARDATA  *psBufCur;
    ET9SYMB ET9FARDATA  *psBufEnd;
    ET9SYMB ET9FARDATA  *psBufTemp;
    ET9U8          bDefaultListIndex = ET9_NO_ACTIVE_INDEX;
    ET9SYMB ET9FARDATA *psBegWord = 0;
    ET9SYMB        sCurSymb;
    ET9U8          bClass;
    ET9U16         wCurWordLen = 0;
    ET9S16         index = 0;
    ET9U8          bFlipExact = 0;
    ET9U8          bFlipExactLast = 0;
    ET9U8          bFlipExactDefault = 0;
    ET9U8          bFlipCompletion = 0;
    ET9U8          bFlipAutoAppend = 0;
    ET9U8          bFlipInactiveSC = 0;
    ET9U16         wSavedMaxSlstCount = 0;
    ET9ASPCSEARCHFILTER eSavedSearchFilter;
    ET9AWPrivWordInfo *pPWI;
    ET9ASPCMODE    eSavedSPCMode;
    ET9WordSymbInfo *pWordSymbInfo;
    ET9SYMB       *psNewBegWord = psBegWord;
    ET9UINT        nNumDigt = 0;
    ET9UINT        nNumPunct = 0;
    ET9U16         wNewWordLen = wCurWordLen;
    ET9U16         i;
    ET9U16         j;
    ET9SYMB        sSymbol;
    ET9BOOL        bIsStripped = 0;
    ET9INPUTSHIFTSTATE eShifter;
    ET9U16          wLdbNum;

    if ((wStatus = _ET9AWSys_BasicValidityCheck(pLingInfo)) != ET9STATUS_NONE) {
        return wStatus;
    }
    if (!psBuf || !wBufLen || !pbTotalWords || !ppsWord || !psBegBuf) {
        return ET9STATUS_INVALID_MEMORY;
    }

    if (ppsString != NULL) {
        *ppsString = NULL;
    }
    if (pwStringLen != NULL) {
        *pwStringLen = 0;
    }

    wLdbNum = pLingInfo->pLingCmnInfo->wLdbNum;
    *pbTotalWords = 0;
    pWordSymbInfo = pLingInfo->pLingCmnInfo->Base.pWordSymbInfo;

    psBufCur = psBuf;
    psBufEnd = &psBufCur[wBufLen - 1];
    ET9ClearAllSymbs(pWordSymbInfo);

    if (!ET9EXACTINLIST(pLingInfo->pLingCmnInfo)) {
        pLingInfo->pLingCmnInfo->Private.bStateExactInList = 1;
        bFlipExact = 1;
    }
    if (ET9EXACTLAST(pLingInfo->pLingCmnInfo)) {
        pLingInfo->pLingCmnInfo->Private.bStateExactLast = 0;
        bFlipExactLast = 1;
    }
    if (ET9EXACTISDEFAULT(pLingInfo->pLingCmnInfo)) {
        pLingInfo->pLingCmnInfo->Private.bStateExactIsDefault = 0;
        bFlipExactDefault = 1;
    }
    if (ET9WORDCOMPLETION_MODE(pLingInfo->pLingCmnInfo)) {
        pLingInfo->pLingCmnInfo->Private.bStateWordCompletion = 0;
        bFlipCompletion = 1;
    }
    if (ET9AUTOAPPENDINLIST(pLingInfo->pLingCmnInfo)) {
        pLingInfo->pLingCmnInfo->Private.bStateAutoAppendInList = 0;
        bFlipAutoAppend = 1;
    }
    if (ET9AWSys_GetBilingualSupported(pLingInfo)) {
        if (!ET9INACTIVELANGSPELLCORRECTENABLED(pLingInfo->pLingCmnInfo)) {
            pLingInfo->pLingCmnInfo->Private.bStateInactiveLangSpellCorrect = 1;
            bFlipInactiveSC = 1;
        }
    }

    eSavedSPCMode = pLingInfo->pLingCmnInfo->Private.ASpc.eMode;
    pLingInfo->pLingCmnInfo->Private.ASpc.eMode = ET9ASPCMODE_EXACT;
    wSavedMaxSlstCount = pLingInfo->pLingCmnInfo->Private.ASpc.wMaxSpcTermCount;
    pLingInfo->pLingCmnInfo->Private.ASpc.wMaxSpcTermCount = pLingInfo->pLingCmnInfo->Private.bListSize - 1;
    eSavedSearchFilter = pLingInfo->pLingCmnInfo->Private.ASpc.eSearchFilter;
    pLingInfo->pLingCmnInfo->Private.ASpc.eSearchFilter = ET9ASPCSEARCHFILTER_UNFILTERED;

    while (psBufCur <= psBufEnd) {

        sCurSymb = *psBufCur++;
        bClass = ET9GetSymbolClass(sCurSymb);

        /* if we're parsing an exploded sequence, just skip over punct between words */

        if (!wCurWordLen && bIsStripped && bClass == ET9SYMPUNCT) {
            ++index;
            continue;
        }
        if (!(bClass == ET9SYMWHITE || bClass == ET9SYMUNKN || (bIsStripped && bClass == ET9SYMPUNCT))) {
            if (!wCurWordLen) {
                psBegWord = psBufCur - 1;
            }
            wCurWordLen++;
            if (index == (wBufLen-1)) {
                bClass = ET9SYMWHITE;
            }
        }
        if ((bClass == ET9SYMWHITE || bClass == ET9SYMUNKN || (bIsStripped && bClass == ET9SYMPUNCT)) && wCurWordLen) {

            /* _ET9AWSelLstStripActualTaps */

            if (!bIsStripped) {
                psNewBegWord = psBegWord;
                nNumDigt = 0;
                nNumPunct = 0;
                wNewWordLen = wCurWordLen;
                i = wCurWordLen;
                while (i--) {
                    sSymbol = psBegWord[i];

                     /* Count punctuation and digits */

                    if (_ET9_IsNumeric(sSymbol)) {
                        ++nNumDigt;
                    }
                    else if (_ET9_IsPunctChar(sSymbol)) {
                        ++nNumPunct;
                    }
                }
                /* leading/terminal punct stripping */

                if (nNumPunct && (nNumDigt + nNumPunct < wCurWordLen - nNumDigt - nNumPunct)) {
                    bIsStripped = 1;
                    while (_ET9_IsPunctChar(*psNewBegWord) && wNewWordLen) {
                        --wNewWordLen;
                        ++psNewBegWord;
                        --nNumPunct;
                    }
                    while (wNewWordLen && _ET9_IsPunctChar(psNewBegWord[wNewWordLen - 1])) {
                        --wNewWordLen;
                        --nNumPunct;
                    }
                }
                else {
                    if (nNumDigt == wCurWordLen) {

                        /* tag as numeric string */

                        bIsStripped = 4;
                    }
                    else if ((nNumDigt + nNumPunct) == wCurWordLen) {

                        /* tag as non-alpha string */

                        bIsStripped = 3;
                    }
                    else {

                        /* tag as non-stripped string with some alpha */

                        bIsStripped = 2;
                    }
                }
            }
            else {
                wNewWordLen = wCurWordLen;
                psNewBegWord = psBegWord;
            }
            if (ppsString != NULL && pwStringLen != NULL) {

                psBufTemp = psBegWord;
                *pwStringLen = wCurWordLen - 1;
                bClass = ET9SYMALPHA;

                while ((psBufTemp > psBegBuf) && !(bClass == ET9SYMWHITE || bClass == ET9SYMUNKN)) {
                    --psBufTemp;
                    bClass = ET9GetSymbolClass(*psBufTemp);
                }

                if (bClass == ET9SYMWHITE || bClass == ET9SYMUNKN) {
                    ++psBufTemp;
                }

                *ppsString = psBufTemp;
                *pwStringLen = 0;
                bClass = ET9SYMALPHA;

                while (psBufTemp < psBufEnd && !(bClass == ET9SYMWHITE || bClass == ET9SYMUNKN)) {
                    (*pwStringLen)++;
                    ++psBufTemp;
                    bClass = ET9GetSymbolClass(*psBufTemp);
                }

                if (psBufTemp == psBufEnd && !(bClass == ET9SYMWHITE || bClass == ET9SYMUNKN)) {
                    (*pwStringLen)++;
                }
            }
            if (wNewWordLen && (wNewWordLen <= ET9MAXWORDSIZE)) {

                if (bIsStripped <= 2) {

                    /* if the segment is all numeric, skip over it */

                    if (!(bIsStripped && _ET9_IsNumericString(psNewBegWord, wNewWordLen))) {

                        for (i = 0; i < wNewWordLen; i++) {
                            if (_ET9SymIsUpper(psNewBegWord[i], pWordSymbInfo->Private.wLocale)) {
                                eShifter = ET9SHIFT;
                            }
                            else {
                                eShifter = ET9NOSHIFT;
                            }
                            wStatus = ET9AddExplicitSymb(pWordSymbInfo, psNewBegWord[i], eShifter, ET9_NO_ACTIVE_INDEX);

                            /* switch type to allow spellcheck for explicitly entered sym */

                            pWordSymbInfo->SymbsInfo[pWordSymbInfo->bNumSymbs - 1].eInputType = ET9MULTITAPKEY;
                            pLingInfo->pLingCmnInfo->Base.bSymbInvalidated[pWordSymbInfo->bNumSymbs - 1] = 0;
                        }
                        wStatus = _ET9ASpellCheckSelLstBuild(pLingInfo, wLdbNum, pbTotalWords, &bDefaultListIndex);

                        /* wStatus = ET9AWSelLstGetWord(pLingInfo, &pWord, bDefaultListIndex); */
                        /* get the exact word */

                        pPWI  = &pLingInfo->pLingCmnInfo->Private.pWordList[pLingInfo->pLingCmnInfo->Private.bWordList[0]];

                        /* if the source is EXACT, EXACTISH or QUDB, consider it 'misspelled' */

                        if ((GETRAWSRC(pPWI->bWordSrc) == ET9WORDSRC_QUDB) ||
                            (GETRAWSRC(pPWI->bWordSrc) <  ET9WORDSRC_CDB)) {

                            /* if there are spellcheck suggestions OR none, but no internal punct OR too MUCH punct */

                            if (*pbTotalWords > 1 || !nNumPunct || (bIsStripped == 2)) {

                                for (j = 1; j < *pbTotalWords; ++j) {

                                    /* if the word is NOT a spell correction, probably an alternate form of the word... NOT mispelled */

                                    if (pLingInfo->pLingCmnInfo->Private.pWordList[pLingInfo->pLingCmnInfo->Private.bWordList[j]].Base.bIsTerm &&
                                        !pLingInfo->pLingCmnInfo->Private.pWordList[pLingInfo->pLingCmnInfo->Private.bWordList[j]].Base.bIsSpellCorr &&
                                        GETRAWSRC(pLingInfo->pLingCmnInfo->Private.pWordList[pLingInfo->pLingCmnInfo->Private.bWordList[j]].bWordSrc) != ET9WORDSRC_QUDB) {
                                        break;
                                    }
                                }

                                /* if all words are spell corrections, flag as mispelled */

                                if (j == *pbTotalWords) {

                                    /* save misspelled word position and return */

                                    *ppsWord = psNewBegWord;
                                    break;
                                }
                                else {
                                    bIsStripped = 0;
                                }
                            }

                            /* otherwise parse segments of word */

                            else {
                                index = (ET9S16)((ET9S16)(psNewBegWord - psBuf) - 1);
                                psBufCur = psNewBegWord;
                                nNumPunct = 0;
                            }
                        }
                        else {
                            bIsStripped = 0;
                        }
                    }
                }
                else {
                    bIsStripped = 0;
                }
            }
            else {
                bIsStripped = 0;
            }
            wCurWordLen = 0;
            ET9ClearAllSymbs(pWordSymbInfo);
        }
        ++index;
    }

    /* reset original settings */

    if (bFlipExact) {
        pLingInfo->pLingCmnInfo->Private.bStateExactInList = 0;
    }
    if (bFlipExactLast) {
        pLingInfo->pLingCmnInfo->Private.bStateExactLast = 1;
    }
    if (bFlipExactDefault) {
        pLingInfo->pLingCmnInfo->Private.bStateExactIsDefault = 1;
    }
    if (bFlipCompletion) {
        pLingInfo->pLingCmnInfo->Private.bStateWordCompletion = 1;
    }
    if (bFlipAutoAppend) {
        pLingInfo->pLingCmnInfo->Private.bStateAutoAppendInList = 1;
    }
    if (bFlipInactiveSC) {
        pLingInfo->pLingCmnInfo->Private.bStateInactiveLangSpellCorrect = 0;
    }

    pLingInfo->pLingCmnInfo->Private.ASpc.eSearchFilter = eSavedSearchFilter;
    pLingInfo->pLingCmnInfo->Private.ASpc.wMaxSpcTermCount = wSavedMaxSlstCount;
    pLingInfo->pLingCmnInfo->Private.ASpc.eMode = eSavedSPCMode;

    /* if no other status, and the whole buffer's been processed, indicate that the processing is finished */

    if (!wStatus && (index == wBufLen)) {
        ET9ClearAllSymbs(pWordSymbInfo);
        wStatus = ET9STATUS_NO_MATCHING_WORDS;
    }

    return wStatus;
}

/*---------------------------------------------------------------------------*/
/**
 * Delete UDB word.
 * Deletes a given UDB word from the RUDB.
 *
 * @param pLingInfo      Pointer to alpha information structure.
 * @param psBuf          Pointer to word to match for deletion.
 * @param wWordLen       Length (in symbols) of word pointed to by psBuf.
 *
 * @return ET9STATUS_NONE if UDB word found and deleted;.
 *           ET9STATUS_NO_MATCHING_WORDS if no matching UDB word was found
 */

ET9STATUS ET9FARCALL ET9AWUDBDeleteWord(ET9AWLingInfo * const pLingInfo,
                                        ET9SYMB       * const psBuf,
                                        const ET9U16          wWordLen)
{
    ET9STATUS wStatus;
    ET9AWRUDBInfo ET9FARDATA   *pRUDB;
    ET9U8 ET9FARDATA *pbNext;
    ET9U8 ET9FARDATA *pbTemp;
    ET9U8 ET9FARDATA *pbEnd;
    ET9U16  wSize;
    ET9U16  i;
    ET9U8 ET9FARDATA *pSymb;
    ET9UINT nRecordLen;
    ET9U16  wCheckSum, wTemp;
    ET9U16  wRegion;
    ET9SYMB *pTarget;
    const ET9U8 ET9FARDATA *pbByte;

    if ((wStatus = _ET9AWSys_BasicValidityCheck(pLingInfo)) != ET9STATUS_NONE) {
        return wStatus;
    }
    if (psBuf == NULL) {
        wStatus = ET9STATUS_INVALID_MEMORY;
    }
    else if (wWordLen < 1 || wWordLen > ET9MAXUDBWORDSIZE) {
        wStatus = ET9STATUS_BAD_PARAM;
    }
    else if (pLingInfo->pLingCmnInfo->pRUDBInfo == NULL) {
        wStatus = ET9STATUS_NO_RUDB;
    }
    else if (!pLingInfo->pLingCmnInfo->pRUDBInfo->wUDBWordCount) {
        wStatus = ET9STATUS_NO_MATCHING_WORDS;
    }
    else {
        pRUDB = pLingInfo->pLingCmnInfo->pRUDBInfo;

        /* Age the RUDB. */

        _ET9AWRUDBUpdateCounter(pLingInfo, 1);

        /* Get the address range for the region that should hold word (if it exists) */

        __ET9AWSetRUDBRangeInfo(pLingInfo, wWordLen, 0, &wRegion, &pbNext, &pbEnd);

        /* Skip past region indicator record */

        ++pbNext;
        __CheckForWrap_ByteP(pbNext, pRUDB);

        /* loop through the entries in that region */

        do {
            /* if this is a UDB record */

            if (ET9UDBTYPE == _ET9AWGetRecordType(pbNext)) {

                /* check to see if it matches */

                wSize = _ET9AWGetUDBWordLen(pbNext);

                /* if it's the same length, continue comparison */

                if (wSize == wWordLen) {

                    /* move past the length and frequency bytes of record header */

                    pSymb = pbNext + UDB_RECORD_HEADER_SIZE;
                    __CheckForWrap_ByteP(pSymb, pRUDB);
                    pTarget = psBuf;

                    /* loop through comparing symbols */

                    for (i = wSize; i; --i) {

                        /* if no match, move on to the next record */
                        /* ReadSymbol */

                        pbByte = pSymb + 1;
                        __CheckForWrap_ByteP(pbByte, pRUDB);

                        if (*pTarget++ != ET9MAKEWORD(*pSymb, *pbByte)) {
                            break;
                        }

                        /* point to the next symbol */

                        pSymb += ET9SYMBOLWIDTH;
                        __CheckForWrap_ByteP(pSymb, pRUDB);
                    }

                    /* if all symbols match, mark as deleted */

                    if (!i) {

                        /* save the address of the record being deleted */

                        pbTemp = pbNext;

                        /* loop through decrementing the RUDB checksum by the bytes in the record */

                        nRecordLen = _ET9AWGetRecordLength(pRUDB, pbNext);
                        i = (ET9U16)nRecordLen;
                        wCheckSum = pRUDB->wDataCheck;
                        while (i--) {
                            wCheckSum = (ET9U16)(wCheckSum - *pbTemp);
                            ++pbTemp;
                            __CheckForWrap_ByteP(pbTemp, pRUDB);
                        }

                        /* add new free record header */

                        wCheckSum = __ET9AWRUDBWriteHeader(pLingInfo, pbNext, nRecordLen, ET9FREETYPE, wCheckSum);

                        /* Decrement the word count */

                        wTemp = (ET9U16)(pRUDB->wUDBWordCount - 1);

                        __ET9AWWriteRUDBData(pLingInfo,
                                        (void ET9FARDATA *)&pRUDB->wUDBWordCount,
                                        (const void ET9FARDATA *)&wTemp,
                                        sizeof(wTemp) );

                        /* Increment remaining free memory count */

                        wTemp = (ET9U16)(pRUDB->wRemainingMemory + nRecordLen);

                        __ET9AWWriteRUDBData(pLingInfo,
                                        (void ET9FARDATA *)&pRUDB->wRemainingMemory,
                                        (const void ET9FARDATA *)&wTemp,
                                        sizeof(wTemp) );

                        /* Store new udb checksum (minus 1 for decremented word count) */

                        wCheckSum = (ET9U16)(wCheckSum - (ET9UINT)1 + nRecordLen);

                        __ET9AWWriteRUDBData(pLingInfo,
                                        (void ET9FARDATA *)&pRUDB->wDataCheck,
                                        (const void ET9FARDATA *)&wCheckSum,
                                        sizeof(wCheckSum) );

                        break;
                    }
                }
            }

            /* move to the next record */

            nRecordLen = _ET9AWGetRecordLength(pRUDB, pbNext);
            pbNext += nRecordLen;
            __CheckForWrap_ByteP(pbNext, pRUDB);

        } while (pbNext != pbEnd);

        /* if we exit loop, did not find a match */

        if (pbNext == pbEnd) {
            wStatus = ET9STATUS_NO_MATCHING_WORDS;
        }
    }

    return wStatus;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Delete a given RDB word from the RUDB.
 * Delete a RDB entry by passing a word string and the word length.
 * Used mainly to remove ASDB shortcut references when they're removed from the ASDB (either user or LDB).
 *
 * @param pLingInfo      Pointer to alpha information structure.
 * @param psBuf          Pointer to word to match for deletion.
 * @param wWordLen       Length (in symbols) of word pointed to by psBuf.
 *
 * @return ET9STATUS_NONE if RDB word found and deleted;.
 *           ET9STATUS_NO_MATCHING_WORDS if no matching RDB word was found
 */

ET9STATUS ET9FARCALL _ET9AWRDBDeleteWord(ET9AWLingInfo *pLingInfo,
                                         ET9SYMB       *psBuf,
                                         ET9U16         wWordLen)
{
    ET9STATUS wStatus;
    if ((wStatus = _ET9AWSys_BasicValidityCheck(pLingInfo)) != ET9STATUS_NONE) {
        return wStatus;
    }
    if (psBuf == NULL) {
        return ET9STATUS_INVALID_MEMORY;
    }
    if (wWordLen < 1 || wWordLen > ET9MAXUDBWORDSIZE) {
        return ET9STATUS_BAD_PARAM;
    }
    if (pLingInfo->pLingCmnInfo->pRUDBInfo == NULL) {
        return ET9STATUS_NO_RUDB;
    }
    if (!pLingInfo->pLingCmnInfo->pRUDBInfo->wRDBWordCount) {
        return ET9STATUS_NO_MATCHING_WORDS;
    }

    {
        ET9AWLingCmnInfo * const pLingCmnInfo = pLingInfo->pLingCmnInfo;

        ET9AWRUDBInfo ET9FARDATA   *pRUDB;
        ET9U8 ET9FARDATA *pbNext;
        ET9U8 ET9FARDATA *pbTemp;
        ET9U8 ET9FARDATA *pbEnd;
        ET9U16  wSize;
        ET9U16  i;
        ET9U8 ET9FARDATA *pSymb;
        ET9UINT nRecordLen;
        ET9U16  wCheckSum, wTemp;
        ET9U16  wRegion;
        ET9SYMB *pTarget;
        ET9SYMB sCompSymb;

        const ET9U8 ET9FARDATA *pbByte;
        ET9U8   bLang;

        pRUDB = pLingCmnInfo->pRUDBInfo;

        /* Age the RUDB. */

        _ET9AWRUDBUpdateCounter(pLingInfo, 1);

        /* Get the address range for the region that should hold word (if it exists) */

        __ET9AWSetRUDBRangeInfo(pLingInfo, wWordLen, 0, &wRegion, &pbNext, &pbEnd);

        /* Skip past region indicator record */

        ++pbNext;
        __CheckForWrap_ByteP(pbNext, pRUDB);

        /* loop through the entries in that region */

        do {

            /* if this is a RDB record */

            if (ET9RDBTYPE == _ET9AWGetRecordType(pbNext)) {

                /* check to see if it matches */

                wSize = _ET9AWGetRDBWordLen(pbNext);

                /* if it's the same length, continue comparison */

                if (wSize == wWordLen) {

                    /* move past the length and frequency bytes of record header */

                    bLang = (ET9U8)*(_ET9AWMoveRUDBPtrRight(pRUDB, pbNext, (RDB_RECORD_HEADER_SIZE - 1)));

                    if ((bLang == (ET9U8)(pLingCmnInfo->wFirstLdbNum & ET9PLIDMASK)) ||
                        (bLang == (ET9U8)(pLingCmnInfo->wSecondLdbNum & ET9PLIDMASK))){

                        /* move past the length and frequency bytes of record header */

                        pSymb = pbNext + RDB_RECORD_HEADER_SIZE;
                        __CheckForWrap_ByteP(pSymb, pRUDB);
                        pTarget = psBuf;

                        /* loop through comparing symbols */

                        for (i = wSize; i; --i) {

                            /* if no match, move on to the next record */
                            /* ReadSymbol */

                            pbByte = pSymb + 1;
                            __CheckForWrap_ByteP(pbByte, pRUDB);

                            /* new logic doesn't expect lowercased RDB words */

                            sCompSymb = *pTarget++;
                            if (sCompSymb != ET9MAKEWORD(*pSymb, *pbByte)) {
                                break;
                            }

                            /* point to the next symbol */

                            pSymb += ET9SYMBOLWIDTH;
                            __CheckForWrap_ByteP(pSymb, pRUDB);
                        }

                        /* if all symbols match, mark as deleted */

                        if (!i) {

                            /* save the address of the record being deleted */

                            pbTemp = pbNext;

                            /* loop through decrementing the RUDB checksum by the bytes in the record */

                            nRecordLen = _ET9AWGetRecordLength(pRUDB, pbNext);
                            i = (ET9U16)nRecordLen;
                            wCheckSum = pRUDB->wDataCheck;

                            while (i--) {
                                wCheckSum = (ET9U16)(wCheckSum - *pbTemp);
                                ++pbTemp;
                                __CheckForWrap_ByteP(pbTemp, pRUDB);
                            }

                            /* add new free record header */

                            wCheckSum = __ET9AWRUDBWriteHeader(pLingInfo, pbNext, nRecordLen, ET9FREETYPE, wCheckSum);

                            /* Decrement the word count */

                            wTemp = (ET9U16)(pRUDB->wRDBWordCount - 1);

                            __ET9AWWriteRUDBData(pLingInfo,
                                            (void ET9FARDATA *)&pRUDB->wRDBWordCount,
                                            (const void ET9FARDATA *)&wTemp,
                                            sizeof(wTemp) );

                            /* Increment remaining free memory count */

                            wTemp = (ET9U16)(pRUDB->wRemainingMemory + nRecordLen);

                            __ET9AWWriteRUDBData(pLingInfo,
                                            (void ET9FARDATA *)&pRUDB->wRemainingMemory,
                                            (const void ET9FARDATA *)&wTemp,
                                            sizeof(wTemp) );

                            /* Store new udb checksum (minus 1 for decremented word count) */

                            wCheckSum = (ET9U16)(wCheckSum - (ET9UINT)1 + nRecordLen);

                            __ET9AWWriteRUDBData(pLingInfo,
                                            (void ET9FARDATA *)&pRUDB->wDataCheck,
                                            (const void ET9FARDATA *)&wCheckSum,
                                            sizeof(wCheckSum) );

                            break;
                        }
                    }
                }
            }

            /* move to the next record */

            nRecordLen = _ET9AWGetRecordLength(pRUDB, pbNext);
            pbNext += nRecordLen;
            __CheckForWrap_ByteP(pbNext, pRUDB);

        } while (pbNext != pbEnd);

        /* if we exit loop, did not find a match */

        if (pbNext == pbEnd) {
            wStatus = ET9STATUS_NO_MATCHING_WORDS;
        }

        ET9Assert(pLingCmnInfo->pRUDBInfo->wDataCheck == __ET9AWGetRUDBChecksum(pLingInfo));
    }

    return wStatus;
}

/*---------------------------------------------------------------------------*/
/**
 * Set RUDB to an initial clean state.
 * Removes all UDB.RDB records from RUDB.
 *
 * @param pLingInfo      Pointer to alpha information structure.
 *
 * @return ET9STATUS_NONE on success, else error status.
 */

ET9STATUS ET9FARCALL ET9AWRUDBReset(ET9AWLingInfo * const pLingInfo)
{
    ET9STATUS wStatus;
    ET9AWRUDBInfo   ET9FARDATA   *pRUDB;
    ET9U16       wTemp, wOffset;
    ET9UINT       nIndex, nLength;
    ET9U8        ET9FARDATA  *pbNext;

    wStatus = _ET9AWSys_BasicValidityCheck(pLingInfo);

    if (wStatus) {
        return wStatus;
    }

    if (!pLingInfo->pLingCmnInfo->pRUDBInfo) {
        return ET9STATUS_NO_RUDB;
    }

    pRUDB = pLingInfo->pLingCmnInfo->pRUDBInfo;

    /* zero out the wUDBWordCount and wRDBWordCount */

    wTemp = 0;

    __ET9AWWriteRUDBData(pLingInfo,
                (void ET9FARDATA *)&pRUDB->wUDBWordCount,
                (const void ET9FARDATA *)&wTemp, sizeof(wTemp));

    __ET9AWWriteRUDBData(pLingInfo,
                (void ET9FARDATA *)&pRUDB->wRDBWordCount,
                (const void ET9FARDATA *)&wTemp, sizeof(wTemp));

    /* reset wLastDelCutOffFreq to base value */

    wTemp = ET9RD_WORD_INIT_FREQ;

    __ET9AWWriteRUDBData(pLingInfo,
                (void ET9FARDATA *)&pRUDB->wLastDelCutOffFreq,
                (const void ET9FARDATA *)&wTemp, sizeof(wTemp));

    /* set remaining memory to full size of RUDB data area */

    wTemp = (ET9U16)ET9RUDBDataAreaBytes(pRUDB);

    /* reduce by the number of range markers */

    wTemp -= NUMSIZERANGES;

    __ET9AWWriteRUDBData(pLingInfo,
                (void ET9FARDATA *)&pRUDB->wRemainingMemory,
                (const void ET9FARDATA *)&wTemp, sizeof(wTemp));

    /* set up size region indices and free records in database.
       Initialize size regions with a single free byte at
       the beginning that will remain forever. */

    pbNext = ET9RUDBData(pRUDB);
    for (nIndex = 0; nIndex < NUMSIZERANGES; ++nIndex) {

        /* initially make size region sizes equitable */

        wOffset = (ET9U16)(nIndex * (ET9RUDBDataAreaBytes(pRUDB) / NUMSIZERANGES));

        __ET9AWWriteRUDBData(pLingInfo,
                    (void ET9FARDATA *)&pRUDB->wSizeOffset[nIndex],
                    (const void ET9FARDATA *)&wOffset, sizeof(wOffset) );

        /* region size will be same for all except (possibly) last region */

        nLength = (nIndex == (NUMSIZERANGES - 1)) ?
                                (ET9RUDBDataAreaBytes(pRUDB) - wOffset) :
                                (ET9RUDBDataAreaBytes(pRUDB) / NUMSIZERANGES);

        /* write single byte free record as region marker */

        __ET9AWRUDBWriteHeader(pLingInfo, pbNext, 1, ET9FREETYPE, 0);

        ++pbNext;
        __CheckForWrap_ByteP(pbNext, pRUDB);

        /* write single multi-byte free record that covers entire region */

        __ET9AWRUDBWriteHeader(pLingInfo, pbNext, nLength - 1, ET9FREETYPE, 0);
        pbNext += (nLength - 1);
        __CheckForWrap_ByteP(pbNext, pRUDB);
    }

    wTemp = (ET9U16) __ET9AWGetRUDBChecksum(pLingInfo);
    __ET9AWWriteRUDBData(pLingInfo, (void ET9FARDATA *)&pRUDB->wDataCheck, (const void ET9FARDATA *)&wTemp, sizeof(wTemp) );

    /* reset update counter to zero */

    _ET9AWRUDBUpdateCounter(pLingInfo, 0);

    /* Reset UDB entry  */

    pLingInfo->pLingCmnInfo->Private.pUDBGetEntry = 0;

    return wStatus;
}

/*---------------------------------------------------------------------------*/
/**
 * Removes all UDB records from RUDB, but leaves RDB records.
 *
 * @param pLingInfo      Pointer to alpha information structure.
 *
 * @return ET9STATUS_NONE on success, else error status.
 */

ET9STATUS ET9FARCALL ET9AWUDBOnlyReset(ET9AWLingInfo * const pLingInfo)
{
    ET9STATUS wStatus;
    ET9AWRUDBInfo ET9FARDATA   *pRUDB;
    ET9U8 ET9FARDATA *pbNext;
    ET9U8 ET9FARDATA *pbTemp;
    ET9U8 ET9FARDATA *pbEnd;
    ET9U16  i;
    ET9UINT nRecordLen;
    ET9U16  wCheckSum, wTemp;
    ET9U16  wRegion;

    if ((wStatus = _ET9AWSys_BasicValidityCheck(pLingInfo)) != ET9STATUS_NONE) {
        return wStatus;
    }
    if (pLingInfo->pLingCmnInfo->pRUDBInfo == NULL) {
        wStatus = ET9STATUS_NO_RUDB;
    }
    else if (!pLingInfo->pLingCmnInfo->pRUDBInfo->wUDBWordCount) {
        wStatus = ET9STATUS_NO_MATCHING_WORDS;
    }
    else {
        pRUDB = pLingInfo->pLingCmnInfo->pRUDBInfo;

        /* Age the RUDB. */

        _ET9AWRUDBUpdateCounter(pLingInfo, 1);

        /* get the beginning and end addresses of entire RUDB */

        __ET9AWSetRUDBRangeInfo(pLingInfo, 2, 1, &wRegion, &pbNext, &pbEnd);

        /* Move past range marker */

        pbNext = _ET9AWMoveRUDBPtrRight(pRUDB, pbNext, 1);

        /* loop through the region(s) returned */

        while (pbNext != pbEnd) {

            nRecordLen = _ET9AWGetRecordLength(pRUDB, pbNext);

            /* if this is a UDB record */

            if (ET9UDBTYPE == _ET9AWGetRecordType(pbNext)) {

                /* save the address of the record being deleted */

                pbTemp = pbNext;

                /* loop through decrementing the RUDB checksum by the bytes in the record */

                i = (ET9U16)nRecordLen;
                wCheckSum = pRUDB->wDataCheck;
                while (i--) {
                    wCheckSum = (ET9U16)(wCheckSum - *pbTemp);
                    ++pbTemp;
                    __CheckForWrap_ByteP(pbTemp, pRUDB);
                }

                /* add new free record header */

                wCheckSum = __ET9AWRUDBWriteHeader(pLingInfo,
                                        pbNext, nRecordLen,
                                        ET9FREETYPE, wCheckSum);

                /* Decrement the word count */

                wTemp = (ET9U16)(pRUDB->wUDBWordCount - 1);

                __ET9AWWriteRUDBData(pLingInfo,
                                (void ET9FARDATA *)&pRUDB->wUDBWordCount,
                                (const void ET9FARDATA *)&wTemp,
                                sizeof(wTemp) );

                /* Increment remaining free memory count */

                wTemp = (ET9U16)(pRUDB->wRemainingMemory + nRecordLen);

                __ET9AWWriteRUDBData(pLingInfo,
                                (void ET9FARDATA *)&pRUDB->wRemainingMemory,
                                (const void ET9FARDATA *)&wTemp,
                                sizeof(wTemp) );

                /* Store new udb checksum (minus 1 for decremented word count) */

                wCheckSum = (ET9U16)(wCheckSum - (ET9UINT)1 + nRecordLen);

                __ET9AWWriteRUDBData(pLingInfo,
                                (void ET9FARDATA *)&pRUDB->wDataCheck,
                                (const void ET9FARDATA *)&wCheckSum,
                                sizeof(wCheckSum) );
            }

            /* move to the next record */

            pbNext += nRecordLen;
            __CheckForWrap_ByteP(pbNext, pRUDB);
        }
    }

    return wStatus;
}

/*---------------------------------------------------------------------------*/
/**
 * Sets up the initial RUDB.
 * Either using a previously saved RUDB or a 'fresh' memory buffer.
 * Will reset the provided RUDB if it looks corrupted.
 *
 * @param pLingInfo      Pointer to alpha information structure.
 * @param pRUDBInfo      Pointer to RUDB buffer.
 * @param wDataSize      Size (in bytes) of provided RUDB buffer.
 * @param pWriteCB       Callback routine pointer (if integration layer handling RUDB writes); otherwise NULL.
 *
 * @return ET9STATUS_NONE on success, otherwise return error code.
 */

ET9STATUS ET9FARCALL ET9AWRUDBInit(ET9AWLingInfo * const                pLingInfo,
                                   ET9AWRUDBInfo ET9FARDATA * const     pRUDBInfo,
                                   const ET9U16                         wDataSize,
                                   const ET9DBWRITECALLBACK             pWriteCB)
{
    ET9STATUS wStatus;

    if ((wStatus = _ET9AWSys_BasicValidityCheck(pLingInfo)) == ET9STATUS_NONE) {

        if (((pRUDBInfo != NULL) && !wDataSize) || ((pRUDBInfo == NULL) && wDataSize)) {
            wStatus = ET9STATUS_INVALID_MEMORY;
        }

        /* Check minimum RUDB size
        */
        else if ((pRUDBInfo != NULL) && (wDataSize < ET9MINRUDBDATABYTES)) {
             wStatus = ET9STATUS_INVALID_SIZE;
        }
        else {

            /* if overwriting already initialized RUDB pointer */

            if ((pLingInfo->pLingCmnInfo->pRUDBInfo != NULL) && (pRUDBInfo != NULL) &&
               ((pLingInfo->pLingCmnInfo->pRUDBInfo != pRUDBInfo) ||
                (pRUDBInfo->wDataSize != wDataSize))) {

                wStatus = ET9STATUS_ALREADY_INITIALIZED;
            }
            pLingInfo->pRUDBWriteData = pWriteCB;
            pLingInfo->pLingCmnInfo->pRUDBInfo = pRUDBInfo;

            /* ok to have no RUDB */

            if (pRUDBInfo != NULL) {

                pLingInfo->pLingCmnInfo->Private.pUDBGetEntry = 0;

                /*  If checksum doesn't match, set up an empty RUDB with cur symbol class.*/

                if (((pRUDBInfo->wUDBWordCount + pRUDBInfo->wRDBWordCount) == 0) ||
                    (pRUDBInfo->wDataSize != wDataSize) ||
                    (pRUDBInfo->wDataCheck != (ET9U16)__ET9AWGetRUDBChecksum(pLingInfo))) {

                    __ET9AWWriteRUDBData(pLingInfo,
                            (void ET9FARDATA *)&pRUDBInfo->wDataSize,
                            (const void ET9FARDATA *)&wDataSize, sizeof(wDataSize));

                    ET9AWRUDBReset(pLingInfo);
                }
            }

            pLingInfo->pLingCmnInfo->Private.bStateRUDBEnabled = 1;
        }
    }

    return wStatus;
}

/*---------------------------------------------------------------------------*/
/**
 * Gets the number of currently defined UDB words.
 *
 * @param pLingInfo      Pointer to alpha information structure.
 * @param pEntryCount    Pointer to store UDB word count in.
 *
 * @return ET9STATUS_NONE on success, otherwise return error code.
 */

ET9STATUS ET9FARCALL ET9AWUDBGetWordCount(ET9AWLingInfo * const pLingInfo,
                                          ET9U16        * const pEntryCount)
{
    ET9STATUS wStatus;

    if ((wStatus = _ET9AWSys_BasicValidityCheck(pLingInfo)) == ET9STATUS_NONE) {
        if (pEntryCount == NULL) {
            wStatus = ET9STATUS_INVALID_MEMORY;
        }
        else if (pLingInfo->pLingCmnInfo->pRUDBInfo == NULL) {
            wStatus = ET9STATUS_NO_RUDB;
        }
        else {
            *pEntryCount = pLingInfo->pLingCmnInfo->pRUDBInfo->wUDBWordCount;
        }
    }
    return wStatus;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Process selection list QUDB entries.
 * Looks at words in selection list preceding passed index (which
 * is the index of the word user selected). If word is QUDB entry,
 * assume user saw word and bypassed it, so adjust the freq downward.
 * If the user has seen the word enough, go ahead and purge it.
 *
 * @param pLingInfo      Pointer to alpha information structure.
 * @param bIndex         Index position of selection list word user selected.
 *
 * @return None
 */

void ET9FARCALL _ET9ProcessSelListQUDBEntries(ET9AWLingInfo   *pLingInfo,
                                              ET9U8            bIndex)
{
    ET9AWLingCmnInfo * const pLingCmnInfo = pLingInfo->pLingCmnInfo;
    ET9AWRUDBInfo ET9FARDATA * const pRUDB = pLingCmnInfo->pRUDBInfo;

    ET9AWPrivWordInfo  *pWord;
    ET9U8               i;
    ET9U8 ET9FARDATA   *pbStart;
    ET9U8 ET9FARDATA   *pbTemp;
    ET9U8 ET9FARDATA   *pbEnd;
    ET9U16              wRegion;
    ET9UINT             nCurRecType = ET9FREETYPE;
    ET9SYMB ET9FARDATA *pSymb;
    ET9UINT             nRecordLength;
    ET9U16              wWordSize = 0;
    ET9U16              j;
    ET9U16              wCheckSum;
    ET9U16              wTemp;
    ET9U16              wFreq;
    const ET9U8 ET9FARDATA *pbByte;

    ET9Assert(pLingInfo);

    if (!pRUDB || !ET9RUDBENABLED(pLingCmnInfo)) {
        return;
    }

    ET9Assert(pLingCmnInfo->pRUDBInfo->wDataCheck == __ET9AWGetRUDBChecksum(pLingInfo));

    /* loop through all words in the selection list */

    for (i = 0; i < pLingCmnInfo->Private.bTotalWords; ++i) {

        /* if this was not the selected word */

        if (i != bIndex) {

            pWord =  &pLingCmnInfo->Private.pWordList[pLingCmnInfo->Private.bWordList[i]];

            /* if it is a QUDB sourced word, is a terminal and is NOT a spell correction... */

            if ((GETRAWSRC(pWord->bWordSrc) == ET9WORDSRC_QUDB) &&
                !pWord->Base.bIsSpellCorr &&
                !pWord->Base.wWordCompLen) {

                /* get the beginning and end addresses of range record should be in (if there) */

                __ET9AWSetRUDBRangeInfo(pLingInfo, pWord->Base.wWordLen, 0, &wRegion, &pbStart, &pbEnd);

                /* Move past range marker */

                ++pbStart;
                __CheckForWrap_ByteP(pbStart, pRUDB);

                /* loop through the region returned */

                while (pbStart != pbEnd) {

                    nRecordLength = _ET9AWGetRecordLength(pRUDB, pbStart);
                    nCurRecType = _ET9AWGetRecordType(pbStart);

                    /* if it's a UDB word */

                    if (nCurRecType == ET9UDBTYPE) {

                        wWordSize = _ET9AWGetUDBWordLen(pbStart);
                        wFreq  = (ET9U16)_ET9AWGetUDBFrequency(pRUDB, pbStart);

                        /* if QUDB word of the right size */

                        if ((wFreq > ET9MAX_FREQ_COUNT) && (wWordSize == pWord->Base.wWordLen)) {

                            /* loop through comparing symbols */

                            pSymb = (ET9SYMB ET9FARDATA *)(pbStart + UDB_RECORD_HEADER_SIZE);
                            __CheckForWrap_WordP(pSymb, pRUDB);
                            for (j = 0; j < wWordSize; ++j) {

                                /* if symbol doesn't match, move on to the next record */
                                /* ReadSymbol */

                                pbByte = (const ET9U8 ET9FARDATA *)pSymb + 1;
                                __CheckForWrap_ByteP(pbByte, pRUDB);

                                if (pWord->Base.sWord[j] != ET9MAKEWORD(*((const ET9U8 ET9FARDATA *)pSymb), *pbByte)) {
                                    break;
                                }

                                /* point to the next symbol in the record entry */

                                ++pSymb;
                                __CheckForWrap_WordP(pSymb, pRUDB);
                            }

                            /* if looped through all symbols and all matched, update frequency */

                            if (j == wWordSize) {

                                --wFreq;

                                /* if record has already been presented in sellist enough times, purge it */

                                if (wFreq == ET9MAX_FREQ_COUNT) {

                                    j = (ET9U16)nRecordLength;

                                    /* save the address of the record being deleted */

                                    pbTemp = pbStart;
                                    wCheckSum = pRUDB->wDataCheck;
                                    while (j--) {
                                        wCheckSum = (ET9U16)(wCheckSum - *pbTemp);
                                        ++pbTemp;
                                        __CheckForWrap_ByteP(pbTemp, pRUDB);
                                    }

                                    /* add new free record header */

                                    wCheckSum = __ET9AWRUDBWriteHeader(pLingInfo,
                                                    pbStart, nRecordLength,
                                                    ET9FREETYPE, wCheckSum);

                                    /* Decrement the word count */

                                    wTemp = (ET9U16)(pRUDB->wUDBWordCount - 1);

                                    __ET9AWWriteRUDBData(pLingInfo,
                                            (void ET9FARDATA *)&pRUDB->wUDBWordCount,
                                            (const void ET9FARDATA *)&wTemp,
                                            sizeof(wTemp) );

                                    /* Increment remaining free memory count */

                                    wTemp = (ET9U16)(pRUDB->wRemainingMemory + nRecordLength);

                                    __ET9AWWriteRUDBData(pLingInfo,
                                            (void ET9FARDATA *)&pRUDB->wRemainingMemory,
                                            (const void ET9FARDATA *)&wTemp,
                                            sizeof(wTemp));

                                    /* Store new udb checksum (minus 1 for decremented word count) */

                                    wCheckSum = (ET9U16)(wCheckSum - (ET9UINT)1 + nRecordLength);

                                    __ET9AWWriteRUDBData(pLingInfo,
                                            (void ET9FARDATA *)&pRUDB->wDataCheck,
                                            (const void ET9FARDATA *)&wCheckSum,
                                            sizeof(wCheckSum) );

                                }

                                /* otherwise just decrement the freq by 1 */

                                else {

                                    wTemp = wFreq + 1;
                                    wCheckSum = pRUDB->wDataCheck;
                                    wCheckSum = (ET9U16)(wCheckSum - ET9HIBYTE(wTemp) - ET9LOBYTE(wTemp) + ET9HIBYTE(wFreq) + ET9LOBYTE(wFreq));

                                    /* move to freq position and write it. */

                                    pbTemp = pbStart + 1;
                                    __CheckForWrap_ByteP(pbTemp, pRUDB);
                                    __ET9AWRUDBWriteWord(pLingInfo, pbTemp, wFreq);

                                    /* rewrite checksum */

                                    __ET9AWWriteRUDBData(pLingInfo, (void ET9FARDATA *)&pRUDB->wDataCheck, (const void ET9FARDATA *)&wCheckSum, sizeof(wCheckSum) );

                                }
                                break;
                            }
                        }
                    }

                    /* move to the next record */

                    pbStart += nRecordLength;
                    __CheckForWrap_ByteP(pbStart, pRUDB);
                }
            }
        }
    }

    ET9Assert(pLingCmnInfo->pRUDBInfo->wDataCheck == __ET9AWGetRUDBChecksum(pLingInfo));
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Search RUDB for RDB/UDB/BOTH selection list candidates.
 *
 * @param pLingInfo          Pointer to alpha information structure.
 * @param wRecType           Type of record to search for (ET9UDBTYPE, ET9RDBTYPE, or 0 (== both UDB and RDB).
 * @param wIndex             Pass-along for _ET9AWSelLstWordSearch.
 * @param wLength            Length (in symbols) of current word.
 * @param bFreqIndicator     Pass-along for _ET9AWSelLstWordSearch.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL _ET9AWRUDBWordsSearch(ET9AWLingInfo        *pLingInfo,
                                           ET9UINT              wRecType,
                                           ET9U16               wIndex,
                                           ET9U16               wLength,
                                           ET9_FREQ_DESIGNATION bFreqIndicator)

{
    ET9AWLingCmnInfo        * const pLingCmnInfo = pLingInfo->pLingCmnInfo;
    ET9AWRUDBInfo ET9FARDATA * const pRUDB = pLingCmnInfo->pRUDBInfo;
    ET9STATUS               wStatus = ET9STATUS_NONE;
    ET9U8 ET9FARDATA        *pbNext;
    ET9U8 ET9FARDATA        *pbEnd;
    ET9UINT                 nRecordLength;
    ET9UINT                 nRecordType;
    ET9U16                  wRegion;
    ET9UINT                 wSize, i;
    const ET9U8 ET9FARDATA  *pSymb;
    ET9SYMB                 *pHolder;
    const ET9U8 ET9FARDATA  *pbByte;
    ET9U8                   bLdbNum;
    ET9U8                   bFound;
    const ET9BOOL           bUsingLM = (ET9BOOL)(ET9LMENABLED(pLingCmnInfo) && pLingCmnInfo->Private.ALdbLM.bSupported);

    ET9Assert(pLingInfo);

    if ((wLength < 1) || !pRUDB || !ET9RUDBENABLED(pLingCmnInfo)) {
        return wStatus;
    }

    ET9Assert(pLingCmnInfo->pRUDBInfo->wDataCheck == __ET9AWGetRUDBChecksum(pLingInfo));

    __ET9AWSetRUDBRangeInfo(pLingInfo,
                            pLingCmnInfo->Private.wCurrMinSourceLength,
                            1 /* bIncludeCompletions */,
                            &wRegion,
                            &pbNext,
                            &pbEnd);

    /* loop through qualifying size regions in the RUDB */

    do {

        /* if this is an RDB record */

        nRecordType = _ET9AWGetRecordType(pbNext);

        if (nRecordType != ET9FREETYPE &&
           (!wRecType || (wRecType == nRecordType))) {

            ET9AWPrivWordInfo sLocalWord;

            _InitPrivWordInfo(&sLocalWord);

            /* go see if it is a possible selection list candidate */

            if (nRecordType == ET9RDBTYPE) {

                /* Transfer the RDB record to the ET9AWPrivWordInfo struct */

                /* get the length of the RDB word */

                wSize = _ET9AWGetRDBWordLen(pbNext);

                /* only continue if same language id */

                bLdbNum = (ET9U8)*(_ET9AWMoveRUDBPtrRight(pRUDB, pbNext, (RDB_RECORD_HEADER_SIZE - 1)));

                if ((bLdbNum == (ET9UINT)(pLingCmnInfo->wFirstLdbNum & ET9PLIDMASK)) ||
                    (bLdbNum == (ET9UINT)(pLingCmnInfo->wSecondLdbNum & ET9PLIDMASK)) ){

                    /* get the frequency */

                    /* Read Word */

                    pSymb = pbNext + 1;
                    __CheckForWrap_ByteP(pSymb, pRUDB);
                    pbByte = pSymb + 1;
                    __CheckForWrap_ByteP(pbByte, pRUDB);
                    sLocalWord.xWordFreq = (ET9FREQPART)ET9MAKEWORD(*pSymb, *pbByte);
                    sLocalWord.xTapFreq  = 1;
                    sLocalWord.Base.wWordLen  = (ET9U16)wSize;
                    sLocalWord.bWordSrc = ET9WORDSRC_RUDB;

                    /* skip past length, LDB and frequency bytes in record header */

                    pSymb = pbNext + RDB_RECORD_HEADER_SIZE;
                    __CheckForWrap_ByteP(pSymb, pRUDB);
                    pHolder = sLocalWord.Base.sWord;

                    /* loop through and transfer word */

                    for (i = wSize; i; --i) {
                        pbByte = pSymb + 1;
                        __CheckForWrap_ByteP(pbByte, pRUDB);
                        *pHolder++ = ET9MAKEWORD(*pSymb, *pbByte);
                        pSymb = pbByte + 1;
                        __CheckForWrap_ByteP(pSymb, pRUDB);
                    }

                    /* if downshifting LDB words, need to do RDB words as well (CR 23769) */

                    if (ET9DOWNSHIFTALLLDB(pLingInfo->pLingCmnInfo)) {

                        pHolder = sLocalWord.Base.sWord;

                        /* loop through and transfer word */

                        for (i = wSize; i; --i, ++pHolder) {
                            *pHolder = _ET9SymToLower(*pHolder, (ET9U16)bLdbNum);
                        }
                    }
                    if (bLdbNum == (ET9UINT)(pLingCmnInfo->wFirstLdbNum & ET9PLIDMASK)) {
                        sLocalWord.Base.bLangIndex = ET9AWFIRST_LANGUAGE;
                    }
                    else if (bLdbNum == (ET9UINT)(pLingCmnInfo->wSecondLdbNum & ET9PLIDMASK)) {
                        sLocalWord.Base.bLangIndex = ET9AWSECOND_LANGUAGE;
                    }
                }
            }
            else if (nRecordType == ET9UDBTYPE) {

                /* Transfer the UDB record to the ET9AWPrivWordInfo struct */

                /* get the length of the UDB word */

                wSize = _ET9AWGetUDBWordLen(pbNext);

                /* get the frequency */

                /* Read Word */

                pSymb = pbNext + 1;
                __CheckForWrap_ByteP(pSymb, pRUDB);
                pbByte = pSymb + 1;
                __CheckForWrap_ByteP(pbByte, pRUDB);
                sLocalWord.xWordFreq = (ET9FREQPART)ET9MAKEWORD(*pSymb, *pbByte);
                sLocalWord.xTapFreq  = 1;
                sLocalWord.Base.wWordLen = (ET9U16)wSize;

                if (ET9AWSys_GetBilingualSupported(pLingInfo)) {
                    sLocalWord.Base.bLangIndex = ET9AWBOTH_LANGUAGES;
                }
                else {
                    sLocalWord.Base.bLangIndex = ET9AWFIRST_LANGUAGE;
                }
                sLocalWord.bWordSrc = ET9WORDSRC_RUDB;
                pSymb = pbNext + UDB_RECORD_HEADER_SIZE;
                __CheckForWrap_ByteP(pSymb, pRUDB);
                pHolder = sLocalWord.Base.sWord;

                /* loop through and transfer word */

                for (i = wSize; i; --i) {
                    pbByte = pSymb + 1;
                    __CheckForWrap_ByteP(pbByte, pRUDB);
                    *pHolder++ = ET9MAKEWORD(*pSymb, *pbByte);
                    pSymb = pbByte + 1;
                    __CheckForWrap_ByteP(pSymb, pRUDB);
                }
                sLocalWord.bIsUDBWord = 1;
            }
            else {
                ET9Assert(0);
            }

            /* assign index to keep ties consistent */

            sLocalWord.wEWordFreq = 0;
            sLocalWord.wTWordFreq = 0;
            sLocalWord.dwWordIndex = 0;

            /* if possible selection, go verify */

            if (wStatus == ET9STATUS_NONE) {

                /* if this is a 'questionable' UDB entry, mark as QUDB word source */

                if (nRecordType == ET9UDBTYPE && sLocalWord.xWordFreq > ET9MAX_FREQ_COUNT) {
                    sLocalWord.bWordSrc = ET9WORDSRC_QUDB;
                }

                /* if word being compounded, downshift it */

                if (wIndex) {
                    ET9SYMB *pSymb = sLocalWord.Base.sWord;
                    ET9UINT i = sLocalWord.Base.wWordLen;

                    for (; i; --i, ++pSymb) {
                        *pSymb = _ET9SymToLower(*pSymb, pLingCmnInfo->wLdbNum);
                    }
                }

                wStatus = _ET9AWSelLstWordMatch(pLingInfo, &sLocalWord, wIndex, wLength, &bFound);

                if (wStatus != ET9STATUS_NONE) {
                    return wStatus;
                }

                if (bFound) {
                    if (nRecordType == ET9UDBTYPE && sLocalWord.xWordFreq > ET9MAX_FREQ_COUNT) {
                        if (pLingCmnInfo->Private.eCurrSelectionListMode == ET9ASLMODE_MIXED) {
                            sLocalWord.xWordFreq = 1;
                        }
                    }
                    else if (pLingCmnInfo->Private.eCurrSelectionListMode == ET9ASLMODE_MIXED) {

                        sLocalWord.xWordFreq = (ET9FREQPART)(sLocalWord.xWordFreq / ET9FREQ_BUMP_COUNT);

                        if (bUsingLM) {
                            sLocalWord.xWordFreq = (ET9FREQPART)(sLocalWord.xWordFreq * ET9SUPP_DB_FREQ_BUMP_COUNT);
                            sLocalWord.xWordFreq = (ET9FREQPART)(sLocalWord.xWordFreq + ET9_SUPP_DB_BASE_FREQ);
                        }
                        else {
                            sLocalWord.xWordFreq = (ET9FREQPART)(sLocalWord.xWordFreq * ET9SUPP_DB_FREQ_BUMP_COUNT_UNI);
                            sLocalWord.xWordFreq = (ET9FREQPART)(sLocalWord.xWordFreq + ET9_SUPP_DB_BASE_FREQ_UNI);
                        }

                    }

                    _ET9AWSelLstAdd(pLingInfo, &sLocalWord, wLength, bFreqIndicator);
                }
            }
        }

        /* move to the next record */

        nRecordLength = _ET9AWGetRecordLength(pRUDB, pbNext);
        pbNext += nRecordLength;
        __CheckForWrap_ByteP(pbNext, pRUDB);

    } while (pbNext != pbEnd);

    ET9Assert(pLingCmnInfo->pRUDBInfo->wDataCheck == __ET9AWGetRUDBChecksum(pLingInfo));

    return wStatus;
}

/*---------------------------------------------------------------------------*/
/**
 * Get UDB Word.
 * Designed to be used repeatedly to retrieve all of the UDB words contained in the RUDB.<P>
 *
 * The first time the routine is called, the value contained in *pmBufLen
 * should be 0; subsequent calls should contain the previously retrieved
 * UDB word in *psBuf and the size of that previously retrieved word in
 * *pmBufLen.<P>
 *
 * If *pmBufLen is non-zero, function first compares the word passed in
 * *psBuf against the record pointed to by the internally maintained
 * (and private)pUDBGetEntry (which points to the last processed UDB record).
 * If they don't match, it indicates an update has been made to the
 * RUDB that would disrupt normal, consecutive record processing, and the
 * function restarts the process at the beginning of the RUDB
 * (and returns a status of ET9STATUS_WORD_NOT_FOUND along with the first UDB word).<P>
 *
 * When processing 'forward', the routine will return the first UDB record
 * and proceed forward through the RUDB. When processing in 'reverse', the routine
 * starts with the last UDB word in the current RUDB and then word backwords
 * through the RUDB.<BR>
 * If the API is called again after traversing all UDB words, either forward
 * or in reverse, the routine will return the ET9STATUS_NO_MATCHING_WORDS status.
 *
 * @param pLingInfo       Pointer to alphabetic information structure.
 * @param psWordBuf       Pointer to buffer for loading UDB word (should contain the previously retrieved UDB word on subsequent calls).
 * @param wWordBufLen     Size (in symbols) of the input buffer (should be atleast ET9MAXWORDSIZE).
 * @param pwWordLen       Pointer to load current UDB word length into (should contain the previously retrieved UDB word length on subsequent calls).
 * @param bForward        1 to get first/next word, 0 to get previous word.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.<BR>
 *         On success:  *psBuf contains the (first/next) UDB word,
 *                      *pmBufLen contains the UDB word length
 */

ET9STATUS ET9FARCALL ET9AWUDBGetWord(ET9AWLingInfo * const  pLingInfo,
                                     ET9SYMB       * const  psWordBuf,
                                     const ET9U16           wWordBufLen,
                                     ET9U16        * const  pwWordLen,
                                     const ET9U8            bForward)
{
    ET9STATUS   wStatus;
    ET9U8       ET9FARDATA *pbNext;
    ET9U8       ET9FARDATA *pbLast;
    ET9SYMB     ET9FARDATA *pSymb;
    ET9UINT     nFound, nRecordType, i;
    ET9U16      wSizeChecked;
    ET9STATUS   mStatus = ET9STATUS_NONE;
    ET9AWRUDBInfo ET9FARDATA *pRUDB;
    ET9U8 ET9FARDATA *pEntry;
    ET9SYMB    *pTarget;
    ET9U8 ET9FARDATA *pbByte;
    ET9U16      wOldBufLen;
    ET9SYMB    *psCurrWordBuf = (ET9SYMB *)psWordBuf;

    if ((wStatus = _ET9AWSys_BasicValidityCheck(pLingInfo)) != ET9STATUS_NONE) {
        return wStatus;
    }
    if (psWordBuf == NULL || pwWordLen == NULL) {
        return ET9STATUS_INVALID_MEMORY;
    }
    else if (wWordBufLen < ET9MAXWORDSIZE) {
        return ET9STATUS_BUFFER_TOO_SMALL;
    }

    pRUDB = pLingInfo->pLingCmnInfo->pRUDBInfo;
    if (pRUDB == NULL) {
        return ET9STATUS_NO_RUDB;
    }
    wOldBufLen = *pwWordLen;

    /* if wordlen is too big, return error */

    if (wOldBufLen > ET9MAXUDBWORDSIZE) {
        return ET9STATUS_BAD_PARAM;
    }

    /* if no udb words in UDB, return */

    if (!pRUDB->wUDBWordCount) {
        return ET9STATUS_NO_MATCHING_WORDS;
    }
    /* if the saved entry point is 0, either never used or reset; */
    /* in either case, start over */

    if (!pLingInfo->pLingCmnInfo->Private.pUDBGetEntry) {
        wOldBufLen = 0;
    }

    nFound = 0;

    /* Not getting the first UDB word */

    if (wOldBufLen) {

        /*
         * Check the last return word at pLingInfo->Private.pUDBGetEntry.. If we
         * can't find the word, garbage collection/word deletion happened. We then
         * need to search from the beginning.
         */

        pEntry = pLingInfo->pLingCmnInfo->Private.pUDBGetEntry;

        if (_ET9AWGetRecordType(pEntry) == ET9UDBTYPE) {

            if (_ET9AWGetUDBWordLen(pEntry) == wOldBufLen) {

                pSymb = (ET9SYMB ET9FARDATA *)(pEntry + UDB_RECORD_HEADER_SIZE);
                __CheckForWrap_WordP(pSymb, pRUDB);
                pTarget = psCurrWordBuf;

                for (i = wOldBufLen; i; --i) {

                    /* ReadSymbol */

                    pbByte = (ET9U8 ET9FARDATA *)pSymb + 1;
                    __CheckForWrap_ByteP(pbByte, pRUDB);
                    if (*pTarget++ != ET9MAKEWORD(*((const ET9U8 ET9FARDATA *)pSymb), *pbByte)) {
                       break;
                    }

                    ++pSymb;
                    __CheckForWrap_WordP(pSymb, pRUDB);
                }
                if (!i) {
                    nFound = 1;
                }
            }
        }

        /* if not found, start over */

        if (!nFound) {
            mStatus = ET9STATUS_WORD_NOT_FOUND;
            pEntry = ET9RUDBData(pRUDB) + pRUDB->wSizeOffset[0];
        }
    }

    /* otherwise, start at the beginning */

    else {
        pEntry = ET9RUDBData(pRUDB) + pRUDB->wSizeOffset[0];
    }

    /* pbNext will point to requested word */

    pbNext = pEntry;

    /* if found and going forward, go to next record */

    if (nFound && bForward) {
        pbNext += _ET9AWGetRecordLength(pRUDB, pbNext);
        __CheckForWrap_ByteP(pbNext, pRUDB);

        /* if we wrapped without finding next word */

        if (pbNext == ET9RUDBData(pRUDB) + pRUDB->wSizeOffset[0]) {
            return ET9STATUS_NO_MATCHING_WORDS;

        }
    }

    /* at this point, we are pointing to the beginning of the database if
    mOldBufLen = 0 and we are searching forward. In this case we want to continue with
    the processing if we are not pointing to a udb record.in all other cases, we need
    to continue processing. */

    if (!(wOldBufLen == 0 && bForward && _ET9AWGetRecordType(pbNext) == ET9UDBTYPE)) {

        wSizeChecked = 0;
        if ((!nFound && (wOldBufLen > 0)) || bForward) {

            /*  continue forwardto get next UDB record */

            nRecordType = _ET9AWGetRecordType(pbNext);
            while (nRecordType != ET9UDBTYPE) {

                pbNext += _ET9AWGetRecordLength(pRUDB, pbNext);
                __CheckForWrap_ByteP(pbNext, pRUDB);

                if ((pbNext == ET9RUDBData(pRUDB) + pRUDB->wSizeOffset[0]) || /* we wrapped without find next word */
                    (wSizeChecked > ET9RUDBDataAreaBytes(pRUDB))) {           /* this is here for safety */
                    return ET9STATUS_NO_MATCHING_WORDS;
                }

                wSizeChecked = (ET9U16)(wSizeChecked +  _ET9AWGetRecordLength(pRUDB, pbNext));
                nRecordType = _ET9AWGetRecordType(pbNext);
            }
        }
        else { /* searching backward */

            /* did find word, search through database looking for previous one. */

            pbNext = ET9RUDBData(pRUDB) + pRUDB->wSizeOffset[0];
            nRecordType = _ET9AWGetRecordType(pbNext);
            pbLast = 0;

            while (wSizeChecked < ET9RUDBDataAreaBytes(pRUDB)) {  /* this is here for safety */

                wSizeChecked = (ET9U16)(wSizeChecked + _ET9AWGetRecordLength(pRUDB, pbNext));

                if ((pbNext == pEntry) && wOldBufLen) {

                    /* reached sent in word, set pbNext to previous one */

                    pbNext = pbLast;
                    break;
                }
                if (wSizeChecked == ET9RUDBDataAreaBytes(pRUDB)) {

                    /* reached the end, set pbNext to last udb type in database */

                    pbNext = (nRecordType == ET9UDBTYPE) ? pbNext : pbLast;
                    break;
                }
                if (nRecordType == ET9UDBTYPE) {
                    pbLast = pbNext; /* update pbLast */
                }
                pbNext += _ET9AWGetRecordLength(pRUDB, pbNext);
                __CheckForWrap_ByteP(pbNext, pRUDB);
                nRecordType = _ET9AWGetRecordType(pbNext);
            }

            if (!pbLast) { /* pbLast will be zero if the sent in word was first */
                return ET9STATUS_NO_MATCHING_WORDS;
            }
        }
    }

    /* at this point pbNext is pointing at our word */
    /* Save the pointer of the return UDB word. */

    pLingInfo->pLingCmnInfo->Private.pUDBGetEntry = pbNext;

    /* Get word length */

    *pwWordLen = _ET9AWGetUDBWordLen(pbNext);

    /* Copy word */

    pSymb = (ET9SYMB ET9FARDATA *)(pbNext + UDB_RECORD_HEADER_SIZE);
    __CheckForWrap_WordP(pSymb, pRUDB);

    for (i = *pwWordLen; i; --i) {

        /* ReadSymbol */

        pbByte = (ET9U8 ET9FARDATA *)pSymb + 1;
        __CheckForWrap_ByteP(pbByte, pRUDB);
        *psCurrWordBuf++ = ET9MAKEWORD(*((ET9U8 ET9FARDATA *)pSymb), *pbByte);

        ++pSymb;
        __CheckForWrap_WordP(pSymb, pRUDB);
    }

    return mStatus;
}

#ifdef LONG_RUDB_TEST
void ET9FARCALL _TestRUDBIntegrity(ET9AWLingInfo  *pLingInfo) {

    ET9U8 ET9FARDATA   *pbStart;
    ET9U8 ET9FARDATA   *pbEnd;
    ET9U8 ET9FARDATA   *pbReg;
    ET9U16              wRegion;
    ET9UINT             nCurRecType;
    ET9SYMB ET9FARDATA *pSymb;
    ET9UINT             nRecordLength;
    ET9U16              wWordSize = 0;
    ET9AWRUDBInfo ET9FARDATA *pRUDB;
    ET9UINT       nRemainingMemory = 0;

    ET9Assert(pLingInfo);
    pRUDB = pLingInfo->pLingCmnInfo->pRUDBInfo;
    ET9Assert(pRUDB != NULL);

#ifdef ET9_DEBUG
    __VerifySingleBytes(pRUDB);
#endif

    /* get the beginning and end addresses of range record should be in (if there) */
    __ET9AWSetRUDBRangeInfo(pLingInfo, 2, 1, &wRegion, &pbStart, &pbEnd);
    /* Move past range marker */
    pbStart = _ET9AWMoveRUDBPtrRight(pRUDB, pbStart, 1);
    ++nRemainingMemory;
    ++wRegion;
    pbReg = ET9RUDBData(pRUDB) + pRUDB->wSizeOffset[wRegion];

    /* loop through the region(s) returned */
    while (pbStart != pbEnd) {
        nRecordLength = _ET9AWGetRecordLength(pRUDB, pbStart);
        nCurRecType = _ET9AWGetRecordType(pbStart);

        /* if it's not a free record (no use for free records in search) */
        if (nCurRecType != ET9FREETYPE) {
            /* if it's a UDB record */
            if (nCurRecType == ET9UDBTYPE) {
                wWordSize = _ET9AWGetUDBWordLen(pbStart);
                /* move past the length and freq bytes */
                pSymb = (ET9SYMB *)_ET9AWMoveRUDBPtrRight(pRUDB, pbStart, UDB_RECORD_HEADER_SIZE);
            }
            else {
                /* it had _better_ be a RDB record */
                ET9Assert(nCurRecType == ET9RDBTYPE);
                wWordSize = _ET9AWGetRDBWordLen(pbStart);
                /* move past the length, LDB and freq bytes */
                pSymb = (ET9SYMB *)_ET9AWMoveRUDBPtrRight(pRUDB, pbStart, RDB_RECORD_HEADER_SIZE);
            }
        }
        else {
            if (pbStart == pbReg) {
                ET9Assert(*pbStart == ET9ONEBYTEFREEMASK);
                if (wRegion == NUMSIZERANGES) {
                    ET9Assert(pbStart == pbEnd);
                    break;
                }
                nRemainingMemory += nRecordLength;
                ++wRegion;
                if (wRegion == NUMSIZERANGES) {
                    pbReg = ET9RUDBData(pRUDB) + pRUDB->wSizeOffset[0];
                }
                else {
                    pbReg = ET9RUDBData(pRUDB) + pRUDB->wSizeOffset[wRegion];
                }
            }
            else {
                nRemainingMemory += nRecordLength;
            }
        }
        /* move to the next record */
        pbStart = _ET9AWMoveRUDBPtrRight(pRUDB, pbStart, nRecordLength);
    }
    /* remove the range markers from available size */
    nRemainingMemory -= NUMSIZERANGES;
    /* the free size calculated SHOULD match the value maintained in the header */
    ET9Assert(nRemainingMemory == (ET9UINT)pRUDB->wRemainingMemory);

    return;
}
#endif

/*---------------------------------------------------------------------------*/
/** \internal
 * Calculates the TUDB record length (in bytes) from RUDB record.
 *
 * @param pRUDB          Pointer to RUDB.
 * @param pbCurrent      Pointer to RUDB record.
 *
 * @return Record length in bytes.
 */

static ET9U16 ET9LOCALCALL __ET9AWGetTRUDBRecLen(ET9AWRUDBInfo ET9FARDATA *pRUDB,
                                                 ET9U8         ET9FARDATA *pbCurrent)
{
    ET9U8 ET9FARDATA *pbSymb;
    ET9U8 ET9FARDATA *pbByte;
    ET9SYMB  sSymb;
    ET9U16   wWordLen;
    ET9UINT  nRecType = _ET9AWGetRecordType(pbCurrent);
    ET9U16   wRecLen;
    ET9U16   i;
    ET9U8    bUtf8[4];

    /* determine size of the exported record */
    /* this function should only either point to ET9UDBTYPE or ET9RDBTYPE */

    if (nRecType == ET9UDBTYPE) {
        wWordLen = _ET9AWGetUDBWordLen(pbCurrent);
        pbSymb = _ET9AWMoveRUDBPtrRight(pRUDB, pbCurrent, UDB_RECORD_HEADER_SIZE);
        wRecLen = ET9_TUDBRECFIXLEN;
    }
    else if (nRecType == ET9RDBTYPE){
        wWordLen = _ET9AWGetRDBWordLen(pbCurrent);
        pbSymb = _ET9AWMoveRUDBPtrRight(pRUDB, pbCurrent, RDB_RECORD_HEADER_SIZE);
        wRecLen = ET9_TRDBRECFIXLEN;
    }
    else {
        wWordLen = 0;
        wRecLen = 0;
        pbSymb = NULL;
    }

    for (i = wWordLen; i; --i) {
        pbByte = (ET9U8 ET9FARDATA *)pbSymb + 1;
        __CheckForWrap_ByteP(pbByte, pRUDB);
        sSymb = ET9MAKEWORD(*pbSymb, *pbByte);
        pbSymb = _ET9AWMoveRUDBPtrRight(pRUDB, pbSymb, ET9SYMBOLWIDTH);
        wRecLen = wRecLen + _ET9SymbToUtf8(sSymb, bUtf8);

    }

    return wRecLen;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Calculates the size of TUDB needed to export a given RUDB.
 *
 * @param pLingInfo          Pointer to ET9 information structure.
 * @param pdwTotalWords      Pointer to store total words being exported.
 *
 * @return Size of TUDB.
 */

ET9U32 ET9FARCALL _ET9AWRUDBGetSize(ET9AWLingInfo    *pLingInfo,
                                    ET9U32           *pdwTotalWords)
{
    ET9AWLingCmnInfo * const pLingCmnInfo = pLingInfo->pLingCmnInfo;
    ET9AWRUDBInfo ET9FARDATA * const pRUDB = pLingCmnInfo->pRUDBInfo;

    ET9U8 ET9FARDATA   *pEntry;
    ET9U16              wOffset;
    ET9UINT             nRecType;
    ET9U32              dwSize = 0;

    *pdwTotalWords = 0;

    if (pRUDB == NULL) {
        return 0;
    }

    wOffset = pRUDB->wSizeOffset[0];

    pEntry = ET9RUDBData(pRUDB) + wOffset;

    /* loop through UDB records */

    do {

        nRecType = _ET9AWGetRecordType(pEntry);
        switch(nRecType) {
            case ET9UDBTYPE:
            case ET9RDBTYPE:
            {

                /* determine size of the exported record */

                dwSize = dwSize + __ET9AWGetTRUDBRecLen(pRUDB, pEntry);
                (*pdwTotalWords)++;
                break;
            }

            default:
                break;
        }

        wOffset = (ET9U16)(wOffset + _ET9AWGetRecordLength(pRUDB, pEntry));
        pEntry += _ET9AWGetRecordLength(pRUDB, pEntry);
        __CheckForWrap_ByteP(pEntry, pRUDB);

    } while (pEntry != (ET9RUDBData(pRUDB) + pRUDB->wSizeOffset[0]));

    return dwSize;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Export RUDB to TUDB.
 *
 * @param pLingInfo              Pointer to ET9 information structure.
 * @param pTUdb                  Pointer to target TUDB.
 * @param dwTUdbSize             Size of TUDB.
 * @param pdwExportSize          Size of TUDB data actually written.
 * @param ET9WriteTUDB           Function pointer to OEM TUDB write routine.
 * @param pNextRecord            Pointer to structure to cache last exported record into.
 * @param pwRecordsExported      Pointer to total number of records exported.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL _ET9AWRUDBExport(ET9AWLingInfo       *pLingInfo,
                                      ET9U8 ET9FARDATA    *pTUdb,
                                      ET9U32               dwTUdbSize,
                                      ET9U32              *pdwExportSize,
                                      ET9WriteTUDB_f       ET9WriteTUDB,
                                      ET9AWTUDBNextRecord *pNextRecord,
                                      ET9U16              *pwRecordsExported)
{
    ET9AWLingCmnInfo * const pLingCmnInfo = pLingInfo->pLingCmnInfo;
    ET9AWRUDBInfo ET9FARDATA * const pRUDB = pLingCmnInfo->pRUDBInfo;

    ET9STATUS           wStatus = ET9STATUS_NONE;
    ET9U8 ET9FARDATA   *pEntry, *pbByte;
    ET9U8 ET9FARDATA   *pSymb = NULL;
    ET9U16              wOffset;
    ET9UINT             nRecType;
    ET9U16              wTUdbOffset = 0;
    ET9U16              wWordLen = 0;
    ET9INT              nSize;
    ET9U8               byBuffer[4];
    ET9U16              wBuffer = 0;
    ET9SYMB             sSymb;
    ET9U16              i;

    *pdwExportSize = 0;
    *pwRecordsExported = 0;

    if (pRUDB == NULL) {
        return ET9STATUS_NONE;
    }

    /* if no udb words in UDB, return */

    if (!(pRUDB->wUDBWordCount + pRUDB->wRDBWordCount)) {
        return ET9STATUS_NONE;
    }

    if ((pNextRecord->bDBType == ET9_TUDBTYPE_RUDB) && (pNextRecord->wDBOffset >= pRUDB->wDataSize)) {
        return ET9STATUS_BAD_PARAM;
    }

    /* now set next record for exporting */

    if (pNextRecord->bDBType == ET9_TUDBTYPE_RUDB) {

        /*
         * start from the top, scan forward until the current record is >= dboffset.
         * if it is the same offset, jump to next record and continue.
         * if it is greater offset, then use this a next record
         */

        wOffset = pRUDB->wSizeOffset[0];
        pEntry = ET9RUDBData(pRUDB) + wOffset;

        while (wOffset <= pNextRecord->wDBOffset) {

            wOffset = (ET9U16)(wOffset + _ET9AWGetRecordLength(pRUDB, pEntry));
            pEntry += _ET9AWGetRecordLength(pRUDB, pEntry);
            __CheckForWrap_ByteP(pEntry, pRUDB);

            if (pEntry == (ET9RUDBData(pRUDB) + pRUDB->wSizeOffset[0])) {
                return ET9STATUS_NONE;
            }
        }
    }
    else {
        wOffset = pRUDB->wSizeOffset[0];
    }

    pEntry = ET9RUDBData(pRUDB) + wOffset;

    /* loop through UDB records */

    do {

        nRecType = _ET9AWGetRecordType(pEntry);

        switch(nRecType) {

            case ET9UDBTYPE:
            case ET9RDBTYPE:
            {
                /* determine size of the exported record */

                nSize = __ET9AWGetTRUDBRecLen(pRUDB, pEntry);

                /* if there is not enough room to export this record, end export */

                if ((ET9U32)(wTUdbOffset + nSize) > dwTUdbSize) {
                    pNextRecord->bDBType = ET9_TUDBTYPE_RUDB;
                    pNextRecord->wDBOffset = wOffset;
                    *pdwExportSize = wTUdbOffset;
                    return ET9STATUS_NONE;
                }
                *pdwExportSize = (ET9U32)(*pdwExportSize + nSize);

                /* write record header - record type */

                byBuffer[0] = (nRecType == ET9UDBTYPE) ? ET9TUDB_CUSTOMWORD_REC : ET9TUDB_REORDERWORD_REC;
                wStatus = _ET9TUdbWriteData(byBuffer, 1, pTUdb, wTUdbOffset++, ET9WriteTUDB);
                _ET9CHECKGENSTATUS();

                /* record body size does not include record header */
                /* write record header - Record body size */

                wStatus = _ET9TUdbWriteWord((ET9U16)(nSize-3), pTUdb, wTUdbOffset, ET9WriteTUDB);
                _ET9CHECKGENSTATUS();
                wTUdbOffset += 2;

                /* write word length attribute */

                byBuffer[0] = (nRecType == ET9UDBTYPE) ? ET9TUDB_CUSTOMWORD_LEN : ET9TUDB_REORDERWORD_LEN;
                wStatus = _ET9TUdbWriteData(byBuffer, 1, pTUdb, wTUdbOffset++, ET9WriteTUDB);
                _ET9CHECKGENSTATUS();

                if (nRecType == ET9UDBTYPE) {
                    wWordLen = _ET9AWGetUDBWordLen(pEntry);

                    wStatus = _ET9TUdbWriteWord(2, pTUdb, wTUdbOffset, ET9WriteTUDB);
                    _ET9CHECKGENSTATUS();
                    wTUdbOffset += 2;

                    wStatus = _ET9TUdbWriteWord(wWordLen, pTUdb, wTUdbOffset, ET9WriteTUDB);
                    _ET9CHECKGENSTATUS();
                    wTUdbOffset += 2;

                }
                else {
                    wWordLen = byBuffer[0] = (ET9U8)_ET9AWGetRDBWordLen(pEntry);

                    wStatus = _ET9TUdbWriteWord(1, pTUdb, wTUdbOffset, ET9WriteTUDB);
                    _ET9CHECKGENSTATUS();
                    wTUdbOffset += 2;

                    wStatus = _ET9TUdbWriteData(byBuffer, 1, pTUdb, wTUdbOffset++, ET9WriteTUDB);
                    _ET9CHECKGENSTATUS();
                }

                /* write word frequency attribute */

                byBuffer[0] = (nRecType == ET9UDBTYPE) ? ET9TUDB_CUSTOMWORD_FREQ : ET9TUDB_REORDERWORD_FREQ;
                wStatus = _ET9TUdbWriteData(byBuffer, 1, pTUdb, wTUdbOffset++, ET9WriteTUDB);
                _ET9CHECKGENSTATUS();

                wStatus = _ET9TUdbWriteWord(2, pTUdb, wTUdbOffset, ET9WriteTUDB);
                _ET9CHECKGENSTATUS();
                wTUdbOffset += 2;

                wStatus = _ET9TUdbWriteData(
                    _ET9AWMoveRUDBPtrRight(pRUDB, pEntry, 1), 2, pTUdb, wTUdbOffset, ET9WriteTUDB);
                _ET9CHECKGENSTATUS();
                wTUdbOffset += 2;

                if (nRecType == ET9RDBTYPE) {

                    /* write LDB number attribute */

                    byBuffer[0] = ET9TUDB_REORDERWORD_LDBNUM;
                    wStatus = _ET9TUdbWriteData(byBuffer, 1, pTUdb, wTUdbOffset++, ET9WriteTUDB);
                    _ET9CHECKGENSTATUS();

                    wStatus = _ET9TUdbWriteWord(2, pTUdb, wTUdbOffset, ET9WriteTUDB);
                    _ET9CHECKGENSTATUS();
                    wTUdbOffset += 2;

                    pbByte = _ET9AWMoveRUDBPtrRight(pRUDB, pEntry, 3);
                    byBuffer[0] = *pbByte;
                    wStatus = _ET9TUdbWriteWord((ET9U16)byBuffer[0], pTUdb, wTUdbOffset, ET9WriteTUDB);
                    _ET9CHECKGENSTATUS();
                    wTUdbOffset += 2;
                }

                /* write word symbols attribute */

                byBuffer[0] = (nRecType == ET9UDBTYPE) ? ET9TUDB_CUSTOMWORD_SYMBOLS : ET9TUDB_REORDERWORD_SYMBOLS;
                wStatus = _ET9TUdbWriteData(byBuffer, 1, pTUdb, wTUdbOffset++, ET9WriteTUDB);
                _ET9CHECKGENSTATUS();

                /* determine size of the word symbols */

                if (nRecType == ET9UDBTYPE) {
                    pSymb = _ET9AWMoveRUDBPtrRight(pRUDB, pEntry, UDB_RECORD_HEADER_SIZE);
                    wBuffer = (ET9U16)(nSize - ET9_TUDBRECFIXLEN);
                }
                else {
                    pSymb = _ET9AWMoveRUDBPtrRight(pRUDB, pEntry, RDB_RECORD_HEADER_SIZE);
                    wBuffer = (ET9U16)(nSize - ET9_TRDBRECFIXLEN);
                }

                wStatus = _ET9TUdbWriteWord(wBuffer, pTUdb, wTUdbOffset, ET9WriteTUDB);
                _ET9CHECKGENSTATUS();
                wTUdbOffset += 2;

                for (i = wWordLen; i; --i) {
                    pbByte = (ET9U8 ET9FARDATA *)pSymb + 1; /* point to second byte */
                    __CheckForWrap_ByteP(pbByte, pRUDB);
                    sSymb = ET9MAKEWORD(*pSymb, *pbByte);
                    pSymb = _ET9AWMoveRUDBPtrRight(pRUDB, pSymb, ET9SYMBOLWIDTH);
                    nSize = _ET9SymbToUtf8(sSymb, byBuffer);
                    wStatus = _ET9TUdbWriteData(byBuffer, (ET9U16)nSize, pTUdb, wTUdbOffset, ET9WriteTUDB);
                    _ET9CHECKGENSTATUS();
                    wTUdbOffset = (ET9U16)(wTUdbOffset + nSize);
                }
                (*pwRecordsExported)++;
                break;
            }

            default:
                break;

        }

        wOffset = (ET9U16)(wOffset + _ET9AWGetRecordLength(pRUDB, pEntry));
        pEntry += _ET9AWGetRecordLength(pRUDB, pEntry);
        __CheckForWrap_ByteP(pEntry, pRUDB);

    } while (pEntry != (ET9RUDBData(pRUDB) + pRUDB->wSizeOffset[0]));

    return wStatus;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Import UDB.
 *
 * @param pLingInfo          Pointer to ET9 information structure.
 * @param pTUdb              Pointer to source TUDB.
 * @param ET9ReadTUDB        Function pointer to OEM TUDB read routine.
 * @param dwTUdbOffset       Offset to TUDB for the record.
 * @param wRecSize           Size of record minus header.
 * @param pbImported         Pointer to status of imported.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL _ET9AWUDBImport(ET9AWLingInfo        *pLingInfo,
                                      ET9U8 ET9FARDATA    *pTUdb,
                                      ET9ReadTUDB_f        ET9ReadTUDB,
                                      ET9U32               dwTUdbOffset,
                                      ET9U16               wRecSize,
                                      ET9U8               *pbImported)
{
    ET9AWLingCmnInfo * const pLingCmnInfo = pLingInfo->pLingCmnInfo;
    ET9AWRUDBInfo ET9FARDATA * const pRUDB = pLingCmnInfo->pRUDBInfo;

    ET9STATUS  wStatus = ET9STATUS_NONE;
    ET9U16  wRecOffset = 0;
    ET9U8   byBuffer[4];
    ET9U16  i;
    ET9U8   byAttrType;
    ET9U16  wAttrSize;
    ET9U16  wToRead = 4;
    ET9U8   bSymbolSize;
    ET9U16  wWordLen = 0;
    ET9U16  wWordFreq = 256;
    ET9SYMB sWord[ET9MAXUDBWORDSIZE];
    ET9U8   bRetrievedWordSymbs = 0;

    *pbImported = 0;

    if (pRUDB == NULL) {
        return ET9STATUS_NONE;
    }

    while ((wRecOffset + 3) < wRecSize) {

        /* get attribute type */

        wStatus = _ET9TUdbReadData(&byAttrType, 1, pTUdb, dwTUdbOffset + wRecOffset++, ET9ReadTUDB);
        _ET9CHECKGENSTATUS();

        /* get attribute size */

        wStatus = _ET9TUdbReadWord(&wAttrSize, pTUdb, dwTUdbOffset + wRecOffset, ET9ReadTUDB);
        _ET9CHECKGENSTATUS();
        wRecOffset += 2;

        if ((wRecOffset + wAttrSize) > wRecSize) {
            return ET9STATUS_NONE;
        }

        switch(byAttrType)
        {
            case ET9TUDB_CUSTOMWORD_LEN:
                if (wAttrSize != 2) {
                    return ET9STATUS_NONE;
                }
                wStatus = _ET9TUdbReadWord(&wWordLen, pTUdb, dwTUdbOffset + wRecOffset, ET9ReadTUDB);
                _ET9CHECKGENSTATUS();
                if (wWordLen > ET9MAXUDBWORDSIZE) {
                    return ET9STATUS_NONE;
                }
                break;

            case ET9TUDB_CUSTOMWORD_FREQ:
                if (wAttrSize != 2) {
                    return ET9STATUS_NONE;
                }
                wStatus = _ET9TUdbReadWord(&wWordFreq, pTUdb, dwTUdbOffset + wRecOffset, ET9ReadTUDB);
                _ET9CHECKGENSTATUS();
                break;

            case ET9TUDB_CUSTOMWORD_SYMBOLS:
                {
                    i = 0;
                    while (wAttrSize) {
                        wToRead = (wAttrSize > 4) ? 4 : wAttrSize;
                        wStatus = _ET9TUdbReadData(byBuffer, wToRead, pTUdb, dwTUdbOffset + wRecOffset, ET9ReadTUDB);
                        _ET9CHECKGENSTATUS();
                        bSymbolSize = _ET9Utf8ToSymb(byBuffer, NULL, &sWord[i++]);
                        if (! bSymbolSize) {
                            return ET9STATUS_NONE;
                        }
                        wAttrSize = (ET9U16)(wAttrSize - bSymbolSize);
                        wRecOffset = (ET9U16)(wRecOffset + bSymbolSize);
                    }
                    if (i != wWordLen) {
                        return ET9STATUS_NONE;
                    }
                    bRetrievedWordSymbs = 1;
                    break;
                }

            case ET9TUDB_CUSTOMWORD_INVALID:
                return ET9STATUS_NONE;

            default:
                break;

        }

        wRecOffset = (ET9U16)(wRecOffset + wAttrSize);
    }

    /* validate that minimum data are there */

    if (!wWordLen || !bRetrievedWordSymbs) {

        /* REJECT */

        return ET9STATUS_NONE;
    }

    /* if the word doesn't already exist in the UDB */

    if (!__ET9UDBSetHigherFreq(pLingInfo, sWord, wWordLen, wWordFreq)) {
        wStatus = _ET9AWGeneralUDBAddWord(pLingInfo, sWord, wWordLen, wWordFreq);
        _ET9CHECKGENSTATUS();
    }

    *pbImported = 1;

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Import RDB.
 *
 * @param pLingInfo          Pointer to ET9 information structure.
 * @param pTUdb              Pointer to source TUDB.
 * @param ET9ReadTUDB        Function pointer to OEM TUDB read routine.
 * @param dwTUdbOffset       Offset to TUDB for the record.
 * @param wRecSize           Size of record minus header.
 * @param pbImported         Pointer to status of imported.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL _ET9AWRDBImport(ET9AWLingInfo      *pLingInfo,
                                     ET9U8 ET9FARDATA   *pTUdb,
                                     ET9ReadTUDB_f       ET9ReadTUDB,
                                     ET9U32              dwTUdbOffset,
                                     ET9U16              wRecSize,
                                     ET9U8              *pbImported)
{
    ET9AWLingCmnInfo * const pLingCmnInfo = pLingInfo->pLingCmnInfo;
    ET9AWRUDBInfo ET9FARDATA * const pRUDB = pLingCmnInfo->pRUDBInfo;

    ET9STATUS wStatus = ET9STATUS_NONE;
    ET9AWPrivWordInfo PrivWord;
    ET9U16  wRecOffset = 0;
    ET9U8   byBuffer[4];
    ET9U16  i;
    ET9U8   byAttrType;
    ET9U16  wAttrSize;
    ET9U16  wToRead = 4;
    ET9U8   bSymbolSize;
    ET9U16  wWordFreq = 0;
    ET9U16  wLdbNum = 0;
    ET9U8   bRetrievedWordSymbs = 0;

    *pbImported = 0;

    if (! pRUDB) {
        return ET9STATUS_NONE;
    }

    PrivWord.Base.wWordLen = 0;

    while ((wRecOffset + 3) < wRecSize) {

        /* get attribute type */

        wStatus = _ET9TUdbReadData(&byAttrType, 1, pTUdb, dwTUdbOffset + wRecOffset++, ET9ReadTUDB);
        _ET9CHECKGENSTATUS();

        /* get attribute size */

        wStatus = _ET9TUdbReadWord(&wAttrSize, pTUdb, dwTUdbOffset + wRecOffset, ET9ReadTUDB);
        _ET9CHECKGENSTATUS();
        wRecOffset += 2;

        if ((wRecOffset + wAttrSize) > wRecSize) {
            return ET9STATUS_NONE;
        }

        switch(byAttrType)
        {
            case ET9TUDB_REORDERWORD_LEN:
                if (wAttrSize != 1) {
                    return ET9STATUS_NONE;
                }
                wStatus = _ET9TUdbReadData(byBuffer, 1, pTUdb, dwTUdbOffset + wRecOffset, ET9ReadTUDB);
                _ET9CHECKGENSTATUS();
                PrivWord.Base.wWordLen = byBuffer[0];
                if (PrivWord.Base.wWordLen > ET9MAXUDBWORDSIZE) {
                    return ET9STATUS_NONE;
                }
                break;

            case ET9TUDB_REORDERWORD_FREQ:
                if (wAttrSize != 2) {
                    return ET9STATUS_NONE;
                }
                wStatus = _ET9TUdbReadWord(&wWordFreq, pTUdb, dwTUdbOffset + wRecOffset, ET9ReadTUDB);
                _ET9CHECKGENSTATUS();
                break;

            case ET9TUDB_REORDERWORD_LDBNUM:
                if (wAttrSize != 2) {
                    return ET9STATUS_NONE;
                }
                wStatus = _ET9TUdbReadWord(&wLdbNum, pTUdb, dwTUdbOffset + wRecOffset, ET9ReadTUDB);
                _ET9CHECKGENSTATUS();
                break;

            case ET9TUDB_REORDERWORD_SYMBOLS:
                {
                    i = 0;
                    while(wAttrSize) {
                        wToRead = (wAttrSize > 4) ? 4 : wAttrSize;
                        wStatus = _ET9TUdbReadData(byBuffer, wToRead, pTUdb, dwTUdbOffset + wRecOffset, ET9ReadTUDB);
                        _ET9CHECKGENSTATUS();
                        bSymbolSize = _ET9Utf8ToSymb(byBuffer, NULL, &PrivWord.Base.sWord[i++]);
                        if (! bSymbolSize) {
                            return ET9STATUS_NONE;
                        }
                        wAttrSize = (ET9U16)(wAttrSize - bSymbolSize);
                        wRecOffset = (ET9U16)(wRecOffset + bSymbolSize);
                    }
                    if (i != PrivWord.Base.wWordLen) {
                        return ET9STATUS_NONE;
                    }
                    bRetrievedWordSymbs = 1;
                    break;
                }

            case ET9TUDB_CUSTOMWORD_INVALID:
                return ET9STATUS_NONE;

            default:
                break;

        }
        wRecOffset = (ET9U16)(wRecOffset + wAttrSize);
    }

    /* verify that minimum data are there */

    if (!wLdbNum || (PrivWord.Base.wWordLen <= 1) || !wWordFreq || !bRetrievedWordSymbs) {

        /* reject */

        return ET9STATUS_NONE;
    }

    /* if the word doesn't already exist in the RDB */

    if (!__ET9RDBSetHigherFreq(pLingInfo, &PrivWord, wWordFreq)) {

        /* add 'as is' by setting Exact = 1, lowercase = 0 */

        wStatus = _ET9AWRDBAddWord(pLingInfo, &PrivWord, wWordFreq, (ET9U8)(wLdbNum & ET9PLIDMASK), 1, 0);
        _ET9CHECKGENSTATUS();
    }

    *pbImported = 1;

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Import UDB from a T9-generated TUDB.
 *
 * @param pLingInfo          Pointer to ET9 information structure.
 * @param pTUdb              Pointer to source TUDB.
 * @param ET9ReadTUDB        Function pointer to OEM TUDB read routine.
 * @param dwTUdbOffset       Offset to TUDB for the record.
 * @param wRecSize           Size of record minus header.
 * @param pbImported         Pointer to status of imported.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL _T9UDBImport(ET9AWLingInfo     *pLingInfo,
                                  ET9U8 ET9FARDATA  *pTUdb,
                                  ET9ReadTUDB_f      ET9ReadTUDB,
                                  ET9U32             dwTUdbOffset,
                                  ET9U16             wRecSize,
                                  ET9U8             *pbImported)
{
    ET9AWLingCmnInfo * const pLingCmnInfo = pLingInfo->pLingCmnInfo;
    ET9AWRUDBInfo ET9FARDATA * const pRUDB = pLingCmnInfo->pRUDBInfo;

    ET9STATUS wStatus = ET9STATUS_NONE;
    ET9U16   wRecOffset = 0;
    ET9U8    byBuffer[4];
    ET9U16   i;
    ET9U8    byAttrType;
    ET9U16   wAttrSize;
    ET9U16   wToRead = 4;
    ET9U8    bSymbolSize;
    ET9U16   wWordLen = 0;
    ET9U16   wWordFreq = 0;
    ET9SYMB  sWord[ET9MAXUDBWORDSIZE];
    ET9U8    bRetrievedWordSymbs = 0;

    *pbImported = 0;

    if (pRUDB == NULL) {
        return ET9STATUS_NONE;
    }

    while ((wRecOffset + 3) < wRecSize) {

        /* get attribute type */

        wStatus = _ET9TUdbReadData(&byAttrType, 1, pTUdb, dwTUdbOffset + wRecOffset++, ET9ReadTUDB);
        _ET9CHECKGENSTATUS();

        /* get attribute size */

        wStatus = _ET9TUdbReadWord(&wAttrSize, pTUdb, dwTUdbOffset + wRecOffset, ET9ReadTUDB);
        _ET9CHECKGENSTATUS();
        wRecOffset += 2;

        if ((wRecOffset + wAttrSize) > wRecSize) {
            return ET9STATUS_NONE;
        }

        switch(byAttrType)
        {
            case ET9TUDB_T9CUSTOMWORD_LEN:
                if (wAttrSize != 2) {
                    return ET9STATUS_NONE;
                }
                wStatus = _ET9TUdbReadWord(&wWordLen, pTUdb, dwTUdbOffset + wRecOffset, ET9ReadTUDB);
                _ET9CHECKGENSTATUS();
                if (wWordLen > ET9MAXUDBWORDSIZE) {
                    return ET9STATUS_NONE;
                }
                wRecOffset = (ET9U16)(wRecOffset + wAttrSize);
                break;

            case ET9TUDB_T9CUSTOMWORD_FREQ:
                if (wAttrSize != 2) {
                    return ET9STATUS_NONE;
                }
                wStatus = _ET9TUdbReadWord(&wWordFreq, pTUdb, dwTUdbOffset + wRecOffset, ET9ReadTUDB);
                _ET9CHECKGENSTATUS();
                wRecOffset = (ET9U16)(wRecOffset + wAttrSize);
                break;

            case ET9TUDB_T9CUSTOMWORD_SYMBOLS:
                {
                    i = 0;
                    while (wAttrSize) {
                        wToRead = (wAttrSize > 4) ? 4 : wAttrSize;
                        wStatus = _ET9TUdbReadData(byBuffer, wToRead, pTUdb, dwTUdbOffset + wRecOffset, ET9ReadTUDB);
                        _ET9CHECKGENSTATUS();
                        bSymbolSize = _ET9Utf8ToSymb(byBuffer, NULL, &sWord[i++]);
                        if (! bSymbolSize) {
                            return ET9STATUS_NONE;
                        }
                        wAttrSize = (ET9U16)(wAttrSize - bSymbolSize);
                        wRecOffset = (ET9U16)(wRecOffset + bSymbolSize);
                    }
                    if (i != wWordLen) {
                        return ET9STATUS_NONE;
                    }
                    bRetrievedWordSymbs = 1;
                    break;
                }

            case ET9TUDB_T9CUSTOMWORD_INVALID:
                return ET9STATUS_NONE;

            case ET9TUDB_T9CUSTOMWORD_KEYS: /* skip importing key sequence */
            default:
                wRecOffset = (ET9U16)(wRecOffset + wAttrSize);
                break;

        }
    }

    if (! wWordLen || !bRetrievedWordSymbs) {

        /* reject */

        return ET9STATUS_NONE;
    }

    wStatus = _ET9AWGeneralUDBAddWord(pLingInfo, sWord, wWordLen, wWordFreq);

    _ET9CHECKGENSTATUS();

    *pbImported = 1;

    return ET9STATUS_NONE;
}


#endif /* ET9_ALPHABETIC_MODULE */
/*! @} */
/* ----------------------------------< eof >--------------------------------- */
