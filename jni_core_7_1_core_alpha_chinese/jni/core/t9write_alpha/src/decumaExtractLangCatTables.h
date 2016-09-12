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


#ifndef DECUMAEXTRACTLANGCATTABLES_H
#define DECUMAEXTRACTLANGCATTABLES_H

#ifdef DECUMA_MANGLE
#include "mangle.h"
#endif

#include "decumaConfig.h"
#include "decumaBasicTypes.h"
#include "decumaCategoryTableType.h"

/*****
 *
 * These functions are used by decumaCategoryTranslation to gain access to the 
 * category- and language-tables in the database. This servers as a wrapper between
 * the db-independent category-magic and the db-specific format of the tables.
 *
 */

DECUMA_HWR_PRIVATE const DECUMA_UINT32 DECUMA_DB_STORAGE * getCategoryIds(CATEGORY_TABLE_PTR pCatTable);

DECUMA_HWR_PRIVATE const DECUMA_UINT32 DECUMA_DB_STORAGE * getLanguageIds(CATEGORY_TABLE_PTR pCatTable);

#endif
