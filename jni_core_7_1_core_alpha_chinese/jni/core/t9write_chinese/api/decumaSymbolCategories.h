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


#ifndef DECUMA_CATEGORY_SYMBOL_CATEGORIES_H_1234234
#define DECUMA_CATEGORY_SYMBOL_CATEGORIES_H_1234234


/*///////////////////////////////////////////////////////////////////// */
/*  Standard Decuma Categories                                       // */
/*///////////////////////////////////////////////////////////////////// */



/* --  Gesture Categories --  */

#define DECUMA_CATEGORY_WHITESPACE_STROKE                               100 /* The gesture for white space.  */
#define DECUMA_CATEGORY_RETURN_STROKE                                   101 /* The gesture for carriage return.  */
#define DECUMA_CATEGORY_BACKSPACE_STROKE                                102 /* The gesture for back space.  */
#define DECUMA_CATEGORY_TAB_STROKE                                      103 /* The gesture for tabular.  */
#define DECUMA_CATEGORY_EDIT_STROKE                                     104 /* The "v" gesture for opening up a space between characters.  */
#define DECUMA_CATEGORY_GESTURES		    															  105 /* All gestures */

#define DECUMA_CATEGORY_ARABIC_WHITESPACE_STROKE                        150 /* Arabic whitespace gesture (right-to-left).  */
#define DECUMA_CATEGORY_ARABIC_RETURN_STROKE                            151 /* Arabic carriage return gesture (left-to-right).  */
#define DECUMA_CATEGORY_ARABIC_BACKSPACE_STROKE                         152 /* Arabic back space gesture (left-to-right).  */
#define DECUMA_CATEGORY_ARABIC_TAB_STROKE                               153 /* Arabic tabular gesture (right-to-left).  */

#define DECUMA_CATEGORY_ARABIC_GESTURES                                 154 /* = ARABIC_RETURN_STROKE + ARABIC_BACKSPACE_STROKE + ARABIC_WHITESPACE_STROKE */

#define DECUMA_CATEGORY_MULTITOUCH_HORIZONTAL_2ARROW                    200 /* Multi-touch horizontal double arrow gestures.  */
#define DECUMA_CATEGORY_MULTITOUCH_VERTICAL_2ARROW                      201 /* Multi-touch vertical double arrow gestures.  */
#define DECUMA_CATEGORY_MULTITOUCH_2ARROW_GESTURES                      202 /* = MULTITOUCH_HORIZONTAL_2ARROW + MULTITOUCH_VERTICAL_2ARROW */

#define DECUMA_CATEGORY_MULTITOUCH_HORIZONTAL_3ARROW                    210 /* Multi-touch horizontal triple arrow gestures.  */
#define DECUMA_CATEGORY_MULTITOUCH_VERTICAL_3ARROW                      211 /* Multi-touch vertical triple arrow gestures.  */
#define DECUMA_CATEGORY_MULTITOUCH_3ARROW_GESTURES                      212 /* = MULTITOUCH_HORIZONTAL_3ARROW + MULTITOUCH_VERTICAL_3ARROW */

#define DECUMA_CATEGORY_MULTITOUCH_HORIZONTAL_4ARROW                    220 /* Multi-touch horizontal quadruple arrow gestures.  */
#define DECUMA_CATEGORY_MULTITOUCH_VERTICAL_4ARROW                      221 /* Multi-touch vertical quadruple arrow gestures.  */
#define DECUMA_CATEGORY_MULTITOUCH_4ARROW_GESTURES                      222 /* = MULTITOUCH_HORIZONTAL_4ARROW + MULTITOUCH_VERTICAL_4ARROW */

#define DECUMA_CATEGORY_MULTITOUCH_HORIZONTAL_SWIPE_GESTURES            230 /* = MULTITOUCH_HORIZONTAL_2ARROW + MULTITOUCH_HORIZONTAL_3ARROW + MULTITOUCH_HORIZONTAL_4ARROW */

#define DECUMA_CATEGORY_MULTITOUCH_VERTICAL_SWIPE_GESTURES              231 /* = MULTITOUCH_VERTICAL_2ARROW + MULTITOUCH_VERTICAL_3ARROW + MULTITOUCH_VERTICAL_4ARROW */

#define DECUMA_CATEGORY_MULTITOUCH_SWIPE_GESTURES                       232 /* = MULTITOUCH_2ARROW_GESTURES + MULTITOUCH_3ARROW_GESTURES + MULTITOUCH_4ARROW_GESTURES */

