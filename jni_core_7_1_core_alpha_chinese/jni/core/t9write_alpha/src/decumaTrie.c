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


/* A basic trie, for use with dynamic generation of Decuma-style dictionares 
 * from standard eZiText Z8D files. 
 * 
 * This is intended to be used only dynamically, i.e. it is not designed to be
 * restored from a previous session. That gives us some leeway to use pointers
 * instead of pointer-diffs, reducing execution time by almost 50% for searches
 * in the trie. 
 * Each child or sibling dereference is directly a pointer instead of a 
 * pointer + (offset * sizeof(node)). Since child/sibling dereferences dominate
 * other operations done when searching, reducing them by two thirds gives a 
 * pretty good impact. We need a 32-bit value for child/sibling anyway, since 
 * we have more than 0xFFFF nodes in a typical trie.
 * 
 * TODOs: Allow words to be removed from the trie
 *        Create DAWG(?)
 */
#define DECUMA_TRIE_C

#include "decumaTrie.h"
#include "decumaTrieData.h"
#include "decumaDictionaryBinary.h"
#include "decumaAssert.h"
#include "decumaRuntimeMalloc.h"
#include "decumaMemory.h"
#include "decumaBasicTypesMinMax.h"

#include <math.h>

static DECUMA_UINT8 getFreqClass(DECUMA_TRIE_HEADER * pTrieHeader, DECUMA_UINT32 wordIdx);

static DECUMA_TRIE_NODE * allocNode( DECUMA_TRIE_HEADER * pTrieHeader);

static DECUMA_STATUS copyWord( DECUMA_TRIE_HEADER * pTrieHeader,
								 const DECUMA_UNICODE * pWordStart,
								 const DECUMA_UNICODE * pWordEnd,
								 const DECUMA_UINT32     wordIdx);	


/*//////////////// External functions ///////////////////// */

DECUMA_UINT32 decumaTrieGetSize(const DECUMA_TRIE * pTrie)
{
	/*Note that this does not return the size of the header only but all the allocated memory */
	return sizeof(DECUMA_TRIE) + 
		pTrie->trieHeader.nMaxFreqClasses * sizeof(DECUMA_UINT32) + 
		pTrie->trieHeader.nNodes * sizeof(DECUMA_TRIE_NODE) + 
		pTrie->trieHeader.nNodeBuffers *  sizeof(DECUMA_TRIE_NODE*);
}

