/*******************************************************************************
;*******************************************************************************
;**                                                                           **
;**                    COPYRIGHT 2010-2011 NUANCE COMMUNICATIONS              **
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
;**     FileName: et9cptrace.c                                                **
;**                                                                           **
;**  Description: Chinese XT9 Trace module                                    **
;**               Conforming to the development version of Chinese XT9        **
;**                                                                           **
;*******************************************************************************
;******************************************************************************/

#include "et9cpapi.h"
#include "et9cptrace.h"
#include "et9cpsys.h"
#include "et9cppbuf.h"
#include "et9cstrie.h"
#include "et9cptrace.h"
#include "et9cpspel.h"
#include "et9cprdb.h"
#include "et9cpldb.h"
#include "et9imu.h"
#include "et9misc.h"
#include "et9sym.h"

#ifdef ET9_ALPHABETIC_MODULE
#include "et9amisc.h"
#include "et9asys.h"


void ET9FARCALL ET9_CP_Trace_Clear(ET9_CP_Trace * pTrace)
{
    pTrace->pCLing = NULL;
    pTrace->pALing = NULL;
    pTrace->bNumSpells = 0;
    pTrace->bActiveSpell = 0xFF;
    pTrace->fPrefixOnly = 1;
    pTrace->fDBLoaded = 0;
    pTrace->sDefaultSpell.bLen = 0;
}

static void ET9LOCALCALL _ET9_CP_Trace_AWWordToSpell(ET9_CP_Spell * pSpell, ET9AWWordInfo * pWord)
{
    ET9U16 w;
    ET9U16 wWordSrcLen = pWord->wWordLen;
    ET9SYMB * psWordSrc = pWord->sWord;

    pSpell->bLen = 0;
    for (w = 0; w < wWordSrcLen; w++) {
        pSpell->pbChars[pSpell->bLen++] = ET9_CP_ExternalPhoneticToInternal(psWordSrc[w]);
    }
}

static ET9BOOL ET9LOCALCALL _ET9_CP_Trace_StrEqual(ET9U8 * pb1, ET9U8 * pb2, ET9UINT nLen) 
{
    ET9UINT n;

    for (n = 0; n < nLen; n++) {
        if (*pb1++ != *pb2++) {
            return 0;
        }
    }
    return 1;
}

/**
 *  Check if there is a tap after trace, return the 1st tap location.
 */
ET9UINT ET9LOCALCALL _ET9_CP_Trace_HasTapAfterTrace(ET9WordSymbInfo * pWSI)
{
    ET9UINT n, nNumSymbs;
    ET9BOOL fHasTrace;

    fHasTrace = 0;
    nNumSymbs = pWSI->bNumSymbs;
    for (n = 0; n < nNumSymbs; n++) {
        ET9U8 bTraceIndex = pWSI->SymbsInfo[n].bTraceIndex;
        if (bTraceIndex) {
            fHasTrace = 1;
        }
        else if (fHasTrace) {
            return n;
        }
    }

    return 0;
}

/**
 *  Calling for getting alpha core result as Chinese spell.
 */
static ET9STATUS ET9LOCALCALL _ET9_CP_Trace_GetAWWord(const ET9_CP_Trace *  pTrace,
                                                                  ET9U16    wIndex,
                                                            ET9_CP_Spell *  psSpell,
                                                                 ET9BOOL *  pfIsCompletion)
{
    ET9STATUS status;
    ET9AWWordInfo * psWord;

    status = ET9AWSelLstGetWord(pTrace->pALing, &psWord, (ET9U8)wIndex);

    if (ET9STATUS_NONE != status) {
        return status;
    }

    _ET9_CP_Trace_AWWordToSpell(psSpell, psWord);

    if (pfIsCompletion) {
        *pfIsCompletion = (ET9BOOL)(psWord->wWordCompLen ? 1 : 0);
    }

    return status;
}

/**
 *  Get the spelling of the default phrase (index 0).
 */
static void ET9LOCALCALL _ET9_CP_Trace_GetDefaultPhraseSpell(ET9CPLingInfo * pCLing, ET9_CP_Spell * pSpell)
{
    ET9STATUS status;
    ET9CPPhrase phrase;
    ET9CPSpell spell;
    ET9U8 b;

    status = ET9_CP_GetPhrase(pCLing, 0, &phrase, &spell);
    ET9Assert(ET9STATUS_NONE == status);
    for (b = 0; b < spell.bLen; b++) {
        pSpell->pbChars[b] = ET9_CP_ExternalSpellCodeToInternal(pCLing, spell.pSymbs[b]);
    }
    pSpell->bLen = spell.bLen;
}

