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


#ifndef DECUMA_UNICODE_H
#define DECUMA_UNICODE_H

#ifdef __cplusplus
extern "C"{
#endif

#include "decumaBasicTypes.h"
#include "decumaConfig.h"


DECUMA_HWR_PRIVATE int decumaIsFullwidth(DECUMA_UINT16 unicode);
DECUMA_HWR_PRIVATE int decumaIsBopomofo(DECUMA_UINT16 unicode);
DECUMA_HWR_PRIVATE int decumaIsDigit(DECUMA_UINT16 unicode);
DECUMA_HWR_PRIVATE int decumaIsGesture(DECUMA_UINT16 unicode);
DECUMA_HWR_PRIVATE int decumaIsLatinLower(DECUMA_UINT16 unicode);
DECUMA_HWR_PRIVATE int decumaIsLatinUpper(DECUMA_UINT16 unicode);
DECUMA_HWR_PRIVATE int decumaIsLatin(DECUMA_UINT16 unicode);
DECUMA_HWR_PRIVATE int decumaIsRadical(DECUMA_UINT16 u);
DECUMA_HWR_PRIVATE int decumaIsHiragana(DECUMA_UINT16 unicode);
DECUMA_HWR_PRIVATE int decumaIsHiraganaAnySize(DECUMA_UINT16 unicode);
DECUMA_HWR_PRIVATE int decumaIsHiraganaSmall(DECUMA_UINT16 u);
DECUMA_HWR_PRIVATE int decumaIsKatakana(DECUMA_UINT16 u);
DECUMA_HWR_PRIVATE int decumaIsKatakanaAnySize(DECUMA_UINT16 u);
DECUMA_HWR_PRIVATE int decumaIsKatakanaSmall(DECUMA_UINT16 u);
DECUMA_HWR_PRIVATE int decumaIsHan(DECUMA_UINT16 u);
DECUMA_HWR_PRIVATE int decumaIsHangul(DECUMA_UINT16 u);
DECUMA_HWR_PRIVATE int decumaIsHKSCS(DECUMA_UINT16 u);
DECUMA_HWR_PRIVATE int decumaIsPunctuation(DECUMA_UINT16 u);
DECUMA_HWR_PRIVATE int decumaIsSymbol(DECUMA_UINT16 u);
DECUMA_HWR_PRIVATE int decumaIsFullWidthPunctuation(DECUMA_UINT16 u);
DECUMA_HWR_PRIVATE int decumaIsFullWidthSymbol(DECUMA_UINT16 u);
DECUMA_HWR_PRIVATE DECUMA_UINT16 decumaGetAltUnicode(DECUMA_UINT16 u);
DECUMA_HWR_PRIVATE DECUMA_UINT16 decumaGetStdUnicode(DECUMA_UINT16 u);

#ifdef __cplusplus
} /*extern "C" */
#endif

#endif
