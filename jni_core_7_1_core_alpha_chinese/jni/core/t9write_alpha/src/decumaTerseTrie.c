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

#define DECUMA_TERSE_TRIE_C

#include "decumaTerseTrie.h"
#include "decumaTerseTrieData.h"
#include "decumaTrie.h"
#include "decumaTrieData.h"  /*We have an exception to look at the data structure also from this file for  */
							 /*abnormal access when packing to a terse trie */
#include "decumaDictionaryBinary.h"

#include "decumaHashHandler.h"
#include "decumaAssert.h"
#include "decumaMemory.h"
#include "decumaRuntimeMalloc.h"
#include "decumaBasicTypesMinMax.h"
#include "decumaConfig.h"

#define DECUMA_TERSE_TRIE_MAX_DEPTH 64 /* This is set to 64 as this currently accomodates the max value for word lengths */
                                       /* from the XT9 databases. Compare with the value ET9MAXWORDSIZE in decumaDictionaryXT9.c */

#define OFFSET_BETWEEN(ptr1,ptr0) ((DECUMA_UINT32) ((char*)(ptr1) - (char*)(ptr0)))
#define CONVERT_TO_PTR(ptr_type,offset,ptr0) ((ptr_type) ((char*)(ptr0)+(offset)))

static int equals(const DECUMA_TRIE_NODE * a, const DECUMA_TRIE_NODE * b);

static int terse_equals(const DECUMA_TRIE_NODE * a, const DECUMA_TERSE_TRIE_DATA * b);

/*/////////////////////////////////////////// */

