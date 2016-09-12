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

#define CJK_DATABASE_C
#define CJK_SYSTEM_CODE

#include "cjkSession_Types.h"

#include "cjkDatabase.h"
#include "cjkDatabaseData.h"

#include "cjkDatabaseCoarse_Macros.h"
#include "cjkDatabaseFormat_Macros.h"

#ifdef CJK_ENABLE_INTERNAL_API
#include "cjkDatabase_Test.h"
#endif

#include "cjkCompressedCharacter_Macros.h"
#include "cjkDynamicDatabase_Macros.h"

#include "decumaLanguages.h"

#include "decumaInterrupts.h"

#include "decumaStatus.h"
#include "decumaCommon.h"
#include "decumaUnicodeMacros.h"

/* like &&= if it had existed */
#define AND_EQ(a, b) (a) = ((a) && (b))

/* 
 * There are some few-stroked characters already existing in GB
 * that are sometimes reused instead of more complicated ones.
 * We put these more complicated ones on second place in the candidate list
 * if the first candidate indicates that this could be the case.
 * Likewise, there are simplifications in Bigfive that also exist in
 * Bigfive.
 * Anyway, if the first candidate is a left side character below
 * then the right side character will appear as second candidate.
 */
static const CJK_UNICHAR simpreplace[] = {
	0x5170, 0x84DD,
	0x5170, 0x7BEE,
	0x5480, 0x5634,
	0x65E6, 0x86CB,
	0x5348, 0x821E,
	0x65E2, 0x66A8,
	0x90A6, 0x5E2E,
	0x4EC3, 0x505C,
	0x5C0F, 0x6653,
	0x4E8D, 0x8857,
	0x4ED8, 0x5085,
	0x4ED8, 0x8150,
	0x4ED8, 0x526F,
	0x67DA, 0x697C,
	0x4EDD, 0x540C,
	0x82B7, 0x85CF,
	0x753B, 0x5212,
	0x5212, 0x753B,
	0x953A, 0x949F,
	0x949F, 0x953A,
	0x9980, 0x4F59,
	0x4F59, 0x9980,
	0x9CB6, 0x9C87,
	0x9C87, 0x9CB6,
	0x78B1, 0x7877,
	0x7877, 0x78B1,
	0x9EBD, 0x4E48,
	0x4E48, 0x9EBD,
	0
};
static const CJK_UNICHAR tradreplace[] = {
	0x9EBC, 0x4E48,
	0x4E48 ,0x9EBC,
	0x81FA, 0x53F0,
	0x53F0, 0x81FA,
	0x4F53, 0x9AD4,
	0x540E, 0x5F86,
	0x79CD, 0x72AE,
	0x6E7E, 0x7063,
	0x673A, 0x6A5F,
	/* s->t1+t2 is done as s->t1 earlier t1->2 now: */
	0x500B, 0x7B87,
	0x885D, 0x6C96,
	0x9E75, 0x6EF7,
	0x6B77, 0x66C6,
	0x767C, 0x9AEE,
	0x58C7, 0x7F48,
	0x5864, 0x58CE,
	0x76E1, 0x5118,
	0x5F37, 0x5F4A,
	0x7576, 0x5679,
	0x532F, 0x5F59,
	0x6FD5, 0x6EBC,
	0x81DF, 0x9AD2,
	0x85E5, 0x846F,
	0x7372, 0x7A6B,
	0x8E7A, 0x8E7B,
	0x8DE1, 0x8E5F,
	0x9418, 0x937E,
	0x9B1A, 0x9808,
	0x9951, 0x98E2,
	0
};
 

DECUMA_UINT32 cjkDbGetSessionSize(void)
{
	return sizeof(CJK_DB_SESSION) + cjkCompressedCharacterGetSize();
}

DECUMA_STATUS cjkDbGetLanguage(DECUMA_STATIC_DB_PTR pDbBase, DECUMA_UINT32 * pLang)
{
	DECUMA_UINT32 dbIdentifier;
	DECUMA_UINT8 dbLang;

	if (pLang == NULL) return decumaNullPointer;
	if (pDbBase == NULL) return decumaNullDatabasePointer;

	dbIdentifier = *((DECUMA_UINT32 *) pDbBase);
	dbLang = dbIdentifier & 0xFF;

	switch (dbLang)
	{
	case 'S':
		*pLang = DECUMA_LANG_PRC;
		break;
	case 'T':
		*pLang = DECUMA_LANG_TW;
		break;
	case 'H':
		*pLang = DECUMA_LANG_HK;
		break;
	case 'J':
		*pLang = DECUMA_LANG_JP;
		break;
	case 'K':
		*pLang = DECUMA_LANG_KO;
		break;
	default:
		return decumaInvalidDatabase;
	}

	return decumaNoError;
}

