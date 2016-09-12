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

  This file implements aslTools.h.

\******************************************************************/

#include "aslConfig.h"

#include "aslTools.h"

#include "decumaMemory.h"
#include "decumaAssert.h"



/* */
/* Public function definitions */
/* */



int aslAddElems(void * pElems, int nCurrentNbrOfElems, int nNbrOfElemsToAdd, int nElemSize, const DECUMA_MEM_FUNCTIONS* pMemFunctions)
{
	void * pNewElems;
	void ** ppElems = (void **) pElems;

	pNewElems = aslCalloc((nCurrentNbrOfElems + nNbrOfElemsToAdd) * nElemSize);	

	if (pNewElems == 0) return 0;

	if (*ppElems)
	{
		decumaMemcpy(pNewElems, *ppElems, nCurrentNbrOfElems * nElemSize);
		aslFree(*ppElems);
	}
	*ppElems = pNewElems;

	return 1;
}
