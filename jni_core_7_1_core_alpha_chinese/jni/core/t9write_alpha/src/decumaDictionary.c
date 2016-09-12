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


/* Generic dictionary functions 
 * The intent is to provide a "cleaner" API compared to the old eZiText implementation. 
 * This first revision keeps the older implementation style derived from Achilles' 
 * ZOUTEDGE struct definition, but it should now be completely up to this module 
 * precisely how it's implemented.
 * 
 * 
 * Important unfinished tasks:
 *  - The rank estimation is poorly constructed.
 *    There are three separate functions that needs to be coordinated:
 *
 *    getRank() which turns a word index into a rank estimation
 *
 *    getRankBoost() / getMaxRankBoost() which gives a rank boost for a specific dictionary entry, and the best rank boost possible
 *     [member functions of the dictionary object, so instances use 
 *      trieGetRankBoost() / trieGetMaxRankBoost() or
 *      terseTrieGetRankBoost() / terseTrieGetMaxRankBoost()]
 */

#include "decumaConfig.h"

#define DECUMA_DICTIONARY_C

#include "decumaDictionary.h"

#ifdef DECUMA_HWR_ENABLE_EXTRA_API
#include "decumaDictionaryExtra.h"
#endif

#include "decumaDictionaryData.h"
#include "decumaDictionaryMacros.h"
#include "decumaTrie.h"
#include "decumaTerseTrie.h"
#include "decumaDictionaryXT9.h"
#include "decumaAssert.h"
#include "decumaMemory.h"
#include "decumaBasicTypes.h"
#include "decumaBasicTypesMinMax.h"
#include "decumaDictionaryBinary.h"

/*/////////////////////////// */

/*Used to hold the stack variables that were previously the parameters of the */
/*recursive function. */
typedef struct _SEARCH_STACK_ELEMENT
{
	DECUMA_HWR_DICTIONARY_REF nodeRef;
	DECUMA_UINT8 minRem;
	DECUMA_UINT8 maxRem;
	DECUMA_UINT8 bestSoFar;
	DECUMA_UINT8 bestPossible;
} SEARCH_STACK_ELEMENT;

static int nonRecGetSubTreeFreqClassGivenMinMax(const DECUMA_HWR_DICTIONARY * pDictionary, 
	SEARCH_STACK_ELEMENT * pThisVals, SEARCH_STACK_ELEMENT * pChildVals);
	
#if defined(DECUMA_ASSERT_ENABLE) || defined(DECUMA_ASSERT_OVERRIDE)
static DECUMA_UINT8 recGetSubTreeFreqClassGivenMinMax(const DECUMA_HWR_DICTIONARY * pDictionary, 
													  DECUMA_HWR_DICTIONARY_REF ref,
													  DECUMA_UINT8 minRem, DECUMA_UINT8 maxRem,
													  DECUMA_UINT8 bestSoFar, DECUMA_UINT8 bestPossible);
#endif 

static void initWithTerseTrie(DECUMA_HWR_DICTIONARY * pDictionary, 
	DECUMA_TERSE_TRIE * pTerseTrie);

static void initWithTrie(DECUMA_HWR_DICTIONARY * pDictionary, 
	DECUMA_TRIE * pTrie);

static int decumaDictionaryIsInitializedTrie(DECUMA_HWR_DICTIONARY *pDictionary);

static int decumaDictionaryIsInitializedTerseTrie(DECUMA_HWR_DICTIONARY *pDictionary);

/*/////////////////////////////////////// */

DECUMA_UINT32 decumaDictionaryGetSize(void)
{
	return sizeof(DECUMA_HWR_DICTIONARY);
}

/*Returns an error code if the data is not a recognized dictionary type */
DECUMA_STATUS decumaDictionaryValidateBinaryData(const void * pDictionaryData)
{
	DECUMA_DICTIONARY_BINARY_STRUCTURE type;

	if (!decumaDictionaryBinaryIsDecumaDict(pDictionaryData))
		return decumaInvalidDictionary;

	if (!decumaDictionaryBinaryFormatSupported(pDictionaryData))
		return decumaInvalidDictionary;

	type = decumaDictionaryBinaryGetStructure(pDictionaryData);

	switch (type)
	{
		case decumaDictionaryBinaryStructureTerseTrie:
		case decumaDictionaryBinaryStructureTrie:
			break;

		default:
			return decumaInvalidDictionary;
	}
	return decumaNoError;
}

