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


/* $Header: /data/cvsroot/seattle/t9write/cjk/core/src/decumaString.c,v 1.5 2011/02/14 11:15:16 jianchun_meng Exp $ */

#include <stddef.h> /* Definition of NULL */

#include "decumaConfig.h"
#include "decumaString.h"
#include "decumaAssert.h"
#include "globalDefs.h"

/*void decumaStrrev(DECUMA_UNICODE* a, const DECUMA_UNICODE *b)
{
	const DECUMA_UNICODE * pb = b + decumaStrlen(b)-1;
	int i;

	decumaAssert(a!=b); //Can not operate on same buffer

	for (i=0; i<decumaStrlen(b); i++, pb--, a++)
	{
		*a = *pb;
	}

	*a=0; //Terminating zero
}*/

int decumaStrcmp(const DECUMA_UNICODE* a, const DECUMA_UNICODE* b)
{
	decumaAssert( a );
	decumaAssert( b );

	while((*a) && (*b))
	{
		if(*a > *b)
			return 1;
		if(*a < *b)
			return -1;
		a++;
		b++;
	}

	if(*a == *b)
		return 0;
	if(*a == 0)
		return -1;
	else
		return 1;
}

int decumaStrcmpi(const DECUMA_UNICODE* a, const DECUMA_UNICODE* b,int bDecumaSpecial)
{
	decumaAssert( a );
	decumaAssert( b );

	while((*a) && (*b))
	{
		if (*a != *b) 
		{
			DECUMA_UNICODE la=decumaUnicodeToLower(*a, bDecumaSpecial);
			DECUMA_UNICODE lb=decumaUnicodeToLower(*b, bDecumaSpecial);
			
			if (la>lb)
				return 1;
			if(la < lb)
				return -1;
		}
		a++;
		b++;
	}

	if(*a == 0 && *b == 0)
		return 0;
	if(*a == 0)
		return -1;
	else
		return 1;

}

int decumaStrlen(const DECUMA_UNICODE* a)
{
	int nToReturn = 0;

	decumaAssert( a );

	while(*(a++))
		nToReturn++;

	return nToReturn;
}


int decumaStrlenUTF8(const UTF8* a)
{
	int nToReturn = 0;

	decumaAssert( a );

	while(*(a++))
		nToReturn++;

	return nToReturn;
}

int decumaChrlenUTF8(const UTF8* a)
{
  int i, bytes;
  int nChars = 0;

	decumaAssert( a );

  bytes  = decumaStrlenUTF8( a );
  
  for (i = 0; i < bytes; i++)
    {
      UTF8 c = (UTF8)a[i];
      
      if ( c >= 0xC2 && c < 0xE0 )
        i++;
      else if ( c >= 0xE0 && c < 0xF0 )
        i += 2;
      else if ( c >= 0xF0 && c < 0xF8 )
        i += 3;
      else if ( c >= 0xF8 && c < 0xFC )
        i += 4;
      else if ( c >= 0xFC && c < 0xFE )
        i += 5;
      else if ( c == 0xFF )
        return -1;
      
      nChars++;
    }
  
  return nChars;
}

const DECUMA_UNICODE * decumaStrchr(const DECUMA_UNICODE* a, int c)
{
	decumaAssert( a );

	while(*a != 0 && *a != c)	
		a++;
	
	if(*a == c) /* this behaves correctly even when c == 0 */
		return a;
	else
		return NULL;
}

int decumaStrncpy(DECUMA_UNICODE* a, const DECUMA_UNICODE* b , int max)
{
	int nCopy = 0;

	decumaAssert( a );
	decumaAssert( b );

	while ( nCopy < max && *b != 0 ) 
	{
		*a = *b;
		a++;b++;
		nCopy++;
	}

	if (nCopy < max)
	{
		*a = 0;
	}

	return nCopy;
}

int decumaStrncpyUTF8(UTF8* a, const UTF8* b, int max)
{
	int nCopy = 0;

	decumaAssert( a );
	decumaAssert( b );

	while ( nCopy < max && *b != 0 ) 
	{
		*a = *b;
		a++;b++;
		nCopy++;
	}

	if (nCopy < max)
	{
		*a = 0;
	}

	return nCopy;
}

