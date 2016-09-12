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
;**     FileName: et9cprdb.c                                                  **
;**                                                                           **
;**  Description: Chinese XT9 RDB module.                                     **
;**               Conforming to the development version of Chinese XT9.       **
;**                                                                           **
;*******************************************************************************
;******************************************************************************/


#include "et9api.h"
#include "et9cprdb.h"
#include "et9cpldb.h"
#include "et9cpwdls.h"
#include "et9cptone.h"
#include "et9cpkey.h"
#include "et9cpsys.h"
#include "et9cpspel.h"
#include "et9cpsldb.h"
#include "et9cppbuf.h"
#include "et9cpmisc.h"
#include "et9cptone.h"
#include "et9cpcntx.h"
#include "et9cpname.h"

#if (defined(WIN32_PLATFORM_WFSP) || defined(WIN32_PLATFORM_PSPC)) && !defined(ET9_DEBUG)
/* prevent infinite loop that is caused by optimizations being turned on in windows mobile
   we think this is a optimization bug with the compiler */
#pragma optimize ("", off)
#endif

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
#if defined(ET9_DEBUG) || defined(WIN32_PLATFORM_WFSP) || defined(WIN32_PLATFORM_PSPC) /* need to expose in et9cprdb.h for debug */

ET9U8 ET9FARDATA * ET9FARCALL ET9_CP_MoveUdbPR(ET9CPUdbInfo ET9FARDATA *pUdb,
                                      ET9U8 ET9FARDATA * pbThis,
                                      ET9U16 wNumMoves)
{
    ET9U8 ET9FARDATA * pbDest;
    ET9Assert(pUdb);
    ET9Assert((pbThis >= ET9_CP_UdbData(pUdb)) && (pbThis < ET9_CP_UdbEnd(pUdb)));
    ET9Assert(wNumMoves < ET9_CP_UdbDataAreaBytes(pUdb));

    pbDest = pbThis + wNumMoves;
    if (pbDest >= ET9_CP_UdbEnd(pUdb)) {
        pbDest -= ET9_CP_UdbDataAreaBytes(pUdb);
    }
    return pbDest;
} /* ET9_CP_MoveUdbPR() */

#endif /* ET9_DEBUG or windows mobile */

/*------------------------------------------------------------------------
 *  Function    : ET9_CP_UdbReadWord
 *
 *  Synopsis    : Read a ET9U16 from UDB data are. Wrap around at UDB end.
 *
 *     Input:   : pET9CPLingInfo = pointer to Chinese XT9 LingInfo
 *                pbData = pointer to the starting byte of the data
 *
 *    Output    : none
 *
 *    Return    : the ET9U16 value at pbData
 *-----------------------------------------------------------------------*/
#if defined(ET9_DEBUG)
static ET9U16 ET9LOCALCALL ET9_CP_UdbReadWord(ET9CPUdbInfo ET9FARDATA *pUdb,
                                         ET9U8 ET9FARDATA * pbData)
{
    ET9U8 bHiByte, bLoByte;

    ET9Assert(pUdb);
    ET9Assert((pbData >= ET9_CP_UdbData(pUdb)) && (pbData < ET9_CP_UdbEnd(pUdb)));
    /* read hi byte */
    bHiByte = *pbData;
    /* read lo byte */
    pbData = ET9_CP_MoveUdbPR(pUdb, pbData, 1);
    bLoByte = *pbData;
    return ET9_CP_MAKEWORD(bHiByte, bLoByte);
} /* ET9_CP_UdbReadWord() */
#else
/* release version for optimization */
#define ET9_CP_UdbReadWord(pUdb, pbData)      \
    ET9_CP_MAKEWORD(*(pbData), *(ET9_CP_MoveUdbPR(pUdb, pbData, 1)))
#endif /* ET9_DEBUG */

/*------------------------------------------------------------------------
 *  Function    : ET9_CP_UdbWriteWord
 *
 *  Synopsis    : Write a ET9U16 into UDB data are. Wrap around at UDB end.
 *
 *     Input:   : pET9CPLingInfo = pointer to Chinese XT9 LingInfo
 *                pbDest = pointer to the starting byte of the destination
 *                wSrc = the ET9U16 value to be written.
 *
 *    Output    : none
 *
 *    Return    : none
 *-----------------------------------------------------------------------*/
static void ET9LOCALCALL ET9_CP_UdbWriteWord(ET9CPLingInfo *pET9CPLingInfo,
                                         ET9U8 ET9FARDATA * pbDest,
                                         ET9U16 wSrc)
{
    ET9CPUdbInfo ET9FARDATA *pUdb;
    ET9U8 bByte;

    ET9Assert(pET9CPLingInfo);
    ET9Assert(pET9CPLingInfo->pUdb);

    pUdb = pET9CPLingInfo->pUdb;
    /* write hi byte */
    bByte = ET9_CP_HIBYTE(wSrc);
    ET9_CP_WriteUdbData(pET9CPLingInfo,
                      (void ET9FARDATA*)pbDest,
                      (const void ET9FARDATA*)&bByte,
                      1);
    /* write lo byte */
    pbDest = ET9_CP_MoveUdbPR(pUdb, pbDest, 1);
    bByte = ET9_CP_LOBYTE(wSrc);
    ET9_CP_WriteUdbData(pET9CPLingInfo,
                      (void ET9FARDATA*)pbDest,
                      (const void ET9FARDATA*)&bByte,
                      1);
} /* ET9_CP_UdbWriteWord() */

/* Increment Udb's dwDirtyCount
 * Should be called during every UDB write */
static void ET9LOCALCALL ET9_CP_WriterSetDirtyCount(ET9CPLingInfo *pET9CPLingInfo)
{
    ET9U32 dwNewDirtyCount;

    ET9Assert(pET9CPLingInfo && pET9CPLingInfo->pUdb);

    dwNewDirtyCount = pET9CPLingInfo->pUdb->dwDirtyCount + 1;

    /* Write it to Udb */
    ET9_CP_WriteUdbData(pET9CPLingInfo,
        (void ET9FARDATA*)&(pET9CPLingInfo->pUdb->dwDirtyCount),
        (const void ET9FARDATA*)&dwNewDirtyCount,
        sizeof(dwNewDirtyCount));
}

static ET9U16 ET9LOCALCALL ET9_CP_UdbPointerDiff(ET9CPUdbInfo ET9FARDATA *pUdb,
                                            ET9U8 ET9FARDATA * pbStart,
                                            ET9U8 ET9FARDATA * pbEnd)
{
    ET9U16 wDiff;
    if (pbStart <= pbEnd) {
        wDiff = (ET9U16)(pbEnd - pbStart);
    }
    else {
        wDiff = (ET9U16)(ET9_CP_UdbDataAreaBytes(pUdb) - (pbStart - pbEnd));
    }
    return wDiff;
} /* ET9_CP_UdbPointerDiff() */

/* Count the total number of words in the whole UDB */
static ET9U16 ET9LOCALCALL ET9_CP_CountWords(ET9CPUdbInfo ET9FARDATA *pUdb)
{
    ET9U16 wWordCount = 0;
    ET9U8 b;
    ET9Assert(pUdb);
    for (b = 0; b < ET9_CP_UDBZONEMAX; b++) {
        wWordCount = (ET9U16)(wWordCount + pUdb->wZoneWordCount[b]);
    }
    return wWordCount;
}/* ET9_CP_CountWords() */

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
                                  ET9U8 bOption)
{
    ET9U8 ET9FARDATA * pbEntryHead;
    ET9UINT nCheckSum;
    ET9UINT nIsFreeType;
    ET9U16 wEntrySize;
    ET9U8 bWordSize;
    ET9U8 bData;

    ET9Assert(pUdb);
    ET9Assert((pbData >= ET9_CP_UdbData(pUdb)) && (pbData < ET9_CP_UdbEnd(pUdb)));
    ET9Assert(pObj && pObj->pwID);

    /* init local variables */
    pbEntryHead = pbData;
    bData = *pbData;
    nCheckSum = 0;

    if (bOption & ET9_CP_GET_TYPE) { /* Get type */
        ET9_CP_EntryType eType;
        if (0 == (bData & ET9_CP_NON_FREE_TYPE_MASK)) {
            eType = ET9_CP_FREE; /* ET9_CP_FREE entry */
        }
        else {
            ET9U16 wOffset, wPhoneticStart, wStrokeStart;
            ET9U8 bIsStroke = 0;
            wOffset = (ET9U16)(pbData - ET9_CP_UdbData(pUdb));
            wPhoneticStart = pUdb->wZoneOffset[0];
            wStrokeStart = pUdb->wZoneOffset[ET9_CP_UDBPHONETICZONES];
            /* check if wOffset belongs to stroke zones */
            if (wStrokeStart > wPhoneticStart) {
                if ((wOffset >= wStrokeStart) || (wOffset < wPhoneticStart)) {
                    bIsStroke = 1;
                }
            }
            else if (wStrokeStart < wPhoneticStart) { /* stroke zone starts at a wrapped around offset */
                if ((wOffset >= wStrokeStart) && (wOffset < wPhoneticStart)) {
                    bIsStroke = 1;
                }
            }
            else { /* either all stroke zones or all phonetic zones are zero-width */
                const ET9U16 *pwWordCount = &(pUdb->wZoneWordCount[ET9_CP_UDBPHONETICZONES]);
                ET9U8 b;
                for (b = 0; b < ET9_CP_UDBSTROKEZONES; b++) {
                    if (*pwWordCount++) {
                        break;
                    }
                }
                if (b < ET9_CP_UDBSTROKEZONES) {
                    bIsStroke = 1;
                }
            }
            /* Phonetic words are stored in the first ET9_CP_UDBPHONETICZONES zones */
            if (bIsStroke) {
                if (bData & ET9_CP_RDB_TYPE_MASK) {
                    eType = ET9_CP_RDBSTROKE;
                }
                else if (bData & ET9_CP_AUDB_TYPE_MASK) {
                    eType = ET9_CP_AUDBSTROKE;
                }
                else {
                    eType = ET9_CP_UDBSTROKE;
                }
            }
            else {
                if (bData & ET9_CP_RDB_TYPE_MASK) {
                    eType = ET9_CP_RDBPHONETIC;
                }
                else if (bData & ET9_CP_AUDB_TYPE_MASK) {
                    eType = ET9_CP_AUDBPHONETIC;
                }
                else {
                    eType = ET9_CP_UDBPHONETIC;
                }
            }
        } /* END non-ET9_CP_FREE entry */
        pObj->eType = eType;
    }
    nIsFreeType = (ET9_CP_FREE == pObj->eType) ? 1 : 0;

    if (bOption & ET9_CP_GET_ENTRY_SIZE) { /* Get size */
        if (nIsFreeType) { /* ET9_CP_FREE entry format: [0yxxxxxx] [zzzzzzzz] */
            /* y == 0: size = value of bits 'x' */
            wEntrySize = (ET9U16)(bData & ET9_CP_FREE_SIZE_UPPER_MASK);
            /* y == 1: size = value of bits 'x' and 'z' combined */
            if (bData & ET9_CP_FREE_SIZE_FLAG) {
                pbData = ET9_CP_MoveUdbPR(pUdb, pbData, 1);
                wEntrySize = (ET9U16)((wEntrySize << 8) + *pbData);
            }
        }
        else { /* Non-ET9_CP_FREE entry: compute entry size from word size */
            bWordSize = (ET9U8)((bData & ET9_CP_WORD_SIZE_MASK) + ET9_CP_MIN_WORD_SIZE);
            ET9Assert(bWordSize <= ET9CPMAXUDBPHRASESIZE);
            wEntrySize = (ET9U16)ET9_CP_WordSizeToEntrySize(bWordSize);
        }
        pObj->wEntrySize = wEntrySize;
    }
    else {
        wEntrySize = pObj->wEntrySize; /* assume passed in through pObj */
    }

    if (bOption & ET9_CP_GET_CHECKSUM) { /* Get checksum */
        if (nIsFreeType) { /* ET9_CP_FREE entries only check type and size info */
            nCheckSum += *pbEntryHead;
            /* get next byte of size info if size > 0x3F */
            if (wEntrySize > ET9_CP_FREE_SIZE_UPPER_MASK) {
                pbData = ET9_CP_MoveUdbPR(pUdb, pbEntryHead, 1);
                nCheckSum += *pbData;
            }
        }
        else { /* Non-ET9_CP_FREE entries are checked as byte array */
            ET9U8 b;
            ET9Assert(wEntrySize <= 0xFF); /* because word size <= 32 */
            for (pbData = pbEntryHead, b = 0; b < wEntrySize; b++) {
                nCheckSum += *pbData;
                pbData = ET9_CP_MoveUdbPR(pUdb, pbData, 1);
            }
        }
    } /* END get checksum */

    /* Get ID and freq for non-ET9_CP_FREE type only.  Undefined for ET9_CP_FREE type */
    if (!nIsFreeType) {
        if (bOption & ET9_CP_GET_ID) { /* Get ID */
            ET9U8 b;
            bWordSize = ET9_CP_EntrySizeToWordSize(wEntrySize);
            ET9Assert(bWordSize <= ET9CPMAXUDBPHRASESIZE);
            pbData = ET9_CP_MoveUdbPR(pUdb, pbEntryHead, 3); /* go to ID part */
            for (b = 0; b < bWordSize; b++) {
                pObj->pwID[b] = ET9_CP_UdbReadWord(pUdb, pbData);
                pbData = ET9_CP_MoveUdbPR(pUdb, pbData, 2);
            }
        } /* END get ID */

        if (bOption & ET9_CP_GET_FREQ) { /* Get frequency */
            pbData = ET9_CP_MoveUdbPR(pUdb, pbEntryHead, 1); /* go to freq part */
            pObj->wFreq = ET9_CP_UdbReadWord(pUdb, pbData);
        } /* END get frequency */

    } /* END non-ET9_CP_FREE type operations */

    /* checksum == 0 if not needed */
    return (ET9U16)nCheckSum;
} /* ET9_CP_GetEntryInfo() */

/*------------------------------------------------------------------------
 *  Function    : ET9_CP_GetUdbCheckSum
 *
 *  Synopsis    : Compute and return the checksum of the whole UDB.
 *
 *     Input:   : pUdb = pointer to the UDB
 *
 *    Output    : none
 *
 *    Return    : checksum of the whole UDB
 *-----------------------------------------------------------------------*/
static ET9U16 ET9LOCALCALL ET9_CP_GetUdbCheckSum(ET9CPUdbInfo ET9FARDATA *pUdb)
{
    ET9UINT nCheckSum = 0;

    ET9Assert(pUdb);
    /* Header area checksum */
    {
        ET9U8 b;
        /* only include fields that affect UDB integrity */
        nCheckSum = (ET9UINT)(pUdb->wDataSize + pUdb->wFreeBytes + pUdb->wCutOffFreq);
        for (b = 0; b < ET9_CP_UDBZONEMAX; b++) {
            nCheckSum += pUdb->wZoneOffset[b];
            nCheckSum += pUdb->wZoneWordCount[b];
        }
    }
    /* Data area checksum */
    {
        ET9U8 ET9FARDATA * pbCurrent;
        ET9_CP_RUdbObj sObj;
        ET9INT sToCheck;
        /* start from first zone, which may not be at the start of data area. */
        pbCurrent = ET9_CP_UdbData(pUdb) + pUdb->wZoneOffset[0];
        for (sToCheck = ET9_CP_UdbDataAreaBytes(pUdb); sToCheck > 0;) {
            nCheckSum += ET9_CP_GetEntryInfo(pUdb, pbCurrent, &sObj,
                                           ET9_CP_GET_TYPE | ET9_CP_GET_ENTRY_SIZE | ET9_CP_GET_CHECKSUM);
            if (0 == sObj.wEntrySize) {
                break; /* entry size corrupted, avoid infinite loop */
            }
            /* next entry */
            sToCheck -= sObj.wEntrySize;
            pbCurrent = ET9_CP_MoveUdbPR(pUdb, pbCurrent, sObj.wEntrySize);
        }
        ET9Assert(0 == sToCheck);
    } /* END data area */
    return (ET9U16)nCheckSum;
} /* ET9_CP_GetUdbCheckSum() */

/*------------------------------------------------------------------------
 *  Function    : ET9_CP_UdbChkIntegrity
 *
 *  Synopsis    : Check the integrity of the whole UDB.
 *
 *     Input:   : pUdb = pointer to the UDB
 *                wUdbSize = number of bytes of the whole UDB
 *
 *    Output    : none
 *
 *    Return    : 1 if the integrity check pass; 0 otherwise.
 *-----------------------------------------------------------------------*/
static int ET9LOCALCALL ET9_CP_UdbChkIntegrity(ET9CPUdbInfo ET9FARDATA *pUdb,
                                           ET9U16 wUdbSize)
{
    ET9U8 ET9FARDATA * pbCurrent;
    ET9U8 ET9FARDATA * pbNextZone;
    ET9INT sToCheck;
    ET9U16 wFree, wZoneWordCount;
    ET9U8 b, bSpecial = 0;
    ET9_CP_RUdbObj sObj;

    sToCheck = ET9_CP_UdbDataAreaBytes(pUdb);
    if (sToCheck != (ET9INT)(wUdbSize - ET9_CP_UdbHeaderBytes(pUdb))) {
        return 0;
    }
    pbCurrent = ET9_CP_UdbData(pUdb) + pUdb->wZoneOffset[0];
    pbNextZone = ET9_CP_UdbData(pUdb) + pUdb->wZoneOffset[1];
    wFree = pUdb->wFreeBytes;
    for (b = 0, wZoneWordCount = 0; (b < ET9_CP_UDBZONEMAX) && (sToCheck > 0);) {
        if ((pbCurrent == pbNextZone) && !bSpecial) { /* at zone boundary */
            if (wZoneWordCount != pUdb->wZoneWordCount[b]) {
                /* special case: one zone take up all space */
                bSpecial = 1;
            }
            else { /* this zone is done, go to next zone */
                ++b;
                wZoneWordCount = 0;
                pbNextZone = ET9_CP_UdbData(pUdb) + pUdb->wZoneOffset[ET9_CP_NextZone(b)];
            }
        }
        else { /* within a zone */
            ET9_CP_GetEntryInfo(pUdb, pbCurrent, &sObj, ET9_CP_GET_TYPE | ET9_CP_GET_ENTRY_SIZE);
            if (0 == sObj.wEntrySize) {
                return 0; /* entry size corrupted, avoid infinite loop */
            }
            else if (ET9_CP_FREE == sObj.eType) {
                wFree = (ET9U16)(wFree - sObj.wEntrySize);
            }
            else {
                wZoneWordCount++;
            }
            /* next entry */
            sToCheck -= sObj.wEntrySize;
            pbCurrent = ET9_CP_MoveUdbPR(pUdb, pbCurrent, sObj.wEntrySize);
        }
    }
    return (0 == sToCheck) &&
           (0 == wFree) &&
           (b < ET9_CP_UDBZONEMAX) &&
           (pbCurrent == ET9_CP_UdbData(pUdb) + pUdb->wZoneOffset[0]) &&
           (pUdb->wDataCheck == ET9_CP_GetUdbCheckSum(pUdb));
} /* ET9_CP_UdbChkIntegrity() */

