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
  poperties of and operate on aslArcSession objects.

  The main function of a aslArcSession is to translate DECUMA_ARCs
  to ASL_ARCs and store them for later access.

\******************************************************************/

#ifndef ASL_ARC_SESSION_H
#define ASL_ARC_SESSION_H

#include "aslConfig.h"

typedef struct _ASL_ARC_SESSION ASL_ARC_SESSION;

#if !defined(_DEBUG)
#include "aslArcSessionMacros.h"
#endif

#include "decuma_hwr.h"
#include "decumaStatus.h"
#include "decumaCurve.h"
#include "decumaRuntimeMalloc.h"
#include "aslArc.h"

#define REFERENCE_SEGMENT_SEPARATION_SCALED 200 /*Scaled with 1000 for db-usage */
#define REFERENCE_SEGMENT_SEPARATION ((double)((double)REFERENCE_SEGMENT_SEPARATION_SCALED/1000))  /*Will be multiplied with the baseline-helpline distance  */


/*
    Returns the size of a ASL_ARC_SESSION object.
*/
DECUMA_HWR_PRIVATE int aslArcSessionGetSize(void);

/*
    Initializes pAslArcSession. Memory for pAslArcSession have been allocated
	by the caller and the size of the memory must be at least that
	returned by aslArcSessionGetSize().

    An already initialized pAslArcSession must be destroyed before it can
	be initialized again.

	Iff UseRefSegments is 0 the reference segments created never created. Otherwise
	they will be created as vertical non-pen lift segments starting	on the help line
	REFERENCE_SEGMENT_SEPARATION * nDistBaseToHelpLine to the left or right of the
	XMin or XMax of the first arc and all arcs, depending on writingDirection, and
	ending on the base line. Pen lift segments will	also be created to connect them
	to the first and last arc. Thus making all backward and forward connection
	distance matching of first and last segment possible. This allows for position 
	(e.g. p vs. P) and scale distinction (e.g. o vs. O), but makes the recognition
	more sensitive to the vertical position and scale of the input characters.

    Returns decumaNoError if input parameters are valid.
*/
DECUMA_HWR_PRIVATE DECUMA_STATUS aslArcSessionInit(ASL_ARC_SESSION* pAslArcSession,
					   const DECUMA_MEM_FUNCTIONS* pMemFunctions,
					   int nBaseLine,
                       int nDistBaseToHelpLine,
					   WRITING_DIRECTION writingDirection);

/*
	Returns 1 if pAslArcSession is corrupt, 0 otherwise.
*/
DECUMA_HWR_PRIVATE int aslArcSessionIsCorrupt(const ASL_ARC_SESSION* pAslArcSession);

/*
    Releases memory allocated for added arcs and destroys pAslArcSession. pAslArcSession
	must be re-initialized before it can be used again.
*/
DECUMA_HWR_PRIVATE void aslArcSessionDestroy(ASL_ARC_SESSION* pAslArcSession);

#ifdef _DEBUG
/*
    Returns the baseline of pAslArcSession.
*/
DECUMA_HWR_PRIVATE int aslArcSessionGetBaseline(const ASL_ARC_SESSION* pAslArcSession);

/*
    Returns the distance between base and helpline of pAslArcSession. Always >0
*/
DECUMA_HWR_PRIVATE int aslArcSessionGetDistBaseToHelpline(const ASL_ARC_SESSION* pAslArcSession);

#endif

DECUMA_HWR_PRIVATE void aslArcSessionSetBaseline(ASL_ARC_SESSION* pAslArcSession, int nBaseline);
DECUMA_HWR_PRIVATE void aslArcSessionSetDistBaseToHelpline(ASL_ARC_SESSION* pAslArcSession, int nDistBaseToHelpline);


/*
    Translates pDecumaArc to a ASL_ARC which is then added to pAslArcSession.
	A ASL_ARC is created by this function for this. Memory will be allocated.
	Ending reference (if existing) will be removed. No ending reference will be
	added. Call aslArcSessionAddEndingReference() separately to add ending
	reference. NOTE: Adding and evaluating ending reference cost extra time
	and should therefore be avoided for consecutive arc additions without any
	need for recognition.

	If bNextArcIsConcurrent is set to 1, the arc session will remember that the next
	added arc was entered simultaneously as this one. (multi-touch)
*/

