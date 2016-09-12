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


#include "decumaExtractLangCatTables.h"
#include "decumaBasicTypes.h"
#include "databaseStructTypes.h"
#include "decumaCategoryTableType.h"
#include "decumaAssert.h"

const DECUMA_UINT32 DECUMA_DB_STORAGE * getCategoryIds(CATEGORY_TABLE_PTR pCatTable)
{
  decumaAssert(pCatTable);
  return (const DECUMA_UINT32 DECUMA_DB_STORAGE *)&(((CAT_TABLE_PTR)pCatTable)->categoryIds);
}

const DECUMA_UINT32 DECUMA_DB_STORAGE * getLanguageIds(CATEGORY_TABLE_PTR pCatTable)
{
  decumaAssert(pCatTable);
  return (const DECUMA_UINT32 DECUMA_DB_STORAGE *)&(((CAT_TABLE_PTR)pCatTable)->languageIds);
}
