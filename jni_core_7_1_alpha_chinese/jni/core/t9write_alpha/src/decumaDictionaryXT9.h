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

\******************************************************************/

#ifndef DECUMA_DICTIONARY_XT9_H
#define DECUMA_DICTIONARY_XT9_H

#include "decumaDictionary.h"
#include "decumaTrie.h"

DECUMA_HWR_PRIVATE DECUMA_STATUS decumaXT9UnpackToTrie(DECUMA_TRIE ** ppTrie,
	void const * pRawData, DECUMA_UINT32 dataSize,
	DECUMA_XT9_DICTIONARY_TYPE type, const DECUMA_MEM_FUNCTIONS * pMemFunctions);


#endif /*DECUMA_DICTIONARY_XT9_H */