DECUMA_STATUS cjkDbIsValid(DECUMA_STATIC_DB_PTR pDbBase)
{
	DECUMA_UINT32 dbIdentifier;
	DECUMA_UINT8 dbType, dbLang, dbVersion, dbSubVersion;

	if (pDbBase == NULL) return decumaNullDatabasePointer;

	dbIdentifier = *((DECUMA_UINT32 *) pDbBase);
	dbType = (dbIdentifier >> 24) & 0xFF;
	dbLang = dbIdentifier & 0xFF;

	/* Endianness and validity check */
	if (!(dbType == 's' || dbType == 'S' || dbType == 'f' || dbType == 'F') ||
		!(dbLang == 'S' || dbLang == 'T' || dbLang == 'H' || dbLang == 'P' || dbLang == 'J' || dbLang == 'K'))
	{
		return decumaInvalidDatabase;
	}

	/* Version number check */
	dbVersion = (dbIdentifier >> 16) & 0xFF;
	dbSubVersion = (dbIdentifier >> 8) & 0xFF;
	if ( dbVersion != (DB_VERSION    + '0') ||
		 dbSubVersion != (DB_SUBVERSION + '0'))
	{
		return decumaInvalidDatabaseVersion;
	}

	return decumaNoError;
}

#ifdef EXTENDED_DATABASE
static DECUMA_STATUS dbInitFeatures(DLTDB * pDb, DLTDB_NAMED_INDEX_NODE * pFeatures, DLTDB_FEATURE_TABLE * pTable)
{
	DECUMA_UINT32 * pFeatureCount;
	pFeatureCount  = dbGetNamedNode(pDb, pFeatures, DBNODENAME_FEATURE_BYTES_PER_FEATURE,  0);

	if (pFeatureCount) {
		pTable->nFeatureCount = pFeatureCount[0];
		pTable->pKeyIndex     = dbGetNamedNode(pDb, pFeatures, DBNODENAME_FEATURE_KEY_INDEX, 0);
	}

	return decumaNoError;
}
#endif

static DECUMA_STATUS dbInitClusterTree(DLTDB                     * pDb, 
									   DLTDB_NAMED_INDEX_NODE    * pClusterTreeIndex, 
									   DLTDB_COARSE_CLUSTER_TREE * pClusterTreeTable, 
									   int                       * pbHaveNecessaryTables)
{
	DECUMA_UINT8 * pTmp;
	int            bFound = 0;

	pTmp = dbGetNamedNode(pDb, pClusterTreeIndex, DBNODENAME_CLUSTER_TYPE, &bFound); /* AND_EQ(*pbHaveNecessaryTables, bFound); */

	decumaAssert( bFound != 0 && pTmp != NULL );
	pClusterTreeTable->clusterType = pTmp[0];

	if (pClusterTreeTable->clusterType == DLTDB_COARSE_CLUSTER_TREE_TYPE_UNKNOWN) {
#ifdef EXTENDED_DATABASE
		*pbHaveNecessaryTables = 1;
		return decumaNoError;
#else
		*pbHaveNecessaryTables = 0;
		return decumaInvalidDatabase;
#endif
	}

	if (pClusterTreeTable->clusterType == DLTDB_COARSE_CLUSTER_TREE_TYPE_DCS) {
		pClusterTreeTable->pDCSClusters           = dbGetNamedNode(pDb, pClusterTreeIndex, DBNODENAME_DCS_CLUSTERS,           &bFound); AND_EQ(*pbHaveNecessaryTables, bFound);
		pClusterTreeTable->pClusterParentIndices  = dbGetNamedNode(pDb, pClusterTreeIndex, DBNODENAME_CLUSTER_PARENT_INDICES, NULL); /*  &bFound); AND_EQ(*pbHaveNecessaryTables, bFound); */
		pClusterTreeTable->pClusterChildIndices   = dbGetNamedNode(pDb, pClusterTreeIndex, DBNODENAME_CLUSTER_CHILD_INDICES,  NULL); /* &bFound); AND_EQ(*pbHaveNecessaryTables, bFound); */
		pClusterTreeTable->pNChildrenUpToCluster  = dbGetNamedNode(pDb, pClusterTreeIndex, DBNODENAME_NCHILDREN_UPTO_CLUSTER, &bFound); AND_EQ(*pbHaveNecessaryTables, bFound);
	}
	else {
		pClusterTreeTable->pClusterRepresentative = NULL;
		pClusterTreeTable->pClusters = dbGetNamedNode(pDb, pClusterTreeIndex, DBNODENAME_CLUSTERS, &bFound); AND_EQ(*pbHaveNecessaryTables, bFound);

		pTmp = dbGetNamedNode(pDb, pClusterTreeIndex, DBNODENAME_CLUSTER_INDEX_LIST_LIMIT, &bFound);
	}

	return *pbHaveNecessaryTables ? decumaNoError : decumaInvalidDatabase;
}


