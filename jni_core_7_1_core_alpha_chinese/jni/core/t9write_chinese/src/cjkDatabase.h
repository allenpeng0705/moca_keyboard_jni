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
#ifndef CJK_DATABASE_H_
#define CJK_DATABASE_H_

#pragma once

typedef struct _tagCJK_DB_SESSION CJK_DB_SESSION;

#include "dltConfig.h"
#include "decuma_hwr_types.h" /* Definition of DECUMA_STATIC_DB_PTR */

/**
 * @page DOX_DLTDB_BIN_FORMAT
 *
 * @section DOX_DLTDB_BIN_FORMAT Storing Character Data
 *
 * A database character is stored as a string of unsigned bytes. The first
 * byte is the total number of bytes in the character, including the first
 * byte itself. Then come the strokes. A stroke is a byte counting the
 * number of points in the stroke. This is the same as the number of bytes
 * in the stroke minus one, since a point is stored in a byte and the
 * initial point number byte does not count itself. All of this is
 * encapsulated in c structures @ref CJK_COMPRESSED_CHAR, @ref CJK_STROKE,
 * and @ref CJK_GRIDPOINT.
 *
 * A complication is that a database character may contain component references.
 * This is indicated by a stroke initial byte larger than @ref MAXPNTSPERSTR.
 * In that case a index number is assebled. If the byte is less than
 * <tt>pSession->db.firsttwobyteindex</tt> then the component index is that value
 * minus @ref MAXPNTSPERSTR. If it is not then the next byte is also used.
 * The resulting index is used into the array @ref dbComponentsCcdata the
 * reach the component data. Variants are stored after each other in the memory
 * with a terminating zero byte to indicate end of list.
 *
 * The figure below shows a character with two components. The first component
 * has two variants, one with three point stroke and a two point stroke, the
 * other variant has only on stroke with five points. The number of points
 * in variants of the same component always have the same number of points.
 * The second component in the character is indexed by two bytes. The total
 * number of points (including the components) are 14.
 * \f[
 *    \epsfig{file=../../figures/datadesc.eps}
 * \f]
 * \image html ../figures/datadesc.eps
 *
 */

/**
 * @defgroup DOX_CJK_DATABASE cjkDatabase
 */
/** @{ */

/* #include "cjkDatabaseFormat.h" */
/* #include "cjkDatabaseCoarse.h" */
/* #include "cjkCommon.h" */
/* #include "cjkSession.h" */
#include "cjkBestList.h"
/* #include "cjkCompressedCharacter.h" */
/* #include "cjkClusterTree.h" */

/* Bit Operations for byte arrays in session */
#define DLTDB_GET_BIT(a, b)                (((a)[(b)>>3] >> ((b)&7)) & 1)
#define DLTDB_SET_BIT(a, b)                ((a)[(b)>>3] |= (1 << ((b)&7)))
    



#define DLTDB_TWOBYTE_COMPONENT_INDEX(b1, b2) (((b1 - FIRSTTWOBYTEINDEX) << 8) + b2 + FIRSTTWOBYTEINDEX)

#define DLTDB_IS_SIMP_TRAD(m_db) ((m_db).language == 'P')
#define DLTDB_IS_SIMP(m_db)      ((m_db).language == 'S')
#define DLTDB_IS_TRAD(m_db)      ((m_db).language == 'H' || (m_db).language == 'T')
#define DLTDB_IS_JAPANESE(m_db)  ((m_db).language == 'J')
#define DLTDB_IS_KOREAN(m_db)    ((m_db).language == 'K')

/**
 * @returns the size required for allocating the persistent memory needed by cjkDatabase
 */


DECUMA_HWR_PRIVATE DECUMA_UINT32 cjkDbGetSessionSize(void);

/**
 * @returns decumaNoError if pDbBase points to a valid binary and the retrieved language parameter
 * is acceptable. pLang is then set with the corresponding language from decumaLanguages.h
 */

DECUMA_HWR_PRIVATE DECUMA_STATUS cjkDbGetLanguage(DECUMA_STATIC_DB_PTR pDbBase, DECUMA_UINT32 * pLang);

/**
 * @returns decumaNoError if pDbBase points to a valid binary database memory area 
 */
