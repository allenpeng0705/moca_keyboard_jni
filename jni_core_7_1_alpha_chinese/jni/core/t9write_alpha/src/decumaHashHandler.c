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

  This file implements decumaHashHandler.h.

\******************************************************************/

#define DECUMA_HASH_HANDLER_C

#include "decumaHashHandler.h"
#include "decumaHashHandlerMacros.h"
#include "decumaBasicTypes.h"
#include "decumaAssert.h"
#include "decumaMemory.h"
#include "decumaHashHandlerData.h"
#include "decumaConfig.h"


/* */
/* Private macro definitions */
/* */

/* The hash functions have been designed with the assumption that hash calculation */
/* speed is more important than low collision probability. */

#define dhhHashMacro(tableSize,nKey) \
( \
	(tableSize) == decumaHHSizeLarge ? \
		((nKey) & 0xFFFF) + \
		((nKey) >> 16 & 0xFFFF) \
	:((tableSize) == decumaHHSizeMedium ? \
		((nKey) & 0xFFF) + \
		((nKey) >> 12 & 0xFFF) + \
		((nKey) >> 24 & 0xFF) \
	: \
		((nKey) & 0xFF) + \
		((nKey) >> 8 & 0xFF) + \
		((nKey) >> 16 & 0xFF) + \
		((nKey) >> 24 & 0xFF)) \
)

/* Note that an assumtion about the hash function is used */
/* below (max key value results in max hash value). Beware. */
#define dhhHashTableLenMacro(tableSize)  (dhhHashMacro((tableSize),~0) + 1)

/* elemRef is both input and output */
#define dhhFindKeyMacro(elemRef, _nKey) while ((elemRef) && (elemRef)->nKey != (_nKey)) (elemRef) = (elemRef)->pNext

/* elemRef is both input and output */
#define dhhFindDataMacro(elemRef, _pData) while ((elemRef) && (elemRef)->pData != (_pData)) (elemRef) = (elemRef)->pNext



/* */
/* Private function definitions */
/* */

static void inserElemBufferToFreeList(DECUMA_HH * pHH, DECUMA_HH_ELEM * pElemBuf, int nElems);


/* */
/* Public function definitions */
/* */



int decumaHHGetSize(unsigned int nStartElems, DECUMA_HH_TABLESIZE tableSize)
{
	return sizeof(DECUMA_HH) + dhhHashTableLenMacro(tableSize) * sizeof(DECUMA_HH_ELEM*) + 
		nStartElems * sizeof(DECUMA_HH_ELEM);
}

int decumaHHGetExtraElemBufferSize(unsigned int nElems)
{
	return nElems * sizeof(DECUMA_HH_ELEM);
}

void * decumaHHAttachExtraElemBuffer(DECUMA_HH* pHH, void * pBuf, unsigned int nElems)
{
	DECUMA_HH_ELEM* pElemBuf  = (DECUMA_HH_ELEM*)pBuf;

	if (pHH->nExtraElemBufs >= DECUMA_HH_MAX_EXTRA_ELEM_BUFS)
		return NULL;

	pHH->pExtraElemBufs[pHH->nExtraElemBufs]=pElemBuf;
	pHH->nExtraElemBufs++;
	inserElemBufferToFreeList(pHH,pElemBuf,nElems);
	return pElemBuf;
}

int decumaHHGetNExtraElemBuffers(DECUMA_HH* pHH)
{
	return pHH->nExtraElemBufs;
}

void * decumaHHGetExtraElemBuffer(DECUMA_HH* pHH,int nBufIdx)
{
	decumaAssert(nBufIdx<pHH->nExtraElemBufs);

	return pHH->pExtraElemBufs[nBufIdx];
}


void decumaHHInit(DECUMA_HH* pHH, unsigned int nStartElems, DECUMA_HH_TABLESIZE tableSize)
{
	unsigned nTableLen;

	decumaAssert(pHH);
	decumaAssert(nStartElems > 0);

	pHH->ppTable = (DECUMA_HH_ELEM**)&pHH[1];
	pHH->tableSize = tableSize;
	pHH->nExtraElemBufs = 0;

	nTableLen = dhhHashTableLenMacro(tableSize);

	decumaMemset(pHH->ppTable, 0, (char*)&pHH->ppTable[nTableLen] - (char*)&pHH->ppTable[0]);

	pHH->pFreeElem = NULL; /*Will be initialized below */
	inserElemBufferToFreeList(pHH,(DECUMA_HH_ELEM*)&pHH->ppTable[nTableLen],nStartElems);
}

