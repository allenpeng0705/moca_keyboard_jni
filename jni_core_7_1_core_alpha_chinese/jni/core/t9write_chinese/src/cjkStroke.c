/*****************************************************************************
* COPYRIGHT ZI AND SUBJECT TO CONFIDENTIALITY RESTRICTIONS                   *
*                                                                            *
* This file is the confidential and proprietary property of Zi Corporation   *
* of Canada, Inc. ("Zi") or one of its affiliates.                           *
******************************************************************************/

#include "cjkStroke.h"
#include "cjkStroke_Macros.h"
#include "cjkBestList.h" /* Definition of LARGEDIST */
#include "cjkSession_Types.h"
#include "cjkDatabaseFormat_Macros.h" /* DLTDB_GET_COMPONENT_DATA */

/* Used by 
 * @li @ref cjkStrokeGetMinXGridpoint
 * @li @ref cjkStrokeGetMinYGridpoint
 * @li @ref cjkStrokeGetMaxXGridpoint
 * @li @ref cjkStrokeGetMaxYGridpoint
 */
#define BIGNUMBER 1000
#define SMALLNUMBER -1000


/** The [[MAXNPTS]] macro is an upper limit to the number of points
* a stroke may have for it to be accepted. The reason we want a
* low limit is that the space and speed is quadratic in the number of
* points and furthermore if the number of points is larger the
* [[dltCCharGetDTWDistance]] algorithm takes care of comparison between
* characters anyway.
*/ 
#define MAXNPNTS 40


void cjkStrokeInit(CJK_STROKE * pStroke) 
{
	decumaAssert(pStroke);

	pStroke->stdata = NULL;
	pStroke->end = NULL;
	pStroke->stdata_backup = NULL;
	pStroke->end_backup = NULL;
}



DECUMA_INT32 cjkStrokeIsSmall(CJK_STROKE * pStroke) 
{
	DECUMA_INT32 minx, miny, maxx, maxy;
	const DECUMA_INT32 small_limit = 4;

	minx = CJK_GP_GET_X(cjkStrokeGetMinXGridpoint(pStroke));
	miny = CJK_GP_GET_Y(cjkStrokeGetMinYGridpoint(pStroke));
	maxx = CJK_GP_GET_X(cjkStrokeGetMaxXGridpoint(pStroke));
	maxy = CJK_GP_GET_Y(cjkStrokeGetMaxYGridpoint(pStroke));
	if (((maxy - miny) < small_limit) && ((maxx - minx) < small_limit)) {
		return 1;
	}
	else {
		return 0;
	}
}



DECUMA_INT32 cjkStrokeIsClose(CJK_STROKE * pStroke1, CJK_STROKE * pStroke2) 
{
	DECUMA_INT32 minx1, miny1, maxx1, maxy1;
	DECUMA_INT32 minx2, miny2, maxx2, maxy2;
	const DECUMA_INT32 close_limit = 4;

	minx1 = CJK_GP_GET_X(cjkStrokeGetMinXGridpoint(pStroke1));
	miny1 = CJK_GP_GET_Y(cjkStrokeGetMinYGridpoint(pStroke1));
	maxx1 = CJK_GP_GET_X(cjkStrokeGetMaxXGridpoint(pStroke1));
	maxy1 = CJK_GP_GET_Y(cjkStrokeGetMaxYGridpoint(pStroke1));
	minx2 = CJK_GP_GET_X(cjkStrokeGetMinXGridpoint(pStroke2));
	miny2 = CJK_GP_GET_Y(cjkStrokeGetMinYGridpoint(pStroke2));
	maxx2 = CJK_GP_GET_X(cjkStrokeGetMaxXGridpoint(pStroke2));
	maxy2 = CJK_GP_GET_Y(cjkStrokeGetMaxYGridpoint(pStroke2));
	if (((maxy1 - maxy2) * (maxy1 - maxy2) < close_limit * close_limit) ||
		((miny1 - miny2) * (miny1 - miny2) < close_limit * close_limit) ||
		((maxy1 - maxy2) * (maxx1 - maxx2) < close_limit * close_limit) ||
		((miny1 - miny2) * (minx1 - minx2) < close_limit * close_limit)) {
			return 1;
	}
	else {
		return 0;
	}
}




