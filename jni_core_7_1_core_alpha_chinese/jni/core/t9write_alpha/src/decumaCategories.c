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


/************************************************** Description ***\

      File: decumaCategories.c
 $Revision: 1.5 $
     $Date: 2011/02/14 11:41:14 $
   $Author: jianchun_meng $
 Copyright: Decuma AB

\************************************************* Header files ***/

#include "decumaCategories.h"
#include "decumaCategoriesHidden.h"
#include "decumaAssert.h"
#include "decumaString.h"
#include "decumaMemory.h"
#include "database.h"
#include "globalDefs.h"
#include "decumaCharacterSetType.h"
#include "decumaCategoryTranslation.h"
#include "decumaStorageSpecifiers.h" /* Definition of DECUMA_SPECIFIC_DB_STORAGE */

#ifdef __NITRO__
#include <nitro.h>
#include <nitro/version_begin.h>
static char id_string[] = SDK_MIDDLEWARE_STRING("Zi Corporation", "DCLIB");
#include <nitro/version_end.h>
#endif

#if !defined(DECUMA_SPECIFIC_DB_STORAGE) || !(DECUMA_SPECIFIC_DB_STORAGE == 1 || DECUMA_SPECIFIC_DB_STORAGE == 0)
#error DECUMA_SPECIFIC_DB_STORAGE must be defined to 1 or 0
#endif
#if DECUMA_SPECIFIC_DB_STORAGE == 0 /* The implementation of this module does not support specific DB storage */


/*************************************************** Local data ***/

#define UNREFERENCED_PARAM(P) ((void) P)

/************************************************** Global data ***/

/********************************************* Local prototypes ***/

static DECUMA_STATUS addCategoriesToTable(struct _CAT_TABLE * pCT, 
		const DECUMA_CHARACTER_SET * pCharacterSet);
		
static int addSymbolToTable(CAT_TABLE_PTR pCT, const DECUMA_UNICODE * pSymbol,
	const DECUMA_CHARACTER_SET * pCharacterSet,
	STATIC_DB_HEADER_PTR pStaticDB, int bOnlyLanguage);

static DECUMA_STATUS checkSymbolPointers(const DECUMA_UNICODE ** ppSymbols, int nSymbols);
static DECUMA_STATUS getCatTableStatus(CAT_TABLE_PTR pCT);


/* Valid category? */
#define CATEGORY_OK( cat ) ( (cat).symbolCat != 0 && (((cat).symbolCat & SYMBOL_CAT_RESERVED) == 0) && \
	(((cat).languageCat & LANGUAGE_CAT_RESERVED) == 0) )

static DECUMA_STATUS checkCategoriesToAdd(CAT_TABLE_PTR pCT, 
	const DECUMA_CHARACTER_SET * pCharacterSet);

/************************************************** Global code ***/

DECUMA_UINT32 dcGetTableSize(STATIC_DB_PTR pStaticDB)
{
	STATIC_DB_HEADER_PTR pDB = (STATIC_DB_HEADER_PTR) pStaticDB;

	if ( !pDB || databaseHasWrongFormat(pDB) ) {
		return 0; /* To indicate that the database is not correct. */
	}
	else {
		KEY_DB_HEADER_PTR pKeyDB = staticDBGetKeyDB(pDB);
		PROPERTY_DB_HEADER_PTR pPropDB = staticDBGetPropDB(pDB);

		return sizeof(struct _CAT_TABLE) +
			pKeyDB->nBaseSymbolTypes*sizeof(struct _CATEGORIES) +
			pPropDB->nDiacSymbols*sizeof(struct _CATEGORIES);
	}
}

