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
;**                                                                           **
;*******************************************************************************
;**                                                                           **
;**     FileName: et9imkdb.h                                                  **
;**                                                                           **
;**  Description: KDB input module for ET9.                                   **
;**                                                                           **
;*******************************************************************************
;******************************************************************************/

#ifndef ET9IMKDB_H
#define ET9IMKDB_H 1


#include "et9api.h"
#ifdef ET9_KDB_MODULE

/* don't mangle the function name if compile under C++ */
#if defined(__cplusplus)
extern "C" {
#endif


#define ET9KDBOFFSET_LAYOUTVER          0x20
#define ET9KDBOFFSET_DATABASETYPE       0x21
#define ET9KDBOFFSET_COMPATID           0x22
#define ET9KDBOFFSET_OEMID              0x24
#define ET9KDBOFFSET_CHECKSUM           0x26
#define ET9KDBOFFSET_CONTENTSMAJORVER   0x28
#define ET9KDBOFFSET_CONTENTSMINORVER   0x29
#define ET9KDBOFFSET_SYMBOLCLASS        0x2B
#define ET9KDBOFFSET_PRIMARYKEYBOARDID  0x2C
#define ET9KDBOFFSET_SECONDKEYBOARDID   0x2D
#define ET9KDBOFFSET_TOTALPAGES         0x2E
#define ET9KDBOFFSET_KDBWIDTH           0x30
#define ET9KDBOFFSET_KDBHEIGHT          0x32
#define ET9KDBOFFSET_PAGEARRAYOFFSET    0x34

#define ET9KDB_PAGEHDROFFSET_LEFT            0x00
#define ET9KDB_PAGEHDROFFSET_TOP             0x02
#define ET9KDB_PAGEHDROFFSET_RIGHT           0x04
#define ET9KDB_PAGEHDROFFSET_BOTTOM          0x06
#define ET9KDB_PAGEHDROFFSET_TOTALREGIONS    0x08
#define ET9KDB_PAGEHDROFFSET_REGIONOFFSET    0x09

#define ET9KDB_REGHDROFFSET_LEFT             0x00
#define ET9KDB_REGHDROFFSET_TOP              0x02
#define ET9KDB_REGHDROFFSET_RIGHT            0x04
#define ET9KDB_REGHDROFFSET_BOTTOM           0x06
#define ET9KDB_REGHDROFFSET_TOTALREGIONS     0x08
#define ET9KDB_REGHDROFFSET_REGIONOFFSET     0x09

#define ET9KDB_REGIONOFFSET_AMBIGFLAG        0x00
#define ET9KDB_REGIONOFFSET_LEFT             0x01
#define ET9KDB_REGIONOFFSET_TOP              0x03
#define ET9KDB_REGIONOFFSET_RIGHT            0x05
#define ET9KDB_REGIONOFFSET_BOTTOM           0x07
#define ET9KDB_REGIONOFFSET_TOTALKEYS        0x09
#define ET9KDB_REGIONOFFSET_TOTALBLOCKROWS   0x0A
#define ET9KDB_REGIONOFFSET_TOTALBLOCKCOLS   0x0B
#define ET9KDB_REGIONOFFSET_BLOCKWIDTH       0x0C
#define ET9KDB_REGIONOFFSET_BLOCKHEIGHT      0x0E

#define ET9KDBAOFFSET_KEYOFFSET         0x10
#define ET9KDBAOFFSET_BLOCKOFFSET       0x12

#define ET9KDBNAOFFSET_BLOCKOFFSET      0x10
#define ET9KDBNAOFFSET_KEYOFFSET        0x12

#define ET9KDB_DISKEY_LEFT              0x00
#define ET9KDB_DISKEY_TOP               0x02
#define ET9KDB_DISKEY_RIGHT             0x04
#define ET9KDB_DISKEY_BOTTOM            0x06
#define ET9KDB_DISKEY_TYPE              0x08
#define ET9KDB_DISKEY_TOTCHARS          0x09
#define ET9KDB_DISKEY_DEFAULTINDEX      0x0A
#define ET9KDB_DISKEY_CHARS             0x0B

#define ET9KDB_REGKEY_TYPE              0x00
#define ET9KDB_REGKEY_TOTCHARS          0x01
#define ET9KDB_REGKEY_CHARS             0x02

#define ET9KDB_DISBLOCK_NUMBAMBIGKEYS   0x00

#define ET9KDB_REGBLOCK_NUMBTOTALKEYS   0x00
#define ET9KDB_REGBLOCK_KEYS            0x01

#define ET9KDB_REGBLOCKKEY_KEYINDEX     0x00
#define ET9KDB_REGBLOCKKEY_KEYPROB      0x01

#define ET9HASHKDBBUFFSIZE               32

/*------------------------------------------------------------------
 * Functions
 *------------------------------------------------------------------*/

#ifdef ET9_DIRECT_KDB_ACCESS

#define __ET9KDBGETDATA(dwOffset, dwNumberOfBytesToRead, pbDest)                                    \
    {                                                                                               \
        _ET9ByteCopy(pbDest, &pKDBInfo->Private.pKdbData[dwOffset], dwNumberOfBytesToRead);         \
    }                                                                                               \

#define __ET9KDBREADBYTE(dwOffset, pbData)                                                          \
    {                                                                                               \
        *(pbData) = pKDBInfo->Private.pKdbData[dwOffset];                                           \
    }                                                                                               \

#define __ET9KDBREADWORD(dwOffset, pwData)                                                          \
    {                                                                                               \
        *(pwData) = (ET9U16)((pKDBInfo->Private.pKdbData[(dwOffset) + 0] << 8) |                    \
                              pKDBInfo->Private.pKdbData[(dwOffset) + 1]);                          \
    }                                                                                               \

#define __ET9KDBREADDWORD(dwOffset, pwData)                                                         \
    {                                                                                               \
        *(pwData) = (ET9U16)((pKDBInfo->Private.pKdbData[(dwOffset) + 0] << 24) |                   \
                             (pKDBInfo->Private.pKdbData[(dwOffset) + 1] << 16) |                   \
                             (pKDBInfo->Private.pKdbData[(dwOffset) + 2] << 8)  |                   \
                              pKDBInfo->Private.pKdbData[(dwOffset) + 3]);                          \
    }

#define __InitDirectKDBAccess(pX)   ((pX)->ET9KDBReadData((pX), &(pX)->Private.pKdbData, &(pX)->Private.dwKdbDataSize))

#else /* ET9_DIRECT_KDB_ACCESS */

#define __ET9KDBGETDATA(dwOffset, dwNumberOfBytesToRead, pbDest)                                    \
    {                                                                                               \
        ET9STATUS wStatus;                                                                          \
        ET9U32 dwNumberOfBytesRead;                                                                 \
        wStatus = pKDBInfo->ET9KDBReadData(                                                         \
            pKDBInfo,                                                                               \
            dwOffset,                                                                               \
            dwNumberOfBytesToRead,                                                                  \
            (ET9U8*)pbDest,                                                                         \
            &dwNumberOfBytesRead);                                                                  \
        if (wStatus || (dwNumberOfBytesRead != dwNumberOfBytesToRead)) {                            \
            return ET9STATUS_READ_DB_FAIL;                                                          \
        }                                                                                           \
    }                                                                                               \

#define __ET9KDBREADBYTE(dwOffset, pbData)                                                          \
    {                                                                                               \
        __ET9KDBGETDATA(dwOffset, 1, pbData);                                                       \
    }                                                                                               \

#define __ET9KDBREADWORD(dwOffset, pwData)                                                          \
    {                                                                                               \
        ET9U8 byWord[2];                                                                            \
        __ET9KDBGETDATA(dwOffset, 2, byWord);                                                       \
        *pwData = (ET9U16)((byWord[0] << 8) | byWord[1]);                                           \
    }                                                                                               \

#define __ET9KDBREADDWORD(dwOffset, pwData)                                                         \
    {                                                                                               \
        ET9U8 byWord[4];                                                                            \
        __ET9KDBGETDATA(dwOffset, 4, byWord);                                                       \
        *pwData = (ET9U32)((byWord[0] << 24) | (byWord[1] << 16) | (byWord[2] << 8) | byWord[3]);   \
    }                                                                                               \

#define __InitDirectKDBAccess(pX)   (ET9STATUS_NONE)

#endif /* ET9_DIRECT_KDB_ACCESS */


/** \internal
 * Structure holding trace point info for dbg.
 */

typedef struct ET9TracePointDbg_s {
    ET9UINT         nX;                                 /**< \internal X coordinate. */
    ET9UINT         nY;                                 /**< \internal Y coordinate. */
    ET9UINT         nQuality;                           /**< \internal Point quality */
    ET9BOOL         bMergeWithPrev;                     /**< \internal Marked for merge with previous */
} ET9TracePointDbg;                                     /**< \internal */

#define _ET9KDBSecondKDBSupported(pKDBInfo)  ((((pKDBInfo) != NULL) &&  (pKDBInfo)->wSecondKdbNum) ? ((((pKDBInfo)->wSecondKdbNum & ET9PLIDMASK) != ET9PLIDNone) && (((pKDBInfo)->wSecondKdbNum & ET9PLIDMASK) != ET9PLIDNull)) : 0)

#define __ET9KDBREADET9SYMB(dwOffset, psData)   __ET9KDBREADWORD(dwOffset, psData);


ET9STATUS ET9FARCALL _ET9KDB_GetLastTrace(ET9KDBInfo             * const pKDBInfo,
                                          ET9WordSymbInfo        * const pWordSymbInfo,
                                          ET9TracePoint          * const pPoints,
                                          const ET9UINT                  nMaxPointCount,
                                          ET9UINT                * const pnPointCount);

ET9STATUS ET9FARCALL _ET9KDB_GetLastTraceDbg(ET9KDBInfo             * const pKDBInfo,
                                             ET9TracePointDbg       * const pPoints,
                                             const ET9UINT                  nMaxPointCount,
                                             ET9UINT                * const pnPointCount);

ET9STATUS ET9FARCALL _ET9KDB_FindSymbol(ET9KDBInfo          * const pKDBInfo,
                                        const ET9SYMB               sSymbol,
                                        const ET9U16                wKdbNum,
                                        const ET9U16                wPageNum,
                                        ET9U8               * const pbyRegionalKey,
                                        ET9U16              * const pwKeyIndex,
                                        ET9Region           * const pKeyRegion,
                                        const ET9BOOL               bInitialSymCheck);

ET9STATUS ET9FARCALL ET9KDB_SetLocale(ET9KDBInfo        * const pKDBInfo,
                                      ET9WordSymbInfo   * const pWordSymbInfo,
                                      const ET9U16              wLocale);

ET9STATUS ET9FARCALL ET9KDB_GetLocale(ET9KDBInfo        * const pKDBInfo,
                                      ET9WordSymbInfo   * const pWordSymbInfo,
                                      ET9U16            * const pwLocale);


/* End don't mangle the function name if compile under C++ */
#if defined (__cplusplus)
    }
#endif

#endif /* ET9_KDB_MODULE */

#endif /* ET9IMKDB_H */

/* ----------------------------------< eof >--------------------------------- */