DECUMA_STATUS decumaDictionaryInitWithData(DECUMA_HWR_DICTIONARY * pDictionary, 
								  const void * pDictionaryData)
{
	DECUMA_DICTIONARY_BINARY_STRUCTURE type;
	DECUMA_STATUS status;
	decumaAssert(pDictionary);
	decumaAssert(pDictionaryData);

	status = decumaDictionaryValidateBinaryData(pDictionaryData);
	if (status != decumaNoError)
		return status;

	type = decumaDictionaryBinaryGetStructure(pDictionaryData);

	switch (type)
	{
		case decumaDictionaryBinaryStructureTerseTrie:
			initWithTerseTrie(pDictionary, (DECUMA_TERSE_TRIE *) pDictionaryData);
			break;

		case decumaDictionaryBinaryStructureTrie:
			initWithTrie(pDictionary, (DECUMA_TRIE *) pDictionaryData);
			break;

		default:
			decumaAssert(0); /*Should already have been validated */
	}

	return decumaNoError;
}



/*Identifies the kind of ditionary data and destroys it if the type is recognized */
DECUMA_STATUS decumaDictionaryDestroyBinaryData(void ** ppDictionaryData,
	const DECUMA_MEM_FUNCTIONS * pMemFunctions)
{
	DECUMA_DICTIONARY_BINARY_STRUCTURE dictStructureType;
	DECUMA_STATUS status;
	decumaAssert(ppDictionaryData);
	decumaAssert(*ppDictionaryData); /*This should already have been checked outside of this function */

	status = decumaDictionaryValidateBinaryData(*ppDictionaryData);
	if (status != decumaNoError)
		return status;

	dictStructureType = decumaDictionaryBinaryGetStructure(*ppDictionaryData);

	switch (dictStructureType)
	{
		case decumaDictionaryBinaryStructureTerseTrie:
			decumaTerseTrieDestroy((DECUMA_TERSE_TRIE **) ppDictionaryData,pMemFunctions);
			break;

		case decumaDictionaryBinaryStructureTrie:
			decumaTrieDestroy((DECUMA_TRIE **) ppDictionaryData,pMemFunctions);
			break;

		default:
			decumaAssert(0); /*Should already have been validated */
	}

	return decumaNoError;
}



void decumaDictionaryDestroyData(DECUMA_HWR_DICTIONARY * pDictionary,
								 const DECUMA_MEM_FUNCTIONS * pMemFunctions)
{
	decumaAssert(pDictionary);
	decumaAssert(pMemFunctions);

	if ( decumaDictionaryIsInitializedTerseTrie(pDictionary))
	{
		decumaTerseTrieDestroy((DECUMA_TERSE_TRIE**)&pDictionary->pDictionaryData,
			pMemFunctions);
	}
	else if ( decumaDictionaryIsInitializedTrie(pDictionary))
	{
		decumaTrieDestroy((DECUMA_TRIE**)&pDictionary->pDictionaryData,
			pMemFunctions);
	}
	else{
		decumaAssert(0);
	}
}

void * decumaDictionaryGetDataPtr(DECUMA_HWR_DICTIONARY *pDictionary)
{
	return pDictionary->pDictionaryData;
}


#ifdef DECUMA_HWR_ENABLE_EXTRA_API

/*/////////////// DECUMA DICTIONARY EXTRA FUNCTIONS //////////////////////// */

/* Create HWR dictionary from UTF-8 text buffer */
void decumaDictionaryInitFromUTF8(DECUMA_HWR_DICTIONARY * ppDictionary,
	const char * pUTF8,int const length,int const bNullDelimited)
{
}

