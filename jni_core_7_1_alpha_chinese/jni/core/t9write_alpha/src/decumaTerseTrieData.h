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
  by a terse trie. (A compact and serialized trie, tree) 
    
\******************************************************************/

#ifndef DECUMA_TERSE_TRIE_DATA_H
#define DECUMA_TERSE_TRIE_DATA_H


#if defined(DEBUG) && !defined(DECUMA_TERSE_TRIE_C)
#error Only decumaTerseTrie.c should need the data definition
#endif

#include "decumaBasicTypes.h" /* Definition of DECUMA_UINT32 */
#include "decumaDictionaryBinary.h"


/* Type definitions for a terse trie -- this is what's used in the more compact trie variant */
struct _DECUMA_TERSE_TRIE_DATA {
	/*-- 32-bit aligned */
	DECUMA_UNICODE unicode;
	DECUMA_UINT8 min;
	DECUMA_UINT8 max;
	/*-- 32-bit aligned */
	DECUMA_UINT8 thisFreqClass;
	DECUMA_UINT8 bestFreqClass;
	DECUMA_UINT8  bestFCMin;      /* The least number of nodes to a complete word with the freq class==bestFreqClass */
	DECUMA_UINT8  bestFCMax;      /* The most number of nodes to a complete word with the freq class==bestFreqClass */
	/*-- 32-bit aligned */
};

struct _DECUMA_TERSE_TRIE_NODE {
	DECUMA_UINT16 nodeData;      /* Offset into an array of DECUMA_TERSE_NODE_DATA structures */
								 /* Note that this assumes that we never have more than 65536 unique node datas */
	DECUMA_UINT16 siblingOffset; /* Offset of next sibling from this node */
								 /* Note that this assumes that we never have an offset from one node to 
								 its sibbling of more than 65536 nodes */
	/*-- 32-bit aligned */
};

typedef struct _DECUMA_TERSE_TRIE_HEADER {
	DECUMA_UINT32 nNodes;
	DECUMA_UINT32 nDataElements;
	DECUMA_UINT32 nWords;
	/*-- 32-bit aligned */
	DECUMA_UINT8  nMin;		/* Length of the shortest word in this trie */
	DECUMA_UINT8  nMax;		/* Length of the longest word in this trie */
	DECUMA_UINT8  nFC0Min;		/* Length of the shortest word in this trie with frequency class==0*/
	DECUMA_UINT8  nFC0Max;		/* Length of the longest word in this trie with frequency class ==0*/
	/*-- 32-bit aligned */
	DECUMA_UINT8  nFreqClasses;      /* The number of different freq classese in the trie */
	DECUMA_UINT8  dummy1;  /*To get byte alignment correct for all platforms */
	DECUMA_UINT16 dummy2;  /*To get byte alignment correct for all platforms */
	/*-- 32-bit aligned */
	/*Offsets are from the address of this DECUMA_TERSE_TRIE_HEADER  */
	DECUMA_UINT32 freqClassTableOffset; /*Gives pFreqClassTable */
	DECUMA_UINT32 firstNodeOffset;   /*Gives pFirst */
	DECUMA_UINT32 dataBufferOffset; /*Gives pDataBuffer */
} DECUMA_TERSE_TRIE_HEADER;

struct _DECUMA_TERSE_TRIE 
{
	char binaryHeader[DECUMA_DICTIONARY_BINARY_HEADER_SIZE];
	DECUMA_TERSE_TRIE_HEADER trieHeader;
};



#endif /*DECUMA_TERSE_TRIE_DATA_H */
