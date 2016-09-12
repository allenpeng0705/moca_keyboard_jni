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


#ifndef UDMLIB_H
#define UDMLIB_H

#include "decumaUnicodeTypes.h"
#include "decuma_point.h"
#include "decumaBasicTypes.h"	/* definition of DECUMA_INT32 etc. */
#include "decumaCurve.h"
#include "decumaStatus.h"
#include "udmType.h"
#include "udmAccess.h"
#include "decumaCharacterSetType.h"
#include "decumaRuntimeMallocData.h" /* Definition of DECUMA_MEM_FUNCTIONS */

#if defined (__cplusplus)
extern "C" {
#endif



#define MAX_NUMBER_OF_ARCS_IN_UDMCURVE		(6)


/* udmCreateDynamic
This function create a UDM database that can grow and shrink
dynamically. UDMs created with this function must be released
using udmReleaseDynamic or udmDestroyDynamic*/
DECUMA_HWR_PRIVATE UDM_PTR udmCreateDynamic(const DECUMA_MEM_FUNCTIONS * pMemFunctions);

DECUMA_HWR_PRIVATE void udmDestroyDynamic(UDM_PTR pUDMToDestroy,
								  const DECUMA_MEM_FUNCTIONS * pMemFunctions);


/* udmAddAllograph
This function is used to add new allographs that can be recognized
by scr.
The data pointed to by pCurve and
pSymbol will be copied into the UDM. If a base line and a help
line are used the curve coordinates will, if necessary, be shrinked
to fit the following limits (Note that the y-coords are upside down):

max_x - min_x <= 4 * (nBaseLine - nHelpLine)
min_y >= nBaseLine - 4 * (nBaseLine - nHelpLine)
max_y <= nBaseLine + 4 * (nBaseLine - nHelpLine)

If baseline == helpline, the function will not fail, instead it
will set baseline = max_y and helpline = min_y and proceed.

Returns:
	0 if no error.

Parameters:
	ppUDM		The user database modifier database
            that stores the allographs. The pointer to the UDM will
            be changed after the function call. That is why a pointer to the
            pointer has to be provided.	
	pCurve	The curve that represents the shape of the
				allograph that the user wishes to add. It can consist of no more
				than MAX_NUMBER_OF_ARCS_IN_UDMCURVE arcs.
	pSymbol		the text that the allograph represents.
	nSymbolLength	the length of the text that the allograph represents.
	
	pSymbolCategories,
	nSymbolCategories,
	pLanguages,
	nLanguages
			The categories/languages that the symbol shall belong to.
			A maximum of 32 categories and 32 languages will be added to the db
			Needs to contain at least 1 category and 1 language
			
	nSymbolType The symbol type number for this allograph.
                    This number does not affect the interpretation.
                    It is only stored in the database and returned upon request
				    from the user e.g. mcrGetSymbolType().
	bGesture	    Says if this shall be marked as a gesture
	bInstantGesture Says if this shall be marked as an instant gesture. 
	                Is only looked at if bGesture is set to 1
	nBaseline	    What shall be considered the baseline
				    in the curve.
	nHelpline	    What shall be considered the helpline
				    in the curve.

*/
DECUMA_HWR_PRIVATE DECUMA_STATUS udmAddAllograph(UDM_PTR* ppUDM, 
		const DECUMA_CURVE * pCurve, 
		const DECUMA_UNICODE * pSymbol,
		int nSymbolLength,
		const DECUMA_CHARACTER_SET * pCharacterSet,
		DECUMA_INT8 nSymbolType,
		int nBaseline, int nHelpline,
		int bGesture, int bInstantGesture,
		const DECUMA_MEM_FUNCTIONS * pMemFunctions);

DECUMA_HWR_PRIVATE DECUMA_UINT32 udmGetByteSize(UDM_PTR pUDM);


DECUMA_HWR_PRIVATE int udmIsValid(UDM_PTR pUDM);


#if defined (__cplusplus)
} /* extern "C" { */
#endif

#endif
