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


/*****************************************************************************
* COPYRIGHT ZI AND SUBJECT TO CONFIDENTIALITY RESTRICTIONS                   *
*                                                                            *
* This file is the confidential and proprietary property of Zi Corporation   *
* of Canada, Inc. ("Zi") or one of its affiliates.                           *
******************************************************************************/

#ifndef INCLUDED_DECUMAUNICODEMACROS_H
#define INCLUDED_DECUMAUNICODEMACROS_H

/* C0 controls */

#define UC_BACKSPACE					  0x0008
#define UC_TABULATION					  0x0009
#define UC_RETURN						  	0x000D
#define UC_SPACE						  	0x0020

/* ASCII punctuation and symbols */

#define UC_EXCLAMATION_MARK				  			0x0021
#define UC_QUOTATION_MARK                 0x0022
#define UC_NUMBER_SIGN					  				0x0023
#define UC_DOLLAR_SIGN					  				0x0024
#define UC_PERCENT_SIGN					  				0x0025
#define UC_AMPERSAND					  					0x0026
#define UC_APOSTROPHE					  					0x0027
#define UC_LEFT_PARENTHESIS				  			0x0028
#define UC_RIGHT_PARENTHESIS			  			0x0029
#define UC_ASTERISK						  					0x002A
#define UC_PLUS_SIGN					  					0x002B
#define UC_COMMA						  						0x002C
#define UC_HYPHEN_MINUS					  				0x002D
#define UC_FULL_STOP					  					0x002E
#define UC_SOLIDUS						  					0x002F

#define UC_COLON						  						0x003A
#define UC_SEMICOLON					  					0x003B
#define UC_LESS_THAN_SIGN				  				0x003C
#define UC_EQUALS_SIGN					  				0x003D
#define UC_GREATER_THAN_SIGN			  			0x003E
#define UC_QUESTION_MARK				  				0x003F
#define UC_COMMERCIAL_AT                  0x0040

#define UC_LEFT_SQUARE_BRACKET			  		0x005B
#define UC_REVERSE_SOLIDUS				  			0x005C
#define UC_RIGHT_SQUARE_BRACKET			  		0x005D
#define UC_CIRCUMFLEX_ACCENT              0x005E
#define UC_LOW_LINE						  					0x005F
#define UC_GRAVE_ACCENT                   0x0060

/* Lower case Latin alphabet */

#define UC_LATIN_SMALL_LETTER_O           0x006F

/* ASCII punctuation and symbols */

#define UC_LEFT_CURLY_BRACKET   					0x007B
#define UC_VERTICAL_LINE									0x007C
#define UC_RIGHT_CURLY_BRACKET  					0x007D
#define UC_TILDE                					0x007E

#define UC_POUND_SIGN											0x00A3
#define UC_YEN_SIGN												0x00A5

#define UC_EM_DASH                        0x2014
#define UC_HORIZONTAL_BAR                 0x2015
#define UC_LEFT_SINGLE_QUOTATION_MARK     0x2018
#define UC_RIGHT_SINGLE_QUOTATION_MARK    0x2019
#define UC_LEFT_DOUBLE_QUOTATION_MARK     0x201C
#define UC_RIGHT_DOUBLE_QUOTATION_MARK    0x201D
#define UC_BULLET                         0x2022
#define UC_HORIZONTAL_ELLIPSIS            0x2026

#define UC_FULLWIDTH_WHITE_CIRCLE         0x25CB

#define UC_IDEOGRAPHIC_COMMA              0x3001
#define UC_IDEOGRAPHIC_FULL_STOP          0x3002
#define UC_IDEOGRAPHIC_ITERATION_MARK     0x3005
#define UC_IDEOGRAPHIC_CLOSING_MARK       0x3006
#define UC_IDEOGRAPHIC_NUMBER_ZERO        0x3007
#define UC_LEFT_ANGLE_BRACKET             0x3008
#define UC_RIGHT_ANGLE_BRACKET            0x3009
#define UC_LEFT_DOUBLE_ANGLE_BRACKET      0x300A
#define UC_RIGHT_DOUBLE_ANGLE_BRACKET     0x300B
#define UC_LEFT_CORNER_BRACKET            0x300C
#define UC_RIGHT_CORNER_BRACKET           0x300D
#define UC_LEFT_WHITE_CORNER_BRACKET      0x300E
#define UC_RIGHT_WHITE_CORNER_BRACKET     0x300F
#define UC_LEFT_BLACK_LENTICULAR_BRACKET  0x3010
#define UC_RIGHT_BLACK_LENTICULAR_BRACKET 0x3011
#define UC_POSTAL_MARK                    0x3012
#define UC_HIRAGANA_VOICED_ITERATION_MARK 0x309E
#define UC_MIDDLE_DOT                     0x30FB

