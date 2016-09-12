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


/*$Header: /data/cvsroot/seattle/t9write/alphabetic/core/src/databaseKEY.c,v 1.5 2011/02/14 11:41:14 jianchun_meng Exp $*/
#include "databaseKEY.h"
#include "decumaMemory.h"
#include "decumaAssert.h"
#include "database.h"
#include "decumaMath.h"

#include "globalDefs.h"

#ifdef _WIN32
#ifndef MATLAB_MEX_FILE
#include <stdio.h>
#include <stdlib.h>
#endif
#endif

void kidInitToFirstKeyInStaticDB(STATIC_DB_HEADER_PTR pStaticDB, KID * pKid)
{
	decumaAssert( pKid );
	myMemSet(pKid, 0, sizeof(KID));
	pKid->pKeyDB = (KEY_DB_HEADER_PTR) staticDBGetKeyDB(pStaticDB);
	pKid->pPropDB = (PROPERTY_DB_HEADER_PTR) staticDBGetPropDB(pStaticDB);
	pKid->pCatTable = (CATEGORY_TABLE_PTR) keyDBGetStaticCategoryTable(pKid->pKeyDB);

	/* Set base key. */
	pKid->baseKeyIndex = 0;
	pKid->noBaseArcs = 1;
	kidSetBaseKeyPointer(pKid);
}

void kidSetBaseKeyPointer(KID * pKid)
{
	KEYLIST_PTR pKeyList;
	KEY_PTR pKey ;

	decumaAssert( pKid->pKeyDB );

	pKeyList = (KEYLIST_PTR) ((DECUMA_DB_ADDRESS) pKid->pKeyDB + 
		pKid->pKeyDB->keyListsOffset);
	pKeyList += pKid->noBaseArcs-1;

	decumaAssert(pKid->baseKeyIndex >= 0);
	decumaAssert(pKid->baseKeyIndex < pKeyList->nKeys);

	pKey = (KEY_PTR) ((DECUMA_DB_ADDRESS) pKid->pKeyDB + pKeyList->keysOffset);
	pKid->pKey = pKey + pKid->baseKeyIndex;

	decumaAssert(KEY_VALID_DUMMY_FIELDS(pKid->pKey));
}	

void kidSetDiacKeyPointer(KID * pKid)
{
	KEY_DIACRITIC_LIST_PTR pList;
	KEY_DIACRITIC_PTR pDiacKey ;

	decumaAssert( pKid->pPropDB );

	pList = (KEY_DIACRITIC_LIST_PTR) ((DECUMA_DB_ADDRESS) pKid->pPropDB + 
		pKid->pPropDB->diacListsOffset);
	pList += pKid->noDiacArcs-1;

	decumaAssert(pKid->diacKeyIndex >= 0);
	decumaAssert(pKid->diacKeyIndex < pList->nKeys);

	pDiacKey = (KEY_DIACRITIC_PTR) ((DECUMA_DB_ADDRESS) pKid->pPropDB + pList->diacKeysOffset);
	pKid->pDiacKey = pDiacKey+pKid->diacKeyIndex;

	decumaAssert(KEY_DIACRITIC_VALID_DUMMY_FIELDS(pKid->pDiacKey));
}
 
DECUMA_UNICODE_DB_PTR kidGetSymbol( const KID * pKid )
{
	decumaAssert(databaseValidKID(*pKid));

	if ( kidHasDiac( pKid ) )
	{
		DIAC_SYMBOL_INFO_PTR pDiacSymbolInfo = kidGetDiacSymbolInfo(pKid);

		decumaAssert(((DECUMA_UNICODE_DB_PTR) 
			((DECUMA_DB_ADDRESS) pKid->pKeyDB + pDiacSymbolInfo->diacSymbolOffset))[0] &&
			((DECUMA_UNICODE_DB_PTR) 
			((DECUMA_DB_ADDRESS) pKid->pKeyDB + pDiacSymbolInfo->diacSymbolOffset))[1] == 0); /* Exactly one char long */

		return (DECUMA_UNICODE_DB_PTR) ((DECUMA_DB_ADDRESS) pKid->pKeyDB + pDiacSymbolInfo->diacSymbolOffset);

	}
	else
	{
		return (DECUMA_UNICODE_DB_PTR) ((DECUMA_DB_ADDRESS) pKid->pKeyDB + kidGetSymbolInfo(pKid)->symbolOffset);
	}
}

