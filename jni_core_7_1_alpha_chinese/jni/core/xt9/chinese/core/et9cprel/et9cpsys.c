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
;**     FileName: et9cpsys.c                                                  **
;**                                                                           **
;**  Description: Chinese XT9 system module.                                  **
;**               Conforming to the development version of Chinese XT9.       **
;**                                                                           **
;*******************************************************************************
;******************************************************************************/

#include "et9api.h"
#include "et9cpsys.h"
#include "et9cpldb.h"
#include "et9cpsldb.h"
#include "et9cpinit.h"
#include "et9cprdb.h"
#include "et9cpkey.h"
#include "et9cpsldb.h"
#include "et9cpcntx.h"
#include "et9cpname.h"
#include "et9cpwdls.h"
#include "et9cppbuf.h"
#include "et9cpspel.h"
#include "et9cptone.h"
#include "et9cpmisc.h"
#include "et9imu.h"
#include "et9misc.h"
#ifdef EVAL_BUILD
#include "__et9eval.h"
#endif
#ifdef ET9_ALPHABETIC_MODULE
#include "et9asys.h"
#include "et9amisc.h"
#endif

#include "et9cptrace.h"
#include "et9cstrie.h"
#include "et9cpcj.h"


/*---------------------------------------------------------------------------*/
/** \internal
 * Product identifier.
 */

const ET9U8 _pbXt9Chinese[] = { 'c', 'o', 'm', '.', 'n', 'u', 'a', 'n', 'c', 'e', '.', 'x', 't', '9', '.', 'c', 'h', 'i', 'n', 'e', 's', 'e', 0 };

/*---------------------------------------------------------------------------*/
/** \internal
 * Read U16 in big-endian order
 * Only for non speed-opt use.
 *
 * @param pbData         pointer to source
 *
 * @return value
 */

