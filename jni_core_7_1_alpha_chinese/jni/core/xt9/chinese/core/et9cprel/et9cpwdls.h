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
;**     FileName: et9cpwdls.h                                                 **
;**                                                                           **
;**  Description: Chinese Phrase Text Input word list module header file.     **
;**               Conforming to the development version of Chinese XT9.       **
;**                                                                           **
;*******************************************************************************
;******************************************************************************/

#ifndef ET9CPWDLS_H
#define ET9CPWDLS_H 1

/* don't mangle the function name if compile under C++ */
#if defined(__cplusplus)
extern "C" {
#endif

#include "et9cpname.h"

#define ET9_CP_PHRASE_GROUP_ADDR_TABLE_OFFSET    0x06

/*----------------------------------------------------------------------------
 *  Define the word list function prototypes.
 *----------------------------------------------------------------------------*/
void ET9FARCALL ET9_CP_GetLdbPhrases(
    ET9CPLingInfo *pET9CPLingInfo,
    ET9BOOL        bIsSID,
    ET9BOOL        bNeedPartialSyl,
    ET9BOOL        bNeedPartialPhrase,
    ET9_CP_SpellMatch *pMatchType,
    ET9BOOL        bGetToneOption,
    ET9U8         *pbToneOptions,
    ET9U8         *pbTones,
    const ET9U16  *pwCntxPrefix,
    ET9U8          bCntxPrefixLen,
    ET9BOOL        bValidateCntxPrefix,
    ET9_CP_NameTableBookmark *pNameTableBookmarks,
    ET9_CP_PhraseBuf *pPhraseBuf);

ET9BOOL ET9FARCALL ET9_CP_FindPhraseInLdb(
    ET9CPLingInfo *pLingInfo,
    ET9_CP_IDEncode eEncode,
    const ET9CPPhrase *pPhrase);

/* End don't mangle the function name if compile under C++ */
#if defined(__cplusplus)
    }
#endif

#endif  /* #ifndef ET9CPWDLS_H */

/* ----------------------------------< eof >--------------------------------- */
