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


/* AEFineSearch.h  */
/*Written */
/*Date: 2000/09/14  */
/*Author: Rikardb (implemented from Anders Erikssons matlab version) */

/*This module implements a fine search version that takes a set of candidate interpretations */
/*as input and outputs a subset of candidates.  */
#ifndef AEFineSearch_h_jkxhdfgkyuh4w587ty07fgkagrfi72346grfouwbfckuhaeb4i7r63
#define AEFineSearch_h_jkxhdfgkyuh4w587ty07fgkagrfi72346grfouwbfckuhaeb4i7r63


#ifdef DECUMA_MANGLE
#include "mangle.h"
#endif

#include "databaseKEY.h"
#include "scrOutput.h"
#include "scrCurve.h"
#include "scrOutputHandler.h"
#include "decumaStatus.h"

/*
This function performs a fine search of the candidates with IDs specified in candidatesIn.
The function adds candidates to the candidateOut list if the candidate is good enough. Note
that the candidateOut vector must have been initialized using outputResetVector(...).

Calling AEFineSearch several times with different candidatesIn but the same candidatesOut,
will result in that the best matches from all candidatesIn are stored in the 
candidatesOut vector.

*/
/*This function performes a fine search search of candidates. */
/*Output: candidate */
/*Input:  */
/*psi = input curve */
/*ind = list of indices into database for candidates */
DECUMA_HWR_PRIVATE DECUMA_STATUS scrFineSearch( OUTPUT_HANDLER * pFinesearchOutputs, const SCR_CURVE *pCurve,
				   const scrOUTPUT_LIST * pPrecalculatedBaseOutputs,
				   OUTPUT_HANDLER * pFullsearchOutputs, const int * baseLine, const int *helpLine,
				   const DECUMA_INTERRUPT_FUNCTIONS * pInterruptFunctions);

DECUMA_HWR_PRIVATE DECUMA_INT16 PunishForScalingAndVerticalOffset(const scrOUTPUT *proxData,
										const SCR_CURVE_PROP * curveProp,
										const KID * pKid,
										int baseLine,
										int helpLine,
										int* pnAdjustmentFactor);

DECUMA_HWR_PRIVATE int scrCurveAndKeyAreQuiteSmall(const scrOUTPUT *proxData,
				const SCR_CURVE_PROP * curveProp, const KID * pKid, const int baseLine,
				const int helpLine);

#endif /* AEFineSearch_h_jkxhdfgkyuh4w587ty07fgkagrfi72346grfouwbfckuhaeb4i7r63 */
