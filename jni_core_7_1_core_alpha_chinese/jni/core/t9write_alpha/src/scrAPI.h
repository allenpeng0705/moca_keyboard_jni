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

      File: scrAPI.h
   Package: This file is part of the package scr
 $Revision: 1.5 $
     $Date: 2011/02/14 11:41:14 $
   $Author: jianchun_meng $

\************************************************* Header files ***/

#ifndef scrAPI_h
#define scrAPI_h

#ifdef DECUMA_MANGLE
#include "mangle.h"
#endif

#include "decumaCategoryTableType.h"
#include "decumaCharacterSetType.h"
#include "decumaDataTypes.h" /* Definition of INT16 INT16_POINT DECUMA_BOOL etc */
#include "decumaSimTransf.h"
#include "decumaUnicodeTypes.h"
#include "databaseStatic.h"
#include "udmType.h"
#include "decumaStatus.h"
#include "scrVersion.h"

#include "decuma_hwr_types.h"

#include "scrAPIHidden.h"  /* Don't use the datatypes defined here!!! */

/******************************************************************/

typedef const void DECUMA_DB_STORAGE * SCR_API_ANY_DB_PTR; /* Static or dynamic DB */

typedef struct {
	INT16_POINT point[NUMBER_OF_POINTS_IN_ARC];
} ARC;

typedef struct {
	ARC * pArc[MAX_NUMBER_OF_ARCS_IN_CURVE];
	DECUMA_UINT8 arcTimelineDiff[MAX_NUMBER_OF_ARCS_IN_CURVE-1];
	DECUMA_UINT8 nArcs;
} CURVE;

typedef struct {
	SCR_OUTPUT * pOut;
	int nOutputs;
} SCR_OUTPUT_LIST;



/*
  nCategory
		A category mask that sets what categories of keys
		that are allowed to be returned from an interpretation.

  nRefAngle
		What angle is considered to be "up" when punishing for
		rotation, default is zero. The unit for the angle is
		radians times 100, i.e. one lap is approximately 628.

		It is possible to improve the interpretation by supplying an angle
		that tells the algorithm what is considered to be "up". The angle
		makes it possible to distinguish between 'c' and 'U' for example.

		The normal angle is zero, which implies that up is in the
		direction of negative Y axis.

	                (157)
	                  Y-
	                  ^
	                  |
	                  |
	    (314) X- <----+----> X+ (0)
	                  |
	                  |
	                  v
	                  Y+
	                (471)

  nRefAngleImportance
		How important is rotation. FULL_ROTATION_IMPORTANCE causes full
		punishment for rotation, 0 means that rotation
		will not be punished

  nBaseLineY
		The Y-coordinate for the baseline.
		Note that if a baseline exists there must be a helpline. Both
		nBaseLineY and nHelpLineY shall be zero if the lines do not exist.

  nHelpLineY
		The Y-coordinate for the helpline (see comment above).

  pStaticDB
		A pointer to the static database. NOTE: This must point to a
		valid static database.

  pUDM
		A pointer to a database User Database Modifier
		user allographs. May be NULL.

  pCategoryTable
		A pointer to a user defined category table. May be NULL.

*/

typedef struct {
	DECUMA_CHARACTER_SET characterSet; /*The set of which characters should be selected */
	int nRefAngle;
	int nRefAngleImportance;
	DECUMA_INT16 nBaseLineY;
	DECUMA_INT16 nHelpLineY;
	STATIC_DB_PTR pStaticDB;
	UDM_PTR pUDM; /* The key database part of the udm */
	CATEGORY_TABLE_PTR pCategoryTable;

/* Parameters for cooperative multitasking */
/*
	void * pCoopFcn;		Pointer to cooperative multitasking function.
	void * pCoopData;		Pointer to cooperative multitasking data. Passed as
					 arguement to the function that pFcn points to.
	short coopInterval;
*/
} SCR_API_SETTINGS;

/* NOTE: The functions below do not handle NULL pointer arguments */
/* unless this explicitly specified.                              */



/* Returns the number of bytes needed for the memory buffer
   used in the scrSelect call. */
DECUMA_HWR_PRIVATE int scrGetMemBufSize(void);


