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

  This file describes the interface to a Decumas HWR dictionary binary header
  which is used to identify the format of a dictionary binary.
    
\******************************************************************/

#ifndef DECUMA_DICTIONARY_BINARY_H
#define DECUMA_DICTIONARY_BINARY_H

#include "decumaConfig.h"
#include "decumaBasicTypes.h"

/*The maximum size of the datastructure DECUMA_DICTIONARY_BINARY_HEADER */
#define DECUMA_DICTIONARY_BINARY_HEADER_SIZE 32 /*Bytes. All might not be used currently, though */

typedef struct _DECUMA_DICTIONARY_BINARY_HEADER DECUMA_DICTIONARY_BINARY_HEADER;


typedef enum _DECUMA_DICTIONARY_BINARY_STRUCTURE
{
	decumaDictionaryBinaryStructureTrie = 0,
	decumaDictionaryBinaryStructureTerseTrie
	
} DECUMA_DICTIONARY_BINARY_STRUCTURE;


DECUMA_HWR_PRIVATE void decumaDictionaryBinaryInitHeader(DECUMA_DICTIONARY_BINARY_HEADER * pDictBinary,
	DECUMA_DICTIONARY_BINARY_STRUCTURE structType);


/*Returns 1 if the dictionary is a decuma dictionary */
DECUMA_HWR_PRIVATE int decumaDictionaryBinaryIsDecumaDict(const DECUMA_DICTIONARY_BINARY_HEADER * pDictBinary);

/*Returns 1 if the dictionary format is supported */
DECUMA_HWR_PRIVATE int decumaDictionaryBinaryFormatSupported(const DECUMA_DICTIONARY_BINARY_HEADER * pDictBinary);


/*Returns an integer which says what kind of dictionary structure the */
/*dictionary binary conatins (laidout just after the header) */
DECUMA_HWR_PRIVATE DECUMA_DICTIONARY_BINARY_STRUCTURE decumaDictionaryBinaryGetStructure(
	const DECUMA_DICTIONARY_BINARY_HEADER * pDictBinary);




#endif /*DECUMA_DICTIONARY_BINARY_H */

