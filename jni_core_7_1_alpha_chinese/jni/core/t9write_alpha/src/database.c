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



#include "databaseKEY.h"
#include "database.h"
#include "databaseFormat.h"
#include "decumaMemory.h"
#include "decumaString.h"
#include "decumaAssert.h"
#include "decumaMath.h"
#include "globalDefs.h"
#include "decumaCategoriesHidden.h"
#include "decumaCategoryTranslation.h"

CATEGORY_TABLE_PTR keyDBGetStaticCategoryTable(KEY_DB_HEADER_PTR pKeyDB)
{
	return (CATEGORY_TABLE_PTR) ((DECUMA_DB_ADDRESS) pKeyDB + pKeyDB->staticCategoryTableOffset );
}

int keyDBGetNoKeys( KEY_DB_HEADER_PTR pKeyDB, int noArcs )
{
	if ((noArcs-1 < 0) || (noArcs-1 >= pKeyDB->nKeyLists)  )
		return 0;
	else
	{
		KEYLIST_PTR pKeyLists = (KEYLIST_PTR) ((DECUMA_DB_ADDRESS) pKeyDB + pKeyDB->keyListsOffset);

		return pKeyLists[ noArcs-1 ].nKeys;
	}
}

SYMBOL_TYPE_INFO_PTR keyDBGetStaticTypeTable(KEY_DB_HEADER_PTR pKeyDB)
{
	return (SYMBOL_TYPE_INFO_PTR) ((DECUMA_DB_ADDRESS) pKeyDB + pKeyDB->typeTableOffset);
}

SYMBOL_INFORMATION_PTR keyDBGetSymbolTable(KEY_DB_HEADER_PTR pKeyDB)
{
	return (SYMBOL_INFORMATION_PTR) ((DECUMA_DB_ADDRESS) pKeyDB + pKeyDB->symbolInfoTableOffset);
}



int kidIsGesture(const KID * pKid, int *pbInstant)
{
	SYMBOL_TYPE_INFO_PTR pTypeInfo = kidGetTypeInfo(pKid);
	decumaAssert(pTypeInfo);
	decumaAssert(pbInstant);

	*pbInstant = (pTypeInfo->gestureFlag == DBKEY_IS_INSTANT_GESTURE);

	return (pTypeInfo->gestureFlag == DBKEY_IS_GESTURE ||
		pTypeInfo->gestureFlag == DBKEY_IS_INSTANT_GESTURE);
}


SYMBOL_TYPE_INFO_PTR kidGetTypeInfo( const KID * pKid )
{
	KEY_PTR pKey = kidGetBaseKEY( pKid );
	DECUMA_INT16 typeIdx = pKey->typeIdx;
	SYMBOL_TYPE_INFO_PTR pTypeInfo = keyDBGetStaticTypeTable(pKid->pKeyDB);

	pTypeInfo+=typeIdx;

	decumaAssert(SYMBOL_TYPE_INFO_VALID_DUMMY_FIELDS(pTypeInfo));

	return pTypeInfo;
}

SYMBOL_INFORMATION_PTR  kidGetSymbolInfo( const KID * pKid )
{
	SYMBOL_TYPE_INFO_PTR pTypeInfo = kidGetTypeInfo(pKid);
	SYMBOL_INFORMATION_PTR pSymbolInfo = (SYMBOL_INFORMATION_PTR)
		((DECUMA_DB_ADDRESS) pKid->pKeyDB + pKid->pKeyDB->symbolInfoTableOffset);

	decumaAssert(pTypeInfo->symbolInfoIdx<pKid->pKeyDB->nSymbols);
	pSymbolInfo += pTypeInfo->symbolInfoIdx;

	decumaAssert(SYMBOL_INFORMATION_VALID_DUMMY_FIELDS(pSymbolInfo));

	return pSymbolInfo;
}

