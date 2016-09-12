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
;**     FileName: et9cpinit.c                                                 **
;**                                                                           **
;**  Description: Chinese Phrase Text Input initialization module.            **
;**               Conforming to the development version of Chinese XT9.       **
;**                                                                           **
;*******************************************************************************
;******************************************************************************/

#include "et9cpapi.h"
#include "et9cpldb.h"
#include "et9cpinit.h"
#include "et9cpname.h"
#include "et9cpwdls.h"
#include "et9cpmisc.h"
#include "et9cppbuf.h"
#include "et9cpsys.h"

#include "et9cstrie.h"

/*---------------------------------------------------------------------------
 *
 *   Function: ET9_CP_CheckCompatOffset
 *
 *   Synopsis: Check the compatibility index offset between LDB and core.
 *
 *      Input: wLDBCompatID          = LDB Compatibility index.
 *             wCoreCompatBase       = Core compatibility index base.
 *             wCoreCompatOffsetMask = Core compatibility index offset mask.
 *
 *     Return: ET9STATUS_NONE on success, ET9STATUS_DB_CORE_INCOMP on failure.
 *
 *---------------------------------------------------------------------------*/
static ET9STATUS ET9LOCALCALL ET9_CP_CheckCompatOffset(ET9U16 wLDBCompatID,
                                                       ET9U16 wCoreCompatBase,
                                                       ET9U16 wCoreCompatOffsetMask)
{
    ET9U16  wLDBCompatOffset;

    if (wLDBCompatID < wCoreCompatBase) {
        return ET9STATUS_DB_CORE_INCOMP;
    }

    /* Compute LDB Compatibility index offset */
    wLDBCompatOffset = (ET9U16)(wLDBCompatID - wCoreCompatBase);

    if (wLDBCompatOffset > ET9_CP_MAX_COMPAT_ID_OFFSET) {
        return ET9STATUS_DB_CORE_INCOMP;
    }

    /*
     * If the compat idx from LDB is equal to the compat idx base defined in core,
     * the offset is 0. Therefore, they are compatible. Otherwise, compare the offset.
     */
    if (0 != wLDBCompatOffset) {
        /* Comvert the offset to bit mask. */
        wLDBCompatOffset = ET9MASKOFFSET(wLDBCompatOffset);
        if (!(wCoreCompatOffsetMask & wLDBCompatOffset)) {
            return ET9STATUS_DB_CORE_INCOMP;
        }
    }

    return ET9STATUS_NONE;
}

static void ET9LOCALCALL ConversionTableInit(ET9CPLingInfo *pLing)
{
    ET9U32 dwReadOffset;
    ET9_CP_ConversionTable *pConvTable;

    pConvTable = &pLing->ConvTable;
    dwReadOffset = pLing->CommonInfo.sOffsets.dwConvTableOffset;

    if (dwReadOffset) {

        pConvTable->wSimpTradCount = ET9_CP_LdbReadWord(pLing, dwReadOffset);
        dwReadOffset += 2; /* sizeof(ET9U16) */
        pConvTable->wTradSimpCount = ET9_CP_LdbReadWord(pLing, dwReadOffset);
        dwReadOffset += 2;

        pConvTable->dwSimpTradOffset = dwReadOffset;
        pConvTable->dwTradSimpOffset = dwReadOffset + (ET9U32)(pConvTable->wSimpTradCount) * 4; /* (2 * sizeof(ET9U16)) */
    }
    else {
        pConvTable->wSimpTradCount = 0;
        pConvTable->wTradSimpCount = 0;
        pConvTable->dwSimpTradOffset = 0;
        pConvTable->dwTradSimpOffset = 0;
    }
}

