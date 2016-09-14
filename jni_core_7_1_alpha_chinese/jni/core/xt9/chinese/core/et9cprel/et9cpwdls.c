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
;**     FileName: et9cpwdls.c                                                 **
;**                                                                           **
;**  Description: Chinese Phrase Text Input phonetic wordlist module.         **
;**               Conforming to the development version of Chinese XT9.       **
;**                                                                           **
;*******************************************************************************
;******************************************************************************/

#include "et9api.h"
#include "et9cpldb.h"
#include "et9cpname.h"
#include "et9cpwdls.h"
#include "et9cppbuf.h"
#include "et9cptone.h"
#include "et9cpmisc.h"
#include "et9cpsys.h"

#define ET9_CP_BLOCK_OFFSET_PREFIX_INC     0x10000

#define ET9_CP_MULTI_PHRASE_MASK    0x8000
#define ET9_CP_WORDLIST_PID_MASK    0x7FFF
#define ET9_CP_PID_EOW_MASK         0x8000
#define ET9_CP_2ND_PID_EOW_MASK     0x8000
#define ET9_CP_SUBGROUP_SIZE_MASK   0x7FFF
#define ET9_CP_U8_NIBBLE_COUNT        0x02
#define ET9_CP_U16_NIBBLE_COUNT       0x04

/*---------------------------------------------------------------------------
 *
 *   Function: ET9_CP_PIDToPhraseGroup
 *
 *   Synopsis: This function looks up the word entry address for a given PID.
 *
 *     Input:  pET9CPLingInfo   = Pointer to Chinese XT9 LingInfo structure.
 *                       wPID   = the given PID.
 *
 *    Output:  pdwStartOffset   = Pointer to start offset of the desired phrase group
 *               pdwEndOffset   = Pointer to end offset of the desired phrase group
 *
 *     Return:
 *
 *---------------------------------------------------------------------------*/
static void ET9LOCALCALL ET9_CP_PIDToPhraseGroup(ET9CPLingInfo *pET9CPLingInfo,
                                                ET9U32        *pdwStartOffset,
                                                ET9U32        *pdwEndOffset,
                                                ET9U16         wPID)
{
    ET9U32  dwOffset, dwStartOffsetPrefix, dwEndOffsetPrefix;
    ET9_CP_CommonInfo *pCommInfo = &pET9CPLingInfo->CommonInfo;
    ET9UINT nBlockIndex;

    if (ET9_CP_IS_MUTE_PID(&pET9CPLingInfo->CommonInfo, wPID)) {
        *pdwStartOffset = 0;
        *pdwEndOffset = 0;
        return;
    }

    ET9Assert(ET9_CP_IS_NORMAL_PID(pCommInfo, wPID));
    ET9Assert(pdwStartOffset);
    ET9Assert(pdwEndOffset);

    /* Find the data block that contains the given PID as first char's PID.
       Adjust offset prefix along the way */
    dwStartOffsetPrefix = 0;
    for (nBlockIndex = 0; wPID > pCommInfo->pwMaxFirstPIDInBlock[nBlockIndex]; nBlockIndex++) {
        dwStartOffsetPrefix += ET9_CP_BLOCK_OFFSET_PREFIX_INC;
    }
    ET9Assert(nBlockIndex < ET9_CP_MAX_PHRASE_DATA_BLOCK_COUNT);
    dwEndOffsetPrefix = dwStartOffsetPrefix;
    if (wPID == pCommInfo->pwMaxFirstPIDInBlock[nBlockIndex]) { /* last block end is exclusive */
        /* at block end, phrase group spans beyond current addr prefix to the next prefix */
        dwEndOffsetPrefix += ET9_CP_BLOCK_OFFSET_PREFIX_INC;
    }

    /* Compute the Phrase group's absolute offset */
    /* The phrase group addr table has an entry for (MaxPID + 1) so we can get the end address for the MaxPID */
    dwOffset = (ET9U32)(pCommInfo->sOffsets.dwPhraseGroupAddrTableOffset + wPID * 2); /* sizeof(ET9U16) */

    *pdwStartOffset = (ET9U32)(ET9_CP_LdbReadWord(pET9CPLingInfo, dwOffset) + pCommInfo->sOffsets.dwPhraseDataOffset + dwStartOffsetPrefix);
    dwOffset += 2;
    *pdwEndOffset = (ET9U32)(ET9_CP_LdbReadWord(pET9CPLingInfo, dwOffset) + pCommInfo->sOffsets.dwPhraseDataOffset + dwEndOffsetPrefix);
} /* end ET9_CP_PIDToPhraseGroup() */

