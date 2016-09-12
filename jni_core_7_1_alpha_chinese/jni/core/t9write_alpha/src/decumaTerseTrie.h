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

\******************************************************************/

#ifndef DECUMA_TERSE_TRIE_H_
#define DECUMA_TERSE_TRIE_H_

#include "decumaBasicTypes.h"
#include "decumaTrie.h"

#ifdef DECUMA_DICTIONARY_C
typedef void DECUMA_TERSE_TRIE;
typedef void * DECUMA_TERSE_TRIE_NODE_REF;
#else
typedef struct _DECUMA_TERSE_TRIE DECUMA_TERSE_TRIE;
typedef struct _DECUMA_TERSE_TRIE_NODE DECUMA_TERSE_TRIE_NODE;
typedef DECUMA_TERSE_TRIE_NODE * DECUMA_TERSE_TRIE_NODE_REF;
#endif
typedef struct _DECUMA_TERSE_TRIE_DATA DECUMA_TERSE_TRIE_DATA;



DECUMA_HWR_PRIVATE DECUMA_STATUS decumaTerseTrieCreateFromTrie(DECUMA_TERSE_TRIE ** ppTerseTrie,
											const DECUMA_TRIE * pTrie,
											const DECUMA_MEM_FUNCTIONS * pMemFunctions);

DECUMA_HWR_PRIVATE void decumaTerseTrieDestroy(DECUMA_TERSE_TRIE **  ppTerseTrie,
							const DECUMA_MEM_FUNCTIONS * pMemFunctions);


/*Access functions used by DECUMA_HWR_DICTIONARY ----- */
DECUMA_HWR_PRIVATE DECUMA_TERSE_TRIE_NODE_REF decumaTerseTrieGetSibling(DECUMA_TERSE_TRIE * pDictionary,
											  DECUMA_TERSE_TRIE_NODE_REF currentRef);

DECUMA_HWR_PRIVATE DECUMA_TERSE_TRIE_NODE_REF decumaTerseTrieGetChild(DECUMA_TERSE_TRIE * pDictionary,
											DECUMA_TERSE_TRIE_NODE_REF currentRef);

DECUMA_HWR_PRIVATE void decumaTerseTrieGetMinMax(const DECUMA_TERSE_TRIE * pDictionary,
						DECUMA_TERSE_TRIE_NODE_REF currentRef, 
						DECUMA_UINT8 * pMin, DECUMA_UINT8 * pMax);

DECUMA_HWR_PRIVATE DECUMA_UNICODE decumaTerseTrieGetUnicode(const DECUMA_TERSE_TRIE * pDictionary,DECUMA_TERSE_TRIE_NODE_REF currentRef);

DECUMA_HWR_PRIVATE int decumaTerseTrieGetEndOfWord(const DECUMA_TERSE_TRIE * pDictionary,DECUMA_TERSE_TRIE_NODE_REF currentRef);

DECUMA_HWR_PRIVATE DECUMA_UINT8 decumaTerseTrieGetNFreqClasses(const DECUMA_TERSE_TRIE * pDictionary);

DECUMA_HWR_PRIVATE void decumaTerseTrieGetRankInFreqClass(const DECUMA_TERSE_TRIE * pDictionary, DECUMA_UINT8 freqClass,
												DECUMA_UINT32 * pMinRank, DECUMA_UINT32 * pMaxRank);

DECUMA_HWR_PRIVATE DECUMA_UINT8 decumaTerseTrieGetSubTreeFreqClass(const DECUMA_TERSE_TRIE * pDictionary, DECUMA_TERSE_TRIE_NODE_REF currentRef);
DECUMA_HWR_PRIVATE DECUMA_UINT8 decumaTerseTrieGetWordFreqClass(const DECUMA_TERSE_TRIE * pDictionary, DECUMA_TERSE_TRIE_NODE_REF currentRef);
DECUMA_HWR_PRIVATE void decumaTerseTrieGetSubTreeFreqClassMinMax(const DECUMA_TERSE_TRIE * pTrie, DECUMA_TERSE_TRIE_NODE_REF currentRef,
												  DECUMA_UINT8 * pMin, DECUMA_UINT8 * pMax);
DECUMA_HWR_PRIVATE DECUMA_UINT32 decumaTerseTrieGetSize(const DECUMA_TERSE_TRIE * pDictionary);

DECUMA_HWR_PRIVATE DECUMA_UINT32 decumaTerseTrieGetNWords(const DECUMA_TERSE_TRIE * pDictionary);
/*End of access functions used by DECUMA_HWR_DICTIONARY ----- */

/*Extra access functions for this data type */
DECUMA_HWR_PRIVATE DECUMA_UINT32 decumaTerseTrieGetNNodes(const DECUMA_TERSE_TRIE * pTerseTrie);
DECUMA_HWR_PRIVATE DECUMA_UINT32 decumaTerseTrieGetNDataElems(const DECUMA_TERSE_TRIE * pTerseTrie);


#endif /*DECUMA_TERSE_TRIE_H_ */

