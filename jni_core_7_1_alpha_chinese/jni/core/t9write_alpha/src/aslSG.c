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

  This file implements aslSG.h. It uses dynamic memory
  allocation.

\******************************************************************/

#define ASL_SG_C

#include "aslConfig.h"

#include "aslSG.h"
#include "aslSGMacros.h"

#include "decumaAssert.h"
#include "decumaMemory.h"
#include "aslSGData.h"
#include "aslTools.h"
#include "decumaQCandHandler.h"
#include "decumaHashHandler.h"
#include "decumaBasicTypes.h"
#include "decumaBasicTypesMinMax.h"
#include "decumaSymbolCategories.h"
#include "decumaLanguages.h"
#include "decumaCategoryTranslation.h"

#include "scrlib.h"
#include "scrlibExtra.h"


/*#define DEBUG_OUTPUT */
#ifdef DEBUG_OUTPUT
#include <stdio.h>
FILE * g_debugFile;
#endif

#define NUMBER_OF_SCR_RESULTS 23

#define FIX_CONN_DIST 250
#define THAI_CONN_DIST_OFFSET (-250)
#define CYRILLIC_CONN_DIST_OFFSET (-200)
#define NOISE_TO_NOISE_CONN_DIST 1000

/* */
/* Private macros (inlining for speed improvement) */
/* */



#define NO_REPLACEE_SEARCH

#ifdef ARM_PROFILING_EXTRA
#include "ARM_profiling_extra.h"
#define PROF(func) PRF_##func()
#define PROF_IF( cond, func) if (cond) {PRF_##func();}
#define PROF_IF_ELSE( cond, func_if, func_else) if (cond) {PRF_##func_if();} else {PRF_##func_else();}
#else
/*Define PROF-macros as empty when not profiling */
#define PROF(a)
#define PROF_IF(a,b)
#define PROF_IF_ELSE(a,b,c)
#endif


#define GUARANTEE_BEST_PATH_IN_SG            0

#if GUARANTEE_BEST_PATH_IN_SG
/* This options needs increased maximum number of complete edges to */
/* avoid hitrate reduction, which implies slower SG and RG. */
#define CLASS_BY_TYPE_FOR_COMPLETE_EDGES       1
#define MAX_COMPLETE_EDGES_OF_SAME_CLASS       1 /* More than one here is pointless if class is by variations */
#else
/* This seems to be the optimal solution with the trained database */
#define CLASS_BY_TYPE_FOR_COMPLETE_EDGES       0 /* Class by prototype */
#define MAX_COMPLETE_EDGES_OF_SAME_CLASS       1 /* Verified to work best with trained DB */
#endif

#if CLASS_BY_TYPE_FOR_COMPLETE_EDGES
#define edgesBelongToSameCompleteEdgesClassMacro edgesBelongToSameTypeClassMacro
#else
#define edgesBelongToSameCompleteEdgesClassMacro edgesBelongToSameSymbolClassMacro
#endif

#define edgesBelongToSameSymbolClassMacro(nArcs, nSymbol, nType, pEdge) \
	(0)

#define edgesBelongToSameTypeClassMacro(nArcs, nSymbol, nType, pEdge) \
	(0)

/* TODO Handle changes in class rule as above */
#define calcEdgeCheckSum(nArcs, nSymbol, nType) \
	((nArcs) * (nSymbol) * (nType))

#define initEdgeMacro(pEdge) \
{\
	/* decumaMemset removed (unnecessary and costed to much) */\
\
	(pEdge)->nEdgeArcs = 0;\
\
	(pEdge)->nDistance = 0;\
	(pEdge)->nBestPathDist = 0;\
	(pEdge)->nBestConnector = -1;\
	(pEdge)->nBestConnectorNode = -1;\
	(pEdge)->nBestConnectorConnDist = DECUMA_MAX_DISTANCE;\
}



/* */
/* Private data structure definitions */
/* */



typedef struct _CANDIDATE_REPLACEE_PARAM
{
	const ASL_SG_EDGE*        pCandidate;    /* The replacee */
	DECUMA_HH_ELEMREF         pCandidateRef; /* Candidate handler reference of pCandidate */
	int nBestPathDist;           /* Best path distance of pCandidate (TODO change CH to use long) */
	int bFound;                  /* Already found the replacee */
	int bPossibleNonWorst;       /* The replacee rules might result in another candidate than the one with largest best path distance */
	int bInitialized;
} CANDIDATE_REPLACEE_PARAM;


/* */
/* Private function declarations */
/* */



static void sgRelease(ASL_SG* pSG);

/* Returns 0 in case of allocation failure */
static DECUMA_STATUS setEdges(ASL_SG* pSG);

static int initSetEdgesData(ASL_SG* pSG, int bDiac, int* pbNoEdgesToSet);

static void destroySetEdgesData(ASL_SG* pSG);

static DECUMA_STATUS createNewEdges(ASL_SG* pSG);

static void initCandidateReplaceeParam(CANDIDATE_REPLACEE_PARAM* pReplaceeParam,
									   int bDiac,
									   int bCanCombineWithOtherPrevSegVars,
									   int bCompleteCandidate,
									   int bCompleteCandidateList);

static void updateCandidateReplacee(CANDIDATE_REPLACEE_PARAM* pReplaceeParam,
									const DECUMA_QCH* pCH,
									const DECUMA_HH* pHH,
									int bCompleteCandHandler);

static DECUMA_HH_ELEMREF findWorstEdgeOfSameClass(const DECUMA_HH* pHHCheckSum,
												  DECUMA_HH_ELEMREF elemRef,
												  DECUMA_UINT32 nCheckSum,
												  int nArcs,
												  int nSymbol,
												  int nType,
												  int nMaxEdgesOfSameClass);

static int addEdgeIfQualified(ASL_SG* pSG, const ASL_SG_EDGE* pEdge, DECUMA_QCH * pCH, long nMaxDist, const DECUMA_QCH_CANDREF** pCandidateToReplaceRef);

static int verifyCandHandler(const DECUMA_QCH* pCH);

static int findBestEdge(const ASL_SG_EDGE* pEdges, int nEdges);

/* Returns 0 in case of allocation failure */
static int matchEdge(ASL_SG* pSG, int nNode, ASL_SG_EDGE* pEdge, long nMaxDist);

static void findBestConnector(ASL_SG* pSG,
							  int nNode,
							  ASL_SG_EDGE* pEdge,
							  long nMaxPrevPathPlusConnDist,
							  int nMaxConnectionsToTest,
							  long* pnBestPrevPathPlusConnDist,
							  DECUMA_INT8* pnBestConnector,
							  long * pnBestConnectorConnDist);

static int matchEdgeConnection(const ASL_SG* pSG,
							   int nNode,
							   const ASL_SG_EDGE* pEdge,
							   const ASL_SG_EDGE* pPrevEdge);

static ASL_SG_NODE* sgAddNode(ASL_SG* pSG);
static void sgRemoveNode(ASL_SG* pSG);

/*If pEdgeList is != NULL it will be copied. nListLen is then the number of edges already in pEdgeList */
/*If pEdgeList is NULL a new list will be allocated. nListLen is then the maximum length of the new list */
static ASL_SG_EDGE* sgNodeAddCompleteEdgesList(const ASL_SG* pSG, ASL_SG_NODE* pNode, ASL_SG_EDGE* pEdgeList, int nListLen);
static void sgNodeRemoveCompleteEdgesList(const ASL_SG* pSG, ASL_SG_NODE* pNode);

static int getForcedStartNode(const ASL_SG * pSG, int nNode);
static int getForcedEndNode(const ASL_SG * pSG, int nNode);


static int curveIsBigOrOffEnoughToBeQuickGesture(DECUMA_CURVE * pCurve,
	int baseline, int helpline,
	const DECUMA_INSTANT_GESTURE_SETTINGS * pInstantGestureSettings);

/* */
/* Public function definitions */
/* */



int aslSGGetSize( void )
{
	return sizeof(ASL_SG);
}

void aslSGInit(ASL_SG* pSG,
			   const DECUMA_MEM_FUNCTIONS* pMemFunctions,
			   DECUMA_STATIC_DB_PTR pStaticDB,
			   DECUMA_DYNAMIC_DB_PTR pDynamicDB,
			   const DECUMA_CHARACTER_SET* pCharacterSet,
			   CATEGORY_TABLE_PTR pCatTable,
			   CATEGORY_TABLE_PTR pDynCatTable,
               const ASL_ARC_SESSION* pArcSession,
               RECOGNITION_MODE recognitionMode,
			   DECUMA_INT16 * forcedSegmentation,
			   DECUMA_INT16 nForcedSegmentationLen)
{
	DECUMA_UINT32 symbCatThaiLetter = DECUMA_CATEGORY_THAI;
	DECUMA_UINT32 symbCatCyrillicLetter = DECUMA_CATEGORY_CYRILLIC;
	DECUMA_UINT32 langCatThai = DECUMA_LANG_TH;
	DECUMA_UINT32 langCatCyrillic[4] = {DECUMA_LANG_RU, DECUMA_LANG_UK, DECUMA_LANG_BG, DECUMA_LANG_SRCY};
	DECUMA_CHARACTER_SET charSet;
	DECUMA_STATUS status;

	decumaAssert(pSG);
	decumaAssert(pMemFunctions);
	decumaAssert(pCatTable);
	decumaAssert(pArcSession);

	decumaMemset(pSG, 0, sizeof(pSG[0]));

	pSG->pMemFunctions = pMemFunctions;
	pSG->pStaticDB = pStaticDB;
	pSG->pDynamicDB = pDynamicDB;
	pSG->pCharacterSet = pCharacterSet;
	pSG->pCatTable = pCatTable;
	pSG->pDynCatTable = pDynCatTable;
	pSG->pArcSession = pArcSession;
	pSG->recognitionMode = recognitionMode;

	if (forcedSegmentation)
	{
		pSG->forcedSegmentation = forcedSegmentation;
		pSG->nForcedSegmentationLen = nForcedSegmentationLen;
		pSG->bUseForcedSegmentation = 1;
	}

	charSet.pSymbolCategories = &symbCatThaiLetter;
	charSet.nSymbolCategories = 1;
	charSet.pLanguages = &langCatThai;
	charSet.nLanguages = 1;

	status = translateToCategoryStructs(&charSet,pCatTable,NULL,&pSG->thaiLetterCat,NULL);
	/* Note status might not be ok if Thai is not supported by the DB. That is fine, */
	/* just make sure everything is still as expected. */
	decumaAssert(status == decumaNoError || decumaInvalidCategory &&
		CATEGORY_MASK_IS_EMPTY(pSG->thaiLetterCat.symbolCat) &&
		CATEGORY_MASK_IS_EMPTY(pSG->thaiLetterCat.languageCat));

	charSet.pSymbolCategories = &symbCatCyrillicLetter;
	charSet.nSymbolCategories = 1;
	charSet.pLanguages = langCatCyrillic;
	charSet.nLanguages = 4;

	status = translateToCategoryStructs(&charSet,pCatTable,NULL,&pSG->cyrillicLetterCat,NULL);
	/* Note status might not be ok if Thai is not supported by the DB. That is fine, */
	/* just make sure everything is still as expected. */
	decumaAssert(status == decumaNoError || decumaInvalidCategory &&
		CATEGORY_MASK_IS_EMPTY(pSG->cyrillicLetterCat.symbolCat) &&
		CATEGORY_MASK_IS_EMPTY(pSG->cyrillicLetterCat.languageCat));

	status = translateToCategoryStructs(pCharacterSet,pCatTable,NULL,&pSG->cat,NULL);
	decumaAssert(status == decumaNoError);
}

