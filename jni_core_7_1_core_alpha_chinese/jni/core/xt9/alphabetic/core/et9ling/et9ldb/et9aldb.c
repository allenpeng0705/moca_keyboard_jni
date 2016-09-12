/*******************************************************************************
;*******************************************************************************
;**                                                                           **
;**                  COPYRIGHT 2001-2011 NUANCE COMMUNICATIONS                **
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
;**     FileName: et9aldb.c                                                   **
;**                                                                           **
;**  Description: ET9 Alphabetic LDB Module (Intervals)                       **
;**                                                                           **
;*******************************************************************************
;******************************************************************************/

/*! \addtogroup et9aldb LDB for alphabetic
* LDB for alphabetic XT9.
* @{
*/

#include "et9api.h"
#ifdef ET9_ALPHABETIC_MODULE
#include "et9imu.h"
#include "et9asys.h"
#include "et9aldb.h"
#include "et9amisc.h"
#include "et9sym.h"
#include "et9aspc.h"
#include "et9aasdb.h"
#include "et9alsasdb.h"


#ifdef ET9_DEBUGLOG3
#ifdef _WIN32
#pragma message ("*** ET9_DEBUGLOG3 ACTIVATED ***")
#endif
#include <stdio.h>
#define WLOG3(q) { if (pLogFile3 == NULL) { pLogFile3 = fopen("zzzET9ALDB.txt", "w"); } q fflush(pLogFile3); }
static FILE *pLogFile3 = NULL;
#else
#define WLOG3(q)
#endif

#ifdef ET9_DEBUGLOG3B
#ifdef _WIN32
#pragma message ("*** ET9_DEBUGLOG3B ACTIVATED ***")
#endif
#define WLOG3B(q) WLOG3(q)
#else
#define WLOG3B(q)
#endif


#ifdef ET9_DIRECT_LDB_ACCESS
#ifdef _WIN32
#pragma message ("*** USING DIRECT LDB ACCESS (FAST) ***")
#endif
#endif


/*******************************************************************************
 **
 **          G L O B A L S   A N D   L O C A L   S T A T I C S
 **
 ** ET9 does not make use of any dynamic global or local static variables!!
 ** It is acceptable to make use of constant globals or local statics.
 ** If you need persistent dynamic memory in the ET9 core, it should be
 ** allocated in the ET9AWLingPrivate data structure and fogged through the definitions
 ** found in the et9asystm.h file.
 **
 ******************************************************************************/

#define ASPC                            (pLingCmnInfo->Private.ASpc)    /**< \internal Convenience data access for ASPC (from common) structure. */
#define ALDB                            (pLingCmnInfo->Private.ALdb)    /**< \internal Convenience data access for ALDB (from common) structure. */
#define ALDBL                           (pLingInfo->Private.ALdb)       /**< \internal Convenience data access for ALDB (from local) structure. */

#define ALDB_INTERVAL_MULTI_BYTE_CODE   0xFF                            /**< \internal single byte value that escapes to multi byte */
#define ALDB_INTERVAL_END_CHAR_VAL      0x16FD                          /**< \internal unicode value for "end" marker */
#define ALDB_INTERVAL_JUMP_CHAR_VAL     0x16FE                          /**< \internal unicode value for "jump" */
#define ALDB_INTERVAL_EXTEND_CHAR_VAL   0x16FF                          /**< \internal unicode value for "extended interval" */

#define ET9HASHLDBBUFFSIZE              0x100                           /**< \internal buffer size for checksum calculations */

/*---------------------------------------------------------------------------*/
/** \internal
 * Check if a symbol is private.
 *
 * @param wSymbol       Symbol to check.
 *
 * @return True/false
 */

#define __IsPrivateSymbol(wSymbol)                                                                      \
    ((wSymbol) == 0 ||                                                                                  \
     (wSymbol) == ALDB_INTERVAL_END_CHAR_VAL ||                                                         \
     (wSymbol) == ALDB_INTERVAL_JUMP_CHAR_VAL ||                                                        \
     (wSymbol) == ALDB_INTERVAL_EXTEND_CHAR_VAL)                                                        \

/*---------------------------------------------------------------------------*/
/** \internal
 * Calculate distance between two pointers, in bytes.
 *
 * @param p1                First pointer (smaller value tha p2).
 * @param p2                Second pointer (bigger value than p1).
 *
 * @return Byte distance.
 */

#define __ET9ByteDistance(p1, p2)       ((ET9U32)((ET9U8*)(p1) - (ET9U8*)(p2)))


#ifdef ET9_DEBUGLOG3

static ET9U32 dwAttemptCount;
static ET9U32 dwPassedCount;
static ET9U32 dwRejectedCount;
static ET9U32 dwLimitedCount;
static ET9U32 dwLengthCalcCount;
static ET9U32 dwLengthsCount[1000];
static ET9U32 dwJumpsCount[1000];

#define __IncScreenAttempt()        { ++dwAttemptCount; }
#define __IncScreenPassed()         { ++dwPassedCount; }
#define __IncScreenRejected()       { ++dwRejectedCount; }
#define __IncScreenLimited()        { ++dwLimitedCount; }
#define __IncScreenLengthCalc()     { ++dwLengthCalcCount; }
#define __IncScreenLength(x)        { ++dwLengthsCount[(x) < 1000 ? (x) : 999]; }
#define __IncScreenJump(x)          { ++dwJumpsCount[(x) < 1000 ? (x) : 999]; }

#define __ResetScreenCounters()                                                                         \
{                                                                                                       \
    dwAttemptCount = 0;                                                                                 \
    dwPassedCount = 0;                                                                                  \
    dwRejectedCount = 0;                                                                                \
    dwLimitedCount = 0;                                                                                 \
    dwLengthCalcCount = 0;                                                                              \
    memset(dwLengthsCount, 0, sizeof(dwLengthsCount));                                                  \
    memset(dwJumpsCount, 0, sizeof(dwJumpsCount));                                                      \
}                                                                                                       \

#define __LogScreenCounters()                                                                           \
{                                                                                                       \
    int i;                                                                                              \
    fprintf(pLogFile3,                                                                                  \
            "__LogScreenCounters, Tot %d, pass %d, reject %d, limited %d, lengths %d\n",                \
            dwAttemptCount, dwPassedCount, dwRejectedCount, dwLimitedCount, dwLengthCalcCount);         \
    for (i = 0; i < 1000; ++i) {                                                                        \
        if (dwLengthsCount[i]) {                                                                        \
            fprintf(pLogFile3, "Length run %3d : %5d\n", i, dwLengthsCount[i]);                         \
        }                                                                                               \
    }                                                                                                   \
    for (i = 0; i < 1000; ++i) {                                                                        \
        if (dwJumpsCount[i]) {                                                                          \
            fprintf(pLogFile3, "Executed jump %3d : %5d\n", i, dwJumpsCount[i]);                        \
        }                                                                                               \
    }                                                                                                   \
}                                                                                                       \

#else /* ET9_DEBUGLOG3 */

#define __IncScreenAttempt()
#define __IncScreenPassed()
#define __IncScreenRejected()
#define __IncScreenLimited()
#define __IncScreenLengthCalc()
#define __IncScreenLength(x)
#define __IncScreenJump(x)

#define __LogScreenCounters()
#define __ResetScreenCounters()

#endif /* ET9_DEBUGLOG3 */


#ifdef ET9_DEBUGLOG3

/*---------------------------------------------------------------------------*/
/** \internal
 * Log a word.
 * Debug function to log a word.
 *
 * @param pF                Log file.
 * @param pcComment         Comment to log with the word.
 * @param pWord             The word.
 *
 * @return None
 */

static void ET9LOCALCALL WLOG3Word(FILE *pF, char *pcComment, ET9AWPrivWordInfo *pWord)
{
    ET9U16 wIndex;

    ET9Assert(pF);
    ET9Assert(pcComment);
    ET9Assert(pWord);

    fprintf(pF, "%s: ", pcComment);

    for (wIndex = 0; wIndex < pWord->Base.wWordLen; ++wIndex) {

        ET9SYMB sSymb = pWord->Base.sWord[wIndex];

        if (sSymb >= 0x20 && sSymb <= 0x7F) {
            fprintf(pF, "%c", (char)sSymb);
        }
        else {
            fprintf(pF, "<%x>", (int)sSymb);
        }
    }

    fprintf(pF, "  (%c : %d,%d : %d,%d : %d)\n",
                (char)(pWord->Base.bIsTerm ? 'T' : 'S'),
                pWord->Base.wWordLen,
                pWord->Base.wWordCompLen,
                pWord->bEditDistSpc,
                pWord->bEditDistStem,
                pWord->bWordSrc);

    fflush(pF);
}
#else /* ET9_DEBUGLOG3 */
#define WLOG3Word(pF,pcComment,pWord)
#endif /* ET9_DEBUGLOG3 */

/*---------------------------------------------------------------------------*/
/** \internal
 * Basic validity check.
 * Check for good initializtion setting AND good LDB.
 *
 * @param pLingInfo         Pointer to alphabetic information structure.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

static ET9STATUS ET9LOCALCALL __ET9AWLdbBasicValidityCheck(ET9AWLingInfo * const pLingInfo)
{
    if (pLingInfo == NULL) {
        return ET9STATUS_INVALID_MEMORY;
    }
    if (pLingInfo->Private.wInfoInitOK != ET9GOODSETUP) {
        return ET9STATUS_NO_INIT;
    }
    if (pLingInfo->pLingCmnInfo == NULL) {
        return ET9STATUS_INVALID_MEMORY;
    }
    if (pLingInfo->pLingCmnInfo->Private.wInfoInitOK != ET9GOODSETUP) {
        return ET9STATUS_NO_INIT;
    }
    if (pLingInfo->Private.wLDBInitOK != ET9GOODSETUP) {
        return ET9STATUS_NO_DB_INIT;
    }
    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Read LDB data.
 * Read a given number of individual bytes from LDB.
 *
 * @param pLingInfo                 Pointer to alphabetic information structure.
 * @param dwOffset                  Offset into LDB.
 * @param dwNumberOfBytesToRead     Number of bytes to read.
 * @param pbDst                     Destination buffer pointer.
 *
 * @return ET9STATUS_NONE on success, otherwise ET9STATUS_READ_DB_FAIL.
 */