/* Create HWR dictionary from UTF-16 (native endianess) text buffer */
void decumaDictionaryInitFromUTF16(DECUMA_HWR_DICTIONARY * pDictionary,
	const DECUMA_UNICODE * pUTF16,int const length,int const bNullDelimited)
{
	/* TODO 
	 * Create DECUMA_TRIE -- estimate proper size
	 * Initialize a trie dictionary
	 * Call decumaDictionaryAddAllWords()
	 * Compare with this function originally from decuma_hwr_verify.c :
	 */

#if 0
/* Add all words in a unicode buffer to a trie. This uses low-level eZiText functions, so perhaps it is better suited in a library?
 * We don't use the decuma_hwr function decumaAddAllWords(), since that requires an active session which we might not have here.
 */
static DECUMA_STATUS addAllWordsToTrie(void * pTrie, DECUMA_UNICODE * pWordBuffer, int nUnicodesInBuffer, int bNullDelimitedOnly)
{
	const DECUMA_UNICODE * pWordStart = 0;
	const DECUMA_UNICODE * pWordBufferEnd;
	DECUMA_UNICODE delimiter = (bNullDelimitedOnly) ? 0x000 : 0x0020;

	pWordBufferEnd = pWordBuffer + nUnicodesInBuffer;

	/* If we see a BOM with wrong byte order, return an error. A correct BOM is skipped, though. */
	if ( *pWordBuffer == 0xFFFE ) return decumaInvalidBufferType;
	if ( *pWordBuffer == 0xFEFF ) ++pWordBuffer;

	/* Iterate over all pWordBuffer */
	for ( ; pWordBuffer < pWordBufferEnd; ++pWordBuffer )
	{
		/* All whitespaces are skipped; since whitespace here is defined as anything less than unicode 0x0020 we will also skip some control characters and such. Note that we also skip null characters, i.e. a buffer may contain null-delimited words. */
		if ( *pWordBuffer <= delimiter )
		{
			/* First whitespace after a set of non-whitespace unicodes; we have a word end! */
			if ( pWordStart )
			{
				/* Attempt to add the word. */
				ZI8BOOL ziStatus = Zi8TrieCopyWord(pTrie, pWordStart, pWordBuffer);
				if ( !ziStatus ) return decumaTooShortBuffer;
				pWordStart = 0; /* Clear word start marker */
			}
		} else {
			/* Otherwise, if we have not seen a non-whitespace unicode previously, we have a word start! */
			if ( pWordStart == 0 )
			{
				pWordStart = pWordBuffer;
			}
		}
	}

	/* Don't forget to add the final word if the buffer ends with a non-whitespace */
	if ( pWordStart )
	{
		ZI8BOOL ziStatus = Zi8TrieCopyWord(pTrie, pWordStart, pWordBufferEnd);
		if ( !ziStatus ) return decumaTooShortBuffer;
	}

	return decumaNoError;
}

#endif
}

DECUMA_STATUS decumaDictionaryAddAllWords(DECUMA_HWR_DICTIONARY * pDictionaryBuffer,
	const DECUMA_UNICODE * const pWordBufferStart,int nUnicodesInBuffer,
	int bNullDelimitedOnly)
{
	return decumaFunctionNotSupported;
}

DECUMA_STATUS decumaDictionaryWordExists(const DECUMA_HWR_DICTIONARY * pDictionary,
	DECUMA_UNICODE * pWord,int maxLength,DECUMA_STRING_TYPE * pStringType)
{
	int n;
	DECUMA_HWR_DICTIONARY_REF tempRef = NULL;
	DECUMA_HWR_DICTIONARY_REF prevRef = NULL;

	if ( pDictionary == NULL || pWord == NULL || pStringType == NULL )
		return decumaNullPointer;

	*pStringType = notFromDictionary;

	for ( n = 0; n < maxLength; n++ )
	{
		tempRef = decumaDictionaryGetChild(pDictionary, prevRef);
		while ( tempRef && pWord[n] > decumaDictionaryGetUnicode(pDictionary, tempRef))
			tempRef = decumaDictionaryGetSibling(pDictionary, tempRef);

		if ( tempRef == NULL || pWord[n] != decumaDictionaryGetUnicode(pDictionary, tempRef) ) 
			break;
	}

	if ( n == maxLength || pWord[n]==0)
		*pStringType = completeWord;
	else if ( n > 0 )
		*pStringType = startOfWord;

	return decumaNoError;
}

#endif /* DECUMA_HWR_ENABLE_EXTRA_API */

/*/////////////// DECUMA DICTIONARY LOW LEVEL FUNCTIONS //////////////////////// */

/* Support functions; these simply wrap respective macros with some asserts */