static ET9U16 ET9LOCALCALL ET9_CP_FindMaxPIDMatch(ET9CPLingInfo *pET9CPLingInfo,
                                                 ET9UINT nIDPosition, /* 0-based position */
                                                 ET9UINT fRangeIsSID,
                                                 const ET9U16 *pwContextPrefix,
                                                 ET9UINT nContextPrefixLen)
{
    ET9U16 wMaxPIDMatch;

    ET9Assert(0 == nContextPrefixLen || pwContextPrefix);

    if (nIDPosition < nContextPrefixLen) { /* context prefix */
        if (fRangeIsSID) {
            ET9UINT nAltCount, i;
            ET9U16 pwAltPID[ET9_CP_MAX_ALT_SYLLABLE];
            nAltCount = ET9_CP_LookupID(pET9CPLingInfo, pwAltPID, pwContextPrefix[nIDPosition], (ET9U8)ET9_CP_MAX_ALT_SYLLABLE, ET9_CP_Lookup_SIDToPID);
            ET9Assert(nAltCount > 0);
            wMaxPIDMatch = pwAltPID[0];
            for (i = 1; i < nAltCount; i++) {
                if (wMaxPIDMatch < pwAltPID[i]) {
                    wMaxPIDMatch = pwAltPID[i];
                }
            }
        }
        else { /* PID context prefix, only 1 PID */
            wMaxPIDMatch = pwContextPrefix[nIDPosition];
        }
    }
    else { /* non-context range */
        if (fRangeIsSID) {
            /* too slow to find max among all alt PIDs of each SID in range, just assume any ID would match */
            wMaxPIDMatch = (ET9U16)(ET9_CP_NORMAL_PID_COUNT(&pET9CPLingInfo->CommonInfo) - 1);
        }
        else { /* PID range */
            ET9UINT nRangeStart, nRangeEnd, i;
            ET9U16 *pwRange = pET9CPLingInfo->CommonInfo.pwRange;
            ET9U8 *pbRangeEnd = pET9CPLingInfo->CommonInfo.pbRangeEnd;

            nIDPosition = nIDPosition - nContextPrefixLen; /* becomes 0-based position after context */

            if (nIDPosition >= pET9CPLingInfo->CommonInfo.bSylCount) {
                /* ID position is beyond search criteria length, any ID should match */
                wMaxPIDMatch = (ET9U16)(ET9_CP_NORMAL_PID_COUNT(&pET9CPLingInfo->CommonInfo) - 1);
            }
            else { /* has range */
                /* compute range start and end for this ID position */
                if (nIDPosition) {
                    nRangeStart = pbRangeEnd[nIDPosition - 1] + 1;
                }
                else {
                    nRangeStart = 1;
                }
                nRangeEnd = pbRangeEnd[nIDPosition];

                /* loop through each range for this ID position */
                wMaxPIDMatch = pwRange[nRangeStart];
                for (i = nRangeStart + 1; i < nRangeEnd; i++) {
                    if (wMaxPIDMatch < pwRange[i]) {
                        wMaxPIDMatch = pwRange[i];
                    }
                }
                wMaxPIDMatch--; /* because range end is exclusive */
            }
        }
    }
    return wMaxPIDMatch;
} /* end ET9_CP_FindMaxPIDMatch() */

static ET9_CP_PhraseMatch ET9LOCALCALL ET9_CP_Cmp_ID_Range(ET9CPLingInfo    *pET9CPLingInfo,
                                                           ET9BOOL           bRangeIsSID,
                                                           const ET9U16     *pwContextPrefix,
                                                           ET9UINT           nContextPrefixLen,
                                                           ET9U8            *pbTones,
                                                           ET9BOOL           bGetToneOption,
                                                           ET9U8            *pbToneOptions,
                                                           ET9UINT           nIDPosition, /* 0-based position */
                                                           ET9U16           *pwMatchID) /* I/O matched ID */
{
    ET9UINT nAltCount, nAltIndex;
    ET9U16 pwAltID[ET9_CP_MAX_ALT_SYLLABLE];
    ET9U16 wPID;
    ET9Assert(!bGetToneOption || pbToneOptions);
    ET9Assert(0 == nContextPrefixLen || pwContextPrefix);

    wPID = *pwMatchID;

    if (bRangeIsSID) { /* Stroke mode */
        nAltCount = ET9_CP_LookupID(pET9CPLingInfo, pwAltID, wPID, (ET9U8)ET9_CP_MAX_ALT_SYLLABLE, ET9_CP_Lookup_PIDToSID);
    }
    else { /* Phonetic mode */
        pwAltID[0] = wPID;
        nAltCount = 1;
    }

    if (nIDPosition < nContextPrefixLen) { /* check the context prefix */
        ET9U16 wExpected;
        wExpected = pwContextPrefix[nIDPosition];
        for (nAltIndex = 0; nAltIndex < nAltCount; nAltIndex++) {
            if (pwAltID[nAltIndex] == wExpected) {
                *pwMatchID = wExpected;
                return ET9_CP_EXACT_MATCH;
            }
        }
        return ET9_CP_NO_MATCH;
    }
    {
    ET9UINT nSyllableCount = pET9CPLingInfo->CommonInfo.bSylCount;

    /* check given ID against active range */
    ET9U16 *pwRange = pET9CPLingInfo->CommonInfo.pwRange;
    ET9U8 *pbRangeEnd = pET9CPLingInfo->CommonInfo.pbRangeEnd;

    ET9UINT nRangeStart, nRangeEnd, nRangeSize, i;
    ET9U16 wStartID, wExactEndID, wPartialEndID;
    ET9U16 wCurrentAltID;
    ET9U8 bToneFlags;

    nIDPosition = nIDPosition - nContextPrefixLen; /* becomes 0-based position after context */
    ET9Assert(nIDPosition < nSyllableCount);

    /* compute range size and setup alt IDs */
    nRangeSize = ET9_CP_ID_RANGE_SIZE;

    /* compute range start and end for this ID */
    if (nIDPosition) {
        nRangeStart = pbRangeEnd[nIDPosition - 1];
    }
    else {
        nRangeStart = 0;
    }
    nRangeEnd = pbRangeEnd[nIDPosition];

    /* loop through each range for this ID */
    for (i = nRangeStart; i < nRangeEnd; ++i) {
        /* setup startID, exactEnd, partialEnd */
        wStartID = pwRange[i++];
        wExactEndID = pwRange[i++];
        wPartialEndID = pwRange[i];
        for (nAltIndex = 0; nAltIndex < nAltCount; nAltIndex++) {
            wCurrentAltID = pwAltID[nAltIndex];
            if (wCurrentAltID < wStartID || wCurrentAltID >= wPartialEndID) {
                continue; /* ID doesn't match range */
            }
            *pwMatchID = wCurrentAltID;
            /* exact match */
            if (pbTones && pbTones[nIDPosition]) {
                /* has tone info for this ID, do tone filtering */
                ET9Assert(nAltCount == 1);
                if (wCurrentAltID >= wExactEndID) {
                    return ET9_CP_NO_MATCH; /* tone mismatch */
                }
                bToneFlags = ET9_CP_LookupTone(pET9CPLingInfo, wCurrentAltID);
                if (0 == (bToneFlags & pbTones[nIDPosition])) {
                    return ET9_CP_NO_MATCH; /* tone mismatch */
                }
            }
            else if (bGetToneOption && nIDPosition == nSyllableCount - 1) { /* obsolete? (kwtodo) */
                ET9Assert(nAltCount == 1);
                /* need tone option, is last char, is exact match */
                if (wCurrentAltID >= wExactEndID) {
                    return ET9_CP_NO_MATCH; /* tone mismatch */
                }
                bToneFlags = ET9_CP_LookupTone(pET9CPLingInfo, wCurrentAltID);
                *pbToneOptions |= (ET9U8)(bToneFlags & ET9_CP_ALL_TONES_BIT_MASK);
            }
            else if (wCurrentAltID >= wExactEndID) {
                return ET9_CP_PARTIAL_SYL_MATCH;
            }
            return ET9_CP_EXACT_MATCH;
        } /* end loop each alt ID */
    } /* end loop each active range */
    }
    return ET9_CP_NO_MATCH;
} /* end ET9_CP_Cmp_ID_Range */

