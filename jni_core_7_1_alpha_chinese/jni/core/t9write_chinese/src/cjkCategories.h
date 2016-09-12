/*****************************************************************************
* COPYRIGHT ZI AND SUBJECT TO CONFIDENTIALITY RESTRICTIONS                   *
*                                                                            *
* This file is the confidential and proprietary property of Zi Corporation   *
* of Canada, Inc. ("Zi") or one of its affiliates.                           *
******************************************************************************/

#pragma once

#ifndef CJK_CATEGORIES_H_
#define CJK_CATEGORIES_H_

/**
 * @defgroup DOX_CATEGORIES cjkCategories
 * Engine Supported Character Categories
 * @{
 */

/**
 * @name Supported Categories
 * @{
 */
#define CJK_LATIN_LOWER          		0x00000001 /**< @hideinitializer */
#define CJK_LATIN_UPPER          		0x00000002 /**< @hideinitializer */
#define CJK_DIGIT                		0x00000004 /**< @hideinitializer */
#define CJK_PUNCTUATION          		0x00000008 /**< @hideinitializer */
#define CJK_GESTURE		          		0x00000010 /**< @hideinitializer */
#define CJK_SYMBOL               		0x00000020 /**< @hideinitializer */
#define CJK_GB2312_B_RADICALS    		0x00000080 /**< @hideinitializer */
#define CJK_GB2312_A			    			0x00000100 /**< @hideinitializer */
#define CJK_GB2312_B_CHARS       		0x00000200 /**< @hideinitializer */
#define CJK_BIGFIVE_LEVEL_1         	0x00000400 /**< @hideinitializer */
#define CJK_BIGFIVE_LEVEL_2				0x00000800 /**< @hideinitializer */
#define CJK_BOPOMOFO             		0x00001000 /**< @hideinitializer */
#define CJK_JIS_LEVEL_1          		0x00002000 /**< @hideinitializer */
#define CJK_JIS_LEVEL_2          		0x00004000 /**< @hideinitializer */
#define CJK_JIS_LEVEL_3          		0x00008000 /**< @hideinitializer */
#define CJK_JIS_LEVEL_4          		0x00010000 /**< @hideinitializer */
#define CJK_HIRAGANA             		0x00020000 /**< @hideinitializer */
#define CJK_KATAKANA             		0x00040000 /**< @hideinitializer */
#define CJK_HIRAGANASMALL        		0x00080000 /**< @hideinitializer */
#define CJK_KATAKANASMALL        		0x00100000 /**< @hideinitializer */
#define CJK_HKSCS                		0x00200000 /**< @hideinitializer */
#define CJK_HANGUL_A							0x00400000 /**< @hideinitializer */
#define CJK_HANGUL_B							0x00800000 /**< @hideinitializer */

/* Writing style categories */

#define CJK_PUNCTUATION_SINGLE_STROKE 	0x01000000 /**< @hideinitializer */
#define CJK_LATIN_LOWER_SINGLE_STROKE 	0x02000000 /**< @hideinitializer */
#define CJK_LATIN_UPPER_SINGLE_STROKE 	0x04000000 /**< @hideinitializer */
#define CJK_DIGIT_SINGLE_STROKE 			0x08000000 /**< @hideinitializer */
#define CJK_SYMBOL_SINGLE_STROKE 		0x00000040 /**< @hideinitializer */

#define CJK_POPULARFORM             	0x20000000 /**< @hideinitializer */
#define CJK_TRADSIMPDUAL         		0x40000000 /**< @hideinitializer */

/** @hideinitializer */
#define CJK_GB2312_B       			( CJK_GB2312_B_RADICALS | CJK_GB2312_B_CHARS )

/** @hideinitializer */
#define CJK_GB2312       				( CJK_GB2312_A | CJK_GB2312_B )

/** @hideinitializer */
#define CJK_LATIN								( CJK_LATIN_LOWER              \
                                    | CJK_LATIN_UPPER )

/** @hideinitializer */
#define CJK_HANGUL							( CJK_HANGUL_A                 \
                                    | CJK_HANGUL_B )

/** @hideinitializer */
#define CJK_BIGFIVE             ( CJK_BIGFIVE_LEVEL_1          \
                                    | CJK_BIGFIVE_LEVEL_2 )

/** @hideinitializer */
#define CJK_JIS0                ( CJK_JIS_LEVEL_1              \
	                                | CJK_JIS_LEVEL_2 )

/** @hideinitializer */
#define CJK_HAN                 ( CJK_GB2312									 \
																| CJK_BIGFIVE                  \
															  | CJK_JIS0                     \
						  									| CJK_HKSCS )

#endif /* CJK_CATEGORIES_H_ */
