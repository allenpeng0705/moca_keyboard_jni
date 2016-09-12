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

#pragma once

#ifndef CJK_COARSE_FEATURES_DATA_H_
#define CJK_COARSE_FEATURES_DATA_H_

#if defined(_DEBUG) && !defined(CJK_COARSE_FEATURES_C)
#error Direct data access in debug mode only allowed from cjkArcSession.c
#endif

#include "decumaBasicTypes.h"
#include "decumaKMeans.h"
#include "cjkDatabaseLimits.h"
#include "cjkCompressedCharacter.h"

/* --------------------------------------------------------------------------
 * Data types
 * -------------------------------------------------------------------------- */

/**
* The peripheral segment features are derived from an article on coarse classification
* of traditional Chinese characters by Kuo-Chin Fan.
*/

typedef struct _tagPERIPHERAL_SEGMENT {
	int high;
	int  low;
	DECUMA_UINT32 dir;
	CJK_GRIDPOINT gp[2];
} PERIPHERAL_SEGMENT;

#define NBR_PERIPHERAL_SEGMENTS 4

typedef struct _tagPERIPHERAL_FEATURE {
	DECUMA_UINT32 nSegments;
	PERIPHERAL_SEGMENT segments[NBR_PERIPHERAL_SEGMENTS];
} PERIPHERAL_FEATURE;

typedef struct _tagSWIFT_DIST {
	DECUMA_UINT8  splitpoint;
	DECUMA_UINT8  splitnum;
	DECUMA_UINT8  x1;
	DECUMA_UINT8  x2;
	DECUMA_UINT8  y1;
	DECUMA_UINT8  y2;
	DECUMA_UINT8  y3;
	DECUMA_UINT8  y4;
} SWIFT_DIST;

struct _tagK_MEANS_VALUE {
	unsigned char value;
	unsigned char idx;
};

typedef struct {
	unsigned char idx;
	unsigned char maxPrevVal[2]; /* first and second  */
	unsigned char minAfterVal;
} CJK_SPLIT_IDX;

/** Scratchpad used in the function @ref cjkCoarseFeatures */
struct _tagCJK_COARSE_FEATURES_SCRATCHPAD {
	DECUMA_INT32 sx[MAXPNTSPERCHAR];
	DECUMA_INT32 sy[MAXPNTSPERCHAR];
	DECUMA_INT32 st[MAXPNTSPERCHAR];
	DECUMA_INT32 sxx[MAXPNTSPERCHAR];
	DECUMA_INT32 syy[MAXPNTSPERCHAR];
	DECUMA_INT32 sty[MAXPNTSPERCHAR];
	DECUMA_INT32 stt[MAXPNTSPERCHAR];

	K_MEANS_VALUE kMeanValues[MAXPNTSPERCHAR];
	CJK_GRIDPOINT gridPoints[MAXPNTSPERCHAR];

	CJK_COMPRESSED_CHAR * pCCFirstStrokeIn;
	CJK_COMPRESSED_CHAR * pCCFirstStrokeDb;
};


#endif /* CJK_COARSE_FEATURES_DATA_H_*/