static void ET9LOCALCALL ET9_CP_SearchPhraseGroup(ET9CPLingInfo  *pET9CPLingInfo,
                                                  ET9U32          dwGroupStartOffset,
                                                  ET9U32          dwGroupEndOffset,
                                                  ET9BOOL         bNeedPartialSyl,
                                                  ET9BOOL         bNeedPartialPhrase,
                                                  ET9BOOL         b1stIDIsExact,
                                                  ET9BOOL         bIsSID,
                                                  const ET9U16   *pwContextPrefix,
                                                  ET9UINT         nContextPrefixLen,
                                                  ET9U8          *pbTones,
                                                  ET9BOOL         bContextPrefixIsPhrase,
                                                  ET9_CP_SpellMatch *pMatchType,
                                                  ET9BOOL         bGetToneOption,
                                                  ET9U8          *pbToneOptions,
                                                  ET9U16          wFirstID,
                                                  ET9_CP_PhraseBuf *pPhraseBuf)
{
    ET9UINT fEndOfPhrase, nSubgroupSize, nPhraseFreq, nPhraseLen, nCriteriaLen;
    ET9_CP_PhraseMatch ePhraseMatch, e2ndIDMatch, eIDMatch;
    ET9U32 dwCurrentOffset, dwNextSubgroupOffset;
    ET9U16 wMax2ndPIDMatch, w2ndID, wID;
    ET9SYMB pwPhrase[ET9_CP_MAX_LDB_PHRASE_SIZE];

    nCriteriaLen = pET9CPLingInfo->CommonInfo.bSylCount + nContextPrefixLen;
    if (0 == nContextPrefixLen) {
        pwPhrase[0] = wFirstID; /* no context prefix, add the first ID into phrase */
    }
    /* find max PID match on 2nd position in criteria */
    wMax2ndPIDMatch = ET9_CP_FindMaxPIDMatch(pET9CPLingInfo, 1, bIsSID,
                                            pwContextPrefix, nContextPrefixLen);
    dwNextSubgroupOffset = dwGroupStartOffset;
    do {
        dwCurrentOffset = dwNextSubgroupOffset;
        w2ndID = ET9_CP_LdbReadWord(pET9CPLingInfo, dwCurrentOffset); /* get the shared 2nd character in this subgroup */
        dwCurrentOffset += 2; /* sizeof(ET9U16) */
        fEndOfPhrase = 0;
        nPhraseLen = 2;

        /* multi-phrase subgroup */
        if (w2ndID & ET9_CP_MULTI_PHRASE_MASK) {
            w2ndID = (ET9U16)(w2ndID & ET9_CP_WORDLIST_PID_MASK);
            nSubgroupSize = ET9_CP_LdbReadWord(pET9CPLingInfo, dwCurrentOffset); /* read subgroup size */
            dwCurrentOffset += 2; /* sizeof(ET9U16) */
            if (nSubgroupSize & ET9_CP_2ND_PID_EOW_MASK) {
                nSubgroupSize = nSubgroupSize & ET9_CP_SUBGROUP_SIZE_MASK;
                fEndOfPhrase = 1;
            }
        }
        else { /* single-phrase subgroup */
            fEndOfPhrase = 1;
            nSubgroupSize = 3;
        }
        if (w2ndID > wMax2ndPIDMatch) {
            /* the 2nd ID of this subgroup is beyond the max match, subsequent subgroups won't match */
            return;
        }
        dwNextSubgroupOffset += nSubgroupSize;

        if (nPhraseLen > nCriteriaLen) {
            if (bNeedPartialPhrase) {
                /* phrase length is beyond search criteria length, treat as partial match. */
                e2ndIDMatch = ET9_CP_PARTIAL_PHRASE_MATCH;
                if (bIsSID) {
                    ET9U8 bAltCount;
                    /* convert PID to any corresponding SID because it's beyond search criteria */
                    bAltCount = ET9_CP_LookupID(pET9CPLingInfo, &w2ndID, w2ndID, 1, ET9_CP_Lookup_PIDToSID);
                    ET9Assert(bAltCount > 0);
                }
            }
            else {
                return;
            }
        }
        else { /* nPhraseLen <= nCriteriaLen */
            /* compare 2nd ID with corresponding range */
            e2ndIDMatch = ET9_CP_Cmp_ID_Range(pET9CPLingInfo, bIsSID,
                                             pwContextPrefix, nContextPrefixLen, pbTones,
                                             bGetToneOption, pbToneOptions,
                                             1, &w2ndID);
            if (pbToneOptions && ET9_CP_ALL_TONES_BIT_MASK == *pbToneOptions) {
                return; /* finished getting tone options */
            }
            else if (!(ET9_CP_EXACT_MATCH == e2ndIDMatch || ET9_CP_PARTIAL_SYL_MATCH == e2ndIDMatch && (bNeedPartialSyl || nPhraseLen == nCriteriaLen && bNeedPartialPhrase) ) ) {
                continue; /* exact match OR partial match (need partial syl or at end of criteria), otherwise: all phrases in this subgroup won't match */
            }
        }
        /* match criteria, step into this subgroup. if end of phrase, read freq will be done within the loop */

        if (nPhraseLen > nContextPrefixLen) { /* add ID into phrase */
            pwPhrase[nPhraseLen - nContextPrefixLen - 1] = w2ndID;
        }
        if (!b1stIDIsExact) { /* if 1stID is partial match, stay as partial match */
            e2ndIDMatch = ET9_CP_PARTIAL_SYL_MATCH;
        }
        ePhraseMatch = e2ndIDMatch;

        /* todo: can do more performance improvement by sorting phrases within subgroups by length */

        /* linearly step through each ID of each phrase in this subgroup */
        for (; dwCurrentOffset < dwNextSubgroupOffset;) {
            if (fEndOfPhrase) {
                /* read frequency */
                nPhraseFreq = ET9_CP_LdbReadByte(pET9CPLingInfo, dwCurrentOffset); /* read the frequency of this phrase */
                dwCurrentOffset++;
                if (ET9_CP_NO_MATCH != ePhraseMatch) { /* all IDs of this phrase match their corresponding range */

                    if (bContextPrefixIsPhrase) { /* verifying context prefix is a complete phrase */
                        ET9Assert(pMatchType);
                        if (nPhraseLen == nContextPrefixLen) {
                            *pMatchType = eExactMatch;
                            return;
                        }
                    }
                    else if (nPhraseLen >= nCriteriaLen && nPhraseLen > nContextPrefixLen) { /* phrase (not shorter than criteria) and (longer than context prefix) */
                        if (pMatchType) {
                            if (ET9_CP_EXACT_MATCH == ePhraseMatch) {
                                *pMatchType = eExactMatch;
                                return; /* found exact match: done */
                            }
                            else {
                                *pMatchType = ePartialMatch; /* found partial match: continue */
                            }
                        }
                        else if (!bGetToneOption) { /* get tone options doesn't need to fill phrase buffer */
                            ET9_CP_IDEncode eEncode = bIsSID ? ET9_CP_IDEncode_SID: ET9_CP_IDEncode_PID;
                            ET9U8 bSelectionLen = (ET9U8)(nPhraseLen - nContextPrefixLen);
                            ET9U8 bIsExact = (ET9U8)((ePhraseMatch == ET9_CP_EXACT_MATCH)? 1:0);
                            ET9U8 bIsContextMatch = (ET9U8)(nContextPrefixLen ? 1 : 0);
                            ET9U16 wFreqEncoded;
                            ET9_CP_Spell * pSpell = &pET9CPLingInfo->CommonInfo.SpellData.sSpell;
                            ET9UINT fSurpress;

                            ET9Assert(0 == pSpell->bLen || ET9CPIsModePhonetic(pET9CPLingInfo));

                            /* encode freq */
                            wFreqEncoded = ET9_CP_EncodeFreq(pET9CPLingInfo,
                                                             pwPhrase,
                                                             bSelectionLen,
                                                             (ET9U16)nPhraseFreq,
                                                             bIsExact, 
                                                             bIsContextMatch,
                                                             0,
                                                             &fSurpress);

                            if (!fSurpress) {
                                /* add to phrase buffer */
                                ET9_CP_AddPhraseToBuf(pET9CPLingInfo, pPhraseBuf,
                                                      pwPhrase, bSelectionLen, pSpell->pbChars, pSpell->bLen,
                                                      eEncode, wFreqEncoded);
                            }

                            if (nPhraseLen == 2) {
                                /* when the shared 2nd char is end-of-phrase (ie. criteria and context prefix are short),
                                   don't do phrase completion, skip this subgroup */
                                ET9Assert(nCriteriaLen < 3);
                                break;
                            }
                        }
                    }
                }
                fEndOfPhrase = 0; /* done with this phrase. continue with next phrase */
                nPhraseLen = 2;
                ePhraseMatch = e2ndIDMatch; /* restore phrase match to the 2nd ID's state for next phrase */
            }
            else { /* read the next character in the current phrase */

                if (bContextPrefixIsPhrase) { /* verifying context prefix is a complete phrase */
                    ET9Assert(pMatchType);
                    if (nContextPrefixLen <= 2) {
                        *pMatchType = eNoMatch;
                        return; /* all subsequent phrases of this subgroup have length > 2, other subgroups won't match this 2nd ID */
                    }
                    else if (nPhraseLen >= nContextPrefixLen) {
                        ePhraseMatch = ET9_CP_NO_MATCH; /* already reach context prefix length, no more comparisons */
                    }
                }
                wID = ET9_CP_LdbReadWord(pET9CPLingInfo, dwCurrentOffset); /* get the next ID in this phrase */
                dwCurrentOffset += 2; /* sizeof(ET9U16) */
                if (wID & ET9_CP_PID_EOW_MASK) {
                    fEndOfPhrase = 1;
                    wID = (ET9U16)(wID & ET9_CP_WORDLIST_PID_MASK);
                }

                if (ET9_CP_NO_MATCH != ePhraseMatch) { /* no mismatch so far */

                    nPhraseLen++;

                    if (nPhraseLen > nCriteriaLen) {
                        if (bNeedPartialPhrase) {
                            /* phrase length is beyond search criteria length, treat as partial match.
                               just keep reading until EOW without compare */
                            if (ET9_CP_EXACT_MATCH == ePhraseMatch) {
                                ePhraseMatch = ET9_CP_PARTIAL_PHRASE_MATCH; /* change EXACT to PARTIAL_PHRASE */
                            }
                            if (bIsSID) {
                                ET9U8 bAltCount;
                                /* convert PID to any corresponding SID because it's beyond search criteria */
                                bAltCount = ET9_CP_LookupID(pET9CPLingInfo, &wID, wID, 1, ET9_CP_Lookup_PIDToSID);
                                ET9Assert(bAltCount > 0);
                            }
                            pwPhrase[nPhraseLen - nContextPrefixLen - 1] = wID;
                        }
                        else { /* no need for phrases longer than criteria length */
                            ePhraseMatch = ET9_CP_NO_MATCH;
                        }
                    }
                    else { /* phrase length is within search criteria length */ /* nPhraseLen < nCriteriaLen will be rejected when handling EOW */
                        /* compare this PID with the corresponding criteria */
                        eIDMatch = ET9_CP_Cmp_ID_Range(pET9CPLingInfo, bIsSID,
                                                      pwContextPrefix, nContextPrefixLen, pbTones,
                                                      bGetToneOption, pbToneOptions,
                                                      nPhraseLen - 1, &wID);
                        if (pbToneOptions && ET9_CP_ALL_TONES_BIT_MASK == *pbToneOptions) {
                            return; /* finished getting tone options */
                        }
                        if (ET9_CP_EXACT_MATCH == eIDMatch || ET9_CP_PARTIAL_SYL_MATCH == eIDMatch && (bNeedPartialSyl || (nPhraseLen == nCriteriaLen && bNeedPartialPhrase) ) ) {
                            /* exact match OR partial match with need partial or at the EOW: this ID matches */

                            ET9Assert(nPhraseLen > nContextPrefixLen || /* if within context prefix, must be exact match */
                                      (ET9_CP_EXACT_MATCH == ePhraseMatch && ET9_CP_EXACT_MATCH == eIDMatch));

                            /* start adding PID into phrase and have partial match when beyond context prefix */
                            if (nPhraseLen > nContextPrefixLen) {
                                if (ET9_CP_EXACT_MATCH == ePhraseMatch) {
                                    /* exact match may change to partial but once partial, stay partial */
                                    ePhraseMatch = eIDMatch;
                                }
                                pwPhrase[nPhraseLen - nContextPrefixLen - 1] = wID;
                            }
                        }
                        else {
                            /* this char doesn't match, other phrases in this subgroup may match */
                            ePhraseMatch = ET9_CP_NO_MATCH; /* this ID mismatch, just keep reading until EOW without compare */
                        }
                    }
                } /* end no mismatch */
            } /* end non-end-of-phrase */
        } /* end loop through this subgroup */
    } while (dwNextSubgroupOffset < dwGroupEndOffset);
} /* end ET9_CP_SearchPhraseGroup() */


