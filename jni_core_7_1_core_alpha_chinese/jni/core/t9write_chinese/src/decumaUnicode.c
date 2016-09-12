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


#include "decumaUnicode.h"
#include "decumaUnicodeMacros.h"
#include "decumaBasicTypes.h"



typedef struct _tagDECUMA_UC_INFO {
   DECUMA_UINT16 uc;
   DECUMA_UINT16 isFullwidth;
   DECUMA_UINT16 altuc;
} DECUMA_UC_INFO;


typedef struct _tagDECUMA_UC_MAP {
	DECUMA_UINT16 private_code;
	DECUMA_UINT16 code;
} DECUMA_UC_MAP;


static const DECUMA_UC_INFO s_punctuationTable[] = {
	{UC_FULLWIDTH_COMMA,                1, 0x002C},
	{UC_FULLWIDTH_FULL_STOP,            1, 0x002E},
	{UC_FULLWIDTH_SPACING_UNDERSCORE,   1, 0x005F},
	{UC_LEFT_SINGLE_QUOTATION_MARK,     1, 0},
	{UC_RIGHT_SINGLE_QUOTATION_MARK,    1, 0},
	{UC_LEFT_DOUBLE_QUOTATION_MARK,     1, 0},
	{UC_RIGHT_DOUBLE_QUOTATION_MARK,    1, 0},
	{UC_IDEOGRAPHIC_COMMA,              1, 0},
	{UC_IDEOGRAPHIC_FULL_STOP,          1, 0},
	{UC_LEFT_ANGLE_BRACKET,             1, 0},
	{UC_RIGHT_ANGLE_BRACKET,            1, 0},
	{UC_LEFT_DOUBLE_ANGLE_BRACKET,      1, 0},
	{UC_RIGHT_DOUBLE_ANGLE_BRACKET,     1, 0},
	{UC_LEFT_CORNER_BRACKET,            1, 0},
	{UC_RIGHT_CORNER_BRACKET,           1, 0},
	{UC_LEFT_WHITE_CORNER_BRACKET,      1, 0},
	{UC_RIGHT_WHITE_CORNER_BRACKET,     1, 0},
	{UC_LEFT_BLACK_LENTICULAR_BRACKET,  1, 0},
	{UC_RIGHT_BLACK_LENTICULAR_BRACKET, 1, 0},
	{UC_POSTAL_MARK,                    1, 0},
	{UC_FULLWIDTH_EXCLAMATION_MARK,     1, 0x0021},
	{UC_FULLWIDTH_LEFT_PARENTHESIS,     1, 0x0028},
	{UC_FULLWIDTH_RIGHT_PARENTHESIS,    1, 0x0029},
	{UC_FULLWIDTH_SOLIDUS,              1, 0x002F},
	{UC_FULLWIDTH_COLON,                1, 0x003A},
	{UC_FULLWIDTH_SEMICOLON,            1, 0x003B},
	{UC_FULLWIDTH_QUESTION_MARK,        1, 0x003F},
	{UC_FULLWIDTH_LEFT_SQUARE_BRACKET,  1, 0x005B},
	{UC_FULLWIDTH_REVERSE_SOLIDUS,      1, 0x005C},
	{UC_FULLWIDTH_RIGHT_SQUARE_BRACKET, 1, 0x005D},
	{UC_FULLWIDTH_LEFT_CURLY_BRACKET,   1, 0x007B},
	{UC_FULLWIDTH_VERTICAL_LINE,        1, 0x007C},
	{UC_FULLWIDTH_RIGHT_CURLY_BRACKET,  1, 0x007D},
	{UC_FULLWIDTH_TILDE,                1, 0x007E},
	{UC_HORIZONTAL_ELLIPSIS,            1, 0},
	{UC_BULLET,                         1, 0},
	{UC_FULLWIDTH_QUOTATION_MARK,       1, 0x0022},
	{UC_QUOTATION_MARK,                 0, 0},
	{UC_FULLWIDTH_APOSTROPHE,           1, 0x0027},
	{UC_FULLWIDTH_GRAVE_ACCENT,         1, 0x0060},
#ifdef CERTIFICATION
	{0x0020,1,0},
	{0x000D,1,0},
	{0x0008,1,0},
	{0x001E,1,0},
#endif
	{0, 0, 0}
};


