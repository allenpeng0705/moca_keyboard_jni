/*****************************************************************************
* COPYRIGHT ZI AND SUBJECT TO CONFIDENTIALITY RESTRICTIONS                   *
*                                                                            *
* This file is the confidential and proprietary property of Zi Corporation   *
* of Canada, Inc. ("Zi") or one of its affiliates.                           *
******************************************************************************/
#ifndef CJK_DATABASE_COARSE_H_
#define CJK_DATABASE_COARSE_H_

#pragma once

#include "dltConfig.h"
#include "cjkClusterTree.h"

/** constants for beam search */
#define DLTDB_BEAM_MIN_DIST            0.0001
#define DLTDB_BEAM_NODE_COUNT          100
/** The maximum number of leaves that a template can be included in */
#define DLTDB_BEAM_MAX_INCLUSION_COUNT 300




/** 
	@defgroup DOX_DLTDB_COARSE_SEARCH cjkDatabaseCoarse
	@{
	@note Coarse database search implementation is not yet finalized. 
	At present there are two entry points to this module, 
	@ref cjkDbCoarseBinarySearch and @ref cjkDbCoarseBeamSearch.

	The former supports the Legacy & DTW separation algorithms and tree types,
	the latter the DCS & kMeans ditto.
	
	@section Usage

	Create and initialize an appropriate @ref _tagDLTDB_COARSE_SEARCH_SESSION 
	"DLTDB_COARSE_SEARCH_SESSION" subtype,
	then call the appropriate entry point with that argument. Results will be
	returned in the @ref _tagDLTDB_COARSE_SEARCH_SESSION::pResult 
	"DLTDB_COARSE_SEARCH_SESSION::pResult" field. Example:

	@code
DLTDB_COARSE_SEARCH_SESSION_KMEANS cSSessionKM = {0};
DLTDB_COARSE_SEARCH_SESSION * pCSSession = (DLTDB_COARSE_SEARCH_SESSION *)&cSSessionKM;

pCSSession->type         = DLTDB_COARSE_CLUSTER_TREE_TYPE_KMEANS;
pCSSession->pSession     = pSession;
pCSSession->pClusterTree = pClusterTree;
pCSSession->pCChar       = pCChar;
pCSSession->pResult      = pResult;

status = cjkDbCoarseBeamSearch(pCSSession);
	@endcode

	This pattern could be removed once the clustering strategy has been
	decided on, but the overhead in terms of speed is probably negligible.
	Doing so will however reduce the complexity of this module considerably.
 */

/** @name Coarse Search Types
 *  @{
 */

typedef struct _tagCJK_DB_COARSE_SCRATCHPAD CJK_DB_COARSE_SCRATCHPAD;

/** Types of clustering that can be used in the database */
typedef enum _tagDLTDB_COARSE_CLUSTER_TREE_TYPE {
	DLTDB_COARSE_CLUSTER_TREE_TYPE_UNKNOWN = 0,
	DLTDB_COARSE_CLUSTER_TREE_TYPE_SINGLE  = 1,
	DLTDB_COARSE_CLUSTER_TREE_TYPE_DCS     = 2
} DLTDB_COARSE_CLUSTER_TREE_TYPE;

/** The cluster tree */
typedef struct _tagDLTDB_COARSE_CLUSTER_TREE
	DLTDB_COARSE_CLUSTER_TREE;

/** The result of a coarse search */
typedef struct _tagDLTDB_COARSE_SEARCH_RESULT
	DLTDB_COARSE_SEARCH_RESULT;






/** @name Search Session Types
 *  Type hierarchy corresponding to tree/search types.
 *  @{ 
 */

/** Base type for Search sessions */
typedef struct _tagDLTDB_COARSE_SEARCH_SESSION
	DLTDB_COARSE_SEARCH_SESSION;

/** Inherits DLTDB_COARSE_SEARCH_SESSION as base */
typedef struct _tagDLTDB_COARSE_SEARCH_SESSION_BINARY
	DLTDB_COARSE_SEARCH_SESSION_BINARY;

/** Inherits DLTDB_COARSE_SEARCH_SESSION as base */
typedef struct _tagDLTDB_COARSE_SEARCH_SESSION_GENERIC
	DLTDB_COARSE_SEARCH_SESSION_GENERIC;

/** Inherits DLTDB_COARSE_SEARCH_SESSION_GENERIC as base */
typedef struct _tagDLTDB_COARSE_SEARCH_SESSION_DCS
	DLTDB_COARSE_SEARCH_SESSION_DCS;

/** @} */





/** @name The Beam Search Types
 *  KMEANS and DCS Both use beam search.
 *  @{
 */

