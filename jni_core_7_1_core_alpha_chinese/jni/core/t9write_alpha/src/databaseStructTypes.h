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


#ifndef _STRUCT_TYPES_H
#define _STRUCT_TYPES_H (1)

#include "decumaBasicTypes.h"
#include "decumaStorageSpecifiers.h"

#define DUMMY_VALUE 80
#define FORMAT_VERSION 11

typedef const char DECUMA_DB_STORAGE * DECUMA_DB_ADDRESS;

typedef const DECUMA_INT8 DECUMA_DB_STORAGE * DECUMA_INT8_DB_PTR;

typedef const DECUMA_UINT16 DECUMA_DB_STORAGE * DECUMA_UINT16_DB_PTR;

typedef const DECUMA_UINT16 DECUMA_DB_STORAGE * DECUMA_UNICODE_DB_PTR;

typedef const struct _STATIC_DB_HEADER {
	DECUMA_UINT16 formatVersionNbr;
	DECUMA_UINT8 dummy_1;
	DECUMA_UINT8 dummy_2;
	DECUMA_UINT32 keyDBOffset;
	DECUMA_UINT32 propertyDBOffset;
} DECUMA_DB_STORAGE *STATIC_DB_HEADER_PTR;

#define STATIC_DB_HEADER_VALID_DUMMY_FIELDS(pX) ((pX)->dummy_1==DUMMY_VALUE && (pX)->dummy_2==DUMMY_VALUE )
#define SET_STATIC_DB_HEADER_DUMMY_FIELDS(pX) {(pX)->dummy_1=DUMMY_VALUE; (pX)->dummy_2=DUMMY_VALUE; }



/*======================== */
typedef const struct _PROPERTY_DB_HEADER {
	DECUMA_UINT16 nDiacSymbols;
	DECUMA_UINT16 nDiacKeyLists;
	DECUMA_UINT32 diacListsOffset;
	DECUMA_UINT32 diacCurveListsOffset;
	DECUMA_UINT32 diacSymbolTableOffset;
	DECUMA_UINT32 rotationTableOffset;
	DECUMA_UINT32 symmetryTableOffset;
	DECUMA_UINT32 arcOrderOffset;
	DECUMA_UINT32 allAOSetsOffset;
	DECUMA_UINT32 diacMasksOffset;
	DECUMA_UINT32 combTableOffset;
	DECUMA_UINT32 typeConflictsOffset;
	DECUMA_UINT32 zoomInfoListsOffset;
	DECUMA_UINT32 smallKeyOffset;
	DECUMA_UINT32 minDistanceOffset;
	DECUMA_UINT16 nTypeConflicts;
	DECUMA_UINT8 distBaseLine2HelpLine;
	DECUMA_UINT8 nDiacTypes;
	DECUMA_UINT8 altSymbolScaleNum;
	DECUMA_UINT8 altSymbolScaleDenom;
	DECUMA_INT8 altSymbolScaleThreshold;
	DECUMA_INT8 altSymbolTranslation;
	DECUMA_UINT32 svmDBHeaderOffset;
} DECUMA_DB_STORAGE *PROPERTY_DB_HEADER_PTR;

#define PROPERTY_DB_HEADER_VALID_DUMMY_FIELDS(pX) (1)
#define SET_PROPERTY_DB_HEADER_DUMMY_FIELDS(pX) {}



/*======================== */
typedef const struct _ROTATION {
	DECUMA_INT16 leftThresholdAngle;
	DECUMA_INT16 leftPunishment;
	DECUMA_INT16 rightThresholdAngle;
	DECUMA_INT16 rightPunishment;
} DECUMA_DB_STORAGE *ROTATION_PTR;

#define ROTATION_VALID_DUMMY_FIELDS(pX) (1)
#define SET_ROTATION_DUMMY_FIELDS(pX) {}



/*======================== */
typedef const struct _ARC_ORDER_SET {
	DECUMA_UINT16 nArcOrders;
	DECUMA_UINT8 dummy_1;
	DECUMA_UINT8 dummy_2;
	DECUMA_UINT32 arcOrdersOffset;
} DECUMA_DB_STORAGE *ARC_ORDER_SET_PTR;

#define ARC_ORDER_SET_VALID_DUMMY_FIELDS(pX) ((pX)->dummy_1==DUMMY_VALUE && (pX)->dummy_2==DUMMY_VALUE )
#define SET_ARC_ORDER_SET_DUMMY_FIELDS(pX) {(pX)->dummy_1=DUMMY_VALUE; (pX)->dummy_2=DUMMY_VALUE; }



