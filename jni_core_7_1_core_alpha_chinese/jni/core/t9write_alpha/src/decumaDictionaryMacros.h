/*******************************************************************************
;*******************************************************************************
;**                                                                           **
;**                    COPYRIGHT 2010 NUANCE COMMUNICATIONS                   **
;**                                                                           **
;**                NUANCE COMMUNICATIONS PROPRIETARY INFORMATION              **
;**                                                                           **
;**     This software is supplied under the terms of a license agreement      **
;**     or non-disclosure agreement with Nuance Communications and may not    **
;**     be copied or disclosed except in accordance with the terms of that    **
;**     agreement.                                                            **
;**                                                                           **
;*******************************************************************************
;******************************************************************************/


/* "Internal" dictionary functions that represents more low-level operations on a dictionary object. */

#ifndef DECUMA_DICTIONARY_MACROS_H
#define DECUMA_DICTIONARY_MACROS_H

/* Macros to call object functions with correct parameters */
#define decumaDictionaryGetDataSizeMacro(d)              (sizeof(DECUMA_HWR_DICTIONARY) + (d)->getSize((d)->pDictionaryData))
#define decumaDictionaryGetMinMaxMacro(d, ref, min, max) (d)->getMinMax((d)->pDictionaryData, (ref), (min), (max))
#define decumaDictionaryGetChildMacro(d, ref)            (d)->getChild((d)->pDictionaryData, (ref))
#define decumaDictionaryGetSiblingMacro(d, ref)          (d)->getSibling((d)->pDictionaryData, (ref))
#define decumaDictionaryGetUnicodeMacro(d, ref)          (d)->getUnicode((d)->pDictionaryData, (ref))
#define decumaDictionaryGetEndOfWordMacro(d, ref)        (d)->getEndOfWord((d)->pDictionaryData, (ref))
#define decumaDictionaryGetNumberOfWordsMacro(d)         (d)->getNumberOfWords((d)->pDictionaryData)

#define decumaDictionaryGetNFreqClassesMacro(d)                   (d)->getNumberOfFreqs((d)->pDictionaryData)
#define decumaDictionaryGetRankInFreqClassMacro(d,fc,pMin,pMax)   (d)->getRankForFreq((d)->pDictionaryData, (fc), (pMin),(pMax))
#define decumaDictionaryGetSubTreeFreqClassMacro(d,ref)           (d)->getSubTreeFreq((d)->pDictionaryData, (ref))
#define decumaDictionaryGetSubTreeFreqClassMinMaxMacro(d,ref,pmin,pmax)(d)->getSubTreeFreqMinMax((d)->pDictionaryData, (ref),(pmin),(pmax))
#define decumaDictionaryGetWordFreqClassMacro(d,ref)              (d)->getWordFreq((d)->pDictionaryData, (ref))

#endif /* DECUMA_DICTIONARY_MACROS_H */
