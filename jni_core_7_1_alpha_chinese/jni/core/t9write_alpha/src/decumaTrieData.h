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

#ifndef DECUMA_TRIE_DATA_H
#define DECUMA_TRIE_DATA_H

#if defined(DEBUG) && !(defined(DECUMA_TRIE_C) || defined(DECUMA_TERSE_TRIE_C)) /*Exception */
#error Only decumaTrie.c and decumaTerseTrie.c should need the data definition
#endif

#include "decumaBasicTypes.h" /* Definition of DECUMA_UINT32 */
#include "decuma_hwr.h"		  /* Definition of DECUMA_UNICODE */
#include "decumaDictionaryBinary.h"

/* For XT9 Arabic, we get a trie of 150196 nodes = 2403152 bytes 
 * This is exacly equal to having a trie definition of 14 bytes = 16 with padding + 150196 nodes of 16 bytes
 * Shortening the trie node definition to use a 8-bit value for unicode (and then look it up) plus reducing the bestUpcoming to an 8-bit rank value,
 * we should get a trie size of 1802352 + 20 bytes (assuming we can just add a pointer to a lookup table in the trie struct). 
 *
 * 150K is too large to use 16-bit skip values (i.e. use an offset rather than a pointer), so we might as well keep the pointers.
 */


#define DECUMA_TRIE_MAX_DEPTH 254  /*MAX_DECUMA_UINT8-1 since we use DECUMA_UINT8 for some values  */
                                   /*and MAX_DECUMA_UINT8 is used to indicate empty node */

typedef struct _DECUMA_TRIE_HEADER
{
	DECUMA_UINT32  nNodes;		/* Total number of used nodes */
	DECUMA_UINT8   nMin;		/* Length of the shortest word in this trie */
	DECUMA_UINT8   nMax;		/* Length of the longest word in this trie */
	DECUMA_UINT8   nFC0Min;		/* Length of the shortest word in this trie with frequency class==0*/
	DECUMA_UINT8   nFC0Max;		/* Length of the longest word in this trie with frequency class ==0*/
	DECUMA_TRIE_NODE * pFirst;	/* Pointer to first node in trie */
	DECUMA_UINT32 nNodeBufferSize; /*The number of nodes that fit in one node buffer */
	DECUMA_UINT32 nNodesInNewNodeBuf; /*The number of used nodes in the last node buffer */
	DECUMA_UINT32 nNodesInOldNodeBuf; /*The number of used nodes in the 2nd last node buffer */
	DECUMA_UINT32 nNodeBuffers;    /*The number of node buffers */
	DECUMA_TRIE_NODE ** ppNodeBuffers; /* Array of pointers to the starts of the (unstructured) buffers of nodes */
	DECUMA_UINT32  nWords;      /* The total number of words in the trie */
	DECUMA_UINT8   nMaxFreqClasses;   /* The maximum number of different freq classes in the trie */
	DECUMA_UINT8   nFreqClasses;      /* The actual number of different freq classes in the trie */
	DECUMA_UINT32 * pFreqClassTable; /* pFreqClassTable[i] is the minimum word rank for a word with freq class = i. 
								   An allocated array of size=nFreqClasses */
} DECUMA_TRIE_HEADER;

struct _DECUMA_TRIE_NODE {
	DECUMA_UNICODE unicode;			/* Unicode value */
	DECUMA_UINT8   nMin;			/* Least number of nodes until a complete word */
	DECUMA_UINT8   nMax;			/* Most number of nodes until a complete word */
	DECUMA_UINT8  thisFreqClass;   /* FreqClass of this word */
	DECUMA_UINT8  bestFreqClass;	/* Best FreqClass of all words reachable from this node (including itself) */
	DECUMA_UINT8  bestFCMin;        /* The least number of nodes to a complete word with the freq class==bestFreqClass */
	DECUMA_UINT8  bestFCMax;        /* The most number of nodes to a complete word with the freq class==bestFreqClass */
	DECUMA_TRIE_NODE * pSibling;	/* Pointer to sibling */
	DECUMA_TRIE_NODE * pChild;		/* Pointer to child */
};

struct _DECUMA_TRIE 
{
	char binaryHeader[DECUMA_DICTIONARY_BINARY_HEADER_SIZE];
	DECUMA_TRIE_HEADER trieHeader;
};

struct _DECUMA_TRIE_ITERATOR
{
	const DECUMA_TRIE * pTrie;
	DECUMA_UINT32 nodeBufferIdx;
	DECUMA_UINT32 nodeIdxInBuffer;
	DECUMA_UINT32 nodeIdx;
};


#endif /* #ifdef DECUMA_TRIE_DATA_H */