static void ET9LOCALCALL _ET9_CP_Trace_TrimSpellTo1stSyl(ET9_CP_Spell *psSpell)
{
    ET9U8 b;

    for (b = 1; b < psSpell->bLen; b++) {
        if (ET9_CP_IsUpperCase(psSpell->pbChars[b])) {
            psSpell->bLen = b;
            break;
        }
    }
}

static ET9STATUS ET9LOCALCALL _ET9_CP_Trace_GetPrefix(const ET9_CP_Trace * pTrace,
                                                      ET9U16               wIndex,
                                                      ET9_CP_Spell       * psSpell)
{
    ET9STATUS status = ET9STATUS_NONE;
    ET9U16 wCoreIndex;
    ET9U16 wExtIndex = 0;
    ET9U8 bTargetLen;
    ET9_CP_Spell sActiveSpellPrefix = pTrace->sDefaultSpell;
    ET9U16 wCoreMaxCount = 5;

    _ET9_CP_Trace_TrimSpellTo1stSyl(&sActiveSpellPrefix);

    for (bTargetLen = ET9_CP_MAX_SINGLE_SYL_SIZE; bTargetLen > 0; bTargetLen--) {
        
        /**
         *  Make sure add the first syllable of the active spelling to prefix list.
         */
        if (bTargetLen == sActiveSpellPrefix.bLen && 
            ET9STATUS_NONE == _ET9_CP_Trace_GetAWWord(pTrace, 0, psSpell, NULL)) {     /* Check whether the alpha core is validated. */
            /**
             *  This is only called once since bTargetLen will never be equal to ActiveSpellPrefix.bLen
             *  in next loop. wExtIndex is samller or equal to wIndex. If wExtIndex is smaller than wIndex,
             *  that means this sActiveSpellPrefix is already be gotten by a smaller wIndex.
             */
            if (wExtIndex == wIndex) {
                psSpell->bLen = sActiveSpellPrefix.bLen;
                _ET9ByteCopy(psSpell->pbChars, sActiveSpellPrefix.pbChars, sActiveSpellPrefix.bLen);
                return ET9STATUS_NONE;
            }
            wExtIndex++;
        }

        for (wCoreIndex = 0; bTargetLen == 1 || wCoreIndex < wCoreMaxCount; wCoreIndex++) {
            ET9BOOL fIsCompletion;

            status = _ET9_CP_Trace_GetAWWord(pTrace, wCoreIndex, psSpell, &fIsCompletion);
            if (ET9STATUS_NONE != status) {     
                break;      /* Done with current bTargetLen, try next. */
            }

            _ET9_CP_Trace_TrimSpellTo1stSyl(psSpell);
            
            if (bTargetLen == psSpell->bLen) {
                ET9U16 wCoreDupIndex;
                ET9_CP_Spell sDupSpell;

                /**
                 *  If psSpell is same with sActiveSpellPrefix, that means this psSpell is 
                 *  already gotten by a smaller wIndex. So in this case, psSpell is ignored here.
                 */
                if (sActiveSpellPrefix.bLen != psSpell->bLen ||
                    !_ET9_CP_Trace_StrEqual(sActiveSpellPrefix.pbChars, psSpell->pbChars, sActiveSpellPrefix.bLen))
                {
                    /**
                     *  Check whether exist a same prefix spell in font of wCoreIndex.
                     *  If exist, that means this psSpell is already gotten by a smaller wCoreIndex.
                     *  So in this case, psSpell is ignored here.
                     */
                    for (wCoreDupIndex = 0; wCoreDupIndex < wCoreIndex; wCoreDupIndex++) {
                        _ET9_CP_Trace_GetAWWord(pTrace, wCoreDupIndex, &sDupSpell, NULL);
                        _ET9_CP_Trace_TrimSpellTo1stSyl(&sDupSpell);

                        if (sDupSpell.bLen == psSpell->bLen &&
                            _ET9_CP_Trace_StrEqual(sDupSpell.pbChars, psSpell->pbChars, sDupSpell.bLen))
                        {
                            if (fIsCompletion) {
                                wCoreMaxCount++;
                            }
                            break;
                        }
                    }

                    /* No dup found */
                    if (wCoreDupIndex == wCoreIndex) {   
                        if (wExtIndex == wIndex) {
                            return ET9STATUS_NONE;
                        }
                        wExtIndex++;
                    }
                }
                else if (fIsCompletion) {
                    wCoreMaxCount++;
                }
            }
        }
    }
    return ET9STATUS_OUT_OF_RANGE;
}