ET9INLINE static ET9STATUS ET9LOCALCALL __ET9ReadLDBData(ET9AWLingInfo * const    pLingInfo,
                                                         const ET9U32             dwOffset,
                                                         const ET9U32             dwNumberOfBytesToRead,
                                                         ET9U8 * const            pbDst)
{
    ET9Assert(pLingInfo);
    ET9Assert(pbDst);
    ET9Assert(dwNumberOfBytesToRead);

#ifdef ET9_DIRECT_LDB_ACCESS

    _ET9ByteCopy(pbDst, &ALDBL.pLdbData[dwOffset], dwNumberOfBytesToRead);

#else

    {
        ET9U32  dwNumberOfBytesRead;

        ET9Assert(pLingInfo->ET9AWLdbReadData != NULL);

        if (pLingInfo->ET9AWLdbReadData(pLingInfo,
                                        dwOffset,
                                        dwNumberOfBytesToRead,
                                        pbDst,
                                        &dwNumberOfBytesRead) ||
            (dwNumberOfBytesRead != dwNumberOfBytesToRead)) {

            return ET9STATUS_READ_DB_FAIL;
        }
    }

#endif /* ET9_SINGLE_LDB_READ_BUFFER */

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Read LDB word.
 * Read 2 bytes and piece together into endian correct word.
 *
 * @param pLingInfo                 Pointer to alphabetic information structure.
 * @param dwOffset                  Offset into LDB.
 * @param pwData                    Destination word pointer.
 *
 * @return ET9STATUS_NONE on success, otherwise ET9STATUS_READ_DB_FAIL.
 */

ET9INLINE static ET9STATUS ET9LOCALCALL __ET9ReadLDBWord(ET9AWLingInfo * const    pLingInfo,
                                                         const ET9U32             dwOffset,
                                                         ET9U16 * const           pwData)
{
    ET9U8  byWord[2];

    ET9Assert(pLingInfo);
    ET9Assert(pwData);

    if (__ET9ReadLDBData(pLingInfo, dwOffset, 2, byWord)) {
        return ET9STATUS_READ_DB_FAIL;
    }

    *pwData = (ET9U16) ((ET9U16)(byWord[0] << 8) | byWord[1]);

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Read LDB byte.
 * Read 1 byte.
 *
 * @param pLingInfo                 Pointer to alphabetic information structure.
 * @param dwOffset                  Offset into LDB.
 *
 * @return The value.
 */

ET9U8 ET9FARCALL _ET9ReadLDBByte(ET9AWLingInfo * const pLingInfo,
                                 const ET9U32          dwOffset)
{
    ET9U8  bValue;

    ET9Assert(pLingInfo);

    if (__ET9ReadLDBData(pLingInfo, dwOffset, 1, &bValue)) {
        bValue = 0;
    }

    return bValue;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Read a 2 byte number.
 *
 * @param pLingInfo                 Pointer to alphabetic information structure.
 * @param dwOffset                  Offset into LDB.
 *
 * @return The value.
 */

ET9U16 ET9FARCALL _ET9ReadLDBWord2(ET9AWLingInfo * const pLingInfo,
                                   const ET9U32          dwOffset)
{
    ET9U8  byWord[2];
    ET9U16 wValue;

    ET9Assert(pLingInfo);

    if (__ET9ReadLDBData(pLingInfo, dwOffset, 2, byWord)) {
        wValue = 0;
    }
    else {
        wValue = (ET9U16)((ET9U16)(byWord[0] << 8) | byWord[1]);
    }

    return wValue;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Read a 3 byte number.
 *
 * @param pLingInfo                 Pointer to alphabetic information structure.
 * @param dwOffset                  Offset into LDB.
 *
 * @return The value.
 */

ET9U32 ET9FARCALL _ET9ReadLDBWord3(ET9AWLingInfo * const pLingInfo,
                                   const ET9U32          dwOffset)
{
    ET9U8  byWord[3];
    ET9U32 dwValue;

    ET9Assert(pLingInfo);

    if (__ET9ReadLDBData(pLingInfo, dwOffset, 3, byWord)) {
        dwValue = 0;
    }
    else {
        dwValue = (ET9U32)((ET9U32)(byWord[0] << 16) | (ET9U32)(byWord[1] << 8) | byWord[2]);
    }

    return dwValue;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Check if the OEM ID is valid or not.
 *
 * @param pLingInfo                 Pointer to alphabetic information structure.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

static ET9STATUS ET9LOCALCALL __ET9AWLdbCheckOEMID (ET9AWLingInfo * const pLingInfo)
{
    ET9STATUS   wStatus;
    ET9U16      wOEMID = 0;

    ET9Assert(pLingInfo);

    wStatus = __ET9ReadLDBWord(pLingInfo, ET9LDBOFFSET_OEMID, &wOEMID);

    if (!wStatus) {

        /* Compare the computed OEM ID with the one stored in LDB */

        wStatus = (ET9STATUS)((wOEMID == ET9OEMID) ? ET9STATUS_NONE : ET9STATUS_WRONG_OEMID);
    }

    return wStatus;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Check if the LDB is compatible or not.
 *
 * @param pLingInfo                 Pointer to alphabetic information structure.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

static ET9STATUS ET9LOCALCALL __ET9AWLdbCheckCompat (ET9AWLingInfo * const pLingInfo)
{
    ET9STATUS   wStatus;
    ET9U16      wLdbCompatID;
    ET9U16      wLdbCompatOffset;

    ET9Assert(pLingInfo);

    /* Get the compatibility ID from LDB */

    wStatus = __ET9ReadLDBWord(pLingInfo, ET9LDBOFFSET_COMPATID, &wLdbCompatID);
    if (!wStatus) {

        if (wLdbCompatID < ET9COMPATIDLDBXBASEALPH) {
            wStatus = ET9STATUS_DB_CORE_INCOMP;
        }
        else {

            /* Compute LDB Compatibility index offset */

            wLdbCompatOffset = (ET9U16)(wLdbCompatID - ET9COMPATIDLDBXBASEALPH);

            if (wLdbCompatOffset > ET9MAXCOMPATIDLDBXOFFSET) {
                wStatus = ET9STATUS_DB_CORE_INCOMP;
            }
            else {

                /*
                 * If the compat idx from LDB is equal to the compat idx base defined in core,
                 * the offset is 0. Therefore, they are compatible. Otherwise, compare the offset.
                 */

                if (wLdbCompatOffset) {

                    /* Convert the offset to bit mask. */

                    wLdbCompatOffset = ET9MASKOFFSET(wLdbCompatOffset);

                    if (!(ET9COMPATIDLDBXOFFSETALPH & wLdbCompatOffset)) {
                        wStatus = ET9STATUS_DB_CORE_INCOMP;
                    }
                }
            }
        }
    }

    return wStatus;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Hash a memory chunk.
 * Return the checksum of a piece of the LDB.
 *
 * @param pLingInfo                 Pointer to alphabetic information structure.
 * @param pdwHashValue              In/out value with the hash to be updated.
 * @param dwPos                     Byte position of start of data to read.
 * @param dwSize                    # of bytes to read (can be over big for "read to end").
 *
 * @return None
 */

static void ET9LOCALCALL __ET9AWLdbHashChunk(ET9AWLingInfo * const pLingInfo,
                                             ET9U32        * const pdwHashValue,
                                             ET9U32                dwPos,
                                             ET9U32                dwSize)
{
#ifdef ET9_DIRECT_LDB_ACCESS

    ET9U8               *pStr;
    ET9U32              dwCount;
    ET9U32              dwHashValue;

    ET9Assert(pLingInfo);
    ET9Assert(pdwHashValue);
    ET9Assert(dwSize);
    ET9Assert(ALDBL.dwLdbDataSize);
    ET9Assert(ALDBL.pLdbData != NULL);

    dwCount = dwSize;

    if (dwCount >= ALDBL.dwLdbDataSize - dwPos) {
        dwCount = ALDBL.dwLdbDataSize - dwPos;
    }

    /* hash data chunk */

    pStr = &ALDBL.pLdbData[dwPos];

    dwHashValue = *pdwHashValue;

    while (dwCount--) {
        dwHashValue = *(pStr++) + (65599 * dwHashValue);
    }

    *pdwHashValue = dwHashValue;

#else /* ET9_DIRECT_LDB_ACCESS */

    ET9STATUS   wStatus;
    ET9U8       *pStr;
    ET9U32      dwNum;
    ET9U32      dwHashValue;
    ET9U32      dwReadSize;
    ET9U32      dwNumberOfBytesRead;
    ET9U8       byHashLDBBuff[ET9HASHLDBBUFFSIZE];

    ET9Assert(pLingInfo);
    ET9Assert(pdwHashValue);
    ET9Assert(dwSize);

    while (dwSize) {

        if (ET9HASHLDBBUFFSIZE > dwSize) {
            dwReadSize = dwSize;
        }
        else {
            dwReadSize = ET9HASHLDBBUFFSIZE;
        }

        wStatus = pLingInfo->ET9AWLdbReadData(pLingInfo, dwPos, dwReadSize, byHashLDBBuff, &dwNumberOfBytesRead);

        /* Check for end-of-file (or other failure). */

        if (wStatus || !dwNumberOfBytesRead) {
            break;
        }

        /* Hash LDB data */

        pStr = byHashLDBBuff;
        dwNum = dwNumberOfBytesRead;
        dwHashValue = *pdwHashValue;
        while (dwNum--) {
            dwHashValue = *(pStr++) + (65599 * dwHashValue);
        }
        *pdwHashValue = dwHashValue;

        dwSize -= dwNumberOfBytesRead;
        dwPos += dwNumberOfBytesRead;
    }

#endif /* ET9_DIRECT_LDB_ACCESS */
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Convert extended symbol (character) to symbol code (ldb specific value).
 *
 * @param pLingCmnInfo              Pointer to alphabetic cmn information structure.
 * @param sChar                     Symbol to encode.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

static ET9U16 ET9LOCALCALL __ET9ExtendedSymbolToCode(ET9AWLingCmnInfo   * const pLingCmnInfo,
                                                     const ET9SYMB              sChar)
{
    ET9U16  wIndex;
    ET9SYMB *psChar;

    for (wIndex = ALDB.header.wCharacterDecodeCount, psChar = ALDB.header.psCharacterDecodeTable; wIndex; --wIndex, ++psChar) {

        if (*psChar == sChar) {

            return (ET9U16)(ALDB.header.wCharacterDecodeCount - wIndex);
        }
    }

    return ALDB_CHAR_CODE_NONE;
}

/* *************************************************************************
   *********************** COMPARE *****************************************
   ************************************************************************* */

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 */

#define __SetCodeIsFree(wCode)                                                                 \
    ALDB.compare.pbCodeIsFree[wCode >> 3] |= (1 << (wCode & 0x7));

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 */

#define __IsCodeFree(wCode)                                                                    \
    ((ALDB.compare.pbCodeIsFree[wCode >> 3] & (1 << (wCode & 0x7))) != 0)

/*---------------------------------------------------------------------------*/
/** \internal
 * Symbol to LDB specific code.
 *
 * @param pLingCmnInfo              Pointer to alphabetic cmn information structure.
 * @param sSymb                     Symbol to encode (unicode).
 *
 * @return Corresponding code, or ALDB_CHAR_CODE_NONE if it's not in use.
 */

ET9INLINE static ET9U16 ET9LOCALCALL __SymbolToCode(ET9AWLingCmnInfo    * const pLingCmnInfo,
                                                    const ET9SYMB               sSymb)
{
    if (sSymb < ALDB_HEADER_MAX_DIRECT_ENCODE) {

        return ALDB.header.pwCharacterEncodeTable[sSymb];

    }
    else if (!ALDB.header.wCharacterEncodeExtendCount ||
             sSymb < ALDB.header.sCharacterEncodeExtendFirstChar ||
             sSymb > ALDB.header.sCharacterEncodeExtendLastChar) {

        return ALDB_CHAR_CODE_NONE;

    }
    else {

        return __ET9ExtendedSymbolToCode(pLingCmnInfo, sSymb);
    }
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Check if an input position is locked.
 *
 * @param wPos          Input position (0 - n).
 *
 * @return If locked, and corresponding lock value.
 */

#define __GetPosLock(wPos)                                                                              \
    (ALDB.compare.pbLocked[wPos])

/*---------------------------------------------------------------------------*/
/** \internal
 * Set lock for a poition.
 * The lock can be either just symbol value, or both value and position.
 *
 * @param wPos          Input position (0 - n).
 * @param bLock         Lock kind, to set.
 *
 * @return None
 */

#define __SetPosLock(wPos, bLock)                                                                       \
{                                                                                                       \
    if (wPos < ALDB_COMPARE_MAX_POS) {                                                                  \
        ALDB.compare.pbLocked[wPos] = bLock;                                                            \
    }                                                                                                   \
    WLOG3(fprintf(pLogFile3, "__SetPosLock, wPos = %2d, bLock = %s\n",                                  \
                  wPos,                                                                                 \
                  (bLock == ET9_LOCKED_SYMB_NONE ? "NONE" :                                             \
                   bLock == ET9_LOCKED_SYMB_VALUE ? "LOCKED_SYMB_VALUE" :                               \
                   bLock == ET9_LOCKED_SYMB_VALUE_AND_POS ? "LOCKED_SYMB_VALUE_AND_POS" :               \
                   "\?\?\?"));)                                                                         \
}                                                                                                       \

/*---------------------------------------------------------------------------*/
/** \internal
 * Classic compare.
 * This is used for a classic compare of input against a word.
 *
 * @param wPos          Input position (0 - n).
 * @param wCode         Code to check.
 *
 * @return Non zero if active, otherwise zero.
 */

#define __IsCodeActive(wPos, wCode)                                                                     \
    (ALDB.compare.ppbActive[wPos][wCode >> 3] & (1 << (wCode & 0x7)))

/*---------------------------------------------------------------------------*/
/** \internal
 * Get a compare section, for a length dependent spell correction compare.
 * The index is zero for the shortest possible length, and then increases.
 * E.g. for input length 9 and 3 edits a word of length 6 gets index 0.
 *
 * @param bLengthIndex      The index of the possible word lengths.
 *
 * @return Pointer to compare section.
 */

#define __GetActiveSectionSpc(bLengthIndex)                                                             \
    (&ASPC.u.sCmpData.pppbActiveSpc[bLengthIndex])

/*---------------------------------------------------------------------------*/
/** \internal
 * Get a compare section, for flex kind (regional exact).
 *
 * @param bIndexKind        The index of the possible word lengths.
 *
 * @return Pointer to compare section.
 */

#define __GetActiveSectionFlexSpc(bIndexKind)                                                           \
    (&ASPC.u.sCmpDataFlex.pppbActiveSpc[bIndexKind])

/*---------------------------------------------------------------------------*/
/** \internal
 * Spell correction compare.
 * This is used for a set based spell correction compare of input against a word.
 *
 * @param ppbActiveSpc  Length controlled section.
 * @param wPos          Input position (0 - n).
 * @param wCode         Code to check.
 *
 * @return Non zero if active, otherwise zero.
 */

#define __IsCodeActiveSpc(ppbActiveSpc, wPos, wCode)                                                    \
    ((*ppbActiveSpc)[wPos][wCode >> 3] & (1 << (wCode & 0x7)))

/*---------------------------------------------------------------------------*/
/** \internal
 * Exact compare.
 * This can be used to check if a classic match is exact or not.
 *
 * @param wPos          Input position (0 - n).
 * @param wCode         Code to check.
 *
 * @return Non zero if exact, otherwise zero.
 */

#define __IsCodeExact(wPos, wCode)                                                                      \
    (ALDB.compare.ppbExact[wPos][wCode >> 3] & (1 << (wCode & 0x7)))

/*---------------------------------------------------------------------------*/
/** \internal
 * Activate symbol.
 * This is used to activate a symbol for classic compare (based on the input).
 *
 * @param wPos          Input position (0 - n).
 * @param wSymbol       Symbol to activate (unicode, non zero).
 * @param bSymFreq      Tap frequency (non zero).
 * @param bExact        If exact or not.
 *
 * @return None
 */

#define __ActivateSymbol(wPos, wSymbol, bSymFreq, bExact)                                               \
{                                                                                                       \
    if (wPos < ALDB_COMPARE_MAX_POS) {                                                                  \
        const ET9U16 wCode = (ET9U16)__SymbolToCode(pLingCmnInfo, wSymbol);                             \
        if (wCode != ALDB_CHAR_CODE_NONE) {                                                             \
            ALDB.compare.ppbActive[wPos][wCode >> 3] |= (1 << (wCode & 0x7));                           \
            if (bExact) {                                                                               \
                ALDB.compare.ppbExact[wPos][wCode >> 3] |= (1 << (wCode & 0x7));                        \
            }                                                                                           \
            if (wPos < ALDB_COMPARE_MAX_FREQ_POS) {                                                     \
                ALDB.compare.ppbFreq[wPos][wCode] = bSymFreq;                                           \
            }                                                                                           \
        }                                                                                               \
    }                                                                                                   \
    WLOG3(fprintf(pLogFile3,                                                                            \
          "__ActivateSymbol, wPos = %2d, wSymbol = %4x, bSymFreq = %3d, bExact = %d\n",                 \
          wPos, wSymbol, bSymFreq, bExact);)                                                            \
    ET9Assert((wSymbol || wPos == ALDB.compare.wLength) && bSymFreq);                                   \
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Deactivate symbol.
 * This is used to deactivate a symbol during a classic compare.<br>
 * The feature only exist for a very special case, speeding up search by limiting the set during the search.
 *
 * @param wPos          Input position (0 - n).
 * @param wCode         Code to deactivate.
 *
 * @return None
 */

#define __DeactivateCode(wPos, wCode)                                                                   \
{                                                                                                       \
    if (wPos < ALDB_COMPARE_MAX_POS) {                                                                  \
        if (wCode != ALDB_CHAR_CODE_NONE) {                                                             \
            ALDB.compare.ppbActive[wPos][wCode >> 3] &= ~(1 << (wCode & 0x7));                          \
            ALDB.compare.ppbExact[wPos][wCode >> 3] &= ~(1 << (wCode & 0x7));                           \
            if (wPos < ALDB_COMPARE_MAX_FREQ_POS) {                                                     \
                ALDB.compare.ppbFreq[wPos][wCode] = 0;                                                  \
            }                                                                                           \
        }                                                                                               \
    }                                                                                                   \
    WLOG3(fprintf(pLogFile3, "__DeactivateSymbol, wPos = %2d, wCode = %d\n", wPos, wCode);)             \
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Get code frequency.
 * This can be used to get the tap frequency for a code.
 * Only the beginning of a word is used for the tap frequency calculation, thus only maintained for that part.
 *
 * @param wPos          Input position (0 - n).
 * @param wCode         Code to get tap frequency for.
 *
 * @return Tap frequency.
 */

#define __GetCodeFreq(wPos, wCode)                                                                      \
(                                                                                                       \
    (wPos < ALDB_COMPARE_MAX_FREQ_POS)                                                                  \
    ?                                                                                                   \
    (ALDB.compare.ppbFreq[wPos][wCode])                                                                 \
    :                                                                                                   \
    (__IsCodeActive(wPos, wCode) ? 1 : 0)                                                               \
)

/*---------------------------------------------------------------------------*/
/** \internal
 * Checks if a symbol is used in the LDB.
 *
 * @param pLingInfo                 Pointer to alphabetic information structure.
 * @param wLdbNum                   Language id.
 * @param sSymb                     Symbol to check (unicode).
 *
 * @return Non zero if used, otherwise zero.
 */

ET9BOOL ET9FARCALL _ET9AWLdbIsSymbolUsed (ET9AWLingInfo * const pLingInfo,
                                          const ET9U16          wLdbNum,
                                          const ET9SYMB         sSymb)
{
    ET9AWLingCmnInfo * const pLingCmnInfo = pLingInfo->pLingCmnInfo;

    ET9U16 wSavedLDBNum = 0;
    ET9BOOL bResult;

    /* pretend that digits always are in... */

    if (sSymb >= '0' && sSymb <= '9') {
        return 1;
    }

    /* assure cache */

    if (wLdbNum != pLingCmnInfo->wLdbNum) {
        wSavedLDBNum = pLingCmnInfo->wLdbNum;
        _ET9AWLdbSetActiveLanguage(pLingInfo, wLdbNum);
    }

    /* actual LDB check */

    if (pLingInfo->Private.wLDBInitOK == ET9GOODSETUP && ET9LDBENABLED(pLingCmnInfo)) {

        const ET9U16 wCode = __SymbolToCode(pLingCmnInfo, sSymb);

        bResult = (wCode == ALDB_CHAR_CODE_NONE) ? 0 : 1;

        if (wSavedLDBNum) {
            _ET9AWLdbSetActiveLanguage(pLingInfo, wSavedLDBNum);
        }

        return bResult;
    }
    else {

        if (wSavedLDBNum) {
            _ET9AWLdbSetActiveLanguage(pLingInfo, wSavedLDBNum);
        }

        /* some useful support for when there is no LDB activated */

        if (sSymb >= 'a' && sSymb <= 'z') {
            return 1;
        }
        else {
            return 0;
        }
    }
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Activate symbols in a specific position in the selection list.
 *
 * @param pLingCmnInfo              Pointer to alphabetic cmn information structure.
 * @param wIndex                    Selection list index/position (might not start from zero).
 * @param pbLocked                  If locked or not (explicit input lock).
 * @param wPos                      Compare position to activate (0 - n).
 *
 * @return None
 */

static void ET9LOCALCALL __ET9CompareActivatePos (ET9AWLingCmnInfo  * const pLingCmnInfo,
                                                  const ET9U16              wIndex,
                                                  ET9U8 * const             pbLocked,
                                                  const ET9U16              wPos)
{
    ET9U8 bBaseIndex;

    ET9SymbInfo * const pSymbInfo = &pLingCmnInfo->Base.pWordSymbInfo->SymbsInfo[wIndex];

    WLOG3(fprintf(pLogFile3, "__ET9CompareActivatePos, wIndex = %2d, wPos = %2d\n", wIndex, wPos);)

    *pbLocked |= pSymbInfo->bLocked;

    if (*pbLocked) {

        const ET9U8 bExact = 1;
        const ET9SYMB sLockedSymb = pSymbInfo->sLockedSymb;

        __SetPosLock(wPos, ET9_LOCKED_SYMB_VALUE_AND_POS);
        __ActivateSymbol(wPos, sLockedSymb, 1, bExact);
        __ActivateSymbol(wPos, _ET9SymToOther(sLockedSymb, pLingCmnInfo->wLdbNum), 1, bExact);
    }
    else {

        ET9DataPerBaseSym *pDPBS;

        pDPBS = pSymbInfo->DataPerBaseSym;
        for (bBaseIndex = 0; bBaseIndex < pSymbInfo->bNumBaseSyms; ++bBaseIndex, ++pDPBS) {

            ET9U8 bExact;
            ET9U8 bSymFreq;
            ET9U8 bSymsIndex;
            ET9SYMB *pLower;
            ET9SYMB *pUpper;

            if (pSymbInfo->eInputType == ET9DISCRETEKEY ||
                pSymbInfo->eInputType == ET9MULTITAPKEY ||
                pSymbInfo->eInputType == ET9CUSTOMSET ||
                pSymbInfo->bSymbType == ET9KTSMARTPUNCT) {

                bExact = 1;
            }
            else {

                bExact = !bBaseIndex;

                if (bBaseIndex >= pLingCmnInfo->Private.bCurrMaxRegionality) {
                    WLOG3(fprintf(pLogFile3, "    bCurrMaxRegionality %u - skip\n", pLingCmnInfo->Private.bCurrMaxRegionality);)
                    break;
                }
            }

            bSymFreq = pDPBS->bSymFreq;

            if (!bSymFreq) {
                bSymFreq = 1;
            }

            pLower = pDPBS->sChar;
            pUpper = pDPBS->sUpperCaseChar;

            for (bSymsIndex = 0; bSymsIndex < pDPBS->bNumSymsToMatch; ++bSymsIndex, ++pLower, ++pUpper) {

                __ActivateSymbol(wPos, *pLower, bSymFreq, bExact);
                __ActivateSymbol(wPos, *pUpper, bSymFreq, bExact);
            }
        }

        __SetPosLock(wPos, ET9_SPC_IS_LOCKED_POS(pSymbInfo));
    }
}

#ifdef ET9ASPC_DEV_SCREEN

/*---------------------------------------------------------------------------*/
/** \internal
 * Activate quality point.
 *
 * @param pLingCmnInfo              Pointer to alphabetic cmn information structure.
 * @param nSymbIndex                .
 * @param nQualityIndex             .
 *
 * @return None
 */

static void ET9LOCALCALL __ET9CompareActivateQualityPos (ET9AWLingCmnInfo  * const pLingCmnInfo,
                                                         const ET9UINT             nSymbIndex,
                                                         const ET9UINT             nQualityIndex)
{
    ET9U8               wBaseIndex;
    ET9DataPerBaseSym   *pDPBS;

    ET9SymbInfo * const pSymbInfo           = &pLingCmnInfo->Base.pWordSymbInfo->SymbsInfo[nSymbIndex];
    ET9ASPCFlexCompareData  * const pCMP    = &pLingCmnInfo->Private.ASpc.u.sCmpDataFlex;

    WLOG3(fprintf(pLogFile3, "__ET9CompareActivateQualityPos, nSymbIndex = %2d, nQualityIndex = %2d\n", nSymbIndex, nQualityIndex);)

    pDPBS = pSymbInfo->DataPerBaseSym;
    for (wBaseIndex = 0; wBaseIndex < pSymbInfo->bNumBaseSyms; ++wBaseIndex, ++pDPBS) {

        ET9U8   wSymsIndex;

        ET9SYMB *pLower = pDPBS->sChar;
        ET9SYMB *pUpper = pDPBS->sUpperCaseChar;

        for (wSymsIndex = 0; wSymsIndex < pDPBS->bNumSymsToMatch; ++wSymsIndex, ++pLower, ++pUpper) {
            {
                const ET9UINT nCode = __SymbolToCode(pLingCmnInfo, *pLower);

                if (nCode != ALDB_CHAR_CODE_NONE) {

                    ET9ASPCSupportRelations * const pRelations = &pCMP->pCharCodeRelations[nCode];

                    if (!pRelations->bCount || pRelations->pbIndex[pRelations->bCount - 1] != nQualityIndex) {

                        WLOG3(fprintf(pLogFile3, "  + nCode %2u (%c)\n", nCode, (char)ALDB.header.psCharacterDecodeTable[nCode]);)

                        pRelations->pbIndex[pRelations->bCount++] = (ET9U8)nQualityIndex;
                    }
                    else {
                        WLOG3(fprintf(pLogFile3, "  x nCode %2u (%c)\n", nCode, (char)ALDB.header.psCharacterDecodeTable[nCode]);)
                    }
                }
            }
            {
                const ET9UINT nCode = __SymbolToCode(pLingCmnInfo, *pUpper);

                if (nCode != ALDB_CHAR_CODE_NONE) {

                    ET9ASPCSupportRelations * const pRelations = &pCMP->pCharCodeRelations[nCode];

                    if (!pRelations->bCount || pRelations->pbIndex[pRelations->bCount - 1] != nQualityIndex) {

                        WLOG3(fprintf(pLogFile3, "  + nCode %2u (%c)\n", nCode, (char)ALDB.header.psCharacterDecodeTable[nCode]);)

                        pRelations->pbIndex[pRelations->bCount++] = (ET9U8)nQualityIndex;
                    }
                    else {
                        WLOG3(fprintf(pLogFile3, "  x nCode %2u (%c)\n", nCode, (char)ALDB.header.psCharacterDecodeTable[nCode]);)
                    }
                }
            }
        }
    }
}

#endif

/*---------------------------------------------------------------------------*/
/** \internal
 * Activate symbols in a specific selection list sequence ("word").
 *
 * @param pLingCmnInfo              Pointer to alphabetic cmn information structure.
 * @param wIndex                    Selection list index/position (might not start from zero).
 * @param wLength                   Sequence/word length.
 *
 * @return None
 */

static void ET9LOCALCALL __ET9CompareActivateList (ET9AWLingCmnInfo     * const pLingCmnInfo,
                                                   const ET9U16                 wIndex,
                                                   const ET9U16                 wLength)
{
    ET9U8 bLocked = 0;
    ET9U16 wCmpIndex;

    ET9Assert(pLingCmnInfo != NULL);

    WLOG3(fprintf(pLogFile3, "__ET9CompareActivateList, wIndex = %2d, wLength = %2d\n", wIndex, wLength);)

    if (!wLength) {
        return;
    }

    for (wCmpIndex = (ET9U16)(wLength - 1); ; --wCmpIndex) {

        __ET9CompareActivatePos(pLingCmnInfo, (ET9U16)(wCmpIndex + wIndex), &bLocked, wCmpIndex);

        if (!wCmpIndex) {
            break;
        }
    }
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Calculate and activate compare sets for spell correction screening (std).
 * A different set of sets is calculated for each possible spell correction word length.<br>
 * In short, unions of classic input positions are built up for quick screening before
 * sending words for edit distance handling.<br>
 * During the calculation checks will also be made to see if "exact" is significant for the compare or not.
 * If it's significant the compare becomes more complex.
 *
 * @param pLingCmnInfo              Pointer to alphabetic cmn information structure.
 * @param bSpcMode                  Spell correction mode.
 * @param pbExactCompare            (out) if the exact concept is active or not.
 *
 * @return None
 */

static void ET9LOCALCALL __ET9CompareActivateSpcInfoStd (ET9AWLingCmnInfo   * const pLingCmnInfo,
                                                         const ET9U8                bSpcMode,
                                                         ET9BOOL           * const  pbExactCompare)
{
    const ET9BOOL bExactSpcMode         = (ET9U8)(ET9_SPC_GET_MODE(bSpcMode) == ET9ASPCMODE_EXACT);
    const ET9BOOL bSpcFilterOne         = ET9_SPC_SEARCH_FILTER_IS_ONE(ASPC.eSearchFilter);
    const ET9BOOL bSpcFilterTwo         = ET9_SPC_SEARCH_FILTER_IS_TWO(ASPC.eSearchFilter);
    const ET9BOOL bSearchFilterExact    = ET9_SPC_SEARCH_FILTER_IS_EXACT(ASPC.eSearchFilter);

    const ET9U16 wInputLength           = ALDB.compare.wLength;
    const ET9S16 swMaxDist              = (ET9S16)pLingCmnInfo->Private.bCurrMaxEditDistance;
    const ET9U16 wDistLengths           = swMaxDist * 2 + 1;

    ET9U16 wLengthIndex;
    ET9BOOL bExactIsActive = 0;

    if (!ALDB.compare.bSpcActive) {
        *pbExactCompare = bExactIsActive;
        return;
    }

    ALDB.compare.bSpcMaxEdits = (ET9U8)swMaxDist;
    ALDB.compare.bSpcLengthOffset = (ET9U8)pLingCmnInfo->Private.wCurrMinSourceLength;

    ALDB.compare.bPosLo = (ET9U8)(ALDB.compare.wLength - swMaxDist);
    ALDB.compare.bPosHi = (ET9U8)(ALDB.compare.wLength + swMaxDist - 1);

    if (ALDB.compare.bPosHi >= ALDB.header.bPosCount) {
        ALDB.compare.bPosHi = (ET9U8)(ALDB.header.bPosCount - 1);
    }

    WLOG3(fprintf(pLogFile3, "__ET9CompareActivateSpcPosInfo, wInputLength = %d, wDistLengths = %d\n", wInputLength, wDistLengths);)

    _ET9ClearMem((ET9U8*)ASPC.u.sCmpData.pppbActiveSpc, sizeof(ASPC.u.sCmpData.pppbActiveSpc));

    for (wLengthIndex = 0; wLengthIndex < wDistLengths; ++wLengthIndex) {

        ET9S16 swCmpLen;
        ET9S16 swStartPos;
        ET9S16 swCmpWidth;
        ET9S16 swBaseWidth;
        ET9S16 swCurrStartPos;
        ET9S16 swIndex;
        ET9S16 swPosIndex;

        swBaseWidth = (wLengthIndex + 1) / 2;
        swStartPos = - swBaseWidth;
        swCmpWidth = (wLengthIndex % 2) + swMaxDist + 1;
        swCurrStartPos = swStartPos;

        swCmpLen = wInputLength + wLengthIndex - swMaxDist;

        if (swCmpLen > ET9MAXLDBWORDSIZE) {
            swCmpLen = ET9MAXLDBWORDSIZE;
        }

        /* don't fuzz with array pointers */

        for (swPosIndex = 0; swPosIndex < swCmpLen; ++swPosIndex, ++swCurrStartPos) {

            for (swIndex = 0; swIndex < swCmpWidth; ++swIndex) {

                ET9S16 swPos = swCurrStartPos + swIndex;

                if (__GetPosLock(swPosIndex) == ET9_LOCKED_SYMB_VALUE_AND_POS && swPos != swPosIndex) {
                    continue;
                }
                else if (swPos < 0) {
                    continue;
                }
                else if (swPos >= wInputLength) {
                    break;
                }
                else if (!swPosIndex && bSpcFilterOne && swPos > 0 && !wLengthIndex) {
                    break;
                }
                else if (!swPosIndex && bSpcFilterTwo && swPos > 1 && !wLengthIndex) {
                    break;
                }
                else if (((swMaxDist == 2 && wLengthIndex == 1) ||
                          (swMaxDist == 3 && (wLengthIndex == 1 || wLengthIndex == 3))) &&
                         swPosIndex + 1 == swCmpLen && !swIndex) {
                    continue;
                }
                else if (((swMaxDist == 2 && wLengthIndex == 3) ||
                          (swMaxDist == 3 && (wLengthIndex == 3 || wLengthIndex == 5))) &&
                         swPos + 1 == wInputLength && swIndex + 1 == swCmpWidth) {
                    continue;
                }

                {
                    ET9U16 wCount;
                    ET9BOOL bAddExact;

                    ET9U8 *pbExact = ALDB.compare.ppbExact[swPos];
                    ET9U8 *pbActive = ALDB.compare.ppbActive[swPos];
                    ET9U8 *pbActiveSpc = ASPC.u.sCmpData.pppbActiveSpc[wLengthIndex][swPosIndex];

                    ET9Assert(wLengthIndex <= 7);
                    ET9Assert(swPos >= 0 && swPos < wInputLength);
                    ET9Assert(swPosIndex >= 0 && swPosIndex < swCmpLen);

                    if (!swPosIndex && !wLengthIndex && (bSpcFilterOne || bSpcFilterTwo)) {
                        bAddExact = bSearchFilterExact;
                    }
                    else {
                        bAddExact = bExactSpcMode;
                    }

                    for (wCount = ALDB_COMPARE_MAX_CODE_BYTES; wCount; --wCount, ++pbExact, ++pbActive, ++pbActiveSpc) {

                        if (bAddExact) {

                            *pbActiveSpc |= *pbExact;

                            if (*pbExact != *pbActive) {
                                bExactIsActive = 1;
                            }
                        }
                        else {
                            *pbActiveSpc |= *pbActive;
                        }
                    }
                }
            }
        }

        WLOG3({
            ET9U16 wCodeIndex;

            fprintf(pLogFile3, "wLengthIndex = %d (bExactIsActive = %d)\n", wLengthIndex, bExactIsActive);

            for (swPosIndex = 0; swPosIndex < swCmpLen; ++swPosIndex) {

                fprintf(pLogFile3, "..pos %02d: ", swPosIndex);

                for (wCodeIndex = 0; wCodeIndex < ALDB.header.wCharacterDecodeCount; ++wCodeIndex) {

                    if (__IsCodeActiveSpc(__GetActiveSectionSpc(wLengthIndex), swPosIndex, wCodeIndex)) {

                        ET9SYMB sSymb = ALDB.header.psCharacterDecodeTable[wCodeIndex];

                        if (sSymb >= 0x20 && sSymb <= 0x7F) {
                            fprintf(pLogFile3, "%c", (char)sSymb);
                        }
                        else {
                            fprintf(pLogFile3, "<%x>", sSymb);
                        }
                    }
                }

                fprintf(pLogFile3, "\n");
            }
        })
    }

    *pbExactCompare = bExactIsActive;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Calculate and activate compare sets for spell correction screening (flex).
 *
 * @param pLingCmnInfo              Pointer to alphabetic cmn information structure.
 * @param nInputOffset              Input index/position (non zero if partial).
 *
 * @return None
 */

static void ET9LOCALCALL __ET9CompareActivateSpcInfoFlex (ET9AWLingCmnInfo   * const pLingCmnInfo,
                                                          const ET9UINT              nInputOffset)
{
    const ET9U16 wPos = 0;

    ET9ALdbSymbPosInfo * const pRegSection = __GetActiveSectionFlexSpc(0);
    ET9ALdbSymbPosInfo * const pExtSection = __GetActiveSectionFlexSpc(1);

    ET9U16 wCode;

    _ET9ClearMem((ET9U8*)ASPC.u.sCmpDataFlex.pppbActiveSpc, sizeof(ASPC.u.sCmpDataFlex.pppbActiveSpc));

    _ET9ByteCopy((ET9U8*)pRegSection, (ET9U8*)ALDB.compare.ppbActive, sizeof(ET9ALdbSymbPosInfo));
    _ET9ByteCopy((ET9U8*)pExtSection, (ET9U8*)ALDB.compare.ppbExact, sizeof(ET9ALdbSymbPosInfo));

    for (wCode = 0; wCode < ALDB.header.wCharacterDecodeCount; ++wCode) {

        if (__IsCodeFree(wCode)) {
            (*pRegSection)[wPos][wCode >> 3] |= (1 << (wCode & 0x7));
            (*pExtSection)[wPos][wCode >> 3] |= (1 << (wCode & 0x7));
        }
    }

#ifdef ET9ASPC_DEV_SCREEN
    {
        ET9ASPCFlexCompareData  * const pCMP    = &pLingCmnInfo->Private.ASpc.u.sCmpDataFlex;

        const ET9UINT nInputLength              = ALDB.compare.wLength;
        ET9U8 * const pbIsQualityKey            = pCMP->pbIsQualityKey;

        const ET9UINT nActiveQualityCount = pCMP->nQualityCount - 1;

        ET9UINT nIndex;
        ET9UINT nActiveIndex;

        ET9Assert(pCMP->nQualityCount);

        pCMP->nActiveQualityCount = nActiveQualityCount;
        pCMP->nSupportedQualityCount = 0;

        for (nIndex = 0; nIndex < ALDB_HEADER_MAX_CHAR_CODES; ++nIndex) {
            pCMP->pbCharCounters[nIndex] = 0;
            pCMP->pCharCodeRelations[nIndex].bCount = 0;
        }

        nActiveIndex = 0;
        for (nIndex = 1; nIndex < nInputLength; ++nIndex) {

            if (!pbIsQualityKey[nIndex + 1]) {
                continue;
            }

            pCMP->pbQualityCounters[nActiveIndex] = 0;

            __ET9CompareActivateQualityPos(pLingCmnInfo, nIndex + nInputOffset, nActiveIndex);

            ++nActiveIndex;
        }

        ET9Assert(nActiveIndex == nActiveQualityCount);
    }
#else
    ET9_UNUSED(nInputOffset);
#endif
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Start a new compare using input information (symbols).
 *
 * @param pLingCmnInfo              Pointer to alphabetic cmn information structure.
 * @param wIndex                    Selection list index/position (might not start from zero).
 * @param wLength                   Sequence/word length.
 * @param bUsingFlex                If targeting flex compare.
 *
 * @return None
 */

static void ET9LOCALCALL __ET9CompareStart (ET9AWLingCmnInfo    * const pLingCmnInfo,
                                            const ET9U16                wIndex,
                                            ET9U16                      wLength,
                                            const ET9BOOL               bUsingFlex)
{
    const ET9U8 bSpcMode = pLingCmnInfo->Private.bCurrSpcMode;

    const ET9U16 wMaxWordLength = __ET9Min(pLingCmnInfo->Private.wMaxWordLength, ET9MAXLDBWORDSIZE);

    const ET9U16 wUsefulLength = (wLength <= ALDB_COMPARE_MAX_POS) ? wLength : ALDB_COMPARE_MAX_POS;

    ET9Assert(pLingCmnInfo != NULL);

    WLOG3(fprintf(pLogFile3, "__ET9CompareStart, wIndex = %2d, wLength = %2d, wUsefulLength = %2d, bSpcMode = %d\n", wIndex, wLength, wUsefulLength, bSpcMode);)

    ALDB.compare.wLength                = wUsefulLength;
    ALDB.compare.bSpcActive             = ET9_SPC_IS_ACTIVE(bSpcMode);
    ALDB.compare.bFirstPosSetOpt        = (wMaxWordLength == 1) ? 1 : 0;

    ALDB.compare.bSpcFilteredCompare    = ALDB.compare.bSpcActive && !ET9_SPC_SEARCH_FILTER_IS_UNFILTERED(ASPC.eSearchFilter);
    ALDB.compare.bSpcExactFilter        = ALDB.compare.bSpcActive && ET9_SPC_SEARCH_FILTER_IS_EXACT(ASPC.eSearchFilter);
    ALDB.compare.bSpcExactFilterTrace   = ALDB.compare.bSpcActive && ET9_SPC_SEARCH_FILTER_IS_EXACT(ASPC.eSearchFilterTrace);

    ALDB.compare.bSpcExactPrunedTrace   = 0;
    ALDB.compare.pppbSpcFlexSection     = __GetActiveSectionFlexSpc(0);

    WLOG3(fprintf(pLogFile3, "..wMaxWordLength = %2d, bFirstPosSetOpt = %d\n", wMaxWordLength, ALDB.compare.bFirstPosSetOpt);)

    /* clear all compare info, leave completion positions "matching" (to prevent length checking) */

    _ET9ClearMem((ET9U8*)ALDB.compare.ppbActive, __ET9ByteDistance(&ALDB.compare.ppbActive[wUsefulLength], ALDB.compare.ppbActive));

    if (wUsefulLength < ALDB_COMPARE_MAX_POS) {

        _ET9ByteSet((ET9U8*)&ALDB.compare.ppbActive[wUsefulLength],
                     __ET9ByteDistance(&ALDB.compare.ppbActive[ALDB_COMPARE_MAX_POS], &ALDB.compare.ppbActive[wUsefulLength]),
                     0xFF
                     );
    }

    _ET9ClearMem((ET9U8*)ALDB.compare.ppbExact, sizeof(ALDB.compare.ppbExact));
    _ET9ClearMem((ET9U8*)ALDB.compare.pbLocked, sizeof(ALDB.compare.pbLocked));
    _ET9ClearMem((ET9U8*)ALDB.compare.ppbFreq, sizeof(ALDB.compare.ppbFreq));

    __ET9CompareActivateList(pLingCmnInfo, wIndex, wUsefulLength);

    ALDB.compare.bActiveCmpLength = (ET9U8)wUsefulLength;

    if (bUsingFlex) {
        __ET9CompareActivateSpcInfoFlex(pLingCmnInfo, wIndex);
    }
    else {
        __ET9CompareActivateSpcInfoStd(pLingCmnInfo, bSpcMode, &ALDB.compare.bSpcExactCompare);
    }
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Start a new compare using information from a word.
 * This used to check if a word is in the LDB or not.
 *
 * @param pLingInfo                 Pointer to alphabetic information structure.
 * @param pWord                     Pointer to the word.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

static ET9STATUS ET9LOCALCALL __ET9CompareStartWord (ET9AWLingInfo * const       pLingInfo,
                                                     ET9AWPrivWordInfo * const   pWord)
{
    ET9AWLingCmnInfo * const pLingCmnInfo = pLingInfo->pLingCmnInfo;

    ET9U16   wIndex;
    ET9SYMB *pSymb;

    ET9Assert(pLingInfo != NULL);
    ET9Assert(pWord);


    WLOG3(fprintf(pLogFile3, "__ET9CompareStartWord\n");)

    /* setup things first - needed if aborting early */

    ALDB.compare.wLength = pWord->Base.wWordLen;

    if (ALDB.compare.wLength > ET9MAXLDBWORDSIZE) {
        ALDB.compare.wLength = ET9MAXLDBWORDSIZE;
    }

    ALDB.compare.bSpcActive = 0;
    ALDB.compare.bFirstPosSetOpt = 0;

    ALDB.compare.bActiveCmpLength = (ET9U8)ALDB.compare.wLength;

    if (ALDB.compare.wLength < ALDB.header.bPosCount && pWord->Base.wWordLen) {
        ++ALDB.compare.bActiveCmpLength;
    }

    /* valid compare? */

    if (pWord->Base.wWordLen > ALDB.header.bPosCount) {
        WLOG3(fprintf(pLogFile3, "__ET9CompareStartWord, too long word!\n");)
        return ET9STATUS_ERROR;
    }

    if (pWord->Base.wWordCompLen != 0) {
        WLOG3(fprintf(pLogFile3, "__ET9CompareStartWord, bad completion length\n");)
        return ET9STATUS_ERROR;
    }

    /* init compare info */

    _ET9ClearMem((ET9U8*)ALDB.compare.ppbActive, __ET9ByteDistance(&ALDB.compare.ppbActive[ALDB.compare.bActiveCmpLength], ALDB.compare.ppbActive));

    if (ALDB.compare.bActiveCmpLength < ALDB_COMPARE_MAX_POS) {

        _ET9ByteSet((ET9U8*)&ALDB.compare.ppbActive[ALDB.compare.bActiveCmpLength],
                     __ET9ByteDistance(&ALDB.compare.ppbActive[ALDB_COMPARE_MAX_POS], &ALDB.compare.ppbActive[ALDB.compare.bActiveCmpLength]),
                     0xFF
                     );
    }

    _ET9ClearMem((ET9U8*)ALDB.compare.ppbExact, sizeof(ALDB.compare.ppbExact));
    _ET9ClearMem((ET9U8*)ALDB.compare.pbLocked, sizeof(ALDB.compare.pbLocked));
    _ET9ClearMem((ET9U8*)ALDB.compare.ppbFreq, sizeof(ALDB.compare.ppbFreq));

    /* get the symbols */

    pSymb = pWord->Base.sWord;
    for (wIndex = 0; wIndex < pWord->Base.wWordLen; ++wIndex, ++pSymb) {

        const ET9U8 bExact = 1;

        if (pLingInfo->Private.pConvertSymb != NULL) {

            ET9STATUS   wStatus;
            ET9SYMB     sConvertSymb;
            ET9SYMB     sSymbOther = _ET9SymToOther(*pSymb, pLingCmnInfo->wLdbNum);
            ET9U16      wDecodeCount = ALDB.header.wCharacterDecodeCount;
            ET9SYMB     *pDecodeSymb = ALDB.header.psCharacterDecodeTable;

            WLOG3(fprintf(pLogFile3, "__ET9CompareStartWord, pos %02d, word symb %04x (%04x)\n", wIndex, *pSymb, sSymbOther);)

            for (; wDecodeCount; --wDecodeCount, ++pDecodeSymb) {

                sConvertSymb = *pDecodeSymb;

                if (__IsPrivateSymbol(sConvertSymb)) {
                    continue;
                }

                wStatus = pLingInfo->Private.pConvertSymb(pLingInfo->Private.pConvertSymbInfo, &sConvertSymb);

                if (!wStatus && (sConvertSymb == *pSymb || sConvertSymb == sSymbOther)) {

                    __ActivateSymbol(wIndex, *pDecodeSymb, 1, bExact);
                    __ActivateSymbol(wIndex, _ET9SymToOther(*pDecodeSymb, pLingCmnInfo->wLdbNum), 1, bExact);
                }
            }
        }
        else {
            __ActivateSymbol(wIndex, *pSymb, 1, bExact);
            __ActivateSymbol(wIndex, _ET9SymToOther(*pSymb, pLingCmnInfo->wLdbNum), 1, bExact);
        }
    }

    /* termination */

    if (ALDB.compare.wLength < ALDB.header.bPosCount && pWord->Base.wWordLen) {
        const ET9U8 bExact = 1;
        __ActivateSymbol(ALDB.compare.wLength, 0, 1, bExact);
    }

    return ET9STATUS_NONE;
}

/* *************************************************************************
   *********************** SEARCH ******************************************
   ************************************************************************* */

/*---------------------------------------------------------------------------*/
/** \internal
 * Get the character code for a position in the search "word".
 * This exist in order to make the code a bit more LDB format independent.
 *
 * @param wPos          Input position (0 - n).
 *
 * @return Code value.
 */

#define __LdbGetCharCodeAtPos(wPos)     (ALDB.pCursors[wPos].wCode)


#ifdef ET9_DIRECT_LDB_ACCESS

#define ET9FARDATA_LDB                  ET9FARDATA

#define __InitDirectAccess()            (pLingInfo->ET9AWLdbReadData(pLingInfo, &(ALDBL.pLdbData), &(ALDBL.dwLdbDataSize)))

#define __LdbGetByte(pCursor)           (*pbCurrData)

#define __LdbGetIndex(pCursor)          ((ET9U32)(pbCurrData - ALDBL.pLdbData))

#define __LdbSetIndex(pCursor,index)    pbCurrData = (index) + ALDBL.pLdbData;

#define __LdbIncIndex(pCursor)          ++pbCurrData;

#define __LdbDecIndex(pCursor)          --pbCurrData;

#else /* ET9_DIRECT_LDB_ACCESS */

#define ET9FARDATA_LDB

#define __InitDirectAccess()    (ET9STATUS_NONE)

#define __LdbGetByte(pCursor)   (*pbCurrData)

#define __LdbGetIndex(pCursor)  ((ET9U32)((pCursor)->dwCurrCacheStart + (pbCurrData - (pCursor)->pbCache)))

#define __LdbUpdateCache(pCursor)                                                                       \
{                                                                                                       \
    ET9U32 dwBytesRead;                                                                                 \
    pLingInfo->ET9AWLdbReadData(pLingInfo,                                                              \
                                (pCursor)->dwCurrCacheStart,                                            \
                                ALDB_CURSOR_DATA_CACHE_SIZE,                                            \
                                (pCursor)->pbCache,                                                     \
                                &dwBytesRead);                                                          \
}

#define __LdbSetIndex(pCursor,index)                                                                    \
{                                                                                                       \
    (pCursor)->dwCurrCacheStart = (index);                                                              \
    pbCurrData = (pCursor)->pbCache; __LdbUpdateCache(pCursor);                                         \
}                                                                                                       \

#define __LdbIncIndex(pCursor)                                                                          \
{                                                                                                       \
    if (++pbCurrData == (pCursor)->pbCacheEnd) {                                                        \
        __LdbSetIndex(pCursor, (pCursor)->dwCurrCacheStart + ALDB_CURSOR_DATA_CACHE_SIZE);              \
    };                                                                                                  \
}                                                                                                       \

#define __LdbDecIndex(pCursor)                                                                          \
{                                                                                                       \
    if (--pbCurrData < (pCursor)->pbCache) {                                                            \
        __LdbSetIndex(pCursor, (pCursor)->dwCurrCacheStart - 1);                                        \
    };                                                                                                  \
}                                                                                                       \

#endif /* ET9_DIRECT_LDB_ACCESS */

/*---------------------------------------------------------------------------*/
/** \internal
 * Check if the LDB is exhausted (during search).
 * This exist in order to make the code a bit more LDB format independent.
 *
 * @return Non zero if exhausted, otherwise zero.
 */

#define __IsExhausted()         (ALDB.search.bExhausted)

/*---------------------------------------------------------------------------*/
/** \internal
 * Get the index of the current word (during search).
 * This exist in order to make the code a bit more LDB format independent.
 *
 * @return Word index.
 */

#define __GetWordIndex()        (ALDB.search.dwCurrItem)

/*---------------------------------------------------------------------------*/
/** \internal
 * Convenience setup of const locals for commonly used things from the ALDB structure.
 */

#define __ET9HeaderConstsGetNextInterval                                                                \
    const ET9U16    wCodeIntervalEnd = ALDB.header.wCodeIntervalEnd;                                    \
    const ET9U16    wCodeIntervalJump = ALDB.header.wCodeIntervalJump;                                  \
    const ET9U16    wCodeIntervalExtend = ALDB.header.wCodeIntervalExtend;                              \
    ET9U8   * const pbOneByteCodes = ALDB.header.pbOneByteCodes;                                        \
    ET9U8   * const pbOneByteLengths = ALDB.header.pbOneByteLengths;                                    \
    ET9SYMB * const psTarget = ALDB.search.psTarget;                                                    \
    ET9SYMB * const psCharacterDecodeTable = ALDB.header.psCharacterDecodeTable;                        \

/*---------------------------------------------------------------------------*/
/** \internal
 * Update a cursor to a new target position in the LDB, next applicable interval.
 * A cursor is keeping track of one LDB stream, related to one input symol/position.<br>
 * This will bring it up-to-date for a new target position (potential word match during search).<br>
 * It can only move forward in the stream.<br>
 * If the stream becomes exhausted during the update it will perform a "return". This is to prevent the
 * need for checking for exhausted after every cursor update.<br>
 * This piece of code is the one that is by far the most executed one and has great impact on over all performance.
 * It's also the code that is the most LDB format dependent, at least when it comes to data streams.
 * <br><br>*** Do NOT modify this code without proper performance benchmarks!!!
 *
 * @param pLingInfo                 Pointer to alphabetic information structure.
 * @param pCursor                   Current cursor.
 * @param bPos                      Current position (corresponds with the cursor).
 * @param dwTargetPosIn             The target position.
 *
 * @return None
 */

#define __ET9SearchGetNextInterval(pLingInfo, pCursor, bPos, dwTargetPosIn)                             \
{                                                                                                       \
    register ET9U8                  bCode;                                                              \
    register ET9U16                 wCode = 0xcccc;                                                     \
    register ET9U32                 dwStartPos = (pCursor)->dwStartPos;                                 \
    register ET9U32                 dwEndPos = (pCursor)->dwEndPos;                                     \
    register ET9U32                 dwJumpPos = (pCursor)->dwJumpPos;                                   \
    register ET9U8 ET9FARDATA_LDB   *pbCurrData = (pCursor)->pbCurrData;                                \
                                                                                                        \
    const ET9U32                    dwTargetPos = dwTargetPosIn;                                        \
                                                                                                        \
    while (dwTargetPos >= dwEndPos || dwStartPos == dwEndPos) {                                         \
                                                                                                        \
        /* Move start pos */                                                                            \
                                                                                                        \
        dwStartPos = dwEndPos;                                                                          \
                                                                                                        \
        /* Check if jump is possible */                                                                 \
                                                                                                        \
        if (dwTargetPos >= dwJumpPos && dwStartPos + 1 < dwJumpPos) {                                   \
                                                                                                        \
            dwStartPos = dwJumpPos;                                                                     \
            dwEndPos = dwStartPos;                                                                      \
                                                                                                        \
            __LdbSetIndex(pCursor, (pCursor)->dwJumpAddress);                                           \
                                                                                                        \
            continue;                                                                                   \
        }                                                                                               \
                                                                                                        \
        /* Get interval code - could be single or multi encoded */                                      \
                                                                                                        \
        bCode = __LdbGetByte(pCursor);                                                                  \
        __LdbIncIndex(pCursor);                                                                         \
                                                                                                        \
        /* Check if single, and possibly a double */                                                    \
                                                                                                        \
        if (bCode != ALDB_INTERVAL_MULTI_BYTE_CODE) {                                                   \
                                                                                                        \
            wCode = pbOneByteCodes[bCode];                                                              \
            dwEndPos = dwStartPos + pbOneByteLengths[bCode];                                            \
                                                                                                        \
            bCode = __LdbGetByte(pCursor);                                                              \
                                                                                                        \
            if (wCode == pbOneByteCodes[bCode] && bCode != ALDB_INTERVAL_MULTI_BYTE_CODE) {             \
                __LdbIncIndex(pCursor);                                                                 \
                dwEndPos += pbOneByteLengths[bCode];                                                    \
            }                                                                                           \
                                                                                                        \
            continue;                                                                                   \
        }                                                                                               \
                                                                                                        \
        /* Get multi code - could be jump, char with length or end */                                   \
                                                                                                        \
        wCode = __LdbGetByte(pCursor);                                                                  \
        __LdbIncIndex(pCursor);                                                                         \
                                                                                                        \
        /* Check if jump info */                                                                        \
                                                                                                        \
        if (wCode == wCodeIntervalJump) {                                                               \
                                                                                                        \
            dwJumpPos = dwStartPos;                                                                     \
                                                                                                        \
            dwJumpPos += ((ET9U32)__LdbGetByte(pCursor)) << 8;                                          \
            __LdbIncIndex(pCursor);                                                                     \
                                                                                                        \
            dwJumpPos += __LdbGetByte(pCursor);                                                         \
            __LdbIncIndex(pCursor);                                                                     \
                                                                                                        \
            (pCursor)->dwJumpAddress = __LdbGetIndex(pCursor) - 4;                                      \
                                                                                                        \
            (pCursor)->dwJumpAddress += ((ET9U32)__LdbGetByte(pCursor)) << 8;                           \
            __LdbIncIndex(pCursor);                                                                     \
                                                                                                        \
            (pCursor)->dwJumpAddress += __LdbGetByte(pCursor);                                          \
            __LdbIncIndex(pCursor);                                                                     \
                                                                                                        \
            /* equal start/end implies loop again */                                                    \
                                                                                                        \
            ET9Assert(dwStartPos == dwEndPos);                                                          \
                                                                                                        \
            continue;                                                                                   \
        }                                                                                               \
                                                                                                        \
        /* Check if end */                                                                              \
                                                                                                        \
        if (wCode == wCodeIntervalEnd) {                                                                \
            ALDB.search.bExhausted = 1;                                                                 \
            return;                                                                                     \
        }                                                                                               \
                                                                                                        \
        /* Direct interval... */                                                                        \
                                                                                                        \
        dwEndPos = dwStartPos + __LdbGetByte(pCursor);                                                  \
        __LdbIncIndex(pCursor);                                                                         \
                                                                                                        \
        /* Check length extension */                                                                    \
                                                                                                        \
        if (__LdbGetByte(pCursor) != ALDB_INTERVAL_MULTI_BYTE_CODE) {                                   \
            continue;                                                                                   \
        }                                                                                               \
                                                                                                        \
        __LdbIncIndex(pCursor);                                                                         \
                                                                                                        \
        if (__LdbGetByte(pCursor) != wCodeIntervalExtend) {                                             \
                                                                                                        \
            __LdbDecIndex(pCursor);                                                                     \
                                                                                                        \
            continue;                                                                                   \
        }                                                                                               \
                                                                                                        \
        /* Found extension */                                                                           \
                                                                                                        \
        __LdbIncIndex(pCursor);                                                                         \
                                                                                                        \
        dwEndPos += ((ET9U32)__LdbGetByte(pCursor)) << 8;                                               \
        __LdbIncIndex(pCursor);                                                                         \
    }                                                                                                   \
                                                                                                        \
    /* Pick up the actual symbol as well... */                                                          \
                                                                                                        \
    psTarget[bPos] = psCharacterDecodeTable[wCode];                                                     \
                                                                                                        \
    /* store locals */                                                                                  \
                                                                                                        \
    (pCursor)->wCode = wCode;                                                                           \
    (pCursor)->dwStartPos = dwStartPos;                                                                 \
    (pCursor)->dwEndPos = dwEndPos;                                                                     \
    (pCursor)->dwJumpPos = dwJumpPos;                                                                   \
    (pCursor)->pbCurrData = pbCurrData;                                                                 \
                                                                                                        \
    ET9Assert(wCode != 0xcccc);                                                                         \
    ET9Assert(__IsExhausted() || dwStartPos <= dwTargetPos && dwTargetPos < dwEndPos);                  \
}                                                                                                       \

/*---------------------------------------------------------------------------*/
/** \internal
 * Catchup a stream, assuring that the interval info is valid.
 *
 * @param pLingInfo                 Pointer to alphabetic information structure.
 * @param pCursor                   Current cursor.
 * @param bPos                      Current position (corresponds with the cursor).
 * @param dwTargetPosIn             The target position.
 *
 * @return None
 */

#define __ET9SearchCatchupStream(pLingInfo, pCursor, bPos, dwTargetPosIn)                               \
{                                                                                                       \
    if ((pCursor)->dwEndPos <= dwTargetPosIn) {                                                         \
                                                                                                        \
        __ET9SearchGetNextInterval(pLingInfo, pCursor, bPos, dwTargetPosIn);                            \
    }                                                                                                   \
}                                                                                                       \

/*---------------------------------------------------------------------------*/
/** \internal
 * Catchup a stream, assuring that the interval info is valid + update counters.
 *
 * @param pLingInfo                 Pointer to alphabetic information structure.
 * @param pCursor                   Current cursor.
 * @param bPos                      Current position (corresponds with the cursor).
 * @param dwTargetPosIn             The target position.
 *
 * @return None
 */

#define __ET9SearchCatchupStreamCounters(pLingInfo, pCursor, bPos, dwTargetPosIn)                       \
{                                                                                                       \
    if ((pCursor)->dwEndPos <= dwTargetPosIn) {                                                         \
                                                                                                        \
        WLOG3(fprintf(pLogFile3, "__ET9SearchCatchupStreamCounters, bPos %2u, dwTargetPosIn %6u\n", bPos, dwTargetPosIn);) \
                                                                                                        \
        __DecCharCounter(pLingCmnInfo, pCursor->wCode);                                                 \
                                                                                                        \
        __ET9SearchGetNextInterval(pLingInfo, pCursor, bPos, dwTargetPosIn);                            \
                                                                                                        \
        __IncCharCounter(pLingCmnInfo, pCursor->wCode);                                                 \
    }                                                                                                   \
}                                                                                                       \

/*---------------------------------------------------------------------------*/
/** \internal
 * Catchup a stream, assuring that the interval info is valid.
 * If no update is necessary it will execute a "break".
 *
 * @param pLingInfo                 Pointer to alphabetic information structure.
 * @param pCursor                   Current cursor.
 * @param bPos                      Current position (corresponds with the cursor).
 * @param dwTargetPosIn             The target position.
 *
 * @return None
 */

#define __ET9SearchCatchupStreamWithBreak(pLingInfo, pCursor, bPos, dwTargetPosIn)                      \
{                                                                                                       \
    if (pCursor->dwEndPos <= dwTargetPosIn) {                                                           \
                                                                                                        \
        __ET9SearchGetNextInterval(pLingInfo, pCursor, bPos, dwTargetPosIn);                            \
    }                                                                                                   \
    else {                                                                                              \
        break;                                                                                          \
    }                                                                                                   \
}                                                                                                       \

#ifdef ET9ASPC_DEV_SCREEN

/*---------------------------------------------------------------------------*/
/** \internal
 * .
 *
 * @param pLingCmnInfo              Pointer to alphabetic cmn information structure.
 * @param wCode                     Code to handle.
 *
 * @return None
 */

ET9INLINE static void ET9LOCALCALL __IncCharCounter (ET9AWLingCmnInfo * const pLingCmnInfo,
                                                     const ET9U16             wCode)
{
    ET9ASPCFlexCompareData  * const pCMP = &pLingCmnInfo->Private.ASpc.u.sCmpDataFlex;

    if (wCode == ALDB.header.wCodeZero) {
        return;
    }

    ET9Assert(wCode < ALDB_HEADER_MAX_CHAR_CODES);
    ET9Assert(pCMP->pbCharCounters[wCode] < ALDB.header.bPosCount);

    if (++pCMP->pbCharCounters[wCode] == 1) {

        ET9ASPCSupportRelations const * const pRelations = &pCMP->pCharCodeRelations[wCode];

        ET9U8 bIndex;

        WLOG3(fprintf(pLogFile3, "__IncCharCounter, wCode %2u (%c) ON\n", wCode, (char)ALDB.header.psCharacterDecodeTable[wCode]);)

        for (bIndex = 0; bIndex < pRelations->bCount; ++bIndex) {

            if (++pCMP->pbQualityCounters[pRelations->pbIndex[bIndex]] == 1) {

                ++pCMP->nSupportedQualityCount;

                ET9Assert(pCMP->nSupportedQualityCount <= pCMP->nActiveQualityCount);

                WLOG3(fprintf(pLogFile3, "  q-index %2u ON - new s-count %2u\n", pRelations->pbIndex[bIndex], pCMP->nSupportedQualityCount);)
            }
        }
    }
}

#else

#define __IncCharCounter(pLingCmnInfo, wCode)

#endif

#ifdef ET9ASPC_DEV_SCREEN

/*---------------------------------------------------------------------------*/
/** \internal
 * .
 *
 * @param pLingCmnInfo              Pointer to alphabetic cmn information structure.
 * @param wCode                     Code to handle.
 *
 * @return None
 */

ET9INLINE static void ET9LOCALCALL __DecCharCounter (ET9AWLingCmnInfo * const pLingCmnInfo,
                                                     const ET9U16             wCode)
{
    ET9ASPCFlexCompareData  * const pCMP = &pLingCmnInfo->Private.ASpc.u.sCmpDataFlex;

    if (wCode == ALDB.header.wCodeZero) {
        return;
    }

    ET9Assert(wCode < ALDB_HEADER_MAX_CHAR_CODES);
    ET9Assert(pCMP->pbCharCounters[wCode] > 0);

    if (--pCMP->pbCharCounters[wCode] == 0) {

        ET9ASPCSupportRelations const * const pRelations = &pCMP->pCharCodeRelations[wCode];

        ET9U8 bIndex;

        WLOG3(fprintf(pLogFile3, "__DecCharCounter, wCode %2u (%c) OFF\n", wCode, (char)ALDB.header.psCharacterDecodeTable[wCode]);)

        for (bIndex = 0; bIndex < pRelations->bCount; ++bIndex) {

            if (--pCMP->pbQualityCounters[pRelations->pbIndex[bIndex]] == 0) {

                --pCMP->nSupportedQualityCount;

                ET9Assert(pCMP->nSupportedQualityCount <= pCMP->nActiveQualityCount);

                WLOG3(fprintf(pLogFile3, "  q-index %2u OFF - new s-count %2u\n", pRelations->pbIndex[bIndex], pCMP->nSupportedQualityCount);)
            }
        }
    }
}

#else

#define __DecCharCounter(pLingCmnInfo, wCode)

#endif

/*---------------------------------------------------------------------------*/
/** \internal
 * Function version of the __ET9SearchGetNextInterval macro.
 * It can be used in non performance critical parts of the code.
 *
 * @param pLingInfo                 Pointer to alphabetic information structure.
 * @param pCursor                   Current cursor.
 * @param wPos                      Current position (corresponds with the cursor).
 * @param dwTargetPosIn             The target position.
 * @param bUsingFlex                If targeting flex compare.
 *
 * @return None
 */

static void ET9LOCALCALL __ET9SearchGetNextIntervalSlow (ET9AWLingInfo * const       pLingInfo,
                                                         ET9ALdbCursorData * const   pCursor,
                                                         const ET9U16                wPos,
                                                         const ET9U32                dwTargetPosIn,
                                                         const ET9BOOL               bUsingFlex)
{
    ET9AWLingCmnInfo * const pLingCmnInfo = pLingInfo->pLingCmnInfo;

    __ET9HeaderConstsGetNextInterval

    WLOG3(fprintf(pLogFile3, "__ET9SearchGetNextIntervalSlow, wPos %2u, dwTargetPosIn %6u, bUsingFlex %u\n", wPos, dwTargetPosIn, bUsingFlex);)

    __ET9SearchGetNextInterval(pLingInfo, pCursor, wPos, dwTargetPosIn);

    if (bUsingFlex && wPos) {
        __IncCharCounter(pLingCmnInfo, pCursor->wCode);
    }
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Move the search forward to the next matching word - standard method.
 * This function has great impact on over all performance.
 * This code is partly LDB format dependent, also highly optimized for spell correction performance.
 * <br><br>*** Do NOT modify this code without proper performance benchmarks!!!
 *
 * @param pLingInfo                 Pointer to alphabetic information structure.
 *
 * @return None
 */

static void ET9LOCALCALL __ET9SearchGetNextStd (ET9AWLingInfo * const pLingInfo)
{
    ET9AWLingCmnInfo * const pLingCmnInfo = pLingInfo->pLingCmnInfo;

    __ET9HeaderConstsGetNextInterval

    const ET9U8     bPosCount           = ALDB.header.bPosCount;
    const ET9U16    wCodeZero           = ALDB.header.wCodeZero;

    const ET9U8     bRegCmpLength       = ALDB.search.bRegCmpLength;
    ET9U8 * const   pbRegPosCurrOrder   = ALDB.search.pbRegPosCurrOrder;

    ET9ALdbCursorData * const pCursors = ALDB.pCursors;

    ET9U8               bPos;
    ET9U8               bPosIndex;
    ET9ALdbCursorData   *pCursor;
    ET9U32              dwCurrItem = ALDB.search.dwCurrItem + 1;

    WLOG3B(fprintf(pLogFile3, "__ET9SearchGetNextStd, dwCurrItem = %6d, spcActive = %d\n", dwCurrItem, ALDB.compare.bSpcActive);)

    /* validate */

    ET9Assert(ALDB.search.bSpcNonZeroPos < ALDB.compare.bActiveCmpLength);

    /* should not already be exhausted */

    ET9Assert(!__IsExhausted());

    /* find a "match" - either the regular way or with spell correction */

    if (ALDB.compare.bSpcActive && ALDB.compare.wLength > 2) {

        const ET9U8     bInputLength            = (ET9U8)ALDB.compare.wLength;

        const ET9BOOL   bSpcExactCompare        = ALDB.compare.bSpcExactCompare;
        const ET9BOOL   bSpcFilteredCompare     = ALDB.compare.bSpcFilteredCompare;
        const ET9BOOL   bSpcExactFilter         = ALDB.compare.bSpcExactFilter;

        const ET9U8     bSpcMaxEdits            = ALDB.compare.bSpcMaxEdits;
        const ET9U8     bSpcLengthOffset        = ALDB.compare.bSpcLengthOffset;

        const ET9U8     bSpcNonZeroPos          = ALDB.search.bSpcNonZeroPos;
        const ET9U8     bSpcControlPos          = ALDB.search.bSpcControlPos;

        ET9BOOL         bSpcCompare             = ALDB.search.bSpcCompare;
        ET9U32          dwSpcControlEndPos      = ALDB.search.dwSpcControlEndPos;
        ET9U32          dwRegNonMatchEndPos     = ALDB.search.dwRegNonMatchEndPos;

        WLOG3B(fprintf(pLogFile3, "__ET9SearchGetNextStd, bSpcCompare = %d, dwSpcControlEndPos = %d, non zero pos = %d, control pos = %d\n", bSpcCompare, dwSpcControlEndPos, bSpcNonZeroPos, bSpcControlPos);)

        /* repeat regular or spell correction search depending on word length properties */

        for (;;) {

            WLOG3B(fprintf(pLogFile3, "__ET9SearchGetNextStd, NEW REPEAT (%d)\n", dwCurrItem);)

            /* skip too short words */

            pCursor = &pCursors[bSpcNonZeroPos];

            __ET9SearchCatchupStream(pLingInfo, pCursor, bSpcNonZeroPos, dwCurrItem);

            if (pCursor->wCode == wCodeZero) {

                WLOG3B(fprintf(pLogFile3, "__ET9SearchGetNextStd, too short word, skipping %d\n", (pCursor->dwEndPos - dwCurrItem));)

                dwCurrItem = pCursor->dwEndPos;
                continue;
            }

            /* check if how-to-compare changed */

            if (dwCurrItem >= dwSpcControlEndPos) {

                ET9U32 dwOldCurrItem = dwCurrItem;

                pCursor = &pCursors[bSpcControlPos];

                /* catch up on interval data (test end to assure side effects from tails) */

                for (;;) {

                    __ET9SearchCatchupStreamWithBreak(pLingInfo, pCursor, bSpcControlPos, dwCurrItem);

                    /* make use of known non math */

                    if (dwRegNonMatchEndPos > dwCurrItem && pCursor->wCode != wCodeZero) {

                        WLOG3B(fprintf(pLogFile3, "__ET9SearchGetNextStd, using known non match (%d)\n", (dwRegNonMatchEndPos < pCursor->dwEndPos ? (dwRegNonMatchEndPos - dwCurrItem) : (pCursor->dwEndPos - dwCurrItem)));)

                        if (dwRegNonMatchEndPos < pCursor->dwEndPos) {

                            dwCurrItem = dwRegNonMatchEndPos;
                        }
                        else {
                            dwCurrItem = pCursor->dwEndPos;
                        }
                    }
                }

                /* set compare info */

                bSpcCompare = (ET9U8)(pCursor->wCode == wCodeZero);

                WLOG3B(fprintf(pLogFile3, "__ET9SearchGetNextStd, new how-to-compare, spcCompare = %d, dwEndPos = %d (len %d)\n", bSpcCompare, pCursor->dwEndPos, (pCursor->dwEndPos - dwSpcControlEndPos));)

                dwSpcControlEndPos = pCursor->dwEndPos;

                /* need to recheck too short words? */

                if (bSpcCompare && dwOldCurrItem != dwCurrItem) {
                    continue;
                }
            }

            /* compare depending on spc/regular */

            if (bSpcCompare) { /* spc check (while in this mode) */

                ET9S8 sbPosIndex;
                ET9S8 sbAvailableEdits;
                ET9U32 dwPossibleJump;
                ET9ALdbSymbPosInfo *pbActiveSection;

                ET9U8  bCurrWordLength = ALDB.search.bCurrWordLength;
                ET9U32 dwWordLengthEndPos = ALDB.search.dwWordLengthEndPos;
                ET9U32 dwWordLengthActiveEdgeEndPos = dwWordLengthEndPos;
                ET9U32 dwWordLengthZeroEdgeEndPos = dwWordLengthEndPos;

                __IncScreenAttempt();

                /* handle filter */

                if (bSpcFilteredCompare) {

                    const ET9U8 bFilterPos = 0;

                    pCursor = &pCursors[bFilterPos];

                    __ET9SearchCatchupStream(pLingInfo, pCursor, bFilterPos, dwCurrItem);

                    /* check filter position */

                    if (!__IsCodeActiveSpc(__GetActiveSectionSpc(0), bFilterPos, pCursor->wCode)) {

                        WLOG3B(fprintf(pLogFile3, "__ET9SearchGetNextStd, filter pos not matching, dwEndPos = %d (%d)\n", pCursor->dwEndPos, dwCurrItem);)

                        dwPossibleJump = pCursor->dwEndPos;

                        /* if an exact filter and exact is significant, then we need a supporting (failing) classic position */

                        if (bSpcExactFilter && bSpcExactCompare) {

                            WLOG3B(fprintf(pLogFile3, "__ET9SearchGetNextStd, looking for a failing classic\n");)

                            sbPosIndex = bInputLength - 1;
                            pCursor = &pCursors[sbPosIndex];

                            for (; sbPosIndex >= 0; --sbPosIndex, --pCursor) {

                                __ET9SearchCatchupStream(pLingInfo, pCursor, sbPosIndex, dwCurrItem);

                                if (!__IsCodeActive(sbPosIndex, pCursor->wCode)) {

                                    WLOG3B(fprintf(pLogFile3, "__ET9SearchGetNextStd, found a failing classic at pos %d, dwEndPos = %d\n", sbPosIndex, pCursor->dwEndPos);)

                                    if (dwPossibleJump > pCursor->dwEndPos) {
                                        dwPossibleJump = pCursor->dwEndPos;
                                    }

                                    break;
                                }
                            }

                            if (sbPosIndex >= 0) {

                                __IncScreenRejected();

                                WLOG3B(fprintf(pLogFile3, "__ET9SearchGetNextStd, spc filter rejection with support, skipping %d\n", (dwPossibleJump - dwCurrItem));)

                                dwCurrItem = dwPossibleJump;

                                continue;
                            }
                        }
                        else {

                            __IncScreenRejected();

                            WLOG3B(fprintf(pLogFile3, "__ET9SearchGetNextStd, spc filter rejection, skipping %d\n", (dwPossibleJump - dwCurrItem));)

                            dwCurrItem = dwPossibleJump;

                            continue;
                        }
                    } /* __IsCodeActiveSpc */
                } /* bSpcFilteredCompare */

                /* determine the current word length */

                if (dwCurrItem >= dwWordLengthEndPos) {

                    ET9U8 bMid;
                    ET9U8 bLo = ALDB.compare.bPosLo;
                    ET9U8 bHi = ALDB.compare.bPosHi;

                    while (bLo < bHi) {

                        bMid = (ET9U8)((bLo + bHi) / 2);

                        pCursor = &pCursors[bMid];

                        __ET9SearchCatchupStream(pLingInfo, pCursor, bMid, dwCurrItem);

                        if (pCursor->wCode == wCodeZero) {
                            bHi = (ET9U8)(bMid - 1);
                        }
                        else {
                            bLo = (ET9U8)(bMid + 1);
                        }
                    }

                    pCursor = &pCursors[bLo];

                    __ET9SearchCatchupStream(pLingInfo, pCursor, bLo, dwCurrItem);

                    if (pCursor->wCode == wCodeZero) {
                        bCurrWordLength = bLo;
                    }
                    else {
                        bCurrWordLength = (ET9U8)(bLo + 1);
                    }

                    /* investigate how long the length run is (checking the edge streams) */

                    {
                        ET9U8 bEdgePos = bCurrWordLength;

                        __IncScreenLengthCalc();

                        pCursor = &pCursors[bEdgePos];

                        if (bEdgePos < bPosCount) {

                            __ET9SearchCatchupStream(pLingInfo, pCursor, bEdgePos, dwCurrItem);

                            dwWordLengthZeroEdgeEndPos = pCursor->dwEndPos;
                        }
                        else {
                            dwWordLengthZeroEdgeEndPos = ~((ET9U32)0);
                        }

                        WLOG3B(fprintf(pLogFile3, "__ET9SearchGetNextStd, how long run, zero edge end = %d %+d (%d)\n", dwWordLengthZeroEdgeEndPos, (dwWordLengthZeroEdgeEndPos - dwCurrItem), dwCurrItem);)

                        ET9Assert(bEdgePos > 0);

                        --pCursor;
                        --bEdgePos;

                        __ET9SearchCatchupStream(pLingInfo, pCursor, bEdgePos, dwCurrItem);

                        dwWordLengthActiveEdgeEndPos = pCursor->dwEndPos;

                        WLOG3B(fprintf(pLogFile3, "__ET9SearchGetNextStd, how long run, active edge end = %d %+d (%d)\n", dwWordLengthActiveEdgeEndPos, (dwWordLengthActiveEdgeEndPos - dwCurrItem), dwCurrItem);)

                        if (dwWordLengthActiveEdgeEndPos > dwWordLengthZeroEdgeEndPos) {
                            dwWordLengthEndPos = dwWordLengthZeroEdgeEndPos;
                        }
                        else {
                            dwWordLengthEndPos = dwWordLengthActiveEdgeEndPos;
                        }

                        WLOG3B(fprintf(pLogFile3, "__ET9SearchGetNextStd, how long run, calculated end = %d %+d (%d)\n", dwWordLengthEndPos, (dwWordLengthEndPos - dwCurrItem), dwCurrItem);)
                    }

                    /* done with the length thing */

                    __IncScreenLength(dwWordLengthEndPos - dwCurrItem);

                    WLOG3B(fprintf(pLogFile3, "__ET9SearchGetNextStd, determined word length, bCurrWordLength = %d, remaining run = %d (%d)\n", bCurrWordLength, dwWordLengthEndPos - dwCurrItem, dwCurrItem);)

                    /* store locals */

                    ALDB.search.bCurrWordLength = bCurrWordLength;
                    ALDB.search.dwWordLengthEndPos = dwWordLengthEndPos;
                }
                else {
                    WLOG3B(fprintf(pLogFile3, "__ET9SearchGetNextStd, recovered word length, bCurrWordLength = %d, remaining run = %d (%d)\n", bCurrWordLength, dwWordLengthEndPos - dwCurrItem, dwCurrItem);)
                }

                /* set up for compare */

                dwPossibleJump = ~((ET9U32)0);
                sbAvailableEdits = bSpcMaxEdits;
                pbActiveSection = __GetActiveSectionSpc(bCurrWordLength - bSpcLengthOffset);

                WLOG3B(fprintf(pLogFile3, "__ET9SearchGetNextStd, using section %d (%d)\n", (bCurrWordLength - bSpcLengthOffset), dwCurrItem);)

                /* words shorter than the input length take advantage of its "non matches" */

                if (bCurrWordLength < bInputLength) {
                    sbAvailableEdits -= (bInputLength - bCurrWordLength);
                }

                /* loop to find non matching spc word positions */

                sbPosIndex = bCurrWordLength - 1;
                pCursor = &pCursors[sbPosIndex];

                for (; sbPosIndex >= 0; --sbPosIndex, --pCursor) {

                    __ET9SearchCatchupStream(pLingInfo, pCursor, sbPosIndex, dwCurrItem);

                    /* check against the spc calculated set */

                    if (!__IsCodeActiveSpc(pbActiveSection, sbPosIndex, pCursor->wCode)) {

                        WLOG3B(fprintf(pLogFile3, "__ET9SearchGetNextStd, spc pos %2d not matching, dwEndPos = %d %+2d (%d)\n", sbPosIndex, pCursor->dwEndPos, (pCursor->dwEndPos - dwCurrItem), dwCurrItem);)

                        --sbAvailableEdits;

                        if (dwPossibleJump > pCursor->dwEndPos) {
                            dwPossibleJump = pCursor->dwEndPos;
                        }

                        /* found enough? */

                        if (sbAvailableEdits < 0) {
                            break;
                        }
                    }

                } /* loop spc positions */

                /* if necessary, find a supporting non matching classic position */

                if (bSpcExactCompare && sbPosIndex >= 0) {

                    ET9U32 dwPossibleClassicJump = 0;

                    sbPosIndex = bCurrWordLength - 1;
                    pCursor = &pCursors[sbPosIndex];

                    /* loop to find a non matching classic word position */

                    for (; sbPosIndex >= 0; --sbPosIndex, --pCursor) {

                        __ET9SearchCatchupStream(pLingInfo, pCursor, sbPosIndex, dwCurrItem);

                        /* check against classic */

                        if (!__IsCodeActive(sbPosIndex, pCursor->wCode)) {

                            WLOG3B(fprintf(pLogFile3, "__ET9SearchGetNextStd, classic pos %2d not matching, dwEndPos = %d %+2d (%d)\n", sbPosIndex, pCursor->dwEndPos, (pCursor->dwEndPos - dwCurrItem), dwCurrItem);)

                            if (dwPossibleClassicJump < pCursor->dwEndPos) {
                                dwPossibleClassicJump = pCursor->dwEndPos;
                            }

                            if (dwPossibleClassicJump >= dwPossibleJump) {
                                break;
                            }
                        }

                    } /* loop classic positions */

                    if (dwPossibleClassicJump) {

                        if (dwPossibleJump > dwPossibleClassicJump) {
                            dwPossibleJump = dwPossibleClassicJump;
                        }

                        sbPosIndex = 0;
                    }
                }

                /* break if pass (could return if we clean up before leaving) */

                if (sbPosIndex < 0) {

                    __IncScreenPassed();

                    bPosIndex = bCurrWordLength;
                    break;
                }

                /* the compare is length dependent, so we can't jump longer than the length runs */

                if (dwPossibleJump > dwWordLengthEndPos) {

                    WLOG3B(fprintf(pLogFile3, "__ET9SearchGetNextStd, length run limits the jump, compare %+d, length %+d\n", (dwPossibleJump - dwCurrItem), (dwWordLengthEndPos - dwCurrItem));)

                    if (dwWordLengthActiveEdgeEndPos < dwWordLengthZeroEdgeEndPos) {

                        ET9BOOL bExtended = 0;
                        ET9U8 bAtiveEdgePos = bCurrWordLength - 1;

                        WLOG3B(fprintf(pLogFile3, "__ET9SearchGetNextStd, attempting a length extension, active edge %d, zero edge %d\n", dwWordLengthActiveEdgeEndPos, dwWordLengthZeroEdgeEndPos);)

                        pCursor = &pCursors[bAtiveEdgePos];

                        /* while the active edge is non zero, shorter than the zero edge and the possible jump, extend it */

                        for (;;) {

                            __ET9SearchCatchupStream(pLingInfo, pCursor, bAtiveEdgePos, dwWordLengthActiveEdgeEndPos);

                            if (pCursor->wCode == wCodeZero) {
                                break;
                            }

                            bExtended = 1;
                            dwWordLengthActiveEdgeEndPos = pCursor->dwEndPos;

                            WLOG3B(fprintf(pLogFile3, "__ET9SearchGetNextStd, found extension %d %+d\n", pCursor->dwEndPos, (pCursor->dwEndPos - dwCurrItem));)

                            if (dwWordLengthActiveEdgeEndPos >= dwWordLengthZeroEdgeEndPos ||
                                dwWordLengthActiveEdgeEndPos >= dwPossibleJump) {
                                break;
                            }
                        }

                        if (!bExtended) {

                            WLOG3B(fprintf(pLogFile3, "__ET9SearchGetNextStd, no extension possible\n");)

                            dwPossibleJump = dwWordLengthEndPos;
                        }
                        else {

                            /* reevaluate the word length run end */

                            if (dwWordLengthActiveEdgeEndPos > dwWordLengthZeroEdgeEndPos) {
                                dwWordLengthEndPos = dwWordLengthZeroEdgeEndPos;
                            }
                            else {
                                dwWordLengthEndPos = dwWordLengthActiveEdgeEndPos;
                            }

                            WLOG3B(fprintf(pLogFile3, "__ET9SearchGetNextStd, result, active edge %d %+d, zero edge %d %+d\n", dwWordLengthActiveEdgeEndPos, (dwWordLengthActiveEdgeEndPos - dwCurrItem), dwWordLengthZeroEdgeEndPos, (dwWordLengthZeroEdgeEndPos - dwCurrItem));)

                            /* store locals */

                            ALDB.search.dwWordLengthEndPos = dwWordLengthEndPos;

                            /* reevaluate the length based jump limitation */

                            if (dwPossibleJump > dwWordLengthEndPos) {

                                WLOG3B(fprintf(pLogFile3, "__ET9SearchGetNextStd, length run limits jump by %d\n", dwPossibleJump - dwWordLengthEndPos);)

                                __IncScreenLimited();

                                dwPossibleJump = dwWordLengthEndPos;
                            }
                        }
                    }
                    else {

                        WLOG3B(fprintf(pLogFile3, "__ET9SearchGetNextStd, length run limits jump by %d\n", dwPossibleJump - dwWordLengthEndPos);)

                        __IncScreenLimited();

                        dwPossibleJump = dwWordLengthEndPos;
                    }
                }

                WLOG3B(fprintf(pLogFile3, "__ET9SearchGetNextStd, spc rejection, jumping %d\n", (dwPossibleJump - dwCurrItem));)

                __IncScreenRejected();
                __IncScreenJump(dwPossibleJump - dwCurrItem);

                dwCurrItem = dwPossibleJump;
            }
            else { /* regular check (while in this mode) */

                bPosIndex = 0;

                while (bPosIndex < bRegCmpLength) {

                    bPos = pbRegPosCurrOrder[bPosIndex];

                    pCursor = &pCursors[bPos];

                    __ET9SearchCatchupStream(pLingInfo, pCursor, bPos, dwCurrItem);

                    if (__IsCodeActive(bPos, pCursor->wCode)) {
                        ++bPosIndex;
                        continue;
                    }

                    /* no match, skip this interval (max as far as this mode goes, save the rest if any) */

                    WLOG3B(fprintf(pLogFile3, "__ET9SearchGetNextStd, spc in regular mode, skipping %d (lost %d)\n", ((dwSpcControlEndPos < pCursor->dwEndPos ? dwSpcControlEndPos : pCursor->dwEndPos) - dwCurrItem), (pCursor->dwEndPos > dwSpcControlEndPos ? (pCursor->dwEndPos - dwSpcControlEndPos) : 0));)

                    if (dwSpcControlEndPos < pCursor->dwEndPos) {

                        dwCurrItem = dwSpcControlEndPos;
                        dwRegNonMatchEndPos = pCursor->dwEndPos;
                        break;
                    }
                    else {
                        dwCurrItem = pCursor->dwEndPos;
                        bPosIndex = 0;
                        continue;
                    }
                }

                /* check if candidate passed */

                if (bPosIndex == bRegCmpLength) {
                    break;
                }
            }

        } /* repeat */

        /* store locals */

        ALDB.search.bSpcCompare = bSpcCompare;
        ALDB.search.dwSpcControlEndPos = dwSpcControlEndPos;
        ALDB.search.dwRegNonMatchEndPos = dwRegNonMatchEndPos;
    }
    else { /* non spell correction search */

        /* first pos set opt (after previous word) */

        if (ALDB.compare.bFirstPosSetOpt && *ALDB.search.pwLength == 1) {

            const ET9U8 bPos = 0;

            ET9U16 wFirstCode = __LdbGetCharCodeAtPos(bPos);

            __DeactivateCode(bPos, wFirstCode);

            WLOG3B(fprintf(pLogFile3, "__ET9SearchGetNextStd, removing code from first set, code = %d\n", wFirstCode);)
        }

        /* loop over positions that must "match" */

        bPosIndex = 0;

        while (bPosIndex < bRegCmpLength) {

            bPos = pbRegPosCurrOrder[bPosIndex];

            WLOG3B(fprintf(pLogFile3, "__ET9SearchGetNextStd, bPos = %d, dwItem = %d\n", bPos, dwCurrItem);)

            pCursor = &pCursors[bPos];

            __ET9SearchCatchupStream(pLingInfo, pCursor, bPos, dwCurrItem);

            /* if match continue with next pos */

            if (__IsCodeActive(bPos, pCursor->wCode)) {
                ++bPosIndex;
                continue;
            }

            /* no match, skip this interval */

            dwCurrItem = pCursor->dwEndPos;

            /* no match, restart the comparison run */

            WLOG3B(fprintf(pLogFile3, "__ET9SearchGetNextStd, no match, restart\n");)

            bPosIndex = 0;

        } /* while more positions */

    } /* bSpcActive */

    /* get non stem chars */

    {
        ET9U8 bZeroPos = bPosCount;

        pCursor = &pCursors[bPosIndex];

        for (; bPosIndex < bPosCount; ++bPosIndex, ++pCursor) {

            WLOG3B(fprintf(pLogFile3, "__ET9SearchGetNextStd, non stem part, bPos = %d\n", bPosIndex);)

            __ET9SearchCatchupStream(pLingInfo, pCursor, bPosIndex, dwCurrItem);

            /* check for EOS */

            if (pCursor->wCode == wCodeZero) {
                bZeroPos = bPosIndex;
                break;
            }
        }

        /* length */

        *ALDB.search.pwLength = bZeroPos;
    }

    /* store locals */

    ALDB.search.dwCurrItem = dwCurrItem;

    /* done */

    WLOG3B({
        ET9U16 wIndex;

        fprintf(pLogFile3, "__ET9SearchGetNextStd, found one, length = %d, dwItem = %d, word = ", *ALDB.search.pwLength, dwCurrItem);

        for (wIndex = 0; wIndex < *ALDB.search.pwLength; ++wIndex) {

            ET9SYMB sSymb = ALDB.search.psTarget[wIndex];

            if (sSymb >= 0x20 && sSymb <= 0x7F) {
                fprintf(pLogFile3, "%c", (char)sSymb);
            }
            else {
                fprintf(pLogFile3, "<%x>", (int)sSymb);
            }
        }

        fprintf(pLogFile3, "\n");
    })
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Move the search forward to the next matching word - flex method.
 * This function has great impact on over all performance.
 * This code is partly LDB format dependent, also highly optimized for spell correction performance.
 * <br><br>*** Do NOT modify this code without proper performance benchmarks!!!
 *
 * @param pLingInfo                 Pointer to alphabetic information structure.
 *
 * @return None
 */

static void ET9LOCALCALL __ET9SearchGetNextFlex (ET9AWLingInfo * const pLingInfo)
{
    ET9AWLingCmnInfo * const pLingCmnInfo = pLingInfo->pLingCmnInfo;

#ifdef ET9ASPC_DEV_SCREEN

    ET9ASPCFlexCompareData  * const pCMP = &ASPC.u.sCmpDataFlex;

#endif

    __ET9HeaderConstsGetNextInterval

    const ET9U8     bPosCount           = ALDB.header.bPosCount;
    const ET9U16    wCodeZero           = ALDB.header.wCodeZero;

    const ET9U8     bMatchPos           = 0;
    const ET9U8     bNonZeroPos         = ALDB.search.bSpcNonZeroPos;

    ET9ALdbCursorData * const pCursors = ALDB.pCursors;

    ET9ALdbCursorData * const pMatchCursor = &pCursors[bMatchPos];
    ET9ALdbCursorData * const pNonZeroCursor = &pCursors[bNonZeroPos];

    ET9U32 dwCurrItem = ALDB.search.dwCurrItem + 1;

    WLOG3B(fprintf(pLogFile3, "__ET9SearchGetNextFlex, dwCurrItem = %6d\n", dwCurrItem);)

    /* should not already be exhausted */

    ET9Assert(!__IsExhausted());

    /* find a "match" */

    for (;;) {

        WLOG3B(fprintf(pLogFile3, "  @ %u\n", dwCurrItem);)

        __ET9SearchCatchupStream(pLingInfo, pMatchCursor, bMatchPos, dwCurrItem);

        if (!__IsCodeActiveSpc(ALDB.compare.pppbSpcFlexSection, bMatchPos, pMatchCursor->wCode)) {

            WLOG3B(fprintf(pLogFile3, "  no match, skipping %u\n", (pMatchCursor->dwEndPos - dwCurrItem));)

            dwCurrItem = pMatchCursor->dwEndPos;
            continue;
        }

        __ET9SearchCatchupStreamCounters(pLingInfo, pNonZeroCursor, bNonZeroPos, dwCurrItem);

        if (pNonZeroCursor->wCode == wCodeZero) {

            WLOG3B(fprintf(pLogFile3, "  too short, skipping %u\n", (pNonZeroCursor->dwEndPos - dwCurrItem));)

            dwCurrItem = pNonZeroCursor->dwEndPos;
            continue;
        }

        /* exact matching? */

        if (ALDB.compare.bSpcExactFilterTrace && !ALDB.compare.bSpcExactPrunedTrace && dwCurrItem > ALDB.header.dwTopCount) {

            ALDB.compare.bSpcExactPrunedTrace = 1;
            ALDB.compare.pppbSpcFlexSection = __GetActiveSectionFlexSpc(1);
        }

        /* get the rest */

        {
#ifdef ET9ASPC_DEV_SCREEN
            const ET9U16 wPrevWordLength = *ALDB.search.pwLength;
#endif

            ET9U8 bPosIndex = 1;

            ET9ALdbCursorData * pCursor = &pCursors[bPosIndex];

            for (; bPosIndex < bPosCount; ++bPosIndex, ++pCursor) {

                WLOG3B(fprintf(pLogFile3, "  + bPos = %d\n", bPosIndex);)

                __ET9SearchCatchupStreamCounters(pLingInfo, pCursor, bPosIndex, dwCurrItem);

                /* check for EOS */

                if (pCursor->wCode == wCodeZero) {
                    *ALDB.search.pwLength = bPosIndex;
                    break;
                }
            }

            if (bPosIndex >= bPosCount) {
                *ALDB.search.pwLength = bPosIndex;
            }

#ifdef ET9ASPC_DEV_SCREEN
            for (; bPosIndex < wPrevWordLength; ++bPosIndex, ++pCursor) {

                WLOG3B(fprintf(pLogFile3, "  + bPos = %d\n", bPosIndex);)

                __ET9SearchCatchupStreamCounters(pLingInfo, pCursor, bPosIndex, dwCurrItem);
            }
#endif
        }

        __STAT_AWLdb_Flex_EDLL_Cnt;

#ifdef ET9ASPC_DEV_SCREEN

        /* supported? */

        if (pCMP->nSupportedQualityCount < (pCMP->nActiveQualityCount - pLingCmnInfo->Private.bCurrMaxEditDistance)) {
            ++dwCurrItem;
            __STAT_AWLdb_Flex_EDLLS_Cnt;
            WLOG3(fprintf(pLogFile3, " __ET9SearchGetNextFlex, screening, nSupportedQualityCount %2u, nActiveQualityCount %2u, bCurrMaxEditDistance %u\n", pCMP->nSupportedQualityCount, pCMP->nActiveQualityCount, pLingCmnInfo->Private.bCurrMaxEditDistance);)
            continue;
        }

#endif

        /* found one */

        break;
    }

    /* store locals */

    ALDB.search.dwCurrItem = dwCurrItem;

    /* done */

    WLOG3B({
        ET9U16 wIndex;

        fprintf(pLogFile3, "__ET9SearchGetNextFlex, found one, length = %d, dwItem = %d, word = ", *ALDB.search.pwLength, dwCurrItem);

        for (wIndex = 0; wIndex < *ALDB.search.pwLength; ++wIndex) {

            ET9SYMB sSymb = ALDB.search.psTarget[wIndex];

            if (sSymb >= 0x20 && sSymb <= 0x7F) {
                fprintf(pLogFile3, "%c", (char)sSymb);
            }
            else {
                fprintf(pLogFile3, "<%x>", (int)sSymb);
            }
        }

        fprintf(pLogFile3, "\n");
    })
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Start up the search mechanism for a new run.
 * Values that are used a lot during serahc will be pre-calculated here.<br>
 * All stream cursors will be initiated here.<br>
 * After this call the search will have found the first word, or become exhausted.
 *
 * @param pLingInfo                 Pointer to alphabetic information structure.
 * @param psTarget                  Pointer to where to store current word (unicode).
 * @param pwLength                  Pointer to where to store current word length.
 * @param wTargetLength             Max size of target string (symbols/characters).
 * @param bUsingFlex                If targeting flex compare.
 *
 * @return None
 */

static void ET9LOCALCALL __ET9SearchStart (ET9AWLingInfo    * const pLingInfo,
                                           ET9SYMB          * const psTarget,
                                           ET9U16           * const pwLength,
                                           const ET9U16             wTargetLength,
                                           const ET9BOOL            bUsingFlex)
{
    ET9AWLingCmnInfo * const pLingCmnInfo = pLingInfo->pLingCmnInfo;

    ET9U8 bIndex;
    ET9ALdbCursorData *pCursor;

    WLOG3(fprintf(pLogFile3, "__ET9SearchStart, wTargetLength = %d\n", wTargetLength);)

    ALDB.search.psTarget = psTarget;
    ALDB.search.pwLength = pwLength;
    ALDB.search.wTargetLength = wTargetLength;

    ALDB.search.bExhausted = (ET9BOOL)(wTargetLength < ALDB.header.bPosCount ||
                                       (ALDB.compare.wLength > ALDB.header.bPosCount && !bUsingFlex) ||
                                       (pLingCmnInfo->Private.wCurrMinSourceLength >= ALDB.header.bPosCount));

    /* set up non zero pos and the spc control pos */

    if (ALDB.compare.bSpcActive || bUsingFlex) {

        /* the non zero pos is the same as the min length */

        ALDB.search.bSpcNonZeroPos = (ET9U8)pLingCmnInfo->Private.wCurrMinSourceLength;

        if (ALDB.search.bSpcNonZeroPos) {
            --ALDB.search.bSpcNonZeroPos;
        }

        ALDB.search.bSpcControlPos = (ET9U8)pLingCmnInfo->Private.wCurrMaxSourceLength;

        if (ALDB.search.bSpcControlPos >= ALDB.header.bPosCount) {
            ALDB.search.bSpcControlPos = 0;
        }
    }
    else {
        ALDB.search.bSpcNonZeroPos = 0;
        ALDB.search.bSpcControlPos = 0;
    }

    WLOG3(fprintf(pLogFile3, "__ET9SearchStart, wSpcNonZeroPos = %d, wSpcControlPos = %d\n", ALDB.search.bSpcNonZeroPos, ALDB.search.bSpcControlPos);)

    ET9Assert(ALDB.search.bSpcNonZeroPos < ALDB.header.bPosCount || ALDB.search.bExhausted);

    /* set up word length info */

    ALDB.search.bCurrWordLength = 0;
    ALDB.search.dwWordLengthEndPos = 0;

    /* set up pos order for regular compare (don't fuzz with array pointers) */

    {
        ET9U8 bOrderIndex = 0;

        ALDB.search.bRegCmpLength = bUsingFlex? 1 : ALDB.compare.bActiveCmpLength;

        for (bIndex = 0; bOrderIndex < ALDB.search.bRegCmpLength; ++bIndex) {
            if (ALDB.header.pbPosOrder[bIndex] < ALDB.search.bRegCmpLength) {
                ET9Assert(bOrderIndex < ET9MAXLDBWORDSIZE);
                ALDB.search.pbRegPosCurrOrder[bOrderIndex++] = ALDB.header.pbPosOrder[bIndex];
            }
        }
        for (bIndex = (ET9U8)ALDB.search.bRegCmpLength; bIndex < ALDB.header.bPosCount; ++bIndex) {
            ET9Assert(bOrderIndex < ET9MAXLDBWORDSIZE);
            ALDB.search.pbRegPosCurrOrder[bOrderIndex++] = bIndex;
        }

        WLOG3({
            fprintf(pLogFile3, "__ET9SearchStart, reg pos order: ");

            for (bIndex = 0; bIndex < ALDB.header.bPosCount; ++bIndex) {
                fprintf(pLogFile3, "%2d ", ALDB.search.pbRegPosCurrOrder[bIndex]);
                }

            fprintf(pLogFile3, "\n");
        })
    }

    /* set up cursors */

    pCursor = ALDB.pCursors;
    for (bIndex = 0; bIndex < ALDB.header.bPosCount; ++bIndex, ++pCursor) {

        pCursor->wCode = 0;
        pCursor->dwStartPos = 0;
        pCursor->dwEndPos = 0;
        pCursor->dwJumpPos = 0;
        pCursor->dwJumpAddress = 0;

        {
            ET9U8 ET9FARDATA_LDB *pbCurrData;

            __LdbSetIndex(pCursor, pCursor->dwSourceDataStart);

            pCursor->pbCurrData = pbCurrData;
        }

        if (!__IsExhausted()) {
            __ET9SearchGetNextIntervalSlow(pLingInfo, pCursor, bIndex, 0, bUsingFlex);
        }
    }

    /* set up initial search mode info */

    if (ALDB.search.bSpcControlPos) {
        ALDB.search.bSpcCompare = (ET9BOOL)(ALDB.pCursors[ALDB.search.bSpcControlPos].wCode == ALDB.header.wCodeZero);
        ALDB.search.dwSpcControlEndPos = ALDB.pCursors[ALDB.search.bSpcControlPos].dwEndPos;
    }
    else {
        ALDB.search.bSpcCompare = 1;
        ALDB.search.dwSpcControlEndPos = ~((ET9U32)0);
    }
    ALDB.search.dwRegNonMatchEndPos = 0;

    /* move to the first word */

    if (!__IsExhausted()) {

        /* __ET9SearchGetNext moves forward one step, compensate by moving one back first, to get to the first one... */

        ALDB.search.dwCurrItem = 0;

        --ALDB.search.dwCurrItem;

        /* need a defined value for first pos opt (anything but '1') */

        *ALDB.search.pwLength = 0;

        /* move */

        if (bUsingFlex) {
            __ET9SearchGetNextFlex(pLingInfo);
        }
        else {
            __ET9SearchGetNextStd(pLingInfo);
        }
    }
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Initiate the search mechanism for a new language.
 * This will read out header info from the LDB chunk and store it in the ALDB structure for faster access.
 *
 * @param pLingInfo                 Pointer to alphabetic information structure.
 * @param dwChunkStartPos           Offset to where in the binary the LDB info resides.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

static ET9STATUS ET9LOCALCALL __ET9SearchInit (ET9AWLingInfo * const pLingInfo,
                                               const ET9U32          dwChunkStartPos)
{
    ET9AWLingCmnInfo * const pLingCmnInfo = pLingInfo->pLingCmnInfo;
    ET9WordSymbInfo * const pWordSymbInfo   = pLingCmnInfo->Base.pWordSymbInfo;

    ET9U32            dwIndex;
    ET9U32            dwStartPos = dwChunkStartPos;
    ET9ALdbCursorData *pCursor;
    ET9SYMB           *pSymb;
    ET9U8             *pbItem;
    ET9U16            *pwItem;
    ET9U32            *pDW;

    /* Number of character positions (max word length) - 1 byte */

    ALDB.header.bPosCount = _ET9ReadLDBByte(pLingInfo, dwStartPos++);

    if (!ALDB.header.bPosCount || ALDB.header.bPosCount > ET9MAXLDBWORDSIZE) {
        return ET9STATUS_CORRUPT_DB;
    }

    /* Position order data - 1 byte per position */

    pbItem = ALDB.header.pbPosOrder;
    for (dwIndex = 0; dwIndex < ALDB.header.bPosCount; ++dwIndex) {

        *pbItem++ = _ET9ReadLDBByte(pLingInfo, dwStartPos++);
    }

    /* Word length end (last) data - 3 bytes per position (obsolete) */

    dwStartPos += 3 * ALDB.header.bPosCount;

    /* Character decode data - 2 byte count plus 2 bytes per item */

    ALDB.header.wCharacterDecodeCount = _ET9ReadLDBWord2(pLingInfo, dwStartPos);

    dwStartPos += 2;

    if (ALDB.header.wCharacterDecodeCount > ALDB_HEADER_MAX_CHAR_CODES) {
        return ET9STATUS_CORRUPT_DB;
    }

    pSymb = ALDB.header.psCharacterDecodeTable;
    for (dwIndex = 0; dwIndex < ALDB.header.wCharacterDecodeCount; ++dwIndex) {

        *pSymb++ = _ET9ReadLDBWord2(pLingInfo, dwStartPos);

        dwStartPos += 2;
    }

    /* One byte interval decode data (codes + lengths) - 2 * 255 bytes */

    pbItem = ALDB.header.pbOneByteCodes;
    for (dwIndex = 0; dwIndex < ALDB_HEADER_ONE_BYTE_SIZE; ++dwIndex) {

        *pbItem++ = _ET9ReadLDBByte(pLingInfo, dwStartPos++);
    }
    *pbItem = 0xFF;    /* really not in play more than for performance purposes (unused or rare code) */

    pbItem = ALDB.header.pbOneByteLengths;
    for (dwIndex = 0; dwIndex < ALDB_HEADER_ONE_BYTE_SIZE; ++dwIndex) {

        *pbItem++ = _ET9ReadLDBByte(pLingInfo, dwStartPos++);
    }

    /* Start offset for each position's interval data stream - 3 bytes per (position + 1) */

    pDW = ALDB.header.pdwIntervalOffsets;
    for (dwIndex = 0; dwIndex <= ALDB.header.bPosCount; ++dwIndex) {

        *pDW++ = _ET9ReadLDBWord3(pLingInfo, dwStartPos);

        dwStartPos += 3;
    }

    /* Locate the control codes */

    ALDB.header.wCodeZero = 0xFFFF;
    ALDB.header.wCodeIntervalEnd = 0xFFFF;
    ALDB.header.wCodeIntervalJump = 0xFFFF;
    ALDB.header.wCodeIntervalExtend = 0xFFFF;

    pSymb = ALDB.header.psCharacterDecodeTable;
    for (dwIndex = 0; dwIndex < ALDB.header.wCharacterDecodeCount; ++dwIndex, ++pSymb) {

        if (*pSymb == 0) {

            ALDB.header.wCodeZero = (ET9U16)dwIndex;
        }
        else if (*pSymb == ALDB_INTERVAL_END_CHAR_VAL) {

            ALDB.header.wCodeIntervalEnd = (ET9U16)dwIndex;
        }
        else if (*pSymb == ALDB_INTERVAL_JUMP_CHAR_VAL) {

            ALDB.header.wCodeIntervalJump = (ET9U16)dwIndex;
        }
        else if (*pSymb == ALDB_INTERVAL_EXTEND_CHAR_VAL) {

            ALDB.header.wCodeIntervalExtend = (ET9U16)dwIndex;
        }
    }

    if (ALDB.header.wCodeZero == 0xFFFF ||
        ALDB.header.wCodeIntervalEnd == 0xFFFF ||
        ALDB.header.wCodeIntervalJump == 0xFFFF ||
        ALDB.header.wCodeIntervalExtend == 0xFFFF) {

        return ET9STATUS_CORRUPT_DB;
    }

    /* Recreate character encode info - also check if there are upper/lower case characters */

    ALDB.header.bLowerCount = 0;
    ALDB.header.bUpperCount = 0;

    pwItem = ALDB.header.pwCharacterEncodeTable;
    for (dwIndex = 0; dwIndex < ALDB_HEADER_MAX_DIRECT_ENCODE; ++dwIndex) {
        *pwItem++ = ALDB_CHAR_CODE_NONE;
    }

    ALDB.header.wCharacterEncodeExtendCount = 0;
    ALDB.header.sCharacterEncodeExtendFirstChar = 0xFFFF;
    ALDB.header.sCharacterEncodeExtendLastChar = 0;

    pSymb = ALDB.header.psCharacterDecodeTable;
    for (dwIndex = 0; dwIndex < ALDB.header.wCharacterDecodeCount; ++dwIndex, ++pSymb) {

        if (*pSymb == ALDB_INTERVAL_END_CHAR_VAL ||
            *pSymb == ALDB_INTERVAL_JUMP_CHAR_VAL ||
            *pSymb == ALDB_INTERVAL_EXTEND_CHAR_VAL) {

            continue;
        }

        /* Check to see if sym has table support; if not, flag LDB as incompatible */

        if (*pSymb && _ET9_GetSymbolClass(*pSymb) == ET9_UnassSymbClass) {
            return ET9STATUS_DB_CORE_INCOMP;
        }

        if (_ET9SymIsLower(*pSymb, pWordSymbInfo->Private.wLocale)) {
            ++ALDB.header.bLowerCount;
        }
        else if (_ET9SymIsUpper(*pSymb, pWordSymbInfo->Private.wLocale)) {
            ++ALDB.header.bUpperCount;
        }

        if (*pSymb >= ALDB_HEADER_MAX_DIRECT_ENCODE) {

            ++ALDB.header.wCharacterEncodeExtendCount;

            if (ALDB.header.sCharacterEncodeExtendFirstChar > *pSymb) {
                ALDB.header.sCharacterEncodeExtendFirstChar = *pSymb;
            }
            if (ALDB.header.sCharacterEncodeExtendLastChar < *pSymb) {
                ALDB.header.sCharacterEncodeExtendLastChar = *pSymb;
            }

            continue;
        }

        ALDB.header.pwCharacterEncodeTable[*pSymb] = (ET9U16)dwIndex;
    }

    /* Const cursor info */

    pCursor = ALDB.pCursors;
    for (dwIndex = 0; dwIndex < ALDB.header.bPosCount; ++dwIndex, ++pCursor) {

        pCursor->dwSourceDataStart = ALDB.header.pdwIntervalOffsets[dwIndex];
        pCursor->dwSourceDataLength = ALDB.header.pdwIntervalOffsets[dwIndex + 1] - ALDB.header.pdwIntervalOffsets[dwIndex];

#ifdef ET9_DIRECT_LDB_ACCESS
#else
        pCursor->pbCacheEnd = &pCursor->pbCache[ALDB_CURSOR_DATA_CACHE_SIZE];
#endif /* ET9_SINGLE_LDB_READ_BUFFER */

    }

    /* */

    _ET9ClearMem((ET9U8*)ALDB.compare.pbCodeIsFree, sizeof(ALDB.compare.pbCodeIsFree));

    {
        ET9U16 wCode;
        ET9SYMB *psSymb;

        psSymb = ALDB.header.psCharacterDecodeTable;
        for (wCode = 0; wCode < ALDB.header.wCharacterDecodeCount; ++wCode, ++psSymb) {

            if (_ET9_IsFree(*psSymb)) {
                __SetCodeIsFree(wCode);
            }
        }
    }

#ifdef ET9_DEBUGLOG3
    {
        ET9U16 wCode;
        ET9SYMB *psSymb;

        WLOG3(fprintf(pLogFile3, "\n=== Character Decode Table ===\n");)

        psSymb = ALDB.header.psCharacterDecodeTable;
        for (wCode = 0; wCode < ALDB.header.wCharacterDecodeCount; ++wCode, ++psSymb) {
            if (*psSymb >= 0x20 && *psSymb <= 0x7F) {
                WLOG3(fprintf(pLogFile3, "%c\n", (char)*psSymb);)
            }
            else {
                WLOG3(fprintf(pLogFile3, "<%x>\n", (int)*psSymb);)
            }
        }

        WLOG3(fprintf(pLogFile3, "\n");)
    }
#endif

    /* Done */

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Investigate the LDB word count (not an attribute in the header).
 *
 * @param pLingInfo                 Pointer to alphabetic information structure.
 *
 * @return None
 */

static void ET9LOCALCALL __InvestigateWordCount (ET9AWLingInfo * const pLingInfo)
{
    ET9AWLingCmnInfo * const pLingCmnInfo = pLingInfo->pLingCmnInfo;

    ET9U8 bLastPos = ALDB.header.bPosCount - 1;

    ET9ALdbCursorData *pCursor = &ALDB.pCursors[bLastPos];

    ET9AWPrivWordInfo sWord;

    pCursor->wCode = 0;
    pCursor->dwStartPos = 0;
    pCursor->dwEndPos = 0;
    pCursor->dwJumpPos = 0;
    pCursor->dwJumpAddress = 0;

    {
        ET9U8 ET9FARDATA_LDB *pbCurrData;

        __LdbSetIndex(pCursor, pCursor->dwSourceDataStart);

        pCursor->pbCurrData = pbCurrData;
    }

    ALDB.search.bExhausted = 0;
    ALDB.search.psTarget = sWord.Base.sWord;
    ALDB.search.pwLength = &sWord.Base.wWordLen;
    ALDB.search.wTargetLength = ET9MAXWORDSIZE;

    __ET9SearchGetNextIntervalSlow(pLingInfo, pCursor, bLastPos, 0, 0);

    while (!__IsExhausted()) {
        __ET9SearchGetNextIntervalSlow(pLingInfo, pCursor, bLastPos, pCursor->dwEndPos, 0);
    }

    ALDB.header.dwWordCount = pCursor->dwEndPos;
    ALDB.header.dwTopCount = (ET9U32)(ALDB.header.dwWordCount / 20);
}

/* *************************************************************************
   *********************** ACCESS ******************************************
   ************************************************************************* */

#undef ET9_SPC_ED_GetMatchInfo
#undef ET9_SPC_ED_GetMatchOnly
#undef ET9_SPC_ED_GetWordSymb
#undef ET9_SPC_ED_USE_COMPARE_CACHE
#undef __ET9AWCalcEditDistance

/*---------------------------------------------------------------------------*/
/** \internal
 * Get match info, plus frequency and lock info.
 * Macro for adopting the LDB source to __ET9AWCalcEditDistance.
 *
 * @param index1                    Index in source string (input).
 * @param index2                    Index in compared string (source word).
 * @param bExactMatchOnly           If only to perform exact match.
 * @param pbMatch                   (out) the resulting match info.
 * @param pbFreq                    (out) the resulting match frequency.
 * @param pbLocked                  (out) lock information.
 *
 * @return None
 */

#define ET9_SPC_ED_GetMatchInfo(index1, index2, bExactMatchOnly, pbMatch, pbFreq, pbLocked)             \
{                                                                                                       \
    pbFreq = (ET9U8)__GetCodeFreq((index1), __LdbGetCharCodeAtPos(index2));                             \
                                                                                                        \
    if (pbFreq) {                                                                                       \
        if (__IsCodeExact((index1), __LdbGetCharCodeAtPos(index2))) {                                   \
            pbMatch = ET9_SPC_ED_MATCH_EXACT;                                                           \
        }                                                                                               \
        else {                                                                                          \
            pbMatch = ET9_SPC_ED_MATCH_FULL;                                                            \
        }                                                                                               \
    }                                                                                                   \
    else {                                                                                              \
        pbMatch = ET9_SPC_ED_MATCH_NONE;                                                                \
    }                                                                                                   \
                                                                                                        \
    pbLocked = __GetPosLock(index1);                                                                    \
                                                                                                        \
    __STAT_INC_CmpLdbInfo;                                                                              \
}                                                                                                       \

/*---------------------------------------------------------------------------*/
/** \internal
 * Get match info only.
 * Macro for adopting the LDB source to __ET9AWCalcEditDistance.
 *
 * @param index1                    Index in source string (input).
 * @param index2                    Index in compared string (source word).
 * @param bExactMatchOnly           If only to perform exact match.
 * @param pbMatch                   (out) the resulting match info.
 *
 * @return None
 */

#define ET9_SPC_ED_GetMatchOnly(index1, index2, bExactMatchOnly, pbMatch)                               \
{                                                                                                       \
    if (__IsCodeActive((index1), __LdbGetCharCodeAtPos(index2))) {                                      \
        if (__IsCodeExact((index1), __LdbGetCharCodeAtPos(index2))) {                                   \
            pbMatch = ET9_SPC_ED_MATCH_EXACT;                                                           \
        }                                                                                               \
        else {                                                                                          \
            pbMatch = ET9_SPC_ED_MATCH_FULL;                                                            \
        }                                                                                               \
    }                                                                                                   \
    else {                                                                                              \
        pbMatch = ET9_SPC_ED_MATCH_NONE;                                                                \
    }                                                                                                   \
                                                                                                        \
    __STAT_INC_CmpLdbOnly;                                                                              \
}                                                                                                       \

/*---------------------------------------------------------------------------*/
/** \internal
 * Get word symbol (character in a word).
 * Macro for adopting the LDB source to __ET9AWCalcEditDistance.
 *
 * @param index                     Index in compared string (source word).
 * @param psSymb                    (out) the symbol.
 *
 * @return None
 */

#define ET9_SPC_ED_GetWordSymb(index, psSymb)                                                           \
{                                                                                                       \
    (psSymb) = ALDB.search.psTarget[index];                                                             \
}                                                                                                       \

/*---------------------------------------------------------------------------*/
/** \internal
 * Function (name) for edit distance calculation for the LDB source.
 * The "include" will bring in a static function making use of the other source adopting macros.<br>
 * This macro really sets up the name of the __ET9AWCalcEditDistance function for the ALDB.<br>
 * The name change is necessary since the function will be instantiated in more than one place
 * and it's possible that all of them will be compiled as one big C file.<br>
 * This way of doing things is necessary for reasons of performance and code management.
 */

#define __ET9AWCalcEditDistance __ET9AWCalcEditDistanceALDB

#include "et9aspci.h"

/* *************************************************************************
   *********************** HIGHER LEVEL ************************************
   ************************************************************************* */

/*---------------------------------------------------------------------------*/
/** \internal
 * Get the class of a word from the LM.
 *
 * @param pLingInfo                 Pointer to alphabetic information structure.
 * @param dwIndex                   Word index.
 *
 * @return Class
 */

static ET9U16 ET9LOCALCALL __ET9AWLdbGetWordClass(ET9AWLingInfo * const         pLingInfo,
                                                  const ET9U32                    dwIndex)
{
     ET9AWLingCmnInfo * const pLingCmnInfo = pLingInfo->pLingCmnInfo;
     ET9U16 wWordClass = 0;

     ET9Assert(pLingInfo);

     if (pLingCmnInfo->Private.ALdbLM.wNumClasses == 256) { /* Single byte */

         __ET9ReadLDBData(pLingInfo,
                         pLingCmnInfo->Private.ALdbLM.dwALMStartAddress + dwIndex,
                         1,
                         (ET9U8 *) &wWordClass);
    }
    else if (pLingCmnInfo->Private.ALdbLM.wNumClasses == 512) { /* Double byte */

        __ET9ReadLDBWord(pLingInfo,
                         pLingCmnInfo->Private.ALdbLM.dwALMStartAddress + (dwIndex << 1),
                         (ET9U16 *) &wWordClass);
    }

    return wWordClass;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Get the context word class from the LM.
 *
 * @param pLingInfo                 Pointer to alphabetic information structure.
 *
 * @return NONE
 */

void ET9FARCALL _ET9AWLdbTagContextClass(ET9AWLingInfo * const pLingInfo)
{
    ET9U32 dwContextWordIndex = 0;
    ET9U8 bExact = 0;
    ET9U8 bLowercase = 0;
    ET9U8 bWordLen = 0;
    ET9U8 bSubWordLen = 0;
    ET9SYMB sSubWord[ET9MAXWORDSIZE];
    ET9U8 bPunctSubWordLen = 0;
    ET9SYMB sPunctSubWord[ET9MAXWORDSIZE];
    ET9U8 bCharIndex;
    ET9BOOL bFoundBuildAround = 0;
    ET9STATUS wStatus = ET9STATUS_NONE;
    ET9AWLingCmnInfo * const pLingCmnInfo = pLingInfo->pLingCmnInfo;

    ET9Assert(pLingInfo);

    pLingCmnInfo->Private.wContextWordClass = 0;

    if (!ET9LMENABLED(pLingCmnInfo) ||
        !pLingCmnInfo->Private.ALdbLM.bSupported ||
        !pLingCmnInfo->Private.bContextWordSize ||
        pLingInfo->Private.wLDBInitOK != ET9GOODSETUP) {

        return;
    }

    if ((wStatus = _ET9AWLdbFindEntry(pLingInfo,
                                      pLingCmnInfo->wLdbNum,
                                      pLingCmnInfo->Private.sContextWord,
                                      pLingCmnInfo->Private.bContextWordSize,
                                      &dwContextWordIndex,
                                      &bExact,
                                      &bLowercase)) == ET9STATUS_WORD_EXISTS) { /* exact match or lower case variant available */

    }
    else { /* track for buildarounds */

        for (bWordLen = pLingCmnInfo->Private.bContextWordSize; bWordLen > 0; --bWordLen) {
            if (_ET9_IsPunctChar(pLingCmnInfo->Private.sContextWord[bWordLen - 1])) {
                for (bCharIndex = 0; bWordLen <= pLingCmnInfo->Private.bContextWordSize; bCharIndex++, bWordLen++) {
                    sPunctSubWord[bCharIndex] = pLingCmnInfo->Private.sContextWord[bWordLen - 1];
                    if (bCharIndex) {
                        sSubWord[bCharIndex - 1] = pLingCmnInfo->Private.sContextWord[bWordLen - 1];
                    }
                }
                bPunctSubWordLen = bCharIndex;
                bSubWordLen = bCharIndex - 1;
                bFoundBuildAround = 1;
                break;
            }
        }

        if (bFoundBuildAround) {
            if ((wStatus = _ET9AWLdbFindEntry(pLingInfo,
                                              pLingCmnInfo->wLdbNum,
                                              sPunctSubWord,
                                              bPunctSubWordLen,
                                              &dwContextWordIndex,
                                              &bExact,
                                              &bLowercase)) == ET9STATUS_WORD_EXISTS) { /* exact match or lowercase variant available */

            }
            else if ((wStatus = _ET9AWLdbFindEntry(pLingInfo,
                                                   pLingCmnInfo->wLdbNum,
                                                   sSubWord,
                                                   bSubWordLen,
                                                   &dwContextWordIndex,
                                                   &bExact,
                                                   &bLowercase)) == ET9STATUS_WORD_EXISTS) { /* exact match or lowercase variant available */

            }
        }
    }

    /* Get previous word class */

    if (wStatus == ET9STATUS_WORD_EXISTS && (bExact || bLowercase)) {
        pLingCmnInfo->Private.wContextWordClass = __ET9AWLdbGetWordClass(pLingInfo, dwContextWordIndex);
    }
}

/*---------------------------------------------------------------------------*/
/** \internal
 * This function returns the frequency for the word index in active/current LDB.
 *
 * @param pLingInfo                 Pointer to alphabetic information structure.
 * @param wLdbNum                   Language ID.
 * @param dwIndex                   Index to word.
 * @param pxWordFreq                Pointer to word frequency.
 * @param pwEWordFreq               Pointer to word emission frequency.
 * @param pwTWordFreq               Pointer to word transition frequency.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL _ET9AWLdbGetWordFreq(ET9AWLingInfo     * const pLingInfo,
                                          const ET9U16              wLdbNum,
                                          const ET9U32              dwIndex,
                                          ET9FREQPART       * const pxWordFreq,
                                          ET9U16            * const pwEWordFreq,
                                          ET9U16            * const pwTWordFreq)
{
    ET9U16 wCurrentWordClass = 0;
    ET9U16 wEmissionScore = 0;
    ET9U16 wTransitionScore = 0;
    ET9U32 dwAddressTracker = 0;
    ET9U32 dwTransitionTableEntry = 0;
    ET9AWLingCmnInfo * const pLingCmnInfo = pLingInfo->pLingCmnInfo;

    ET9Assert(pLingInfo);

    if (wLdbNum != pLingCmnInfo->wLdbNum) {
        return ET9STATUS_LDB_ID_ERROR;
    }

    if (ET9LMENABLED(pLingCmnInfo) &&
        pLingCmnInfo->Private.ALdbLM.bSupported &&
        pLingInfo->Private.wLDBInitOK == ET9GOODSETUP) {

        const ET9U16 wMaxFreq = pLingCmnInfo->Private.ALdbLM.wEBits > pLingCmnInfo->Private.ALdbLM.wTBits ? (ET9U16) (1 << (pLingCmnInfo->Private.ALdbLM.wEBits + 1)) : (ET9U16) (1 << (pLingCmnInfo->Private.ALdbLM.wTBits + 1));

        ET9Assert(dwIndex < pLingInfo->pLingCmnInfo->Private.ALdbLM.dwNumEntries);

        /* Get word class */

        wCurrentWordClass = __ET9AWLdbGetWordClass(pLingInfo, dwIndex);

        if (pLingCmnInfo->Private.ALdbLM.wNumClasses == 256) {
            dwAddressTracker = pLingCmnInfo->Private.ALdbLM.dwALMStartAddress + pLingCmnInfo->Private.ALdbLM.dwNumEntries;
        }
        else if (pLingCmnInfo->Private.ALdbLM.wNumClasses == 512) {
            dwAddressTracker = pLingCmnInfo->Private.ALdbLM.dwALMStartAddress + (pLingCmnInfo->Private.ALdbLM.dwNumEntries << 1);
        }

        if (pLingCmnInfo->Private.ALdbLM.wEBits == 4) {
            __ET9ReadLDBData(pLingInfo,
                             dwAddressTracker + (dwIndex >> 1),
                             1,
                             (ET9U8*)&wEmissionScore);

            if (dwIndex % 2) { /* odd is trailing and even is leading */
                wEmissionScore &= 0x0F;
            }
            else {
                wEmissionScore >>= 4;
            }
        }
        else if (pLingCmnInfo->Private.ALdbLM.wEBits == 8) {

            __ET9ReadLDBData(pLingInfo,
                             dwAddressTracker + dwIndex,
                             1,
                             (ET9U8*)&wEmissionScore);
        }
        else if (pLingCmnInfo->Private.ALdbLM.wEBits == 16) {

            __ET9ReadLDBData(pLingInfo,
                             dwAddressTracker + (dwIndex << 1),
                             2,
                             (ET9U8*)&wEmissionScore);
        }

        dwAddressTracker += ((pLingCmnInfo->Private.ALdbLM.dwNumEntries * pLingCmnInfo->Private.ALdbLM.wEBits) >> 3);

        if (pLingCmnInfo->Private.ALdbLM.dwNumEntries % 2) {
            ++dwAddressTracker;
        }

        /* Get transition score */

        dwTransitionTableEntry = pLingCmnInfo->Private.wContextWordClass * pLingCmnInfo->Private.ALdbLM.wNumClasses + wCurrentWordClass;

        if (pLingCmnInfo->Private.ALdbLM.wTBits == 4) {

            __ET9ReadLDBData(pLingInfo,
                             dwAddressTracker + (dwTransitionTableEntry >> 1),
                             1,
                             (ET9U8*)&wTransitionScore);

            if (dwTransitionTableEntry % 2) { /* odd is trailing and even is leading */
                wTransitionScore &= 0x0F;
            }
            else {
                wTransitionScore >>= 4;
            }
        }
        else if (pLingCmnInfo->Private.ALdbLM.wTBits == 8) {

            __ET9ReadLDBData(pLingInfo,
                             dwAddressTracker + dwTransitionTableEntry,
                             1,
                             (ET9U8*)&wTransitionScore);
        }
        else if (pLingCmnInfo->Private.ALdbLM.wTBits == 16) {

            __ET9ReadLDBData(pLingInfo,
                             dwAddressTracker + (dwTransitionTableEntry << 1),
                             2,
                             (ET9U8*)&wTransitionScore);

        }

        if (pLingCmnInfo->Private.ALdbLM.wTBits > pLingCmnInfo->Private.ALdbLM.wEBits) {
            wEmissionScore <<= (pLingCmnInfo->Private.ALdbLM.wTBits - pLingCmnInfo->Private.ALdbLM.wEBits);
        }
        else if (pLingCmnInfo->Private.ALdbLM.wEBits > pLingCmnInfo->Private.ALdbLM.wTBits) {
            wTransitionScore <<= (pLingCmnInfo->Private.ALdbLM.wEBits - pLingCmnInfo->Private.ALdbLM.wTBits);
        }

#ifdef ET9_USE_FLOAT_FREQS
        {
            const ET9U16 wLogFreq = (wMaxFreq - (wTransitionScore + wEmissionScore));

            *pxWordFreq = _ET9pow_f(2, wLogFreq);
        }
#else
        {
            const ET9U16 wLogFreqSub = wMaxFreq > 16 ? (wMaxFreq - 16) : 0;

            ET9U16 wLogFreq = (wMaxFreq - (wTransitionScore + wEmissionScore));

            if (wLogFreq > wLogFreqSub) {
                wLogFreq = wLogFreq - wLogFreqSub;
            }
            else {
                wLogFreq = 0;
            }

            if (wLogFreq >= 16) {
                *pxWordFreq = 0xFFFF;
            }
            else {
                *pxWordFreq = 1 << wLogFreq;
            }
        }
#endif

        if (pwEWordFreq != NULL) {
            *pwEWordFreq = wMaxFreq - wEmissionScore;
        }

        if (pwTWordFreq != NULL) {
            *pwTWordFreq = wMaxFreq - wTransitionScore;
        }
    }
    else {

#ifdef ET9_USE_FLOAT_FREQS
        *pxWordFreq = (ET9FREQPART)(1000000.0 / (dwIndex + 1));     /* zero freq becomes one in "add" */
#else
        *pxWordFreq = (ET9FREQPART)(0xFFFF / (dwIndex + 1));        /* zero freq becomes one in "add" */
#endif

        if (pwEWordFreq != NULL) {
            *pwEWordFreq = 0;
        }

        if (pwTWordFreq != NULL) {
            *pwTWordFreq = 0;
        }
    }

    ET9Assert(*pxWordFreq >= 0);

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Get all matching candidates in the LDB.
 * Given an "input" this will setup all necessary info and perform a search to get all potentially matching candidates from the LDB data.
 * Rather than fetching all LDB words and checking if they match using the match function, this will perform a low level screening in order
 * to as early as possible screen non matching words.<br>
 * When spell correction is off this is a perfect screening (only finding words that actually are candidates). With spell correction on
 * this is much more complex, and it will over generate.<br>
 *
 * @param pLingInfo                 Pointer to alphabetic information structure.
 * @param wIndex                    Input index/position (non zero if partial).
 * @param wLength                   Word length (active input length).
 * @param bFreqIndicator            Designation
 * @param bSpcMode                  Spell correction mode.
 *
 * @return None
 */

static void ET9LOCALCALL __ET9AWLdbGetCandidates (ET9AWLingInfo * const         pLingInfo,
                                                  const ET9U16                  wIndex,
                                                  const ET9U16                  wLength,
                                                  const ET9_FREQ_DESIGNATION    bFreqIndicator,
                                                  const ET9U8                   bSpcMode)
{
    ET9AWPrivWordInfo sPublicWord;
    ET9AWPrivWordInfo sPrivateWord;

    ET9AWLingCmnInfo * const pLingCmnInfo = pLingInfo->pLingCmnInfo;

    const ET9U8 bLangIndex = pLingCmnInfo->wLdbNum == pLingInfo->pLingCmnInfo->wFirstLdbNum ? ET9AWFIRST_LANGUAGE : ET9AWSECOND_LANGUAGE;

    const ET9U8 bWordSrc =  ADDON_FROM_FREQ_IND(bFreqIndicator, ET9WORDSRC_LDB);

    /* if all LDB words being downshifted, or word being compounded, downshift it */

    const ET9BOOL bDownShift = (ET9BOOL)((ET9DOWNSHIFTALLLDB(pLingCmnInfo) || wIndex) && ALDB.header.bUpperCount);

    ET9Assert(pLingInfo);

    if (!wLength || wLength > ET9MAXLDBWORDSIZE) {
        return;
    }

    if (_ET9AWCalcEditDistanceInit(pLingInfo, wIndex, wLength, bSpcMode)) {
        return;
    }

    __STAT_AWLdb_Call;

    __ResetScreenCounters();

    WLOG3(fprintf(pLogFile3, "__ET9AWLdbGetCandidates, start, wIndex = %2d, wLength = %2d, bSpcMode = %d\n", wIndex, wLength, bSpcMode);)

    _InitPrivWordInfo(&sPrivateWord);

    sPrivateWord.Base.bLangIndex = bLangIndex;

    __ET9CompareStart(pLingCmnInfo, wIndex, wLength, 0);

    __ET9SearchStart(pLingInfo, sPrivateWord.Base.sWord, &sPrivateWord.Base.wWordLen, ET9MAXWORDSIZE, 0);

    for (; !__IsExhausted(); __ET9SearchGetNextStd(pLingInfo)) {

        __STAT_INC_TotLdbCount;

        WLOG3B(fprintf(pLogFile3, "__ET9AWLdbGetCandidates, word index = %6d, word len = %2d, wLength = %2d\n", __GetWordIndex(), sPrivateWord.Base.wWordLen, wLength);)

        ET9Assert(sPrivateWord.Base.wWordLen >= wLength || ET9_SPC_IS_ACTIVE(pLingCmnInfo->Private.bCurrSpcMode));

        sPrivateWord.bWordSrc = bWordSrc;

        if (__ET9AWCalcEditDistance(pLingInfo, &sPrivateWord, wIndex, wLength)) {
            continue;
        }

        ET9Assert(sPrivateWord.xTapFreq >= 0);

        /* candidate found */

        sPublicWord = sPrivateWord;

        /* downshift word if option selected */

        if (bDownShift) {

            ET9U16 wCount = sPublicWord.Base.wWordLen;
            ET9SYMB *pSymb = sPublicWord.Base.sWord;

            for (; wCount; --wCount, ++pSymb) {
                *pSymb = _ET9SymToLower(*pSymb, pLingCmnInfo->wLdbNum);
            }
        }

        /* prepare and add to list */

        _ET9AWSelLstWordPreAdd(pLingInfo, &sPublicWord, wIndex, (ET9U8)wLength);

        {
            const ET9U32 dwCurrWordIndex = __GetWordIndex() + 1;

            sPublicWord.xWordFreq = (ET9FREQPART)((ET9FREQPART)0xFFFF / dwCurrWordIndex);   /* zero freq becomes one in "add" */
            sPublicWord.wEWordFreq = 0;
            sPublicWord.wTWordFreq = 0;
            sPublicWord.dwWordIndex = dwCurrWordIndex;
        }

        ET9Assert(sPublicWord.xTapFreq >= 0);
        ET9Assert(sPublicWord.xTotFreq >= 0);

        sPublicWord.Base.wWordCompLen = sPrivateWord.Base.wWordCompLen;

        sPublicWord.bIsTop5 = (__GetWordIndex() < ALDB.header.dwTopCount) ? 1 : 0;

        _ET9AWSelLstAdd(pLingInfo, &sPublicWord, wLength, bFreqIndicator);
    }

    __LogScreenCounters();

    _ET9AWCalcEditDistanceDone(pLingInfo);
}

/*---------------------------------------------------------------------------*/

#ifdef ET9_DEBUGLOG8
#ifdef _WIN32
#pragma message ("*** ET9_DEBUGLOG8 ACTIVATED ***")
#endif
#include <stdio.h>
#include <string.h>
#define WLOG8(q) { if (pLogFile8 == NULL) { pLogFile8 = fopen("zzzET9LDBTRACE.txt", "w"); } { q fflush(pLogFile8); } }
static FILE *pLogFile8 = NULL;
#else
#define WLOG8(q)
#endif

#ifdef ET9_DEBUGLOG8B
#ifdef _WIN32
#pragma message ("*** ET9_DEBUGLOG8 ACTIVATED ***")
#endif
#define WLOG8B(q) WLOG8(q)
#else
#define WLOG8B(q)
#endif

/*---------------------------------------------------------------------------*/
/** \internal
 * This function performs the flex edit distance calculation for a given word.
 *
 * @param pLingInfo                 Pointer to alphabetic information structure.
 * @param pWord                     Pointer to the word.
 * @param wIndex                    Input index/position (non zero if partial).
 * @param wLength                   Word length (active input length).
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

static ET9STATUS ET9LOCALCALL __LDB_CalcEditDistanceFlex(ET9AWLingInfo     * const pLingInfo,
                                                         ET9AWPrivWordInfo * const pWord,
                                                         const ET9U16              wIndex,
                                                         const ET9U16              wLength)
{
    ET9AWLingCmnInfo        * const pLingCmnInfo    = pLingInfo->pLingCmnInfo;
    ET9WordSymbInfo         * const pWordSymbInfo   = pLingCmnInfo->Base.pWordSymbInfo;
    ET9ASPCFlexCompareData  * const pCMP            = &ASPC.u.sCmpDataFlex;

    ET9U8             const * const pbLockInfo      = pCMP->pbLockInfo;
    ET9U8             const * const pbIsFreqPos     = pCMP->pbIsFreqPos;
    ET9U8             const * const pbIsQualityKey  = pCMP->pbIsQualityKey;

    const ET9U8   bMaxEditDist = pLingCmnInfo->Private.bCurrMaxEditDistance;
    const ET9U16  wCurrMinSourceLength = pLingCmnInfo->Private.wCurrMinSourceLength;

    const ET9BOOL bAllowSpcCmpl = pCMP->bAllowSpcCmpl;
    const ET9BOOL bAllowFreePunct = pCMP->bAllowFreePunct;
    const ET9BOOL bAllowFreeDouble = pCMP->bAllowFreeDouble;

    const ET9UINT nKeyCount = wLength;
    const ET9UINT nSymbCount = pWord->Base.wWordLen;

    const ET9UINT nQualityCount = pCMP->nQualityCount;

    ET9_UNUSED(wIndex);     /* used in "init" */

    ET9Assert(ASPC.bSpcState == ET9_SPC_STATE_FLEX_INIT_OK);

    _ET9AWValidateFlexArea(pLingCmnInfo);

    WLOG8B({

        fprintf(pLogFile8, "__LDB_CalcEditDistanceFlex, spc cmpl %d, free punct %d, free double %d, word ", bAllowSpcCmpl, bAllowFreePunct, bAllowFreeDouble);

        {
            ET9U16 wIndex;

            for (wIndex = 0; wIndex < pWord->Base.wWordLen; ++wIndex) {
                fprintf(pLogFile8, "%c", pWord->Base.sWord[wIndex]);
            }
        }

        fprintf(pLogFile8, "\n");
    })

    /* screening */

    __STAT_AWLdb_Flex_EDS_Call;

    if (nSymbCount < wCurrMinSourceLength) {
        WLOG8B(fprintf(pLogFile8, "  no match (too short, %u < %u - %u)\n", nSymbCount, nQualityCount, bMaxEditDist);)
        ET9Assert(0);
        _ET9AWModifiedFlexArea(pLingCmnInfo);
        return ET9STATUS_NO_MATCH;
    }

    /* screen calculation */

    {
        ET9UINT nR;
        ET9UINT nC;

        ET9BOOL bInStem = 1;

        /* matrix */

        for (nC = 1; nC <= nSymbCount; ++nC) {

            const ET9U16 wCharCode = __LdbGetCharCodeAtPos(nC - 1);

            if (bInStem) {
                if (pCMP->pwPrevWordSC[nC] == wCharCode) {
                    continue;
                }
                else {
                    bInStem = 0;
                }
            }

            pCMP->pwPrevWordSC[nC] = wCharCode;

            {
                const ET9BOOL bIsFreeC = (ET9BOOL)(bAllowFreePunct && !pCMP->psLockSymb[nC - 1] && __IsCodeFree(wCharCode));

                ET9U8 bBestColEditDist;

                pCMP->ppbEditDist[0][nC - 0] = pCMP->ppbEditDist[0][nC - 1] + (bIsFreeC ? 0 : 1);

                bBestColEditDist = pCMP->ppbEditDist[0][nC];

                for (nR = 1; nR <= nKeyCount; ++nR) {

                    WLOG8B(fprintf(pLogFile8, "matrix %2d %2d\n", nR, nC);)

                    /* initial */

                    pCMP->ppbEditDist[nR][nC] = _ET9_FLEX_TUBE_MAX_EDIT_DIST;

                    /* pointless? */

                    if (pCMP->ppbEditDist[nR - 1][nC - 1] > bMaxEditDist &&
                        pCMP->ppbEditDist[nR - 1][nC - 0] > bMaxEditDist &&
                        pCMP->ppbEditDist[nR - 0][nC - 1] > bMaxEditDist) {

                        WLOG8B(fprintf(pLogFile8, "  pointless (edit) [sc]\n");)

                        pCMP->pbSubstFreqSC[nR][nC] = 0;

                        continue;
                    }

                    {
                        const ET9U8 bFreeR = !pbLockInfo[nR];
                        const ET9U8 bQualityR = pbIsQualityKey[nR];

                        const ET9U8 bMatchFreq = (ET9U8)__GetCodeFreq((nR - 1), wCharCode);

                        /* */

                        pCMP->pbSubstFreqSC[nR][nC] = bMatchFreq;

                        /* subst */

                        if (bMatchFreq) {

                            WLOG8B(fprintf(pLogFile8, "  subst-1, bMatchFreq %d (%u, max %u) [sc]\n", bMatchFreq, pCMP->ppbEditDist[nR - 1][nC - 1], bMaxEditDist);)

                            pCMP->ppbEditDist[nR][nC] = pCMP->ppbEditDist[nR - 1][nC - 1];
                        }
                        else if (pCMP->ppbEditDist[nR - 1][nC - 1] < bMaxEditDist && bFreeR) {

                            WLOG8B(fprintf(pLogFile8, "  subst-2, ppbEditDist %d [sc]\n", pCMP->ppbEditDist[nR - 1][nC - 1]);)

                            pCMP->ppbEditDist[nR][nC] = pCMP->ppbEditDist[nR - 1][nC - 1] + 1;
                        }

                        /* del */

                        if (!bQualityR) {

                            const ET9U8 bEditCost = pCMP->ppbEditDist[nR - 1][nC - 0];

                            if (bEditCost < pCMP->ppbEditDist[nR][nC]) {

                                WLOG8B(fprintf(pLogFile8, "  del-1 (%u, max %u) [sc]\n", bEditCost, bMaxEditDist);)

                                pCMP->pbSubstFreqSC[nR][nC] = 0;

                                pCMP->ppbEditDist[nR][nC] = bEditCost;
                            }
                        }
                        else if (pCMP->ppbEditDist[nR - 1][nC - 0] < bMaxEditDist && bFreeR) {

                            const ET9U8 bEditCost = pCMP->ppbEditDist[nR - 1][nC - 0] + 1;

                            if (bEditCost < pCMP->ppbEditDist[nR][nC]) {

                                WLOG8B(fprintf(pLogFile8, "  del-2 [sc]\n");)

                                pCMP->pbSubstFreqSC[nR][nC] = 0;

                                pCMP->ppbEditDist[nR][nC] = bEditCost;
                            }
                        }

                        /* ins */

                        if (bAllowFreeDouble && bMatchFreq && bMatchFreq == pCMP->pbSubstFreqSC[nR][nC - 1] || bIsFreeC) {

                            const ET9U8 bEditCost = pCMP->ppbEditDist[nR - 0][nC - 1];

                            if (bEditCost < pCMP->ppbEditDist[nR][nC]) {

                                WLOG8B(fprintf(pLogFile8, "  ins-1 [sc]\n");)

                                pCMP->pbSubstFreqSC[nR][nC] = pCMP->pbSubstFreqSC[nR][nC - 1];

                                pCMP->ppbEditDist[nR][nC] = bEditCost;
                            }
                        }
                        else if (nR == nKeyCount && nC > nQualityCount && (!pCMP->ppbEditDist[nR - 0][nC - 1] || bAllowSpcCmpl)) {

                            const ET9U8 bEditCost = pCMP->ppbEditDist[nR - 0][nC - 1];

                            if (bEditCost < pCMP->ppbEditDist[nR][nC]) {

                                WLOG8B(fprintf(pLogFile8, "  ins-2, nC %d, nQualityCount %d [sc]\n", nC, nQualityCount);)

                                pCMP->pbSubstFreqSC[nR][nC] = 0;

                                pCMP->ppbEditDist[nR][nC] = bEditCost;
                            }
                        }
                        else if (pCMP->ppbEditDist[nR - 0][nC - 1] < bMaxEditDist) {

                            const ET9U8 bEditCost = pCMP->ppbEditDist[nR - 0][nC - 1] + 1;

                            if (bEditCost < pCMP->ppbEditDist[nR][nC]) {

                                WLOG8B(fprintf(pLogFile8, "  ins-3 (%u, max %u) [sc]\n", bEditCost, bMaxEditDist);)

                                pCMP->pbSubstFreqSC[nR][nC] = bMatchFreq;

                                pCMP->ppbEditDist[nR][nC] = bEditCost;
                            }
                        }

                        /* best */

                        if (bBestColEditDist > pCMP->ppbEditDist[nR][nC]) {
                            bBestColEditDist = pCMP->ppbEditDist[nR][nC];
                        }
                    }
                }

                /* pre-empt? */

                if (bBestColEditDist > bMaxEditDist) {
                    WLOG8B(fprintf(pLogFile8, "  no match (best edit) [sc], %d > %d\n", bBestColEditDist, bMaxEditDist);)
                    pCMP->pwPrevWordSC[nC] = ALDB_CHAR_CODE_NONE;
                    __STAT_AWLdb_Flex_EDS_Done;
                    _ET9AWModifiedFlexArea(pLingCmnInfo);
                    return ET9STATUS_NO_MATCH;
                }
            }
        }

        pCMP->pwPrevWordSC[nC] = ALDB_CHAR_CODE_NONE;

        WLOG8B({

            fprintf(pLogFile8, "\n--- edist distance ---\n\n");

            for (nR = 0; nR <= nKeyCount; ++nR) {

                fprintf(pLogFile8, "%2d %2x : ", nR, pbIsQualityKey[nR]);

                for (nC = 0; nC <= nSymbCount; ++nC) {
                    fprintf(pLogFile8, "%2x ", pCMP->ppbEditDist[nR][nC]);
                }

                fprintf(pLogFile8, "\n");
            }

            fprintf(pLogFile8, "\n");
        })

        /* really a match? */

        if (pCMP->ppbEditDist[nKeyCount][nSymbCount] > bMaxEditDist) {
            WLOG8B(fprintf(pLogFile8, "  no match (final edit), %d > %d (%d)\n", pCMP->ppbEditDist[nKeyCount][nSymbCount], bMaxEditDist, pCMP->ppbStemDist[nKeyCount][nSymbCount]);)
            __STAT_AWLdb_Flex_EDS_Done;
            _ET9AWModifiedFlexArea(pLingCmnInfo);
            return ET9STATUS_NO_MATCH;
        }

        __STAT_AWLdb_Flex_EDS_Done;
    }

    /* full calculation */

    __STAT_AWLdb_Flex_EDC_Call;

    {
        ET9U8 pbSubstFreq[ALDB_COMPARE_MAX_POS + 1];
        ET9U8 pbComplLen[ET9MAXWORDSIZE + 1];

        ET9UINT nR;
        ET9UINT nC;

        /* init */

        pbComplLen[0] = 0;

        /* init rows */

        for (nR = 1; nR <= nKeyCount; ++nR) {
            pbSubstFreq[nR] = 0;
        }

        /* matrix */

        for (nC = 1; nC <= nSymbCount; ++nC) {

            const ET9BOOL bIsFreeC = (ET9BOOL)(bAllowFreePunct && !pCMP->psLockSymb[nC - 1] && __IsCodeFree(__LdbGetCharCodeAtPos(nC - 1)));

            pbComplLen[nC] = 0;

            pCMP->ppbStemDist[0][nC - 0] = pCMP->ppbEditDist[0][nC - 0];
            pCMP->ppbFreeDist[0][nC - 0] = (ET9U8)(nC - pCMP->ppbEditDist[0][nC - 0]);

            for (nR = 1; nR <= nKeyCount; ++nR) {

                WLOG8B(fprintf(pLogFile8, "matrix %2d %2d\n", nR, nC);)

                /* pointless? */

                if (pCMP->ppbEditDist[nR][nC] > bMaxEditDist) {

                    WLOG8B(fprintf(pLogFile8, "  pointless (edit %u > %u) [full]\n", pCMP->ppbEditDist[nR][nC], bMaxEditDist);)

                    pCMP->ppbFreeDist[nR][nC] = _ET9_FLEX_TUBE_MAX_FREE_DIST;
                    pCMP->ppbStemDist[nR][nC] = _ET9_FLEX_TUBE_MAX_STEM_DIST;
                    pCMP->ppxStemFreq[nR][nC] = 1;

                    pbSubstFreq[nR] = 0;

                    continue;
                }

                {
                    const ET9U8 bFreeR = !pbLockInfo[nR];
                    const ET9U8 bQualityR = pbIsQualityKey[nR];
                    const ET9U8 bSubstFreqPrev = pbSubstFreq[nR];

                    const ET9U8 bMatchFreq = (ET9U8)__GetCodeFreq((nR - 1), __LdbGetCharCodeAtPos(nC - 1));

                    const ET9U8 bDefaultFreq = bQualityR ? 10 : 1;

                    ET9U8 bEditDist;
                    ET9U8 bFreeDist;
                    ET9U8 bStemDist;
                    ET9FREQ xStemFreq;

                    /* */

                    pbSubstFreq[nR] = bMatchFreq;

                    /* subst */

                    if (bMatchFreq) {

                        const ET9BOOL bMatchExact = (ET9BOOL)__IsCodeExact((nR - 1), __LdbGetCharCodeAtPos(nC - 1));

#ifdef ET9_USE_FLOAT_FREQS
                        const ET9FREQ xFreq = (ET9FREQ)(!pbIsFreqPos[nR] ? bDefaultFreq : bMatchFreq);
#else
                        const ET9FREQ xFreq = (!pbIsFreqPos[nR] || bMatchFreq <= 17) ? 1 : ((bMatchFreq * 15) / 255);
#endif

                        WLOG8B(fprintf(pLogFile8, "  subst-1, bMatchFreq %d (%u, max %u) [full]\n", bMatchFreq, pCMP->ppbEditDist[nR - 1][nC - 1], bMaxEditDist);)

                        bFreeDist = pCMP->ppbFreeDist[nR - 1][nC - 1] + (bQualityR ? 0 : 1);
                        bEditDist = pCMP->ppbEditDist[nR - 1][nC - 1];
                        bStemDist = pCMP->ppbStemDist[nR - 1][nC - 1] + (bMatchExact ? 0 : 1);
                        xStemFreq = pCMP->ppxStemFreq[nR - 1][nC - 1] * xFreq;
                    }
                    else if (pCMP->ppbEditDist[nR - 1][nC - 1] < bMaxEditDist && bFreeR) {

                        WLOG8B(fprintf(pLogFile8, "  subst-2, ppbEditDist %d [full]\n", pCMP->ppbEditDist[nR - 1][nC - 1]);)

                        bFreeDist = pCMP->ppbFreeDist[nR - 1][nC - 1] + (bQualityR ? 0 : 1);
                        bEditDist = pCMP->ppbEditDist[nR - 1][nC - 1] + 1;
                        bStemDist = pCMP->ppbStemDist[nR - 1][nC - 1] + (bQualityR ? 1 : 0);
                        xStemFreq = pCMP->ppxStemFreq[nR - 1][nC - 1] * bDefaultFreq;
                    }
                    else {
                        bFreeDist = _ET9_FLEX_TUBE_MAX_FREE_DIST;
                        bEditDist = _ET9_FLEX_TUBE_MAX_EDIT_DIST;
                        bStemDist = _ET9_FLEX_TUBE_MAX_STEM_DIST;
                        xStemFreq = 1;
                    }

                    /* del */

                    if (!bQualityR) {

                        const ET9U8 bFreeCost = pCMP->ppbFreeDist[nR - 1][nC - 0];
                        const ET9U8 bEditCost = pCMP->ppbEditDist[nR - 1][nC - 0];
                        const ET9U8 bStemCost = pCMP->ppbStemDist[nR - 1][nC - 0];

                        if (_ET9_Flex_IsBetterOp(-1, 0)) {

                            WLOG8B(fprintf(pLogFile8, "  del-1 [full]\n");)

                            pbSubstFreq[nR] = 0;

                            bFreeDist = bFreeCost;
                            bEditDist = bEditCost;
                            bStemDist = bStemCost;
                            xStemFreq = pCMP->ppxStemFreq[nR - 1][nC - 0];
                        }
                    }
                    else if (pCMP->ppbEditDist[nR - 1][nC - 0] < bMaxEditDist && bFreeR) {

                        const ET9U8 bFreeCost = pCMP->ppbFreeDist[nR - 1][nC - 0];
                        const ET9U8 bEditCost = pCMP->ppbEditDist[nR - 1][nC - 0] + 1;
                        const ET9U8 bStemCost = pCMP->ppbStemDist[nR - 1][nC - 0] + 1;

                        if (_ET9_Flex_IsBetterOp(-1, 0)) {

                            WLOG8B(fprintf(pLogFile8, "  del-2 [full]\n");)

                            pbSubstFreq[nR] = 0;

                            bFreeDist = bFreeCost;
                            bEditDist = bEditCost;
                            bStemDist = bStemCost;
                            xStemFreq = pCMP->ppxStemFreq[nR - 1][nC - 0] * bDefaultFreq;
                        }
                    }

                    /* ins */

                    if (bAllowFreeDouble && bMatchFreq && bMatchFreq == bSubstFreqPrev || bIsFreeC) {

                        const ET9U8 bFreeCost = pCMP->ppbFreeDist[nR - 0][nC - 1] + 1;
                        const ET9U8 bEditCost = pCMP->ppbEditDist[nR - 0][nC - 1];
                        const ET9U8 bStemCost = pCMP->ppbStemDist[nR - 0][nC - 1];

                        if (_ET9_Flex_IsBetterOp(0, -1)) {

                            WLOG8B(fprintf(pLogFile8, "  ins-2 (free) [full]\n");)

                            pbSubstFreq[nR] = bSubstFreqPrev;

                            if (nR == nKeyCount) {
                                pbComplLen[nC] = pbComplLen[nC - 1];
                            }

                            bEditDist = bEditCost;
                            bFreeDist = bFreeCost;
                            bStemDist = bStemCost;
                            xStemFreq = pCMP->ppxStemFreq[nR - 0][nC - 1];
                        }
                    }
                    else if (nR == nKeyCount && nC > nQualityCount && (!pCMP->ppbEditDist[nR - 0][nC - 1] || bAllowSpcCmpl)) {

                        const ET9U8 bFreeCost = pCMP->ppbFreeDist[nR - 0][nC - 1];
                        const ET9U8 bEditCost = pCMP->ppbEditDist[nR - 0][nC - 1];
                        const ET9U8 bStemCost = pCMP->ppbStemDist[nR - 0][nC - 1];

                        if (_ET9_Flex_IsBetterOp(0, -1)) {

                            WLOG8B(fprintf(pLogFile8, "  ins-1 (cmpl), nC %d, nQualityCount %d [full]\n", nC, nQualityCount);)

                            pbSubstFreq[nR] = 0;

                            pbComplLen[nC] = pbComplLen[nC - 1] + 1;

                            bFreeDist = bFreeCost;
                            bEditDist = bEditCost;
                            bStemDist = bStemCost;
                            xStemFreq = pCMP->ppxStemFreq[nR - 0][nC - 1];
                        }
                    }
                    else if (pCMP->ppbEditDist[nR - 0][nC - 1] < bMaxEditDist) {

                        const ET9U8 bFreeCost = pCMP->ppbFreeDist[nR - 0][nC - 1];
                        const ET9U8 bEditCost = pCMP->ppbEditDist[nR - 0][nC - 1] + 1;
                        const ET9U8 bStemCost = pCMP->ppbStemDist[nR - 0][nC - 1] + 1;

                        if (_ET9_Flex_IsBetterOp(0, -1)) {

                            WLOG8B(fprintf(pLogFile8, "  ins-3 (spc) [full]\n");)

                            pbSubstFreq[nR] = bMatchFreq;

                            bFreeDist = bFreeCost;
                            bEditDist = bEditCost;
                            bStemDist = bStemCost;
                            xStemFreq = pCMP->ppxStemFreq[nR - 0][nC - 1];
                        }
                    }

                    /* assure same as screening */

                    if (bEditDist != pCMP->ppbEditDist[nR][nC]) {
                        WLOG8B(fprintf(pLogFile8, "  inconsistent (%u <-> %u) [full]\n", bEditDist, pCMP->ppbEditDist[nR][nC]);)
                    }

                    ET9Assert(bEditDist == pCMP->ppbEditDist[nR][nC]);

                    /* persist */

                    pCMP->ppbFreeDist[nR][nC] = bFreeDist;
                    pCMP->ppbStemDist[nR][nC] = bStemDist;
                    pCMP->ppxStemFreq[nR][nC] = xStemFreq;
                }
            }
        }

        WLOG8B({

            fprintf(pLogFile8, "\n--- free distance ---\n\n");

            for (nR = 0; nR <= nKeyCount; ++nR) {

                fprintf(pLogFile8, "%2d %2x : ", nR, pbIsQualityKey[nR]);

                for (nC = 0; nC <= nSymbCount; ++nC) {
                    fprintf(pLogFile8, "%2x ", pCMP->ppbFreeDist[nR][nC]);
                }

                fprintf(pLogFile8, "\n");
            }

            fprintf(pLogFile8, "\n--- edist distance ---\n\n");

            for (nR = 0; nR <= nKeyCount; ++nR) {

                fprintf(pLogFile8, "%2d %2x : ", nR, pbIsQualityKey[nR]);

                for (nC = 0; nC <= nSymbCount; ++nC) {
                    fprintf(pLogFile8, "%2x ", pCMP->ppbEditDist[nR][nC]);
                }

                fprintf(pLogFile8, "\n");
            }

            fprintf(pLogFile8, "\n--- stem distance ---\n\n");

            for (nR = 0; nR <= nKeyCount; ++nR) {

                fprintf(pLogFile8, "%2d %2x : ", nR, pbIsQualityKey[nR]);

                for (nC = 0; nC <= nSymbCount; ++nC) {
                    fprintf(pLogFile8, "%2x ", pCMP->ppbStemDist[nR][nC]);
                }

                fprintf(pLogFile8, "\n");
            }

            fprintf(pLogFile8, "\n--- stem freq ---\n\n");

            for (nR = 0; nR <= nKeyCount; ++nR) {

                fprintf(pLogFile8, "%2d %2x : ", nR, pbIsQualityKey[nR]);

                for (nC = 0; nC <= nSymbCount; ++nC) {
                    fprintf(pLogFile8, "%8.0f ", pCMP->ppxStemFreq[nR][nC]);
                }

                fprintf(pLogFile8, "\n");
            }

            fprintf(pLogFile8, "\n--- completion length ---\n\n");

            {
                fprintf(pLogFile8, "      : ");

                for (nC = 0; nC <= nSymbCount; ++nC) {
                    fprintf(pLogFile8, "%2x ", pbComplLen[nC]);
                }

                fprintf(pLogFile8, "\n");
            }

            fprintf(pLogFile8, "\n");
        })

        /* assure same as screening */

        ET9Assert(pCMP->ppbEditDist[nKeyCount][nSymbCount] <= bMaxEditDist);

        {
            const ET9BOOL bAvoidStems = (pLingCmnInfo->Private.bTraceBuild || wIndex) ? 1 : 0;

            /* suppress completions before point */

            if (pbComplLen[nSymbCount] && nQualityCount < pLingCmnInfo->Private.wWordCompletionPoint) {
                if (bAvoidStems) {
                    WLOG8B(fprintf(pLogFile8, "  suppressed completion before point\n");)
                    __STAT_AWLdb_Flex_EDC_Done(NULL);
                    _ET9AWModifiedFlexArea(pLingCmnInfo);
                    return ET9STATUS_NO_MATCH;
                }
            }

            /* assign distance */

            pWord->bEditDistSpc  = pCMP->ppbEditDist[nKeyCount][nSymbCount];
            pWord->bEditDistStem = pCMP->ppbStemDist[nKeyCount][nSymbCount];
            pWord->bEditDistFree = pCMP->ppbFreeDist[nKeyCount][nSymbCount];

            /**/

            if (pbComplLen[nSymbCount] && !pWord->bEditDistSpc && (pWordSymbInfo->bNumSymbs < pLingCmnInfo->Private.wWordCompletionPoint)) {
                if (bAvoidStems) {
                    WLOG8B(fprintf(pLogFile8, "  assigning edit distance - before completion point\n");)
                    pWord->bEditDistSpc = 1;
                }
            }

#if 1
            if (pWord->Base.wWordLen > pLingCmnInfo->Private.wMaxWordLength + pWord->bEditDistFree) {
                pWord->bEditDistFree = 0;
                pWord->Base.wWordCompLen = 0;
            }
            else {
                pWord->Base.wWordCompLen = pbComplLen[nSymbCount];
            }
#else
            if (pbComplLen[nSymbCount] && (!pWord->bEditDistSpc || bAllowSpcCmpl) && pWord->Base.wWordLen <= pLingCmnInfo->Private.wMaxWordLength) {
                pWord->Base.wWordCompLen = pbComplLen[nSymbCount];
            }
            else {
                pWord->Base.wWordCompLen = 0;
            }
#endif

            if (!pWord->Base.wWordCompLen && pWord->Base.wWordLen > pWordSymbInfo->bNumSymbs) {
                if (!pWord->bEditDistSpc && pWord->Base.wWordLen <= pLingCmnInfo->Private.wMaxWordLength) {
                    WLOG8B(fprintf(pLogFile8, "  longer than input and shorter than max - special zero compl len\n");)
                    pWord->Base.wWordCompLen = 0xFFFF;
                }
                else if (pWord->Base.wWordLen == pLingCmnInfo->Private.wMaxWordLength + pWord->bEditDistFree) {
                    WLOG8B(fprintf(pLogFile8, "  longer than input and longer than max - special zero compl len\n");)
                    pWord->Base.wWordCompLen = 0xFFFF;
                }
            }
        }

        /* assign tap freq */

#ifdef ET9_USE_FLOAT_FREQS

        pWord->xTapFreq = pCMP->ppxStemFreq[nKeyCount][nSymbCount] / _ET9pow_f(10, (ET9FLOAT)__ET9Min(nQualityCount, ET9_SPC_ED_MAX_FREQ_LEN));

#else /* ET9_USE_FLOAT_FREQS */

        if (nQualityCount <= 4) {
            pWord->xTapFreq = (ET9FREQPART)pCMP->ppxStemFreq[nKeyCount][nSymbCount];
        }
        else {
            pWord->xTapFreq = (ET9FREQPART)(pCMP->ppxStemFreq[nKeyCount][nSymbCount] >> (4 * (nQualityCount - 4)));
        }

#endif /* ET9_USE_FLOAT_FREQS */

        if (!pWord->xTapFreq) {
            pWord->xTapFreq = 1;
        }

        ET9Assert(pWord->xTapFreq >= 0);

        WLOG8B(fprintf(pLogFile8, "  match, bEditDistStem %d, bEditDistSpc %d, wWordCompLen %d, \n\n",
                                  pWord->bEditDistStem,
                                  pWord->bEditDistSpc,
                                  pWord->Base.wWordCompLen);)

        __STAT_AWLdb_Flex_EDC_Done(pWord);

        _ET9AWModifiedFlexArea(pLingCmnInfo);

        return ET9STATUS_NONE;
    }
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Get all matching candidates in the LDB using the flex calculator.
 * Given an "input" this will setup all necessary info and perform a search to get all potentially matching candidates from the LDB data.
 * Rather than fetching all LDB words and checking if they match using the match function, this will perform a low level screening in order
 * to as early as possible screen non matching words.<br>
 * When spell correction is off this is a perfect screening (only finding words that actually are candidates). With spell correction on
 * this is much more complex, and it will over generate.<br>
 *
 * @param pLingInfo                 Pointer to alphabetic information structure.
 * @param wIndex                    Input index/position (non zero if partial).
 * @param wLength                   Word length (active input length).
 * @param bFreqIndicator            Designation
 * @param bSpcMode                  Spell correction mode.
 *
 * @return None
 */

static void ET9LOCALCALL __ET9AWLdbGetCandidatesFlex (ET9AWLingInfo * const         pLingInfo,
                                                      const ET9U16                  wIndex,
                                                      const ET9U16                  wLength,
                                                      const ET9_FREQ_DESIGNATION    bFreqIndicator,
                                                      const ET9U8                   bSpcMode)
{
    ET9AWPrivWordInfo sPublicWord;
    ET9AWPrivWordInfo sPrivateWord;

    ET9AWLingCmnInfo * const pLingCmnInfo = pLingInfo->pLingCmnInfo;

    const ET9U8 bLangIndex = pLingCmnInfo->wLdbNum == pLingInfo->pLingCmnInfo->wFirstLdbNum ? ET9AWFIRST_LANGUAGE : ET9AWSECOND_LANGUAGE;

    const ET9U8 bWordSrc =  ADDON_FROM_FREQ_IND(bFreqIndicator, ET9WORDSRC_LDB);

    /* if all LDB words being downshifted, or word being compounded, downshift it */

    const ET9BOOL bDownShift = (ET9BOOL)((ET9DOWNSHIFTALLLDB(pLingCmnInfo) || wIndex) && ALDB.header.bUpperCount);

    ET9Assert(pLingInfo);

    WLOG8(fprintf(pLogFile8, "\n__ET9AWLdbGetCandidatesFlex\n\n");)

    __STAT_AWLdb_Call;
    __STAT_AWLdb_Flex_Call;

    if (!wLength || wLength > ET9MAXWORDSIZE) {
        return;
    }

    if (_ET9AWCalcEditDistanceInit(pLingInfo, wIndex, wLength, bSpcMode)) {
        return;
    }

    WLOG3(fprintf(pLogFile3, "__ET9AWLdbGetCandidatesFlex, start, wIndex = %2d, wLength = %2d, bSpcMode = %d\n", wIndex, wLength, bSpcMode);)

    _InitPrivWordInfo(&sPrivateWord);

    sPrivateWord.Base.bLangIndex = bLangIndex;

    __ET9CompareStart(pLingCmnInfo, wIndex, wLength, 1);

    __ET9SearchStart(pLingInfo, sPrivateWord.Base.sWord, &sPrivateWord.Base.wWordLen, ET9MAXWORDSIZE, 1);

    for (; !__IsExhausted(); __ET9SearchGetNextFlex(pLingInfo)) {

        __STAT_INC_TotLdbCount;

        WLOG3B(fprintf(pLogFile3, "__ET9AWLdbGetCandidatesFlex, word index = %6d, word len = %2d, wLength = %2d\n", __GetWordIndex(), sPrivateWord.Base.wWordLen, wLength);)

        sPrivateWord.bWordSrc = bWordSrc;

        if (__LDB_CalcEditDistanceFlex(pLingInfo, &sPrivateWord, wIndex, wLength)) {
            continue;
        }

        ET9Assert(sPrivateWord.xTapFreq >= 0);

        /* candidate found */

        sPublicWord = sPrivateWord;

        /* downshift word if option selected */

        if (bDownShift) {

            ET9U16 wCount = sPublicWord.Base.wWordLen;
            ET9SYMB *pSymb = sPublicWord.Base.sWord;

            for (; wCount; --wCount, ++pSymb) {
                *pSymb = _ET9SymToLower(*pSymb, pLingCmnInfo->wLdbNum);
            }
        }

        /* prepare and add to list */

        __STAT_AWLdb_Flex_SLA_Call;

        _ET9AWSelLstWordPreAdd(pLingInfo, &sPublicWord, wIndex, (ET9U8)wLength);

        {
            const ET9U32 dwCurrWordIndex = __GetWordIndex() + 1;

            sPublicWord.xWordFreq = (ET9FREQPART)((ET9FREQPART)0xFFFF / dwCurrWordIndex);   /* zero freq becomes one in "add" */
            sPublicWord.wEWordFreq = 0;
            sPublicWord.wTWordFreq = 0;
            sPublicWord.dwWordIndex = dwCurrWordIndex;
        }

        ET9Assert(sPublicWord.xTapFreq >= 0);
        ET9Assert(sPublicWord.xTotFreq >= 0);

        sPublicWord.Base.wWordCompLen = sPrivateWord.Base.wWordCompLen;

        sPublicWord.bIsTop5 = (__GetWordIndex() < ALDB.header.dwTopCount) ? 1 : 0;

        _ET9AWSelLstAdd(pLingInfo, &sPublicWord, wLength, bFreqIndicator);

        __STAT_AWLdb_Flex_SLA_Done;
    }

    _ET9AWCalcEditDistanceDone(pLingInfo);

    __STAT_AWLdb_Flex_Done;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * This function initializes the LDB.
 * Different checks will be performed to validate it.<br>
 * If it passes validation the LDB specifics will be setup and it's ready for searching.
 *
 * @param pLingInfo                 Pointer to alphabetic information structure.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

static ET9STATUS ET9LOCALCALL __ET9AWLdbInit(ET9AWLingInfo * const pLingInfo)
{
    ET9STATUS       wStatus;
    ET9U8           byData;
    ET9U8           byPrimeLanguageID1;
    ET9U8           byPrimeLanguageID2;
    ET9U8           bySecondaryLanguageID1;
    ET9U8           bySecondaryLanguageID2;
    ET9U16          wDatabaseType;

    ET9AWLingCmnInfo * const pLingCmnInfo = pLingInfo->pLingCmnInfo;

    const ET9U16    wLdbNum = pLingCmnInfo->wLdbNum;

    ET9Assert(pLingInfo);

    /* Skip checking for pLingInfo since this is not an API. */

    /* note: not checking wLDBInitOK here, because this is handled by calling function.*/

    /* Layout version check */

    wStatus = __ET9ReadLDBData(pLingInfo, ET9LDBOFFSET_LDBLAYOUTVER, 1, &byData);

    if (wStatus) {
        return wStatus;
    }
    if (byData != 3) {
        return ET9STATUS_LDB_VERSION_ERROR;
    }

    /* Check the database type */

    wStatus = __ET9ReadLDBData(pLingInfo, ET9LDBOFFSET_DATABASETYPE, 1, &byData);

    if (wStatus) {
        return wStatus;
    }

    /* Database type should not be greater than ET9MAXDBTYPE. */

    if (byData > ET9MAXDBTYPE) {
        return ET9STATUS_INVALID_DB_TYPE;
    }

    /* Convert the number to bit mask. */

    wDatabaseType = (ET9U16)(1 << byData);

    if (!(wDatabaseType & (ET9DB_LDB_LATIN_MASK))) {
        return ET9STATUS_INVALID_DB_TYPE;
    }

    /* Skip flags check */

    /* Compatibility Check */

    wStatus = __ET9AWLdbCheckCompat(pLingInfo);

    if (wStatus != ET9STATUS_NONE) {
        return wStatus;
    }

    /* OEM ID Check */

    wStatus = __ET9AWLdbCheckOEMID(pLingInfo);

    if (wStatus != ET9STATUS_NONE) {
        return wStatus;
    }

    /* check primary language id */

    byPrimeLanguageID1 = (ET9U8)(wLdbNum & ET9PLIDMASK);

    wStatus = __ET9ReadLDBData(pLingInfo, ET9LDBOFFSET_PRIMARYLANGID, 1, &byPrimeLanguageID2);

    if (wStatus) {
        return wStatus;
    }

    if (byPrimeLanguageID1 != byPrimeLanguageID2) {
        return ET9STATUS_LDB_ID_ERROR;                /* this is NOT the requested LDB */
    }

    /* check secondary language id */

    bySecondaryLanguageID1 = (ET9U8)((wLdbNum & ET9SLIDMASK) >> 8);

    wStatus = __ET9ReadLDBData(pLingInfo, ET9LDBOFFSET_SECONDARYLANGID, 1, &bySecondaryLanguageID2);

    if (wStatus) {
        return wStatus;
    }

    if (bySecondaryLanguageID1 != bySecondaryLanguageID2) {
        return ET9STATUS_LDB_ID_ERROR;                /* this is NOT the requested LDB */
    }

    /* Specific LDB internals */

    wStatus = __ET9SearchInit(pLingInfo, ET9LDBOFFSET_BODY);

    if (wStatus) {
        return wStatus;
    }

    pLingCmnInfo->Private.bStateLDBEnabled = 1;

    /* investigate word count */

    __InvestigateWordCount(pLingInfo);

    /* done */

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Retrieve index based on word.
 *
 * @param pLingInfo             Pointer to alphabetic information structure.
 * @param wLdbNum               LDB number.
 * @param psWord                Pointer to shortcut entry.
 * @param wWordLen              Length of shortcut entry.
 * @param pdwIndex              Pointer to index (0 based).
 * @param pbExact               Pointer to put indicator of 'exact match found'.
 * @param pbLowercase           Pointer to return 'lowercase version of word found'.
 *
 * @return ET9STATUS_NONE on success, otherwise return error code.
 */

ET9STATUS ET9FARCALL _ET9AWLdbFindEntry(ET9AWLingInfo       * const pLingInfo,
                                        const ET9U16                wLdbNum,
                                        ET9SYMB             * const psWord,
                                        const ET9U16                wWordLen,
                                        ET9U32              * const pdwIndex,
                                        ET9U8               * const pbExact,
                                        ET9U8               * const pbLowercase)
{
    ET9AWLingCmnInfo * const pLingCmnInfo = pLingInfo->pLingCmnInfo;
    ET9WordSymbInfo * const pWordSymbInfo   = pLingCmnInfo->Base.pWordSymbInfo;

    ET9STATUS         wStatus;
    ET9AWPrivWordInfo LocalWord;
    ET9U16 wSavedLDBNum = 0;
    ET9AWPrivWordInfo Word;
    ET9U8 i = 0;

    ET9Assert(pLingInfo);
    ET9Assert(psWord);

    WLOG3(fprintf(pLogFile3, "_ET9AWLdbFindEntry\n");)

    *pbExact = 0;
    *pbLowercase = 0;

    if (pLingInfo->Private.wLDBInitOK != ET9GOODSETUP) {
        WLOG3(fprintf(pLogFile3, "_ET9AWLdbFindEntry, no init\n");)
        return ET9STATUS_NO_INIT;
    }

    if (((wLdbNum & ET9PLIDMASK) == ET9PLIDNone) || !ET9LDBENABLED(pLingCmnInfo)) {
        WLOG3(fprintf(pLogFile3, "_ET9AWLdbFindEntry, no active LDB\n");)
        return ET9STATUS_NO_MATCHING_WORDS;
    }

    if (wWordLen > ET9MAXLDBWORDSIZE) {
        WLOG3(fprintf(pLogFile3, "_ET9AWLdbFindEntry, too long word\n");)
        return ET9STATUS_NO_MATCHING_WORDS;
    }

    if (!wWordLen) {
         WLOG3(fprintf(pLogFile3, "_ET9AWLdbFindEntry, zero length word\n");)
        return ET9STATUS_NO_MATCHING_WORDS;
    }

     /* assure cache */

    if (wLdbNum != pLingCmnInfo->wLdbNum) {
        wSavedLDBNum = pLingCmnInfo->wLdbNum;
        _ET9AWLdbSetActiveLanguage(pLingInfo, wLdbNum);
    }

    Word.Base.wWordLen = wWordLen;
    Word.Base.wWordCompLen = 0;

    for (i = 0; i < wWordLen; i++) {
        Word.Base.sWord[i] = psWord[i];
    }

    if ((wStatus = __ET9CompareStartWord(pLingInfo, &Word)) != ET9STATUS_NONE) {
        WLOG3(fprintf(pLogFile3, "_ET9AWLdbFindEntry, bad word\n");)
        if (wSavedLDBNum) {
            _ET9AWLdbSetActiveLanguage(pLingInfo, wSavedLDBNum);
        }
        return wStatus;
    }

    __ET9SearchStart(pLingInfo, LocalWord.Base.sWord, &LocalWord.Base.wWordLen, ET9MAXWORDSIZE, 0);

    wStatus = ET9STATUS_NO_MATCHING_WORDS;

    for (; !__IsExhausted(); __ET9SearchGetNextStd(pLingInfo)) {

        ET9U8 bExactMatch = 1;
        ET9U8 bAllLowerCase = 1;

        {
            ET9U16 wCount = wWordLen;
            ET9SYMB *pSymbWord = psWord;
            ET9SYMB *pSymbLocal = LocalWord.Base.sWord;

            ET9Assert(ALDB.compare.wLength <  ALDB.header.bPosCount || LocalWord.Base.wWordLen == wWordLen);
            ET9Assert(ALDB.compare.wLength == ALDB.header.bPosCount || LocalWord.Base.wWordLen == wWordLen + 1);

            for (; wCount; --wCount, ++pSymbWord, ++pSymbLocal) {

                if (*pSymbWord != *pSymbLocal) {
                    bExactMatch = 0;
                }
                if (_ET9SymIsUpper(*pSymbLocal, pWordSymbInfo->Private.wLocale)) {
                    bAllLowerCase = 0;
                }
            }
        }

        if (bAllLowerCase) {
            WLOG3(fprintf(pLogFile3, "_ET9AWLdbFindEntry, ET9STATUS_WORD_EXISTS in lowercase form\n");)
            *pbLowercase = 1;
            *pdwIndex = __GetWordIndex();
        }

        if (bExactMatch) {
            WLOG3(fprintf(pLogFile3, "_ET9AWLdbFindEntry, ET9STATUS_WORD_EXISTS exactly as entered\n");)
            *pbExact = 1;
            wStatus = ET9STATUS_WORD_EXISTS;
            *pdwIndex = __GetWordIndex();
            break;
        }

        wStatus = ET9STATUS_WORD_EXISTS;
    }

    WLOG3(
        if (wStatus == ET9STATUS_NO_MATCHING_WORDS) {
            fprintf(pLogFile3, "_ET9AWLdbFindEntry, ET9STATUS_NO_MATCHING_WORDS\n");
        })

    if (wSavedLDBNum) {
        _ET9AWLdbSetActiveLanguage(pLingInfo, wSavedLDBNum);
    }

    return wStatus;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Find a word in the LDB.
 * Checks if a word exists in the LDB or not. If found, load indications
 * showing that 1) word in LDB matches exactly y/n, and 2) a lowercase
 * version of the word exists in the LDB (set even if word passed is lowercase)
 *
 * @param pLingInfo             Pointer to alphabetic information structure.
 * @param wLdbNum               Language id.
 * @param pWord                 Pointer to lookup word.
 * @param pbExact               Pointer to put indicator of 'exact match found'.
 * @param pbLowercase           Pointer to return 'lowercase version of word found'.
 * @param bRetrieve             Indication to update passed word with LDB version.
 *
 * @return ET9STATUS_NO_MATCHING_WORDS or ET9STATUS_WORD_EXISTS.
 */

ET9STATUS ET9FARCALL _ET9AWLdbFind(ET9AWLingInfo        * const pLingInfo,
                                   const ET9U16                 wLdbNum,
                                   ET9AWPrivWordInfo    * const pWord,
                                   ET9U8                * const pbExact,
                                   ET9U8                * const pbLowercase,
                                   const ET9BOOL                bRetrieve)
{
    ET9AWLingCmnInfo * const pLingCmnInfo = pLingInfo->pLingCmnInfo;
    ET9WordSymbInfo * const pWordSymbInfo   = pLingCmnInfo->Base.pWordSymbInfo;

    ET9STATUS         wStatus;
    ET9AWPrivWordInfo LocalWord;
    ET9U16 wSavedLDBNum = 0;
    ET9Assert(pLingInfo);
    ET9Assert(pWord);
    ET9Assert(pbExact);
    ET9Assert(pbLowercase);

    WLOG3(fprintf(pLogFile3, "_ET9AWLdbFind\n");)

    WLOG3Word(pLogFile3, "_ET9AWLdbFind word", pWord);

    *pbExact = 0;
    *pbLowercase = 0;

    if (pLingInfo->Private.wLDBInitOK != ET9GOODSETUP) {
        WLOG3(fprintf(pLogFile3, "_ET9AWLdbFind, no init\n");)
        return ET9STATUS_NO_INIT;
    }

    if (((wLdbNum & ET9PLIDMASK) == ET9PLIDNone) || !ET9LDBENABLED(pLingCmnInfo)) {
        WLOG3(fprintf(pLogFile3, "_ET9AWLdbFind, no active LDB\n");)
        return ET9STATUS_NO_MATCHING_WORDS;
    }

    if (pWord->Base.wWordLen > ET9MAXLDBWORDSIZE) {
        WLOG3(fprintf(pLogFile3, "_ET9AWLdbFind, too long word\n");)
        return ET9STATUS_NO_MATCHING_WORDS;
    }

     /* assure cache */

    if (wLdbNum != pLingCmnInfo->wLdbNum) {

        wSavedLDBNum = pLingCmnInfo->wLdbNum;

        wStatus = _ET9AWLdbSetActiveLanguage(pLingInfo, wLdbNum);

        if (wStatus) {
            return wStatus;
        }
    }

    wStatus = __ET9CompareStartWord(pLingInfo, pWord);

    if (wStatus) {
        WLOG3(fprintf(pLogFile3, "_ET9AWLdbFind, bad word\n");)
        if (wSavedLDBNum) {
            _ET9AWLdbSetActiveLanguage(pLingInfo, wSavedLDBNum);
        }
        return wStatus;
    }

    __ET9SearchStart(pLingInfo, LocalWord.Base.sWord, &LocalWord.Base.wWordLen, ET9MAXWORDSIZE, 0);

    wStatus = ET9STATUS_NO_MATCHING_WORDS;

    for (; !__IsExhausted(); __ET9SearchGetNextStd(pLingInfo)) {

        ET9U8 bExactMatch = 1;
        ET9U8 bAllLowerCase = 1;

        {
            ET9U16 wCount = pWord->Base.wWordLen;
            ET9SYMB *pSymbWord = pWord->Base.sWord;
            ET9SYMB *pSymbLocal = LocalWord.Base.sWord;

            ET9Assert(ALDB.compare.wLength <  ALDB.header.bPosCount || LocalWord.Base.wWordLen == pWord->Base.wWordLen);
            ET9Assert(ALDB.compare.wLength == ALDB.header.bPosCount || LocalWord.Base.wWordLen == pWord->Base.wWordLen + 1);

            for (; wCount; --wCount, ++pSymbWord, ++pSymbLocal) {

                if (*pSymbWord != *pSymbLocal) {
                    bExactMatch = 0;
                }
                if (_ET9SymIsUpper(*pSymbLocal, pWordSymbInfo->Private.wLocale)) {
                    bAllLowerCase = 0;
                }
                if (bRetrieve) {
                    *pSymbWord = *pSymbLocal;
                }
            }
        }

        if (bAllLowerCase) {
            WLOG3(fprintf(pLogFile3, "_ET9AWLdbFind, ET9STATUS_WORD_EXISTS in lowercase form\n");)
            *pbLowercase = 1;
        }

        if (bExactMatch) {
            WLOG3(fprintf(pLogFile3, "_ET9AWLdbFind, ET9STATUS_WORD_EXISTS exactly as entered\n");)
            *pbExact = 1;
        }

        wStatus = ET9STATUS_WORD_EXISTS;
    }

    WLOG3(
        if (wStatus == ET9STATUS_NO_MATCHING_WORDS) {
            fprintf(pLogFile3, "_ET9AWLdbFind, ET9STATUS_NO_MATCHING_WORDS\n");
        }
        else if (!*pbLowercase && !*pbExact) {
            fprintf(pLogFile3, "_ET9AWLdbFind, ET9STATUS_WORD_EXISTS (but in new non-lowercase format)\n");
        })

    if (wSavedLDBNum) {
        _ET9AWLdbSetActiveLanguage(pLingInfo, wSavedLDBNum);
    }

    return wStatus;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Get all candidates in the LDB that matches the input.
 * If the LDB isn't properly setup it will exit with zero candidates found.
 *
 * @param pLingInfo                 Pointer to alphabetic information structure.
 * @param wLdbNum                   Language id.
 * @param wIndex                    Input index/position (non zero if partial).
 * @param wLength                   Word length (active input length).
 * @param pbLdbEntries              (out) number of candidates found, truncated if large.
 * @param bFreqIndicator            Designation
 * @param bSpcMode                  Spell correction mode.
 *
 * @return None
 */

void ET9FARCALL _ET9AWLdbWordsSearch(ET9AWLingInfo              * const pLingInfo,
                                     const ET9U16                       wLdbNum,
                                     const ET9U16                       wIndex,
                                     const ET9U16                       wLength,
                                     ET9U8                      * const pbLdbEntries,
                                     const ET9_FREQ_DESIGNATION         bFreqIndicator,
                                     const ET9U8                        bSpcMode)
{
    ET9AWLingCmnInfo * const pLingCmnInfo = pLingInfo->pLingCmnInfo;
    ET9U16 wStartValue = pLingCmnInfo->Private.wTotalWordInserts;
    ET9U16 wSavedLDB = 0;

    *pbLdbEntries = 0;

    /* Major problems if any passed pointer params are NULL. */

    ET9Assert(pLingInfo && pbLdbEntries);

    /* length within limits */

    if (wIndex + wLength > ET9MAXWORDSIZE) {
        ET9Assert(0);
        return;
    }

    /* if length is zero or ldb not initialized, no words from ldb.
       Return with no error, because only propagating low level read errors */

    if (!wLdbNum ||
        !wLength ||
        pLingInfo->Private.wLDBInitOK != ET9GOODSETUP ||
        !ET9LDBENABLED(pLingCmnInfo)) {

        return;
    }

     /* assure cache */

    if (wLdbNum != pLingCmnInfo->wLdbNum) {

        wSavedLDB = pLingCmnInfo->wLdbNum;

        if (_ET9AWLdbSetActiveLanguage(pLingInfo, wLdbNum)) {
            return;
        }
    }

    /* get candidates */

    if (ASPC.bSpcFeatures) {
        __ET9AWLdbGetCandidatesFlex(pLingInfo, wIndex, wLength, bFreqIndicator, bSpcMode);
    }
    else {
        __ET9AWLdbGetCandidates(pLingInfo, wIndex, wLength, bFreqIndicator, bSpcMode);
    }

    /* track shift significance */

    if (pLingCmnInfo->Private.ALdb.header.bLowerCount && pLingCmnInfo->Private.ALdb.header.bUpperCount) {
        pLingCmnInfo->Private.bCurrBuildHasShiftSignificance = 1;
    }

    /* restore */

    if (wSavedLDB) {
        _ET9AWLdbSetActiveLanguage(pLingInfo, wSavedLDB);
    }

    /* return the number of entries added to the selection list (proper value in most cases, != 0 is the most important part) */

    if (pLingCmnInfo->Private.wTotalWordInserts - wStartValue > 0xFF) {
        *pbLdbEntries = 0xFF;
    }
    else {
        *pbLdbEntries = (ET9U8)(pLingCmnInfo->Private.wTotalWordInserts - wStartValue);
    }
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Get candidates in the LDB by index.
 * If indexes are out of order the passed array will become sorted.
 *
 * @param pLingInfo                 Pointer to alphabetic information structure.
 * @param wLdbNum                   Language id.
 * @param pdwIndexes                Input index/position (non zero if partial).
 * @param nIndexCount               Word length (active input length).
 *
 * @return None
 */

void ET9FARCALL _ET9AWLdbWordsByIndex(ET9AWLingInfo              * const pLingInfo,
                                      const ET9U16                       wLdbNum,
                                      ET9U32                     * const pdwIndexes,
                                      const ET9UINT                      nIndexCount)
{
    ET9AWLingCmnInfo * const pLingCmnInfo = pLingInfo->pLingCmnInfo;
    ET9U16 wSavedLDB = 0;

    /* Major problems if any passed pointer params are NULL. */

    ET9Assert(pLingInfo && pdwIndexes);

    /* indexes to get? */

    if (!nIndexCount) {
        return;
    }

    /* if length is zero or ldb not initialized, no words from ldb.
       Return with no error, because only propagating low level read errors */

    if (!wLdbNum ||
        pLingInfo->Private.wLDBInitOK != ET9GOODSETUP ||
        !ET9LDBENABLED(pLingCmnInfo)) {

        return;
    }

     /* assure cache */

    if (wLdbNum != pLingCmnInfo->wLdbNum) {

        wSavedLDB = pLingCmnInfo->wLdbNum;

        if (_ET9AWLdbSetActiveLanguage(pLingInfo, wLdbNum)) {
            return;
        }
    }

    /* sort indexes */

    for (;;) {

        ET9BOOL bDirty = 0;

        ET9UINT nIndex;

        for (nIndex = 0; nIndex + 1 < nIndexCount; ++nIndex) {

            if (pdwIndexes[nIndex] > pdwIndexes[nIndex + 1]) {

                const ET9U32 dwTmp = pdwIndexes[nIndex];

                pdwIndexes[nIndex] = pdwIndexes[nIndex + 1];
                pdwIndexes[nIndex + 1] = dwTmp;

                bDirty = 1;
            }
        }

        if (!bDirty) {
            break;
        }
    }

    /* get words */

    {
        const ET9U16 wCodeZero = ALDB.header.wCodeZero;
        const ET9BOOL bDownShift = (ET9BOOL)(ET9DOWNSHIFTALLLDB(pLingCmnInfo) && ALDB.header.bUpperCount);

        ET9UINT nIndex;

        ET9U8 bCursorIndex;
        ET9ALdbCursorData *pCursor;

        ET9AWPrivWordInfo sPrivateWord;

        /* set up 'search' */

        ALDB.search.psTarget = sPrivateWord.Base.sWord;

        ALDB.search.bExhausted = 0;

        /* set up private word*/

        _InitPrivWordInfo(&sPrivateWord);

        sPrivateWord.bWordSrc = ET9WORDSRC_LDB;
        sPrivateWord.bEditDistFree = 1;
        sPrivateWord.Base.bLangIndex = pLingCmnInfo->wLdbNum == pLingInfo->pLingCmnInfo->wFirstLdbNum ? ET9AWFIRST_LANGUAGE : ET9AWSECOND_LANGUAGE;
        sPrivateWord.Base.wWordCompLen = ET9MAXWORDSIZE + 1;

        /* set up cursors */

        pCursor = ALDB.pCursors;
        for (bCursorIndex = 0; bCursorIndex < ALDB.header.bPosCount; ++bCursorIndex, ++pCursor) {

            pCursor->wCode = 0;
            pCursor->dwStartPos = 0;
            pCursor->dwEndPos = 0;
            pCursor->dwJumpPos = 0;
            pCursor->dwJumpAddress = 0;

            {
                ET9U8 ET9FARDATA_LDB *pbCurrData;

                __LdbSetIndex(pCursor, pCursor->dwSourceDataStart);

                pCursor->pbCurrData = pbCurrData;

                if (!__IsExhausted()) {
                    __ET9SearchGetNextIntervalSlow(pLingInfo, pCursor, bCursorIndex, 0, 0);
                }
            }
        }

        /* get index words */

        for (nIndex = 0; nIndex < nIndexCount && !__IsExhausted(); ++nIndex) {

            __ET9HeaderConstsGetNextInterval;

            pCursor = ALDB.pCursors;
            for (bCursorIndex = 0; bCursorIndex < ALDB.header.bPosCount; ++bCursorIndex, ++pCursor) {

                __ET9SearchCatchupStream(pLingInfo, pCursor, bCursorIndex, pdwIndexes[nIndex]);

                if (__IsExhausted()) {
                    return;
                }

                if (pCursor->wCode == wCodeZero) {
                    break;
                }
            }

            sPrivateWord.Base.wWordLen = bCursorIndex;

            {
                ET9AWPrivWordInfo sPublicWord;

                sPublicWord = sPrivateWord;

                /* downshift word if option selected */

                if (bDownShift) {

                    ET9U16 wCount = sPublicWord.Base.wWordLen;
                    ET9SYMB *pSymb = sPublicWord.Base.sWord;

                    for (; wCount; --wCount, ++pSymb) {
                        *pSymb = _ET9SymToLower(*pSymb, pLingCmnInfo->wLdbNum);
                    }
                }

                /* prepare and add to list */

                {
                    const ET9U32 dwCurrWordIndex = pdwIndexes[nIndex] + 1;

                    sPublicWord.xWordFreq = (ET9FREQPART)((ET9FREQPART)0xFFFF / dwCurrWordIndex);   /* zero freq becomes one in "add" */
                    sPublicWord.wEWordFreq = 0;
                    sPublicWord.wTWordFreq = 0;
                    sPublicWord.dwWordIndex = dwCurrWordIndex;
                }

                _ET9AWSelLstAdd(pLingInfo, &sPublicWord, 0, FREQ_NORMAL);
            }
        }
    }

    /* restore */

    if (wSavedLDB) {
        _ET9AWLdbSetActiveLanguage(pLingInfo, wSavedLDB);
    }
}

/*---------------------------------------------------------------------------*/
/** \internal
 * This function sets the active LDB and associated read callback routine.
 * On success it will also setup LDB auto substitution info.
 * Only to be used internally and from the LDB-AS...!
 *
 * @param pLingInfo                 Pointer to alphabetic information structure.
 * @param wLdbNum                   LDB number.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL _ET9AWLdbSetActiveLanguage(ET9AWLingInfo * const pLingInfo,
                                                const ET9U16          wLdbNum)
{
    ET9AWLingCmnInfo * const pLingCmnInfo = pLingInfo->pLingCmnInfo;
    ET9STATUS wStatus;

    WLOG3(fprintf(pLogFile3, "_ET9AWLdbSetActiveLanguage, wLdbNum = %04x\n", wLdbNum);)

    wStatus = _ET9AWSys_BasicValidityCheck(pLingInfo);

    if (wStatus) {
        return wStatus;
    }

    /* set current LDB (this value is used even if the LDB isn't) */

    pLingCmnInfo->wLdbNum = wLdbNum;

    /* set locale (this value is used even if the LDB isn't) */

    _ET9_SetAutoLocale(pLingCmnInfo->Base.pWordSymbInfo, wLdbNum);

    /* consider it an LDB fail until proven ok */

    pLingInfo->Private.wLDBInitOK = 0;

    /*  */

    if (!pLingInfo->ET9AWLdbReadData) {
        return ET9STATUS_NO_DB_INIT;
    }

    /*  If no secondary language ID, set it to 1 */

    if ((pLingCmnInfo->wLdbNum & ET9SLIDMASK) == ET9SLIDNone) {
        pLingCmnInfo->wLdbNum += ET9SLIDDEFAULT;
    }

    /* ok to have no DB, but it's won't be "init ok" */

    if ((wLdbNum & ET9PLIDMASK) == ET9PLIDNone) {
        return ET9STATUS_NONE;
    }

    /* assure LDB access */

    wStatus = __InitDirectAccess();

    if (wStatus) {
        return wStatus;
    }

    /* init the LDB */

    wStatus = __ET9AWLdbInit(pLingInfo);

    if (wStatus) {
        return wStatus;
    }

    /* LDB ok */

    pLingInfo->Private.wLDBInitOK = ET9GOODSETUP;

    /* check for LDB based AS support */

    _ET9AWLdbASInit(pLingInfo, wLdbNum, 0);

    if (ET9LMENABLED(pLingCmnInfo)) {
        _ET9AWLdbLanguageModelInit(pLingInfo, wLdbNum);
    }

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/**
 * This function validates an LDB.
 * It can be called for an LDB other than the currently active one.
 *
 * @param pLingInfo                 Pointer to alphabetic information structure.
 * @param wLdbNum                   LDB number.
 * @param ET9AWLdbReadData          LDB read callback function.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL ET9AWLdbValidate(ET9AWLingInfo * const     pLingInfo,
                                      const ET9U16              wLdbNum,
                                      ET9DBREADCALLBACK const   ET9AWLdbReadData)
{
    ET9STATUS           wStatus;
    ET9U16              wLDBValidID;
    ET9U16              wOldLdbNum;
    ET9U32              dwHashValue = 0;
    ET9DBREADCALLBACK   wOldLdbReadCallback;

    if (pLingInfo == NULL || ET9AWLdbReadData == NULL) {
        return ET9STATUS_INVALID_MEMORY;
    }

    if (pLingInfo->Private.wInfoInitOK != ET9GOODSETUP) {
        return ET9STATUS_NO_INIT;
    }

    if ((wLdbNum & ET9PLIDMASK) == ET9PLIDNone)  {
        return ET9STATUS_LDB_ID_ERROR;
    }

    {
        ET9AWLingCmnInfo * const pLingCmnInfo = pLingInfo->pLingCmnInfo;

        /* save previous LDB etc */

        wOldLdbNum = pLingCmnInfo->wLdbNum;
        wOldLdbReadCallback = pLingInfo->ET9AWLdbReadData;

        pLingCmnInfo->wLdbNum = wLdbNum;
        pLingInfo->ET9AWLdbReadData = ET9AWLdbReadData;

        if ((pLingCmnInfo->wLdbNum & ET9SLIDMASK) == ET9SLIDNone) {
            pLingCmnInfo->wLdbNum += ET9SLIDDEFAULT;
        }

        if ((wStatus = __InitDirectAccess()) == ET9STATUS_NONE) {

            /* get the integrity ID from LDB */

            wStatus = __ET9ReadLDBWord(pLingInfo, ET9LDBOFFSET_CHECKSUM, &wLDBValidID);

            if (!wStatus) {
                __ET9AWLdbHashChunk(pLingInfo, &dwHashValue, 0, ET9LDBOFFSET_CHECKSUM);
                __ET9AWLdbHashChunk(pLingInfo, &dwHashValue, ET9LDBOFFSET_CHECKSUM + 2, (ET9U32) -1);
                wStatus = (ET9STATUS)((wLDBValidID == (ET9U16)dwHashValue) ? ET9STATUS_NONE : ET9STATUS_CORRUPT_DB);
            }
        }

        /* restore previous language */

        pLingInfo->pLingCmnInfo->wLdbNum = ET9PLIDNone;
        pLingInfo->ET9AWLdbReadData = wOldLdbReadCallback;

        if (wOldLdbReadCallback != NULL) {
            _ET9AWLdbSetActiveLanguage(pLingInfo, wOldLdbNum);
        }
    }

    /* done*/

    return wStatus;
}

/*---------------------------------------------------------------------------*/
/**
 * This function initializes ET9AWLingInfo structure.
 * It also sets up the initial language.
 *
 * @param pLingInfo                 Pointer to alphabetic information structure.
 * @param ET9AWLdbReadData          LDB read callback function.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL ET9AWLdbInit(ET9AWLingInfo * const     pLingInfo,
                                  ET9DBREADCALLBACK const   ET9AWLdbReadData)
{
    ET9STATUS wStatus;

    wStatus = _ET9AWSys_BasicValidityCheck(pLingInfo);

    if (wStatus) {
        return wStatus;
    }

    pLingInfo->Private.wLDBInitOK = 0;

    if (!ET9AWLdbReadData)  {
        return ET9STATUS_BAD_PARAM;
    }

    /* skip check ET9AWLdbReadData because it will be checked later on */

    pLingInfo->ET9AWLdbReadData = ET9AWLdbReadData;

    /**/

    WLOG3(fprintf(pLogFile3, "\nET9AWLdbInit, pLingInfo = %p\n", pLingInfo);)

    WLOG3(fprintf(pLogFile3, "\n");)
    WLOG3(fprintf(pLogFile3, "  sizeof(ET9AWLingInfo)                   = %6u\n", sizeof(ET9AWLingInfo));)
    WLOG3(fprintf(pLogFile3, "  sizeof(ET9AWLingCmnInfo)                = %6u\n", sizeof(ET9AWLingCmnInfo));)
    WLOG3(fprintf(pLogFile3, "    sizeof(ET9ASpc)                       = %6u\n", sizeof(pLingInfo->pLingCmnInfo->Private.ASpc));)
    WLOG3(fprintf(pLogFile3, "      sizeof(u)                           = %6u\n", sizeof(pLingInfo->pLingCmnInfo->Private.ASpc.u));)
    WLOG3(fprintf(pLogFile3, "        sizeof(sCmpData)                  = %6u\n", sizeof(pLingInfo->pLingCmnInfo->Private.ASpc.u.sCmpData));)
    WLOG3(fprintf(pLogFile3, "          sizeof(ppbFreqRowStore)         = %6u\n", sizeof(pLingInfo->pLingCmnInfo->Private.ASpc.u.sCmpData.ppbFreqRowStore));)
    WLOG3(fprintf(pLogFile3, "          sizeof(ppbCmpResultRowStore)    = %6u\n", sizeof(pLingInfo->pLingCmnInfo->Private.ASpc.u.sCmpData.ppbCmpResultRowStore));)
    WLOG3(fprintf(pLogFile3, "          sizeof(pppbActiveSpc)           = %6u\n", sizeof(pLingInfo->pLingCmnInfo->Private.ASpc.u.sCmpData.pppbActiveSpc));)
    WLOG3(fprintf(pLogFile3, "        sizeof(sCmpDataFlex)              = %6u\n", sizeof(pLingInfo->pLingCmnInfo->Private.ASpc.u.sCmpDataFlex));)
    WLOG3(fprintf(pLogFile3, "          sizeof(pbLockInfo)              = %6u\n", sizeof(pLingInfo->pLingCmnInfo->Private.ASpc.u.sCmpDataFlex.pbLockInfo));)
    WLOG3(fprintf(pLogFile3, "          sizeof(pbIsFreqPos)             = %6u\n", sizeof(pLingInfo->pLingCmnInfo->Private.ASpc.u.sCmpDataFlex.pbIsFreqPos));)
    WLOG3(fprintf(pLogFile3, "          sizeof(pbIsQualityKey)          = %6u\n", sizeof(pLingInfo->pLingCmnInfo->Private.ASpc.u.sCmpDataFlex.pbIsQualityKey));)
    WLOG3(fprintf(pLogFile3, "          sizeof(pwPrevWordSC)            = %6u\n", sizeof(pLingInfo->pLingCmnInfo->Private.ASpc.u.sCmpDataFlex.pwPrevWordSC));)
    WLOG3(fprintf(pLogFile3, "          sizeof(pbSubstFreqSC)           = %6u\n", sizeof(pLingInfo->pLingCmnInfo->Private.ASpc.u.sCmpDataFlex.pbSubstFreqSC));)
    WLOG3(fprintf(pLogFile3, "          sizeof(ppbFreeDist)             = %6u\n", sizeof(pLingInfo->pLingCmnInfo->Private.ASpc.u.sCmpDataFlex.ppbFreeDist));)
    WLOG3(fprintf(pLogFile3, "          sizeof(ppbEditDist)             = %6u\n", sizeof(pLingInfo->pLingCmnInfo->Private.ASpc.u.sCmpDataFlex.ppbEditDist));)
    WLOG3(fprintf(pLogFile3, "          sizeof(ppbStemDist)             = %6u\n", sizeof(pLingInfo->pLingCmnInfo->Private.ASpc.u.sCmpDataFlex.ppbStemDist));)
    WLOG3(fprintf(pLogFile3, "          sizeof(ppxStemFreq)             = %6u\n", sizeof(pLingInfo->pLingCmnInfo->Private.ASpc.u.sCmpDataFlex.ppxStemFreq));)
    WLOG3(fprintf(pLogFile3, "          sizeof(psLockSymb)              = %6u\n", sizeof(pLingInfo->pLingCmnInfo->Private.ASpc.u.sCmpDataFlex.psLockSymb));)
    WLOG3(fprintf(pLogFile3, "          sizeof(psLockSymbOther)         = %6u\n", sizeof(pLingInfo->pLingCmnInfo->Private.ASpc.u.sCmpDataFlex.psLockSymbOther));)
    WLOG3(fprintf(pLogFile3, "          sizeof(pppbActiveSpc)           = %6u\n", sizeof(pLingInfo->pLingCmnInfo->Private.ASpc.u.sCmpDataFlex.pppbActiveSpc));)
    WLOG3(fprintf(pLogFile3, "\n");)

    /**/

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * This function returns number of chunks appended to the LDB.
 *
 * @param pLingInfo      Pointer to alphabetic information structure.
 *
 * @return Number of chunks.
 */

static ET9U8 ET9LOCALCALL __ET9AWLdbGetChunkCount(ET9AWLingInfo * const     pLingInfo)
{
    ET9U8 bChunkCt = 0;
    ET9U8 bVer = 0;

    __ET9ReadLDBData(pLingInfo, ET9LDBOFFSET_CHUNK_COUNT_BYTE, 1, &bChunkCt);

    __ET9ReadLDBData(pLingInfo, ET9LDBOFFSET_LDBLAYOUTVER, 1, &bVer);

    if (bVer <= 3) {
        bChunkCt >>= 2;
    }

    return bChunkCt;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * This function returns end LDB address.
 *
 * @param pLingInfo      Pointer to alphabetic information structure.
 *
 * @return End address for LDB.
 */

static ET9U32 ET9LOCALCALL __ET9AWLdbGetLDBEndAddress(ET9AWLingInfo * const     pLingInfo)
{
    ET9U32 retAddress = 0;
    ET9U8 bPosCt = 0;
    ET9U32 iSymDecodeCtByte = 0;
    ET9U8 bSymDecCt[2] = {0};
    ET9U16 wSymDecCt = 0;
    ET9U8 bIntervalOffset[3] = {0};
    ET9U32 iLastIntervalOffset = 0;

    __ET9ReadLDBData(pLingInfo, ET9LDBOFFSET_BODY, 1, &bPosCt);

    iSymDecodeCtByte = ET9LDBOFFSET_BODY + 4 * bPosCt + 1;

    __ET9ReadLDBData(pLingInfo, iSymDecodeCtByte, 2, bSymDecCt);

    wSymDecCt = (ET9U16)((((ET9U16)bSymDecCt[0] << 8) & 0xFF00) | (bSymDecCt[1]& 0x00FF));

    iLastIntervalOffset = iSymDecodeCtByte + 2 * (1 + wSymDecCt + 255) + 3 * bPosCt;

    __ET9ReadLDBData(pLingInfo, iLastIntervalOffset, 3, bIntervalOffset);

    retAddress = (ET9U32)((((ET9U32)bIntervalOffset[0] << 16) & 0xFF0000) |
                          (((ET9U32)bIntervalOffset[1] << 8 ) & 0xFF00)   |
                                   (bIntervalOffset[2]        & 0xFF));

    return retAddress - 1;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * This function returns the chunk size.
 *
 * @param pLingInfo             Pointer to alphabetic information structure.
 * @param dwChunkStartAddress   Start address for chunk.
 *
 * @return Chunk size.
 */

static ET9U32 ET9LOCALCALL __ET9AWLdbGetChunkSize(ET9AWLingInfo * const     pLingInfo,
                                                  const ET9U32              dwChunkStartAddress)
{
    ET9U32 dwChunkSize = 0;

    ET9U8 bSize[3] = {0};

    __ET9ReadLDBData(pLingInfo, dwChunkStartAddress + 1, 3, bSize);

    dwChunkSize = (ET9U32)((((ET9U32)bSize[0] << 16) & 0xFF0000) |
                           (((ET9U32)bSize[1] <<  8) & 0xFF00)   |
                                    (bSize[2]        & 0xFF));

    return dwChunkSize;
}

/*---------------------------------------------------------------------------*/
/**
 * This function gets the version string form the active/current LDB.
 *
 * @param pLingInfo                 Pointer to alphabetic information structure.
 * @param psLdbVerBuf               (out) buffer for the LDB version number.
 * @param wBufMaxSize               Maximum buffer size for the LDB version.
 * @param pwBufSize                 (out) actual string length.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL ET9AWLdbGetVersion(ET9AWLingInfo * const   pLingInfo,
                                        ET9SYMB * const         psLdbVerBuf,
                                        const ET9U16            wBufMaxSize,
                                        ET9U16 * const          pwBufSize)
{
    ET9STATUS   wStatus;
    ET9U8      *pbyVer;
    ET9U8       byData;
    ET9U8       bChunkCt = 0;
    ET9U32      dwAddressTracker = 0;
    ET9U32      dwLDBEndAddress = 0;
    ET9U32      dwChunkStartAddress = 0;
    ET9U32      dwChunkEndAddress = 0;
    ET9U8       bChunkID = 0;
    ET9SYMB    *psTmp;
    ET9SYMB    *psBuf = psLdbVerBuf;
    ET9U8       byTemplateStr[] = "XT9 LDB Taa.bb Lcc.dd Vff.gg.hh.ii";

    ET9U16      wTempActiveLdbNum;

    if ((wStatus = __ET9AWLdbBasicValidityCheck(pLingInfo)) != ET9STATUS_NONE) {
        return wStatus;
    }
    if (psBuf == NULL || pwBufSize == NULL) {
        return ET9STATUS_INVALID_MEMORY;
    }
    if (wBufMaxSize < ET9MAXVERSIONSTR) {
        return ET9STATUS_NO_MEMORY;
    }

    wTempActiveLdbNum = pLingInfo->pLingCmnInfo->wLdbNum;

    *pwBufSize = 34;

    /* Copy template string. */

    pbyVer = byTemplateStr;
    psTmp  = psBuf;
    while(*pbyVer) {
        *psTmp++ = (ET9SYMB)*pbyVer++;
    }

    if ((wTempActiveLdbNum == pLingInfo->pLingCmnInfo->wFirstLdbNum) ||
        ((wStatus = _ET9AWLdbSetActiveLanguage(pLingInfo, pLingInfo->pLingCmnInfo->wFirstLdbNum)) == ET9STATUS_NONE)) {

        /* LDB version */

        psBuf += 9;                     /* Skip "ET9 LDB T"   */

        /* Database type */

        wStatus = __ET9ReadLDBData(pLingInfo, ET9LDBOFFSET_DATABASETYPE, 1, &byData);

        if (wStatus) {
            return wStatus;
        }
        _ET9BinaryToHex(byData, psBuf);
        psBuf += 3;                     /* Skip "aa."    */

        /* LDB type */

        wStatus = __ET9ReadLDBData(pLingInfo, ET9LDBOFFSET_LDBLAYOUTVER, 1, &byData);

        if (wStatus) {
            return wStatus;
        }
        _ET9BinaryToHex(byData, psBuf);
        psBuf += 4;                     /* Skip "bb L"   */

        /* Primary language ID, Secondary language ID, Symbol Class */

        wStatus = __ET9ReadLDBData(pLingInfo, ET9LDBOFFSET_PRIMARYLANGID, 1, &byData);

        if (wStatus) {
            return wStatus;
        }
        _ET9BinaryToHex(byData, psBuf);
        psBuf += 3;                     /* Skip "cc."    */

        wStatus = __ET9ReadLDBData(pLingInfo, ET9LDBOFFSET_SECONDARYLANGID, 1, &byData);

        if (wStatus) {
            return wStatus;
        }
        _ET9BinaryToHex(byData, psBuf);
        psBuf += 4;                     /* Skip "dd V"    */

        /* Contents major and minor version, Contents and header deviation. */

        wStatus = __ET9ReadLDBData(pLingInfo, ET9LDBOFFSET_CONTENTSMAJORVER, 1, &byData);

        if (wStatus) {
            return wStatus;
        }
        _ET9BinaryToHex(byData, psBuf);
        psBuf += 3;                     /* Skip "ff."    */
        wStatus = __ET9ReadLDBData(pLingInfo, ET9LDBOFFSET_CONTENTSMINORVER, 1, &byData);

        if (wStatus) {
            return wStatus;
        }
        _ET9BinaryToHex(byData, psBuf);
        psBuf += 3;                     /* Skip "gg."    */
        wStatus = __ET9ReadLDBData(pLingInfo, ET9LDBOFFSET_CONTENTSDEVIATION, 1, &byData);

        if (wStatus) {
            return wStatus;
        }
        _ET9BinaryToHex(byData, psBuf);
        psBuf += 3;                     /* Skip "hh."    */
        wStatus = __ET9ReadLDBData(pLingInfo, ET9LDBOFFSET_CONTENTSHEADERDEV, 1, &byData);

        if (wStatus) {
            return wStatus;
        }
        _ET9BinaryToHex(byData, psBuf);
        psBuf += 2;

        /* loop through LDB's chunks */

        bChunkCt = __ET9AWLdbGetChunkCount(pLingInfo);
        dwLDBEndAddress = __ET9AWLdbGetLDBEndAddress(pLingInfo);
        dwAddressTracker = dwLDBEndAddress;

        while (bChunkCt--) {

            /* Get start and end address of chunk */
            dwChunkStartAddress = dwAddressTracker + 1;
            dwChunkEndAddress = dwAddressTracker + __ET9AWLdbGetChunkSize(pLingInfo, dwChunkStartAddress);

            wStatus = __ET9ReadLDBData(pLingInfo, dwChunkStartAddress, 1, &bChunkID);

            if (bChunkID == ASDB_CHUNK_ID) { /* Get ASDB version */

                ET9U32 dwStartOfASDBheader = dwChunkStartAddress;
                ET9U8 bVersion = 0;

                *psBuf++ = (ET9SYMB)' ';
                *psBuf++ = (ET9SYMB)'A';
                *psBuf++ = (ET9SYMB)'S';
                *psBuf++ = (ET9SYMB)'v';

                wStatus = __ET9ReadLDBData(pLingInfo, dwStartOfASDBheader + 4, 1, &bVersion);
                if (wStatus) {
                    return wStatus;
                }
                _ET9BinaryToHex(bVersion, psBuf);

                psBuf += 2;
                *pwBufSize += 6;

            }
            else if (bChunkID == CONTEXT_CHUNK_ID) { /* Get LM version */

                ET9U32 dwStartOfLMheader = dwChunkStartAddress;
                ET9U8 bVersion = 0;

                *psBuf++ = (ET9SYMB)' ';
                *psBuf++ = (ET9SYMB)'L';
                *psBuf++ = (ET9SYMB)'M';
                *psBuf++ = (ET9SYMB)'v';

                wStatus = __ET9ReadLDBData(pLingInfo, dwStartOfLMheader + 4, 1, &bVersion);
                if (wStatus) {
                    return wStatus;
                }
                _ET9BinaryToHex(bVersion, psBuf);

                psBuf += 2;
                *pwBufSize += 6;
            }

            dwAddressTracker = dwChunkEndAddress;

        }
    }

    if (ET9AWSys_GetBilingualSupported(pLingInfo) &&
        (wStatus = _ET9AWLdbSetActiveLanguage(pLingInfo, pLingInfo->pLingCmnInfo->wSecondLdbNum)) == ET9STATUS_NONE) {

        /* LDB version */

        *psBuf++ = (ET9SYMB)' ';
        *psBuf++ = (ET9SYMB)'T';

        /* Database type */

        wStatus = __ET9ReadLDBData(pLingInfo, ET9LDBOFFSET_DATABASETYPE, 1, &byData);

        if (wStatus) {
            return wStatus;
        }
        _ET9BinaryToHex(byData, psBuf);
        psBuf += 2;                     /* Skip "aa"    */
        *psBuf++ = (ET9SYMB)'.';

        /* LDB type */

        wStatus = __ET9ReadLDBData(pLingInfo, ET9LDBOFFSET_LDBLAYOUTVER, 1, &byData);

        if (wStatus) {
            return wStatus;
        }
        _ET9BinaryToHex(byData, psBuf);
        psBuf += 2;                     /* Skip "bb"   */
        *psBuf++ = (ET9SYMB)' ';
        *psBuf++ = (ET9SYMB)'L';

        /* Primary language ID, Secondary language ID, Symbol Class */

        wStatus = __ET9ReadLDBData(pLingInfo, ET9LDBOFFSET_PRIMARYLANGID, 1, &byData);

        if (wStatus) {
            return wStatus;
        }
        _ET9BinaryToHex(byData, psBuf);
        psBuf += 2;                     /* Skip "cc"    */
        *psBuf++ = (ET9SYMB)'.';

        wStatus = __ET9ReadLDBData(pLingInfo, ET9LDBOFFSET_SECONDARYLANGID, 1, &byData);

        if (wStatus) {
            return wStatus;
        }
        _ET9BinaryToHex(byData, psBuf);
        psBuf += 2;                     /* Skip "dd"    */
        *psBuf++ = (ET9SYMB)' ';
        *psBuf++ = (ET9SYMB)'V';

        /* Contents major and minor version, Contents and header deviation. */

        wStatus = __ET9ReadLDBData(pLingInfo, ET9LDBOFFSET_CONTENTSMAJORVER, 1, &byData);

        if (wStatus) {
            return wStatus;
        }
        _ET9BinaryToHex(byData, psBuf);
        psBuf += 2;                     /* Skip "ff"    */
        *psBuf++ = (ET9SYMB)'.';

        wStatus = __ET9ReadLDBData(pLingInfo, ET9LDBOFFSET_CONTENTSMINORVER, 1, &byData);

        if (wStatus) {
            return wStatus;
        }
        _ET9BinaryToHex(byData, psBuf);
        psBuf += 2;                     /* Skip "gg"    */
        *psBuf++ = (ET9SYMB)'.';
        wStatus = __ET9ReadLDBData(pLingInfo, ET9LDBOFFSET_CONTENTSDEVIATION, 1, &byData);

        if (wStatus) {
            return wStatus;
        }
        _ET9BinaryToHex(byData, psBuf);
        psBuf += 2;                     /* Skip "hh"    */
        *psBuf++ = (ET9SYMB)'.';

        wStatus = __ET9ReadLDBData(pLingInfo, ET9LDBOFFSET_CONTENTSHEADERDEV, 1, &byData);

        if (wStatus) {
            return wStatus;
        }

        _ET9BinaryToHex(byData, psBuf);

        psBuf += 2;
        *pwBufSize += 27;

        /* loop through LDB's chunks */

        bChunkCt = __ET9AWLdbGetChunkCount(pLingInfo);
        dwLDBEndAddress = __ET9AWLdbGetLDBEndAddress(pLingInfo);
        dwAddressTracker = dwLDBEndAddress;

        while (bChunkCt--) {

            /* Get start and end address of chunk */
            dwChunkStartAddress = dwAddressTracker + 1;
            dwChunkEndAddress = dwAddressTracker + __ET9AWLdbGetChunkSize(pLingInfo, dwChunkStartAddress);

            wStatus = __ET9ReadLDBData(pLingInfo, dwChunkStartAddress, 1, &bChunkID);

            if (bChunkID == ASDB_CHUNK_ID) { /* Get ASDB version */

                ET9U32 dwStartOfASDBheader = dwChunkStartAddress;
                ET9U8 bVersion = 0;

                *psBuf++ = (ET9SYMB)' ';
                *psBuf++ = (ET9SYMB)'A';
                *psBuf++ = (ET9SYMB)'S';
                *psBuf++ = (ET9SYMB)'v';

                wStatus = __ET9ReadLDBData(pLingInfo, dwStartOfASDBheader + 4, 1, &bVersion);
                if (wStatus) {
                    return wStatus;
                }
                _ET9BinaryToHex(bVersion, psBuf);

                psBuf += 2;
                *pwBufSize += 6;

            }
            else if (bChunkID == CONTEXT_CHUNK_ID) { /* Get LM version */

                ET9U32 dwStartOfLMheader = dwChunkStartAddress;
                ET9U8 bVersion = 0;

                *psBuf++ = (ET9SYMB)' ';
                *psBuf++ = (ET9SYMB)'L';
                *psBuf++ = (ET9SYMB)'M';
                *psBuf++ = (ET9SYMB)'v';

                wStatus = __ET9ReadLDBData(pLingInfo, dwStartOfLMheader + 4, 1, &bVersion);
                if (wStatus) {
                    return wStatus;
                }
                _ET9BinaryToHex(bVersion, psBuf);

                psBuf += 2;
                *pwBufSize += 6;
            }

            dwAddressTracker = dwChunkEndAddress;

        }
    }

    if (wTempActiveLdbNum != pLingInfo->pLingCmnInfo->wLdbNum) {
        _ET9AWLdbSetActiveLanguage(pLingInfo, wTempActiveLdbNum);
    }

    return wStatus;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * This function initializes the class model for the active/current LDB.
 *
 * @param pLingInfo                 Pointer to alphabetic information structure.
 * @param wLdbNum                   Language ID.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL _ET9AWLdbLanguageModelInit(ET9AWLingInfo  * const pLingInfo,
                                                const ET9U16           wLdbNum)
{
    ET9STATUS               wStatus = ET9STATUS_NONE;
    ET9U8                   bChunkCt;
    ET9U8                   bChunkID;
    ET9U32                  dwAddressTracker;
    ET9U32                  dwChunkSize;
    ET9U8                   bACMChunkFound = 0;
    ET9AWLingCmnInfo * const pLingCmnInfo = pLingInfo->pLingCmnInfo;

    ET9Assert(pLingInfo);

#ifdef ALDB_LAYOUT_VERSION_4_SUPPORT
    dwAddressTracker = ET9LDBOFFSET_BODY;
#else /* ALDB_LAYOUT_VERSION_4_SUPPORT */
    dwAddressTracker = pLingCmnInfo->Private.ALdb.header.pdwIntervalOffsets[pLingCmnInfo->Private.ALdb.header.bPosCount];
#endif /* ALDB_LAYOUT_VERSION_4_SUPPORT */

    /* if LDB has been initialized/validated */

    if ((wLdbNum & ET9PLIDMASK) != ET9PLIDNone &&
        pLingInfo->Private.wLDBInitOK == ET9GOODSETUP) {

        /* check to see if LDB has AS chunk */

        bChunkCt = _ET9ReadLDBByte(pLingInfo, ET9LDBOFFSET_CHUNK_COUNT_BYTE);

        if (_ET9ReadLDBByte(pLingInfo, ET9LDBOFFSET_LDBLAYOUTVER) <= 3) {
            bChunkCt >>= 2;
        }

        /* loop through LDB's chunks, looking for the CONTEXT_CHUNK_ID */

        while (bChunkCt--) {

            bChunkID = _ET9ReadLDBByte(pLingInfo, dwAddressTracker + CHUNK_ID_OFFSET);

            /* save the context model specific info in the lingustics struct, if context info chunk found in LDB */

            if (bChunkID == CONTEXT_CHUNK_ID) {

                bACMChunkFound = 1;

                pLingCmnInfo->Private.ALdbLM.bSupported = 1;
                pLingCmnInfo->Private.ALdbLM.bALMVersion = _ET9ReadLDBByte(pLingInfo, dwAddressTracker + ALM_VERSION_OFFSET);
                pLingCmnInfo->Private.ALdbLM.bALMLangID = _ET9ReadLDBByte(pLingInfo, dwAddressTracker + ALM_LANGUAGE_OFFSET);
                pLingCmnInfo->Private.ALdbLM.dwALMStartAddress = _ET9ReadLDBWord3(pLingInfo, dwAddressTracker + ALM_START_ADDRESS_OFFSET);
                pLingCmnInfo->Private.ALdbLM.dwALMEndAddress = _ET9ReadLDBWord3(pLingInfo, dwAddressTracker + ALM_END_ADDRESS_OFFSET);
                pLingCmnInfo->Private.ALdbLM.dwNumEntries = _ET9ReadLDBWord3(pLingInfo, dwAddressTracker + ALM_NUM_ENTRIES_OFFSET);
                pLingCmnInfo->Private.ALdbLM.wNumClasses = _ET9ReadLDBWord2(pLingInfo, dwAddressTracker + ALM_NUM_CLASSES_OFFSET);
                pLingCmnInfo->Private.ALdbLM.wEBits = _ET9ReadLDBWord2(pLingInfo, dwAddressTracker + ALM_EMISSION_ENCODING_OFFSET);
                pLingCmnInfo->Private.ALdbLM.wTBits = _ET9ReadLDBWord2(pLingInfo, dwAddressTracker + ALM_TRANSITION_ENCODING_OFFSET);
                pLingCmnInfo->Private.ALdbLM.bScalingFactor = _ET9ReadLDBByte(pLingInfo, dwAddressTracker + ALM_SCALE_FACTOR_OFFSET);
                pLingCmnInfo->Private.ALdbLM.bAddConstant = _ET9ReadLDBByte(pLingInfo, dwAddressTracker + ALM_ADD_CONSTANT_OFFSET);

            }
            else { /* else skip to next chunk in LDB */
                dwChunkSize = _ET9ReadLDBWord3(pLingInfo, dwAddressTracker + CHUNK_SIZE_OFFSET);
                dwAddressTracker += dwChunkSize;
            }
        }

        if (!bACMChunkFound) {
            pLingCmnInfo->Private.ALdbLM.bSupported = 0;
            pLingCmnInfo->Private.ALdbLM.dwNumEntries = 0;
            pLingCmnInfo->Private.ALdbLM.wNumClasses = 0;
            pLingCmnInfo->Private.ALdbLM.wEBits = 0;
            pLingCmnInfo->Private.ALdbLM.wTBits = 0;
            pLingCmnInfo->Private.ALdbLM.bScalingFactor = 0;
            pLingCmnInfo->Private.ALdbLM.bAddConstant = 0;
        }
    }

    return wStatus;
}

#if 0
/*---------------------------------------------------------------------------*/
/**
 * Get next symbols prediction.
 *
 * @param pLingInfo                 Pointer to alphabetic information structure.
 * @param pWordSymbInfo             Pointer to word symbol info structure.
 *
 * @return Number of found symbols.
 */

ET9U8 ET9FARCALL _ET9AWLdbGetNextSymbolsPrediction(ET9AWLingInfo     *pLingInfo,
                                                   ET9AWWordSymbInfo *pWordSymbInfo)
{
    ET9U16  wTargetLen;
    ET9SYMB psTargetWord[ET9MAXLDBWORDSIZE];
    ET9SYMB psTargetSyms[ET9MAXLDBWORDSIZE];
    ET9U32  pdwTargetSymsFreq[ET9MAXLDBWORDSIZE];
    ET9U32  dwWordFreq;
    ET9UINT nSymbIndex = 0;
    ET9U16  wWordFreq;
    ET9U8   bI;
    ET9U8   bJ;

    ET9Assert(pLingInfo != NULL);
    ET9Assert(pWordSymbInfo != NULL);

    _ET9ClearMem((ET9U8*)psTargetSyms, sizeof(psTargetSyms));
    _ET9ClearMem((ET9U8*)pdwTargetSymsFreq, sizeof(pdwTargetSymsFreq));
    _ET9ClearMem((ET9U8*)pLingInfo->Private.sPredictSyms, sizeof(pLingInfo->Private.sPredictSyms));

    __ET9CompareStart(pLingInfo, pWordSymbInfo, 0, pWordSymbInfo->bNumSymbs, 0);

    __ET9SearchStart(pLingInfo, psTargetWord, &wTargetLen, ET9MAXLDBWORDSIZE);

    while (!__IsExhausted()) {

        if (psTargetWord[pWordSymbInfo->bNumSymbs]) {

            if (__GetWordIndex() >= 0xFFFF) {
                wWordFreq = 1;
            }
            else {
                wWordFreq = (ET9U16)(0xFFFF / (__GetWordIndex() + 1));
            }

            /* search for existing until hit end */

            for (bI = 0; bI < ET9MAXLDBWORDSIZE; ++bI) {

                if (!psTargetSyms[bI]) {
                    /* create new entry */
                    psTargetSyms[bI] = psTargetWord[pWordSymbInfo->bNumSymbs];
                    pdwTargetSymsFreq[bI] = wWordFreq;
                    break;
                }
                else if (psTargetSyms[bI] == psTargetWord[pWordSymbInfo->bNumSymbs]) {
                    /* add new freq */
                    pdwTargetSymsFreq[bI] += wWordFreq;
                    break;
                }
            }
        }

        __ET9SearchGetNext();
    }

    /* now fill up predict symbols array */

    for (bI = 0; bI < ET9PREDICT_SYMS; ++bI) {

        dwWordFreq = 0;

        for (bJ = 0; bJ < ET9MAXLDBWORDSIZE; ++bJ) {
            if (!psTargetSyms[bJ]) {
                break;
            }
            if (pdwTargetSymsFreq[bJ] > dwWordFreq) {
                dwWordFreq = pdwTargetSymsFreq[bJ];
                nSymbIndex = bJ;
            }
        }
        pdwTargetSymsFreq[nSymbIndex] = 0;
        pLingInfo->Private.sPredictSyms[bI] = psTargetSyms[nSymbIndex];
    }

    return bI;
}

#endif


/* clean up defines */

#undef __GetPosLock
#undef __SetPosLock
#undef __IsCodeActive
#undef __GetActiveSectionSpc
#undef __GetActiveSectionFlexSpc
#undef __IsCodeActiveSpc
#undef __IsCodeExact
#undef __ActivateSymbol
#undef __DeactivateCode
#undef __GetCodeFreq


#endif /* ET9_ALPHABETIC_MODULE */
/*! @} */
/* ----------------------------------< eof >--------------------------------- */
