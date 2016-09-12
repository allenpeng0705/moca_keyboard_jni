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

        File: decumaCategoriesHidden.h
   $Revision: 1.5 $
     Package: CATEGORY
   Copyright: Decuma AB (2002)
 Information: www.decuma.com, info@decuma.com

\************************************************* Header files ***/

#ifndef decumaCategoriesHidden_h
#define decumaCategoriesHidden_h

#ifdef DECUMA_MANGLE
#include "mangle.h"
#endif

#include "decumaCategoryTableType.h" /* Definition of CATEGORY_TABLE_PTR */
#include "databaseFormat.h"

#ifdef __cplusplus
extern "C" {
#endif

DECUMA_HWR_PRIVATE int dcCategoryTableIsOK(CATEGORY_TABLE_PTR pCategoryTable);

DECUMA_HWR_PRIVATE CATEGORIES_PTR dcGetBaseCategoryTable(CATEGORY_TABLE_PTR pCategoryTable);

DECUMA_HWR_PRIVATE CATEGORIES_PTR dcGetDiacCategoryTable(CATEGORY_TABLE_PTR pCategoryTable);


#ifdef __cplusplus
} /* extern "C" { */
#endif

#endif /* #define decumaCategoriesHidden_h */

/************************************************** End of file ***/