DECUMA_STATUS cjkDbInit(DLTDB * pDb, DECUMA_STATIC_DB_PTR pDbBase)
{
	DLTDB_NAMED_INDEX_NODE * pTmp;
	DECUMA_STATUS            status;

	int bFound               = 0;
	int bHaveConstants       = 1;
	int bHaveNecessaryTables = 1;


	decumaAssert(pDb);
	
	status = cjkDbIsValid(pDbBase);

	if (status != decumaNoError) {
		return status;
	}


	/* ----- Base & Index Nodes ----- */

	pDb->pBase    = (DECUMA_UINT8 *) pDbBase;
	pDb->pDbIndex = (DLTDB_NAMED_INDEX_NODE *)(pDb->pBase + 4);


	/* ----- Constants ----- */

	pDb->pCopyright       = dbGetNamedNode(pDb, pDb->pDbIndex, DBNODENAME_COPYRIGHT,   &bFound); AND_EQ(bHaveNecessaryTables, bFound);
	pDb->pVersion         = dbGetNamedNode(pDb, pDb->pDbIndex, DBNODENAME_VERSION,     &bFound); AND_EQ(bHaveNecessaryTables, bFound);
	pDb->pLabel           = dbGetNamedNode(pDb, pDb->pDbIndex, DBNODENAME_LABEL,       &bFound); AND_EQ(bHaveNecessaryTables, bFound);
	pDb->pConstants       = dbGetNamedNode(pDb, pDb->pDbIndex, DBNODENAME_CONSTANTS,   &bFound); AND_EQ(bHaveNecessaryTables, bFound);
	pDb->pVersionStr      = dbGetNamedNode(pDb, pDb->pDbIndex, DBNODENAME_VERSION_STR, &bFound); AND_EQ(bHaveNecessaryTables, bFound);
	pDb->language          = *((DECUMA_UINT32 *)pDb->pBase) & 0xFF;
	pDb->maxnstr           = dbGetNamedUI32NodeEntry(pDb->pConstants, DBNODENAME_MAXNSTRK,          &bFound); AND_EQ(bHaveConstants, bFound);
	pDb->maxnptsperchar    = dbGetNamedUI32NodeEntry(pDb->pConstants, DBNODENAME_MAXNPTSPERCHAR,    &bFound); AND_EQ(bHaveConstants, bFound);
	pDb->maxnptsperstr     = dbGetNamedUI32NodeEntry(pDb->pConstants, DBNODENAME_MAXNPTSPERSTROKE,  &bFound); AND_EQ(bHaveConstants, bFound);
	pDb->firsttwobyteindex = dbGetNamedUI32NodeEntry(pDb->pConstants, DBNODENAME_FIRSTTWOBYTEINDEX, &bFound); AND_EQ(bHaveConstants, bFound);
	pDb->latinlimit        = dbGetNamedUI32NodeEntry(pDb->pConstants, DBNODENAME_LATINLIMIT,        &bFound); AND_EQ(bHaveConstants, bFound);
	pDb->categoryMask      = dbGetNamedUI32NodeEntry(pDb->pConstants, DBNODENAME_CATEGORYMASK,      &bFound); AND_EQ(bHaveConstants, bFound);
	pDb->writingStyleMask  = dbGetNamedUI32NodeEntry(pDb->pConstants, DBNODENAME_WRITINGSTYLE_MASK, &bFound); AND_EQ(bHaveConstants, bFound);
	pDb->nLengthLimit      = dbGetNamedUI32NodeEntry(pDb->pConstants, DBNODENAME_LENGTH_LIMIT,      &bFound); AND_EQ(bHaveConstants, bFound);
	pDb->nUnicodes         = dbGetNamedUI32NodeEntry(pDb->pConstants, DBNODENAME_N_UNICODES_IN_DB,  &bFound); AND_EQ(bHaveConstants, bFound);
	pDb->nKeys	           = dbGetNamedUI32NodeEntry(pDb->pConstants, DBNODENAME_N_KEYS_IN_DB,  &bFound); AND_EQ(bHaveConstants, bFound);

	{
		KNOWN_COMPONENTS * pKc = &pDb->knownComponents;
		DECUMA_UINT32 c;
		c = dbGetNamedUI32NodeEntry(pDb->pConstants, DBNODENAME_EARTH_L,  &bFound);
		pKc->earth_l        = (bFound) ? (DECUMA_UINT16)c : -1;
		c = dbGetNamedUI32NodeEntry(pDb->pConstants, DBNODENAME_SPEAKINGS_L,  &bFound);
		pKc->speakingS_l    = (bFound) ? (DECUMA_UINT16)c : -1;
		c = dbGetNamedUI32NodeEntry(pDb->pConstants, DBNODENAME_SPEAKINGS_L2,  &bFound);
		pKc->speakingS_l2   = (bFound) ? (DECUMA_UINT16)c : -1;
		c = dbGetNamedUI32NodeEntry(pDb->pConstants, DBNODENAME_SPEAKINGS_L3,  &bFound);
		pKc->speakingS_l3   = (bFound) ? (DECUMA_UINT16)c : -1; 
		c = dbGetNamedUI32NodeEntry(pDb->pConstants, DBNODENAME_THREEDROPS_I, &bFound);
		pKc->threedrops_i   = (bFound) ? (DECUMA_UINT16)c : -1; 
		c = dbGetNamedUI32NodeEntry(pDb->pConstants, DBNODENAME_THREEDROPS_L, &bFound);
		pKc->threedrops_l   = (bFound) ? (DECUMA_UINT16)c : -1;
		c = dbGetNamedUI32NodeEntry(pDb->pConstants, DBNODENAME_THREEDROPS_L2, &bFound);
		pKc->threedrops_l2  = (bFound) ? (DECUMA_UINT16)c : -1;
		c = dbGetNamedUI32NodeEntry(pDb->pConstants, DBNODENAME_THREEDROPS_L3, &bFound);
		pKc->threedrops_l3  = (bFound) ? (DECUMA_UINT16)c : -1;
		c = dbGetNamedUI32NodeEntry(pDb->pConstants, DBNODENAME_THREEDROPS_LD, &bFound);
		pKc->threedrops_ld  = (bFound) ? (DECUMA_UINT16)c : -1;
		c = dbGetNamedUI32NodeEntry(pDb->pConstants, DBNODENAME_THREEDROPS_LD2, &bFound);
		pKc->threedrops_ld2 = (bFound) ? (DECUMA_UINT16)c : -1;
	}

	/* pDb->dense.nClusters   = dbGetNamedUI32NodeEntry(pDb->pConstants, DBNODENAME_N_DENSE_CLUSTERS,  &bFound); AND_EQ(bHaveConstants, bFound); */
	/* pDb->sparse.nClusters  = dbGetNamedUI32NodeEntry(pDb->pConstants, DBNODENAME_N_SPARSE_CLUSTERS, &bFound); AND_EQ(bHaveConstants, bFound); */


	if (!bHaveConstants) {
		return decumaInvalidDatabase;
	}


	/* ----- Remaining Index Node Entries ----- */

	pDb->pCompData        = dbGetNamedNode(pDb, pDb->pDbIndex, DBNODENAME_COMPONENTS,    &bFound); AND_EQ(bHaveNecessaryTables, bFound);
	pDb->pCharData        = dbGetNamedNode(pDb, pDb->pDbIndex, DBNODENAME_CHARACTERS,    &bFound); AND_EQ(bHaveNecessaryTables, bFound);

	pDb->pIndexlistOffsetEnd = dbGetNamedNode(pDb, pDb->pDbIndex, DBNODENAME_INDEXLIST_OFFSET_END, NULL);
	pDb->pIndex2Indexlists = dbGetNamedNode(pDb, pDb->pDbIndex, DBNODENAME_INDEXLIST_VECTOR, NULL);

	pDb->pCatAndAttribList = dbGetNamedNode(pDb, pDb->pDbIndex, DBNODENAME_CATANDATTRIB_LIST, &bFound); AND_EQ(bHaveNecessaryTables, bFound);
	pDb->pCategory        = dbGetNamedNode(pDb, pDb->pDbIndex, DBNODENAME_CATEGORY,      &bFound); AND_EQ(bHaveNecessaryTables, bFound);
	pDb->pWritingStyle    = dbGetNamedNode(pDb, pDb->pDbIndex, DBNODENAME_WRITING_STYLE, NULL);
	pDb->pUnicode         = dbGetNamedNode(pDb, pDb->pDbIndex, DBNODENAME_UNICODE,       &bFound); AND_EQ(bHaveNecessaryTables, bFound);
	pDb->pAttrib          = dbGetNamedNode(pDb, pDb->pDbIndex, DBNODENAME_ATTRIBUTE,     &bFound); AND_EQ(bHaveNecessaryTables, bFound);

#ifdef EXTENDED_DATABASE
	pDb->pStrokeOrder     = dbGetNamedNode(pDb, pDb->pDbIndex, DBNODENAME_STROKE_ORDER, NULL);
	pDb->pType            = dbGetNamedNode(pDb, pDb->pDbIndex, DBNODENAME_TYPE,         NULL);
#endif

	pDb->pUnicodeVector   = dbGetNamedNode(pDb, pDb->pDbIndex, DBNODENAME_UNICODE_VECTOR, &bFound); AND_EQ(bHaveNecessaryTables, bFound);
	pDb->pTrad2Simp       = dbGetNamedNode(pDb, pDb->pDbIndex, DBNODENAME_TRAD2SIMP,      NULL);
	pDb->pSimp2Trad       = dbGetNamedNode(pDb, pDb->pDbIndex, DBNODENAME_SIMP2TRAD,      NULL);



#ifdef EXTENDED_DATABASE
	pTmp = dbGetNamedNode(pDb, pDb->pDbIndex, DBNODENAME_DENSE_FEATURES, 0);
	if (pTmp) dbInitFeatures(pDb, pTmp, &pDb->denseFeatures);
	pTmp = dbGetNamedNode(pDb, pDb->pDbIndex, DBNODENAME_SPARSE_FEATURES, 0);
	if (pTmp) dbInitFeatures(pDb, pTmp, &pDb->sparseFeatures);
#endif

#ifdef EXTENDED_DATABASE
	pDb->pSamples = dbGetNamedNode(pDb, pDb->pDbIndex, DBNODENAME_SAMPLES, 0);
#endif

	pTmp = dbGetNamedNode(pDb, pDb->pDbIndex, DBNODENAME_DENSE_CLUSTER_TREE, &bFound);  AND_EQ(bHaveNecessaryTables, bFound);
	if (pTmp) dbInitClusterTree(pDb, pTmp, &pDb->dense, &bHaveNecessaryTables);


	pTmp = dbGetNamedNode(pDb, pDb->pDbIndex, DBNODENAME_SPARSE_CLUSTER_TREE, &bFound); AND_EQ(bHaveNecessaryTables, bFound);
	if (pTmp) dbInitClusterTree(pDb, pTmp, &pDb->sparse, &bHaveNecessaryTables);

	if (!bHaveNecessaryTables) {
		return decumaInvalidDatabase;
	}

	pDb->maxindex = pDb->pCharData->nElements - 1;

	return decumaNoError;

	#undef AND_EQ
}


