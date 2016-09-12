/*******************************************************************************
;*******************************************************************************
;**                                                                           **
;**                  COPYRIGHT 2001-2011 NUANCE COMMUNICATIONS                **
;**                                                                           **
;**               NUANCE COMMUNICATIONS PROPRIETARY INFORMATION               **
;**                                                                           **
;**     This software is supplied under the terms of a license agreement      **
;**     or non-disclosure agreement with Nuance Communications and may not    **
;**     be copied or disclosed except in accordance with the terms of that    **
;**     agreement.                                                            **
;**                                                                           **
;*******************************************************************************
;**                                                                           **
;**     FileName: __et9eval.c                                                 **
;**                                                                           **
;**  Description: eval code for ET9                                           **
;**                                                                           **
;*******************************************************************************
;******************************************************************************/

/*! \internal \addtogroup et9eval Eval for XT9
* Eval for generic XT9.
* @{
*/

#include "__et9eval.h"

#ifdef EVAL_BUILD

void ET9FARCALL _ET9Eval_StartTrackingUsage(ET9BaseLingInfo * const pBaseLingInfo)
{
    pBaseLingInfo->wReserve1 = 0;
    pBaseLingInfo->wReserve2 = 0;
}

void ET9FARCALL _ET9Eval_UpdateUsage(ET9BaseLingInfo * const pBaseLingInfo)
{
    pBaseLingInfo->wReserve1 += 1;
}

ET9BOOL ET9FARCALL _ET9Eval_HasExpired(ET9BaseLingInfo * const pBaseLingInfo, const ET9U16 wUsageCount)
{
    if (wUsageCount >= ET9EVAL_MAX_LIFE_TIME_USAGE || /* max life time usage */
        pBaseLingInfo->wReserve1 >= ET9EVAL_MAX_SESSION_USAGE) { /* max session usage */

        ET9ClearAllSymbs(pBaseLingInfo->pWordSymbInfo);
        return 1;
    }

    return 0;
}

#endif

/*! @} */
/* ----------------------------------< eof >--------------------------------- */