/* Write the content of an obj as an entry at the given offset
 * Return the entry's checksum */
static ET9U16 ET9LOCALCALL ET9_CP_WriteEntry(ET9CPLingInfo *pET9CPLingInfo,
                                        ET9_CP_RUdbObj *pObj,
                                        ET9U16 wTargetOffset)
{
    /* Buffer for writing the whole entry as a block.
     * Minimize overhead for write operations */
    ET9U8 pbBuf[ET9_CP_WordSizeToEntrySize(ET9CPMAXUDBPHRASESIZE)];
    ET9UINT nBufLen; /* length of the buffer above */
    ET9UINT i;
    ET9UINT nCheckSum;
    ET9UINT nWordSize;

    ET9Assert(pET9CPLingInfo);
    ET9Assert(pET9CPLingInfo->pUdb);
    ET9Assert(wTargetOffset < ET9_CP_UdbDataAreaBytes(pET9CPLingInfo->pUdb));
    ET9Assert(pObj && (pObj->pwID));
    /* ET9_CP_FREE type    : EntrySize <= ET9_CP_MAX_FREE_SIZE */
    /* Non-ET9_CP_FREE type: WordSize <= ET9CPMAXUDBPHRASESIZE */
    ET9Assert(((ET9_CP_FREE == pObj->eType) && (pObj->wEntrySize <= ET9_CP_MAX_FREE_SIZE)) ||
             ((ET9_CP_FREE != pObj->eType) && (pObj->wEntrySize <= ET9_CP_WordSizeToEntrySize(ET9CPMAXUDBPHRASESIZE))));
    ET9Assert(pObj->eType <= ET9_CP_UDBSTROKE);

    /* Form the entry in the data buffer */

    /* ET9_CP_FREE entry format: [0yxxxxxx] [zzzzzzzz] */
    if (ET9_CP_FREE == pObj->eType) {
        /* size <= 0x3F, use 1 byte */
        if (pObj->wEntrySize <= ET9_CP_FREE_SIZE_UPPER_MASK) {
            /* y = 0, x = size */
            pbBuf[0] = ET9_CP_LOBYTE(pObj->wEntrySize);
            nBufLen = 1;
        }
        /*  0x3F < size <= ET9_CP_MAX_FREE_SIZE, use 2 bytes */
        else {
            /* y = 1, x = hi byte of size */
            pbBuf[0] = (ET9U8)(ET9_CP_HIBYTE(pObj->wEntrySize) | ET9_CP_FREE_SIZE_FLAG);
            /* z = lo byte of size */
            pbBuf[1] = ET9_CP_LOBYTE(pObj->wEntrySize);
            nBufLen = 2;
        }
    }
    /* ET9_CP_UDBPHONETIC entry format : [10Lw wwww] [freq]x2 [Phonetic ID]x w */
    /* ET9_CP_RDBPHONETIC entry format : [11Lw wwww] [freq]x2 [Phonetic ID]x w */
    /* ET9_CP_UDBSTROKE entry format   : [10Lw wwww] [freq]x2 [Stroke ID]x w */
    /* ET9_CP_RDBSTROKE entry format   : [11Lw wwww] [freq]x2 [Stroke ID]x w */
    else {
        ET9U8 bData = ET9_CP_NON_FREE_TYPE_MASK;
        /* ET9_CP_UDBPHONETIC or ET9_CP_UDBSTROKE   : 100x xxxx
         * ET9_CP_RDBPHONETIC or ET9_CP_RDBSTROKE   : 110x xxxx
         * ET9_CP_AUDBPHONETIC or ET9_CP_AUDBSTROKE : 101x xxxx */
        if (pObj->eType <= ET9_CP_RDBSTROKE) {
            bData = (ET9U8)(ET9_CP_RDB_TYPE_MASK | bData);
        }
        else if (pObj->eType <= ET9_CP_AUDBSTROKE) {
            bData = (ET9U8)(ET9_CP_AUDB_TYPE_MASK | bData);
        }
        nWordSize = ET9_CP_EntrySizeToWordSize(pObj->wEntrySize);
        bData = (ET9U8)(bData | (nWordSize - ET9_CP_MIN_WORD_SIZE));

        pbBuf[0] = bData;
        pbBuf[1] = ET9_CP_HIBYTE(pObj->wFreq); /* hi byte of frequency */
        pbBuf[2] = ET9_CP_LOBYTE(pObj->wFreq); /* lo byte of frequency */

        ET9Assert(nWordSize <= ET9CPMAXUDBPHRASESIZE);
        for (i = 0; i < nWordSize; i++) {
            pbBuf[2*i+3] = ET9_CP_HIBYTE(pObj->pwID[i]); /* hi byte of pwID[i] */
            pbBuf[2*i+4] = ET9_CP_LOBYTE(pObj->pwID[i]); /* lo byte of pwID[i] */
        }
        nBufLen = pObj->wEntrySize;
    }

    /* Compute checksum as a byte array */
    nCheckSum = 0;
    for (i = 0; i < nBufLen; i++) {
        nCheckSum += pbBuf[i];
    }

    /* write the buffer to the UDB's circular data area */
    {
        ET9CPUdbInfo ET9FARDATA *pUdb;
        ET9U8 ET9FARDATA * pDest;
        ET9U8 *pbSrc;
        ET9UINT nSizeBeforeWrap;

        pUdb = pET9CPLingInfo->pUdb;
        pDest = ET9_CP_UdbData(pUdb) + wTargetOffset;
        pbSrc = pbBuf;

        /* Write the part before wrap if it needs to wrap around */
        nSizeBeforeWrap = ET9_CP_UdbDataAreaBytes(pUdb) - wTargetOffset;
        if (nSizeBeforeWrap < nBufLen) {

            ET9_CP_WriteUdbData(pET9CPLingInfo,
                              (void ET9FARDATA*)pDest,
                              (const void ET9FARDATA*)pbSrc,
                              nSizeBeforeWrap);

            /* setup for the remaining part */
            pDest = ET9_CP_UdbData(pUdb);
            pbSrc += nSizeBeforeWrap;
            nBufLen -= nSizeBeforeWrap;
        }
        /* Write the whole buffer, or the remaining part if wrapped around */
        ET9_CP_WriteUdbData(pET9CPLingInfo,
                          (void ET9FARDATA*)pDest,
                          (const void ET9FARDATA*)pbSrc,
                          nBufLen);
    }

    return (ET9U16)nCheckSum;

} /* ET9_CP_WriteEntry() */

/* UDB is over-crowded if it has < 5% space free
 * Return 1 if UDB is over-crowded
 *        0 otherwise */
static ET9UINT ET9LOCALCALL ET9_CP_UdbOverCrowded(ET9CPUdbInfo ET9FARDATA *pUdb)
{
    ET9Assert(pUdb);

    if (pUdb->wFreeBytes < (ET9_CP_UdbDataAreaBytes(pUdb) / ET9_CP_UDB_MIN_FREE_FACTOR)) {
        return 1;
    }
    return 0;
} /* ET9_CP_UdbOverCrowded() */

/*------------------------------------------------------------------------
 *  Function    : ET9_CP_CleanUpUdb
 *
 *  Synopsis    : Clean up the UDB until
 *                1. all RDB entries at/below cutoff frequency are freed.
 *                2. all UDB entries below 1/2 of cutoff frequency are freed.
 *                3. 1/ET9_CP_UDB_MIN_CLEANUP_FACTOR of space is freed.  This is
 *                   guaranteed by adjusting the cutoff level after each loop.
 *                Update the cutoff frequency in UDB header to reflect the
 *                cutoff level after the cleanup.
 *                Update all affected header fields except the checksum,
 *                which is output to the caller for further updates.
 *
 *     Input:   : pET9CPLingInfo = Pointer to Chinese XT9 LingInfo
 *                pnCheckSum = current checksum
 *
 *    Output    : pnCheckSum = updated checksum
 *
 *    Return    : none
 *-----------------------------------------------------------------------*/
static void ET9LOCALCALL ET9_CP_CleanUpUdb(ET9CPLingInfo *pET9CPLingInfo,
                                       ET9UINT *pnCheckSum)
{
    ET9CPUdbInfo ET9FARDATA *pUdb;
    ET9INT sBytesToCheck;
    ET9UINT nCheckSum;
    ET9U16 wFreeBytes;
    ET9U16 wCutOffFreq, wNewCutOffFreq, wNextLoFreq;
    ET9U16 wCutOffBytes[ET9_CP_UDB_CUTOFF_LEVELS];
    ET9U16 wWordsToCheck; /* non-free entries to be checked in current zone */
    ET9U16 wZoneWordCount[ET9_CP_UDBZONEMAX];
    ET9U16 wSizeNeeded;
    ET9U8 ET9FARDATA * pbCurrent;
    ET9U16 wEntryCheckSum;
    ET9_CP_RUdbObj currentObj;
    ET9U8 bZone;

    ET9Assert(pET9CPLingInfo);
    ET9Assert(pET9CPLingInfo->pUdb);
    /* init local variables */
    pUdb = pET9CPLingInfo->pUdb;
    nCheckSum = *pnCheckSum;
    for (bZone = 0; bZone < ET9_CP_UDBZONEMAX; bZone++) {
        wZoneWordCount[bZone] = pUdb->wZoneWordCount[bZone];
    }
    wNewCutOffFreq = pUdb->wCutOffFreq;
    wFreeBytes = pUdb->wFreeBytes;
    wSizeNeeded = (ET9U16)(ET9_CP_UdbDataAreaBytes(pUdb) / ET9_CP_UDB_MIN_CLEANUP_FACTOR);

    do {
        /* reset pointer and cutoff sizes before rescan */
        pbCurrent = ET9_CP_UdbData(pUdb) + pUdb->wZoneOffset[0];
        sBytesToCheck = ET9_CP_UdbDataAreaBytes(pUdb);
        bZone = 0;
        wWordsToCheck = wZoneWordCount[0];
        wCutOffFreq = wNewCutOffFreq;
        wNextLoFreq = ET9_CP_MAXFREQ;
        {
            ET9U8 b;
            for (b = 0; b < ET9_CP_UDB_CUTOFF_LEVELS; b++) {
                wCutOffBytes[b] = 0;
            }
        }
        do {
            wEntryCheckSum = ET9_CP_GetEntryInfo(pUdb, pbCurrent, &currentObj,
                                               ET9_CP_GET_TYPE | ET9_CP_GET_ENTRY_SIZE | ET9_CP_GET_CHECKSUM | ET9_CP_GET_FREQ);
            if (ET9_CP_FREE != currentObj.eType) {
                ET9U8 bTrim;
                /* skip empty zones */
                while (!wWordsToCheck) {
                    bZone++;
                    wWordsToCheck = wZoneWordCount[bZone];
                }
                ET9Assert(bZone < ET9_CP_UDBZONEMAX);
                wWordsToCheck--;
                bTrim = 0;
                /* UDB entries is cutoff only if their freq < cutoff freq / 2 */
                if (currentObj.eType >= ET9_CP_UDBPHONETIC) {
                    currentObj.wFreq = (ET9U16)((currentObj.wFreq >= (ET9_CP_MAXFREQ / 2)) ?
                                               ET9_CP_MAXFREQ : currentObj.wFreq * 2 + 1);
                }
                if (currentObj.wFreq <= wCutOffFreq) {
                    bTrim = 1;
                }
                else if (currentObj.wFreq < wNextLoFreq) {
                    wNextLoFreq = currentObj.wFreq;
                }
                /* Trim the current entry */
                if (bTrim) {
                    /* update word count for the current zone */
                    (wZoneWordCount[bZone])--;
                    /* write a ET9_CP_FREE entry with the current entry size */
                    currentObj.eType = ET9_CP_FREE;
                    nCheckSum -= wEntryCheckSum;
                    nCheckSum += ET9_CP_WriteEntry(pET9CPLingInfo, &currentObj,
                                                 (ET9U16)(pbCurrent - ET9_CP_UdbData(pUdb)));
                    /* update free bytes */
                    wFreeBytes = (ET9U16)(wFreeBytes + currentObj.wEntrySize);
                    /* update sizes needed */
                    if (wSizeNeeded > currentObj.wEntrySize) {
                        wSizeNeeded = (ET9U16)(wSizeNeeded - currentObj.wEntrySize);
                    }
                    else {
                        wSizeNeeded = 0;
                    }
                }
                else {
                    ET9U8 b;
                    /* Find the cumulative cutoff bytes at each estimated
                     * freq level.
                     * Note: Estimated freq levels goes descending here.
                     * Thus the other for loop, which choose one of the estimated
                     * freq level, must also go in the same way. */
                    wNewCutOffFreq = (ET9U16)((wCutOffFreq >= (ET9_CP_MAXFREQ / 2)) ?
                                             ET9_CP_MAXFREQ : wCutOffFreq * 2);
                    for (b = ET9_CP_UDB_CUTOFF_LEVELS; b > 0; b--) {
                        if (currentObj.wFreq > wNewCutOffFreq) {
                            break;
                        }
                        wCutOffBytes[b - 1] = (ET9U16)(wCutOffBytes[b - 1] +
                                                      currentObj.wEntrySize);
                        wNewCutOffFreq = (ET9U16)(wNewCutOffFreq -
                                                 (wCutOffFreq / ET9_CP_UDB_CUTOFF_LEVELS));
                    }
                }
            } /* END non-free */

            /* Next entry */
            sBytesToCheck -= currentObj.wEntrySize;
            ET9Assert(sBytesToCheck >= 0);
            pbCurrent = ET9_CP_MoveUdbPR(pUdb, pbCurrent, currentObj.wEntrySize);
        } while (sBytesToCheck);

        /* If didn't clean up enough size, adjust the cutoff frequency
         * to the estimated level before rescan */
        if (wSizeNeeded) {
            if (wCutOffBytes[ET9_CP_UDB_CUTOFF_LEVELS - 1] < wSizeNeeded) {
                /* highest estimated cutoff level doesn't give enough bytes,
                 * rescan with double cutoff level. (Should happen very rarely) */
                wNewCutOffFreq = (ET9U16)((wCutOffFreq >= (ET9_CP_MAXFREQ / 2)) ?
                                         ET9_CP_MAXFREQ : wCutOffFreq * 2);
            }
            else {
                ET9U8 b;
                /* Set new cutoff freq to the lowest estimated level that can
                 * give required size.
                 * Note: Estimated freq levels must go in the same way as the
                 * above loop, which finds the cumulative cutoff bytes. */
                wNewCutOffFreq = (ET9U16)((wCutOffFreq >= (ET9_CP_MAXFREQ / 2)) ?
                                         ET9_CP_MAXFREQ : wCutOffFreq * 2);
                for (b = ET9_CP_UDB_CUTOFF_LEVELS - 1; b > 0; b--) {
                    if (wCutOffBytes[b - 1] < wSizeNeeded) {
                        break;
                    }
                    wNewCutOffFreq = (ET9U16)(wNewCutOffFreq -
                                             (wCutOffFreq / ET9_CP_UDB_CUTOFF_LEVELS));
                }
            }
            /* To ensure we cutoff some entries in the next round */
            if (wNewCutOffFreq < wNextLoFreq) {
                wNewCutOffFreq = wNextLoFreq;
            }
        }
        /* Already cleaned up enough space, use the next lowest frequency
         * as the new cutoff for next cleanup call */
        else {
            wNewCutOffFreq = wNextLoFreq;
        }
    } while (wSizeNeeded);

    /* Update affected UDB header fields */
    /* Update free bytes */
    nCheckSum = nCheckSum - pUdb->wFreeBytes + wFreeBytes;
    ET9_CP_WriteUdbData(pET9CPLingInfo,
                      (void ET9FARDATA*)&(pUdb->wFreeBytes),
                      (const void ET9FARDATA*)&wFreeBytes,
                      sizeof(wFreeBytes));
    /* Update cutoff freq */
    nCheckSum = nCheckSum - pUdb->wCutOffFreq + wNewCutOffFreq;
    ET9_CP_WriteUdbData(pET9CPLingInfo,
                      (void ET9FARDATA*)&(pUdb->wCutOffFreq),
                      (const void ET9FARDATA*)&wNewCutOffFreq,
                      sizeof(wNewCutOffFreq));
    /* Update word count for each zone */
    for (bZone = 0; bZone < ET9_CP_UDBZONEMAX; bZone++) {
        if (wZoneWordCount[bZone] != pUdb->wZoneWordCount[bZone]) {
            nCheckSum = nCheckSum - pUdb->wZoneWordCount[bZone] + wZoneWordCount[bZone];
            ET9_CP_WriteUdbData(pET9CPLingInfo,
                              (void ET9FARDATA*)&(pUdb->wZoneWordCount[bZone]),
                              (const void ET9FARDATA*)&(wZoneWordCount[bZone]),
                              sizeof(wZoneWordCount[bZone]));
        }
    }
    /* Updated checksum is output to caller for further update before
     * writing to UDB header */
    *pnCheckSum = nCheckSum;
} /* ET9_CP_CleanUpUdb() */

/*------------------------------------------------------------------------
 *  Function    : ET9_CP_UdbSetupObj
 *
 *  Synopsis    : Setup an obj from the given PID/SID.
 *
 *     Input:   : pwID = array of given PID/SID.
 *                bWordSize = number of characters in the array.
 *                bIsAuto = 1: auto-created (RDB / AUDB type)
 *                          0: user-created (RDB / UDB type)
 *                bIsLdbPhrase = 1: this is an Ldb phrase
 *                               0: otherwise
 *                wFreq = (> 0): to use this frequency for the obj
 *                        0: to use default frequency.
 *                eEncode = ID encoding (either PID or SID)
 *
 *    Output    : pOutObj = the result obj
 *
 *    Return    : none
 *-----------------------------------------------------------------------*/
