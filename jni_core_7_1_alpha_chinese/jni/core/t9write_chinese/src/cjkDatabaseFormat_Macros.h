/*****************************************************************************
* COPYRIGHT ZI AND SUBJECT TO CONFIDENTIALITY RESTRICTIONS                   *
*                                                                            *
* This file is the confidential and proprietary property of Zi Corporation   *
* of Canada, Inc. ("Zi") or one of its affiliates.                           *
******************************************************************************/
#ifndef CJK_DATABASE_FORMAT_MACROS_H_
#define CJK_DATABASE_FORMAT_MACROS_H_

#pragma once

#include "cjkCommon.h"
#include "cjkDatabaseFormat.h"
#include "cjkDatabaseFormat_Types.h"

#include "decumaBasicTypes.h"

/** @addtogroup DOX_CJK_DATABASE_FORMAT
  * @{ */

/** @name Database Node Data Accessors
 *  @{ 
 */

/** @name Cluster accessors 
 *  @{ 
 */
#define DLTDB_CLUSTER_GET_KEY_INDEX_TBL(m_pDb, m_pTree, m_nIdx)     ((DLTDB_UI16_NODE *)dbGetIndexNodeEntry(m_pDb, m_pTree->pClusters, m_nIdx))
#define DLTDB_CLUSTER_GET_REPRESENTATIVE_IDX(  m_pTree, m_nIdx)     ((m_pTree)->pClusterRepresentative[(m_nIdx)])
#define DLTDB_CLUSTER_GET_FIRST_FEATURE(       m_pTree, m_nIdx)     ((m_nIdx) ? (m_pTree)->pNFeaturesUpToCluster[(m_nIdx) - 1] : 0)
#define DLTDB_CLUSTER_GET_LAST_FEATURE(        m_pTree, m_nIdx)      (m_pTree->pNFeaturesUpToCluster[(m_nIdx)] - 1)
#define DLTDB_CLUSTER_GET_FEATURE_INDEX(       m_pTree, m_nFeature) ((m_pTree)->pClusterFeatureIndices[(m_nFeature)])
#define DLTDB_CLUSTER_GET_FEATURE_VALUE(       m_pTree, m_nFeature) ((m_pTree)->pClusterFeatureValues[(m_nFeature)])

#define DLTDB_CLUSTER_GET_CHOSEN_FEATURE_COUNT(m_pTree)             ((m_pTree)->pClusterChosenFeature.nElements)
#define DLTDB_CLUSTER_GET_CHOSEN_FEATURE(      m_pTree, m_nIdx)     ((m_pTree)->pClusterChosenFeature.pElements[(m_nIdx)])

#define DLTDB_CLUSTER_GET_PARENT_INDEX(        m_pTree, m_nIdx)     ((m_pTree)->pClusterParentIndices[(m_nIdx)])
#define DLTDB_CLUSTER_GET_FIRST_CHILD(         m_pTree, m_nIdx)     ((m_nIdx) ? (m_pTree)->pNChildrenUpToCluster[(m_nIdx) - 1] : 0)
#define DLTDB_CLUSTER_GET_LAST_CHILD(          m_pTree, m_nIdx)      (m_pTree->pNChildrenUpToCluster[(m_nIdx)] - 1)
#define DLTDB_CLUSTER_GET_CHILD_INDEX(         m_pTree, m_nChild)   ((m_pTree)->pClusterChildIndices[(m_nChild)])
/** @} */


/** @name DCS Cluster accessors 
 *  @{ 
 */
