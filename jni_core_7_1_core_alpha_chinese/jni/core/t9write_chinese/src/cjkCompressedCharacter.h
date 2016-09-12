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

#pragma once

#ifndef CJK_COMPRESSED_CHARACTER_H_
#define CJK_COMPRESSED_CHARACTER_H_

#include "dltConfig.h"
#include "decumaBasicTypes.h"

typedef struct _tagCJK_CC_DTW_SCRATCHPAD CJK_CC_DTW_SCRATCHPAD;
typedef struct _tagCJK_CC_COMPRESS_SCRATCHPAD CJK_CC_COMPRESS_SCRATCHPAD;

/**
 * The graphical representation of a written Chinese character.
 * Its value is provided either by the database or by @ref dltCCharCompress.
 *
 * A character is always a member of a singly linked list, i.e. you
 * can always ask it for the next character using the method @ref dltCCharGetNext.
 * The natural way to scan the database is to get the first character with k 
 * points using the method @ref cjkDbGetFirstChar (k,m) and then iterate over
 * all characters in the list of characters with k points.
 *
 * Note that a character may not occupy more than 255 bytes since the
 * byte count should be available in the first byte.
 */
typedef struct _tagCJK_COMPRESSED_CHAR CJK_COMPRESSED_CHAR;

/** 
 * The first element in an array of CJK_COMPRESSED_CHAR_DATA in a CJK_COMPRESSED_CHAR contains the number of 
 * elements in the array, followed by the elements themselves, which are Gridpoints 
 */
typedef DECUMA_UINT8 CJK_COMPRESSED_CHAR_DATA;


#include "cjkCommon.h"
#include "cjkStroke.h"
#include "cjkDatabaseLimits.h"
#include "cjkDatabaseFormat.h"
#include "cjkArcSession.h"
#include "cjkTypes.h"

#include "decumaCurve.h"

/* --------------------------------------------------------------------------
 * Types
 * -------------------------------------------------------------------------- */

/**
 * The DLTDB_ATTRIBUTE is an unsigned short, and right now 6 bits are used. The first four
 * is used to save the frequency of the character. No 5 is and old remain, should
 * be removed. No 6 is used to remember if the character was sampled dense or
 * sparse.
 */
typedef DECUMA_UINT16 DLTDB_ATTRIBUTE;


/* TODO: redefine as something like */
/* typedef const struct _tagCJK_COMPRESSED_CHAR_DATA { */
/*      DECUMA_UINT8 nGridPoints; */
/*      Gridpoint    pGridpoints[1]; */
/* } CJK_COMPRESSED_CHAR_DATA; */


/** Used in @ref dltCCharCompress to indicate compression type to use */
typedef enum _tagCJK_COMPRESSION_STYLE {
	CC_FREE   = 0x0,
	CC_SPARSE = 0x1,
	CC_DENSE  = 0x2,
	CC_SPARSE_KO = 0x4
} CJK_COMPRESSION_STYLE;


/** Used to iterate over the Gridpoints in a CJK_STROKE */
typedef struct _tagCJK_GRIDPOINT_ITERATOR {
   CJK_STROKE            stroke; /**< CJK_STROKE to iterate over */
   const CJK_GRIDPOINT * p;      /**< pointer to the present gridpoint */
   DECUMA_INT32          n;      /**< remaining points in the stroke, including the present one. */
} CJK_GRIDPOINT_ITERATOR;

/* --------------------------------------------------------------------------
 * Global data
 * -------------------------------------------------------------------------- */

DECUMA_HWR_PRIVATE_DATA_H const DECUMA_UINT8 pGridPointDistTable[512];



/* --------------------------------------------------------------------------
 * Macros
 * -------------------------------------------------------------------------- */

/** @defgroup DOX_CJK_CCHAR cjkCompressedCharacter
 *  @{
 */

/** Bandwidth for the diagonal band in the matrix of the dynamic time warping algorithm 
    @hideinitializer
 */
#define BW 9

/** Use double bandwidth when building database to allow for comparisons of templates
    which could be mutually comparable at sample level
    @hideinitializer
 */