/*======================== */
typedef const struct _DIAC_SYMBOL_INFO {
	DECUMA_UINT32 diacSymbolOffset;
} DECUMA_DB_STORAGE *DIAC_SYMBOL_INFO_PTR;

#define DIAC_SYMBOL_INFO_VALID_DUMMY_FIELDS(pX) (1)
#define SET_DIAC_SYMBOL_INFO_DUMMY_FIELDS(pX) {}



/*======================== */
typedef const struct _TYPE_CONFLICTS {
	DECUMA_UINT16 typeIdx;
	DECUMA_UINT8 nConflicts;
	DECUMA_UINT8 dummy_1;
	DECUMA_UINT16 conflicts[4];
} DECUMA_DB_STORAGE *TYPE_CONFLICTS_PTR;

#define TYPE_CONFLICTS_VALID_DUMMY_FIELDS(pX) ((pX)->dummy_1==DUMMY_VALUE )
#define SET_TYPE_CONFLICTS_DUMMY_FIELDS(pX) {(pX)->dummy_1=DUMMY_VALUE; }



/*======================== */
typedef const struct _KEY_DIACRITIC {
	DECUMA_UINT16 diacSymbol;
	DECUMA_UINT8 diacSymbolNr;
	DECUMA_UINT8 symmetryIdx;
	DECUMA_UINT8 arcOrderSetIdx;
	DECUMA_UINT8 arcRotationMask;
	DECUMA_UINT8 placementMask;
	DECUMA_UINT8 dummy_1;
	DECUMA_UINT16 diacCurveIndex;
	DECUMA_UINT16 xWidth;
	DECUMA_INT16 sumX;
	DECUMA_INT16 sumY;
	DECUMA_UINT32 qSum;
	DECUMA_UINT32 dqSum;
	DECUMA_UINT32 interleavedQSum;
	DECUMA_UINT32 interleavedNorm;
	DECUMA_INT16 interleavedSumX;
	DECUMA_INT16 interleavedSumY;
} DECUMA_DB_STORAGE *KEY_DIACRITIC_PTR;

#define KEY_DIACRITIC_VALID_DUMMY_FIELDS(pX) ((pX)->dummy_1==DUMMY_VALUE )
#define SET_KEY_DIACRITIC_DUMMY_FIELDS(pX) {(pX)->dummy_1=DUMMY_VALUE; }



/*======================== */
typedef const struct _KEY_DIACRITIC_LIST {
	DECUMA_UINT16 nKeys;
	DECUMA_UINT8 dummy_1;
	DECUMA_UINT8 dummy_2;
	DECUMA_UINT32 diacKeysOffset;
} DECUMA_DB_STORAGE *KEY_DIACRITIC_LIST_PTR;

#define KEY_DIACRITIC_LIST_VALID_DUMMY_FIELDS(pX) ((pX)->dummy_1==DUMMY_VALUE && (pX)->dummy_2==DUMMY_VALUE )
#define SET_KEY_DIACRITIC_LIST_DUMMY_FIELDS(pX) {(pX)->dummy_1=DUMMY_VALUE; (pX)->dummy_2=DUMMY_VALUE; }



/*======================== */
typedef const struct _SMALL_KEY {
	DECUMA_INT16 smallKeyNoHelplines[4];
	DECUMA_INT16 smallKeyBelowHelplineUpper[4];
	DECUMA_INT16 smallKeyBelowHelplineLower[4];
	DECUMA_INT16 smallKeyAboveHelpline[4];
	DECUMA_INT16 smallKeyRelativeChars[4];
} DECUMA_DB_STORAGE *SMALL_KEY_PTR;

#define SMALL_KEY_VALID_DUMMY_FIELDS(pX) (1)
#define SET_SMALL_KEY_DUMMY_FIELDS(pX) {}



/*======================== */
typedef const struct _MIN_DISTANCES {
	DECUMA_INT16 gestureMinDist;
	DECUMA_INT16 defaultCharMinDist;
	DECUMA_UINT8 charDistTableLength;
	DECUMA_UINT8 dummy_1;
	DECUMA_UINT8 dummy_2;
	DECUMA_UINT8 dummy_3;
	DECUMA_UINT32 charDistTableOffset;
} DECUMA_DB_STORAGE *MIN_DISTANCES_PTR;

