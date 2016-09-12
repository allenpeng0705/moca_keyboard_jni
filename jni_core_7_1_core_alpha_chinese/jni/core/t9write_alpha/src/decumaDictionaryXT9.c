/*******************************************************************************
;*******************************************************************************
;**                                                                           **
;**                    COPYRIGHT 2010 NUANCE COMMUNICATIONS                   **
;**                                                                           **
;**                NUANCE COMMUNICATIONS PROPRIETARY INFORMATION              **
;**                                                                           **
;**     This software is supplied under the terms of a license agreement      **
;**     or non-disclosure agreement with Nuance Communications and may not    **
;**     be copied or disclosed except in accordance with the terms of that    **
;**     agreement.                                                            **
;**                                                                           **
;*******************************************************************************
;******************************************************************************/


/* Implements decumaUnpackXT9Dictionary()
 * Base LDB reading is adapted from the file ldb_verify.c which was recieved 
 * via email from Erland Unruhe. 
 *
 * TODO: We should be able to use the module 'ldbwords' in Tegic CVS pretty 
 *       straight off. 
 *       The files et9/ldbwords/core/ldb_words.c and .h should be able to 
 *       replace just about all of the XT9-specific code in this file.
 * 
 * TODO: We should also be able to use the 'udbwords' in Tegic CVS to 
 *       parse an XT9 UDB. 
 */

#include "decumaDictionaryXT9.h"
#include "decumaAssert.h"
#include "decumaTrie.h"
#include "decumaMemory.h"
#include "decuma_hwr.h"
#include "et9api.h"
#include "udb_words.h"
#include "decumaBasicTypesMinMax.h"

#ifdef _DEBUG
/*#define PRINT_WORDS */
#endif
#ifdef PRINT_WORDS
#include <stdio.h>
#include <tchar.h>
FILE * g_fp;
errno_t error;
#endif

#define NODE_GRANULARITY 10000

/*******************************************************************************
;*******************************************************************************
;**                                                                           **
;**                  COPYRIGHT 2001-2006 TEGIC COMMUNICATIONS                 **
;**                                                                           **
;**                TEGIC COMMUNICATIONS PROPRIETARY INFORMATION               **
;**                                                                           **
;**     This software is supplied under the terms of a license agreement      **
;**     or non-disclosure agreement with Tegic Communications and may not     **
;**     be copied or disclosed except in accordance with the terms of that    **
;**     agreement.                                                            **
;**                                                                           **
;*******************************************************************************
;**                                                                           **
;**     FileName: ldb_verify.c                                                **
;**                                                                           **
;**  Description:                                                             **
;**                                                                           **
;*******************************************************************************
;******************************************************************************/

/*#include <assert.h> */
/*#include <string.h> */
/*#include <stdio.h> */
/*#include <ctype.h> */
/*#include <wchar.h> */
/*#include <time.h> */

/* #include "ldb_verify.h" */
/* #include "wordlist.h" */
/* #include "et9awapi.h" */


/* ********************************************************************  */

#define ET9MAX_CHARMAP       256
#define ET9MAX_EXPLFREQS     5
#define ET9MAX_EXP_TERM_PUNCTS 16

#define ET9MAXLDBWORDSIZE    32

#define ET9PREDICT_SYMS      5

#define ALDB_COMPARE_MAX_POS             (ET9MAXLDBWORDSIZE)
#define ALDB_COMPARE_MAX_CODE_BYTES      (17)

#define ALDB_HEADER_MAX_CHAR_CODES       (ALDB_COMPARE_MAX_CODE_BYTES*8)
#define ALDB_HEADER_MAX_DIRECT_ENCODE    0x100

#define ALDB_HEADER_ONE_BYTE_SIZE        0xFF

#define ALDB_CURSOR_DATA_CACHE_SIZE      10


#define ALDB_INTERVAL_MULTI_BYTE_CODE    0xFF
#define ALDB_INTERVAL_END_CHAR_VAL       0x16FD
#define ALDB_INTERVAL_JUMP_CHAR_VAL      0x16FE
#define ALDB_INTERVAL_EXTEND_CHAR_VAL    0x16FF

#if ET9MAXLDBWORDSIZE >= ET9MAXUDBWORDSIZE
#define ET9MAXWORDSIZE ET9MAXLDBWORDSIZE 
#else
#define ET9MAXWORDSIZE ET9MAXUDBWORDSIZE
#endif

/* ********************************************************************  */

#define __LdbUpdateCache(pCursor)                                                           \
{                                                                                           \
    ET9U32 dwBytesRead;                                                                     \
    ET9AWLdbReadData(pRawData, dataSize, (pCursor)->dwCurrCacheStart, \
					 ALDB_CURSOR_DATA_CACHE_SIZE,                                           \
                     (pCursor)->pbCache,                                                    \
                     &dwBytesRead);                                                         \
}

#define __LdbGetByte(pCursor)   (*(pCursor)->pbCurrData)

#define __LdbSetIndex(pCursor,index)                                                        \
{                                                                                           \
    (pCursor)->dwCurrCacheStart = (index);                                                  \
    (pCursor)->pbCurrData = (pCursor)->pbCache; __LdbUpdateCache(pCursor);                  \
}

#define __LdbIncIndex(pCursor)                                                              \
{                                                                                           \
    if (++(pCursor)->pbCurrData == (pCursor)->pbCacheEnd) {                                 \
        __LdbSetIndex(pCursor, (pCursor)->dwCurrCacheStart + ALDB_CURSOR_DATA_CACHE_SIZE);  \
    };                                                                                      \
}

#define __LdbDecIndex(pCursor)                                                              \
{                                                                                           \
    if (--(pCursor)->pbCurrData < (pCursor)->pbCache) {                                     \
        __LdbSetIndex(pCursor, (pCursor)->dwCurrCacheStart - 1);                            \
    };                                                                                      \
}