static ET9BOOL ET9LOCALCALL _ET9_CP_Trace_PrefixMatchesSpell(ET9_CP_Spell * psPrefix,
                                               ET9_CP_Spell * psSpell)
{
    if (psPrefix->bLen <= psSpell->bLen
        && _ET9_CP_Trace_StrEqual(psPrefix->pbChars, psSpell->pbChars, psPrefix->bLen)
        && (psPrefix->bLen == psSpell->bLen || ET9_CP_IsUpperCase(psSpell->pbChars[psPrefix->bLen]) ) ) {
        return 1;
    }
    else {
        return 0;
    }
}



ET9STATUS ET9FARCALL ET9_CP_Trace_GetPrefix(const ET9_CP_Trace * pTrace,
                                    ET9U16              wIndex,
                                    ET9CPSpell          *psSpell)
{
    ET9STATUS status;

    ET9_CP_Spell sSpellInt;

    ET9Assert(pTrace && ET9CPIsTraceActive(pTrace->pCLing));

    if (pTrace->fPrefixOnly) {
        status = _ET9_CP_Trace_GetPrefix(pTrace, wIndex, &sSpellInt);
    }
    else {
        status = _ET9_CP_Trace_GetAWWord(pTrace, wIndex, &sSpellInt, NULL);
    }
    if (ET9STATUS_NONE == status) {
        ET9_CP_ToExternalSpellInfo(pTrace->pCLing, &sSpellInt, psSpell);
    }
    return status;
}

ET9U8 ET9FARCALL ET9_CP_Trace_GetPrefixCount(const ET9_CP_Trace * pTrace)
{
    ET9U16 w;
    ET9CPSpell spell;

    ET9Assert(pTrace && ET9CPIsTraceActive(pTrace->pCLing));

    w = 0; 
    while (!ET9_CP_Trace_GetPrefix(pTrace, w, &spell))
    {
        w++;
    }
        
    return (ET9U8)w;
}

ET9STATUS ET9FARCALL ET9_CP_Trace_SetActivePrefix(ET9_CP_Trace * pTrace,
                                                  ET9U8         bPrefixIndex)
{
    ET9Assert(pTrace && ET9CPIsTraceActive(pTrace->pCLing));

    /* caller should have verified this */
    ET9Assert(bPrefixIndex < ET9_CP_Trace_GetPrefixCount(pTrace));

    pTrace->bActiveSpell = bPrefixIndex;

    ET9_CP_ClearPhraseBuf(ET9_CP_GetMainPhraseBuf(pTrace->pCLing));

    _ET9_CP_Trace_GetDefaultPhraseSpell(pTrace->pCLing, &pTrace->pCLing->CommonInfo.sActiveSpell);

    return ET9STATUS_NONE;
}

ET9STATUS ET9FARCALL ET9_CP_Trace_ClearActivePrefix(ET9_CP_Trace * pTrace)
{
    ET9Assert(pTrace && ET9CPIsTraceActive(pTrace->pCLing));

    pTrace->bActiveSpell = 0xFF;

    ET9_CP_ClearPhraseBuf(ET9_CP_GetMainPhraseBuf(pTrace->pCLing));

    _ET9_CP_Trace_GetDefaultPhraseSpell(pTrace->pCLing, &pTrace->pCLing->CommonInfo.sActiveSpell);

    return ET9STATUS_NONE;
}

ET9STATUS ET9FARCALL ET9_CP_Trace_GetActivePrefixIndex(ET9_CP_Trace * pTrace,
                                                       ET9U8            *pbPrefixIndex)
{
    ET9Assert(pTrace && ET9CPIsTraceActive(pTrace->pCLing));

    if (0xff == pTrace->bActiveSpell) {
        return ET9STATUS_EMPTY;
    }

    *pbPrefixIndex = pTrace->bActiveSpell;

    return ET9STATUS_NONE;
}

