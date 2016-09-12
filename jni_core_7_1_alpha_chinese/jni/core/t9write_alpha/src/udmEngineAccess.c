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


#include "udm.h"
#include "udmAccess.h"
#include "decumaAssert.h"

UDM_HEADER_PTR udmGetDatabase(UDM_PTR pExtUDM)
{
	UDM_HEADER_PTR pUDM = (UDM_HEADER_PTR) pExtUDM;

	decumaAssert(udmIsValid(pExtUDM));

	return (UDM_HEADER_PTR) ((DECUMA_DB_ADDRESS) pUDM + pUDM->keyDBOffset);
}

DECUMA_UINT32 udmGetDatabaseVersionNr(UDM_PTR pExtUDM)
{
	UDM_HEADER_PTR pUDM = (UDM_HEADER_PTR) pExtUDM;

	return pUDM->dbVersionNr;
}