int aslSGIsCorrupt(const ASL_SG* pSG)
{
	return pSG->bIsCorrupt || aslArcSessionIsCorrupt(pSG->pArcSession);
}

void aslSGReset(ASL_SG * pSG)
{
	/* Reinitialize the segmentation graph */
	sgRelease(pSG);
	aslSGInit(pSG,
		pSG->pMemFunctions,
		pSG->pStaticDB,
		pSG->pDynamicDB,
		pSG->pCharacterSet,
		pSG->pCatTable,
		pSG->pDynCatTable,
        pSG->pArcSession,
        pSG->recognitionMode,
		pSG->forcedSegmentation,
		pSG->nForcedSegmentationLen);
}

void aslSGDestroy(ASL_SG* pSG)
{
	decumaAssert(pSG);

	sgRelease(pSG);

	decumaMemset(pSG, 0, sizeof(pSG[0]));
}

DECUMA_STATUS aslSGEvaluateArcs(ASL_SG* pSG)
{
	ASL_SG_NODE* pNode = NULL;
	int nArcSessionArcs = aslArcSessionGetArcCount(pSG->pArcSession);
	int i;
	DECUMA_STATUS status = decumaNoError;

	decumaAssert(pSG);

	for (i = pSG->nNodes; i < nArcSessionArcs + 1; i++)
	{
		/* If the arc of this node is concurrent with the next arc to be added */
		/* then we cannot start evaluating yet */
		if (i > 0 && aslArcSessionArcsAreConcurrent(pSG->pArcSession, i-1, nArcSessionArcs)) return decumaNoError;

		pNode = sgAddNode(pSG);

		if (!pNode) {
			status = decumaAllocationFailed;
			goto aslSGEvaluateArcs_failure;
		}

		if (pSG->nNodes > 1)
		{
			/* Set the properties of all edges of pNode */

			status = setEdges(pSG);
			if (status != decumaNoError) {
				goto aslSGEvaluateArcs_failure;
			}
		}
		else
		{
			/* At this point the node should have number of complete edges initialized */
			/* to 0 (all bytes of the SG are zero initialized by aslSGInit()). */

			decumaAssert(pNode->nCompleteEdges == 0);
		}
	}

	decumaAssert(pSG->nNodes == nArcSessionArcs + 1);

	return status;

aslSGEvaluateArcs_failure:

	/* State needs to be ok for next call. Easy solution: reset to initial state. */
	aslSGReset(pSG);

	return status;
}


DECUMA_STATUS aslSGIndicateInstantGesture( ASL_SG * pSG,
	int * pbInstantGesture,
	const DECUMA_INSTANT_GESTURE_SETTINGS * pInstantGestureSettings)
{
	int nArcSessionArcs;
	DECUMA_CURVE curve;
	int bRecognizeGesture=0;
	DECUMA_STATUS status;
	void* pMemoryBuffer=NULL;
	int nStartArc, nResults;
	const DECUMA_MEM_FUNCTIONS * pMemFunctions = pSG->pMemFunctions;
	int baseline, helpline;

	decumaAssert(pSG);
	decumaAssert(pbInstantGesture);
	*pbInstantGesture = 0;
	nArcSessionArcs= aslArcSessionGetArcCount(pSG->pArcSession);
	if (nArcSessionArcs == 0) return decumaNoError; /*Nothing to recognize */

	nStartArc = nArcSessionArcs-1;

	while (nStartArc > 0 &&
		aslArcSessionArcsAreConcurrent(pSG->pArcSession,nStartArc-1,nStartArc))
	{
		nStartArc--;
	}

	curve = *aslArcSessionGetCurve(pSG->pArcSession);
	curve.pArcs += nStartArc;
	curve.pArcTimelineDiff += nStartArc;
	curve.nArcs = nArcSessionArcs - nStartArc;

	baseline = aslArcSessionGetBaseline(pSG->pArcSession);
	helpline = baseline - aslArcSessionGetDistBaseToHelpline(pSG->pArcSession);

	if (nStartArc == 0)
	{
		/*This is the first arc (or first multitouch arcs) */
		bRecognizeGesture=1;
	}
	else if (nStartArc < nArcSessionArcs-1)
	{
		/*We have multitouch - go ahead even if not first in the arc session */
		bRecognizeGesture=1;
	}
	else if (curveIsBigOrOffEnoughToBeQuickGesture(
		&curve,baseline,helpline,pInstantGestureSettings))
	{
		/*The curve is very big or very off compared to previous input */
		bRecognizeGesture=1;
	}

	if (bRecognizeGesture)
	{
		/*See if the curve is recognized as a gesture (as first candidate) */
		SCR_SETTINGS scr_settings;
		SCR_RESULT scr_result;
		SCR_OUTPUT scr_output;
		DECUMA_UNICODE text[3];

		decumaMemset(&scr_settings, 0, sizeof(scr_settings));
		scr_settings.characterSet = *pSG->pCharacterSet;
		scr_settings.pCatTable = pSG->pDynCatTable;
		scr_settings.pDB = (STATIC_DB_PTR)pSG->pStaticDB;
		scr_settings.pUDM = (UDM_PTR)pSG->pDynamicDB;
		scr_settings.refangleimportance = FULL_ROTATION_IMPORTANCE;
		scr_settings.nBaseLineY = aslArcSessionGetBaseline(pSG->pArcSession);
		scr_settings.nHelpLineY = scr_settings.nBaseLineY - aslArcSessionGetDistBaseToHelpline(pSG->pArcSession);

		pMemoryBuffer = aslCalloc(scrGetMemoryBufferSize());

		if (!pMemoryBuffer)
		{
			status=decumaAllocationFailed;
			goto aslSGIndicateQuickGesture_error;
		}

		decumaMemset(&scr_result, 0, sizeof(scr_result));
		decumaMemset(&scr_output, 0, sizeof(scr_output));

		scr_result.pText = &text[0];
		scr_result.nMaxText = 3;

		status = scrRecognizeExtra(&curve, &scr_result, &scr_output,
		   1, &nResults, &scr_settings, pMemoryBuffer,NULL);
		if (status != decumaNoError) goto aslSGIndicateQuickGesture_error;

		if (nResults > 0)
		{
			int bGesture;
			DECUMA_STATUS tempStatus = scrOutputIsGesture(&scr_output,&bGesture,pbInstantGesture);
			decumaAssert(tempStatus == decumaNoError);
		}
		aslFree(pMemoryBuffer);
	}

	return decumaNoError;

aslSGIndicateQuickGesture_error:
	if (pMemoryBuffer)
	{
		aslFree(pMemoryBuffer);
	}
	return status;

}

void aslSGAdjustForNewEstimates(ASL_SG* pSG, DECUMA_COORD nNewBaseLineEstimate, DECUMA_COORD nNewHelpLineEstimate)
{
	int n, e;

	for (n = 0; n < pSG->nNodes; n++)
	{
		ASL_SG_NODE* pNode = pSG->pNodes[n];
		int bSorted = 0;

		for (e = 0; e < pNode->nCompleteEdges; e++)
		{
			ASL_SG_EDGE* pEdge = &pNode->pCompleteEdges[e];
			DECUMA_STATUS status;
			long nOldDistance = 0, nNewDistance = 0;
			int nArcs = n - pEdge->nStartNode;

			if (pEdge->scr_output.symbol)
			{
				/* TODO Are conflicts handled properly? */

				nOldDistance = (pEdge->scr_output.mu + pEdge->scr_output.punish) * nArcs;

				status = scrAdjustMuAndPunish(&pEdge->scr_output, &pEdge->scr_curve_prop,
					nNewBaseLineEstimate, nNewHelpLineEstimate,
					aslArcSessionGetBaseline(pSG->pArcSession),
					aslArcSessionGetBaseline(pSG->pArcSession) - aslArcSessionGetDistBaseToHelpline(pSG->pArcSession));

				decumaAssert(status == decumaNoError);

				nNewDistance = (pEdge->scr_output.mu + pEdge->scr_output.punish) * nArcs;
			}

			pEdge->nDistance += (nNewDistance - nOldDistance);

			if (pEdge->nStartNode > 0)
			{
				decumaAssert(pEdge->nBestConnector == 0);
				findBestConnector(pSG, n, pEdge, DECUMA_MAX_DISTANCE, MAX_DECUMA_INT16, &pEdge->nBestPathDist, &pEdge->nBestConnector, &pEdge->nBestConnectorConnDist);
				decumaAssert(pEdge->nBestConnector == 0);
				decumaAssert(pEdge->nBestConnectorConnDist == FIX_CONN_DIST ||
					pEdge->nBestConnectorConnDist - CYRILLIC_CONN_DIST_OFFSET == FIX_CONN_DIST ||
					pEdge->nBestConnectorConnDist - THAI_CONN_DIST_OFFSET == FIX_CONN_DIST ||
					pEdge->nBestConnectorConnDist == NOISE_TO_NOISE_CONN_DIST ||
					pEdge->nBestConnectorConnDist - CYRILLIC_CONN_DIST_OFFSET == NOISE_TO_NOISE_CONN_DIST ||
					pEdge->nBestConnectorConnDist - THAI_CONN_DIST_OFFSET == NOISE_TO_NOISE_CONN_DIST);
				pEdge->nBestPathDist += pEdge->nDistance;
			}
			else
			{
				pEdge->nBestPathDist = pEdge->nDistance;

				decumaAssert(pEdge->nBestConnector == -1);
			}
		}

		/* Re-sort */
		while (!bSorted)
		{
			bSorted = 1;

			for (e = 1; e < pNode->nCompleteEdges; e++)
			{
				if (pNode->pCompleteEdges[e].nBestPathDist < pNode->pCompleteEdges[e-1].nBestPathDist)
				{
					ASL_SG_EDGE tmpEdge = pNode->pCompleteEdges[e];
					pNode->pCompleteEdges[e] = pNode->pCompleteEdges[e-1];
					pNode->pCompleteEdges[e-1] = tmpEdge;
					bSorted = 0;
				}
			}
		}
	}
}

