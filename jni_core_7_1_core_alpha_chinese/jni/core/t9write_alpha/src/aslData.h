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
  asl session objects.

\******************************************************************/

#ifndef ASL_DATA_H
#define ASL_DATA_H

#if defined(_DEBUG) && !defined(ASL_C)
#error Direct data access in debug mode only allowed from asl.c
#endif

#include "aslConfig.h"

#include "aslArcSession.h"
#include "aslSG.h"
#include "aslRG.h"
#include "decumaTrigramTable.h"

#include "decuma_hwr.h" /*For DECUMA_SESSION_SETTINGS */
#include "decumaDictionary.h" /*For DECUMA_HWR_DICTIONARY */

struct _ASL_SESSION
{
	const DECUMA_SESSION_SETTINGS * pSessionSettings; /*Pointer to the session settings stored in DECUMA_SESSION object	 */
	DECUMA_CHARACTER_SET * pCharSet; /*Own copy of the active charset. Allocated memory */
	                                 /*Can be different than the one pointed to by pSessionSettings when char set extension is used */
	                                 /*Pointers are allocated */
	const DECUMA_MEM_FUNCTIONS * pMemFunctions;

	ASL_ARC_SESSION * pArcSession;

	CATEGORY cat;

	ASL_SG * pSG;
	ASL_RG * pRG;

	int bAddingArcs;

	/*Store these that are calculated from the settings */
	int baselinePos; /* Y-coordinate of baseline */
	int distBaseToHelpLine; /*dist > 0 */

	int bReferenceEstimated; /* baselinePos and distBaseToHelpLine have been set to estimates */
	                         /* based on selected candidate of current add arc session */

	/* Store previous add arc session based baselinePos and distBaseToHelpLine to be able to restore */
	/* a valid value in case new values were set based on selected candidate and then selected */
	/* candidate is deleted */
	int nPrevBaselinePos;
	int nPrevDistBaseToHelpLine;
	int bPrevReferenceEstimated;

	/* Data need by eZiText, should only be allocated when needed (RG life-cycle and getStringType) */
#ifdef NODES_IN_SESSION_MEM

#define MAX_STATIC_DICTIONARIES 2
#define MAX_DICTIONARIES 5

	ZI8DAWGINFO ziDawgInfo[MAX_STATIC_DICTIONARIES]; /* DAWG info when not allocating dynamically */
	ZI8OUTEDGEINFO aOutEdgeInfos[MAX_DICTIONARIES];
	ZI_DICTIONARY_LIST aDictionaries[MAX_DICTIONARIES];

#endif
	
	DECUMA_HWR_DICTIONARY ** pDictionaries; /* Array of pointers */
	int nDictionaries;
	int nMaxDictionaries; /* ...or assume we never realloc pDictionaries and make this a const int? */

	/*These are graphs used for the forced recognition. They will only be allocated when needed. */
	ASL_SG * pForcedSG;
	ASL_RG * pForcedRG;

	DECUMA_INT16 * forcedSegmentation;
	DECUMA_INT16 nForcedSegmentationLen;

	DECUMA_UNICODE * pDictionaryFilterStr;
	DECUMA_INT16 nDictionaryFilterStrLen;

	int bPositionAndScaleInvariant;

	int bNoTypeConnectionRequirement;
	int bStrictTypeConnectionRequirement;

#ifdef EXTRA_VERIFICATION_INFO
	int bDelayedRGDestruction; /*Used to delay destruction of RG until the arcSession ends or a new recognition is started */
#endif

	DECUMA_COORD* pnBaseLineEstimates; /* Allocated copy of the estimates for each result */
	DECUMA_COORD* pnHelpLineEstimates; /* Allocated copy of the estimates for each result */
	int nResults; /* No of results in last recognize call */

	int nLastNotedResult; /* Last noted "correct" result from user candidate selection */

	DECUMA_INT16* pSymbolStrokes; /* To facilitate temp buffer allocation and free */
	DECUMA_INT16* pSymbolChars; /* To facilitate temp buffer allocation and free */

	int bIsCorrupt;

	CATEGORY_TABLE_PTR pCatTable;
};

#endif /*ASL_DATA_H */