/*---------------------------------------------------------------------------
 *
 *   Function: ET9_CP_GetLdbPhrases
 *
 *   Synopsis: This function searches the word tree to find the matching phrases
 *             for the specified ID ranges in pET9CPLingInfo. Then add them
 *             into the phrase buffer. If it is phonetic input, it is PID range.
 *             Otherwise it is SID range.
 *
 *     Input:  pET9CPLingInfo    = Pointer to Chinese XT9 LingInfo structure.
 *             pbTones           = 32 Tones (32 bytes) in phonetic mode.
 *                                 If it is stroke mode, set it to NULL(0).
 *             bNeedPartialSyl   = 1 if need partial syllable match, 0 otherwise
 *             bNeedPartialPhrase= 1 if need partial phrase match, 0 otherwise
 *             bIsSID            = 0 - ID is encoded in PID; 1 - ID is encoded in SID;
 *             bGetToneOption    = 0 - do not get tone options and phrases are added
 *                                     into phrase buffer.
 *                                 1 - get tone options and phrases are not added into
 *                                     phrase buffer.
 *             pwCntxPrefix      = the context prefix phrase.
 *             bCntxPrefixLen    = the context prefix length.
 *           bValidateCntxPrefix = 0/1 - (do not) validate the given context prefix is in LDB.
 *                                 If bValidateCntxPrefix is 1, *pMatchType = eExactMatch if the given context prefix
 *                                 is a LDB word; otherwise, *pMatchType = eNoMatch.
 *    In/Out:  pMatchType        = if pMatchType is NULL, the function will search
 *                                 the word list, find all matched phrases and add them
 *                                 into the phrase buffer.
 *                               = if pMatchType is not NULL, the function will find the "best" match type among
 *                                 all matched phrase for given ID ranges.
 *                                 *pMatchType will be set to one of {eNoMatch, eExactMatch, ePartialMatch}.
 *             pNameTableBookmarks = bookmarks to speed up name table search, updated during search for next search
 *             pPhraseBuf        = phrase buffer, provided by caller, for storing the phrase found
 *
 *    Output:  pbToneOptions     = if bGetToneOption is 1, it stores the retrieved tone options.
 *                                 Its format is one byte and bit arrangement is: 000xxxxx.
 *                                 Least significant bit is for tone 1.
 *                                 2nd least significant bit is for tone 2, etc.
 *
 *     Return: ET9STATUS_NONE on success, otherwise return XT9 error code.
 *
 *---------------------------------------------------------------------------*/