DECUMA_STATUS aslSGSetStringStart(ASL_SG* pSG,
								  const DECUMA_UNICODE* pSymbol,
								  int nSymbolLen)
{
	/* Creates a start string edge in node 0 */
	const DECUMA_MEM_FUNCTIONS* pMemFunctions = pSG->pMemFunctions;
	ASL_SG_NODE* pNode;
	ASL_SG_EDGE* pEdge;

	/* Assign pNode to node 0 (create it if not existing) */
	if (pSG->nNodes < 1)
		pNode = sgAddNode(pSG); /* Node 0 does not exist, create it */
	else
		pNode = pSG->pNodes[0];

	if (!pNode) return decumaAllocationFailed;

	if (pNode->nCompleteEdgesLists) sgNodeRemoveCompleteEdgesList(pSG, pNode);

	if (nSymbolLen > 0) /*Don't create an edge for an empty start string */
	{

		pEdge = aslCalloc(sizeof(pEdge[0]) + (nSymbolLen+1) * sizeof(pSymbol[0]));

		if (!pEdge) return decumaAllocationFailed;

		/* Set the edge's symbol to pSymbol */
		pEdge->scr_output.symbol = (DECUMA_UNICODE*)&pEdge[1]; /* Point to allocated symbol memory */

		/* Copy pSymbol to edge */
		decumaAssert(sizeof(DECUMA_UNICODE) == sizeof(DECUMA_UNICODE));
		decumaMemcpy((DECUMA_UNICODE*)pEdge->scr_output.symbol, pSymbol, nSymbolLen * sizeof(pSymbol[0]));
		((DECUMA_UNICODE*)(pEdge->scr_output.symbol))[nSymbolLen] = 0; /* Zero-terminate */

		/* Assign pre-allocated edge to edge list of node 0 */
		sgNodeAddCompleteEdgesList(pSG, pNode, pEdge, 1);
	}

	return decumaNoError;
}

#ifdef _DEBUG
int aslSGGetNodeCount(const ASL_SG* pSG)
{
	decumaAssert(pSG);

	return aslSGGetNodeCountMacro(pSG);
}

const ASL_SG_NODE* aslSGGetNode(const ASL_SG* pSG, int nIndex)
{
	decumaAssert(pSG);
	decumaAssert(nIndex >= 0);
	decumaAssert(nIndex < pSG->nNodes);

	return aslSGGetNodeMacro(pSG, nIndex);
}

/* asl SG node */

int aslSGGetEdgeCount(const ASL_SG_NODE* pNode)
{
	decumaAssert(pNode);

	return aslSGGetEdgeCountMacro(pNode);
}

const ASL_SG_EDGE* aslSGGetEdge(const ASL_SG_NODE* pNode, int nIndex)
{
	decumaAssert(pNode);
	decumaAssert(nIndex >= 0);
	decumaAssert(nIndex < pNode->nCompleteEdges);

	return aslSGGetEdgeMacro(pNode, nIndex);
}

int aslSGGetBestEdgeIndex(const ASL_SG_NODE* pNode)
{
	decumaAssert(pNode);
	decumaAssert(findBestEdge(pNode->pCompleteEdges, pNode->nCompleteEdges) == aslSGGetBestEdgeIndexMacro(pNode));

	return aslSGGetBestEdgeIndexMacro(pNode);
}

/* asl SG edge */

DECUMA_UNICODE* aslSGGetSymbol(const ASL_SG_EDGE* pEdge)
{
	decumaAssert(pEdge);

	return aslSGGetSymbolMacro(pEdge);
}

int aslSGGetStartNode(const ASL_SG_EDGE* pEdge)
{
	decumaAssert(pEdge);

	return aslSGGetStartNodeMacro(pEdge);
}

int aslSGGetBestConnectorEdgeIndex(const ASL_SG_EDGE* pEdge)
{
	decumaAssert(pEdge);

	return aslSGGetBestConnectorEdgeIndexMacro(pEdge);
}

int aslSGGetBestPathDistance(const ASL_SG_EDGE* pEdge)
{
	decumaAssert(pEdge);

	return aslSGGetBestPathDistanceMacro(pEdge);
}
#endif

long aslSGGetDistance(const ASL_SG_EDGE* pEdge)
{
	decumaAssert(pEdge);

	return pEdge->nDistance;
}

long aslSGGetBestConnectorConnectionDist(const ASL_SG_EDGE* pEdge)
{
	decumaAssert(pEdge);
	decumaAssert(pEdge->nStartNode >= 0);

	return pEdge->nBestConnectorConnDist;
}

int aslSGGetConnectionDistance(const ASL_SG * pSG,
	int nNode, int edgeIdx, int prevEdgeIdx)
{
	const ASL_SG_EDGE * pEdge;
	const ASL_SG_EDGE * pPrevEdge;
	int nConnDist;
	int nPrevEdgeNode;

	decumaAssert(pSG);
	decumaAssert(nNode>0 && nNode < pSG->nNodes);

	pEdge = &pSG->pNodes[nNode]->pCompleteEdges[edgeIdx];

	if (prevEdgeIdx == pEdge->nBestConnector) return pEdge->nBestConnectorConnDist;

	nPrevEdgeNode = pEdge->nStartNode;

	pPrevEdge = &pSG->pNodes[nPrevEdgeNode]->pCompleteEdges[prevEdgeIdx];

	nConnDist = matchEdgeConnection(pSG, nNode, pEdge, pPrevEdge);

	return nConnDist;
}

int aslSGGetBestStringDistance(const ASL_SG * pSG)
{
	int nNode = pSG->nNodes - 1;

	if (nNode >= 0)
	{
		const ASL_SG_NODE * pNode = pSG->pNodes[nNode];
		if ( pNode->pCompleteEdges )
		{
			int nBestEdge = aslSGGetBestEdgeIndex(pNode);
			return pNode->pCompleteEdges[nBestEdge].nBestPathDist;
		}
	}

	return 0;
}

void aslSGGetBestString(const ASL_SG * pSG,
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
						DECUMA_UINT8 * pbEndsWithInstantGesture)
{
	DECUMA_INT16 nNode = (DECUMA_INT16)(pSG->nNodes - 1);
	const ASL_SG_NODE * pNode = pSG->pNodes[nNode];
	int nBestEdge = nStrIdx;
	DECUMA_INT32 nAverageDistBaseToHelpLineEstimate = 0;
	DECUMA_INT32 nAverageBaseLineEstimate = 0;
	int nEstimates = 0;
	int i,j;

	if (pnChars) *pnChars = 0;
	if (pnResultingChars) *pnResultingChars = 0;
	if (pChars) pChars[0]=0;

	if (pnSymbolStrokesLen && pnResultingSymbolStrokesLen)
	{
		*pnSymbolStrokesLen = 0;
		*pnResultingSymbolStrokesLen = 0;
	}
	else
	{
		/* Cannot use segmentation buffer */
		pSymbolStrokes = NULL;
	}

	if (pnSymbolCharsLen && pnResultingSymbolCharsLen)
	{
		*pnSymbolCharsLen = 0;
		*pnResultingSymbolCharsLen = 0;
	}
	else
	{
		/* Cannot use segmentation buffer */
		pSymbolChars = NULL;
	}

	if (pnSymbolArcTimelineDiffMaskLen && pnResultingSymbolArcTimelineDiffMaskLen)
	{
		*pnSymbolArcTimelineDiffMaskLen = 0;
		*pnResultingSymbolArcTimelineDiffMaskLen = 0;
	}
	else
	{
		/* Cannot use segmentation buffer */
		pSymbolArcTimelineDiffMask = NULL;
	}
	if (pbEndsWithGesture || pbEndsWithInstantGesture)
	{
		int bGest=0,bInstGest=0;
		aslSGEdgeIsGesture(&pNode->pCompleteEdges[nBestEdge],&bGest,&bInstGest);
		if (pbEndsWithGesture) *pbEndsWithGesture = bGest;
		if (pbEndsWithInstantGesture) *pbEndsWithInstantGesture = bInstGest;
	}
	
	while (nNode > 0)
	{
		if (pNode->nCompleteEdges == 0)
		{
			/* No connector for ending reference? */
			pNode = pSG->pNodes[--nNode];
			nBestEdge = aslSGGetBestEdgeIndex(pNode);
			continue;
		}
		else
		{
			const ASL_SG_EDGE * pEdge = &pNode->pCompleteEdges[nBestEdge];
			const DECUMA_UNICODE * pUnicodes = pEdge->scr_output.outSymbol == alternative ? pEdge->alternativeSymbol : &pEdge->scr_output.symbol[0];
			DECUMA_UINT8 nUnicodes = 0;

			while (pUnicodes[nUnicodes]) nUnicodes++;

			if (pnChars && pChars)
			{
				/*Insert the unicodes */
				for (i=0; i<nUnicodes && *pnChars < (int)nMaxChars-1; i++)
				{
					decumaAssert(pUnicodes[nUnicodes-1-i]);
					pChars[*pnChars] = pUnicodes[nUnicodes-1-i]; /*Reverse the characters, since they are later reversed again... */
					(*pnChars)++;
				}
			}
			if (pnResultingChars) *pnResultingChars= (DECUMA_UINT16) (*pnResultingChars + nUnicodes);

			if (pSymbolStrokes)
			{
				if (*pnSymbolStrokesLen < nMaxSymbolStrokesLen)
				{
					if (*pnSymbolStrokesLen) pSymbolStrokes[*pnSymbolStrokesLen-1] -= (DECUMA_INT16)nNode;
					pSymbolStrokes[*pnSymbolStrokesLen] = nNode;
					(*pnSymbolStrokesLen)++;
				}
				(*pnResultingSymbolStrokesLen)++;
			}

			if (pSymbolChars)
			{
				if (*pnSymbolCharsLen < nMaxSymbolCharsLen)
				{
					pSymbolChars[*pnSymbolCharsLen] = nUnicodes;
					(*pnSymbolCharsLen)++;
				}
				(*pnResultingSymbolCharsLen)++;
			}

			if (pSymbolArcTimelineDiffMask)
			{
				if (*pnSymbolArcTimelineDiffMaskLen < nMaxSymbolArcTimelineDiffMaskLen)
				{
					pSymbolArcTimelineDiffMask[*pnSymbolArcTimelineDiffMaskLen] = pEdge->scr_output.arcTimelineDiffMask;
					(*pnSymbolArcTimelineDiffMaskLen)++;
				}
				(*pnResultingSymbolArcTimelineDiffMaskLen)++;
			}

			if (nUnicodes > 0 && pEdge->nBaseLineYEstimate != pEdge->nHelpLineYEstimate)
			{
				int bIsReliable;

				scrIsOutputEstimateReliable(&pEdge->scr_output, &bIsReliable);

				if (bIsReliable)
				{
					nAverageBaseLineEstimate += pEdge->nBaseLineYEstimate;
					nAverageDistBaseToHelpLineEstimate += pEdge->nBaseLineYEstimate - pEdge->nHelpLineYEstimate;
					nEstimates++;
				}
			}

			nNode = pEdge->nStartNode;
			nBestEdge = pEdge->nBestConnector;
			pNode = pSG->pNodes[nNode];

			decumaAssert(nNode <= 0 || nBestEdge != -1);
		}
	}

	decumaAssert(nNode == 0);

	if (pnChars && pChars)
	{
		/* pChars needs to be reversed */
		for (i = 0, j = (int)*pnChars-1; i < j; i++, j--)
		{
			DECUMA_UNICODE tmp = pChars[i];
			pChars[i] = pChars[j];
			pChars[j] = tmp;
		}

		pChars[*pnChars]=0;
	}

	if (pSymbolStrokes)
	{
		/* pSymbolStrokes needs to be reversed */
		for (i = 0, j = *pnSymbolStrokesLen-1; i < j; i++, j--)
		{
			DECUMA_INT16 tmp = pSymbolStrokes[i];
			pSymbolStrokes[i] = pSymbolStrokes[j];
			pSymbolStrokes[j] = tmp;
		}
	}

	if (pSymbolChars)
	{
		/* pSymbolChars needs to be reversed */
		for (i = 0, j = *pnSymbolCharsLen-1; i < j; i++, j--)
		{
			DECUMA_INT16 tmp = pSymbolChars[i];
			pSymbolChars[i] = pSymbolChars[j];
			pSymbolChars[j] = tmp;
		}
	}

	if (pSymbolArcTimelineDiffMask)
	{
		/* pSymbolArcTimelineDiffMask needs to be reversed */
		for (i = 0, j = *pnSymbolArcTimelineDiffMaskLen-1; i < j; i++, j--)
		{
			DECUMA_INT16 tmp = pSymbolArcTimelineDiffMask[i];
			pSymbolArcTimelineDiffMask[i] = pSymbolArcTimelineDiffMask[j];
			pSymbolArcTimelineDiffMask[j] = tmp;
		}
	}

	if (nEstimates)
	{
		nAverageBaseLineEstimate /= nEstimates;
		nAverageDistBaseToHelpLineEstimate /= nEstimates;
	}
	else
	{
		/* No new estimate available. Set estimate to the current one. */
		nAverageBaseLineEstimate = aslArcSessionGetBaseline(pSG->pArcSession);
		nAverageDistBaseToHelpLineEstimate = aslArcSessionGetDistBaseToHelpline(pSG->pArcSession);
	}

	if (pnAverageBaseLineEstimate) *pnAverageBaseLineEstimate = nAverageBaseLineEstimate;
	if (pnAverageHelpLineEstimate) *pnAverageHelpLineEstimate = nAverageBaseLineEstimate - nAverageDistBaseToHelpLineEstimate;
}