int kidGetArcOrders(const KID * pKid, DECUMA_INT8_DB_PTR* ppArcOrders, int nMaxArcOrders)
{
	KEY_DB_HEADER_PTR pKeyDB = pKid->pKeyDB;
	PROPERTY_DB_HEADER_PTR pPropDB = pKid->pPropDB;
	ARC_ORDER_SET_PTR pAOSet = (ARC_ORDER_SET_PTR)
		((DECUMA_DB_ADDRESS) pPropDB + pPropDB->allAOSetsOffset);

	DECUMA_INT16 arcOrderSetIdx;
	DECUMA_INT8_DB_PTR pArcOrderIdxs;
	DECUMA_INT8_DB_PTR pAllArcOrders;
	int i;


	if ( kidHasDiac(pKid) )
	{
		decumaAssert(pKid->noBaseArcs == 0); /*Diac arc order only. Not combination */
		arcOrderSetIdx = kidGetDiacKEY(pKid)->arcOrderSetIdx;
	}
	else
	{
		SYMBOL_TYPE_INFO_PTR pInfo = kidGetTypeInfo(pKid);
		arcOrderSetIdx = pInfo->arcOrderSetIdx;
	}


	pAOSet += arcOrderSetIdx;

	pArcOrderIdxs = (DECUMA_INT8_DB_PTR)
		((DECUMA_DB_ADDRESS) pPropDB + pAOSet->arcOrdersOffset);

	pAllArcOrders = (DECUMA_INT8_DB_PTR)
		((DECUMA_DB_ADDRESS) pPropDB + pPropDB->arcOrderOffset);

	for (i=0; i<pAOSet->nArcOrders && i<nMaxArcOrders; i++)
	{
		ppArcOrders[i] = pAllArcOrders + pArcOrderIdxs[i]*pKeyDB->nMaxArcsInCurve;
	}

	return i; /*The number of arc orders. */


}


SMALL_KEY_PTR propDBGetSmallKey(PROPERTY_DB_HEADER_PTR pPropDB)
{
	SMALL_KEY_PTR pSmallKey;

	decumaAssert(pPropDB);

	pSmallKey = (SMALL_KEY_PTR) ((DECUMA_DB_ADDRESS) pPropDB + pPropDB->smallKeyOffset);

	decumaAssert(SMALL_KEY_VALID_DUMMY_FIELDS(pRotation));

	return pSmallKey;
}

MIN_DISTANCES_PTR propDBGetMinDistances(PROPERTY_DB_HEADER_PTR pPropDB)
{
	MIN_DISTANCES_PTR pMinDistances;

	decumaAssert(pPropDB);

	pMinDistances = (MIN_DISTANCES_PTR) ((DECUMA_DB_ADDRESS) pPropDB + pPropDB->minDistanceOffset);

	decumaAssert(MIN_DISTANCES_VALID_DUMMY_FIELDS(pMinDistances));

	return pMinDistances;
}

CH_PAIR_DIST_CAT_PTR propDBGetCharDistanceTable(PROPERTY_DB_HEADER_PTR pPropDB)
{
	MIN_DISTANCES_PTR pMinDistances;
	CH_PAIR_DIST_CAT_PTR pTable;

	decumaAssert(pPropDB);
	pMinDistances = propDBGetMinDistances(pPropDB);

	pTable = (CH_PAIR_DIST_CAT_PTR) ((DECUMA_DB_ADDRESS) pPropDB + pMinDistances->charDistTableOffset);

	decumaAssert(pTable == 0 || CH_PAIR_DIST_CAT_VALID_DUMMY_FIELDS(pTable) );

	return pTable;
}


ROTATION_PTR propDBGetRotation(PROPERTY_DB_HEADER_PTR pPropDB, int idx)
{
	ROTATION_PTR pRotation = (ROTATION_PTR) ((DECUMA_DB_ADDRESS) pPropDB + pPropDB->rotationTableOffset);

	return &pRotation[idx];
}


DECUMA_UINT16 propDBGetSymmetry(PROPERTY_DB_HEADER_PTR pPropDB, int idx)
{
	DECUMA_UINT16_DB_PTR pSymmetry = (DECUMA_UINT16_DB_PTR)
		((DECUMA_DB_ADDRESS) pPropDB + pPropDB->symmetryTableOffset);

	return pSymmetry[idx];
}

int propDBGetTypeConflicts(PROPERTY_DB_HEADER_PTR pPropDB,
						   TYPE_CONFLICTS_PTR* ppConflicts)
{
	decumaAssert( ppConflicts );

	if (pPropDB->nTypeConflicts)
	{
		*ppConflicts = (TYPE_CONFLICTS_PTR) ((DECUMA_DB_ADDRESS) pPropDB  + pPropDB->typeConflictsOffset);

		decumaAssert(TYPE_CONFLICTS_VALID_DUMMY_FIELDS(ppConflicts[0]));
	}

	return pPropDB->nTypeConflicts;
}

