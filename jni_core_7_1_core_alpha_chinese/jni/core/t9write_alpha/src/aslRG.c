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

  This file implements aslRG.h. It uses dynamic memory
  allocation.
  
\******************************************************************/

#define ASL_RG_C

#include "aslRG.h"
#include "aslRGMacros.h"
#include "aslConfig.h"

#include "decumaAssert.h"
#include "decumaMemory.h"
#include "decumaQCandHandler.h"
#include "decumaHashHandler.h"
#include "aslRGData.h"
#include "aslTools.h"
#include "aslSG.h"
#include "decumaString.h"
#include "decumaMemoryPool.h"
#include "decumaSymbolCategories.h"
#include "decumaLanguages.h"
#include "decumaCategoryTranslation.h"

#include "decumaCommon.h"
#include "decumaTrigramTable.h"
#include "decumaModid.h"

#include <math.h>

/*#define DEBUG_OUTPUT */
#ifdef DEBUG_OUTPUT
#include <stdio.h>
FILE * g_debugFile;
#endif


#ifdef ARM_PROFILING_EXTRA
#include "ARM_profiling_extra.h"
#define PROF(func) PRF_##func()
#define PROF_IF( cond, func) if (cond) {PRF_##func();}
#define PROF_IF_ELSE( cond, func_if, func_else) if (cond) {PRF_##func_if();} else {PRF_##func_else();}
#else
/*Define PROF-macros as empty when not profiling */
#define PROF(a)
#define PROF_IF(a,b)
#define PROF_IF_ELSE(a,b,c)
#endif

/*//////////////////////////////////// */
/* Private constant definitions */
/* */

#define FIX_UNKNOWN_WRITING_DIRECTION_CONN_DIST				200
#define THAI_CONN_DIST_OFFSET							  (-250)
#define CYRILLIC_CONN_DIST_OFFSET						  (-200)

#define SCALE_AND_VERTICAL_OFFSET_DIST_WEIGHT64				 48 /* To be diveded by 64 */
#define ROTATION_DIST_WEIGHT64								 48

#define SCALE_AND_VERTICAL_OFFSET_CONN_DIST_WEIGHT64		 80
#define ROTATION_CONN_DIST_WEIGHT64							 32


/*#define NO_EARLY_CONCLUSION_FOR_SPEED */

#define MAX_WORD_LEN 50  /*The maximum number of characters per word. This is used for forced recognition */

#define MAX_MEMORY_POOLS 20 /*This is far more than needed... But we want to make sure that this does not set the limit */

/* String unicodes allocation enum */
#define UNICODES_NEED_ALLOCATION 1
#define UNICODES_ARE_ALLOCATED 2
#define UNICODES_ALLOCATION_TRIED_WITH_ERROR 3


#define MAX_STRINGS_OF_SAME_CLASS            1

#define DICT_MEM_POOL_SIZE 10000 /*granularity for dictionary reference allocations // 32768 = 32 kB , 4096 = 4kB */

#define PUNISHABLE_ALTERNATIVE_SYMBOL_EXPANSION_PUNISH128		8

/*//////////////////////////////////// */
/* Private macro definitions */
/* */

#ifdef _DEBUG
#define DEBUG_EXEC(statement)
/*Sets correct default values. If no default value exists for a member, it is set to an invalid value */
#define INIT_STRING(pStr) debugInitString(pStr) 
#else
#define DEBUG_EXEC(statement)
/*Lazy initialization. Use 0 as default value for all members. Other values need to be set at a later stage */
#define INIT_STRING(pStr) initString(pStr)
#endif

#ifdef ASL_HIDDEN_API

#define DEBUG_REPORT(errorCode,pRG,bPartOfRefBestPath,nodeIdx,edgeIdx,strIdx,pliftIdx,dist) \
	debugReport(pRG,bPartOfRefBestPath,errorCode,nodeIdx,edgeIdx,strIdx,pliftIdx,dist)

#define FINALIZE_DEBUG_REPORT(pRG) if ((pRG)->pRefRG){ finalizeDebugOutput(pRG, (pRG)->pDebugOutput);}

#else
/*Empty definitions */
#define DEBUG_REPORT(errorCode,pRG,bPartOfRefBestPath,nodeIdx,edgeIdx,strIdx,pliftIdx,dist)  
#define FINALIZE_DEBUG_REPORT(pRG)
#endif

#define MAX_OF_2(a,b)   ( (a)>=(b) ? (a) : (b))
#define MAX_OF_3(a,b,c) ( MAX_OF_2(MAX_OF_2((a),(b)),(c)) )
#define MAX_OF_4(a,b,c,d) ( MAX_OF_2(MAX_OF_3((a),(b),(c)),(c)) )

#define MIN_OF_2(a,b)   ( (a)<=(b) ? (a) : (b))
#define MIN_OF_3(a,b,c) ( MIN_OF_2(MIN_OF_2((a),(b)),(c)) )
#define MIN_OF_4(a,b,c,d) ( MIN_OF_2(MIN_OF_3((a),(b),(c)),(c)) )


#define candidatesHaveSameSymbols(_nStringUnicodes, _stringUnicodes, _nUnicodeCheckSum,_pStr2) \
( \
	(\
		(_nUnicodeCheckSum) == (_pStr2)->nUnicodeCheckSum && \
		(_nStringUnicodes) == (_pStr2)->nStringUnicodes && \
		decumaMemcmp((_stringUnicodes), (_pStr2)->pStringUnicodes, (_nStringUnicodes) * sizeof((_pStr2)->pStringUnicodes[0]) ) == 0 \
	)\
)

/* Dictionary nodes are considered diffing if both exists and are not equal. */
/* They need to be tested since they can be diffing when it comes to contractions, trailers etc. */
#define candidatesHaveSameStartStringFirstDictionaryNode(_pStr1, _pStr2) \
( \
	(\
		(_pStr1)->pDictionaryEntries == NULL && (_pStr2)->pDictionaryEntries == NULL || \
		(_pStr1)->pDictionaryEntries && (_pStr2)->pDictionaryEntries && \
		(_pStr1)->pDictionaryEntries[0].pReference == (_pStr2)->pDictionaryEntries[0].pReference \
	)\
)



#define initEdgeDescriptorMacro(_pRG, _nodeIdx, _edgeIdx, _pRgString, _pDesc) \
{ \
	decumaAssert((_nodeIdx)<aslSGGetNodeCount((_pRG)->pSG));\
	decumaAssert((_nodeIdx)>=0);\
\
	(_pDesc)->nodeIdx = (_nodeIdx);\
	(_pDesc)->pNode = aslSGGetNode((_pRG)->pSG,_nodeIdx);\
	(_pDesc)->edgeIdx = (_edgeIdx);\
\
	decumaAssert((_edgeIdx)< aslSGGetEdgeCount((_pDesc)->pNode) && aslSGGetEdgeCount((_pDesc)->pNode));\
	(_pDesc)->pEdge = aslSGGetEdge((_pDesc)->pNode,(_edgeIdx));\
}

#define initRGStringDescriptorMacro(_pRG, _nodeIdx, _stringIdx, _pDesc) \
{ \
	decumaAssert((_nodeIdx)<(_pRG)->nNodes);\
	decumaAssert((_nodeIdx)>=0);\
\
	(_pDesc)->nodeIdx = (_nodeIdx);\
	(_pDesc)->pNode = (_pRG)->pNodes[(_nodeIdx)];\
\
	decumaAssert((_stringIdx)<(_pDesc)->pNode->nStrings);\
	decumaAssert((_stringIdx)>=0);\
\
	(_pDesc)->stringIdx = (_stringIdx);\
	(_pDesc)->pString = &(_pDesc)->pNode->pStrings[(_stringIdx)];\
}

#define restCannotContainUnicodesMacro(_pRgStartNode, _pRgNode, _thisEdgeClass) \
	((_pRgStartNode)->pcdEdge.nMaxRemUnicodes == 0 ||\
	 (_pRgNode)->pcdEdge.nMaxRemUnicodes == 0 || \
	 (_pRgNode)->pcdEdge.nMaxRemUnicodes == MIN_DECUMA_INT16)


#define maxUnicodesInRGMacro(pRG) ((pRG)->pNodes[0]->pcdEdge.nMaxRemUnicodes)
#define maxLettersInRGMacro(pRG) ((pRG)->pNodes[0]->pcdEdge.nMaxRemLetters) /*With letters we mean dictionary unicodes */

/* Please note that the contract for this macro has changed; it now offers the feature of building a checksum incrementally when just appending strings. */
#define getUnicodeCheckSumMacro(nCheckSum, nOldCheckSum, pUnicodes, nUnicodes) \
{\
	int i;\
\
	(nCheckSum) = (nOldCheckSum) + (DECUMA_UINT8) (8*(nUnicodes));\
	for (i=0; i<(nUnicodes); i++)\
	{\
		(nCheckSum) = (DECUMA_UINT8) ((nCheckSum) + (DECUMA_UINT8) ((pUnicodes)[i]));\
	}\
}


#define updatePrecalcMinMaxValues( pStartNodePCD, pNodePCD, nMinNewUnicodes, nMaxNewUnicodes,nMinNewLetters, nMaxNewLetters ) \
{	\
	(pStartNodePCD)->nMaxRemUnicodes = (DECUMA_INT16) \
		((pNodePCD)->nMaxRemUnicodes == MIN_DECUMA_INT16 ? (pStartNodePCD)->nMaxRemUnicodes : \
		MAX_OF_2( (pStartNodePCD)->nMaxRemUnicodes,(pNodePCD)->nMaxRemUnicodes + (nMaxNewUnicodes))); \
\
	(pStartNodePCD)->nMinRemUnicodes = (DECUMA_INT16) \
		((pNodePCD)->nMinRemUnicodes == MAX_DECUMA_INT16 ? (pStartNodePCD)->nMinRemUnicodes : \
		MIN_OF_2( (pStartNodePCD)->nMinRemUnicodes,(pNodePCD)->nMinRemUnicodes + (nMinNewUnicodes))); \
\
	(pStartNodePCD)->nMaxRemLetters = (DECUMA_INT16) \
		((pNodePCD)->nMaxRemLetters == MIN_DECUMA_INT16 ? (pStartNodePCD)->nMaxRemLetters : \
		MAX_OF_2( (pStartNodePCD)->nMaxRemLetters,(pNodePCD)->nMaxRemLetters + (nMaxNewLetters))); \
\
	(pStartNodePCD)->nMinRemLetters = (DECUMA_INT16) \
		((pNodePCD)->nMinRemLetters == MAX_DECUMA_INT16 ? (pStartNodePCD)->nMinRemLetters : \
		MIN_OF_2( (pStartNodePCD)->nMinRemLetters,(pNodePCD)->nMinRemLetters + (nMinNewLetters))); \
}

#define VALID_PRECALCDATA( pPCD, nMaxUnicodes, nMaxLetters ) ( \
	(( (pPCD)->nMaxRemUnicodes >= 0 && (pPCD)->nMaxRemUnicodes <= (nMaxUnicodes) ) || (pPCD)->nMaxRemUnicodes == MIN_DECUMA_INT16 ) && \
	(( (pPCD)->nMinRemUnicodes >= 0 && (pPCD)->nMinRemUnicodes <= (nMaxUnicodes) ) || (pPCD)->nMinRemUnicodes == MAX_DECUMA_INT16 ) && \
	(( (pPCD)->nMaxRemLetters >= 0 && (pPCD)->nMaxRemLetters <= (nMaxLetters) ) || (pPCD)->nMaxRemLetters == MIN_DECUMA_INT16 ) && \
	(( (pPCD)->nMinRemLetters >= 0 && (pPCD)->nMinRemLetters <= (nMaxLetters) ) || (pPCD)->nMinRemLetters == MAX_DECUMA_INT16 ) )


/*We try to allocate from the memory pool. If we don' get any memory, */
/*we are uut of memory in the memoryPoolHandler. We need to add a new pool. */
/*Allocate a new pool and add to handler. Then try again */
#define DICTIONARY_REF_ALLOC( _ppList, _pRG, _size, _pStatus) \
{\
	*_ppList = MEMORYPOOLHANDLER_ALLOC(_pRG->pDictMemPoolHandler, sizeof(ZI_DICTIONARY_LIST), 0); \
	if (!*_ppList) \
	{\
		*_pStatus = allocNewDictMemPool(_pRG);\
		if (*_pStatus==decumaNoError)\
		{ \
			*_ppList = MEMORYPOOLHANDLER_ALLOC(_pRG->pDictMemPoolHandler, sizeof(ZI_DICTIONARY_LIST), 0); \
			decumaAssert(_ppList); \
		} \
	}\
}

#define STR_HAS_TRAILER(nStrUnicodes,pDictEntry) \
	((pDictEntry)->nWordLen && (nStrUnicodes)> (pDictEntry)->nWordStart + (pDictEntry)->nWordLen)

#define STR_HAS_CONTRACTED(pDictEntry) ((pDictEntry)->nLeftBoost > 0)

#define GET_DICTWORD_START_AND_LEN(_pEntry,_pStartIdx,_pLen) \
{ \
	*(_pStartIdx) = (_pEntry)->nWordStart; \
	*(_pLen) = (_pEntry)->nWordLen + ((_pEntry)->nLeftBoost ? (_pEntry)->nWordStart-(_pEntry)->nLeftStart : 0); \
}



/*Datatypes used for easier parameter passing */
typedef struct _EDGE_DESCRIPTOR
{
	int nodeIdx;
	int edgeIdx; /*Arrival index of edge to this node */
	const ASL_SG_NODE * pNode;
	const ASL_SG_EDGE * pEdge;
} EDGE_DESCRIPTOR;

typedef struct _RG_STRING_DESCRIPTOR
{
	int nodeIdx;
	int stringIdx;
	ASL_RG_NODE * pNode;
	ASL_RG_STRING * pString;
} RG_STRING_DESCRIPTOR;



/*/////////////////////////////////////////////////////////////// */
/* Private function declarations */
/* */

static void rgRelease(ASL_RG* pRG);

static DECUMA_STATUS createAndQualifyNewString(ASL_RG *pRG,
											   int nNodeIdx,
											   int nUnicodeIdx,
											   int nEdgeIdx,
											   int nStartStrIdx,
											   ASL_RG_STRING* pStartStr,
											   DECUMA_UNICODE symbol,
											   int nUnicodesInEdge,
											   int bContractionExpansion,
											   int bAlternativeDictSymbol,
											   int bExpandNonDictSymbols,
											   int bRestartDictString,
											   int bUseHorisontalConnDist,
											   int* pbCancelEdge);

static void addNewString(ASL_RG* pRG,
						 ASL_RG_NODE* pNode,
						 ASL_RG_STRING* pStr,
						 const DECUMA_QCH_CANDREF* replaceeCandidateRef);

static long getConnectionDist(const ASL_RG * pRG,
							  int nNodeIdx,
							  int nEdgeIdx,
							  int nStartStrIdx,
							  int bUseHorisontalConnDist);

static DECUMA_STATUS checkWithDictionary(ASL_RG * pRG,
	const DECUMA_UNICODE * pThisUnicodes, 
	DECUMA_UINT8 nThisUnicodes,
	const ASL_SG_EDGE * pThisEdge, 
	const ASL_RG_STRING * pStartStr, 
	const ASL_RG_NODE * pRgNode,
	ASL_RG_DICTIONARY_ENTRY ** ppNewDictEntries,
	int * pnNewDictEntries,
	DECUMA_UINT8 *pbIsListOwner,
	int bTrigramCheckOnly,
	int bContractionExpansion,
	int bAlternativeDictSymbol,
	int bExpandNonDictSymbols,
	int bRestartDictString);

static DECUMA_UINT16 getRankBoost1024(const ASL_RG * pRG,
									  ASL_RG_DICTIONARY_ENTRY * pDictionaryEntries, 
									  int nDictionaryEntries,
									  DECUMA_UNICODE* pStringUnicodes,
									  int nStringUnicodes,
									  const ASL_RG_NODE * pRgNode,
									  int nIgnoredStringStartLen,
									  DECUMA_INT8 * pBestDictEntryIdx);
static DECUMA_UINT32 getLingDistFactor128(DECUMA_UINT32 nRankBoost1024, DECUMA_UINT32 nNodeProgressionSqr1024);

static void freeDictionaryList(ASL_RG * pRG, ASL_RG_DICTIONARY_ENTRY * pList);

static void searchCandidateReplacee(ASL_RG* pRG,
									DECUMA_QCH* pCH,
									ASL_RG_STRING* pStr,
									DECUMA_UINT8 nCheckSum,
									ASL_RG_STRING** ppReplaceeCandidate,
									DECUMA_HH_ELEMREF* pReplaceeElemRef,
									int *pnReplaceeDist,
									int bLastNode);

static DECUMA_STRING_TYPE rgStringGetType(const ASL_RG * pRG, const ASL_RG_STRING* pStr,
										  DECUMA_INT16 * pStartIdx, DECUMA_INT16 * pLen);

static int rgStringGetStartNode(const ASL_RG * pRG, const ASL_RG_STRING * pRgString, int nodeIdx);

static void rgStringSetStart(ASL_RG * pRG, ASL_RG_STRING * pRgString, int nodeIdx);

static void preRemovalFunc(ASL_RG * pRG, ASL_RG_STRING * pStr);

static DECUMA_STATUS preProcessRG(ASL_RG * pRG);

static DECUMA_STATUS allocAndAddMemPool(const ASL_RG * pRG, TMemoryPoolHandler * pMemPoolHandler, TMemoryPoolLayout * pMemPoolLayouts, int nMemPoolLayouts, 
										TMemoryPool *** pppMemPools, int *pnMemPools);
										
static DECUMA_STATUS allocNewStringUnicodesMemPool(ASL_RG * pRG, int minStrLen, int maxStrLen);
static DECUMA_STATUS allocNewDictMemPool(ASL_RG * pRG);

static ASL_RG_NODE* rgAddNode(ASL_RG* pRG);
static void rgRemoveNode(ASL_RG* pRG);

static ASL_RG_STRING* rgNodeAddStringList(const ASL_RG * pRG, ASL_RG_NODE* pNode, ASL_RG_STRING* pStringList, int nListLen);
static void rgNodeRemoveStringList(const ASL_RG * pRG, ASL_RG_NODE* pNode);

static ASL_RG_SCRATCHPAD* rgAddScratchPad(ASL_RG* pRG);
static void rgRemoveScratchPad(ASL_RG* pRG);

static void initString(ASL_RG_STRING * pStr);

static int getMaxDictWordLen(const ASL_RG * pRG);

#ifdef _DEBUG
static void debugInitString(ASL_RG_STRING * pStr);
#endif /*_DEBUG */

static void preInsertFunc(ASL_RG * pRG, ASL_RG_STRING * pStr);
/*// */


#ifdef _DEBUG_EXTRA

static int restCannotContainSymbols(const ASL_RG_NODE * pRgStartNode, 
	const ASL_RG_NODE * pRgNode, int thisEdgeClass);


#endif /*_DEBUG_EXTRA */


/*/////////////////////////////////////////////////////////////////////////////////// */
/* Public function definitions */
/* */



int aslRGGetSize(void)
{
	return sizeof(ASL_RG) + decumaQCHGetSize() + MemoryPoolHandler_GetSize() + MemoryPoolHandler_GetSize() +
		0;
}

void aslRGInit(ASL_RG* pRG, 
			   const DECUMA_MEM_FUNCTIONS* pMemFunctions,
			   const DECUMA_CHARACTER_SET* pCharacterSet,
			   CATEGORY_TABLE_PTR pCatTable,
			   ASL_SG * pSG,
               const ASL_ARC_SESSION* pArcSession,
			   DECUMA_HWR_DICTIONARY ** pDictionaries,
			   int nDictionaries,
			   BOOST_LEVEL boostLevel,
			   STRING_COMPLETENESS strCompleteness,
			   DECUMA_UNICODE * pStringStart,
			   WRITING_DIRECTION writingDirection,
			   RECOGNITION_MODE recognitionMode,
			   DECUMA_UNICODE * pDictionaryFilterStr,
			   int nDictionaryFilterStrLen,
			   const DECUMA_UNICODE * forcedRecognitionStr,
			   const DECUMA_HWR_RESULT * pForceResult,
			   int nForceResultLen)
{
	DECUMA_UINT32 symbCatThaiLetter = DECUMA_CATEGORY_THAI;
	DECUMA_UINT32 symbCatCyrillicLetter = DECUMA_CATEGORY_CYRILLIC;
	DECUMA_UINT32 symbCatThaiNextLetterShouldBeOntop = DECUMA_CATEGORY_THAI_BELOW_VOWELS;
	DECUMA_UINT32 symbCatThaiShouldBeOntopPrevLetter[3] = {DECUMA_CATEGORY_THAI_ABOVE_VOWELS, DECUMA_CATEGORY_THAI_DIACRITICS, DECUMA_CATEGORY_THAI_TONES};
	DECUMA_UINT32 langCatThai = DECUMA_LANG_TH;
	DECUMA_UINT32 langCatCyrillic[4] = {DECUMA_LANG_RU, DECUMA_LANG_UK, DECUMA_LANG_BG, DECUMA_LANG_SRCY};
	DECUMA_CHARACTER_SET charSet;
	DECUMA_STATUS status;

	decumaAssert(pRG);
	decumaAssert(pMemFunctions);
	decumaAssert(pCharacterSet);
	decumaAssert(pCatTable);
	decumaAssert(pSG);
	decumaAssert(pArcSession);
	decumaAssert(pRG->pStringCandHandler!=(DECUMA_QCH*) &pRG[1]); /*Already initialized */

	decumaMemset(pRG, 0, sizeof(pRG[0]));

	pRG->pMemFunctions = pMemFunctions;
	pRG->pCharacterSet = pCharacterSet;
	pRG->pCatTable = pCatTable;
	pRG->pSG = pSG;
	pRG->pArcSession = pArcSession;

	/*Use the memory immediately after the ASL_RG struct for the  */
	/*candidate handler */
	pRG->pStringCandHandler = (DECUMA_QCH *) &pRG[1];

	/*Use the memory immediately after that to the string mem pool handler */
	pRG->pStringMemPoolHandler = (TMemoryPoolHandler *) ((char*)pRG->pStringCandHandler + decumaQCHGetSize());

	MemoryPoolHandler_Init(MODID_HWRLIB, pRG->pStringMemPoolHandler);

	/*Use the memory immediately after that to the string mem pool handler */
	pRG->pDictMemPoolHandler = (TMemoryPoolHandler *) ((char*)pRG->pStringMemPoolHandler + MemoryPoolHandler_GetSize());

	MemoryPoolHandler_Init(MODID_HWRLIB, pRG->pDictMemPoolHandler);

	pRG->boostLevel = boostLevel;

	pRG->nMaxRankBoost1024 = 0;

	if ( pRG->boostLevel != noBoost )
	{
		if ( !pDictionaries || !nDictionaries )
		{
			/* The caller wants to use dictionary-based search, but does not 
			 * provide any. */
			pRG->boostLevel = noBoost;
		}
		else
		{
			decumaAssert(nDictionaries < MAX_DECUMA_UINT8);
			pRG->pAllDictionaries = pDictionaries;
			pRG->nAllDictionaries = nDictionaries;
			pRG->nMaxDictWordLen = getMaxDictWordLen(pRG);
		}
	}

	pRG->strCompleteness = strCompleteness;
	pRG->writingDirection = writingDirection;
	pRG->recognitionMode = recognitionMode;

	pRG->pStringStart = pStringStart;
	while (pStringStart && pStringStart[pRG->nStringStartLen])
	{
		pRG->bStringStartHasLetter |= decumaUnicodeIsLetter(pStringStart[pRG->nStringStartLen]);
		pRG->nStringStartLen++;
	}

	pRG->pDictionaryFilterStr = pDictionaryFilterStr;
	pRG->nDictionaryFilterStrLen = nDictionaryFilterStrLen;

	if (forcedRecognitionStr && forcedRecognitionStr != pRG->forcedRecognition) /* Latter check in case of re-init */
	{
		int i;

		for (i=0; forcedRecognitionStr[i]!=0 && 
			i<sizeof(pRG->forcedRecognition)/sizeof(pRG->forcedRecognition[0])-1; i++)
		{
			pRG->forcedRecognition[i]=forcedRecognitionStr[i];
		}
		pRG->forcedRecognition[i]=0;
		pRG->bUseForcedRecognition=1;
		pRG->strlenForced=i;
	}

	pRG->pForceResult = pForceResult;
	pRG->nForceResultLen = nForceResultLen;

	charSet.pSymbolCategories = &symbCatThaiLetter;
	charSet.nSymbolCategories = 1;
	charSet.pLanguages = &langCatThai;
	charSet.nLanguages = 1;

	status = translateToCategoryStructs(&charSet,pCatTable,NULL,&pRG->thaiLetterCat,NULL);
	/* Note status might not be ok if Thai is not supported by the DB. That is fine, */
	/* just make sure everything is still as expected. */
	decumaAssert(status == decumaNoError || decumaInvalidCategory &&
		CATEGORY_MASK_IS_EMPTY(pRG->thaiLetterCat.symbolCat) &&
		CATEGORY_MASK_IS_EMPTY(pRG->thaiLetterCat.languageCat));

	charSet.pSymbolCategories = &symbCatThaiNextLetterShouldBeOntop;
	charSet.nSymbolCategories = 1;
	charSet.pLanguages = &langCatThai;
	charSet.nLanguages = 1;

	status = translateToCategoryStructs(&charSet,pCatTable,NULL,&pRG->thaiNextLetterShouldBeOntopCat,NULL);
	/* Note status might not be ok if Thai is not supported by the DB. That is fine, */
	/* just make sure everything is still as expected. */
	decumaAssert(status == decumaNoError || decumaInvalidCategory &&
		CATEGORY_MASK_IS_EMPTY(pRG->thaiNextLetterShouldBeOntopCat.symbolCat) &&
		CATEGORY_MASK_IS_EMPTY(pRG->thaiNextLetterShouldBeOntopCat.languageCat));

	charSet.pSymbolCategories = symbCatThaiShouldBeOntopPrevLetter;
	charSet.nSymbolCategories = 3;
	charSet.pLanguages = &langCatThai;
	charSet.nLanguages = 1;

	status = translateToCategoryStructs(&charSet,pCatTable,NULL,&pRG->thaiShouldBeOntopPrevLetterCat,NULL);
	/* Note status might not be ok if Thai is not supported by the DB. That is fine, */
	/* just make sure everything is still as expected. */
	decumaAssert(status == decumaNoError || decumaInvalidCategory &&
		CATEGORY_MASK_IS_EMPTY(pRG->thaiShouldBeOntopPrevLetterCat.symbolCat) &&
		CATEGORY_MASK_IS_EMPTY(pRG->thaiShouldBeOntopPrevLetterCat.languageCat));

	charSet.pSymbolCategories = &symbCatCyrillicLetter;
	charSet.nSymbolCategories = 1;
	charSet.pLanguages = langCatCyrillic;
	charSet.nLanguages = 4;

	status = translateToCategoryStructs(&charSet,pCatTable,NULL,&pRG->cyrillicLetterCat,NULL);
	/* Note status might not be ok if Thai is not supported by the DB. That is fine, */
	/* just make sure everything is still as expected. */
	decumaAssert(status == decumaNoError || decumaInvalidCategory &&
		CATEGORY_MASK_IS_EMPTY(pRG->cyrillicLetterCat.symbolCat) &&
		CATEGORY_MASK_IS_EMPTY(pRG->cyrillicLetterCat.languageCat));

	status = translateToCategoryStructs(pCharacterSet,pCatTable,NULL,&pRG->cat,NULL);
	decumaAssert(status == decumaNoError);

}

