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

#ifndef CJK_SESSION_TYPES_H_
#define CJK_SESSION_TYPES_H_

#include "dltConfig.h"

#include "cjk.h"

#include "cjkSession.h"
#include "cjkArcSession.h"
#include "cjkDatabase.h"
#include "cjkDatabaseFormat.h"
#include "cjkDatabaseFormat_Types.h"
#include "cjkBestList.h"
#include "cjkContext.h"
#include "cjkCoarseFeatures.h"
#include "cjkDatabaseCoarse_Types.h"

#include "decumaFeature.h"

/** @addtogroup DOX_CJK_SESSION
 *  @{
 */

/**
 * The session type
 */
struct _tagCJK_SESSION {

	/* Data needed for database access */
	DLTDB db;

	/* Session state */
	CJK_SESSION_STATE state;

	/* From DECUMA_SESSION */
	const DECUMA_SESSION_SETTINGS * pSessionSettings; /* Pointer to the session settings stored in DECUMA_SESSION object	*/
	const DECUMA_MEM_FUNCTIONS * pMemFunctions;
	const DECUMA_INTERRUPT_FUNCTIONS * pInterruptFunctions; /* Abort function callback */

	/* Current converted categories from session settings */
	DECUMA_UINT32 sessionCategories;

	/* The arc session */
	CJK_ARC_SESSION  * pArcSession;

	/* Global variables */
	DECUMA_INT32   boxybase;
	DECUMA_INT32   boxheight;
	DECUMA_INT32   boxxbase;
	DECUMA_INT32   boxwidth;

	/* Personal Categories */
	DECUMA_UINT32  nPersonalCategories;
	DECUMA_UINT8   pPersonalCategories[MAX_NBR_DB_IDX_BIT_CHARS];

	DECUMA_UINT16  global_usepersonalchar;
	void           * pVerificationAddress; /*! @< Stores the address of the object when initialized. Used for verification of initialization. */

	/* Coarse features -- TODO: Should be moved to CJK_COMPRESSED_CHAR and CJK_COARSE_SCRATCHPAD ? */
	DECUMA_FEATURE coarseInputFeatures[COARSE_NBR_FEATURES];
	DECUMA_INT32   nLeftRightSplitIndex;
	DECUMA_INT32   nLeftRightSplitScore;
	DECUMA_INT32   nTopDownSplitIndex;
	DECUMA_INT32   nTopDownSplitScore;

	/* CJK_CONTEXT */
	CJK_CONTEXT con;

	DLTDB_COARSE_SEARCH_RESULT clusterResult;

	/* Contains persistent dbinfo that needs to survive passing to other modules e.g. template info */
	CJK_DB_SESSION * pDbSession;
	/* cjkDbLookup */
	CJK_BESTLIST db_lookup_bl; /* static */

	/** Scratchpads */
	/** NOTE: The actual scratchpad will be located straight after the session struct
	 *        in memory */
	void * pScratchpad;
};

/** @} */

#endif /* CJK_SESSION_TYPES_H_ */
