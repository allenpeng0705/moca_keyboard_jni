/*****************************************************************************
* COPYRIGHT ZI AND SUBJECT TO CONFIDENTIALITY RESTRICTIONS                   *
*                                                                            *
* This file is the confidential and proprietary property of Zi Corporation   *
* of Canada, Inc. ("Zi") or one of its affiliates.                           *
******************************************************************************/
#ifndef CJK_DATABASE_FORMAT_H_
#define CJK_DATABASE_FORMAT_H_

#pragma once

#include "dltConfig.h"

#include "cjkCommon.h"
#include "cjkDatabaseLimits.h"
#include "decumaBasicTypes.h"


#define DB_VERSION                                    4
#define DB_SUBVERSION                                 2

#define DLTDB_COARSE_CLUSTER_TREE_INVALID_INDEX       0xFFFF

/* turn on the cluster key bandwith filter to speed up the performance */
#define USE_CLUSTER_BANDWIDTH_FILTER

/** @defgroup DOX_CJK_DATABASE_FORMAT cjkDatabaseFormat
  * Defines database access functions, macros and types.
  * @{ */

/** @name Head index node entries 
  * @{ */

#define DBNODENAME_COPYRIGHT                     "COPYRIGHT"              /**< @hideinitializer */
#define DBNODENAME_VERSION_STR                   "VERSION_STR"            /**< @hideinitializer */
#define DBNODENAME_VERSION                       "VERSION"                /**< @hideinitializer */
#define DBNODENAME_LABEL                         "LABEL"                  /**< @hideinitializer */
#define DBNODENAME_CONSTANTS                     "CONSTANTS"              /**< @hideinitializer */
#define DBNODENAME_COMPONENTS                    "COMPONENTS"             /**< @hideinitializer */
#define DBNODENAME_CHARACTERS                    "CHARACTERS"			  /**< @hideinitializer */
#define DBNODENAME_CATEGORY                      "CATEGORY"				  /**< @hideinitializer */
#define DBNODENAME_WRITING_STYLE                 "WRITING_STYLE"		  /**< @hideinitializer */
#define DBNODENAME_UNICODE                       "UNICODE"				  /**< @hideinitializer */
#define DBNODENAME_CATANDATTRIB_LIST             "CATANDATTRIB_LIST"      /**< @hideinitializer */
#define DBNODENAME_ATTRIBUTE                     "ATTRIBUTE"              /**< @hideinitializer */
#define DBNODENAME_TRAD2SIMP                     "TRAD2SIMP"			  /**< @hideinitializer */
#define DBNODENAME_SIMP2TRAD                     "SIMP2TRAD"			  /**< @hideinitializer */
#define DBNODENAME_UNICODE_VECTOR                "UNICODE_VECTOR"		  /**< @hideinitializer */
#define DBNODENAME_UNICODE2INDEX_TBL             "UNICODE2INDEX_TBL"	  /**< @hideinitializer */
#define DBNODENAME_INDEXLIST_OFFSET_END          "INDEXLIST_OFFSET_END"	  /**< @hideinitializer */
#define DBNODENAME_INDEXLIST_VECTOR              "INDEXLIST_VECTOR"	  	  /**< @hideinitializer */
#define DBNODENAME_DENSE_CLUSTER_TREE            "DENSE_CLUSTER_TREE"	  /**< @hideinitializer */
#define DBNODENAME_SPARSE_CLUSTER_TREE           "SPARSE_CLUSTER_TREE"	  /**< @hideinitializer */

/** @} */


/** @name Cluster index node entries
  * @{ */