#define UC_FULLWIDTH_EXCLAMATION_MARK     0xFF01
#define UC_FULLWIDTH_QUOTATION_MARK       0xFF02
#define UC_FULLWIDTH_NUMBER_SIGN          0xFF03
#define UC_FULLWIDTH_DOLLAR_SIGN          0xFF04
#define UC_FULLWIDTH_PERCENT_SIGN         0xFF05
#define UC_FULLWIDTH_AMPERSAND            0xFF06
#define UC_FULLWIDTH_APOSTROPHE           0xFF07
#define UC_FULLWIDTH_LEFT_PARENTHESIS     0xFF08
#define UC_FULLWIDTH_RIGHT_PARENTHESIS    0xFF09
#define UC_FULLWIDTH_ASTERISK             0xFF0A
#define UC_FULLWIDTH_PLUS_SIGN            0xFF0B
#define UC_FULLWIDTH_COMMA                0xFF0C
#define UC_FULLWIDTH_HYPHEN_MINUS         0xFF0D
#define UC_FULLWIDTH_FULL_STOP            0xFF0E
#define UC_FULLWIDTH_SOLIDUS              0xFF0F
#define UC_FULLWIDTH_DIGIT_ZERO           0xFF10
#define UC_FULLWIDTH_DIGIT_ONE            0xFF11
#define UC_FULLWIDTH_DIGIT_TWO            0xFF12
#define UC_FULLWIDTH_DIGIT_THREE          0xFF13
#define UC_FULLWIDTH_DIGIT_FOUR           0xFF14
#define UC_FULLWIDTH_DIGIT_FIVE           0xFF15
#define UC_FULLWIDTH_DIGIT_SIX            0xFF16
#define UC_FULLWIDTH_DIGIT_SEVEN          0xFF17
#define UC_FULLWIDTH_DIGIT_EIGHT          0xFF18
#define UC_FULLWIDTH_DIGIT_NINE           0xFF19
#define UC_FULLWIDTH_COLON                0xFF1A
#define UC_FULLWIDTH_SEMICOLON            0xFF1B
#define UC_FULLWIDTH_LESS_THAN_SIGN       0xFF1C
#define UC_FULLWIDTH_EQUALS_SIGN          0xFF1D
#define UC_FULLWIDTH_GREATER_THAN_SIGN    0xFF1E
#define UC_FULLWIDTH_QUESTION_MARK        0xFF1F
#define UC_FULLWIDTH_COMMERCIAL_AT        0xFF20

#define UC_FULLWIDTH_LATIN_CAPITAL_LETTER_A 0xFF21
#define UC_FULLWIDTH_LATIN_CAPITAL_LETTER_B 0xFF22
#define UC_FULLWIDTH_LATIN_CAPITAL_LETTER_C 0xFF23
#define UC_FULLWIDTH_LATIN_CAPITAL_LETTER_D 0xFF24
#define UC_FULLWIDTH_LATIN_CAPITAL_LETTER_E 0xFF25
#define UC_FULLWIDTH_LATIN_CAPITAL_LETTER_F 0xFF26
#define UC_FULLWIDTH_LATIN_CAPITAL_LETTER_G 0xFF27
#define UC_FULLWIDTH_LATIN_CAPITAL_LETTER_H 0xFF28
#define UC_FULLWIDTH_LATIN_CAPITAL_LETTER_I 0xFF29
#define UC_FULLWIDTH_LATIN_CAPITAL_LETTER_J 0xFF2A
#define UC_FULLWIDTH_LATIN_CAPITAL_LETTER_K 0xFF2B
#define UC_FULLWIDTH_LATIN_CAPITAL_LETTER_L 0xFF2C
#define UC_FULLWIDTH_LATIN_CAPITAL_LETTER_M 0xFF2D
#define UC_FULLWIDTH_LATIN_CAPITAL_LETTER_N 0xFF2E
#define UC_FULLWIDTH_LATIN_CAPITAL_LETTER_O 0xFF2F
#define UC_FULLWIDTH_LATIN_CAPITAL_LETTER_P 0xFF30
#define UC_FULLWIDTH_LATIN_CAPITAL_LETTER_Q 0xFF31
#define UC_FULLWIDTH_LATIN_CAPITAL_LETTER_R 0xFF32
#define UC_FULLWIDTH_LATIN_CAPITAL_LETTER_S 0xFF33
#define UC_FULLWIDTH_LATIN_CAPITAL_LETTER_T 0xFF34
#define UC_FULLWIDTH_LATIN_CAPITAL_LETTER_U 0xFF35
#define UC_FULLWIDTH_LATIN_CAPITAL_LETTER_V 0xFF36
#define UC_FULLWIDTH_LATIN_CAPITAL_LETTER_W 0xFF37
#define UC_FULLWIDTH_LATIN_CAPITAL_LETTER_X 0xFF38
#define UC_FULLWIDTH_LATIN_CAPITAL_LETTER_Y 0xFF39
#define UC_FULLWIDTH_LATIN_CAPITAL_LETTER_Z 0xFF3A

