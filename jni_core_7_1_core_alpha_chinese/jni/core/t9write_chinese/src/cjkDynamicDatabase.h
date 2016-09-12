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

#include "decuma_hwr_types.h"
#include "decumaRuntimeMallocData.h"

/* Create and manage a dynamic database suitable for dltlib.
 *
 * This API should be included in the common API, however, the structure is not
 * finalized yet so this is currently a separate support API.
 */

#ifndef DECUMA_DYNAMIC_TEMPLATE_DATABASE_H
#define DECUMA_DYNAMIC_TEMPLATE_DATABASE_H

/* Create a new dynamic database */
DECUMA_HWR_PRIVATE DECUMA_STATUS cjkCreateDynamicDatabase(DECUMA_DYNAMIC_DB_PTR * ppDynamicDB, const DECUMA_MEM_FUNCTIONS * pMemFunctions);

/* Add allograph to the dynamic database, with the specified character set.
 * Since the cjk.compression routine handles scaling/translation, we can add an unbounded curve (i.e. no need to pass constraints).
 */
DECUMA_HWR_PRIVATE DECUMA_STATUS cjkAddAllograph(      DECUMA_DYNAMIC_DB_PTR  * ppDynamicDB,
								 const DECUMA_CURVE         * pCurve, 
								 const DECUMA_UNICODE       * pUnicode, DECUMA_UINT32 nUnicodes, 
								 const DECUMA_CHARACTER_SET * pCharacterSet,
								 DECUMA_INT32 nBaseline, DECUMA_INT32 nHelpline,
								 int bGesture, int bInstantGesture,
								 const DECUMA_MEM_FUNCTIONS * pMemFunctions);

/* Check if the supplied template database is valid */
DECUMA_HWR_PRIVATE DECUMA_STATUS cjkDynamicDatabaseIsValid(DECUMA_DYNAMIC_DB_PTR pDynamicDB);

/* Get actual number of bytes used. All template data is stored contiguously, so this function can be used to serialize the database to some persistent storage. */ 
DECUMA_HWR_PRIVATE DECUMA_STATUS cjkGetDynamicDatabaseByteSize(DECUMA_DYNAMIC_DB_PTR pDynamicDB, DECUMA_UINT32 * pSize);

/* Actual freeing of the dynamic database */
DECUMA_HWR_PRIVATE void cjkDestroyDynamicDatabase(DECUMA_DYNAMIC_DB_PTR * ppDynamicDB, const DECUMA_MEM_FUNCTIONS * pMemFunctions);

#endif /* DECUMA_DYNAMIC_TEMPLATE_DATABASE_H */
