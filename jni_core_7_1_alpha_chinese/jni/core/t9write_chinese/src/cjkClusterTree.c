/*****************************************************************************
* COPYRIGHT ZI AND SUBJECT TO CONFIDENTIALITY RESTRICTIONS                   *
*                                                                            *
* This file is the confidential and proprietary property of Zi Corporation   *
* of Canada, Inc. ("Zi") or one of its affiliates.                           *
******************************************************************************/

/*
 *  @author Jakob Sternby
 *  @date 2008-10-06.
 */

#include "cjkClusterTree.h"

/* ----------------------------------------------------------------------- --
 * Exported function definitions
 * ------------------------------------------------------------------------- */

CLUSTER_TREE_DECISION cjkClusterTreeGetDecision(CLUSTER_TREE_DECISION_RULE * pCTDRule, DECUMA_FEATURE featValue)
{
	if (featValue <= pCTDRule->nMidpoint)
		return CLUSTER_TREE_GO_LEFT;	
	return CLUSTER_TREE_GO_RIGHT;
}

int cjkClusterTreeGetFeatureIdx(CLUSTER_TREE_DECISION_RULE * pCTDRule)
{
	return pCTDRule->nFeatureIdx;
}

void cjkClusterTreeSetReps(CLUSTER_TREE_DECISION_RULE * pCTDRule, int nLeftRep, int nRightRep)
{
	pCTDRule->nLeftRepIdx = nLeftRep;
	pCTDRule->nRightRepIdx = nRightRep;
}
