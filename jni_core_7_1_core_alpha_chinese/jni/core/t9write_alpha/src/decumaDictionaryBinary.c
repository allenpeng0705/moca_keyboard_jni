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

#define DECUMA_DICTIONARY_BINARY_C

#include "decumaDictionaryBinary.h"
#include "decumaDictionaryBinaryData.h"
#include "decumaAssert.h"


void decumaDictionaryBinaryInitHeader(DECUMA_DICTIONARY_BINARY_HEADER * pDictBinary,
	DECUMA_DICTIONARY_BINARY_STRUCTURE structType)
{
	decumaAssert( sizeof(DECUMA_DICTIONARY_BINARY_HEADER) ==
		DECUMA_DICTIONARY_BINARY_HEADER_SIZE);

	pDictBinary->decumaID = DECUMA_DICTIONARY_BINARY_ID;
	pDictBinary->formatVersion = DECUMA_DICTIONARY_BINARY_FORMAT_VERSION;
	pDictBinary->decumaDictionaryStructure = structType;
}



/*Returns 1 if the dictionary is a decuma dictionary */
int decumaDictionaryBinaryIsDecumaDict(const DECUMA_DICTIONARY_BINARY_HEADER * pDictBinary)
{
	return pDictBinary->decumaID == DECUMA_DICTIONARY_BINARY_ID;
}

/*Returns 1 if the dictionary format is supported */
int decumaDictionaryBinaryFormatSupported(const DECUMA_DICTIONARY_BINARY_HEADER * pDictBinary)
{
	return pDictBinary->formatVersion == DECUMA_DICTIONARY_BINARY_FORMAT_VERSION;
}


/*Returns an integer which says what kind of dictionary structure the */
/*database contains */
DECUMA_DICTIONARY_BINARY_STRUCTURE decumaDictionaryBinaryGetStructure(
	const DECUMA_DICTIONARY_BINARY_HEADER * pDictBinary)
{
	return pDictBinary->decumaDictionaryStructure;
}
