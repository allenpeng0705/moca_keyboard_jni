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



#ifndef DECUMA_RUNTIME_MALLOC_H
#define DECUMA_RUNTIME_MALLOC_H

#include "decumaConfig.h"
#include "decumaRuntimeMallocData.h" /* Standard definition of DECUMA_MEM_FUNCTIONS */
#include "decumaStatus.h"

#ifdef DECUMA_DEBUG_MEMORY
#include "decumaDebugMemory.h"
#endif

/* Some convenience macros related to the run-time defined dynamic memory functions: */
#define Uses_Runtime_Malloc DECUMA_MEM_FUNCTIONS const * const pMemFunctions
#define Pass_Runtime_Malloc pMemFunctions

/* Pointer validation of the callback functions */
DECUMA_HWR_PRIVATE DECUMA_STATUS decumaRuntimeMallocValidateMemfunctions(DECUMA_MEM_FUNCTIONS const * const pMemFunctions);

#ifdef DECUMA_DEBUG_MEMORY 

/* Pass file and line strings as well */

#define decumaCalloc(n, s)            Debug_rcalloc  (MODID_HWRLIB, pMemFunctions, n, s, __FILE__, __LINE__)
#define decumaAlloc(s)                Debug_rmalloc  (MODID_HWRLIB, pMemFunctions, s,    __FILE__, __LINE__)
#define decumaFree(p)                 Debug_rfree    (MODID_HWRLIB, pMemFunctions, p)

#define decumaModCalloc(mod, n, s)    Debug_rcalloc  (mod, pMemFunctions, n, s, __FILE__, __LINE__)
#define decumaModAlloc(mod, s)        Debug_rmalloc  (mod, pMemFunctions, s,    __FILE__, __LINE__)
#define decumaModFree(mod, p)         Debug_rfree    (mod, pMemFunctions, p)


#else

/* Call malloc/calloc/free through the respective pointers */
#define decumaCalloc(elements, size)	((pMemFunctions)->pCalloc)( (elements), (size), (pMemFunctions)->pMemUserData)
#define decumaAlloc(size)				((pMemFunctions)->pMalloc)( (size), (pMemFunctions)->pMemUserData)
#define decumaFree(pointer)			    { ((pMemFunctions)->pFree)( (pointer), (pMemFunctions)->pMemUserData); pointer = NULL; }

#define decumaModCalloc(mod,n, s)      decumaCalloc(n,s)
#define decumaModAlloc(mod,s)          decumaAlloc(s)
#define decumaModFree(mod,p)           decumaFree(p)

#endif /* DECUMA_DEBUG_MEMORY */

#endif /* DECUMA_RUNTIME_MALLOC_H */
