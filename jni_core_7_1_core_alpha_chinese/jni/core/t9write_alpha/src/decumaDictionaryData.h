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



#if defined(DEBUG) && !defined(DECUMA_DICTIONARY_C)
#error Only decumaDictionary.c should need the data definition
#endif

#ifndef DECUMA_DICTIONARY_DATA_H
#define DECUMA_DICTIONARY_DATA_H

#include "decumaBasicTypes.h"
#include "decumaTrigramTable.h"
#include "decumaDictionary.h"         /* Definition of DECUMA_HWR_DICTIONARY */

/* This expands Achilles' idea of a dictionary object, where all dictionary accessors are function pointers. 
 * The preferred method to implement is getSubstringRef(), since that abstracts away the implicit graph 
 * structure from getChild()/getSibling(). 
 *
 * TODO:
 *   getSubstringRef should get a pointer to previous context, since that's where a possible trigram table would be used.
 */

typedef DECUMA_HWR_DICTIONARY_REF (*DECUMA_HWR_DICTIONARY_GET_SIBLING)(void * pDictionary,  DECUMA_HWR_DICTIONARY_REF currentRef);
typedef DECUMA_HWR_DICTIONARY_REF (*DECUMA_HWR_DICTIONARY_GET_CHILD)(void * pDictionary,  DECUMA_HWR_DICTIONARY_REF currentRef);
typedef DECUMA_HWR_DICTIONARY_REF (*DECUMA_HWR_DICTIONARY_GET_SUBSTR_REF)(void * pDictionary,  DECUMA_HWR_DICTIONARY_REF currentRef, const DECUMA_UNICODE * pUnicodes, DECUMA_UINT8 nUnicodes);
typedef void (*DECUMA_HWR_DICTIONARY_GET_MINMAX)(const void * pDictionary,  DECUMA_HWR_DICTIONARY_REF currentRef, DECUMA_UINT8 * pMin, DECUMA_UINT8 * pMax);
typedef DECUMA_UINT32 (*DECUMA_HWR_DICTIONARY_GET_SIZE)(const void * pDictionary);
typedef DECUMA_UNICODE (*DECUMA_HWR_DICTIONARY_GET_UNICODE)(const void * pDictionary, DECUMA_HWR_DICTIONARY_REF currentRef);
typedef int (*DECUMA_HWR_DICTIONARY_IS_EOW)(const void * pDictionary, DECUMA_HWR_DICTIONARY_REF currentRef);
typedef DECUMA_UINT32 (*DECUMA_HWR_DICTIONARY_GET_NWORDS)(const void * pDictionary);


typedef double (*DECUMA_HWR_DICTIONARY_GET_MAX_RANK_BOOST)(void * pDictionary, DECUMA_UINT32 nExpectedCharacters);
typedef double (*DECUMA_HWR_DICTIONARY_GET_RANK_BOOST)(const void * pDictionary, DECUMA_HWR_DICTIONARY_REF currentRef);

typedef void (*DECUMA_HWR_DICTIONARY_GET_RANK_IN_FREQCLASS)(const void * pDictionary, DECUMA_UINT8 freqClass,
																	 DECUMA_UINT32 * pMinRank, DECUMA_UINT32 * pMaxRank);
typedef DECUMA_UINT8 (*DECUMA_HWR_DICTIONARY_GET_SUBTREE_FREQCLASS)(const void * pDictionary, DECUMA_HWR_DICTIONARY_REF currentRef);
typedef void (*DECUMA_HWR_DICTIONARY_GET_SUBTREE_FREQCLASS_MINMAX)
	(const void * pDictionary, DECUMA_HWR_DICTIONARY_REF currentRef,
	DECUMA_UINT8 * pMin, DECUMA_UINT8 * pMax);
typedef DECUMA_UINT8 (*DECUMA_HWR_DICTIONARY_GET_WORD_FREQCLASS)
	(const void * pDictionary, DECUMA_HWR_DICTIONARY_REF currentRef);
typedef DECUMA_UINT8 (*DECUMA_HWR_DICTIONARY_GET_N_FREQCLASSES)(const void * pDictionary);

struct _DECUMA_HWR_DICTIONARY 
{
	/* Start with an ID field? */ 
	DECUMA_HWR_DICTIONARY_GET_SIBLING                       getSibling;
	DECUMA_HWR_DICTIONARY_GET_CHILD                         getChild;
	DECUMA_HWR_DICTIONARY_GET_MINMAX                        getMinMax;
	DECUMA_HWR_DICTIONARY_GET_SIZE                          getSize;
	DECUMA_HWR_DICTIONARY_GET_UNICODE                       getUnicode;
	DECUMA_HWR_DICTIONARY_IS_EOW                            getEndOfWord;
	DECUMA_HWR_DICTIONARY_GET_SUBSTR_REF                    getSubstringRef;
	DECUMA_HWR_DICTIONARY_GET_RANK_IN_FREQCLASS				getRankForFreq;
	DECUMA_HWR_DICTIONARY_GET_SUBTREE_FREQCLASS             getSubTreeFreq;
	DECUMA_HWR_DICTIONARY_GET_SUBTREE_FREQCLASS_MINMAX		getSubTreeFreqMinMax;
	DECUMA_HWR_DICTIONARY_GET_N_FREQCLASSES                 getNumberOfFreqs;
	DECUMA_HWR_DICTIONARY_GET_NWORDS                        getNumberOfWords;
	DECUMA_HWR_DICTIONARY_GET_WORD_FREQCLASS                getWordFreq;
	void * pDictionaryData;
};

#endif /* DECUMA_DICTIONARY_DATA_H */
