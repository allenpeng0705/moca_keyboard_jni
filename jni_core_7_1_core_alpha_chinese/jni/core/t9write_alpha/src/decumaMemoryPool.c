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

#include "decumaConfig.h"
#include "decumaBasicTypes.h"
#include "decumaAssert.h"
#include "decumaMemory.h"
#include "decumaMemoryPool.h"
#include "decumaRuntimeMalloc.h"

#ifdef DECUMA_NO_MEMORY_POOL
/* This is an alternative implemenation that is "no pool" */

DECUMA_UINT32 MemoryPool_GetSize(TMemoryPoolLayout* pLayout, DECUMA_UINT32 nLayouts) {
	return 1;
}

DECUMA_UINT32 MemoryPool_Init(TMemoryPool* pMemoryPool, 
							  TMemoryPoolLayout* pLayout, 
							  DECUMA_UINT32 nLayouts)
{
	return 0;
}

void * MemoryPool_Alloc(TMemoryPool* pMemoryPool, DECUMA_UINT32 size, 
							  DECUMA_UINT32 bSpillOver)
{
	return decumaAlloc(size);
}

void MemoryPool_Free(TMemoryPool* pMemoryPool, void* pBase)
{
	decumaFree(pBase);
}

DECUMA_UINT32 MemoryPoolHandler_GetSize(void) 
{
	return 1;
}

void MemoryPoolHandler_Init(MODULE_ID modid, TMemoryPoolHandler* pMemoryPoolHandler) 
{
	;
}

void MemoryPoolHandler_Destroy(TMemoryPoolHandler* pMemoryPoolHandler) 
{
	;
}

DECUMA_UINT32 MemoryPoolHandler_AddPool(TMemoryPoolHandler* pMemoryPoolHandler, 
                                        TMemoryPool * pMemoryPool)
{
	return 0;
}


void * MemoryPoolHandler_Alloc(TMemoryPoolHandler* pMemoryPoolHandler, DECUMA_UINT32 size, DECUMA_UINT32 bSpillOver) 
{
	return decumaAlloc(size);
}

void MemoryPoolHandler_Free(TMemoryPoolHandler* pMemoryPoolHandler, void* pBase) 
{
	decumaFree(pBase);
}

#else

#define ALIGN_OFFSET(m_offset, m_bytes) ((m_offset) + (((m_bytes) - (m_offset) % (m_bytes)) & ((m_bytes) - 1) ))

#define FIFO_GetSize(x)      (x)->size
#define FIFO_GetNElements(x) (x)->nElements

/*#define DEBUG_OUTPUT */
#ifdef DEBUG_OUTPUT
#include <stdio.h>
extern FILE * g_debugFile;
#endif


/*////////////////////////////////// */
/* Data structures and prototypes // */
/* for the FIFO queue             // */
/*////////////////////////////////// */
typedef struct {
	DECUMA_UINT32 size;    /*The maximum number of elements */
	DECUMA_UINT32 head;
	DECUMA_UINT32 tail;
	DECUMA_UINT32 nElements; /*The actual number of elements */
	void** ppElements;
} TFIFO;

static void FIFO_Create(TFIFO* pFIFO, DECUMA_UINT32 size, void* pBase);

static DECUMA_UINT32 FIFO_Insert(TFIFO* pFIFO, void* pElem);

static void* FIFO_Extract(TFIFO* pFIFO);

typedef struct {
	void* pUpper;
	void* pLower;
} TMemoryPoolAccountRange;

#ifdef DECUMA_DEBUG_MEMORYPOOL
typedef struct _TMemoryPoolBlockDebugInfo
{
	int           bBlockIsOccupied;
	const char   *pFile;
	int           line;
} TMemoryPoolBlockDebugInfo;
#endif

typedef struct {
	DECUMA_UINT32 blockSize; /*the size in bytes of each block */
	TFIFO* pBlockQueue;    /*available blocks */
	TMemoryPoolAccountRange range;
#ifdef DECUMA_DEBUG_MEMORYPOOL
	TMemoryPoolBlockDebugInfo * pBlockDebugInfo; /*An array will be allocated the same size as nElements of pVacant. */
#endif
} TMemoryPoolAccount;