#define BW_DOUBLE 17

/** A database character can contain max this many versions
    @hideinitializer
 */
#define MAXCMVERSIONS 8

/** Maximum number of Gridpoints in a CJK_COMPRESSED_CHAR
    @hideinitializer
 */
#define CC_MAXCCHARSIZE 255

/** Maximum number of working Gridpoints (buffer size) before final
 *  write to CChar
 *  @hideinitializer
 */
#define CC_MAX_PT_BUFFER_SZ (2*CC_MAXCCHARSIZE)

/* TODO rename to DENSE_SAMPLING_LIMIT ?  */
/** Max number of strokes for dense sampling.
    @hideinitializer
    @see dltCCharCompress 
 */
#define LATINLIMIT 2


#define CJK_GP_MAX 0x0F

/** @hideinitializer */
#define CJK_ATTRIB_IS_DENSE(m_attrib)         ((m_attrib) & AM_ISDENSE)
/** @hideinitializer */
#define CJK_ATTRIB_FREQUENCY(m_attrib)        ((m_attrib) & AM_FREQ)

/** @hideinitializer
 * Move to next gridpoint. Stops at the end of a character. Uses inline
 * version of cjkStrokeNext.
 */
#define CJK_GPITER_NEXT_INLINE(m_pGridpointIterator, m_pSession) \
if (--(m_pGridpointIterator)->n == 0) { \
	cjkStrokeNext(&(m_pGridpointIterator)->stroke, m_pSession); \
	if (CJK_STROKE_EXISTS(&(m_pGridpointIterator)->stroke)) { \
		(m_pGridpointIterator)->n = CJK_STROKE_NPOINTS(&(m_pGridpointIterator)->stroke); \
		(m_pGridpointIterator)->p = CJK_STROKE_FIRST_POINT(&(m_pGridpointIterator)->stroke);} \
} else (m_pGridpointIterator)->p++


/** @hideinitializer
 * Move to next gridpoint. Stops at the end of a character.
 */
#define CJK_GPITER_NEXT(m_pGridpointIterator, m_pSession) \
if (--(m_pGridpointIterator)->n == 0) { \
	cjkStrokeNext(&(m_pGridpointIterator)->stroke, m_pSession); \
	if (CJK_STROKE_EXISTS(&(m_pGridpointIterator)->stroke)) { \
		(m_pGridpointIterator)->n = CJK_STROKE_NPOINTS(&(m_pGridpointIterator)->stroke); \
		(m_pGridpointIterator)->p = CJK_STROKE_FIRST_POINT(&(m_pGridpointIterator)->stroke);} \
} else (m_pGridpointIterator)->p++


/** @hideinitializer
 *  Get current gridpoint
 */
#define CJK_GPITER_GET_GRIDPOINT(m_pGridpointIterator) (*(m_pGridpointIterator)->p)


/** @hideinitializer
 * @returns True if the iterator has a valid point.
 */
#define CJK_GPITER_HAS_GRIDPOINT(m_pGridpointIterator) CJK_STROKE_EXISTS(&((m_pGridpointIterator)->stroke))

/** @hideinitializer
 * @returns True if the current gridpoint is the last one.
 * TODO Why not a pointer to a CJK_GRIDPOINT_ITERATOR here?
 */
#define CJK_GPITER_IS_LAST(m_gridpointIterator)  ((m_gridpointIterator).n == 1)

/** Sets a m_gridpoint from a x, and y value @hideinitializer */
#define CJK_GP(x, y) (((x) & 0xF) | (((y) & 0xF) << 4))

/** @returns The x-coordinate of m_gridpoint @hideinitializer */
#define CJK_GP_GET_X(m_gridpoint) ((m_gridpoint)  & 0x0F)

/** @returns The x-coordinate of m_gridpoint @hideinitializer */
#define CJK_GP_GET_Y(m_gridpoint) (((m_gridpoint) & 0xF0) >> 4)

