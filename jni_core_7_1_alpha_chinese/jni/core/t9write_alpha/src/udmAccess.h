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


#ifndef UDM_ACCESS_H_ajshrbglabg
#define UDM_ACCESS_H_ajshrbglabg


#ifndef UDMLIB_API
#define UDMLIB_API
#endif

#if defined (__cplusplus)
extern "C" {
#endif

/* udmIsValid
Returns 1 if the udm pointed to by pUDM is valid. Otherwise returns 0
*/
UDMLIB_API int udmIsValid(UDM_PTR pUDM);

/* udmGetByteSize
returns the number of bytes consumed by the UDM for the moment
*/
UDMLIB_API DECUMA_UINT32 udmGetByteSize( UDM_PTR pUDM);


#if defined (__cplusplus)
} /*extern "C" { */
#endif


#endif
