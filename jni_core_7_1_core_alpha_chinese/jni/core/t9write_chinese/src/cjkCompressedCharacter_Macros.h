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

#ifndef CJK_COMPRESSED_CHARACTER_MACROS_H_
#define CJK_COMPRESSED_CHARACTER_MACROS_H_

#if !defined(_DEBUG)

#define dltCCCompressGetNbrPoints	CJK_CCHAR_NPOINTS
#define dltCCCompressGetNbrStrokes  CJK_CCHAR_NSTROKES
#define dltCCCompressGetIndex			CJK_CCHAR_INDEX
#define dltCCCompressIsDense			CJK_CCHAR_IS_DENSE
#define dltCCCompressGetFeatureVal  CJK_CCHAR_GET_FEATURE
#define dltCCCompressGetXmax			CJK_CCHAR_GET_XMAX
#define dltCCCompressGetXmin			CJK_CCHAR_GET_XMIN
#define dltCCCompressGetYmax			CJK_CCHAR_GET_YMAX
#define dltCCCompressGetYmin			CJK_CCHAR_GET_YMIN
#define dltCCCompressSetNull			CJK_CC_SET_NULLCHAR

#endif

#if !defined(_DEBUG) || defined(CJK_CC_COMPRESS_C)

#include "cjkCompressedCharacter_Types.h"
#include "cjkStroke_Macros.h"

/** @addtogroup DOX_CJK_CCHAR
 *  @{
 */

/** @hideinitializer */
#define CJK_CCHAR_EXISTS(m_pCChar)            (CJK_CCHAR_NPOINTS(m_pCChar)!=0)
/** @hideinitializer */
#define CJK_CCHAR_NBYTES(m_pCChar)            (*(m_pCChar)->pCCData)
/** @hideinitializer */
#define CJK_CCHAR_NPOINTS(m_pCChar)           ((m_pCChar)->nPoints)
/** @hideinitializer */
#define CJK_CCHAR_NSTROKES(m_pCChar)          ((m_pCChar)->nStrokes)
/** @hideinitializer */
#define CJK_CCHAR_INDEX(m_pCChar)             ((m_pCChar)->index)
/** @hideinitializer */
#define CJK_CCHAR_GET_XMIN(m_pCChar)			 ((m_pCChar)->xmin)
/** @hideinitializer */
#define CJK_CCHAR_GET_XMAX(m_pCChar)			 ((m_pCChar)->xmax)
/** @hideinitializer */
#define CJK_CCHAR_GET_YMIN(m_pCChar)			 ((m_pCChar)->ymin)
/** @hideinitializer */
#define CJK_CCHAR_GET_YMAX(m_pCChar)			 ((m_pCChar)->ymax)
/** @hideinitializer */
#define CJK_CCHAR_IS_DENSE(m_pCChar)          CJK_ATTRIB_IS_DENSE((m_pCChar)->attrib)
/** @hideinitializer */
#define CJK_CCHAR_FREQUENCY(m_pCChar)         CJK_ATTRIB_FREQUENCY((m_pCChar)->attrib)


/** @hideinitializer */
#define CJK_CCHAR_GET_FEATURE(m_pCChar, m_nIndex) \
	((m_pCChar)->pFeatures[(m_nIndex)])

/** @hideinitializer */
#define CJK_CC_SET_NULLCHAR(m_pCChar)  { \
	m_pCChar->pCCData          = NULL;   \
	m_pCChar->nStrokes         = 0;      \
	m_pCChar->pOriginalStrokes = NULL;   \
	m_pCChar->nPoints          = 0;      \
	m_pCChar->index            = 0;      \
	m_pCChar->attrib           = 0; }

/** @} */

#endif

#endif /* CJK_COMPRESSED_CHARACTER_MACROS_H_ */
