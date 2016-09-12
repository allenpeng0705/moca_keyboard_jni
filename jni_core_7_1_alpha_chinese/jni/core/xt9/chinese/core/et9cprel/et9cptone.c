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
;**     FileName: et9cptone.c                                                  **
;**                                                                           **
;**  Description: Chinese Phrase Text Input phonetic tone module.             **
;**               Conforming to the development version of Chinese XT9.               **
;**                                                                           **
;*******************************************************************************
;******************************************************************************/

#include "et9api.h"
#include "et9cpsys.h"
#include "et9cpkey.h"
#include "et9cpinit.h"
#include "et9cprdb.h"
#include "et9cpspel.h"
#include "et9cptone.h"
#include "et9cpsys.h"
#include "et9cpname.h"
#include "et9cpwdls.h"
#include "et9cppbuf.h"
#include "et9cpmisc.h"
#include "et9cpldb.h"
#include "et9cstrie.h"
#include "et9cptrace.h"


#include "et9imu.h"




/*---------------------------------------------------------------------------
 *
 *   Function: ET9_CP_GetTones
 *
 *   Synopsis: This function gets the valid tones for the current active spell
 *
 *     Input:  pET9CPLingInfo    = Pointer to Chinese XT9 LingInfo structure.
 *             pSpell            = pointer to the active spell
 *
 *     Return:  valid tone bit mask.
 *                                 Its output format is defined as follows:
 *
 *              x       x       x       x       x       x       x       x
 *                                   tone 5  tone 4  tone 3  tone 2  tone 1
 *
 *---------------------------------------------------------------------------*/
ET9U8 ET9FARCALL ET9_CP_GetTones(ET9CPLingInfo * pET9CPLingInfo, ET9_CP_Spell * pSpell)
{
    ET9_CP_CommonInfo *pCommon;
    ET9U16 wPID, wExactStart, wExactEnd;
    ET9U8 bSylCount, bTone, i;

    pCommon = &(pET9CPLingInfo->CommonInfo);

    /* set PID ranges */
    bSylCount = ET9_CP_SpellingToPidRanges(pET9CPLingInfo, pSpell->pbChars, pSpell->bLen);
    if ( bSylCount == 0 )
    {
        return 0;  /* Mute char OR S# but partial spelling off */
    }
    ET9Assert(1 == bSylCount); /* should have at most 1 syllable */
    bTone = 0;
    /* loop thru each PID range for this syllable */
    for (i = 0; i < pCommon->pbRangeEnd[0] && bTone != ET9_CP_ALL_TONES_BIT_MASK; i += ET9_CP_ID_RANGE_SIZE) {
        wExactStart = pCommon->pwRange[i];
        wExactEnd = pCommon->pwRange[i + 1];
        /* loop thru each PID in range and combine their available tones */
        for (wPID = wExactStart; wPID < wExactEnd && bTone != ET9_CP_ALL_TONES_BIT_MASK; wPID++) {
            bTone = (ET9U8)(bTone | (ET9_CP_LookupTone(pET9CPLingInfo, wPID) & ET9_CP_ALL_TONES_BIT_MASK));
        }
    }
    return bTone;
} /* END ET9_CP_GetTones() */

/** Retrieve all the valid tones for the active spelling if the active spell is a single syllable.
 *
 *   Function: ET9CPGetToneOptions
 *
 *  This function gets all valid tones for last valid syllable for
 *  the current selected(active). The least significant 5 bits in
 *  pbToneBitMask indicate the 5 tone options.
 *
 * @param pET9CPLingInfo     pointer to chinese information structure:
 * @param pbToneBitMask      returns the value indicating which tone marks are valid:
 *                         - Tone 1 (the even tone) is valid if the least significant bit is set to 1.
 *                         - Tone 2 (the rising tone) is valid if the second least significant bit is set to 1.
 *                         - Tone 3 (the dipping tone) is valid if the third least significant bit is set to 1.
 *                         - Tone 4 (the falling tone) is valid if the fourth least significant bit is set to 1.
 *                         - Tone 5 (the soft or muted tone) is valid if the fifth least significant bit is set to 1.
 *
 *
 * @return ET9STATUS_NONE               Succeeded
 * @return ET9STATUS_NO_INIT            pET9CPLingInfo is not properly initialized
 * @return ET9STATUS_NEED_SELLIST_BUILD Should call ET9CPBuildSelectionList() before call this function
 * @return ET9STATUS_BAD_PARAM          Some argument pointer is NULL
 * @return ET9STATUS_INVALID_MODE       Not phonetic mode
 *
 */