ET9U16 ET9FARCALL ET9_CP_Trace_ScorePhrase(ET9_CP_SpellData * psSpellData,
                                           ET9_CP_CommonInfo * pCommon,
                                           ET9SYMB * psPhrase,
                                           ET9U8 bPhraseLen,
                                           ET9U16 wFreq,
                                           ET9BOOL fIsFromContext,
                                           ET9BOOL fIsFromUdb)
{
    ET9_CP_TraceSpellData * psTraceSpellData = &psSpellData->u.sTrace;

    /* which kinds of completion were used */
    ET9BOOL fInitialExpansion, fSyllableCompletion, fPhraseCompletion;
    ET9FLOAT fLogTapFreq;

    /* map UDB freqs to LDB scale (approximation) */
    if (fIsFromUdb) {
        wFreq = (ET9U16)(wFreq >> 3);
        wFreq = (ET9U16)(wFreq + 0x100);
    }

    if (fIsFromContext) {
        wFreq = (ET9U16)(wFreq + 0x40);
    }

    ET9_CP_MatchType(pCommon, psPhrase, bPhraseLen,
        psTraceSpellData->fEndsWithInitial,
        &fInitialExpansion,
        &fSyllableCompletion,
        &fPhraseCompletion);

    fLogTapFreq = _ET9log_f(psTraceSpellData->psAWPrivWord->xTapFreq);

    wFreq = (ET9U16)(0x2000 + fLogTapFreq * 20 + wFreq);


    /* promote phrases not from completions */
    if (0 == psTraceSpellData->psAWPrivWord->Base.wWordCompLen) {
        wFreq = (ET9U16)(wFreq + 180);
    }
    else {
        ET9AWWordInfo * psWordBase = &psTraceSpellData->psAWPrivWord->Base;
        ET9SYMB sLastLetter = psWordBase->sWord[psWordBase->wWordLen - psWordBase->wWordCompLen - 1];

        /* promote partial pinyin style promotions BeiJ[ing], but not Xi[ao] */
        if (ET9CPIsUpperCaseSymbol(sLastLetter) || 'h' == sLastLetter) {
            wFreq = (ET9U16)(wFreq + 90);
        }
    }

    if (!fSyllableCompletion) {
        wFreq = (ET9U16)(wFreq + 0x3000);
    }

    if (!fInitialExpansion) {
        wFreq += 0x30;
    }

    return wFreq;
}

/**
 * Load the appropriate alpha LDB for this language/mode/charset 
 */
static ET9STATUS ET9LOCALCALL _ET9_CP_Trace_LdbSync(ET9CPLingInfo * pCPLing)
{
    ET9STATUS status;
    const ET9U16 wLdbNum = pCPLing->wLdbNum;
    const ET9CPMode eInputMode = pCPLing->eMode;
    ET9U16 wAWLdbNum;
    ET9U16 wSLID;
    ET9_CP_Trace * pTrace = &pCPLing->Trace;

    ET9Assert(ET9CPMODE_BPMF == eInputMode || ET9CPMODE_PINYIN == eInputMode);
    ET9Assert(!pTrace->fDBLoaded);

    /* determine secondary LID by BPMF mode */
    if (ET9CPMODE_BPMF == eInputMode) {
        wSLID = ET9SLIDBpmfTrace; /* BPMF */
    }
    else { /* assume PinYin for now */
        wSLID = ET9SLIDPinyinTrace; /* Pinyin */
    }

    wAWLdbNum = (ET9U16)( (ET9PLIDMASK & wLdbNum) | (ET9SLIDMASK & wSLID) );

    status = ET9AWLdbSetLanguage(pTrace->pALing, wAWLdbNum, 0);
    if (status != ET9STATUS_NONE) {
        if (ET9STATUS_LDB_ID_ERROR == status) {
            status = ET9STATUS_LDB_VERSION_ERROR;
        }
        return status;
    }

    /* validate version match */
    {
        ET9SYMB asChineseVersion[ET9MAXVERSIONSTR];
        ET9SYMB asAlphaVersion[ET9MAXVERSIONSTR];
        ET9U16 wAlphaVersionLen;
        ET9U16 wChineseVersionLen;


        status = ET9CPLdbGetVersion(pCPLing, asChineseVersion, ET9MAXVERSIONSTR, &wChineseVersionLen);
        ET9Assert(ET9STATUS_NONE == status);

        status = ET9AWLdbGetVersion(pTrace->pALing, asAlphaVersion, ET9MAXVERSIONSTR, &wAlphaVersionLen);
        ET9Assert(ET9STATUS_NONE == status);

        {
            ET9U16 wC;
            ET9U16 wA;
            ET9U16 wNeededMatchLen = 8; /* match ff.gg.hh -- major_ver minor_ver contents_deviation */
            
            for (wC = 0; wC < wChineseVersionLen; wC++) {
                if ('V' == asChineseVersion[wC]) {
                    wC++;
                    break;
                }
            }
            if (wChineseVersionLen - wC < wNeededMatchLen) {
                return ET9STATUS_LDB_VERSION_ERROR;
            }
            
            for (wA = 0; wA < wAlphaVersionLen; wA++) {
                if ('V' == asAlphaVersion[wA]) {
                    wA++;
                    break;
                }
            }
            if (wAlphaVersionLen - wA < wNeededMatchLen) {
                return ET9STATUS_LDB_VERSION_ERROR;
            }

            while (wNeededMatchLen > 0) {
                if (asChineseVersion[wC++] != asAlphaVersion[wA++]) {
                    return ET9STATUS_LDB_VERSION_ERROR;
                }
                wNeededMatchLen--;
            }
        }
    }

    return status;
}



/**
 * Load the appropriate alpha LDB for this language/mode/charset 
 */