DECUMA_STATUS decumaTerseTrieCreateFromTrie(DECUMA_TERSE_TRIE ** ppTerseTrie,
											const DECUMA_TRIE * pTrie,
											const DECUMA_MEM_FUNCTIONS * pMemFunctions)
{
	DECUMA_STATUS status =decumaNoError;
	DECUMA_TERSE_TRIE_NODE * pTerseNode;
	const DECUMA_TRIE_NODE * stack[DECUMA_TERSE_TRIE_MAX_DEPTH];
	DECUMA_TERSE_TRIE_NODE * terseStack[DECUMA_TERSE_TRIE_MAX_DEPTH];
	const DECUMA_TRIE_HEADER * pTrieHeader = &pTrie->trieHeader;
	const DECUMA_TRIE_NODE * pTrieNode;

	int n = 0;

	DECUMA_HH * indexHash=NULL;
	DECUMA_HH * nodeHash=NULL;

	DECUMA_TERSE_TRIE_HEADER * pTerseTrieHeader;
	DECUMA_UINT32 nUniqueNodes = 0;
	
	DECUMA_TERSE_TRIE_NODE * pTerseTrieFirst;
	DECUMA_UINT32 * pFreqClassTable;
	DECUMA_TERSE_TRIE_DATA * pDataBuffer;
	DECUMA_TRIE_ITERATOR * pTrieIterator=NULL;
	DECUMA_UINT8 nFreqClasses = decumaTrieGetNFreqClasses(pTrie);
	DECUMA_UINT32 nNodes=0;
	DECUMA_HH_ITERATOR * pHHIterator=NULL;

#ifdef DEBUG
	DECUMA_UINT16 max_offset[DECUMA_TERSE_TRIE_MAX_DEPTH];
	DECUMA_UINT16 min_offset[DECUMA_TERSE_TRIE_MAX_DEPTH];
	long nodes_per_level[DECUMA_TERSE_TRIE_MAX_DEPTH];

	decumaMemset(nodes_per_level, 0, sizeof(nodes_per_level));
	decumaMemset(max_offset, 0, sizeof(max_offset));
	decumaMemset(min_offset, 0x7f, sizeof(min_offset));
#endif

	*ppTerseTrie = NULL;
	if (pTrie->trieHeader.nMax > DECUMA_TERSE_TRIE_MAX_DEPTH)
	{
		/*The dictionary (trie) has longer words than can be used by this format. */
		/*Either the trie is invalid or we need to update the terse trie format to */
		/*support that long words */
		decumaAssert(0);
		return decumaInvalidDictionary;
	}

	/* Find all unique nodes; this is a linear traversal of the graph */
#define HASH_TABLE_GRANULARITY 5000    /* = 65536. This is also the limit on number of uniue nodes  */
                                             /* due to the datatype size */

	nodeHash = decumaModCalloc(MODID_DICTIONARY, 
		1, decumaHHGetSize(HASH_TABLE_GRANULARITY,decumaHHSizeMedium));
	if (!nodeHash)
	{
		status = decumaAllocationFailed;
		goto decumaTerseTrieCreateFromTrie_failed;
	}

	decumaHHInit(nodeHash, HASH_TABLE_GRANULARITY,decumaHHSizeMedium);

	pTrieIterator = decumaTrieIteratorCreate(pTrie,pMemFunctions);
	if (!pTrieIterator)
	{
		status = decumaAllocationFailed;
		goto decumaTerseTrieCreateFromTrie_failed;
	}

	while ( (pTrieNode = decumaTrieIteratorGetNext(pTrieIterator)) )
	{
		/* Very simplistic hash function, just lay out some of the fields within one 32-bit integer */
			DECUMA_UINT32 key = pTrieNode->unicode << 9 | ((DECUMA_UINT16)pTrieNode->bestFreqClass) << 6 | 
				pTrieNode->nMax << 2 | pTrieNode->nMin;
		DECUMA_HH_ELEMREF elemRef;

		nNodes++;

		/* Look up the current node in the hash table */
		elemRef = decumaHHFindKey(nodeHash, key); 
		while ( elemRef && !equals(pTrieNode, decumaHHGetData(elemRef)) )
			elemRef = decumaHHFindNextKey(elemRef);

		/* If the current node does not exist, add it to the hash */
		if ( !elemRef ) {
			elemRef = decumaHHAdd(nodeHash, key, pTrieNode);
			/*decumaAssert(elemRef); */
			if (!elemRef)
			{
				void * pElemBuf;
				/*We have filled the existing buffer in the hash handler. Create and attach a new buffer */
				pElemBuf = decumaModCalloc(MODID_DICTIONARY,1,
					decumaHHGetExtraElemBufferSize(HASH_TABLE_GRANULARITY));
				if (!pElemBuf)
				{
					status = decumaAllocationFailed;
					goto decumaTerseTrieCreateFromTrie_failed;
				}

				if (!decumaHHAttachExtraElemBuffer(nodeHash, pElemBuf,HASH_TABLE_GRANULARITY))
				{
					decumaAssert(0);
					/*We can not add more elem buffers */
					/*Change the size of or the HASH_TABLE_GRANULARITY? */
					if (pElemBuf) decumaModFree(MODID_DICTIONARY,pElemBuf);

					status = decumaInvalidDictionary;
					goto decumaTerseTrieCreateFromTrie_failed;
				}
				elemRef = decumaHHAdd(nodeHash, key, pTrieNode);
				decumaAssert(elemRef); /*This should never fail, now that we have added more buffer space */
			}
			nUniqueNodes += 1;
		}
	}
	decumaTrieIteratorDestroy(pTrieIterator,pMemFunctions);
	pTrieIterator=NULL;
#ifdef HASH_HANDLER_STATISTICS
	decumaHHPrintStats(nodeHash,"nodeHash");
#endif

	decumaAssert(nUniqueNodes < nNodes);
	decumaAssert(nUniqueNodes <= MAX_DECUMA_UINT16);
	if (nUniqueNodes > MAX_DECUMA_UINT16)
	{
		/*We have too many unique nodes for the data type */
		status = decumaInvalidDictionary;
		goto decumaTerseTrieCreateFromTrie_failed;
	}

	/* Here, we know 
	 * -> the total number of nodes
	 * -> the number of unique nodes
	 * so now we can alllocate just the right amount of memory for the terse trie.
	 */

	*ppTerseTrie = decumaModCalloc(MODID_DICTIONARY,1,
		sizeof(DECUMA_TERSE_TRIE) + 
		nFreqClasses*sizeof(DECUMA_UINT32) + 
		sizeof(DECUMA_TERSE_TRIE_NODE) * nNodes + 
		sizeof(DECUMA_TERSE_TRIE_DATA) * nUniqueNodes);
	
	if (!(*ppTerseTrie)) 
	{
		status = decumaAllocationFailed;
		goto decumaTerseTrieCreateFromTrie_failed;
	}

	/*Initialize the binary header */
	decumaDictionaryBinaryInitHeader((DECUMA_DICTIONARY_BINARY_HEADER*) &(*ppTerseTrie)->binaryHeader,
		decumaDictionaryBinaryStructureTerseTrie);

	/*Initialize the terse trie */
	pTerseTrieHeader = &((*ppTerseTrie)->trieHeader);

	/*Set pointers correctly */
	pTerseTrieHeader->nDataElements = nUniqueNodes;
	pTerseTrieHeader->nNodes = nNodes;
	pTerseTrieHeader->nWords = decumaTrieGetNWords(pTrie);

	decumaTrieGetSubTreeFreqClassMinMax(pTrie,NULL,&pTerseTrieHeader->nFC0Min,&pTerseTrieHeader->nFC0Max);
	decumaTrieGetMinMax(pTrie,NULL,&pTerseTrieHeader->nMin,&pTerseTrieHeader->nMax);
	
	pTerseTrieHeader->nFreqClasses = nFreqClasses;

	/*Put the freq table just after the header */
	pFreqClassTable = (DECUMA_UINT32 *) &pTerseTrieHeader[1];

	pTerseTrieHeader->freqClassTableOffset = OFFSET_BETWEEN(pFreqClassTable,pTerseTrieHeader);
	{
		int i;
		for (i = 0; i < nFreqClasses; i++)
		{
			DECUMA_UINT32 dummy;
			decumaTrieGetRankInFreqClass(pTrie,i,&pFreqClassTable[i],&dummy);
		}
	}

	/*Put the first node just after the freq table */
	pTerseTrieFirst = (DECUMA_TERSE_TRIE_NODE *) &pFreqClassTable[pTerseTrieHeader->nFreqClasses];
	pTerseTrieHeader->firstNodeOffset = OFFSET_BETWEEN(pTerseTrieFirst,pTerseTrieHeader);

	pTerseNode = pTerseTrieFirst;

	/*Put the data buffer after all the nodes */
	pDataBuffer = (DECUMA_TERSE_TRIE_DATA *) &pTerseTrieFirst[pTerseTrieHeader->nNodes];
	pTerseTrieHeader->dataBufferOffset = OFFSET_BETWEEN(pDataBuffer,pTerseTrieHeader);

	
	
	indexHash = decumaModCalloc(MODID_DICTIONARY,1, decumaHHGetSize(nUniqueNodes,decumaHHSizeMedium));
	if (!indexHash) 
	{
		status = decumaAllocationFailed;
		goto decumaTerseTrieCreateFromTrie_failed;
	}

	decumaHHInit(indexHash, nUniqueNodes,decumaHHSizeMedium);

	{
		DECUMA_HH_ELEMREF elemRef;
		DECUMA_TERSE_TRIE_DATA * pDataElement = pDataBuffer;
		
		pHHIterator = decumaModCalloc(MODID_DICTIONARY,1,decumaHHIteratorGetSize());
		if (!pHHIterator)
		{
			status = decumaAllocationFailed;
			goto decumaTerseTrieCreateFromTrie_failed;
		}
		decumaHHIteratorInit(pHHIterator,nodeHash);

		while ( (elemRef = decumaHHIteratorNext(pHHIterator)) != NULL)
		{
			DECUMA_TRIE_NODE * pTempNode = decumaHHGetData(elemRef);
			/*DECUMA_UINT32 key = pTempNode->unicode << 16 |  */
			/*	pTempNode->nMax << 12 | pTempNode->nMin << 8 |  */
			/*	pTempNode->bestFreqClass; */
			DECUMA_UINT32 key = pTempNode->unicode << 9 | ((DECUMA_UINT16)pTempNode->bestFreqClass) << 6 |
				pTempNode->nMax << 2 | pTempNode->nMin;

			pDataElement->max = pTempNode->nMax;
			pDataElement->min = pTempNode->nMin;
			pDataElement->unicode = pTempNode->unicode;
			pDataElement->bestFreqClass = pTempNode->bestFreqClass;
			pDataElement->thisFreqClass = pTempNode->thisFreqClass;
			pDataElement->bestFCMin = pTempNode->bestFCMin;
			pDataElement->bestFCMax = pTempNode->bestFCMax;

			decumaHHAdd(indexHash, key, pDataElement);
			pDataElement++;
		}
		decumaAssert( (pDataElement - pDataBuffer) == nUniqueNodes);
		decumaModFree(MODID_DICTIONARY,pHHIterator);
	}

	/* Free up temporary structures */
	{
		int i;
		for (i=0; i<decumaHHGetNExtraElemBuffers(nodeHash);i++)
		{
			void * pElemBuf = decumaHHGetExtraElemBuffer(nodeHash,i);
			decumaModFree(MODID_DICTIONARY,pElemBuf);
		}
	}
	decumaModFree(MODID_DICTIONARY,nodeHash);

	nodeHash = NULL;

#ifdef HASH_HANDLER_STATISTICS
	decumaHHPrintStats(indexHash,"indexHash");
#endif


	/* Re-set pTrieNode for another traversal, a DFS this time */
	pTrieNode = decumaTrieGetChild((DECUMA_TRIE*)pTrie,NULL);

	/* Do a depth-first traversal and add build up the 'terse' array */
	do
	{
		do
		{
			decumaAssert(n < DECUMA_TERSE_TRIE_MAX_DEPTH);
			stack[n] = pTrieNode;

			/* Look up the "unique-node" index and set pTerseNode->nodeData */
			{
				DECUMA_UINT32 key = pTrieNode->unicode << 9 | ((DECUMA_UINT16)pTrieNode->bestFreqClass) << 6 | 
					pTrieNode->nMax << 2 | pTrieNode->nMin;
				DECUMA_HH_ELEMREF elemRef;

				elemRef = decumaHHFindKey(indexHash, key); 
				while ( elemRef && !terse_equals(pTrieNode, decumaHHGetData(elemRef)) )
					elemRef = decumaHHFindNextKey(elemRef);

				decumaAssert(elemRef);
				
				/* Asserting that operation result will fit in result type */
				decumaAssert((DECUMA_TERSE_TRIE_DATA *) decumaHHGetData(elemRef) - pDataBuffer >= MIN_DECUMA_UINT16);
				decumaAssert((DECUMA_TERSE_TRIE_DATA *) decumaHHGetData(elemRef) - pDataBuffer <= MAX_DECUMA_UINT16);
				
				pTerseNode->nodeData = (DECUMA_UINT16) ((DECUMA_TERSE_TRIE_DATA *) decumaHHGetData(elemRef) - pDataBuffer);
			}

#if defined(DECUMA_ASSERT_OVERRIDE) || defined(DECUMA_ASSERT_ENABLE)
			{
				DECUMA_TERSE_TRIE_DATA * pData = &pDataBuffer[pTerseNode->nodeData];
				/* decumaAssert(pData->frequencyClass == pTrieNode->bestUpcoming); */
				decumaAssert(pData->unicode == pTrieNode->unicode );
				decumaAssert(pData->max == pTrieNode->nMax);
				decumaAssert(pData->min == pTrieNode->nMin);
				decumaAssert(pTrieNode->pChild == NULL ? (pData->min + pData->max) == 0 : (pData->min + pData->max) != 0 ); /* (min, max) = (0, 0) means end of child tree  */
			}
#endif
			terseStack[n] = pTerseNode;

			pTerseNode->siblingOffset = 0; /* Will be set later */

#ifdef DEBUG
			nodes_per_level[n] += 1;
#endif
			pTerseNode++;
			n++;
		} while ( (pTrieNode = pTrieNode->pChild) != NULL );

		do 
		{
			n-=1;
		} while ( n >= 0 && (pTrieNode = stack[n]->pSibling) == NULL );

		if ( n >= 0 ) 
		{
			/* Asserting that operation result will fit in result type */
			decumaAssert(pTerseNode - terseStack[n] >= MIN_DECUMA_UINT16); /* Assert no casting truncation */
			decumaAssert(pTerseNode - terseStack[n] <= MAX_DECUMA_UINT16); /* Assert no casting truncation */

			if (pTerseNode - terseStack[n] > MAX_DECUMA_UINT16) 
			{
				/*We have an offset from one node to its sibbling of more than 65536  */
				status = decumaInvalidDictionary;
				goto decumaTerseTrieCreateFromTrie_failed;
			}

#ifdef DEBUG
			if ( pTerseNode - terseStack[n] > max_offset[n] )
				max_offset[n] = pTerseNode - terseStack[n];
			if ( pTerseNode - terseStack[n] < min_offset[n] ) 
				min_offset[n] = pTerseNode - terseStack[n];
#endif

			/* Here we a problem if using variable-sized nodes with this algorithm; we modify structs already in the array. */
			terseStack[n]->siblingOffset = (DECUMA_UINT16) (pTerseNode - terseStack[n]);
		}
	} while ( n >= 0 );

	decumaAssert(pTerseNode == pTerseTrieFirst + pTrieHeader->nNodes);

	/* Here, we no longer need the pointer-to-index hash */
	decumaModFree(MODID_DICTIONARY,indexHash);
	indexHash = NULL;

	return decumaNoError;

decumaTerseTrieCreateFromTrie_failed:
	if (pTrieIterator)
	{
		decumaTrieIteratorDestroy(pTrieIterator,pMemFunctions);
	}
	if (indexHash) 
	{
		/*We have not added any buffers to index hash */
		decumaAssert(decumaHHGetNExtraElemBuffers(indexHash)==0);
		decumaModFree(MODID_DICTIONARY,indexHash);
	}
	if (nodeHash) 
	{
		int i;
		for (i=0; i<decumaHHGetNExtraElemBuffers(nodeHash);i++)
		{
			void * pElemBuf = decumaHHGetExtraElemBuffer(nodeHash,i);
			decumaModFree(MODID_DICTIONARY,pElemBuf);
		}
		decumaModFree(MODID_DICTIONARY,nodeHash);
	}
	if (*ppTerseTrie) 
	{
		decumaModFree(MODID_DICTIONARY,*ppTerseTrie);
		*ppTerseTrie = NULL;
	}

	return status;
}