static void ET9LOCALCALL ET9_CP_UdbSetupObj(
    ET9U16 *pwID,
    ET9U8 bWordSize,
    ET9BOOL bIsAuto,
    ET9BOOL bIsLdbPhrase,
    ET9U16 wFreq,
    ET9_CP_IDEncode eEncode,
    ET9_CP_RUdbObj *pOutObj)
{
    ET9Assert(pwID);
    ET9Assert(bWordSize <= ET9CPMAXUDBPHRASESIZE);
    ET9Assert(pOutObj);

    /* determine type with ID encode */
    if (ET9_CP_IDEncode_SID == eEncode) {
        if (bIsLdbPhrase) {
            pOutObj->eType = ET9_CP_RDBSTROKE;
        }
        else {
            if (bIsAuto) {
                pOutObj->eType = ET9_CP_AUDBSTROKE;
            }
            else {
                pOutObj->eType = ET9_CP_UDBSTROKE;
            }
        }
    }
    else {
        ET9Assert((ET9_CP_IDEncode_PID == eEncode) ||
                 (ET9_CP_IDEncode_BID == eEncode));
        if (bIsLdbPhrase) {
            pOutObj->eType = ET9_CP_RDBPHONETIC;
        }
        else {
            if (bIsAuto) {
                pOutObj->eType = ET9_CP_AUDBPHONETIC;
            }
            else {
                pOutObj->eType = ET9_CP_UDBPHONETIC;
            }
        }
    }
    /* compute entry size from word size */
    pOutObj->wEntrySize = (ET9U16)ET9_CP_WordSizeToEntrySize(bWordSize);
    /* Setup the freq.  ET9_CP_UdbAddObj() will use it.  ET9_CP_UdbRecordUsage() will ignore it */
    if (wFreq) { /* use caller specified freq */
        pOutObj->wFreq = wFreq;
    }
    else { /* use init freq based on type */
        if (bIsLdbPhrase) {
            pOutObj->wFreq = (ET9U16)ET9_CP_RDB_INITFREQ;
        }
        else {
            if (bIsAuto) {
                pOutObj->wFreq = (ET9U16)ET9_CP_AUDB_INITFREQ;
            }
            else {
                pOutObj->wFreq = (ET9U16)ET9_CP_UDB_INITFREQ;
            }
        }
    }
    /* Assume PID or SID from input.  Will be used according to type */
    {
        ET9U8 b;
        for (b = 0; b < bWordSize; b++) {
            pOutObj->pwID[b] = pwID[b];
        }
    }
} /* end ET9_CP_UdbSetupObj() */

/*------------------------------------------------------------------------
 *  Function    : ET9_CP_UdbZoneForID
 *
 *  Synopsis    : Find the zone where an obj belongs if the obj starts with the given ID.
 *                UDB zoning is based on the first letter of the first syllable.
 *
 *     Input:   : pET9CPLingInfo = pointer to Chinese XT9 LingInfo
 *                eEncode = ID encoding (PID, BID or SID)
 *                wID = PID, BID or SID that starts the obj
 *
 *    Output    : none
 *
 *    Return    : result zone index (0-based)
 *-----------------------------------------------------------------------*/
static ET9U8 ET9LOCALCALL ET9_CP_UdbZoneForID(ET9CPLingInfo *pET9CPLingInfo,
                                         ET9_CP_IDEncode eEncode,
                                         ET9U16 wID)
{
    ET9UINT nZone = ET9_CP_UDBZONEMAX;
    ET9Assert(pET9CPLingInfo);
    if ((ET9_CP_IDEncode_PID == eEncode) || (ET9_CP_IDEncode_BID == eEncode)){ /* PID or BID */
        ET9U8 pbSyl[ET9_CP_MAX_SINGLE_SYL_SIZE], bSylLen;

        ET9_CP_PidBidToSyllable(pET9CPLingInfo, wID, pbSyl, &bSylLen, (ET9U8)(ET9_CP_IDEncode_BID == eEncode));
        ET9Assert(bSylLen);

        if (ET9_CP_IDEncode_BID == eEncode) { /* BPMF encoded UDB */
            nZone = (pbSyl[0] | ET9_CP_BPMFUPPERCASEBIT) - ET9_CP_BPMFFIRSTUPPERLETTER;
        }
        else {  /* pinyin encoded UDB */
            ET9U8 bFirstLetter = ET9_CP_FIRSTLOWERLETTER;
            if (ET9_CP_IsPinyinUpperCase(pbSyl[0])) {
                bFirstLetter = ET9_CP_FIRSTUPLETTER;
            }
            nZone = pbSyl[0] - bFirstLetter;
        }
        ET9Assert(nZone < ET9_CP_UDBPHONETICZONES);
    }
    else {
#ifndef ET9CP_DISABLE_STROKE
        ET9U32 dwReadOffset;
        ET9UINT nZoneLimit, nSidPerZone;
        ET9Assert(ET9_CP_IDEncode_SID == eEncode); /* SID */

        dwReadOffset = pET9CPLingInfo->CommonInfo.sOffsets.dwSIDToPIDOffset;

        nSidPerZone = ET9_CP_LdbReadWord(pET9CPLingInfo, dwReadOffset) / ET9_CP_UDBSTROKEZONES; /* number of SIDs per zone */
        /* determine zone according to even distribution of SIDs */
        for (nZone = 0, nZoneLimit = nSidPerZone; nZone < ET9_CP_UDBSTROKEZONES - 1; nZone++) {
            if (wID < (ET9U16)nZoneLimit) {
                break;
            }
            nZoneLimit += nSidPerZone;
        }
        nZone += ET9_CP_UDBPHONETICZONES;
#else
        nZone = ET9_CP_UDBPHONETICZONES;
#endif
    }
    ET9Assert(nZone < ET9_CP_UDBZONEMAX);
    return (ET9U8)nZone;
} /* ET9_CP_UdbZoneForID() */

/*------------------------------------------------------------------------------
 * Function   : ET9_CP_UdbCompareObj
 *
 * Description: Returns if an obj should be before/after a ref obj
 *              If ref obj is PID encoded, PID ranges for its syllables has been
 *              setup in CommonInfo.pwRange
 *              (Assumes the obj and ref obj are of the same encoding
 *               both in PID or both in SID)
 *              Sorting Criteria:
 *              Phonetic type: 1. alphabetic order of each syllable (need to setup PID range)
 *                             2. ascending number of syllables
 *                             3. ascending PID for each character
 *              Stroke type: 1. ascending SID for each chaaracter
 *                           2. ascending number of characters
 *
 *      Input:  eEncode      - encoding of pObj and pRefObj
 *              pObj         - pointer to the obj to be checked
 *              pRefObj      - pointer to the ref obj
 *
 *      Return: 2 if the obj should be before the ref obj
 *              1 if the obj should be after the ref obj
 *              0 if the obj is identical to the ref obj except frequency
 *
 *----------------------------------------------------------------------------*/
static ET9U8 ET9LOCALCALL ET9_CP_UdbCompareObj(ET9CPLingInfo *pET9CPLingInfo,
                                          ET9_CP_IDEncode eEncode,
                                          ET9_CP_RUdbObj *pObj,
                                          ET9_CP_RUdbObj *pRefObj)
{
    ET9U8 bWordSize, bRefWordSize, b;

    ET9Assert(pObj);
    ET9Assert(pRefObj);
    bWordSize = ET9_CP_EntrySizeToWordSize(pObj->wEntrySize);
    bRefWordSize = ET9_CP_EntrySizeToWordSize(pRefObj->wEntrySize);
    ET9Assert(bWordSize <= ET9CPMAXUDBPHRASESIZE);
    ET9Assert(bRefWordSize <= ET9CPMAXUDBPHRASESIZE);

    /* Phonetic type obj, encoded in PID */
    if ((ET9_CP_IDEncode_PID == eEncode) || (ET9_CP_IDEncode_BID == eEncode)) {
        ET9U16 *pwStart, *pwEnd;
        pwStart = pET9CPLingInfo->CommonInfo.pwRange;
        /* compare each syllable of obj against refObj */
        for (b = 0; (b < bWordSize) && (b < bRefWordSize); b++) {
            pwEnd = pwStart + 1;
            if (pObj->pwID[b] < *pwStart) { /* obj before refObj */
                return 2;
            }
            else if (pObj->pwID[b] >= *pwEnd) { /* obj after refObj */
                return 1;
            }
            pwStart += 2; /* same spelling for this syllable, next syllable */
        } /* END for each ID */

        /* if all syllables are identical,
         * the obj with shorter spelling is before the longer */
        if (bWordSize < bRefWordSize) {
            return 2;
        }
        else if (bWordSize > bRefWordSize) {
            return 1;
        }
    }
    /* Phonetic type obj: Same spellings, compare IDs
     * Stroke type obj  : Just compare IDs */
    for (b = 0; (b < bWordSize) && (b < bRefWordSize); b++) {
        /* obj before refObj */
        if (pObj->pwID[b] < pRefObj->pwID[b]) {
            return 2;
        }
        /* obj after refObj */
        else if (pObj->pwID[b] > pRefObj->pwID[b]) {
            return 1;
        }
        /* obj == refObj, next ID */
    } /* END for each ID */
    /* if all IDs are identical, the shorter obj is before the longer */
    if (bWordSize < bRefWordSize) {
        return 2;
    }
    else if (bWordSize > bRefWordSize) {
        return 1;
    }
    /* same syllables, IDs, and sizes, obj == refObj */
    return 0;
} /* ET9_CP_UdbCompareObj() */

/*------------------------------------------------------------------------
 *  Function    : ET9_CP_UdbGetMidObj
 *
 *  Synopsis    : Get mid obj within a given range of entry indices.
 *
 *     Input:   : pUdb = pointer to UDB
 *                pbStart = pointer to the entry at search range start, which
 *                          may be a ET9_CP_FREE entry.
 *                wStart, wEnd = search range indices
 *
 *    Output    : pwMid = result mid index
 *                pObj = result mid obj
 *
 *    Return    : pointer to the entry at mid index.
 *-----------------------------------------------------------------------*/
static ET9U8 ET9FARDATA * ET9LOCALCALL ET9_CP_UdbGetMidObj(ET9CPUdbInfo ET9FARDATA *pUdb,
                                                 ET9U8 ET9FARDATA * pbStart,
                                                 ET9U16 wStart,
                                                 ET9U16 wEnd,
                                                 ET9U16 *pwMid,
                                                 ET9_CP_RUdbObj *pObj)
{
    ET9U8 ET9FARDATA * pbCurrent;
    ET9U16 wMid, wCurrent; /* entry indices (1-based) */

    ET9Assert(pUdb && pbStart && pwMid && pObj);
    ET9Assert(wStart && (wStart <= wEnd));
    pbCurrent = pbStart;
    pObj->wEntrySize = 0;
    /* Init wCurrent = wStart - 1 to handle the case where wMid == wStart and
     * pbStart is pointing to a ET9_CP_FREE entry before the entry at wStart. */
    wCurrent = (ET9U16)(wStart - 1);
    wMid = (ET9U16)((wStart + wEnd) / 2);
    do {
        /* move pointer before get obj to ensure pbCurrent is pointing to the
         * entry stored in pObj when the while loop exit */
        pbCurrent = ET9_CP_MoveUdbPR(pUdb, pbCurrent, pObj->wEntrySize);
        ET9_CP_GetEntryInfo(pUdb, pbCurrent, pObj, ET9_CP_GET_TYPE | ET9_CP_GET_ENTRY_SIZE);
        if (ET9_CP_FREE != pObj->eType) {
            wCurrent++;
        }
    } while (wCurrent < wMid);
    ET9_CP_GetEntryInfo(pUdb, pbCurrent, pObj, ET9_CP_GET_TYPE | ET9_CP_GET_ENTRY_SIZE | ET9_CP_GET_ID);
    *pwMid = wMid;
    return pbCurrent;
} /* ET9_CP_UdbGetMidObj() */

/*------------------------------------------------------------------------
 *  Function    : ET9_CP_UdbFindObj
 *
 *  Synopsis    : Find the zone and offset of the given obj in the UDB.
 *
 *     Input:   : pET9CPLingInfo = pointer to Chinese XT9 LingInfo
 *                pObj = pointer to obj being found
 *                eEncode = ID encoding (either PID or SID)
 *
 *    Output    : *pbZone = zone index in which the obj is located, or should be
 *                          inserted if the obj is not found.
 *                *pwOffset = offset of the obj if found, or
 *                            offset for inserting the obj if not found.
 *
 *    Return    : 1 if the obj is found; 0 otherwise.
 *-----------------------------------------------------------------------*/
static ET9BOOL ET9LOCALCALL ET9_CP_UdbFindObj(
    ET9CPLingInfo *pET9CPLingInfo,
    ET9_CP_RUdbObj *pObj,
    ET9_CP_IDEncode eEncode,
    ET9U8 *pbZone,
    ET9U16 *pwOffset)
{
    ET9CPUdbInfo ET9FARDATA *pUdb;
    ET9U8 ET9FARDATA * pbStart;
    ET9U8 ET9FARDATA * pbMid;
    ET9_CP_RUdbObj sMidObj;
    ET9U16 wStart, wMid, wEnd;
    ET9U8 bCompareResult;
    ET9U16 wPhraseCount;

    ET9Assert(pET9CPLingInfo && pET9CPLingInfo->pUdb);
    ET9Assert(pObj && (pObj->wEntrySize <= ET9_CP_WordSizeToEntrySize(ET9CPMAXUDBPHRASESIZE)));
    ET9Assert(pbZone && pwOffset);

    pUdb = pET9CPLingInfo->pUdb;
    /* Find out which zone the obj should belong to */
    *pbZone = ET9_CP_UdbZoneForID(pET9CPLingInfo, eEncode, pObj->pwID[0]);
    wPhraseCount = pUdb->wZoneWordCount[*pbZone];

    /* No word in this zone.  No need to search.
     * Return not found with insertion point at the beginning of this zone. */
    if (0 == wPhraseCount) {
        *pwOffset = pUdb->wZoneOffset[*pbZone];
        return 0;
    }

    /* setup PID ranges for the pObj, for ordering by syllable */
    if ((ET9_CP_IDEncode_PID == eEncode) || (ET9_CP_IDEncode_BID == eEncode)) {
        ET9U16 *pwStart, *pwEnd, *pwID;
        ET9U8 pbSyl[ET9_CP_MAX_SINGLE_SYL_SIZE], bSylLen, bWordSize;
        bWordSize = ET9_CP_EntrySizeToWordSize(pObj->wEntrySize);
        pwID = pObj->pwID;
        pwStart = pET9CPLingInfo->CommonInfo.pwRange; /* todo: use local array to store PID ranges here */
        for (; bWordSize; bWordSize--, pwID++) {
            ET9BOOL bFoundPidRange;
            ET9_CP_PidBidToSyllable(pET9CPLingInfo, *pwID, pbSyl, &bSylLen, (ET9U8)(ET9_CP_IDEncode_BID == eEncode));
            pwEnd = pwStart + 1;
            bFoundPidRange = ET9_CP_SyllableToPidRange(pET9CPLingInfo, pbSyl, bSylLen, 0, pwStart, pwEnd, 0);
            ET9Assert(bFoundPidRange);
            pwStart += 2;
        }
    }
    /* binary search for the input obj in this zone. */
    /* Init start and end index */
    wStart = 1;
    wEnd = wPhraseCount;
    /* Init pbStart to point to beginning of this zone. */
    pbStart = ET9_CP_UdbData(pUdb) + pUdb->wZoneOffset[*pbZone];
    do {
        pbMid = ET9_CP_UdbGetMidObj(pUdb, pbStart, wStart, wEnd, &wMid, &sMidObj);
        /* Compare the mid with the input obj */
        bCompareResult = ET9_CP_UdbCompareObj(pET9CPLingInfo, eEncode, &sMidObj, pObj); /* todo: pass in local array with PID ranges */
        /* the mid should be after the input obj */
        if (1 == bCompareResult) {
            wEnd = (ET9U16)(wMid - 1);
        }
        /* the mid should be before the input obj */
        else if (2 == bCompareResult) {
            wStart = (ET9U16)(wMid + 1);
            /* start of the new range is immediately after the mid */
            pbStart = ET9_CP_MoveUdbPR(pUdb, pbMid, sMidObj.wEntrySize);
        }
        /* found the obj at mid. Return corresponding offset. */
        else { /* 0 == bCompareResult */
            break;
        }
    } while (wStart <= wEnd);
    /* Not found during the loop. */
    if (bCompareResult) {
        /* whether the mid should be before/after the in obj,
         * the in obj should still be inserted at pbStart. */
        pbMid = pbStart;
    }
    *pwOffset = ET9_CP_UdbPtrToOffset(pUdb, pbMid);
    return (ET9BOOL)(0 == bCompareResult);
} /* ET9_CP_UdbFindObj() */

/*------------------------------------------------------------------------
 *  Function    : ET9_CP_UdbRecordUsage
 *
 *  Synopsis    : Recode one use of an existing entry by updating its freq.
 *                Keep the RDB/UDB type of the existing entry unchanged.
 *                Update the checksum before return.
 *
 *     Input:   : pET9CPLingInfo = pointer to Chinese XT9 LingInfo
 *                pDstObj = pointer to an obj containing the type, entry size, and IDs
 *                          (but not the current frequency in the UDB)
 *                wInFreq = (> 0): take the higher btwn this and the existing frequency.
 *                          0: increase the frequency by default value.
 *                wOffset = offset at which the existing entry is found
 *
 *    Output    : none
 *
 *    Return    : none
 *-----------------------------------------------------------------------*/
