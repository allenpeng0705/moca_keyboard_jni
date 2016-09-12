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

#define CJK_CC_COMPRESS_C
#define CJK_SYSTEM_CODE

#include "decuma_hwr_types.h"
#include "cjkCompressedCharacter_Macros.h"
#include "cjkDatabaseFormat_Macros.h"
#include "cjkStroke_Macros.h"
#include "cjkSession_Types.h"
#include "cjkArcSession.h"
#include "cjkDatabase.h"
#include "cjkMath.h"
#include "cjkCoarseFeatures.h"
#include "cjkCommon.h"

#include "cjkDynamicDatabase_Types.h" /* Need the macro DYNAMIC_DB_STARTINDEX */

#include "decumaCommon.h"
#include "decumaMemory.h"
#include "decumaInterrupts.h"
#include "decumaIntegerMath.h"

/** @addtogroup DOX_CJK_CCHAR
 *  @{
 */


/* -------------------------------------------------------------------------
 * Macros
 * ------------------------------------------------------------------------- */

/** 
 * Used in @ref dltCCharGetDTWDistance for telling the end of [[bss]], which is a vector 
 * of indicator variables telling the start of a stroke in the [[b]] character 
 */
#define STOPVALUE 0xFF

/** Used in @ref dltCCharGetIntersectionCount */
#define BIGNUMBER 1000

/** Used in cc_dist_comp() */
#define CC_COMP_END 9999

/** @hideinitializer
 *  Copy the whole vector pTotDist.
 */
#define TOTDIST_COPY(m_dest, m_src) \
	m_dest[0] = m_src[0]; m_dest[1] = m_src[1]; m_dest[2] = m_src[2]; m_dest[3] = m_src[3]; m_dest[4] = m_src[4]; \
	m_dest[5] = m_src[5]; m_dest[6] = m_src[6]; m_dest[7] = m_src[7]; m_dest[8] = m_src[8]; decumaAssert(BW == 9)

#define TOTDIST_COPY_DOUBLE(m_dest, m_src) \
	m_dest[0] = m_src[0]; m_dest[1] = m_src[1]; m_dest[2] = m_src[2]; m_dest[3] = m_src[3]; m_dest[4] = m_src[4]; \
	m_dest[5] = m_src[5]; m_dest[6] = m_src[6]; m_dest[7] = m_src[7]; m_dest[8] = m_src[8]; \
	m_dest[9] = m_src[9]; m_dest[10] = m_src[10]; m_dest[11] = m_src[11]; m_dest[12] = m_src[12]; m_dest[13] = m_src[13]; \
	m_dest[14] = m_src[14]; m_dest[15] = m_src[15]; m_dest[16] = m_src[16]; decumaAssert(BW_DOUBLE == 17)

/**
 * @hideinitializer
 * Overwrite a value of m_dest with the corresponding m_src value if it is smaller.
 */
#define TOTDIST_MIN(m_dest,m_src) \
	if (m_src[0] < m_dest[0]) m_dest[0] = m_src[0]; if (m_src[1] < m_dest[1]) m_dest[1] = m_src[1]; \
	if (m_src[2] < m_dest[2]) m_dest[2] = m_src[2]; if (m_src[3] < m_dest[3]) m_dest[3] = m_src[3]; \
	if (m_src[4] < m_dest[4]) m_dest[4] = m_src[4]; if (m_src[5] < m_dest[5]) m_dest[5] = m_src[5]; \
	if (m_src[6] < m_dest[6]) m_dest[6] = m_src[6]; if (m_src[7] < m_dest[7]) m_dest[7] = m_src[7]; \
	if (m_src[8] < m_dest[8]) m_dest[8] = m_src[8]; decumaAssert(BW == 9)

/**
 * @hideinitializer
 * Overwrite a value of m_dest with the corresponding m_src value if it is smaller when using
 * double bandwidth.
 */
#define TOTDIST_MIN_DOUBLE(m_dest,m_src) \
	if (m_src[0] < m_dest[0]) m_dest[0] = m_src[0]; if (m_src[1] < m_dest[1]) m_dest[1] = m_src[1]; \
	if (m_src[2] < m_dest[2]) m_dest[2] = m_src[2]; if (m_src[3] < m_dest[3]) m_dest[3] = m_src[3]; \
	if (m_src[4] < m_dest[4]) m_dest[4] = m_src[4]; if (m_src[5] < m_dest[5]) m_dest[5] = m_src[5]; \
	if (m_src[6] < m_dest[6]) m_dest[6] = m_src[6]; if (m_src[7] < m_dest[7]) m_dest[7] = m_src[7]; \
	if (m_src[8] < m_dest[8]) m_dest[8] = m_src[8]; \
	if (m_src[9] < m_dest[9]) m_dest[9] = m_src[9]; if (m_src[10] < m_dest[10]) m_dest[10] = m_src[10]; \
	if (m_src[11] < m_dest[11]) m_dest[11] = m_src[11]; if (m_src[12] < m_dest[12]) m_dest[12] = m_src[12]; \
	if (m_src[13] < m_dest[13]) m_dest[13] = m_src[13]; if (m_src[14] < m_dest[14]) m_dest[14] = m_src[14]; \
	if (m_src[15] < m_dest[15]) m_dest[15] = m_src[15]; if (m_src[16] < m_dest[16]) m_dest[16] = m_src[16]; \
	decumaAssert(BW_DOUBLE == 17)

/* The naming convention here is that UDL means "update left", */
/* UDR means "update right" and ADD means "add the new point */
/* distance." The variable r is the current point in the b character */
/* adjusted with mean point difference delta. */

/** @hideinitializer */
#define GAP(m_totDist, m_nIdx, m_pUserGp, m_pUserStrokeStart, m_pDbStrokeStart, m_left) \
	m_left = m_totDist[m_nIdx - 1];                                                     \
	if (m_pUserStrokeStart[m_nIdx]) {                                                   \
		if (CJK_GP_GET_SQ_DISTANCE(m_pUserGp[m_nIdx], m_pUserGp[m_nIdx - 1]) > 2)       \
			m_left += 40;                                                               \
		if (!*m_pDbStrokeStart)                                                         \
		m_totDist[m_nIdx] += 40;                                                        \
	}

/** @hideinitializer */
#define UDLG(m_totDist, m_nIdx, m_left) \
	if (m_left < m_totDist[m_nIdx]) m_totDist[m_nIdx] = m_left

/** @hideinitializer */
#define UDL(m_totDist, m_nIdx) \
	if (m_totDist[m_nIdx - 1] < m_totDist[m_nIdx]) m_totDist[m_nIdx] = m_totDist[m_nIdx - 1]

/** @hideinitializer */
#define UDR(m_totDist, m_nIdx) \
	if (m_totDist[m_nIdx + 1] < m_totDist[m_nIdx]) m_totDist[m_nIdx] = m_totDist[m_nIdx + 1]

/** @hideinitializer */
#define ADD(m_totDist, m_nIdx, m_pUserGp, m_dbGp) \
	m_totDist[m_nIdx] += CJK_GP_GET_SQ_DISTANCE(m_pUserGp[m_nIdx], m_dbGp)

/** @hideinitializer */
#define UDL_DIAG(m_totDist, m_nIdx, m_pUserGp, m_dbGp, m_bDiag) \
	if (m_totDist[m_nIdx - 1] < m_totDist[m_nIdx] + CJK_GP_GET_SQ_DISTANCE(m_pUserGp[m_nIdx], m_dbGp)) { \
		m_totDist[m_nIdx] = m_totDist[m_nIdx - 1];														 \
		m_bDiag = 0;																					 \
	}

/** @hideinitializer */
#define UDR_DIAG(m_totDist, m_nIdx, m_pUserGp, m_dbGp, m_bDiag) \
	if (m_totDist[m_nIdx + 1] < m_totDist[m_nIdx] + CJK_GP_GET_SQ_DISTANCE(m_pUserGp[m_nIdx], m_dbGp)) { \
		m_totDist[m_nIdx] = m_totDist[m_nIdx + 1];														 \
		m_bDiag = 0;																					 \
	}

/** @hideinitializer */
#define ADD_DIAG(m_totDist, m_nIdx, m_pUserGp, m_dbGp, m_bDiag) \
	m_totDist[m_nIdx] += CJK_GP_GET_SQ_DISTANCE(m_pUserGp[m_nIdx], m_dbGp);				 \
	if (m_bDiag) m_totDist[m_nIdx] += CJK_GP_GET_SQ_DISTANCE(m_pUserGp[m_nIdx], m_dbGp); \
	m_bDiag = 1


/**
	The right side of the table below is shifted up one position due to
	the carry from the fourth bit in the subtraction.
	Column eight (or nine if one starts counting from one as ordinary
	people do) is special.
	A difference of 8 ticks has then occurred in the @c x direction
	(the low nybble), but we don't know whether it was to the left or right.
	Consider the hexadecimal difference [[18]].
	This may be either something like hexadecimal 6A-52 or something like
	72-5A.
	We don't know whether there was a carry or not from the fourth bit.
	Then we suppose that the shortest way in the @c y direction
	(the upper nybble) is correct, i.e., that it was something like 6A-52.

	Here is the Matlab (or Octave) program that computed the table.

	@code
	for i = 1:17
	   for j=1:9
		  d(i,j) = (i-1)^2 + (j-1)^2;
	   end
	end
	dL = [d,; d(16:-1:2, :)];
	dR = fliplr(dL);
	dR = [dR(2:32, :); dR(1, :)];
	col8 = min([dL(:, 9), dR(:, 1)]')';
	dd = [dL(:,1:8), col8, dR(:, 2:8)];
	dd = min(dd, 255);
	for i = 1:32
	   for j = 1:16
		   fprintf('//3g,', dd(i, j));
	   end
	   fprintf('\n');
	end
	@endcode

	This is equivalent to the folowing program.

	@code
	clear d
	for i = 1:32
	   for j=1:16
		  x = min(j-1, 17-j);
		  k = i + ((j>9) | ((j==9) & (i>16)));
		  y = min(k-1, 33-k);
		  d(i, j) = x^2 + y^2;
	   end
	end

	d = min(d, 255)
	@endcode

*/



/* Here is the table. We squeeze it into unsigned bytes instead of */
/* integers. Then 255 is the largest squared distance stored. */

DECUMA_HWR_PRIVATE_DATA_C const DECUMA_UINT8 pGridPointDistTable[] = {
  0,  1,  4,  9, 16, 25, 36, 49, 64, 50, 37, 26, 17, 10,  5,  2,
  1,  2,  5, 10, 17, 26, 37, 50, 65, 53, 40, 29, 20, 13,  8,  5,
  4,  5,  8, 13, 20, 29, 40, 53, 68, 58, 45, 34, 25, 18, 13, 10,
  9, 10, 13, 18, 25, 34, 45, 58, 73, 65, 52, 41, 32, 25, 20, 17,
 16, 17, 20, 25, 32, 41, 52, 65, 80, 74, 61, 50, 41, 34, 29, 26,
 25, 26, 29, 34, 41, 50, 61, 74, 89, 85, 72, 61, 52, 45, 40, 37,
 36, 37, 40, 45, 52, 61, 72, 85,100, 98, 85, 74, 65, 58, 53, 50,
 49, 50, 53, 58, 65, 74, 85, 98,113,113,100, 89, 80, 73, 68, 65,
 64, 65, 68, 73, 80, 89,100,113,128,130,117,106, 97, 90, 85, 82,
 81, 82, 85, 90, 97,106,117,130,145,149,136,125,116,109,104,101,
100,101,104,109,116,125,136,149,164,170,157,146,137,130,125,122,
121,122,125,130,137,146,157,170,185,193,180,169,160,153,148,145,
144,145,148,153,160,169,180,193,208,218,205,194,185,178,173,170,
169,170,173,178,185,194,205,218,233,245,232,221,212,205,200,197,
196,197,200,205,212,221,232,245,255,255,255,250,241,234,229,226,
225,226,229,234,241,250,255,255,255,255,255,255,255,255,255,255,
255,255,255,255,255,255,255,255,255,255,255,250,241,234,229,226,
225,226,229,234,241,250,255,255,255,245,232,221,212,205,200,197,
196,197,200,205,212,221,232,245,233,218,205,194,185,178,173,170,
169,170,173,178,185,194,205,218,208,193,180,169,160,153,148,145,
144,145,148,153,160,169,180,193,185,170,157,146,137,130,125,122,
121,122,125,130,137,146,157,170,164,149,136,125,116,109,104,101,
100,101,104,109,116,125,136,149,145,130,117,106, 97, 90, 85, 82,
 81, 82, 85, 90, 97,106,117,130,128,113,100, 89, 80, 73, 68, 65,
 64, 65, 68, 73, 80, 89,100,113,113, 98, 85, 74, 65, 58, 53, 50,
 49, 50, 53, 58, 65, 74, 85, 98,100, 85, 72, 61, 52, 45, 40, 37,
 36, 37, 40, 45, 52, 61, 72, 85, 89, 74, 61, 50, 41, 34, 29, 26,
 25, 26, 29, 34, 41, 50, 61, 74, 80, 65, 52, 41, 32, 25, 20, 17,
 16, 17, 20, 25, 32, 41, 52, 65, 73, 58, 45, 34, 25, 18, 13, 10,
  9, 10, 13, 18, 25, 34, 45, 58, 68, 53, 40, 29, 20, 13,  8,  5,
  4,  5,  8, 13, 20, 29, 40, 53, 65, 50, 37, 26, 17, 10,  5,  2,
  1,  2,  5, 10, 17, 26, 37, 50, 64, 49, 36, 25, 16,  9,  4,  1
};


/* The unconstrained version of the table looks like this:
const unsigned int pGridPointDistTable[] = {
  0,  1,  4,  9, 16, 25, 36, 49, 64, 50, 37, 26, 17, 10,  5,  2,
  1,  2,  5, 10, 17, 26, 37, 50, 65, 53, 40, 29, 20, 13,  8,  5,
  4,  5,  8, 13, 20, 29, 40, 53, 68, 58, 45, 34, 25, 18, 13, 10,
  9, 10, 13, 18, 25, 34, 45, 58, 73, 65, 52, 41, 32, 25, 20, 17,
 16, 17, 20, 25, 32, 41, 52, 65, 80, 74, 61, 50, 41, 34, 29, 26,
 25, 26, 29, 34, 41, 50, 61, 74, 89, 85, 72, 61, 52, 45, 40, 37,
 36, 37, 40, 45, 52, 61, 72, 85,100, 98, 85, 74, 65, 58, 53, 50,
 49, 50, 53, 58, 65, 74, 85, 98,113,113,100, 89, 80, 73, 68, 65,
 64, 65, 68, 73, 80, 89,100,113,128,130,117,106, 97, 90, 85, 82,
 81, 82, 85, 90, 97,106,117,130,145,149,136,125,116,109,104,101,
100,101,104,109,116,125,136,149,164,170,157,146,137,130,125,122,
121,122,125,130,137,146,157,170,185,193,180,169,160,153,148,145,
144,145,148,153,160,169,180,193,208,218,205,194,185,178,173,170,
169,170,173,178,185,194,205,218,233,245,232,221,212,205,200,197,
196,197,200,205,212,221,232,245,260,274,261,250,241,234,229,226,
225,226,229,234,241,250,261,274,289,305,292,281,272,265,260,257,
256,257,260,265,272,281,292,305,289,274,261,250,241,234,229,226,
225,226,229,234,241,250,261,274,260,245,232,221,212,205,200,197,
196,197,200,205,212,221,232,245,233,218,205,194,185,178,173,170,
169,170,173,178,185,194,205,218,208,193,180,169,160,153,148,145,
144,145,148,153,160,169,180,193,185,170,157,146,137,130,125,122,
121,122,125,130,137,146,157,170,164,149,136,125,116,109,104,101,
100,101,104,109,116,125,136,149,145,130,117,106, 97, 90, 85, 82,
 81, 82, 85, 90, 97,106,117,130,128,113,100, 89, 80, 73, 68, 65,
 64, 65, 68, 73, 80, 89,100,113,113, 98, 85, 74, 65, 58, 53, 50,
 49, 50, 53, 58, 65, 74, 85, 98,100, 85, 72, 61, 52, 45, 40, 37,
 36, 37, 40, 45, 52, 61, 72, 85, 89, 74, 61, 50, 41, 34, 29, 26,
 25, 26, 29, 34, 41, 50, 61, 74, 80, 65, 52, 41, 32, 25, 20, 17,
 16, 17, 20, 25, 32, 41, 52, 65, 73, 58, 45, 34, 25, 18, 13, 10,
  9, 10, 13, 18, 25, 34, 45, 58, 68, 53, 40, 29, 20, 13,  8,  5,
  4,  5,  8, 13, 20, 29, 40, 53, 65, 50, 37, 26, 17, 10,  5,  2,
  1,  2,  5, 10, 17, 26, 37, 50, 64, 49, 36, 25, 16,  9,  4,  1
};
*/

/** @hideinitializer
 * Used for initialising the dtw algorithm in @ref dltCCharGetDTWDistance, and for picking
 * out the final value at the end.
 */
