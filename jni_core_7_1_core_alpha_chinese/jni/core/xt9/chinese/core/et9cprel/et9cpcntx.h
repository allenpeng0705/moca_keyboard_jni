/*******************************************************************************
;*******************************************************************************
;**                                                                           **
;**                    COPYRIGHT 2004-2011 NUANCE COMMUNICATIONS              **
;**                                                                           **
;**               NUANCE COMMUNICATIONS PROPRIETARY INFORMATION               **
;**                                                                           **
;**     This software is supplied under the terms of a license agreement      **
;**     or non-disclosure agreement with Nuance Communications and may not    **
;**     be copied or disclosed except in accordance with the terms of that    **
;**     agreement.                                                            **
;**                                                                           **
;**                                                                           **
;*******************************************************************************
;**                                                                           **
;**     FileName: et9cpcntx.h                                                 **
;**                                                                           **
;**  Description: Chinese XT9 context prediction module header file.          **
;**               Conforming to the development version of Chinese XT9.       **
;**                                                                           **
;*******************************************************************************
;******************************************************************************/

#ifndef ET9CPCNTX_H
#define ET9CPCNTX_H 1

/* don't mangle the function name if compile under C++ */
#if defined(__cplusplus)
extern "C" {
#endif

/*----------------------------------------------------------------------------
 *  Define the context function prototypes.
 *----------------------------------------------------------------------------*/

void ET9FARCALL ET9_CP_UpdateContextBuf(ET9CPLingInfo *pET9CPLingInfo,
                                     ET9CPPhrase *psPhrase);

void ET9FARCALL ET9_CP_ClrContextBuf(ET9CPLingInfo *pET9CPLingInfo);

/* End don't mangle the function name if compile under C++ */
#if defined(__cplusplus)
    }
#endif

#endif  /* #ifndef ET9CPCNTX_H */

/* ----------------------------------< eof >--------------------------------- */
