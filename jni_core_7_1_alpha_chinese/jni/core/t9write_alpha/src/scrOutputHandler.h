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


#ifndef OUTPUTLIST_H
#define OUTPUTLIST_H

#ifdef DECUMA_MANGLE
#include "mangle.h"
#endif

#include "scrOutput.h"

#define MAX_NUMBER_OF_OUTPUTS_IN_BUFFER 30


/*A node of a single linked list. Each node has a pointer to an scrOUTPUT, and a  */
/*compareValue that decides its position in the list. */
typedef struct tagNode{
	struct tagNode * pNext;
	int compareValue;
	scrOUTPUT * pOut;
} NODE;


/* The outputList structure will keep track of the outputs that are added */
/* and will contain a linked list of nodes, where each node has a pointer to */
/* an scrOUTPUT. The linked list has its head in pOutputList->headNode which will */
/* have a pNext that points to the added output with worst (mu-)value. The whole */
/* list is sorted "worst first". */
typedef struct {

	scrOUTPUT * outputBuffer; /*Pointer to a memory chunk that can hold at least nMaxOutputs scrOUTPUTs. */
	NODE nodeBuffer[MAX_NUMBER_OF_OUTPUTS_IN_BUFFER];
	
	NODE headOfFreeList; /*Head of a list of the free nodes that can be used */
	NODE headNode; /*Head of sorted linked list */

	int nMaxOutputs; /*The maximum allowed outputs */
	int nMaxSameSymbol; /*The maximum allowed outputs of same symbol, 0 means no constraint */
	int nMaxSameType; /*The maximum allowed outputs of same type, 0 means no constraint */
	int nOutputs; /*How many outputs are there actually */

	NODE * pCurrentSeqNode; /*This is used for keeping track of the sequensial access to the outputs */

}OUTPUT_HANDLER;



DECUMA_HWR_PRIVATE void initOutputHandler( OUTPUT_HANDLER * pOutputHandler, scrOUTPUT * pOutputBuf, int nMaxOutputs, int nMaxOfSameSymbol, int nMaxOfSameType);

DECUMA_HWR_PRIVATE void addToOutputHandlerIfGoodEnoughAndNotTooManyOfSameSymbolOrType(
	OUTPUT_HANDLER * pOutputHandler, int compareValue,
	scrOUTPUT * pOut, int bConsiderConflicts);

DECUMA_HWR_PRIVATE int getTheOutputBufferSortedBestFirstAndDestroyHandler( OUTPUT_HANDLER * pOutputHandler);

DECUMA_HWR_PRIVATE scrOUTPUT * seqGetWorstOutput( OUTPUT_HANDLER * pOutputHandler, int * pCompareValue);

DECUMA_HWR_PRIVATE scrOUTPUT * seqGetNextBetterOutput( OUTPUT_HANDLER * pOutputHandler, int * pCompareValue );

DECUMA_HWR_PRIVATE scrOUTPUT * getBestOutput( OUTPUT_HANDLER * pOutputHandler, int * pCompareValue );

DECUMA_HWR_PRIVATE void keepOnlyTheNBestOutputs( OUTPUT_HANDLER * pOutputHandler, int nOutputsToKeep);

#endif