#define DBNODENAME_CLUSTERS                      "CLUSTERS"               /**< @hideinitializer */
#define DBNODENAME_CLUSTER_TYPE                  "CLUSTER_TYPE"			  /**< @hideinitializer */
#define DBNODENAME_NCHARS_IN_CLUSTER             "NCHARS_IN_CLUSTER"	  /**< @hideinitializer */
#define DBNODENAME_CLUSTER_LEFT_CHILD            "CLUSTER_LEFT_CHILD"	  /**< @hideinitializer */
#define DBNODENAME_CLUSTER_RIGHT_CHILD           "CLUSTER_RIGHT_CHILD"	  /**< @hideinitializer */
#define DBNODENAME_NCHILDREN_UPTO_CLUSTER        "NCHILDREN_UPTO_CLUSTER" /**< @hideinitializer */
#define DBNODENAME_CLUSTER_CHILD_INDICES         "CLUSTER_CHILD_INDICES"  /**< @hideinitializer */
#define DBNODENAME_CLUSTER_PARENT_INDICES        "CLUSTER_PARENT_INDICES" /**< @hideinitializer */
#define DBNODENAME_CLUSTER_REPRESENTATIVE        "CLUSTER_REPRESENTATIVE" /**< @hideinitializer */
#define DBNODENAME_CLUSTER_CHOSEN_INDICES        "CLUSTER_CHOSEN_INDICES" /**< @hideinitializer */
#define DBNODENAME_NFEATURES_UPTO_CLUSTER        "NFEATURES_UPTO_CLUSTER" /**< @hideinitializer */
#define DBNODENAME_CLUSTER_FEATURE_INDICES       "CLUSTER_FEATURE_INDICES"/**< @hideinitializer */
#define DBNODENAME_CLUSTER_FEATURE_VALUES        "CLUSTER_FEATURE_VALUES" /**< @hideinitializer */
#define DBNODENAME_CLUSTER_INDEX_LIST_LIMIT      "INDEX_LIST_LIMIT"		  /**< @hideinitializer */
																		  
#define DBNODENAME_DCS_CLUSTERS                  "DCS_CLUSTERS"               /**< @hideinitializer */
#define DBNODENAME_CLUSTER_KEY_INDICES           "CLUSTER_KEY_INDICES"		  /**< @hideinitializer */
#define DBNODENAME_CLUSTER_CHOSEN_FEATURES       "CLUSTER_CHOSEN_FEATURES"	  /**< @hideinitializer */
#define DBNODENAME_CLUSTER_FEATURE_REP           "CLUSTER_FEATURE_REP"		  /**< @hideinitializer */
#define DBNODENAME_CLUSTER_FEATURE_MEANS         "CLUSTER_FEATURE_MEANS"	  /**< @hideinitializer */
#define DBNODENAME_CLUSTER_FEATURE_STD	         "CLUSTER_FEATURE_STD"		  /**< @hideinitializer */
#define DBNODENAME_CLUSTER_FEATURE_WEIGHT	     "CLUSTER_FEATURE_WEIGHT"	  /**< @hideinitializer */

#if defined(USE_CLUSTER_BANDWIDTH_FILTER)
#define DBNODENAME_CLUSTER_KEY_INDICES_TBL       "CLUSTER_KEY_INDICES_TBL"
#endif
/** @} */


/** @name Constants
  * @{ */

#define DBNODENAME_MAXNSTRK                      "MAXNSTRK"             /**< @hideinitializer */
#define DBNODENAME_MAXNPTSPERCHAR                "MAXNPTSPERCHAR"		/**< @hideinitializer */
#define DBNODENAME_MAXNPTSPERSTROKE              "MAXNPTSPERSTROKE"		/**< @hideinitializer */
#define DBNODENAME_FIRSTTWOBYTEINDEX             "FIRSTTWOBYTEINDEX"	/**< @hideinitializer */
#define DBNODENAME_LATINLIMIT                    "LATINLIMIT"			/**< @hideinitializer */
#define DBNODENAME_CATEGORYMASK                  "CATEGORYMASK"			/**< @hideinitializer */
#define DBNODENAME_WRITINGSTYLE_MASK             "WRITINGSTYLE_MASK"	/**< @hideinitializer */
#define DBNODENAME_LENGTH_LIMIT                  "LENGTH_LIMIT"			/**< @hideinitializer */
#define DBNODENAME_N_UNICODES_IN_DB              "N_UNICODES_IN_DB"		/**< @hideinitializer */
#define DBNODENAME_N_KEYS_IN_DB		             "N_KEYS_IN_DB"			/**< @hideinitializer */

/** @} */


/** @name Known components' indices
  * @{ */

