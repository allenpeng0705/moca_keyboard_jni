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

  This file provides the interface to operate on DECUMA_HH object.

  DECUMA_HH provides basic, but very fast, operations on a list of
  elements.

  - Very fast element search (table lookup + short linear search)
  - Very fast element add (no search, no memmove)
  - Very fast element removal (about one search + one add in time)
  
  An object is initialized with memory allocated for it to handle
  a maximum number of list elements. An element to be added the
  is provided as a const data pointer together with an element key.
  The element keys enable fast operations. The data pointers of
  added elements are assumed to be unique. Different elements may
  have the same key, but if many elements in the list share the
  same key the operations on these elements will be slower.
  
  The elements are retrieved through element references.

\******************************************************************/

#ifndef DECUMA_HASH_HANDLER_H
#define DECUMA_HASH_HANDLER_H

typedef struct _DECUMA_HH              DECUMA_HH;
typedef const struct _DECUMA_HH_ELEM*  DECUMA_HH_ELEMREF;
typedef struct _DECUMA_HH_ITERATOR     DECUMA_HH_ITERATOR;

#include "decumaBasicTypes.h"
#include "decumaConfig.h"


/* Provide three possible hash table lengths for optimal speed / memory */
/* performance depending on the maximum number of list elements. */
/* */
/* Table   Size    Efficiency limit */
/* Small     4 kB    1000 elements */
/* Medium   33 kB   10000 elements */
/* Large   512 kB  100000 elements */
/* */
/* Oversized table has some negative speed effect as well due to unnecessary size */
/* overhead, but increased collision from undersizing should have more severe */
/* speed effect. */
/* */
typedef enum _DECUMA_HH_TABLESIZE
{
	decumaHHSizeSmall=0,
	decumaHHSizeMedium,
	decumaHHSizeLarge,

	decumaHHNumberOfSizes

} DECUMA_HH_TABLESIZE;

#define DECUMA_HH_MAX_EXTRA_ELEM_BUFS 20

#if !defined(_DEBUG)
#include "decumaHashHandlerMacros.h"
#endif

/*
	Returns the size in bytes of a DECUMA_HH object handling
	nMaxElems elements.
*/
DECUMA_HWR_PRIVATE int decumaHHGetSize(unsigned int nMaxElems, DECUMA_HH_TABLESIZE tableSize);

/*
	Initializes a DECUMA_HH object for handling nMaxElems
	elements. At least the number of bytes returned by
	decumaHHGetSize(nMaxElems) must have been allocated
	for pHH.
*/
DECUMA_HWR_PRIVATE void decumaHHInit(DECUMA_HH* pHH, unsigned int nMaxElems, DECUMA_HH_TABLESIZE tableSize);

/*
	Attaches a new allocated buffer to the hash handler where 
	elems can be stored. This is best used when you have tried to
	call decumaHHAdd but the function returned 0, i.e. out of buffer space.
	Then allocate a new elem buffer with size taken from 
	decumaHHGetNewElemBufferSize() and attach to the hash handler via this function.
	Remember to free the buffer when the hash handler is destroyed.

	Note that no more than DECUMA_HH_MAX_EXTRA_ELEM_BUFS can be attached.
	Returns 0 if the buffer is not attached due to too many buffers
*/
DECUMA_HWR_PRIVATE void * decumaHHAttachExtraElemBuffer(DECUMA_HH* pHH, void * pBuf, unsigned int nElems);

/*
	Returns the size needed in bytes for a new buffer that can hold
	nElems elements.
*/
DECUMA_HWR_PRIVATE int decumaHHGetExtraElemBufferSize(unsigned int nElems);

/*
	Returns the number of attached extra elem buffers
*/
DECUMA_HWR_PRIVATE int decumaHHGetNExtraElemBuffers(DECUMA_HH* pHH);

/*
	Returns the attached extra elem buffer with index nBufIdx
*/
DECUMA_HWR_PRIVATE void * decumaHHGetExtraElemBuffer(DECUMA_HH* pHH,int nBufIdx);


DECUMA_HWR_PRIVATE DECUMA_UINT32 decumaHHCreateKey(DECUMA_HH* pHH, DECUMA_UINT32 nValue);


/*
	Adds an element. Returns the reference to the added
	element	if successful or 0 if pHH is full.
*/
DECUMA_HWR_PRIVATE DECUMA_HH_ELEMREF decumaHHAdd(DECUMA_HH* pHH, DECUMA_UINT32 nKey, const void* pData);

/*
	Removes an element.
*/
DECUMA_HWR_PRIVATE void decumaHHRemove(DECUMA_HH* pHH, DECUMA_HH_ELEMREF elemRef);

/*
	Returns the reference to an element that was added with key	nKey
	or 0 if none is found. All other elements that were added with
	key nKey can be found by iteratively calling decumaHHFindNextKey.
*/
DECUMA_HWR_PRIVATE DECUMA_HH_ELEMREF decumaHHFindKey(const DECUMA_HH* pHH, DECUMA_UINT32 nKey);

/*
	Returns the reference to another element that was added with the
	same key as	the element of the provided element reference or 0 if
	none is	found. If starting an iteration of calls to this function
	with the reference returned from decumaHHFindKey for key nKey all
	elements that were added with key nKey are found exactly one time
	each.
*/
DECUMA_HWR_PRIVATE DECUMA_HH_ELEMREF decumaHHFindNextKey(DECUMA_HH_ELEMREF elemRef);

/*
	Returns the reference to an element that was added with key	nKey
	and data pointer pData or 0 if none is found. Although the element
	is uniquely defined by the data pointer the key is needed for fast
	search.
*/
DECUMA_HWR_PRIVATE DECUMA_HH_ELEMREF decumaHHFindData(const DECUMA_HH* pHH, DECUMA_UINT32 nKey, const void* pData);

/* 
	Enumerate all data in the table - with pPrev = NULL return the first element, when return value == NULL the last element has been reached.
	NOTE:This function 
DECUMA_HH_ELEMREF decumaHHEnumerateEntries(const DECUMA_HH* pHH, DECUMA_HH_ELEMREF pPrev);
 */


DECUMA_HWR_PRIVATE int decumaHHIteratorGetSize(void);

DECUMA_HWR_PRIVATE void decumaHHIteratorInit(DECUMA_HH_ITERATOR * pHHIterator,const DECUMA_HH* pHH);

DECUMA_HWR_PRIVATE DECUMA_HH_ELEMREF decumaHHIteratorNext(DECUMA_HH_ITERATOR * pHHIterator);


#ifdef _DEBUG
/*
	Returns the key provided when adding the element of the provided
	element	reference.
*/
DECUMA_HWR_PRIVATE DECUMA_UINT32 decumaHHGetKey(DECUMA_HH_ELEMREF elemRef);

/*
	Returns the data pointer provided when adding the element of the
	provided element reference.
*/
DECUMA_HWR_PRIVATE void* decumaHHGetData(DECUMA_HH_ELEMREF elemRef);
#endif


#ifdef HASH_HANDLER_STATISTICS
DECUMA_HWR_PRIVATE void decumaHHPrintStats(const DECUMA_HH* pHH,const char * hashName);
#endif /*HASH_HANDLER_STATISTICS */

#endif /* DECUMA_HASH_HANDLER_H */
