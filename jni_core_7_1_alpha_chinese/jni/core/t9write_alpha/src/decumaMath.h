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


#ifndef decumaMath_h_zxcnfjmshdkjfksdhflsdh
#define decumaMath_h_zxcnfjmshdkjfksdhflsdh

/* File: decumaMath.h */

#ifdef DECUMA_MANGLE
#include "mangle.h"
#endif

#include "decumaConfig.h"
#include "decumaDataTypes.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PI 314

#define scaleFactorForSin (256)
/*#define shiftForRoundingEffects (4) */
/*#define shiftForSin (8) */

#define decumaAbs(x) ((x) >= 0 ? (x) : -(x))

#define maxNUMB(a,b)  ((a) > (b) ? (a) : (b))
#define minNUMB(a,b)  ((a) < (b) ? (a) : (b))
#define maxAbs(a,b) maxNUMB(decumaAbs(a), decumaAbs(b))
#define minAbs(a,b) minNUMB(decumaAbs(a), decumaAbs(b))


DECUMA_HWR_PRIVATE DECUMA_INT32 decumaLog2(DECUMA_INT32 x);

DECUMA_HWR_PRIVATE int mod_sym(int a, int b);
DECUMA_HWR_PRIVATE int angle(DECUMA_INT32 a, DECUMA_INT32 b);


DECUMA_HWR_PRIVATE DECUMA_INT32 invNorm(DECUMA_INT32 a, DECUMA_INT32 b);
DECUMA_HWR_PRIVATE DECUMA_INT32 norm(DECUMA_INT32 a, DECUMA_INT32 b);
DECUMA_HWR_PRIVATE int mostSignificantBit(DECUMA_UINT32 x);
DECUMA_HWR_PRIVATE DECUMA_INT32 newNorm(DECUMA_INT32 a, DECUMA_INT32 b);
DECUMA_HWR_PRIVATE DECUMA_INT32 normFine(DECUMA_INT32 a, DECUMA_INT32 b);


DECUMA_HWR_PRIVATE int indexToMin(const DECUMA_INT16* p, int nElements);
DECUMA_HWR_PRIVATE int lastIndexToMin(const DECUMA_INT16* p, int nElements);
DECUMA_HWR_PRIVATE int indexToMax(const DECUMA_INT16* p, int nElements);
DECUMA_HWR_PRIVATE int lastIndexToMax(const DECUMA_INT16* p, int nElements);

DECUMA_HWR_PRIVATE int indexToMin32(const DECUMA_INT32* p, int nElements);
DECUMA_HWR_PRIVATE int indexToMax32(const DECUMA_INT32* p, int nElements);

DECUMA_HWR_PRIVATE DECUMA_INT16 minNUMBER(const DECUMA_INT16* p, int nElements);
DECUMA_HWR_PRIVATE DECUMA_INT16 maxNUMBER(const DECUMA_INT16* p, int nElements);

DECUMA_HWR_PRIVATE DECUMA_UINT32 decumaScale(DECUMA_UINT32 a, DECUMA_UINT32 b, int scaleFactorForRoundingEffects);

/* Returns x rounded to the closest integer. */
DECUMA_HWR_PRIVATE DECUMA_INT32 decumaRound( float x );

DECUMA_HWR_PRIVATE DECUMA_UINT32 decumaSqrt(DECUMA_UINT32 x);
DECUMA_HWR_PRIVATE DECUMA_INT16 SquareRoot(float f);

/* Returns 1000*sqrt(1-f) */
DECUMA_HWR_PRIVATE DECUMA_INT16 SquareRootVerySpecial(float f, const DECUMA_INT16 maxProxValue);

DECUMA_HWR_PRIVATE DECUMA_UINT32 decumaScaleFloat(float a, float b, int scaleFactorForRoundingEffects);

/* takes the values at pPointer, derivates them and put the result */
/* at derivPointer. */
DECUMA_HWR_PRIVATE void derivative(const DECUMA_INT16 *pPointer, DECUMA_INT16 * derivPointer, int delta);

#define RANGE16(str,x)  if((x) < -32768 || (x) > 32767) printf("%s: 16-bit overflow in %s at %d\n",str,__FILE__, __LINE__);

DECUMA_HWR_PRIVATE int deltaAngle(int a, int aRef);

DECUMA_HWR_PRIVATE DECUMA_INT32 decumaSin(DECUMA_INT32 x);
DECUMA_HWR_PRIVATE DECUMA_INT32 decumaCos(DECUMA_INT32 x);


#define boundsCheck(val, lowMark, highMark) ((val) < (lowMark) ? (lowMark) : ((val) > (highMark) ? (highMark) : (val) ))

#ifdef __cplusplus
} /*extern "C" { */
#endif

#endif /* #define decumaMath_h_zxcnfjmshdkjfksdhflsdh */