DECUMA_HWR_PRIVATE DECUMA_STATUS
cjkDbIsValid(DECUMA_STATIC_DB_PTR pDbBase);


/**
 * Initialize a DLTDB object using binary database pDbBase
 * @returns decumaNoError if initialization was OK
 */
DECUMA_HWR_PRIVATE DECUMA_STATUS
cjkDbInit(DLTDB * pDb, DECUMA_STATIC_DB_PTR pDbBase);

/**
 * Initialize a NULL DLTDB object, i.e. NOT backed by an actual binary database.
 * This is useful for functions like cjkGetCoarseFeatures() which needs a DLTDB
 * object even if no database is present.
 *
 * @returns decumaNoError if initialization was OK
 */
DECUMA_HWR_PRIVATE DECUMA_STATUS
cjkDbInitDefault(DLTDB * pDb);


/**
 * @returns a pointer to an array of the best matches ordered by decreasing 
 * match. The array is terminated with nul entry. 
 * The function is not reentrant. 
 * It is an unchecked error to call it with a null pointer input argument. 
 * It is safe to call it with very many or zero strokes
 * but then it might answer a number of zero best fits.
 */
DECUMA_HWR_PRIVATE DECUMA_STATUS
cjkDbLookup(CJK_COMPRESSED_CHAR * const pCChar,
            CJK_BOOLEAN           const bUseFreq,
            CJK_SESSION         * const pSession,
			CJK_BESTLIST  const ** const pBestList);


/**
 */
DECUMA_HWR_PRIVATE void
cjkDbGetCCharByIndex(DLTDB_INDEX           index,
                     CJK_COMPRESSED_CHAR * pCChar,
                     CJK_SESSION         * pSession);


/**
 * Find the CJK_COMPRESSED_CHAR in the database
 * that has the desired unicode. In case there are more than one character
 * it finds only the first one, i e the one with the lowest index. The method
 * is slow and should be used only for debugging purposes.
 */
#ifdef DEBUG
void 
cjkDbGetCCharByUnicode(CJK_UNICHAR           u,
                       CJK_COMPRESSED_CHAR * pCChar,
                       CJK_SESSION         * pSession);
#endif


/**
 * Get the distance from pCcharIn to the template stored in nIdx in the database.
 */
DECUMA_HWR_PRIVATE CJK_DISTANCE
cjkDbGetDistanceToIndex(CJK_SESSION		    * pSession, 
                        CJK_COMPRESSED_CHAR * pCcharIn, 
                        DECUMA_UINT16         nIdx,
                        int                   bDoubleBandwidth,
                        int                   bFinal,
								int						 bUseFreq);


/**
 */
DECUMA_HWR_PRIVATE DLTDB_INDEX
cjkDbGetIndexByUnicode(CJK_UNICHAR u, CJK_SESSION * pSession);


/**
 */
DECUMA_HWR_PRIVATE CJK_UNICHAR
cjkDbUnicode(DLTDB_INDEX i, CJK_SESSION * pSession);

/**
 * Retrieves the categorymask of the categories stored in the database.
 */

DECUMA_HWR_PRIVATE DECUMA_INT32
cjkDbGetCategoryMask(const DLTDB * pDB);

/** 
 * If both simplified and traditional is compiled into the engine and the
 * database, then we need to map the traditional characters to
 * the simplified ones. In case only simplified was compiled then we do
 * not need this function since the traditional characters got their
 * simplified Unicodes from the database generating program -- and the
 * translation table is not present in order to save 10 kilobytes of space.
 */
DECUMA_HWR_PRIVATE CJK_UNICHAR
cjkDbUCHan2Han(CJK_UNICHAR const uc, CJK_SESSION const * const pSession);

/**
 * Returns the a boolean if index is in the personal category array.
 */
DECUMA_HWR_PRIVATE DECUMA_UINT32 cjkDbIsInPersonalCategory(CJK_SESSION * pSession, DLTDB_INDEX index);

/**
 * Set up the persistent memory needed to match templates from the database.
 */
DECUMA_HWR_PRIVATE DECUMA_STATUS cjkDbInitDbSession(CJK_DB_SESSION * pDbSession);

/** @} */

#endif /* CJK_DATABASE_H_ */
