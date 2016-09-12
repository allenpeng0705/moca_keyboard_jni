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

#include <stddef.h> /* Definition of NULL */
#include "udm.h"
#include "udmAccess.h"

/*These functions can be accessed from customer and engine */

UDMLIB_API int udmIsValid(UDM_PTR pExtUDM)
{
	UDM_HEADER_PTR pUDM = (UDM_HEADER_PTR) pExtUDM;

	return (VALID_DECUMA_BASIC_TYPES &&
		pUDM != NULL && pUDM->udmVersionNr == UDM_FORMAT_VERSION_NR &&
		pUDM->dbVersionNr == DATABASE_FORMAT_VERSION_NR);
}

UDMLIB_API DECUMA_UINT32 udmGetByteSize( UDM_PTR pExtUDM )
{
	UDM_HEADER_PTR pUDM = (UDM_HEADER_PTR) pExtUDM;

	return pUDM->udmSize;
}

