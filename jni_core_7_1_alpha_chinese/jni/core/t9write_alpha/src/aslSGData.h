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
  aslSG objects.

\******************************************************************/

#ifndef ASL_SG_DATA_H
#define ASL_SG_DATA_H

#if defined(_DEBUG) && !defined(ASL_SG_C)
#error Direct data access in debug mode only allowed from aslSG.c
#endif

#include "aslConfig.h"

#include "decumaBasicTypes.h"
#include "decuma_point.h"
#include "decumaCategoryType.h"
#include "decumaQCandHandler.h"
#include "decumaHashHandler.h"
#include "aslArcSession.h"
#include "aslSG.h"
#include "scrlibExtra.h"

struct _ASL_SG_EDGE
{
	/* An edge contains a match of the segments between a start node and */
	/* the edge's node against the segments (including pen up segments) */
	/* of the M first arcs of a database prototype with N arcs, 1 <= M <= N. */
	/* If M == N, the edge is considered complete, i.e. it matches all */
	/* arcs of the prototype. */

	/* The best path of a SG is the path from its start node to its end node */
	/* with the smallest edge distance and edge connection distance sum. The */
	/* best path of a node of a SG is the path back to its start node with the */
	/* smallest edge distance and edge connection distance sum. The edge distance */
	/* and edge connection distance sum along a path defines the path's distance. */

	/* When comparing edges against each other it is not only the distance */
	/* of the edge that is compared. The length of an edge impacts its */
	/* distance, since the edge distance is the sum of the segment distances */
	/* (including segment connection distances) from its start node to its end */
	/* node. Instead edges are compared by their best path distance. I.e. the */
	/* resulting best edge of a node is the edge that is in the best path of */
	/* the node, i.e. the edge of the node with the smallest best path distance, */
	/* the smallest (local edge) distance. */

	/* TODO Are all properties below needed always in all edges (e.g. complete edges)? Otherwise optimize */

	SCR_OUTPUT scr_output; /*More information about the recognition result */
	SCR_CURVE_PROP scr_curve_prop; /*Pre-calculated curve properties used during recognition. Needed for fast punish adjustment. */

	DECUMA_UINT32 nCheckSumComplete;

	long     nDistance;        /* Sum of distances between each segment from nStartNode to this edge's node and */
	                           /* the corresponding (first) segments of prototype nPrototype (with stroke order */
	                           /* nStrokeOrder and segment variation nVariation). This includes segment connection */
	                           /* distances for prototypes with more than one segment. */
	long     nBestPathDist;    /* Sum of the edge distances and the edge connection distances along the best */
	                           /* path of this edge */

	DECUMA_INT16      nStartNode;        /* Index of the start node of this match (defines the arc session segments matched) */

	DECUMA_INT8       nArcs;

	DECUMA_INT8       nBestConnector;   /* Index of the edge ending in nBestConnectorNode that is in the best path of this edge */

	long              nBestConnectorConnDist;

	DECUMA_UNICODE    alternativeSymbol[2]; /* Allocate for alternative symbol storage (assume only one unicode for alternative symbols) */

	DECUMA_COORD nBaseLineYEstimate;
	DECUMA_COORD nHelpLineYEstimate;

};

struct _ASL_SG_NODE
{
	ASL_SG_EDGE* pCompleteEdges;
	int nCompleteEdges;
	int nCompleteEdgesLists;
};

typedef struct _SET_EDGES_DATA
{
	ASL_SG_EDGE* pCompleteEdges; /* Pointer the array of complete base or diac edges that ends in nNode */
	DECUMA_UINT16* pCompleteEdgesRanking; /* Rank sorted indeces of array pointed to by pCompleteEdges */
	int* pnCompleteEdges; /* Pointer to integer containing length of array pointed to by pCompleteEdges */

	DECUMA_QCH * pCompleteCandHandler; /* Handles complete edge qualification */

	DECUMA_QCH_CANDREF* pCompleteCandRefs; /* Pointer the array of DECUMA_QCH_CANDREF:s allocated for pCompleteCandHandler */

	DECUMA_HH * pCompleteCandHandlerCheckSum; /* Handles fast complete edge search */

	ASL_SG_NODE* pNode; /* The node to set base or diac edges for */
	int nNode; /* Index of the node to set base or diac edges for */

	const ASL_ARC* pArc; /* The nNode-1 arc of the arc session, i.e. the last arc of the edges to set */

	int nArcs; /* Number of arc in the arc session */

	int nForcedStartNode; /* The only valid start node for edges to create or -1 if no restriction */
	int nForcedEndNode; /* The only valid end node for complete edges to add */

	int nForbiddenNodes; /*The number of forbidden nodes in the array pForbiddenNodes */
	int * pForbiddenNodes; /*An array of indices to forbidden nodes */

} SET_EDGES_DATA;

struct _ASL_SG
{
	/*  All values are in reference to the coordinate system of the */
	/*  original input arc segment from which the ASL_SEGMENT is created. */

#ifdef ASL_HIDDEN_API

	PROTOTYPE_FILTER* pBackwardFilter;
	PROTOTYPE_FILTER* pForwardFilter;

	int nFilters;

#endif

	const DECUMA_MEM_FUNCTIONS* pMemFunctions;

	DECUMA_STATIC_DB_PTR      pStaticDB;
	DECUMA_DYNAMIC_DB_PTR     pDynamicDB;
	const DECUMA_CHARACTER_SET*  pCharacterSet;
	CATEGORY_TABLE_PTR pCatTable;
	CATEGORY_TABLE_PTR pDynCatTable;
	const ASL_ARC_SESSION*       pArcSession;     /* The arc session containing the segments to construct the SG from */

	ASL_SG_NODE**            pNodes;          /* Nodes of the SG */

	int                      nNodes;          /* Equals number of arcs of pArcSession + 1 */

	DECUMA_QCH * pCompleteCandHandler;

	DECUMA_QCH_CANDREF* pCompleteCandRefs;

	DECUMA_HH * pCompleteCandHandlerCheckSum;

	DECUMA_UINT16 completeEdgesRanking[MAX_EDGES_PER_NODE];

	RECOGNITION_MODE recognitionMode;

	/*Forced segmentation */
	int bUseForcedSegmentation;
	DECUMA_INT16* forcedSegmentation; /* Nodes indeces specifying segmentation, e.g. [0, 3, 4, 9] for a 10 node graph */
	int nForcedSegmentationLen;

	CATEGORY thaiLetterCat;
	CATEGORY cyrillicLetterCat;

	CATEGORY cat;

	SET_EDGES_DATA setEdgesData;

	int bIsCorrupt;

};

#endif /*ASL_SG_DATA_H */
