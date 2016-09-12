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


#include "scrOutputHandler.h"
#include "database.h"
#include "decumaAssert.h"
#include "decumaString.h"
#include "decumaMath.h"
#include "globalDefs.h"

#ifndef NULL
#define NULL 0
#endif


/*************************************************************************************/

static int getDepthOfList( NODE * pHead )
{
	int i = 0;
	NODE * pNode= pHead;

	while (pNode->pNext)
	{
		i=i+1;
		pNode = pNode->pNext;
	};
	return i;
}

void initOutputHandler( OUTPUT_HANDLER * pOutputHandler, scrOUTPUT * pOutputBuf, int nMaxOutputs, int nMaxOfSameSymbol, int nMaxOfSameType )
{
	/* Initiates an outputHandler. Call this before calls to addToOutputHandlerIf... */
	/* The outputHandler structure will keep track of the outputs that are added */
	/* and will contain a linked list of nodes, where each node has a pointer to */
	/* an scrOUTPUT. The linked list has its head in pOutputHandler->headNode which will */
	/* have a pNext that points to the added output with worst (mu-)value. The whole */
	/* list is sorted worst first. */

	/* pOutputBuffer should be a pointer to a memory chunk that can hold at least nMaxOutputs scrOUTPUTs. */
	/* pNodeBuffer should be a pointer to a memory chunk that can hold at least nMaxOutputs NODEs. */

	/*Connects outputs to nodes. Makes a freeList that will contain free NODEs. */
	
	int i;

	decumaAssert(nMaxOutputs <= MAX_NUMBER_OF_OUTPUTS_IN_BUFFER);
	decumaAssert(nMaxOutputs >0);


	pOutputHandler->outputBuffer = pOutputBuf;

	pOutputHandler->nMaxOutputs = minNUMB(nMaxOutputs,MAX_NUMBER_OF_OUTPUTS_IN_BUFFER);
	pOutputHandler->nMaxSameSymbol = nMaxOfSameSymbol;
	pOutputHandler->nMaxSameType = nMaxOfSameType;
	pOutputHandler->headNode.pNext = NULL;
	for (i=0; i<nMaxOutputs-1; i++)
	{
		pOutputHandler->nodeBuffer[i].pNext = &pOutputHandler->nodeBuffer[i+1];
		pOutputHandler->nodeBuffer[i].pOut = &pOutputHandler->outputBuffer[i];
	}
	pOutputHandler->nodeBuffer[nMaxOutputs-1].pNext = NULL;
	pOutputHandler->nodeBuffer[nMaxOutputs-1].pOut = &pOutputHandler->outputBuffer[nMaxOutputs-1];
	pOutputHandler->headOfFreeList.pNext = &pOutputHandler->nodeBuffer[0];
	pOutputHandler->nOutputs = 0;

	decumaAssert( getDepthOfList(&pOutputHandler->headOfFreeList) == pOutputHandler->nMaxOutputs - pOutputHandler->nOutputs);
}

static void removeNode( NODE * pPredecessor, OUTPUT_HANDLER * pOutputHandler)
{
	/*Takes out a node from the outputHandler and puts it into the freeList. */

	NODE * pNodeToRemove = pPredecessor->pNext;

	decumaAssert( getDepthOfList(&pOutputHandler->headOfFreeList) == pOutputHandler->nMaxOutputs - pOutputHandler->nOutputs);
	decumaAssert( getDepthOfList(&pOutputHandler->headNode) == pOutputHandler->nOutputs);
	decumaAssert( pOutputHandler->headNode.pNext );
	decumaAssert( pPredecessor->pNext);
	decumaAssert( pOutputHandler->nOutputs >= 0 );

	pPredecessor->pNext = pNodeToRemove->pNext;
	pNodeToRemove->pNext = pOutputHandler->headOfFreeList.pNext;
	pOutputHandler->headOfFreeList.pNext = pNodeToRemove;
	pOutputHandler->nOutputs--;

	decumaAssert( getDepthOfList(&pOutputHandler->headOfFreeList) == pOutputHandler->nMaxOutputs - pOutputHandler->nOutputs);
	decumaAssert( getDepthOfList(&pOutputHandler->headNode) == pOutputHandler->nOutputs);
}

static void insertNode( NODE * pPredecessor, NODE * pNewNode, OUTPUT_HANDLER * pOutputHandler )
{
	/*Puts the NODE pointed to by pNewNode into the outputHandler just after the NODE pointed to by pPredecessor. */

	decumaAssert( pOutputHandler->nOutputs < pOutputHandler->nMaxOutputs );
	decumaAssert( pNewNode);
	decumaAssert( getDepthOfList(&pOutputHandler->headNode) == pOutputHandler->nOutputs);

	pNewNode->pNext = pPredecessor->pNext;
	pPredecessor->pNext = pNewNode;
	pOutputHandler->nOutputs ++;

	decumaAssert(  getDepthOfList(&pOutputHandler->headNode) == pOutputHandler->nOutputs);
	decumaAssert( pOutputHandler->headNode.pNext);
	decumaAssert( getDepthOfList(&pOutputHandler->headOfFreeList) == pOutputHandler->nMaxOutputs - pOutputHandler->nOutputs);
}