ET9STATUS ET9FARCALL ET9_CP_Trace_LdbSync(ET9CPLingInfo * pCPLing)
{
    ET9STATUS status;

    pCPLing->Trace.fDBLoaded = 0;

    status = _ET9_CP_Trace_LdbSync(pCPLing);

    if (ET9STATUS_NONE == status) {
        pCPLing->Trace.fDBLoaded = 1;
    }

    return status;
}


/* UDB functions */

/*---------------------------------------------------------------------------*/
/** \internal
 * Udb spelling iterator to allow AWLingInfo to search Chinese UDB/AUDB words.
 * It should be passed into the AW MDB interface. See AW MDB reference for 
 * params/behavior explanation.
 *
 * @param pAWLing              I   - pointer to LingInfo struct owning MDB
 * @param eMdbRequestType      I   - MDB request type. Should be one of the values defined above
 * @param wReqWordLen          I   - word length
 * @param wMaxWordLen          I   - maximum word length
 * @param psBuildTxtBuf        O   - word to return
 * @param pwActWordLen         O   - length of the returned word
 * @param pdwWordListIdx       I/O - MDB word list index
 *
 * @return                     status
 */
ET9STATUS ET9FARCALL ET9_CP_Trace_MDBCallback(ET9AWLingInfo *pAWLing,
                                              ET9REQMDB eMdbRequestType,
                                              ET9U16 wReqWordLen,
                                              ET9U16 wMaxWordLen,
                                              ET9SYMB *psBuildTxtBuf,
                                              ET9U16 *pwActWordLen,
                                              ET9U32 *pdwWordListIdx)
{
    ET9STATUS status = ET9STATUS_NONE;
    ET9CPLingInfo * pCLing = (ET9CPLingInfo*)pAWLing->pLingCmnInfo->Base.pWordSymbInfo->Private.ppEditionsList[ET9EDITION_CP];
    
    ET9_CP_CHECK_LINGINFO(pCLing);

    while (ET9STATUS_NONE == status) {
        ET9CPPhrase phrase;
        ET9CPSpell spell;
        ET9U16 wSpellLen;
        ET9U16 wWordListIndex = (ET9U16)*pdwWordListIdx;

        status = ET9CPUdbGetPhrase(pCLing, ET9CPUdbPhraseType_ALL_MASK, wWordListIndex/2, &phrase, &spell);

        /* if we can get no (more) phrases, return the error to caller */
        if (ET9STATUS_NONE != status) {
            break;
        }

        /* every other index will have jianpin */
        if (wWordListIndex % 2) {
            wSpellLen = (ET9U16)spell.bLen;
        }
        else {
            ET9U8 b;

            wSpellLen = 1;

            /* build jianpin version */
            for (b = 1; b < spell.bLen; b++) {
                if (ET9_CP_IsUpperCase(spell.pSymbs[b])) {
                    spell.pSymbs[wSpellLen++] = spell.pSymbs[b];
                }
            }
        }

        /* increment index for caller */
        (*pdwWordListIdx)++;

        /* see if this spell passes the word length requirements given by caller */
        if (ET9MDBGETEXACTWORDS == eMdbRequestType && wReqWordLen == wSpellLen ||
            ET9MDBGETALLWORDS == eMdbRequestType && wSpellLen >= wReqWordLen && wSpellLen <= wMaxWordLen)
        {
            _ET9SymCopy(psBuildTxtBuf, spell.pSymbs, wSpellLen);
            *pwActWordLen = wSpellLen;

            /* return this word */
            break;
        }
    }

    return status;
}


/** Enable/disable Chinese Trace.
 *
 *  Enables Chinese Trace by passing in pAWLing, a correctly initialized ET9AWLingInfo data structure.<br>
 *  Disables Chinese Trace by passing in a NULL pointer for pET9AWLingInfo.<br>
 *  
 *  Once Trace is initialized, integration should make no further modifications to pET9AWLingInfo 
 *  such as changing settings.
 *
 * @param pET9CPLingInfo     pointer to chinese information structure.
 * @param pET9AWLingInfo     pointer to correctly initialized ET9AWLingInfo data structure, NULL to disable the feature.
 *
 * @return ET9STATUS_NONE               Succeeded
 * @return ET9STATUS_NO_INIT            pET9CPLingInfo is not properly initialized
 * @return ET9STATUS_BAD_PARAM          pET9AWLingInfo is not properly initialized
 * @return ET9STATUS_LDB_VERSION_ERROR  Proper Trace LDB for this Chinese LDB not found
 *
 */