#define MIN_DISTANCES_VALID_DUMMY_FIELDS(pX) ((pX)->dummy_1==DUMMY_VALUE && (pX)->dummy_2==DUMMY_VALUE && (pX)->dummy_3==DUMMY_VALUE )
#define SET_MIN_DISTANCES_DUMMY_FIELDS(pX) {(pX)->dummy_1=DUMMY_VALUE; (pX)->dummy_2=DUMMY_VALUE; (pX)->dummy_3=DUMMY_VALUE; }



/*======================== */
typedef const struct _CH_PAIR_DIST_CAT {
	DECUMA_UINT16 symbol1;
	DECUMA_UINT16 symbol2;
	DECUMA_INT16 charDistance;
	DECUMA_UINT8 dummy_1;
	DECUMA_UINT8 dummy_2;
	DECUMA_UINT32 conflictCategory[2];
} DECUMA_DB_STORAGE *CH_PAIR_DIST_CAT_PTR;

#define CH_PAIR_DIST_CAT_VALID_DUMMY_FIELDS(pX) ((pX)->dummy_1==DUMMY_VALUE && (pX)->dummy_2==DUMMY_VALUE )
#define SET_CH_PAIR_DIST_CAT_DUMMY_FIELDS(pX) {(pX)->dummy_1=DUMMY_VALUE; (pX)->dummy_2=DUMMY_VALUE; }



/*======================== */
typedef const struct _CH_PAIR_ZOOM_INFO {
	DECUMA_UINT16 symbol1;
	DECUMA_UINT16 symbol2;
 	DECUMA_UINT8 type1;
 	DECUMA_UINT8 type2;
 	DECUMA_UINT8 arcOrderIdx1;
 	DECUMA_UINT8 arcOrderIdx2;
	DECUMA_UINT8 likelyIfTestPasses;
	DECUMA_UINT8 functionNr;
	DECUMA_UINT8 functionArg2;
	DECUMA_UINT8 functionArg1;
} DECUMA_DB_STORAGE *CH_PAIR_ZOOM_INFO_PTR;

#define CH_PAIR_ZOOM_INFO_VALID_DUMMY_FIELDS(pX) (1)
#define SET_CH_PAIR_ZOOM_INFO_DUMMY_FIELDS(pX) {}



/*======================== */
typedef const struct _CH_PAIR_ZOOM_INFO_TABLE {
	DECUMA_UINT16 nZoomInfos;
	DECUMA_UINT8 dummy_1;
	DECUMA_UINT8 dummy_2;
	DECUMA_UINT32 zoomInfosOffset;
} DECUMA_DB_STORAGE *CH_PAIR_ZOOM_INFO_TABLE_PTR;

#define CH_PAIR_ZOOM_INFO_TABLE_VALID_DUMMY_FIELDS(pX) ((pX)->dummy_1==DUMMY_VALUE && (pX)->dummy_2==DUMMY_VALUE )
#define SET_CH_PAIR_ZOOM_INFO_TABLE_DUMMY_FIELDS(pX) {(pX)->dummy_1=DUMMY_VALUE; (pX)->dummy_2=DUMMY_VALUE; }



/*======================== */
typedef const struct _SVM_DB_HEADER {
	DECUMA_UINT8 nParametersStruct;
	DECUMA_UINT8 nNormalLists;
	DECUMA_UINT8 dummy_1;
	DECUMA_UINT8 dummy_2;
	DECUMA_UINT32 parametersStructOffset;
	DECUMA_UINT32 normalListOffset;
} DECUMA_DB_STORAGE *SVM_DB_HEADER_PTR;

#define SVM_DB_HEADER_VALID_DUMMY_FIELDS(pX) ((pX)->dummy_1==DUMMY_VALUE && (pX)->dummy_2==DUMMY_VALUE )
#define SET_SVM_DB_HEADER_DUMMY_FIELDS(pX) {(pX)->dummy_1=DUMMY_VALUE; (pX)->dummy_2=DUMMY_VALUE; }



/*======================== */
typedef const struct _SVM_PARAMETERS_STRUCT {
	DECUMA_UINT8 normalIndex;
	DECUMA_UINT8 dummy_1;
	DECUMA_UINT8 dummy_2;
	DECUMA_UINT8 dummy_3;
	DECUMA_INT32 bias;
	DECUMA_UINT32 normalScale;
	DECUMA_INT16 offsetForNormal;
	DECUMA_INT8 normalOffsetShift;
	DECUMA_INT8 biasShift;
} DECUMA_DB_STORAGE *SVM_PARAMETERS_STRUCT_PTR;