void decumaTerseTrieDestroy(DECUMA_TERSE_TRIE ** ppTerseTrie,
							const DECUMA_MEM_FUNCTIONS * pMemFunctions)
{
	if (*ppTerseTrie)
	{
		decumaModFree(MODID_DICTIONARY,*ppTerseTrie); 
		*ppTerseTrie = NULL;
	}
}


/* Access functions for creating a DECUMA_HWR_DICTIONARY structure of a DECUMA_TERSE_TRIE */

DECUMA_TERSE_TRIE_NODE_REF decumaTerseTrieGetSibling(DECUMA_TERSE_TRIE * pTerseTrie,  DECUMA_TERSE_TRIE_NODE_REF currentRef)
{
	if ( currentRef->siblingOffset == 0 )
	{
		return NULL;
	}
	else
	{
		return currentRef + currentRef->siblingOffset;
	}
}


DECUMA_TERSE_TRIE_NODE_REF decumaTerseTrieGetChild(DECUMA_TERSE_TRIE * pTerseTrie,  DECUMA_TERSE_TRIE_NODE_REF currentRef)
{
	const DECUMA_TERSE_TRIE_HEADER * pTerseTrieHeader = &pTerseTrie->trieHeader;
	if ( currentRef ) 
	{
		DECUMA_TERSE_TRIE_DATA * pDataElement = 
			CONVERT_TO_PTR(DECUMA_TERSE_TRIE_DATA*,pTerseTrieHeader->dataBufferOffset,pTerseTrieHeader);
		pDataElement += currentRef->nodeData;
	
		if ( pDataElement->max > 0 )
		{
			return currentRef + 1;
		}
		else
		{
			decumaAssert(pDataElement->max == 0);
			decumaAssert(pDataElement->min == 0);
			return NULL;
		}
	}
	else
	{
		return CONVERT_TO_PTR(DECUMA_TERSE_TRIE_NODE_REF,
			pTerseTrieHeader->firstNodeOffset,pTerseTrieHeader);
	}
}

