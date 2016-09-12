/* This file was sent by Erland Unruh, but can also be found in the Tegic CVS:
 *  http://se-cvs01/viewvc/et9/udbwords/
 */

/*******************************************************************************
;*******************************************************************************
;**                                                                           **
;**                  COPYRIGHT 2006-2009 NUANCE COMMUNICATIONS                **
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
;**  Description: Mini miscellaneous tools for ET9.                           **
;**                                                                           **
;**                                                                           **
;*******************************************************************************
;******************************************************************************/
#ifndef ET9MISC_H
#define ET9MISC_H


/* don't mangle the function name if compile under C++ */
#if defined(__cplusplus)
extern "C" {
#endif


#ifdef ET9_DEBUG

#include "stdio.h"
#include "string.h"
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
#include <wchar.h>
#include <memory.h>
#endif /* ET9ACTIVATEMISCSTDCLIBUSE */


ET9STATUS ET9FARCALL _ET9CheckFundamentalTypes(void);


#ifdef ET9ACTIVATEMISCSTDCLIBUSE

#define _ET9SymCopy(des, src, size) wmemcpy(des, src, size)
#define _ET9SymMove(des, src, size) wmemmove(des, src, size)
#define _ET9ClearMem(des, size) memset(des, 0, size)
#define _ET9ByteCopy(des, src, size) memcpy(des, src, size)
#define _ET9ByteMove(des, src, size) memcpy(des, src, size)
#define _ET9ByteSet(des, size, s) memset(des, s, size)
#define _ET9WordSet(des, size, s) wmemset(des, s, size)

#else /* ET9ACTIVATEMISCSTDCLIBUSE */

void ET9FARCALL _ET9SymCopy(ET9SYMB *des, ET9SYMB *src, ET9U32 size);
void ET9FARCALL _ET9SymMove(ET9SYMB *dst, ET9SYMB *src, ET9U32 size);
void ET9FARCALL _ET9ClearMem(ET9U8 *des, ET9U32 size);
void ET9FARCALL _ET9ByteCopy(ET9U8 *des, ET9U8 const *src, ET9U32 size);
void ET9FARCALL _ET9ByteMove(ET9U8 *des, ET9U8 *src, ET9U32 size);
void ET9FARCALL _ET9ByteSet(ET9U8 *des, ET9U32 size, ET9U8 s);
void ET9FARCALL _ET9WordSet(ET9U16 *des, ET9U32 size, ET9U16 s);

#endif /* ET9ACTIVATEMISCSTDCLIBUSE */


/* End don't mangle the function name if compile under C++ */
#if defined(__cplusplus)
    }
#endif

#endif /* ET9MISC_H */
/* ----------------------------------< eof >--------------------------------- */

