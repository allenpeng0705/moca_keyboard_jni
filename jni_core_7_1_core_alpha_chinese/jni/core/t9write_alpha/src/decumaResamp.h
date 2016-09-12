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


/************************************************** Description ***\

        File: decumaResamp.h
     Package: This file is part of the package generic
   Copyright: Decuma AB (2001)
     $Author: jianchun_meng $
   $Revision: 1.5 $
       $Date: 2011/02/14 11:41:14 $

\******************************************************************/

#ifndef decumaResamp_h_mscadjsfdasfjklsd
#define decumaResamp_h_mscadjsfdasfjklsd

#ifdef DECUMA_MANGLE
#include "mangle.h"
#endif

#include "decuma_point.h"
#include "decumaDataTypes.h"
#include "decumaStatus.h"

/** resampArc resamples an arc with fixed arc length.

	pPoints
		The input arc points.

	nPoints
		The number of arc points in pPoints.

	pArc
		The output arc.

	nPointsInArc
  		Desired number of points in arc (must be allocated).

	nOffsetX
	    see below

	nOffsetY
		see below

		All coordinates pointed to by ppPoint are
		repositioned:
		nOffsetX is added to all x values and
		nOffsetY to all y values.

*/

DECUMA_HWR_PRIVATE DECUMA_STATUS resampArc(const DECUMA_POINT * pPoints, int nPoints, INT16_POINT *pArc, int nPointsInArc,
			   DECUMA_INT32 nOffsetX, DECUMA_INT32 nOffsetY);


#endif /* #define decumaResamp_h_mscadjsfdasfjklsd */