#define DECUMA_CATEGORY_MULTITOUCH_HORIZONTAL_ZOOM_IN                   240 /* Multi-touch horizontal zoom in gestures.  */
#define DECUMA_CATEGORY_MULTITOUCH_VERTICAL_ZOOM_IN                     241 /* Multi-touch vertical zoom in gestures.  */
#define DECUMA_CATEGORY_MULTITOUCH_ROTINV_ZOOM_IN                       242 /* Multi-touch rotational invariant zoom in gestures.  */
#define DECUMA_CATEGORY_MULTITOUCH_ZOOM_IN_GESTURES                     243 /* = MULTITOUCH_HORIZONTAL_ZOOM_IN + MULTITOUCH_VERTICAL_ZOOM_IN */

#define DECUMA_CATEGORY_MULTITOUCH_HORIZONTAL_ZOOM_OUT                  250 /* Multi-touch Horizontal Zoom Out gestures.  */
#define DECUMA_CATEGORY_MULTITOUCH_VERTICAL_ZOOM_OUT                    251 /* Multi-touch vertical zoom out gestures.  */
#define DECUMA_CATEGORY_MULTITOUCH_ROTINV_ZOOM_OUT                      252 /* Multi-touch rotational invariant zoom out gestures.  */
#define DECUMA_CATEGORY_MULTITOUCH_ZOOM_OUT_GESTURES                    253 /* = MULTITOUCH_HORIZONTAL_ZOOM_OUT + MULTITOUCH_VERTICAL_ZOOM_OUT */

#define DECUMA_CATEGORY_MULTITOUCH_ZOOM_GESTURES                        255 /* = MULTITOUCH_ZOOM_IN_GESTURES + MULTITOUCH_ZOOM_OUT_GESTURES */

#define DECUMA_CATEGORY_MULTITOUCH_GESTURES                             260 /* = MULTITOUCH_SWIPE_GESTURES + MULTITOUCH_ZOOM_GESTURES */


/* -- Character Relation Categories --  */

#define DECUMA_CATEGORY_WHITESPACE                                      500 /* This does not represent a written stroke but a space between two symbols.  */
#define DECUMA_CATEGORY_LINEFEED                                        501 /* This does not represent a written stroke but the next symbol is found on a new row..  */


/* -- Alphabetic Symbol Categories --  */

#define DECUMA_CATEGORY_LC_ANSI                                        1000 /*  */
#define DECUMA_CATEGORY_UC_ANSI                                        1001 /*  */
#define DECUMA_CATEGORY_ANSI                                           1002 /* = LC_ANSI + UC_ANSI */

#define DECUMA_CATEGORY_LC_ISO8859_1_SUPPLEMENTS                       1010 /*  */
#define DECUMA_CATEGORY_LC_ISO8859_1                                   1011 /* = LC_ANSI + LC_ISO8859_1_SUPPLEMENTS */
#define DECUMA_CATEGORY_UC_ISO8859_1_SUPPLEMENTS                       1012 /*  */
#define DECUMA_CATEGORY_UC_ISO8859_1                                   1013 /* = UC_ANSI + UC_ISO8859_1_SUPPLEMENTS */
#define DECUMA_CATEGORY_ISO8859_1                                      1014 /* = LC_ISO8859_1 + UC_ISO8859_1 */

#define DECUMA_CATEGORY_DIGIT                                          1101 /*  */

#define DECUMA_CATEGORY_EMAIL_SUPPLEMENTS                              1200 /*  */
#define DECUMA_CATEGORY_EMAIL                                          1201 /* = ANSI + DIGIT + EMAIL_SUPPLEMENTS */
#define DECUMA_CATEGORY_PHONE_NUMBER_SUPPLEMENTS                       1210 /*  */
#define DECUMA_CATEGORY_PHONE_NUMBER                                   1211 /* = DIGIT + PHONE_NUMBER_SUPPLEMENTS */
#define DECUMA_CATEGORY_URL_SUPPLEMENT                                 1220 /*  */
#define DECUMA_CATEGORY_URL                                            1221 /* = ANSI + DIGIT + URL_SUPPLEMENT */

#define DECUMA_CATEGORY_NUM_SUP                                        1230 /*  */
#define DECUMA_CATEGORY_ALT                                            1240 /* Not recommended together with ANSI.  */
#define DECUMA_CATEGORY_EXTRA                                          1241 /* Can be used together with ANSI.  */
#define DECUMA_CATEGORY_HORIZONTAL_SYMBOLS                             1242 /* Not recommended together with WHITESPACE_STROKE.  */

/* Used by Decuma Input Panel (Aa) mode */
#define DECUMA_CATEGORY_PUNCTUATIONS                                   1250 /*  */
/* Used by Decuma Input Panel (12) mode */
#define DECUMA_CATEGORY_NUMERIC_PUNCTUATIONS                           1251 /* Can be used with WHITESPACE_STROKE.  */
#define DECUMA_CATEGORY_SPANISH_PUNCTUATIONS                           1252 /*  */

