/*******************************************************************************
;*******************************************************************************
;**                                                                           **
;**                    COPYRIGHT 2004-2011 NUANCE COMMUNICATIONS              **
;**                                                                           **
;**               NUANCE COMMUNICATIONS PROPRIETARY INFORMATION               **
;**                                                                           **
;**     This software is supplied under the terms of a license agreement      **
;**     or non-disclosure agreement with Nuance Communications and may not    **
;**     be copied or disclosed except in accordance with the terms of that    **
;**     agreement.                                                            **
;**                                                                           **
;**                                                                           **
;*******************************************************************************
;**                                                                           **
;**     FileName: et9cpcntx.c                                                  **
;**                                                                           **
;**  Description: Chinese XT9 context predication module.                     **
;**               Conforming to the development version of Chinese XT9.               **
;**                                                                           **
;*******************************************************************************
;******************************************************************************/

#include "et9cpapi.h"
#include "et9cpldb.h"
#include "et9cppbuf.h"
#include "et9cpkey.h"
#include "et9cpinit.h"
#include "et9cpsys.h"
#include "et9cpspel.h"
#include "et9cpcntx.h"
#include "et9cpname.h"
#include "et9cpwdls.h"
#include "et9cpmisc.h"

/*
   update the context buffer after a phrase is selected
   input: psPhrase - the selected phrase (in PID/SID)
 */
void ET9FARCALL ET9_CP_UpdateContextBuf(ET9CPLingInfo *pET9CPLingInfo,
                                     ET9CPPhrase *psPhrase)
{
    ET9U16 *pwSrc, *pwDst;
    ET9U8 *pbContextLen;
    ET9U8 b, bLen, bCount;

    pbContextLen = pET9CPLingInfo->CommonInfo.pbContextLen;
    /* flush any external context so that internal/external context won't
     * coexist in the context history */
    if (pET9CPLingInfo->CommonInfo.bExtContext) {
        ET9_CP_ClrContextBuf(pET9CPLingInfo);
    }
    bLen = psPhrase->bLen;
    if (bLen >= ET9CPMAXPHRASESIZE) {
        /* clr context if the selection has max phrase length. */
        ET9_CP_ClrContextBuf(pET9CPLingInfo);
        return;
    }
    /* find the oldest history to keep */
    bCount = 0;
    for (b = 0; pbContextLen[b]; b++) {
        ET9Assert(b < ET9_CP_MAX_CONTEXT_HISTORY);
    }
    for (; b; b--) {
        ET9Assert(pbContextLen[b - 1]);
        if (((bLen + pbContextLen[b - 1]) < ET9CPMAXPHRASESIZE) &&
            (bCount < ET9_CP_MAX_CONTEXT_HISTORY - 1)) {
            bLen = (ET9U8)(bLen + pbContextLen[b - 1]);
            bCount++;
        }
        else {
            break;
        }
    }
    pwDst = pET9CPLingInfo->CommonInfo.pwContextBuf;
    if (0 == b) {
        /* keep all history, append selection after the newest history */
        for (b = 0; b < bCount; b++) {
            ET9Assert(pbContextLen[b]);
            pwDst += pbContextLen[b];
        }
    }
    else {
        ET9U8 bOldest;
        /* shift wanted history to left, displace unwanted ones */
        bOldest = b;
        pwSrc = pwDst;
        for (b = 0; b < bOldest; b++) {
            ET9Assert(pbContextLen[b]);
            pwSrc += pbContextLen[b];
        }
        for (b = bOldest; b < bOldest + bCount; b++) {
            for (bLen = pbContextLen[b]; bLen; bLen--) {
                *pwDst++ = *pwSrc++;
            }
            pbContextLen[b - bOldest] = pbContextLen[b];
        }
        b = bCount;
    }
    /* copy current selection into context buf */
    pwSrc = psPhrase->pSymbs;
    bLen = psPhrase->bLen;
    ET9Assert(b < ET9_CP_MAX_CONTEXT_HISTORY);
    pbContextLen[b] = bLen; /* copy current selection len into context len */
    pbContextLen[b + 1] = 0; /* 0-termination */
    for (b = 0; b < bLen; b++) {
        *pwDst++ = *pwSrc++;
    }
} /* ET9_CP_UpdateContextBuf() */

void ET9FARCALL ET9_CP_ClrContextBuf(ET9CPLingInfo *pET9CPLingInfo)
{
    pET9CPLingInfo->CommonInfo.pbContextLen[0] = 0;
    pET9CPLingInfo->CommonInfo.bExtContext = 0; /* clr external context flag */
}

/* ----------------------------------< eof >--------------------------------- */