int decumaStrncpyToUpper(DECUMA_UNICODE* a, const DECUMA_UNICODE* b, int max, int bDecumaSpecial)
{
	int nWritten = 0;

	decumaAssert( a );
	decumaAssert( b );

	while ( nWritten < max && *b != 0 ) 
	{
		*a = decumaUnicodeToUpper( *b, bDecumaSpecial);
		a++; 
		b++; 
		nWritten++;
	}

	if ( nWritten < max ) 
	{
		*a = 0;
	}

	return nWritten;
} /* decumaStrncpyToUpper() */

int decumaStrncpyToLower(DECUMA_UNICODE* a, const DECUMA_UNICODE* b ,int max,int bDecumaSpecial)
{
	int nWritten = 0;

	decumaAssert( a );
	decumaAssert( b );

	while ( nWritten < max && *b != 0 ) 
	{
		*a = decumaUnicodeToLower( *b,bDecumaSpecial);
		a++; b++; nWritten++;
	}

	if ( nWritten < max ) 
	{
		*a = 0;
	}
	return nWritten;
} /* decumaStrncpyToLower() */

DECUMA_BOOL decumaIsUpper(const DECUMA_UNICODE* a, int bDecumaSpecial)
{
	decumaAssert( a );

	decumaAssert( sizeof(DECUMA_UNICODE) <= sizeof(DECUMA_UNICODE) );

	return decumaUnicodeIsUpper( (const DECUMA_UNICODE *) a,bDecumaSpecial );
} /* decumaIsUpper() */


DECUMA_UNICODE decumaUnicodeToUpper(DECUMA_UNICODE ch,int bDecumaSpecial)
{
	if ( ch >= 0x03B1 && ch <= 0x03CB && ch != 0x03C2) { /* Primary Greek block */
		return (DECUMA_UNICODE) (ch + (0x0391 - 0x03B1));
	}
	else if ( ch >= 0x03AC && ch <= 0x03AF) { /* Minor Greek block */
		return (DECUMA_UNICODE) (ch == 0x03AC ? 0x0386 : ch + (0x038A - 0x3AF));
	}
	else if ( ch >= 0x03CC && ch <= 0x03CE) { /* Minor Greek block */
		return (DECUMA_UNICODE) (ch == 0x03CC ? 0x038C : ch + (0x038F - 0x3CE));
	}
	else if ( ch >= 'a' && ch <= 'z') {
		return (DECUMA_UNICODE) (ch + ('A' - 'a'));
	}
	else if ( ch >= 0xE0 && ch <= 0xF6 ) { /* '�' to '�' */
		return (DECUMA_UNICODE) (ch - (0x20)); /*('�' - '�') = 0x20 */
	}
	else if ( ch >= 0xF8 && ch <= 0xFE) { /* '�' && ch <= '�' */
		return (DECUMA_UNICODE) (ch - (0x20)); /*('�' - '�') = 0x20 */
	}
	else if ( ch == 0x00FF ){	/* "y + �" and its upper case equivalent */
		return 0x0178;
	}
	else if ( ch >= 0x0100 && ch <= 0x0137 && ch%2 == 1) /*Latin Extended-A */
	/* lowercase is odd */
	{
		if (ch == 0x0131) { /* LATIN SMALL LETTER DOTTLESS I */
			return 'I';
		}
		else
			return (DECUMA_UNICODE) (ch - 1);
	}
	else if ( ch >= 0x0139 && ch <= 0x0148 && ch%2 == 0) /*Latin Extended-A */
	/* lowercase is even */
	{
		return (DECUMA_UNICODE) (ch - 1);
	}
	else if ( ch >= 0x014A && ch <= 0x0177 && ch%2 == 1) /*Latin Extended-A */
	/* lowercase is odd */
	{
		return (DECUMA_UNICODE) (ch - 1);
	}
	else if ( ch >= 0x0179 && ch <= 0x017E && ch%2 == 0) /*Latin Extended-A */
	/* lowercase is even */
	{
		return (DECUMA_UNICODE) (ch - 1);
	}
	else if ( ch >= 0x200 && ch <= 0x021B && ch%2 == 1) /*Latin Extended-B */
	/* (Croatian, Romanian and Slovenien characters) */
	/* lowercase is odd  */
	{
		return (DECUMA_UNICODE) (ch - 1);
	}
	else if ( ch >= 0x3041 && ch <= 0x30FE) /*hiragana & katakana */
	{
		if (ch == 0x30AB) {
			return 0x30F5;
		}
		else if (ch == 0x30B1) {
			return 0x30F6;
		}
		else
			return (DECUMA_UNICODE) (ch - 1);
	}
	else if (ch >= 0x0430 && ch <= 0x044F) /* Cyrillic (Basic Russian) */
	/* lowercase is offset 0x20 */
	{
		return (DECUMA_UNICODE) (ch - (0x20));
	}
	else if (ch >= 0x0450 && ch <= 0x045F) /* Cyrillic (Cyrillic extention (Serbian, Ukrainian, Bulgarian)) */
	/* lowercase is offset 0x50 */
	{
		return (DECUMA_UNICODE) (ch - (0x50));
	}
	else if (ch == 0x0491) /* Cyrillic (Extended cyrillic (Ukrainian GHE WITH UPTURN)) */
	/* lowercase is odd */
	{
		return (DECUMA_UNICODE) (ch - 1);
	}
	/*--- BEGIN SPECIAL DECUMA RULES */
	else if ( bDecumaSpecial && ch == '_' ) {
		return '-';	
	}
	else if ( bDecumaSpecial && ch == ',' ){	
		return '\'';
	}
	/*--- END SPECIAL DECUMA RULES */
	else {
		return ch;
	}
} /* decumaUnicodeToUpper() */


