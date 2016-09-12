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


#include "decumaConfig.h"

#if !defined(DECUMA_USE_STDLIB_MEMORY_FUNCTIONS)

#include "decumaMemory.h" /* Possibly contains definition of 'restrict' */
#include "decumaAssert.h"

DECUMA_HWR_PRIVATE void * decumaMemset ( void * ptr, int value, size_t n )
{
	unsigned char charValue = (unsigned char) value;
	unsigned char * charPtr = ptr;

	decumaAssert(ptr || n == 0);

	for ( ; n > 0; n-- )
	{
		*charPtr++ = charValue;
	}

	return ptr;
}

DECUMA_HWR_PRIVATE void * decumaMemcpy ( void * restrict s1, const void * restrict s2, size_t n )
{
	char * cs1 = s1;
	const char * cs2 = s2;

	decumaAssert(s1 || n == 0);
	decumaAssert(s2 || n == 0);
	decumaAssert( ( cs1 < cs2 && (cs1 + n) < cs2) || ( cs1 > cs2 && cs1 > (cs2 + n) ) ); /* Memory areas may not overlap. If they do, use memmove() instead */

	for ( ; n > 0; n--) {
        *cs1++ = *cs2++;
    }

	return s1;
}

DECUMA_HWR_PRIVATE void * decumaMemmove ( void * s1, const void * s2, size_t n )
{
	size_t i;
	char * cs1 = s1;
	const char * cs2 = s2;

	decumaAssert(s1 || n == 0);
	decumaAssert(s2 || n == 0);

	if ( s1 > s2 )
	{
		for (i = n; i > 0; i--) {
		    cs1[i-1] = cs2[i-1];
		}
	} 
	else
	{
		for (i = 0; i < n; i++) {
		    cs1[i] = cs2[i];
		}
	}

	return s1;
}

DECUMA_HWR_PRIVATE int decumaMemcmp(const void * s1, const void * s2, size_t n)
{
	size_t i;
	const unsigned char * cs1 = s1;
	const unsigned char * cs2 = s2;

	decumaAssert(s1 || n == 0);
	decumaAssert(s2 || n == 0);

	for ( i = 0; i < n; i++ )
	{
		if ( cs1[i] != cs2[i] )
			return cs1[i] - cs2[i];
	}

	return 0;
}

#endif /* !DECUMA_USE_STDLIB_MEMORY_FUNCTIONS */