struct _TMemoryPool {
	DECUMA_UINT32       nEntries;
	TMemoryPoolLayout  *pLayout;
	TMemoryPoolAccount *pAccount;	
};

struct _TMemoryPoolHandler {
	MODULE_ID     modid;
	TMemoryPool** ppMemoryPools;
	DECUMA_UINT32 nMemoryPools;
};

#ifdef DECUMA_DEBUG_MEMORYPOOL
#define MEMORYPOOL_ALLOCATE_FROM(pAccount) MemoryPool_AllocateFrom(pAccount,pFile,line)
#else
#define MEMORYPOOL_ALLOCATE_FROM(pAccount) MemoryPool_AllocateFrom(pAccount)
#endif

static void* MemoryPool_AllocateFrom(TMemoryPoolAccount* pAccount
#ifdef DECUMA_DEBUG_MEMORYPOOL
	,const char * pFile, int line
#endif
);
static void MemoryPool_FreeFrom(TMemoryPoolAccount* pAccount, void * pBase);





/*/////////////////////////////////////// */
/* Memory pool hanldler implementation // */
/*/////////////////////////////////////// */



DECUMA_UINT32 MemoryPoolHandler_GetSize(void) {
	return sizeof(TMemoryPoolHandler);
}


void MemoryPoolHandler_Init(MODULE_ID modid, TMemoryPoolHandler* pMemoryPoolHandler) {
	decumaMemset(pMemoryPoolHandler, 0, sizeof(TMemoryPoolHandler));
	pMemoryPoolHandler->modid = modid;
}

void MemoryPoolHandler_Destroy(TMemoryPoolHandler* pMemoryPoolHandler, Uses_Runtime_Malloc) {
	if (pMemoryPoolHandler->ppMemoryPools) {
#ifdef _DEBUG
		int i;

		decumaAssert(pMemoryPoolHandler);

		/* Assert that pools are fully freed. Otherwise there might be pool leaks. */
		for (i = 0; i < pMemoryPoolHandler->nMemoryPools; i++) {
			int j;

			for (j = 0; j < pMemoryPoolHandler->ppMemoryPools[i]->nEntries; j++) {
				decumaAssert(pMemoryPoolHandler->ppMemoryPools[i]->pAccount[j].pBlockQueue->nElements == pMemoryPoolHandler->ppMemoryPools[i]->pLayout[j].nElements);
			}
		}
#endif

		decumaFree(pMemoryPoolHandler->ppMemoryPools);
	}

	decumaMemset(pMemoryPoolHandler, 0, sizeof(TMemoryPoolHandler));
}

DECUMA_UINT32 MemoryPoolHandler_AddPool(TMemoryPoolHandler* pMemoryPoolHandler, 
										TMemoryPool* pMemoryPool,
										Uses_Runtime_Malloc) {
	TMemoryPool** ppMemoryPools;
	DECUMA_UINT32 i;

	/*Extend the pointer array by one, should be a very small allocation */
	ppMemoryPools = decumaCalloc(pMemoryPoolHandler->nMemoryPools + 1, sizeof(pMemoryPoolHandler->ppMemoryPools[0]));

	if (!ppMemoryPools) {
		return 1; /*Allocation Error */
	}

	if (pMemoryPoolHandler->nMemoryPools > 0) {
		/*Insert the new pool at its correct place (sorted order according to memory position) */
		for (i = 0; i < pMemoryPoolHandler->nMemoryPools; i++) {
			if (pMemoryPoolHandler->ppMemoryPools[i] > pMemoryPool) {
				break;
			}
		}

		decumaMemcpy(ppMemoryPools, &pMemoryPoolHandler->ppMemoryPools[0], 
			sizeof(pMemoryPoolHandler->ppMemoryPools[0]) * i);
		ppMemoryPools[i] = pMemoryPool; 

		if (i < pMemoryPoolHandler->nMemoryPools) {
			decumaMemcpy(&ppMemoryPools[i + 1], &pMemoryPoolHandler->ppMemoryPools[i], 
				sizeof(pMemoryPoolHandler->ppMemoryPools[0]) * (pMemoryPoolHandler->nMemoryPools - i));
		}

		decumaFree(pMemoryPoolHandler->ppMemoryPools); 
	}
	else
	{
		ppMemoryPools[0] = pMemoryPool;
	}
	
	pMemoryPoolHandler->ppMemoryPools = ppMemoryPools;
	pMemoryPoolHandler->nMemoryPools++;

	return 0;
}


