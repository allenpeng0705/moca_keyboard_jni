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
;**     FileName:  et9cstrie.h                                                **
;**                                                                           **
;**  Description:  Header file of SBI trie                                    **
;**                                                                           **
;**                                                                           **
;*******************************************************************************
;******************************************************************************/
#ifndef __SBI_Trie_H__
#define __SBI_Trie_H__

#ifdef __cplusplus
extern "C" {
#endif


#define ET9_CS_SBI_VERSION      0x0003
#define ET9_CS_PINYIN_TRIE      0x00
#define ET9_CS_BPMF_TRIE        0x01
#define ET9_CS_STROKE_TRIE      0x02

ET9STATUS ET9FARCALL ET9_CS_SysInit( ET9_CP_SSBITrie* pTrie, ET9CPLingInfo * pET9CPLingInfo );
ET9STATUS ET9FARCALL ET9_CS_SBIInit( ET9_CP_SSBITrie* pTrie );
void      ET9FARCALL ET9_CS_ResetSBI(ET9_CP_SSBITrie* pTrie);

ET9_CP_SpellMatch ET9FARCALL ET9_CS_GetCandidate( const ET9_CP_SSBITrie* pTrie, ET9S32 i, ET9_CS_Candidate * pCandidate );
ET9S32        ET9FARCALL ET9_CS_GetCandidateCount( const ET9_CP_SSBITrie* pTrie );

ET9STATUS     ET9FARCALL ET9_CS_SelectSegment(ET9_CP_SSBITrie* pTrie,
                                             const ET9SYMB * psPhrase,
                                             ET9U8 bPraseLen,
                                             const ET9U8 * pcSpell,
                                             ET9U8 bSpellLen ); /* pcSelect contains neither DELEMITER nor '_' */
ET9STATUS     ET9FARCALL ET9_CS_GetCondition(ET9_CP_SSBITrie* pTrie, ET9_CS_Prefix * pPfx);
ET9STATUS     ET9FARCALL ET9_CS_SetCondition(ET9_CP_SSBITrie* pTrie, const ET9U8 * pcCondition, ET9S32 iConditionLen, ET9S32 iPfxProb); /* pcPrefix is a onset, contains neither DELEMITER nor '_' */
ET9_CP_SpellMatch ET9FARCALL ET9_CS_BuildCandidates( ET9_CP_SSBITrie* pTrie );
ET9_CP_SpellMatch ET9FARCALL ET9_CP_SegmentationToSpell(ET9CPLingInfo * const pET9CPLingInfo, ET9_CP_Spell * pSpell);

/* Prefix Module */
void      ET9FARCALL ET9_CS_PhrasalPrefix( ET9_CP_SSBITrie* pTrie );
void      ET9FARCALL ET9_CS_ClearPrefixBuf( ET9_CP_SSBITrie* pTrie );
ET9STATUS ET9FARCALL ET9_CS_GetPrefix(const ET9_CP_SSBITrie* pTrie, ET9S32 iIndex, ET9_CS_Prefix* pPrefix );
ET9S32    ET9FARCALL ET9_CS_GetPrefixCount( ET9_CP_SSBITrie* pTrie );

ET9STATUS ET9FARCALL ET9_CP_BuildSBISpellings(ET9CPLingInfo * const pLing);
ET9INT ET9FARCALL ET9_CP_HasPhrasalCPSpelling(ET9CPLingInfo * pET9CPLingInfo);


ET9U16 ET9FARCALL ET9_CP_SBI_ScorePhrase(ET9_CP_SpellData * psSpellData,
                                         ET9_CP_CommonInfo * pCommon,
                                         ET9SYMB * psPhrase,
                                         ET9U8 bPhraseLen,
                                         ET9U16 wFreq, 
                                         ET9U8 bIsFromContext,
                                         ET9U8 bIsFromUdb,
                                         ET9UINT * pfSuppress);

#ifdef __cplusplus
    }
#endif

#endif  /*  __SBI_Trie_H__  */

/* ----------------------------------< eof >--------------------------------- */