#define DECUMA_CATEGORY_BASIC_PUNCTUATIONS                             1253 /* = PERIOD_COMMA_PUNCTUATIONS + COLON_PUNCTUATIONS + QUEST_EXCL_MARK_PUNCTUATIONS */
#define DECUMA_CATEGORY_PERIOD_COMMA_PUNCTUATIONS                      1255 /* Can be used together with ANSI.  */
#define DECUMA_CATEGORY_COLON_PUNCTUATIONS                             1256 /* Can be used together with ANSI.  */
#define DECUMA_CATEGORY_QUEST_EXCL_MARK_PUNCTUATIONS                   1257 /* Can be used together with ANSI.  */

#define DECUMA_CATEGORY_CONTRACTION_MARK                               1260 /* Can be used together with ANSI.  */
#define DECUMA_CATEGORY_AT_SIGN                                        1261 /* Can be used together with ANSI.  */
#define DECUMA_CATEGORY_SUPPLEMENTAL_PUNCTUATIONS                      1262 /* Can be used together with ANSI.  */


/* -- Arabic Symbol Categories --  */

#define DECUMA_CATEGORY_ARABIC_ISOLATED                                2000 /*  */

#define DECUMA_CATEGORY_ARABIC_STANDARD                                2010 /*  */
#define DECUMA_CATEGORY_ARABIC_PERCENT_SIGN                            2040 /* Arabic percent sign.  */
#define DECUMA_CATEGORY_ARABIC_TATWEEL                                 2050 /* Used to stretch arabic characters.  */

#define DECUMA_CATEGORY_ARABIC_DIGITS                                  2100 /*  */

#define DECUMA_CATEGORY_ARABIC_PUNCTUATIONS                            2200 /*  */
#define DECUMA_CATEGORY_ARABIC_BASIC_PUNCTUATIONS                      2210 /*	*/ 
#define DECUMA_CATEGORY_ARABIC_EXTRA_NUM_SYMBOLS                       2220 /* Some extra symbols that useful in digit mode. */
#define DECUMA_CATEGORY_ARABIC_EXTRA_ALPHABETIC_NUM_SYMBOLS            2230 /*	*/


/* -- Urdu Symbol Categories --  */

#define DECUMA_CATEGORY_URDU_ISOLATED                                  2300 /*  */

#define DECUMA_CATEGORY_EXTENDED_ARABIC_INDIC_DIGITS                   2350 /*  */


/* -- Farsi Symbol Categories --  */

#define DECUMA_CATEGORY_FARSI_ISOLATED                                 2400 /*  */


/* -- Hebrew Symbol Categories --  */

#define DECUMA_CATEGORY_HEBREW_CURSIVE                                 2500 /*  */

#define DECUMA_CATEGORY_HEBREW_CURSIVE_NUMERALS                        2550 /* Suitable for combination with GERESH (no conflict with Yod).  */

#define DECUMA_CATEGORY_HEBREW_GERESH                                  2620 /* Conflicts with 0x27.  */

#define DECUMA_CATEGORY_HEBREW_GERESH_GESTURE                          2621 /* Conflicts with 0x27 type 3.  */

#define DECUMA_CATEGORY_HEBREW_GERESHAYIM                              2630 /* Conflicts with 0x22.  */

#define DECUMA_CATEGORY_HEBREW_MAQAF                                   2640 /* Conflicts with gestures.  */

#define DECUMA_CATEGORY_HEBREW_SHEQEL                                  2650 /*  */


/* -- Cyrillic Symbol Categories --  */

#define DECUMA_CATEGORY_LC_CYRILLIC                                    3000 /*  */

#define DECUMA_CATEGORY_UC_CYRILLIC                                    3001 /*  */
#define DECUMA_CATEGORY_CYRILLIC                                       3002 /* = LC_CYRILLIC + UC_CYRILLIC */


/* -- Greek Symbol Categories --  */

#define DECUMA_CATEGORY_LC_GREEK                                       4000 /* Not recommended together with LC_ANSI or LC_CYRILLIC.  */
#define DECUMA_CATEGORY_UC_GREEK                                       4001 /* Not recommended together with UC_ANSI or UC_CYRILLIC.  */
#define DECUMA_CATEGORY_GREEK                                          4002 /* = LC_GREEK + UC_GREEK */


/* -- Thai Symbol Categories --  */

#define DECUMA_CATEGORY_THAI_NORMAL_CONSONANTS                         5000 /*  */
#define DECUMA_CATEGORY_THAI_VOWEL_COMPOSING_CONSONANTS                5001 /*  */
#define DECUMA_CATEGORY_THAI_CONSONANTS                                5002 /* = THAI_NORMAL_CONSONANTS + THAI_VOWEL_COMPOSING_CONSONANTS */
#define DECUMA_CATEGORY_THAI_OBSOLETE_CONSONANTS                       5003 /*  */

