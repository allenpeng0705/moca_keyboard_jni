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
;**     FileName: et9cpbpmf.c                                                 **
;**                                                                           **
;**  Description: Chinese Phrase Text Input ldb database BPMF Chinese XT9     **
;**               module.Conforming to the development version of Chinese XT9.**
;**                                                                           **
;*******************************************************************************
;******************************************************************************/

#include "et9cpapi.h"
#include "et9cpsys.h"
#include "et9cpldb.h"
#include "et9cpmisc.h"
#include "et9cpspel.h"
#include "et9cpkey.h"

/*------------------------------------------------------------------------
 *
 *  Function    : ET9_CP_BpmfSyllableToBIN
 *
 *  Synopsis    : It converts the BPMF syllable to the encoded BIN.
 *                (exclude the syllable index part)
 *
 *     Input    : pbSyl      = the syllable string
 *                bSylSize   = syllable size
 *
 *    output    : pdwBIN     = Encoded BIN
 *
 *    Return    : bit mask for the BIN
 *
 *-----------------------------------------------------------------------*/
ET9U32 ET9FARCALL ET9_CP_BpmfSyllableToBIN(ET9U8         *pbSyl,
                                           ET9U8          bSylSize,
                                           ET9U32        *pdwBIN)
{
    ET9U8    bLetter, i;
    ET9U32   dwMask = 0;

#ifdef ET9_DEBUG
    /* validate inputs */
    ET9Assert(pbSyl);
    ET9Assert(pdwBIN);
    ET9Assert(0 < bSylSize && bSylSize <= ET9_CP_MAX_BPMF_SYL_SIZE);
    ET9Assert(ET9_CP_IsBpmfLetter(pbSyl[0])); /* 1st letter can be upper or lower case */
    for (i = 1; i < bSylSize; i++) {
        ET9Assert(ET9_CP_IsBpmfLowerCase(pbSyl[i]));
    }
#endif
    *pdwBIN = 0;
    for (i = 0; i < ET9_CP_MAX_BPMF_SYL_SIZE; i++) {
        if (i == 1) {
            *pdwBIN <<= 3;
            dwMask <<= 3;
        }
        else {
            *pdwBIN <<= 6;
            dwMask <<= 6;
        }
        if (i < bSylSize) {
            bLetter = (ET9U8)(pbSyl[i] | ET9_CP_BPMFUPPERCASEBIT); /* change to upper case */
            if (i == 1) {
                dwMask |= 0x07;
                if (0xE2 <= bLetter && bLetter <= 0xE4) { /* 2nd letter is i, u , v */
                    *pdwBIN |= (1 << (bLetter - 0xE2));
                }
                else { /* 2nd letter is not i, u, v, place it at the 3rd letter's place and done */
                    *pdwBIN <<= 6;
                    dwMask <<= 6;
                    *pdwBIN |= (bLetter - 0xC0 + 1);
                    dwMask |= 0x3F;
                    break;
                }
            }
            else {
                *pdwBIN |= (bLetter - 0xC0 + 1);
                dwMask |= 0x3F;
            }
        }
    }

    /* pad zero for reserved bits */
    *pdwBIN = *pdwBIN << 1;
    dwMask = dwMask << 1;

    return dwMask;
} /* end of ET9_CP_BpmfSyllableToBIN() */


/*------------------------------------------------------------------------
 *
 *  Function    : ET9_CP_BINToBpmfSyllable
 *
 *  Synopsis    : It converts the encoded BIN to BPMF syllable.
 *
 *     input    : dwBIN         = Encoded BIN
 *
 *    Output    : pbSyl         = the syllable string
 *
 *    Return    : syllable size
 *
 *-----------------------------------------------------------------------*/
ET9U8 ET9FARCALL ET9_CP_BINToBpmfSyllable(ET9U32         dwBIN,
                                          ET9U8         *pbSyl)
{
    ET9UINT nLetterIndex;
    ET9U8 bSylSize;

    bSylSize = 0;
    /* 1st letter */
    ET9Assert(dwBIN & 0xFC00); /* must have the 1st letter */
    nLetterIndex = (dwBIN >> 10) & 0x3F;
    pbSyl[bSylSize++] = (ET9U8)(ET9_CP_BPMFFIRSTUPPERLETTER + nLetterIndex - 1);

    if (dwBIN & 0x0380) {
        /* 2nd letter is i, u, v */
        nLetterIndex = (dwBIN >> 7) & 0x07;
        pbSyl[bSylSize++] = (ET9U8)(ET9_CP_BPMFLASTLOWERLETTER - 2 + (nLetterIndex >> 1));
    }
    if (dwBIN & 0x007E) {
        /* 2nd letter is not i, u, v.  OR has 3rd letter */
        nLetterIndex = (dwBIN >> 1) & 0x3F;
        pbSyl[bSylSize++] = (ET9U8)(ET9_CP_BPMFFIRSTLOWERLETTER + nLetterIndex - 1);
    }

    return bSylSize;
} /* end of ET9_CP_BINToBpmfSyllable() */


