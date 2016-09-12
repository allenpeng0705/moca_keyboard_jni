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

        File: udmKey.c
     Package: This file is part of the package udm
   Copyright: Decuma AB (2001)
     $Author: jianchun_meng $
   $Revision: 1.5 $
       $Date: 2011/02/14 11:41:14 $

\******************************************************************/

#include "decumaDataTypes.h"
#include "udm.h"
#include "udmKey.h"
#include "decumaMemory.h"
#include "decumaAssert.h"
#include "decumaMath.h"
#include "globalDefs.h"


#if 0

#include "udmMalloc.h"

void keyInit(struct _KEY * pKey, int nArcs)
{
	int siz = nArcs*NUMBER_OF_POINTS_IN_ARC * sizeof(pKey->keyCurve[0]) * 2; /* *2 since x and y are in same vector */

	DECUMA_INT8 * points = (DECUMA_INT8*) udmMalloc(siz);
	struct _SYMBOL_TYPE_INFO * pTypeInfo = (struct _SYMBOL_TYPE_INFO*) udmMalloc( sizeof(struct _SYMBOL_TYPE_INFO) );
	struct _SYMBOL_INFORMATION * pSymbolInfo = (struct _SYMBOL_INFORMATION*) udmMalloc( sizeof(struct _SYMBOL_INFORMATION) );
	struct _CATEGORIES * pCategories = (struct _CATEGORIES*) udmMalloc( sizeof(struct _CATEGORIES) );

	myMemSet((void*)pTypeInfo,0,sizeof(SYMBOL_TYPE_INFO));
	myMemSet((void*)pSymbolInfo,0,sizeof(SYMBOL_INFORMATION));
	myMemSet((void*)pCategories,0,sizeof(CATEGORIES));
	myMemSet((void*)points,0,siz);
	myMemSet((void*)pKey,0,sizeof(*pKey));

	(KEY_CURVE) (pKey->keyCurve) = points;
	pTypeInfo->pSymbolInfo = pSymbolInfo;
	pTypeInfo->pCategories = pCategories;
	pKey->pTypeInfo = pTypeInfo;
}

void keyRelease(struct _KEY * pKey)
{
	udmFree((void*)pKey->keyCurve); /* Since we only used a single call to udmMalloc in the keyInit, we */
								/* only need a single call to udmFree. */
	udmFree((void*)pKey->pTypeInfo->pSymbolInfo->symbol);
	udmFree((void*)pKey->pTypeInfo->pSymbolInfo);
	udmFree((void*)pKey->pTypeInfo->pCategories);
	udmFree((void*)pKey->pTypeInfo);
} /* KeyRelease() */

void keylistRelease(struct _KEYLIST * pToRelease)
{
	int i;
	if(pToRelease == 0)
		return;

	for(i= 0 ; i < pToRelease->noKeys ; i++)
	{
		keyRelease((struct _KEY *)&pToRelease->pKeys[i]);
	}

	udmFree((struct _KEYLIST*)pToRelease->pKeys);
	pToRelease->noKeys = 0;
} /* keyListRelease() */

#endif /* 0 */

