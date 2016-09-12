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


#ifndef DECUMA_CATEGORY_COMBINATION_DATA_H_123451346 
#define DECUMA_CATEGORY_COMBINATION_DATA_H_123451346 

#include "decumaConfig.h"
#include "decumaBasicTypes.h"
#include "decumaCatCombDataType.h"

#define DECUMA_NUMBER_OF_SYMBOL_CATEGORIES	202 
#define DECUMA_NUMBER_OF_LANGUAGES		85

#define MAX_SYMBOL_CATEGORIES_IN_COMBINATION 11
#define MAX_LANGUAGES_IN_COMBINATION 1
#define MAX_ATOMS_IN_COMBINATION 11

DECUMA_HWR_PRIVATE_DATA_H const DECUMA_UINT32 decumaAllSymbolCategories[202];
DECUMA_HWR_PRIVATE_DATA_H const DECUMA_UINT32 decumaAllLanguages[85];

DECUMA_HWR_PRIVATE_DATA_H const CATEGORY_COMB_PAIR decumaCombinationCategories[290];
DECUMA_HWR_PRIVATE_DATA_H const CATEGORY_COMB_PAIR decumaCombinationLanguages[15];

#endif