DECUMA_STATUS decumaTrieCreate(DECUMA_TRIE ** ppTrie, DECUMA_UINT32 nNodeBufferSize, 
							   DECUMA_UINT32 * pFreqClassTable, DECUMA_UINT8 nMaxFreqClasses,
							   const DECUMA_MEM_FUNCTIONS * pMemFunctions)
{
	DECUMA_TRIE_HEADER * pTrieHeader;
	DECUMA_TRIE * pTrie=NULL;
	DECUMA_UINT32 * pFreqClassTableCpy=NULL;
	DECUMA_TRIE_NODE** ppNodeBuffer=NULL;
	DECUMA_TRIE_NODE* pNodeBuffer=NULL;
	DECUMA_STATUS status=decumaNoError;

	decumaAssert(pFreqClassTable);

	*ppTrie = NULL;

	pTrie = decumaModCalloc(MODID_DICTIONARY,1, sizeof(DECUMA_TRIE));
	if ( !pTrie )
	{
		status=decumaAllocationFailed;
		goto decumaCreateTrie_failure;
	}

	/*Initialize the binary header */
	decumaDictionaryBinaryInitHeader((DECUMA_DICTIONARY_BINARY_HEADER*) &pTrie->binaryHeader,
		decumaDictionaryBinaryStructureTrie);

	/*Initialize the trie */
	pTrieHeader = &pTrie->trieHeader;

	pTrieHeader->nMin = 0xFF;
	pTrieHeader->nFC0Min = 0xFF;
	pTrieHeader->nMax = 0;
	pTrieHeader->nFC0Max = 0;
	pTrieHeader->nFreqClasses = 0;
	pTrieHeader->nMaxFreqClasses = nMaxFreqClasses;
	pTrieHeader->nNodeBufferSize = nNodeBufferSize;
	pTrieHeader->nNodesInNewNodeBuf = 0;
	pTrieHeader->nNodesInOldNodeBuf = nNodeBufferSize; /*Says that the preivous (non-existing) buffer was filled */

	pFreqClassTableCpy = decumaModCalloc(MODID_DICTIONARY,nMaxFreqClasses, sizeof(DECUMA_UINT32));
	if ( !pFreqClassTableCpy )
	{
		status=decumaAllocationFailed;
		goto decumaCreateTrie_failure;
	}
	decumaMemcpy(pFreqClassTableCpy, pFreqClassTable, sizeof(*pFreqClassTable)*nMaxFreqClasses);
	
	ppNodeBuffer = decumaModCalloc(MODID_DICTIONARY,1, sizeof(DECUMA_TRIE_NODE*)); /*Start with only one buffer */
	if ( !ppNodeBuffer )
	{
		status=decumaAllocationFailed;
		goto decumaCreateTrie_failure;
	}

	pNodeBuffer = decumaModCalloc(MODID_DICTIONARY,nNodeBufferSize, sizeof(DECUMA_TRIE_NODE));
	if ( !pNodeBuffer )
	{
		status=decumaAllocationFailed;
		goto decumaCreateTrie_failure;
	}

	ppNodeBuffer[0] = pNodeBuffer;	
	pTrieHeader->ppNodeBuffers = ppNodeBuffer;
	pTrieHeader->nNodeBuffers = 1;
	pTrieHeader->pFreqClassTable = pFreqClassTableCpy;
	
	*ppTrie = pTrie;

	return decumaNoError;

decumaCreateTrie_failure:
	if (ppNodeBuffer) decumaModFree(MODID_DICTIONARY,ppNodeBuffer);
	if (pNodeBuffer) decumaModFree(MODID_DICTIONARY,pNodeBuffer);
	if (pFreqClassTableCpy) decumaModFree(MODID_DICTIONARY,pFreqClassTableCpy);
	if (pTrie) decumaModFree(MODID_DICTIONARY,pTrie);
	return status;

}

void decumaTrieDestroy(DECUMA_TRIE ** ppTrie,
					   const DECUMA_MEM_FUNCTIONS * pMemFunctions)
{
	int i;
	if (!(*ppTrie)) return; /*Nothing to free */

	decumaModFree(MODID_DICTIONARY,(*ppTrie)->trieHeader.pFreqClassTable);
	for (i=0; i<(*ppTrie)->trieHeader.nNodeBuffers; i++)
	{
		decumaModFree(MODID_DICTIONARY,(*ppTrie)->trieHeader.ppNodeBuffers[i]);
	}
	decumaModFree(MODID_DICTIONARY,(*ppTrie)->trieHeader.ppNodeBuffers);
	decumaModFree(MODID_DICTIONARY,*ppTrie);
	*ppTrie=NULL;
}

DECUMA_STATUS decumaTrieAddWord ( DECUMA_TRIE * pTrie, const DECUMA_UNICODE * pWord, DECUMA_UINT32 wordIdx )
{
	const DECUMA_UNICODE * pZero;
	decumaAssert(pTrie);
	decumaAssert(pWord);
	for ( pZero = pWord; *pZero; pZero++);
	
	return decumaTrieCopyWord(pTrie, pWord, pZero, wordIdx);
}


DECUMA_STATUS decumaTrieCopyWord( DECUMA_TRIE * pTrie,
								 const DECUMA_UNICODE * pWordStart,
								 const DECUMA_UNICODE * pWordEnd,
								 DECUMA_UINT32     wordIdx)
{
	DECUMA_STATUS status;
	decumaAssert(pTrie);
	decumaAssert(pWordStart);
	decumaAssert(pWordEnd);
	
	/*status = checkAvailableSpace(&pTrie->trieHeader, pWordStart, pWordEnd); */

	/*if ( status == decumaNoError ) */
		status = copyWord(&pTrie->trieHeader, pWordStart, pWordEnd, wordIdx);
	
	return status;
}