#define SVM_PARAMETERS_STRUCT_VALID_DUMMY_FIELDS(pX) ((pX)->dummy_1==DUMMY_VALUE && (pX)->dummy_2==DUMMY_VALUE && (pX)->dummy_3==DUMMY_VALUE )
#define SET_SVM_PARAMETERS_STRUCT_DUMMY_FIELDS(pX) {(pX)->dummy_1=DUMMY_VALUE; (pX)->dummy_2=DUMMY_VALUE; (pX)->dummy_3=DUMMY_VALUE; }



/*======================== */
typedef const struct _SVM_NORMAL_LIST {
	DECUMA_UINT8 nNormals;
	DECUMA_UINT8 dummy_1;
	DECUMA_UINT8 dummy_2;
	DECUMA_UINT8 dummy_3;
	DECUMA_UINT32 normalOffset;
} DECUMA_DB_STORAGE *SVM_NORMAL_LIST_PTR;

#define SVM_NORMAL_LIST_VALID_DUMMY_FIELDS(pX) ((pX)->dummy_1==DUMMY_VALUE && (pX)->dummy_2==DUMMY_VALUE && (pX)->dummy_3==DUMMY_VALUE )
#define SET_SVM_NORMAL_LIST_DUMMY_FIELDS(pX) {(pX)->dummy_1=DUMMY_VALUE; (pX)->dummy_2=DUMMY_VALUE; (pX)->dummy_3=DUMMY_VALUE; }



/*======================== */
typedef const struct _CURVELIST {
	DECUMA_UINT16 nCurves;
	DECUMA_UINT8 dummy_1;
	DECUMA_UINT8 dummy_2;
	DECUMA_UINT32 curvesOffset;
} DECUMA_DB_STORAGE *CURVELIST_PTR;

#define CURVELIST_VALID_DUMMY_FIELDS(pX) ((pX)->dummy_1==DUMMY_VALUE && (pX)->dummy_2==DUMMY_VALUE )
#define SET_CURVELIST_DUMMY_FIELDS(pX) {(pX)->dummy_1=DUMMY_VALUE; (pX)->dummy_2=DUMMY_VALUE; }



/*======================== */
typedef const struct _KEY_DB_HEADER {
	DECUMA_UINT8 databaseType;
	DECUMA_UINT8 nMaxArcsInCurve;
	DECUMA_UINT8 nKeyLists;
	DECUMA_UINT8 dummy_1;
	DECUMA_UINT32 keyListsOffset;
	DECUMA_UINT32 typeTableOffset;
	DECUMA_UINT16 nBaseSymbolTypes;
	DECUMA_UINT8 dummy_2;
	DECUMA_UINT8 dummy_3;
	DECUMA_UINT32 symbolInfoTableOffset;
	DECUMA_UINT16 nSymbols;
	DECUMA_UINT16 sizeOfSymbols;
	DECUMA_UINT32 curveListsOffset;
	DECUMA_UINT32 staticCategoryTableOffset;
	DECUMA_UINT16 nPointsInArc;
	DECUMA_UINT8 nPointsInInterleavedArc;
	DECUMA_UINT8 nMaxArcOrdersPerSymbol;
} DECUMA_DB_STORAGE *KEY_DB_HEADER_PTR;

#define KEY_DB_HEADER_VALID_DUMMY_FIELDS(pX) ((pX)->dummy_1==DUMMY_VALUE && (pX)->dummy_2==DUMMY_VALUE && (pX)->dummy_3==DUMMY_VALUE )
#define SET_KEY_DB_HEADER_DUMMY_FIELDS(pX) {(pX)->dummy_1=DUMMY_VALUE; (pX)->dummy_2=DUMMY_VALUE; (pX)->dummy_3=DUMMY_VALUE; }



/*======================== */
typedef const struct _SYMBOL_INFORMATION {
	DECUMA_UINT32 symbolOffset;
	DECUMA_INT8 diacriticMaskNr;
	DECUMA_INT8 combSymbIdx;
	DECUMA_UINT8 unknownProperty;
	DECUMA_UINT8 dummy_1;
} DECUMA_DB_STORAGE *SYMBOL_INFORMATION_PTR;

