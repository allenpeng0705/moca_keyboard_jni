/*****************************************************************************
* COPYRIGHT ZI AND SUBJECT TO CONFIDENTIALITY RESTRICTIONS                   *
*                                                                            *
* This file is the confidential and proprietary property of Zi Corporation   *
* of Canada, Inc. ("Zi") or one of its affiliates.                           *
******************************************************************************/
#ifndef CJK_DATABASE_FORMAT_TYPES_H_
#define CJK_DATABASE_FORMAT_TYPES_H_

#pragma once

#include "dltConfig.h"

#include "cjkDatabaseFormat.h"
#include "cjkDatabaseCoarse_Types.h"

#include "decumaBasicTypes.h"

/** @addtogroup DOX_CJK_DATABASE_FORMAT
  * @{ */

/* Node types */
struct _tagDLTDB_UI8_NODE {
	DECUMA_UINT32  nElements;
	DECUMA_UINT8   pElements[1];
};

struct _tagDLTDB_UI16_NODE {
	DECUMA_UINT32  nElements;
	DECUMA_UINT16  pElements[1];
};

struct _tagDLTDB_UI32_NODE {
	DECUMA_UINT32  nElements;
	DECUMA_UINT32  pElements[1];
};

struct _tagDLTDB_INDEX_NODE {
	DECUMA_UINT32   nElements;
	DECUMA_UINT32   pElements[1];
};

struct _tagDLTDB_NAMED_INDEX_NODE {
	DECUMA_UINT32   nElements;
	DECUMA_UINT8    pData[1];
};

struct _tagDLTDB_INDEX_NODE_NAMED_ENTRY {
	DECUMA_UINT32  nIndex;
	DECUMA_UINT8   pName[1];
};




/* The database type and some components */

struct _tagKNOWN_COMPONENTS {
	DECUMA_UINT16 earth_l; 
	DECUMA_UINT16 speakingS_l;
	DECUMA_UINT16 speakingS_l2;
	DECUMA_UINT16 speakingS_l3; 

	DECUMA_UINT16 threedrops_i; 
	DECUMA_UINT16 threedrops_l;
	DECUMA_UINT16 threedrops_l2;
	DECUMA_UINT16 threedrops_l3;
	DECUMA_UINT16 threedrops_ld;
	DECUMA_UINT16 threedrops_ld2;
};

#ifdef EXTENDED_DATABASE
	struct _tagDLTDB_FEATURE_TABLE {
		DECUMA_UINT32      nFeatureCount;
		DLTDB_INDEX_NODE * pKeyIndex;
	};
#endif

/* Database */
struct _tagDLTDB {
	/** Points to the beginning of the binary database */
	DECUMA_UINT8              * pBase;
	/** The main index node in the database */
	DLTDB_NAMED_INDEX_NODE    * pDbIndex;
			
	/** @name Database Nodes
	    @{ 
	*/

	/** Copyright message */
	DECUMA_UINT8              * pCopyright;
	/** Compact version information */
	DECUMA_UINT8              * pVersion;
	/** version string */
	DECUMA_UINT8              * pVersionStr;
	/** Database label */
	DECUMA_UINT8              * pLabel;
	/** Database constants node */
	DLTDB_NAMED_UI32_NODE     * pConstants;
						      
	/** Component data, points to DECUMA_UINT8 nodes. @see CJK_COMPRESSED_CHAR */
	DLTDB_INDEX_NODE          * pCompData;
	/** Character data, points to DECUMA_UINT8 nodes. @see CJK_COMPRESSED_CHAR */
	DLTDB_INDEX_NODE          * pCharData;