#define DECUMA_CATEGORY_THAI_INDEPENDENT_VOWELS                        5010 /*  */
#define DECUMA_CATEGORY_THAI_LEADING_VOWELS                            5011 /*  */
#define DECUMA_CATEGORY_THAI_FOLLOWING_VOWELS                          5012 /*  */
#define DECUMA_CATEGORY_THAI_BELOW_VOWELS                              5013 /*  */
#define DECUMA_CATEGORY_THAI_ABOVE_VOWELS                              5014 /*  */
#define DECUMA_CATEGORY_THAI_VOWELS                                    5015 /* = THAI_INDEPENDENT_VOWELS + THAI_LEADING_VOWELS + THAI_FOLLOWING_VOWELS + THAI_BELOW_VOWELS + THAI_ABOVE_VOWELS */
#define DECUMA_CATEGORY_THAI_OBSOLETE_VOWELS                           5016 /*  */

#define DECUMA_CATEGORY_THAI_TONES                                     5020 /*  */

#define DECUMA_CATEGORY_THAI_DIACRITICS                                5030 /*  */

#define DECUMA_CATEGORY_THAI_DIGITS                                    5040 /*  */

#define DECUMA_CATEGORY_THAI_SYMBOLS                                   5050 /*  */

#define DECUMA_CATEGORY_THAI                                           5060 /* = THAI_CONSONANTS + THAI_VOWELS + THAI_TONES + THAI_DIACRITICS + THAI_DIGITS + THAI_SYMBOLS */

#define DECUMA_CATEGORY_THAI_BASE                                      5061 /* = THAI_CONSONANTS + THAI_INDEPENDENT_VOWELS + THAI_LEADING_VOWELS + THAI_FOLLOWING_VOWELS + THAI_DIGITS + THAI_SYMBOLS */

#define DECUMA_CATEGORY_THAI_NON_BASE                                  5062 /* = THAI_BELOW_VOWELS + THAI_ABOVE_VOWELS + THAI_TONES + THAI_DIACRITICS */


/* -- CJK Symbol Categories */

#define DECUMA_CATEGORY_CJK_SYMBOL               															 6000 /* = CJK specific symbols */
#define DECUMA_CATEGORY_GB2312_A                															 6001 /* = 3755 most frequent characters in GB2312 */
#define DECUMA_CATEGORY_GB2312_B_RADICALS																		 6002 /* = GB2312_B unicodes that are radicals, i.e. part of characters */
#define DECUMA_CATEGORY_GB2312_B_CHARS_ONLY																	 6003 /* = GB2312_B - DECUMA_CATEGORY_GB2312_B_RADICALS */
#define DECUMA_CATEGORY_BIGFIVE_LEVEL_1          															 6004 /* = Bigfive level 1 characters (5401 chars) */
#define DECUMA_CATEGORY_BIGFIVE_LEVEL_2          															 6005 /* = Bigfive level 2 characters (7652 chars) */
#define DECUMA_CATEGORY_JIS_LEVEL_1            																 6006 /* = JIS X 0208 level 1 characters (2965 chars) */
#define DECUMA_CATEGORY_JIS_LEVEL_2            																 6007 /* = JIS X 0208 level 2 characters (3390 chars) */
#define DECUMA_CATEGORY_JIS_LEVEL_3            																 6008 /* = JIS X 0213 level 3 characters (1177 chars) */
																																						/*   32-bit to 16-bit private mapping according to Unicode 3.1/3.2 */
#define DECUMA_CATEGORY_JIS_LEVEL_4            																 6009 /* = JIS X 0213 level 4 characters (2427 chars) */
																																						/*   32-bit to 16-bit private mapping according to Unicode 3.1/3.2 */
#define DECUMA_CATEGORY_HIRAGANA             																 6010 /* = Hiragana characters */
#define DECUMA_CATEGORY_KATAKANA             																 6011 /* = Katakana characters */
#define DECUMA_CATEGORY_HIRAGANASMALL        																 6012 /* = Hiragana subscript characters */
#define DECUMA_CATEGORY_KATAKANASMALL        																 6013 /* = Katakana subscript characters */
#define DECUMA_CATEGORY_BOPOMOFO             																 6014 /* = Bopomofo characters */
#define DECUMA_CATEGORY_HKSCS_2001           																 6015 /* = 4310 (out of 4818) characters from HKSCS-2001 standard */
																																						/*   code mapping according to ISO/IEC 10646-1:2000 */
#define DECUMA_CATEGORY_HANGUL_1001_A	       																 6016 /* = 1000 most frequent hangul characters from KS X 1001 */
#define DECUMA_CATEGORY_HANGUL_1001_B	       																 6017 /* = 1350 less frequent hangul characters from KS X 1001 */

