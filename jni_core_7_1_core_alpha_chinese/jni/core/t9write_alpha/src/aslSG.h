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

  This file provides the interface to aslSG (asl Segmentation Graph)
  objects.


  A segmentation graph consist of a number of nodes (N1, N2 and N3
  in the example figure below). Two nodes can be connected by an edge
  (E1_1, E1_2 etc. below). An edge has a start node and an end node.

  For the segmentation graph the rule for edges are that an edge
  must end in a node with higher index than its start node. In
  the example below this means that E1_1 starts in node 0 and
  ends in node 1.

  Nodes are simply identified by an index. Edges are identified by
  their end node and an index. In the example below this means that
  E2_1 is the first edge ending in node 2. The index of an edge is
  irrelevant, if E2_2 and E2_1 switch index that would result in
  an equivalent segmentation graph.

  A node contains references to the edges ending in the node.

  An edge contains a reference to its starting node.


     ____________
    /____   E2_2 \
   //E1_2\        \
  |/______\  ______\
  |/ E1_1 \|/ E2_1 \|
  *        *        *
  N0       N1       N2


\******************************************************************/

#ifndef ASL_SG_H
#define ASL_SG_H

#include "aslConfig.h"
#include "decumaBasicTypesMinMax.h"
#include "decumaRuntimeMalloc.h"
#include "decumaCategories.h"

#ifdef EXTRA_VERIFICATION_INFO
#include "scrAPIHidden.h"
#endif

typedef struct _ASL_SG ASL_SG;
typedef struct _ASL_SG_NODE ASL_SG_NODE;
typedef struct _ASL_SG_EDGE ASL_SG_EDGE;

/*Defines that could be useful also outside the SG module */
#define MAX_NODES_IN_SEGMENTATION_GRAPH  MAX_DECUMA_INT16

/* Use artifical limit if defined, intended for unit tests of how a filled input buffer is handled. */
#ifdef ARTIFICIAL_MAX_NODES_IN_SG
#undef MAX_NODES_IN_SEGMENTATION_GRAPH
#define MAX_NODES_IN_SEGMENTATION_GRAPH		ARTIFICIAL_MAX_NODES_IN_SG
#endif

/* NOTE: The increased number of edges at or after pen lift are needed */
/* to support sufficient amount of "long pen lifts" */
#ifndef MAX_EDGES_PER_NODE
#define MAX_EDGES_PER_NODE                                  50
#endif

#if !defined(_DEBUG)
#include "aslSGMacros.h"
#endif

#include "decumaStatus.h"
#include "decumaBasicTypes.h"
#include "decumaCategoryTableType.h"
#include "decumaCategoryType.h"
#include "aslArcSession.h"

#include "scrlibExtra.h"



/*
    Returns the size of a ASL_SG object.
*/
DECUMA_HWR_PRIVATE int aslSGGetSize( void );

/*
    Initializes pSG. Memory for pSG have been allocated
	by the caller and the size of the memory must be at least that
	returned by aslSGGetSize().

    An already initialized pSG must be destroyed before it can
	be initialized again.

    Returns decumaNoError if input parameters are valid.
*/
DECUMA_HWR_PRIVATE void aslSGInit(ASL_SG* pSG,
			   const DECUMA_MEM_FUNCTIONS* pMemFunctions,
			   DECUMA_STATIC_DB_PTR pStaticDB,
			   DECUMA_DYNAMIC_DB_PTR pDynamicDB,
			   const DECUMA_CHARACTER_SET* pCharacterSet,
			   CATEGORY_TABLE_PTR pCatTable,
			   CATEGORY_TABLE_PTR pDynCatTable,
               const ASL_ARC_SESSION* pArcSession,
               RECOGNITION_MODE recognitionMode,
			   DECUMA_INT16 * forcedSegmentation,
			   DECUMA_INT16 nForcedSegmentationLen);

/*
	Returns 1 if pSG is corrupt, 0 otherwise.
*/
DECUMA_HWR_PRIVATE int aslSGIsCorrupt(const ASL_SG* pSG);

/*
    Releases all memory allocated for pSG and and destroys pSG.
	pSG must be re-initialized before it can be used again.
*/
DECUMA_HWR_PRIVATE void aslSGDestroy(ASL_SG* pSG);

/*
	Releases all memory allocated for pSG and reverts to its
	state after initialization, except for the fact that any
	filters set after initialization are kept (aslSGDestroy
	must be used to release these).
*/
DECUMA_HWR_PRIVATE void aslSGReset(ASL_SG* pSG);


/*
    Evaluates any arc of the arc session of pSG not yet evaluated.
	When an arc is evaluated the segmentation graph	is expanded with
	all new possible segmentation paths that the arc provides and the
	nodes and edges of the graph are scored using the prototype db and
	the variation db respectively.

*/
DECUMA_HWR_PRIVATE DECUMA_STATUS aslSGEvaluateArcs(ASL_SG* pSG);

