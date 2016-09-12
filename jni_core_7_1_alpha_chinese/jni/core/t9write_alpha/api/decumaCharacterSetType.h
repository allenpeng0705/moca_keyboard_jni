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


#ifndef DECUMA_CHARACTER_SET_H
#define DECUMA_CHARACTER_SET_H

#include "decumaBasicTypes.h"

/* 
 The character set limits the set of characters from which the interpretation
 is taken. The limitation has two dimensions, "symbol categories" and "languages".

 It is not suitable to search
 among all the symbols at once, there are too many symbols 
 that look too similar: 'C' and '(' for example. This is
 the reason for symbol categories.
 The languages is another dimension for limiting the character set.
 Several categories and languages can be used for one call. Therefore
 arrays are used.
 Note that every category and language is not supported by every database.
 There are functions that can be used to ask if the database supports a certain category.
*/

typedef struct _DECUMA_CHARACTER_SET {
	DECUMA_UINT32 * pSymbolCategories; /* Pointer to an array of symbol categories see decumaSymbolCategories.h */
	DECUMA_UINT32 * pLanguages; /* Pointer to an array of languages see decumaLanguages.h */
	DECUMA_UINT8 nSymbolCategories; /* The number of symbol categories in the array */
	DECUMA_UINT8 nLanguages; /* The number of languages in the array */
} DECUMA_CHARACTER_SET;

#endif /* DECUMA_CHARACTER_SET_H */
