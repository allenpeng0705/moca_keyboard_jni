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

  This file describes the interface to a dictionary that is represented
  by a trie. (Tree) 
    
\******************************************************************/

#ifndef DECUMA_TRIE_H
#define DECUMA_TRIE_H

#include "decumaConfig.h"
#include "decumaBasicTypes.h" /* Definition of DECUMA_UINT32 */
#include "decuma_hwr.h"		  /* Definition of DECUMA_UNICODE */

#ifdef DECUMA_DICTIONARY_C
typedef void DECUMA_TRIE;
typedef void * DECUMA_TRIE_NODE_REF;
#else
typedef struct _DECUMA_TRIE DECUMA_TRIE;
typedef struct _DECUMA_TRIE_NODE DECUMA_TRIE_NODE;
typedef DECUMA_TRIE_NODE * DECUMA_TRIE_NODE_REF; /*This needs to be void to allow compatibility with the typedefs in decumaDictionary.h */
#endif
typedef struct _DECUMA_TRIE_ITERATOR DECUMA_TRIE_ITERATOR;

DECUMA_HWR_PRIVATE DECUMA_STATUS decumaTrieCreate(DECUMA_TRIE ** ppTrie, DECUMA_UINT32 nNodeBufferSize,
							   DECUMA_UINT32 * pFreqClassTable, DECUMA_UINT8 nFreqClasses,
							   const DECUMA_MEM_FUNCTIONS * pMemFunctions);

DECUMA_HWR_PRIVATE void decumaTrieDestroy(DECUMA_TRIE ** ppTrie,const DECUMA_MEM_FUNCTIONS * pMemFunctions);

DECUMA_HWR_PRIVATE DECUMA_STATUS decumaTrieAddNodeBuffer(DECUMA_TRIE *pTrie,
									  const DECUMA_MEM_FUNCTIONS * pMemFunctions);

DECUMA_HWR_PRIVATE DECUMA_STATUS decumaTrieAddWord( DECUMA_TRIE * pTrie,
								 const DECUMA_UNICODE * pWord,
								 DECUMA_UINT32 wordIdx);

DECUMA_HWR_PRIVATE DECUMA_STATUS decumaTrieCopyWord( DECUMA_TRIE * pTrie,
								  const DECUMA_UNICODE * pWordStart,
								  const DECUMA_UNICODE * pWordEnd,
  								  DECUMA_UINT32 wordIdx);


/* Access functions for creating a DECUMA_HWR_DICTIONARY structure of a standard DECUMA_TRIE */
DECUMA_HWR_PRIVATE DECUMA_TRIE_NODE_REF decumaTrieGetSibling(DECUMA_TRIE * pTrie, DECUMA_TRIE_NODE_REF currentRef);

DECUMA_HWR_PRIVATE DECUMA_TRIE_NODE_REF decumaTrieGetChild(DECUMA_TRIE * pTrie,  DECUMA_TRIE_NODE_REF currentRef);

DECUMA_HWR_PRIVATE void decumaTrieGetMinMax(const DECUMA_TRIE * pTrie,  DECUMA_TRIE_NODE_REF currentRef, DECUMA_UINT8 * pMin, DECUMA_UINT8 * pMax);

DECUMA_HWR_PRIVATE DECUMA_UNICODE decumaTrieGetUnicode(const DECUMA_TRIE * pTrie, DECUMA_TRIE_NODE_REF currentRef);

DECUMA_HWR_PRIVATE int decumaTrieGetEndOfWord(const DECUMA_TRIE * pTrie, DECUMA_TRIE_NODE_REF currentRef);

DECUMA_HWR_PRIVATE DECUMA_UINT8 decumaTrieGetNFreqClasses(const DECUMA_TRIE * pTrie);

DECUMA_HWR_PRIVATE void decumaTrieGetRankInFreqClass(const DECUMA_TRIE * pTrie, DECUMA_UINT8 freqClass,
										   DECUMA_UINT32 * pMinRank, DECUMA_UINT32 * pMaxRank);

DECUMA_HWR_PRIVATE DECUMA_UINT8 decumaTrieGetSubTreeFreqClass(const DECUMA_TRIE * pTrie, DECUMA_TRIE_NODE_REF currentRef);
DECUMA_HWR_PRIVATE void decumaTrieGetSubTreeFreqClassMinMax(const DECUMA_TRIE * pTrie, DECUMA_TRIE_NODE_REF currentRef,
												  DECUMA_UINT8 * pMin, DECUMA_UINT8 * pMax);
DECUMA_HWR_PRIVATE DECUMA_UINT8 decumaTrieGetWordFreqClass(const DECUMA_TRIE * pTrie, DECUMA_TRIE_NODE_REF currentRef);

DECUMA_HWR_PRIVATE DECUMA_UINT32 decumaTrieGetSize(const DECUMA_TRIE * pTrie);

DECUMA_HWR_PRIVATE DECUMA_UINT32 decumaTrieGetNWords(const DECUMA_TRIE * pTrie);

/*------------- Functions for traversing a trie in a flat manner --- */
DECUMA_HWR_PRIVATE DECUMA_TRIE_ITERATOR * decumaTrieIteratorCreate(const DECUMA_TRIE * pTrie,
	const DECUMA_MEM_FUNCTIONS * pMemFunctions);

DECUMA_HWR_PRIVATE void decumaTrieIteratorDestroy(DECUMA_TRIE_ITERATOR * pIterator,
							   const DECUMA_MEM_FUNCTIONS * pMemFunctions);

DECUMA_HWR_PRIVATE DECUMA_TRIE_NODE_REF decumaTrieIteratorGetNext(DECUMA_TRIE_ITERATOR * pIterator);



#endif /* #ifdef DECUMA_TRIE_H */
