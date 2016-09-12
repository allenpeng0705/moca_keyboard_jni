/*****************************************************************************
* COPYRIGHT ZI AND SUBJECT TO CONFIDENTIALITY RESTRICTIONS                   *
*                                                                            *
* This file is the confidential and proprietary property of Zi Corporation   *
* of Canada, Inc. ("Zi") or one of its affiliates.                           *
******************************************************************************/
#ifndef CJK_DATABASE_COARSE_MACROS_H_
#define CJK_DATABASE_COARSE_MACROS_H_

#pragma once

#include "dltConfig.h"

#include "cjkCommon.h"
#include "cjkDatabaseCoarse.h"

/** @addtogroup DOX_DLTDB_COARSE_SEARCH
 *  @{ 
 */

/** Beam node inclusion cutoff criteria 
 *  @hideinitializer
 */
#define DLTDB_CLUSTER_BEAM_CUTOFF(m_dist, m_elem_index, m_elem_count, m_accumulated_distance) \
	(((m_accumulated_distance) > 0.991) || ((m_elem_index) >= (DLTDB_BEAM_NODE_COUNT / 2)))
/*	((m_elem_index) > 2) */
/*	((m_key_count) > 700) */
/*	((m_elem_index) != 0 && (((m_dist) > DLTDB_BEAM_CUTOFF_DIST))) */
/*	((m_elem_index) != 0 && (((m_dist) > DLTDB_BEAM_CUTOFF_DIST) || (m_elem_index) * 3 >= (m_elem_count))) */


/** Cutoff criteria used whe collecting leaf nodes from beam. 
 *  @hideinitializer
 */
#define DLTDB_CLUSTER_BEAM_FINAL_CUTOFF(m_dist, m_elem_index, m_elem_count, m_accumulated_distance) \
	(((m_accumulated_distance) > 0.97) || ((m_elem_index) >= (DLTDB_BEAM_NODE_COUNT / 2)))
/*	( ((m_accumulated_distance) > 0.97 && (m_elem_index) >= 2) || ((m_elem_index) >= (DLTDB_BEAM_NODE_COUNT / 2))) */
/*	(((m_accumulated_distance) > 0.991) || ((m_elem_index) >= (DLTDB_BEAM_NODE_COUNT / 2))) */


/** @} */

#endif /* CJK_DATABASE_COARSE_MACROS_H_ */