/* TODO DLTDB const * const pDB ? */
DECUMA_INT32 cjkDbGetCategoryMask(const DLTDB * pDB)
{	
	decumaAssert(pDB);
	return pDB->categoryMask;
}


CJK_DISTANCE cjkDbGetDistanceToIndex(CJK_SESSION * pSession, CJK_COMPRESSED_CHAR * pCcharIn, DECUMA_UINT16 nIdx, int bDoubleBandwidth, int bFinal, int bUseFreq)
{
	DECUMA_UINT16 unicode;
	CJK_DB_SESSION * pDbSession = pSession->pDbSession;
	CJK_COMPRESSED_CHAR * pCCharDb = pDbSession->pCCharDb; /* Located in persistent memory */
	CJK_DISTANCE dist = LARGEDIST;

	dltCCCompressSetNull(pCCharDb);
	dltCCCompressSetCCData(pCCharDb, DLTDB_GET_CHARACTER_DATA(&pSession->db, nIdx));
	unicode         = DLTDB_GET_UNICODE(&pSession->db, nIdx);

	dltCCharPrecompute(pCCharDb, pSession);

	dltCCCompressSetAttribute(pCCharDb, DLTDB_GET_ATTRIBUTE(&pSession->db, nIdx));
	dltCCCompressSetIndex(pCCharDb, nIdx);

	/* Only use the computationally costly final version if necessary (zooming) */
	if ( bFinal ) {
		dist = dltCCharGetDistanceFinal(pCcharIn, pCCharDb, bUseFreq, bDoubleBandwidth, pSession);
	}
	else if ( dltCCCompressIsDense(pCcharIn) ) {
		/* Transferred from <<try this dense char>> */
		int delta = dltCCharGetDelta(pCcharIn, pCCharDb);
		dist = dltCCharGetRawDistance(pCcharIn, pCCharDb, delta, pSession);
	}
	else if ( dltCCCompressGetNbrStrokes(pCcharIn) <= dltCCCompressGetNbrStrokes(pCCharDb)+2 ) {
		/* Transferred from <<try this sparse char>> */
		int delta = dltCCharGetDelta(pCcharIn, pCCharDb);
		int punish = dltCCharGetPointDifferencePunishDistance(pCcharIn, pCCharDb); /* + dltCCharCompStartPunish(pCcharIn, &cCharDb, pSession); */
		int nDistLim = DECUMA_MIN(pSession->db_lookup_bl.dist[BESTLEN-1], LARGEDIST);

		if ( decumaIsHan(cjkDbUnicode(dltCCCompressGetIndex(pCCharDb), pSession)) || 
			decumaIsHangul(cjkDbUnicode(dltCCCompressGetIndex(pCCharDb), pSession)) || 
			dltCCCompressGetNbrStrokes(pCcharIn) > 6 ) {
				dist = dltCCharGetDTWDistance(pCcharIn, pCCharDb, delta, nDistLim-punish, 0, 0, 0, bDoubleBandwidth, pSession);
				dist += punish;
		}		
		if ( dltCCCompressGetNbrStrokes(pCcharIn) == dltCCCompressGetNbrStrokes(pCCharDb) ) {
			CJK_DISTANCE tdist = LARGEDIST;
			/* NOTE: Not else in orignal code - both inserted into bestlist */
			if ( dltCCCompressGetNbrStrokes(pCcharIn) <= 8 && dltCCCompressGetNbrStrokes(pCcharIn) >= 4 ) {
				tdist = dltCCharGetSOFDistance(pCcharIn, pCCharDb, delta, nDistLim-punish-20, pSession) + punish + 20;
			}
			else {
				tdist = dltCCharGetRawDistance(pCcharIn, pCCharDb, delta, pSession) + punish;
			}
			if (tdist < dist) dist = tdist;
		}
	}
	return dist;
}