DECUMA_STATUS decumaTrieAddNodeBuffer(DECUMA_TRIE * pTrie, const DECUMA_MEM_FUNCTIONS * pMemFunctions)
{
	DECUMA_TRIE_HEADER * pTrieHeader = &pTrie->trieHeader;
	DECUMA_TRIE_NODE ** ppNodeBuffers = NULL;
	DECUMA_TRIE_NODE * pNewNodeBuffer = NULL;
	DECUMA_STATUS status;
	
	ppNodeBuffers = decumaModCalloc(MODID_DICTIONARY,1, (pTrieHeader->nNodeBuffers+1)*sizeof(ppNodeBuffers[0]));
	if (!ppNodeBuffers) 
	{ 
		status = decumaAllocationFailed;
		goto decumaTrieAddNodeBuffer_error;
	}
	
	decumaMemcpy(ppNodeBuffers,pTrieHeader->ppNodeBuffers,pTrieHeader->nNodeBuffers*sizeof(ppNodeBuffers[0]));

	pNewNodeBuffer= decumaModCalloc(MODID_DICTIONARY,1, pTrieHeader->nNodeBufferSize*sizeof(DECUMA_TRIE_NODE));
	if (!pNewNodeBuffer) 
	{ 
		status = decumaAllocationFailed;
		goto decumaTrieAddNodeBuffer_error;
	}

	ppNodeBuffers[pTrieHeader->nNodeBuffers] = pNewNodeBuffer;

	decumaModFree(MODID_DICTIONARY,pTrieHeader->ppNodeBuffers);
	pTrieHeader->ppNodeBuffers = ppNodeBuffers;
	pTrieHeader->nNodeBuffers++;
	pTrieHeader->nNodesInOldNodeBuf = pTrieHeader->nNodesInNewNodeBuf;
	pTrieHeader->nNodesInNewNodeBuf = 0;
	
	return decumaNoError;

decumaTrieAddNodeBuffer_error:

	if (pNewNodeBuffer) decumaModFree(MODID_DICTIONARY,pNewNodeBuffer);

	if (ppNodeBuffers) decumaModFree(MODID_DICTIONARY,ppNodeBuffers);

	return status;

}


/* Access functions for creating a DECUMA_HWR_DICTIONARY structure of a standard DECUMA_TRIE */

DECUMA_TRIE_NODE_REF decumaTrieGetSibling(DECUMA_TRIE * pTrie, DECUMA_TRIE_NODE_REF currentRef)
{
	return ((DECUMA_TRIE_NODE *)currentRef)->pSibling;
}


DECUMA_TRIE_NODE_REF decumaTrieGetChild(DECUMA_TRIE * pTrie,  DECUMA_TRIE_NODE_REF currentRef)
{
	if ( currentRef )
		return ((DECUMA_TRIE_NODE *) currentRef)->pChild;
	else
		return pTrie->trieHeader.pFirst;
}

void decumaTrieGetMinMax(const DECUMA_TRIE * pTrie,  DECUMA_TRIE_NODE_REF currentRef, DECUMA_UINT8 * pMin, DECUMA_UINT8 * pMax)
{
	const DECUMA_TRIE_HEADER * pTrieHeader = &pTrie->trieHeader;
	if ( currentRef == NULL ) 
	{
		if ( pMin ) *pMin = pTrieHeader->nMin;
		if ( pMax ) *pMax = pTrieHeader->nMax;
	} 
	else
	{
		if ( pMin ) *pMin = ((DECUMA_TRIE_NODE *) currentRef)->nMin;
		if ( pMax ) *pMax = ((DECUMA_TRIE_NODE *) currentRef)->nMax;
	}
}

