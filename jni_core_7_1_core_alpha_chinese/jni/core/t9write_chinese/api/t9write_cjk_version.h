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
 * Define T9Write CJK Ver
 *
 * String format is defined as "T9WRITE CJK VMM.mm"
 * Number format is defined as 0xMMmm. Number is in hexadecimal where
 *     MM = major version number
 *     mm = minor version number
 *****************************************************************************/

#define T9WRITECJKMAJORVER "04"
#define T9WRITECJKMINORVER "03"
#define T9WRITECJKPATCHVER "01"
#define T9WRITECJKRCVER    "02"

#define T9WRITECJKCOREVER "T9WRITE CJK V" T9WRITECJKMAJORVER "." T9WRITECJKMINORVER "." T9WRITECJKPATCHVER "." T9WRITECJKRCVER

#define T9WRITECJKCOREVERSIONNUM 0x0403