static const DECUMA_UC_INFO s_symbolTable[] = {
	{UC_FULLWIDTH_NUMBER_SIGN,          1, 0x0023},
	{UC_FULLWIDTH_DOLLAR_SIGN,          1, 0x0024},
	{UC_FULLWIDTH_PERCENT_SIGN,         1, 0x0025},
	{UC_FULLWIDTH_AMPERSAND,            1, 0x0026},
	{UC_FULLWIDTH_ASTERISK,             1, 0x002A},
	{UC_FULLWIDTH_PLUS_SIGN,            1, 0x002B},
	{UC_FULLWIDTH_HYPHEN_MINUS,         1, 0x002D},
	{UC_FULLWIDTH_LESS_THAN_SIGN,       1, 0x003C},
	{UC_FULLWIDTH_EQUALS_SIGN,          1, 0x003D},
	{UC_FULLWIDTH_GREATER_THAN_SIGN,    1, 0x003E},
	{UC_COMMERCIAL_AT,                  0, 0},
	{UC_FULLWIDTH_YEN_SIGN,             1, 0x00A5},
	{UC_EM_DASH,                        1, 0},
	{UC_CIRCUMFLEX_ACCENT,              0, 0},
	{UC_FULLWIDTH_POUND_SIGN,           1, 0x00A3},
	{UC_IDEOGRAPHIC_CLOSING_MARK,       1, 0},
	{0, 0, 0}
};


int decumaIsFullwidth(DECUMA_UINT16 unicode)
{
	/*
		The following rule has been implemented based on a
		fullwidth table generated on Symbian 7.0 with this
		piece of code

			_LIT(KFileName,"C:\\table.txt");
			RFs Fs;
			RFile file;
			Fs.Connect();
			file.Replace(Fs,KFileName,EFileWrite);
			unsigned char *pTable = (unsigned char *)User::AllocL(256 * 256 / 8);
			Mem::FillZ(pTable, 256 * 256 / 8);
			for( TUint i = 0; i < 256 * 256; i++ )
			{
				TChar c(i);

				TChar::TCjkWidth width = c.GetCjkWidth();

				if( width == TChar::EFullWidth ||
					width ==  TChar::EWide )
				{
					pTable[i/8] |= 1 << (i%8);
				}
			}
			TPtr8 data(pTable, 256 * 256 / 8, 256 * 256 / 8);
			file.Write(data);
			User::Free(pTable);
			file.Close();
			Fs.Close();

		The generated 8 kB table was very sparse and was therefore
		converted to the conditional rule below to save memory.

		/JA
	*/

	if( (unicode >= 0x1100 && unicode <= 0x115F) ||
		(unicode >= 0x2E80 && unicode <= 0xD7A3) ||
		(unicode >= 0xF900 && unicode <= 0xFA2D) ||
		(unicode >= 0xFE30 && unicode <= 0xFE6B) ||
		(unicode >= 0xFF01 && unicode <= 0xFF5E) ||
		(unicode >= 0xFFE0 && unicode <= 0xFFE6) )
	{
		return 1; /* Fullwidth */
	}
	else
	{
		return 0; /* Not fullwidth */
	}
}

int decumaIsBopomofo(DECUMA_UINT16 unicode) {
   if ((unicode >= 0x3105) && (unicode <= 0x312C)) {
		return 1;
	}
	else {
		return 0;
	}

}

int decumaIsDigit(DECUMA_UINT16 unicode) {
   if (('0' <= unicode) && (unicode <= '9')) {
		return 1;
	}
	else {
		return 0;
	}
}


int decumaIsLatinLower(DECUMA_UINT16 unicode) {
   if (('a' <= unicode) && (unicode <= 'z')) {
		return 1;
	}
	else {
		return 0;
	}
}

int decumaIsLatinUpper(DECUMA_UINT16 unicode) {
   if (('A' <= unicode) && (unicode <= 'Z')) {
		return 1;
	}
	else {
		return 0;
	}
}

int decumaIsGesture(DECUMA_UINT16 unicode) {
   if (unicode == UC_BACKSPACE || unicode == UC_TABULATION	|| 
   	 unicode == UC_RETURN || unicode == UC_SPACE) {
		return 1;
	}
	else {
		return 0;
	}

}


