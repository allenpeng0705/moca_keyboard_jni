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


/************************************************** Description ***\

  This file describes the interface to a Decumas HWR dictionary which is
  a datatype used run-time to hold a pointer to the dictionary structure
  and pointers to the functions of the dictionary.
    
\******************************************************************/


/* High-level dictionary functions. */

#ifndef DECUMA_DICTIONARY_H
#define DECUMA_DICTIONARY_H

/* Early type definition, since decuma_hwr.h depends on this symbol */
typedef struct _DECUMA_HWR_DICTIONARY DECUMA_HWR_DICTIONARY; 
typedef void * DECUMA_HWR_DICTIONARY_REF; /*A node pointer */

#include "decumaConfig.h"
#include "decumaStatus.h"
#include "decumaBasicTypes.h" /* Defintion of basic integer types */
#include "decumaTerseTrie.h"     /* Definition of DECUMA_TERSE_TRIE */
#include "decumaTrie.h"     /* Definition of DECUMA_TRIE */
#include "decumaDictionaryMacros.h"
#include "decumaStatus.h" /*Definition of DECUMA_STATUS */

/*//////////////////// DICTIONARY INIT/DESTROY FUNCTIONS  /////////////////////// */

/* Get size (in bytes) of a HWR dictionary structure without including the actual data*/
DECUMA_HWR_PRIVATE DECUMA_UINT32 decumaDictionaryGetSize(void);

/*Init with a trie trie, or a terseTrie. The dictionary will look at the header to find out */
/*what kind of data it is */
DECUMA_HWR_PRIVATE DECUMA_STATUS decumaDictionaryInitWithData(DECUMA_HWR_DICTIONARY * pDictionary,
								  const void * pData);

/*Returns the pointer to the trie, or terseTrie that the dicitonary was initialized with */
DECUMA_HWR_PRIVATE void * decumaDictionaryGetDataPtr(DECUMA_HWR_DICTIONARY * pDictionary);

/*Also destroys its contianed graph */
DECUMA_HWR_PRIVATE void decumaDictionaryDestroyData(DECUMA_HWR_DICTIONARY * pDictionary,
								 const DECUMA_MEM_FUNCTIONS * pMemFunctions);

/*Returns an error code if the data is not a recognized dictionary type */
DECUMA_HWR_PRIVATE DECUMA_STATUS decumaDictionaryValidateBinaryData(const void * pData);

/*Identifies the kind of ditionary data and destroys it if the type is recognized */
DECUMA_HWR_PRIVATE DECUMA_STATUS decumaDictionaryDestroyBinaryData(void ** ppData,
	const DECUMA_MEM_FUNCTIONS * pMemFunctions);

DECUMA_HWR_PRIVATE DECUMA_STATUS decumaDictionaryWordExists(const DECUMA_HWR_DICTIONARY * pDictionary,
	DECUMA_UNICODE * pWord,int maxLength,
	DECUMA_STRING_TYPE * pStringType);

/*//////////////////// DICTIONARY ACCESS FUNCTIONS  /////////////////////// */

#ifdef DEBUG

/* In a debug configuration, use proper functions for all operations. */

