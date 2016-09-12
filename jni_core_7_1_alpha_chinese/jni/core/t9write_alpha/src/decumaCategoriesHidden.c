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

#include <stddef.h> /* Definition of NULL */

#include "decumaCategoryTableType.h"  /* Definition of CATEGORY_TABLE  */
#include "decumaCategoriesHidden.h"
#include "database.h"
#include "decumaAssert.h"

/*************************************************** Local data ***/
static int dcCategoryTableIsInitialized(CATEGORY_TABLE_PTR pCategoryTable);

/************************************************** Global data ***/

/********************************************* Local prototypes ***/

/************************************************** Global code ***/

/*************************************** Global code Hidden API ***/

int dcCategoryTableIsOK(CATEGORY_TABLE_PTR pCategoryTable)
{
	CAT_TABLE_PTR pCT = (CAT_TABLE_PTR) pCategoryTable;

	return pCT != NULL && pCT->status == DC_STATUS_READY;
}

static int dcCategoryTableIsInitialized(CATEGORY_TABLE_PTR pCategoryTable)
{
	CAT_TABLE_PTR pCT = (CAT_TABLE_PTR) pCategoryTable;

	return pCT != NULL && (pCT->status == DC_STATUS_READY || pCT->status == DC_STATUS_INITIALIZED);
}

CATEGORIES_PTR dcGetBaseCategoryTable(CATEGORY_TABLE_PTR pCategoryTable)
{
	decumaAssert( dcCategoryTableIsInitialized(pCategoryTable) );

	return (CATEGORIES_PTR) ((DECUMA_DB_ADDRESS) pCategoryTable + 
		((CAT_TABLE_PTR) pCategoryTable)->baseSymbolsOffset);
}

CATEGORIES_PTR dcGetDiacCategoryTable(CATEGORY_TABLE_PTR pCategoryTable)
{
	decumaAssert( dcCategoryTableIsInitialized(pCategoryTable) );

	return (CATEGORIES_PTR) ((DECUMA_DB_ADDRESS) pCategoryTable + 
		((CAT_TABLE_PTR) pCategoryTable)->diacSymbolsOffset);
}

/************************************************** End of file ***/



