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

#ifndef CJK_CLUSTER_TREE_H_
#define CJK_CLUSTER_TREE_H_

#include "decumaConfig.h"
#include "decumaFeature.h"
#include "decumaConfig.h"

typedef struct _tagBINARY_TREE_NODE_DECISION_RULE {
	int nFeatureIdx;
	int nMidpoint; /* Less than midpoint implies left decision */
	int nLeftRepIdx; /* Representative of left child */
	int nRightRepIdx; /* Representative of right child */
} BINARY_TREE_NODE_DECISION_RULE;

typedef BINARY_TREE_NODE_DECISION_RULE 
	CLUSTER_TREE_DECISION_RULE;

typedef enum {
	CLUSTER_TREE_GO_LEFT=-1,
	CLUSTER_TREE_GO_RIGHT=1,
	CLUSTER_TREE_GO_BOTH=0
} CLUSTER_TREE_DECISION;


/* Assumes all features have been loaded into feature vector */
DECUMA_HWR_PRIVATE CLUSTER_TREE_DECISION cjkClusterTreeGetDecision(CLUSTER_TREE_DECISION_RULE * pCTDRule, DECUMA_FEATURE featValue);

DECUMA_HWR_PRIVATE int cjkClusterTreeGetFeatureIdx(CLUSTER_TREE_DECISION_RULE * pCTDRule);

DECUMA_HWR_PRIVATE void cjkClusterTreeSetReps(CLUSTER_TREE_DECISION_RULE * pCTDRule, int nLeftRep, int nRightRep);

#endif /* CJK_CLUSTER_TREE_H_ */