/* ********************************************************************  */

#define __IsExhausted()         (pALDB->search.bExhausted)

#define __IsLengthExhausted()   (pALDB->search.dwCurrItem > pALDB->search.dwLengthEnd)

#define __GetPosOrder(bPos)     (pALDB->search.pbPosCurrOrder[bPos])

#define __GetWordIndex()        (pALDB->search.dwCurrItem)

#define __ET9SearchGetNext()    __ET9SearchMoveToItem(pRawData, dataSize, pALDB, pALDB->search.dwCurrItem + 1)

/* ********************************************************************  */

#define __CodeToSymbol(wCode)                                                   \
(                                                                               \
    pALDB->header.psDecodeTable[wCode]                                            \
)

#define __SymbolToCode(sSymb)                                                   \
(                                                                               \
    (sSymb < ALDB_HEADER_MAX_DIRECT_ENCODE)                                     \
    ?                                                                           \
    (pALDB->header.pbEncodeTable[sSymb])                                          \
    :                                                                           \
    (                                                                           \
        (                                                                       \
         !pALDB->header.wEncodeExtendCount ||                                     \
         sSymb < pALDB->header.sEncodeExtendFirstChar ||                          \
         sSymb > pALDB->header.sEncodeExtendLastChar                              \
        )                                                                       \
        ?                                                                       \
        (pALDB->header.wCodeNone)                                                 \
        :                                                                       \
        (__ET9ExtendedSymbolToCode(&ALDB, sSymb))                               \
    )                                                                           \
)

#define __IsCodeActive(wPos, wCode)                                             \
(                                                                               \
    (wPos >= pALDB->compare.wLength)                                              \
    ?                                                                           \
    (pALDB->compare.bStemsAllowed || wCode == pALDB->header.wCodeZero)              \
    :                                                                           \
    (pALDB->compare.ppbKey[wPos][wCode])                                          \
)

#define __IsCodeExact(wPos, wCode)                                              \
    (pALDB->compare.ppbExact[wPos][wCode >> 3] & (1 << (wCode & 0x7)))

#define __GetCodeFreq(wPos, wCode)                                              \
    (pALDB->compare.ppbKey[wPos][wCode])

#define __ActivateSymbol(wPos, wSymbol, bSymFreq, bExact)                       \
{                                                                               \
    if (wPos < ALDB_COMPARE_MAX_POS) {                                          \
        ET9U16 wCode = (ET9U16)__SymbolToCode(wSymbol);                         \
        pALDB->compare.ppbKey[wPos][wCode] = bSymFreq;                            \
        if (bExact) {                                                           \
            pALDB->compare.ppbExact[wPos][wCode >> 3] |= (1 << (wCode & 0x7));    \
        }                                                                       \
    };                                                                          \
}

#define ET9SYMBMAXVAL    ((ET9SYMB)((1 << (sizeof(ET9SYMB)*8)) - 1))    /**< \internal unicode symbol max val */



typedef struct ET9ALdbHeaderData_s
{
    ET9U8               bPosCount;

    ET9U8               pbEncodeTable[ALDB_HEADER_MAX_DIRECT_ENCODE];

    ET9U16              wEncodeExtendCount;
    ET9SYMB             sEncodeExtendFirstChar;
    ET9SYMB             sEncodeExtendLastChar;

    ET9U16              wDecodeCount;
    ET9SYMB             psDecodeTable[ALDB_HEADER_MAX_CHAR_CODES];

    ET9U8               pbOneByteCodes[ALDB_HEADER_ONE_BYTE_SIZE];
    ET9U8               pbOneByteLengths[ALDB_HEADER_ONE_BYTE_SIZE];

    ET9U8               pbPosOrder[ET9MAXLDBWORDSIZE];

    ET9U32              pdwLengthEnds[ET9MAXLDBWORDSIZE];

    ET9U32              pdwIntervalOffsets[ET9MAXLDBWORDSIZE+1];
    
    ET9U16              wCodeNone;                  /* Code for unmapped char */
    ET9U16              wCodeZero;                  /* Code for string 0 / termination */

    ET9U16              wCodeIntervalEnd;
    ET9U16              wCodeIntervalJump;
    ET9U16              wCodeIntervalExtend;
} ET9ALdbHeaderData;

typedef struct ET9ALdbCompareData_s
{
    ET9U16              wLength;
    ET9U16              wCmpLength;

    ET9U8               bStemsAllowed;

    ET9U8               bSpcCompare;
    ET9U16              wSpcMinLength;

    ET9U8               ppbKey[ALDB_COMPARE_MAX_POS][ALDB_HEADER_MAX_CHAR_CODES];
    ET9U8               ppbExact[ALDB_COMPARE_MAX_POS][ALDB_COMPARE_MAX_CODE_BYTES];
} ET9ALdbCompareData;

typedef struct ET9ALdbCursorData_s
{
    ET9U32              dwStartPos;
    ET9U32              dwLength;

    ET9U32              dwJumpPos;
    ET9U32              dwJumpAddress;

    ET9U32              dwSourceDataStart;
    ET9U32              dwSourceDataLength;

    ET9U32              dwCurrCacheStart;

    ET9U8               *pbCurrData;

    ET9U8               *pbCacheEnd;

    ET9U16              wCode;

    ET9U8               pbCache[ALDB_CURSOR_DATA_CACHE_SIZE];

} ET9ALdbCursorData;

typedef struct ET9ALdbSearchData_s
{
    ET9SYMB             *psTarget;
    ET9U16              *pwLength;

    ET9U32              dwLengthEnd;
    ET9U32              dwCurrItem;
    ET9U16              wTargetLength;
    ET9U8               bExhausted;
    ET9U8               pbPosCurrOrder[ET9MAXLDBWORDSIZE];

} ET9ALdbSearchData;