DIAC_SYMBOL_INFO_PTR propDBGetDiacSymbolTable(PROPERTY_DB_HEADER_PTR pPropDB)
{
	return (DIAC_SYMBOL_INFO_PTR) ((DECUMA_DB_ADDRESS) pPropDB +
		pPropDB->diacSymbolTableOffset);
}

int propDBGetNoDiacKeys( PROPERTY_DB_HEADER_PTR pPropDB, int noArcs )
{
	if ((noArcs-1 < 0) || (noArcs-1 >= pPropDB->nDiacKeyLists)  )
		return 0;
	else
	{
		KEY_DIACRITIC_LIST_PTR pDiacKeyLists = (KEY_DIACRITIC_LIST_PTR)
			((DECUMA_DB_ADDRESS) pPropDB + pPropDB->diacListsOffset);

		return pDiacKeyLists[ noArcs-1 ].nKeys;
	}
}


/*DECUMA_INT8_DB_PTR propDBGetArcOrders( PROPERTY_DB_HEADER_PTR pPropDB) */
/*{ */
/*	return (DECUMA_INT8_DB_PTR) ((DECUMA_DB_ADDRESS) pPropDB + pPropDB->arcOrderOffset); */
/*} */


DECUMA_UINT16 propDBGetDiacriticMask( PROPERTY_DB_HEADER_PTR pPropDB,
									 int maskNr, int nDiacArcs )
{
	DECUMA_UINT16_DB_PTR pDiacMasks = (DECUMA_UINT16_DB_PTR)
		((DECUMA_DB_ADDRESS) pPropDB + pPropDB->diacMasksOffset);

	decumaAssert(nDiacArcs <= pPropDB->nDiacKeyLists);
	decumaAssert(nDiacArcs > 0);

	return pDiacMasks[maskNr*pPropDB->nDiacKeyLists + nDiacArcs-1];
}




CH_PAIR_ZOOM_INFO_PTR kidGetZoomInfo( const KID * pKid1,	const KID * pKid2,
		DECUMA_INT8 * pLikelyCandidateIfTestPasses)
{
	DECUMA_UNICODE symbol[2];
	SYMBOL_TYPE_INFO_PTR pTypeInfo[2];

	CH_PAIR_ZOOM_INFO_TABLE_PTR pPairTable;
	CH_PAIR_ZOOM_INFO_PTR pZoomInfo;
	PROPERTY_DB_HEADER_PTR pPropDB = pKid1->pPropDB;

	int idx[2] = {0,1};
	int i;



	symbol[0] = kidGetSymbol(pKid1)[0];
	symbol[1] = kidGetSymbol(pKid2)[0];
	pTypeInfo[0] = kidGetTypeInfo(pKid1);
	pTypeInfo[1] = kidGetTypeInfo(pKid2);

	if ( pKid1->pKeyDB->databaseType != DB_TYPE_STATIC ||
		 pKid2->pKeyDB->databaseType != DB_TYPE_STATIC  )
		 return NULL; /* Zoom is only supported in static database */

	if (symbol[0] > symbol[1])
	{
		/* Reverse order */
		idx[0]=1; 
		idx[1]=0;
	}


	decumaAssert(pKid1->pPropDB == pKid2->pPropDB);

	pPairTable = ( CH_PAIR_ZOOM_INFO_TABLE_PTR)
		((DECUMA_DB_ADDRESS) pPropDB + pPropDB->zoomInfoListsOffset);

	pPairTable += pKid1->noBaseArcs + pKid1->noDiacArcs -1;

	pZoomInfo = (CH_PAIR_ZOOM_INFO_PTR)
		((DECUMA_DB_ADDRESS) pPropDB + pPairTable->zoomInfosOffset);

	for (i = 0; i < pPairTable->nZoomInfos; i++, pZoomInfo++)
	{
		if ( pZoomInfo->symbol1 != symbol[idx[0]] ||
			 pZoomInfo->symbol2 != symbol[idx[1]])
		{
			/* Symbol mismatch */
			continue;
		}

		if ( (pZoomInfo->type1 != 255 && pZoomInfo->type1 != pTypeInfo[idx[0]]->typeNr) ||
			(pZoomInfo->type2 != 255 && pZoomInfo->type2 != pTypeInfo[idx[1]]->typeNr))
		{
			/* Symbol type mismatch when info is there */
			continue;
		}

		/* We could also go on to check arc order to be extra certain...
		* .. then check that ZoomArcOrder1(pOut1->pArcOrder) == ZoomArcOrder2(pOut2->pArcOrder)
		* But we skip this for now, and assume that it is unlikely that 
		* we get 1st and 2nd candidates of right types but mismatching arc orders. 
		*/

		if (pZoomInfo->likelyIfTestPasses != idx[0]) /* (idx[0]==0 && likelyIfTestPasses== 1) || (idx[0]==1 && likelyIfTestPasses==0) */
		{
			*pLikelyCandidateIfTestPasses = 1;
		}
		else
		{
			*pLikelyCandidateIfTestPasses = 0;
		}

		decumaAssert(CH_PAIR_ZOOM_INFO_VALID_DUMMY_FIELDS(pZoomInfo));

		return pZoomInfo;
	}

	return NULL;
}

