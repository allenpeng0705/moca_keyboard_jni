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

  File: decumaCategories.h
  $Revision: 1.5 $
  Package: CATEGORY

  This file contains the types, macros and function declarations for
  all personal category releated functions. These functions are used
  to create personal character set when the character sets that can
  be made up by symbolCategories defined in "decumaSymbolCategories.h" 
  and languages defined in "decumaLanguages.h" are not sufficient.

  Each symbol in the database belongs to one or more symbol categories
  and one or more languages, either a pre-defined or defined with this API. 
  When an interpretation is done, only the symbols that are included in the 
  active character set (the character set supplied to the engine for 
  the interpretation) may be returned.

\************************************************* Header files ***/

#ifndef decumaCategories_h
#define decumaCategories_h

#include "decumaUnicodeTypes.h"            /* Definition of DECUMA_UNICODE */
#include "decumaCategoryTableType.h"  /* Definition of CATEGORY_TABLE_PTR */
#include "databaseStatic.h"           /* Definition of STATIC_DB_PTR */
#include "decumaStatus.h"             /* Definition of DECUMA_STATUS */
#include "decumaBasicTypes.h"
#include "decumaCharacterSetType.h"       /* Definition of DECUMA_CHARACTER_SET */

#ifndef DCLIB_API
#define DCLIB_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
FUNCTION:	dcGetTableSize
This function returns the size needed for the datatype CATEGORY_TABLE.
If the static database is not correct (example wrong endianess) 0 is returned.
May be used with malloc() for allocating space for a CATEGORY_TABLE struct.

Parameters:
  pStaticDB               Pointer to the static database
Returns:
  The size of the datatype CATEGORY_TABLE.
**/
DCLIB_API DECUMA_UINT32 dcGetTableSize(STATIC_DB_PTR pStaticDB);

/**
FUNCTION:	dcInit
This function initialize the category table. Must be called before dcAdd().

Parameters:
  pCategoryTable          Pointer to a category table.

Returns:
  0 if everything is OK, otherwise an error code from the DECUMA_STATUS enumeration.
**/
DCLIB_API DECUMA_STATUS dcInit(CATEGORY_TABLE_PTR pCategoryTable,STATIC_DB_PTR pStaticDB);

/**
FUNCTION:	dcAdd
This function adds one or several symbols to the category table. The
character set parameter defines in which symbol categories and in which
languages all the symbols will be included.
To later get one of these symbols interpreted by the engine, the active 
"interpretation character set" needs to include at least one of these 
symbol categories and one of the languages.

This function can be called several times to add more categories and languages,
to the symbols.

NOTE: A category table cannot have more than MAX_CATEGORIES_IN_TABLE categories
      and more than MAX_LANGUAGES_IN_TABLE languages. Attempts to exceed these
      limits will result in returning an error code.

Parameters:
  pCategoryTable          Pointer to a category table.
  pStaticDB               Pointer to the static database
  pCharacterSet           The character subsets that the symbols shall belong to.
                          A maximum of 32 symbol categories and 32 languages will be regarded
                          Needs to contain at least 1 symbol category and 1 language
  ppSymbol                Pointer to an array of symbol strings. These strings
                          can be freed after the call to dcAdd().
                          NOTE: Each symbol shall be represented by a null-
                          terminated string containing only the symbol.
  nSymbols                Number of strings pointed to by ppSymbols.
Returns:
  0 if everything is OK, otherwise an error code from the DECUMA_STATUS enumeration.
**/
DCLIB_API DECUMA_STATUS dcAdd(CATEGORY_TABLE_PTR pCategoryTable, STATIC_DB_PTR pStaticDB,
		const DECUMA_CHARACTER_SET * pCharacterSet,
		const DECUMA_UNICODE ** ppSymbol, int nSymbols);

/**
FUNCTION:	dcAddSymbolsInString
This function adds symbols to the category table in a similiar way as dcAdd().
The difference is that for dcAddSymbolsInString() all symbols are added in 
one string instead of one string for each symbol. Consequently all symbols
that are added with dcAddSymbolsInString() must have length 1 (one character
per symbol). All symbols are given the category "category". Call this 
function several times to add more than one category.
NOTE: A category table cannot have more than MAX_CATEGORIES_IN_TABLE categories
      and more than MAX_LANGUAGES_IN_TABLE languages. Attempts to exceed these
      limits will result in returning an error code.

Parameters:
  pCategoryTable          Pointer to a category table.
  pStaticDB               Pointer to the static database
  pCharacterSet           The character subsets that the symbols shall belong to.
                          A maximum of 32 symbol categories and 32 languages will be regarded
                          Needs to contain at least 1 symbol category and 1 language
  pSymbols                Pointer to a string of symbols. The string must
                          be null-terminated. The string can be freed after
                          the call to dcAddSymbolsInString().
Returns:
  0 if everything is OK, otherwise an error code from the DECUMA_STATUS enumeration.
**/ 
DCLIB_API DECUMA_STATUS dcAddSymbolsInString(CATEGORY_TABLE_PTR pCategoryTable, STATIC_DB_PTR pStaticDB,
		const DECUMA_CHARACTER_SET * pCharacterSet,
		const DECUMA_UNICODE * pSymbols);


/**
FUNCTION:	dcAddLanguageToSymbolsInString
See dcAddSymbolsInString. All symbols are given the language category "languageCat". 
The symbolCategory will not be affected.
NOTE: A category table cannot have more than MAX_CATEGORIES_IN_TABLE categories
      and more than MAX_LANGUAGES_IN_TABLE languages. Attempts to exceed these
      limits will result in returning an error code.

Parameters:
  pCategoryTable          Pointer to a category table.
  pStaticDB               Pointer to the static database
  pLanguages,nLanguages   Languages for the symbols that are added.
  pSymbols                Pointer to a string of symbols. The string must
                          be null-terminated. The string can be freed after
                          the call to dcAddSymbolsInString().
Returns:
  0 if everything is OK, otherwise an error code from the DECUMA_STATUS enumeration.
**/ 
DCLIB_API DECUMA_STATUS dcAddLanguageToSymbolsInString(CATEGORY_TABLE_PTR pCategoryTable, 
		STATIC_DB_PTR pStaticDB,  
		const DECUMA_UINT32 * pLanguages, DECUMA_UINT8 nLanguages,
		const DECUMA_UNICODE * pSymbols);

/**
FUNCTION:	dcCopyStaticCategoryTable
This function makes an exact copy of the category table in the static database.
The copy can then be modified. Note that dcFinish should be called 
before the copy is used is used.

Parameters:
  pCategoryTable          Pointer to a category table.
  pStaticDB               Pointer to the static database

Returns:
  0 if everything is OK, otherwise an error code from the DECUMA_STATUS enumeration.
**/
DCLIB_API DECUMA_STATUS dcCopyStaticCategoryTable(CATEGORY_TABLE_PTR pCategoryTable, 
								   STATIC_DB_PTR pStaticDB);


/**
FUNCTION:	dcFinish
This function finishes the category table and makes it ready to use. Shall be
called after the last dcAdd() call.

Parameters:
  pCategoryTable          Pointer to a category table.
Returns:
  0 if everything is OK, otherwise an error code from the DECUMA_STATUS enumeration.
**/
DCLIB_API DECUMA_STATUS dcFinish(CATEGORY_TABLE_PTR pCategoryTable, STATIC_DB_PTR pStaticDB);

#ifdef __cplusplus
} /* extern "C" { */
#endif

#endif /* #define decumaCategories_h */

/************************************************** End of file ***/