CJK_DISTANCE cjkStrokeGetDistanceDTW(CJK_STROKE * pStroke1, 
									 CJK_STROKE * pStroke2, 
									 DECUMA_INT32 delta) 
{
	CJK_DISTANCE totdist[MAXNPNTS];
	CJK_DISTANCE d;
	DECUMA_INT32 na = CJK_STROKE_NPOINTS(pStroke1);
	DECUMA_INT32 nb = CJK_STROKE_NPOINTS(pStroke2);
	const CJK_GRIDPOINT *pa = CJK_STROKE_FIRST_POINT(pStroke1);
	const CJK_GRIDPOINT *pb = CJK_STROKE_FIRST_POINT(pStroke2);
	DECUMA_INT32 i, j;


	/*----------------------------------------------------- */
	/* special for strokes with length two */
	/*  */
	/* For the very common case of two points in each stroke we know what the */
	/* result is without running the algorithm. */
	if (na == 2 && nb == 2) {
		d = CJK_GP_GET_SQ_DISTANCE(*pa++, *pb++ + delta);
		d += CJK_GP_GET_SQ_DISTANCE(*pa, *pb + delta);
		return d;
	}



	/*----------------------------------------------------- */
	/* check stroke lengths */
	/*  */
	/* There is pStroke1 limit in the locally allocated array, so we must check */
	/* for that. */
	if (na > MAXNPNTS || nb > MAXNPNTS) {
		return LARGEDIST;
	}



	/*----------------------------------------------------- */
	/* initialize first row */
	/*  */
	/* The first row is pStroke1 cumulative sum of distances. */
	d = 0;
	for (j = 0; j < nb; j++) {
		totdist[j] = d += CJK_GP_GET_SQ_DISTANCE(pa[0], pb[j] + delta);
	}



	/*----------------------------------------------------- */
	/* loop over points in pStroke1 and pStroke2 */
	/*  */
	/* This is the main loop. */
	for (i = 1; i < na; i++) {
		CJK_GRIDPOINT gpa = pa[i];
		CJK_DISTANCE totdiag = totdist[0];
		CJK_DISTANCE tothori = totdist[0] += 
			CJK_GP_GET_SQ_DISTANCE(gpa, pb[0] + delta);
		for (j = 1; j < nb; j++) {
			CJK_DISTANCE temp = totdist[j];
			if (totdiag < totdist[j]) totdist[j] = totdiag;
			if (tothori < totdist[j]) totdist[j] = tothori;
			totdiag = temp;
			totdist[j] += CJK_GP_GET_SQ_DISTANCE(gpa, pb[j] + delta);
			tothori = totdist[j];
		}
	}


	return totdist[nb - 1];
}


const CJK_GRIDPOINT * cjkStrokeGetGapGridpoint(CJK_STROKE  * pStroke, 
											   DECUMA_UINT8  njump, 
											   CJK_SESSION * pSession) 
{
	CJK_GRIDPOINT_ITERATOR gi;
	gi.stroke = *pStroke;
	gi.n = CJK_STROKE_NPOINTS(&gi.stroke);
	decumaAssert(njump < MAXPNTSPERSTR);
	if (gi.n <= njump + 1 ) {
		return CJK_STROKE_FIRST_POINT(&gi.stroke);
	}
	else {
		gi.p = CJK_STROKE_FIRST_POINT(&gi.stroke);
		return cjkStrokeGetGridpoint(pStroke, 
			dltGPIterGetClosestPos(&gi, gi.n, njump, pSession));
	}
}


const CJK_GRIDPOINT * cjkStrokeGetGridpoint(CJK_STROKE * st, DECUMA_INT32 pos)
{
	DECUMA_INT32 n = CJK_STROKE_NPOINTS(st);


	decumaAssert(pos != 0); /* TODO According to the comment... */

	if      (pos >  n) pos = n;
	else if (pos < -n) pos = 1;
	else if (pos <  0) pos = n + 1 + pos;
	/*else if (pos == 0) pos = 1; */
	return CJK_STROKE_FIRST_POINT(st) + (pos - 1);
}