typedef struct ET9ALdb_s
{
    ET9ALdbHeaderData   header;
    ET9ALdbCompareData  compare;
    ET9ALdbCursorData   pCursors[ET9MAXLDBWORDSIZE];
    ET9ALdbSearchData   search;

} ET9ALdb;
/* */
/*// ********************************************************************  */
/* */
/*class ldbRead */
/*{ */
/*    public: */
/*                            ldbRead(char *pcLdbName); */
/*                            ~ldbRead(); */
/* */
/*        ET9U8               isOk()      { return bContentOk; }; */
/* */
/*        void                getWords(wordList *pwl); */
/* */
/*        void                benchmark1(); */
/* */
/*    private: */
/*        FILE                *pF; */
/* */
/*        ET9U8               bContentOk; */
/* */
/*        ET9U32              readBytes(ET9U32 dwTargetPos, void *pDest, ET9U32 dwSize); */
/* */
/*        ET9U8               _ET9ReadLDBByte(ET9U32 dwTargetPos)     { ET9U8  bValue;  readByte(dwTargetPos, bValue); return bValue; }; */
/*        ET9U16              _ET9ReadLDBWord2 (ET9U32 dwTargetPos)   { ET9U16 wValue;  readWord2(dwTargetPos, wValue); return wValue; }; */
/*        ET9U32              _ET9ReadLDBWord3 (ET9U32 dwTargetPos)   { ET9U32 dwValue; readWord3(dwTargetPos, dwValue); return dwValue; }; */
/* */
/*        ET9U32              readByte(ET9U32 dwTargetPos, ET9U8 &bValue); */
/*        ET9U32              readWord2(ET9U32 dwTargetPos, ET9U16 &wValue); */
/*        ET9U32              readWord3(ET9U32 dwTargetPos, ET9U32 &dwValue); */
/* */
/*        void                readStandardHeader(); */
/*        void                readSpecific(ET9U32 dwStartPos); */
/* */
/*        ET9ALdb             ALDB; */
/* */
/*        void                __ET9CompareStart(); */
/*        void                __ET9SearchStart(ET9SYMB *psTarget, ET9U16 *pwLength, ET9U16 wTargetLength); */
/* */
/*        void                __ET9SearchMoveToItem(ET9U32 dwItem); */
/*        void                __ET9SearchGetNextInterval(ET9U8 bPos, ET9U32 dwTargetPos); */
/*        void                ET9AWLdbReadData(ET9U32 dwOffset, ET9U32 dwNumberOfBytesToRead, ET9U8 *pbDst, ET9U32 *pdwNumberOfBytesRead); */
/* */
/*        void                iterateStrings(void *pOwner = NULL, pHandleCodeStringW_t pHandler = NULL); */
/* */
/*        static ET9U8        addString(void *pOwner, ET9SYMB *psString, ET9U16 wLen); */
/*}; */

/* ********************************************************************  */

/* Function prototypes */

static DECUMA_STATUS __ET9SearchGetNextInterval (void const * const pRawData, long const dataSize, ET9ALdb * pALDB, ET9U8 bPos, ET9U32 dwTargetPos);
static DECUMA_STATUS __ET9SearchMoveToItem (void const * const pRawData, long const dataSize, ET9ALdb * pALDB, ET9U32 dwItem);
static DECUMA_STATUS readStandardHeader (void const * const pRawDatalong, long const dataSize );
static DECUMA_STATUS readSpecific (void const * const pRawData, long const dataSize, ET9ALdb * pALDB, ET9U32 dwStartPos);
static ET9U32 readByte (void const * const pRawData, long const dataSize, ET9U32 dwTargetPos, ET9U8  * bValue);
static ET9U32 readWord2 (void const * const pRawData, long const dataSize, ET9U32 dwTargetPos, ET9U16 * wValue);
static ET9U32 readWord3 (void const * const pRawData, long const dataSize, ET9U32 dwTargetPos, ET9U32 * dwValue);

/* Function definitions */

static ET9U8               _ET9ReadLDBByte  (void const * const pRawData, long const dataSize, ET9U32 dwTargetPos)     { ET9U8  bValue;  readByte(pRawData, dataSize, dwTargetPos, &bValue); return bValue; }
static ET9U16              _ET9ReadLDBWord2 (void const * const pRawData, long const dataSize, ET9U32 dwTargetPos)   { ET9U16 wValue;  readWord2(pRawData, dataSize, dwTargetPos, &wValue); return wValue; }
static ET9U32              _ET9ReadLDBWord3 (void const * const pRawData, long const dataSize, ET9U32 dwTargetPos)   { ET9U32 dwValue; readWord3(pRawData, dataSize, dwTargetPos, &dwValue); return dwValue; }

#if defined(DECUMA_ASSERT_ENABLE) || defined(DECUMA_ASSERT_OVERRIDE)
#include <string.h>
void const * pTemp = NULL;
#endif

/* ********************************************************************  */

static ET9U32 readBytes (char const * const pRawData, long const dataSize, ET9U32 dwTargetPos, void *pDest, ET9U32 dwSize)
{
	decumaAssert(pRawData == pTemp);

	if ( dwTargetPos + dwSize >= dataSize )
	{
		dwSize = dataSize - dwTargetPos;
	}

	decumaMemcpy(pDest, pRawData + dwTargetPos, dwSize);

    return dwTargetPos + dwSize;
}

/* ********************************************************************  */

static void ET9AWLdbReadData (char const * const pRawData, long const dataSize, ET9U32 dwOffset, ET9U32 dwNumberOfBytesToRead, ET9U8 *pbDst, ET9U32 *pdwNumberOfBytesRead)
{
	decumaAssert(pRawData == pTemp);
    /* size_t status; */

    /* status = fseek(pF, dwOffset, SEEK_SET); */
    /* assert(status == 0); */
	/* *pdwNumberOfBytesRead = fread(pbDst, 1, dwNumberOfBytesToRead, pF); */

	if ( dwOffset + dwNumberOfBytesToRead >= dataSize )
	{
		dwNumberOfBytesToRead = dataSize - dwOffset;
	}

	decumaMemcpy(pbDst, pRawData + dwOffset, dwNumberOfBytesToRead);
	*pdwNumberOfBytesRead = dwNumberOfBytesToRead;
}

