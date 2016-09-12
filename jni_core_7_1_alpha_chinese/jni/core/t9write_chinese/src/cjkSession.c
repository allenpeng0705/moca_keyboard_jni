/*****************************************************************************
* COPYRIGHT ZI AND SUBJECT TO CONFIDENTIALITY RESTRICTIONS                   *
*                                                                            *
* This file is the confidential and proprietary property of Zi Corporation   *
* of Canada, Inc. ("Zi") or one of its affiliates.                           *
******************************************************************************/

#include "cjkSession_Types.h"
#include "cjkDatabase.h"
#include "decumaMemory.h"

DECUMA_UINT32 cjkSessionGetSize(void)
{
	return sizeof(CJK_SESSION);
}


DECUMA_UINT32 cjkSessionGetScratchpadSize(void) {
	DECUMA_UINT32 sz_scratchpad = cjkCoarseFeaturesGetScratchpadSize();
	sz_scratchpad = MAX(sz_scratchpad, cjkCompressedCharacterDTWDistGetScratchpadSize());
	sz_scratchpad = MAX(sz_scratchpad, cjkDbCoarseGetScratchpadSize());
	sz_scratchpad = MAX(sz_scratchpad, cjkCompressedCharacterGetScratchpadSize());
	return sz_scratchpad;
}

DECUMA_STATUS 
cjkSessionInit(CJK_SESSION  * pSession, 
			   DECUMA_SESSION_SETTINGS * pSessionSettings,
			   const DECUMA_MEM_FUNCTIONS * pMemFunctions)
{
	DECUMA_STATIC_DB_PTR pDb;

	decumaAssert(pSession);
	decumaAssert(pSessionSettings);

	if (CJK_SESSION_IS_INITIALIZED(pSession)) {
		return decumaSessionAlreadyInitialized;
	}
	pDb = pSessionSettings->pStaticDB;

	/* OBServe that this size includes the size of the arc session */
	decumaMemset(pSession, 0, cjkGetSessionSize());

	pSession->pSessionSettings = pSessionSettings;

	pSession->pMemFunctions    = pMemFunctions;
	pSession->pArcSession = (CJK_ARC_SESSION *) &pSession[1];
	pSession->pDbSession = (CJK_DB_SESSION *)((char*)pSession + cjkSessionGetSize() + cjkArcSessionGetSize());
	pSession->pScratchpad = (void *)((char*)pSession + cjkSessionGetSize() + cjkArcSessionGetSize() + cjkDbGetSessionSize());


	cjkDbInitDbSession(pSession->pDbSession);

	pSession->state = STATE_NORMAL;
	

	if (pDb) {
		DECUMA_STATUS status = cjkDbInit(&pSession->db, pDb);

		if (status != decumaNoError) {
			return status;
		}
	}
	else {
		DECUMA_STATUS status = cjkDbInitDefault(&pSession->db);
		decumaAssert(status == decumaNoError);

		if (status != decumaNoError) {
			return status;
		}
	}

	/* TODO This does not look good. Fields in the database binary are REQUIRED to be equal to the constants? Then why are they needed in the DLTDB struct?! 
	 *      These asserts are adapted from a previous 'if' statement that returned decumaInvalidDatabase; reformatted to more clearly show the badness.
	 */
	decumaAssert(pSession->db.maxnstr == MAXNSTRK);
	decumaAssert(pSession->db.maxnptsperchar == MAXPNTSPERCHAR);
	decumaAssert(pSession->db.maxnptsperstr == MAXPNTSPERSTR);
	decumaAssert(pSession->db.latinlimit == LATINLIMIT);
	

	/* ----- Coarse features */
	pSession->nLeftRightSplitIndex = -1;
	pSession->nLeftRightSplitScore = 0;
	pSession->nTopDownSplitIndex = -1;
	pSession->nTopDownSplitScore = 0;
	decumaMemset(pSession->coarseInputFeatures, -1, COARSE_NBR_FEATURES*sizeof(pSession->coarseInputFeatures[0]));

	
	if ( pSessionSettings->pDynamicDB )
	{
		DECUMA_STATUS status = cjkValidateDynamicDatabase(pSessionSettings->pDynamicDB);
		if ( status != decumaNoError )
			return status;
	}

	/* ----- Global variables */
	pSession->sessionCategories = pSession->db.categoryMask;

	/* ----- No error 
	 * cjkSetCategories requires session to be initialized so uninitialize if that fails.
	 */
	CJK_SESSION_SET_INITIALIZED(pSession);

	/* Convert categories to cjk.categories and 
	   Make sure the categories are valid  */
	
	{
		DECUMA_STATUS status = cjkSetCategories( pSession, pSessionSettings );

		if (status != decumaNoError) {
			/* Uninitialize session again */
			pSession->pVerificationAddress = NULL;
			return status;
		}
	}


	return decumaNoError;
}

DECUMA_FEATURE * cjkSessionGetCoarseInputFeatures(CJK_SESSION * pSession) {
	return pSession->coarseInputFeatures;
}

/** 
 ** Set the category mask to be used for recognition to mask
 */

void cjkSessionSetCategoryMask(CJK_SESSION * pSession, DECUMA_UINT32 mask)
{
	pSession->sessionCategories = mask;
}