static NODE * getAndFillFreeNode( OUTPUT_HANDLER * pOutputHandler, scrOUTPUT * pOut, int compareValue)
{
	/*Returns a pointer to a NODE that is taken out from the freeList. The output connected */
	/* to this node will be set to *pOut, and the "compareValue" of the node will be set to compareValue. */

	NODE * pToReturn;

	if (pOutputHandler->headOfFreeList.pNext == NULL)
		return NULL;
	else
	{
		pToReturn = pOutputHandler->headOfFreeList.pNext;
		pOutputHandler->headOfFreeList.pNext = pToReturn->pNext;

#ifdef _DEBUG
		pToReturn->pNext = 0; /*Not really necessary */
#endif
		*pToReturn->pOut = *pOut; /*Could maybe be optimized //takes time */
		pToReturn->compareValue = compareValue;

		return pToReturn;
	}
}

static int hasAlreadyTooManyBetterOnesOfSameSymbolOrType( scrOUTPUT * pOut, NODE ** ppLastWorseNode, 
									  int nWorseSymbolsAlreadyFound, int nWorseTypesAlreadyFound,
									  NODE * pPredToWorstSymbolAlreadyFound, NODE * pPredToWorstTypeAlreadyFound,
									  OUTPUT_HANDLER * pOutputHandler)
{
	/*Returns 1 if there are already nMaxSameSymbol better nodes with the same symbol as pOut. */
	/*If there are nMaxSameSymbols nodes with same symbol but not all of them are better */
	/*than this output, the worst of these nodes are removed from the list, and 0 is returned. */
	/*0 is also returned if there is still room for more nodes with the same symbol. */

	NODE * pNode = (*ppLastWorseNode)->pNext;
	int nAllowMoreSymbols;
	int nAllowMoreTypes;

	if( pOutputHandler->nMaxSameSymbol )
	{
		nAllowMoreSymbols = pOutputHandler->nMaxSameSymbol - nWorseSymbolsAlreadyFound; /*Count backwards */
	}
	else
	{
		nAllowMoreSymbols = pOutputHandler->nMaxOutputs; /* No limit */
	}

	if( pOutputHandler->nMaxSameType )
	{
		nAllowMoreTypes = pOutputHandler->nMaxSameType - nWorseTypesAlreadyFound; /*Count backwards */
	}
	else
	{
		nAllowMoreTypes = pOutputHandler->nMaxOutputs; /* No limit */
	}

	decumaAssert(ppLastWorseNode);
	decumaAssert(*ppLastWorseNode);

	while ( pNode && nAllowMoreSymbols && nAllowMoreTypes )
	{
		decumaAssert(pNode->pOut);

		if ( pOut->outSymbol == pNode->pOut->outSymbol && isSameDBSymbol(pNode->pOut->symbol, pOut->symbol) )
		{
			if( pNode->pOut->DBindex.pKey->typeIdx == pOut->DBindex.pKey->typeIdx )
			{
				nAllowMoreTypes --;
			}
			nAllowMoreSymbols --;
		}
		pNode = pNode->pNext;
	}

	if (!nAllowMoreTypes)
	{
		if (nWorseTypesAlreadyFound == 0)
		{
			/*Do not insert a new node. We have enough better ones of this type. */
			return 1;
		}
		else
		{
			/*Remove worst of this type and return that (one) more of this type can be added */
			if (*ppLastWorseNode == pPredToWorstTypeAlreadyFound->pNext)
			{
				/*If we are about to remove the node pointed to by *ppLastWorseNode we have to */
				/*change this pointer. */
				*ppLastWorseNode = pPredToWorstTypeAlreadyFound;
			}
			removeNode(pPredToWorstTypeAlreadyFound,pOutputHandler);
			return 0;
		}
	}

	if (!nAllowMoreSymbols)
	{
		/* If there are type constraints, we should not end up here (if more types are allowed more symbols */
		/* should also be allowed). /JA */
		decumaAssert( pOutputHandler->nMaxSameType == 0 );

		if (nWorseSymbolsAlreadyFound == 0)
		{
			/*Do not insert a new node. We have enough better ones of this symbol. */
			return 1;
		}
		else
		{
			/*Remove worst of this symbol and return that (one) more of this symbol can be added */
			if (*ppLastWorseNode == pPredToWorstSymbolAlreadyFound->pNext)
			{
				/*If we are about to remove the node pointed to by *ppLastWorseNode we have to */
				/*change this pointer. */
				*ppLastWorseNode = pPredToWorstSymbolAlreadyFound;
			}
			removeNode(pPredToWorstSymbolAlreadyFound,pOutputHandler);
			return 0;
		}
	}
	return 0;
}

