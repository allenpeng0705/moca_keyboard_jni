/*******************************************************************************
;*******************************************************************************
;**                                                                           **
;**                    COPYRIGHT 1998-2011 NUANCE COMMUNICATIONS              **
;**                                                                           **
;**               NUANCE COMMUNICATIONS PROPRIETARY INFORMATION               **
;**                                                                           **
;**     This software is supplied under the terms of a license agreement      **
;**     or non-disclosure agreement with Nuance Communications and may not    **
;**     be copied or disclosed except in accordance with the terms of that    **
;**     agreement.                                                            **
;**                                                                           **
;*******************************************************************************
;**                                                                           **
;**     FileName: et9acdb.c                                                   **
;**                                                                           **
;**  Description: Context data base access routines source file.              **
;**                                                                           **
;*******************************************************************************
;******************************************************************************/

/*! \addtogroup et9acdb Context database for alphabetic
* XT9 alphabetic context database features.
* @{
*/

#include "et9api.h"
#ifdef ET9_ALPHABETIC_MODULE
#include "et9asys.h"
#include "et9adb.h"
#include "et9acdb.h"
#include "et9amisc.h"
#include "et9sym.h"
#include "et9aasdb.h"
#include "et9alsasdb.h"

#define BROKEN_CONTEXT_DELIMITER_COUNT  2       /**< \internal ? */

/*---------------------------------------------------------------------------*/
/** \internal
 * Handle ALL writes to CDB memory here.
 *
 * @param pLingInfo     Pointer to ET9 information structure.
 * @param pTo           Where in memory change begins.
 * @param pFrom         Location of data to copy.
 * @param nSize         How much data.
 *
 * @return None
 */

static void ET9LOCALCALL __ET9AWWriteCDBData(ET9AWLingInfo *pLingInfo,
                                        void ET9FARDATA    *pTo,
                                  const void ET9FARDATA    *pFrom,
                                             ET9UINT        nSize)
{
    /* Adjust pointers to write in reverse order */
    ET9U8 ET9FARDATA *psTo = (ET9U8 ET9FARDATA *)pTo + (nSize - 1);
    ET9U8 ET9FARDATA *psFrom = (ET9U8 ET9FARDATA *)pFrom + (nSize - 1);

    ET9Assert(pLingInfo);
    ET9Assert(pTo);
    ET9Assert(pFrom);
    ET9Assert(nSize);

    /* Have OEM write the data, if requested */
    if (pLingInfo->pCDBWriteData != NULL) {
        pLingInfo->pCDBWriteData(pLingInfo, psTo, psFrom, nSize);
    }
    /* otherwise do it as if doing RAM transfer */
    else {

#ifdef ET9_DEBUG
        ET9AWCDBInfo ET9FARDATA *pCDB = pLingInfo->pLingCmnInfo->pCDBInfo;
#endif

        ET9Assert(pCDB != NULL);

        while (nSize--) {

            ET9Assert(psTo >= (ET9U8*)pCDB);
            ET9Assert(psTo <= (ET9U8*)(pCDB + (pCDB->wDataSize - 1)) || !pCDB->wDataSize);

            *psTo-- = *psFrom--;
        }
    }
}


/*---------------------------------------------------------------------------*/
/** \internal
 * Update ET9CDBInfo::wUpdateCounter and to signal changing of database.
 *
 * @param pLingInfo     Pointer to ET9 information structure.
 * @param bValue        0(reset), non-zero(update value).
 *
 * @return None
 */

static void ET9LOCALCALL __ET9AWCDBUpdateCounter(ET9AWLingInfo *pLingInfo,
                                                 ET9U8          bValue)
{
    ET9U16 wTemp = 0;

    ET9AWCDBInfo ET9FARDATA * const pCDB = pLingInfo->pLingCmnInfo->pCDBInfo;

    ET9Assert(pLingInfo);
    ET9Assert(pCDB != NULL);

    if (bValue) {
        wTemp = (ET9U16)(pCDB->wUpdateCounter + (ET9U16)bValue);
    }
    /* write in all cases (if 0, this will indicate reset) */
    __ET9AWWriteCDBData(pLingInfo,
                        (void ET9FARDATA *)&pCDB->wUpdateCounter,
                        (const void ET9FARDATA *)&wTemp, sizeof(ET9U16));
}


/*---------------------------------------------------------------------------*/
/** \internal
 * Set data area to know fill value.
 *
 * @param pLingInfo     Pointer to ET9 information structure.
 *
 * @return None
 */

static void ET9LOCALCALL __ET9AWFillCDBWithDelims(ET9AWLingInfo *pLingInfo)
{
    ET9SYMB ET9FARDATA *psNext;
    ET9UINT nIndex;
    ET9SYMB sSymb = CDBDELIMITER;

    ET9AWCDBInfo ET9FARDATA * const pCDB = pLingInfo->pLingCmnInfo->pCDBInfo;

    ET9Assert(pLingInfo);
    ET9Assert(pCDB != NULL);

    psNext = ET9CDBData(pCDB);
    /* Loop through all of the data area symbols */
    for (nIndex = 0; nIndex < ET9CDBDataAreaSymbs(pCDB); ++nIndex, ++psNext) {
        __ET9AWWriteCDBData(pLingInfo,
                            (void ET9FARDATA *)psNext,
                            (const void ET9FARDATA *)&sSymb,
                            sizeof(ET9SYMB));
    }
}


