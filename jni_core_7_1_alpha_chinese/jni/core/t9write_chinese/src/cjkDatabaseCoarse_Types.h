/*****************************************************************************
* COPYRIGHT ZI AND SUBJECT TO CONFIDENTIALITY RESTRICTIONS                   *
*                                                                            *
* This file is the confidential and proprietary property of Zi Corporation   *
* of Canada, Inc. ("Zi") or one of its affiliates.                           *
******************************************************************************/
#ifndef CJK_DATABASE_COARSE_TYPES_H_
#define CJK_DATABASE_COARSE_TYPES_H_

#pragma once

#include "dltConfig.h"
#include "cjkDatabaseCoarse.h"


/** @addtogroup DOX_DLTDB_COARSE_SEARCH
 *  @{ 
 */

struct _tagDLTDB_COARSE_CLUSTER_TREE {
	DLTDB_INDEX_NODE             * pClusters; /** @< Child nodes are @ref DLTDB_UI16_NODE */
	DLTDB_COARSE_CLUSTER_TREE_TYPE clusterType;
	DECUMA_UINT16                * pClusterRepresentative;
	DLTDB_UI8_NODE               * pClusterChosenIndices;
	DECUMA_UINT16                * pClusterParentIndices;
	DECUMA_UINT16                * pNChildrenUpToCluster;
	DECUMA_UINT16                * pClusterChildIndices;
	DECUMA_UINT16                * pNFeaturesUpToCluster;
	DECUMA_UINT8                 * pClusterFeatureIndices;
	DECUMA_UINT8                 * pClusterFeatureValues;
	DLTDB_INDEX_NODE             * pDCSClusters;
};

struct _tagDLTDB_COARSE_SEARCH_RESULT {
	/* A bit array to keep track of which indexes have been treated */
	DECUMA_UINT8    pIndices[MAX_NBR_DB_IDX_BIT_CHARS];
	int             nIndices;
	DECUMA_UINT16   pStack[DLTDB_MAX_CLUSTER_TREE_HEIGHT];
	int				bCorrectFound;
};


struct _tagDLTDB_BEAM_NODE {
	DECUMA_UINT16              nNodeIdx;
	DLTDB_COARSE_DCS_DIST_TYPE dist;
};

struct _tagDLTDB_BEAM_NODEVEC {
	DLTDB_BEAM_NODE        nodes[DLTDB_BEAM_NODE_COUNT];
	DECUMA_UINT32          nNodes;
} ;

struct _tagDLTDB_BEAM_INCLUSION_VEC {
	DLTDB_BEAM_NODE        nodes[DLTDB_BEAM_MAX_INCLUSION_COUNT];
	DECUMA_UINT32          nNodes;
} ;


struct _tagDLTDB_BEAM {
	/** @< The leaf nodes that should be considered in the end */
	DLTDB_BEAM_NODEVEC   includedNodes;
	/** @< Internal nodes that are found during a pass are stored here */
	DLTDB_BEAM_NODEVEC * pWorkingNodesStore;
	/** @< Internal nodes that are found during the previous pass are stored here */
	DLTDB_BEAM_NODEVEC * pWorkingNodesUse;
};




struct _tagCJK_DB_COARSE_SCRATCHPAD {
	DLTDB_BEAM_NODEVEC beamNodes1;
	DLTDB_BEAM_NODEVEC beamNodes2;
	DLTDB_BEAM_NODEVEC childList;
};

struct _tagDLTDB_COARSE_SEARCH_SESSION {

	/** The type of search, must correspond to object type. E.g.
	    with <tt>pCSS->type == DLTDB_COARSE_CLUSTER_TREE_TYPE_DTWSEP</tt>, 
		the cast <tt>(DLTDB_COARSE_SEARCH_SESSION_DTW *)pCSS</tt> should
		be meaningful.
	*/
	DLTDB_COARSE_CLUSTER_TREE_TYPE   type;

	/** Current Session */
	CJK_SESSION                    * pSession;

	/** The tree to search */
	DLTDB_COARSE_CLUSTER_TREE      * pClusterTree;

	/** Character to match */
	CJK_COMPRESSED_CHAR            * pCChar;

	/** List of potential candidates is placed here by the search. */
	DLTDB_COARSE_SEARCH_RESULT     * pResult;

	/** Scratchpad to reduce stack usage by some functions */
	CJK_DB_COARSE_SCRATCHPAD	   * pScratchpad;
};


struct _tagDLTDB_COARSE_SEARCH_SESSION_GENERIC {
	DLTDB_COARSE_SEARCH_SESSION base;
	int                         nLastCompactionGain;
#if defined CJK_ENABLE_INTERNAL_API
	DLTDB_COARSE_DBG_GENERIC_TREE_TEST_PATH   * pPath;
#endif
};


struct _tagDLTDB_COARSE_SEARCH_SESSION_DCS {
	DLTDB_COARSE_SEARCH_SESSION_GENERIC base;
};


#if defined CJK_ENABLE_INTERNAL_API || defined __DOXYGEN__
struct _tagDLTDB_COARSE_DBG_BINARY_TREE_TEST_PATH {
	int bUsed;
	int nNodeIndex;
	int nLeftDist;
	int nRightDist;
	CLUSTER_TREE_DECISION decision;
};

struct _tagDLTDB_COARSE_DBG_GENERIC_TREE_TEST_PATH {
	int bUsed;
	int index[100];
	DLTDB_COARSE_DCS_DIST_TYPE distance[100];
	int isExternal[100];
	int isIncluded[100];
	int isCorrect[100];
	int nIndices;
};
#endif /* CJK_ENABLE_INTERNAL_API */


/** @} */

#endif /* CJK_DATABASE_CORASE_TYPES_H_ */