SVM_PARAMETERS_STRUCT_PTR propDBGetSvmParameterStruct( PROPERTY_DB_HEADER_PTR pPropDB,
										int svmIndex)
{
	SVM_DB_HEADER_PTR pSVMDB = (SVM_DB_HEADER_PTR)
		((DECUMA_DB_ADDRESS) pPropDB + pPropDB->svmDBHeaderOffset);

	SVM_PARAMETERS_STRUCT_PTR pSVMParametersStruct = (SVM_PARAMETERS_STRUCT_PTR)
		((DECUMA_DB_ADDRESS) pPropDB + pSVMDB->parametersStructOffset);

	decumaAssert(svmIndex>=0);
	decumaAssert(svmIndex<pSVMDB->nParametersStruct);

	return &pSVMParametersStruct[svmIndex];
}

SVM_NORMAL_LIST_PTR propDBGetSvmNormalList( PROPERTY_DB_HEADER_PTR pPropDB,
										int nArcs)
{
	SVM_DB_HEADER_PTR pSVMDB = (SVM_DB_HEADER_PTR)
		((DECUMA_DB_ADDRESS) pPropDB + pPropDB->svmDBHeaderOffset);

	SVM_NORMAL_LIST_PTR pSVMNormalList = (SVM_NORMAL_LIST_PTR)
		((DECUMA_DB_ADDRESS) pPropDB + pSVMDB->normalListOffset);

	decumaAssert(nArcs>=1);
	decumaAssert(nArcs<=pSVMDB->nNormalLists);

	return &pSVMNormalList[nArcs-1];
}



DECUMA_UINT16 kidGetSymmetry( const KID * pKid )
{
	if (!kidHasDiac(pKid))
		return propDBGetSymmetry(pKid->pPropDB,kidGetTypeInfo(pKid)->symmetryIdx);
	else
	{
		/*Diacritic arcs */
		if (pKid->noBaseArcs == 0)
			return propDBGetSymmetry(pKid->pPropDB,kidGetDiacKEY(pKid)->symmetryIdx);
		else
			return propDBGetSymmetry(pKid->pPropDB,0); /*default setting */
	}
}

ROTATION_PTR kidGetRotation( const KID * pKid )
{
	if (!kidHasDiac(pKid))
		return propDBGetRotation(pKid->pPropDB,kidGetTypeInfo(pKid)->rotIdx);
	else
		return propDBGetRotation(pKid->pPropDB,0);/*default setting */
}





KEY_DIACRITIC_PTR kidGetDiacKEY(const KID * pKid )
{
	decumaAssert(pKid->pPropDB != 0);
	decumaAssert(pKid->noDiacArcs > 0);
	decumaAssert(pKid->noDiacArcs <= pKid->pPropDB->nDiacKeyLists);

	if (!pKid->pDiacKey)
	{
		decumaAssert(0);

		/*KEY_DIACRITIC_LIST_PTR pList = (KEY_DIACRITIC_LIST_PTR) */
		/*	((DECUMA_DB_ADDRESS) pKid->pPropDB + pKid->pPropDB->diacListsOffset); */
		/*KEY_DIACRITIC_PTR pDiacKey ; */

		/*pList += pKid->noDiacArcs-1; */

		/*decumaAssert(pList != 0); */
		/*decumaAssert(pKid->diacKeyIndex >= 0); */
		/*decumaAssert(pKid->diacKeyIndex < pList->nKeys); */

		/*pDiacKey = (KEY_DIACRITIC_PTR) ((DECUMA_DB_ADDRESS) pKid->pPropDB + pList->diacKeysOffset); */
		/*((KID *)pKid)->pDiacKey = pDiacKey+pKid->diacKeyIndex; */
	}
	return pKid->pDiacKey;
}