void decumaTerseTrieGetMinMax(const DECUMA_TERSE_TRIE * pTerseTrie,  DECUMA_TERSE_TRIE_NODE_REF currentRef, DECUMA_UINT8 * pMin, DECUMA_UINT8 * pMax)
{
	const DECUMA_TERSE_TRIE_HEADER * pTerseTrieHeader = &pTerseTrie->trieHeader;
	
	if ( currentRef == NULL ) 
	{
		if ( pMin ) *pMin = pTerseTrieHeader->nMin;
		if ( pMax ) *pMax = pTerseTrieHeader->nMax;
	} 
	else
	{
		const DECUMA_TERSE_TRIE_DATA * pDataElement = 
			CONVERT_TO_PTR(const DECUMA_TERSE_TRIE_DATA*,pTerseTrieHeader->dataBufferOffset,pTerseTrieHeader);
		pDataElement += currentRef->nodeData;

		if ( pMin ) *pMin = pDataElement->min;
		if ( pMax ) *pMax = pDataElement->max;
	}
}

DECUMA_UNICODE decumaTerseTrieGetUnicode(const DECUMA_TERSE_TRIE * pTerseTrie, DECUMA_TERSE_TRIE_NODE_REF currentRef)
{
	const DECUMA_TERSE_TRIE_HEADER * pTerseTrieHeader = &pTerseTrie->trieHeader;
	const DECUMA_TERSE_TRIE_DATA * pDataElement = 
		CONVERT_TO_PTR(const DECUMA_TERSE_TRIE_DATA*,
		pTerseTrieHeader->dataBufferOffset,pTerseTrieHeader);
	pDataElement += currentRef->nodeData;

	return pDataElement->unicode;
}