#if defined(DEBUG)

DECUMA_HWR_DICTIONARY_REF decumaDictionaryGetSibling(const DECUMA_HWR_DICTIONARY * pDictionary,  DECUMA_HWR_DICTIONARY_REF currentRef)
{
	decumaAssert(pDictionary);
	return decumaDictionaryGetSiblingMacro(pDictionary, currentRef);
}

DECUMA_HWR_DICTIONARY_REF decumaDictionaryGetChild(const DECUMA_HWR_DICTIONARY * pDictionary,	DECUMA_HWR_DICTIONARY_REF currentRef)
{
	decumaAssert(pDictionary);
	return decumaDictionaryGetChildMacro(pDictionary, currentRef);
}

void decumaDictionaryGetMinMax(const DECUMA_HWR_DICTIONARY * pDictionary,  DECUMA_HWR_DICTIONARY_REF currentRef, DECUMA_UINT8 * pMin, DECUMA_UINT8 * pMax)
{
	decumaAssert(pDictionary);
	decumaDictionaryGetMinMaxMacro(pDictionary, currentRef, pMin, pMax);
}

DECUMA_UINT32 decumaDictionaryGetDataSize(const DECUMA_HWR_DICTIONARY * pDictionary)
{
	decumaAssert(pDictionary);
	return decumaDictionaryGetDataSizeMacro(pDictionary);
}

DECUMA_UNICODE decumaDictionaryGetUnicode(const DECUMA_HWR_DICTIONARY * pDictionary, DECUMA_HWR_DICTIONARY_REF currentRef)
{
	decumaAssert(pDictionary);
	decumaAssert(currentRef);
	return decumaDictionaryGetUnicodeMacro(pDictionary, currentRef);
}

int decumaDictionaryGetEndOfWord(const DECUMA_HWR_DICTIONARY * pDictionary, DECUMA_HWR_DICTIONARY_REF currentRef)
{
	decumaAssert(pDictionary);
	decumaAssert(currentRef);
#if defined(DECUMA_ASSERT_ENABLE) || defined(DECUMA_ASSERT_OVERRIDE)
	{
		int bHasChild = decumaDictionaryGetChild(pDictionary, currentRef) != NULL;
		int bEOW = decumaDictionaryGetEndOfWordMacro(pDictionary, currentRef);
		decumaAssert(  bEOW  || bHasChild);
	}
#endif
	return decumaDictionaryGetEndOfWordMacro(pDictionary, currentRef);
}

DECUMA_UINT8  decumaDictionaryGetNFreqClasses(const DECUMA_HWR_DICTIONARY * pDictionary)
{
	decumaAssert(pDictionary);
	return decumaDictionaryGetNFreqClassesMacro(pDictionary);
}

void decumaDictionaryGetRankInFreqClass(const DECUMA_HWR_DICTIONARY * pDictionary, DECUMA_UINT8 freqClass,
												 DECUMA_UINT32 * pMinRank, DECUMA_UINT32 * pMaxRank)
{
	decumaAssert(pDictionary);
	decumaAssert(pMinRank);
	decumaAssert(pMaxRank);
	decumaAssert(freqClass<decumaDictionaryGetNFreqClasses(pDictionary));

	decumaDictionaryGetRankInFreqClassMacro(pDictionary,freqClass,pMinRank,pMaxRank);
}

DECUMA_UINT8 decumaDictionaryGetSubTreeFreqClass(const DECUMA_HWR_DICTIONARY * pDictionary, DECUMA_HWR_DICTIONARY_REF currentRef)
{
	decumaAssert(pDictionary);

	return decumaDictionaryGetSubTreeFreqClassMacro(pDictionary,currentRef);
}

DECUMA_UINT8 decumaDictionaryGetWordFreqClass(const DECUMA_HWR_DICTIONARY * pDictionary, DECUMA_HWR_DICTIONARY_REF currentRef)
{
	decumaAssert(pDictionary);

	return decumaDictionaryGetWordFreqClassMacro(pDictionary,currentRef);
}