int aslRGIsCorrupt(const ASL_RG* pRG)
{
	return pRG->bIsCorrupt;
}

void aslRGDestroy(ASL_RG* pRG)
{
	aslSGSetStringStart(pRG->pSG, NULL, 0);
	rgRelease(pRG);
	decumaMemset(pRG,0,sizeof(ASL_RG));
}

DECUMA_STATUS aslRGConstruct(ASL_RG* pRG, int bUseHorisontalConnDist)
{
	const DECUMA_MEM_FUNCTIONS* pMemFunctions = pRG->pMemFunctions;
	int nodeIdx;
	int nNodesInSG, nCriticalNode, nMaxStringsPerNode;
	DECUMA_STATUS status=decumaNoError;
	ASL_RG_STRING * pNullStr = NULL; /*Used for macros */
	
	decumaAssert(pRG);
	decumaAssert(pRG->pStringCandHandler==(DECUMA_QCH*) &pRG[1]); /*Initialized */

	/* Allocate tmp memory only needed during construction */
	if (!rgAddScratchPad(pRG)) goto aslRGConstruct_error;

	nNodesInSG = aslSGGetNodeCount(pRG->pSG);
	if (nNodesInSG <= 1) goto aslRGConstruct_exit; /*No arcs added */


	/* Create new RG nodes for new SG nodes */
	for (nodeIdx = pRG->nAddedNodes; nodeIdx < nNodesInSG; nodeIdx++)
	{
		if(!rgAddNode(pRG)) goto aslRGConstruct_error;
	}

	status = preProcessRG(pRG);

	if (status != decumaNoError) goto aslRGConstruct_error;

	pRG->nNodes = 0;
	
	if (pRG->pNodes[0]->pcdEdge.nMaxRemUnicodes<=0) goto aslRGConstruct_exit; /*We can not get any characters with this SG */

	/* ------- Allocate for the first string unicode memory pool, more can later be alloced upon need ----- */
	status = allocNewStringUnicodesMemPool(pRG, 
		1, /*The minimum number of characters is 1  */
		pRG->pNodes[0]->pcdEdge.nMaxRemUnicodes /*The maximum number of characters in a string. NOTE that nMaxRemUnicodes in node 0 includes nStringStartLen */
	); 
	if (status!=decumaNoError) goto aslRGConstruct_error;

	status = allocNewDictMemPool(pRG); /*Create the mempool for the dictionary lists. And add to mem pool handler */
	if (status!=decumaNoError) goto aslRGConstruct_error;	
	
	decumaAssert(pRG->pStringCheckSumHandler == 0);
	pRG->pStringCheckSumHandler = aslCalloc(decumaHHGetSize(MAX_STRINGS_PER_NODE,decumaHHSizeSmall));
	if (!pRG->pStringCheckSumHandler) goto aslRGConstruct_error;

	if ( pRG->boostLevel != noBoost )
	{
		double maxRankBoost = 0.0;
		DECUMA_UINT16 nExpectedUnicodes;
		int i;

		pRG->pDictionaryData = aslCalloc(pRG->nAllDictionaries*sizeof(ASL_RG_DICTIONARY_DATA));
		if (!pRG->pDictionaryData) goto aslRGConstruct_error;	

		pRG->pInitialDictEntries = aslCalloc(pRG->nAllDictionaries*sizeof(ASL_RG_DICTIONARY_ENTRY));
		if (!pRG->pInitialDictEntries) goto aslRGConstruct_error;	

		aslSGGetBestString(pRG->pSG, NULL, 0, NULL, &nExpectedUnicodes, 0, NULL, 0, NULL, NULL, NULL, 0, NULL, NULL, NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL); /* Let best SG string length set the expectation */
		for ( i = 0; i < pRG->nAllDictionaries; i++ )
		{
			DECUMA_HWR_DICTIONARY * pDictionary = pRG->pAllDictionaries[i];
			DECUMA_UINT8 c,nClasses;

			/* This is a smooth, careful ranking function. The parameters 0.378 and 0.425 have been tuned */
			/* for optimal performance with one 135263 words dictionary case and one 135163 + one 4963 words */
			/* dictionary case. The test data coverage of the dictionaries represented the dictionaries expected */
			/* average coverage. */
			/* => [1, 10^8.86] -> [0.4, 0.0], in reality no neg boost (must be close to one in a billion dict word for that) */
			/* Note that the addition of 100 to the rank limits the maximum boost, but has little effect on boost of less */
			/* frequent words. */
			/* */
			/* NOTE: Note the average rank used to be calculated as half the word count (=max rank), since this equaled */
			/* the average before rank offset support. This has been changed to actual rank now that dictionary format */
			/* supports non overlapping compisite dictionaries. This results in lower rank value of unfrequent words. */
			/* Since the rankBoost function was tuned for the previous bad rank estimate we now as a quick fix applies */
			/* an adjustment to the relative rank by doubling this value (4 left shifts instead of 5 previously). */
			/* */
			nClasses = decumaDictionaryGetNFreqClasses(pDictionary);
			pRG->pDictionaryData[i].pFreqClassBoost = aslCalloc(nClasses*sizeof(DECUMA_UINT16));
			if (!pRG->pDictionaryData[i].pFreqClassBoost) goto aslRGConstruct_error;	

			for (c=0; c<nClasses; c++)
			{
				DECUMA_UINT32 minRank,maxRank;
				DECUMA_UINT32 averageRank;
				DECUMA_UINT32 relativeRank;
				double rankBoost;
				decumaDictionaryGetRankInFreqClass(pDictionary,c, &minRank, &maxRank);
				averageRank = (minRank+maxRank)/2;
				relativeRank = (averageRank << 4) >> nExpectedUnicodes; /* Adjust rank impact based on expected word length (long words are allowed great boost for dictionary match) */
				rankBoost = 1.0 - 0.420 * pow(log10(relativeRank + 100) + 1, 0.420);
				/* 0.420 and 0.420 tuned for best compromise on generated English letter data (ASLLIB v0.0.94.0) */

				pRG->pDictionaryData[i].pFreqClassBoost[c]=(DECUMA_UINT16) (1024 * rankBoost);
				
				if (rankBoost > maxRankBoost) maxRankBoost = rankBoost;
			}

			/*Initialize the pInitialDictEntries member */
			decumaMemset(&pRG->pInitialDictEntries[i],0,sizeof(pRG->pInitialDictEntries[i]));
			pRG->pInitialDictEntries[i].dictIdx = i;
		}

		decumaAssert(maxRankBoost >= 0.0 && maxRankBoost <= 1.0);
		pRG->nMaxRankBoost1024 = (DECUMA_UINT16) (1024 * maxRankBoost); /* Max punish equal no boost */
	}

	/* TODO Hard coded node indeces has been adjusted for arcs instead of segments connecting nodes, */
	/* but just as an initial guess. These should be tuned! */

	/* Critical node upper limit */
	if (pRG->boostLevel == noBoost)
	{
		nCriticalNode = 6;
	}
	else if (pRG->boostLevel == boostDictWords)
	{
		nCriticalNode = 4;
	}
	else
	{
		decumaAssert(pRG->boostLevel == forceDictWords);
		nCriticalNode = 4;
	}

	/* If there are a lot of nodes the max strings need in the critical node is reduced (less recognition rate */
	/* gained per extra calculations carried out for the extra string slots) */
	if (nNodesInSG > 9)
		nMaxStringsPerNode = MAX_STRINGS_PER_NODE * (9 * 9) / (nNodesInSG * nNodesInSG);
	else
		nMaxStringsPerNode = MAX_STRINGS_PER_NODE;

	/* Lower limit independent of MAX_STRINGS_PER_NODE for best performance */
	if (nMaxStringsPerNode < 20) nMaxStringsPerNode = 20;

	/* If not exceeding the critical node upper limit the mid node is the optimal critical node */
	/* otherwise it is the upper limit. In the dictionary case this is more true than in the non-dictionary */
	/* case where the upper limit more works as a sensible speed compromise. */
	nCriticalNode = ASL_MIN(nCriticalNode, nNodesInSG / 2);

	/* ------------------------ */
	/* Loop through all nodes */
	/* ------------------------ */
	for (nodeIdx=0; nodeIdx<nNodesInSG; nodeIdx++) /*0 can constain string start edge */
	{
		const ASL_SG_NODE * pSgNode = aslSGGetNode(pRG->pSG, nodeIdx);
		ASL_RG_NODE * pRgNode = pRG->pNodes[nodeIdx];
		int unicodeIdx;
		int nEdges = aslSGGetEdgeCount(pSgNode);
		int edgeIdx;
		int nCandidates = 0;
		int nSGMaxPunishForNeedReduction = 0;
		int nCriticalNodeDist;
		int nCombRankLimitNonPenLift, nCombRankLimitPenLift;
		int i;

		decumaAssert(pRgNode);

		pRG->nNodes=nodeIdx+1;

		PROF(TNoValidCharPath); /*JM: 2007-11-14: Here we are about 20 times/word */
		/*Can we already from preprocess conclude that this node is unnecessary to look at, since it is not going to be involved in any valid path? */
		if (pRgNode->pcdEdge.nMaxRemUnicodes == MIN_DECUMA_INT16)
		{
			DEBUG_REPORT(rgNoValidCharPath,pRG,nodeIsInBestPathOfRefRG(pRG,nodeIdx),nodeIdx,-1,-1,-1,0);

			PROF(NoValidCharPath); /*JM: 2007-11-02: Here we end up: 0% */
			decumaAssert(pRgNode->pcdEdge.nMinRemUnicodes== MAX_DECUMA_INT16);			
			/*Skip this node. It will not be involved in any valid path anyway */
			continue;
		}

		nCriticalNodeDist = 64 * nodeIdx / nCriticalNode - 64;
		pRgNode->nMaxStrings = nMaxStringsPerNode - (nMaxStringsPerNode-1) * nCriticalNodeDist * nCriticalNodeDist / (64 * 64);

		if (pRgNode->nMaxStrings < 20)
		{
			/* Use a lower limit for two main purposes (same limit is suitable for both) */
			/* 1. Keep a minimum number of strings in the final node (do not cut the end result unnecessarily) */
			/* 2. Avoid eliminating to many diacritic options in early nodes */
			pRgNode->nMaxStrings = 20;
		}

		if (pRgNode->nMaxStrings > MAX_STRINGS_PER_NODE)
		{
			decumaAssert(0);
			pRgNode->nMaxStrings = MAX_STRINGS_PER_NODE;
		}

		if (nodeIdx == 0) pRgNode->nMaxStrings = MAX_STRINGS_PER_NODE;

/*#define NO_GUESSES_FOR_SPEED */
#ifdef NO_GUESSES_FOR_SPEED
		pRgNode->nMaxStrings = MAX_STRINGS_PER_NODE;
#endif

		/* The profile for the requirement on combined rank limit is naturally dependent on the nMaxStrings. */
		/* Besides that it shows the following profile: */
		/* 1. Up to critical node a fast (exponential) growth from 0 to top value */
		/* 2. After the critical node the requirement is more constant */
		if (nodeIdx < nCriticalNode)
		{
			nCombRankLimitNonPenLift = 3 * pRgNode->nMaxStrings * pRgNode->nMaxStrings / nMaxStringsPerNode;
			nCombRankLimitPenLift = 4 * pRgNode->nMaxStrings * pRgNode->nMaxStrings / nMaxStringsPerNode;
		}
		else
		{
			nCombRankLimitNonPenLift = 3 * pRgNode->nMaxStrings;
			nCombRankLimitPenLift = 4 * pRgNode->nMaxStrings;
		}

		if (!rgNodeAddStringList(pRG, pRgNode, NULL, pRgNode->nMaxStrings)) goto aslRGConstruct_error;

		decumaQCHInit(pRG->pStringCandHandler, pRG->candRefs, pRgNode->pStrings, 
			pRgNode->nMaxStrings,
			sizeof(pRgNode->pStrings[0]),(char*)&pRgNode->pStrings[0].sortingDist - (char*)&pRgNode->pStrings[0]);

		decumaHHInit(pRG->pStringCheckSumHandler, pRgNode->nMaxStrings,decumaHHSizeSmall);

		pRgNode->nNodeProgressionSqr1024 = 1024 * nodeIdx / nNodesInSG * nodeIdx / nNodesInSG;
		pRgNode->nMaxLinguisticDistFactor128 = getLingDistFactor128(pRG->nMaxRankBoost1024, pRgNode->nNodeProgressionSqr1024);

/*		pRgNode->nNodeAntiProgressionSqrt128 = isqrt(128 * 128 * (nNodesInSG - nodeIdx) / nNodesInSG); */
		pRgNode->nNodeAntiProgressionSqrt128 = (DECUMA_UINT32) (sqrt(128 * 128 * (nNodesInSG - nodeIdx) / nNodesInSG));

		pRgNode->nQualificationDist128 = DECUMA_MAX_DISTANCE * 128;

		for (unicodeIdx = 0; unicodeIdx == 0 || unicodeIdx < pRgNode->nMaxUnicodesInNode; unicodeIdx++)
		{
			if (unicodeIdx > 0)
			{
				/* We need to make a copy of the string list in this node, since this list */
				/* should be continued rather than the start string list */


				if (pRgNode->nStrings == 0) break; /* Nothing to continue */

				decumaAssert(!pRG->pScratchPad->pTmpStrings);
				pRG->pScratchPad->nTmpStrings = 0;
				pRG->pScratchPad->pTmpStrings = aslCalloc(pRgNode->nStrings * sizeof(pRG->pScratchPad->pTmpStrings[0]));

				if (!pRG->pScratchPad->pTmpStrings) goto aslRGConstruct_error;

				/* Copy old list before reseting */
				for (i = 0; i < pRgNode->nStrings; i++)
				{
					pRG->pScratchPad->pTmpStrings[i] = pRgNode->pStrings[i];
					pRG->pScratchPad->nTmpStrings++;
				}

				/* Remove old string list for this node */
				rgNodeRemoveStringList(pRG, pRgNode);

				/* Create a new string list for this node */
				if (!rgNodeAddStringList(pRG, pRgNode, NULL, pRgNode->nMaxStrings)) goto aslRGConstruct_error;

				decumaQCHInit(pRG->pStringCandHandler, pRG->candRefs, pRgNode->pStrings, 
					pRgNode->nMaxStrings,
					sizeof(pRgNode->pStrings[0]),(char*)&pRgNode->pStrings[0].sortingDist - (char*)&pRgNode->pStrings[0]);

				decumaHHInit(pRG->pStringCheckSumHandler, pRgNode->nMaxStrings,decumaHHSizeSmall);

				if (pRG->pScratchPad->pOldStrings)
				{
					for (i = 0; i < pRG->pScratchPad->nOldStrings; i++) preRemovalFunc(pRG, &pRG->pScratchPad->pOldStrings[i]);

					aslFree(pRG->pScratchPad->pOldStrings);
				}

				pRG->pScratchPad->nOldStrings = 0;
				pRG->pScratchPad->pOldStrings = aslCalloc(pRG->pScratchPad->nTmpStrings * sizeof(pRG->pScratchPad->pOldStrings[0]));

				if (!pRG->pScratchPad->pOldStrings) goto aslRGConstruct_error;

				/* Copy old list before reseting */
				for (i = 0; i < pRG->pScratchPad->nTmpStrings; i++)
				{
					const ASL_RG_STRING* pStr = &pRG->pScratchPad->pTmpStrings[i];
					const ASL_SG_EDGE* pEdge = aslSGGetEdge(pSgNode, pStr->lastEdgeIdx);
					const DECUMA_UNICODE* pEdgeUnicodes;
					int nEdgeUnicodes = 0;

					pEdgeUnicodes = aslSGGetSymbol(pEdge);

					while (pEdgeUnicodes[nEdgeUnicodes]) nEdgeUnicodes++;

					if (unicodeIdx >= nEdgeUnicodes)
					{
						/* This string is finished. Add it directly to handlers and continue. */
						const DECUMA_QCH_CANDREF * replaceeCandidateRef = NULL;

						decumaQCHAdd(pRG->pStringCandHandler, pStr->sortingDist, pStr, &replaceeCandidateRef);
						decumaAssert(replaceeCandidateRef);
						decumaHHAdd(pRG->pStringCheckSumHandler, pStr->nUnicodeCheckSum, decumaQCHGetCand(replaceeCandidateRef));
						
						continue;
					}
					
					pRG->pScratchPad->pOldStrings[pRG->pScratchPad->nOldStrings] = pRG->pScratchPad->pTmpStrings[i];
					pRG->pScratchPad->nOldStrings++;
				}

				pRgNode->nStrings = decumaQCHGetCount(pRG->pStringCandHandler);

				aslFree(pRG->pScratchPad->pTmpStrings);
				pRG->pScratchPad->nTmpStrings = 0;
			}


			/* ------------------------ */
			/* Loop all edges to current node and add to available strings */
			/* ------------------------ */

			for (edgeIdx=0; edgeIdx < nEdges; edgeIdx++ )
			{
				EDGE_DESCRIPTOR thisEdgeD;
				int startNodeIdx; 

				ASL_RG_STRING* pPrevStrings;
				int nPrevStrings;
				
				int thisEdgeDist=0;
				const DECUMA_UNICODE* thisEdgeSymbol;
				const DECUMA_UNICODE * theseUnicodePtrs[1]; /*Store all the possible unicode-ptrs for this edge */
				DECUMA_UINT8 theseNOFUnicodes[1]; /*Store all the possible number of unicodes for this edge */
				DECUMA_UNICODE symbol = 0;
				int bContractionExpansion, bAlternativeDictSymbol, bExpandNonDictSymbols, bRestartDictString;
				int bIsLetter;

				initEdgeDescriptorMacro(pRG,nodeIdx,edgeIdx,  pNullStr,&thisEdgeD);

				thisEdgeSymbol = aslSGGetSymbol(thisEdgeD.pEdge);
				thisEdgeDist = aslSGGetDistance(thisEdgeD.pEdge);

/*#define ALLOW_NON_ENDING_GESTURES */
#ifndef ALLOW_NON_ENDING_GESTURES
				/* Forbid gestures to preceed anything in the result */
				/* TODO Allow gestures to preceed gestures or noise, but not non-gestures? Can't see why now. */
				if (nodeIdx != nNodesInSG-1 && nodeIdx > 0 && thisEdgeSymbol && thisEdgeSymbol[0])
				{
					int bIsGesture, bIsInstantGesture;

					status = scrOutputIsGesture(aslSGGetOutput(thisEdgeD.pEdge), &bIsGesture, &bIsInstantGesture);

					if (status == decumaNoError)
					{
						decumaAssert(bIsGesture || !bIsInstantGesture);

						if (bIsGesture) continue;
					}
					else
					{
						decumaAssert(0);
					}
				}
#endif

				theseUnicodePtrs[0]=thisEdgeSymbol;

				theseNOFUnicodes[0] = 0;

				while(thisEdgeSymbol && thisEdgeSymbol[theseNOFUnicodes[0]]) theseNOFUnicodes[0]++;
					
				startNodeIdx = aslSGGetStartNode(thisEdgeD.pEdge);

				if (unicodeIdx > 0)
				{
					nPrevStrings = pRG->pScratchPad->nOldStrings;
					pPrevStrings = nPrevStrings ? pRG->pScratchPad->pOldStrings : NULL;
				}
				else if (nodeIdx > 0)
				{
					nPrevStrings = pRG->pNodes[startNodeIdx]->nStrings;
					pPrevStrings = nPrevStrings ? pRG->pNodes[startNodeIdx]->pStrings : NULL;
				}
				else
				{
					nPrevStrings = 0;
					pPrevStrings = NULL;
				}

				if (unicodeIdx > 0 && theseNOFUnicodes[0] <= unicodeIdx)
				{
					/* This edge does not have enough unicodes for unicodeIdx. Just add the old strings with this edge. */
					int strIdx;

					for (strIdx = 0; strIdx < nPrevStrings; strIdx++)
					{
						ASL_RG_STRING newAslString;

						if (pPrevStrings[strIdx].lastEdgeIdx != edgeIdx) continue;

						newAslString = pPrevStrings[strIdx];
						newAslString.nStringUnicodesAllocFlag = UNICODES_NEED_ALLOCATION;
						preInsertFunc(pRG, &newAslString);

						addNewString(pRG, pRgNode, &newAslString, NULL);
					}
					
					continue; 
				}
				
				symbol = thisEdgeSymbol[unicodeIdx];

				bIsLetter = decumaUnicodeIsLetter(symbol);

				pRgNode->bResetPreCalcArrays = 0;

				for (bContractionExpansion = 0; bContractionExpansion <= 1; bContractionExpansion++)
				{
					if (bContractionExpansion && pRG->boostLevel == noBoost) continue; /* Nothing to contract */

					if (bContractionExpansion && nodeIdx == 0 && unicodeIdx == 0) continue;

#define ASL_ALLOW_CONTRACTIONS
#ifndef ASL_ALLOW_CONTRACTIONS
					if (bContractionExpansion) continue;
#endif

					if (bContractionExpansion && !symbol) continue; /* Don't contract noise */

					/*Leader-trailer handling */
					for (bExpandNonDictSymbols = 0; bExpandNonDictSymbols <= 1; bExpandNonDictSymbols++)
					{
						if (bExpandNonDictSymbols)
						{
							if (pRG->boostLevel == noBoost) continue; /* Nothing to expand */

							if (bContractionExpansion) continue; /* Paths cannot be expanded when contracting */

							if (bIsLetter) continue; /* A non-letter is required to expand dictionary paths */
						}

						/* Test both the edge symbol and an alternative dictionary representation of */
						/* the edge symbol. For now this applies to start of sentense letter capitalization */
						/* not being covered by dictionary. */
						for (bAlternativeDictSymbol = 0; bAlternativeDictSymbol <= 1; bAlternativeDictSymbol++)
						{
							/*We have a start string to extend with this new edge or with a penlift and the new edge */
							const ASL_RG_NODE * pEdgeStartRgNode = pRG->pNodes[startNodeIdx];			
							int strIdx;
							EDGE_DESCRIPTOR * pPrevEdgeD = NULL;

							if (bAlternativeDictSymbol && pRG->boostLevel == noBoost) continue; /* Nothing to alternate */

							if (bAlternativeDictSymbol && !bIsLetter) continue;

							PROF(TNoValidCharPath4);
							if (pEdgeStartRgNode->pcdEdge.nMaxRemUnicodes == MIN_DECUMA_INT16)
							{
								DEBUG_REPORT(rgNoValidCharPath,pRG, edgeIsInBestPathOfRefRG(pRG,nodeIdx,edgeIdx),nodeIdx,edgeIdx,-1,-1,0);
								PROF(NoValidCharPath4);/*JM: 2007-11-02: Here we end up: 0% */

								decumaAssert(pEdgeStartRgNode->pcdEdge.nMinRemUnicodes == MAX_DECUMA_INT16);
								continue; /*It will not be involved in any valid path anyway */
							}

							for (bRestartDictString = 0; bRestartDictString <= 1; bRestartDictString++)
							{
								if (bRestartDictString && pRG->boostLevel == noBoost) continue; /* No point in restarting if we are not boosting */

								if (bRestartDictString && nodeIdx == 0) continue; /* Nothing to restart */

								if (bRestartDictString && (startNodeIdx > 0 || unicodeIdx > 0)) continue; /* Only the string start can be restarted */

								if (bRestartDictString && pRG->nStringStartLen == 0) continue; /* No string start to restart */

								if (bRestartDictString && bContractionExpansion) continue; /* Cannot both contract and restarting */

								for (strIdx = 0; startNodeIdx == 0 && unicodeIdx == 0 && strIdx == 0 || strIdx<nPrevStrings; strIdx++)
								{
									ASL_RG_STRING * pStartStr = NULL;
									int bCancelEdge = 0;

#ifndef NO_GUESSES_FOR_SPEED
									/* Check the combined rank of the start string and the edge to be connected */
									/* and break if it is to high. Ideally the new string candidates would also */
									/* be evaluated (and inserted in the string list) according to this rank order */
									/* to reduce evaluation effort of bad candidates. */
									if (nodeIdx > 0 && unicodeIdx == 0 && (edgeIdx+1) * (strIdx+1) > nCombRankLimitNonPenLift) break;
#endif

									if (startNodeIdx == 0 && unicodeIdx == 0)
									{
										if (bRestartDictString && pRG->nStringStartLen == 0)  continue; /* Nothing to restart */

#ifdef FORCE_RESTART_STRING_START
										if (!bRestartDictString && pRG->nStringStartLen > 0) continue;
#endif

#ifdef DO_NOT_RESTART_STRING_START
										if (bRestartDictString) continue;
#endif
									}

									pStartStr = pPrevStrings ? &pPrevStrings[strIdx] : NULL;

									if (unicodeIdx > 0 && pStartStr->lastEdgeIdx != edgeIdx) continue; /* Don't continue with another edge! */

									if (pStartStr &&
										symbol == '\'' &&
										pStartStr->pStringUnicodes[pStartStr->nStringUnicodes-1] == '\'')
									{
										/* Don't allow two consecutive ' in a dict path where one of them is part of the dict entry and one is not, */
										/* since ' at the end and start of a dict entry with high probability mean that it is a contraction and */
										/* therefore should both be used for non-contractions. Although we forbidden these entries from non-contraction */
										/* dict matching we do forbid that for this special case since '' is a forbidden bigram */
										if (pStartStr->pDictionaryEntries && pStartStr->pDictionaryEntries[0].nWordLen == 0 && !bExpandNonDictSymbols)  goto aslRGConstruct_strDone;	
										if (pStartStr->pDictionaryEntries && pStartStr->pDictionaryEntries[0].nWordLen > 0 &&
											!STR_HAS_TRAILER(pStartStr->nStringUnicodes,&pStartStr->pDictionaryEntries[0]) && bExpandNonDictSymbols)  goto aslRGConstruct_strDone;	
									}

									if (bContractionExpansion)
									{
										int d, bOkToContract = 0;

										if (!pStartStr) continue; /* Nothing to contract */

										/* We need to have a startstring with a dictionary path to allow contraction */
										if (pStartStr->nStringUnicodes == 0) goto aslRGConstruct_strDone;										
										if (pStartStr->pDictionaryEntries == NULL) goto aslRGConstruct_strDone; /* TODO .com start should be allowed for strings without dict match */

										/* One of either thisEdgeSymbol OR the last symbol in startStr must be '  */

										/* Must fullfill exactly one of left side contraction or right side contraction symbol condition */
										/* TODO Other symbols can contract, e.g. . for .com etc. */
										if (symbol != '\'' &&
											pStartStr->pStringUnicodes[pStartStr->nStringUnicodes-1] != '\'' ||
											symbol == '\'' &&
											pStartStr->pStringUnicodes[pStartStr->nStringUnicodes-1] == '\'') goto aslRGConstruct_strDone;

										for (d=0; d<pStartStr->nDictionaryEntries; d++)
										{
											ASL_RG_DICTIONARY_ENTRY * pEntry = &pStartStr->pDictionaryEntries[d];


											/* The start string cannot be contracted already. Do not allow multiple contractions! */
											/* The start string cannot have a trailer, when contracting */
											if (pEntry->nWordLen > 0 &&
												!STR_HAS_CONTRACTED(pEntry) &&
												!STR_HAS_TRAILER(pStartStr->nStringUnicodes,pEntry) &&
												decumaDictionaryGetEndOfWord(pRG->pAllDictionaries[pEntry->dictIdx],pEntry->pReference)) 
											{
												/* Complete, no previous contraction, no leader and no trailer. Ok to contract this one. */
												bOkToContract = 1;
												break; 
											}
										}
										if (!bOkToContract) goto aslRGConstruct_strDone;
										
										/* NOTE that we've only concluded that some list is ok here. We need to */
										/* redo this check for individual list use later. */
									}

									if (pStartStr && bExpandNonDictSymbols)
									{
										/*Should we allow leader/trailer */
										int d, bOkWithExpansion = 0;
										if (!pStartStr || pStartStr->pDictionaryEntries == NULL) goto aslRGConstruct_strDone; /* There must be something to expand dictionary paths */

										for (d=0; d<pStartStr->nDictionaryEntries; d++)
										{
											ASL_RG_DICTIONARY_ENTRY * pEntry = &pStartStr->pDictionaryEntries[d];
											
											if (pEntry->nWordLen == 0 ||
												(pRgNode->pcdEdge.nMinRemLetters == 0 &&
												decumaDictionaryGetEndOfWord(pRG->pAllDictionaries[pEntry->dictIdx],pEntry->pReference)) ) /*Can not use more letters from RG when doing trailer expansion */
											{
												/* NULL or complete, i.e. can be leader or trailer. Ok to expand this one. */
												bOkWithExpansion = 1;
												break; 
											}

										}
										if (!bOkWithExpansion) goto aslRGConstruct_strDone; /* Do not allow expansion within a word */
										/* NOTE that we've only concluded that some list is ok here. We need to */
										/* redo this check for individual list use later. */
									}

									if (pStartStr && bAlternativeDictSymbol && !bContractionExpansion && !bRestartDictString)
									{
										int d, bOkWithAlternative = 0;

										if (pStartStr->pDictionaryEntries == NULL) goto aslRGConstruct_strDone; /* There must be something to expand dictionary paths */

										for (d=0; d<pStartStr->nDictionaryEntries; d++)
										{
											DECUMA_UNICODE prevUnicode = 0;

											if (pStartStr->nStringUnicodes) prevUnicode = pStartStr->pStringUnicodes[pStartStr->nStringUnicodes-1];

											if (pStartStr->pDictionaryEntries[d].nWordLen == 0 &&
												(decumaUnicodeToLower(symbol, 0) != symbol ||
												 decumaUnicodeToUpper(symbol, 0) != symbol)) 
											{
												/* Unstarted and letter. Ok to expand this one with alternative symbol. */
												bOkWithAlternative = 1;
												break; 
											}
											else if (pStartStr->pDictionaryEntries[d].bPunishableAlternativeSymbolExpansion ||
												decumaUnicodeToLower(symbol, 0) != symbol &&
												(!decumaUnicodeIsLetter(prevUnicode) || decumaUnicodeToLower(prevUnicode, 0) != prevUnicode))
											{
												/* Consistent upper case dict path. Ok to expand this one with alternative symbol. */
												bOkWithAlternative = 1;
												break; 
											}
										}

										if (!bOkWithAlternative) goto aslRGConstruct_strDone; /* Do not allow alternative symbol for other that first word character */
										/* NOTE that we've only concluded that some list is ok here. We need to */
										/* redo this check for individual list use later. */
									}

									if (pStartStr && pStartStr->nDictionaryEntries > 0 && !bAlternativeDictSymbol && !bRestartDictString && !bExpandNonDictSymbols && bIsLetter)
									{
										int d, bOkWithNonAlternative = 0;

										for (d=0; d<pStartStr->nDictionaryEntries; d++)
										{
											if (!pStartStr->pDictionaryEntries[d].bPunishableAlternativeSymbolExpansion)
											{
												bOkWithNonAlternative = 1;
												break; 
											}
										}

										if (!bOkWithNonAlternative) goto aslRGConstruct_strDone;
									}

									status = createAndQualifyNewString(pRG, nodeIdx, unicodeIdx, edgeIdx, strIdx, pStartStr, symbol, theseNOFUnicodes[0],
										bContractionExpansion, bAlternativeDictSymbol,
										bExpandNonDictSymbols, bRestartDictString,
										bUseHorisontalConnDist, &bCancelEdge);

aslRGConstruct_strDone:
									if (status != decumaNoError) goto aslRGConstruct_error;

									if (bCancelEdge) break;
								}
							}
						}
					}

				}

				/* Allocate own diac need for strings */

				{
					/* NOTE: We must use QCH to access strings here, since they have not been sorted yet! We cannot */
					/* even make the assumtion that the first pRgNode->nStrings elements in pRgNode->pStrings are */
					/* the strings (although unsorted) due to possible fragmentation in the QCH. */

					const DECUMA_QCH_CANDREF* pCandRef = decumaQCHGetNextCandRef(decumaQCHGetStartOfCandRefs(pRG->pStringCandHandler));

					while (pCandRef < decumaQCHGetEndOfCandRefs(pRG->pStringCandHandler))
					{
						ASL_RG_STRING* pRGString = decumaQCHGetCand(pCandRef);

						pCandRef = decumaQCHGetNextCandRef(pCandRef);
					}
				}
			}

			/*---- Finalize before stepping to next node (next iteration) ----- */

			/*This sorts the candidates within the pRgNode->pStrings array */
			if (pRgNode->nStrings > 0)
			{
				ASL_RG_STRING* pNewStrings; /* New list of exact size */

				pNewStrings = aslCalloc(pRgNode->nStrings * sizeof(pNewStrings[0]));

				if (!pNewStrings) goto aslRGConstruct_error;

				decumaQCHGetRanking(pRG->pStringCandHandler, pRG->pScratchPad->stringsRanking);

				/* Copy edges in ranked order to new list */
				for (i = 0; i < pRgNode->nStrings; i++)
				{
					pNewStrings[i] = pRgNode->pStrings[pRG->pScratchPad->stringsRanking[i]];
				}

				rgNodeRemoveStringList(pRG, pRgNode);
				rgNodeAddStringList(pRG, pRgNode, pNewStrings, pRgNode->nStrings);
			}
			else
			{
				rgNodeRemoveStringList(pRG, pRgNode);
			}

			/* Free dictionary data identical to start string and reference start string data instead */
			if (pRG->boostLevel != noBoost)
			{
				for (i = 0; i < pRgNode->nStrings; i++)
				{
					int j;
					ASL_RG_STRING * pStr = &pRgNode->pStrings[i];
					ASL_RG_STRING * pStartStr = pStr->pStartStr;
					int bIdentical = 0;

					if (pStr->pDictionaryEntries == NULL ||
						!pStr->bListOwner ||
						!pStartStr ||
						pStartStr->nDictionaryEntries != pStr->nDictionaryEntries)
					{
						continue;
					}
					
					bIdentical=1;
					for (j=0; j<pStr->nDictionaryEntries; j++)
					{
						if (decumaMemcmp(&pStr->pDictionaryEntries[j],&pStartStr->pDictionaryEntries[j],
							sizeof(pStr->pDictionaryEntries[j]))!=0)
						{
							/*There is a difference. We cannot reuse */
							bIdentical=0;
							break;
						}
					}
					if (bIdentical)
					{
						decumaAssert(pStr->bListOwner);
						freeDictionaryList(pRG, pStr->pDictionaryEntries);
						pStr->pDictionaryEntries = pStartStr->pDictionaryEntries;
						pStr->bListOwner = 0;
					}
				}
			}

			FINALIZE_DEBUG_REPORT(pRG);
		}
	}

	/* Free tmp memory only needed during construction */
	rgRemoveScratchPad(pRG);

#ifdef _DEBUG
	/*Make sure no two candidates have the same unicode in the final result of RG */
	if (pRG->nNodes)
	{
		int i;
		for (i=0; i<pRG->pNodes[pRG->nNodes-1]->nStrings; i++)
		{
			int j;
			for (j=i+1; j<pRG->pNodes[pRG->nNodes-1]->nStrings; j++)
			{
				ASL_RG_STRING * pStr1 = &pRG->pNodes[pRG->nNodes-1]->pStrings[i];
				ASL_RG_STRING * pStr2 = &pRG->pNodes[pRG->nNodes-1]->pStrings[j];

				decumaAssert(pStr1->nStringUnicodes != pStr2->nStringUnicodes || 
					decumaMemcmp((pStr1)->pStringUnicodes, (pStr2)->pStringUnicodes, (pStr1->nStringUnicodes) * sizeof((pStr1)->pStringUnicodes[0]) ) != 0);
			}
		}
	}
#endif


aslRGConstruct_exit:

	if (pRG->pStringCheckSumHandler) aslFree(pRG->pStringCheckSumHandler);

	return decumaNoError;

aslRGConstruct_error:

	if (pRG->pStringCheckSumHandler) aslFree(pRG->pStringCheckSumHandler);

	/* State needs to be ok for next call. Easy solution: reset to initial state. */
	aslRGReset(pRG);

	return decumaAllocationFailed;
}






