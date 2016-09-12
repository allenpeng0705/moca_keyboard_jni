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


/* Extra (Internal) High-level dictionary functions. */

#ifndef DECUMA_DICTIONARY_EXTRA_H
#define DECUMA_DICTIONARY_EXTRA_H

#include "decumaConfig.h"

#ifndef DECUMA_HWR_ENABLE_EXTRA_API
#error There is no point in using the dictionary extra-API if it is not enabled 
#endif

#include "decumaDictionary.h" /* Defintion of basic integer types */
#include "decumaBasicTypes.h" /* Defintion of basic integer types */
#include "decumaStatus.h"     /* Definition of DECUMA_STATUS */
#include "decuma_hwr.h"       /* Definitions of DECUMA_UNICODE and */

#ifdef DECUMA_USE_EZITEXT_INTEGRATION

#include "zoutedge2.h" /* Definitions of ZI8UCHAR and ZI8_GBL_PARM */

/* Create HWR dictionary from eZiText dictionary. Please note that this creates a dynamic dictionary that can be extended. */
DECUMA_STATUS decumaUnpackEZiTextDictionary(const DECUMA_HWR_DICTIONARY ** ppDictionary,
														   const ZI8UCHAR ziLanguage ZI8_GBL_PARM);
#endif

/* Create a HWR dictionary from a UTF-8 text buffer. This creates a dynamic dictionary that can be extended. */
DECUMA_STATUS decumaDictionaryCreateFromUTF8(DECUMA_HWR_DICTIONARY ** ppDictionary,
	const char * pUTF8,int const length,int const bNullDelimited);

/* Create a HWR dictionary from a UTF-16 (native endianess) text buffer. This creates a dynamic dictionary that can be extended. */
DECUMA_STATUS decumaDictionaryCreateFromUTF16(DECUMA_HWR_DICTIONARY ** ppDictionary,
	const DECUMA_UNICODE * pUTF16,int const length,int const bNullDelimited);


/* Some utility functions, mainly used for internal verification tests: */
DECUMA_STATUS decumaDictionaryAddAllWords(DECUMA_HWR_DICTIONARY * pDictionaryBuffer,
	const DECUMA_UNICODE * const pWordBufferStart,int nUnicodesInBuffer,
	int bNullDelimitedOnly);

#endif /* DECUMA_DICTIONARY_H */
