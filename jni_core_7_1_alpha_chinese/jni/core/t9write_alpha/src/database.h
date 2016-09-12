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


/*$Header: /data/cvsroot/seattle/t9write/alphabetic/core/src/database.h,v 1.5 2011/02/14 11:41:14 jianchun_meng Exp $*/
#ifndef DATABASE_H
#define DATABASE_H

#ifdef DECUMA_MANGLE
#include "mangle.h"
#endif

#include "databaseFormat.h"
#include "databaseKID.h"
#include "decumaStatus.h"
#include "decumaCharacterSetType.h"
#ifdef __cplusplus
extern "C" {
#endif

extern const int straightOneIndexInDatabase; 
extern SYMBOL_INFORMATION_PTR symTable;

extern const CATEGORY allValidCategories; /* All valid categories in the current static DB */

#define CAN_BE_SCALED_OR_TRANSLATED_TO_UPPER( pTypeInfo ) ((pTypeInfo)->lowerToUpper )

#define CAN_BE_SCALED_TO_UPPER( pTypeInfo ) ((pTypeInfo)->lowerToUpper == 1 )
#define CAN_BE_TRANSLATED_TO_UPPER( pTypeInfo ) ((pTypeInfo)->lowerToUpper == 2 )

/*
#define CATEGORY_IS_VALID( cat_in , pAllValidUDMCategories) \
   ( (cat_in & allValidCategories) || \
     (pAllValidUDMCategories && (cat_in & *pAllValidUDMCategories))) 
*/

#define IS_CONTAINED_IN_MASK(mask,index) ( ((mask)>>(index)) &1 )
	 
/* NOTE: The meaning of g_alpha has change from sqr(fw) to fw * 8, i.e. */
/* 8 -> sqrt(8) * 8 ~= 23, 1 -> 8, fw = feature weight /JA */
/* */
/* JA Comments: */
/* It was noted that sum_dx and sum_dy feature was used. This was assumed to be a bug, but */
/* proved to have a positive effect on hitrate. For now this feature is kept, but tunability */
/* is added with the feature weight (0 turns it off). If this feature is valuable to describe */
/* the net track of a curve, then the average magnitude of deviation from this net track could */
/* also be a valueable feature. This feature has been added, but is for now disabled. The extra */
/* features seems valuable, but it is hard to optimize for templates in all databases. For a specific */
/* database it is easier to find optimums. Before further work on this, calculation accuracy problems */
/* in the engine should be fixed. */
/*  */
#define g_alpha_x			8 /* corresponds to old (implicit) value 1 */
#define g_alpha_y			8 /* corresponds to old (implicit) value 1 */
#define g_alpha_dx			23 /* Approx. corresponds to old value 8 */
#define g_alpha_dy			23 /* Approx. corresponds to old value 8 */
#define g_alpha_sum_dx			23 /* Keeps old "bug" */
#define g_alpha_sum_dy			23 /* Keeps old "bug" */
#define g_alpha_sum_abs_dx		0 /* Seems useful, but is proximity measure suitable for this feature? Disabled for now. */
#define g_alpha_sum_abs_dy		0 /* Seems useful, but is proximity measure suitable for this feature? Disabled for now. */
#define g_measureId		0

#define databaseEqualKID( k1, k2 ) (k1.pDB == k2.pDB && k1.baseKeyIndex == k2.baseKeyIndex && k1.noBaseArcs == k2.noBaseArcs &&\
	k1.noDiacArcs == k2.noDiacArcs && (k1.noDiacArcs == 0 || k1.diacKeyIndex == k2.diacKeyIndex) )

#define databaseValidKID( kid )   ( ((kid).pKeyDB != NULL) && \
	((kid).pPropDB != NULL) && \
	((kid).pCatTable != NULL) && \
	((kid).baseKeyIndex >= 0) && \
	((kid).baseKeyIndex < keyDBGetNoKeys((kid).pKeyDB,  (kid).noBaseArcs)) && \
	(((kid).noDiacArcs == 0) || (((kid).diacKeyIndex >= 0) && ((kid).diacKeyIndex < propDBGetNoDiacKeys((kid).pPropDB, (kid).noDiacArcs)))))


#define IS_STRAIGHT_ONE(pKID) ( (pKID->pDB == databaseGetStaticDB()) && (pKID->baseKeyIndex == straightOneIndexInDatabase) && (pKID->noBaseArcs == 1) )
#define KID_FROM_DIFFERENT_DATABASES(kid1, kid2) ( (kid1.pDB) != (kid2.pDB) )

DECUMA_HWR_PRIVATE int databaseInit(void);
DECUMA_HWR_PRIVATE void databaseRelease(void);