int aslSGGetEdgeSize(void)
{
	return sizeof(ASL_SG_EDGE);
}


DECUMA_COORD aslSGGetBaseLineYEstimate(const ASL_SG_EDGE * pEdge)
{
	return pEdge->nBaseLineYEstimate;
}

DECUMA_COORD aslSGGetHelpLineYEstimate(const ASL_SG_EDGE * pEdge)
{
	return pEdge->nHelpLineYEstimate;
}

int aslSGIsEdgeEstimateReliable(const ASL_SG* pSG, const ASL_SG_EDGE * pEdge)
{
	int bIsReliable;

	scrIsOutputEstimateReliable(&pEdge->scr_output, &bIsReliable);

	return bIsReliable;
}

const SCR_OUTPUT* aslSGGetOutput(const ASL_SG_EDGE * pEdge)
{
	return &pEdge->scr_output;
}

const SCR_CURVE_PROP* aslSGGetScrCurveProp(const ASL_SG_EDGE * pEdge)
{
	return &pEdge->scr_curve_prop;
}

const KID* aslSGGetKID(const ASL_SG_EDGE * pEdge)
{
	return &pEdge->scr_output.DBindex;
}

int aslSGGetPunish(const ASL_SG_EDGE * pEdge)
{
	return pEdge->scr_output.punish;
}

static int aslSGEdgeIsInstantGesture(const ASL_SG_EDGE * pEdge)
{
	int bGesture=0, bInstantGesture=0;
	if (!aslSGGetSymbol(pEdge)[0]) return 0; /*Noise */

	scrOutputIsGesture(&pEdge->scr_output,&bGesture, &bInstantGesture);

	return bInstantGesture;
}

void aslSGEdgeIsGesture(const ASL_SG_EDGE * pEdge, int * pbGesture, int * pbInstantGesture)
{
	decumaAssert(pbGesture);
	decumaAssert(pbInstantGesture);
	*pbGesture = *pbInstantGesture= 0;
	if (!aslSGGetSymbol(pEdge)[0]) return; /*Noise */

	scrOutputIsGesture(&pEdge->scr_output,pbGesture, pbInstantGesture);
}
/*///////////////////////////////////////////////////////////////// */
/* Private function definitions */
/* */



static void sgRelease(ASL_SG* pSG)
{
	const DECUMA_MEM_FUNCTIONS* pMemFunctions = pSG->pMemFunctions;
	decumaAssert(pSG);

	while (pSG->nNodes > 0) sgRemoveNode(pSG);

	if (pSG->pNodes) aslFree (pSG->pNodes);
}

static int sgEdgeComp(const void * pEdge1, const void * pEdge2)
{
	return ((const ASL_SG_EDGE*)(pEdge1))->nBestPathDist - ((const ASL_SG_EDGE*)(pEdge2))->nBestPathDist;
}

static DECUMA_STATUS setEdges(ASL_SG* pSG)
{
	const DECUMA_MEM_FUNCTIONS* pMemFunctions = pSG->pMemFunctions;
	SET_EDGES_DATA* pSED = &pSG->setEdgesData; /* Abbreviate for better readability */
	DECUMA_STATUS retVal = decumaAllocationFailed; /* Error by default */
	DECUMA_STATUS status;

	if(!initSetEdgesData(pSG, 0, NULL)) goto setEdges_exit;

	if (pSED->pCompleteCandHandler)
	{
		ASL_SG_EDGE* pNewCompleteEdges; /* New list of exact size */
		int i;
		int nCompleteEdges;

		decumaAssert(verifyCandHandler(pSED->pCompleteCandHandler));

		status = createNewEdges(pSG);
		if (status != decumaNoError) {
			retVal = status;
			goto setEdges_exit;
		}

		*pSED->pnCompleteEdges = decumaQCHGetCount(pSED->pCompleteCandHandler);

		if (*pSED->pnCompleteEdges > 0)
		{
			pNewCompleteEdges = aslCalloc(*pSED->pnCompleteEdges * sizeof(pNewCompleteEdges[0]));

			if (!pNewCompleteEdges) goto setEdges_exit;

			decumaQCHGetRanking(pSED->pCompleteCandHandler, pSED->pCompleteEdgesRanking);

			/* Copy edges in ranked order to new list */
			for (i = 0; i < *pSED->pnCompleteEdges; i++)
			{
				pNewCompleteEdges[i] = pSED->pCompleteEdges[pSED->pCompleteEdgesRanking[i]];
			}
		}

		nCompleteEdges = *pSED->pnCompleteEdges; /*NOTE: pnCompleteEdges should probably be changed to nCompleteEdges in the SED struct instead. This is a bad hertiage from UCR */
		sgNodeRemoveCompleteEdgesList(pSG, pSED->pNode);

		pSED->pCompleteEdges = nCompleteEdges > 0 ? sgNodeAddCompleteEdgesList(pSG, pSED->pNode, pNewCompleteEdges, nCompleteEdges) : NULL;

		decumaAssert(nCompleteEdges == 0 || pSED->pCompleteEdges); /* Adding pre-allocated list should never fail */

		/* Don't need handler anymore. Release it. */
		decumaAssert(pSED->pCompleteCandRefs);
#ifndef NO_REPLACEE_SEARCH
		decumaAssert(pSED->pCompleteCandHandlerCheckSum);
#endif
		if (pSED->pCompleteCandHandler != pSG->pCompleteCandHandler) aslFree(pSED->pCompleteCandHandler);
		if (pSED->pCompleteCandRefs != pSG->pCompleteCandRefs) aslFree(pSED->pCompleteCandRefs);
#ifndef NO_REPLACEE_SEARCH
		if (pSED->pCompleteCandHandlerCheckSum != pSG->pCompleteCandHandlerCheckSum) aslFree(pSED->pCompleteCandHandlerCheckSum);
#endif
	}
	else
	{
		decumaAssert(*pSED->pnCompleteEdges == 0);
	}

	retVal = decumaNoError;

setEdges_exit:

	destroySetEdgesData(pSG);

	return retVal;
}

static void findBestConnector(ASL_SG* pSG,
							  int nNode,
							  ASL_SG_EDGE* pEdge,
							  long nMaxPrevPathPlusConnDist,
							  int nMaxConnectionsToTest,
							  long* pnBestPrevPathPlusConnDist,
							  DECUMA_INT8* pnBestConnector,
							  long * pnBestConnectorConnDist)
{
	ASL_SG_NODE* pStartNode;
	ASL_SG_EDGE* pPrevEdge;
	RECOGNITION_MODE recognitionMode = pSG->recognitionMode;
	int nBestPathDist = nMaxPrevPathPlusConnDist;
	int nConnDist = 0;
	int nTestedNonPenliftLigatures = 0;

	*pnBestPrevPathPlusConnDist = 0;
	*pnBestConnector = -1;
	*pnBestConnectorConnDist = 0;

	if (pEdge->nStartNode <= 0) return;

	pStartNode = pSG->pNodes[pEdge->nStartNode];

	if (pStartNode->nCompleteEdges <= 0) return;

	/*TODO Find pPrevEdge (best connector) */
	pPrevEdge = &pStartNode->pCompleteEdges[0];

	if (!pEdge->scr_output.symbol && !pPrevEdge->scr_output.symbol)
		nConnDist += NOISE_TO_NOISE_CONN_DIST;
	else
		nConnDist += FIX_CONN_DIST;

	/* Thai characters tend to have under segmentation problem (contrary to the */
	/* over segmentation problem other scripts have). Adjust connection distance */
	/* accordingly. */
	if (IS_IN_CATEGORY(pSG->cat, pSG->thaiLetterCat)) nConnDist += THAI_CONN_DIST_OFFSET;
	else if (IS_IN_CATEGORY(pSG->cat, pSG->cyrillicLetterCat)) nConnDist += CYRILLIC_CONN_DIST_OFFSET;

	nBestPathDist = pPrevEdge->nBestPathDist;
	*pnBestPrevPathPlusConnDist = nBestPathDist + nConnDist;
	*pnBestConnector = 0;
	*pnBestConnectorConnDist = nConnDist;
}

