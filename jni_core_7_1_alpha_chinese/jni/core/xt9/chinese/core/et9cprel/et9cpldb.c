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
;**     FileName: et9cpldb.c                                                  **
;**                                                                           **
;**  Description: Chinese Phrase Text Input ldb database module.              **
;**               Conforming to the development version of Chinese XT9.       **
;**                                                                           **
;*******************************************************************************
;******************************************************************************/

#include "et9cpapi.h"
#include "et9cpsys.h"
#include "et9cpkey.h"
#include "et9cpspel.h"
#include "et9cpinit.h"
#include "et9cppbuf.h"
#include "et9cpmisc.h"
#include "et9cpldb.h"
#include "et9cpname.h"
#include "et9cpwdls.h"
#include "et9cpcntx.h"
#include "et9cptone.h"
#include "et9misc.h"

const ET9U8 ET9_CP_Bpmf_Letter_To_Pinyin[ET9CPBpmfLetterCount] =
{
    'b','p','m','f',
    'd','t','n','l',
    'g','k','h',
    'j','q','x',
    'z','c','s','r',
    'z','c','s',
    'a','o','e',0, /* 0 means cannot start a syl */
    'a','e','a','o',
    'a','e','a','e',
    'e','y','w','y'
};

#ifdef ET9_DIRECT_LDB_ACCESS
#ifdef _WIN32
#pragma message ("*** USING DIRECT LDB ACCESS (FAST) ***")
#endif
#endif


/*---------------------------------------------------------------------------*/
/** \internal
 * Read LDB data.
 * Read a given number of individual bytes from LDB.
 *
 * @param pLingInfo                 Pointer to Chinese information structure.
 * @param dwOffset                  Offset into LDB.
 * @param dwBytesToRead             Number of bytes to read.
 * @param ppbDst                    pointer to address of destination buffer
 *
 */
#ifdef ET9_DIRECT_LDB_ACCESS
ET9INLINE static void ET9LOCALCALL ET9_CP_ReadLdbData(ET9CPLingInfo * const        pLingInfo,
                                                      const ET9U32                 dwOffset,
                                                      const ET9U32                 dwBytesToRead,
                                                      const ET9U8 ET9FARDATA **    ppbDst)
{
    ET9Assert(pLingInfo);
    ET9Assert(ppbDst);
    ET9Assert(dwBytesToRead);
    ET9Assert(dwOffset + dwBytesToRead <= pLingInfo->Private.dwLdbDataSize);
    *ppbDst = &(pLingInfo->Private.pLdbData[dwOffset]);
}
#else
/** \internal
 * Read LDB data.
 * Read a given number of individual bytes from LDB.
 *
 * @param pLingInfo                 Pointer to Chinese information structure.
 * @param dwOffset                  Offset into LDB.
 * @param dwBytesToRead             Number of bytes to read.
 * @param pbDst                     pointer to destination buffer
 *
 * @return ET9STATUS_NONE on success, otherwise ET9STATUS_READ_DB_FAIL.
 */
ET9INLINE static ET9STATUS ET9LOCALCALL ET9_CP_ReadLdbData(ET9CPLingInfo * const    pLingInfo,
                                                           const ET9U32             dwOffset,
                                                           const ET9U32             dwBytesToRead,
                                                           ET9U8         * const    pbDst)
{
    ET9U32  dwBytesRead;

    ET9Assert(pLingInfo);
    ET9Assert(pbDst);
    ET9Assert(dwBytesToRead);
    ET9Assert(pLingInfo->pLdbReadData != NULL);

    if ( (ET9STATUS_NONE != pLingInfo->pLdbReadData(pLingInfo, dwOffset, dwBytesToRead, pbDst, &dwBytesRead) )
        || (dwBytesRead != dwBytesToRead) )
    {
        return ET9STATUS_READ_DB_FAIL;
    }
    return ET9STATUS_NONE;
}
#endif /* NOT ET9_DIRECT_LDB_ACCESS */

#ifdef ET9_DIRECT_LDB_ACCESS
ET9STATUS ET9FARCALL ET9_CP_ReadLdbMultiBytes(ET9CPLingInfo * const     pLingInfo,
                                              ET9U32                    dwOffset,
                                              ET9U32                    dwBytesToRead,
                                              const ET9U8 ET9FARDATA ** ppbDst)
{
    ET9_CP_ReadLdbData(pLingInfo, dwOffset, dwBytesToRead, ppbDst);
    return ET9STATUS_NONE;
}
#else
ET9STATUS ET9FARCALL ET9_CP_ReadLdbMultiBytes(ET9CPLingInfo * const     pLingInfo,
                                              ET9U32                    dwOffset,
                                              ET9U32                    dwBytesToRead,
                                              ET9U8         * const     pbDst)
{
    ET9STATUS status = ET9_CP_ReadLdbData(pLingInfo, dwOffset, dwBytesToRead, pbDst);
    ET9Assert(ET9STATUS_NONE == status);
    return status;
}
#endif /* NOT ET9_DIRECT_LDB_ACCESS */

/*---------------------------------------------------------------------------*/
/** \internal
 * Read LDB byte.
 * Read 1 byte.
 *
 * @param pLingInfo                 Pointer to Chinese information structure.
 * @param dwOffset                  Offset into LDB.
 *
 * @return the byte value.
 */

