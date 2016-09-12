/*****************************************************************************
* COPYRIGHT ZI AND SUBJECT TO CONFIDENTIALITY RESTRICTIONS                   *
*                                                                            *
* This file is the confidential and proprietary property of Zi Corporation   *
* of Canada, Inc. ("Zi") or one of its affiliates.                           *
******************************************************************************/

#pragma once

#ifndef CJK_CONTEXT_H_
#define CJK_CONTEXT_H_

#include "cjkCommon.h"


/** @defgroup DOX_CJK_CONTEXT cjkContext
 *  @{
 */

/**
 * The type CJK_CONTEXT consists of the unicode of the previously
 * interpreted character, presently set as the latest interpretation. In the
 * future it should be set by gui. In addition to the previosly interpreted
 * unicode, two quotation counters are in the struct.
 *
 * The CJK_CONTEXT type consists of information of the surrounding characters,
 * i.e. the previous characters unicode and some information of quotation marks.
 */
typedef struct _tagCJK_CONTEXT {
   CJK_UNICHAR  previous_unichar;
   DECUMA_INT32 double_quotation_counter;
   DECUMA_INT32 single_quotation_counter;
} CJK_CONTEXT;



DECUMA_HWR_PRIVATE void         cjkContextInit(CJK_CONTEXT * const con);
DECUMA_HWR_PRIVATE CJK_UNICHAR  cjkContextGetPrevious(CJK_CONTEXT const * const con);
DECUMA_HWR_PRIVATE void         cjkContextSetPrevious(CJK_CONTEXT * const con, CJK_UNICHAR prev_uc);
DECUMA_HWR_PRIVATE DECUMA_INT32 cjkContextGetSingleQuoteCount(CJK_CONTEXT const * const con);
DECUMA_HWR_PRIVATE void         cjkContextSetSingleQuoteCount(CJK_CONTEXT * const con);
DECUMA_HWR_PRIVATE void         cjkContextIncSingleQuoteCount(CJK_CONTEXT * const con);
DECUMA_HWR_PRIVATE DECUMA_INT32 cjkContextGetDoubleQuoteCount(CJK_CONTEXT const * const con);
DECUMA_HWR_PRIVATE void         cjkContextSetDoubleQuoteCount(CJK_CONTEXT * const con);
DECUMA_HWR_PRIVATE void         cjkContextIncDoubleQuoteCount(CJK_CONTEXT * const con);
DECUMA_HWR_PRIVATE void         cjkContextResetPrevious(CJK_CONTEXT * const con);
DECUMA_HWR_PRIVATE void         cjkContextResetSingleQuoteCount(CJK_CONTEXT * const con);
DECUMA_HWR_PRIVATE void         cjkContextResetDoubleQuoteCount(CJK_CONTEXT * const con);

/** 
 * @returns true iff the context is to be considered. 
 * Presently false is returned if the unicode is
 * 0x0000, other unicodes could be handled as no context, e.g. full stop.
 */
DECUMA_HWR_PRIVATE CJK_BOOLEAN cjkContextHasPrevious(CJK_CONTEXT * const con);

/** @} */

#endif /* CJK_CONTEXT_H_ */