#define DECUMA_CATEGORY_LC_SINGLE_STROKE																	    6018 /* = Special single_stroke writing styles of lower case latin */
																																	/*   characters, particularly useful for full screen UIs */
#define DECUMA_CATEGORY_UC_SINGLE_STROKE																	    6019 /* = Special single_stroke writing styles of upper case latin */
																																	/*   characters, particularly useful for full screen UIs */
#define DECUMA_CATEGORY_DIGIT_SINGLE_STROKE																	 6020 /* = Special single_stroke writing styles of digits */
																																	/*   characters, particularly useful for full screen UIs */
#define DECUMA_CATEGORY_PUNCTUATION_SINGLE_STROKE															 6021 /* = Special single_stroke writing styles of some punctuations */
																																	/*   characters, particularly useful for full screen UIs */
#define DECUMA_CATEGORY_CJK_SYMBOL_SINGLE_STROKE															 6022 /* = Special single_stroke writing styles of some CJK symbol */
																																	/*   characters, particularly useful for full screen UIs */

/* -- CJK Writing style limiting categories */

#define DECUMA_CATEGORY_POPULAR_FORM          																 6030 /* = Non-standard but popular shapes for some characters */
#define DECUMA_CATEGORY_SIMPLIFIED_FORM         															 6031 /* = Simplified forms (PRC) in traditional writing (TW, HK) */
#define DECUMA_CATEGORY_TRADITIONAL_FORM         															 6032 /* = Traditional forms (TW) in simplified writing (PRC) */

/* -- CJK Combination categories */

#define DECUMA_CATEGORY_HANGUL			        																 6040 /* = HANGUL_1001_A, HANGUL_1001_B */
#define DECUMA_CATEGORY_BIGFIVE                  															 6041 /* = BIGFIVE_LEVEL_1, BIGFIVE_LEVEL_2 */
#define DECUMA_CATEGORY_JIS                     															 6042 /* = Any JIS_level* coded characters */
#define DECUMA_CATEGORY_GB2312_B                      													 6043 /* = GB2312_B_CHARS_ONLY, GB2312_B_RADICALS */
#define DECUMA_CATEGORY_GB2312                      														 6044 /* = GB2312_A, GB2312_B */
#define DECUMA_CATEGORY_HAN                      															 6045 /* = Any Chinese character (JIS, BIGFIVE, GB_2312) */


/*///////////////////////////////////////////////////////////////////// */
/*  Categories for databases designed for a special purpose          // */
/*///////////////////////////////////////////////////////////////////// */

/* CATEGORIES FOR DATABASES DESIGNED FOR A SPECIAL PURPOSE */
/* These databases are normally produced to support a certain UI configuration. */
/* Each category defined here exist for either of the reasons below: */
/* 1) To limit the character set in a way that is not supported by the */
/*   standard symbol categories */
/* 2) To facilitate for a UI implementation: Instead of having to set a lot */
/*    of different atomic categories, this category represents a union */
/*    of the atomic ones. */


/* -- Alphabetic Fullscreen database --  */

#define DECUMA_CATEGORY_ALPHABETIC_FS_LOWAREA                      10000000 /*  */
#define DECUMA_CATEGORY_ALPHABETIC_FS_MIDAREA                      10000001 /*  */
#define DECUMA_CATEGORY_ALPHABETIC_FS_NUMBERS                      10000002 /*  */

#define DECUMA_CATEGORY_ALPHABETIC_FS_EXTENDED_CHARS               10000010 /*  */

#define DECUMA_CATEGORY_ALPHABETIC_FS_EXTENDED_CHARS_LOWAREA       10000011 /*  */

#define DECUMA_CATEGORY_ALPHABETIC_FS_EXTENDED_CHARS_MIDAREA       10000012 /*  */

#define DECUMA_CATEGORY_ALPHABETIC_FS_EXTENDED_CHARS_TOPAREA       10000013 /*  */

#define DECUMA_CATEGORY_ALPHABETIC_FS_ACCENT_GRAVE                 10000020 /* The diacritic mark: accent grave.  */
#define DECUMA_CATEGORY_ALPHABETIC_FS_ACCENT_ACUTE                 10000021 /* The diacritic mark: accent acute.  */
#define DECUMA_CATEGORY_ALPHABETIC_FS_ACCENT_CIRCUMFLEX            10000022 /* The diacritic mark: circumflex.  */
#define DECUMA_CATEGORY_ALPHABETIC_FS_ACCENT_TILDE                 10000023 /* The diacritic mark: tilde.  */
#define DECUMA_CATEGORY_ALPHABETIC_FS_ACCENT_RING_ABOVE            10000024 /* The diacritic mark: ring above.  */

#define DECUMA_CATEGORY_VIPDATA_PUNCTUATIONS                       10001000 /*  */
#define DECUMA_CATEGORY_VIPDATA_NUMERIC                            10001001 /*  */