/*	Tries to recognize the last arc (or multitouch arcs) as a gesture
	In that case the gesture unicode is stored in pGesture. Otherwise
	*pGesture will be set to zero.
	pQuickGestureSettings contains settings for what is needed for a quick
	gesture to be recognized, in addition to being the best recognition of
	the arc.
*/
DECUMA_HWR_PRIVATE DECUMA_STATUS aslSGIndicateInstantGesture( ASL_SG* pSG,
	int * pbInstantGesture,
	const DECUMA_INSTANT_GESTURE_SETTINGS * pInstantGestureSettings);

DECUMA_HWR_PRIVATE void aslSGAdjustForNewEstimates(ASL_SG* pSG, DECUMA_COORD nNewBaseLineEstimate, DECUMA_COORD nNewHelpLineEstimate);

/*
    Creates/replaces an edge in node 0, with the symbol pSymbol. Its distance will
	be set to 0.
*/
DECUMA_HWR_PRIVATE DECUMA_STATUS aslSGSetStringStart(ASL_SG* pSG,
								  const DECUMA_UNICODE* pSymbol,
								  int nSymbolLen);

#ifdef _DEBUG
/*
    Returns number of nodes of pSG. Should equal the number of
	segments of the arc session of pSG, after a succesful call
	to aslSGEvaluateArcs.
*/
DECUMA_HWR_PRIVATE int aslSGGetNodeCount(const ASL_SG* pSG);

/*
    Returns a const pointer to node nIndex of pSG.
*/
DECUMA_HWR_PRIVATE const ASL_SG_NODE* aslSGGetNode(const ASL_SG* pSG, int nIndex);

/************************************************** asl SG node ***

  A node represent the point of connection for two consecutive
  arcs of the SG's arc session. A node contain references to the
  edges (node connectors) ending in the node.

*******************************************************************/

/*
    Returns number of edges ending in pNode.
*/
DECUMA_HWR_PRIVATE int aslSGGetEdgeCount(const ASL_SG_NODE* pNode);

/*
    Returns a const pointer to edge nIndex of the edges ending in pNode.
*/
DECUMA_HWR_PRIVATE const ASL_SG_EDGE* aslSGGetEdge(const ASL_SG_NODE* pNode, int nIndex);

/*
    Returns index of the best edge ending in pNode. This function can
    be used to initiate a backtrack (start at the last node and track
    back to the first) of the best path of the SG. After getting the
    last edge of the best path with this function the remaining edges
    in the best path can be obtained by using the function
    aslSGGetBestConnectorEdgeIndex().
    NOTE: Best edge means the edge with the smallest best path
    distance, not the smallest (local edge) distance.
*/
DECUMA_HWR_PRIVATE int aslSGGetBestEdgeIndex(const ASL_SG_NODE* pNode);

/************************************************** asl SG edge ***

  An edge connects two nodes. One of the connected nodes is
  considered start node and the other end node of the edge. An edge
  can thus represent a sequence of consecutive segement of the arc
  session of the SG (starting in the start node and ending in the
  end node of the edge).

  An egde is owned by the node in which it ends and contains a
  reference to the node in which it starts. The main content of an
  edge is the result of a match between the segment sequence in the
  arc session that it represents and a key in the prototype
  database. This result includes the matched prototype, the stroke
  order and segment variation of the prototype that matched best
  and the matching distance.

*******************************************************************/

/*
    Returns a pointer to a zero-terminate string containing the symbol of pEdge.
*/
DECUMA_HWR_PRIVATE DECUMA_UNICODE* aslSGGetSymbol(const ASL_SG_EDGE* pEdge);

/*
    Returns the index of the node pEdge begins in.
*/
DECUMA_HWR_PRIVATE int aslSGGetStartNode(const ASL_SG_EDGE* pEdge);

/*
    Returns index of the edge ending in the best connector node of pEdge
    that is in the best path of pEdge, i.e. the best backward connecting edge
    of pEdge. This function can be used to backtrack the best path of
    the SG, by starting at the best edge of the last node and track back
    the best path back to the first node.
*/
DECUMA_HWR_PRIVATE int aslSGGetBestConnectorEdgeIndex(const ASL_SG_EDGE* pEdge);

/*
    Returns the sum of the prototype matching distances and the edge
    connection distances along the best path from pEdge back to the
    start node.
*/
DECUMA_HWR_PRIVATE int aslSGGetBestPathDistance(const ASL_SG_EDGE* pEdge);
#endif