/*#ifndef CJK_DB_DTW_BW */
static const DECUMA_UINT8 startindex[2 * BW - 1] =
	{8, 7, 7, 6, 6, 5, 5, 4, 4, 4, 3, 3, 2, 2, 1, 1, 0};
/*#else */
static const DECUMA_UINT8 startindex_double[2 * BW_DOUBLE - 1] =
	{16, 15, 15, 14, 14, 13, 13, 12, 12, 11, 11, 10, 10, 9, 9,
	  8, 8, 8, 7, 7, 6, 6, 5, 5, 4, 4, 3, 3, 2, 2, 1, 1, 0};
/*#endif */

/** @hideinitializer
 * Used for initialising the dtw algorithm in @ref dltCCharGetDTWDistance, and for picking
 * out the final value at the end.
 */
/*#ifndef CJK_DB_DTW_BW */
static const DECUMA_UINT8 stopindex[2 * BW - 1]  =
	{0, 0, 1, 1, 2, 2, 3, 3, 4, 5, 5, 6, 6, 7, 7, 8, 8};
/*#else */
static const DECUMA_UINT8 stopindex_double[2 * BW_DOUBLE - 1]  =
	{0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8,
	 9, 9, 10, 10, 11, 11, 12, 12, 13, 13, 14, 14, 15, 15, 16, 16};
/*#endif */





/* -------------------------------------------------------------------------
 * Local function declarations
 * ------------------------------------------------------------------------- */


/** The [[cc_dist_sof1]] method is an internal subroutine that
 *  may give different results when the input arguments are swapped.
 */
static CJK_DISTANCE cc_dist_sof1(
	CJK_COMPRESSED_CHAR * pCCharUser, 
	CJK_COMPRESSED_CHAR * pCCharDb, 
	DECUMA_INT32          delta, 
	CJK_DISTANCE          distanceLimit, 
	CJK_SESSION         * pSession);


/**
 * Here is the innermost chunk of the whole program.
 * The goal is to update the nine cells in the vector [[pTotDist]].
 * See the figure on page 45.
 * 
 * The [[pTotDist]] buffer updating is unrolled for speed.
 * This decresed the time consumption for dynamic time warping maching
 * from 70 to 20 seconds in an experiment.
 * The rest of the program took 20 seconds in the same experiment
 * so the total execution time was reduced from 90 to 40 seconds.
 * 
 */
static void updateTotalDistance_gap(
	      CJK_DISTANCE   * pTotDist, 
	const DECUMA_UINT8   * pUserGridPoints, 
	const DECUMA_INT32     r,
	const DECUMA_UINT8   * pCurrUserStrokeStart, 
	const DECUMA_UINT8   * pCurrDbStrokeStart,
		  CJK_BOOLEAN      bDoubleBandwidth,
	      DECUMA_INT32     nUserGpIdx
#ifdef CJK_DTW_ALIGNMENT
	      ,
	      DECUMA_UINT32  * pDtwMatrixRow
#endif
		);


/**
 * The same but we skip the gap-punishment. The pGridPointDistTable argument
 * is there because of the ARM assembler implementation.
 */ 
static void updateTotalDistance_nogap(
          CJK_DISTANCE  * pTotDist, 
	const DECUMA_UINT8  * pUserGridPoints, 
	const DECUMA_INT32    r,
	const DECUMA_UINT8  * pGridPointDistTable,
		  CJK_BOOLEAN     bDoubleBandwidth,
	      DECUMA_INT32    nUserGpIdx
#ifdef CJK_DTW_ALIGNMENT
	      ,
	      DECUMA_UINT32 * pDtwMatrixRow
#endif
		);

/** 
 * Unfortunately this tricky part digs right into the internal data format
 * -- bypassing the encapsulation routines -- since it must be fast.
 * The internal database format is described in chapter 8, the database
 * chapter. See also figure 1 on page \ref{bgpfig}.
 * 
 * This is not inlined with a chunk in [[dltCCharGetDTWDistance]]
 * for the sole purpose that we want to profile the execution time.
 */
static void dltCCharCopyDbPoints(CJK_COMPRESSED_CHAR         const * const b, 
						 CJK_GRIDPOINT      (* const bgp)[MAXPNTSPERSTR],
						 DECUMA_UINT8       (* const bss)[MAXPNTSPERSTR], 
						 DECUMA_UINT8    componentVariationFork[], 
						 CJK_SESSION       const * const pSession);

/* -------------------------------------------------------------------------
 * Exported functions
 * ------------------------------------------------------------------------- */

DECUMA_UINT32 cjkCompressedCharacterGetSize(void)
{
	return sizeof(CJK_COMPRESSED_CHAR);
}

DECUMA_UINT32 cjkCompressedCharacterGetScratchpadSize(void)
{
	return sizeof(CJK_CC_COMPRESS_SCRATCHPAD);
}

DECUMA_UINT32 cjkCompressedCharacterDTWDistGetScratchpadSize(void)
{
	return sizeof(CJK_CC_DTW_SCRATCHPAD);
}

void cjkCompressedCharacterSetDTWCached(CJK_COMPRESSED_CHAR * pCChar, int bIsCached)
{
	decumaAssert(pCChar);
	pCChar->dtwCached = bIsCached;
}

void dltCCCompressSetIndex(CJK_COMPRESSED_CHAR * pCChar, DLTDB_INDEX index)
{
	decumaAssert(pCChar);
	/* Need to improve index size check since dynamic db also makes use of this ...*/
	decumaAssert(index >= 0);
	pCChar->index = index;
	return;
}

#if defined(_DEBUG) 

DLTDB_INDEX dltCCCompressGetIndex(CJK_COMPRESSED_CHAR * pCChar)
{
	decumaAssert(pCChar);

	return pCChar->index;
}

DECUMA_INT32 dltCCCompressGetNbrStrokes(CJK_COMPRESSED_CHAR * pCChar)
{
	decumaAssert(pCChar);

	return pCChar->nStrokes;
}

DECUMA_INT32 dltCCCompressGetNbrPoints(CJK_COMPRESSED_CHAR * pCChar)
{
	decumaAssert(pCChar);

	return pCChar->nPoints;
}

int dltCCCompressGetYmin(CJK_COMPRESSED_CHAR * pCChar)
{
	decumaAssert(pCChar);

	return pCChar->ymin;
}

int dltCCCompressGetYmax(CJK_COMPRESSED_CHAR * pCChar)
{
	decumaAssert(pCChar);

	return pCChar->ymax;
}

int dltCCCompressGetXmin(CJK_COMPRESSED_CHAR * pCChar)
{
	decumaAssert(pCChar);

	return pCChar->xmin;
}

int dltCCCompressGetXmax(CJK_COMPRESSED_CHAR * pCChar)
{
	decumaAssert(pCChar);

	return pCChar->xmax;
}

int dltCCCompressIsDense(CJK_COMPRESSED_CHAR * pCChar)
{
	decumaAssert(pCChar);

	return ((pCChar->attrib) & AM_ISDENSE);
}

DECUMA_FEATURE dltCCCompressGetFeatureVal(CJK_COMPRESSED_CHAR * pCChar, int nFeatureIdx)
{
	decumaAssert(pCChar);
	decumaAssert(nFeatureIdx >= 0 && nFeatureIdx < COARSE_NBR_FEATURES);

	return pCChar->pFeatures[nFeatureIdx];
}

void dltCCCompressSetNull(CJK_COMPRESSED_CHAR * pCChar)
{
	decumaAssert(pCChar);

	pCChar->pCCData = 0;
	pCChar->nStrokes = 0;
	pCChar->pOriginalStrokes = 0;
	pCChar->nPoints = 0;
	pCChar->index = 0;
	pCChar->attrib = 0;
	return;
}


#endif

void dltCCCompressSetNbrStrokes(CJK_COMPRESSED_CHAR * pCChar, int nStrokes)
{
	decumaAssert(pCChar);
	decumaAssert(nStrokes >= 0 && nStrokes < MAXNSTRK);

	pCChar->nStrokes = nStrokes;
	return;
}

void dltCCCompressSetNbrPoints(CJK_COMPRESSED_CHAR * pCChar, int nPoints)
{
	decumaAssert(pCChar);
	decumaAssert(nPoints >= 0 && nPoints < MAXPNTSPERCHAR);

	pCChar->nPoints = nPoints;
	return;
}

void dltCCCompressGetMaxMin(CJK_COMPRESSED_CHAR * pCChar, int * xmin, int * xmax, int * ymin, int * ymax)
{
	decumaAssert(pCChar && xmin && xmax && ymin && ymax);
	
	*xmin = pCChar->xmin;
	*xmax = pCChar->xmax;
	*ymin = pCChar->ymin;
	*ymax = pCChar->ymax;
	return;
}

DECUMA_ARC * dltCCCompressGetOrgStrokes(CJK_COMPRESSED_CHAR * pCChar)
{
	decumaAssert(pCChar);

	return pCChar->pOriginalStrokes;
}

void dltCCCompressSetAttribute(CJK_COMPRESSED_CHAR * pCChar, DLTDB_ATTRIBUTE attrib)
{
	decumaAssert(pCChar);
	
	pCChar->attrib = attrib;
	return;
}

DLTDB_ATTRIBUTE dltCCCompressGetAttribute(CJK_COMPRESSED_CHAR * pCChar)
{
	decumaAssert(pCChar);

	return pCChar->attrib;
}

#ifdef CJK_ENABLE_INTERNAL_API
CJK_COMPRESSED_CHAR_DATA * dltCCCompressGetCCData(CJK_COMPRESSED_CHAR * pCChar)
#else 
const CJK_COMPRESSED_CHAR_DATA * dltCCCompressGetCCData(CJK_COMPRESSED_CHAR * pCChar)
#endif
{
	decumaAssert(pCChar);

	return pCChar->pCCData;
}

void dltCCCompressSetCCData(CJK_COMPRESSED_CHAR * pCChar, const CJK_COMPRESSED_CHAR_DATA * pCCData)
{
	decumaAssert(pCChar);
	decumaAssert(pCCData);

	pCChar->pCCData = pCCData;

	return;
}

void dltCCompressSetDotStart(CJK_COMPRESSED_CHAR * pCChar, int dotStart)
{
	decumaAssert(pCChar);
	decumaAssert(dotStart >= 0 && dotStart <=1);

	pCChar->dotstart = dotStart;
	return;
}

void dltCCompressSetNotSpeaking(CJK_COMPRESSED_CHAR * pCChar, int bNotSpeaking)
{
	decumaAssert(pCChar);
	decumaAssert(bNotSpeaking >= 0 && bNotSpeaking <=1);

	pCChar->bIsNotSpeaking = bNotSpeaking;
	return;
}

int dltCCompressGetNotThreeDrops(CJK_COMPRESSED_CHAR * pCChar)
{
	decumaAssert(pCChar);

	return pCChar->bIsNotThreeDrops;
}

void dltCCompressSetNotThreeDrops(CJK_COMPRESSED_CHAR * pCChar, int bNotThreeDrops)
{
	decumaAssert(pCChar);
	decumaAssert(bNotThreeDrops >= 0 && bNotThreeDrops <=1);

	pCChar->bIsNotThreeDrops = bNotThreeDrops;
	return;
}

void dltCCompressSetNotSoil(CJK_COMPRESSED_CHAR * pCChar, int bNotSoil)
{
	decumaAssert(pCChar);
	decumaAssert(bNotSoil >= 0 && bNotSoil <=1);

	pCChar->bIsNotSoil = bNotSoil;
	return;
}


CJK_BOOLEAN dltCCharHasHook(CJK_COMPRESSED_CHAR * pCChar, DECUMA_INT32 nStrokeIdx, CJK_STROKE * pStroke) 
{
	CJK_BOOLEAN    bHasHook;
	int            i;
	DECUMA_INT32   nPoints;
	const DECUMA_POINT * pPoints;

	/* WARNING: sample uses 1-based index while original stroke uses 0-based array (direct access) */
	nStrokeIdx--;

	decumaAssert(0 <= nStrokeIdx);
	decumaAssert(pCChar->pOriginalStrokes != 0);

	nPoints = pCChar->pOriginalStrokes[nStrokeIdx].nPoints;
	pPoints = (const DECUMA_POINT*) pCChar->pOriginalStrokes[nStrokeIdx].pPoints;
	
	bHasHook = 
		(CJK_STROKE_GET_Y(pStroke, -1) <= CJK_STROKE_GET_Y(pStroke, -2)) &&
		(CJK_STROKE_GET_X(pStroke, -3) - CJK_STROKE_GET_X(pStroke, -1) >= 2);
	
	if (!bHasHook) {
		DECUMA_INT32 x_end = pPoints[nPoints - 1].x;
		DECUMA_INT32 y_end = pPoints[nPoints - 1].y;

		for (i = nPoints - 2; i >= 2; i--) {
			DECUMA_INT32 x = pPoints[i].x;
			DECUMA_INT32 y = pPoints[i].y;
		
			if (x - x_end >= 4 && (y_end - y) < (x - x_end)) {
				bHasHook = 1;
			}
		}
	}

	return bHasHook;
}


DECUMA_INT32 dltCCharGetDelta(CJK_COMPRESSED_CHAR * pCCharA, CJK_COMPRESSED_CHAR * pCCharB) 
{
	DECUMA_INT32 xm, ym, nn;

	/* TODO division by zero possible */
	nn = pCCharA->nPoints * pCCharB->nPoints;
	xm = pCCharB->nPoints * pCCharA->xSum - pCCharA->nPoints * pCCharB->xSum;
	xm = (xm + 8 * nn + nn / 2) / nn - 8;
	ym = pCCharB->nPoints * pCCharA->ySum - pCCharA->nPoints * pCCharB->ySum;
	ym = (ym + 8 * nn + nn / 2) / nn - 8;
	return (ym << 4) + xm;
}


void dltCCharGetLastStroke(CJK_COMPRESSED_CHAR * pCChar, CJK_STROKE * pStroke, CJK_SESSION * pSession)
{
	CJK_STROKE nextStroke;
	CJK_STROKE prevStroke;

	dltCCharGetFirstStroke(pCChar, &prevStroke, pSession);

	nextStroke = prevStroke;

	while CJK_STROKE_EXISTS(&nextStroke) {
		prevStroke = nextStroke;
		cjkStrokeNext(&nextStroke, pSession);
	}

	pStroke->stdata        = prevStroke.stdata;
	pStroke->end           = prevStroke.end;
	pStroke->stdata_backup = prevStroke.stdata_backup;
	pStroke->end_backup    = prevStroke.end_backup;
}


CJK_DISTANCE dltCCharGetRawDistance(CJK_COMPRESSED_CHAR * pCCharA, 
						 CJK_COMPRESSED_CHAR * pCCharB, 
						 DECUMA_INT32          delta, 
						 CJK_SESSION         * pSession)
{
	CJK_DISTANCE dist = 0;
	CJK_STROKE strokeA;
	CJK_STROKE strokeB;

	decumaAssert(CJK_CCHAR_IS_DENSE(pCCharA) == CJK_CCHAR_IS_DENSE(pCCharB));

	dltCCharGetFirstStroke(pCCharA, &strokeA, pSession);
	dltCCharGetFirstStroke(pCCharB, &strokeB, pSession);

	while (CJK_STROKE_EXISTS(&strokeA) && CJK_STROKE_EXISTS(&strokeB)) {
		dist += cjkStrokeGetDistanceDTW(&strokeA, &strokeB, delta);
		cjkStrokeNext(&strokeA, pSession);
		cjkStrokeNext(&strokeB, pSession);
	}

	if (CJK_STROKE_EXISTS(&strokeA) || CJK_STROKE_EXISTS(&strokeB)) return LARGEDIST;

	return dist;
}



CJK_DISTANCE dltCCharGetDistanceFast(CJK_COMPRESSED_CHAR * pCCharA,
						  CJK_COMPRESSED_CHAR * pCCharB,
						  DECUMA_INT32          delta, 
						  CJK_SESSION         * pSession)
						  
{
	CJK_DISTANCE dist = 0;
	CJK_COMPRESSED_CHAR * c;
	CJK_GRIDPOINT agp, bgp, agp_, bgp_;
	CJK_GRIDPOINT_ITERATOR agi, bgi;
	DECUMA_INT32 na = CJK_CCHAR_NPOINTS(pCCharA), ia = 0;
	DECUMA_INT32 nb = CJK_CCHAR_NPOINTS(pCCharB), ib = 0;
	DECUMA_INT32 sum = 0, nc;

	/*----------------------------------------------------- */
	/* Swap pCCharA and pCCharB if needed, so Bresenham works correctly. */
	/*  */
	if (na < nb) {
		c = pCCharA; pCCharA = pCCharB; pCCharB = c;
		nc = na; na = nb; nb = nc;
		delta = - delta;
	}


	dltGPIterInit(&agi, pCCharA, pSession);
	dltGPIterInit(&bgi, pCCharB, pSession);

	agp = CJK_GPITER_GET_GRIDPOINT(&agi); agp_ = agp;
	bgp = CJK_GPITER_GET_GRIDPOINT(&bgi); bgp_ = bgp;

	while (CJK_GPITER_HAS_GRIDPOINT(&agi)) {
		dist += CJK_GP_GET_SQ_DISTANCE(agp,bgp + delta);
	
		if (ia > 0) {
			dist += CJK_GP_GET_SQ_DISTANCE(agp,bgp_ + delta);
		}

		if (ib > 0) {
			dist += CJK_GP_GET_SQ_DISTANCE(agp_,bgp + delta);
		}

		/*----------------------------------------------------- */
		/* increase the iterators and gridpoints: */
		/*  */
		/* The iterator for pCCharA is always increased, but the iterator for pCCharB is increased */
		/* as in the Bresenham line pixel drawing algorithm. */
		CJK_GPITER_NEXT_INLINE(&agi, pSession);
		agp_ = agp;
		agp  = CJK_GPITER_GET_GRIDPOINT(&agi);
		ia++;
		sum += nb;

		if (sum >= na) {
			sum -= na;
			ib++;
			CJK_GPITER_NEXT_INLINE(&bgi, pSession);
			bgp_ = bgp;
			bgp  = CJK_GPITER_GET_GRIDPOINT(&bgi);
		}
	}
	return dist;
}



