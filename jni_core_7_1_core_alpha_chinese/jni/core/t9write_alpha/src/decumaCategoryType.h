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


#ifndef DECUMA_CATEGORY_TYPE_H
#define DECUMA_CATEGORY_TYPE_H 1

#include "decumaBasicTypes.h"

/*
The data type CATEGORY holds two bitmask. Each bit in the symbolCat bitmask represents a specific
category of symbols, i.e. "upper case","lower case", "numeric", "email", etc.

The bits of the second bitmask, languageCat, represent languages.
*/

typedef DECUMA_UINT32 CATEGORY_MASK[2];  /*Use 64 bits */

typedef struct _CATEGORY
{
	CATEGORY_MASK symbolCat;  /* The symbol group */
	CATEGORY_MASK languageCat; /* The language */
} CATEGORY;


/*Useful macro to check if two categories overlap*/
#define IS_IN_CATEGORY( cat1, cat2) ( (((cat1).symbolCat[0] & (cat2).symbolCat[0]) || ((cat1).symbolCat[1] & (cat2).symbolCat[1])) && (((cat1).languageCat[0] & (cat2).languageCat[0]) || ((cat1).languageCat[1] & (cat2).languageCat[1])))

/* Check whether the bits set in mask1 are also set in mask2 
 *     x = CATEGORY_MASK_CONTAINS(mask1, mask2);
 * is equvalent to
 *     CATEGORY_MASK tmpMask;
 *     CATEGORY_MASK_OR(tmpMask, mask1, mask2);
 *     x = CATEGORY_MASK_EQUALS(tmpMask, mask2);
 */
#define CATEGORY_MASK_CONTAINS(mask1, mask2) ( ( ((mask1)[0] | (mask2)[0]) == (mask2)[0]) && ( ((mask1)[1] | (mask2)[1]) == (mask2)[1]) )

#define CATEGORY_MASK_COPY( dest,src) {(dest)[0]=(src)[0]; (dest)[1]=(src)[1];}

#define CATEGORY_MASK_OR( result, mask1, mask2) {\
	(result)[0]=(mask1)[0] | (mask2)[0];(result)[1]=(mask1)[1] | (mask2)[1]; \
}


#define CATEGORY_MASK_AND( result, mask1, mask2) {\
	(result)[0]=(mask1)[0] & (mask2)[0];(result)[1]=(mask1)[1] & (mask2)[1]; \
}

#define CATEGORY_MASK_EQUAL(mask1, mask2) ((mask1)[0]==(mask2)[0] && (mask1)[1]==(mask2)[1])

#define CATEGORY_MASK_RESET(mask) {(mask)[0]=0; (mask)[1]=0;}

#define CATEGORY_MASK_SET_ONLY_FIRST_BIT(mask) {(mask)[0]=1; (mask)[1]=0;}

#define CATEGORY_MASK_SET_ALL_BITS(mask) {(mask)[0]=0xFFFFFFFF; (mask)[1]=0xFFFFFFFF;}

#define CATEGORY_MASK_SHIFT_1_LEFT(mask) {DECUMA_UINT32 extraBit= ((mask)[0]>=(0x80000000) ? 1 : 0); (mask)[0]<<=1; (mask)[1]<<=1;(mask)[1]+=extraBit;}

#define CATEGORY_MASK_IS_EMPTY(mask) ((mask)[0]==0 && (mask)[1]==0)

#endif
