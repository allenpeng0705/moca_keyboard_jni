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


#ifndef SCRLIB_EXTRA_INCLUDED_H
#define SCRLIB_EXTRA_INCLUDED_H


#include "scrlib.h"
#include "scrAPIHidden.h"

DECUMA_HWR_PRIVATE DECUMA_STATUS scrRecognizeExtra(
			const DECUMA_CURVE * pCurve,
			SCR_RESULT * pResults, 
			SCR_OUTPUT * pOutputs, 
			unsigned int nResults, 
			int * pnResultsReturned, 
			const SCR_SETTINGS * pSettings,
			void * pMemoryBuffer,
			const DECUMA_INTERRUPT_FUNCTIONS * pInterruptFunctions);

/*
This function can be called immedately after scrRecognize/scrRecognizeExtra
to get the SCR_CURVE used in the recognition.
*/
DECUMA_HWR_PRIVATE void scrGetCurve(SCR_CURVE* pScrCurve, void * pMemoryBuffer);

DECUMA_HWR_PRIVATE void scrGetCurveProp(const SCR_OUTPUT* pOutput, SCR_CURVE_PROP* pScrCurveProp, void * pMemoryBuffer);

DECUMA_HWR_PRIVATE DECUMA_STATUS scrIsOutputEstimateReliable(const SCR_OUTPUT* pOutput,
										  int *pnIsReliable);

DECUMA_HWR_PRIVATE DECUMA_STATUS scrEstimateScalingAndVerticalOffset(const SCR_OUTPUT* pOutput,
												  DECUMA_COORD* pnBaseLineYEstimate,
												  DECUMA_COORD* pnHelpLineYEstimate);

/*
Since a SCR_CURVE takes time to build from a DECUMA_CURVE this function must take a
SCR_CURVE in order to support frequent calling at feasible speed.
*/
DECUMA_HWR_PRIVATE DECUMA_STATUS scrGetScalingAndVerticalOffsetPunish(const SCR_OUTPUT* pOutput,
												   const SCR_CURVE_PROP* pScrCurveProp,
												   DECUMA_COORD nBaseLineY,
												   DECUMA_COORD nHelpLineY,
												   DECUMA_INT16 *pnPunish);

DECUMA_HWR_PRIVATE DECUMA_STATUS scrGetRotationPunish(const SCR_OUTPUT* pOutput,
								   const SCR_CURVE_PROP* pScrCurveProp,
								   DECUMA_INT16 nBaseLineY,
								   DECUMA_INT16 nHelpLineY,
								   int nRefAngle,
								   DECUMA_INT16 *pnPunish);

DECUMA_HWR_PRIVATE DECUMA_INT16 scrGetSymmetry(const SCR_OUTPUT* pOutput);

/*
This function adjusts pOutput's mu and punish for new baseline and helpline values.
The (old) baseline and helpline that resulted in pOutput must be provided.
*/
DECUMA_HWR_PRIVATE DECUMA_STATUS scrAdjustMuAndPunish(SCR_OUTPUT* pOutput,
								   const SCR_CURVE_PROP* pScrCurveProp,
								   DECUMA_COORD nNewBaseLineY,
								   DECUMA_COORD nNewHelpLineY,
								   DECUMA_COORD nOldBaseLineY,
								   DECUMA_COORD nOldHelpLineY);

DECUMA_HWR_PRIVATE DECUMA_STATUS scrOutputIsGesture(const SCR_OUTPUT* pOutput,
								 int * pbGesture, int * pbInstantGesture);

DECUMA_HWR_PRIVATE CATEGORY_TABLE_PTR scrDatabaseGetCatTable(SCR_ANY_DB_PTR pDB);

DECUMA_HWR_PRIVATE int scrOutputInCategory(const SCR_OUTPUT* pOutput,
						const CATEGORY Category);

#endif