static int initSetEdgesData(ASL_SG* pSG, int bDiac, int* pbNoEdgesToSet)
{
	const DECUMA_MEM_FUNCTIONS* pMemFunctions = pSG->pMemFunctions;
	SET_EDGES_DATA* pSED = &pSG->setEdgesData; /* Abbreviate for better readability */
	int arcIdx;

	int nMaxCompleteEdges = 0;

	decumaMemset(pSED, 0, sizeof(pSED[0]));

	decumaAssert(pSG->nNodes > 1);

	pSED->nNode = pSG->nNodes - 1;
	pSED->pNode = pSG->pNodes[pSED->nNode];

	pSED->nForcedStartNode = -1;
	pSED->nForcedEndNode = -1;

	pSED->nArcs = aslArcSessionGetArcCount(pSG->pArcSession);
	pSED->pArc = aslArcSessionGetArc(pSG->pArcSession,pSED->nNode - 1);

	pSED->pnCompleteEdges = &pSED->pNode->nCompleteEdges;
	pSED->pCompleteEdgesRanking = pSG->completeEdgesRanking;

	if (pSG->recognitionMode == scrMode &&
		pSED->nNode < aslArcSessionGetArcCount(pSG->pArcSession) &&
		!(aslArcIsStartingNewSymbol(aslArcSessionGetArc(pSG->pArcSession, pSED->nNode)) ||
		  aslArcSessionIsStartingNewSymbol(pSG->pArcSession) &&
		  aslArcSessionArcsAreConcurrent(pSG->pArcSession, pSED->nNode, pSED->nArcs) &&
		  !aslArcSessionArcsAreConcurrent(pSG->pArcSession, pSED->nNode-1, pSED->nNode)))
	{
		/* Next arc does not force start of a new symbol. We cannot end an SCR symbol in this arc. */
		/* To reduce total RAM allocation we need to make sure an edge list is not allocated here. */

		return 1;
	}

	{
		/* TODO Add function in arc session to count number of concurrent arcs directly? */
		int nConcurrent = 0;
		for (arcIdx=0; arcIdx<pSED->nArcs-1; arcIdx++)
		{
			if (aslArcSessionArcsAreConcurrent(pSG->pArcSession,arcIdx,arcIdx+1))
			{
				nConcurrent++;
			}
		}

		if ( nConcurrent > 0 )
		{
			if ( !aslAddElems(&pSED->pForbiddenNodes, pSED->nForbiddenNodes, nConcurrent, sizeof(pSED->pForbiddenNodes[0]), pSG->pMemFunctions) )
			{
				return 0;
			}

			for (arcIdx=0; arcIdx<pSED->nArcs-1; arcIdx++)
			{
				if (aslArcSessionArcsAreConcurrent(pSG->pArcSession,arcIdx,arcIdx+1))
				{
					pSED->pForbiddenNodes[pSED->nForbiddenNodes++]= arcIdx+1;
				}
			}
		}
	}

	{
		nMaxCompleteEdges = MAX_EDGES_PER_NODE;

		if (pSG->recognitionMode == scrMode) nMaxCompleteEdges = NUMBER_OF_SCR_RESULTS; /* Save RAM in SCR mode */

		if (pSG->bUseForcedSegmentation)
		{
			pSED->nForcedStartNode = getForcedStartNode(pSG, pSED->nNode);
			pSED->nForcedEndNode = getForcedEndNode(pSG, pSED->nNode-1);
		}

		if (pSED->nForcedEndNode >= 0 && pSED->nNode != pSED->nForcedEndNode)
		{
			/* No complete edges allowed. No list needed. */
		}
		else
		{
			pSED->pCompleteEdges = sgNodeAddCompleteEdgesList(pSG, pSED->pNode, NULL, nMaxCompleteEdges);

			if (!pSED->pCompleteEdges) return 0;
		}
	}

	/* IMPORTANT: */
	/* This function must be called after a node has been added (except the first node) */
	/* and must never be called again until another node has been added. */

	decumaAssert(*pSED->pnCompleteEdges == 0);

	if (pSED->nForcedEndNode >= 0 && pSED->nNode != pSED->nForcedEndNode || nMaxCompleteEdges == 0)
	{
		/* No complete edges allowed. No cand handlers needed. */
	}
	else
	{
		if (pSG->pCompleteCandHandler)
		{
			/* Use handler memory provided in session memory */
			pSED->pCompleteCandHandler = pSG->pCompleteCandHandler;
			pSED->pCompleteCandRefs = pSG->pCompleteCandRefs;
		}
		else
		{
			/* Allocate handler memory */
			pSED->pCompleteCandHandler = aslCalloc(decumaQCHGetSize());

			if (!pSED->pCompleteCandHandler) return 0;

			pSED->pCompleteCandRefs = aslCalloc(nMaxCompleteEdges * sizeof(pSED->pCompleteCandRefs[0]));

			if (!pSED->pCompleteCandRefs) return 0;
		}

		decumaQCHInit(pSED->pCompleteCandHandler, pSED->pCompleteCandRefs, pSED->pCompleteEdges,
			nMaxCompleteEdges,
			sizeof(pSED->pCompleteEdges[0]),
			(char*)&pSED->pCompleteEdges[0].nBestPathDist - (char*)&pSED->pCompleteEdges[0]);

#ifndef NO_REPLACEE_SEARCH
		if (pSG->pCompleteCandHandlerCheckSum)
		{
			/* Use handler memory provided in session memory */
			pSED->pCompleteCandHandlerCheckSum = pSG->pCompleteCandHandlerCheckSum;
		}
		else
		{
			/* Allocate handler memory */
			pSED->pCompleteCandHandlerCheckSum = aslCalloc(decumaHHGetSize(nMaxCompleteEdges,decumaHHSizeSmall));

			if (!pSED->pCompleteCandHandlerCheckSum) return 0;
		}

		decumaHHInit(pSED->pCompleteCandHandlerCheckSum, nMaxCompleteEdges,decumaHHSizeSmall);
#endif
	}

	return 1;
}

static void destroySetEdgesData(ASL_SG* pSG)
{
	const DECUMA_MEM_FUNCTIONS* pMemFunctions = pSG->pMemFunctions;
	SET_EDGES_DATA* pSED = &pSG->setEdgesData; /* Abbreviate for better readability */

	/* Free handler memory allocated for temporary use when setting edges */

	if (pSED->pCompleteCandHandler && pSED->pCompleteCandHandler != pSG->pCompleteCandHandler) aslFree(pSED->pCompleteCandHandler);
	if (pSED->pCompleteCandRefs && pSED->pCompleteCandRefs != pSG->pCompleteCandRefs) aslFree(pSED->pCompleteCandRefs);
#ifndef NO_REPLACEE_SEARCH
	if (pSED->pCompleteCandHandlerCheckSum && pSED->pCompleteCandHandlerCheckSum != pSG->pCompleteCandHandlerCheckSum) aslFree(pSED->pCompleteCandHandlerCheckSum);
#endif
	if (pSED->pForbiddenNodes) aslFree(pSED->pForbiddenNodes);
}