DECUMA_STATUS dcInit(CATEGORY_TABLE_PTR pCategoryTable, STATIC_DB_PTR pStaticDB)
{
	struct _CAT_TABLE * pCT = (struct _CAT_TABLE *) pCategoryTable;
	STATIC_DB_HEADER_PTR pDB = (STATIC_DB_HEADER_PTR) pStaticDB;
	KEY_DB_HEADER_PTR pKeyDB;

#if defined(__NITRO__)
	SDK_USING_MIDDLEWARE(id_string);
#endif
	
	if ( pCT == NULL )
		return decumaNullPointer;

	if ( !pDB )
		return decumaNullDatabasePointer;

	if ( databaseHasWrongFormat(pDB) )
		return decumaInvalidDatabase;


	myMemSet(pCT, 0, dcGetTableSize(pStaticDB));
	pKeyDB = staticDBGetKeyDB(pDB);
	pCT->status = DC_STATUS_INITIALIZED;
	pCT->baseSymbolsOffset = sizeof(struct _CAT_TABLE);
	pCT->diacSymbolsOffset = pCT->baseSymbolsOffset +
		pKeyDB->nBaseSymbolTypes*sizeof(struct _CATEGORIES);

	return decumaNoError;
}

DECUMA_STATUS dcCopyStaticCategoryTable(CATEGORY_TABLE_PTR pCategoryTable,
					 STATIC_DB_PTR pStaticDB)
{	
	struct _CAT_TABLE * pCT = (struct _CAT_TABLE *) pCategoryTable;
	STATIC_DB_HEADER_PTR pDB = (STATIC_DB_HEADER_PTR) pStaticDB;
	KEY_DB_HEADER_PTR pKeyDB;
	CATEGORY_TABLE_PTR pStaticCT;

	if (pCT == NULL)
		return decumaNullPointer;

	if ( !pDB )
		return decumaNullDatabasePointer;

	if ( databaseHasWrongFormat(pDB) )
		return decumaInvalidDatabase;
	
	pKeyDB = staticDBGetKeyDB(pDB);
	pStaticCT = keyDBGetStaticCategoryTable(pKeyDB);

	decumaMemcpy(pCT, pStaticCT, dcGetTableSize(pStaticDB));

	pCT->status = DC_STATUS_INITIALIZED;

	return decumaNoError;

}

DECUMA_STATUS dcAdd(CATEGORY_TABLE_PTR pCategoryTable, STATIC_DB_PTR pStaticDB,
		const DECUMA_CHARACTER_SET * pCharacterSet,
		const DECUMA_UNICODE ** ppSymbols, int nSymbols)
{
	struct _CAT_TABLE * pCT = (struct _CAT_TABLE *) pCategoryTable;
	STATIC_DB_HEADER_PTR pDB = (STATIC_DB_HEADER_PTR) pStaticDB;
	DECUMA_STATUS status;
	int n, bSymbolNotInDatabase = 0;

	status = getCatTableStatus(pCT);
	if ( status != decumaNoError )
		return status;

	if ( !pDB )
		return decumaNullDatabasePointer;

	if ( databaseHasWrongFormat(pDB) )
		return decumaInvalidDatabase;

	status = checkSymbolPointers(ppSymbols, nSymbols);
	if ( status != decumaNoError )
		return status;

	if (!pCharacterSet)
		return decumaNullPointer;

	if (pCharacterSet->nSymbolCategories == 0)
		return decumaInvalidCategory;
		
	if (pCharacterSet->nLanguages == 0)
		return decumaInvalidLanguage;

	status = checkCategoriesToAdd(pCT, pCharacterSet);
	if ( status != decumaNoError )
		return status;

	status = addCategoriesToTable(pCT,pCharacterSet);
	if ( status != decumaNoError )
		return status;

	for ( n = 0; n < nSymbols; n++, ppSymbols++)
	{
		int bFoundSymbol = addSymbolToTable(pCT, *ppSymbols, 
			pCharacterSet,
			pDB,0);
		bSymbolNotInDatabase |= !bFoundSymbol;
	}

	if ( bSymbolNotInDatabase )
		return decumaSymbolNotInDatabase;
	else
		return decumaNoError;
}