DECUMA_HH_ELEMREF decumaHHAdd(DECUMA_HH* pHH, DECUMA_UINT32 nKey, const void* pData)
{
	DECUMA_HH_ELEM* pAddee;
	unsigned int nTableInd;

	decumaAssert(pHH);

	if (!pHH->pFreeElem) return NULL; /* No free slot => fail */

	/* Allocate free elem for addee */
	pAddee = pHH->pFreeElem;
	pHH->pFreeElem = pHH->pFreeElem->pNext; /* Remove allocated elem from free elem list */

	/* Init addee */
	pAddee->nKey = nKey;
	pAddee->pData = pData;
	
	/* Insert addee as first bucket */
	nTableInd = dhhHashMacro(pHH->tableSize,nKey);
	pAddee->pNext = pHH->ppTable[nTableInd];
	pHH->ppTable[nTableInd] = pAddee;

	return pHH->ppTable[nTableInd];
}

void decumaHHRemove(DECUMA_HH* pHH, DECUMA_HH_ELEMREF elemRef)
{
	DECUMA_HH_ELEM** ppRemovee;
	DECUMA_HH_ELEM*  pRemoveeSucc;

	decumaAssert(pHH);
	decumaAssert(elemRef);

	ppRemovee = &pHH->ppTable[dhhHashMacro(pHH->tableSize,elemRef->nKey)];

	/* Search among the buckets */
	while (*ppRemovee && (*ppRemovee)->pData != elemRef->pData) ppRemovee = &(*ppRemovee)->pNext; /* Check next bucket */

	decumaAssert(*ppRemovee);

	pRemoveeSucc = (*ppRemovee)->pNext; /* Remember next after removee */
	(*ppRemovee)->pNext = pHH->pFreeElem; /* Removee joins the free elem list */
	pHH->pFreeElem = *ppRemovee; /* -"- */
	*ppRemovee = pRemoveeSucc; /* Remove removee */
}

DECUMA_HH_ELEMREF decumaHHFindKey(const DECUMA_HH* pHH, DECUMA_UINT32 nKey)
{
	DECUMA_HH_ELEMREF elemRef;

	decumaAssert(pHH);

	elemRef = pHH->ppTable[dhhHashMacro(pHH->tableSize,nKey)];

	dhhFindKeyMacro(elemRef, nKey);

	return elemRef;
}

DECUMA_HH_ELEMREF decumaHHFindNextKey(DECUMA_HH_ELEMREF elemRef)
{
	DECUMA_UINT32 nKey;
	decumaAssert(elemRef);

	nKey = elemRef->nKey;
	elemRef = elemRef->pNext;

	dhhFindKeyMacro(elemRef, nKey);

	return elemRef;
}

DECUMA_HH_ELEMREF decumaHHFindData(const DECUMA_HH* pHH, DECUMA_UINT32 nKey, const void* pData)
{
	DECUMA_HH_ELEMREF elemRef;

	decumaAssert(pHH);

	elemRef = pHH->ppTable[dhhHashMacro(pHH->tableSize,nKey)];

	dhhFindDataMacro(elemRef, pData);

	return elemRef;
}

int decumaHHIteratorGetSize(void)
{
	return sizeof(DECUMA_HH_ITERATOR);
}


void decumaHHIteratorInit(DECUMA_HH_ITERATOR * pHHIterator,const DECUMA_HH* pHH)
{
	decumaAssert(pHHIterator);
	decumaAssert(pHH);
	pHHIterator->pHH = pHH;
	pHHIterator->nTableLen = dhhHashTableLenMacro(pHH->tableSize);
	pHHIterator->nTableIdx=0;
	pHHIterator->elemRef = pHH->ppTable[0];
}

DECUMA_HH_ELEMREF decumaHHIteratorNext(DECUMA_HH_ITERATOR * pHHIterator)
{
	const DECUMA_HH* pHH=pHHIterator->pHH;
	DECUMA_HH_ELEMREF elemRef;
	if (!pHHIterator->elemRef)
	{
		/*Step until you find a bucket with contents */
		do 
		{
			pHHIterator->nTableIdx++;
			if (pHHIterator->nTableIdx == pHHIterator->nTableLen)
			{
				return NULL;
			}
		}while (!pHH->ppTable[pHHIterator->nTableIdx]);
		pHHIterator->elemRef = pHH->ppTable[pHHIterator->nTableIdx];
	}
	elemRef = pHHIterator->elemRef;

	/*Setup for next iteration */
	pHHIterator->elemRef = pHHIterator->elemRef->pNext;
	return elemRef;
}

