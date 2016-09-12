/******************************************************************************
;******************************************************************************
;**                                                                          **
;**                   COPYRIGHT 2010 NUANCE COMMUNICATIONS                   **
;**                                                                          **
;**               NUANCE COMMUNICATIONS PROPRIETARY INFORMATION              **
;**                                                                          **
;**    This software is supplied under the terms of a license agreement      **
;**    or non-disclosure agreement with Nuance Communications and may not    **
;**    be copied or disclosed except in accordance with the terms of that    **
;**    agreement.                                                            **
;**                                                                          **
;******************************************************************************
;*****************************************************************************/

/*****************************************************************************
 * Define T9Write Alpha Ver
 *
 * String format is defined as "T9WRITE ALPHABETIC VMM.mm"
 * Number format is defined as 0xMMmm. Number is in hexadecimal where
 *     MM = major version number
 *     mm = minor version number
 *****************************************************************************/

#define T9WRITEALPHAMAJORVER "05"
#define T9WRITEALPHAMINORVER "01"
#define T9WRITEALPHAPATCHVER "02"
#define T9WRITEALPHARCVER    "02"

#define T9WRITEALPHACOREVER "T9WRITE ALPHABETIC V" T9WRITEALPHAMAJORVER "." T9WRITEALPHAMINORVER "." T9WRITEALPHAPATCHVER "." T9WRITEALPHARCVER

#define T9WRITEALPHACOREVERSIONNUM 0x0501