DECUMA_STATUS dcAddSymbolsInString(CATEGORY_TABLE_PTR pCategoryTable, STATIC_DB_PTR pStaticDB,
		const DECUMA_CHARACTER_SET * pCharacterSet,
		const DECUMA_UNICODE * pSymbols)
{
	struct _CAT_TABLE * pCT = (struct _CAT_TABLE *) pCategoryTable;
	STATIC_DB_HEADER_PTR pDB = (STATIC_DB_HEADER_PTR) pStaticDB;
	DECUMA_STATUS status;
	int bSymbolNotInDatabase = 0;

	status = getCatTableStatus(pCT);
	if ( status != decumaNoError )
		return status;

	if ( !pDB )
		return decumaNullDatabasePointer;

	if ( databaseHasWrongFormat(pDB) )
		return decumaInvalidDatabase;

	if ( pSymbols == NULL )
		return decumaNullTextPointer;

	if (pCharacterSet->nSymbolCategories == 0)
		return decumaInvalidCategory;
		
	if (pCharacterSet->nLanguages == 0)
		return decumaInvalidLanguage;

	status = checkCategoriesToAdd(pCT, pCharacterSet);
	if ( status != decumaNoError )
		return status;

	status = addCategoriesToTable(pCT,pCharacterSet);
	if ( status != decumaNoError )
		return status;

	while ( *pSymbols )
	{
		DECUMA_UNICODE symbolBuf[2];
		int bFoundSymbol;

		symbolBuf[0] = *pSymbols; symbolBuf[1] = 0;
		bFoundSymbol = addSymbolToTable(pCT, symbolBuf, 
			pCharacterSet,(STATIC_DB_HEADER_PTR) pStaticDB, 0);
		bSymbolNotInDatabase |= !bFoundSymbol;
		pSymbols++;
	}

	if ( bSymbolNotInDatabase )
		return decumaSymbolNotInDatabase;
	else
		return decumaNoError;
}

DECUMA_STATUS dcAddLanguageToSymbolsInString(CATEGORY_TABLE_PTR pCategoryTable, 
		STATIC_DB_PTR pStaticDB,
		const DECUMA_UINT32 * pLanguages, DECUMA_UINT8 nLanguages,
		const DECUMA_UNICODE * pSymbols)
{

	struct _CAT_TABLE * pCT = (struct _CAT_TABLE *) pCategoryTable;
	STATIC_DB_HEADER_PTR pDB = (STATIC_DB_HEADER_PTR) pStaticDB;
	DECUMA_STATUS status;
	DECUMA_CHARACTER_SET charSet;
	
	int bSymbolNotInDatabase = 0;

	status = getCatTableStatus(pCT);
	if ( status != decumaNoError )
		return status;

	if ( !pDB )
		return decumaNullDatabasePointer;

	if ( databaseHasWrongFormat(pDB) )
		return decumaInvalidDatabase;

	if ( pSymbols == NULL )
		return decumaNullTextPointer;

	if (nLanguages == 0)
		return decumaInvalidLanguage;

	charSet.pSymbolCategories = NULL; /*Does not matter */
	charSet.nSymbolCategories = 0; 
	charSet.pLanguages = (DECUMA_UINT32 *) pLanguages;
	charSet.nLanguages = nLanguages;
	
	status = checkCategoriesToAdd(pCT, &charSet);
	if ( status != decumaNoError )
		return status;

	status = addCategoriesToTable(pCT, &charSet);
	if ( status != decumaNoError )
		return status;


	while ( *pSymbols )
	{
		DECUMA_UNICODE symbolBuf[2];
		int bFoundSymbol;

		symbolBuf[0] = *pSymbols; symbolBuf[1] = 0;
		bFoundSymbol = addSymbolToTable(pCT, symbolBuf, &charSet,
			(STATIC_DB_HEADER_PTR) pStaticDB, 1); /*Affect only language */
		bSymbolNotInDatabase |= !bFoundSymbol;
		pSymbols++;
	}

	if ( bSymbolNotInDatabase )
		return decumaSymbolNotInDatabase;
	else
		return decumaNoError;
}


DECUMA_STATUS dcFinish(CATEGORY_TABLE_PTR pCategoryTable, STATIC_DB_PTR pStaticDB)
{
	struct _CAT_TABLE * pCT = (struct _CAT_TABLE *) pCategoryTable;
	const DECUMA_STATUS status = getCatTableStatus(pCT);

	if (pCT == NULL)
		return decumaNullPointer;

	if ( !pStaticDB )
		return decumaNullDatabasePointer;

	if ( databaseHasWrongFormat( (STATIC_DB_HEADER_PTR ) pStaticDB) )
		return decumaInvalidDatabase;

	if ( status != decumaNoError )
		return status;

	pCT->status = DC_STATUS_READY;
	return decumaNoError;
}

/*************************************************** Local code ***/