/**
 * The method is composed of three parts, one part that unpacks
 * the database character and the character that the user just wrote into
 * a private format, a second optional differetiation step, and
 * the algorithm that carries out the distance calculations.
 *
 * \f{figure}
 * \begin{center}
 *       \epsfig{file=../../figures/dtw.eps}
 * \end{center}
 * \f}
 *
 */
CJK_DISTANCE dltCCharGetDTWDistance(CJK_COMPRESSED_CHAR   * const pCCharUser,
									CJK_COMPRESSED_CHAR   const * const pCCharDb, 
									DECUMA_INT32            const delta, 
									CJK_DISTANCE            const distanceLimit,
									CJK_BOOLEAN             const bUseDiff, 
									CJK_BOOLEAN             const bUseGap, 
									CJK_BOOLEAN             const bUseComp, 
									CJK_BOOLEAN             const bDoubleBandwidth,
									CJK_SESSION           * const pSession)
{

	/** @internal
	 * @name dltCCharGetDTWDistance Local Data
	 * @{
	 * The vector @c pTotDist is illustrated in the above figure.
	 * The vector @c pUserGridPoints will contain the points from the user
	 * data with a little bit of extra length so that the algorithm
	 * can start and end without fuss. The data from the
	 * database is stored in the array @c pDbGridPoints, which is illustrated
	 * in the figure below. The pointers @c pCurrDbGridPoint and @c pCurrUserGridPoint will point
	 * into @c pDbGridPoints and @c pUserGridPoints respectively. The vectors @c pUserStrokeStarts 
	 * and @c pDbStrokeStarts consist of vector indicator variables that tell the start of a stroke
	 * in the respective character. The extra length of @c pUserGridPoints
	 * and @c pUserStrokeStarts is going to be used at the beginning and end of the algorithm.
	 * @}
	 */
	CJK_DISTANCE pTotDist[BW_DOUBLE];
	CJK_DISTANCE d;
	CJK_GRIDPOINT (* const pDbGridPoints)[MAXPNTSPERSTR] = ((CJK_CC_DTW_SCRATCHPAD *)pSession->pScratchpad)->bgp;
	CJK_GRIDPOINT  const * pCurrDbGridPoint;
	DECUMA_UINT8  (* const pDbStrokeStarts)[MAXPNTSPERSTR] = ((CJK_CC_DTW_SCRATCHPAD *)pSession->pScratchpad)->bss;
	DECUMA_UINT8   const * pCurrDbStrokeStart;
	CJK_GRIDPOINT  * pCurrUserGridPoint;
	DECUMA_UINT8   * pCurrUserStrokeStart;
	DECUMA_UINT8   * pCurrUserStrokeStart_back = NULL;
	DECUMA_INT32     ndiff, i;
	DECUMA_INT32     const nPointsUser = CJK_CCHAR_NPOINTS(pCCharUser);
	DECUMA_INT32     const nPointsDb   = CJK_CCHAR_NPOINTS(pCCharDb);

	/* The user character [[pCCharUser]] may be cached so the data is static. */

	CJK_GRIDPOINT * const pUserGridPoints   = ((CJK_CC_DTW_SCRATCHPAD *)pSession->pScratchpad)->cc_dist_dtw_agp;
	DECUMA_UINT8  * const pUserStrokeStarts = ((CJK_CC_DTW_SCRATCHPAD *)pSession->pScratchpad)->cc_dist_dtw_ass;

	/* The algorithm will need to handle */
	/* component variants properly. This means that it will store */
	/* the vector [[pTotDist]] at the end of a variant and reload */
	/* with the state it had at the component start, and then run the algorithm */
	/* on the new variant and select the best variant in each of the [[BW]] */
	/* cells. The variable [[compVariantIndex]] are the index of the present */
	/* component variant and the total naumber of variants in the present */
	/* component. The [[componentVariationFork]] vector is explained in figure on the next page. */

	CJK_DISTANCE    totdist_back[BW_DOUBLE] = { 0 };
	CJK_DISTANCE    totdist_front[BW_DOUBLE] = { 0 };
	DECUMA_INT32    compVariantIndex, compVariantCount;
	DECUMA_INT32    i_back = 0;
	DECUMA_UINT8  * const componentVariationFork = ((CJK_CC_DTW_SCRATCHPAD *)pSession->pScratchpad)->bcvfork;
	DECUMA_UINT32   nCurrUserGridPoint_back = 0;
	CJK_GRIDPOINT * pCurrUserGridPoint_back = NULL;

#ifdef CJK_DTW_ALIGNMENT
	CJK_DISTANCE  * const pDtwMatrix     = (CJK_DISTANCE  *)&(((CJK_CC_DTW_SCRATCHPAD *)pSession->pScratchpad)->dtwMatrix);
	DECUMA_UINT32 * const pDtwAlignment  = ((CJK_CC_DTW_SCRATCHPAD *)pSession->pScratchpad)->pDtwAlignment;
#endif

	int nCurrUserGridPoint;
	int const nBandwidth = bDoubleBandwidth ?  BW_DOUBLE : BW;

	DECUMA_UINT8 const * const pStartIndex = bDoubleBandwidth ? startindex_double : startindex;
	DECUMA_UINT8 const * const pStopIndex  = bDoubleBandwidth ? stopindex_double  : stopindex;

	/* TODO debug only */

	decumaAssert(CJK_CCHAR_IS_DENSE(pCCharUser) == CJK_CCHAR_IS_DENSE(pCCharDb));

	/*----------------------------------------------------- */
	/* BEGIN CHUNK: check number of points */
	/*  */
	/* We assure that the difference in number of points is at most [[nBandwidth]]-1, */
	/* octherwise the algorithm can't run. */
	/*  */
	ndiff = nPointsUser - nPointsDb;
	if (ABS(ndiff) >= nBandwidth || nPointsUser > MAXPNTSPERSTR || nPointsDb > MAXPNTSPERSTR) {
		return LARGEDIST;
	}
	/* END CHUNK: check number of points */
	/*----------------------------------------------------- */


#ifdef CJK_DTW_ALIGNMENT
		/* initialize the matrix */
/*		int row, col; */
/*		for (row = 0; row < nPointsDb; row++) { */
/*			for (col = 0; col < nPointsUser; col++) { */
/*				pDtwMatrix[row * MAXPNTSPERSTR + col] = 0xFFFF; */
/*			} */
/*		} */
	decumaMemset(pDtwMatrix, 0xFF, (MAXPNTSPERSTR * nPointsDb + nPointsUser) * sizeof(CJK_DISTANCE));
#endif

	if (!pCCharUser->dtwCached || bUseDiff) {

		/*----------------------------------------------------- */
		/* BEGIN CHUNK: copy points pCCharUser */
		/*  */
		/*  */
		/* The points in the user character is copied into place so that the */
		/* algorithm starts out correctly.  */
		/*  */
		{
			DECUMA_UINT8 segstart;
			const DECUMA_UINT8 *p, *p_end;
			DECUMA_INT32 j;
			p = (const DECUMA_UINT8 *) pCCharUser->pCCData; /* Source */
			p_end = p + *p;
			p++;
			/*decumaAssert(BW == 9); // else change startindex initialisation */
			pCurrUserGridPoint   = pUserGridPoints   + nBandwidth; /* Dest */
			pCurrUserStrokeStart = pUserStrokeStarts + nBandwidth;
			i = 0;
			while (p != p_end) {
				decumaAssert(*p <= MAXPNTSPERSTR);
				decumaAssert(p < p_end);
				/* TODO Haven't I seen this construct somewhere before? 
				 *      This tranformation is almost what's commented elsewhere in this file, perhaps we need to refactor them into a support function instead.
				 */
				segstart = 1;
				for (j = *p++; j != 0; j--) {
					pCurrUserStrokeStart[i] = segstart;
					decumaAssert(&pCurrUserGridPoint[i] - pUserGridPoints < MAXPNTSPERSTR + nBandwidth); /* Bufsize */
					pCurrUserGridPoint[i++] = *p++;
					segstart = 0;
				}
			}
		}
		/* END CHUNK: copy points pCCharUser */
		/*----------------------------------------------------- */


		pCCharUser->dtwCached = (DECUMA_UINT8) nPointsDb;
	}

	dltCCharCopyDbPoints(pCCharDb, pDbGridPoints, pDbStrokeStarts, componentVariationFork, pSession);

	if (bUseDiff) {

		/**
		 * diff all data
		 * 
		 * All points in the database character are copied into place.
		 * This will be tricky. Figure 1 is
		 * a picture of the end result. The character starts with a
		 * component with three variants, then comes a component with one
		 * variant followed by a segment with no component reference and
		 * finally a component with two variants. Note that there is no
		 * difference in the end result between a component with one variant
		 * and a segment that is not a component.
		 * 
		 * \f{table}
		 * \begin{center}
		 *       \epsfig{file=../../figures/compnent.eps}
		 *   \caption{The matrix pDbGridPoints and the vector componentVariationFork.}
		 *   \label{bgpfig}
		 * \end{center}
		 * \f}
		 * 
		 * The algorithm may be run on differenced data, or rather
		 * a difference plus a limiter preprocessing -- the civilisation grid.
		 */
		{
			DECUMA_INT32 m, k, i, j;

			/*----------------------------------------------------- */
			/* BEGIN CHUNK: diff data pCCharUser */
			/*  */
			/* The number of points is one less when we difference data. */
			/*  */
			m = nPointsUser - 1;
			pCurrUserGridPoint = pUserGridPoints + nBandwidth;
			for (i = 0; i < m; i++) {
				pCurrUserGridPoint[i] = dltGPGetDirection(pCurrUserGridPoint[i], pCurrUserGridPoint[i + 1]);
			}
			/* END CHUNK: diff data pCCharUser */
			/*----------------------------------------------------- */


			pCCharUser->dtwCached = 0; /* cache destroyed */

			/*----------------------------------------------------- */
			/* BEGIN CHUNK: diff data pCCharDb */
			/*  */
			/* For difference at a point between two components we have to use */
			/* the last point on the first variant of the first component since */
			/* we don't yet know which variant that will fit. */
			/*  */
			m = nPointsDb - 1;
			k = componentVariationFork[0];
			for (i = 0; i < m; i++) {
				/* component end and start */
				if (componentVariationFork[i + 1] != 0) {
					k = componentVariationFork[i] = componentVariationFork[i + 1];
					componentVariationFork[i + 1] = 0;
				}
				for (j = 0; j < k; j++) {
					if (componentVariationFork[i]) {
						/* component start */
						pDbGridPoints[j][i] = dltGPGetDirection(pDbGridPoints[0][i], pDbGridPoints[j][i + 1]);
					}
					else {
						/* inside component */
						pDbGridPoints[j][i] = dltGPGetDirection(pDbGridPoints[j][i], pDbGridPoints[j][i + 1]);
					}
				}
			}
			componentVariationFork[m] = STOPVALUE;
			/* END CHUNK: diff data pCCharDb */
			/*----------------------------------------------------- */


		}
		/* END CHUNK: diff all data */
		/*----------------------------------------------------- */


	}

	/*----------------------------------------------------- */
	/* BEGIN CHUNK: init dtwalgorithm */
	/*  */
	/*  */
	/* The algorithm is initialised so that it will behave properly */
	/* when it hits the first number in [[componentVariationFork]], which is always */
	/* non-zero. */
	/*  */

	nCurrUserGridPoint   = - pStartIndex[nBandwidth - 1 + ndiff];
	pCurrDbGridPoint     = pDbGridPoints[0];
	pCurrDbStrokeStart   = pDbStrokeStarts[0];
	pCurrUserGridPoint   = pUserGridPoints   + nBandwidth - pStartIndex[nBandwidth - 1 + ndiff];
	pCurrUserStrokeStart = pUserStrokeStarts + nBandwidth - pStartIndex[nBandwidth - 1 + ndiff];

	compVariantIndex = 0; /* Current component variant */
	compVariantCount = 1; /* Total number of component variants */
	/* END CHUNK: init dtwalgorithm */
	/*----------------------------------------------------- */



	/*----------------------------------------------------- */
	/* BEGIN CHUNK: run dtwalgorithm */
	/*  */
	/* The loop variable [[i]] is always the number of the present */
	/* point in the database character. It takes a jump backwards */
	/* inside the for-loop if there is a component with more than */
	/* one variant. This is OK according to ANSI even though */
	/* it is greasy programming style. */
	/*  */

	/*----------------------------------------------------- */
	/* BEGIN CHUNK: initialize pTotDist */
	/*  */
	/* The [[pTotDist]] vector is initialized. Note that [[j = k]] at */
	/* the start of the second for-loop. */
	/*  */
	{
		DECUMA_INT32 j, k;
		k = pStartIndex[nBandwidth - 1 + ndiff];
		/* TODO What happens if we use decumaMemset here? */
		for (j = 0; j < k; j++) {
			pTotDist[j] = LARGEDIST;
		}
		d = 0;
		for (; j < nBandwidth; j++) {
			d += CJK_GP_GET_SQ_DISTANCE(pCurrUserGridPoint[j], *pCurrDbGridPoint + delta);
			pTotDist[j] = d;
		}
	}
	/* END CHUNK: initialize pTotDist */
	/*----------------------------------------------------- */


	for (i = 0; ; i++) {
		if (componentVariationFork[i] != 0) {

			/*----------------------------------------------------- */
			/* BEGIN CHUNK: do action at segmentstart */
			/*  */
			/* At a segment start we either restart the present component */
			/* with a new variant or we close the present component and continue */
			/* to the next component if there are no more variants in the present */
			/* component. */
			/*  */
			compVariantIndex++;
			if (compVariantIndex == compVariantCount) {

				/*----------------------------------------------------- */
				/* BEGIN CHUNK: continue to next segment */
				/*  */
				/* Only if there is more than one variant in the present component we */
				/* need to restore the state and fix things up. */
				/* Only if there is more than one variant in the next component we */
				/* need to store present state for later variant runs. */
				/*  */
				/* OBS! For some cases the [[pTotDist]] vector is never initialized when coming into  */
				/* the write state backup chunk. No time to analyze when this happens and  */
				/* therefore a control variable is added. */
				/*  */
				if (compVariantCount > 1) {
					if (bDoubleBandwidth) { TOTDIST_MIN_DOUBLE(pTotDist, totdist_front); }
					else                  { TOTDIST_MIN(       pTotDist, totdist_front); }
				}
				if (componentVariationFork[i] == STOPVALUE) break;
				pCurrDbGridPoint   = &pDbGridPoints[0][i];
				pCurrDbStrokeStart = &pDbStrokeStarts[0][i];
				compVariantIndex  = 0;
				compVariantCount  = componentVariationFork[i];
				if (compVariantCount > 1) {

					/*----------------------------------------------------- */
					/* BEGIN CHUNK: write state backup */
					/*  */
					/* We remember the state vector [[pTotDist]] and the point index [[i]] */
					/* for the coming variant rerun. For efficiency and simplicity we */
					/* remember [[pCurrUserGridPoint]] too even though it could be computed given [[i]]. */
					/*  */
					if (bDoubleBandwidth) { TOTDIST_COPY_DOUBLE(totdist_back, pTotDist); }
					else                  { TOTDIST_COPY(       totdist_back, pTotDist); }

					i_back = i;
					nCurrUserGridPoint_back   = nCurrUserGridPoint;
					pCurrUserGridPoint_back   = pCurrUserGridPoint;
					pCurrUserStrokeStart_back = pCurrUserStrokeStart;
					/* END CHUNK: write state backup */
					/*----------------------------------------------------- */


				}
				/* END CHUNK: continue to next segment */
				/*----------------------------------------------------- */


			}
			else {

				/*----------------------------------------------------- */
				/* BEGIN CHUNK: restart this segment */
				/*  */
				/* When the first variant has been scanned the state is saved. */
				/* When a later variant is scanned the state is saved only */
				/* if it was better than the best previous saved state. */
				/* After this exercise the stage is set up for a replay using */
				/* the next variant of the present component. */
				/*  */
				if (compVariantIndex == 1) {
					if (bDoubleBandwidth) { TOTDIST_COPY_DOUBLE(totdist_front, pTotDist); }
					else                  { TOTDIST_COPY(       totdist_front, pTotDist); }
				}
				else {
					if (bDoubleBandwidth) { TOTDIST_MIN_DOUBLE(totdist_front, pTotDist); }
					else                  { TOTDIST_MIN(       totdist_front, pTotDist); }
				}

				if (bDoubleBandwidth) { TOTDIST_COPY_DOUBLE(pTotDist, totdist_back); }
				else                  { TOTDIST_COPY(       pTotDist, totdist_back); }

				i = i_back;
				pCurrDbGridPoint     = &pDbGridPoints[compVariantIndex][i];
				pCurrDbStrokeStart   = &pDbStrokeStarts[compVariantIndex][i];
				
				nCurrUserGridPoint   = nCurrUserGridPoint_back;
				pCurrUserGridPoint   = pCurrUserGridPoint_back;
				pCurrUserStrokeStart = pCurrUserStrokeStart_back;
				
				/* END CHUNK: restart this segment */
				/*----------------------------------------------------- */


			}
			/* END CHUNK: do action at segmentstart */
			/*----------------------------------------------------- */
		}

		if (i > 0) {
			if (i > 3 && compVariantCount == 1) {

				/*----------------------------------------------------- */
				/* BEGIN CHUNK: check for distanceLimit */
				/*  */
				/* For speed reasons we simply break the execution if the */
				/* minimum distance predicts that the final distance will be larger than */
				/* the limiting distance [[distanceLimit]] which the caller has given us. */
				/* The logic behind the somewhat complicated condition is the following. */
				/* At the end -- i e when index [[i]] is at the number of points [[nPointsDb]] */
				/* in the database character [[pCCharDb]] -- the limit is [[distanceLimit]]. At the */
				/* beginning, after the first point -- when index[[i]] is 1 -- the limit is */
				/* half of [[distanceLimit]]. In between linear interpolation is used. */
				/*  */
				CJK_DISTANCE mindist;
/*				decumaAssert(nBandwidth==9); */

				mindist = pTotDist[0];

				if (pTotDist[1] < mindist) mindist = pTotDist[1];
				if (pTotDist[2] < mindist) mindist = pTotDist[2];
				if (pTotDist[3] < mindist) mindist = pTotDist[3];
				if (pTotDist[4] < mindist) mindist = pTotDist[4];
				if (pTotDist[5] < mindist) mindist = pTotDist[5];
				if (pTotDist[6] < mindist) mindist = pTotDist[6];
				if (pTotDist[7] < mindist) mindist = pTotDist[7];
				if (pTotDist[8] < mindist) mindist = pTotDist[8];

				if (bDoubleBandwidth) {
					if (pTotDist[9]  < mindist) mindist = pTotDist[9];
					if (pTotDist[10] < mindist) mindist = pTotDist[10];
					if (pTotDist[11] < mindist) mindist = pTotDist[11];
					if (pTotDist[12] < mindist) mindist = pTotDist[12];
					if (pTotDist[13] < mindist) mindist = pTotDist[13];
					if (pTotDist[14] < mindist) mindist = pTotDist[14];
					if (pTotDist[15] < mindist) mindist = pTotDist[15];
					if (pTotDist[16] < mindist) mindist = pTotDist[16];
				}

				if (mindist >
					((distanceLimit * ((DECUMA_UINT32)(nPointsDb + i + 1))) /
					((DECUMA_UINT32) nPointsDb) >> 1)) {
						return LARGEDIST;
				}
				/* END CHUNK: check for distanceLimit */
				/*----------------------------------------------------- */


			}

			if (bUseGap) {
				updateTotalDistance_gap(pTotDist, 
					     pCurrUserGridPoint, 
						*pCurrDbGridPoint + delta, 
						 pCurrUserStrokeStart, 
						 pCurrDbStrokeStart,
						 bDoubleBandwidth,
						 nCurrUserGridPoint
#ifdef CJK_DTW_ALIGNMENT
						 ,
						 &pDtwMatrix[i * MAXPNTSPERSTR]
#endif
				);

			}
			else {
				updateTotalDistance_nogap(pTotDist, 
					       pCurrUserGridPoint, 
						  *pCurrDbGridPoint + delta, 
						   pGridPointDistTable,
						   bDoubleBandwidth,
						   nCurrUserGridPoint
#ifdef CJK_DTW_ALIGNMENT
						   ,
						   &pDtwMatrix[i * MAXPNTSPERSTR]
#endif
				);
			}

		}
#ifdef CJK_DTW_ALIGNMENT
		else {
			int j; 
			/* TODO Can we use decumaMemcpy here? */
			for (j = nCurrUserGridPoint < 0 ? - nCurrUserGridPoint : 0 ; j < nBandwidth ; j++) {
				pDtwMatrix[j + nCurrUserGridPoint] = pTotDist[j];
			}
		}
#endif

#if defined(DTW_DEBUG_OUPUT) && defined(CJK_DTW_ALIGNMENT)
		/* DEBUG print pTotDist vector */
		{
			int row;
			int col;

			printf("pTotDist (%2lu):", i);
			for (col = 0; col < i; col++) 
				printf("-----  ");
			
			
			for (row = 0; row < nBandwidth; row++) {
				printf("%5lu  ", pTotDist[row]);
			}
			printf("\n");
		}
#endif

		nCurrUserGridPoint++;
		pCurrDbGridPoint++;
		pCurrDbStrokeStart++;
		pCurrUserGridPoint++;
		pCurrUserStrokeStart++;
	}
	/* END CHUNK: run dtwalgorithm */
	/*----------------------------------------------------- */

#ifdef CJK_DTW_ALIGNMENT
	/* Caclualte alignment vector from distance matrix */
	{
		int row;
		int col;
		
		/* Start in the top right corner of the matrix */
		row = nPointsDb   - 1;
		col = nPointsUser - 1;

		pDtwAlignment[row] = col;

		/* Find a path to the bottom left corner */
		while (row > 0 || col > 0) {

			/* In ppDtwMatrix, we're in '*', looking to go to the smallest of a, b, and c: */
			/* */
			/*   ........ */
			/*   .....a*. */
			/*   .....bc. */
			/*   ........ */
			/*  */
			/* In case of equal values, b is preferred over a and c,  */
			/* and c is preferred over a. */

			int a = (col > 0           ) ? pDtwMatrix[(row    ) * MAXPNTSPERSTR + col - 1] : 65535;
			int b = (col > 0 && row > 0) ? pDtwMatrix[(row - 1) * MAXPNTSPERSTR + col - 1] : 65535;
			int c = (           row > 0) ? pDtwMatrix[(row - 1) * MAXPNTSPERSTR + col    ] : 65535;

			if (a < b) {

				if (a < c)
					pDtwAlignment[row]   = --col; /* a */
				else
					pDtwAlignment[--row] = col;   /* c */

			}
			else if (c < b) {

				if (a < c)
					pDtwAlignment[row]   = --col; /* a */
				else
					pDtwAlignment[--row] = col;   /* c */

			}
			else {
					pDtwAlignment[--row] = --col; /* b */
			}
		}

#if defined(DTW_DEBUG_OUPUT)
		/* Debug ouput */

		printf("matrix:\n");

		for (row = nPointsDb - 1; row >= 0; row--) {
			for (col = 0; col < nPointsUser; col++) {
				printf("%6lu ", pDtwMatrix[row * MAXPNTSPERSTR + col]);
			}
			printf("\n");
		}

		printf("match: ");

		for (row = 0; row < nPointsDb; row++) {
			/*fprintf(stderr, "%3ul ", pDtwAlignment[row]); */
		}

		printf("\n\n");
#endif
	}
#endif /* CJK_DTW_ALIGNMENT */

	if (bUseComp) {

		int nMiddleElem = nBandwidth / 2;

		/*----------------------------------------------------- */
		/* BEGIN CHUNK: pick mindist cell */
		/*  */
		/* If the user wants we pick the minimum cell and we may then leave some */
		/* points in [[pCCharUser]] unmatched. */
		/*  */
		CJK_DISTANCE mindist = MAX_CJK_DISTANCE;
		DECUMA_INT32 starti, stopi;
		stopi = pStopIndex[nBandwidth - 1 + ndiff];
		if (stopi >= nMiddleElem) {
			starti = stopi - nMiddleElem;
		}
		else {
			starti = 0;
		}
		for (i = starti; i <= stopi; i++) {
			if (pTotDist[i] < mindist) {
				mindist = pTotDist[i];
			}
		}
		/* END CHUNK: pick mindist cell */
		/*----------------------------------------------------- */


		return mindist;
	}

	return pTotDist[pStopIndex[nBandwidth - 1 + ndiff]];
}