/*
 * If points are at a distance of at most 7 units in x-direction then a simple
 * subtraction in combination with a table lookup is needed to compute the
 * squared distance.
 *
 * The table is 512 bytes long so that 9 bits are used in the lookup, which
 * means that the carry bit from the y-direction is used.
 * @hideinitializer
 */
#define CJK_GP_GET_SQ_DISTANCE(m_gridpointA, m_gridpointB) \
	pGridPointDistTable[((m_gridpointA)-(m_gridpointB)) & 0x1FF]

#define CJK_GP_GET_SQ_DISTANCE_TRUE(m_gridpointA, m_gridpointB) \
	((CJK_GP_GET_X(m_gridpointA) - CJK_GP_GET_X(m_gridpointB)) * (CJK_GP_GET_X(m_gridpointA) - CJK_GP_GET_X(m_gridpointB)) + \
     (CJK_GP_GET_Y(m_gridpointA) - CJK_GP_GET_Y(m_gridpointB)) * (CJK_GP_GET_Y(m_gridpointA) - CJK_GP_GET_Y(m_gridpointB)))





/* --------------------------------------------------------------------------
 * Exported functions - CJK_COMPRESSED_CHAR methods
 * -------------------------------------------------------------------------- */

DECUMA_HWR_PRIVATE DECUMA_UINT32 cjkCompressedCharacterGetSize(void);

DECUMA_HWR_PRIVATE DECUMA_UINT32 cjkCompressedCharacterGetScratchpadSize(void);

DECUMA_HWR_PRIVATE DECUMA_UINT32 cjkCompressedCharacterDTWDistGetScratchpadSize(void);

DECUMA_HWR_PRIVATE void cjkCompressedCharacterSetDTWCached(CJK_COMPRESSED_CHAR * pCChar, int bIsCached);

DECUMA_HWR_PRIVATE void dltCCCompressSetIndex(CJK_COMPRESSED_CHAR * pCChar, DLTDB_INDEX index);

DECUMA_HWR_PRIVATE void dltCCCompressSetNbrStrokes(CJK_COMPRESSED_CHAR * pCChar, int nStrokes);

DECUMA_HWR_PRIVATE void dltCCCompressSetNbrPoints(CJK_COMPRESSED_CHAR * pCChar, int nPoints);

DECUMA_HWR_PRIVATE void dltCCCompressGetMaxMin(CJK_COMPRESSED_CHAR * pCChar, int * xmin, int * xmax, int * ymin, int * ymax);

/* Some functions defined as macros in release mode */

#include "cjkCompressedCharacter_Macros.h"

#if defined(_DEBUG) 

DLTDB_INDEX dltCCCompressGetIndex(CJK_COMPRESSED_CHAR * pCChar);
DECUMA_INT32 dltCCCompressGetNbrStrokes(CJK_COMPRESSED_CHAR * pCChar);
DECUMA_INT32 dltCCCompressGetNbrPoints(CJK_COMPRESSED_CHAR * pCChar);
int dltCCCompressGetYmin(CJK_COMPRESSED_CHAR * pCChar);
int dltCCCompressGetYmax(CJK_COMPRESSED_CHAR * pCChar);
int dltCCCompressGetXmin(CJK_COMPRESSED_CHAR * pCChar);
int dltCCCompressGetXmax(CJK_COMPRESSED_CHAR * pCChar);
int dltCCCompressIsDense(CJK_COMPRESSED_CHAR * pCChar);
DECUMA_FEATURE dltCCCompressGetFeatureVal(CJK_COMPRESSED_CHAR * pCChar, int nFeatureIdx);
void dltCCCompressSetNull(CJK_COMPRESSED_CHAR * pCChar);

#endif


DECUMA_HWR_PRIVATE DECUMA_ARC * dltCCCompressGetOrgStrokes(CJK_COMPRESSED_CHAR * pCChar);

DECUMA_HWR_PRIVATE void dltCCCompressSetAttribute(CJK_COMPRESSED_CHAR * pCChar, DLTDB_ATTRIBUTE attrib);

DECUMA_HWR_PRIVATE DLTDB_ATTRIBUTE dltCCCompressGetAttribute(CJK_COMPRESSED_CHAR * pCChar);