int decumaIsLatin(DECUMA_UINT16 unicode) {
   if (decumaIsLatinLower(unicode) || decumaIsLatinUpper(unicode)) {
		return 1;
	}
	else {
		return 0;
	}
}


int decumaIsRadical(DECUMA_UINT16 u) {
   if (u == 0x4E0C || u == 0x4E28 || u == 0x4E2C || u == 0x4E36
        || u == 0x4E3F || u == 0x4EA0 || u == 0x4EBB || u == 0x5182
        || u == 0x5196 || u == 0x51AB || u == 0x51F5 || u == 0x5202
        || u == 0x52F9 || u == 0x531A || u == 0x5369 || u == 0x53B6
        || u == 0x56D7 || u == 0x5902 || u == 0x5B80 || u == 0x5C6E
        || u == 0x5EF4 || u == 0x5F50 || u == 0x5F61 || u == 0x5F73 || u == 0x5FC4
        || u == 0x624C || u == 0x6535 || u == 0x6C35 || u == 0x706C
        || u == 0x72AD || u == 0x7592 || u == 0x793B || u == 0x7E9F
        || u == 0x8080 || u == 0x8279 || u == 0x864D || u == 0x8864
        || u == 0x8BA0 || u == 0x8FB6 || u == 0x9485 || u == 0x961D
        || u == 0x9963) {
		return 1;
	}
	else {
		return 0;
	}
}

int decumaIsHiragana(DECUMA_UINT16 unicode) {
   if ((((unicode >= 0x3041) && (unicode <= 0x3094))
           || unicode == 0x309E) && !decumaIsHiraganaSmall(unicode)) {
		return 1;
	}
	else {
		return 0;
	}
}

int decumaIsHiraganaAnySize(DECUMA_UINT16 unicode) {
   if (((unicode >= 0x3041) && (unicode <= 0x3094))
           || unicode == 0x309E) {
		return 1;
	}
	else {
		return 0;
	}
}

int decumaIsHiraganaSmall(DECUMA_UINT16 u) {
   if (u == 0x3041 || u == 0x3043 || u == 0x3045 || u == 0x3047 ||
           u == 0x3049 || u == 0x3063 || u == 0x3083 || u == 0x3085 ||
           u == 0x3087 || u == 0x308E) {
		return 1;
	}
	else {
		return 0;
	}
}

int decumaIsKatakana(DECUMA_UINT16 u) {
   if ((((u >= 0x30A1) && (u <= 0x30FA)) ||
             u == UC_MIDDLE_DOT || u == 0x30FC ||
             u == UC_DIACRITIC_HANDAKUTEN || u == UC_DIACRITIC_DAKUTEN) &&
             !decumaIsKatakanaSmall(u)) {
		return 1;
	}
	else {
		return 0;
	}
}

int decumaIsKatakanaAnySize(DECUMA_UINT16 u) {
   if (((u >= 0x30A1) && (u <= 0x30FA)) ||
             u == UC_MIDDLE_DOT || u == 0x30FC) {
		return 1;
	}
	else {
		return 0;
	}
}

int decumaIsKatakanaSmall(DECUMA_UINT16 u) {
   if (u == 0x30A1 || u == 0x30A3 || u == 0x30A5 || u == 0x30A7 ||
           u == 0x30A9 || u == 0x30C3 || u == 0x30E3 || u == 0x30E5 ||
           u == 0x30E7 || u == 0x30EE || u == 0x30F5 || u == 0x30F6) {
		return 1;
	}
	else {
		return 0;
	}
}

int decumaIsHan(DECUMA_UINT16 u)
{
	return (u >= UC_CJK_UNIFIED_FIRST && u <= UC_CJK_UNIFIED_LAST) ||
		u == UC_IDEOGRAPHIC_NUMBER_ZERO || u == UC_IDEOGRAPHIC_ITERATION_MARK ||
		u == UC_FULLWIDTH_WHITE_CIRCLE;
}



int decumaIsHangul(DECUMA_UINT16 u) 
{
	return (u >= UC_HANGUL_FIRST && u <= UC_HANGUL_LAST);
}