void MemoryPoolHandler_Free(TMemoryPoolHandler* pMemoryPoolHandler, void* pBase) {
	DECUMA_UINT32 j;

	decumaAssert(pBase);
	decumaAssert(pMemoryPoolHandler->nMemoryPools>0);

	/*find out which pool to free from */
	/*TODO:since the ranges are sorted a binary search */
	/*(sh|c)ould be used instead */
	for (j = 0; j < pMemoryPoolHandler->nMemoryPools - 1; j++) {
		if((void*)pMemoryPoolHandler->ppMemoryPools[j + 1] > pBase) {
			break;
		}
	}

	MemoryPool_Free(pMemoryPoolHandler->ppMemoryPools[j], pBase);
}

void* MemoryPoolHandler_Alloc(TMemoryPoolHandler* pMemoryPoolHandler, DECUMA_UINT32 size, 
	DECUMA_UINT32 bSpillOver
#ifdef DECUMA_DEBUG_MEMORYPOOL
	,const char * pFile, int line
#endif
	)
{
	int j;
	void* pBase = NULL;

	/*TODO: It would probably be better to first search all the pools for the insertion */
	/*      in the correct account before spilling over.  */
	/*      However it is harder to do that time efficiect. */

	/*Loop backwards, since the later ones are more likely to have free */
	for(j  = pMemoryPoolHandler->nMemoryPools - 1; j >= 0 && !pBase; j--) {
		pBase = MemoryPool_Alloc(pMemoryPoolHandler->ppMemoryPools[j], size, bSpillOver
#ifdef DECUMA_DEBUG_MEMORYPOOL
			,pFile,line
#endif
			);
	}

	return pBase;
}



/*/////////////////////////////////////// */
/* Memory pool hanldler implementation // */
/*/////////////////////////////////////// */



DECUMA_UINT32 MemoryPool_GetSize(TMemoryPoolLayout* pLayout, DECUMA_UINT32 nLayouts) {
	DECUMA_UINT32 result = 0;
	DECUMA_UINT32 i;

	decumaAssert(pLayout);
	decumaAssert(nLayouts > 0);

	result += nLayouts * (sizeof(TMemoryPoolLayout) + sizeof(TMemoryPoolAccountRange) + sizeof(TMemoryPoolAccount));
	result += nLayouts * sizeof(TFIFO);
	
	for(i = 0; i < nLayouts; i++) {
		result += pLayout[i].nElements * (sizeof(void*) + pLayout[i].size);
#ifdef DECUMA_DEBUG_MEMORYPOOL
		result += pLayout[i].nElements * sizeof(TMemoryPoolBlockDebugInfo); 
#endif
		/* ensure proper alignment */
		result = ALIGN_OFFSET(result, sizeof(void *));
	}

	result += sizeof(TMemoryPool);

	return result;
}

