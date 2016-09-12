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
  decumaHWR session objects.

\******************************************************************/

#ifndef DECUMA_HWR_DATA_H
#define DECUMA_HWR_DATA_H

#if defined(_DEBUG) && !defined(DECUMA_HWR_C)
#error Direct data access in debug mode only allowed from decuma_hwr.c
#endif

#include "decumaConfig.h"

#ifdef UCR_ENGINE
#include "ucr.h"
typedef UCR_SESSION LIB_SESSION;
#elif defined(ASL_ENGINE)
#include "asl.h"
typedef ASL_SESSION LIB_SESSION;
#elif defined(CJK_ENGINE)
#include "cjk.h"
typedef CJK_SESSION LIB_SESSION;
#elif defined(T9WL_ENGINE)
#include "t9wl.h"
typedef T9WL_SESSION LIB_SESSION;
#else
#error Need to define one of the engines above
#endif 


typedef enum LOG_STATE_ {
	LOG_STATE_READY,
	LOG_STATE_RECOGNITION_DONE
} LOG_STATE;

typedef struct _LOG_DATA {
	DECUMA_LOG_STRING_FUNC * pfLogStringFunction;
	void                   * pUserData;
	LOG_STATE                state;
	int                      nStartArcIdx;
	int                      nArcsAdded;
	int                      nRecognitionStartArcIdx;
	int                      nRecognitionArcsUsed;
} LOG_DATA;


struct _DECUMA_SESSION
{
	DECUMA_SESSION_SETTINGS * pSessionSettings; /*Note that this pointer is used also by lib modules (asl.c ucr.c etc) */
	                                            /*So before freeing, make sure that the submodules will not reference this */
	DECUMA_MEM_FUNCTIONS memFunctions;

	/* In case allocating and changing to the new settings fail we need a copy of old settings */
	/* to revert to. Therefore we keep two copies. */

	int activeSettingsCopy; /* 0 or 1 indicating corresponding copies below */

	DECUMA_SESSION_SETTINGS sessionSettingsCopy0;
	DECUMA_UINT32 symbolCategoriesCopy0[DECUMA_MAX_CATEGORIES]; /* Symbol category array memory for charSet in sessionSettingCopy0 */
	DECUMA_UINT32 languagesCopy0[DECUMA_MAX_CATEGORIES]; /* Language category array memory for charSet in sessionSettingCopy0 */

	DECUMA_SESSION_SETTINGS sessionSettingsCopy1;
	DECUMA_UINT32 symbolCategoriesCopy1[DECUMA_MAX_CATEGORIES]; /* Symbol category array memory for charSet in sessionSettingCopy1 */
	DECUMA_UINT32 languagesCopy1[DECUMA_MAX_CATEGORIES]; /* Language category array memory for charSet in sessionSettingCopy1 */

	int bAddingArcs;

	LOG_DATA logData;	

	/*Module session */
	LIB_SESSION * pLibSession;
};

#endif /*DECUMA_HWR_DATA_H */