static DECUMA_STATUS addCategoriesToTable(struct _CAT_TABLE * pCT, 
		const DECUMA_CHARACTER_SET * pCharacterSet)
{
	DECUMA_UINT32 symbolCategoryIdsCopy[NUMBER_OF_CATEGORIES_IN_TABLE];
	DECUMA_UINT32 languageIdsCopy[NUMBER_OF_CATEGORIES_IN_TABLE];
	DECUMA_STATUS status;
	
	/*See if the categories are all in the database already, otherwise they need to be added */
	/*If they don't fit there is a problem, return error and do nothing else. */
	decumaAssert(sizeof(pCT->categoryIds)==sizeof(symbolCategoryIdsCopy));
	decumaAssert(sizeof(pCT->languageIds)==sizeof(languageIdsCopy));
	
	decumaMemcpy(&symbolCategoryIdsCopy[0], &pCT->categoryIds[0], sizeof(symbolCategoryIdsCopy));
	decumaMemcpy(&languageIdsCopy[0], &pCT->languageIds[0], sizeof(languageIdsCopy));

	status = addUnfamiliarAtomicCategories(pCharacterSet, &symbolCategoryIdsCopy[0], 
		&languageIdsCopy[0]);

	if ( status != decumaNoError )
		return status;

	decumaMemcpy(&pCT->categoryIds[0], &symbolCategoryIdsCopy[0], sizeof(symbolCategoryIdsCopy));
	decumaMemcpy(&pCT->languageIds[0], &languageIdsCopy[0], sizeof(languageIdsCopy));

	return decumaNoError;
}