int aslRGGetNOFStrings(const ASL_RG* pRG)
{
	if (pRG->nNodes <= 0) return 0;

	return pRG->pNodes[pRG->nNodes-1]->nStrings;
}

void aslRGGetString(const ASL_RG* pRG,
					DECUMA_UNICODE * pChars,
					DECUMA_UINT16 nMaxChars,
					DECUMA_UINT16 * pnChars, 
					DECUMA_UINT16 * pnResultingChars, 
					int strIdx,
					DECUMA_INT16 * pSymbolChars,
					DECUMA_INT16 nMaxSymbolCharsLen,
					DECUMA_INT16 * pnSymbolCharsLen,
					DECUMA_INT16 * pnResultingSymbolCharsLen,
					DECUMA_INT16 * pSymbolStrokes,
					DECUMA_INT16 nMaxSymbolStrokesLen,
					DECUMA_INT16 * pnSymbolStrokesLen,
					DECUMA_INT16 * pnResultingSymbolStrokesLen,
					DECUMA_UINT8 * pSymbolArcTimelineDiffMask,
					DECUMA_INT16 nMaxSymbolArcTimelineDiffMaskLen,
					DECUMA_INT16 * pnSymbolArcTimelineDiffMaskLen,
					DECUMA_INT16 * pnResultingSymbolArcTimelineDiffMaskLen,
					DECUMA_COORD * pnAverageBaseLineEstimate,
					DECUMA_COORD * pnAverageHelpLineEstimate,
					DECUMA_UINT8 * pbEndsWithGesture,
					DECUMA_UINT8 * pbEndsWithInstantGesture)
{
	const ASL_RG_NODE * pRgNode;
	const ASL_RG_STRING * pStr;
	int nNode=pRG->nNodes-1;
	DECUMA_INT32 nAverageDistBaseToHelpLineEstimate = 0;
	DECUMA_INT32 nAverageBaseLineEstimate =  0;
	int nEstimates = 0;
	int i, j;

	decumaAssert(pnChars);
	decumaAssert(pnResultingChars);
	decumaAssert(pChars);
	
	*pnChars=0;
	*pnResultingChars=0;
	pChars[0]=0;
	
	decumaAssert(nNode > 0);
	
	pRgNode = pRG->pNodes[nNode];
	decumaAssert(strIdx<pRgNode->nStrings);
	
	pStr = &pRgNode->pStrings[strIdx];
	
	*pnResultingChars = pStr->nStringUnicodes;
	*pnChars = ASL_MIN(*pnResultingChars,nMaxChars-1);

	decumaMemcpy(pChars, pStr->pStringUnicodes, (*pnChars)*sizeof(pChars[0]));

	pChars[*pnChars]=0;

	if (pnSymbolStrokesLen && pnResultingSymbolStrokesLen)
	{
		*pnSymbolStrokesLen = 0;
		*pnResultingSymbolStrokesLen=0;
	}
	else
	{
		/* Cannot use segmentation buffer */
		pSymbolStrokes = NULL;
	}

	if (pnSymbolCharsLen && pnResultingSymbolCharsLen)
	{
		*pnSymbolCharsLen = 0;
		*pnResultingSymbolCharsLen = 0;
	}
	else
	{
		/* Cannot use segmentation buffer */
		pSymbolChars = NULL;
	}

	if (pnSymbolArcTimelineDiffMaskLen && pnResultingSymbolArcTimelineDiffMaskLen)
	{
		*pnSymbolArcTimelineDiffMaskLen = 0;
		*pnResultingSymbolArcTimelineDiffMaskLen = 0;
	}
	else
	{
		/* Cannot use segmentation buffer */
		pSymbolArcTimelineDiffMask = NULL;
	}
	if (pbEndsWithGesture || pbEndsWithInstantGesture)
	{
		const ASL_SG_EDGE* pEdge   = aslSGGetEdge(aslSGGetNode(pRG->pSG, nNode), pStr->lastEdgeIdx);
		int bGest=0,bInstGest=0;
		aslSGEdgeIsGesture(pEdge,&bGest,&bInstGest);
		if (pbEndsWithGesture) *pbEndsWithGesture = bGest;
		if (pbEndsWithInstantGesture) *pbEndsWithInstantGesture = bInstGest;
	}
	
	/* Write the segmentation of pStr to segmentation. The segmentation is */
	/* defined as the node sequence of pSG that contains all the edges of */
	/* the base symbols and the ligatures (including pen lift ligatures) */
	/* between the base base symbols of pStr. The start nodes and end nodes */
	/* of the diacritic edges of pStr are skipped so that segmentation */
	/* instead covers the recalculated long pen lift ligature edges between */
	/* base symbols. */

	while (nNode && pStr)
	{
		const ASL_SG_EDGE* pEdge   = aslSGGetEdge(aslSGGetNode(pRG->pSG, nNode), pStr->lastEdgeIdx);
		const SCR_OUTPUT*  pOutput = aslSGGetOutput(pEdge);
		

		if (pSymbolStrokes)
		{
			if (*pnSymbolStrokesLen < nMaxSymbolStrokesLen)
			{
				if (*pnSymbolStrokesLen) pSymbolStrokes[*pnSymbolStrokesLen-1] -= (DECUMA_INT16)nNode;
				pSymbolStrokes[*pnSymbolStrokesLen] = (DECUMA_INT16)nNode;
				(*pnSymbolStrokesLen)++;
			}
			(*pnResultingSymbolStrokesLen)++;
		}

		if (pSymbolChars)
		{
			if (*pnSymbolCharsLen < nMaxSymbolCharsLen)
			{
				if (*pnSymbolCharsLen) pSymbolChars[*pnSymbolCharsLen-1] -= pStr->nStringUnicodes;
				pSymbolChars[*pnSymbolCharsLen] = pStr->nStringUnicodes;
				(*pnSymbolCharsLen)++;
			}
			(*pnResultingSymbolCharsLen)++;
		}

		if (pSymbolArcTimelineDiffMask)
		{
			if (*pnSymbolArcTimelineDiffMaskLen < nMaxSymbolArcTimelineDiffMaskLen)
			{
				pSymbolArcTimelineDiffMask[*pnSymbolArcTimelineDiffMaskLen] = pOutput->arcTimelineDiffMask;
				(*pnSymbolArcTimelineDiffMaskLen)++;
			}
			(*pnResultingSymbolArcTimelineDiffMaskLen)++;
		}

		if (aslSGGetBaseLineYEstimate(pEdge) != aslSGGetHelpLineYEstimate(pEdge) &&
			aslSGIsEdgeEstimateReliable(pRG->pSG, pEdge))
		{
			nAverageBaseLineEstimate += aslSGGetBaseLineYEstimate(pEdge);
			nAverageDistBaseToHelpLineEstimate += aslSGGetBaseLineYEstimate(pEdge) - aslSGGetHelpLineYEstimate(pEdge);
			nEstimates++;
		}

		/* Get the start node of the edge of pStr ending in nNode (or if there is a preceeding pen lift */
		/* ligature edge, the start node of this edge (its end node was obtained above)). */
		/* Note that this is the same as the end node of the new pStr returned. */
		nNode = aslRGStringGetStartNode(pStr);
		pStr = aslRGStringGetStartString(pStr);
	}

	decumaAssert(nNode == 0);
	if (pSymbolChars)
	{
		int nSymbolCharsSum = 0;
		int nTruncatedSymbolChars = *pnSymbolCharsLen;

		if (pRG->nStringStartLen)
		{
			/* Adjust for pre-existing start string */
			pSymbolChars[(*pnSymbolCharsLen)-1] -= pRG->nStringStartLen;
			if (*pnSymbolCharsLen < nMaxSymbolCharsLen) pSymbolChars[(*pnSymbolCharsLen)++] = pRG->nStringStartLen;
			(*pnResultingSymbolCharsLen)++;
		}

		/* pSymbolChars needs to be reversed */
		for (i = 0, j = *pnSymbolCharsLen-1; i < j; i++, j--)
		{
			DECUMA_INT16 tmp = pSymbolChars[i];
			pSymbolChars[i] = pSymbolChars[j];
			pSymbolChars[j] = tmp;
		}

		/* Null all symbols not represented by characters in input */
		for (i = 0; i < *pnSymbolCharsLen; i++) {
			if (nSymbolCharsSum >= *pnChars) {
				pSymbolChars[i] = 0;
			}
			else {
				if (nSymbolCharsSum + pSymbolChars[i] <= *pnChars) {
					nSymbolCharsSum += pSymbolChars[i];
				}
				else {
					pSymbolChars[i] = *pnChars - nSymbolCharsSum;
					nSymbolCharsSum = *pnChars;
				}
			}
			
		}
	}

	if (pSymbolStrokes)
	{
		if (pRG->nStringStartLen)
		{
			/* Adjust for pre-existing start string */
			if (*pnSymbolStrokesLen < nMaxSymbolStrokesLen) pSymbolStrokes[(*pnSymbolStrokesLen)++] = 0;
			(*pnResultingSymbolStrokesLen)++;
		}

		/* pSymbolStrokes needs to be reversed */
		for (i = 0, j = *pnSymbolStrokesLen-1; i < j; i++, j--)
		{
			DECUMA_INT16 tmp = pSymbolStrokes[i];
			pSymbolStrokes[i] = pSymbolStrokes[j];
			pSymbolStrokes[j] = tmp;
		}
	}

	/* Apparently this is not true, since noise may give 0 elements in pSymbolChars
	 decumaAssert(*pnChars >= *pnSymbolCharsLen); */
	decumaAssert(*pnSymbolCharsLen == *pnSymbolStrokesLen);

	if (pSymbolArcTimelineDiffMask)
	{
		if (pRG->nStringStartLen)
		{
			/* Adjust for pre-existing start string */
			if (*pnSymbolArcTimelineDiffMaskLen < nMaxSymbolArcTimelineDiffMaskLen) pSymbolArcTimelineDiffMask[(*pnSymbolArcTimelineDiffMaskLen)++] = 255;
			(*pnResultingSymbolArcTimelineDiffMaskLen)++;
		}

		/* pSymbolArcTimelineDiffMask needs to be reversed */
		for (i = 0, j = *pnSymbolArcTimelineDiffMaskLen-1; i < j; i++, j--)
		{
			DECUMA_INT16 tmp = pSymbolArcTimelineDiffMask[i];
			pSymbolArcTimelineDiffMask[i] = pSymbolArcTimelineDiffMask[j];
			pSymbolArcTimelineDiffMask[j] = tmp;
		}
	}

	if (nEstimates)
	{
		nAverageBaseLineEstimate /= nEstimates;
		nAverageDistBaseToHelpLineEstimate /= nEstimates;
	}
	else
	{
		/* No new estimate available. Set estimate to the current one. */
		nAverageBaseLineEstimate = aslArcSessionGetBaseline(pRG->pArcSession);
		nAverageDistBaseToHelpLineEstimate = aslArcSessionGetDistBaseToHelpline(pRG->pArcSession);
	}

	if (pnAverageBaseLineEstimate) *pnAverageBaseLineEstimate = nAverageBaseLineEstimate;
	if (pnAverageHelpLineEstimate) *pnAverageHelpLineEstimate = nAverageBaseLineEstimate - nAverageDistBaseToHelpLineEstimate;
}

