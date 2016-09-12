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
;**     FileName: et9aasdb.c                                                  **
;**                                                                           **
;**  Description: AutoSubstitution data base access routines source file.     **
;**                                                                           **
;*******************************************************************************
;******************************************************************************/

/*! \addtogroup et9as Auto substitution for alphabetic
* Alphabetic auto substitution features.
* @{
*/

#include "et9api.h"

#ifdef ET9_ALPHABETIC_MODULE

#include "et9asys.h"
#include "et9adb.h"
#include "et9aasdb.h"
#include "et9amisc.h"
#include "et9sym.h"
#include "et9alsasdb.h"
#include "et9aspc.h"

#define __CheckForASDBWrap_ByteP(pbPtr, pASDB)                                  \
{                                                                               \
    if (pbPtr >= (ET9ASDBData(pASDB) + ET9ASDBDataAreaBytes(pASDB))) {          \
        pbPtr -= ET9ASDBDataAreaBytes(pASDB);                                   \
    }                                                                           \
}


#define __CheckForASDBWrap_WordP(pwPtr, pASDB)                                                  \
{                                                                                               \
    if ((ET9U8 ET9FARDATA *)pwPtr >= (ET9ASDBData(pASDB) + ET9ASDBDataAreaBytes(pASDB))) {      \
        pwPtr = (ET9U16 ET9FARDATA *)((ET9U8 ET9FARDATA *)pwPtr - ET9ASDBDataAreaBytes(pASDB)); \
    }                                                                                           \
}

#define ALDB_FIRST_LDBNUM         (pLingInfo->pLingCmnInfo->wFirstLdbNum)    /**< \internal  Convenience data access for first wldbnum. */
#define ALDB_SECOND_LDBNUM        (pLingInfo->pLingCmnInfo->wSecondLdbNum)   /**< \internal  Convenience data access for second wldbnum. */

/*---------------------------------------------------------------------------*/
/** \internal
 * Move the byte pointer to the right, and circle back to the beginning if at or past the end of the database.
 *
 * @param pASDB          Pointer to ASDB.
 * @param pbNext         Position to move from.
 * @param nNumMoves      Number of bytes to move.
 *
 * @return New pointer location.
 */

