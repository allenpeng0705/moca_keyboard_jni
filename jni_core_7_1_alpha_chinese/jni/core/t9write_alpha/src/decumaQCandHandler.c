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

  This file implements decumaQCandHandler.h.

\******************************************************************/

#define DECUMA_QCAND_HANDLER_C

#include "decumaQCandHandler.h"
#include "decumaQCandHandlerMacros.h"

#include "decumaAssert.h"
#include "decumaMemory.h"
#include "decumaBasicTypes.h"
#include "decumaBasicTypesMinMax.h"
#include "decumaQCandHandlerData.h"



/* */
/* Public function definitions */
/* */



int decumaQCHGetSize(void)
{
	return sizeof(DECUMA_QCH);
}

void decumaQCHInit(DECUMA_QCH* pQCH, DECUMA_QCH_CANDREF* pCandRefBuf, void* pCandBuf, unsigned int nMaxCands, unsigned int nCandSize, unsigned int nKeyOffset)
{
	unsigned int i;

	decumaAssert(pQCH);
	decumaAssert(pCandBuf);
	decumaAssert(nMaxCands > 0);
	decumaAssert(nCandSize > 0);

	pQCH->pCandRefBuf = pCandRefBuf;
	pQCH->pLastCandRef = &pQCH->pCandRefBuf[0]-1;
	pQCH->nCands = 0;

	pQCH->pCandBuf = pCandBuf;
	pQCH->nMaxCands = nMaxCands;
	pQCH->nCandSize = nCandSize;
	pQCH->nKeyOffset = nKeyOffset;

	for (i = 0; i < nMaxCands; i++)
	{
		pQCH->pCandRefBuf[i].pCand = (char*)pCandBuf + i * nCandSize;
	}
}

#ifdef _DEBUG
int decumaQCHIsFull(const DECUMA_QCH* pQCH)
{
	return decumaQCHIsFullMacro(pQCH);
}

int decumaQCHGetCount(const DECUMA_QCH* pQCH)
{
	return decumaQCHGetCountMacro(pQCH);
}
#endif

void decumaQCHAdd(DECUMA_QCH* pQCH, DECUMA_UINT32 nKey, const void* pCand, const DECUMA_QCH_CANDREF** pCandRefToReplace)
{
	DECUMA_QCH_CANDREF* pNew;
	const DECUMA_QCH_CANDREF* pReplaceeCandRef = NULL;
	void* pReplaceeCand;

	if (pCandRefToReplace) pReplaceeCandRef = *pCandRefToReplace;

	if (!pReplaceeCandRef && pQCH->nCands == pQCH->nMaxCands && nKey >= pQCH->pLastCandRef->nKey) return; /* Not qualifying */

	if (!pReplaceeCandRef)
	{
		if (pQCH->nCands == pQCH->nMaxCands)
		{
			pReplaceeCandRef = pQCH->pLastCandRef;
		}
		else
		{
			pReplaceeCandRef = pQCH->pLastCandRef+1;
		}
	}

	pNew = (DECUMA_QCH_CANDREF*)decumaQCHFindKey(pQCH, nKey);

	decumaAssert(pReplaceeCandRef >= &pQCH->pCandRefBuf[0]);
	decumaAssert(pReplaceeCandRef <= pQCH->pLastCandRef+1);
	decumaAssert(pNew >= &pQCH->pCandRefBuf[0]);
	decumaAssert(pNew <= pQCH->pLastCandRef+1);

	pReplaceeCand = pReplaceeCandRef->pCand; /* Store before overwritten */

	if (pNew < pReplaceeCandRef)
	{
		decumaMemmove(&pNew[1], pNew, (char*)pReplaceeCandRef - (char*)pNew);
	}
	else if (pNew > pReplaceeCandRef)
	{
		pNew--;
		if (pNew > pReplaceeCandRef)
		{
			decumaMemmove((DECUMA_QCH_CANDREF*)pReplaceeCandRef, &pReplaceeCandRef[1], (char*)pNew - (char*)pReplaceeCandRef);
		}
	}

	pNew->nKey = nKey;
	pNew->pCand = pReplaceeCand;
	if (pCand) decumaMemcpy(pNew->pCand, pCand, pQCH->nCandSize);

	if (pCandRefToReplace) *pCandRefToReplace = pNew;

	if (pReplaceeCandRef == pQCH->pLastCandRef+1)
	{
		pQCH->pLastCandRef++;

		pQCH->nCands++;
	}
}

const DECUMA_QCH_CANDREF* decumaQCHFindKey(const DECUMA_QCH* pQCH, DECUMA_UINT32 nKey)
{
	const DECUMA_QCH_CANDREF* const pCandRefs = pQCH->pCandRefBuf;
	unsigned int nInd, nLowInd, nHighInd;

	if (pQCH->nCands == 0 || nKey >= pCandRefs[pQCH->nCands-1].nKey) return decumaQCHGetEndOfCandRefs(pQCH);
	if (nKey < pCandRefs[0].nKey) return &pCandRefs[0];

	nLowInd = 0;
	nHighInd = pQCH->nCands;

	do
	{
		nInd = (nLowInd + nHighInd) / 2;

		if (nKey < pCandRefs[nInd].nKey) nHighInd = nInd;
		else nLowInd = nInd+1;

	} while (nLowInd != nHighInd);

	decumaAssert(nLowInd > 0);
	decumaAssert(nKey >= pCandRefs[nLowInd-1].nKey);
	decumaAssert(nLowInd == pQCH->nCands || nKey < pCandRefs[nLowInd].nKey);

	return &pCandRefs[nLowInd];
}

#ifdef _DEBUG
const DECUMA_QCH_CANDREF* decumaQCHGetStartOfCandRefs(const DECUMA_QCH* pQCH)
{
	return decumaQCHGetStartOfCandRefsMacro(pQCH);
}