DIAC_SYMBOL_INFO_PTR kidGetDiacSymbolInfo( const KID * pKid ) 
{
	decumaAssert(databaseValidKID(*pKid));
	
	if ( kidHasDiac( pKid ) )
	{
		SYMBOL_INFORMATION_PTR pBaseSymbolInfo = kidGetSymbolInfo(pKid);

		DECUMA_UINT8 diacSymbolNr = kidGetDiacKEY(pKid)->diacSymbolNr;
		DECUMA_UINT8 tableIndex = pBaseSymbolInfo->combSymbIdx;

		DECUMA_INT8_DB_PTR pCombSymbolIdx = (DECUMA_INT8_DB_PTR) 
			((DECUMA_DB_ADDRESS) pKid->pPropDB + pKid->pPropDB->combTableOffset);

		DIAC_SYMBOL_INFO_PTR pDiacSymbolInfo = (DIAC_SYMBOL_INFO_PTR)
			((DECUMA_DB_ADDRESS) pKid->pPropDB + pKid->pPropDB->diacSymbolTableOffset);

		decumaAssert( diacSymbolNr < propDBGetNoDiacTypes(pKid->pPropDB) );

		pCombSymbolIdx += tableIndex * propDBGetNoDiacTypes(pKid->pPropDB) + diacSymbolNr;
		decumaAssert(*pCombSymbolIdx >= 0);

		pDiacSymbolInfo += *pCombSymbolIdx;

		decumaAssert(DIAC_SYMBOL_INFO_VALID_DUMMY_FIELDS(pDiacSymbolInfo));

		return pDiacSymbolInfo;
	}
	else 
	{
		return NULL;
	}
} /* kidGetDiacSymbolInfo() */

DECUMA_INT32 kidGetInterleavedNorm( const KID * pKid)
{
	if (pKid->noDiacArcs == 0)
		return kidGetBaseKEY(pKid)->interleavedNorm;
	else if (pKid->noBaseArcs == 0)
		return kidGetDiacKEY(pKid)->interleavedNorm;
	else
	{
		KEY_PTR pKey = kidGetBaseKEY(pKid);
		KEY_DIACRITIC_PTR pDiacKey = kidGetDiacKEY(pKid);
		DECUMA_INT32 sum, temp;

		sum = pKey->interleavedQSum + pDiacKey->interleavedQSum;
		
		/* Add the Y-offset for the diac KEY */
		sum += pKid->diacKeyXOffset * ( 2 * (DECUMA_INT32)pDiacKey->interleavedSumX + (DECUMA_INT32)pKid->diacKeyXOffset * pKid->noDiacArcs * 
			NUMBER_OF_POINTS_IN_INTERLEAVED_ARC);
		sum += pKid->diacKeyYOffset * ( 2 * (DECUMA_INT32)pDiacKey->interleavedSumY + (DECUMA_INT32)pKid->diacKeyYOffset * pKid->noDiacArcs * 
			NUMBER_OF_POINTS_IN_INTERLEAVED_ARC);
		
		sum *= (DECUMA_INT32)(pKid->noBaseArcs+ pKid->noDiacArcs)*NUMBER_OF_POINTS_IN_INTERLEAVED_ARC; /*mass */

		temp = pKey->interleavedSumX + (DECUMA_INT32)pDiacKey->interleavedSumX +
			(DECUMA_INT32)pKid->diacKeyXOffset * pKid->noDiacArcs * NUMBER_OF_POINTS_IN_INTERLEAVED_ARC;
		sum -= temp * temp;
			
		temp = pKey->interleavedSumY + (DECUMA_INT32)pDiacKey->interleavedSumY + 
			(DECUMA_INT32)pKid->diacKeyYOffset * pKid->noDiacArcs * NUMBER_OF_POINTS_IN_INTERLEAVED_ARC;
		sum -= temp * temp;

		return sum;
	}
}

DECUMA_INT8_DB_PTR kidGetArcPointer( const KID * pKid, int arcNr ) 
{
	if (!kidHasDiac(pKid) || arcNr < pKid->noBaseArcs) 
	{
		DECUMA_INT8_DB_PTR pPoints;
		CURVELIST_PTR pCurveList = (CURVELIST_PTR) ((DECUMA_DB_ADDRESS)
			pKid->pKeyDB + pKid->pKeyDB->curveListsOffset);

		pCurveList += pKid->noBaseArcs-1;

		decumaAssert(CURVELIST_VALID_DUMMY_FIELDS(pCurveList));

		pPoints = (DECUMA_INT8_DB_PTR) ((DECUMA_DB_ADDRESS) pKid->pKeyDB + pCurveList->curvesOffset);

		decumaAssert(kidGetBaseKEY(pKid)->curveIdx < pCurveList->nCurves);

		pPoints += kidGetBaseKEY(pKid)->curveIdx * pKid->noBaseArcs * NUMBER_OF_POINTS_IN_ARC*2;
		pPoints += (arcNr)*NUMBER_OF_POINTS_IN_ARC*2;

		return pPoints;
	}
	else
	{
		DECUMA_INT8_DB_PTR pPoints;

		CURVELIST_PTR pCurveList = (CURVELIST_PTR) ((DECUMA_DB_ADDRESS) pKid->pPropDB + 
			pKid->pPropDB->diacCurveListsOffset);

		pCurveList += pKid->noDiacArcs-1;

		pPoints = (DECUMA_INT8_DB_PTR) ((DECUMA_DB_ADDRESS) pKid->pPropDB + pCurveList->curvesOffset);

		decumaAssert(kidGetDiacKEY(pKid)->diacCurveIndex < pCurveList->nCurves);

		pPoints += kidGetDiacKEY(pKid)->diacCurveIndex * pKid->noDiacArcs * NUMBER_OF_POINTS_IN_ARC*2;
		pPoints += (arcNr-pKid->noBaseArcs)*NUMBER_OF_POINTS_IN_ARC*2;
		
		return pPoints;
	}
}


