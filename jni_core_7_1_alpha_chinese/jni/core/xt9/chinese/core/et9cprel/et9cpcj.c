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
;**     FileName: et9cpcj.c                                                   **
;**                                                                           **
;**  Description: Cang Jie input support                              .       **
;**                                                                           **
;*******************************************************************************
;******************************************************************************/

#include "et9api.h"
#include "et9cpldb.h"
#include "et9cpsys.h"
#include "et9misc.h"
#include "et9cppbuf.h"
#include "et9cpspel.h"
#include "et9cpcj.h"
#include "et9cprdb.h"
#include "et9cpname.h"
#include "et9cpwdls.h"
#include "et9cprdb.h"

#ifndef ET9CP_DISABLE_STROKE

#define ET9_CP_CUTOFF_THRSHLD      8
#define ET9_CP_QSORT_STACK_SIZE   30


/*****************************************************************************************************************/
typedef ET9INT (ET9LOCALCALL* COMPARE_FCT)(const ET9_CP_SidFreq*, const ET9_CP_SidFreq*);

static ET9INT ET9LOCALCALL CompareSid(const ET9_CP_SidFreq* p, const ET9_CP_SidFreq* q)
{
    if ( p->wSid > q->wSid )
        return 1;
    else if ( p->wSid < q->wSid )
        return -1;
    else
        return 0;
}

static void ET9LOCALCALL Swap( ET9_CP_SidFreq* a, ET9_CP_SidFreq* b )
{
    ET9_CP_SidFreq tmp;
    if ( a != b )
    {
        tmp = *a;
        *a = *b;
        *b = tmp;
    }
}

static void ET9LOCALCALL InserionSort( ET9_CP_SidFreq* pF, ET9_CP_SidFreq* pL, COMPARE_FCT comp )
{
    ET9_CP_SidFreq* pMax, *p;
    while ( pL > pF )
    {
        pMax = pF;
        for (p = pF+1; p <= pL; p++)
        {
            if (comp(p, pMax) > 0)
                pMax = p;
        }

        Swap(pMax, pL);
        pL--;
    }
}

static ET9_CP_SidFreq ET9LOCALCALL PivotValue(ET9_CP_SidFreq* pLow, ET9_CP_SidFreq* pHigh, COMPARE_FCT comp)
{
    ET9_CP_SidFreq* pMid = pLow + (pHigh - pLow + 1) / 2;

    if ( comp( pLow, pMid ) > 0 )
        Swap(pLow, pMid);
    if ( comp( pLow, pHigh ) > 0 )
        Swap(pLow, pHigh);
    if ( comp( pMid, pHigh ) > 0)
        Swap(pMid, pHigh);
    return *pMid;
}

static ET9_CP_SidFreq* ET9LOCALCALL Partition( ET9_CP_SidFreq*  pLow, ET9_CP_SidFreq*  pHigh, ET9_CP_SidFreq* pPivot, COMPARE_FCT comp )
{
    ET9_CP_SidFreq* pL = pLow;
    ET9_CP_SidFreq* pH = pHigh;
    for (;;)
    {
        do
        {
            pL++;
        } while ( pL < pH && comp(pL, pPivot) < 0 );

        do
        {
            pH--;
        } while ( pH >= pL && comp(pH, pPivot) >= 0 );
        if ( pH < pL )
            return pL;

        Swap(pL, pH);
    }
}

static void ET9LOCALCALL InternalQSort ( ET9_CP_SidFreq* pLow, ET9_CP_SidFreq* pHigh, COMPARE_FCT comp )
{
    ET9_CP_SidFreq*  pLow2, *pHigh1;

    ET9_CP_SidFreq* aLowStack[ET9_CP_QSORT_STACK_SIZE];
    ET9_CP_SidFreq* aHighStack[ET9_CP_QSORT_STACK_SIZE];
    ET9INT nStackTop = 0;

    if (pHigh - pLow + 1 < 2)
        return;

loop_label:
    if (pHigh - pLow + 1 <= ET9_CP_CUTOFF_THRSHLD)
        InserionSort(pLow, pHigh, comp);
    else
    {
        ET9_CP_SidFreq pivot = PivotValue( pLow, pHigh, comp );
        pLow2 = Partition( pLow, pHigh, &pivot, comp );
        pHigh1 = pLow2 - 1;
        while (pLow2 < pHigh && comp(pLow2, &pivot) == 0)
        {
            pLow2++;
        }
        while (pHigh1 > pLow && comp(pHigh1, &pivot) == 0)
        {
            pHigh1--;
        }

        /* Now, we have intervals [pLow, pHigh1] and [pLow2, pHigh]  */
        if ( pHigh1 - pLow >= pHigh - pLow2 )
        {
            if (pLow < pHigh1)
            {
                aLowStack[nStackTop] = pLow;
                aHighStack[nStackTop] = pHigh1;
                ++nStackTop;
            }

            if (pLow2 < pHigh)
            {
                pLow = pLow2;
                goto loop_label;
            }
        }
        else
        {
            if (pLow2 < pHigh)
            {
                aLowStack[nStackTop] = pLow2;
                aHighStack[nStackTop] = pHigh;
                ++nStackTop;
            }

            if (pLow < pHigh1)
            {
                pHigh = pHigh1;
                goto loop_label;
            }
        }
    }  /*   END OF else, -- i.e., if ( size > ET9_CP_CUTOFF_THRSHLD )  */

    if (nStackTop > 0)
    {
        --nStackTop;
        pLow = aLowStack[nStackTop];
        pHigh = aHighStack[nStackTop];
        goto loop_label;
    }
    else
        return;
}