int decumaTerseTrieGetEndOfWord(const DECUMA_TERSE_TRIE * pTerseTrie, DECUMA_TERSE_TRIE_NODE_REF currentRef)
{
	const DECUMA_TERSE_TRIE_HEADER * pTerseTrieHeader = &pTerseTrie->trieHeader;
	const DECUMA_TERSE_TRIE_DATA * pDataElement = 
		CONVERT_TO_PTR(const DECUMA_TERSE_TRIE_DATA*,
		pTerseTrieHeader->dataBufferOffset,pTerseTrieHeader);
	pDataElement += currentRef->nodeData;

	return pDataElement->min == 0;
}

DECUMA_UINT8 decumaTerseTrieGetNFreqClasses(const DECUMA_TERSE_TRIE * pTerseTrie)
{
	return pTerseTrie->trieHeader.nFreqClasses;
}

void decumaTerseTrieGetRankInFreqClass(const DECUMA_TERSE_TRIE * pTerseTrie, DECUMA_UINT8 freqClass,
												DECUMA_UINT32 * pMinRank, DECUMA_UINT32 * pMaxRank)
{
	const DECUMA_TERSE_TRIE_HEADER * pTerseTrieHeader = &pTerseTrie->trieHeader;
	const DECUMA_UINT32 * pFreqClassTable;

	decumaAssert(pTerseTrie);
	decumaAssert(freqClass<pTerseTrieHeader->nFreqClasses);
	decumaAssert(pMinRank);
	decumaAssert(pMaxRank);

	pFreqClassTable = 
		CONVERT_TO_PTR(const DECUMA_UINT32*,
		pTerseTrieHeader->freqClassTableOffset,pTerseTrieHeader);

	decumaAssert(freqClass<pTerseTrieHeader->nFreqClasses);

	*pMinRank = pFreqClassTable[freqClass];
	*pMaxRank =  (freqClass >= pTerseTrieHeader->nFreqClasses-1 ? pTerseTrieHeader->nWords : pFreqClassTable[freqClass+1])-1;

	/*If we have more freq classes than words to fill them... */
	if (*pMinRank > pTerseTrieHeader->nWords) *pMinRank = pTerseTrieHeader->nWords-1;
	if (*pMaxRank > pTerseTrieHeader->nWords) *pMaxRank = pTerseTrieHeader->nWords-1;
}