#ifdef CJK_ENABLE_INTERNAL_API
DECUMA_HWR_PRIVATE CJK_COMPRESSED_CHAR_DATA * dltCCCompressGetCCData(CJK_COMPRESSED_CHAR * pCChar);
#else
DECUMA_HWR_PRIVATE const CJK_COMPRESSED_CHAR_DATA * dltCCCompressGetCCData(CJK_COMPRESSED_CHAR * pCChar);
#endif

DECUMA_HWR_PRIVATE void dltCCCompressSetCCData(CJK_COMPRESSED_CHAR * pCChar, const CJK_COMPRESSED_CHAR_DATA * pCCData);

DECUMA_HWR_PRIVATE void dltCCompressSetDotStart(CJK_COMPRESSED_CHAR * pCChar, int dotStart);

DECUMA_HWR_PRIVATE void dltCCompressSetNotSpeaking(CJK_COMPRESSED_CHAR * pCChar, int bNotSpeaking);

DECUMA_HWR_PRIVATE int dltCCompressGetNotThreeDrops(CJK_COMPRESSED_CHAR * pCChar);

DECUMA_HWR_PRIVATE void dltCCompressSetNotThreeDrops(CJK_COMPRESSED_CHAR * pCChar, int bNotThreeDrops);

DECUMA_HWR_PRIVATE void dltCCompressSetNotSoil(CJK_COMPRESSED_CHAR * pCChar, int bNotSoil);

#ifdef CJK_ENABLE_INTERNAL_API
DECUMA_HWR_PRIVATE int cjkCompressedCharacterCanBeSparse(CJK_SAMPLING_RULE samplingRule, int nPoints);
#endif

/**
 * Takes the data on the form of a DECUMA_ARC and squeezes it into the internal
 * format and fills in the @ref CJK_COMPRESSED_CHAR object allocated by the 
 * caller. This is a kind of constructor method.
 *
 * @param pCChar
 * @param pArcs 
 * @param nArcs
 * @param style      Compression style. Value from the enumeration 
 *                   CJK_COMPRESSION_STYLE:
 *                   @li CC_DENSE  dense sampling (same scaling in x & y)
 *                   @li CC_SPARSE sparse samlping (separate scaling in x & y)
 *                   @li CC_FREE   leave the decision to @ref dltCCharCompress
 *                   Later on, @ref cjkDbLookup has to know which way the
 *                   character was sampled. This information is stored in a 
 *                   bit in the attrib field of pCChar.
 * @param pSession
 * 
 * The method uses an internal static return buffer that will be overwritten
 * on the next call and destroy the character from the previous call.
 * 
 * In case something goes wrong one can use @ref CJK_CCHAR_EXISTS to check if there
 * was really anything returned.
 */
DECUMA_HWR_PRIVATE DECUMA_STATUS dltCCharCompressChar(      CJK_COMPRESSED_CHAR * pCChar,
								   const DECUMA_CURVE        * pCurve, 
								         CJK_ARC_SESSION         * pCjkArcSession);



/**
 */
DECUMA_HWR_PRIVATE void dltCCharPrecompute(CJK_COMPRESSED_CHAR * pCChar, CJK_SESSION * pSession);

/** 
 * @returns a pointer to the first stroke. 
 */
DECUMA_HWR_PRIVATE void dltCCharGetFirstStroke(CJK_COMPRESSED_CHAR * pCChar, CJK_STROKE * pStroke, CJK_SESSION * pSession);


/** 
 * @returns a pointer to the last stroke. 
 */
DECUMA_HWR_PRIVATE void dltCCharGetLastStroke(CJK_COMPRESSED_CHAR * pCChar, CJK_STROKE * pStroke, CJK_SESSION * pSession);


/**
 * Matches strokes one to one.
 * This method computes the raw character distance which is the sum of the
 * (squared) distances between all the strokes in the two characters.
 */ 