#define DLTDB_DCSCLUSTER_GET_CLUSTER(             m_pDb, m_pTree, m_nIdx) ((DLTDB_NAMED_INDEX_NODE *)dbGetIndexNodeEntry(m_pDb, m_pTree->pDCSClusters, m_nIdx))
#define DLTDB_DCSCLUSTER_GET_KEY_INDEX_TBL(       m_pDb, m_pClusterNode)  ((DLTDB_UI16_NODE        *)dbGetNamedNode(m_pDb, m_pClusterNode, DBNODENAME_CLUSTER_KEY_INDICES, NULL))
#define DLTDB_DCSCLUSTER_GET_CHOSEN_FEATURES_TBL( m_pDb, m_pClusterNode)  ((DLTDB_UI8_NODE         *)dbGetNamedNode(m_pDb, m_pClusterNode, DBNODENAME_CLUSTER_CHOSEN_FEATURES, NULL))
#define DLTDB_DCSCLUSTER_GET_FEATURE_REP_TBL(     m_pDb, m_pClusterNode)  ((DLTDB_UI8_NODE         *)dbGetNamedNode(m_pDb, m_pClusterNode, DBNODENAME_CLUSTER_FEATURE_REP, NULL))
#if defined(USE_CLUSTER_BANDWIDTH_FILTER)
#define DLTDB_DCSCLUSTER_GET_KEY_INDEX_TBL_AUX(m_pDb, m_pClusterNode)  ((DLTDB_UI16_NODE        *)dbGetNamedNode(m_pDb, m_pClusterNode, DBNODENAME_CLUSTER_KEY_INDICES_TBL, NULL))
#endif

#define DLTDB_DCSCLUSTER_GET_FEATURE_MEANS_TBL(   m_pDb, m_pClusterNode)  ((DLTDB_UI16_NODE *)dbGetNamedNode(m_pDb, m_pClusterNode, DBNODENAME_CLUSTER_FEATURE_MEANS, NULL))
#define DLTDB_DCSCLUSTER_GET_FEATURE_MEAN(        m_pDb, m_pClusterNode, m_nFeatIdx)  \\
((double)(DLTDB_DCSCLUSTER_GET_FEATURE_MEANS_TBL((m_pDb), (m_pClusterNode))->pElements[(m_nFeatIdx) * 2]) / \\
          DLTDB_DCSCLUSTER_GET_FEATURE_MEANS_TBL((m_pDb), (m_pClusterNode))->pElements[(m_nFeatIdx) * 2 + 1])

#define DLTDB_DCSCLUSTER_GET_FEATURE_STD_TBL(     m_pDb, m_pClusterNode)  ((DLTDB_UI16_NODE *)dbGetNamedNode(m_pDb, m_pClusterNode, DBNODENAME_CLUSTER_FEATURE_STD, NULL))
#define DLTDB_DCSCLUSTER_GET_FEATURE_STD(        m_pDb, m_pClusterNode, m_nFeatIdx)  ((double)(DLTDB_DCSCLUSTER_GET_FEATURE_STD_TBL((m_pDb), (m_pClusterNode))->pElements[(m_nFeatIdx) * 2]) / DLTDB_DCSCLUSTER_GET_FEATURE_STD_TBL((m_pDb), (m_pClusterNode))->pElements[(m_nFeatIdx) * 2 + 1])
/* #define DLTDB_DCSCLUSTER_GET_FEATURE_STD(         m_pDb, m_pClusterNode, m_nFeatIdx)  (double)((DLTDB_DCSCLUSTER_GET_FEATURE_STD_TBL((m_pDb), (m_pClusterNode))->pElements[(m_nFeatIdx)]) / 255.0) */

#define DLTDB_DCSCLUSTER_GET_FEATURE_WEIGHT_TBL(  m_pDb, m_pClusterNode)  ((DLTDB_UI16_NODE *)dbGetNamedNode(m_pDb, m_pClusterNode, DBNODENAME_CLUSTER_FEATURE_WEIGHT, NULL))
#define DLTDB_DCSCLUSTER_GET_FEATURE_WEIGHT(        m_pDb, m_pClusterNode, m_nFeatIdx)  ((double)(DLTDB_DCSCLUSTER_GET_FEATURE_WEIGHT_TBL((m_pDb), (m_pClusterNode))->pElements[(m_nFeatIdx) * 2]) / DLTDB_DCSCLUSTER_GET_FEATURE_WEIGHT_TBL((m_pDb), (m_pClusterNode))->pElements[(m_nFeatIdx) * 2 + 1])
/* #define DLTDB_DCSCLUSTER_GET_FEATURE_WEIGHT(      m_pDb, m_pClusterNode, m_nFeatIdx)  (double)((DLTDB_DCSCLUSTER_GET_FEATURE_WEIGHT_TBL((m_pDb), (m_pClusterNode))->pElements[(m_nFeatIdx)]) / 255.0) */


