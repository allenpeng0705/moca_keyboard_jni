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


#ifndef ASL_CONFIG_H
#define ASL_CONFIG_H

#include "decumaConfig.h"

#ifdef DECUMA_HWR_UNIT_TEST

#ifdef _DEBUG
/*#define _DEBUG_EXTRA */
#endif

/* We artificially restrict the max number of nodes in order to test what 
 * happens after the input buffer fills.
 * (with MAX_NODES_IN_SEGMENTATION_GRAPH == MAX_DECUMA_INT16, these tests
 * takes forever to finish.
 */
#define ARTIFICIAL_MAX_NODES_IN_SG 100

#endif /*DECUMA_HWR_UNIT_TEST */


#endif /* ASL_CONFIG_H */