static DECUMA_STATUS createNewEdges(ASL_SG* pSG)
{
	const DECUMA_MEM_FUNCTIONS* pMemFunctions = pSG->pMemFunctions;
	SET_EDGES_DATA* pSED = &pSG->setEdgesData; /* Abbreviate for better readability */

	/* TODO replace dummy stuff */
	ASL_SG_EDGE dummyEdge;
	DECUMA_CURVE curve;
	SCR_OUTPUT scr_output[NUMBER_OF_SCR_RESULTS];
	SCR_RESULT scr_result[NUMBER_OF_SCR_RESULTS];
	SCR_SETTINGS scr_settings;
	DECUMA_STATUS status = decumaNoError;
	int nResults;
	void* pMemoryBuffer = NULL;
	DECUMA_UNICODE text[NUMBER_OF_SCR_RESULTS][2];
	int i;
	DECUMA_CURVE connectedArcsCurve; /* TODO keep precalculated in arc session */
	int nDistBaseToHelpline = aslArcSessionGetDistBaseToHelpline(pSG->pArcSession);

	decumaMemset(&connectedArcsCurve, 0, sizeof(connectedArcsCurve));

	for (i=0; i<pSED->nForbiddenNodes; i++)
	{
		if (pSED->nNode == pSED->pForbiddenNodes[i])
		{
			/*We are on a forbidden node (e.g. in the middle of a multitouch-arc), */
			/*don't create any non-noise edges to this node */
			goto createNewEdges_AddNoiseEdge;
		}
	}

	curve = *aslArcSessionGetCurve(pSG->pArcSession);
	curve.nArcs = pSED->nNode; /* Process the arc corresponding to the added node */

	decumaMemset(&scr_settings, 0, sizeof(scr_settings));
	scr_settings.characterSet = *pSG->pCharacterSet;
	scr_settings.pCatTable = pSG->pDynCatTable;
	scr_settings.pDB = (STATIC_DB_PTR)pSG->pStaticDB;
	scr_settings.pUDM = (UDM_PTR)pSG->pDynamicDB;
	scr_settings.refangleimportance = FULL_ROTATION_IMPORTANCE;
	scr_settings.nBaseLineY = aslArcSessionGetBaseline(pSG->pArcSession);
	scr_settings.nHelpLineY = scr_settings.nBaseLineY - aslArcSessionGetDistBaseToHelpline(pSG->pArcSession);

	pMemoryBuffer = aslCalloc(scrGetMemoryBufferSize());

	if (!pMemoryBuffer) {
		status = decumaAllocationFailed;
		goto createNewEdges_exit;
	}

	decumaAssert(pSED->nForcedEndNode < 0 || pSED->nForcedEndNode == pSED->nNode);

	/* Build a curve with connected arc pairs */
	if (curve.nArcs > 1)
	{
		connectedArcsCurve.pArcs = aslCalloc((curve.nArcs-1) * sizeof(connectedArcsCurve.pArcs[0]));

		if (!connectedArcsCurve.pArcs) {
			status = decumaAllocationFailed;
			goto createNewEdges_exit;
		}

		connectedArcsCurve.nArcs = curve.nArcs - 1;

		for (i = 0; i < connectedArcsCurve.nArcs; i++)
		{
			connectedArcsCurve.pArcs[i].pPoints = aslCalloc((curve.pArcs[i].nPoints + curve.pArcs[i+1].nPoints) * sizeof(connectedArcsCurve.pArcs[i].pPoints[0]));

			if (!connectedArcsCurve.pArcs[i].pPoints) {
				status = decumaAllocationFailed;
				goto createNewEdges_exit;
			}

			connectedArcsCurve.pArcs[i].nPoints = curve.pArcs[i].nPoints + curve.pArcs[i+1].nPoints;

			decumaMemcpy(connectedArcsCurve.pArcs[i].pPoints, curve.pArcs[i].pPoints, curve.pArcs[i].nPoints * sizeof(connectedArcsCurve.pArcs[i].pPoints[0]));
			decumaMemcpy(&connectedArcsCurve.pArcs[i].pPoints[curve.pArcs[i].nPoints], curve.pArcs[i+1].pPoints, curve.pArcs[i+1].nPoints * sizeof(connectedArcsCurve.pArcs[i].pPoints[0]));
		}
	}

	for (; curve.nArcs > 0; curve.nArcs--, curve.pArcs++, (curve.pArcTimelineDiff ? curve.pArcTimelineDiff++ : NULL) )
	{
		int nConnectToNext, nResultsAdded = 0;

		for (nConnectToNext = -1; nConnectToNext < curve.nArcs-1; nConnectToNext++)
		{
			DECUMA_INT16 nStartNode = pSED->nNode - curve.nArcs;
			DECUMA_CURVE* pCurve;
			DECUMA_CURVE connectedArcCurve;
			DECUMA_COORD nBaseLineYEstimate, nHelpLineYEstimate;
			int bForbiddenNode = 0;

#define ALLOW_PENLIFT_NOISE
#ifndef ALLOW_PENLIFT_NOISE
			if (nConnectToNext >= 0) break;
#endif

			/* Is it possible to start an edge here? */
			if (nStartNode > 0 && pSG->pNodes[nStartNode]->nCompleteEdges == 0) continue;

			/* Are we allowed to start an edge here? */
			if (pSED->nForcedStartNode >= 0 && pSED->nForcedStartNode != nStartNode) continue;

			/* Check concurrent arc consistent segmentation */
			for (i=0; i<pSED->nForbiddenNodes; i++)
			{
				if (nStartNode == pSED->pForbiddenNodes[i])
				{
					bForbiddenNode = 1;
					break;
				}
			}
			if (bForbiddenNode) continue;

			/* Check that the first arc forces start of a new symbol in SCR mode */
			if (pSG->recognitionMode == scrMode &&
				!aslArcIsStartingNewSymbol(aslArcSessionGetArc(pSG->pArcSession, nStartNode))) continue;

			/* Check that no arc other than the first one forces start of a new symbol */
			for (i = nStartNode+1; i < pSED->nNode; i++)
			{
				if (aslArcIsStartingNewSymbol(aslArcSessionGetArc(pSG->pArcSession, i)))
				{
					bForbiddenNode = 1;
					break;
				}
			}
			if (bForbiddenNode) continue;

			/* TODO Some smart speed opt is required here */
/*#define SPEED_OPT */
#ifdef SPEED_OPT
			if (nConnectToNext > 0) break;
#endif

			if (nConnectToNext >= 0)
			{
				DECUMA_POINT penliftStartPt, penliftEndPt;
				int penliftLen2;
				long j;

				if (curve.pArcTimelineDiff && !curve.pArcTimelineDiff[nConnectToNext]) continue; /*We cannot connect two multitouch arcs */

				penliftStartPt = curve.pArcs[nConnectToNext].pPoints[curve.pArcs[nConnectToNext].nPoints-1];
				penliftEndPt = curve.pArcs[nConnectToNext+1].pPoints[0];

				penliftLen2 = (penliftStartPt.x - penliftEndPt.x) * (penliftStartPt.x - penliftEndPt.x) +
					(penliftStartPt.y - penliftEndPt.y) * (penliftStartPt.y - penliftEndPt.y);

				/* Only consider small penlifts as noise to reduce response-time */
				/* TODO More sophisticated speed opt is needed if this is going to be effective */
				/* as a DB defiency robustness solution. Ideally incremental stroke recognition */
				/* is needed. */
				if (4 * 4 * penliftLen2 > nDistBaseToHelpline * nDistBaseToHelpline) continue;

				decumaMemset(&connectedArcCurve, 0, sizeof(connectedArcCurve));

				connectedArcCurve.pArcs = aslCalloc((curve.nArcs - 1) * sizeof(connectedArcCurve.pArcs[0]));

				if (!connectedArcCurve.pArcs) {
					status = decumaAllocationFailed;
					goto createNewEdges_exit;
				}

				connectedArcCurve.nArcs = curve.nArcs - 1;

				/* Connect arc nConnectToNext to nConnectToNext+1 */

				for (j = 0; j < nConnectToNext; j++)
					connectedArcCurve.pArcs[j] = curve.pArcs[j];

				connectedArcCurve.pArcs[j] = connectedArcsCurve.pArcs[j+connectedArcsCurve.nArcs-curve.nArcs+1];

				for (j++; j < connectedArcCurve.nArcs; j++)
					connectedArcCurve.pArcs[j] = curve.pArcs[j+1];

				pCurve = &connectedArcCurve;
			}
			else
			{
				pCurve = &curve;
			}

			decumaMemset(scr_result, 0, sizeof(scr_result));

			for (i = 0; i < NUMBER_OF_SCR_RESULTS; i++)
			{
				/* NOTE We only need this buffer to retrieve alternative symbol */
				/* which we assume is always exactly one character */
				scr_result[i].pText = text[i];
				scr_result[i].nMaxText = 2;
			}

			decumaMemset(scr_output, 0, sizeof(scr_output));


			status = scrRecognizeExtra(pCurve, scr_result, scr_output,
			   NUMBER_OF_SCR_RESULTS, &nResults, &scr_settings, pMemoryBuffer,NULL);

			if (status != decumaNoError) {
				goto createNewEdges_exit;
			}


#ifdef DEBUG_OUTPUT
		fprintf(g_debugFile,"SG: SCR: nResults==%d\n",nResults);
		fflush(g_debugFile);
#endif /*DEBUG_OUTPUT */

			decumaMemset(&dummyEdge, 0, sizeof(dummyEdge));

			for (i = 0; i < nResults; i++)
			{
				/* Fix proximity to grow with rank (the conflict handling can cause this inconsistency) */
				if (i > 0 && scr_result[i].proximity < scr_result[i-1].proximity)
				{
					/* Make the edges of the conflicting symbols identical except for symbol by */
					/* copying their output and curve properties. This is needed for equal further */
					/* processing (edge adjustment for new estimate and RG connection). Then just */
					/* let the non-prioritized symbol get 1 higher proximity than the prioritized */
					/* symbol. */
					const DECUMA_UNICODE* pSymbol = scr_output[i].symbol;
					OUT_SYMBOL outSymbol = scr_output[i].outSymbol;

					scr_result[i].proximity = scr_result[i-1].proximity;
					scr_output[i] = scr_output[i-1];
					scr_output[i].symbol = pSymbol;
					scr_output[i].outSymbol = outSymbol;
					scr_result[i].proximity++;
				}

				dummyEdge.nArcs = curve.nArcs;
				dummyEdge.nStartNode = nStartNode;
				dummyEdge.scr_output = scr_output[i];
				dummyEdge.nDistance = scr_result[i].proximity * curve.nArcs;
				if (nConnectToNext >= 0) dummyEdge.nDistance += 325; /* Penlift noise punish */
				dummyEdge.nBestPathDist =  dummyEdge.nDistance;
				if (dummyEdge.nStartNode > 0)
				{
					dummyEdge.nBestConnectorConnDist = FIX_CONN_DIST;

					/* Thai characters tend to have under segmentation problem (contrary to the */
					/* over segmentation problem other scripts have). Adjust connection distance */
					/* accordingly. */
					if (IS_IN_CATEGORY(pSG->cat, pSG->thaiLetterCat)) dummyEdge.nBestConnectorConnDist += THAI_CONN_DIST_OFFSET;
					else if (IS_IN_CATEGORY(pSG->cat, pSG->cyrillicLetterCat)) dummyEdge.nBestConnectorConnDist += CYRILLIC_CONN_DIST_OFFSET;

					dummyEdge.nBestPathDist += dummyEdge.nBestConnectorConnDist + pSG->pNodes[dummyEdge.nStartNode]->pCompleteEdges[dummyEdge.nBestConnector].nBestPathDist;
				}
				else
					dummyEdge.nBestConnector = -1;

				if (scr_output[i].outSymbol == alternative)
				{
					decumaAssert(scr_result[i].pText[0] != 0);
					decumaAssert(scr_result[i].pText[0] != scr_output[i].symbol[0]);
					decumaAssert(scr_output[i].symbol[1] == 0);
					decumaAssert(dummyEdge.alternativeSymbol[1] == 0);
					dummyEdge.alternativeSymbol[0] = scr_result[i].pText[0];
				}
				else
				{
					decumaAssert(scr_output[i].symbol[0] != 0);
					decumaAssert(scr_result[i].pText[0] == scr_output[i].symbol[0]);
				}

				/* Get curve properties */
				scrGetCurveProp(&scr_output[i], &dummyEdge.scr_curve_prop, pMemoryBuffer);

				scrEstimateScalingAndVerticalOffset(&scr_output[i], &dummyEdge.nBaseLineYEstimate, &dummyEdge.nHelpLineYEstimate);

				addEdgeIfQualified(pSG, &dummyEdge, pSED->pCompleteCandHandler, DECUMA_MAX_DISTANCE, NULL);
			}

			nResultsAdded += nResults;

			if (nConnectToNext == curve.nArcs-2 && nResultsAdded == 0)
			{
				/* Add an edge with max dist just to ensure some path through the graph will exist */

				dummyEdge.nArcs = curve.nArcs;
				dummyEdge.nStartNode = nStartNode;
				if (pSG->recognitionMode != scrMode)
				{
					dummyEdge.nDistance = 1000 * curve.nArcs;
					if (nConnectToNext >= 0) dummyEdge.nDistance += 325; /* Penlift noise punish */
					dummyEdge.nBestPathDist =  dummyEdge.nDistance;
				}
				/* else */
				/* Don't dilute distance sum in forced nodes path */

				if (dummyEdge.nStartNode > 0)
				{
					if (pSG->recognitionMode == scrMode)
						dummyEdge.nBestConnectorConnDist = 0;
					else if (!dummyEdge.scr_output.symbol && !pSG->pNodes[dummyEdge.nStartNode]->pCompleteEdges[dummyEdge.nBestConnector].scr_output.symbol)
						dummyEdge.nBestConnectorConnDist = NOISE_TO_NOISE_CONN_DIST;
					else
						dummyEdge.nBestConnectorConnDist = FIX_CONN_DIST;

					/* Thai characters tend to have under segmentation problem (contrary to the */
					/* over segmentation problem other scripts have). Adjust connection distance */
					/* accordingly. */
					if (IS_IN_CATEGORY(pSG->cat, pSG->thaiLetterCat)) dummyEdge.nBestConnectorConnDist += THAI_CONN_DIST_OFFSET;
					else if (IS_IN_CATEGORY(pSG->cat, pSG->cyrillicLetterCat)) dummyEdge.nBestConnectorConnDist += CYRILLIC_CONN_DIST_OFFSET;

					dummyEdge.nBestPathDist += dummyEdge.nBestConnectorConnDist + pSG->pNodes[dummyEdge.nStartNode]->pCompleteEdges[dummyEdge.nBestConnector].nBestPathDist;
				}
				else
					dummyEdge.nBestConnector = -1;

				dummyEdge.scr_output.outSymbol = alternative; /* Trick to avoid returning null pointer in aslSGGetSymbol */

				decumaAssert(!aslSGGetSymbol(&dummyEdge) || !aslSGGetSymbol(&dummyEdge)[0]); /* This edge should not have a symbol */

				addEdgeIfQualified(pSG, &dummyEdge, pSED->pCompleteCandHandler, DECUMA_MAX_DISTANCE, NULL);
			}

			if (nConnectToNext >= 0) aslFree(connectedArcCurve.pArcs);
		}
	}

createNewEdges_AddNoiseEdge:

#define ALLOW_NOISE_STROKES
#if defined(ALLOW_NOISE_STROKES)
	if (pSG->recognitionMode != scrMode && /* No segmentation support in scrMode implies no noise support (otherwise mode speed edge would be lost) */
		(pSED->nNode == 1 || pSG->pNodes[pSED->nNode-1]->nCompleteEdges > 0))
	{
		/* Add (if qualified) a noise edge for the stroke. Note that this only support noise strokes between characters */
		/* not within characters. */
		decumaMemset(&dummyEdge, 0, sizeof(dummyEdge));

		dummyEdge.nArcs = 1;
		dummyEdge.nStartNode = pSED->nNode - 1;
		dummyEdge.nDistance = 1000;
		dummyEdge.nBestPathDist =  dummyEdge.nDistance;
		if (dummyEdge.nStartNode > 0)
		{
			if (!dummyEdge.scr_output.symbol && !pSG->pNodes[dummyEdge.nStartNode]->pCompleteEdges[dummyEdge.nBestConnector].scr_output.symbol)
				dummyEdge.nBestConnectorConnDist = NOISE_TO_NOISE_CONN_DIST;
			else
				dummyEdge.nBestConnectorConnDist = FIX_CONN_DIST;

			/* Thai characters tend to have under segmentation problem (contrary to the */
			/* over segmentation problem other scripts have). Adjust connection distance */
			/* accordingly. */
			if (IS_IN_CATEGORY(pSG->cat, pSG->thaiLetterCat)) dummyEdge.nBestConnectorConnDist += THAI_CONN_DIST_OFFSET;
			else if (IS_IN_CATEGORY(pSG->cat, pSG->cyrillicLetterCat)) dummyEdge.nBestConnectorConnDist += CYRILLIC_CONN_DIST_OFFSET;

			dummyEdge.nBestPathDist += dummyEdge.nBestConnectorConnDist + pSG->pNodes[dummyEdge.nStartNode]->pCompleteEdges[dummyEdge.nBestConnector].nBestPathDist;
		}
		else
			dummyEdge.nBestConnector = -1;

		dummyEdge.scr_output.outSymbol = alternative; /* Trick to avoid returning null pointer in aslSGGetSymbol */

		addEdgeIfQualified(pSG, &dummyEdge, pSED->pCompleteCandHandler, DECUMA_MAX_DISTANCE, NULL);
	}
#endif


createNewEdges_exit:

	if (connectedArcsCurve.pArcs)
	{
		for (i = 0; i < connectedArcsCurve.nArcs; i++)
		{
			if (connectedArcsCurve.pArcs[i].pPoints) aslFree(connectedArcsCurve.pArcs[i].pPoints);
		}
		aslFree(connectedArcsCurve.pArcs);
	}

	if (pMemoryBuffer) aslFree(pMemoryBuffer);

	return status;
}