CJK_DISTANCE dltCCharGetSOFDistance(CJK_COMPRESSED_CHAR * pCCharA, 
									CJK_COMPRESSED_CHAR * pCCharB, 
									DECUMA_INT32          delta, 
									CJK_DISTANCE          distanceLimit, 
									CJK_SESSION         * pSession)
{
	CJK_DISTANCE dist1 = cc_dist_sof1(pCCharA, pCCharB, delta, distanceLimit, pSession);
	CJK_DISTANCE dist2 = cc_dist_sof1(pCCharB, pCCharA, -delta, distanceLimit, pSession);

	if (dist2 < dist1) {
		return dist2;
	}

	return dist1;
}

CJK_DISTANCE dltCCharGetCoarseDTW(CJK_COMPRESSED_CHAR * pCCharUser, 
								  CJK_COMPRESSED_CHAR * pCCharDb, 
								  CJK_SESSION         * pSession)
{
	DECUMA_INT32 delta = dltCCharGetDelta(pCCharUser, pCCharDb);
	/* Call DTW with double bandwidth */
	CJK_DISTANCE dist = dltCCharGetDTWDistance(pCCharUser, pCCharDb, delta, MAX_CJK_DISTANCE, 0, 0, 0, 1, pSession);

	/* Normalize by number of point calculations */
	if (dist < MAX_CJK_DISTANCE) {
		dist = (100 * dist) / (CJK_CCHAR_NPOINTS(pCCharUser) + CJK_CCHAR_NPOINTS(pCCharDb));
	}
	else {
		dist = LARGEDIST;
	}
	return dist;
}


CJK_DISTANCE dltCCharGetDistanceFinal(CJK_COMPRESSED_CHAR * pCCharA,
									  CJK_COMPRESSED_CHAR * pCCharB, 
									  CJK_BOOLEAN           bUseFreq, 
									  CJK_BOOLEAN           bDoubleBandwidth, 
									  CJK_SESSION         * pSession)
{
	CJK_DISTANCE d;
	CJK_DISTANCE dist = LARGEDIST;
	CJK_DISTANCE punish;
	DECUMA_INT32 delta;
	DECUMA_INT32 freq;

	decumaAssert(!CJK_CCHAR_IS_DENSE(pCCharA) == !CJK_CCHAR_IS_DENSE(pCCharB));

	delta = dltCCharGetDelta(pCCharA, pCCharB);

	if (!CJK_CCHAR_IS_DENSE(pCCharA)) {
		punish = dltCCharGetPointDifferencePunishDistance(pCCharA, pCCharB); /* + dltCCharCompStartPunish(pCCharA, pCCharB, pSession); */
		
		/* We only connect strokes for unihan, not latin, kana and others */
		/* unless the user wrote at least 7 strokes. */
		if (!pCCharB->index || (decumaIsHan(cjkDbUnicode(pCCharB->index, pSession)) || decumaIsHangul(cjkDbUnicode(pCCharB->index, pSession)) || CJK_CCHAR_NSTROKES(pCCharA) > 6)) {
			dist = dltCCharGetDTWDistance(pCCharA, pCCharB, delta, LARGEDIST, 0, 1, 0, bDoubleBandwidth, pSession);
			dist += punish;
		}
		
		if (CJK_CCHAR_NSTROKES(pCCharA) == CJK_CCHAR_NSTROKES(pCCharB)) {
			if ( CJK_CCHAR_NSTROKES(pCCharA) <= 8) {
				d = dltCCharGetSOFDistance(pCCharA, pCCharB, delta, LARGEDIST, pSession) + punish + 20;
				if (d < dist) dist = d;
			}
			
			d = dltCCharGetRawDistance(pCCharA, pCCharB, delta, pSession) + punish; /* + 5; */
			
			if (d < dist) dist = d;
		}
	
		dist += dltCCharGetDTWDistance(pCCharA, pCCharB, delta, LARGEDIST, 1, 1, 0, bDoubleBandwidth, pSession) / 2;
	}
	else if (CJK_CCHAR_NSTROKES(pCCharA) == CJK_CCHAR_NSTROKES(pCCharB)) {
		dist = dltCCharGetRawDistance(pCCharA, pCCharB, delta, pSession);
		
		/* Make sure not to run this on non-comparable really small input */
		
		if ((pCCharA->xmax - pCCharA->xmin)* 10 <= pSession->boxwidth &&
			(pCCharA->ymax - pCCharA->ymin)* 15 <= pSession->boxheight) {
				/* What to do for really small input? */
		}
		else {
			dist += dltCCharGetDTWDistance(pCCharA, pCCharB, delta, LARGEDIST, 1, 1, 0, bDoubleBandwidth, pSession);
		}
	}
	if ((bUseFreq) &&  (pCCharB->index < DYNAMIC_DB_STARTINDEX)) {
		freq = DLTDB_GET_FREQUENCY(&pSession->db, pCCharB->index);
		dist = (75 * dist + dist * (15 - freq)) / 75;
	}

	if (dist > LARGEDIST) {
		dist = LARGEDIST;
	}
	return dist;
}


CJK_DISTANCE dltCCharGetDistanceNoDB(CJK_COMPRESSED_CHAR * pCCharA,
									 CJK_COMPRESSED_CHAR * pCCharB,
									 DECUMA_UINT16		   uc,
									 CJK_SESSION         * pSession)
{
	CJK_DISTANCE d;
	CJK_DISTANCE dist = LARGEDIST;
	CJK_DISTANCE punish;
	DECUMA_INT32 delta;

	decumaAssert(CJK_CCHAR_IS_DENSE(pCCharA) == CJK_CCHAR_IS_DENSE(pCCharB));

	delta = dltCCharGetDelta(pCCharA, pCCharB);

	if (!CJK_CCHAR_IS_DENSE(pCCharA)) {
		punish = dltCCharGetPointDifferencePunishDistance(pCCharA, pCCharB);
		
		/* We only connect strokes for unihan, not latin, kana and others */
		/* unless the user wrote at least 7 strokes. */
		if (decumaIsHan(uc) || decumaIsHangul(uc) || CJK_CCHAR_NSTROKES(pCCharA) > 6) {
			dist = dltCCharGetDTWDistance(pCCharA, pCCharB, delta, LARGEDIST, 0, 1, 0, 0, pSession);
			dist += punish;
		}
		
		if (CJK_CCHAR_NSTROKES(pCCharA) == CJK_CCHAR_NSTROKES(pCCharB)) {
			if ( CJK_CCHAR_NSTROKES(pCCharA) <= 8) {
				d = dltCCharGetSOFDistance(pCCharA, pCCharB, delta, LARGEDIST, pSession) + punish + 20;
				if (d < dist) dist = d;
			}
			
			d = dltCCharGetRawDistance(pCCharA, pCCharB, delta, pSession) + punish; /* + 5; */
			
			if (d < dist) dist = d;
		}
	
		dist += dltCCharGetDTWDistance(pCCharA, pCCharB, delta, LARGEDIST, 1, 1, 0, 0, pSession) / 2;
	}
	else if (CJK_CCHAR_NSTROKES(pCCharA) == CJK_CCHAR_NSTROKES(pCCharB)) {
		dist = dltCCharGetRawDistance(pCCharA, pCCharB, delta, pSession);
		
		/* Make sure not to run this on non-comparable really small input */
		
		if ((pCCharA->xmax - pCCharA->xmin)* 10 <= pSession->boxwidth &&
			(pCCharA->ymax - pCCharA->ymin)* 15 <= pSession->boxheight) {
				/* What to do for really small input? */
		}
		else {
			dist += 1 * dltCCharGetDTWDistance(pCCharA, pCCharB, delta, LARGEDIST, 1, 1, 0, 0, pSession);
		}
	}
	if (dist > LARGEDIST) {
		dist = LARGEDIST;
	}
	return dist;
}

CJK_DISTANCE dltCCharGetPointDifferencePunishDistance(CJK_COMPRESSED_CHAR * pCCharA, 
													  CJK_COMPRESSED_CHAR * pCCharB)
{
	DECUMA_INT32 nPointsA = CJK_CCHAR_NPOINTS(pCCharA);
	DECUMA_INT32 nPointsB = CJK_CCHAR_NPOINTS(pCCharB);
	CJK_DISTANCE punish;

	punish = (CJK_DISTANCE) ((nPointsA >= nPointsB)? 8*((nPointsA - nPointsB)): 0);
	/*punish = (nPointsA >= nPointsB)? ((nPointsA - nPointsB + 1)*(nPointsA - nPointsB)): 2*(nPointsB - nPointsA - 1); */
	/* Oldest: punish = (na >= nb)? ((na - nb + 4)*(na - nb)): 2*(nb - na - 1); */
	if (CJK_CCHAR_NSTROKES(pCCharA) > CJK_CCHAR_NSTROKES(pCCharB)) {
		punish += 20;
	}
	return punish;
}