ET9U16 ET9FARCALL ET9_CP_ReadU16(ET9U8 const * const pbData)
{
    ET9U16 u;

    u = pbData[0];
    u <<= 8;
    u |= pbData[1];

    return u;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Read U32 in big-endian order
 * Only for non speed-opt use.
 *
 * @param pbData         pointer to source
 *
 * @return value
 */

ET9U32 ET9FARCALL ET9_CP_ReadU32(ET9U8 const * const pbData)
{
    ET9U32 u;

    ET9Assert(pbData != NULL);

    u = pbData[0];
    u <<= 8;
    u |= pbData[1];
    u <<= 8;
    u |= pbData[2];
    u <<= 8;
    u |= pbData[3];

    return u;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Write U16 in big-endian order
 * Only for non speed-opt use.
 *
 * @param pbData         pointer to target
 * @param wValue         data value
 *
 * @return none
 */

void ET9FARCALL ET9_CP_WriteU16(ET9U8 * const pbData, ET9U16 wValue)
{
    ET9Assert(pbData != NULL);

    pbData[1] = (ET9U8)(wValue & 0xff);
    wValue = (ET9U16)(wValue >> 8);
    pbData[0] = (ET9U8)(wValue & 0xff);
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Write U32 in big-endian order
 * Only for non speed-opt use.
 *
 * @param pbData         pointer to target
 * @param dwValue        data value
 *
 * @return none
 */

void ET9FARCALL ET9_CP_WriteU32(ET9U8 * const pbData, ET9U32 dwValue)
{
    ET9Assert(pbData != NULL);

    pbData[3] = (ET9U8)(dwValue & 0xff);
    dwValue = (dwValue >> 8);
    pbData[2] = (ET9U8)(dwValue & 0xff);
    dwValue = (dwValue >> 8);
    pbData[1] = (ET9U8)(dwValue & 0xff);
    dwValue = (dwValue >> 8);
    pbData[0] = (ET9U8)(dwValue & 0xff);
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Byte move.
 * Move src to des (can be overlapping).
 * This function will NOT assert on size zero.
 *
 * @param dst            to move to
 * @param src            to move from
 * @param size           size to move
 *
 * @return none
 */

void ET9FARCALL ET9_CP_ByteMove(ET9U8 *dst,
                             ET9U8 *src,
                             ET9U32 size)
{
    ET9U8 *svdst;

    ET9Assert(dst);
    ET9Assert(src);

    if (!size) {
        return;
    }

    if ((dst > src) && (dst < src + size)) {
        src += size;
        for (svdst = dst + size; size-- > 0; ) {
            *--svdst = *--src;
        }
    }
    else {
        for (svdst = dst; size-- > 0; ) {
            *svdst++ = *src++;
        }
    }
}


/** \internal
 * Memory comparison
 * This function will NOT assert on size zero.
 *
 * @param buf1           pointer to memory block to compare
 * @param buf2           pointer to memory block to compare
 * @param size           size of memory to compare
 *
 * @return 1 if buf1 is greater then buf2, -1 if buf1 is less then buf2, 0 if buf1 is equal to buf2
 */
ET9INT ET9FARCALL ET9_CP_MemCmp(const ET9U8 *buf1, const ET9U8 *buf2, ET9U32 size)
{
    while ( size )
    {
        if ( *buf1 > *buf2 )
            return 1;
        if ( *buf1 < *buf2 )
            return -1;
        buf1++;
        buf2++;
        size--;
    }
    return 0;
}

/** \internal
 * Check if WSI contains trace
 *
 * @param pET9CPLingInfo   x
 *
 * @return 1 if WSI contains trace, 0 otherwise.
 */
ET9UINT ET9FARCALL ET9_CP_InputContainsTrace(const ET9CPLingInfo * pET9CPLingInfo) {
    return pET9CPLingInfo->Base.pWordSymbInfo->bNumSymbs > 1 && _ET9HasTraceInfo(pET9CPLingInfo->Base.pWordSymbInfo);
}

static ET9INT ET9LOCALCALL AllSymbEncoded(ET9CPPhrase * pPhrase)
{
    ET9U8 b;
    ET9Assert(pPhrase != NULL);
    for (b = 0; b < pPhrase->bLen; b++) {
        if (pPhrase->pSymbs[b] == ET9_CP_NOT_ENCODED_SYMBOL) {
            return 0;
        }
    }
    return 1;
}


/** Determines whether a given unicode value is a component
 *
 *  @param pET9CPLingInfo   pointer to Chinese linguistic information structure
 *  @param wUnicode         Unicode value to be evaluated
 *
 *  @returns 1 if wUnicode is a component.
 *  @returns 0 is wUnicode is not a component or pET9CPLingInfo is not correctly initialized
 */
ET9BOOL ET9FARCALL ET9CPIsComponent(ET9CPLingInfo *pET9CPLingInfo,
                                    ET9U16 wUnicode)
{
    ET9U16 wPID;

    if (ET9_CP_IS_LINGINFO_NOINIT(pET9CPLingInfo)) {
        return 0; /* assume not component */
    }
    wPID = ET9_CP_UnicodeToPID(pET9CPLingInfo, wUnicode, 0);
    if (ET9_CP_NOMATCH == wPID) {
        return 0;
    }
    if (ET9_CP_IS_COMP_PID(&(pET9CPLingInfo->CommonInfo), wPID)) {
        ET9U16 wSID;
        ET9U8 bAltCount;
        bAltCount = ET9_CP_LookupID(pET9CPLingInfo, &wSID, wPID, 1, ET9_CP_Lookup_PIDToSID);
        return (ET9BOOL)(bAltCount > 0);
    }
    return 0;
}

/** Sets the context buffer to the given string
 *
 * Chinese XT9 core maintains a context buffer to save the context for prediction.
 * Integration layer can manage this buffer by two functions:
 * - ET9CPSetContext()
 * - ET9CPClearContext()
 *
 * @param pET9CPLingInfo     pointer to chinese information structure.
 * @param psContext          Context string
 * @param nContextLength     Length of the Context string.
 *
 * @return ET9STATUS_NONE       Succeeded
 * @return ET9STATUS_NO_INIT    pET9CPLingInfo is not properly initialized
 * @return ET9STATUS_BAD_PARAM  nContextLength != 0 but psContext == NULL
 */
ET9STATUS ET9FARCALL ET9CPSetContext(ET9CPLingInfo *pET9CPLingInfo,
                                     ET9SYMB *psContext,
                                     ET9UINT nContextLength)
{
    ET9STATUS status = ET9STATUS_NONE;
    ET9UINT n;

    ET9_CP_CHECK_LINGINFO(pET9CPLingInfo);

    if (0 == nContextLength) { /* flush context */
        status = ET9CPClearContext(pET9CPLingInfo);
    }
    else {
        ET9U16 *pwBuf;
        if (!psContext) {
            return ET9STATUS_BAD_PARAM; /* context is not flushed */
        }
        status = ET9CPClearContext(pET9CPLingInfo);
        ET9Assert(ET9STATUS_NONE == status);

        /* need this flag to distinguish between external/internal context
         * so that the two types cannot coexist in the context history buffer */
        pET9CPLingInfo->CommonInfo.bExtContext = 1;
        /* start from the last few char */
        if (nContextLength > ET9_CP_MAX_CONTEXT_HISTORY) {
            psContext = psContext + nContextLength - ET9_CP_MAX_CONTEXT_HISTORY;
            nContextLength = ET9_CP_MAX_CONTEXT_HISTORY;
        }
        /* convert Unicode to PID/SID */
        pwBuf = pET9CPLingInfo->CommonInfo.pwContextBuf;
        for (n = 0; n < nContextLength; psContext++) {
            /* This Unicode is a component, restart from next char */
            if (ET9CPIsComponent(pET9CPLingInfo, *psContext)) {
                /* ignore anything before the component */
                *pwBuf = ET9_CP_NOMATCH;
            }
            else {
                /* UID to PID */
                *pwBuf = ET9_CP_UnicodeToPID(pET9CPLingInfo, *psContext, 0);
                if (ET9CPIsModeStroke(pET9CPLingInfo) || ET9CPIsModeCangJie(pET9CPLingInfo) || ET9CPIsModeQuickCangJie(pET9CPLingInfo)) {
                    /* PID to SID, only get most common */
                    if (ET9_CP_NOMATCH != *pwBuf) {
                        ET9U8 bAltCount;
                        bAltCount = ET9_CP_LookupID(pET9CPLingInfo, pwBuf, *pwBuf, 1, ET9_CP_Lookup_PIDToSID);
                        ET9Assert(bAltCount > 0);
                    }
                }
                else if (ET9_CP_IS_MUTE_PID(&pET9CPLingInfo->CommonInfo, *pwBuf)) {
                    *pwBuf = ET9_CP_NOMATCH;
                }
            }
            /* Unicode is not a Chinese character, restart from next char */
            if (ET9_CP_NOMATCH == *pwBuf) {
                nContextLength = nContextLength - n - 1;
                n = 0;
                pwBuf = pET9CPLingInfo->CommonInfo.pwContextBuf;
                continue;
            }
            n++; pwBuf++;
        }
        for (n = 0; n < nContextLength; n++) { /* all context units are single char */
            pET9CPLingInfo->CommonInfo.pbContextLen[n] = 1;
        }
        pET9CPLingInfo->CommonInfo.pbContextLen[n] = 0; /* 0-termination */
    }
    ET9_CP_INVALIDATE_BUILD_CLEAR_CACHE(pET9CPLingInfo);
    return status;
} /* ET9CP_SetContextBuf() */

/** Retrieves the Chinese XT9 ldb version string.
 *
 *  String format is defined as follows:
 *  DB Taa.bb Lcc.dd.ee Vff.gg.hh.ii
 *
 *  where
 *          aa = Database type
 *          bb = Database version
 *          cc = Primary language ID
 *          dd = Secondary language ID
 *          ee = Symbol class
 *
 *          ff = Contents major version
 *          gg = Contents minor version
 *          hh = Contents deviation
 *          ii = Module and character set
 *
 * @param pET9CPLingInfo            pointer to chinese information structure
 * @param psLdbVerBuf               (out) buffer for the LDB version number
 * @param wBufMaxSize               maximum buffer size for the LDB version
 * @param pwBufSize                 (out) actual string length
 *
 * @return ET9STATUS_NONE             if succeeded, otherwise
 * @return ET9STATUS_NO_INIT          if pET9CPLingInfo is not initialized
 * @return ET9STATUS_INVALID_MEMORY   if some argument pointer is NULL
 * @return ET9STATUS_BUFFER_TOO_SMALL if the given buffer is too small
 *
 */
ET9STATUS ET9FARCALL ET9CPLdbGetVersion(ET9CPLingInfo * const pET9CPLingInfo,
                                        ET9SYMB       * const psLdbVerBuf,
                                        const ET9U16          wBufMaxSize,
                                        ET9U16        * const pwBufSize)
{
    ET9STATUS status;
    ET9SYMB *psDst = psLdbVerBuf;
    ET9_CP_VersionInfo sVersion;
    static const ET9U8 bTemplateStr[] = "DB Taa.bb Lcc.dd.ee Vff.gg.hh.ii";

    ET9_CP_CHECK_LINGINFO(pET9CPLingInfo);

    if (psLdbVerBuf == NULL || pwBufSize == NULL) {
        return ET9STATUS_INVALID_MEMORY;
    }
    if (wBufMaxSize < ET9MAXVERSIONSTR) {
        return ET9STATUS_BUFFER_TOO_SMALL;
    }

    /* Get the raw version bytes from the LDB */
    {
        status = ET9_CP_ReadLdbVersion(pET9CPLingInfo, &sVersion);
        if (ET9STATUS_NONE != status) {
            return status;
        }
    }
    /* Copy template string, excluding the null terminator. */
    {
        const ET9U8 *pbTmp = bTemplateStr;
        psDst = psLdbVerBuf;
        while(*pbTmp) {
            *psDst++ = (ET9SYMB)*pbTmp++;
        }
        ET9Assert((ET9UINT)(pbTmp - bTemplateStr) <= ET9MAXVERSIONSTR);
        *pwBufSize = (ET9U16)(pbTmp - bTemplateStr);
    }

    psDst = psLdbVerBuf; /* point to start of buffer */
    /* LDB type */
    psDst += 4;                                /* Skip "DB T"   */
    _ET9BinaryToHex(sVersion.bLDBType, psDst);
    psDst += 3;                                /* Skip "aa."    */

    /* LDB version */
    _ET9BinaryToHex(sVersion.bLDBLayoutVer, psDst);
    psDst += 4;                                /* Skip "bb L"   */

    /*  Primary language ID, Secondary language ID, Symbol Class */
    _ET9BinaryToHex(sVersion.bPrimID, psDst);
    psDst += 3;                                /* Skip "cc."    */
    _ET9BinaryToHex(sVersion.bSecID, psDst);
    psDst += 3;                                /* Skip "dd."    */
    _ET9BinaryToHex(sVersion.bSymClass, psDst);
    psDst += 4;                                /* Skip "ee V"   */

    /*  Contents major version, minor version, and deviation. Module and character set. */
    _ET9BinaryToHex(sVersion.bContMajVer, psDst);
    psDst += 3;                                /* Skip "ff."    */
    _ET9BinaryToHex(sVersion.bContMinVer, psDst);
    psDst += 3;                                /* Skip "gg."    */
    _ET9BinaryToHex(sVersion.bContDev, psDst);
    psDst += 3;                                /* Skip "hh."    */
    _ET9BinaryToHex(sVersion.bModuleCharSet, psDst);
    psDst += 2;                                /* Skip "ii"     */

    return ET9STATUS_NONE;
}

/** Initializes an XT9 Chinese LDB.
 *
 * @param  pET9CPLingInfo     pointer to chinese information structure
 * @param  wLdbNum            the LDB identifying number, it must be ET9PLIDChineseTraditional OR  ET9PLIDChineseSimplified OR ET9PLIDChineseHongkong
 * @param  ET9CPLdbReadData   callback function used to read data from the LDB
 *
 * @return ET9STATUS_NONE              Succeeded
 * @return ET9STATUS_NO_INIT           pET9CPLingInfo is not properly initialized
 * @return ET9STATUS_LDB_VERSION_ERROR LDB not supported
 * @return ET9STATUS_BAD_PARAM         Callback function is NULL
 * @return ET9STATUS_READ_DB_FAIL      Failed to read LDB
 * @return ET9STATUS_INVALID_DB_TYPE   LDB type is incorrect
 * @return ET9STATUS_DB_CORE_INCOMP    Core and LDB not compatible
 * @return ET9STATUS_WRONG_OEMID       OEMID wrong
 * @return ET9STATUS_LDB_ID_ERROR      LDB's LangId is wrong
 * @return ET9STATUS_NO_RUDB           LDB and RUDB is not compatible
*/
ET9STATUS ET9FARCALL ET9CPLdbInit(ET9CPLingInfo * const    pET9CPLingInfo,
                                  const ET9U16              wLdbNum,
                                  const ET9CPDBREADCALLBACK ET9CPLdbReadData)
{
    ET9STATUS    mStatus = ET9STATUS_NONE;
    ET9_CP_CommonInfo *pCommon;

    if (!pET9CPLingInfo ||
        (pET9CPLingInfo->Private.wInfoInitOK != ET9GOODSETUP))
    {
        return ET9STATUS_NO_INIT;
    }

    if (ET9PLIDChineseTraditional != wLdbNum && ET9PLIDChineseSimplified != wLdbNum && ET9PLIDChineseHongkong != wLdbNum)
    {
        return ET9STATUS_LDB_VERSION_ERROR;    /* LDB not supported */
    }
    if (ET9CPLdbReadData == NULL) {
        return ET9STATUS_BAD_PARAM;
    }
    else {
        pET9CPLingInfo->pLdbReadData = ET9CPLdbReadData;
    }

#ifdef EVAL_BUILD
    _ET9Eval_StartTrackingUsage(&pET9CPLingInfo->Base);
#endif

    pCommon = &(pET9CPLingInfo->CommonInfo);

    pET9CPLingInfo->wLdbNum = wLdbNum;

    /* verify and initialize the Chinese XT9 database header */
    mStatus = ET9_CP_CheckLdb(pET9CPLingInfo);
    if (ET9STATUS_NONE != mStatus) {
        pET9CPLingInfo->Private.wLDBInitOK = 0;

        ET9_CS_ResetSBI(&pET9CPLingInfo->SBI);
        pET9CPLingInfo->SBI.wInitOK = 0;

        return mStatus;
    }

    /* we need to set wLDBInitOK to let ET9CPSetInputMode pass the wLangOK check */
    pET9CPLingInfo->Private.wLDBInitOK = ET9GOODSETUP;
    /* clear component range when switching LDB */
    pCommon->wComponentFirst = 0;
    pCommon->wComponentLast = 0;
    /* clear reverse lookup cache when switching LDB */
    pET9CPLingInfo->Private.PPrivate.sLookupCache.sUnicode = ET9_CP_NOMATCH;

    /* default mode is ambiguous pinyin for simplified Chinese and BPMF for traditional Chinese */
    /* always initialize stroke AND one of the phonetic if possible */
    mStatus = ET9STATUS_LDB_VERSION_ERROR;
    for (;;) {
#ifndef ET9CP_DISABLE_STROKE
        if (pCommon->sOffsets.dwStrokeLDBOffset) {
            mStatus = ET9CPSetInputMode(pET9CPLingInfo, ET9CPMODE_STROKE);
            if (ET9STATUS_NONE != mStatus) {
                break; /* stop: LDB & Core contradict */
            }
        }
#endif
        if (pCommon->sOffsets.dwPinyinLDBOffset) {
            mStatus = ET9CPSetInputMode(pET9CPLingInfo, ET9CPMODE_PINYIN);
            if (ET9STATUS_NONE != mStatus) {
                break; /* stop: LDB & Core contradict */
            }
        }
        if (pCommon->sOffsets.dwBpmfLDBOffset) { /* Bpmf or Dual phonetic */
            mStatus = ET9CPSetInputMode(pET9CPLingInfo, ET9CPMODE_BPMF);
            if (ET9STATUS_NONE != mStatus) {
                break; /* stop: LDB & Core contradict */
            }
        }
        break; /* loop only once */
    }
    if (ET9STATUS_NONE != mStatus) {
        /* if set mode failed, set wLDBInitOK back to 0 */
        pET9CPLingInfo->Private.wLDBInitOK = 0;
        return mStatus;
    }

    /* check if UDB is compatible with the new LDB */
    mStatus = ET9_CP_CheckUdbCompat(pET9CPLingInfo);
    return mStatus;
}


/** Initializes Chinese XT9 system.
 *
 * @return  ET9STATUS_NONE       Succeeded
 * @return  ET9STATUS_BAD_PARAM  pET9CPLingInfo == NULL or pWordSymbInfo == NULL
 */
ET9STATUS ET9FARCALL ET9CPSysInit(ET9CPLingInfo *pET9CPLingInfo,            /**< pointer to Chinese linguistic information structure */
                                  ET9WordSymbInfo * const pWordSymbInfo,    /**< pointer to word symbol information structure */
                                  void ET9FARDATA * const pPublicExtension) /**< an optional data pointer. It will be passed as the first parameter of the RUDB write callback */
{
    if (_ET9ByteStringCheckSum(_pbXt9Chinese) != 4205438929U) {
        return ET9STATUS_ERROR;
    }

    if (NULL == pET9CPLingInfo || NULL == pWordSymbInfo)
        return ET9STATUS_BAD_PARAM;

    _ET9WordSymbInit(pWordSymbInfo);
    pET9CPLingInfo->Base.pWordSymbInfo = pWordSymbInfo;
    pET9CPLingInfo->Base.bSelListInvalidated = 1;
    pWordSymbInfo->Private.ppEditionsList[ET9EDITION_CP] = &pET9CPLingInfo->Base;

    pET9CPLingInfo->pPublicExtension = pPublicExtension;
    pET9CPLingInfo->wLdbNum = 0;
    pET9CPLingInfo->pLdbReadData = 0;
    pET9CPLingInfo->Private.wInfoInitOK = 0;
    pET9CPLingInfo->Private.wLDBInitOK = 0;
#ifdef ET9_DIRECT_LDB_ACCESS
    pET9CPLingInfo->Private.pLdbData = 0;
    pET9CPLingInfo->Private.dwLdbDataSize = 0;
#endif

    pET9CPLingInfo->pUdb = 0;   /* reset UDB */
    pET9CPLingInfo->pfUdbWrite = NULL; /* turn off UDB write by OEM */

    pET9CPLingInfo->CommonInfo.bKeyBufLen = 0;
    pET9CPLingInfo->CommonInfo.bCachedNumSymbs = 0;
    pET9CPLingInfo->CommonInfo.bFailNumSymbs = 0xFF;
    pET9CPLingInfo->CommonInfo.wMohuFlags = 0;
    pET9CPLingInfo->CommonInfo.pbContextLen[0] = 0;
    pET9CPLingInfo->CommonInfo.bExtContext = 0;
    ET9_CP_INIT_PHRASE_BUF(pET9CPLingInfo->CommonInfo.sStdPhraseBuf);

    pET9CPLingInfo->Private.PPrivate.bInputHasTone = 0;

    pET9CPLingInfo->bState = 0;
    pET9CPLingInfo->bState |= (ET9U8)ET9_CP_STATE_COMPONENT;
    pET9CPLingInfo->bState |= (ET9U8)ET9_CP_STATE_SMARTPUNCT;
    pET9CPLingInfo->bState |= (ET9U8)ET9_CP_STATE_PARTIAL_SPELL;
    pET9CPLingInfo->bState |= (ET9U8)ET9_CP_STATE_COMMON_CHAR;

    pET9CPLingInfo->Private.wInfoInitOK = ET9GOODSETUP; /* initialization succeeds */

    ET9_CS_SysInit(&pET9CPLingInfo->SBI, pET9CPLingInfo);
#ifdef ET9_ALPHABETIC_MODULE
    pET9CPLingInfo->pAWLing = NULL; /* turn it off by default */
    ET9_CP_Trace_Clear(&pET9CPLingInfo->Trace);
#endif

    ET9_CP_ClearBuildCache(pET9CPLingInfo);
    ET9_CP_SelectionHistInit(&pET9CPLingInfo->SelHistory);

    return ET9STATUS_NONE;
}   /* end of ET9CPSysInit() */

/** Enable/disabe alphabetic XT9.
 *
 *  Enables alphabetic XT9 by passing in pAWLing, a correctly initialized ET9AWLingInfo data structure.<br>
 *  Disables alphabetic XT9 by passing in a NULL pointer.
 *
 * @param pET9CPLingInfo     pointer to chinese information structure.
 * @param pET9AWLingInfo     pointer to correctly initialized ET9AWLingInfo data structure, NULL to disable the feature.
 *
 * @return ET9STATUS_NONE       Succeeded
 * @return ET9STATUS_NO_INIT    pET9CPLingInfo is not properly initialized
 * @return ET9STATUS_BAD_PARAM  pAWLing is not  properly initialized
 *
 */
#ifdef ET9_ALPHABETIC_MODULE
ET9STATUS ET9FARCALL ET9CPSetAW(ET9CPLingInfo *pET9CPLingInfo, ET9AWLingInfo *pET9AWLingInfo)
{
    ET9STATUS eStatus = ET9STATUS_NONE;
    if (!pET9CPLingInfo ||
        (pET9CPLingInfo->Private.wInfoInitOK != ET9GOODSETUP)) {

        return(ET9STATUS_NO_INIT);
    }

    if (pET9AWLingInfo) {
        eStatus = _ET9AWSys_BasicValidityCheck(pET9AWLingInfo);
        if (ET9STATUS_NONE != eStatus || pET9AWLingInfo->ET9AWLdbReadData == NULL) {
            eStatus = ET9STATUS_BAD_PARAM;
        }
    }
    if (ET9STATUS_NONE == eStatus) {
        pET9CPLingInfo->pAWLing = pET9AWLingInfo;
        ET9_CP_INVALIDATE_BUILD_CLEAR_CACHE(pET9CPLingInfo);
    }
    return eStatus;
} /* END ET9CPSetAW */
#endif /* END ET9_ALPHABETIC_MODULE */

#ifndef ET9CP_DISABLE_STROKE

static void ET9LOCALCALL ET9_CP_InitStrokeMode(ET9CPLingInfo *pET9CPLingInfo)
{
    ET9U32 dwReadOffset;
    ET9U8  i;
    ET9_CP_CommonInfo *pCommInfo = &pET9CPLingInfo->CommonInfo;
    ET9_CP_SPrivate   *pSPrivate = &pET9CPLingInfo->Private.SPrivate;

    /* skip one byte, the number of tree layers */
    dwReadOffset = pCommInfo->sOffsets.dwStrokeLDBOffset;
    pSPrivate->wCurNodeSIDBase[0] = 0;
    /* Count of normal SID */
    pSPrivate->wCurTotalMatch[0] = ET9_CP_LdbReadWord(pET9CPLingInfo, dwReadOffset);
    dwReadOffset += 2;
    /* component module offset */
    pSPrivate->dwOffsetComponent = (ET9U32)(pCommInfo->sOffsets.dwStrokeLDBOffset + ET9_CP_LdbReadWord(pET9CPLingInfo, dwReadOffset));
    dwReadOffset += 2;
    /* stroke tree root node offset */
    pSPrivate->dwNodeOffsets[0] = (ET9U32)(pCommInfo->sOffsets.dwStrokeLDBOffset + ET9_CP_LdbReadWord(pET9CPLingInfo, dwReadOffset));
    dwReadOffset += 2;
#ifdef ET9_DEBUG
    for (i = 1; i <= ET9_CP_STROKE_MAXNODES; i++) {
        pSPrivate->dwNodeOffsets[i] = 0xFFFFFFFF; /* clear data cache */
    }
#endif

    /* initializing component */
    dwReadOffset = pSPrivate->dwOffsetComponent;
    /* reading the unicode of the first component */
    pET9CPLingInfo->CommonInfo.wComponentFirst = ET9_CP_LdbReadWord(pET9CPLingInfo, dwReadOffset);
    dwReadOffset += 2;
    pET9CPLingInfo->CommonInfo.wComponentLast = ET9_CP_LdbReadWord(pET9CPLingInfo, dwReadOffset);
    dwReadOffset += 2;

    pSPrivate->wCompUnicode = 0;
    pSPrivate->bCompNumStrokes = 0;

    /* initialize pbSylIDRangeEnd since stroke always has one ID range for each syllable */
    for (i = 0; i < ET9CPMAXPHRASESIZE; i++) {
        pCommInfo->pbRangeEnd[i] = (ET9U8)((i + 1) * ET9_CP_ID_RANGE_SIZE);
    }
}

static void ET9LOCALCALL ET9_CP_InitCangJieMode(ET9CPLingInfo *pET9CPLingInfo)
{
    ET9U32 dwReadOffset;
    ET9UINT i;
    ET9_CP_CPrivate   *pCPrivate = &pET9CPLingInfo->Private.CPrivate;

    ET9Assert(pET9CPLingInfo->CommonInfo.sOffsets.dwCangJieOffset);
    dwReadOffset = pET9CPLingInfo->CommonInfo.sOffsets.dwCangJieOffset;

    /* Header:  4 + 4 + 27 * 2 = 62 bytes */
    pCPrivate->bCangJieLDBVersion = ET9_CP_LdbReadByte(pET9CPLingInfo, dwReadOffset);
    dwReadOffset++;
    ET9Assert(pCPrivate->bCangJieLDBVersion == CANG_JIE_LDB_VERSION);
    pCPrivate->eCangJieType = (ET9_CP_CangJieType)ET9_CP_LdbReadByte(pET9CPLingInfo, dwReadOffset);
    dwReadOffset++;
    dwReadOffset++; /* Not used yet */
    dwReadOffset++; /* Not used yet */
    pCPrivate->nCangJieSidCount = ET9_CP_LdbReadDWord(pET9CPLingInfo, dwReadOffset);
    dwReadOffset += 4; /* sizeof(ET9U32) */

    for ( i = 0; i < CANG_JIE_SUBTREE_TABLE_SIZE; i++ )
    {
        pCPrivate->awSubTreeOffsets[i] = ET9_CP_LdbReadWord(pET9CPLingInfo, dwReadOffset);
        dwReadOffset += 2;/* sizeof(ET9U16) */
    }

    ET9Assert(dwReadOffset - pET9CPLingInfo->CommonInfo.sOffsets.dwCangJieOffset == CANG_JIE_HEADER_SIZE);
    pCPrivate->nCangJieSidOffset = dwReadOffset - pET9CPLingInfo->CommonInfo.sOffsets.dwCangJieOffset;
    pCPrivate->nUnicodeSortedCIDOffset = pCPrivate->nCangJieSidOffset + pCPrivate->nCangJieSidCount * CANG_JIE_UNICODE_SIZE;
}
#endif /* #ifdef ET9CP_DISABLE_STROKE */

/** Sets XT9 Chinese to a specified text input mode.
 *
 *  @param      pET9CPLingInfo                 pointer to chinese information structure
 *  @param      eMode                          Text input mode to set XT9 Chinese to. Valid values are:
 *
 *  @return     ET9STATUS_NONE                 Succeeded
 *  @return     ET9STATUS_NO_INIT              pET9CPLingInfo is not properly initialized
 *  @return     ET9STATUS_BAD_PARAM            eMode is invalid
 *  @return     ET9STATUS_INVALID_MODE         LDB does not support eMode
 */
ET9STATUS ET9FARCALL ET9CPSetInputMode(ET9CPLingInfo *pET9CPLingInfo, ET9CPMode eMode)
{
    ET9STATUS status = ET9STATUS_INVALID_MODE;

    ET9_CP_CHECK_LINGINFO(pET9CPLingInfo);

    if ((eMode != ET9CPMODE_PINYIN) &&
        (eMode != ET9CPMODE_STROKE) &&
        (eMode != ET9CPMODE_BPMF) &&
        (eMode != ET9CPMODE_CANGJIE) &&
        (eMode != ET9CPMODE_QUICK_CANGJIE)) {
        return ET9STATUS_BAD_PARAM;
    }
    ET9_CP_INVALIDATE_BUILD_CLEAR_CACHE(pET9CPLingInfo);
    ET9_CP_SelectionHistInit(&pET9CPLingInfo->SelHistory);

    if (((eMode == ET9CPMODE_PINYIN) && (pET9CPLingInfo->CommonInfo.sOffsets.dwPinyinLDBOffset)) ||
        ((eMode == ET9CPMODE_BPMF) && (pET9CPLingInfo->CommonInfo.sOffsets.dwBpmfLDBOffset))) {

        pET9CPLingInfo->eMode = eMode;
        status = ET9STATUS_NONE;
    }

#ifndef ET9CP_DISABLE_STROKE
    if ((eMode == ET9CPMODE_STROKE) && (pET9CPLingInfo->CommonInfo.sOffsets.dwStrokeLDBOffset)) {
        pET9CPLingInfo->eMode = eMode;
        /* reading parameters from the ldb */
        ET9_CP_InitStrokeMode(pET9CPLingInfo);
        status = ET9STATUS_NONE;
    }
    else if ((eMode == ET9CPMODE_CANGJIE || eMode == ET9CPMODE_QUICK_CANGJIE) && (pET9CPLingInfo->CommonInfo.sOffsets.dwCangJieOffset)) {
        pET9CPLingInfo->eMode = eMode;
        /* reading parameters from the ldb */
        ET9_CP_InitCangJieMode(pET9CPLingInfo);
        status = ET9STATUS_NONE;
    }
#endif
    if (ET9STATUS_NONE == status) {
        ET9CPClearContext(pET9CPLingInfo);
        if ((eMode == ET9CPMODE_PINYIN || eMode == ET9CPMODE_BPMF)) {
            /* Init SBI according to InputMode */
            if (pET9CPLingInfo->CommonInfo.sOffsets.dwSBITrieOffset) {
                ET9_CS_SBIInit(&pET9CPLingInfo->SBI);
            }
#ifdef ET9_ALPHABETIC_MODULE
            /* update Trace alpha LDB */
            if (pET9CPLingInfo->Trace.pALing) {
                ET9_CP_Trace_LdbSync(pET9CPLingInfo);
            }
#endif
        }
    }

    return status;
}

/** \internal
 * Check if ET9SymbInfo contains a symbol
 *
 * @param symb        an ET9SYMB
 * @param pSymbInfo   an ET9SymbInfo
 *
 * @return 1 if symb is in the lower case array and upper case array of pSymbInfo, 0 otherwise.
 */
ET9U8 ET9FARCALL ET9_CP_IsSymbInSymbInfo(ET9SYMB symb, const ET9SymbInfo* pSymbInfo)
{
    const ET9SYMB *psChar;
    ET9U8 i, j;
    if (symb == 0) {
        return 0;
    }
    if (ET9CPIsBpmfUpperCaseSymbol(symb)) {
        symb = ET9CPBpmfSymbolToLower(symb);
    }
    if ( ET9_CP_IsPinyinUpperCase(symb) ) {
        for ( i = 0; i < /*pSymbInfo->bNumBaseSyms*/1; i++ ) {
            psChar = pSymbInfo->DataPerBaseSym[i].sUpperCaseChar;
            for (j = 0; j < pSymbInfo->DataPerBaseSym[i].bNumSymsToMatch; j++) {
                if (symb == psChar[j]) {
                    return 1;
                }
            }
        }
    }
    else {
        for ( i = 0; i < /*pSymbInfo->bNumBaseSyms*/1; i++ ) {
            psChar = pSymbInfo->DataPerBaseSym[i].sChar;
            for (j = 0; j < pSymbInfo->DataPerBaseSym[i].bNumSymsToMatch; j++) {
                if (symb == psChar[j]) {
                    return 1;
                }
            }
        }
    }
    return 0;
}


/* return the length of the WSI that has not yet been processed by BuildSpellings */
ET9UINT ET9FARCALL ET9_CP_WSIValidLen(const ET9CPLingInfo * const pET9CPLingInfo, const ET9WordSymbInfo * const pWordSymbInfo)
{
    ET9UINT n;

    for (n = 0; n < pWordSymbInfo->bNumSymbs; n++) {
        if (pET9CPLingInfo->Base.bSymbInvalidated[n] ||
            pET9CPLingInfo->Base.bLockInvalidated[n]) {
            break;
        }
    }

    return n;
}


/* return 1 if it is needed to build the spelling list because the input has
 * changed from last successful build.  Return 0 if the input has not changed.
 *
 * note: input includes WSI, component, locked spelling stem, but not other state settings
 *      so when a change of configuration can change the spelling/phrase, the buffers
 *      should be cleared.
 */
static ET9UINT ET9LOCALCALL NeedBuildSpell(const ET9CPLingInfo * const pET9CPLingInfo,
                                    const ET9WordSymbInfo * const pWordSymbInfo)
{
    ET9U8 bCachedKeyLen, bValidLen;
    if ( pET9CPLingInfo->Base.bSymbsInfoInvalidated && !ET9CPIsModeStroke(pET9CPLingInfo) )
        return 1;

    bCachedKeyLen = pET9CPLingInfo->CommonInfo.bCachedNumSymbs;

    bValidLen = (ET9U8)ET9_CP_WSIValidLen(pET9CPLingInfo, pWordSymbInfo);

    if (pWordSymbInfo->bNumSymbs != bCachedKeyLen ||
        bValidLen < pWordSymbInfo->bNumSymbs)
    {
        return 1;
    } else {
        return 0;
    }
}


static void ET9LOCALCALL ValidateBuild(ET9CPLingInfo * const pET9CPLingInfo)
{
    ET9UINT n;

    pET9CPLingInfo->Base.bSelListInvalidated = 0;
    pET9CPLingInfo->Base.bSymbsInfoInvalidated = 0;

    for (n = 0; n < pET9CPLingInfo->Base.pWordSymbInfo->bNumSymbs; n++) {
        pET9CPLingInfo->Base.bSymbInvalidated[n] = 0;
        pET9CPLingInfo->Base.bLockInvalidated[n] = 0;
    }
}

static ET9INT ET9LOCALCALL PinYinUpperCharCount( const ET9U8 * pc, ET9U8 bLen)  /* Only works for PinYin */
{
    ET9INT n = 0;
    while ( bLen-- > 0 )
    {
        if (ET9_CP_IsPinyinUpperCase(*pc))
            n++;
        pc++;
    }
    return n;
}
static ET9INT ET9LOCALCALL BPMFUpperCharCount( ET9U8 * pc, ET9U8 bLen)  /* Only works for BPMF */
{
    ET9INT n = 0;
    while ( bLen-- > 0 )
    {
        if (ET9_CP_IsBpmfUpperCase(*pc))
            n++;
        pc++;
    }
    return n;
}

void ET9FARCALL ET9_CP_MakeInternalJianpinSpell(ET9CPLingInfo  * const pLing,
                                                ET9CPPhrase    *pPhrase,
                                                ET9_CP_Spell   *pOutSpell)
{
    ET9WordSymbInfo * pWSI;
    const ET9SymbInfo *pSI;
    ET9U8 i;
    ET9_CP_Spell sFullSpell;

    ET9_CP_PidBidToSpelling(pLing, pPhrase->pSymbs, pPhrase->bLen, &sFullSpell);

    pWSI = pLing->Base.pWordSymbInfo;
    pSI = pWSI->SymbsInfo;
    pOutSpell->bLen = 0;

    for ( i = 0; pOutSpell->bLen < pWSI->bNumSymbs; )
    {
        ET9Assert(pSI < (pWSI->SymbsInfo + pWSI->bNumSymbs) );

        if ( ET9_CP_SymbIsDelim(pSI) ) {
            /* if WSI has delimiter, add it to Jianpin spell instead of adding the next upper case letter */
            pOutSpell->pbChars[pOutSpell->bLen++] = ET9CPSYLLABLEDELIMITER;
            pSI++; /* go to next symb in WSI, do not advance the full spell */
            continue;
        }
        else if ( i < sFullSpell.bLen && ET9_CP_IsUpperCase(sFullSpell.pbChars[i]) )
        {
            pOutSpell->pbChars[pOutSpell->bLen++] = sFullSpell.pbChars[i];
            pSI++; /* go to next symb in WSI */
        }
        i++; /* advance the full spell */
    }
}

static ET9INT ET9LOCALCALL PrefixGroupLessThan( ET9_CP_PrefixGroup * p1, ET9_CP_PrefixGroup * p2, ET9U8 bMethod)
{
    if (bMethod == ET9_CP_PREFIX_SORT_BY_LENGTH) {
        if (p1->bPfxLen < p2->bPfxLen)
            return 1;
        else if (p1->bPfxLen > p2->bPfxLen)
            return 0;
        else
            return (p1->iProb < p2->iProb);
    }
    else {
        return (p1->iProb > p2->iProb);
    }
}

void ET9FARCALL ET9_CP_SortPrefixGrp(ET9CPLingInfo * pET9CPLingInfo)
{
    ET9INT i, iPrefixCount, fGrpIsRejected;
    ET9U8 bGrpIdx = 0;
    ET9_CS_Prefix sPrefix;
    ET9_CP_SSBITrie * pTrie = &pET9CPLingInfo->SBI;
    ET9U8 bSortMethod = pET9CPLingInfo->CommonInfo.bPrefixSortMethod;

    iPrefixCount = ET9_CS_GetPrefixCount(pTrie);
    ET9Assert(iPrefixCount <= ET9_CP_MAX_PREFIX_COUNT);
    if ( iPrefixCount > ET9_CP_MAX_PREFIX_COUNT ) {
        iPrefixCount = ET9_CP_MAX_PREFIX_COUNT;  /* Prevent buffer overrun */
    }
    pET9CPLingInfo->CommonInfo.bActivePrefix = 0xFF;
    pET9CPLingInfo->CommonInfo.bSyllablePrefixCount = 0;
    fGrpIsRejected = 0;

    for ( i = 0; i < iPrefixCount; i++ )
    {
        ET9INT iUpperCount = 0;

        ET9_CS_GetPrefix( pTrie, i, &sPrefix );

        if (ET9CPIsModePinyin(pET9CPLingInfo))
            iUpperCount = PinYinUpperCharCount(sPrefix.m_pcPfx, sPrefix.m_bPfxLen);
        else if (ET9CPIsModeBpmf(pET9CPLingInfo))
            iUpperCount = BPMFUpperCharCount(sPrefix.m_pcPfx, sPrefix.m_bPfxLen);

        ET9Assert(iUpperCount != 0);
        if (iUpperCount > 1)
        {   /* Do not give multi_syllable prefix to caller */
            if (!fGrpIsRejected) {
                pET9CPLingInfo->CommonInfo.pPrefixGroup[bGrpIdx].bPfxCount++;
            }
        }
        else /* Prefix is single syllable */
        {
            bGrpIdx = pET9CPLingInfo->CommonInfo.bSyllablePrefixCount++;
            pET9CPLingInfo->CommonInfo.pPrefixGroup[bGrpIdx].bOrigIndex = bGrpIdx;
            pET9CPLingInfo->CommonInfo.pPrefixGroup[bGrpIdx].bStartIndex = (ET9U8)i;
            pET9CPLingInfo->CommonInfo.pPrefixGroup[bGrpIdx].bPfxLen = sPrefix.m_bPfxLen;
            pET9CPLingInfo->CommonInfo.pPrefixGroup[bGrpIdx].iProb = sPrefix.m_iPfxProb;
            pET9CPLingInfo->CommonInfo.pPrefixGroup[bGrpIdx].bPfxCount = 1;
        }
    }

    if (bSortMethod == ET9_CP_PREFIX_SORT_BY_LENGTH || bSortMethod == ET9_CP_PREFIX_SORT_BY_FREQ)
    {   /* Use Insertion Sort */
        ET9_CP_PrefixGroup tmp, *pPrefixGroup = pET9CPLingInfo->CommonInfo.pPrefixGroup;
        ET9S32 iCount = (ET9S32)pET9CPLingInfo->CommonInfo.bSyllablePrefixCount;
        for ( i = 1; i < iCount; i++ )
        {
            ET9S32  j;
            tmp = pPrefixGroup[i];
            j = i;
            while ((j > 0) && PrefixGroupLessThan(&pPrefixGroup[j-1], &tmp, bSortMethod))
            {
                pPrefixGroup[j] = pPrefixGroup[j-1];
                j = j - 1;
            }
            pPrefixGroup[j] = tmp;
        }
    }
    if ( pET9CPLingInfo->SBI.m_bSBIConditionLen )
    {
        ET9_CS_Prefix pfx, condition;
        ET9_CS_GetCondition(&pET9CPLingInfo->SBI, &condition);
        for ( i = 0; i < (ET9S32)pET9CPLingInfo->CommonInfo.bSyllablePrefixCount; i++ )
        {
            ET9S32 index = (ET9S32)pET9CPLingInfo->CommonInfo.pPrefixGroup[i].bStartIndex;
            ET9Assert( ET9STATUS_NONE == ET9_CS_GetPrefix( &pET9CPLingInfo->SBI, index, &pfx ) );
            ET9_CS_GetPrefix( &pET9CPLingInfo->SBI, index, &pfx );
            if ( pfx.m_bPfxLen == condition.m_bPfxLen && ET9_CP_MemCmp(pfx.m_pcPfx, condition.m_pcPfx, condition.m_bPfxLen) == 0 )
            {
                pET9CPLingInfo->CommonInfo.bActivePrefix = (ET9U8)i;
                break;
            }
        }
        ET9Assert(pET9CPLingInfo->CommonInfo.bSyllablePrefixCount == 0 || pET9CPLingInfo->CommonInfo.bActivePrefix != 0xFF);
    }
}

ET9STATUS ET9FARCALL ET9_CP_JianpinBuildSpellings(ET9CPLingInfo * pET9CPLingInfo)
{
    ET9_CS_ClearPrefixBuf(&pET9CPLingInfo->SBI);
    ET9_CP_ClearPhraseBuf(ET9_CP_GetMainPhraseBuf(pET9CPLingInfo));
    /* Try to find Jianpin phrase. If cannot find phrase, return invalid input */
    return ET9_CP_JianpinFillPhraseBuffer(pET9CPLingInfo);
}

ET9STATUS ET9FARCALL ET9_CP_PhoneticBuildSpellings(ET9CPLingInfo * pET9CPLingInfo)
{
    ET9STATUS status = ET9STATUS_NEED_SELLIST_BUILD;

    if (ET9_CP_InputContainsTrace(pET9CPLingInfo)) {
#ifdef ET9_ALPHABETIC_MODULE
        /* trace */
        status = ET9_CP_Trace_BuildSelectionList(pET9CPLingInfo);
#else
        status = ET9STATUS_TRACE_NOT_AVAILABLE;
#endif
    }

    if (ET9STATUS_NEED_SELLIST_BUILD == status) {
        /* SBI */
        status = ET9_CP_BuildSBISpellings(pET9CPLingInfo);

        /* Jianpin if SBI failed and Partial Spell is OFF */
        if (ET9STATUS_INVALID_INPUT == status && !ET9CPIsPartialSpellActive(pET9CPLingInfo) && !ET9_CP_InputHasTone(pET9CPLingInfo) )
        {
            status = ET9_CP_JianpinBuildSpellings(pET9CPLingInfo);
        }
    }

    return status;
}

/** Search the Chinese databases to find all the prefixes and phrases matching the ET9WordSymbInfo associated with the ET9CPLingInfo.
 *
 * If this function succeeds, the integration layer can call ET9CPGetPrefix() and ET9CPGetPhrase()
 * to update the prefix/selection lists for presentation to the user.

 * @param pET9CPLingInfo     pointer to chinese information structure

 * @return ET9STATUS_NONE                 Succeeded
 * @return ET9STATUS_NO_INIT              pET9CPLingInfo is not properly initialized
 * @return ET9STATUS_INVALID_INPUT        No output can be generated for current input
 * @return ET9STATUS_ALL_SYMB_SELECTED    All symbols are selected, caller should call ET9CPCommitSelection().
*/
ET9STATUS ET9FARCALL ET9CPBuildSelectionList(ET9CPLingInfo * const pET9CPLingInfo)
{
    ET9STATUS status = ET9STATUS_INVALID_INPUT;
    ET9WordSymbInfo * pWordSymbInfo;
    ET9_CP_CommonInfo * pCommInfo;
    ET9U8 bUnselectedStart, bUnchanged;

    ET9_CP_CHECK_LINGINFO(pET9CPLingInfo);
    pWordSymbInfo = pET9CPLingInfo->Base.pWordSymbInfo;

    pCommInfo = &pET9CPLingInfo->CommonInfo;
    ET9Assert(pWordSymbInfo);

    if (pWordSymbInfo->bNumSymbs > ET9_CP_MAXKEY) {
        return ET9STATUS_INVALID_INPUT;
    }

    /* Disable trace in non-phonetic mode */
    if (_ET9HasTraceInfo(pWordSymbInfo) && !ET9CPIsModePhonetic(pET9CPLingInfo)) {
        return ET9STATUS_INVALID_INPUT;
    }

    if (ET9_CP_IsUdbChangedByOtherThread((pET9CPLingInfo))) {
        ET9_CP_INVALIDATE_BUILD_CLEAR_CACHE((pET9CPLingInfo));
        ET9_CP_SelectionHistInit(&pET9CPLingInfo->SelHistory);
    }

    bUnselectedStart = ET9_CP_SelectionHistUnselectedStart(&pET9CPLingInfo->SelHistory);
    bUnchanged = (ET9U8)ET9_CP_WSIValidLen(pET9CPLingInfo, pWordSymbInfo);

    /* If we've changed or cleared the WSI before selection point, unselect */
    if ( bUnchanged < bUnselectedStart )  {
        ET9U8 b;
        ET9_CP_SelectionHistInit(&pET9CPLingInfo->SelHistory);
        ET9_CS_ResetSBI(&pET9CPLingInfo->SBI);
        for ( b = 0; b < pWordSymbInfo->bNumSymbs; b++ ) /* Mark all SymbInfo dirty */
        {
            pET9CPLingInfo->Base.bSymbInvalidated[b] = 1;
        }
        bUnselectedStart = 0;
        bUnchanged = 0;
    }

    /* No symbols in WSI so no spellings. Prepare for common char */
    if ( pWordSymbInfo->bNumSymbs == 0 )
    {
        ET9_CP_ClearBuildCache(pET9CPLingInfo);
        ET9_CP_SelectionHistInit(&pET9CPLingInfo->SelHistory); /* Reset everything in case core needs to be reset */
        status = ET9STATUS_NONE;
    }
    /* No unselected symbols, return error */
    else if (bUnselectedStart == pWordSymbInfo->bNumSymbs)
    {
        status = ET9STATUS_ALL_SYMB_SELECTED;
    }
    /* WSI matches cache, no need to do anything */
    else if (!NeedBuildSpell(pET9CPLingInfo, pWordSymbInfo)){
        status = ET9STATUS_NONE;
    }
    /* WSI has changed since last build, so rebuild spellings */
    else {
        /* each spell builder should update active spell only if the build is successful */

        /* Phonetic */
        if (ET9CPIsModePhonetic(pET9CPLingInfo)) {
            status = ET9_CP_PhoneticBuildSpellings(pET9CPLingInfo);
        }
        /* Stroke */
#ifndef ET9CP_DISABLE_STROKE
        else if (ET9CPIsModeStroke(pET9CPLingInfo)) {
            status = ET9_CP_StrokeBuildSpellings(pET9CPLingInfo);
        }
        /* Cangjie */
        else if (ET9CPIsModeCangJie(pET9CPLingInfo)) {
            status = ET9_CP_CangJieBuildSpellings(pET9CPLingInfo);
        }
        /* Quick Cangjie */
        else if (ET9CPIsModeQuickCangJie(pET9CPLingInfo)) {
            status = ET9_CP_QuickCangJieBuildSpellings(pET9CPLingInfo);
        }
#endif
    }

    /* we were succesful if we built spellings or if there were no keys */
    if (ET9STATUS_NONE == status) {
        /* we should never have keys and no spellings or spellings with no keys */
        ET9Assert(pWordSymbInfo->bNumSymbs && ET9_CP_HasActiveSpell(pET9CPLingInfo) ||
            !pWordSymbInfo->bNumSymbs && !ET9_CP_HasActiveSpell(pET9CPLingInfo));

        ValidateBuild(pET9CPLingInfo);
        pCommInfo->bCachedNumSymbs = (ET9U8)pWordSymbInfo->bNumSymbs;

        if (pET9CPLingInfo->pUdb) { /* update our local dirtycount */
            pET9CPLingInfo->UdbReadCache.dwDirtyCount = pET9CPLingInfo->pUdb->dwDirtyCount;
        }
    }
    else if ( status != ET9STATUS_ALL_SYMB_SELECTED ) {
        pET9CPLingInfo->Base.bSelListInvalidated = 1;
        status = ET9STATUS_INVALID_INPUT;
    }

    return status;
}

/** Clears context buffer.
 *
 * See ET9CPSetContext()
 *
 * @param pET9CPLingInfo     pointer to chinese information structure.
 *
 * @return ET9STATUS_NO_INIT   - pET9CPLingInfo is null or Chinese XT9 is not initialized successfully
 * @return ET9STATUS_NONE      - success
 */
ET9STATUS ET9FARCALL ET9CPClearContext(ET9CPLingInfo *pET9CPLingInfo)
{
    ET9_CP_CHECK_LINGINFO(pET9CPLingInfo);

    /* we validated pET9CPLingInfo already, so don't have to look at the return status */
    ET9_CP_INVALIDATE_BUILD_CLEAR_CACHE(pET9CPLingInfo);

    ET9_CP_ClrContextBuf(pET9CPLingInfo);
    return ET9STATUS_NONE;
}

ET9STATUS ET9FARCALL ET9_CP_GetPhrase(ET9CPLingInfo *pET9CPLingInfo, ET9U16 wPhraseIndex, ET9CPPhrase *psPhrase, ET9CPSpell *psSpell)
{
    ET9STATUS ET9FARCALL (*FctFill)(ET9CPLingInfo *pET9CPLingInfo) = (ET9STATUS ET9FARCALL (*)(ET9CPLingInfo *))NULL;
    ET9_CP_IDEncode eEnc = ET9_CP_IDEncode_UNI;
    ET9STATUS status;

#ifdef EVAL_BUILD
    {
        ET9U32 dwDirtyCount = 0;
        if (pET9CPLingInfo->pUdb) {
            dwDirtyCount = pET9CPLingInfo->pUdb->dwDirtyCount;
        }
        if (_ET9Eval_HasExpired(&pET9CPLingInfo->Base, (ET9U16)dwDirtyCount)) {
            return ET9STATUS_EVAL_BUILD_EXPIRED;
        }
    }
#endif /*EVAL_BUILD*/

    if (!psPhrase) {
        return ET9STATUS_BAD_PARAM;
    }

    /* default to phonetic mode */
#ifdef ET9_ALPHABETIC_MODULE
    if (ET9_CP_InputContainsTrace(pET9CPLingInfo)) {
        FctFill = ET9_CP_Trace_FillPhraseBuffer;
    }
    else 
#endif
    if (ET9_CP_HasActiveSpell(pET9CPLingInfo) ) {
        if (ET9_CS_GetCandidateCount(&pET9CPLingInfo->SBI)) {
            FctFill = ET9_CP_PrefixFillPhraseBuffer;
        }
        else {
            FctFill = ET9_CP_JianpinFillPhraseBuffer;
        }
    }
    else { /* context prediction and common char */
        FctFill = ET9_CP_ContextFillPhraseBuffer;
    }
    eEnc = ET9_CP_IDEncode_PID;
#ifndef ET9CP_DISABLE_STROKE
    if (ET9CPIsModeStroke(pET9CPLingInfo)) {
        FctFill = ET9_CP_StrokeFillPhraseBuffer;
        eEnc = ET9_CP_IDEncode_SID;
        ET9_CP_LoadStrokeSpell(pET9CPLingInfo, &pET9CPLingInfo->CommonInfo.sActiveSpell);
    }
    else if (ET9CPIsModeCangJie(pET9CPLingInfo) || ET9CPIsModeQuickCangJie(pET9CPLingInfo)) {
        FctFill = ET9_CP_CangJieFillPhraseBuffer;
        eEnc = ET9_CP_IDEncode_SID;
    }
#endif

    /* Fill buffer if necessary */
    status = ET9_CP_AdjustPointerFillPhraseBuf(pET9CPLingInfo, wPhraseIndex, FctFill);
    if (ET9STATUS_INVALID_INPUT == status) {
        /* this could only happen when there are no keys and common char is disabled, map to OUT_OF_RANGE */
        ET9Assert(pET9CPLingInfo->Base.pWordSymbInfo->bNumSymbs == ET9_CP_SelectionHistUnselectedStart(&pET9CPLingInfo->SelHistory) &&
            !ET9CPIsCommonCharActive(pET9CPLingInfo));
        status = ET9STATUS_OUT_OF_RANGE;
    }
    ET9Assert(ET9STATUS_INVALID_INPUT != status);

    /* Get phrase */
    if (ET9STATUS_NONE == status) {
        ET9_CP_Spell sInternalSpell;
        
        sInternalSpell.bLen = 0;

        ET9_CP_GetPhraseFromBuf(ET9_CP_GetMainPhraseBuf(pET9CPLingInfo), psPhrase, &sInternalSpell, wPhraseIndex + 1);

        if (ET9_CP_JianpinFillPhraseBuffer == FctFill) {
            ET9_CP_MakeInternalJianpinSpell(pET9CPLingInfo, psPhrase, &sInternalSpell);
        }

        /* convert to Unicode if necessary */
        ET9_CP_ConvertPhraseToUnicode(pET9CPLingInfo, psPhrase, eEnc);

        /* convert to external spell if requested */
        if (psSpell) {
            ET9_CP_Spell * pInternalSpell = &sInternalSpell;

            /* no spelling provided in phrase buffer, use active spell */
            if (0 == sInternalSpell.bLen) {
                /* this should only be neccessary for stroke/cangjie/no keys/tone */
                ET9Assert(!ET9CPIsModePhonetic(pET9CPLingInfo)
                          || 0 == pET9CPLingInfo->Base.pWordSymbInfo->bNumSymbs);

#ifndef ET9CP_DISABLE_STROKE
                if (ET9CPIsModeStroke(pET9CPLingInfo) ) {
                    /* in Stroke mode, copy active spell to local spell, trim the local spell length to phrase length */
                    sInternalSpell = pET9CPLingInfo->CommonInfo.sActiveSpell;
                    ET9_CP_TrimStrokeSpellLength(&sInternalSpell, psPhrase->bLen);
                }
                else
#endif
                {
                    /* otherwise, just use the active spell */
                    pInternalSpell = &pET9CPLingInfo->CommonInfo.sActiveSpell;
                }
            }

            ET9_CP_ToExternalSpellInfo(pET9CPLingInfo, pInternalSpell, psSpell);
        }
    }
    return status;
}

/** Retrieve the candidate phrase specified by a 0-based index
 *
 *  XT9 Chinese provides a selection list at all times. When WordSymbInfo is empty, the selection list contains predicted phrases
 *  that the user may enter, based on the context( See ET9CPSetContext() and ET9CPClearContext() and ET9CPCommitSelection() ).
 *
 *  @param pET9CPLingInfo   (input) pointer to chinese information structure.
 *  @param wPhraseIndex     (input) 0-based index.
 *  @param psPhrase         (output) The phrase matching the user's input.
 *  @param psSpell          (output, optional) The spelling of the phrase according to user's input.
 *                              For phonetic phrases, this could be different than the spelling returned by ET9CPGetSpell.
 *
 *  @return ET9STATUS_NONE               Success
 *  @return ET9STATUS_NO_INIT            pET9CPLingInfo is not properly initialized
 *  @return ET9STATUS_NEED_SELLIST_BUILD Should call ET9CPBuildSelectionList() before calling this function
 *  @return ET9STATUS_BAD_PARAM          some argument pointer is NULL
 *  @return ET9STATUS_INVALID_INPUT      There is no candidate phrase
 *  @return ET9STATUS_OUT_OF_RANGE       wPhraseIndex is too big
 *
 */
ET9STATUS ET9FARCALL ET9CPGetPhrase(ET9CPLingInfo *pET9CPLingInfo, ET9U16 wPhraseIndex, ET9CPPhrase *psPhrase, ET9CPSpell *psSpell)
{
    ET9_CP_CHECK_LINGINFO(pET9CPLingInfo);
    ET9_CP_CHECK_UDB_UP_TO_DATE(pET9CPLingInfo);
    ET9_CP_CHECK_BUILD_UP_TO_DATE(pET9CPLingInfo);
    ET9Assert(!pET9CPLingInfo->Base.pWordSymbInfo->bNumSymbs ||
              ET9_CP_HasActiveSpell(pET9CPLingInfo));

    return ET9_CP_GetPhrase(pET9CPLingInfo, wPhraseIndex, psPhrase, psSpell);
}

static ET9STATUS ET9LOCALCALL ET9_CP_SBI_SelectSegment(ET9CPLingInfo *pET9CPLingInfo,
                                                       ET9U16         wPhraseIndex,
                                                       ET9CPSpell    *pOutSpell)
{
    ET9STATUS status;
    ET9CPPhrase  sPhrase;
    ET9_CP_Spell spellOfSegment;
    ET9_CP_PhraseBuf *pMainPhraseBuf = ET9_CP_GetMainPhraseBuf(pET9CPLingInfo);

    ET9_CP_GetPhraseFromBuf(pMainPhraseBuf, &sPhrase, &spellOfSegment, wPhraseIndex + 1);

    status = ET9_CS_SelectSegment(&pET9CPLingInfo->SBI, sPhrase.pSymbs, sPhrase.bLen, spellOfSegment.pbChars, spellOfSegment.bLen);
    if ( status != ET9STATUS_NONE ) {
        return status;
    }
    if ( pOutSpell && spellOfSegment.bLen > 0)
    {
        /* get the complete spelling from PIDs of sPhrase */
        ET9_CP_PidBidToSpelling(pET9CPLingInfo, sPhrase.pSymbs, sPhrase.bLen, &spellOfSegment);
        ET9_CP_ToExternalSpellInfo(pET9CPLingInfo, &spellOfSegment, pOutSpell);
    }
    return ET9STATUS_NONE;
}


static ET9STATUS ET9LOCALCALL ET9_CP_Jianpin_SelectPhrase(ET9CPLingInfo   *pLing,
                                                          ET9U16          wPhraseIndex,
                                                          ET9CPSpell      *pSpell)
{
    ET9STATUS status = ET9STATUS_NONE;
    ET9CPPhrase phraseUnicode, phraseEncoded;
    ET9_CP_PhraseBuf *pMainPhraseBuf;
    ET9_CP_Spell sInternalSpell;

    pMainPhraseBuf = ET9_CP_GetMainPhraseBuf(pLing);

    ET9_CP_GetPhraseFromBuf(pMainPhraseBuf, &phraseEncoded, NULL, wPhraseIndex + 1);

    if (pSpell) {
        /* get the complete spelling from PIDs of sPhrase */
        ET9_CP_PidBidToSpelling(pLing, phraseEncoded.pSymbs, phraseEncoded.bLen, &sInternalSpell);
        ET9_CP_ToExternalSpellInfo(pLing, &sInternalSpell, pSpell);
    } 

    _ET9SymCopy(phraseUnicode.pSymbs, (ET9SYMB *)phraseEncoded.pSymbs, phraseEncoded.bLen);
    phraseUnicode.bLen = phraseEncoded.bLen;
    ET9_CP_ConvertPhraseToUnicode(pLing, &phraseUnicode, ET9_CP_IDEncode_PID);

    if (ET9_CP_PhraseIsAllChn(pLing, phraseEncoded.pSymbs, phraseEncoded.bLen) )
    {
        status = ET9_CP_SelectionHistAdd(&pLing->SelHistory, phraseUnicode.pSymbs, phraseEncoded.pSymbs, phraseEncoded.bLen, pLing->Base.pWordSymbInfo->bNumSymbs);
    }
    else /*  NULL in the phraseEncoded position means do not add to ContextBuf */
    {
        status = ET9_CP_SelectionHistAdd(&pLing->SelHistory, phraseUnicode.pSymbs, NULL, phraseEncoded.bLen, pLing->Base.pWordSymbInfo->bNumSymbs);
    }

    return status;
}


static ET9STATUS ET9LOCALCALL ET9_CP_Phonetic_SelectPhrase(ET9CPLingInfo   *pET9CPLingInfo,
                                                           ET9U16          wPhraseIndex,
                                                           ET9CPSpell      *pSpell)
{
    ET9STATUS status;

#ifdef ET9_ALPHABETIC_MODULE
    if (ET9_CP_InputContainsTrace(pET9CPLingInfo)) {
        /* trace */
        status = ET9_CP_Trace_SelectPhrase(&pET9CPLingInfo->Trace, wPhraseIndex, pSpell);
    }
    else 
#endif
    if (ET9_CP_HasActiveSpell(pET9CPLingInfo) && 0 == ET9_CS_GetCandidateCount(&pET9CPLingInfo->SBI)) {
        /* jianpin : has active spell but no SBI candidate */
        status = ET9_CP_Jianpin_SelectPhrase(pET9CPLingInfo, wPhraseIndex, pSpell);
    }
    else {
        /* sbi */
        status = ET9_CP_SBI_SelectSegment(pET9CPLingInfo, wPhraseIndex, pSpell);
    }

    return status;
}

/** Notifies Chinese XT9 that the user has selected the phrase specified by the 0-based index.
 *
 *  The integration layer calls this function to notify Chinese XT9 that a phrase
 *  has been selected. Each call to this function will return the status ET9STATUS_NONE, until all
 *  inputs in the WordSymbInfo have been consumed, then it returns the status ET9STATUS_ALL_SYMB_SELECTED.
*   This return status ET9STATUS_ALL_SYMB_SELECTED notifies the integration layer to call the function ET9CPCommitSelection().<p>
 *  If the character selected is a component, ET9CPSelectPhrase() will return
 *  ET9STATUS_SELECTED_CHINESE_COMPONENT. In that case the integration layer
 *  should add the component symbol to the end of the WordSymbInfo by ET9AddExplicitSymb().<p>
 *  If this function returns ET9STATUS_NEED_SELLIST_BUILD (means last build is out of date),
 *  integration layer should call ET9CPBuildSelectionList() before calling this function.
 *
 * @param pET9CPLingInfo     pointer to chinese information structure.
 * @param wPhraseIndex       0-based phrase index .
 * @param psSpell            (output)spelling of the selected phrase and its length. If it is NULL, spell is not returned
 *
 * @return ET9STATUS_NONE               Succeeded
 * @return ET9STATUS_SELECTED_CHINESE_COMPONENT Succeeded, but selected is a component
 * @return ET9STATUS_ALL_SYMB_SELECTED  Succeeded, but all the symbols in ET9WordSymbInfo are selected
 * @return ET9STATUS_NO_INIT            pET9CPLingInfo is not properly initialized
 * @return ET9STATUS_NEED_SELLIST_BUILD Should call ET9CPBuildSelectionList() before calling this function
 * @return ET9STATUS_OUT_OF_RANGE       The given index is too big
 * @return ET9STATUS_FULL               Accumulated selection is too long
 *
 */
ET9STATUS ET9FARCALL ET9CPSelectPhrase(ET9CPLingInfo    *pET9CPLingInfo,
                                       ET9U16           wPhraseIndex,
                                       ET9CPSpell       *psSpell)
{
    ET9STATUS status = ET9STATUS_NONE;
    ET9_CP_PhraseBuf *pMainPhraseBuf;
    ET9U8 bUnselectedStart, bNumSymbs;

    /* validate inputs */
    ET9_CP_CHECK_LINGINFO(pET9CPLingInfo);
    ET9_CP_CHECK_UDB_UP_TO_DATE(pET9CPLingInfo);
    ET9_CP_CHECK_BUILD_UP_TO_DATE(pET9CPLingInfo);

    ET9Assert(0 == pET9CPLingInfo->Base.pWordSymbInfo->bNumSymbs ||
              ET9_CP_HasActiveSpell(pET9CPLingInfo));

    if (psSpell) {
        psSpell->bLen = 0;
    }
    pMainPhraseBuf = ET9_CP_GetMainPhraseBuf(pET9CPLingInfo);
    if (pMainPhraseBuf->bTestClear || pMainPhraseBuf->wTotal <= wPhraseIndex) {
        return ET9STATUS_OUT_OF_RANGE;
    }

    bUnselectedStart = ET9_CP_SelectionHistUnselectedStart(&pET9CPLingInfo->SelHistory);
    bNumSymbs = (ET9U8)(pET9CPLingInfo->Base.pWordSymbInfo->bNumSymbs - bUnselectedStart);
    /* if we have symbols we should have spellings */
    if (bNumSymbs > 0) {
        ET9Assert(pET9CPLingInfo->Base.pWordSymbInfo->bNumSymbs > 0);
        ET9Assert(ET9_CP_HasActiveSpell(pET9CPLingInfo)); /* should be set in buildspellings */
    }
    else
    {
        ET9Assert(!ET9_CP_HasActiveSpell(pET9CPLingInfo));
    }

    if (ET9CPIsModePhonetic(pET9CPLingInfo)) {
        status = ET9_CP_Phonetic_SelectPhrase(pET9CPLingInfo, wPhraseIndex, psSpell);
    }
#ifndef ET9CP_DISABLE_STROKE
    else if (ET9CPIsModeCangJie(pET9CPLingInfo) || ET9CPIsModeQuickCangJie(pET9CPLingInfo)) {
        status = ET9_CP_CangJieSelectPhrase(pET9CPLingInfo, wPhraseIndex);
    }
    else {
        ET9Assert(ET9CPIsModeStroke(pET9CPLingInfo));
        status = ET9_CP_StrokeSelectPhrase(pET9CPLingInfo, wPhraseIndex, &pET9CPLingInfo->CommonInfo.sActiveSpell, &bNumSymbs);
    }
#endif

    if (ET9STATUS_NONE != status && ET9STATUS_SELECTED_CHINESE_COMPONENT != status) {
        return status;
    }

    if ( status == ET9STATUS_NONE
        && pET9CPLingInfo->Base.pWordSymbInfo->bNumSymbs == ET9_CP_SelectionHistUnselectedStart(&pET9CPLingInfo->SelHistory) )
    {
        status = ET9STATUS_ALL_SYMB_SELECTED;
    }

    ET9_CP_INVALIDATE_BUILD_CLEAR_CACHE(pET9CPLingInfo);

    return status;
} /* end of ET9CPSelectPhrase() */

/** Discard all the previously selected phrases.
 *
 * See ET9CPSelectPhrase() and ET9CPUnselectPhrase().<br>
 *
 * Calling this function is equivalent to repeatedly calling ET9CPUnselectPhrase() until ET9STATUS_NO_OPERATION is returned.
 *
 * @param pET9CPLingInfo     pointer to chinese information structure.
 *
 * @return ET9STATUS_NONE               Succeeded
 * @return ET9STATUS_NO_INIT            pET9CPLingInfo is not properly initialized
 * @return ET9STATUS_NO_OPERATION       Nothing has been selected
 */
ET9STATUS ET9FARCALL ET9CPUnselectAll(ET9CPLingInfo * pET9CPLingInfo)
{
    ET9_CP_CHECK_LINGINFO(pET9CPLingInfo);
    if (pET9CPLingInfo->SelHistory.bSelectionCount != 0) {
        pET9CPLingInfo->SelHistory.bSelectionCount = 0;
        pET9CPLingInfo->SelHistory.bSelPhraseLen = 0;

        ET9_CP_INVALIDATE_BUILD_CLEAR_CACHE(pET9CPLingInfo);
        ET9_CS_ResetSBI(&pET9CPLingInfo->SBI);
        return ET9STATUS_NONE;
    }
    else
        return ET9STATUS_NO_OPERATION;
}

/** Unselect the last selected phrase.
 *
 * See ET9CPSelectPhrase().<br>
 *
 * XT9 Chinese core can handle long input, user can select phrase segment by segment. XT9 Chinese core will remember the selection history.
 * This function will unselect the last selected segment, that is, the last element in selection history is cleared.
 *
 * @param pET9CPLingInfo     pointer to chinese information structure.
 *
 * @return ET9STATUS_NONE               Succeeded
 * @return ET9STATUS_NO_INIT            pET9CPLingInfo is not properly initialized
 * @return ET9STATUS_NO_OPERATION       Nothing has been selected
 */
ET9STATUS ET9FARCALL ET9CPUnselectPhrase(ET9CPLingInfo    *pET9CPLingInfo)
{
    ET9U8 b, bOld, bNew;
    ET9_CP_CHECK_LINGINFO(pET9CPLingInfo);
    if (pET9CPLingInfo->SelHistory.bSelectionCount == 0)
        return ET9STATUS_NO_OPERATION;
    ET9CPGetSelection(pET9CPLingInfo, NULL, NULL, &bOld);

    ET9_CP_SelectionHistClear(&pET9CPLingInfo->SelHistory);

    ET9CPGetSelection(pET9CPLingInfo, NULL, NULL, &bNew);
    for ( b = bNew; b < bOld; b++ )
    {
        pET9CPLingInfo->Base.bSymbInvalidated[b] = 1;
    }
    pET9CPLingInfo->Base.bSymbsInfoInvalidated = 1;
    return ET9STATUS_NONE;
}

/** Retrieves the selected phrase and its spellings.
 *
 * See ET9CPSelectPhrase() and ET9CPUnselectPhrase().<br>
 * XT9 Chinese can handle long input and the user can select phrases segment by segment.
 * Integration layer does not need to remember what is selected. The core has a selection history module to manage the selection history.
 * At any moment, Integration layer can call this function to get the accumulated selection.<br>
 * This function retrives the accumulated selection by previous calls to ET9CPSelectPhrase()<br>
 * After ET9CPCommitSelection(), the selection history is cleared.
 *
 * @param pET9CPLingInfo     pointer to Chinese information structure.
 * @param pUnicodePhrase     the Unicode characters of the selection.
 * @param pSpell             (optional)the spelling of the selection.
 * @param pbSelSymbCount     (optional)The number of symbols consumed by the selection including any syllable delimiters.
 *
 * @return ET9STATUS_NONE               Success
 * @return ET9STATUS_NO_INIT            pET9CPLingInfo is not properly initialized
 * @return ET9STATUS_EMPTY              No selection history
 */
ET9STATUS ET9FARCALL ET9CPGetSelection(ET9CPLingInfo * pET9CPLingInfo,
                                       ET9CPPhrase * pUnicodePhrase,   /* Unicode string */
                                       ET9CPSpell* pSpell,
                                       ET9U8 * pbSelSymbCount)
{
    ET9STATUS status;

    if (pUnicodePhrase != NULL)
        pUnicodePhrase->bLen = 0;
    if (pbSelSymbCount != NULL)
        *pbSelSymbCount = 0;
    if ( pSpell )
        pSpell->bLen = 0;

    ET9_CP_CHECK_LINGINFO(pET9CPLingInfo);
    if ( !ET9CPIsModePhonetic(pET9CPLingInfo) )
        pSpell = NULL;

    if ( pSpell )  /* Must be Phonetic mode */
    {
        ET9CPPhrase phraseEncoded;
        pSpell->bLen = 0;
        status = ET9_CP_SelectionHistGet(&pET9CPLingInfo->SelHistory, pUnicodePhrase, &phraseEncoded);
        if (status == ET9STATUS_NONE && AllSymbEncoded(&phraseEncoded) )
        {
            ET9U8 i, j;
            ET9BOOL fIsBpmf = (ET9U8)ET9CPIsModeBpmf(pET9CPLingInfo);
            ET9_CP_Syl sSyl;

            for ( i = 0; i < phraseEncoded.bLen; i++ )
            {
                if (!ET9_CP_PidBidToSyllable(pET9CPLingInfo, phraseEncoded.pSymbs[i], sSyl.aChars, &(sSyl.bSize), fIsBpmf) ) {
                    pSpell->bLen = 0;
                    return ET9STATUS_NONE;
                }
                for ( j = 0; j < sSyl.bSize; j++ )
                {
                    pSpell->pSymbs[pSpell->bLen++] = ET9_CP_InternalSpellCodeToExternal(pET9CPLingInfo, sSyl.aChars[j]);
                }
            }
        }
    }
    else   /* Might be Phonetic OR Stroke mode */
    {
        status = ET9_CP_SelectionHistGet(&pET9CPLingInfo->SelHistory, pUnicodePhrase, NULL);
    }
    if (status == ET9STATUS_NONE)
    {
        if (pbSelSymbCount != NULL)
            *pbSelSymbCount = ET9_CP_SelectionHistUnselectedStart(&pET9CPLingInfo->SelHistory);
    }
    return status;
}

static void ET9LOCALCALL AddToRUDB(ET9CPLingInfo *pET9CPLingInfo,
                                   ET9CPPhrase *pPhrase,
                                   ET9_CP_IDEncode eEncode)
{
    /* concatenate the context prefix and the selection */
    ET9_CP_PhraseBuf *pMainPhraseBuf;
    ET9U16 *pwPrefix, *pwSrc, *pwDst;
    ET9U8 *pbContextLen;
    ET9CPPhrase sConcat;
    ET9U8 i;
    ET9BOOL bIsSID;
    ET9_CP_SpellMatch eMatchType;

    eMatchType = eNoMatch;
    bIsSID = (ET9BOOL)(eEncode == ET9_CP_IDEncode_SID);
    pMainPhraseBuf = ET9_CP_GetMainPhraseBuf(pET9CPLingInfo);
    pbContextLen = pET9CPLingInfo->CommonInfo.pbContextLen;
    pwPrefix = pET9CPLingInfo->CommonInfo.pwContextBuf;
    for (i = 0; pbContextLen[i]; i++) {
        ET9U8 b, bPrefixLen;
        bPrefixLen = 0;
        for (b = i; pbContextLen[b]; b++) {
            if (bPrefixLen + pbContextLen[b] + pPhrase->bLen > ET9CPMAXPHRASESIZE) {
                break;
            }
            bPrefixLen = (ET9U8)(bPrefixLen + pbContextLen[b]);
        }
        pwDst = sConcat.pSymbs;
        pwSrc = pwPrefix;
        for (b = 0; b < bPrefixLen; b++) {
            ET9Assert((pwDst - sConcat.pSymbs) < ET9CPMAXPHRASESIZE);
            *pwDst++ = *pwSrc++;
        }
        pwSrc = pPhrase->pSymbs;
        for (b = 0; b < pPhrase->bLen; b++) {
            ET9Assert((pwDst - sConcat.pSymbs) < ET9CPMAXPHRASESIZE);
            *pwDst++ = *pwSrc++;
        }
        sConcat.bLen = (ET9U8)(bPrefixLen + pPhrase->bLen);
        /* Validate concatenated string as complete phrase, no need for partial syl or partial phrase */
        ET9_CP_GetLdbPhrases(pET9CPLingInfo, bIsSID, 0, 0, &eMatchType, 0, 0, 0, sConcat.pSymbs, sConcat.bLen, 1, 0, pMainPhraseBuf);
        if (eExactMatch != eMatchType) {
            ET9_CP_GetUdbPhrases(pET9CPLingInfo, 0, 0, sConcat.pSymbs, sConcat.bLen, &eMatchType, bIsSID, 0, 0, 0, 1, pMainPhraseBuf);
        }
        if (eExactMatch == eMatchType) {
            break; /* found exact match: done */
        }
        pwPrefix += pbContextLen[i]; /* next prefix */
    }
    if (eExactMatch == eMatchType) {
        ET9_CP_UdbUsePhrase(pET9CPLingInfo, &sConcat, eEncode, 0, 1);
    }
    else { /* not a context suffix, just add the selected phrase */
        ET9_CP_UdbUsePhrase(pET9CPLingInfo, pPhrase, eEncode, 0, 1);
    }
}

/** Accepts all previously selected phrases and clears selection history.
 *
 * The integration layer calls this function to notify Chinese XT9 that all the previously selected phrases are accepted.
 * When ET9CPSelectPhrase() OR ET9CPBuildSelectionList() returns ET9STATUS_ALL_SYMB_SELECTED, the integration layer should
 * call ET9CPGetSelection() to get the full selected text, then call ET9CPCommitSelection() to notify Chinese XT9.
 * Once the phrase is committed, the integration layer should add the full selected text to the text area,
 * then call ET9ClearAllSymbs(), ET9CPBuildSelectionList() and refresh the display (Prefix, Spelling, Phrase list).
 *
 * @param pET9CPLingInfo     pointer to chinese information structure.
 *
 * @return ET9STATUS_NONE      Succeeded
 * @return ET9STATUS_NO_INIT   pET9CPLingInfo is not properly initialized
 * @return ET9STATUS_EMPTY     Selection history is empty, nothing to commit
 */
ET9STATUS ET9FARCALL ET9CPCommitSelection(ET9CPLingInfo *pET9CPLingInfo)
{
    ET9STATUS status;
    ET9CPPhrase phraseEncoded;
    ET9_CP_CHECK_LINGINFO(pET9CPLingInfo);

#ifdef EVAL_BUILD
    _ET9Eval_UpdateUsage(&pET9CPLingInfo->Base);
#endif

    status = ET9_CP_SelectionHistGet(&pET9CPLingInfo->SelHistory, NULL, &phraseEncoded);
    if (status == ET9STATUS_NONE)
    {
        if (AllSymbEncoded(&phraseEncoded)) {
            ET9_CP_IDEncode eEncode;
            if (ET9CPIsModeStroke(pET9CPLingInfo) || ET9CPIsModeCangJie(pET9CPLingInfo) || ET9CPIsModeQuickCangJie(pET9CPLingInfo)) {
                eEncode = ET9_CP_IDEncode_SID;
            }
            else if (ET9_CP_LdbHasBpmf(pET9CPLingInfo)) {
                eEncode = ET9_CP_IDEncode_BID;
            }
            else {
                ET9Assert(ET9_CP_LdbHasPinyin(pET9CPLingInfo));
                eEncode = ET9_CP_IDEncode_PID;
            }

            if (pET9CPLingInfo->SelHistory.bSelectionCount > 1 && phraseEncoded.bLen > ET9_CP_AUDB_LEN_THRESHOLD) {
                ET9U8 b = 0;
                ET9CPPhrase phraseSegment;
                while ( b < pET9CPLingInfo->SelHistory.bSelPhraseLen )
                {
                    phraseSegment.bLen = 0;
                    do
                    {
                        phraseSegment.pSymbs[phraseSegment.bLen++] = pET9CPLingInfo->SelHistory.psSelEndodedPhrase[b++];
                    } while (b < pET9CPLingInfo->SelHistory.bSelPhraseLen && pET9CPLingInfo->SelHistory.psSelUnicodePhrase[b] != ET9CP_SEGMENT_DELIMITER);
                    AddToRUDB(pET9CPLingInfo, &phraseSegment, eEncode);
                    ET9_CP_UpdateContextBuf(pET9CPLingInfo, &phraseSegment);
                    b++;
                }
            }
            else {
                AddToRUDB(pET9CPLingInfo, &phraseEncoded, eEncode);
                ET9_CP_UpdateContextBuf(pET9CPLingInfo, &phraseEncoded);
            }
        }
        else {
            /* Some character is not encoded, e.g., "99" in Ranch99 */
            ET9_CP_ClrContextBuf(pET9CPLingInfo);
        }

        ET9_CP_SelectionHistInit(&pET9CPLingInfo->SelHistory);
    }

    return status;
}


/** Enables the Smart Punctuation feature.
 *
 * See ::ET9CPClearSmartPunct
 *
 * @param pET9CPLingInfo        pointer to chinese information structure.
 *
 * @return ET9STATUS_NONE       Succeeded
 * @return ET9STATUS_NO_INIT    pET9CPLingInfo is not properly initialized
 */
ET9STATUS ET9FARCALL ET9CPSetSmartPunct(ET9CPLingInfo *pET9CPLingInfo)
{
    ET9_CP_CHECK_LINGINFO(pET9CPLingInfo);
    if (!ET9CPIsSmartPunctActive(pET9CPLingInfo)) {
        ET9_CP_INVALIDATE_BUILD_CLEAR_CACHE(pET9CPLingInfo);
        pET9CPLingInfo->bState |= (ET9U8)ET9_CP_STATE_SMARTPUNCT;
    }
    return ET9STATUS_NONE;
} /* END ET9CPSetSmartPunct() */

/** Enables the Smart Punctuation feature.
 *
 * See ::ET9CPSetSmartPunct
 *
 * @param pET9CPLingInfo        pointer to chinese information structure.
 *
 * @return ET9STATUS_NONE       Succeeded
 * @return ET9STATUS_NO_INIT    pET9CPLingInfo is not properly initialized
 */
ET9STATUS ET9FARCALL ET9CPClearSmartPunct(ET9CPLingInfo *pET9CPLingInfo)
{
    ET9_CP_CHECK_LINGINFO(pET9CPLingInfo);
    if (ET9CPIsSmartPunctActive(pET9CPLingInfo)) {
        ET9_CP_INVALIDATE_BUILD_CLEAR_CACHE(pET9CPLingInfo);
        pET9CPLingInfo->bState = (ET9U8)(pET9CPLingInfo->bState & (~ET9_CP_STATE_SMARTPUNCT));
    }
    return ET9STATUS_NONE;
} /* END ET9CPClearSmartPunct() */

ET9INT ET9FARCALL ET9_CP_AllowComponent(ET9CPLingInfo *pET9CPLingInfo)
{
    ET9_CP_PhraseBuf *pMainPhraseBuf;
    ET9Assert(pET9CPLingInfo);
    if (!ET9CPIsComponentActive(pET9CPLingInfo)) {
        return 0;
    }
    if (pET9CPLingInfo->CommonInfo.bSylCount != 1) {
        return 0;
    }
    pMainPhraseBuf = ET9_CP_GetMainPhraseBuf(pET9CPLingInfo);

    /* following if() is the delemiter expansion failed case */
    if (ET9CPSYLLABLEDELIMITER == pET9CPLingInfo->CommonInfo.bKeyBuf[pET9CPLingInfo->CommonInfo.bKeyBufLen - 1] &&
        !pMainPhraseBuf->bIsDelimiterExpansion)
    {
        return 0;
    }

    return 1;
}

/** Enables the pratial spell feature.
 *
 * See ::ET9CPClearPartialSpell
 *
 * @param pET9CPLingInfo        pointer to chinese information structure.
 *
 * @return ET9STATUS_NONE       Succeeded
 * @return ET9STATUS_NO_INIT    pET9CPLingInfo is not properly initialized
 */
ET9STATUS ET9FARCALL ET9CPSetPartialSpell(ET9CPLingInfo *pET9CPLingInfo)
{
    ET9_CP_CHECK_LINGINFO(pET9CPLingInfo);
    if (!ET9CPIsPartialSpellActive(pET9CPLingInfo)) {
        ET9_CP_INVALIDATE_BUILD_CLEAR_CACHE(pET9CPLingInfo);
        pET9CPLingInfo->bState |= (ET9U8)ET9_CP_STATE_PARTIAL_SPELL;
    }
    return ET9STATUS_NONE;
} /* END ET9CPSetPartialSpell() */

/** Disables the pratial spell feature.
 *
 * See ::ET9CPSetPartialSpell
 *
 * @param pET9CPLingInfo        pointer to chinese information structure.
 *
 * @return ET9STATUS_NONE       Succeeded
 * @return ET9STATUS_NO_INIT    pET9CPLingInfo is not properly initialized
 */
ET9STATUS ET9FARCALL ET9CPClearPartialSpell(ET9CPLingInfo *pET9CPLingInfo)
{
    ET9_CP_CHECK_LINGINFO(pET9CPLingInfo);
    if (ET9CPIsPartialSpellActive(pET9CPLingInfo)) {
        ET9_CP_INVALIDATE_BUILD_CLEAR_CACHE(pET9CPLingInfo);
        pET9CPLingInfo->bState = (ET9U8)(pET9CPLingInfo->bState & (~ET9_CP_STATE_PARTIAL_SPELL));
    }
    return ET9STATUS_NONE;
} /* END ET9CPClearPartialSpell() */


/*************************************************************************************
 *  SelectionHistory
 *************************************************************************************/
void ET9FARCALL ET9_CP_SelectionHistInit(ET9_CP_SelectionHist * pSelHist)
{
    ET9Assert(pSelHist != NULL);
    pSelHist->bSelPhraseLen = 0;
    pSelHist->bSelectionCount = 0;
}

/* return: ET9STATUS_NONE if succeeded, ET9STATUS_EMPTY if there is no selection currently */
ET9STATUS ET9FARCALL ET9_CP_SelectionHistGet(ET9_CP_SelectionHist * pSelHist,
                                            ET9CPPhrase * pUnicodePhrase,
                                            ET9CPPhrase * pEncodedPhrase)   /* PID, SID, BID string */
{
    ET9U8 b;
    ET9Assert(pSelHist != NULL);
    if (pSelHist->bSelectionCount == 0) {
        if (pEncodedPhrase != NULL)  {
            pEncodedPhrase->bLen = 0;
        }
        if (pUnicodePhrase != NULL)  {
            pUnicodePhrase->bLen = 0;
        }
        return ET9STATUS_EMPTY;
    }
    if (pEncodedPhrase != NULL)  {
        pEncodedPhrase->bLen = 0;
        for ( b = 0; b < pSelHist->bSelPhraseLen; b++ ) {
            if (pSelHist->psSelUnicodePhrase[b] != ET9CP_SEGMENT_DELIMITER) {
                ET9Assert(pEncodedPhrase->bLen < ET9CPMAXPHRASESIZE);
                pEncodedPhrase->pSymbs[pEncodedPhrase->bLen++] = pSelHist->psSelEndodedPhrase[b];
            }
        }
    }
    if (pUnicodePhrase != NULL)  {
        pUnicodePhrase->bLen = 0;
        for ( b = 0; b < pSelHist->bSelPhraseLen; b++ ) {
            if (pSelHist->psSelUnicodePhrase[b] != ET9CP_SEGMENT_DELIMITER) {
                ET9Assert(pUnicodePhrase->bLen < ET9CPMAXPHRASESIZE);
                pUnicodePhrase->pSymbs[pUnicodePhrase->bLen++] = pSelHist->psSelUnicodePhrase[b];
            }
        }
    }
    return ET9STATUS_NONE;
}

ET9U8 ET9FARCALL ET9_CP_SelectionHistUnselectedStart(const ET9_CP_SelectionHist * pSelHist)
{
    ET9Assert(pSelHist != NULL);
    if (pSelHist->bSelectionCount == 0) {
        return 0;
    }
    return pSelHist->pbSelSymbCount[pSelHist->bSelectionCount - 1];
}

/* return: ET9STATUS_NONE if succeeded, ET9STATUS_FULL if history stack is full */
ET9STATUS ET9FARCALL ET9_CP_SelectionHistAdd(ET9_CP_SelectionHist * pSelHist,
                                           const ET9SYMB * psUnicodePhrase,   /* Unicode string */
                                           const ET9SYMB * psEncodedPhrase,   /* PID, SID, BID string */
                                           ET9U8 bPraseLen,
                                           ET9U8 bConsumed)
{
    ET9Assert(pSelHist != NULL && psUnicodePhrase != NULL && bPraseLen <= ET9CPMAXPHRASESIZE);
    if (pSelHist->bSelectionCount >= ET9_CP_MAX_SEGMENT_NUMBER) {
        return ET9STATUS_FULL;
    }
    if (pSelHist->bSelPhraseLen + bPraseLen + 1 >= ET9_CP_MAX_CANDIDATE_LENGTH) {
        return ET9STATUS_FULL;
    }
    /* if (accumulated char count + new char count) > ET9CPMAXPHRASESIZE */
    if ( pSelHist->bSelectionCount > 0 && pSelHist->bSelPhraseLen - pSelHist->bSelectionCount + bPraseLen >= ET9CPMAXPHRASESIZE ) {
        return ET9STATUS_FULL;
    }

    if (pSelHist->bSelPhraseLen > 0) {
        pSelHist->psSelEndodedPhrase[pSelHist->bSelPhraseLen] = ET9CP_SEGMENT_DELIMITER;
        pSelHist->psSelUnicodePhrase[pSelHist->bSelPhraseLen] = ET9CP_SEGMENT_DELIMITER;
        pSelHist->bSelPhraseLen++;
    }

    _ET9SymCopy(pSelHist->psSelUnicodePhrase + pSelHist->bSelPhraseLen, (ET9SYMB *)psUnicodePhrase, bPraseLen);
    if (psEncodedPhrase != NULL)
        _ET9SymCopy(pSelHist->psSelEndodedPhrase + pSelHist->bSelPhraseLen, (ET9SYMB *)psEncodedPhrase, bPraseLen);
    else {
        ET9U8 b;
        for (b = 0; b < bPraseLen; b++) {
            pSelHist->psSelEndodedPhrase[pSelHist->bSelPhraseLen + b] = ET9_CP_NOT_ENCODED_SYMBOL;
        }
    }
    pSelHist->bSelPhraseLen = (ET9U8)(pSelHist->bSelPhraseLen + bPraseLen);

    if (pSelHist->bSelectionCount > 0)
        pSelHist->pbSelSymbCount[pSelHist->bSelectionCount] = (ET9U8)(pSelHist->pbSelSymbCount[pSelHist->bSelectionCount - 1] + bConsumed);
    else
        pSelHist->pbSelSymbCount[pSelHist->bSelectionCount] = bConsumed;
    pSelHist->bSelectionCount++;

    return ET9STATUS_NONE;
}

/* return: ET9STATUS_NONE if succeeded, ET9STATUS_FULL if history stack is full */
ET9STATUS ET9FARCALL ET9_CP_SelectionHistClear(ET9_CP_SelectionHist * pSelHist)
{
    ET9U8 i;
    ET9Assert(pSelHist != NULL);
    if (pSelHist->bSelectionCount == 0) {
        return ET9STATUS_EMPTY;
    }

    for ( i = (ET9U8)(pSelHist->bSelPhraseLen - 1); i > 0; i-- ) {
        if ( pSelHist->psSelUnicodePhrase[i] == ET9CP_SEGMENT_DELIMITER ) {
            break;
        }
    }
    pSelHist->bSelPhraseLen = i;

    pSelHist->bSelectionCount--;

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------
*
*  Function: ET9_CP_ClearBuildCache
*
*  Synopsis: Reset the global database search variables.
*
*      Input: pET9CPLingInfo = Pointer to Chinese XT9 LingInfo structure.
*
*      Return:
*
*---------------------------------------------------------------------------*/
void ET9FARCALL ET9_CP_ClearBuildCache(ET9CPLingInfo * pET9CPLingInfo)
{
    ET9_CP_CommonInfo *pCommon;
    ET9_CP_PhraseBuf *pMainPhraseBuf;

    pMainPhraseBuf = ET9_CP_GetMainPhraseBuf(pET9CPLingInfo);
    ET9_CP_ZeroPhraseBuffer(pMainPhraseBuf);

    pCommon = &(pET9CPLingInfo->CommonInfo);
    pCommon->bKeyBufLen = 0;
    pCommon->bSylCount = 0;
    pCommon->bCachedNumSymbs = 0;
    pCommon->bFailNumSymbs = 0xFF;
    pCommon->sActiveSpell.bLen = 0; /* clear active spell */

#ifndef ET9CP_DISABLE_STROKE
    pET9CPLingInfo->Private.SPrivate.wCompUnicode = 0;
    pET9CPLingInfo->Private.SPrivate.bCompNumStrokes = 0;
    ET9_CP_StrokeKeysChangeInit(pET9CPLingInfo);
#endif

    pCommon->SpellData.sSpell.bLen = 0;
    pCommon->SpecialPhrase.eLastAlphaPhraseType = ET9CPPhraseType_Unknown;
    pCommon->eCandMatchType = eNoMatch;
    ET9_CS_ResetSBI(&pET9CPLingInfo->SBI);
    ET9_CS_ClearPrefixBuf(&pET9CPLingInfo->SBI);

    /* todo: clear AW spellings? */
}   /* end of ET9_CP_ClearBuildCache() */

/** Retrieve the current spelling
 *
 * Spellings are a representation of XT9's interpretation of the user's input, useful in providing feedback
 * to the user. They correspond closely to the keys the user has pressed.
 *
 * Phonetic spellings show XT9's disambiguation and segmentation of the user's input. For instance, phonepad
 * input of "234234234234" might become a spelling of "BeiBei_BeiBei", showing how XT9 has interpreted the
 * user's input.
 * Phonetic spellings are expressed as follows:
 *  - The beginning of each syllable is upper-case, the rest are lower-case. For example: BeiJing.
 *  - Note uppercase BPMF will be expressed by values offset from ET9CPBpmfFirstUpperSymbol
 *  - Delimiters and tones entered by the user are also included. For example: Bei3 or Xi'An
 *  - Tones are expressed as ET9CPTONE1-ET9CPTONE5
 *  - Delimiters are expressed as ET9CPSYLLABLEDELIMITER
 *  - Segments are seperated by ET9CP_SEGMENT_DELIMITER, For example: WoMen_Qu_ChiFan
 *
 * Stroke spellings show the keys the user has entered and the current component that has been selected.
 * - The format is as follows:
 *  - [current component], S1, S2, ...
 *     where current component is present only if the user has selected a component, S1, S2, ... are strokes entered after the component.
 * - Strokes are expressed as one of these enum values:
 *  - ET9CPSTROKE1,
 *  - ET9CPSTROKE2,
 *  - ET9CPSTROKE3,
 *  - ET9CPSTROKE4,
 *  - ET9CPSTROKE5,
 *  - ET9CPSTROKEWILDCARD
 * - Delimiters are expressed as ET9CPSYLLABLEDELIMITER
 * - The current component is expressed by values in the range defined by ET9CPGetFirstComponent() and ET9CPGetLastComponent().
 *   It is stored as the 1st symbol in the spelling and is counted as one symbol in the spelling length.
 *
 * Cangjie and QuickCangjie spellings are cangjie codes composed of A-Z.
 *
 * @param pET9CPLingInfo     pointer to chinese information structure.
 * @param psSpell            A spelling data structure where XT9 Chinese will write the spelling and its length.
 *
 * @return ET9STATUS_NONE               Success
 * @return ET9STATUS_NO_INIT            pET9CPLingInfo is not properly initialized
 * @return ET9STATUS_NEED_SELLIST_BUILD Should call ET9CPBuildSelectionList() before calling this function
 * @return ET9STATUS_BAD_PARAM          some argument pointer is NULL
 *
 */
ET9STATUS ET9FARCALL ET9CPGetSpell(ET9CPLingInfo *pET9CPLingInfo, ET9CPSpell *psSpell)
{
    ET9_CP_CHECK_LINGINFO(pET9CPLingInfo);
    ET9_CP_CHECK_UDB_UP_TO_DATE(pET9CPLingInfo);
    ET9_CP_CHECK_BUILD_UP_TO_DATE(pET9CPLingInfo);

    if (!psSpell) {
        return ET9STATUS_BAD_PARAM;
    }
    ET9_CP_ToExternalSpellInfo(pET9CPLingInfo, &(pET9CPLingInfo->CommonInfo.sActiveSpell), psSpell);
    return ET9STATUS_NONE;
}

/** Retrieve special (non-Chinese) phrases.
 *
 *  This allows matching the ET9WordSymbInfo associated with the ET9CPLingInfo to different phrase types, such as 
 *  punctuation, numbers, and alphabetic words.
 *
 *  @param pET9CPLingInfo     pointer to chinese information structure.
 *  @param ePhraseType        The desired type of phrase.
 *  @param wPhraseIndex       0-based index.
 *  @param psPhrase           A phrase data structure where XT9 Chinese will write the requested phrase and its length.
 *
 *  @return ET9STATUS_NONE               Success
 *  @return ET9STATUS_NO_INIT            pET9CPLingInfo is not properly initialized
 *  @return ET9STATUS_BAD_PARAM          some argument is invalid
 *  @return ET9STATUS_INVALID_MEMORY     some argument is invalid
 *  @return ET9STATUS_INVALID_INPUT      There is no candidate phrase OR current input is from Trace
 *  @return ET9STATUS_OUT_OF_RANGE       wPhraseIndex is too big
 *---------------------------------------------------------------------------*/
ET9STATUS ET9FARCALL ET9CPGetSpecialPhrase(ET9CPLingInfo *pET9CPLingInfo, ET9CPPhraseType ePhraseType, ET9U16 wPhraseIndex, ET9CPPhrase *psPhrase)
{
    ET9STATUS status = ET9STATUS_BAD_PARAM;

    /* validate inputs */
    ET9_CP_CHECK_LINGINFO(pET9CPLingInfo);
    if (NULL == psPhrase) {
        return ET9STATUS_BAD_PARAM;
    }

    if (ET9_CP_InputContainsTrace(pET9CPLingInfo)) {
        return ET9STATUS_INVALID_INPUT;
    }

    switch (ePhraseType) {
        case ET9CPPhraseType_ExactInput:
            if (0 == wPhraseIndex) {
                status = ET9_CP_MakeExactInputPhrase(pET9CPLingInfo, psPhrase);
            }
            else {
                status = ET9STATUS_OUT_OF_RANGE;
            }
            break;
        case ET9CPPhraseType_Num:
            if (0 == wPhraseIndex) {
                status = ET9_CP_MakeNumPhrase(pET9CPLingInfo, psPhrase);
            }
            else {
                status = ET9STATUS_OUT_OF_RANGE;
            }
            break;
        case ET9CPPhraseType_Sym:
            status = ET9_CP_GetSymPhrase(pET9CPLingInfo, psPhrase, wPhraseIndex);
            break;
        case ET9CPPhraseType_abc:
        case ET9CPPhraseType_Abc:
        case ET9CPPhraseType_ABC:
            status = ET9_CP_GetAlphaPhrase(pET9CPLingInfo, psPhrase, wPhraseIndex, ePhraseType);
            break;
        default:
            /* invalid phrase type */
            status = ET9STATUS_BAD_PARAM;
            break;
    }

    if (ET9STATUS_NO_MATCHING_WORDS == status) {
        status = ET9STATUS_OUT_OF_RANGE;
    }

    return status;
}

/** Select a special (non-Chinese) phrase.
 *
 *  This informs the core that the user has selected a special phrase. The core will take appropriate action to update
 *  any databases accordingly. The integration layer should insert the phrase into the application's text buffer and
 *  clear the ET9WordSymbInfo. Note that unlike when selecting Chinese phrases with ET9CPSelectPhrase, there is no need
 *  to call ET9CPCommitSelection.  Only need to call ET9ClearAllSymbs.
 *
 *  @param pET9CPLingInfo     pointer to chinese information structure.
 *  @param ePhraseType        The type of phrase selected.
 *  @param wPhraseIndex       0-based index.
 *
 *  @return ET9STATUS_ALL_SYMB_SELECTED  Success--all input has been selected, caller should call ET9ClearAllSymbs()
 *  @return ET9STATUS_NO_INIT            pET9CPLingInfo is not properly initialized
 *  @return ET9STATUS_INVALID_MEMORY     Some argument pointer is NULL
 *  @return ET9STATUS_BAD_PARAM          some argument is invalid
 *  @return ET9STATUS_INVALID_INPUT      There is no candidate phrase OR current input is from Trace
 *  @return ET9STATUS_OUT_OF_RANGE       wPhraseIndex is too big
 *---------------------------------------------------------------------------*/
ET9STATUS ET9FARCALL ET9CPSelectSpecialPhrase(ET9CPLingInfo *pET9CPLingInfo, ET9CPPhraseType ePhraseType, ET9U16 wPhraseIndex)
{
    ET9STATUS status = ET9STATUS_BAD_PARAM;

    /* validate inputs */
    ET9_CP_CHECK_LINGINFO(pET9CPLingInfo);

    if (ET9_CP_InputContainsTrace(pET9CPLingInfo)) {
        return ET9STATUS_INVALID_INPUT;
    }

    switch (ePhraseType) {
        case ET9CPPhraseType_ExactInput:
        case ET9CPPhraseType_Num:
        case ET9CPPhraseType_Sym:
            status = ET9STATUS_NONE;
            break;
        case ET9CPPhraseType_abc:
        case ET9CPPhraseType_Abc:
        case ET9CPPhraseType_ABC:
#ifdef ET9_ALPHABETIC_MODULE
            {
                ET9CPPhrase phrase;
                if (!ET9CPIsAWActive(pET9CPLingInfo)) {
                    return ET9STATUS_NO_INIT;
                }
                if (0x100 <= wPhraseIndex) {
                    /* if index is larger than a ET9U8 value, it's too big */
                    return ET9STATUS_OUT_OF_RANGE;
                }
                /* ensure core state is fresh wrt shift state, build, etc. */
                ET9_CP_GetAlphaPhrase(pET9CPLingInfo, &phrase, wPhraseIndex, ePhraseType);
                status = ET9AWSelLstSelWord(pET9CPLingInfo->pAWLing, (ET9U8)wPhraseIndex);
                status = ET9AWNoteWordDone( pET9CPLingInfo->pAWLing, phrase.pSymbs, phrase.bLen );
            }
#else
            pET9CPLingInfo;
            wPhraseIndex;
#endif
            break;
        default:
            /* invalid phrase type */
            status = ET9STATUS_BAD_PARAM;
            break;
    }
    if (ET9STATUS_NONE == status) {
        status = ET9STATUS_ALL_SYMB_SELECTED;
    }

    return status;
}

/** Enables the Chinese Common Character feature.
 * See ::ET9CPClearCommonChar
 *
 *      @param pET9CPLingInfo     pointer to chinese information structure
 *
 *      @return ET9STATUS_NONE     succeeded.
 *      @return ET9STATUS_NO_INIT  pET9CPLingInfo is not properly initialized.
 */
ET9STATUS ET9FARCALL ET9CPSetCommonChar(ET9CPLingInfo *pET9CPLingInfo)
{
    ET9_CP_CHECK_LINGINFO(pET9CPLingInfo);
    if (!ET9CPIsCommonCharActive(pET9CPLingInfo)) {
        ET9_CP_INVALIDATE_BUILD_CLEAR_CACHE(pET9CPLingInfo);
        pET9CPLingInfo->bState |= (ET9U8)ET9_CP_STATE_COMMON_CHAR;
    }
    return ET9STATUS_NONE;
} /* END ET9CPSetCommonChar() */

/** Disables the Chinese Common Character feature.
 * See ::ET9CPSetCommonChar
 *
 *      @param pET9CPLingInfo     pointer to chinese information structure
 *
 *      @return ET9STATUS_NONE     succeeded.
 *      @return ET9STATUS_NO_INIT  pET9CPLingInfo is not properly initialized.
 */
ET9STATUS ET9FARCALL ET9CPClearCommonChar(ET9CPLingInfo *pET9CPLingInfo)
{
    ET9_CP_CHECK_LINGINFO(pET9CPLingInfo);
    if (ET9CPIsCommonCharActive(pET9CPLingInfo)) {
        ET9_CP_INVALIDATE_BUILD_CLEAR_CACHE(pET9CPLingInfo);
        pET9CPLingInfo->bState = (ET9U8)(pET9CPLingInfo->bState & (~ET9_CP_STATE_COMMON_CHAR));
    }
    return ET9STATUS_NONE;
} /* END ET9CPClearCommonChar() */

/** Enables Whole Sentence in Selection list.
 * See ::ET9CPClearFullSentence
 *
 *  If this function succeeds, caller should refresh the selection list.
 *
 *      @param pET9CPLingInfo     pointer to chinese information structure
 *
 *      @return ET9STATUS_NONE     succeeded.
 *      @return ET9STATUS_NO_INIT  pET9CPLingInfo is not properly initialized.
 */
ET9STATUS ET9FARCALL ET9CPSetFullSentence(ET9CPLingInfo *pET9CPLingInfo)
{
    ET9_CP_CHECK_LINGINFO(pET9CPLingInfo);

    if ( !ET9CPIsFullSentenceActive(pET9CPLingInfo) ) {
        pET9CPLingInfo->bState |= (ET9U8)ET9_CP_STATE_FULL_SENTENCE;
        if ( ET9CPIsModePhonetic(pET9CPLingInfo) )   {
            ET9_CP_ClearPhraseBuf(ET9_CP_GetMainPhraseBuf(pET9CPLingInfo));
        }
    }
    return ET9STATUS_NONE;
} /* END ET9CPSetFullSentence() */

/** Disables Whole Sentence in Selection list.
 * See ::ET9CPSetFullSentence
 *
 *  If this function succeeds, caller should refresh the selection list.
 *
 *      @param pET9CPLingInfo     pointer to chinese information structure
 *
 *      @return ET9STATUS_NONE     succeeded.
 *      @return ET9STATUS_NO_INIT  pET9CPLingInfo is not properly initialized.
 */
ET9STATUS ET9FARCALL ET9CPClearFullSentence(ET9CPLingInfo *pET9CPLingInfo)
{
    ET9_CP_CHECK_LINGINFO(pET9CPLingInfo);

    if ( ET9CPIsFullSentenceActive(pET9CPLingInfo) ) {
        pET9CPLingInfo->bState = (ET9U8)(pET9CPLingInfo->bState & (~ET9_CP_STATE_FULL_SENTENCE));
        if ( ET9CPIsModePhonetic(pET9CPLingInfo) )   {
            ET9_CP_ClearPhraseBuf(ET9_CP_GetMainPhraseBuf(pET9CPLingInfo));
        }
    }

    return ET9STATUS_NONE;
} /* END ET9CPClearFullSentence() */


/** Given a base phrase in Unicode, get the number of phrases that have the same pronounciation
 *  as the base phrase (or its variant pronounciations).
 *
 *  This function is useful to determine the range of index that can be used in ET9CPGetHomophonePhrase.
 *
 * @param pET9CPLingInfo     pointer to chinese information structure.
 * @param pBasePhrase        the base phrase (in Unicode).
 * @param pwCount            to output the phrase count.
 *
 * @return ET9STATUS_NONE               Succeeded
 * @return ET9STATUS_NO_INIT            pET9CPLingInfo is not properly initialized
 * @return ET9STATUS_BAD_PARAM          some parameter is invalid
 */
ET9STATUS ET9FARCALL ET9CPGetHomophonePhraseCount(ET9CPLingInfo     *pET9CPLingInfo,
                                                  const ET9CPPhrase *pBasePhrase,
                                                  ET9U16            *pwCount)
{
    ET9_CP_CHECK_LINGINFO(pET9CPLingInfo);

    if (   NULL == pwCount
        || NULL == pBasePhrase
        || 0 == pBasePhrase->bLen
        || pBasePhrase->bLen > ET9CPMAXPHRASESIZE) {
        return ET9STATUS_BAD_PARAM;
    }

    {
        ET9_CP_PhraseBuf *pPhraseBuf;
        ET9_CP_StdPhraseBuf sHomophonePhraseBuf;

        ET9_CP_INIT_PHRASE_BUF(sHomophonePhraseBuf);
        pPhraseBuf = (ET9_CP_PhraseBuf*)(&sHomophonePhraseBuf);
        ET9_CP_GetHomophone(pET9CPLingInfo, pBasePhrase, pPhraseBuf);
        *pwCount = pPhraseBuf->wTotal;
    }
    return ET9STATUS_NONE;
}

/** Given a base phrase in Unicode, get the phrase that have the same pronounciation
 *  as the base phrase (or its variant pronounciations) by index.
 *
 * @param pET9CPLingInfo     pointer to chinese information structure.
 * @param pBasePhrase        the base phrase (in Unicode).
 * @param wIndex             0-based index of the desired phrase.
 * @param pPhrase            structure to output the desired phrase (in Unicode).
 * @param pSpell             (optional)structure to output the spelling of the desired phrase. Supported in Phonetic modes only.
 *
 * @return ET9STATUS_NONE               Succeeded
 * @return ET9STATUS_NO_INIT            pET9CPLingInfo is not properly initialized
 * @return ET9STATUS_BAD_PARAM          some parameter is invalid or Ldb has no phonetic data
 * @return ET9STATUS_OUT_OF_RANGE       Specified index too big
 *
 */
ET9STATUS ET9FARCALL ET9CPGetHomophonePhrase(ET9CPLingInfo      *pET9CPLingInfo,
                                             const ET9CPPhrase  *pBasePhrase,
                                             ET9U16             wIndex,
                                             ET9CPPhrase        *pPhrase,
                                             ET9CPSpell         *pSpell)
{
    ET9_CP_CHECK_LINGINFO(pET9CPLingInfo);
    if (   NULL == pBasePhrase
        || 0 == pBasePhrase->bLen
        || pBasePhrase->bLen > ET9CPMAXPHRASESIZE
        || NULL == pPhrase
        || !ET9_CP_LdbHasPhonetic(pET9CPLingInfo)
        || (pSpell && !ET9CPIsModePhonetic(pET9CPLingInfo) ) ) { /* pSpell is supported in Phonetic modes only */
        return ET9STATUS_BAD_PARAM;
    }
    {
        ET9_CP_PhraseBuf *pPhraseBuf;
        ET9_CP_StdPhraseBuf sHomophonePhraseBuf;

        ET9_CP_INIT_PHRASE_BUF(sHomophonePhraseBuf);
        pPhraseBuf = (ET9_CP_PhraseBuf*)(&sHomophonePhraseBuf);
        ET9_CP_GetHomophone(pET9CPLingInfo, pBasePhrase, pPhraseBuf);
        if (wIndex >= pPhraseBuf->wTotal) {
            return ET9STATUS_OUT_OF_RANGE;
        }
        ET9_CP_GetPhraseFromBuf(pPhraseBuf, pPhrase, NULL, wIndex+1);
        if (pSpell) { /* output spelling */
            ET9_CP_Spell sInternalSpell;
            ET9_CP_PidBidToSpelling(pET9CPLingInfo, pPhrase->pSymbs, pPhrase->bLen, &sInternalSpell);
            ET9_CP_ToExternalSpellInfo(pET9CPLingInfo, &sInternalSpell, pSpell);
        }
        /* convert phrase to Unicode */
        ET9_CP_ConvertPhraseToUnicode(pET9CPLingInfo, pPhrase, ET9_CP_IsLdbPinyinOnly(pET9CPLingInfo) ? ET9_CP_IDEncode_PID : ET9_CP_IDEncode_BID);
    }
    return ET9STATUS_NONE;
}

/*! \mainpage

\section intro_sec XT9 Chinese 7.0 Release Summary

SOURCE CODE: Please be aware that you must obtain approval from Nuance Communications
prior to making any modifications to XT9 source code.<p>

\subsection  Sub_SBI Sentence based input
Starting from XT9 Chinese Version 4.0, Sentence based input is supported by the core.  <p>

In phonetic (PinYin or BoPoMoFo) mode, the core can accept inputs even if they do not form a phrase in the database.
The best segmentation is generated automatically. A <b>segmentation</b> is the spelling of a sentence.
It contains one or more segments seperated by ET9CP_SEGMENT_DELIMITER.
Each <b>segment</b> is the spelling of a phrase found in the database.
Within a segment, each syllable begins with an upper case letter and can be optionally followed by a
ET9CPSYLLABLEDELIMITER entered by the user.<p>

Scenario 1: (user selects phrases <b>segment by segment</b> to complete the long sentence input)

 -# User wants to enter @htmlonly &#x5317;&#x6d77;&#x516c;&#x56ed; @endhtmlonly in PinYin mode on a 12 key phone pad,
   he/she enters key 23442446649826.
   (API call: ET9CPBuildSelectionList())
 -# The segmentation will be <b>BeiHai_GongYuan</b>,
   where '_' represents ET9CP_SEGMENT_DELIMITER. It has 2 segments: <b>BeiHai</b> and <b>GongYuan</b>.
   Each segment is the spelling of a phrase in our database.
   (API call: ET9CPGetSpell())
 -# The phrase candidate (selection list) will contain @htmlonly &#x5317;&#x6d77;, &#x88ab;&#x5bb3;, &#x8d1d;, &#x88ab;, ... @endhtmlonly.
   The first two phrase candidates corresponding to the first segment <b>BeiHai</b>, and the other phrase candidates
   correspond to the first segment of other segmentations.
   (API call: ET9CPGetPhrase())
 -# User can select the phrase @htmlonly &#x5317;&#x6d77;  @endhtmlonly.
   (API call: ET9CPSelectPhrase())
 -# After this selection, the remaining inputs are 46649826, the new segmentation will be <b>GongYuan</b> (one segment),
   the the selection list will contain @htmlonly &#x516c;&#x56ed;, &#x516c;&#x5143;, &#x516c;, &#x5de5;, ... @endhtmlonly.
   (API call: ET9CPBuildSelectionList(), ET9CPGetSpell(), ET9CPGetPhrase())
 -# User then selects @htmlonly &#x516c;&#x56ed;@endhtmlonly. All the inputs are consumed.
   (API call: ET9CPSelectPhrase())
   This ends the sentence input process.
   (API call: ET9CPCommitSelection()) <p> 

Scenario 2: (user changes <b>prefix</b> to change segmentation and selection list)

 -# User wants to enter @htmlonly &#x897f;&#x5b89;&#x516c;&#x56ed; @endhtmlonly in PinYin mode on a 12 key phone pad,
   he/she enters key 942646649826.
   (API call: ET9CPBuildSelectionList())
 -# The segmentation will be <b>Xian_GongYuan</b>. The selection list will contain @htmlonly &#x73b0;, &#x5148;, ... @endhtmlonly.
   Another segmentation <b>XiAn_GongYuan</b> is not given since the core gives only the best segmentation with highest probability.
   (API call: ET9CPGetSpell())
 -# However, the core gives a list of possible prefixes <b>Xiang, Zhang, ... Yi, Xi, ...</b>.
   (API call: ET9CPGetPrefix())
 -# A <b>prefix</b> is a possible first syllable for the given input 942646649826.
 -# User can select the prefix <b>Xi</b>, to indicate that the desired first syllable is <b>Xi</b>.
   (API call: ET9CPSetActivePrefix())
 -# Then the core automatically changes the segmentation to <b>XiAn_GongYuan</b>, which is the best segmentation
   with first syllable being Xi.
   (API call: ET9CPGetSpell())
 -# The selection list will contain @htmlonly &#x897f;&#x5b89;, &#x7a00;&#x8584;, ...,
   where &#x7a00;&#x8584; @endhtmlonly comes from a different segmentation <b>XiBo_GongYuan</b>, whose first syllable
   is also <b>Xi</b>.
   (API call: ET9CPGetPhrase())
 -# User can select the phrase @htmlonly &#x897f;&#x5b89;@endhtmlonly.
   (API call: ET9CPSelectPhrase())
 -# After this selection, the remaining inputs are 46649826, the new segmentation will be <b>GongYuan</b> (one segment),
   the the selection list will contain @htmlonly &#x516c;&#x56ed;, &#x516c;&#x5143;, &#x516c;, &#x5de5;, ... @endhtmlonly.
   (API call: ET9CPBuildSelectionList(), ET9CPGetSpell(), ET9CPGetPhrase())
 -# User then selects @htmlonly &#x516c;&#x56ed;@endhtmlonly. All inputs are consumed.
   (API call: ET9CPSelectPhrase())
   This ends the sentence input process.
   (API call: ET9CPCommitSelection()) <p> 

Scenario 3: (user selects <b>full sentence</b> to complete the long sentence input)

 -# User wants to enter @htmlonly &#x5317;&#x6d77;&#x516c;&#x56ed; @endhtmlonly and see the full sentence @htmlonly &#x5317;&#x6d77;&#x516c;&#x56ed; @endhtmlonly in Scenario 1.
   by calling: ET9CPSetFullSentence()
 -# User enters key 23442446649826.
   (API call: ET9CPBuildSelectionList())
 -# The segmentation will be <b>BeiHai_GongYuan</b>,
   (API call: ET9CPGetSpell())
 -# The phrase candidate (selection list) will contain @htmlonly &#x5317;&#x6d77;&#x516c;&#x56ed;, &#x5317;&#x6d77;, ... @endhtmlonly.
   The first phrase candidates corresponding to the whole segmentation <b>BeiHai_GongYuan</b>, and the other phrase candidates
   correspond to the first segment of other segmentations.
   (API call: ET9CPGetPhrase())
 -# User can select the full sentence @htmlonly &#x5317;&#x6d77;&#x516c;&#x56ed;  @endhtmlonly.
   (API call: ET9CPSelectPhrase())
   This ends the sentence input process.
   (API call: ET9CPCommitSelection()) <p> 

The UI can display the segmentation and prefix list to the user.
User can filter selection list by setting an active prefix.
User can select a phrase from the selection list. After a phrase is selected, a new segmentation and new prefix list is generated.
Repeat the above precess, user can select phrase segment by segment.
Eventually, when all inputs are consumed, the user completes the sentence input process.<p>

Before calling ET9CPCommitSelection(), user can unselect a phrase if they accidentally selected a wrong one. This makes the clear/retype unnecessary.

\subsection  Sub_CangJie Cang Jie input
Starting from XT9 Chinese Version 5.1, Cang Jie input and Quick Cang Jie input for traditional Chinese is supported by the core.
Current implementation is Cang Jie version 3. HKSCS CangJie support is optional.<p>
In Cang Jie and Quick Cang Jie mode, the core will give the character candidates of the exact match followed by the partial match.<p>
The Cang Jie and Quick Cang Jie input supports Qwerty keyboard and multitap phone keyboard.

<p>

\subsection  Sub_OneStep One Step input
Starting from Version 6.2, XT9 Chinese supports one step input.  That means the selection list will contain all phrases
whose spelling matches the current input, even if their spellings are not the best segmentation.

Scenario 1:

 -# User wants to enter @htmlonly &#x665a;&#x996d; @endhtmlonly in PinYin mode on a 12 key phone pad,
   he/she enters key 926326.
   (API call: ET9CPBuildSelectionList())
 -# The segmentation will be <b>ZaoFan</b> because it is the best segmentation for the current input.
   (API call: ET9CPGetSpell())
 -# The selection list will contain @htmlonly &#x65e9;&#x996d;, &#x665a;&#x996d;, &#x906d;&#x5230; ...,
   where &#x665a;&#x996d; @endhtmlonly comes from a different segmentation <b>WanFan</b>.
   (API call: ET9CPGetPhrase())

Scenario 2:

 -# User wants to enter @htmlonly &#x65e9;&#x5230; @endhtmlonly in PinYin mode on a 12 key phone pad,
   he/she enters key 926326.
   (API call: ET9CPBuildSelectionList())
 -# The segmentation will be <b>ZaoFan</b> because it is the best segmentation for the current input.
   (API call: ET9CPGetSpell())
 -# The selection list does not contain @htmlonly &#x65e9;&#x5230; @endhtmlonly.
   (API call: ET9CPGetPhrase())
 -# However, the prefix list will contain <b>Zao, Yao, ...</b>.
   (API call: ET9CPGetPrefix())
 -# The user can select <b>Zao</b> from the prefix list.
   (API call: ET9CPSetActivePrefix())
 -# The selection list will be filtered and contain @htmlonly &#x65e9;, ...@endhtmlonly
   (API call: ET9CPGetPhrase())
 -# The user can select @htmlonly &#x65e9; @endhtmlonly from the selection list.
   (API call: ET9CPSelectPhrase())
 -# The segmentation of the remaining input will become <b>Dao</b>. And the selection list
   will contain @htmlonly &#x5230;, ...@endhtmlonly
   (API call: ET9CPBuildSelectionList(), ET9CPGetSpell(), ET9CPGetPhrase())
 -# The user can select @htmlonly &#x5230; @endhtmlonly from the selection list.
   (API call: ET9CPSelectPhrase())
 -# This will consume all inputs and complete the process.
   (API call: ET9CPCommitSelection()) <p>

\subsection  Sub_PartialSpelling Partial Spelling input
Starting from Version 6.2, XT9 Chinese supports partial spelling input. That means user can enter the leading letter
instead of the complete syllable for any of the syllables within the whole input.<p>
Note:
 -# For syllables starting with <b>Zh, Ch, or Sh</b>, the user can enter <b>Zh, Ch, or Sh</b> to represent the
   syllable. This can filter out phrases from other syllables such as Zi, Ci, and Si.
 -# This feature is supported in Pinyin mode only because BPMF has at most 3 letters per syllable and this will lead
   to more confusion than benefits.

Scenario 1:

 -# User wants to enter @htmlonly &#x597d;&#x73a9; @endhtmlonly in Pinyin mode on a Qwerty keyboard,
   he/she enters "hwan", where 'h' represent the syllable "Hao" and the syllable "Wan" is entered completely.
   (API call: ET9CPBuildSelectionList())
 -# The segmentation will be <b>HWan</b> because it is the best segmentation for the current input.
   (API call: ET9CPGetSpell())
 -# The selection list will contain @htmlonly &#x597d;&#x73a9;, &#x5f88;&#x665a;, &#x6d77;&#x6e7e; ...@endhtmlonly
   (API call: ET9CPGetPhrase())

Scenario 2:

 -# User wants to enter @htmlonly &#x5403;&#x996d; @endhtmlonly in Pinyin mode on a Qwerty keyboard,
   he/she enters "chfan", where 'ch' represent the syllable "Chi" and the syllable "Fan" is entered completely.
   (API call: ET9CPBuildSelectionList())
 -# The segmentation will be <b>ChFan</b> because it is the best segmentation for the current input.
   (API call: ET9CPGetSpell())
 -# The selection list will contain @htmlonly &#x5403;&#x996d;, &#x7092;&#x996d;, &#x91cd;&#x8fd4; ...@endhtmlonly
   (API call: ET9CPGetPhrase())

<p>
*/

/* ----------------------------------< eof >--------------------------------- */