void kidGetMinMaxX(const KID * pKid, DECUMA_INT8 * pnMinX, DECUMA_INT8 * pnMaxX)
{
	/*Returns the minimum and maximum Y-coordinates of the key. The diacritic */
	/*arcs are excluded */

	int k,t;

	*pnMaxX = MIN_KEY_NUMBER;
	*pnMinX = MAX_KEY_NUMBER;
	
	for ( k = 0; k < pKid->noBaseArcs; k++) 
	/* Only the base arcs (not the diacritic arcs) are used */
	/* to calculate the size of the key and curve. */
	{
		/* Note: It is assumed that the first "noBaseArcs" arcs in the */
		/* key curve are used to the base symbol. */
		DECUMA_INT8_DB_PTR keyArc = kidGetArcPointer(pKid,k);

		decumaAssert( keyArc );
		for (t = 0; t < keyarcGetNoPoints(keyArc); t++)
		{
			const DECUMA_INT8 keyX = keyarcGetX(keyArc,t);/*Y-coordinates. */
			if ( keyX > *pnMaxX )
				*pnMaxX = keyX;
			if ( keyX < *pnMinX )
				*pnMinX = keyX;
		}
	}
}

void kidGetMinMaxY(const KID * pKid, DECUMA_INT8 * pnMinY, DECUMA_INT8 * pnMaxY)
{
	/*Returns the minimum and maximum Y-coordinates of the key. The diacritic */
	/*arcs are excluded */

	int k,t;

	*pnMaxY = MIN_KEY_NUMBER;
	*pnMinY = MAX_KEY_NUMBER;
	
	for ( k = 0; k < pKid->noBaseArcs; k++) 
	/* Only the base arcs (not the diacritic arcs) are used */
	/* to calculate the size of the key and curve. */
	{
		/* Note: It is assumed that the first "noBaseArcs" arcs in the */
		/* key curve are used to the base symbol. */
		DECUMA_INT8_DB_PTR keyArc = kidGetArcPointer(pKid,k);

		decumaAssert( keyArc );
		for (t = 0; t < keyarcGetNoPoints(keyArc); t++)
		{
			const DECUMA_INT8 keyY = keyarcGetY(keyArc,t);/*Y-coordinates. */
			if ( keyY > *pnMaxY )
				*pnMaxY = keyY;
			if ( keyY < *pnMinY )
				*pnMinY = keyY;
		}
	}
}

DECUMA_INT32 kidGetQSum( const KID * pKid)
{
	if ( pKid->noDiacArcs == 0 )
		return kidGetBaseKEY(pKid)->qSum;
	else
	{
		KEY_PTR pKey = kidGetBaseKEY(pKid);
		KEY_DIACRITIC_PTR pDiacKey = kidGetDiacKEY(pKid);
		DECUMA_INT32 sum;

		sum = pKey->qSum + pDiacKey->qSum;

		sum += pKid->diacKeyXOffset * ( 2* (DECUMA_INT32)pDiacKey->sumX + (DECUMA_INT32)pKid->diacKeyXOffset * pKid->noDiacArcs * NUMBER_OF_POINTS_IN_ARC);
		sum += pKid->diacKeyYOffset * ( 2* (DECUMA_INT32)pDiacKey->sumY + (DECUMA_INT32)pKid->diacKeyYOffset * pKid->noDiacArcs * NUMBER_OF_POINTS_IN_ARC);
		return sum;
	}
}

