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
;**     FileName: ET9ALSASDB.h                                                **
;**                                                                           **
;**  Description: LDB supported Auto Substitution data base access routines   **
;**               header file.                                                **
;**               Conforms to Version 3.0                                     **
;**                                                                           **
;*******************************************************************************
;******************************************************************************/

#ifndef ET9ALSASDB_H
#define ET9ALSASDB_H    1

#include "et9api.h"
#ifdef ET9_ALPHABETIC_MODULE
#include "et9aslst.h"
#include "et9aldb.h"

/* don't mangle the function name if compile under C++ */
#if defined(__cplusplus)
extern "C" {
#endif

#define CHUNK_ID_OFFSET                 0
#define CHUNK_SIZE_OFFSET               1
#define LSASDB_VERSION_OFFSET           4
#define LSASDB_LANGUAGE_OFFSET          5
#define LSASDB_START_ADDRESS_OFFSET     6
#define LSASDB_END_ADDRESS_OFFSET       9
#define LSASDB_NUM_ENTRIES_OFFSET       12

/*------------------------------------------------------------------------
 *  Define ET9 Alphabetic LsASDB functions
 *------------------------------------------------------------------------*/
ET9STATUS ET9FARCALL _ET9AWLdbASGetEntry(ET9AWLingInfo *pLingInfo,
                                         ET9U16         wLdbNum,
                                         ET9U16         wRecordNum,
                                         ET9SYMB       *psShortcut,
                                         ET9U16        *pwShortcutLen,
                                         ET9SYMB       *psSubstitution,
                                         ET9U16        *pwSubstitutionLen);

ET9STATUS ET9FARCALL _ET9AWLdbASFindEntry(ET9AWLingInfo *pLingInfo,
                                          ET9U16         wLdbNum,
                                          ET9SYMB       *psShortcut,
                                          ET9U16         wShortcutLen,
                                          ET9SYMB       *psSubstitution,
                                          ET9U16        *pwSubstitutionLen,
                                          ET9U16        *pwRecordNum);


ET9STATUS ET9FARCALL _ET9AWLdbASWordsSearch(ET9AWLingInfo           *pLingInfo,
                                            ET9U16                   wLdbNum,
                                            ET9U16                   wIndex,
                                            ET9U16                   wLength,
                                            ET9_FREQ_DESIGNATION     bFreqIndicator);

ET9UINT ET9FARCALL _ET9AWFindLdbASObject(ET9AWLingInfo     *pLingInfo,
                                         ET9U16             wLdbNum,
                                         ET9SYMB           *pWord,
                                         ET9U16             wLength,
                                         ET9U8              bCaseSensitive,
                                         ET9U8              bUpdateWithMatch);

#if defined(__cplusplus)
}
#endif

#endif /* ET9_ALPHABETIC_MODULE */
#endif /* ET9ALSASDB_H */


/* eof */