/** scrSelect is the main entry point for calls to the scr algorithm.

parameters:
	pOutputs
		this points to an array of scrOUTPUT elements with the length
		of nOutputs. If zooming shall work it is crucial that it is
		at least two elements long. Upon exit this vector contains
		the two best guesses of the algorithm, with the most likely
		match at index 0.

	nMaxOutputs
		The length of the vector pointed to by pOutputs.

	pSettings
		See above.
	pCurve
		The input curve. It must have at least one arc. The arcs
		in the curve is assumed to have NUMBER_OF_POINTS_IN_ARC
		points sampled with a fixed arc length. The function
		resampArc() perform this resampling of an arc.

	pPrecalculatedBaseOutputs
		If this pointer is other than zero, it should point at a vector
		of length MAX_ARCS_IN_DIACRITIC. The vector should contain
		structs (scrOUTPUT_LIST) that tells how many baseOutputs are
		supplied, and a pointers to the first baseOutput. The baseOutputs
		contain the result of prior comparisons with the first n arcs of
		the input curve. Here n= (number of arcs in input curve) -
		(index+1 of the struct in the vector)
		E.g. if we call scrSelect with a 4-arc-long curve,
		pPrecalculatedBaseOutputs[1].pOut contains a vector of
		precalculated outputs from a comparison of the 4-(1+1) = 2
		first arcs of the curve against the database.
		pPrecalculatedBaseOutputs[1].nOutputs contains the number of
		precalculated outputs (for 2 first arcs)
	
	pMemoryBuffer
		An allocated buffer of memory that can be used by the SCR engine.
		The memory should preferably be stored on heap and be at least
		the size returned by scrGetMemBufSize().

Returns:
	The number of written SCR_OUTPUTs
*/
DECUMA_HWR_PRIVATE DECUMA_STATUS scrSelect(SCR_OUTPUT * pOutputs, int * pnOutputs, int nMaxOutputs,
			  const SCR_API_SETTINGS * pSettings, const CURVE * pCurve,
			  const SCR_OUTPUT_LIST * pPrecalculatedBaseOutputs,
			  void * pMemoryBuffer,
			  const DECUMA_INTERRUPT_FUNCTIONS * pInterruptFunctions);


DECUMA_HWR_PRIVATE DECUMA_STATUS scrCheckSettings(const SCR_API_SETTINGS * pSettings);

/* Returns a pointer to a null terminated string with the symbol. */
DECUMA_HWR_PRIVATE DECUMA_UNICODE_DB_PTR scrOutputGetSymbol(const SCR_OUTPUT * pOut);

/* Returns TRUE if the contents of the SCR_OUTPUT does not come
  from a proximity measurement but rather set by the SCR-engine
  since it considered the curve to be too small for proximity
  measurement */
DECUMA_HWR_PRIVATE DECUMA_BOOL scrOutputIsInterpretationOfSmallCurve(const SCR_OUTPUT * pOut,
										   const SCR_API_SETTINGS * pSettings);

/* Returns the symmetry of the symbol in the database */
DECUMA_HWR_PRIVATE DECUMA_INT16 scrOutputGetSymmetry(const SCR_OUTPUT * pOut);

/* Returns the width of the symbol in the database */
DECUMA_HWR_PRIVATE DECUMA_UINT16 scrOutputGetWidth(const SCR_OUTPUT * pOut);

/* Returns the proximity of the SCR_OUTPUT */
DECUMA_HWR_PRIVATE DECUMA_INT16 scrOutputGetProximity(const SCR_OUTPUT * pOut);

/* Returns TRUE if the SCR_OUTPUT is the original symbol */
DECUMA_HWR_PRIVATE DECUMA_BOOL scrOutputIsOriginal(const SCR_OUTPUT * pOut);

/* Returns TRUE if the SCR_OUTPUT is the alternative symbol
   e.g. a "c" that shall be converted to a "C" */
DECUMA_HWR_PRIVATE DECUMA_BOOL scrOutputIsAlternative(const SCR_OUTPUT * pOut);

/* Returns TRUE if the SCR_OUTPUT is in category nCategory */
DECUMA_HWR_PRIVATE DECUMA_BOOL scrOutputIsInCategory(const SCR_OUTPUT * pOut,
			   const CATEGORY nCategory);

/* Returns TRUE if the SCR_OUTPUT can be both the original
   symbol and the alternative symbol.
   That is it is up to the caller to decide if it shall
   be upper or lower case. */
DECUMA_HWR_PRIVATE DECUMA_BOOL scrOutputCanBeBoth(const SCR_OUTPUT * pOut);