DECUMA_UINT8 decumaTerseTrieGetSubTreeFreqClass(const DECUMA_TERSE_TRIE * pTerseTrie, DECUMA_TERSE_TRIE_NODE_REF currentRef)
{
	if (!currentRef) 
	{
		return 0; /*Return class 0 for root */
	}
	else
	{
		
		const DECUMA_TERSE_TRIE_HEADER * pTerseTrieHeader = &pTerseTrie->trieHeader;
		const DECUMA_TERSE_TRIE_DATA * pDataElement = 
			CONVERT_TO_PTR(const DECUMA_TERSE_TRIE_DATA*,
			pTerseTrieHeader->dataBufferOffset,pTerseTrieHeader);
		pDataElement += currentRef->nodeData;

		return pDataElement->bestFreqClass;
	}
}

DECUMA_UINT8 decumaTerseTrieGetWordFreqClass(const DECUMA_TERSE_TRIE * pTerseTrie, DECUMA_TERSE_TRIE_NODE_REF currentRef)
{
	if (!currentRef) 
	{
		return MAX_DECUMA_UINT8;
	}
	else
	{
		const DECUMA_TERSE_TRIE_HEADER * pTerseTrieHeader = &pTerseTrie->trieHeader;
		const DECUMA_TERSE_TRIE_DATA * pDataElement = 
			CONVERT_TO_PTR(const DECUMA_TERSE_TRIE_DATA*,
			pTerseTrieHeader->dataBufferOffset,pTerseTrieHeader);
		pDataElement += currentRef->nodeData;

		return pDataElement->thisFreqClass;
	}
}