ET9STATUS ET9FARCALL ET9CPGetToneOptions(ET9CPLingInfo *pET9CPLingInfo,
                                      ET9U8 *pbToneBitMask)
{
    ET9_CP_Spell * psSpell;
    ET9_CP_CommonInfo *pCommon;

    ET9_CP_CHECK_LINGINFO(pET9CPLingInfo);
    ET9_CP_CHECK_UDB_UP_TO_DATE(pET9CPLingInfo);
    ET9_CP_CHECK_BUILD_UP_TO_DATE(pET9CPLingInfo);

    if (!pbToneBitMask) {
        return ET9STATUS_BAD_PARAM;
    }
    if (!(ET9CPIsModePinyin(pET9CPLingInfo) || ET9CPIsModeBpmf(pET9CPLingInfo))) {
        return ET9STATUS_INVALID_MODE;
    }

    *pbToneBitMask = 0;
    /* no input => no tone */
    if ( pET9CPLingInfo->Base.pWordSymbInfo->bNumSymbs == 0 ) {
        return ET9STATUS_NONE;
    }
    pCommon = &(pET9CPLingInfo->CommonInfo);
    psSpell = &pCommon->sActiveSpell;

    ET9Assert(ET9_CP_HasActiveSpell(pET9CPLingInfo)); /* should have been set by build selection list */
    ET9Assert(ET9CPIsModePhonetic(pET9CPLingInfo));

    if (1 == ET9_CP_CountSyl(pET9CPLingInfo->eMode, psSpell)) {
        *pbToneBitMask = ET9_CP_GetTones(pET9CPLingInfo, psSpell);
    }

    return ET9STATUS_NONE;
}   /* end of ET9CPGetToneOptions() */

