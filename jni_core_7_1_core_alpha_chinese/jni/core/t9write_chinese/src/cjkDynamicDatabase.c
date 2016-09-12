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

/* This file provides implementations of the external functions as defined in 
 * cjkDynamicDatbase.h, and the internals defined in cjkDynamicDatabase_Macros.h
 */

#define CJK_DYNAMIC_DATABASE_C
#define CJK_SYSTEM_CODE

#include "dltConfig.h"

#include "cjk.h" /* Prototype for dltCreateCategoryMask() */
#include "cjkDynamicDatabase.h"
#include "cjkDynamicDatabase_Macros.h"
#include "cjkDynamicDatabase_Types.h"
#include "decumaRuntimeMalloc.h"
#include "decumaMemory.h"
#include "cjkCompressedCharacter.h"
#include "cjkCompressedCharacter_Macros.h"
#include "decumaAssert.h"

DECUMA_STATUS cjkCreateDynamicDatabase(DECUMA_DYNAMIC_DB_PTR * ppDynamicDB, const DECUMA_MEM_FUNCTIONS * pMemFunctions)
{
	struct _DECUMA_DYNAMIC_DB * pDynamicDB;
	decumaAssert(ppDynamicDB);
	decumaAssert(pMemFunctions);

	pDynamicDB = decumaCalloc(1, sizeof(struct _DECUMA_DYNAMIC_DB));
	
	if ( pDynamicDB == NULL )
		return decumaAllocationFailed;

	/* Initialize the header fields */
	pDynamicDB->MagicNumber = CJK_UDM_IMPLEMENTATION;
	pDynamicDB->nEntries = 0;
	
	*ppDynamicDB = pDynamicDB;

	return decumaNoError;
}