static void initCandidateReplaceeParam(CANDIDATE_REPLACEE_PARAM* pReplaceeParam,
									   int bDiac,
									   int bCanCombineWithOtherPrevSegVars,
									   int bCompleteCandidate,
									   int bCompleteCandidateList)
{
	if (pReplaceeParam->bInitialized &&
		(bCompleteCandidate && !CLASS_BY_TYPE_FOR_COMPLETE_EDGES && MAX_COMPLETE_EDGES_OF_SAME_CLASS == 1))
	{
		/* Go with the currently set values from last iteration */

		/* Verify some assumptions */
		/*  - Complete candidates can qualify for both list, but incomplete only for the incomplete list */
		/*  - A found candidate was provided or the candidate pointer is set to 0 */
		decumaAssert(bCompleteCandidate || !bCompleteCandidateList);
		decumaAssert(pReplaceeParam->bFound || pReplaceeParam->pCandidate == 0);

		return;
	}

	/* Cannot use currently found (if any) candidate. Reset with default values. */
	pReplaceeParam->pCandidate = NULL;
	pReplaceeParam->pCandidateRef = NULL;
	pReplaceeParam->nBestPathDist = DECUMA_MAX_DISTANCE;
	pReplaceeParam->bFound = 0;
	pReplaceeParam->bInitialized = 1;

	pReplaceeParam->bPossibleNonWorst =
		/* Could have a replacee from another incomplete edges extension */
		bCompleteCandidate && !CLASS_BY_TYPE_FOR_COMPLETE_EDGES;
}

static void updateCandidateReplacee(CANDIDATE_REPLACEE_PARAM* pReplaceeParam,
									const DECUMA_QCH* pCH,
									const DECUMA_HH* pHH,
									int bCompleteCandHandler)
{
	decumaAssert(pReplaceeParam->pCandidate == 0);
	decumaAssert(pReplaceeParam->nBestPathDist == DECUMA_MAX_DISTANCE);

	if (pReplaceeParam->pCandidateRef)
	{
		/* This is the worst edge of the same class */
		pReplaceeParam->pCandidate = decumaHHGetData(pReplaceeParam->pCandidateRef);
		pReplaceeParam->nBestPathDist = pReplaceeParam->pCandidate->nBestPathDist;
	}
	else if (decumaQCHIsFull(pCH))
	{
		/* No same class edge found. Handler is full. Worst candidate should be the replacee. */
		pReplaceeParam->pCandidate = decumaQCHGetCand(decumaQCHGetPrevCandRef(decumaQCHGetEndOfCandRefs(pCH)));
		pReplaceeParam->nBestPathDist = pReplaceeParam->pCandidate->nBestPathDist;
		pReplaceeParam->pCandidateRef = decumaHHFindData(pHH,
			pReplaceeParam->pCandidate->nCheckSumComplete,
			pReplaceeParam->pCandidate);
	}
}

static DECUMA_HH_ELEMREF findWorstEdgeOfSameClass(const DECUMA_HH* pHHCheckSum,
												  DECUMA_HH_ELEMREF elemRef,
												  DECUMA_UINT32 nCheckSum,
												  int nArcs,
												  int nSymbol,
												  int nType,
												  int nMaxEdgesOfSameClass)
{
	const ASL_SG_EDGE* pEdge;
	DECUMA_HH_ELEMREF worstCandWithCheckSumElemRef = NULL;
	long nWorstEdgeWithCheckSumBestPathDist = -1;
	int nEdgesOfSameClass = 0;

	decumaAssert(nMaxEdgesOfSameClass > 0);

	/* Get a first candidate with checksum nCheckSum (if not provided) */
	/* Caller might have tested this existence condition to avoid expensive */
	/* function call (a lot of parameter copying). */
	if (elemRef == NULL) elemRef = decumaHHFindKey(pHHCheckSum, nCheckSum);

	while (elemRef)
	{
		decumaAssert(decumaHHGetKey(elemRef) == nCheckSum);

		pEdge = decumaHHGetData(elemRef);

		if (edgesBelongToSameCompleteEdgesClassMacro(
			0, /*TODO */
			0, /*TODO */
			0, /*TODO */
			pEdge))
		{
			if (pEdge->nBestPathDist > nWorstEdgeWithCheckSumBestPathDist)
			{
				worstCandWithCheckSumElemRef = elemRef;
				nWorstEdgeWithCheckSumBestPathDist = pEdge->nBestPathDist;
			}

			nEdgesOfSameClass++;
		}

		if (nEdgesOfSameClass == nMaxEdgesOfSameClass) break;

		elemRef = decumaHHFindNextKey(elemRef);
	}

	decumaAssert(nEdgesOfSameClass <= nMaxEdgesOfSameClass);

	if (nEdgesOfSameClass < nMaxEdgesOfSameClass) worstCandWithCheckSumElemRef = NULL;

	return worstCandWithCheckSumElemRef;
}

static int addEdgeIfQualified(ASL_SG* pSG, const ASL_SG_EDGE* pEdge, DECUMA_QCH * pCH, long nMaxDist, const DECUMA_QCH_CANDREF** ppCandidateToReplaceRef)
{
	decumaAssert(pSG);
	decumaAssert(pEdge);
	decumaAssert(pCH);

	decumaAssert(pEdge->nBestPathDist >= 0);
	decumaAssert(pEdge->nDistance >= 0);

	if (pEdge->nBestPathDist >= nMaxDist)
	{
		/* Keep graph small by not adding edges with max distance */
		/* TODO cut off edges with "too big" distance but less than max? */

		return decumaQCHGetCount(pCH);
	}

	/* TODO implement isEdgeValid and call that before (possible) addition */
	decumaAssert(pEdge->nBestConnector >= -1);

	decumaQCHAdd(pCH, pEdge->nBestPathDist, pEdge, ppCandidateToReplaceRef);

	/* Assume that the actual candidate data alreay had been added to the underlying buffer */
	decumaAssert(!ppCandidateToReplaceRef || *ppCandidateToReplaceRef && decumaQCHVerifyCandRef(pCH, *ppCandidateToReplaceRef));

	return decumaQCHGetCount(pCH);
}

static int verifyCandHandler(const DECUMA_QCH* pCH)
{
	const DECUMA_QCH_CANDREF* pCandRef;
	int i, nCands;

	nCands = decumaQCHGetCount(pCH);

	pCandRef = decumaQCHGetStartOfCandRefs(pCH);
	pCandRef++;

	for (i = 0; i < nCands; i++)
	{
		if (!decumaQCHVerifyCandRef(pCH, &pCandRef[i])) goto verifyCandHandler_failure;
		if (i > 0 && pCandRef[i].nKey < pCandRef[i-1].nKey) goto verifyCandHandler_failure;
	}

	return 1;

verifyCandHandler_failure:

	decumaAssert(0);

	return 0;
}

static int findBestEdge(const ASL_SG_EDGE* pEdges, int nEdges)
{
	long nBestEdgeBestPathDist;
	int nBestEdge = 0;
	int i;

	nBestEdgeBestPathDist = DECUMA_MAX_DISTANCE; /* Guaranteed larger or equal to the best edge's best path distance */

	for (i = 0; i < nEdges; i++)
	{
		const ASL_SG_EDGE* pEdge = &pEdges[i];
		long nBestPathDist = pEdge->nBestPathDist;

		if (nBestPathDist < nBestEdgeBestPathDist)
		{
			nBestEdgeBestPathDist = nBestPathDist;
			nBestEdge = i;
		}
	}

	return nBestEdge;
}

static int matchEdge(ASL_SG* pSG, int nNode, ASL_SG_EDGE* pEdge, long nMaxDist)
{
	int nDistance = 0;
	int retVal = 0;

	/* SCR match pEdge */

	/* */
	/* Connect pEdge */
	/* */

	{
		/* Find and make best connection with last segment of an edge ending in pEdge's start node. */

		long nBestPrevPathPlusConnDist;

		/* The limit parameters 1, 2, 3 seems to be the optimal compromise for hitrate and speed. Especially note */
		/* that higher limit not necessarily higher hitrate. */
		findBestConnector(pSG, nNode, pEdge, nMaxDist - nDistance - pEdge->nBestPathDist, 3,
			&nBestPrevPathPlusConnDist,
			&pEdge->nBestConnector,
			&pEdge->nBestConnectorConnDist);

		if (pEdge->nBestConnector >= 0)
		{
			/* Update best path distance */
			pEdge->nBestPathDist +=	nBestPrevPathPlusConnDist;
		}
		else
			pEdge->nBestPathDist = nMaxDist - nDistance; /* Unqualify */
	}

matchSegment_done:

	pEdge->nDistance += nDistance;
	pEdge->nBestPathDist += nDistance;
	decumaAssert(pEdge->nDistance >= 0);
	decumaAssert(pEdge->nBestPathDist >= 0);

	retVal = 1;

matchSegment_exit:

	return retVal;
}

static int matchEdgeConnection(const ASL_SG* pSG,
							   int nNode,
							   const ASL_SG_EDGE* pEdge,
							   const ASL_SG_EDGE* pPrevEdge)
{
	int nConnDist = 0;

	decumaAssert(pEdge);
	decumaAssert(pPrevEdge);

	if (!pEdge->scr_output.symbol && !pPrevEdge->scr_output.symbol)
		nConnDist += NOISE_TO_NOISE_CONN_DIST;
	else
		nConnDist += FIX_CONN_DIST;

	/* Thai characters tend to have under segmentation problem (contrary to the */
	/* over segmentation problem other scripts have). Adjust connection distance */
	/* accordingly. */
	if (IS_IN_CATEGORY(pSG->cat, pSG->thaiLetterCat)) nConnDist += THAI_CONN_DIST_OFFSET;
	else if (IS_IN_CATEGORY(pSG->cat, pSG->cyrillicLetterCat)) nConnDist += CYRILLIC_CONN_DIST_OFFSET;

	return nConnDist;
}