DECUMA_INT32 dltCCharCompStartPunish(CJK_COMPRESSED_CHAR * pCCharA, 
									 CJK_COMPRESSED_CHAR * pCCharB, 
									 CJK_SESSION         * pSession)
{
	if (DLTDB_IS_SIMP(pSession->db)) /* Simplified DB*/
	{
		if (pSession->db.categoryMask & CJK_GB2312) {
			DECUMA_INT32 comp = dltCCharGetStartComponent(pCCharB, pSession);
			if (comp == -1) {
				return 0;
			} else if (comp == pSession->db.knownComponents.speakingS_l ||
				comp == pSession->db.knownComponents.speakingS_l2 ||
				comp == pSession->db.knownComponents.speakingS_l3) {
					if (pCCharA->bIsNotSpeaking) {
						return 100;
					}
			}
			else if (comp == pSession->db.knownComponents.threedrops_l ||
				comp == pSession->db.knownComponents.threedrops_l2 ||
				comp == pSession->db.knownComponents.threedrops_l3 ||
				comp == pSession->db.knownComponents.threedrops_ld ||
				comp == pSession->db.knownComponents.threedrops_i  ||
				comp == pSession->db.knownComponents.threedrops_ld2) {
					if (pCCharA->bIsNotThreeDrops) {
						return 20;
					}
			}
			else if (comp == pSession->db.knownComponents.earth_l) {
				if (pCCharA->bIsNotSoil) {
					return 20;
				}
			}
		}
	}
	return 0;
}


DECUMA_INT32 dltCCharGetStartComponent(CJK_COMPRESSED_CHAR * pCChar, 
									   CJK_SESSION         * pSession) 
{
	const DECUMA_UINT8 * p;

	decumaAssert(pCChar);

	p = pCChar->pCCData + 1;

	if (*p <= MAXPNTSPERSTR) {return -1;}

	if (*p < pSession->db.firsttwobyteindex) {
		return *p;
	}

	return DLTDB_TWOBYTE_COMPONENT_INDEX(*p, *(p + 1));
}


DECUMA_INT32 dltCCharGetIntersectionCount(CJK_COMPRESSED_CHAR * pCChar, 
										  CJK_SESSION         * pSession)
{
	CJK_GRIDPOINT_ITERATOR gi;
	dltGPIterInit(&gi, pCChar, pSession);
	return dltGPIterGetIntersectionCount(&gi, BIGNUMBER, pSession);
}

DECUMA_STATUS dltCCharSetFeatureBits(CJK_COMPRESSED_CHAR * const pCChar,  
							CJK_SESSION         * const pSession)
{

	DECUMA_STATUS status;
	COARSE_SETTINGS settings; 

	if (dltCCCompressGetNbrPoints(pCChar) == 0) return decumaZeroPoints;

	/* Initialize here (since ARM compiler did not like = { style init for this structure) */
	settings.ymax = 15; settings.ymin = 0; settings.xmax = 15; settings.xmin = 0;

	/* Some initialization */
	pSession->nLeftRightSplitIndex = -1;
	pSession->nTopDownSplitIndex = -1;
	pSession->nLeftRightSplitScore = 0;
	pSession->nTopDownSplitScore = 0;
	
	/* TODO Use different feature set for dense and sparse templates */
	/*      Right now, we use the coarse features for both. */
	/* 	if (CJK_CCHAR_IS_DENSE(pCChar)) { */
	/*	nFeatures = cjkGetCoarseFeatures(pSession->coarseInputFeatures, COARSE_NBR_FEATURES, */
	/*		pCChar, pSession, &settings); */
	/*} */
	/*else { */
	status = cjkGetCoarseFeatures(pSession->coarseInputFeatures, COARSE_NBR_FEATURES,
			pCChar, pSession, &settings);
	/*} */
	if (status != decumaNoError)
		return status;

	dltCoarseSetOldDecumaFeatures(pCChar, pSession);

	pCChar->pFeatures    = pSession->coarseInputFeatures;
	pCChar->bFeaturesSet = 1;

	return status;
}

void dltCCharPrecompute(CJK_COMPRESSED_CHAR * pCChar, CJK_SESSION * pSession) 
{
	DECUMA_INT32 nPoints  = 0;
	DECUMA_INT32 nStrokes = 0;
	DECUMA_INT32 i;
	DECUMA_INT32 xSum = 0, ySum = 0;
	CJK_STROKE   stroke;
	const CJK_GRIDPOINT * pPoint;

	for(dltCCharGetFirstStroke(pCChar, &stroke, pSession); 
		CJK_STROKE_EXISTS(&stroke); 
		cjkStrokeNext(&stroke, pSession)) 
	{
		/* TODO More obvious implementation:
		 *
		 *      int strokePoints = CJK_STROKE_NPOINTS(&stroke);
		 *      nPoints += i;
		 *      nStrokes += 1;
		 *      pPoint = CJK_STROKE_FIRST_POINT(&stroke); 
		 *      for ( i = 0; i < strokePoints; i++ )
		 *      {
		 *          xSum += CJK_GB_GET_X(pPoint[i]);
		 *          ySum += CJK_GB_GET_Y(pPoint[i]);
		 *      }
		 */

		nPoints += i = CJK_STROKE_NPOINTS(&stroke);
		nStrokes++;
		pPoint = CJK_STROKE_FIRST_POINT(&stroke);
		for (; i; i--) {
			xSum += CJK_GP_GET_X(*pPoint);
			ySum += CJK_GP_GET_Y(*pPoint);
			++pPoint;
		}
	}
	pCChar->nPoints = nPoints;
	pCChar->nStrokes = nStrokes;
	pCChar->dtwCached = 0;
	pCChar->xSum = xSum;
	pCChar->ySum = ySum;
}

CJK_BOOLEAN dltCCharHasAllowedForm(CJK_COMPRESSED_CHAR * a, 
								   CJK_SESSION         * pSession)
{
	/* Upper 8 bits of DLTDB_ATTRIBUTE are reserved for typemask */

	char type = (char)(a->attrib >> 8);

	/* Shift categorymask to 8 bit format and compare to typemask */
	if ( (type & (CJK_TRADSIMPDUAL >> 20)) ) {
		if (pSession->sessionCategories & CJK_TRADSIMPDUAL) {
			return 1;
		}
		else {
			return 0;
		}
	}
	if ( (type & (CJK_POPULARFORM >> 20)) ) {
		if (pSession->sessionCategories & CJK_POPULARFORM) {
			return 1;
		}
		else {
			return 0;
		}
	}

	return 1;
}


void dltCCharGetFirstStroke(CJK_COMPRESSED_CHAR * pCChar, 
							CJK_STROKE          * pStroke, 
							CJK_SESSION * pSession)
{
	pStroke->stdata = pCChar->pCCData + 1;
	pStroke->end = pCChar->pCCData + CJK_CCHAR_NBYTES(pCChar);
	pStroke->stdata_backup = 0;
	pStroke->end_backup = 0;

	/* Check for jump to component
	 * 
	 * This chunk is used twice in the program.
	 * The insanely strange [[volatile]] is for the broken Green Hill compiler
	 * that screws optimization up without this directive.
	*/ 
	decumaAssert(pSession || (*(pStroke->stdata) <= MAXPNTSPERSTR));
	if (pSession && (*(pStroke->stdata) > MAXPNTSPERSTR)) {
		volatile DECUMA_INT32 k;
		CJK_COMPRESSED_CHAR_DATA * ccdata;
		pStroke->end_backup = pStroke->end;
		if (*(pStroke->stdata) < pSession->db.firsttwobyteindex) {
			k = *(pStroke->stdata) - (MAXPNTSPERSTR + 1);
			pStroke->stdata_backup = pStroke->stdata + 1;
		}
		else {
			k = ((*(pStroke->stdata) - pSession->db.firsttwobyteindex) << 8)
				+ *(pStroke->stdata + 1)
				+ pSession->db.firsttwobyteindex - (MAXPNTSPERSTR + 1);
			pStroke->stdata_backup = pStroke->stdata + 2;
		}
		ccdata = DLTDB_GET_COMPONENT_DATA(&pSession->db, k);
		pStroke->stdata = ccdata + 1;
		pStroke->end = ccdata + *ccdata;
	}
}