#ifdef _DEBUG
/* If it is a release build, there will be a macro. */
KEY_PTR kidGetBaseKEY(const KID * pKid )
{
	decumaAssert(pKid->pKeyDB != 0);
	decumaAssert(pKid->noBaseArcs > 0);
	decumaAssert(pKid->noBaseArcs <= pKid->pKeyDB->nKeyLists);

	if (!pKid->pKey)
	{
		decumaAssert(0);
		/*KEYLIST_PTR pList; */

		/*pList = &pKid->pDB->pKeyLists[pKid->noBaseArcs -1]; */

		/*decumaAssert(pList != 0); */
		/*decumaAssert(pKid->baseKeyIndex >= 0); */
		/*decumaAssert(pKid->baseKeyIndex < pList->noKeys); */

		/*((KID *)pKid)->pKey = &pList->pKeys[pKid->baseKeyIndex]; */
	}
	return pKid->pKey;
}
#endif



PROPERTY_DB_HEADER_PTR staticDBGetPropDB(STATIC_DB_HEADER_PTR pDB)
{
	PROPERTY_DB_HEADER_PTR pPropDB = (PROPERTY_DB_HEADER_PTR)
		((DECUMA_DB_ADDRESS) pDB + pDB->propertyDBOffset);

	decumaAssert(PROPERTY_DB_HEADER_VALID_DUMMY_FIELDS(pPropDB));

	return pPropDB;
}

KEY_DB_HEADER_PTR staticDBGetKeyDB(STATIC_DB_HEADER_PTR pDB)
{
	KEY_DB_HEADER_PTR pKeyDB = (KEY_DB_HEADER_PTR )
		((DECUMA_DB_ADDRESS) pDB + pDB->keyDBOffset);

	decumaAssert(KEY_DB_HEADER_VALID_DUMMY_FIELDS(pKeyDB));

	return pKeyDB;
}








DECUMA_INT8 advanceArcNumber(DECUMA_INT8 num, int steps)
{
	if (num<0)
		return (DECUMA_INT8) (num-steps);
	else
		return (DECUMA_INT8) (num+steps);
}

int curveToKeyIndexOfArc(const DECUMA_INT8 * arcOrder, const int arcNrCURVEindexed)
{
	/*This function takes a CURVEindexed arcNr and translates it into a KEYindexed arcNr. */
	/*i.e. it gives the number of the arc in the key that corresponds to the arc */
	/*with the given number (arcNrCURVEindexed). */

	int i;
	/*	JM TODO: The correct value here is really pKeyDB->nMaxArcsInCurve */
	for (i=0; i<MAX_NUMBER_OF_ARCS_IN_CURVE; i++)
	{
		if (getArcNr(arcOrder,i)==arcNrCURVEindexed)
			return i;
	}
	return -1;
}

int databaseInit(void)
{
	return 0;
}

void databaseRelease(void)
{
}

/*const DBID databaseGetStaticDB() */
/*{ */
/*	return (const DBID)&static_db; */
/*} */

/*void databaseInitStaticDB(CATEGORY_TABLE_PTR pCategoryTable) */
/*{ */
	/* Initilize the category table. */
/*	databaseSetCategoryTable(pCategoryTable); */
/*} */

CATEGORIES_PTR kidGetCategories( const KID * pKid )
{
	if ( !kidHasDiac( pKid ) )
	{
		int typeIndex = pKid->pKey->typeIdx;
		CATEGORIES_PTR pBaseCats = dcGetBaseCategoryTable( pKid->pCatTable);


		decumaAssert(typeIndex >= 0 && typeIndex < keyDBGetNoSymbolTypes(pKid->pKeyDB));
		decumaAssert(pBaseCats);

		return &pBaseCats[typeIndex];
	}
	else
	{
		/* Only diac keys in static database */
		DIAC_SYMBOL_INFO_PTR pDiacInfo = kidGetDiacSymbolInfo(pKid);
		const int typeIndex = (pDiacInfo - propDBGetDiacSymbolTable(pKid->pPropDB));
		CATEGORIES_PTR pDiacCats = dcGetDiacCategoryTable( pKid->pCatTable);


		decumaAssert( pKid->pKeyDB->databaseType == DB_TYPE_STATIC );
		decumaAssert(typeIndex >= 0 && typeIndex < propDBGetNoDiacSymbols(pKid->pPropDB) );
		decumaAssert(pDiacCats);

		return &pDiacCats[typeIndex];
	}
}


