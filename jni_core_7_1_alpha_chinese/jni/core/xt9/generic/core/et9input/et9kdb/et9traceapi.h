/*******************************************************************************
;*******************************************************************************
;**                                                                           **
;**                  COPYRIGHT 2001-2011 NUANCE COMMUNICATIONS                **
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
;**     FileName: et9traceapi.h                                               **
;**                                                                           **
;**  Description: ET9 TRACE API Interface Header File.                        **
;**               Conforming to the development version of ET9                **
;**                                                                           **
;*******************************************************************************
;******************************************************************************/

#ifndef ET9TRACEAPI_H
#define ET9TRACEAPI_H 1

/*! \addtogroup et9trace Functions for XT9 Keyboard Trace Module
 * Trace input for generic XT9.
 * @{
 */

/* don't mangle the function name if compile under C++ */
#if defined(__cplusplus)
extern "C" {
#endif


#ifndef ET9_KDB_TRACE_MODULE
#error ET9_KDB_TRACE_MODULE not defined
#endif


#define ET9_TRACE_VER           "3.12"                      /**< \internal Current trace version. */


ET9STATUS ET9FARCALL ET9KDB_ProcessTrace(ET9KDBInfo             * const pKDBInfo,
                                         ET9WordSymbInfo        * const pWordSymbInfo,
                                         ET9TracePoint    const * const pPoints,
                                         const ET9UINT                  nPointCount,
                                         const ET9U8                    bCurrIndexInList,
                                         ET9SYMB                * const psFunctionKey);

ET9BOOL ET9FARCALL ET9KDB_IsAutoAcceptBeforeTrace(ET9KDBInfo             * const pKDBInfo,
                                                  ET9WordSymbInfo        * const pWordSymbInfo,
                                                  ET9TracePoint    const * const pPoints,
                                                  const ET9UINT                  nPointCount,
                                                  ET9BOOL                * const pbAddSpace);


/* End don't mangle the function name if compile under C++ */
#if defined (__cplusplus)
    }
#endif

/*! @} */
#endif /* #ifndef ET9API_H */
/* ----------------------------------< eof >--------------------------------- */