int aslRGGetStringDist(const ASL_RG* pRG, int strIdx)
{
	if (pRG->nNodes <= 0) return 0;

	if (pRG->pNodes[pRG->nNodes-1]->nStrings <= strIdx) return 0;

	return pRG->pNodes[pRG->nNodes-1]->pStrings[strIdx].sortingDist;
}

int aslRGGetStringShapeDist(const ASL_RG* pRG, int strIdx)
{
	if (pRG->nNodes <= 0) return 0;

	if (pRG->pNodes[pRG->nNodes-1]->nStrings <= strIdx) return 0;

	return pRG->pNodes[pRG->nNodes-1]->pStrings[strIdx].dist;
}

DECUMA_STRING_TYPE aslRGGetStringType(const ASL_RG* pRG, int strIdx, 
									  DECUMA_INT16 * pStartIdx, DECUMA_INT16 * pLen)
{
	decumaAssert(pRG->nNodes>0);
	decumaAssert(pRG->pNodes[pRG->nNodes-1]->nStrings > strIdx);
	decumaAssert(pRG && pRG->pStringCandHandler==(DECUMA_QCH*) &pRG[1]); /*Initialized */

	/* RG can effectively answer this if it was constructed using dictionary. */
	/* Otherwise it cannot provide an answer at all and the caller should not inquiry RG on this. */
	decumaAssert(pRG->boostLevel != noBoost); /* RG cannot answer this. Caller should not inquiry RG on this. */

	return rgStringGetType(pRG, &pRG->pNodes[pRG->nNodes-1]->pStrings[strIdx], pStartIdx, pLen);
}

int aslRGGetCharStrokeMtxSize(DECUMA_UINT16 nMaxChars)
{
#define MAX_SYMBOL_STROKES 8 /*TODO fix */
	return MAX_SYMBOL_STROKES*nMaxChars;
}

DECUMA_UINT16 * aslRGCharStrokeDataGetStrokeIdxPtr(const void * pCharStrokesMtx, DECUMA_UINT16 nCharIdx)
{
	return (DECUMA_UINT16*) pCharStrokesMtx + MAX_SYMBOL_STROKES * nCharIdx;
}

void aslRGSetCharStrokesData(const ASL_RG* pRG, void * pCharStrokesMtx, DECUMA_UINT16 * pnStrokesArr,
									  int strIdx, DECUMA_UINT16 nMaxChars, DECUMA_UINT16 nChars)
{
	const ASL_RG_NODE * pRgNode;
	const ASL_RG_STRING * pStr;
	const ASL_RG_STRING * pResultStr;

	const ASL_RG_STRING * pPrevStr = NULL;
	int nPrevStrNode = 0;

	int nNode=pRG->nNodes-1;

	decumaAssert(pCharStrokesMtx);
	decumaAssert(pnStrokesArr);	
	decumaAssert(nNode > 0);
	
	pRgNode = pRG->pNodes[nNode];
	decumaAssert(strIdx<pRgNode->nStrings);
	
	pResultStr = &pRgNode->pStrings[strIdx];
	pStr = pResultStr;

	/*Walk through the strings backwards to find the strokes for each character... */
	while (pStr)
	{		
		if (pPrevStr)
		{
			if (pStr->nStringUnicodes < nChars)
			{
				/*One or more of the characters in pPrevStr have been written to the result */
				/*Therefore we need to add stroke data for those characters (can be several chars since one */
				/*edge can contain multiple unicodes) */
				int chIdx;
				EDGE_DESCRIPTOR symbolEdgeD;
				int symbolStartNode;
				DECUMA_UINT16 arcIdxs[MAX_SYMBOL_STROKES];
				int nArcIdxs=0;
				int arcIndex;

				initEdgeDescriptorMacro((ASL_RG *)pRG, nPrevStrNode,pPrevStr->lastEdgeIdx,pPrevStr,&symbolEdgeD);
				
				symbolStartNode = aslSGGetStartNode(symbolEdgeD.pEdge);

				for (arcIndex = symbolStartNode+1; arcIndex<=nPrevStrNode; arcIndex++)
				{
					if (nArcIdxs == 0 || arcIndex != arcIdxs[nArcIdxs-1])
					{
						arcIdxs[nArcIdxs]=arcIndex;
						nArcIdxs++;
					}
				}

				for (chIdx = pStr->nStringUnicodes-1; chIdx < nChars && chIdx<pPrevStr->nStringUnicodes; chIdx++)
				{
					DECUMA_UINT16 * pStrokeIdxs = (DECUMA_UINT16*) pCharStrokesMtx + chIdx*MAX_SYMBOL_STROKES;
					
					pnStrokesArr[chIdx] = (DECUMA_UINT16) nArcIdxs;
					decumaMemcpy(pStrokeIdxs,arcIdxs, nArcIdxs*sizeof(DECUMA_UINT16));
				}
			}
		}

		pPrevStr = pStr;
		nPrevStrNode = nNode;

		nNode = aslRGStringGetStartNode(pStr);
		pStr = aslRGStringGetStartString(pStr);

		decumaAssert(nNode == 0 || pStr);
	}
}


#ifdef _DEBUG
int aslRGStringGetStartNode(const ASL_RG_STRING * pRgString)
{
	decumaAssert(pRgString);
	decumaAssert(pRgString->nStartNode >= 0);

	return aslRGStringGetStartNodeMacro(pRgString);
}

const ASL_RG_STRING * aslRGStringGetStartString(const ASL_RG_STRING * pRgString)
{
	decumaAssert(pRgString);
	decumaAssert(pRgString->nStartNode >= 0);

	return aslRGStringGetStartStringMacro(pRgString);
}
#endif

void aslRGReset(ASL_RG *pRG)
{
	aslSGSetStringStart(pRG->pSG, NULL, 0);
	/* Release and reinitialize the recognition graph */
	rgRelease(pRG);
	pRG->pStringCandHandler = NULL; /* Uninitialize */
	aslRGInit(pRG,
		pRG->pMemFunctions,
		pRG->pCharacterSet,
		pRG->pCatTable,
		pRG->pSG,
		pRG->pArcSession,
		pRG->pAllDictionaries,
		pRG->nAllDictionaries,
		pRG->boostLevel,
		pRG->strCompleteness,
		pRG->pStringStart,
		pRG->writingDirection,
		pRG->recognitionMode,
		pRG->pDictionaryFilterStr,
		pRG->nDictionaryFilterStrLen,
		pRG->forcedRecognition,
		pRG->pForceResult,
		pRG->nForceResultLen);
}


#ifdef EXTRA_VERIFICATION_INFO

int aslRGGetNodeCount(const ASL_RG* pRG)
{
	decumaAssert(pRG && pRG->pStringCandHandler==(DECUMA_QCH*) &pRG[1]); /*Initialized */

	return pRG->nNodes;
}

const ASL_RG_NODE * aslRGGetNode(const ASL_RG* pRG, int nNode)
{
	decumaAssert(pRG && pRG->pStringCandHandler==(DECUMA_QCH*) &pRG[1]); /*Initialized */
	decumaAssert(nNode<pRG->nNodes);
	decumaAssert(nNode>=0);

	return pRG->pNodes[nNode];
}


int aslRGNodeGetStringCount(const ASL_RG_NODE * pRGNode)
{
	decumaAssert(pRGNode);

	return pRGNode->nStrings;
}

const ASL_RG_STRING * aslRGNodeGetString(const ASL_RG_NODE * pRGNode, int nRgStringIdx)
{
	decumaAssert(pRGNode);
	decumaAssert(nRgStringIdx<pRGNode->nStrings);
	decumaAssert(nRgStringIdx>=0);

	return &pRGNode->pStrings[nRgStringIdx];
}

const ASL_SG_EDGE * aslRGStringGetLastEdge(const ASL_RG_STRING * pRgString, const ASL_RG * pRG, int nodeIdx)
{
	const ASL_SG_NODE * pSGNode;

	decumaAssert(pRG);
	decumaAssert(pRgString);

	pSGNode = aslSGGetNode(pRG->pSG, nodeIdx);
	return aslSGGetEdge(pSGNode, pRgString->lastEdgeIdx);
}

							 

int aslRGStringGetPrevious(const ASL_RG_STRING * pRgString)
{
	decumaAssert(pRgString);

	return pRgString->prevStringIdx;
}

/*Returns the index of the edge to the last node in the string path */
int aslRGStringGetLastEdgeIdx(const ASL_RG_STRING * pRgString)
{
	decumaAssert(pRgString);

	return pRgString->lastEdgeIdx;
}

int aslRGStringGetDist(const ASL_RG_STRING * pRgString)
{
	decumaAssert(pRgString);
	return pRgString->dist;
}

int aslRGStringGetLastDist(const ASL_RG_STRING * pRgString)
{
	decumaAssert(pRgString);
	return pRgString->lastEdgeDist;
}

int aslRGStringGetConnectionDist(const ASL_RG_STRING * pRgString)
{
	decumaAssert(pRgString);
	return pRgString->connectionDist;
}


int aslRGStringGetSortingDist(const ASL_RG_STRING * pRgString)
{
	decumaAssert(pRgString);
	return pRgString->sortingDist;
}


/*
	Returns the unicode representing the last edge of the string

		nMaxUnicodes    - The number of unicodes that can be written in pUnicodes including terminating zero

	Return value is the number of written unicodes excluding terminating zero
*/
int aslRGStringGetLastSymbolUnicodes(const ASL_RG * pRG, const ASL_RG_STRING * pRgString, 
												 int nodeIdx, DECUMA_UNICODE * pUnicodes, int nMaxUnicodes)
{
	ASL_RG_STRING * pStartString;
	int nUnicodes,nUnicodesToWrite;

	decumaAssert(pRgString);
	decumaAssert(pUnicodes);
	if (pRgString->nStringUnicodes == 0) return 0;

	decumaAssert(pRgString->nStartNode >= 0);
	pStartString = pRgString->pStartStr;
	if (pStartString)
	{
		nUnicodes = pRgString->nStringUnicodes - pStartString->nStringUnicodes;
	} 
	else
	{
		nUnicodes = pRgString->nStringUnicodes;
	}

	nUnicodesToWrite = ASL_MIN(nMaxUnicodes-1, nUnicodes);
	decumaMemcpy(pUnicodes, &pRgString->pStringUnicodes[pRgString->nStringUnicodes-nUnicodes],
		nUnicodesToWrite * sizeof(pUnicodes[0]));
	pUnicodes[nUnicodesToWrite]=0;

	return nUnicodes;
}

/* 
	Returns the distance of the rg string divided by the number of segments. This should be in the same
	format as calculated in decuma_hwr.c
*/
DECUMA_UINT32	aslRGStringGetMeanDistance(const ASL_RG * pRG, const ASL_RG_STRING * pRgString, int nNode)
{
	int divisor;
	decumaAssert(pRG && pRG->pStringCandHandler==(DECUMA_QCH*) &pRG[1]); /*Initialized */
	decumaAssert(pRgString);
	decumaAssert(nNode < pRG->nNodes && nNode>=0);

	/*This calculation should reflect how the mean calculation is done in decuma_hwr.c */
	
	divisor = nNode; /*Number of semgents = number of nodes -1 = nodeIdx */

	return (DECUMA_UINT32) (divisor ? (pRgString->dist / (DECUMA_UINT32)divisor) : 0);
}

#endif /*EXTRA_VERIFICATION_INFO */




/*/////////////////////////////////////////////////////////////////// */
/* Private function definitions */
/* */


static void rgRelease(ASL_RG* pRG)
{
	const DECUMA_MEM_FUNCTIONS* pMemFunctions = pRG->pMemFunctions;
	int i;
	decumaAssert(pRG);

	decumaAssert(pRG->pStringCheckSumHandler == 0);

	while (pRG->nAddedNodes > 0) rgRemoveNode(pRG);

	if (pRG->pNodes) aslFree(pRG->pNodes);

	while (pRG->nScratchPads > 0) rgRemoveScratchPad(pRG);
	
	if (pRG->pStringMemPoolHandler)
	{
		MemoryPoolHandler_Destroy(pRG->pStringMemPoolHandler, pRG->pMemFunctions);
	}
	for (i=0; i<pRG->nStringMemPools; i++)
	{
		aslFree(pRG->ppStringMemPools[i]);
	}
	if (pRG->ppStringMemPools)
	{
		aslFree(pRG->ppStringMemPools);
	}
	
	if (pRG->pDictMemPoolHandler)
	{
		MemoryPoolHandler_Destroy(pRG->pDictMemPoolHandler, pRG->pMemFunctions);
	}
	for (i=0; i<pRG->nDictMemPools; i++)
	{
		aslFree(pRG->ppDictMemPools[i]);
	}
	if (pRG->ppDictMemPools)
	{
		aslFree(pRG->ppDictMemPools);
	}
	if (pRG->pDictionaryData)
	{
		for (i=0; i<pRG->nAllDictionaries; i++)
		{
			if (pRG->pDictionaryData[i].pFreqClassBoost)
			{
				aslFree(pRG->pDictionaryData[i].pFreqClassBoost);
			}
		}
		aslFree(pRG->pDictionaryData);
	}
	if (pRG->pInitialDictEntries)
	{
		aslFree(pRG->pInitialDictEntries);
	}
}

