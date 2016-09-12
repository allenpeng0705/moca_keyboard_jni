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


/* scrProxCurve.h */


#ifndef scrPROXCURVE_H
#define scrPROXCURVE_H

#ifdef DECUMA_MANGLE
#include "mangle.h"
#endif

#include "scrCurve.h"
#include "databaseKEY.h"
#include "decumaSimTransf.h"
#include "decumaDataTypes.h"

#define SCR_SMALL_CURVE_MU_VALUE 200

/**
Returns true if the curve is too small for the proximity measure to make any sense
*/
DECUMA_HWR_PRIVATE DECUMA_BOOL scrCurveIsTooSmallForProximityMeasure(const SCR_CURVE_PROP * curveProp,
										   int nBaseLine, int nHelpLine);

/**
Calculates the proximity value between a curve and a key. resulting proximity value is stored at pMuOut.
parameters
pMuOut				Point this to the variable that shall recieve the mu
					value, or set pMuOut to NULL.
pSimTransfOut		Point this to the variable that is to receive the
					similarity transformation values, or set pSimTransfOut
					to NULL.
pScalarProd			Point this to the varialbe that shall recieve the
					intermediate scalar products, or set pScalarProd
					to NULL.
pCurve				The curve to compare. The curve must be precalculated
					with measure id zero and alpha that matches the key
pKey				The key to compare the curve to.
*/
DECUMA_HWR_PRIVATE void proxCurveKey(DECUMA_INT16 * pMuOut, SIM_TRANSF * pSimTransfOut, const SCR_CURVE *pCurve,
				  const KID * pKid, const DECUMA_INT8 * arcOrder);

/**
This function is virtual the same as proxCurveKey, but assumes that the key has
not been precalculated.
*/
DECUMA_HWR_PRIVATE void proxCurveKeyWithoutPreCalculatedKey(DECUMA_INT16 * pMuOut, SIM_TRANSF * pSimTransfOut,
					SCR_CURVE *p, const KID * kid, int measure_id, /*NUMBERLIST* mu, */const DECUMA_INT8 * arcOrder);
/**
This function is virtual the same as proxCurveKey, but assumes that nothing has
not been precalculated.
*/
DECUMA_HWR_PRIVATE void proxCurveKeyNothingPreCalculated(DECUMA_INT16 * pMuOut, SIM_TRANSF * pSimTransfOut, const SCR_CURVE *pCurve, const KID * kid,
										 int start, int stop, int arcToZoom, const DECUMA_INT8 * arcOrder);

/**
This function is a "rough" proxCurveKey. It wont care about derivate and is able
to calculate upon an subsampling on the curve and key.
Only points 0,4,8,13,18,23,27,31 will be looked upon.

pMuOut				Point this to the variable that shall recieve the mu
					value, or set pMuOut to NULL.
pTheta				Point this to the variable that shall recieve the
					rotation angle, or set pTheta to NULL.
pCurve				The curve to compare. The curve must be precalculated
					with measure id zero and alpha that matches the key
pKey				The key to compare the curve to.
*/
DECUMA_HWR_PRIVATE void proxCurveKeyInterleavedNoDerivate(DECUMA_INT16 * pMuOut, int * pTheta, const SCR_CURVE *pCurve,
									   const KID * kid,  const DECUMA_INT8 * arcOrder);

/**
This function precalculates the norm, sum_x and sum_y, for curves that shall be
passed to proxCurveKeyInterleavedNoDerivate.
NOTE! The precalculated curve object cannot be used in the standard
proxCurveKey.
*/
DECUMA_HWR_PRIVATE void preCalculateCurveInterleavedNoDerivate(SCR_CURVE *pCurve);

/** This function computes the proximity measure squared, i.e. no sqrt is computed.
    It is used in ArcOrder.c and needs therefore to be exported. RB
*/
DECUMA_HWR_PRIVATE DECUMA_INT16 getProxMuApproximation(DECUMA_INT32 sum1, DECUMA_INT32 sum2, DECUMA_UINT32 sumSjNorm, float curveNorm, DECUMA_UINT32 qNorm);

DECUMA_HWR_PRIVATE void proxStoreTransformation(SIM_TRANSF * pTargetTrans, const SCR_CURVE * pCurve,
		DECUMA_INT32 isum1, DECUMA_INT32 isum2, DECUMA_INT32 sumSjNorm,
		DECUMA_INT32 keyNorm, float curveNorm,
							 int mass, DECUMA_INT32 key_sum_x, DECUMA_INT32 key_sum_y,
							 DECUMA_INT32 curve_sum_x, DECUMA_INT32 curve_sum_y);

#define proxStoreAngle( p, s1 , s2 ) (*(p) = (int) angle(s1,s2))

#endif
