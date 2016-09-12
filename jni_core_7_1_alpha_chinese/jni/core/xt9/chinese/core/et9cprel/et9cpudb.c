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
;**     FileName: et9cpudb.c                                                  **
;**                                                                           **
;**  Description: Phrase Text Input UDB (augment of LDB) module.              **
;**               Conforming to the development version of Chinese XT9.       **
;**                                                                           **
;*******************************************************************************
;******************************************************************************/

#include "et9cpapi.h"
#include "et9cppbuf.h"
#include "et9cpinit.h"
#include "et9cpkey.h"
#include "et9cpsys.h"
#include "et9cpspel.h"
#include "et9cpldb.h"
#include "et9cprdb.h"
#include "et9cpmisc.h"
#include "et9cpsldb.h"
#include "et9cpname.h"
#include "et9cpwdls.h"
#include "et9misc.h"

/*---------------------------------------------------------------------------
 *
 *   Function: ET9_CP_UIDSylToPID
 *
 *   Synopsis: Finds the PID that matches the given syllable and Unicode
 *
 *     Input:  pET9CPLingInfo = pointer to Chinese XT9 LingInfo structure.
 *             wUnicode       = unicode to be looked up for alternative index.
 *             pbSyl = pointer to the syllable.
 *             bSylLen = length of the syllable.
 *
 *       Output: none.
 *
 *     Return: matched PID, or ET9_CP_NOMATCH if not found.
 *
 *---------------------------------------------------------------------------*/
static ET9U16 ET9LOCALCALL ET9_CP_UIDSylToPID(ET9CPLingInfo *pET9CPLingInfo,
                                              ET9SYMB wUnicode,
                                              ET9U8 *pbSyl,
                                              ET9U8 bSylLen)
{
    ET9U16 wPID;
    ET9U16 wPidRangeStart, wPidRangeEnd;
    ET9U8 i, j;

    ET9Assert(pbSyl && bSylLen);
    if (ET9_CP_IsPinyinLetter(pbSyl[0]) && ET9_CP_IsLdbDualPhonetic(pET9CPLingInfo) ) {
        ET9U16 *pwSylPidTable;
        ET9U16 pwExactSylIndex[3];
        ET9U8 pbExactSylIndexRange[3];

        pwSylPidTable = pET9CPLingInfo->Private.PPrivate.awSylPidTable;
        if (ET9_CP_GetSyllableIndex(pET9CPLingInfo, pbSyl, bSylLen, 0,
                                   pwExactSylIndex, pbExactSylIndexRange, 0, 0)) {
            for (i = 0; pwExactSylIndex[i] != 0xFFFF; i++) {
                wPidRangeStart = pwSylPidTable[pwExactSylIndex[i]];
                wPidRangeEnd = pwSylPidTable[pwExactSylIndex[i] + pbExactSylIndexRange[i]];
                /* find the PID that matches this Unicode and is within this syl's PID range */
                for (j = 0; j < ET9_CP_MAX_ALT_SYLLABLE; j++) {
                    wPID = ET9_CP_UnicodeToPID(pET9CPLingInfo, wUnicode, j); /* binary search, and results are cached */
                    if (ET9_CP_NOMATCH == wPID || (wPidRangeStart <= wPID && wPID < wPidRangeEnd) ) {
                        /* no PID for this Unicode with this syllable OR found a match */
                        return wPID;
                    }
                }
            }
        }
    }
    else if (  (ET9_CP_IsPinyinLetter(pbSyl[0]) && ET9_CP_LdbHasPinyin(pET9CPLingInfo) )
            || (ET9_CP_IsBpmfLetter(pbSyl[0]) && ET9_CP_LdbHasBpmf(pET9CPLingInfo) ) ) {

        if (ET9_CP_SyllableToPidRange(pET9CPLingInfo, pbSyl, bSylLen, 0,
                                      &wPidRangeStart, &wPidRangeEnd, 0)) {
            /* find the PID that matches this Unicode and is within this syl's PID range */
            for (j = 0; j < ET9_CP_MAX_ALT_SYLLABLE; j++) {
                wPID = ET9_CP_UnicodeToPID(pET9CPLingInfo, wUnicode, j);
                if (ET9_CP_NOMATCH == wPID || (wPidRangeStart <= wPID && wPID < wPidRangeEnd) ) {
                    /* no PID for this Unicode with this syllable OR found a match */
                    return wPID;
                }
            }
        }
    }
    return ET9_CP_NOMATCH; /* no match */
} /* END ET9_CP_UIDSylToPID() */


/* start of new local functions  */
/* This function is for Phonetic only */
static ET9STATUS ET9LOCALCALL ET9_CP_ChkExtSpell(
                                ET9CPPhrase *psPhrase,
                                ET9U8 *pbSpell,
                                ET9U16 bSpellLen)
{
    ET9U8 *pb, *pbSpellEnd, bSylLen, bMaxSylLen, bSylCount;

    if (0 == bSpellLen) {
        return ET9STATUS_NONE; /* no given spelling, return success */
    }
    /* has spelling */
    ET9Assert(pbSpell);

    /* setup max syllable length */
    if (ET9_CP_IsPinyinUpperCase(pbSpell[0])) {
        bMaxSylLen = ET9_CP_MAX_PINYIN_SYL_SIZE;
    }
    else if (ET9_CP_IsBpmfUpperCase(pbSpell[0])) {
        bMaxSylLen = ET9_CP_MAX_BPMF_SYL_SIZE;
    }
    else {
        return ET9STATUS_BAD_PARAM; /* spelling not started with upper case */
    }

    /* parse the spelling for any error */
    bSylCount = 0;
    pbSpellEnd = pbSpell + bSpellLen;
    bSylLen = 0; /* to avoid warning */
    for (pb = pbSpell; pb < pbSpellEnd; pb++) {
        if (ET9_CP_IsUpperCase(*pb)) {
            bSylCount++;
            bSylLen = 1;
        }
        else if (ET9_CP_IsLowerCase(*pb)) {
            bSylLen++;
            if (bSylLen > bMaxSylLen) {
                return ET9STATUS_BAD_PARAM; /* syllable longer than max length */
            }
        }
        else if (ET9CPSYLLABLEDELIMITER == *pb || ET9CPSymToCPTone(*pb)) {
            if (pb < pbSpellEnd - 1) { /* not the last symb */
                ET9U8 bNext = *(pb + 1);
                if (!ET9_CP_IsUpperCase(bNext)) {
                    /* tone/delimiter not followed by upper case */
                    return ET9STATUS_BAD_PARAM;
                }
            }
        }
        else {
            return ET9STATUS_BAD_PARAM; /* unknown symbol */
        }
    }
    if (bSylCount != psPhrase->bLen) {
        return ET9STATUS_BAD_PARAM; /* syllable count doesn't match phrase length */
    }
    return ET9STATUS_NONE;
} /* END ET9_CP_ChkExtSpell() */