static void ET9LOCALCALL QSort( ET9_CP_SidFreq* pArray, ET9U32 nCount, COMPARE_FCT comp )
{
    if ( nCount > 0 )
        InternalQSort(pArray,  pArray + nCount - 1, comp);
}

/*****************************************************************************************************************/

typedef struct ET9_CP_CJKeyseq_s {
    ET9U8 abKeys[MAX_CANG_JIE_CODE_LENGTH];
    ET9U8 bLen;
} ET9_CP_CJKeyseq;

typedef struct
{
#ifdef ET9_DIRECT_LDB_ACCESS
    const ET9U8 ET9FARDATA *pcCanJie;
#else
    ET9U8 pcCanJie[MAX_CANG_JIE_CODE_LENGTH];
#endif
    ET9U16 wSid;
} ET9_CP_CangJieSid;

static ET9INT ET9LOCALCALL  ET9_CP_FindPartialMatchSid( ET9_CP_CangJieResultArray * pArray, ET9U16 wSid )
{
    ET9U16 nL = pArray->wExactCount;
    ET9U16 nH = pArray->wTotalCount;
    ET9U16 nM;

    while (nL < nH)
    {
        nM = (ET9U16)((nL + nH) / 2);
        if ( wSid == pArray->aSidFreq[nM].wSid )
            return nM;
        else if (wSid < pArray->aSidFreq[nM].wSid)
            nH = nM;
        else
            nL = (ET9U16)(nM + 1);
    }
    return -1;
}

#define ET9_CP_RDB_FREQ_FLAG   0x8000
#define ET9_CP_FREQ_NO_VALUE   0xFFFF
static void ET9LOCALCALL ET9_CP_RUDB_Freq_Stroke_Zones(ET9CPLingInfo * pET9CPLingInfo, ET9_CP_CangJieResultArray * pArray )
{
    ET9CPUdbInfo ET9FARDATA *pUdb = pET9CPLingInfo->pUdb;
    ET9_CP_RUdbObj sObj;
    ET9U8 bZone, bPhraseLen;
    ET9U8 ET9FARDATA * pbCurrent;

    if ( pUdb == NULL )
        return;
    for (bZone = ET9_CP_UDBPHONETICZONES; bZone < ET9_CP_UDBZONEMAX; bZone++)
    {
        ET9U16 wOffset = pUdb->wZoneOffset[bZone];
        ET9U16 wWordCount = pUdb->wZoneWordCount[bZone];
        pbCurrent = ET9_CP_UdbData(pUdb) + wOffset;
        sObj.wEntrySize = 0;
        while (wWordCount)
        {
            pbCurrent = ET9_CP_MoveUdbPR(pUdb, pbCurrent, sObj.wEntrySize);
            ET9Assert( pbCurrent < ET9_CP_UdbEnd(pUdb) );
            ET9_CP_GetEntryInfo(pUdb, pbCurrent, &sObj, ET9_CP_GET_TYPE | ET9_CP_GET_ENTRY_SIZE);

            if (sObj.eType != ET9_CP_FREE)
            {
                wWordCount--;
                ET9Assert (sObj.eType == ET9_CP_RDBSTROKE || sObj.eType == ET9_CP_UDBSTROKE || sObj.eType == ET9_CP_AUDBSTROKE );
                {
                    bPhraseLen = ET9_CP_EntrySizeToWordSize(sObj.wEntrySize);
                    if ( bPhraseLen == 1 )
                    {
                        ET9_CP_GetEntryInfo(pUdb, pbCurrent, &sObj, ET9_CP_GET_FREQ | ET9_CP_GET_ID);
                        if ( !ET9_CP_Is_Comp_Sid(pET9CPLingInfo, sObj.pwID[0]) )  /* skip component */
                        {
                            ET9INT idx = ET9_CP_FindPartialMatchSid( pArray, sObj.pwID[0] );
                            if ( idx >= 0 )
                            {
                                pArray->aSidFreq[idx].wFreq = (ET9U16)(ET9_CP_RDB_FREQ_FLAG | (sObj.wFreq >> 1));
                            }
                        }
                    }
                }
            }
        }
    }
}

static ET9STATUS ET9LOCALCALL ET9_CP_GetCangJieSid( ET9CPLingInfo * pET9CPLingInfo, ET9U16 wCID, ET9_CP_CangJieSid * pCjSid )
{
    ET9U32 dwReadOffset;

    if ( wCID >= pET9CPLingInfo->Private.CPrivate.nCangJieSidCount )
        return ET9STATUS_OUT_OF_RANGE;
    if ( pCjSid == NULL )
        return ET9STATUS_BAD_PARAM;

    dwReadOffset = pET9CPLingInfo->CommonInfo.sOffsets.dwCangJieOffset
                   + pET9CPLingInfo->Private.CPrivate.nCangJieSidOffset
                   + wCID * CANG_JIE_UNICODE_SIZE;

#ifdef ET9_DIRECT_LDB_ACCESS
    ET9_CP_ReadLdbMultiBytes(pET9CPLingInfo, dwReadOffset, MAX_CANG_JIE_CODE_LENGTH, &(pCjSid->pcCanJie) );
#else
    ET9_CP_ReadLdbMultiBytes(pET9CPLingInfo, dwReadOffset, MAX_CANG_JIE_CODE_LENGTH, pCjSid->pcCanJie );
#endif
    dwReadOffset += MAX_CANG_JIE_CODE_LENGTH;

    pCjSid->wSid = ET9_CP_LdbReadWord(pET9CPLingInfo, dwReadOffset);

    return ET9STATUS_NONE;
}

