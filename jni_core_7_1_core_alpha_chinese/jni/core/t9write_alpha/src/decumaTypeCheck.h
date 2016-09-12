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


#ifndef DECUMA_TYPE_CHECK_H
#define DECUMA_TYPE_CHECK_H 1

#include "decumaBasicTypes.h"

#define BASIC_TYPES_CORRECT_DEFINED		((sizeof(DECUMA_UINT32) == 4) && (sizeof(DECUMA_INT32) == 4) && (sizeof(DECUMA_UINT16) == 2) && (sizeof(DECUMA_INT16) == 2) && (sizeof(DECUMA_UINT8) == 1) && (sizeof(DECUMA_INT8) == 1))

#endif