DECUMA_UNICODE decumaTrieGetUnicode(const DECUMA_TRIE * pTrie, DECUMA_TRIE_NODE_REF currentRef)
{
	return ((DECUMA_TRIE_NODE *) currentRef)->unicode;
}

int decumaTrieGetEndOfWord(const DECUMA_TRIE * pTrie, DECUMA_TRIE_NODE_REF currentRef)
{
	return ((DECUMA_TRIE_NODE *) currentRef)->nMin == 0;
}

DECUMA_UINT8 decumaTrieGetNFreqClasses(const DECUMA_TRIE * pTrie)
{
	return pTrie->trieHeader.nFreqClasses;
}

void decumaTrieGetRankInFreqClass(const DECUMA_TRIE * pTrie, DECUMA_UINT8 freqClass,
								 DECUMA_UINT32 * pMinRank, DECUMA_UINT32 * pMaxRank)
{
	const DECUMA_TRIE_HEADER * pTrieHeader = &pTrie->trieHeader;
	/*Asserts on parameters have already been done in decumaDictionary */

	*pMinRank = pTrieHeader->pFreqClassTable[freqClass];
	*pMaxRank  =  (freqClass >= pTrieHeader->nFreqClasses-1 ? pTrieHeader->nWords : pTrieHeader->pFreqClassTable[freqClass+1])-1;

	/*If we have more freq classes than words to fill them... */
	if (*pMinRank > pTrieHeader->nWords) *pMinRank = pTrieHeader->nWords-1;
	if (*pMaxRank > pTrieHeader->nWords) *pMaxRank = pTrieHeader->nWords-1;
}

DECUMA_UINT8 decumaTrieGetSubTreeFreqClass(const DECUMA_TRIE * pTrie, DECUMA_TRIE_NODE_REF currentRef)
{
	DECUMA_TRIE_NODE * pNode = currentRef;
	if (!pNode) return 0; /*Return class 0 for root */

	return pNode->bestFreqClass;
}


void decumaTrieGetSubTreeFreqClassMinMax(const DECUMA_TRIE * pTrie, DECUMA_TRIE_NODE_REF currentRef,
	DECUMA_UINT8 * pMin, DECUMA_UINT8 * pMax)
{
	decumaAssert(pMin);
	decumaAssert(pMax);

	if (!currentRef) 
	{
		*pMin = pTrie->trieHeader.nFC0Min;
		*pMax = pTrie->trieHeader.nFC0Max;
	} 
	else
	{
		*pMin = currentRef->bestFCMin;
		*pMax = currentRef->bestFCMax;
	}
}

DECUMA_UINT8 decumaTrieGetWordFreqClass(const DECUMA_TRIE * pTrie, DECUMA_TRIE_NODE_REF currentRef)
{
	DECUMA_TRIE_NODE * pNode = currentRef;

	if (!currentRef) return MAX_DECUMA_UINT8;

	return pNode->thisFreqClass;
}

DECUMA_UINT32 decumaTrieGetNWords(const DECUMA_TRIE * pTrie)
{
	return pTrie->trieHeader.nWords;
}

DECUMA_TRIE_ITERATOR * decumaTrieIteratorCreate(const DECUMA_TRIE * pTrie, 
												const DECUMA_MEM_FUNCTIONS * pMemFunctions)
{
	DECUMA_TRIE_ITERATOR * pIterator;
	const DECUMA_TRIE_HEADER * pTrieHeader = &pTrie->trieHeader;
	decumaAssert(pTrie);

	if (pTrieHeader->nNodes == 0) return NULL;
	
	decumaAssert(pTrieHeader->nNodeBuffers);

	pIterator = decumaModCalloc(MODID_DICTIONARY,1,sizeof(DECUMA_TRIE_ITERATOR));
	
	if (!pIterator) return NULL;

	pIterator->pTrie = pTrie;
	pIterator->nodeBufferIdx = 0;
	pIterator->nodeIdx= 0;

	return pIterator;
}

