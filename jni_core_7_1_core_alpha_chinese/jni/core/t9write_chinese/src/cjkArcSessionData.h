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


/************************************************** Description ***\

  This file contains private data structure definitions of
  cjkArcSession objects.

\******************************************************************/

#ifndef CJK_ARC_SESSION_DATA_H
#define CJK_ARC_SESSION_DATA_H

#if defined(_DEBUG) && !defined(CJK_ARC_SESSION_C)
#error Direct data access in debug mode only allowed from cjkArcSession.c
#endif

#include "dltConfig.h"

#include "decuma_hwr_types.h"
#include "cjkCompressedCharacter.h"
#include "cjkArcSession.h"

struct _CJK_ARC_SESSION
{
	CJK_SAMPLING_RULE samplingRule;
	CJK_COMPRESSED_CHAR * pCChar;
	CJK_COMPRESSED_CHAR_DATA CCharData[CC_MAXCCHARSIZE];

	CJK_CC_COMPRESS_SCRATCHPAD * pScratchpad;

	/* OBS: Because cjk currently does not support multi-touch, these two fields
	 * will be quite sufficient for handling the API restrictions for input IDs
	 */
	DECUMA_POINT pointBuffer[CJK_ARC_SESSION_POINT_BUFFER_SIZE];
	DECUMA_UINT32 nUsedPoints;
	DECUMA_ARC	 arcBuffer[CJK_ARC_SESSION_ARC_BUFFER_SIZE];
	DECUMA_CURVE curve;
	DECUMA_UINT32 externalArcIDs[CJK_ARC_SESSION_ARC_BUFFER_SIZE];
	DECUMA_UINT32 nCommittedArcs;
};

#endif /* CJK_ARC_SESSION_DATA_H */
