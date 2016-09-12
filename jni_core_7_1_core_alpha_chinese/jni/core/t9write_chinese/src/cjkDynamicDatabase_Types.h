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

/* Type definitions for dltlib dynamic template database. 
 * This replaces what was previously in dltPersonalCharacter (hence the 'pec' 
 * abbreviation still used at a few places).
 */

#include "cjkCompressedCharacter.h"

#ifndef CJK_DYNAMIC_DATABASE_TYPES_H
#define CJK_DYNAMIC_DATABASE_TYPES_H

typedef struct {
	DECUMA_UNICODE unicode;
	DLTDB_ATTRIBUTE attrib;
	DECUMA_UINT32 categoryMask;
	CJK_COMPRESSED_CHAR_DATA ccData[CC_MAXCCHARSIZE]; /* It's not strictly nessescary to have a maximal buffer here, but we'll keep it simple for now */
} CJK_PECDATA;

struct _DECUMA_DYNAMIC_DB {
	DECUMA_UINT32 MagicNumber;
	DECUMA_UINT32 nEntries;
	CJK_PECDATA entries[1]; /* Actual, accessible size depends on memory allocated */
};

/* A magic 32-bit number that must never be equal to UDM_FORMAT_VERSION_NR */
/* TODO select something more appropriate, deadbeef just comes from the top of my head */
#define CJK_UDM_IMPLEMENTATION (0xdeadbeef)
#define CJK_UDM_INCREMENT 16

/* An arbitrary number greater than the number of templates in a static dictionary.
 * Entries in the best-list with indices greater than this are assumed to lie in
 * the dynamic dictionary
 */
#define DYNAMIC_DB_STARTINDEX 50000

#endif /*CJK_DYNAMIC_DATABASE_TYPES_H */
