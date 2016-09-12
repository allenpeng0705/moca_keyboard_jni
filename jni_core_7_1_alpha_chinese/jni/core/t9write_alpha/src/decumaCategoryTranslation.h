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


#ifndef DECUMA_CATEGORY_TRANSLATION_H
#define DECUMA_CATEGORY_TRANSLATION_H

#ifdef DECUMA_MANGLE
#include "mangle.h"
#endif

#include "decumaConfig.h"
#include "decumaBasicTypes.h"
#include "decumaCharacterSetType.h"
#include "decumaCategoryType.h"
#include "decumaCatCombDataType.h"
#include "decumaCategoryTableType.h"
#include "decumaStatus.h"

DECUMA_HWR_PRIVATE int isAtomInSymbolCategory( DECUMA_UINT32 atomicCategory,
					 DECUMA_UINT32 combinationCategory);

DECUMA_HWR_PRIVATE int getAtomicCategories( DECUMA_UINT32 combinationCategory, DECUMA_UINT32 * pAtomicCategories,
	DECUMA_UINT8 nMaxAtomicCategories);


DECUMA_HWR_PRIVATE int getAtomicLanguages( DECUMA_UINT32 combinationLanguage, DECUMA_UINT32 * pAtomicLanguages,
	DECUMA_UINT8 nMaxAtomicLanguages);

/*Functions to see if a category or language belongs to the standard set. */
DECUMA_HWR_PRIVATE int isStandardSymbolCategory( DECUMA_UINT32 cat);
DECUMA_HWR_PRIVATE int isStandardLanguage( DECUMA_UINT32 lang);

DECUMA_HWR_PRIVATE DECUMA_STATUS checkCharacterSetValidity(const DECUMA_CHARACTER_SET * pCharacterSet,
	int bAcceptReserved);

DECUMA_HWR_PRIVATE DECUMA_STATUS translateToCategoryStructs(
	const DECUMA_CHARACTER_SET * pCharacterSet, 
	CATEGORY_TABLE_PTR pCatTable, 
	CATEGORY_TABLE_PTR pUDMCatTable,
	CATEGORY * pCat, CATEGORY * pUDMCat);

DECUMA_HWR_PRIVATE DECUMA_STATUS addUnfamiliarAtomicCategories(const DECUMA_CHARACTER_SET * pCharacterSet,
	DECUMA_UINT32 * pSymbolCatIdMap, DECUMA_UINT32 * pLanguageIdMap);

DECUMA_HWR_PRIVATE void translateIdToCatBit(const DECUMA_UINT32 DECUMA_DB_STORAGE * pCatIdMap, DECUMA_UINT32 id, CATEGORY_MASK * pCatMask );
	
	
#endif /* DECUMA_CATEGORY_TRANSLATION_H */