static void ET9LOCALCALL ET9_CP_UdbRecordUsage(ET9CPLingInfo *pET9CPLingInfo,
                                              ET9_CP_RUdbObj *pDstObj,
                                              ET9U16 wInFreq,
                                              ET9U16 wOffset)
{
    ET9CPUdbInfo ET9FARDATA *pUdb;
    ET9U8 ET9FARDATA *pbDst;
    ET9U16 wNewFreq;
    ET9_CP_EntryType  eNewType;
    ET9U16 wCheckSum;

    ET9Assert(pET9CPLingInfo);
    ET9Assert(pET9CPLingInfo->pUdb);
    ET9Assert(wOffset < ET9_CP_UdbDataAreaBytes(pET9CPLingInfo->pUdb));

    pUdb = pET9CPLingInfo->pUdb;
    wCheckSum = pUdb->wDataCheck;
    eNewType = pDstObj->eType;
    /* get current freq */
    pbDst = ET9_CP_UdbData(pUdb) + wOffset;
    ET9_CP_GetEntryInfo(pUdb, pbDst, pDstObj, ET9_CP_GET_FREQ | ET9_CP_GET_TYPE);

    if (wInFreq) {
        wNewFreq = (ET9U16)__ET9Max(wInFreq, pDstObj->wFreq);
    }
    else {
        wNewFreq = (ET9U16)(pDstObj->wFreq + ET9_CP_FREQ_RISE);
    }
    if (wNewFreq > ET9_CP_MAXFREQ) {
        wNewFreq = ET9_CP_MAXFREQ; /* new freq is bound by ET9_CP_MAXFREQ */
    }

    /* write type */
    if (pDstObj->eType == ET9_CP_AUDBSTROKE && eNewType == ET9_CP_UDBSTROKE ||
             pDstObj->eType == ET9_CP_AUDBPHONETIC && eNewType == ET9_CP_UDBPHONETIC)
    {
        wCheckSum = (ET9U16)(wCheckSum - *pbDst);
        *pbDst = (ET9U8)(*pbDst & ~ET9_CP_AUDB_TYPE_MASK);
        wCheckSum = (ET9U16)(wCheckSum + *pbDst);
    }
    /* write new freq at the freq field of the entry */
    if (wNewFreq != pDstObj->wFreq) {
        pbDst = ET9_CP_MoveUdbPR(pUdb, pbDst, 1);
        ET9_CP_UdbWriteWord(pET9CPLingInfo, pbDst, wNewFreq);
        wCheckSum = (ET9U16)(wCheckSum
                            - ET9_CP_HIBYTE(pDstObj->wFreq) - ET9_CP_LOBYTE(pDstObj->wFreq)
                            + ET9_CP_HIBYTE(wNewFreq) + ET9_CP_LOBYTE(wNewFreq));
    }
    /* update checksum */
    if (wCheckSum != pUdb->wDataCheck) {
        ET9_CP_WriteUdbData(pET9CPLingInfo,
                          (void ET9FARDATA*)&(pUdb->wDataCheck),
                          (const void ET9FARDATA*)&wCheckSum,
                          sizeof(wCheckSum));
    }
    ET9Assert(ET9_CP_UdbChkIntegrity(pUdb, pUdb->wDataSize));
} /* ET9_CP_UdbRecordUsage() */

static void ET9LOCALCALL ET9_CP_AdjustZoneOffsets(ET9CPLingInfo *pET9CPLingInfo,
                                              ET9U8 ET9FARDATA * pbSrc,
                                              ET9U8 ET9FARDATA * pbDest,
                                              ET9U16 wBlockSize,
                                              ET9UINT *pnCheckSum)
{
    ET9CPUdbInfo ET9FARDATA *pUdb;
    ET9U8 ET9FARDATA * pbZone;
    ET9U8 ET9FARDATA * pbSrcEnd;
    ET9U8 ET9FARDATA * pbDestEnd;
    ET9U16 wMove, wNewOffset;
    ET9U8 b;

    pUdb = pET9CPLingInfo->pUdb;
    ET9Assert(pUdb);
    pbSrcEnd = ET9_CP_MoveUdbPR(pUdb, pbSrc, wBlockSize);
    pbDestEnd = ET9_CP_MoveUdbPR(pUdb, pbDest, wBlockSize);
    wMove = ET9_CP_UdbPointerDiff(pUdb, pbSrc, pbDest);
    for (b = 0; b < ET9_CP_UDBZONEMAX; b++) {
        pbZone = ET9_CP_UdbData(pUdb) + pUdb->wZoneOffset[b];
        wNewOffset = 0xFFFF;
        /* Src <= zone < SrcEnd --> zone += nMove */
        if (((pbSrc < pbSrcEnd) && (pbSrc <= pbZone) && (pbZone < pbSrcEnd)) ||
            ((pbSrc > pbSrcEnd) && ((pbSrc <= pbZone) || (pbZone < pbSrcEnd))))
        {
            wNewOffset = ET9_CP_UdbPtrToOffset(pUdb, ET9_CP_MoveUdbPR(pUdb, pbZone, wMove));
        }
        /* SrcEnd <= zone < DestEnd --> zone = DestEnd */
        /* Assumption: area from SrcEnd to DestEnd is free */
        else if (((pbSrcEnd < pbDestEnd) && (pbSrcEnd <= pbZone) && (pbZone < pbDestEnd)) ||
                 ((pbSrcEnd > pbDestEnd) && ((pbSrcEnd <= pbZone) || (pbZone < pbDestEnd))))
        {
            wNewOffset = ET9_CP_UdbPtrToOffset(pUdb, pbDestEnd);
        }
        if (0xFFFF != wNewOffset) {
            *pnCheckSum = *pnCheckSum - pUdb->wZoneOffset[b] + wNewOffset;
            ET9_CP_WriteUdbData(pET9CPLingInfo,
                              (void ET9FARDATA*)&(pUdb->wZoneOffset[b]),
                              (const void ET9FARDATA*)&wNewOffset,
                              sizeof(wNewOffset));
        }
    }
} /* ET9_CP_AdjustZoneOffsets() */

static void ET9LOCALCALL ET9_CP_MoveBlock(ET9CPLingInfo *pET9CPLingInfo,
                                      ET9U8 ET9FARDATA * pbSrc,
                                      ET9U8 ET9FARDATA * pbDest,
                                      ET9U16 wBlockSize,
                                      ET9UINT *pnCheckSum)
{
    ET9CPUdbInfo ET9FARDATA *pUdb;
    ET9U16 wDestLap, wSrcLap, wSizeRemained;

    ET9Assert(pET9CPLingInfo->pUdb);
    ET9Assert(wBlockSize);

    pUdb = pET9CPLingInfo->pUdb;
    wSrcLap = 0;
    wDestLap = 0;
    /* Adjust zone boundaries to reflect data movement */
    ET9_CP_AdjustZoneOffsets(pET9CPLingInfo, pbSrc, pbDest, wBlockSize, pnCheckSum);
    /* In place copy of the data block, including logic to handle wrap around.
     * Assume ET9FARDATA and ET9FARDATA have same width.  Rely on compiler
     * to enforce. */
    if ((pbDest + wBlockSize) > ET9_CP_UdbEnd(pUdb)) {
        wDestLap = (ET9U16)(pbDest + wBlockSize - ET9_CP_UdbEnd(pUdb));
    }
    if ((pbSrc + wBlockSize) > ET9_CP_UdbEnd(pUdb)) {
        wSrcLap = (ET9U16)(pbSrc + wBlockSize - ET9_CP_UdbEnd(pUdb));
    }
    wSizeRemained = wBlockSize;
    /* Move the end part where the src wraps around. */
    if (wSrcLap > 0) {
        ET9_CP_WriteUdbData(pET9CPLingInfo,
                          (void ET9FARDATA*)ET9_CP_MoveUdbPR(pUdb, pbDest, (ET9U16)(wSizeRemained - wSrcLap)),
                          (const void ET9FARDATA*)ET9_CP_MoveUdbPR(pUdb, pbSrc, (ET9U16)(wSizeRemained - wSrcLap)),
                          wSrcLap);
        wSizeRemained = (ET9U16)(wSizeRemained - wSrcLap);
    }
    /* Move the middle part where the dest wraps around. */
    if (wDestLap > 0) {
        /* if there is a destlap, its always greater than srclap */
        wDestLap = (ET9U16)(wDestLap - wSrcLap);
        ET9_CP_WriteUdbData(pET9CPLingInfo,
                          (void ET9FARDATA*)ET9_CP_MoveUdbPR(pUdb, pbDest, (ET9U16)(wSizeRemained - wDestLap)),
                          (const void ET9FARDATA*)ET9_CP_MoveUdbPR(pUdb, pbSrc, (ET9U16)(wSizeRemained - wDestLap)),
                          wDestLap);
        wSizeRemained = (ET9U16)(wSizeRemained - wDestLap);
    }
    /* Move the front part which is not wrapped around. */
    if (wSizeRemained > 0) {
        ET9_CP_WriteUdbData(pET9CPLingInfo,
                          (void ET9FARDATA*)pbDest,
                          (const void ET9FARDATA*)pbSrc,
                          wSizeRemained);
    }
} /* ET9_CP_MoveBlock() */

/*------------------------------------------------------------------------
 *  Function    : ET9_CP_MakeRoom
 *
 *  Synopsis    : Consolidate free entries into one that has the size needed
 *                and starts at a given offset.  Zone boundaries are shifted
 *                accordingly.  Also output the resulted checksum.
 *
 *
 *     Input:   : pET9CPLingInfo = pointer to Chinese XT9 LingInfo
 *                wTargetOffset = offset at which the
 *                nNeeded = bytes needed to consolidate
 *                *pnCheckSum = checksum before consolidation
 *
 *    Output    : *pnCheckSum = checksum after consolidation
 *
 *    Return    : none
 *-----------------------------------------------------------------------*/
#define ET9_CP_CONSOLIDATE_BLOCKS 10
static void ET9LOCALCALL ET9_CP_MakeRoom(ET9CPLingInfo *pET9CPLingInfo,
                                     ET9U16 wTargetOffset,
                                     ET9U16 wNeeded,
                                     ET9UINT *pnCheckSum)
{
    ET9CPUdbInfo ET9FARDATA *pUdb;
    ET9U8 ET9FARDATA * pbTarget;
    ET9U8 ET9FARDATA * pbCurrent;
    ET9U8 ET9FARDATA * pbSrc;
    ET9U8 ET9FARDATA * pbDest;
    ET9U8 ET9FARDATA * pbFreeBlock[ET9_CP_CONSOLIDATE_BLOCKS];
    ET9U16 wFreeBlockSize[ET9_CP_CONSOLIDATE_BLOCKS];
    ET9U16 wFree, wExtraFree, wExtraFreeOffset, wMove, wDataBlockSize;
    ET9U8 bFreeBlockCount, bHasData; /* indicate data entry since the last free entry */
    signed char b;
    ET9_CP_RUdbObj currentObj;

    ET9Assert(pET9CPLingInfo);
    ET9Assert(pET9CPLingInfo->pUdb);
    ET9Assert(wTargetOffset < ET9_CP_UdbDataAreaBytes(pET9CPLingInfo->pUdb));
    ET9Assert(wNeeded <= (ET9U16)ET9_CP_WordSizeToEntrySize(ET9CPMAXUDBPHRASESIZE));
#ifdef ET9_DEBUG
    currentObj.eType = ET9_CP_FREE; /* Avoid compiler warning */
#endif

    pUdb = pET9CPLingInfo->pUdb;
    pbTarget = ET9_CP_UdbData(pUdb) + wTargetOffset;
    /* loop until consolidate enough free space */
    for (wFree = 0, bFreeBlockCount = 0; wFree < wNeeded;) {
        ET9Assert(pbTarget);
        pbCurrent = pbTarget; /* start from target */
        for (b = 0; b < ET9_CP_CONSOLIDATE_BLOCKS; b++) {
            wFreeBlockSize[b] = 0;
        }
        /* Obsolete Note: can't put
           while ((bFreeBlockCount < ET9_CP_CONSOLIDATE_BLOCKS) || !bHasData)
         * here because optimization somehow screws up and break out of the
         * loop too early. */
        for (bFreeBlockCount = 0, bHasData = 1; (bFreeBlockCount < ET9_CP_CONSOLIDATE_BLOCKS) || !bHasData;) {
            ET9_CP_GetEntryInfo(pUdb, pbCurrent, &currentObj, ET9_CP_GET_TYPE | ET9_CP_GET_ENTRY_SIZE);
            /* non-free entry */
            if (ET9_CP_FREE != currentObj.eType) {
                /* Note: can't put
                   (bFreeBlockCount >= ET9_CP_CONSOLIDATE_BLOCKS)
                 * here because optimization somehow screws up and always
                 * break out of the loop. */
                if (bFreeBlockCount >= ET9_CP_CONSOLIDATE_BLOCKS) {
                    /* consolidate up to ET9_CP_CONSOLIDATE_BLOCKS free blocks */
                    break;
                }
                bHasData = 1;
            }
            /* free entry */
            else {
                if (bHasData) {
                    bFreeBlockCount++;
                    pbFreeBlock[bFreeBlockCount - 1] = pbCurrent;
                }
                wFreeBlockSize[bFreeBlockCount - 1] = (ET9U16)(wFreeBlockSize[bFreeBlockCount - 1] + currentObj.wEntrySize);
                wFree = (ET9U16)(wFree + currentObj.wEntrySize);
                bHasData = 0;
                /* Remove the current free entry from checksum as it will be
                 * overwritten after consolidation */
                *pnCheckSum -= ET9_CP_GetEntryInfo(pUdb, pbCurrent, &currentObj, ET9_CP_GET_CHECKSUM);
                if (wFree >= wNeeded) {
                    /* This free entry gives enough free space. */
                    wExtraFree = (ET9U16)(wFree - wNeeded);
                    if (wExtraFree) {
                        /* Consolidate up to the free space needed.  Write a
                         * single free entry to represent the extra free space.
                         * This may minimize data movement in the next Add. */
                        wFreeBlockSize[bFreeBlockCount - 1] = (ET9U16)(wFreeBlockSize[bFreeBlockCount - 1] - wExtraFree);
                        wExtraFreeOffset = (ET9U16)(ET9_CP_MoveUdbPR(pUdb, pbCurrent, (ET9U16)(currentObj.wEntrySize - wExtraFree)) - ET9_CP_UdbData(pUdb));
                        ET9Assert(ET9_CP_FREE == currentObj.eType);
                        currentObj.wEntrySize = wExtraFree;
                        *pnCheckSum += ET9_CP_WriteEntry(pET9CPLingInfo, &currentObj, wExtraFreeOffset);
                    }
                    break;
                }
            }
            pbCurrent = ET9_CP_MoveUdbPR(pUdb, pbCurrent, currentObj.wEntrySize);
        } /* END inner loop */

        /* Avoid compiler warning.  With only 1 free block and no data at target
         * should mean enough free space.  Shouldn't loop back.  So set it to
         * 0 here for assert to catch it at the start of the outer loop. */
        pbDest = 0;
        /* Consolidate data between the free blocks by moving the "right-most"
         * first towards pbEnd */
        ET9Assert(bFreeBlockCount);
        for (wMove = 0, b = (signed char)(bFreeBlockCount - 1); b >= 0; b--) {
            if (b > 0) {
                pbSrc = ET9_CP_MoveUdbPR(pUdb, pbFreeBlock[b - 1], wFreeBlockSize[b - 1]);
            }
            else {
                if (pbTarget != pbFreeBlock[0]) {
                    pbSrc = pbTarget;
                }
                else {
                    /* target is not data, adjust zone offsets inside FreeBlock[0] */
                    pbSrc = pbFreeBlock[0];
                    ET9_CP_AdjustZoneOffsets(pET9CPLingInfo, pbSrc,
                                           ET9_CP_MoveUdbPR(pUdb, pbSrc, wNeeded), 0,
                                           pnCheckSum);
                    break;
                }
            }
            wMove = (ET9U16)(wMove + wFreeBlockSize[b]);
            wDataBlockSize = ET9_CP_UdbPointerDiff(pUdb, pbSrc, pbFreeBlock[b]);
            pbDest = ET9_CP_MoveUdbPR(pUdb, pbSrc, wMove);
            ET9_CP_MoveBlock(pET9CPLingInfo, pbSrc, pbDest, wDataBlockSize, pnCheckSum);
        }
        /* update target to continue from the "left-most" data entry */
        pbTarget = pbDest;
    } /* END outer loop */

    /* Write a single free entry to represent the free space needed.
     * No need to include the checksum of this free entry becuase it'll be
     * overwritten by the new entry we're making room for. */
    ET9Assert(ET9_CP_FREE == currentObj.eType);
    currentObj.wEntrySize = wNeeded;
    ET9_CP_WriteEntry(pET9CPLingInfo, &currentObj, wTargetOffset);

} /* ET9_CP_MakeRoom() */

