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
;**     FileName: et9acdb.h                                                   **
;**                                                                           **
;**  Description: Context data base access routines header file.              **
;**                                                                           **
;*******************************************************************************
;******************************************************************************/

#ifndef ET9ACDB_H
#define ET9ACDB_H    1

#include "et9api.h"
#ifdef ET9_ALPHABETIC_MODULE
#include "et9aslst.h"


/* don't mangle the function name if compile under C++ */
#if defined(__cplusplus)
extern "C" {
#endif

void ET9FARCALL _ET9AWCDBAddWord(ET9AWLingInfo      *pLingInfo,
                                 ET9AWPrivWordInfo  *pWord);

void ET9FARCALL _ET9AWCDBUpdateContext(ET9AWLingInfo     *pLingInfo,
                                       ET9AWPrivWordInfo *pWord);

void ET9FARCALL _ET9AWCDBAddShortcut(ET9AWLingInfo     *pLingInfo,
                                     ET9AWPrivWordInfo *pWord);

ET9STATUS ET9FARCALL _ET9AWCDBWordsSearch(ET9AWLingInfo          *pLingInfo,
                                          ET9U16                 wIndex,
                                          ET9U16                 wLength,
                                          ET9_FREQ_DESIGNATION   bFreqIndicator);

void ET9FARCALL _ET9AWCDBBreakContext(ET9AWLingInfo *pLingInfo);

/* define the delimiter that triggers prediction */
#define CDBDELIMITER (ET9SYMB) (0x20)

/* The Cdb data area pointer */
#define ET9CDBData(cdb) ((ET9SYMB ET9FARDATA *) cdb->sDataArea)

/* Macro to get actual number of bytes in the cdb header area */
#define ET9CDBHeaderBytes(cdb)    \
            ((ET9UINT)((ET9U8 *)ET9CDBData(cdb) - (ET9U8 ET9FARDATA *)cdb))

/* Macro to get actual number of symbols in the cdb data area */
#define ET9CDBDataAreaSymbs(cdb)    \
            ((ET9UINT)((cdb->wDataSize - ET9CDBHeaderBytes(cdb))/ET9SYMBOLWIDTH))

#if defined(__cplusplus)
}
#endif

#endif /* ET9_ALPHABETIC_MODULE */
#endif /* ET9ACDB_H */


/* eof */