ARC_ORDER_SET_PTR kidGetArcOrderSetPtr( const KID * pKid )
{
	DECUMA_UINT8 arcOrderSetNr;
	ARC_ORDER_SET_PTR pArcOrderSet;

	decumaAssert( pKid );

	pArcOrderSet = (ARC_ORDER_SET_PTR) 
		((DECUMA_DB_ADDRESS) pKid->pKeyDB + pKid->pPropDB->allAOSetsOffset);

	decumaAssert(ARC_ORDER_SET_VALID_DUMMY_FIELDS(pArcOrderSet));
	
	if ( !kidHasDiac(pKid) ) {
		SYMBOL_TYPE_INFO_PTR pInfo = kidGetTypeInfo(pKid);
		arcOrderSetNr = pInfo->arcOrderSetIdx;
	}
	else {
		arcOrderSetNr = kidGetDiacKEY(pKid)->arcOrderSetIdx;
	}

	return pArcOrderSet + arcOrderSetNr;
}


#ifdef DEBUG_DUMP_SCR_DATA
void keyDump(char *filename, const KID kid, int noArcs)
{
    int i, k;
	DECUMA_INT8_DB_PTR keyArc;

	FILE *fp;
	if ( filename )
		fp = fopen(filename, "w+");
	else
		fp = stdout;

	/*fprintf(fp,"%d",p->x[0].nNumbers); */
	for(k = 0; k < noArcs; k++)
	{
		keyArc = kidGetArcPointer( &kid,k );
		fprintf(fp, "%d ", NUMBER_OF_POINTS_IN_ARC); 
		for(i = 0; i < NUMBER_OF_POINTS_IN_ARC ; i++) {
			fprintf(fp,"%d %d ", 
				(int)( 100 + keyarcGetX(keyArc,i) ), 
				(int)( 200 + keyarcGetY(keyArc,i) + keyGetArcOffsetY( &kid, k ) ));
			/* The constants above are just there to get positive coordinates */
			/* (Sabed only supports positive coordinates) */
		}
		fprintf(fp, "\n");
	} 
	
		if(fp != stdout)
			fclose(fp);
} 
#endif /*DEBUG_DUMP_SCR_DATA */

UNCERTAINTY keySizeUncertainty( const KID * pKid )
{

	SYMBOL_INFORMATION_PTR pSymInfo = kidGetSymbolInfo(pKid);

	decumaAssert(pKid);
	decumaAssert(pSymInfo);

	return pSymInfo->unknownProperty & SCALE_BITS_IN_UNKNOWN_PROPERTY;
}

UNCERTAINTY keyPositionUncertainty( const KID * pKid )
{

	SYMBOL_INFORMATION_PTR pSymInfo = kidGetSymbolInfo(pKid);

	decumaAssert(pKid);
	decumaAssert(pSymInfo);

	return pSymInfo->unknownProperty & POSITION_BITS_IN_UNKNOWN_PROPERTY;
}

UNCERTAINTY keyShapeUncertainty( const KID * pKid )
{
	DECUMA_UNICODE_DB_PTR pSymb = kidGetSymbol(pKid);

	/* TODO replace this hack with a proper implementation */
	if (pSymb[0] == '.' && pSymb[1] == 0) return db_veryUncertain;
	if (pSymb[0] == ',' && pSymb[1] == 0) return db_quiteUncertain;
	if (pSymb[0] == '\'' && pSymb[1] == 0) return db_quiteUncertain;

	return db_certain;
}

DECUMA_UINT8 keyGetArcRotationMask( const KID * pKid )
{
	decumaAssert(pKid->noBaseArcs > 0 || pKid->noDiacArcs > 0);

	if (pKid->noBaseArcs == 0)
		return kidGetDiacKEY(pKid)->arcRotationMask;
	else
	{
		DECUMA_UINT8 mask = kidGetTypeInfo(pKid)->arcRotationMask;

		if (pKid->noDiacArcs)
		{
			KEY_DIACRITIC_PTR pDiacKey = kidGetDiacKEY(pKid);
			DECUMA_UINT8 diacMask = pDiacKey->arcRotationMask;

			if (pKid->noDiacArcs == 1 && propDBGetSymmetry(pKid->pPropDB, pDiacKey->symmetryIdx) == 0)
			{
				diacMask = 1;
			}

			if (pKid->noBaseArcs == 1 && propDBGetSymmetry(pKid->pPropDB, kidGetTypeInfo(pKid)->symmetryIdx) == 0)
			{
				mask = 1;
			}
			mask |= diacMask << pKid->noBaseArcs;
		}

		return mask;
	}
}

#ifdef _DEBUG
const DECUMA_UINT8 kidGetArcTimelineDiffMask( const KID * pKid )
{
	decumaAssert(pKid);
	return kidGetTypeInfo(pKid)->arcTimelineDiffMask;
}
#endif
