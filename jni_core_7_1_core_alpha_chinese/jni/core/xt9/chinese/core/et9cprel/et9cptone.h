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
;**     FileName: et9cptone.h                                                 **
;**                                                                           **
;**  Description: Chinese Phrase Text Input tone module header file.          **
;**               Conforming to the development version of Chinese XT9.       **
;**                                                                           **
;*******************************************************************************
;******************************************************************************/

#ifndef ET9CPTONE_H
#define ET9CPTONE_H 1

/* don't mangle the function name if compile under C++ */
#if defined(__cplusplus)
extern "C" {
#endif

#include "et9cpsys.h"

#define ET9_CP_ALL_TONES_BIT_MASK 0x1F

#define ET9_CP_SymbIsTone(pSymbInfo)    (ET9BOOL)(   (pSymbInfo)->bNumBaseSyms == 1 \
                                                  && (pSymbInfo)->DataPerBaseSym[0].bNumSymsToMatch == 1 \
                                                  && ET9CPSymToCPTone((pSymbInfo)->DataPerBaseSym[0].sChar[0]) )

#define ET9_CP_InputHasTone(pCPLing)    (ET9BOOL)((pCPLing)->Private.PPrivate.bInputHasTone)

/*----------------------------------------------------------------------------
 *  Define the tone function prototypes.
 *----------------------------------------------------------------------------*/

ET9U8 ET9FARCALL ET9_CP_GetTones(ET9CPLingInfo * pET9CPLingInfo, ET9_CP_Spell * pSpell);
ET9U8 ET9FARCALL ET9_CP_GetSpellTones(ET9U8 *pbSpell, ET9U8 bLen, ET9U8 *pbTones);
ET9_CP_SpellMatch ET9FARCALL ET9_CP_ValidateToneSpell(
    ET9CPLingInfo *pLing,
    ET9_CP_Spell *pSpell,
    ET9BOOL bNeedPartialPhrase);

/* End don't mangle the function name if compile under C++ */
#if defined(__cplusplus)
    }
#endif

#endif  /* #ifndef ET9CPTONE_H */

/* ----------------------------------< eof >--------------------------------- */
