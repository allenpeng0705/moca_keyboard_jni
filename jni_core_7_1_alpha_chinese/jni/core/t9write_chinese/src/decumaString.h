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


/*decumaString.h */

#ifndef DECUMASTRING_H_IUYGFHRBVIUW5YGT95
#define DECUMASTRING_H_IUYGFHRBVIUW5YGT95

#include "decumaConfig.h"
#include "decumaUnicodeTypes.h"
#include "decumaDataTypes.h"
#include "decumaUnicodeTypes.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef UTF8
#define UTF8 DECUMA_UINT8
#endif

/* bDecumaSpecialCaseConversion - Uses the decuma specific rules for converting between upper and lowercase */
/*                                e.g. '_' to '-' */
DECUMA_HWR_PRIVATE int decumaStrcmpi(const DECUMA_UNICODE* a, const DECUMA_UNICODE* b,int bDecumaSpecialCaseConversion);

DECUMA_HWR_PRIVATE int decumaStrcmp(const DECUMA_UNICODE* a, const DECUMA_UNICODE* b);
/* returns -1 if a < b, 0 if a == b, and 1 if a > b. */

DECUMA_HWR_PRIVATE int decumaStrlen(const DECUMA_UNICODE* a);

/*This function returns the number of bytes in the string. */
DECUMA_HWR_PRIVATE int decumaStrlenUTF8(const UTF8* a);

/*This function returns the number of characters in the string. */
DECUMA_HWR_PRIVATE int decumaChrlenUTF8(const UTF8* a);

DECUMA_HWR_PRIVATE const DECUMA_UNICODE * decumaStrchr( const DECUMA_UNICODE *a, int c );

/* Copies the string and encodes it with the appropriate format. */
DECUMA_HWR_PRIVATE int decumaStrncpy(DECUMA_UNICODE* a, const DECUMA_UNICODE* b , int max);

DECUMA_HWR_PRIVATE int decumaStrncpyUTF8(UTF8* a, const UTF8* b , int max);

/* Copies the string and encodes it with the appropriate format and */
/* converts lower case letters to upper case. */
/* bDecumaSpecialCaseConversion - Uses the decuma specific rules for converting between upper and lowercase */
/*                                e.g. '_' to '-' */
DECUMA_HWR_PRIVATE int decumaStrncpyToUpper(DECUMA_UNICODE* a, const DECUMA_UNICODE* b , int max,int bDecumaSpecialCaseConversion);

/* Copies the string and encodes it with the appropriate format and */
/* converts upper case letters to lower case. */
/* bDecumaSpecialCaseConversion - Uses the decuma specific rules for converting between upper and lowercase */
/*                                e.g. '_' to '-' */
DECUMA_HWR_PRIVATE int decumaStrncpyToLower(DECUMA_UNICODE* a, const DECUMA_UNICODE* b , int max,int bDecumaSpecialCaseConversion);

/* Returns TRUE if character is upper case, otherwise it returns FALSE. */
/* bDecumaSpecialCaseConversion - Uses the decuma specific rules for converting between upper and lowercase */
/*                                e.g. '_' to '-' */
DECUMA_HWR_PRIVATE DECUMA_BOOL decumaIsUpper(const DECUMA_UNICODE* a,int bDecumaSpecialCaseConversion);

/* Convert a int to a char string. */
/* Returns the number of characters written to pBuf, excluding the terminating NUL-character, */
/*         or 0 if the buffer is too small. */
DECUMA_HWR_PRIVATE int decumaIToA (int nVal, int nRadix, char *pBuf, int nBufLen);

/* Convert an unsigned int to a char string. */
/* Returns the number of characters written to pBuf, excluding the terminating NUL-character, */
/*         or 0 if the buffer is too small. */
DECUMA_HWR_PRIVATE int decumaUToA (unsigned int nVal, int nRadix, char *pBuf, int nBufLen);

/* Convert char string to an int. */
/* Returns the number of characters read from pStr, excluding the terminating NUL-character if any, */
/*         or 0 the string could not be converted. Result is in *pnResult. */
DECUMA_HWR_PRIVATE int decumaAToI(char * pStr, int nStrLen, int nRadix, int * pnResult);

DECUMA_HWR_PRIVATE DECUMA_BOOL decumaUnicodeIsLower(DECUMA_UNICODE ch, int bDecumaSpecial);
DECUMA_HWR_PRIVATE DECUMA_BOOL decumaUnicodeIsUpper(const DECUMA_UNICODE * a,int bDecumaSpecial);
DECUMA_HWR_PRIVATE DECUMA_BOOL decumaUnicodeIsLetter(DECUMA_UNICODE ch);


DECUMA_HWR_PRIVATE DECUMA_UNICODE decumaUnicodeToUpper(DECUMA_UNICODE ch, int bDecumaSpecial);
DECUMA_HWR_PRIVATE DECUMA_UNICODE decumaUnicodeToLower(DECUMA_UNICODE ch, int bDecumaSpecial);

DECUMA_HWR_PRIVATE DECUMA_UNICODE decumaUnicodeToUpper(DECUMA_UNICODE ch, int bDecumaSpecial);
DECUMA_HWR_PRIVATE DECUMA_UNICODE decumaUnicodeToLower(DECUMA_UNICODE ch, int bDecumaSpecial);

#ifdef __cplusplus
} /*extern "C" { */
#endif

#endif