DECUMA_UNICODE decumaUnicodeToLower(DECUMA_UNICODE ch, int bDecumaSpecial)
{
	if ( ch >= 0x0391 && ch <= 0x3AB && ch != 0x03A2) { /* Primary Greek block */
		return (DECUMA_UNICODE) (ch + (0x03B1 - 0x0391));
	}
	else if ( ch == 0x0386 ) { /* Minor Greek block */
		return 0x03AC;
	}
	else if ( ch >= 0x0388 && ch <= 0x038A ) { /* Minor Greek block */
		return (DECUMA_UNICODE) (ch + (0x03AD - 0x0388));
	}
	else if ( ch == 0x038C ) { /* Minor Greek block */
		return 0x03CC;
	}
	else if ( ch >= 0x038E && ch <= 0x038F ) { /* Minor Greek block */
		return (DECUMA_UNICODE) (ch + (0x03CE - 0x038F));
	}
	else if ( ch >= 'A' && ch <= 'Z') {
		return (DECUMA_UNICODE) (ch - ('A' - 'a'));
	}
	else if ( ch >= 0xC0 && ch <= 0xD6 ) { /* '�' to '�' */
		return (DECUMA_UNICODE) (ch + (0x20)); /* ('�' - '�') = 0x20 */
	}
	else if ( ch >= 0xD8 && ch <= 0xDE) { /* '�' && ch <= '�' */
		return (DECUMA_UNICODE) (ch + (0x20)); /* ('�' - '�') = 0x20 */
 	}
	else if ( ch == 0x0178 ){	/* "y + �" and its upper case equivalent */
		return 0x00FF;
	}
	else if ( ch >= 0x0100 && ch <= 0x0137 && ch%2 == 0) /*Latin Extended-A */
	/* uppercase is even */
	{
    return (DECUMA_UNICODE) (ch + 1);
	}
	else if ( ch >= 0x0139 && ch <= 0x0148 && ch%2 == 1) /*Latin Extended-A */
	/* uppercase is odd */
	{
		return (DECUMA_UNICODE) (ch + 1);
	}
	else if ( ch >= 0x014A && ch <= 0x0177 && ch%2 == 0) /*Latin Extended-A */
	/* uppercase is even */
	{
		return (DECUMA_UNICODE) (ch + 1);
	}
	else if ( ch >= 0x0179 && ch <= 0x017E && ch%2 == 1) /*Latin Extended-A */
	/* uppercase is odd */
	{
		return (DECUMA_UNICODE) (ch + 1);
	}
	else if ( ch >= 0x200 && ch <= 0x021B && ch%2 == 0) /*Latin Extended-B */
	/* (Croatian, Romanian and Slovenien characters) */
	/* uppercase is even  */
	{
		return (DECUMA_UNICODE) (ch + 1);
	}
	else if (ch >= 0x0410 && ch <= 0x042F) /* Cyrillic (Basic Russian) */
	{
		return (DECUMA_UNICODE) (ch + 0x20);
	}
	else if (ch >= 0x0400 && ch <= 0x040F) /* Cyrillic (Cyrillic extention (Serbian, Ukrainian, Bulgarian)) */
	{
		return (DECUMA_UNICODE) (ch + 0x50);
	}
	else if (ch == 0x0490) /* Cyrillic (Extended cyrillic (Ukrainian GHE WITH UPTURN)) */
	{
		return (DECUMA_UNICODE) (ch + 1);
	}
	/*--- BEGIN SPECIAL DECUMA RULES */
	else if ( bDecumaSpecial && ch == '-' ) {
		return '_';	
	}
	else if ( bDecumaSpecial && ch == '\'' ){	
		return ',';
	}
	/*--- END SPECIAL DECUMA RULES */
	else {
		return ch;
	}
} /* decumaUnicodeToLower() */