DECUMA_STATUS cjkDbInitDbSession(CJK_DB_SESSION * pDbSession)
{
	/* Initialize dbsession, member allocated after the
	   session (which just holds the pointer) */
	pDbSession->pCCharDb = (CJK_COMPRESSED_CHAR *)(&pDbSession[1]);
	return decumaNoError;
}

/**
* The best key is the one that minimizes the euclidean distance from
* the written character to the key.
*/
DECUMA_STATUS 
cjkDbLookup(CJK_COMPRESSED_CHAR * const pCChar,
            CJK_BOOLEAN           const bUseFreq,
            CJK_SESSION         * const pSession,
			CJK_BESTLIST const ** const pBestList)
{
	CJK_BESTLIST * const bl  = &pSession->db_lookup_bl;
	CJK_CONTEXT * const con = &pSession->con;
	DLTDB_COARSE_CLUSTER_TREE * pClusterTree;
	DECUMA_UINT32 i;
	DECUMA_STATUS status = decumaNoError;
	DLTDB_COARSE_SEARCH_SESSION_DCS cSSessionDCS;
	DLTDB_COARSE_SEARCH_SESSION * pCSSession = (DLTDB_COARSE_SEARCH_SESSION *) &cSSessionDCS;
	CJK_DB_SESSION * pDbSession = pSession->pDbSession;
	
	decumaAssert(pBestList);
	decumaAssert(pSession);

	decumaAssert(pCChar != NULL);
	if (pCChar == NULL)
	{
		status = decumaNullPointer;
		goto db_lookup_done;
	}

	if (dltCCCompressGetNbrStrokes(pCChar) == 0)
		return decumaCurveZeroArcs;

	/* Initialize */
	status = dltCCharSetFeatureBits(pCChar, pSession);

	if (status != decumaNoError) goto db_lookup_done;

	/* For the certification only. */
#if defined(CERTIFICATION)
	/*printf("\n------------ %d %d %d %d\n", pSession->boxheight, pSession->boxwidth, */
	/*pCChar->ymin, pCChar->ymax); */
	if (pSession->boxheight == 0 && pSession->boxwidth == 0 &&
		pCChar->ymin >= 420 && pCChar->ymax - pCChar->ymin <= 150) {
			pSession->boxybase = 434;
			pSession->boxheight = 143;
			pSession->boxxbase = 93;
			pSession->boxwidth = 130;
	}
	/*printf("\n============ %d %d %d %d\n", pSession->boxheight, pSession->boxwidth, */
	/*pCChar->ymin, pCChar->ymax); */
#endif

	cjkBestListInit(&pSession->db_lookup_bl);

	/* -----------------------------------------------------
	 * save previous interpretation
	 * 
	 * The first element in bestlist is saved in the session object, to
	 * constitute pCChar hint for the interpretation of the next character.
	 * This makes it easier to distinguish between i.e. "2" and "Z".
	 * If the previous character was pCChar digit, the interpretation is "2",
	 * else if the previous character was pCChar latin character, the
	 * interpretation is "Z". There are some Chinese characters that
	 * easily can be mistaken for pCChar digit or latin character, too.

	* -----------------------------------------------------
	 * update quotation counters
	 * 
	 * A variable is set to keep the information if pCChar quotation mark is
	 * recently written. In that case, pCChar right quotation mark will be the
	 * first proposal next time pCChar quotation mark is written. There is pCChar
	 * limit [[MAX_QUOTATION_LENGTH]] for how many characters to interpret
	 * between two quotation marks, i.e. the quotation variable will be reset
	 * after some characters are written.
	*/

	if ((cjkContextGetDoubleQuoteCount(con) > 0) &&
		(cjkContextGetDoubleQuoteCount(con) < MAX_QUOTATION_LENGTH)) {
			cjkContextIncDoubleQuoteCount(con);
	}
	if ((cjkContextGetSingleQuoteCount(con) > 0) &&
		(cjkContextGetSingleQuoteCount(con) < MAX_QUOTATION_LENGTH)) {
			cjkContextIncSingleQuoteCount(con);
	}
	if (cjkContextGetPrevious(con) == UC_LEFT_DOUBLE_QUOTATION_MARK) {
		cjkContextSetDoubleQuoteCount(con);
		cjkContextResetSingleQuoteCount(con);
	}
	else if (cjkContextGetPrevious(con) == UC_LEFT_SINGLE_QUOTATION_MARK) {
		cjkContextSetSingleQuoteCount(con);
	}
	else if (cjkContextGetPrevious(con) == UC_RIGHT_SINGLE_QUOTATION_MARK) {
		cjkContextResetSingleQuoteCount(con);
	}
	else if ((cjkContextGetPrevious(con) == UC_RIGHT_DOUBLE_QUOTATION_MARK) ||
		(cjkContextGetDoubleQuoteCount(con) >= MAX_QUOTATION_LENGTH) ||
		(cjkContextGetSingleQuoteCount(con) >= MAX_QUOTATION_LENGTH)) {
			cjkContextResetDoubleQuoteCount(con);
			cjkContextResetSingleQuoteCount(con);
	}

	pClusterTree = dltCCCompressIsDense(pCChar) ?
		&pSession->db.dense :
		&pSession->db.sparse;

    /* Initialize for coarse search */
	pCSSession->type         = pClusterTree->clusterType;
	pCSSession->pSession     = pSession;
	pCSSession->pClusterTree = pClusterTree;
	pCSSession->pCChar       = pCChar;
	pCSSession->pResult      = &pSession->clusterResult;
	pCSSession->pResult->nIndices = 0;

	if (TEST_ABORT_RECOGNITION(pSession->pInterruptFunctions)) {
		return decumaRecognitionAborted;
	}

	status = cjkDbCoarseBeamSearch(pCSSession);

	if (status != decumaNoError || (bl->unichar[0] == 0))
		goto db_lookup_done;

	/* Now do the final bestlist Zoom */
	status = cjkBestListZoom(pCChar, pSession, bUseFreq);

	if (status != decumaNoError)
		goto db_lookup_done;

#if defined(DECUMA_ASSERT_ENABLE) || defined(DECUMA_ASSERT_OVERRIDE)
	{
		/* If we have enabled asserts, assert that no entry in the best-list has a database index that trespasses into the personal character database */
		int n;
		for ( n = 0; n < BESTLEN; n++ )
		{
			decumaAssert(bl->index[n] < DYNAMIC_DB_STARTINDEX);
		}
	}
#endif

	/* if (pSession->global_usepersonalchar) { */
	if ( pSession->pSessionSettings->pDynamicDB )
	{
		/*----------------------------------------------------- */
		/* lookup personal characters */
		/*  */
		/* If the flag [[usePersonalChar]] is set, then all the personal characters are */
		/* compared to the written character. */
		
		DECUMA_DYNAMIC_DB_PTR pDB = pSession->pSessionSettings->pDynamicDB;

		decumaAssert(cjkValidateDynamicDatabase(pDB) == decumaNoError);

		for ( i = 0; i < pDB->nEntries; i++ ) {
			CJK_PECDATA const * const pCurrent = &pDB->entries[i];

			if ( (pCurrent->categoryMask & pSession->sessionCategories) == 0 )
				continue;

			if (dltCCCompressIsDense(pCChar) == pCurrent->attrib) {
				CJK_DISTANCE pecdist;
				CJK_COMPRESSED_CHAR * pb = pDbSession->pCCharDb;

				dltCCCompressSetIndex(pb, DYNAMIC_DB_STARTINDEX + i); /* Need to set index in order to skip frequency lookup inside dltCCharPreCompute() */
				dltCCCompressSetAttribute(pb, pCurrent->attrib);
				dltCCCompressSetCCData(pb, (CJK_COMPRESSED_CHAR_DATA *) &pCurrent->ccData[0]);
				dltCCharPrecompute(pb, pSession);

				pecdist = dltCCharGetDistanceFinal(pCChar, pb, bUseFreq, 0, pSession);

				cjkBestListInsert(bl, pCurrent->unicode, DYNAMIC_DB_STARTINDEX + i, pecdist, pSession);
			}
		}
	}

	for (i = 0; bl->unichar[i] != 0 && i < BESTLEN; i++) {
		if (bl->dist[i] >=LARGEDIST) {
			bl->unichar[i] = 0;
		}
	}

	if (bl->unichar[0] == 0) /* bestlist is empty */
		goto db_lookup_done;


	/*-----------------------------------------------------
	 * special fixes
	 * 
	 * We separate similarly looking characters with the help of the
	 * shape of the input character and the type of previous character.
	 * If pCChar character is very similar to others, these unicodes are forced
	 * into bestlist.
	 * Especially this function is useful to force punctuation
	 * symbols into bestlist, since there are no small punctuations in
	 * the database.
	 */

	pSession->state = STATE_SPECIAL_CHECKS;
	
	if (bl->index[0] < DYNAMIC_DB_STARTINDEX) {
		cjkBestListPreCheck(pCChar, pSession);
		cjkBestListSpecialCheck(pCChar, pSession);
		cjkBestListThreeDropsCheck(pCChar, pSession);
	}
	cjkBestListCapsCheck(pCChar, pSession);
	cjkBestListFinalCheck(pSession);
	
	pSession->state = STATE_NORMAL;

	/*-----------------------------------------------------
	 * add phonetic simplifications
	 * 
	 * There are two characters that show up more
	 * than once on the left side in the above list.
	 * Therefore we can not break the loop at pCChar hit.
	 * Instead we continure to insert at place two so
	 * that if there are three simplifications using the
	 * same character (0x4ED8), the corresponding three
	 * possible sources will show up in the candidate list
	 * in places 2, 3 and 4.
	 */
	if (DLTDB_IS_SIMP(pSession->db)) { /* Simplified DB */
		if ((pSession->sessionCategories & (CJK_GB2312_A | CJK_GB2312_B))
			&& !(pSession->sessionCategories & CJK_BIGFIVE)) {
				CJK_UNICHAR uc = bl->unichar[0];
				i = 0;
				while (simpreplace[i] != 0) {
					if (uc == simpreplace[i]) {
						cjkBestListInsertAt(simpreplace[i + 1], 0, 1, pSession);
					}
					i += 2;
				}
		}
	}

	if (DLTDB_IS_TRAD(pSession->db)) { /* Traditional DB */
		if (!(pSession->sessionCategories & (CJK_GB2312_A | CJK_GB2312_B))
			&& (pSession->sessionCategories & CJK_BIGFIVE)) {
				CJK_UNICHAR uc = bl->unichar[0];
				i = 0;
				while (tradreplace[i] != 0) {
					if (uc == tradreplace[i]) {
						cjkBestListInsertAt(tradreplace[i + 1], 0, 1, pSession);
					}
					i += 2;
				}
		}
	}


db_lookup_done:

	if ( status == decumaNoError ) 
		*pBestList = bl;
	cjkCompressedCharacterSetDTWCached(pCChar,0);

	return status;
}