#define DBNODENAME_SPEAKINGS_L                   "SPEAKINGS_L"           /**< @hideinitializer */
#define DBNODENAME_SPEAKINGS_L2                  "SPEAKINGS_L2"			 /**< @hideinitializer */
#define DBNODENAME_SPEAKINGS_L3                  "SPEAKINGS_L3"			 /**< @hideinitializer */
#define DBNODENAME_THREEDROPS_L                  "THREEDROPS_L"			 /**< @hideinitializer */
#define DBNODENAME_THREEDROPS_L2                 "THREEDROPS_L2"		 /**< @hideinitializer */
#define DBNODENAME_THREEDROPS_L3                 "THREEDROPS_L3"         /**< @hideinitializer */
#define DBNODENAME_THREEDROPS_LD                 "THREEDROPS_LD"		 /**< @hideinitializer */
#define DBNODENAME_THREEDROPS_LD2                "THREEDROPS_LD2"		 /**< @hideinitializer */
#define DBNODENAME_THREEDROPS_I                  "THREEDROPS_I"			 /**< @hideinitializer */
#define DBNODENAME_EARTH_L                       "EARTH_L"				 /**< @hideinitializer */

/** @} */


/** @name Available in an extended database
  * @{ */

#ifdef EXTENDED_DATABASE
	#define DBNODENAME_STROKE_ORDER              "STROKE_ORDER"         /**< @hideinitializer */
	#define DBNODENAME_TYPE                      "TYPE"					/**< @hideinitializer */
																		/**< @hideinitializer */
	/* features */														/**< @hideinitializer */
	#define DBNODENAME_DENSE_FEATURES            "DENSE_FEATURES"		     /**< @hideinitializer */
	#define DBNODENAME_SPARSE_FEATURES           "SPARSE_FEATURES"			 /**< @hideinitializer */
	#define DBNODENAME_FEATURE_BYTES_PER_FEATURE "FEATURE_BYTES_PER_FEATURE" /**< @hideinitializer */
	#define DBNODENAME_FEATURE_KEY_INDEX         "FEATURE_KEY_INDEX"		 /**< @hideinitializer */
																			 
	/* samples */
	#define	DBNODENAME_SAMPLES                   "SAMPLES"         /**< @hideinitializer */
#endif

/** @} */
/** @} */




/** @name Database Node Data Types
  * @{ */

/** An 8-bit data node with an initial element count */
typedef struct _tagDLTDB_UI8_NODE
	DLTDB_UI8_NODE ;

/** A 16-bit data node with an initial element count */
typedef struct _tagDLTDB_UI16_NODE
	DLTDB_UI16_NODE ;

/** A 32-bit data node with an initial element count */
typedef struct _tagDLTDB_UI32_NODE
	DLTDB_UI32_NODE ;

/** An index node with an initial element count */
typedef struct _tagDLTDB_INDEX_NODE
	DLTDB_INDEX_NODE;

/** An index node with named entries and an initial element count */
typedef struct _tagDLTDB_NAMED_INDEX_NODE
	DLTDB_NAMED_INDEX_NODE;

/** A 32-bit data node with named entries and an initial element count */
typedef DLTDB_NAMED_INDEX_NODE 
	DLTDB_NAMED_UI32_NODE;

/** An entry in a @ref DLTDB_NAMED_INDEX_NODE */
typedef struct _tagDLTDB_INDEX_NODE_NAMED_ENTRY
	DLTDB_INDEX_NODE_NAMED_ENTRY;

/** An entry in a @ref DLTDB_NAMED_UI32_NODE */
typedef DLTDB_INDEX_NODE_NAMED_ENTRY 
	DLTDB_UI32_NODE_NAMED_ENTRY;

/** @} */



/** @name The database type and some types contained in it
  * @{ */

/** Contains all information needed for database access; initialized in @ref cjkDbInit, 
    or indirectly by @ref cjkSessionInit.
	@see DOX_DLTDB_BIN_FORMAT
	*/
typedef struct _tagDLTDB
	DLTDB;

#ifdef EXTENDED_DATABASE
	typedef struct _tagDLTDB_FEATURE_TABLE
		DLTDB_FEATURE_TABLE;
#endif


typedef struct _tagKNOWN_COMPONENTS
	KNOWN_COMPONENTS;

/** @} */




/** The distance measure type */
typedef double DLTDB_COARSE_DCS_DIST_TYPE;



#endif /* CJK_DATABASE_FORMAT_H_ */
