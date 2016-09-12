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

      File: decumaAssert.h
   Package: This file is part of the package generic
 $Revision: 1.4 $
     $Date: 2011/02/14 11:15:16 $
   $Author: jianchun_meng $

\******************************************************************/

#ifndef INCLUDED_DECUMA_ASSERT_H
#define INCLUDED_DECUMA_ASSERT_H


#include "decumaConfig.h"

#if defined(DECUMA_ASSERT_ENABLE)
	
	#if defined(DECUMA_ASSERT_DISABLE)
		#undef DECUMA_ASSERT_DISABLE
	#endif

	#if !defined(_DEBUG)
		#define __DEBUG_DEFINED
		#define _DEBUG
	#endif

#endif

/* Note: asserts are not enabled unless the macro DECUMA_ASSERT_ENABLE is defined. */

#ifndef __DOXYGEN__ /* Documentation below */

#ifdef DECUMA_ASSERT_OVERRIDE
	#define decumaAssert(e) decumaAssertFunction(#e, !!(e), __FILE__, __LINE__)

	/* definition has to be done elsewhere */
	void decumaAssertFunction(const char * str, int e, const char * filename, int line);

#elif !defined(DECUMA_ASSERT_DISABLE) && defined (_DEBUG)
	/* Please leave __SYMBIAN32_ on top, because there are 
	 * several others that may apply when compiling for symbian.
	 * (Such as __WIN32__...)
	 */
	#if defined (__SYMBIAN32__)
/*		#include <assert.h>
 *		#define decumaAssert(x) assert(x);
 */
		#ifdef __cplusplus
		extern "C" {
		#endif
		void assertMessageBox(const char *pFile, int nLine, const char *pCondition);
		#ifdef __cplusplus
		} /* extern "C" */
		#endif
		#define decumaAssert( x ) if ( !(x) ) assertMessageBox( __FILE__, __LINE__, #x )
	#elif defined(_WIN32_WCE)
		/* Use a global buffer for building the assert string. If the assertMessageBox
		 * macro would create a buffer, several invokations of that macro in a function
		 * would cause multiple of buffers beeing stored on the stack. (yes.. it is a problem!)
		 */
		#include <windows.h>
		static TCHAR s_assertMsg[256];
		#define assertMessageBox( file, line, assertion ) \
		do { \
			_sntprintf( s_assertMsg,sizeof(s_assertMsg) / sizeof(s_assertMsg[0]), \
			_T("Assertion failed: %s\nFile: %s\nLine: %d"), assertion, file, line ); \
			MessageBox(GetActiveWindow(), s_assertMsg, _T("HWR assert!"), MB_OK | MB_SETFOREGROUND ); \
		} while(0)
		#if defined(_WIN32_WCE_EMULATION)
			#define decumaAssert(x) \
			if (!(x)) \
			{\
				assertMessageBox( _T(__FILE__) , __LINE__, _T(#x));\
				__asm {int 3};/*equals __debugBreak*/\
			}
		#else
			#define decumaAssert( x ) if ( !(x) ) assertMessageBox( _T(__FILE__) , __LINE__, _T(#x))
		#endif
	#elif defined(ONPALM_ARMLET)
		void assertMessageBox(char * file, int line, char * assertion );

		#define decumaAssert( x ) if ( !(x) ) assertMessageBox( __FILE__, __LINE__, #x)

	#elif defined( ONPALM_6 )
		#include <ErrorMgr.h>
/*		#define decumaAssert( x ) if ( !(x) ) ErrDisplayFileLineMsg(__FILE__,__LINE__,#x) */
		#define decumaAssert( x ) DbgOnlyFatalErrorIf(!(x), #x " " __FILE__)
	#elif defined( ONPALM_4 ) || defined( ONPALM_5 )
		#include <PalmOS.h>
		#undef NDEBUG
		#ifndef ASSERT_AS_FUNCTION

		#ifdef DUI_TRACE_ON
		#include "duiTrace.h"
		#else
		#define DUI_TRACE(y,x)
		#endif

			#define assertMessageBox( file, line, assertion ) \
			do { \
				char * p = (char*) MemPtrNew(256); \
				StrPrintF(p, \
				"Assertion failed: %s\nFile: %s\nLine: %d", assertion, file, line ); \
				DUI_TRACE(0,  p ); \
				SysFatalAlert(p); \
				MemPtrFree(p); \
			} while(0)
		#else
		extern
			#ifdef __cplusplus
			"C"
			#endif
		void assertMessageBox(char * file, int line, char * assertion );
		#endif
		#define decumaAssert( x ) if ( !(x) ) assertMessageBox(__FILE__,__LINE__,#x)
	#elif defined(_WIN32)
		#include <assert.h>
		#define decumaAssert assert
	#elif defined(ONLINUXONARM9)
		#define decumaAssert(x)
	#elif defined(ONLINUX)
		#include <assert.h>
		#define decumaAssert assert
	#else
		#include <assert.h>
		#define decumaAssert assert
	#endif
#else /* if defined(DECUMA_ASSERT_ENABLE) */
	#define decumaAssert(x)
#endif

#ifdef __DEBUG_DEFINED
#undef _DEBUG
#endif


#else /* __DOXYGEN__ */

/* This section is for documentation purposes only */

/** @addtogroup DOX_GRP_UTILS
 *  @{
 */

/** @defgroup DOX_GRP_UTILS_ASSERT decumaAssert
 *  @{
 */

/** Platform dependent assert, enabled if @ref DECUMA_ASSERT_ENABLE is defined.
 *  @param e Expression to assert
 */
#define decumaAssert(e)

/** Calls to @ref decumaAssert will be replaced with calls to this user defined function if 
 *  @ref DECUMA_ASSERT_OVERRIDE is defined. Not affected by @ref DECUMA_ASSERT_ENABLE.
 *
 *  @param e_str      The verbatim expression asserted
 *  @param e          The value of the expression
 *  @param filename   The filename where the assert is made
 *  @param line       The line number where the assert is made
 */
void decumaAssertFunction(const char * e_str, int e, const char * filename, int line);

/** @} */
/** @} */

#endif /* __ DOXYGEN__ */

#endif /* INCLUDED_DECUMA_ASSERT_H */