#define DLTDB_DCSCLUSTER_GET_REP_IDX(m_pDb,  m_pClusterNode)          (DLTDB_DCSCLUSTER_GET_CONSTANT(m_pDb, m_pClusterNode, "REP_IDX"))
#define DLTDB_DCSCLUSTER_GET_REP_COMPARE_NBR(m_pDb,  m_pClusterNode)  (DLTDB_DCSCLUSTER_GET_CONSTANT(m_pDb, m_pClusterNode, "COMPARE_NBR"))
/** @} */


/** @name Other accessors 
 *  @{ 
 */
#define DLTDB_GET_COMPONENT_DATA(m_pDb, m_nIdx)  ((CJK_COMPRESSED_CHAR_DATA *)dbGetIndexNodeEntry((m_pDb), (m_pDb)->pCompData, (m_nIdx)))
#define DLTDB_GET_CHARACTER_DATA(m_pDb, m_nIdx)  ((CJK_COMPRESSED_CHAR_DATA *)dbGetIndexNodeEntry((m_pDb), (m_pDb)->pCharData, (m_nIdx)))

#define DLTDB_GET_INDEXLIST_OFFSET_START(m_pDb, m_nIdx) ((m_nIdx) > 0 ? (m_pDb)->pIndexlistOffsetEnd[(m_nIdx)-1] : 0)
#define DLTDB_GET_INDEXLIST_LENGTH(m_pDb, m_nIdx) ((m_nIdx) > 0 ? (m_pDb)->pIndexlistOffsetEnd[(m_nIdx)] -  (m_pDb)->pIndexlistOffsetEnd[(m_nIdx)-1]: (m_pDb)->pIndexlistOffsetEnd[(m_nIdx)])
#define DLTDB_GET_INDEXLIST_BY_OFFSET(m_pDb, m_nOffset) ((DLTDB_INDEX *)&(m_pDb)->pIndex2Indexlists[(m_nOffset)])

#define DLTDB_GET_COMPONENT_COUNT(m_pDb)         ((m_pDb)->pCompData->nElements)
#define DLTDB_GET_CHARACTER_COUNT(m_pDb)         ((m_pDb)->pCharData->nElements)

#define DLTDB_GET_UNICODE(m_pDb, m_nIdx)         ((m_pDb)->pUnicode[(m_nIdx)])
#define DLTDB_GET_CATANDATTRIB_LIST_IDX(m_pDb, m_nIdx) ((m_pDb)->pCatAndAttribList[(m_nIdx)])
#define DLTDB_GET_CATEGORY(m_pDb, m_nIdx)        ((m_pDb)->pCategory[DLTDB_GET_CATANDATTRIB_LIST_IDX(m_pDb, m_nIdx)])
#define DLTDB_GET_ATTRIBUTE(m_pDb, m_nIdx)       ((m_pDb)->pAttrib[DLTDB_GET_CATANDATTRIB_LIST_IDX(m_pDb, m_nIdx)])
#define DLTDB_GET_FREQUENCY(m_pDb, m_nIdx)       (DLTDB_GET_ATTRIBUTE(m_pDb, m_nIdx) & AM_FREQ)

#ifdef EXTENDED_DATABASE
	#define DLTDB_GET_KEY_FEATURES_TABLE(m_pDb, m_pFeatures, m_idx)    \
	(DLTDB_INDEX_NODE *) dbGetIndexNodeEntry((m_pDb), (m_pFeatures), (m_idx))
	#define DLTDB_GET_STROKE_ORDER(m_pDb, m_nIdx)                    ((m_pDb)->pStrokeOrder ? ((m_pDb)->pStrokeOrder[(m_nIdx)]) : 0)
	#define DLTDB_GET_TYPE(m_pDb, m_nIdx)                            ((m_pDb)->pStrokeOrder ? ((m_pDb)->pType[(m_nIdx)])        : 0)
#endif

#define DLTDB_UNICODE_IS_IN_DB(m_pDb, m_unicode)                   ((((m_pDb)->pUnicodeVector[(m_unicode) / 32]) & (1 << ((m_unicode) % 32))) != 0)
/** @} */

/** @} */