static ET9STATUS ET9LOCALCALL ET9_CP_GetCangJieSidFromUniSortedArrIdx( ET9CPLingInfo * pET9CPLingInfo, ET9U32 nIdx, ET9_CP_CangJieSid * pCjSid )
{
    ET9U32 dwReadOffset;
    ET9STATUS status;
    ET9U16 wIdx;

    if ( nIdx >= pET9CPLingInfo->Private.CPrivate.nCangJieSidCount ) {
        return ET9STATUS_OUT_OF_RANGE;
    }

    dwReadOffset = pET9CPLingInfo->CommonInfo.sOffsets.dwCangJieOffset
                   + pET9CPLingInfo->Private.CPrivate.nUnicodeSortedCIDOffset
                   + nIdx * 2; /* sizeof(ET9U16) */
    wIdx = ET9_CP_LdbReadWord(pET9CPLingInfo, dwReadOffset);
    status = ET9_CP_GetCangJieSid(pET9CPLingInfo, wIdx, pCjSid);
    return status;
}


static ET9STATUS ET9LOCALCALL  ET9_CP_GetUnicodeFromUniSortedArr( ET9CPLingInfo * pET9CPLingInfo, ET9U32 nIdx, ET9U16 * pwUnicode )
{
    ET9STATUS status;
    ET9U16 wPID;
    ET9_CP_CangJieSid  sCjSid;
    ET9Assert( pwUnicode != NULL );
    *pwUnicode = 0;

    status = ET9_CP_GetCangJieSidFromUniSortedArrIdx(pET9CPLingInfo, nIdx, &sCjSid);
    if ( status == ET9STATUS_NONE &&
         ET9_CP_LookupID(pET9CPLingInfo, &wPID, sCjSid.wSid, 1, ET9_CP_Lookup_SIDToPID) > 0)
    {
        *pwUnicode = ET9_CP_LookupUnicode(pET9CPLingInfo, wPID);
    }
    return status;
}

static ET9STATUS ET9LOCALCALL ET9_CP_Uncode2UniSortedArrIdx( ET9CPLingInfo * pET9CPLingInfo, ET9U16 wChar, ET9U16* pwStartIndex, ET9U16* pwCount )
{
    ET9STATUS status;
    ET9U16 nL = 0;
    ET9U16 nH = (ET9U16)pET9CPLingInfo->Private.CPrivate.nCangJieSidCount;
    ET9U16 nM;
    ET9U16 wUnicode, wStartIndex;

    ET9Assert ( pwStartIndex != NULL && pwCount != NULL );

    *pwStartIndex = (ET9U16)-1;
    *pwCount = 0;

    ET9_CP_CHECK_LINGINFO(pET9CPLingInfo);

    while (nL < nH)
    {
        nM = (ET9U16)((nL + nH) / 2);
        status = ET9_CP_GetUnicodeFromUniSortedArr(pET9CPLingInfo, nM, &wUnicode);
        ET9Assert(status == ET9STATUS_NONE);
        if ( wChar == wUnicode )
        {
            wStartIndex = nM;
            while ( wStartIndex > 0 )
            {
                status = ET9_CP_GetUnicodeFromUniSortedArr(pET9CPLingInfo, wStartIndex - 1, &wUnicode);
                ET9Assert(status == ET9STATUS_NONE);
                if ( wChar != wUnicode )
                    break;
                wStartIndex--;
            }
            while ( ++nM < pET9CPLingInfo->Private.CPrivate.nCangJieSidCount )
            {
                status = ET9_CP_GetUnicodeFromUniSortedArr(pET9CPLingInfo, nM, &wUnicode);
                ET9Assert(status == ET9STATUS_NONE);
                if ( wChar != wUnicode )
                    break;
            }
            *pwStartIndex = wStartIndex;
            *pwCount = (ET9U16)(nM - wStartIndex);
            return ET9STATUS_NONE;
        }
        else if (wChar < wUnicode)
            nH = nM;
        else
            nL = (ET9U16)(nM + 1);
    }
    return ET9STATUS_NONE;
}


/*---------------------------------------------------------------------------*/
/** Get CangJie code of a Unicode character.
 *
 * @param pET9CPLingInfo   the ET9CPLingInfo
 * @param sChar            the Unicode character to get the Cang Jie code
 * @param pbBuf            buffer to hold Cang Jie code
 * @param pbLen            IN: hold the size of pbBuf,  OUT: hold the char number of the Cang Jie code
 * @param bAltIndex        0-based index of Cang Jie code (for multiple Cang Jie code case)
 *
 * @return ET9STATUS_NONE   if succeeded, otherwise
 * @return ET9STATUS_NO_INIT          if pET9CPLingInfo is not initialized
 * @return ET9STATUS_BAD_PARAM        if some pointer argument is NULL
 * @return ET9STATUS_INVALID_DB_TYPE  if LDB does not contains Cang Jie code
 * @return ET9STATUS_INVALID_MODE     if is neither ET9CPMODE_CANGJIE nor ET9CPMODE_QUICK_CANGJIE
 * @return ET9STATUS_WORD_NOT_FOUND   if bAltIndex is too big
 * @return ET9STATUS_BUFFER_TOO_SMALL if the buffer pbBuf is too small to hold Cang Jie code
 * @return ET9STATUS_OUT_OF_RANGE     internal error
 */