static DECUMA_STATUS createAndQualifyNewString(ASL_RG *pRG,
											   int nNodeIdx,
											   int nUnicodeIdx,
											   int nEdgeIdx,
											   int nStartStrIdx,
											   ASL_RG_STRING* pStartStr,
											   DECUMA_UNICODE symbol,
											   int nUnicodesInEdge,
											   int bContractionExpansion,
											   int bAlternativeDictSymbol,
											   int bExpandNonDictSymbols,
											   int bRestartDictString,
											   int bUseHorisontalConnDist,
											   int* pbCancelEdge)
{
	int nNodesInSG = aslSGGetNodeCount(pRG->pSG);
	DECUMA_STATUS status = decumaNoError;
	ASL_RG_NODE* pRgNode = pRG->pNodes[nNodeIdx];
	const ASL_SG_NODE* pSgNode = aslSGGetNode(pRG->pSG, nNodeIdx);
	const ASL_SG_EDGE* pEdge = aslSGGetEdge(pSgNode, nEdgeIdx);
	int nStartNodeIdx = aslSGGetStartNode(pEdge);
	long thisEdgeDist;
	int nCandidates = decumaQCHGetCount(pRG->pStringCandHandler);

	const ASL_RG_NODE * pEdgeStartRgNode = pRG->pNodes[nStartNodeIdx];			

	ASL_RG_STRING newAslString;
	int nConnectionDist=0;
	int bAdded=1; /* Means unexisting */
	/* TODO: 
	 * Here, we should be able to utilize ZiGetNodeInfo2 better. 
	 * We can directly fill a buffer with all valid unicodes from
	 * the current dictionary node(s), and use an array lookup
	 * instead of calling checkWithDictionary()
	 * ZiGetNodeInfo2 accepts a min- and max-value, and aborts
	 * if they are out of range. 
	 * Especially useful if the results from ZiGetNodeInfo2 are
	 * guaranteed to be ordered.
	 */
	DECUMA_UINT32 linguisticDistFactorStartStr128 = 128;
	int nQualificationDist128;

	DECUMA_UNICODE initialStartStringWithExtraRoom[2];
	DECUMA_UINT8 thisUnicodeCheckSum;
	ASL_RG_STRING* pReplaceeCandidate;
	const DECUMA_QCH_CANDREF* replaceeCandidateRef;
	DECUMA_HH_ELEMREF replaceeElemRef;
	int nReplaceeDist;
	DECUMA_UINT32 linguisticDistFactor128;
	DECUMA_UINT32 nStartStrDist = pStartStr ? pStartStr->dist : 0;
	DECUMA_UINT16 nStartStrRankBoost1024 = pStartStr ? pStartStr->nRankBoost1024 : pRG->nMaxRankBoost1024;
	DECUMA_UNICODE* pStartStrUnicodes = pStartStr ? pStartStr->pStringUnicodes : NULL;
	DECUMA_INT16 nStartStrUnicodes = pStartStr ? pStartStr->nStringUnicodes : 0;

	*pbCancelEdge = 0;

	thisEdgeDist = nUnicodeIdx == 0 ?  aslSGGetDistance(pEdge) : 0;

	if (pEdgeStartRgNode->pcdEdge.nMaxRemUnicodes == MIN_DECUMA_INT16)
	{
		decumaAssert(pEdgeStartRgNode->pcdEdge.nMinRemUnicodes == MAX_DECUMA_INT16);
		return decumaNoError; /*It will not be involved in any valid path anyway */
	}

	if (nStartStrRankBoost1024 == pRG->nMaxRankBoost1024)
	{
		linguisticDistFactorStartStr128 = pRgNode->nMaxLinguisticDistFactor128;
	}
	else if (nStartStrRankBoost1024 > 0)
	{
		linguisticDistFactorStartStr128 = 
			getLingDistFactor128(nStartStrRankBoost1024,
				pRgNode->nNodeProgressionSqr1024);
	}

	nQualificationDist128 = pRgNode->nQualificationDist128;

#ifndef NO_EARLY_CONCLUSION_FOR_SPEED
	if (linguisticDistFactorStartStr128 * (nStartStrDist + thisEdgeDist) >=
		nQualificationDist128)
	{
		/* No point in continuing with worse start strings */
		/* NOTE This is actually not correct since, start string are sorted by sortingDist not */
		/* dist, but after applying linguisticDistFactorStartStr128 the break rule should make */
		/* good sense anyway (almost no hitrate difference at all in tests). */
		*pbCancelEdge = 1;
		return decumaNoError;
	}
#endif

	if (pRG->pForceResult && symbol)
	{
		int bFoundMatch = 0;

		{
			int fri;

			/* TODO use hash table look up of (fri + pChars[fri] comb). Whole string */
			/* does not need to be forced just continuation */
			for (fri = 0; fri < pRG->nForceResultLen; fri++)
			{
				if (nStartStrUnicodes + 1 <= pRG->pForceResult[fri].nChars &&
					decumaMemcmp(pStartStrUnicodes, pRG->pForceResult[fri].pChars,
					nStartStrUnicodes * sizeof(symbol)) == 0 &&
					decumaMemcmp(&symbol,&pRG->pForceResult[fri].pChars[nStartStrUnicodes],
					1 * sizeof(symbol)) == 0)
				{
					bFoundMatch = 1;
					break;
				}
			}
		}

		if (!bFoundMatch)
			return decumaNoError; /*We might need to free some allocated data  */
	}

	PROF(InRGStartStrLoop); /*JM: 2007-11-14: Here we are: ~45000 times per word  */
	
	if (!pRgNode->bResetPreCalcArrays)
	{
		decumaMemset(pRG->pScratchPad->preCalculatedConnDists, 0, sizeof(pRG->pScratchPad->preCalculatedConnDists));

		pRgNode->bResetPreCalcArrays = 1;
	}

	if (nStartNodeIdx > 0 && nUnicodeIdx == 0) nConnectionDist += getConnectionDist(pRG, nNodeIdx, nEdgeIdx, nStartStrIdx, bUseHorisontalConnDist);

#ifndef NO_EARLY_CONCLUSION_FOR_SPEED
	if (linguisticDistFactorStartStr128 * (nStartStrDist + thisEdgeDist + nConnectionDist) >= nQualificationDist128) goto createAndQualifyNewString_exit;
#endif


	PROF(TStartStrAndThisEdgeDistTooHigh);
	if (linguisticDistFactorStartStr128 * (nStartStrDist + thisEdgeDist + nConnectionDist)
			>= nQualificationDist128)
	{
		/*Note: At this stage we don't yet know the connection distance or any distance at */
		/*all if we are looking at a diacritic edge. We do not either know which of the different */
		/*symbols of the edge that we are regarding, therefore we cannot select a replacee */
		
		PROF(StartStrAndThisEdgeDistTooHigh); /*JM 2007-11-14: Here we end up: 30% withDict (3500/12000), 40% noDict (5500/14000) */
#ifndef NO_EARLY_CONCLUSION_FOR_SPEED
		goto createAndQualifyNewString_exit;
#endif
	}
	/*--- Begin: Initialization --- */

	INIT_STRING(&newAslString);
	bAdded = 0;
	newAslString.lastEdgeIdx = nEdgeIdx;

	newAslString.prevStringIdx = nUnicodeIdx > 0 ? pStartStr->prevStringIdx : (DECUMA_INT8) nStartStrIdx;						

	newAslString.nStartNode = nStartNodeIdx;
	newAslString.nDictionaryEntries = 0;
	newAslString.pDictionaryEntries = NULL;
	newAslString.bListOwner = 0;

	newAslString.nPrependedSymbols = nStartNodeIdx == 0 && nUnicodeIdx == 0 ? (!bRestartDictString ? pRG->nStringStartLen : 0) : pStartStr->nPrependedSymbols;

	/*JM TEMP */
	/*newAslString.bStrUsingAltSymb = (bAlternativeDictSymbol || pStartStr->bStrUsingAltSymb); */
	/*newAslString.bStrRestarted = (bRestartDictString || pStartStr->bStrRestarted); */


	newAslString.pStartStr = nUnicodeIdx > 0 ? pStartStr->pStartStr : pStartStr;


	/*--- End: Initialization --- */


#ifdef _DEBUG_EXTRA
	decumaAssert(restCannotContainUnicodesMacro(pEdgeStartRgNode, pRgNode, thisEdgeClass)==
		restCannotContainSymbols(pEdgeStartRgNode, pRgNode, thisEdgeClass));
#endif /*_DEBUG_EXTRA */

	/*We don't want empty strings (or only the string start) as results  */
	PROF(TCreatingEmptyString); /*JM 2007-11-14: Here we are about 7000-8000 times/word */
	if (nUnicodesInEdge + nStartStrUnicodes - pRG->nStringStartLen == 0 &&
		restCannotContainUnicodesMacro(pEdgeStartRgNode, pRgNode, thisEdgeClass))
	{
		DEBUG_REPORT(rgOnlyEmptyRecognition,pRG,newAslString.bStringIsPartOfRefBestPath,nNodeIdx,nEdgeIdx,nStartStrIdx,penliftEdgeIdx,0);
		PROF(CreatingEmptyString); /*JM 2007-11-14:  0% */
		goto createAndQualifyNewString_exit; /*This path will only create empty words */
	}

	/*Here we can have many symbols for each edge!! (with different diacritic needs) e.g. a,�� */
	/*Loop through all of these different symbols and check dictionary etc. */
	
#ifdef DEBUG_OUTPUT
	fprintf(g_debugFile,"nNodeIdx==%d && nEdgeIdx ==%d && nStartStrIdx==%d\n",
		nNodeIdx,nEdgeIdx,nStartStrIdx);
	fflush(g_debugFile);
#endif /*DEBUG_OUTPUT */
	bAdded=0;

	if (pRG->bUseForcedRecognition)
	{
		int nStartStrLenOfForced = pStartStr ? pStartStr->nLenOfForced : 0;

		newAslString.nLenOfForced = nStartStrLenOfForced;
		if (symbol)
		{
			if (decumaMemcmp(&symbol,&pRG->forcedRecognition[nStartStrLenOfForced],
					1*sizeof(symbol))!=0)
			{
				/*Here is the forcing taking place!! */
				goto createAndQualifyNewString_exit; /*We might need to free some allocated data  */
			}
		
			newAslString.nLenOfForced = (DECUMA_UINT8) (nStartStrLenOfForced+1);
		}

		if ( pRG->strlenForced-newAslString.nLenOfForced> pRgNode->pcdEdge.nMaxRemUnicodes || 
			pRG->strlenForced-newAslString.nLenOfForced< pRgNode->pcdEdge.nMinRemUnicodes)
		{
			/*Here is the forcing taking place!! We need to get the full word */
			/*No path will fill the full length of the word. So don't add this candidate */
			goto createAndQualifyNewString_exit; /*We might need to free some allocated data  */
		}
	}

	newAslString.dist = nStartStrDist + thisEdgeDist + nConnectionDist; /*Add connection distance already here if already calculated. Enables earlier decisions */

#ifdef EXTRA_VERIFICATION_INFO
	newAslString.lastEdgeDist = thisEdgeDist;
	newAslString.connectionDist = nConnectionDist;
#endif

	PROF(InRGSymbolsLoop); /*JM 2007-11-14: Here we end up:15000-16000 times (higher for noDict) */
	
	linguisticDistFactor128 = linguisticDistFactorStartStr128;

	decumaAssert(nCandidates < pRgNode->nMaxStrings ||
		linguisticDistFactor128 * newAslString.dist < nQualificationDist128);

	newAslString.pStringUnicodes = pStartStrUnicodes;
	newAslString.nStringUnicodes = nStartStrUnicodes;

	{
		DECUMA_UINT8 nU;
		/*This might look a little strange. But we have prepared the buffer */
		/*pointed to by pStartStrUnicodes to have room for */
		/*another symbol in the end. So we can re-use the unicode buffer of the */
		/*start string and just extend it with this edge's unicode(s) temporarily */
		/*However, if this string ends up in the candidate handler,  */
		/*we need to alloc new own memory for the unicodes and some extra room so that this */
		/*procedure can be repeated for future nodes...										 */

		if (newAslString.nStringUnicodes == 0)
		{
			/*Special case. If we do not have any alloced pStringUnicodes for the start string */
			/*We temprarily use this "initialStartStringWithExtraRoom" buffer */
			newAslString.pStringUnicodes = initialStartStringWithExtraRoom;
		}
		
		for (nU=0; nU<1 && symbol; nU++) /* TODO no need to loop */
		{
			decumaAssert(newAslString.nStringUnicodes < MAX_DECUMA_UINT8); /*Check for overflow */
			newAslString.pStringUnicodes[newAslString.nStringUnicodes++] = symbol; 
		}
		newAslString.nStringUnicodesAllocFlag = UNICODES_NEED_ALLOCATION; /*This is an indication that we need to alloc if it is added to candidate handler */
	}									
	decumaAssert(newAslString.nStringUnicodes<=maxUnicodesInRGMacro(pRG));
	
#if defined(DECUMA_ASSERT_ENABLE) || defined(DECUMA_ASSERT_OVERRIDE)
	/* This assert exposes a possibly fatal problem - the pre-calculated checksum does not match the content of pStartStrUnicodes */
	if (pStartStr)
	{
		DECUMA_UINT8 correctCheckSum;
		getUnicodeCheckSumMacro(correctCheckSum, 0, pStartStrUnicodes, nStartStrUnicodes);
		decumaAssert(pStartStr->nUnicodeCheckSum == correctCheckSum);
	}
#endif

	getUnicodeCheckSumMacro(thisUnicodeCheckSum, pStartStr ? pStartStr->nUnicodeCheckSum : 0, &symbol, symbol ? 1 : 0);
	
#if defined(DECUMA_ASSERT_ENABLE) || defined(DECUMA_ASSERT_OVERRIDE)
	{
		DECUMA_UINT8 correctCheckSum;
		getUnicodeCheckSumMacro(correctCheckSum, 0, newAslString.pStringUnicodes, newAslString.nStringUnicodes);
		decumaAssert(thisUnicodeCheckSum == correctCheckSum);
	}
#endif

	/* ------------------------ */
	/* Check resulting strings validity in the dictionary */
	/* ----------------------- */
	if (pRG->boostLevel == forceDictWords || pRG->boostLevel == boostDictWords)
	{
		int rgMinRemUnicodes = pRgNode->pcdEdge.nMinRemUnicodes;

		decumaAssert(status == decumaNoError);

		/* NOTE: rgMinRemUnicodes does not help as intended now because it is very often 0 in */
		/* early nodes for quite long words. In order for RG to be more effective (here and */
		/* elsewhere) the rgMinRemUnicodes must be based on high probability instead of 100% */
		/* probability (similar to the new non-lossless speed optimizations). /JA */

		/* TODO When restarting dictionary string. The ranking of the string that */
		/* we restart from should be reflected. Now only the restarted string ranking */
		/* will determine boost factor. */
		status = checkWithDictionary(pRG, &symbol, symbol ? 1 : 0,
			pEdge, pStartStr, pRgNode, 
			&newAslString.pDictionaryEntries,
			&newAslString.nDictionaryEntries,
			&newAslString.bListOwner,
			0,
			bContractionExpansion,
			bAlternativeDictSymbol,
			bExpandNonDictSymbols,
			bRestartDictString);
		/* No need to update bCheckedWithDictionary (and bEziNodeOwner) here */

		decumaAssert(!bContractionExpansion || !newAslString.pDictionaryEntries || newAslString.pDictionaryEntries[0].nLeftBoost > 0);
		/*decumaAssert(newAslString.nDictionaryEntries==0 || */
		/*	((newAslString.pDictionaryEntries[0].nLeftBoost > 0)== */
		/*	newAslString.bStrRestarted)); */

		if (status != decumaNoError)
		{
			status = decumaAllocationFailed;
			goto createAndQualifyNewString_exit;
		}

		if (pRG->boostLevel == forceDictWords && newAslString.nStringUnicodes - pRG->nStringStartLen + rgMinRemUnicodes > 1)
		{
			/* NOTE that we never force single chars to match dictionary. Single char input should */
			/* always be a possible dictionary escape. */
			PROF(TNoDictionaryWord);
			if (!newAslString.pDictionaryEntries)
			{
				/*No dictionary matches. This is an invalid path. Try next string */
				PROF(NoDictionaryWord); /*JM 2007-11-02: Here we end up: ~70% (of 14500) */
				goto createAndQualifyNewString_exit;
			}
		}

		/* NOTE that we do not except single chars from boosting (just forcing) */
			newAslString.nRankBoost1024 = getRankBoost1024(pRG,
				newAslString.pDictionaryEntries, newAslString.nDictionaryEntries, 
				newAslString.pStringUnicodes, newAslString.nStringUnicodes, pRgNode,
				pRG->nStringStartLen - newAslString.nPrependedSymbols,
				&newAslString.bestDictEntryIdx);
	}
	else
	{
		decumaAssert(!pStartStr || pStartStr->pDictionaryEntries == 0);
		decumaAssert(!pStartStr || pStartStr->nDictionaryEntries == 0);
		decumaAssert(!pStartStr || pStartStr->bListOwner == 0);
		decumaAssert(newAslString.pDictionaryEntries == 0);
		decumaAssert(newAslString.nDictionaryEntries == 0);
		decumaAssert(newAslString.bListOwner == 0);
		newAslString.nRankBoost1024 = 0;
	}

	if (newAslString.nRankBoost1024 != nStartStrRankBoost1024)
	{
		if (newAslString.nRankBoost1024 == pRG->nMaxRankBoost1024)
		{
			linguisticDistFactor128 = pRgNode->nMaxLinguisticDistFactor128;
		}
		else
		{
			linguisticDistFactor128 = getLingDistFactor128(newAslString.nRankBoost1024, pRgNode->nNodeProgressionSqr1024);
		}

		if (linguisticDistFactor128 * newAslString.dist >= (DECUMA_UINT32)nQualificationDist128) 
		{
			goto createAndQualifyNewString_exit; /*We might need to free some allocated data */
		}
	}

	/* Check first if it is possible at all to find a replacee, since */
	/* the function call is expensive (e.g a lot of parameter to copy). */
	/* A found reference will be reused by the function */
	replaceeElemRef = decumaHHFindKey(pRG->pStringCheckSumHandler, thisUnicodeCheckSum);

	if (replaceeElemRef)
	{
		searchCandidateReplacee(pRG, pRG->pStringCandHandler, &newAslString, thisUnicodeCheckSum,
			&pReplaceeCandidate, &replaceeElemRef, &nReplaceeDist, nNodeIdx == nNodesInSG-1);
	}
	else
	{
		if (decumaQCHIsFull(pRG->pStringCandHandler) ||
			( nCandidates > 0 && 128 * decumaQCHGetKey(decumaQCHGetPrevCandRef(decumaQCHGetEndOfCandRefs(pRG->pStringCandHandler))) > nQualificationDist128) )
		{
			/* Candidate handler full or worst candidate isn't qualifying anymore */
			pReplaceeCandidate = decumaQCHGetCand(decumaQCHGetPrevCandRef(decumaQCHGetEndOfCandRefs(pRG->pStringCandHandler)));
			nReplaceeDist = pReplaceeCandidate->sortingDist;
			replaceeElemRef = decumaHHFindData(pRG->pStringCheckSumHandler, pReplaceeCandidate->nUnicodeCheckSum, pReplaceeCandidate);
		}	
		else
		{
			replaceeElemRef = NULL;
			pReplaceeCandidate = NULL;
			nReplaceeDist = DECUMA_MAX_DISTANCE;
		}
	}

	if (128 * nReplaceeDist > nQualificationDist128) nReplaceeDist = nQualificationDist128 / 128;

	PROF_IF(nReplaceeDist==DECUMA_MAX_DISTANCE,NonFilledRG); /*JM 2007-11-14: This is true, candidateBuf not filled, ~1000 times per word (~25% withDict/~10% noDict) */

	PROF_IF_ELSE( nReplaceeDist >= nCandidateHandlerWorstDist,
		TDistWorseThanWorst,
		TDistWorseThanReplacee);									

	if (linguisticDistFactor128 * newAslString.dist >= (DECUMA_UINT32)nReplaceeDist * 128) 
	{
		/*JM 2007-11-14: Here we end up: ~25% withDict, ~30% noDict */
		PROF_IF_ELSE( nReplaceeDist >= nCandidateHandlerWorstDist,
			DistWorseThanWorst,     /*JM 2007-11-14: Here we end up: ~30% (600/3500)  (3000/10000) */
			DistWorseThanReplacee); /*JM 2007-11-14: Here we end up: ~30% (230/750) withDict (300/900) noDict  */
			                          
		goto createAndQualifyNewString_exit; /*We might need to free some allocated data */
	}

	newAslString.sortingDist = linguisticDistFactor128 * newAslString.dist / 128;
	/*If the string not ends up in the candidate handler it is lost in competition */
	DEBUG_REPORT(rgStringLostInCompetition,pRG,newAslString.bStringIsPartOfRefBestPath,nNodeIdx,nEdgeIdx,nStartStrIdx,penliftEdgeIdx,newAslString.dist);

	newAslString.nUnicodeCheckSum=thisUnicodeCheckSum;

	PROF(TryingToAddCandidate); /*2007-11-02: Here we end up: 1800-2300 times */

	preInsertFunc(pRG, &newAslString);
	if (newAslString.nStringUnicodesAllocFlag == UNICODES_ALLOCATION_TRIED_WITH_ERROR)
	{
		status = decumaAllocationFailed;
		goto createAndQualifyNewString_exit;
	}

	if (replaceeElemRef)
	{
		replaceeCandidateRef = decumaQCHFindCand(pRG->pStringCandHandler, pReplaceeCandidate->sortingDist, pReplaceeCandidate);

		decumaHHRemove(pRG->pStringCheckSumHandler, replaceeElemRef);

		preRemovalFunc(pRG, pReplaceeCandidate); /*Don't call this function unless we are sure that the candidate is replaced */
	}
	else
	{
		replaceeCandidateRef = NULL;
	}

	addNewString(pRG, pRgNode, &newAslString, replaceeCandidateRef);

	nCandidates = decumaQCHGetCount(pRG->pStringCandHandler);

	bAdded = 1;

createAndQualifyNewString_exit:

	if (!bAdded)
	{			
		/*We need to free the allocations if we don't add the string to candidate handler. */
		/*If it is added now and later removed, this will be taken care of by the candidate handler  */
		/*calling preRemovalFunc. */
		/*If it is added and stays in handler the freeing will finally be done upon destruction of RG */
		preRemovalFunc(pRG, &newAslString);
	}

	return status;
}

static void addNewString(ASL_RG* pRG,
						 ASL_RG_NODE* pNode,
						 ASL_RG_STRING* pStr,
						 const DECUMA_QCH_CANDREF* replaceeCandidateRef)
{
	decumaQCHAdd(pRG->pStringCandHandler,  pStr->sortingDist, pStr, &replaceeCandidateRef);
	pNode->nStrings = decumaQCHGetCount(pRG->pStringCandHandler);

	decumaAssert(replaceeCandidateRef);
	
	/* Update nQualificationDist128 */
	if (decumaQCHGetCount(pRG->pStringCandHandler) == pNode->nMaxStrings)
	{
		pNode->nQualificationDist128 = decumaQCHGetKey(decumaQCHGetPrevCandRef(decumaQCHGetEndOfCandRefs(pRG->pStringCandHandler)));
	}
	else
	{
		pNode->nQualificationDist128 = DECUMA_MAX_DISTANCE;
	}

#ifndef NO_GUESSES_FOR_SPEED
	if (pRG->nNodes > 1 && decumaQCHGetCount(pRG->pStringCandHandler) > 0)
	{
		/* The qualification dist a factor on the currently best dist or */
		/* the currently worst dist if the candidate handler is full and */
		/* this is lower than the above. */
		pNode->nQualificationDist128 = ASL_MIN(pNode->nQualificationDist128, (3 * 128 + 2 * pNode->nNodeAntiProgressionSqrt128) / 2 * decumaQCHGetKey(decumaQCHGetNextCandRef(decumaQCHGetStartOfCandRefs(pRG->pStringCandHandler))) / 128);

		/* NOTE Now it is possible that the candidate handler contains candidates that are no longer qualifying */
		/* (if the newly added candidate is a new best candidate). Removing no longer qualifying candidates */
		/* has been tested, but it seems qualification loss is rare and the speed effect of handling this */
		/* was negative, so we do not care about removing them (it has no effect on recognition rate). */
	}
#endif

	pNode->nQualificationDist128 *= 128;

	/* TODO nQualificationDist128 also needs to be updated here accordingly */
	decumaHHAdd(pRG->pStringCheckSumHandler, pStr->nUnicodeCheckSum, decumaQCHGetCand(replaceeCandidateRef));
}

