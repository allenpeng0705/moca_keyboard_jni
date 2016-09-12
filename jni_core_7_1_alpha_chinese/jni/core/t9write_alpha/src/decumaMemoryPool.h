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


#ifdef __cplusplus
extern "C" {
#endif

#ifndef DECUMAMEMORYPOOL_H
#define DECUMAMEMORYPOOL_H

#include "decumaConfig.h"
#include "decumaDataTypes.h"
#include "decumaRuntimeMalloc.h"
#include "decumaModid.h"

/** @defgroup DOX_GRP_MEM_POOL_API Memory Pool API
 *  @ingroup DOX_GRP_MEM
 *  @{
 *
 *  Fast allocation from pools of memory with predefined capacities.
 *
 *  @b Header: <tt>decumaMemoryPool.h</tt>
 */

/** @name Memory Pool 
 *  @{
 */

/**
  * A <code>TMemoryPoolLayout</code> defines the layout of the memory (?!)
  * i.e. the desired size and multiplicity of memory blocks contained
  * within the pool.
  *
  * Example:
  * @code 
  * TMemoryPoolLayout myLayout[] = {{1024, 10}, {2048, 20}}; 
  * @endcode
  *
  * Would define 10 blocks with a 1024 byte capacity, and 20 blocks with
  * a 2048 byte capacity.
  */
typedef struct {
	DECUMA_UINT32 size;
	DECUMA_UINT32 nElements;
} TMemoryPoolLayout;

/** Memory pool type
 */
typedef struct _TMemoryPool TMemoryPool;

/**
  * Calculates and returns the number of bytes required in order to
  * create a memory pool that can accommodate an arbitrary @ref TMemoryPoolLayout.
  *
  * @param pLayout a pointer to a list of @ref TMemoryPoolLayout structures
  * @param nLayouts the number of structures in the list
  * @return the number of bytes required in order to create a memory pool for the layout
  */
DECUMA_HWR_PRIVATE DECUMA_UINT32 MemoryPool_GetSize(TMemoryPoolLayout* pLayout, DECUMA_UINT32 nLayouts);

/**
  * Initializes a @ref TMemoryPool
  *
  *	@param pMemoryPool a pointer to the @ref TMemoryPool that is to be initialized. Note that
  *        at least @ref MemoryPool_GetSize (pLayout, nLayouts) bytes of memory
  *        must have been allocated for <code>pMemoryPool</code>.
  * @param pLayout a pointer to a list of @ref TMemoryPoolLayout structures
  * @param nLayouts the number of structures in the layout
  */
DECUMA_HWR_PRIVATE DECUMA_UINT32 MemoryPool_Init(TMemoryPool* pMemoryPool,
							  TMemoryPoolLayout* pLayout, 
							  DECUMA_UINT32 nLayouts);


#ifdef DECUMA_DEBUG_MEMORYPOOL
#define MEMORYPOOL_ALLOC(pMemoryPool, size, bSpillOver) \
	MemoryPool_Alloc(pMemoryPool, size, bSpillOver, __FILE__, __LINE__)
#else
#define MEMORYPOOL_ALLOC(pMemoryPool, size, bSpillOver) \
	MemoryPool_Alloc(pMemoryPool, size, bSpillOver)
#endif

/**
  * Allocate at least <code>size</code> bytes from the memory pool pointed to by
  * <code>pMemoryPool</code>. Normally an attempt is made to allocate memory from
  * the part of the pool that holds the elements specified by that layout which
  * defines elements of a size that is equal to, or larger but closest to, <code>size</code>.
  * If no available element was found then allocation fails. <code>bSpillOver</code> can
  * be used in order to change this behaviour so that an attempt is made to try and
  * allocate in the next available layout if the most suitable had no available elements.
  *
  * @param pMemoryPool the memory pool from which to allocate memory
  * @param size the number of bytes to allocate
  * @param bSpillOver if equal to zero then the allocation will fail immediately if an
  *        available element was not found in the most sutiable layout. If not equal
  *        to zero then allocation will fail only if no layout contained an element
  *        that could satisfy the allocation.
  * @return a pointer to the allocated memory if the allocation was successfull, otherwise
  *         zero
  */

DECUMA_HWR_PRIVATE void* MemoryPool_Alloc(TMemoryPool* pMemoryPool, DECUMA_UINT32 size,
	DECUMA_UINT32 bSpillOver
#ifdef DECUMA_DEBUG_MEMORYPOOL
	, const char * pFile, int line
#endif
	);

/**
  * Frees allocated memory by returning it to the memory pool.
  *
  * @param pMemoryPool the pool from which the memory was allocated
  * @param pBase a pointer to the allocated memory
  */
DECUMA_HWR_PRIVATE void MemoryPool_Free(TMemoryPool* pMemoryPool, void* pBase);

/** @}
 */





/** @name Memory Pool Handler
 *  @{
 */

/** Memory pool handler type
 */
typedef struct _TMemoryPoolHandler TMemoryPoolHandler;

/**
 * @returns The size of a @ref TMemoryPoolHandler object
 */
DECUMA_HWR_PRIVATE DECUMA_UINT32 MemoryPoolHandler_GetSize(void);

/**
 * Initialize a TMemoryPoolHandler object
 * @param modid                The @ref MODULE_ID of the module owning the handler
 * @param pMemoryPoolHandler   The @ref TMemoryPoolHandler to initialize
 */
DECUMA_HWR_PRIVATE void MemoryPoolHandler_Init(MODULE_ID modid, TMemoryPoolHandler* pMemoryPoolHandler);

/**
 * Destroy a TMemoryPoolHandler object
 * @param pMemoryPoolHandler   The @ref TMemoryPoolHandler to destroy
 */
DECUMA_HWR_PRIVATE void MemoryPoolHandler_Destroy(TMemoryPoolHandler* pMemoryPoolHandler, Uses_Runtime_Malloc);

/**
 * Add a pool to the handler.
 * @param pMemoryPoolHandler   The @ref TMemoryPoolHandler to add a pool to
 * @param pMemoryPool          The @ref TMemoryPool to add
 *
 * @returns 1 if allocation error
 *
 * @b NOTE: Allocates (very little) memory
 */
DECUMA_HWR_PRIVATE DECUMA_UINT32 MemoryPoolHandler_AddPool(TMemoryPoolHandler* pMemoryPoolHandler,
                                        TMemoryPool * pMemoryPool,
										Uses_Runtime_Malloc);



#ifdef DECUMA_DEBUG_MEMORYPOOL
#define MEMORYPOOLHANDLER_ALLOC(pMemoryPoolHandler, size, bSpillOver) \
	MemoryPoolHandler_Alloc(pMemoryPoolHandler, size, bSpillOver, __FILE__, __LINE__)
#else
#define MEMORYPOOLHANDLER_ALLOC(pMemoryPoolHandler, size, bSpillOver) \
	MemoryPoolHandler_Alloc(pMemoryPoolHandler, size, bSpillOver)
#endif


/**
 * Allocate memory from a memory pool selected by this handler
 *
 * @param pMemoryPoolHandler   The @ref TMemoryPoolHandler that the pool will be selected from
 * @param size                 The number of bytes to allocate
 * @param bSpillOver           If zero then the allocation will fail immediately if an available element 
 *                             was not found in the most sutiable layout. If non-zero then 
 *                             the allocation will fail only if no layout contained an element that could 
 *                             satisfy the allocation.
 *
 * @returns Pointer to allocated memory
 */
DECUMA_HWR_PRIVATE void* MemoryPoolHandler_Alloc(TMemoryPoolHandler* pMemoryPoolHandler, DECUMA_UINT32 size,
	DECUMA_UINT32 bSpillOver
#ifdef DECUMA_DEBUG_MEMORYPOOL
	,const char * pFile, int line
#endif
	);

/**
 * Free memory allocated by a pool managed by this handler
 *
 * @param pMemoryPoolHandler   The @ref TMemoryPoolHandler that allocated the memory
 * @param pBase                Pointer to the memory to free
 */
DECUMA_HWR_PRIVATE void MemoryPoolHandler_Free(TMemoryPoolHandler* pMemoryPoolHandler, void* pBase);

/** @}
 */

/** @}
 */


#endif /*DECUMAMEMORYPOOL_H */

#ifdef __cplusplus
}
#endif