void decumaTrieIteratorDestroy(DECUMA_TRIE_ITERATOR * pIterator,
							   const DECUMA_MEM_FUNCTIONS * pMemFunctions)
{
	decumaModFree(MODID_DICTIONARY,pIterator);
}

DECUMA_TRIE_NODE_REF decumaTrieIteratorGetNext(DECUMA_TRIE_ITERATOR * pIterator)
{
	const DECUMA_TRIE_HEADER * pTrieHeader = &pIterator->pTrie->trieHeader;
	decumaAssert(pIterator);

	if (pIterator->nodeIdx<pTrieHeader->nNodes)
	{
		DECUMA_TRIE_NODE * pNode;

		decumaAssert(pIterator->nodeBufferIdx<pTrieHeader->nNodeBuffers);
		decumaAssert(pIterator->nodeIdxInBuffer<pTrieHeader->nNodeBufferSize);
		
		pNode=pTrieHeader->ppNodeBuffers[pIterator->nodeBufferIdx];
		pNode += pIterator->nodeIdxInBuffer;

		pIterator->nodeIdxInBuffer++;
		pIterator->nodeIdx++;
		if (pIterator->nodeIdxInBuffer == pTrieHeader->nNodeBufferSize)
		{
			/*Next buffer */
			pIterator->nodeIdxInBuffer=0;
			pIterator->nodeBufferIdx++;
		}
		
		return pNode;
	}
	else
	{
		/*No more nodes to iterate through */
		return NULL;
	}
}



/*------------------------------------------------------------ */