/*------------------------------------------------------------------------
 *
 *  Function    : ET9_CP_CompressSparseList
 *
 *  Synopsis    : Sort and compress a list of sparse data.
 *
 *     Input    : pwList    = list of sparse data, ends with 0xFFFF
 *
 *    Output    : pwList    = start of each consecutive data range,
 *                            in ascending order, ends with 0xFFFF
 *                pbRange = size of each data range, ends with 0xFF
 *
 *    Return    : none
 *
 *-----------------------------------------------------------------------*/
static void ET9LOCALCALL ET9_CP_CompressSparseList(ET9U16 *pwList,
                                               ET9U8 *pbRange)
{
    ET9UINT i, j;
    ET9U16 wTmpData;

    /* sort the list in ascending order */
    for(i = 0; pwList[i] != 0xFFFF; i++) {
        for (j = i + 1; pwList[j] != 0xFFFF; j++) {
            /* swap the content if minimum found */
            if (pwList[i] > pwList[j]) {
                wTmpData = pwList[i];
                pwList[i] = pwList[j];
                pwList[j] = wTmpData;
            }
        }
        pbRange[i] = 1; /* each range is one before compression */
    }
    /* compress consecutive data into ranges */
    for(i = 0, j = 0, wTmpData = pwList[0]; pwList[i] != 0xFFFF; i++) {
        if (wTmpData == pwList[i]) {
            continue; /* discard identical data */
        }
        else if (pwList[i] == (wTmpData + 1)) {
            wTmpData++; /* compress consecutive data */
            pbRange[j]++;
        }
        else { /* non-consecutive data, start a new range */
            wTmpData = pwList[i];
            j++;
            pwList[j] = pwList[i];
        }
    }
    /* end the list with 0xFFFF and the range with 0xFF */
    pwList[++j] = 0xFFFF;
    pbRange[j] = 0xFF;

} /* END ET9_CP_CompressSparseList() */

/*------------------------------------------------------------------------
 *
 *  Function    : ET9_CP_GetSyllableIndex
 *
 *  Synopsis    : It gets the BPMF syllable indices for the specified syllable.
 *
 *     Input    : pET9CPLingInfo = Pointer to Chinese XT9 LingInfo structure
 *                pbSpell        = Character's spelling
 *                bSpellLen      = Character's spelling length
 *                bPartial       = 0 - find exact match only
 *                                 1 - find exact and partial match
 *
 *    Output    : pwExactSylIndex    = exact match syllable indices buffer, ends with 0xFFFF
 *                pbExactSylIndexRange = ranges of exact match syllable indices, ends with 0xFF
 *                pwPartSylIndex     = partial match syllable indices buffer, ends with 0xFFFF
 *                pbPartSylIndexRange  = ranges of partial match syllable indices, ends with 0xFF
 *
 *    Return    : 1 - success; 0 - the specified syllable is not found
 *
 *-----------------------------------------------------------------------*/
ET9BOOL ET9FARCALL ET9_CP_GetSyllableIndex(ET9CPLingInfo *pET9CPLingInfo,
                                           ET9U8         *pbSpell,
                                           ET9U8          bSpellLen,
                                           ET9BOOL        bPartial,
                                           ET9U16        *pwExactSylIndex,
                                           ET9U8         *pbExactSylIndexRange,
                                           ET9U16        *pwPartSylIndex,
                                           ET9U8         *pbPartSylIndexRange)
{
    ET9U32   dwCharBIN, dwMask, dwExactBIN;
    ET9U16   i, wSylIndex, wStartIndex = 0, wEndIndex = 0;
    ET9U16   *pwSavedPartSylIndex = pwPartSylIndex; /* save the pointer */

    ET9Assert(pwExactSylIndex && pbExactSylIndexRange);
    ET9Assert(!bPartial || (pwPartSylIndex && pbPartSylIndexRange));
    /* encode the given syllable to a BIN */
    dwMask = ET9_CP_PinyinSyllableToBIN(pbSpell, bSpellLen, &dwCharBIN);

    if (ET9_CP_SearchSylbFromTable(pET9CPLingInfo, 0, bPartial, dwCharBIN, dwMask, &wStartIndex, &wEndIndex)) {
        return 0;
    }

    ET9Assert(ET9_CP_IsLdbDualPhonetic(pET9CPLingInfo));
    dwExactBIN = pET9CPLingInfo->Private.PPrivate.adwPinyinBinTable[wStartIndex];
    wSylIndex = (ET9U16)(dwExactBIN & 0x01FF); /* extract the syl index from BIN */
    dwExactBIN &= 0xFFFFFE00; /* remove the syl index part */

    /* check for exact match */
    dwMask <<= 8;
    if ((dwExactBIN & dwMask) == dwExactBIN) {
        *pwExactSylIndex = wSylIndex;
        *(pwExactSylIndex + 1) = 0xFFFF;
    }
    else {
        /* no exact match */
        *pwExactSylIndex = 0xFFFF;
        if (bPartial) {
            /* add the first syllable to partial list */
            *pwPartSylIndex++ = wSylIndex;
        }
        else {
            return 0;
        }
    }

    if (bPartial) {
        for (i = (ET9U16)(wStartIndex + 1); i < wEndIndex; i++) {
            *pwPartSylIndex++ = (ET9U16)(pET9CPLingInfo->Private.PPrivate.adwPinyinBinTable[i] & 0x000001FF);
        }
        *pwPartSylIndex = 0xFFFF;
        ET9_CP_CompressSparseList(pwSavedPartSylIndex, pbPartSylIndexRange);
    }

    ET9_CP_CompressSparseList(pwExactSylIndex, pbExactSylIndexRange);

    return 1;
} /* end of ET9_CP_GetSyllableIndex() */