/*---------------------------------------------------------------------------*/
/**
 * Set CDB to empty.
 *
 * @param pLingInfo        Pointer to ET9 information structure.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL ET9AWCDBReset(ET9AWLingInfo * const pLingInfo)
{
    ET9STATUS wStatus;
    ET9AWCDBInfo ET9FARDATA  *pCDB;
    ET9U16         wTemp;

    if ((wStatus = _ET9AWSys_BasicValidityCheck(pLingInfo)) == ET9STATUS_NONE) {
        pCDB = pLingInfo->pLingCmnInfo->pCDBInfo;
        if (pCDB == NULL) {
            wStatus = ET9STATUS_NO_CDB;
        }
        else {
            /* reset update counter to zero */
            __ET9AWCDBUpdateCounter(pLingInfo, 0);
            /* set the 'data end' indicator to 0 */
            wTemp = 0;
            __ET9AWWriteCDBData(pLingInfo, (void ET9FARDATA *)&pCDB->wDataEndOffset,
                    (const void ET9FARDATA *)&wTemp, sizeof(ET9U16));
            /* */
            __ET9AWFillCDBWithDelims(pLingInfo);
        }
    }
    return wStatus;
}



/*---------------------------------------------------------------------------*/
/**
 * CDB Init.
 *
 * @param pLingInfo        Pointer to ET9 information structure.
 * @param pCDBInfo         Pointer to allocated CDB buffer.
 * @param wDataSize        Size of the CDB buffer pointed to by pCDBInfo (in bytes).
 * @param pWriteCB         Callback pointer to integration layer write routine (if used).
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL ET9AWCDBInit(ET9AWLingInfo * const             pLingInfo,
                                  ET9AWCDBInfo ET9FARDATA * const   pCDBInfo,
                                  const ET9U16                      wDataSize,
                                  const ET9DBWRITECALLBACK          pWriteCB)
{
    ET9STATUS wStatus;

    if ((wStatus = _ET9AWSys_BasicValidityCheck(pLingInfo)) == ET9STATUS_NONE) {
        if (((pCDBInfo != NULL) && !wDataSize) || ((pCDBInfo == NULL) && wDataSize)) {
            wStatus = ET9STATUS_INVALID_MEMORY;
        }
        /* Check minimum CDB size and even number of bytes */
        else if ((pCDBInfo  != NULL) && ((wDataSize < ET9MINCDBDATABYTES) || (wDataSize & 0x0001))) {
            wStatus = ET9STATUS_INVALID_SIZE;
        }
        else {

            ET9AWLingCmnInfo * const pLingCmnInfo = pLingInfo->pLingCmnInfo;

            if ((pLingCmnInfo->pCDBInfo != NULL) && (pCDBInfo != NULL) &&
               ((pLingCmnInfo->pCDBInfo != pCDBInfo) ||
                (pCDBInfo->wDataSize != wDataSize))) {
                wStatus = ET9STATUS_ALREADY_INITIALIZED;
            }
            pLingCmnInfo->pCDBInfo = pCDBInfo;
            pLingInfo->pCDBWriteData = pWriteCB;

            /* Reset the CDB if internal size doesn't match indicated size */
            if ((pCDBInfo != NULL) && ((pCDBInfo->wDataSize != wDataSize) ||
                (pCDBInfo->wDataEndOffset >= ET9CDBDataAreaSymbs(pCDBInfo)))) {
                /* Update the CDB header with the given size */
                __ET9AWWriteCDBData(pLingInfo,
                         (void ET9FARDATA *)&pCDBInfo->wDataSize,
                         (const void ET9FARDATA*)&wDataSize,
                         sizeof(ET9U16));
                /* and reset to a blank state */
                ET9AWCDBReset(pLingInfo);
            }

            pLingCmnInfo->Private.bStateCDBEnabled = 1;

            _ET9AWCDBBreakContext(pLingInfo);
        }
    }
    return wStatus;
}


/*---------------------------------------------------------------------------*/
/** \internal
 * Write the passed string to the context database.
 *
 * @param pLingInfo    Pointer to ET9 information structure.
 * @param psCopyBuf    String to be added to the CDB.
 * @param nCopyLen     Size (in symbs) of string being added.
 *
 * @return None
 */

static void ET9LOCALCALL __ET9AWAddTextToCDB(ET9AWLingInfo *pLingInfo,
                                             ET9SYMB       *psCopyBuf,
                                             ET9UINT        nCopyLen)
{
    ET9U16 wTemp, wHolder;
    ET9SYMB sTemp = CDBDELIMITER;
    ET9SYMB ET9FARDATA *psNext, *psStart;

    ET9AWCDBInfo ET9FARDATA * const pCDB = pLingInfo->pLingCmnInfo->pCDBInfo;

    ET9Assert(pLingInfo);
    ET9Assert(pCDB != NULL);
    ET9Assert(psCopyBuf);
    ET9Assert(nCopyLen);
    ET9Assert(nCopyLen < ET9CDBDataAreaSymbs(pCDB));

    /* get the end address of write (accounting for possible wrap-around) */
    wTemp = (ET9U16)((pCDB->wDataEndOffset + nCopyLen) % ET9CDBDataAreaSymbs(pCDB));

    /* get the start address of the write (end of previous write) */
    psNext = ET9CDBData(pCDB) + pCDB->wDataEndOffset;

    wHolder = (ET9U16)(ET9CDBDataAreaSymbs(pCDB) - pCDB->wDataEndOffset);
    /* if the string requires wrap-around in CDB data area */
    if (nCopyLen > wHolder) {
        /* write as much of the string that fits prior to wrap */
        /* decrement the size of the remaining data to be written */
        nCopyLen -= wHolder;
        while (wHolder--) {
            ET9Assert(psNext < ET9CDBData(pCDB) + ET9CDBDataAreaSymbs(pCDB));
            __ET9AWWriteCDBData(pLingInfo,
                           (void ET9FARDATA *)psNext++,
                           (const void ET9FARDATA *)psCopyBuf++,
                           sizeof(ET9SYMB));
        }
        /* and start the next write at the beginning of the data area */
        psNext = ET9CDBData(pCDB);
    }
    /* now write the string (either whole or remaining wrap-around portion) */
    psStart = psNext;
    while (nCopyLen--) {
        ET9Assert(psNext < ET9CDBData(pCDB) + ET9CDBDataAreaSymbs(pCDB));
        __ET9AWWriteCDBData(pLingInfo,
                           (void ET9FARDATA *)psNext++,
                           (const void ET9FARDATA *)psCopyBuf++,
                           sizeof(ET9SYMB));
    }
    /* increment update counter */
    __ET9AWCDBUpdateCounter(pLingInfo, 1);

    /* this write may have overwritten an older CDB entry; check to */
    /* remove any leftover characters from older entry              */
    /* save original address to prevent tight loop */
    /* check for wrap... psNext may be right at the end of the data area */
    if (psNext >= ET9CDBData(pCDB) + ET9CDBDataAreaSymbs(pCDB)) {
        psNext = psNext - ET9CDBDataAreaSymbs(pCDB);
    }
    /* clear the leftover symbols of the older entry (if there are any ) */
    while (*psNext != CDBDELIMITER && psNext != psStart) {
        __ET9AWWriteCDBData(pLingInfo, (void ET9FARDATA *)psNext,
                 (const void ET9FARDATA *)&sTemp, sizeof(ET9SYMB));
        /* account for possible wrap-around */
        if (++psNext >= ET9CDBData(pCDB) + ET9CDBDataAreaSymbs(pCDB)) {
            psNext = ET9CDBData(pCDB);
        }
    }

    /* write the new data end offset */
    __ET9AWWriteCDBData(pLingInfo, (void ET9FARDATA *)&pCDB->wDataEndOffset,
            (const void ET9FARDATA *)&wTemp, sizeof(ET9U16) );

    /* increment update counter */
    __ET9AWCDBUpdateCounter(pLingInfo, 1);
}