DECUMA_HWR_PRIVATE KEY_DIACRITIC_PTR kidGetDiacKEY(const KID * pKid );

#ifdef _DEBUG
DECUMA_HWR_PRIVATE KEY_PTR kidGetBaseKEY(const KID * pKid );
#else
#define kidGetBaseKEY(pKid) ((pKid)->pKey)
/*#define kidGetDiacKEY(pKid) ((pKid)->pDiacKey) */
#endif

/*This returns the number of the arc of the written curve (the CURVEindex) that corresponds to the */
/*a:th arc of the key (KEYindex a), according to the arc order defined by arcOrder. */
/*	JM TODO: The correct value here is really pKeyDB->nMaxArcsInCurve */
#define getArcNr( arcOrder, a )	((a) < MAX_NUMBER_OF_ARCS_IN_CURVE ? decumaAbs((arcOrder)[(a)])-1 : (a))

/*This returns 1 if the written arc with KEYindex 'a' should be read backwards according to arcOrder */
/*	JM TODO: The correct value here is really pKeyDB->nMaxArcsInCurve */
#define arcIsReverse( arcOrder, a)	((a) < MAX_NUMBER_OF_ARCS_IN_CURVE ? ((arcOrder)[(a)]>0 ? 0 : 1) : 0)

DECUMA_HWR_PRIVATE DECUMA_INT8 advanceArcNumber(DECUMA_INT8 num, int steps);
DECUMA_HWR_PRIVATE int curveToKeyIndexOfArc(const DECUMA_INT8 * arcOrder, const int arcNrCURVEindexed);

DECUMA_HWR_PRIVATE CH_PAIR_ZOOM_INFO_PTR propDBGetZoomInfo( const KID * pKid1,	const KID * pKid2,
		DECUMA_INT8 * pLikelyCandidateIfTestPasses);

DECUMA_HWR_PRIVATE SVM_PARAMETERS_STRUCT_PTR propDBGetSvmParameterStruct( PROPERTY_DB_HEADER_PTR pPropDB,
										int svmIndex);

DECUMA_HWR_PRIVATE SVM_NORMAL_LIST_PTR propDBGetSvmNormalList( PROPERTY_DB_HEADER_PTR pPropDB,
										int nArcs);


/* This function shall only be used when the KID is not available!! */
/* Returns the symbol info pointer to the symbol if availabe in the  */
/* static DB else zero. */
DECUMA_HWR_PRIVATE SYMBOL_INFORMATION_PTR databaseGetSymInfo(const DECUMA_UNICODE * symbol);

/* This function writes 1 at pbInCat if the symbol is in the Category otherwise 0. */
/* It also writes 1 at pbAltInCat if the alternative symbol is in the category otherwise 0. */
DECUMA_HWR_PRIVATE void databaseCheckCategory(int * pbInCat, int * pbAltInCat, const KID * pKid, const CATEGORY Category);

/* Returns the number of elements in the type conflicts table of the  */
/* static database. *ppConflicts is set to the first element in the  */
/* table. */
DECUMA_HWR_PRIVATE int databaseGetStaticDBTypeConflicts(TYPE_CONFLICTS_PTR* ppConflicts);

#define getSymmetry(key) static_db.pSymmetryTable[(key)->symmetryNr]
#define getRotation(key) static_db.pRotationTable[(key)->rotationNr]

#define databaseIsStaticDB( pDB ) ((pDB) == &static_db)
DECUMA_HWR_PRIVATE STATIC_DB_HEADER_PTR databaseGetStaticDB(void);
DECUMA_HWR_PRIVATE void databaseInitStaticDB(CATEGORY_TABLE_PTR pCategoryTable);


DECUMA_HWR_PRIVATE DECUMA_UINT16 kidGetSymmetry( const KID * pKid );
DECUMA_HWR_PRIVATE ROTATION_PTR kidGetRotation( const KID * pKid );
DECUMA_HWR_PRIVATE SYMBOL_INFORMATION_PTR  kidGetSymbolInfo( const KID * pKid );
DECUMA_HWR_PRIVATE int kidGetArcOrders(const KID * pKid, DECUMA_INT8_DB_PTR* ppArcOrders, int nMaxArcOrders);

DECUMA_HWR_PRIVATE CH_PAIR_ZOOM_INFO_PTR kidGetZoomInfo( const KID * pKid1,	const KID * pKid2,
		DECUMA_INT8 * pLikelyCandidateIfTestPasses);

DECUMA_HWR_PRIVATE int databaseHasWrongFormat(STATIC_DB_HEADER_PTR pStaticDB);

#define keyDBGetNoSymbolTypes( pKeyDB ) ( pKeyDB->nBaseSymbolTypes )


