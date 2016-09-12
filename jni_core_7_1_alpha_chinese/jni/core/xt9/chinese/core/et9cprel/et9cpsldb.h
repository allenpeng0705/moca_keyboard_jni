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
;**     FileName: et9cpsldb.h                                                 **
;**                                                                           **
;**  Description: Chinese XT9 stroke ldb access functions.                    **
;**                                                                           **
;*******************************************************************************
;******************************************************************************/

#ifndef ET9CPSLDB_H
#define ET9CPSLDB_H 1

/* don't mangle the function name if compile under C++ */
#if defined(__cplusplus)
extern "C" {
#endif
#ifndef ET9CP_DISABLE_STROKE

typedef enum {
    ET9_CP_SID_NO_MATCH = 0,
    ET9_CP_SID_EXACT_MATCH,
    ET9_CP_SID_PARTIAL_MATCH,
    ET9_CP_SID_UNKNOWN_MATCH
} ET9_CP_SIDMatch;

void ET9_CP_StrokeKeysChangeInit(ET9CPLingInfo *pET9CPLingInfo);
ET9STATUS ET9FARCALL ET9_CP_StrokeSelectPhrase(ET9CPLingInfo *pET9CPLingInfo, ET9U16 wPhraseIndex, ET9_CP_Spell *pSpell, ET9U8 *pbNumSymbs);
ET9STATUS ET9FARCALL ET9_CP_StrokeFillPhraseBuffer(ET9CPLingInfo *pET9CPLingInfo);
ET9U8 ET9FARCALL ET9_CP_StrokeLookup(ET9CPLingInfo *pET9CPLingInfo, ET9U16 wSID, ET9U8 *pbBuf, ET9U8 bLen);
void ET9_CP_StrokeSetupKey(ET9CPLingInfo *pET9CPLingInfo, ET9U8 bKey);
ET9U16 ET9FARCALL ET9_CP_SIDToTID(ET9CPLingInfo *pET9CPLingInfo, ET9U16 wSID);
ET9U16 ET9FARCALL ET9_CP_TIDToSID(ET9CPLingInfo *pET9CPLingInfo, ET9U16 wTID, ET9U16 wStart, ET9U16 wEnd);
void ET9FARCALL ET9_CP_StrokeAddValidKeys(ET9CPLingInfo *pET9CPLingInfo, ET9U8 * pbKeySeq, ET9U8 bKeySeqLen);
void ET9FARCALL ET9_CP_StrokeCountChars(const ET9U8 *pbKeys, ET9UINT nLen, ET9U8 *pbNumChars, ET9U8 *pbNumStrokes1stChar, ET9U8 *pbLastDelPos);
void ET9FARCALL ET9_CP_SetTrailingSIDRanges(ET9CPLingInfo *pET9CPLingInfo, ET9U8 *pbKeyBuf, ET9U8 bNumStrokes1stChar, ET9U8 bKeyLen);
#endif /* END ET9CP_DISABLE_STROKE */
/* End don't mangle the function name if compile under C++ */
#if defined(__cplusplus)
    }
#endif

#endif  /* #ifndef ET9CPSLDB_H */