/*DECUMA_HH_ELEMREF decumaHHEnumerateEntries(const DECUMA_HH* pHH, DECUMA_HH_ELEMREF pPrev)
{
	decumaAssert(pHH);
	
	if ( pPrev++ ) 
	{
		if ( pPrev == pHH->pFreeElem ) //JM: Is this correct?? We might have removed objects etc. then I think this is wrong
			return NULL;
		
		return pPrev;
	}
	else
	{
		unsigned int nTableLen = dhhHashTableLenMacro(pHH->tableSize);
		return (DECUMA_HH_ELEM *) &pHH->ppTable[nTableLen];
	}
}
*/
#ifdef _DEBUG


DECUMA_UINT32 decumaHHGetKey(DECUMA_HH_ELEMREF elemRef)
{
	decumaAssert(elemRef);

	return decumaHHGetKeyMacro(elemRef);
}

void* decumaHHGetData(DECUMA_HH_ELEMREF elemRef)
{
	decumaAssert(elemRef);

	return decumaHHGetDataMacro(elemRef);
}
#endif

#ifdef HASH_HANDLER_STATISTICS
#include <stdio.h>
void decumaHHPrintStats(const DECUMA_HH* pHH,const char * hashName)
{
	int key;

	DECUMA_HH_ELEMREF elemRef;
	int nElems=0, nMaxElems=0;
	double qSum=0.0;
	int nUsedBuckets=0, nBuckets=0;
	int hist[13]={0};
	int hist_lower[13]={0,1,2,3,5,8,12,20,30,40,50,75,100};
	int i;

	nBuckets = dhhHashTableLenMacro(pHH->tableSize);

	for (key=0;key<dhhHashTableLenMacro(pHH->tableSize); key++)
	{
		int nInBucket = 0;

		elemRef = pHH->ppTable[key];
		if (elemRef)
		{
			do
			{
				nInBucket++;
			} while ((elemRef= elemRef->pNext));
		}

		if (nInBucket > 0)
		{
			nElems += nInBucket;
			if (nInBucket > nMaxElems) 
				nMaxElems = nInBucket;
			
			qSum += (double)nInBucket*nInBucket;
			nUsedBuckets++;
		}
		/*Add to histogram */
		for (i=1; i<sizeof(hist_lower)/sizeof(hist_lower[0]); i++)
		{
			if (nInBucket<hist_lower[i])
			{
				hist[i-1]++;
				break;
			}
		}
		
	}

	printf( "HASH:%s\n",hashName);
	printf( "       nElems=%d\n",nElems);
	printf( "       nBuckets=%d\n",nBuckets);
	printf( "       nUsedBuckets=%d (=%.1f%%)\n",nUsedBuckets, 100.0*nUsedBuckets/nBuckets);
	printf( "       nAvElemsInUsedBuckets=%f nMaxElems=%d\n",nUsedBuckets==0?0:(double)nElems/nUsedBuckets,nMaxElems);
	printf( "       qSum=%f\n",qSum);
	printf( "       Hist:");
	for (i=1; i<sizeof(hist_lower)/sizeof(hist_lower[0]); i++)
	{
		if (hist_lower[i-1] == hist_lower[i]-1)
		{
			printf("%d",hist_lower[i-1]);
		}
		else
		{
			printf("%d-%d",hist_lower[i-1],hist_lower[i]-1);
		}
		printf(":%d ",hist[i-1]);		
	}
	printf("%d+:%d",hist_lower[i-1],hist[i-1]);
	printf("\n");

}
#endif /*HASH_HANDLER_STATISTICS */


/*-------------- Local functions ------------------ */
static void inserElemBufferToFreeList(DECUMA_HH * pHH, DECUMA_HH_ELEM * pElemBuf, int nElems)
{
	DECUMA_HH_ELEM* pOldFreeElems;
	int i;


	/* Link free elems list */
	pOldFreeElems = pHH->pFreeElem;

	pHH->pFreeElem = pElemBuf;

	for (i = 0; i < nElems-1; i++)
	{
		pHH->pFreeElem[i].pNext = &pHH->pFreeElem[i+1];
	}
	pHH->pFreeElem[nElems-1].pNext = pOldFreeElems;
}

#undef DECUMA_HASH_HANDLER_C