#define DECUMA_CATEGORY_VIPDATA_COMMAND_LETTERS                    10001010 /*  */
#define DECUMA_CATEGORY_VIPDATA_ONEARC_COMMANDS                    10001011 /*  */
#define DECUMA_CATEGORY_VIPDATA_COMMANDS                           10001012 /* = VIPDATA_COMMAND_LETTERS + VIPDATA_ONEARC_COMMANDS */

/* -- Arabic fullscreen database --  */

#define DECUMA_CATEGORY_ARABIC_FS_ALPHABETIC_GESTURES              10002000 /* = RETURN_STROKE + BACKSPACE_STROKE + WHITESPACE_STROKE + TAB_STROKE */
#define DECUMA_CATEGORY_ARABIC_FS_ARABIC_GESTURES                  10002001 /* = ARABIC_RETURN_STROKE + ARABIC_BACKSPACE_STROKE + ARABIC_WHITESPACE_STROKE + ARABIC_TAB_STROKE */

#define DECUMA_CATEGORY_ARABIC_FS_ALPHABETIC_DIGITSYMBOLS          10002010 /* Symbols to be used with alphabetic digits. Hyphen type 1.  */
#define DECUMA_CATEGORY_ARABIC_FS_ALPHABETIC_LETTERSYMBOLS         10002011 /* Symbols to be used with alphabetic letters. Comma type 4. Full Stop type 2. Quotation Mark type 3. Apostrophe type 2.  */
#define DECUMA_CATEGORY_ARABIC_FS_ARABIC_DIGITSYMBOLS              10002012 /* Symbols to be used with arabic digits. Arabic-Indic digit zero type 3 and 4. Hyphen type 3. Backslash type 2.  */
#define DECUMA_CATEGORY_ARABIC_FS_ARABIC_LETTERSYMBOLS             10002013 /* Symbols to be used with arabic letters. Quotation Mark type 3. Apostrophe type 2.  */

#define DECUMA_CATEGORY_ARABIC_FS_ALPHABETIC_TOPAREA               10002020 /* = DIGIT + ARABIC_FS_ALPHABETIC_DIGITSYMBOLS + BACKSPACE_STROKE + RETURN_STROKE + TAB_STROKE */
#define DECUMA_CATEGORY_ARABIC_FS_LC_ANSI                          10002021 /* Lowercase characters supported in alphabetic uc mode. Straight lowercase l excluded.  */
#define DECUMA_CATEGORY_ARABIC_FS_ALPHABETIC_MIDAREA               10002022 /* = UC_ANSI + ARABIC_FS_LC_ANSI + LC_ISO8859_1_SUPPLEMENTS + UC_ISO8859_1_SUPPLEMENTS + ARABIC_FS_ALPHABETIC_LETTERSYMBOLS + ARABIC_FS_ALPHABETIC_GESTURES */
#define DECUMA_CATEGORY_ARABIC_FS_ALPHABETIC_BOTTOMAREA            10002023 /* = ISO8859_1 + ARABIC_FS_ALPHABETIC_LETTERSYMBOLS + ARABIC_FS_ALPHABETIC_GESTURES */

#define DECUMA_CATEGORY_ARABIC_FS_ARABIC_TOPAREA                   10002030 /* = ARABIC_DIGITS + ARABIC_FS_ARABIC_DIGITSYMBOLS + ARABIC_BACKSPACE_STROKE + ARABIC_RETURN_STROKE + ARABIC_TAB_STROKE */
#define DECUMA_CATEGORY_ARABIC_FS_ARABIC_MIDAREA                   10002031 /* = ARABIC_ISOLATED + ARABIC_FS_ARABIC_LETTERSYMBOLS + ARABIC_FS_ARABIC_GESTURES + ARABIC_PUNCTUATIONS */
#define DECUMA_CATEGORY_ARABIC_FS_ARABIC_BOTTOMAREA                10002032 /* = ARABIC_ISOLATED + ARABIC_FS_ARABIC_LETTERSYMBOLS + ARABIC_FS_ARABIC_GESTURES + ARABIC_PUNCTUATIONS */

#define DECUMA_CATEGORY_ARABIC_FS_ARABIC_NUM_MODE                  10002050 /* = ARABIC_FS_ARABIC_TOPAREA */
#define DECUMA_CATEGORY_ARABIC_FS_ARABIC_ISOLATED_MODE             10002052 /* = ARABIC_FS_ARABIC_BOTTOMAREA */