	/** Indexlist information. Entry pIndex2Indexlists[x] corresponds to
	    to indexlist element corresponding to offset [x] obtained by
	    pIndexlistOffsetEnd [indexlist_index]. */
	DECUMA_UINT16             * pIndex2Indexlists;
	/** Indexlist length information. Entry pIndexlistOffsetEnd[x] corresponds to
	    the end offset for indexlist [x]. nElements in list by
	    pIndexlistOffsetEnd[x] - pIndexlistOffsetEnd[x-1]*/
	DECUMA_UINT32             * pIndexlistOffsetEnd;
	/** Key category and attribute list index. Entry pCatAndAttribList[x] for keyIndex x corresponds to
	    category pCategory[pCatAndAttribList[x]] and pAttrib[pCatAndAttribList[x]]. */
	DECUMA_UINT8 			  * pCatAndAttribList;
	/** Key category information. Entry pCategory[x] corresponds to 
	    database character pCharData[x]. */
	DECUMA_UINT32             * pCategory;
	/** Key writing style information. Entry pWritingStyle[x] corresponds to 
	    database character pCharData[x]. */
	DECUMA_UINT8              * pWritingStyle;
	/** Key unicode information. Entry pUnicode[x] corresponds to 
	    database character pCharData[x]. */
	DECUMA_UINT16             * pUnicode;
	/** Key category information. Entry pCategory[x] corresponds to 
	    database character pCharData[x]. */
	DLTDB_ATTRIBUTE           * pAttrib;

	/** Traditional to simplified unicode conversion table */
	DLTDB_UI16_NODE           * pTrad2Simp;
	/** Simplified to traditional unicode conversion table */
	DLTDB_UI16_NODE           * pSimp2Trad;

	/** Unicode includion table. Bitvector of size 0xFFFF, bit number N represents
	    the unicode code point N. The bit is set iff the unicode is present in the 
		database. */
	DECUMA_UINT32             * pUnicodeVector;

	/** List of pairs (unicode, key index) sorted by unicode. Used to perform a
	    binary search for keys by unicode */
	DECUMA_UINT16             * pUnicode2IndexList;

	/** @} */


	/** @name Cluster nodes
	    @{
	*/
	DLTDB_COARSE_CLUSTER_TREE   dense;
	DLTDB_COARSE_CLUSTER_TREE   sparse;
	/** @} */


#ifdef EXTENDED_DATABASE
	/** @name Extended database content
	    @{ 
	*/
	DLTDB_FEATURE_TABLE         denseFeatures;
	DLTDB_FEATURE_TABLE         sparseFeatures;

	DECUMA_UINT8              * pStrokeOrder;
	DECUMA_UINT8              * pType;
	DLTDB_INDEX_NODE          * pSamples;
	/** @} */
#endif

	/** @name Prefetched Constants
	    Constants read from the database
	    @{
	*/

	/** Indices in pCompData of known components */
	KNOWN_COMPONENTS            knownComponents;
	/** Database language identifier */
	char                        language;
	/** Max number of strokes in any character */
	DECUMA_INT32                maxnstr;
	/** Highest index in pCharData */
	DECUMA_INT32                maxindex;
	/** Max number of points in a character */
	DECUMA_INT32                maxnptsperchar;
	/** Max number of points in a stroke */
	DECUMA_INT32                maxnptsperstr;
	/** Component indices uses two bytes if the first byte is == firsttwobyteindex  */
	DECUMA_INT32                firsttwobyteindex;
	/** Max number of strokes in a dense sampled character */
	DECUMA_INT32                latinlimit;
	/** Bitmask describing the atomic character categories present in the database */
	DECUMA_INT32                categoryMask;
	/** Bitmask describing the atomic writing styles present in the database */
	DECUMA_INT32                writingStyleMask;
	/** ...? */
	DECUMA_INT32                nLengthLimit;
	/** Numer of unicode code points the database covers */
	DECUMA_INT32                nUnicodes;
	/** Numer of template keys in the database */
	DECUMA_INT32                nKeys;

	/** @} */

};

/** @} */

#endif /* CJK_DATABASE_FORMAT_TYPES_H_ */
