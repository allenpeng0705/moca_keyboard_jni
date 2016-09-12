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

  This file implements aslArc.h. It uses dynamic memory allocation.

  TODO : This file uses sqrt() at a few places, specifically 
         sqrt(dx*dx + dy*dy) -- could we use isqrt() and inorm() and 
		 still retain recognition performance?

\******************************************************************/

#define ASL_ARC_C

#include "aslConfig.h"

#include "aslArc.h"

#include "decumaBasicTypes.h"
#include "decumaBasicTypesMinMax.h"
#include "decumaAssert.h"
#include "decumaMemory.h"
#include "decumaCurve.h"
#include "aslArcData.h"
#include "aslArcSession.h"
#include "aslTools.h"

#include "decumaCommon.h"


/* */
/* Private function declarations */
/* */



static void setMinMaxStartEndPts(ASL_ARC* pAslArc);



/* */
/* Public function definitions */
/* */


#if 0
unsigned long aslArcGetSize(void)
{
	return sizeof(ASL_ARC);
}
#endif

ASL_ARC* aslArcCreate(const ASL_ARC_SESSION* pOwner,
					  const DECUMA_ARC* pDecumaArc,
					  const DECUMA_MEM_FUNCTIONS* pMemFunctions)
{
	ASL_ARC* pAslArc;

	pAslArc = aslCalloc(sizeof(pAslArc[0]));

	if (!pAslArc) return NULL;

	pAslArc->pOwner = pOwner;
	pAslArc->pMemFunctions = pMemFunctions;

	pAslArc->arc.nPoints = pDecumaArc->nPoints;
	pAslArc->arc.pPoints = aslCalloc(pAslArc->arc.nPoints * sizeof(pAslArc->arc.pPoints[0]));
	
	if (!pAslArc->arc.pPoints)
	{
		aslFree(pAslArc);
		return NULL;
	}

	decumaMemcpy(pAslArc->arc.pPoints, pDecumaArc->pPoints, pAslArc->arc.nPoints * sizeof(pAslArc->arc.pPoints[0]));

	/* Precalculate arc properties */
	setMinMaxStartEndPts(pAslArc);

	return pAslArc;
}

void aslArcDestroy(ASL_ARC* pAslArc)
{
	const DECUMA_MEM_FUNCTIONS* pMemFunctions = pAslArc->pMemFunctions;
	decumaAssert(pAslArc);
	decumaAssert(pAslArc->arc.pPoints);

	aslFree(pAslArc->arc.pPoints);
	aslFree(pAslArc);
}

void aslArcStartNewSymbol(ASL_ARC* pAslArc)
{
	decumaAssert(pAslArc);

	pAslArc->bStartNewSymbol = 1;
}

const ASL_ARC_SESSION* aslArcGetOwner(const ASL_ARC* pAslArc)
{
	decumaAssert(pAslArc);

	return pAslArc->pOwner;
}

const DECUMA_ARC* aslArcGetDecumaArc(const ASL_ARC* pAslArc)
{
	decumaAssert(pAslArc);

	return &pAslArc->arc;
}

int aslArcGetIndex(const ASL_ARC* pAslArc)
{
	const ASL_ARC_SESSION* pOwner = aslArcGetOwner(pAslArc);
	int nArcs = aslArcSessionGetArcCount(pOwner);

	int i;

	for (i = 0; i < nArcs; i++)
	{
		if (pAslArc == aslArcSessionGetArc(pOwner,i)) return i;
	}

	decumaAssert(0);

	return i;
}

DECUMA_COORD aslArcGetXMin(const ASL_ARC* pAslArc)
{
	decumaAssert(pAslArc);

	return pAslArc->nXMin;
}

DECUMA_COORD aslArcGetXMax(const ASL_ARC* pAslArc)
{
	decumaAssert(pAslArc);
	
	return pAslArc->nXMax;
}

DECUMA_COORD aslArcGetYMin(const ASL_ARC* pAslArc)
{
	decumaAssert(pAslArc);
	
	return pAslArc->nYMin;
}

DECUMA_COORD aslArcGetYMax(const ASL_ARC* pAslArc)
{
	decumaAssert(pAslArc);
	
	return pAslArc->nYMax;
}

const DECUMA_POINT* aslArcGetStartPt(const ASL_ARC* pAslArc)
{
	decumaAssert(pAslArc);

	return &pAslArc->startPt;
}

const DECUMA_POINT* aslArcGetEndPt(const ASL_ARC* pAslArc)
{
	decumaAssert(pAslArc);

	return &pAslArc->endPt;
}

int aslArcIsStartingNewSymbol(const ASL_ARC* pAslArc)
{
	decumaAssert(pAslArc);

	return pAslArc->bStartNewSymbol;
}



/* */
/* Private function definitions */
/* */



static void setMinMaxStartEndPts(ASL_ARC* pAslArc)
{
	DECUMA_COORD nXMin, nXMax, nYMin, nYMax;
	int i;

	nXMin = MAX_DECUMA_COORD;
	nXMax = MIN_DECUMA_COORD;
	nYMin = MAX_DECUMA_COORD;
	nYMax = MIN_DECUMA_COORD;

	for (i = 0; i < pAslArc->arc.nPoints; i++)
	{
		if (pAslArc->arc.pPoints[i].x < nXMin) nXMin = pAslArc->arc.pPoints[i].x;
		if (pAslArc->arc.pPoints[i].x > nXMax) nXMax = pAslArc->arc.pPoints[i].x;
		if (pAslArc->arc.pPoints[i].y < nYMin) nYMin = pAslArc->arc.pPoints[i].y;
		if (pAslArc->arc.pPoints[i].y > nYMax) nYMax = pAslArc->arc.pPoints[i].y;
	}

	pAslArc->nXMin = nXMin;
	pAslArc->nXMax = nXMax;
	pAslArc->nYMin = nYMin;
	pAslArc->nYMax = nYMax;

	decumaAssert(pAslArc->arc.nPoints > 0);

	pAslArc->startPt = pAslArc->arc.pPoints[0];
	pAslArc->endPt = pAslArc->arc.pPoints[pAslArc->arc.nPoints-1];
}
