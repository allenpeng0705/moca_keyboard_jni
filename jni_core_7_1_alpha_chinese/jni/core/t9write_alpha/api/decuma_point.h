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


/************************************************** Description ***\

  File: decuma_point.h

 The type DECUMA_POINT is used to send points to Decuma's HWR engines.
 The type can be changed to suit the customers needs, but must contain
 the members x and y;

\******************************************************************/

#ifndef decuma_point_h
#define decuma_point_h


#include "decumaBasicTypes.h"

typedef DECUMA_INT16 DECUMA_COORD;

#define MAX_DECUMA_COORD ((DECUMA_INT16)  0x7FFF)
#define MIN_DECUMA_COORD ((DECUMA_INT16) -0x8000)

typedef struct tagDECUMA_POINT
{
	DECUMA_COORD x; /* X coordinate */
	DECUMA_COORD y; /* Y coordinate */
} DECUMA_POINT;


#endif /* ifndef decuma_point_h */