#define SYMBOL_INFORMATION_VALID_DUMMY_FIELDS(pX) ((pX)->dummy_1==DUMMY_VALUE )
#define SET_SYMBOL_INFORMATION_DUMMY_FIELDS(pX) {(pX)->dummy_1=DUMMY_VALUE; }



/*======================== */
typedef const struct _SYMBOL_TYPE_INFO {
	DECUMA_UINT16 symbolInfoIdx;
	DECUMA_INT8 typeNr;
	DECUMA_UINT8 lowerToUpper;
	DECUMA_UINT8 rotIdx;
	DECUMA_UINT8 symmetryIdx;
	DECUMA_UINT8 arcOrderSetIdx;
	DECUMA_UINT8 arcRotationMask;
	DECUMA_UINT8 arcTimelineDiffMask;
	DECUMA_UINT8 gestureFlag;
	DECUMA_UINT8 dummy_1;
	DECUMA_UINT8 dummy_2;
} DECUMA_DB_STORAGE *SYMBOL_TYPE_INFO_PTR;

#define SYMBOL_TYPE_INFO_VALID_DUMMY_FIELDS(pX) ((pX)->dummy_1==DUMMY_VALUE && (pX)->dummy_2==DUMMY_VALUE )
#define SET_SYMBOL_TYPE_INFO_DUMMY_FIELDS(pX) {(pX)->dummy_1=DUMMY_VALUE; (pX)->dummy_2=DUMMY_VALUE; }



/*======================== */
typedef const struct _KEY {
	DECUMA_UINT16 typeIdx;
	DECUMA_UINT16 curveIdx;
	DECUMA_INT16 sumX;
	DECUMA_INT16 sumY;
	DECUMA_UINT32 qSum;
	DECUMA_UINT32 dqSum;
	DECUMA_UINT16 xWidth;
	DECUMA_UINT8 dummy_1;
	DECUMA_UINT8 dummy_2;
	DECUMA_UINT32 interleavedQSum;
	DECUMA_UINT32 interleavedNorm;
	DECUMA_INT16 interleavedSumX;
	DECUMA_INT16 interleavedSumY;
} DECUMA_DB_STORAGE *KEY_PTR;

#define KEY_VALID_DUMMY_FIELDS(pX) ((pX)->dummy_1==DUMMY_VALUE && (pX)->dummy_2==DUMMY_VALUE )
#define SET_KEY_DUMMY_FIELDS(pX) {(pX)->dummy_1=DUMMY_VALUE; (pX)->dummy_2=DUMMY_VALUE; }



/*======================== */
typedef const struct _KEYLIST {
	DECUMA_UINT16 nKeys;
	DECUMA_UINT8 dummy_1;
	DECUMA_UINT8 dummy_2;
	DECUMA_UINT32 keysOffset;
} DECUMA_DB_STORAGE *KEYLIST_PTR;

#define KEYLIST_VALID_DUMMY_FIELDS(pX) ((pX)->dummy_1==DUMMY_VALUE && (pX)->dummy_2==DUMMY_VALUE )
#define SET_KEYLIST_DUMMY_FIELDS(pX) {(pX)->dummy_1=DUMMY_VALUE; (pX)->dummy_2=DUMMY_VALUE; }



/*======================== */
typedef const struct _CAT_TABLE {
	DECUMA_UINT32 status;
	DECUMA_UINT32 baseSymbolsOffset;
	DECUMA_UINT32 diacSymbolsOffset;
	DECUMA_UINT32 categoryIds[64];
	DECUMA_UINT32 languageIds[64];
} DECUMA_DB_STORAGE *CAT_TABLE_PTR;

#define CAT_TABLE_VALID_DUMMY_FIELDS(pX) (1)
#define SET_CAT_TABLE_DUMMY_FIELDS(pX) {}



/*======================== */
typedef const struct _DB_CATEGORIES {
	DECUMA_UINT32 category[2];
	DECUMA_UINT32 language[2];
	DECUMA_UINT32 altCategory[2];
	DECUMA_UINT32 altLanguage[2];
} DECUMA_DB_STORAGE *DB_CATEGORIES_PTR;

#define DB_CATEGORIES_VALID_DUMMY_FIELDS(pX) (1)
#define SET_DB_CATEGORIES_DUMMY_FIELDS(pX) {}



/*======================== */
#endif