static long getConnectionDist(const ASL_RG * pRG,
							  int nNodeIdx,
							  int nEdgeIdx,
							  int nStartStrIdx,
							  int bUseHorisontalConnDist)
{
	const ASL_SG_NODE* pNode = aslSGGetNode(pRG->pSG, nNodeIdx);
	const ASL_SG_EDGE* pEdge = aslSGGetEdge(pNode, nEdgeIdx);
	int nStartNodeIdx = aslSGGetStartNode(pEdge);
	const ASL_RG_STRING* pStartStr = &pRG->pNodes[nStartNodeIdx]->pStrings[nStartStrIdx];
	const ASL_RG_STRING* pPrevStr = pStartStr;
	const ASL_SG_EDGE* pPrevEdge;
	int nPrevEdgeNode = nStartNodeIdx;
	long nConnectionDist = 0;

	if (!aslSGGetSymbol(pEdge) || !aslSGGetSymbol(pEdge)[0]) return aslSGGetConnectionDistance(pRG->pSG, nNodeIdx, nEdgeIdx, pStartStr->lastEdgeIdx); /* Just return noise connection dist according to SG */

	while (pPrevStr && (!aslSGGetSymbol(pPrevEdge = aslSGGetEdge(aslSGGetNode(pRG->pSG, nPrevEdgeNode), pPrevStr->lastEdgeIdx)) || !aslSGGetSymbol(pPrevEdge)[0]))
	{
		/* Empty symbol edge is assumed noise stroke. Skip edge for horisontal connection distance. */

		nPrevEdgeNode = pPrevStr->nStartNode;
		pPrevStr = nPrevEdgeNode > 0 ? pPrevStr->pStartStr : NULL;
	}

	if (!pPrevStr) return 0; /* Only noise previously in string. TODO return noise connection dist instead. */

	/* This edge represents a symbol (i.e. not noise). We have a preceeding string. */

	if (pRG->pScratchPad->preCalculatedConnDists[pPrevStr->lastEdgeIdx])
	{
		nConnectionDist = pRG->pScratchPad->preCalculatedConnDists[pPrevStr->lastEdgeIdx];
	}
	else
	{
		const ASL_RG_STRING* pPrevStrWithRef = pPrevStr;
		const ASL_SG_EDGE* pPrevEdgeWithRef = pPrevEdge;
		int nPrevEdgeNodeWithRef = nPrevEdgeNode;
		const ASL_RG_STRING* pPrevStrNonSymm = pPrevStr;
		const ASL_SG_EDGE* pPrevEdgeNonSymm = pPrevEdge;
		int nPrevEdgeNodeNonSymm = nPrevEdgeNode;
		DECUMA_INT16 nOldThisScalingAndVerticalOffsetDist, nBackwardThisScalingAndVerticalOffsetDist;
		DECUMA_INT16 nForwardPrevScalingAndVerticalOffsetDist;
		DECUMA_INT16 nOldThisRotationDist, nBackwardThisRotationDist;
		DECUMA_INT16 nForwardPrevRotationDist;
		int nThisArcs, nPrevArcs;
		DECUMA_INT32 nAverageDistBaseToHelpLineEstimate = aslArcSessionGetDistBaseToHelpline(pRG->pArcSession);

		nThisArcs = nNodeIdx - nStartNodeIdx;
		nPrevArcs = nPrevEdgeNode - pPrevStr->nStartNode;

		while (pPrevStrWithRef && ((!aslSGGetSymbol(pPrevEdgeWithRef = aslSGGetEdge(aslSGGetNode(pRG->pSG, nPrevEdgeNodeWithRef), pPrevStrWithRef->lastEdgeIdx)) || !aslSGGetSymbol(pPrevEdgeWithRef)[0]) ||
			aslSGGetBaseLineYEstimate(pPrevEdgeWithRef) == aslSGGetHelpLineYEstimate(pPrevEdgeWithRef)))
		{
			/* Empty symbol edge is assumed noise stroke. Skip edge for horisontal connection distance. */

			nPrevEdgeNodeWithRef = pPrevStrWithRef->nStartNode;
			pPrevStrWithRef = nPrevEdgeNodeWithRef > 0 ? pPrevStrWithRef->pStartStr : NULL;
		}

		while (pPrevStrNonSymm && ((!aslSGGetSymbol(pPrevEdgeNonSymm = aslSGGetEdge(aslSGGetNode(pRG->pSG, nPrevEdgeNodeNonSymm), pPrevStrNonSymm->lastEdgeIdx)) || !aslSGGetSymbol(pPrevEdgeNonSymm)[0]) ||
			scrGetSymmetry(aslSGGetOutput(pPrevEdgeNonSymm)) == 0))
		{
			/* Empty symbol edge is assumed noise stroke. Skip edge for horisontal connection distance. */

			nPrevEdgeNodeNonSymm = pPrevStrNonSymm->nStartNode;
			pPrevStrNonSymm = nPrevEdgeNodeNonSymm > 0 ? pPrevStrNonSymm->pStartStr : NULL;
		}

		if (pPrevStrNonSymm == NULL) pPrevEdgeNonSymm = NULL;

		if (bUseHorisontalConnDist && pRG->writingDirection != unknownWriting && nAverageDistBaseToHelpLineEstimate)
		{
			/* We have a global reference scale. We can make therefore make expectations about horisontal */
			/* positioning of this symbol based on that. */
			/* */
			/* Punish unexpected horisontal displacement of symbol relative to previous symbol. */

			DECUMA_COORD nThisMinX, nThisMaxX, nPrevMinX, nPrevMaxX;
			int nDistBaseToHelpline = nAverageDistBaseToHelpLineEstimate;
			int nDelta, nNegativeDelta, nPositiveDelta;
			int a;

			nThisMinX = MAX_DECUMA_COORD;
			nThisMaxX = MIN_DECUMA_COORD;
			nPrevMinX = MAX_DECUMA_COORD;
			nPrevMaxX = MIN_DECUMA_COORD;

			for (a = 0; a < nThisArcs; a++)
			{
				const ASL_ARC* pArc = aslArcSessionGetArc(pRG->pArcSession, nPrevEdgeNode + a);
				
				if (aslArcGetXMin(pArc) < nThisMinX) nThisMinX = aslArcGetXMin(pArc);
				if (aslArcGetXMax(pArc) > nThisMaxX) nThisMaxX = aslArcGetXMax(pArc);
			}

			for (a = 0; a < nPrevArcs; a++)
			{
				const ASL_ARC* pArc = aslArcSessionGetArc(pRG->pArcSession, pPrevStr->nStartNode + a);
				
				if (aslArcGetXMin(pArc) < nPrevMinX) nPrevMinX = aslArcGetXMin(pArc);
				if (aslArcGetXMax(pArc) > nPrevMaxX) nPrevMaxX = aslArcGetXMax(pArc);
			}

			if (pRG->writingDirection == onTopWriting)
			{
				int nThisMeanX, nPrevMeanX;

				nThisMeanX = (nThisMinX + nThisMaxX) / 2;
				nPrevMeanX = (nPrevMinX + nPrevMaxX) / 2;

				/* We expect same mean X */
				nNegativeDelta = ASL_ABS(nThisMeanX - nPrevMeanX);

				nPositiveDelta = ASL_MAX(nThisMinX - nPrevMaxX - nDistBaseToHelpline / 5,
					nPrevMinX - nThisMaxX - nDistBaseToHelpline / 5);
			}
			else if (pRG->writingDirection == leftToRight)
			{
				int nExpectedThisMinX = nPrevMaxX + nDistBaseToHelpline / 5;
				int nExpectedPrevPrevPrevMaxX = nPrevMinX - 7 * nDistBaseToHelpline / 5;
				int nExpectedNextMinX = nPrevMaxX + 7 * nDistBaseToHelpline / 5;

				nNegativeDelta = ASL_ABS(nExpectedThisMinX - nThisMinX);

				nPositiveDelta = ASL_MAX(nExpectedPrevPrevPrevMaxX - nThisMaxX, nThisMinX - nExpectedNextMinX);
			}
			else if (pRG->writingDirection == rightToLeft)
			{
				int nExpectedThisMaxX = nPrevMinX - nDistBaseToHelpline / 5;
				int nExpectedPrevPrevPrevMinX = nPrevMaxX + 7 * nDistBaseToHelpline / 5;
				int nExpectedNextMaxX = nPrevMinX - 7 * nDistBaseToHelpline / 5;

				decumaAssert(pRG->writingDirection == rightToLeft);

				nNegativeDelta = ASL_ABS(nThisMaxX - nExpectedThisMaxX);

				nPositiveDelta = ASL_MAX(nThisMinX - nExpectedPrevPrevPrevMinX, nExpectedNextMaxX - nThisMaxX);
			}

			/* We need to cap the maximum horisontal connection distance	 */
			if (pRG->writingDirection == onTopWriting)
			{
				if (nNegativeDelta > nDistBaseToHelpline / 3) nNegativeDelta = nDistBaseToHelpline / 3;
			}
			else
			{
				if (nNegativeDelta > nDistBaseToHelpline) nNegativeDelta = nDistBaseToHelpline;
			}

			nDelta = nNegativeDelta;

			if (nPositiveDelta > 0) nDelta -= nPositiveDelta;

			if (nDelta > 0) nConnectionDist += (pRG->writingDirection == onTopWriting ? 850 : 300) * nDelta / nDistBaseToHelpline;
		}
		else
		{
			/* We need a fix connection distance here to reduce over segmentation */
			nConnectionDist += FIX_UNKNOWN_WRITING_DIRECTION_CONN_DIST;
		}

		/* Scaling and vertical offset connection dist */
		scrGetScalingAndVerticalOffsetPunish(aslSGGetOutput(pEdge), aslSGGetScrCurveProp(pEdge), aslArcSessionGetBaseline(pRG->pArcSession), aslArcSessionGetBaseline(pRG->pArcSession) - aslArcSessionGetDistBaseToHelpline(pRG->pArcSession), &nOldThisScalingAndVerticalOffsetDist);
		scrGetScalingAndVerticalOffsetPunish(aslSGGetOutput(pEdge), aslSGGetScrCurveProp(pEdge), aslSGGetBaseLineYEstimate(pPrevEdgeWithRef), aslSGGetHelpLineYEstimate(pPrevEdgeWithRef), &nBackwardThisScalingAndVerticalOffsetDist);
		scrGetScalingAndVerticalOffsetPunish(aslSGGetOutput(pPrevEdge), aslSGGetScrCurveProp(pPrevEdge), aslSGGetBaseLineYEstimate(pEdge), aslSGGetHelpLineYEstimate(pEdge), &nForwardPrevScalingAndVerticalOffsetDist);

		if (pPrevEdgeNonSymm && scrGetSymmetry(aslSGGetOutput(pEdge)))
		{
			/* TODO find first non symmetric prev instead */
			/* TODO find a global refScale average as global rotation feature reference */
			scrGetRotationPunish(aslSGGetOutput(pEdge), aslSGGetScrCurveProp(pEdge), aslArcSessionGetBaseline(pRG->pArcSession), aslArcSessionGetBaseline(pRG->pArcSession) - aslArcSessionGetDistBaseToHelpline(pRG->pArcSession), 0, &nOldThisRotationDist);
			scrGetRotationPunish(aslSGGetOutput(pEdge), aslSGGetScrCurveProp(pEdge), aslArcSessionGetBaseline(pRG->pArcSession), aslArcSessionGetBaseline(pRG->pArcSession) - aslArcSessionGetDistBaseToHelpline(pRG->pArcSession), aslSGGetOutput(pPrevEdgeNonSymm)->simTransf.theta, &nBackwardThisRotationDist);
			scrGetRotationPunish(aslSGGetOutput(pPrevEdgeNonSymm), aslSGGetScrCurveProp(pPrevEdgeNonSymm), aslArcSessionGetBaseline(pRG->pArcSession), aslArcSessionGetBaseline(pRG->pArcSession) - aslArcSessionGetDistBaseToHelpline(pRG->pArcSession), aslSGGetOutput(pEdge)->simTransf.theta, &nForwardPrevRotationDist);

			nConnectionDist -= nOldThisRotationDist * nThisArcs;
			nConnectionDist += ROTATION_DIST_WEIGHT64 * nOldThisRotationDist * nThisArcs / 64;
			nConnectionDist += ROTATION_CONN_DIST_WEIGHT64 *
				ASL_MIN(nForwardPrevRotationDist, nBackwardThisRotationDist) / 64;
		}

		nConnectionDist -=nOldThisScalingAndVerticalOffsetDist * nThisArcs;

		nConnectionDist += SCALE_AND_VERTICAL_OFFSET_DIST_WEIGHT64 *
			nOldThisScalingAndVerticalOffsetDist * nThisArcs / 64;

		if (aslSGGetBaseLineYEstimate(pPrevEdgeWithRef) != aslSGGetHelpLineYEstimate(pPrevEdgeWithRef) &&
			aslSGGetBaseLineYEstimate(pEdge) != aslSGGetHelpLineYEstimate(pEdge))
		{
			nConnectionDist += SCALE_AND_VERTICAL_OFFSET_CONN_DIST_WEIGHT64 *
				(nBackwardThisScalingAndVerticalOffsetDist + nForwardPrevScalingAndVerticalOffsetDist) / 128;
		}
		else if (aslSGGetBaseLineYEstimate(pPrevEdgeWithRef) != aslSGGetHelpLineYEstimate(pPrevEdgeWithRef))
		{
			nConnectionDist += SCALE_AND_VERTICAL_OFFSET_CONN_DIST_WEIGHT64 *
				nBackwardThisScalingAndVerticalOffsetDist / 64;
		}
		else if (aslSGGetBaseLineYEstimate(pEdge) != aslSGGetHelpLineYEstimate(pEdge))
		{
			nConnectionDist += SCALE_AND_VERTICAL_OFFSET_CONN_DIST_WEIGHT64 *
				nForwardPrevScalingAndVerticalOffsetDist / 64;
		}
		else
		{
			/* Adjust back */

			nConnectionDist += (64 - SCALE_AND_VERTICAL_OFFSET_DIST_WEIGHT64) *
				nOldThisScalingAndVerticalOffsetDist * nThisArcs / 64;
		}

		/* Thai characters tend to have under segmentation problem (contrary to the */
		/* over segmentation problem other scripts have). Adjust connection distance */
		/* accordingly. */
		if (IS_IN_CATEGORY(pRG->cat, pRG->thaiLetterCat)) nConnectionDist += THAI_CONN_DIST_OFFSET;
		else if (IS_IN_CATEGORY(pRG->cat, pRG->cyrillicLetterCat)) nConnectionDist += CYRILLIC_CONN_DIST_OFFSET;
	}

	pRG->pScratchPad->preCalculatedConnDists[pStartStr->lastEdgeIdx] = nConnectionDist;

	return nConnectionDist;
}

static void searchCandidateReplacee(ASL_RG* pRG,
									DECUMA_QCH* pCH,
									ASL_RG_STRING* pStr,
									DECUMA_UINT8 nCheckSum,
									ASL_RG_STRING** ppReplaceeCandidate,
									DECUMA_HH_ELEMREF* pReplaceeElemRef,
									int *pnReplaceeDist,
									int bLastNode)
{
	ASL_RG_STRING* pReplaceeCandidate;
	DECUMA_HH_ELEMREF elemRef, replaceeElemRef;
	DECUMA_HH_ELEMREF worstCandWithCheckSumElemRef = NULL;
	int nReplaceeDist;
	int nWorstStringWithCheckSumBestPathDist = -1;
	int nStringsOfSameClass = 0;

	decumaAssert(ppReplaceeCandidate);
	decumaAssert(pReplaceeElemRef);
	decumaAssert(pnReplaceeDist);

	if (*pReplaceeElemRef)
	{
		/* Start point provided */
		elemRef = *pReplaceeElemRef;
	}
	else
	{
		/* Get start point */
		elemRef = decumaHHFindKey(pRG->pStringCheckSumHandler, nCheckSum);
	}

	while(elemRef)
	{
		decumaAssert(decumaHHGetKey(elemRef) == nCheckSum);

		pReplaceeCandidate = decumaHHGetData(elemRef);

		/* If this is not the worse than worst replacee found, skip they heavy replacee checks */
		if (pReplaceeCandidate->sortingDist <= nWorstStringWithCheckSumBestPathDist) goto searchCandidateReplacee_continue;

#ifdef _DEBUG_EXTRA										
		/*Check that the unicode checksum is working as it should... */
		decumaAssert( pStr->nStringUnicodes != pReplaceeCandidate->nStringUnicodes || 
			memcmp(pStr->pStringUnicodes,pReplaceeCandidate->pStringUnicodes, pStr->nStringUnicodes * sizeof(pReplaceeCandidate->pStringUnicodes[0]) ) != 0 ||
			nCheckSum == pReplaceeCandidate->nUnicodeCheckSum);
#endif /*_DEBUG_EXTRA */

		/* Keep unique string and if not last node unique (existing) dictionary nodes (start string nodes will */
		/* be compared, since pStr might not have been dictionary evaluated yet. */
		if (candidatesHaveSameSymbols(pStr->nStringUnicodes, pStr->pStringUnicodes, nCheckSum, pReplaceeCandidate) &&
			(bLastNode || candidatesHaveSameStartStringFirstDictionaryNode(pStr, pReplaceeCandidate)))
		{
			/*JM 2007-11-14: Here we end up: 800-1000 times ~1/300-1/750 of the number of calls to candidatesHaveSameSymbolsAndConnector */
			if (!bLastNode && pStr->pDictionaryEntries && pReplaceeCandidate->pDictionaryEntries)
			{
				int d;

				if (pReplaceeCandidate->nDictionaryEntries != pStr->nDictionaryEntries) 
					goto searchCandidateReplacee_continue;

				for (d=1;d<pStr->nDictionaryEntries; d++) /*First dictionary is already checked in macro above */
				{					
					if (pStr->pDictionaryEntries[d].pReference != pReplaceeCandidate->pDictionaryEntries[d].pReference) 
						goto searchCandidateReplacee_continue;
				}
			}

			PROF(EqualCandidates);
			worstCandWithCheckSumElemRef = elemRef;
			nWorstStringWithCheckSumBestPathDist = pReplaceeCandidate->sortingDist;
			nStringsOfSameClass++;
		}

searchCandidateReplacee_continue:

		if (nStringsOfSameClass == MAX_STRINGS_OF_SAME_CLASS) break;

		elemRef = decumaHHFindNextKey(elemRef);
	}
	/*JM 2007-11-14: 800-1000 of ~4300 times (20%) withDict / ~11000 times (~8%) noDict we DO find a non-worst-replacee */

	decumaAssert(nStringsOfSameClass <= MAX_STRINGS_OF_SAME_CLASS);

	if (nStringsOfSameClass < MAX_STRINGS_OF_SAME_CLASS) worstCandWithCheckSumElemRef = NULL;

	if (worstCandWithCheckSumElemRef == NULL)
	{
		if (decumaQCHIsFull(pCH))
		{
			pReplaceeCandidate = decumaQCHGetCand(decumaQCHGetPrevCandRef(decumaQCHGetEndOfCandRefs(pCH)));
			nReplaceeDist = pReplaceeCandidate->sortingDist;
			replaceeElemRef = decumaHHFindData(pRG->pStringCheckSumHandler, pReplaceeCandidate->nUnicodeCheckSum, pReplaceeCandidate);
		}	
		else
		{
			replaceeElemRef = NULL;
			pReplaceeCandidate = NULL;
			nReplaceeDist = DECUMA_MAX_DISTANCE;
		}
	}
	else
	{
		replaceeElemRef = worstCandWithCheckSumElemRef;
		pReplaceeCandidate = decumaHHGetData(replaceeElemRef);
		nReplaceeDist = pReplaceeCandidate->sortingDist;
	}

	decumaAssert(!replaceeElemRef || pReplaceeCandidate);
	decumaAssert(replaceeElemRef || !pReplaceeCandidate);

	*ppReplaceeCandidate = pReplaceeCandidate;
	*pReplaceeElemRef = replaceeElemRef;
	*pnReplaceeDist = nReplaceeDist;
}

/* "Original" checkWithDictionary is now a helper */
static DECUMA_STATUS performCheck(ASL_RG * pRG,
	const DECUMA_UNICODE * pThisUnicodes, 
	const DECUMA_UINT8 nThisUnicodes,
	const ASL_SG_EDGE * pThisEdge, 
	const ASL_RG_STRING * pStartStr, 
	const ASL_RG_NODE * pRgNode,
	const ASL_RG_DICTIONARY_ENTRY * pStartDictEntry,
	const ASL_RG_DICTIONARY_ENTRY * pOldStartDictEntry,
	ASL_RG_DICTIONARY_ENTRY * pNewDictEntry,
	int * pbEntryWritten,
	int bAlternativeDictSymbol,
	int bExpandNonDictSymbols)
{
	int bUseStartList = 1;

	decumaAssert(pNewDictEntry);
	decumaAssert(pbEntryWritten);
	decumaAssert(pStartDictEntry);

	*pbEntryWritten = 0;
#ifdef _DEBUG
	decumaMemset(pNewDictEntry,0,sizeof(ASL_RG_DICTIONARY_ENTRY));
#endif
	
	if (pThisEdge && nThisUnicodes > 0)
	{
		DECUMA_HWR_DICTIONARY_REF thisRef = pStartDictEntry->pReference;
		DECUMA_HWR_DICTIONARY_REF nextRef = NULL;
		DECUMA_HWR_DICTIONARY * pStartDictionary = pRG->pAllDictionaries[pStartDictEntry->dictIdx];
		DECUMA_UNICODE prevUnicode = 0;
		int bPunishableAlternativeSymbolExpansion = pOldStartDictEntry ? pOldStartDictEntry->bPunishableAlternativeSymbolExpansion : pStartDictEntry->bPunishableAlternativeSymbolExpansion;
		int u, nPathStart, nPathUnicodes;

		/*JM TEMP code to verify that thisRef points to the expected unicode */
		/*DECUMA_UNICODE unicode=0; */
		/*if (thisRef) { */
		/*	unicode = decumaDictionaryGetUnicode(pStartDictionary, thisRef); */
		/*} */

		decumaAssert(nThisUnicodes == 1);

		if (pStartStr && pStartStr->nStringUnicodes) prevUnicode = pStartStr->pStringUnicodes[pStartStr->nStringUnicodes-1];

		/*All characters must match */
		for (u=0, nPathStart=0, nPathUnicodes=0; u<nThisUnicodes; u++)
		{		
			DECUMA_UNICODE unicode = pThisUnicodes[u];
			int bIsLetter = decumaUnicodeIsLetter(unicode);

			if (u > 0) prevUnicode = pThisUnicodes[u-1];

			/* Leading or trailing non-letters get free pass when expanding non-dict symbols */
			/* TODO We probably need to do more general expansion validation here on a per unicode basis to */
			/* handle all prepend and multi char edge situations */
			/* TODO Not all of the non-letters has to be considered leaders or trailers! */
			if (bExpandNonDictSymbols &&
				!bIsLetter &&
				(thisRef == NULL || (pRgNode->pcdEdge.nMinRemLetters ==0 && decumaDictionaryGetEndOfWord(pStartDictionary,thisRef)))) continue;

			/* At string (re)starts we allow alternatives */
			if (bAlternativeDictSymbol)
			{
				decumaAssert(bIsLetter);
				decumaAssert(!bExpandNonDictSymbols);

				if (pStartDictEntry->nWordLen + nPathUnicodes == 0)
				{
					/* UpperToLower alternative of initial character in word */
					unicode = decumaUnicodeToLower(unicode, 0);

					if (unicode == pThisUnicodes[u]) unicode = decumaUnicodeToUpper(unicode, 0); /* No alternative symbol. Try convert lower to Upper instead */

					if (unicode == pThisUnicodes[u]) return decumaNoError; /* No alternative symbol to process */
				}
				else if ((!decumaUnicodeIsLetter(prevUnicode) || decumaUnicodeToLower(prevUnicode, 0) != prevUnicode) &&
					decumaUnicodeToLower(unicode, 0) != unicode)
				{
					/* Prev is upper and this is upper. Lower should be allowed as alternative to support consistent case expansion. */
					unicode	= decumaUnicodeToLower(unicode, 0);
					bPunishableAlternativeSymbolExpansion = 1;
				}
				else
				{
					return decumaNoError;
				}
			}
			else if (bPunishableAlternativeSymbolExpansion && bIsLetter)
			{
				/* Punishable alternative on non-first letter. Force continued alternative only. */

				decumaAssert(!bExpandNonDictSymbols);

				return decumaNoError;
			}

			if (pStartStr && STR_HAS_TRAILER(pStartStr->nStringUnicodes, pStartDictEntry) ||
				!(nextRef = decumaDictionaryGetSubstringRef(pStartDictionary, thisRef, &unicode, 1)))
			{
				/* Node is already not matching whole string to the end cannot continue to match it. */

				/* Temp fix to get relevant results for at least English */
				/*FAIL: Return without setting adding a new dict entry */
				/*This is an invalid path. Return the NULL-pointer. */
				/*Don't need to do the complete word check */

				return decumaNoError;
			}		
			
			/*OK:  */
			/* Allow this string, but don't update the dict entry			 */
			thisRef = nextRef;
			if (nPathUnicodes == 0) nPathStart = u;
			nPathUnicodes++;
		}

		if (bExpandNonDictSymbols && nPathUnicodes == nThisUnicodes) return decumaNoError; /* Must expand if we are expanding */
		if (!bExpandNonDictSymbols && nPathUnicodes < nThisUnicodes) return decumaNoError; /* Must not expand if we are not expanding */

		if (nPathUnicodes)
		{
			/* Write new node (copy thisref, basically) */
			if (pOldStartDictEntry)
			{
				/*Contract.  */
				DECUMA_UINT8 oldFreqClass = decumaDictionaryGetWordFreqClass(pRG->pAllDictionaries[pOldStartDictEntry->dictIdx],
					pOldStartDictEntry->pReference); /*Use word frequency. Left side should be a complete word */
				decumaAssert(oldFreqClass != MAX_DECUMA_UINT8); /*It should be a complete word and then have a relevant freq class */
				pNewDictEntry->nLeftBoost = pRG->pDictionaryData[pOldStartDictEntry->dictIdx].pFreqClassBoost[oldFreqClass];
				pNewDictEntry->nLeftStart = pOldStartDictEntry->nWordStart;
			}
			else
			{
				pNewDictEntry->nLeftBoost =  pStartDictEntry->nLeftBoost;
				pNewDictEntry->nLeftStart =  pStartDictEntry->nLeftStart;
			}

			pNewDictEntry->nWordStart = (pOldStartDictEntry ? pOldStartDictEntry->nWordStart : 0) + pStartDictEntry->nWordStart + nPathStart;
			pNewDictEntry->nWordLen = (pOldStartDictEntry ? pOldStartDictEntry->nWordLen : 0) + pStartDictEntry->nWordLen + nPathUnicodes;
			pNewDictEntry->bPunishableAlternativeSymbolExpansion = bPunishableAlternativeSymbolExpansion;
			pNewDictEntry->pReference = thisRef;
			pNewDictEntry->dictIdx = pStartDictEntry->dictIdx;
			*pbEntryWritten = 1;
			bUseStartList=0;
		}
	}

	if (bUseStartList)
	{
		*pNewDictEntry = *pStartDictEntry;
		
		if (pNewDictEntry->nWordLen == 0)
		{
			pNewDictEntry->nWordStart += nThisUnicodes;
		}

		*pbEntryWritten = 1;
	}

	/*minMax restrictions */
	if ( *pbEntryWritten && pNewDictEntry->pReference && pRG->strCompleteness == willNotBeContinued)
	{
		/*The default values are used when the rg string has no characters */
		DECUMA_UINT8 dictMinRemLetters=1,dictMaxRemLetters=MAX_DECUMA_UINT8;

		/*Here we use the "letters" instead of "unicodes" since some unicodes can freely */
		/*be placed inside dicitonary words without counting. */
		int rgMaxRemLetters = pRgNode->pcdEdge.nMaxRemLetters;
			
		int rgMinRemLetters = pRgNode->pcdEdge.nMinRemLetters;
				 
		decumaDictionaryGetMinMax(pRG->pAllDictionaries[pNewDictEntry->dictIdx], pNewDictEntry->pReference,
			&dictMinRemLetters, &dictMaxRemLetters);
		
		decumaAssert(dictMinRemLetters<=dictMaxRemLetters);

		if (!STR_HAS_CONTRACTED(pNewDictEntry))
		{
			/* If this is not contracted, then we must assume it can be and then that */
			/* right side can be max length. This essentially devalues dictMaxRemLetters */
			/* to nothing, but it is better than false contraction rejection. */
			dictMaxRemLetters += pRG->nMaxDictWordLen;
		}		

		PROF(TCanNotBeCompleteWord);
		if ( dictMinRemLetters > rgMaxRemLetters || 
			dictMaxRemLetters < rgMinRemLetters)
		{
			/*This is a path that cannot end up in a complete word. */
			PROF(CanNotBeCompleteWord);
			*pbEntryWritten = 0; 
		} else
		{
			DECUMA_UINT8 freqClass = decumaDictionaryGetSubTreeFreqClassGivenMinMax(
				pRG->pAllDictionaries[pNewDictEntry->dictIdx], pNewDictEntry->pReference,
				(STR_HAS_CONTRACTED(pNewDictEntry) ? rgMinRemLetters : 0),rgMaxRemLetters);
				/*Note that if str has not contracted we cannot check rgMinRemLetters against dictMaxRemLetters */
				/*since dictMaxRemLetters can be more than the contents of one dictionary when we allow contractions */
				/*However if we already have contracted and disallow mulitple contractions. The condition can be used. */

			if (freqClass ==MAX_DECUMA_UINT8) 
			{
				/*This can happen e.g. if the subtree of the dict contains "baby,fc=2" and "babysitter,fc=4" and the */
				/*rg-candidate is "baby" and rgMinRemLetters,rgMaxRemLetters =(3,5). Then no word */
				/*fits the minmax and the output should not be boosted. */
				/*This should really be noticed in checkWithDictionary instead, but we don't have the  */
				/*information of min,max conditioned on frequency class as accurately as before. */
				/*And decumaDictionaryGetSubTreeFreqClassGivenMinMax() takes time to run too often. */
				PROF(CanNotBeCompleteWord2);
				*pbEntryWritten = 0; 
			}
		}
	}

	return decumaNoError;
}