ET9STATUS ET9FARCALL ET9CPGetCharCangJieCode(ET9CPLingInfo *pET9CPLingInfo, ET9SYMB sChar, ET9U8 *pbBuf, ET9U8 *pbLen, ET9U8 bAltIndex)
{
    ET9STATUS status;
    ET9U16 wStartIndex, wCount;
    ET9_CP_CangJieSid cs;
    ET9U8  bL;

    ET9_CP_CHECK_LINGINFO(pET9CPLingInfo);
    if (!pbBuf || !pbLen) {
        return ET9STATUS_BAD_PARAM;
    }

    if ( pET9CPLingInfo->CommonInfo.sOffsets.dwCangJieOffset == 0 )
        return ET9STATUS_INVALID_DB_TYPE;

    if (pET9CPLingInfo->eMode != ET9CPMODE_CANGJIE && pET9CPLingInfo->eMode != ET9CPMODE_QUICK_CANGJIE ) {
        return ET9STATUS_INVALID_MODE;
    }

    status = ET9_CP_Uncode2UniSortedArrIdx( pET9CPLingInfo, sChar, &wStartIndex, &wCount );
    ET9Assert( status == ET9STATUS_NONE );
    if (bAltIndex >= (ET9U8)wCount) {
        return ET9STATUS_WORD_NOT_FOUND;
    }

    status = ET9_CP_GetCangJieSidFromUniSortedArrIdx(pET9CPLingInfo, (wStartIndex + bAltIndex), &cs);
    if ( status != ET9STATUS_NONE )
        return status;

    bL = 0;
    while ( bL < MAX_CANG_JIE_CODE_LENGTH && cs.pcCanJie[bL] != 0 )
    {
        bL++;
    }
    if ( bL > (*pbLen) )
        return ET9STATUS_BUFFER_TOO_SMALL;

    _ET9ByteCopy(pbBuf, cs.pcCanJie, bL);
    *pbLen = bL;

    return status;
}

static void ET9LOCALCALL ET9_CP_FillFreq(ET9CPLingInfo *pET9CPLingInfo, ET9_CP_CangJieResultArray * pArray)
{
    ET9U16 w;
    if ( pArray->wTotalCount == 0 )
        return;
    if ( pArray->aSidFreq[0].wFreq != ET9_CP_FREQ_NO_VALUE )
        return;

    ET9Assert( pArray->wTotalCount >= pArray->wExactCount );
    ET9Assert( pArray->wTotalCount <= sizeof(pArray->aSidFreq) / sizeof(pArray->aSidFreq[0]) );

    if (ET9CPIsModeCangJie(pET9CPLingInfo)) {
        /* QuickCJ do not sort partial and do not use RUDB order */
        QSort( pArray->aSidFreq + pArray->wExactCount, (ET9U32)(pArray->wTotalCount - pArray->wExactCount), CompareSid );
        /* RUDB Freq */
        ET9_CP_RUDB_Freq_Stroke_Zones(pET9CPLingInfo, pArray);
    }

    /* LDB Freq */
    for ( w = 0; w < pArray->wTotalCount; w++ )
    {
        ET9U16 wPID;
        if ( pArray->aSidFreq[w].wFreq == ET9_CP_FREQ_NO_VALUE &&
             ET9_CP_LookupID(pET9CPLingInfo, &wPID, pArray->aSidFreq[w].wSid, 1, ET9_CP_Lookup_SIDToPID) > 0 )
        {
            pArray->aSidFreq[w].wFreq = (ET9U16)ET9_CP_FreqLookup(pET9CPLingInfo, wPID);
        }
    }
}