/* Add the obj into the UDB at the offset */
static void ET9LOCALCALL ET9_CP_UdbAddObj(ET9CPLingInfo *pET9CPLingInfo,
                                      ET9_CP_RUdbObj *pInputObj,
                                      ET9U16 wTargetOffset,
                                      ET9U8 bZone)
{
    ET9CPUdbInfo ET9FARDATA *pUdb;
    ET9UINT nCheckSum;

    ET9Assert(pET9CPLingInfo);
    ET9Assert(pET9CPLingInfo->pUdb);
    ET9Assert(pInputObj);
    ET9Assert(ET9_CP_FREE != pInputObj->eType);
    ET9Assert(pInputObj->wEntrySize > 0);
    ET9Assert(pInputObj->wEntrySize <= ET9_CP_WordSizeToEntrySize(ET9CPMAXUDBPHRASESIZE));
    ET9Assert(wTargetOffset < ET9_CP_UdbDataAreaBytes(pET9CPLingInfo->pUdb));

    pUdb = pET9CPLingInfo->pUdb;
    nCheckSum = pUdb->wDataCheck;
    if (ET9_CP_UdbOverCrowded(pUdb)) {
        ET9_CP_CleanUpUdb(pET9CPLingInfo, &nCheckSum);
    }
    /* Udb should have >= 5% free space, which is 400 bytes for a min size UDB.
     * 400 bytes should fit the max size entry of 67 bytes.
     * So if it doesn't fit, UDB size is wrong.  Return error at once */
    ET9Assert(pUdb->wFreeBytes > pInputObj->wEntrySize);

    /* The entry can fit but may not have a continuous block of free space.
     * Compact free space into one free entry staring at target offset. */
    {
        ET9U16 wZoneOffset;
        wZoneOffset = pUdb->wZoneOffset[bZone]; /* original zone offset */
        ET9_CP_MakeRoom(pET9CPLingInfo, wTargetOffset, pInputObj->wEntrySize, &nCheckSum);
        if ((wZoneOffset == wTargetOffset) && (wZoneOffset != pUdb->wZoneOffset[bZone])) {
            /* The boundary of the target zone may have moved
             * during consolidation if it was originally at the target offset,
             * need to move them back. */
            ET9U8 b;
            wZoneOffset = pUdb->wZoneOffset[bZone];
            nCheckSum = nCheckSum - wZoneOffset + wTargetOffset;
            ET9_CP_WriteUdbData(pET9CPLingInfo,
                              (void ET9FARDATA*)&(pUdb->wZoneOffset[bZone]),
                              (const void ET9FARDATA*)&wTargetOffset,
                              sizeof(wTargetOffset));

            /* The boundary of the previous zone(s) may have been moved
             * during consolidation if they were originally at the target
             * offset and are zero width. Need to move them back. */
            for (b = bZone; b != ET9_CP_NextZone(bZone); ) {
                b = (ET9U8)(b ? (b - 1) : (ET9_CP_UDBZONEMAX - 1));
                if ((pUdb->wZoneOffset[b] == wZoneOffset) &&
                    (pUdb->wZoneWordCount[b] == 0)) {

                    nCheckSum = nCheckSum - wZoneOffset + wTargetOffset;
                    ET9_CP_WriteUdbData(pET9CPLingInfo,
                                      (void ET9FARDATA*)&(pUdb->wZoneOffset[b]),
                                      (const void ET9FARDATA*)&wTargetOffset,
                                      sizeof(wTargetOffset));
                }
                else {
                    break;
                }
            }
        }
    }

    /* Write the new obj as an entry at target offset */
    nCheckSum += ET9_CP_WriteEntry(pET9CPLingInfo, pInputObj, wTargetOffset);

    /* Update UDB header fields */
    {
        ET9U16 wTemp;

        /* word count for the affected zone */
        wTemp = (ET9U16)(pUdb->wZoneWordCount[bZone] + 1);
        nCheckSum++;
        ET9_CP_WriteUdbData(pET9CPLingInfo,
                          (void ET9FARDATA*)&(pUdb->wZoneWordCount[bZone]),
                          (const void ET9FARDATA*)&wTemp,
                          sizeof(wTemp));

        /* free bytes */
        wTemp = (ET9U16)(pUdb->wFreeBytes - pInputObj->wEntrySize);
        nCheckSum -= pInputObj->wEntrySize;
        ET9_CP_WriteUdbData(pET9CPLingInfo,
                          (void ET9FARDATA*)&(pUdb->wFreeBytes),
                          (const void ET9FARDATA*)&wTemp,
                          sizeof(wTemp));

        /* checksum */
        wTemp = (ET9U16)nCheckSum;
        ET9_CP_WriteUdbData(pET9CPLingInfo,
                          (void ET9FARDATA*)&(pUdb->wDataCheck),
                          (const void ET9FARDATA*)&wTemp,
                          sizeof(wTemp));
    }
    ET9Assert(ET9_CP_UdbChkIntegrity(pUdb, pUdb->wDataSize));
} /* ET9_CP_UdbAddObj() */

/* Recale frequency has a lower bound of ET9_CP_MINFREQ */
static ET9U16 ET9LOCALCALL ET9_CP_RescaleFreq(ET9U16 wFreq)
{
    ET9U16 wNewFreq;
    wNewFreq = (ET9U16)(wFreq - (wFreq + ET9_CP_UDB_RESCALE_FACTOR - 1) / ET9_CP_UDB_RESCALE_FACTOR);
    return (ET9U16)((wNewFreq > ET9_CP_MINFREQ) ? wNewFreq : ET9_CP_MINFREQ);
} /* ET9_CP_RescaleFreq() */

static void ET9LOCALCALL ET9_CP_RescaleAllFreq(ET9CPLingInfo *pET9CPLingInfo)
{
    ET9CPUdbInfo ET9FARDATA *pUdb;
    ET9U8 ET9FARDATA * pbCurrentEntry;
    ET9U8 ET9FARDATA * pbFreqDest;
    ET9_CP_RUdbObj currentObj;
    ET9INT sBytesToCheck;
    ET9U16 wNewCutOffFreq;
    ET9U16 wNewFreq;
    ET9U16 wCheckSum;

    ET9Assert(pET9CPLingInfo);
    ET9Assert(pET9CPLingInfo->pUdb);

    /* init local variables */
    pUdb = pET9CPLingInfo->pUdb;
    wCheckSum = pUdb->wDataCheck;
    wNewCutOffFreq = ET9_CP_RescaleFreq(pUdb->wCutOffFreq);
    if (wNewCutOffFreq < ET9_CP_UDB_MIN_CUTOFF) {
        wNewCutOffFreq = ET9_CP_UDB_MIN_CUTOFF;
    }
    /* reset pointer and cutoff sizes before rescan */
    pbCurrentEntry = ET9_CP_UdbData(pUdb) + pUdb->wZoneOffset[0];
    sBytesToCheck = ET9_CP_UdbDataAreaBytes(pUdb);

    /* Step through all entries to rescale freq of each non-ET9_CP_FREE entry.
     * Meanwhile, record checksum changes */
    do {
        ET9_CP_GetEntryInfo(pUdb,
                          pbCurrentEntry,
                          &currentObj,
                          ET9_CP_GET_TYPE | ET9_CP_GET_ENTRY_SIZE | ET9_CP_GET_FREQ);

        if (ET9_CP_FREE != currentObj.eType) {
            wNewFreq = ET9_CP_RescaleFreq(currentObj.wFreq);
            wCheckSum = (ET9U16)(wCheckSum
                                - ET9_CP_HIBYTE(currentObj.wFreq)
                                - ET9_CP_LOBYTE(currentObj.wFreq)
                                + ET9_CP_HIBYTE(wNewFreq)
                                + ET9_CP_LOBYTE(wNewFreq));
            pbFreqDest = ET9_CP_MoveUdbPR(pUdb, pbCurrentEntry, 1);
            ET9_CP_UdbWriteWord(pET9CPLingInfo, pbFreqDest, wNewFreq);
        } /* END non-free */
        sBytesToCheck -= currentObj.wEntrySize;
        ET9Assert(sBytesToCheck >= 0);
        /* Next entry */
        pbCurrentEntry = ET9_CP_MoveUdbPR(pUdb, pbCurrentEntry, currentObj.wEntrySize);
    } while (sBytesToCheck);

    wCheckSum = (ET9U16)(wCheckSum - pUdb->wCutOffFreq + wNewCutOffFreq);
    ET9_CP_WriteUdbData(pET9CPLingInfo,
                      (void ET9FARDATA*)&(pUdb->wCutOffFreq),
                      (const void ET9FARDATA*)&wNewCutOffFreq,
                      sizeof(wNewCutOffFreq));

    /* update checksum */
    ET9_CP_WriteUdbData(pET9CPLingInfo,
                      (void ET9FARDATA*)&(pUdb->wDataCheck),
                      (const void ET9FARDATA*)&wCheckSum,
                      sizeof(wCheckSum));

    ET9Assert(ET9_CP_UdbChkIntegrity(pUdb, pUdb->wDataSize));
} /* ET9_CP_RescaleAllFreq() */

/* refresh update count and rescale all frequencies if needed */
static void ET9LOCALCALL ET9_CP_UdbRefreshUpdateCount(ET9CPLingInfo *pET9CPLingInfo,
                                                  ET9U8 bRecordUsage)
{
    ET9U16 wUpdateCount;

    ET9Assert(pET9CPLingInfo);
    ET9Assert(pET9CPLingInfo->pUdb);

    wUpdateCount = (ET9U16)(pET9CPLingInfo->pUdb->wUpdateCount + 1);

    /* Since rescale is a slow operation.  Only do it if
     * 1. update count reaches max and this call is just recording usage of
     *    an existing entry, which is a fast operation.
     * or
     * 2. update count reaches 2 * max, to prevent some words reaching
     *    the max frequency. */
    if ((bRecordUsage && (wUpdateCount >= ET9_CP_MAX_UPDATE_COUNT)) ||
        ((wUpdateCount / 2) >= ET9_CP_MAX_UPDATE_COUNT)) {

        ET9_CP_RescaleAllFreq(pET9CPLingInfo);
        wUpdateCount = 0; /* reset to 0 after rescale */
    }
    ET9_CP_WriteUdbData(pET9CPLingInfo,
                      (void ET9FARDATA*)&(pET9CPLingInfo->pUdb->wUpdateCount),
                      (const void ET9FARDATA*)&wUpdateCount,
                      sizeof(wUpdateCount));
} /* ET9_CP_UdbRefreshUpdateCount() */

/* Returns nonzero if any char in psPhrase (which is phonetic encoded) has a mute character */
static ET9UINT ET9LOCALCALL ET9_CP_HasMuteChar(ET9_CP_CommonInfo * psCommon, ET9CPPhrase *psPhrase) {
    ET9UINT n = psPhrase->bLen;

    while (n--) {
        if (ET9_CP_IS_MUTE_PID(psCommon, psPhrase->pSymbs[n])) {
            return 1;
        }
    }
    return 0;
}

/*------------------------------------------------------------------------
 *  Function    : ET9_CP_UdbUsePhrase
 *
 *  Synopsis    : Update the UDB in respond to the use of a phrase.
 *
 *     Input:   : pET9CPLingInfo = pointer to Chinese XT9 LingInfo
 *                pPhrase = pointer to the phrase that was used.
 *                eEncode = ID encoding (either PID or SID)
 *                wFreq = (> 0): use this freq if phrase exists and its freq is lower
 *                        0: to use default frequency.
 *                bIsAuto = 1: auto-created (RDB / AUDB type)
 *                          0: user-created (UDB type)
 *    Return    : none
 *-----------------------------------------------------------------------*/
void ET9FARCALL ET9_CP_UdbUsePhrase(
    ET9CPLingInfo *pET9CPLingInfo,
    ET9CPPhrase *pPhrase,
    ET9_CP_IDEncode eEncode,
    ET9U16 wFreq,
    ET9BOOL bIsAuto)
{
    ET9_CP_RUdbObj sObj;
    ET9U16 wOffset;
    ET9U8 bZone;
    ET9BOOL bFound, bIsLdbPhrase;

    ET9Assert(pET9CPLingInfo);
    ET9Assert(pPhrase);
    /* return immediately if word is too long, UDB doesn't exist, if PID is mute */
    if (!pET9CPLingInfo->pUdb ||
        pPhrase->bLen > ET9CPMAXUDBPHRASESIZE ||
        (ET9_CP_IDEncode_PID == eEncode || ET9_CP_IDEncode_BID == eEncode) &&
            ET9_CP_HasMuteChar(&pET9CPLingInfo->CommonInfo, pPhrase))
    {
        return;
    }
    bIsLdbPhrase = ET9_CP_FindPhraseInLdb(pET9CPLingInfo, eEncode, pPhrase);

    /* setup the obj */
    ET9_CP_UdbSetupObj(pPhrase->pSymbs, pPhrase->bLen,
                       bIsAuto, bIsLdbPhrase, wFreq, eEncode, &sObj);
    /* check if the obj exists */
    bFound = ET9_CP_UdbFindObj(pET9CPLingInfo, &sObj, eEncode, &bZone, &wOffset);
    /* found at offset, record usage */
    if (bFound) {
        ET9_CP_UdbRecordUsage(pET9CPLingInfo, &sObj, wFreq, wOffset);
    }
    /* not found, offset == insertion point */
    else {
        ET9_CP_UdbAddObj(pET9CPLingInfo, &sObj, wOffset, bZone);
        /* Increment dwDirtyCount */
        ET9_CP_WriterSetDirtyCount(pET9CPLingInfo);
    }
    /* increment update count and rescale if needed */
    ET9_CP_UdbRefreshUpdateCount(pET9CPLingInfo, bFound);
} /* ET9_CP_UdbUsePhrase() */

static void ET9LOCALCALL ET9_CP_UdbDeleteEntry(ET9CPLingInfo *pET9CPLingInfo,
                                ET9U8 ET9FARDATA * pbEntry,
                                ET9_CP_RUdbObj *pObj,
                                ET9U8 bZone)
{
    ET9CPUdbInfo ET9FARDATA *pUdb;
    ET9U16 wCheckSum;

    ET9Assert(pET9CPLingInfo);
    ET9Assert(pET9CPLingInfo->pUdb);
    /* init variables */
    pUdb = pET9CPLingInfo->pUdb;
    wCheckSum = pUdb->wDataCheck;
    /* Replace the entry with a ET9_CP_FREE entry of the same size */
    {
        ET9U16 wEntryCheckSum;
        wEntryCheckSum = ET9_CP_GetEntryInfo(pUdb, pbEntry, pObj, ET9_CP_GET_CHECKSUM);
        pObj->eType = ET9_CP_FREE;
        wCheckSum = (ET9U16)(wCheckSum - wEntryCheckSum +
                            ET9_CP_WriteEntry(pET9CPLingInfo, pObj,
                                            ET9_CP_UdbPtrToOffset(pUdb, pbEntry)));
    }
    /* Update affected UDB header fields */
    {
        ET9U16 wTmp;
        /* update word count for the given zone */
        wTmp = (ET9U16)(pUdb->wZoneWordCount[bZone] - 1);
        wCheckSum--;
        ET9_CP_WriteUdbData(pET9CPLingInfo,
                          (void ET9FARDATA*)&(pUdb->wZoneWordCount[bZone]),
                          (const void ET9FARDATA*)&wTmp,
                          sizeof(wTmp));
        /* update free bytes */
        wTmp = (ET9U16)(pUdb->wFreeBytes + pObj->wEntrySize);
        wCheckSum = (ET9U16)(wCheckSum + pObj->wEntrySize);
        ET9_CP_WriteUdbData(pET9CPLingInfo,
                          (void ET9FARDATA*)&(pUdb->wFreeBytes),
                          (const void ET9FARDATA*)&wTmp,
                          sizeof(wTmp));
        /* update checksum */
        ET9_CP_WriteUdbData(pET9CPLingInfo,
                          (void ET9FARDATA*)&(pUdb->wDataCheck),
                          (const void ET9FARDATA*)&wCheckSum,
                          sizeof(wCheckSum));
        /* Increment dwDirtyCount */
        ET9_CP_WriterSetDirtyCount(pET9CPLingInfo);
    }
} /* ET9_CP_UdbDeleteEntry() */

/*---------------------------------------------------------------------------
 *
 *   Function: ET9_CP_ComparePhrase
 *
 *   Synopsis: This function compares two phrases.
 *             If they match up to bLen, return 0;
 *             otherwise, return 1;
 *
 *     Input:  pwPhrase1  = phrase 1.
 *             pwPhrase2  = phrase 2.
 *             bLen       = length to match.
 *
 *     Return: 0 if the 2 phrases match up to bLen;
 *             1 otherwise.
 *
 *---------------------------------------------------------------------------*/
static ET9U8 ET9LOCALCALL ET9_CP_ComparePhrase(const ET9U16 *pwPhrase1,
                                          const ET9U16 *pwPhrase2,
                                          ET9U8 bLen)
{
    for (; bLen; bLen--) {
        if (*pwPhrase1++ != *pwPhrase2++) {
            return 1;
        }
    }
    return 0;
}   /* end of ET9_CP_ComparePhrase() */

/* Inputs : pwID = point to IDs after context prefix
 *          pbTones = array of tones for each syllable
 *          bIDCount = length of content in pwID
 *          bGetToneOption = 1: to get tone options, 0: otherwise
 *                           Note : caller set this to 1 for U_obj only
 *
 * Outputs : pbToneOptions = output valid tone options
 *           Note: caller should check if all tones are valid after
 *                 this function returns.  If so, caller is done.
 */
static ET9_CP_PhraseMatch ET9LOCALCALL ET9_CP_ObjIDMatch(ET9CPLingInfo  *pLing,
                                                         ET9U16         *pwID,
                                                         ET9BOOL        *pbLastSylPartial,
                                                         ET9U8          *pbTones,
                                                         ET9U8          *pbToneOptions,
                                                         ET9BOOL         bGetToneOption,
                                                         ET9U8           bIDCount)
{
    ET9_CP_PhraseMatch eMatch;
    ET9U16 *pwRange, wStartID, wPartialStartID, wEndID, wID;
    ET9U8 *pbRangeEnd;
    ET9UINT nSylCount, i, j;
    ET9U8 bToneFlags;

    ET9Assert(pLing && pwID && bIDCount > 0);
    ET9Assert(bIDCount >= pLing->CommonInfo.bSylCount);

    pwRange = pLing->CommonInfo.pwRange;
    pbRangeEnd = pLing->CommonInfo.pbRangeEnd;
    nSylCount = pLing->CommonInfo.bSylCount;

    eMatch = ET9_CP_EXACT_MATCH; /* assume exact match */
    *pbLastSylPartial = 0; /* assume not "only-last-syl-partial" */

    for (i = 0, j = 0; i < nSylCount; i++) { /* each syllable */
        wID = pwID[i];
        for (; j < pbRangeEnd[i]; j += ET9_CP_ID_RANGE_SIZE) { /* each mohu range */
            wStartID = pwRange[j];
            wPartialStartID = pwRange[j+1];
            wEndID = pwRange[j+2];
            if ( wID < wStartID || wID >= wEndID ) {
                continue; /* ID does not match this range */
            }
            if (pbTones && pbTones[i]) { /* tone filtering */
                if (wID >= wPartialStartID) {
                    continue; /* ID is not exact match in this range, try next range */
                }
                bToneFlags = ET9_CP_LookupTone(pLing, wID);
                if (!(bToneFlags & pbTones[i])) {
                    return ET9_CP_NO_MATCH; /* tone mismatch, no need to try next range */
                }
            }
            else if (bGetToneOption && i == nSylCount - 1 ) { /* get tone options */
                if (wID >= wPartialStartID) {
                    continue; /* ID is not exact match in this range, try next range */
                }
                bToneFlags = ET9_CP_LookupTone(pLing, wID);
                *pbToneOptions |= (ET9U8)(bToneFlags & ET9_CP_ALL_TONES_BIT_MASK);
            }
            else if (wID >= wPartialStartID) { /* partial match */
                if (i == nSylCount - 1 && ET9_CP_EXACT_MATCH == eMatch) { /* only-last-syl-partial */
                    *pbLastSylPartial = 1;
                }
                eMatch = ET9_CP_PARTIAL_SYL_MATCH;
            }
            break; /* match, no need to try other mohu ranges */
        } /* END each range for this syllable */
        if (j >= pbRangeEnd[i]) {
            return ET9_CP_NO_MATCH; /* this ID doesn't match any ranges of this syllable */
        }
        j = pbRangeEnd[i];
    } /* END loop syllables */

    if (bIDCount > nSylCount) { /* all syllables matched, but more IDs than syllables */
        eMatch = ET9_CP_PARTIAL_PHRASE_MATCH;
    }
    return eMatch;
} /* end ET9_CP_ObjIDMatch() */