static ET9STATUS ET9LOCALCALL ET9_CP_UIDSpellToPIDSID(
                                    ET9CPLingInfo *pET9CPLingInfo,
                                    ET9CPPhrase *pUIDPhrase,
                                    ET9CPPhrase *pPIDPhrase,
                                    ET9CPPhrase *pSIDPhrase,
                                    ET9U8 *pbSpell,
                                    ET9U8 bSpellLen)
{
    ET9UINT fGetSID;
#ifndef ET9CP_DISABLE_STROKE
    ET9U16 wSID;
#endif
    ET9U16 wUID, wPID, wFirstComponentPID;
    ET9U8 bPhraseIndex = 0;

    wFirstComponentPID = pET9CPLingInfo->CommonInfo.w1stComponentPID;
    if (ET9_CP_LdbHasStroke(pET9CPLingInfo)) {
        fGetSID = 1;
    }
    else {
        fGetSID = 0;
    }

    if (bSpellLen) { /* use given spelling to find PID */
        ET9U8 *pbSpellEnd, *pbSyl, *pb, bSylLen;
        pbSpellEnd = pbSpell + bSpellLen;
        pbSyl = pbSpell; /* first syllable */
        bSylLen = 1;
        for (pb = pbSpell + 1; pb <= pbSpellEnd; pb++) {
            if ((pb == pbSpellEnd) || ET9_CP_IsUpperCase(*pb)) { /* to include the last syl but won't access out of bound */
                /* Got a syllable.  Find the matching PID */
                ET9Assert(bPhraseIndex < pUIDPhrase->bLen);
                wUID = pUIDPhrase->pSymbs[bPhraseIndex];
                wPID = ET9_CP_UIDSylToPID(pET9CPLingInfo, wUID, pbSyl, bSylLen);
                if (ET9_CP_NOMATCH == wPID) {
                    return ET9STATUS_BAD_PARAM;
                }
#ifndef ET9CP_DISABLE_STROKE
                if (fGetSID) {
                    /* get most common SID */
                    if (0 == ET9_CP_LookupID(pET9CPLingInfo, &wSID, wPID, 1, ET9_CP_Lookup_PIDToSID) ) {
                        return ET9STATUS_BAD_PARAM;
                    }
                    pSIDPhrase->pSymbs[bPhraseIndex] = wSID;
                }
#endif
                pPIDPhrase->pSymbs[bPhraseIndex] = wPID;
                bPhraseIndex++;
                pbSyl = pb; /* start another syllable */
                bSylLen = 1;
            }
            else if (ET9_CP_IsLowerCase(*pb)) {
                bSylLen++;
            } /* ignore tone/delimiter */
        }
    } /* END use given spelling */

    /* when spelling is not given, get most common PID */
    for (; bPhraseIndex < pUIDPhrase->bLen; bPhraseIndex++) {
        wUID = pUIDPhrase->pSymbs[bPhraseIndex];
        wPID = ET9_CP_UnicodeToPID(pET9CPLingInfo, wUID, 0);
        if (ET9_CP_NOMATCH == wPID || wPID >= wFirstComponentPID) {
            /* unknown UID or component, which has PID but no syllable */
            return ET9STATUS_BAD_PARAM;
        }
#ifndef ET9CP_DISABLE_STROKE
        if (fGetSID) {

            if (0 == ET9_CP_LookupID(pET9CPLingInfo, &wSID, wPID, 1, ET9_CP_Lookup_PIDToSID) ) {
                return ET9STATUS_BAD_PARAM;
            }
            pSIDPhrase->pSymbs[bPhraseIndex] = wSID;
        }
#endif
        pPIDPhrase->pSymbs[bPhraseIndex] = wPID;
    }
    pPIDPhrase->bLen = pUIDPhrase->bLen;
    if (fGetSID) {
        pSIDPhrase->bLen = pUIDPhrase->bLen;
    }
    else {
        pSIDPhrase->bLen = 0;
    }
    return ET9STATUS_NONE;
} /* END ET9_CP_UIDSpellToPIDSID */

/** Adds a user-defined phrase to the RUDB.
 *
 * This function adds a Unicode phrase into the RUDB as a user-defined phrase.
 * An optional phonetic spelling may be provided to specify which spelling each Unicode character should take.
 * If omitted, the most common spelling for each character will be used.
 * The spelling should be in the same format as spellings returned from ET9CPSelectPhrase(),
 * presumably concatenated together from the returned spellings of previous calls to ET9CPSelectPhrase().
 * The spell type (pinyin/BPMF) must match the current input mode.<br>
 * Phrases will only be added in standard stroke sequence.
 *
 * @param pET9CPLingInfo     pointer to chinese information structure.
 * @param psPhrase           Phrase to be added (in Unicode).
 * @param psSpell            (optional)Spelling of phrase to be added.
 * @param bSpellLen          length of psSpell.
 *
 * @return ET9STATUS_NONE         Succeeded
 * @return ET9STATUS_NO_INIT      pET9CPLingInfo is not properly initialized
 * @return ET9STATUS_NO_RUDB      No Udb attached to pET9CPLingInfo .
 * @return ET9STATUS_BAD_PARAM    psPhrase is NULL. Or its length is 0. Or it is too long. Or bSpellLen is too big. Or specify a spelling in non-phonetic mode.
 */
ET9STATUS ET9FARCALL ET9CPUdbAddPhrase(ET9CPLingInfo *pET9CPLingInfo,
                                       ET9CPPhrase *psPhrase,
                                       ET9SYMB *psSpell,
                                       ET9U8 bSpellLen)
{
    ET9STATUS        status;
    ET9CPPhrase  sPhrasePID, sPhraseSID;
    ET9U8  pUdbSpell[ET9CPMAXSPELLSIZE]; /* bSpellLen is always less than or equal to ET9CPMAXSPELLSIZE  */
    ET9U8 *pbSpell = NULL;

    /* Validate inputs */
    ET9_CP_CHECK_LINGINFO(pET9CPLingInfo);

    if (!pET9CPLingInfo->pUdb) {
        return ET9STATUS_NO_RUDB; /* no UDB */
    }
    ET9Assert(ET9CPIsModePinyin(pET9CPLingInfo) ||
              ET9CPIsModeStroke(pET9CPLingInfo) ||
              ET9CPIsModeBpmf(pET9CPLingInfo) ||
              ET9CPIsModeCangJie(pET9CPLingInfo) ||
              ET9CPIsModeQuickCangJie(pET9CPLingInfo));

    if (0 == psPhrase || 0 == psPhrase->bLen || psPhrase->bLen > ET9CPMAXUDBPHRASESIZE) {
        return ET9STATUS_BAD_PARAM; /* no phrase OR empty phrase OR phrase too long */
    }
    if (psSpell == 0 && bSpellLen != 0) {
        return ET9STATUS_BAD_PARAM; /* inconsistency between spell string & spell length */
    }
    /* validate the given spelling */
    status = ET9STATUS_NONE;
    if (ET9CPIsModeStroke(pET9CPLingInfo) || ET9CPIsModeCangJie(pET9CPLingInfo) || ET9CPIsModeQuickCangJie(pET9CPLingInfo)) { /* Stroke or CangJie mode */
        if (bSpellLen > 0) {
            return ET9STATUS_BAD_PARAM;
        }
    }
    else { /* Pinyin or Bpmf mode */
        ET9Assert(ET9CPIsModePinyin(pET9CPLingInfo) || ET9CPIsModeBpmf(pET9CPLingInfo));
        if (psSpell) { /* convert external spell to internal format */
            ET9U8 b;
            if (bSpellLen > ET9CPMAXSPELLSIZE) {
                return ET9STATUS_BAD_PARAM;  /* Spell too long */
            }
            for (b = 0; b < bSpellLen; b++) {
                pUdbSpell[b] = ET9_CP_ExternalSpellCodeToInternal(pET9CPLingInfo, psSpell[b]);
            }
            pbSpell = pUdbSpell;
        }
        /* validate the given spelling */
        status = ET9_CP_ChkExtSpell(psPhrase, pbSpell, bSpellLen);
    }

    if (ET9STATUS_NONE == status) { /* convert UID to PID & SID entries according to the given spelling */
        status = ET9_CP_UIDSpellToPIDSID(pET9CPLingInfo, psPhrase,
                                        &sPhrasePID, &sPhraseSID,
                                        pbSpell, bSpellLen);
        if (ET9STATUS_NONE == status) { /* store PID & SID entries into UDB */
            /* save PID entry to UDB if phonetic modules exist */
            if (ET9_CP_LdbHasBpmf(pET9CPLingInfo)) {
                ET9_CP_UdbUsePhrase(pET9CPLingInfo, &sPhrasePID, ET9_CP_IDEncode_BID, 0, 0);
            }
            else if (ET9_CP_LdbHasPinyin(pET9CPLingInfo)) {
                ET9_CP_UdbUsePhrase(pET9CPLingInfo, &sPhrasePID, ET9_CP_IDEncode_PID, 0, 0);
            }
#ifndef ET9CP_DISABLE_STROKE
            /* save SID entry to UDB if stroke module exists */
            if (ET9_CP_LdbHasStroke(pET9CPLingInfo)) {
                ET9_CP_UdbUsePhrase(pET9CPLingInfo, &sPhraseSID, ET9_CP_IDEncode_SID, 0, 0);
            }
#endif
        }
    }
    return status;
} /* END ET9CPUdbAddPhrase() */