CJK_GRIDPOINT cjkStrokeGetMinXGridpoint(CJK_STROKE * pStroke) 
{
	DECUMA_INT32 minx;
	DECUMA_INT32 i, minpos;
	DECUMA_INT32 n = CJK_STROKE_NPOINTS(pStroke);
	minpos = 0;
	minx = BIGNUMBER;
	for (i = 1; i <= n; i++) {
		if (CJK_STROKE_GET_X(pStroke, i) < minx) {
			minx = CJK_STROKE_GET_X(pStroke, i);
			minpos = i;
		}
	}
	return *cjkStrokeGetGridpoint(pStroke, minpos);
}

CJK_GRIDPOINT cjkStrokeGetMinYGridpoint(CJK_STROKE * pStroke) 
{
	DECUMA_INT32 miny;
	DECUMA_INT32 i, minpos;
	DECUMA_INT32 n = CJK_STROKE_NPOINTS(pStroke);
	minpos = 0;
	miny = BIGNUMBER;
	for (i = 1; i <= n; i++) {
		if (CJK_STROKE_GET_Y(pStroke, i) < miny) {
			miny = CJK_STROKE_GET_Y(pStroke, i);
			minpos = i;
		}
	}
	return *cjkStrokeGetGridpoint(pStroke, minpos);
}

CJK_GRIDPOINT cjkStrokeGetMaxXGridpoint(CJK_STROKE * pStroke) 
{
	DECUMA_INT32 maxx;
	DECUMA_INT32 i, maxpos;
	DECUMA_INT32 n = CJK_STROKE_NPOINTS(pStroke);
	maxpos = 0;
	maxx = SMALLNUMBER;
	for (i = 1; i <= n; i++) {
		if (CJK_STROKE_GET_X(pStroke, i) > maxx) {
			maxx = CJK_STROKE_GET_X(pStroke, i);
			maxpos = i;
		}
	}
	return *cjkStrokeGetGridpoint(pStroke, maxpos);
}

CJK_GRIDPOINT cjkStrokeGetMaxYGridpoint(CJK_STROKE * pStroke) 
{
	DECUMA_INT32 maxy;
	DECUMA_INT32 i, maxpos;
	DECUMA_INT32 n = CJK_STROKE_NPOINTS(pStroke);
	maxpos = 0;
	maxy = SMALLNUMBER;
	for (i = 1; i <= n; i++) {
		if (CJK_STROKE_GET_Y(pStroke, i) > maxy) {
			maxy = CJK_STROKE_GET_Y(pStroke, i);
			maxpos = i;
		}
	}
	return *cjkStrokeGetGridpoint(pStroke, maxpos);
}



#undef DEBUG_PRINT_STROKE
#ifdef DEBUG_PRINT_STROKE
#include <stdio.h>
static void st_debug_print(FILE * fh, CJK_STROKE * s) 
{
	char grid[][17] = {
		"                ",
		"                ",
		"                ",
		"                ",
		"                ",
		"                ",
		"                ",
		"                ",
		"                ",
		"                ",
		"                ",
		"                ",
		"                ",
		"                ",
		"                ",
		"                " 
	};
	DECUMA_INT32 i,x, y;
	DECUMA_INT32 n = CJK_STROKE_NPOINTS(s);

	for (i = 1; i < n; i++) {
		x = CJK_STROKE_GET_X(s, i);
		y = CJK_STROKE_GET_Y(s, i);
		grid[y][x] = i < 10 ? i + '0' : i + 'A' - 1;
	}

	fprintf(fh, "\n+----------------+\n");

	for (i = 0; i < 16; i++) {
		fprintf(fh, "|%s|\n", grid[i]);
	}

	fprintf(fh, "+----------------+\n");
}
#endif