/*----------------- Phonetic specific code --------------------*/

/* wRangeEnd is exclusive */
static void ET9LOCALCALL ET9_CP_UdbGetPhrasesInRange(
    ET9CPLingInfo       *pET9CPLingInfo,
    ET9BOOL              bNeedPartialSyl,
    ET9BOOL              bNeedPartialPhrase,
    const ET9U16        *pwPrefix,
    ET9U8                bPrefixLen,
    ET9_CP_SpellMatch   *pMatchType,
    ET9BOOL              bIsSID,
    ET9BOOL              bGetToneOption,
    ET9U8               *pbToneOptions,
    ET9U8               *pbTones,
    ET9U16              *pwOffset,
    ET9U16              *pwPhraseCount,
    ET9U16               wRangeEnd,
    ET9BOOL              bValidateCntxPrefix,
    ET9_CP_PhraseBuf    *pPhraseBuf)
{
    ET9CPUdbInfo ET9FARDATA *pUdb;
    ET9U8 ET9FARDATA * pbCurrent;
    ET9_CP_CommonInfo *pCommon = &(pET9CPLingInfo->CommonInfo);
    ET9_CP_PhraseMatch eMatch;
    ET9_CP_RUdbObj sObj;
    ET9U8 bCharCount, bWordSize;

    ET9Assert(!bGetToneOption || pbToneOptions);
    if (bGetToneOption && (ET9_CP_ALL_TONES_BIT_MASK == (*pbToneOptions & ET9_CP_ALL_TONES_BIT_MASK))) {
        return; /* all tones are valid, done */
    }
    pUdb = pET9CPLingInfo->pUdb;
    bCharCount = (ET9U8)(bValidateCntxPrefix ? 0 : pCommon->bSylCount);
    pbCurrent = ET9_CP_UdbOffsetToPtr(pUdb, *pwOffset);

    /* for each obj in the search range */
    for (sObj.wEntrySize = 0; *pwPhraseCount;) {
        ET9BOOL bLastSylPartial;

        pbCurrent = ET9_CP_MoveUdbPR(pUdb, pbCurrent, sObj.wEntrySize);
        ET9_CP_GetEntryInfo(pUdb, pbCurrent, &sObj, ET9_CP_GET_TYPE | ET9_CP_GET_ENTRY_SIZE);
        if (ET9_CP_FREE == sObj.eType) {
            continue; /* skip ET9_CP_FREE obj */
        }
        (*pwPhraseCount)--;
        if (pMatchType && ((ET9_CP_RDBPHONETIC == sObj.eType) || (ET9_CP_RDBSTROKE == sObj.eType))) {
            continue; /* validation, skip R_obj because no difference from LDB validation */
        }
        /* check size match */
        bWordSize = ET9_CP_EntrySizeToWordSize(sObj.wEntrySize);
        if (bWordSize < bPrefixLen + bCharCount) {
            continue;
        }
        else if (bWordSize == bPrefixLen) {
            if (!pMatchType || pbTones) {
                continue; /* not validating OR is validating expl input: need size > prefix */
            }
        }
        else { /* bWordSize > bPrefixLen */
            if (bValidateCntxPrefix) {
                continue; /* Validating context (prefix + suffix): need size exact match */
            }
            if (!bNeedPartialPhrase && bWordSize > bPrefixLen + bCharCount) {
                continue; /* Not needing partial match: need size exact match */
            }
        }

        ET9_CP_GetEntryInfo(pUdb, pbCurrent, &sObj, ET9_CP_GET_FREQ | ET9_CP_GET_ID);
        if (bPrefixLen && ET9_CP_ComparePhrase(pwPrefix, sObj.pwID, bPrefixLen)) {
            continue; /* context prefix mismatch */
        }

        if (bValidateCntxPrefix) {
            *pMatchType = eExactMatch; /* found context exact match as complete phrase: done */
            return;
        }
        if (bCharCount && (sObj.pwID[bPrefixLen] >= wRangeEnd)) {
            (*pwPhraseCount)++;
            break;
        }
        eMatch = ET9_CP_ObjIDMatch(pET9CPLingInfo, &(sObj.pwID[bPrefixLen]),
                                   &bLastSylPartial,
                                   pbTones, pbToneOptions, bGetToneOption,
                                   (ET9U8)(bWordSize - bPrefixLen) );
        if (ET9_CP_NO_MATCH == eMatch) {
            continue; /* mismatch */
        }
        if (ET9_CP_PARTIAL_SYL_MATCH == eMatch && !bNeedPartialSyl && !(bLastSylPartial && bNeedPartialPhrase) ) {
            continue; /* reject partial match if it is not needed */
        }
        if (pMatchType) {
            if (ET9_CP_EXACT_MATCH == eMatch) {
                *pMatchType = eExactMatch;
                return; /* found exact match: done */
            }
            else {
                *pMatchType = ePartialMatch; /* found partial match: continue */
            }
        }
        if (bGetToneOption) {
            if (ET9_CP_ALL_TONES_BIT_MASK == (*pbToneOptions & ET9_CP_ALL_TONES_BIT_MASK)) {
                return; /* all tones are valid, done */
            }
            continue; /* get tone options, no adding to phrase buf */
        }
#ifndef ET9CP_DISABLE_STROKE
        if (bIsSID && !ET9_CP_AllowComponent(pET9CPLingInfo)) { /* no componet allowed once a delimiter is added */
            if (ET9_CP_Is_Comp_Sid(pET9CPLingInfo, sObj.pwID[0]) ) {
                continue; /* skip components if component is disabled */
            }
        }
#endif
        if (!pMatchType) {
            ET9U8 bFreq = 0;
            ET9U16 wFreqEncoded;
            ET9_CP_Spell * pSpell = &pCommon->SpellData.sSpell;
            ET9UINT fSurpress;

            ET9Assert(0 == pSpell->bLen || ET9CPIsModePhonetic(pET9CPLingInfo));

            if (!bPrefixLen && (bWordSize == 1) && ET9CPIsNameInputActive(pET9CPLingInfo)) {
                bFreq = ET9_CP_GetNameCharFreq(pET9CPLingInfo, bIsSID, sObj.pwID[0]);
            }
            /* add obj to phrase buf */
            wFreqEncoded = ET9_CP_EncodeFreq(pET9CPLingInfo,
                                (ET9SYMB*)(sObj.pwID + bPrefixLen),
                                (ET9U8)(bWordSize - bPrefixLen),
                                sObj.wFreq,
                                (ET9U8)( (ET9_CP_EXACT_MATCH == eMatch) ? 1 : 0),
                                (ET9U8)((bPrefixLen ? 1 : 0) || bFreq),
                                1,
                                &fSurpress);
            if (!fSurpress) {
                ET9_CP_AddPhraseToBuf(pET9CPLingInfo,
                                     pPhraseBuf,
                                    (ET9SYMB*)(sObj.pwID + bPrefixLen),
                                    (ET9U8)(bWordSize - bPrefixLen), pSpell->pbChars, pSpell->bLen,
                                    bIsSID? ET9_CP_IDEncode_SID: ET9_CP_IDEncode_PID,
                                    wFreqEncoded);
            }
        }
    } /* end each obj */

    *pwOffset = ET9_CP_UdbPtrToOffset(pUdb, pbCurrent);
} /* end ET9_CP_UdbGetPhrasesInRange() */



/*---------------------------------------------------------------------------
 *
 *   Function: ET9_CP_GetUdbPhrases
 *
 *   Synopsis: This function searches the UDB to find any phrases that
 *             match the given context prefix and the current keys, which is
 *             in a form of ID ranges in pET9CPLingInfo. Then add the matching
 *             phrases into the phrase buffer.
 *
 *      Input: pET9CPLingInfo = Pointer to Chinese XT9 LingInfo structure.
 *             bNeedPartialSyl= 1 if need partial syllable match, 0 otherwise
 *             bNeedPartialPhrase = 1 if need partial phrase match, 0 otherwise
 *             pwPrefix       = the context prefix in PID/SID
 *             bPrefixLen     = the context prefix length.
 *             bIsSID         = 0 - ID is encoded in PID;
 *                              1 - ID is encoded in SID;
 *             bGetToneOption = 0 - do not get tone options and phrases are added
 *                                  into phrase buffer.
 *                              1 - get tone options and phrases are not added into
 *                                  phrase buffer.
 *             pbTones        = array of 32 Tones (32 bytes) in phonetic mode.
 *                              NULL if in stroke mode.
 *     In/Out: pMatchType     = if NULL, the function will search the UDB,
 *                              find matching phrases and add them into the phrase buffer.
 *                              else, *pMatchType will be set to the "best" match type among {eNoMatch, eExactMatch, ePartialMatch}
 *     Output: pbToneOptions  = if bGetToneOption is 1, it stores the retrieved tone options.
 *                              Its format is one byte and bit arrangement is: 000xxxxx.
 *                              Least significant bit is for tone 1.
 *                              2nd least significant bit is for tone 2, etc.
 *             pPhraseBuf     = pointer to the phrase buffer for storing result phrases
 *
 *     Return: none
 *
 *---------------------------------------------------------------------------*/
void ET9FARCALL ET9_CP_GetUdbPhrases(
    ET9CPLingInfo *pET9CPLingInfo,
    ET9BOOL bNeedPartialSyl,
    ET9BOOL bNeedPartialPhrase,
    const ET9U16 *pwPrefix,
    ET9U8 bPrefixLen,
    ET9_CP_SpellMatch *pMatchType,
    ET9BOOL bIsSID,
    ET9BOOL bGetToneOption, /* obsolete? (kwtodo) */
    ET9U8 *pbToneOptions, /* obsolete? */
    ET9U8 *pbTones,
    ET9BOOL bValidateCntxPrefix,
    ET9_CP_PhraseBuf *pPhraseBuf)
{
    ET9CPUdbInfo ET9FARDATA *pUdb;
    ET9_CP_CommonInfo *pCommon = &(pET9CPLingInfo->CommonInfo);
    ET9U16 wOffset, wWordCount;
    ET9_CP_IDEncode eEncode;

    if (pMatchType) {
        *pMatchType = eNoMatch; /* assume no match */
    }
    pUdb = pET9CPLingInfo->pUdb;
    if (!pUdb) {
        return; /* no Udb, do nothing */
    }
    ET9Assert(!bGetToneOption || pbToneOptions);
    if (bGetToneOption && (ET9_CP_ALL_TONES_BIT_MASK == (*pbToneOptions & ET9_CP_ALL_TONES_BIT_MASK))) {
        return; /* all tones are valid, done */
    }
    if (!pMatchType) { /* shortcuts */
        if (!(
            (ET9_CP_PhraseNeedUdb(pPhraseBuf)) ||
            (bPrefixLen && ET9_CP_PhraseNeedContext(pPhraseBuf)) ||
            (!bPrefixLen && ET9_CP_PhraseNeedNonContext(pPhraseBuf))
           ))
        {
            return;
        }
    }

    if (bIsSID) {
        eEncode = ET9_CP_IDEncode_SID;
    }
    else {
        eEncode = (ET9_CP_IDEncode)((ET9_CP_IsLdbPinyinOnly(pET9CPLingInfo))? ET9_CP_IDEncode_PID : ET9_CP_IDEncode_BID);
    }

    if (bPrefixLen) {
        ET9UINT nZone;

        /* Has context prefix, use first ID in prefix to find zone */
        ET9Assert(pwPrefix);
        nZone = ET9_CP_UdbZoneForID(pET9CPLingInfo, eEncode, pwPrefix[0]);
        wOffset = pUdb->wZoneOffset[nZone];
        wWordCount = pUdb->wZoneWordCount[nZone];

        ET9_CP_UdbGetPhrasesInRange(pET9CPLingInfo, bNeedPartialSyl, bNeedPartialPhrase, pwPrefix, bPrefixLen,
                                  pMatchType, bIsSID, bGetToneOption,
                                  pbToneOptions, pbTones, &wOffset, &wWordCount,
                                  0xFFFF, bValidateCntxPrefix, pPhraseBuf);
    }
#ifndef ET9CP_DISABLE_STROKE
    else if (bIsSID) { /* Stroke mode */
        ET9_CP_SPrivate *pPrivate = &(pET9CPLingInfo->Private.SPrivate);
        ET9UINT nZone, nEndZone;
        ET9U16 wRangeStart, wRangeEnd;
        wRangeStart = pCommon->pwRange[0];
        wRangeEnd = pCommon->pwRange[ET9_CP_PART_ID_RANGE_NUM - 1];
        nEndZone = ET9_CP_UdbZoneForID(pET9CPLingInfo, ET9_CP_IDEncode_SID, wRangeEnd);
        if (0xFFFF == pPrivate->wUdbLastRangeEnd) { /* first time search */
            nZone = ET9_CP_UDBZONEMAX; /* ensure restart */
        }
        else {
            nZone = ET9_CP_UdbZoneForID(pET9CPLingInfo, ET9_CP_IDEncode_SID, pPrivate->wUdbLastRangeEnd);
        }
        if ((wRangeStart >= pPrivate->wUdbLastRangeEnd) &&
            (pPrivate->wUdbLastWordCount || (nZone == nEndZone))) { /* not done with search area or done but new range doesn't expand search area */
            /* continue from last search */
            wOffset = pPrivate->wUdbLastOffset;
            wWordCount = pPrivate->wUdbLastWordCount;
            for (nZone++; nZone <= nEndZone; nZone++) { /* expand search area */
                wWordCount = (ET9U16)(wWordCount + pUdb->wZoneWordCount[nZone]);
            }
        }
        else { /* restart the search from beginning */
            nZone = ET9_CP_UdbZoneForID(pET9CPLingInfo, ET9_CP_IDEncode_SID, wRangeStart);
            ET9Assert(nZone <= nEndZone);
            /* new range covers more than 1 zone */
            wOffset = pUdb->wZoneOffset[nZone];
            for (wWordCount = 0; nZone <= nEndZone; nZone++) {
                wWordCount = (ET9U16)(wWordCount + pUdb->wZoneWordCount[nZone]);
            }
        }
        ET9_CP_UdbGetPhrasesInRange(pET9CPLingInfo, bNeedPartialSyl, bNeedPartialPhrase, pwPrefix, bPrefixLen,
                                  pMatchType, bIsSID, bGetToneOption,
                                  pbToneOptions, pbTones, &wOffset, &wWordCount,
                                  wRangeEnd, bValidateCntxPrefix, pPhraseBuf);

        pPrivate->wUdbLastRangeEnd = wRangeEnd;
        pPrivate->wUdbLastWordCount = wWordCount; /* update for next search */
        pPrivate->wUdbLastOffset = wOffset;
    }
#endif /* !ET9CP_DISABLE_STROKE */
    else { /* Phonetic mode, use each range's start of first syllable to determine zone */
        ET9U16 *pwRange;
        ET9UINT nRangeCount;
        ET9U8 bZone, bStartZone, bEndZone;

        bZone = ET9_CP_UDBZONEMAX;
        pwRange = pCommon->pwRange;
        nRangeCount = pCommon->pbRangeEnd[0];
        ET9Assert(pCommon->bSylCount > 0);
        for (; nRangeCount; pwRange += ET9_CP_ID_RANGE_SIZE, nRangeCount -= ET9_CP_ID_RANGE_SIZE) {
            ET9U16 wFirstID = *pwRange;
            ET9U16 wLastID = (ET9U16)(*(pwRange + ET9_CP_ID_RANGE_SIZE - 1) - 1); /* inclusive */
            bStartZone = ET9_CP_UdbZoneForID(pET9CPLingInfo, eEncode, wFirstID);
            bEndZone = ET9_CP_UdbZoneForID(pET9CPLingInfo, eEncode, wLastID); /* inclusive */
            if (bStartZone != bZone) {
                wOffset = pUdb->wZoneOffset[bStartZone];
                for (bZone = bStartZone, wWordCount = 0; bZone <= bEndZone; bZone++) {
                    wWordCount = (ET9U16)(wWordCount + pUdb->wZoneWordCount[bZone]);
                }
                bZone = bEndZone;
            } /* if bStartZone of this range == bEndZone of last range, continue (for Trad Pinyin, spell = "Y") */
            ET9_CP_UdbGetPhrasesInRange(pET9CPLingInfo, bNeedPartialSyl, bNeedPartialPhrase, pwPrefix, bPrefixLen,
                                      pMatchType, bIsSID, bGetToneOption,
                                      pbToneOptions, pbTones, &wOffset,
                                      &wWordCount, (ET9U16)(wLastID + 1),
                                      bValidateCntxPrefix, pPhraseBuf);
            if (pMatchType && eExactMatch == *pMatchType) {
                break; /* found exact match: done */
            }
        }
    }
} /* end ET9_CP_GetUdbPhrases() */

/* edianess */

/* Reverse endianess of a 16-bit value so that 0x1234 becomes 0x3412
 *
 * input : a 16-bit value
 * return : a 16-bit value with byte order reversed.
 */
static ET9U16 ET9LOCALCALL ET9_CP_ReverseEndianess16(ET9U16 wValue)
{
    return (ET9U16)(((wValue >> 8) & 0xFF) | (wValue << 8));
}

/* Reverse endianess of a 32-bit value so that 0x12345678 becomes 0x78563412
 *
 * input : a 32-bit value
 * return : a 32-bit value with byte order reversed.
 */
static ET9U32 ET9LOCALCALL ET9_CP_ReverseEndianess32(ET9U32 dwValue)
{
    ET9U16 wHiWord, wLoWord;
    wHiWord = ET9_CP_ReverseEndianess16((ET9U16)(dwValue >> 16));
    wLoWord = ET9_CP_ReverseEndianess16((ET9U16)dwValue);
    return (ET9U32)((((ET9U32)wLoWord) << 16) | wHiWord);
}