ET9U8 ET9FARCALL ET9_CP_LdbReadByte(ET9CPLingInfo * const pLingInfo,
                                    ET9U32                dwOffset)
{
#ifdef ET9_DIRECT_LDB_ACCESS
    const ET9U8 ET9FARDATA *pbData;
    ET9Assert(pLingInfo);
    ET9_CP_ReadLdbData(pLingInfo, dwOffset, 1, &pbData);
    return *pbData;
#else
    ET9U8 bData;
    ET9Assert(pLingInfo);
    if (ET9STATUS_NONE != ET9_CP_ReadLdbData(pLingInfo, dwOffset, 1, &bData)) {
        return 0;
    }
    return bData;
#endif
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Read a 2 byte number.
 *
 * @param pLingInfo                 Pointer to Chinese information structure.
 * @param dwOffset                  Offset into LDB.
 *
 * @return the word value.
 */

ET9U16 ET9FARCALL ET9_CP_LdbReadWord(ET9CPLingInfo * const pLingInfo,
                                     ET9U32                dwOffset)
{
    ET9U16 wValue;
#ifdef ET9_DIRECT_LDB_ACCESS
    const ET9U8 ET9FARDATA *pbData;
    ET9Assert(pLingInfo);
    ET9_CP_ReadLdbData(pLingInfo, dwOffset, 2, &pbData);
    /* assume the WORD in LDB is stored in big-endian order */
    wValue = (ET9U16)(((ET9U16)pbData[0] << 8) | pbData[1]);
#else
    ET9U8 pbData[2];
    ET9Assert(pLingInfo);
    if (ET9STATUS_NONE == ET9_CP_ReadLdbData(pLingInfo, dwOffset, 2, pbData) ) {
        /* assume the WORD in LDB is stored in big-endian order */
        wValue = (ET9U16)(((ET9U16)pbData[0] << 8) | pbData[1]);
    }
    else {
        /* read failed */
        wValue = 0;
    }
#endif
    return wValue;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Read a 4 byte number.
 *
 * @param pLingInfo                 Pointer to Chinese information structure.
 * @param dwOffset                  Offset into LDB.
 *
 * @return the dword value.
 */

ET9U32 ET9FARCALL ET9_CP_LdbReadDWord(ET9CPLingInfo * const pLingInfo,
                                      ET9U32                dwOffset)
{
    ET9U32 dwValue;
#ifdef ET9_DIRECT_LDB_ACCESS
    const ET9U8 ET9FARDATA *pbData;
    ET9Assert(pLingInfo);
    ET9_CP_ReadLdbData(pLingInfo, dwOffset, 4, &pbData);
    /* assume the DWORD in LDB is stored in big-endian order */
    dwValue = (ET9U32)(((ET9U32)pbData[0] << 24) | ((ET9U32)pbData[1] << 16) | ((ET9U32)pbData[2] << 8) | pbData[3]);
#else
    ET9U8 pbData[4];
    ET9Assert(pLingInfo);
    if (ET9STATUS_NONE == ET9_CP_ReadLdbData(pLingInfo, dwOffset, 4, pbData)) {
        /* assume the DWORD in LDB is stored in big-endian order */
        dwValue = (ET9U32)(((ET9U32)pbData[0] << 24) | ((ET9U32)pbData[1] << 16) | ((ET9U32)pbData[2] << 8) | pbData[3]);
    }
    else {
        /* read failed */
        dwValue = 0;
    }
#endif
    return dwValue;
}

#ifdef ET9_DIRECT_LDB_ACCESS
ET9STATUS ET9FARCALL ET9_CP_InitDirectLdbAccess(ET9CPLingInfo * const pLingInfo)
{
    /* call the callback function once to setup the pointer to the whole LDB data memory and data size */
    return pLingInfo->pLdbReadData(pLingInfo, &(pLingInfo->Private.pLdbData), &(pLingInfo->Private.dwLdbDataSize));
}
#endif

/*---------------------------------------------------------------------------*/
/** \internal
 * Hash a memory chunk.
 * Return the checksum of a piece of the LDB.
 *
 * @param pLingInfo                 Pointer to Chinese information structure.
 * @param pdwHashValue              In/out value with the hash to be updated.
 * @param dwPos                     Byte position of start of data to read.
 * @param dwSize                    # of bytes to read (can be over big for "read to end").
 *
 * @return None
 */
#define ET9_CP_LDBHASHBUFSIZE          0x100                           /**< \internal  buffer size for checksum calculations */

static void ET9LOCALCALL ET9_CP_LdbHashChunk(ET9CPLingInfo * const pLingInfo,
                                             ET9U32        * const pdwHashValue,
                                             ET9U32                dwPos,
                                             ET9U32                dwSize)
{
#ifdef ET9_DIRECT_LDB_ACCESS

    ET9U8               *pStr;
    ET9U32              dwHashValue;

    ET9Assert(pLingInfo);
    ET9Assert(pdwHashValue);
    ET9Assert(dwSize);
    ET9Assert(pLingInfo->Private.dwLdbDataSize);
    ET9Assert(pLingInfo->Private.pLdbData != NULL);

    if (dwSize >= pLingInfo->Private.dwLdbDataSize - dwPos) {
        dwSize = pLingInfo->Private.dwLdbDataSize - dwPos;
    }

    /* hash data chunk */

    pStr = &(pLingInfo->Private.pLdbData[dwPos]);

    dwHashValue = *pdwHashValue;

    while (dwSize--) {
        dwHashValue = *(pStr++) + (65599 * dwHashValue);
    }

    *pdwHashValue = dwHashValue;

#else /* ET9_DIRECT_LDB_ACCESS */

    ET9STATUS   eStatus;
    ET9U8       *pStr;
    ET9U32      dwNum;
    ET9U32      dwHashValue;
    ET9U32      dwReadSize;
    ET9U32      dwNumberOfBytesRead;
    ET9U8       byHashLDBBuff[ET9_CP_LDBHASHBUFSIZE];

    ET9Assert(pLingInfo);
    ET9Assert(pdwHashValue);
    ET9Assert(dwSize);

    while (dwSize) {

        if (ET9_CP_LDBHASHBUFSIZE > dwSize) {
            dwReadSize = dwSize;
        }
        else {
            dwReadSize = ET9_CP_LDBHASHBUFSIZE;
        }

        eStatus = pLingInfo->pLdbReadData(pLingInfo, dwPos, dwReadSize, byHashLDBBuff, &dwNumberOfBytesRead);

        /* Check for end-of-file (or other failure). */

        if (ET9STATUS_NONE != eStatus || 0 == dwNumberOfBytesRead) {
            break;
        }

        /* Hash LDB data */

        pStr = byHashLDBBuff;
        dwNum = dwNumberOfBytesRead;
        dwHashValue = *pdwHashValue;
        while (dwNum--) {
            dwHashValue = *(pStr++) + (65599 * dwHashValue);
        }
        *pdwHashValue = dwHashValue;

        dwSize -= dwNumberOfBytesRead;
        dwPos += dwNumberOfBytesRead;
    }

#endif /* ET9_DIRECT_LDB_ACCESS */
}


/*---------------------------------------------------------------------------
 *
 *   Function: ET9_CP_ValidateLDB
 *
 *   Synopsis: Validate LDB Data for Alpha-family LDBs
 *
 *      Input: pLingInfo = Pointer to linguistic information structure.
 *
 *     Return: ET9STATUS_NONE            - On success
 *             ET9STATUS_CORRUPT_DB      - On failure
 *             ET9STATUS_READ_DB_FAIL    - Unable to read LDB
 *
 *---------------------------------------------------------------------------*/
static ET9STATUS ET9LOCALCALL ET9_CP_ValidateLDB(ET9CPLingInfo *const pLingInfo)
{
    ET9U16 wCheckSum = 0;
    ET9U32 dwHashValue = 0;
    ET9U32 dwReadOffset;

#ifdef ET9_DIRECT_LDB_ACCESS
    {
        ET9STATUS eStatus;
        eStatus = ET9_CP_InitDirectLdbAccess(pLingInfo);
        if (ET9STATUS_NONE != eStatus) {
            return eStatus;
        }
    }
#endif
    /* Get the integrity ID from LDB */
    dwReadOffset = ET9_CP_CHECKSUM_OFFSET;
    wCheckSum = ET9_CP_LdbReadWord(pLingInfo, dwReadOffset);
    if (wCheckSum == 0) {             /* could not read database */
        return ET9STATUS_READ_DB_FAIL;
    }

    /* skip the value of the checksum when computing the checksum */
    ET9_CP_LdbHashChunk(pLingInfo, &dwHashValue, 0, ET9_CP_CHECKSUM_OFFSET); /* from start to checksum offset */
    ET9_CP_LdbHashChunk(pLingInfo, &dwHashValue, ET9_CP_CHECKSUM_OFFSET + 2, (ET9U32)(-1)); /* from after checksum to end of data */

    if (wCheckSum == (ET9U16)dwHashValue) {
        return ET9STATUS_NONE;
    }
    else {
        return ET9STATUS_CORRUPT_DB;
    }
} /* end of ET9_CP_ValidateLDB */


/** Validates the integrity of an XT9 Chinese LDB.
 *
 * @return  ET9STATUS_NONE              Succeeded
 * @return  ET9STATUS_NO_INIT           pET9CPLingInfo is not properly initialized
 * @return  ET9STATUS_LDB_ID_ERROR      LDB number is not supported
 * @return  ET9STATUS_BAD_PARAM         callback function is invalid
 * @return  ET9STATUS_READ_DB_FAIL      Failed to read LDB
 * @return  ET9STATUS_CORRUPT_DB        LDB is corrupted
 *
*/
ET9STATUS ET9FARCALL ET9CPLdbValidate(ET9CPLingInfo *pET9CPLingInfo,                  /**< pointer to Chinese linguistic information structure */
                                      const ET9U16  wLdbNum,                          /**< LDB number, it must be ET9PLIDChineseTraditional OR  ET9PLIDChineseSimplified OR ET9PLIDChineseHongkong */
                                      const ET9CPDBREADCALLBACK ET9CPLdbReadData)     /**< callback function used to read data from the LDB  */
{
    ET9STATUS eStatus;
    ET9CPDBREADCALLBACK pfSavedCallBack;
    ET9U16 wSavedLdbNum;

    /* validate inputs */
    if (!pET9CPLingInfo ||
        pET9CPLingInfo->Private.wInfoInitOK != ET9GOODSETUP ) {

        return ET9STATUS_NO_INIT;
    }
    if ((ET9PLIDChineseSimplified != (ET9U16)(wLdbNum & ET9PLIDMASK)) &&
        (ET9PLIDChineseTraditional != (ET9U16)(wLdbNum & ET9PLIDMASK)) && (ET9PLIDChineseHongkong != (ET9U16)(wLdbNum & ET9PLIDMASK)) ) {
        return ET9STATUS_LDB_ID_ERROR;    /* LDB not supported */
    }
    if (NULL == ET9CPLdbReadData) {
        return ET9STATUS_BAD_PARAM;
    }

    /* save Ldb num and callback function pointer */
    pfSavedCallBack = pET9CPLingInfo->pLdbReadData;
    wSavedLdbNum = pET9CPLingInfo->wLdbNum;

    /* use the given Ldb num and callback function */
    pET9CPLingInfo->wLdbNum = wLdbNum;
    pET9CPLingInfo->pLdbReadData = ET9CPLdbReadData;
    eStatus = ET9_CP_ValidateLDB(pET9CPLingInfo);

    /* restore saved Ldb num and callback function pointer */
    pET9CPLingInfo->pLdbReadData = pfSavedCallBack;
    pET9CPLingInfo->wLdbNum = wSavedLdbNum;

    if (pfSavedCallBack) {
#ifdef ET9_DIRECT_LDB_ACCESS
    /* restore the direct access pointer to the original Ldb and its data size */
        ET9_CP_InitDirectLdbAccess(pET9CPLingInfo);
#endif
    }

    return eStatus;
} /* end of ET9CPValidateLdb() */


#define ET9_CP_LDB_VERSION_STRING_LEN 0x20
#if ET9MAXVERSIONSTR < ET9_CP_LDB_VERSION_STRING_LEN
#error ET9MAXVERSIONSTR must be >= 32
#endif

/*---------------------------------------------------------------------------
 *
 *   Function: ET9_CP_ReadLdbVersion
 *
 *   Synopsis: Reads the version info from the database
 *
 *      Input: pLingInfo = Pointer to linguistic information structure.
 *             sBuff = structure for version information
 *
 *     Return: ET9STATUS_NONE
 *
 *---------------------------------------------------------------------------*/
ET9STATUS ET9FARCALL ET9_CP_ReadLdbVersion(ET9CPLingInfo * const pLingInfo, ET9_CP_VersionInfo *sBuff)
{
    ET9U32 dwReadOffset;

    /* validate the LDB */
    dwReadOffset = ET9_CP_SYMBOL_BYTE_OFFSET;
    if (0 == ET9_CP_LdbReadByte(pLingInfo, dwReadOffset)) {
        return ET9STATUS_NO_INIT;
    }
    /* skip the copyright */

    /* Save layout version */
    dwReadOffset = ET9_CP_LAYOUT_VER_OFFSET;
    sBuff->bLDBLayoutVer = ET9_CP_LdbReadByte(pLingInfo, dwReadOffset);

    /* Save Database type */
    dwReadOffset = ET9_CP_DB_TYPE_OFFSET;
    sBuff->bLDBType = ET9_CP_LdbReadByte(pLingInfo, dwReadOffset);

    /* Save Contents Major Version */
    dwReadOffset = ET9_CP_CONTENT_VER_OFFSET;
    sBuff->bContMajVer = ET9_CP_LdbReadByte(pLingInfo, dwReadOffset++);
    /* Save Contents Minor Version */
    sBuff->bContMinVer = ET9_CP_LdbReadByte(pLingInfo, dwReadOffset++);
    /* Save Contents Deviation */
    sBuff->bContDev = ET9_CP_LdbReadByte(pLingInfo, dwReadOffset);

    /* Save Module and Char Set */
    dwReadOffset = ET9_CP_MODULE_CHARSET_OFFSET;
    sBuff->bModuleCharSet = ET9_CP_LdbReadByte(pLingInfo, dwReadOffset);

    /* Save Primary language ID */
    dwReadOffset = ET9_CP_LANGUAGE_ID_OFFSET;
    sBuff->bPrimID = ET9_CP_LdbReadByte(pLingInfo, dwReadOffset++);
    /* Save Secondary language ID */
    sBuff->bSecID = ET9_CP_LdbReadByte(pLingInfo, dwReadOffset);

    /* Save Symbol Class */
    dwReadOffset = ET9_CP_SYMBOL_BYTE_OFFSET + 1;
    sBuff->bSymClass = ET9_CP_LdbReadByte(pLingInfo, dwReadOffset);

    return ET9STATUS_NONE;
}

/*------------------------------------------------------------------------
 *
 *  Function    : ET9_CP_PinyinSyllableToBIN
 *
 *  Synopsis    : It converts the Pinyin syllable to the encoded BIN.
 *                (exclude the syllable index part)
 *
 *     Input    : pbSyl       = the syllable string
 *                bSylSize    = syllable size
 *
 *    output    : pdwBIN      = Encoded BIN (exclude the syllable index)
 *
 *    Return    : bit mask for the BIN (exclude the syllable index)
 *
 *-----------------------------------------------------------------------*/
ET9U32 ET9FARCALL ET9_CP_PinyinSyllableToBIN(ET9U8  *pbSyl,
                                             ET9U8   bSylSize,
                                             ET9U32 *pdwBIN)
{
    ET9U8    i, bLetter, bRest = ET9_CP_MAX_PINYIN_SYL_SIZE - 2;
    ET9U32   dwMask = 0x07FFFF;
    int     fHasH = 0;

#ifdef ET9_DEBUG
    /* validate inputs */
    ET9Assert(pbSyl);
    ET9Assert(pdwBIN);
    ET9Assert((0 < bSylSize) && (bSylSize <= ET9_CP_MAX_PINYIN_SYL_SIZE));
    ET9Assert( ET9_CP_IsPinyinLetter(pbSyl[0]) );
    for (i = 1; i < bSylSize; i++) {
        ET9Assert( ET9_CP_IsPinyinLowerCase(pbSyl[i]) );
    }
#endif

    *pdwBIN = 0;
    if (ET9_CP_IsPinyinUpperCase(pbSyl[0])) {
        bLetter = (ET9U8)(pbSyl[0] - ET9_CP_FIRSTUPLETTER + ET9_CP_FIRSTLOWERLETTER);
    }
    else {
        bLetter = (ET9U8)pbSyl[0];
    }

    *pdwBIN = bLetter - ET9_CP_FIRSTLOWERLETTER + 1;
    /* set the bit if there is 'h' at the second position of the spelling */
    *pdwBIN = *pdwBIN << 1;
    i = 1;
    if (bSylSize > 1) {
        dwMask = dwMask >> 2;
        if (pbSyl[1] > 'h') {
            *pdwBIN |= 1;
        }
        *pdwBIN = *pdwBIN << 1;
        if (pbSyl[1] == 'h') {
            i++;
            *pdwBIN |= 1;
            bRest++;
            fHasH = 1;
        } else if (bSylSize > ET9_CP_MAX_PINYIN_SYL_SIZE - 1) {
            *pdwBIN = 0xFFFFFFFF;
        }
    } else {
        *pdwBIN = *pdwBIN << 1;
    }
    bSylSize--; /* to be compared with i */
    for(; i < bRest; i++) {
        *pdwBIN = (*pdwBIN << ET9_CP_PHLETTERLEN);
        if (i <= bSylSize) {
            *pdwBIN = *pdwBIN | ((ET9U32)pbSyl[i] - ET9_CP_FIRSTLOWERLETTER + 1);
            dwMask = dwMask >> ET9_CP_PHLETTERLEN;
        }
    }
    /* set the bit if there is a 'g' at the sixth position of the spelling */
    *pdwBIN = *pdwBIN << 1;
    if (bSylSize == bRest) {
        dwMask = dwMask >> 1;
        if (pbSyl[bRest] == 'g') {
            *pdwBIN |= 1;
        } else if ((!fHasH) || (bSylSize == ET9_CP_MAX_PINYIN_SYL_SIZE - 1)) {
            *pdwBIN = 0xFFFFFFFF;
        }
    }
    /* shifting to align the highest bit to the start of the second highest byte */
    *pdwBIN = *pdwBIN << (3 * 8 - ET9_CP_PHLETTERLEN * (ET9_CP_MAX_PINYIN_SYL_SIZE - 2) - 3);

    dwMask = ~dwMask;

    return dwMask;
} /* end of ET9_CP_PinyinSyllableToBIN() */


/*------------------------------------------------------------------------
 *
 *  Function    : ET9_CP_BINToPinyinSyllable
 *
 *  Synopsis    : It converts the encoded BIN to Pinyin syllable.
 *
 *     input    : dwBIN         = Encoded BIN. It uses only 3 bytes. The
 *                                most significant byte is set to 0.
 *
 *    Output    : pbSyl         = the syllable string
 *
 *    Return    : syllable size
 *
 *-----------------------------------------------------------------------*/
static ET9U8 ET9LOCALCALL ET9_CP_BINToPinyinSyllable(ET9U32  dwBIN,
                                                     ET9U8  *pbSyl)
{
    ET9U8 bSylSize, bCount, bTmp;

    bTmp = (ET9U8)(dwBIN >> 16);

    bSylSize = ET9_CP_MAX_PINYIN_SYL_SIZE;
    if (bTmp & 0x02) {
        *(pbSyl + 1)= 'h';
    } else {
        bSylSize--;
    }
    bTmp = (ET9U8)(bTmp & 0xF8);
    *pbSyl = (ET9U8)(bTmp >> 3);
    *pbSyl += (ET9U8)(ET9_CP_FIRSTUPLETTER - 1);
    pbSyl += (bSylSize - 1);
    dwBIN = dwBIN >> 1;
    if (dwBIN & 1) {
        *pbSyl = 'g';
    } else {
        bSylSize--;
    }
    pbSyl--;
    dwBIN = dwBIN >> 1;
    for (bCount = ET9_CP_MAX_PINYIN_SYL_SIZE - 2; bCount > 1; bCount--) {
        *pbSyl = (ET9U8)(dwBIN & 0x1F);
        dwBIN = dwBIN >> ET9_CP_PHLETTERLEN;
        if (*pbSyl) {
            *pbSyl = (ET9U8)(*pbSyl + ET9_CP_FIRSTLOWERLETTER - 1);
        } else {
            bSylSize--;
        }
        pbSyl--;
    }
    ET9Assert(bSylSize);

    return bSylSize;
} /* end of ET9_CP_BINToPinyinSyllable() */

static ET9U32 ET9LOCALCALL ET9_CP_ReadSylBin(ET9CPLingInfo *pLingInfo,
                                             ET9U32 dwBinOffset,
                                             ET9UINT nSylBinSize)
{
    ET9U32 dwBIN;
    ET9UINT i;

    dwBIN = 0;
    for (i = 0; i < nSylBinSize; i++) {
        dwBIN <<= 8;
        dwBIN |=(ET9U32)(ET9_CP_LdbReadByte(pLingInfo, dwBinOffset + i));
    }
    return dwBIN;
}


/*------------------------------------------------------------------------
 *
 *  Function    : ET9_CP_PidBidToSyllable
 *
 *  Synopsis    : It gets the character's spelling from its phonetic ID (PID or BID).
 *
 *     Input    : pET9CPLingInfo = Pointer to Chinese XT9 LingInfo structure.
 *                wPID           = Specified character's phonetic ID.
 *                pbSpell        = The buffer to store the retrieved spelling.
 *                                 It must be no less than 6 ET9U8 long.
 *                bIsBpmfOutput  = 0: output Pinyin, 1: output BPMF
 *
 *    output    : pbSpell         = The spelling for the specified phonetic ID.
 *                pbSpellLen      = The spelling length.
 *
 *    Return    : 1 on success, 0 otherwise
 *
 *-----------------------------------------------------------------------*/
ET9BOOL ET9FARCALL ET9_CP_PidBidToSyllable(ET9CPLingInfo * pET9CPLingInfo,
                                           ET9U16 wPID,
                                           ET9U8 *pbSpell,
                                           ET9U8 *pbSpellLen,
                                           ET9BOOL bIsBpmfOutput)
{
    ET9_CP_CommonInfo *psCommon = &pET9CPLingInfo->CommonInfo;
    ET9_CP_LdbOffsets *psLdbOffset = &(psCommon->sOffsets);
    ET9U32 dwBIN, *pdwSylBinTable;
    ET9UINT nSylCount, nCurrent, nStart, nEnd;
    ET9U16 *pwSylPidTable;
    ET9BOOL bIsDualLdb;

    *pbSpellLen = 0;

    if (!ET9_CP_IS_NORMAL_PID(psCommon, wPID)) {
        return 0;
    }

    bIsDualLdb = ET9_CP_IsLdbDualPhonetic(pET9CPLingInfo);
    /* decide which table to look */
    if (psLdbOffset->dwBpmfLDBOffset) { /* BPMF Syl to BID table */
        nSylCount = psCommon->wBpmfSylCount;
        pdwSylBinTable = pET9CPLingInfo->Private.PPrivate.adwBpmfBinTable;
    }
    else { /* Pinyin Syl to PID table */
        ET9Assert(psLdbOffset->dwPinyinLDBOffset);
        nSylCount = psCommon->wPinyinSylCount;
        pdwSylBinTable = pET9CPLingInfo->Private.PPrivate.adwPinyinBinTable;
    }
    pwSylPidTable = pET9CPLingInfo->Private.PPrivate.awSylPidTable;

    /* binary search to find the syllable that contains wPID, nStart is the 0-based index of the syllable */
    for (nStart = 0, nEnd = nSylCount; nStart < nEnd - 1; ) {
        ET9U16 wSylStartID;
        nCurrent = (nStart + nEnd) / 2;
        wSylStartID = pwSylPidTable[nCurrent];
        if (wSylStartID > wPID) {
            nEnd = nCurrent;
        }
        else {
            nStart = nCurrent;
            if (wSylStartID == wPID) {
                break; /* wPID happens to be the startID of this syllable, */
            }
        }
    }
    ET9Assert(nStart <= nSylCount);
    /* directly read out the BIN of the syllable found */
    dwBIN = pdwSylBinTable[nStart];
    if (bIsDualLdb) {
        dwBIN = (dwBIN >> 9) << 1; /* shift out the syl index */
    }
    if (bIsBpmfOutput) { /* output BPMF */
        ET9Assert(!ET9_CP_IsLdbPinyinOnly(pET9CPLingInfo));
        *pbSpellLen = ET9_CP_BINToBpmfSyllable(dwBIN, pbSpell);
    }
    else { /* output Pinyin */
        ET9Assert(!ET9_CP_IsLdbBpmfOnly(pET9CPLingInfo));
        if (bIsDualLdb) {
            /* read the Pinyin syllable index */
            nCurrent = (ET9UINT)(pdwSylBinTable[nStart] & 0x000001FF);
            /* get the Pinyin BIN from the Pinyin syllable table */
            dwBIN = (pET9CPLingInfo->Private.PPrivate.adwPinyinBinTable[nCurrent] >> 9) << 1; /* shift out the syl index */
        }
        *pbSpellLen = ET9_CP_BINToPinyinSyllable(dwBIN, pbSpell);
    }
    return 1;
} /* end of ET9_CP_PidBidToSyllable() */


/* Output a string of PIDs or BIDs to spelling. Encoding determined by LingInfo state */
void ET9FARCALL ET9_CP_PidBidToSpelling(ET9CPLingInfo *pET9CPLingInfo, ET9SYMB *pwID, ET9U8 bIDLen, ET9_CP_Spell *psSpell)
{
    ET9U8 b;
    ET9U8 *pb = psSpell->pbChars;
    ET9U8 bLen;
    ET9BOOL bNeedBpmf;

    ET9Assert(ET9CPIsModeBpmf(pET9CPLingInfo) || ET9CPIsModePinyin(pET9CPLingInfo));

    bNeedBpmf = ET9CPIsModeBpmf(pET9CPLingInfo);
    /* this loop is to fill up pSpellInfo->pbChars */
    for(b = 0; b < bIDLen; b++) {
        if (!ET9_CP_PidBidToSyllable(pET9CPLingInfo, pwID[b], pb, &bLen, bNeedBpmf) ) {
            psSpell->bLen = 0;
            return;
        }
        pb += bLen;
    }
    psSpell->bLen = (ET9U8)(pb - psSpell->pbChars);
}

/* Output a string of PIDs or BIDs to spelling, according to the native LDB spelling */
void ET9FARCALL ET9_CP_PidBidToNativeSpelling(ET9CPLingInfo *pET9CPLingInfo, ET9SYMB *pwID, ET9U8 bIDLen, ET9_CP_Spell *psSpell)
{
    ET9U8 b;
    ET9U8 *pb = psSpell->pbChars;
    ET9U8 bLen;
    ET9BOOL bNeedBpmf = ET9_CP_LdbHasBpmf(pET9CPLingInfo);
    /* this loop is to fill up pSpellInfo->pbChars */
    for(b = 0; b < bIDLen; b++) {
        if (!ET9_CP_PidBidToSyllable(pET9CPLingInfo, pwID[b], pb, &bLen, bNeedBpmf) ) {
            psSpell->bLen = 0;
            return;
        }
        pb += bLen;
    }
    psSpell->bLen = (ET9U8)(pb - psSpell->pbChars);
}

/* returns the mohu pinyin length, if not a mohu pinyin, return 0 */
static ET9U8 ET9LOCALCALL ET9_CP_ApplyMohuToSyl(ET9CPMOHU_PAIR ePair,
                                                const ET9U8 *pbSingleSyl,  /* todo: use ET9_CP_Syl struct */
                                                ET9U8 *pbMohu,
                                                ET9U8 bLen)
{
    ET9U8 b = 1;
    ET9U8 bLenMohu = bLen;

    if ((((ePair == ET9CPMOHU_PAIR_Z_ZH) && (*pbSingleSyl == 'Z')) ||
         ((ePair == ET9CPMOHU_PAIR_C_CH) && (*pbSingleSyl == 'C')) ||
         ((ePair == ET9CPMOHU_PAIR_S_SH) && (*pbSingleSyl == 'S'))) &&
        (bLenMohu > 1)) {
        if (*(pbSingleSyl + 1) == 'h') {
            *pbMohu++ = *pbSingleSyl++;
            pbSingleSyl++;
            b = 2;
            (bLenMohu--);
        } else {
            *pbMohu++ = *pbSingleSyl++;
            *pbMohu++ = 'h';
            (bLenMohu)++;
        }
    } else {
        *pbMohu = *pbSingleSyl;
        if (ePair == ET9CPMOHU_PAIR_N_L) {
            if (*pbSingleSyl == 'N') {
                *pbMohu = 'L';
            } else if (*pbSingleSyl == 'L') {
                *pbMohu = 'N';
            } else {
                bLenMohu = 0;
            }
        } else if (ePair == ET9CPMOHU_PAIR_R_L) {
            if (*pbSingleSyl == 'R') {
                *pbMohu = 'L';
            } else if (*pbSingleSyl == 'L') {
                *pbMohu = 'R';
            } else {
                bLenMohu = 0;
            }
        } else if (ePair == ET9CPMOHU_PAIR_F_H) {
            if (*pbSingleSyl == 'F') {
                *pbMohu = 'H';
            } else if (*pbSingleSyl == 'H') {
                *pbMohu = 'F';
            } else {
                bLenMohu = 0;
            }
        } else {
            bLenMohu = 0;
        }
        pbMohu++;
        pbSingleSyl++;
    }
    for(; b < bLen; b++) {
        *pbMohu++ = *pbSingleSyl++;
    }
    ET9Assert(bLenMohu <= ET9_CP_MAX_SYL_SIZE);
    ET9Assert(bLen <= ET9_CP_MAX_SYL_SIZE);
    if (ePair == ET9CPMOHU_PAIR_LAST) {
        return bLen;
    } else {
        return bLenMohu;
    }
}

/* return: syl size */
static ET9U8 ET9_CP_NextSylInSpell(ET9U8 *pbSpell,
                                     ET9UINT nSpellSize,
                                     ET9U8 **ppbSyl)
{
    ET9U8 bSylSize;

    ET9Assert(pbSpell && nSpellSize && ppbSyl);

    bSylSize = 0;
    /* skip to the 1st upper case letter, which is the beginning of the next syl */
    for (; nSpellSize > 0 && 0 == bSylSize; nSpellSize--, pbSpell++) {
        if (ET9_CP_IsUpperCase(*pbSpell)) {
            *ppbSyl = pbSpell;
            bSylSize = 1;
        }
    }
    /* find size of next syl */
    for (; nSpellSize > 0 && ET9_CP_IsLowerCase(*pbSpell); nSpellSize--, pbSpell++) {
        bSylSize++;
    }
    return bSylSize;
}

static ET9BOOL ET9LOCALCALL ET9_CP_SylCanBePartial(ET9U8 *pbSyl, ET9U8 bSylSize)
{
    ET9Assert(pbSyl && bSylSize > 0);
    if (1 == bSylSize) {
        return 1;
    }
    else if (2 == bSylSize && 'h' == pbSyl[1]) {
        return 1;
    }
    return 0;
}

static ET9BOOL ET9LOCALCALL ET9_CP_Has_ng_Mohu(ET9U8 *pbSyl,
                                               ET9U8 *pbSylSize,
                                               ET9U16 wMohuFlags)
{
    ET9BOOL bHas_ng;

    bHas_ng = 0;

    if (wMohuFlags & (ET9CPMOHU_PAIR_AN_ANG_MASK | ET9CPMOHU_PAIR_EN_ENG_MASK | ET9CPMOHU_PAIR_IN_ING_MASK)) {
        ET9U8 bVowel;
        if (*(pbSyl + *pbSylSize - 1) == 'n') {
            bVowel = *(pbSyl + *pbSylSize - 2);
            if ((((bVowel == 'a') || (bVowel == 'A')) && (wMohuFlags & ET9CPMOHU_PAIR_AN_ANG_MASK)) ||
                ((bVowel == 'i') && (wMohuFlags & ET9CPMOHU_PAIR_IN_ING_MASK)) ||
                (((bVowel == 'e') || (bVowel == 'E')) && (wMohuFlags & ET9CPMOHU_PAIR_EN_ENG_MASK))) {
                bHas_ng = 1; /* n -> n + partial match */
            }
        } else if ((*(pbSyl + *pbSylSize - 1) == 'g') && (*(pbSyl + *pbSylSize - 2) == 'n')) {
            bVowel = *(pbSyl + *pbSylSize - 3);
            if ((((bVowel == 'a') || (bVowel == 'A')) && (wMohuFlags & ET9CPMOHU_PAIR_AN_ANG_MASK)) ||
                ((bVowel == 'i') && (wMohuFlags & ET9CPMOHU_PAIR_IN_ING_MASK)) ||
                (((bVowel == 'e') || (bVowel == 'E')) && (wMohuFlags & ET9CPMOHU_PAIR_EN_ENG_MASK))
                ) {
                bHas_ng = 1;
                (*pbSylSize)--; /* ng -> n + partial match */
            }
        }
    }
    return bHas_ng;
}

ET9U8 ET9FARCALL ET9_CP_SpellingToPidRanges(ET9CPLingInfo * pLingInfo,
                                            ET9U8 *pbSpell,
                                            ET9UINT nSpellSize)
{
    ET9U16 *pwRange;
    ET9U8 *pbRangeEnd;
    ET9U8 *pbSyl = NULL; /* avoid compiler warning */
    ET9U8 bSylCount, bSylSize;
    ET9BOOL bIsPinyinSpell, bIsDualPhoneticLdb, bFindPartial, bHas_ng, bHasTone;
    ET9U16 wMohuFlags;

    ET9Assert(!ET9_CP_IS_LINGINFO_NOINIT(pLingInfo));
    ET9Assert(pbSpell && nSpellSize);

    pwRange = pLingInfo->CommonInfo.pwRange;
    pbRangeEnd = pLingInfo->CommonInfo.pbRangeEnd;
    *pbRangeEnd = 0;
    
    if (!ET9_CP_IsUpperCase(pbSpell[0])) {
        pLingInfo->CommonInfo.bSylCount = 0;
        return 0;
    }

    bIsPinyinSpell = (ET9BOOL)ET9_CP_IsPinyinLetter(*pbSpell);
    bIsDualPhoneticLdb = ET9_CP_IsLdbDualPhonetic(pLingInfo);
    wMohuFlags = ET9_CP_GetMohuFlags(pLingInfo);

    bHasTone = 0;
    for (bSylCount = 0; nSpellSize > 0; bSylCount++) {
        ET9BOOL fSylEndDel = 0;
        bSylSize = ET9_CP_NextSylInSpell(pbSpell, nSpellSize, &pbSyl);
        if (0 == bSylSize) {
            break; /* no more syllable, done */
        }
        nSpellSize -= (pbSyl + bSylSize - pbSpell);
        pbSpell = (pbSyl + bSylSize);
        if (nSpellSize > 0 && ET9_CP_IsToneOrDelim(*pbSpell) ) { /* skip tone/delimiter after the current syl */
            if (ET9CPSymToCPTone(*pbSpell)) {
                bHasTone = 1;
            }
            nSpellSize--;
            pbSpell++;
            fSylEndDel = 1;
        }
        if (bHasTone)
        {
            bFindPartial = (ET9BOOL)(0 == nSpellSize && !fSylEndDel);
        }
        else 
        {
            bFindPartial = (ET9BOOL)(0 == nSpellSize && !fSylEndDel ||
                ((ET9_CP_InputContainsTrace(pLingInfo)) || ET9CPIsPartialSpellActive(pLingInfo))
                && ET9_CP_SylCanBePartial(pbSyl, bSylSize) );
        }

        if (bIsPinyinSpell) { /* Pinyin syl */
            ET9UINT nPair;
            ET9U16 wMohuMask;
            ET9U8 pbMohuSyl[ET9_CP_MAX_PINYIN_SYL_SIZE + 1], bMohuSylSize; /* todo: use ET9_CP_Syl struct */

            /* check if the syl has -ng mohu, if so, change it to -n and let partial match handle -ng */
            bHas_ng = ET9_CP_Has_ng_Mohu(pbSyl, &bSylSize, wMohuFlags);
            bFindPartial = (ET9BOOL)(bFindPartial || bHas_ng);
            for(nPair = wMohuFlags ? ET9CPMOHU_PAIR_Z_ZH : ET9CPMOHU_PAIR_LAST; nPair <= ET9CPMOHU_PAIR_LAST; nPair++)
            {
                wMohuMask = (ET9U16)( 1 << nPair );
                /* this loop deals with initial sound (ShengMu) only.  final sound (YunMu) will be handled by partial range match */
                if (nPair == ET9CPMOHU_PAIR_LAST || (wMohuMask & wMohuFlags) )
                {
                    if (ET9_CP_MAX_PINYIN_SYL_SIZE < bSylSize) {
                        continue; /* reject overly long syllables */
                    }
                    bMohuSylSize = ET9_CP_ApplyMohuToSyl((ET9CPMOHU_PAIR)nPair, pbSyl, pbMohuSyl, bSylSize);
                    ET9Assert(bMohuSylSize <= ET9_CP_MAX_PINYIN_SYL_SIZE + 1); /* should not have overrun pbMohuSyl[] */

                    if (bMohuSylSize > 0 && bMohuSylSize <= ET9_CP_MAX_PINYIN_SYL_SIZE)
                    {
                        if (bIsDualPhoneticLdb) {
                            /* Traditional Pinyin does not have Mohu support. Only check Partial Pinyin */
                            if (!ET9_CP_SylbToBIDRanges(pLingInfo, pbMohuSyl, bMohuSylSize, bFindPartial, &pwRange, &pbRangeEnd)){
                                continue; /* reject invalid syllable */
                            }
                        }
                        else { /* Pinyin syl in Pinyin-only Ldb */
                            if (!ET9_CP_SyllableToPidRange(pLingInfo, pbMohuSyl, bMohuSylSize, bFindPartial, pwRange, pwRange + 1, pwRange + 2)) {
                                continue; /* reject invalid syllable */
                            }
                            if (bHas_ng) { /* for Mohu: treat "...ng" partial as "...n" exact match. */
                                *(pwRange + 1) = *(pwRange + 2);
                            }
                            *pbRangeEnd += ET9_CP_ID_RANGE_SIZE;
                            pwRange += ET9_CP_ID_RANGE_SIZE;
                        }
                    }
                }
            }
        }
        else { /* BPMF syl */
            if ( ET9_CP_SyllableToPidRange(pLingInfo, pbSyl, bSylSize, bFindPartial, pwRange, pwRange + 1, pwRange + 2) )
            {
                pwRange += ET9_CP_ID_RANGE_SIZE;
                *pbRangeEnd += ET9_CP_ID_RANGE_SIZE;
            }
        }
        if (   (bSylCount == 0 && *pbRangeEnd == 0)
            || (bSylCount > 0 && *pbRangeEnd == *(pbRangeEnd - 1) ) )
        {   /* this syllable gives no new ranges, so the whole spelling is rejected */
            pLingInfo->CommonInfo.bSylCount = 0;
            return 0;
        }
        pbRangeEnd++;
        *pbRangeEnd = *(pbRangeEnd - 1);
    }
    pLingInfo->CommonInfo.bSylCount = bSylCount;
    return bSylCount;
}

ET9BOOL ET9FARCALL ET9_CP_WSIToJianpinPidRanges(ET9CPLingInfo * pLingInfo)
{
    ET9WordSymbInfo * pWSI;
    const ET9SymbInfo *pSI;
    ET9U16 *pwRange;
    ET9U8 *pbRangeEnd;
    ET9U16 wMohuFlags;
    ET9U8 bSylCount, b;
    ET9BOOL bIsPinyinMode, bIsDualPhoneticLdb, bLastIsDelim;

    ET9Assert(ET9CPIsModePhonetic(pLingInfo));

    pWSI = pLingInfo->Base.pWordSymbInfo;
    pwRange = pLingInfo->CommonInfo.pwRange;
    pbRangeEnd = pLingInfo->CommonInfo.pbRangeEnd;
    *pbRangeEnd = 0;
    bSylCount = 0;

    bIsPinyinMode = ET9CPIsModePinyin(pLingInfo);
    bIsDualPhoneticLdb = ET9_CP_IsLdbDualPhonetic(pLingInfo);
    wMohuFlags = ET9_CP_GetMohuFlags(pLingInfo);

    bLastIsDelim = 1;
    for (b = 0, pSI = pWSI->SymbsInfo; b < pWSI->bNumSymbs; b++, pSI++)
    {
        ET9SYMB symb = 0;
        ET9U8 bSymbInternal, i, j;

        if (ET9_CP_SymbIsDelim(pSI)) { /* user-entered delimiter, skip to next symb info */
            if ( bLastIsDelim ) {
                return 0;   /* Leading delimiter or 2 consecutive delimiter is not allowed */
            }
            bLastIsDelim = 1;
            continue; /* Assumption: delimiter must be the only symb in a symb info */
        }

        bLastIsDelim = 0;
        if (bSylCount >= ET9CPMAXUDBPHRASESIZE) {
            return 0; /* reject WSI that's too long */
        }

        /* find a symb valid for Jianpin spelling */
        for (i = 0; i < /*pSI->bNumBaseSyms*/1; i++)
        {
            for (j = 0; j < pSI->DataPerBaseSym[i].bNumSymsToMatch; j++)
            {
                symb = pSI->DataPerBaseSym[i].sUpperCaseChar[j];

                /* symb is Pinyin letter */
                if (bIsPinyinMode && ET9CPIsPinyinSymbol(symb))
                {
                    ET9UINT nPair;
                    ET9U16 wMohuMask;
                    ET9U8 pbMohuSyl[ET9_CP_MAX_PINYIN_SYL_SIZE], bMohuSylSize;
                    bSymbInternal = (ET9U8)symb;
                    /* Since the given syllable has only 1 letter, we can ignore Zh-Ch-Sh mohu pairs and start from N-L pair */
                    for(nPair = wMohuFlags ? ET9CPMOHU_PAIR_N_L : ET9CPMOHU_PAIR_LAST; nPair <= ET9CPMOHU_PAIR_LAST; nPair++) {
                        wMohuMask = (ET9U16)( 1 << nPair );
                        /* this loop deals with initial sound (ShengMu) only.  final sound (YunMu) will be handled by partial range match */
                        if (nPair == ET9CPMOHU_PAIR_LAST || (wMohuMask & wMohuFlags) ) {
                            bMohuSylSize = ET9_CP_ApplyMohuToSyl((ET9CPMOHU_PAIR)nPair, &bSymbInternal, pbMohuSyl, 1);
                            if (0 == bMohuSylSize) {
                                continue;
                            }
                            if (bIsDualPhoneticLdb) { /* Dual phonetic Ldb */
                                /* Traditional Pinyin does not have Mohu support. Only check Partial Pinyin */
                                if (!ET9_CP_SylbToBIDRanges(pLingInfo, pbMohuSyl, bMohuSylSize, 1, &pwRange, &pbRangeEnd)){
                                    continue; /* reject invalid syllable */
                                }
                            }
                            else { /* Pinyin syl in Pinyin-only Ldb */
                                if (!ET9_CP_SyllableToPidRange(pLingInfo, pbMohuSyl, bMohuSylSize, 1, pwRange, pwRange + 1, pwRange + 2)) {
                                    continue; /* reject invalid syllable */
                                }
                                *pbRangeEnd += ET9_CP_ID_RANGE_SIZE;
                                pwRange += ET9_CP_ID_RANGE_SIZE;
                            }
                        }
                    }
                }
                else if (!bIsPinyinMode && ET9CPIsBpmfSymbol(symb))
                { /* symb is BPMF letter */
                    bSymbInternal = ET9_CP_BpmfExternalToInternal(ET9CPBpmfSymbolToLower(symb));
                    if ( ET9_CP_SyllableToPidRange(pLingInfo, &bSymbInternal, 1, 1, pwRange, pwRange + 1, pwRange + 2) )
                    {
                        pwRange += ET9_CP_ID_RANGE_SIZE;
                        *pbRangeEnd += ET9_CP_ID_RANGE_SIZE;
                    }
                }
            }
        }
        if (   (bSylCount == 0 && *pbRangeEnd == 0)
            || (bSylCount > 0 && *pbRangeEnd == *(pbRangeEnd - 1) ) )
        {   /* this symb info gives no new ranges, so Jianpin cannot interpret this WSI */
            pLingInfo->CommonInfo.bSylCount = 0;
            return 0;
        }
        /* has new ranges, setup for next syllable before going to next symb info */
        pbRangeEnd++;
        *pbRangeEnd = *(pbRangeEnd - 1);
        bSylCount++;
    }

    pLingInfo->CommonInfo.bSylCount = bSylCount;
    return (ET9BOOL)(bSylCount > 0);
}

ET9BOOL ET9FARCALL ET9_CP_UniPhraseToPidRanges(ET9CPLingInfo *pLing,
                                               const ET9CPPhrase *pPhrase)
{
    ET9U16 *pwRange;
    ET9U8 *pbRangeEnd;
    ET9U16 wMohuFlags;
    ET9U16 awPID[ET9CPMAXPHRASESIZE][ET9_CP_MAX_ALT_SYLLABLE];
    ET9U8 bSylCount, i, j;
    ET9BOOL bIsBpmfSyl;

    if (!ET9_CP_UniPhraseToAltPID(pLing, pPhrase, (ET9U16*)awPID, ET9_CP_MAX_ALT_SYLLABLE) ) {
        pLing->CommonInfo.bSylCount = 0;
        return 0; /* some Unicode has no PID, return */
    }

    pwRange = pLing->CommonInfo.pwRange;
    pbRangeEnd = pLing->CommonInfo.pbRangeEnd;
    pbRangeEnd[0] = 0;
    bSylCount = 0;

    wMohuFlags = ET9_CP_GetMohuFlags(pLing);
    bIsBpmfSyl = ET9_CP_LdbHasBpmf(pLing);

    for (i = 0; i < pPhrase->bLen; i++) {
        /* try each alt PID of the current char */
        for (j = 0; j < ET9_CP_MAX_ALT_SYLLABLE && awPID[i][j] != ET9_CP_NOMATCH; j++) {
            ET9_CP_Syl sSyl;

            /* get syl of this alt PID, according to native LDB spelling */
            if (!ET9_CP_PidBidToSyllable(pLing, awPID[i][j], sSyl.aChars, &(sSyl.bSize), bIsBpmfSyl) ) {
                continue; /* reject mute PID */
            }

            /* use this syl to setup a new PID range for this char */

            /* apply the non-mohu syl first */
            if (ET9_CP_SyllableToPidRange(pLing, sSyl.aChars, sSyl.bSize, 0, pwRange, pwRange + 1, pwRange + 2) ) {
                *pbRangeEnd += ET9_CP_ID_RANGE_SIZE; /* next range for this syllable */
                pwRange += ET9_CP_ID_RANGE_SIZE;
            }

            /* append the mohu syl if applicable */
            if (wMohuFlags) {
                ET9_CP_Syl sMohuSyl;
                int nPair;
                ET9U16 wMohuMask;
                ET9BOOL bHas_ng;

                /* check if the syl has -ng mohu, if so, change it to -n and let partial match handle -ng */
                bHas_ng = ET9_CP_Has_ng_Mohu(sSyl.aChars, &sSyl.bSize, wMohuFlags);

                for(nPair = ET9CPMOHU_PAIR_Z_ZH; nPair < ET9CPMOHU_PAIR_LAST; nPair++) {
                    wMohuMask = (ET9U16)( 1 << nPair );
                    /* this loop deals with initial sound (ShengMu) only.  final sound (YunMu) will be handled by partial range match */
                    if (wMohuMask & wMohuFlags) {
                        sMohuSyl.bSize = ET9_CP_ApplyMohuToSyl((ET9CPMOHU_PAIR)nPair, sSyl.aChars, sMohuSyl.aChars, sSyl.bSize);
                        if (0 == sMohuSyl.bSize) {
                            continue; /* reject */
                        }
                        ET9Assert(!ET9_CP_IsLdbDualPhonetic(pLing) ); /* assume Trad Ldb does not support Mohu */
                        if (ET9_CP_SyllableToPidRange(pLing, sMohuSyl.aChars, sMohuSyl.bSize, bHas_ng, pwRange, pwRange + 1, pwRange + 2) ) {
                            if (bHas_ng) { /* for Mohu: treat "...ng" partial as "...n" exact match. */
                                *(pwRange + 1) = *(pwRange + 2);
                            }
                            *pbRangeEnd += ET9_CP_ID_RANGE_SIZE; /* next range for this syllable */
                            pwRange += ET9_CP_ID_RANGE_SIZE;
                        }
                    }
                }
            } /* END apply mohu */
        } /* END loop all alt PID */

        if (   (bSylCount == 0 && *pbRangeEnd == 0)
            || (bSylCount > 0 && *pbRangeEnd == *(pbRangeEnd - 1) ) )
        {   /* this char gives no new ranges, so reject the whole phrase */
            pLing->CommonInfo.bSylCount = 0;
            return 0;
        }
        /* next syllable */
        pbRangeEnd++;
        *pbRangeEnd = *(pbRangeEnd - 1);
        bSylCount++;
    } /* END loop all char in phrase */

    pLing->CommonInfo.bSylCount = bSylCount;
    return (ET9BOOL)(bSylCount > 0);
}

/*------------------------------------------------------------------------
 *
 *  Function    : ET9_CP_SearchSylbFromTable
 *
 *  Synopsis    : It does binary search for the syllable from the LDB table.
 *
 *     Input    : pET9CPLingInfo = Pointer to Chinese XT9 LingInfo structure
 *                bIsBpmf        = 1: the syl is BPMF, 0: the syl is Pinyin
 *                bPartial       = 0 - the syllable is complete
 *                                 1 - the syllabel can be partial or complete
 *                pdwCharBIN     = encoded syllable
 *                pdwMask        = mask for the syllable
 *
 *    Output    : pwStartIndex   = start syllable index
 *                pwEndIndex     = end syllable index (for partial match only)
 *
 *    Return    : 0 - success; 1 - the specified syllable is not found.
 *
 *-----------------------------------------------------------------------*/
ET9U8 ET9FARCALL ET9_CP_SearchSylbFromTable(ET9CPLingInfo * pET9CPLingInfo,
                                            ET9BOOL         bIsBpmf,
                                            ET9BOOL         bPartial,
                                            ET9U32          dwCharBIN,
                                            ET9U32          dwMask,
                                            ET9U16 *        pwStartIndex,
                                            ET9U16 *        pwEndIndex)
{
    ET9_CP_CommonInfo *psCommon = &pET9CPLingInfo->CommonInfo;
    ET9U32 dwBIN, *pdwSylBinTable;
    ET9UINT nSylCount, nStart, nEnd, nMid, nMidPoint;
    ET9BOOL bIsDualLdb, bLookForStart = 0, bLookForEnd = 0;

    bIsDualLdb = ET9_CP_IsLdbDualPhonetic(pET9CPLingInfo);
    if (bIsBpmf) {
        nSylCount = psCommon->wBpmfSylCount;
        pdwSylBinTable = pET9CPLingInfo->Private.PPrivate.adwBpmfBinTable;
    }
    else {
        nSylCount = psCommon->wPinyinSylCount;
        pdwSylBinTable = pET9CPLingInfo->Private.PPrivate.adwPinyinBinTable;
    }

    /* this for loop looks for both the start and end syllable that matches the spell.
     * The algorithm is:
     * 1, look for a syllabe that matches the spell exactly or partially, nMidPoint
     * 2, in the range from beginning to this point, search for where the match starts
     * 3, search for where the match ends from wMidPoint to the end.
     * Note: binary search.  In this loop, nStart, nMid and nEnd are all 1-based
     */
    nMidPoint = 0;
    for (nStart = 0, nMid = nEnd = nSylCount + 1; ; ) {
        if ((nMid == nStart) && (nStart == nEnd - 1)) {
            /* in the first step to look for a matching bin.  We know dwBIN is different from dwCharBIN */
            nStart++;
        }
        nMid = (nStart + nEnd) / 2;
        if (nMid) {
            dwBIN = pdwSylBinTable[nMid - 1];
            if (bIsDualLdb) {
                dwBIN = dwBIN >> 8;
            }
        }
        else {
            dwBIN = 0; /* nEnd is the first syllable */
        }
        if ((dwMask & dwBIN) > dwCharBIN) {
            nEnd = nMid;
        } else if ((dwMask & dwBIN) == dwCharBIN) {
            if (bLookForStart) {
                nEnd = nMid;
            } else if (bLookForEnd) {
                nStart = nMid;
            } else {
                nStart = nEnd = nMid;
            }
        } else {
            nStart = nMid;
        }
        if (nStart == nEnd) {
            if ((dwMask & dwBIN) != dwCharBIN) {
                return 1;
            }
            nMidPoint = nStart;
            bLookForStart = 1;
            nStart = 0;
        } else if (nStart == nEnd - 1) {
            if (bLookForStart) {
                *pwStartIndex = (ET9U16)(nEnd - 1); /* inclusive, converting to 0-based */
                if (!bPartial) {
                    break;
                }
                bLookForStart = 0;
                bLookForEnd = 1;
                nStart = nMidPoint;
                nEnd = nSylCount + 1;
            } else if (bLookForEnd) {
                *pwEndIndex = (ET9U16)(nEnd - 1); /* exclusive, converting to 0-based */
                break;
            }
        }
    }
    return 0;
}   /* end of ET9_CP_SearchSylbFromTable() */


/*------------------------------------------------------------------------
 *
 *  Function    : ET9_CP_SyllableToPidRange
 *
 *  Synopsis    : It gets the phonetic ID (BID or PID) range for the specified syllable.
 *
 *     Input    : pET9CPLingInfo = Pointer to Chinese XT9 Linguistic info structure.
 *                pbSpell        = Character's spelling.
 *                bSpellLen      = Character's spelling length.
 *                bPartial       = 0 - the syllable is complete;
 *                                 1 - the syllable can be partial or complete
 *
 *    Output    : pwStartPID     = First phonetic ID.
 *                pwPartStartPID = First phonetic ID for partial syllable.
 *                pwPartEndPID   = Last phonetic ID for partial syllable.
 *
 *    Return    : 1 - success; 0 - the specified syllable is not found.
 *
 *    NOTE:       *pwStartPID <= exact match < *pwPartStartPID
 *                *pwPartStartPID <= partial match < *pwPartEndPID
 *-----------------------------------------------------------------------*/
ET9BOOL ET9FARCALL ET9_CP_SyllableToPidRange(ET9CPLingInfo * pET9CPLingInfo,
                                             ET9U8 *         pbSyl,
                                             ET9U8           bSylSize,
                                            ET9BOOL          bPartial,
                                            ET9U16 *         pwStartPID,
                                            ET9U16 *         pwPartStartPID,
                                            ET9U16 *         pwPartEndPID)
{
    ET9U32   dwCharBIN, dwBIN, dwMask, *pdwSylBinTable;
    ET9U16   wStartIndex = 0, wEndIndex = 0, *pwSylPidTable;
    ET9BOOL bIsBpmf, bIsDualLdb;

    ET9Assert(bSylSize);
    ET9Assert(!(ET9CPSymToCPTone(pbSyl[bSylSize - 1]) || (ET9CPSYLLABLEDELIMITER == pbSyl[bSylSize - 1])));

    /* encode the given syllable to a BIN */
    bIsDualLdb = ET9_CP_IsLdbDualPhonetic(pET9CPLingInfo);
    if (ET9_CP_IsBpmfLetter(pbSyl[0])) {
        ET9Assert(ET9_CP_IsLdbBpmfOnly(pET9CPLingInfo) || ET9_CP_IsLdbDualPhonetic(pET9CPLingInfo));
        dwMask = ET9_CP_BpmfSyllableToBIN(pbSyl, bSylSize, &dwCharBIN);
        pdwSylBinTable = pET9CPLingInfo->Private.PPrivate.adwBpmfBinTable;
        bIsBpmf = 1;
    }
    else {
        if (!ET9_CP_IsLdbPinyinOnly(pET9CPLingInfo)) {
            return 0;  /*  the specified syllable is not found  */
        }
        dwMask = ET9_CP_PinyinSyllableToBIN(pbSyl, bSylSize, &dwCharBIN);
        pdwSylBinTable = pET9CPLingInfo->Private.PPrivate.adwPinyinBinTable;
        bIsBpmf = 0;
    }
    pwSylPidTable = pET9CPLingInfo->Private.PPrivate.awSylPidTable;

    if (ET9_CP_SearchSylbFromTable(pET9CPLingInfo, bIsBpmf, bPartial, dwCharBIN, dwMask, &wStartIndex, &wEndIndex)) {
        return 0;
    }

    /* read the encoded syl from the syl bin table */
    dwBIN = pdwSylBinTable[wStartIndex];
    if (bIsDualLdb) {
        dwBIN = (dwBIN >> 9) << 1; /* shift out the syl index */
    }

    /* read the syl PID from table (start, partial-start, partial-end) */
    *pwStartPID = pwSylPidTable[wStartIndex];
    if (dwBIN == dwCharBIN) { /* this syl is a complete syl, so has exact match PID */
        *pwPartStartPID = pwSylPidTable[wStartIndex + 1];
    }
    else { /* not a complete syl, no exact match PID */
        *pwPartStartPID = *pwStartPID;
    }
    if (pwPartEndPID) {
        *pwPartEndPID = *pwPartStartPID;
        if (bPartial) { /* caller accept partial syl */
            *pwPartEndPID = pwSylPidTable[wEndIndex];
        } else if (dwBIN != dwCharBIN) { /* caller refuse partial syl but this syl is not a complete syl => invalid result */
            return 0;
        }
    }
    return 1;
} /* END ET9_CP_SyllableToPidRange() */

ET9U8 ET9FARCALL ET9_CP_FreqLookup(ET9CPLingInfo *pET9CPLingInfo, ET9U16 wPID)
{
    ET9U32 dwOffset;
    dwOffset = pET9CPLingInfo->CommonInfo.sOffsets.dwFrequencyOffset + wPID + 2; /* skip PID count at table header, which is sizeof(ET9U16) */
    return ET9_CP_LdbReadByte(pET9CPLingInfo, dwOffset);
} /* END ET9_CP_FreqLookup() */

ET9U8 ET9FARCALL ET9_CP_LookupTone(ET9CPLingInfo *pET9CPLingInfo,
                                   ET9U16 wPID)
{
    return ET9_CP_LdbReadByte(pET9CPLingInfo, pET9CPLingInfo->CommonInfo.sOffsets.dwToneOffset + wPID);
}

/* Attempt to fill our Unicode to PID cache with PIDs for this unicode value.  *
 * If the value doesn't match any PID, the cache remains unchanged.            */
static void ET9LOCALCALL ET9_CP_FillUnicodePIDLookupCache(ET9CPLingInfo *pET9CPLingInfo, ET9SYMB sUnicode)
{
    ET9_CP_CommonInfo *psCommon = &pET9CPLingInfo->CommonInfo;
    ET9UINT nCacheAltIndex;
    ET9U32 dwOffset;

    ET9U16 wCurrentPID;
    ET9U16 wCurrentUID;

    /* binary search for PID that matches this unicode */
    /* reverse lookup table contains PIDs for normal chars and mute chars only */
    ET9UINT high = ET9_CP_NORMAL_PID_COUNT(&pET9CPLingInfo->CommonInfo) + ET9_CP_MUTE_PID_COUNT(&pET9CPLingInfo->CommonInfo);
    ET9UINT mid = high/2;
    ET9UINT low = 0;

    for (;;) {

        dwOffset = psCommon->sOffsets.dwUnicodeSortedPIDOffset + (ET9U32)mid * sizeof(ET9U16);
        wCurrentPID = ET9_CP_LdbReadWord(pET9CPLingInfo, dwOffset);
        wCurrentUID = ET9_CP_LookupUnicode(pET9CPLingInfo, wCurrentPID);

        if (mid == low && wCurrentUID != sUnicode) {
            return; /* no matches for this unicode */
        }

        if (wCurrentUID < sUnicode) {
            low = mid;
            mid = (high + low)/2;
        }
        else if (wCurrentUID > sUnicode) {
            high = mid;
            mid = (high + low)/2;
        }
        else {
            break;
        }
    }

    /* jump back to ensure we're before the first matching PID */
    if (mid < low + ET9_CP_MAX_ALT_SYLLABLE - 1) {
        mid = low;
    }
    else {
        mid -= (ET9_CP_MAX_ALT_SYLLABLE - 1);
    }

    /* increment until we find the first unicode match */
    dwOffset = psCommon->sOffsets.dwUnicodeSortedPIDOffset + (ET9U32)mid * sizeof(ET9U16);
    do {
        ET9Assert(mid < high);

        wCurrentPID = ET9_CP_LdbReadWord(pET9CPLingInfo, dwOffset);
        wCurrentUID = ET9_CP_LookupUnicode(pET9CPLingInfo, wCurrentPID);
        mid++;
        dwOffset += 2; /* sizeof(ET9U16) */
    }
    while (wCurrentUID != sUnicode);

    /* cache our results */
    {
        ET9_CP_SpellLookupCache *psLookupCache = &pET9CPLingInfo->Private.PPrivate.sLookupCache;

        psLookupCache->sUnicode = sUnicode;
        for(nCacheAltIndex = 0; wCurrentUID == sUnicode; nCacheAltIndex++) {
            ET9Assert(nCacheAltIndex < ET9_CP_MAX_ALT_SYLLABLE);
            ET9Assert(mid + nCacheAltIndex <= high);

            psLookupCache->awPIDs[nCacheAltIndex] = wCurrentPID;
            wCurrentPID = ET9_CP_LdbReadWord(pET9CPLingInfo, dwOffset);
            wCurrentUID = ET9_CP_LookupUnicode(pET9CPLingInfo, wCurrentPID);
            dwOffset += 2; /* sizeof(ET9U16) */
        }
        psLookupCache->bNumAlternates = (ET9U8)nCacheAltIndex;
    }
}


/*------------------------------------------------------------------------
 *
 *  Function    : ET9_CP_UnicodeToPID
 *
 *  Synopsis    : It converts a character's Unicode to its phonetic ID for
 *                specified alternate index. Supports normal chars, mute
 *                chars, and components, but not symbols.
 *
 *     Input    : pET9CPLingInfo = Pointer to Chinese XT9 LingInfo structure.
 *                wUnicode       = Specified character's Unicode.
 *                bAltIndex      = Specified alternate spelling index.
 *                                 0: most common pronunciation.
 *
 *    output    :
 *
 *    Return    : The character's phonetic ID.
 *
 *-----------------------------------------------------------------------*/
ET9U16 ET9FARCALL ET9_CP_UnicodeToPID(ET9CPLingInfo *pET9CPLingInfo,
                                  ET9SYMB sUnicode,
                                  ET9U8 bAltIndex)
{
    ET9_CP_SpellLookupCache *psLookupCache = &pET9CPLingInfo->Private.PPrivate.sLookupCache;

    if (ET9_CP_NOMATCH == sUnicode) { /* because this is our code for 'no cache' */
        return ET9_CP_NOMATCH;
    }

    /* handle components */
    {
        ET9U32 dwOffset;
        ET9U16 wCompStart, wCompEnd;

        dwOffset = pET9CPLingInfo->CommonInfo.sOffsets.dwPIDToUnicodeOffset;
        wCompStart = ET9_CP_LdbReadWord(pET9CPLingInfo, dwOffset);
        dwOffset += 2; /* sizeof(ET9U16) */
        wCompEnd = ET9_CP_LdbReadWord(pET9CPLingInfo, dwOffset);
        if ((sUnicode >= wCompStart) && (sUnicode <= wCompEnd)) {
            if (ET9_CP_LdbHasStroke(pET9CPLingInfo) ) {
                static const ET9SYMB asNON_COMPONENT_UNICODE[] = {0xF116}; /* a list of "holes" in the continuous component Unicode range */
                int i;
                for (i = 0; i < sizeof(asNON_COMPONENT_UNICODE) / sizeof(asNON_COMPONENT_UNICODE[0]); i++) {
                    if (sUnicode == asNON_COMPONENT_UNICODE[i]) {
                        return ET9_CP_NOMATCH;
                    }
                }
                /* valid component Unicode, use formula to compute PID */
                return ((ET9U16)(sUnicode - wCompStart + pET9CPLingInfo->CommonInfo.w1stComponentPID));
            }
            else {
                return ET9_CP_NOMATCH;
            }
        }
    }

    /* do we need to fill cache? */
    if (ET9_CP_NOMATCH == psLookupCache->sUnicode || psLookupCache->sUnicode != sUnicode) {
        ET9_CP_FillUnicodePIDLookupCache(pET9CPLingInfo, sUnicode);
    }

    /* does our cache match, and does it have the desired alternate? */
    if (psLookupCache->sUnicode != sUnicode || bAltIndex >= psLookupCache->bNumAlternates) {
        return ET9_CP_NOMATCH;
    }

    return psLookupCache->awPIDs[bAltIndex];
}


/*------------------------------------------------------------------------
 *
 *  Function    : ET9_CP_LookupUnicode
 *
 *  Synopsis    : It converts an PID its Unicode.
 *
 *     Input    : pET9CPLingInfo = Pointer to Chinese XT9 LingInfo structure.
 *                wPID           = Specified phonetic ID.
 *
 *    output    :
 *
 *    Return    : The character's Unicode.
 *
 *-----------------------------------------------------------------------*/
ET9SYMB ET9FARCALL ET9_CP_LookupUnicode(ET9CPLingInfo * pET9CPLingInfo, ET9U16 wPID)
{
    ET9_CP_CommonInfo *psCommon = &pET9CPLingInfo->CommonInfo;
    ET9SYMB  sID;

    if (ET9_CP_IS_COMP_PID(psCommon, wPID) && ET9CPIsModeStroke(pET9CPLingInfo)) {
        sID = (ET9SYMB)(wPID - psCommon->w1stComponentPID + psCommon->wComponentFirst);
    }
    else {
        ET9U32 dwOffset;

        /* skip PID count, the 1st component Unicode, and the last component Unicode */
        dwOffset = psCommon->sOffsets.dwPIDToUnicodeOffset + ((ET9U32)wPID + 3) * sizeof(ET9U16);
        sID = ET9_CP_LdbReadWord(pET9CPLingInfo, dwOffset);
    }

    return sID;
} /* end of ET9_CP_LookupUnicode() */

/*------------------------------------------------------------------------
 *
 *  Function    : ET9_CP_LookupID
 *
 *  Synopsis    : It converts a PID to SIDs or SID to PIDs.
 *                NOTE: Lookup tables must be present (do not call when using phonetic only ldb)
 *
 *     Input    : pET9CPLingInfo = Pointer to Chinese XT9 LingInfo structure.
 *                wID = the input PID or SID.
 *                bBufSize = number of IDs that pwIDBuf can hold
 *                bType = ET9_CP_Lookup_PIDToSID or ET9_CP_Lookup_SIDToPID
 *
 *    Output    : pwIDBuf = the buffer to store the result IDs
 *
 *    Return    : the actual number of alt IDs stored into pwIDBuf (including most common)
 *
 *-----------------------------------------------------------------------*/
ET9U8 ET9FARCALL ET9_CP_LookupID(ET9CPLingInfo *pET9CPLingInfo,
                             ET9U16 *pwIDBuf,
                             ET9U16 wID,
                             ET9U8 bBufSize,
                             ET9U8 bType)
{
#define ET9_CP_ID_MASK    0x7FFF
    /* PID to SID Lookup table structure:
       Total# of PIDs (ET9U16);
       main entry: [axxx xxxx xxxx xxxx]
                   a == 1 --> multiple IDs, x's is offset in alt ID area.
                   a == 0 --> single ID, x's is the ID.
       ...
       sub-entry in alt ID area: [bccc cccc cccc cccc]...
                                 b == 1 --> last alt ID for this entry
                                 b == 0 --> more alt IDs.
     */
    ET9U32 dwTableOffset, dwReadOffset;
    ET9U16 wRead;
    ET9U8 bAltCount = 0;
    ET9Assert(pwIDBuf && (bBufSize >= 1));

    dwTableOffset = bType ? pET9CPLingInfo->CommonInfo.sOffsets.dwSIDToPIDOffset : pET9CPLingInfo->CommonInfo.sOffsets.dwPIDToSIDOffset;
    ET9Assert(dwTableOffset);
#ifdef ET9_DEBUG
    wRead = ET9_CP_LdbReadWord(pET9CPLingInfo, dwTableOffset); /* read total ID count in the table */
    ET9Assert(wID < wRead);
#endif
    /* set offset to the desired main entry */
    dwReadOffset = dwTableOffset + ((ET9U32)wID + 1) * sizeof(ET9U16);
    wRead = ET9_CP_LdbReadWord(pET9CPLingInfo, dwReadOffset);
    if (ET9_CP_NOMATCH == wRead) {
        return 0; /* when a Unicode in the component range is not supported, it has PID but no SID, indicated by 0xFFFF in the table */
    }
    if (wRead & ((ET9U16)~ET9_CP_ID_MASK)) { /* has multiple IDs */
        ET9U16 wIDCount;
        wRead &= (ET9U16)ET9_CP_ID_MASK; /* relative offset in alt ID area */
        wIDCount = ET9_CP_LdbReadWord(pET9CPLingInfo, dwTableOffset); /* read total ID count */
        /* set offset to desired alt ID sub-entry */
        dwReadOffset = dwTableOffset + ((ET9U32)wIDCount + 1 + wRead) * sizeof(ET9U16);
        do {
            wRead = ET9_CP_LdbReadWord(pET9CPLingInfo, dwReadOffset);
            dwReadOffset += 2; /* sizeof(ET9U16) */
            *pwIDBuf++ = (ET9U16)(wRead & ET9_CP_ID_MASK);
            bAltCount++;
        } while (!(wRead & (ET9U16)(~ET9_CP_ID_MASK)) && (bAltCount < bBufSize));
        ET9Assert(bAltCount >= 2 || bBufSize < 2); /* at least 2 alt IDs unless buf size < 2 */
    }
    else { /* only has 1 ID */
        *pwIDBuf = wRead;
        bAltCount++;
    }
    return bAltCount;
} /* END ET9_CP_LookupID() */

ET9BOOL ET9FARCALL ET9_CP_Is_Comp_Sid(ET9CPLingInfo * pLing, ET9U16 wSid)
{
    ET9U16 wPID;
    if (ET9_CP_LookupID(pLing, &wPID, wSid, 1, ET9_CP_Lookup_SIDToPID) > 0
        && ET9_CP_IS_COMP_PID(&pLing->CommonInfo, wPID) )
    {
        return 1;
    }
    return 0;
}

void ET9FARCALL ET9_CP_LoadSylTable(ET9CPLingInfo *pET9CPLingInfo,
                                    ET9U32 *pdwPinyinBinTable,
                                    ET9U32 *pdwBpmfBinTable,
                                    ET9U16 *pwSylPIDTable)
{
    ET9_CP_LdbOffsets *psLdbOffset = &(pET9CPLingInfo->CommonInfo.sOffsets);
    ET9U32 dwOffset;
    ET9UINT nSylBinSize, nSylIndexSize, nSylCount, i;

    nSylCount = 0; /* to avoid warnings */
    dwOffset = 0;

    nSylIndexSize = ET9_CP_IsLdbDualPhonetic(pET9CPLingInfo) ? 1 : 0;
    /* load Pinyin Bin table */
    if (psLdbOffset->dwPinyinLDBOffset) { /* Pinyin Syl to PID table */
        nSylBinSize = 3;
        nSylCount = pET9CPLingInfo->CommonInfo.wPinyinSylCount;
        dwOffset = psLdbOffset->dwSyllablePIDOffset;
        for (i = 0; i < nSylCount; i++) {
            *pdwPinyinBinTable++ = ET9_CP_ReadSylBin(pET9CPLingInfo, dwOffset, (nSylBinSize + nSylIndexSize));
            dwOffset += (nSylBinSize + nSylIndexSize);
        }
        /* The offset is now at part 2 of the syllable table: starting PID of each syllable
           Note: in Dual Ldb, this is invalid offset and will be overwritten later by BPMF, so BPMF Bin must be loaded after Pinyin Bin */
    }
    /* load Bpmf Bin table */
    if (psLdbOffset->dwBpmfLDBOffset) { /* BPMF Syl to BID table */
        nSylBinSize = 2;
        nSylCount = pET9CPLingInfo->CommonInfo.wBpmfSylCount;
        dwOffset = psLdbOffset->dwSyllableBIDOffset; /* the Bin table */
        for (i = 0; i < nSylCount; i++) {
            *pdwBpmfBinTable++ = ET9_CP_ReadSylBin(pET9CPLingInfo, dwOffset, (nSylBinSize + nSylIndexSize));
            dwOffset += (nSylBinSize + nSylIndexSize);
        }
        /* The offset is now at part 2 of the syllable table: starting PID of each syllable */
    }
    ET9Assert(dwOffset && nSylCount);
    /* load Syl PID table: either from BPMF syl table or (from Pinyin syl table, if Ldb is Pinyin-only) */
    nSylCount++; /* an extra PID as the end PID of the last syl */
    for (i = 0; i < nSylCount; i++) {
        *pwSylPIDTable++ = ET9_CP_LdbReadWord(pET9CPLingInfo, dwOffset);
        dwOffset += sizeof(ET9U16);
    }
}

/*  Get all alt PID of each char in a Unicode phrase.

    Return: 1 if succeed,
            0 if any of the chars has no PID.
*/
ET9BOOL ET9FARCALL ET9_CP_UniPhraseToAltPID(ET9CPLingInfo *pLing,
                                                const ET9CPPhrase *pPhrase,
                                                ET9U16 *pwPID,
                                                const ET9U8 bMaxCol)
{
    ET9_CP_SpellLookupCache *psLookupCache;
    ET9U8 i, j;

    /* validate parameters */
    ET9Assert(!ET9_CP_IS_LINGINFO_NOINIT(pLing) );
    ET9Assert( !(NULL == pPhrase || 0 == pPhrase->bLen || pPhrase->bLen > ET9CPMAXPHRASESIZE) );
    ET9Assert(bMaxCol <= ET9_CP_MAX_ALT_SYLLABLE);

    psLookupCache = &pLing->Private.PPrivate.sLookupCache;

    for (i = 0; i < pPhrase->bLen; i++) {
        pwPID[i * bMaxCol + 0] = ET9_CP_UnicodeToPID(pLing, pPhrase->pSymbs[i], 0);
        if (ET9_CP_NOMATCH == pwPID[i * bMaxCol + 0]) {
            return 0; /* a Unicode has no PID, return FALSE */
        }
        /* use the cache to populate pwPID[i][] */
        for (j = 1; j < psLookupCache->bNumAlternates; j++) {
            pwPID[i * bMaxCol + j] = psLookupCache->awPIDs[j];
        }
        if (j < ET9_CP_MAX_ALT_SYLLABLE) {
            pwPID[i * bMaxCol + j] = ET9_CP_NOMATCH; /* indicate end of alt PID for pwPID[i][] */
        }
    }
    return 1;
}
/* ----------------------------------< eof >--------------------------------- */