/* ********************************************************************  */

static ET9U32 readByte (void const * const pRawData, long const dataSize, ET9U32 dwTargetPos, ET9U8  * bValue)
{
	decumaAssert(pRawData == pTemp);

    readBytes(pRawData, dataSize, dwTargetPos, bValue, 1);

    return dwTargetPos + 1;
}

/* ********************************************************************  */

static ET9U32 readWord2 (void const * const pRawData, long const dataSize, ET9U32 dwTargetPos, ET9U16 * wValue)
{
    ET9U8 bData[2];

	decumaAssert(pRawData == pTemp);

	readBytes(pRawData, dataSize, dwTargetPos, &bData[0], 2);

    *wValue = (bData[0] << 8) + bData[1];

    return dwTargetPos + 2;
}

/* ********************************************************************  */

static ET9U32 readWord3 (void const * const pRawData, long const dataSize, ET9U32 dwTargetPos, ET9U32 * dwValue)
{
    ET9U8 bData[3];

	decumaAssert(pRawData == pTemp);

	readBytes(pRawData, dataSize, dwTargetPos, &bData[0], 3);

    *dwValue = (bData[0] << 16) + (bData[1] << 8) + bData[2];

    return dwTargetPos + 3;
}

/* ********************************************************************  */

static DECUMA_STATUS readStandardHeader (void const * const pRawData, long const dataSize)
{
    /* LDB Layout version */
    ET9U8 bLayoutVersion;
	decumaAssert(pRawData == pTemp);

	readByte(pRawData, dataSize, 0x20, &bLayoutVersion);
	if (bLayoutVersion != 3)
		return decumaInvalidDictionary;

	return decumaNoError;
}

/* ********************************************************************  */

static DECUMA_STATUS readSpecific (void const * const pRawData, long const dataSize, ET9ALdb * pALDB, ET9U32 dwStartPos)
{
    ET9U32            dwIndex;
    ET9ALdbCursorData *pCursData;
    ET9SYMB           *pSymb;
    ET9U8             *pByter;
    ET9U32            *pDW;    

	decumaAssert(pRawData == pTemp);
    /* Number of character positions (max word length) - 1 byte */

    pALDB->header.bPosCount = _ET9ReadLDBByte(pRawData, dataSize, dwStartPos++);

    if (!pALDB->header.bPosCount || pALDB->header.bPosCount > ET9MAXLDBWORDSIZE) {
		return decumaInvalidDictionary;
    }

    /* Position order data - 1 byte per position */

    pByter = pALDB->header.pbPosOrder;
    for (dwIndex = 0; dwIndex < pALDB->header.bPosCount; ++dwIndex) {

        *pByter++ = _ET9ReadLDBByte(pRawData, dataSize, dwStartPos++);
    }

    /* Word length end (last) data - 3 bytes per position */

    pDW = pALDB->header.pdwLengthEnds;
    for (dwIndex = 0; dwIndex < pALDB->header.bPosCount; ++dwIndex) {

        *pDW++ = _ET9ReadLDBWord3(pRawData, dataSize, dwStartPos);

        dwStartPos += 3;
    }

    /* Character decode data - 2 byte count plus 2 bytes per item */

    pALDB->header.wDecodeCount = _ET9ReadLDBWord2(pRawData, dataSize, dwStartPos);

    dwStartPos += 2;

    if (pALDB->header.wDecodeCount > ALDB_HEADER_MAX_CHAR_CODES) {
		return decumaInvalidDictionary;
    }

    pSymb = pALDB->header.psDecodeTable;
    for (dwIndex = 0; dwIndex < pALDB->header.wDecodeCount; ++dwIndex) {

        ET9SYMB sSymb = _ET9ReadLDBWord2(pRawData, dataSize, dwStartPos);

#define LDB_SPACE_CHAR_SUBSTITUTE 0x20
        if (sSymb == 0x20) {
            sSymb = LDB_SPACE_CHAR_SUBSTITUTE;
        }

        *pSymb++ = sSymb;

        dwStartPos += 2;
    }

    /* One byte interval decode data (codes + lengths) - 2 * 255 bytes */

    pByter = pALDB->header.pbOneByteCodes;
    for (dwIndex = 0; dwIndex < ALDB_HEADER_ONE_BYTE_SIZE; ++dwIndex) {

        *pByter++ = _ET9ReadLDBByte(pRawData, dataSize, dwStartPos++);
    }

    pByter = pALDB->header.pbOneByteLengths;
    for (dwIndex = 0; dwIndex < ALDB_HEADER_ONE_BYTE_SIZE; ++dwIndex) {

        *pByter++ = _ET9ReadLDBByte(pRawData, dataSize, dwStartPos++);
    }

    /* Start offset for each position's interval data stream - 3 bytes per (position + 1) */

    pDW = pALDB->header.pdwIntervalOffsets;
    for (dwIndex = 0; dwIndex <= pALDB->header.bPosCount; ++dwIndex) {

        *pDW++ = _ET9ReadLDBWord3(pRawData, dataSize, dwStartPos);

        dwStartPos += 3;
    }

    /* Locate the control codes */

    pALDB->header.wCodeZero = 0xFFFF;
    pALDB->header.wCodeIntervalEnd = 0xFFFF;
    pALDB->header.wCodeIntervalJump = 0xFFFF;
    pALDB->header.wCodeIntervalExtend = 0xFFFF;

    pSymb = pALDB->header.psDecodeTable;
    for (dwIndex = 0; dwIndex < pALDB->header.wDecodeCount; ++dwIndex, pSymb++) {
        
        if (*pSymb == 0) {

            pALDB->header.wCodeZero = (ET9U16)dwIndex;
        }
        else if (*pSymb == ALDB_INTERVAL_END_CHAR_VAL) {

            pALDB->header.wCodeIntervalEnd = (ET9U16)dwIndex;
        }
        else if (*pSymb == ALDB_INTERVAL_JUMP_CHAR_VAL) {

            pALDB->header.wCodeIntervalJump = (ET9U16)dwIndex;
        }
        else if (*pSymb == ALDB_INTERVAL_EXTEND_CHAR_VAL) {

            pALDB->header.wCodeIntervalExtend = (ET9U16)dwIndex;
        }
    }

    if (pALDB->header.wCodeZero == 0xFFFF ||
        pALDB->header.wCodeIntervalEnd == 0xFFFF ||
        pALDB->header.wCodeIntervalJump == 0xFFFF ||
        pALDB->header.wCodeIntervalExtend == 0xFFFF) {

		return decumaInvalidDictionary;
	}

    pALDB->header.wCodeNone = pALDB->header.wCodeIntervalEnd;     /* Anything that isn't actually decoded... */

    /* Recreate character encode info */

    pByter = pALDB->header.pbEncodeTable;
    for (dwIndex = 0; dwIndex < ALDB_HEADER_MAX_DIRECT_ENCODE; ++dwIndex) {
        *pByter++ = (ET9U8)pALDB->header.wCodeIntervalEnd;
    }

    pALDB->header.wEncodeExtendCount = 0;
    pALDB->header.sEncodeExtendFirstChar = 0xFFFF;
    pALDB->header.sEncodeExtendLastChar = 0;

    pSymb = pALDB->header.psDecodeTable;
    for (dwIndex = 0; dwIndex < pALDB->header.wDecodeCount; ++dwIndex, pSymb++) {

        if (*pSymb == ALDB_INTERVAL_END_CHAR_VAL ||
            *pSymb == ALDB_INTERVAL_JUMP_CHAR_VAL ||
            *pSymb == ALDB_INTERVAL_EXTEND_CHAR_VAL) {

            continue;
        }

        if (*pSymb >= ALDB_HEADER_MAX_DIRECT_ENCODE) {

            ++pALDB->header.wEncodeExtendCount;

            if (pALDB->header.sEncodeExtendFirstChar > *pSymb) {
                pALDB->header.sEncodeExtendFirstChar = *pSymb;
            }
            if (pALDB->header.sEncodeExtendLastChar < *pSymb) {
                pALDB->header.sEncodeExtendLastChar = *pSymb;
            }

            continue;
        }

        pALDB->header.pbEncodeTable[*pSymb] = (ET9U8)dwIndex;
    }

    /* Const cursor info */

    pCursData = pALDB->pCursors;
    for (dwIndex = 0; dwIndex < pALDB->header.bPosCount; ++dwIndex, pCursData++) {

        pCursData->pbCacheEnd = &pCursData->pbCache[ALDB_CURSOR_DATA_CACHE_SIZE];
        pCursData->dwSourceDataStart = pALDB->header.pdwIntervalOffsets[dwIndex];
        pCursData->dwSourceDataLength = pALDB->header.pdwIntervalOffsets[dwIndex + 1] - pALDB->header.pdwIntervalOffsets[dwIndex];
    }

	return decumaNoError;
}

