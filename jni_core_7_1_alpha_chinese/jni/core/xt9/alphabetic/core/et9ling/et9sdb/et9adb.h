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
;**     FileName: et9adb.h                                                    **
;**                                                                           **
;**  Description: Alphabetic supplemental database routines     .             **
;**                                                                           **
;*******************************************************************************
;******************************************************************************/

#ifndef ET9ADB_H
#define ET9ADB_H    1

#include "et9api.h"
#ifdef ET9_ALPHABETIC_MODULE
#include "et9arudb.h"


#define ET9SUPPDB_ALL_SOURCES       0xFF
#define ET9SUPPDB_AS_SOURCES        0x01
#define ET9SUPPDB_NONAS_SOURCES     0x02

#define USE_PROCESSING   ((ET9UINT) 0)
#define SPACE_PROCESSING ((ET9UINT) 1)

ET9STATUS ET9FARCALL _ET9AWSuppDBSelListBuild(ET9AWLingInfo           * const pLingInfo,
                                              const ET9U16                    wIndex,
                                              const ET9U16                    wLength,
                                              ET9U8                   * const pbSuppEntries,
                                              const ET9_FREQ_DESIGNATION      bFreqIndicator,
                                              const ET9U8                     bSourceTypes,
                                              const ET9U8                     bSpcMode);

ET9STATUS ET9FARCALL _ET9AWSuppDBAddSelection(ET9AWLingInfo     *pLingInfo,
                                              ET9AWPrivWordInfo *pSelWord,
                                              ET9UINT            nProcessType);

/* TUDB */

ET9STATUS ET9FARCALL _ET9TUdbReadData(ET9U8 ET9FARDATA  *pbTo,
                                      ET9U16             wSize,
                                      ET9U8 ET9FARDATA  *pTUdb,
                                      ET9U32             dwOffset,
                                      ET9ReadTUDB_f      ET9ReadTUDB);

ET9STATUS ET9FARCALL _ET9TUdbReadWord(ET9U16            *pwWord,
                                      ET9U8 ET9FARDATA  *pTUdb,
                                      ET9U32             dwTUdbOffset,
                                      ET9ReadTUDB_f      ET9ReadTUDB);

ET9STATUS ET9FARCALL _ET9TUdbWriteData(ET9U8 ET9FARDATA  *pbFrom,
                                       ET9U16             wSize,
                                       ET9U8 ET9FARDATA  *pTUdb,
                                       ET9U32             dwOffset,
                                       ET9WriteTUDB_f     ET9WriteTUDB);

ET9STATUS ET9FARCALL _ET9TUdbWriteWord(ET9U16             wWord,
                                       ET9U8 ET9FARDATA  *pTUdb,
                                       ET9U32             dwTUdbOffset,
                                       ET9WriteTUDB_f     ET9WriteTUDB);

#endif /* ET9_ALPHABETIC_MODULE */
#endif /* !ET9ADB_H */

/* ----------------------------------< eof >--------------------------------- */
