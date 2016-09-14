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
;**     FileName: et9cprel.c                                                  **
;**                                                                           **
;**     Description: Include Chinese XT9 files                                **
;**                                                                           **
;*******************************************************************************
;******************************************************************************/

#include "et9cpinit.c"
#include "et9cpsys.c"
#include "et9cpwdls.c"

#include "et9cpcntx.c"
#include "et9cppbuf.c"
#include "et9cpudb.c"
#include "et9cpname.c"

#include "et9cpbpmf.c"
#include "et9cpspel.c"
#include "et9cptone.c"
#include "et9cpphse.c"
#include "et9cptrace.c"
#ifndef _64K_LIMIT_

#ifndef ET9CP_DISABLE_STROKE
#include "et9cpstrk.c"
#include "et9cpsldb.c"
#endif

#include "et9cpldb.c"

#include "et9cprdb.c"

#include "et9cstrie.c"

#include "et9cpcj.c"
#include "et9cpcnvt.c"

#endif
/* ----------------------------------< eof >--------------------------------- */