static DECUMA_STATUS checkWithDictionary(ASL_RG * pRG,
	const DECUMA_UNICODE * pThisUnicodes, 
	DECUMA_UINT8 nThisUnicodes,
	const ASL_SG_EDGE * pThisEdge, 
	const ASL_RG_STRING * pStartStr, 
	const ASL_RG_NODE * pRgNode,
	ASL_RG_DICTIONARY_ENTRY ** ppNewDictEntries,
	int * pnNewDictEntries,
	DECUMA_UINT8 *pbIsListOwner,
	int bTrigramCheckOnly,
	int bContractionExpansion,
	int bAlternativeDictSymbol,
	int bExpandNonDictSymbols,
	int bRestartDictString)
{
	DECUMA_STATUS status = decumaNoError;
	
	const ASL_RG_DICTIONARY_ENTRY * pOldStartDictEntry = NULL;
	DECUMA_UINT32 nBestMemberBoost = 0;

	/* TODO This could/should be moved to the RG scratch pad, that way we could  */
	/* allocate pRG->nAllDictionaries work-entries rather than use a fix number //CH */
	ASL_RG_DICTIONARY_ENTRY newEntries[16];
	DECUMA_UINT8 nNewEntries = 0;
	int nStartNodeIdx = aslSGGetStartNode(pThisEdge);
	int n,nDicts;

	*pbIsListOwner = 0;
	*ppNewDictEntries = NULL;
	*pnNewDictEntries = 0;

	if (bContractionExpansion)
	{
		/* Contraction request. Check if it is possible. */
		int d;

		decumaAssert(pStartStr);

		/*The rules are that the first part must be a complete word. Check that! */
		for ( d=0; d<pStartStr->nDictionaryEntries; d++)
		{
			DECUMA_UINT8 freqClass;
			DECUMA_UINT32 nMemberBoost;
			const ASL_RG_DICTIONARY_ENTRY * pStartDictEntry = &pStartStr->pDictionaryEntries[d];
			DECUMA_HWR_DICTIONARY * pStartDict = pRG->pAllDictionaries[pStartDictEntry->dictIdx];


			if (pStartDictEntry->nWordLen == 0) continue; /* Contraction after leader is not allowed */

			if (STR_HAS_TRAILER(pStartStr->nStringUnicodes,pStartDictEntry)) continue; /* Contraction after trailer is not allowed */

			if (STR_HAS_CONTRACTED(pStartDictEntry)) continue; /* Multiple contractions are not allowed */

			if (!decumaDictionaryGetEndOfWord(pStartDict,pStartDictEntry->pReference)) continue; /* Contracting incomplete word is not allowed */

			freqClass = decumaDictionaryGetWordFreqClass(pStartDict,pStartDictEntry->pReference);
			nMemberBoost = pRG->pDictionaryData[d].pFreqClassBoost[freqClass];

			if (nMemberBoost > nBestMemberBoost)
			{
				/*NOTE: This might not be the optimal handling of contractions between different dictionaries */
				/*      When we have several dicitionaries e.g. from several languages we need to define */
				/*      rules how to support contractions between dictionaries. //JM */
				pOldStartDictEntry = pStartDictEntry;
				nBestMemberBoost = nMemberBoost;
			}
		}

		if (!pOldStartDictEntry) return decumaNoError;

		/* Ok to contract */

		pStartStr = NULL;
		nBestMemberBoost = 0; /* Reset to 0 for later use */
	}

	if (bRestartDictString)
	{
		pStartStr = NULL;
		nBestMemberBoost = 0; /* Reset to 0 for later use */
	}

	/* TODO: We could use the pRG->pScratchPad to hold  */
	if ( !pStartStr ) 
	{
		/* No start string, use the RG dictionary list */
		nDicts = pRG->nAllDictionaries;
	} 
	else 
	{
		/* We have a start string, use its dictionary list */
		nDicts = pStartStr->nDictionaryEntries;
	}

	for ( n = 0; n < nDicts && status == decumaNoError; n++ )
	{
		ASL_RG_DICTIONARY_ENTRY * pStartDictEntry;
		ASL_RG_DICTIONARY_ENTRY rootEntry;

		if ( !pStartStr ) 
		{
			decumaMemset(&rootEntry,0,sizeof(rootEntry));
			rootEntry.dictIdx = n;
			pStartDictEntry = &rootEntry;
		} 
		else 
		{
			pStartDictEntry = &pStartStr->pDictionaryEntries[n];
		}

/* Currently the performCheck does more dynamic expansion than is done here. Avoid */
/* trigram check to allow this dynamic expansion to work */
/* TODO fix properly (same dynamic expansion for trigram lookup as for dictionary lookup) */
#if defined(CHECK_TRIGRAMS)
		/* Perform the "full" dictionary check only if 
		 * -> we can't do a fast-fail check because the trigram table is missing
		 * -> the key exists in the trigram table 
		 */

		DECUMA_TRIGRAM_TABLE * pTrigramTable = (DECUMA_TRIGRAM_TABLE *) pStartList->pDictionary->pTrigramTable;
		DECUMA_INT16 exists = -1;

		/* Check if the current trigram exists in the dictionary.
		 * We have six specific cases that are handled explicitly: 
		 */
		if ( nThisUnicodes != 0 && pTrigramTable ) 
		{
			switch ( nThisUnicodes )
			{
			case 1:
				if ( !pStartStr || pStartStr->nStringUnicodes == 0 ) {
					/* Case #1: A single unicode as candidate. No previous string, just the current candidate */
					exists = decumaTrigramExists(pTrigramTable, 0, 0, pThisUnicodes[0]);
				} else if ( pStartStr->nStringUnicodes == 1 ) {
					/* Case #2: A single unicode as candidate, one unicode in the previous string */
					exists = decumaTrigramExists(pTrigramTable, 0, pStartStr->pStringUnicodes[0], pThisUnicodes[0]);
				} else {
					/* Case #3: A single unicode as candidate, two or more unicodes in the previous string */
					exists = decumaTrigramExists(pTrigramTable, pStartStr->pStringUnicodes[pStartStr->nStringUnicodes - 2], pStartStr->pStringUnicodes[nStartStrUnicodes - 1], pThisUnicodes[0]);
				}
				break;

			case 2:
				if ( !pStartStr || pStartStr->nStringUnicodes == 0 ) 
				{
					/* Case #4: Two unicodes as candidate, no unicodes in the previous string */
					exists = decumaTrigramExists(pTrigramTable, 0, pThisUnicodes[0], pThisUnicodes[1] );
				} else {
					/* Case #5: Two unicodes as candidate, one or more unicodes in the previous string */
					exists = decumaTrigramExists(pTrigramTable, pStartStr->pStringUnicodes[pStartStr->nStringUnicodes - 1], pThisUnicodes[0], pThisUnicodes[1] );
				}
				break;

			default:
				/* Case #6: We have a candidate of three or more unicodes; the trigram can be calculated fully from the candidate.
				 *
				 * TODO: Should this special case be assert'ed zero?
				 */
				exists = decumaTrigramExists(pTrigramTable, pThisUnicodes[nThisUnicodes - 3], pThisUnicodes[nThisUnicodes - 2], pThisUnicodes[nThisUnicodes - 1] );

				break;
			}
		}

		if ( bTrigramCheckOnly )
		{
			if ( exists != 0 )
			{
				DECUMA_UINT32 nMemberBoost = ZiGetMemberBoost(pStartList->pDictionary);

				if (nMemberBoost > nBestMemberBoost)
				{
					*ppNewList = pStartList;
					nBestMemberBoost = nMemberBoost;
				}
			}
		}
		else if ( exists != 0 ) 
#endif /*CHECK_TRIGRAMS */
		{
			int bAddedEntry;
			/* Do the full search unless we know for sure that the current triplet of unicodes does not occur in the dictionary.
			 * If the parameters to decumaTrigramExists() are out of bounds it will return -1, and we still do the full search. 
			 * This can be caused by gestures, digits et al in the input, and is handled specifically in performCheck().
			 */
			status = performCheck(pRG, 
				pThisUnicodes, 
				nThisUnicodes, 
				pThisEdge, 
				pStartStr,
				pRgNode, 
				pStartDictEntry,
				pOldStartDictEntry,
				&newEntries[nNewEntries],
				&bAddedEntry,
				bAlternativeDictSymbol,
				bExpandNonDictSymbols);

			if (status!=decumaNoError) goto checkWithDictionary_error;

			if (bAddedEntry && bRestartDictString)
			{
				/* We need to adjust nWordStart to be nStringUnicodes consistent */

				ASL_RG_DICTIONARY_ENTRY * pEntry = &newEntries[nNewEntries];

				decumaAssert(!pOldStartDictEntry);
				pEntry->nWordStart += pRG->nStringStartLen;
			}

			nNewEntries += bAddedEntry;
		}
	}

	if ( nNewEntries > 0 )
	{
		unsigned int size = nNewEntries * sizeof(**ppNewDictEntries);
		
		*ppNewDictEntries = MEMORYPOOLHANDLER_ALLOC(pRG->pDictMemPoolHandler, size, 0);
		if ( !*ppNewDictEntries ) 
		{
			status = allocNewDictMemPool(pRG);
			if (status==decumaNoError)
			{
				*ppNewDictEntries = MEMORYPOOLHANDLER_ALLOC(pRG->pDictMemPoolHandler, size, 0);
				decumaAssert(ppNewDictEntries);
			}
		}
		if (status!=decumaNoError) goto checkWithDictionary_error;	

		decumaMemcpy(*ppNewDictEntries, &newEntries[0], size);
		*pnNewDictEntries = nNewEntries;
		*pbIsListOwner = 1;
	}
	
	return status;

checkWithDictionary_error:
	
	return status;
}


/* TODO Proper handling of this */
static int isExpectedWordTrailer(DECUMA_UNICODE unicode)
{
	return unicode == ' ' || unicode == ',' || unicode == '.' || unicode == '!' || unicode == '?';  
}

/* TODO Proper handling of this */
static int isExpectedWordTrailerTrailer(DECUMA_UNICODE unicode)
{
	return unicode == ' ';  
}


static DECUMA_UINT16 getRankBoost1024(const ASL_RG * pRG,
									  ASL_RG_DICTIONARY_ENTRY * pDictionaryEntries, 
									  int nDictionaryEntries,
									  DECUMA_UNICODE* pStringUnicodes,
									  int nStringUnicodes,
									  const ASL_RG_NODE * pRgNode,
									  int nIgnoredStringStartLen,
									  DECUMA_INT8 * pBestDictEntryIdx)
{
	int i;

	decumaAssert( ( (pDictionaryEntries == 0) && (nDictionaryEntries == 0) ) || ( (pDictionaryEntries != 0) && (nDictionaryEntries > 0) ));
	decumaAssert(pBestDictEntryIdx);
	*pBestDictEntryIdx=-1;
	if (!pDictionaryEntries)
	{
		return 0;
	}
	else
	{
		DECUMA_UINT32 rankBoost = 0;

		decumaAssert(nDictionaryEntries > 0);

		*pBestDictEntryIdx=0; /* Init to this in case all have 0 boost (e.g. all just leaders) */

		for ( i = 0; i < nDictionaryEntries; i++ )
		{
			ASL_RG_DICTIONARY_ENTRY * pEntry = &pDictionaryEntries[i];
			const DECUMA_HWR_DICTIONARY * pDict = pRG->pAllDictionaries[pEntry->dictIdx];
			DECUMA_UINT8 freqClass;
			DECUMA_UINT32 memberBoost;
			int nWordEnd = pEntry->nWordStart + pEntry->nWordLen - 1;
			int bBoostForEndOfWord = 0;
			/*Here we use the "letters" instead of "unicodes" since some unicodes can freely */
			/*be placed inside dicitonary words without counting. */
			int rgMaxRemLetters = pRgNode->pcdEdge.nMaxRemLetters;
				
			int rgMinRemLetters = pRgNode->pcdEdge.nMinRemLetters;
				
			if (pEntry->nWordLen==0) continue; /*Will not be boosted anyway */

			if (pRG->strCompleteness == willNotBeContinued && pRgNode->pcdEdge.nMaxRemLetters==0)
			{
				/*We will not add any more letters. We will (have to) use the word freq class */
				/*which might be worse than the subtree freq class (eg. "yo" vs. "you" and "yours" vs. "yourself") */
				bBoostForEndOfWord =  1;
			}
			if (STR_HAS_TRAILER(nStringUnicodes,pEntry))
			{
				/*We have a trailer. Boost for word end. */
				bBoostForEndOfWord =  1;
			}
			
			if (bBoostForEndOfWord)
			{
				freqClass = decumaDictionaryGetWordFreqClass(pDict, pEntry->pReference);
				decumaAssert(freqClass!=MAX_DECUMA_UINT8); /*We shall be at a word end. They shall have a valid word freq class */
			}
			else
			{
				if (pRG->strCompleteness == willNotBeContinued)
				{
					/*Take min and max into consideration to get a frequency class that is valid in the interval */
					/*of string lengths in {rgMinRemLetters,rgMaxRemLetters}. */

					freqClass = decumaDictionaryGetSubTreeFreqClassGivenMinMax(pDict, pEntry->pReference,
						(STR_HAS_CONTRACTED(pEntry)?rgMinRemLetters:0),rgMaxRemLetters);
						/*Note that if str has not contracted we cannot check rgMinRemLetters against dictMaxRemLetters */
						/*since dictMaxRemLetters can be more than the contents of one dictionary when we allow contractions */
						/*However if we already have contracted and disallow mulitple contractions. The condition can be used. */


				} 
				else 
				{
					/*Don't bother about min max */
					freqClass = decumaDictionaryGetSubTreeFreqClass(pDict, pEntry->pReference);
				}
			}

			decumaAssert(freqClass<decumaDictionaryGetNFreqClasses(pDict));

			memberBoost = pRG->pDictionaryData[i].pFreqClassBoost[freqClass];
			
			/* Only one non-dictionary symbol should not result in boost reduction, */
			/* since e.g. a trailing space gesture must be considered a common */
			/* use case. I.e. we avoid boost reduction for the case when a */
			/* space gesture follows a complete word. As a side effect we */
			/* also avoid boost reduction for other cases e.g. trailing */
			/* punctuations (good) or other trailing non-letter (bad?) or */
			/* leading gestures, punctuation or non-letters (bad? or won't */
			/* connect anyway). Note that with two or more non-dictionary */
			/* symbols there will be a boost reduction proportional to the */
			/* amount of these. E.g. punctuation plus gesture after complete */
			/* word will get reduction, but this should be less of a problem */
			/* since the two non-dictionary symbols case should have a lot */
			/* less conflicts with word endings. */

			/* */
			/* NEW: Only allow full boost for non-dictionary symbols if */
			/* there's only one non-dictionary symbol and it is an expected */
			/* complete word trailer. */

			if (pRG->strCompleteness == canBeContinued && !STR_HAS_TRAILER(nStringUnicodes,pEntry))
			{
				/* TODO This (costly?) calculation has already been done. Store and reuse result. */
				
				/*The default values are used when the rg string has no characters */
				DECUMA_UINT8 dictMinRemLetters,dictMaxRemLetters;
				decumaDictionaryGetMinMax(pDict, pEntry->pReference, &dictMinRemLetters, &dictMaxRemLetters);

				decumaAssert(dictMinRemLetters<=dictMaxRemLetters);

				if (!STR_HAS_CONTRACTED(pEntry))
				{
					/* If this is not contracted, then we must assume it can be and then that */
					/* right side can be max length. This essentially devalues dictMaxRemLetters */
					/* to nothing, but it is better than false contraction rejection. */
					dictMaxRemLetters += pRG->nMaxDictWordLen;
				}

				if ( dictMinRemLetters > rgMaxRemLetters || 
					dictMaxRemLetters < rgMinRemLetters)
				{
					/*This is a path that cannot end up in a complete word */
					/*Reduce boost for incomplete words. Tuned for best balance. */
					memberBoost = memberBoost* 120 / 128;
				} 
				else
				{
					

					DECUMA_UINT8 freqClassComplete = decumaDictionaryGetSubTreeFreqClassGivenMinMax(pDict,pEntry->pReference,
						(STR_HAS_CONTRACTED(pEntry)?rgMinRemLetters:0),rgMaxRemLetters);
						/*Note that if str has not contracted we cannot check rgMinRemLetters against dictMaxRemLetters */
						/*since dictMaxRemLetters can be more than the contents of one dictionary when we allow contractions */
						/*However if we already have contracted and disallow mulitple contractions. The condition can be used. */

					if ( freqClassComplete != freqClass)
					{
						/*This is a path that cannot end up in a complete word, within this frequency class */

						/*Reduce boost for incomplete words. Tuned for best balance. */
						memberBoost = memberBoost* 120 / 128;
						if (freqClassComplete != MAX_DECUMA_UINT8)
						{
							/*The if statement was actually needed e.g. when rgMinMax are {3,5} and */
							/*the dictionary subtree has min max={1,8} but no strings end in the interval {3,5} */

							/*See what we would get if choosing the complete path instead, even if it has worse freqclass */
							DECUMA_UINT32 memberBoostComplete = pRG->pDictionaryData[i].pFreqClassBoost[freqClassComplete];

							decumaAssert(freqClassComplete > freqClass);
							
							/*Select the best value */
							if (memberBoostComplete>memberBoost) memberBoost = memberBoostComplete;
						}						
					}
				}
			}

			if (pEntry->bPunishableAlternativeSymbolExpansion) memberBoost = memberBoost * (128 - PUNISHABLE_ALTERNATIVE_SYMBOL_EXPANSION_PUNISH128) / 128;

			if (nWordEnd == nStringUnicodes - 3 &&
				isExpectedWordTrailer(pStringUnicodes[nWordEnd+1]) &&
				!isExpectedWordTrailerTrailer(pStringUnicodes[nWordEnd+1]) &&
				isExpectedWordTrailerTrailer(pStringUnicodes[nWordEnd+2])) nStringUnicodes -= 2;

			if (nWordEnd == nStringUnicodes - 2 && isExpectedWordTrailer(pStringUnicodes[nWordEnd+1])) nStringUnicodes--;

			if (STR_HAS_CONTRACTED(pEntry))
				memberBoost = memberBoost * pEntry->nLeftBoost / pRG->nMaxRankBoost1024;

			memberBoost = memberBoost * pEntry->nWordLen / (ASL_MAX(1, nStringUnicodes) + nIgnoredStringStartLen);

			if (memberBoost > rankBoost) 
			{
				*pBestDictEntryIdx=i;
				rankBoost = memberBoost;
			}
		}

	return (DECUMA_UINT16) rankBoost;
	}
}

static DECUMA_UINT32 getLingDistFactor128(DECUMA_UINT32 nRankBoost1024, DECUMA_UINT32 nNodeProgressionSqr1024)
{
	/* Comments: Higher boost factor in early nodes than in late seems to be best solution. The following */
	/* cases were tested */
	/* */
	/* 1. Words exists in dictionary */
	/* 2. All but last letter of word exists in dictionary (punished in late node) */
	/* 3. First letter of word does not exist in dictionary (punished from first node) */
	/* */
	/* This was compared to both node independent boost and higher boost in late nodes than earlier nodes. */
	/* */
	/* The conclusion is that early high punish is beneficial for the first two cases (the more frequently */
	/* expected of the 3 cases) and bad for the 3rd case. Early low punish has the opposite effect, the */
	/* third case is not bad and the benefit for case 1 and 2 are reduced. The optimal function for case 1 */
	/* and 2 has been implemented below. */

	/* linguisticDistFactor128 = 128 * (1 - boost * (0.177 + 0.0527 * (1 - (nodeIdx / nNodesInSG)^2))) */

	/* NOTE: New boost function. Do not boost most in the start, boost most in the ends. */
	if (nNodeProgressionSqr1024 < 512)
		return (1024 - nRankBoost1024 * (1024 + 306 * (512 - nNodeProgressionSqr1024) / 1024) / 1024) / 8;
	else
		return (1024 - nRankBoost1024 * (1024 + 306 * (nNodeProgressionSqr1024 - 512) / 1024) / 1024) / 8;
}

static void freeDictionaryList(ASL_RG * pRG, ASL_RG_DICTIONARY_ENTRY * pList)
{
	MemoryPoolHandler_Free(pRG->pDictMemPoolHandler, pList);
}

#ifdef _DEBUG_EXTRA

static int restCannotContainSymbols(const ASL_RG_NODE * pRgStartNode, const ASL_RG_NODE * pRgNode,
									int thisEdgeClass)
{
	if (pRgStartNode->pcdDiacEdge.nMaxRemUnicodes == 0 && 
		thisEdgeClass == ASL_PROTOTYPE_DB_CLASS_DIACRITIC)
	{
		return 1;
	}

	if (pRgStartNode->pcdNonDiacEdge.nMaxRemUnicodes == 0 && 
		pRgStartNode->pcdNonDiacViaRGEdge.nMaxRemUnicodes == 0 &&
		thisEdgeClass != ASL_PROTOTYPE_DB_CLASS_DIACRITIC)
	{
		return 1;
	}

	if ((pRgNode->pcdNonDiacViaRGEdge.nMaxRemUnicodes == 0 || pRgNode->pcdNonDiacViaRGEdge.nMaxRemUnicodes == MIN_DECUMA_INT16) &&
		(pRgNode->pcdNonDiacEdge.nMaxRemUnicodes == 0 || pRgNode->pcdNonDiacEdge.nMaxRemUnicodes == MIN_DECUMA_INT16) &&
		(pRgNode->pcdDiacEdge.nMaxRemUnicodes == 0 || pRgNode->pcdDiacEdge.nMaxRemUnicodes == MIN_DECUMA_INT16))
	{
		return 1;
	}
	return 0;
}
#endif /*#endif //_DEBUG_EXTRA */



static void preRemovalFunc(ASL_RG * pRG, ASL_RG_STRING * pStr)
{
	/*Before removing candidates from the candidate handler. We need to free */
	/*the allocated data */
	if ( pStr->bListOwner )
	{
		freeDictionaryList(pRG, pStr->pDictionaryEntries);
	}

	if (pStr->pStringUnicodes && pStr->nStringUnicodesAllocFlag == UNICODES_ARE_ALLOCATED) 
	{
		MemoryPoolHandler_Free(pRG->pStringMemPoolHandler, pStr->pStringUnicodes); 
		pStr->pStringUnicodes=NULL;
		pStr->nStringUnicodesAllocFlag = 0;
	}
}

