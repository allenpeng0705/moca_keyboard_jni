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


#ifndef decumaDataTypes_h
#define decumaDataTypes_h

#include "decumaBasicTypes.h"
#include "decumaBasicTypesMinMax.h"
#include "decumaUnicodeTypes.h"


#ifdef __cplusplus
extern "C" {
#endif


/* The definitions of (U)INT32, (U)INT16 and (U)INT8 are here just for
compability with old products in the long run they should be replaced by
DECUMA_(U)INT32, DECUMA_(U)INT16 nad DECUMA_(U)INT8.
They are better to use because they avoid conflicts with previously
declared constants*/

#if defined(_WIN32_WCE) && defined(_DEBUG)
#include <windows.h> /* Needed to get asserts working under WinCE */
/* "windows.h" Must be included in order to get assert on WinCE */
/* and "windows.h" defines the types INT32 and UINT32. */

#elif !defined( _BASETSD_H_ )
#ifdef NEED_INT_TYPES
typedef DECUMA_UINT32	UINT32;
typedef DECUMA_INT32	INT32;
#endif
#endif

#ifdef NEED_INT_TYPES
typedef DECUMA_UINT16	UINT16;
typedef DECUMA_INT16	INT16;

typedef DECUMA_UINT8	UINT8;
typedef DECUMA_INT8		INT8;
#endif

/*
typedef enum _tagDECUMA_BOOL{
	D_FALSE = 0,
	D_TRUE = 1
} DECUMA_BOOL;
*/
typedef int DECUMA_BOOL;



#ifndef _WINDEF_ /* windef.h used by for instance gui_interpret_sample */
				 /* has already defined FALSE, TRUE and typedef:ed */
				 /* BOOL. These definitions will be used in this */
				 /* case to avoid redefinition. Only affects compiler */
				 /* warnings. */



#if !defined(TRUE) && !defined(FALSE) && !defined(TRUE_DEFINED) && !defined(FALSE_DEFINED)
#define TRUE_DEFINED
#define FALSE_DEFINED

#define FALSE 0
#define TRUE  1
/*
#if defined( ONPALM_5 ) && !defined( ONPALM_ARMLET )
// BOOL is defined for Palm OS5.
#include <PalmTypes.h>
#define FALSE 0
#define TRUE 1

#elif !defined( __MWERKS__)
typedef enum {
	FALSE = 0,
	TRUE = 1
} BOOL;
#else
  // METROWERKS
#define FALSE 0
#define TRUE 1
#endif  // ONPALM_5
*/
#endif /* !defined(TRUE_DEFINED) && !defined(FALSE_DEFINED) */
#endif  /* _WINDEF_ */

#ifndef NULL
#define NULL 0
#endif

#define MAX_UINT32  MAX_DECUMA_UINT32   
#define MAX_INT32   MAX_DECUMA_INT32    
#define MIN_INT32   MIN_DECUMA_INT32    

#define MAX_UINT16  MAX_DECUMA_UINT16	
#define MAX_INT16	MAX_DECUMA_INT16		
#define MIN_INT16   MIN_DECUMA_INT16		

#define MAX_UINT8   MAX_DECUMA_UINT8    
#define MAX_INT8    MAX_DECUMA_INT8	   
#define MIN_INT8    MIN_DECUMA_INT8		

#define MAX_INT			( -((int) ((-1)<<(sizeof(int)*8 - 1)) + 1) )
#define MIN_INT			(   (int) ((-1)<<(sizeof(int)*8 - 1))      )

#define MAX_INT32_SQRT   (46340)   /* square root of MAX_INT32 */
#define MAX_UINT32_SQRT  (MAX_UINT16)   /* square root of MAX_UINT32 */

typedef struct _tagINT16_POINT{
	DECUMA_INT16 x;
	DECUMA_INT16 y;
} INT16_POINT;

typedef struct _tagINT32_POINT{
	DECUMA_INT32 x;
	DECUMA_INT32 y;
} INT32_POINT;

typedef struct _tagINT16_ARC {
	INT16_POINT *point;
	DECUMA_UINT32 noPoints;
} INT16_ARC;

typedef struct _tagINT32_ARC {
	INT32_POINT *point;
	DECUMA_UINT32 noPoints;
} INT32_ARC;

typedef struct _tagINT16_CURVE {
	INT16_ARC *arc;
	DECUMA_UINT32 noArcs;
} INT16_CURVE;

typedef struct _tagINT32_CURVE {
	INT32_ARC *arc;
	DECUMA_UINT32 noArcs;
} INT32_CURVE;

#ifdef __cplusplus
} /*extern "C" { */
#endif


#endif