void addToOutputHandlerIfGoodEnoughAndNotTooManyOfSameSymbolOrType(
	OUTPUT_HANDLER * pOutputHandler, int compareValue,
	scrOUTPUT * pOut, int bConsiderConflicts)
{
	/*Adds the output pointed to by pOut to the outputBuffer and inserts it into */
	/*the linked list on its correct position, IF it is good enough and there is not */
	/*too many of that symbol already. If bConsiderConflicts is non-zero */
	/*conflicts are taken in consideration as well. */
	/*NOTE: This function doesn't check if there are outputs with the same allograph */
	/*in the buffer already. Just the same symbol. */

	int nSameSymbol = 0;
	int nSameType = 0;
	NODE * pPredToWorstNodeSameSymbol = NULL; /*Store a pointer to the predecessor to the worst */
		/*node with the same symbol; */
	NODE * pPredToWorstNodeSameType = NULL; /*Store a pointer to the predecessor to the worst */
		/*node with the same type; */
	NODE * pNode = &pOutputHandler->headNode;
	TYPE_CONFLICTS_PTR pOutConflict = NULL;

	/*Iterate through the nodes. Start with the worst node. Iterate until you find a better compare value */
	/*than the one passed to this funciton. */
	/*Always check one step ahead because we don't want to lose the contact to the predecessor.  */

	/* This ifdef is just a temporary hack. The conflict categories in */
	/* the japanese DB is not working properly. */
	/* This hack was moved from the old category handling (AJ). */
#ifdef JAPANESE
		bConsiderConflicts = 0;
#endif

	if ( bConsiderConflicts ) {
		/* Lets check if the new output has any conflicts. */
		pOutConflict = outputGetConflictData(pOut);
	}

	/*CONFLICT HANDLING - overview */
	/* */
	/*If there is a conflict with this output, and even if the conflicting output */
	/*is worse than this output, then this output will be put  */
	/*just before (worse than) the conflicting output in the linked list. */
	/* */
	/*If there already is an output in conflict with this output, and if it is */
	/*the following (just better) output. Then the new output will be put  */
	/*just after (better than) the output in conflict with this. */
	/* */
	/*JM: TODO Is this the best way to handle conflicts? It works OK for nMaxOutputs==2.  */
	/*    But in other cases..? The result is highly depending on the order of which */
	/*    outputs are added to the handler. See decumaCandidateHandler.c for a better */
	/*    approach via the function decumaCHMoveCandidate */

	while ( pNode->pNext && 
		(	(pNode->pNext->compareValue > compareValue && 
		( !bConsiderConflicts || 
			/* Check if the node shall be prioritized because of a conflict? */
		  !outputConflict(pOutConflict, pNode->pNext->pOut) )) 
		||
			( bConsiderConflicts && 
			/* Check if the new output shall be prioritized because of a conflict? */
			outputConflict(outputGetConflictData(pNode->pNext->pOut), pOut) )
		))

	{
		if ( pOut->outSymbol == pNode->pNext->pOut->outSymbol && isSameDBSymbol(pOut->symbol,pNode->pNext->pOut->symbol) )
		{
			if( pNode->pNext->pOut->DBindex.pKey->typeIdx == pOut->DBindex.pKey->typeIdx )
			{
				if (nSameType==0)
				{
					/*This next node is the first (and thus worst) node of this symbol */
					pPredToWorstNodeSameType = pNode;
				}
				nSameType++;
			}
			if (nSameSymbol==0)
			{
				/*This next node is the first (and thus worst) node of this symbol */
				pPredToWorstNodeSameSymbol = pNode;
			}
			nSameSymbol++;
		}
		pNode = pNode->pNext;
	}

	if (pNode == &pOutputHandler->headNode)
	{
		/*Current output was worse than worst, or there was no one before. */
		if (pOutputHandler->headOfFreeList.pNext)
		{
			/*We have free nodes */
			if (!hasAlreadyTooManyBetterOnesOfSameSymbolOrType(
				   pOut, &pNode, 0, 0, NULL, NULL,pOutputHandler)
				)
			{
				/*And we are allowed to insert a(nother) node of this symbol */
				insertNode(&pOutputHandler->headNode , 
					getAndFillFreeNode(pOutputHandler,pOut,compareValue),
					pOutputHandler);
			}
			return;
		}
	}
	else
	{
		/*This was better than some nodes. */

		if (!hasAlreadyTooManyBetterOnesOfSameSymbolOrType(
			   pOut, &pNode, nSameSymbol, nSameType, pPredToWorstNodeSameSymbol, pPredToWorstNodeSameType, pOutputHandler) )
		{
			/*And we are allowed to insert a(nother) node of this symbol */
			if ( !pOutputHandler->headOfFreeList.pNext )
			{
				/*There is no free node, remove the worst node. */
				if ( pNode == pOutputHandler->headNode.pNext)
				{
					pNode = &pOutputHandler->headNode;
				}
				removeNode( &pOutputHandler->headNode, pOutputHandler);
			}
			insertNode(pNode , getAndFillFreeNode(pOutputHandler,pOut,compareValue), 
				pOutputHandler);
		}
	}
}