void ET9FARCALL ET9_CP_GetLdbPhrases(
    ET9CPLingInfo *pET9CPLingInfo,
    ET9BOOL        bIsSID,
    ET9BOOL        bNeedPartialSyl,
    ET9BOOL        bNeedPartialPhrase,
    ET9_CP_SpellMatch *pMatchType,
    ET9BOOL        bGetToneOption, /* obsolete? no phrasal tone (kwtodo) */
    ET9U8         *pbToneOptions, /* obsolete? no phrasal tone */
    ET9U8         *pbTones,
    const ET9U16  *pwCntxPrefix,
    ET9U8          bCntxPrefixLen,
    ET9BOOL        bValidateCntxPrefix,
    ET9_CP_NameTableBookmark *pNameTableBookmarks,
    ET9_CP_PhraseBuf *pPhraseBuf)
{
    ET9U16           wFirstPID, wFirstIDStart, wFirstIDExactEnd, wFirstIDEnd, wID;
    ET9_CP_CommonInfo *pCommInfo = &pET9CPLingInfo->CommonInfo;
    ET9U8            bTone = 0, bCount, bNumFirstIDRange, bIndex = 0;
    ET9U8            bInc = ET9_CP_ID_RANGE_SIZE, bAltCount, bAltIndex;

#if 0
    if (bGetToneOption && (ET9_CP_ALL_TONES_BIT_MASK == (*pbToneOptions & ET9_CP_ALL_TONES_BIT_MASK))) {
        return;
    }
    if (bGetToneOption) {
        ET9Assert(pbToneOptions);
    }
#endif
    /* input validation */
    ET9Assert(!bCntxPrefixLen || pwCntxPrefix);
    ET9Assert((bValidateCntxPrefix && bCntxPrefixLen) || !bValidateCntxPrefix);

    if (pMatchType) {
        *pMatchType = eNoMatch; /* assume no match */
    }
    /* todo: revisit shortcuts w/ freq */
    /*
    else {
        if (!bValidateCntxPrefix &&
            !(ET9_CP_PhraseNeedLdb(pPhraseBuf) &&
                ((bCntxPrefixLen && ET9_CP_PhraseNeedContext(pPhraseBuf)) ||
                (!bCntxPrefixLen && ET9_CP_PhraseNeedNonContext(pPhraseBuf)))
           ))
        {
            return;
        }
    }
    */

    if (bIsSID || bCntxPrefixLen > 0) {
        bNumFirstIDRange = 1;
    }
    else { /* Phonetic without context prefix: may have multiple first ID ranges due to Mohu Pinyin */
        bNumFirstIDRange = (ET9U8)(pCommInfo->pbRangeEnd[0] / ET9_CP_ID_RANGE_SIZE);
    }

    /* - when no active keys, the ranges are not maintained, so we can't use ET9_CP_FindNameMatch to find the matches
       - no context match from Name table
       - name table contains only single characters, so no validation */
    if (!pMatchType && (0 == bCntxPrefixLen) &&
        ET9CPIsNameInputActive(pET9CPLingInfo) &&
        pET9CPLingInfo->Base.pWordSymbInfo->bNumSymbs)
    {
        ET9_CP_FindNameMatch(pET9CPLingInfo, (ET9UINT)bIsSID, 1, pNameTableBookmarks, pbTones, 1);
    }
    for (bCount = 0; bCount < bNumFirstIDRange; bCount++) {
        if (bCntxPrefixLen) { /* context prefix match: exact range has only 1 ID, no partial */
            wFirstIDStart = pwCntxPrefix[0];
            wFirstIDExactEnd = (ET9U16)(wFirstIDStart + 1);
            wFirstIDEnd = wFirstIDExactEnd;
        }
        else {
            bIndex = (ET9U8)(bCount * bInc);
            wFirstIDStart = pCommInfo->pwRange[bIndex];
            wFirstIDExactEnd = pCommInfo->pwRange[bIndex + 1];
            /* search partial match if need partial syl or at the last syl */
            wFirstIDEnd = (ET9U16)((bNeedPartialSyl || 1 == pCommInfo->bSylCount && bNeedPartialPhrase) ? pCommInfo->pwRange[bIndex + 2] : wFirstIDExactEnd);
        }

        for (wID = wFirstIDStart; wID < wFirstIDEnd; wID++) {
            ET9U16 pwAltPID[ET9_CP_MAX_ALT_SYLLABLE];
            if (bIsSID) {
                bAltCount = ET9_CP_LookupID(pET9CPLingInfo, pwAltPID, wID, (ET9U8)ET9_CP_MAX_ALT_SYLLABLE, ET9_CP_Lookup_SIDToPID);
            }
            else { /* only 1 desired PID */
                bAltCount = 1;
            }
            for (bAltIndex = 0; bAltIndex < bAltCount; bAltIndex++) {
                if (bIsSID) {
                    wFirstPID = pwAltPID[bAltIndex];
#ifndef ET9CP_DISABLE_STROKE
                    ET9Assert(!ET9_CP_IS_SYMBOL_PID(pCommInfo, wFirstPID));
                    if (ET9_CP_IS_COMP_PID(pCommInfo, wFirstPID) &&
                        (!ET9_CP_AllowComponent(pET9CPLingInfo) || /* discard component if a delimiter is entered */
                         (pET9CPLingInfo->CommonInfo.bKeyBufLen <= 1) ) )/* discard component with only 1 stroke */
                    {
                        continue; /* reject this PID: try next alt ID */
                    }
#endif
                }
                else {
                    if (pbTones && pbTones[0]) { /* 1st char has tone */
                        if (0 == bCntxPrefixLen) { /* this is the 1st char, do tone filtering only when no context prefix */
                            bTone = ET9_CP_LookupTone(pET9CPLingInfo, wID); /* Lookup tones */
                            if (!(pbTones[0] & bTone) || (wID >= wFirstIDExactEnd) ) {
                                continue;   /* tone mismatch or ID not exact match: try next alt ID */
                            }
                        }
                    }
                    wFirstPID = wID;
                }
                if (bCntxPrefixLen || (pCommInfo->bSylCount > 1)) { /* phrase search */
                    ET9U32 dwGroupStartOffset, dwGroupEndOffset;
                    ET9_CP_PIDToPhraseGroup(pET9CPLingInfo, &dwGroupStartOffset, &dwGroupEndOffset, wFirstPID); /* get the 1st char's phrase group offset and go there */
                    if (dwGroupStartOffset < dwGroupEndOffset) {
                        /* scan the phrases for this 1st char */
                        ET9_CP_SearchPhraseGroup(pET9CPLingInfo, dwGroupStartOffset, dwGroupEndOffset,
                                                 bNeedPartialSyl, bNeedPartialPhrase,
                                                 (ET9BOOL)(wID < wFirstIDExactEnd), bIsSID,
                                                 pwCntxPrefix, bCntxPrefixLen, pbTones,
                                                 bValidateCntxPrefix, pMatchType,
                                                 bGetToneOption, pbToneOptions, wID, pPhraseBuf);
                        if (pMatchType && eExactMatch == *pMatchType) {
                            return; /* found exact match: done */
                        }
                    }
                    else if (bCntxPrefixLen && (bAltIndex >= bAltCount - 1)) {
                        return;
                    }
                }
                else { /* single character search */
                    if (bGetToneOption) {
                        if (wID < wFirstIDExactEnd) { /* get tone option for exact match only */
                            bTone = ET9_CP_LookupTone(pET9CPLingInfo, wFirstPID);
                            *pbToneOptions |= (ET9U8)(bTone & ET9_CP_ALL_TONES_BIT_MASK);
                        }
                    }
                    else {
                        ET9U16 wFreqEncoded;
                        ET9_CP_Spell * pSpell = &pCommInfo->SpellData.sSpell;
                        ET9UINT fSurpress;
                        ET9U8 bFreq;

                        ET9Assert(0 == pSpell->bLen || ET9CPIsModePhonetic(pET9CPLingInfo));

                        if (pMatchType) {
                            *pMatchType = eExactMatch;
                            return;
                        }
                        bFreq = ET9_CP_FreqLookup(pET9CPLingInfo, wFirstPID);
                        wFreqEncoded = ET9_CP_EncodeFreq(pET9CPLingInfo, 
                                                         (ET9SYMB *)&wID,
                                                         /*bPhraseLen*/1, 
                                                         (ET9U16)bFreq,
                                                         (ET9U8)(wID < wFirstIDExactEnd),
                                                         /*bIsFromContext*/0, 
                                                         /*bIsFromUdb*/0, 
                                                         &fSurpress);
                        if (!fSurpress) {
                            ET9_CP_AddPhraseToBuf(pET9CPLingInfo, pPhraseBuf,
                                                  (ET9SYMB *)&wID, 1, pSpell->pbChars, pSpell->bLen,
                                                  bIsSID? ET9_CP_IDEncode_SID: ET9_CP_IDEncode_PID,
                                                  wFreqEncoded);
                        }
                    }
                    bAltCount = 1;    /* for single character, do not need to consider alternative IDs */
                }

                if (pbToneOptions && (ET9_CP_ALL_TONES_BIT_MASK == *pbToneOptions)) {
                    return;
                }
            } /* END for each alt index */
        } /* END for each first ID in the phrase */
    } /* END for each ID range of first character */
}   /* end of ET9_CP_GetLdbPhrases() */