ET9STATUS ET9FARCALL ET9CPTraceInit(ET9CPLingInfo *pET9CPLingInfo,
                                    ET9AWLingInfo *pET9AWLingInfo)
{
    ET9STATUS status;

    ET9_CP_CHECK_LINGINFO(pET9CPLingInfo);

    if (pET9AWLingInfo) {
        status = _ET9AWSys_BasicValidityCheck(pET9AWLingInfo);
        if (ET9STATUS_NONE != status || pET9AWLingInfo->ET9AWLdbReadData == NULL) {
            return ET9STATUS_BAD_PARAM;
        }
        /* keep completion un-demoted */
        pET9AWLingInfo->pLingCmnInfo->Private.dwDevStateBits |= ET9DEVSTATEINHBTRACECMPLDEMOTEMASK;

        status = ET9AWSysSetCompletionCount(pET9AWLingInfo, 12);  /* Default: 32 */
        ET9Assert(ET9STATUS_NONE == status);

        status = ET9ClearDownshiftDefault(pET9AWLingInfo);
        ET9Assert(ET9STATUS_NONE == status);

        status =  ET9AWRegisterMDB(pET9AWLingInfo, ET9_CP_Trace_MDBCallback);
        ET9Assert(ET9STATUS_NONE == status);

        pET9CPLingInfo->Trace.pALing = pET9AWLingInfo;
        pET9CPLingInfo->Trace.pCLing = pET9CPLingInfo;
        if (ET9CPIsModePinyin(pET9CPLingInfo) || ET9CPIsModeBpmf(pET9CPLingInfo) ) {
            /* Sync the trace LDB only in Pinyin/BPMF mode.
               If not in these modes, do not sync because when user SetInputMode to these modes, it will sync */
            status = ET9_CP_Trace_LdbSync(pET9CPLingInfo);
        }
    }
    else {
        if (pET9CPLingInfo->Trace.pALing) {
            pET9CPLingInfo->Trace.pALing->pLingCmnInfo->Private.dwDevStateBits &= ~ET9DEVSTATEINHBTRACECMPLDEMOTEMASK;
        }

        ET9_CP_Trace_Clear(&pET9CPLingInfo->Trace);
        status = ET9STATUS_NONE;
    }

    ET9_CP_INVALIDATE_BUILD_CLEAR_CACHE(pET9CPLingInfo);

    return status;
}

static void ET9LOCALCALL _ET9_CP_Trace_TraceSpellDataInit(ET9_CP_SpellData *  psSpellData,
                                            ET9AWPrivWordInfo * psWord,
                                            ET9U8               bSpellIndex)
{
    ET9_CP_TraceSpellData * psTraceSpellData = &psSpellData->u.sTrace;

    psSpellData->eSpellSource = ET9_CP_SpellSource_Trace;

    _ET9_CP_Trace_AWWordToSpell(&psSpellData->sSpell, &psWord->Base);

    psTraceSpellData->psAWPrivWord = psWord;
    psTraceSpellData->bSpellIndex = bSpellIndex;
    psTraceSpellData->wPhraseIndex = 0;
    psTraceSpellData->fEndsWithInitial = (ET9BOOL)ET9_CP_EndsWithInitial(psSpellData->sSpell.pbChars, psSpellData->sSpell.bLen);
}

/**
 *  Check for any non-LDB/MDB words in the alpha selection list (usually constructed words)
 */
static ET9INT ET9LOCALCALL _ET9_CP_Trace_HasConstructedWords(ET9AWLingInfo * pALing) {

    ET9STATUS status = ET9STATUS_NONE;
    ET9AWWordInfo * psWord;
    ET9U8 b;

    for (b = 0; ; b++) {
        status = ET9AWSelLstGetWord(pALing, &psWord, b);
        if (ET9STATUS_NONE != status) {
            break;
        }

        if (!(ET9AWORDSOURCE_LDB == (ET9AWORDSOURCE)psWord->bWordSource ||
              ET9AWORDSOURCE_MDB == (ET9AWORDSOURCE)psWord->bWordSource)) {
            /* found a non-LDB/MDB word, probably constructed by some alpha core rule */
            return 1;
        }
    }
    return 0;
}

static ET9BOOL ET9LOCALCALL _ET9_CP_Trace_HasToneDelim(ET9WordSymbInfo * pWSI)
{
    ET9UINT n, nNumSymbs = pWSI->bNumSymbs;
    for (n = 0; n < nNumSymbs; n++) {
        if ( ET9_CP_SymbIsToneOrDelim(&pWSI->SymbsInfo[n]) )
            return 1;
    }

    return 0;
}