ET9STATUS ET9FARCALL ET9_CP_CangJieFillPhraseBuffer(ET9CPLingInfo *pET9CPLingInfo)
{
    ET9_CP_CommonInfo *pCommon;
    ET9_CP_PhraseBuf *pMainPhraseBuf;
    ET9U8 *pbContextLen, bPrefixLen, b;
    ET9U16 *pwPrefix;

    pCommon = &(pET9CPLingInfo->CommonInfo);
    pMainPhraseBuf = ET9_CP_GetMainPhraseBuf(pET9CPLingInfo);

    ET9Assert(pMainPhraseBuf->wLastTotal == pMainPhraseBuf->wTotal);

    if (!ET9_CP_HasActiveSpell(pET9CPLingInfo))
    {
        pbContextLen = pCommon->pbContextLen;
        pwPrefix = pCommon->pwContextBuf;
        pCommon->bSylCount = 0;
        bPrefixLen = 0;
        for (b = 0; pbContextLen[b]; b++)
        {
            bPrefixLen = (ET9U8)(bPrefixLen + pbContextLen[b]);
        }
        for (b = 0; pbContextLen[b]; b++)
        {
            ET9Assert(bPrefixLen); /* search for context suffix, no partial syl but need partial phrase */
            ET9_CP_GetUdbPhrases(pET9CPLingInfo, 0, 1, pwPrefix, bPrefixLen, NULL, /*isSid*/1, 0, 0, 0, 0, pMainPhraseBuf);
            ET9_CP_GetLdbPhrases(pET9CPLingInfo, /*isSid*/1, 0, 1, NULL, 0, 0, 0, pwPrefix, bPrefixLen, 0, 0, pMainPhraseBuf);
            bPrefixLen = (ET9U8)(bPrefixLen - pbContextLen[b]);
            pwPrefix += pbContextLen[b];
        }
        ET9_CP_GetSmartPuncts(pET9CPLingInfo);

        if (ET9_CP_IsPhraseBufEmpty(pMainPhraseBuf) || /* the current page is not full */
            !ET9_CP_PhraseBufPageFillDone(pMainPhraseBuf))
        {
            /* build common charcters */
            if (ET9CPIsNameInputActive(pET9CPLingInfo)) {
                ET9_CP_GetCommonNameChar(pET9CPLingInfo, 1);
            }
            else {
                ET9_CP_GetCommonChar(pET9CPLingInfo);
            }
        }
    }
    else
    { /* spell buffer is not empty */
        ET9U8 bIsExact, bIsFromContext, bIsFromUdb;
        ET9U16 i, wFreq;
        ET9_CP_CangJieResultArray * pArray;
        ET9_CP_PhraseBuf *pMainPhraseBuf = ET9_CP_GetMainPhraseBuf(pET9CPLingInfo);

        ET9Assert(ET9_CP_HasActiveSpell(pET9CPLingInfo)); /* should have been set by buildspellings */
        ET9Assert(ET9CPIsModeCangJie(pET9CPLingInfo) || ET9CPIsModeQuickCangJie(pET9CPLingInfo));
        pArray = &pET9CPLingInfo->Private.CPrivate.sCangJieResultArray;
        ET9_CP_FillFreq( pET9CPLingInfo, pArray );

        bIsFromContext = 0;
        bIsFromUdb = 0;
        bIsExact = 1;
        for ( i = 0; i < pArray->wExactCount; i++ )
        {
            wFreq = pArray->aSidFreq[i].wFreq;
            wFreq = ET9_CP_EncodeFreq(pET9CPLingInfo, NULL, 1, wFreq, bIsExact, bIsFromContext, bIsFromUdb, NULL);
            ET9_CP_AddPhraseToBuf(pET9CPLingInfo, pMainPhraseBuf, (ET9SYMB *)&pArray->aSidFreq[i].wSid, 1, NULL, 0,
                                  ET9_CP_IDEncode_SID, wFreq);
        }
        bIsExact = 0;
        for ( ; i < pArray->wTotalCount; i++ )
        {
            wFreq = pArray->aSidFreq[i].wFreq;
            if ( wFreq & ET9_CP_RDB_FREQ_FLAG && ET9CPIsModeCangJie(pET9CPLingInfo) )  /* QuickCJ does not use RUDB order */
            {
                bIsFromUdb = 1;
                wFreq = (ET9U16)(wFreq << 1);
            }
            else {
                bIsFromUdb = 0;
            }
            wFreq = ET9_CP_EncodeFreq(pET9CPLingInfo, NULL, 1, wFreq, bIsExact, bIsFromContext, bIsFromUdb, NULL);
            ET9_CP_AddPhraseToBuf(pET9CPLingInfo, pMainPhraseBuf, (ET9SYMB *)&pArray->aSidFreq[i].wSid, 1, NULL, 0,
                                  ET9_CP_IDEncode_SID, wFreq);
        }
    } /* end of if (0 == pCommon->bKeyBufLen) else */
    pMainPhraseBuf->wLastTotal = pMainPhraseBuf->wTotal;
    return ET9STATUS_NONE;
}  /*  ET9_CP_CangJieFillPhraseBuffer */

ET9STATUS ET9FARCALL ET9_CP_CangJieSelectPhrase(ET9CPLingInfo *    pET9CPLingInfo,
                                                ET9U16 wPhraseIndex)
{
    ET9STATUS        mStatus = ET9STATUS_NONE;
    ET9CPPhrase  phraseUnicode, phraseEncoded;
    ET9_CP_PhraseBuf *pMainPhraseBuf;

    pMainPhraseBuf = ET9_CP_GetMainPhraseBuf(pET9CPLingInfo);

    ET9_CP_GetPhraseFromBuf(pMainPhraseBuf, &phraseEncoded, NULL, wPhraseIndex + 1);

    _ET9SymCopy(phraseUnicode.pSymbs, (ET9SYMB *)phraseEncoded.pSymbs, phraseEncoded.bLen);
    phraseUnicode.bLen = phraseEncoded.bLen;
    ET9_CP_ConvertPhraseToUnicode(pET9CPLingInfo, &phraseUnicode, ET9_CP_IDEncode_SID);

    if (ET9_CP_PhraseIsAllChn(pET9CPLingInfo, phraseEncoded.pSymbs, phraseEncoded.bLen))
        mStatus = ET9_CP_SelectionHistAdd(&pET9CPLingInfo->SelHistory, phraseUnicode.pSymbs, phraseEncoded.pSymbs, phraseEncoded.bLen, pET9CPLingInfo->Base.pWordSymbInfo->bNumSymbs);
    else /*  NULL in the phraseEncoded position means do not add to ContextBuf */
        mStatus = ET9_CP_SelectionHistAdd(&pET9CPLingInfo->SelHistory, phraseUnicode.pSymbs, NULL, phraseEncoded.bLen, pET9CPLingInfo->Base.pWordSymbInfo->bNumSymbs);

    return mStatus;
}   /* end of ET9_CP_CangJieSelectPhrase() */


