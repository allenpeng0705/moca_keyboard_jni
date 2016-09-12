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

        File: udmKey.h
     Package: This file is part of the package udm 
   Copyright: Decuma AB (2001)
     $Author: jianchun_meng $ 
   $Revision: 1.5 $
       $Date: 2011/02/14 11:41:14 $
 
\******************************************************************/

/* 
   Types and function prototypes for creating dynamic
   keys with the same format as in the static database.
*/

#ifndef udmKey_h_jklxdfzjklfsdzuilvxcjkgjflklkdfs
#define udmKey_h_jklxdfzjklfsdzuilvxcjkgjflklkdfs

#ifdef DECUMA_MANGLE
#include "mangle.h"
#endif

#include "databaseFormat.h"
#include "databaseKEY.h"

#ifdef __cplusplus
extern "C" {
#endif

/* KEY initialisation and destruction */
DECUMA_HWR_PRIVATE void		keyInit(struct _KEY * pKey, int noArcs);
DECUMA_HWR_PRIVATE void		keyRelease(struct _KEY * pKey);

/* KEYLIST  destruction */
DECUMA_HWR_PRIVATE void		keylistRelease(struct _KEYLIST * pToRelease);

/*	Precalculates the ligatures, mean_x, mean_y, norm_1 and norm_2 */
DECUMA_HWR_PRIVATE void keyPreCalculate(struct _KEY * pKey, DECUMA_INT8_DB_PTR pKeyCurve, int noArcs);

#ifdef __cplusplus
} /*extern "C" { */
#endif

#endif