DECUMA_UINT32 MemoryPool_Init(TMemoryPool       *pMemoryPool, 
							  TMemoryPoolLayout *pLayout, 
							  DECUMA_UINT32      nLayouts)
{
	char * pBase = (char*)pMemoryPool;
	DECUMA_UINT32 i;
	DECUMA_UINT32 dataOffset = 0; /*offset used for calculating where to put the data */
	DECUMA_UINT32 accountOffset;  /*offset used for calculating where to position the accounts */

	decumaAssert(pLayout);
	decumaAssert(nLayouts > 0);
	decumaAssert(pMemoryPool);

	dataOffset += sizeof(TMemoryPool);

	/*copy the layout */
	pMemoryPool->nEntries = nLayouts;
	pMemoryPool->pLayout  = (TMemoryPoolLayout*)(pBase + dataOffset);

	for(i = 0; i < pMemoryPool->nEntries; i++) {
		pMemoryPool->pLayout[i] = pLayout[i];
		dataOffset += sizeof(TMemoryPoolLayout);
	}

	/*position the accounts pointer */
	pMemoryPool->pAccount = (TMemoryPoolAccount*)(pBase + dataOffset);
	accountOffset = dataOffset + pMemoryPool->nEntries * (sizeof(TMemoryPoolAccount) + sizeof(TFIFO));
	dataOffset += pMemoryPool->nEntries * sizeof(TMemoryPoolAccount);	

	/*create account entries */
	for(i = 0; i < pMemoryPool->nEntries; i++) {
		pMemoryPool->pAccount[i].blockSize = pMemoryPool->pLayout[i].size;

		pMemoryPool->pAccount[i].pBlockQueue = (TFIFO*)(pBase + dataOffset);
		FIFO_Create(pMemoryPool->pAccount[i].pBlockQueue, pMemoryPool->pLayout[i].nElements,
					(void*)(pBase + accountOffset));
		dataOffset += sizeof(TFIFO);
		accountOffset += pMemoryPool->pLayout[i].nElements * (sizeof(void*) + pMemoryPool->pLayout[i].size);

#ifdef DECUMA_DEBUG_MEMORYPOOL
		pMemoryPool->pAccount[i].pBlockDebugInfo = (TMemoryPoolBlockDebugInfo*)(pBase + accountOffset);
		accountOffset += pMemoryPool->pLayout[i].nElements * sizeof(TMemoryPoolBlockDebugInfo);
#endif
		/* ensure proper alignment */
		accountOffset = ALIGN_OFFSET(accountOffset, sizeof(void*));
	}

	/*initialize the accounts and set the pointer ranges spanned by each account */
	for(i = 0; i < pMemoryPool->nEntries; i++) {
		DECUMA_UINT32 j;
		TMemoryPoolAccount* pAccount = &pMemoryPool->pAccount[i];
		/*find out where the account data should go */
		void* dsOffset = (void*)(((char*)pMemoryPool->pAccount[i].pBlockQueue->ppElements) +
						 pMemoryPool->pLayout[i].nElements * sizeof(void*));		

		/*set the pointer ranges */
		pMemoryPool->pAccount[i].range.pLower = dsOffset;
		pMemoryPool->pAccount[i].range.pUpper = (void*)(((char*)dsOffset) +
										pMemoryPool->pLayout[i].nElements * pMemoryPool->pLayout[i].size);		

		/*insert the addresses of the available memory blocks into the queue */
		for(j = 0; j < pMemoryPool->pLayout[i].nElements; j++) {
			FIFO_Insert(pAccount->pBlockQueue, (void*)(((char*)dsOffset) + j * pMemoryPool->pLayout[i].size));
#ifdef DECUMA_DEBUG_MEMORYPOOL
			pAccount->pBlockDebugInfo[j].bBlockIsOccupied=0;
			pAccount->pBlockDebugInfo[j].pFile=0;
			pAccount->pBlockDebugInfo[j].line=0;
#endif
		}
	}

	return 0;
}

void* MemoryPool_Alloc(TMemoryPool* pMemoryPool, DECUMA_UINT32 size, 
							  DECUMA_UINT32 bSpillOver
#ifdef DECUMA_DEBUG_MEMORYPOOL
	,const char * pFile, int line
#endif
	)
{
	DECUMA_UINT32 testSize = size;
	DECUMA_UINT32 i;
	void* pBase = NULL;

	decumaAssert(size > 0);

	/*find out which account is most suitable for the requested allocation */
	/*TODO: this is candiate for a binary search, would require the memory */
	/*layout to be sorted with respect to size */
	for(i = 0; i < pMemoryPool->nEntries && !pBase; i++) {
		if(testSize <= pMemoryPool->pAccount[i].blockSize) {
			/*try allocating from this account */
			pBase = MEMORYPOOL_ALLOCATE_FROM(&pMemoryPool->pAccount[i]);

			if(bSpillOver) {
				/*if we couldn't find a block in the most suitable class, */
				/*then let size spill over to the next class and see if we can */
				/*find anything there */
				testSize = pMemoryPool->pAccount[i].blockSize + 1;
				/*NOTE: this is more efficient if the memory layout */
				/*is sorted with respect to size */
			}
		}
	}

	return pBase;
}

void MemoryPool_Free(TMemoryPool* pMemoryPool, void* pBase) 
{
	DECUMA_UINT32 i;
	decumaAssert(pMemoryPool);
	/*find out which account to return the pointer to */
	/*TODO:since the ranges are sorted a binary search */
	/*(sh|c)ould be used instead */
	for(i = 0; i < pMemoryPool->nEntries; i++) 	{
		TMemoryPoolAccount* pAccount = &pMemoryPool->pAccount[i];
		TMemoryPoolAccountRange* pRange = &pAccount->range;

		if(pRange->pLower <= pBase && pBase <= pRange->pUpper) {
			MemoryPool_FreeFrom(pAccount,pBase);
			break;
		}
	}
}