DECUMA_BOOL decumaUnicodeIsLower(DECUMA_UNICODE ch, int bDecumaSpecial)
{
	if ( ch == 0x0390 || (ch >= 0x03AC && ch <= 0x03CE) ) { /* Greek block */
		return TRUE;
	}
	else if ( ch >= 'a' && ch <= 'z') {
		return TRUE;
	}
	else if ( ch >= 0xE0 && ch <= 0xF6 ) { /* '�' to '�' */
		return TRUE;
	}
	else if ( ch >= 0xF8 && ch <= 0xFF) { /* '�' to '�' */
		return TRUE;
	}
	else if ( ch >= 0x0100 && ch <= 0x0137 && ch%2 == 1) /*Latin Extended-A */
	/* lowercase is odd */
	{
		return TRUE;
	}
	else if ( ch >= 0x0139 && ch <= 0x0148 && ch%2 == 0) /*Latin Extended-A */
	/* lowercase is even */
	{
		return TRUE;
	}
	else if ( ch >= 0x014A && ch <= 0x0177 && ch%2 == 1) /*Latin Extended-A */
	/* lowercase is odd */
	{
		return TRUE;
	}
	else if ( ch >= 0x0179 && ch <= 0x017E && ch%2 == 0) /*Latin Extended-A */
	/* lowercase is even */
	{
		return TRUE;
	}
	else if ( ch == 0x0138 || ch == 0x017F) /*Latin Extended-A */
	/* lowercase without corresponding uppercase */
	{
		return TRUE;
	}
	else if ( ch >= 0x200 && ch <= 0x021B && ch%2 == 1) /*Latin Extended-B */
	/* (Croatian, Romanian and Slovenien characters) */
	/* lowercase is odd  */
	{
		return TRUE;
	}
	else if ( ch >= 0x3041 && ch <= 0x30FE) /*hiragana & katakana */
	{
		return TRUE;
	}
	else if (ch >= 0x0430 && ch <= 0x044F) /* Cyrillic (Basic Russian) */
	{
		return TRUE;
	}
	else if (ch >= 0x0450 && ch <= 0x045F) /* Cyrillic (Cyrillic extention (Serbian, Ukrainian, Bulgarian)) */
	{
		return TRUE;
	}
	else if (ch == 0x0491) /* Cyrillic (Extended cyrillic (Ukrainian GHE WITH UPTURN)) */
	{
		return TRUE;
	}
	/*--- BEGIN SPECIAL DECUMA RULES */
	else if ( bDecumaSpecial && ch == '_' ) {
		return TRUE;	
	}
	else if ( bDecumaSpecial && ch == ',' ){	
		return TRUE;
	}
	/*--- END SPECIAL DECUMA RULES */
	else {
		return FALSE;
	}
} /* decumaUnicodeIsLower() */