DECUMA_HWR_PRIVATE int keyDBGetNoKeys( KEY_DB_HEADER_PTR pKeyDB, int noArcs );
DECUMA_HWR_PRIVATE SYMBOL_TYPE_INFO_PTR keyDBGetStaticTypeTable(KEY_DB_HEADER_PTR pKeyDB);
DECUMA_HWR_PRIVATE CATEGORY_TABLE_PTR keyDBGetStaticCategoryTable(KEY_DB_HEADER_PTR pKeyDB);
DECUMA_HWR_PRIVATE SYMBOL_INFORMATION_PTR keyDBGetSymbolTable(KEY_DB_HEADER_PTR pKeyDB);
DECUMA_HWR_PRIVATE KEY_PTR keyDBGetKey( KEY_DB_HEADER_PTR pKeyDB, int nArcs, int nKeyIndex);
DECUMA_HWR_PRIVATE DECUMA_INT8_DB_PTR keyDBGetCurve( KEY_DB_HEADER_PTR pKeyDB,
								 int noArcs, int nCurveIndex);



DECUMA_HWR_PRIVATE DIAC_SYMBOL_INFO_PTR databaseGetStaticDiacSymbolTable(void);


DECUMA_HWR_PRIVATE PROPERTY_DB_HEADER_PTR staticDBGetPropDB(STATIC_DB_HEADER_PTR pDB);
DECUMA_HWR_PRIVATE KEY_DB_HEADER_PTR staticDBGetKeyDB(STATIC_DB_HEADER_PTR pDB);


#define propDBGetDistBase2Help( pPropDB ) ( (DECUMA_INT32) ((pPropDB)->distBaseLine2HelpLine) )
#define propDBGetNoDiacTypes( pPropDB ) ( (int) ((pPropDB)->nDiacTypes) )
#define propDBGetNoDiacSymbols( pPropDB ) ( (int) ((pPropDB)->nDiacSymbols) )

DECUMA_HWR_PRIVATE int propDBGetNoDiacKeys( PROPERTY_DB_HEADER_PTR pPropDB, int noArcs );
/* Returns the number of diacritic symbols in the database. */

DECUMA_HWR_PRIVATE DECUMA_UINT16 propDBGetSymmetry(PROPERTY_DB_HEADER_PTR pPropDB, int idx);
DECUMA_HWR_PRIVATE DIAC_SYMBOL_INFO_PTR propDBGetDiacSymbolTable(PROPERTY_DB_HEADER_PTR pPropDB);
DECUMA_HWR_PRIVATE ROTATION_PTR propDBGetRotation(PROPERTY_DB_HEADER_PTR pPropDB, int idx);
DECUMA_HWR_PRIVATE SMALL_KEY_PTR propDBGetSmallKey(PROPERTY_DB_HEADER_PTR pPropDB);

DECUMA_HWR_PRIVATE MIN_DISTANCES_PTR propDBGetMinDistances(PROPERTY_DB_HEADER_PTR pPropDB);
DECUMA_HWR_PRIVATE CH_PAIR_DIST_CAT_PTR propDBGetCharDistanceTable(PROPERTY_DB_HEADER_PTR pPropDB);

DECUMA_HWR_PRIVATE int propDBGetTypeConflicts(PROPERTY_DB_HEADER_PTR pPropDB,
						   TYPE_CONFLICTS_PTR* ppConflicts);
DECUMA_HWR_PRIVATE DECUMA_UINT16 propDBGetDiacriticMask( PROPERTY_DB_HEADER_PTR pPropDB,
									 int maskNr, int nDiacArcs );


DECUMA_HWR_PRIVATE int keyDBHasWrongFormat(KEY_DB_HEADER_PTR pKeyDB);
DECUMA_HWR_PRIVATE int propDBHasWrongFormat(PROPERTY_DB_HEADER_PTR pPropDB);

DECUMA_HWR_PRIVATE DECUMA_STATUS databaseIncludesSymbol(KEY_DB_HEADER_PTR pKeyDB,
									 PROPERTY_DB_HEADER_PTR pPropDB,
									 const DECUMA_CHARACTER_SET * pCharSet, 
									 const DECUMA_UNICODE * pSymbol,
									 int * pbIncluded);


DECUMA_HWR_PRIVATE int isSameDBSymbol(DECUMA_UNICODE_DB_PTR pDBSymbol1, DECUMA_UNICODE_DB_PTR pDBSymbol2);

DECUMA_HWR_PRIVATE int isSameSymbol(DECUMA_UNICODE_DB_PTR pDBSymbol, const DECUMA_UNICODE * pSymbol, int bToUpper);

#ifdef __cplusplus
} /*extern "C" { */
#endif

#endif