static void* MemoryPool_AllocateFrom(TMemoryPoolAccount* pAccount
#ifdef DECUMA_DEBUG_MEMORYPOOL
	,const char * pFile, int line
#endif
	) 
{
	void* pBase = NULL;

	/*if there are elements left in the vacant queue */
	/*mark it a occupied */
	if(FIFO_GetNElements(pAccount->pBlockQueue) > 0) {
		pBase = FIFO_Extract(pAccount->pBlockQueue);
#if defined(_DEBUG) || defined(DECUMA_DEBUG_MEMORYPOOL) || defined(DECUMA_ASSERT_ENABLE)
		if (pBase) {
			int j=((char*)pBase - (char*)pAccount->range.pLower) / pAccount->blockSize;
			decumaAssert((char*)pAccount->range.pLower + j*pAccount->blockSize == (char*)pBase);
			decumaAssert(j>=0 && j<(int)FIFO_GetSize(pAccount->pBlockQueue));

#ifdef DECUMA_DEBUG_MEMORYPOOL
			decumaAssert(!pAccount->pBlockDebugInfo[j].bBlockIsOccupied);
			pAccount->pBlockDebugInfo[j].bBlockIsOccupied=1;
			pAccount->pBlockDebugInfo[j].pFile=pFile;
			pAccount->pBlockDebugInfo[j].line=line;
#endif
		}
#endif
	}

	return pBase;
}

static void MemoryPool_FreeFrom(TMemoryPoolAccount* pAccount, void * pBase) {
	
	/*Add the element to the vacant queue */
#if defined(_DEBUG) || defined(DECUMA_DEBUG_MEMORYPOOL) || defined(DECUMA_ASSERT_ENABLE)
	int j = ((char*)pBase - (char*)pAccount->range.pLower) / pAccount->blockSize;
	decumaAssert((char*)pAccount->range.pLower + j * pAccount->blockSize == (char*)pBase);
	decumaAssert(j >=0 && j < (int)FIFO_GetSize(pAccount->pBlockQueue));

#ifdef DECUMA_DEBUG_MEMORYPOOL
	decumaAssert(pAccount->pBlockDebugInfo[j].bBlockIsOccupied);
	pAccount->pBlockDebugInfo[j].bBlockIsOccupied = 0;
	pAccount->pBlockDebugInfo[j].pFile= 0;
	pAccount->pBlockDebugInfo[j].line= 0;
#endif
#endif
	decumaAssert(FIFO_GetNElements(pAccount->pBlockQueue) < FIFO_GetSize(pAccount->pBlockQueue));
	FIFO_Insert(pAccount->pBlockQueue, pBase);
}

/*///////////////////////////// */
/* FIFO queue implementation // */
/*///////////////////////////// */

void FIFO_Create(TFIFO* pFIFO, DECUMA_UINT32 size, void* pBase) {
	pFIFO->size = size;
	pFIFO->head = 0;
	pFIFO->tail = 0;
	pFIFO->nElements = 0;
	pFIFO->ppElements = pBase;
}

DECUMA_UINT32 FIFO_Insert(TFIFO* pFIFO, void* pElem) {
	DECUMA_UINT32 result = 0;

	if(pFIFO->nElements < pFIFO->size) {
		pFIFO->ppElements[pFIFO->head] = pElem;
		pFIFO->head = (pFIFO->head + 1) % pFIFO->size;
		result = ++pFIFO->nElements;
	}

	return result;
}

void* FIFO_Extract(TFIFO* pFIFO) {
	void* pElem = NULL;

	if(pFIFO->nElements > 0) {
		pElem = pFIFO->ppElements[pFIFO->tail];
		decumaAssert(pElem != 0);
#ifdef _DEBUG
		pFIFO->ppElements[pFIFO->tail] = 0;
#endif
		pFIFO->tail = (pFIFO->tail + 1) % pFIFO->size;
		--pFIFO->nElements;
	}

	return pElem;
}

#endif