DECUMA_HWR_DICTIONARY_REF decumaDictionaryGetSibling      (const DECUMA_HWR_DICTIONARY *pDictionary, DECUMA_HWR_DICTIONARY_REF currentRef);
DECUMA_HWR_DICTIONARY_REF decumaDictionaryGetChild        (const DECUMA_HWR_DICTIONARY *pDictionary, DECUMA_HWR_DICTIONARY_REF currentRef);
                     void decumaDictionaryGetMinMax       (const DECUMA_HWR_DICTIONARY *pDictionary, DECUMA_HWR_DICTIONARY_REF currentRef, DECUMA_UINT8 * pMin, DECUMA_UINT8 * pMax);
            DECUMA_UINT32 decumaDictionaryGetDataSize     (const DECUMA_HWR_DICTIONARY *pDictionary);
           DECUMA_UNICODE decumaDictionaryGetUnicode      (const DECUMA_HWR_DICTIONARY *pDictionary, DECUMA_HWR_DICTIONARY_REF currentRef);
                      int decumaDictionaryGetEndOfWord    (const DECUMA_HWR_DICTIONARY *pDictionary, DECUMA_HWR_DICTIONARY_REF currentRef);
            DECUMA_UINT32 decumaDictionaryGetNumberOfWords(const DECUMA_HWR_DICTIONARY * pDictionary);
            DECUMA_UINT8  decumaDictionaryGetNFreqClasses (const DECUMA_HWR_DICTIONARY * pDictionary);
            void decumaDictionaryGetRankInFreqClass       (const DECUMA_HWR_DICTIONARY * pDictionary, 
				                                                    DECUMA_UINT8 freqClass,
																	DECUMA_UINT32 * pMinRank, DECUMA_UINT32 * pMaxRank);
             DECUMA_UINT8 decumaDictionaryGetSubTreeFreqClass      (const DECUMA_HWR_DICTIONARY * pDictionary, 
				                                                    DECUMA_HWR_DICTIONARY_REF currentRef);

             DECUMA_UINT8 decumaDictionaryGetWordFreqClass         (const DECUMA_HWR_DICTIONARY * pDictionary, 
				                                                    DECUMA_HWR_DICTIONARY_REF currentRef);


			         void decumaDictionaryGetSubTreeFreqClassMinMax(const DECUMA_HWR_DICTIONARY * pDictionary, 
				                                                    DECUMA_HWR_DICTIONARY_REF currentRef,
																	DECUMA_UINT8 * pMin, DECUMA_UINT8 * pMax);

#else

/* In a release configuration, use macros directly. Also, in a release configuration we need to #include the struct definitions directly */

#include "decumaDictionaryData.h" 

#define decumaDictionaryGetDataSize		 decumaDictionaryGetDataSizeMacro
#define decumaDictionaryGetMinMax		 decumaDictionaryGetMinMaxMacro
#define decumaDictionaryGetChild		 decumaDictionaryGetChildMacro
#define decumaDictionaryGetSibling		 decumaDictionaryGetSiblingMacro
#define decumaDictionaryGetUnicode		 decumaDictionaryGetUnicodeMacro
#define decumaDictionaryGetEndOfWord	 decumaDictionaryGetEndOfWordMacro
#define decumaDictionaryGetNumberOfWords decumaDictionaryGetNumberOfWordsMacro
#define decumaDictionaryGetNFreqClasses	 decumaDictionaryGetNFreqClassesMacro
#define decumaDictionaryGetRankInFreqClass	 decumaDictionaryGetRankInFreqClassMacro
#define decumaDictionaryGetSubTreeFreqClass          decumaDictionaryGetSubTreeFreqClassMacro
#define decumaDictionaryGetSubTreeFreqClassMinMax          decumaDictionaryGetSubTreeFreqClassMinMaxMacro
#define decumaDictionaryGetWordFreqClass          decumaDictionaryGetWordFreqClassMacro

#endif /* DEBUG */

/* This function takes a dictionary reference and a unicode array, and returns a new dictionary reference if the substring can be appended.
 * On failure, this function returns NULL.
 */
DECUMA_HWR_PRIVATE DECUMA_HWR_DICTIONARY_REF decumaDictionaryGetSubstringRef(const DECUMA_HWR_DICTIONARY * pDictionary,
														  DECUMA_HWR_DICTIONARY_REF currentRef, 
														  const DECUMA_UNICODE * pUnicodes, 
														  DECUMA_UINT8 nUnicodes);

DECUMA_HWR_PRIVATE DECUMA_UINT8 decumaDictionaryGetSubTreeFreqClassGivenMinMax(const DECUMA_HWR_DICTIONARY * pDictionary,
	                                                    DECUMA_HWR_DICTIONARY_REF currentRef,
														DECUMA_UINT8 minRem, DECUMA_UINT8 maxRem);


#endif /* DECUMA_DICTIONARY_H */
