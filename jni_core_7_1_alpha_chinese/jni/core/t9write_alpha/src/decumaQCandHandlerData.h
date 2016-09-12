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

  This file contains private data structure definitions of
  decumaQCandHandler objects.

\******************************************************************/

#ifndef DECUMA_QCAND_HANDLER_DATA_H
#define DECUMA_QCAND_HANDLER_DATA_H

#if defined(_DEBUG) && !defined(DECUMA_QCAND_HANDLER_C)
#error Direct data access in debug mode only allowed from decumaQCandHandler.c
#endif

#include "decumaQCandHandler.h"

struct _DECUMA_QCH
{
	DECUMA_QCH_CANDREF* pCandRefBuf;
	DECUMA_QCH_CANDREF* pLastCandRef;

	void* pCandBuf;
	unsigned int nCands;
	unsigned int nMaxCands;
	unsigned int nCandSize;
	unsigned int nKeyOffset;

};

#endif /*DECUMA_QCAND_HANDLER_DATA_H */
