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

  This file contains private data structure definitions of
  aslRG objects.

\******************************************************************/

#ifndef ASL_RG_DATA_H
#define ASL_RG_DATA_H

#if defined(_DEBUG) && !defined(ASL_RG_C)
#error Direct data access in debug mode only allowed from aslRG.c
#endif

#include "aslConfig.h"

#include "decumaBasicTypes.h"
#include "decuma_point.h"
#include "decumaMemoryPool.h"
#include "decumaQCandHandler.h"
#include "decumaHashHandler.h"
#define MAX_GET_DIST_SYMBOL_LEN 32/*#include "decuma_hwr_extra.h" */
#include "aslSG.h"
#include "decumaDictionary.h" /*For DECUMA_HWR_DICTIONARY */

#ifndef MAX_STRINGS_PER_NODE
#if defined(SPEED_OPT_FOR_ASSUMED_WELL_TUNED_DB)
#define MAX_STRINGS_PER_NODE 80
#else
#define MAX_STRINGS_PER_NODE 100
#endif
#endif

#define NUM_DICTIONARIES	2	/* Two dictionaries right now; one static, one PD */
#define STATIC_DICTIONARY	0	/* index of the static dict */
#define PERSONAL_DICTIONARY	1	/* index of the personal dict */
/* These two parameters can be tuned for best speed/memory compromise */
/* Too low values will slow down the engine (redoing same calculation more) */
/* and too high values will use a lot of memory */
#define MAX_PRECALC_CONN_SYMB_NODES   10
#define MAX_DIST_TO_CONN_SYMB_NODE    100



/* An "entry" is a compound dictionary + reference structure */
typedef struct _ASL_RG_DICTIONARY_ENTRY
{
	DECUMA_HWR_DICTIONARY_REF	  pReference;
	DECUMA_UINT8                  dictIdx; /*The idx of the dictionary in pRG */
	DECUMA_UINT8				  nWordLen;
	DECUMA_UINT8                  nWordStart;
	DECUMA_UINT8                  bPunishableAlternativeSymbolExpansion;
	DECUMA_UINT16                 nLeftBoost;
	DECUMA_UINT16                 nLeftStart;  /*The nWordStart of the left word (stored after a restart and later communicated through API) */

} ASL_RG_DICTIONARY_ENTRY;


struct _ASL_RG_STRING
{
	/*NOTE: Many instances of this datatype is used in the engine. E.g. 8000. */
	/*      Therefore, minimize its size!! */
	/*      Try to keep alignment in mind when modifying this datatype. So that the */
	/*      amount of padding is minimized. */

	DECUMA_UINT8 lastEdgeIdx; /* Index of the edge coming to the node with this number in the SG. */

	/* Noderef:s pointing to corresponding node in the static and personal dictionaries, respectively. */
	ASL_RG_DICTIONARY_ENTRY * pDictionaryEntries; /* Array */
	int nDictionaryEntries;
	DECUMA_INT8 bestDictEntryIdx; /*The dictionary idx that was actually used for the best boost in this string (if any) */
	DECUMA_UINT8 bListOwner; /* Responsible for freeing */

	DECUMA_UINT8 nUnicodeCheckSum; /*Used for faster discrimination of different unicode strings */
	DECUMA_INT16 nStringUnicodes; /*The length of pStringUnicodes */

	DECUMA_UINT32 dist; /* The distance of this path. This is the "REAL" distance that is used for the sum when extending */
	                    /* this string with new characters. Within this node sorting the candidates the sortingDist is used. */
	long sortingDist; /*The sorting distance is used only locally when adding the string to the candidate handler */
	                           /*But it will not be used in the sum which makes up the distance for future strings extending */
	                           /*this string. For that purpose, see dist. */

	DECUMA_UNICODE * pStringUnicodes; /*Pointer to a buffer containing all the previous unicodes of this string and the unicode(s) of the last edge.  NOT zero-terminated */
	DECUMA_UINT8 nStringUnicodesAllocFlag; /*Flag saying how the allocation of the string unicodes buffer should be / is handled */
	DECUMA_UINT8 nLenOfForced; /*How much of the forced recognition word is covered in this string */

	DECUMA_INT8 prevStringIdx; /*The index of the string stored in the RG in the starting node of the  */
	                   /*edge pointing to this node */

	DECUMA_UINT16 nRankBoost1024;

	DECUMA_INT16   nStartNode;
	ASL_RG_STRING* pStartStr;

	DECUMA_INT8 nPrependedSymbols;
	/*JM TEMP */
	/*DECUMA_UINT8 bStrUsingAltSymb; */
	/*DECUMA_UINT8 bStrRestarted; */

#ifdef EXTRA_VERIFICATION_INFO
	int lastEdgeDist;
	int connectionDist;
#endif

};

typedef struct _NODE_PRECALC_DATA
{
	DECUMA_INT16 nMaxRemUnicodes;
	DECUMA_INT16 nMinRemUnicodes;
	DECUMA_INT16 nMaxRemLetters;  /*Letters are different from unicodes. 'abcABC...' are letters but gestures,punctuations,numbers,... are not */
	DECUMA_INT16 nMinRemLetters;
} NODE_PRECALC_DATA;

