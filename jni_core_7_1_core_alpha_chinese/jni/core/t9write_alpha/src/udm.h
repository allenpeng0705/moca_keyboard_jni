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


#ifndef UDM_H_sdfo9o978ujdfdsggsd66tggyhnjmk
#define UDM_H_sdfo9o978ujdfdsggsd66tggyhnjmk

#ifdef DECUMA_MANGLE
#include "mangle.h"
#endif

#include "databaseFormat.h"
#include "decuma_point.h"
#include "decumaUnicodeTypes.h"
#include "decumaBasicTypes.h"
#include "decumaStorageSpecifiers.h"
#include "udmType.h"

/*This header files should only be accessed by the Decuma engines and not delivered */
/*to customers.// */

/* UDM
The UDM struct holds all the information about how a user
have customized a database. Additional allograph, eclipsed
allographs, etc are stored here.
*/
#define UDM_FORMAT_VERSION_NR (1) /*Change this when UDM format is changed */

typedef const struct tagUDM_HEADER
{
	DECUMA_UINT32 udmVersionNr;		
	DECUMA_UINT32 udmSize;
	DECUMA_UINT32 dbVersionNr;			/* If this variable is changed or moved Change the error handling in mcrBeginSession. */
	DECUMA_UINT32 keyDBOffset;
	/*INT32 dbDistBaseLine2HelpLine; */

	/*/ ...Alot of magic stuff; */
} DECUMA_DB_STORAGE *UDM_HEADER_PTR; /*This is the datatype behind the datatype UDM in udmlib.h */

DECUMA_HWR_PRIVATE UDM_HEADER_PTR udmGetDatabase(UDM_PTR pUDM);

DECUMA_HWR_PRIVATE DECUMA_UINT32 udmGetDatabaseVersionNr(UDM_PTR pUDM);

#endif