ET9STATUS ET9FARCALL ET9_CP_Trace_BuildSelectionList(ET9CPLingInfo * pCLing) {
    ET9STATUS status;
    ET9AWLingInfo * pALing;
    ET9U8 bNumSpells, bUnused;
    ET9_CP_Trace * pTrace;
    ET9UINT nTapIndex;

    if (!ET9CPIsTraceActive(pCLing)) {
        return ET9STATUS_TRACE_NOT_AVAILABLE;
    }

    if ( _ET9_CP_Trace_HasToneDelim(pCLing->Base.pWordSymbInfo) ) {
        return ET9STATUS_INVALID_INPUT;
    }

    pTrace = &pCLing->Trace;
    pTrace->bActiveSpell = 0xFF;

    /**
     *  nTapIndex is index of first tap after trace. It is also the number of trace symbols.
     *  If nTapIndex is non-zero, convert these trace symbols to tap version and get a new WSI.
     *  Then for caller, it can use SBI to build selection list just like tap.
     */
    nTapIndex = _ET9_CP_Trace_HasTapAfterTrace(pCLing->Base.pWordSymbInfo);
    if (nTapIndex) {
        ET9CPSpell spell;
        ET9_CP_Spell * pActiveSpell = &pCLing->CommonInfo.sActiveSpell;
        ET9U8 b;

        if (!ET9_CP_HasActiveSpell(pCLing)) {
            return ET9STATUS_INVALID_INPUT;
        }

        /* Convert the active spell getting from trace to tap format */
        for (b = 0; b < pActiveSpell->bLen; b++) {
            spell.pSymbs[b] = ET9_CP_InternalPhoneticToExternal(pActiveSpell->pbChars[b]);
        }
        spell.bLen = pActiveSpell->bLen;
        status = ET9_CP_Trace_ReplaceByExplicitSymb(pCLing->Base.pWordSymbInfo, nTapIndex, spell.pSymbs, spell.bLen);
        if (ET9STATUS_NONE == status) {
            return ET9STATUS_NEED_SELLIST_BUILD;
        }
        else {
            return ET9STATUS_INVALID_INPUT;
        }
    }
    else  {
        _ET9ClearShiftInfo(pCLing->Base.pWordSymbInfo);
    }

    ET9_CS_ClearPrefixBuf(&pCLing->SBI);
    ET9_CP_ClearPhraseBuf(&pCLing->CommonInfo.sStdPhraseBuf.sPhraseBuf);
    ET9_CP_SelectionHistInit(&pCLing->SelHistory); /* don't support partial selection and trace */

    pALing = pTrace->pALing;

    status = ET9AWSelLstBuild(pALing, &bNumSpells, &bUnused);
    if (ET9STATUS_NONE == status && _ET9_CP_Trace_HasConstructedWords(pALing)) {
        status = ET9STATUS_INVALID_INPUT;
    }
    if (ET9STATUS_NONE == status) {
        pTrace->bNumSpells = bNumSpells;
        /* use best phrase's spelling */
        _ET9_CP_Trace_GetDefaultPhraseSpell(pCLing, &pCLing->CommonInfo.sActiveSpell);
        _ET9_CP_Trace_GetDefaultPhraseSpell(pCLing, &pTrace->sDefaultSpell);
    }
    else {
        pTrace->bNumSpells = 0;
    }

    return status;
}

ET9STATUS ET9FARCALL ET9_CP_Trace_FillPhraseBuffer(ET9CPLingInfo *pET9CPLingInfo)
{
    ET9STATUS status;
    ET9_CP_Trace * pTrace = &pET9CPLingInfo->Trace;
    ET9AWLingInfo * pALing = pTrace->pALing;
    ET9_CP_PhraseBuf * pPhraseBuf = ET9_CP_GetMainPhraseBuf(pTrace->pCLing);

    /* get a word to handle AWLing validation */
    {
        ET9AWWordInfo * psWord;
        status = ET9AWSelLstGetWord(pALing, &psWord, 0);
    }

    /* spell search succeeded, find phrases for each spell */
    if (ET9STATUS_NONE == status) {
        ET9U8 b;
        ET9AWPrivWordInfo * psWord;

        for (b = 0; b < pTrace->bNumSpells; b++) {
            ET9_CP_Spell sPrefix;

            psWord = &pALing->pLingCmnInfo->Private.pWordList[pALing->pLingCmnInfo->Private.bWordList[b]];

            if (0xFF != pTrace->bActiveSpell) {
                _ET9_CP_Trace_GetPrefix(pTrace, (ET9U16)pTrace->bActiveSpell, &sPrefix);
                _ET9_CP_Trace_TrimSpellTo1stSyl(&sPrefix);
            }

            if (!pTrace->fPrefixOnly && (0xFF == pTrace->bActiveSpell || b == pTrace->bActiveSpell) ||
                pTrace->fPrefixOnly)
            {
                ET9_CP_SpellData * pSpellData = &pTrace->pCLing->CommonInfo.SpellData;
                ET9_CP_Spell * pSpell = &pSpellData->sSpell;

                _ET9_CP_Trace_TraceSpellDataInit(pSpellData, psWord, b);
                if (pTrace->fPrefixOnly && 0xFF != pTrace->bActiveSpell && !_ET9_CP_Trace_PrefixMatchesSpell(&sPrefix, pSpell)) {
                    continue;
                }
                ET9_CP_GetPhraseFromSpell(pTrace->pCLing,
                                          pSpell,
                                          pPhraseBuf,
                                          /*bNeedPartialSyl*/1,
                                          /*bNeedPartialPhrase*/0,
                                          /*bSearchContext*/1);
                ET9_CP_SpellDataClear(pSpellData);
            }
        }
    }

    pPhraseBuf->wLastTotal = pPhraseBuf->wTotal;
    return ET9STATUS_NONE;
}

