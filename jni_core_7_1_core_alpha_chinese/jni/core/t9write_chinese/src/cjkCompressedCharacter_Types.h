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

#ifndef CJK_COMPRESSED_CHARACTER_DATA_H_
#define CJK_COMPRESSED_CHARACTER_DATA_H_

#if defined(_DEBUG) && !defined(CJK_CC_COMPRESS_C)
#error Direct data access in debug mode only allowed from dltCCCompress.c	
#endif

#include "decumaFeature.h"
#include "cjkCompressedCharacter.h"


/** @addtogroup DOX_CJK_CCHAR
 *  @{
 */

/** Scratchpad used in the function @ref dltCCharGetDTWDistance */
struct _tagCJK_CC_DTW_SCRATCHPAD {
	CJK_GRIDPOINT  bgp[MAXCMVERSIONS][MAXPNTSPERSTR];
	DECUMA_UINT8   bss[MAXCMVERSIONS][MAXPNTSPERSTR];
	DECUMA_UINT8   bcvfork[MAXPNTSPERSTR + 1];
	CJK_GRIDPOINT cc_dist_dtw_agp[MAXPNTSPERSTR + 2 * BW_DOUBLE]; /* static */
	DECUMA_UINT8  cc_dist_dtw_ass[MAXPNTSPERSTR + 2 * BW_DOUBLE]; /* static */
#ifdef CJK_ENABLE_INTERNAL_API
	CJK_DISTANCE  dtwMatrix[MAXPNTSPERSTR * MAXPNTSPERSTR];
	DECUMA_UINT32 pDtwAlignment[MAXPNTSPERSTR];
#endif
};

/** Scratchpad used in the function @ref dltCCharCompress */

struct _tagCJK_CC_COMPRESS_SCRATCHPAD
{
	short         xydata[CC_MAX_PT_BUFFER_SZ];
	DECUMA_POINT  gpsbuf[CC_MAXCCHARSIZE];
	DECUMA_INT32  split[16];
	DECUMA_INT32  ink[16];
	union {
		DECUMA_UINT32 mt_fintpts_buffer[CC_MAXCCHARSIZE];
		DECUMA_UINT8  cchar_data[CC_MAX_PT_BUFFER_SZ];
	} PT_BUFFER;
	DECUMA_UINT32 mt_walkfpnt_stack[CC_MAXCCHARSIZE];
};


struct _tagCJK_COMPRESSED_CHAR 
{
#ifdef CJK_ENABLE_INTERNAL_API
	CJK_COMPRESSED_CHAR_DATA * pCCData;
#else
	const CJK_COMPRESSED_CHAR_DATA * pCCData;
#endif
	DECUMA_INT32               nStrokes;
	DECUMA_INT32               nPoints;
	DECUMA_INT32               xSum;
	DECUMA_INT32               ySum;
	short                      xmin;
	short                      xmax;
	short                      ymin;
	short                      ymax;
	DLTDB_INDEX                index;
	DLTDB_ATTRIBUTE            attrib;

	DECUMA_UINT8               dtwCached;
	DECUMA_FEATURE           * pFeatures;
	DECUMA_UINT8               bFeaturesSet;

	DECUMA_ARC					 * pOriginalStrokes;

	/* TODO: Should be moved to cjkCoarseFeatures somehow */
	DECUMA_UINT8               dotstart;
	DECUMA_UINT8               bIsNotSpeaking;
	DECUMA_UINT8               bIsNotThreeDrops;
	DECUMA_UINT8               bIsNotSoil;
	DECUMA_UINT8               split;

};

/** @} */

#endif /* CJK_COMPRESSED_CHARACTER_DATA_H_ */