ET9BOOL ET9FARCALL ET9_CP_FindPhraseInLdb(
    ET9CPLingInfo *pLingInfo,
    ET9_CP_IDEncode eEncode,
    const ET9CPPhrase *pPhrase)
{
    ET9U8 bSylCountBkup;
    ET9_CP_SpellMatch eMatchType;

    if (1 == pPhrase->bLen) { /* single character always in Ldb, caller has validated */
        return (ET9BOOL)1;
    }
    /* Backup bSylCount, then set pwRange to empty */
    bSylCountBkup = pLingInfo->CommonInfo.bSylCount;
    pLingInfo->CommonInfo.bSylCount = 0;

    ET9Assert(ET9_CP_IDEncode_PID == eEncode || ET9_CP_IDEncode_BID == eEncode || ET9_CP_IDEncode_SID == eEncode);
    /* Validate the phrase in LDB as context, no partial syl or partial phrase, set pwRange to empty */
    ET9_CP_GetLdbPhrases(pLingInfo, (ET9BOOL)(ET9_CP_IDEncode_SID == eEncode), 0, 0, &eMatchType, 0, NULL, NULL, pPhrase->pSymbs, pPhrase->bLen, 1, NULL, ET9_CP_GetMainPhraseBuf(pLingInfo));

    /* Restore bSylCount, thus restoring pwRange */
    pLingInfo->CommonInfo.bSylCount = bSylCountBkup;

    return (ET9BOOL)(eExactMatch == eMatchType);
}

