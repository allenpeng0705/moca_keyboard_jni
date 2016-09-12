/*****************************************************************************
* COPYRIGHT ZI AND SUBJECT TO CONFIDENTIALITY RESTRICTIONS                   *
*                                                                            *
* This file is the confidential and proprietary property of Zi Corporation   *
* of Canada, Inc. ("Zi") or one of its affiliates.                           *
******************************************************************************/

/************************************************** Description ***\

  This file provides the interface to optimized integer math
  functions.
 
\******************************************************************/

#ifndef DECUMA_INTEGER_MATH_H
#define DECUMA_INTEGER_MATH_H

#include "decumaConfig.h"
#include "decumaBasicTypes.h"

/**
    Returns an integer approximation of 1024 * log(x).
    The worst case error is less than 0.4%.
*/
DECUMA_HWR_PRIVATE DECUMA_UINT32 ilog_1024(DECUMA_UINT32 x);

/**
    Returns an integer approximation of 1024 * atan2(y,x).
    The worst case error is less than 0.2%.
*/
DECUMA_HWR_PRIVATE DECUMA_INT32 iatan2_1024(DECUMA_INT32 y, DECUMA_INT32 x);

/**
	Returns the integer part of sqrt(value).
*/
DECUMA_HWR_PRIVATE DECUMA_UINT32 isqrt(DECUMA_UINT32 x);

/**
	Returns an approximation of the integer part of sqrt(dx^2 + dy^2).
	The error is less than 5% (excluding round off error).
*/
DECUMA_HWR_PRIVATE DECUMA_UINT32 inorm(DECUMA_INT32 dx, DECUMA_INT32 dy);

/** Fast approximation of exp() */
DECUMA_HWR_PRIVATE double decumaExp(double y);

#endif /*DECUMA_INTEGER_MATH_H */