DECUMA_HWR_PRIVATE CJK_DISTANCE dltCCharGetRawDistance(CJK_COMPRESSED_CHAR * pCCharA,
									CJK_COMPRESSED_CHAR * pCCharB,
									DECUMA_INT32          delta,
									CJK_SESSION         * pSession);


/** 
 * Uses a sophisticated dynamic programming algorithm to match characters with
 * different number of strokes, thus trying to match semicursive characters.
 *
 * @param pCCharUser The character the user wrote, can NOT contain components 
 *                   and component variants.
 * @param pCCharDb    Character from the database, can contain components and 
 *                   component variants.
 * @param delta      The difference between the two mass centres, which is the 
 *                   same as for all calls to character distance functions and 
 *                   therefore needs not be recalculated in every call.
 * @param bUseDiff   Use difference preprocessing before the distance 
 *                   calculation.
 * @param bUseGap    Use punishment when the user has lifted the pen but the 
 *                   database has not, which is less likely than the other way 
 *                   around.
 * @param bUseComp   Pick out the minimum value of some cells in totdist at the
 *                   end so that not all points of the user character @ref 
 *                   pCCharUser are guaranteed to have been consumed. This is used
 *                   for matching initial components for the featurebits 
 *                   calculations.
 *
 * @param bCalculateAlignment 
 *                   If true, the alignment between the two point vectors are 
 *                   calculated in addition to the distance. The vector is stored
 *                   in pSession->pDtwAlignment, and the length of the vector is
 *                   CJK_CCHAR_NPOINTS(pCCharDb). The index of each element in the 
 *                   vector corresponds to the index of a point in pCCharDb, and 
 *                   the value corresponds to an index of a point in pCCharUser.
 *
 * @param bDoubleBandwidth
 *                   If true, The bandwidth used by the algorithm is doubled. Used 
 *                   in database clustering only.
 *
 * @param pSession
 *                   
 *
 * The function assumes both pCCharUser and pCCharDb are sampled dense or that
 * both are sampled sparse.
 */
DECUMA_HWR_PRIVATE
CJK_DISTANCE dltCCharGetDTWDistance(CJK_COMPRESSED_CHAR   * const pCCharUser,
									CJK_COMPRESSED_CHAR   const * const pCCharDb, 
									DECUMA_INT32            const delta, 
									CJK_DISTANCE            const distanceLimit,
									CJK_BOOLEAN             const bUseDiff, 
									CJK_BOOLEAN             const bUseGap, 
									CJK_BOOLEAN             const bUseComp,
									CJK_BOOLEAN             const bDoubleBandwidth,
									CJK_SESSION           * const pSession);


/** 
 * Stroke-order free distance method
 * 
 * This method is designed to handle input of chinese characters that haven't
 * been drawn in the correct stroke order.
 * The function dltCCharGetSOFDistance takes two Chinese characters with equal
 * number of strokes as input and returns the distance between them.
 * It uses a greedy algorithm where each stroke in pCCharA is matched against
 * the closest stroke in pCCharB which has not yet been matched, and vice versa.
 */ 
DECUMA_HWR_PRIVATE CJK_DISTANCE dltCCharGetSOFDistance(CJK_COMPRESSED_CHAR * pCCharA,
									CJK_COMPRESSED_CHAR * pCCharB, 
									DECUMA_INT32          delta, 
									CJK_DISTANCE          distanceLimit, 
									CJK_SESSION         * pSession);
/** 
 * Coarse DTW distance
 * 
 * This method is designed for use in clustering and coarse search for prototypes.
 * It calls the normal DTW algorithm with a double bandwidth and with double distance for 
 * diagonal steps in the DTW matrix. In the end it is normalized by the sum of points in 
 * both characters.
 */ 

DECUMA_HWR_PRIVATE CJK_DISTANCE dltCCharGetCoarseDTW(CJK_COMPRESSED_CHAR * cc1,
								  CJK_COMPRESSED_CHAR * cc2, 
								  CJK_SESSION         * pSession);

/** 
 * Computes a fast, approximative distance.
 */ 