void decumaTerseTrieGetSubTreeFreqClassMinMax(const DECUMA_TERSE_TRIE * pTerseTrie, DECUMA_TERSE_TRIE_NODE_REF currentRef,
	DECUMA_UINT8 * pMin, DECUMA_UINT8 * pMax)
{
	decumaAssert(pMin);
	decumaAssert(pMax);

	if (!currentRef) 
	{
		*pMin = pTerseTrie->trieHeader.nFC0Min;
		*pMax = pTerseTrie->trieHeader.nFC0Max;
	} 
	else
	{
		const DECUMA_TERSE_TRIE_HEADER * pTerseTrieHeader = &pTerseTrie->trieHeader;
		const DECUMA_TERSE_TRIE_DATA * pDataElement = 
			CONVERT_TO_PTR(const DECUMA_TERSE_TRIE_DATA*,
			pTerseTrieHeader->dataBufferOffset,pTerseTrieHeader);
		pDataElement += currentRef->nodeData;

		*pMin = pDataElement->bestFCMin;
		*pMax = pDataElement->bestFCMax;
	}
}



DECUMA_UINT32 decumaTerseTrieGetSize(const DECUMA_TERSE_TRIE * pTerseTrie)
{
	return sizeof(DECUMA_TERSE_TRIE) + 
		pTerseTrie->trieHeader.nFreqClasses*sizeof(DECUMA_UINT32) + 
		sizeof(DECUMA_TERSE_TRIE_DATA) * pTerseTrie->trieHeader.nDataElements + 
		sizeof(DECUMA_TERSE_TRIE_NODE) * pTerseTrie->trieHeader.nNodes;
}

DECUMA_UINT32 decumaTerseTrieGetNWords(const DECUMA_TERSE_TRIE * pTerseTrie)
{
	return pTerseTrie->trieHeader.nWords;
}

/* */
/*Extra access functions */
/* */
DECUMA_UINT32 decumaTerseTrieGetNNodes(const DECUMA_TERSE_TRIE * pTerseTrie)
{
	return pTerseTrie->trieHeader.nNodes;
}

DECUMA_UINT32 decumaTerseTrieGetNDataElems(const DECUMA_TERSE_TRIE * pTerseTrie)
{
	return pTerseTrie->trieHeader.nDataElements;
}



/*/------------------------- Local functions ----------------------------- */

static int equals(const DECUMA_TRIE_NODE * a, const DECUMA_TRIE_NODE * b)
{
	return a->unicode == b->unicode && 
		a->nMax == b->nMax && 
		a->nMin == b->nMin && 
		a->thisFreqClass == b->thisFreqClass && 
		a->bestFreqClass == b->bestFreqClass &&
		a->bestFCMin == b->bestFCMin &&
		a->bestFCMax == b->bestFCMax;

}

static int terse_equals(const DECUMA_TRIE_NODE * a, const DECUMA_TERSE_TRIE_DATA * b)
{
	return a->unicode == b->unicode && 
		a->nMax == b->max && 
		a->nMin == b->min && 
		a->thisFreqClass == b->thisFreqClass && 
		a->bestFreqClass == b->bestFreqClass &&
		a->bestFCMin == b->bestFCMin &&
		a->bestFCMax == b->bestFCMax;
}