DECUMA_STATUS cjkAddAllograph( DECUMA_DYNAMIC_DB_PTR * ppDynamicDB,
								 const DECUMA_CURVE         * pCurve, 
								 const DECUMA_UNICODE       * pUnicode, DECUMA_UINT32 nUnicodes, 
								 const DECUMA_CHARACTER_SET * pCharacterSet,
								 DECUMA_INT32 nBaseline, DECUMA_INT32 nHelpline,
								 int bGesture, int bInstantGesture,
								 const DECUMA_MEM_FUNCTIONS * pMemFunctions)
{
	DECUMA_STATUS status;
	DECUMA_UINT32 categoryMask;
	struct _DECUMA_DYNAMIC_DB * pNewDynamicDB;

	decumaAssert(ppDynamicDB && *ppDynamicDB);
	decumaAssert(pUnicode);
	decumaAssert(pCharacterSet);
	decumaAssert(pCurve);
	decumaAssert(pMemFunctions);

	if  (nBaseline != 0 || nHelpline != 0 || bGesture != 0 || bInstantGesture != 0)
		return decumaUnsupportedParameterValue;

	status = dltCreateCategoryMask(pCharacterSet, &categoryMask);

	if ( status != decumaNoError )
		return status;

	/* Check that the string is all right (dltlib only allows single-unicode strings for the moment) */
	if ( nUnicodes < 1 || pUnicode[0] == 0 )
		return decumaEmptyString;

	if ( nUnicodes > 1 )
		return decumaTooLongString;

	/* Allocate new dynamic db with one more element */
	pNewDynamicDB = decumaCalloc(1, sizeof(struct _DECUMA_DYNAMIC_DB) + ( (*ppDynamicDB)->nEntries + 1) * sizeof(CJK_PECDATA) );

	if ( pNewDynamicDB == NULL)
		return decumaAllocationFailed;

	decumaMemcpy(pNewDynamicDB, *ppDynamicDB, sizeof(struct _DECUMA_DYNAMIC_DB) + ((*ppDynamicDB)->nEntries + 1) * sizeof(CJK_PECDATA));

	/* We might add 2 more entries (one sparse and one dense) so make sure there is room for 2 more items */
#if 0
	if ( (*ppDynamicDB)->nEntries + 1 >= (*ppDynamicDB)->maxEntries )
	{
		/* Try to reallocate the UDM */
		DECUMA_DYNAMIC_DB_PTR pOldDatabase = *ppDynamicDB;
		DECUMA_DYNAMIC_DB_PTR pNewDatabase;
		DECUMA_UINT32 oldSize;
		
		status = cjkGetDynamicDatabaseByteSize(pOldDatabase, &oldSize);
		
		if ( status != decumaNoError ) 
			return status;
		
		pNewDatabase = decumaCalloc(1, oldSize + CJK_UDM_INCREMENT * sizeof(CJK_PECDATA));
		if ( !pNewDatabase )
			return decumaAllocationFailed;
		
		decumaMemcpy(pNewDatabase, pOldDatabase, oldSize);
		pNewDatabase->maxEntries += CJK_UDM_INCREMENT;
		decumaFree(pOldDatabase);

		*ppDynamicDB = pNewDatabase;
	}
#endif

	/* Actually add a character to this UDM */
	{
		CJK_PECDATA * pPecData = &pNewDynamicDB->entries[pNewDynamicDB->nEntries];
		CJK_COMPRESSED_CHAR * tempCC;
		const CJK_COMPRESSED_CHAR_DATA * pCCData;
		
		/* Create a temporary session for use in the calls to dltCCharCompress */
		/* TODO Should we refactor dltCCharCompress() so that it no longer requires a SESSION (!) to access required scratch and output buffers? */
		CJK_ARC_SESSION * pArcSession = decumaAlloc(cjkArcSessionGetSize() + cjkCompressedCharacterGetScratchpadSize());
		CJK_CC_COMPRESS_SCRATCHPAD * pScratchpad = (CJK_CC_COMPRESS_SCRATCHPAD *)((char *)pArcSession + cjkArcSessionGetSize());

		if ( pArcSession == NULL || pScratchpad == NULL)
		{
			status = decumaAllocationFailed;
			goto decumaAddAllograph_cleanup;
		}

		/* Also need to init session to set pointers correctly, but avoid allocating the sampler */

		status = cjkArcSessionInit(pArcSession, pScratchpad);
		if (status != decumaNoError) {
			goto decumaAddAllograph_cleanup;
		}

		cjkArcSessionSetSamplingRule(pArcSession, categoryMask, pCurve->nArcs);

		/* Unlike the old implementation, don't call dltCCHarCompress() directly, but use its wrapper function dltCCharCompressChar() to get the proper sampling type */
		tempCC = cjkArcSessionGetCompressedChar(pArcSession);
		status = dltCCharCompressChar(tempCC, pCurve, pArcSession);

		if ( status != decumaNoError ) 
		{
			goto decumaAddAllograph_cleanup;
		}
		
		pPecData->unicode = pUnicode[0];
		pPecData->attrib = dltCCCompressGetAttribute(tempCC);
		pPecData->categoryMask = categoryMask;
		pCCData = dltCCCompressGetCCData(tempCC);
		decumaMemcpy(&pPecData->ccData[0], pCCData, pCCData[0]);

		/* Now we have a character in pPecData that is either sparse or dense sampled. 
		 * However, we might have to add another entry with the other sample type, so we
		 * don't increase the counter yet:
		 */
		
		/* TODO: This rule does not yet handle the case when first sample method was dense */
		if (cjkArcSessionGetSamplingRule(pArcSession) == ChineseNonHanSampling &&
			dltCCCompressGetNbrPoints(tempCC) <= LENGTHLIMIT + 2 && !dltCCCompressIsDense(tempCC)) 
		{
			/* Add another instance of this character with the 'other' sampling type */

			/* Number of entries is not yet updated, hence (pNewDynamicDB->nEntries + 2) below */
			struct _DECUMA_DYNAMIC_DB * pTempDynamicDB = decumaCalloc(1, sizeof(struct _DECUMA_DYNAMIC_DB) + (pNewDynamicDB->nEntries + 2) * sizeof(CJK_PECDATA) );

			if ( pTempDynamicDB == NULL)
			{
				status = decumaAllocationFailed;
				goto decumaAddAllograph_cleanup;
			}

			decumaMemcpy(pTempDynamicDB, pNewDynamicDB, sizeof(struct _DECUMA_DYNAMIC_DB) + (pNewDynamicDB->nEntries + 2) * sizeof(CJK_PECDATA) );
			decumaFree(pNewDynamicDB);

			pNewDynamicDB = pTempDynamicDB;
			pNewDynamicDB->nEntries += 1;

			/* nEntries is not yet updated, hence [pNewDynamicDB->nEntries + 1] */
			pPecData = &pNewDynamicDB->entries[pNewDynamicDB->nEntries + 1];

			dltCCCompressSetCCData(tempCC, &pPecData->ccData[0]);

			/* Has been sparse sampled now add dense sampled */
			cjkArcSessionSetSamplingRule(pArcSession, 0, pCurve->nArcs);
			
			status = dltCCharCompressChar(tempCC, pCurve, pArcSession);
	
			if ( status != decumaNoError ) 
			{
				goto decumaAddAllograph_cleanup;
			}

			pPecData->attrib = dltCCCompressGetAttribute(tempCC);
			pPecData->unicode = pUnicode[0];
			pPecData->categoryMask = categoryMask;

			pNewDynamicDB->nEntries += 1;
		}

		pNewDynamicDB->nEntries += 1;

decumaAddAllograph_cleanup:
		decumaFree(pArcSession);
	}

	if ( status == decumaNoError )
	{
		struct _DECUMA_DYNAMIC_DB * pToFree = (struct _DECUMA_DYNAMIC_DB *) *ppDynamicDB;
		decumaFree(pToFree);
		*ppDynamicDB = pNewDynamicDB;
	} else {
		decumaFree(pNewDynamicDB);
	}

	return status;
}

DECUMA_STATUS cjkGetDynamicDatabaseByteSize(DECUMA_DYNAMIC_DB_PTR pDynamicDB, DECUMA_UINT32 * pSize)
{
	decumaAssert(pDynamicDB);
	decumaAssert(pSize);
	
	*pSize = sizeof(struct _DECUMA_DYNAMIC_DB) + pDynamicDB->nEntries * sizeof(CJK_PECDATA);

	return decumaNoError;
}

void cjkDestroyDynamicDatabase(DECUMA_DYNAMIC_DB_PTR * ppDynamicDB, const DECUMA_MEM_FUNCTIONS * pMemFunctions)
{
	struct _DECUMA_DYNAMIC_DB * pToFree = (struct _DECUMA_DYNAMIC_DB *) *ppDynamicDB;
	decumaFree(pToFree);
}

DECUMA_STATUS cjkDynamicDatabaseIsValid(DECUMA_DYNAMIC_DB_PTR pDynamicDB)
{
	decumaAssert(pDynamicDB);

	if ( pDynamicDB->MagicNumber != CJK_UDM_IMPLEMENTATION )
		return decumaInvalidDatabase;

	return decumaNoError;
}