ET9STATUS ET9FARCALL ET9_CP_Trace_SelectPhrase(ET9_CP_Trace * pTrace,
                                               ET9U16         wPhraseIndex,
                                               ET9CPSpell     *pSpell)
{
    ET9STATUS status = ET9STATUS_NONE;
    ET9CPPhrase phraseUnicode, phraseEncoded;
    ET9_CP_PhraseBuf *pMainPhraseBuf;
    ET9_CP_Spell sInternalSpell;
    ET9CPLingInfo * pLing = pTrace->pCLing;

    ET9Assert(pTrace && ET9CPIsTraceActive(pTrace->pCLing));

    pMainPhraseBuf = ET9_CP_GetMainPhraseBuf(pLing);

    ET9_CP_GetPhraseFromBuf(pMainPhraseBuf, &phraseEncoded, &sInternalSpell, wPhraseIndex + 1);

    if (pSpell) {
        /* get the complete spelling from PIDs of sPhrase */
        ET9Assert(0 < sInternalSpell.bLen);
        ET9_CP_ToExternalSpellInfo(pLing, &sInternalSpell, pSpell);
    } 

    _ET9SymCopy(phraseUnicode.pSymbs, (ET9SYMB *)phraseEncoded.pSymbs, phraseEncoded.bLen);
    phraseUnicode.bLen = phraseEncoded.bLen;
    ET9_CP_ConvertPhraseToUnicode(pLing, &phraseUnicode, ET9_CP_IDEncode_PID);

    ET9Assert(ET9_CP_PhraseIsAllChn(pLing, phraseEncoded.pSymbs, phraseEncoded.bLen));
    status = ET9_CP_SelectionHistAdd(&pLing->SelHistory, phraseUnicode.pSymbs, phraseEncoded.pSymbs, phraseEncoded.bLen, pLing->Base.pWordSymbInfo->bNumSymbs);

    return status;
}

/**
 *  Replace first nReplaceLen keys in WordSymbInfo with new tapping input. 
 *  This function is usually used for replacing trace input to tap version.
 */
ET9STATUS ET9FARCALL ET9_CP_Trace_ReplaceByExplicitSymb(ET9WordSymbInfo * pWSI, ET9UINT nReplaceLen, const ET9SYMB * psExplicitSymb, ET9UINT nExplicitSymbLen)
{
    ET9UINT n;
    ET9STATUS status;
    
    /**
     *  Check whether has exception when call ET9AddExplicitSymb
     */   
    if (pWSI->bNumSymbs - nReplaceLen + nExplicitSymbLen > ET9MAXWORDSIZE) {       /* Out of WSI size */
        return ET9STATUS_FULL;
    }
    for (n = 0; n < nExplicitSymbLen; n++) {      /* Verify explicit Symbols */
        if (pWSI->Private.bPreventWhiteSpaceInput && _ET9_IsWhiteSpace(ET9CPBpmfSymbolToLower(psExplicitSymb[n]))) {
            return ET9STATUS_INVALID_TEXT;
        }
    }
    
    status = ET9DeleteSymbs(pWSI, 0, (ET9U8)nReplaceLen);
    ET9Assert(ET9STATUS_NONE == status);

    for (n = 0; n < nExplicitSymbLen; n++) {
        status = ET9AddExplicitSymb(pWSI, ET9CPBpmfSymbolToLower(psExplicitSymb[n]), ET9NOSHIFT, ET9_NO_ACTIVE_INDEX);
        ET9Assert(ET9STATUS_NONE == status);
    }

    status = ET9MoveSymbs(pWSI, (ET9U8)(pWSI->bNumSymbs - nExplicitSymbLen), 0, (ET9U8)nExplicitSymbLen);
    ET9Assert(ET9STATUS_NONE == status || ET9STATUS_EMPTY == status);

    return ET9STATUS_NONE;
}

#endif  /* ET9_ALPHABETIC_MODULE */

/* ----------------------------------< eof >--------------------------------- */
