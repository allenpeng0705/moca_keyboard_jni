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
  decumaHashHandler objects.

\******************************************************************/

#ifndef DECUMA_HASH_HANDLER_DATA_H
#define DECUMA_HASH_HANDLER_DATA_H

#if defined(_DEBUG) && !defined(DECUMA_HASH_HANDLER_C)
#error Direct data access in debug mode only allowed from decumaHashHandler.c
#endif

#include "decumaBasicTypes.h"
#include "decumaHashHandler.h"

typedef struct _DECUMA_HH_ELEM DECUMA_HH_ELEM;

struct _DECUMA_HH_ELEM
{
	DECUMA_UINT32    nKey;
	const void*      pData;

	DECUMA_HH_ELEM*  pNext;
};

struct _DECUMA_HH
{
	DECUMA_HH_TABLESIZE tableSize;  /*Small, medium or large */
	DECUMA_HH_ELEM**  ppTable;    /* Array */

	DECUMA_HH_ELEM*    pFreeElem; /* Linked list */
	DECUMA_HH_ELEM*    pExtraElemBufs[20]; /*Array of pointers */
	int	               nExtraElemBufs; 
};

struct _DECUMA_HH_ITERATOR
{
	const DECUMA_HH * pHH;
	DECUMA_UINT32 nTableLen;
	DECUMA_UINT32 nTableIdx;
	DECUMA_HH_ELEMREF elemRef;

};
#endif /*DECUMA_HASH_HANDLER_DATA_H */
