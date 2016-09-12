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



#ifndef DECUMA_TRIGRAM_TABLE_H
#define DECUMA_TRIGRAM_TABLE_H

/* Trigram table (and friends) for fail-fast dictionary handling */

#include "decumaConfig.h"
#include "decumaBasicTypes.h"

typedef char DECUMA_TRIGRAM_TABLE;

#if defined(DECUMA_TRIGRAM_TABLE_CREATE)
#include "zoutedge2.h"
/* Create a trigram table from an eZiText dictionary */
DECUMA_TRIGRAM_TABLE * decumaCreateTrigramTable(const ZI8OUTEDGEINFO * outEdgeInfo);
#endif

/* Load from a memory buffer (actually, just cast the pointer to a DECUMA_TRIGRAM_TABLE and check that it is valid) */
const DECUMA_TRIGRAM_TABLE * decumaLoadTrigramTable(const void * buffer);

/* Returns the number of bytes in the table */
DECUMA_UINT32 decumaGetTrigramTableSize(const DECUMA_TRIGRAM_TABLE * pTable);

/* Check that a trigram exists. 
 * Return values are
 *    0 - Trigram does NOT exist
 *    1 - Trigram exists
 *   -1 - Could not calculate value (input out of bounds, or table pointer is zero)
 */
DECUMA_INT16 decumaTrigramExists(const DECUMA_TRIGRAM_TABLE * pTable, DECUMA_UINT16 first, DECUMA_UINT16 second, DECUMA_UINT16 third);

#endif
