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

  This file provides the interface to aslRG (asl Recognition Graph)
  objects.


  TODO: Document recognition graph structure


\******************************************************************/

#ifndef ASL_RG_H
#define ASL_RG_H

#include "aslConfig.h"

typedef struct _ASL_RG ASL_RG;
typedef struct _ASL_RG_NODE ASL_RG_NODE;
typedef struct _ASL_RG_STRING ASL_RG_STRING;

#if !defined(_DEBUG)
#include "aslRGMacros.h"
#endif

#include "decumaStatus.h"
#include "decumaCategoryType.h"
#include "decumaRuntimeMalloc.h"
#include "aslSG.h"
#include "decumaDictionary.h" /*For DECUMA_HWR_DICTIONARY */

/*#include "aslArcSession.h" */
/*#include "aslPrototypeDB.h" */
/*#include "aslDistanceDB.h" */


/*
    Returns the size of a ASL_RG object.
*/
DECUMA_HWR_PRIVATE int aslRGGetSize(void);

/*
    Initializes pRG. Memory for pRG have been allocated
	by the caller and the size of the memory must be at least that
	returned by aslRGGetSize().

    An already initialized pRG must be destroyed before it can
	be initialized again.
*/
DECUMA_HWR_PRIVATE void aslRGInit(ASL_RG* pRG,
			   const DECUMA_MEM_FUNCTIONS* pMemFunctions,
			   const DECUMA_CHARACTER_SET* pCharacterSet,
			   CATEGORY_TABLE_PTR pCatTable,
			   ASL_SG * pSG,
               const ASL_ARC_SESSION* pArcSession,
			   DECUMA_HWR_DICTIONARY ** pDictionaries,
			   int nDictionaries,
			   BOOST_LEVEL boostLevel,
			   STRING_COMPLETENESS strCompleteness,
			   DECUMA_UNICODE * pStringStart,
			   WRITING_DIRECTION writingDirection,
			   RECOGNITION_MODE recognitionMode,
			   DECUMA_UNICODE * pDictionaryFilterStr,
			   int nDictionaryFilterStrLen,
			   const DECUMA_UNICODE * forcedRecognitionStr,
			   const DECUMA_HWR_RESULT * pForceResult,
			   int nForceResultLen);

/*
	Returns 1 if pRG is corrupt, 0 otherwise.
*/
DECUMA_HWR_PRIVATE int aslRGIsCorrupt(const ASL_RG* pRG);

/*
    Releases all memory allocated for pRG and and destroys pRG.
	pRG must be re-initialized before it can be used again.
*/
DECUMA_HWR_PRIVATE void aslRGDestroy(ASL_RG* pRG);



/*
	Builds up the recognition graph from the segmentation graph
	provided at the initialization.

	In the case of FDE (fuzzy) this constitutes calculating additive 
	connection distances.
*/
DECUMA_HWR_PRIVATE DECUMA_STATUS aslRGConstruct(ASL_RG* pRG, int bUseHorisontalConnDist);


/* Return the number of strings in the last node of the RG */
DECUMA_HWR_PRIVATE int aslRGGetNOFStrings(const ASL_RG* pRG);