DECUMA_UINT32 decumaDictionaryGetNumberOfWords(const DECUMA_HWR_DICTIONARY * pDictionary)
{
	decumaAssert(pDictionary);
	return decumaDictionaryGetNumberOfWordsMacro(pDictionary);
}

 void decumaDictionaryGetSubTreeFreqClassMinMax(const DECUMA_HWR_DICTIONARY * pDictionary, 
	DECUMA_HWR_DICTIONARY_REF currentRef, DECUMA_UINT8 * pMin, DECUMA_UINT8 * pMax)
 {
	decumaAssert(pDictionary);

	decumaDictionaryGetSubTreeFreqClassMinMaxMacro(pDictionary,currentRef,pMin,pMax);
 }


#endif /* DEBUG */


DECUMA_HWR_DICTIONARY_REF decumaDictionaryGetSubstringRef(
	const DECUMA_HWR_DICTIONARY * pDictionary, DECUMA_HWR_DICTIONARY_REF currentRef, 
	const DECUMA_UNICODE * pUnicodes, DECUMA_UINT8 nUnicodes)
{
	decumaAssert(pDictionary);
	decumaAssert(pUnicodes);
	if ( pDictionary->getSubstringRef )
	{
		return pDictionary->getSubstringRef(pDictionary->pDictionaryData, currentRef, pUnicodes, nUnicodes);
	}
	else
	{
		DECUMA_UINT8 i;

		for ( i = 0; i < nUnicodes; i++ )
		{
			DECUMA_UNICODE currentUnicode;

			currentRef = decumaDictionaryGetChildMacro(pDictionary, currentRef);
			if ( !currentRef ) goto getSubstringRef_error;
			currentUnicode = decumaDictionaryGetUnicodeMacro(pDictionary, currentRef);

			while ( currentUnicode < pUnicodes[i] )
			{
				currentRef = decumaDictionaryGetSiblingMacro(pDictionary, currentRef);
				if ( !currentRef ) goto getSubstringRef_error;
				currentUnicode = decumaDictionaryGetUnicodeMacro(pDictionary, currentRef);
			}

			if ( currentUnicode != pUnicodes[i] )
				goto getSubstringRef_error;
		}

		return currentRef;

getSubstringRef_error:

		return NULL;
	}
}

DECUMA_UINT8 decumaDictionaryGetSubTreeFreqClassGivenMinMax(
	const DECUMA_HWR_DICTIONARY * pDictionary, 
	DECUMA_HWR_DICTIONARY_REF currentRef, DECUMA_UINT8 minRem, DECUMA_UINT8 maxRem)
{
	DECUMA_UINT8 bestFCMin, bestFCMax;
	SEARCH_STACK_ELEMENT stack[32];
	int n;

	decumaDictionaryGetSubTreeFreqClassMinMaxMacro(pDictionary,currentRef,&bestFCMin,&bestFCMax);
	
	/*Quick test before starting the recursive call */
	if (maxRem >= bestFCMin && minRem <= bestFCMax) 
	{
		return decumaDictionaryGetSubTreeFreqClassMacro(pDictionary,currentRef);
	}

	/*We cannot reach the node in the subtree with the best frequency class */
	/*We need to find out recursively what is the best we can reach, but we know that it is not better than pNode->bestFreqClass+1 */

	n=0;
	stack[0].nodeRef =currentRef;
	stack[0].minRem=minRem;
	stack[0].maxRem=maxRem;
	stack[0].bestSoFar=MAX_DECUMA_UINT8;
	stack[0].bestPossible=0;

	/* Do a depth-first traversal*/
	do
	{
		int bFoundBranchBest;
		do
		{
			decumaAssert(n+1 < sizeof(stack)/sizeof(stack[0]));
			bFoundBranchBest = nonRecGetSubTreeFreqClassGivenMinMax(pDictionary,&stack[n],&stack[n+1]);
		}
		while (!bFoundBranchBest && stack[n+1].nodeRef && ++n); /*Check child if we need to (and exists) //Only increase n when needed */
		
		if (n>0)
		{
			do
			{
				decumaAssert(stack[n].bestSoFar >= stack[n-1].bestPossible);
				if (stack[n].bestSoFar < stack[n-1].bestSoFar) stack[n-1].bestSoFar=stack[n].bestSoFar;

				/*Look for the next sibbling, if not found, traceback and search for siblings */
				/*Or if we have already found the best possible among the sibbling */

			} while ((stack[n].bestSoFar == stack[n-1].bestPossible || 
				(stack[n].nodeRef= decumaDictionaryGetSibling(pDictionary,stack[n].nodeRef))== NULL ) && 
				--n);
		}

		if (n>0)
		{
			/*We have reached a sibbling. Reset the value for the sibbling */
			stack[n].bestPossible = stack[n-1].bestPossible;
			/*We can reuse the other values from the previous siblings */
		}

	} while ( n > 0 );

	decumaAssert(stack[0].bestSoFar == 	
		recGetSubTreeFreqClassGivenMinMax(pDictionary,currentRef,minRem,maxRem,MAX_DECUMA_UINT8, 0));

	return stack[0].bestSoFar;
}