static DECUMA_STATUS cc_compress_dostuff(CJK_COMPRESSED_CHAR * a, 
						 DECUMA_ARC          * pstroke, 
						 DECUMA_INT32    const nStrokes,
						 DECUMA_INT32          style, 
						 CJK_ARC_SESSION     * pCjkArcSession)
{
	CJK_CC_COMPRESS_SCRATCHPAD * pScratchpad = cjkArcSessionGetScratchpad(pCjkArcSession);
	CJK_COMPRESSED_CHAR_DATA * cchardata = cjkArcSessionGetCCharData(pCjkArcSession);
	CJK_COMPRESSED_CHAR_DATA * internal_cchardata = pScratchpad->PT_BUFFER.cchar_data;
	short * xydata = pScratchpad->xydata;
	short const * xydata_start = pScratchpad->xydata; /* Pointer to start of pSession->scratchpad.xydata */
	short const * xydata_end; /* Pointer to the end of written data in pSession->scratchpad.xydata */
	DECUMA_INT32 n, j, i, x, y;
	DECUMA_INT32 mx, my, vx, vy, sizex, sizey;
	
	int nrawpoints;
	int nRawStrokes = 0;
	DECUMA_POINT const * pRawPoints;
	DECUMA_STATUS status = decumaNoError;

	decumaAssert(pCjkArcSession);

	/* Both can not be NULL */
	if ( pstroke == NULL )
		return decumaNullArcPointer;

	nRawStrokes = nStrokes;

	if (nRawStrokes == 0) {
		status = decumaCurveZeroArcs;
		goto LABEL_cc_compress_dostuff_return_null;
	}

	/* The maximum and minimum x and y values for the */
	/* original input field are filled in as well. */
	/* They will for example be used for punctuation recognition in [[cjkDbLookup]]. */
	/*  */
	a->xmin = a->ymin = 0x7FFF;
	a->xmax = a->ymax = -0x7FFF;
	nrawpoints = 0;
	for (i = 0; i < nRawStrokes; i++) {
		int nStrokePts;
		pRawPoints = (const DECUMA_POINT*) pstroke[i].pPoints;
		nStrokePts = pstroke[i].nPoints;
		
		if ( pRawPoints == NULL )
		{
			status = decumaNullPointPointer;
			goto LABEL_cc_compress_dostuff_return_null;
		}

		if ( nStrokePts <= 0 )
		{
			status = decumaArcZeroPoints;
			goto LABEL_cc_compress_dostuff_return_null;
		}

		nrawpoints += nStrokePts;
		for (j = 0; j < nStrokePts; j++) {
			if (pRawPoints[j].x < a->xmin) a->xmin = pRawPoints[j].x;
			if (pRawPoints[j].x > a->xmax) a->xmax = pRawPoints[j].x;
			if (pRawPoints[j].y < a->ymin) a->ymin = pRawPoints[j].y;
			if (pRawPoints[j].y > a->ymax) a->ymax = pRawPoints[j].y;
		}
	}

	/*  */
	/* We loop through the array of strokes and put all coordinate data into */
	/* one single array. */
	/*  */
	{
		short * q = xydata;

		for (i = 0; i < nRawStrokes; i++) {
			int nStrokePts;
			DECUMA_POINT * gpsbuf;

			pRawPoints = (const DECUMA_POINT*) pstroke[i].pPoints;
			nStrokePts = pstroke[i].nPoints;

			if ( style & (CC_SPARSE | CC_SPARSE_KO)) 
			{
				/* If [[n]] is two then two points have been written. Now we do a fix so that
				 * the {\em first} point is deleted if the ink direction is between
				 * west and north east.
				 */ 
				while (nStrokePts >= 2 &&
					pRawPoints[1].y <= pRawPoints[0].y &&
					2 * (pRawPoints[1].x - pRawPoints[0].x) <=
					pRawPoints[0].y - pRawPoints[1].y) {
						pRawPoints += 1;
						nStrokePts -= 1;
				}
			}

			if ((style & (CC_SPARSE | CC_SPARSE_KO)) && nStrokePts > 2) {

				gpsbuf = pScratchpad->gpsbuf;

				n = cjkMathFindPoints(gpsbuf, pRawPoints, nStrokePts, pScratchpad->mt_walkfpnt_stack, CC_MAXCCHARSIZE,
						pScratchpad->PT_BUFFER.mt_fintpts_buffer, CC_MAXCCHARSIZE, style & CC_SPARSE_KO);

				if (n > MAXPNTSPERSTR) {
					status = decumaInputBufferFull;
					goto LABEL_cc_compress_dostuff_return_null;
				} else if (n == 0) {
					status = decumaZeroPoints;
					goto LABEL_cc_compress_dostuff_return_null;
				}

			} else {
				decumaAssert(style & CC_DENSE || nStrokePts <= 2);

				/* For dense sampled strokes, or very short spares-sampled strokes, use raw points directly */
				gpsbuf = (DECUMA_POINT*) pRawPoints;
				n = nStrokePts;
			}
			/* Chunk for appending to xydata */
			{
				DECUMA_INT32 stepsize = 1;
				*q = (DECUMA_INT16) n;

				/* Special treatment for extremely dense characters */
				if (style & CC_DENSE) {
					if (2 * nrawpoints > CC_MAXCCHARSIZE - 10) {
						stepsize = 2;
						*q = (DECUMA_INT16) (n + 1) / 2;
					}
					if (nrawpoints > CC_MAXCCHARSIZE - 10) {
						stepsize = 4;
						*q = (DECUMA_INT16) (n + 3) / 4;
					}
				}

				q++;
				for (j = 0; j < n; j += stepsize) {

					/* There must be room for two more coordinates and one finishing zero.
					*/ 
					if (q - xydata >= CC_MAX_PT_BUFFER_SZ - 3) {
						status = decumaInputBufferFull;
						goto LABEL_cc_compress_dostuff_return_null;
					}

					*q++ = gpsbuf[j].x;
					*q++ = gpsbuf[j].y;
				}
			}
		}
		*q = 0; /* Set final zero after we've added everything */
		xydata_end = q;
	}

	/* We compute the mean in $x$ and $y$ direction. Here the local variable
	 * [[n]] is reused for number of points in character -- as opposed to a previous
	 * loop where it was number of points in a stroke. Note the cool
	 * round off correction.
	*/ 
	{ 
		short const * q = xydata_start;
		mx = my = n = 0;
	
		while ( q < xydata_end ) {
			for (j = *q++; j > 0; j--) {
				mx += *q++;
				my += *q++;
				n++;
			}
		}
		mx = (2 * mx + n) / (2 * n);
		my = (2 * my + n) / (2 * n);
	}

	/* Center origin 
	* We subtract the mean values from all coordinates.
	*/ 
	{
		short * q = xydata;
		while (q < xydata_end) {
			for (j = *q++; j > 0; j--) {
				*q++ -= ((DECUMA_INT16) mx);
				*q++ -= ((DECUMA_INT16) my);
			}
		}
	}

	/** Variance calculation :
	* We must make an overflow analysis for the next chunk. There are well below
	 100 points in the most complicated characters, certainly never
	 as much as 500 points.
	 Those terms are squares of coordinates. The requirement on the input
	 coordinates
	 is that they be $-2048 \leq x,y < 2048$. After centering, this might no
	 longer be
	 true but the worst case is actually that half of the points is at -2048
	 and the
	 other half is at 2047. If a number is at most
	 2048 then the sum of squares fits in a 32 bit signed number (the number is
	 actually unsigned so we have a one bit marginal).
	*/ 
	{
		short const * q = xydata_start;

		vx = vy = 0;
		while (q < xydata_end) {
			for (j = *q++; j > 0; j--) {
				vx += *q * *q;
				q++;
				vy += *q * *q;
				q++;
			}
		}
		vx = vx / n;
		vy = vy / n;
	}

	/**  Compute size:
	 The factor 7/2=3.5 below is the normalizing factor.
	 The standard deviation is normalized
	 separately in the $x$- and $y$-directions to 3.5 apart from
	 some inevitable roundoff errors due to integer arithmetic. This number
	 was found slightly better than 3.0 or 4.0 in a testrun.
	 
	 The variable [[size]] must be at least 1 in order to avoid dividing by
	 zero later. Furthermore, an aspect ratio larger than 2 seems strange.
	 
	 In order to avoid integer arithmetic we have to scale with a
	 factor before division.
	 */

#define AFACTOR 256
	if (vx < 30000) {
		sizex = (2 * isqrt(AFACTOR * AFACTOR * vx)) / 7;
	}
	else if (vx < 0x07FFFFFF) {
		sizex = (2 * AFACTOR * isqrt(vx)) / 7;
	}
	else {
		/* DECUMA_WARN("vx overflow"); */
		status = decumaInvalidCoordinates; 
		goto LABEL_cc_compress_dostuff_return_null;
	}

	if (vy < 30000) {
		sizey = (2 * isqrt(AFACTOR * AFACTOR * vy)) / 7;
	}
	else if (vy < 0x07FFFFFF) {
		sizey = (2 * AFACTOR * isqrt(vy)) / 7;
	}
	else {
		/* DECUMA_WARN("vy overflow"); */
		status = decumaInvalidCoordinates; 
		goto LABEL_cc_compress_dostuff_return_null;
	}

	if (sizex < sizey / 2) sizex = sizey / 2;
	if (sizey < sizex / 2) sizey = sizex / 2;
	if (!(style & (CC_SPARSE | CC_SPARSE_KO))) {
		sizex = sizey = MAX(sizex, sizey);
	}
	if (sizex == 0) sizex = 1;
	if (sizey == 0) sizey = 1;

	/* Put coordinates into grid
	 * The fiddling with the number 8 below is for moving the origin to the point
	 * $(7.5, 7.5)$. Remember that ANSI says that integer division for positive
	 * numbers are rounded off downwards. The first byte in the character is
	 * the total number of bytes. The byte {\em after} the CJK_COMPRESSED_CHAR_DATA is set to zero
	 * in case someone tries the [[dltCCharGetNext]] method.
	 
	 * Before writing a gridpoint we make sure there is room for
	 * at least two bytes more -- the gridpont and the terminating zero.
	 * Last of all we put in the terminating zero and the first byte, which is
	 * a count of the total number of bytes in the character.
	*/ 

	{
		CJK_GRIDPOINT * pg;
		short const * q;

		q = xydata_start;
		pg = internal_cchardata + 1;

		while (q < xydata_end) {
			*pg++ = (DECUMA_UINT8) *q; /* write the number of points */
			for (j = *q++; j > 0; j--) {
				decumaAssert(pg - internal_cchardata < CC_MAX_PT_BUFFER_SZ - 3);
				if ((pg - internal_cchardata) >= CC_MAX_PT_BUFFER_SZ - 3) {
					status = decumaInputBufferFull;
					goto LABEL_cc_compress_dostuff_return_null;
				}
				x = (AFACTOR * (*q++) + 8 * sizex) / sizex;
				if (x < 0) x = 0;
				if (x > 15) x = 15;
				y = (AFACTOR * (*q++) + 8 * sizey) / sizey;
				if (y < 0) y = 0;
				if (y > 15) y = 15;
				*pg++ = (DECUMA_UINT8) (x | (y << 4));
			}
			decumaAssert( pg - internal_cchardata < CC_MAX_PT_BUFFER_SZ ); /* If not, we have overflowed the internal buffer, corrupting the following fields -- THIS IS VERY BAD :) */
		}
		
		decumaAssert( pg - internal_cchardata < CC_MAX_PT_BUFFER_SZ ); /* If not, we have overflowed the internal buffer, corrupting the following fields -- THIS IS VERY BAD :) */
		decumaAssert((DECUMA_UINT32) (pg - internal_cchardata) < CC_MAX_PT_BUFFER_SZ && (DECUMA_UINT32) (pg - internal_cchardata) >= 0); /* If not, we wrongly truncate the number of strokes */

		*pg = 0;
		internal_cchardata[0] = (DECUMA_UINT8)(pg - internal_cchardata);
	}

	/* Remove some points for dense sampling */
	if (!(style & (CC_SPARSE | CC_SPARSE_KO))) {
		n = cjkMathRemovePoints(&internal_cchardata[0], n, nRawStrokes);
		decumaAssert(n <= MAXPNTSPERSTR);
		decumaAssert(n);
		if (n == 0 || n > MAXPNTSPERSTR) {
			status = decumaInvalidCoordinates;
			goto LABEL_cc_compress_dostuff_return_null;
		}
	}
	else {

		/* Remove some sparse sampled points */
		{
			DECUMA_UINT8 *p = internal_cchardata + 1;
			DECUMA_UINT8 *q = p, *pn;
			DECUMA_INT32 n;

			while (*p != 0) {
				n = *p++;
				pn = q++;
				for (i = 0; i < n; i++) {

					if (i == n - 1) {
						goto LABEL_cc_compress_dostuff_writepoint; /* It is the last point */
					}
					if (*p == *(p + 1)) {
						goto LABEL_cc_compress_dostuff_dontwritepoint; /* It equals the next point */
					}
					if (q - pn == 1) {
						goto LABEL_cc_compress_dostuff_writepoint; /* It is the first point */
					}
					{
						DECUMA_INT32 x1 = CJK_GP_GET_X(*(q - 1));   /* Yes, q! */
						DECUMA_INT32 x2 = CJK_GP_GET_X(*p);
						DECUMA_INT32 x3 = CJK_GP_GET_X(*(p + 1));
						DECUMA_INT32 y1 = CJK_GP_GET_Y(*(q - 1));   /* Yes, q! */
						DECUMA_INT32 y2 = CJK_GP_GET_Y(*p);
						DECUMA_INT32 y3 = CJK_GP_GET_Y(*(p + 1));
						DECUMA_INT32 ux = x3 - x1;
						DECUMA_INT32 uy = y3 - y1;
						DECUMA_INT32 vx = x2 - x1;
						DECUMA_INT32 vy = y2 - y1;
						DECUMA_INT32 d2;
						if (ux == 0 && uy == 0) { /* neighbour points are equal! */
							if (ABS(vx) <= 1 && ABS(vy) <= 1) {
								p++;
								i++; /* Skip both this and next even if next is last! */
								goto LABEL_cc_compress_dostuff_dontwritepoint;
							}
							goto LABEL_cc_compress_dostuff_writepoint;
						}
						if ((ux - vx) * vx + (uy - vy) * vy < 0) {
							goto LABEL_cc_compress_dostuff_writepoint; /* sharp angle */
						}
						d2 = vx * uy - vy * ux;
						decumaAssert(ux * ux + uy * uy != 0); /* Can't happen, see above. */
						d2 = (100 * d2 * d2) / (ux * ux + uy * uy);
						if (d2 < 25) {
							goto LABEL_cc_compress_dostuff_dontwritepoint;
						}
					}

LABEL_cc_compress_dostuff_writepoint:
					*q++ = *p;
LABEL_cc_compress_dostuff_dontwritepoint:
					p++;
				}
				decumaAssert((DECUMA_UINT32)(q - pn - 1) >= 0 && (DECUMA_UINT32)(q - pn - 1) < CC_MAX_PT_BUFFER_SZ);
				*pn = (DECUMA_UINT8)(q - pn - 1);

				/* Limit the last upward trace (improbable to be deliberate input) */
				if (*pn > 2) {
					DECUMA_INT32 dy = CJK_GP_GET_Y(*(q - 2)) - CJK_GP_GET_Y(*(q - 1));
					DECUMA_INT32 dx = CJK_GP_GET_X(*(q - 1)) - CJK_GP_GET_X(*(q - 2));
					if (dy > 3 && ABS(dx) < dy) {
						y = CJK_GP_GET_Y(*(q - 2)) - 3;
						x = CJK_GP_GET_X(*(q - 2)) + (dx * 4) / dy;
						*(q - 1) = (DECUMA_UINT8) ((y << 4) | x);
					}

				}
			}
			*q = 0;
			decumaAssert((DECUMA_UINT32)(q - internal_cchardata) < CC_MAX_PT_BUFFER_SZ && (DECUMA_UINT32)(q - internal_cchardata) >= 0);
			internal_cchardata[0] = (DECUMA_UINT8) (q - internal_cchardata);
		}
	}

	/* Copy the internal buffer (with larger size) to the pCChar buffer */
	if (internal_cchardata[0] >= CC_MAXCCHARSIZE) {
		status = decumaInputBufferFull;
		goto LABEL_cc_compress_dostuff_return_null;
	}
	decumaMemcpy(&cchardata[0], &internal_cchardata[0], CC_MAXCCHARSIZE);

	/** Fill in the compressed character and return 
	 * Note that [[dltCCharSetFeatureBits]] is {\em not} called here; only
	 * [[dltCCharPrecompute]]. The reason is that [[arcs2arx]] should work without
	 * a database linked to it, and [[dltCCharSetFeatureBits]] unfortunately needs one.
	 */ 
	if (style & CC_DENSE) {
		a->attrib = AM_ISDENSE;
	}
	else {
		a->attrib = 0;
	}
	a->pCCData = cchardata;
	a->nStrokes = nRawStrokes;
	a->pOriginalStrokes = pstroke;

	a->index = 0;
	dltCCharPrecompute(a, 0 /* Calling without session is OK! */);
	if (a->nPoints > MAXPNTSPERCHAR) {
		status = decumaInputBufferFull;
		goto LABEL_cc_compress_dostuff_return_null;
	}
	a->bFeaturesSet = 0;

	return decumaNoError;

LABEL_cc_compress_dostuff_return_null:

	CJK_CC_SET_NULLCHAR(a);
	decumaAssert(status != decumaNoError); /* Never return null and pretend it's OK */
	return status;
}

DECUMA_HWR_PRIVATE int cjkCompressedCharacterCanBeSparse(CJK_SAMPLING_RULE samplingRule, int nSparsePoints)
{
	if (samplingRule == ChineseNonHanSampling && nSparsePoints <= LENGTHLIMIT) {
		return 0;
	}
	else if (samplingRule == NonHanSampling) {
		return 0;
	}
	return 1;
}

DECUMA_STATUS dltCCharCompressChar(CJK_COMPRESSED_CHAR * pCChar, 
								     const DECUMA_CURVE        * pCurve, 
								           CJK_ARC_SESSION         * pCjkArcSession)
{

	CJK_SAMPLING_RULE samplingRule = cjkArcSessionGetSamplingRule(pCjkArcSession);
	if (samplingRule == NonHanSampling) 
	{
		return cc_compress_dostuff(pCChar, pCurve->pArcs,pCurve->nArcs, CC_DENSE, pCjkArcSession);
	}
	else if (samplingRule == ChineseNonHanSampling) {
		/* This amounts to the CC_FREE style */
		DECUMA_STATUS status = cc_compress_dostuff(pCChar, pCurve->pArcs,pCurve->nArcs, CC_SPARSE, pCjkArcSession);

		if (status == decumaNoError && !cjkCompressedCharacterCanBeSparse(cjkArcSessionGetSamplingRule(pCjkArcSession), pCChar->nPoints)) {
			status = cc_compress_dostuff(pCChar, pCurve->pArcs,pCurve->nArcs, CC_DENSE, pCjkArcSession);
			pCChar->attrib |= AM_ISDENSE;
		}
		return status;
	}
	else if (samplingRule == HangulSampling) {
		return cc_compress_dostuff(pCChar, pCurve->pArcs,pCurve->nArcs, CC_SPARSE_KO, pCjkArcSession);
	}
	else { /* Default sampling for more than 2 strokes - HanSampling */
		return cc_compress_dostuff(pCChar, pCurve->pArcs,pCurve->nArcs, CC_SPARSE, pCjkArcSession);
	}
}

CJK_GRIDPOINT dltGPGetDirection(CJK_GRIDPOINT last_gp, CJK_GRIDPOINT next_gp)
{

	DECUMA_INT32 x1, y1, x2, y2;
	CJK_GRIDPOINT temp_gp;

	x1 = CJK_GP_GET_X(last_gp);
	y1 = CJK_GP_GET_Y(last_gp);
	x2 = CJK_GP_GET_X(next_gp);
	y2 = CJK_GP_GET_Y(next_gp);

	x1 = x2 - x1;
	y1 = y1 - y2; /*y increasing uppwards. */

	temp_gp = 0;

	if (x1*x1 <= 1 && y1*y1 <= 1) {
		x1 = x1 + 2;
		y1 = y1 + 2;
		temp_gp = (CJK_GRIDPOINT) (x1 | (y1 << 4));
	}
	else {
		if (x1 > 0 && y1 >= 0) { /*1:st quadrant */
			if (y1*y1 > 14*x1*x1) temp_gp = 66;
			else if (x1*x1 > 14*y1*y1) temp_gp = 36;
			else if (x1*x1 < y1*y1) temp_gp = 67;
			else temp_gp = 52;
		}
		else if (x1 <= 0 && y1 > 0) { /*2:nd quadrant */
			if (y1*y1 > 14*x1*x1) temp_gp = 66;
			else if (x1*x1 > 14*y1*y1) temp_gp = 32;
			else if (x1*x1 < y1*y1) temp_gp = 65;
			else temp_gp = 48;
		}
		else if (x1 < 0 && y1 <= 0) { /*3:rd quadrant */
			if (y1*y1 > 14*x1*x1) temp_gp = 2;
			else if (x1*x1 > 14*y1*y1) temp_gp = 32;
			else if (x1*x1 < y1*y1) temp_gp = 1;
			else temp_gp = 16;
		}
		else if (x1 >= 0 && y1 < 0) { /*4:th quadrant */
			if (y1*y1 > 14*x1*x1) temp_gp = 2;
			else if (x1*x1 > 14*y1*y1) temp_gp = 36;
			else if (x1*x1 < y1*y1) temp_gp = 3;
			else temp_gp = 20;
		}
	}
	decumaAssert(temp_gp);
	return temp_gp;
}


void dltGPIterInit(CJK_GRIDPOINT_ITERATOR * pGPIter, 
			 CJK_COMPRESSED_CHAR    * pCChar, 
			 CJK_SESSION            * pSession) 
{
	dltCCharGetFirstStroke(pCChar, &pGPIter->stroke, pSession);

	pGPIter->n = CJK_STROKE_NPOINTS(&pGPIter->stroke);
	pGPIter->p = CJK_STROKE_FIRST_POINT(&pGPIter->stroke);
}



DECUMA_INT32 dltGPIterGetClosestPos(CJK_GRIDPOINT_ITERATOR * pGPIter, 
							  DECUMA_INT32             nPoints, 
							  DECUMA_UINT8             njump, 
							  CJK_SESSION            * pSession) 
{
	CJK_GRIDPOINT_ITERATOR firstgi;
	CJK_GRIDPOINT_ITERATOR nextgi;
	CJK_DISTANCE mindist, dist;
	DECUMA_INT32 pos = 0;
	DECUMA_INT32 i, counter = 1;

	decumaAssert(njump <= nPoints);

	if (njump > nPoints) {
		return 0;
	}

	mindist = MAX_CJK_DISTANCE;
	firstgi = *pGPIter;
	nextgi  = *pGPIter;

	for (i = 0; i < njump + 1; i++) {
		CJK_GPITER_NEXT(&nextgi, pSession);
		counter++;
	}

	while (CJK_GPITER_HAS_GRIDPOINT(&nextgi) && counter < nPoints) {
		dist = CJK_GP_GET_SQ_DISTANCE(CJK_GPITER_GET_GRIDPOINT(&firstgi), CJK_GPITER_GET_GRIDPOINT(&nextgi));
		if (dist < mindist) {
			pos = counter;
			mindist = dist;
		}
		CJK_GPITER_NEXT(&nextgi, pSession);
		counter++;
	}

	return pos;
}