/*---------------------------------------------------------------------------*/
/** \internal
 * Get number of trailing delimiters in CDB.
 *
 * @param pLingInfo    Pointer to ET9 information structure.
 *
 * @return Number trailing delims, (any more than 2 returned as 2).
 */

static ET9UINT ET9LOCALCALL __ET9AWGetNumCDBTrailingDelims(ET9AWLingInfo *pLingInfo)
{
    ET9U16  wCharPos;
    ET9UINT nNumDelims = 0;

    ET9AWCDBInfo ET9FARDATA * const pCDB = pLingInfo->pLingCmnInfo->pCDBInfo;

    ET9Assert(pLingInfo);
    ET9Assert(pCDB != NULL);

    wCharPos = (ET9U16)((pCDB->wDataEndOffset + ET9CDBDataAreaSymbs(pCDB) - 1) %
                                 ET9CDBDataAreaSymbs(pCDB));
    while (nNumDelims < BROKEN_CONTEXT_DELIMITER_COUNT) {
        if (*(ET9CDBData(pCDB) + wCharPos) != CDBDELIMITER) {
            break;
        }
        ++nNumDelims;
        wCharPos = (ET9U16)((wCharPos + ET9CDBDataAreaSymbs(pCDB) - 1) %
                                 ET9CDBDataAreaSymbs(pCDB));
    }
    return nNumDelims;
}


/*---------------------------------------------------------------------------*/
/** \internal
 * Break context in CDB by makeing sure two delimiters are entered.
 *
 * @param pLingInfo    Pointer to ET9 information structure.
 *
 * @return None
 */

