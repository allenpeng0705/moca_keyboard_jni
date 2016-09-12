/*******************************************************************************
;*******************************************************************************
;**                                                                           **
;**                  COPYRIGHT 2001-2011 NUANCE COMMUNICATIONS                **
;**                                                                           **
;**               NUANCE COMMUNICATIONS PROPRIETARY INFORMATION               **
;**                                                                           **
;**     This software is supplied under the terms of a license agreement      **
;**     or non-disclosure agreement with Nuance Communications and may not    **
;**     be copied or disclosed except in accordance with the terms of that    **
;**     agreement.                                                            **
;**                                                                           **
;**                                                                           **
;*******************************************************************************
;**                                                                           **
;**     FileName: et9api.h                                                    **
;**                                                                           **
;**  Description: ET9 API Interface Header File.                              **
;**               Conforming to the development version of ET9                **
;**                                                                           **
;*******************************************************************************
;******************************************************************************/

#ifndef ET9API_H
#define ET9API_H 1

/*! \addtogroup et9api API for XT9 Input
* The API for generic XT9.
* @{
*/

/* don't mangle the function name if compile under C++ */
#if defined(__cplusplus)
extern "C" {
#endif


/*----------------------------------------------------------------------------
 * include file to override ET9 defaults.  If using ET9 defaults, this
 * will be an empty file
 *----------------------------------------------------------------------------*/
#include "xxet9oem.h"

#ifdef ET9_KDB_MODULE
/*----------------------------------------------------------------------------
 *  Activate direct KDB access
 *----------------------------------------------------------------------------*/
#ifndef ET9_DIRECT_KDB_ACCESS
#define ET9_DIRECT_KDB_ACCESS
#endif

#if defined(ET9_DEACTIVATE_DIRECT_KDB_ACCESS)
#undef ET9_DIRECT_KDB_ACCESS
#endif
#endif /* ET9_KDB_MODULE */

/*----------------------------------------------------------------------------
 *  Activate direct LDB access
 *----------------------------------------------------------------------------*/
#ifndef ET9_DIRECT_LDB_ACCESS
#define ET9_DIRECT_LDB_ACCESS
#endif

#if defined(ET9_DEACTIVATE_DIRECT_LDB_ACCESS)
#undef ET9_DIRECT_LDB_ACCESS
#endif

/*----------------------------------------------------------------------------
 *  Define symbol encoding
 *----------------------------------------------------------------------------*/

#if defined(ET9SYMBOLENCODING_SHIFTJIS)
#ifdef ET9SYMBOLENCODING_UNICODE
#error multiple symbol encodings defined
#endif
#else
#define ET9SYMBOLENCODING_UNICODE
#endif

/*----------------------------------------------------------------------------
 *  Define fundamental boolean
 *----------------------------------------------------------------------------*/

#ifndef ET9BOOL
#define ET9BOOL    char                 /**< Anything that fits the architecture. */
#endif

/*----------------------------------------------------------------------------
 *  Define fundamental 8, 16, and 32 bit quantities
 *----------------------------------------------------------------------------*/

#ifndef ET9S8
#define ET9S8      signed char          /**< Signed  8-bit quantity. */
#endif
#ifndef ET9S16
#define ET9S16     signed short         /**< Signed 16-bit quantity. */
#endif
#ifndef ET9S32
#define ET9S32     signed long          /**< Signed 32-bit quantity. */
#endif

/*----------------------------------------------------------------------------
 *  Define fundemental unsigned 8, 16, and 32 bit quantities
 *----------------------------------------------------------------------------*/

#ifndef ET9U8
#define ET9U8      unsigned char        /**< Unsigned  8-bit quantity. */
#endif
#ifndef ET9U16
#define ET9U16     unsigned short       /**< Unsigned 16-bit quantity. */
#endif
#ifndef ET9U32
#define ET9U32     unsigned long        /**< Unsigned 32-bit quantity. */
#endif


/*----------------------------------------------------------------------------
 *  Define the ET9  ET9SYMBOLWIDTH
 *----------------------------------------------------------------------------*/

#define ET9SYMB  ET9U16
#define ET9SYMBOLWIDTH   2                                              /**< Width of XT9 symbols. */

#define ET9SYMBMAXVAL    ((ET9SYMB)((1 << (sizeof(ET9SYMB)*8)) - 1))    /**< \internal unicode symbol max val */


/*----------------------------------------------------------------------------
 *  Define the data element for general numbers
 *----------------------------------------------------------------------------*/

#ifndef ET9INT
#define ET9INT      signed int          /**< Should be the most natural compiler type ('int' is 16 bits or more by ANSI definition). */
#endif

#ifndef ET9UINT
#define ET9UINT     unsigned int        /**< Should be the most natural compiler type ('int' is 16 bits or more by ANSI definition). */
#endif

/*----------------------------------------------------------------------------
 *  Define the data element for floating point
 *----------------------------------------------------------------------------*/

#ifndef ET9FLOAT
#define ET9FLOAT float
#endif

#ifndef ET9DOUBLE
#define ET9DOUBLE double
#endif

/*----------------------------------------------------------------------------
 *  Define function and data prefixes
 *----------------------------------------------------------------------------*/

#ifndef ET9FARCALL
#define ET9FARCALL                      /**< Functions that can be called from any module. */
#endif /* ET9FARCALL */

#ifndef ET9LOCALCALL
#define ET9LOCALCALL                    /**< Functions local to module. */
#endif /* ET9LOCALCALL */

#ifndef ET9FARDATA
#define ET9FARDATA                      /**< Constant romable data blocks. */
#endif /* ET9FARDATA */

/*----------------------------------------------------------------------------
 *  Float frequency control
 *----------------------------------------------------------------------------*/

#ifndef ET9_DEACTIVATE_USE_FLOAT_FREQS
#define ET9_USE_FLOAT_FREQS             /**< If using float for frequency calculations. */
#endif

#ifdef ET9_USE_FLOAT_FREQS
#define ET9FREQ         ET9FLOAT
#define ET9FREQPART     ET9FLOAT
#else
#define ET9FREQ         ET9U32
#define ET9FREQPART     ET9U16
#endif

/**
 * Speed option for important functions.
 * It's really important that the FORCED inlining is applied for all compilers, as it will have a big impact on performance.
 * If the default value isn't the correct one for a specific compiler, find it and assign it in a file like xxet9oem.h.
 *
 * (Simple __inline options are not useful since they are not forced, and thus, usually disregarded by the compiler.)
 */

#ifndef ET9INLINE

#ifdef ET9_DEBUG
#define ET9INLINE
#else

#if defined(_MSC_VER) && (_MSC_VER < 1200)
/* old MSC inlining */
#define ET9INLINE  __inline

#elif defined(__GNUC__) && ((__GNUC__ > 3) || ((__GNUC__ == 3) && (__GNUC_MINOR__ >= 2)))
/* GCC inlining */
#define ET9INLINE   __attribute__ ((always_inline))

#elif defined(__ARMCC_VERSION) && (__ARMCC_VERSION < 210000)
/* ADS
 * NOTE! This must be used in conjunction with the compiler flag --no_inlinemax
 * for inlining to have any effect !*/
#define ET9INLINE __inline

#elif defined(__TI_COMPILER_VERSION__)
/* TI inlining */

/* NOTE: This must be used in conjunction with -o3 compiler option
 * for inlining to have any effect */

#define ET9INLINE __inline

#else /* default */

/* No definition of inlining, set to default inlining (applicable to several compilers).
 * It will either work or create an error. The error is necessary to force the investigation!!! */
/* WIN32/CL inlining, also ARMCC */

#define ET9INLINE  __forceinline /* NEVER CHANGE THE DEFAULT TO ANYTHING OTHER THAN __forceinline! */

#endif
#endif /* ET9_DEBUG */
#endif /* ET9INLINE */

/******************************************************************************/

/*----------------------------------------------------------------------------
 *  Define ET9 core version number
 *
 *  String format is defined as "XT9 VMM.mm.bf.qa"
 *  Number format is defined as 0xMMmmbfqa. Number is in hexadecimal
 *  Where
 *      MM   = major version number
 *      mm   = minor version number
 *      bf   = bug fix version number
 *      qa   = QA release version number
 *  Update ET9COREVER defn. if any component version # goes beyond 2 digits.
 *----------------------------------------------------------------------------*/

#define ET9MAJORVER   "7"
#define ET9MINORVER   "1"
#define ET9PATCHVER   "0"
#define ET9RCVER      "0"
#define ET9COREVER   "XT9 V0" ET9MAJORVER ".0" ET9MINORVER ".0" ET9PATCHVER ".0" ET9RCVER

#define ET9COREVERSIONNUM  0x07010000

/* define ET9 compatibility index for keyboard core and KDB databases */

#define ET9COMPATIDKDBXBASEALPH     1
#define ET9COMPATIDKDBXOFFSETALPH   0

    /*------------------------------------------------------------------------
     *  Define ET9 compatibility index related macros.
     *------------------------------------------------------------------------*/

/** \internal
 * max number of offsets each core can be compatible with (KDB)
 */

#define ET9MAXCOMPATIDKDBXOFFSET    16

/** \internal
 * convert the compatibility ID offset to the bit mask value
 */

#define ET9MASKOFFSET(CompatIdxOffset)  (ET9U16)(1L << (CompatIdxOffset - 1))


/**
 * ET9 error codes returned by ET9 APIs
 */

typedef enum ET9STATUS_e {
    ET9STATUS_NONE = 0,                     /**< 00 : No errors encountered (guaranteed to be zero) */
    ET9STATUS_ERROR,                        /**< 01 : General error status */
    ET9STATUS_NO_INIT,                      /**< 02 : System initialization is required */
    ET9STATUS_ABORT,                        /**< 03 : Abort ST system */
    ET9STATUS_NO_MATCHING_WORDS,            /**< 04 : No word match found */
    ET9STATUS_FULL,                         /**< 05 : Buffer full */
    ET9STATUS_EMPTY,                        /**< 06 : Buffer empty */
    ET9STATUS_OUT_OF_RANGE,                 /**< 07 : Out of Range */
    ET9STATUS_NO_MEMORY,                    /**< 08 : Required memory */
    ET9STATUS_INVALID_MEMORY,               /**< 09 : Invalid memory */
    ET9STATUS_READ_DB_FAIL,                 /**< 10 : Unable to read DB */
    ET9STATUS_WRITE_DB_FAIL,                /**< 11 : Unable to write DB */
    ET9STATUS_DB_CORE_INCOMP,               /**< 12 : The loaded database is not compatible with the core */
    ET9STATUS_WRONG_OEMID,                  /**< 13 : Wrong OEM ID */
    ET9STATUS_LDB_VERSION_ERROR,            /**< 14 : LDB Version mismatch */
    ET9STATUS_KDB_VERSION_ERROR,            /**< 15 : Keyboard Version mismatch */
    ET9STATUS_LDB_ID_ERROR,                 /**< 16 : Requested LDB and LDB data ID mismatch */
    ET9STATUS_INVALID_KDB_PAGE,             /**< 17 : Invalid keyboard page number */
    ET9STATUS_KDB_OUT_OF_RANGE,             /**< 18 : Out of range keyboard tap position */
    ET9STATUS_NO_KEY,                       /**< 19 : No key in the current tap */
    ET9STATUS_WORD_EXISTS,                  /**< 20 : Word exist */
    ET9STATUS_NO_MATCH,                     /**< 21 : No matching words */
    ET9STATUS_CORRUPT_DB,                   /**< 22 : Corrupted DB */
    ET9STATUS_INVALID_DB_TYPE,              /**< 23 : Invalid database type */
    ET9STATUS_NO_OPERATION,                 /**< 24 : No operation */
    ET9STATUS_NO_CDB,                       /**< 25 : No CDB */
    ET9STATUS_INVALID_SIZE,                 /**< 26 : Invalid DB data size */
    ET9STATUS_BAD_PARAM,                    /**< 27 : Bad Parameter passed */
    ET9STATUS_ALREADY_INITIALIZED,          /**< 28 : DB already initialized */
    ET9STATUS_DB_NOT_ENOUGH_MEMORY,         /**< 29 : Not enough mem to add entry */
    ET9STATUS_DB_NOT_ACTIVE,                /**< 30 : DB is inactive */
    ET9STATUS_BUFFER_TOO_SMALL,             /**< 31 : Passed buffer too small to hold entry */
    ET9STATUS_NO_RUDB,                      /**< 32 : RUDB call made with no RUDB defined */
    ET9STATUS_WORD_NOT_FOUND,               /**< 33 : Word search came up empty */
    ET9STATUS_NEED_SELLIST_BUILD,           /**< 34 : Need to build selection list */
    ET9STATUS_INVALID_TEXT,                 /**< 35 : Invalid syms found in passed text */
    ET9STATUS_OUT_OF_RANGE_MAXALTSYMBS,     /**< 36 : Too many alt symbs */
    ET9STATUS_OUT_OF_RANGE_MAXBASESYMBS,    /**< 37 : Too many base symbs */
    ET9STATUS_NO_ASDB,                      /**< 38 : ASDB call made with no ASDB defined */
    ET9STATUS_NO_LDBAS_FOUND,               /**< 39 : No AutoSubstitution record exists for requested LDB */
    ET9STATUS_NO_CHAR,                      /**< 40 : Character not found(not displayable) */
    ET9STATUS_NO_DB_INIT,                   /**< 41 : Language/Keyboard database not set for module */
    ET9STATUS_INVALID_MODE,                 /**< 42 : This operation is not support for this mode */
    ET9STATUS_INVALID_INPUT,                /**< 43 : Input is invalid */
    ET9STATUS_DB_CHANGED_WARNING,           /**< 44 : Db changed since previous calls, old data may be invalid */
    ET9STATUS_TYPE_ERROR,                   /**< 45 : Type error, e.g. with fundamental type definitions */
    ET9STATUS_MATH_ERROR,                   /**< 46 : Math error, e.g. with fundamental math function */
    ET9STATUS_NO_LDB,                       /**< 47 : LDB does not exist */
    ET9STATUS_KDB_MISMATCH,                 /**< 48 : KDBs mismatch in bilingual */
    ET9STATUS_SETTING_SAME_LDBS,            /**< 49 : Cannot have same first and second language databases */
    ET9STATUS_INVALID_KDB_NUM,              /**< 50 : Invalid keyboard number */
    ET9STATUS_NEED_KDB_TO_LOAD_PAGE,        /**< 51 : Need a valid keyboard number to load page */
    ET9STATUS_NOT_SUPPORTED_BY_ENCODING,    /**< 52 : This feature is not supported by the current encoding */
    ET9STATUS_EVAL_BUILD_EXPIRED,           /**< 53 : Evaluation build has expired */
    ET9STATUS_CHARPROP_ERROR,               /**< 54 : CharProp error, likely caused by compiler error */
    ET9STATUS_NO_LM,                        /**< 55 : LM not set */
    ET9STATUS_KDB_PAGE_HAS_NO_KEYS,         /**< 56 : KDB page contains no keys */
    ET9STATUS_KDB_HAS_NO_TRACEABLE_KEYS,    /**< 57 : KDB contains no keys suitable for tracing */
    ET9STATUS_KDB_HAS_TOO_MANY_KEYS,        /**< 58 : KDB contains too many keys */
    ET9STATUS_KDB_HAS_TOO_MANY_CHARS,       /**< 59 : KDB has too many key chars */
    ET9STATUS_KDB_KEY_HAS_TOO_FEW_CHARS,    /**< 60 : KDB key has too few key chars */
    ET9STATUS_KDB_KEY_HAS_TOO_MANY_CHARS,   /**< 61 : KDB key has repeat/recurring chars */
    ET9STATUS_KDB_KEY_HAS_REPEAT_CHARS,     /**< 62 : KDB key has too many key chars */
    ET9STATUS_TOO_LONG_SUBSTITUTIONS,       /**< 63 : Substitutions can be longer than a word */
    ET9STATUS_SETTINGS_INHIBITED,           /**< 64 : In a state where settings are inhibited - e.g. during reselect */
    ET9STATUS_KDB_IS_LOADING,               /**< 65 : In a state where only KDB loading is allowed */
    ET9STATUS_KDB_IS_NOT_LOADING,           /**< 66 : In a state where KDB loading is not allowed */
    ET9STATUS_KDB_WRONG_LOAD_STATE,         /**< 67 : In a state where this event wasn't expected */
    ET9STATUS_KDB_REPEAT_LOAD_ATTACH,       /**< 68 : Trying to attach the same information more than once */
    ET9STATUS_KDB_KEY_OUTSIDE_KEYBOARD,     /**< 69 : The key region is outside the keyboard layout */
    ET9STATUS_KDB_KEY_OVERLAP,              /**< 70 : The key region overlaps another key region */
    ET9STATUS_KDB_INCORRECT_TYPE_FOR_KEY,   /**< 71 : The top character doesn't match the key type */
    ET9STATUS_KDB_ID_MISMATCH,              /**< 72 : Keyboard ID/language mismatch */
    ET9STATUS_KDB_INCONSISTENT_PAGE_COUNT,  /**< 73 : Keyboard has inconsistent page count */
    ET9STATUS_KDB_SYNTAX_ERROR,             /**< 74 : Keyboard has syntax error */
    ET9STATUS_KDB_DUPLICATE_ATTRIBUTE,      /**< 75 : Keyboard entity has a duplicate attribute */
    ET9STATUS_KDB_UNEXPECTED_ATTRIBUTE,     /**< 76 : Keyboard entity has an unexpected attribute */
    ET9STATUS_KDB_MISSING_ATTRIBUTE,        /**< 77 : Keyboard entity has a missing attribute */
    ET9STATUS_KDB_ATTRIBUTE_VALUE_ERROR,    /**< 78 : Keyboard has an attribute value error */
    ET9STATUS_KDB_HAS_TOO_FEW_ROWS,         /**< 79 : Keyboard has too few rows */
    ET9STATUS_KDB_HAS_TOO_MANY_ROWS,        /**< 80 : Keyboard has too many rows */
    ET9STATUS_KDB_ROW_HAS_TOO_FEW_KEYS,     /**< 81 : Keyboard row has too few keys */
    ET9STATUS_KDB_UNEXPECTED_CONTENT,       /**< 82 : Keyboard has an unexpected mix of content */
    ET9STATUS_KDB_KEY_INDEX_ALREADY_USED,   /**< 83 : Keyboard key index already used */
    ET9STATUS_KDB_WRONG_CONTENT_ENCODING,   /**< 84 : Keyboard content encoding not supported */
    ET9STATUS_KDB_PAGE_NOT_FOUND,           /**< 85 : The keyboard page was not found */
    ET9STATUS_KDB_BAD_LAYOUT_SIZE,          /**< 86 : The keyboard layout size is bad */
    ET9STATUS_KDB_BAD_PAGE_COUNT,           /**< 87 : The keyboard page count is bad */
    ET9STATUS_KDB_KEY_BAD_REGION,           /**< 88 : The keyboard key region is bad */
    ET9STATUS_KDB_WM_ERROR,                 /**< 89 : The keyboard internal working memory conflict */

/* Chinese-specific status codes. */

#ifdef ET9_CHINESE_MODULE
    ET9STATUS_SELECTED_CHINESE_COMPONENT = 100, /**< 100: the selected item is a component */
    ET9STATUS_ALL_SYMB_SELECTED,                /**< 101: all symbols are selected */
    ET9STATUS_TRACE_NOT_AVAILABLE,              /**< 102: trace not activated or no trace db available */
#endif

#ifdef ET9_NAV_ALPHABETIC_MODULE
    /* invalid/unint struct */
    ET9STATUS_NAV_INVALID_CORE = 200,                   /**< ET9NAVCore struct not initialized or invalid */
    ET9STATUS_NAV_INVALID_INDEX,                        /**< ET9NAVIndex struct not initialized or invalid */
    ET9STATUS_NAV_INVALID_RECORD,                       /**< ET9NAVRecord struct not initialized or invalid */
    ET9STATUS_NAV_INVALID_QUERYSTATE,                   /**< ET9NAVQueryState struct not initialized or invalid */
    ET9STATUS_NAV_INVALID_INPUT,                        /**< ET9NAVInput struct not initialized or invalid */
    ET9STATUS_NAV_INVALID_RESULTS,                      /**< ET9NAVResults struct not initialized or invalid */
    ET9STATUS_NAV_INVALID_MATCHHANDLE,                  /**< ET9NAVMatchHandle struct invalid */
    ET9STATUS_NAV_INVALID_TYPEINFO,                     /**< ET9NAVTypeInfo struct not initialized or invalid */
    /* out of range */
    ET9STATUS_NAV_INDEX_ID_OUT_OF_RANGE,                /**< IndexID exceeds ET9NAVMAX_INDEX_COUNT */
    ET9STATUS_NAV_TYPE_ID_OUT_OF_RANGE,                 /**< One or more Type IDs exceeds ET9NAVMAX_TYPE_COUNT */
    /* invalid storage specified */
    ET9STATUS_NAV_DISPLAY_STR_STORAGE_INVALID,          /**< Invalid display string storage location specified. */
    ET9STATUS_NAV_SECONDARY_FIELDS_STORAGE_INVALID,     /**< Invalid record key storage location specified. */
    ET9STATUS_NAV_USER_DATA_STORAGE_INVALID,            /**< Invalid user data storage location specified. */
    /* out of mem */
    ET9STATUS_NAV_INDEX_OUT_OF_MEM,                     /**< Index out of memory */
    ET9STATUS_NAV_SYMB_BUFFER_OUT_OF_MEM,               /**< Symbol buffer out of memory */
    ET9STATUS_NAV_OUT_OF_RECORD_SYMBS,                  /**< Record has too many symbols (configuration) */
    ET9STATUS_NAV_OUT_OF_WORKING_MEM,                   /**< Info won't fit the working memory */
    /* corruption - garbage or invalid values read */
    ET9STATUS_NAV_INDEX_CORRUPTED,                      /**< Corrupt index content */
    /* memory operations */
    ET9STATUS_NAV_MEM_ALLOC_ERROR,                      /**< PAL memory allocation failed */
    ET9STATUS_NAV_MEM_INVALID_MEMORY_CEILING,           /**< Memory ceiling is less than ET9NAV_MIN_RAM_CEILING */
    /* PAL file operations */
    ET9STATUS_NAV_FILE_READ_ERROR,                      /**< Error reading from file */
    ET9STATUS_NAV_FILE_READ_ERROR_FATAL,                /**< Error reading from file; File/Index may be in undefined state; Reindexing required */
    ET9STATUS_NAV_FILE_ERROR,                           /**< File error */
    ET9STATUS_NAV_FILE_REMOVE_ERROR,                    /**< Couldn't remove a file */
    ET9STATUS_NAV_FILE_ERROR_FATAL,                     /**< File error; File/Index may be in undefined state; Reindexing required */
    ET9STATUS_NAV_FILE_WRITE_ERROR,                     /**< Error writing to file */
    ET9STATUS_NAV_FILE_IN_USE,                          /**< File required by one or more mounted indexes */
    ET9STATUS_NAV_FILE_OPEN_ERROR,                      /**< Error opening file */
    ET9STATUS_NAV_FILE_NOT_OPENED,                      /**< file not opened */
    ET9STATUS_NAV_READ_OR_WRITE_FUNC_NOT_SPECIFIED,     /**< Read and or write callback functions not specified. */
    /* not mounted */
    ET9STATUS_NAV_INDEX_NOT_MOUNTED,                    /**< Index not mounted */
    ET9STATUS_NAV_TYPE_NOT_MOUNTED,                     /**< Type not mounted  */
    ET9STATUS_NAV_INDEX_UNSAFE_UNMOUNT,                 /**< Index unsafely unmounted; index is still mounted, but may require reindexing */
    /* already mounted */
    ET9STATUS_NAV_INDEX_ALREADY_MOUNTED,                /**< Index already mounted */
    ET9STATUS_NAV_TYPE_ALREADY_MOUNTED,                 /**< Type already mounted  */
    ET9STATUS_NAV_EXCEEDS_MAX_REC_KEY_LENGTH,           /**< Record key length exceeds ET9NAVMAX_RECORD_KEY_LENGTH */
    ET9STATUS_NAV_EXCEEDS_MAX_USER_DATA_LENGTH,         /**< User data length exceeds ET9NAVMAX_USER_DATA_LENGTH */
    /* not stored */
    ET9STATUS_NAV_DISPLAY_STRING_NOT_STORED,            /**< Display string not stored */
    ET9STATUS_NAV_USER_DATA_NOT_STORED,                 /**< User data not stored */
    /* record existence */
    ET9STATUS_NAV_RECORD_DOES_NOT_EXIST,                /**< Record does not exist in index */
    ET9STATUS_NAV_RECORD_ALREADY_EXISTS,                /**< Record already exist in index */
    /* record validation */
    ET9STATUS_NAV_NO_FIELDS_SPECIFIED,                  /**< No fields specified in ET9NAVRecord; A record must contain one or more fields */
    ET9STATUS_NAV_EXCEEDS_MAX_FIELD_COUNT,              /**< ET9NAVRecord is limited to ET9NAVMAX_FIELD_COUNT fields */
    ET9STATUS_NAV_EXCEEDS_MAX_SYMB_COUNT,               /**< ET9NAVRecord field strings exceeds max symbol count */
    ET9STATUS_NAV_REJECTED_RECORD,                      /**< The record is rejected based on content (symbol values) */
    ET9STATUS_NAV_NOT_CREATING_RECORD,                  /**< No record is being created */
    ET9STATUS_NAV_ALREADY_ATTACHED_SORT_STRING,         /**< A sort string has already been attached to the field. */
    ET9STATUS_NAV_INVALID_ATTACH,                       /**< Can't attach to this field, check record properties. */
    /* misc invalid */
    ET9STATUS_NAV_INVALID_USER_DATA,                    /**< Invalid user data */
    ET9STATUS_NAV_INVALID_TYPE_COUNT,                   /**< Invalid type count */
    ET9STATUS_NAV_INVALID_CAPACITY,                     /**< Invalid capacity */
    ET9STATUS_NAV_INVALID_RECORD_KEY,                   /**< Invalid record key */
    ET9STATUS_NAV_INVALID_FIELD_ID,                     /**< Invalid field ID */
    ET9STATUS_NAV_INVALID_RESULT_ORDER,                 /**< Invalid result order */
    ET9STATUS_NAV_INVALID_DISPLAY_ORDER,                /**< Invalid display order */
    ET9STATUS_NAV_INVALID_RANK_GROUP_SIZE,              /**< Invalid rank group size */
    ET9STATUS_NAV_INVALID_DISPLAY_FIELD_INDEX,          /**< Invalid ET9NAVRecord display string field index */
    ET9STATUS_NAV_INVALID_TYPE_INFO_COUNT,              /**< Invalid type info count */
    /* misc */
    ET9STATUS_NAV_TYPE_ALREADY_ADDED,                   /**< Type already added (obsolete) */
    ET9STATUS_NAV_TYPES_WITH_MULTIPLE_FILES,            /**< An index with types using more than one file */
    ET9STATUS_NAV_MATCHHANDLE_QUERYSTATE_MISMATCH,      /**< Mismatch between ET9NAVMatchHandle and ET9NAVQueryState */
    ET9STATUS_NAV_MALFORMED_STRING,                     /**< Malformed string */
    ET9STATUS_NAV_UNKNOWN_MATCH_LOGIC,                  /**< Unknown match logic */
    ET9STATUS_NAV_ENCODING_TABLE_FULL,                  /**< \internal The symbol encoding table is full */
    ET9STATUS_NAV_ENCODING_NO_CODE,                     /**< \internal There is no code for the symbol */
    ET9STATUS_NAV_ENCODING_BAD_CODE,                    /**< \internal There is no symbol for the code */
    ET9STATUS_NAV_ENCODING_DENIED_CODE,                 /**< \internal The symbol was denied T8BIT encoding */
    ET9STATUS_NAV_ENCODING_BAD_HASH,                    /**< \internal Bad hash table result */
    /* block update */
    ET9STATUS_NAV_NOT_UPDATING_TYPE,                    /**< \internal Not block updating data type */
    ET9STATUS_NAV_ALREADY_UPDATING_TYPE,                /**< \internal Already block updating data type */
    ET9STATUS_NAV_BLOCK_UPDATE_IN_PROGRESS,             /**< \internal Operation is invalid because of block update */
    /* unused */
    ET9STATUS_NAV_UNUSED_1,                             /**< \internal unused/obsolete */
#endif

    ET9STATLAST                                         /**< \internal to have a max stat number and to have an entry which ends without a comma */

} ET9STATUS;


    /*------------------------------------------------------------------------
     *  Define ET9 word defines, structures, and functions
     *------------------------------------------------------------------------*/

#ifndef ET9MAXUDBWORDSIZE
#ifdef ET9MAXWORDSIZE
#define ET9MAXUDBWORDSIZE ET9MAXWORDSIZE
#else
#define ET9MAXUDBWORDSIZE    64             /**< Maximum UDB word length supported by XT9 (default). */
#endif
#endif

#if (ET9MAXUDBWORDSIZE < 32)
#error ET9MAXUDBWORDSIZE must be at least 32
#elif (ET9MAXUDBWORDSIZE > 127)
#error ET9MAXUDBWORDSIZE must be less than 128
#endif


/* **** IF ANY OF THESE CHANGE THE FIELD INFO STRUCTURE MUST CHANGE ******** */

#define ET9MAXLDBWORDSIZE    32             /**< Maximum LDB word length supported by XT9. */

#ifdef ET9MAXWORDSIZE

#if ET9MAXWORDSIZE < ET9MAXUDBWORDSIZE
#error ET9MAXWORDSIZE must be ET9MAXUDBWORDSIZE or more
#endif
#if ET9MAXWORDSIZE < ET9MAXLDBWORDSIZE
#error ET9MAXWORDSIZE must be ET9MAXLDBWORDSIZE or more
#endif
#if ET9MAXUDBWORDSIZE < ET9MAXLDBWORDSIZE
#error ET9MAXUDBWORDSIZE must be ET9MAXLDBWORDSIZE or more
#endif

#else /* ET9MAXWORDSIZE */

#if (ET9MAXUDBWORDSIZE > ET9MAXLDBWORDSIZE)
#define ET9MAXWORDSIZE   ET9MAXUDBWORDSIZE  /**< Maximum word length supported by XT9. */
#else
#define ET9MAXWORDSIZE   ET9MAXLDBWORDSIZE  /**< Maximum word length supported by XT9. */
#endif

#endif /* ET9MAXWORDSIZE */

#ifdef ET9_KDB_TRACE_MODULE
#if (ET9MAXWORDSIZE < (2 * ET9MAXLDBWORDSIZE))
#error ET9MAXWORDSIZE too small for Trace
#endif
#endif /* ET9_KDB_TRACE_MODULE */

#define ET9MAXVERSIONSTR 100                 /**< Maximum version string length. */

/*----------------------------------------------------------------------------
 *  Define ET9 "Symbol Class", values
 *----------------------------------------------------------------------------*/

#define ET9SYMWHITE 0                       /**< \internal symbol class white space */
#define ET9SYMPUNCT 1                       /**< \internal symbol class punctuation */
#define ET9SYMNUMBR 2                       /**< \internal symbol class number */
#define ET9SYMALPHA 3                       /**< \internal symbol class alpha */
#define ET9SYMUNKN  4                       /**< \internal symbol class unknown */

/*----------------------------------------------------------------------------
 *  ET9WordSymbInfo and generic related structures, typedefs, and defines
 *                          ** ALWAYS IN **
 *----------------------------------------------------------------------------*/

#define ET9MAXEDITIONS          4                   /**< \internal max number of engine editions */

#define ET9MAXBASESYMBS         16                  /**< \internal max number of base symbs (input symbol) */
#define ET9MAXALTSYMBS          16                  /**< \internal max number of alternate symbs (input symbol) */

#ifndef ET9MAXSUBSTITUTIONSIZE
#define ET9MAXSUBSTITUTIONSIZE   ET9MAXWORDSIZE     /**< Maximum substitution length (currently the same as selection list word size). */
#elif (ET9MAXSUBSTITUTIONSIZE > 255)
#error ET9MAXSUBSTITUTIONSIZE must be less than 256
#endif

#ifndef ET9SAVEINPUTSTORESIZE
#define ET9SAVEINPUTSTORESIZE  0x200                /**< Number of "symbol" positions available for input words that can be saved. */
#endif

#if (ET9SAVEINPUTSTORESIZE < ET9MAXWORDSIZE)
#error ET9SAVEINPUTSTORESIZE must at least have the same size as ET9MAXWORDSIZE
#endif

#define ET9MAXSAVEINPUTWORDS    ((ET9SAVEINPUTSTORESIZE) >> 3)      /**< \internal max number of input words that can be saved */

#if (ET9MAXSAVEINPUTWORDS == 0)
#error ET9MAXSAVEINPUTWORDS cannot be zero, increase ET9SAVEINPUTSTORESIZE
#endif


/**
 * Word sym state bits
 */

typedef enum ET9_WORDSYM_STATEBITS_e {
    ET9STATENEXTLOCKING = 0,            /**< 0 \internal state bit for next locking */
    ET9STATE_SHIFT,                     /**< 1 shift */
    ET9STATE_CAPS                       /**< 2 caps  */

} ET9_WORDSYM_STATEBITS;

/**
 * Word sym state bit masks
 */

typedef enum ET9_WORDSYM_STATEBITSMASK_e {
    ET9STATENEXTLOCKINGMASK       = ((ET9U32)(1L << ET9STATENEXTLOCKING)), /**< 0 \internal bit mask for next locking */
    ET9STATE_SHIFT_MASK           = ((ET9U32)(1L << ET9STATE_SHIFT)),      /**< 1 shift */
    ET9STATE_CAPS_MASK            = ((ET9U32)(1L << ET9STATE_CAPS))        /**< 2 caps  */

} ET9_WORDSYM_STATEBITSMASK;

#define ET9SHIFT_MODE(dwState)       ((dwState) & ET9STATE_SHIFT_MASK)
#define ET9CAPS_MODE(dwState)        ((dwState) & ET9STATE_CAPS_MASK)
#define ET9NEXTLOCKING_MODE(dwState) ((dwState) & ET9STATENEXTLOCKINGMASK)


/**
 * Structure for holding a simple word object.
 */

typedef struct ET9SimpleWord_s {
    ET9U16          wLen;                                   /**< Specifies the total word length. */
    ET9U16          wCompLen;                               /**< Specifies the length of the completion part. */
    ET9SYMB         sString[ET9MAXWORDSIZE];                /**< Specifies the word string (not terminated). */
} ET9SimpleWord;

/**
 * Input key types
 */

typedef enum ET9EDITION_e {
    ET9EDITION_AW_1,        /**< alphabetic 1 */
    ET9EDITION_AW_2,        /**< alphabetic 2 */
    ET9EDITION_CP,          /**< chinese phrasal */
    ET9EDITION_NAV,         /**< nav */

    ET9EDITION_LAST         /**< sentinel */

} ET9EDITION;

/**
 * Input key types
 */

typedef enum ET9KEYTYPE_e {
    ET9KTINVALID,           /**< 0 invalid key type */
    ET9KTLETTER,            /**< 1 key type letter */
    ET9KTPUNCTUATION,       /**< 2 key type punctuation */
    ET9KTNUMBER,            /**< 3 key type number */
    ET9KTSTRING,            /**< 4 key type string */
    ET9KTFUNCTION,          /**< 5 key type function */
    ET9KTSMARTPUNCT,        /**< 6 key type smart punct */
    ET9KTUNKNOWN,           /**< 7 key type unknown */
    ET9KT_LAST              /**< sentinel */

} ET9KEYTYPE;

/**
 * Input load key types
 */

typedef enum ET9LOADKEYTYPE_e {
    ET9LKT_REGIONAL,        /**< 0 */
    ET9LKT_NONREGIONAL,     /**< 1 */
    ET9LKT_SMARTPUNCT,      /**< 2 */
    ET9LKT_STRING,          /**< 3 */
    ET9LKT_FUNCTION,        /**< 4 */
    ET9LKT_LAST             /**< sentinel */

} ET9LOADKEYTYPE;

/**
 * Type of input
 */

typedef enum ET9INPUTTYPE_e {
    ET9DISCRETEKEY,         /**< 0 */
    ET9REGIONALKEY,         /**< 1 */
    ET9HANDWRITING,         /**< 2 */
    ET9MULTITAPKEY,         /**< 3 */
    ET9CUSTOMSET,           /**< 4 */
    ET9EXPLICITSYM,         /**< 5 */
    ET9MULTISYMBEXPLICIT,   /**< 6 */
    ET9INPUTTYPE_LAST       /**< sentinel */

} ET9INPUTTYPE;

/** \internal
 * KDB load states.
 */

typedef enum ET9KDBLOADSTATES_e {

    ET9KDBLOADSTATES_START,             /**< \internal  */
    ET9KDBLOADSTATES_HAS_PROPERTIES,    /**< \internal  */
    ET9KDBLOADSTATES_HAS_KEY,           /**< \internal  */
    ET9KDBLOADSTATES_LAST               /**< \internal sentinel */

} ET9KDBLOADSTATES;

/**
 * Interface for XT9 to request symbol conversion (on-the-fly mapping) from the integration layer.<br>
 * This callback function is used to convert one character to another character of the same
 * character encoding, or it can be used to filter characters out of the multitap sequence.
 */

typedef ET9STATUS (ET9FARCALL *ET9CONVERTSYMBCALLBACK)(
    void    *pConvertInfo,                  /**< Pointer to an integration layer object. */
    ET9SYMB *psConvertSymb                  /**< Pointer to the character that XT9 wants to add to the text buffer. If the integration layer wants to convert this character to different character, it changes the character-code value pointed to by psConvertSymb. */
);

/** \internal
 * Callback function for symbol filtering.
 */

typedef ET9STATUS (ET9FARCALL *ET9FILTERSYMBCALLBACK)(
    void            * const pFilterInfo,            /**< \internal pointer to an object */
    const ET9SYMB           sFilterSymb             /**< \internal filtering symbol */
);

/** \internal
 * Callback function for symbol filter reset.
 */

typedef void (ET9FARCALL *ET9FILTERSYMBRESETCALLBACK)(
    void            * const pFilterInfo             /**< \internal pointer to an object */
);

/** \internal
 * Callback function for getting the number of currently available filters.
 */

typedef ET9STATUS (ET9FARCALL *ET9FILTERSYMBCOUNTCALLBACK)(
    void            * const pFilterInfo,            /**< \internal pointer to an object */
    ET9U8           * const pbFilterCount           /**< \internal pointer to the number of available filters */
);

/** \internal
 * Callback function for switching to the next filter.
 */

typedef ET9STATUS (ET9FARCALL *ET9FILTERSYMBNEXTCALLBACK)(
    void            * const pFilterInfo             /**< \internal pointer to an object */
);

/** \internal
 * Callback function for group comparison - check if two symbols belongs to the same (alternate) group.
 */

typedef ET9BOOL (ET9FARCALL *ET9FILTERSYMBGROUPCALLBACK)(
    void            * const pFilterInfo,            /**< \internal pointer to an object */
    const ET9SYMB           sSymb1,                 /**< \internal first symbol */
    const ET9SYMB           sSymb2                  /**< \internal second symbol */
);

/**
 * Structure for holding data for a base (input) symbol.
 */

typedef struct ET9DataPerBaseSym_s {
    ET9SYMB sChar[ET9MAXALTSYMBS];                  /**< Specifies the lower case character list. */
    ET9SYMB sUpperCaseChar[ET9MAXALTSYMBS];         /**< Specifies the upper case character list. */
    ET9U8   bSymFreq;                               /**< Specifies the tap frequency. */
    ET9U8   bNumSymsToMatch;                        /**< Specifies the number of symbols in the list. */
    ET9U8   bDefaultCharIndex;                      /**< Specifies the default character in the list. */

} ET9DataPerBaseSym;

/**
 * Input shift states
 */

typedef enum ET9INPUTSHIFTSTATE_e {
    ET9NOSHIFT = 0,                     /**< 0 no shift (default shift info) */
    ET9SHIFT,                           /**< 1 shift (one character) */
    ET9CAPSLOCK                         /**< 2 caps lock (all characters) */

} ET9INPUTSHIFTSTATE;


/**
 * AutoCaps related
 */

typedef enum ET9AUTOCAPSITUATION_e {
    ET9AUTOCAP_OFF,
    ET9AUTOCAP_PENDING,
    ET9AUTOCAP_APPLIED
} ET9AUTOCAPSITUATION;

/**
 * Callback function for buffer access.
 */

typedef ET9STATUS (ET9FARCALL *ET9BUFFERREADCALLBACK)(
    void          *pBufferReadInfo,                 /**< pointer to an integration layer object */
    ET9U16         wNumberOfReadSymbols,            /**< specifies the number of symbols to be read */
    ET9SYMB       *psDest,                          /**< buffer for the data */
    ET9U16        *pwSymbolsRead                    /**< pointer to the variable that receives the number of symbols read */
);

/**
 * Post shift states
 */

typedef enum ET9POSTSHIFTMODE_e {
    ET9POSTSHIFTMODE_LOWER,             /**< 0 force all lower case */
    ET9POSTSHIFTMODE_INITIAL,           /**< 1 force first up and the rest low */
    ET9POSTSHIFTMODE_UPPER,             /**< 2 force all upper case */
    ET9POSTSHIFTMODE_DEFAULT,           /**< 3 based on data bases, context, input shift state etc */
    ET9POSTSHIFTMODE_NEXT,              /**< 4 move to the next post shift state (that differs in content) */

    ET9POSTSHIFTMODE_LAST               /**< sentinel */

} ET9POSTSHIFTMODE;

/**
 * Ambig type
 */

typedef enum ET9AMBIGTYPE_e {
    ET9AMBIG,                 /**< 0 */
    ET9EXACT                  /**< 1 */
} ET9AMBIGTYPE;

/**
 * Lock type
 */

typedef enum ET9LOCKTYPE_e {
    ET9NOLOCK = 0,          /**< 0 */
    ET9STEMLOCK,            /**< 1 a stem was locked (normal lock) */
    ET9EXACTLOCK            /**< 2 the exact was locked (special behaviour) */
} ET9LOCKTYPE;

/** \internal
 * Language source
 */

typedef enum ET9AWLANGUAGESOURCE_e {
    ET9AWUNKNOWN_LANGUAGE = 0,
    ET9AWFIRST_LANGUAGE,
    ET9AWSECOND_LANGUAGE,
    ET9AWBOTH_LANGUAGES

} ET9AWLANGUAGESOURCE;

/** \internal
 * Input events
 */

typedef enum ET9InputEvent_e {
    ET9InputEvent_none = 0,         /**< \internal no-op, no event */
    ET9InputEvent_clear,            /**< \internal clear symb event */
    ET9InputEvent_add               /**< \internal add symb event */
} ET9InputEvent;                    /**< \internal */

/** \internal
 * Input track kind
 */

typedef enum ET9InputTrack_e {
    ET9InputTrack_none = 0,         /**< \internal no input */
    ET9InputTrack_tap,              /**< \internal tap/key input */
    ET9InputTrack_trace             /**< \internal trace input */
} ET9InputTrack;                    /**< \internal */

/** \internal
 * Size of buffer requested in callback (syms prior to current insertion point)
 */

#define ET9AUTOCAP_READ_SIZE            20

#define ET9_KDB_MAX_KEY_BLOCKS          50                          /**< \internal Max number of key blocks that will be analyzed per KDB key. */

#define ET9_KDB_LOAD_UNDEF_VALUE        0xFFFF                      /**< Value to use for indicating an undefined value. */

#define ET9_INPUT_TRACK_SIZE            5                           /**< \internal Input track history size. */

#ifndef ET9_TRACE_MAX_POINTS
#define ET9_TRACE_MAX_POINTS            250                         /**< Max number of points/samples in a trace. */
#endif

#if (ET9_TRACE_MAX_POINTS < 100)
#error ET9_TRACE_MAX_POINTS be at least 100
#elif (ET9_TRACE_MAX_POINTS > 1000)
#error ET9_TRACE_MAX_POINTS must be less than or equal to 1000
#endif

#ifndef ET9_KDB_MAX_KEYS
#define ET9_KDB_MAX_KEYS                70                          /**< Max number of keys in a KDB. */
#endif

#if (ET9_KDB_MAX_KEYS < 16)
#error ET9_KDB_MAX_KEYS must be at least 16
#elif (ET9_KDB_MAX_KEYS > 255)
#error ET9_KDB_MAX_KEYS must be less than or equal to 255
#endif

#ifndef ET9_KDB_MAX_ROWS
#define ET9_KDB_MAX_ROWS                8                           /**< Max number of rows in a KDB. */
#endif

#if (ET9_KDB_MAX_ROWS < 1)
#error ET9_KDB_MAX_ROWS must be at least 1
#elif (ET9_KDB_MAX_ROWS > 32)
#error ET9_KDB_MAX_ROWS must be less than or equal to 32
#endif

#ifndef ET9_KDB_MAX_PAGE_CACHE
#define ET9_KDB_MAX_PAGE_CACHE          4                           /**< Max number of pages cached. */
#endif

#if (ET9_KDB_MAX_PAGE_CACHE < 1)
#error ET9_KDB_MAX_PAGE_CACHE must be at least 1
#elif (ET9_KDB_MAX_PAGE_CACHE > 64)
#error ET9_KDB_MAX_PAGE_CACHE must be less than or equal to 64
#endif

#ifndef ET9_KDB_MAX_POOL_CHARS
#define ET9_KDB_MAX_POOL_CHARS          (ET9_KDB_MAX_KEYS * 32)     /**< Max number of chars in a KDB. */
#endif

#if (ET9_KDB_MAX_POOL_CHARS < 100)
#error ET9_KDB_MAX_POOL_CHARS must be at least 100
#elif (ET9_KDB_MAX_POOL_CHARS > 0xFFFF)
#error ET9_KDB_MAX_POOL_CHARS must be less than or equal to 0xFFFF
#endif

#ifndef ET9_KDB_XML_MAX_ICON_CHARS
#define ET9_KDB_XML_MAX_ICON_CHARS      32                          /**< \internal Max number of chars per icon for readers. */
#endif

#if (ET9_KDB_XML_MAX_ICON_CHARS < 1)
#error ET9_KDB_XML_MAX_ICON_CHARS must be at least 1
#elif (ET9_KDB_XML_MAX_ICON_CHARS > 255)
#error ET9_KDB_XML_MAX_ICON_CHARS must be less than or equal to 255
#endif

#ifndef ET9_KDB_XML_MAX_LABEL_CHARS
#define ET9_KDB_XML_MAX_LABEL_CHARS     16                          /**< \internal Max number of chars per label for readers. */
#endif

#if (ET9_KDB_XML_MAX_LABEL_CHARS < 1)
#error ET9_KDB_XML_MAX_LABEL_CHARS must be at least 1
#elif (ET9_KDB_XML_MAX_LABEL_CHARS > 255)
#error ET9_KDB_XML_MAX_LABEL_CHARS must be less than or equal to 255
#endif

#ifndef ET9_KDB_XML_MAX_POPUP_CHARS
#define ET9_KDB_XML_MAX_POPUP_CHARS     32                          /**< \internal Max number of chars per popup for readers. */
#endif

#if (ET9_KDB_XML_MAX_POPUP_CHARS < 1)
#error ET9_KDB_XML_MAX_POPUP_CHARS must be at least 1
#elif (ET9_KDB_XML_MAX_POPUP_CHARS > 255)
#error ET9_KDB_XML_MAX_POPUP_CHARS must be less than or equal to 255
#endif

#ifndef ET9_KDB_XML_MAX_MULTITAP_CHARS
#define ET9_KDB_XML_MAX_MULTITAP_CHARS  32                          /**< \internal Max number of chars per multitap for readers. */
#endif

#if (ET9_KDB_XML_MAX_MULTITAP_CHARS < 1)
#error ET9_KDB_XML_MAX_MULTITAP_CHARS must be at least 1
#elif (ET9_KDB_XML_MAX_MULTITAP_CHARS > 255)
#error ET9_KDB_XML_MAX_MULTITAP_CHARS must be less than or equal to 255
#endif

#ifndef ET9_KDB_MAX_KEY_CHARS
#define ET9_KDB_MAX_KEY_CHARS           100                         /**< \internal Max number of chars per key for readers. */
#endif

#if (ET9_KDB_MAX_KEY_CHARS < 16)
#error ET9_KDB_MAX_KEY_CHARS must be at least 16
#elif (ET9_KDB_MAX_KEY_CHARS > 255)
#error ET9_KDB_MAX_KEY_CHARS must be less than or equal to 255
#endif

#ifndef ET9_KDB_XML_MAX_STRING_CHARS
#define ET9_KDB_XML_MAX_STRING_CHARS    1024                        /**< \internal Max number of chars in an XML attribute string. */
#endif

#if (ET9_KDB_XML_MAX_STRING_CHARS < 256)
#error ET9_KDB_XML_MAX_STRING_CHARS must be at least 256
#elif (ET9_KDB_XML_MAX_STRING_CHARS > 0x2000)
#error ET9_KDB_XML_MAX_STRING_CHARS must be less than or equal to 0x2000
#endif

#ifndef ET9_KDB_XML_MAX_POOL_CHARS
#define ET9_KDB_XML_MAX_POOL_CHARS      2048                        /**< \internal Max number of chars in an XML KDB (all key strings). */
#endif

#if (ET9_KDB_XML_MAX_POOL_CHARS < 256)
#error ET9_KDB_XML_MAX_POOL_CHARS must be at least 256
#elif (ET9_KDB_XML_MAX_POOL_CHARS > 0xFFFF)
#error ET9_KDB_XML_MAX_POOL_CHARS must be less than or equal to 0xFFFF
#endif

#ifndef ET9_KDB_XML_MAX_DEDUPE_CHARS
#define ET9_KDB_XML_MAX_DEDUPE_CHARS    1021                        /**< \internal Max number of chars in the dedupe table - preferbly a prime number. */
#endif

#if (ET9_KDB_XML_MAX_DEDUPE_CHARS < 1000)
#error ET9_KDB_XML_MAX_DEDUPE_CHARS must be at least 1000
#elif (ET9_KDB_XML_MAX_DEDUPE_CHARS > 0xFFFF)
#error ET9_KDB_XML_MAX_DEDUPE_CHARS must be less than or equal to 0xFFFF
#endif

/**
 * Structure holding key region info.
 */

typedef struct ET9Region_s {
    ET9U16 wLeft;                                       /**< Left */
    ET9U16 wTop;                                        /**< Top */
    ET9U16 wRight;                                      /**< Right */
    ET9U16 wBottom;                                     /**< Bottom */
} ET9Region;                                            /**< */

/**
 * Structure holding coordinates for a trace point (int).
 */

typedef struct ET9TracePoint_s {
    ET9UINT         nX;                                 /**< X coordinate. */
    ET9UINT         nY;                                 /**< Y coordinate. */
} ET9TracePoint;                                        /**< */

/**
 * Structure holding coordinates for a trace point (float).
 */

typedef struct ET9TracePoint_f_s {
    ET9FLOAT        fX;                                 /**< X coordinate. */
    ET9FLOAT        fY;                                 /**< Y coordinate. */
} ET9TracePoint_f;                                      /**< */

/** \internal
 * Structure holding internal trace point info.
 */

typedef struct ET9TracePointExt_s {
    ET9TracePoint_f sPoint;                             /**< \internal Coordinate */

    ET9FLOAT        fAngle;                             /**< \internal Mid angle. */
    ET9FLOAT        fDist;                              /**< \internal Distance to next point. */

    ET9U16          wKey;                               /**< \internal KDB key */
    ET9SYMB         sSymb;                              /**< \internal Top charcter on the key */
    ET9UINT         nPos;                               /**< \internal Position index */
    ET9UINT         nDensity;                           /**< \internal Point density */
    ET9UINT         nQuality;                           /**< \internal Point quality */
    ET9BOOL         bQualityId;                         /**< \internal Quality ID */
    ET9BOOL         bRemoved;                           /**< \internal Marked for removal */
    ET9BOOL         bMergeWithPrev;                     /**< \internal Marked for merge with previous */
} ET9TracePointExt;                                     /**< \internal */

/**
 * Structure holding keyboard key information.
 */

typedef struct ET9KeyPoint_s {
    ET9KEYTYPE      eKeyType;                           /**< Key type */
    ET9INPUTTYPE    eInputType;                         /**< Input type */
    ET9U16          wKey;                               /**< Key index */
    ET9SYMB         sTopSymb;                           /**< Key top symb */
    ET9UINT         nSymbCount;                         /**< Key symb count */
    ET9SYMB const * psSymbs;                            /**< Key symbs */
    ET9UINT         nX;                                 /**< X coordinate */
    ET9UINT         nY;                                 /**< Y coordinate */
    ET9Region       sArea;                              /**< Key area - if available */
} ET9KeyPoint;                                          /**< */

/** \internal
 * Structure holding a KDB block info.
 */

typedef struct ET9KdbBlockInfo_s {

    ET9U8           bProb;                              /**< \internal Probability */

    ET9U16          wX;                                 /**< \internal Center X coordinate */
    ET9U16          wY;                                 /**< \internal Center Y coordinate */
} ET9KdbBlockInfo;                                      /**< \internal */

/** \internal
 * Structure holding information about a KDB key.
 */

typedef struct ET9KdbAreaInfo_s {
    ET9U16          wKeyIndex;                          /**< \internal Key value (for e.g. process-key) */
    ET9BOOL         bRegionOk;                          /**< \internal If key region is ok (actual or estimate) */
    ET9KEYTYPE      eKeyType;                           /**< \internal Key type */
    ET9INPUTTYPE    eInputType;                         /**< \internal Input type */

    ET9UINT         nCenterX;                           /**< \internal Center X coordinate */
    ET9UINT         nCenterY;                           /**< \internal Center Y coordinate */

    ET9Region       sRegion;                            /**< \internal Key region - not always available */

    ET9UINT         nCharCount;                         /**< \internal Total char count */
    ET9SYMB         *psChars;                           /**< \internal Characters assigned to the key */

    ET9UINT         nShiftedCharCount;                  /**< \internal Total shifted char count */
    ET9SYMB         *psShiftedChars;                    /**< \internal Shifted characters assigned to the key */

    ET9UINT         nMultitapCharCount;                 /**< \internal Total multitap char count */
    ET9SYMB         *psMultitapChars;                   /**< \internal Multitap characters assigned to the key */

    ET9UINT         nMultitapShiftedCharCount;          /**< \internal Total multitap shifted char count */
    ET9SYMB         *psMultitapShiftedChars;            /**< \internal Multitap shifted characters assigned to the key */
} ET9KdbAreaInfo;                                       /**< \internal */

/** \internal
 * Structure holding extra information about a KDB key, when being restored/calculated.
 */

typedef struct ET9KdbAreaExtraInfo_s {
    ET9U8           bMaxProbCenter;                     /**< \internal Max probability for center */
    ET9U16          wMaxProbCenterX;                    /**< \internal Max probability center X */
    ET9U16          wMaxProbCenterY;                    /**< \internal Max probability center Y */
    ET9U16          wMaxProbCenterCount;                /**< \internal Max probability center count */

    ET9UINT         nBlockCount;                        /**< \internal Total block count */
    ET9KdbBlockInfo pBlocks[ET9_KDB_MAX_KEY_BLOCKS];    /**< \internal Block info's */
} ET9KdbAreaExtraInfo;                                  /**< \internal */

/** \internal
 * Structure holding KDB layout information.
 */

typedef struct ET9KdbLayoutInfo_s {
    ET9BOOL                     bOk;                                /**< \internal If this info ok (valid). */
    ET9BOOL                     bDynamic;                           /**< \internal If this info comes from dynamic or not - if it's static. */

    ET9U16                      wKdbNum;                            /**< \internal Layout info for keyboard. */
    ET9U16                      wPageNum;                           /**< \internal Layout info for keyboard page. */

    ET9U8                       bDatabaseType;                      /**< \internal KDB property. */
    ET9U8                       bLayoutVer;                         /**< \internal KDB property. */
    ET9U8                       bPrimaryID;                         /**< \internal KDB property. */
    ET9U8                       bSecondaryID;                       /**< \internal KDB property. */
    ET9U8                       bSymbolClass;                       /**< \internal KDB property. */
    ET9U8                       bContentsMajor;                     /**< \internal KDB property. */
    ET9U8                       bContentsMinor;                     /**< \internal KDB property. */
    ET9U16                      wTotalPages;                        /**< \internal Total number of keyboard pages. */

    ET9U16                      wLayoutWidth;                       /**< \internal KDB layout width. */
    ET9U16                      wLayoutHeight;                      /**< \internal KDB layout height. */

    ET9UINT                     nRadius;                            /**< \internal Radius (regionality) */

    ET9UINT                     nMinKeyOverlap;                     /**< \internal Minimum key overlap (regionality) */

    ET9UINT                     nMinKeyWidth;                       /**< \internal Min key width. */
    ET9UINT                     nMinKeyHeight;                      /**< \internal Min key height. */

    ET9UINT                     nMedianKeyWidth;                    /**< \internal Median key width. */
    ET9UINT                     nMedianKeyHeight;                   /**< \internal Median key height. */

    ET9UINT                     nBoxWidth;                          /**< \internal Box width. */
    ET9UINT                     nBoxHeight;                         /**< \internal Box height. */

    ET9UINT                     nKeyAreaCount;                      /**< \internal Number of key areas in the KDB. */
    ET9KdbAreaInfo              pKeyAreas[ET9_KDB_MAX_KEYS];        /**< \internal Array of key areas. */

    ET9UINT                     nCharPoolCount;                     /**< \internal Total char pool count */
    ET9SYMB                     psCharPool[ET9_KDB_MAX_POOL_CHARS]; /**< \internal Character pool for key chars */
} ET9KdbLayoutInfo;                                                 /**< \internal */

/** \internal
 * Structure holding KDB layout information.
 */

typedef struct ET9KdbLayoutExtraInfo_s {
    ET9KdbAreaExtraInfo         pKeyAreas[ET9_KDB_MAX_KEYS];        /**< \internal Array of key areas. */
} ET9KdbLayoutExtraInfo;                                            /**< \internal */

/**
 * Structure for holding information for one input symbol.
 */

typedef struct ET9SymbInfo_s {
    ET9DataPerBaseSym           DataPerBaseSym[ET9MAXBASESYMBS];    /**< Specifies information for all base symbols. */
    ET9SYMB                     sLockedSymb;                        /**< Specifies the locked symbol(s) store. */
    ET9INPUTTYPE                eInputType;                         /**< Specifies the type of input (e.g. regional, discrete). */
    ET9INPUTSHIFTSTATE          eShiftState;                        /**< Specifies the shift state. */
    ET9U16                      wInputIndex;                        /**< Specifies the input index built. */
    ET9U8 /* ET9AMBIGTYPE */    bAmbigType;                         /**< Specifies the ambiguous type of input used (e.g. regional, exact). */
    ET9U8                       bNumBaseSyms;                       /**< Specifies the number of base symbymbols in the list. */
    ET9U8 /* ET9KEYTYPE */      bSymbType;                          /**< Specifies the key type (e.g. letter, punctuation). */
    ET9U8                       bLocked;                            /**< Specifies the lock value (a lock took place at this input position). */
    ET9U8                       bLockLanguageSource;                /**< \internal Specifies the language source for lock (1: first language; 2: second language; 3: both languages). */
    ET9BOOL                     bAutoDowncase;                      /**< \internal Specifies that the symbol should be automatically downcased.*/
    ET9BOOL                     bForcedLowercase;                   /**< \internal Specifies that the symbol should be downcased postshift. */
    ET9U8                       bTraceProbability;                  /**< \internal Specifies the trace point probability. */
    ET9U8                       bTraceIndex;                        /**< \internal Specifies the trace point index (group). */
    ET9BOOL                     bFreqsInvalidated;                  /**< \internal Specifies the need to validate freqs for matching. */

    ET9KdbAreaInfo       const *pKdbKey;                            /**< \internal Specifies the key used. */
    ET9U16                      wTapX;                              /**< Specifies the tap x used. */
    ET9U16                      wTapY;                              /**< Specifies the tap y used. */
    ET9U16                      wKeyIndex;                          /**< Specifies the key index used. */

    ET9U16                      wKdb1;                              /**< \internal Keeping history of KDB info used to create the symb. */
    ET9U16                      wPage1;                             /**< \internal Keeping history of KDB info used to create the symb. */
    ET9U16                      wKdb2;                              /**< \internal Keeping history of KDB info used to create the symb. */
    ET9U16                      wPage2;                             /**< \internal Keeping history of KDB info used to create the symb. */
} ET9SymbInfo;                                                      /**< */

typedef struct ET9BaseLingInfo_s ET9BaseLingInfo;

/** \internal
 * Structure holding information for one saved input symb
 */

typedef struct ET9SavedInputSymb_s {

    ET9INPUTTYPE        eInputType;                     /**< \internal Type of input */
    ET9BOOL             bLocked;                        /**< \internal Lock flag */
    ET9INPUTSHIFTSTATE  eShiftState;                    /**< \internal Shift state */
    ET9U16              bInputIndex;                    /**< \internal Built by input index (strings etc, 8 bits for now) */
    ET9U8               bForcedLowercase;               /**< \internal postshift 'lower' causes forced downshift of this symb */
    ET9U8               bTraceProbability;              /**< \internal the trace point probability */
    ET9U8               bTraceIndex;                    /**< \internal the trace point index (group) */

    ET9U16              wKeyIndex;                      /**< \internal key index used */

    ET9U16              wTapX;                          /**< \internal tap x used */
    ET9U16              wTapY;                          /**< \internal tap y used */

    ET9U16              wKdb1;                          /**< \internal first KDB used */
    ET9U16              wPage1;                         /**< \internal first KDB page used */
    ET9U16              wKdb2;                          /**< \internal second KDB used */
    ET9U16              wPage2;                         /**< \internal second KDB page used */

    ET9SYMB             sSymb;                          /**< \internal symbol that can be used for explicit entry */

} ET9SavedInputSymb;                                    /**< \internal */

/** \internal
 * Structure holding information for one saved input word
 */

typedef struct ET9SavedInputWord_s {

    ET9U16              wStorePos;                              /**< \internal the store position (circular use) */

    ET9U16              wWordLen;                               /**< \internal total word length */
    ET9U16              wInputLen;                              /**< \internal total input length */

    ET9INPUTSHIFTSTATE  eLastShiftState;                        /**< \internal the last known shift state when the word was entered */

} ET9SavedInputWord;                                            /**< \internal */

/** \internal
 * Structure holding information for saved input words
 */

typedef struct ET9SavedInputWords_s {

    ET9U16              wCurrInputSaveIndex;                    /**< \internal current input index, for words */

    ET9SavedInputWord   pSavedWords[ET9MAXSAVEINPUTWORDS];      /**< \internal storage for saved input words */

    ET9SYMB             sWords[ET9SAVEINPUTSTORESIZE];          /**< \internal storage for word symbols */
    ET9SavedInputSymb   sInputs[ET9SAVEINPUTSTORESIZE];         /**< \internal storage for input info */

} ET9SavedInputWords;                                           /**< \internal */

/** \internal
 * Private Structure for holding the word symbol object - the "agnostic" object between engine and input.
 */

typedef struct ET9WordSymbInfoPrivate_s {
    ET9INPUTSHIFTSTATE      eLastShiftState;                        /**< \internal Specifies the last known shift state. */
    ET9POSTSHIFTMODE        eCurrPostShiftMode;                     /**< \internal Specifies the current post shift mode. */

    ET9U16                  wLocale;                                /**< \internal Locale language used for case conversion etc. */
    ET9BOOL                 bManualLocale;                          /**< \internal If the loacale has been manually assigned, or is handled automatically (manual overrides auto) */

    ET9U8                   bCurrSelListIndex;                      /**< \internal Specifies the the current selection list index, e.g. known from a key press. */
    ET9S8                   bCompoundingDownshift;                  /**< \internal Specifies that compounded uppercase words should be downshifted. */
    ET9AUTOCAPSITUATION     eAutocapWord;                           /**< \internal Specifies the status of input with regard to auto-capitalization. */
    ET9U8                   bSwitchLanguage;                        /**< \internal Specifies whether to toggle languages. */

    ET9SimpleWord           sRequiredWord;                          /**< \internal Specifies the word that is required to be in the selection list. */
    ET9BOOL                 bRequiredLocate;                        /**< \internal Specifies whether to make the required word the default if it is found in the list. */
    ET9BOOL                 bRequiredVerifyInput;                   /**< \internal Specifies that input of the required word be verified. */
    ET9BOOL                 bRequiredInhibitOverride;               /**< \internal Specifies that the exact can't be overridden. */
    ET9BOOL                 bRequiredInhibitCorrection;             /**< \internal Specifies that corrections like e.g. completion should be inhibited. */
    ET9BOOL                 bRequiredHasRegionalInfo;               /**< \internal Specifies that if the input has/had regional info. */
    ET9INPUTSHIFTSTATE      eRequiredLastShiftState;                /**< \internal Specifies the the required word's last known shift state. */

    ET9BOOL                 bInputRestarted;                        /**< \internal Specifies whether input started a new word. */

    ET9SavedInputWords      sSavedInputWords;                       /**< \internal Specifies the storage for saved input words. */

    ET9BOOL                 bPreventWhiteSpaceInput;                /**< \internal Specifies whether white space input should be prevented. */

    ET9InputEvent           eLastInputEvent;                        /**< \internal Last input event. */
    ET9U8                   bClearSymbEpisodeCount;                 /**< \internal Counter for clear-symb episodes during a single word input. */

    ET9U8                   bCurrInputTrackIndex;                   /**< \internal Current index in the circular history buffer (for next input). */
    ET9InputTrack           peInputTracks[ET9_INPUT_TRACK_SIZE];    /**< \internal Last input tracking history. */

    ET9U16                  wIDBVersionStrSize;                     /**< \internal Specifies the the length of the KDB version string. */
    ET9SYMB                 szIDBVersion[ET9MAXVERSIONSTR];         /**< \internal Specifies the the KDB version string. */

    ET9BaseLingInfo*        ppEditionsList[ET9MAXEDITIONS];         /**< \internal Pointers to related linguistic engines. */

} ET9WordSymbInfoPrivate;                                           /**< \internal */

/**
 * Structure for holding the word symbol object - the "agnostic" object between engine and input.
 */

typedef struct ET9WordSymbInfo_s {
    ET9U32                      dwStateBits;                    /**< Specifies the imu state bits. */

    ET9SymbInfo                 SymbsInfo[ET9MAXWORDSIZE];      /**< Specifies the list of input symbols. */
    ET9U8                       bNumSymbs;                      /**< Specifies the number of input symbols used (available). */

    void                        *pPublicExtension;              /**< Pointer for OEM extension. */

    ET9WordSymbInfoPrivate      Private;                        /**< Private word symb info. */

    ET9U16                      wInitOK;                        /**< Specifies to verify if initialized ok. */

} ET9WordSymbInfo;                                              /**< */

/** \internal
 * Base linguistic engine object.
 */

struct ET9BaseLingInfo_s {

    ET9BOOL                 bContentExplicified;                        /**< \internal the word symb info content is an explicification */

    ET9BOOL                 bSelListInvalidated;                        /**< \internal the selection list content is invalid, not handled yet */
    ET9BOOL                 bSymbsInfoInvalidated;                      /**< \internal the symbol information has been modified, not handled yet */

    ET9BOOL                 bSymbInvalidated[ET9MAXWORDSIZE];           /**< \internal symbol has been modified, not handled yet */
    ET9BOOL                 bLockInvalidated[ET9MAXWORDSIZE];           /**< \internal lock has been modified, not handled yet */

    ET9WordSymbInfo        *pWordSymbInfo;                              /**< \internal pointer to entered symbols */

#ifdef EVAL_BUILD
    ET9U16  wReserve1;
    ET9U16  wReserve2;
#endif
};


/* defines for private use in selection list building */

#define ET9SLEXACTINLIST         ((ET9U32)0x1)
#define ET9SLBUILDABLEEXACT      ((ET9U32)0x2)

ET9SYMB ET9FARCALL ET9SymToLower(const ET9SYMB sData);
ET9SYMB ET9FARCALL ET9SymToUpper(const ET9SYMB sData);
ET9SYMB ET9FARCALL ET9SymToOther(const ET9SYMB sData);
ET9UINT ET9FARCALL ET9SymIsLower(const ET9SYMB sData);
ET9UINT ET9FARCALL ET9SymIsUpper(const ET9SYMB sData);
ET9U8   ET9FARCALL ET9GetSymbolClass(const ET9SYMB sData);

/*------------------------------------------------------------------------
 *  Define ET9 database types and bit masks.
 *------------------------------------------------------------------------*/

#define ET9MAXDBTYPE  8

typedef enum {
    ET9DB_LDB_LATIN = 2,
    ET9DB_KDB       = 3

} ET9DB_TYPE;


typedef enum {
    ET9DB_LDB_LATIN_MASK = (1 << ET9DB_LDB_LATIN),
    ET9DB_KDB_MASK       = (1 << ET9DB_KDB)

} ET9DB_TYPE_MASK;


/*----------------------------------------------------------------------------
 *  End: ET9WordSymbInfo and generic related structures, typedefs, and defines
 *                          ** ALWAYS IN **
 *----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
 *  Generic Input Module ** ALWAYS IN **
 *----------------------------------------------------------------------------*/
ET9STATUS ET9FARCALL ET9WordSymbInit(ET9WordSymbInfo * const pWordSymbInfo,
                                     const ET9BOOL           bResetWordSymbInfo);

ET9STATUS ET9FARCALL ET9ClearOneSymb(ET9WordSymbInfo * const pWordSymbInfo);

ET9STATUS ET9FARCALL ET9ClearAllSymbs(ET9WordSymbInfo * const pWordSymbInfo);

ET9STATUS ET9FARCALL ET9DeleteSymbs(ET9WordSymbInfo     * const pWordSymbInfo,
                                    const ET9U8                 bIndex,
                                    const ET9U8                 bCount);

ET9STATUS ET9FARCALL ET9AddExplicitSymb(ET9WordSymbInfo   * const pWordSymbInfo,
                                        const ET9SYMB             sSymb,
                                        const ET9INPUTSHIFTSTATE  eShiftState,
                                        const ET9U8               bCurrIndexInList);

ET9STATUS ET9FARCALL ET9AddCustomSymbolSet(ET9WordSymbInfo   * const pWordSymbInfo,
                                           ET9SYMB           * const psSymbs,
                                           ET9U8             * const pbSymbProbs,
                                           const ET9UINT             nNumSymbsInSet,
                                           const ET9INPUTSHIFTSTATE  eShiftState,
                                           const ET9U8               bCurrIndexInList);

ET9STATUS ET9FARCALL ET9GetExactWord(ET9WordSymbInfo      * const pWordSymbInfo,
                                     ET9SimpleWord        * const pWord,
                                     const ET9CONVERTSYMBCALLBACK pConvertSymb,
                                     void                 * const pConvertSymbInfo);

ET9STATUS ET9FARCALL ET9LockWord(ET9WordSymbInfo   * const pWordSymbInfo,
                                 ET9SimpleWord     * const pWord);


ET9STATUS ET9FARCALL ET9SetNextLocking(ET9WordSymbInfo * const pWordSymbInfo);

ET9STATUS ET9FARCALL ET9ClearNextLocking(ET9WordSymbInfo * const pWordSymbInfo);

ET9STATUS ET9FARCALL ET9GetAutoCapSituation(ET9WordSymbInfo * const pWordSymbInfo,
                                            ET9BOOL         *       pbAutoCap,
                                            const ET9BUFFERREADCALLBACK pBufferRead,
                                            void            * const pBufferReadInfo);

ET9STATUS ET9FARCALL ET9SetUnShift(ET9WordSymbInfo  * const pWordSymbInfo);

ET9STATUS ET9FARCALL ET9SetShift(ET9WordSymbInfo  * const pWordSymbInfo);

ET9STATUS ET9FARCALL ET9SetCapsLock(ET9WordSymbInfo  * const pWordSymbInfo);


/*----------------------------------------------------------------------------
 *  End: Generic Input Module ** ALWAYS IN **
 *----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
 *  KDB Input Module  ** REMOVABLE **
 *----------------------------------------------------------------------------*/

#ifdef ET9_KDB_MODULE

typedef struct ET9KDBInfo_s ET9KDBInfo;


/**
 * Interface for Keyboard Input Module requests for dynamic loading of keyboard data.
 */

typedef ET9STATUS (ET9FARCALL *ET9KDBLOADCALLBACK)(
    ET9KDBInfo      * const pKDBInfo,              /**< Pointer to the Keyboard Information data structure (ET9KDBInfo). */
    const ET9U16            wKdbNum,               /**< KDB to be loaded. */
    const ET9U16            wPageNum               /**< Page to be loaded. */
);

#ifdef ET9_DIRECT_KDB_ACCESS

/**
 * Interface for Keyboard Input Module requests for keyboard data.
 */

typedef ET9STATUS (ET9FARCALL *ET9KDBREADCALLBACK)(
    ET9KDBInfo             *pKDBInfo,               /**< Pointer to the Keyboard Information data structure (ET9KDBInfo). */
    ET9U8 * ET9FARDATA *    ppbSrc,                 /**< Pointer to the pointer of the supplied buffer that is directly accessed by the integration layer. */
    ET9U32                 *pdwSizeInBytes          /**< Pointer to the size of the KDB in bytes. */
);

#else /* ET9_DIRECT_KDB_ACCESS */

/**
 * Interface for Keyboard Input Module requests for keyboard data.
 */

typedef ET9STATUS (ET9FARCALL *ET9KDBREADCALLBACK)(
    ET9KDBInfo *pKDBInfo,                           /**< Pointer to the Keyboard Information data structure (ET9KDBInfo). */
    ET9U32      dwOffset,                           /**< Offset from the beginning of the database to the start of the data that XT9 requires.
                                                         This value indicates where the integration layer must begin reading in the database. */

    ET9U32      dwNumberOfBytesToRead,              /**< Number of bytes the integration layer should read from the database and write to pbDest. */
    ET9U8      *pbDest,                             /**< Pointer to a buffer that will contain the requested data. */
    ET9U32     *pdwNumberOfBytesRead                /**< Pointer to a value indicating the number of bytes of data that were actually read from
                                                         the database and written to pbDest. */

);

#endif /* ET9_DIRECT_KDB_ACCESS */

/**
 * KDB state bits
 */

typedef enum ET9_KDB_STATEBITS_e {
    ET9_KDB_AMBIGUOUS = 0,    /**< ambiguous */
    ET9_KDB_MULTITAP,         /**< multitap */
    ET9_KDB_INSERT,           /**< insert */
    ET9_KDB_DISCRETE          /**< discrete*/

} ET9_KDB_STATEBITS;

typedef enum ET9_KDB_STATEBITSMASK_e {
    ET9_KDB_AMBIGUOUS_MODE_MASK = ((ET9U32)(1L << ET9_KDB_AMBIGUOUS)),
    ET9_KDB_MULTITAP_MODE_MASK  = ((ET9U32)(1L << ET9_KDB_MULTITAP)),
    ET9_KDB_INSERT_MASK         = ((ET9U32)(1L << ET9_KDB_INSERT)),
    ET9_KDB_DISCRETE_MASK       = ((ET9U32)(1L << ET9_KDB_DISCRETE))

} ET9_KDB_STATEBITSMASK;


#define ET9_KDB_AMBIGUOUS_MODE(dwState)     ((dwState & ET9_KDB_AMBIGUOUS_MODE_MASK) != 0)
#define ET9_KDB_MULTITAP_MODE(dwState)      ((dwState & ET9_KDB_MULTITAP_MODE_MASK) != 0)
#define ET9_KDB_INSERT_MODE(dwState)        ((dwState & ET9_KDB_INSERT_MASK) != 0)
#define ET9_KDB_DISCRETE_MODE(dwState)      ((dwState & ET9_KDB_DISCRETE_MASK) != 0)
#define ET9_KDB_REGIONAL_MODE(dwState)      ((dwState & ET9_KDB_DISCRETE_MASK) == 0)

/**
 * Handle request for KDB input module
 */

typedef enum ET9_KDB_REQ_e {
    ET9_KDB_REQ_NONE,           /**< 0: nop */
    ET9_KDB_REQ_TIMEOUT,        /**< 1: request a timer be set */
    ET9_KDB_REQ_PAGE_LOADED     /**< 2: inform that a keyboard layout just got loaded */

} ET9_KDB_REQ;


/**
 * Handle timer for KDB input module
 */

typedef enum ET9_KDB_TMR_e {
    ET9_KDB_TMRNONE,            /**< 0: undefined */
    ET9_KDB_TMRMULT             /**< 1: multiTap timer */

} ET9_KDB_TMR;


typedef struct ET9KDB_ReqSetTimeout_s {
    ET9_KDB_TMR   eTimerType;   /**< type of timer requested */
    ET9INT        nTimerID;     /**< currently only one timer is active at a time */

} ET9KDB_ReqSetTimeout;


typedef struct ET9KDB_ReqGetSymbol_s {
    ET9U16  wKeyId;
    ET9SYMB sSymbol;

} ET9KDB_ReqGetSymbol;

typedef struct ET9KDB_Request_s {
    ET9_KDB_REQ                 eType;              /**< ET9 KDB input module request type */
    union {
        ET9U32                  ulReturn;           /**< any return request value that can be fitted within 32-bit quantity */
        ET9KDB_ReqSetTimeout    sTimeout;           /**<  */
        ET9KDB_ReqGetSymbol     sMultitapSymbol;    /**<  */
    } data;

} ET9KDB_Request;


typedef ET9STATUS (ET9FARCALL *ET9KDBREQUESTCALLBACK)(ET9KDBInfo      * const pKDBInfo,
                                                      ET9WordSymbInfo * const pWordSymbInfo,
                                                      ET9KDB_Request  * const pET9KDB_Request);

/**
 * Max number of multitap symbols (in a single sequence).
 */

#define ET9_KDB_MAX_MT_SYMBS 32


/** \internal
 */

typedef struct ET9KDBRegionHdr_s {
    ET9U8  bRegional;
    ET9U16 wLeft;
    ET9U16 wTop;
    ET9U16 wRight;
    ET9U16 wBottom;
    ET9U8  bTotalRegions;
    ET9U32 dwRegionHdrOffset;
    ET9U8  bTotalBlockRows;
    ET9U8  bTotalBlockCols;
    ET9U16 wBlockWidth;
    ET9U16 wBlockHeight;
    ET9U8  bBlockX;
    ET9U8  bBlockY;

} ET9KDBRegionHdr;


/** \internal
 */

typedef struct ET9KDBPageHdr_s {
    ET9U16 wLeft;
    ET9U16 wTop;
    ET9U16 wRight;
    ET9U16 wBottom;
    ET9U8  bTotalRegions;
    ET9U32 dwRegionHdrOffset;

} ET9KDBPageHdr;

/** \internal
 */

typedef struct ET9KDBAction_s {

    ET9BOOL   bIsKeyAction;
    ET9U8     bShiftState;
    ET9U8     bCurrIndexInList;
    ET9U32    dwCurrChecksum;

    /* shared areas */

    union
    {
        struct
        {
            ET9U16      wKeyIndex;
        } keyAction;
        struct
        {
            ET9U16      wX;
            ET9U16      wY;
        } tapAction;
    } u;
} ET9KDBAction;

/*---------------------------------------------------------------------------*/
/** \internal
 * XML value suffix
 */

typedef enum ET9_xml_valueSuffix_e {

    ET9_xml_valueSuffix_undef = 0,

    ET9_xml_valueSuffix_percent,
    ET9_xml_valueSuffix_pixelsDI,

    ET9_xml_valueSuffix_Last

} ET9_xml_valueSuffix;

/*---------------------------------------------------------------------------*/
/** \internal
 * XML key type
 */

typedef enum ET9_xml_keyType_e {

    ET9_xml_keyType_undef = 0,

    ET9_xml_keyType_regional,
    ET9_xml_keyType_nonRegional,
    ET9_xml_keyType_smartPunct,
    ET9_xml_keyType_string,
    ET9_xml_keyType_function,

    ET9_xml_keyType_void,

    ET9_xml_keyType_Last

} ET9_xml_keyType;

/*---------------------------------------------------------------------------*/
/** \internal
 * XML row type
 */

typedef enum ET9_xml_rowType_e {

    ET9_xml_rowType_undef = 0,

    ET9_xml_rowType_area,
    ET9_xml_rowType_keys,

    ET9_xml_rowType_void,

    ET9_xml_rowType_Last

} ET9_xml_rowType;

/*---------------------------------------------------------------------------*/
/** \internal
 * XML supports exact
 */

typedef enum ET9_xml_booleanValue_e {

    ET9_xml_booleanValue_undef = 0,

    ET9_xml_booleanValue_no,
    ET9_xml_booleanValue_yes,

    ET9_xml_booleanValue_Last

} ET9_xml_booleanValue;

/*---------------------------------------------------------------------------*/
/** \internal
 * Structure holding KDB xml key information.
 */

typedef struct ET9KdbXmlKey_s {

    ET9UINT                     nULX;                                           /**< \internal */
    ET9UINT                     nULY;                                           /**< \internal */

    ET9UINT                     nWidth;                                         /**< \internal */

    ET9UINT                     nTop;                                           /**< \internal */
    ET9UINT                     nLeft;                                          /**< \internal */
    ET9UINT                     nHeight;                                        /**< \internal */

    ET9UINT                     nLeftGap;                                       /**< \internal */
    ET9UINT                     nRightGap;                                      /**< \internal */


    ET9UINT                     nSourceLine;                                    /**< \internal */

    ET9FLOAT                    fKeyTop;                                        /**< \internal */
    ET9_xml_valueSuffix         eKeyTopSuffix;                                  /**< \internal */

    ET9FLOAT                    fKeyLeft;                                       /**< \internal */
    ET9_xml_valueSuffix         eKeyLeftSuffix;                                 /**< \internal */

    ET9FLOAT                    fKeyWidth;                                      /**< \internal */
    ET9_xml_valueSuffix         eKeyWidthSuffix;                                /**< \internal */

    ET9FLOAT                    fKeyHeight;                                     /**< \internal */
    ET9_xml_valueSuffix         eKeyHeightSuffix;                               /**< \internal */

    ET9FLOAT                    fVoidSize;                                      /**< \internal */
    ET9_xml_valueSuffix         eVoidSizeSuffix;                                /**< \internal */

    ET9FLOAT                    fHorizontalGap;                                 /**< \internal */
    ET9_xml_valueSuffix         eHorizontalGapSuffix;                           /**< \internal */

    ET9_xml_keyType             eKeyType;                                       /**< \internal */

    ET9UINT                     nIconCharCount;                                 /**< \internal */
    ET9SYMB                     *psIconChars;                                   /**< \internal */

    ET9UINT                     nLabelCharCount;                                /**< \internal */
    ET9SYMB                     *psLabelChars;                                  /**< \internal */

    ET9UINT                     nLabelShiftedCharCount;                         /**< \internal */
    ET9SYMB                     *psLabelShiftedChars;                           /**< \internal */

    ET9UINT                     nKeyCharCount;                                  /**< \internal */
    ET9SYMB                     *psKeyChars;                                    /**< \internal */

    ET9UINT                     nKeyShiftedCharCount;                           /**< \internal */
    ET9SYMB                     *psKeyShiftedChars;                             /**< \internal */

    ET9UINT                     nKeyPopupCharCount;                             /**< \internal */
    ET9SYMB                     *psKeyPopupChars;                               /**< \internal */

    ET9UINT                     nKeyPopupShiftedCharCount;                      /**< \internal */
    ET9SYMB                     *psKeyPopupShiftedChars;                        /**< \internal */

    ET9UINT                     nKeyMultitapCharCount;                          /**< \internal */
    ET9SYMB                     *psKeyMultitapChars;                            /**< \internal */

    ET9UINT                     nKeyMultitapShiftedCharCount;                   /**< \internal */
    ET9SYMB                     *psKeyMultitapShiftedChars;                     /**< \internal */

} ET9KdbXmlKey;                                                                 /**< \internal */

/*---------------------------------------------------------------------------*/
/** \internal
 * Structure holding KDB xml row information.
 */

typedef struct ET9KdbXmlRow_s {

    ET9_xml_rowType             eRowType;                                       /**< \internal */

    ET9UINT                     nHeight;                                        /**< \internal */

    ET9UINT                     nUpGap;                                         /**< \internal */
    ET9UINT                     nDownGap;                                       /**< \internal */

    ET9FLOAT                    fKeyHeight;                                     /**< \internal */
    ET9_xml_valueSuffix         eKeyHeightSuffix;                               /**< \internal */

    ET9FLOAT                    fVoidSize;                                      /**< \internal */
    ET9_xml_valueSuffix         eVoidSizeSuffix;                                /**< \internal */

    ET9FLOAT                    fVerticalGap;                                   /**< \internal */
    ET9_xml_valueSuffix         eVerticalGapSuffix;                             /**< \internal */

    ET9UINT                     nKeyCount;                                      /**< \internal */
    ET9KdbXmlKey                *pKeys;                                         /**< \internal */

} ET9KdbXmlRow;                                                                 /**< \internal */

/*---------------------------------------------------------------------------*/
/** \internal
 * Structure holding KDB xml keyboard information.
 */

typedef struct ET9KdbXmlKeyboard_s {

    ET9U8                       bPrimaryID;                                     /**< \internal */
    ET9U8                       bSecondaryID;                                   /**< \internal */
    ET9U8                       bMajorVersion;                                  /**< \internal */
    ET9U8                       bMinorVersion;                                  /**< \internal */

    ET9U16                      wLayoutWidth;                                   /**< \internal */
    ET9U16                      wLayoutHeight;                                  /**< \internal */

    ET9U16                      wConditionValueMax;                             /**< \internal */

    ET9UINT                     nVerticalBorder;                                /**< \internal */
    ET9UINT                     nHorizontalBorder;                              /**< \internal */

    ET9FLOAT                    fVerticalBorder;                                /**< \internal */
    ET9_xml_valueSuffix         eVerticalBorderSuffix;                          /**< \internal */

    ET9FLOAT                    fHorizontalBorder;                              /**< \internal */
    ET9_xml_valueSuffix         eHorizontalBorderSuffix;                        /**< \internal */

    ET9_xml_booleanValue        eSupportsExact;                                 /**< \internal */

    ET9UINT                     nRowCount;                                      /**< \internal */
    ET9KdbXmlRow                *pRows;                                         /**< \internal */

} ET9KdbXmlKeyboard;                                                            /**< \internal */

/*---------------------------------------------------------------------------*/
/** \internal
 * Structure holding KDB xml keyboard information.
 */

typedef struct ET9KdbXmlKeyboardInfo_s {

    ET9BOOL                     bAreaBased;                                     /**< \internal */
    ET9BOOL                     bAltCharsAdded;                                 /**< \internal */

    ET9KdbXmlKeyboard           sKeyboard;                                      /**< \internal */

    ET9UINT                     nUsedKeys;                                      /**< \internal */
    ET9KdbXmlKey                pKeyPool[ET9_KDB_MAX_KEYS];                     /**< \internal */

    ET9UINT                     nUsedRows;                                      /**< \internal */
    ET9KdbXmlRow                pRowPool[ET9_KDB_MAX_ROWS];                     /**< \internal */

    ET9UINT                     nUsedSymbs;                                     /**< \internal */
    ET9SYMB                     psSymbPool[ET9_KDB_XML_MAX_POOL_CHARS];         /**< \internal */

} ET9KdbXmlKeyboardInfo;                                                        /**< \internal */

/** \internal
 */

typedef struct ET9KDBPrivate_s {

    ET9BOOL                     bUsingDynamicKDB;                       /**< \internal If the active KDB is dynamic or not. */
    ET9BOOL                     bIsLoadingKDB;                          /**< \internal If the active KDB is dynamic or not. */

    ET9KDBLOADSTATES            eLoadState;                             /**< \internal Load state for dynamic KDB. */

    ET9U16                      wPageNum;                               /**< \internal active language keyboard page */
    ET9U16                      wPageKeyNum;                            /**< \internal active language keyboard page, number of keys */

    ET9CONVERTSYMBCALLBACK      pConvertSymb;                           /**< \internal pointer to ConvertSymb (input related use) */
    void                        *pConvertSymbInfo;                      /**< \internal pointer to ConvertSymb info (that gets passed back in the callback) */

    ET9KDBAction                sKdbAction;                             /**< \internal essential data for a KDB action (e.g. key or tap) */

    ET9BOOL                     bKDBLoaded;                             /**< \internal if a KDB is properly loaded or not */
    ET9U16                      wInfoInitOK;                            /**< \internal xxx */
    ET9U16                      wKDBInitOK;                             /**< \internal xxx */

    ET9UINT                     nLoadID;                                /**< \internal Layout load id. */

    ET9KdbLayoutInfo            *pCurrLayoutInfo;                       /**< \internal xxx */
    ET9KdbLayoutInfo            *pLastLayoutInfo;                       /**< \internal xxx */

    ET9KdbLayoutInfo            pLayoutInfos[ET9_KDB_MAX_PAGE_CACHE];   /**< \internal xxx */

    ET9U16                      wLayoutOffsetX;                         /**< \internal xxx */
    ET9U16                      wLayoutOffsetY;                         /**< \internal xxx */

    ET9U16                      wScaleToLayoutWidth;                    /**< \internal xxx */
    ET9U16                      wScaleToLayoutHeight;                   /**< \internal xxx */

    ET9U16                      wLocale;                                /**< \internal Locale language used for case conversion etc (for now this should only be use when the WSI isn't available) */

    ET9UINT                     nWmUseID;                               /**< \internal xxx */

    union
    {
        struct
        {
            ET9KdbLayoutExtraInfo   sLayoutExtraInfo;                                           /**< \internal xxx */
        } staticLoad;
        struct
        {
            ET9UINT                 nTraceExtPointCount;                                        /**< \internal xxx */
            ET9TracePointExt        pTraceExtPoints[ET9_TRACE_MAX_POINTS];                      /**< \internal xxx */
        } traceEvent;
        struct
        {
            ET9KdbXmlKeyboardInfo   sKeyboardInfo;                                              /**< \internal */

            ET9SYMB                 psIconChars[ET9_KDB_XML_MAX_ICON_CHARS];                    /**< \internal */
            ET9SYMB                 psLabelChars[ET9_KDB_XML_MAX_LABEL_CHARS];                  /**< \internal */
            ET9SYMB                 psLabelShiftedChars[ET9_KDB_XML_MAX_LABEL_CHARS];           /**< \internal */
            ET9SYMB                 psKeyChars[ET9_KDB_MAX_KEY_CHARS];                          /**< \internal */
            ET9SYMB                 psKeyShiftedChars[ET9_KDB_MAX_KEY_CHARS];                   /**< \internal */
            ET9SYMB                 psKeyPopupChars[ET9_KDB_XML_MAX_POPUP_CHARS];               /**< \internal */
            ET9SYMB                 psKeyPopupShiftedChars[ET9_KDB_XML_MAX_POPUP_CHARS];        /**< \internal */
            ET9SYMB                 psKeyMultitapChars[ET9_KDB_XML_MAX_MULTITAP_CHARS];         /**< \internal */
            ET9SYMB                 psKeyMultitapShiftedChars[ET9_KDB_XML_MAX_MULTITAP_CHARS];  /**< \internal */

            ET9SYMB                 psDedupe[ET9_KDB_XML_MAX_DEDUPE_CHARS];                     /**< \internal */

            ET9UINT                 nStringLen;                                                 /**< \internal xxx */
            ET9SYMB                 psString[ET9_KDB_XML_MAX_STRING_CHARS];                     /**< \internal xxx */
        } xmlReader;
    } wm;                                                               /**< \internal working memory, shared between different events */

    ET9INPUTSHIFTSTATE          eShiftState;                            /**< \internal Mirrors the current WSI shift state. */

    ET9INPUTSHIFTSTATE          eMTLastShiftState;                      /**< \internal last shift state used during multitap */
    ET9U8                       bMTLastSymbIndex;                       /**< \internal index of the last used multitap symbol */
    ET9U8                       bMTSymbCount;                           /**< \internal number of actual symbols to multitap between */
    ET9U8                       bMTSymbCountSrc;                        /**< \internal number of source multitap symbols (before convertions) */
    ET9U16                      wMTLastInput;                           /**< \internal Last input. */
    ET9KdbAreaInfo       const *pMTKdbKey;                              /**< \internal KDB key used to create this symb. */
    ET9SYMB                     sMTSymbs[ET9_KDB_MAX_MT_SYMBS];         /**< \internal list of actual multitap symbols */
    ET9SYMB                     sMTSymbsSrc[ET9_KDB_MAX_MT_SYMBS];      /**< \internal list of source multitap symbols (before conversion) */
    ET9SYMB                     sMTSymbsCnv[ET9_KDB_MAX_MT_SYMBS];      /**< \internal list of converted multitap symbols (after conversion) */

    ET9U8                       bCurrDiacriticState;                    /**< \internal current diacritic state */

    ET9FILTERSYMBCALLBACK       pFilterSymb;                            /**< \internal pointer to FilterSymb (input related use) */
    ET9FILTERSYMBRESETCALLBACK  pFilterSymbReset;                       /**< \internal pointer to FilterSymbReset (input related use) */
    ET9FILTERSYMBCOUNTCALLBACK  pFilterSymbCount;                       /**< \internal pointer to FilterSymbCount (input related use) */
    ET9FILTERSYMBNEXTCALLBACK   pFilterSymbNext;                        /**< \internal pointer to FilterSymbNext (input related use) */
    ET9FILTERSYMBGROUPCALLBACK  pFilterSymbGroup;                       /**< \internal pointer to FilterSymbGroup (input related use) */
    void                        *pFilterSymbInfo;                       /**< \internal pointer to FilterSymb info (that gets passed back in the callbacks) */

#ifdef ET9_DIRECT_KDB_ACCESS
    ET9U8  ET9FARDATA           *pKdbData;                              /**< \internal static property, pointer to a byte array holding the whole KDB */
    ET9U32                      dwKdbDataSize;                          /**< \internal static property, size of the whole KDB */
#endif /* ET9_DIRECT_KDB_ACCESS */

    ET9U16                      wLayoutWidth;                           /**< \internal layout width - static property, mirror of the one in layout info */
    ET9U16                      wLayoutHeight;                          /**< \internal layout height - static property, mirror of the one in layout info */

    ET9U16                      wPageArrayOffset;                       /**< \internal static property */
    ET9U32                      dwPageHdrOffset;                        /**< \internal static property */
    ET9KDBPageHdr               PageHeader;                             /**< \internal static property */
    ET9KDBRegionHdr             RegionHeader;                           /**< \internal static property */

} ET9KDBPrivate;                                                        /**< \internal */


/**
 * Keyboard info object.
 */

struct ET9KDBInfo_s {
    ET9U32                  dwStateBits;                /**< State bits. */
    ET9U16                  wFirstKdbNum;               /**< Current keyboard. */
    ET9U16                  wFirstPageNum;              /**< Current keyboard page. */
    ET9U16                  wSecondKdbNum;              /**< Current second language keyboard. */
    ET9U16                  wSecondPageNum;             /**< Current second language keyboard page. */
    ET9U16                  wKdbNum;                    /**< Active language keyboard. */
    ET9U16                  wTotalPages;                /**< Total number of keyboard pages. */
    void                    *pPublicExtension;          /**< Pointer for OEM extension. */
    ET9KDBLOADCALLBACK      pKDBLoadData;               /**< Callback for data load. */
    ET9KDBREADCALLBACK      ET9KDBReadData;             /**< Callback for data read. */
    ET9KDBREQUESTCALLBACK   ET9Handle_KDB_Request;      /**< Callback for KDB-related requests. */
    ET9KDBPrivate           Private;                    /**< Persistent memory for use by XT9. */
};


/**
 * Check if KDB input has an OK init.
 */

#define ET9KDB_InfoInit_OK(pKDBInfo) ((pKDBInfo != NULL) && (((ET9KDBInfo *)pKDBInfo)->Private.wInfoInitOK == ET9GOODSETUP))

#define ET9KDB_DBInit_OK(pKDBInfo) ((pKDBInfo != NULL) && (((ET9KDBInfo *)pKDBInfo)->Private.wKDBInitOK == ET9GOODSETUP))

ET9STATUS ET9FARCALL ET9KDB_Init(ET9KDBInfo                 * const pKDBInfo,
                                 const ET9U16                       wFirstKdbNum,
                                 const ET9U16                       wFirstPageNum,
                                 const ET9U16                       wSecondKdbNum,
                                 const ET9U16                       wSecondPageNum,
                                 const ET9KDBLOADCALLBACK           pKDBLoadData,
                                 const ET9KDBREADCALLBACK           pKDBReadData,
                                 const ET9KDBREQUESTCALLBACK        ET9Handle_KDB_Request,
                                 void                       * const pPublicExtension);

ET9STATUS ET9FARCALL ET9KDB_Validate(ET9KDBInfo         * const pKDBInfo,
                                     const ET9U16               wKdbNum,
                                     const ET9KDBLOADCALLBACK   pKDBLoadData,
                                     const ET9KDBREADCALLBACK   pKDBReadData);

ET9STATUS ET9FARCALL ET9KDB_GetKdbVersion(ET9KDBInfo   * const pKDBInfo,
                                          ET9SYMB      * const psKdbVerBuf,
                                          const ET9U16         wBufMaxSize,
                                          ET9U16       * const pwBufSize);

ET9STATUS ET9FARCALL ET9KDB_SetKdbNum(ET9KDBInfo   * const pKDBInfo,
                                      const ET9U16         wFirstKdbNum,
                                      const ET9U16         wFirstPageNum,
                                      const ET9U16         wSecondKdbNum,
                                      const ET9U16         wSecondPageNum);

ET9STATUS ET9FARCALL ET9KDB_GetKdbNum(ET9KDBInfo * const pKDBInfo,
                                      ET9U16     * const pwFirstKdbNum,
                                      ET9U16     * const pwSecondKdbNum);

ET9STATUS ET9FARCALL ET9KDB_SetPageNum(ET9KDBInfo   * const pKDBInfo,
                                       const ET9U16         wFirstPageNum,
                                       const ET9U16         wSecondPageNum);

ET9STATUS ET9FARCALL ET9KDB_GetPageNum(ET9KDBInfo   * const pKDBInfo,
                                       ET9U16       * const pwFirstPageNum,
                                       ET9U16       * const pwSecondPageNum);

ET9STATUS ET9FARCALL ET9KDB_TimeOut(ET9KDBInfo * const pKDBInfo,
                                    ET9WordSymbInfo * const pWordSymbInfo);

ET9STATUS ET9FARCALL ET9KDB_SetAmbigMode(ET9KDBInfo      * const pKDBInfo,
                                         const ET9U16            wFirstPageNum,
                                         const ET9U16            wSecondPageNum,
                                         ET9WordSymbInfo * const pWordSymbInfo);

ET9STATUS ET9FARCALL ET9KDB_SetMultiTapMode(ET9KDBInfo      * const pKDBInfo,
                                            const ET9U16            wFirstPageNum,
                                            const ET9U16            wSecondPageNum,
                                            ET9WordSymbInfo * const pWordSymbInfo);

ET9STATUS ET9FARCALL ET9KDB_SetRegionalMode(ET9KDBInfo * const pKDBInfo);

ET9STATUS ET9FARCALL ET9KDB_SetDiscreteMode(ET9KDBInfo * const pKDBInfo);

ET9STATUS ET9FARCALL ET9KDB_ProcessTap(ET9KDBInfo        * const pKDBInfo,
                                       ET9WordSymbInfo   * const pWordSymbInfo,
                                       const ET9U16              wX,
                                       const ET9U16              wY,
                                       const ET9U8               bCurrIndexInList,
                                       ET9SYMB           * const psFunctionKey);

ET9STATUS ET9FARCALL ET9KDB_ProcessKey(ET9KDBInfo        * const pKDBInfo,
                                       ET9WordSymbInfo   * const pWordSymbInfo,
                                       const ET9U16              wKeyIndex,
                                       const ET9U8               bCurrIndexInList,
                                       ET9SYMB           * const psFunctionKey);

ET9STATUS ET9FARCALL ET9KDB_ProcessKeyBySymbol(ET9KDBInfo      * const pKDBInfo,
                                               ET9WordSymbInfo * const pWordSymbInfo,
                                               const ET9SYMB           sSymbol,
                                               const ET9U8             bCurrIndexInList,
                                               ET9SYMB         * const psFunctionKey,
                                               const ET9BOOL           bInitialSymCheck);

ET9STATUS ET9FARCALL ET9KDB_NextDiacritic(ET9KDBInfo        * const pKDBInfo,
                                          ET9WordSymbInfo   * const pWordSymbInfo);

ET9STATUS ET9FARCALL ET9KDB_ReselectWord(ET9KDBInfo        * const pKDBInfo,
                                         ET9WordSymbInfo   * const pWordSymbInfo,
                                         ET9SYMB           * const psWord,
                                         const ET9U16              wWordLen);

ET9STATUS ET9FARCALL ET9KDB_SetConvertSymb(ET9KDBInfo           * const pKDBInfo,
                                           const ET9CONVERTSYMBCALLBACK pConvertSymb,
                                           void                 * const pConvertSymbInfo);

ET9STATUS ET9FARCALL ET9KDB_GetMultiTapSequence(ET9KDBInfo   * const pKDBInfo,
                                                ET9SYMB      * const psMultiTapSequenceBuf,
                                                const ET9U16         wBufSize,
                                                ET9U16       * const pwTotalSymbs,
                                                ET9U8        * const pbCurrentSelSymb);

ET9STATUS ET9FARCALL ET9KDB_SetKeyboardSize(ET9KDBInfo          * const pKDBInfo,
                                            const ET9U16                wLayoutWidth,
                                            const ET9U16                wLayoutHeight);

ET9STATUS ET9FARCALL ET9KDB_GetKeyboardDefaultSize(ET9KDBInfo       * const pKDBInfo,
                                                   ET9U16           * const pwLayoutWidth,
                                                   ET9U16           * const pwLayoutHeight);

ET9STATUS ET9FARCALL ET9KDB_SetKeyboardOffset(ET9KDBInfo          * const pKDBInfo,
                                              const ET9U16                wLayoutOffsetX,
                                              const ET9U16                wLayoutOffsetY);

ET9STATUS ET9FARCALL ET9KDB_GetKeyPositions(ET9KDBInfo             * const pKDBInfo,
                                            ET9KeyPoint            * const pPoints,
                                            const ET9UINT                  nMaxPointCount,
                                            ET9UINT                * const pnPointCount);

ET9STATUS ET9FARCALL ET9KDB_GetKeyPositionByTap(ET9KDBInfo             * const pKDBInfo,
                                                const ET9U16                   wX,
                                                const ET9U16                   wY,
                                                ET9KeyPoint            * const pPoint);

/* KDB dynamic load functions */

ET9STATUS ET9FARCALL ET9KDB_Load_SetProperties(ET9KDBInfo       * const pKDBInfo,
                                               const ET9U8              bMajorVersion,
                                               const ET9U8              bMinorVersion,
                                               const ET9U8              bPrimaryID,
                                               const ET9U8              bSecondaryID,
                                               const ET9U16             wLayoutWidth,
                                               const ET9U16             wLayoutHeight,
                                               const ET9U16             wTotalPages);

ET9STATUS ET9FARCALL ET9KDB_Load_AddKey(ET9KDBInfo          * const pKDBInfo,
                                        const ET9U16                wKeyIndex,
                                        const ET9LOADKEYTYPE        eKeyType,
                                        const ET9U16                wLeft,
                                        const ET9U16                wTop,
                                        const ET9U16                wRight,
                                        const ET9U16                wBottom,
                                        const ET9UINT               nCharCount,
                                        ET9SYMB       const * const psChars);

ET9STATUS ET9FARCALL ET9KDB_Load_AttachShiftedChars(ET9KDBInfo          * const pKDBInfo,
                                                    const ET9UINT               nShiftedCharCount,
                                                    ET9SYMB       const * const psShiftedChars);

ET9STATUS ET9FARCALL ET9KDB_Load_AttachMultitapInfo(ET9KDBInfo          * const pKDBInfo,
                                                    const ET9UINT               nCharMultitapCount,
                                                    ET9SYMB       const * const psMultitapChars,
                                                    const ET9UINT               nCharMultitapShiftedCount,
                                                    ET9SYMB       const * const psMultitapShiftedChars);

ET9STATUS ET9FARCALL ET9KDB_Load_TextKDB(ET9KDBInfo             * const pKDBInfo,
                                         const ET9U16                   wPageNum,
                                         ET9U8            const * const pbContent,
                                         const ET9U32                   dwContentLen,
                                         ET9UINT                * const pnErrorLine);

ET9STATUS ET9FARCALL ET9KDB_Load_XmlKDB(ET9KDBInfo             * const pKDBInfo,
                                        const ET9U16                   wLayoutWidth,
                                        const ET9U16                   wLayoutHeight,
                                        const ET9UINT                  nConditionValue,
                                        ET9U8            const * const pbContent,
                                        const ET9U32                   dwContentLen,
                                        ET9KdbXmlKeyboardInfo  * const pKeyboardLayout,
                                        ET9UINT                * const pnUnknownAttributes,
                                        ET9UINT                * const pnErrorLine);

ET9STATUS ET9FARCALL ET9KDB_Load_Reset(ET9KDBInfo   * const pKDBInfo);

#endif /* ET9_KDB_MODULE */

/*----------------------------------------------------------------------------
 *  End: Regional Input Module  ** REMOVABLE **
 *----------------------------------------------------------------------------*/


/*------------------------------------------------------------------------
 *  Define ET9 selection list functions
 *------------------------------------------------------------------------*/

#define ET9_NO_ACTIVE_INDEX    ((ET9U8)0xFF)                /**< The value for no active index (selection list). */

/*------------------------------------------------------------------------
 *  Define ET9 system structures and functions
 *------------------------------------------------------------------------*/

ET9STATUS ET9FARCALL ET9GetCodeVersion(ET9SYMB * const psCodeVerBuf,
                                       const ET9U16    wBufMaxSize,
                                       ET9U16  * const pwBufSize);

/*------------------------------------------------------------------------
 *  Primary keyboard data base indentifier enumeration
 *------------------------------------------------------------------------*/

#define PKDBID_MASK ((ET9U16)0x00FF)

typedef enum ET9PKDBID_e {
    PKDBID_NONE,
    PKDBID_English

} ET9PKDBID;


/*------------------------------------------------------------------------
 *  Secondary keyboard data base indentifier definitions
 *------------------------------------------------------------------------*/

#define SKDBID_MASK ((ET9U16)0xFF00)

typedef enum ET9SKDBID_e {
    SKDBID_NONE

} ET9SKDBID;


/*----------------------------------------------------------------------------
 *  Primary language data base indentifier enumeration
 *----------------------------------------------------------------------------*/

#define ET9PLIDMASK             ((ET9U16)0x00FF)

#define ET9PLIDNone             ((ET9U16)0x0000)
#define ET9PLIDArabic           ((ET9U16)0x0001)
#define ET9PLIDBulgarian        ((ET9U16)0x0002)
#define ET9PLIDCatalan          ((ET9U16)0x0003)
#define ET9PLIDChinese          ((ET9U16)0x0004)
#define ET9PLIDCzech            ((ET9U16)0x0005)
#define ET9PLIDDanish           ((ET9U16)0x0006)
#define ET9PLIDGerman           ((ET9U16)0x0007)
#define ET9PLIDGreek            ((ET9U16)0x0008)
#define ET9PLIDEnglish          ((ET9U16)0x0009)
#define ET9PLIDSpanish          ((ET9U16)0x000A)
#define ET9PLIDFinnish          ((ET9U16)0x000B)
#define ET9PLIDFrench           ((ET9U16)0x000C)
#define ET9PLIDHebrew           ((ET9U16)0x000D)
#define ET9PLIDHungarian        ((ET9U16)0x000E)
#define ET9PLIDIcelandic        ((ET9U16)0x000F)
#define ET9PLIDItalian          ((ET9U16)0x0010)
#define ET9PLIDJapanese         ((ET9U16)0x0011)
#define ET9PLIDKorean           ((ET9U16)0x0012)
#define ET9PLIDDutch            ((ET9U16)0x0013)
#define ET9PLIDNorwegian        ((ET9U16)0x0014)
#define ET9PLIDPolish           ((ET9U16)0x0015)
#define ET9PLIDPortuguese       ((ET9U16)0x0016)
#define ET9PLIDRhaetoRomance    ((ET9U16)0x0017)
#define ET9PLIDRomanian         ((ET9U16)0x0018)
#define ET9PLIDRussian          ((ET9U16)0x0019)
#define ET9PLIDSerboCroatian    ((ET9U16)0x001A)
#define ET9PLIDSlovak           ((ET9U16)0x001B)
#define ET9PLIDAlbanian         ((ET9U16)0x001C)
#define ET9PLIDSwedish          ((ET9U16)0x001D)
#define ET9PLIDThai             ((ET9U16)0x001E)
#define ET9PLIDTurkish          ((ET9U16)0x001F)
#define ET9PLIDUrdu             ((ET9U16)0x0020)
#define ET9PLIDIndonesian       ((ET9U16)0x0021)
#define ET9PLIDUkrainian        ((ET9U16)0x0022)
#define ET9PLIDBelarusian       ((ET9U16)0x0023)
#define ET9PLIDSlovenian        ((ET9U16)0x0024)
#define ET9PLIDEstonian         ((ET9U16)0x0025)
#define ET9PLIDLatvian          ((ET9U16)0x0026)
#define ET9PLIDLithuanian       ((ET9U16)0x0027)
#define ET9PLIDMaori            ((ET9U16)0x0028)
#define ET9PLIDFarsi            ((ET9U16)0x0029)
#define ET9PLIDVietnamese       ((ET9U16)0x002A)
#define ET9PLIDLao              ((ET9U16)0x002B)
#define ET9PLIDKhmer            ((ET9U16)0x002C)
#define ET9PLIDBasque           ((ET9U16)0x002D)
#define ET9PLIDSorbian          ((ET9U16)0x002E)
#define ET9PLIDMacedonian       ((ET9U16)0x002F)
#define ET9PLIDSutu             ((ET9U16)0x0030)
#define ET9PLIDTsonga           ((ET9U16)0x0031)
#define ET9PLIDTswana           ((ET9U16)0x0032)
#define ET9PLIDVenda            ((ET9U16)0x0033)
#define ET9PLIDXhosa            ((ET9U16)0x0034)
#define ET9PLIDZulu             ((ET9U16)0x0035)
#define ET9PLIDAfrikaans        ((ET9U16)0x0036)
#define ET9PLIDFaeroese         ((ET9U16)0x0038)
#define ET9PLIDHindi            ((ET9U16)0x0039)
#define ET9PLIDMaltese          ((ET9U16)0x003A)
#define ET9PLIDSami             ((ET9U16)0x003B)
#define ET9PLIDScotsGaelic      ((ET9U16)0x003C)
#define ET9PLIDMalay            ((ET9U16)0x003E)
#define ET9PLIDSwahili          ((ET9U16)0x0041)
#define ET9PLIDAfar             ((ET9U16)0x0042)
#define ET9PLIDAbkhazian        ((ET9U16)0x0043)
#define ET9PLIDAmharic          ((ET9U16)0x0044)
#define ET9PLIDAssamese         ((ET9U16)0x0045)
#define ET9PLIDAymara           ((ET9U16)0x0046)
#define ET9PLIDAzerbaijani      ((ET9U16)0x0047)
#define ET9PLIDBashkir          ((ET9U16)0x0048)
#define ET9PLIDBihari           ((ET9U16)0x0049)
#define ET9PLIDBislama          ((ET9U16)0x004a)
#define ET9PLIDBengali          ((ET9U16)0x004b)
#define ET9PLIDTibetan          ((ET9U16)0x004c)
#define ET9PLIDBreton           ((ET9U16)0x004d)
#define ET9PLIDCorsican         ((ET9U16)0x004e)
#define ET9PLIDWelsh            ((ET9U16)0x004f)
#define ET9PLIDBhutani          ((ET9U16)0x0050)
#define ET9PLIDEsperanto        ((ET9U16)0x0051)
#define ET9PLIDFiji             ((ET9U16)0x0052)
#define ET9PLIDFrisian          ((ET9U16)0x0053)
#define ET9PLIDIrish            ((ET9U16)0x0054)
#define ET9PLIDGalician         ((ET9U16)0x0055)
#define ET9PLIDGuarani          ((ET9U16)0x0056)
#define ET9PLIDGujarati         ((ET9U16)0x0057)
#define ET9PLIDHausa            ((ET9U16)0x0058)
#define ET9PLIDCroatian         ((ET9U16)0x0059)
#define ET9PLIDArmenian         ((ET9U16)0x005a)
#define ET9PLIDInterlingua      ((ET9U16)0x005b)
#define ET9PLIDInterlingue      ((ET9U16)0x005c)
#define ET9PLIDInupiak          ((ET9U16)0x005d)
#define ET9PLIDInuktitut        ((ET9U16)0x005e)
#define ET9PLIDJavanese         ((ET9U16)0x005f)
#define ET9PLIDGeorgian         ((ET9U16)0x0060)
#define ET9PLIDKazakh           ((ET9U16)0x0061)
#define ET9PLIDGreenlandic      ((ET9U16)0x0062)
#define ET9PLIDKannada          ((ET9U16)0x0063)
#define ET9PLIDKashmiri         ((ET9U16)0x0064)
#define ET9PLIDKurdish          ((ET9U16)0x0065)
#define ET9PLIDKirghiz          ((ET9U16)0x0066)
#define ET9PLIDLatin            ((ET9U16)0x0067)
#define ET9PLIDLingala          ((ET9U16)0x0068)
#define ET9PLIDMalagasy         ((ET9U16)0x0069)
#define ET9PLIDMalayalam        ((ET9U16)0x006a)
#define ET9PLIDMongolian        ((ET9U16)0x006b)
#define ET9PLIDMoldavian        ((ET9U16)0x006c)
#define ET9PLIDMarathi          ((ET9U16)0x006d)
#define ET9PLIDBurmese          ((ET9U16)0x006e)
#define ET9PLIDNauru            ((ET9U16)0x006f)
#define ET9PLIDNepali           ((ET9U16)0x0070)
#define ET9PLIDOccitan          ((ET9U16)0x0071)
#define ET9PLIDOromo            ((ET9U16)0x0072)
#define ET9PLIDOriya            ((ET9U16)0x0073)
#define ET9PLIDPunjabi          ((ET9U16)0x0074)
#define ET9PLIDPashto           ((ET9U16)0x0075)
#define ET9PLIDQuechua          ((ET9U16)0x0076)
#define ET9PLIDKirundi          ((ET9U16)0x0077)
#define ET9PLIDKiyarwanda       ((ET9U16)0x0078)
#define ET9PLIDSanskrit         ((ET9U16)0x0079)
#define ET9PLIDSindhi           ((ET9U16)0x007a)
#define ET9PLIDSangho           ((ET9U16)0x007b)
#define ET9PLIDSinhala          ((ET9U16)0x007c)
#define ET9PLIDSamoan           ((ET9U16)0x007d)
#define ET9PLIDShona            ((ET9U16)0x007e)
#define ET9PLIDSomali           ((ET9U16)0x007f)
#define ET9PLIDSerbian          ((ET9U16)0x0080)
#define ET9PLIDSiswati          ((ET9U16)0x0081)
#define ET9PLIDSesotho          ((ET9U16)0x0082)
#define ET9PLIDSudanese         ((ET9U16)0x0083)
#define ET9PLIDTamil            ((ET9U16)0x0084)
#define ET9PLIDTelugu           ((ET9U16)0x0085)
#define ET9PLIDTajik            ((ET9U16)0x0086)
#define ET9PLIDTigrinya         ((ET9U16)0x0087)
#define ET9PLIDTurkmen          ((ET9U16)0x0088)
#define ET9PLIDTagalog          ((ET9U16)0x0089)
#define ET9PLIDSetswana         ((ET9U16)0x008a)
#define ET9PLIDTonga            ((ET9U16)0x008b)
#define ET9PLIDTatar            ((ET9U16)0x008c)
#define ET9PLIDTwi              ((ET9U16)0x008d)
#define ET9PLIDUyghur           ((ET9U16)0x008e)
#define ET9PLIDUzbek            ((ET9U16)0x008f)
#define ET9PLIDVolapuk          ((ET9U16)0x0090)
#define ET9PLIDWolof            ((ET9U16)0x0091)
#define ET9PLIDYiddish          ((ET9U16)0x0092)
#define ET9PLIDYoruba           ((ET9U16)0x0093)
#define ET9PLIDZhuang           ((ET9U16)0x0094)
#define ET9PLIDIgbo             ((ET9U16)0x0095)
#define ET9PLIDTamazight        ((ET9U16)0x0096)
#define ET9PLIDBosnian          ((ET9U16)0x0097)
#define ET9PLIDDari             ((ET9U16)0x0098)
#define ET9PLIDSundanese        ((ET9U16)0x0099)
#define ET9PLIDCebuano          ((ET9U16)0x009a)
#define ET9PLIDNavajo           ((ET9U16)0x009b)
#define ET9PLIDHawaiian         ((ET9U16)0x009c)

/* language ID from 0x009D through 00BF are reserved for future */

/* language ID from 0x00C0 through 00CF are reserved for Alternate alphabets for existing languages */

#define ET9PLIDSerbianCyrillic  ((ET9U16)0x00C0)    /* Serbian in Cyrillic script */
#define ET9PLIDTurkmenCyrillic  ((ET9U16)0x00C1)    /* Turkmen in Cyrillic script */

/* language ID from 0x00D0 through 00DF are hybrid languages */

#define ET9PLIDHinglish         ((ET9U16)0x00D0)
#define ET9PLIDSpanglish        ((ET9U16)0x00D1)

/* language ID 0x00E0 up are reserved to match Chinese T9 lang ids */

#define ET9PLIDChineseReservePoint  ((ET9U16)0x00E0)

#define ET9PLIDChineseTraditional   ((ET9U16)0x00E0)
#define ET9PLIDChineseSimplified    ((ET9U16)0x00E1)
#define ET9PLIDChineseHongkong      ((ET9U16)0x00E2)
#define ET9PLIDChineseSingapore     ((ET9U16)0x00E3)

#define ET9PLIDNull             ((ET9U16)0x00FF)

/*----------------------------------------------------------------------------
 *  Secondary language data base indentifier definitions
 *----------------------------------------------------------------------------*/

#define ET9SLIDMASK             ((ET9U16)0xFF00)
#define ET9SLIDNone             ((ET9U16)0x0000)
#define ET9SLIDDEFAULT          ((ET9U16)0x0100)
#define ET9SLIDCHINESE          ((ET9U16)0x0400)
#define ET9SLIDPinyinTrace      ((ET9U16)0x0500)
#define ET9SLIDBpmfTrace        ((ET9U16)0x0600)

/*----------------------------------------------------------------------------
 *  Secondary input data base indentifier definitions
 *----------------------------------------------------------------------------*/

#define ET9SKIDMASK             ((ET9U16)0xFF00)
#define ET9SKIDNone             ((ET9U16)0x0000)
#define ET9SKIDAccented         ((ET9U16)0x0100)
#define ET9SKIDEmoticon         ((ET9U16)0x0200)
#define ET9SKIDNumber           ((ET9U16)0x0300)
#define ET9SKIDSymbol           ((ET9U16)0x0400)
#define ET9SKIDReducedQwerty    ((ET9U16)0x0500)
#define ET9SKIDPhonePad         ((ET9U16)0x0600)
#define ET9SKIDQwerty           ((ET9U16)0x0700)
#define ET9SKIDATQwerty         ((ET9U16)0x0800)
#define ET9SKIDATQwertyReg      ((ET9U16)0x0900)
#define ET9SKIDPhonePadPinyin   ((ET9U16)0x0A00)
#define ET9SKIDPhonePadBpmf     ((ET9U16)0x0B00)
#define ET9SKIDPhonePadStroke   ((ET9U16)0x0C00)
#define ET9SKIDATQwertyPinyin   ((ET9U16)0x0D00)
#define ET9SKIDATQwertyBpmf     ((ET9U16)0x0E00)

/*----------------------------------------------------------------------------
 *  Language data base symbol class enumeration
 *----------------------------------------------------------------------------*/

/* Defined values for backwards compatibility with version 3.0.x */

#define SC_GenericLatin1    SC_ISO_Latin1


/*----------------------------------------------------------------------------
 *  Misc definition
 *----------------------------------------------------------------------------*/

#ifndef NULL
#ifdef __cplusplus
#define NULL    0
#else
#define NULL    ((void*)0)
#endif
#endif

#define ET9GOODSETUP   0x1428               /**< \internal a value representing "good" set up */

#define ET9TXTFILLSYM  ((ET9SYMB)0x20)      /**< \internal text buffer unused location fill value */

#ifdef ET9CHECKCOMPILE
enum {
    ET9NULL_POINTERS = -1,
    ET9WRONGSIZE_ET9U8 = 0,
    ET9WRONGSIZE_ET9U16,
    ET9WRONGSIZE_ET9U32,
    ET9WRONGSIZE_ET9UINT,
    ET9WRONGSIZE_ET9S8,
    ET9WRONGSIZE_ET9S16,
    ET9WRONGSIZE_ET9S32,
    ET9WRONGSIZE_ET9INT,
    ET9WRONGSIZE_ET9SYMB,
    ET9WRONGSIZE_ET9BOOL,
    ET9WRONGSIZE_ET9FARDATA,
    ET9WRONGSIZE_ET9FARCALL,
    ET9WRONGSIZE_ET9LOCALCALL,
    ET9WRONGSIZE_VOIDPOINTER,
    ET9WRONGSIZE_ET9SYMBINFO,
    ET9WRONGSIZE_ET9WORDSYMBINFO,
    ET9CHECKCOMPILE_NEXT
};

ET9U32 ET9FARCALL ET9_CheckCompileParameters(ET9U8   *pbET9U8,
                                             ET9U8   *pbET9U16,
                                             ET9U8   *pbET9U32,
                                             ET9U8   *pbET9UINT,
                                             ET9U8   *pbET9S8,
                                             ET9U8   *pbET9S16,
                                             ET9U8   *pbET9S32,
                                             ET9U8   *pbET9INT,
                                             ET9U8   *pbET9SYMB,
                                             ET9U8   *pbET9BOOL,
                                             ET9U8   *pbET9FARDATA,
                                             ET9U8   *pbET9FARCALL,
                                             ET9U8   *pbET9LOCALCALL,
                                             ET9U8   *pbVoidPtr,
                                             ET9U16  *pwET9SymbInfo,
                                             ET9UINT  *pwET9WordSymbInfo);

#endif

/*----------------------------------------------------------------------------
 *  Bring in edition specific API's
 *----------------------------------------------------------------------------*/

#ifdef ET9_KDB_MODULE
#ifdef ET9_KDB_TRACE_MODULE
#include "et9traceapi.h"
#endif /* ET9_KDB_TRACE_MODULE */
#endif /* ET9_KDB_MODULE */

#ifdef ET9_ALPHABETIC_MODULE
#include "et9awapi.h"
#ifdef ET9_JAPANESE_MODULE
#include "et9japi.h"
#endif /* ET9_JAPANESE_MODULE */
#endif /* ET9_ALPHABETIC_MODULE */

#ifdef ET9_CHINESE_MODULE
#include "et9cpapi.h"
#endif /* ET9_CHINESE_MODULE */

#ifdef ET9_KOREAN_MODULE
#include "et9kapi.h"
#endif /* ET9_KOREAN_MODULE */

#ifdef ET9_NAV_ALPHABETIC_MODULE
#include "et9navapi.h"
#endif /* ET9_NAV_ALPHABETIC_MODULE */

/*----------------------------------------------------------------------------
 *
 *----------------------------------------------------------------------------*/

/* End don't mangle the function name if compile under C++ */
#if defined (__cplusplus)
    }
#endif

/*! @} */
#endif /* #ifndef ET9API_H */
/* ----------------------------------< eof >--------------------------------- */

