/*****************************************************************************
* COPYRIGHT ZI AND SUBJECT TO CONFIDENTIALITY RESTRICTIONS                   *
*                                                                            *
* This file is the confidential and proprietary property of Zi Corporation   *
* of Canada, Inc. ("Zi") or one of its affiliates.                           *
******************************************************************************/
#ifndef CJK_DATABASE_LIMITS_H_
#define CJK_DATABASE_LIMITS_H_

#include "decumaBasicTypes.h"


/* Some session members use this to avoid memory allocation of per 
   db-idx information */
#define MAX_NBR_DB_IDX_BIT_CHARS        3000
#define DLTDB_MAX_CLUSTER_TREE_HEIGHT     16

#define HANTBL_ISIN_JIS_1         0x01
#define HANTBL_ISIN_JIS_2         0x02
#define HANTBL_ISIN_GB_A          0x04
#define HANTBL_ISIN_GB_B          0x08
#define HANTBL_ISIN_BF_1          0x10
#define HANTBL_ISIN_BF_2          0x20
#define HANTBL_ISIN_HKSCS         0x40
#define HANTBL_ISIN_JIS_EXTENDED  0x80

#define CJK_UNIFIED_FIRST         0x4E00
#define CJK_UNIFIED_LAST          0x9FA5
#define CJK_UNIFIED_N            (CJK_UNIFIED_LAST-CJK_UNIFIED_FIRST+1)

#define HANGULTBL_ISIN_HANGUL_A   0x01
#define HANGULTBL_ISIN_HANGUL_B   0x02
#define HANGUL_FIRST              0xAC00
#define HANGUL_LAST               0xD7A3
#define HANGUL_N                 (HANGUL_LAST-HANGUL_FIRST+1)
#define NTRAD2SIMP                2401
#define NSIMP2TRAD                2122

#undef AM_FREQ
#define AM_FREQ                   0xF

#define AM_ISDENSE                0x10
#define LATINLIMIT                2
#define LENGTHLIMIT               10
#define MAXNSTRK                  50
#define MAXPNTSPERSTR             120
#define MAXPNTSPERCHAR            120
#define MAXNINDEXPERUNICODE       100
#define MAXNBRALLOGRAPHS          63000

#ifdef L_SIMPTRAD
#define L_SIMP
#define L_TRAD
#endif

#ifdef L_TRAD
#define FIRSTTWOBYTEINDEX 241
#else
#define FIRSTTWOBYTEINDEX 241 /* 244 */
#endif

#endif /* CJK_DATABASE_LIMITS_H_ */
