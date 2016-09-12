/*****************************************************************************
* COPYRIGHT ZI AND SUBJECT TO CONFIDENTIALITY RESTRICTIONS                   *
*                                                                            *
* This file is the confidential and proprietary property of Zi Corporation   *
* of Canada, Inc. ("Zi") or one of its affiliates.                           *
******************************************************************************/
#include "dltConfig.h"


const char g_compilerOptions[] = 

""

#if defined DISCLOSE_COMPILER_OPTIONS

#ifdef CFLAGS
"\t" CFLAGS "\n"
#endif

#ifdef EXTENDED_DATABASE
"\t" "-DEXTENDED_DATABASE\n"
#endif

#ifdef _WIN32
"\t" "-D_WIN32\n"
#endif

#ifdef ONLINUX
"\t" "-D_ONLINUX\n"
#endif

#ifdef __linux__
"\t" "-D__linux__\n"
#endif

#ifdef _DEBUG
"\t" "-D_DEBUG\n"
#endif

#ifdef NDEBUG
"\t" "-DNDEBUG\n"
#endif

#ifdef DEBUG
"\t" "-DDEBUG\n"
#endif

#ifdef DECUMA_DEBUG_MEMORY
"\t" "-DDECUMA_DEBUG_MEMORY\n"
#endif

#ifdef DECUMA_DEBUG_MEMORY_STATISTICS
"\t" "-DDECUMA_DEBUG_MEMORY_STATISTICS\n"
#endif

#ifdef DECUMA_DEBUG_MEMORY_WITH_STACKTRACE
"\t" "-DDECUMA_DEBUG_MEMORY_WITH_STACKTRACE\n"
#endif

#ifdef DECUMA_DEBUG_MEMORY_ENABLE_ALLOCATION_FAIL
"\t" "-DDECUMA_DEBUG_MEMORY_ENABLE_ALLOCATION_FAIL\n"
#endif

#ifdef DECUMA_DEBUG_MEMORY_CUSTOM_ALLOCATION
"\t" "-DDECUMA_DEBUG_MEMORY_CUSTOM_ALLOCATION\n"
#endif

#ifdef DECUMA_STACK_MEASURE
"\t" "-DDECUMA_STACK_MEASURE\n"
#endif

#ifdef DECUMA_NO_GLOBAL_VARIABLES
"\t" "-DDECUMA_NO_GLOBAL_VARIABLES\n"
#endif

#ifdef DECUMA_MEM_STAT_ENABLE
"\t" "-DDECUMA_MEM_STAT_ENABLE\n"
#endif

#ifdef DECUMA_ASSERT_OVERRIDE
"\t" "-DDECUMA_ASSERT_OVERRIDE\n"
#endif

#ifdef DECUMA_ASSERT_ENABLE
"\t" "-DDECUMA_ASSERT_ENABLE\n"
#endif

#endif /* DISCLOSE_COMPILER_OPTIONS */

;