DECUMA_BOOL decumaUnicodeIsUpper(const DECUMA_UNICODE * ch, int bDecumaSpecial)
{
	if ( *ch == 0x0386 || (*ch >= 0x0388 && *ch <= 0x038A) || *ch == 0x038C || (*ch >= 0x038E && *ch <= 0x038F) ||
		 (*ch >= 0x0391 && *ch <= 0x03AB && *ch != 0x03A2)) { /* Greek blocks */
		return TRUE;
	}
	else if ( *ch >= 'A' && *ch <= 'Z') {
		return TRUE;
	}
	else if ( *ch >= 0xC0 && *ch <= 0xD6 ) { /* '�' to '�' */
		return TRUE;
	}
	else if ( *ch >= 0xD8 && *ch <= 0xDE) { /* '�' && ch <= '�' */
		return TRUE;
 	}
	else if ( *ch == 0x0178 ){	/* Y_DIAERESIS */
		return TRUE;
	}
	else if ( *ch >= 0x0100 && *ch <= 0x0137 && *ch%2 == 0) /* Latin Extended-A */
	/* uppercase is even */
	{
	    return TRUE;
	}
	else if ( *ch >= 0x0139 && *ch <= 0x0148 && *ch%2 == 1) /* Latin Extended-A */
	/* uppercase is odd */
	{
		return TRUE;
	}
	else if ( *ch >= 0x014A && *ch <= 0x0177 && *ch%2 == 0) /* Latin Extended-A */
	/* uppercase is even */
	{
		return TRUE;
	}
	else if ( *ch >= 0x0179 && *ch <= 0x017E && *ch%2 == 1) /* Latin Extended-A */
	/* uppercase is odd */
	{
		return TRUE;
	}
	else if ( *ch >= 0x200 && *ch <= 0x021B && *ch%2 == 0) /* Latin Extended-B */
	/* (Croatian, Romanian and Slovenien characters) */
	/* uppercase is even  */
	{
		return TRUE;
	}
	else if (*ch >= 0x0410 && *ch <= 0x042F) /* Cyrillic (Basic Russian) */
	{
		return TRUE;
	}
	else if (*ch >= 0x0400 && *ch <= 0x040F) /* Cyrillic (Cyrillic extention (Serbian, Ukrainian, Bulgarian)) */
	{
		return TRUE;
	}
	else if (*ch == 0x0490) /* Cyrillic (Extended cyrillic (Ukrainian GHE WITH UPTURN)) */
	{
		return TRUE;
	}
	/*--- BEGIN SPECIAL DECUMA RULES */
	else if ( bDecumaSpecial && *ch == '-' ) {
		return TRUE;	
	}
	else if ( bDecumaSpecial && *ch == '\'' ){	
		return TRUE;
	}
	/*--- END SPECIAL DECUMA RULES */
	else {
		return FALSE;
	}
} /* decumaUnicodeIsUpper() */

DECUMA_BOOL decumaUnicodeIsLetter(DECUMA_UNICODE ch)
{
	return decumaUnicodeIsLower(ch, 0) || decumaUnicodeIsUpper(&ch, 0) || /* Latin, Cyrillic, Greek */
		( 0x5D0 <= ch && ch <= 0x5EA ) || /* Hebrew */
		( 0x621 <= ch && ch <= 0x63A ) || ( 0x641 <= ch && ch <= 0x64A); /* Arabic */
} /* decumaUnicodeIsLetter() */