#ifdef ASL_HIDDEN_API

static int sgInitFilters(ASL_SG* pSG)
{
	const DECUMA_MEM_FUNCTIONS* pMemFunctions = pSG->pMemFunctions;
	PROTOTYPE_FILTER* pForwardFilter = 0;
	PROTOTYPE_FILTER* pBackwardFilter = 0;

	if (pSG->nFilters < pSG->nNodes)
	{
		pForwardFilter = (PROTOTYPE_FILTER*)aslCalloc(pSG->nNodes * sizeof(pSG->pForwardFilter[0]));

		if (!pForwardFilter) goto sgInitFilters_failure;

		pBackwardFilter = (PROTOTYPE_FILTER*)aslCalloc(pSG->nNodes * sizeof(pSG->pBackwardFilter[0]));

		if (!pBackwardFilter) goto sgInitFilters_failure;

		if (pSG->nFilters > 0)
		{
			decumaAssert(pSG->pForwardFilter);
			decumaAssert(pSG->pBackwardFilter);

			decumaMemcpy(pForwardFilter, pSG->pForwardFilter, pSG->nFilters * sizeof(pSG->pForwardFilter[0]));
			decumaMemcpy(pBackwardFilter, pSG->pBackwardFilter, pSG->nFilters * sizeof(pSG->pBackwardFilter[0]));

			aslFree(pSG->pForwardFilter);
			aslFree(pSG->pBackwardFilter);
		}
		else
		{
			decumaAssert(!pSG->pForwardFilter);
			decumaAssert(!pSG->pBackwardFilter);
		}

		pSG->pForwardFilter = pForwardFilter;
		pSG->pBackwardFilter = pBackwardFilter;

		pSG->nFilters = pSG->nNodes;
	}

	return 1;

sgInitFilters_failure:

	if (pForwardFilter) aslFree(pForwardFilter);
	if (pBackwardFilter) aslFree(pBackwardFilter);

	return 0;
}

/* TODO do not expose inner fields of the graph like this: */
PROTOTYPE_FILTER * aslSGGetForwardFilter ( const ASL_SG * pSG, int nNode )
{
	decumaAssert(nNode >= 0);
	decumaAssert(nNode < pSG->nNodes);

	if (!sgInitFilters(pSG)) return 0;

	return &pSG->pForwardFilter[nNode];
}

PROTOTYPE_FILTER * aslSGGetBackwardFilter ( const ASL_SG * pSG, int nNode )
{
	decumaAssert(nNode >= 0);
	decumaAssert(nNode < pSG->nNodes);

	if (!sgInitFilters(pSG)) return 0;

	return &pSG->pBackwardFilter[nNode];
}

static int filterPrototypes(ASL_SG * pSG, int nStartNode, int nEndNode, int nClass, int nSegments, int nProtIdx )
{
	/* Default true; if force information is missing allow any prototype to match */
	int bRet = 1;

	decumaAssert(nStartNode >= 0);
	decumaAssert(nStartNode < nEndNode);
	decumaAssert(nEndNode < pSG->nNodes);

	if (!sgInitFilters(pSG)) return bRet;

	if ( pSG->pForwardFilter[nStartNode].bUseFilter )
	{
		bRet = ( pSG->pForwardFilter[nStartNode].nClass == PROTOTYPE_FILTER_ANY || pSG->pForwardFilter[nStartNode].nClass == nClass ) &&
			   ( pSG->pForwardFilter[nStartNode].nProtIdx == PROTOTYPE_FILTER_ANY || pSG->pForwardFilter[nStartNode].nProtIdx == nProtIdx ) &&
			   ( pSG->pForwardFilter[nStartNode].nSegments == PROTOTYPE_FILTER_ANY || pSG->pForwardFilter[nStartNode].nSegments == nSegments);
	}

	if ( bRet && pSG->pBackwardFilter[nEndNode].bUseFilter )
	{
		bRet = ( pSG->pBackwardFilter[nEndNode].nClass == PROTOTYPE_FILTER_ANY || pSG->pBackwardFilter[nEndNode].nClass == nClass ) &&
			   ( pSG->pBackwardFilter[nEndNode].nProtIdx == PROTOTYPE_FILTER_ANY || pSG->pBackwardFilter[nEndNode].nProtIdx == nProtIdx ) &&
			   ( pSG->pBackwardFilter[nEndNode].nSegments == PROTOTYPE_FILTER_ANY || pSG->pBackwardFilter[nEndNode].nSegments == nSegments);
	}

	return bRet;
}
#endif

/* Returns the start node of an forced segmentation edge with */
/* nEdgeStartNode < nNode <= nEdgeEndNode */
static int getForcedStartNode(const ASL_SG * pSG, int nNode)
{
	int i, nForcedNode = 0;

	decumaAssert(nNode > 0);

	for (i = 0; i < pSG->nForcedSegmentationLen; i++)
	{
		nForcedNode += pSG->forcedSegmentation[i];
		if (nForcedNode >= nNode) return nForcedNode-pSG->forcedSegmentation[i];
	}

	decumaAssert(0);

	return nForcedNode;
}

/* Returns the end node of an forced segmentation edge with */
/* nEdgeStartNode <= nNode < nEdgeEndNode */
static int getForcedEndNode(const ASL_SG * pSG, int nNode)
{
	int i, nForcedNode = 0;

	decumaAssert(nNode >= 0);

	for (i = 0; i < pSG->nForcedSegmentationLen; i++)
	{
		nForcedNode += pSG->forcedSegmentation[i];
		if (nForcedNode > nNode) return nForcedNode;
	}

	decumaAssert(0);

	return nForcedNode;
}

static int curveIsBigOrOffEnoughToBeQuickGesture(DECUMA_CURVE * pCurve,
	int baseline, int helpline,
	const DECUMA_INSTANT_GESTURE_SETTINGS * pInstantGestureSettings)
{
#define WIDTH_THRESHOLD_NUM 7
#define WIDTH_THRESHOLD_DENOM 4
#define HEIGHT_THRESHOLD_NUM 2
#define HEIGHT_THRESHOLD_DENOM 1

	int a;
	DECUMA_COORD min_x = MAX_DECUMA_COORD;
	DECUMA_COORD min_y = MAX_DECUMA_COORD;
	DECUMA_COORD max_x = MIN_DECUMA_COORD;
	DECUMA_COORD max_y = MIN_DECUMA_COORD;
	int widthThreshold, heightThreshold;

	for(a = 0 ; a < pCurve->nArcs; a++)
	{
		DECUMA_ARC * pArc = &pCurve->pArcs[a];
		int i;
		for (i=0; i<pArc->nPoints;i++)
		{
			DECUMA_POINT pt = pArc->pPoints[i];
			if ( pt.x < min_x ) min_x = pt.x;
			if ( pt.x > max_x ) max_x = pt.x;
			if ( pt.y < min_y ) min_y = pt.y;
			if ( pt.y > max_y ) max_y = pt.y;
		}
	}

	if (baseline == helpline){
		widthThreshold = (int)pInstantGestureSettings->widthThreshold;
		heightThreshold = (int)pInstantGestureSettings->heightThreshold;
	} else {
		widthThreshold = ((baseline - helpline)*WIDTH_THRESHOLD_NUM)/WIDTH_THRESHOLD_DENOM;
		heightThreshold = ((baseline - helpline)*HEIGHT_THRESHOLD_NUM)/HEIGHT_THRESHOLD_DENOM;;
	}

	if (max_x - min_x >= widthThreshold) return 1;
	if (max_y - min_y >= heightThreshold) return 1;

	/*TODO: Also check if min_x is much larger than previous strokes' max_x */
	/*TODO: Also check if max_x is much smaller than previous strokes' min_x */

	return 0;
}


static ASL_SG_NODE* sgAddNode(ASL_SG* pSG)
{
	const DECUMA_MEM_FUNCTIONS* pMemFunctions = pSG->pMemFunctions;
	ASL_SG_NODE** ppNodes;
	int nNewNodeIdx;

	decumaAssert(pSG);

	nNewNodeIdx = pSG->nNodes;

	ppNodes = aslCalloc((pSG->nNodes + 1) * sizeof(pSG->pNodes[0]));

	if (!ppNodes) return NULL;

	if (pSG->pNodes)
	{
		decumaMemcpy(ppNodes, pSG->pNodes, pSG->nNodes * sizeof(pSG->pNodes[0]));
		aslFree(pSG->pNodes);
	}
	else
	{
		decumaAssert(nNewNodeIdx == 0);
	}

	pSG->pNodes = ppNodes;

	pSG->pNodes[nNewNodeIdx] = aslCalloc(sizeof(pSG->pNodes[nNewNodeIdx][0]));

	if (pSG->pNodes[nNewNodeIdx]) pSG->nNodes++;

	return pSG->pNodes[nNewNodeIdx];
}

static void sgRemoveNode(ASL_SG* pSG)
{
	const DECUMA_MEM_FUNCTIONS* pMemFunctions = pSG->pMemFunctions;
	ASL_SG_NODE* pNode;

	decumaAssert(pSG);
	decumaAssert(pSG->nNodes > 0);
	decumaAssert(pSG->pNodes[pSG->nNodes-1]);

	pNode = pSG->pNodes[pSG->nNodes-1];

	while (pNode->nCompleteEdgesLists > 0) sgNodeRemoveCompleteEdgesList(pSG, pNode);

	aslFree(pSG->pNodes[pSG->nNodes-1]);

	pSG->nNodes--;
}

static ASL_SG_EDGE* sgNodeAddCompleteEdgesList(const ASL_SG* pSG, ASL_SG_NODE* pNode, ASL_SG_EDGE* pEdgeList, int nListLen)
{
	const DECUMA_MEM_FUNCTIONS* pMemFunctions = pSG->pMemFunctions;
	decumaAssert(pNode);
	decumaAssert(nListLen <= MAX_EDGES_PER_NODE);

	if (pNode->nCompleteEdgesLists != 0) return NULL; /* Node is full */

	decumaAssert(pNode->pCompleteEdges == 0);

	if (pEdgeList)
	{
		/* Pre-allocated edge list provided */
		pNode->pCompleteEdges = pEdgeList;
		pNode->nCompleteEdges = nListLen;
	}
	else
	{
		/* Edge list not provided. Allocate nListLen long list. */

		pNode->pCompleteEdges = aslCalloc(nListLen * sizeof(pNode->pCompleteEdges[0]));
	}

	if (!pNode->pCompleteEdges) return NULL; /* Add failed */

	pNode->nCompleteEdgesLists++;

	return pNode->pCompleteEdges;
}

static void sgNodeRemoveCompleteEdgesList(const ASL_SG* pSG, ASL_SG_NODE* pNode)
{
	const DECUMA_MEM_FUNCTIONS* pMemFunctions = pSG->pMemFunctions;
	decumaAssert(pNode);
	decumaAssert(pNode->nCompleteEdgesLists == 1);

	aslFree(pNode->pCompleteEdges);

	pNode->nCompleteEdgesLists--;
	pNode->nCompleteEdges=0;
}