DECUMA_HWR_PRIVATE CJK_DISTANCE dltCCharGetDistanceFast(CJK_COMPRESSED_CHAR * pCCharA,
									 CJK_COMPRESSED_CHAR * pCCharB,
									 DECUMA_INT32          delta, 
									 CJK_SESSION         * pSession);


/**
 */
DECUMA_HWR_PRIVATE CJK_DISTANCE dltCCharGetDistanceFinal(CJK_COMPRESSED_CHAR * pCCharA,
									  CJK_COMPRESSED_CHAR * pCCharB, 
									  CJK_BOOLEAN           bUseFreq, 
									  CJK_BOOLEAN           bDoubleBandwidth, 
									  CJK_SESSION         * pSession);

/**
 */
DECUMA_HWR_PRIVATE CJK_DISTANCE dltCCharGetDistanceNoDB(CJK_COMPRESSED_CHAR * pCCharA,
									 CJK_COMPRESSED_CHAR * pCCharB,
									 DECUMA_UINT16		   uc,
									 CJK_SESSION         * pSession);

/**
	A versiojn of DTW with compensation for diagonal shortcut compared to Manhattan.
	Statically matches component in db coordinate system without mass center refitting.

	Also returns the best matching endpoint in pCCharIn for possible reuse for such a mass center refit.
 */
DECUMA_HWR_PRIVATE CJK_DISTANCE dltCCharGetDTWDistanceComponent(CJK_COMPRESSED_CHAR   * pCCharIn,
									CJK_COMPRESSED_CHAR   * pCCharComponent, 
									DECUMA_UINT32		  * pMatchPointIdx,
									CJK_BOOLEAN             bUseDiff, 
									CJK_SESSION           * pSession);

/**
 */
/*CJK_DISTANCE cc_dist_comp(CJK_COMPRESSED_CHAR * a, const ushort * comp, CJK_SESSION * pSession); */


/**
 * Writes the next character into pCChar.
 * If there is no next character, clears  the fields of pCChar.
 * NB previously called dltCCharGetNext.
 */
DECUMA_HWR_PRIVATE void dltCCharGetNext(CJK_COMPRESSED_CHAR * pCChar, CJK_SESSION * pSession);


/**
 * It is assumed that all characters in a linked list have the same
 * number of points. The method dltCCharGetNext therefore does not need to
 * update that information. The index in the database is syncronized
 * so that in order to get the index for the next character we need only
 * add one to the present index. If a character is not allowed as an
 * interpretation according to the [[categorymask]] then we skip
 * it and go to the next instead.
 * NB previously called cc_next_empty.
 */ 
DECUMA_HWR_PRIVATE void dltCCharSkipUnallowed(CJK_COMPRESSED_CHAR * pCChar, CJK_SESSION * pSession);


/**
 * NB previously called cc_next_empty_fast 
 */
DECUMA_HWR_PRIVATE void dltCCharSkipNext(CJK_COMPRESSED_CHAR * pCChar, CJK_SESSION * pSession);

/**
 * Checks if the prototype has a form allowed. This is used to enable traditional
 * forms of simplified characters and vice versa.
 */ 
DECUMA_HWR_PRIVATE CJK_BOOLEAN dltCCharHasAllowedForm(CJK_COMPRESSED_CHAR * pCChar, CJK_SESSION * pSession);


/**
 * This computes an additional punishment for difference in
 * number of points and a little for number of strokes too.
 */ 
DECUMA_HWR_PRIVATE CJK_DISTANCE dltCCharGetPointDifferencePunishDistance(CJK_COMPRESSED_CHAR * pCCharA,
													  CJK_COMPRESSED_CHAR * pCCharB);