/*//////////////// Local functions //////////////////// */

/*Returns 1 if found - then the result can be read from pThisVals->bestSoFar */
/*Returns 0 if !found yet. If pChildVals->nodeRef == NULL no (suitable) child is found =>back-trace. */
/*otherwise (pNewArgs->nodeRef != NULL) the contents of pNewArgs is set for the suitable child. */
static int nonRecGetSubTreeFreqClassGivenMinMax(const DECUMA_HWR_DICTIONARY * pDictionary, 
	SEARCH_STACK_ELEMENT * pThisVals, SEARCH_STACK_ELEMENT * pChildVals)
{
	DECUMA_UINT8 bestFCmin, bestFCmax;
	DECUMA_UINT8 dictMin, dictMax;
	DECUMA_UINT8 bestFreqClass;

	decumaAssert(pThisVals->bestSoFar>pThisVals->bestPossible);
	decumaAssert(pThisVals->minRem<=pThisVals->maxRem);

	decumaDictionaryGetSubTreeFreqClassMinMaxMacro(pDictionary,pThisVals->nodeRef,&bestFCmin, &bestFCmax);
	bestFreqClass = decumaDictionaryGetSubTreeFreqClassMacro(pDictionary,pThisVals->nodeRef);

	if (pThisVals->maxRem >= bestFCmin && pThisVals->minRem <= bestFCmax) 
	{
		pThisVals->bestSoFar = bestFreqClass;
		return 1;
	}

	if (pThisVals->nodeRef && bestFreqClass >= pThisVals->bestSoFar) 
	{
		return 1; /*We will not find anything better than pThisVals->bestSoFar in this subtree */
	}

	decumaDictionaryGetMinMaxMacro(pDictionary,pThisVals->nodeRef,&dictMin, &dictMax);
	if (pThisVals->maxRem < dictMin || pThisVals->minRem > dictMax )
	{
		return 1; /*We will not find anything better than pThisVals->bestSoFar in this subtree */
	}

	/*Else, we cannot reach the node in the subtree with the best frequency class */
	/*Best reachable node for this subtree is at least one worse */
	pThisVals->bestPossible = bestFreqClass+1;
		
	if (pThisVals->nodeRef)
	{
		DECUMA_UINT8 thisFreqClass = decumaDictionaryGetWordFreqClassMacro(pDictionary,pThisVals->nodeRef);
		decumaAssert(pThisVals->minRem>0 || thisFreqClass >= pThisVals->bestPossible); /*It cannot be better */
		if (pThisVals->minRem== 0 && thisFreqClass < pThisVals->bestSoFar) pThisVals->bestSoFar=thisFreqClass;

		/*If bestSoFar is bestPossible,then we can return that. We will not reach anything better anyway. */
		if (pThisVals->bestSoFar == pThisVals->bestPossible) 
		{
			return 1; /*We will not find anything better than pThisVals->bestSoFar in this subtree */
		}
	}

	/*Else, We need to find out recursively from the child what is the best we can reach */
	pChildVals->nodeRef = NULL;
	if (pThisVals->maxRem>0)
	{
		DECUMA_HWR_DICTIONARY_REF childRef =
			decumaDictionaryGetChildMacro(pDictionary,pThisVals->nodeRef);
		if (childRef)
		{
			*pChildVals = *pThisVals;
			pChildVals->nodeRef = childRef;
			if (pChildVals->minRem>0) pChildVals->minRem--;
			pChildVals->maxRem--;
		}
	}

	return 0; /*We can not be sure that we have found a final value */
}

