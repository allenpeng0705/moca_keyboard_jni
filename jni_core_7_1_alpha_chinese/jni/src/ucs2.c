#include <assert.h>
#include <stdarg.h>
#include <stddef.h>
#include "ucs2.h"

static const unsigned char  FirstByteMark[7]   = {0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC};
static const unsigned char utf8CharBytes[16] =
{
    1,      /* 0000xxxx   1 byte UTF-8 char */
    1,      /* 0001xxxx   1 byte UTF-8 char */
    1,      /* 0010xxxx   1 byte UTF-8 char */
    1,      /* 0011xxxx   1 byte UTF-8 char */
    1,      /* 0100xxxx   1 byte UTF-8 char */
    1,      /* 0101xxxx   1 byte UTF-8 char */
    1,      /* 0110xxxx   1 byte UTF-8 char */
    1,      /* 0111xxxx   1 byte UTF-8 char */
    0,      /* 1000xxxx   continuation byte */
    0,      /* 1001xxxx   continuation byte */
    0,      /* 1010xxxx   continuation byte */
    0,      /* 1011xxxx   continuation byte */
    2,      /* 1100xxxx   2 byte UTF-8 char */
    2,      /* 1101xxxx   2 byte UTF-8 char */
    3,      /* 1110xxxx   3 byte UTF-8 char */
    0,      /* 1111xxxx   not legal for 16-bit UCS-2 */
};
int utf8ToUcs2(ucs2_Char* dest, int destMaxLen, const char* src)
{
    int copy = 0;
    /* Copy UTF-8 characters until reach the end*/
    while (copy < destMaxLen && *src != '\0') {
        switch (utf8CharBytes[((unsigned char) *src) >> 4]) {
            /* Invalid character - skip over it */
            case 0:
                src++;
                break;
            /* 1 byte, 7-bit character  */
            case 1:
                dest[copy++] = (ucs2_Char)(*src & 0x7F);
                src++;
                break;
            /* 2 byte, 11-bit character  */
            case 2:
                dest[copy++] = (ucs2_Char) (((*src     & 0x1F) << 6) |
                                             (*(src+1) & 0x3F));
                src += 2;
                break;
            /* 3 byte, 16-bit character  */
            case 3:
                dest[copy++] = (ucs2_Char)( ((*src     & 0x0F) << 12) |
                                            ((*(src+1) & 0x3F) <<  6) |
                                             (*(src+2) & 0x3F));
                src += 3;
                break;
            /* Invalid character - skip over it */
            default:
                src++;
                break;
        }
    }
    if (copy < destMaxLen) {
        dest[copy] = (ucs2_Char)'\0';
    }
    return copy;
}

static int UnicdeCharToUtf8(ucs2_Char     sIn,
                            char *pbOut)
{
    int  nBytesToWrite = 3;
    const ucs2_Char sByteMask = 0xBF;
    const ucs2_Char sByteMark = 0x80;

    /* It would be nice to avoid this conditional */
    if (sIn < 0x80) {
        nBytesToWrite = 1;
    }
    else if (sIn < 0x800) {
        nBytesToWrite = 2;
    }
    pbOut += nBytesToWrite - 1;
    /* Write the converted bytes to the target buffer */
    switch (nBytesToWrite) { /* note: code falls through cases! */
        case 3:
            *pbOut-- = (char)((sIn | sByteMark) & sByteMask);
            sIn >>= 6;
        case 2:
            *pbOut-- = (char)((sIn | sByteMark) & sByteMask);
            sIn >>= 6;
        case 1:
            *pbOut   = (char)(sIn | FirstByteMark[nBytesToWrite]);
    }
    return nBytesToWrite;
}
int ucs2ToUtf8(char* dest, int destMaxLen, const ucs2_Char* src)
{
    int i, nLen = 0;
    for ( i = 0; src[i]; i++)
    {
        nLen += UnicdeCharToUtf8(src[i], (dest + nLen));
    }
    dest[nLen] = 0;
    return nLen;

}

int ucs2_strncmp(const ucs2_Char* src1, const ucs2_Char* src2, int len)
{
    if (len == 0) {
        return 0;
    }

    do {
         if (*src1 != *src2) {
             return (*src1 - *src2);
         }
         src1++;
         src2++;
     } while (--len != 0);

     return 0;
}
