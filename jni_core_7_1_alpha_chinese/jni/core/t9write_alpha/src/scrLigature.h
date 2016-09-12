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


#ifndef scrLigature_h_asldkfjnasdfasfygblaslfjnasngasufygbalsfga
#define scrLigature_h_asldkfjnasdfasfygblaslfjnasngasufygbalsfga

#ifdef DECUMA_MANGLE
#include "mangle.h"
#endif

#include "scrOutput.h"
#include "scrCurve.h"
#include "decumaDataTypes.h"
#include "databaseKEY.h"

/*These functions calculate the best way of cutting a curve and how to punish this cutting */
/*Previously in scrFinesearch */

DECUMA_HWR_PRIVATE int GaussNewton(scrOUTPUT *proxData, const int measureId, const int arcToCut,
				 const SCR_CURVE *curve, SCR_CURVE *curveCut, const scrOUTPUT_LIST * pPrecalculatedBaseOutputs,
				 DECUMA_BOOL fromZoom, int start, int stop, const int nMaxCuts);

DECUMA_HWR_PRIVATE void getPunishPerCut(int lowestUncutMu, int* pPunishPerCut, int * pMaxCuts);

DECUMA_HWR_PRIVATE void PunishForCutting(scrOUTPUT *proxData, const int punishPerCut);

#endif