/** A cluster node index and distance */
typedef struct _tagDLTDB_BEAM_NODE
	DLTDB_BEAM_NODE;

/** A vector of DLTDB_BEAM_NODEs */
typedef struct _tagDLTDB_BEAM_NODEVEC
	DLTDB_BEAM_NODEVEC;

/** A vector of DLTDB_BEAM_NODEs */
typedef struct _tagDLTDB_BEAM_INCLUSION_VEC
	DLTDB_BEAM_INCLUSION_VEC;

/** A beam */
typedef struct _tagDLTDB_BEAM
	DLTDB_BEAM;

/** @} */




/** @name The Binary Search Types
 *  LEGACY and DTW Both use binary search.
 *  @{
 */

/** Function type used in the legacy & dtw algorithms */
typedef CLUSTER_TREE_DECISION (DLTDB_COARSE_BINARY_DECISION_FN)(
	DLTDB_COARSE_SEARCH_SESSION * pCSSession,
	int                           nClusterIndex
);

/** @} */



#if defined CJK_ENABLE_INTERNAL_API || defined __DOXYGEN__
/** @name  Internal Testing Types
 *  These types are only available when debugging internally.
 *  @{
 */

/** Used to track the path taken through a binary cluster tree during testing */
typedef struct _tagDLTDB_COARSE_DBG_BINARY_TREE_TEST_PATH
	DLTDB_COARSE_DBG_BINARY_TREE_TEST_PATH;

/** Used to track the path taken through a k-means cluster tree during testing */
typedef struct _tagDLTDB_COARSE_DBG_GENERIC_TREE_TEST_PATH
	DLTDB_COARSE_DBG_GENERIC_TREE_TEST_PATH;

/** @} */

#endif /* CJK_ENABLE_INTERNAL_API */

#include "cjkDatabaseFormat.h"
#include "cjkSession.h"
#include "cjkCompressedCharacter.h"

DECUMA_HWR_PRIVATE DECUMA_UINT32 cjkDbCoarseGetScratchpadSize(void);

/** @name Beam Coarse Search functions
 *  Beam search implementation for DCS & kMeans clustering. Probably not working 
 *  for kMeans anymore, this was superseeded by DCS.
 *  @{
 */

/** Main beam search function. See DLTDB_COARSE_SEARCH_SESSION for paramters. */
DECUMA_HWR_PRIVATE DECUMA_STATUS
cjkDbCoarseBeamSearch(DLTDB_COARSE_SEARCH_SESSION * pCSSession);

/** Called by cjkDbCoarseBeamSearch, creates a list of child nodes of 
    the nodes in the beam, sorted according to decreasing probability.
   */
DECUMA_HWR_PRIVATE DECUMA_STATUS
cjkDbCoarseBeamPrepareChildList(DLTDB_COARSE_SEARCH_SESSION * pCSSession,
                                DLTDB_BEAM                  * pBeam, 
								DLTDB_BEAM_NODEVEC          * childList,
                                CJK_BOOLEAN                   bUseNPoints);


/** Move node indices from childList to pBeam, stopping when cutoff criteria is reached. 
 *  @see DLTDB_CLUSTER_BEAM_CUTOFF 
 */
DECUMA_HWR_PRIVATE void
cjkDbCoarseBeamMoveElementsToBeam(DLTDB_COARSE_SEARCH_SESSION * pCSSession, 
                                  DLTDB_BEAM                  * pBeam, 
                                  DLTDB_BEAM_NODEVEC          * childList);

/** Fetch key indices from the nodes stored in pBeam into pCSSession->pResult */
DECUMA_HWR_PRIVATE DECUMA_STATUS
cjkDbCoarseBeamFetchKeyIndicesFromBeam(DLTDB_COARSE_SEARCH_SESSION * pCSSession,
                                       DLTDB_BEAM                  * pBeam);


/** Get the probability of cluster nClusterIndex containing the correct character. 
    Called by cjkDbCoarseBeamSearch().
*/
DECUMA_HWR_PRIVATE double
getCoarseDistanceToClusterRep_DCS(DLTDB_COARSE_SEARCH_SESSION * pSession, int nClusterIndex);

/** @} */


/**
 * @name Other
 * @{
 */

/** Remove duplicates from results.
 *  Sorts the list as a side effect.
 */
DECUMA_HWR_PRIVATE void
cjkDbCoarseResultsRemoveDuplicates(DLTDB_COARSE_SEARCH_RESULT * pResults);
/** @} */


/** @} */

#endif /* CJK_DATABASE_COARSE_H_ */