ET9BOOL ET9LOCALCALL ET9_CP_PidIsInSet(ET9U16 wPID,
                                       ET9U16 *pwPIDSet,
                                       ET9U8 bSetSize)
{
    ET9U8 i;
    for (i = 0; i < bSetSize && pwPIDSet[i] != ET9_CP_NOMATCH; i++) {
        if (wPID == pwPIDSet[i]) {
            return 1;
        }
    }
    return 0;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Interface for other modules (eg. T9Write) to get Ldb phrase frequency.
 *
 * @param pLing         I   - pointer to Chinese LingInfo structure
 * @param pPhrase       I   - pointer to the target phrase in Unicode
 *
 * @return              frequency of pPhrase (1st match in Ldb) (range 0 to 255)
 *                      -1 if phrase is not in Ldb.
 */
ET9INT ET9FARCALL ET9_CP_GetPhraseFreq(ET9CPLingInfo *pLing,
                                       const ET9CPPhrase *pPhrase)
{
    ET9INT iFreq = -1;
    ET9U16 awPID[ET9_CP_MAX_LDB_PHRASE_SIZE][ET9_CP_MAX_ALT_SYLLABLE];

    /* validate parameters */
    ET9Assert(!ET9_CP_IS_LINGINFO_NOINIT(pLing) );
    ET9Assert( !(NULL == pPhrase || 0 == pPhrase->bLen || pPhrase->bLen > ET9_CP_MAX_LDB_PHRASE_SIZE) );

    if (!ET9_CP_UniPhraseToAltPID(pLing, pPhrase, (ET9U16*)awPID, ET9_CP_MAX_ALT_SYLLABLE) ) {
        return -1; /* some Unicode has no PID, return */
    }

    if (pPhrase->bLen == 1)
    {
        iFreq = (ET9INT)ET9_CP_FreqLookup(pLing, awPID[0][0]); /* return the freq of the most common PID */
    }
    else
    {
        int j;
        /* try each alt PID of the 1st char in phrase */
        for (j = 0; j < ET9_CP_MAX_ALT_SYLLABLE && awPID[0][j] != ET9_CP_NOMATCH; j++)
        {
            ET9U32 dwGroupStartOffset, dwGroupEndOffset;
            ET9U32 dwCurrentOffset, dwNextSubgroupOffset;
            ET9INT nSubgroupSize, nPhraseLen;
            ET9U16 w2ndID, wID;
            ET9BOOL fEndOfPhrase;

            ET9_CP_PIDToPhraseGroup(pLing, &dwGroupStartOffset, &dwGroupEndOffset, awPID[0][j]); /* get the 1st char's phrase group offset and go there */

            dwNextSubgroupOffset = dwGroupStartOffset;
            do {
                dwCurrentOffset = dwNextSubgroupOffset;
                w2ndID = ET9_CP_LdbReadWord(pLing, dwCurrentOffset); /* get the shared 2nd character in this subgroup */
                dwCurrentOffset += 2; /* sizeof(ET9U16) */
                fEndOfPhrase = 0;

                /* multi-phrase subgroup */
                if (w2ndID & ET9_CP_MULTI_PHRASE_MASK) 
                {
                    w2ndID = (ET9U16)(w2ndID & ET9_CP_WORDLIST_PID_MASK);
                    nSubgroupSize = ET9_CP_LdbReadWord(pLing, dwCurrentOffset); /* read subgroup size */
                    dwCurrentOffset += 2; /* sizeof(ET9U16) */
                    if (nSubgroupSize & ET9_CP_2ND_PID_EOW_MASK) 
                    {
                        nSubgroupSize = nSubgroupSize & ET9_CP_SUBGROUP_SIZE_MASK;
                        fEndOfPhrase = 1;
                    }
                }
                else { /* single-phrase subgroup */
                    fEndOfPhrase = 1;
                    nSubgroupSize = 3;
                }
                dwNextSubgroupOffset += nSubgroupSize;

                if (ET9_CP_PidIsInSet(w2ndID, awPID[1], ET9_CP_MAX_ALT_SYLLABLE) )
                {   /* 2nd PID matches, linearly step through each ID of each phrase in this subgroup */
                    int fMatch = 1;
                    nPhraseLen = 2;
                    do {
                        if (fEndOfPhrase) 
                        {   /* read frequency */
                            iFreq = (ET9INT)ET9_CP_LdbReadByte(pLing, dwCurrentOffset); /* read the frequency of this phrase */
                            dwCurrentOffset++;
                            if (fMatch && nPhraseLen == pPhrase->bLen) 
                            {
                                return iFreq;  /* Found it, return freq. */
                            }
                            fEndOfPhrase = 0; /* done with this phrase. continue with next phrase */
                            nPhraseLen = 2;
                            fMatch = 1;
                        }
                        else 
                        { /* read the next character in the current phrase */
                            wID = ET9_CP_LdbReadWord(pLing, dwCurrentOffset); /* get the next ID in this phrase */
                            dwCurrentOffset += 2; /* sizeof(ET9U16) */
                            if (wID & ET9_CP_PID_EOW_MASK) 
                            {
                                fEndOfPhrase = 1;
                                wID = (ET9U16)(wID & ET9_CP_WORDLIST_PID_MASK);
                            }
                            if ( fMatch )
                            {
                                if (ET9_CP_PidIsInSet(wID, awPID[nPhraseLen], ET9_CP_MAX_ALT_SYLLABLE) )
                                {   /* no mismatch so far */
                                    nPhraseLen++;
                                }
                                else
                                { 
                                    fMatch = 0;
                                }
                            }
                        } 
                    } while ( dwCurrentOffset < dwNextSubgroupOffset );
                }
            } while (dwNextSubgroupOffset < dwGroupEndOffset); /* END each subgroup of this 1st char's group */
        } /* END each alt PID of 1st char in phrase */
    }
    return iFreq;
}

/** Retrieve the frequency of a given phrase.
 *  This is provided to external modules as an additional information for phrase sorting.
 *
 *  @param pET9CPLingInfo   (input) pointer to chinese information structure.
 *  @param pPhrase          (input) The desired phrase in Unicode.
 *  @param pnFreq           (output) The frequency of the phrase if found.
 *
 *  @return ET9STATUS_NONE               Success
 *  @return ET9STATUS_NO_INIT            pET9CPLingInfo is not properly initialized
 *  @return ET9STATUS_BAD_PARAM          some argument pointer is NULL
 *  @return ET9STATUS_NO_MATCH           the give phrase is not found, content of pnFreq is undefined.
 *
 */
ET9STATUS ET9FARCALL ET9CPGetPhraseFreq(ET9CPLingInfo *pET9CPLingInfo,
                                        const ET9CPPhrase *pPhrase,
                                        ET9INT *pnFreq)
{
    /* validate inputs */
    ET9_CP_CHECK_LINGINFO(pET9CPLingInfo);

    if (NULL == pPhrase || NULL == pnFreq) {
        return ET9STATUS_BAD_PARAM;
    }
    if (0 == pPhrase->bLen || pPhrase->bLen > ET9_CP_MAX_LDB_PHRASE_SIZE) {
        return ET9STATUS_NO_MATCH;
    }

    *pnFreq = ET9_CP_GetPhraseFreq(pET9CPLingInfo, pPhrase);
    if (-1 == *pnFreq) {
        return ET9STATUS_NO_MATCH;
    }

    return ET9STATUS_NONE;
}

/* ----------------------------------< eof >--------------------------------- */