DECUMA_INT32 dltGPIterGetIntersectionCount(CJK_GRIDPOINT_ITERATOR * pGPIter, DECUMA_INT32 nPoints, CJK_SESSION * pSession) 
{
	DECUMA_INT32 npointsinter, totinter = 0;
	CJK_GRIDPOINT_ITERATOR gi1, gi2, gi3, gi4, gi2next, gi4next;
	DECUMA_INT32 x1, x2, x2next = 0, x3, x4, x4next = 0, y1, y2, y2next = 0, y3, y4, y4next = 0, x, y;
	DECUMA_INT32 status;
	DECUMA_INT32 counter_seg1 = 1, counter_seg2;

	/* first segment init */
	gi1 = *pGPIter;
	gi2 = *pGPIter;
	CJK_GPITER_NEXT(&gi2, pSession);
	gi2next = gi2;
	CJK_GPITER_NEXT(&gi2next, pSession);


	while(CJK_GPITER_HAS_GRIDPOINT(&gi2) && counter_seg1 < nPoints ) {

		/* second segment init */
		gi3 = gi1;
		CJK_GPITER_NEXT(&gi3, pSession);
		CJK_GPITER_NEXT(&gi3, pSession);
		gi4 = gi2;
		CJK_GPITER_NEXT(&gi4, pSession);
		CJK_GPITER_NEXT(&gi4, pSession);
		gi4next = gi4;
		CJK_GPITER_NEXT(&gi4next, pSession);
		counter_seg2 = counter_seg1 + 2; /*Segment2  starts 2 segments ahed of segment1. */



		/* calculate coordinates first segment */
		x1 = CJK_GP_GET_X(*gi1.p);
		y1 = CJK_GP_GET_Y(*gi1.p);
		x2 = CJK_GP_GET_X(*gi2.p);
		y2 = CJK_GP_GET_Y(*gi2.p);
		if (CJK_GPITER_HAS_GRIDPOINT(&gi2next)) {
			x2next = CJK_GP_GET_X(*gi2next.p);
			y2next = CJK_GP_GET_Y(*gi2next.p);
		}


		while (CJK_GPITER_HAS_GRIDPOINT(&gi4) && counter_seg2 < nPoints) {
			if (!CJK_GPITER_IS_LAST(gi1) && !CJK_GPITER_IS_LAST(gi3)) {

				/* calculate coordinates second segment */
				x3 = CJK_GP_GET_X(*gi3.p);
				y3 = CJK_GP_GET_Y(*gi3.p);
				x4 = CJK_GP_GET_X(*gi4.p);
				y4 = CJK_GP_GET_Y(*gi4.p);
				if (CJK_GPITER_HAS_GRIDPOINT(&gi4next)) {
					x4next = CJK_GP_GET_X(*gi4next.p);
					y4next = CJK_GP_GET_Y(*gi4next.p);
				}


				/*----------------------------------------------------- */
				/* calc status and increase totinter */
				/*  */
				/* This chunk is calling [[cjkMathLinesIntersect]] and then incrementing totinter */
				/* if there was an intersection. The variable npointsinter remembers */
				/* how many points coincide with the intersection point. Different cases has to be */
				/* considered for $npointsinter = 0, 1, 2$ */

				status = cjkMathLinesIntersect( x1, y1, x2, y2, x3, y3, x4, y4, &x, &y);

				/* calc npointsinter */
				npointsinter = 0;
				if (status == DO_INTERSECT) {
					npointsinter = (x == x1 && y == y1) +
						(x == x2 && y == y2) +
						(x == x3 && y == y3) +
						(x == x4 && y == y4);
				}
				decumaAssert(npointsinter <= 2);



				if (npointsinter == 0 && status == DO_INTERSECT ) {

					/**
					 * npointsinter = 0
					 * 
					 * The case npointsinter == 0 is easy, see figure \ref{fig:intersections0}.
					 * \f{figure}[h!]
					 *     \begin{center}
					 *       \epsfig{file=../../figures/intersection0.eps}
					 *     \end{center}
					 * \caption{npointsinter = 0} \label{fig:intersections0}
					 * \f}
					 */

					totinter++;


				}
				else if (npointsinter == 1) {

					/**
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
					 */

					if (((x == x4 && y == y4) || (x == x3 && y == y3)) &&
						cjkMathPointOnWhatSide(x1, y1, x2, y2, x3, y3)  !=
						cjkMathPointOnWhatSide(x1, y1, x2, y2, x4, y4) &&
						cjkMathPointOnWhatSide(x1, y1, x2, y2, x3, y3) != 0 &&
						cjkMathPointOnWhatSide(x1, y1, x2, y2, x4, y4) != 0) {
							totinter++;
					}
					else if ( ((x == x2 && y == y2) || (x == x1 && y == y1)) &&
						cjkMathPointOnWhatSide(x3, y3, x4, y4, x2, y2)  !=
						cjkMathPointOnWhatSide(x3, y3, x4, y4, x1, y1) &&
						cjkMathPointOnWhatSide(x3, y3, x4, y4, x2, y2) != 0 &&
						cjkMathPointOnWhatSide(x3, y3, x4, y4, x1, y1) != 0) {
							totinter++;
					}
					else if ((x == x4 && y == y4) && !CJK_GPITER_IS_LAST(gi4)) {
						if (cjkMathPointOnWhatSide(x1, y1, x2, y2, x3, y3) !=
							cjkMathPointOnWhatSide(x1, y1, x2, y2, x4next, y4next)) {totinter++;}
					}
					else if ((x == x2 && y == y2) && !CJK_GPITER_IS_LAST(gi2)) {
						if (cjkMathPointOnWhatSide(x3, y3, x4, y4, x1, y1) !=
							cjkMathPointOnWhatSide(x3, y3, x4, y4, x2next, y2next)) {totinter++;}
					}

				}
				else if (npointsinter == 2) {

					/**
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

					if ((x == x4 && y == y4) && (x == x2 && y == y2) &&
						!CJK_GPITER_IS_LAST(gi4) && !CJK_GPITER_IS_LAST(gi2) ) {
							if ((cjkMathPointOnWhatSide(x3, y3, x4, y4, x1, y1) !=
								cjkMathPointOnWhatSide(x3, y3, x4, y4, x2next, y2next) &&
								cjkMathPointOnWhatSide(x4, y4, x4next, y4next, x1, y1) !=
								cjkMathPointOnWhatSide(x4, y4, x4next, y4next, x2next, y2next)) &&
								(cjkMathPointOnWhatSide(x1, y1, x2, y2, x3, y3) !=
								cjkMathPointOnWhatSide(x1, y1, x2, y2, x4next, y4next) &&
								cjkMathPointOnWhatSide(x2, y2, x2next, y2next, x3, y3) !=
								cjkMathPointOnWhatSide(x2, y2, x2next, y2next, x4next, y4next) )) {
									totinter++;
							}
					}


				}

			}

			/*----------------------------------------------------- */
			/* update second segment */
			/*  */
			/* Let gi3 and gi4 (the second segment) move one step forward. */

			CJK_GPITER_NEXT(&gi3, pSession);
			CJK_GPITER_NEXT(&gi4, pSession);
			CJK_GPITER_NEXT(&gi4next, pSession);
			counter_seg2++;
		}

		/*----------------------------------------------------- */
		/* update first segment */
		/*  */
		/* Let gi1 and gi2 (the first segment) move one step forward. */

		CJK_GPITER_NEXT(&gi1, pSession);
		CJK_GPITER_NEXT(&gi2, pSession);
		CJK_GPITER_NEXT(&gi2next, pSession);
		counter_seg1++;
	}

	return totinter;
}

static void dltCCharCopyDbPoints(CJK_COMPRESSED_CHAR const * const b, 
						 CJK_GRIDPOINT                    (* const bgp)[MAXPNTSPERSTR],
						 DECUMA_UINT8                     (* const bss)[MAXPNTSPERSTR], 
						 DECUMA_UINT8                              componentVariationFork[], 
						 CJK_SESSION const * const pSession) 
{
	DECUMA_UINT8 const *p;
	DECUMA_UINT8 const *p_end;
	DECUMA_UINT8 *qbgp;
	DECUMA_UINT8 *qbss;
	DECUMA_UINT8 segstart;
	DECUMA_INT32 i, j, k;


	/*----------------------------------------------------- */
	/* init cmfork to zero */
	/*  */
	/* The vector [[componentVariationFork]] will be non-zero only at points that start */
	/* new components, so we initialise it to zero first. */
	/*  */
	decumaMemset(componentVariationFork, 0, CJK_CCHAR_NPOINTS(b));

	p = (DECUMA_UINT8 *) b->pCCData;
	p_end = p + *p;
	p++;
	i = 0;
	while (p != p_end) {
		decumaAssert(p < p_end);
		if (*p > MAXPNTSPERSTR) {
			const DECUMA_UINT8 *r, *r_end;

			/*----------------------------------------------------- */
			/* find component */
			/*  */
			/* This chunk makes the */
			/* variable [[r]] point to the component we shall get. The */
			/* variable [[p]] will point to the next position in the character to */
			/* use after we have processed the component. */
			/*  */
			if (*p < pSession->db.firsttwobyteindex) {
				k = *p++ - (MAXPNTSPERSTR + 1);
			}
			else {
				k = (*p++ - pSession->db.firsttwobyteindex) << 8;
				k += *p++ + pSession->db.firsttwobyteindex - (MAXPNTSPERSTR + 1);
			}
			r = DLTDB_GET_COMPONENT_DATA(&pSession->db, k);


			/*----------------------------------------------------- */
			/* copy component */
			/*  */
			/* A component may have up to [[MAXCMVERSIONS]] versions. The local */
			/* variable [[qbgp]] will point to the first element in the correct */
			/* row of [[bgp]] and [[i]] will be the index number of the present point */
			/* so that [[qbgp[i]]] will be the destination of a point. */

			{
				DECUMA_INT32 i_restore = i, compcount = 0;
				while (1) {
					qbgp = bgp[compcount]; /* Destination gridpoint */
					qbss = bss[compcount]; /* Destination segstart */
					decumaAssert(compcount == componentVariationFork[i]);
					componentVariationFork[i]++;

					/* All the points in a component variant are copied. */
					r_end = r + *r;
					r++;
					while (r != r_end) {
						decumaAssert(r < r_end);
						/* TODO Identical loop in a few lines down; move to support function? */
						segstart = 1;
						for (j = *r++; j != 0; j--) {
							qbss[i] = segstart;
							qbgp[i++] = *r++;
							segstart = 0;
						}
					}


					if (*r == 0) break;
					compcount++;
					decumaAssert(compcount < MAXCMVERSIONS);
					i = i_restore;
				}
			}
		}
		else {
			decumaAssert(componentVariationFork[i] == 0);

			/*----------------------------------------------------- */
			/* copy noncomponent strip */
			/*  */
			/* All the points are copied until we hit a component or the end */
			/* of the character. */

			componentVariationFork[i] = 1;
			qbgp = bgp[0];
			qbss = bss[0];
			while (p != p_end && *p <= MAXPNTSPERSTR) {
				decumaAssert(p < p_end);
				/* TODO Alternative (faster?) solution:
				 *		
				 *		qbss[i] = 1;
				 *		decumaMemset(&qbss[1], 0, *p - 1) 
				 *      decumaMemcpy(&qbgp[0], p + 1, *p);
				 *      i += *p;
				 *
				 */
				segstart = 1;
				for (j = *p++; j != 0; j--) {
					qbss[i] = segstart;
					qbgp[i++] = *p++;
					segstart = 0;
				}
			}
		}
	}
	componentVariationFork[i] = STOPVALUE;
}



void updateTotalDistance_gap(
	      CJK_DISTANCE * pTotDist, 
	const DECUMA_UINT8 * pUserGridPoints, 
	const DECUMA_INT32   r,
	const DECUMA_UINT8 * pCurrUserStrokeStart, 
	const DECUMA_UINT8 * pCurrDbStrokeStart,
		  CJK_BOOLEAN    bDoubleBandwidth,
	      DECUMA_INT32   nUserGpIdx
#ifdef CJK_DTW_ALIGNMENT
	      ,
	      CJK_DISTANCE * pDtwMatrixRow
#endif
	      )
{
	CJK_DISTANCE left;

	/* define a "local macro" to make the code more readable */
	#define GAP_LOCAL(m_k) GAP(pTotDist, m_k, pUserGridPoints, pCurrUserStrokeStart, pCurrDbStrokeStart, left)

	               UDR(pTotDist, 0);                            ADD(pTotDist, 0, pUserGridPoints, r);
	GAP_LOCAL(1);  UDR(pTotDist, 1);  UDLG(pTotDist, 1, left);  ADD(pTotDist, 1, pUserGridPoints, r);
	GAP_LOCAL(2);  UDR(pTotDist, 2);  UDLG(pTotDist, 2, left);  ADD(pTotDist, 2, pUserGridPoints, r);
	GAP_LOCAL(3);  UDR(pTotDist, 3);  UDLG(pTotDist, 3, left);  ADD(pTotDist, 3, pUserGridPoints, r);
	GAP_LOCAL(4);  UDR(pTotDist, 4);  UDLG(pTotDist, 4, left);  ADD(pTotDist, 4, pUserGridPoints, r);
	GAP_LOCAL(5);  UDR(pTotDist, 5);  UDLG(pTotDist, 5, left);  ADD(pTotDist, 5, pUserGridPoints, r);
	GAP_LOCAL(6);  UDR(pTotDist, 6);  UDLG(pTotDist, 6, left);  ADD(pTotDist, 6, pUserGridPoints, r);
	GAP_LOCAL(7);  UDR(pTotDist, 7);  UDLG(pTotDist, 7, left);  ADD(pTotDist, 7, pUserGridPoints, r);

	if (!bDoubleBandwidth) {

	GAP_LOCAL(8);                     UDLG(pTotDist, 8, left);  ADD(pTotDist, 8, pUserGridPoints, r);
	}
	else {
	GAP_LOCAL(8);  UDR(pTotDist, 8);  UDLG(pTotDist, 8, left);  ADD(pTotDist, 8, pUserGridPoints, r);
	GAP_LOCAL(9);  UDR(pTotDist, 9);  UDLG(pTotDist, 9, left);  ADD(pTotDist, 9, pUserGridPoints, r);
	GAP_LOCAL(10); UDR(pTotDist,10);  UDLG(pTotDist,10, left);  ADD(pTotDist,10, pUserGridPoints, r);
	GAP_LOCAL(11); UDR(pTotDist,11);  UDLG(pTotDist,11, left);  ADD(pTotDist,11, pUserGridPoints, r);
	GAP_LOCAL(12); UDR(pTotDist,12);  UDLG(pTotDist,12, left);  ADD(pTotDist,12, pUserGridPoints, r);
	GAP_LOCAL(13); UDR(pTotDist,13);  UDLG(pTotDist,13, left);  ADD(pTotDist,13, pUserGridPoints, r);
	GAP_LOCAL(14); UDR(pTotDist,14);  UDLG(pTotDist,14, left);  ADD(pTotDist,14, pUserGridPoints, r);
	GAP_LOCAL(15); UDR(pTotDist,15);  UDLG(pTotDist,15, left);  ADD(pTotDist,15, pUserGridPoints, r);
	GAP_LOCAL(16);					  UDLG(pTotDist,16, left);  ADD(pTotDist,16, pUserGridPoints, r);
	}

	/* undefine "local macro" */
	#undef GAP_LOCAL

#ifdef CJK_DTW_ALIGNMENT
	/* TODO What happens if we use decumaMemcpy here? */
	{
		int i; 
		for (i = nUserGpIdx < 0 ? - nUserGpIdx : 0; i < (bDoubleBandwidth ? BW_DOUBLE : BW); i++) {
			pDtwMatrixRow[nUserGpIdx + i] = pTotDist[i];
		}
	}
#endif /* CJK_DTW_ALIGNMENT */
}



#if defined( __16bis__) && defined(__TMS470__) && defined (TUPD_ARM_ASM_IS_UPDATED)
/* The implementation is in tupd_arm.asm */
/* TODO update tupd_arm.asm with alignment calculation  */
#else
void updateTotalDistance_nogap(
          CJK_DISTANCE  * pTotDist, 
	const DECUMA_UINT8  * pUserGridPoints, 
	const DECUMA_INT32    r,
	const DECUMA_UINT8  * pGridPointDistTable,
		  CJK_BOOLEAN     bDoubleBandwidth,
	      DECUMA_INT32    nUserGpIdx
#ifdef CJK_DTW_ALIGNMENT
		  ,
		  CJK_DISTANCE  * pDtwMatrixRow
#endif
		  )
{
	DECUMA_UNUSED_PARAM(pGridPointDistTable);

	if (!bDoubleBandwidth) {
	UDR(pTotDist, 0);                     ADD(pTotDist, 0, pUserGridPoints, r);
	UDR(pTotDist, 1);  UDL(pTotDist, 1);  ADD(pTotDist, 1, pUserGridPoints, r);
	UDR(pTotDist, 2);  UDL(pTotDist, 2);  ADD(pTotDist, 2, pUserGridPoints, r);
	UDR(pTotDist, 3);  UDL(pTotDist, 3);  ADD(pTotDist, 3, pUserGridPoints, r);
	UDR(pTotDist, 4);  UDL(pTotDist, 4);  ADD(pTotDist, 4, pUserGridPoints, r);
	UDR(pTotDist, 5);  UDL(pTotDist, 5);  ADD(pTotDist, 5, pUserGridPoints, r);
	UDR(pTotDist, 6);  UDL(pTotDist, 6);  ADD(pTotDist, 6, pUserGridPoints, r);
	UDR(pTotDist, 7);  UDL(pTotDist, 7);  ADD(pTotDist, 7, pUserGridPoints, r);	
					   UDL(pTotDist, 8);  ADD(pTotDist, 8, pUserGridPoints, r);
	}
	else {
	CJK_BOOLEAN bIsDiag = 1;
	UDR_DIAG(pTotDist, 0, pUserGridPoints, r, bIsDiag)                
														ADD_DIAG(pTotDist, 0, pUserGridPoints, r, bIsDiag);
	UDR_DIAG(pTotDist, 1, pUserGridPoints, r, bIsDiag)	UDL_DIAG(pTotDist, 1, pUserGridPoints, r, bIsDiag)
														ADD_DIAG(pTotDist, 1, pUserGridPoints, r, bIsDiag);
	UDR_DIAG(pTotDist, 2, pUserGridPoints, r, bIsDiag)  UDL_DIAG(pTotDist, 2, pUserGridPoints, r, bIsDiag) 
														ADD_DIAG(pTotDist, 2, pUserGridPoints, r, bIsDiag);
	UDR_DIAG(pTotDist, 3, pUserGridPoints, r, bIsDiag)  UDL_DIAG(pTotDist, 3, pUserGridPoints, r, bIsDiag) 
														ADD_DIAG(pTotDist, 3, pUserGridPoints, r, bIsDiag);
	UDR_DIAG(pTotDist, 4, pUserGridPoints, r, bIsDiag)  UDL_DIAG(pTotDist, 4, pUserGridPoints, r, bIsDiag) 
														ADD_DIAG(pTotDist, 4, pUserGridPoints, r, bIsDiag);
	UDR_DIAG(pTotDist, 5, pUserGridPoints, r, bIsDiag)  UDL_DIAG(pTotDist, 5, pUserGridPoints, r, bIsDiag) 
														ADD_DIAG(pTotDist, 5, pUserGridPoints, r, bIsDiag);
	UDR_DIAG(pTotDist, 6, pUserGridPoints, r, bIsDiag)  UDL_DIAG(pTotDist, 6, pUserGridPoints, r, bIsDiag) 
														ADD_DIAG(pTotDist, 6, pUserGridPoints, r, bIsDiag);
	UDR_DIAG(pTotDist, 7, pUserGridPoints, r, bIsDiag)  UDL_DIAG(pTotDist, 7, pUserGridPoints, r, bIsDiag) 
														ADD_DIAG(pTotDist, 7, pUserGridPoints, r, bIsDiag);
	UDR_DIAG(pTotDist, 8, pUserGridPoints, r, bIsDiag)  UDL_DIAG(pTotDist, 8, pUserGridPoints, r, bIsDiag) 
														ADD_DIAG(pTotDist, 8, pUserGridPoints, r, bIsDiag);
	UDR_DIAG(pTotDist, 9, pUserGridPoints, r, bIsDiag)  UDL_DIAG(pTotDist, 9, pUserGridPoints, r, bIsDiag) 
														ADD_DIAG(pTotDist, 9, pUserGridPoints, r, bIsDiag);
	UDR_DIAG(pTotDist, 10, pUserGridPoints, r, bIsDiag) UDL_DIAG(pTotDist, 10, pUserGridPoints, r, bIsDiag) 
														ADD_DIAG(pTotDist, 10, pUserGridPoints, r, bIsDiag);
	UDR_DIAG(pTotDist, 11, pUserGridPoints, r, bIsDiag) UDL_DIAG(pTotDist, 11, pUserGridPoints, r, bIsDiag) 
														ADD_DIAG(pTotDist, 11, pUserGridPoints, r, bIsDiag);
	UDR_DIAG(pTotDist, 12, pUserGridPoints, r, bIsDiag) UDL_DIAG(pTotDist, 12, pUserGridPoints, r, bIsDiag) 
														ADD_DIAG(pTotDist, 12, pUserGridPoints, r, bIsDiag);
	UDR_DIAG(pTotDist, 13, pUserGridPoints, r, bIsDiag) UDL_DIAG(pTotDist, 13, pUserGridPoints, r, bIsDiag) 
														ADD_DIAG(pTotDist, 13, pUserGridPoints, r, bIsDiag);
	UDR_DIAG(pTotDist, 14, pUserGridPoints, r, bIsDiag) UDL_DIAG(pTotDist, 14, pUserGridPoints, r, bIsDiag) 
														ADD_DIAG(pTotDist, 14, pUserGridPoints, r, bIsDiag);
	UDR_DIAG(pTotDist, 15, pUserGridPoints, r, bIsDiag) UDL_DIAG(pTotDist, 15, pUserGridPoints, r, bIsDiag) 
														ADD_DIAG(pTotDist, 15, pUserGridPoints, r, bIsDiag);
														UDL_DIAG(pTotDist, 16, pUserGridPoints, r, bIsDiag) 
														ADD_DIAG(pTotDist, 16, pUserGridPoints, r, bIsDiag);
	}

#ifdef CJK_DTW_ALIGNMENT
	{
		int i;
		/* TODO What happens if we use decumaMemcpy here? */
		for (i = nUserGpIdx < 0 ? - nUserGpIdx : 0; i < (bDoubleBandwidth ? BW_DOUBLE : BW); i++) {
			pDtwMatrixRow[nUserGpIdx + i] = pTotDist[i];
		}
	}
#endif /* CJK_DTW_ALIGNMENT */
}
#endif










static CJK_DISTANCE cc_dist_sof1(CJK_COMPRESSED_CHAR * a, CJK_COMPRESSED_CHAR * b, DECUMA_INT32 delta, CJK_DISTANCE distanceLimit, CJK_SESSION * pSession) 
{
	signed char i;
	signed char match[MAXNSTRK];
	CJK_DISTANCE dist;
	if (a->nStrokes != b->nStrokes || !a->nStrokes) {
		return LARGEDIST;
	}

	/*----------------------------------------------------- */
	/* initialize match array */
	/*  */
	/* When looping over the strokes in the second argument [[b]], the datatype */
	/* [[match]] is an array of flags that are set to the stroke number in [[b]] */
	/* that is already matched. */
	/* At the outset, all flags are set to [[-1]]. */
	for (i = 0; i < b->nStrokes; i++) {
		/* TODO Use decumaMemset instead? */
		match[i] = -1;
	}



	/*----------------------------------------------------- */
	/* loop over strokes in first argument */
	/*  */
	/* For each stroke in [[a]], we find the closest stroke in [[b]], */
	/* update the distance, and mark the closest stroke in the [[match]] array. */
	{
		CJK_DISTANCE mindist;
		DECUMA_INT32 minind = 0;
		CJK_STROKE sa;
		dltCCharGetFirstStroke(a, &sa, pSession);
		dist = 0;
		i = 0;
		while (CJK_STROKE_EXISTS(&sa)) {
			mindist = MAXDIST;

			/*----------------------------------------------------- */
			/* loop over non-matched strokes in second argument */
			/*  */
			/* We find the closest stroke in [[b]] that has not yet been matched with */
			/* a stroke in [[a]]. */
			/* The closest distance is stored in [[mindist]] and the index of the */
			/* closest stroke is stored in [[minind]]. */
			{
				CJK_DISTANCE   strkdist;
				DECUMA_INT32    j  = 0;
				CJK_STROKE sb;
				dltCCharGetFirstStroke(b, &sb, pSession);
				while (CJK_STROKE_EXISTS(&sb)) {
					if (match[j] == -1) {
						strkdist = cjkStrokeGetDistanceDTW(&sa, &sb, delta);
						if (strkdist < mindist) {
							mindist = strkdist;
							minind  = j;
						}
					}
					j++;
					cjkStrokeNext(&sb, pSession);
				}
			}


			dist += mindist;
			if (dist >= distanceLimit) return LARGEDIST;
			match[minind] = i;
			i++;
			cjkStrokeNext(&sa, pSession);
		}
	}

	return dist;
}

#define CJK_DTW_BW_RIGHT 5
#define CJK_DTW_BW_LEFT  4
#define CJK_DTW_BW  (CJK_DTW_BW_LEFT+CJK_DTW_BW_RIGHT+1)
#define CJK_DTW_PENUP_PUNISH 40

static void cjkCCharCopyPoints(CJK_GRIDPOINT * pStrokeStarts, 
									  CJK_GRIDPOINT * pGPDest, 
									  const CJK_COMPRESSED_CHAR_DATA * pCCDataSrc)
{
	CJK_GRIDPOINT * pEnd = ((CJK_GRIDPOINT *)pCCDataSrc) + *(pCCDataSrc);
	CJK_GRIDPOINT * pGPIter = ((CJK_GRIDPOINT *)pCCDataSrc)+1;
	int i = 0, j;

	while (pGPIter != pEnd) {
		decumaAssert(*pGPIter <= MAXPNTSPERSTR);
		decumaAssert(pGPIter < pEnd);
		pStrokeStarts[i] = 1;
		for (j = *pGPIter++; j > 0; j--) {
			decumaAssert(i < MAXPNTSPERSTR);
			/* TODO Use decumaMemcpy instead? */
			pGPDest[i++] = *pGPIter++;
		}		
	}


}

CJK_DISTANCE dltCCharGetDTWDistanceComponent(CJK_COMPRESSED_CHAR   * pCCharIn, 
									CJK_COMPRESSED_CHAR   * pCCharComponent, 
									DECUMA_UINT32		  * pMatchPointIdx,
									CJK_BOOLEAN             bUseDiff, 
									CJK_SESSION           * pSession)
{

	CJK_DISTANCE	 nCumDistAtCompIdx[CJK_DTW_BW], bestDist = LARGEDIST; /* Spans right points and left points + 1 */
	DECUMA_INT32     nPointsIn = CJK_CCHAR_NPOINTS(pCCharIn);
	DECUMA_INT32     nPointsComp = CJK_CCHAR_NPOINTS(pCCharComponent);

	CJK_GRIDPOINT * pGPIn = &((CJK_CC_DTW_SCRATCHPAD *)pSession->pScratchpad)->cc_dist_dtw_agp[0];
	DECUMA_UINT8  * pStrokeStartIn = &((CJK_CC_DTW_SCRATCHPAD *)pSession->pScratchpad)->cc_dist_dtw_ass[0];
	CJK_GRIDPOINT * pGPComp = (CJK_GRIDPOINT *)((CJK_CC_DTW_SCRATCHPAD *)pSession->pScratchpad)->bgp;
	DECUMA_UINT8  * pStrokeStartComp = (DECUMA_UINT8  *)((CJK_CC_DTW_SCRATCHPAD *)pSession->pScratchpad)->bss;

	CJK_GRIDPOINT    pGPDirIn[CC_MAXCCHARSIZE];
	CJK_GRIDPOINT    pGPDirComp[CC_MAXCCHARSIZE];

	int i,j;

	/* Copy all input points locally to simplify DTW algorithm */
	decumaMemset(pGPIn, 0, nPointsIn*sizeof(pGPIn[0]));
	decumaMemset(pStrokeStartIn, 0, nPointsIn*sizeof(pStrokeStartIn[0]));
	cjkCCharCopyPoints(pStrokeStartIn, pGPIn, pCCharIn->pCCData);

	/* Copy all component points locally to simplify DTW algorithm */
	decumaMemset(pGPComp, 0, nPointsComp*sizeof(pGPComp[0]));
	decumaMemset(pStrokeStartComp, 0, nPointsComp*sizeof(pStrokeStartComp[0]));
	cjkCCharCopyPoints(pStrokeStartComp, pGPComp, pCCharComponent->pCCData);

	/* Compute directional elements */
	if (bUseDiff) {
		pGPDirIn[0] = 0;
		for (i=1; i<nPointsIn; i++) {
			pGPDirIn[i] = dltGPGetDirection(pGPIn[i-1], pGPIn[i]);
		}
		pGPDirComp[0] = 0;
		for (i=1; i<nPointsComp; i++) {
			pGPDirComp[i] = dltGPGetDirection(pGPComp[i-1], pGPComp[i]);
		}
	}

	/* Go through the component points one by one */
	for (i=0; i < CJK_DTW_BW; i++) {
		/* TODO Use decumaMemset? */
		nCumDistAtCompIdx[i] = LARGEDIST;
	}
	for (i=0; i < DECUMA_MIN(CJK_DTW_BW_RIGHT, nPointsIn); i++) {
		/* TODO Check bUseDiff outside the loop and use a temporary CJK_DISTANCE pointer instead of checking each iteration 
		 *      Actually, (pGPDirComp, pGPDirIn) seems mutually exclusive to (pGPComp, pGPIn). Refactor?
		 */
		if (bUseDiff) {
			nCumDistAtCompIdx[CJK_DTW_BW_LEFT+i] = CJK_GP_GET_SQ_DISTANCE(pGPDirComp[0], pGPDirIn[i]);
		}
		else {
			nCumDistAtCompIdx[CJK_DTW_BW_LEFT+i] = CJK_GP_GET_SQ_DISTANCE(pGPComp[0], pGPIn[i]);
		}
		if (i > 0) nCumDistAtCompIdx[CJK_DTW_BW_LEFT+i] += nCumDistAtCompIdx[CJK_DTW_BW_LEFT+i-1];
		/* Punish for strokelift */
		if (i > 0 && pStrokeStartIn[i] && !pStrokeStartIn[i-1]) {
			nCumDistAtCompIdx[CJK_DTW_BW_LEFT+i] += CJK_DTW_PENUP_PUNISH;
		}
	}
	/* Now consume all points in component */
	for (i=1; i < nPointsComp; i++) {
		register CJK_DISTANCE prevDistLeft = LARGEDIST, pointDist;
		for (j=-DECUMA_MIN(i, CJK_DTW_BW_LEFT); j < DECUMA_MIN(CJK_DTW_BW_RIGHT, nPointsIn-i); j++) {
			/* TODO Check bUseDiff outside the loop(s) and use a temporary CJK_DISTANCE pointer instead of checking each iteration 
			 *      Actually, (pGPDirComp, pGPDirIn) seems mutually exclusive to (pGPComp, pGPIn). Refactor?
			 */
			if (bUseDiff) {
				pointDist = CJK_GP_GET_SQ_DISTANCE(pGPDirComp[i], pGPDirIn[i+j]);
			}
			else {
				pointDist = CJK_GP_GET_SQ_DISTANCE(pGPComp[i], pGPIn[i+j]);
			}
			if (j > -DECUMA_MIN(i, CJK_DTW_BW_LEFT)) {
				/* Update left distance - this has already been updated for i */
				prevDistLeft = nCumDistAtCompIdx[CJK_DTW_BW_LEFT+j-1]+pointDist;
				/* Calculate Gap punishment */
				if (i+j > 0 && pStrokeStartIn[i+j] && ! pStrokeStartIn[i+j-1]) {
					if (CJK_GP_GET_SQ_DISTANCE(pGPIn[i+j], pGPIn[i+j-1])>2) prevDistLeft += CJK_DTW_PENUP_PUNISH;
					if (!pStrokeStartComp[i]) nCumDistAtCompIdx[CJK_DTW_BW_LEFT+j] += CJK_DTW_PENUP_PUNISH;
				}
			}
			/* Assume diagonal update - do not worry about max dist */
			nCumDistAtCompIdx[CJK_DTW_BW_LEFT+j] += 2*pointDist; /* to even weight of taking Manhattan trip */
			/* for dltlib it has always been 1 - favoring diagonal moves */

			/* Update Right == updating from below (i=i-1) */
			if (nCumDistAtCompIdx[CJK_DTW_BW_LEFT+j] > nCumDistAtCompIdx[CJK_DTW_BW_LEFT+j+1]+pointDist) {
				nCumDistAtCompIdx[CJK_DTW_BW_LEFT+j] = nCumDistAtCompIdx[CJK_DTW_BW_LEFT+j+1]+pointDist;
			}
			/* Update Left - choose best preceeding (j=j-1), prevDistLeft = LARGEDIST for first j */
			if (nCumDistAtCompIdx[CJK_DTW_BW_LEFT+j] > prevDistLeft) nCumDistAtCompIdx[CJK_DTW_BW_LEFT+j] = prevDistLeft;
		}
	}
	/* Now find the minimum cell in the cumulative distance which will be the best  */
	/* way to sequentially associate each point in component to incoming character */
	*pMatchPointIdx = 0;
	for (j=-DECUMA_MIN(nPointsComp-1, CJK_DTW_BW_LEFT); j < DECUMA_MIN(CJK_DTW_BW_RIGHT, nPointsIn-(nPointsComp-1)); j++) {
		if (bestDist > nCumDistAtCompIdx[CJK_DTW_BW_LEFT+j]) {
			bestDist = nCumDistAtCompIdx[CJK_DTW_BW_LEFT+j];
			*pMatchPointIdx = nPointsComp-1+j;
		}
	}	
	return bestDist;

}

#ifdef _DEBUG

#include <stdio.h>

void dltCCharDebugDump(FILE * fh, CJK_COMPRESSED_CHAR * pCChar) 
{
	char * p, * e;

	p = (char *)pCChar->pCCData;
	e = p + *p;
	p++;

	fprintf(fh, "Index: %d\nnPoints: %d\n", pCChar->index, pCChar->nPoints);

	while (p < e) {
		int n = *p++;
		int i;

		fprintf(fh, "    ");
		for (i = 0; i < n; i++) {
			fprintf(fh, "%u,%u ", CJK_GP_GET_X(*p), CJK_GP_GET_Y(*p));
			p++;
		}
		fprintf(fh, "\n");
	}
}

#endif /* _DEBUG */

#ifdef CJK_ENABLE_INTERNAL_API
CJK_BOOLEAN dltCCharFromCurve(CJK_SESSION * pSession, CJK_COMPRESSED_CHAR * pCChar, DECUMA_CURVE * pCurve)
{
	/* dltCcharCompressChar puts pCcData in Session */

	CJK_COMPRESSED_CHAR_DATA * pCCData = pCChar->pCCData;

	pCChar->pCCData[0] = 0;

	dltCCharCompressChar(pCChar, pCurve, pSession->pArcSession);
	
	if (pCCData && pCChar->pCCData && pCChar->pCCData[0] != 0) {
		decumaMemcpy(pCCData, pCChar->pCCData, CC_MAXCCHARSIZE);
		pCChar->pCCData = &pCCData[0]; 
		return 1;
	}
	else {
		pCChar->pCCData = &pCCData[0]; 
		return 0;
	}
}
#endif /* CJK_ENABLE_INTERNAL_API */
/** @} */
