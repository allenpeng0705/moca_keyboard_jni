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

  This file provides macro interface to aslSG objects.

\******************************************************************/

#ifndef ASL_SG_MACROS_H
#define ASL_SG_MACROS_H

#if !defined(_DEBUG) 
/*
   In debug mode only aslSG module should be able to
   access the data directly. In release mode we allow external
   direct access by macros to improve speed. Only being able to
   access the data through access functions in debug mode makes
   sure the abstraction is kept intact (preventing spaghetti code
   evolving). The type control is enabled in both modes.
*/

#define aslSGGetNodeCount                         aslSGGetNodeCountMacro
#define aslSGGetNode                              aslSGGetNodeMacro
#define aslSGGetDiacCount                         aslSGGetDiacCountMacro
#define aslSGGetDiac                              aslSGGetDiacMacro

#define aslSGGetEdgeCount                         aslSGGetEdgeCountMacro
#define aslSGGetEdge                              aslSGGetEdgeMacro
#define aslSGGetBestEdgeIndex                     aslSGGetBestEdgeIndexMacro
#define aslSGNodeGetDiac                          aslSGNodeGetDiacMacro

#define aslSGDiacGetEdgeCount                     aslSGDiacGetEdgeCountMacro
#define aslSGDiacGetEdge                          aslSGDiacGetEdgeMacro
#define aslSGDiacGetEdgeAnchorCount               aslSGDiacGetEdgeAnchorCountMacro
#define aslSGDiacGetEdgeAnchor                    aslSGDiacGetEdgeAnchorMacro
#define aslSGDiacGetStartNode                     aslSGDiacGetStartNodeMacro
#define aslSGDiacGetEndNode                       aslSGDiacGetEndNodeMacro

#define aslSGDiacEdgeAnchorGetNode                aslSGDiacEdgeAnchorGetNodeMacro
#define aslSGDiacEdgeAnchorGetDiacriticConnector  aslSGDiacEdgeAnchorGetDiacriticConnectorMacro
#define aslSGDiacEdgeAnchorGetEdgeIndex           aslSGDiacEdgeAnchorGetEdgeIndexMacro
#define aslSGDiacEdgeAnchorGetDiacIndex           aslSGDiacEdgeAnchorGetDiacIndexMacro
#define aslSGDiacEdgeAnchorGetConnectionDistance  aslSGDiacEdgeAnchorGetConnectionDistanceMacro

#define aslSGGetSymbol                            aslSGGetSymbolMacro
#define aslSGGetStartNode                         aslSGGetStartNodeMacro
#define aslSGGetNOFPrototypeSegments              aslSGGetNOFPrototypeSegmentsMacro
#define aslSGGetPrototype                         aslSGGetPrototypeMacro
#define aslSGGetStrokeOrder                       aslSGGetStrokeOrderMacro
#define aslSGGetVariation                         aslSGGetVariationMacro
#define aslSGGetVariationSequence                 aslSGGetVariationSequenceMacro
#define aslSGGetMasksPtr                          aslSGGetMasksPtrMacro
#define aslSGGetBestConnectorEdgeIndex            aslSGGetBestConnectorEdgeIndexMacro
#define aslSGGetBestConnectorNode                 aslSGGetBestConnectorNodeMacro
#define aslSGGetBestPathDistance                  aslSGGetBestPathDistanceMacro
#define aslSGGetNoiseDistance                     aslSGGetNoiseDistanceMacro
#define aslSGGetEndPoint                          aslSGGetEndPointMacro
#define aslSGGetStartPoint                        aslSGGetStartPointMacro
#define aslSGGetMaxX                              aslSGGetMaxXMacro
#define aslSGGetMinX                              aslSGGetMinXMacro
#define aslSGGetBeforeAnchorPoint                 aslSGGetBeforeAnchorPointMacro
#define aslSGGetAfterAnchorPoint                  aslSGGetAfterAnchorPointMacro
#define aslSGGetBestConnectorAlignment            aslSGGetBestConnectorAlignmentMacro
#define aslSGEdgeIsPenliftLigature                aslSGEdgeIsPenliftLigatureMacro
#define aslSGEdgeIsArabicLetterBase               aslSGEdgeIsArabicLetterBaseMacro
#define aslSGEdgeIsIsolatedArabicLetterBase       aslSGEdgeIsIsolatedArabicLetterBaseMacro

#endif /* !defined(_DEBUG) */

#if !defined(_DEBUG) || defined(ASL_SG_C)
/*
   Although macros are not needed in debug mode they are made
   available to aslSG for debug verification by use in
   corresponding functions.
*/

#include "aslConfig.h"

#include "aslSGData.h"

#define aslSGGetNodeCountMacro(pSG)                                     ((pSG)->nNodes)
#define aslSGGetNodeMacro(pSG, nIndex)                                  ((pSG)->pNodes[(nIndex)])
#define aslSGGetDiacCountMacro(pSG)                                     ((pSG)->nDiacs)
#define aslSGGetDiacMacro(pSG, nIndex)                                  ((pSG)->pDiacs[(nIndex)])
																	 
