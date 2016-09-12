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


/*$Header: /data/cvsroot/seattle/t9write/alphabetic/core/src/scrIterators.c,v 1.5 2011/02/14 11:41:14 jianchun_meng Exp $*/

#include "databaseFormat.h"
#include "scrIterators.h"
#include "database.h"
#include "databaseKEY.h"
#include "decumaAssert.h"

#include "globalDefs.h"

static DECUMA_BOOL kitGetCurrentElement(KIT * pIt);
/*extern const BASEKEY_INDEX_TABLE baseKeysWithPoorShapeTable[]; */

void kitCreate(KIT * k, int nBaseArcs, int nDiacArcs,
			   const SEARCH_SETTINGS * pSS, const scrOUTPUT_LIST * precalculatedBaseKeys)
{
	/*Creates a key iterator.
	precalculatedBaseKeys contains outputs of already made comparisons with nBaseArcs arcs.
	precalculatedBaseKeys.nOutputs = -1 means that this is a invalid List, and the list
	won't be used. Instead all keys with nBaseArcs that can take diacritic arcs
	will be iterated.
	precalculatedBaseKeys.nOutputs = -2 means that this is a invalid List, and the list
	won't be used. Instead all baseKeysWithPoorShape that can take diacritic arcs
	will be iterated.

	If nDiacArcs = 0 all (base)keys with nBaseArcs arcs will be iterated*/

	decumaAssert(k);
	decumaAssert(nBaseArcs<=pSS->pKeyDB->nMaxArcsInCurve);
	decumaAssert(nBaseArcs>0);
	decumaAssert(nDiacArcs<=pSS->pPropDB->nDiacKeyLists);
	decumaAssert(nDiacArcs>=0);

	k->kid.pKey = NULL;
	k->kid.pDiacKey = NULL;
	k->kid.noBaseArcs = nBaseArcs;
	k->kid.noDiacArcs = 0;
	k->kid.diacKeyIndex = 0;
	k->kid.pKeyDB = (KEY_DB_HEADER_PTR ) pSS->pKeyDB;
	k->kid.pPropDB = (PROPERTY_DB_HEADER_PTR ) pSS->pPropDB;
	k->kid.pCatTable = (CATEGORY_TABLE_PTR )pSS->pCatTable;
	k->kid.baseKeyIndex=-1;  /*Should return FIRST key on call to kitGoNext */
	k->kid.diacKeyXOffset = 0;
	k->kid.diacKeyYOffset = 0;

	k->usePrecalculated = nDiacArcs > 0 && precalculatedBaseKeys->nOutputs > -1;
	k->checkBaseKeysWithPoorShape = (nDiacArcs > 0) && (precalculatedBaseKeys->nOutputs == -2);
	k->precalculatedOutputs = precalculatedBaseKeys;
	k->counter = -1;
	k->nDiacArcs = nDiacArcs;

	if (nBaseArcs> pSS->pKeyDB->nKeyLists || nDiacArcs> pSS->pPropDB->nDiacKeyLists)
	{
		k->nStop = 0;
	}
	else
	{
		if (k->usePrecalculated)
			k->nStop = precalculatedBaseKeys->nOutputs;
		/*else if (k->checkBaseKeysWithPoorShape) */
		/*	k->nStop = baseKeysWithPoorShapeTable[nBaseArcs-1].nKeys; */
		else
			k->nStop = keyDBGetNoKeys(pSS->pKeyDB, nBaseArcs);
	}

}