/*------------------------------------------------------------------------
 *  Function    : ET9_CP_WriteUdbData
 *  Description : Handle ALL writes to UDB memory here.  Assume data move
 *                towards higher address during in-place copy.
 *
 *  Input       : pFieldInfo = Pointer to field information structure.
 *                pTo        = where in memory change begins
 *                pFrom      = location of data to copy
 *                nSize      = how much data
 *
 *  Return      : none
 *-----------------------------------------------------------------------*/
void ET9FARCALL ET9_CP_WriteUdbData(ET9CPLingInfo *pLingInfo,
                                    void ET9FARDATA *pTo,
                                    const void ET9FARDATA *pFrom,
                                    ET9UINT nSize)
{
    /* Have OEM write the data, if requested */
    if (pLingInfo->pfUdbWrite != NULL) {
        ET9STATUS status;
        status = pLingInfo->pfUdbWrite(pLingInfo->pPublicExtension, (unsigned char ET9FARDATA *) pTo, (const unsigned char ET9FARDATA *)pFrom, nSize);
        ET9Assert(ET9STATUS_NONE == status);
    }
    else { /* Write data directly if no request for external write by OEM */

        ET9U8 ET9FARDATA* pbTo = (ET9U8 ET9FARDATA*)pTo + nSize - 1;
        ET9U8 ET9FARDATA* pbFrom = (ET9U8 ET9FARDATA*)pFrom + nSize - 1;

        for (; nSize--; ) {
            *pbTo-- = *pbFrom--;  /* ignore invalid insure warnings */
        }
    }
} /* ET9_CP_WriteUdbData() */

static ET9U8 ET9LOCALCALL ET9_CP_ValueToHexAscii(ET9U32 dwValue,
                                                 ET9SYMB *psAsciiBuf,
                                                 ET9U8 bBufMaxSize)
{
    ET9U8 abReversedByteBuf[4]; /* size of ET9U32 */
    ET9U8 bHexAsciiSize, i;
    for (i = 0; i < 4 && dwValue > 0; i++) {
        abReversedByteBuf[i] = (ET9U8)dwValue;
        dwValue >>= 8;
    }
    bHexAsciiSize = 0;
    for (; i > 0 && bHexAsciiSize + ET9_CP_U8_ASCII_STR_SIZE <= bBufMaxSize; --i) {
        _ET9BinaryToHex(abReversedByteBuf[i - 1], psAsciiBuf + bHexAsciiSize);
        bHexAsciiSize += ET9_CP_U8_ASCII_STR_SIZE;
    }
    return bHexAsciiSize;
}

/* Return : 0xFFFFFFFF to indicate error */
static ET9U32 ET9LOCALCALL ET9_CP_HexAsciiToValue(ET9SYMB *psAsciiBuf,
                                                  ET9U8 bBufSize)
{
    ET9U32 dwValue;
    ET9U8 i;

    dwValue = 0;
    if (bBufSize > ET9_CP_U32_ASCII_STR_SIZE) {
        return ~((ET9U32)0); /* ASCII str too long */
    }
    for (i = 0; i < bBufSize; i++) {
        ET9SYMB sDigit;
        ET9U8 bDigit;
        sDigit = psAsciiBuf[i];
        if ('0' <= sDigit && sDigit <= '9') {
            bDigit = (ET9U8)(sDigit - '0');
        }
        else if ('A' <= sDigit && sDigit <= 'F') {
            bDigit = (ET9U8)(sDigit - 'A' + 0xA);
        }
        else {
            return ~((ET9U32)0); /* unknown symbol, return error */
        }
        dwValue = (dwValue << 4) | bDigit;
    }
    return dwValue;
}

/*------------------------------------------------------------------------
 *  Description : Serialize the TUDB header into a UTF-8 encoded byte buffer .
 *
 *  Input       : pLing = pointer to Chinese Ling Info.
 *                pbUtf8Buffer = (OUT) destination buffer for writing the serialized TUDB header
 *
 *  Serialized Header format:
 *      Version number (to identify the TUDB format and contents)
 *          Hex value converted to ASCII string that ends with an entry delimiter.
 *      LDB Version string (for troubleshooting)
 *          ASCII string that ends with an entry delimiter.
 *      Core Version string (for troubleshooting)
 *          ASCII string that ends with an entry delimiter.
 *
 *  Return      : size of the serialized header.
 *-----------------------------------------------------------------------*/
static ET9U32 ET9LOCALCALL ET9_CP_Tudb_SerializeHeader(ET9CPLingInfo *pLing,
                                                       ET9U8 *pbUtf8Buffer)
{
    ET9U32 dwExportSize;
    ET9UINT i;
    ET9STATUS status;

    dwExportSize = 0;
    /* serialize Tudb version number */
    {
        ET9SYMB asTudbVerAsciiStr[ET9_CP_U8_ASCII_STR_SIZE];
        ET9U8 bTudbVerAsciiStrSize;

        bTudbVerAsciiStrSize = ET9_CP_ValueToHexAscii(ET9_CP_TUDB_CURRENT_VERSION, asTudbVerAsciiStr, ET9_CP_U8_ASCII_STR_SIZE);
        for (i = 0; i < bTudbVerAsciiStrSize; i++) {
            dwExportSize += _ET9SymbToUtf8(asTudbVerAsciiStr[i], pbUtf8Buffer + dwExportSize);
        }
        dwExportSize += _ET9SymbToUtf8((ET9SYMB)ET9_CP_TUDB_ENTRY_DELIMITER, pbUtf8Buffer + dwExportSize); /* ends with entry delimiter */
    }

    /* serialize Ldb version string */
    {
        ET9SYMB asLdbVerStr[ET9MAXVERSIONSTR];
        ET9U16 wLdbVerStrSize;
        status = ET9CPLdbGetVersion(pLing, asLdbVerStr, ET9MAXVERSIONSTR, &wLdbVerStrSize);
        ET9Assert(ET9STATUS_NONE == status);
        for (i = 0; i < wLdbVerStrSize; i++) {
            dwExportSize += _ET9SymbToUtf8(asLdbVerStr[i], pbUtf8Buffer + dwExportSize);
        }
        dwExportSize += _ET9SymbToUtf8((ET9SYMB)ET9_CP_TUDB_ENTRY_DELIMITER, pbUtf8Buffer + dwExportSize); /* ends with entry delimiter */
    }
    /* serialize Core version string */
    {
        ET9SYMB asCoreVerStr[ET9MAXVERSIONSTR];
        ET9U16 wCoreVerStrSize;
        status = ET9GetCodeVersion(asCoreVerStr, ET9MAXVERSIONSTR, &wCoreVerStrSize);
        ET9Assert(ET9STATUS_NONE == status);
        for (i = 0; i < wCoreVerStrSize; i++) {
            dwExportSize += _ET9SymbToUtf8(asCoreVerStr[i], pbUtf8Buffer + dwExportSize);
        }
        dwExportSize += _ET9SymbToUtf8((ET9SYMB)ET9_CP_TUDB_ENTRY_DELIMITER, pbUtf8Buffer + dwExportSize); /* ends with entry delimiter */
    }
    return dwExportSize;
}


