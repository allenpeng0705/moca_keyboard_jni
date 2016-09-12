/*****************************************************************************
* COPYRIGHT ZI AND SUBJECT TO CONFIDENTIALITY RESTRICTIONS                   *
*                                                                            *
* This file is the confidential and proprietary property of Zi Corporation   *
* of Canada, Inc. ("Zi") or one of its affiliates.                           *
******************************************************************************/

#pragma once

#ifndef CJK_STROKE_MACROS_H_
#define CJK_STROKE_MACROS_H_

/** @hideinitializer @returns the number of points in the stroke */
#define CJK_STROKE_NPOINTS(m_pStroke) (*((m_pStroke)->stdata))

/** @hideinitializer @returns the first gridpoint in the stroke */
#define CJK_STROKE_FIRST_POINT(m_pStroke) ((m_pStroke)->stdata+1)

/** @hideinitializer @returns the last gridpoint in the stroke */
#define CJK_STROKE_LAST_POINT(m_pStroke)  (*((m_pStroke)->stdata + CJK_STROKE_NPOINTS(m_pStroke)))

/** @hideinitializer @returns if the stroke exists */
#define CJK_STROKE_EXISTS(m_pStroke)  ((m_pStroke)->stdata != NULL)

/** @hideinitializer @returns the x-component of the gridpoint at m_nPos*/
#define CJK_STROKE_GET_X(m_pStroke, m_nPos) CJK_GP_GET_X(*cjkStrokeGetGridpoint(m_pStroke, m_nPos))

/** @hideinitializer @returns the first gridpoint in the stroke */
#define CJK_STROKE_GET_Y(m_pStroke, m_nPos) CJK_GP_GET_Y(*cjkStrokeGetGridpoint(m_pStroke, m_nPos))

#endif /* CJK_STROKE_MACROS_H_ */