/* -- Arabic standard MCR modes -- */
#define DECUMA_CATEGORY_ARABIC_ISOLATED_LETTER_MODE                10002100 /* Arabic, Farsi and Urdu standard letter mode. Can be combined with AT_SIGN and SUPPLEMENTAL_PUNCTUATIONS. = ARABIC_ISOLATED + ARABIC_BASIC_PUNCTUATIONS + ARABIC_GESTURES */
#define DECUMA_CATEGORY_ARABIC_NUM_MODE                            10002110 /* Arabic, Farsi and Urdu standard numerical mode. . = ARABIC_DIGITS + ARABIC_EXTRA_NUM_SYMBOLS + ARABIC_BACKSPACE_STROKE + ARABIC_RETURN_STROKE */
#define DECUMA_CATEGORY_ARABIC_ALPHABETIC_NUM_MODE                 10002120 /* Arabic, Farsi and Urdu standard alphabetic numerical mode. . = DIGIT + ARABIC_EXTRA_ALPHABETIC_NUM_SYMBOLS + BACKSPACE_STROKE + RETURN_STROKE */


/* -- Hebrew fullscreen database --  */

#define DECUMA_CATEGORY_HEBREW_FS_ALPHABETIC_DIGITSYMBOLS          10003012 /* Symbols to be used with arabic digits. Hyphen type 3. Full Stop type 3. Comma type 5.  */
#define DECUMA_CATEGORY_HEBREW_FS_HEBREW_LETTERSYMBOLS             10003013 /* Symbols to be used with Hebrew letters. Full Stop type 3. Comma type 5.  */

#define DECUMA_CATEGORY_HEBREW_FS_ALPHABETIC_TOPAREA               10003020 /* = DIGIT + HEBREW_FS_ALPHABETIC_DIGITSYMBOLS + BACKSPACE_STROKE + RETURN_STROKE + TAB_STROKE */
#define DECUMA_CATEGORY_HEBREW_FS_ALPHABETIC_MIDAREA               10003022 /* = UC_ANSI + ARABIC_FS_LC_ANSI + ARABIC_FS_ALPHABETIC_LETTERSYMBOLS + ARABIC_FS_ALPHABETIC_GESTURES */
#define DECUMA_CATEGORY_HEBREW_FS_ALPHABETIC_BOTTOMAREA            10003023 /* = ANSI + ARABIC_FS_ALPHABETIC_LETTERSYMBOLS + ARABIC_FS_ALPHABETIC_GESTURES */

#define DECUMA_CATEGORY_HEBREW_FS_HEBREW_TOPAREA                   10003030 /* = DIGIT + HEBREW_FS_ALPHABETIC_DIGITSYMBOLS + HEBREW_GERESH_GESTURE + HEBREW_GERESHAYIM + ARABIC_BACKSPACE_STROKE + ARABIC_RETURN_STROKE + ARABIC_TAB_STROKE */
#define DECUMA_CATEGORY_HEBREW_FS_HEBREW_MIDAREA                   10003031 /* = HEBREW_CURSIVE + HEBREW_FS_HEBREW_LETTERSYMBOLS + ARABIC_FS_ARABIC_GESTURES + BASIC_PUNCTUATIONS */
#define DECUMA_CATEGORY_HEBREW_FS_HEBREW_BOTTOMAREA                10003032 /* = HEBREW_CURSIVE + HEBREW_FS_HEBREW_LETTERSYMBOLS + ARABIC_FS_ARABIC_GESTURES + BASIC_PUNCTUATIONS */


/* -- Hebrew using guidelines --  */

#define DECUMA_CATEGORY_HEBREW_GL_ALPHABETIC_DIGITSYMBOLS          10003040 /* Symbols to be used with arabic digits. Hyphen type 1.  */
#define DECUMA_CATEGORY_HEBREW_GL_ALPHABETIC_LETTERSYMBOLS         10003041 /* Symbols to be used with alphabetic letters.  */
#define DECUMA_CATEGORY_HEBREW_GL_HEBREW_DIGITSYMBOLS              10003042 /* Symbols to be used with arabic digits. Hyphen type 3.  */
#define DECUMA_CATEGORY_HEBREW_GL_HEBREW_LETTERSYMBOLS             10003043 /* Symbols to be used with Hebrew letters.  */

#define DECUMA_CATEGORY_HEBREW_GL_ALPHABETIC_NUM_MODE              10003050 /* = DIGIT + HEBREW_GL_ALPHABETIC_DIGITSYMBOLS + BACKSPACE_STROKE + RETURN_STROKE + TAB_STROKE */
#define DECUMA_CATEGORY_HEBREW_GL_ALPHABETIC_UC_MODE               10003052 /* = UC_ANSI + HEBREW_GL_ALPHABETIC_LETTERSYMBOLS + ARABIC_FS_ALPHABETIC_GESTURES */
#define DECUMA_CATEGORY_HEBREW_GL_ALPHABETIC_LC_MODE               10003053 /* = LC_ANSI + HEBREW_GL_ALPHABETIC_LETTERSYMBOLS + ARABIC_FS_ALPHABETIC_GESTURES */