/* ********************************************************************  */

static void __ET9CompareStart (ET9ALdb * pALDB)
{      
	pALDB->compare.wLength = 0;
    pALDB->compare.bStemsAllowed = 1;

    pALDB->compare.bSpcCompare = 1;

    if (pALDB->compare.bSpcCompare) {
        pALDB->compare.wSpcMinLength = 0;
    }
	
	/* decumaMemset(&pALDB->compare.ppbKey, 0, sizeof(pALDB->compare.ppbKey)); */
	/* decumaMemset(&pALDB->compare.ppbExact, 0, sizeof(pALDB->compare.ppbKey)); */

    pALDB->compare.wCmpLength = 0;

} /* __ET9CompareStart */

/* ********************************************************************  */

static DECUMA_STATUS __ET9SearchStart (void const * const pRawData, long const dataSize, ET9ALdb * pALDB, ET9SYMB *psTarget, ET9U16 *pwLength, ET9U16 wTargetLength)
{
    ET9U8 bIndex;
    ET9U8 bOrderIndex;
    ET9ALdbCursorData *pCursData;
    ET9U8 *pByter;
	DECUMA_STATUS status;

	decumaAssert(pRawData == pTemp);

	pALDB->search.psTarget = psTarget;
    pALDB->search.pwLength = pwLength;
    pALDB->search.wTargetLength = wTargetLength;

    pALDB->search.bExhausted = (ET9U8)(wTargetLength < pALDB->header.bPosCount || pALDB->compare.wLength > pALDB->header.bPosCount);

    if (pALDB->compare.bSpcCompare) {
        pALDB->search.dwLengthEnd = (ET9U32)(~0);   /* no end */
    }
    else {
        pALDB->search.dwLengthEnd = pALDB->header.pdwLengthEnds[pALDB->compare.wLength-1];
    }

    pByter = pALDB->header.pbPosOrder;
    for (bIndex = 0, bOrderIndex = 0; bOrderIndex < pALDB->compare.wCmpLength; ++bIndex, ++pByter) {

        if (*pByter < pALDB->compare.wCmpLength) {

            pALDB->search.pbPosCurrOrder[bOrderIndex++] = *pByter;
        }
    }

    pALDB->search.dwCurrItem = 0;

    pCursData = pALDB->pCursors;
    for (bIndex = 0; bIndex < pALDB->header.bPosCount; ++bIndex, pCursData++) {

        pCursData->wCode = 0;
        pCursData->dwStartPos = 0;
        pCursData->dwLength = 0;
        pCursData->dwJumpPos = 0;
        pCursData->dwJumpAddress = 0;

        __LdbSetIndex(pCursData, pALDB->header.pdwIntervalOffsets[bIndex]);

        if (!__IsExhausted()) {
            status = __ET9SearchGetNextInterval(pRawData, dataSize, pALDB, bIndex, 0);
			if (status != decumaNoError)
				return status;
        }
    }

    if (!__IsExhausted()) {
        status = __ET9SearchMoveToItem(pRawData, dataSize, pALDB, 0);
		if (status != decumaNoError)
			return status;
    }
	return decumaNoError;

} /* __ET9SearchStart */