const DECUMA_QCH_CANDREF* decumaQCHGetEndOfCandRefs(const DECUMA_QCH* pQCH)
{
	return decumaQCHGetEndOfCandRefsMacro(pQCH);
}

const DECUMA_QCH_CANDREF* decumaQCHGetPrevCandRef(const DECUMA_QCH_CANDREF* pCandRef)
{
	return decumaQCHGetPrevCandRefMacro(pCandRef);
}

const DECUMA_QCH_CANDREF* decumaQCHGetNextCandRef(const DECUMA_QCH_CANDREF* pCandRef)
{
	return decumaQCHGetNextCandRefMacro(pCandRef);
}

DECUMA_UINT32 decumaQCHGetKey(const DECUMA_QCH_CANDREF* pCandRef)
{
	return decumaQCHGetKeyMacro(pCandRef);
}

void* decumaQCHGetCand(const DECUMA_QCH_CANDREF* pCandRef)
{
	return decumaQCHGetCandMacro(pCandRef);
}
#endif

const DECUMA_QCH_CANDREF* decumaQCHFindCand(const DECUMA_QCH* pCH, DECUMA_UINT32 nKey, const void *pCand)
{
	const DECUMA_QCH_CANDREF* pCandRef;

	/* Get first candidate with higher key */
	pCandRef = decumaQCHGetPrevCandRef(decumaQCHFindKey(pCH, nKey));

	if (pCandRef == decumaQCHGetStartOfCandRefs(pCH)) return NULL;

	while (decumaQCHGetKey(pCandRef) == nKey)
	{
		if (decumaQCHGetCand(pCandRef) == pCand) break;

		pCandRef = decumaQCHGetPrevCandRef(pCandRef);
	}

	return decumaQCHGetCand(pCandRef) == pCand ? pCandRef : NULL;
}

void decumaQCHRepair(const DECUMA_QCH* pQCH, const DECUMA_QCH_CANDREF** ppCandRef, DECUMA_UINT32 nNewKey)
{
	DECUMA_QCH_CANDREF* pOldPos;
	DECUMA_QCH_CANDREF* pNewPos;
	void* pNewCand;

	decumaAssert(ppCandRef);

	pOldPos = (DECUMA_QCH_CANDREF*)(*ppCandRef);

	decumaAssert(pOldPos >= &pQCH->pCandRefBuf[0]);
	decumaAssert(pOldPos <= pQCH->pLastCandRef);

	pNewPos = (DECUMA_QCH_CANDREF*)decumaQCHFindKey(pQCH, nNewKey);

	decumaAssert(pNewPos >= &pQCH->pCandRefBuf[0]);
	decumaAssert(pNewPos <= pQCH->pLastCandRef+1);

	pNewCand = pOldPos->pCand;

	if (pNewPos < pOldPos)
	{
		decumaMemmove(&pNewPos[1], pNewPos, (char*)pOldPos - (char*)pNewPos);
	}
	else if (pNewPos > pOldPos)
	{
		pNewPos--;
		if (pNewPos > pOldPos)
		{
			decumaMemmove((DECUMA_QCH_CANDREF*)pOldPos, &pOldPos[1], (char*)pNewPos - (char*)pOldPos);
		}
	}

	pNewPos->nKey = nNewKey;
	pNewPos->pCand = pNewCand;

	*ppCandRef = pNewPos;
}

int decumaQCHVerifyCandRef(const DECUMA_QCH* pQCH, const DECUMA_QCH_CANDREF* pCandRef)
{
	return pCandRef->nKey == *((DECUMA_UINT32*)((char*)pCandRef->pCand + pQCH->nKeyOffset));
}

int decumaQCHGetRanking(const DECUMA_QCH* pQCH, DECUMA_UINT16* pRanking)
{
	const DECUMA_QCH_CANDREF* pCandRefs = pQCH->pCandRefBuf;
	const DECUMA_QCH_CANDREF* const pLastCandRef = pQCH->pLastCandRef;
	char* const pBuf = pQCH->pCandBuf;
	const unsigned int nCandSize = pQCH->nCandSize;

	for (; pCandRefs <= pLastCandRef; pCandRefs++, pRanking++)
	{
		*pRanking = (DECUMA_UINT16)(((char*)pCandRefs->pCand - pBuf) / nCandSize);
	}

	return pQCH->nCands;
}


int decumaQCHApplyRanking(DECUMA_QCH* pQCH, void* pTmpCandMem)
{
	DECUMA_QCH_CANDREF* pCandRefs = pQCH->pCandRefBuf;
	DECUMA_QCH_CANDREF* const pLastCandRef = pQCH->pLastCandRef;
	char* pBuf = pQCH->pCandBuf;
	const unsigned int nCandSize = pQCH->nCandSize;

	for (; pCandRefs <= pLastCandRef; pCandRefs++)
	{
		DECUMA_QCH_CANDREF* pCandRefs2 = pCandRefs+1;

		decumaMemcpy(pTmpCandMem, pBuf, nCandSize);
		decumaMemcpy(pBuf, pCandRefs->pCand, nCandSize);

		for (; pCandRefs2 <= pLastCandRef; pCandRefs2++)
		{
			if (pCandRefs2->pCand == pBuf)
			{
				pCandRefs2->pCand = pCandRefs->pCand;
				decumaMemcpy(pCandRefs2->pCand, pTmpCandMem, nCandSize);
				break;
			}
		}
		pCandRefs->pCand = pBuf;

		pBuf += nCandSize;
	}

	return pQCH->nCands;
}

/*Adding dummy #pragma to get this file to compile correctly with RVCT 3.0 -O3 -Otime */
/*Buggy compiler */
 
