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


#ifndef DATABASE_FORMAT_H
#define DATABASE_FORMAT_H (1)

#include "databaseStructTypes.h"
#include "decumaDataTypes.h"
#include "decumaString.h"
#include "decumaCategoryType.h"

#define MIN_KEY_NUMBER (MIN_INT8)
#define MAX_KEY_NUMBER (MAX_INT8)

#define MAX_ARC_ORDERS_PER_SYMBOL 16
#define MAX_NUMBER_OF_ARCS_IN_CURVE 6
#define MAX_ARCS_IN_DIACRITIC 3

#define NUMBER_OF_POINTS_IN_ARC 32
#define NUMBER_OF_POINTS_IN_INTERLEAVED_ARC 8
#define ALT_SYMBOL_SCALE_NUM 2
#define ALT_SYMBOL_SCALE_DENOM 1
#define ALT_SYMBOL_SCALE_THRESHOLD 2
#define ALT_SYMBOL_TRANSLATION -1

#define DC_STATUS_NOT_INITIALIZED 0
#define DC_STATUS_INITIALIZED 1
#define DC_STATUS_READY 2

#define DB_TYPE_STATIC 0
#define DB_TYPE_DYNAMIC 1

#define NUMBER_OF_CATEGORIES_IN_TABLE 64  /*64 bit categories are used */

/******************************************************************\
*                                                                  *
* The symbol DATABASE_FORMAT_VERSION_NR is used to verify that     *
* the database format is correct. Increment this number for every  *
* change in the database format.                                   *
*                                                                  *
* The UDM database format depends on parts of the static database  *
* format. If any of these parts are changed, the compatibility     *
* limits must be changed to the new static database version. If no *
* common parts are changed, keeping the two formats compatible,    *
* only the higher limit needs to be changed. The compatibility     *
* limits are defined in 'udm.h'.                                   *
*                                                                  *
\******************************************************************/
#define DATABASE_FORMAT_VERSION_NR 11

#define DBKEY_IS_GESTURE 1
#define DBKEY_IS_INSTANT_GESTURE 2

/*// The following struct holds information about keys that shall be rejected when
// the active category is part of "notAllowedCategory"
typedef struct tagBASEKEY_CATEGORY_PAIR{
	const UINT8 nBaseArcs;
	const UINT16 keyIndex;
	const CATEGORY notAllowedCategory;
} BASEKEY_CATEGORY_PAIR;

typedef struct  {
	short nSimilarSymbols; // Number of similar symbols-category pairs
	const BASEKEY_CATEGORY_PAIR *pKeyCategoryPair;
} BASEKEY_CATEGORY_PAIR_LIST;*/

#define SCALE_BITS_IN_UNKNOWN_PROPERTY 0x03
#define POSITION_BITS_IN_UNKNOWN_PROPERTY 0x30

#define N_MAX_CONFLICT_TYPES 4

/* These constants describe what bit position in the strokeMask that tells */
/* the algorithm that it may attach the named diacritic. */
/* For example, if bit number three is set, at strokeMask[0], it is allowed */
/* to use an acute accent above the symbol. */
enum databaseDiacriticAllographId
{
	/* Single strokes */
	DIAERISIS_singlestroke			= 1,	/* 0x0308 UNICODE */
	DIAERISIS_singlestroke_waved	= 2,	/* 0x0308 UNICODE */
	RING_ABOVE_onestroke			= 4,	/* 0x030A UNICODE */
	ACUTE_ACCENT_onestroke			= 8,	/* 0x0301 UNICODE (not supported in database) */

	/* Dual strokes. (indexs restart since they are in a different bit-mask) */
	DIAERISIS_doubledot				= 1,	/* 0x0308 UNICODE */
	DIAERISIS_doublestroke			= 2,	/* 0x0308 UNICODE */
	CANDRABINDU_twostrokes			= 4,	/* 0x0310 UNICODE (not supported in database) */
	DOUBLE_ACUTE_ACCENT_twostrokes	= 8	/* 0x030b UNICODE (not supported in database) */
};

enum databaseDiacriticId
{
	/* Single strokes */
	DIAERISIS,	/* 0x0308 UNICODE */
	RING_ABOVE,	/* 0x030A UNICODE */
	ACUTE_ACCENT,/* 0x0301 UNICODE (not supported in database) */
	CANDRABINDU,	/* 0x0310 UNICODE (not supported in database) */
	DOUBLE_ACUTE_ACCENT	/* 0x030b UNICODE (not supported in database) */
};

/*typedef struct{
	DECUMA_UNICODE ch1;
	DECUMA_UNICODE ch2;
} CH_PAIR;


typedef struct {
	int noPairs;
	const CH_PAIR * pPair;
} CH_PAIR_TABLE;

typedef struct {
	DECUMA_UNICODE ch1;
	DECUMA_UNICODE ch2;
	INT16 distance;
	CATEGORY_MASK symbolCategory; //NOTE that languageCat is ignored for simplicity, TODO!!
} CH_PAIR_DIST_CAT;*/

typedef const struct _CATEGORIES
{
	CATEGORY cat;
	CATEGORY altCat;
/*	CATEGORY conflictCat; */
} DECUMA_DB_STORAGE *CATEGORIES_PTR;

/* The following symbols do not seem to be referenced anywhere but scrZoom.c, where they now are defined 'static'
 *
 * extern const int noindexForZoom1;
 * extern const int indexForZoom1[];
 * extern const int startPointForZoom1[];
 * extern const int stopPointForZoom1[];
 */

#endif
