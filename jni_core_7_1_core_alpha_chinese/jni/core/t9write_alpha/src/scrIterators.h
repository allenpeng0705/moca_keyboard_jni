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


#ifndef scrIterators_h_cmjlakcvsdnvsdvnssjcvljslvjxjhv
#define scrIterators_h_cmjlakcvsdnvsdvnssjcvljslvjxjhv

/*$Header: /data/cvsroot/seattle/t9write/alphabetic/core/src/scrIterators.h,v 1.5 2011/02/14 11:41:14 jianchun_meng Exp $*/

#ifdef DECUMA_MANGLE
#include "mangle.h"
#endif

#include "decumaDataTypes.h"
#include "scrOutput.h"
#include "scrFullSearch.h"

typedef struct
{
	DECUMA_UINT32 diacMask;
	int nStop;
	KID kid;
} DIAKIT;

DECUMA_HWR_PRIVATE void diakitCreate(DIAKIT * pIt, KID * kid, int nDiacArcs);
DECUMA_HWR_PRIVATE DECUMA_BOOL diakitGoNext(DIAKIT * pIt);

typedef struct
{
	KID kid;
	int nDiacArcs;
	int nStop;
	DECUMA_BOOL usePrecalculated;
	DECUMA_BOOL checkBaseKeysWithPoorShape;
	const scrOUTPUT_LIST * precalculatedOutputs;
	int counter;
} KIT;

#define KIT_GET_COUNTER(pIt) ((pIt)->counter)

DECUMA_HWR_PRIVATE void kitCreate(KIT * kit, int nBaseArcs, int nDiacArcs,
			   const SEARCH_SETTINGS * pSS, const scrOUTPUT_LIST * precalculatedBaseKeys);
DECUMA_HWR_PRIVATE DECUMA_BOOL kitGoNext(KIT * pIt);

/* Iterate over all one-arc symbols without diacritics.
scrOUTPUT_LIST temp;
temp.noOutputs = -1;

KIT k = kitCreate(1,0,db,temp);
kid = GoNext(&k);
while(databaseIsValidKID(kid)!= 0)
{
	DoSomeStuff(kid);
	kid = GoNext(&k);
}*/

/* Return a pointer to the precalculated scr output if any. */
DECUMA_HWR_PRIVATE scrOUTPUT * kitGetSCROut(KIT * pIt);

DECUMA_HWR_PRIVATE void kidSetDiacKeyOffset(KID * pKid, DECUMA_INT16 diacKeyXOffset, DECUMA_INT16 diacKeyYOffset);

#endif
