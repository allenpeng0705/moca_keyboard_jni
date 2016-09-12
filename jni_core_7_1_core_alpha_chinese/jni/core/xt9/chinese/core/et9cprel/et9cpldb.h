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
;**     FileName: et9cpldb.h                                                  **
;**                                                                           **
;**  Description: Chinese XT9 database access routines header file.           **
;**               Conforming to the development version of Chinese XT9.       **
;**                                                                           **
;*******************************************************************************
;******************************************************************************/

#ifndef ET9CPLDB_H
#define ET9CPLDB_H 1

/* don't mangle the function name if compile under C++ */
#if defined(__cplusplus)
extern "C" {
#endif

#define     ET9_CP_NOMATCH             ((ET9U16)0xFFFF)

/* PID range macros */
/* PIDs are split into four ranges in this order:
 *      Normal    - Chinese characters with stroke order and reading
 *      Symbol    - Punctuation/symbols (no stroke order or reading)
 *      Mute      - Characters with strokes but no reading
 *      Component - Nuance's components, strokes but no reading
 */
#define ET9_CP_IS_NORMAL_PID(psCommon, wPID) ((wPID) < (psCommon)->w1stSymbolPID)
#define ET9_CP_IS_SYMBOL_PID(psCommon, wPID) ((wPID) >= (psCommon)->w1stSymbolPID && (wPID) < (psCommon)->w1stMuteCharPID)
#define ET9_CP_IS_MUTE_PID(psCommon, wPID) ((wPID) >= (psCommon)->w1stMuteCharPID && (wPID) < (psCommon)->w1stComponentPID)
#define ET9_CP_IS_COMP_PID(psCommon, wPID) ((wPID) >= (psCommon)->w1stComponentPID)

#define ET9_CP_NORMAL_PID_COUNT(psCommon) ((psCommon)->w1stSymbolPID)
#define ET9_CP_MUTE_PID_COUNT(psCommon) ((psCommon)->w1stComponentPID - (psCommon)->w1stMuteCharPID)

#define ET9_CP_SID_RANGE_SIZE   3
#define ET9_CP_PART_ID_RANGE_NUM   3
#define ET9_CP_EXACT_ID_RANGE_NUM  2

enum ET9_CP_Lookup_e {
    ET9_CP_Lookup_PIDToSID = 0,
    ET9_CP_Lookup_SIDToPID
};

ET9U8 ET9FARCALL ET9_CP_FreqLookup(ET9CPLingInfo * pET9CPLingInfo,
                                  ET9U16 wPID);
ET9U8 ET9FARCALL ET9_CP_LookupTone(ET9CPLingInfo *pET9CPLingInfo,
                                  ET9U16 wPID);

/*----------------------------------------------------------------------------
 *  Define the LDB parameters and constants.
 *----------------------------------------------------------------------------*/
#define ET9_CP_DBTYPE               10

#define ET9_CP_MAX_NUM_MT_SYM       8        /* maximum number of multitap symbol */
/* Define XT9 compatibility index for Chinese XT9 core and databases.
   This number should be updated for each release. */
#define ET9_CP_COMPAT_ID_BASE           5
#define ET9_CP_COMPAT_ID_OFFSET         0
#define ET9_CP_MAX_COMPAT_ID_OFFSET     16

#define ET9_CP_MAX_PARTIAL_SYL     37  /* max number of partial syllable */
/*----------------------------------------------------------------------------
 *  Define the LDB structure and variable types.
 *----------------------------------------------------------------------------*/

typedef struct ET9_CP_VersionInfo_s { /* Structure for storing version info. */
    ET9U8    bLDBLayoutVer;      /* database layout version */
    ET9U8    bLDBType;           /* database type */
    ET9U8    bPrimID;            /* primary language id */
    ET9U8    bSecID;             /* secondary language id */
    ET9U8    bSymClass;          /* symbol class */
    ET9U8    bContMajVer;        /* content major version */
    ET9U8    bContMinVer;        /* content minor version */
    ET9U8    bContDev;           /* content deviation */
    ET9U8    bModuleCharSet;     /* module and character set */
} ET9_CP_VersionInfo;

enum ET9_CP_MOHU_RULES_e {
    ET9_CP_MOHU_NO_RULE = 0,
    ET9_CP_MOHU_Appendh,
    ET9_CP_MOHU_Appendg,
    ET9_CP_MOHU_Droph,
    ET9_CP_MOHU_Dropg,
    ET9_CP_MOHU_ChangeToR,
    ET9_CP_MOHU_ChangeToL,
    ET9_CP_MOHU_ChangeToN,
    ET9_CP_MOHU_ChangeToF,
    ET9_CP_MOHU_ChangeToH,
    ET9_CP_MOHU_NUMRULES
};

#define ET9_CP_MOHU_AppendhMask   (1 << (ET9_CP_MOHU_Appendh - 1))
#define ET9_CP_MOHU_AppendgMask   (1 << (ET9_CP_MOHU_Appendg - 1))
#define ET9_CP_MOHU_DrophMask     (1 << (ET9_CP_MOHU_Droph - 1))
#define ET9_CP_MOHU_DropgMask     (1 << (ET9_CP_MOHU_Dropg - 1))
#define ET9_CP_MOHU_ChangeToRMask (1 << (ET9_CP_MOHU_ChangeToR - 1))
#define ET9_CP_MOHU_ChangeToLMask (1 << (ET9_CP_MOHU_ChangeToL - 1))
#define ET9_CP_MOHU_ChangeToNMask (1 << (ET9_CP_MOHU_ChangeToN - 1))
#define ET9_CP_MOHU_ChangeToFMask (1 << (ET9_CP_MOHU_ChangeToF - 1))
#define ET9_CP_MOHU_ChangeToHMask (1 << (ET9_CP_MOHU_ChangeToH - 1))

typedef struct ET9_CP_MOHU_KEYS_s {
    ET9U8 bKeyN;
    ET9U8 bKeyL;
    ET9U8 bKeyR;
    ET9U8 bKeyF;
    ET9U8 bKeyH;
    ET9U8 bKeyg;
} ET9_CP_MOHU_KEYS;

#define ET9_CP_GetMohuFlags(pLing)  (ET9U16)( (ET9PLIDChineseSimplified == (pLing)->wLdbNum) ? (pLing)->CommonInfo.wMohuFlags: 0 )

/* Ldb read functions prototype */

#ifdef ET9_DIRECT_LDB_ACCESS
ET9STATUS ET9FARCALL ET9_CP_ReadLdbMultiBytes(ET9CPLingInfo * const     pLingInfo,
                                              ET9U32                    dwOffset,
                                              ET9U32                    dwBytesToRead,
                                              const ET9U8 ET9FARDATA ** ppbDst);
#else
ET9STATUS ET9FARCALL ET9_CP_ReadLdbMultiBytes(ET9CPLingInfo * const     pLingInfo,
                                              ET9U32                    dwOffset,
                                              ET9U32                    dwBytesToRead,
                                              ET9U8         * const     pbDst);
#endif /* NOT ET9_DIRECT_LDB_ACCESS */

ET9U8 ET9FARCALL ET9_CP_LdbReadByte(ET9CPLingInfo * const pLingInfo,
                                    ET9U32                dwOffset);

ET9U16 ET9FARCALL ET9_CP_LdbReadWord(ET9CPLingInfo * const pLingInfo,
                                     ET9U32                dwOffset);

ET9U32 ET9FARCALL ET9_CP_LdbReadDWord(ET9CPLingInfo * const pLingInfo,
                                      ET9U32                dwOffset);

#ifdef ET9_DIRECT_LDB_ACCESS
ET9STATUS ET9FARCALL ET9_CP_InitDirectLdbAccess(ET9CPLingInfo * const pLingInfo);
#endif

/* Macros to verify LDB content */
#define ET9_CP_LdbHasStroke(pET9CPLingInfo)       (ET9BOOL)( (pET9CPLingInfo)->CommonInfo.sOffsets.dwStrokeLDBOffset != 0)
#define ET9_CP_LdbHasPinyin(pET9CPLingInfo)       (ET9BOOL)( (pET9CPLingInfo)->CommonInfo.sOffsets.dwPinyinLDBOffset != 0)
#define ET9_CP_LdbHasBpmf(pET9CPLingInfo)         (ET9BOOL)( (pET9CPLingInfo)->CommonInfo.sOffsets.dwBpmfLDBOffset != 0)
#define ET9_CP_LdbHasPhonetic(pET9CPLingInfo)     (ET9BOOL)(ET9_CP_LdbHasPinyin(pET9CPLingInfo) || ET9_CP_LdbHasBpmf(pET9CPLingInfo))

#define ET9_CP_IsLdbPinyinOnly(pET9CPLingInfo)    (ET9BOOL)(ET9_CP_LdbHasPinyin(pET9CPLingInfo) && !ET9_CP_LdbHasBpmf(pET9CPLingInfo))
#define ET9_CP_IsLdbBpmfOnly(pET9CPLingInfo)      (ET9BOOL)(!ET9_CP_LdbHasPinyin(pET9CPLingInfo) && ET9_CP_LdbHasBpmf(pET9CPLingInfo))
#define ET9_CP_IsLdbDualPhonetic(pET9CPLingInfo)  (ET9BOOL)(ET9_CP_LdbHasPinyin(pET9CPLingInfo) && ET9_CP_LdbHasBpmf(pET9CPLingInfo))

/*----------------------------------------------------------------------------
 *  Define the LDB function prototypes.
 *----------------------------------------------------------------------------*/
ET9STATUS ET9FARCALL ET9_CP_ReadLdbVersion(ET9CPLingInfo * const pLingInfo, ET9_CP_VersionInfo *sBuff);
ET9STATUS ET9FARCALL ET9_CP_GetLdbVersion(ET9CPLingInfo * const pLingInfo,
                                          ET9U8 * pbDest);
void ET9FARCALL ET9_CP_WriteUdbData(ET9CPLingInfo *pLingInfo,
                                    void ET9FARDATA *pTo,
                                    const void ET9FARDATA *pFrom,
                                    ET9UINT nSize);
ET9STATUS ET9FARCALL ET9_CP_CheckLdb(ET9CPLingInfo *);
ET9U16 ET9FARCALL ET9_CP_UnicodeToPID(ET9CPLingInfo *, ET9U16, ET9U8);
ET9SYMB ET9FARCALL ET9_CP_LookupUnicode(ET9CPLingInfo * pET9CPLingInfo, ET9U16 wPID);
ET9U8 ET9FARCALL ET9_CP_LookupID(ET9CPLingInfo *pET9CPLingInfo, ET9U16 *pwIDBuf, ET9U16 wID, ET9U8 bBufSize, ET9U8 bType);
ET9BOOL ET9FARCALL ET9_CP_Is_Comp_Sid(ET9CPLingInfo * pLing, ET9U16 wSid);
ET9BOOL ET9FARCALL ET9_CP_PidBidToSyllable(ET9CPLingInfo * pET9CPLingInfo,
                                           ET9U16 wPID,
                                           ET9U8 *pbSpell,
                                           ET9U8 *pbSpellLen,
                                           ET9BOOL bIsBpmfOutput);
void ET9FARCALL ET9_CP_PidBidToSpelling(ET9CPLingInfo *pET9CPLingInfo, ET9SYMB *pwID, ET9U8 bIDLen, ET9_CP_Spell *psSpell);
void ET9FARCALL ET9_CP_PidBidToNativeSpelling(ET9CPLingInfo *pET9CPLingInfo, ET9SYMB *pwID, ET9U8 bIDLen, ET9_CP_Spell *psSpell);
ET9U32 ET9FARCALL ET9_CP_PinyinSyllableToBIN(ET9U8 *pbSpell, ET9U8 bSpellLen, ET9U32 *pdwBIN);

ET9BOOL ET9FARCALL ET9_CP_SyllableToPidRange(ET9CPLingInfo * pET9CPLingInfo,
                                             ET9U8 *          pbSpell,
                                             ET9U8            bSpellLen,
                                             ET9BOOL          bPartial,
                                             ET9U16 *         pwStartPID,
                                             ET9U16 *         pwPartStartPID,
                                             ET9U16 *         pwPartEndPID);

ET9U8 ET9FARCALL ET9_CP_SearchSylbFromTable(ET9CPLingInfo * pET9CPLingInfo,
                                            ET9BOOL         bIsBpmf,
                                            ET9BOOL         bPartial,
                                            ET9U32          dwCharBIN,
                                            ET9U32          dwMask,
                                            ET9U16 *        pwStartIndex,
                                            ET9U16 *        pwEndIndex);
ET9STATUS ET9FARCALL ET9_CP_BuildPhrase(ET9CPLingInfo *, ET9CPPhrase *, ET9CPSpell *, ET9U8, ET9U8, ET9U8);
ET9U8 ET9FARCALL ET9_CP_PIDSearchLdb(ET9CPLingInfo *, ET9U16 *, ET9U8);

ET9U8 ET9FARCALL ET9_CP_SpellingToPidRanges(ET9CPLingInfo * pLingInfo,
                                            ET9U8 *pbSpell,
                                            ET9UINT nSpellSize);

ET9BOOL ET9FARCALL ET9_CP_WSIToJianpinPidRanges(ET9CPLingInfo * pLingInfo);

ET9BOOL ET9FARCALL ET9_CP_UniPhraseToPidRanges(ET9CPLingInfo *pLing,
                                               const ET9CPPhrase *pPhrase);


/* BPMF mode LDB function prototypes */
ET9U8 ET9FARCALL ET9_CP_BINToBpmfSyllable(ET9U32             dwBIN,
                                          ET9U8             *pbSyl);

ET9U32 ET9FARCALL ET9_CP_BpmfSyllableToBIN(ET9U8            *pbSpell,
                                           ET9U8             bSpellLen,
                                           ET9U32           *pdwBIN);

ET9U32 ET9FARCALL ET9_CP_BpmfSylbIndexToBIN(ET9CPLingInfo   *pET9CPLingInfo,
                                            ET9U16           wIndex);

ET9BOOL ET9FARCALL ET9_CP_GetSyllableIndex(ET9CPLingInfo      *pET9CPLingInfo,
                                           ET9U8              *pbSpell,
                                           ET9U8               bSpellLen,
                                           ET9BOOL             bPartial,
                                           ET9U16             *pwExactSylIndex,
                                           ET9U8              *pbExactSylIndexRange,
                                           ET9U16             *pwPartSylIndex,
                                           ET9U8              *pbPartSylIndexRange);

ET9BOOL ET9FARCALL ET9_CP_SylbToBIDRanges(ET9CPLingInfo     *pET9CPLingInfo,
                                          ET9U8             *pbSpell,
                                          ET9U8             bSpellLen,
                                          ET9BOOL           bSylComplete,
                                          ET9U16            **ppwRange,
                                          ET9U8             **ppbRangeEnd);

void ET9FARCALL ET9_CP_LoadSylTable(ET9CPLingInfo   *pET9CPLingInfo,
                                    ET9U32          *pdwPinyinBinTable,
                                    ET9U32          *pdwBpmfBinTable,
                                    ET9U16          *pwSylPIDTable);

ET9BOOL ET9FARCALL ET9_CP_UniPhraseToAltPID(ET9CPLingInfo *pLing,
                                            const ET9CPPhrase *pPhrase,
                                            ET9U16 *pwPID,
                                            const ET9U8 bMaxCol);

/*----------------------------------------------------------------------------
 *  Define Chinese XT9 LDB header field offsets
 *----------------------------------------------------------------------------*/
typedef enum {
    ET9_CP_LAYOUT_VER_OFFSET        = 0x20, /* Offsets to LDB layout version        */
    ET9_CP_DB_TYPE_OFFSET           = 0x21, /* Offset of database type              */
    ET9_CP_CORE_COMPAT_ID_OFFSET    = 0x22, /* Offset to Core compatible number     */
    ET9_CP_OEMID_OFFSET             = 0x24, /* Offset to OEM ID                     */
    ET9_CP_CHECKSUM_OFFSET          = 0x26, /* Offset to checksum                   */
    ET9_CP_LANGUAGE_ID_OFFSET       = 0x28, /* Offset to primary language ID        */
    ET9_CP_CONTENT_VER_OFFSET       = 0x2A, /* Offset to start of contents version  */
    ET9_CP_MODULE_CHARSET_OFFSET    = 0x2D, /* Offset to module and character set flags  */
    ET9_CP_SYMBOL_BYTE_OFFSET       = 0x2E, /* Offset to symbol bytes               */
    ET9_CP_HEADER_SIZE_OFFSET       = 0x30, /* Offset to Ldb header size in bytes   */
    ET9_CP_BPMF_MOD_OFFSET          = 0x32, /* Offset to BPMF module                */
    ET9_CP_PINYIN_MOD_OFFSET        = 0x36, /* Offset to Pinyin module              */
    ET9_CP_PHRASE_MOD_OFFSET        = 0x3A, /* offset to the phrase module          */
    ET9_CP_STROKE_MOD_OFFSET        = 0x3E, /* Offset to stroke LDB module          */
    ET9_CP_PID_TO_SID_OFFSET        = 0x42, /* offset to the PID to SID lookup table */
    ET9_CP_SID_TO_PID_OFFSET        = 0x46, /* offset to the SID to PID lookup table */
    ET9_CP_PID_TO_UNI_OFFSET        = 0x4A, /* offset to the PID to Unicode lookup table */
    ET9_CP_UNI_SORTED_PID_OFFSET    = 0x4E, /* offset to the Unicode-sorted PID table */
    ET9_CP_PID_TO_FREQ_OFFSET       = 0x52, /* offset to the PID to frequency lookup table */
    ET9_CP_C_COMMONCHAR_OFFSET      = 0x56, /* offset to the Context Common character table  */
    ET9_CP_NC_COMMONCHAR_OFFSET     = 0x5A, /* offset to the Non-Context Common character table  */
    ET9_CP_SMART_PUNCT_OFFSET       = 0x5E, /* offset to the Smart punct bigram table */
    ET9_CP_NAME_CHAR_OFFSET         = 0x62, /* offset to the Name character table    */
    ET9_CP_SYM_COUNT_OFFSET         = 0x66, /* offset to the Symbol character count  */
    ET9_CP_MUTE_CHAR_COUNT_OFFSET   = 0x67, /* offset to the stroke-only character count  */
    ET9_CP_SBI_MOD_OFFSET           = 0x69,   /* offset to the SBI module  */
    ET9_CP_CANGJIE_MOD_OFFSET       = 0x6D,   /* offset to the CangJie module  */
    ET9_CP_SIMP_TRAD_OFFSET         = 0x71    /* offset to the Simp<->Trad conversion table  */
} ET9_CP_LdbFieldOffsets;

extern const ET9U8 ET9_CP_Bpmf_Letter_To_Pinyin[];

/* End don't mangle the function name if compile under C++ */
#if defined(__cplusplus)
    }
#endif

#endif  /* #ifndef ET9CPLDB_H */

/* ----------------------------------< eof >--------------------------------- */