#if defined(DECUMA_ASSERT_ENABLE) || defined(DECUMA_ASSERT_OVERRIDE)
static DECUMA_UINT8 recGetSubTreeFreqClassGivenMinMax(const DECUMA_HWR_DICTIONARY * pDictionary, 
													  DECUMA_HWR_DICTIONARY_REF ref,
													  DECUMA_UINT8 minRem, DECUMA_UINT8 maxRem,
													  DECUMA_UINT8 bestSoFar, DECUMA_UINT8 bestPossible)
{
	DECUMA_HWR_DICTIONARY_REF newRef = 0;
	DECUMA_UINT8 bestFCmin, bestFCmax;
	DECUMA_UINT8 dictMin, dictMax;
	DECUMA_UINT8 bestFreqClass;
	decumaAssert(bestSoFar>bestPossible);
	decumaAssert(minRem<=maxRem);

	decumaDictionaryGetSubTreeFreqClassMinMaxMacro(pDictionary,ref,&bestFCmin, &bestFCmax);
	bestFreqClass = decumaDictionaryGetSubTreeFreqClassMacro(pDictionary,ref);

	if (maxRem >= bestFCmin && minRem <= bestFCmax) 
	{
		return bestFreqClass;
	}

	if (ref && bestFreqClass >= bestSoFar) return bestSoFar; /*We will not find anything better in this subtree */

	decumaDictionaryGetMinMaxMacro(pDictionary,ref,&dictMin, &dictMax);
	if (maxRem < dictMin || minRem > dictMax ) return bestSoFar;

	/*Else, we cannot reach the node in the subtree with the best frequency class */
	/*Best reachable node for this subtree is at least one worse */
	bestPossible = bestFreqClass+1;
		
	if (ref)
	{
		DECUMA_UINT8 thisFreqClass = decumaDictionaryGetWordFreqClassMacro(pDictionary,ref);
		decumaAssert(minRem>0 || thisFreqClass >= bestPossible); /*It cannot be better */
		if (minRem== 0 && thisFreqClass < bestSoFar) bestSoFar=thisFreqClass;

		/*If bestSoFar is bestPossible,then we can return that. We will not reach anything better anyway. */
		if (bestSoFar == bestPossible) return bestSoFar; /*We will not find anything better in this subtree */
	}
	/*Else, We need to find out recursively from the child what is the best we can reach */
	newRef = decumaDictionaryGetChildMacro(pDictionary,ref);

	if (newRef && maxRem>0)
	{
		/*Recurse */
		do 
		{
			DECUMA_UINT8 childFC = recGetSubTreeFreqClassGivenMinMax(pDictionary, newRef, (minRem>0? minRem-1:0), maxRem-1,
				bestSoFar,bestPossible);
			decumaAssert(childFC >= bestPossible);
			
			if (childFC == bestPossible) return childFC;
			if (childFC < bestSoFar) bestSoFar=childFC;
		}
		while ( (newRef = decumaDictionaryGetSiblingMacro(pDictionary,newRef)) );
	}
	return bestSoFar;
}
#endif /*defined(DECUMA_ASSERT_ENABLE) || defined(DECUMA_ASSERT_OVERRIDE) */