static DECUMA_STATUS copyWord( DECUMA_TRIE_HEADER * pTrieHeader,
							 const DECUMA_UNICODE * pWordStart,
							 const DECUMA_UNICODE * pWordEnd,
							 const DECUMA_UINT32     wordIdx)
{
	int i, nCharsToGo;
	DECUMA_TRIE_NODE * pNode, * pPrevSibling, * pParent;
	DECUMA_UINT8 wordFreqClass = getFreqClass(pTrieHeader, wordIdx);
	int bAlreadyExisting=0;

	decumaAssert(pTrieHeader);
	decumaAssert(pTrieHeader->nNodes >= 0);
	decumaAssert(pWordStart);
	decumaAssert(pWordEnd);
	decumaAssert(pWordStart < pWordEnd);

	nCharsToGo = pWordEnd - pWordStart;
	if (nCharsToGo > DECUMA_TRIE_MAX_DEPTH)
	{
		/*The word if longer than can be supported by this format. */
		/*Either the dictionary is invalid or we need to update the trie format to */
		/*support that long words */
		decumaAssert(0);
		return decumaInvalidDictionary;
	}


	/*Assume the worst case that each character adds a new node */
	if ( pTrieHeader->nNodesInNewNodeBuf + nCharsToGo >= pTrieHeader->nNodeBufferSize ) 
		return decumaTooShortBuffer;

	if (wordFreqClass > pTrieHeader->nFreqClasses-1)
	{
		pTrieHeader->nFreqClasses++;
		decumaAssert(pTrieHeader->nFreqClasses<=pTrieHeader->nMaxFreqClasses);
	}

	pNode = pTrieHeader->pFirst;

	/* Table min/max adjustment */
	if( pTrieHeader->nMin > nCharsToGo ) pTrieHeader->nMin = nCharsToGo;
	if( pTrieHeader->nMax < nCharsToGo ) pTrieHeader->nMax = nCharsToGo;

	if (wordFreqClass == 0)
	{
		if( pTrieHeader->nFC0Min > nCharsToGo ) pTrieHeader->nFC0Min = nCharsToGo;
		if( pTrieHeader->nFC0Max < nCharsToGo ) pTrieHeader->nFC0Max = nCharsToGo;
	}

	nCharsToGo -= 1;

	pParent = NULL;
	pPrevSibling = NULL;

	/* Traverse the current trie as far as possible. This updates the trie as we go, so 
	 * it is assumed that the word will fit (check with checkAvailableSpace() in external functions)
	 */
	for( i = 0; nCharsToGo >= 0 && pNode; i++ )
	{
		while( pNode && pNode->unicode < pWordStart[i] )
		{
			pPrevSibling = pNode;
			pNode = pNode->pSibling;
			decumaAssert(pNode == NULL || pNode->unicode > pPrevSibling->unicode );
		}

		if( !pNode || pNode->unicode != pWordStart[i] )
		{
			break;
		}

		/* Check best upcoming word count */
		/* When we know that each new word has higher count than any previously added, this is redundant. */
		if ( pNode->bestFreqClass > wordFreqClass ) 
		{
			pNode->bestFreqClass = wordFreqClass;
			pNode->bestFCMin = pNode->bestFCMax = nCharsToGo;
		} 
		else if ( pNode->bestFreqClass == wordFreqClass ) 
		{
			if (pNode->bestFCMin > nCharsToGo) pNode->bestFCMin = nCharsToGo;
			if (pNode->bestFCMax < nCharsToGo) pNode->bestFCMax = nCharsToGo;
		}

		if ( nCharsToGo == 0)
		{
			/*This is the end of a word			 */
			if (pNode->nMin == 0)
			{
				/*We have already added this word */
				decumaAssert(pNode->thisFreqClass!=MAX_DECUMA_UINT8);
				bAlreadyExisting = 1;
			}

			/*Set the freq class for this complete word different than the best of the subtree */
			/*Compare "yo" and "you" in English */
			if ( pNode->thisFreqClass > wordFreqClass ) 
			{
				pNode->thisFreqClass = wordFreqClass; 
			}
		}

		/* Node min/max adjustment */
		if( pNode->nMin > nCharsToGo ) pNode->nMin = nCharsToGo;
		if( pNode->nMax < nCharsToGo ) pNode->nMax = nCharsToGo;



		pParent = pNode;
		pPrevSibling = NULL;

		pNode = pNode->pChild;
		nCharsToGo--;
	}

	if ( nCharsToGo < 0 ) 
	{
		/* We added an already existing word (or a substring thereof) */
		goto copyWord_wordAdditionDone;
	}

	/* Right here, we have traversed the current trie as far as possible. Now we 
	 * can find the proper place to insert the new subtree, such that we get an 
	 * ordered trie, i.e. with siblings sorted by unicode values
	 *  Because of how the trie is implemented, we have two basic cases:
	 *   -> New subtree before the first existing sibling => modify the parent child pointer
	 *   -> New subtree between two existing siblings => modify the preceeding siblings' sibling pointer
	 */

	{ 
		DECUMA_TRIE_NODE * newNode = allocNode(pTrieHeader);
		decumaAssert(newNode);

		newNode->unicode = pWordStart[i];
		newNode->nMax = nCharsToGo;
		newNode->nMin = nCharsToGo;
		newNode->bestFreqClass = wordFreqClass;
		newNode->bestFCMin = newNode->bestFCMax = nCharsToGo;
		newNode->thisFreqClass = nCharsToGo==0 ? wordFreqClass : MAX_DECUMA_UINT8;
	
		decumaAssert(newNode->pChild == NULL);
		decumaAssert(newNode->pSibling == NULL);

		if ( pPrevSibling == NULL )
		{
			/* No previous sibling; we put the new node first in the sibling list. 
			 * pNode is the current child to pParent (i.e. current first sibling in the sibling list)
			 */

			/* Special case: If we have no parent here, we're adding something that should be put first in the trie */
			if ( pParent == NULL )
			{
				decumaAssert(pTrieHeader->pFirst == pNode);
				pTrieHeader->pFirst = newNode;
			} 
			else 
			{
				decumaAssert( pParent->pChild == pNode );
				pParent->pChild = newNode;
			}

			decumaAssert( pNode == NULL || pNode->unicode > newNode->unicode );
			newNode->pSibling = pNode; /* Might be NULL, but that's ok. */
		}
		else
		{
			/* There is a previous sibling, insert the new node between pPrevSibling and its current sibling, pNode
			 */
			decumaAssert( pPrevSibling->unicode < newNode->unicode);
			decumaAssert( pNode == NULL || pNode->unicode > newNode->unicode );
			pPrevSibling->pSibling = newNode;
			newNode->pSibling = pNode; /* Might be NULL, but that's ok. */
		}

		/* New node becomes parent for the new subtree */
		pParent = newNode; 
		i++;
		nCharsToGo--;
	}

	decumaAssert(pParent != NULL);

	/* Start adding nodes to the new subtree */
	for( ; nCharsToGo >= 0; nCharsToGo--, i++ )
	{
		pNode = allocNode( pTrieHeader );

		/* With the fast-fail detection, we can assume that allocNode always 
		 * succeeds. 
		 * If it does fail, we should suspect some concurrency issue, or an 
		 * illegal write to pTrieHeader->nNodes.
		 */
		decumaAssert(pNode);

		/* Initialize node */
		pNode->unicode = pWordStart[i];
		pNode->nMax = nCharsToGo;
		pNode->nMin = nCharsToGo;
		pNode->bestFreqClass = wordFreqClass;
		pNode->bestFCMin = pNode->bestFCMax = nCharsToGo;
		pNode->thisFreqClass = nCharsToGo==0 ? wordFreqClass : MAX_DECUMA_UINT8;

		/* Connect node to parent, and become parent for the subsequent nodes */
		pParent->pChild = pNode;
		pParent = pNode;
	}

copyWord_wordAdditionDone:

	if (!bAlreadyExisting)
	{
		pTrieHeader->nWords++;
	}

	return decumaNoError;
}