#define UC_FULLWIDTH_LEFT_SQUARE_BRACKET  0xFF3B
#define UC_FULLWIDTH_REVERSE_SOLIDUS      0xFF3C
#define UC_FULLWIDTH_RIGHT_SQUARE_BRACKET 0xFF3D
#define UC_FULLWIDTH_SPACING_UNDERSCORE   0xFF3F
#define UC_FULLWIDTH_GRAVE_ACCENT         0xFF40

#define UC_FULLWIDTH_LATIN_SMALL_LETTER_A 0xFF41
#define UC_FULLWIDTH_LATIN_SMALL_LETTER_B 0xFF42
#define UC_FULLWIDTH_LATIN_SMALL_LETTER_C 0xFF43
#define UC_FULLWIDTH_LATIN_SMALL_LETTER_D 0xFF44
#define UC_FULLWIDTH_LATIN_SMALL_LETTER_E 0xFF45
#define UC_FULLWIDTH_LATIN_SMALL_LETTER_F 0xFF46
#define UC_FULLWIDTH_LATIN_SMALL_LETTER_G 0xFF47
#define UC_FULLWIDTH_LATIN_SMALL_LETTER_H 0xFF48
#define UC_FULLWIDTH_LATIN_SMALL_LETTER_I 0xFF49
#define UC_FULLWIDTH_LATIN_SMALL_LETTER_J 0xFF4A
#define UC_FULLWIDTH_LATIN_SMALL_LETTER_K 0xFF4B
#define UC_FULLWIDTH_LATIN_SMALL_LETTER_L 0xFF4C
#define UC_FULLWIDTH_LATIN_SMALL_LETTER_M 0xFF4D
#define UC_FULLWIDTH_LATIN_SMALL_LETTER_N 0xFF4E
#define UC_FULLWIDTH_LATIN_SMALL_LETTER_O 0xFF4F
#define UC_FULLWIDTH_LATIN_SMALL_LETTER_P 0xFF50
#define UC_FULLWIDTH_LATIN_SMALL_LETTER_Q 0xFF51
#define UC_FULLWIDTH_LATIN_SMALL_LETTER_R 0xFF52
#define UC_FULLWIDTH_LATIN_SMALL_LETTER_S 0xFF53
#define UC_FULLWIDTH_LATIN_SMALL_LETTER_T 0xFF54
#define UC_FULLWIDTH_LATIN_SMALL_LETTER_U 0xFF55
#define UC_FULLWIDTH_LATIN_SMALL_LETTER_V 0xFF56
#define UC_FULLWIDTH_LATIN_SMALL_LETTER_W 0xFF57
#define UC_FULLWIDTH_LATIN_SMALL_LETTER_X 0xFF58
#define UC_FULLWIDTH_LATIN_SMALL_LETTER_Y 0xFF59
#define UC_FULLWIDTH_LATIN_SMALL_LETTER_Z 0xFF5A


#define UC_FULLWIDTH_LEFT_CURLY_BRACKET   0xFF5B
#define UC_FULLWIDTH_VERTICAL_LINE        0xFF5C
#define UC_FULLWIDTH_RIGHT_CURLY_BRACKET  0xFF5D
#define UC_FULLWIDTH_TILDE                0xFF5E
#define UC_DIACRITIC_DAKUTEN              0xFF9E
#define UC_DIACRITIC_HANDAKUTEN           0xFF9F
#define UC_FULLWIDTH_POUND_SIGN           0xFFE1
#define UC_FULLWIDTH_YEN_SIGN             0xFFE5

#define UC_CJK_UNIFIED_FIRST 0x4E00
#define UC_CJK_UNIFIED_LAST  0x9FA5
#define UC_CJK_UNIFIED_N     (CJK_UNIFIED_LAST-CJK_UNIFIED_FIRST+1)

#define UC_HANGUL_FIRST 0xAC00
#define UC_HANGUL_LAST 0xD7A3

#endif
