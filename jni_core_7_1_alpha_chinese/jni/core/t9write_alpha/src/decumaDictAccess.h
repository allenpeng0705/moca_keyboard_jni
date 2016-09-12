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


#ifndef DECUMA_DICT_ACCESS_H
#define DECUMA_DICT_ACCESS_H

#include "decumaBasicTypes.h"

int decumaDictIsDecumaDict(const void *pData);
DECUMA_INT32 decumaDictGetDictCount(const void *pDecumaDict);
const void* decumaDictGetDict(const void *pDecumaDict, int nDictIndex);
DECUMA_INT32 decumaDictGetSize(const void *pDecumaDict, int nDictIndex);
DECUMA_INT32 decumaDictGetWordCount(const void *pDecumaDict, int nDictIndex);
DECUMA_INT32 decumaDictGetRankOffset(const void *pDecumaDict, int nDictIndex);
const void* decumaDictGetTrigramTable(const void *pDecumaDict, int nDictIndex);

#endif /* DECUMA_DICT_ACCESS_H */
