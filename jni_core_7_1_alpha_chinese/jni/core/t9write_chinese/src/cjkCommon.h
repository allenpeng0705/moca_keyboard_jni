/*****************************************************************************
* COPYRIGHT ZI AND SUBJECT TO CONFIDENTIALITY RESTRICTIONS                   *
*                                                                            *
* This file is the confidential and proprietary property of Zi Corporation   *
* of Canada, Inc. ("Zi") or one of its affiliates.                           *
******************************************************************************/

#pragma once

#ifndef CJK_COMMON_H_
#define CJK_COMMON_H_



/* -------------------------------------------------------------------------
 * Common includes
 * ------------------------------------------------------------------------- */

#include "decumaConfig.h"
#include "decumaAssert.h"
#include "cjkTypes.h"

#ifdef MANGLE
#include "mangle.h"
#endif


/* -------------------------------------------------------------------------
 * Platform dependent stuff
 * ------------------------------------------------------------------------- */


#ifdef ONPALM_5
#ifndef ONPALM_ARMLET
#include <MemoryMgr.h>
#endif
#endif


/* -------------------------------------------------------------------------
 * Some common macros
 * ------------------------------------------------------------------------- */

#ifndef ONPALM_5

#define LIMIT_TO_RANGE(m_val, m_min, m_max) ((DECUMA_FEATURE)((m_val) < (m_min) ? (m_min) : ((m_val) > (m_max) ? (m_max) : (m_val))))

#ifndef MAX
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a, b) (((a) > (b)) ? (b) : (a))
#endif

#endif /* ONPALM_5 */

#define ABS(x)  ((x) >= 0 ? (x) : -(x))

#define SIGN(a) ((a) > 0 ? 1 : ((a) < 0 ? -1 : 0))


/**
 * The maximum number of characters in between two quotation marks. The
 * function @ref cjkDbLookup "remembers" the fact that a quotation mark has been
 * written, until @c MAX_QUOTATION_LENGTH characters have been written. This
 * is used to match right and left quotation marks.
 */
#define MAX_QUOTATION_LENGTH 30



#endif /* CJK_COMMON_H_ */
