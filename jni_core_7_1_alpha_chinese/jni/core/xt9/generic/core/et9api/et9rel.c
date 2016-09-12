/*******************************************************************************
;*******************************************************************************
;**                                                                           **
;**                  COPYRIGHT 1998-2011 NUANCE COMMUNICATIONS                **
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
;**     FileName: et9rel.c                                                    **
;**                                                                           **
;**  Description: Example XT9 release build file -- gather up all files       **
;**               apropriate for a particular build into one .OBJ file.       **
;**                                                                           **
;*******************************************************************************
;******************************************************************************/

#include "et9api.h" /* This can modify the definitions used below. */

#ifdef _WIN32
#pragma message ("=== XT9 Generic Module ===")
#endif

/* Utility Modules */
#include "et9misc.c"
#include "et9sym.c"
#include "et9sys.c"

/* Input Modules */
#include "et9imu.c"
#include "et9kdb.c"

#ifdef ET9_ALPHABETIC_MODULE
#ifdef _WIN32
#pragma message ("=== XT9 Alphabetic Module Enabled ===")
#endif

/* Utility Modules */
#include "et9amisc.c"
#include "et9asym.c"
#include "et9asys.c"

/* Input Modules */
#include "et9aimu.c"

/* Linguistic Modules */
#include "et9aasdb.c"
#include "et9acdb.c"
#include "et9adb.c"
#include "et9aldb.c"
#include "et9alsasdb.c"
#include "et9amdb.c"
#include "et9arudb.c"
#include "et9aslst.c"
#include "et9aspc.c"

#endif /* ET9_ALPHABETIC_MODULE */


#ifdef ET9_JAPANESE_MODULE
#ifdef _WIN32
#pragma message ("=== XT9 Japanese Module Enabled ===")
#endif

/* Utility Modules */
#include "et9jsys.c"
#include "et9jkana.c"

/* Linguistic Modules */
#include "et9jldb.c"
#include "et9jrudb.c"
#include "et9jslst.c"

#endif /* ET9_JAPANESE_MODULE */


#ifdef ET9_CHINESE_MODULE
#ifdef _WIN32
#pragma message ("=== XT9 Chinese Module Enabled ===")
#endif

#include "et9cprel.c"
#endif /* ET9_CHINESE_MODULE */

#ifdef ET9_KOREAN_MODULE
#ifdef _WIN32
#pragma message ("=== XT9 Korean Module Enabled ===")
#endif

#include "et9krel.c"
#endif /* ET9_KOREAN_MODULE */

#ifdef ET9_NAV_ALPHABETIC_MODULE
#ifdef _WIN32
#pragma message ("=== XT9 NAV Alphabetic Module Enabled ===")
#endif

#include "et9navcore.c"
#include "et9navindex.c"
#include "et9navinput.c"
#include "et9navmatch.c"
#include "et9navquery.c"
#include "et9navrecord.c"
#include "et9navtype.c"
#include "et9navutil.c"
#include "et9navmem.c"
#include "et9navstore.c"
#include "et9navlookup.c"
#include "et9navcache.c"

#endif /* ET9_NAV_ALPHABETIC_MODULE */

#ifdef EVAL_BUILD
#include "__et9eval.c"
#endif

/* ----------------------------------< eof >--------------------------------- */