/* Returns how much the symbol should be translated to be recognized as the  "alternative" symbol.
   Returns zero if it is not translated at all to its alternative.
   (example: for p -> P, altSymbolTranslation = -1) */
DECUMA_HWR_PRIVATE void scrOutputAlternativeTranslation(const SCR_OUTPUT * pOut, DECUMA_INT8* altSymbolTranslation);

/* Returns how much the symbol should be scaled to be recognized as the  "alternative" symbol.
   Returns zero if it is not scaled at all to its alternative.
   (example: for s -> S, altSymbolScaleNum = 2, altSymbolScaleDenom = 1) */
DECUMA_HWR_PRIVATE void scrOutputAlternativeScale(const SCR_OUTPUT * pOut, DECUMA_INT8* altSymbolScaleNum, DECUMA_INT8* altSymbolScaleDenom);

/* Returns a pointer to the SIM_TRANSF */
DECUMA_HWR_PRIVATE const SIM_TRANSF * scrOutputGetSimTransf(const SCR_OUTPUT * pOut);

/* Returns an integer with value about zoom function */
DECUMA_HWR_PRIVATE DECUMA_INT8 scrOutputGetZoomValue(const SCR_OUTPUT * pOut);

/* Returns a pointer to a vector of arc orders
   The vector is of the same length as the number of arcs
   that were supplied to scrSelect() */
DECUMA_HWR_PRIVATE const DECUMA_INT8 * scrOutputGetArcOrder(const SCR_OUTPUT * pOut);

/* Sets the min and max y-values of the key in the adresses
   pYMin and pYMax respectively */
DECUMA_HWR_PRIVATE void scrOutputGetKeyMinMaxY(const SCR_OUTPUT * pOut,DECUMA_INT8 * pMinY, DECUMA_INT8 * pMaxY);

/*
Returns the allograph type id for the scr output.
Example of "a" with different type ID's:

            ** *      ***
           *   *         *
           *   *      ****
           *   *     *   *
            * * *     *** *
*/
DECUMA_HWR_PRIVATE int scrOutputGetAllographType( const SCR_OUTPUT * pOut );


typedef enum {
	certain = 0,
	littleUncertain,
	quiteUncertain,
	veryUncertain
} DEGREE_OF_UNCERTAINTY;

/*
Returns a measure of how certain the size of the key is. Could this key be written
in many different sizes by different users ? Keys that are typically uncertain are
e.g. '@' '.' '$' etc. While the size of e.g. 'a' 'b' and 'c' are more exact defined.
*/
DECUMA_HWR_PRIVATE DEGREE_OF_UNCERTAINTY scrOutputGetSizeUncertainty(const SCR_OUTPUT * pOut);

/*
The same as 'scrOutputGetSizeUncertainty' except that this concerns 'position'
instead of 'size'.
*/
DECUMA_HWR_PRIVATE DEGREE_OF_UNCERTAINTY scrOutputGetPositionUncertainty(const SCR_OUTPUT * pOut);

/*
Return the database distance from the baseline to the helpline
*/
DECUMA_HWR_PRIVATE DECUMA_INT32 scrOutputGetDbDistBase2Help(const SCR_OUTPUT * pOut);

/*
This function creates a faked SCR_OUTPUT with the interpretation
pSymbol. The proximity and punish variables are given high values.
NOTE: The DBindex is set to the first KEY in the static database.
*/
DECUMA_HWR_PRIVATE void scrOutputCreateFaked(const SCR_API_SETTINGS * pSettings, SCR_OUTPUT * pOut,
	const DECUMA_UNICODE * pSymbol );

/*
This function creates a faked SCR_OUTPUT with the properties of
the symbol that is set in the database as the default small symbol
for mcr. 
*/
DECUMA_HWR_PRIVATE void scrOutputCreateMcrSmall(const SCR_API_SETTINGS * pSettings,
	SCR_OUTPUT * pOut, CATEGORY * pCategory);

/*
This function returns several values that are database specific and controls
the distance between characters.
*/
DECUMA_HWR_PRIVATE void scrGetCharacterDistances(const SCR_API_SETTINGS * pSettings,
							  DECUMA_INT16 * pDefaultCharDist, DECUMA_INT16 * pDefaultGestureDist,
							  DECUMA_UINT8 * pnDistInTable, CH_PAIR_DIST_CAT_PTR* ppCharDistTable);