/* ********************************************************************  */

static DECUMA_STATUS __ET9SearchMoveToItem (void const * const pRawData, long const dataSize, ET9ALdb * pALDB, ET9U32 dwItem)
{
	ET9U8               bPos;
    ET9U8               bPosIndex = 0;
    ET9U8               bZeroPos = pALDB->header.bPosCount;
    ET9ALdbCursorData   *pCursor;
    const ET9U8         bSpcCompare = pALDB->compare.bSpcCompare;
    const ET9U16        wSpcMinLength = pALDB->compare.wSpcMinLength;
	DECUMA_STATUS status;
	
	decumaAssert(pRawData == pTemp);
	if (dwItem < pALDB->search.dwCurrItem)
	{
		return decumaInvalidDictionary;
	}

    pALDB->search.dwCurrItem = dwItem;

    /* Find a "stem" match */

    while (bPosIndex < pALDB->compare.wCmpLength) {

        bPos = __GetPosOrder(bPosIndex);

        pCursor = &pALDB->pCursors[bPos];

        /* Catch up on interval data */

        if (pCursor->dwStartPos + pCursor->dwLength - 1 < pALDB->search.dwCurrItem) {

            if (!__IsExhausted()) {
                status = __ET9SearchGetNextInterval(pRawData, dataSize, pALDB, bPos, pALDB->search.dwCurrItem);
				if (status != decumaNoError)
					return status;
            }
            if (__IsExhausted()) return decumaNoError;
        }

        /* If match continue with next pos, or if spc continue anyway.
           (During spc it will only fall through on a too short result.) */

        if (!pALDB->search.psTarget[bPos] && bPos < bZeroPos) {
            bZeroPos = bPos;
        }

        if (bSpcCompare && bZeroPos < wSpcMinLength) {
            /* quick skip of too short words */
        }
        else if (__IsCodeActive(bPos, pCursor->wCode) || bSpcCompare) {
            ++bPosIndex;
            continue;
        }

        /* No match, jump to next interval and restart the comparison run */

        if (bSpcCompare) {
            ++pALDB->search.dwCurrItem;
        }
        else {
            pALDB->search.dwCurrItem = pCursor->dwStartPos + pCursor->dwLength;
        }

        bPosIndex = 0;
        bZeroPos = pALDB->header.bPosCount;
    }

    /* Get non stem chars */

    bPos = (ET9U8)pALDB->compare.wCmpLength;
    pCursor = &pALDB->pCursors[bPos];

    while (bPos < pALDB->header.bPosCount && bZeroPos == pALDB->header.bPosCount) {

        /* Catch up on interval data */

        if (pCursor->dwStartPos + pCursor->dwLength - 1 < pALDB->search.dwCurrItem) {

            if (!__IsExhausted()) {
                status = __ET9SearchGetNextInterval(pRawData, dataSize, pALDB, bPos, pALDB->search.dwCurrItem);
				if (status!=decumaNoError) return status;
            }
            if (__IsExhausted()) {
                return decumaNoError;
            }
        }

        /* Check for EOS */

        if (!pALDB->search.psTarget[bPos]) {
            bZeroPos = bPos;
        }

        ++bPos;
        ++pCursor;
    }

    /* Length */

    *pALDB->search.pwLength = bZeroPos;
    return decumaNoError;

} /* __ET9SearchMoveToItem */

/* ********************************************************************  */