static int addSymbolToTable(CAT_TABLE_PTR pCT, const DECUMA_UNICODE * pSymbol, 
		const DECUMA_CHARACTER_SET * pCharacterSet,
		STATIC_DB_HEADER_PTR pStaticDB, int bOnlyLanguage)
{
	int n;
	PROPERTY_DB_HEADER_PTR pPropDB = staticDBGetPropDB(pStaticDB);
	KEY_DB_HEADER_PTR pKeyDB = staticDBGetKeyDB(pStaticDB);

	SYMBOL_TYPE_INFO_PTR pTI = keyDBGetStaticTypeTable(pKeyDB);
	DIAC_SYMBOL_INFO_PTR pDSI = propDBGetDiacSymbolTable(pPropDB);
	SYMBOL_INFORMATION_PTR pSItable = keyDBGetSymbolTable(pKeyDB);
	struct _CATEGORIES * pBaseCategories = (struct _CATEGORIES *) dcGetBaseCategoryTable( (CATEGORY_TABLE_PTR) pCT);
	struct _CATEGORIES * pDiacCategories = (struct _CATEGORIES *) dcGetDiacCategoryTable( (CATEGORY_TABLE_PTR) pCT);

	CATEGORY cat;

	int bFoundSymbol = 0;
	const int nSymbolTypes = keyDBGetNoSymbolTypes( pKeyDB );
	const int nDiacSymbols = propDBGetNoDiacSymbols( pPropDB );

	DECUMA_STATUS status = translateToCategoryStructs(
		pCharacterSet,	(CATEGORY_TABLE_PTR) pCT, NULL, &cat, NULL);
	
	CATEGORY_TABLE_PTR pStaticCatTable = keyDBGetStaticCategoryTable(pKeyDB);
	CATEGORIES_PTR pStaticDiacCategories = dcGetDiacCategoryTable(pStaticCatTable);

	decumaAssert(status == decumaNoError);

	/* Try to find the symbol among the base symbols */
	for ( n = 0; n < nSymbolTypes; n++, pTI++)
	{
		SYMBOL_INFORMATION_PTR pSI = &pSItable[pTI->symbolInfoIdx];
		DECUMA_UNICODE_DB_PTR pStaticSymbol = (DECUMA_UNICODE_DB_PTR)
			((DECUMA_DB_ADDRESS) pKeyDB + pSI->symbolOffset);
		int bThisSymbol;

		bThisSymbol = isSameSymbol(pStaticSymbol, pSymbol, 0);
		if ( bThisSymbol ) {
			if (!bOnlyLanguage)
			{
				CATEGORY_MASK_OR(pBaseCategories[n].cat.symbolCat,pBaseCategories[n].cat.symbolCat,cat.symbolCat);
			}
			CATEGORY_MASK_OR(pBaseCategories[n].cat.languageCat,pBaseCategories[n].cat.languageCat,cat.languageCat);
		}
		else if ( CAN_BE_SCALED_OR_TRANSLATED_TO_UPPER(pTI) )
		{
			/* Check if the database symbol is equal to the symbol */
			/* when it is scaled to upper. */
			bThisSymbol = isSameSymbol(pStaticSymbol, pSymbol, 1);

			if ( bThisSymbol ) {
				if (!bOnlyLanguage)
				{
					CATEGORY_MASK_OR(pBaseCategories[n].altCat.symbolCat,pBaseCategories[n].altCat.symbolCat,cat.symbolCat);
				}
				CATEGORY_MASK_OR(pBaseCategories[n].altCat.languageCat, pBaseCategories[n].altCat.languageCat,cat.languageCat);
			}
		}
		bFoundSymbol |= bThisSymbol;
	}

	/* Try to find the symbol among the diacritic keys. */
	for ( n = 0; n < nDiacSymbols; n++, pDSI++, pStaticDiacCategories++)
	{
		DECUMA_UNICODE_DB_PTR pStaticSymbol = (DECUMA_UNICODE_DB_PTR)
			((DECUMA_DB_ADDRESS) pKeyDB + pDSI->diacSymbolOffset);

		int bThisSymbol = isSameSymbol(pStaticSymbol, pSymbol, 0);

		if ( bThisSymbol ) {
			if (!bOnlyLanguage)
			{
				CATEGORY_MASK_OR(pDiacCategories[n].cat.symbolCat,pDiacCategories[n].cat.symbolCat,cat.symbolCat);
			}
			CATEGORY_MASK_OR(pDiacCategories[n].cat.languageCat,pDiacCategories[n].cat.languageCat,cat.languageCat);
		}
		/*
		Check the static category table if the symbol can be scaled
		or translated
		NOTE This test requires that the alternative symbol has an
		active category bit in the static database. This is normally the
		case, otherwise it would not be possible to get the symbol in the
		static database. */
		else if ( !CATEGORY_MASK_IS_EMPTY(pStaticDiacCategories->altCat.symbolCat) )
		{
			/* Check if the database symbol is equal to the symbol */
			/* when it is scaled to upper. */
			bThisSymbol = isSameSymbol(pStaticSymbol, pSymbol, 1);
			if ( bThisSymbol ) {
				if (!bOnlyLanguage)
				{
					CATEGORY_MASK_OR(pDiacCategories[n].altCat.symbolCat,pDiacCategories[n].altCat.symbolCat,cat.symbolCat);
				}
				CATEGORY_MASK_OR(pDiacCategories[n].altCat.languageCat,pDiacCategories[n].altCat.languageCat,cat.languageCat);
			}
		}
		bFoundSymbol |= bThisSymbol;
	}

	return bFoundSymbol;
} /* addSymbolToTable() */

static DECUMA_STATUS getCatTableStatus(CAT_TABLE_PTR pCT)
{
	if ( pCT == NULL )
		return decumaNullPointer;
		
	else if ( pCT->status != DC_STATUS_INITIALIZED &&
		      pCT->status != DC_STATUS_READY )
		return decumaCategoryTableNotInitialized;
	else
		return decumaNoError;
} /* catTableStatus() */

static DECUMA_STATUS checkSymbolPointers(const DECUMA_UNICODE ** ppSymbols, int nSymbols)
{
	if ( ppSymbols == NULL )
		return decumaNullPointer;
	else
	{
		int n;

		for ( n = 0; n < nSymbols; n++, ppSymbols++)
		{
			if ( *ppSymbols == NULL )
				return decumaNullTextPointer;
		}
		return decumaNoError;
	}
} /* checkSymbolPointers() */

static DECUMA_STATUS checkCategoriesToAdd(CAT_TABLE_PTR pCT, 
	const DECUMA_CHARACTER_SET * pCharacterSet)
{
	UNREFERENCED_PARAM(pCT);
	return checkCharacterSetValidity(pCharacterSet,0);
}

/************************************************** End of file ***/

#endif /* DECUMA_SPECIFIC_DB_STORAGE */