void cjkDbGetCCharByIndex(DLTDB_INDEX           index,
						CJK_COMPRESSED_CHAR * pCChar,
						CJK_SESSION         * pSession)
{

	/* check index sanity
	 * 
	 * If called with invalid index we simply fill the character with zeros and
	 * return pCChar null pointer.
	 */
	if (index == 0 || index > pSession->db.maxindex) {
		dltCCCompressSetNull(pCChar);
		return;
	}

	dltCCCompressSetCCData(pCChar, DLTDB_GET_CHARACTER_DATA(&pSession->db, index));
	dltCCCompressSetAttribute(pCChar, DLTDB_GET_ATTRIBUTE(&pSession->db, index));
	dltCCCompressSetIndex(pCChar, index);

	dltCCharPrecompute(pCChar, pSession);

	return;
}


/* This function finds the index of the first database 
 * character that has the wanted unicode. If there is no such
 * character the return value is 0. Warning: uses linear search, use carefully
 * (takes time)!
 */
DLTDB_INDEX cjkDbGetIndexByUnicode(CJK_UNICHAR u, CJK_SESSION * pSession) 
{
	DLTDB_INDEX i;
	for (i = 0; i <= pSession->db.maxindex; i++) {
		if (DLTDB_GET_UNICODE(&pSession->db, i) == u) 
			return i;
	}
	return 0;
}





