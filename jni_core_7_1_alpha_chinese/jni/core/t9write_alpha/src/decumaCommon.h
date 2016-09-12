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


#ifndef DECUMA_COMMON_H_
#define DECUMA_COMMON_H_


/** @addtogroup DOX_GRP_UTILS Utilities
 *  @{
 */

/** @hideinitializer Use to get rid of warnings for unused function paramters */
#define DECUMA_UNUSED_PARAM(x) ((void)x)


#ifndef DECUMA_NO_EXPLICIT_CAST
/**
 * Use this macro for explicit type conversions instead of the ordinary C cast operator
 * @see DECUMA_NO_EXPLICIT_CAST
 * @param type  target type
 * @param expr  expression to convert
 * @hideinitializer
 */
#define DECUMA_CAST(type, expr) ((type)(expr))
#else
#define DECUMA_CAST(type, expr) (expr)
#endif

/** @hideinitializer Return the smallest of a and b */
#define DECUMA_MIN(a, b) (((a) < (b)) ? (a) : (b))

/** @hideinitializer Return the largest of a and b */
#define DECUMA_MAX(a, b) (((a) > (b)) ? (a) : (b))

/** @}
 */

#endif /* DECUMA_COMMON_H_ */
