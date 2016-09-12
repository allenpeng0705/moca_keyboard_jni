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


#ifndef scrMEASURE_ID_sadf9878997878798778665r
#define scrMEASURE_ID_sadf9878997878798778665r

#include "decumaConfig.h"
#include "decumaDataTypes.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef DECUMA_MANGLE
#include "mangle.h"
#endif

/*
// This is needed for some of the macros used..
typedef struct _tagMEASURE_ID_DESCRIPTOR
{
	const char nSections;
	const char * pIncExcl; // boolean.
} MEASURE_ID_DESCRIPTOR;
*/



	/* returns 1 if element number nCheckMe is important for measure_id when the curve is nTotalLength long. */
/*int GetMeasureIncExcl(int measure_id, int nTotalLength, int nCheckMe); */
/*extern const MEASURE_ID_DESCRIPTOR measureid_midd[]; */

DECUMA_HWR_PRIVATE int GetMeasureIncExcl(int mid, int totlen, int checkme);

/*#define GetMeasureIncExcl( mid, totlen, checkme) (mid == 0 ? 1 : measureid_midd[mid].pIncExcl[  (checkme * measureid_midd[mid].nSections ) / totlen ]) */


DECUMA_HWR_PRIVATE int GetMeasureSection(int measure_id, DECUMA_INT16 * pTarget, int nTotalLength, int nStartAt, int nExtractLength);

/* The old version of "GetMeasure" */
/*int GetMeasure(int measure_id, NUMBERLIST* p); */


/* */
/* The measure id macros are using the Bresenhams line drawing algoritmh to keep */
/* track of what section in the descriptor that is active. */
/* Consider the point number as X and section as Y for the following BASIC code */
/* taken from http://www.cs.ualberta.ca/~juancho/courses/pub/311/bres.html: */
/* */
/* x1 = first point (0) */
/* x2 = last point (number of points -1) */
/* y1 = first section (0) */
/* y2 = last section (given by the descriptor -1) */
/* */
/*Draw_line(x1,y1,x2,y2) */
/*integer dx = x2-x1 */
/*integer dy = y2-y1 */
/*d = 2dy-dx */
/*dincr1 = 2dy */
/*dincr2 = 2(dy-dx) */
/*y = y1 */
/*for x from x1 to x2 */
/*	set_pixel(x,y) */
/*	if d< 0 then */
/*		d = d + dincr1 */
/*	else */
/*		d = d + dincr2 */
/*		y = y + 1 */
/*	end if */
/*end for */
/*end draw_line */
/* */
/* */

/* gets Y1 and Y2 */

/*firstsection*/
/*measureidGetStartSection
 *[in] measure_id, 
 *[in] nTotalLength 
 *[in] nFirstPoint) */
/*measureidGetStartSection(measure_id, nTotalLength, nFirstPoint); */

/*lastsection*/
/*measureidGetEndSection
 *[in] measure_id, 
 *[in] nTotalLength 
 *[in] nLastPoint) */
/*measureidGetEndSection(measure_id, nTotalLength, nLastPoint); */

/*pidmask*/
/*measureidGetIDMask(measure_id, firstsection) */

/* gets d, dincr1 and dincr2 */
/* measureidGetIterators( 
 * d [out], dincr1 [out], dincr2 [out], firstpoint [in], firstsection [in] , lastpoint [in], lastsection [in]) 
*/

/* (bool) include or not */
/*measureidNextPoint(pidmask [in], d [in|out] , dincr1 [in], dincr2 [in]) */


#ifdef __cplusplus
} /*extern "C" { */
#endif

#endif /*scrMEASURE_ID_sadf9878997878798778665r */