/*------------------------------------------------------------------------
 *  Description : Serialize a UDB entry into a UTF-8 encoded byte buffer .
 *
 *  Input       : pLing = pointer to Chinese Ling Info.
 *                pObj  = pointer to a UDB obj
 *                pbUtf8Buffer = (OUT) destination buffer for writing the serialized obj
 *
 *  Serialized Entry format:
 *      Type (single letter)
 *          * "U" for User-defined phrases - same as the current UDB type
 *          * "A" for Auto-created phrases - include the current AUDB and RDB types
 *      Mode (single letter)
 *          * "P" for phonetic phrases
 *          * "S" for stroke phrases
 *      Frequency
 *      Unicode string of the phrase (ends with a field delimiter)
 *      Spelling info
 *          * For Pinyin/BPMF mode, export native spelling (i.e. Pinyin only LDB exports Pinyin, otherwise exports BPMF) (terminated by a field delimiter)
 *             * For BPMF, use Unicode BPMF letters (no upper case) and add '.' as syllable delimiter
 *             * For Pinyin, to be consistent with BPMF, use all lower case and add '.' as syllable delimiter. 
 *          * For stroke type, no content.
 *
 *  Return      : size of the serialized entry.
 *-----------------------------------------------------------------------*/
static ET9U32 ET9LOCALCALL ET9_CP_Tudb_SerializeEntry(ET9CPLingInfo *pLing,
                                                      const ET9_CP_RUdbObj *pObj,
                                                      ET9U8 *pbUtf8Buffer)
{
    ET9U32 dwEntryExportSize;
    ET9_CP_IDEncode eEncode;
    ET9CPPhrase sPhrase, sUniPhrase;
    ET9SYMB sSymb;
    ET9U8 i;

    dwEntryExportSize = 0;
    /* type */
    {
        sSymb = (ET9SYMB)( (ET9_CP_UDBPHONETIC == pObj->eType || ET9_CP_UDBSTROKE == pObj->eType) ? ET9_CP_TUDB_USER_TYPE : ET9_CP_TUDB_AUTO_TYPE);
        dwEntryExportSize += _ET9SymbToUtf8(sSymb, pbUtf8Buffer + dwEntryExportSize);
    }
    /* mode */
    {
        if (ET9_CP_RDBPHONETIC == pObj->eType || ET9_CP_AUDBPHONETIC == pObj->eType || ET9_CP_UDBPHONETIC == pObj->eType) {
            if (ET9_CP_LdbHasBpmf(pLing) ) {
                sSymb = (ET9SYMB)ET9_CP_TUDB_BPMF_MODE; /* native spelling is BPMF */
                eEncode = ET9_CP_IDEncode_BID;
            }
            else {
                sSymb = (ET9SYMB)ET9_CP_TUDB_PINYIN_MODE; /* native spelling is Pinyin */
                eEncode = ET9_CP_IDEncode_PID;
            }
        }
        else {
            ET9Assert(ET9_CP_RDBSTROKE == pObj->eType || ET9_CP_AUDBSTROKE == pObj->eType || ET9_CP_UDBSTROKE == pObj->eType);
            sSymb = (ET9SYMB)ET9_CP_TUDB_STROKE_MODE; /* stroke mode */
            eEncode = ET9_CP_IDEncode_SID;
        }
        dwEntryExportSize += _ET9SymbToUtf8(sSymb, pbUtf8Buffer + dwEntryExportSize);
    }
    /* frequency */
    {
        ET9SYMB asFreqAsciiStr[ET9_CP_U16_ASCII_STR_SIZE];
        ET9U8 bFreqAsciiStrSize;

        bFreqAsciiStrSize = ET9_CP_ValueToHexAscii(pObj->wFreq, asFreqAsciiStr, ET9_CP_U16_ASCII_STR_SIZE);
        for (i = 0; i < bFreqAsciiStrSize; i++) {
            dwEntryExportSize += _ET9SymbToUtf8(asFreqAsciiStr[i], pbUtf8Buffer + dwEntryExportSize);
        }
        dwEntryExportSize += _ET9SymbToUtf8((ET9SYMB)ET9_CP_TUDB_FIELD_DELIMITER, pbUtf8Buffer + dwEntryExportSize); /* field delimiter */
    }
    /* Unicode string of the phrase */
    {
        /* copy to encoded phrase */
        sPhrase.bLen = ET9_CP_EntrySizeToWordSize(pObj->wEntrySize);
        sUniPhrase.bLen = sPhrase.bLen;
        for (i = 0; i < sPhrase.bLen; i++) {
            sPhrase.pSymbs[i] = pObj->pwID[i];
            sUniPhrase.pSymbs[i] = pObj->pwID[i];
        }
        /* convert phrase to Unicode */
        ET9_CP_ConvertPhraseToUnicode(pLing, &sUniPhrase, eEncode);
        /* export Unicode phrase (ends with a field delimiter) */
        for (i = 0; i < sUniPhrase.bLen; i++) {
            dwEntryExportSize += _ET9SymbToUtf8(sUniPhrase.pSymbs[i], pbUtf8Buffer + dwEntryExportSize);
        }
        dwEntryExportSize += _ET9SymbToUtf8((ET9SYMB)ET9_CP_TUDB_FIELD_DELIMITER, pbUtf8Buffer + dwEntryExportSize); /* field delimiter */
    }
    /* export spelling */
    {
        if (ET9_CP_IDEncode_PID == eEncode || ET9_CP_IDEncode_BID == eEncode) { /* Pinyin or BPMF mode entry */
            ET9_CP_Spell sInternalSpell;
            ET9_CP_PidBidToNativeSpelling(pLing, sPhrase.pSymbs, sPhrase.bLen, &sInternalSpell);
            /* phonetic spelling (ends with field delimiter) */
            for (i = 0; i < sInternalSpell.bLen; i++) {
                /* Replace upper case BPMF/Pinyin letters with Unicode/lower case Pinyin
                   and add syllable delimiter */
                if (ET9_CP_IsUpperCase(sInternalSpell.pbChars[i]) ) {
                    sInternalSpell.pbChars[i] = ET9_CP_ToLowerCase(sInternalSpell.pbChars[i]);
                    if (i > 0) {
                        dwEntryExportSize += _ET9SymbToUtf8((ET9SYMB)ET9_CP_TUDB_SYL_DELIMITER, pbUtf8Buffer + dwEntryExportSize);
                    }
                }
                sSymb = ET9_CP_InternalSpellCodeToExternal(pLing, sInternalSpell.pbChars[i]);
                dwEntryExportSize += _ET9SymbToUtf8(sSymb, pbUtf8Buffer + dwEntryExportSize);
            }
        } /* no content for Stroke mode entry */
        dwEntryExportSize += _ET9SymbToUtf8((ET9SYMB)ET9_CP_TUDB_FIELD_DELIMITER, pbUtf8Buffer + dwEntryExportSize); /* field delimiter */
    }
    /* ends with entry delimiter */
    dwEntryExportSize += _ET9SymbToUtf8((ET9SYMB)ET9_CP_TUDB_ENTRY_DELIMITER, pbUtf8Buffer + dwEntryExportSize);
    ET9Assert(dwEntryExportSize <= ET9_CP_TUDB_MAX_ENTRY_SIZE);
    return dwEntryExportSize;
}

