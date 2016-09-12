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
;**     FileName: __et9eval.h                                                 **
;**                                                                           **
;**  Description: eval code for ET9                                           **
;**                                                                           **
;*******************************************************************************
;******************************************************************************/

#ifndef __ET9EVAL_H
#define __ET9EVAL_H    1

#include "et9api.h"

/* don't mangle the function name if compile under C++ */
#if defined(__cplusplus)
extern "C" {
#endif


/** \internal
 * Define how many words can be entered using xt9 per session.  Session is started when
 * Setlanguage is called.  We're not starting session during sysinit since this
 * function may only be called once.  Setlanguage however, is called everytime a text
 * session begins, at least with android amd wm intergration.
 */

#define ET9EVAL_MAX_SESSION_USAGE 25

/** \internal
 * Define how may words can be entered using xt9 per life time.  This value must be less than
 * ET9UDBMAXUPDATE. Using #if (ET9EVAL_MAX_LIFE_TIME_USAGE > ET9UDBMAXUPDATE) to enforce this rule will not
 * work since there is type casting in the ET9UDBMAXUPDATE define which cause compiler error.
 */

#define ET9EVAL_MAX_LIFE_TIME_USAGE 300

void ET9FARCALL _ET9Eval_StartTrackingUsage(ET9BaseLingInfo * const pBaseLingInfo);
void ET9FARCALL _ET9Eval_UpdateUsage(ET9BaseLingInfo * const pBaseLingInfo);
ET9BOOL ET9FARCALL _ET9Eval_HasExpired(ET9BaseLingInfo * const pBaseLingInfo, const ET9U16 wUsageCount);


/* End don't mangle the function name if compile under C++ */
#if defined (__cplusplus)
    }
#endif


#endif /* !__ET9EVAL_H */
/* ----------------------------------< eof >--------------------------------- */