#define DECUMA_CATEGORY_HEBREW_GL_HEBREW_NUM_MODE                  10003060 /* = DIGIT + HEBREW_GL_HEBREW_DIGITSYMBOLS + HEBREW_GERESH + HEBREW_GERESHAYIM + ARABIC_BACKSPACE_STROKE + ARABIC_RETURN_STROKE + ARABIC_TAB_STROKE */
#define DECUMA_CATEGORY_HEBREW_GL_HEBREW_CURSIVE_MODE              10003061 /* Possible but not optimal to combine with HEBREW_MAQAF. = HEBREW_CURSIVE + HEBREW_GL_HEBREW_LETTERSYMBOLS + ARABIC_FS_ARABIC_GESTURES */


/* -- Additional Alphabetic fullscreen categories --  */
#define DECUMA_CATEGORY_ALPHABETIC_FS_ALPHABETIC_DIGITSYMBOLS      10004010 /* = ARABIC_FS_ALPHABETIC_DIGITSYMBOLS */

#define DECUMA_CATEGORY_ALPHABETIC_FS_ALPHABETIC_LETTERSYMBOLS     10004011 /* = ARABIC_FS_ALPHABETIC_LETTERSYMBOLS */

#define DECUMA_CATEGORY_ALPHABETIC_FS_GREEK_DIGITSYMBOLS           10004020 /* = ARABIC_FS_ALPHABETIC_DIGITSYMBOLS */
#define DECUMA_CATEGORY_ALPHABETIC_FS_GREEK_LETTERSYMBOLS          10004021 /* Symbols to be used with Greek letters. Comma type 4. Full Stop type 2. Quotation Mark type 3. Apostrophe type 2. Semicolon as Greek Question Mark.  */

#define DECUMA_CATEGORY_ALPHABETIC_FS_ALPHABETIC_NUM_MODE          10004050 /* = DIGIT + ALPHABETIC_FS_ALPHABETIC_DIGITSYMBOLS + BACKSPACE_STROKE + RETURN_STROKE + TAB_STROKE */
#define DECUMA_CATEGORY_ALPHABETIC_FS_ALPHABETIC_UC_MODE           10004052 /* = UC_ANSI + ALPHABETIC_FS_ALPHABETIC_LETTERSYMBOLS + ARABIC_FS_ALPHABETIC_GESTURES */
#define DECUMA_CATEGORY_ALPHABETIC_FS_ALPHABETIC_LC_MODE           10004053 /* = LC_ANSI + ALPHABETIC_FS_ALPHABETIC_LETTERSYMBOLS + ARABIC_FS_ALPHABETIC_GESTURES */

#define DECUMA_CATEGORY_ALPHABETIC_FS_CYRILLIC_UC_MODE             10004062 /* = UC_CYRILLIC + ALPHABETIC_FS_ALPHABETIC_LETTERSYMBOLS + ARABIC_FS_ALPHABETIC_GESTURES */
#define DECUMA_CATEGORY_ALPHABETIC_FS_CYRILLIC_LC_MODE             10004063 /* = LC_CYRILLIC + ALPHABETIC_FS_ALPHABETIC_LETTERSYMBOLS + ARABIC_FS_ALPHABETIC_GESTURES */

#define DECUMA_CATEGORY_ALPHABETIC_FS_GREEK_UC_MODE                10004072 /* = UC_GREEK + ALPHABETIC_FS_GREEK_LETTERSYMBOLS + ARABIC_FS_ALPHABETIC_GESTURES */
#define DECUMA_CATEGORY_ALPHABETIC_FS_GREEK_LC_MODE                10004073 /* = LC_GREEK + ALPHABETIC_FS_GREEK_LETTERSYMBOLS + ARABIC_FS_ALPHABETIC_GESTURES */


/*///////////////////////////////////////////////////////////////////// */
/*  Space reserved for Decuma internal categories                    // */
/*///////////////////////////////////////////////////////////////////// */

#define DECUMA_CATEGORY_RESERVED_START                           0xE0000000 /* (3758096384 in decimal) */
#define DECUMA_CATEGORY_RESERVED_END                             0xEFFFFFFF /* (4026531839 in decimal) */


/*///////////////////////////////////////////////////////////////////// */
/*  Space for user defined categories                                // */
/*///////////////////////////////////////////////////////////////////// */

#define DECUMA_CATEGORY_USER_DEFINED_START                       0xF0000000 /* (4026531840 in decimal) */
#define DECUMA_CATEGORY_USER_DEFINED_END                         0xFFFFFFFF /* (4294967295 in decimal) */

#endif /*DECUMA_CATEGORY_SYMBOL_CATEGORIES_H_1234234 */
