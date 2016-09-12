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

#include "decumaRuntimeMalloc.h"
#include "decumaRuntimeMallocData.h"

DECUMA_STATUS decumaRuntimeMallocValidateMemfunctions(DECUMA_MEM_FUNCTIONS const * const pMemFunctions)
{
	if ( pMemFunctions == NULL )
		return decumaNullMemoryFunctions;

	if ( pMemFunctions->pCalloc == NULL )
		return decumaNullFunctionPointer;

	if ( pMemFunctions->pMalloc == NULL )
		return decumaNullFunctionPointer;

	if ( pMemFunctions->pFree == NULL )
		return decumaNullFunctionPointer;

	return decumaNoError;
}