static DECUMA_STATUS __ET9SearchGetNextInterval (void const * const pRawData, long const dataSize, ET9ALdb * pALDB, ET9U8 bPos, ET9U32 dwTargetPos)
{
    ET9ALdbCursorData *pCursor = &pALDB->pCursors[bPos];
    ET9U8 bLoopNeeded = (ET9U8)!pCursor->dwLength;
    ET9U8 bCode;

	decumaAssert(pRawData == pTemp);

    while (bLoopNeeded || dwTargetPos >= pCursor->dwStartPos + pCursor->dwLength) {

        /* Move start pos */

        if (bLoopNeeded) {
            bLoopNeeded = 0;
        }
        else {
            pCursor->dwStartPos += pCursor->dwLength;
        }

        /* Check if jump is possible */

        if (dwTargetPos >= pCursor->dwJumpPos && pCursor->dwStartPos + 1 < pCursor->dwJumpPos) {

            pCursor->dwStartPos = pCursor->dwJumpPos;

            __LdbSetIndex(pCursor, pCursor->dwJumpAddress + pALDB->header.pdwIntervalOffsets[bPos]);

            bLoopNeeded = 1;

            continue;
        }

        /* Get interval code - could be single or multi encoded */

        bCode = __LdbGetByte(pCursor);
        __LdbIncIndex(pCursor);

        /* Check if single */

        if (bCode != ALDB_INTERVAL_MULTI_BYTE_CODE) {

            pCursor->wCode = pALDB->header.pbOneByteCodes[bCode];
            pCursor->dwLength = pALDB->header.pbOneByteLengths[bCode];

            continue;
        }

        /* Get multi code - could be jump, char with length or end */

        pCursor->wCode = __LdbGetByte(pCursor);
        __LdbIncIndex(pCursor);

        /* Check if end */

        if (pCursor->wCode == pALDB->header.wCodeIntervalEnd) {
            pALDB->search.bExhausted = 1;
            break;
        }

        /* Check if jump info */

        if (pCursor->wCode == pALDB->header.wCodeIntervalJump) {

            pCursor->dwJumpPos = pCursor->dwStartPos;

            pCursor->dwJumpPos += ((ET9U32)__LdbGetByte(pCursor)) << 8;
            __LdbIncIndex(pCursor);

            pCursor->dwJumpPos += __LdbGetByte(pCursor);
            __LdbIncIndex(pCursor);

            pCursor->dwJumpAddress = (pCursor->dwCurrCacheStart - pALDB->header.pdwIntervalOffsets[bPos]) + (pCursor->pbCurrData - pCursor->pbCache) - 4;

            pCursor->dwJumpAddress += ((ET9U32)__LdbGetByte(pCursor)) << 8;
            __LdbIncIndex(pCursor);

            pCursor->dwJumpAddress += __LdbGetByte(pCursor);
            __LdbIncIndex(pCursor);

            bLoopNeeded = 1;

            continue;
        }

        /* Direct interval... */

        pCursor->dwLength = __LdbGetByte(pCursor);
        __LdbIncIndex(pCursor);

        /* Check length extension */

        if (__LdbGetByte(pCursor) != ALDB_INTERVAL_MULTI_BYTE_CODE) {
            continue;
        }

        __LdbIncIndex(pCursor);

        if (__LdbGetByte(pCursor) != pALDB->header.wCodeIntervalExtend) {

            __LdbDecIndex(pCursor);

            continue;
        }

        /* Found extension */

        __LdbIncIndex(pCursor);

        pCursor->dwLength += ((ET9U32)__LdbGetByte(pCursor)) << 8;
        __LdbIncIndex(pCursor);
    }

    /* Pick up the actual symbol as well... */

    if (bPos < pALDB->search.wTargetLength) {

        pALDB->search.psTarget[bPos] = __CodeToSymbol(pCursor->wCode);
    }

	if (!__IsExhausted() && 
		!(pCursor->dwStartPos <= dwTargetPos && dwTargetPos < pCursor->dwStartPos + pCursor->dwLength))
	{
		return decumaInvalidDictionary;
	}
	return decumaNoError;

} /* __ET9SearchGetNextInterval */


/* ----------------------------------< eof >--------------------------------- */

typedef struct _XT9_UDB_CONVERSION_DATA
{
	DECUMA_UINT32 nExtractedWords;
	DECUMA_TRIE * pTrie;

} XT9_UDB_CONVERSION_DATA;

static ET9STATUS handleUDBWord(void *pOwner, const UDBWORDS_Word *pWord)
{
	DECUMA_STATUS status;
	XT9_UDB_CONVERSION_DATA * pData = (XT9_UDB_CONVERSION_DATA*) pOwner;

    switch (pWord->eType)
    {
        case UDBWORDS_WordType_RUDB:
			/*RUDB = reordering words */
            break;
        case UDBWORDS_WordType_UDB:
			/*UDB = custom words */
            break;
        case UDBWORDS_WordType_QUDB:
			/*QUDB = questionable reordering words (can be promoted later to RUDB words) */
            return ET9STATUS_NONE; /*We don't want to add questionable words */
        default:
            break;
    }

	pData->nExtractedWords++;
	
	/*NOTE: TODO: */
	/*We currently disregard the UDB information about language and frequency. */
	/*We add all the words to the dictionary regardless of language and we give all UDB  */
	/*words the highest rank (== 0)	 */
	status = decumaTrieCopyWord(pData->pTrie, pWord->psString, &pWord->psString[pWord->wLen], 
		0);

	switch (status)
	{
		case decumaNoError: 
			break;
		case decumaTooShortBuffer:
			return ET9STATUS_ERROR;
		default:
			decumaAssert(0);
	}
	
	/*decumaAssert(pData->nExtractedWords == decumaTrieGetNWords(pData->pTrie)); */

	return ET9STATUS_NONE;
}


/* ******************************************************************** */ 