void cjkStrokeNext(CJK_STROKE * pStroke, CJK_SESSION * pSession) 
{
	decumaAssert(CJK_STROKE_EXISTS(pStroke));
	if (!CJK_STROKE_EXISTS(pStroke)) {
		return;
	}
	pStroke->stdata += *(pStroke->stdata) + 1;
	decumaAssert(CJK_STROKE_NPOINTS(pStroke) > 0 || pStroke->stdata == pStroke->end); /* No zero len stroke. */
	decumaAssert(pStroke->stdata <= pStroke->end); /* Serious error indeed. */

	if (pStroke->stdata == pStroke->end) {

		/* Check for return from component */
		if (pStroke->stdata_backup) {

			pStroke->stdata = pStroke->stdata_backup;
			pStroke->end = pStroke->end_backup;
			pStroke->stdata_backup = NULL;
			pStroke->end_backup = NULL;
			if (pStroke->stdata != pStroke->end) {

				/* Check jump to components 
				* This chunk is used twice in the program.
				* The insanely strange [[volatile]] is for the broken Green Hill compiler
				* that screws optimization up without this directive.*/
				decumaAssert(pSession || (*(pStroke->stdata) <= MAXPNTSPERSTR));
				if (pSession && (*(pStroke->stdata) > MAXPNTSPERSTR)) {
					int k;
					CJK_COMPRESSED_CHAR_DATA * ccdata;
					pStroke->end_backup = pStroke->end;
					if (*(pStroke->stdata) < pSession->db.firsttwobyteindex) {
						k = *(pStroke->stdata) - (MAXPNTSPERSTR + 1);
						pStroke->stdata_backup = pStroke->stdata + 1;
					}
					else {
						k = ((*(pStroke->stdata) - pSession->db.firsttwobyteindex) << 8)
							+ *(pStroke->stdata + 1)
							+ pSession->db.firsttwobyteindex - (MAXPNTSPERSTR + 1);
						pStroke->stdata_backup = pStroke->stdata + 2;
					}
					ccdata = DLTDB_GET_COMPONENT_DATA(&pSession->db, k);
					pStroke->stdata = ccdata + 1;
					pStroke->end = ccdata + *ccdata;
				}

				return;
			}
		}

		pStroke->stdata = NULL; /* end of cchar */
		return;
	}

	/*-----------------------------------------------------
	* BEGIN CHUNK: possible jump to component
	* 
	* This chunk is used twice in the program.
	* The insanely strange [[volatile]] is for the broken Green Hill compiler
	* that screws optimization up without this directive.
	*/
	decumaAssert(pSession || (*(pStroke->stdata) <= MAXPNTSPERSTR));
	if (pSession && (*(pStroke->stdata) > MAXPNTSPERSTR)) {
		int k;
		CJK_COMPRESSED_CHAR_DATA * ccdata;
		pStroke->end_backup = pStroke->end;

		if (*(pStroke->stdata) < pSession->db.firsttwobyteindex) {
			k = *(pStroke->stdata) - (MAXPNTSPERSTR + 1);
			pStroke->stdata_backup = pStroke->stdata + 1;
		}
		else {
			k = ((*(pStroke->stdata) - pSession->db.firsttwobyteindex) << 8)
				+ *(pStroke->stdata + 1)
				+ pSession->db.firsttwobyteindex - (MAXPNTSPERSTR + 1);
			pStroke->stdata_backup = pStroke->stdata + 2;
		}
		/* TODO */
		ccdata = DLTDB_GET_COMPONENT_DATA(&pSession->db, k);
		/* pSession->db.pBase + pSession->db.pCompData->pIndices[k]; */
		pStroke->stdata = ccdata + 1;
		pStroke->end = ccdata + *ccdata;
	}

	/* END CHUNK: possible jump to component
     *-----------------------------------------------------
	 */

	return;
}


DECUMA_INT32 cjkStrokeGetIntersectionCount(CJK_STROKE  * pStroke, 
										   CJK_SESSION * pSession) 
{
	CJK_GRIDPOINT_ITERATOR gi;
	gi.stroke = *pStroke;
	gi.n = CJK_STROKE_NPOINTS(&gi.stroke);
	gi.p = CJK_STROKE_FIRST_POINT(&gi.stroke);
	return dltGPIterGetIntersectionCount(&gi, gi.n, pSession);
}