/** Compute the size of the buffer needed to export the active UDB.
 *
 *  This function is useful for migrating a Chinese XT9 UDB from one device to another.
 *
 * @param pET9CPLingInfo    pointer to chinese information structure.
 * @param pdwSize           output the buffer size needed to export the active UDB.
 *
 * @return ET9STATUS_NONE               Succeeded
 * @return ET9STATUS_NO_INIT            pET9CPLingInfo is not properly initialized
 * @return ET9STATUS_NO_RUDB            No Udb attached to pET9CPLingInfo
 * @return ET9STATUS_BAD_PARAM          some parameters are invalid
 */
ET9STATUS ET9FARCALL ET9CPUdbGetExportSize(ET9CPLingInfo    *pET9CPLingInfo,
                                           ET9U32           *pdwSize)
{
    ET9CPUdbInfo ET9FARDATA *pUdb;
    ET9UINT i;

    /* validate input */
    ET9_CP_CHECK_LINGINFO(pET9CPLingInfo);
    if (NULL == pdwSize) {
        return ET9STATUS_BAD_PARAM;
    }
    if (NULL == pET9CPLingInfo->pUdb) {
        return ET9STATUS_NO_RUDB; /* no UDB */
    }
    pUdb = pET9CPLingInfo->pUdb;
    *pdwSize = 0;

    { /* Find TUDB header size */
        ET9U8 abUtf8Buffer[ET9_CP_TUDB_MAX_HEADER_SIZE];
        *pdwSize = *pdwSize + ET9_CP_Tudb_SerializeHeader(pET9CPLingInfo, abUtf8Buffer);
    }

    for (i = 0; i < ET9_CP_UDBZONEMAX; i++) { /* for each zone */

        ET9U8 ET9FARDATA *pbEntry = ET9_CP_UdbOffsetToPtr(pUdb, pUdb->wZoneOffset[i]); /* point to start of this zone */
        ET9UINT nWord;
        ET9_CP_RUdbObj sObj;

        for (nWord = 0; nWord < pUdb->wZoneWordCount[i]; ) { /* for each word in the current zone */

            ET9U8 abUtf8Buffer[ET9_CP_TUDB_MAX_ENTRY_SIZE];

            ET9Assert( pbEntry < ET9_CP_UdbEnd(pUdb) );
            ET9_CP_GetEntryInfo(pUdb, pbEntry, &sObj, ET9_CP_GET_TYPE | ET9_CP_GET_ENTRY_SIZE);
            if (ET9_CP_FREE == sObj.eType) {
                pbEntry = ET9_CP_MoveUdbPR(pUdb, pbEntry, sObj.wEntrySize); /* move to next entry */
                continue; /* skip free entries */
            }
            nWord++;
            ET9_CP_GetEntryInfo(pUdb, pbEntry, &sObj, ET9_CP_GET_FREQ | ET9_CP_GET_ID);
            if (ET9_CP_RDBSTROKE == sObj.eType && ET9_CP_Is_Comp_Sid(pET9CPLingInfo, sObj.pwID[0]) ) {
                pbEntry = ET9_CP_MoveUdbPR(pUdb, pbEntry, sObj.wEntrySize); /* move to next entry */
                continue; /* do not export a component */
            }
            *pdwSize = *pdwSize + ET9_CP_Tudb_SerializeEntry(pET9CPLingInfo, &sObj, abUtf8Buffer);

            pbEntry = ET9_CP_MoveUdbPR(pUdb, pbEntry, sObj.wEntrySize); /* move to next entry */
        }
    }
    *pdwSize += ET9_CP_TUDB_TYPE_SIZE; /* add the size for End-of-list mark */
    return ET9STATUS_NONE;
}

/** Writes the entries of the active UDB into a byte buffer provided by integration layer.
 *  It allows resuming from previous export by skipping the entries that was previously exported.
 *
 *  This function is useful for migrating a Chinese XT9 UDB from one device to another.
 *
 * @param pET9CPLingInfo    pointer to chinese information structure.
 * @param pbDst             the output byte buffer.
 * @param dwSize            size of pbDst.
 * @param nSkipCount        number of entries to skip from the beginning.
 * @param pnSuccessCount    output the number of entries that are successfully exported.
 *
 * @return ET9STATUS_NONE               Succeeded
 * @return ET9STATUS_NO_INIT            pET9CPLingInfo is not properly initialized
 * @return ET9STATUS_NO_RUDB            No Udb attached to pET9CPLingInfo
 * @return ET9STATUS_BAD_PARAM          some parameters are invalid
 * @return ET9STATUS_BUFFER_TOO_SMALL   size of pbDst is too small
 */
ET9STATUS ET9FARCALL ET9CPUdbExport(ET9CPLingInfo       *pET9CPLingInfo,
                                    ET9U8 ET9FARDATA    *pbDst,
                                    ET9U32              dwSize,
                                    ET9UINT             nSkipCount,
                                    ET9UINT             *pnSuccessCount)
{
    ET9CPUdbInfo ET9FARDATA *pUdb;
    ET9UINT i;
    ET9U32 dwDstOffset;

    /* validate input */
    ET9_CP_CHECK_LINGINFO(pET9CPLingInfo);
    if (NULL == pbDst || 0 == dwSize || NULL == pnSuccessCount) {
        return ET9STATUS_BAD_PARAM;
    }
    if (!pET9CPLingInfo->pUdb) {
        return ET9STATUS_NO_RUDB; /* no UDB */
    }
    pUdb = pET9CPLingInfo->pUdb;
    dwDstOffset = 0;
    *pnSuccessCount = 0;

    { /* export TUDB header */
        ET9U32 dwExportHeaderSize;
        ET9U8 abUtf8Buffer[ET9_CP_TUDB_MAX_HEADER_SIZE];
        dwExportHeaderSize = ET9_CP_Tudb_SerializeHeader(pET9CPLingInfo, abUtf8Buffer);
        if (dwExportHeaderSize + ET9_CP_TUDB_TYPE_SIZE > dwSize) {
            /* if the header+End-of-list mark is greater than buffer size, the buffer is too small to hold any entries. */
            return ET9STATUS_BUFFER_TOO_SMALL;
        }
        /* write the serialized TUDB header to the export buffer */
        ET9_CP_WriteUdbData(pET9CPLingInfo, (void ET9FARDATA *)(pbDst + dwDstOffset), (const void ET9FARDATA *)abUtf8Buffer, dwExportHeaderSize);
        dwDstOffset += dwExportHeaderSize;
    }

    for (i = 0; i < ET9_CP_UDBZONEMAX; i++) { /* for each zone */

        ET9U8 ET9FARDATA *pbEntry = ET9_CP_UdbOffsetToPtr(pUdb, pUdb->wZoneOffset[i]); /* point to start of this zone */
        ET9UINT nWord;
        ET9_CP_RUdbObj sObj;

        for (nWord = 0; nWord < pUdb->wZoneWordCount[i]; ) { /* for each word in the current zone */

            ET9U32 dwExportEntrySize;
            ET9U8 abUtf8Buffer[ET9_CP_TUDB_MAX_ENTRY_SIZE];

            ET9Assert( pbEntry < ET9_CP_UdbEnd(pUdb) );
            ET9_CP_GetEntryInfo(pUdb, pbEntry, &sObj, ET9_CP_GET_TYPE | ET9_CP_GET_ENTRY_SIZE);
            if (ET9_CP_FREE == sObj.eType) {
                pbEntry = ET9_CP_MoveUdbPR(pUdb, pbEntry, sObj.wEntrySize); /* move to next entry */
                continue; /* skip free entries */
            }
            nWord++;
            ET9_CP_GetEntryInfo(pUdb, pbEntry, &sObj, ET9_CP_GET_FREQ | ET9_CP_GET_ID);
            if (ET9_CP_RDBSTROKE == sObj.eType && ET9_CP_Is_Comp_Sid(pET9CPLingInfo, sObj.pwID[0]) ) {
                pbEntry = ET9_CP_MoveUdbPR(pUdb, pbEntry, sObj.wEntrySize); /* move to next entry */
                continue; /* do not export a component */
            }
            if (nSkipCount > 0) {
                nSkipCount--; /* decrement skip count only when skipping */
                pbEntry = ET9_CP_MoveUdbPR(pUdb, pbEntry, sObj.wEntrySize); /* move to next entry */
                continue;
            }
            dwExportEntrySize = ET9_CP_Tudb_SerializeEntry(pET9CPLingInfo, &sObj, abUtf8Buffer);
            if (dwDstOffset + dwExportEntrySize + ET9_CP_TUDB_TYPE_SIZE > dwSize) {
                /* Not enough room to export this entry, stop here, write End-of-list mark. */
                char cEOL = ET9_CP_TUDB_EOL_MARK;
                ET9Assert(dwDstOffset + ET9_CP_TUDB_TYPE_SIZE <= dwSize);
                ET9_CP_WriteUdbData(pET9CPLingInfo, (void ET9FARDATA *)(pbDst + dwDstOffset), (const void ET9FARDATA *)&cEOL, ET9_CP_TUDB_TYPE_SIZE);
                return ET9STATUS_NONE;
            }
            /* write the serialized entry to the export buffer */
            ET9_CP_WriteUdbData(pET9CPLingInfo, (void ET9FARDATA *)(pbDst + dwDstOffset), (const void ET9FARDATA *)abUtf8Buffer, dwExportEntrySize);
            (*pnSuccessCount)++;
            dwDstOffset += dwExportEntrySize;

            pbEntry = ET9_CP_MoveUdbPR(pUdb, pbEntry, sObj.wEntrySize); /* move to next entry */
        }
    }
    { /* write End-of-list mark */
        char cEOL = ET9_CP_TUDB_EOL_MARK;
        ET9_CP_WriteUdbData(pET9CPLingInfo, (void ET9FARDATA *)(pbDst + dwDstOffset), (const void ET9FARDATA *)&cEOL, ET9_CP_TUDB_TYPE_SIZE);
    }
    return ET9STATUS_NONE;
}