scrOUTPUT * seqGetWorstOutput( OUTPUT_HANDLER * pOutputHandler, int * pCompareValue)
{
	/* A call to this function sets the current node pointer to the worst node */
	/* and returns a pointer to the output of this node. */
	/* The *pComareValue will be set to the compareValue of the current node, if pCompareValue != NULL */
	/* If there is no output in the handler, nothing will be done and NULL returned. */

	pOutputHandler->pCurrentSeqNode = pOutputHandler->headNode.pNext;

	if (pOutputHandler->pCurrentSeqNode)
	{
		if (pCompareValue)
		{
			*pCompareValue = pOutputHandler->pCurrentSeqNode->compareValue;
		}
		return pOutputHandler->pCurrentSeqNode->pOut;
	}
	else return NULL;
}

scrOUTPUT * seqGetNextBetterOutput( OUTPUT_HANDLER * pOutputHandler, int * pCompareValue )
{
	/* A call to this function sets the current node pointer to the next node, */
	/* i.e. the node that is just better than the one we were at before, */
	/* and returns a pointer to the output of this node. */
	/* The *pComareValue will be set to the compareValue of the current node, if pCompareValue != NULL */
	/* If there is no better output nothing will be done and NULL returned. */

	if (pOutputHandler->pCurrentSeqNode && pOutputHandler->pCurrentSeqNode->pNext)
	{
		pOutputHandler->pCurrentSeqNode = pOutputHandler->pCurrentSeqNode->pNext;

		if (pCompareValue)
		{
			*pCompareValue = pOutputHandler->pCurrentSeqNode->compareValue;
		}
		return pOutputHandler->pCurrentSeqNode->pOut;
	}
	else return NULL;
}

scrOUTPUT * getBestOutput( OUTPUT_HANDLER * pOutputHandler, int * pCompareValue )
{
	/*This gives a pointer to the best output. */
	/*It does not affect the current pointer. */
	/*The *pComareValue will be set to the compareValue of the best node, if pCompareValue != NULL */

	NODE * pTemp = pOutputHandler->headNode.pNext;

	if (!pTemp)
	{
		return NULL;
	}
	else
	{
		while (pTemp->pNext)
			pTemp = pTemp->pNext;

		if (pCompareValue)
		{
			*pCompareValue = pTemp->compareValue;
		}
		return pTemp->pOut;
	}
}

void keepOnlyTheNBestOutputs( OUTPUT_HANDLER * pOutputHandler, int nOutputsToKeep)
{
	int nOutputsToThrowAway = pOutputHandler->nOutputs - nOutputsToKeep;

	for ( ; nOutputsToThrowAway > 0; nOutputsToThrowAway--)
	{
		removeNode( &pOutputHandler->headNode, pOutputHandler);
	}
}

int getTheOutputBufferSortedBestFirstAndDestroyHandler( OUTPUT_HANDLER * pOutputHandler)
{
	/* Sorts the output buffer of the outputHandler with help from the linked list. 
	 * The linked list (starting with pOutputHandler->headNode) will be invalid 
	 * after this function is called since the outputs are shuffled.
	 */
	
	scrOUTPUT tempBuffer[MAX_NUMBER_OF_OUTPUTS_IN_BUFFER];
	NODE * pNode;
	int i;

	decumaAssert( getDepthOfList(&pOutputHandler->headOfFreeList) == pOutputHandler->nMaxOutputs - pOutputHandler->nOutputs);
	decumaAssert( getDepthOfList(&pOutputHandler->headNode) == pOutputHandler->nOutputs);
	decumaAssert( pOutputHandler->headNode.pNext );

	pNode = pOutputHandler->headNode.pNext;
	for ( i = pOutputHandler->nOutputs - 1; i >= 0; i-- )
	{
		decumaAssert( pNode );
		tempBuffer[i] = *pNode->pOut;
		pNode = pNode ->pNext;
	}
	
	decumaAssert(!pNode);

	myMemCopy(pOutputHandler->outputBuffer, tempBuffer, sizeof(tempBuffer[0]) * pOutputHandler->nOutputs);
	
	/* Destroy the linked list */
	pOutputHandler->headNode.pNext = NULL;

	return pOutputHandler->nOutputs;
}

