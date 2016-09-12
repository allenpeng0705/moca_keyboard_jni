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
;**     FileName: et9cpcj.h                                                   **
;**                                                                           **
;**  Description: Cang Jie input support                              .       **
;**                                                                           **
;*******************************************************************************
;******************************************************************************/

#ifndef __ET9CP_CANG_JIE_H__
#define __ET9CP_CANG_JIE_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifndef ET9CP_DISABLE_STROKE

#define SHOW_PARTIAL_FOR_ONE_KEY    1

#define CANG_JIE_LDB_VERSION        0
#define CANG_JIE_HEADER_SIZE        (4 + 4 + 27 * 2)
#define CANG_JIE_SUBTREE_TABLE_SIZE 27
#define MAX_CANG_JIE_CODE_LENGTH    5
#define CANG_JIE_UNICODE_SIZE       (MAX_CANG_JIE_CODE_LENGTH + sizeof(ET9U16))

ET9STATUS ET9FARCALL ET9_CP_CangJieBuildSpellings(ET9CPLingInfo * pET9CPLingInfo);
ET9STATUS ET9FARCALL ET9_CP_QuickCangJieBuildSpellings(ET9CPLingInfo * pET9CPLingInfo);

#endif  /* ET9CP_DISABLE_STROKE */

#ifdef __cplusplus
    }
#endif

#endif  /*  __ET9CP_CANG_JIE_H__  */

/* ----------------------------------< eof >--------------------------------- */