/* Takes a unicode and returns the first(!) character from the database.
 * WARNING uses [[cjkDbGetIndexByUnicode]] (that uses linear search), use carefully.
 * 
 * this function is only used by dlttest,c, don't include in release.
 */
#ifdef DEBUG
void cjkDbGetCCharByUnicode(CJK_UNICHAR           u, 
						  CJK_COMPRESSED_CHAR * pCChar, 
						  CJK_SESSION         * pSession)
{
	DLTDB_INDEX i = cjkDbGetIndexByUnicode(u, pSession);
	if (i) {
		cjkDbGetCCharByIndex(i, pCChar, pSession);
		return;
	}

	dltCCCompressSetNull(pCChar);
	return;

}
#endif



/* An index is mapped in the database to a Unicode through [[cjkDbUnicode]]. */
CJK_UNICHAR cjkDbUnicode(DLTDB_INDEX i, CJK_SESSION * pSession) 
{
	CJK_UNICHAR u = 0;

	if (i >= DYNAMIC_DB_STARTINDEX) {
		DECUMA_DYNAMIC_DB_PTR const pDB = pSession->pSessionSettings->pDynamicDB;

		decumaAssert(cjkValidateDynamicDatabase(pDB) == decumaNoError);
		
		if ( i < pDB->nEntries + DYNAMIC_DB_STARTINDEX )
		{
			u = pDB->entries[i - DYNAMIC_DB_STARTINDEX].unicode;
		}

	} else {
		u = DLTDB_GET_UNICODE(&pSession->db, i); /*pSession->db.pUnicode[i]; */
	}

	return u;
}