/*
	Precalculates the ligatures, mean_x, mean_y, norm_1 and norm_2
*/
void keyPreCalculate(struct _KEY * pKey, DECUMA_INT8_DB_PTR pKeyCurve, int noArcs)
{

	/*This is only intended for measureId == 0. */

	/*The pointnumbers for interleaved calculations will be */
	/*0,4,8,13,18,23,27,31 */
	/*This is done to get symmetry so that comparisons can be made */
	/*with arcs traversed backwards */

	int a,p;

	register DECUMA_INT8 keyX;
	register DECUMA_INT8 keyY;

	DECUMA_INT8_DB_PTR keyArc;

	int massInterleaved = 0;
	DECUMA_INT32 keyQSumInterleaved = 0;
	DECUMA_INT32 keySumXInterleaved = 0;
	DECUMA_INT32 keySumYInterleaved = 0;
	int xMax = -MAX_DECUMA_INT16;

	int mass = 0;
	DECUMA_INT32 keyXqSum = 0;
	DECUMA_INT32 keyYqSum = 0;
	DECUMA_INT32 keyDXqSum = 0;
	DECUMA_INT32 keyDYqSum = 0;
	DECUMA_INT32 keySumX= 0;
	DECUMA_INT32 keySumY= 0;

	for(a = 0 ; a < noArcs ; a++)
	{
		keyArc = pKeyCurve + a*2*NUMBER_OF_POINTS_IN_ARC;

		for(p = 0 ; p < NUMBER_OF_POINTS_IN_ARC ; p++ )
		{
			int dx, dy;
			keyX = keyarcGetX(keyArc,p);
			keyY = keyarcGetY(keyArc,p);

			if (keyX > xMax)
				xMax = keyX;

			/* This section calculates the normal values */
			if(p == NUMBER_OF_POINTS_IN_ARC - 1) /*end of arc. */
			{
				dx = keyX - keyarcGetX(keyArc,p-1);
				dy = keyY - keyarcGetY(keyArc,p-1);
			}
			else
			{
				dx = keyarcGetX(keyArc,p+1) - keyX;
				dy = keyarcGetY(keyArc,p+1) - keyY;
			}

			mass++;
			keyXqSum += keyX * keyX;
			keyYqSum += keyY * keyY;
			keyDXqSum += dx * dx;
 			keyDYqSum += dy * dy;

			keySumX+= keyX;
			keySumY+= keyY;


			/* This section calculates the interleaved values */
			if(p==0 || p==4 || p==8 || p==13 || p==18 || p==23 || p== 27 || p==31)
			{
				massInterleaved++;
				keyQSumInterleaved += (keyX * keyX + keyY * keyY);
				keySumXInterleaved += keyX;
				keySumYInterleaved += keyY;
			}
		}
	}

	/* Assert the interleaved values */
	decumaAssert( keyQSumInterleaved <= keyQSumInterleaved * massInterleaved );
	decumaAssert( keySumXInterleaved * keySumXInterleaved >= keySumXInterleaved);
	decumaAssert( keySumYInterleaved * keySumYInterleaved >= keySumYInterleaved);

	/* Assert the normal values. */
	decumaAssert( keyXqSum <= keyXqSum * mass );
	decumaAssert( keyYqSum <= keyYqSum * mass );
	decumaAssert( keySumX* keySumX >= keySumX );
	decumaAssert( keySumY* keySumY >= keySumY );

	pKey->interleavedQSum = keyQSumInterleaved;

	pKey->dqSum = keyDXqSum + keyDYqSum;
	pKey->interleavedNorm = keyQSumInterleaved * massInterleaved
		- keySumXInterleaved * keySumXInterleaved - keySumYInterleaved * keySumYInterleaved;

	decumaAssert( keySumXInterleaved <= MAX_DECUMA_INT16 && keySumXInterleaved >= MIN_DECUMA_INT16);
	pKey->interleavedSumX = (DECUMA_INT16) keySumXInterleaved;

	decumaAssert( keySumYInterleaved <= MAX_DECUMA_INT16 && keySumYInterleaved >= MIN_DECUMA_INT16);
	pKey->interleavedSumY = (DECUMA_INT16) keySumYInterleaved;

	decumaAssert( keySumX <= MAX_DECUMA_INT16 && keySumX >= MIN_DECUMA_INT16);
	pKey->sumX = (DECUMA_INT16) keySumX;

	decumaAssert( keySumY <= MAX_DECUMA_INT16 && keySumY >= MIN_DECUMA_INT16);
	pKey->sumY = (DECUMA_INT16) keySumY;

	pKey->qSum = keyXqSum + keyYqSum;
	pKey->xWidth = (DECUMA_UINT8) xMax;

} /* keyPreCalculate() */