static ET9STATUS ET9LOCALCALL ArrayAddSidValue( ET9_CP_CangJieResultArray * pArray, ET9U16 w, ET9BOOL fExact )
{
    if ( pArray->wTotalCount >= sizeof(pArray->aSidFreq) / sizeof(pArray->aSidFreq[0]) )
        return ET9STATUS_FULL;
    if ( fExact )
    {
        ET9U16 i;
        for ( i = pArray->wTotalCount; i > pArray->wExactCount; i-- )
        {
            pArray->aSidFreq[i] = pArray->aSidFreq[i - 1];
        }
        pArray->aSidFreq[pArray->wExactCount].wSid = w;
        pArray->aSidFreq[pArray->wExactCount].wFreq = ET9_CP_FREQ_NO_VALUE;
        pArray->wExactCount++;
        pArray->wTotalCount++;
    }
    else
    {
        pArray->aSidFreq[pArray->wTotalCount].wSid = w;
        pArray->aSidFreq[pArray->wTotalCount].wFreq = ET9_CP_FREQ_NO_VALUE;
        pArray->wTotalCount++;
    }
    return ET9STATUS_NONE;
}

static void ET9LOCALCALL InitCangJieResultArray( ET9_CP_CangJieResultArray * pArray )
{
    pArray->wTotalCount = 0;
    pArray->wExactCount = 0;
}

static ET9U8 ET9LOCALCALL LastLetter( const ET9_CP_CangJieSid * pCangJieSid )
{
    ET9U8 i, c = 0;
    for ( i = 1; i < MAX_CANG_JIE_CODE_LENGTH; i++ )
    {
        if ( pCangJieSid->pcCanJie[i] == 0 )
            break;
        else
            c = pCangJieSid->pcCanJie[i];
    }
    return c;
}

static ET9STATUS ET9LOCALCALL ET9_CP_QuickCJ( ET9CPLingInfo * pET9CPLingInfo, ET9U16 wStartCID, ET9U16 wCount, ET9_CP_CJKeyseq*  pCangJieKeyseq )
{
    ET9STATUS status;
    ET9U8 c;
    ET9BOOL fExact;
    ET9_CP_CangJieSid cs;
    ET9U16 i;

    InitCangJieResultArray( &pET9CPLingInfo->Private.CPrivate.sCangJieResultArray );
    ET9Assert(pCangJieKeyseq->bLen != 0);
    if ( pCangJieKeyseq->bLen > 2 )
        return ET9STATUS_INVALID_INPUT;
    if ( pCangJieKeyseq->bLen == 2 )
    {
        c = pCangJieKeyseq->abKeys[1];
        ET9Assert( ET9_CP_IsPinyinUpperCase(c) );

        for ( i = 0; i < wCount; i++ )
        {
            status = ET9_CP_GetCangJieSid(pET9CPLingInfo, (ET9U16)(wStartCID + i), &cs);
            ET9Assert(status == ET9STATUS_NONE);
            if ( LastLetter(&cs) == c )
            {
                fExact = (ET9BOOL)( (cs.pcCanJie[2] == 0) ? 1 : 0); /* QuickCJ: add exact matches to the front */
                status = ArrayAddSidValue( &pET9CPLingInfo->Private.CPrivate.sCangJieResultArray, cs.wSid, fExact );
                ET9Assert(status == ET9STATUS_NONE);
            }
        }
    }
    else /* if ( pCangJieKeyseq->bLen == 1 )  */
    {
        c = pCangJieKeyseq->abKeys[0];
        ET9Assert( ET9_CP_IsPinyinUpperCase(c) );
        for ( i = 0; i < wCount; i++ )
        {
            status = ET9_CP_GetCangJieSid(pET9CPLingInfo, (ET9U16)(wStartCID + i), &cs);
            ET9Assert(status == ET9STATUS_NONE);
            if ( c == cs.pcCanJie[0] && cs.pcCanJie[1] == 0 )
            {
                fExact = 1;
                status = ArrayAddSidValue( &pET9CPLingInfo->Private.CPrivate.sCangJieResultArray, cs.wSid, fExact );
                ET9Assert(status == ET9STATUS_NONE);
            }
            else
            {
                fExact = 0;
                status = ArrayAddSidValue( &pET9CPLingInfo->Private.CPrivate.sCangJieResultArray, cs.wSid, fExact );
                ET9Assert(status == ET9STATUS_NONE);
            }
        }
    }
    return ET9STATUS_NONE;
}

typedef struct
{
    ET9U16 nStartIndex;
    ET9U16 nNumExact;
    ET9U16 nNumPartial;  /* Exact is a special case of partial --- m_nNumPartial >= m_nNumExact */
} ET9_CP_FindResult;