/* Switch endianess when detected endianess mismatch
 *
 * input : pUdb - pointer to the udb with mismatched endianess.
 * return : none
 */
static void ET9LOCALCALL ET9_CP_SwitchEndianess(ET9CPLingInfo *pET9CPLingInfo,
                                            ET9CPUdbInfo ET9FARDATA *pUdb)
{
    ET9U32 dw;
    ET9U16 w;
    ET9U8 i;
    w = ET9_CP_ReverseEndianess16(pUdb->wDataSize);
    ET9_CP_WriteUdbData(pET9CPLingInfo,
                      (void ET9FARDATA*)&(pUdb->wDataSize),
                      (const void ET9FARDATA*)&w,
                      sizeof(w));

    w = ET9_CP_ReverseEndianess16(pUdb->wDataCheck);
    ET9_CP_WriteUdbData(pET9CPLingInfo,
                      (void ET9FARDATA*)&(pUdb->wDataCheck),
                      (const void ET9FARDATA*)&w,
                      sizeof(w));

    dw = ET9_CP_ReverseEndianess32(pUdb->dwDirtyCount);
    ET9_CP_WriteUdbData(pET9CPLingInfo,
                      (void ET9FARDATA*)&(pUdb->dwDirtyCount),
                      (const void ET9FARDATA*)&dw,
                      sizeof(dw));

    w = ET9_CP_ReverseEndianess16(pUdb->wUpdateCount);
    ET9_CP_WriteUdbData(pET9CPLingInfo,
                      (void ET9FARDATA*)&(pUdb->wUpdateCount),
                      (const void ET9FARDATA*)&w,
                      sizeof(w));
    w = ET9_CP_ReverseEndianess16(pUdb->wFreeBytes);
    ET9_CP_WriteUdbData(pET9CPLingInfo,
                      (void ET9FARDATA*)&(pUdb->wFreeBytes),
                      (const void ET9FARDATA*)&w,
                      sizeof(w));

    w = ET9_CP_ReverseEndianess16(pUdb->wCutOffFreq);
    ET9_CP_WriteUdbData(pET9CPLingInfo,
                      (void ET9FARDATA*)&(pUdb->wCutOffFreq),
                      (const void ET9FARDATA*)&w,
                      sizeof(w));

    w = ET9_CP_ReverseEndianess16(pUdb->wLdbCompatibility);
    ET9_CP_WriteUdbData(pET9CPLingInfo,
                      (void ET9FARDATA*)&(pUdb->wLdbCompatibility),
                      (const void ET9FARDATA*)&w,
                      sizeof(w));

    for (i = 0; i < ET9_CP_UDBZONEMAX; i++) {
        w = ET9_CP_ReverseEndianess16(pUdb->wZoneOffset[i]);
        ET9_CP_WriteUdbData(pET9CPLingInfo,
                          (void ET9FARDATA*)&(pUdb->wZoneOffset[i]),
                          (const void ET9FARDATA*)&w,
                          sizeof(w));

        w = ET9_CP_ReverseEndianess16(pUdb->wZoneWordCount[i]);
        ET9_CP_WriteUdbData(pET9CPLingInfo,
                          (void ET9FARDATA*)&(pUdb->wZoneWordCount[i]),
                          (const void ET9FARDATA*)&w,
                          sizeof(w));
    }
} /* ET9_CP_SwitchEndianess() */
/* END endianess */

/*  LDB compatibility format in binary is:
    rrrr rrrr aaaa bccc
    r : reserved
    a : as character set. (used after b)
        same representation as in LDB.
          aaaa == 0: GB2312 or Big5LevelOne,
          aaaa == 1: GB13000 or Big5LevelOnePlus,
          aaaa == 2: GB18030 or Big5
          aaaa == 3: Big5-HK500
          aaaa == 4: Big5-HKSCS
    b : as 2nd LangID;  (must be used first)
        0 = Trad, 1 = Simp
    c : as module presence compatibility; (used after b and a)
        0 = with Pinyin only (w/ or w/o Stroke) (use PID wordlist)
        1 = with BPMF (w/ or w/o Stroke/Pinyin) (use BID wordlist)
 */
static ET9U16 ET9LOCALCALL ET9_CP_GetLdbCompatibility(ET9CPLingInfo *pET9CPLingInfo)
{
    ET9U32 dwReadOffset;
    ET9U16 wLdbCompat;

    dwReadOffset = ET9_CP_MODULE_CHARSET_OFFSET;
    wLdbCompat = (ET9U16)(ET9_CP_LdbReadByte(pET9CPLingInfo, dwReadOffset) & 0xF0);

    dwReadOffset = ET9_CP_LANGUAGE_ID_OFFSET;
    wLdbCompat = (ET9U16)(wLdbCompat | ((ET9_CP_LdbReadByte(pET9CPLingInfo, dwReadOffset) & 0x01) << 3)); /* ET9PLIDChineseTraditional: E0,  ET9PLIDChineseSimplified: E1,  ET9PLIDChineseHongkong: E2 */
    if (!ET9_CP_IsLdbPinyinOnly(pET9CPLingInfo)) {
        wLdbCompat++;
    }
    return wLdbCompat;
} /* ET9_CP_GetLdbCompatibility */

/* Description : Check if the UDB is compatible with the LDB.
 * Return      : ET9STATUS_NO_RUDB if has a UDB and the UDB is incompatible with LDB.
 */
ET9STATUS ET9FARCALL ET9_CP_CheckUdbCompat(ET9CPLingInfo *pET9CPLingInfo)
{
    ET9U16 wLdbCompat;
    wLdbCompat = ET9_CP_GetLdbCompatibility(pET9CPLingInfo);
    if (pET9CPLingInfo->pUdb &&
        (wLdbCompat != pET9CPLingInfo->pUdb->wLdbCompatibility)) {
        pET9CPLingInfo->pUdb = 0; /* detach the incompatible UDB */
        return ET9STATUS_NO_RUDB;
    }
    return ET9STATUS_NONE;
} /* ET9CP_CheckUdb */

/*
 * Function: ET9_CP_UdbFindDelUIDMatch
 * Synopsis: Find 1st or Delete all phrases that exactly match the given Unicode
 * Input:    psUIDPhrase = given Unicode
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
                                         ET9CPPhrase *psUIDPhrase,
                                         ET9CPPhrase *psIDPhrase,
                                         ET9_CP_IDEncode eEncode,
                                         ET9U16 *pwAltID,
                                         ET9U8 bAltCount)
{
    ET9CPUdbInfo ET9FARDATA *pUdb = pET9CPLingInfo->pUdb;
    ET9U8 ET9FARDATA * pbCurrent;
    ET9_CP_RUdbObj sObj;
    ET9U16 *pwObjID, wPhraseCount;
    ET9U8 b, bZone, bWordSize, fFound = 0;
    pwObjID = sObj.pwID;
    for (b = 0; b < bAltCount; b++) {
        bZone = ET9_CP_UdbZoneForID(pET9CPLingInfo, eEncode, pwAltID[b]);
        wPhraseCount = pUdb->wZoneWordCount[bZone];
        pbCurrent = ET9_CP_UdbOffsetToPtr(pUdb, pUdb->wZoneOffset[bZone]);
        for (sObj.wEntrySize = 0; wPhraseCount; ) {
            pbCurrent = ET9_CP_MoveUdbPR(pUdb, pbCurrent, sObj.wEntrySize);
            ET9_CP_GetEntryInfo(pUdb, pbCurrent, &sObj, ET9_CP_GET_TYPE | ET9_CP_GET_ENTRY_SIZE);
            if (ET9_CP_FREE == sObj.eType) {
                continue; /* skip free obj */
            }
            wPhraseCount--;
            bWordSize = ET9_CP_EntrySizeToWordSize(sObj.wEntrySize);
            if (bWordSize == psUIDPhrase->bLen) {
                ET9_CP_GetEntryInfo(pUdb, pbCurrent, &sObj, ET9_CP_GET_ID);
                if (pwAltID[b] == pwObjID[0]) {
                    ET9U16 wPID, *pwUID = psUIDPhrase->pSymbs + 1;
                    ET9U8 i;
                    for (i = 1; i < psUIDPhrase->bLen; i++) {
                        /* convert obj ID to PID if needed */
                        if (ET9_CP_IDEncode_SID == eEncode) {
                            ET9U8 bAltPIDCount;
                            bAltPIDCount = ET9_CP_LookupID(pET9CPLingInfo, &wPID,
                                                          pwObjID[i], 1,
                                                          ET9_CP_Lookup_SIDToPID);
                            ET9Assert(bAltPIDCount);
                        }
                        else {
                            wPID = pwObjID[i];
                        }
                        /* PID to Unicode and then match with given Unicode */
                        if (*pwUID++ != ET9_CP_LookupUnicode(pET9CPLingInfo, wPID)) {
                            break;
                        }
                    }
                    if (i == psUIDPhrase->bLen) { /* exact match obj */
                        if (psIDPhrase) { /* output obj's ID and return */
                            for (i = 0; i < psUIDPhrase->bLen; i++) {
                                psIDPhrase->pSymbs[i] = *pwObjID++;
                            }
                            psIDPhrase->bLen = i;
                            return 1;
                        }
                        else { /* delete this entry and continue */
                            ET9_CP_UdbDeleteEntry(pET9CPLingInfo, pbCurrent, &sObj, bZone);
                            fFound = 1;
                        }
                    }
                }
            } /* END match phrase length */
        } /* END for each obj in current zone */
    } /* END for each alt ID of 1st Unicode */
    return fFound;
} /* END ET9_CP_UdbFindDelUIDMatch() */

/* -------- API functions -------- */



/** Activates RUDB operations.
 *
 * By default, RUDB operations are not activated. The integration layer calls this function to activate RUDB operations.<br>
 * Before calling it, the integration layer must allocate a block of RAM as the RUDB and pass it to Chinese XT9 by calling this function.
 * Recommended RUDB size is 8 KB to 10 KB.
 *
 * @param pET9CPLingInfo     pointer to Chinese information structure.
 * @param pfUdbWrite         (optional)Callback function for writing data to RUDB if direct write is not desired.
 * @param pUdb               a block of RAM allocated by the integration layer to store RUDB data.
 * @param wDataSize          size of pUdb in bytes.
 *
 * @return ET9STATUS_NONE         Succeeded
 * @return ET9STATUS_NO_INIT      pET9CPLingInfo is not properly initialized
 * @return ET9STATUS_BAD_PARAM    pUdb == NULL
 * @return ET9STATUS_INVALID_SIZE wDataSize is too small or high byte and low byte of wDataSize are the same
 * @return ET9STATUS_NO_RUDB      The new RUDB could not be activated because it was not compatible with the current LDB.
 *                                If the function returns NO_RUDB the integration layer should attach a RUDB compatible with the LDB.
 *
 */
ET9STATUS ET9FARCALL ET9CPUdbActivate(ET9CPLingInfo                    *pET9CPLingInfo,
                                      ET9CPDBWRITECALLBACK              pfUdbWrite,
                                      ET9CPUdbInfo ET9FARDATA           *pUdb,
                                      ET9U16                            wDataSize)
{
    ET9STATUS eStatus = ET9STATUS_NONE;
    /* validate inputs */
    ET9_CP_CHECK_LINGINFO(pET9CPLingInfo);
    /* If no UDB, return error without processing */
    if (!pUdb) {
        return ET9STATUS_BAD_PARAM;
    }
    /* Must have at least UDB minimum size.
     * The size cannot have upper and lower bytes identical (for endianess check) */
    if ((wDataSize < ET9CPUDBMINSIZE) ||
        ((wDataSize & 0xFF) == ((wDataSize >> 8) & 0xFF))
        ) {
        return ET9STATUS_INVALID_SIZE;
    }
    pET9CPLingInfo->pUdb = pUdb;
    pET9CPLingInfo->pfUdbWrite = pfUdbWrite;
    /* Switch endianess of the UDB header if detected endianess mismatch */
    if (pUdb->wDataSize == ET9_CP_ReverseEndianess16(wDataSize)) {
        ET9_CP_SwitchEndianess(pET9CPLingInfo, pUdb);
    }
    /* Set up an empty UDB if it is not initialized or corrupted.
     * UpdateCount doesn't affect UDB integrity */
    if ((pUdb->wDataSize != wDataSize) ||
        (pUdb->wFreeBytes >= ET9_CP_UdbDataAreaBytes(pUdb)) ||
        (0 == ET9_CP_CountWords(pUdb)) ||
        (0 == ET9_CP_UdbChkIntegrity(pUdb, wDataSize))) {

        ET9U32 dwDirtyCount = 0;

        /* Set data size. */
        ET9_CP_WriteUdbData(pET9CPLingInfo,
                          (void ET9FARDATA*)&(pUdb->wDataSize),
                          (const void ET9FARDATA*)&wDataSize,
                          sizeof(wDataSize));
        /* Zero dirtycount. */
        ET9_CP_WriteUdbData(pET9CPLingInfo,
                          (void ET9FARDATA*)&(pUdb->dwDirtyCount),
                          (const void ET9FARDATA*)&dwDirtyCount,
                          sizeof(dwDirtyCount));

        /* Reset UDB */
        eStatus = ET9CPUdbReset(pET9CPLingInfo);
    }
    /* If the UDB doesn't need a reset, chk if it is compatible with LDB */
    else if (ET9STATUS_NO_RUDB == ET9_CP_CheckUdbCompat(pET9CPLingInfo)) {
        pET9CPLingInfo->pUdb = 0; /* detach the incompatible UDB */
        eStatus = ET9STATUS_NO_RUDB; /* notify caller the UDB is detached */
    }
    ET9_CP_INVALIDATE_BUILD_CLEAR_CACHE(pET9CPLingInfo);
    /* invalidate cache for build selection list */
    pET9CPLingInfo->UdbReadCache.dwDirtyCount = pUdb->dwDirtyCount - 1;
    /* invalidate cache for Udb get phrase */
    pET9CPLingInfo->UdbReadCache.dwLastGetDC = pUdb->dwDirtyCount - 1;

    return eStatus;
} /* ET9CPUdbActivate() */

/** Resets (empties) the RUDB, that is, all RUDB entries are removed.
 *
 *  The integration layer should not call this function when the WordSymbInfo is not empty.
 *
 * @param pET9CPLingInfo     pointer to chinese information structure. *
 *
 * @return ET9STATUS_NONE               Succeeded
 * @return ET9STATUS_NO_INIT            pET9CPLingInfo is not properly initialized
 * @return ET9STATUS_NO_RUDB            No Udb attached to pET9CPLingInfo
 */
ET9STATUS ET9FARCALL ET9CPUdbReset(ET9CPLingInfo *pET9CPLingInfo)
{
    ET9CPUdbInfo ET9FARDATA *pUdb;
    /* validate inputs */
    ET9_CP_CHECK_LINGINFO(pET9CPLingInfo);
    /* If no UDB, return error without processing */
    pUdb = pET9CPLingInfo->pUdb;
    if (!pUdb) {
        return ET9STATUS_NO_RUDB;
    }
    /* wFreeBytes = size of data area */
    {
        ET9U16 wFreeBytes = (ET9U16)(ET9_CP_UdbDataAreaBytes(pUdb));
        ET9_CP_WriteUdbData(pET9CPLingInfo,
                          (void ET9FARDATA*)&(pUdb->wFreeBytes),
                          (const void ET9FARDATA*)&wFreeBytes,
                          sizeof(wFreeBytes));
    }
    /* wCutOffFreq = RDB word init freq */
    {
        ET9U16 wCutOffFreq = ET9_CP_RDB_INITFREQ;
        ET9_CP_WriteUdbData(pET9CPLingInfo,
                          (void ET9FARDATA*)&(pUdb->wCutOffFreq),
                          (const void ET9FARDATA*)&wCutOffFreq,
                          sizeof(wCutOffFreq));
    }
    /* dwDirtyCount++ */
    {
        ET9_CP_WriterSetDirtyCount(pET9CPLingInfo);
    }
    /* wUpdateCount = 0 */
    {
        ET9U16 wUpdateCount = 0;
        ET9_CP_WriteUdbData(pET9CPLingInfo,
                          (void ET9FARDATA*)&(pUdb->wUpdateCount),
                          (const void ET9FARDATA*)&wUpdateCount,
                          sizeof(wUpdateCount));
    }
    /* Reset zones to start at 0, distribute evenly, with only free space */
    {
        ET9U16 wWordCount, wOffset, wSize, wStdSize;
        ET9_CP_RUdbObj freeObj;
        ET9U8 b;
        ET9U8 bPhoneticZones = (ET9U8)(ET9_CP_IsLdbPinyinOnly(pET9CPLingInfo)?ET9_CP_UDBPINYINZONES:ET9_CP_UDBBPMFZONES);
        /* These items are constant through out the loop */
        freeObj.eType = ET9_CP_FREE;
        wStdSize = (ET9U16)(ET9_CP_UdbDataAreaBytes(pUdb) / (bPhoneticZones + ET9_CP_UDBSTROKEZONES));
        wWordCount = 0;
        for (b = 0, wOffset = 0; b < ET9_CP_UDBZONEMAX; b++) {
            /* write word count */
            ET9_CP_WriteUdbData(pET9CPLingInfo,
                              (void ET9FARDATA*)&(pUdb->wZoneWordCount[b]),
                              (const void ET9FARDATA*)&wWordCount,
                              sizeof(wWordCount));
            /* write offset */
            ET9_CP_WriteUdbData(pET9CPLingInfo,
                              (void ET9FARDATA*)&(pUdb->wZoneOffset[b]),
                              (const void ET9FARDATA*)&wOffset,
                              sizeof(wOffset));
            /* write a free entry for this zone */
            if (ET9_CP_IsLdbPinyinOnly(pET9CPLingInfo) &&  /* assign empty space for non-existing letter zone */
                (ET9_CP_UDBPINYINZONES <= b) && (b < ET9_CP_UDBPHONETICZONES)) {
                wSize = 0;
            }
            else if (b < ET9_CP_UDBZONEMAX - 1) {
                wSize = wStdSize;
            }
            else { /* last zone takes all space left */
                wSize = (ET9U16)(ET9_CP_UdbDataAreaBytes(pUdb) - wOffset);
            }
            freeObj.wEntrySize = wSize;
            ET9_CP_WriteEntry(pET9CPLingInfo, &freeObj, wOffset);
            /* next zone's offset */
            wOffset = (ET9U16)(wOffset + wSize);
        }
        ET9Assert(wOffset == ET9_CP_UdbDataAreaBytes(pUdb));
    }
    /* Reset checksum */
    {
        ET9U16 wCheckSum;
        wCheckSum = ET9_CP_GetUdbCheckSum(pUdb);
        ET9_CP_WriteUdbData(pET9CPLingInfo,
                          (void ET9FARDATA*)&(pUdb->wDataCheck),
                          (const void ET9FARDATA*)&wCheckSum,
                          sizeof(wCheckSum));
    }

    /* Make the UDB compatible with the LDB */
    {
        ET9U16 wLdbCompatibility;
        wLdbCompatibility = ET9_CP_GetLdbCompatibility(pET9CPLingInfo);
        ET9_CP_WriteUdbData(pET9CPLingInfo,
                          (void ET9FARDATA*)&(pUdb->wLdbCompatibility),
                          (const void ET9FARDATA*)&wLdbCompatibility,
                          sizeof(wLdbCompatibility));
    }

    return ET9STATUS_NONE;
} /* ET9CPUdbReset() */

