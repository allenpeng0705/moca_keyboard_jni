/*****************************************************************************
* COPYRIGHT ZI AND SUBJECT TO CONFIDENTIALITY RESTRICTIONS                   *
*                                                                            *
* This file is the confidential and proprietary property of Zi Corporation   *
* of Canada, Inc. ("Zi") or one of its affiliates.                           *
******************************************************************************/

#ifndef CJK_TYPES_H_
#define CJK_TYPES_H_

#pragma once

#include "decumaBasicTypes.h"
#include "decumaBasicTypesMinMax.h"

/** Maximum distance measure for @ref BestList elements. */
#define MAX_CJK_DISTANCE MAX_DECUMA_UINT32


/* -------------------------------------------------------------------------
 * Common types
 * ------------------------------------------------------------------------- */


/** */
typedef int
	CJK_BOOLEAN;

/** */
typedef DECUMA_UINT16 
	CJK_UNICHAR;

/**
 * A gridpoint is stored as two four-bit numbers (nybbles) in a byte.
 * The x-coordinate is in the low nybble and the
 * y-coordinate is in the high nybble.
 */
typedef DECUMA_UINT8 
	CJK_GRIDPOINT;

#define MAX_CJK_GRIDPOINT MAX_DECUMA_UINT8
#define MIN_CJK_GRIDPOINT MIN_DECUMA_UINT8

/** The database is indexed with the type DLTDB_INDEX. */
typedef DECUMA_UINT16 
	DLTDB_INDEX;

/** Distance measure for @ref BestList elements. */
typedef DECUMA_UINT32 
	CJK_DISTANCE;

/** Position of an element in a @ref BestList. */
typedef DECUMA_UINT32
	CJK_BESTLIST_POSITION;




/* -------------------------------------------------------------------------
 * Deprecated types
 * ------------------------------------------------------------------------- */

#if 0
/* TODO this type seems redundant */
#ifndef _SYS_TYPES_H
typedef unsigned short ushort;
#endif
#endif

#if 0
/* TODO this type seems redundant */
typedef struct {
   short x;
   short y;
} Gpshort;
#endif

#if 0
/* TODO this type seems redundant */
typedef struct {
   DECUMA_INT32 n;
   Gpshort * pxy;
} DECUMA_ARC;
#endif

#if 0
/* TODO this type seems redundant */
#ifndef uchar
typedef unsigned char uchar;
#endif
#endif



#endif /* CJK_TYPES_H_ */