static ET9STATUS ET9LOCALCALL CangJie2SidBSrch( ET9CPLingInfo * pET9CPLingInfo, ET9U16 wStartCID, ET9U16 wCount, ET9_CP_CJKeyseq* pCangJieKeyseq )
{
    ET9STATUS status;
    ET9U16 nEndCID = (ET9U16)(wStartCID + wCount);
    ET9U16 nL = (ET9U16)wStartCID;
    ET9U16 nH = nEndCID;
    ET9U16 nM;
    ET9INT iResult;
    ET9_CP_CangJieSid cs;
    ET9U16 wCIDStart = wStartCID;
    ET9U16 wExactEnd = wStartCID;
    ET9U16 wPartialEnd = wStartCID;  /* Exact is a special case of partial --- wNumPartial >= wNumExact */

    InitCangJieResultArray( &pET9CPLingInfo->Private.CPrivate.sCangJieResultArray );
    ET9Assert( pCangJieKeyseq->bLen > 0 && pCangJieKeyseq->bLen <= MAX_CANG_JIE_CODE_LENGTH);

    while (nL < nH)
    {
        nM = (ET9U16)((nL + nH) / 2);
        status = ET9_CP_GetCangJieSid(pET9CPLingInfo, nM, &cs);
        ET9Assert(status == ET9STATUS_NONE);
        iResult = ET9_CP_MemCmp(pCangJieKeyseq->abKeys, cs.pcCanJie, MAX_CANG_JIE_CODE_LENGTH );
        if ( iResult == 0 )
        {
            wCIDStart = nM;
            while ( wCIDStart > 0 )
            {
                status = ET9_CP_GetCangJieSid(pET9CPLingInfo, (ET9U16)(wCIDStart - 1), &cs);
                ET9Assert(status == ET9STATUS_NONE);
                if ( ET9_CP_MemCmp( pCangJieKeyseq->abKeys, cs.pcCanJie, MAX_CANG_JIE_CODE_LENGTH ) != 0 )
                    break;
                wCIDStart--;
            }
            while ( ++nM < nEndCID )
            {
                status = ET9_CP_GetCangJieSid(pET9CPLingInfo, nM, &cs);
                ET9Assert(status == ET9STATUS_NONE);
                if ( ET9_CP_MemCmp( pCangJieKeyseq->abKeys, cs.pcCanJie, MAX_CANG_JIE_CODE_LENGTH ) != 0 )
                    break;
            }
            wPartialEnd = wExactEnd = nM;
#if !SHOW_PARTIAL_FOR_ONE_KEY
            if ( pCangJieKeyseq->bLen > 1 )
#endif
            {
                while ( nM < nEndCID )
                {
                    status = ET9_CP_GetCangJieSid(pET9CPLingInfo, nM, &cs);
                    ET9Assert(status == ET9STATUS_NONE);
                    if ( ET9_CP_MemCmp( pCangJieKeyseq->abKeys, cs.pcCanJie, pCangJieKeyseq->bLen ) != 0 )
                        break;
                    nM++;
                }
                wPartialEnd = nM;
            }
            goto convert;
        }
        else if (iResult < 0)
            nH = nM;
        else
            nL = (ET9U16)(nM + 1);
    }

    /* in case there is no exact match */
    ET9Assert( nL == nH );
#if !SHOW_PARTIAL_FOR_ONE_KEY
    if ( pCangJieKeyseq->bLen > 1 )
#endif
    {
        if ( nH < nEndCID
            && ET9STATUS_NONE == ET9_CP_GetCangJieSid(pET9CPLingInfo, nH, &cs)
            && 0 == ET9_CP_MemCmp( pCangJieKeyseq->abKeys, cs.pcCanJie, pCangJieKeyseq->bLen ) )
        {
            wCIDStart = nH;
            wExactEnd = wCIDStart;
            nM = nH;
            while ( ++nM < nEndCID )
            {
                status = ET9_CP_GetCangJieSid(pET9CPLingInfo, nM, &cs);
                ET9Assert(status == ET9STATUS_NONE);
                if ( ET9_CP_MemCmp( pCangJieKeyseq->abKeys, cs.pcCanJie, pCangJieKeyseq->bLen ) != 0 )
                    break;
            }
            wPartialEnd = nM;
        }
    }

convert:
    {
        ET9U8  bFreq, bAltCount;
        ET9U16 wPID, wCID;
        for ( wCID = wCIDStart; wCID < wExactEnd; wCID++ )
        {
            status = ET9_CP_GetCangJieSid(pET9CPLingInfo, wCID, &cs);
            ET9Assert(status == ET9STATUS_NONE);
            bAltCount = ET9_CP_LookupID(pET9CPLingInfo, &wPID, cs.wSid, 1, ET9_CP_Lookup_SIDToPID);
            ET9Assert(bAltCount > 0);
            bFreq = ET9_CP_FreqLookup(pET9CPLingInfo, wPID);
            status = ArrayAddSidValue( &pET9CPLingInfo->Private.CPrivate.sCangJieResultArray, cs.wSid, /*fExact*/1 );
            ET9Assert(status == ET9STATUS_NONE);
        }
        for ( ; wCID < wPartialEnd; wCID++ )
        {
            status = ET9_CP_GetCangJieSid(pET9CPLingInfo, wCID, &cs);
            ET9Assert(status == ET9STATUS_NONE);
            bAltCount = ET9_CP_LookupID(pET9CPLingInfo, &wPID, cs.wSid, 1, ET9_CP_Lookup_SIDToPID);
            ET9Assert(bAltCount > 0);
            bFreq = ET9_CP_FreqLookup(pET9CPLingInfo, wPID);
            status = ArrayAddSidValue( &pET9CPLingInfo->Private.CPrivate.sCangJieResultArray, cs.wSid, /*fExact*/0 );
            ET9Assert(status == ET9STATUS_NONE);
        }
    }
    return ET9STATUS_NONE;
}

