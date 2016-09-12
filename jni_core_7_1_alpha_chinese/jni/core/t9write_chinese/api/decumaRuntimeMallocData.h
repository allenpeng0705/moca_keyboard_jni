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


/* Definition of the data types used for runtime-defined dynamic allocation 
 * functions 
 */

#ifndef DECUMA_RUNTIME_MALLOC_DATA_H
#define DECUMA_RUNTIME_MALLOC_DATA_H

#include <stddef.h>

typedef void * (*DECUMA_MALLOC_FUNC)( size_t size, void * pMemUserData );
typedef void * (*DECUMA_CALLOC_FUNC)( size_t nElements, size_t elementSize, void * pMemUserData );
typedef void   (*DECUMA_FREE_FUNC)  ( void * pMem, void * pMemUserData );

typedef struct {
	DECUMA_MALLOC_FUNC pMalloc;
	DECUMA_CALLOC_FUNC pCalloc;
	DECUMA_FREE_FUNC   pFree;
	void * pMemUserData; /* This will be passed as final argument to each function */
} DECUMA_MEM_FUNCTIONS;

#endif /* DECUMA_RUNTIME_MALLOC_DATA_H */