void ET9FARCALL _ET9AWCDBBreakContext(ET9AWLingInfo *pLingInfo)
{
    ET9SYMB sSpaceHolder = CDBDELIMITER;
    ET9UINT nDelims;

    ET9Assert(pLingInfo);

    /* may still get called even with no CDB; just return */

    if (!pLingInfo->pLingCmnInfo->pCDBInfo) {
        return;
    }

    for (nDelims = __ET9AWGetNumCDBTrailingDelims(pLingInfo); nDelims < BROKEN_CONTEXT_DELIMITER_COUNT; ++nDelims) {
        __ET9AWAddTextToCDB(pLingInfo, &sSpaceHolder, 1);
    }
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Retrieve the last word in the CDB.
 *
 * @param pLingInfo    Pointer to ET9 information structure.
 * @param pWord        Word buffer to collect last word in.
 *
 * @return 1 if passed word is last word, zero otherwise.
 */

static void ET9LOCALCALL __ET9AWGetLastWordEntered(ET9AWLingInfo     *pLingInfo,
                                                   ET9AWPrivWordInfo *pWord)
{
    ET9UINT nOffsetOfSymb;
    ET9U16  i = 0;
    ET9U16  j = 0;
    ET9SYMB sSym;

    ET9AWCDBInfo ET9FARDATA * const pCDB = pLingInfo->pLingCmnInfo->pCDBInfo;

    ET9Assert(pLingInfo);
    ET9Assert(pWord);
    ET9Assert(pCDB != NULL);

    pWord->Base.wWordLen = 0;
    nOffsetOfSymb = (ET9U16)((pCDB->wDataEndOffset - (ET9U16)1 + ET9CDBDataAreaSymbs(pCDB)) % ET9CDBDataAreaSymbs(pCDB));

    if (*(ET9CDBData(pCDB) + nOffsetOfSymb) == CDBDELIMITER) {
        nOffsetOfSymb = nOffsetOfSymb ? (ET9UINT) (nOffsetOfSymb - 1) :
                 (ET9UINT) (ET9CDBDataAreaSymbs(pCDB) - 1);
        /* loop through, comparing CDB entry with passed word */
        while (*(ET9CDBData(pCDB) + nOffsetOfSymb) != CDBDELIMITER) {
            pWord->Base.sWord[i++] = *(ET9CDBData(pCDB) + nOffsetOfSymb);
            nOffsetOfSymb = nOffsetOfSymb ? (ET9UINT) (nOffsetOfSymb - 1) :
                 (ET9UINT) (ET9CDBDataAreaSymbs(pCDB) - 1);
        }
        if (i) {
            pWord->Base.wWordLen = i;
            i--;
            for (; j < i; ++j, --i) {
                sSym = pWord->Base.sWord[j];
                pWord->Base.sWord[j] = pWord->Base.sWord[i];
                pWord->Base.sWord[i] = sSym;
            }
        }
    }
    return;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Check if last word in the CDB is the passed word.
 *
 * @param pLingInfo    Pointer to ET9 information structure.
 * @param psWord       Word buffer.
 * @param nWordLen     Length of word buffer (in symbs).
 *
 * @return 1 if passed word is last word, zero otherwise.
 */

static ET9UINT ET9LOCALCALL __ET9AWIsLastWordEntered(ET9AWLingInfo *pLingInfo,
                                                     ET9SYMB       *psWord,
                                                     ET9UINT        nWordLen)
{
    ET9UINT nOffsetOfSymb;
    ET9UINT nMatch = 0;

    ET9AWCDBInfo ET9FARDATA * const pCDB = pLingInfo->pLingCmnInfo->pCDBInfo;

    ET9Assert(pLingInfo);
    ET9Assert(psWord);
    ET9Assert(pCDB != NULL);
    ET9Assert(nWordLen);

    nOffsetOfSymb = (ET9U16)((pCDB->wDataEndOffset - (ET9U16)1 + ET9CDBDataAreaSymbs(pCDB)) % ET9CDBDataAreaSymbs(pCDB));

    if (*(ET9CDBData(pCDB) + nOffsetOfSymb) == CDBDELIMITER) {
        /* loop through, comparing CDB entry with passed word */
        while (nWordLen--) {
            nOffsetOfSymb = nOffsetOfSymb ? (ET9UINT) (nOffsetOfSymb - 1) :
                 (ET9UINT) (ET9CDBDataAreaSymbs(pCDB) - 1);
            /* if the syms don't match, get outta here with failure indication */
            if (ET9AWSys_GetBilingualSupported(pLingInfo)) {
                if ((_ET9SymToLower(*(ET9CDBData(pCDB) + nOffsetOfSymb), pLingInfo->pLingCmnInfo->wFirstLdbNum) != *(psWord + nWordLen)) &&
                    (_ET9SymToLower(*(ET9CDBData(pCDB) + nOffsetOfSymb), pLingInfo->pLingCmnInfo->wSecondLdbNum) != *(psWord + nWordLen))) {
                        break;
                }
            }
            else if (_ET9SymToLower(*(ET9CDBData(pCDB) + nOffsetOfSymb), pLingInfo->pLingCmnInfo->wLdbNum) != *(psWord + nWordLen)) {
                break;
            }
        }
        nMatch = 1;
    }
    return nMatch;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Update context buffer.
 *
 * @param pLingInfo    Pointer to ET9 information structure.
 * @param pWord        Word to be added.
 *
 * @return None
 */

void ET9FARCALL _ET9AWCDBUpdateContext(ET9AWLingInfo     *pLingInfo,
                                       ET9AWPrivWordInfo *pWord)
{
    ET9U16 wCopyLen;
    ET9U16 i;
    ET9SYMB *pDest;
    ET9SYMB *pSrc;
    ET9AWLingCmnInfo * const pLingCmnInfo = pLingInfo->pLingCmnInfo;
    wCopyLen = pWord->Base.wWordLen;

    ET9Assert(pLingInfo);
    ET9Assert(pWord);
    ET9Assert(pWord->Base.wWordLen);
    ET9Assert(wCopyLen);

    if (wCopyLen > ET9MAXWORDSIZE) {
        wCopyLen = ET9MAXWORDSIZE;
    }

    if (pLingCmnInfo->Private.bContextWordSize) {
        pLingCmnInfo->Private.bPreviousContextWordSize = pLingCmnInfo->Private.bContextWordSize;
        pDest = pLingCmnInfo->Private.sPreviousContextWord;
        pSrc = pLingCmnInfo->Private.sContextWord;
        for (i = pLingCmnInfo->Private.bContextWordSize; i; --i) {
            *pDest++ = *pSrc++;
        }
    }

    pLingCmnInfo->Private.bContextWordSize = (ET9U8)wCopyLen;
    pDest = pLingCmnInfo->Private.sContextWord;
    pSrc = pWord->Base.sWord;

    for (i = wCopyLen; i; --i) {
        *pDest++ = *pSrc++;
    }
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Save word to context buffer and add to context database.
 * Note: it is assumed that this function is called only after a space has been entered.
 *
 * @param pLingInfo    Pointer to ET9 information structure.
 * @param pWord        Word to be added.
 *
 * @return None
 */

static void ET9LOCALCALL __ET9AWAddToContextDatabase(ET9AWLingInfo     *pLingInfo,
                                                     ET9AWPrivWordInfo *pWord)
{
    ET9U16 wCopyLen;
    ET9SYMB sSpaceHolder = CDBDELIMITER;
    ET9AWLingCmnInfo * const pLingCmnInfo = pLingInfo->pLingCmnInfo;

    ET9Assert(pLingInfo);
    ET9Assert(pWord);
    ET9Assert(pLingCmnInfo->pCDBInfo != NULL);
    wCopyLen = pWord->Base.wWordLen;
    ET9Assert(wCopyLen);

    if (wCopyLen > ET9MAXWORDSIZE) {
        wCopyLen = ET9MAXWORDSIZE;
    }
    /* Check if last word in database is word from context buffer. */
    /* If not, add context buffer word first.                      */

    if (pLingCmnInfo->Private.bContextWordSize &&
        !__ET9AWIsLastWordEntered(pLingInfo, pLingCmnInfo->Private.sContextWord, pLingCmnInfo->Private.bContextWordSize)) {

        __ET9AWAddTextToCDB(pLingInfo, pLingCmnInfo->Private.sContextWord, pLingCmnInfo->Private.bContextWordSize);

        if (!__ET9AWGetNumCDBTrailingDelims(pLingInfo)) {
            __ET9AWAddTextToCDB(pLingInfo, &sSpaceHolder, 1);
        }
    }

    /* add the passed word to the CDB */
    /* if word exists as ASDB or LDB-AS entry, get original version */

    if (!_ET9AWFindASDBObject(pLingInfo, pWord->Base.sWord, wCopyLen, 1, 1)) {
        if (!_ET9AWFindLdbASObject(pLingInfo, pLingCmnInfo->wFirstLdbNum, pWord->Base.sWord, wCopyLen, 1, 1)) {
            if (ET9AWSys_GetBilingualSupported(pLingInfo)) {
                _ET9AWFindLdbASObject(pLingInfo, pLingCmnInfo->wSecondLdbNum, pWord->Base.sWord, wCopyLen, 1, 1);
            }
        }
    }
    __ET9AWAddTextToCDB(pLingInfo, pWord->Base.sWord, wCopyLen);

    if (!__ET9AWGetNumCDBTrailingDelims(pLingInfo)) {
        __ET9AWAddTextToCDB(pLingInfo, &sSpaceHolder, 1);
    }

    _ET9AWCDBUpdateContext(pLingInfo, pWord);

}

/*---------------------------------------------------------------------------*/
/** \internal
 * Add word to CDB database and context buffer.
 *
 * @param pLingInfo    Pointer to ET9 information structure.
 * @param pWord        Word to be added.
 *
 * @return None
 */

void ET9FARCALL _ET9AWCDBAddWord(ET9AWLingInfo     *pLingInfo,
                                 ET9AWPrivWordInfo *pWord)
{
    ET9Assert(pLingInfo);
    ET9Assert(pWord);
    ET9Assert(pWord->Base.wWordLen);


    if (pLingInfo->pLingCmnInfo->pCDBInfo != NULL) {
        __ET9AWAddToContextDatabase(pLingInfo, pWord);
    }
    else {
        _ET9AWCDBUpdateContext(pLingInfo, pWord);
    }

}


/*---------------------------------------------------------------------------*/
/** \internal
 * Add shortcut to CDB database and context buffer.
 *
 * @param pLingInfo    Pointer to ET9 information structure.
 * @param pWord        Word to be added.
 *
 * @return None
 */

void ET9FARCALL _ET9AWCDBAddShortcut(ET9AWLingInfo     *pLingInfo,
                                     ET9AWPrivWordInfo *pWord)
{
    ET9AWPrivWordInfo pWord2;

    ET9Assert(pLingInfo);
    ET9Assert(pWord);
    ET9Assert(pWord->Base.wWordLen);

    if (!pLingInfo->pLingCmnInfo->pCDBInfo || !ET9CDBENABLED(pLingInfo->pLingCmnInfo)) {
        return;
    }

    __ET9AWGetLastWordEntered(pLingInfo, &pWord2);

    if (pWord2.Base.wWordLen) {
        __ET9AWAddToContextDatabase(pLingInfo, pWord);
        _ET9AWCDBBreakContext(pLingInfo);
        __ET9AWAddToContextDatabase(pLingInfo, &pWord2);
    }
}


/*---------------------------------------------------------------------------*/
/** \internal
 * Search CDB for a tap sequence with a frequency searching range.
 *
 * @param pLingInfo        Pointer to alpha information structure.
 * @param wIndex           Index of beginning of active word segment.
 * @param wLength          Length of active sequence.
 * @param bFreqIndicator   Freq (for _ET9AWSelLstWordSearch callback).
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL _ET9AWCDBWordsSearch(ET9AWLingInfo         *pLingInfo,
                                          ET9U16                wIndex,
                                          ET9U16                wLength,
                                          ET9_FREQ_DESIGNATION  bFreqIndicator)
{
    ET9STATUS               wStatus = ET9STATUS_NONE;
    ET9UINT                 nCDBWordLen, nMatch, nNextCharPos;
    ET9UINT                 nCDBWordLen2;
    ET9UINT                 nTrigramMatch;
    ET9UINT                 nSize = 0, nNumTested = 0, nNumWhite = 0;
    ET9UINT                 nNumTrailingPunct = 0, nNumLeadingPunct = 0, nNumNumerics = 0;
    ET9UINT                 nIndex = 0;
    ET9UINT                 nContextWordSize;
    ET9UINT                 nPreviousContextWordSize;
    ET9INT                  snSearchPosition2;
    ET9UINT                 nWordEnd = 0, nWordStart = 0, nRHWEnd = 0, nRHWStart = 0, nRHWLength;
    ET9INT                  snSearchPosition, snStopPoint = 0;
    ET9U8                   aCharClass;
    ET9SYMB                 sLastWord[ET9MAXWORDSIZE];
    ET9U16                  wLastWordLen = 0;
    ET9SYMB                 *pStr1;
    ET9SYMB                 *pStr2;
    ET9UINT                 i;
    ET9AWPrivWordInfo       sLookedUpWord;


    ET9AWLingCmnInfo * const pLingCmnInfo = pLingInfo->pLingCmnInfo;
    ET9AWCDBInfo ET9FARDATA * const pCDB = pLingCmnInfo->pCDBInfo;

    const ET9U16 wCurrMinSourceLength = pLingCmnInfo->Private.wCurrMinSourceLength;

    ET9Assert(pLingInfo);

    /* if no context database */

    if ((pCDB == NULL) || !ET9CDBENABLED(pLingInfo->pLingCmnInfo)){
        return ET9STATUS_NONE;
    }

    nContextWordSize = (ET9UINT)pLingCmnInfo->Private.bContextWordSize;
    nPreviousContextWordSize = (ET9UINT)pLingCmnInfo->Private.bPreviousContextWordSize;

    /* IF no context word exists OR active word segment doesn't start at */
    /* beginning of the active word, no need to search                   */

    if (!nContextWordSize || wIndex) {
        return ET9STATUS_NONE;
    }

    /* if trying to get prediction and turned off, leave */

    if (!wLength && !ET9NEXTWORDPREDICTION_MODE(pLingCmnInfo)) {
        return ET9STATUS_NONE;
    }

    /* if there is trailing punct in the context word, */
    /* and it doesn't look like an emoticon, return    */

    if (_ET9_IsPunctChar(pLingCmnInfo->Private.sContextWord[nContextWordSize - 1])) {
        if (!_ET9StringLikelyEmoticon(pLingCmnInfo->Private.sContextWord, nContextWordSize)) {
            return ET9STATUS_NONE;
        }
    }

    snSearchPosition = (ET9INT)(pCDB->wDataEndOffset - (ET9U16)1 + ET9CDBDataAreaSymbs(pCDB)) % ET9CDBDataAreaSymbs(pCDB);

    /* search the entire CDB, working backwards from the most recent entry */

    while (snSearchPosition >= snStopPoint) {
        nCDBWordLen = 0;
        nMatch = 0;
        nTrigramMatch = 0;

        /* if the current symbol is a delimiter, continue */

        if (*(ET9CDBData(pCDB) + snSearchPosition) == CDBDELIMITER) {
            ++nNumWhite;
        }

        /* search to see if this is context word, and hold pointers to beginning and end  */

        else {

            /* assume match if context not broken after word */

            nMatch = (nNumWhite == 1) ? 1: 0;
            nWordEnd = (ET9UINT)snSearchPosition;

            /* this loops through the word that was found to see if it matches current word */

            nIndex = ET9CDBDataAreaSymbs(pCDB);
            while (nIndex--) {

                /* if the symbol is a delimiter... */

                if (*(ET9CDBData(pCDB) + snSearchPosition) == CDBDELIMITER) {

                    /* if the db word and the context word have different sizes */

                    if (nCDBWordLen != nContextWordSize) {

                        /* no possible match */

                        nMatch = 0;
                    }
                    break;
                }
                /* if db word appears to be larger than context word... */

                if (nMatch && (nCDBWordLen >= nContextWordSize)) {

                    /* no possible match */

                    nMatch = 0;
                }
                /* if a symbol mismatch occurs comparing db word and context word */

                if (ET9AWSys_GetBilingualSupported(pLingInfo)) {
                    if (nMatch) {
                        if (nContextWordSize == (nCDBWordLen + 1)){
                            if ((_ET9SymToLower(*(ET9CDBData(pCDB) + snSearchPosition), pLingCmnInfo->wFirstLdbNum) !=
                                _ET9SymToLower(pLingCmnInfo->Private.sContextWord[0], pLingCmnInfo->wFirstLdbNum)) &&
                                (_ET9SymToLower(*(ET9CDBData(pCDB) + snSearchPosition), pLingCmnInfo->wSecondLdbNum) !=
                                _ET9SymToLower(pLingCmnInfo->Private.sContextWord[0], pLingCmnInfo->wSecondLdbNum))) {

                                    /* no possible match */

                                    nMatch = 0;
                            }
                        }
                        else {
                            if (*(ET9CDBData(pCDB) + snSearchPosition) != pLingCmnInfo->Private.sContextWord[nContextWordSize - 1 - nCDBWordLen]) {

                                /* no possible match */

                                nMatch = 0;
                            }
                        }
                    }
                }
                else if (nMatch) {
                    if (nContextWordSize == (nCDBWordLen + 1)) {
                        if (_ET9SymToLower(*(ET9CDBData(pCDB) + snSearchPosition), pLingCmnInfo->wLdbNum) !=
                            _ET9SymToLower(pLingCmnInfo->Private.sContextWord[0], pLingCmnInfo->wLdbNum)) {

                                /* no possible match */

                                nMatch = 0;
                        }
                    }
                    else {
                        if (*(ET9CDBData(pCDB) + snSearchPosition) != pLingCmnInfo->Private.sContextWord[nContextWordSize - 1 - nCDBWordLen]) {

                            /* no possible match */

                            nMatch = 0;
                        }
                    }
                }
                /* set wordstart to current position so it will be set correctly when we're done */

                nWordStart = snSearchPosition;

                if (!snSearchPosition) {
                    if (!pCDB->wDataEndOffset) {
                        break;
                    }
                    snStopPoint = (ET9INT)pCDB->wDataEndOffset;
                    snSearchPosition = (ET9INT)ET9CDBDataAreaSymbs(pCDB) - 1;
                }
                else {
                    --snSearchPosition;
                }
                ++nCDBWordLen;
            }
            ++nNumTested;

            /* if context match, check previous context entry */

            if (nMatch && nPreviousContextWordSize) {
                nTrigramMatch = 1;

                /* loops through the word before the one that was found to see if it matches previous context word */

                nIndex = ET9CDBDataAreaSymbs(pCDB);
                nCDBWordLen2 = 0;
                if (!snSearchPosition) {
                   snSearchPosition2 = (ET9INT)ET9CDBDataAreaSymbs(pCDB) - 1;
                   if (!pCDB->wDataEndOffset) {
                        nTrigramMatch = 0;
                        nIndex = 0;
                    }
                }
                else {
                    snSearchPosition2 = snSearchPosition - 1;
                }
                /* search for length of previous context word */

                while (nIndex--) {

                    /* if the symbol is a delimiter... */

                    if (*(ET9CDBData(pCDB) + snSearchPosition2) == CDBDELIMITER) {

                        /* if the db word and the context word have different sizes */

                        if (nCDBWordLen2 != nPreviousContextWordSize) {

                            /* no possible match */

                            nTrigramMatch = 0;
                        }
                        break;
                    }
                    /* if db word appears to be larger than context word... */

                    if (nTrigramMatch && (nCDBWordLen2 >= nPreviousContextWordSize)) {

                        /* no possible match */

                        nTrigramMatch = 0;
                        break;
                    }

                    /* if a symbol mismatch occurs comparing db word and context word */

                    if (ET9AWSys_GetBilingualSupported(pLingInfo)) {
                        if (nTrigramMatch) {
                            if (nPreviousContextWordSize == (nCDBWordLen2 + 1)) {
                                if ((_ET9SymToLower(*(ET9CDBData(pCDB) + snSearchPosition2), pLingCmnInfo->wFirstLdbNum) !=
                                    _ET9SymToLower(pLingCmnInfo->Private.sPreviousContextWord[0], pLingCmnInfo->wFirstLdbNum)) &&
                                    (_ET9SymToLower(*(ET9CDBData(pCDB) + snSearchPosition2), pLingCmnInfo->wSecondLdbNum) !=
                                    _ET9SymToLower(pLingCmnInfo->Private.sPreviousContextWord[0], pLingCmnInfo->wSecondLdbNum))) {

                                        /* no possible match */

                                        nTrigramMatch = 0;
                                        break;
                                }
                            }
                            else {
                                if (*(ET9CDBData(pCDB) + snSearchPosition2) != pLingCmnInfo->Private.sPreviousContextWord[nPreviousContextWordSize - 1 - nCDBWordLen2]) {

                                    /* no possible match */

                                    nTrigramMatch = 0;
                                    break;
                                }
                            }
                        }
                    }
                    else if (nTrigramMatch) {
                        if (nPreviousContextWordSize == (nCDBWordLen2 + 1)) {
                            if (_ET9SymToLower(*(ET9CDBData(pCDB) + snSearchPosition2), pLingCmnInfo->wLdbNum) !=
                                _ET9SymToLower(pLingCmnInfo->Private.sPreviousContextWord[0], pLingCmnInfo->wLdbNum)) {

                                    /* no possible match */

                                    nTrigramMatch = 0;
                                    break;
                            }
                        }
                        else {
                            if (*(ET9CDBData(pCDB) + snSearchPosition2) != pLingCmnInfo->Private.sPreviousContextWord[nPreviousContextWordSize - 1 - nCDBWordLen2]) {

                                /* no possible match */

                                nTrigramMatch = 0;
                                break;
                            }
                        }
                    }

                    if (!snSearchPosition2) {
                        if (!pCDB->wDataEndOffset) {
                            break;
                        }
                        snSearchPosition2 = (ET9INT)ET9CDBDataAreaSymbs(pCDB) - 1;
                    }
                    else {
                        --snSearchPosition2;
                    }
                    ++nCDBWordLen2;
                }

            }
        }

        /* okay, we've found a match, lets see if the following word matches active word */

        aCharClass = ET9SYMUNKNMASK;

        if (nMatch) {

            /* check to see if following word matches currently active sequence */

            nRHWLength = (nRHWEnd + 1 + ET9CDBDataAreaSymbs(pCDB) - nRHWStart) % ET9CDBDataAreaSymbs(pCDB);
            if ((nNumTested <= 1) || (nRHWLength < wCurrMinSourceLength)) {
                nMatch = 0;
            }
            else {

                ET9U8 aClass;
                ET9U8 aKey;

                _InitPrivWordInfo(&sLookedUpWord);

                nSize = 0;
                nNumTrailingPunct = nNumLeadingPunct = nNumNumerics = 0;

                while (nSize < ET9MAXWORDSIZE && nSize < nRHWLength) {

                    nNextCharPos = (nRHWStart + nSize) % ET9CDBDataAreaSymbs(pCDB);
                    sLookedUpWord.Base.sWord[nSize] = *(ET9CDBData(pCDB) + nNextCharPos);
                    ++sLookedUpWord.Base.wWordLen;
                    _ET9_GetFullSymbolKeyAndClass(sLookedUpWord.Base.sWord[nSize], &aKey, &aClass);

                    if (nSize >= wLength) { /* this appears outdated */

                        if (aClass == ET9SYMPUNCTMASK) {  /* punct case */
                            if (nNumLeadingPunct == nSize) {
                                ++nNumLeadingPunct;
                            }
                            else {
                                ++nNumTrailingPunct;
                            }
                        }
                        else {
                            if (aClass == ET9SYMNUMBRMASK) {
                                ++nNumNumerics;
                            }
                            nNumTrailingPunct = 0;
                        }
                    }
                    ++nSize;

                    /* make sure we're not checking off the end */

                    if (((nNextCharPos + 1) % ET9CDBDataAreaSymbs(pCDB)) == pCDB->wDataEndOffset) {
                        nMatch = 0;
                        break;
                    }
                    aCharClass = aClass;
                }
            }
        }

        /* if there is a match so far, we still want to do the following processing:

            a) if the word HAS NO leading punct, we will strip trailing punct unless this
                looks like an emoticon (there are more non-alpha than there are alpha
                characters in the word).
            b) if the word HAS leading punct,  we will reject the
                word unless it looks like an emoticon.
        */

        if (nMatch) {

            /* doesn' look like emoticon */

            if ((2 * (nNumTrailingPunct + nNumLeadingPunct + nNumNumerics)) < nSize) {

                if (nSize != wLength) { /* this appears outdated */

                    if (!nNumLeadingPunct) {
                        nSize -= nNumTrailingPunct;
                        if (nSize < wCurrMinSourceLength) {
                            nMatch = 0;
                        }
                        else {
                            sLookedUpWord.Base.wWordLen = (ET9U16)nSize;
                        }
                    }
                    else {
                        nMatch = 0;
                    }
                }

            }

            /* looks like emoticon. */

            else if (nSize == 1 && aCharClass == ET9SYMPUNCTMASK) {  /* don't allow single character emoticons */
                nMatch = 0;
            }

            /* if completion not on, reject here if not right size */

            if (!ET9WORDCOMPLETION_MODE(pLingCmnInfo) &&
                wLength && nSize != wLength) { /* this appears outdated */
                nMatch = 0;
            }
        }

        /* if key match also, deal with match */

        if (nMatch) {

            sLookedUpWord.bCDBTrigram = (ET9U8)nTrigramMatch;
            sLookedUpWord.bWordSrc = ET9WORDSRC_CDB;

            if (ET9AWSys_GetBilingualSupported(pLingInfo)) {
                sLookedUpWord.Base.bLangIndex = ET9AWBOTH_LANGUAGES;
            }
            else {
                sLookedUpWord.Base.bLangIndex = ET9AWFIRST_LANGUAGE;
            }

            sLookedUpWord.bEditDistSpc = 0;
            sLookedUpWord.bEditDistStem = 0;

            /* the following assigns a frequency that keeps the CDB */
            /* entries in the order they are picked from the CDB    */

            sLookedUpWord.xWordFreq = (ET9FREQPART)(pLingCmnInfo->Private.bListSize - pLingCmnInfo->Private.bTotalWords);
            if (pLingCmnInfo->Private.eCurrSelectionListMode == ET9ASLMODE_MIXED) {
                sLookedUpWord.xWordFreq += (ET9FREQPART) (ET9_SUPP_DB_BASE_FREQ + (ET9U8) nTrigramMatch * ET9SUPP_DB_FREQ_BUMP_COUNT);
            }
            sLookedUpWord.xTapFreq = 1;

            /* if not a prediction */

            if (wLength) {

                /* if word being compounded, downshift it */

                if (wIndex) {

                    ET9SYMB *pSymb = sLookedUpWord.Base.sWord;
                    ET9UINT i = sLookedUpWord.Base.wWordLen;

                    for (; i; --i, ++pSymb) {
                        *pSymb = _ET9SymToLower(*pSymb, pLingCmnInfo->wLdbNum);
                    }
                }

                wStatus = _ET9AWSelLstWordSearch(pLingInfo, &sLookedUpWord, wIndex, wLength, bFreqIndicator);

                if (wStatus != ET9STATUS_NONE) {
                    return wStatus;
                }
            }

            /* otherwise, just add to list */

            else {

                /* make sure word is not a shortcut... shortcuts should never be submitted as NWPs */

                wStatus = ET9AWASDBFindEntry(pLingInfo,
                                             sLookedUpWord.Base.sWord,
                                             sLookedUpWord.Base.wWordLen,
                                             sLookedUpWord.Base.sSubstitution,
                                             ET9MAXWORDSIZE,
                                             &sLookedUpWord.Base.wSubstitutionLen);

                if (wStatus == ET9STATUS_NO_MATCHING_WORDS) {

                     wStatus = _ET9AWSelLstAdd(pLingInfo,
                                               &sLookedUpWord,
                                               wLength,
                                               bFreqIndicator);

                     _ET9SymCopy(sLastWord, sLookedUpWord.Base.sWord, sLookedUpWord.Base.wWordLen);

                     wLastWordLen = sLookedUpWord.Base.wWordLen;

                     /* if first NWP (which is default) and 'downshift default' option is on */
                     /*  and word is uppercase (without shift), add lowercase version        */

                    if (pLingCmnInfo->Private.bTotalWords == 1 &&
                        ET9DOWNSHIFTDEFAULTENABLED(pLingCmnInfo) &&
                        !ET9SHIFT_MODE(pLingCmnInfo->Base.pWordSymbInfo->dwStateBits) &&
                        !ET9CAPS_MODE(pLingCmnInfo->Base.pWordSymbInfo->dwStateBits) &&
                        ET9SymIsUpper(sLookedUpWord.Base.sWord[0])) {

                        ET9U8 bIndex;

                        for (bIndex = 0; bIndex < sLookedUpWord.Base.wWordLen; ++bIndex) {
                            sLookedUpWord.Base.sWord[bIndex] = _ET9SymToLower(sLookedUpWord.Base.sWord[bIndex], pLingCmnInfo->wLdbNum);
                        }

                        wStatus = _ET9AWSelLstAdd(pLingInfo, &sLookedUpWord, wLength, bFreqIndicator);
                    }
                }
                else {

                    /* if last word submitted wasn't the substitution for this shortcut, ok to submit */
                    /* if last word longer than substitution, can't be a match */

                    if (wLastWordLen > sLookedUpWord.Base.wSubstitutionLen) {
                        wLastWordLen = 0;
                    }

                    pStr1 = sLookedUpWord.Base.sSubstitution;
                    pStr2 = sLastWord;

                    for (i = 0; i < wLastWordLen; ++i) {
                        if (_ET9SymToLower(*pStr1++, pLingCmnInfo->wLdbNum) != _ET9SymToLower(*pStr2++, pLingCmnInfo->wLdbNum)) {
                            break;
                        }
                    }

                    /* if no match (or no last word), submit WITHOUT substitution */

                    if (!wLastWordLen || i != wLastWordLen) {

                        sLookedUpWord.Base.wSubstitutionLen = 0;
                        wStatus = _ET9AWSelLstAdd(pLingInfo, &sLookedUpWord, wLength, bFreqIndicator);
                        _ET9SymCopy(sLastWord, sLookedUpWord.Base.sWord, sLookedUpWord.Base.wWordLen);
                        wLastWordLen = sLookedUpWord.Base.wWordLen;
                    }
                }
            }
        }

        if (nCDBWordLen) {

        /* set right hand word info for next time through */

            nRHWStart = nWordStart;
            nRHWEnd = nWordEnd;
            nNumWhite = 1;
        }

        /* modify while loop condition to search righ side of datbase */

        if (!snSearchPosition) {
            if (!pCDB->wDataEndOffset) {
                break;
            }
            snStopPoint = (ET9INT) pCDB->wDataEndOffset;
            snSearchPosition = (ET9INT) ET9CDBDataAreaSymbs(pCDB) - 1;
        }
        else {
            --snSearchPosition;
        }
    }

    return wStatus;
}

#endif /* ET9_ALPHABETIC_MODULE */
/*! @} */
/* ----------------------------------< eof >--------------------------------- */
