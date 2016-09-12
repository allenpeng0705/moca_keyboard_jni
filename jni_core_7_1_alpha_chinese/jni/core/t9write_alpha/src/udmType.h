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


#ifndef UDM_TYPE_H_154154gl154g
#define UDM_TYPE_H_154154gl154g

#include "decumaStorageSpecifiers.h"

/* UDM
This is the hidden datastructure for a User Database Modifier

The UDM struct holds all the information about how a user
have customized a database (personal allographs). Additional
allograph, eclipsed allographs, etc are stored here. Requires
the package UDM, see udmlib.h for details.
*/

typedef const struct tagUDM DECUMA_DB_STORAGE * UDM_PTR;


#endif