/**
 * 
 * A main problem with the speaking -- threedrops confusion is
 * that people can actually mean threedrops when they write
 * speaking. A test showed that the punishment 20 is good.
 * 
 * The code only executes if simplified is selected.
 * 
 * In the table below the numbers in parenthesis are hitrates without
 * the extra punishment for number of points. It can be seen that
 * we indeed often increase the hitrate a few percent using this technique.
 * \f[
 * \begin{tabular}{|l|r|r|}
 * \hline
 * Hitrates   & First guess & Any of the ten first \\
 * \hline
 * \hline
 * Clean 5x280 chars        & 96.5\% (96.1\%) & 99.1\% (99.1\%) \\
 * \hline
 * Semi-cursive 5x168 chars & 86.7\% (82.3\%) & 97.0\% (96.4\%) \\
 * \hline
 * Semi-cursive 4x276 chars & 51.5\% (47.7\%) & 79.3\% (77.1\%) \\
 * \hline
 * Nokia 250 A data         & 93.2\% (92.8\%) & 99.2\% (98.8\%) \\
 * \hline
 * Nokia 250 B data         & 62.8\% (60.0\%) & 92.8\% (91.2\%)\\
 * \hline
 * \end{tabular}
 * \f]
 * The theory behind this is that the distribution for any parameter
 * should look the same for correct candidates and erroneous
 * candidates. If there is no punishment for the difference in
 * number of points then we get a result shown in the bottom figure below
 * for the datamaterial [[d_nokia_A]].
 * With the punishment we get histograms that look approximately
 * the same, and the hit rate indeed increses as seen in the table above.
 * 
 * \f[ \epsfig{file=../../figures/histograms} \f]
 * --------------------------------------------------------------------------
 */
DECUMA_HWR_PRIVATE DECUMA_INT32 dltCCharCompStartPunish(CJK_COMPRESSED_CHAR * pCCharA, CJK_COMPRESSED_CHAR * pCCharB, CJK_SESSION * pSession);


/**
 */ 
DECUMA_HWR_PRIVATE DECUMA_INT32 dltCCharGetDelta(CJK_COMPRESSED_CHAR * pCCharA, CJK_COMPRESSED_CHAR * pCCharB);


/**
 * This function returns the index of the start component of a
 * database character.
 */ 
DECUMA_HWR_PRIVATE DECUMA_INT32 dltCCharGetStartComponent(CJK_COMPRESSED_CHAR * pCChar, CJK_SESSION * pSession);


/**
 * This function sets the featurebits depending on if the character is dense or sparsely sampled. It maps the
 * featbits pointer in CJK_COMPRESSED_CHAR to the storage area allocated for input features in the CJK_SESSION object.
 */ 
DECUMA_HWR_PRIVATE DECUMA_STATUS dltCCharSetFeatureBits(CJK_COMPRESSED_CHAR * const pCChar, CJK_SESSION * const pSession);


/**
 */ 
DECUMA_HWR_PRIVATE void dltCCharSwiftData(CJK_COMPRESSED_CHAR * pCChar, CJK_SESSION * pSession);


/**
 * This function sets the byte code of a component's index and returns the number
 * of bytes (1 or 2) which was needed to code the component.
 */
/*DECUMA_INT32 cc_getcompbytecode(ushort compindex, uchar * byte1, uchar * byte2, CJK_SESSION * pSession); */


/**
 * This method takes a chinese character and calculates how many intersections
 * there are in the character. It uses the function @ref dltGPIterGetIntersectionCount.
 */ 
DECUMA_HWR_PRIVATE DECUMA_INT32 dltCCharGetIntersectionCount(CJK_COMPRESSED_CHAR * pCChar, CJK_SESSION * pSession);


/**
 * @returns true if the stroke has a hook at the end. It uses
 * both the ordinary subsampled stroke and the original data to
 * analyze this. Note that the second argument is {\em one-based index}.
 */ 
DECUMA_HWR_PRIVATE CJK_BOOLEAN dltCCharHasHook(CJK_COMPRESSED_CHAR * pCChar, DECUMA_INT32 nStrokeIdx, CJK_STROKE * pStroke);

#ifdef CJK_ENABLE_INTERNAL_API
DECUMA_HWR_PRIVATE CJK_BOOLEAN dltCCharFromCurve(CJK_SESSION * pSession, CJK_COMPRESSED_CHAR * pCChar, DECUMA_CURVE * pCurve);
#endif /* CJK_ENABLE_INTERNAL_API */






