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
;**     FileName: et9cpname.h                                                 **
;**                                                                           **
;**  Description: Chinese Phrase Text Input name input module header file.    **
;**               Conforming to the development version of Chinese XT9.       **
;**                                                                           **
;*******************************************************************************
;******************************************************************************/

#ifndef ET9CPNAME_H
#define ET9CPNAME_H 1

/* don't mangle the function name if compile under C++ */
#if defined(__cplusplus)
extern "C" {
#endif

#define ET9_CP_MAX_NAMETABLE_BOOKMARK     6

/* a structure to temporarily remember a reference point in name table */
typedef struct ET9_CP_NameTableBookmark_s {
    ET9U16 wOffset;  /* the 0-based entry index as the bookmark location */
    ET9U16 wID; /* entries starting from wOffset will have ID >= wID */
} ET9_CP_NameTableBookmark;

/*----------------------------------------------------------------------------
 *  Define function prototypes.
 *----------------------------------------------------------------------------*/
void ET9FARCALL ET9_CP_InitNameTableBookmarks(ET9CPLingInfo *pET9CPLingInfo,
                                           ET9_CP_NameTableBookmark *pBookmarks,
                                           ET9UINT nCount,
                                           ET9UINT fIsSID);

void ET9FARCALL ET9_CP_GetCommonNameChar(ET9CPLingInfo *pET9CPLingInfo,
                                      ET9UINT fIsSID);

ET9U8 ET9FARCALL ET9_CP_GetNameCharFreq(ET9CPLingInfo *pET9CPLingInfo,
                                    ET9UINT fIsSID,
                                    ET9U16 wID);
ET9U8 ET9FARCALL ET9_CP_FindNameMatch(
    ET9CPLingInfo *pET9CPLingInfo,
    ET9UINT fIsSID,
    ET9BOOL bNeedPartialSyl,
    ET9_CP_NameTableBookmark *pBookmarks,
    ET9U8 *pbTones,
    ET9U8 fFindPhrase);
/* End don't mangle the function name if compile under C++ */
#if defined(__cplusplus)
    }
#endif

#endif  /* #ifndef ET9CPNAME_H */

/* ----------------------------------< eof >--------------------------------- */