int decumaIToA (int nVal, int nRadix, char *pBuf, int nBufLen)
{
	int nDigits, nTmpVal, bIsNeg = 0, i;

	decumaAssert (pBuf);
	decumaAssert (nRadix >= 2 && nRadix <= 16);

	nTmpVal = nVal;
	for (nDigits = 0; nTmpVal != 0; nTmpVal /= nRadix, nDigits++)
		;

	bIsNeg = (nVal < 0);

	if (nVal == 0)
		nDigits = 1;

	if (nDigits + bIsNeg + 1 > nBufLen)
		return 0;

	pBuf += nDigits + bIsNeg;

	*pBuf-- = '\0';

	for (i = 0; i < nDigits; nVal /= nRadix, i++) {
		int n = (bIsNeg) ? - (nVal % nRadix) : (nVal % nRadix);
		*pBuf-- = "0123456789ABCDEF"[n];
	}

	if (bIsNeg) 
		*pBuf = '-';

	return nDigits + bIsNeg;
}


int decumaUToA (unsigned int nVal, int nRadix, char *pBuf, int nBufLen)
{
	int nDigits, i;
	unsigned int nTmpVal;

	decumaAssert (pBuf);
	decumaAssert (nRadix >= 2 && nRadix <= 16);

	nTmpVal = nVal;
	for (nDigits = 0; nTmpVal != 0; nTmpVal /= nRadix, nDigits++)
		;

	if (nVal == 0)
		nDigits = 1;

	if (nDigits + 1 > nBufLen)
		return 0;

	pBuf += nDigits;

	*pBuf-- = '\0';

	for (i = 0; i < nDigits; nVal /= nRadix, i++) {
		int n = nVal % nRadix;
		*pBuf-- = "0123456789ABCDEF"[n];
	}

	return nDigits;
}


/* returns 1 on success, 0 on fail */
int decumaAToI(char * pStr, int nStrLen, int nRadix, int * pnResult)
{
	char pRadixStringUC[] = "ABCDEF";
	char pRadixStringLC[] = "abcdef";

	int  nRadixLimitUC = 0;
	int  nRadixLimitLC = 0;
	int  nRadixLimitNum;
	
	int  bIsNegative = 0;
	
	int nIdx = 0;
	int nStartIdx;

	int nResult = 0;

	decumaAssert (nStrLen >= 0);
	decumaAssert (pStr);
	decumaAssert (nRadix >= 2 && nRadix <= 16);

	if (nRadix > 10) {
		nRadixLimitUC  = pRadixStringUC[nRadix - 11];
		nRadixLimitLC  = pRadixStringLC[nRadix - 11];
	}

	nRadixLimitNum = (nRadix > 10) ? '9' : nRadix + '0' - 1;

	/* skip whitespace */
	while ((pStr[nIdx] == ' ' || pStr[nIdx] == '\t' || pStr[nIdx] == '\n' || pStr[nIdx] == '\r') && nIdx < nStrLen) {
		++nIdx;
	}

	if (nIdx >= nStrLen) return 0;

	/* Negative? */
	if (pStr[nIdx] == '-') {
		bIsNegative = 1;
		++nIdx;
		if (nIdx >= nStrLen) return 0;
	}
	
	nStartIdx = nIdx;
	
	while (nIdx < nStrLen && pStr[nIdx] != 0) {
		int nTerm;
		if (pStr[nIdx] >= '0' && pStr[nIdx] <= nRadixLimitNum) {
			nTerm = pStr[nIdx] - '0';
		}
		else if (nRadix > 10) {
			if (pStr[nIdx] >= 'a' && pStr[nIdx] <= nRadixLimitLC) {
				nTerm = 10 + pStr[nIdx] - 'a';
			}
			else if (pStr[nIdx] >= 'A' && pStr[nIdx] <= nRadixLimitUC) {
				nTerm = 10 + pStr[nIdx] - 'A';
			}
			else {
				break;
			}
		}
		else {
			break;
		}
		
		nResult = nResult * nRadix + nTerm;
		nIdx++;
	}
	
	if (nIdx == nStartIdx) return 0;

	*pnResult = bIsNegative ? -nResult : nResult;

	return nIdx;
}

/*********** end of file **************/