/* This function shall only be used when the KID is not available!! */
/*SYMBOL_INFORMATION_PTR databaseGetSymInfo(const DECUMA_UNICODE * symbol)
{
	int i;
	DBID pDB = databaseGetStaticDB();
	SYMBOL_INFORMATION_PTR pSymInfo = &st[0]; //Symbol table in static DB.

	for ( i = 0; i < pDB->nSymbols; i++)
	{
		if ( decumaStrcmp(symbol, pSymInfo->symbol) == 0 )
			return pSymInfo;
	}

	// Not found!
	return 0;
} // databaseGetSymInfo()*/


int isSameDBSymbol(DECUMA_UNICODE_DB_PTR pDBSymbol1, DECUMA_UNICODE_DB_PTR pDBSymbol2)
{
	for (; *pDBSymbol1 && *pDBSymbol2 && *pDBSymbol1 == *pDBSymbol2; pDBSymbol1++, pDBSymbol2++);
	return *pDBSymbol1 == 0 && *pDBSymbol2 == 0;
}

int isSameSymbol(DECUMA_UNICODE_DB_PTR pDBSymbol, const DECUMA_UNICODE * pSymbol,
						int bToUpper)
{
	if ( bToUpper ) {
		decumaAssert(pDBSymbol[0] && !pDBSymbol[1]); /* Alternative symbols are always assumed to be exactly one character long */
		if (pSymbol[1] == 0 && decumaUnicodeToUpper(pDBSymbol[0], 1) == pSymbol[0]) return 1;
		return 0;
	} else {
		for (; *pDBSymbol && *pSymbol && *pDBSymbol == *pSymbol; pDBSymbol++, pSymbol++);
		return *pDBSymbol == 0 && *pSymbol == 0;
	}
}