/* read a WSI into the phonetic module to prepare for spelling search. */
/* doesn't modify anything unless STATNONE */
static ET9STATUS ET9LOCALCALL CangJieWSIToKeyseq( ET9WordSymbInfo * pWordSymbInfo,
                                                  ET9_CP_CJKeyseq * psKeyseq )
{
    ET9U8 bSymbIndex, bWSILen = pWordSymbInfo->bNumSymbs;

    ET9Assert(psKeyseq);
    if ( bWSILen > MAX_CANG_JIE_CODE_LENGTH )
        return ET9STATUS_INVALID_INPUT;

    psKeyseq->bLen = 0;

    for (bSymbIndex = 0; bSymbIndex < bWSILen; bSymbIndex++)
    {

        ET9U8 bSymbCountOnKey = pWordSymbInfo->SymbsInfo[bSymbIndex].DataPerBaseSym[0].bNumSymsToMatch;
        ET9U8 b;

        /* search the SymbInfo for a phonetic letter */
        for (b = 0; b < bSymbCountOnKey; b++)
        {
            ET9SYMB symb = pWordSymbInfo->SymbsInfo[bSymbIndex].DataPerBaseSym[0].sUpperCaseChar[b];
            if (ET9CPIsCangJieUpperSymbol(symb))
            {
                psKeyseq->abKeys[bSymbIndex] = (ET9U8)symb;
                break;  /* for (b = 0; b < bSymbCountOnKey; b++)  */
            }
        } /* end for bSymbIndex on SymbInfo */
        if (b == bSymbCountOnKey) { /* no symb on this key can be a phonetic key */
            return ET9STATUS_INVALID_INPUT;
        }
    }

    psKeyseq->bLen = bSymbIndex;
    for ( ; bSymbIndex < MAX_CANG_JIE_CODE_LENGTH; bSymbIndex++)
    {
        psKeyseq->abKeys[bSymbIndex] = (ET9U8)0;
    }

    return ET9STATUS_NONE;
}

static ET9U16 ET9LOCALCALL GetCangJie2UniArray(ET9CPLingInfo * pET9CPLingInfo, ET9U8 c, ET9U16 * pwCount)
{
    ET9INT iIdx = c - 'A';
    ET9U16 * pwOffs;

    ET9Assert( pET9CPLingInfo );
    ET9Assert( ET9CPIsCangJieUpperSymbol(c) );
    pwOffs = pET9CPLingInfo->Private.CPrivate.awSubTreeOffsets;
    *pwCount = (ET9U16)(pwOffs[iIdx + 1] - pwOffs[iIdx]);
    return pwOffs[iIdx];
}

ET9STATUS ET9FARCALL ET9_CP_QuickCangJieBuildSpellings(ET9CPLingInfo * pET9CPLingInfo)
{
    ET9U16  wStartIndex,  wCount;
    ET9_CP_CJKeyseq  sCangJieKeyseq;
    ET9STATUS status = ET9STATUS_INVALID_INPUT;

    if (pET9CPLingInfo->CommonInfo.sOffsets.dwCangJieOffset == 0)
        return ET9STATUS_INVALID_MODE;

    ET9Assert(ET9CPIsModeQuickCangJie(pET9CPLingInfo));
    status = CangJieWSIToKeyseq(pET9CPLingInfo->Base.pWordSymbInfo, &sCangJieKeyseq);
    if (ET9STATUS_NONE != status) {
        return status;
    }

    wStartIndex = GetCangJie2UniArray( pET9CPLingInfo, sCangJieKeyseq.abKeys[0], &wCount );

    ET9_CP_QuickCJ(pET9CPLingInfo, wStartIndex, wCount, &sCangJieKeyseq );
    if ( pET9CPLingInfo->Private.CPrivate.sCangJieResultArray.wTotalCount )
    {
        /* Add Spell  */
        _ET9ByteCopy(pET9CPLingInfo->CommonInfo.sActiveSpell.pbChars, sCangJieKeyseq.abKeys, sCangJieKeyseq.bLen );
        pET9CPLingInfo->CommonInfo.sActiveSpell.bLen = sCangJieKeyseq.bLen;

        ET9_CP_ClearPhraseBuf(ET9_CP_GetMainPhraseBuf(pET9CPLingInfo));
        status = ET9STATUS_NONE;
    }
    else {
        status = ET9STATUS_INVALID_INPUT;
    }
    return status;
}

ET9STATUS ET9FARCALL ET9_CP_CangJieBuildSpellings(ET9CPLingInfo * pET9CPLingInfo)
{
    ET9U16  wStartIndex,  wCount;
    ET9_CP_CJKeyseq  sCangJieKeyseq;
    ET9STATUS status = ET9STATUS_INVALID_INPUT;

    if (pET9CPLingInfo->CommonInfo.sOffsets.dwCangJieOffset == 0)
        return ET9STATUS_INVALID_MODE;

    ET9_CP_SelectionHistInit(&pET9CPLingInfo->SelHistory);

    ET9Assert(ET9CPIsModeCangJie(pET9CPLingInfo));
    status = CangJieWSIToKeyseq(pET9CPLingInfo->Base.pWordSymbInfo, &sCangJieKeyseq);
    if (ET9STATUS_NONE != status) {
        return status;
    }

    wStartIndex = GetCangJie2UniArray( pET9CPLingInfo, sCangJieKeyseq.abKeys[0], &wCount );

    CangJie2SidBSrch(pET9CPLingInfo, wStartIndex, wCount, &sCangJieKeyseq );
    if ( pET9CPLingInfo->Private.CPrivate.sCangJieResultArray.wTotalCount )
    {
        /* Add Spell  */
        _ET9ByteCopy(pET9CPLingInfo->CommonInfo.sActiveSpell.pbChars, sCangJieKeyseq.abKeys, sCangJieKeyseq.bLen );
        pET9CPLingInfo->CommonInfo.sActiveSpell.bLen = sCangJieKeyseq.bLen;

        ET9_CP_ClearPhraseBuf(ET9_CP_GetMainPhraseBuf(pET9CPLingInfo));
        status = ET9STATUS_NONE;
    }
    else {
        status = ET9STATUS_INVALID_INPUT;
    }

    return status;
}

#endif   /* #ifndef ET9CP_DISABLE_STROKE */

/* ----------------------------------< eof >--------------------------------- */