static void initWithTerseTrie(DECUMA_HWR_DICTIONARY * pDictionary, 
	DECUMA_TERSE_TRIE * pTerseTrie)
{
	decumaAssert(pDictionary);
	decumaAssert(pTerseTrie);

	decumaMemset(pDictionary, 0, sizeof(DECUMA_HWR_DICTIONARY));

	pDictionary->pDictionaryData = pTerseTrie;
	
	pDictionary->getChild = (DECUMA_HWR_DICTIONARY_GET_CHILD) decumaTerseTrieGetChild;
	pDictionary->getEndOfWord = (DECUMA_HWR_DICTIONARY_IS_EOW) decumaTerseTrieGetEndOfWord;
	pDictionary->getMinMax = (DECUMA_HWR_DICTIONARY_GET_MINMAX) decumaTerseTrieGetMinMax;
	pDictionary->getSize = (DECUMA_HWR_DICTIONARY_GET_SIZE) decumaTerseTrieGetSize;
	pDictionary->getSibling = (DECUMA_HWR_DICTIONARY_GET_SIBLING) decumaTerseTrieGetSibling;
	/*pDictionary->getSubstringRef = NULL; */
	pDictionary->getUnicode = (DECUMA_HWR_DICTIONARY_GET_UNICODE) decumaTerseTrieGetUnicode;
	pDictionary->getNumberOfWords = (DECUMA_HWR_DICTIONARY_GET_NWORDS) decumaTerseTrieGetNWords;

	pDictionary->getRankForFreq = (DECUMA_HWR_DICTIONARY_GET_RANK_IN_FREQCLASS) decumaTerseTrieGetRankInFreqClass;
	pDictionary->getSubTreeFreq = (DECUMA_HWR_DICTIONARY_GET_SUBTREE_FREQCLASS) decumaTerseTrieGetSubTreeFreqClass;
	pDictionary->getSubTreeFreqMinMax = (DECUMA_HWR_DICTIONARY_GET_SUBTREE_FREQCLASS_MINMAX) decumaTerseTrieGetSubTreeFreqClassMinMax;
	pDictionary->getNumberOfFreqs = (DECUMA_HWR_DICTIONARY_GET_N_FREQCLASSES) decumaTerseTrieGetNFreqClasses;
	pDictionary->getWordFreq = (DECUMA_HWR_DICTIONARY_GET_WORD_FREQCLASS) decumaTerseTrieGetWordFreqClass;
}

static void initWithTrie(DECUMA_HWR_DICTIONARY * pDictionary, 
	DECUMA_TRIE * pTrie)
{
	decumaAssert(pDictionary);
	decumaAssert(pTrie);

	decumaMemset(pDictionary, 0, sizeof(DECUMA_HWR_DICTIONARY));

	pDictionary->pDictionaryData = pTrie;

	pDictionary->getChild = (DECUMA_HWR_DICTIONARY_GET_CHILD) decumaTrieGetChild;
	pDictionary->getEndOfWord = (DECUMA_HWR_DICTIONARY_IS_EOW) decumaTrieGetEndOfWord;
	pDictionary->getMinMax = (DECUMA_HWR_DICTIONARY_GET_MINMAX) decumaTrieGetMinMax;
	pDictionary->getSize = (DECUMA_HWR_DICTIONARY_GET_SIZE) decumaTrieGetSize;
	pDictionary->getSibling = (DECUMA_HWR_DICTIONARY_GET_SIBLING) decumaTrieGetSibling;
	/*pDictionary->getSubstringRef = NULL; */
	pDictionary->getUnicode = (DECUMA_HWR_DICTIONARY_GET_UNICODE) decumaTrieGetUnicode;
	pDictionary->getNumberOfWords = (DECUMA_HWR_DICTIONARY_GET_NWORDS) decumaTrieGetNWords;
	
	pDictionary->getRankForFreq = (DECUMA_HWR_DICTIONARY_GET_RANK_IN_FREQCLASS) decumaTrieGetRankInFreqClass;
	pDictionary->getSubTreeFreq = (DECUMA_HWR_DICTIONARY_GET_SUBTREE_FREQCLASS) decumaTrieGetSubTreeFreqClass;
	pDictionary->getSubTreeFreqMinMax = (DECUMA_HWR_DICTIONARY_GET_SUBTREE_FREQCLASS_MINMAX) decumaTrieGetSubTreeFreqClassMinMax;
	pDictionary->getNumberOfFreqs = (DECUMA_HWR_DICTIONARY_GET_N_FREQCLASSES) decumaTrieGetNFreqClasses;
	pDictionary->getWordFreq = (DECUMA_HWR_DICTIONARY_GET_WORD_FREQCLASS) decumaTrieGetWordFreqClass;
}

static int decumaDictionaryIsInitializedTrie(DECUMA_HWR_DICTIONARY *pDictionary)
{
	return (pDictionary->getSize == (DECUMA_HWR_DICTIONARY_GET_SIZE) decumaTrieGetSize);
}


static int decumaDictionaryIsInitializedTerseTrie(DECUMA_HWR_DICTIONARY *pDictionary)
{
	return (pDictionary->getSize == (DECUMA_HWR_DICTIONARY_GET_SIZE) decumaTerseTrieGetSize);
}