DECUMA_STATUS databaseIncludesSymbol(KEY_DB_HEADER_PTR pKeyDB,
									 PROPERTY_DB_HEADER_PTR pPropDB,
									 const DECUMA_CHARACTER_SET * pCharSet,
									 const DECUMA_UNICODE * pSymbol,
									 int * pbIncluded)
{
	SYMBOL_TYPE_INFO_PTR pTI = keyDBGetStaticTypeTable(pKeyDB);
	DIAC_SYMBOL_INFO_PTR pDSI = pPropDB ? propDBGetDiacSymbolTable(pPropDB) : NULL;
	SYMBOL_INFORMATION_PTR pSItable = keyDBGetSymbolTable(pKeyDB);

	CATEGORY activeCat;

	CATEGORY_TABLE_PTR pStaticCatTable = keyDBGetStaticCategoryTable(pKeyDB);
	CATEGORIES_PTR pStaticDiacCategories = dcGetDiacCategoryTable(pStaticCatTable);
	CATEGORIES_PTR pStaticBaseCategories = dcGetBaseCategoryTable(pStaticCatTable);

	int nSymbolTypes = keyDBGetNoSymbolTypes( pKeyDB );
	int nDiacSymbols = pPropDB ? propDBGetNoDiacSymbols( pPropDB ) : 0;

	DECUMA_STATUS status = decumaNoError;
	int n;
	decumaAssert(pbIncluded);

	*pbIncluded =0;

	if (pCharSet)
	{
		status = translateToCategoryStructs(
			pCharSet,	pStaticCatTable, NULL, &activeCat, NULL);

		if (status != decumaNoError)
			return status;
	}
	else
	{
		CATEGORY_MASK_SET_ALL_BITS(activeCat.symbolCat);
		CATEGORY_MASK_SET_ALL_BITS(activeCat.languageCat);
	}

	/* Try to find the symbol among the base symbols */
	for ( n = 0; n < nSymbolTypes; n++, pTI++, pStaticBaseCategories++)
	{
		SYMBOL_INFORMATION_PTR pSI = &pSItable[pTI->symbolInfoIdx];
		DECUMA_UNICODE_DB_PTR pStaticSymbol = (DECUMA_UNICODE_DB_PTR)
			((DECUMA_DB_ADDRESS) pKeyDB + pSI->symbolOffset);

		if (IS_IN_CATEGORY(pStaticBaseCategories->cat,activeCat) &&
			isSameSymbol(pStaticSymbol, pSymbol, 0))
		{
			*pbIncluded =1;
			return decumaNoError;
		}

			/* Check if the database symbol is equal to the symbol */
			/* when it is scaled to upper. */
		if (IS_IN_CATEGORY(pStaticBaseCategories->altCat,activeCat) &&
			isSameSymbol(pStaticSymbol, pSymbol, 1))
		{
			{
				decumaAssert(CAN_BE_SCALED_OR_TRANSLATED_TO_UPPER(pTI));
				*pbIncluded =1;
				return decumaNoError;
			}
		}
	}

	/* Try to find the symbol among the diacritic keys. */
	/* Note that the symbol categories for diacs are stored per symbol and not per symbol type */
	for ( n = 0; n < nDiacSymbols; n++, pDSI++, pStaticDiacCategories++)
	{
		DECUMA_UNICODE_DB_PTR pStaticSymbol = (DECUMA_UNICODE_DB_PTR)
			((DECUMA_DB_ADDRESS) pKeyDB + pDSI->diacSymbolOffset);

		if ( IS_IN_CATEGORY(pStaticDiacCategories->cat,activeCat) &&
			isSameSymbol(pStaticSymbol, pSymbol, 0))
		{
			*pbIncluded =1;
			return decumaNoError;
		}

		/*
		Check the static category table if the symbol can be scaled
		or translated */
		if ( IS_IN_CATEGORY(pStaticDiacCategories->altCat,activeCat) &&
			isSameSymbol(pStaticSymbol, pSymbol, 1))
		{
			*pbIncluded =1;
			return decumaNoError;
		}
	}

	/*Not found */
	return decumaNoError;
}




void databaseCheckCategory(int * pbInCat, int * pbAltInCat, const KID * pKid,
						   const CATEGORY Category)
{
	CATEGORIES_PTR pCategories;

	decumaAssert( pKid );
	decumaAssert(databaseValidKID(*pKid));
	decumaAssert( pbInCat );
	decumaAssert( pbAltInCat );
	decumaAssert( sizeof(struct _DB_CATEGORIES) == sizeof(struct _CATEGORIES));

	pCategories = kidGetCategories( pKid );

	*pbInCat = IS_IN_CATEGORY(pCategories->cat, Category);

	*pbAltInCat = IS_IN_CATEGORY(pCategories->altCat, Category);
}

int databaseHasWrongFormat(STATIC_DB_HEADER_PTR pStaticDB)
{
	if ( pStaticDB->formatVersionNbr != DATABASE_FORMAT_VERSION_NR )
		return 1; /* Wrong db-version or endianess. */

	if ( keyDBHasWrongFormat(staticDBGetKeyDB(pStaticDB)) )
		return 1;

	if ( propDBHasWrongFormat(staticDBGetPropDB(pStaticDB)) )
		return 1;

	return 0; /* This database seems OK! */
}

int keyDBHasWrongFormat(KEY_DB_HEADER_PTR pKeyDB)
{
	if ( pKeyDB->nPointsInArc != NUMBER_OF_POINTS_IN_ARC )
		return 1; /* Wrong number of points in arc or endianess */

	if ( pKeyDB->nPointsInInterleavedArc != NUMBER_OF_POINTS_IN_INTERLEAVED_ARC )
		return 1;

	if ( pKeyDB->nMaxArcsInCurve > MAX_NUMBER_OF_ARCS_IN_CURVE )
		return 1;

	if ( pKeyDB->nMaxArcOrdersPerSymbol > MAX_ARC_ORDERS_PER_SYMBOL )
		return 1;

	if (!KEY_DB_HEADER_VALID_DUMMY_FIELDS(pKeyDB))
		return 1;

	return 0; /* This database seems OK! */
}

