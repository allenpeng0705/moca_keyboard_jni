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

  This file provides the interface to operate on DECUMA_QCH object.

  This module provides basic, but fast, operations on a list of
  ranked candidates. This includes

  - Very fast candidate search by key (using binary search)
  - Fast candidate addition (supports provision of optional replacee)
  - Fast linear traversing of candidates in ranking order
  - Fast provision of sorting indeces according to ranking
  - Application of sorting according to ranking
  - Repairation (if other handler has been operating on the same buffer)
  - Verification

  The handler is initialized with a pointer to a buffer where the
  candidate list should be stored. Candidates to be added to the
  list are provided as a key and a pointer to the candidate data.
  The candidates are accessed through references to enable fast
  operations.

  IMPORTANT NOTE:
  Any candidate reference returned becomes invalid after next add,
  repair and apply ranking operation.

\******************************************************************/

#ifndef DECUMA_QCAND_HANDLER_H
#define DECUMA_QCAND_HANDLER_H

#include "decumaConfig.h"

typedef struct _DECUMA_QCH          DECUMA_QCH;

#include "decumaBasicTypes.h"

typedef struct _DECUMA_QCH_CANDREF
{
	DECUMA_UINT32 nKey;
	void*         pCand;

} DECUMA_QCH_CANDREF;

#if !defined(_DEBUG)
#include "decumaQCandHandlerMacros.h"
#endif

/*
    Returns the size in bytes of a DECUMA_QCH object.
*/
DECUMA_HWR_PRIVATE int decumaQCHGetSize(void);

/*
    Initializes pQCH with pCandBuf. At least the number of bytes returned by
	decumaQCHGetSize() must have been allocated for pQCH. The memory pointed
	to by pCandBuf will be used to write the added candidates to. At least
	nMaxCands * nCandSize bytes of memory must have been allocated for it.
	The memory pointed to by pCandRefBuf will be used to write the references
	to the added candidates to. At least nMaxCands * sizeof(DECUMA_QCH_CANDREF)
	bytes of memory must have been allocated for it. The nKeyOffset defines an
	offset from the candidate start to a position within the candidate where
	the key is stored. This is only use for	repair and verification and it is
	therefore optional to set it properly (candidates do not have to contain
	the key).
*/
DECUMA_HWR_PRIVATE void decumaQCHInit(DECUMA_QCH* pQCH, DECUMA_QCH_CANDREF* pCandRefBuf, void* pCandBuf, unsigned int nMaxCands, unsigned int nCandSize, unsigned int nKeyOffset);

#ifdef _DEBUG
/*
    Returns 1 if pQCH is full and 0 if it is not full.
*/
DECUMA_HWR_PRIVATE int decumaQCHIsFull(const DECUMA_QCH* pQCH);

/*
    Returns the number of candidates in pQCH.
*/
DECUMA_HWR_PRIVATE int decumaQCHGetCount(const DECUMA_QCH* pQCH);
#endif

/*
    Tries to add pCand as a new candidate with key nKey to the list. If pQCH
	is not full the addition will succeed. If *pCandRefToReplace contains a
	pointer to a valid candidate reference the addition will succeed and the
	candidate referred to by *pCandRefToReplace will be removed (even if pQCH
	is not full). If *pCandRefToReplace is invalid and pQCH is full the	addition
	will succeed if there is a candidate with larger key value in pQCH (then
	this candidate will be replaced), otherwise it will fail. If the addition
	was successful a reference to the added candidate will be written to
	*pCandRefToReplace unless pCandRefToReplace	is 0. It is ok to set 
	pCandRefToReplace to 0 if there is no replacee and a reference to the added
	candidate is not needed. If the candidate has already been written (e.g. by
	another handler) to the candidate buffer (pCandBuf) pCand can be set to 0
	to avoid writing the same data again, then this function only updates the
	candidate references.

	IMPORTANT NOTE:
	Makes all previously returned candidate references invalid.
*/
DECUMA_HWR_PRIVATE void decumaQCHAdd(DECUMA_QCH* pQCH, DECUMA_UINT32 nKey, const void* pCand, const DECUMA_QCH_CANDREF** pCandRefToReplace);

/*
    Finds the first candidate in pQCH with key that is larger than nKey and
	returns a reference to it. If no such candidate is found this function
	returns the	same result as decumaQCHGetEndOfCandRefs().
*/
DECUMA_HWR_PRIVATE const DECUMA_QCH_CANDREF* decumaQCHFindKey(const DECUMA_QCH* pQCH, DECUMA_UINT32 nKey);

