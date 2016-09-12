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

  This file provides the interface to initialize, destroy, access
  poperties of and operate on cjkArcSession objects.

  The main function of a cjkArcSession is to store DECUMA_ARCs
  for later access.

\******************************************************************/

#ifndef CJK_ARC_SESSION_H
#define CJK_ARC_SESSION_H

#include "dltConfig.h"

typedef struct _CJK_ARC_SESSION CJK_ARC_SESSION;

typedef enum _tagCJK_SAMPLING_RULE {
   NonHanSampling=0,
   ChineseNonHanSampling=1,
   HanSampling=2,
   HangulSampling=3
} CJK_SAMPLING_RULE;

#include "cjkCompressedCharacter.h"
#include "decuma_hwr_types.h"
#include "decumaHwrSampler.h"
#include "decumaStatus.h"
#include "decumaCurve.h"
#include "decumaRuntimeMalloc.h"

#define CJK_ARC_SESSION_POINT_BUFFER_SIZE 1000
#define CJK_ARC_SESSION_ARC_BUFFER_SIZE 80

/*
    Returns the size of a CJK_ARC_SESSION object.
*/
DECUMA_HWR_PRIVATE int cjkArcSessionGetSize(void);

/***
	Resets the internal input curve to an empty curve.
**/

DECUMA_HWR_PRIVATE void cjkArcSessionResetCurve(CJK_ARC_SESSION* pCjkArcSession);

/***
    Initializes pCjkArcSession. Memory for pCjkArcSession have been allocated
	by the caller and the size of the memory must be at least that
	returned by cjkArcSessionGetSize().

    An already initialized pCjkArcSession must be destroyed before it can
	be initialized again.

    Returns decumaNoError if input parameters are valid.
**/
DECUMA_HWR_PRIVATE DECUMA_STATUS cjkArcSessionInit(CJK_ARC_SESSION* pCjkArcSession, CJK_CC_COMPRESS_SCRATCHPAD * pScratchpad);

/*
    Releases memory allocated for added arcs and destroys pCjkArcSession. pCjkArcSession
	must be re-initialized before it can be used again.
*/
DECUMA_HWR_PRIVATE void cjkArcSessionDestroySampler(CJK_ARC_SESSION* pCjkArcSession);

/*
    Stores a pDecumaArc to  pCjkArcSession. Memory is allocated accordingly.
	
	If bNextArcIsConcurrent is set to 1, the arc session will remember that the next
	added arc was entered simultaneously as this one. (multi-touch)
*/

DECUMA_HWR_PRIVATE DECUMA_STATUS cjkArcSessionStartNewArc(CJK_ARC_SESSION * pCjkArcSession, DECUMA_UINT32 arcID);

/*Adds point(s) to a started arc  */
DECUMA_HWR_PRIVATE DECUMA_STATUS cjkArcSessionAddPoint(CJK_ARC_SESSION * pCjkArcSession,
									DECUMA_COORD x, DECUMA_COORD y, 
									DECUMA_UINT32 arcID);

DECUMA_HWR_PRIVATE DECUMA_STATUS cjkArcSessionAddPoints(CJK_ARC_SESSION * pCjkArcSession,
							  DECUMA_POINT * pPts, int nPts, 
							  DECUMA_UINT32 arcID,
							  int * pnPtsAdded);


/*Ends the arc and removes it from the session */
DECUMA_HWR_PRIVATE DECUMA_STATUS cjkArcSessionCancelArc(CJK_ARC_SESSION * pCjkArcSession,DECUMA_UINT32 arcID);

/*Ends the sampling and adds the arc lasst in the "permanent storage" of the arc session */
DECUMA_HWR_PRIVATE DECUMA_STATUS cjkArcSessionCommitArc(CJK_ARC_SESSION * pCjkArcSession,DECUMA_UINT32 arcID);

#ifdef CJK_ENABLE_INTERNAL_API
DECUMA_HWR_PRIVATE DECUMA_STATUS cjkArcSessionAddCurve(CJK_ARC_SESSION * pCjkArcSession, const DECUMA_CURVE * pCurve);
#endif

DECUMA_HWR_PRIVATE int cjkArcSessionGetUncommittedArcCount(CJK_ARC_SESSION * pCjkArcSession);

DECUMA_HWR_PRIVATE DECUMA_STATUS cjkArcSessionGetUncommittedArcID(CJK_ARC_SESSION * pCjkArcSession, int idx,
										DECUMA_UINT32 * pSrcID);

/*
	Returns 1 if the arcs with index1 and index2 are concurrent (part of the same "multi-touch-sequence")
*/
DECUMA_HWR_PRIVATE int cjkArcSessionArcsAreConcurrent(const CJK_ARC_SESSION* pCjkArcSession,
								   int nIndex1,int nIndex2);

/*
    Returns number of arcs of pCjkArcSession, i.e. the number of added arcs.
*/
DECUMA_HWR_PRIVATE int cjkArcSessionGetArcCount(const CJK_ARC_SESSION* pCjkArcSession);

/*
    Fills a vector with arc indexes to the arcs that were added "simultaneously" at arcTimeStamp.
	Returns the number of indexes written
*/
DECUMA_HWR_PRIVATE int cjkArcSessionGetConcurrentArcs(const CJK_ARC_SESSION* pCjkArcSession, DECUMA_UINT32 arcTimeStamp,
								   int * pArcIdxs, int nMaxArcs);


DECUMA_HWR_PRIVATE DECUMA_SAMPLER_DATA * cjkArcSessionGetSampler(CJK_ARC_SESSION * pCjkArcSession);

DECUMA_HWR_PRIVATE CJK_SAMPLING_RULE cjkArcSessionGetSamplingRule(CJK_ARC_SESSION * pCjkArcSession);

/*
	There are two different rules for dltCCharCompress and this specifies which rule to use when compressing the data
	to the internal format.
*/

DECUMA_HWR_PRIVATE void cjkArcSessionSetSamplingRule(CJK_ARC_SESSION * pCjkArcSession, DECUMA_UINT32 categorymask, int nStrokes);

/*
	Get the compressed character stored in CJK_ARC_SESSION after cjkArcSessionCommitArc()
*/

DECUMA_HWR_PRIVATE CJK_COMPRESSED_CHAR * cjkArcSessionGetCompressedChar(CJK_ARC_SESSION * pCjkArcSession);

DECUMA_HWR_PRIVATE CJK_COMPRESSED_CHAR_DATA * cjkArcSessionGetCCharData(CJK_ARC_SESSION * pCjkArcSession);

DECUMA_HWR_PRIVATE CJK_CC_COMPRESS_SCRATCHPAD * cjkArcSessionGetScratchpad(CJK_ARC_SESSION * pCjkArcSession);

#endif /*CJK_ARC_SESSION_H */
