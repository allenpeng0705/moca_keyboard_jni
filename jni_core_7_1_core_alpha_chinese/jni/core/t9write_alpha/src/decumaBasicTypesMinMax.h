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


#ifndef DECUMA_BASIC_TYPES_MIN_MAX
#define DECUMA_BASIC_TYPES_MIN_MAX

#define MAX_DECUMA_INT32  ((DECUMA_INT32)   0x7FFFFFFF)
#define MAX_DECUMA_UINT32 ((DECUMA_UINT32)  0xFFFFFFFF)
#define MAX_DECUMA_INT16  ((DECUMA_INT16)   0x7FFF)
#define MAX_DECUMA_UINT16 ((DECUMA_UINT16)  0xFFFF)
#define MAX_DECUMA_INT8   ((DECUMA_INT8)    0x7F)
#define MAX_DECUMA_UINT8  ((DECUMA_UINT8)   0xFF)

#define MIN_DECUMA_INT32  ((DECUMA_INT32)  -0x7FFFFFFF - 1)
#define MIN_DECUMA_UINT32 ((DECUMA_UINT32)  0x00000000)
#define MIN_DECUMA_INT16  ((DECUMA_INT16)  -0x8000)
#define MIN_DECUMA_UINT16 ((DECUMA_UINT16)  0x0000)
#define MIN_DECUMA_INT8   ((DECUMA_INT8)   -0x80)
#define MIN_DECUMA_UINT8  ((DECUMA_UINT8)   0x00)


#define VALID_DECUMA_BASIC_TYPES (\
	sizeof(DECUMA_UINT8) == 1 && sizeof(DECUMA_INT8) == 1 && \
	sizeof(DECUMA_UINT16) == 2 && sizeof(DECUMA_INT16) == 2 && \
	sizeof(DECUMA_UINT32) == 4 && sizeof(DECUMA_INT32) == 4 && \
	((DECUMA_UINT8) -1) > 0 && ((DECUMA_INT8) -1) < 0 && \
	((DECUMA_UINT16) -1) > 0 && ((DECUMA_INT16) -1) < 0 && \
	((DECUMA_UINT32) -1) > 0 && ((DECUMA_INT32) -1) < 0)

#endif