#define aslSGGetEdgeCountMacro(pNode)                                   ((pNode)->nCompleteEdges)
#define aslSGGetEdgeMacro(pNode, nIndex)                                (&(pNode)->pCompleteEdges[(nIndex)])
#define aslSGGetBestEdgeIndexMacro(pNode)                               (0)
#define aslSGNodeGetDiacMacro(pNode)                                    ((pNode)->pDiac)
																	 
#define aslSGDiacGetEdgeCountMacro(pDiac)                               ((pDiac)->nCompleteEdges)
#define aslSGDiacGetEdgeMacro(pDiac, nIndex)                            (&(pDiac)->pCompleteEdges[(nIndex)])
#define aslSGDiacGetEdgeAnchorCountMacro(pDiac, nIndex)                 ((pDiac)->nEdgeAnchors[(nIndex)])
#define aslSGDiacGetEdgeAnchorMacro(pDiac, nEdgeIndex, nAnchorIndex)    ((pDiac)->pEdgeAnchors[(nEdgeIndex)][(nAnchorIndex)])
#define aslSGDiacGetStartNodeMacro(pDiac)                               ((pDiac)->nStartNode)
#define aslSGDiacGetEndNodeMacro(pDiac)                                 ((pDiac)->nEndNode)
		
#define aslSGDiacEdgeAnchorGetNodeMacro(pDiacEdgeAnchor)                ((pDiacEdgeAnchor)->nNode)
#define aslSGDiacEdgeAnchorGetDiacriticConnectorMacro(pDiacEdgeAnchor)  ((pDiacEdgeAnchor)->nDiacConnIdx)
#define aslSGDiacEdgeAnchorGetEdgeIndexMacro(pDiacEdgeAnchor)           ((pDiacEdgeAnchor)->nDiacEdge)
#define aslSGDiacEdgeAnchorGetDiacIndexMacro(pDiacEdgeAnchor)           ((pDiacEdgeAnchor)->nDiac)
#define aslSGDiacEdgeAnchorGetConnectionDistanceMacro(pDiacEdgeAnchor)  ((pDiacEdgeAnchor)->nConnDist)
		
#define aslSGGetSymbolMacro(pEdge)                                      ((pEdge)->scr_output.outSymbol == alternative ? (pEdge)->alternativeSymbol : (pEdge)->scr_output.symbol)
#define aslSGGetStartNodeMacro(pEdge)                                   ((pEdge)->nStartNode)
#define aslSGGetClassMacro(pEdge)                                       ((pEdge)->nClass)
#define aslSGGetNOFPrototypeSegmentsMacro(pEdge)                        ((pEdge)->nNOFProtSegs)
#define aslSGGetPrototypeMacro(pEdge)                                   ((pEdge)->nPrototype)
#define aslSGGetStrokeOrderMacro(pEdge)                                 ((pEdge)->nStrokeOrder)
#define aslSGGetVariationMacro(pEdge, nIndex)                           ((pEdge)->nVariation[(nIndex)])
#define aslSGGetVariationSequenceMacro(pEdge)							(&((pEdge)->nVariation[0]))
#define aslSGGetMasksPtrMacro(pEdge)                                    ((pEdge)->pMasks)
#define aslSGGetBestConnectorEdgeIndexMacro(pEdge)                      ((pEdge)->nBestConnector)
#define aslSGGetBestConnectorNodeMacro(pEdge)                           ((pEdge)->nBestConnectorNode)
#define aslSGGetBestPathDistanceMacro(pEdge)                            ((pEdge)->nBestPathDist)
#define aslSGGetNoiseDistanceMacro(pEdge)                               ((pEdge)->nNoiseDist)
#define aslSGGetEndPointMacro(pEdge, pPoint)                            (*(pPoint) = (pEdge)->endPt)
#define aslSGGetStartPointMacro(pEdge, pPoint)                          (*(pPoint) = (pEdge)->startPt)
#define aslSGGetMaxXMacro(pEdge)                                        ((pEdge)->nMaxX)
#define aslSGGetMinXMacro(pEdge)                                        ((pEdge)->nMinX)
#define aslSGGetBeforeAnchorPointMacro(pEdge)                           ((pEdge)->firstSegMatchData.nBeforeAnchorPtX)
#define aslSGGetAfterAnchorPointMacro(pEdge)                            ((pEdge)->lastSegMatchData.nAfterAnchorPtX)
#define aslSGGetBestConnectorAlignmentMacro(pEdge)                      ((pEdge)->nBestConnectorAlignment)
#define aslSGEdgeIsPenliftLigatureMacro(pEdge)                          ((pEdge)->bIsPenliftLigature)
#define aslSGEdgeIsArabicLetterBaseMacro(pEdge)                         ((pEdge)->bIsArabicLetterBase)
#define aslSGEdgeIsIsolatedArabicLetterBaseMacro(pEdge)                 ((pEdge)->bIsIsolatedArabicLetterBase)

#endif /* !defined(_DEBUG) || defined(ASL_SG_C) */

#endif /* ASL_SG_MACROS_H */
