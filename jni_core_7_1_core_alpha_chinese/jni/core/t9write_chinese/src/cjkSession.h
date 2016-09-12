/*****************************************************************************
* COPYRIGHT ZI AND SUBJECT TO CONFIDENTIALITY RESTRICTIONS                   *
*                                                                            *
* This file is the confidential and proprietary property of Zi Corporation   *
* of Canada, Inc. ("Zi") or one of its affiliates.                           *
******************************************************************************/

#pragma once

#ifndef CJK_SESSION_H_
#define CJK_SESSION_H_

#include "dltConfig.h"
#include "decuma_hwr_types.h"

#include "cjkDatabaseLimits.h"
#include "cjkCommon.h"
#include "decumaStatus.h"
#include "decumaFeature.h"

/** @defgroup DOX_CJK_SESSION cjkSession
 *  @{
 */

#define CJK_SESSION_IS_INITIALIZED(m_pSession)  ((m_pSession)->pVerificationAddress == (m_pSession))
#define CJK_SESSION_SET_INITIALIZED(m_pSession) ((m_pSession)->pVerificationAddress =  (m_pSession))


/** */
typedef enum _tagCJK_SESSION_STATE {
	STATE_NORMAL,
	STATE_SPECIAL_CHECKS
} CJK_SESSION_STATE;


/** */
typedef struct _tagCJK_SESSION 
	CJK_SESSION;

/**
 * Gets the size of a @ref CJK_SESSION object.
 */

DECUMA_HWR_PRIVATE DECUMA_UINT32 cjkSessionGetSize(void);

/**
 * Gets the size of the scratchpad required in a @ref CJK_SESSION object.
 */

DECUMA_HWR_PRIVATE DECUMA_UINT32 cjkSessionGetScratchpadSize(void);

/**
 * Initialize a @ref CJK_SESSION object.
 */
DECUMA_HWR_PRIVATE DECUMA_STATUS
cjkSessionInit(CJK_SESSION  * pSession, 
			   DECUMA_SESSION_SETTINGS * pSessionSettings,
			   const DECUMA_MEM_FUNCTIONS * pMemFunctions);

/** */

DECUMA_HWR_PRIVATE DECUMA_FEATURE * cjkSessionGetCoarseInputFeatures(CJK_SESSION * pSession);

/** 
 ** Set the category mask to be used for recognition to mask
 */

DECUMA_HWR_PRIVATE void cjkSessionSetCategoryMask(CJK_SESSION * pSession, DECUMA_UINT32 mask);

/** @} */

#endif /* CJK_SESSION_H_ */