DECUMA_STATUS decumaXT9UnpackToTrie(DECUMA_TRIE ** ppTrie,
	void const * pRawData, DECUMA_UINT32 dataSize,
	DECUMA_XT9_DICTIONARY_TYPE type, const DECUMA_MEM_FUNCTIONS * pMemFunctions)
{
	DECUMA_STATUS status=decumaNoError;
	DECUMA_TRIE * pCompactTrie=NULL;
	long nExtractedWords = 0;
	ET9SYMB symbols[ET9MAXWORDSIZE];
	ET9U16 nSymbols;
	ET9ALdb	ALDB;
	ET9ALdb	* const pALDB = &ALDB;
	DECUMA_UINT32 minWordRankForFreqClass[7];
	DECUMA_UINT8 nFreqClasses=0;

	decumaMemset(pALDB, 0, sizeof(ET9ALdb));

#if defined(DECUMA_ASSERT_ENABLE) || defined(DECUMA_ASSERT_OVERRIDE)
	pTemp = pRawData;
#endif

	decumaAssert(ppTrie);
	decumaAssert(pRawData);
	decumaAssert(dataSize>0);
	decumaAssert(type >= 0 && type<numberOfXT9DictionaryTypes);

	*ppTrie=NULL;

	/* Table that creates a "FreqClass" from a word index */
	switch (type)
	{
		case decumaXT9LDB:
			/*TODO: How should this be set up for a generic dictionary?? */
			/*      The current numbers are taken from the current generation of ENUK dictionary from XT9 to old Decuma Format */
			minWordRankForFreqClass[0]=0;
			minWordRankForFreqClass[1]=556;
			minWordRankForFreqClass[2]=1674;
			minWordRankForFreqClass[3]=4930;
			minWordRankForFreqClass[4]=14948;
			minWordRankForFreqClass[5]=40001;  /*Note, currently only 31327 words in ENUK dictionary */
			minWordRankForFreqClass[6]=100001; /*Note, currently only 31327 words in ENUK dictionary */
			nFreqClasses = 7;
			break;
		case decumaXT9UDB:
			/*So far we only use one frequency class for UDB words */
			minWordRankForFreqClass[0]=0;
			minWordRankForFreqClass[1]=MAX_DECUMA_UINT32;
			nFreqClasses = 1;
			break;
		default:
			decumaAssert(0);
	}


	/* Create empty trie */
	status = decumaTrieCreate(ppTrie, NODE_GRANULARITY,
		minWordRankForFreqClass, nFreqClasses,
		pMemFunctions);

	if (status != decumaNoError)
		goto decumaXT9UnpackToTrie_error;

	decumaAssert(*ppTrie);

	switch (type)
	{
		case decumaXT9LDB:
			/* Initialize (and verify) the given LDB */
			status = readStandardHeader(pRawData, dataSize);
			if (status != decumaNoError)
				goto decumaXT9UnpackToTrie_error;

			status = readSpecific(pRawData, dataSize, pALDB, 0x41);
			if (status != decumaNoError)
				goto decumaXT9UnpackToTrie_error;

			/* Enumerate all words and add to trie */
			status = __ET9SearchStart(pRawData, dataSize, pALDB, symbols, &nSymbols, ET9MAXLDBWORDSIZE);
			if (status != decumaNoError)
				goto decumaXT9UnpackToTrie_error;


#ifdef PRINT_WORDS
			error = _tfopen_s(&g_fp,_T("tmp21.txt"),_T("w+, ccs=UTF-16LE"));
#endif

			for (; !__IsExhausted(); status=__ET9SearchGetNext()) 
			{
#ifdef PRINT_WORDS
				wchar_t wbuf[100];
				int i;
#endif /*PRINT_WORDS */

				if (status != decumaNoError)
					goto decumaXT9UnpackToTrie_error;

				/*The words are received in a ranked order from the ET9Search */
				
#ifdef PRINT_WORDS
				for (i=0; i<nSymbols; i++)
				{
					wbuf[i]=symbols[i];
				}
				wbuf[nSymbols]=0;
				_ftprintf (g_fp,_T("%s\n"), wbuf);
				/*printf("%S\n", buf); */
				fflush(g_fp);
#endif /*PRINT_WORDS */

				/* TODO: If nExtractedWords > [some treshold] start a new trie, just to see effects on size of using the "old" pardigm with one graph per frequency class */
				nExtractedWords++;
#ifdef PRINT_WORDS	
				{
					int nWordsBefore = decumaTrieGetNWords(*ppTrie);
#endif /*PRINT_WORDS */
				status = decumaTrieCopyWord(*ppTrie, &symbols[0], 
					&symbols[nSymbols], nExtractedWords);  /*Note that nExtractedWords is the index */
				
				if (status == decumaTooShortBuffer)
				{
					/*We have filled the trie and we have not already retried to enlarge it by adding node buffers */
					status = decumaTrieAddNodeBuffer(*ppTrie, pMemFunctions);
					if (status != decumaNoError)
						goto decumaXT9UnpackToTrie_error;

					status = decumaTrieCopyWord(*ppTrie, &symbols[0], 
						&symbols[nSymbols], nExtractedWords);  /*Retry after realloc						 */

					decumaAssert(status == decumaNoError); /*Now, it should succeed */
					if (status != decumaNoError)
						goto decumaXT9UnpackToTrie_error;
				}
				decumaAssert(status == decumaNoError);
				/*Commented below line since we might extract douplettes e.g. for danish and norwegian XT9 LDBs */
				/*decumaAssert(nExtractedWords == decumaTrieGetNWords(*ppTrie));  */
#ifdef PRINT_WORDS	
					if (decumaTrieGetNWords(*ppTrie) == nWordsBefore)
					{
						fprintf(stderr, "Douplette found. Word nr %d\n",nWordsBefore);
						_ftprintf (stderr,_T("%s\n"), wbuf);
					}
				}
#endif /*PRINT_WORDS */
			}
			break;
		case decumaXT9UDB:
			{
				XT9_UDB_CONVERSION_DATA data;
				ET9STATUS et9status;

				data.nExtractedWords=0;
				data.pTrie = *ppTrie;

				et9status = UDBWORDS_GetWords((void*)pRawData, dataSize, &data, handleUDBWord);
				switch (et9status)
				{
					case ET9STATUS_BAD_PARAM: 
						status=decumaNullPointer; 
						goto decumaXT9UnpackToTrie_error;
						break;
					case ET9STATUS_ERROR: 
						status=decumaTooShortBuffer; 
						goto decumaXT9UnpackToTrie_error;
						break;
					case ET9STATUS_CORRUPT_DB:
						status = decumaInvalidDictionary;
						goto decumaXT9UnpackToTrie_error;
						break;
					case ET9STATUS_NONE: 
						status=decumaNoError; 
						break;
					default: 
						decumaAssert(0); /*We don't know of any other possible return values */
				}
			}
			break;
		default:
			decumaAssert(0); /*Unknown type */
	}

	if ( status != decumaNoError ) goto decumaXT9UnpackToTrie_error;

/*	status = decumaTrieReallocLastNodeBuffer(ppTrie, pMemFunctions); //Removing unnessecary empty buffer end */
/*	if ( status != decumaNoError ) goto decumaXT9UnpackToTrie_error; */

	return status;

decumaXT9UnpackToTrie_error:
	
	decumaTrieDestroy(ppTrie,pMemFunctions);
	decumaTrieDestroy(&pCompactTrie,pMemFunctions);
	decumaAssert(*ppTrie==0);
	return status;
}