static ET9U32 ET9LOCALCALL ET9_CP_TUDB_ReadHeader(const ET9U8 ET9FARDATA *pbSrc,
                                                  const ET9U8 ET9FARDATA *pbEnd,
                                                  ET9U8 *pbVersion)
{
    ET9U32 dwValue, dwTotalReadSize; /* total read size */
    ET9SYMB asHexAscii[ET9_CP_U8_ASCII_STR_SIZE]; /* buffer to store hex ASCII str of the freq */
    ET9U8 bHexAsciiSize;

    dwTotalReadSize = 0;
    bHexAsciiSize = 0;
    *pbVersion = 0;
    for (; ;) {
        ET9SYMB sDigit;
        ET9U8 bCharReadSize; /* single char read size */

        if (pbSrc >= pbEnd) {
            return 0; /* beyond buffer end */
        }
        bCharReadSize = _ET9Utf8ToSymb(pbSrc, pbEnd, &sDigit);
        pbSrc += bCharReadSize;
        dwTotalReadSize += bCharReadSize;
        if (0 == bCharReadSize || pbSrc > pbEnd) {
            return 0; /* invalid content OR beyond buffer end */
        }
        else if (ET9_CP_TUDB_ENTRY_DELIMITER == sDigit) {
            break; /* reached entry delimiter */
        }
        if (bHexAsciiSize < ET9_CP_U8_ASCII_STR_SIZE) {
            asHexAscii[bHexAsciiSize++] = sDigit;
        }
        else {
            return 0; /* Hex ASCII str too long */
        }
    }
    dwValue = ET9_CP_HexAsciiToValue(asHexAscii, bHexAsciiSize);
    if (dwValue & 0xFFFFFF00) {
        return 0; /* invalid value */
    }
    else {
        *pbVersion = (ET9U8)dwValue;
    }
    /* read Ldb version string */
    for (; ;) {
        ET9SYMB sSymb;
        ET9U8 bCharReadSize; /* single char read size */

        if (pbSrc >= pbEnd) {
            return 0; /* beyond buffer end */
        }
        bCharReadSize = _ET9Utf8ToSymb(pbSrc, pbEnd, &sSymb);
        pbSrc += bCharReadSize;
        dwTotalReadSize += bCharReadSize;
        if (0 == bCharReadSize || pbSrc > pbEnd) {
            return 0; /* invalid content OR beyond buffer end */
        }
        else if (ET9_CP_TUDB_ENTRY_DELIMITER == sSymb) {
            break; /* reached entry delimiter */
        }
    }
    /* read Core version string */
    for (; ;) {
        ET9SYMB sSymb;
        ET9U8 bCharReadSize; /* single char read size */

        if (pbSrc >= pbEnd) {
            return 0; /* beyond buffer end */
        }
        bCharReadSize = _ET9Utf8ToSymb(pbSrc, pbEnd, &sSymb);
        pbSrc += bCharReadSize;
        dwTotalReadSize += bCharReadSize;
        if (0 == bCharReadSize || pbSrc > pbEnd) {
            return 0; /* invalid content OR beyond buffer end */
        }
        else if (ET9_CP_TUDB_ENTRY_DELIMITER == sSymb) {
            break; /* reached entry delimiter */
        }
    }
    return dwTotalReadSize;
}

static ET9U32 ET9LOCALCALL ET9_CP_TUDB_ReadFreq(const ET9U8 ET9FARDATA *pbSrc,
                                                const ET9U8 ET9FARDATA *pbEnd,
                                                ET9U16 *pwFreq)
{
    ET9U32 dwValue, dwTotalReadSize; /* total read size */
    ET9SYMB asHexAscii[ET9_CP_U16_ASCII_STR_SIZE]; /* buffer to store hex ASCII str */
    ET9U8 bHexAsciiSize;

    dwTotalReadSize = 0;
    bHexAsciiSize = 0;
    *pwFreq = 0;
    for (; ;) {
        ET9SYMB sDigit;
        ET9U8 bCharReadSize; /* single char read size */

        if (pbSrc >= pbEnd) {
            return 0; /* beyond buffer end */
        }
        bCharReadSize = _ET9Utf8ToSymb(pbSrc, pbEnd, &sDigit);
        pbSrc += bCharReadSize;
        dwTotalReadSize += bCharReadSize;
        if (0 == bCharReadSize || pbSrc > pbEnd) {
            return 0; /* invalid content OR beyond buffer end */
        }
        else if (ET9_CP_TUDB_FIELD_DELIMITER == sDigit) {
            break; /* reached field delimiter */
        }
        if (bHexAsciiSize < ET9_CP_U16_ASCII_STR_SIZE) {
            asHexAscii[bHexAsciiSize++] = sDigit;
        }
        else {
            return 0; /* Hex ASCII str too long */
        }
    }
    dwValue = ET9_CP_HexAsciiToValue(asHexAscii, bHexAsciiSize);
    if (dwValue & 0xFFFF0000) {
        return 0; /* invalid value */
    }
    else {
        *pwFreq = (ET9U16)dwValue;
    }
    return dwTotalReadSize;
}