static DECUMA_TRIE_NODE * allocNode( DECUMA_TRIE_HEADER * pTrieHeader )
{
	decumaAssert(pTrieHeader);

	/* TODO: Allow deletions in the trie? If nodes have been deleted, the trie
	*       might be fragmented, so search for next free node:
	*
	*       if (pTrieHeader->isFragmented)
	*       {
	*           TRIE_NODE * pNode = &pTrieHeader[1];
	*           int i = 0;
	*           for ( ; i < pTrieHeader->nNodes; i++ )
	*           {
	*               if ( pNode[i].unicode == 0xFFFF )
	*                   break;
	*           }
	*           pTrieHeader->isFragmented = 0;
	*           return &pNode[i];
	*       }
	*/

	if( pTrieHeader->nNodesInNewNodeBuf >= pTrieHeader->nNodeBufferSize )
	{
		return NULL;
	}
	else
	{
		DECUMA_TRIE_NODE * pNode; 
		DECUMA_TRIE_NODE * pNodeBuf;
		if (pTrieHeader->nNodesInOldNodeBuf < pTrieHeader->nNodeBufferSize)
		{
			/*The second last buffer was not yet completely filled */
			pNodeBuf = pTrieHeader->ppNodeBuffers[pTrieHeader->nNodeBuffers-2];
			pNode = &pNodeBuf[pTrieHeader->nNodesInOldNodeBuf++];
		}
		else
		{
			pNodeBuf = pTrieHeader->ppNodeBuffers[pTrieHeader->nNodeBuffers-1];
			pNode = &pNodeBuf[pTrieHeader->nNodesInNewNodeBuf++];
		}

		pTrieHeader->nNodes++;
		pNode->nMin = 0xFF;
		return pNode;
	}
}

/* Function that estimates a "FreqClass" value from a word index */
static DECUMA_UINT8 getFreqClass(DECUMA_TRIE_HEADER * pTrieHeader, DECUMA_UINT32 wordIdx) 
{
	DECUMA_UINT8 freqClass=0;
	while (freqClass < pTrieHeader->nMaxFreqClasses-1 && pTrieHeader->pFreqClassTable[freqClass+1]<=wordIdx) freqClass++;

	return freqClass;
}