struct _ASL_RG_NODE
{
	ASL_RG_STRING* pStrings;

	int nStrings;
	int nStringLists;

	/*Store data about the future */
	NODE_PRECALC_DATA pcdEdge;

	DECUMA_UINT32 nNodeProgressionSqr1024;
	DECUMA_UINT32 nNodeAntiProgressionSqrt128;
	DECUMA_UINT32 nMaxLinguisticDistFactor128;
	int nMaxStrings;

	int nMaxUnicodesInNode;
	int nMaxLettersInNode;

	long nQualificationDist128;

	int bResetPreCalcArrays;

};

typedef struct _ASL_RG_SCRATCHPAD
{
	/* Temporary data storage needed during construction */

	/* --- Temporary storage of minDistances, that are used during calculations --- */

	long preCalculatedConnDists[MAX_EDGES_PER_NODE];

	DECUMA_UINT16 stringsRanking[MAX_STRINGS_PER_NODE];

	ASL_RG_STRING* pOldStrings;
	int nOldStrings;

	ASL_RG_STRING* pTmpStrings;
	int nTmpStrings;

} ASL_RG_SCRATCHPAD;

typedef struct _ASL_RG_DICTIONARY_DATA
{
	/*Extra dynamic data that RG wants to store for each dictionary */
	DECUMA_UINT16 * pFreqClassBoost; /*Calculated "member boost" for each frequency class */
	                                 /*Allocated array of size = dictionaryGetNFreqClasses */

} ASL_RG_DICTIONARY_DATA;

struct _ASL_RG
{
	const DECUMA_MEM_FUNCTIONS* pMemFunctions;

	const DECUMA_CHARACTER_SET* pCharacterSet;
	CATEGORY_TABLE_PTR pCatTable;
	ASL_SG * pSG;
	const ASL_ARC_SESSION* pArcSession;

	ASL_RG_NODE** pNodes;

	int nAddedNodes;
	int nNodes;

	DECUMA_QCH * pStringCandHandler;    /*Handles the addition and sorting of string candidates in one node */
	DECUMA_QCH_CANDREF candRefs[MAX_STRINGS_PER_NODE];
	DECUMA_HH * pStringCheckSumHandler;        /*Handles string checksum search */
	BOOST_LEVEL boostLevel;               /*Should we use the dictionary for hitrate boosting */
	STRING_COMPLETENESS strCompleteness;       /*Are we only looking for complete words? */
	WRITING_DIRECTION writingDirection;        /*Right-to-left or left-to-right? */
	RECOGNITION_MODE recognitionMode;          /*SCR,MCR or ASL mode. TODO: MCR mode is not yet supported in RG */

	DECUMA_UNICODE * pStringStart;             /*Are we continuing an existing string? */
	int nStringStartLen;
	int bStringStartHasLetter;
	ASL_RG_STRING stringStart;

	/* Dictionary parameters: */
	DECUMA_HWR_DICTIONARY ** pAllDictionaries;
	DECUMA_UINT8 nAllDictionaries;
	DECUMA_UINT8 nMaxDictWordLen;
	ASL_RG_DICTIONARY_DATA * pDictionaryData;  /*Will be allocated to an array of ASL_RG_DICTIONARY_DATAs of length=nAllDictionaries */
	                                           /*Extra dynamic data that RG wants to store for each dictionary */

	ASL_RG_DICTIONARY_ENTRY * pInitialDictEntries;

	DECUMA_UINT16 nMaxRankBoost1024;

	int bUseForcedRecognition;                 /*Are we trying to force the recognition towards a special word */
	DECUMA_UNICODE forcedRecognition[MAX_GET_DIST_SYMBOL_LEN+1]; /*Zero terminated unicode string of the forced word. */
	int strlenForced;                          /*The strlen of forcedRecognition */

	const DECUMA_HWR_RESULT* pForceResult;
	int nForceResultLen;

	DECUMA_UNICODE * pDictionaryFilterStr;
	int nDictionaryFilterStrLen;

	/*Use memory pools to store the StringUnicodes instead of having to statically allocate */
	/*If not using memory pools. This could mean a lot of mallocs which are done in the inner loop */
	/*of RG. The memory pools are more time efficient. But take up a little more memory */
	/*Allocate new memory pools when needed */
	TMemoryPoolHandler * pStringMemPoolHandler;
	TMemoryPool ** ppStringMemPools;
	int nStringMemPools;

	TMemoryPoolHandler * pDictMemPoolHandler;
	TMemoryPool ** ppDictMemPools;
	int nDictMemPools;

	CATEGORY thaiLetterCat;
	CATEGORY thaiNextLetterShouldBeOntopCat;
	CATEGORY thaiShouldBeOntopPrevLetterCat;

	CATEGORY cyrillicLetterCat;

	CATEGORY cat;

	ASL_RG_SCRATCHPAD *pScratchPad;

	int nScratchPads;

	int bIsCorrupt;

};

#endif /*ASL_RG_DATA_H */
