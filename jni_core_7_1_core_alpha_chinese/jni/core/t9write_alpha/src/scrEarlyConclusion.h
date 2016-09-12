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


/*
Autogenerated header.
$Revision: 1.5 $
$Date: 2011/02/14 11:41:14 $
$Author: jianchun_meng $
*/

#ifndef scrEarlyConclusion_h_nfjkshfjsdhfznvkzsjhgfsdhvn
#define scrEarlyConclusion_h_nfjkshfjsdhfznvkzsjhgfsdhvn

#ifdef DECUMA_MANGLE
#include "mangle.h"
#endif

# include "scrOutputHandler.h"


/* earlyConclusion(...)
	This function analyses a vector of scrOUTPUTs and determines
	if one can for sure rule out that the last x elements are totally
	unrealistic. It returns the number of elements that 
	should be considered for further evaluation, counting from 
	the begining.

	Parameters:
	pOutputHandler - a pointer to a output handler that contains a buffer of the outputs

	Returns:
		The number of elements in the vector that are relevant.

*/
/* #define EARLY_LIM 1000 */

DECUMA_HWR_PRIVATE int earlyConclusion(OUTPUT_HANDLER * pOutputHandler);

#endif