int propDBHasWrongFormat(PROPERTY_DB_HEADER_PTR pPropDB)
{
	if ( pPropDB->nDiacKeyLists > MAX_ARCS_IN_DIACRITIC )
		return 1;

	if ( pPropDB->altSymbolScaleNum != ALT_SYMBOL_SCALE_NUM ||
		 pPropDB->altSymbolScaleDenom != ALT_SYMBOL_SCALE_DENOM ||
		 pPropDB->altSymbolScaleThreshold != ALT_SYMBOL_SCALE_THRESHOLD ||
		 pPropDB->altSymbolTranslation != ALT_SYMBOL_TRANSLATION )
		return 1;

	return 0; /* This database seems OK! */
}



#ifdef ONPALM_ARMLET
#include "dataReloc.h"
void databaseReloc(void * pArmlet)
{
	/* Relocate all constant pointers (addresses) in the database */
	/* with offset "pArmlet". */
	DBID pDB = (DBID) databaseGetStaticDB();
	int n,k;

	dataReloc(pArmlet, (void*) &pDB->pKeyLists);

	for ( n = 0; n < pDB->nMaxArcs; n++)
	{
		KEY_PTR pKey;
		dataReloc(pArmlet, (void*) &pDB->pKeyLists[n].pKeys);

		pKey = pDB->pKeyLists[n].pKeys;
		for ( k = 0; k < pDB->pKeyLists[n].noKeys; k++ , pKey++)
		{
			dataReloc(pArmlet, (void*) &pKey->pTypeInfo );
			dataReloc(pArmlet, (void*) &pKey->keyCurve );
		}
	}

	dataReloc(pArmlet, (void*) &pDB->pDiacLists);
	for ( n = 0; n < pDB->nMaxDiacArcs; n++)
	{
		KEY_DIACRITIC_PTR pDiacKey;
		dataReloc(pArmlet, (void*) &pDB->pDiacLists[n].pKeys);
		pDiacKey = pDB->pDiacLists[n].pKeys;
		for ( k = 0; k < pDB->pDiacLists[n].noKeys; k++ , pDiacKey++) {
			dataReloc(pArmlet, (void*) &pDiacKey->keyCurve );
		}
	}

	dataReloc(pArmlet, (void*) &pDB->pRotationTable);
	dataReloc(pArmlet, (void*) &pDB->pSymmetryTable);
	dataReloc(pArmlet, (void*) &pDB->pAllAOSets);
	dataReloc(pArmlet, (void*) &pDB->pDiacriticMasks);
	dataReloc(pArmlet, (void*) &pDB->pCombinationSymbols);
	dataReloc(pArmlet, (void*) &pDB->pBaseCategories);
	dataReloc(pArmlet, (void*) &pDB->pDiacCategories);

	for ( n = 0; n < sizeof(allAOSets)/sizeof(allAOSets[0]); n++)
	{
		dataReloc(pArmlet, (void*) &allAOSets[n].arcOrder);

		for ( k = 0; k < allAOSets[n].noOrders; k++) {
			dataReloc(pArmlet, (void*)&allAOSets[n].arcOrder[k]);
		}
	}

	for ( n = 0; n < sizeof(st)/sizeof(st[0]); n++) {
		dataReloc(pArmlet, (void*) &st[n].symbol);
	}

	for ( n = 0; n < sizeof(dst)/sizeof(dst[0]); n++) {
		dataReloc(pArmlet, (void*) &dst[n].pSymbol);
	}

	for ( n = 0; n < sizeof(tt)/sizeof(tt[0]); n++) {
		dataReloc(pArmlet, (void*) &tt[n].pSymbolInfo);
	}

	for ( n = 0; n < sizeof(combinationSymbol)/sizeof(combinationSymbol[0]); n++) {
		for ( k = 0; k < sizeof(combinationSymbol[0])/sizeof(combinationSymbol[0][0]); k++) {
			if (combinationSymbol[n][k]) {
				dataReloc(pArmlet, (void*) &combinationSymbol[n][k]);
			}
		}
	}
	dataReloc(pArmlet, (void*) &smallKeysTable.pKey);

	for ( n = 0; n < sizeof(chPairZoomInfoTable)/sizeof(chPairZoomInfoTable[0]); n++) {
		if (chPairZoomInfoTable[n].pPair )
			dataReloc(pArmlet, (void*) &chPairZoomInfoTable[n].pPair);
	}
}
#endif