static ET9U8 ET9FARDATA * ET9LOCALCALL __ET9AWMoveASDBPtrRight(ET9AWASDBInfo ET9FARDATA *pASDB,
                                                         const ET9U8 ET9FARDATA         *pbNext,
                                                               ET9UINT                   nNumMoves)
{
    ET9Assert(pASDB != NULL);
    ET9Assert(pbNext);

    pbNext += nNumMoves;
    __CheckForASDBWrap_ByteP(pbNext, pASDB);
    return (ET9U8 ET9FARDATA *)pbNext;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Handles ALL writes to ASDB memory.
 *
 * @param pLingInfo      Pointer to field information structure.
 * @param pTo            Where in memory write begins.
 * @param pFrom          Location of data to write.
 * @param nSize          How much data.
 *
 * @return None
 */

static void ET9LOCALCALL __ET9AWWriteASDBData(ET9AWLingInfo *pLingInfo,
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

    /* If OEM is handling ASDB writes... */
    if (pLingInfo->pASDBWriteData != NULL) {
        pLingInfo->pASDBWriteData(pLingInfo, pbTo, pbFrom, nSize);
    }
    /* Else write data locally */
    else {
        while (nSize--) {
            ET9Assert(pbTo >= (ET9U8*)pLingInfo->pLingCmnInfo->pASDBInfo);
            ET9Assert(pbTo < (ET9U8*)(pLingInfo->pLingCmnInfo->pASDBInfo + pLingInfo->pLingCmnInfo->pASDBInfo->wDataSize) || !pLingInfo->pLingCmnInfo->pASDBInfo->wDataSize);
            *pbTo-- = *pbFrom--;
        }
    }
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Copy ASDB strings in memory.
 * Include logic to handle possibilty that copy regions overlap the end of the database.
 *
 * @param pLingInfo      Pointer to field information structure.
 * @param pTo            Where in memory copy begins.
 * @param pFrom          Location of data to copy.
 * @param nSize          How much data.
 *
 * @return None
 */

static void ET9LOCALCALL __ET9AWCopyASDBString(ET9AWLingInfo    *pLingInfo,
                                               ET9U8 ET9FARDATA *pTo,
                                               ET9U8 ET9FARDATA *pFrom,
                                               ET9UINT           nSize)
{
    ET9UINT   nToLap, nFromLap, nSizeLeft;
    ET9AWASDBInfo ET9FARDATA * const pASDB = pLingInfo->pLingCmnInfo->pASDBInfo;

    ET9Assert(pLingInfo);
    ET9Assert(pTo);
    ET9Assert(pFrom);
    ET9Assert(nSize);
    ET9Assert(pASDB != NULL);

    nToLap = (ET9UINT)(((pTo + nSize) >= (ET9ASDBData(pASDB) + ET9ASDBDataAreaBytes(pASDB))) ?
                            (pTo + nSize - (ET9ASDBData(pASDB) + ET9ASDBDataAreaBytes(pASDB))) : 0);

    nFromLap = (ET9UINT)(((pFrom + nSize) >= (ET9ASDBData(pASDB) + ET9ASDBDataAreaBytes(pASDB))) ?
                            (pFrom + nSize - (ET9ASDBData(pASDB) + ET9ASDBDataAreaBytes(pASDB))) : 0);

    nSizeLeft = nSize;
    if (nFromLap) {
        __ET9AWWriteASDBData(pLingInfo, (void ET9FARDATA*)__ET9AWMoveASDBPtrRight(pASDB, pTo, nSizeLeft - nFromLap),
                     (const void ET9FARDATA *)__ET9AWMoveASDBPtrRight(pASDB, pFrom, nSizeLeft - nFromLap), nFromLap);
        nSizeLeft = nSizeLeft - nFromLap;
    }
    if (nToLap) { /* if there is a tolap, its always greater than fromlap */
        nToLap = nToLap - nFromLap;
        __ET9AWWriteASDBData(pLingInfo, (void ET9FARDATA*)__ET9AWMoveASDBPtrRight(pASDB, pTo, nSizeLeft - nToLap),
                     (const void ET9FARDATA *)__ET9AWMoveASDBPtrRight(pASDB, pFrom, nSizeLeft - nToLap), nToLap);
        nSizeLeft = nSizeLeft - nToLap;
    }

    if (nSizeLeft) {
        __ET9AWWriteASDBData(pLingInfo, (void ET9FARDATA*)pTo, (const void ET9FARDATA *)pFrom, nSizeLeft);
    }
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Construct and write an ASDB record header (based on record type).
 * NOTE: This function assumes that the 'pbDest' pointer is valid. The caller should validate the pointer.
 *
 * @param pLingInfo          Pointer to field information structure.
 * @param pbDest             Byte stream to write to.
 * @param nType              Type of header to write.
 * @param nShortcutLen       Length:  in bytes for free type and in symbol length of shortcut word for ASDB type.
 * @param nSubstitutionLen   Length:  0 for free type and in symbol length of substitution word(s) for ASDB type.
 * @param wCheckSum          Checksum (updated checksum us returned by function).
 *
 * @return New checksum.
 */

static ET9U16 ET9LOCALCALL __ET9AWASDBWriteHeader(ET9AWLingInfo    *pLingInfo,
                                                  ET9U8 ET9FARDATA *pbDest,
                                                  ET9UINT           nType,
                                                  ET9UINT           nShortcutLen,
                                                  ET9UINT           nSubstitutionLen,
                                                  ET9U16            wCheckSum)
{
    ET9U16 wTemp;
    ET9U8  bTemp;
    ET9U8  bTemp2;
    ET9AWASDBInfo ET9FARDATA * const pASDB = pLingInfo->pLingCmnInfo->pASDBInfo;

    ET9Assert(pLingInfo);
    ET9Assert(pbDest);
    ET9Assert(pASDB != NULL);
    ET9Assert(pbDest >= (ET9U8*)ET9ASDBData(pASDB));
    ET9Assert(pbDest < (ET9U8*)(ET9ASDBData(pASDB) + ET9ASDBDataAreaBytes(pASDB)));

    /* if record is ASDB entry */
    if (nType == ET9ASDBTYPE) {
        /* make sure the shortcut and substitution sizes are ok (debug sanity check) */
        ET9Assert(nShortcutLen <= ET9MAXWORDSIZE);
        ET9Assert(nSubstitutionLen <= ET9MAXWORDSIZE);
        /* prepare the record header with the status byte */
        bTemp = (ET9U8)ET9ASDBTYPEMASK;
        wCheckSum = (ET9U16)(wCheckSum + bTemp);
        __ET9AWWriteASDBData(pLingInfo, (void ET9FARDATA*)pbDest, (const void ET9FARDATA *)&bTemp, 1 );
        /* now write the unused second header byte */
        ++pbDest;
        __CheckForASDBWrap_ByteP(pbDest, pASDB);
        bTemp = (ET9U8)0;
        __ET9AWWriteASDBData(pLingInfo, (void ET9FARDATA*)pbDest, (const void ET9FARDATA *)&bTemp, 1 );
        /* now write the shortcut length (in symbols)... 3rd byte */
        ++pbDest;
        __CheckForASDBWrap_ByteP(pbDest, pASDB);
        bTemp = (ET9U8)(nShortcutLen);
        wCheckSum = (ET9U16)(wCheckSum + bTemp);
        __ET9AWWriteASDBData(pLingInfo, (void ET9FARDATA*)pbDest, (const void ET9FARDATA *)&bTemp, 1 );
        /* and the substitution length (in symbols)... 4th byte */
        ++pbDest;
        __CheckForASDBWrap_ByteP(pbDest, pASDB);
        bTemp = (ET9U8)(nSubstitutionLen);
        wCheckSum = (ET9U16)(wCheckSum + bTemp);
        __ET9AWWriteASDBData(pLingInfo, (void ET9FARDATA*)pbDest, (const void ET9FARDATA *)&bTemp, 1 );
    }
    /* if this is a 1-byte free 'record' */
    else if (nType == ET9ASSINGLEFREETYPE) {
        /* mark unique single byte */
        bTemp = (ET9U8)ET9SINGLEFREETYPEMASK;
        wCheckSum = (ET9U16)(wCheckSum + bTemp);
        __ET9AWWriteASDBData(pLingInfo, (void ET9FARDATA*)pbDest, (const void ET9FARDATA *)&bTemp, 1 );
    }
    /* if this is a multi-byte free record */
    else if (nType == ET9ASMULTIFREETYPE) {
        /* make sure size doesn't exceed number of bits allocated for size */
        ET9Assert(nShortcutLen <= 0x1FFF);
        /* construct the header */
        wTemp = (ET9U16)((ET9MULTIFREETYPEMASK << 8) + nShortcutLen);
        /* separate for endianness */
        bTemp = ET9HIBYTE(wTemp);
        bTemp2 = ET9LOBYTE(wTemp);
        wCheckSum = (ET9U16)(wCheckSum + bTemp + bTemp2);
        /* write upper byte */
        __ET9AWWriteASDBData(pLingInfo, (void ET9FARDATA*)pbDest, (const void ET9FARDATA *)&bTemp, 1);
        ++pbDest;
        __CheckForASDBWrap_ByteP(pbDest, pASDB);
        /* write lower byte */
        __ET9AWWriteASDBData(pLingInfo, (void ET9FARDATA*)pbDest, (const void ET9FARDATA *)&bTemp2, 1);
    }
    /* catch for unrecognized record type */
    else {
        ET9Assert(0);
    }

    return wCheckSum;
}


/*---------------------------------------------------------------------------*/
/** \internal
 * Return type of record located at pbReadFrom.
 * This function assumes that the 'pbReadFrom' pointer is valid.
 * The caller should validate the pointer.
 *
 * @param pbReadFrom     Byte stream to read.
 *
 * @return Record type: ET9ASDBTYPE, ET9ASMULTIFREETYPE, ET9ASSINGLEFREETYPE.
 */

static ET9U8 ET9LOCALCALL __ET9AWASGetRecordType(ET9U8 ET9FARDATA *pbReadFrom)
{
    ET9U8 bData;

    ET9Assert(pbReadFrom);

    /* focus on the first 3 bits of record header */
    bData = *pbReadFrom & ET9ASDBRECORDTYPEMASK;
    if (bData == ET9ASDBTYPEMASK) {                 /* 3 MSBits are 100, defined ASDB rec type */
        bData = ET9ASDBTYPE;
    }
    else if (bData == ET9MULTIFREETYPEMASK) {       /* 3 MSBits are 011, multibyte free type   */
        bData = ET9ASMULTIFREETYPE;
    }
    else if (bData == ET9SINGLEFREETYPEMASK) {      /* 3 MSBits are 010, single byte free type */
        bData = ET9ASSINGLEFREETYPE;
    }
    /* bug... should not happen (probably pbReadFrom is pointing at bogus location) */
    else {
        bData = ET9ASDBUNKNOWN;
    }
    return bData;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Read ASDB record header.
 *
 * @param pASDB              Pointer to ASDB.
 * @param pbCurrent          Pointer to record.
 * @param pASDBRecHeader     Struct to read header info into.
 *
 * @return Pointer to position immediately after record header (start of shortcut word).
 */

static ET9U8 ET9FARDATA * ET9LOCALCALL __ET9AWReadASDBRecordHeader(ET9AWASDBInfo ET9FARDATA *pASDB,
                                                                   ET9U8 ET9FARDATA         *pbCurrent,
                                                                   ET9ASDBRecordHeader      *pASDBRecHeader)
{
    ET9U16 i = ASDB_RECORD_HEADER_SIZE;
    /* load struct with byte-by-byte read */
    ET9U8 *pbLoader = (ET9U8 *)pASDBRecHeader;

    ET9Assert(pASDB != NULL);
    ET9Assert(pbCurrent);
    ET9Assert(pASDBRecHeader);
    ET9Assert(pbCurrent >= (ET9U8*)ET9ASDBData(pASDB));
    ET9Assert(pbCurrent < (ET9U8*)(ET9ASDBData(pASDB) + ET9ASDBDataAreaBytes(pASDB)));

    /* do 4 byte read byte-by-byte */
    for (; i; --i) {
        *pbLoader++ = *pbCurrent++;
        __CheckForASDBWrap_ByteP(pbCurrent, pASDB);
    }
    return pbCurrent;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Determine length of record pointed to.
 * This function assumes that the 'pbReadFrom' pointer is valid.
 * The caller should validate the pointer.
 *
 * @param pASDB              Pointer to ASDB information structure.
 * @param pbReadFrom         Byte stream to read.
 * @param pnRecordLength     Pointer to store length of record in bytes.
 *
 * @return Type of record.
 */

static ET9U8 ET9LOCALCALL __ET9AWASGetRecordInfo(ET9AWASDBInfo ET9FARDATA *pASDB,
                                                 ET9U8         ET9FARDATA *pbReadFrom,
                                                 ET9UINT                  *pnRecordLength)
{
    ET9U8   bRecType;
    ET9UINT nLength;
    ET9U8   bData;
    ET9ASDBRecordHeader sASDBRecHeader;

    ET9Assert(pASDB != NULL);
    ET9Assert(pbReadFrom);
    ET9Assert(pnRecordLength);
    ET9Assert(pbReadFrom >= (ET9U8*)ET9ASDBData(pASDB));
    ET9Assert(pbReadFrom < (ET9U8*)(ET9ASDBData(pASDB) + ET9ASDBDataAreaBytes(pASDB)));

    /* get whether ASDB record, multi-byte free record, or single free byte record */
    bRecType = __ET9AWASGetRecordType(pbReadFrom);
    /* if actual AutoSubstitution record */
    if (bRecType == ET9ASDBTYPE) {
        /* calc record length as header len (4) + byte len of shortcut + byte len of substitution */
        __ET9AWReadASDBRecordHeader(pASDB, pbReadFrom, &sASDBRecHeader);
        nLength = ASDB_RECORD_HEADER_SIZE +
           (ET9UINT)((sASDBRecHeader.bShortcutLen + sASDBRecHeader.bSubstitutionLen) << (sizeof(ET9SYMB) - 1));
    }
    /* if multi-byte free record */
    else if (bRecType == ET9ASMULTIFREETYPE) {
        /* calc record length as header len (4) + byte len of shortcut + byte len of substitution */
        bData = *pbReadFrom++;
        /* length is the Least Significant 5 bits of the header byte + entire 2nd byte */
        /* The length value INCLUDES the 2 header bytes */
        bData &= 0x1F;
        __CheckForASDBWrap_ByteP(pbReadFrom, pASDB);
        nLength = (ET9UINT)(bData << 8) + (ET9UINT)*pbReadFrom;
    }
    /* lump both ET9ASSINGLEFREETYPE and ET9ASDBUNKNOWN as single byte entries */
    else {
        nLength = 1;
    }
    /* load the passed length pointer with the calculated value */
    *pnRecordLength = nLength;

    return bRecType;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Determine length of record pointed to.
 * This function assumes that the 'pbReadFrom' pointer is valid.
 * The caller should validate the pointer.
 *
 * @param pASDB              Pointer to ASDB information structure.
 * @param pbReadFrom         Byte stream to read.
 *
 * @return Length of record in bytes.
 */

static ET9UINT ET9LOCALCALL __ET9AWGetASDBRecordLength(ET9AWASDBInfo ET9FARDATA *pASDB,
                                                       ET9U8         ET9FARDATA *pbReadFrom)
{
    ET9UINT nLength;

    ET9Assert(pASDB != NULL);
    ET9Assert(pbReadFrom);
    ET9Assert(pbReadFrom >= (ET9U8*)ET9ASDBData(pASDB));
    ET9Assert(pbReadFrom < (ET9U8*)(ET9ASDBData(pASDB) + ET9ASDBDataAreaBytes(pASDB)));

    __ET9AWASGetRecordInfo(pASDB, pbReadFrom, &nLength);
    return nLength;
}

#ifdef ET9_DEBUG

/*---------------------------------------------------------------------------*/
/** \internal
 * Verify that all size ranges start with a single one byte free record.
 *
 * @param pASDB          Pointer to ASDB information structure.
 *
 * @return None
 */

static void ET9LOCALCALL __ET9AWASDBVerifySingleBytes(ET9AWASDBInfo ET9FARDATA *pASDB)
{

    ET9UINT nIndex, nRecordLength, nRecordType;
    ET9U8 ET9FARDATA *pbAsdbData;

    ET9Assert(pASDB != NULL);

    for (nIndex = 0; nIndex < ET9NUMASDBSIZERANGES; ++nIndex) {
        pbAsdbData = ET9ASDBData(pASDB) + pASDB->wSizeOffset[nIndex];
        nRecordType = __ET9AWASGetRecordInfo(pASDB, pbAsdbData, &nRecordLength);
        ET9Assert(nRecordLength == 1);
        ET9Assert(nRecordType == ET9ASSINGLEFREETYPE);
    }
}

#endif

/*---------------------------------------------------------------------------*/
/** \internal
 * Compute the asdb checksum value.
 * defined as: Checksum(data) + __ET9AWASDBHeaderChecksum(ASDB)
 *
 * @param pLingInfo          Pointer to udb information structure.
 *
 * @return 16 bit ASDB checksum value.
 */

static ET9U16 ET9LOCALCALL __ET9AWGetASDBChecksum(ET9AWLingInfo *pLingInfo)
{
    ET9U8 ET9FARDATA *pbAsdbData;
    ET9U8 ET9FARDATA *pbData;
    ET9U16            wSizeChecked = 0;
    ET9U16            wCheckSum;
    ET9UINT           nRecordLength, nIndex;
    ET9UINT           nRemainingMemory = 0;
    ET9U8             bRecordType;
    ET9UINT           nIndex2;
    ET9LdbASRecMap ET9FARDATA *pLDBMap;
    ET9U16 ET9FARDATA *pLDBTracker;
    ET9U8  ET9FARDATA *pByte;

    ET9AWASDBInfo ET9FARDATA * const pASDB = pLingInfo->pLingCmnInfo->pASDBInfo;

    ET9Assert(pLingInfo);
    ET9Assert(pASDB != NULL);

    /* checksum of the ASDB header */
    wCheckSum = (ET9U16)(pASDB->wDataSize +
                         pASDB->wEntryCount +
                         pASDB->wRemainingMemory);
    for (nIndex = 0; nIndex < ET9NUMASDBSIZERANGES; ++nIndex) {
        wCheckSum = (ET9U16)(wCheckSum + pASDB->wSizeOffset[nIndex]);
    }
    pLDBMap = pASDB->sLdbASRecord;
    pLDBTracker = pASDB->wLDBUseTracker;
    for (nIndex = 0; nIndex < ET9MAXASDBLANGUAGERECORDS; ++nIndex, ++pLDBMap) {
        wCheckSum = (ET9U16)(wCheckSum + *pLDBTracker++ +
            pLDBMap->wLDBID + pLDBMap->wEnabledRecords + pLDBMap->wTotalRecords);
        pByte = pLDBMap->bMap;
        for (nIndex2 = 0; nIndex2 < (ET9MAXLDBSUPPORTEDASRECORDS/8); ++nIndex2) {
            wCheckSum = (ET9U16)(wCheckSum + *pByte++);
        }
    }

    /* start at the beginning of the smallest size entry region */
    pbAsdbData = ET9ASDBData(pASDB) + pASDB->wSizeOffset[0];

    /* loop until entire database is covered */
    /* this is just a safety check to close the loop */
    while (wSizeChecked < ET9ASDBDataAreaBytes(pASDB)) {
        /* get the record type/size */
        bRecordType = __ET9AWASGetRecordInfo(pASDB, pbAsdbData, &nRecordLength);
        /* if bogus length (CR 19696), reset and restart */
        if (!nRecordLength || nRecordLength > ET9ASDBDataAreaBytes(pASDB)) {
            ET9AWASDBReset(pLingInfo);
            return pASDB->wDataCheck;
        }
        if (bRecordType == ET9ASDBUNKNOWN) {
            /* force a bad checksum to be returned */
            return wCheckSum - 1;
        }
        /* only length/type field goes toward the checksum for free records */
        switch (bRecordType) {
            case ET9ASDBTYPE:
                nIndex = nRecordLength;
                break;
            case ET9ASMULTIFREETYPE:
                nRemainingMemory = nRemainingMemory + nRecordLength;
                nIndex = 2;
                break;
            /* lump bot ET9ASSINGLEFREETYPE and ET9ASDBUNKNOWN as single byte types */
            default:
                nRemainingMemory++;
                nIndex = 1;
                break;
        }

        /* now loop through the entire record to sum it's bytes */
        pbData = pbAsdbData;
        while (nIndex--) {
            wCheckSum = (ET9U16)(wCheckSum + *pbData++);
            __CheckForASDBWrap_ByteP(pbData, pASDB);
        }

        /* update the over size tracker */
        wSizeChecked = (ET9U16)(wSizeChecked + nRecordLength);
        /* and point to the next record */
        pbAsdbData += nRecordLength;
        __CheckForASDBWrap_ByteP(pbAsdbData, pASDB);
    }

    /* remove the range markers from available size */
    nRemainingMemory -= ET9NUMASDBSIZERANGES;
    /* the free size calculated SHOULD match the value maintained in the header */
    ET9Assert(nRemainingMemory == (ET9UINT)pASDB->wRemainingMemory);

#ifdef ET9_DEBUG
    __ET9AWASDBVerifySingleBytes(pASDB);
#endif

    return wCheckSum;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Set range info for searching portion of ASDB.
 * Initially, there will be 6 (ET9NUMASDBSIZERANGES) size regions<BR>
 * divided into the following categories:<BR>
 * Region 0    - shortcut words 2 characters long<BR>
 * Region 1    - shortcut words 3 characters long<BR>
 * Region 2    - shortcut words 4 characters long<BR>
 * Region 3    - shortcut words 5 characters long<BR>
 * Region 4    - shortcut words 6 characters long<BR>
 * Region 5    - shortcut words > 6 characters long
 *
 * @param pLingInfo              Pointer to field information structure.
 * @param wLength                Size of word being searched.
 * @param bIncludeCompletions    Whether or not completions accepted.
 * @param wRegionIndex           Pointer to var to load with start region index.
 * @param ppbStart               Pointer to range start pointer.
 * @param ppbEnd                 Pointer to range end pointer.
 *
 * @return None
 */

static void ET9LOCALCALL __ET9AWSetASDBRangeInfo(ET9AWLingInfo     *pLingInfo,
                                                 ET9U16             wLength,
                                                 ET9U8              bIncludeCompletions,
                                                 ET9U16            *wRegionIndex,
                                                 ET9U8 ET9FARDATA **ppbStart,
                                                 ET9U8 ET9FARDATA **ppbEnd)
{
    ET9U16 wRegion;

    ET9AWASDBInfo ET9FARDATA * const pASDB = pLingInfo->pLingCmnInfo->pASDBInfo;

    ET9Assert(pLingInfo);
    ET9Assert(wRegionIndex);
    ET9Assert(ppbStart);
    ET9Assert(ppbEnd);
    ET9Assert(pASDB != NULL);

    if (wLength == 1) {
        wRegion = 0;
    }
    else if (wLength <= ET9NUMASDBSIZERANGES) {

        /* since regions 0 - (ET9NUMASDBSIZERANGES - 2) only contain 1 word length each */

        wRegion = (ET9U16)(wLength - 2);
    }

    /* otherwise region is all inclusive final region that includes all shortcuts */
    /* of size ET9NUMASDBSIZERANGES symbols or longer                             */

    else {
        wRegion = (ET9NUMASDBSIZERANGES - 1);
    }

    /* load the return values, including the region index word might be in */

    *wRegionIndex = wRegion;

    /* the start address of that region */

    *ppbStart = ET9ASDBData(pASDB) + pASDB->wSizeOffset[wRegion];

    /* and the end address of the region (or region group) */
    /* if completions included,  */

    if (bIncludeCompletions) {

        /* include all regions after this one as well */

        *ppbEnd = ET9ASDBData(pASDB) + pASDB->wSizeOffset[0];
    }
    else {

        /* only examine this one region */

        *ppbEnd = ET9ASDBData(pASDB) + pASDB->wSizeOffset[(wRegion + 1) % ET9NUMASDBSIZERANGES];
    }
}

#if 0               /* *** DISABLE UNTIL DETERMINED NECESSARY *** */
/*---------------------------------------------------------------------------*/
/** \internal
 * Called whenever a specific, previously unaccessed entry in the ASDB is accessed.
 *
 * @param pLingInfo          Pointer to field information structure.
 * @param pbEntry            Pointer to R/UDB entry in database.
 *
 * @return None
 */

static void ET9LOCALCALL __ET9AWSetASDBUseMarker(ET9AWLingInfo    *pLingInfo,
                                                 ET9U8 ET9FARDATA *pbEntry)
{
    ET9U16   wTemp;
    ET9U8    bOldByte;
    ET9U8    bNewByte;
    ET9AWASDBInfo ET9FARDATA *pASDB;

    ET9Assert(pLingInfo);
    ET9Assert(pbEntry);

    pASDB = pLingInfo->pASDBInfo;
    ET9Assert(pASDB != NULL);

    /* read the current value */
    bOldByte  = *pbEntry;
    bNewByte  = bOldByte | ASDB_STATUS_USED_BITMASK;

    /* update the record with the new status byte */
    __ET9AWWriteASDBData(pLingInfo, (void ET9FARDATA*)pbEntry,
                        (const void ET9FARDATA *)&bNewByte, 1);

    /* Also add into ASDB checksum */
    wTemp = (ET9U16)(pASDB->wDataCheck - bOldByte + bNewByte);
    __ET9AWWriteASDBData(pLingInfo, (void ET9FARDATA*)&pASDB->wDataCheck,
                         (const void ET9FARDATA *)&wTemp, sizeof(wTemp));
}
#endif

/*---------------------------------------------------------------------------*/
/** \internal
 * Search for existence of word in ASDB.
 *
 * @param pLingInfo          Pointer to alpha info struct.
 * @param pWord              Pointer to shortcut word info being searched for.
 * @param wLength            Length of shortcut word.
 * @param bCaseSensitive     If set, does direct comparison; if clear, does lowercase comparison.
 * @param bUpdateWithMatch   If set, will overwrite passed pWord with ASDB shortcut version IFF match found.
 *
 * @return None-zero - entry found, zero - not found.
 */

ET9UINT ET9FARCALL _ET9AWFindASDBObject(ET9AWLingInfo     *pLingInfo,
                                        ET9SYMB           *pWord,
                                        ET9U16             wLength,
                                        ET9U8              bCaseSensitive,
                                        ET9U8              bUpdateWithMatch)
{
    ET9U8 ET9FARDATA   *pbNext;
    ET9U8 ET9FARDATA   *pbEnd;
    ET9U16              wRegion;
    ET9UINT             nCurRecType;
    ET9SYMB ET9FARDATA *pSymb;
    ET9SYMB            *pSymb2;
    ET9UINT             nRecordLength;
    ET9U16              wWordSize = 0;
    ET9U16              i;
    ET9ASDBRecordHeader sASDBRecHeader;
    ET9SYMB             sSymb;
    const ET9U8 ET9FARDATA *pbByte;

    ET9AWASDBInfo ET9FARDATA * const pASDB = pLingInfo->pLingCmnInfo->pASDBInfo;

    ET9Assert(pLingInfo);
    ET9Assert(pWord);

    /* if no ASDB or bad word length */
    if ((pASDB == NULL) || !wLength || wLength > ET9MAXWORDSIZE) {
        return 0;
    }

    /* get the beginning and end addresses of range record should be in (if there) */
    __ET9AWSetASDBRangeInfo(pLingInfo, wLength, 0, &wRegion, &pbNext, &pbEnd);

    /* Move past range marker */
    ++pbNext;
    __CheckForASDBWrap_ByteP(pbNext, pASDB);

    /* loop through the region(s) returned */
    while (pbNext != pbEnd) {
        nCurRecType = __ET9AWASGetRecordInfo(pASDB, pbNext, &nRecordLength);

        /* if it's an auto-substitution record */
        if (nCurRecType == ET9ASDBTYPE) {
            pSymb = (ET9SYMB *)__ET9AWReadASDBRecordHeader(pASDB, pbNext, &sASDBRecHeader);
            wWordSize = (ET9U16)sASDBRecHeader.bShortcutLen;
            /* if the word size matches and the record type is OK */
            if (wWordSize == wLength) {
                /* loop through comparing symbols */
                pSymb2 = pWord;
                for (i = 0; i < wWordSize; ++i, ++pSymb2) {
                    /* ReadSymbol */
                    pbByte = (const ET9U8 ET9FARDATA *)pSymb + 1;
                    __CheckForASDBWrap_ByteP(pbByte, pASDB);
                    sSymb = ET9MAKEWORD(*((const ET9U8 ET9FARDATA *)pSymb), *pbByte);
                    /* if OTFM used, convert ASDB symbol */
                    if (pLingInfo->Private.pConvertSymb != NULL) {
                        pLingInfo->Private.pConvertSymb(pLingInfo->Private.pConvertSymbInfo, &sSymb);
                    }

                    /* if not case sensitive, do lowercase compare; */
                    /* if mismatch, jump out of 'for' loop          */
                    if (!bCaseSensitive) {
                        ET9SYMB  sSymbOther = _ET9SymToOther(*pSymb2, pLingInfo->pLingCmnInfo->wFirstLdbNum);
                        if (ET9AWSys_GetBilingualSupported(pLingInfo)) {
                            ET9SYMB  sSymbOther2 = _ET9SymToOther(*pSymb2, pLingInfo->pLingCmnInfo->wSecondLdbNum);
                            /* if symbol doesn't match, move on to the next record */
                            if (*pSymb2 != sSymb && sSymbOther != sSymb && sSymbOther2 != sSymb) {
                                break;
                            }
                        }
                        else {
                            /* if symbol doesn't match, move on to the next record */
                            if (*pSymb2 != sSymb && sSymbOther != sSymb) {
                                break;
                            }
                        }
                    }
                    /* else IF case sensitive, do a direct comparison, and break out if no match */
                    else {
                        if (*pSymb2 != sSymb) {
                            break;
                        }
                    }
                    /* on match, move on to the next symbol in the record entry */
                    ++pSymb;
                    __CheckForASDBWrap_WordP(pSymb, pASDB);
                }
                /* if looped through all symbols and all matched, break out of 'while' loop */
                if (i == wWordSize) {

#if 0               /**** DISABLE UNTIL DETERMINED NECESSARY ****/
                    /* found, mark the usage indicator if not already set */
                    if (!(sASDBRecHeader.bStatus & ASDB_STATUS_USED_BITMASK)) {
                        __ET9AWSetASDBUseMarker(pLingInfo, pbNext);
                    }
#endif
                    /* if caller wants ASDB version of the shortcut, update buffer */
                    if (bUpdateWithMatch) {
                        /* start over at beginning of shortcut location */
                        pSymb = (ET9SYMB *)__ET9AWReadASDBRecordHeader(pASDB, pbNext, &sASDBRecHeader);
                        pSymb2 = pWord;
                        for (i = 0; i < wWordSize; ++i) {
                            /* ReadSymbol */
                            pbByte = (const ET9U8 ET9FARDATA *)pSymb + 1;
                            __CheckForASDBWrap_ByteP(pbByte, pASDB);
                            *pSymb2++ = ET9MAKEWORD(*((const ET9U8 ET9FARDATA *)pSymb), *pbByte);

                            ++pSymb;
                            __CheckForASDBWrap_WordP(pSymb, pASDB);
                        }
                    }
                    /* break out of the 'while' loop */
                    break;
                }
            }
        }
        /* move to the next record */
        pbNext += nRecordLength;
        __CheckForASDBWrap_ByteP(pbNext, pASDB);
    }

    /* if all entries in region examined, return in failure */
    if (pbNext == pbEnd)  {
        wWordSize = 0;
    }
    return wWordSize;
}


/*---------------------------------------------------------------------------*/
/** \internal
 * Gets a memory chunk to the 'right' of given DB address.
 *
 * @param pLingInfo          Pointer to field information structure.
 * @param pbEnterSpot        Pointer to go right from.
 * @param nSizeRequired      Size required.
 * @param nSizeRangeIndex    Index of size range entry belongs to.
 *
 * @return Size consolidated on right.
 */

static ET9UINT ET9LOCALCALL __ET9AWASDBGoRightForMemoryChunk(ET9AWLingInfo    *pLingInfo,
                                                             ET9U8 ET9FARDATA *pbEnterSpot,
                                                             ET9UINT           nSizeRequired,
                                                             ET9UINT           nSizeRangeIndex)
{
    /* this is a little worrisome... 48 bytes in the next 3 arrays */
    /* all locals together currently ~108  bytes on stack          */
    ET9U8     ET9FARDATA *pbDataPointer[ET9NUMASDBSIZERANGES];
    ET9U16    wDataLengths[ET9NUMASDBSIZERANGES];
    ET9U16    wSizeRangeMoveLengths[ET9NUMASDBSIZERANGES];
    ET9UINT   nSizeRecovered = 0;
    ET9U8     ET9FARDATA *pbNextRange;
    ET9U8     ET9FARDATA *pbNext;
    ET9U8     ET9FARDATA *pbDest = 0;
    ET9U8     ET9FARDATA *pbEnd = 0;
    ET9U16    wCheckSum, wNewSizeOffset;
    ET9UINT   nNumDataChunks = 0;
    ET9UINT   nRecordType;
    ET9UINT   nRecordLength;
    ET9UINT   nAmPointing = 0;
    ET9UINT   nIndex;
    ET9UINT   nSizeStillNeeded;
    ET9UINT   nIsSizeRegionStart = 0;
    ET9S32    snUncheckedSize;

    ET9AWLingCmnInfo * const pLingCmnInfo = pLingInfo->pLingCmnInfo;
    ET9AWASDBInfo ET9FARDATA * const pASDB = pLingCmnInfo->pASDBInfo;

    ET9Assert(pLingInfo);
    ET9Assert(pbEnterSpot);
    ET9Assert(pASDB != NULL);

    /* initialize tracking array */
    _ET9ByteSet((ET9U8*)wSizeRangeMoveLengths, ET9NUMASDBSIZERANGES * sizeof(ET9U16), 0xFF);
    wCheckSum = pASDB->wDataCheck;

    /* store pointers to data until the recovered size is greater than the needed size.*/
    nSizeRangeIndex = (nSizeRangeIndex + 1) % ET9NUMASDBSIZERANGES;
    pbNextRange = __ET9AWMoveASDBPtrRight(pASDB, ET9ASDBData(pASDB), pASDB->wSizeOffset[nSizeRangeIndex]);
    pbNext = pbEnterSpot;

    nSizeStillNeeded = nSizeRequired;

    snUncheckedSize = (ET9S32)ET9ASDBDataAreaBytes(pASDB);

    while (snUncheckedSize > 0) {
        nRecordType = __ET9AWASGetRecordInfo(pASDB, pbNext, &nRecordLength);
        snUncheckedSize = snUncheckedSize - (ET9S32)nRecordLength;

        /* monitoring the need for shifting of size region indices */
        /* if the record is the start of a region */
        if  (pbNextRange == pbNext) {
            nIsSizeRegionStart = 1;
            wSizeRangeMoveLengths[nSizeRangeIndex] = (ET9U16)nSizeStillNeeded;
            nSizeRangeIndex = (nSizeRangeIndex + 1) % ET9NUMASDBSIZERANGES;
            pbNextRange = __ET9AWMoveASDBPtrRight(pASDB, ET9ASDBData(pASDB),
                                            pASDB->wSizeOffset[nSizeRangeIndex]);
        }

        /* if data record or beginning of size region */
        if ((nRecordType == ET9ASDBTYPE) || nIsSizeRegionStart) {
            if (!nAmPointing) {
                /* need to limit the number of free chunks we're accumulating */
                if (nNumDataChunks == ET9NUMASDBSIZERANGES) {
                    pbEnd = pbNext;
                    ET9Assert(pASDB->wDataCheck == __ET9AWGetASDBChecksum(pLingInfo));
                    break;
                }
                pbDataPointer[nNumDataChunks++] = pbNext;
                nAmPointing = 1;
            }
        }
        /* else it's a free record thats NOT a size region marker */
        else {
            if (nAmPointing) {
                /* Calculate the distance between two data pointers in circular ASDB */
                if (pbNext >= pbDataPointer[nNumDataChunks - 1]) {
                    wDataLengths[nNumDataChunks - 1] =
                        (ET9U16)(pbNext - pbDataPointer[nNumDataChunks - 1]);
                }
                else {
                    wDataLengths[nNumDataChunks - 1] =
                        (ET9U16)(ET9ASDBDataAreaBytes(pASDB) - (ET9UINT)(pbDataPointer[nNumDataChunks - 1] - pbNext));
                }
                nAmPointing = 0;
            }
            wCheckSum = (ET9U16)(wCheckSum - *pbNext);
            /* if multiple byte free record, include next byte in checksum adjustment */
            if (nRecordType == ET9ASMULTIFREETYPE) {
                wCheckSum = (ET9U16)(wCheckSum - *(__ET9AWMoveASDBPtrRight(pASDB, pbNext, 1)));
            }
            nSizeRecovered = nSizeRecovered + nRecordLength;
            if (nRecordLength >= nSizeStillNeeded) {
                pbEnd = pbNext + nSizeStillNeeded;
                __CheckForASDBWrap_ByteP(pbEnd, pASDB);
                break;
            }
            else {
                pbEnd = pbNext + nRecordLength;
                __CheckForASDBWrap_ByteP(pbEnd, pASDB);
            }
            nSizeStillNeeded = nSizeRequired - nSizeRecovered;
        }
        nIsSizeRegionStart = 0;
        pbNext += nRecordLength;
        __CheckForASDBWrap_ByteP(pbNext, pASDB);

    }

    if (!pbEnd) {
        return 0;
    }
    /* Consolidate data chunks */
    pbDest = pbEnd;

    if (nNumDataChunks) {
            /* as added precaution, pASDBGetEntry should be cleared to prevent */
            /* traversal from pointing to a bogus location */
            pLingCmnInfo->Private.pASDBGetEntry = 0;

        for (nIndex = nNumDataChunks; nIndex > 0; --nIndex) {
            /* __ET9AWMoveASDBPtrLeft(pASDB, pbDest, wDataLengths[nIndex - 1]) */
            pbDest -= wDataLengths[nIndex - 1];
            /* if move is past beginning of ASDB data area, circle back to end of data area */
            if (pbDest  < ET9ASDBData(pASDB)) {
                pbDest += ET9ASDBDataAreaBytes(pASDB);
            }
            __ET9AWCopyASDBString(pLingInfo, pbDest, pbDataPointer[nIndex - 1], wDataLengths[nIndex - 1]);
        }
    }

    /* Write new large free record info */
    wCheckSum = __ET9AWASDBWriteHeader(pLingInfo, pbEnterSpot, ET9ASMULTIFREETYPE,
        ((nSizeRecovered < nSizeRequired) ? nSizeRecovered : nSizeRequired), 0, wCheckSum);

    /* write leftover record if there is one. */
    pbDest = pbEnd;
    if (nSizeRecovered > nSizeRequired) {
        if (nSizeRecovered - nSizeRequired == 1) {
            wCheckSum = __ET9AWASDBWriteHeader(pLingInfo, pbDest, ET9ASSINGLEFREETYPE,
                           1, 0, wCheckSum);
        }
        else {
            wCheckSum = __ET9AWASDBWriteHeader(pLingInfo, pbDest, ET9ASMULTIFREETYPE,
                           nSizeRecovered - nSizeRequired, 0, wCheckSum);
        }
    }

    /* reset affected Key offsets */
    for (nIndex = 0; nIndex < ET9NUMASDBSIZERANGES; ++nIndex) {
        if (wSizeRangeMoveLengths[nIndex] != 0xFFFF) {
            wCheckSum = (ET9U16)(wCheckSum - pASDB->wSizeOffset[nIndex]);
            wNewSizeOffset =
                (ET9U16)(pASDB->wSizeOffset[nIndex] + wSizeRangeMoveLengths[nIndex] -
                (ET9U16)((nSizeRecovered < nSizeRequired) ? nSizeRequired - nSizeRecovered : 0));
            wNewSizeOffset = (ET9U16)(wNewSizeOffset % ET9ASDBDataAreaBytes(pASDB));
            __ET9AWWriteASDBData(pLingInfo, (void ET9FARDATA*)&pASDB->wSizeOffset[nIndex],
                (const void ET9FARDATA *)&wNewSizeOffset, sizeof(wNewSizeOffset) );
            wCheckSum = (ET9U16)(wCheckSum + wNewSizeOffset);
        }
    }
    /* write new checksum */
    __ET9AWWriteASDBData(pLingInfo, (void ET9FARDATA*)&pASDB->wDataCheck,
                 (const void ET9FARDATA *)&wCheckSum, sizeof(wCheckSum));

    ET9Assert(pASDB->wDataCheck == __ET9AWGetASDBChecksum(pLingInfo));

    return nSizeRecovered;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Loops until enough free ASDB memory has been consolidated.
 *
 * @param pLingInfo          Pointer to field information structure.
 * @param pbEnterSpot        Pointer to go right from.
 * @param nSizeRequired      Size required.
 * @param nSizeRangeIndex    Index of size range entry belongs to.
 *
 * @return Size consolidated on right.
 */

static ET9UINT ET9LOCALCALL __ET9AWASDBGoRightForMemory(ET9AWLingInfo    *pLingInfo,
                                                        ET9U8 ET9FARDATA *pbEnterSpot,
                                                        ET9UINT           nSizeRequired,
                                                        ET9UINT           nSizeRangeIndex)
{
    ET9UINT nSize = 0;
    ET9UINT nLastSizeRecovered = 0;

    /* Loop over __ET9AWASDBGoRightForMemoryChunk, which can only consolidate up   */
    /* to ET9NUMASDBSIZERANGES free records per call (may take more than one call  */
    /* to get the enough memory segments consolidated into a contiguous chunk).    */
    while (nSize < nSizeRequired) {
        nSize = __ET9AWASDBGoRightForMemoryChunk(pLingInfo, pbEnterSpot, nSizeRequired, nSizeRangeIndex);
        /* if call failed somehow, just return... ain't gonna happen */
        if (nSize <= nLastSizeRecovered) {
            nSize = 0;
            break;
        }
        /* otherwise, record current free memory. */
        nLastSizeRecovered = nSize;
    }
    return nSize;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Find positon to add Add one object to ASDB data base.
 *
 * @param pLingInfo          Pointer to field information structure.
 * @param wShortcutLen       Length (in symbols) of shortcut.
 * @param wSubstitutionLen   Length (in symbols) of substitution.
 *
 * @return Ptr to position to add new record (or 0 if failed to find location).
 */

static ET9U8 ET9LOCALCALL ET9FARDATA * __ET9AWASDBFindAddPosition(ET9AWLingInfo *pLingInfo,
                                                                  ET9U16         wShortcutLen,
                                                                  ET9U16         wSubstitutionLen)
{
    ET9U8          ET9FARDATA *pbNext = 0;
    ET9U8          ET9FARDATA *pbLast;
    ET9U8          ET9FARDATA *pbEnterSpot = 0;
    ET9U8          ET9FARDATA *pbStart;
    ET9U8          ET9FARDATA *pbEnd;
    ET9UINT        nMemoryNeeded;
    ET9UINT        nMemoryFound = 0;
    ET9UINT        nRecordLength;
    ET9U16         wRegion;

    ET9AWASDBInfo ET9FARDATA * const pASDB = pLingInfo->pLingCmnInfo->pASDBInfo;

    ET9Assert(pLingInfo);
    ET9Assert(pASDB != NULL);
    ET9Assert(wShortcutLen);
    ET9Assert(wSubstitutionLen);

    /* calc required memory to store AS record */
    nMemoryNeeded = (ET9UINT)(wShortcutLen * ET9SYMBOLWIDTH) +
                    (ET9UINT)(wSubstitutionLen * ET9SYMBOLWIDTH) +
                    (ET9UINT)(ASDB_RECORD_HEADER_SIZE);

    /* if there's actually enough free memory left for record, do it */
    /* otherwise just return NULL pointer to indicate failure        */
    if (nMemoryNeeded <= pASDB->wRemainingMemory) {

        /* get the beginning and end addresses of range record should go in */
        __ET9AWSetASDBRangeInfo(pLingInfo, wShortcutLen, 0, &wRegion, &pbStart, &pbEnd);

        /* Skip past the single byte region indicator record */
        pbLast = pbStart + 1;
        __CheckForASDBWrap_ByteP(pbLast, pASDB);

        /* first look for an available free record that's big enough to accomodate new record */
        while (pbLast != pbEnd) {
            /* if it's a free record */
            if (ET9ASMULTIFREETYPE == __ET9AWASGetRecordInfo(pASDB, pbLast, &nRecordLength)) {
                /* if it's big enough */
                if (nRecordLength >= nMemoryNeeded) {
                    /* save location and get out of this loop */
                    pbEnterSpot = pbLast;
                    break;
                }
                /* else if this is the first free rec in region, save ptr to it          */
                /* to use if __ET9AWASDBGoRightForMemory is needed to consolidate memory */
                else if (!pbNext) {
                    pbNext = pbLast;
                }
            }
            /* move on to next record */
            pbLast += nRecordLength;
            __CheckForASDBWrap_ByteP(pbLast, pASDB);
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

        nMemoryFound = __ET9AWASDBGoRightForMemory(pLingInfo, pbEnterSpot, nMemoryNeeded, wRegion);
        /* if we couldn't consolidate enough memory for ercord, return in failure */
        if (nMemoryFound < nMemoryNeeded) {
            pbEnterSpot = 0;
        }
    }
    return pbEnterSpot;
}

/*---------------------------------------------------------------------------*/
/**
 * Adds a shortcut/substitution pair entry to the ASDB.
 *
 * @param pLingInfo             Pointer to alphabetic information structure.
 * @param psShortCut            Shortcut being added.
 * @param psSubstitution        Substitution for shortcut.
 * @param wShortcutLen          Length (in symbols) of shortcut.
 * @param wSubstitutionLen      Length (in symbols) of substitution.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL ET9AWASDBAddEntry(ET9AWLingInfo * const    pLingInfo,
                                       ET9SYMB * const          psShortCut,
                                       ET9SYMB * const          psSubstitution,
                                       const ET9U16             wShortcutLen,
                                       const ET9U16             wSubstitutionLen)
{
    ET9AWASDBInfo ET9FARDATA    *pASDB;
    ET9U8         ET9FARDATA    *pbAsdbData;

    ET9U16        wChecksum;
    ET9U16        wTemp;
    ET9UINT       nRecordLen;
    ET9UINT       nCount = 0;
    ET9SYMB       sData;
    ET9U8         bSymByte;
    ET9U8         bSymByte2;
    ET9STATUS     wStatus;
    ET9U16        wRecordNum;

    if ((wStatus = _ET9AWSys_BasicValidityCheck(pLingInfo)) != ET9STATUS_NONE) {
        return wStatus;
    }
    if (psShortCut == NULL ||  psSubstitution == NULL) {
        return ET9STATUS_INVALID_MEMORY;
    }
    pASDB = pLingInfo->pLingCmnInfo->pASDBInfo;
    if (pASDB == NULL) {
        return ET9STATUS_NO_ASDB;
    }
    if (!ET9USERDEFINEDAUTOSUBENABLED(pLingInfo->pLingCmnInfo)) {
        return ET9STATUS_DB_NOT_ACTIVE;
    }

    /* currently only allowing shortcuts/subs >= 1 syms long */

    if (!wShortcutLen || wShortcutLen > ET9MAXWORDSIZE ||
        !wSubstitutionLen || wSubstitutionLen > ET9MAXWORDSIZE) {
        return ET9STATUS_BAD_PARAM;
    }

    /* No white spaces allowed in the shortcut word. */

    if (_ET9FindSpacesAndUnknown(psShortCut, wShortcutLen)) {
        return ET9STATUS_INVALID_TEXT;
    }

    /* Zero not allowed in the substitution. */

    {
        ET9UINT nCount;
        ET9SYMB const * psSymb;

        psSymb = psSubstitution;
        for (nCount = wSubstitutionLen; nCount; --nCount, ++psSymb) {
            if (!*psSymb) {
                return ET9STATUS_INVALID_TEXT;
            }
        }
    }

    /* go make sure shortcut is not already defined */

    if (_ET9AWFindASDBObject(pLingInfo, psShortCut, wShortcutLen, 0, 0)) {
        return ET9STATUS_WORD_EXISTS;
    }

    /* find a place to store the new AS record */

    pbAsdbData = __ET9AWASDBFindAddPosition(pLingInfo, wShortcutLen, wSubstitutionLen);

    /* __ET9AWASDBFindAddPosition returns a 0 ptr if couldn't find a spot big enough */

    if (!pbAsdbData) {
        return ET9STATUS_DB_NOT_ENOUGH_MEMORY;
    }

    wChecksum = pASDB->wDataCheck;

    /* remove the free record header info from the checksum */

    wChecksum = (ET9U16)(wChecksum - *pbAsdbData);
    wChecksum = (ET9U16)(wChecksum - *(__ET9AWMoveASDBPtrRight(pASDB, pbAsdbData, 1)));

    /* write AS record header length/type data */

    wChecksum = __ET9AWASDBWriteHeader(pLingInfo, pbAsdbData, ET9ASDBTYPE,
                                       wShortcutLen, wSubstitutionLen, wChecksum);

    /* get the entire AS record length, and set pointer to point immediately after header */

    nRecordLen = __ET9AWGetASDBRecordLength(pASDB, pbAsdbData);
    pbAsdbData += ASDB_RECORD_HEADER_SIZE;
    __CheckForASDBWrap_ByteP(pbAsdbData, pASDB);

    /* Copy ASDB shortcut symbols, and update checksum */

    while (nCount < wShortcutLen) {
        sData = psShortCut[nCount];
        bSymByte  = (ET9U8)ET9HIBYTE(sData);
        bSymByte2 = (ET9U8)ET9LOBYTE(sData);
        wChecksum = (ET9U16)(wChecksum + bSymByte + bSymByte2);
        __ET9AWWriteASDBData(pLingInfo, (void ET9FARDATA *)pbAsdbData,
                             (const void ET9FARDATA *)&bSymByte, 1);
        ++pbAsdbData;
        __CheckForASDBWrap_ByteP(pbAsdbData, pASDB);
        __ET9AWWriteASDBData(pLingInfo, (void ET9FARDATA *)pbAsdbData,
                             (const void ET9FARDATA *)&bSymByte2, 1);
        ++pbAsdbData;
        __CheckForASDBWrap_ByteP(pbAsdbData, pASDB);
        ++nCount;
    }

    /* Copy ASDB substitution symbols, and update checksum */

    nCount = 0;
    while (nCount < wSubstitutionLen) {
        sData = psSubstitution[nCount];
        bSymByte = (ET9U8)ET9HIBYTE(sData);
        bSymByte2 = (ET9U8)ET9LOBYTE(sData);
        wChecksum = (ET9U16)(wChecksum + bSymByte + bSymByte2);
        __ET9AWWriteASDBData(pLingInfo, (void ET9FARDATA *)pbAsdbData,
                             (const void ET9FARDATA *)&bSymByte, 1);
        ++pbAsdbData;
        __CheckForASDBWrap_ByteP(pbAsdbData, pASDB);
        __ET9AWWriteASDBData(pLingInfo, (void ET9FARDATA *)pbAsdbData,
                             (const void ET9FARDATA *)&bSymByte2, 1);
        ++pbAsdbData;
        __CheckForASDBWrap_ByteP(pbAsdbData, pASDB);
        ++nCount;
    }

    /* Increment AS record entry count */

    wTemp = (ET9U16)(pASDB->wEntryCount + 1);
    __ET9AWWriteASDBData(pLingInfo, (void ET9FARDATA*)&pASDB->wEntryCount,
                    (const void ET9FARDATA *)&wTemp, sizeof(wTemp));

    /* modify remaining available memory count */

    wTemp = (ET9U16)(pASDB->wRemainingMemory - nRecordLen);
    __ET9AWWriteASDBData(pLingInfo, (void ET9FARDATA*)&pASDB->wRemainingMemory,
                (const void ET9FARDATA *)&wTemp, sizeof(wTemp));

    /* Store new asdb checksum (plus 1 for incremented word count) */

    wTemp = (ET9U16)(wChecksum + (ET9UINT)1 - nRecordLen);
    __ET9AWWriteASDBData(pLingInfo, (void ET9FARDATA*)&pASDB->wDataCheck,
                (const void ET9FARDATA *)&wTemp, sizeof(wTemp));

    /* check LDB-supported entries to see if shortcut defined; if it is, */
    /* set it as 'removed/replaced' for this language                    */

    if (!_ET9AWLdbASFindEntry(pLingInfo, pLingInfo->pLingCmnInfo->wLdbNum, psShortCut, wShortcutLen, 0, 0, &wRecordNum)) {
        _ET9AWLdbASDisableRecord(pLingInfo, pLingInfo->pLingCmnInfo->wLdbNum, wRecordNum);
    }

    return (ET9STATUS_NONE); /* success */
}

/*---------------------------------------------------------------------------*/
/**
 * Determine if a given shortcut word exists as an ASDB entry.
 * Find an ASDB shortcut word by passing a word string and the word length.<BR>
 * NOTE: ASSUMES psSubstitution is big enough to hold substitution...<BR>
 * should be ET9SYMB [ET9MAXWORDSIZE] array
 *
 * @param pLingInfo             Pointer to alphabetic information structure.
 * @param psShortcut            Pointer to shortcut word to match.
 * @param wShortcutLen          Length (in symbols) of the shortcut.
 * @param psSubstitutionBuf     Pointer to buffer to return found record's substitution in (if provided).
 * @param wSubstitutionBufLen   Length (in symbols) of the input buffer pointed to by psSubstitutionIn (if provided).
 * @param pwSubstitutionLen     Pointer to return found record's substitution length (if provided).
 *
 * @return ET9STATUS NONE if word is found or ET9STATUS_NO_MATCHING_WORDS if no matching word is found.
 */

ET9STATUS ET9FARCALL ET9AWASDBFindEntry(ET9AWLingInfo * const pLingInfo,
                                        ET9SYMB       * const psShortcut,
                                        const ET9U16          wShortcutLen,
                                        ET9SYMB       * const psSubstitutionBuf,
                                        const ET9U16          wSubstitutionBufLen,
                                        ET9U16        * const pwSubstitutionLen)
{
    ET9STATUS wStatus;
    ET9AWASDBInfo ET9FARDATA   *pASDB;
    ET9U8 ET9FARDATA *pbNext = 0;
    ET9U8 ET9FARDATA *pbEnd = 0;
    ET9U16  i;
    ET9U8 ET9FARDATA *pSymb = 0;
    ET9UINT nRecordLen;
    ET9U16  wRegion;
    ET9U16  wRecordNum;
    ET9SYMB *pTarget;
    ET9ASDBRecordHeader sRecHeader;
    ET9SYMB  sSymb;
    ET9SYMB  sSymb2;
    const ET9U8 ET9FARDATA *pbByte;
    ET9SYMB *psSubstitution = psSubstitutionBuf;

    if ((wStatus = _ET9AWSys_BasicValidityCheck(pLingInfo)) == ET9STATUS_NONE) {
        if (psShortcut == NULL) {
            wStatus = ET9STATUS_INVALID_MEMORY;
        }
        else if (pLingInfo->pLingCmnInfo->pASDBInfo == NULL) {
            if (!ET9LDBSUPPORTEDAUTOSUBENABLED(pLingInfo->pLingCmnInfo) ){
                wStatus = ET9STATUS_NO_ASDB;
            }
        }
        else if (!wShortcutLen || wShortcutLen > ET9MAXWORDSIZE) {
            wStatus = ET9STATUS_BAD_PARAM;
        }
        else if ((psSubstitution != NULL) && wSubstitutionBufLen < ET9MAXSUBSTITUTIONSIZE) {
            wStatus = ET9STATUS_BUFFER_TOO_SMALL;
        }
        else {
            pASDB = pLingInfo->pLingCmnInfo->pASDBInfo;
            if (ET9USERDEFINEDAUTOSUBENABLED(pLingInfo->pLingCmnInfo)) {
                /* Get the address range for the region that should hold record (if it exists) */
                __ET9AWSetASDBRangeInfo(pLingInfo, wShortcutLen, 0, &wRegion, &pbNext, &pbEnd);
                /* Skip past single byte region indicator record */
                ++pbNext;
                __CheckForASDBWrap_ByteP(pbNext, pASDB);

                /* loop through the entries in that region */
                while (pbNext != pbEnd) {
                    /* if this is a ASDB record */
                    if (ET9ASDBTYPE == __ET9AWASGetRecordInfo(pASDB, pbNext, &nRecordLen)) {
                        /* check to see if it matches */
                        pSymb  = __ET9AWReadASDBRecordHeader(pASDB, pbNext, &sRecHeader);
                        /* only need to check closer if shortcut length matches */
                        if ((ET9U16)sRecHeader.bShortcutLen == wShortcutLen) {
                            pTarget = psShortcut;
                            /* do a lowercase sym-to-sym comparison of the shortcut */
                            for (i = wShortcutLen; i; --i) {
                                /* if no match, move on */
                                sSymb2 = _ET9SymToLower(*pTarget++, pLingInfo->pLingCmnInfo->wLdbNum);
                                /* ReadSymbol */
                                pbByte = (const ET9U8 ET9FARDATA *)pSymb + 1;
                                __CheckForASDBWrap_ByteP(pbByte, pASDB);
                                sSymb = _ET9SymToLower(ET9MAKEWORD(*((const ET9U8 ET9FARDATA *)pSymb), *pbByte), pLingInfo->pLingCmnInfo->wLdbNum);

                                /* if there's a mismatch, get outta town */
                                if (sSymb2 != sSymb) {
                                    break;
                                }
                                pSymb += ET9SYMBOLWIDTH;
                                __CheckForASDBWrap_ByteP(pSymb, pASDB);
                            }
                            /* if all symbols match, shortcut match was found */
                            if (!i) {
                                /* if caller wants associated substitution string */
                                if ((psSubstitution != NULL) && (pwSubstitutionLen != NULL)) {
                                    /* pSymb points to Substitution string; sRecHeader is header */
                                    *pwSubstitutionLen = (ET9U16)sRecHeader.bSubstitutionLen;
                                    for (i = 0; i < sRecHeader.bSubstitutionLen; ++i) {
                                        /* ReadSymbol */
                                        pbByte = (const ET9U8 ET9FARDATA *)pSymb + 1;
                                        __CheckForASDBWrap_ByteP(pbByte, pASDB);
                                        *psSubstitution++ = ET9MAKEWORD(*((const ET9U8 ET9FARDATA *)pSymb), *pbByte);

                                        pSymb += ET9SYMBOLWIDTH;
                                        __CheckForASDBWrap_ByteP(pSymb, pASDB);
                                    }
                                }
                                break;
                            }
                        }
                    }
                    /* move to the next record */
                    pbNext += nRecordLen;
                    __CheckForASDBWrap_ByteP(pbNext, pASDB);

                } /* just look in the single region */
            }
            /* if no matching user-defined shortcut found... */
            if (pbNext == pbEnd) {
                if (ET9LDBSUPPORTEDAUTOSUBENABLED(pLingInfo->pLingCmnInfo)) {
                    /* go check current language's LDB AS entries */
                    wStatus = _ET9AWLdbASFindEntry(pLingInfo, pLingInfo->pLingCmnInfo->wLdbNum,
                                                   psShortcut, wShortcutLen, psSubstitution,
                                                   pwSubstitutionLen, &wRecordNum);
                    /* if found, still need to make sure it's active */
                    if (!wStatus) {
                        /* if not enabled, make sure substitution is not returned */
                        if (!_ET9AWLdbASRecordEnabled(pLingInfo, pLingInfo->pLingCmnInfo->wLdbNum, wRecordNum)) {
                            psSubstitution[0] = 0;
                            *pwSubstitutionLen = 0;
                            wStatus = ET9STATUS_NO_MATCHING_WORDS;
                        }
                    }
                }
                else {
                    wStatus = ET9STATUS_NO_MATCHING_WORDS;
                }
            }
        }
    }
    return wStatus;
}

/*---------------------------------------------------------------------------*/
/**
 * Delete the ASDB record for a given shortcut word.
 * NOTE: Does a case-sensitive search for the shortcut.
 *
 * @param pLingInfo         Pointer to alphabetic information structure.
 * @param psBuf             Pointer to shortcut word to match for deletion.
 * @param wWordLen          Length of shortcut word pointed to by psBuf.
 *
 * @return ET9STATUS NONE if word deleted or ET9STATUS_NO_MATCHING_WORDS if no matching word is found.
 */

ET9STATUS ET9FARCALL ET9AWASDBDeleteEntry(ET9AWLingInfo * const pLingInfo,
                                          ET9SYMB * const       psBuf,
                                          const ET9U16          wWordLen)
{
    ET9STATUS wStatus;
    ET9AWASDBInfo ET9FARDATA   *pASDB;
    ET9U8 ET9FARDATA *pbNext = 0;
    ET9U8 ET9FARDATA *pbTemp;
    ET9U8 ET9FARDATA *pbEnd = 0;
    ET9U16  i;
    ET9U8 ET9FARDATA *pSymb;
    ET9UINT nRecordLen;
    ET9U16  wCheckSum, wTemp;
    ET9U16  wRegion;
    ET9SYMB *pTarget;
    ET9ASDBRecordHeader sRecHeader;
    ET9U16  wRecordNum;
    const ET9U8 ET9FARDATA *pbByte;

    if ((wStatus = _ET9AWSys_BasicValidityCheck(pLingInfo)) != ET9STATUS_NONE) {
        return wStatus;
    }
    if (psBuf == NULL) {
        wStatus = ET9STATUS_INVALID_MEMORY;
    }
    else if (!wWordLen || wWordLen > ET9MAXWORDSIZE) {
        wStatus = ET9STATUS_BAD_PARAM;
    }
    else if (pLingInfo->pLingCmnInfo->pASDBInfo == NULL) {
        wStatus = ET9STATUS_NO_ASDB;
    }
    /* if there are no AS records, don't waste time */
    else if ((!pLingInfo->pLingCmnInfo->pASDBInfo->wEntryCount || !ET9USERDEFINEDAUTOSUBENABLED(pLingInfo->pLingCmnInfo)) &&
             (!pLingInfo->pLingCmnInfo->Private.sLDBAutoSub.wNumEntries || !ET9LDBSUPPORTEDAUTOSUBENABLED(pLingInfo->pLingCmnInfo))) {
        wStatus = ET9STATUS_NO_MATCHING_WORDS;
    }
    else {
        if (ET9USERDEFINEDAUTOSUBENABLED(pLingInfo->pLingCmnInfo)) {
            pASDB = pLingInfo->pLingCmnInfo->pASDBInfo;

            /* Get the address range for the region that should hold record (if it exists) */
            __ET9AWSetASDBRangeInfo(pLingInfo, wWordLen, 0, &wRegion, &pbNext, &pbEnd);
            /* Skip past the single-byte region indicator record */
            ++pbNext;
            __CheckForASDBWrap_ByteP(pbNext, pASDB);

            /* loop through the entries in that region */
             while (pbNext != pbEnd) {
                /* if this is a ASDB record */
                if (ET9ASDBTYPE == __ET9AWASGetRecordInfo(pASDB, pbNext, &nRecordLen)) {
                    /* check to see if it matches */
                    pSymb  = __ET9AWReadASDBRecordHeader(pASDB, pbNext, &sRecHeader);
                    /* if it's the same length, continue comparison */
                    if ((ET9U16)sRecHeader.bShortcutLen == wWordLen) {
                        pTarget = psBuf;
                        /* loop through comparing symbols (case sensitive) */
                        for (i = wWordLen; i; --i) {
                            /* if no match, move on to the next record*/
                            /* ReadSymbol */
                            pbByte = (const ET9U8 ET9FARDATA *)pSymb + 1;
                            __CheckForASDBWrap_ByteP(pbByte, pASDB);
                            if (*pTarget++ != ET9MAKEWORD(*((const ET9U8 ET9FARDATA *)pSymb), *pbByte)) {
                                break;
                            }
                            /* point to the next symbol */
                            pSymb += ET9SYMBOLWIDTH;
                            __CheckForASDBWrap_ByteP(pSymb, pASDB);
                        }
                        /* if all symbols match, mark as deleted */
                        if (!i) {
                            /* save the address of the record being deleted */
                            pbTemp = pbNext;

                            /* loop through decrementing the ASDB checksum by the bytes in the record */
                            nRecordLen = __ET9AWGetASDBRecordLength(pASDB, pbNext);
                            i = (ET9U16)nRecordLen;
                            wCheckSum = pASDB->wDataCheck;
                            while (i--) {
                                wCheckSum = (ET9U16)(wCheckSum - *pbTemp);
                                ++pbTemp;
                                __CheckForASDBWrap_ByteP(pbTemp, pASDB);
                            }

                            /* add new free record header (also adjusts the checksum) */
                            wCheckSum = __ET9AWASDBWriteHeader(pLingInfo, pbNext,
                                               ET9ASMULTIFREETYPE, nRecordLen, 0, wCheckSum);

                            /* Decrement the AS record count by 1 */
                            wTemp = (ET9U16)(pASDB->wEntryCount - 1);
                            __ET9AWWriteASDBData(pLingInfo,
                                            (void ET9FARDATA*)&pASDB->wEntryCount,
                                            (const void ET9FARDATA *)&wTemp,
                                            sizeof(wTemp));

                            /* Increment remaining free memory count */
                            wTemp = (ET9U16)(pASDB->wRemainingMemory + nRecordLen);
                            __ET9AWWriteASDBData(pLingInfo,
                                            (void ET9FARDATA*)&pASDB->wRemainingMemory,
                                            (const void ET9FARDATA *)&wTemp,
                                            sizeof(wTemp) );

                            /* Store new asdb checksum (minus 1 for decremented word count) */
                            wCheckSum = (ET9U16)(wCheckSum - (ET9UINT)1 + nRecordLen);
                            __ET9AWWriteASDBData(pLingInfo,
                                            (void ET9FARDATA*)&pASDB->wDataCheck,
                                            (const void ET9FARDATA *)&wCheckSum,
                                            sizeof(wCheckSum) );

                            break;
                        }
                    }
                }
                /* move to the next record */
                pbNext += nRecordLen;
                __CheckForASDBWrap_ByteP(pbNext, pASDB);

            }
        }

        /* if we exit loop with all records checked and did not find a match */
        if (pbNext == pbEnd) {
            /* check the current language's LDB-AS entries to see if shortcut defined; if it is, */
            /* set it as 'removed' for this language */
            if (ET9LDBSUPPORTEDAUTOSUBENABLED(pLingInfo->pLingCmnInfo) &&
                !_ET9AWLdbASFindEntry(pLingInfo, pLingInfo->pLingCmnInfo->wLdbNum, psBuf, wWordLen, 0, 0, &wRecordNum) &&
                _ET9AWLdbASRecordEnabled(pLingInfo, pLingInfo->pLingCmnInfo->wLdbNum, wRecordNum)) {
                    _ET9AWLdbASDisableRecord(pLingInfo, pLingInfo->pLingCmnInfo->wLdbNum, wRecordNum);
            }
            else {
                wStatus = ET9STATUS_NO_MATCHING_WORDS;
            }
        }
        if (!wStatus) {
            _ET9AWRDBDeleteWord(pLingInfo, psBuf, wWordLen);
        }
    }
    return wStatus;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Get a pointer to the ET9LdbASRecMap entry for a given language.
 * This struct contains all modifiable AS information for the LDB for that language.
 *
 * @param pASDB          Pointer to ASDB information structure.
 * @param wLDBID         LDB ID to retrieve.
 *
 * @return Pointer to ET9LdbASRecMap record if found, otherwise NULL.
 */

static ET9LdbASRecMap * ET9LOCALCALL __ET9AWGetLdbASEntry(ET9AWASDBInfo ET9FARDATA *pASDB,
                                                          ET9U16                    wLDBID)
{
    ET9LdbASRecMap ET9FARDATA *pLDBChecker;
    ET9UINT         i = ET9MAXASDBLANGUAGERECORDS;

    ET9Assert(pASDB != NULL);
    pLDBChecker = pASDB->sLdbASRecord;
    /* isolate the primary ID */
    wLDBID &= ET9PLIDMASK;

    if (wLDBID != ET9PLIDNone) {
        /* loop through all entries looking for ID match */
        for (i = 0; i < ET9MAXASDBLANGUAGERECORDS; ++i, ++pLDBChecker) {
            /* if primary LDBID matches, we have a winner */
            if (pLDBChecker->wLDBID == wLDBID) {
                break;
            }
        }
    }
    if (i == ET9MAXASDBLANGUAGERECORDS) {
        pLDBChecker = 0;
    }
    return pLDBChecker;
}

/*---------------------------------------------------------------------------*/
/**
 * Sets ASDB to empty condition (no user defined AS records).
 * Clears all LDB tracking info.<BR>
 * Automatically redefines current LDB record with all AS records enabled.
 *
 * @param pLingInfo     Pointer to alphabetic information structure.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL ET9AWASDBReset(ET9AWLingInfo * const pLingInfo)
{
    ET9STATUS wStatus;
    ET9AWASDBInfo ET9FARDATA *pASDB;
    ET9U16             wTemp = 0;
    ET9U16             wOffset;
    ET9UINT            nIndex, nLength, nIndex2;
    ET9U8 ET9FARDATA  *pbNext;
    ET9LdbASRecMap ET9FARDATA   *pLDBMap;
    ET9U8 ET9FARDATA  *pByte;
    ET9U32             dwClear = 0;
    ET9LdbASRecMap ET9FARDATA *pLDB;
    ET9U16 ET9FARDATA *pLDBTracker;
    ET9U16             wTotalCount = 0;

    if ((wStatus = _ET9AWSys_BasicValidityCheck(pLingInfo)) == ET9STATUS_NONE) {
        if (pLingInfo->pLingCmnInfo->pASDBInfo == NULL) {
            wStatus = ET9STATUS_NO_ASDB;
        }
        else {
            pASDB = pLingInfo->pLingCmnInfo->pASDBInfo;
            /* save the current language's record count for re-init */
            if ((pLingInfo->pLingCmnInfo->wLdbNum & ET9PLIDMASK) != ET9PLIDNone) {
                pLDB = __ET9AWGetLdbASEntry(pASDB, pLingInfo->pLingCmnInfo->wLdbNum);
                if (pLDB) {
                    wTotalCount = pLDB->wTotalRecords;
                }
            }
            /* zero out the user-defined AS record count (wEntryCount) */
            __ET9AWWriteASDBData(pLingInfo,
                (void ET9FARDATA*)&pASDB->wEntryCount,
                        (const void ET9FARDATA *)&wTemp, sizeof(wTemp));

            /* set remaining memory to full size of ASDB data area */
            wTemp = (ET9U16)ET9ASDBDataAreaBytes(pASDB);
            /* reduce by the number of single-byte range markers */
            wTemp -= ET9NUMASDBSIZERANGES;
            __ET9AWWriteASDBData(pLingInfo,
                        (void ET9FARDATA*)&pASDB->wRemainingMemory,
                        (const void ET9FARDATA *)&wTemp, sizeof(wTemp));

            /* loop through all LDB AS tracking structs */
            pLDBMap = pASDB->sLdbASRecord;
            pLDBTracker = pASDB->wLDBUseTracker;
            wTemp = 0;
            for (nIndex = 0; nIndex < ET9MAXASDBLANGUAGERECORDS; ++nIndex, ++pLDBMap, ++pLDBTracker) {
                /* clear the 6 byte header for the entry */
                __ET9AWWriteASDBData(pLingInfo,
                            (void ET9FARDATA *)pLDBMap,
                            (const void ET9FARDATA *)&dwClear, sizeof(dwClear));
                __ET9AWWriteASDBData(pLingInfo,
                            (void ET9FARDATA *)&pLDBMap->wEnabledRecords,
                            (const void ET9FARDATA *)&wTemp, sizeof(wTemp));

                /* clear the corresponding position tracker entry */
                __ET9AWWriteASDBData(pLingInfo,
                            (void ET9FARDATA *)pLDBTracker,
                            (const void ET9FARDATA *)&wTemp, sizeof(wTemp));

                pByte = pLDBMap->bMap;
                /* clear the bitmap of active LDB AS records in 4 byte chunks */
                for (nIndex2 = 0; nIndex2 < (ET9MAXLDBSUPPORTEDASRECORDS/8)/sizeof(dwClear); ++nIndex2, pByte += sizeof(dwClear)) {
                    __ET9AWWriteASDBData(pLingInfo,
                            (void ET9FARDATA *)pByte,
                            (const void ET9FARDATA *)&dwClear, sizeof(dwClear));
                }
            }

            /* set up size region indices and free records in database.
               Initialize size regions with a single free byte at
               the beginning that will remain forever. */
            pbNext = ET9ASDBData(pASDB);
            for (nIndex = 0; nIndex < ET9NUMASDBSIZERANGES; ++nIndex) {
                /* initially make size region sizes equitable */
                wOffset = (ET9U16)(nIndex * (ET9ASDBDataAreaBytes(pASDB) / ET9NUMASDBSIZERANGES));
                __ET9AWWriteASDBData(pLingInfo,
                            (void ET9FARDATA*)&pASDB->wSizeOffset[nIndex],
                            (const void ET9FARDATA *)&wOffset, sizeof(wOffset));
                /* region size will be same for all except (possibly) last region */
                nLength = (nIndex == (ET9NUMASDBSIZERANGES - 1)) ?
                                        (ET9ASDBDataAreaBytes(pASDB) - wOffset) :
                                        (ET9ASDBDataAreaBytes(pASDB) / ET9NUMASDBSIZERANGES);
                /* write single byte free record as region marker */
                __ET9AWASDBWriteHeader(pLingInfo, pbNext, ET9ASSINGLEFREETYPE, 1, 0, 0);
                ++pbNext;
                __CheckForASDBWrap_ByteP(pbNext, pASDB);
                /* write single multi-byte free record that covers entire region.           */
                /* OK to leave 'dirty' bytes in record... they're not included in checksum */
                __ET9AWASDBWriteHeader(pLingInfo, pbNext, ET9ASMULTIFREETYPE, nLength - 1, 0, 0);
                pbNext += (nLength - 1);
                __CheckForASDBWrap_ByteP(pbNext, pASDB);
            }

            /* now recalculate entire ASDB checksum, and store it */
            wTemp = (ET9U16) __ET9AWGetASDBChecksum(pLingInfo);
            __ET9AWWriteASDBData(pLingInfo, (void ET9FARDATA*)&pASDB->wDataCheck,
                            (const void ET9FARDATA *)&wTemp, sizeof(wTemp) );

            /* Reset ASDB entry (tracking vars for traversing DBs)  */
            pLingInfo->pLingCmnInfo->Private.pASDBGetEntry = 0;
            pLingInfo->pLingCmnInfo->Private.wLdbASGetEntryRec = 0;

            /* now re-init current language if LDB actually had AS entries */
            if (wTotalCount) {
                _ET9AWLdbASInit(pLingInfo, pLingInfo->pLingCmnInfo->wLdbNum, 1);
            }
            /* default to enabled user and LDB supported AS */
            ET9AWSetUserDefinedAutoSubstitution(pLingInfo);
            ET9AWSetLDBAutoSubstitution(pLingInfo);

        }
    }
    return wStatus;
}

/*---------------------------------------------------------------------------*/
/**
 * Iinitializes the auto substitution database.
 *
 * @param pLingInfo      Pointer to alphabetic information structure.
 * @param pASDBInfo      Pointer to ASDB info.
 * @param wDataSize      Size to expect for ASDB.
 * @param pWriteCB       Callback routine pointer (if integration layer handling ASDB writes).
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL ET9AWASDBInit(ET9AWLingInfo * const                pLingInfo,
                                   ET9AWASDBInfo ET9FARDATA * const     pASDBInfo,
                                   const ET9U16                         wDataSize,
                                   const ET9DBWRITECALLBACK             pWriteCB)
{
    ET9STATUS       wStatus;

    if ((wStatus = _ET9AWSys_BasicValidityCheck(pLingInfo)) == ET9STATUS_NONE) {
        if (((pASDBInfo != NULL) && !wDataSize) || ((pASDBInfo == NULL) && wDataSize)) {
            wStatus = ET9STATUS_INVALID_MEMORY;
        }
        /* Check for minimum ASDB size */
        else if ((pASDBInfo != NULL) && (wDataSize < ET9MINASDBDATABYTES)) {
             wStatus = ET9STATUS_INVALID_SIZE;
        }
        else {
            /* if overwriting already initialized ASDB pointer, */
            /* return indication but continue with init         */
            if ((pLingInfo->pLingCmnInfo->pASDBInfo != NULL) && (pASDBInfo != NULL) &&
               ((pLingInfo->pLingCmnInfo->pASDBInfo != pASDBInfo) ||
                (pASDBInfo->wDataSize != wDataSize))) {
                wStatus = ET9STATUS_ALREADY_INITIALIZED;
            }
            pLingInfo->pASDBWriteData = pWriteCB;
            pLingInfo->pLingCmnInfo->pASDBInfo = pASDBInfo;

            /* ok to have no ASDB */
            if (pASDBInfo != NULL) {
                pLingInfo->pLingCmnInfo->Private.pASDBGetEntry = 0;
                pLingInfo->pLingCmnInfo->Private.wLdbASGetEntryRec = 0;

                /*  If size or checksum doesn't match, set up an empty ASDB.*/
                if ((pASDBInfo->wDataSize != wDataSize) ||
                    (pASDBInfo->wDataCheck != (ET9U16)__ET9AWGetASDBChecksum(pLingInfo))) {

                    __ET9AWWriteASDBData(pLingInfo,
                            (void ET9FARDATA*)&pASDBInfo->wDataSize,
                            (const void ET9FARDATA *)&wDataSize, sizeof(wDataSize));
                    ET9AWASDBReset(pLingInfo);
                }

                /* Check for current LDB based AS support */
               _ET9AWLdbASInit(pLingInfo, pLingInfo->pLingCmnInfo->wLdbNum, 0);

               /* default to enabled user supported AS */
               ET9AWSetUserDefinedAutoSubstitution(pLingInfo);
            }
            else {
                /* Check for current LDB based AS support */
               _ET9AWLdbASInit(pLingInfo, pLingInfo->pLingCmnInfo->wLdbNum, 0);
            }

            /* default to enabled LDB supported AS */
            ET9AWSetLDBAutoSubstitution(pLingInfo);

            pLingInfo->pLingCmnInfo->Private.bStateASDBEnabled = 1;

        }
    }
    return wStatus;
}

/*---------------------------------------------------------------------------*/
/**
 * Get number of defined ASDB entries.
 * This includes user-defined entries in the ASDB and ENABLED<BR>
 * LDB-supported entries in the current LDB.<BR>
 * NOTE: Count adjusted based on whether User and/or LDB portion of ASDB enabled or not.
 *
 * @param pLingInfo     Pointer to alphabetic information structure.
 * @param pEntryCount   Pointer to receive entry count.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL ET9AWASDBGetEntryCount(ET9AWLingInfo * const   pLingInfo,
                                            ET9U16 * const          pEntryCount)
{
    ET9STATUS                   wStatus;
    ET9LdbASRecMap ET9FARDATA * pLDB;

    if ((wStatus = _ET9AWSys_BasicValidityCheck(pLingInfo)) == ET9STATUS_NONE) {
        if (pEntryCount == NULL) {
            wStatus = ET9STATUS_INVALID_MEMORY;
        }
        else if (pLingInfo->pLingCmnInfo->pASDBInfo == NULL) {
            if ( !ET9LDBSUPPORTEDAUTOSUBENABLED(pLingInfo->pLingCmnInfo) ){
                wStatus = ET9STATUS_NO_ASDB;
            }
        }
        if (!wStatus) {
            *pEntryCount = 0;
            if (pLingInfo->pLingCmnInfo->pASDBInfo != NULL) {
                /* first, get the count of user-defined AS records (IF enabled) */
                if (ET9USERDEFINEDAUTOSUBENABLED(pLingInfo->pLingCmnInfo)) {
                    *pEntryCount = pLingInfo->pLingCmnInfo->pASDBInfo->wEntryCount;
                }
                /* then look for tracking struct of this language's LDB AS info */
                if (ET9LDBSUPPORTEDAUTOSUBENABLED(pLingInfo->pLingCmnInfo)) {
                    pLDB = __ET9AWGetLdbASEntry(pLingInfo->pLingCmnInfo->pASDBInfo, pLingInfo->pLingCmnInfo->wLdbNum);
                    /* if struct exists (language had AS records to be tracked) */
                    if (pLDB) {
                        /* increment the returned count with only the ENABLED count of LDB-AS records */
                        *pEntryCount = *pEntryCount + (ET9U16)pLDB->wEnabledRecords;
                    }
                }
            }
            else if (ET9LDBSUPPORTEDAUTOSUBENABLED(pLingInfo->pLingCmnInfo)) {
                *pEntryCount = pLingInfo->pLingCmnInfo->Private.sLDBAutoSub.wNumEntries;
            }
        }

    }
    return wStatus;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Move LDB ID to first position as most recently used.
 * This is done so that oldest (least used) LDB tracking structs can be determined for garbage collection
 *
 * @param pLingInfo      Pointer to alphabetic information structure.
 * @param pASDB          Pointer to ASDB.
 * @param wLDBID         LDB ID being repositioned.
 *
 * @return None
 */

static void ET9LOCALCALL __ET9AWReorderLDBIDTracking(ET9AWLingInfo *pLingInfo,
                                                     ET9AWASDBInfo ET9FARDATA *pASDB,
                                                     ET9U16        wLDBID)
{
    ET9U16  ET9FARDATA  *pLDBTracker;
    ET9U16  ET9FARDATA  *pLDBPrevTracker;
    ET9U16  wPrevLDBID;
    ET9UINT i;

    /* loop through tracking array of LDB IDs */
    pLDBTracker = pASDB->wLDBUseTracker;
    for (i = 0; i < ET9MAXASDBLANGUAGERECORDS; ++i, ++pLDBTracker) {
        /* if location found, break out */
        if (wLDBID == *pLDBTracker) {
            break;
        }
    }
    /* use assert, since never should be calling this for LDB that DOESN'T have an entry */
    ET9Assert(i < ET9MAXASDBLANGUAGERECORDS);

    /* only need to reposition entry if it's not already the most recently set language */
    /* no need to update checksum since just rearranging entries                        */
    if (i != 0) {
        /* run loop to transfer all IDs ahead of this entry down one; then write this   */
        /* LDB ID in the first slot as the most recently used                           */
        pLDBPrevTracker = pLDBTracker;
        for (; i; --i, --pLDBTracker) {
            /* NOTE: This is done here to avoid diagnostic screaming about out-of-bounds ptr */
            --pLDBPrevTracker;
            wPrevLDBID = *pLDBPrevTracker;
            __ET9AWWriteASDBData(pLingInfo, (ET9U8 ET9FARDATA *)pLDBTracker,
                                 (const void ET9FARDATA *)&wPrevLDBID, sizeof(wPrevLDBID));

        }
        __ET9AWWriteASDBData(pLingInfo, (ET9U8 ET9FARDATA *)pLDBTracker, (const void ET9FARDATA *)&wLDBID, sizeof(wLDBID));
    }
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Move LDB ID to last position as least recently used.
 * This is done so that oldest (least used) LDB tracking structs can be determined for garbage collection
 *
 * @param pLingInfo          Pointer to alphabetic information structure.
 * @param pASDB              Pointer to ASDB.
 * @param wLDBID             LDB ID being repositioned.
 *
 * @return None
 */

static void ET9LOCALCALL __ET9AWReorderToEndLDBIDTracking(ET9AWLingInfo *pLingInfo,
                                                     ET9AWASDBInfo ET9FARDATA *pASDB,
                                                     ET9U16        wLDBID)
{
    ET9U16  ET9FARDATA  *pLDBTracker;
    ET9U16  ET9FARDATA  *pLastEntry = 0;
    ET9U16  wLDBID2 = 0;
    ET9UINT i;

    /* loop through tracking array of LDB IDs */
    pLDBTracker = &pASDB->wLDBUseTracker[ET9MAXASDBLANGUAGERECORDS-1];
    for (i = 0; i < ET9MAXASDBLANGUAGERECORDS; ++i, --pLDBTracker) {
        /* save pointer to least recently used position */
        if (!pLastEntry && *pLDBTracker) {
            pLastEntry = pLDBTracker;
            wLDBID2 = *pLDBTracker;
        }
        /* if location found, break out */
        if (wLDBID == *pLDBTracker) {
            break;
        }
    }
    /* use assert, since never should be calling this for LDB that DOESN'T have an entry */
    ET9Assert(i < ET9MAXASDBLANGUAGERECORDS);
    ET9Assert(wLDBID2);

    /* only need to reposition entry if it's not already the least recently set language */
    /* no need to update checksum since just rearranging entries                         */
    if (wLDBID != wLDBID2) {
        /* for simplicity, just switch positions */
        __ET9AWWriteASDBData(pLingInfo, (ET9U8 ET9FARDATA *)pLDBTracker, (const void ET9FARDATA *)&wLDBID2, sizeof(wLDBID2));
        __ET9AWWriteASDBData(pLingInfo, (ET9U8 ET9FARDATA *)pLastEntry, (const void ET9FARDATA *)&wLDBID, sizeof(wLDBID));
    }
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Find/assign a record for the given LDB num in the ASDB.
 *
 * @param pLingInfo      Pointer to field information structure.
 * @param wLDBID         LDB ID being searched for (or added).
 *
 * @return None
 */

static void ET9LOCALCALL __ET9AWUpdateTracker(ET9AWLingInfo *pLingInfo,
                                              ET9U16         wLDBID)
{
    ET9LdbASRecMap ET9FARDATA  *pLDB;
    ET9U16         ET9FARDATA  *pLDBTracker;
    ET9AWASDBInfo  ET9FARDATA  * const pASDB = pLingInfo->pLingCmnInfo->pASDBInfo;
    ET9UINT        i = 0;
    ET9U16         wDelLDBID;
    ET9U16         wChecksum;

    /* go look for the LDB's tracking record in the ASDB */
    pLDB = __ET9AWGetLdbASEntry(pASDB, wLDBID);

    /* if not already defined, make sure there's an available spot; if not,   */
    /* delete the entry for the least used language                           */
    if (!pLDB) {
        pLDBTracker = pASDB->wLDBUseTracker;
        for (i = 0; i < ET9MAXASDBLANGUAGERECORDS; ++i, ++pLDBTracker) {
            /* if blank entry found, stuff LDB ID in there */
            if (!*pLDBTracker) {
                wChecksum = (ET9U16)(pASDB->wDataCheck + wLDBID);
                __ET9AWWriteASDBData(pLingInfo, (ET9U8 ET9FARDATA *)pLDBTracker,
                             (const void ET9FARDATA *)&wLDBID, sizeof(wLDBID));
                break;
            }
        }
        /* if no entries available, delete the least used language and use that slot */
        if (i == ET9MAXASDBLANGUAGERECORDS) {
            /* least used language ID will be in the last array position */
            pLDBTracker = &pASDB->wLDBUseTracker[ET9MAXASDBLANGUAGERECORDS - 1];
            wDelLDBID = *pLDBTracker;
            /* find the sLdbASRecord for the LDB about to be removed */
            pLDB = __ET9AWGetLdbASEntry(pASDB, wDelLDBID);
            ET9Assert(pLDB);
            /* checksum changed by clearing the sLdbASRecord wLDBID field and */
            /* overwriting the tracker array position with the new LDB ID     */
            wChecksum = (ET9U16)(pASDB->wDataCheck - wDelLDBID - wDelLDBID + wLDBID);
            wDelLDBID = 0;
            /* now clear those positions (new ID loaded in tracker position) */
            __ET9AWWriteASDBData(pLingInfo, (ET9U8 ET9FARDATA *)pLDBTracker, (const void ET9FARDATA *)&wLDBID, sizeof(wLDBID));
            __ET9AWWriteASDBData(pLingInfo, (ET9U8 ET9FARDATA *)&pLDB->wLDBID, (const void ET9FARDATA *)&wDelLDBID, sizeof(wDelLDBID));
        }
        /* write new checksum */
        __ET9AWWriteASDBData(pLingInfo, (void ET9FARDATA*)&pASDB->wDataCheck, (const void ET9FARDATA *)&wChecksum, sizeof(wChecksum));
    }
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Set up a record for the current LDB in the ASDB.
 *
 * @param pLingInfo      Pointer to field information structure.
 * @param wLdbNum        LDB number.
 * @param wReset         If non-zero, reset LDB-AS records all enabled (still disables duplicates found in user ASDB).
 *
 * @return None
 */

void ET9FARCALL _ET9AWLdbASInit(ET9AWLingInfo   *pLingInfo,
                                ET9U16          wLdbNum,
                                ET9U16          wReset)
{
    ET9AWASDBInfo  ET9FARDATA  *pASDB;
    ET9LdbASRecMap ET9FARDATA  *pLDB;
    ET9UINT             i = 0;
    ET9UINT             j;
    ET9U16              wChecksum;
    ET9U8              *pByte;
    ET9U8               bHolder = 0xFF;
    ET9SYMB            *pTarget;
    ET9U8 ET9FARDATA   *pbNext;
    ET9U8 ET9FARDATA   *pbEnd;
    ET9U16              wRegion;
    ET9SYMB ET9FARDATA *pSymb;
    ET9UINT             nRecordType;
    ET9ASDBRecordHeader sRecHeader;
    ET9U16              wWordSize;
    ET9U16              wRecordNum;
    ET9U16              wLDBID;
    ET9U8               bChunkCt;
    ET9U8               bChunkID;
    ET9U32              dwAddressTracker;
    ET9U32              dwChunkSize;
    ET9U8               bASDBChunkFound = 0;
    const ET9U8 ET9FARDATA *pbByte;
    ET9SYMB             sString[ET9MAXWORDSIZE];

    ET9AWLingCmnInfo * const pLingCmnInfo = pLingInfo->pLingCmnInfo;

    ET9Assert(pLingInfo);

    /* assume LDB auto-substitution NOT supported by clearing struct */
    _ET9ClearMem((ET9U8 *)&pLingCmnInfo->Private.sLDBAutoSub, sizeof(ET9ALDBAutoSub));
    /* restart any active 'GetEntry' sequence if LDB changed */
    pLingCmnInfo->Private.pASDBGetEntry = 0;
    pLingCmnInfo->Private.wLdbASGetEntryRec = 0;

#ifdef ALDB_LAYOUT_VERSION_4_SUPPORT
    dwAddressTracker = ET9LDBOFFSET_BODY;
#else /* ALDB_LAYOUT_VERSION_4_SUPPORT */
    dwAddressTracker = pLingCmnInfo->Private.ALdb.header.pdwIntervalOffsets[pLingCmnInfo->Private.ALdb.header.bPosCount];
#endif /* ALDB_LAYOUT_VERSION_4_SUPPORT */

    /* if ASDB exists and LDB has been initialized/validated */

    if ((wLdbNum & ET9PLIDMASK) != ET9PLIDNone &&
        pLingInfo->Private.wLDBInitOK == ET9GOODSETUP) {

        /* check to see if LDB has AS chunk */

        bChunkCt = _ET9ReadLDBByte(pLingInfo, ET9LDBOFFSET_CHUNK_COUNT_BYTE);

        if (_ET9ReadLDBByte(pLingInfo, ET9LDBOFFSET_LDBLAYOUTVER) <= 3) {
            bChunkCt >>= 2;
        }

        /* loop through LDB's chunks, looking for the ASDB_CHUNK_ID */

        while (bChunkCt--) {

            bChunkID = _ET9ReadLDBByte(pLingInfo, dwAddressTracker + CHUNK_ID_OFFSET);

            /* if AutoSub info chunk found in LDB */

            if (bChunkID == ASDB_CHUNK_ID) {

                bASDBChunkFound = 1;

                /* save the LDB specific info in the lingustics struct (NOT to be confused with  */
                /* LDB tracking struct maintained in the ASDB)                                   */

                pLingCmnInfo->Private.sLDBAutoSub.bSupported = 1;
                pLingCmnInfo->Private.sLDBAutoSub.bLSASDBVersion = _ET9ReadLDBByte (pLingInfo, dwAddressTracker + LSASDB_VERSION_OFFSET);
                pLingCmnInfo->Private.sLDBAutoSub.bLSASDBLangID = _ET9ReadLDBByte (pLingInfo, dwAddressTracker + LSASDB_LANGUAGE_OFFSET);
                pLingCmnInfo->Private.sLDBAutoSub.dwLSASDBStartAddress = _ET9ReadLDBWord3(pLingInfo, dwAddressTracker + LSASDB_START_ADDRESS_OFFSET);
                pLingCmnInfo->Private.sLDBAutoSub.dwLSASDBEndAddress = _ET9ReadLDBWord3(pLingInfo, dwAddressTracker + LSASDB_END_ADDRESS_OFFSET);
                pLingCmnInfo->Private.sLDBAutoSub.wNumEntries = _ET9ReadLDBWord2(pLingInfo, dwAddressTracker + LSASDB_NUM_ENTRIES_OFFSET);

                wLDBID = wLdbNum & ET9PLIDMASK;   /* isolate the primary ID */
                pASDB = pLingCmnInfo->pASDBInfo;
                if (pASDB != NULL) {
                    /* go make sure LDB has tracking entry */
                    __ET9AWUpdateTracker(pLingInfo, wLDBID);

                    /* go look for the LDB's tracking record in the ASDB */
                    pLDB = __ET9AWGetLdbASEntry(pASDB, wLDBID);


                    /* if not already defined, resetting, or the existing record has discrepancy */
                    if (!pLDB || wReset || (pLDB->wTotalRecords != pLingCmnInfo->Private.sLDBAutoSub.wNumEntries)) {
                        /* loop through and get new (or existing) record */
                        pLDB = pASDB->sLdbASRecord;
                        for (i = 0; i < ET9MAXASDBLANGUAGERECORDS; ++i, ++pLDB) {
                            /* if entry is available */
                            if (!pLDB->wLDBID || pLDB->wLDBID == wLDBID) {
                                /* update checksum with new LDB ID (may just be overwriting same value here) */
                                wChecksum = (ET9U16)(pASDB->wDataCheck - pLDB->wLDBID + wLDBID);
                                __ET9AWWriteASDBData(pLingInfo, (ET9U8 ET9FARDATA *)&pLDB->wLDBID,
                                             (const void ET9FARDATA *)&wLDBID, sizeof(wLDBID));

                                /* set enabled records == total records */
                                wChecksum = (ET9U16)(wChecksum - pLDB->wEnabledRecords +
                                                    pLingCmnInfo->Private.sLDBAutoSub.wNumEntries);
                                __ET9AWWriteASDBData(pLingInfo, (void ET9FARDATA*)&pLDB->wEnabledRecords,
                                        (const void ET9FARDATA *)&pLingCmnInfo->Private.sLDBAutoSub.wNumEntries,
                                        sizeof(pLDB->wEnabledRecords));

                                /* set total records */
                                wChecksum = (ET9U16)(wChecksum - pLDB->wTotalRecords +
                                                    pLingCmnInfo->Private.sLDBAutoSub.wNumEntries);
                                __ET9AWWriteASDBData(pLingInfo, (void ET9FARDATA*)&pLDB->wTotalRecords,
                                        (const void ET9FARDATA *)&pLingCmnInfo->Private.sLDBAutoSub.wNumEntries,
                                        sizeof(pLDB->wTotalRecords));

                                /* now loop through bitmap array of enabled records and set all to 'on' */
                                pByte = pLDB->bMap;
                                for (j = 0; j < (ET9MAXLDBSUPPORTEDASRECORDS/8); ++j, ++pByte) {
                                    wChecksum = (ET9U16)(wChecksum - *pByte + bHolder);
                                    __ET9AWWriteASDBData(pLingInfo, (void ET9FARDATA*)pByte,
                                                (const void ET9FARDATA *)&bHolder, 1);
                                }

                                /* write new checksum */
                                __ET9AWWriteASDBData(pLingInfo, (void ET9FARDATA*)&pASDB->wDataCheck,
                                    (const void ET9FARDATA *)&wChecksum, sizeof(wChecksum));
                                break;
                            }
                        }
                    }

                    ET9Assert(i < ET9MAXASDBLANGUAGERECORDS);


                    /* update LDBTracker array to put this language as most recent */
                    __ET9AWReorderLDBIDTracking(pLingInfo, pASDB, wLDBID);

                    /* if everything ok to this point, need to check for LDB-supported           */
                    /* shortcuts that are duplicates of user-defined shortcuts, and disable them */
                    if (i < ET9MAXASDBLANGUAGERECORDS) {
                        /* get start/end pointers that cover entire ASDB */
                        __ET9AWSetASDBRangeInfo(pLingInfo, 2, 1, &wRegion, &pbNext, &pbEnd);
                        /* Skip past single byte region indicator record */
                        ++pbNext;
                        __CheckForASDBWrap_ByteP(pbNext, pASDB);

                        /* loop through qualifying size regions in the ASDB */
                        while (pbNext != pbEnd) {
                            /* if this is an ASDB record */
                            nRecordType = __ET9AWASGetRecordInfo(pASDB, pbNext, &j);
                            if (nRecordType == ET9ASDBTYPE) {
                                /* in next call, pSymb will point to shortcut word */
                                pSymb  = (ET9SYMB *)__ET9AWReadASDBRecordHeader(pASDB, pbNext, &sRecHeader);
                                wWordSize = (ET9U16)sRecHeader.bShortcutLen;
                                /* assuming context word not being used since LDB being initialized */
                                pTarget = sString;
                                /* copy the shortcut word */
                                for (i = 0; i < wWordSize; ++i) {
                                    /* ReadSymbol */
                                    pbByte = (const ET9U8 ET9FARDATA *)pSymb + 1;
                                    __CheckForASDBWrap_ByteP(pbByte, pASDB);
                                    *pTarget++ = ET9MAKEWORD(*((const ET9U8 ET9FARDATA *)pSymb), *pbByte);

                                    ++pSymb;
                                    __CheckForASDBWrap_WordP(pSymb, pASDB);
                                }
                                /* check LDB-supported entries to see if same shortcut word defined; */
                                /* if it is, set it as 'removed' for this language                   */
                                if (!_ET9AWLdbASFindEntry(pLingInfo, wLdbNum, sString,
                                    wWordSize, 0, 0, &wRecordNum)) {
                                    _ET9AWLdbASDisableRecord(pLingInfo, wLdbNum, wRecordNum);
                                }
                            }
                            /* move to the next record */
                            pbNext += j;
                            __CheckForASDBWrap_ByteP(pbNext, pASDB);

                        }
                    }
                }
            }
            /* skip to next chunk in LDB */
            {
                dwChunkSize = _ET9ReadLDBWord3(pLingInfo, dwAddressTracker + CHUNK_SIZE_OFFSET);
                dwAddressTracker += dwChunkSize;
            }
        }
        if (!bASDBChunkFound) {
            pLingCmnInfo->Private.sLDBAutoSub.bSupported = 0;
            pLingCmnInfo->Private.sLDBAutoSub.wNumEntries = 0;

            wLDBID = wLdbNum & ET9PLIDMASK;   /* isolate the primary ID */
            pASDB = pLingCmnInfo->pASDBInfo;
            /* this is unlikely, but _could_ happen (new LDB with NO LDB-AS entries) */
            if (pASDB != NULL) {
                /* go look for the LDB's tracking record in the ASDB */
                pLDB = __ET9AWGetLdbASEntry(pASDB, wLDBID);
                if (pLDB && pLDB->wTotalRecords) {
                    __ET9AWReorderToEndLDBIDTracking(pLingInfo, pASDB, wLDBID);
                    wLDBID = 0;
                    /* set enabled records == 0 */
                    wChecksum = (ET9U16)(pASDB->wDataCheck - pLDB->wEnabledRecords);
                    __ET9AWWriteASDBData(pLingInfo, (void ET9FARDATA*)&pLDB->wEnabledRecords,
                            (const void ET9FARDATA *)&wLDBID, sizeof(wLDBID));

                    /* set total records == 0 */
                    wChecksum = (ET9U16)(wChecksum - pLDB->wTotalRecords);
                    __ET9AWWriteASDBData(pLingInfo, (void ET9FARDATA*)&pLDB->wTotalRecords,
                            (const void ET9FARDATA *)&wLDBID, sizeof(wLDBID));

                    /* now loop through bitmap array of enabled records and set all to 'off' */
                    pByte = pLDB->bMap;
                    bHolder = 0;
                    for (j = 0; j < (ET9MAXLDBSUPPORTEDASRECORDS/8); ++j, ++pByte) {
                        wChecksum = (ET9U16)(wChecksum - *pByte);
                        __ET9AWWriteASDBData(pLingInfo, (void ET9FARDATA*)pByte,
                                    (const void ET9FARDATA *)&bHolder, 1);
                    }

                    /* write new checksum */
                    __ET9AWWriteASDBData(pLingInfo, (void ET9FARDATA*)&pASDB->wDataCheck,
                        (const void ET9FARDATA *)&wChecksum, sizeof(wChecksum));
                }
            }
        }
    }
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Disable an auto-sub record for the given LDB in the ASDB
 *
 * @param pLingInfo          Pointer to field information structure.
 * @param wLDBID             LDB ID for setup.
 * @param wASRecordNum       0-relative record number to be disabled.
 *
 * @return None
 */

void ET9FARCALL _ET9AWLdbASDisableRecord(ET9AWLingInfo *pLingInfo,
                                         ET9U16         wLDBID,
                                         ET9U16         wASRecordNum)
{
    ET9AWASDBInfo   ET9FARDATA *pASDB;
    ET9LdbASRecMap  ET9FARDATA *pLDB;
    ET9U16           wChecksum;
    ET9U16           wIndex, wNewRec;
    ET9U8            bPosition, bNewRec;

    ET9AWLingCmnInfo * const pLingCmnInfo = pLingInfo->pLingCmnInfo;

    ET9Assert(pLingInfo);
    wLDBID &= ET9PLIDMASK;   /* isolate the primary ID */
    ET9Assert(wLDBID);

    /* if ASDB defined */
    if (pLingCmnInfo->pASDBInfo != NULL) {
        pASDB = pLingCmnInfo->pASDBInfo;
        /* get the pointer to the given language's LDB AS tracking struct in the ASDB */
        pLDB = __ET9AWGetLdbASEntry(pASDB, wLDBID);
        ET9Assert(!pLDB || wASRecordNum < pLDB->wTotalRecords);
        /* if defined AND the record number is in bounds */
        if (pLDB && wASRecordNum < pLDB->wTotalRecords) {
            /* get offset for record flag's byte by dividing the record num by 8 */
            wIndex = wASRecordNum >> 3;
            /* bit position within that byte is the mod 8 */
            bPosition = (ET9U8)(wASRecordNum & 0x0007);
            /* if record is actually enabled */
            if (pLDB->bMap[wIndex] & (1 << bPosition)) {
                wChecksum = pASDB->wDataCheck;
                bNewRec = pLDB->bMap[wIndex];
                /* adjust the checksum by replacing byte with updated byte */
                wChecksum = (ET9U16)(wChecksum - bNewRec);
                bNewRec = (ET9U8)(bNewRec & ~(1 << bPosition));
                wChecksum = (ET9U16)(wChecksum + bNewRec);
                /* now write the updated byte */
                __ET9AWWriteASDBData(pLingInfo, (void ET9FARDATA*)&pLDB->bMap[wIndex],
                        (const void ET9FARDATA *)&bNewRec, 1);
                ET9Assert(pLDB->wEnabledRecords);
                /* also have to adjust the count of enabled LDB AS records (decrement by 1) */
                wChecksum--;
                wNewRec = pLDB->wEnabledRecords - 1;
                __ET9AWWriteASDBData(pLingInfo, (void ET9FARDATA*)&pLDB->wEnabledRecords,
                        (const void ET9FARDATA *)&wNewRec, sizeof(wNewRec));

                /* write new checksum */
                __ET9AWWriteASDBData(pLingInfo, (void ET9FARDATA*)&pASDB->wDataCheck,
                        (const void ET9FARDATA *)&wChecksum, sizeof(wChecksum));
            }
        }
    }
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Returns the current setting for an auto-sub record for the given LDB in the ASDB
 *
 * @param pLingInfo      Pointer to field information structure.
 * @param wLDBID         LDB ID for record.
 * @param wASRecordNum   0-relative record number being checked.
 *
 * @return 1 if enabled, 0 if disabled.
 */

ET9U8 ET9FARCALL _ET9AWLdbASRecordEnabled(ET9AWLingInfo *pLingInfo,
                                          ET9U16         wLDBID,
                                          ET9U16         wASRecordNum)
{
    ET9LdbASRecMap  ET9FARDATA *pLDB;
    ET9U8            wIndex;
    ET9U8            bPosition;
    ET9U8            bResult = 0;

    ET9AWLingCmnInfo * const pLingCmnInfo = pLingInfo->pLingCmnInfo;

    ET9Assert(pLingInfo);
    wLDBID &= ET9PLIDMASK;   /* isolate the primary ID */
    ET9Assert(wLDBID);

    if (pLingCmnInfo->pASDBInfo != NULL) {
        pLDB = __ET9AWGetLdbASEntry(pLingCmnInfo->pASDBInfo, wLDBID);
        ET9Assert(!pLDB || wASRecordNum < pLDB->wTotalRecords);
        /* if defined AND the record number is in bounds */
        if (pLDB && (wASRecordNum < pLDB->wTotalRecords)) {
            /* get offset for record flag's byte by dividing the record num by 8 */
            wIndex = (ET9U8)(wASRecordNum >> 3);
            /* bit position within that byte is the mod 8 */
            bPosition = (ET9U8)(wASRecordNum & 0x0007);
            /* if record enabled, set return value */
            if (pLDB->bMap[wIndex] & (1 << bPosition)) {
                bResult = 1;
            }
        }
    }
    else {
        bResult = 1;
    }

    return bResult;
}


/*---------------------------------------------------------------------------*/
/** \internal
 * Search ASDB for user-defined selection list candidates.
 *
 * @param pLingInfo          Pointer to alpha information structure.
 * @param wIndex             Index for start of current word comparison.
 * @param wLength            Length of current word.
 * @param bFreqIndicator     Pass-through param for ET9AWSelLstWordSearch.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL _ET9AWASDBWordsSearch(ET9AWLingInfo        *pLingInfo,
                                           ET9U16               wIndex,
                                           ET9U16               wLength,
                                           ET9_FREQ_DESIGNATION bFreqIndicator)

{
    ET9STATUS          wStatus = ET9STATUS_NONE;
    ET9U8 ET9FARDATA  *pbNext;
    ET9U8 ET9FARDATA  *pbEnd;
    ET9UINT            nRecordLength;
    ET9UINT            nRecordType;
    ET9U16             wRegion;
    ET9UINT            i;
    ET9U8 ET9FARDATA  *pSymb;
    ET9SYMB           *pHolder;
    ET9ASDBRecordHeader sRecHeader;
    const ET9U8 ET9FARDATA *pbByte;
    ET9U8              bFound;

    ET9AWASDBInfo ET9FARDATA * const pASDB = pLingInfo->pLingCmnInfo->pASDBInfo;

    const ET9BOOL bUsingLM = (ET9BOOL)(ET9LMENABLED(pLingInfo->pLingCmnInfo) && pLingInfo->pLingCmnInfo->Private.ALdbLM.bSupported);

    ET9Assert(pLingInfo);

    /* allow search for stems of size 1; also, if no */
    /* ASDB to begin with, get outta town */

    if (!wLength || !pASDB || ! ET9ASDBENABLED(pLingInfo->pLingCmnInfo)) {
        return wStatus;
    }

    /* get the pointers for the ranges in ASDB to traverse */

    __ET9AWSetASDBRangeInfo(pLingInfo,
                            pLingInfo->pLingCmnInfo->Private.wCurrMinSourceLength,
                            1 /* bIncludeCompletions */,
                            &wRegion,
                            &pbNext,
                            &pbEnd);

    /* Skip past single byte region indicator record */

    ++pbNext;
    __CheckForASDBWrap_ByteP(pbNext, pASDB);

    /* loop through qualifying size regions in the ASDB */

    while (pbNext != pbEnd) {

        /* if this is an ASDB record */

        nRecordType = __ET9AWASGetRecordInfo(pASDB, pbNext, &nRecordLength);

        if (nRecordType == ET9ASDBTYPE) {

            ET9AWPrivWordInfo sLocalWord;

            /* Parse ASDB record for possible addition to selection list */
            /* get the length of the ASDB word */

            pSymb = __ET9AWReadASDBRecordHeader(pASDB, pbNext, &sRecHeader);

            /* NOTE: NEED TO KNOW WHAT FREQUENCY TO PUT HERE!!! */

            _InitPrivWordInfo(&sLocalWord);

            sLocalWord.xWordFreq = (ET9FREQPART)0x3FFF;
            sLocalWord.xTapFreq = 1;
            sLocalWord.Base.wWordLen  = (ET9U16)sRecHeader.bShortcutLen;
            sLocalWord.bWordSrc = ET9WORDSRC_ASDB_SHORTCUT;
            if (ET9AWSys_GetBilingualSupported(pLingInfo)) {
                sLocalWord.Base.bLangIndex = ET9AWBOTH_LANGUAGES;
            }
            else {
                sLocalWord.Base.bLangIndex = ET9AWFIRST_LANGUAGE;
            }
            pHolder = sLocalWord.Base.sWord;

            /* loop through and transfer word */

            for (i = sRecHeader.bShortcutLen; i; --i) {

                /* ReadSymbol */

                pbByte = (const ET9U8 ET9FARDATA *)pSymb + 1;
                __CheckForASDBWrap_ByteP(pbByte, pASDB);
                *pHolder++ = ET9MAKEWORD(*((const ET9U8 ET9FARDATA *)pSymb), *pbByte);

                pSymb += ET9SYMBOLWIDTH;
                __CheckForASDBWrap_ByteP(pSymb, pASDB);
            }

            /* now transfer the associated substitution */

            pHolder = sLocalWord.Base.sSubstitution;
            sLocalWord.Base.wSubstitutionLen  = (ET9U16)sRecHeader.bSubstitutionLen;
            for (i = sRecHeader.bSubstitutionLen; i; --i) {

                /* ReadSymbol */

                pbByte = (const ET9U8 ET9FARDATA *)pSymb + 1;
                __CheckForASDBWrap_ByteP(pbByte, pASDB);
                *pHolder++ = ET9MAKEWORD(*((const ET9U8 ET9FARDATA *)pSymb), *pbByte);

                pSymb += ET9SYMBOLWIDTH;
                __CheckForASDBWrap_ByteP(pSymb, pASDB);
            }

            /* if word being compounded, downshift it */

            if (wIndex) {
                ET9SYMB *pSymb = sLocalWord.Base.sWord;
                ET9UINT i = sLocalWord.Base.wWordLen;

                for (; i; --i, ++pSymb) {
                    *pSymb = _ET9SymToLower(*pSymb, pLingInfo->pLingCmnInfo->wLdbNum);
                }
            }

            wStatus = _ET9AWSelLstWordMatch(pLingInfo, &sLocalWord, wIndex, wLength, &bFound);
            if (wStatus != ET9STATUS_NONE) {
                return wStatus;
            }

            if (bFound) {

                if (pLingInfo->pLingCmnInfo->Private.eCurrSelectionListMode == ET9ASLMODE_MIXED) {
                    if (bUsingLM) {
                        sLocalWord.xWordFreq = (ET9FREQPART)ET9_SUPP_DB_BASE_FREQ;
                    }
                    else {
                        sLocalWord.xWordFreq = (ET9FREQPART)ET9_SUPP_DB_BASE_FREQ_UNI;
                    }
                }

                _ET9AWSelLstAdd(pLingInfo, &sLocalWord, wLength, bFreqIndicator);

            }

        }

        /* move to the next record */

        pbNext += nRecordLength;
        __CheckForASDBWrap_ByteP(pbNext, pASDB);
    }

    return wStatus;
}

/*---------------------------------------------------------------------------*/
/**
 * Get ASDB entry.
 * Designed to be used repeatedly to retrieve all of
 * the ASDB entries contained in both the User ASDB and current LDB-AS (if it exists).<P>
 * The first time the routine is called, the value contained in *pwShortcutLen should
 * be 0; subsequent calls should contain the previously retrieved shortcut in
 * *psShortcutBuf and the size of that previously retrieved shortcut in *pwShortcutLen.<P>
 * If *pwShortcutLen is non-zero, function first compares the shortcut passed in
 * *psShortcutBuf against the record pointed to by the internally maintained (and private)
 * pASDBGetEntry (which points to the last processed User ASDB record). If they don't match,
 * it indicates an update has been made to the ASDB that would disrupt normal, consecutive
 * record processing, and the function restarts the process at the beginning of the ASDB
 * (and returns a status of ET9STATUS_WORD_NOT_FOUND along with the first ASDB record info).<P>
 * When processing the LDB-AS, the last processed record is maintained in the internal
 * (and private) wLdbASGetEntryRec.<P>
 * When processing 'forward', the routine will return the first USER AS record, proceed through
 * the USER ASDB, and then proceed through the current enabled LDB-AS records in the order in which
 * they occur in the LDB.<P>
 * When processing in 'reverse', the routine starts with the last enabled record in the current
 * LDB-AS, traverse to the first, and then word backwords through the USER-AS.<P>
 * If either the USER-AS or LDB-AS is disabled from inclusion, that portion of the ASDB
 * is skipped during the traversal, either forwards or backwards.<P>
 * If the API is called again after traversing all ASDB records, either forward or in reverse,
 * the routine will return the ET9STATUS_NO_MATCHING_WORDS status.
 *
 * @param pLingInfo            Pointer to alphabetic information structure.
 * @param psShortcutBuf        Pointer to buffer for loading shortcut word (should contain the previously retrieved shortcut text on subsequent calls).
 * @param wShortcutBufLen      Size (in symbols) of the shortcut buffer.
 * @param pwShortcutLen        Pointer to load current shortcut length into (should contain the previously retrieved shortcut length on subsequent calls).
 * @param psSubstitutionBuf    Pointer to load associated substitution word(s) into.
 * @param wSubstitutionBufLen  Size (in symbols) of the substitution buffer.
 * @param pwSubstitutionLen    Pointer to load substitution length into.
 * @param bForward             1 to get first/next word, 0 to get previous word.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.<BR>
 *         On success. *psShortcutBuf contains the (first/next) shortcut word,<BR>
 *                     *pwShortcutLen contains the shortcut length<BR>
 *                     *psSubstitutionBuf contains the (first/next) substitution word(s)<BR>
 *                     *pwSubstitutionLen constains the substitution length<BR>
 */

ET9STATUS ET9FARCALL ET9AWASDBGetEntry(ET9AWLingInfo * const pLingInfo,
                                       ET9SYMB       * const psShortcutBuf,
                                       const ET9U16          wShortcutBufLen,
                                       ET9U16        * const pwShortcutLen,
                                       ET9SYMB       * const psSubstitutionBuf,
                                       const ET9U16          wSubstitutionBufLen,
                                       ET9U16        * const pwSubstitutionLen,
                                       const ET9U8           bForward)
{
    ET9STATUS   wStatus;
    ET9U8       ET9FARDATA *pbNext;
    ET9U8       ET9FARDATA *pbLast;
    ET9SYMB     ET9FARDATA *pSymb;
    ET9UINT     nFound = 0;
    ET9UINT     nRecordType;
    ET9U16      i;
    ET9U16      wSizeChecked;
    ET9STATUS   mStatus = ET9STATUS_NONE;
    ET9AWASDBInfo ET9FARDATA *pASDB;
    ET9U8 ET9FARDATA *pEntry = NULL;
    ET9SYMB     *pTarget;
    ET9ASDBRecordHeader sRecHeader;
    ET9LdbASRecMap ET9FARDATA *pLdbASEntry = NULL;
    const ET9U8 ET9FARDATA *pbByte;
    ET9U16 wOldShortcutLen;
    ET9SYMB *psShortBuf = psShortcutBuf;
    ET9SYMB *psSubBuf = psSubstitutionBuf;

    if ((wStatus = _ET9AWSys_BasicValidityCheck(pLingInfo)) != ET9STATUS_NONE) {
        return wStatus;
    }
    if (psShortBuf == NULL || pwShortcutLen == NULL || psSubBuf == NULL || pwSubstitutionLen == NULL) {
        return ET9STATUS_INVALID_MEMORY;
    }
    if (psShortBuf && wShortcutBufLen < ET9MAXWORDSIZE) {
        return ET9STATUS_BUFFER_TOO_SMALL;
    }
    if (psSubBuf && wSubstitutionBufLen < ET9MAXSUBSTITUTIONSIZE) {
        return ET9STATUS_BUFFER_TOO_SMALL;
    }

    wOldShortcutLen = *pwShortcutLen;
    pASDB = pLingInfo->pLingCmnInfo->pASDBInfo;

    if (!pASDB) {
        if (!ET9LDBSUPPORTEDAUTOSUBENABLED(pLingInfo->pLingCmnInfo) ){
            return ET9STATUS_NO_ASDB;
        }
    }

    /* if wordlen is too big, return error */
    if (wOldShortcutLen > ET9MAXWORDSIZE) {
        return ET9STATUS_BAD_PARAM;
    }

    /* if no defined entries in ASDB and LDB-AS, return */
    if ((!pASDB || !pASDB->wEntryCount ||
         !ET9USERDEFINEDAUTOSUBENABLED(pLingInfo->pLingCmnInfo)) &&
        (!pLingInfo->pLingCmnInfo->Private.sLDBAutoSub.wNumEntries ||
         !ET9LDBSUPPORTEDAUTOSUBENABLED(pLingInfo->pLingCmnInfo))) {

        return ET9STATUS_NO_MATCHING_WORDS;
    }

    if (!ET9USERDEFINEDAUTOSUBENABLED(pLingInfo->pLingCmnInfo)) {
        pLingInfo->pLingCmnInfo->Private.pASDBGetEntry = 0;
    }
    if (!ET9LDBSUPPORTEDAUTOSUBENABLED(pLingInfo->pLingCmnInfo)) {
        pLingInfo->pLingCmnInfo->Private.wLdbASGetEntryRec = 0;
    }

    if (!ET9USERDEFINEDAUTOSUBENABLED(pLingInfo->pLingCmnInfo)) {
        ET9Assert(ET9LDBSUPPORTEDAUTOSUBENABLED(pLingInfo->pLingCmnInfo));
        /* if the saved entry point is 0, either never used or reset; */
        /* in either case, start over */
        if (!wOldShortcutLen ||
            (!pLingInfo->pLingCmnInfo->Private.pASDBGetEntry && !pLingInfo->pLingCmnInfo->Private.wLdbASGetEntryRec)) {
            wOldShortcutLen = 0;
            if (!bForward) {
                pLingInfo->pLingCmnInfo->Private.wLdbASGetEntryRec = pLingInfo->pLingCmnInfo->Private.sLDBAutoSub.wNumEntries + 1;
            }
            else {
                pLingInfo->pLingCmnInfo->Private.wLdbASGetEntryRec = 0xFFFF;
            }
        }
    }
    else {
        if (!wOldShortcutLen ||
            (!pLingInfo->pLingCmnInfo->Private.pASDBGetEntry && !pLingInfo->pLingCmnInfo->Private.wLdbASGetEntryRec)) {
            wOldShortcutLen = 0;
            if (!bForward) {
                if (ET9LDBSUPPORTEDAUTOSUBENABLED(pLingInfo->pLingCmnInfo)) {
                    pLingInfo->pLingCmnInfo->Private.wLdbASGetEntryRec = pLingInfo->pLingCmnInfo->Private.sLDBAutoSub.wNumEntries + 1;
                }
                else {
                    pLingInfo->pLingCmnInfo->Private.wLdbASGetEntryRec = 0;
                }
            }
            else {
                pLingInfo->pLingCmnInfo->Private.wLdbASGetEntryRec = 0;
            }
        }
    }

    /* if currently retrieving LDB-AS records */
    if (pLingInfo->pLingCmnInfo->Private.wLdbASGetEntryRec &&
        ET9LDBSUPPORTEDAUTOSUBENABLED(pLingInfo->pLingCmnInfo)){
        /* go get LDB-AS info */

        if (pASDB) {
            if ((pLingInfo->pLingCmnInfo->wLdbNum & ET9PLIDMASK) != ET9PLIDNone) {
                pLdbASEntry = __ET9AWGetLdbASEntry(pASDB, pLingInfo->pLingCmnInfo->wLdbNum);
            }
            if (bForward) {
                if (!ET9USERDEFINEDAUTOSUBENABLED(pLingInfo->pLingCmnInfo) && (pLingInfo->pLingCmnInfo->Private.wLdbASGetEntryRec == 0xFFFF)) {
                    pLingInfo->pLingCmnInfo->Private.wLdbASGetEntryRec = 0;
                }
                for (i = pLingInfo->pLingCmnInfo->Private.wLdbASGetEntryRec + 1; i <= pLingInfo->pLingCmnInfo->Private.sLDBAutoSub.wNumEntries; ++i) {
                    /* if record is enabled, go retrieve it */
                    if (!pLdbASEntry || _ET9AWLdbASRecordEnabled(pLingInfo, pLingInfo->pLingCmnInfo->wLdbNum, (ET9U16)(i - 1))) {
                        wStatus = _ET9AWLdbASGetEntry(pLingInfo, pLingInfo->pLingCmnInfo->wLdbNum,
                                                      (ET9U16)(i - 1), psShortBuf, pwShortcutLen,
                                                      psSubBuf, pwSubstitutionLen);
                        if (!wStatus) {
                            pLingInfo->pLingCmnInfo->Private.wLdbASGetEntryRec = i;
                            return ET9STATUS_NONE;
                        }
                    }
                }
                /* if we got here, we've moved out of the LDB-AS record logic */
                /* as well as the ASDB                                        */
                pLingInfo->pLingCmnInfo->Private.wLdbASGetEntryRec = 0;
                return ET9STATUS_NO_MATCHING_WORDS;

            }
            else {
                for (i = pLingInfo->pLingCmnInfo->Private.wLdbASGetEntryRec - 1; i; --i) {
                    /* if record is enabled, go retrieve it */
                    if (!pLdbASEntry || _ET9AWLdbASRecordEnabled(pLingInfo, pLingInfo->pLingCmnInfo->wLdbNum, (ET9U16)(i - 1))) {
                        wStatus = _ET9AWLdbASGetEntry(pLingInfo, pLingInfo->pLingCmnInfo->wLdbNum,
                                                      (ET9U16)(i - 1), psShortBuf, pwShortcutLen,
                                                      psSubBuf, pwSubstitutionLen);
                        if (!wStatus) {
                            pLingInfo->pLingCmnInfo->Private.wLdbASGetEntryRec = i;
                            return ET9STATUS_NONE;
                        }
                    }
                }
                /* if we got here, we've moved out of the LDB-AS record logic */
                pLingInfo->pLingCmnInfo->Private.wLdbASGetEntryRec = 0;
                wOldShortcutLen = 0;
                if (!ET9USERDEFINEDAUTOSUBENABLED(pLingInfo->pLingCmnInfo)) {
                    return ET9STATUS_NO_MATCHING_WORDS;
                }
            }
        }
        else {
            if (bForward) {
                if (!ET9USERDEFINEDAUTOSUBENABLED(pLingInfo->pLingCmnInfo) && (pLingInfo->pLingCmnInfo->Private.wLdbASGetEntryRec == 0xFFFF)) {
                    pLingInfo->pLingCmnInfo->Private.wLdbASGetEntryRec = 0;
                }
                for (i = pLingInfo->pLingCmnInfo->Private.wLdbASGetEntryRec + 1; i <= pLingInfo->pLingCmnInfo->Private.sLDBAutoSub.wNumEntries; ++i) {
                    /* if record is enabled, go retrieve it */
                    wStatus = _ET9AWLdbASGetEntry(pLingInfo, pLingInfo->pLingCmnInfo->wLdbNum, (ET9U16)(i - 1), psShortBuf, pwShortcutLen, psSubBuf, pwSubstitutionLen);
                    if (!wStatus) {
                        pLingInfo->pLingCmnInfo->Private.wLdbASGetEntryRec = i;
                        return ET9STATUS_NONE;
                    }
                }
                /* if we got here, we've moved out of the LDB-AS record logic */
                /* as well as the ASDB                                        */
                pLingInfo->pLingCmnInfo->Private.wLdbASGetEntryRec = 0;
                return ET9STATUS_NO_MATCHING_WORDS;

            }
            else {
                for (i = pLingInfo->pLingCmnInfo->Private.wLdbASGetEntryRec - 1; i; --i) {
                    wStatus = _ET9AWLdbASGetEntry(pLingInfo, pLingInfo->pLingCmnInfo->wLdbNum, (ET9U16)(i - 1), psShortBuf, pwShortcutLen, psSubBuf, pwSubstitutionLen);
                    if (!wStatus) {
                        pLingInfo->pLingCmnInfo->Private.wLdbASGetEntryRec = i;
                        return ET9STATUS_NONE;
                    }
                }
                /* if we got here, we've moved out of the LDB-AS record logic */
                pLingInfo->pLingCmnInfo->Private.wLdbASGetEntryRec = 0;
                wOldShortcutLen = 0;
                if (!ET9USERDEFINEDAUTOSUBENABLED(pLingInfo->pLingCmnInfo)) {
                    return ET9STATUS_NO_MATCHING_WORDS;
                }
            }
        }
    }

    /* Not getting the first ASDB entry */

    if (wOldShortcutLen && pASDB) {

        /*
         * Check the last return word at pLingInfo->Private.pASDBGetEntry.. If we
         * can't find the word, garbage collection/word deletion happened. We then
         * need to search from the beginning.
         */

        pEntry = pLingInfo->pLingCmnInfo->Private.pASDBGetEntry;
        ET9Assert(pEntry >= (ET9U8*)ET9ASDBData(pASDB));
        ET9Assert(pEntry < (ET9U8*)(ET9ASDBData(pASDB) + ET9ASDBDataAreaBytes(pASDB)));

        if (__ET9AWASGetRecordType(pEntry) == ET9ASDBTYPE) {

            /* get the length of the ASDB word */

            pSymb = (ET9SYMB*)__ET9AWReadASDBRecordHeader(pASDB, pEntry, &sRecHeader);

            if (sRecHeader.bShortcutLen == (ET9U8)wOldShortcutLen) {
                pTarget = psShortBuf;
                for (i = wOldShortcutLen; i; --i) {
                    /* ReadSymbol */
                    pbByte = (const ET9U8 ET9FARDATA *)pSymb + 1;
                    __CheckForASDBWrap_ByteP(pbByte, pASDB);
                    if (*pTarget++ != ET9MAKEWORD(*((const ET9U8 ET9FARDATA *)pSymb), *pbByte)) {
                       break;
                    }
                    ++pSymb;
                    __CheckForASDBWrap_WordP(pSymb, pASDB);
                }
                if (!i) {
                    nFound = 1;
                }
            }
        }
        /* if not found, start over */
        if (!nFound) {
            mStatus = ET9STATUS_WORD_NOT_FOUND;
            pEntry = ET9ASDBData(pASDB) + pASDB->wSizeOffset[0];
        }
    }
    /* otherwise, start at the beginning */
    else {
        if (pASDB) {
            pEntry = ET9ASDBData(pASDB) + pASDB->wSizeOffset[0];
            pLingInfo->pLingCmnInfo->Private.wLdbASGetEntryRec = 0;
        }
        else {
            pLingInfo->pLingCmnInfo->Private.wLdbASGetEntryRec = 0;
        }
    }

    if (ET9USERDEFINEDAUTOSUBENABLED(pLingInfo->pLingCmnInfo) && pASDB) {

        /* pbNext will point to requested word */

        pbNext = pEntry;

        ET9Assert(pbNext);

        /* if found and going forward, go to next record */

        if (nFound && bForward) {

            pbNext += __ET9AWGetASDBRecordLength(pASDB, pbNext);
            __CheckForASDBWrap_ByteP(pbNext, pASDB);

            /* if we wrapped without finding next word */

            if (pbNext == ET9ASDBData(pASDB) + pASDB->wSizeOffset[0]) {

                /* start loop through LDB-AS records */

                pLingInfo->pLingCmnInfo->Private.wLdbASGetEntryRec = 1;

                /* go get LDB-AS info */

                if ((pLingInfo->pLingCmnInfo->wLdbNum & ET9PLIDMASK) != ET9PLIDNone) {
                    pLdbASEntry = __ET9AWGetLdbASEntry(pASDB, pLingInfo->pLingCmnInfo->wLdbNum);
                }
                for (i = pLingInfo->pLingCmnInfo->Private.wLdbASGetEntryRec; i <= pLingInfo->pLingCmnInfo->Private.sLDBAutoSub.wNumEntries; ++i) {

                    /* if record is enabled, go retrieve it */

                    if (!pLdbASEntry || _ET9AWLdbASRecordEnabled(pLingInfo, pLingInfo->pLingCmnInfo->wLdbNum, (ET9U16)(i - 1))) {
                        wStatus = _ET9AWLdbASGetEntry(pLingInfo, pLingInfo->pLingCmnInfo->wLdbNum,
                                                      (ET9U16)(i - 1), psShortBuf, pwShortcutLen,
                                                      psSubBuf, pwSubstitutionLen);
                        if (!wStatus) {
                            pLingInfo->pLingCmnInfo->Private.wLdbASGetEntryRec = i;
                            return ET9STATUS_NONE;
                        }
                    }
                }

                /* if we got here, we've moved out of the LDB-AS record logic */
                /* as well as the ASDB                                        */

                pLingInfo->pLingCmnInfo->Private.wLdbASGetEntryRec = 0;
                return ET9STATUS_NO_MATCHING_WORDS;
            }
        }

        /* at this point, we are pointing to the beginning of the database if
        bOldShortcutLen = 0 and we are searching forward. In this case we want to continue with
        the processing if we are not pointing to a ASDB record.in all other cases, we need
        to continue processing. */

        ET9Assert(pbNext >= (ET9U8*)ET9ASDBData(pASDB));
        ET9Assert(pbNext < (ET9U8*)(ET9ASDBData(pASDB) + ET9ASDBDataAreaBytes(pASDB)));

        if (!(wOldShortcutLen == 0 && bForward && __ET9AWASGetRecordType(pbNext) == ET9ASDBTYPE)) {

            wSizeChecked = 0;

            if ((!nFound && (wOldShortcutLen > 0)) || bForward) {

                /*  continue forward to get next ASDB record */

                nRecordType = __ET9AWASGetRecordType(pbNext);
                while (nRecordType != ET9ASDBTYPE) {
                    pbNext += __ET9AWGetASDBRecordLength(pASDB, pbNext);
                    __CheckForASDBWrap_ByteP(pbNext, pASDB);
                    if ((pbNext == ET9ASDBData(pASDB) + pASDB->wSizeOffset[0]) || /* we wrapped without find next word */
                        (wSizeChecked > ET9ASDBDataAreaBytes(pASDB))) {           /* this is here for safety */

                        /* start loop through LDB-AS records */

                        pLingInfo->pLingCmnInfo->Private.wLdbASGetEntryRec = 1;

                        /* go get LDB-AS info */

                        if (ET9LDBSUPPORTEDAUTOSUBENABLED(pLingInfo->pLingCmnInfo)) {
                            if ((pLingInfo->pLingCmnInfo->wLdbNum & ET9PLIDMASK) != ET9PLIDNone) {
                                pLdbASEntry = __ET9AWGetLdbASEntry(pASDB, pLingInfo->pLingCmnInfo->wLdbNum);
                            }
                            for (i = pLingInfo->pLingCmnInfo->Private.wLdbASGetEntryRec; i <= pLingInfo->pLingCmnInfo->Private.sLDBAutoSub.wNumEntries; ++i) {
                                /* if record is enabled, go retrieve it */
                                if (!pLdbASEntry || _ET9AWLdbASRecordEnabled(pLingInfo, pLingInfo->pLingCmnInfo->wLdbNum, (ET9U16)(i - 1))) {
                                    wStatus = _ET9AWLdbASGetEntry(pLingInfo, pLingInfo->pLingCmnInfo->wLdbNum,
                                                                  (ET9U16)(i - 1), psShortBuf, pwShortcutLen,
                                                                  psSubBuf, pwSubstitutionLen);
                                    if (!wStatus) {
                                        pLingInfo->pLingCmnInfo->Private.wLdbASGetEntryRec = i;
                                        return ET9STATUS_NONE;
                                    }
                                }
                            }
                        }
                        /* if we got here, we've moved out of the LDB-AS record logic */
                        /* as well as the ASDB                                        */
                        pLingInfo->pLingCmnInfo->Private.wLdbASGetEntryRec = 0;
                        return ET9STATUS_NO_MATCHING_WORDS;
                    }
                    wSizeChecked = (ET9U16)(wSizeChecked +  __ET9AWGetASDBRecordLength(pASDB, pbNext));
                    nRecordType = __ET9AWASGetRecordType(pbNext);
                }
            }
            else { /* searching backward */
                /* did find word, search through database looking for previous one. */
                pbNext = ET9ASDBData(pASDB) + pASDB->wSizeOffset[0];
                ET9Assert(pbNext >= (ET9U8*)ET9ASDBData(pASDB));
                ET9Assert(pbNext < (ET9U8*)(ET9ASDBData(pASDB) + ET9ASDBDataAreaBytes(pASDB)));
                nRecordType = __ET9AWASGetRecordType(pbNext);
                pbLast = 0;
                while (wSizeChecked < ET9ASDBDataAreaBytes(pASDB)) {  /* this is here for safety */
                    wSizeChecked = (ET9U16)(wSizeChecked + __ET9AWGetASDBRecordLength(pASDB, pbNext));

                    if ((pbNext == pEntry) && wOldShortcutLen) {
                        /* reached sent in word, set pbNext to previous one */
                        pbNext = pbLast;
                        break;
                    }
                    if (wSizeChecked == ET9ASDBDataAreaBytes(pASDB)) {
                        /* reached the end, set pbNext to last ASDB type in database */
                        pbNext = (nRecordType == ET9ASDBTYPE) ? pbNext : pbLast;
                        break;
                    }
                    if (nRecordType == ET9ASDBTYPE) {
                        pbLast = pbNext; /* update pbLast */
                    }
                    pbNext += __ET9AWGetASDBRecordLength(pASDB, pbNext);
                    __CheckForASDBWrap_ByteP(pbNext, pASDB);
                    nRecordType = __ET9AWASGetRecordType(pbNext);
                }
                if (!pbLast) { /* pbLast will be zero if the sent in word was first */
                    return ET9STATUS_NO_MATCHING_WORDS;
                }
            }
        }

        /* at this point pbNext is pointing at our word */
        /* Save the pointer of the returned ASDB entry. */
        pLingInfo->pLingCmnInfo->Private.pASDBGetEntry = pbNext;
        ET9Assert(pbNext >= (ET9U8*)ET9ASDBData(pASDB));
        ET9Assert(pbNext < (ET9U8*)(ET9ASDBData(pASDB) + ET9ASDBDataAreaBytes(pASDB)));

        /* Get the record header, pointer to shortcut word */
        pSymb = (ET9SYMB *)__ET9AWReadASDBRecordHeader(pASDB, pbNext, &sRecHeader);

        /* Get shortcut word length */
        *pwShortcutLen = (ET9U16)sRecHeader.bShortcutLen;

        /* Copy word */
        for (i = sRecHeader.bShortcutLen; i; --i) {
            /* ReadSymbol */
            pbByte = (const ET9U8 ET9FARDATA *)pSymb + 1;
            __CheckForASDBWrap_ByteP(pbByte, pASDB);
            *psShortBuf++ = ET9MAKEWORD(*((const ET9U8 ET9FARDATA *)pSymb), *pbByte);

            ++pSymb;
            __CheckForASDBWrap_WordP(pSymb, pASDB);
        }

        /* if requested, return substitution info */
        if (psSubBuf && pwSubstitutionLen) {
            /* Get shortcut word length */
            *pwSubstitutionLen = (ET9U16)sRecHeader.bSubstitutionLen;
            /* Copy substitution string */
            for (i = sRecHeader.bSubstitutionLen; i; --i) {
                /* ReadSymbol */
                pbByte = (const ET9U8 ET9FARDATA *)pSymb + 1;
                __CheckForASDBWrap_ByteP(pbByte, pASDB);
                *psSubBuf++ = ET9MAKEWORD(*((const ET9U8 ET9FARDATA *)pSymb), *pbByte);
                ++pSymb;
                __CheckForASDBWrap_WordP(pSymb, pASDB);
            }
        }
    }
    return mStatus;
}

#ifdef LONG_ASDB_TEST
void ET9FARCALL _TestASDBIntegrity(ET9AWLingInfo  *pLingInfo)
{

    ET9U8 ET9FARDATA   *pbStart;
    ET9U8 ET9FARDATA   *pbEnd;
    ET9U8 ET9FARDATA   *pbReg;
    ET9U16              wRegion;
    ET9UINT             nCurRecType;
    ET9SYMB            *pSymb;
    ET9UINT             nRecordLength;
    ET9AWASDBInfo ET9FARDATA *pASDB;
    ET9UINT             nRemainingMemory = 0;
    ET9U16              nEntryCount = 0;

    ET9Assert(pLingInfo);
    pASDB = pLingInfo->pLingCmnInfo->pASDBInfo;
    ET9Assert(pASDB != NULL);

#ifdef ET9_DEBUG
    __ET9AWASDBVerifySingleBytes(pASDB);
#endif

    /* get the beginning and end addresses of range record should be in (if there) */
    __ET9AWSetASDBRangeInfo(pLingInfo, 2, 1, &wRegion, &pbStart, &pbEnd);
    /* Move past range marker */
    pbStart += 1;
    __CheckForASDBWrap_ByteP(pbStart, pASDB);
    ++nRemainingMemory;
    ++wRegion;
    pbReg = ET9ASDBData(pASDB) + pASDB->wSizeOffset[wRegion];

    /* loop through the region(s) returned */
    while (pbStart != pbEnd) {
        nCurRecType = __ET9AWASGetRecordInfo(pASDB, pbStart, &nRecordLength);
        /* if it's not a free record (no use for free records in search) */
        if (nCurRecType == ET9ASDBTYPE) {
            /* move past the header bytes */
            nEntryCount++;
            pSymb = (ET9SYMB *)__ET9AWMoveASDBPtrRight(pASDB, pbStart, ASDB_RECORD_HEADER_SIZE);
        }
        else {
            if (pbStart == pbReg) {
                ET9Assert(nCurRecType == ET9ASSINGLEFREETYPE);
                if (wRegion == ET9NUMASDBSIZERANGES) {
                    ET9Assert(pbStart == pbEnd);
                    break;
                }
                nRemainingMemory += nRecordLength;
                ++wRegion;
                if (wRegion == ET9NUMASDBSIZERANGES) {
                    pbReg = ET9ASDBData(pASDB) + pASDB->wSizeOffset[0];
                }
                else {
                    pbReg = ET9ASDBData(pASDB) + pASDB->wSizeOffset[wRegion];
                }
            }
            else {
                nRemainingMemory += nRecordLength;
            }
        }
        /* move to the next record */
        pbStart += nRecordLength;
        __CheckForASDBWrap_ByteP(pbStart, pASDB);

    }
    /* remove the range markers from available size */
    nRemainingMemory -= ET9NUMASDBSIZERANGES;
    /* the free size calculated SHOULD match the value maintained in the header */
    ET9Assert(nRemainingMemory == (ET9UINT)pASDB->wRemainingMemory);
    /* ASDB record count should be correct */
    ET9Assert(nEntryCount == pASDB->wEntryCount);
    /* checksum should match */
    ET9Assert(pASDB->wDataCheck == __ET9AWGetASDBChecksum(pLingInfo));
    return;
}
#endif

/*---------------------------------------------------------------------------*/
/** \internal
 * Export LDB record map.
 *
 * @param pLingInfo              Pointer to ET9 information structure.
 * @param pTUdb                  Pointer to TUDB.
 * @param dwTUdbSize             Size of TUDB.
 * @param pdwExportSize          Ize of TUDB data written.
 * @param ET9WriteTUDB           Function pointer to OEM write.
 * @param pNextRecord            Pointer to structure to cache last export record info.
 * @param pwRecordsExported      Pointer to total records exported.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL _ET9AWLASDBExport(ET9AWLingInfo       *pLingInfo,
                                       ET9U8 ET9FARDATA    *pTUdb,
                                       ET9U32               dwTUdbSize,
                                       ET9U32              *pdwExportSize,
                                       ET9WriteTUDB_f       ET9WriteTUDB,
                                       ET9AWTUDBNextRecord *pNextRecord,
                                       ET9U16              *pwRecordsExported)
{
    ET9STATUS wStatus = ET9STATUS_NONE;
    ET9AWASDBInfo ET9FARDATA * const pASDB = pLingInfo->pLingCmnInfo->pASDBInfo;
    ET9U16    wOffset = 0;
    ET9INT    i = 0;
    ET9INT    nSize;
    ET9U8     byBuffer[4];
    ET9U16    dwTUdbOffset = 0;

    if (! pASDB) {
        return ET9STATUS_NONE;
    }

    if (pNextRecord->bDBType == ET9_TUDBTYPE_LASDB) {
        if (pNextRecord->wDBOffset >= ET9MAXASDBLANGUAGERECORDS) {
            return ET9STATUS_BAD_PARAM;
        }
        i = pNextRecord->wDBOffset;
    }

    *pdwExportSize = 0;
    *pwRecordsExported = 0;

    /* loop through all LDB record map */
    for ( ; i < ET9MAXASDBLANGUAGERECORDS; i++) {

        if (! pASDB->sLdbASRecord[i].wLDBID) {
            continue;
        }
        /* determine size of the exported record */
        nSize = ET9_LASRECLEN;

        /* if there is not enough room to export this record, end export */
        if ((ET9U32)(dwTUdbOffset + nSize) > dwTUdbSize) {
            pNextRecord->bDBType = ET9_TUDBTYPE_RUDB;
            pNextRecord->wDBOffset = wOffset;
            *pdwExportSize = dwTUdbOffset;
            return ET9STATUS_NONE;
        }
        *pdwExportSize = (ET9U32)(*pdwExportSize + nSize);

        /* write record header - record type */
        byBuffer[0] = ET9TUDB_LDBAUTOSUB_REC;
        wStatus = _ET9TUdbWriteData(byBuffer, 1, pTUdb, dwTUdbOffset++, ET9WriteTUDB);
        _ET9CHECKGENSTATUS();

        /* record body size does not include record header */
        /* write record header - Record body size */
        wStatus = _ET9TUdbWriteWord((ET9U16)(nSize - 3), pTUdb, dwTUdbOffset, ET9WriteTUDB);
        _ET9CHECKGENSTATUS();
        dwTUdbOffset += 2;

        /* write LDB num attribute */
        byBuffer[0] = ET9TUDB_LDBRECMAP_LDBNUM;
        wStatus = _ET9TUdbWriteData(byBuffer, 1, pTUdb, dwTUdbOffset++, ET9WriteTUDB);
        _ET9CHECKGENSTATUS();

        wStatus = _ET9TUdbWriteWord(2, pTUdb, dwTUdbOffset, ET9WriteTUDB);
        _ET9CHECKGENSTATUS();
        dwTUdbOffset += 2;

        wStatus = _ET9TUdbWriteWord(pASDB->sLdbASRecord[i].wLDBID, pTUdb, dwTUdbOffset, ET9WriteTUDB);
        _ET9CHECKGENSTATUS();
        dwTUdbOffset += 2;

        /* write word length attribute */
        byBuffer[0] = ET9TUDB_LDBRECMAP_TOTALRECS;
        wStatus = _ET9TUdbWriteData(byBuffer, 1, pTUdb, dwTUdbOffset++, ET9WriteTUDB);
        _ET9CHECKGENSTATUS();

        wStatus = _ET9TUdbWriteWord(2, pTUdb, dwTUdbOffset, ET9WriteTUDB);
        _ET9CHECKGENSTATUS();
        dwTUdbOffset += 2;

        wStatus = _ET9TUdbWriteWord(pASDB->sLdbASRecord[i].wTotalRecords, pTUdb, dwTUdbOffset, ET9WriteTUDB);
        _ET9CHECKGENSTATUS();
        dwTUdbOffset += 2;

        /* write total enable records attribute */
        byBuffer[0] = ET9TUDB_LDBRECMAP_TOTALENABLERECS;
        wStatus = _ET9TUdbWriteData(byBuffer, 1, pTUdb, dwTUdbOffset++, ET9WriteTUDB);
        _ET9CHECKGENSTATUS();

        wStatus = _ET9TUdbWriteWord(2, pTUdb, dwTUdbOffset, ET9WriteTUDB);
        _ET9CHECKGENSTATUS();
        dwTUdbOffset += 2;

        wStatus = _ET9TUdbWriteWord(pASDB->sLdbASRecord[i].wEnabledRecords, pTUdb, dwTUdbOffset, ET9WriteTUDB);
        _ET9CHECKGENSTATUS();
        dwTUdbOffset += 2;

        /* write bitmap attribute */
        byBuffer[0] = ET9TUDB_LDBRECMAP_BITMAPS;
        wStatus = _ET9TUdbWriteData(byBuffer, 1, pTUdb, dwTUdbOffset++, ET9WriteTUDB);
        _ET9CHECKGENSTATUS();

        wStatus = _ET9TUdbWriteWord(32, pTUdb, dwTUdbOffset, ET9WriteTUDB);
        _ET9CHECKGENSTATUS();
        dwTUdbOffset += 2;

        wStatus = _ET9TUdbWriteData(
            pASDB->sLdbASRecord[i].bMap, 32, pTUdb, dwTUdbOffset, ET9WriteTUDB);
        _ET9CHECKGENSTATUS();
        dwTUdbOffset += 32;

        (*pwRecordsExported)++;
    }

    return wStatus;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Get record length from asdb.
 *
 * @param pASDB                  Pointer to ASDB.
 * @param pbCurrent              Pointer to record.
 * @param pwShortcutUtf8Len      Size of shortcut string in utf8 encoding.
 * @param pwSubUtf8Len           Size of substitution string in utf8 encoding.
 *
 * @return Record length.
 */

static ET9U16 ET9LOCALCALL __ET9AWGetTUASDBRecLen(ET9AWASDBInfo ET9FARDATA *pASDB,
                                                  ET9U8         ET9FARDATA *pbCurrent,
                                                  ET9U16                   *pwShortcutUtf8Len,
                                                  ET9U16                   *pwSubUtf8Len)
{
    ET9ASDBRecordHeader sRecHeader;
    ET9U8 ET9FARDATA *pbSymb, *pbByte;
    ET9SYMB  sSymb;
    ET9U16   wSymbolLen;
    ET9U16   wRecLen = 19;
    ET9U16   i;
    ET9U8    bUtf8[4];

    /* get the length of the ASDB word */
    pbSymb = __ET9AWReadASDBRecordHeader(pASDB, pbCurrent, &sRecHeader);

    /* determine size of the exported record */

    *pwShortcutUtf8Len = 0;
    for (i = sRecHeader.bShortcutLen; i; --i) {
        pbByte = (ET9U8 ET9FARDATA *)pbSymb + 1;
        sSymb = ET9MAKEWORD(*pbSymb, *pbByte);
        pbSymb += ET9SYMBOLWIDTH;
        __CheckForASDBWrap_ByteP(pbSymb, pASDB);
        wSymbolLen = _ET9SymbToUtf8(sSymb, bUtf8);
        wRecLen = wRecLen + wSymbolLen;
        *pwShortcutUtf8Len = *pwShortcutUtf8Len + wSymbolLen;

    }

    *pwSubUtf8Len = 0;

    for (i = sRecHeader.bSubstitutionLen; i; --i) {
        pbByte = (ET9U8 ET9FARDATA *)pbSymb + 1;
        sSymb = ET9MAKEWORD(*pbSymb, *pbByte);
        pbSymb += ET9SYMBOLWIDTH;
        __CheckForASDBWrap_ByteP(pbSymb, pASDB);
        wSymbolLen = _ET9SymbToUtf8(sSymb, bUtf8);
        wRecLen = wRecLen + wSymbolLen;
        *pwSubUtf8Len = *pwSubUtf8Len + wSymbolLen;

    }

    return wRecLen;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Get size of ASDB for export.
 *
 * @param pLingInfo          Pointer to ET9 information structure.
 * @param pdwTotalWords      Pointer to total of words for exporting.
 *
 * @return Size of the ASDB (0 on error).
 */

ET9U32 ET9FARCALL _ET9AWASDBGetSize(ET9AWLingInfo *pLingInfo,
                                    ET9U32        *pdwTotalWords)
{
    ET9AWASDBInfo ET9FARDATA * const pASDB = pLingInfo->pLingCmnInfo->pASDBInfo;
    ET9U8 ET9FARDATA   *pEntry;
    ET9U16              wShortcutlen;
    ET9U16              wSubLen;
    ET9U32              dwSize = 0;
    ET9INT              i;

    *pdwTotalWords = 0;

    if (pASDB == NULL) {
        return 0;
    }
    for (i = 0; i < ET9MAXASDBLANGUAGERECORDS; ++i) {

        if (!pASDB->sLdbASRecord[i].wLDBID) {
            continue;
        }
        dwSize += ET9_LASRECLEN;
        (*pdwTotalWords)++;
    }

    pEntry = ET9ASDBData(pASDB) + pASDB->wSizeOffset[0];

    /* loop through ASDB records */
    do {
        if (ET9ASDBTYPE == __ET9AWASGetRecordType(pEntry)) {
            /* determine size of the exported record */
            dwSize += __ET9AWGetTUASDBRecLen(pASDB, pEntry, &wShortcutlen, &wSubLen);
            (*pdwTotalWords)++;
        }
        pEntry += __ET9AWGetASDBRecordLength(pASDB, pEntry);
        __CheckForASDBWrap_ByteP(pEntry, pASDB);
    } while (pEntry != (ET9ASDBData(pASDB) + pASDB->wSizeOffset[0]));

    return dwSize;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Export User ASDB.
 *
 * @param pLingInfo              Pointer to ET9 information structure.
 * @param pTUdb                  Pointer to TUDB.
 * @param dwTUdbSize             Size of TUDB.
 * @param pdwExportSize          Size of TUDB data written.
 * @param ET9WriteTUDB           Function pointer to OEM write.
 * @param pNextRecord            Pointer to structure to cache last export record info.
 * @param pwRecordsExported      Pointer to total records exported.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL _ET9AWUASDBExport(ET9AWLingInfo       *pLingInfo,
                                       ET9U8 ET9FARDATA    *pTUdb,
                                       ET9U32               dwTUdbSize,
                                       ET9U32              *pdwExportSize,
                                       ET9WriteTUDB_f       ET9WriteTUDB,
                                       ET9AWTUDBNextRecord *pNextRecord,
                                       ET9U16              *pwRecordsExported)
{
    ET9STATUS           wStatus = ET9STATUS_NONE;
    ET9AWASDBInfo ET9FARDATA * const pASDB = pLingInfo->pLingCmnInfo->pASDBInfo;
    ET9ASDBRecordHeader sRecHeader;
    ET9U8 ET9FARDATA   *pEntry, *pbSymb, *pbByte;
    ET9U16              dwTUdbOffset = 0;
    ET9U16              wShortcutlen;
    ET9U16              wSubLen;
    ET9U16              wASDBOffset = 0;
    ET9U16              wSize;
    ET9U8               byBuffer[4];
    ET9SYMB             sSymb;
    ET9U16              i;


    *pdwExportSize = 0;
    *pwRecordsExported = 0;

    /* no ASDB, or if no asdb entries in ASDB, return */
    if ((pASDB == NULL) || !pASDB->wEntryCount) {
        return ET9STATUS_NONE;
    }

    if ((pNextRecord->bDBType == ET9_TUDBTYPE_UASDB) &&
        (pNextRecord->wDBOffset >= ET9ASDBDataAreaBytes(pASDB))) {
        return ET9STATUS_BAD_PARAM;
    }

    /* now set next record for exporting */
    if (pNextRecord->bDBType == ET9_TUDBTYPE_UASDB) {
        /*
         * start from the top, scan forward until the current record is >= dboffset.
         * if it is the same offset, jump to next record and continue.
         * if it is greater offset, then use this a next record
         */

        wASDBOffset = pASDB->wSizeOffset[0];
        pEntry = ET9ASDBData(pASDB) + wASDBOffset;

        while (wASDBOffset <= pNextRecord->wDBOffset) {

            wASDBOffset = (ET9U16)(wASDBOffset + __ET9AWGetASDBRecordLength(pASDB, pEntry));
            pEntry += __ET9AWGetASDBRecordLength(pASDB, pEntry);
            __CheckForASDBWrap_ByteP(pEntry, pASDB);
            if (pEntry == (ET9ASDBData(pASDB) + pASDB->wSizeOffset[0])) {
                /* we wrapped without find next word */
                return ET9STATUS_NONE;
            }
        }
    }
    else {
        wASDBOffset = pASDB->wSizeOffset[0];
    }

    pEntry = ET9ASDBData(pASDB) + wASDBOffset;

    /* loop through ASDB records */
    do {
        if (ET9ASDBTYPE == __ET9AWASGetRecordType(pEntry)) {
            /* determine size of the exported record */
            wSize = __ET9AWGetTUASDBRecLen(pASDB, pEntry, &wShortcutlen, &wSubLen);

            /* if there is not enough room to export this record, end export */
            if ((ET9U32)(dwTUdbOffset + wSize) > dwTUdbSize) {
                pNextRecord->bDBType = ET9_TUDBTYPE_UASDB;
                pNextRecord->wDBOffset = wASDBOffset;
                *pdwExportSize = dwTUdbOffset;
                return ET9STATUS_NONE;
            }
            *pdwExportSize = (ET9U32)(*pdwExportSize + wSize);

            /* write record header - record type */
            byBuffer[0] = ET9TUDB_USERAUTOSUBWORD_REC;
            wStatus = _ET9TUdbWriteData(byBuffer, 1, pTUdb, dwTUdbOffset++, ET9WriteTUDB);
            _ET9CHECKGENSTATUS();

            /* record body size does not include record header */
            /* write record header - Record body size */
            wStatus = _ET9TUdbWriteWord((ET9U16)(wSize - 3), pTUdb, dwTUdbOffset, ET9WriteTUDB);
            _ET9CHECKGENSTATUS();
            dwTUdbOffset += 2;

            pbSymb = __ET9AWReadASDBRecordHeader(pASDB, pEntry, &sRecHeader);

            /* write short cut word length attribute */
            byBuffer[0] = ET9TUDB_USERASWORD_SHORTCUT_LEN;
            wStatus = _ET9TUdbWriteData(byBuffer, 1, pTUdb, dwTUdbOffset++, ET9WriteTUDB);
            _ET9CHECKGENSTATUS();

            wStatus = _ET9TUdbWriteWord(2, pTUdb, dwTUdbOffset, ET9WriteTUDB);
            _ET9CHECKGENSTATUS();
            dwTUdbOffset += 2;

            wStatus = _ET9TUdbWriteWord(sRecHeader.bShortcutLen, pTUdb, dwTUdbOffset, ET9WriteTUDB);
            _ET9CHECKGENSTATUS();
            dwTUdbOffset += 2;

            /* write short cut word word attribute */
            byBuffer[0] = ET9TUDB_USERASWORD_SHORTCUTWORD;
            wStatus = _ET9TUdbWriteData(byBuffer, 1, pTUdb, dwTUdbOffset++, ET9WriteTUDB);
            _ET9CHECKGENSTATUS();

            wStatus = _ET9TUdbWriteWord(wShortcutlen, pTUdb, dwTUdbOffset, ET9WriteTUDB);
            _ET9CHECKGENSTATUS();
            dwTUdbOffset += 2;

            for (i = sRecHeader.bShortcutLen; i; --i) {
                pbByte = (ET9U8 ET9FARDATA *)pbSymb + 1;
                sSymb = ET9MAKEWORD(*pbSymb, *pbByte);
                pbSymb += ET9SYMBOLWIDTH;
                __CheckForASDBWrap_ByteP(pbSymb, pASDB);
                wSize = _ET9SymbToUtf8(sSymb, byBuffer);
                wStatus = _ET9TUdbWriteData(byBuffer, wSize, pTUdb, dwTUdbOffset++, ET9WriteTUDB);
                _ET9CHECKGENSTATUS();
            }

            /* write autosub word length attribute */
            byBuffer[0] = ET9TUDB_USERASWORD_SUBWORD_LEN;
            wStatus = _ET9TUdbWriteData(byBuffer, 1, pTUdb, dwTUdbOffset++, ET9WriteTUDB);
            _ET9CHECKGENSTATUS();

            wStatus = _ET9TUdbWriteWord(2, pTUdb, dwTUdbOffset, ET9WriteTUDB);
            _ET9CHECKGENSTATUS();
            dwTUdbOffset += 2;

            wStatus = _ET9TUdbWriteWord(sRecHeader.bSubstitutionLen, pTUdb, dwTUdbOffset, ET9WriteTUDB);
            _ET9CHECKGENSTATUS();
            dwTUdbOffset += 2;

            /* write substitution word word attribute */
            byBuffer[0] = ET9TUDB_USERASWORD_SUBWORD;
            wStatus = _ET9TUdbWriteData(byBuffer, 1, pTUdb, dwTUdbOffset++, ET9WriteTUDB);
            _ET9CHECKGENSTATUS();

            wStatus = _ET9TUdbWriteWord(wSubLen, pTUdb, dwTUdbOffset, ET9WriteTUDB);
            _ET9CHECKGENSTATUS();
            dwTUdbOffset += 2;

            for (i = sRecHeader.bSubstitutionLen; i; --i) {
                pbByte = (ET9U8 ET9FARDATA *)pbSymb + 1;
                sSymb = ET9MAKEWORD(*pbSymb, *pbByte);
                pbSymb += ET9SYMBOLWIDTH;
                __CheckForASDBWrap_ByteP(pbSymb, pASDB);
                wSize = _ET9SymbToUtf8(sSymb, byBuffer);
                wStatus = _ET9TUdbWriteData(byBuffer, wSize, pTUdb, dwTUdbOffset, ET9WriteTUDB);
                _ET9CHECKGENSTATUS();
                dwTUdbOffset = dwTUdbOffset + wSize;
            }

            (*pwRecordsExported)++;
        }
        wASDBOffset = (ET9U16)(wASDBOffset + __ET9AWGetASDBRecordLength(pASDB, pEntry));
        pEntry += __ET9AWGetASDBRecordLength(pASDB, pEntry);
        __CheckForASDBWrap_ByteP(pEntry, pASDB);

    } while (pEntry != (ET9ASDBData(pASDB) + pASDB->wSizeOffset[0]));

    return wStatus;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Import LDB bit map.
 *
 * @param pLingInfo          Pointer to ET9 information structure.
 * @param pTUdb              Pointer to TUDB.
 * @param ET9ReadTUDB        Function pointer to OEM read.
 * @param dwTUdbOffset       Offset to TUDB for the record.
 * @param wRecSize           Size of record minus header.
 * @param pbImported         Pointer to status of imported.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL _ET9AWLDBBMImport(ET9AWLingInfo       *pLingInfo,
                                       ET9U8 ET9FARDATA    *pTUdb,
                                       ET9ReadTUDB_f        ET9ReadTUDB,
                                       ET9U32               dwTUdbOffset,
                                       ET9U16               wRecSize,
                                       ET9U8               *pbImported)
{
    ET9STATUS  wStatus = ET9STATUS_NONE;
    ET9AWASDBInfo ET9FARDATA * const pASDB = pLingInfo->pLingCmnInfo->pASDBInfo;
    ET9U16  wRecOffset = 0;
    ET9U8  *pByte;
    ET9U16  i, j;
    ET9U8   byAttrType;
    ET9U16  wChecksum;
    ET9U16  wAttrSize;
    ET9U16  wLdbNum = 0;
    ET9U16  wTotalRecs = 0;
    ET9U16  wTotalEnableRecs = 0;
    ET9U8   bMap[ET9MAXLDBSUPPORTEDASRECORDS/8];
    ET9U8   bRetrievedBitmap = 0;
    ET9LdbASRecMap *pLDB;

    *pbImported = 0;

    if (pASDB == NULL) {
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

        switch(byAttrType) {
            case ET9TUDB_LDBRECMAP_LDBNUM:
                if (wAttrSize != 2) {
                    return ET9STATUS_NONE;
                }
                wStatus = _ET9TUdbReadWord(&wLdbNum, pTUdb, dwTUdbOffset + wRecOffset, ET9ReadTUDB);
                _ET9CHECKGENSTATUS();
                break;

            case ET9TUDB_LDBRECMAP_TOTALRECS:
                if (wAttrSize != 2) {
                    return ET9STATUS_NONE;
                }
                wStatus = _ET9TUdbReadWord(&wTotalRecs, pTUdb, dwTUdbOffset + wRecOffset, ET9ReadTUDB);
                _ET9CHECKGENSTATUS();
                break;

            case ET9TUDB_LDBRECMAP_TOTALENABLERECS:
                if (wAttrSize != 2) {
                    return ET9STATUS_NONE;
                }
                wStatus = _ET9TUdbReadWord(&wTotalEnableRecs, pTUdb, dwTUdbOffset + wRecOffset, ET9ReadTUDB);
                _ET9CHECKGENSTATUS();
                break;


            case ET9TUDB_LDBRECMAP_BITMAPS:
                i = 0;
                if (wAttrSize > ET9MAXLDBSUPPORTEDASRECORDS/8) {
                    return ET9STATUS_NONE;
                }
                wStatus = _ET9TUdbReadData(bMap, wAttrSize, pTUdb, dwTUdbOffset + wRecOffset, ET9ReadTUDB);
                _ET9CHECKGENSTATUS();
                bRetrievedBitmap = 1;
                break;

            case ET9TUDB_CUSTOMWORD_INVALID:
                return ET9STATUS_NONE;

            default:
                break;

        }
        wRecOffset = (ET9U16)(wRecOffset + wAttrSize);
    }

    /* Verify that minimum LDB info for ASDB exist */
    if (!wLdbNum || !wTotalRecs || !bRetrievedBitmap) {
        /* reject */
        return ET9STATUS_NONE;
    }
    /* go make sure LDB has tracking entry */
    __ET9AWUpdateTracker(pLingInfo, wLdbNum);

    /* go look for the LDB's tracking record in the ASDB */
    pLDB = __ET9AWGetLdbASEntry(pASDB, wLdbNum);

    /* if not already defined, or the existing record has same total entries as the imported */
    if (!pLDB || (pLDB->wTotalRecords == wTotalRecs)) {
        /* loop through and get new (or existing) record */
        pLDB = pASDB->sLdbASRecord;
        for (i = 0; i < ET9MAXASDBLANGUAGERECORDS; ++i, ++pLDB) {
            /* if entry is available */
            if (!pLDB->wLDBID || pLDB->wLDBID == wLdbNum) {
                /* update checksum with new LDB ID (may just be overwriting same value here) */
                wChecksum = (ET9U16)(pASDB->wDataCheck - pLDB->wLDBID + wLdbNum);
                __ET9AWWriteASDBData(pLingInfo, (ET9U8 ET9FARDATA *)&pLDB->wLDBID,
                             (const void ET9FARDATA *)&wLdbNum, sizeof(wLdbNum));

                /* set enabled records == total records */
                wChecksum = (ET9U16)(wChecksum - pLDB->wEnabledRecords + wTotalEnableRecs);
                __ET9AWWriteASDBData(pLingInfo, (void ET9FARDATA*)&pLDB->wEnabledRecords,
                        (const void ET9FARDATA *)&wTotalEnableRecs, sizeof(pLDB->wEnabledRecords));

                /* set total records */
                wChecksum = (ET9U16)(wChecksum - pLDB->wTotalRecords + wTotalRecs);
                __ET9AWWriteASDBData(pLingInfo, (void ET9FARDATA*)&pLDB->wTotalRecords,
                        (const void ET9FARDATA *)&wTotalRecs,
                        sizeof(pLDB->wTotalRecords));

                /* now loop through bitmap array of enabled records and set all to 'off' */
                pByte = pLDB->bMap;
                for (j = 0; j < (ET9MAXLDBSUPPORTEDASRECORDS/8); ++j, ++pByte) {
                    wChecksum = (ET9U16)(wChecksum - *pByte + bMap[j]);
                    __ET9AWWriteASDBData(pLingInfo, (void ET9FARDATA*)pByte,
                                (const void ET9FARDATA *)&bMap[j], 1);
                }

                /* write new checksum */
                __ET9AWWriteASDBData(pLingInfo, (void ET9FARDATA*)&pASDB->wDataCheck,
                    (const void ET9FARDATA *)&wChecksum, sizeof(wChecksum));
                break;
            }
        }
        ET9Assert(i < ET9MAXASDBLANGUAGERECORDS);
    }
    else {
        return ET9STATUS_NONE;
    }

    *pbImported = 1;

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Import ASDB.
 *
 * @param pLingInfo          Pointer to ET9 information structure.
 * @param pTUdb              Pointer to TUDB.
 * @param ET9ReadTUDB        Function pointer to OEM read.
 * @param dwTUdbOffset       Offset to TUDB for the record.
 * @param wRecSize           Size of record minus header.
 * @param pbImported         Pointer to status of imported.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL _ET9AWASDBImport(ET9AWLingInfo       *pLingInfo,
                                      ET9U8 ET9FARDATA    *pTUdb,
                                      ET9ReadTUDB_f        ET9ReadTUDB,
                                      ET9U32               dwTUdbOffset,
                                      ET9U16               wRecSize,
                                      ET9U8               *pbImported)
{
    ET9STATUS  wStatus = ET9STATUS_NONE;
    ET9AWASDBInfo ET9FARDATA * const pASDB = pLingInfo->pLingCmnInfo->pASDBInfo;
    ET9U16  wRecOffset = 0;
    ET9U8   byBuffer[4];
    ET9U16  i;
    ET9U8   byAttrType;
    ET9U16  wAttrSize;
    ET9U16  wToRead = 4;
    ET9U8   bSymbolSize;
    ET9U16  wShortcutLen = 0;
    ET9U16  wSubstitutionLen = 0;
    ET9SYMB sShortCut[ET9MAXWORDSIZE];
    ET9SYMB sSubstitution[ET9MAXWORDSIZE];
    ET9U8   bRetrievedShortCutWordSymbs = 0;
    ET9U8   bRetrievedSubWordSymbs = 0;

    *pbImported = 0;

    if (pASDB == NULL) {
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

        switch(byAttrType) {
            case ET9TUDB_USERASWORD_SHORTCUT_LEN:
                if (wAttrSize != 2) {
                    return ET9STATUS_NONE;
                }
                wStatus = _ET9TUdbReadWord(&wShortcutLen, pTUdb, dwTUdbOffset + wRecOffset, ET9ReadTUDB);
                _ET9CHECKGENSTATUS();
                if (wShortcutLen > ET9MAXWORDSIZE) {
                    return ET9STATUS_NONE;
                }
                wRecOffset = (ET9U16)(wRecOffset + wAttrSize);
                break;

            case ET9TUDB_USERASWORD_SUBWORD_LEN:
                if (wAttrSize != 2) {
                    return ET9STATUS_NONE;
                }
                wStatus = _ET9TUdbReadWord(&wSubstitutionLen, pTUdb, dwTUdbOffset + wRecOffset, ET9ReadTUDB);
                _ET9CHECKGENSTATUS();
                if (wSubstitutionLen > ET9MAXWORDSIZE) {
                    return ET9STATUS_NONE;
                }
                wRecOffset = (ET9U16)(wRecOffset + wAttrSize);
                break;


            case ET9TUDB_USERASWORD_SHORTCUTWORD:
                i = 0;
                while (wAttrSize) {
                    wToRead = (wAttrSize > 4) ? 4 : wAttrSize;
                    wStatus = _ET9TUdbReadData(byBuffer, wToRead, pTUdb, dwTUdbOffset + wRecOffset, ET9ReadTUDB);
                    _ET9CHECKGENSTATUS();
                    bSymbolSize = _ET9Utf8ToSymb(byBuffer, NULL, &sShortCut[i++]);
                    if (! bSymbolSize) {
                        return ET9STATUS_NONE;
                    }
                    wRecOffset = (ET9U16)(wRecOffset + bSymbolSize);
                    wAttrSize = (ET9U16)(wAttrSize - bSymbolSize);
                }
                if (i != wShortcutLen) {
                    return ET9STATUS_NONE;
                }
                bRetrievedSubWordSymbs = 1;
                break;

            case ET9TUDB_USERASWORD_SUBWORD:
                i = 0;
                while (wAttrSize) {
                    wToRead = (wAttrSize > 4) ? 4 : wAttrSize;
                    wStatus = _ET9TUdbReadData(byBuffer, wToRead, pTUdb, dwTUdbOffset + wRecOffset, ET9ReadTUDB);
                    _ET9CHECKGENSTATUS();
                    bSymbolSize = _ET9Utf8ToSymb(byBuffer, NULL, &sSubstitution[i++]);
                    if (! bSymbolSize) {
                        return ET9STATUS_NONE;
                    }
                    wRecOffset = (ET9U16)(wRecOffset + bSymbolSize);
                    wAttrSize = (ET9U16)(wAttrSize - bSymbolSize);
                }
                if (i != wSubstitutionLen) {
                    return ET9STATUS_NONE;
                }
                bRetrievedShortCutWordSymbs = 1;
                break;

            case ET9TUDB_USERASWORD_INVALID:
                return ET9STATUS_NONE;

            default:
                wRecOffset = (ET9U16)(wRecOffset + wAttrSize);
                break;

        }
    }

    /* validate that a record has minimum data */
    if (!wShortcutLen || !wSubstitutionLen || !bRetrievedSubWordSymbs || !bRetrievedShortCutWordSymbs) {
        /* reject */
        return ET9STATUS_NONE;
    }
    wStatus =  ET9AWASDBAddEntry(pLingInfo, sShortCut, sSubstitution, wShortcutLen, wSubstitutionLen);

    if (wStatus != ET9STATUS_WORD_EXISTS) {
        _ET9CHECKGENSTATUS();
    }

    *pbImported = 1;

    return ET9STATUS_NONE;
}

/*! @} */
#endif /* ET9_ALPHABETIC_MODULE */
/* ----------------------------------< eof >--------------------------------- */