/**
 * Returns the a boolean if index is in the personal category array.
 */

DECUMA_UINT32 cjkDbIsInPersonalCategory(CJK_SESSION * pSession, DLTDB_INDEX index) 
{
	if (pSession->nPersonalCategories > 0) {
		int bitx = DLTDB_GET_BIT(pSession->pPersonalCategories, index);
		return bitx;
	}
	return 0;
}


/** 
 * If both simplified and traditional is compiled into the engine and the
 * database, then we need to map the traditional characters to
 * the simplified ones. In case only simplified was compiled then we do
 * not need this function since the traditional characters got their
 * simplified Unicodes from the database generating program -- and the
 * translation table is not present in order to save 10 kilobytes of space.
 */
typedef CJK_UNICHAR (*CJK_TRAD2SIMP)[2];

CJK_UNICHAR cjkDbUCHan2Han(CJK_UNICHAR const uc, CJK_SESSION const * const pSession)
{
	DECUMA_UINT32 const categorymask = pSession->sessionCategories;
	DECUMA_UINT32 nelems;
	CJK_TRAD2SIMP table;
	DECUMA_INT32 low, mid, high, cmp;

	/* check if it is not a unihan */
	if (uc < CJK_UNIFIED_FIRST || CJK_UNIFIED_LAST < uc) {
		return uc;
	}

	/* check category flags */
	if ((categorymask & CJK_GB2312)
		&& !(categorymask & CJK_BIGFIVE)) {
			table = (CJK_TRAD2SIMP) pSession->db.pTrad2Simp;
			nelems = NTRAD2SIMP;
	}
	else if (categorymask & CJK_BIGFIVE
		&& !(categorymask & CJK_GB2312)) {
			table = (CJK_TRAD2SIMP) pSession->db.pSimp2Trad;
			nelems = NSIMP2TRAD;
	}
	else {
		return uc;
	}



	/* search for a mapping */
	low = 0;
	high = nelems - 1;
	while (low <= high) {
		mid = (low + high) / 2;
		cmp = uc - table[mid][0];
		if (cmp < 0) {
			high = mid - 1;
		}
		else if (cmp > 0) {
			low = mid + 1;
		}
		else {
			return table[mid][1];
		}
	}

	return uc;
}

DECUMA_STATUS cjkDbInitDefault(DLTDB * pDb)
{
	if ( pDb == NULL )
		return decumaNullPointer;

	pDb->maxnstr = MAXNSTRK;
	pDb->maxindex = 0;
	pDb->maxnptsperchar = MAXPNTSPERCHAR;
	pDb->maxnptsperstr = MAXPNTSPERSTR;
	pDb->firsttwobyteindex = FIRSTTWOBYTEINDEX;
	pDb->latinlimit = LATINLIMIT;
	pDb->categoryMask =  0;	
	pDb->nLengthLimit = LENGTHLIMIT;
	
	return decumaNoError;
}
