/*******************************************************************************
;*******************************************************************************
;**                                                                           **
;**                    COPYRIGHT 2001-2011 NUANCE COMMUNICATIONS              **
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
;**     FileName: et9asys.h                                                   **
;**                                                                           **
;**  Description: Alphabetic system routines header file.                     **
;**                                                                           **
;*******************************************************************************
;******************************************************************************/

#ifndef ET9ASYS_H
#define ET9ASYS_H    1

#include "et9api.h"
#ifdef ET9_ALPHABETIC_MODULE

/* don't mangle the function name if compile under C++ */
#if defined(__cplusplus)
extern "C" {
#endif

/* dev state bits for inhibiting features */

#define ET9DEVSTATEINHBEXACTLOCK                        0       /**< \internal dev state bit for inhibiting the exact lock feature */
#define ET9DEVSTATEINHBNOMDBNWP                         1       /**< \internal dev state bit for inhibiting the prevention of MDB NEWP support */
#define ET9DEVSTATEINHBTRACECMPLDEMOTE                  2       /**< \internal dev state bit for inhibiting the trace completion demote */

#define ET9DEVSTATEINHBEXACTLOCKMASK                    (1L << ET9DEVSTATEINHBEXACTLOCK)
#define ET9DEVSTATEINHBNOMDBNWPMASK                     (1L << ET9DEVSTATEINHBNOMDBNWP)
#define ET9DEVSTATEINHBTRACECMPLDEMOTEMASK              (1L << ET9DEVSTATEINHBTRACECMPLDEMOTE)

#define ET9DEVSTATEINHBEXACTLOCK_MODE(dwState)          ((dwState) & ET9DEVSTATEINHBEXACTLOCKMASK)
#define ET9DEVSTATEINHBNOMDBNWP_MODE(dwState)           ((dwState) & ET9DEVSTATEINHBNOMDBNWPMASK)
#define ET9DEVSTATEINHBTRACECMPLDEMOTE_MODE(dwState)    ((dwState) & ET9DEVSTATEINHBTRACECMPLDEMOTEMASK)

ET9STATUS ET9FARCALL ET9AWSetPrimaryFence(ET9AWLingInfo * const pLingInfo,
                                          const ET9U8           bFence);

ET9STATUS ET9FARCALL ET9AWSetSecondaryFence(ET9AWLingInfo * const   pLingInfo,
                                            const ET9U8             bFence);

void ET9FARCALL _ET9AWSelLstResetWordList(ET9AWLingInfo *pLingInfo);

/* flex features */

#define ET9_FLEX_FEATURE_ACTIVATE_BIT               0       /**< \internal  */
#define ET9_FLEX_FEATURE_FREE_DOUBLE_BIT            1       /**< \internal  */
#define ET9_FLEX_FEATURE_FREE_PUNCT_BIT             2       /**< \internal  */
#define ET9_FLEX_FEATURE_SPC_COMPL_BIT              3       /**< \internal  */

#define ET9_FLEX_FEATURE_ACTIVATE_MASK              (1 << ET9_FLEX_FEATURE_ACTIVATE_BIT)
#define ET9_FLEX_FEATURE_FREE_DOUBLE_MASK           (1 << ET9_FLEX_FEATURE_FREE_DOUBLE_BIT)
#define ET9_FLEX_FEATURE_FREE_PUNCT_MASK            (1 << ET9_FLEX_FEATURE_FREE_PUNCT_BIT)
#define ET9_FLEX_FEATURE_SPC_COMPL_MASK             (1 << ET9_FLEX_FEATURE_SPC_COMPL_BIT)

#define ET9_FLEX_FEATURE_ALL_MASK                   0xFF

#define ET9_FLEX_FEATURE_ACTIVATE_MODE(bState)      ((bState) & ET9_FLEX_FEATURE_ACTIVATE_MASK)
#define ET9_FLEX_FEATURE_FREE_DOUBLE_MODE(bState)   ((bState) & ET9_FLEX_FEATURE_FREE_DOUBLE_MASK)
#define ET9_FLEX_FEATURE_FREE_PUNCT_MODE(bState)    ((bState) & ET9_FLEX_FEATURE_FREE_PUNCT_MASK)
#define ET9_FLEX_FEATURE_SPC_COMPL_MODE(bState)     ((bState) & ET9_FLEX_FEATURE_SPC_COMPL_MASK)


ET9STATUS ET9FARCALL ET9AWSysSetSpellCorrectionFeatures(ET9AWLingInfo       * const pLingInfo,
                                                        const ET9U8         bFeatures);

ET9STATUS ET9FARCALL _ET9AWSysSetSelectionListMode(ET9AWLingInfo * const pLingInfo,
                                                   const ET9ASLMODE      eMode);


/* End don't mangle the function name if compile under C++ */
#if defined (__cplusplus)
    }
#endif

#endif /* ET9_ALPHABETIC_MODULE */
#endif /* !ET9ASYS_H */


/* ----------------------------------< eof >--------------------------------- */