/*------------------------------------------------------------------------
 *
 *  Function    : ET9_CP_SylbToBIDRanges
 *
 *  Synopsis    : This function calculates the BID ranges for the spell,
 *                saves it in pwRange and updates pbSylIDRangeEnd.
 *
 *     Input    : pET9CPLingInfo = pointer to Chinese XT9 LingInfo structure
 *                pbSpell        = spelling of the syllable
 *                bSpellLen      = number of letter in the syllable
 *                bSylComplete   = need to find syllable completion
 *
 *    Output    : ppwRange       = pointer to pointer to BID ranges, *ppwRange has to
 *                                    point to an element in pET9CPLingInfo->CommonInfo.pwRange
 *                ppbRangeEnd  = pointer to pointer to end indices of BID ranges
 *
 *    Return    : 1 if success, 0 if failed.
 *
 *-----------------------------------------------------------------------*/
ET9BOOL ET9FARCALL ET9_CP_SylbToBIDRanges(ET9CPLingInfo *pET9CPLingInfo,
                                          ET9U8         *pbSpell,
                                          ET9U8          bSpellLen,
                                          ET9BOOL        bSylComplete,
                                          ET9U16       **ppwRange,
                                          ET9U8        **ppbRangeEnd)
{
    ET9U16 *pwSylPidTable;
    ET9U16 pwExactSylIndex[3], pwPartSylIndex[ET9_CP_MAX_PARTIAL_SYL + 1];      /* syllable index array */
    ET9U8 pbExactSylIndexRange[3], pbPartSylIndexRange[ET9_CP_MAX_PARTIAL_SYL + 1]; /* syllable index range array */
    ET9U8 b;

    if (ET9CPSymToCPTone(pbSpell[bSpellLen - 1]) || (ET9CPSYLLABLEDELIMITER == pbSpell[bSpellLen - 1])) {
        bSpellLen--;
    }

    if (!ET9_CP_GetSyllableIndex(pET9CPLingInfo, pbSpell, bSpellLen, bSylComplete,
                                 pwExactSylIndex, pbExactSylIndexRange,
                                 pwPartSylIndex, pbPartSylIndexRange))
    {
        return 0; /* cannot find the syllable */
    }
    pwSylPidTable = pET9CPLingInfo->Private.PPrivate.awSylPidTable;
    for (b = 0; pwExactSylIndex[b] != 0xFFFF; b++) {
        ET9Assert((*ppwRange - pET9CPLingInfo->CommonInfo.pwRange) <= (ET9_CP_MAX_ID_RANGE_SIZE - 3));

        **ppwRange = pwSylPidTable[pwExactSylIndex[b]];
        *(*ppwRange+1) = pwSylPidTable[pwExactSylIndex[b]+pbExactSylIndexRange[b]];
        *(*ppwRange+2) = *(*ppwRange+1); /* partial end = partial start */
        (*ppwRange) += ET9_CP_ID_RANGE_SIZE;
        (**ppbRangeEnd) += ET9_CP_ID_RANGE_SIZE;
    }

    if (bSylComplete) {
        for (b = 0; pwPartSylIndex[b] != 0xFFFF; b++) {
            ET9Assert((*ppwRange - pET9CPLingInfo->CommonInfo.pwRange) <= (ET9_CP_MAX_ID_RANGE_SIZE - 3));
            **ppwRange = pwSylPidTable[pwPartSylIndex[b]];
            *(*ppwRange+2) = pwSylPidTable[pwPartSylIndex[b]+pbPartSylIndexRange[b]];
            *(*ppwRange+1) = *(*ppwRange); /* partial start = exact start */
            (*ppwRange) += ET9_CP_ID_RANGE_SIZE;
            (**ppbRangeEnd) += ET9_CP_ID_RANGE_SIZE;
        }
    }
    return 1;

} /* end of ET9_CP_SylbToBIDRanges() */

/* ----------------------------------< eof >--------------------------------- */