/*------------------------------------------------------------------------
*
*  Function        : ET9_CP_CheckLdb
*
*  Synopsis        : Check the Chinese XT9 linguistics database.
*
*  Input           : pET9CPgFieldInfo - XT9 field info structure.
*
*  Return          : ET9STATUS_NONE on success, otherwise return XT9 error code.
*
*-----------------------------------------------------------------------*/
ET9STATUS ET9FARCALL ET9_CP_CheckLdb(ET9CPLingInfo * pET9CPLingInfo)
{
    ET9STATUS eStatus = ET9STATUS_NONE;
    ET9_CP_CommonInfo *psCommon = &pET9CPLingInfo->CommonInfo;
    ET9U32 dwReadOffset;
    ET9U16 wLDBHeaderSize;

#ifdef ET9_DIRECT_LDB_ACCESS
    eStatus = ET9_CP_InitDirectLdbAccess(pET9CPLingInfo);
    if (ET9STATUS_NONE != eStatus) {
        return eStatus;
    }
#endif
    {
        dwReadOffset = ET9_CP_HEADER_SIZE_OFFSET;
        wLDBHeaderSize = ET9_CP_LdbReadWord(pET9CPLingInfo, dwReadOffset);
    }

    /* Check the database type */
    {
        ET9U8 bDatabaseType;

        dwReadOffset = ET9_CP_DB_TYPE_OFFSET;
        bDatabaseType = ET9_CP_LdbReadByte(pET9CPLingInfo, dwReadOffset);
        if (0 == bDatabaseType) {
            /* could not read database */
            return ET9STATUS_READ_DB_FAIL;
        }
        if ((ET9U8)(ET9_CP_DBTYPE) != bDatabaseType) {
            /* Database type mismatch */
            return ET9STATUS_INVALID_DB_TYPE;
        }
    }

    /*
    * Check the compatibility index base + offset.
    * The compatibility index in the LDB must be one of the index
    * valuses supported by the core.
    * Note: the compatibility index must be incremented whenever
    *       the database layout is changed.
    */
    dwReadOffset = ET9_CP_CORE_COMPAT_ID_OFFSET;
    eStatus = ET9_CP_CheckCompatOffset(ET9_CP_LdbReadWord(pET9CPLingInfo, dwReadOffset),
                                       (ET9U16)(ET9_CP_COMPAT_ID_BASE),
                                       (ET9U16)(ET9_CP_COMPAT_ID_OFFSET));
    if (ET9STATUS_NONE != eStatus) {
        return eStatus;
    }
    /*
    * Check the OEM ID.
    * Get the OEM ID from Latin LDB. They are not located at two continuous
    * bytes. Therefore, get the high byte first and then the low byte.
    */
    {
        ET9U16 wOEMIDinLdb;
        dwReadOffset = ET9_CP_OEMID_OFFSET;
        wOEMIDinLdb = ET9_CP_LdbReadWord(pET9CPLingInfo, dwReadOffset);
        if ((ET9U16)(ET9OEMID) != wOEMIDinLdb) {
            return ET9STATUS_WRONG_OEMID;
        }
    }

    /* Check primary and secondary language numbers. */
    {
        ET9U16 wLangID;
        dwReadOffset = ET9_CP_LANGUAGE_ID_OFFSET;
        /* Primary language ID */
        wLangID = (ET9U16)(ET9_CP_LdbReadByte(pET9CPLingInfo, dwReadOffset));
        dwReadOffset++;
        /* Secondary language ID */
        wLangID = (ET9U16)(wLangID | ((ET9U16)(ET9_CP_LdbReadByte(pET9CPLingInfo, dwReadOffset)) << 8));
        if (wLangID != pET9CPLingInfo->wLdbNum) {
            return ET9STATUS_LDB_ID_ERROR;
        }
    }

    /* Check the character encoding and endianess */
    {
        ET9U8    bSymbolBytes;

        dwReadOffset = ET9_CP_SYMBOL_BYTE_OFFSET;
        bSymbolBytes = ET9_CP_LdbReadByte(pET9CPLingInfo, dwReadOffset);
        if ((ET9U8)(ET9SYMBOLWIDTH) != bSymbolBytes) {
            return ET9STATUS_LDB_VERSION_ERROR;
        }
    }

    /* get each database module addresses */
    dwReadOffset = ET9_CP_BPMF_MOD_OFFSET;
    psCommon->sOffsets.dwBpmfLDBOffset = ET9_CP_LdbReadDWord(pET9CPLingInfo, dwReadOffset);

    dwReadOffset = ET9_CP_PINYIN_MOD_OFFSET;
    psCommon->sOffsets.dwPinyinLDBOffset = ET9_CP_LdbReadDWord(pET9CPLingInfo, dwReadOffset);

    dwReadOffset = ET9_CP_STROKE_MOD_OFFSET;
    psCommon->sOffsets.dwStrokeLDBOffset = ET9_CP_LdbReadDWord(pET9CPLingInfo, dwReadOffset);

    dwReadOffset = ET9_CP_PID_TO_SID_OFFSET;
    psCommon->sOffsets.dwPIDToSIDOffset = ET9_CP_LdbReadDWord(pET9CPLingInfo, dwReadOffset);

    dwReadOffset = ET9_CP_SID_TO_PID_OFFSET;
    psCommon->sOffsets.dwSIDToPIDOffset = ET9_CP_LdbReadDWord(pET9CPLingInfo, dwReadOffset);

    dwReadOffset = ET9_CP_PID_TO_UNI_OFFSET;
    psCommon->sOffsets.dwPIDToUnicodeOffset = ET9_CP_LdbReadDWord(pET9CPLingInfo, dwReadOffset);

    dwReadOffset = ET9_CP_UNI_SORTED_PID_OFFSET;
    psCommon->sOffsets.dwUnicodeSortedPIDOffset = ET9_CP_LdbReadDWord(pET9CPLingInfo, dwReadOffset);

    dwReadOffset = ET9_CP_PID_TO_FREQ_OFFSET;
    psCommon->sOffsets.dwFrequencyOffset = ET9_CP_LdbReadDWord(pET9CPLingInfo, dwReadOffset);

    dwReadOffset = ET9_CP_C_COMMONCHAR_OFFSET;
    psCommon->sOffsets.dwContextCommonCharOffset = ET9_CP_LdbReadDWord(pET9CPLingInfo, dwReadOffset);

    dwReadOffset = ET9_CP_NC_COMMONCHAR_OFFSET;
    psCommon->sOffsets.dwNoContextCommonCharOffset = ET9_CP_LdbReadDWord(pET9CPLingInfo, dwReadOffset);

    dwReadOffset = ET9_CP_SMART_PUNCT_OFFSET;
    psCommon->sOffsets.dwSmartPunctOffset = ET9_CP_LdbReadDWord(pET9CPLingInfo, dwReadOffset);

    dwReadOffset = ET9_CP_NAME_CHAR_OFFSET;
    psCommon->sOffsets.dwNameTableOffset = ET9_CP_LdbReadDWord(pET9CPLingInfo, dwReadOffset);

    if (wLDBHeaderSize > ET9_CP_SBI_MOD_OFFSET) {
        dwReadOffset = ET9_CP_SBI_MOD_OFFSET;
        psCommon->sOffsets.dwSBITrieOffset = ET9_CP_LdbReadDWord(pET9CPLingInfo, dwReadOffset);
    }
    else {
        psCommon->sOffsets.dwSBITrieOffset = 0;
    }

    if (wLDBHeaderSize > ET9_CP_CANGJIE_MOD_OFFSET) {
        dwReadOffset = ET9_CP_CANGJIE_MOD_OFFSET;
        psCommon->sOffsets.dwCangJieOffset = ET9_CP_LdbReadDWord(pET9CPLingInfo, dwReadOffset);
    }
    else {
        psCommon->sOffsets.dwCangJieOffset = 0;
    }

    if (wLDBHeaderSize > ET9_CP_SIMP_TRAD_OFFSET) {
        dwReadOffset = ET9_CP_SIMP_TRAD_OFFSET;
        psCommon->sOffsets.dwConvTableOffset = ET9_CP_LdbReadDWord(pET9CPLingInfo, dwReadOffset);
    }
    else {
        psCommon->sOffsets.dwConvTableOffset = 0;
    }

    ConversionTableInit(pET9CPLingInfo);

    psCommon->sOffsets.dwToneOffset = 0;
    psCommon->sOffsets.dwSyllablePIDOffset = 0;
    psCommon->sOffsets.dwSyllableBIDOffset = 0;
    /* BPMF Chinese XT9 Module */
    if (psCommon->sOffsets.dwBpmfLDBOffset) {

        /* get absolute offset of syllable to PID table */
        dwReadOffset = psCommon->sOffsets.dwBpmfLDBOffset;
        psCommon->sOffsets.dwSyllableBIDOffset = psCommon->sOffsets.dwBpmfLDBOffset + ET9_CP_LdbReadWord(pET9CPLingInfo, dwReadOffset);

        /* get absolute offset of BID to tone table */
        dwReadOffset += 2; /* sizeof(ET9U16) */
        psCommon->sOffsets.dwToneOffset = psCommon->sOffsets.dwBpmfLDBOffset + ET9_CP_LdbReadWord(pET9CPLingInfo, dwReadOffset);

        /* get the syllable - BID table header data */
        dwReadOffset = psCommon->sOffsets.dwSyllableBIDOffset;
        psCommon->wBpmfSylCount = ET9_CP_LdbReadWord(pET9CPLingInfo, dwReadOffset); /* syllable count */
        psCommon->sOffsets.dwSyllableBIDOffset += 2; /* adjust offset to start of table, skipping sizeof(ET9U16) to after the syllable count */
    }

    /* Pinyin Chinese XT9 Module */
    if (psCommon->sOffsets.dwPinyinLDBOffset) {

        /* get absolute offset of syllable to PID table */
        dwReadOffset = psCommon->sOffsets.dwPinyinLDBOffset;
        psCommon->sOffsets.dwSyllablePIDOffset = psCommon->sOffsets.dwPinyinLDBOffset + ET9_CP_LdbReadWord(pET9CPLingInfo, dwReadOffset);

        if (0 == psCommon->sOffsets.dwToneOffset) { /* haven't been set by BPMF module */
            /* get absolute offset of PID to tone table */
            dwReadOffset += 2; /* sizeof(ET9U16) */
            psCommon->sOffsets.dwToneOffset = psCommon->sOffsets.dwPinyinLDBOffset + ET9_CP_LdbReadWord(pET9CPLingInfo, dwReadOffset);
        }
        /* get the syllable - PID table header data */
        dwReadOffset = psCommon->sOffsets.dwSyllablePIDOffset;
        psCommon->wPinyinSylCount = ET9_CP_LdbReadWord(pET9CPLingInfo, dwReadOffset); /* syllable count */
        psCommon->sOffsets.dwSyllablePIDOffset += 2; /* adjust offset to start of table, skipping sizeof(ET9U16) to after the syllable count */
    }

    /* load Syl table into private buf for faster access */
    ET9_CP_LoadSylTable(pET9CPLingInfo,
                        pET9CPLingInfo->Private.PPrivate.adwPinyinBinTable,
                        pET9CPLingInfo->Private.PPrivate.adwBpmfBinTable,
                        pET9CPLingInfo->Private.PPrivate.awSylPidTable);

    {
        ET9U16 wSymCount;
        ET9U16 wMuteCharCount;

        wSymCount = (ET9U16)ET9_CP_LdbReadByte(pET9CPLingInfo, ET9_CP_SYM_COUNT_OFFSET);
        wMuteCharCount = ET9_CP_LdbReadWord(pET9CPLingInfo, ET9_CP_MUTE_CHAR_COUNT_OFFSET);

        dwReadOffset = psCommon->sOffsets.dwPIDToUnicodeOffset + 2 * sizeof(ET9U16); /* skip first and last component Unicode */
        psCommon->w1stComponentPID = ET9_CP_LdbReadWord(pET9CPLingInfo, dwReadOffset);
        psCommon->w1stMuteCharPID = (ET9U16)(psCommon->w1stComponentPID - wMuteCharCount);
        psCommon->w1stSymbolPID = (ET9U16)(psCommon->w1stMuteCharPID - wSymCount);
    }

    /* setup Phrase module cache */
    {
        ET9U8 bBlockIndex, bPhraseBlockCount;

        dwReadOffset = ET9_CP_PHRASE_MOD_OFFSET;
        psCommon->sOffsets.dwWordLDBOffset = ET9_CP_LdbReadDWord(pET9CPLingInfo, dwReadOffset);

        dwReadOffset = psCommon->sOffsets.dwWordLDBOffset + ET9_CP_PHRASE_GROUP_ADDR_TABLE_OFFSET;
        psCommon->sOffsets.dwPhraseGroupAddrTableOffset = psCommon->sOffsets.dwWordLDBOffset + ET9_CP_LdbReadWord(pET9CPLingInfo, dwReadOffset);
        dwReadOffset += 2;
        psCommon->sOffsets.dwPhraseDataOffset = psCommon->sOffsets.dwWordLDBOffset + ET9_CP_LdbReadWord(pET9CPLingInfo, dwReadOffset);
        dwReadOffset += 2;
        bPhraseBlockCount = ET9_CP_LdbReadByte(pET9CPLingInfo, dwReadOffset);
        ET9Assert(bPhraseBlockCount > 0 && bPhraseBlockCount <= ET9_CP_MAX_PHRASE_DATA_BLOCK_COUNT);
        dwReadOffset++;
        for (bBlockIndex = 0; bBlockIndex < bPhraseBlockCount - 1; bBlockIndex++) {
            psCommon->pwMaxFirstPIDInBlock[bBlockIndex] = (ET9U16)(ET9_CP_LdbReadWord(pET9CPLingInfo, dwReadOffset) - 1); /* inclusive */
            dwReadOffset += 2;
        }
        psCommon->pwMaxFirstPIDInBlock[bBlockIndex] = ET9_CP_NORMAL_PID_COUNT(psCommon); /* last block end is exclusive */
    }

    /* default to phonetic mode */
    if (psCommon->sOffsets.dwBpmfLDBOffset) {
        pET9CPLingInfo->eMode = ET9CPMODE_BPMF;
    }
    else if (psCommon->sOffsets.dwPinyinLDBOffset) {
        pET9CPLingInfo->eMode = ET9CPMODE_PINYIN;
    }
    else if (psCommon->sOffsets.dwStrokeLDBOffset) {
        pET9CPLingInfo->eMode = ET9CPMODE_STROKE;
    }
    else {
        eStatus = ET9STATUS_LDB_VERSION_ERROR;
    }

    /* All checks pass. Return success. */
    return eStatus;
}   /* end of ET9_CP_CheckLdb */