/* Returns a string in from the last node of the RG */
/* The parameters are similar as the fileds in DECUMA_HWR_RESULT. */
/* segmentation can be zero. If it is non-zero pnSegmentationLen must also be non-zero. */
/* If the returned *pnSegmentationLen is larger than nMaxSegmentationLen the returned */
/* segmentation is not complete (first part not included) and cannot be used. */
/* If segmentation i non-zero it will be filled with node indeces uniquely specifying */
/* the base symbol segmentation of the returned string. This segmentation can be used */
/* directly for later initialization of SG to force SG to only allow this segmentation. */
DECUMA_HWR_PRIVATE void aslRGGetString(const ASL_RG* pRG,
					DECUMA_UNICODE * pChars,
					DECUMA_UINT16 nMaxChars,
					DECUMA_UINT16 * pnChars, 
					DECUMA_UINT16 * pnResultingChars, 
					int strIdx,
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

/* Returns the total distance (including possible linguistic component) */
/* to a string in the last node of the RG */
DECUMA_HWR_PRIVATE int aslRGGetStringDist(const ASL_RG* pRG, int strIdx);

/* Returns the pure shape distance to a string in the last node of the RG */
DECUMA_HWR_PRIVATE int aslRGGetStringShapeDist(const ASL_RG* pRG, int strIdx);

/* Returns the type of a string in the last node of the RG */
DECUMA_HWR_PRIVATE DECUMA_STRING_TYPE aslRGGetStringType(const ASL_RG* pRG, int strIdx,DECUMA_INT16 * pStartIdx, DECUMA_INT16 * pLen);


#ifdef _DEBUG
/*Returns the start node of pRgString */
DECUMA_HWR_PRIVATE int aslRGStringGetStartNode(const ASL_RG_STRING * pRgString);

/*Returns a pointer to the start string of pRgString (the rg-string that was added an extra edge to get to this string) */
DECUMA_HWR_PRIVATE const ASL_RG_STRING * aslRGStringGetStartString(const ASL_RG_STRING * pRgString);
#endif


/* Char-to-stroke data */

/* Returns the size of a char stroke matrix. Used to get information about  */
/* what stroke is associated with which character in a recognition candidate (=rgString) */
DECUMA_HWR_PRIVATE int aslRGGetCharStrokeMtxSize(DECUMA_UINT16 nMaxChars);

/* Returns a pointer to an array of stroke idxs belonging to a character in */
/* a recognition candidate whose char-to-stroke data has been gathered in pCharStrokesMtx */
DECUMA_HWR_PRIVATE DECUMA_UINT16 * aslRGCharStrokeDataGetStrokeIdxPtr(const void * pCharStrokesMtx,
																	DECUMA_UINT16 nCharIdx);

/* Fills pCharStrokesMtx and pnStrokesArr with data about how the chars are matched */
/* to strokes. */
/* pCharStrokesMtx - Should point to a memory of size aslRGGetCharStrokeMtxSize() */
/*                   Will be set to a matrix with the actual matching. */
/*                   Use aslRGCharStrokeDataGetStrokeIdxPtr() to read. */
/* pnStrokesArr - Should have room for nChars elements */
/*                pnStrokesArr[i] will be set to the number of strokes that are  */
/*                used for char i */
DECUMA_HWR_PRIVATE void aslRGSetCharStrokesData(const ASL_RG* pRG, void * pCharStrokesMtx, DECUMA_UINT16 * pnStrokesArr,
									  int strIdx, DECUMA_UINT16 nMaxChars, DECUMA_UINT16 nChars);

DECUMA_HWR_PRIVATE void aslRGReset(ASL_RG * pRG);

/*Useful functions when debugging */
#ifdef EXTRA_VERIFICATION_INFO

/*Returns the number of nodes in RG */
DECUMA_HWR_PRIVATE int aslRGGetNodeCount(const ASL_RG* pRG);

/*Returns a pointer to a node */
DECUMA_HWR_PRIVATE const ASL_RG_NODE * aslRGGetNode(const ASL_RG* pRG, int nNode);

/*Returns the number of strings in a node */
DECUMA_HWR_PRIVATE int aslRGNodeGetStringCount(const ASL_RG_NODE * pRGNode);

/*Returns the pointer to a string in a node */
DECUMA_HWR_PRIVATE const ASL_RG_STRING * aslRGNodeGetString(const ASL_RG_NODE * pRGNode, int nRgStringIdx);

/*Returns the edge that take us from the second last string  */
/*to the string in the last node in the string path */
DECUMA_HWR_PRIVATE const ASL_SG_EDGE * aslRGStringGetLastEdge(const ASL_RG_STRING * pRgString,
										   const ASL_RG * pRG, int nodeIdx);

							 

/*Returns the index of the string in the previous node of the string path */
DECUMA_HWR_PRIVATE int aslRGStringGetPrevious(const ASL_RG_STRING * pRgString);

/*Returns the index of the edge to the last node in the string path */
DECUMA_HWR_PRIVATE int aslRGStringGetLastEdgeIdx(const ASL_RG_STRING * pRgString);

/*Returns the disance of the string path */
DECUMA_HWR_PRIVATE int aslRGStringGetDist(const ASL_RG_STRING * pRgString);

/*Returns the distance that the last edge added to the rg-string */
DECUMA_HWR_PRIVATE int aslRGStringGetLastDist(const ASL_RG_STRING * pRgString);

/*Returns the connection distance between the last edge added to the rg-string */
/*and the start string */
DECUMA_HWR_PRIVATE int aslRGStringGetConnectionDist(const ASL_RG_STRING * pRgString);

/*Returns the sorting distance of the string */
DECUMA_HWR_PRIVATE int aslRGStringGetSortingDist(const ASL_RG_STRING * pRgString);

/*
	Returns the unicode representing the last edge of the string

		nMaxUnicodes    - The number of unicodes that can be written in pUnicodes including terminating zero

	Return value is the number of written unicodes excluding terminating zero
*/
DECUMA_HWR_PRIVATE int aslRGStringGetLastSymbolUnicodes(const ASL_RG * pRG, const ASL_RG_STRING * pRgString,
												 int nodeIdx, DECUMA_UNICODE * pUnicodes, int nMaxUnicodes);

/* 
	Returns the distance of the rg string divided by the number of segments. This should be in the same
	format as calculated in decuma_hwr.c
*/
DECUMA_HWR_PRIVATE DECUMA_UINT32	aslRGStringGetMeanDistance(const ASL_RG * pRG, const ASL_RG_STRING * pRgString, int nNode);


#endif /*EXTRA_VERIFICATION_INFO */


#endif /*ASL_RG_H */
