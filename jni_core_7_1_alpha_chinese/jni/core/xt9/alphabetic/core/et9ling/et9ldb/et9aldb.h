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
;**     FileName: et9aldb.h                                                    **
;**                                                                           **
;**  Description: Alphabetic LDB routines header file.                        **
;**                                                                           **
;*******************************************************************************
;******************************************************************************/

#ifndef ET9ALDB_H
#define ET9ALDB_H    1

#include "et9api.h"
#include "et9aslst.h"


/* don't mangle the function name if compile under C++ */
#if defined(__cplusplus)
extern "C" {
#endif


/*---------------------------------------------------------------------------
 *      Locations of data in Alphabetic LDB.
 *---------------------------------------------------------------------------*/

#define ET9LDBOFFSET_LDBLAYOUTVER       0x20
#define ET9LDBOFFSET_DATABASETYPE       0x21
#define ET9LDBOFFSET_CHUNK_COUNT_BYTE   0x23
#define ET9LDBOFFSET_COMPATID           0x33

#define ET9LDBOFFSET_CONTENTSMAJORVER   0x35
#define ET9LDBOFFSET_CONTENTSMINORVER   0x36
#define ET9LDBOFFSET_CONTENTSDEVIATION  0x37
#define ET9LDBOFFSET_CONTENTSHEADERDEV  0x38

#define ET9LDBOFFSET_PRIMARYLANGID      0x39
#define ET9LDBOFFSET_SECONDARYLANGID    0x3A
#define ET9LDBOFFSET_SYMBOLCLASS        0x3C

#define ET9LDBOFFSET_OEMID              0x3D
#define ET9LDBOFFSET_CHECKSUM           0x3F

#define ET9LDBOFFSET_BODY               0x41


/* LM specific header entries */

#define ALM_VERSION_OFFSET              4
#define ALM_LANGUAGE_OFFSET             5
#define ALM_START_ADDRESS_OFFSET        6
#define ALM_END_ADDRESS_OFFSET          9
#define ALM_NUM_ENTRIES_OFFSET          12
#define ALM_NUM_CLASSES_OFFSET          15
#define ALM_EMISSION_ENCODING_OFFSET    17
#define ALM_TRANSITION_ENCODING_OFFSET  19
#define ALM_SCALE_FACTOR_OFFSET         21
#define ALM_ADD_CONSTANT_OFFSET         22


void ET9FARCALL _ET9AWLdbWordsByIndex(ET9AWLingInfo              * const pLingInfo,
                                      const ET9U16                       wLdbNum,
                                      ET9U32                     * const pdwIndexes,
                                      const ET9UINT                      nIndexCount);

void ET9FARCALL _ET9AWLdbWordsSearch(ET9AWLingInfo              * const pLingInfo,
                                     const ET9U16                       wLdbNum,
                                     const ET9U16                       wIndex,
                                     const ET9U16                       wLength,
                                     ET9U8                      * const pbLdbEntries,
                                     const ET9_FREQ_DESIGNATION         bFreqIndicator,
                                     const ET9U8                        bSpcMode);

ET9STATUS ET9FARCALL _ET9AWLdbFind(ET9AWLingInfo        * const pLingInfo,
                                   const ET9U16                 wLdbNum,
                                   ET9AWPrivWordInfo    * const pWord,
                                   ET9U8                * const pbExact,
                                   ET9U8                * const pbLowercase,
                                   const ET9BOOL                bRetrieve);

ET9STATUS ET9FARCALL _ET9AWLdbFindEntry(ET9AWLingInfo       * const pLingInfo,
                                        const ET9U16                wLdbNum,
                                        ET9SYMB             * const psWord,
                                        const ET9U16                wWordLen,
                                        ET9U32              * const pdwIndex,
                                        ET9U8               * const pbExact,
                                        ET9U8               * const pbLowercase);

ET9STATUS ET9FARCALL _ET9AWLdbGetWordFreq(ET9AWLingInfo     * const pLingInfo,
                                          const ET9U16              wLdbNum,
                                          const ET9U32              dwIndex,
                                          ET9FREQPART       * const pxWordFreq,
                                          ET9U16            * const pwEWordFreq,
                                          ET9U16            * const pwTWordFreq);

ET9U8 ET9FARCALL _ET9ReadLDBByte (ET9AWLingInfo * const pLingInfo,
                                  const ET9U32          dwOffset);

ET9U16 ET9FARCALL _ET9ReadLDBWord2 (ET9AWLingInfo * const pLingInfo,
                                    const ET9U32          dwOffset);

ET9U32 ET9FARCALL _ET9ReadLDBWord3 (ET9AWLingInfo * const pLingInfo,
                                    const ET9U32          dwOffset);

ET9BOOL ET9FARCALL _ET9AWLdbIsSymbolUsed(ET9AWLingInfo * const pLingInfo,
                                         const ET9U16          wLdbNum,
                                         const ET9SYMB         sSymb);

ET9STATUS ET9FARCALL _ET9AWLdbSetActiveLanguage(ET9AWLingInfo * const pLingInfo,
                                                const ET9U16          wLdbNum);

ET9STATUS ET9FARCALL _ET9AWLdbLanguageModelInit(ET9AWLingInfo * const   pLingInfo,
                                                const ET9U16            wLdbNum);

void ET9FARCALL _ET9AWLdbTagContextClass(ET9AWLingInfo * const pLingInfo);

/* this feature is for future release */
#if 0
ET9U8 ET9FARCALL _ET9AWLdbGetNextSymbolsPrediction(ET9AWLingInfo     *pLingInfo,
                                                   ET9AWWordSymbInfo *pWordSymbInfo);
#endif

/* End don't mangle the function name if compile under C++ */
#if defined (__cplusplus)
    }
#endif

#endif /* !ET9ALDB_H */
/* ----------------------------------< eof >--------------------------------- */
