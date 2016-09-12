
#ifndef __XXET9OEM_H__
#define __XXET9OEM_H__


/*
 * The following defines which module we want to include in our builds.
 */
#define ET9_KDB_MODULE
#define ET9_KDB_TRACE_MODULE
#define ET9_ALPHABETIC_MODULE
#define ET9_CHINESE_MODULE
#define T9WRITE_ALPHA
#define T9WRITE_CHINESE

//#define ET9_DEBUGLOG2
//#define ET9_DEBUGLOG6
//#define ET9_DEBUGLOG6B

#define ET9CPMAXPAGESIZE 64
#define ET9MAXUDBWORDSIZE 64
//#define ET9MAXSELLISTSIZE 32

/* speed up with direct DB access */

#ifndef ET9_DIRECT_LDB_ACCESS
#define ET9_DIRECT_LDB_ACCESS
#endif

#ifndef ET9_DIRECT_KDB_ACCESS
#define ET9_DIRECT_KDB_ACCESS
#endif

/* specify OEM ID */
#define ET9OEMID             10000

#endif /* __XXSTOEM_H__ */