int decumaIsHKSCS(DECUMA_UINT16 u) {
	return ((u>= 0x00A8 && u<=0x00FC)
		|| (u>= 0x0100 && u<=0x01DC)
		|| (u>= 0x0250 && u<=0x02C6) 
		|| (u>= 0x0401 && u<=0x0451)
		|| (u>= 0x1EBE && u<=0x1EC1)
		|| (u>= 0x2116 && u<=0x21E7)
		|| (u>= 0x2460 && u<=0x247D)
		|| (u>= 0x2550 && u<=0x2570)
		|| (u>= 0x273D && u<=0x273D)
		|| (u>= 0x2E80 && u<=0x2F33)
		|| (u>= 0x3005 && u<=0x3007)
		|| (u>= 0x3041 && u<=0x30FE)
		|| (u>= 0x3231 && u<=0x3231)
		|| (u>= 0x3435 && u<=0x35FE)
		|| (u>= 0x3609 && u<=0x4D9C)
		|| (u>= 0xE000 && u<=0xEEB7)
		|| (u>= 0xF304 && u<=0xF7EE)
		|| (u>= 0xF907 && u<=0xF907)
		|| (u>= 0xFF02 && u<=0xFFED));
}


int decumaIsPunctuation(DECUMA_UINT16 u)
{
	int i, n;

	if (u == 0) return 0;

	n = sizeof(s_punctuationTable) / sizeof(s_punctuationTable[0]);

	for (i = 0; i < n; i++) {
		if (u == s_punctuationTable[i].uc || u == s_punctuationTable[i].altuc) {
			return 1;
		}
	}
	return 0;
}



int decumaIsFullWidthPunctuation(DECUMA_UINT16 u)
{
	int i, n;

	n = sizeof(s_punctuationTable) / sizeof(s_punctuationTable[0]);

	
	for (i = 0; i < n; i++) {
		if (u == s_punctuationTable[i].uc) {
			return s_punctuationTable[i].isFullwidth;
		}
	}

	return 0;
}

int decumaIsSymbol(DECUMA_UINT16 u) 
{
	int i, n;

	if (u == 0) return 0;
	
	n = sizeof(s_symbolTable) / sizeof(s_symbolTable[0]);

	for (i = 0; i < n; i++) {
		if (u == s_symbolTable[i].uc || u == s_symbolTable[i].altuc) {
			return 1;
		}
	}
	return 0;
}


int decumaIsFullWidthSymbol(DECUMA_UINT16 u) 
{
	int i, n;

	n = sizeof(s_symbolTable) / sizeof(s_symbolTable[0]);

	
	for (i = 0; i < n; i++) {
		if (u == s_symbolTable[i].uc) {
			return s_symbolTable[i].isFullwidth;
		}
	}

	return 0;
}

DECUMA_UINT16 decumaGetAltUnicode(DECUMA_UINT16 u) {
	DECUMA_INT32 i;
	DECUMA_UINT16 ui;

	i = 0;
	ui = s_punctuationTable[i].uc;
	while (ui != 0) {
		if (u == ui) {
			return s_punctuationTable[i].altuc;
		}
		ui = s_punctuationTable[++i].uc;
	}
	i = 0;
	ui = s_symbolTable[i].uc;
	while (ui != 0) {
		if (u == ui) {
			return s_symbolTable[i].altuc;
		}
		ui = s_symbolTable[++i].uc;
	}
	return 0;
}



DECUMA_UINT16 decumaGetStdUnicode(DECUMA_UINT16 u) 
{
	DECUMA_INT32 i;
	DECUMA_UINT16 ui, altui;

	i = 0;
	ui = s_punctuationTable[i].uc;
	altui = s_punctuationTable[i].altuc;
	while (ui != 0) {
		if (u == altui) {
			return ui;
		}
		ui = s_punctuationTable[++i].uc;
		altui = s_punctuationTable[i].altuc;
	}
	i = 0;
	ui = s_symbolTable[i].uc;
	altui = s_symbolTable[i].altuc;
	while (ui != 0) {
		if (u == altui) {
			return ui;
		}
		ui = s_symbolTable[++i].uc;
		altui = s_symbolTable[i].altuc;
	}
	return 0;
}