/*
    Returns the matching distance of the matched prototype of pEdge
	iff pDbPenLift is 0. The connection to previous edge is excluded.
*/
DECUMA_HWR_PRIVATE long aslSGGetDistance(const ASL_SG_EDGE* pEdge);

/*
    Returns the connection distance between pEdge and its best connector.
*/
DECUMA_HWR_PRIVATE long aslSGGetBestConnectorConnectionDist(const ASL_SG_EDGE* pEdge);

/*
    Returns the connection distance between an edge and the
    edge with index prevEdgeIdx coming to the start node of the edge.

	There needs to exist a previous edge with the index prevEdgeIdx.

    nNode - is the end node of the edge
    nEdgeIdx - is the index of the edge in that node

*/
DECUMA_HWR_PRIVATE int aslSGGetConnectionDistance(const ASL_SG * pSG,
	int nNode, int nEdgeIdx, int prevEdgeIdx);

DECUMA_HWR_PRIVATE int aslSGGetBestStringDistance(const ASL_SG * pSG);

/* segmentation can be zero. If it is non-zero pnSegmentationLen must also be non-zero. */
/* If the returned *pnSegmentationLen is larger than nMaxSegmentationLen the returned */
/* segmentation is not complete (first part not included) and cannot be used. */
/* If segmentation i non-zero it will be filled with pSG node indeces uniquely specifying */
/* the segmentation of the best SG string returned in pChars. */
/* NOTE: SG can only provide the string of best base segmentation (i.e. pChars will contain */
/* base symbols only). */
DECUMA_HWR_PRIVATE void aslSGGetBestString(const ASL_SG * pSG,
						DECUMA_UNICODE * pChars,
						DECUMA_UINT16 nMaxChars,
						DECUMA_UINT16 * pnChars,
						DECUMA_UINT16 * pnResultingChars,
						int nStrIdx,
						DECUMA_INT16 * pSymbolChars,
						DECUMA_INT16 nMaxSymbolCharsLen,
						DECUMA_INT16 * pnSymbolCharsLen,
						DECUMA_INT16 * pnResultingSymbolCharsLen,
						DECUMA_INT16 * pSymbolStrokes,
						DECUMA_INT16 nMaxSymbolStrokesLen,
						DECUMA_INT16 * pnSymbolStrokesLen,
						DECUMA_INT16 * pnResultingSymbolStrokesLen,
						DECUMA_UINT8 * pSymbolArcTimelineDiffMask,
						DECUMA_INT16 nMaxSymbolArcTimelineDiffMaskLen,
						DECUMA_INT16 * pnSymbolArcTimelineDiffMaskLen,
						DECUMA_INT16 * pnResultingSymbolArcTimelineDiffMaskLen,
						DECUMA_COORD * pnAverageBaseLineEstimate,
						DECUMA_COORD * pnAverageHelpLineEstimate,
						DECUMA_UINT8 * pbEndsWithGesture,
						DECUMA_UINT8 * pbEndsWithInstantGesture);
/*
	Returns the byte size of an edge
*/
DECUMA_HWR_PRIVATE int aslSGGetEdgeSize(void);

/*
	Returns the best estimate of actual baseline used during input given that the user
	intended to write the symbol(s) represented by pEdge.
*/
DECUMA_HWR_PRIVATE DECUMA_COORD aslSGGetBaseLineYEstimate(const ASL_SG_EDGE * pEdge);

/*
	Returns the best estimate of actual helpline used during input given that the user
	intended to write the symbol(s) represented by pEdge.
*/
DECUMA_HWR_PRIVATE DECUMA_COORD aslSGGetHelpLineYEstimate(const ASL_SG_EDGE * pEdge);

DECUMA_HWR_PRIVATE int aslSGIsEdgeEstimateReliable(const ASL_SG* pSG, const ASL_SG_EDGE * pEdge);

/*
	Returns the SCR_OUTPUT of pEdge.
*/
DECUMA_HWR_PRIVATE const SCR_OUTPUT* aslSGGetOutput(const ASL_SG_EDGE * pEdge);

/*
	Returns the SCR_CURVE_PROP of pEdge.
*/
DECUMA_HWR_PRIVATE const SCR_CURVE_PROP* aslSGGetScrCurveProp(const ASL_SG_EDGE * pEdge);


DECUMA_HWR_PRIVATE void aslSGEdgeIsGesture(const ASL_SG_EDGE * pEdge, int * pbGesture, int * pbInstantGesture);

/*
	Returns the database index of the template.
*/
DECUMA_HWR_PRIVATE const KID* aslSGGetKID(const ASL_SG_EDGE * pEdge);

/*
	Returns the non-shape (i.e. transformation) related component of the edge distance.
*/
DECUMA_HWR_PRIVATE int aslSGGetPunish(const ASL_SG_EDGE * pEdge);

#endif /*ASL_SG_H */
