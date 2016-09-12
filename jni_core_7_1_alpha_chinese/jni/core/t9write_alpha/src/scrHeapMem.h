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


#ifndef SCR_HEAP_MEM_H_wne7ot2gb635
#define SCR_HEAP_MEM_H_wne7ot2gb635

#include "scrCurve.h"
#include "scrAPI.h"
#include "scrOutput.h"
#include "scrOutputHandler.h"

/*This should preferably be located on the heap. */
/*Data should be stored in this structure to save stack space. */

#define MAX_NUMBER_OF_OUTPUTS MAX_NUMBER_OF_OUTPUTS_IN_BUFFER

typedef struct _tagSCR_HEAP_MEM
{
	/*//// Used in scrSelect ////// */
	SCR_CURVE scr_curve;
	SCR_ARC scr_arcs[MAX_NUMBER_OF_ARCS_IN_CURVE];
	SCR_API_SETTINGS scrSettings;
	SCR_OUTPUT outputCopies[MAX_ARCS_IN_DIACRITIC][MAX_PRECALC_OUTPUTS_USED];
	SCR_OUTPUT_LIST precalcOutputCopy[MAX_ARCS_IN_DIACRITIC];
	scrOUTPUT scrOutputs[MAX_NUMBER_OF_OUTPUTS];

	/*//// Used in scrSel ////// */
	scrOUTPUT fullsearchKid[MAX_NUMBER_OF_OUTPUTS_IN_BUFFER];
	OUTPUT_HANDLER fullsearchOutputHandler, finesearchOutputHandler;


	
} SCR_HEAP_MEM;


#endif /*SCR_HEAP_MEM_H_wne7ot2gb635 */