/*
This function returns a character set translated into local static db format.
If a personal category table is provided it will be used instead of the category 
table in the static database.
*/
DECUMA_HWR_PRIVATE CATEGORY scrTranslateToStaticDBCategory(STATIC_DB_PTR pStaticDB,
	CATEGORY_TABLE_PTR pCategoryTable,
	const DECUMA_CHARACTER_SET * pCharacterSet);

/*
This function returns 1 if the symbol category provided is included in the
character set provided (note that languages are unimportant in this case)
*/
DECUMA_HWR_PRIVATE int scrCharacterSetIncludesAtomicSymbolCategory(
	const DECUMA_CHARACTER_SET * pCharacterSet,
	DECUMA_UINT32 symbolCategory);

/*
This function returns 1 if the output represents a multitouch template
*/
DECUMA_HWR_PRIVATE int scrOutputIsMultitouch(const SCR_OUTPUT* pOutput);

DECUMA_HWR_PRIVATE DECUMA_UINT8 scrOutputGetArcTimelineDiffMask(const SCR_OUTPUT* pOutput);

/**
FUNCTION: scrGetVersion
Parameters:
  (none)

Return value:
A pointer to a null terminated string with the version of SCR.
**/
DECUMA_HWR_PRIVATE const char * scrAPIGetVersion(void);

DECUMA_HWR_PRIVATE int _scrDatabaseSupportsSymbolCategory(const SCR_API_ANY_DB_PTR pDB, DECUMA_UINT32 cat);

DECUMA_HWR_PRIVATE int _scrDatabaseSupportsLanguage(const SCR_API_ANY_DB_PTR pDB, DECUMA_UINT32 lang);

DECUMA_HWR_PRIVATE DECUMA_STATUS _scrDatabaseIncludesSymbol(const SCR_API_ANY_DB_PTR pDB,
										 const DECUMA_CHARACTER_SET * pCharSet,
										 const DECUMA_UNICODE * pSymbol,
										 int * pbIncluded);

DECUMA_HWR_PRIVATE DECUMA_STATUS _scrCheckStaticDB(STATIC_DB_PTR pDB);

DECUMA_HWR_PRIVATE void _scrGetCurve(SCR_CURVE* pScrCurve, void * pMemoryBuffer);

DECUMA_HWR_PRIVATE void _scrGetCurveProp(const SCR_OUTPUT* pOutput, SCR_CURVE_PROP* pScrCurveProp, void * pMemoryBuffer);

DECUMA_HWR_PRIVATE DECUMA_STATUS _scrIsOutputEstimateReliable(const SCR_OUTPUT* pOutput,
										   int *pnIsReliable);

DECUMA_HWR_PRIVATE DECUMA_STATUS _scrEstimateScalingAndVerticalOffset(const SCR_OUTPUT* pOutput,
		DECUMA_INT16* pnBaseLineYEstimate,
		DECUMA_INT16* pnHelpLineYEstimate);

DECUMA_HWR_PRIVATE DECUMA_STATUS _scrGetScalingAndVerticalOffsetPunish(const SCR_OUTPUT* pOutput,
													const SCR_CURVE_PROP* pScrCurveProp,
													DECUMA_INT16 nBaseLineY,
													DECUMA_INT16 nHelpLineY,
													DECUMA_INT16 *pnPunish);

DECUMA_HWR_PRIVATE DECUMA_STATUS _scrGetRotationPunish(const SCR_OUTPUT* pOutput,
									const SCR_CURVE_PROP* pScrCurveProp,
									DECUMA_INT16 nBaseLineY,
									DECUMA_INT16 nHelpLineY,
									int nRefAngle,
									DECUMA_INT16 *pnPunish);

DECUMA_HWR_PRIVATE DECUMA_STATUS _scrAdjustMuAndPunish(SCR_OUTPUT* pOutput,
								    const SCR_CURVE_PROP* pScrCurveProp,
								    DECUMA_INT16 nNewBaseLineY,
								    DECUMA_INT16 nNewHelpLineY,
								    DECUMA_INT16 nOldBaseLineY,
								    DECUMA_INT16 nOldHelpLineY);

DECUMA_HWR_PRIVATE DECUMA_STATUS _scrOutputIsGesture(const SCR_OUTPUT* pOutput,
								  int * pbGesture, 
								  int * pbInstantGesture);

DECUMA_HWR_PRIVATE int _scrOutputInCategory(const SCR_OUTPUT* pOutput,
						 const CATEGORY Category);

#endif /* scrAPI_h */

/*************************************************** Local code ***/

