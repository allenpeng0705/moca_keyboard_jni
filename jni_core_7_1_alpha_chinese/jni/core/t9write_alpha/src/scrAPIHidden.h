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

      File: scrAPI_hidden.h
   Package: This file is part of the package scr
 $Revision: 1.5 $
     $Date: 2011/02/14 11:41:14 $
   $Author: jianchun_meng $

\************************************************* Header files ***/

#ifndef scrAPI_hidden_h
#define scrAPI_hidden_h

#include "decumaDataTypes.h"
#include "decumaUnicodeTypes.h"
#include "decumaStorageSpecifiers.h"
#include "decumaSimTransf.h"
#include "databaseFormat.h" /* def. of MAX_NUMBER_OF_ARCS_IN_CURVE and NUMBER_OF_POINTS_IN_ARC */
#include "databaseKID.h" /* def. of KID */
#include "scrCurve.h" /*Expose these structures in hidden API too */

/* NOTE: This definition is also made in scrLib.h.  */
/* Change both definitions at the same time. */
#ifndef FULL_ROTATION_IMPORTANCE
#define FULL_ROTATION_IMPORTANCE (1024)
#endif

#define MAX_PRECALC_OUTPUTS_USED 2

/*
NOTE
The datatypes defined here shall NOT be accessed directly outside SCR
*/

typedef enum{
	original=0,
	alternative,
	both
} OUT_SYMBOL;
 
typedef struct {
	KID DBindex;
	DECUMA_INT16 mu;
	DECUMA_INT16 punish;
	DECUMA_UNICODE_DB_PTR symbol;
	SIM_TRANSF simTransf;
	DECUMA_INT8 nCutLeft;
	DECUMA_INT8 nCutRight;
	DECUMA_INT8 arcOrder[MAX_NUMBER_OF_ARCS_IN_CURVE];
	DECUMA_INT8 zoomValue;
	DECUMA_UINT8 arcTimelineDiffMask;
	OUT_SYMBOL outSymbol;
} SCR_OUTPUT;

#endif /* scrAPI_hidden_h */