/** Deletes all RUDB entries that matches the given Unicode phrase.
 *
 *
 *
 * @param pET9CPLingInfo     pointer to chinese information structure.
 * @param psPhrase           the phrase to be deleted (in Unicode).
 *
 * @return ET9STATUS_NONE           Succeeded.
 * @return ET9STATUS_NO_INIT        pET9CPLingInfo is not properly initialized
 * @return ET9STATUS_NO_RUDB        No Udb attached to pET9CPLingInfo .
 * @return ET9STATUS_BAD_PARAM      No phrase is specified or specified phrase is invalid
 * @return ET9STATUS_WORD_NOT_FOUND Specified phrase is not in Udb
 */
ET9STATUS ET9FARCALL ET9CPUdbDeletePhrase(ET9CPLingInfo *pET9CPLingInfo,
                                          ET9CPPhrase *psPhrase)
{
    ET9_CP_CommonInfo *pCommon;
    ET9_CP_IDEncode eEncode;
    ET9UINT fFound, fCharHasSpell;
    ET9U16 pwAltID[ET9_CP_MAX_ALT_SYLLABLE];
    ET9U8 bAltCount;

    /* validate inputs */
    ET9_CP_CHECK_LINGINFO(pET9CPLingInfo);
    if (!pET9CPLingInfo->pUdb) {
        return ET9STATUS_NO_RUDB;
    }
    if (!psPhrase || !psPhrase->bLen) {
        return ET9STATUS_BAD_PARAM;
    }
    pCommon = &(pET9CPLingInfo->CommonInfo);
    { /* find all alt PID of the 1st Unicode to help determine search areas */
        for (bAltCount = 0; bAltCount < ET9_CP_MAX_ALT_SYLLABLE; bAltCount++) {
            pwAltID[bAltCount] = ET9_CP_UnicodeToPID(pET9CPLingInfo, psPhrase->pSymbs[0], bAltCount);
            if (ET9_CP_NOMATCH == pwAltID[bAltCount]) {
                break;
            }
        }
        if (0 == bAltCount) {
            return ET9STATUS_BAD_PARAM; /* unknown Unicode */
        }
        fCharHasSpell = ET9_CP_IS_NORMAL_PID(pCommon, pwAltID[0]); /* this Unicode has spelling info */
    }
    fFound = 0;
    /* Search Phonetic area (only if the 1st Unicode has spelling info) */
    if (fCharHasSpell && ET9_CP_LdbHasPhonetic(pET9CPLingInfo)) {
        if (ET9_CP_LdbHasBpmf(pET9CPLingInfo)) {
            eEncode = ET9_CP_IDEncode_BID;
        }
        else {
            ET9Assert(ET9_CP_LdbHasPinyin(pET9CPLingInfo));
            eEncode = ET9_CP_IDEncode_PID;
        }
        fFound = ET9_CP_UdbFindDelUIDMatch(pET9CPLingInfo, psPhrase, 0, eEncode,
                                         pwAltID, bAltCount);
    }
#ifndef ET9CP_DISABLE_STROKE
    /* Search Stroke area */
    if (ET9_CP_LdbHasStroke(pET9CPLingInfo)) {
        eEncode = ET9_CP_IDEncode_SID;
        /* find all alt SID of the 1st Unicode, each points to a Stroke zone */
        bAltCount = ET9_CP_LookupID(pET9CPLingInfo, pwAltID, pwAltID[0],
                                  ET9_CP_MAX_ALT_SYLLABLE,
                                  ET9_CP_Lookup_PIDToSID);
        if (bAltCount) {
            fFound = ET9_CP_UdbFindDelUIDMatch(pET9CPLingInfo, psPhrase, 0, eEncode, pwAltID, bAltCount)
                     || fFound; /* must call the fnc before || fFound */
        }
    }
#endif
    if (fFound) {
        return ET9STATUS_NONE;
    }
    else {
        return ET9STATUS_WORD_NOT_FOUND;
    }
} /* ET9CPUdbDeletePhrase() */

/* starting from wStartOffset, skip x entries of the given type, then return a
 * pointer to the next entry of that type. */
static ET9U8 ET9FARDATA * ET9LOCALCALL ET9_CP_UdbSkipEntries(ET9CPUdbInfo ET9FARDATA * pUdb, /* udb to search */
                                               ET9U16 wStartOffset,            /* search start offset */
                                               ET9U16 wEndOffset,              /* search end offset */
                                               ET9U16 wToSkip,                 /* how many entries to skip */
                                               ET9UINT nTargetTypeMask,          /* types to search for */
                                               ET9_CP_RUdbObj *psObj)                 /* return len and type */
{
    ET9U8 ET9FARDATA *pbStart = ET9_CP_UdbOffsetToPtr(pUdb, wStartOffset);
    ET9U8 ET9FARDATA *pbEnd = ET9_CP_UdbOffsetToPtr(pUdb, wEndOffset);
    ET9U8 ET9FARDATA *pbCurrent = pbStart;

    /* skip udb entries until the target index */
    for (;;)
    {
        ET9_CP_GetEntryInfo(pUdb, pbCurrent, psObj, ET9_CP_GET_TYPE | ET9_CP_GET_ENTRY_SIZE);

        /* only consider target type */
        if ( ( (1 << psObj->eType) & nTargetTypeMask ) && 0 == wToSkip--) {
            break;
        }

        /* move to next entry */
        pbCurrent = ET9_CP_MoveUdbPR(pUdb, pbCurrent, psObj->wEntrySize);

        /* are we out of bounds of search? */
        if (pbCurrent == pbEnd)
        {
            return (ET9U8 ET9FARDATA *) (NULL);
        }
    }

    /* get target entry's IDs */
    ET9_CP_GetEntryInfo(pUdb, pbCurrent, psObj, ET9_CP_GET_ID);

    return pbCurrent;
}

/** Gets the phrase from UDB by index according to the desired type and the current input mode.
 *
 * @param pET9CPLingInfo     pointer to chinese information structure.
 * @param wTypeMask          bit mask to indicate which types of phrases to include.
 * @param wIndex             0-based index of the desired phrase.
 * @param pPhrase            structure to get the phrase (in Unicode).
 * @param pSpell             (optional)structure to get the spelling of the phrase. Supported in Phonetic modes only.
 *
 * @return ET9STATUS_NONE               Succeeded
 * @return ET9STATUS_NO_INIT            pET9CPLingInfo is not properly initialized
 * @return ET9STATUS_NO_RUDB            No Udb attached to pET9CPLingInfo .
 * @return ET9STATUS_BAD_PARAM          pPhrase is NULL
 * @return ET9STATUS_OUT_OF_RANGE       Specified index too big
 * @return ET9STATUS_DB_CHANGED_WARNING Warning for UDB changed (previously retrieved words are potentially out of date)
 *
 */
ET9STATUS ET9FARCALL ET9CPUdbGetPhrase(ET9CPLingInfo *pET9CPLingInfo,
                                       ET9U16 wTypeMask,
                                       ET9U16 wIndex,
                                       ET9CPPhrase *pPhrase,
                                       ET9CPSpell *pSpell)
{
    ET9_CP_RUdbObj sObj;

    ET9CPUdbInfo ET9FARDATA * pUdb;
    ET9U8 ET9FARDATA *pbEntry;

    ET9U16 wStartOffset, wCacheOffset, wEndOffset;
    ET9U16 wToSkip = wIndex;

    ET9STATUS status = ET9STATUS_NONE;

    ET9UINT nTargetTypeMask; /* types to search for */
    ET9_CP_IDEncode eEnc;
    ET9U8 b;

    /* parameter checks */
    ET9_CP_CHECK_LINGINFO(pET9CPLingInfo);
    pUdb = pET9CPLingInfo->pUdb; /* abbreviate */

    if (!pUdb) {
        return ET9STATUS_NO_RUDB;
    }
    if (!pPhrase) {
        return ET9STATUS_BAD_PARAM;
    }
    if (0 == (wTypeMask & ET9CPUdbPhraseType_ALL_MASK)) {
        return ET9STATUS_BAD_PARAM; /* none of the types is desired */
    }

    if (pET9CPLingInfo->UdbReadCache.dwLastGetDC != pUdb->dwDirtyCount && 0 != wIndex) {
        status = ET9STATUS_DB_CHANGED_WARNING; /* just a warning, continue execution */
    }

    nTargetTypeMask = 0;
    /* for phonetic or stroke: validate params, encoding save, set zone range */
    if (ET9CPIsModeStroke(pET9CPLingInfo) || ET9CPIsModeCangJie(pET9CPLingInfo) || ET9CPIsModeQuickCangJie(pET9CPLingInfo)) {

        if (wTypeMask & ET9CPUdbPhraseType_User_MASK) {
            nTargetTypeMask |= 1 << ET9_CP_UDBSTROKE;
        }
        if (wTypeMask & ET9CPUdbPhraseType_Auto_MASK) {
            nTargetTypeMask |= 1 << ET9_CP_AUDBSTROKE;
        }

        eEnc = ET9_CP_IDEncode_SID;
        wStartOffset = pUdb->wZoneOffset[ET9_CP_UDBPHONETICZONES];
        wEndOffset = pUdb->wZoneOffset[0];
    }
    else {
        ET9Assert(ET9CPIsModeBpmf(pET9CPLingInfo) || ET9CPIsModePinyin(pET9CPLingInfo));

        if (wTypeMask & ET9CPUdbPhraseType_User_MASK) {
            nTargetTypeMask |= 1 << ET9_CP_UDBPHONETIC;
        }
        if (wTypeMask & ET9CPUdbPhraseType_Auto_MASK) {
            nTargetTypeMask |= 1 << ET9_CP_AUDBPHONETIC;
        }

        eEnc = ET9_CP_IDEncode_PID;
        wStartOffset = pUdb->wZoneOffset[0];
        wEndOffset = pUdb->wZoneOffset[ET9_CP_UDBPHONETICZONES];
    }

    /* use cache if its parameters matches current ones */
    wCacheOffset = pET9CPLingInfo->UdbReadCache.wLastGetOffset;
    if (pET9CPLingInfo->UdbReadCache.dwLastGetDC == pUdb->dwDirtyCount && /* cache is valid */
        (ET9U16)nTargetTypeMask == pET9CPLingInfo->UdbReadCache.wLastTypeMask &&  /* type mask matches cache */
        wIndex > pET9CPLingInfo->UdbReadCache.wLastGetIndex)  /* can only use cache for higher indexes */
    {
        wToSkip = (ET9U16)(wIndex - pET9CPLingInfo->UdbReadCache.wLastGetIndex);
        wStartOffset = wCacheOffset;
    }

    /* skip until the entry of the desired type */
    pbEntry = ET9_CP_UdbSkipEntries(pUdb, wStartOffset, wEndOffset, wToSkip, nTargetTypeMask, &sObj);

    if (!pbEntry) {
        return ET9STATUS_OUT_OF_RANGE;
    }

    /* Update cache */
    pET9CPLingInfo->UdbReadCache.wLastTypeMask = (ET9U16)nTargetTypeMask;
    pET9CPLingInfo->UdbReadCache.wLastGetIndex = wIndex;
    pET9CPLingInfo->UdbReadCache.wLastGetOffset = ET9_CP_UdbPtrToOffset(pUdb, pbEntry);
    pET9CPLingInfo->UdbReadCache.dwLastGetDC = pUdb->dwDirtyCount;

    /* copy to phrase */
    pPhrase->bLen = ET9_CP_EntrySizeToWordSize(sObj.wEntrySize);
    for (b = 0; b < pPhrase->bLen; b++) {
        pPhrase->pSymbs[b] = sObj.pwID[b];
    }

    /* output spelling */
    if (pSpell) {
        if (ET9_CP_IDEncode_SID == eEnc) {
            pSpell->bLen = 0;
        }
        else {
            ET9_CP_Spell internalSI;
            ET9_CP_PidBidToSpelling(pET9CPLingInfo, pPhrase->pSymbs, pPhrase->bLen, &internalSI);
            ET9_CP_ToExternalSpellInfo(pET9CPLingInfo, &internalSI, pSpell);
        }
    }

    /* convert to unicode */
    ET9_CP_ConvertPhraseToUnicode(pET9CPLingInfo, pPhrase, eEnc);

    return status;
} /* end ET9CPUdbGetPhrase */

/** Get the phrase count of the desired type from UDB for the current input mode.
 *
 *  This function is useful when developing UI to handle UDB phrase listing or deletion.
 *
 * @param pET9CPLingInfo     pointer to chinese information structure.
 * @param wTypeMask          bit mask to indicate which types of phrases to include.
 * @param pwCount            for getting the count.
 *
 *
 * @return ET9STATUS_NONE               Succeeded
 * @return ET9STATUS_NO_INIT            pET9CPLingInfo is not properly initialized
 * @return ET9STATUS_NO_RUDB            No Udb attached to pET9CPLingInfo .
 * @return ET9STATUS_BAD_PARAM          pwCount is NULL or none of types is desired.
 */
ET9STATUS ET9FARCALL ET9CPUdbGetPhraseCount(ET9CPLingInfo *pET9CPLingInfo,
                                            ET9U16 wTypeMask,
                                            ET9U16 *pwCount)
{
    ET9U16 wCount;
    ET9_CP_RUdbObj sObj;
    ET9BOOL bIsPhonetic;

    ET9CPUdbInfo ET9FARDATA * pUdb;
    ET9U8 ET9FARDATA *pbStart, *pbEnd, *pbCurrent;

    /* parameter checks */
    ET9_CP_CHECK_LINGINFO(pET9CPLingInfo);
    pUdb = pET9CPLingInfo->pUdb; /* abbreviate */

    if (!pUdb) {
        return ET9STATUS_NO_RUDB;
    }
    if (0 == (wTypeMask & ET9CPUdbPhraseType_ALL_MASK)) {
        return ET9STATUS_BAD_PARAM; /* none of the types is desired */
    }
    if (!pwCount) {
        return ET9STATUS_BAD_PARAM;
    }

    /* for phonetic or stroke: validate params, encoding save, set zone range */
    if (ET9CPIsModeStroke(pET9CPLingInfo) || ET9CPIsModeCangJie(pET9CPLingInfo) || ET9CPIsModeQuickCangJie(pET9CPLingInfo)) {

        bIsPhonetic = 0;
        pbStart = ET9_CP_UdbOffsetToPtr(pUdb, pUdb->wZoneOffset[ET9_CP_UDBPHONETICZONES]);
        pbEnd = ET9_CP_UdbOffsetToPtr(pUdb, pUdb->wZoneOffset[0]);
    }
    else {
        ET9Assert(ET9CPIsModeBpmf(pET9CPLingInfo) || ET9CPIsModePinyin(pET9CPLingInfo));

        bIsPhonetic = 1;
        pbStart = ET9_CP_UdbOffsetToPtr(pUdb, pUdb->wZoneOffset[0]);
        pbEnd = ET9_CP_UdbOffsetToPtr(pUdb, pUdb->wZoneOffset[ET9_CP_UDBPHONETICZONES]);
    }

    /* count entries */
    wCount = 0;
    for (pbCurrent = pbStart; pbCurrent != pbEnd; )
    {
        ET9_CP_GetEntryInfo(pUdb, pbCurrent, &sObj, ET9_CP_GET_TYPE | ET9_CP_GET_ENTRY_SIZE);

        /* we shouldn't jump udb sections */
        ET9Assert(bIsPhonetic && sObj.eType != ET9_CP_UDBSTROKE && sObj.eType != ET9_CP_AUDBSTROKE && sObj.eType != ET9_CP_RDBSTROKE ||
                  !bIsPhonetic && sObj.eType != ET9_CP_UDBPHONETIC && sObj.eType != ET9_CP_AUDBPHONETIC && sObj.eType != ET9_CP_RDBPHONETIC);

        /* only count the desired types */
        if (bIsPhonetic) {
            if ( (wTypeMask & ET9CPUdbPhraseType_User_MASK) && ET9_CP_UDBPHONETIC == sObj.eType
                || (wTypeMask & ET9CPUdbPhraseType_Auto_MASK) && ET9_CP_AUDBPHONETIC == sObj.eType) {
                wCount++;
            }
        }
        else {
            if ( (wTypeMask & ET9CPUdbPhraseType_User_MASK) && ET9_CP_UDBSTROKE == sObj.eType
                || (wTypeMask & ET9CPUdbPhraseType_Auto_MASK) && ET9_CP_AUDBSTROKE == sObj.eType) {
                wCount++;
            }
        }

        /* move to next entry */
        pbCurrent = ET9_CP_MoveUdbPR(pUdb, pbCurrent, sObj.wEntrySize);
    }

    *pwCount = wCount;

    return ET9STATUS_NONE;
} /* end ET9CPUdbGetPhraseCount */

#if (defined(WIN32_PLATFORM_WFSP) || defined(WIN32_PLATFORM_PSPC)) && !defined(ET9_DEBUG)
/* restores original optimization options (windows mobile) */
#pragma optimize ("", on)
#endif
/* ----------------------------------< eof >--------------------------------- */