/* --------------------------------------------------------------------------
 * Exported functions - CJK_GRIDPOINT methods
 * -------------------------------------------------------------------------- */

/** 
 * This method takes two @ref CJK_GRIDPOINT and calculate the difference. 
 * The direction is seen as a new gridpoint, approximated into a 
 * "civilization grid". See figure. 
 * The line y = sqrt(14) * x is interesting since
 * atan(sqrt(14)) ~= 75 degrees. For more detailes, ask FM.
 * 
 * \f[ \epsfig{file=../../figures/grid} \f]
 */
DECUMA_HWR_PRIVATE CJK_GRIDPOINT dltGPGetDirection(CJK_GRIDPOINT last_gp, CJK_GRIDPOINT next_gp);






/* --------------------------------------------------------------------------
 * Exported functions - CJK_GRIDPOINT_ITERATOR methods
 * -------------------------------------------------------------------------- */


/**
 * Initialize a CJK_GRIDPOINT_ITERATOR object allocated by the caller for
 */
DECUMA_HWR_PRIVATE void dltGPIterInit(CJK_GRIDPOINT_ITERATOR * pGPIter, CJK_COMPRESSED_CHAR * pCChar, CJK_SESSION * pSession);


/**
 * Takes a gridpoint iterator and returns the order of the gridpoint that is
 * the closest one to the first gridpoint in the iterator, except from the
 * njump closest.
 */
DECUMA_HWR_PRIVATE DECUMA_INT32 dltGPIterGetClosestPos(CJK_GRIDPOINT_ITERATOR * pGPIter, DECUMA_INT32 nPoints, DECUMA_UINT8 njump, CJK_SESSION * pSession);


/**
 * Takes an griditerator and returns the number of intersections
 * in the pointflow. The inargument nPoints tells how many points maximum that
 * should be concidered in the pointflow. Note, CJK_GRIDPOINT_ITERATOR has to be initialized 
 * first.
 * This function is used by the functions @ref dltCCharGetIntersectionCount and 
 * @ref cjkStrokeGetIntersectionCount. In a future structure of the delight project, this function
 * should be hidden, and the ones using it should be visible. To count an 
 * intersection the lines really have to cross, see figure 
 * \f[ \epsfig{file=../../figures/intersection} \f]
 *
 * npointsinter = 0
 * 
 * The case npointsinter == 0 is easy, see figure \ref{fig:intersections0}.
 * \f{figure}[h!]
 *     \begin{center}
 *       \epsfig{file=../../figures/intersection0.eps}
 *     \end{center}
 * \caption{npointsinter = 0} \label{fig:intersections0}
 * \f}
 *
 *
 *
 * npointsinter = 1
 * 
 * For npointsinter == 1, we have to look forward and see if the next point
 * is on the other side of the segment, so we get an intersection.
 * Two cases have to be considered, see figure \ref{fig:intersections1}
 * 
 * \f{figure}[h!]
 *     \begin{center}
 *       \epsfig{file=../../figures/intersection1.eps}
 *     \end{center}
 * \caption{npointsinter = 1} \label{fig:intersections1}
 * \f}
 *
 *
 * npointsinter = 2
 * 
 * The special case from hell! Bring a cup of coffe, look at the picture and let
 * the brain boil of logic.
 * 
 * \f{figure}[h!]
 *     \begin{center}
 *       \epsfig{file=../../figures/intersection2.eps}
 *     \end{center}
 * \caption{npointsinter = 2} \label{fig:intersections2}
 * \f}
 */
DECUMA_HWR_PRIVATE DECUMA_INT32 dltGPIterGetIntersectionCount(CJK_GRIDPOINT_ITERATOR * pGPIter, DECUMA_INT32 nPoints, CJK_SESSION * pSession);

#ifdef _DEBUG
#include <stdio.h>

#define PRINTIDX1 1701
#define PRINTIDX2 1698

void dltCCharDebugDump(FILE * fh, CJK_COMPRESSED_CHAR * pCChar);
#endif
/** @} */

#endif /* CJK_COMPRESSED_CHARACTER_H_ */
