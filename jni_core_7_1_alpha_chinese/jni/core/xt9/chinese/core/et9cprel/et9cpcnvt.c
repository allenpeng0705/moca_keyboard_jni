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
;**     FileName: et9cpcnvt.c                                                 **
;**                                                                           **
;**  Description: Chinese XT9 simplified-traditional Chinese conversion.      **
;**               Conforming to the development version of Chinese XT9.       **
;**                                                                           **
;*******************************************************************************
;******************************************************************************/

#include "et9cpapi.h"
#include "et9cpsys.h"
#include "et9cpldb.h"

/* Function : UnicodePairBSearch
 * Input    : pET9CPLingInfo = Chinese XT9 LingInfo
 *            dwStartOffset:  table starting offset
 *            wPairCount:     count of the unicode pairs
 *            wUnicode:       Unicode to be found
 * Output   : none
 * Return   : if a pair is found whose first unicode is wUnicode, then return the second unicode of it. Otherwise return wUnicode
 */
static ET9U16 ET9LOCALCALL  UnicodePairBSearch(ET9CPLingInfo * pET9CPLingInfo, ET9U32 dwStartOffset, ET9U16 wPairCount, ET9U16 wUnicode)
{
    ET9INT low, mid, high;
    ET9U16 w, wRet = wUnicode;

    low = 0;
    high = wPairCount - 1;

    while ( low <= high )
    {
        mid = (low + high) / 2;
        w = ET9_CP_LdbReadWord(pET9CPLingInfo, dwStartOffset + mid * 4); /* 2 * sizeof(ET9U16) */
        if (w < wUnicode) {
            low = mid + 1;
        }
        else if (w > wUnicode) {
            high = mid - 1;
        }
        else {
            wRet = ET9_CP_LdbReadWord(pET9CPLingInfo, dwStartOffset + mid * 4 + 2);
            break;  /* Find it */
        }
    }

    return wRet;
}

/** Convert a Unicode string of Simplified Chinese to Traditional Chinese
 *
 *  This function is useful when developing UI to display traditional Chinese strings in text area.
 *
 * @param pET9CPLingInfo     pointer to chinese information structure.
 * @param pUnicodeStr        (in/out) The Unicode string of Simplified Chinese to be converted, On exit, it is the converted Unicode string of Traditional Chinese
 * @param wStrLen            The number of Unicode characters in pUnicodeStr
 *
 * @return ET9STATUS_NONE               Succeeded
 * @return ET9STATUS_NO_INIT            pET9CPLingInfo is not properly initialized
 * @return ET9STATUS_BAD_PARAM          pUnicodeStr is NULL
 */
ET9STATUS ET9FARCALL ET9CPSimplifiedToTraditional(ET9CPLingInfo *pET9CPLingInfo,
                                                  ET9SYMB * pUnicodeStr,
                                                  ET9U16    wStrLen)
{
    ET9U16 i;
    ET9_CP_ConversionTable * pConvTable;

    ET9_CP_CHECK_LINGINFO(pET9CPLingInfo);
    if (pUnicodeStr == NULL)
        return ET9STATUS_BAD_PARAM;

    pConvTable = &pET9CPLingInfo->ConvTable;
    for (i = 0; i < wStrLen; i++)
    {
        pUnicodeStr[i] = UnicodePairBSearch(pET9CPLingInfo, pConvTable->dwSimpTradOffset, pConvTable->wSimpTradCount, pUnicodeStr[i]);
    }
    return ET9STATUS_NONE;
}

/** Convert a Unicode string of Traditional Chinese to Simplified Chinese
 *
 *  This function is useful when developing UI to display Simplified Chinese strings in text area.
 *
 * @param pET9CPLingInfo     pointer to chinese information structure.
 * @param pUnicodeStr        (in/out) The Unicode string of Traditional Chinese to be converted, On exit, it is the converted Unicode string of Simplified Chinese
 * @param wStrLen            The number of Unicode characters in pUnicodeStr
 *
 * @return ET9STATUS_NONE               Succeeded
 * @return ET9STATUS_NO_INIT            pET9CPLingInfo is not properly initialized
 * @return ET9STATUS_BAD_PARAM          pUnicodeStr is NULL
 */
ET9STATUS ET9FARCALL ET9CPTraditionalToSimplified(ET9CPLingInfo *pET9CPLingInfo,
                                                  ET9SYMB * pUnicodeStr,
                                                  ET9U16    wStrLen)
{
    ET9U16 i;
    ET9_CP_ConversionTable * pConvTable;

    ET9_CP_CHECK_LINGINFO(pET9CPLingInfo);
    if (pUnicodeStr == NULL)
        return ET9STATUS_BAD_PARAM;

    pConvTable = &pET9CPLingInfo->ConvTable;
    for (i = 0; i < wStrLen; i++)
    {
        pUnicodeStr[i] = UnicodePairBSearch(pET9CPLingInfo, pConvTable->dwTradSimpOffset, pConvTable->wTradSimpCount, pUnicodeStr[i]);
    }
    return ET9STATUS_NONE;
}