#ifdef _DEBUG
/*
    Returns a start indicator to facilitate traversing the pQCH candidate
	list. Note that the "candidate reference" returned by this function is
	not	considered a valid candidate reference. It is rather intended to be
	used to identify when a non-valid candidate reference has been returned.
*/
DECUMA_HWR_PRIVATE const DECUMA_QCH_CANDREF* decumaQCHGetStartOfCandRefs(const DECUMA_QCH* pQCH);

/*
    Returns an end indicator to facilitate traversing the pQCH candidate
	list. Note that the "candidate reference" returned by this function is
	not	considered a valid candidate reference. It is rather intended to be
	used to identify when a non-valid candidate reference has been returned.
*/
DECUMA_HWR_PRIVATE const DECUMA_QCH_CANDREF* decumaQCHGetEndOfCandRefs(const DECUMA_QCH* pQCH);

/*
    Returns a reference to a candidate preceeding another candidate in ranking
	order. pCandRef must point to a valid candidate reference. If pCandRef
	refers to the first ranked candidate this function returns the same result
	as decumaQCHGetStartOfCandRefs().
*/
DECUMA_HWR_PRIVATE const DECUMA_QCH_CANDREF* decumaQCHGetPrevCandRef(const DECUMA_QCH_CANDREF* pCandRef);

/*
    Returns a reference to a candidate succeeding another candidate in ranking
	order. pCandRef must point to a valid candidate reference. If pCandRef
	refers to the last ranked candidate this function returns the same result
	as decumaQCHGetEndOfCandRefs().
*/
DECUMA_HWR_PRIVATE const DECUMA_QCH_CANDREF* decumaQCHGetNextCandRef(const DECUMA_QCH_CANDREF* pCandRef);

/*
    Returns the key of the candidate referred to by pCandRef.
*/
DECUMA_HWR_PRIVATE DECUMA_UINT32 decumaQCHGetKey(const DECUMA_QCH_CANDREF* pCandRef);

/*
    Returns the candidate referred to by pCandRef.
*/
DECUMA_HWR_PRIVATE void* decumaQCHGetCand(const DECUMA_QCH_CANDREF* pCandRef);
#endif

/*
    Finds the candidate pCand in pQCH. Its key nKey is used to speed up the search.
	The reference to the candidate is returned or 0 if it was not found.
*/
DECUMA_HWR_PRIVATE const DECUMA_QCH_CANDREF* decumaQCHFindCand(const DECUMA_QCH* pCH, DECUMA_UINT32 nKey, const void *pCand);

/*
    Repairs *ppCandRef with correct key nNewKey if its underlying candidate has
	changed (e.g. if another handler operates on the same candidate buffer).

	IMPORTANT NOTE:
	Makes all previously returned candidate references invalid.
*/
DECUMA_HWR_PRIVATE void decumaQCHRepair(const DECUMA_QCH* pQCH, const DECUMA_QCH_CANDREF** ppCandRef, DECUMA_UINT32 nNewKey);

/*
    Verifies that pCandRef have a key that is consistent with its cand, using
	the nKeyOffset provided upon init. Note that this functions requires that
	a proper nKeyOffset was provided.
*/
DECUMA_HWR_PRIVATE int decumaQCHVerifyCandRef(const DECUMA_QCH* pQCH, const DECUMA_QCH_CANDREF* pCandRef);

/*
    Writes the ranking of the candidates in pQCH to pRanking. The ranking
	are	represented	by indeces to the pCandBuf that was provided upon
	initialization. Note that the content of this index array can be
	view more as the inverted ranking, i.e. pCandBuf[pRanking[0]] will
	contain the first ranked candidate. The number of bytes allocated
	for pRanking must be t least 2 times the number	candidates in
	pQCH.
*/
DECUMA_HWR_PRIVATE int decumaQCHGetRanking(const DECUMA_QCH* pQCH, DECUMA_UINT16* pRanking);

/*
    Sorts the candidate array in pCandBuf (the buffer provided upon
	initialization) according to the candidate ranking. The pTmpCandMem
	is used for temporary storage of one candidate during this sorting.
	At least nCandSize (see decumaQCHInit()) bytes of memory must have
	been allocated for it. Note that this operation can be quite time-
	consuming if either the number of candidates are or the candidate
	size is large. Then it might be more optimal to retrieve 

	IMPORTANT NOTE:
	Makes all previously returned candidate references invalid.
*/
DECUMA_HWR_PRIVATE int decumaQCHApplyRanking(DECUMA_QCH* pQCH, void* pTmpCandMem);

#endif /* DECUMA_QCAND_HANDLER_H */