/**
 * Implemented and inlined for dbGetNamedUI32NodeEntry(),
 * to avoid library dependencies.
 */
static int 
myStrlen(const char * a)
{
	int len = 0;

	decumaAssert(a);

	while (a[len++])
		;
	return len; /* TODO use size_t */
}



/**
 * Implemented and inlined for dbGetNamedUI32NodeEntry(),
 * to avoid library dependencies.
 */
static int 
myStrcmp(const char * a, const char * b)
{
	decumaAssert(a);
	decumaAssert(b);

	while((*a) && (*b)) {
		if(*a != *b) return *b - *a;
		a++; b++;
	}

	return *b - *a;
}



/* NOTE This code needs to compile as C++ too, hence all the casts */

/**
 * Get a DECUMA_UINT32 entry in a DLTDB_NAMED_UI32_NODE
 * @param pNode The node too look in
 * @param pName The name of the entry
 * @param[out]  pbFound Set to true if the entry was found, otherwise set to false
 * @returns The value of the named entry if found, otherwise 0.
 */
static DECUMA_UINT32 
dbGetNamedUI32NodeEntry(DLTDB_NAMED_UI32_NODE * pNode, 
						const char            * pName, 
						int                   * pbFound)
{
	DLTDB_UI32_NODE_NAMED_ENTRY * pEntry;
	DECUMA_UINT32                 nEntries;

	DECUMA_UINT32 i;

	if (pbFound) *pbFound = 0;

	if (!pNode || !pName) return 0;

	nEntries = pNode->nElements;
	pEntry   = (DLTDB_UI32_NODE_NAMED_ENTRY *)pNode->pData;

	for (i = 0; i < nEntries; i++) {
		DECUMA_UINT32 nEntryLen = myStrlen((const char *)pEntry->pName);
		DECUMA_UINT32 nPadding = ((4 - nEntryLen % 4) & 3);

		if (myStrcmp((const char *)pName, (const char *)pEntry->pName) == 0) {
			if (pbFound) *pbFound = 1;
			return pEntry->nIndex;
		}

		pEntry = (DLTDB_UI32_NODE_NAMED_ENTRY *)((DECUMA_UINT8 *)pEntry + 4 + nEntryLen + nPadding);
	}

	return 0;
}

/**
 * Get a pointer to a node in a DLTDB_NAMED_INDEX_NODE
 * @param pDb   The database
 * @param pNode The index node too look in
 * @param pName The name of the entry
 * @param[out] pbFound Set to true if the entry was found, otherwise set to false
 * @returns A pointer to the named database node if found, otherwise 0.
 */
static void * 
dbGetNamedNode(DLTDB                  * pDb, 
			   DLTDB_NAMED_INDEX_NODE * pNode, 
			   const char             * pName, 
			   int                    * pbFound)
{
	DECUMA_UINT32 nDbIndex;
	int bFound;

	if (!pDb || !pNode || !pName) return NULL;

	nDbIndex = dbGetNamedUI32NodeEntry((DLTDB_NAMED_UI32_NODE *)pNode, pName, &bFound);

	if (pbFound) *pbFound = bFound;

	return (bFound && nDbIndex > 0) ? &pDb->pBase[nDbIndex] : NULL;
}

/**
 * Get a pointer to a node in a DLTDB_INDEX_NODE
 * @param pDb   The database
 * @param pNode The index node too look in
 * @param nIndex The index of the entry
 * @returns A pointer to the named database node if found, otherwise 0.
 */
static void * 
dbGetIndexNodeEntry(DLTDB const * const pDb, DLTDB_INDEX_NODE const * const pNode, DECUMA_UINT32 const nIndex)
{
	DECUMA_UINT32 nDbIndex;

	if (!pDb || !pNode) return NULL;

	decumaAssert(nIndex < (DECUMA_UINT32)pNode->nElements);

	nDbIndex = pNode->pElements[nIndex];

	/* If nDbIndex is 0, it points to the header of the db.
	 * This is used as marker for an missing node.
	 */
	return nDbIndex ? &pDb->pBase[nDbIndex] : NULL;
}


/** @} */


#endif /* CJK_DATABASE_FORMAT_MACROS_H_ */
