/*******************************************************************************
;*******************************************************************************
;**                                                                           **
;**                  COPYRIGHT 2001-2011 NUANCE COMMUNICATIONS                **
;**                                                                           **
;**               NUANCE COMMUNICATIONS PROPRIETARY INFORMATION               **
;**                                                                           **
;**     This software is supplied under the terms of a license agreement      **
;**     or non-disclosure agreement with Nuance Communications and may not    **
;**     be copied or disclosed except in accordance with the terms of that    **
;**     agreement.                                                            **
;**                                                                           **
;**                                                                           **
;*******************************************************************************
;**                                                                           **
;**     FileName: et9misc.h                                                   **
;**                                                                           **
;**  Description: Miscellaneous tools for ET9.                                **
;**                                                                           **
;*******************************************************************************
;******************************************************************************/

#ifndef ET9MISC_H
#define ET9MISC_H 1


#include "et9api.h"

/* don't mangle the function name if compile under C++ */
#if defined(__cplusplus)
extern "C" {
#endif


#ifdef ET9_DEBUG

#include <stdio.h>
#include <string.h>
#include <assert.h>

#ifndef ET9Assert
#define ET9Assert(e)        assert(e)
#endif

#else /* ET9_DEBUG */

#ifndef ET9Assert
#define ET9Assert(e)
#endif

#endif /* ET9_DEBUG */


#ifdef ET9ACTIVATEMISCSTDCLIBUSE
#include <string.h>
#endif /* ET9ACTIVATEMISCSTDCLIBUSE */



/*------------------------------------------------------------------
 * Macros
 *------------------------------------------------------------------*/

#define __ET9Abs(x)         ((x) < 0 ? -(x) : (x))
#define __ET9Min(x, y)      (((x) <= (y)) ? (x) : (y))
#define __ET9Max(x, y)      (((x) >= (y)) ? (x) : (y))
#define __ET9Pin(x, y, z)   ((y) < (x) ? (x) : ((y) > (z) ? (z) : (y)))

#define _ET9InitSimpleWord(pWord)   _ET9ClearMem((ET9U8*)pWord, sizeof(ET9SimpleWord))

#define _ET9CHECKGENSTATUS()        {if (wStatus) { return wStatus; }}                      /**< depricated macro... */

#define ET9_UNUSED(x) (void)(!!(x))

/*------------------------------------------------------------------
 * Functions
 *------------------------------------------------------------------*/



ET9STATUS ET9FARCALL _ET9CheckFundamentalTypes(void);
ET9STATUS ET9FARCALL _ET9CheckCharProps(void);

void ET9FARCALL _ET9BinaryToHex(ET9U8 byNumber, ET9SYMB *psDest);


#ifdef ET9ACTIVATEMISCSTDCLIBUSE

#define _ET9SymCopy(des, src, size)     memcpy((des),  (src), (size) * sizeof(ET9SYMB))
#define _ET9SymMove(des, src, size)     memmove((des), (src), (size) * sizeof(ET9SYMB))
#define _ET9ClearMem(des, size)         memset((des),  0,     (size))
#define _ET9ByteCopy(des, src, size)    memcpy((des),  (src), (size))
#define _ET9ByteMove(des, src, size)    memcpy((des),  (src), (size))
#define _ET9ByteSet(des, size, s)       memset((des),  (s),   (size))

#else /* ET9ACTIVATEMISCSTDCLIBUSE */

void ET9FARCALL _ET9SymCopy(ET9SYMB *des, ET9SYMB const *src, ET9U32 size);
void ET9FARCALL _ET9SymMove(ET9SYMB *dst, ET9SYMB const *src, ET9U32 size);
void ET9FARCALL _ET9ClearMem(ET9U8 *des, ET9U32 size);
void ET9FARCALL _ET9ByteCopy(ET9U8 *des, ET9U8 const *src, ET9U32 size);
void ET9FARCALL _ET9ByteMove(ET9U8 *des, ET9U8 const *src, ET9U32 size);
void ET9FARCALL _ET9ByteSet(ET9U8 *des, ET9U32 size, ET9U8 s);

#endif /* ET9ACTIVATEMISCSTDCLIBUSE */


void ET9FARCALL _ET9WordSet(ET9U16 *des, ET9U32 size, ET9U16 s);    /* obsolete */


ET9FLOAT ET9FARCALL _ET9acos_f(const ET9FLOAT fX);
ET9FLOAT ET9FARCALL _ET9atan2_f(const ET9FLOAT fY, const ET9FLOAT fX);
ET9FLOAT ET9FARCALL _ET9sqrt_f(const ET9FLOAT fX);

ET9FLOAT ET9FARCALL _ET9log_f(const ET9FLOAT fX);
ET9FLOAT ET9FARCALL _ET9pow_f(const ET9FLOAT fX, const ET9FLOAT fY);


ET9U8 ET9FARCALL _ET9SymbToUtf8(const ET9SYMB           sIn,
                                ET9U8           * const pbOut);

ET9U8 ET9FARCALL _ET9Utf8ToSymb(ET9U8      const * const pbIn,
                                ET9U8      const * const pbEnd,
                                ET9SYMB          * const psOut);

ET9U8 ET9FARCALL _ET9DecodeSpecialChar(ET9U8      const * const pbIn,
                                       ET9U8      const * const pbEnd,
                                       ET9SYMB          * const psOut);

ET9INT ET9FARCALL _ET9symbncmp(ET9SYMB  const * const psString1, ET9SYMB  const * const psString2, const ET9U16 wLen);

ET9U32 ET9FARCALL _ET9ByteStringCheckSum(ET9U8 const * const pbString);


#ifdef ET9_DEBUG
ET9STATUS ET9FARCALL _OpenLogFile();
void ET9FARCALL _CloseLogFile();
ET9STATUS ET9FARCALL _WriteLogFile(ET9U8 *pbyData, ET9U16 wDataLen);
#endif /* ET9_DEBUG */


/* End don't mangle the function name if compile under C++ */
#if defined(__cplusplus)
    }
#endif

#endif /* ET9MISC_H */


/* ----------------------------------< eof >--------------------------------- */