static void preInsertFunc(ASL_RG * pRG, ASL_RG_STRING * pStr)
{
	/*Allocate for the string-unicodes that were just temprarily stored  */
	if (pStr->nStringUnicodesAllocFlag==UNICODES_NEED_ALLOCATION)
	{
		DECUMA_UNICODE * pNewStringUnicodes=NULL;

		/*Allocate from the internal memory pool */
		/*Allocate from the latest created memory pool, since we assume that the other ones are full. (even if there might have been free-ups...) */
		/*TODO: We can optimize this, e.g. by destroying the memory pool that is filled (but keeping the allocated memory) and */
		/*      Re-use the overhead. This means we only have one active memory pool... */

		/*Allocate extra bytes more than needed, to have room for temporary extensions when RG processes the next node */
		int nBytesToAlloc = (pStr->nStringUnicodes+1)*sizeof(pStr->pStringUnicodes[0]);
		
		pNewStringUnicodes = MEMORYPOOLHANDLER_ALLOC(pRG->pStringMemPoolHandler, nBytesToAlloc ,1); 
		if (!pNewStringUnicodes) 
		{
			/*The memory pool was full. This can happen, but should not happen often. */
			/*Allocate another one. */
			DECUMA_STATUS status;
			
			status = allocNewStringUnicodesMemPool(pRG, pStr->nStringUnicodes, maxUnicodesInRGMacro(pRG)); /* NOTE that nStringStartLen is included in maxUnicodesInRGMacro */
			if (status==decumaNoError)
			{
				pNewStringUnicodes = MEMORYPOOLHANDLER_ALLOC(pRG->pStringMemPoolHandler, nBytesToAlloc,1);
			}
		}
		
		if (!pNewStringUnicodes) 
		{
			/*We are probably out of memory */
			pStr->nStringUnicodesAllocFlag = UNICODES_ALLOCATION_TRIED_WITH_ERROR;
			pStr->nStringUnicodes = 0;
			pStr->pStringUnicodes = NULL;
			return;
		}
		
		decumaMemcpy(pNewStringUnicodes,pStr->pStringUnicodes,
			pStr->nStringUnicodes*sizeof(pStr->pStringUnicodes[0]));
		pStr->pStringUnicodes = pNewStringUnicodes;
		pStr->nStringUnicodesAllocFlag = UNICODES_ARE_ALLOCATED;
	}
}


static DECUMA_STATUS preProcessRG(ASL_RG * pRG)
{
	int nodeIdx;
	int nNodesInSG = aslSGGetNodeCount(pRG->pSG);
	DECUMA_STATUS status = decumaNoError;

	if (status!=decumaNoError) goto preProcessRG_error;

	/* Create an edge corresponding to the string start */

	status = aslSGSetStringStart(pRG->pSG, pRG->pStringStart, pRG->nStringStartLen);

	if (status!=decumaNoError) goto preProcessRG_error;

	/*---------- Calculate the maximum characters from each node forward ---------- // */
	/* */
	/*Also set the value of nMaxUnicodesInNode */

	for (nodeIdx=0; nodeIdx<nNodesInSG; nodeIdx++)
	{
		const ASL_SG_NODE * pSgNode = aslSGGetNode(pRG->pSG, nodeIdx);
		ASL_RG_NODE * pRgNode = pRG->pNodes[nodeIdx];
		int e, nEdges = aslSGGetEdgeCount(pSgNode);

		pRgNode->pcdEdge.nMaxRemUnicodes = MIN_DECUMA_INT16;

		pRgNode->pcdEdge.nMinRemUnicodes = MAX_DECUMA_INT16;

		pRgNode->pcdEdge.nMaxRemLetters = MIN_DECUMA_INT16;

		pRgNode->pcdEdge.nMinRemLetters = MAX_DECUMA_INT16;

		for (e = 0; e < nEdges; e++)
		{
			const DECUMA_UNICODE* pSymbol = aslSGGetSymbol(aslSGGetEdge(pSgNode, e));
			int nUnicodes = 0;
			int nLetters = 0;

			while (pSymbol[nUnicodes])
			{
				if (decumaUnicodeIsLetter(pSymbol[nUnicodes])) nLetters++;
				nUnicodes++;
			}

			if (nUnicodes > pRgNode->nMaxUnicodesInNode) pRgNode->nMaxUnicodesInNode = nUnicodes;
			if (nLetters > pRgNode->nMaxLettersInNode) pRgNode->nMaxLettersInNode = nLetters;
		}
	}
	
	/* --- ALGORITHM --- */
	/* Start from the last node  */
	/* Set all the minimum and maximum remaining characters to be 0 */
	/* Walk backwards iterating throuch the nodes and evaluate all the  */
	/* edges in each node to find new minimum and maximums from that node forward */
	
	decumaMemset(&pRG->pNodes[nNodesInSG-1]->pcdEdge,0,sizeof(NODE_PRECALC_DATA));

	for (nodeIdx=nNodesInSG-1; nodeIdx>=0; nodeIdx--)
	{
		const ASL_SG_NODE * pSgNode = aslSGGetNode(pRG->pSG, nodeIdx);
		ASL_RG_NODE * pRgNode = pRG->pNodes[nodeIdx];
		int edgeIdx;
		int nEdges=0;

		if (pRgNode->pcdEdge.nMaxRemUnicodes < 0)
		{
			/*There is no use investigating this node, since there is no valid path from this node forward */
			continue;
		}

		nEdges = aslSGGetEdgeCount(pSgNode);

		for (edgeIdx=0; edgeIdx<nEdges; edgeIdx++)
		{
			const ASL_SG_EDGE * pEdge;
			ASL_RG_NODE *pEdgeStartRgNode;
			int startNode;
			DECUMA_UINT8 nUnicodes = 0;
			DECUMA_UINT8 nLetters = 0;
			const DECUMA_UNICODE* thisEdgeSymbol;

			DECUMA_UINT8 nMaxNewUnicodes = 0,nMinNewUnicodes=0;
			DECUMA_UINT8 nMaxNewLetters = 0,nMinNewLetters=0;

			pEdge = aslSGGetEdge(pSgNode, edgeIdx);
			decumaAssert(pEdge);


			startNode = aslSGGetStartNode(pEdge);

			nMaxNewUnicodes = 0;
			nMinNewUnicodes = pRgNode->nMaxUnicodesInNode;
			nMaxNewLetters = 0;
			nMinNewLetters = pRgNode->nMaxLettersInNode;

			thisEdgeSymbol = aslSGGetSymbol(pEdge);

			while(thisEdgeSymbol && thisEdgeSymbol[nUnicodes])
			{
				if (decumaUnicodeIsLetter(thisEdgeSymbol[nUnicodes])) nLetters++;
				nUnicodes++;
			}

			nMaxNewUnicodes = ASL_MAX(nMaxNewUnicodes, nUnicodes);
			nMinNewUnicodes = ASL_MIN(nMinNewUnicodes, nUnicodes);

			/*we don't want to exclude these unicodes from actually being part of a dictionary word. */
			/*At least for now. Therefore we increase the maximum number of letters to maximum number */
			/*of unicodes. */
			/*TODO: We can be more detailed here if classifying letters better in the future							 */
			nMaxNewLetters = ASL_MAX(nMaxNewLetters, nUnicodes);
			nMinNewLetters = ASL_MIN(nMinNewLetters, nLetters);

			decumaAssert(nMaxNewUnicodes>=nMinNewUnicodes);
			decumaAssert(nMaxNewLetters>=nMinNewLetters);

			pEdgeStartRgNode = pRG->pNodes[startNode];

			/*Two nondiacs connecting with sg-penlifts or without any penlifts */
			updatePrecalcMinMaxValues(
				&pEdgeStartRgNode->pcdEdge,
				&pRgNode->pcdEdge, 
				nMinNewUnicodes, nMaxNewUnicodes,
				nMinNewLetters, nMaxNewLetters);
		}
	}

#ifdef _DEBUG
	{
		DECUMA_INT32 nMaxUnicodes = maxUnicodesInRGMacro(pRG);
		DECUMA_INT32 nMaxLetters = maxLettersInRGMacro(pRG);

		if (nMaxUnicodes > MIN_DECUMA_INT16)
		{
			for (nodeIdx=0; nodeIdx<nNodesInSG; nodeIdx++)
			{
				ASL_RG_NODE * pRgNode = pRG->pNodes[nodeIdx];

				decumaAssert(VALID_PRECALCDATA(&pRgNode->pcdEdge,nMaxUnicodes, nMaxLetters));
			}
		}
	}
#endif

	return decumaNoError;

preProcessRG_error:

	return decumaAllocationFailed;
}

static DECUMA_STATUS allocAndAddMemPool(const ASL_RG * pRG, TMemoryPoolHandler * pMemPoolHandler, TMemoryPoolLayout * pMemPoolLayouts, int nMemPoolLayouts, 
										TMemoryPool *** pppMemPools, int *pnMemPools)
{
	const DECUMA_MEM_FUNCTIONS* pMemFunctions = pRG->pMemFunctions;
	int error;
	int memSize;
	TMemoryPool * pMemPool=NULL;
	TMemoryPool ** ppMemPoolsNew=NULL;
	
	TMemoryPool ** ppMemPoolsOld = *pppMemPools;

	decumaAssert(pppMemPools);

	/*Allocate a new space that can store the old and the new memory pool pointers */
	ppMemPoolsNew = aslCalloc((*pnMemPools+1)*sizeof(ppMemPoolsOld[0]));
	if (!ppMemPoolsNew)	goto allocAndAddMemPool_error;
	
	if (*pnMemPools)
	{
		/*Copy the old memory pool pointers to the new space */
		decumaMemcpy( ppMemPoolsNew, ppMemPoolsOld,(*pnMemPools)*sizeof(ppMemPoolsOld[0]));
	}

	/*Create a new memory pool */
	memSize = MemoryPool_GetSize(pMemPoolLayouts, nMemPoolLayouts);
	pMemPool = aslCalloc(memSize);
	if (!pMemPool)	goto allocAndAddMemPool_error;

	MemoryPool_Init( pMemPool, pMemPoolLayouts, nMemPoolLayouts);
	
	error = MemoryPoolHandler_AddPool( pMemPoolHandler, pMemPool, pRG->pMemFunctions);
	if (error) goto allocAndAddMemPool_error;

	ppMemPoolsNew[(*pnMemPools)++] = pMemPool;

	if (*pppMemPools) aslFree(*pppMemPools);
	*pppMemPools = ppMemPoolsNew;

	return decumaNoError;

allocAndAddMemPool_error:

	if (ppMemPoolsNew) aslFree(ppMemPoolsNew);
	if (pMemPool) aslFree(pMemPool);

	return decumaAllocationFailed;

}

static DECUMA_STATUS allocNewDictMemPool(ASL_RG * pRG)
{
	/* ------- Calculate the layout of the StringUnicodes memory pool and allocate ---- */
	TMemoryPoolLayout memPoolLayout;
	int i;

	if (pRG->boostLevel == noBoost) return decumaNoError; /*No need to allocate anything */

	for (i=1; i<=pRG->nAllDictionaries; i++)
	{
		/*JM:Try an even distribution */
		memPoolLayout.size = sizeof(ASL_RG_DICTIONARY_ENTRY)*i;
		memPoolLayout.nElements  = DICT_MEM_POOL_SIZE / memPoolLayout.size / pRG->nAllDictionaries;
	}

	return allocAndAddMemPool(pRG, pRG->pDictMemPoolHandler,&memPoolLayout,1,
		&pRG->ppDictMemPools, &pRG->nDictMemPools);
}




static DECUMA_STATUS allocNewStringUnicodesMemPool(ASL_RG * pRG, int minStrLen, int maxStrLen)
{
	/* ------- Calculate the layout of the StringUnicodes memory pool and allocate ---- */
	const DECUMA_MEM_FUNCTIONS* pMemFunctions = pRG->pMemFunctions;
	int nMaxStringUnicodes = maxUnicodesInRGMacro(pRG);  /*This is the length of the longest possible charstring  */
	int i, nLayouts;
	TMemoryPoolLayout * pStringMemPoolLayout=NULL;
	DECUMA_STATUS status = decumaNoError;

	decumaAssert(minStrLen<=maxStrLen);
	decumaAssert(maxStrLen<=nMaxStringUnicodes);


	nLayouts = (maxStrLen-minStrLen+1);
	pStringMemPoolLayout = aslCalloc(nLayouts*sizeof(TMemoryPoolLayout));
	if (!pStringMemPoolLayout)	goto allocNewStringUnicodesMemPool_error;

	for (i=minStrLen; i<=maxStrLen; i++)
	{	
		/*Portion the memory slots out like a pyramid since we expect a lot of more  */
		/*strings with small lengths than the ones with large length */
		/*Try setting the number of elements as 4*MAX_STRINGS_PER_NODE for i=1 */
		/*and falling exponentially to MAX_STRINGS_PER_NODE/2 for i=nMaxStringUnicodes */
		int exponent = 4-((5*i)/nMaxStringUnicodes);
		pStringMemPoolLayout[i-minStrLen].nElements = (exponent<0 ? MAX_STRINGS_PER_NODE >> (-exponent) : MAX_STRINGS_PER_NODE << exponent); /*Pyramid-like... */
		pStringMemPoolLayout[i-minStrLen].size = sizeof(DECUMA_UNICODE)*(i+1); /*We add room to guarantee space for one extra symbol */
	}

	status = allocAndAddMemPool(pRG, pRG->pStringMemPoolHandler, pStringMemPoolLayout, nLayouts,
		&pRG->ppStringMemPools, &pRG->nStringMemPools);

	if (status != decumaNoError) goto allocNewStringUnicodesMemPool_error;

	aslFree(pStringMemPoolLayout);

	return decumaNoError;

allocNewStringUnicodesMemPool_error:

	if (pStringMemPoolLayout) aslFree(pStringMemPoolLayout);

	return status;
}



static int rgStringGetStartNode(const ASL_RG * pRG, const ASL_RG_STRING * pRgString, int nodeIdx)
{
	/*Get the start node of the provided string. */

	int nStartNode;

	const ASL_SG_NODE * pNode = aslSGGetNode(pRG->pSG,nodeIdx);

	/*DON'T CALL initEdgeDescriptor here since that will cause a cricular dependency through */
	/*the diacritic path handler!! */
/*	if (pRgString->lastEdgeBuffer!=DPH_DIAC_EDGE_BUFFER) */
	{
		const ASL_SG_EDGE * pLastEdge;

		decumaAssert(aslSGGetEdgeCount(pNode));
		pLastEdge = aslSGGetEdge(pNode,pRgString->lastEdgeIdx);
		/*Normal edge */
		nStartNode = aslSGGetStartNode(pLastEdge);

	}

	return nStartNode;
}

static DECUMA_STRING_TYPE rgStringGetType(const ASL_RG * pRG, const ASL_RG_STRING* pStr,
										  DECUMA_INT16 * pStartIdx, DECUMA_INT16 * pLen)
{
	int i;
	ASL_RG_DICTIONARY_ENTRY * pEntry;

	decumaAssert(pStr);
	decumaAssert(pStartIdx);
	decumaAssert(pLen);

	if (pStr->nDictionaryEntries == 0)	
	{
		*pStartIdx = 0;
		*pLen = 0;
		return notFromDictionary;
	}

	/*Return complete word if that exists (even if it does not agree with the bestDictEntryIdx) (??) */
	for ( i = 0; i < pStr->nDictionaryEntries; i++ )
	{
		pEntry = &pStr->pDictionaryEntries[i];

		if (!pEntry->pReference) break; /*startOfWord */

		if ( decumaDictionaryGetEndOfWord(pRG->pAllDictionaries[pEntry->dictIdx], pEntry->pReference) )
		{
			GET_DICTWORD_START_AND_LEN(pEntry,pStartIdx,pLen);
			decumaAssert(*pStartIdx + *pLen <= pStr->nStringUnicodes);
			return completeWord; /* Return directly, since "completeWord" is the "best" result we can get */
		}
	}

	decumaAssert(pStr->bestDictEntryIdx>=0);
	
	pEntry = &pStr->pDictionaryEntries[pStr->bestDictEntryIdx];

	GET_DICTWORD_START_AND_LEN(pEntry,pStartIdx,pLen);
	decumaAssert(*pStartIdx + *pLen <= pStr->nStringUnicodes);

	return startOfWord;
}

static void rgStringSetStart(ASL_RG * pRG, ASL_RG_STRING * pRgString, int nodeIdx)
{
	/*Get the start string of the provided string. Note that the start string */
	/*can be two edges away if there is an intermediate penlift. */

	DECUMA_INT16 nStartNode;
	ASL_RG_NODE * pRgStartNode;
	const ASL_SG_NODE * pNode = aslSGGetNode(pRG->pSG,nodeIdx);

	if (pRgString->nStartNode >= 0) return;

	nStartNode = rgStringGetStartNode(pRG, pRgString, nodeIdx);

	pRgString->nStartNode=nStartNode;

	decumaAssert(nStartNode>=0 && nStartNode<pRG->nNodes);

	pRgStartNode=pRG->pNodes[nStartNode];

	if (!pRgStartNode->nStrings) 
	{
		pRgString->pStartStr = NULL;
		return;
	}

	decumaAssert(pRgString->prevStringIdx>=0 && pRgString->prevStringIdx<pRgStartNode->nStrings);

	pRgString->pStartStr = &pRgStartNode->pStrings[pRgString->prevStringIdx];
}

static ASL_RG_NODE* rgAddNode(ASL_RG* pRG)
{
	const DECUMA_MEM_FUNCTIONS* pMemFunctions = pRG->pMemFunctions;
	ASL_RG_NODE** ppNodes;

	int nNewNodeIdx;

	decumaAssert(pRG);

	nNewNodeIdx = pRG->nAddedNodes;

	ppNodes = aslCalloc((pRG->nAddedNodes + 1) * sizeof(pRG->pNodes[0]));

	if (!ppNodes) return NULL;

	if (pRG->pNodes)
	{
		decumaMemcpy(ppNodes, pRG->pNodes, pRG->nAddedNodes * sizeof(pRG->pNodes[0]));
		aslFree(pRG->pNodes);
	}
	else
	{
		decumaAssert(nNewNodeIdx == 0);
	}
	pRG->pNodes = ppNodes;

	pRG->pNodes[nNewNodeIdx] = aslCalloc(sizeof(pRG->pNodes[nNewNodeIdx][0]));
	
	if (pRG->pNodes[nNewNodeIdx]) pRG->nAddedNodes++;

	return pRG->pNodes[nNewNodeIdx];
}

static void rgRemoveNode(ASL_RG* pRG)
{
	const DECUMA_MEM_FUNCTIONS* pMemFunctions = pRG->pMemFunctions;
	int i;

	decumaAssert(pRG);
	decumaAssert(pRG->nAddedNodes > 0);
	decumaAssert(pRG->pNodes[pRG->nAddedNodes-1]);

	for (i=0; i<pRG->pNodes[pRG->nAddedNodes-1]->nStrings; i++)
	{
		ASL_RG_STRING * pStr = &pRG->pNodes[pRG->nAddedNodes-1]->pStrings[i];
		preRemovalFunc(pRG, pStr);
	}	

	while (pRG->pNodes[pRG->nAddedNodes-1]->nStringLists > 0) rgNodeRemoveStringList(pRG, pRG->pNodes[pRG->nAddedNodes-1]);

	aslFree(pRG->pNodes[pRG->nAddedNodes-1]);

	pRG->nAddedNodes--;
}

static ASL_RG_STRING* rgNodeAddStringList(const ASL_RG * pRG, ASL_RG_NODE* pNode, ASL_RG_STRING* pStringList, int nListLen)
{
	const DECUMA_MEM_FUNCTIONS* pMemFunctions = pRG->pMemFunctions;
	decumaAssert(pNode);
	decumaAssert(nListLen <= MAX_STRINGS_PER_NODE);

	if (pNode->nStringLists != 0) return NULL; /* Node is full */

	decumaAssert(pNode->pStrings == 0);

	if (pStringList)
	{
		/* Pre-allocated string list provided */
		pNode->pStrings = pStringList;
	}
	else
	{
		/* String list not provided. Allocate nListLen long list. */

		pNode->pStrings = aslCalloc(nListLen * sizeof(pNode->pStrings[0]));
	}

	if (!pNode->pStrings) return NULL; /* Add failed */

	pNode->nStringLists++;

	return pNode->pStrings;
}

static void rgNodeRemoveStringList(const ASL_RG * pRG, ASL_RG_NODE* pNode)
{
	const DECUMA_MEM_FUNCTIONS* pMemFunctions = pRG->pMemFunctions;
	decumaAssert(pNode);
	decumaAssert(pNode->nStringLists == 1);

	aslFree(pNode->pStrings);

	pNode->nStringLists--;
}

static ASL_RG_SCRATCHPAD* rgAddScratchPad(ASL_RG* pRG)
{
	const DECUMA_MEM_FUNCTIONS* pMemFunctions = pRG->pMemFunctions;

	decumaAssert(pRG);

	if (pRG->nScratchPads != 0) return NULL; /* RG is full */

	decumaAssert(pRG->pScratchPad == 0);

	pRG->pScratchPad = aslCalloc(sizeof(pRG->pScratchPad[0]));

	if (!pRG->pScratchPad) return NULL; /* Add failed */

	pRG->nScratchPads++;

	return pRG->pScratchPad;
}

static void rgRemoveScratchPad(ASL_RG* pRG)
{
	const DECUMA_MEM_FUNCTIONS* pMemFunctions = pRG->pMemFunctions;
	decumaAssert(pRG);
	decumaAssert(pRG->nScratchPads == 1);

	if (pRG->pScratchPad->pOldStrings)
	{
		int i;

		for (i = 0; i < pRG->pScratchPad->nOldStrings; i++) preRemovalFunc(pRG, &pRG->pScratchPad->pOldStrings[i]);

		aslFree(pRG->pScratchPad->pOldStrings);
	}

	if (pRG->pScratchPad->pTmpStrings)
	{
		int i;

		for (i = 0; i < pRG->pScratchPad->nTmpStrings; i++) preRemovalFunc(pRG, &pRG->pScratchPad->pTmpStrings[i]);

		aslFree(pRG->pScratchPad->pTmpStrings);
	}

	aslFree(pRG->pScratchPad);

	pRG->nScratchPads--;
}

static void initString(ASL_RG_STRING * pStr)
{
	/* Do not memset whole string to 0. Too time-consuming. /JA */

	pStr->pDictionaryEntries = NULL;
	pStr->nDictionaryEntries = 0;
	pStr->bListOwner = 0;

	pStr->pStringUnicodes = NULL;
	pStr->nStringUnicodes = 0;
	pStr->nStringUnicodesAllocFlag = 0;
	pStr->nLenOfForced = 0;

#ifdef EXTRA_VERIFICATION_INFO
	pStr->lastEdgeDist = 0;
	pStr->connectionDist = 0;
#endif
}

static int getMaxDictWordLen(const ASL_RG * pRG)
{
	int i;
	DECUMA_UINT8 maxMax = 0;

	for ( i = 0; i < pRG->nAllDictionaries; i++ )
	{
		DECUMA_UINT8 min, max;
		decumaDictionaryGetMinMax(pRG->pAllDictionaries[i], NULL, &min, &max);
		
		if ( max > maxMax )
			maxMax = max;
	}

	return maxMax; 
}

#ifdef _DEBUG
static void debugInitString(ASL_RG_STRING * pStr)
{
	initString(pStr);

	/*Where it is incorrect to have default values, set invalid values */

	pStr->lastEdgeIdx=MAX_DECUMA_UINT8; /* No default value. Set to invalid value. Must be set at a later stage */
	pStr->prevStringIdx=-1; /* Default is no previous string == -1 */
		

	/* pStr->pEziNode - is default 0 */
	/* pStr->dist - is default 0 */
	/* pStr->symbol - is default 0 */
	/* pStr->StringUnicodes[]; - is default 0 */
	/* pStr->nStringUnicodes; - is default 0 */

	/* pStr->nClass; - is default 0 == INVALID. Must be set at a later stage */

	/* Connection and feature masks are default 0 */
	
	/* pStr->bEziNodeOwner - is default 0 */
	/* pStr->nLenOfForced - is default 0 */
	/* pStr->bStringIsPartOfRefBestPath - is default 0 */
}
#endif /*_DEBUG */