/*  Reads Unicode phrase from Src buffer.
    Return : Total read size. 0 indicates error
*/
static ET9U32 ET9LOCALCALL ET9_CP_TUDB_ReadPhrase(const ET9U8 ET9FARDATA *pbSrc,
                                                  const ET9U8 ET9FARDATA *pbEnd,
                                                  ET9CPPhrase *pUniPhrase)
{
    ET9U32 dwTotalReadSize; /* total read size */

    dwTotalReadSize = 0;
    for (pUniPhrase->bLen = 0; ;) {
        ET9SYMB sUni;
        ET9U8 bCharReadSize; /* single char read size */

        if (pbSrc >= pbEnd) {
            return 0; /* beyond buffer end */
        }
        bCharReadSize = _ET9Utf8ToSymb(pbSrc, pbEnd, &sUni);
        pbSrc += bCharReadSize;
        dwTotalReadSize += bCharReadSize;
        if (0 == bCharReadSize || pbSrc > pbEnd) {
            return 0; /* invalid content OR beyond buffer end */
        }
        else if (ET9_CP_TUDB_FIELD_DELIMITER == sUni) {
            break; /* reached field delimiter */
        }
        if (pUniPhrase->bLen < ET9CPMAXUDBPHRASESIZE) {
            pUniPhrase->pSymbs[pUniPhrase->bLen++] = sUni;
        }
        else {
            return 0; /* Unicode string too long */
        }
    }
    return dwTotalReadSize;
}

/*  Reads spelling info from Src buffer and output a phrase in native ID (PID/BID/SID)
    Return: Total read size.  0 indicates error.
*/
static ET9U32 ET9LOCALCALL ET9_CP_TUDB_ReadSpell(const ET9U8 ET9FARDATA *pbSrc,
                                                 const ET9U8 ET9FARDATA *pbEnd,
                                                 ET9CPSpell             *pSpell)
{
    ET9U32 dwTotalReadSize;
    ET9BOOL bToUpper;

    dwTotalReadSize = 0;
    pSpell->bLen = 0;
    bToUpper = 1; /* 1st letter must change to upper case */
    for ( ; ; ) {
        ET9SYMB sSymb;
        ET9U8 bCharReadSize; /* single char read size */

        if (pbSrc >= pbEnd) {
            return 0; /* beyond buffer end */
        }
        bCharReadSize = _ET9Utf8ToSymb(pbSrc, pbEnd, &sSymb);
        pbSrc += bCharReadSize;
        dwTotalReadSize += bCharReadSize;
        if (0 == bCharReadSize || pbSrc > pbEnd) {
            return 0; /* invalid content OR beyond buffer end */
        }
        else if (ET9_CP_TUDB_FIELD_DELIMITER == sSymb) {
            break; /* reached field delimiter */
        }
        if (bToUpper) {
            sSymb = ET9CPPinyinSymbolToUpper(sSymb); /* BPMF letters will not be changed */
            sSymb = ET9CPBpmfSymbolToUpper(sSymb); /* Pinyin letters will not be changed */
            bToUpper = 0;
        }
        else if ( (ET9SYMB)ET9_CP_TUDB_SYL_DELIMITER == sSymb) {
            bToUpper = 1;
            continue; /* skip TUDB BPMF syl delimiter */
        }
        if (pSpell->bLen < ET9CPMAXSPELLSIZE) {
            pSpell->pSymbs[pSpell->bLen++] = sSymb;
        }
        else {
            return 0; /* spell too long */
        }
    }
    return dwTotalReadSize;
}

static ET9BOOL ET9LOCALCALL ET9_CP_TUDB_ImportEntry(ET9CPLingInfo    *pLing,
                                                    char             cType,
                                                    char             cMode,
                                                    ET9U16           wFreq,
                                                    ET9CPPhrase      *pUniPhrase,
                                                    ET9CPSpell       *pSpell)
{
    ET9_CP_IDEncode eEncode;
    ET9CPPhrase sEncodedPhrase;
    ET9U16 awDefaultPID[ET9CPMAXUDBPHRASESIZE];
    ET9U8 i;

    /* validate type */
    if ( !(ET9_CP_TUDB_USER_TYPE == cType || ET9_CP_TUDB_AUTO_TYPE == cType) ) {
        return 0; /* unknown type */
    }
    /* validate mode */
    if ( !(ET9_CP_TUDB_PINYIN_MODE == cMode || ET9_CP_TUDB_BPMF_MODE == cMode || ET9_CP_TUDB_STROKE_MODE == cMode) ) {
        return 0; /* unknown mode */
    }
    /* validate phrase */
    if (0 == pUniPhrase->bLen) {
        return 0; /* invalid phrase length */
    }
    for (i = 0; i < pUniPhrase->bLen; i++) {
        awDefaultPID[i] = ET9_CP_UnicodeToPID(pLing, pUniPhrase->pSymbs[i], 0);
        if (ET9_CP_NOMATCH == awDefaultPID[i]) {
            return 0; /* this Unicode is not supported */
        }
    }
    /* validate spell and generate encoded phrase */
    if (ET9_CP_TUDB_PINYIN_MODE == cMode || ET9_CP_TUDB_BPMF_MODE == cMode) {
        ET9CPMode eSpellMode; /* Track consistency among spelling symbs. Must all of the same mode (currently accept only Pinyin/BPMF) */
        ET9_CP_Syl sSyl;
        ET9U8 bMaxSylSize;

        if (0 == pSpell->bLen) {
            return 0; /* invalid spell length */
        }
        /* Set mode and max syl size base on 1st symb */
        if (ET9CPIsBpmfSymbol(pSpell->pSymbs[0]) ) {
            eSpellMode = ET9CPMODE_BPMF;
            bMaxSylSize = ET9_CP_MAX_BPMF_SYL_SIZE;
        }
        else if (ET9CPIsPinyinSymbol(pSpell->pSymbs[0]) ) {
            eSpellMode = ET9CPMODE_PINYIN;
            bMaxSylSize = ET9_CP_MAX_PINYIN_SYL_SIZE;
        }
        else {
            return 0; /* unknown symb */
        }

        /* phrase encoding is decided by the native encoding of the active Ldb */
        if (ET9_CP_LdbHasBpmf(pLing) ) {
            eEncode = ET9_CP_IDEncode_BID;
        }
        else {
            eEncode = ET9_CP_IDEncode_PID;
        }

        sSyl.bSize = 0;
        sEncodedPhrase.bLen = 0;
        for (i = 0; i < pSpell->bLen; i++) {
            ET9SYMB sSymb;
            sSymb = pSpell->pSymbs[i];
            /* validate all symbs in the spell are consistent */
            if (ET9CPIsBpmfSymbol(sSymb) ) {
                if (ET9CPMODE_BPMF != eSpellMode) {
                    return 0; /* mode of this symb is inconsistent with the 1st symb */
                }
            }
            else if (ET9CPIsPinyinSymbol(sSymb) ) {
                if (ET9CPMODE_PINYIN != eSpellMode) {
                    return 0; /* mode of this symb is inconsistent with the 1st symb */
                }
            }
            else {
                return 0; /* unknown symb */
            }
            if (sSyl.bSize > 0 && ET9CPIsUpperCaseSymbol(sSymb) ) {
                if (sEncodedPhrase.bLen >= pUniPhrase->bLen) {
                    return 0; /* encoded phrase length > Unicode phrase length */
                }
                /* before starting a new syllable, use this syl to find PID, and then clear this syl. */
                sEncodedPhrase.pSymbs[sEncodedPhrase.bLen] = ET9_CP_UIDSylToPID(pLing, pUniPhrase->pSymbs[sEncodedPhrase.bLen], sSyl.aChars, sSyl.bSize);
                if (ET9_CP_NOMATCH == sEncodedPhrase.pSymbs[sEncodedPhrase.bLen]) {
                    return 0;
                }
                sEncodedPhrase.bLen++;
                sSyl.bSize = 0;
            }
            else if (sSyl.bSize >= bMaxSylSize) { /* validate syl size before appending to the syl */
                return 0; /* syl size too big */
            }
            sSyl.aChars[sSyl.bSize++] = ET9_CP_ExternalSpellCodeToInternal(pLing, sSymb);
        }
        /* last syllable */
        ET9Assert(sSyl.bSize > 0);
        if (sEncodedPhrase.bLen + 1 != pUniPhrase->bLen) {
            return 0; /* encoded phrase length != Unicode phrase length */
        }
        sEncodedPhrase.pSymbs[sEncodedPhrase.bLen] = ET9_CP_UIDSylToPID(pLing, pUniPhrase->pSymbs[sEncodedPhrase.bLen], sSyl.aChars, sSyl.bSize);
        if (ET9_CP_NOMATCH == sEncodedPhrase.pSymbs[sEncodedPhrase.bLen]) {
            return 0;
        }
        sEncodedPhrase.bLen++;
    }
    else { /* for Stroke */
        ET9Assert(ET9_CP_TUDB_STROKE_MODE == cMode);
        if (pSpell->bLen > 0) {
            return 0; /* invalid spell length */
        }
        eEncode = ET9_CP_IDEncode_SID;
        for (i = 0; i < pUniPhrase->bLen; i++) {
            ET9U8 bAltSIDCount;
            ET9Assert(ET9_CP_NOMATCH != awDefaultPID[i]); /* already validated earlier */
            bAltSIDCount = ET9_CP_LookupID(pLing, &(sEncodedPhrase.pSymbs[i]), awDefaultPID[i], 1, ET9_CP_Lookup_PIDToSID);
            if (0 == bAltSIDCount) { /* always use the standard SID */
                return 0; /* no SID for this PID */
            }
        }
        sEncodedPhrase.bLen = pUniPhrase->bLen;
    }

    /* add the phrase to UDB */
    ET9_CP_UdbUsePhrase(pLing, &sEncodedPhrase, eEncode, wFreq, (ET9BOOL)(ET9_CP_TUDB_AUTO_TYPE == cType) );
    return 1;
}

