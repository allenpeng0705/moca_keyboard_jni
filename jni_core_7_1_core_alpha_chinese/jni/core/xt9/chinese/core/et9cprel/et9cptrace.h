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
;**     FileName: et9cptrace.h                                                **
;**                                                                           **
;**  Description: Chinese XT9 Trace module header file.                       **
;**               Conforming to the development version of Chinese XT9        **
;**                                                                           **
;*******************************************************************************
;******************************************************************************/

#ifndef ET9CPTRACE_H
#define ET9CPTRACE_H 1

#ifdef ET9_ALPHABETIC_MODULE

/* don't mangle the function name if compile under C++ */
#if defined(__cplusplus)
extern "C" {
#endif

/*----------------------------------------------------------------------------
 *  Define the internal Trace function prototypes.
 *----------------------------------------------------------------------------*/

void ET9FARCALL ET9_CP_Trace_Clear(ET9_CP_Trace * pTrace);

ET9STATUS ET9FARCALL ET9_CP_Trace_LdbSync(ET9CPLingInfo * pCPLing);

ET9U16 ET9FARCALL ET9_CP_Trace_ScorePhrase(ET9_CP_SpellData * psSpellData,
                                           ET9_CP_CommonInfo * pCommon,
                                           ET9SYMB * psPhrase,
                                           ET9U8 bPhraseLen,
                                           ET9U16 wFreq,
                                           ET9BOOL fIsFromContext,
                                           ET9BOOL fIsFromUdb);

ET9STATUS ET9FARCALL ET9_CP_Trace_BuildSelectionList(ET9CPLingInfo * pCPLing);

/* Prefix-related */
ET9U8    ET9FARCALL ET9_CP_Trace_GetPrefixCount(const ET9_CP_Trace * pTrace);

ET9STATUS ET9FARCALL ET9_CP_Trace_GetPrefix(const ET9_CP_Trace * pTrace, ET9U16 wIndex, ET9CPSpell * psSpell);

ET9STATUS ET9FARCALL ET9_CP_Trace_SetActivePrefix(ET9_CP_Trace * pTrace, ET9U8 bPrefixIndex);

ET9STATUS ET9FARCALL ET9_CP_Trace_ClearActivePrefix(ET9_CP_Trace * pTrace);

ET9STATUS ET9FARCALL ET9_CP_Trace_GetActivePrefixIndex(ET9_CP_Trace * pTrace, ET9U8 * pbPrefixIndex);

/* Phrase-related */
ET9STATUS ET9FARCALL ET9_CP_Trace_FillPhraseBuffer(ET9CPLingInfo *pET9CPLingInfo);

ET9STATUS ET9FARCALL ET9_CP_Trace_SelectPhrase(ET9_CP_Trace * pTrace, ET9U16 wPhraseIndex, ET9CPSpell * pSpell);
                                               
ET9STATUS ET9FARCALL ET9_CP_Trace_ReplaceByExplicitSymb(ET9WordSymbInfo * pWSI, ET9UINT nReplaceLen, const ET9SYMB * psExplicitSymb, ET9UINT nExplicitSymbLen);

/* End don't mangle the function name if compile under C++ */
#if defined(__cplusplus)
    }
#endif

#endif

#endif  /* #ifndef ET9CPTRACE_H */

/* ----------------------------------< eof >--------------------------------- */


