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


/*
Autogenerated header.
$Revision: 1.5 $
$Date: 2011/02/14 11:41:14 $
$Author: jianchun_meng $
*/

#ifndef SCRFULLSEARCH_H
#define SCRFULLSEARCH_H

#ifdef DECUMA_MANGLE
#include "mangle.h"
#endif

#include "scrCurve.h"
#include "databaseKEY.h"
#include "scrOutput.h"
#include "scrOutputHandler.h"
#include "decumaStatus.h"

typedef struct _SEARCH_SETTINGS
{
	KEY_DB_HEADER_PTR pKeyDB;
	PROPERTY_DB_HEADER_PTR pPropDB;
	CATEGORY_TABLE_PTR pCatTable;
} SEARCH_SETTINGS;

DECUMA_HWR_PRIVATE DECUMA_STATUS fullSearch(const SEARCH_SETTINGS * pSearchSettings,
				OUTPUT_HANDLER * pOutputHandler, const SCR_CURVE * pCurve,
				const scrOUTPUT_LIST * pPrecalculatedBaseOutputs,
				SCR_API_SETTINGS * pSettings,
				const DECUMA_INTERRUPT_FUNCTIONS * pInterruptFunctions);


#endif