/* Import function for TUDB version 1 */
ET9STATUS ET9FARCALL ET9_CP_TudbImportV1(ET9CPLingInfo           *pLing,
                                         const ET9U8 ET9FARDATA  *pbSrc,
                                         const ET9U8 ET9FARDATA  *pbEnd,
                                         ET9UINT                 *pnAcceptCount,
                                         ET9UINT                 *pnRejectCount)
{
    ET9U32 dwReadSize;
    ET9BOOL bAbort;

    bAbort = 1; /* only change to normal stop when EOL is reached */
    while (pbSrc < pbEnd) {
        ET9CPPhrase sPhrase;
        ET9CPSpell sSpell; /* for phonetic spelling */
        ET9U16 wFreq;
        char cType, cMode;

        /* read type */
        cType = *pbSrc++; /* calling _ET9Utf8ToSymb() seems overkill */
        if (ET9_CP_TUDB_EOL_MARK == cType) {
            bAbort = 0;
            break; /* reached EOL, normal stop */
        }
        /* read mode */
        if (pbSrc >= pbEnd) {
            break; /* beyond buffer end */
        }
        cMode = *pbSrc++; /* calling _ET9Utf8ToSymb() seems overkill */

        /* read freq */
        dwReadSize = ET9_CP_TUDB_ReadFreq(pbSrc, pbEnd, &wFreq);
        if (0 == dwReadSize) {
            break; /* invalid buffer content */
        }
        pbSrc += dwReadSize;

        /* read Unicode string of the phrase */
        dwReadSize = ET9_CP_TUDB_ReadPhrase(pbSrc, pbEnd, &sPhrase);
        if (0 == dwReadSize) {
            break;
        }
        pbSrc += dwReadSize;

        /* read spelling for phonetic mode entry */
        dwReadSize = ET9_CP_TUDB_ReadSpell(pbSrc, pbEnd, &sSpell);
        if (0 == dwReadSize) {
            break;
        }
        pbSrc += dwReadSize;

        /* read entry delimiter */
        if (pbSrc >= pbEnd) {
            break; /* beyond buffer end */
        }
        if (ET9_CP_TUDB_ENTRY_DELIMITER != *pbSrc++) { /* calling _ET9Utf8ToSymb() seems overkill */
            break; /* missing entry delimiter */
        }

        if (ET9_CP_TUDB_ImportEntry(pLing, cType, cMode, wFreq, &sPhrase, &sSpell) ) {
            (*pnAcceptCount)++;
        }
        else {
            (*pnRejectCount)++;
        }
    }

    if (bAbort) {
        return ET9STATUS_ABORT; /* unexpected end of src buffer */
    }
    return ET9STATUS_NONE;
}

/** Reads the entries in a byte buffer provided by integration layer and add them into the active UDB.
 *  It rejects any entries that are not supported by the active Ldb.
 *
 *  This function is useful for migrating a Chinese XT9 UDB from one device to another.
 *
 * @param pET9CPLingInfo    pointer to chinese information structure.
 * @param pbSrc             the input byte buffer.
 * @param dwSize            size of pbSrc.
 * @param pnAcceptCount     output the number of entries that are accepted.
 * @param pnRejectCount     output the number of entries that are rejected.
 *
 * @return ET9STATUS_NONE               Succeeded
 * @return ET9STATUS_NO_INIT            pET9CPLingInfo is not properly initialized
 * @return ET9STATUS_NO_RUDB            No Udb attached to pET9CPLingInfo
 * @return ET9STATUS_BAD_PARAM          some parameters are invalid
 * @return ET9STATUS_ABORT              unexpected format or content in the input buffer, cannot continue to read.
 */
ET9STATUS ET9FARCALL ET9CPUdbImport(ET9CPLingInfo           *pET9CPLingInfo,
                                    const ET9U8 ET9FARDATA  *pbSrc,
                                    ET9U32                  dwSize,
                                    ET9UINT                 *pnAcceptCount,
                                    ET9UINT                 *pnRejectCount)
{
    const ET9U8 ET9FARDATA *pbEnd;
    ET9U32 dwReadSize;
    ET9U8 bVersion;

    /* validate input */
    ET9_CP_CHECK_LINGINFO(pET9CPLingInfo);
    if (NULL == pbSrc || 0 == dwSize || NULL == pnAcceptCount || NULL == pnRejectCount) {
        return ET9STATUS_BAD_PARAM;
    }
    if (!pET9CPLingInfo->pUdb) {
        return ET9STATUS_NO_RUDB; /* no UDB */
    }

    /* init locals */
    *pnAcceptCount = 0;
    *pnRejectCount = 0;
    pbEnd = pbSrc + dwSize;

    /* read header */
    dwReadSize = ET9_CP_TUDB_ReadHeader(pbSrc, pbEnd, &bVersion);
    if (0 == dwReadSize) {
        return ET9STATUS_ABORT; /* invalid buffer content */
    }
    pbSrc += dwReadSize;

    switch (bVersion) {
        case ET9_CP_TUDB_VERSION_1:
            return ET9_CP_TudbImportV1(pET9CPLingInfo, pbSrc, pbEnd, pnAcceptCount, pnRejectCount);
        default:
            return ET9STATUS_ABORT;
    }
}

/* ----------------------------------< eof >--------------------------------- */