DECUMA_BOOL kitGoNext(KIT * pIt)
{
	/*Increments the iterator and returns the next key in order.
	The next key could either be:
	- the next key in the precalculatedBaseKey list, that key can take diacritic arcs ,
		if this list is valid and nDiacArcs >0, or else
	- the next key in the database key list with nBaseArcs that can take diacritic arcs, if nDiacArcs > 0, or else
	- the next key in the database key list with nBaseArcs, or else ,
	- if there is no next key that fulfils the conditions aboove:
		an invalid key */

	if (pIt->usePrecalculated)
	{
		for(;;)
		{
			pIt->counter++;

			if (pIt->counter >= pIt->nStop)
			{
				return kitGetCurrentElement(pIt);
			}
			else
			{
				/*Search for precalculated basekey that can take diacritical arcs. */
				scrOUTPUT * pPreOut = kitGetSCROut(pIt);

				if ( !(pIt->nDiacArcs>0 && pIt->kid.noBaseArcs>1 && kidGetSymbol(&pPreOut->DBindex)[0]=='i' &&
					pPreOut->outSymbol == original) )
				{
					/*Special hack to deal with lowercase 'i'. Only 1-arced-i can have diacritics */

					int maskNr = keyGetDiacriticMaskNr( &pPreOut->DBindex );
					if ( maskNr != -1 && propDBGetDiacriticMask( pIt->kid.pPropDB, maskNr, pIt->nDiacArcs )!=0)
          {
						return kitGetCurrentElement(pIt);
          }
				}
			}
		}
	}
	else
	{
		if (pIt->nDiacArcs == 0)
		{
			pIt->counter++;
			pIt->kid.baseKeyIndex++;
			pIt->kid.pKey = NULL;
			return kitGetCurrentElement(pIt);
			/* Will return an 'invalid' kid. (correct behaviour) if we have reached beyond the last key */
		}

		else
		{
			for(;;)
			{
				pIt->counter++;

				if(pIt->counter >= pIt->nStop) /*We have reached beyond the last key with this number of arcs */
				{
					return kitGetCurrentElement(pIt); /* Will return an 'invalid' kid. (correct behaviour) */
				}
				else
				{
					KID * pKid = &pIt->kid;
					DECUMA_INT8 diacMaskNr;
					/*Search for basekey that can take diacritical arcs. */
					/*if (pIt->checkBaseKeysWithPoorShape) */
					/*	pKid->baseKeyIndex= baseKeysWithPoorShapeTable[pKid->noBaseArcs-1].pKey[pIt->counter].keyIndex; */
					/*else */
						pKid->baseKeyIndex= pIt->counter;

					kidSetBaseKeyPointer(pKid);
/*					pKid->pKey = &pKid->pDB->pKeyLists[pKid->noBaseArcs - 1].pKeys[pKid->baseKeyIndex]; */
					if (-1 != (diacMaskNr = keyGetDiacriticMaskNr( pKid )) &&
						propDBGetDiacriticMask( pKid->pPropDB, diacMaskNr,pIt->nDiacArcs) > 0)
					{

						return kitGetCurrentElement(pIt); /* Will return a valid kid */
					}
				}
			}
		}
	}
}

static DECUMA_BOOL kitGetCurrentElement(KIT * pIt)
{
	/*Returns the current key index, or a invalid key index if the position of the iterator
	has passed its limit*/
  DECUMA_BOOL retval;
	if (pIt->usePrecalculated)
	{
		if (pIt->counter >= pIt->nStop)
		{
			retval = FALSE;
		}
		else
		{
			pIt->kid = pIt->precalculatedOutputs->pOut[pIt->counter].DBindex;
			retval = TRUE;
		}
	}
	else
	{
		if(pIt->counter >= pIt->nStop)
		{
			retval = FALSE;
		}
		else
		{
			retval = TRUE;
		}
	}

	{
		KID * pKid = &pIt->kid;
		if (retval && !pKid->pKey)
		{
			kidSetBaseKeyPointer(pKid);
/*			pKid->pKey = &pKid->pDB->pKeyLists[pKid->noBaseArcs - 1].pKeys[pKid->baseKeyIndex]; */
		}
	}
  return retval;
}

void diakitCreate(DIAKIT * k, KID * kid, int nDiacArcs)
{
	/*Creates an iterator that iterates through the possible keys that
	have the key described by kid as base and diacritics with nDiacArcs arcs
	*/

	decumaAssert(k);
	decumaAssert(kid);
	decumaAssert(databaseValidKID(*kid));

	k->kid = *kid;
	k->kid.noDiacArcs = nDiacArcs;
	k->diacMask = propDBGetDiacriticMask( kid->pPropDB, keyGetDiacriticMaskNr(kid), nDiacArcs);
	k->nStop = propDBGetNoDiacKeys(k->kid.pPropDB,nDiacArcs );

	k->kid.diacKeyIndex--;
}

DECUMA_BOOL diakitGoNext(DIAKIT * pIt)
{
	/*Returns the next valid combinated key,
	or a invalid if there are no more valid keys*/
	DECUMA_BOOL retval;
	KID * pKid = &pIt->kid;

	for(;;)
	{
		pKid->diacKeyIndex++;
		pKid->pDiacKey = NULL;
		if (pKid->diacKeyIndex >= pIt->nStop)
		{ /*We have reached the last diacritic with this number of arcs */
			retval = FALSE;
			break;
		}
		else if( IS_CONTAINED_IN_MASK( pIt->diacMask, pKid->diacKeyIndex ))
		{
			retval = TRUE;
			break;
		}
		/* This was not valid in this case, try the next. */
	}

	if (retval && !pKid->pDiacKey)
	{
		kidSetDiacKeyPointer(pKid);
		/*pKid->pDiacKey = &pKid->pDB->pDiacLists[pKid->noDiacArcs - 1].pKeys[pKid->diacKeyIndex]; */
	}
	return retval;
}

scrOUTPUT * kitGetSCROut(KIT * pIt)
{
	if ( pIt )
	{
		if ( pIt->usePrecalculated )
			return &pIt->precalculatedOutputs->pOut[pIt->counter];
	}
	return NULL;
}

void kidSetDiacKeyOffset(KID * pKid, DECUMA_INT16 diacKeyXOffset, DECUMA_INT16 diacKeyYOffset)
{
	pKid->diacKeyXOffset = diacKeyXOffset;
	pKid->diacKeyYOffset = diacKeyYOffset;
}

