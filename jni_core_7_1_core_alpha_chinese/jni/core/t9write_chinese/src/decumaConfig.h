/* This is the default engine configuration file for T9Write CJK */

/* Include OEM configuration header */
#include "xxt9wOem.h"

#ifndef DECUMA_CONFIG_H
#define DECUMA_CONFIG_H

/* Mandatory configuration macros: 
 * 
 * CJK_ENGINE enables the recognition engine for CJK handwriting scripts
 * DECUMA_NO_RUNTIME_WARNINGS compiles out runtime warnings completely
 */

#define CJK_ENGINE

#define DECUMA_SPECIFIC_DB_STORAGE 0

/* The following macros are used to hide internal symbols when building the
 * library as a single C file. However, if we build it "normally" they must 
 * have some definition otherwise we'll break the build.
 */

/* Type qualifier for function prototypes and definitions 
 * This is defined to "static" in t9write_rel.c
 */
#ifndef DECUMA_HWR_PRIVATE
#define DECUMA_HWR_PRIVATE
#endif

/* Type qualifier for data fields, different macros for use in the header file
 * and the actual implementation in the C file.
 * Both are defined "static" in t9write_rel.c, but for a normal build we need
 * the header file to add the "extern" qualifier.
 */
#ifndef DECUMA_HWR_PRIVATE_DATA_H
#define DECUMA_HWR_PRIVATE_DATA_H extern
#endif

#ifndef DECUMA_HWR_PRIVATE_DATA_C
#define DECUMA_HWR_PRIVATE_DATA_C 
#endif

#endif /* DECUMA_CONFIG_H */
 