#ifdef ET9CHECKCOMPILE
/** Check if the integration layer and core have the same size on the important data types.
 *
 * Example code for how to use this function:
 <pre>
    ET9U32 dwError;
    ET9U8 bET9U8Size = sizeof(ET9U8);
    ET9U8 bET9U16Size = sizeof(ET9U16);
    ET9U8 bET9U32Size = sizeof(ET9U32);
    ET9U8 bET9UINTSize = sizeof(ET9UINT);
    ET9U8 bET9S8Size = sizeof(ET9S8);
    ET9U8 bET9S16Size = sizeof(ET9S16);
    ET9U8 bET9S32Size = sizeof(ET9S32);
    ET9U8 bET9INTSize = sizeof(ET9INT);
    ET9U8 bET9SymbSize = sizeof(ET9SYMB);
    ET9U8 bET9BOOLSize = sizeof(ET9BOOL);
    ET9U8 bET9FARDATASize = sizeof(void ET9FARDATA *);
    ET9U8 bET9FARCALLSize = sizeof(void ET9FARCALL *);
    ET9U8 bET9LOCALCALLSize = sizeof(void ET9LOCALCALL *);
    ET9U8 bVoidPtrSize = sizeof(void *);
    ET9U16 wET9SymbInfoSize = sizeof(ET9SymbInfo);
    ET9UINT nET9WordSymbInfoSize = sizeof(ET9WordSymbInfo);
    ET9UINT nET9CPLingInfoSize = sizeof(ET9CPLingInfo);

    dwError = ET9CPCheckCompileParameters(&bET9U8Size,
                                         &bET9U16Size,
                                         &bET9U32Size,
                                         &bET9UINTSize,
                                         &bET9S8Size,
                                         &bET9S16Size,
                                         &bET9S32Size,
                                         &bET9INTSize,
                                         &bET9SymbSize,
                                         &bET9BOOLSize,
                                         &bET9FARDATASize,
                                         &bET9FARCALLSize,
                                         &bET9LOCALCALLSize,
                                         &bVoidPtrSize,
                                         &wET9SymbInfoSize,
                                         &nET9WordSymbInfoSize,
                                         &nET9CPLingInfoSize);
</pre>
    if (dwError & ET9WRONGSIZE_ET9UINT) != 0, then the integration layer's sizeof(ET9UINT) is not the same as core's sizeof(ET9UINT).<br> The core's sizeof(ET9UINT) will be written in bET9UINTSize on return.<p>

 * @param pbET9U8               (in, out)pass in integration layer's sizeof(ET9U8), it will becomes core's sizeof(ET9U8) upon return
 * @param pbET9U16              (in, out)pass in integration layer's sizeof(ET9U16), it will becomes core's sizeof(ET9U16) upon return
 * @param pbET9U32              (in, out)pass in integration layer's sizeof(ET9U32), it will becomes core's sizeof(ET9U32) upon return
 * @param pbET9UINT             (in, out)pass in integration layer's sizeof(ET9UINT), it will becomes core's sizeof(ET9UINT) upon return
 * @param pbET9S8               (in, out)pass in integration layer's sizeof(ET9S8), it will becomes core's sizeof(ET9S8) upon return
 * @param pbET9S16              (in, out)pass in integration layer's sizeof(ET9S16), it will becomes core's sizeof(ET9S16) upon return
 * @param pbET9S32              (in, out)pass in integration layer's sizeof(ET9S32), it will becomes core's sizeof(ET9S32) upon return
 * @param pbET9INT              (in, out)pass in integration layer's sizeof(ET9INT), it will becomes core's sizeof(ET9INT) upon return
 * @param pbET9SYMB             (in, out)pass in integration layer's sizeof(ET9SYMB), it will becomes core's sizeof(ET9SYMB) upon return
 * @param pbET9BOOL             (in, out)pass in integration layer's sizeof(ET9BOOL), it will becomes core's sizeof(ET9BOOL) upon return
 * @param pbET9FARDATA          (in, out)pass in integration layer's sizeof(void ET9FARDATA *), it will becomes core's sizeof(void ET9FARDATA *) upon return
 * @param pbET9FARCALL          (in, out)pass in integration layer's sizeof(void ET9FARCALL *), it will becomes core's sizeof(void ET9FARCALL *) upon return
 * @param pbET9LOCALCALL        (in, out)pass in integration layer's sizeof(void ET9LOCALCALL *), it will becomes core's sizeof(void ET9LOCALCALL *)  upon return
 * @param pbVoidPtr             (in, out)pass in integration layer's sizeof(void *), it will becomes core's sizeof(void *) upon return
 * @param pwET9SymbInfo         (in, out)pass in integration layer's sizeof(ET9SymbInfo), it will becomes core's sizeof(ET9SymbInfo) upon return
 * @param pnET9WordSymbInfo     (in, out)pass in integration layer's sizeof(ET9WordSymbInfo), it will becomes core's sizeof(ET9WordSymbInfo) upon return
 * @param pnET9CPLingInfo       (in, out)pass in integration layer's sizeof(ET9CPLingInfo), it will becomes core's sizeof(ET9CPLingInfo) upon return
 *
 * @return  0 on passed. Othwewise failed:
 * @return  ET9NULL_POINTERS  if some argument pointer is NULL
 * @return other value ( neither 0 nor ET9NULL_POINTERS ): one or more of the following bit flag:
 * - ET9WRONGSIZE_ET9U8               if integration layer and core have different size for ET9U8
 * - ET9WRONGSIZE_ET9U16              if integration layer and core have different size for ET9U16
 * - ET9WRONGSIZE_ET9U32              if integration layer and core have different size for ET9U32
 * - ET9WRONGSIZE_ET9UINT             if integration layer and core have different size for ET9UINT
 * - ET9WRONGSIZE_ET9S8               if integration layer and core have different size for ET9S8
 * - ET9WRONGSIZE_ET9S16              if integration layer and core have different size for ET9S16
 * - ET9WRONGSIZE_ET9S32              if integration layer and core have different size for ET9S32
 * - ET9WRONGSIZE_ET9INT              if integration layer and core have different size for ET9INT
 * - ET9WRONGSIZE_ET9SYMB             if integration layer and core have different size for ET9SYMB
 * - ET9WRONGSIZE_ET9BOOL             if integration layer and core have different size for ET9BOOL
 * - ET9WRONGSIZE_ET9FARDATA          if integration layer and core have different size for ET9FARDATA
 * - ET9WRONGSIZE_ET9FARCALL          if integration layer and core have different size for ET9FARCALL
 * - ET9WRONGSIZE_ET9LOCALCALL        if integration layer and core have different size for ET9LOCALCALL
 * - ET9WRONGSIZE_VOIDPOINTER         if integration layer and core have different size for VOIDPOINTER
 * - ET9WRONGSIZE_ET9SYMBINFO         if integration layer and core have different size for ET9SYMBINFO
 * - ET9WRONGSIZE_ET9WORDSYMBINFO     if integration layer and core have different size for ET9WORDSYMBINFO
 * - ET9WRONGSIZE_ET9CPLINGINFO       if integration layer and core have different size for ET9CPLINGINFO
 *
 */
ET9U32 ET9FARCALL ET9CPCheckCompileParameters(ET9U8   *pbET9U8,
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
                                             ET9UINT  *pnET9WordSymbInfo,
                                             ET9UINT  *pnET9CPLingInfo)
{
    ET9U32 dwError;

    dwError = ET9_CheckCompileParameters(pbET9U8,
                                         pbET9U16,
                                         pbET9U32,
                                         pbET9UINT,
                                         pbET9S8,
                                         pbET9S16,
                                         pbET9S32,
                                         pbET9INT,
                                         pbET9SYMB,
                                         pbET9BOOL,
                                         pbET9FARDATA,
                                         pbET9FARCALL,
                                         pbET9LOCALCALL,
                                         pbVoidPtr,
                                         pwET9SymbInfo,
                                         pnET9WordSymbInfo);

    if ( *pnET9CPLingInfo != sizeof(ET9CPLingInfo) ) {
        *pnET9CPLingInfo = sizeof(ET9CPLingInfo);
        dwError |= (1L << ET9WRONGSIZE_ET9CPLINGINFO);
    }

    return dwError;
}
#endif

/* ----------------------------------< eof >--------------------------------- */

