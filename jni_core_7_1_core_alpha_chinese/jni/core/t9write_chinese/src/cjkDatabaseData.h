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

#pragma once

#ifndef CJK_DATABASE_DATA_H_
#define CJK_DATABASE_DATA_H_

#if defined(_DEBUG) && !defined(CJK_DATABASE_C)
#error Direct data access in debug mode only allowed from cjkDatabase.c
#endif

#include "cjkCompressedCharacter.h"

/** @addtogroup DOX_CJK_CCHAR
 *  @{
 */

/** Scratchpad used in the function @ref cjkDatabase */
struct _tagCJK_DB_SESSION {
	CJK_COMPRESSED_CHAR * pCCharDb;
};


#endif /* CJK_DATABASE_DATA_H_*/