/** Appends a tone mark to the end of the WordSymbInfo and locks the spelling.

In order to add a tone to ET9WordSymbInfo, application should call this function where bTone is one of
- ET9CPTONE1  -- Even tone.
- ET9CPTONE2  -- Rising tone.
- ET9CPTONE3  -- Dipping tone (falling then rising).
- ET9CPTONE4  -- Falling tone.
- ET9CPTONE5  -- Soft or muted tone.

and psSpell is the spelling the tone will be added. Every letter in psSpell must occur in the corresponding position of pWordSymbInfo.

@param pWordSymbInfo  pointer to word symbol information structure.
@param psSpell        A spell the tone will be added to.
@param bTone          Tone mark to be added.

@return ET9STATUS_NONE                 Succeeded
@return ET9STATUS_INVALID_MEMORY       NULL pointer is passed in
@return ET9STATUS_INVALID_INPUT        bTone is not a tone OR tone can not be added at this position OR psSpell and pWordSymbInfo have different length OR
                                   some char in psSpell does not occur in the corresponding position of pWordSymbInfo
@return ET9STATUS_FULL                 The active word is already the maximum size allowed (ET9MAXWORDSIZE).
@return ET9STATUS_INVALID_TEXT         bTone is not valid to add to ET9WordSymbInfo.
*/
ET9STATUS ET9FARCALL ET9CPAddToneSymb(ET9WordSymbInfo *pWordSymbInfo, const ET9CPSpell* psSpell, ET9CPSYMB bTone)
{
    ET9STATUS  status;
    ET9SYMB    sLastSymb;
    ET9SymbInfo *pSI;
    ET9U8      bInputCount, i;
    ET9BOOL    bPrevIsToneDelim;
    ET9SimpleWord wrd;
    if (psSpell == 0 || pWordSymbInfo == 0) {
        return ET9STATUS_INVALID_MEMORY;
    }
    if (psSpell->bLen == 0) {
        return ET9STATUS_INVALID_INPUT;
    }
    if (bTone < ET9CPTONE1 || bTone > ET9CPTONE5) {
        return ET9STATUS_INVALID_INPUT;
    }
    sLastSymb = psSpell->pSymbs[psSpell->bLen - 1];
    if (sLastSymb == ET9CPSYLLABLEDELIMITER || ET9CPSymToCPTone(sLastSymb) != 0) {
        return ET9STATUS_INVALID_INPUT;
    }
#ifdef ET9_ALPHABETIC_MODULE
    if (_ET9HasTraceInfo(pWordSymbInfo)) {
        status = ET9_CP_Trace_ReplaceByExplicitSymb(pWordSymbInfo, pWordSymbInfo->bNumSymbs, psSpell->pSymbs, psSpell->bLen);
        if(ET9STATUS_NONE != status) {
            return ET9STATUS_INVALID_INPUT;
        }
    }
#endif
    if (psSpell->bLen != pWordSymbInfo->bNumSymbs) {
        return ET9STATUS_INVALID_INPUT;
    }

    bPrevIsToneDelim = 0;
    pSI = pWordSymbInfo->SymbsInfo;
    bInputCount = 0;
    for (i = 0; i < psSpell->bLen; i++) {
        if (ET9CP_SEGMENT_DELIMITER == psSpell->pSymbs[i]) {
            if (!ET9CPIsUpperCaseSymbol(psSpell->pSymbs[i + 1]) ) {
                /* non-upper case after segment delimiter ==> invalid */
                return ET9STATUS_INVALID_INPUT;
            }
            continue; /* skip segment delimiter */
        }
        bInputCount++;
        if (!ET9_CP_IsSymbInSymbInfo(psSpell->pSymbs[i], pSI)) {
            return ET9STATUS_INVALID_INPUT;
        }
        if (bPrevIsToneDelim && !ET9CPIsUpperCaseSymbol(psSpell->pSymbs[i])) {
            /* non-upper case after tone/delimiter ==> invalid */
            return ET9STATUS_INVALID_INPUT;
        }

        if (psSpell->pSymbs[i] == ET9CPSYLLABLEDELIMITER || ET9CPSymToCPTone(psSpell->pSymbs[i]) != 0) {
            bPrevIsToneDelim = 1;
        }
        else {
            bPrevIsToneDelim = 0;
        }
        pSI++;
    }
    if (bInputCount != pWordSymbInfo->bNumSymbs) {
        return ET9STATUS_INVALID_INPUT;
    }

    wrd.wCompLen = 0;
    for (wrd.wLen = 0;  wrd.wLen < (ET9U16)psSpell->bLen; wrd.wLen++) {
        wrd.sString[wrd.wLen] = (ET9SYMB)psSpell->pSymbs[wrd.wLen];
    }
    status = ET9AddExplicitSymb(pWordSymbInfo, (ET9SYMB)bTone, ET9NOSHIFT, ET9_NO_ACTIVE_INDEX);
    if (ET9STATUS_NONE == status) {
        wrd.sString[wrd.wLen++] = (ET9SYMB)bTone;
        status = ET9LockWord(pWordSymbInfo, &wrd);
        if (ET9STATUS_NONE != status) {
            ET9ClearOneSymb(pWordSymbInfo);
        }
    }

    return status;
}

ET9U8 ET9FARCALL ET9_CP_GetSpellTones(ET9U8 *pbSpell, ET9U8 bLen, ET9U8 *pbTones)
{
    ET9U8 b, bSyls = 0;
    ET9Assert(bLen);
    ET9Assert(ET9_CP_IsPinyinUpperCase(*pbSpell) || ET9_CP_IsBpmfUpperCase(*pbSpell));

    for(b = 0; b < bLen; b++, pbSpell++) {
        ET9U8 bTmp;
        if (ET9_CP_IsPinyinUpperCase(*pbSpell) || ET9_CP_IsBpmfUpperCase(*pbSpell)) {
            if (bSyls) { /* use next slot for tone after first syllable */
                pbTones++;
            }
            bSyls++;
            *pbTones = 0;
        }
        /* calculating the tone info */
        bTmp = *pbSpell;
        if (ET9CPSymToCPTone(bTmp)) {
            *pbTones = (ET9U8)(1 << (bTmp - ET9CPTONE1));
        }
    }
    return bSyls;
}

/* ----------------------------------< eof >--------------------------------- */