DECUMA_HWR_PRIVATE DECUMA_STATUS aslArcSessionStartNewArc(ASL_ARC_SESSION * pAslArcSession, DECUMA_UINT32 arcID);

DECUMA_HWR_PRIVATE DECUMA_STATUS aslArcSessionStartNewSymbol(ASL_ARC_SESSION * pAslArcSession);

/*Adds point(s) to a started arc  */
DECUMA_HWR_PRIVATE DECUMA_STATUS aslArcSessionAddPoint(ASL_ARC_SESSION * pAslArcSession,
									DECUMA_COORD x, DECUMA_COORD y, 
									DECUMA_UINT32 arcID);

DECUMA_HWR_PRIVATE DECUMA_STATUS aslArcSessionAddPoints(ASL_ARC_SESSION * pAslArcSession,
							  DECUMA_POINT * pPts, int nPts, 
							  DECUMA_UINT32 arcID,
							  int * pnPtsAdded);


/*Ends the arc and removes it from the session */
DECUMA_HWR_PRIVATE DECUMA_STATUS aslArcSessionCancelArc(ASL_ARC_SESSION * pAslArcSession,DECUMA_UINT32 arcID);

/*Ends the sampling and adds the arc lasst in the "permanent storage" of the arc session */
DECUMA_HWR_PRIVATE DECUMA_STATUS aslArcSessionCommitArc(ASL_ARC_SESSION * pAslArcSession,DECUMA_UINT32 arcID);


DECUMA_HWR_PRIVATE int aslArcSessionGetUncommittedArcCount(ASL_ARC_SESSION * pAslArcSession);
DECUMA_HWR_PRIVATE DECUMA_STATUS aslArcSessionGetUncommittedArcID(ASL_ARC_SESSION * pAslArcSession, int idx,
										DECUMA_UINT32 * pSrcID);

DECUMA_HWR_PRIVATE DECUMA_STATUS aslArcSessionAddArc(ASL_ARC_SESSION* pAslArcSession, const DECUMA_ARC* pDecumaArc,
								  int bConcurrentWithNext);
/*
	Returns 1 if the arcs with index1 and index2 are concurrent (part of the same "multi-touch-sequence")
*/
DECUMA_HWR_PRIVATE int aslArcSessionArcsAreConcurrent(const ASL_ARC_SESSION* pAslArcSession,
								   int nIndex1,int nIndex2);

/*
	Returns 1 if there is a started but uncommitted arc that starts a new symbol.
*/
DECUMA_HWR_PRIVATE int aslArcSessionIsStartingNewSymbol(const ASL_ARC_SESSION * pAslArcSession);

#ifdef _DEBUG

/*
    Returns number of arcs of pAslArcSession, i.e. the number of added arcs.
*/
DECUMA_HWR_PRIVATE int aslArcSessionGetArcCount(const ASL_ARC_SESSION* pAslArcSession);

/*
    Returns a const pointer to arc nIndex of pAslArcSession.
*/
DECUMA_HWR_PRIVATE const ASL_ARC* aslArcSessionGetArc(const ASL_ARC_SESSION* pAslArcSession, int nIndex);

/*
    Fills a vector with arc indexes to the arcs that were added "simultaneously" at arcTimeStamp.
	Returns the number of indexes written
*/
DECUMA_HWR_PRIVATE int aslArcSessionGetConcurrentArcs(const ASL_ARC_SESSION* pAslArcSession, DECUMA_UINT32 arcTimeStamp,
								   int * pArcIdxs, int nMaxArcs);
#endif

DECUMA_HWR_PRIVATE const DECUMA_CURVE* aslArcSessionGetCurve(const ASL_ARC_SESSION* pAslArcSession);

#endif /*ASL_ARC_SESSION_H */
