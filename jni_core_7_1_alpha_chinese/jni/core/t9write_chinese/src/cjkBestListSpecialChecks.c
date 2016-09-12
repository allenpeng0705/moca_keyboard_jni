/*****************************************************************************
* COPYRIGHT ZI AND SUBJECT TO CONFIDENTIALITY RESTRICTIONS                   *
*                                                                            *
* This file is the confidential and proprietary property of Zi Corporation   *
* of Canada, Inc. ("Zi") or one of its affiliates.                           *
******************************************************************************/

#include "dltConfig.h"
#include "cjkBestList.h"
#include "cjkCommon.h"
#include "cjkStroke_Macros.h"
#include "cjkMath.h"
#include "cjkSession_Types.h"
#include "cjkDatabase.h"

#include "decumaUnicodeMacros.h"
#include "decumaMemory.h"
#include "decumaIntegerMath.h"

#ifdef ONPALM_ARMLET
#include "spch_redirect.h" /* Do after decuma */
#endif



#define SQR(a) ((a)*(a))

/** @addtogroup DOX_CJK_BESTLIST
*  @{
*  @defgroup DOX_CJK_BESTLIST_SPECIALCHECKS cjkBestListSpecialChecks
*  @{
*/

/** 
* \f{figure}[ht!]
*  \begin{center}
*       \epsfig{file=../../figures/check8F66.eps,width=130mm,height=50mm}
*  \end{center}
* \f}
* 
* When 5728 is written cursively, it is often mixed up with 8F66.
*/ 
static const CJK_UNICHAR uclist_8F66[] = {
	0x8F66,
	0x5728,
	0
};

static const CJK_UNICHAR uclist_5728[] = {
	0x5728,
	0x8F66,
	0
};



/**
* Two hiragana and kanji two stroke mountain and kanji two stroke fire.
* 
* \f{figure}[ht!]
*  \begin{center}
*         \begin{tabular}{cc}
*       \epsfig{file=../../figures/308f.eps,width=50mm,height=50mm}  &
*       \epsfig{file=../../figures/308c.eps,width=50mm,height=50mm}
*         \end{tabular}
*  \end{center}
* \f}
*/ 
static const CJK_UNICHAR uclist_308C[] = {
	0x308C,
	0x308F,
	0
};

static const CJK_UNICHAR uclist_308F[] = {
	0x308F,
	0x308C,
	0
};


/*
* This special check only makes sense in japanese since the characters 0x30AA
* and 0x624D are
* different in japanese, but not in chinese. 0x30AA is katakana
* 
* \f{figure}[h!]
*  \begin{center}
*       \epsfig{file=../../figures/30aa.eps,width=50mm,height=50mm}
*     \end{center}
*  \f}
* 
* \f{figure}[h!]
*  \begin{center}
*       \epsfig{file=../../figures/624d.eps,width=50mm,height=50mm}
*  \end{center}
* \f}
*/ 
static const CJK_UNICHAR uclist_624D[] = {
	0x624D,
	0x30AA,
	0
};

static const CJK_UNICHAR uclist_30AA[] = {
	0x30AA,
	0x624D,
	0
};

static const CJK_UNICHAR uclist_30BF[] = {
	0x30BF,
	0x5915,
	0x52FA,
	0
};



/** 
* \f{figure}[h!]
*  \begin{center}
*    \begin{tabular}{ccc}
*       \epsfig{file=../../figures/4e8c.eps,width=50mm,height=50mm} &
*       \epsfig{file=../../figures/4e59.eps,width=50mm,height=50mm} &
*       \epsfig{file=../../figures/5de5.eps,width=50mm,height=50mm} \\
*       \epsfig{file=../../figures/571f.eps,width=50mm,height=50mm} &
*       \epsfig{file=../../figures/58eb.eps,width=50mm,height=50mm} &
*       \epsfig{file=../../figures/0049.eps,width=50mm,height=50mm}
*    \end{tabular}
*  \end{center}
* \f}
* 
* Unicode 4E8C is the character for two, unicode 4E59 is the character
* which looks like a Z, meaning B in enumerations, and unicode 5DE5 is
* the character meaning work. The digit 2 is not considered here yet.
* If any one of these characters has been drawn with one stroke, that
* is in cursive form, we need to check which one it is. This is done
* further down in the code together with 2 Z z etc.
* The printed forms of soil (571F), soldier (58EB), and work (5DE5) are
* easily confused. Only allow the one stroke cursive forms in chinese.
* 
* First however, we check to see if this is Hiragana 3055 (sa), which is
* easily confused with Kanji 58EB and 571F.
*/
static const CJK_UNICHAR uclist_30CB[] = {
	0x30CB,
	0x3053,
	0x4E8C,
	UC_EQUALS_SIGN,
	0
};

static const CJK_UNICHAR uclist_3053[] = {
	0x3053,
	0x30CB,
	0x4E8C,
	UC_EQUALS_SIGN,
	0
};

static const CJK_UNICHAR uclist_5DE5[] = {
	0x5DE5,
	0x4E8C,
	0x4E59,
	0
};

static const CJK_UNICHAR uclist_58EB[] = {
	0x58EB,
	0x571F,
	0
};

static const CJK_UNICHAR uclist_571F[] = {
	0x571F,
	0x58EB,
	0
};



/* This is a special check originally found in the bl\_punctuationcheck function. */
/* This one is for various dots. */
static const CJK_UNICHAR uclist_high[] = {
	UC_CIRCUMFLEX_ACCENT,
	UC_QUOTATION_MARK,
	UC_APOSTROPHE,
	UC_GRAVE_ACCENT,
	UC_RIGHT_SINGLE_QUOTATION_MARK,
	UC_LEFT_SINGLE_QUOTATION_MARK,
	UC_DIACRITIC_HANDAKUTEN,
	0
};

static const CJK_UNICHAR uclist_middle[] = {
	UC_MIDDLE_DOT,
	UC_BULLET,
	0
};

static const CJK_UNICHAR uclist_low[] = {
	UC_IDEOGRAPHIC_FULL_STOP,
	UC_FULL_STOP,
	UC_COMMA,
	UC_IDEOGRAPHIC_COMMA,
	0
};



/** 
* \f{figure}[h!]
*  \begin{center}
*    \begin{tabular}{cccc}
*       \epsfig{file=../../figures/4e5d.eps,width=50mm,height=50mm} &
*       \epsfig{file=../../figures/5200.eps,width=50mm,height=50mm} &
*       \epsfig{file=../../figures/529b.eps,width=50mm,height=50mm} \\
*       \epsfig{file=../../figures/51e0.eps,width=50mm,height=50mm} &
*       \epsfig{file=../../figures/513f.eps,width=50mm,height=50mm} &
*       \epsfig{file=../../figures/5165.eps,width=50mm,height=50mm}
*    \end{tabular}
*  \end{center}
* \f}
* 
* The four things that look like $\pi$ and the number eight.
*/
static const CJK_UNICHAR uclist_529B[] = {
	0x529B,
	0x30AB,
	0x5200,
	0
};

static const CJK_UNICHAR uclist_30AB[] = {
	0x30AB,
	0x529B,
	0x5200,
	0
};

static const CJK_UNICHAR uclist_5200[] = {
	0x5200,
	0x529B,
	0x30AB,
	0
};


/**
* 
* \f{figure}[h!]
*  \begin{center}
*    \begin{tabular}{cccc}
*       \epsfig{file=../../figures/7530.eps,width=50mm,height=50mm} &
*       \epsfig{file=../../figures/7531.eps,width=50mm,height=50mm} &
*       \epsfig{file=../../figures/7532.eps,width=50mm,height=50mm} &
*       \epsfig{file=../../figures/7533.eps,width=50mm,height=50mm}
*    \end{tabular}
*  \end{center}
* \f}
* 
*/ 
static const CJK_UNICHAR uclist_7530[] = {
	0x7530,
	0x7531,
	0x7532,
	0x7533,
	0
};

static const CJK_UNICHAR uclist_7531[] = {
	0x7531,
	0x7530,
	0x7533,
	0x7532,
	0
};

static const CJK_UNICHAR uclist_7532[] = {
	0x7532,
	0x7530,
	0x7533,
	0x7531,
	0
};

static const CJK_UNICHAR uclist_7533[] = {
	0x7533,
	0x7531,
	0x7532,
	0x7530,
	0
};

static const CJK_UNICHAR uclist_spec0Oo[] = {
	0x006F,
	0x004F,
	0x0030,
	UC_IDEOGRAPHIC_NUMBER_ZERO,
	0x0036,
	UC_IDEOGRAPHIC_FULL_STOP,
	0
};



/** 
* \f{figure}[ht!]
*  \begin{center}
*    \begin{tabular}{cc}
*       \epsfig{file=../../figures/5929.eps,width=50mm,height=50mm} &
*       \epsfig{file=../../figures/592b.eps,width=50mm,height=50mm}
*    \end{tabular}
*  \end{center}
* \f}
*/
static const CJK_UNICHAR uclist_592B[] = {
	0x592b,
	0x5929,
	0
};

static const CJK_UNICHAR uclist_5929[] = {
	0x5929,
	0x592b,
	0
};


/**
* Check context to prevent mixing chinese one, latin minus and
* hiragana-katakana prolonged soundmark.
*/
static const CJK_UNICHAR uclist_dashes[] = {
	0x4E00,
	UC_HYPHEN_MINUS,
	0x30FC,
	UC_EM_DASH,
	UC_LOW_LINE,
	UC_TILDE,
	0
};

/**
* Cursive form for chinese character people (4EBA) and
* latin {\Huge \textbf{L}}.
* 
* \f{figure}[h!]
*  \begin{center}
*    \begin{tabular}{c}
*       \epsfig{file=../../figures/4eba.eps,width=50mm,height=50mm}
*    \end{tabular}
*  \end{center}
* \f}
* 
*/ 
static const CJK_UNICHAR uclist_4EBA[] = {
	0x4EBA,
	0x004C,
	0
};
static const CJK_UNICHAR uclist_004C[] = {
	0x004C,
	0x4EBA,
	0
};



static const CJK_UNICHAR uclist_RDQfix[] = {
	UC_RIGHT_DOUBLE_QUOTATION_MARK,
	UC_LEFT_DOUBLE_QUOTATION_MARK,
	UC_QUOTATION_MARK,
	0
};
static const CJK_UNICHAR uclist_LDQfix[] = {
	UC_LEFT_DOUBLE_QUOTATION_MARK,
	UC_RIGHT_DOUBLE_QUOTATION_MARK,
	UC_QUOTATION_MARK,
	0
};
static const CJK_UNICHAR uclist_LDQfix_halfwidth[] = {
	UC_DIACRITIC_DAKUTEN,
	UC_LEFT_DOUBLE_QUOTATION_MARK,
	UC_RIGHT_DOUBLE_QUOTATION_MARK,
	0
};


static const CJK_UNICHAR uclist_6587[] = {
	0x6587,
	0x4E91,
	0
};
static const CJK_UNICHAR uclist_4E4B[] = {
	0x4E4B,
	0x6587,
	0
};



/**
* A onestroke l or 1 is analyzed here. This is so complicated that
* the behaviour is documented in each chunk. Note that 30CE won's show
* up on on japanese products due to bestlist filtering while 4E28
* is compiled into the code only when emitting simplified for certification.
*/ 
static const CJK_UNICHAR uclist_ell_etc[] = {
	0x006C, 0x0031, 0x0049, 0x30CE,
	UC_SOLIDUS,
	UC_REVERSE_SOLIDUS,
	UC_VERTICAL_LINE,
	0
};
static const CJK_UNICHAR uclist_bracket_left[] = {
	UC_LEFT_PARENTHESIS,
	UC_LEFT_SQUARE_BRACKET,
	UC_LEFT_CURLY_BRACKET,
	UC_LESS_THAN_SIGN,
	UC_LEFT_ANGLE_BRACKET,
	0
};
static const CJK_UNICHAR uclist_bracket_right[] = {
	UC_RIGHT_PARENTHESIS,
	UC_RIGHT_SQUARE_BRACKET,
	UC_RIGHT_CURLY_BRACKET,
	UC_GREATER_THAN_SIGN,
	UC_RIGHT_ANGLE_BRACKET,
	0
};


/**
* The katakana 30F1 often erroneously ends up as 30A8 in the match.
* Note that the check for hook at the end of the first stroke is
* heavily dependent on the sampling method. It works a little
* bit better than the obvious check. Furthermore, it is a kanji
* unless the previous was a katakana.
*/ 
static const CJK_UNICHAR uclist_30F1_3st[] = {0x30F1, 0x30A8, 0x5DE5, 0x0049, 0};
static const CJK_UNICHAR uclist_30A8_3st[] = {0x30A8, 0x5DE5, 0x30F1, 0x0049, 0};
static const CJK_UNICHAR uclist_5DE5_3st[] = {0x5DE5, 0x30A8, 0x30F1, 0x0049, 0};


/**
* A number two with one stroke should not end up as twostroked or
* threestroked kana. We also check the difference between number two and
* hiragana 3066 and latin Z and some other stuff. Give only number 2 or
* hiragana 3066 as result. Do you want to have Z?
* Write the two stroked version! No hey, we check that also!
* In chinese the character 53C8 is often written in one stroke.
*/
static const CJK_UNICHAR uclist_3066[] = {
	0x3066, 0x0032, 0x005A, 0x007A, 0x30A8,
	0x30E6, 0x30B3, 0x4E59, 0x5DE5, 0x4E8C, 0
};

static const CJK_UNICHAR uclist_0032[] = {
	0x0032, 0x005A, 0x007A, 0x4E59, 0x5DE5, 0x4E8C, 0
};

static const CJK_UNICHAR uclist_53C8[] = {
	0x53C8, 0x0032, 0x005A, 0x007A, 0x4E59, 0x5DE5, 0x4E8C, 0
};

static const CJK_UNICHAR uclist_007A[] = {
	0x007A, 0x005A, 0x0032, 0x4E59, 0x5DE5, 0x4E8C, 0
};

static const CJK_UNICHAR uclist_4E59[] = {
	0x4E59, 0x5DE5, 0x4E8C, 0x005A, 0x007A, 0x0032, 0
};



/* -------------------------------------------------------------------------
* Zoom helper functions
* 
* These functions facilitates analysis of details in the original stroke(s).
* ------------------------------------------------------------------------- */ 

/**
* A zoom stroke is created from a DECUMA_ARC, and points to the same point data
* as the original stroke. Used to hold metadata used by zoom functions.
*/
typedef struct _tagCJK_ZOOMSTROKE {
	int nPoints;
	int nStartIdx;
	int nEndIdx;
	int length;
	const DECUMA_POINT * pPoints;
} CJK_ZOOMSTROKE;

typedef enum _tagZM_VERTICAL_DIR {
	V_DOWN = -1,
	V_NONE =  0,
	V_UP   =  1
} ZM_VERTICAL_DIR;

typedef enum _tagZM_HORIZONTAL_DIR {
	H_LEFT  = -1,
	H_NONE  =  0,
	H_RIGHT =  1
} ZM_HORIZONTAL_DIR;

/**
* Creates a CJK_ZOOMSTROKE object from a DECUMA_ARC. No additional metadata is calculated.
*/
static void zm_create_zoom_stroke(CJK_COMPRESSED_CHAR * c, int stroke_idx, CJK_ZOOMSTROKE * pZoomStroke)
{
	DECUMA_ARC * p_original_stroke;
	DECUMA_ARC * pOriginalStrokes = dltCCCompressGetOrgStrokes(c);

	decumaAssert(stroke_idx < dltCCCompressGetNbrStrokes(c));
	/* WARNING : Sampler stroke is 1-based while original uses direct array access!!! */

	decumaMemset(pZoomStroke, 0, sizeof(*pZoomStroke));

	p_original_stroke      = &pOriginalStrokes[stroke_idx];
	pZoomStroke->nPoints   = p_original_stroke->nPoints;
	pZoomStroke->pPoints   = (const DECUMA_POINT*) p_original_stroke->pPoints;
	pZoomStroke->nStartIdx = 0;
	pZoomStroke->nEndIdx   = pZoomStroke->nPoints - 1;
	pZoomStroke->length    = -1;
}



/**
* Resets a CJK_ZOOMSTROKE object. All metadata needs to be recalculated after this.
*/ 
#if 0

static void zm_reset_zoom_stroke(CJK_ZOOMSTROKE * pZoomStroke)
{
	pZoomStroke->nStartIdx = 0;
	pZoomStroke->nEndIdx   = pZoomStroke->nPoints - 1;
	pZoomStroke->length    = -1;
}

#endif


/**
* Calcualte the extent of a CJK_ZOOMSTROKE.
*/ 
static void zm_get_stroke_extent(CJK_ZOOMSTROKE * pZoomStroke, DECUMA_INT32 * pXMax, DECUMA_INT32 * pXMin, DECUMA_INT32 * pYMax, DECUMA_INT32 * pYMin)
{
	int i;
	DECUMA_INT32 xmax, ymax, xmin, ymin;

	xmax = ymax = MIN_DECUMA_INT32;
	xmin = ymin = MAX_DECUMA_INT32;

	for (i = 0; i < pZoomStroke->nPoints; i++) {
		if (pZoomStroke->pPoints[i].x > xmax) xmax = pZoomStroke->pPoints[i].x;
		if (pZoomStroke->pPoints[i].x < xmin) xmin = pZoomStroke->pPoints[i].x;
		if (pZoomStroke->pPoints[i].y > ymax) ymax = pZoomStroke->pPoints[i].y;
		if (pZoomStroke->pPoints[i].y < ymin) ymin = pZoomStroke->pPoints[i].y;
	}

	if (pXMax) *pXMax = xmax;
	if (pYMax) *pYMax = ymax;
	if (pXMin) *pXMin = xmin;
	if (pYMin) *pYMin = ymin;
}



/**
* Calculate length of a stroke. Result is returned and stored in the stroke object.
*/
static int zm_get_stroke_length(CJK_ZOOMSTROKE * pZoomStroke)
{
	int i;
	int l = 0;

	for (i = pZoomStroke->nStartIdx; i <= pZoomStroke->nEndIdx - 1; i++)  {
		l += isqrt(SQR(pZoomStroke->pPoints[i + 1].x - pZoomStroke->pPoints[i].x) + SQR(pZoomStroke->pPoints[i + 1].y - pZoomStroke->pPoints[i].y));
	}

	pZoomStroke->length = l;   
	return l;
}



/**
* Remove noise from the edges of a CJK_ZOOMSTROKE. Returns true if noise was removed.
*/ 
static int zm_remove_edge_noise(CJK_ZOOMSTROKE * pZoomStroke) 
{
	int L = zm_get_stroke_length(pZoomStroke);

	/* Any sequence of small line segments in the begining and end of the stroke is considered noise. */
	/* TODO improve on this, condition on directions too? */

	int bHasChanged = 0;
	int nStartIdx, nEndIdx;
	int l = 0;

	nStartIdx = pZoomStroke->nStartIdx;
	l += isqrt(SQR(pZoomStroke->pPoints[nStartIdx + 1].x - pZoomStroke->pPoints[nStartIdx].x) + SQR(pZoomStroke->pPoints[nStartIdx + 1].y - pZoomStroke->pPoints[nStartIdx].y));
	while(l * 10 < L && nStartIdx <= pZoomStroke->nStartIdx + (pZoomStroke->nEndIdx - pZoomStroke->nStartIdx) / 4)  {
		nStartIdx++;
		l += isqrt(SQR(pZoomStroke->pPoints[nStartIdx + 1].x - pZoomStroke->pPoints[nStartIdx].x) + SQR(pZoomStroke->pPoints[nStartIdx + 1].y - pZoomStroke->pPoints[nStartIdx].y));
	}

	l = 0;
	nEndIdx = pZoomStroke->nEndIdx;
	l += isqrt(SQR(pZoomStroke->pPoints[nEndIdx - 1].x - pZoomStroke->pPoints[nEndIdx].x) + SQR(pZoomStroke->pPoints[nEndIdx - 1].y - pZoomStroke->pPoints[nEndIdx].y));
	while(l * 10 < L && nEndIdx >= pZoomStroke->nStartIdx + 3 * (pZoomStroke->nEndIdx - pZoomStroke->nStartIdx) / 4) {
		nEndIdx--;
		l += isqrt(SQR(pZoomStroke->pPoints[nEndIdx - 1].x - pZoomStroke->pPoints[nEndIdx].x) + SQR(pZoomStroke->pPoints[nEndIdx - 1].y - pZoomStroke->pPoints[nEndIdx].y));
	}


	bHasChanged = (pZoomStroke->nStartIdx != nStartIdx) || (pZoomStroke->nEndIdx != nEndIdx);

	pZoomStroke->nStartIdx = nStartIdx;
	pZoomStroke->nEndIdx   = nEndIdx;

	return bHasChanged;
}



/**
* Count the number of points to the right of line P0P1 when standing at 
* P0=(x0, y0)looking towards P1=(x1, y1).
*/
static int zm_npoints_right_of_line(CJK_ZOOMSTROKE * pZoomStroke, int x0, int y0, int x1, int y1)
{
	int i;
	int n = 0;

	for (i = pZoomStroke->nStartIdx; i <= pZoomStroke->nEndIdx; i++)  {
		int x2 = pZoomStroke->pPoints[i].x;
		int y2 = pZoomStroke->pPoints[i].y;
		int a = x0 * y1 - x1 * y0 - x0 * y2 + y0 * x2 + x1 * y2 - x2 * y1;
		n += (a < 0);
	}

	return n;
}


/**
* Finds the smallest angle in a stroke, limited to the range 
* defined by nStartIdx and nEndIdx. Returns the cosine of the angle * 1000,
* or MAX_DECUMA_INT32 if there are not enough line segments.
* 
* TODO Known bugs: If a line segment has length 0, that angle between line segments 
* before and after is not used as it should.
*/ 
static DECUMA_INT32 zm_cosine_of_smallest_angle(CJK_ZOOMSTROKE * pZoomStroke) 
{
	int i          = pZoomStroke->nStartIdx + 1;
	int cosine_max = -1000;

	/* Keep calculated values in array to avoid recalculation */
	int square_distance[2];

	if (pZoomStroke->nEndIdx - i + 1 < 3) return MAX_DECUMA_INT32;   

	square_distance[i % 2] = SQR(pZoomStroke->pPoints[i].x - pZoomStroke->pPoints[i - 1].x) + SQR(pZoomStroke->pPoints[i].y - pZoomStroke->pPoints[i - 1].y);

	for (i++; i <= pZoomStroke->nEndIdx; i++) {
		int cosine;
		int t;

		square_distance[i % 2]   = SQR(pZoomStroke->pPoints[i].x - pZoomStroke->pPoints[i - 1].x) + SQR(pZoomStroke->pPoints[i].y - pZoomStroke->pPoints[i - 1].y);

		cosine = ((pZoomStroke->pPoints[i - 2].x - pZoomStroke->pPoints[i - 1].x) * (pZoomStroke->pPoints[i].x - pZoomStroke->pPoints[i - 1].x)  + 
			(pZoomStroke->pPoints[i - 2].y - pZoomStroke->pPoints[i - 1].y) * (pZoomStroke->pPoints[i].y - pZoomStroke->pPoints[i - 1].y)) * 1000;

		t = square_distance[(i - 1) % 2] * square_distance[i % 2];

		if (t != 0) {
			cosine = cosine / (int)isqrt(t);
			if (cosine > cosine_max) cosine_max = cosine;
		}
	}

	return cosine_max;
}



/**
* Finds the area of the polygon that the stroke would constitute if the start and 
* end points were connected.
*/ 
static DECUMA_INT32 zm_area(CJK_ZOOMSTROKE * pZoomStroke) 
{
	int i    = pZoomStroke->nStartIdx;
	int area = 0;

	/* Keep calculated values in array to avoid recalculation */

	int x[2];
	int y[2];

	x[i % 2] = pZoomStroke->pPoints[i].x;
	y[i % 2] = pZoomStroke->pPoints[i].y;

	for (i++; i <= pZoomStroke->nEndIdx; i++)  {

		x[i % 2] = pZoomStroke->pPoints[i].x;
		y[i % 2] = pZoomStroke->pPoints[i].y;

		area += x[(i - 1) % 2] * y[i % 2] - x[i % 2] * y[(i - 1) % 2];
	}

	x[i % 2] = pZoomStroke->pPoints[pZoomStroke->nStartIdx].x;
	y[i % 2] = pZoomStroke->pPoints[pZoomStroke->nStartIdx].y;

	area += x[(i - 1) % 2] * y[i % 2] - x[i % 2] * y[(i - 1) % 2];

	return area;
}




/** @returns the manhattan distance moved. */
#define zm_go_down(m_p_stroke)  zm_go_direction((m_p_stroke), V_DOWN, H_NONE)

/** @returns the manhattan distance moved. */
#define zm_go_up(m_p_stroke)    zm_go_direction((m_p_stroke), V_UP,   H_NONE)

/** @returns the manhattan distance moved. */
#define zm_go_left(m_p_stroke)  zm_go_direction((m_p_stroke), V_NONE, H_LEFT)

/** @returns the manhattan distance moved. */
#define zm_go_right(m_p_stroke) zm_go_direction((m_p_stroke), V_NONE, H_RIGHT)

/** @returns the manhattan distance moved. */
#define zm_go_ne(m_p_stroke)    zm_go_direction((m_p_stroke), V_UP,   H_RIGHT)

/** @returns the manhattan distance moved. */
#define zm_go_se(m_p_stroke)    zm_go_direction((m_p_stroke), V_DOWN, H_RIGHT)

/** @returns the manhattan distance moved. */
#define zm_go_nw(m_p_stroke)    zm_go_direction((m_p_stroke), V_UP,   H_LEFT)

/** @returns the manhattan distance moved. */
#define zm_go_sw(m_p_stroke)    zm_go_direction((m_p_stroke), V_DOWN, H_LEFT)


/**
* @param pZoomStroke
* @param vertDir
* if up_down    > 0 go up,     up_down    < 0 go down.
* if right_left > 0 go right,  right_left < 0 go left.
*/
static DECUMA_INT32 zm_go_direction(CJK_ZOOMSTROKE * pZoomStroke, ZM_VERTICAL_DIR vertDir, ZM_HORIZONTAL_DIR horDir) 
{
	DECUMA_INT32 dist          = 0;
	int   new_start_idx = pZoomStroke->nStartIdx;
	int   ypos          = pZoomStroke->pPoints[new_start_idx].y;
	int   xpos          = pZoomStroke->pPoints[new_start_idx].x;

	decumaAssert (vertDir != V_NONE || horDir != H_NONE);

	while (new_start_idx < pZoomStroke->nEndIdx) {

		if (vertDir != V_NONE && horDir != H_NONE) {
			if ( (vertDir == V_UP    ? pZoomStroke->pPoints[new_start_idx + 1].y > ypos : pZoomStroke->pPoints[new_start_idx + 1].y < ypos) || 
				(horDir  == H_RIGHT ? pZoomStroke->pPoints[new_start_idx + 1].x < xpos : pZoomStroke->pPoints[new_start_idx + 1].x > xpos)) 
			{
				break;
			}
		}
		else if (vertDir != V_NONE) {
			if (vertDir == V_UP    ? pZoomStroke->pPoints[new_start_idx + 1].y >= ypos : pZoomStroke->pPoints[new_start_idx + 1].y <= ypos)
				break;
		}
		else {
			if (horDir  == H_RIGHT ? pZoomStroke->pPoints[new_start_idx + 1].x <= xpos : pZoomStroke->pPoints[new_start_idx + 1].x >= xpos)
				break;
		}

		dist += vertDir == V_DOWN ? pZoomStroke->pPoints[new_start_idx + 1].y - ypos :
			ypos - pZoomStroke->pPoints[new_start_idx + 1].y;

		dist += horDir == H_RIGHT ? pZoomStroke->pPoints[new_start_idx + 1].x - xpos :
			xpos - pZoomStroke->pPoints[new_start_idx + 1].x;

		new_start_idx++;

		ypos = pZoomStroke->pPoints[new_start_idx].y;
		xpos = pZoomStroke->pPoints[new_start_idx].x;

	}

	pZoomStroke->nStartIdx = new_start_idx;

	return dist;
}


/*----------------------------------------------------- */
/* BEGIN CHUNK: function specialcheck */
/*  */
/* %---------------------------------------------------------------------------- */
/* \subsubsection*{The method [[cjkBestListSpecialCheck]]} */
/*  */
/* This method takes a bestlist and the corresponding input character and checks */
/* for special risk confusions by analysing local features (for example if a */
/* vertical line starts above a certain horizontal line), the contextvariable */
/* and for some unichars adds some similar unichars in bestlist. It fills in */
/* the bestlist, with the new proposals. The context variable is sometimes */
/* checked, to get a hint of if the current character is digit or a character. */
/*  */
/* The strange logic below comes from the fact that japanese is always */
/* the only language but simplified and traditional chinese may coexist. */
/*  */
void cjkBestListSpecialCheck(CJK_COMPRESSED_CHAR * c, CJK_SESSION * pSession) {
	CJK_BESTLIST * bl = &pSession->db_lookup_bl;
	CJK_STROKE s1, s2, s3, s4, s5, s6, s7;
	CJK_STROKE s_fromend[4];
	CJK_UNICHAR u, u_cross, u_nocross;
	CJK_CONTEXT * con = &pSession->con;
	DECUMA_UINT32 categorymask = pSession->sessionCategories;
	DECUMA_INT32 nStrokes = dltCCCompressGetNbrStrokes(c);
	DECUMA_INT32 boxmid = pSession->boxybase + pSession->boxheight / 2;

	u = bl->unichar[0];


	/*----------------------------------------------------- */
	/* BEGIN CHUNK: fill s1 to s7 */
	/*  */
	/*  */
	dltCCharGetFirstStroke(c, &s1, pSession);
	s2 = s1; cjkStrokeNext(&s2, pSession);
	s3 = s2; if (CJK_STROKE_EXISTS(&s2)) cjkStrokeNext(&s3, pSession);
	s4 = s3; if (CJK_STROKE_EXISTS(&s3)) cjkStrokeNext(&s4, pSession);
	s5 = s4; if (CJK_STROKE_EXISTS(&s4)) cjkStrokeNext(&s5, pSession);
	s6 = s5; if (CJK_STROKE_EXISTS(&s5)) cjkStrokeNext(&s6, pSession);
	s7 = s6; if (CJK_STROKE_EXISTS(&s6)) cjkStrokeNext(&s7, pSession);
	/* END CHUNK: fill s1 to s7 */
	/*----------------------------------------------------- */



	/*----------------------------------------------------- */
	/* BEGIN CHUNK: fill s fromend */
	/*  */
	{
		CJK_STROKE s;
		DECUMA_INT32 i, j;
		dltCCharGetFirstStroke(c, &s, pSession);
		for (i = 1; i <= nStrokes; i++) {
			j = nStrokes - i;
			if (j >= 0 && j < 4) {
				s_fromend[j] = s;
			}
			cjkStrokeNext(&s, pSession);
		}
	}
	/* END CHUNK: fill s fromend */
	/*----------------------------------------------------- */



	if (nStrokes == 1 && (pSession->db.categoryMask & CJK_PUNCTUATION)) {
		switch (u) {

			/*----------------------------------------------------- */
			/* BEGIN CHUNK: punctuation specialcheck onestroke */
			/*  */
			case UC_FULL_STOP:
			case UC_MIDDLE_DOT:
			case UC_BULLET:
			case UC_IDEOGRAPHIC_COMMA:
			case UC_IDEOGRAPHIC_FULL_STOP:
			case UC_APOSTROPHE:
			case UC_LEFT_SINGLE_QUOTATION_MARK:
				{
					int xmin, xmax, ymin, ymax;
					dltCCCompressGetMaxMin(c, &xmin, &xmax, &ymin, &ymax);
					/* Dots */
					if (((ABS(ymax - ymin) * 20 < pSession->boxheight) &&
						((ABS(xmax - xmin) * 20 < pSession->boxwidth))) ||
						(((ABS(xmax - xmin) * 10 < pSession->boxwidth)) &&
						(ABS(ymin - pSession->boxybase) * 3 >= pSession->boxheight))) {
							if ((ABS(ymax - pSession->boxybase) * 3 > pSession->boxheight * 2)) {
								cjkBestListBoost((CJK_UNICHAR*)uclist_low, 0, pSession);
							}
							else if ((ABS(ymax - pSession->boxybase) * 3 > pSession->boxheight)){
								cjkBestListBoost((CJK_UNICHAR*)uclist_middle, 0, pSession);
							}
							else {
								cjkBestListBoost((CJK_UNICHAR*)uclist_high, 0, pSession);
							}
					}
					else if (5 * ABS(ymax - pSession->boxybase) < 2 * pSession->boxheight) {
						/* upper half of input square */
						cjkBestListBoost((CJK_UNICHAR*)uclist_high, 0, pSession);
						if (decumaIsKatakanaAnySize(cjkContextGetPrevious(con))) {
							CJK_UNICHAR handakuList[2] = {UC_DIACRITIC_HANDAKUTEN, 0
							};
							cjkBestListBoost((CJK_UNICHAR*)handakuList, 1, pSession);
						}
					}
					return;
				}
			case UC_COMMA:
			case UC_RIGHT_SINGLE_QUOTATION_MARK:
				{
					/* Upper half */
					int xmin, xmax, ymin, ymax;
					dltCCCompressGetMaxMin(c, &xmin, &xmax, &ymin, &ymax);
					if (5 * ABS(ymax - pSession->boxybase) < 2 * pSession->boxheight) {
						cjkBestListInsertFirst(UC_RIGHT_SINGLE_QUOTATION_MARK, pSession);
					}
					else {
						cjkBestListInsertFirst(UC_COMMA, pSession);
					}
					return;
				}
			case UC_SPACE:
			case UC_CIRCUMFLEX_ACCENT:
				{
					int xmin, xmax, ymin, ymax;
					dltCCCompressGetMaxMin(c, &xmin, &xmax, &ymin, &ymax);
					if (5 * ABS(ymax - pSession->boxybase) < 2 * pSession->boxheight) {
						cjkBestListInsertFirst(UC_CIRCUMFLEX_ACCENT, pSession);
					}
					else {
						cjkBestListInsertFirst(UC_SPACE, pSession);
					}
					return;
				}
		}
	}
	if (DLTDB_IS_SIMP(pSession->db)) { /* Simplified DB */
		if (nStrokes == 1 && (pSession->db.categoryMask & CJK_GB2312)) {
			switch (u) {

				/*----------------------------------------------------- */
				/* BEGIN CHUNK: specialcheck onestroke SIMP */
				/*  */
				/* A onestroke surname {\sl Ma} (Horse) should not be [[5199]]. */
				/*  */
			case 0x5199:
				if (CJK_STROKE_NPOINTS(&s1) <= 13) {
					cjkBestListInsertFirst(0x9A6C, pSession);
				}
				return;
				/*  */
				/*  */
				/* A onestroke ``birth'' or ``live'' [[751F]] should not be */
				/* confused with the simplified form of ``village'' [[91CC]]. */
				/*  */
			case 0x751F:
			case 0x91CC:
				if (dltCCharGetIntersectionCount(c, pSession) <= 1) {
					cjkBestListInsertFirst(0x91CC, pSession);
				}
				else {
					cjkBestListInsertFirst(0x751F, pSession);
				}
				return;
				/**
				* \f{figure}[h!]
				*  \begin{center}
				*    \begin{tabular}{cccc}
				*       \epsfig{file=../../figures/53cd.eps,width=50mm,height=50mm} &
				*       \epsfig{file=../../figures/4ec5.eps,width=50mm,height=50mm}
				*    \end{tabular}
				*  \end{center}
				* \f}
				* 
				* The two characters ``turn against'' and ``only''.
				*/
			case 0x53CD:
			case 0x4EC5:
			case 0x006B: {
				DECUMA_INT32 x1 = CJK_STROKE_GET_X(&s1, 1);
				DECUMA_INT32 y1 = CJK_STROKE_GET_Y(&s1, 1);
				DECUMA_INT32 x2 = CJK_STROKE_GET_X(&s1, 2);
				DECUMA_INT32 y2 = CJK_STROKE_GET_Y(&s1, 2);
				if (y2 > y1) {
					if (x1 - x2 >= y2 - y1) {
						cjkBestListInsertFirst(0x53CD, pSession);
					}
					else {
						cjkBestListInsertFirst(0x006B, pSession);
					}
				}
						 }
						 return;
						 /* END CHUNK: specialcheck onestroke SIMP */
						 /*----------------------------------------------------- */


			}
		}
	}

	if (nStrokes == 1) {
		if (DLTDB_IS_JAPANESE(pSession->db)) {
			switch (u) {

				/*----------------------------------------------------- */
				/* BEGIN CHUNK: specialcheck onestroke JAPN */
				/*  */
				/*  */
				/* A one stroke hiragana 3089 should never be on the first place, */
				/* because of confusion with hiragana 308D. */
				/*  */
			case 0x3089:
				if (DLTDB_IS_JAPANESE(pSession->db)) {
					cjkBestListInsertFirst(0x308D, pSession);
					goto CASE_0x4E87_ONE;
				}
				/*  */
				/*  */
				/* A katakana 30DE often ends up as a 30A2. We fix this. */
				/*  */
			case 0x30A2:
CASE_0x30A2_ONE_TWO:
				if (DLTDB_IS_JAPANESE(pSession->db)) {
					CJK_STROKE s = s1;
					if (nStrokes == 2) {
						s = s2;
					}
					if (3 * (CJK_STROKE_GET_X(&s, -1) - CJK_STROKE_GET_X(&s, -3)) >
						CJK_STROKE_GET_Y(&s, -1) - CJK_STROKE_GET_Y(&s, -3)) {
							cjkBestListInsertFirst(0x30DE, pSession);
					}
					return;
				}
				/*  */
				/*  */
				/* A hiragana 308A in one stroke often ends up as a 30EA. We fix this by */
				/* assuming one stroke means a U30EA. */
			case 0x30EA:
				cjkBestListInsertFirst(0x308A, pSession);
				return;
				/* END CHUNK: specialcheck onestroke JAPN */
				/*----------------------------------------------------- */


			}
		}
		switch (u) {

			/**
			* BEGIN CHUNK: specialcheck onestroke
			* 
			* 
			* \f{figure}[h!]
			*  \begin{center}
			*    \begin{tabular}{ccc}
			*       \epsfig{file=../../figures/3007.eps,width=50mm,height=50mm} &
			*       \epsfig{file=../../figures/0030.eps,width=50mm,height=50mm} &
			*       \epsfig{file=../../figures/0036.eps,width=50mm,height=50mm}
			*    \end{tabular}
			*  \end{center}
			* \f}
			* 
			* The two characters 0x738B (a common surname, Wang, meaning King) and
			* 0x4E94 (meaning chinese five) could be cursively written in the same
			* way. When writing any of these characters with one stroke, the more
			* common character 0x738B will be inserted on first place.
			*/
		 case 0x4E94:
			 cjkBestListInsertFirst(0x738B, pSession);
			 return;
			 /*  */
			 /* The three characters 0x5B50 (a common character, meaning Child), */
			 /* 0x5B51 and 0x5B53 (together meaning mosquito baby) could be */
			 /* cursively written in the same way. When writing any of these */
			 /* characters with one stroke, the more common character 0x5B50 will */
			 /* be inserted on first place. */
			 /*  */
		 case 0x5B51:
		 case 0x5B53:
			 cjkBestListInsertFirst(0x5B50, pSession);
			 return;
			 /*  */
			 /* The difference between an arabic zero and a chinese zero is the */
			 /* aspect ratio. A check for number arabic number 6 is also included. */
			 /* With help from the context paramater, the zero symbols are separated from */
			 /* small and capital o. */
			 /*  */
		 case UC_IDEOGRAPHIC_NUMBER_ZERO:
		 case 0x0030:
		 case 0x006F:
		 case 0x004F:
			 {
				int xmin, xmax, ymin, ymax;
				dltCCCompressGetMaxMin(c, &xmin, &xmax, &ymin, &ymax);
				cjkBestListInsertMany(uclist_spec0Oo, pSession);
				if ((categorymask & (CJK_HAN | CJK_HIRAGANA | CJK_KATAKANA)) &&
					 pSession->boxheight && ((ymin - pSession->boxybase) * 4 > pSession->boxheight * 3)) {
					cjkBestListInsertFirst(UC_IDEOGRAPHIC_FULL_STOP, pSession);
				}
				else if (CJK_STROKE_GET_Y(&s1, -1) > CJK_STROKE_GET_Y(&s1, 1) + 2) {
					cjkBestListInsertFirst(0x0036, pSession); /* 6 */
				}
				else if (cjkContextHasPrevious(con)) {
					if (decumaIsDigit(cjkContextGetPrevious(con))) {
					 cjkBestListInsertFirst(0x0030, pSession); /* 0 */
					}
					else if (decumaIsHan(cjkContextGetPrevious(con))) {
					 cjkBestListInsertFirst(UC_IDEOGRAPHIC_NUMBER_ZERO, pSession);
					}
				}
				if (decumaIsKatakanaAnySize(cjkContextGetPrevious(con))) {
					cjkBestListInsertAt(UC_DIACRITIC_HANDAKUTEN, 0, 2, pSession);
				}
				return;
			}

		 case 0x0039:
		 case 0x0047:
		 case 0x0061:
		 case 0x0064:
		 case 0x0071:
			 if (CJK_STROKE_GET_Y(&s1, -1) - CJK_STROKE_GET_Y(&s1, -2) > 0) {
				 DECUMA_INT32 y1up = 15, y2up = 15, y1down = 0, y2down;
				 DECUMA_INT32 y, i, state = 0, downwards;
				 for (i = 2; i < CJK_STROKE_NPOINTS(&s1); i++) {
					 y = CJK_STROKE_GET_Y(&s1, i);
					 downwards  = y - CJK_STROKE_GET_Y(&s1, i - 1);
					 if (state == 0 && downwards > 0) {
						 state = 1;
					 }
					 if (state == 1 && downwards < 0) {
						 state = 2;
					 }
					 if (state == 2 && downwards > 0) {
						 state = 3;
					 }
					 if (state <= 1 && y < y1up) {
						 y1up = y;
					 }
					 if (state == 2 && y > y1down) {
						 y1down = y;
					 }
					 if (state == 2 && y < y2up) {
						 y2up = y;
					 }
				 }
				 y2down = CJK_STROKE_GET_Y(&s1, -1);
				 /*printf("XXX %d %d %d %d\n", y1up, y2up, y2down, y2down); */
				 if (y1up - y2up >= 3) {
					 cjkBestListInsertFirst(0x0064, pSession); /* d */
				 }
				 else if (y2down - y1down >= 6) {
					 cjkBestListInsertFirst(0x0039, pSession); /* 9 */
					 goto CASE_0x0079_ONE;
				 }
				 else if (y2up - y1up <= 5 ||
					 (u == 0x0039 && y2down - y1down <= 4)) {
						 cjkBestListInsertFirst(0x0061, pSession); /* a */
				 }
				 else if (u == 0x0039) {
					 goto CASE_0x0079_ONE;
				 }
			 }
			 return;
			 /*  */
			 /*  */
			 /* Latin 9 and q, only on the allograph that ends downwards we */
			 /* need to use context. */
			 /*  */
		 case 0x0067:
		 case 0x0079: {
CASE_0x0079_ONE:
			 if (pSession -> boxheight > 0) {
				 DECUMA_INT32 ymin_percent = (100 * (dltCCCompressGetYmin(c) - pSession->boxybase)) / pSession->boxheight;
				 DECUMA_INT32 ymax_percent = (100 * (dltCCCompressGetYmax(c) - pSession->boxybase)) / pSession->boxheight;
				 const CJK_GRIDPOINT * gpclose;
				 if (ymin_percent < 25 && ymax_percent < 75) {
					 cjkBestListInsertFirst(0x0039, pSession); /* 9 */
				 }
				 else if (ymin_percent > 25 && ymax_percent > 75) {
					 CJK_GRIDPOINT gp1;
					 CJK_GRIDPOINT_ITERATOR gi;
					 DECUMA_INT32 dist, distanceLimit;

					 dltGPIterInit(&gi, c, pSession);
					 gp1 = CJK_GPITER_GET_GRIDPOINT(&gi);

					 if (dltCCCompressIsDense(c)){
						 gpclose = cjkStrokeGetGapGridpoint(&s1, 4, pSession);
						 distanceLimit = 1;
					 }
					 else{
						 gpclose = cjkStrokeGetGapGridpoint(&s1, 2, pSession);
						 distanceLimit = 3;
					 }

					 dist = ABS(CJK_GP_GET_X(gp1) - CJK_GP_GET_X(*gpclose));
					 if (dist <= distanceLimit){
						 cjkBestListInsertFirst(0x0067, pSession); /* g */
					 }
					 else {
						 cjkBestListInsertFirst(0x0079, pSession); /* y */
					 }
				 }
			 }
			 return;
					  }
					  /*  */
					  /* Note that the code works without boxsize set. */
					  /*  */
		 case 0x4E00:
		 case 0x30FC:
		 case UC_EM_DASH:
		 case UC_HYPHEN_MINUS:
		 case UC_LOW_LINE: {
#if defined(CERTIFICATION)
			 DECUMA_INT32 wide = (pSession->boxheight != 0) &&
				 ((dltCCCompressGetXmax(c) - dltCCCompressGetXmin(c)) * 4 > pSession->boxwidth * 3);
#endif
			 DECUMA_INT32 narrow = (dltCCCompressGetXmax(c) - dltCCCompressGetXmin(c)) * 8 < pSession->boxwidth * 3;
			 DECUMA_INT32 low = (pSession->boxheight != 0) &&
				 ((dltCCCompressGetYmin(c) - pSession->boxybase) * 3 > pSession->boxheight * 2);
			 DECUMA_INT32 high = (dltCCCompressGetYmax(c) - pSession->boxybase) * 3 < pSession->boxheight;
			 cjkBestListInsertMany(uclist_dashes, pSession);

			 if (low) {
				 cjkBestListInsertFirst(UC_LOW_LINE, pSession);
			 }
#if defined(CERTIFICATION)
			 else if (wide) {
				 cjkBestListInsertFirst(UC_EM_DASH, pSession);
			 }
#endif
			 else if ( (DLTDB_IS_JAPANESE(pSession->db)) && ( CJK_GP_GET_Y(cjkStrokeGetMaxYGridpoint(&s1)) - CJK_GP_GET_Y(cjkStrokeGetMinYGridpoint(&s1)) >= 3 &&
				 CJK_STROKE_GET_Y(&s1, -1) >= 9 && CJK_STROKE_GET_Y(&s1, 1) >= 9 ) ) { /* Japanese DB */
					 goto CASE_0x3078_ONE;
			 }
			 else if ( (DLTDB_IS_JAPANESE(pSession->db)) && ( (decumaIsHiraganaAnySize(cjkContextGetPrevious(con)) ||
				 decumaIsKatakanaAnySize(cjkContextGetPrevious(con))) ) ) {
					 cjkBestListInsertFirst(0x30FC, pSession);
			 }
			 else if (narrow) {
				 if (high) {
					 cjkBestListInsertFirst(UC_CIRCUMFLEX_ACCENT, pSession);
				 }
#if defined (CERTIFICATION)
				 else {
					 cjkBestListInsertFirst(UC_HYPHEN_MINUS, pSession);
				 }
#endif
			 }
			 else if (decumaIsDigit(cjkContextGetPrevious(con)) ||
				 decumaIsLatin(cjkContextGetPrevious(con))) {
					 cjkBestListInsertFirst(UC_HYPHEN_MINUS, pSession);
			 }
			 else {
				 cjkBestListInsertFirst(0x4E00, pSession);
			 }
											   }
											   return;
											   /*  */
		 case 0x4EBA:
			 if (cjkContextHasPrevious(con)) {
				 if (decumaIsHan(cjkContextGetPrevious(con))) {
					 cjkBestListInsertMany(uclist_4EBA, pSession);
				 }
				 else {
					 cjkBestListInsertMany(uclist_004C, pSession);
				 }
			 }
			 return;
			 /*  */
			 /*  */
			 /* The Hiragana character 3057 is confused with katakana 30EC, latin 0043 (C), */
			 /* latin 0063 (c), latin 004C (L), latin 0056 (V) and latin 0076 (v), */
			 /* latin 0055 (U) and latin 0075 (u). And hiragana 3093, 304F as */
			 /* well as katakana 30E0. And some parenthesis and other signs. */
			 /* And if a V or v is in first position it perhaps should be a hiragana 3072. */
			 /*  */
			 /*  */
		 case 0x0043: case 0x0063: case 0x004C:
		 case 0x0056: case 0x0076: case 0x0055:
		 case 0x0075:
			 /*CASE_0x0043_ONE: */
			 if (u == UC_LESS_THAN_SIGN || u == UC_LEFT_ANGLE_BRACKET) {
				 if ((bl->unichar[1] == 0x0043) || (bl->unichar[1] == 0x0063)) {
					 if ((bl->dist[1] - bl->dist[0]) < 10) {
						 cjkBestListDiminish(UC_LESS_THAN_SIGN, pSession);
						 cjkBestListDiminish(UC_LEFT_ANGLE_BRACKET, pSession);
					 }
				 }
				 cjkBestListInsertFirst(0x304F, pSession);
				 return;
			 }

			 /*----------------------------------------------------- */
			 /* BEGIN CHUNK: check U V */
			 /*  */
			 /* The latin U and V are not easy to distinguish. */
			 /*  */
			 if (u == 0x0056 || u == 0x0076 || u == 0x0055 || u == 0x0075 ) {
				 if ( (DLTDB_IS_JAPANESE(pSession->db)) && (CJK_STROKE_GET_Y(&s1, 1) <= CJK_STROKE_GET_Y(&s1, 2) &&
					 CJK_STROKE_GET_Y(&s1, -2) <= CJK_STROKE_GET_Y(&s1, -1)) ) {
						 cjkBestListInsertFirst(0x3072, pSession);
						 return;
				 }
				 if ((CJK_STROKE_GET_Y(&s1, 1) > CJK_STROKE_GET_Y(&s1, -1) - 3) &&
					 (CJK_STROKE_GET_Y(&s1, 1) <= CJK_STROKE_GET_Y(&s1, 2)) &&
					 (CJK_STROKE_GET_Y(&s1, -1) <= CJK_STROKE_GET_Y(&s1, -2))) {
						 DECUMA_INT32 i, pos = 0;
						 CJK_GRIDPOINT maxygp;
						 const CJK_GRIDPOINT * agp;
						 DECUMA_INT32 beforegpx, aftergpx, firstgpx, lastgpx;
						 maxygp = cjkStrokeGetMaxYGridpoint(&s1);
						 for (i = 1; i <= CJK_STROKE_NPOINTS(&s1); i++) {
							 agp = cjkStrokeGetGridpoint(&s1, i);
							 if (*agp == maxygp) {
								 pos = i;
							 }
						 }
						 if ((pos != 0) && (pos <= CJK_STROKE_NPOINTS(&s1) - 2) && (pos > 2)) {
							 beforegpx = CJK_GP_GET_X(*cjkStrokeGetGridpoint(&s1, pos - 2));
							 aftergpx = CJK_GP_GET_X(*cjkStrokeGetGridpoint(&s1, pos + 2));
							 firstgpx = CJK_GP_GET_X(*cjkStrokeGetGridpoint(&s1, 1));
							 lastgpx = CJK_GP_GET_X(*cjkStrokeGetGridpoint(&s1, -1));

							 /*printf("+++ %d %d\n", */
							 /*CJK_STROKE_GET_X(&s1, pos + 2) - CJK_STROKE_GET_X(&s1, pos - 2), */
							 /*CJK_STROKE_GET_Y(&s1, pos + 2) + CJK_STROKE_GET_Y(&s1, pos - 2) - 2 * CJK_STROKE_GET_Y(&s1, pos)); */

							 if (ABS(beforegpx - aftergpx) * 3 < ABS(firstgpx - lastgpx) * 2) {
								 cjkBestListInsertFirst(0x0056, pSession);
							 }
							 else {
								 cjkBestListInsertFirst(0x0055, pSession);
							 }
						 }
						 return;
				 }
			 }
			 /* END CHUNK: check U V */
			 /*----------------------------------------------------- */


			 if (cjkContextHasPrevious(con) && u != 0x3093 && u != 0x304F && u != 0x30E0) {
				 if (decumaIsKatakanaAnySize(cjkContextGetPrevious(con))) {
					 cjkBestListInsertFirst(0x30EC, pSession);
					 return;
				 }
				 else if (decumaIsLatin(cjkContextGetPrevious(con))) {
					 cjkBestListDiminish(0x30EC, pSession);
					 cjkBestListDiminish(0x3057, pSession);
					 cjkBestListDiminish(0x30EC, pSession);
					 return;
				 }
			 }
			 {

				 /*----------------------------------------------------- */
				 /* BEGIN CHUNK: check 3057 hiragana */
				 /*  */
				 /*  */
				 DECUMA_INT32 i;
				 DECUMA_INT32 yvalley = 0;
				 DECUMA_INT32 ymaxup = 0;
				 DECUMA_INT32 hashill = 0;
				 DECUMA_INT32 enddir = CJK_STROKE_GET_Y(&s1, -1) - CJK_STROKE_GET_Y(&s1, -2);
				 CJK_GRIDPOINT leftgp = cjkStrokeGetMinXGridpoint(&s1);
				 for (i = 3; i < CJK_STROKE_NPOINTS(&s1); i++) {
					 DECUMA_INT32 ya = CJK_STROKE_GET_Y(&s1, i - 1);
					 DECUMA_INT32 yb = CJK_STROKE_GET_Y(&s1, i);
					 if (yb > yvalley || yb > ya) {
						 yvalley = yb;
					 }
					 else if (yvalley - yb > ymaxup) {
						 ymaxup = yvalley - yb;
					 }
					 if (yb > ya && ymaxup > 0) {
						 hashill = 1;
					 }
				 }
				 if (!hashill && enddir >= 0 &&
					 CJK_STROKE_GET_Y(&s1, -1) > CJK_GP_GET_Y(leftgp) &&
					 CJK_STROKE_GET_X(&s1, 1) >= (CJK_STROKE_GET_X(&s1, -1) + CJK_GP_GET_X(leftgp)) / 2) {
						 cjkBestListInsertFirst(0x304F, pSession);
						 return;
				 }
				 if (hashill && enddir <= 0) {
					 cjkBestListInsertFirst(0x3093, pSession);
					 return;
				 }
				 if (!hashill && enddir <= 1) {
					 /* Use context if possible */
					 if (cjkContextHasPrevious(con) && decumaIsKatakanaAnySize(cjkContextGetPrevious(con))) {
						 cjkBestListInsertFirst(0x30EC, pSession);
					 }
					 else if (cjkContextHasPrevious(con) && decumaIsHiraganaAnySize(cjkContextGetPrevious(con))) {
						 cjkBestListInsertFirst(0x3057, pSession);
					 }
					 else {
						 /* Distinguish between U30EC and U3057 by looking at the smallest angle in the middle of the stroke */
						 int cosine_a;
						 CJK_ZOOMSTROKE s;
						 
						 zm_create_zoom_stroke(c, 0, &s);
						 zm_remove_edge_noise(&s);
						 cosine_a = zm_cosine_of_smallest_angle(&s);

						 if (cosine_a >= -250 && cosine_a != MAX_DECUMA_INT32) {
							 int pos = cjkBestListGetPosition(0x30EC, pSession);
							 if (pos >= 0 || pos <= 2) {
								 /* The katakana should be among top 3 candidates */
								 cjkBestListInsertFirst(0x30EC, pSession);
							 }
						 }
						 else {
							 cjkBestListInsertFirst(0x3057, pSession);
						 }
					 }
					 return;
				 }
				 if (hashill && enddir >= 2) {
					 cjkBestListInsertFirst(0x30E0, pSession);
					 return;
				 }
				 /* END CHUNK: check 3057 hiragana */
				 /*----------------------------------------------------- */

			 }
			 return;
		 case 0x3093: case 0x304F: case 0x30E0:
		 case 0x3057: case 0x30EC:
			 if (DLTDB_IS_JAPANESE(pSession->db)) {
				 if (u == UC_LESS_THAN_SIGN || u == UC_LEFT_ANGLE_BRACKET) {
					 if ((bl->unichar[1] == 0x0043) || (bl->unichar[1] == 0x0063)) {
						 if ((bl->dist[1] - bl->dist[0]) < 10) {
							 cjkBestListDiminish(UC_LESS_THAN_SIGN, pSession);
							 cjkBestListDiminish(UC_LEFT_ANGLE_BRACKET, pSession);
						 }
					 }
					 cjkBestListInsertFirst(0x304F, pSession);
					 return;
				 }

				 /*----------------------------------------------------- */
				 /* BEGIN CHUNK: check U V */
				 /*  */
				 /* The latin U and V are not easy to distinguish. */
				 /*  */
				 if (u == 0x0056 || u == 0x0076 || u == 0x0055 || u == 0x0075 ) {
					 if ( (DLTDB_IS_JAPANESE(pSession->db)) && (CJK_STROKE_GET_Y(&s1, 1) <= CJK_STROKE_GET_Y(&s1, 2) &&
						 CJK_STROKE_GET_Y(&s1, -2) <= CJK_STROKE_GET_Y(&s1, -1)) ) {
							 cjkBestListInsertFirst(0x3072, pSession);
							 return;
					 }
					 if ((CJK_STROKE_GET_Y(&s1, 1) > CJK_STROKE_GET_Y(&s1, -1) - 3) &&
						 (CJK_STROKE_GET_Y(&s1, 1) <= CJK_STROKE_GET_Y(&s1, 2)) &&
						 (CJK_STROKE_GET_Y(&s1, -1) <= CJK_STROKE_GET_Y(&s1, -2))) {
							 DECUMA_INT32 i, pos = 0;
							 CJK_GRIDPOINT maxygp;
							 const CJK_GRIDPOINT * agp;
							 DECUMA_INT32 beforegpx, aftergpx, firstgpx, lastgpx;
							 maxygp = cjkStrokeGetMaxYGridpoint(&s1);
							 for (i = 1; i <= CJK_STROKE_NPOINTS(&s1); i++) {
								 agp = cjkStrokeGetGridpoint(&s1, i);
								 if (*agp == maxygp) {
									 pos = i;
								 }
							 }
							 if ((pos != 0) && (pos <= CJK_STROKE_NPOINTS(&s1) - 2) && (pos > 2)) {
								 beforegpx = CJK_GP_GET_X(*cjkStrokeGetGridpoint(&s1, pos - 2));
								 aftergpx = CJK_GP_GET_X(*cjkStrokeGetGridpoint(&s1, pos + 2));
								 firstgpx = CJK_GP_GET_X(*cjkStrokeGetGridpoint(&s1, 1));
								 lastgpx = CJK_GP_GET_X(*cjkStrokeGetGridpoint(&s1, -1));

								 /*printf("+++ %d %d\n", */
								 /*CJK_STROKE_GET_X(&s1, pos + 2) - CJK_STROKE_GET_X(&s1, pos - 2), */
								 /*CJK_STROKE_GET_Y(&s1, pos + 2) + CJK_STROKE_GET_Y(&s1, pos - 2) - 2 * CJK_STROKE_GET_Y(&s1, pos)); */

								 if (ABS(beforegpx - aftergpx) * 3 < ABS(firstgpx - lastgpx) * 2) {
									 cjkBestListInsertFirst(0x0056, pSession);
								 }
								 else {
									 cjkBestListInsertFirst(0x0055, pSession);
								 }
							 }
							 return;
					 }
				 }
				 /* END CHUNK: check U V */
				 /*----------------------------------------------------- */


				 if (u != 0x3093 && u != 0x304F && u != 0x30E0) {
					 if (cjkContextHasPrevious(con) ) {
						 if (decumaIsKatakanaAnySize(cjkContextGetPrevious(con))) {
							 cjkBestListInsertFirst(0x30EC, pSession);
							 return;
						 }
						 else if (decumaIsLatin(cjkContextGetPrevious(con))) {
							 cjkBestListDiminish(0x30EC, pSession);
							 cjkBestListDiminish(0x3057, pSession);
							 cjkBestListDiminish(0x30EC, pSession);
							 return;
						 }
					 }
				 }
				 {

					 /*----------------------------------------------------- */
					 /* BEGIN CHUNK: check 3057 hiragana */
					 /*  */
					 /*  */
					 DECUMA_INT32 i;
					 DECUMA_INT32 yvalley = 0;
					 DECUMA_INT32 ymaxup = 0;
					 DECUMA_INT32 hashill = 0;
					 DECUMA_INT32 enddir = CJK_STROKE_GET_Y(&s1, -1) - CJK_STROKE_GET_Y(&s1, -2);
					 CJK_GRIDPOINT leftgp = cjkStrokeGetMinXGridpoint(&s1);
					 for (i = 3; i < CJK_STROKE_NPOINTS(&s1); i++) {
						 DECUMA_INT32 ya = CJK_STROKE_GET_Y(&s1, i - 1);
						 DECUMA_INT32 yb = CJK_STROKE_GET_Y(&s1, i);
						 if (yb > yvalley || yb > ya) {
							 yvalley = yb;
						 }
						 else if (yvalley - yb > ymaxup) {
							 ymaxup = yvalley - yb;
						 }
						 if (yb > ya && ymaxup > 0) {
							 hashill = 1;
						 }
					 }
					 if (!hashill && enddir >= 0 &&
						 CJK_STROKE_GET_Y(&s1, -1) > CJK_GP_GET_Y(leftgp) &&
						 CJK_STROKE_GET_X(&s1, 1) >= (CJK_STROKE_GET_X(&s1, -1) + CJK_GP_GET_X(leftgp)) / 2) {
							 cjkBestListInsertFirst(0x304F, pSession);
							 return;
					 }
					 if (hashill && enddir <= 0) {
						 cjkBestListInsertFirst(0x3093, pSession);
						 return;
					 }
					 if (!hashill && enddir <= 1) {
						 /* Use context if possible */
						 if (cjkContextHasPrevious(con) && decumaIsKatakanaAnySize(cjkContextGetPrevious(con))) {
							 cjkBestListInsertFirst(0x30EC, pSession);
						 }
						 else if (cjkContextHasPrevious(con) && decumaIsHiraganaAnySize(cjkContextGetPrevious(con))) {
							 cjkBestListInsertFirst(0x3057, pSession);
						 }
						 else {
							 /* Distinguish between U30EC and U3057 by looking at the smallest angle in the middle of the stroke */
							 int cosine_a;
							 CJK_ZOOMSTROKE s;

							 zm_create_zoom_stroke(c, 0, &s);
							 zm_remove_edge_noise(&s);
							 cosine_a = zm_cosine_of_smallest_angle(&s);

							 if (cosine_a >= -250 && cosine_a != MAX_DECUMA_INT32) {
								 int pos = cjkBestListGetPosition(0x30EC, pSession);
								 if (pos >= 0 || pos <= 2) {
									 /* The katakana should be among top 3 candidates */
									 cjkBestListInsertFirst(0x30EC, pSession);
								 }
							 }
							 else {
								 cjkBestListInsertFirst(0x3057, pSession);
							 }
						 }
						 return;
					 }
					 if (hashill && enddir >= 2) {
						 cjkBestListInsertFirst(0x30E0, pSession);
						 return;
					 }
					 /* END CHUNK: check 3057 hiragana */
					 /*----------------------------------------------------- */

				 }
			 }
			 return;
			 /*  */
			 /*case 0x0031:
			 case 0x0049:
			 case 0x006C:
			 case 0x30CE:
			 case UC_COMMA:
			 case UC_IDEOGRAPHIC_COMMA:
			 case UC_APOSTROPHE:
			 case UC_SOLIDUS:
			 case UC_REVERSE_SOLIDUS:
			 case UC_VERTICAL_LINE:
			 case UC_LEFT_PARENTHESIS:
			 case UC_RIGHT_PARENTHESIS:
			 case UC_LEFT_SQUARE_BRACKET:
			 case UC_RIGHT_SQUARE_BRACKET:
			 case UC_LEFT_CURLY_BRACKET:
			 case UC_RIGHT_CURLY_BRACKET:
			 case UC_LEFT_ANGLE_BRACKET:
			 case UC_RIGHT_ANGLE_BRACKET:
			 case UC_GREATER_THAN_SIGN:
			 case UC_LESS_THAN_SIGN:
			 CASE_BIG_PUNCTUATION:
			 case 0x4E28: // Component vertical stroke
			 case 0x4E36: // Component dot
			 case 0x4E3F: // Component vertical slant ending (like J)
			 {

			 //-----------------------------------------------------
			 // BEGIN CHUNK: set all ell properties
			 // 
			 // Note that the code will work even without box info since the
			 // box values are all 0 in that case.
			 // 
			 DECUMA_INT32 y1 = CJK_STROKE_GET_Y(&s1, 1);
			 DECUMA_INT32 y2 = CJK_STROKE_GET_Y(&s1, 2);
			 DECUMA_INT32 ye1 = CJK_STROKE_GET_Y(&s1, -1);
			 DECUMA_INT32 ye2 = CJK_STROKE_GET_Y(&s1, -2);
			 DECUMA_INT32 x1 = CJK_STROKE_GET_X(&s1, 1);
			 DECUMA_INT32 x2 = CJK_STROKE_GET_X(&s1, 2);
			 DECUMA_INT32 xe1 = CJK_STROKE_GET_X(&s1, -1);
			 DECUMA_INT32 xe2 = CJK_STROKE_GET_X(&s1, -2);
			 DECUMA_INT32 xmin = CJK_GP_GET_X(cjkStrokeGetMinXGridpoint(&s1));
			 DECUMA_INT32 xmax = CJK_GP_GET_X(cjkStrokeGetMaxXGridpoint(&s1));
			 DECUMA_INT32 ymin = CJK_GP_GET_Y(cjkStrokeGetMinYGridpoint(&s1));
			 DECUMA_INT32 ymax = CJK_GP_GET_Y(cjkStrokeGetMaxYGridpoint(&s1));
			 DECUMA_INT32 box_up = pSession->boxybase + pSession->boxheight / 6;
			 DECUMA_INT32 box_down = pSession->boxybase + (5 * pSession->boxheight) / 6;
			 DECUMA_INT32 big = (dltCCCompressGetYmin(c) <= box_up && dltCCCompressGetYmax(c) >= box_down);
			 DECUMA_INT32 formfactor = (10 * (dltCCCompressGetYmax(c) - dltCCCompressGetYmin(c))) / (dltCCCompressGetXmax(c) - dltCCCompressGetXmin(c) + 1);
			 DECUMA_INT32 diagonal_rl = (y1 == ymin && x1 == xmax && ye1 == ymax && xe1 == xmin);
			 DECUMA_INT32 diagonal_lr = (y1 == ymin && x1 == xmin && ye1 == ymax && xe1 == xmax);
			 DECUMA_INT32 dir_start_NE_weakly = (y2 <= y1 && x2 >= x1);
			 DECUMA_INT32 dir_end_E_weakly = (xe1 > xe2 && ABS(xe1 - xe2) >= ABS(ye1 - ye2));
			 DECUMA_INT32 dir_end_W_strongly = (xe1 < xe2 && ABS(xe1 - xe2) > ABS(ye1 - ye2));
			 DECUMA_INT32 spike_left = 0;
			 DECUMA_INT32 spike_right = 0;
			 {
			 DECUMA_INT32 k, n;
			 Gpshort * pxy;
			 decumaAssert(c->pOriginalStrokes != 0);
			 n = c->pOriginalStrokes[0].n;
			 pxy = c->pOriginalStrokes[0].pxy;
			 for (k = n-5; k >= 4; k--) {
			 DECUMA_INT32 x1 = pxy[k - 2].x;
			 DECUMA_INT32 y1 = pxy[k - 2].y;
			 DECUMA_INT32 x2 = pxy[k].x;
			 DECUMA_INT32 y2 = pxy[k].y;
			 DECUMA_INT32 x3 = pxy[k + 2].x;
			 DECUMA_INT32 y3 = pxy[k + 2].y;
			 DECUMA_INT32 dy12 = ABS(y2 - y1) + 1;
			 DECUMA_INT32 dy23 = ABS(y3 - y2) + 1;
			 spike_left =
			 MAX(spike_left, MIN((10*(x1-x2+1))/dy12, (10*(x3-x2+1))/dy23));
			 spike_right =
			 MAX(spike_right, MIN((10*(x2-x1+1))/dy12, (10*(x2-x3+1))/dy23));
			 }
			 }
			 //printf(">>>>>>>>>> %d %d %d %d\n",
			 //dltCCCompressGetYmin(c), pSession->boxybase, dltCCCompressGetYmax(c), pSession->boxybase + pSession->boxheight);
			 // END CHUNK: set all ell properties
			 //-----------------------------------------------------


			 if (!big) {
			 if (u == UC_LESS_THAN_SIGN ||
			 u == UC_LEFT_ANGLE_BRACKET) {
			 goto CASE_0x0043_ONE;
			 }
			 }
			 cjkBestListInsertMany(uclist_ell_etc, pSession);
			 if (y1 > 8 && ye1 > 8) {
			 goto insert_ell;
			 }
			 else if (formfactor < 28 && diagonal_rl) {
			 cjkBestListInsertFirst(UC_SOLIDUS, pSession);
			 if ((DLTDB_IS_JAPANESE(pSession->db)) && !(big && (pSession->sessionCategories & CJK_JIS0)) ) {
			 cjkBestListInsertFirst(0x30CE, pSession);
			 }
			 }
			 else if (formfactor < 28 && diagonal_lr) {
			 #if defined(CERTIFICATION)
			 if (big) {
			 cjkBestListInsertFirst(UC_REVERSE_SOLIDUS, pSession);
			 }
			 else {
			 // insert back for certification, e g 0x4E28
			 cjkBestListInsertFirst(u, pSession);
			 }
			 #else
			 cjkBestListInsertFirst(UC_REVERSE_SOLIDUS, pSession);
			 #endif
			 }
			 else if (big) {

			 //-----------------------------------------------------
			 // BEGIN CHUNK: analyze big
			 // 
			 //printf("\nspikelr: %d %d \n", spike_left, spike_right);
			 if (x1 < x2 && xe2 > xe1) {
			 if (spike_left >= 7) {
			 goto CASE_0x0033;
			 }
			 cjkBestListInsertMany(uclist_bracket_right, pSession);
			 if (spike_right > 10 && formfactor >= 20) {
			 cjkBestListInsertFirst(UC_RIGHT_CURLY_BRACKET, pSession);
			 }
			 else if (ABS(y2 - y1) < x2 - x1 &&
			 ABS(ye1 - ye2) < xe2 - xe1 && formfactor >= 20) {
			 cjkBestListInsertFirst(UC_RIGHT_SQUARE_BRACKET, pSession);
			 }
			 else if (formfactor < 15) {
			 cjkBestListInsertFirst(UC_GREATER_THAN_SIGN, pSession);
			 }
			 else if (formfactor < 23 || (formfactor < 40 && spike_right >= 4)) {
			 cjkBestListInsertFirst(UC_RIGHT_ANGLE_BRACKET, pSession);
			 }
			 else if (formfactor > 100) {
			 cjkBestListInsertFirst(UC_VERTICAL_LINE, pSession);
			 }
			 // else: parenthesis is already in first place
			 }
			 else if (x1 > x2 && xe2 < xe1) {
			 cjkBestListInsertMany(uclist_bracket_left, pSession);
			 if (spike_left > 10 && formfactor >= 20) {
			 cjkBestListInsertFirst(UC_LEFT_CURLY_BRACKET, pSession);
			 }
			 else if (ABS(y2 - y1) < x1 - x2 &&
			 ABS(ye2 - ye1) < x1 - x2 && formfactor >= 20) {
			 cjkBestListInsertFirst(UC_LEFT_SQUARE_BRACKET, pSession);
			 }
			 else if (formfactor < 15) {
			 cjkBestListInsertFirst(UC_LESS_THAN_SIGN, pSession);
			 }
			 else if (formfactor < 23 || (formfactor < 40 && spike_left >= 4)) {
			 cjkBestListInsertFirst(UC_LEFT_ANGLE_BRACKET, pSession);
			 }
			 else if (formfactor > 100) {
			 cjkBestListInsertFirst(UC_VERTICAL_LINE, pSession);
			 }
			 // else: parenthesis is already in first place
			 }
			 else if (diagonal_rl || diagonal_lr || formfactor >= 40) {
			 cjkBestListInsertFirst(UC_VERTICAL_LINE, pSession);
			 }
			 else {
			 // Don't know. Put back original on first place!
			 cjkBestListInsertFirst(u, pSession);
			 }
			 // END CHUNK: analyze big
			 //-----------------------------------------------------


			 }
			 else {

			 //-----------------------------------------------------
			 // BEGIN CHUNK: analyze standard
			 // 
			 #if defined(CERTIFICATION)
			 if (pSession->boxheight == 0) {
			 // Put back original on first place!
			 cjkBestListInsertFirst(u, pSession);
			 return;
			 }
			 #endif
			 if (dir_start_NE_weakly) {
			 if (dir_end_E_weakly) {
			 insert_ell:
			 cjkBestListInsertFirst(0x006C, pSession); // ell
			 }
			 else if (dir_end_W_strongly) {
			 goto CASE_0x4E87_ONE;
			 }
			 else {
			 insert_one:
			 cjkBestListInsertFirst(0x0031, pSession); // one
			 }
			 }
			 else if (diagonal_rl || diagonal_lr || formfactor >= 80) {

			 //-----------------------------------------------------
			 // BEGIN CHUNK: interpret standard vertical line
			 // 
			 // 
			 // When the user writes a vertical line we interpret it differently
			 // depending on language and context.
			 // 
			 #if defined(CERTIFICATION)
			 cjkBestListInsertFirst(0x4E28, pSession);
			 #endif
			 if (decumaIsLatin(cjkContextGetPrevious(con))) {
			 goto insert_ell;
			 }
			 else if (decumaIsDigit(cjkContextGetPrevious(con))) {
			 goto insert_one;
			 }
			 else {
			 if (DLTDB_IS_JAPANESE(pSession->db)) {
			 cjkBestListInsertFirst(0x30CE, pSession);
			 }
			 else {
			 goto insert_one;
			 }
			 }
			 // END CHUNK: interpret standard vertical line
			 //-----------------------------------------------------


			 }
			 else if (y2 - y1 >= 2 * ABS(x2 - x1) && dir_end_E_weakly) {
			 goto insert_ell;
			 }
			 else if ((DLTDB_IS_JAPANESE(pSession->db)) && (categorymask & CJK_HIRAGANA) &&
			 CJK_STROKE_GET_X(&s1, -1) < CJK_STROKE_GET_X(&s1, -3)) {
			 cjkBestListInsertFirst(0x30CE, pSession);
			 }
			 else {
			 // Don't know. Put back original on first place!
			 cjkBestListInsertFirst(u, pSession);
			 }
			 // END CHUNK: analyze standard
			 //-----------------------------------------------------


			 }
			 }
			 return;*/
			 /*  */
		 case 0x0032:
		 case 0x005A:
		 case 0x007A:
		 case 0x3066:
		 case 0x30A8:
		 case 0x30E6:
		 case 0x30B3:
		 case 0x4E59:
		 case 0x53C8:
			 if (pSession->db.categoryMask & (CJK_GB2312_A | CJK_BIGFIVE)) {
				 DECUMA_INT32 x, y, x1, x2, y1, nintersec;
				 CJK_GRIDPOINT g;
				 nintersec = dltCCharGetIntersectionCount(c, pSession);
				 x = CJK_STROKE_GET_X(&s1, -1) - CJK_STROKE_GET_X(&s1, -2);
				 y = CJK_STROKE_GET_Y(&s1, -1) - CJK_STROKE_GET_Y(&s1, -2);
				 if (nintersec >= 1 && y > 0) {
					 cjkBestListInsertMany(uclist_53C8, pSession);
					 return;
				 }
				 if (ABS(y) > ABS(x)) {
					 cjkBestListInsertMany(uclist_4E59, pSession);
					 return;
				 }
				 if ((dltCCCompressGetYmax(c) - dltCCCompressGetYmin(c)) * 3 <= pSession->boxheight) {
					 cjkBestListInsertMany(uclist_007A, pSession); /* 'z' */
					 return;
				 }
				 y1 = CJK_STROKE_GET_Y(&s1, 1);
				 g = cjkStrokeGetMinYGridpoint(&s1);
				 x1 = CJK_STROKE_GET_X(&s1, 1);
				 x2 = CJK_STROKE_GET_X(&s1, 2);
				 if (nintersec != 0 || 2 * (y1 - CJK_GP_GET_Y(g)) > (CJK_GP_GET_X(g) - x1) || x1 >= x2) {
					 cjkBestListInsertMany(uclist_0032, pSession);
					 return;
				 }
				 cjkBestListInsertMany(uclist_007A, pSession);
				 return;
			 }
			 /* Check that it ends rightwards */
			 else if (CJK_STROKE_GET_X(&s1, -1) > CJK_STROKE_GET_X(&s1, -2)) {
				 DECUMA_INT32 xp = 16;
				 DECUMA_INT32 n = CJK_STROKE_NPOINTS(&s1);
				 DECUMA_INT32 i;
				 cjkBestListInsertMany(uclist_3066, pSession);
				 for (i = n / 2; i <= n; i++) {
					 DECUMA_INT32 x = CJK_STROKE_GET_X(&s1, i);
					 if (x < xp) {
						 xp = x;
					 }
				 }
				 if (dltCCharGetIntersectionCount(c, pSession) == 1 ||
					 (xp <= CJK_STROKE_GET_X(&s1, 1) + 1 &&
					 ABS(CJK_STROKE_GET_Y(&s1, -1) - CJK_STROKE_GET_Y(&s1, -2)) <= 1)) {
						 cjkBestListInsertFirst(0x0032, pSession); /* '2' */
				 }
			 }
			 return;
			 /*  */
			 /* An h should perhaps be an n instead. */
			 /* Observe that the raw data may display larger difference */
			 /* in the length of the protruding part differing 'h' from 'n' than */
			 /* compressed data. This is especially true for the more advanced case  */
			 /* when the curve turns upward in the end. */
			 /*  */
		 case 0x0068:
		 case 0x006E:
			 if (CJK_STROKE_GET_Y(&s1, 2) > CJK_STROKE_GET_Y(&s1, 1)) {
				 DECUMA_INT32 i, y1, y2, ymin;
				 y1 = ymin = CJK_STROKE_GET_Y(&s1, 5);
				 for (i = 6; i <= CJK_STROKE_NPOINTS(&s1); i++) {
					 y2 = CJK_STROKE_GET_Y(&s1, i);
					 if (y1 < ymin) {
						 ymin = y1;
						 if (y2 > y1) {
							 break;
						 }
					 }
					 y1 = y2;
				 }
				 if (ymin - CJK_STROKE_GET_Y(&s1, 1) <= 2) {
					 cjkBestListInsertFirst(0x006E, pSession); /* n */
				 }
				 else {
					 cjkBestListInsertFirst(0x0068, pSession); /* h */
				 }
			 }
			 return;
			 /* END CHUNK: specialcheck onestroke */
			 /*----------------------------------------------------- */


		}
	}
	if (nStrokes == 2) {
		if (DLTDB_IS_JAPANESE(pSession->db)) {
			switch (u) {

				/*----------------------------------------------------- */
				/* BEGIN CHUNK: specialcheck twostrokes JAPN */
				/*  */
				/*  */
			case 0x308C:
			case 0x308F:
			case 0x5C71:
			case 0x5C0F:
				if (CJK_STROKE_GET_Y(&s1, 1) < CJK_STROKE_GET_Y(&s2, 1) &&
					CJK_GP_GET_Y(cjkStrokeGetMaxYGridpoint(&s1)) >= CJK_GP_GET_Y(cjkStrokeGetMaxYGridpoint(&s2)) - 2 &&
					CJK_STROKE_GET_Y(&s2, -1) >= CJK_STROKE_GET_Y(&s2, -2)) {
						CJK_GRIDPOINT lastx = CJK_STROKE_GET_X(&s2, -1);
						if (lastx == CJK_GP_GET_X(cjkStrokeGetMaxXGridpoint(&s2)) &&
							lastx != CJK_STROKE_GET_X(&s2, -2)) {

								/*----------------------------------------------------- */
								/* BEGIN CHUNK: loopcheck 306D */
								/*  */
								/* Check if a loop is present at the end of the second stroke. */
								/* If there is then it is 306D. */
								/*  */
								DECUMA_INT32 n = CJK_STROKE_NPOINTS(&s2);
								DECUMA_INT32 i;
								for (i = n; i > n - 6 && i > 6; i--) {
									if (CJK_STROKE_GET_X(&s2, i) <= CJK_STROKE_GET_X(&s2, i-1)) {
										if (CJK_STROKE_GET_Y(&s2, i) < CJK_STROKE_GET_Y(&s2, i-1)) {
											cjkBestListInsertFirst(0x306D, pSession);
											return;
										}
										else {
											break;
										}
									}
								}
								/* END CHUNK: loopcheck 306D */
								/*----------------------------------------------------- */


								cjkBestListInsertMany(uclist_308C, pSession);
						}
						else{
							cjkBestListInsertMany(uclist_308F, pSession);
						}
				}
				return;
				/*  */
				/* Check if there is a loop in the second stroke. In that case it is not a 3061 */
				/* but probably a 3059. */
				/*  */
			case 0x3061:
				{
					DECUMA_INT32 l;
					CJK_ZOOMSTROKE s;

					zm_create_zoom_stroke(c, 1, &s);

					l = zm_get_stroke_length(&s);

					if (zm_go_down(&s) && (20 * zm_go_left(&s) > l) && zm_go_up(&s) && zm_go_down(&s)) {
						cjkBestListInsertFirst(0x3059, pSession);
						return;
					}
				}
				/* Seems like the above is better. TODO remove this?
				if (cjkStrokeGetIntersectionCount(&s2, pSession) > 0) {
				if (cjkBestListGetPosition(0x3059, pSession) == 1 && ((bl->dist[1] - bl->dist[0]) < 8)) {
				cjkBestListInsertFirst(0x3059, pSession);
				return;
				}
				}
				*/
				return;
				/*  */
				/*  */
				/* \subsubsection*{Identical Hiragana Katakana} */
				/*  */
				/* The characters 0x308A (hiragana) is $almost$ identical with katakana 0x30EA. */
				/*  */
			case 0x308A:
			case 0x30EA: {
				DECUMA_INT32 left1 = CJK_STROKE_GET_X(&s1, -1) - CJK_STROKE_GET_X(&s1, 1);
				DECUMA_INT32 right1 = CJK_STROKE_GET_X(&s2, 1) - CJK_STROKE_GET_X(&s1, -1);
				DECUMA_INT32 rightovershoot =
					CJK_GP_GET_X(cjkStrokeGetMaxXGridpoint(&s2)) - CJK_STROKE_GET_X(&s2, 1);
				if (left1 >= 2 + rightovershoot && right1 >= 3 - rightovershoot) {
					cjkBestListInsertFirst(0x30BD, pSession);
				}
				else if (cjkContextHasPrevious(con)) {
					if (decumaIsKatakanaAnySize(cjkContextGetPrevious(con))) {
						cjkBestListInsertFirst(0x30EA, pSession);
					}
					else {
						cjkBestListInsertFirst(0x308A, pSession);
					}
				}
						 }
						 return;
						 /*  */
						 /*  */
						 /*  */
						 /* The katakana character 30CF is confused with hiragana 3044. */
						 /* If the strokes go apart, then it is katakana. If they are parallell */
						 /* or go together we consider it hiragana, unless they go much together, */
						 /* then it is 30BD. */
						 /* If we dont know then we use context. */
						 /* But first of all we check */
						 /* if the last stroke goes (weakly) upwards at the end. Then it is */
						 /* a katakana 30EB. */
						 /*  */
			case 0x516B:
			case 0x3044:
			case 0x30CF: {
				DECUMA_INT32 x1s = CJK_STROKE_GET_X(&s1, 1);
				DECUMA_INT32 x1e = CJK_STROKE_GET_X(&s1, -1);
				DECUMA_INT32 x2s = CJK_STROKE_GET_X(&s2, 1);
				DECUMA_INT32 x2e = CJK_STROKE_GET_X(&s2, -1);
				DECUMA_INT32 trend = (x2e - x1e) - (x2s - x1s);
				if (CJK_STROKE_GET_Y(&s2, -1) - CJK_STROKE_GET_Y(&s2, -2) <= 0) {
					cjkBestListInsertFirst(0x30EB, pSession);
				}
				else if (trend <= 1) {
					if (CJK_STROKE_GET_Y(&s1, -1) - CJK_STROKE_GET_Y(&s1, -2) > 0) {
						if (trend <= -4) {
							cjkBestListInsertFirst(0x30BD, pSession);
							return;
						}
					}
					cjkBestListInsertFirst(0x3044, pSession);
					return;
				}
				else if (trend >= 3) {
					if (decumaIsKatakanaAnySize(cjkContextGetPrevious(con))) {
						cjkBestListInsertFirst(0x30CF, pSession);
					}
					else {
						cjkBestListInsertFirst(0x516B, pSession);
					}
					return;
				}
				else if (cjkContextHasPrevious(con) && decumaIsKatakanaAnySize(cjkContextGetPrevious(con))) {
					cjkBestListInsertFirst(0x30CF, pSession);
					return;
				}
				else {
					cjkBestListInsertFirst(0x3044, pSession);
				}
						 }
						 return;
						 /*  */
						 /*  */
						 /* After careful consideration it (and based on results from casiodata) */
						 /* it was decided that hiragana ku should always appear instead of right */
						 /* angle bracket. This would hence have to be chosen from the candidatelist. */
						 /*  */
			case UC_LEFT_ANGLE_BRACKET: {
				if (categorymask & CJK_HIRAGANA) {
					cjkBestListInsertFirst(0x304F, pSession);
				}
										}
										/*  */
										/* A 30E4 with two or three intersections must be a cursive 3084. */
										/* A turn on the second stroke changes a 30E4 to a 30BB. */
										/*  */
			case 0x30E4:
				if (dltCCharGetIntersectionCount(c, pSession) >= 2) {
					cjkBestListInsertFirst(0x3084, pSession);
				}
				else if (CJK_STROKE_GET_X(&s2, -1) - CJK_STROKE_GET_X(&s2, -2) >
					CJK_STROKE_GET_Y(&s2, -1) - CJK_STROKE_GET_Y(&s2, -2)) {
						cjkBestListInsertFirst(0x30BB, pSession);
				}
				return;
				/*  */
				/*  */
				/* If the vertical stroke 2 goes above stroke 1 on a 30A2 then */
				/* it must be a 30E4 instead. */
				/*  */
			case 0x30A2: {
				DECUMA_INT32 i;
				DECUMA_INT32 y21 = CJK_STROKE_GET_Y(&s2, 1);
				DECUMA_INT32 x21 = CJK_STROKE_GET_X(&s2, 1);
				for (i = 1; i <= CJK_STROKE_NPOINTS(&s1); i++) {
					if (CJK_STROKE_GET_X(&s1, i) >= x21) {
						if (y21 <= CJK_STROKE_GET_Y(&s1, i)) {
							cjkBestListInsertFirst(0x30E4, pSession);
							break;
						}
						else {
							goto CASE_0x30A2_ONE_TWO;
						}
					}
				}
						 }
						 return;
						 /*  */
						 /* A kanji 597D with only two strokes is probably hiragana 306C. */
						 /*  */
			case 0x597D:
				cjkBestListInsertFirst(0x306C, pSession);
				return;
				/*  */
				/* A twostroked hiragana 30B1 or 3051 with two crossings must be a 307F. */
				/*  */
			case 0x3051:
			case 0x30B1:
				if (dltCCharGetIntersectionCount(c, pSession) >= 2) {
					cjkBestListInsertFirst(0x307F, pSession);
				}
				return;
				/*  */
				/*  */
				/* A fourstroked hiragana 30D3 may end up as a 3056 without a check. */
				/*  */
			case 0x3056:
				if (nStrokes == 4) {
					if (CJK_STROKE_GET_X(&s2, 2) - CJK_STROKE_GET_X(&s2, 1) >= 0 &&
						CJK_STROKE_GET_X(&s2, 3) - CJK_STROKE_GET_X(&s2, 2) < 0) {
							cjkBestListInsertFirst(0x30D3, pSession);
					}
				}
				return;
				/*  */
				/*  */
				/* A Y or y with a large gap must be a katakana 30BD. */
				/*  */
			case 0x0059:
			case 0x0079: {
				CJK_GRIDPOINT s1e = *cjkStrokeGetGridpoint(&s1, -1);
				DECUMA_INT32 i;
				for (i = 1; i <= CJK_STROKE_NPOINTS(&s2); i++) {
					if (CJK_GP_GET_SQ_DISTANCE(s1e, *cjkStrokeGetGridpoint(&s2, i)) <= 4) {
						return;
					}
				}
						 }
						 cjkBestListInsertFirst(0x30BD, pSession);
						 return;
						 /*  */
						 /*  */
						 /* For twostroke 4 there is a mixup with 30E0. If there is no intersections, */
						 /* it is for sure not the four. Only in the mixed mode! */
						 /*  */
			case 0x0034:
				if (dltCCharGetIntersectionCount(c, pSession) == 0 &&
					(categorymask & (CJK_HAN | CJK_HIRAGANA | CJK_KATAKANA))){
						cjkBestListDiminish(0x0034, pSession);
				}
				return;
				/* END CHUNK: specialcheck twostrokes JAPN */
				/*----------------------------------------------------- */


			}
		}
		switch (u) {

			/*----------------------------------------------------- */
			/* BEGIN CHUNK: specialcheck twostrokes */
			/*  */
		 case UC_RIGHT_SQUARE_BRACKET:
			 {
				 DECUMA_INT32 box_up = pSession->boxybase + pSession->boxheight / 6;
				 DECUMA_INT32 box_down = pSession->boxybase + (5 * pSession->boxheight) / 6;
				 DECUMA_INT32 big = (dltCCCompressGetYmin(c) <= box_up && dltCCCompressGetYmax(c) >= box_down);
				 if (!big && CJK_STROKE_GET_X(&s2, -1) >= CJK_GP_GET_X(cjkStrokeGetMaxXGridpoint(&s1))) {
					 cjkBestListInsertFirst(0x0049, pSession);
				 }
				 else if (!big) {
					 goto CASE_4E01_TWO;
				 }
			 }
			 return;
			 /*  */
			 /*  */
		 case 0x4E5D:
		 case 0x5200:
		 case 0x529B:
		 case 0x30AB: {
			 DECUMA_INT32 left = 0, up = 0, right = 0;
			 if (CJK_STROKE_NPOINTS(&s1) > 1 && CJK_STROKE_NPOINTS(&s2) > 1) {
				 if (CJK_STROKE_GET_X(&s1, -1) < CJK_STROKE_GET_X(&s2, -1)) {
					 CJK_STROKE tmp;
					 tmp = s1; s1 = s2; s2 = tmp; /* Swich order s1 <-> s2. */
				 }
				 if (dltCCharGetIntersectionCount(c, pSession) == 1) {
					 up = 1;
				 }
				 if ( CJK_STROKE_GET_X(&s1, 1) <  CJK_STROKE_GET_X(&s2, 1)) {
					 left = 1;
				 }
				 if (CJK_STROKE_NPOINTS(&s1) > 2) {
					 DECUMA_INT32 i;
					 for (i = CJK_STROKE_NPOINTS(&s1) / 2; i <= CJK_STROKE_NPOINTS(&s1) - 2; i++) {
						 if (CJK_STROKE_GET_Y(&s1, i+1) > 8 &&
							 CJK_STROKE_GET_X(&s1, i+2) > CJK_STROKE_GET_X(&s1, i+1)) {
								 right = 1;
						 }
					 }
					 if (right && CJK_STROKE_GET_Y(&s1, 2) > CJK_STROKE_GET_Y(&s1, 1) + 1) {
						 /*cjkBestListInsertFirst(0x513F, pSession); */
						 return;
					 }
					 if (right && up) {
						 goto CASE_0x4E5D_TWO;
					 }
					 if (!right){
						 if (decumaIsKatakanaAnySize(cjkContextGetPrevious(con))) {
							 cjkBestListInsertMany(uclist_30AB, pSession);
							 return;
						 }
						 else {
							 if (!up) {
								 cjkBestListInsertMany(uclist_5200, pSession);
								 return;
							 }
							 if (up) {
								 cjkBestListInsertMany(uclist_529B, pSession);
								 return;
							 }
						 }
					 }
					 if (right && !left) {
						 /*cjkBestListInsertFirst(0x51E0, pSession); */
						 return;
					 }
					 if (right){
						 if (!left) {
							 /*cjkBestListInsertFirst(0x51E0, pSession); */
							 return;
						 }
						 else {
							 cjkBestListInsertFirst(0x5165, pSession);
							 return;
						 }
					 }
				 }
			 }
					  }
					  return;
					  /*  */
					  /* If we recognize 51E0 it might be latin R. Also in a big */
					  /* test it is found that the differences between 51E0, 4E5D and */
					  /* 51F3 consist only of the distance between the x-posititons */
					  /* of the first point in the two strokes. A plot shows this to be */
					  /* fairly accurate. A check for capital N is also included. */
					  /*  */
		 case 0x51E0:
		 case 0x513F:
CASE_0x4E5D_TWO: {
			 DECUMA_INT32 dx;
			 DECUMA_INT32 n1 = CJK_STROKE_NPOINTS(&s1);
			 DECUMA_INT32 n2 = CJK_STROKE_NPOINTS(&s2);
			 if (n2 < n1) {
				 CJK_STROKE stmp;
				 int ntmp;
				 stmp = s1; s1 = s2; s2 = stmp; /* Swich order s1 <-> s2. */
				 ntmp = n1; n1 = n2; n2 = ntmp;
			 }
			 if (CJK_STROKE_GET_Y(&s2, -1) >= CJK_STROKE_GET_Y(&s2, -2)) {
				 DECUMA_INT32 i;
				 for (i = 2; i < n2 - 1; i++) {
					 DECUMA_INT32 x1 = CJK_STROKE_GET_X(&s2, i - 1);
					 DECUMA_INT32 x2 = CJK_STROKE_GET_X(&s2, i);
					 DECUMA_INT32 x3 = CJK_STROKE_GET_X(&s2, i + 1);
					 if (x2 < x1 && x3 > x2 && x1 - 2*x2 + x3 >= 3) {
						 cjkBestListInsertFirst(0x0052, pSession); /* R */
						 return;
					 }
				 }
			 }
			 dx = CJK_STROKE_GET_X(&s2, 1) - CJK_STROKE_GET_X(&s1, 1);
			 if (dx <= -2) {
				 cjkBestListInsertFirst(0x4E5D, pSession);
			 }
			 else if (dx <= 1) {
				 int s2y1 = CJK_STROKE_GET_Y(&s2, 1);
				 if (CJK_STROKE_GET_Y(&s2, 2) > s2y1 && CJK_STROKE_GET_Y(&s2, -1) <= s2y1) {
					 cjkBestListInsertFirst(0x004E, pSession); /* 'N' */
				 }
				 else {
					 cjkBestListInsertFirst(0x51E0, pSession);
				 }
			 }
			 else {
				 cjkBestListInsertFirst(0x513F, pSession);
			 }
				 }
				 return;
				 /*  */
				 /*  */
				 /* {\Huge \textbf{D P p b}} */
				 /*  */
		 case 0x0044:
		 case 0x0050:
		 case 0x0070:
		 case 0x0062:
			 if (ABS(CJK_STROKE_GET_Y(&s1,-1) - CJK_STROKE_GET_Y(&s2,-1)) < 3) {
				 if (ABS(CJK_STROKE_GET_Y(&s2, 1) - CJK_STROKE_GET_Y(&s1, 1)) < 3) {

					 /*----------------------------------------------------- */
					 /* BEGIN CHUNK: check for B */
					 /*  */
					 {
						 DECUMA_INT32 i;
						 for (i = 4; i < CJK_STROKE_NPOINTS(&s2) - 3; i++) {
							 DECUMA_INT32 x1 = CJK_STROKE_GET_X(&s2, i - 2);
							 DECUMA_INT32 x2 = CJK_STROKE_GET_X(&s2, i);
							 DECUMA_INT32 x3 = CJK_STROKE_GET_X(&s2, i + 2);
							 if (x3 - 2*x2 + x1 >= 3) {
								 cjkBestListInsertFirst(0x0042, pSession);  /* B */
								 return;
							 }
						 }
					 }
					 /* END CHUNK: check for B */
					 /*----------------------------------------------------- */


					 cjkBestListInsertFirst(0x0044, pSession);  /* D */
				 }
				 else {
					 cjkBestListInsertFirst(0x0062, pSession);  /* b */
				 }
			 }
			 else if (u == 0x0044) {
				 if (ABS(CJK_STROKE_GET_X(&s1,-1) - CJK_STROKE_GET_X(&s2,-1)) <= 3) {
					 cjkBestListInsertFirst(0x0050, pSession);  /* P */
				 }
			 }
			 return;
			 /*  */
			 /* Recognizing an E might mean that the user really wrote a small z. */
			 /*  */
		 case 0x0045:
			 /* The following is copied from the check above to collect all check regardin 0045 into one.
			 This makes it possible to have all checks active at the same time, thus removing need for ifdef. */
			 if (DLTDB_IS_JAPANESE(pSession->db)) {
				 if (nStrokes == 3 &&
					 CJK_STROKE_NPOINTS(&s1) > CJK_STROKE_NPOINTS(&s2) &&
					 CJK_STROKE_NPOINTS(&s1) > CJK_STROKE_NPOINTS(&s3)) {
						 if (CJK_STROKE_GET_Y(&s1, -1) < CJK_STROKE_GET_Y(&s1, -2) &&
							 CJK_STROKE_GET_X(&s1, 1) > CJK_STROKE_GET_X(&s2, 1)) {
								 cjkBestListInsertFirst(0x3082, pSession);
						 }
				 }
				 return;
			 }
			 else if (!DLTDB_IS_SIMP(pSession->db)) { /* Fall through if simplified */
				 return;
			 }

		 case 0x007A:
			 if (DLTDB_IS_SIMP(pSession->db)) {
				 if ((nStrokes == 2) && ((dltCCCompressGetYmax(c) - dltCCCompressGetYmin(c)) * 3 <= pSession->boxheight)) {
					 cjkBestListInsertFirst(0x007A, pSession);
				 }
				 else {
					 cjkBestListInsertFirst(0x0045, pSession);
				 }
			 }
			 return;
			 /* #endif */
			 /** 
			 * 
			 * Latin {\Huge \textbf{ R }} is mixed up with cursive forms of chinese
			 * charcters 53E3 and 53CA.
			 * 
			 * \f{figure}[h!]
			 *  \begin{center}
			 *    \begin{tabular}{cc}
			 *       \epsfig{file=../../figures/53e3.eps,width=50mm,height=50mm} &
			 *       \epsfig{file=../../figures/53ca.eps,width=50mm,height=50mm}
			 *    \end{tabular}
			 *  \end{center}
			 * \f}
			 */
		 case 0x0052:
		 case 0x53CA:
		 case 0x53E3:
			 if (cjkContextHasPrevious(con)) {
				 if (!decumaIsHan(cjkContextGetPrevious(con))) {
					 cjkBestListInsertFirst(0x0052, pSession);
				 }
			 }
			 return;
			 /*  */
			 /*  */
			 /* A cursive square or cursive two strokes? */
			 /*  */
		 case 0x53EF:
		 case 0x4E60:
			 if (CJK_STROKE_GET_X(&s2, 2) - CJK_STROKE_GET_X(&s2, 1) <=
				 CJK_STROKE_GET_Y(&s2, 2) - CJK_STROKE_GET_Y(&s2, 1)) {
					 cjkBestListInsertFirst(0x53EF, pSession); /* vertical start is square */
			 }
			 else {
				 cjkBestListInsertFirst(0x4E60, pSession); /* horizontal start */
			 }
			 return;


			 /**
			 * We have a group of T-looking things: 0054 (T itself), 4E01 (a hook
			 * at the very end), 30A4 (a katakana written the wrong way), 30CA (a
			 * katakana). Note that the code operating on original (not subsampled)
			 * data is done so that it works on the chinese certification database,
			 * which has small upwards endings at penlift.
			 * 
			 * \f{figure}[ht!]
			 *  \begin{center}
			 *    \begin{tabular}{ccc}
			 *       {\Huge \textbf{T \ }} and &
			 *       \epsfig{file=../../figures/4e01.eps,width=50mm,height=50mm}
			 *    \end{tabular}
			 *  \end{center}
			 * \f}
			 */
		 case 0x30A4:
		 case 0x30CA:
			 if (DLTDB_IS_JAPANESE(pSession->db)) {
				 CJK_ZOOMSTROKE s;

				 zm_create_zoom_stroke(c, 1, &s);

				 if (zm_go_down(&s) && zm_go_up(&s) && zm_go_down(&s)) {
					 cjkBestListInsertFirst(0x3059, pSession);
					 return;
				 }

				 if (CJK_STROKE_GET_X(&s1, 3) < CJK_STROKE_GET_X(&s1, 1)) {
					 cjkBestListInsertFirst(0x30A4, pSession);
					 return;
				 }
			 }

		 case 0x0054:
		 case 0x4E01:
CASE_4E01_TWO:
			 {
				 DECUMA_INT32 s2above, s2left, s2hook;
				 DECUMA_INT32 k = 1;

				 DECUMA_INT32   nPoints;
				 const DECUMA_POINT * pPoints;
				 DECUMA_ARC * pOriginalStrokes = dltCCCompressGetOrgStrokes(c);

				 if (CJK_STROKE_GET_Y(&s1, -1) - CJK_STROKE_GET_Y(&s1, 1)  >
					 CJK_STROKE_GET_Y(&s2, -1) - CJK_STROKE_GET_Y(&s2, 1)) 
				 {
					 /* flip to horizontal stroke first */
					 s3 = s1; s1 = s2; s2 = s3;
					 k = 0;
				 }

				 s2above = CJK_STROKE_GET_Y(&s1,  1) + CJK_STROKE_GET_Y(&s1, -1) - 2 * CJK_STROKE_GET_Y(&s2, 1) >= 2;
				 s2left  = CJK_STROKE_GET_X(&s2, -3) - CJK_STROKE_GET_X(&s2, -1)                       >= 2;

				 decumaAssert(pOriginalStrokes != 0);

				 /* Retrieving original data from second stroke !! Seems shaky ... /JS */
				 nPoints = pOriginalStrokes[1].nPoints;
				 pPoints = (const DECUMA_POINT *) pOriginalStrokes[1].pPoints;

				 s2hook  = CJK_STROKE_GET_Y(&s2, -1) <= CJK_STROKE_GET_Y(&s2, -2) && s2left;

				 if (!s2hook) {
					 DECUMA_INT32 x_end = pPoints[nPoints - 1].x;
					 DECUMA_INT32 y_end = pPoints[nPoints - 1].y;
					 for (k = nPoints - 2; k >= 2; k--) {
						 DECUMA_INT32 x = pPoints[k].x;
						 DECUMA_INT32 y = pPoints[k].y;
						 if (x - x_end >= 4 && (y_end - y) < (x - x_end)) {
							 s2hook = 1;
						 }
					 }
				 }
				 if (!s2above && s2hook) {
					 goto CASE_0x4E01_AGAINST_004A;
				 }
				 else if ( (DLTDB_IS_JAPANESE(pSession->db)) && (s2above || s2left) ){
					 goto CASE_0x30CA;
				 }
				 else if ( (DLTDB_IS_JAPANESE(pSession->db)) && (2 * (CJK_STROKE_GET_Y(&s1, 1) - CJK_STROKE_GET_Y(&s1, -1)) >
					 CJK_STROKE_GET_X(&s1, -1) - CJK_STROKE_GET_X(&s1, 1))  ) {
						 cjkBestListInsertFirst(0x30A4, pSession);
				 }
				 else {
					 cjkBestListInsertFirst(0x0054, pSession);
				 }
			 }
			 return;
			 /*  */
		 case UC_QUOTATION_MARK:
			 if (cjkContextGetDoubleQuoteCount(con)) {
				 cjkBestListInsertMany(uclist_RDQfix, pSession);
			 }
			 else {
				 cjkBestListInsertMany(uclist_LDQfix, pSession);
			 }
			 return;
			 /*  */
		 case UC_LEFT_DOUBLE_QUOTATION_MARK:
		 case UC_RIGHT_DOUBLE_QUOTATION_MARK:
			 if (decumaIsKatakanaAnySize(cjkContextGetPrevious(con))) {
				 cjkBestListInsertFirst(UC_DIACRITIC_DAKUTEN, pSession);
			 }
			 return;
			 /**
			 * 
			 * \f{figure}[ht!]
			 *  \begin{center}
			 *   \epsfig{file=../../figures/4e03.eps,width=50mm,height=50mm}
			 *  \end{center}
			 * \f}
			 * 
			 */
		 case 0x4E03:
		 case 0x5315: {
			 CJK_STROKE s_horiz;
			 CJK_STROKE s_other;
			 if (ABS(CJK_GP_GET_Y(cjkStrokeGetMinYGridpoint(&s1)) - CJK_GP_GET_Y(cjkStrokeGetMaxYGridpoint(&s1))) <
				 ABS(CJK_GP_GET_Y(cjkStrokeGetMinYGridpoint(&s2)) - CJK_GP_GET_Y(cjkStrokeGetMaxYGridpoint(&s2)))) {
					 s_horiz = s1;
					 s_other = s2;
			 }
			 else {
				 s_horiz = s2;
				 s_other = s1;
			 }
			 if (CJK_GP_GET_X(cjkStrokeGetMinXGridpoint(&s_horiz)) <
				 CJK_GP_GET_X(cjkStrokeGetMinXGridpoint(&s_other)) - 1) {
					 cjkBestListInsertFirst(0x4E03, pSession);
			 }
			 else {
				 if (categorymask & CJK_KATAKANA) {
					 cjkBestListInsertFirst(0x30D2, pSession);
					 goto CASE_0x30D2_TWO;
				 }
				 else {
					 cjkBestListInsertFirst(0x5315, pSession);
				 }
			 }
					  }
					  return;
					  /*  */
					  /*  */
					  /* The katakana character 30E1 is easily confused with the latin x: 0078, 0058. */
					  /* This conflict is solved with three areas: sure katakana, sure latin or */
					  /* else: context. The stroke order is random so we fix this first. */
					  /*  */
		 case 0x0058:
		 case 0x0078:
		 case 0x30E1: {
			 DECUMA_INT32 yheightA = CJK_STROKE_GET_Y(&s1, -1) - CJK_STROKE_GET_Y(&s1, 1);
			 DECUMA_INT32 yheightB = CJK_STROKE_GET_Y(&s2, -1) - CJK_STROKE_GET_Y(&s2, 1);
			 if (CJK_STROKE_GET_X(&s1, 1) > CJK_STROKE_GET_X(&s2, 1)) {
				 DECUMA_INT32 tmp = yheightA;
				 yheightA = yheightB;
				 yheightB = tmp;
			 }
			 if (yheightB - yheightA <= 2) {
				 goto insert0058;
			 }
			 else if (yheightB - yheightA >= 5) {
				 goto insert30E1;
			 }
			 if (decumaIsLatin(cjkContextGetPrevious(con))) {
insert0058:
				 cjkBestListInsertFirst(0x0058, pSession); /* X */
				 cjkBestListInsertFirst(0x0078, pSession); /* x */
			 }
			 else {
insert30E1:
				 cjkBestListInsertFirst(0x30E1, pSession);
			 }
					  }
					  return;
					  /*  */
					  /* If an i or a j is on the first place -- or possibly an ':' or ';' -- */
					  /* it might be that the user meant hiragana 3046 or katakana 30E9. */
					  /* Note that we are only interested if the upper dot is written first. */
					  /* A sure way to get an i or j is to write the dot last, as most people do. */
					  /* The problem is that a substantial fraction of japanese people */
					  /* write is first. Hiragana and katakana dot is always written first. */
					  /* If the second stroke starts (weakly) upwards, or if the */
					  /* second stroke is enough wide and the first point is also the leftmost point, */
					  /* then it must be hiragana/katakana. */
					  /* The code right now only cares about hiragana/katakana. It needs polishing */
					  /* on i and j and the colons. */
					  /*  */
		 case 0x006A: 
		 case 0x0069:


			 /*----------------------------------------------------- */
			 /* BEGIN CHUNK: check chinese i, j */
			 /*  */
			 /*  */
			 /*  */
			 /* Check for SEMC chinese latin: 'i' with serifs top and bottom are  */
			 /* sometimes interpreted as 'j', 'j' with serif at the top is sometimes  */
			 /* interpreted as 'i'. */
			 /*  */
			 if (DLTDB_IS_SIMP(pSession->db) || DLTDB_IS_TRAD(pSession->db)) {
				 CJK_ZOOMSTROKE zs1, zs2, * zs;
				 int l1, l2;

				 zm_create_zoom_stroke(c, 0, &zs1);
				 zm_create_zoom_stroke(c, 1, &zs2);

				 l1 = zm_get_stroke_length(&zs1);
				 l2 = zm_get_stroke_length(&zs2);

				 zs = l1 > l2 ? &zs1 : &zs2;

				 if (bl->unichar[1] == 0x0069) {

					 if (zm_go_ne(zs) && zm_go_down(zs) && zm_go_ne(zs)) {
						 cjkBestListInsertFirst(0x0069, pSession);
						 return;
					 }
				 }
				 else if (bl->unichar[1] == 0x006A) {
					 if (zm_go_ne(zs) && zm_go_down(zs) && !zm_go_right(zs)) {
						 cjkBestListInsertFirst(0x006A, pSession);
						 return;
					 }
				 }
			 }
			 /* END CHUNK: check chinese i, j */
			 /*----------------------------------------------------- */


			 /* Fall-through */

		 case 0x3046:
		 case 0x30E9:
		 case UC_COLON:
		 case UC_SEMICOLON: {
			 DECUMA_INT32 bottom1 = CJK_GP_GET_Y(cjkStrokeGetMaxYGridpoint(&s1));
			 DECUMA_INT32 top2 = CJK_GP_GET_Y(cjkStrokeGetMinYGridpoint(&s2));
			 DECUMA_INT32 bottom2 = CJK_GP_GET_Y(cjkStrokeGetMaxYGridpoint(&s2));
			 DECUMA_INT32 left2 = CJK_GP_GET_X(cjkStrokeGetMinXGridpoint(&s2));
			 DECUMA_INT32 right2 = CJK_GP_GET_X(cjkStrokeGetMaxXGridpoint(&s2));
			 DECUMA_INT32 firstx2 = CJK_STROKE_GET_X(&s2, 1);
			 DECUMA_INT32 startydir2 = CJK_STROKE_GET_Y(&s2, 2) - CJK_STROKE_GET_Y(&s2, 1);
			 if (bottom1 <= top2 && bottom2 > top2) {
				 if (startydir2 <= 0 || (right2 - left2 >= 3 && firstx2 == left2)) {
					 cjkBestListInsertFirst(0x3046, pSession);
					 goto CASE_0x3046_0x30E9_TWO;
				 }
				 /*if ((bl->unichar[0] == 0x0069) && (pSession -> boxheight) &&
				 !(categorymask & CJK_HAN)) {
				 if  ((bottom2 == CJK_STROKE_GET_Y(&s2, -1)) &&
				 ((dltCCCompressGetYmax(c) - pSession->boxybase) * 4 > (pSession->boxheight) * 3)) {
				 cjkBestListInsertFirst(0x006A, pSession);
				 }
				 }*/
			 }
			 /* Check for largest stroke */
			 /*else if ((bl->unichar[0] == 0x0069) && (pSession -> boxheight) &&
			 !(categorymask & CJK_HAN)) {
			 if  ((bottom1 == CJK_STROKE_GET_Y(&s1, -1)) &&
			 ((dltCCCompressGetYmax(c) - pSession->boxybase) * 4 > (pSession->boxheight) * 3)) {
			 cjkBestListInsertFirst(0x006A, pSession);
			 }
			 }*/
									  }
									  return;
									  /*  */
									  /* We check for obvious cases of katakana 30A6 and 3046/30E9 conflict. */
									  /* Hiragana 3046 */
									  /* is in reality written exactly like katakana 30E9, so for that confusion */
									  /* we use only context. */
									  /*  */
		 case 0x30A6:
CASE_0x3046_0x30E9_TWO: {
			 DECUMA_INT32 y1 = CJK_STROKE_GET_Y(&s2, 1);
			 DECUMA_INT32 y2 = CJK_STROKE_GET_Y(&s2, 2);
			 DECUMA_INT32 y3 = CJK_STROKE_GET_Y(&s2, 3);
			 if (y2 - y1 > 0 && y2 - y1 > y3 - y2) {
				 cjkBestListInsertFirst(0x30A6, pSession);
			 }
			 else if (y2 - y1 <= 0) {
				 cjkBestListInsertFirst(0x3046, pSession);
			 }
						}
						if (bl->unichar[0] == 0x3046 || bl->unichar[0] == 0x30E9) {

							/* Distinguish between U3046 and U30E9 by looking at the "triangularness" of the area */

							int area, tarea, ratio = 1000;
							int x1, x2, x3, y1, y2, y3;
							CJK_ZOOMSTROKE s;

							zm_create_zoom_stroke(c, dltCCCompressGetNbrStrokes(c) == 1 ? 0 : 1, &s);
							zm_remove_edge_noise(&s);

							area = zm_area(&s);

							x1 = s.pPoints[s.nStartIdx].x;
							y1 = s.pPoints[s.nStartIdx].y;
							x3 = s.pPoints[s.nEndIdx].x;
							y3 = s.pPoints[s.nEndIdx].y;

							if (zm_go_right(&s)) {

								x2 = s.pPoints[s.nStartIdx].x;
								y2 = s.pPoints[s.nStartIdx].y;

								tarea = x1 * y2 + x2 * y3 + x3 * y1 - x2 * y1 - x3 * y2 - x1 * y3;

								ratio = tarea != 0 ? (100 * area) / tarea : 1000;
							}

							if (ratio < 140) {      
								cjkBestListInsertFirst(0x30E9, pSession);
							}
							else {
								cjkBestListInsertFirst(0x3046, pSession);
							}
						}
						return;
						/*  */
						/* A twostroked 1 (one) and a exclamation mark may be confused. Also */
						/* the right japanese parenthesis is a risk. */
						/*  */
		 case 0x0031:
		 case UC_RIGHT_CORNER_BRACKET:
		 case UC_EXCLAMATION_MARK: {
			 DECUMA_INT32 y1 = CJK_STROKE_GET_Y(&s1, 1);
			 DECUMA_INT32 y2 = CJK_STROKE_GET_Y(&s1, 2);
			 DECUMA_INT32 x1 = CJK_STROKE_GET_X(&s2, 1);
			 DECUMA_INT32 xe = CJK_STROKE_GET_X(&s2, -1);
			 if (y2 < y1) {
				 cjkBestListInsertFirst(0x0031, pSession);
			 }
			 else if (xe - x1 >= 3) {
				 cjkBestListInsertFirst(UC_RIGHT_CORNER_BRACKET, pSession);
			 }
			 else {
				 cjkBestListInsertFirst(UC_EXCLAMATION_MARK, pSession);
			 }
											 }
											 return;
											 /*  */
											 /*  */
											 /* If 5DE5 (work) is on the first place and it is written with two */
											 /* strokes then it must be a katakana or possibly number 1 written */
											 /* in two strokes. */
											 /*  */
		 case 0x5DE5:
		 case 0x30E6:
			 if (CJK_STROKE_GET_X(&s1, 2) - CJK_STROKE_GET_X(&s1, 1) <=
				 CJK_STROKE_GET_Y(&s1, 1) - CJK_STROKE_GET_Y(&s1, 2) &&
				 CJK_STROKE_GET_X(&s2, -1) - CJK_STROKE_GET_X(&s2, 1) <= 8) {
					 cjkBestListInsertFirst(0x0031, pSession);
			 }
			 else {
				 cjkBestListInsertFirst(0x30E6, pSession);
			 }
			 return;
			 /*  */
			 /* A japanese style seven is confused with katakana 30AF and 30EF. */
			 /* The logic now is the following. If last part of first stroke is */
			 /* concave, then it is seven, else it is one of the katakana characters. */
			 /* If we can't decide if it is concave, then context is used. */
			 /* If it is a katakana then if the first stroke is rightslant, then */
			 /* it is 30EF. If it is leftslant, then it is 30AF. If we can't */
			 /* decide, then we use context. First we check the width. If it is */
			 /* extremely narrow, then it is a 7. */
			 /*  */
			 /* For the mathematics we note that */
			 /* $$ */
			 /* \sum \left((x_i - x_0) - \frac{x_e-x_0}{y_e-y_0}(y_i-y_0)\right) */
			 /* = */
			 /* \left(\sum (x_i - x_0) \right) - \frac{x_e-x_0}{y_e-y_0}\sum(y_i-y_0) */
			 /* $$ */
			 /*  */
		 case 0x0037:
		 case 0x30AF:
		 case 0x30EF: {
			 DECUMA_INT32 x, y, i;
			 DECUMA_INT32 x0 = 0, y0 = 0, xSum = 0, ySum = 0, n = 0;
			 DECUMA_INT32 ystart = CJK_STROKE_GET_Y(&s2, 1);
			 if (CJK_STROKE_NPOINTS(&s2) < CJK_STROKE_NPOINTS(&s1)) {
				 goto CASE_SEVEN_EUROPESTYLE;
			 }
			 if (2 * (dltCCCompressGetXmax(c) - dltCCCompressGetXmin(c)) < (dltCCCompressGetYmax(c) - dltCCCompressGetYmin(c))) {
				 cjkBestListInsertFirst(0x0037, pSession);
				 return;
			 }
			 for (i = 1; i <= CJK_STROKE_NPOINTS(&s2); i++) {
				 x = CJK_STROKE_GET_X(&s2, i);
				 y = CJK_STROKE_GET_Y(&s2, i);
				 if (x >= x0 && y <= ystart) {
					 x0 = x;
					 y0 = y;
					 xSum = 0;
					 ySum = 0;
					 n = 0;
				 }
				 xSum += x - x0;
				 ySum += y - y0;
				 n++;
			 }
			 if ((y - y0) > 0 && n > 0) {
				 DECUMA_INT32 convexity_limit = 0;
				 DECUMA_INT32 convexity = (100 * xSum - (100 * (x - x0) * ySum) / (y - y0)) / n;
				 /*printf("convexity = %d\n", convexity); */
				 if (decumaIsDigit(cjkContextGetPrevious(con))) {
					 convexity_limit = 50;
				 }
				 if (convexity < convexity_limit) {
					 cjkBestListInsertFirst(0x0037, pSession);
				 }
				 else if (CJK_STROKE_GET_X(&s1, 1) <= CJK_STROKE_GET_X(&s1, -1)) {
					 cjkBestListInsertFirst(0x30EF, pSession);
				 }
				 else {
					 cjkBestListInsertFirst(0x30AF, pSession);
				 }
			 }
					  }
					  return;
					  /*  */
					  /*  */
					  /* If katakana is allowed then a twostroke kanji 4E0B and a european style */
					  /* seven are not allowed on first place. Likewise, we must fix that 30B9 is */
					  /* sometimes confused with 30CC (check if second stroke starts weakly to */
					  /* the left of the first stroke) and that 30CC too */
					  /* often ends up as kanji 53C8 (check if the gap is large). */
					  /*  */
		 case 0x30F2:
		 case 0x30B9:
		 case 0x30CC:
		 case 0x4E0B:
CASE_SEVEN_EUROPESTYLE:
			 if (categorymask & CJK_KATAKANA) {
				 CJK_UNICHAR uc;
				 cjkBestListDiminish(0x4E0B, pSession);
				 cjkBestListDiminish(0x0037, pSession);
				 uc = bl->unichar[0]; /* The top candidate after the changes! */
				 if (uc == 0x30CC || uc == 0x30B9) {
					 DECUMA_INT32 i;
					 DECUMA_INT32 y21 = CJK_STROKE_GET_Y(&s2, 1);
					 DECUMA_INT32 x21 = CJK_STROKE_GET_X(&s2, 1);
					 for (i = 3; i <= CJK_STROKE_NPOINTS(&s1); i++) {
						 if (CJK_STROKE_GET_Y(&s1, i) >= y21) {
							 if (x21 >= CJK_STROKE_GET_X(&s1, i) - 1) {
								 cjkBestListInsertFirst(0x30B9, pSession);
							 }
							 else {
								 cjkBestListInsertFirst(0x30CC, pSession);
							 }
							 break;
						 }
					 }
				 }
			 }
			 return;
			 /*  */
			 /* A hook changes a 30D2 to a 30BB. */
			 /*  */
		 case 0x30D2:
CASE_0x30D2_TWO:
			 if (CJK_STROKE_GET_X(&s1, 1) < 8 && CJK_STROKE_GET_X(&s1, -1) <= CJK_STROKE_GET_X(&s1, -2)) {
				 cjkBestListInsertFirst(0x30BB, pSession);
			 }
			 return;
			 /*  */
			 /*  */
			 /* Number 5 must not end up as a [[UC_LEFT_CORNER_BRACKET]]. */
			 /*  */
		 case UC_LEFT_CORNER_BRACKET:
			 if (CJK_GP_GET_X(cjkStrokeGetMaxXGridpoint(&s1)) - CJK_GP_GET_X(cjkStrokeGetMinXGridpoint(&s1)) >= 2) {
				 cjkBestListInsertFirst(0x0035, pSession);
			 }
			 return;
			 /*  */
			 /* We capture that has the horizontal first and does not end to the right */
			 /* on the second stroke. Then it is a chinese 5341 (or a plus) or possibly */
			 /* katakana 30CA. */
			 /*  */
		 case 0x0074:
			 if (CJK_STROKE_GET_Y(&s2, -1) > CJK_STROKE_GET_Y(&s1, -1) &&
				 !(CJK_STROKE_GET_X(&s2, -1) > CJK_STROKE_GET_X(&s2, -2))) {
					 goto CASE_0x30CA;
			 }
			 return;
			 /*  */
			 /* CJK_CONTEXT is used for the plus sign and the corresponding kanji character. */
			 /* Size is used only for certification. */
			 /*  */
		 case 0x5341:
		 case UC_PLUS_SIGN:
CASE_0x30CA:
			 if (DLTDB_IS_JAPANESE(pSession->db))
			 {

				 /*----------------------------------------------------- */
				 /* BEGIN CHUNK: check 30CA against 5341 */
				 /*  */
				 /* A chinese ten (or plus) on the one hand and katakana 30CA on the */
				 /* other should not be confused. The chunk captures 30CA, else the code */
				 /* after is used for the rest. A linear discriminant analysis is used */
				 /* here. */
				 /*  */
				 DECUMA_INT32 y2 = CJK_STROKE_GET_Y(&s1, -1);
				 DECUMA_INT32 x3 = CJK_STROKE_GET_X(&s2, 1);
				 DECUMA_INT32 x4 = CJK_GP_GET_X(cjkStrokeGetMaxXGridpoint(&s2));
				 DECUMA_INT32 x5 = CJK_STROKE_GET_X(&s2, -1);
				 if (609*y2 + 325*x3 - 550*x4 + 470*x5 < 4553) {
					 cjkBestListInsertFirst(0x30CA, pSession);
					 return;
				 }
				 /* END CHUNK: check 30CA against 5341 */
				 /*----------------------------------------------------- */

			 }
			 cjkBestListInsertFirst(UC_PLUS_SIGN, pSession);
			 if (decumaIsLatin(cjkContextGetPrevious(con)) || decumaIsDigit(cjkContextGetPrevious(con))
#if defined(CERTIFICATION)
				 || (pSession->boxheight &&
				 ((dltCCCompressGetYmax(c) - dltCCCompressGetYmin(c)) * 7 < pSession->boxheight * 3) &&
				 ((dltCCCompressGetXmax(c) - dltCCCompressGetXmin(c)) * 7 < pSession->boxwidth * 3))
#endif
				 ) {
					 /* Do nothing, UC_PLUS_SIGN is on first place. */
			 }
			 else {
				 cjkBestListInsertFirst(0x5341, pSession); /* And '+' on second place :-) */
			 }
			 return;
			 /*  */
			 /* For separating big J from 4E01 we demand a small upper stroke for J. */
			 /*  */
		 case 0x004A:
CASE_0x4E01_AGAINST_004A: {
			 if (CJK_GP_GET_Y(cjkStrokeGetMaxYGridpoint(&s1)) > CJK_GP_GET_Y(cjkStrokeGetMaxYGridpoint(&s2))) {
				 s3 = s1; s1 = s2; s2 = s3;
			 }
			 if (CJK_STROKE_GET_X(&s2, -1) >= CJK_STROKE_GET_X(&s1, 1) &&
				 CJK_GP_GET_X(cjkStrokeGetMaxXGridpoint(&s1)) > CJK_GP_GET_X(cjkStrokeGetMaxXGridpoint(&s2))) {
					 cjkBestListInsertFirst(0x4E01, pSession);
			 }
			 else {
				 cjkBestListInsertFirst(0x004A, pSession); /* J */
			 }
						  }
						  return;
						  /* END CHUNK: specialcheck twostrokes */
						  /*----------------------------------------------------- */


		}
	}

	/* No overlaps are allowed between languages. */
	switch (u) {

		/*----------------------------------------------------- */
		/* BEGIN CHUNK: specialcheck anystrokes SIMP */
		/*  */
		/* People to the left and either of two things that look as $\pi$ */
		/* to the right. */
		/*  */
	  case 0x4EC7:
	  case 0x4EC9:
		  if (nStrokes == 3 || nStrokes == 4) {
			  CJK_STROKE *nextlast, *last;
			  if (nStrokes == 3) {
				  nextlast = &s2;
				  last = &s3;
			  }
			  else {
				  nextlast = &s3;
				  last = &s4;
			  }
			  if (CJK_STROKE_NPOINTS(nextlast) == 2 &&
				  CJK_STROKE_GET_X(nextlast, 2) > 6 &&
				  CJK_STROKE_NPOINTS(last) >= 4) {
					  if (CJK_STROKE_GET_X(nextlast, 1) <= CJK_STROKE_GET_X(last, 1) &&
						  CJK_STROKE_GET_Y(nextlast, 1) >= CJK_STROKE_GET_Y(last, 1)) {
							  cjkBestListInsertFirst(0x4EC9, pSession);
					  }
					  else if (CJK_STROKE_GET_X(nextlast, 1) > CJK_STROKE_GET_X(last, 1) &&
						  CJK_STROKE_GET_Y(nextlast, 1) < CJK_STROKE_GET_Y(last, 1)) {
							  cjkBestListInsertFirst(0x4EC7, pSession);
					  }
			  }
		  }
		  return;
		  /*  */
		  /*  */
		  /* We have the same component confusion in [[5F62]] and a */
		  /* popular form of [[5F71]]. */
		  /*  */
	  case 0x5F62:
	  case 0x5F71:
		  if (nStrokes == 7) {
			  if (CJK_STROKE_GET_Y(&s4, 1) < CJK_STROKE_GET_Y(&s1, -1)) {
				  cjkBestListInsertFirst(0x5F71, pSession);
			  }
			  else {
				  cjkBestListInsertFirst(0x5F62, pSession);
			  }
		  }
		  return;
		  /*  */
		  /*  */
	  case 0x53CB:
		  if (nStrokes >= 2 && CJK_STROKE_NPOINTS(&s1) == 2) {
			  if (CJK_STROKE_GET_Y(&s2, 1) >= CJK_GP_GET_Y(cjkStrokeGetMaxYGridpoint(&s1))) {
				  cjkBestListInsertFirst(0x53CD, pSession);
			  }
		  }
		  return;
		  /*  */
		  /*  */
		  /* \f{figure}[h!] */
		  /*  \begin{center} */
		  /*    \begin{tabular}{cc} */
		  /*       \epsfig{file=../../figures/7535.eps} & */
		  /*       \epsfig{file=../../figures/7529.eps} */
		  /*    \end{tabular} */
		  /*  \end{center} */
		  /* \f} */
		  /*  */
		  /*  */
		  /* The characters 'electricity' (7535) and 'throw away' (7529) */
		  /*  */
	  case 0x7535:
	  case 0x7529:
		  if (nStrokes == 5) {
			  if (CJK_STROKE_GET_Y(&s5, 1) < CJK_STROKE_GET_Y(&s2, 2)) {
				  cjkBestListInsertFirst(0x7535, pSession);
			  }
			  else {
				  cjkBestListInsertFirst(0x7529, pSession);
			  }
		  }
		  return;
		  /*  */
	  case 0x8F66:
	  case 0x5728:
		  if (nStrokes == 4 && CJK_STROKE_NPOINTS(&s4) <= 3) {
			  CJK_STROKE  horisontal, vertical;
			  if (CJK_STROKE_GET_Y(&s3,-1) - CJK_STROKE_GET_Y(&s3,1) >
				  CJK_STROKE_GET_X(&s3,-1) - CJK_STROKE_GET_X(&s3,1)) {
					  vertical = s3;
					  horisontal = s4;
			  }
			  else {
				  vertical = s4;
				  horisontal = s3;
			  }
			  if (2* CJK_STROKE_GET_Y(&vertical,-1) > CJK_STROKE_GET_Y(&horisontal,1)
				  + CJK_STROKE_GET_Y(&horisontal,-1) + 2) {
					  cjkBestListInsertMany(uclist_8F66, pSession);
			  }
			  else {
				  cjkBestListInsertMany(uclist_5728, pSession);
			  }
		  }
		  return;
		  /*  */
		  /*  */
		  /* For cursive written characters we furthermore have */
		  /* to look at 4F1A and 5168 as well as 4F58 and 4F59. */
		  /* Note that not only 4F59 and 4F58 but */
		  /* also 5168 comes here via the [[CASE_0x4F1A_ANY]] label. */
		  /*  */
	  case 0x4F1A:
CASE_0x4F1A_ANY:
		  if (nStrokes <= 3) {
			  DECUMA_INT32 n, nloops, leftend;
			  CJK_STROKE s;
			  dltCCharGetLastStroke(c, &s, pSession);
			  nloops = cjkStrokeGetIntersectionCount(&s, pSession);
			  leftend = CJK_STROKE_GET_X(&s, -1) < CJK_STROKE_GET_X(&s, -2);
			  if (nloops == 0 && leftend) {
				  cjkBestListInsertFirst(0x4F1A, pSession);
				  return;
			  }
			  else {

				  /*----------------------------------------------------- */
				  /* BEGIN CHUNK: check loop in 4F59 */
				  /*  */
				  /* We must look for a special counterclockwise loop */
				  /* for detecting 4F59. The numbers are tuned after the */
				  /* file [[C1p4F58.arcs]]. */
				  /*  */
				  const CJK_GRIDPOINT * p;
				  CJK_GRIDPOINT gp1, gp2, gp3, gp4, gp5;
				  DECUMA_UINT8 x1, x2, x3, x4, x5, y1, y2, y3, y4, y5;
				  DECUMA_INT32 i;
				  n = CJK_STROKE_NPOINTS(&s);
				  for (i = 3; i <= n - 4; i++) {
					  p = cjkStrokeGetGridpoint(&s, i);
					  gp1 = p[0];
					  gp2 = p[1];
					  gp3 = p[2];
					  gp4 = p[3];
					  gp5 = p[4];
					  x1 = CJK_GP_GET_X(gp1);
					  x2 = CJK_GP_GET_X(gp2);
					  x3 = CJK_GP_GET_X(gp3);
					  x4 = CJK_GP_GET_X(gp4);
					  x5 = CJK_GP_GET_X(gp5);
					  y1 = CJK_GP_GET_Y(gp1);
					  y2 = CJK_GP_GET_Y(gp2);
					  y3 = CJK_GP_GET_Y(gp3);
					  y4 = CJK_GP_GET_Y(gp4);
					  y5 = CJK_GP_GET_Y(gp5);
					  if ((x1 < x2 && x1 < x3 && x1 < x4 && x2 > x4 && y3 < y1 &&
						  ((y3 <= y2 && x2 > x3 + 1) || (y3 < y2 && x2 > x3)) &&
						  y3 < y4 && y4 > y1 && y4 > y2) ||
						  (x1 < x2 && x1 < x4 && x1 < x5 && x2 > x4 && x2 > x5 &&
						  y4 < y1 && y4 <= y2 && y4 < y5 && y5 > y1 && y5 > y2)) {
							  cjkBestListInsertFirst(0x4F59, pSession);
							  return;
					  }
				  }
				  /* END CHUNK: check loop in 4F59 */
				  /*----------------------------------------------------- */


				  if ((u == 0x4F59 || u == 0x4F1A) && nloops >= 1) {
					  cjkBestListInsertFirst(0x4F58, pSession);
					  return;
				  }
			  }
		  }
		  return;
		  /*  */
		  /* \f{figure}[ht!] */
		  /*  \begin{center} */
		  /*         \begin{tabular}{cc} */
		  /*                 \epsfig{file=../../figures/9cb4.eps}  & */
		  /*                 \epsfig{file=../../figures/9cb7.eps} */
		  /*         \end{tabular} */
		  /*  \end{center} */
		  /* \f} */
		  /*  */
		  /* 0x9CB4 is mixed up with 0x9CB7. This special check assumes and is based on that */
		  /* the last stroke is the horizontal closing the rigth part for 0x9CB4 and the last */
		  /* is one closing the square to the right in 0x9CB7. */
		  /*  */
		  /*  */
	  case 0x9CB4:
	  case 0x9CB7: {
		  DECUMA_INT32 smaxy = 0, maxy = 0;
		  CJK_STROKE s;
		  for (s = s1; CJK_STROKE_EXISTS(&s); cjkStrokeNext(&s, pSession)) {
			  smaxy = CJK_GP_GET_Y(cjkStrokeGetMaxYGridpoint(&s));
			  if (smaxy  > maxy) {
				  maxy = smaxy ;
			  }
		  }
		  if (smaxy + 1 >= maxy) {
			  cjkBestListInsertFirst(0x9CB4, pSession);
		  }
				   }
				   return;
				   /*  */
				   /* \f{figure}[ht!] */
				   /*  \begin{center} */
				   /*   \epsfig{file=../../figures/5408.eps} */
				   /*  \end{center} */
				   /* \f} */
				   /*  */
				   /* The character 5408 gets always mixed up with 4F65 and 5168. */
				   /*  */
	  case 0x5408:
	  case 0x4F65:
	  case 0x5168:
		  if (nStrokes == 5) {
			  if (CJK_STROKE_NPOINTS(&s4) == 2 && CJK_STROKE_NPOINTS(&s5) > 2 &&
				  CJK_GP_GET_X(cjkStrokeGetMaxXGridpoint(&s4)) <= CJK_GP_GET_X(cjkStrokeGetMinXGridpoint(&s5))) {
					  cjkBestListInsertFirst(0x5408, pSession);
			  }
		  }
		  else if (nStrokes == 4 && CJK_STROKE_NPOINTS(&s4)) {
			  DECUMA_INT32 i, k, x1 = 99, x2;
			  for (i = 1; i <= CJK_STROKE_NPOINTS(&s4); i++) {
				  /* get index of minimum x */
				  if ((x2 = CJK_STROKE_GET_X(&s4, i)) < x1) {
					  k = i;
					  x1 = x2;
				  }
			  }
			  if (CJK_STROKE_GET_X(&s4, 1) < 8) {
				  if (k >= CJK_STROKE_NPOINTS(&s4) - 1) {
					  cjkBestListInsertFirst(0x4F65, pSession);
				  }
				  else if (k <= 2) {
					  cjkBestListInsertFirst(0x5408, pSession);
				  }
			  }
		  }
		  else if (nStrokes <= 3 && u == 0x5168) {
			  goto CASE_0x4F1A_ANY;
		  }
		  return;
		  /*  */
		  /* A 7EAB and a 7EA8 can be distinguished through the end direction of the */
		  /* third last stroke. */
		  /*  */
	  case 0x7EAB:
	  case 0x7EA8: {
		  if (nStrokes >= 5) {
			  CJK_STROKE s_thirdlast;
			  DECUMA_INT32 i;

			  /* Find the third last stroke */
			  s_thirdlast = s1;
			  for (i = 1; i < nStrokes - 2; i++){
				  cjkStrokeNext(&s_thirdlast, pSession);
			  }

			  /* If it ends to the right or up it is 7EA8, */
			  /* to the left it is 7EAB. */
			  if (CJK_STROKE_NPOINTS(&s_thirdlast) >= 4) {
				  CJK_GRIDPOINT p3 = *cjkStrokeGetGridpoint(&s_thirdlast, -2);
				  CJK_GRIDPOINT p4 = *cjkStrokeGetGridpoint(&s_thirdlast, -1);
				  DECUMA_INT32 x3 = CJK_GP_GET_X(p3);
				  DECUMA_INT32 x4 = CJK_GP_GET_X(p4);
				  DECUMA_INT32 y3 = CJK_GP_GET_Y(p3);
				  DECUMA_INT32 y4 = CJK_GP_GET_Y(p4);
				  if (y3 > 8 && y4 > 8) {
					  if (x4 > x3 || (x4 == x3 && y4 < y3)) {
						  cjkBestListInsertFirst(0x7EA8, pSession);
					  }
					  else if (x4 < x3 && CJK_STROKE_NPOINTS(&s_thirdlast) == 4) {
						  cjkBestListInsertFirst(0x7EAB, pSession);
					  }
				  }
			  }
		  }
				   }
				   return;
				   /*  */
				   /*  */
				   /* The character [[6F20]] is confused with [[6C49]] since there */
				   /* is a very similar traditional character [[6F22]] that is mapped */
				   /* to [[6C49]]. This only happens in the simplified chinese engine */
				   /* of course. We always make [[6F20]] the first candidate. */
				   /*  */
	  case 0x6C49:
		  if (bl->unichar[1] == 0x6F20 || bl->unichar[2] == 0x6F20) {
			  cjkBestListInsertFirst(0x6F20, pSession);
		  }
		  return;
		  /* END CHUNK: specialcheck anystrokes SIMP */
		  /*----------------------------------------------------- */



		  /*----------------------------------------------------- */
		  /* BEGIN CHUNK: specialcheck anystrokes TRAD */
		  /*  */
		  /* Traditional 958B should not be interpreted as 958C. */
		  /*  */
	  case 0x958C:
		  if (CJK_STROKE_NPOINTS(&s_fromend[0]) == 2) {
			  cjkBestListInsertFirst(0x958B, pSession);
		  }
		  return;
		  /*  */
		  /* If the last stroke in 76E5 is a simple horizontal and it is not at the bottom */
		  /* then it is 58CF instead. */
		  /*  */
	  case 0x76E5:
		  if (nStrokes > 1 && CJK_STROKE_NPOINTS(&s_fromend[0]) == 2) {
			  DECUMA_INT32 lowest_last = CJK_GP_GET_Y(cjkStrokeGetMaxYGridpoint(&s_fromend[0]));
			  DECUMA_INT32 lowest_prev = CJK_GP_GET_Y(cjkStrokeGetMaxYGridpoint(&s_fromend[1]));
			  if (lowest_prev > lowest_last) {
				  cjkBestListInsertFirst(0x5B78, pSession);
			  }
		  }
		  return;
		  /* END CHUNK: specialcheck anystrokes TRAD */
		  /*----------------------------------------------------- */



		  /*----------------------------------------------------- */
		  /* BEGIN CHUNK: specialcheck anystrokes JAPN */
		  /*  */
		  /*  */
		  /* Hiragana 304D is similar to Kanji 738B. When writing Hiragana 307E, Hiragana 304D */
		  /* and Katakana 30E2 often got in first place, which is resolved by checking the */
		  /* existense of a loop in the last stroke. */
		  /* Katakana 30E2 showed up in first place when writing Hiragana 3082. */
		  /*  */
	  case 0x304D:
	  case 0x30E2:
	  case 0x738B:
		  if (nStrokes == 3 && u == 0x304D && CJK_STROKE_NPOINTS(&s1) == 2) {
			  /*cjkBestListInsertFirst(0x3082, pSession); */
		  }
		  else if (nStrokes == 4) {
			  /*if (min(CJK_STROKE_GET_Y(&s2, 1), CJK_STROKE_GET_Y(&s3, 1)) < CJK_STROKE_GET_Y(&s1, 2)) {
			  cjkBestListInsertFirst(0x304D, pSession);
			  }
			  else {
			  if (CJK_STROKE_NPOINTS(&s4) == 2 &&
			  ABS(CJK_STROKE_GET_Y(&s4, 1) - CJK_STROKE_GET_Y(&s4, 2)) <= 1 &&
			  CJK_GP_GET_Y(cjkStrokeGetMaxYGridpoint(&s4)) - CJK_STROKE_GET_Y(&s3, -1) <= 1 &&
			  CJK_STROKE_GET_X(&s4, 2) > CJK_STROKE_GET_X(&s3, -1)) {
			  cjkBestListInsertFirst(0x738B, pSession);
			  }
			  else {
			  cjkBestListInsertFirst(0x304D, pSession);
			  }
			  }*/
		  }
		  else if ((u == 0x304D || u == 0x30E2) &&
			  nStrokes == 3 && cjkStrokeGetIntersectionCount(&s3, pSession) > 0) {
				  cjkBestListInsertFirst(0x307E, pSession);
		  }
		  else if (u == 0x30E2 && nStrokes == 3 &&
			  CJK_GP_GET_Y(cjkStrokeGetMinYGridpoint(&s1)) > CJK_STROKE_GET_Y(&s3, 1)) {
				  cjkBestListInsertFirst(0x3082, pSession);
		  }
		  return;
		  /*  */
		  /*  */
		  /* Recognizing an E might mean that the user really wrote hiragana 3082. */
		  /*  */
		  /*case 0x0045:
		  if (nStrokes == 3 &&
		  CJK_STROKE_NPOINTS(&s1) > CJK_STROKE_NPOINTS(&s2) &&
		  CJK_STROKE_NPOINTS(&s1) > CJK_STROKE_NPOINTS(&s3)) {
		  if (CJK_STROKE_GET_Y(&s1, -1) < CJK_STROKE_GET_Y(&s1, -2) &&
		  CJK_STROKE_GET_X(&s1, 1) > CJK_STROKE_GET_X(&s2, 1)) {
		  cjkBestListInsertFirst(0x3082, pSession);
		  }
		  }
		  return;*/
		  /*  */
	  case 0x30AA:
	  case 0x624D:
		  if (nStrokes == 3 && (categorymask & CJK_KATAKANA) ) {
			  if(CJK_GP_GET_X(cjkStrokeGetMaxXGridpoint(&s3)) > CJK_GP_GET_X(cjkStrokeGetMaxXGridpoint(&s2)) ) {
				  cjkBestListInsertMany(uclist_624D, pSession);
			  }
			  else {
				  cjkBestListInsertMany(uclist_30AA, pSession);
			  }
		  }
		  return;
		  /*  */
		  /* In japanese there is a simplified form of 6A5F that is confused */
		  /* with 6746. We check for that. */
		  /*  */
	  case 0x6A5F:
		  if (nStrokes == 7) {
			  if(CJK_STROKE_GET_Y(&s_fromend[0], 1) >= CJK_STROKE_GET_Y(&s_fromend[2], -1)) {
				  cjkBestListInsertFirst(0x6746, pSession);
			  }
		  }
		  return;
		  /*  */
		  /*  */
		  /*  */
		  /* This special check is for separating katakana and hiragana characters with a */
		  /* ring from similar characters with two dots. */
		  /*  */
	  case 0x3070: case 0x3071:
	  case 0x3073: case 0x3074:
	  case 0x3076: case 0x3077:
	  case 0x307C: case 0x307D:
		  /*case 0x30DC: case 0x30DD: */
		  {

			  /*----------------------------------------------------- */
			  /* BEGIN CHUNK: check for fnutt */
			  /*  */
			  /*  */
			  DECUMA_INT32 isfnutt = 0;
			  DECUMA_INT32 issmall_u_r1 = 0;
			  DECUMA_INT32 issmall_u_r2 = 0;
			  DECUMA_INT32 issmall_u_r3 = 0;
			  DECUMA_INT32 issmall_u_r4 = 0;
			  DECUMA_INT32 issmall_u_r5 = 0;
			  DECUMA_INT32 issmall_u_r6 = 0;
			  DECUMA_INT32 issmall_u_r7 = 0;
			  DECUMA_INT32 isfnutt1 = 0;
			  DECUMA_INT32 isfnutt2 = 0;
			  DECUMA_INT32 isfnutt3 = 0;
			  DECUMA_INT32 isfnutt4 = 0;
			  DECUMA_INT32 isfnutt5 = 0;
			  DECUMA_INT32 isfnutt6 = 0;

			  issmall_u_r1 = (cjkStrokeIsSmall(&s1) &&
				  (CJK_GP_GET_X(cjkStrokeGetMinXGridpoint(&s1)) > 7) &&
				  (CJK_GP_GET_Y(cjkStrokeGetMaxYGridpoint(&s1)) < 7));
			  if (CJK_STROKE_EXISTS(&s2)) issmall_u_r2 = (cjkStrokeIsSmall(&s2) &&
				  (CJK_GP_GET_X(cjkStrokeGetMinXGridpoint(&s2)) > 6) &&
				  (CJK_GP_GET_Y(cjkStrokeGetMaxYGridpoint(&s2)) < 8));
			  if (CJK_STROKE_EXISTS(&s3)) issmall_u_r3 = (cjkStrokeIsSmall(&s3) &&
				  (CJK_GP_GET_X(cjkStrokeGetMinXGridpoint(&s3)) > 6) &&
				  (CJK_GP_GET_Y(cjkStrokeGetMaxYGridpoint(&s3)) < 8));
			  if (CJK_STROKE_EXISTS(&s4)) issmall_u_r4 = (cjkStrokeIsSmall(&s4) &&
				  (CJK_GP_GET_X(cjkStrokeGetMinXGridpoint(&s4)) > 6) &&
				  (CJK_GP_GET_Y(cjkStrokeGetMaxYGridpoint(&s4)) < 8));
			  if (CJK_STROKE_EXISTS(&s5)) issmall_u_r5 = (cjkStrokeIsSmall(&s5) &&
				  (CJK_GP_GET_X(cjkStrokeGetMinXGridpoint(&s5)) > 6) &&
				  (CJK_GP_GET_Y(cjkStrokeGetMaxYGridpoint(&s5)) < 8));
			  if (CJK_STROKE_EXISTS(&s6)) issmall_u_r6 = (cjkStrokeIsSmall(&s6) &&
				  (CJK_GP_GET_X(cjkStrokeGetMinXGridpoint(&s6)) > 6) &&
				  (CJK_GP_GET_Y(cjkStrokeGetMaxYGridpoint(&s6)) < 8));
			  if (CJK_STROKE_EXISTS(&s7)) issmall_u_r7 = (cjkStrokeIsSmall(&s7) &&
				  (CJK_GP_GET_X(cjkStrokeGetMinXGridpoint(&s7)) > 6) &&
				  (CJK_GP_GET_Y(cjkStrokeGetMaxYGridpoint(&s7)) < 8));
			  isfnutt1 = (issmall_u_r1 && issmall_u_r2 && cjkStrokeIsClose(&s1, &s2));
			  isfnutt2 = (issmall_u_r2 && issmall_u_r3 && cjkStrokeIsClose(&s2, &s3));
			  isfnutt3 = (issmall_u_r3 && issmall_u_r4 && cjkStrokeIsClose(&s3, &s4));
			  isfnutt4 = (issmall_u_r4 && issmall_u_r5 && cjkStrokeIsClose(&s4, &s5));
			  isfnutt5 = (issmall_u_r5 && issmall_u_r6 && cjkStrokeIsClose(&s5, &s6));
			  isfnutt6 = (issmall_u_r6 && issmall_u_r7 && cjkStrokeIsClose(&s6, &s7));
			  isfnutt = (isfnutt1 || isfnutt2 ||
				  isfnutt3 || isfnutt4 ||
				  isfnutt5 || isfnutt6);
			  if (!isfnutt) {
				  if (u == 0x3070) {
					  cjkBestListInsertFirst(0x3071, pSession);
					  return;
				  }
				  if (u == 0x3073) {
					  cjkBestListInsertFirst(0x3074, pSession);
					  return;
				  }
				  if (u == 0x3076) {
					  cjkBestListInsertFirst(0x3077, pSession);
					  return;
				  }
				  if (u == 0x3079) {
					  cjkBestListInsertFirst(0x307A, pSession);
					  return;
				  }
				  if (u == 0x307C) {
					  cjkBestListInsertFirst(0x307D, pSession);
					  return;
				  }
				  /*
				  if (u == 0x30D0) {
				  cjkBestListInsertFirst(0x30D1, pSession);
				  return;
				  }
				  if (u == 0x30D3) {
				  cjkBestListInsertFirst(0x30D4, pSession);
				  return;
				  }
				  if (u == 0x30D6) {
				  cjkBestListInsertFirst(0x30D7, pSession);
				  return;
				  }
				  if (u == 0x30D9) {
				  cjkBestListInsertFirst(0x30DA, pSession);
				  return;
				  }
				  if (u == 0x30DC) {
				  cjkBestListInsertFirst(0x30DD, pSession);
				  return;
				  }
				  */
			  }
			  else {
				  if (u == 0x3071) {
					  cjkBestListInsertFirst(0x3070, pSession);
					  return;
				  }
				  /*if (u == 0x3074) {
				  cjkBestListInsertFirst(0x3073, pSession);
				  return;
				  }*/
				  /*if (u == 0x3077) {
				  cjkBestListInsertFirst(0x3076, pSession);
				  return;
				  }*/
				  if (u == 0x307A) {
					  cjkBestListInsertFirst(0x3079, pSession);
					  return;
				  }
				  if (u == 0x307D) {
					  cjkBestListInsertFirst(0x307C, pSession);
					  return;
				  }
				  /*
				  if (u == 0x30D1) {
				  cjkBestListInsertFirst(0x30D0, pSession);
				  return;
				  }
				  if (u == 0x30D4) {
				  cjkBestListInsertFirst(0x30D3, pSession);
				  return;
				  }
				  if (u == 0x30D7) {
				  cjkBestListInsertFirst(0x30D6, pSession);
				  return;
				  }
				  if (u == 0x30DA) {
				  cjkBestListInsertFirst(0x30D9, pSession);
				  return;
				  }
				  if (u == 0x30DD) {
				  cjkBestListInsertFirst(0x30DC, pSession);
				  return;
				  }
				  */
			  }
			  /* END CHUNK: check for fnutt */
			  /*----------------------------------------------------- */


		  }
		  return;
		  /*  */
		  /*  */
	  case 0x30D0:
	  case 0x30D1:
	  case 0x30C9: {
		  if ((categorymask & CJK_KATAKANA) && (categorymask & CJK_JIS0)) {
			  if (nStrokes == 4) {
				  if (CJK_STROKE_NPOINTS(&s2) > 2) {
					  cjkBestListInsertFirst(0x5FC3, pSession);
				  }
			  }
		  }
				   }
				   return;
				   /*  */
				   /*  */
				   /* Hiragana 3075 is often confused with Kanji 793A, */
				   /* when written with three strokes. */
				   /*  */
	  case 0x793A:
		  if (nStrokes == 3) {
			  cjkBestListInsertFirst(0x3075, pSession);
		  }
		  return;
		  /*  */
		  /*  */
	  case 0x5BB9:
		  if (nStrokes == 9 && CJK_STROKE_NPOINTS(&s5) >= 3) {
			  cjkBestListInsertFirst(0x5BA2, pSession);
		  }
		  return;
		  /*  */
		  /* The characters 0x3078, 0x3079 and 0x307A (hiragana) are $almost$ */
		  /* identical with katakana 0x30D8, 0x30D9, 0x30DA. */
		  /* Note the ultra cool coding. */
		  /*  */
	  case 0x3078: case 0x3079: case 0x307A:
	  case 0x30D8: case 0x30D9: case 0x30DA:
CASE_0x3078_ONE:
		  if (cjkContextHasPrevious(con)) {
			  if (decumaIsKatakanaAnySize(cjkContextGetPrevious(con))) {
				  u = (u & 0xFF0F) | 0x00D0;
			  }
			  else {
				  u = (u & 0xFF0F) | 0x0070;
			  }
			  cjkBestListInsertFirst(u, pSession);
		  }
		  return;
		  /*  */
		  /*  */
		  /* The hiragana 3064 conflict with katakana 30D5 is interesting. */
		  /* Characters 3065 are the same but have two dots on them. */
		  /*  */
	  case 0x3064: case 0x3065:
	  case 0x30D5: case 0x30D6:
		  if (nStrokes == 1 && (CJK_GP_GET_X(cjkStrokeGetMaxXGridpoint(&s1)) -
			  CJK_GP_GET_X(cjkStrokeGetMinXGridpoint(&s1)) <= 5)) {
				  cjkBestListInsertFirst(0x0031, pSession); /* '1' */
		  }
		  else {
			  CJK_UNICHAR u_katakana = u + ((u < 0x30A0)? (0x30D5 - 0x3064): 0);
			  CJK_UNICHAR u_hiragana = u - ((u > 0x30A0)? (0x30D5 - 0x3064): 0);

			  /* Use context if possible */

			  if (cjkContextHasPrevious(con) && decumaIsKatakanaAnySize(cjkContextGetPrevious(con))) {
				  u = u_katakana;
			  }
			  else if (cjkContextHasPrevious(con) && decumaIsHiraganaAnySize(cjkContextGetPrevious(con))) {
				  u = u_hiragana;
			  }
			  else {

				  /* First, look at how triangular the area is */

				  int area, tarea, ratio = 1000;
				  int x1, x2, x3, y1, y2, y3;
				  CJK_ZOOMSTROKE s;

				  zm_create_zoom_stroke(c, 0, &s);
				  zm_remove_edge_noise(&s);

				  area = zm_area(&s);

				  x1 = s.pPoints[s.nStartIdx].x;
				  y1 = s.pPoints[s.nStartIdx].y;
				  x3 = s.pPoints[s.nEndIdx].x;
				  y3 = s.pPoints[s.nEndIdx].y;

				  if (zm_go_right(&s)) {

					  x2 = s.pPoints[s.nStartIdx].x;
					  y2 = s.pPoints[s.nStartIdx].y;

					  tarea = x1 * y2 + x2 * y3 + x3 * y1 - x2 * y1 - x3 * y2 - x1 * y3;

					  ratio = tarea != 0 ? (100 * area) / tarea : 1000;
				  }

				  if (ratio < 131) {      
					  int pos = cjkBestListGetPosition(u_katakana, pSession);
					  /* The katakana should be among top 3 candidates */
					  if (pos >= 0 || pos <= 2) {
						  u = u_katakana;
					  }
				  }
				  else {
					  /* Wield magic powers */
					  CJK_GRIDPOINT p1 = *cjkStrokeGetGridpoint(&s1, 1);
					  CJK_GRIDPOINT p2 = cjkStrokeGetMinYGridpoint(&s1);
					  CJK_GRIDPOINT p3 = cjkStrokeGetMaxXGridpoint(&s1);
					  CJK_GRIDPOINT p4 = *cjkStrokeGetGridpoint(&s1, -1);
					  int x1 = CJK_GP_GET_X(p1);
					  int x2 = CJK_GP_GET_X(p2);
					  int x3 = CJK_GP_GET_X(p3);
					  int y3 = CJK_GP_GET_Y(p3);
					  int x4 = CJK_GP_GET_X(p4);
					  int y4 = CJK_GP_GET_Y(p4);

					  u = u_hiragana; /* default */
					  if (decumaIsKatakana(cjkContextGetPrevious(con))) {
						  /*printf("context"); */
						  u = u_katakana;
					  }
					  else if (pSession->boxheight != 0 && dltCCCompressGetYmin(c) > boxmid) {
						  /*printf("size"); */
						  u = u_hiragana; /* small size is hiragana */
					  }
					  else if (nStrokes == 1) {
						  /*printf("\nZZZ %d\n", 633*x1 + 280*x3 - 682*y3 - 237*x4); */
						  if (633*x1 + 280*x3 - 682*y3 - 237*x4 > -500) {
							  /*printf("formula1"); */
							  u = u_katakana;
						  }
					  }
					  else if (nStrokes == 3) {
						  /*printf("\nSSS %d\n", 366*x1 - 195*x2 - 249*y3 + 875*y4); */
						  if (366*x1 - 195*x2 - 249*y3 + 875*y4 > 10700) {
							  u = u_katakana;
							  /*printf("formula2"); */
						  }
					  }
					  else {
						  /*printf("default"); */
					  }
				  }
			  }
		  }
		  cjkBestListInsertFirst(u, pSession);
		  return;
		  /*  */
		  /* The two (for the user) identical characters 0x30BF (katakana) and 0x5915 */
		  /* (kanji, evening, very common) are separated with context. Also 0x52FA */
		  /* (rather unusual kanji) is written very similar. */
		  /*  */
	  case 0x30BF:
	  case 0x52FA:
	  case 0x5915: {
		  if (decumaIsKatakanaAnySize(cjkContextGetPrevious(con))) {
			  cjkBestListInsertMany(uclist_30BF, pSession);
		  }
		  else {
			  if (categorymask & CJK_HAN) {
				  cjkBestListDiminish(0x30BF, pSession);
			  }
		  }
				   }
				   return;
				   /*  */
				   /* A three stroke katakana that is identified as 30BC but des not */
				   /* go to the left at the end */
				   /* of the first stroke must be katakana 30D4 instead. A four stroke version */
				   /* with the same property must be katakana 30D3. We make a check that the */
				   /* first two strokes are written in the expected order at the very beginning. */
				   /*  */
	  case 0x30BC:
		  if (nStrokes >= 3) {
			  if (CJK_STROKE_GET_Y(&s2, 1) < CJK_STROKE_GET_Y(&s1, 1) &&
				  CJK_STROKE_GET_X(&s1, -1) > CJK_STROKE_GET_X(&s1, -2) &&
				  CJK_STROKE_GET_Y(&s1, -1) <= CJK_STROKE_GET_Y(&s1, -2)) {
					  if (nStrokes == 3) {
						  cjkBestListInsertFirst(0x30D4, pSession);
					  }
					  else if (nStrokes == 4) {
						  cjkBestListInsertFirst(0x30D3, pSession);
					  }
			  }
		  }
		  return;
		  /*  */
		  /*  */
		  /* If the guess is katakana 30C2 it might be katakana 30C7 */
		  /* in reality. And cursive fourstroke 30C2 with no crossing */
		  /* must be a cursive 30B2. */
		  /*  */
	  case 0x30C2:
		  if (nStrokes == 5) {
			  DECUMA_INT32 undershoot = 2 * CJK_STROKE_GET_Y(&s3, 1) -
				  (CJK_STROKE_GET_Y(&s2, 1) + CJK_STROKE_GET_Y(&s2, -1));
			  if (undershoot >= -2) {
				  cjkBestListInsertFirst(0x30C7, pSession);
			  }
		  }
		  else if (nStrokes == 4 && dltCCharGetIntersectionCount(c, pSession) == 0) {
			  cjkBestListInsertFirst(0x30B2, pSession);
		  }
		  return;
		  /*  */
		  /* There are four haracters that should be kanji unless */
		  /* the previous character is katakana. Three of them of them are treated */
		  /* on other places, the fourth here. */
		  /*  */
	  case 0x53E3:
	  case 0x30ED:
		  if (decumaIsKatakanaAnySize(cjkContextGetPrevious(con))) {
			  cjkBestListInsertFirst(0x30ED, pSession);
		  }
		  else {
			  cjkBestListInsertFirst(0x53E3, pSession);
		  }
		  return;
		  /* END CHUNK: specialcheck anystrokes JAPN */
		  /*----------------------------------------------------- */



		  /*----------------------------------------------------- */
		  /* BEGIN CHUNK: specialcheck anystrokes */
		  /*  */
	  case 0x4E8C: case 0x4E59: case 0x5DE5: case 0x571F:
	  case 0x58EB: case 0x0049: case 0x4E0A:
	  case UC_EQUALS_SIGN:
	  case 0x30CB:
	  case 0x3053:
	  case 0x30A8:

		  /*----------------------------------------------------- */
		  /* BEGIN CHUNK: check 4E8C anystrokes */
		  /*  */
		  if (nStrokes == 2 && (u == 0x571F || u == 0x58EB) ) {
			  if (CJK_STROKE_GET_X(&s2, -1) - CJK_STROKE_GET_X(&s2, 1) <
				  CJK_GP_GET_X(cjkStrokeGetMaxXGridpoint(&s1)) - CJK_GP_GET_X(cjkStrokeGetMinXGridpoint(&s1)) ) {
					  cjkBestListInsertMany(uclist_58EB, pSession);
					  return;
			  }
			  else {
				  cjkBestListInsertMany(uclist_571F, pSession);
				  return;
			  }
		  }

		  if (nStrokes == 2 && (u == 0x30CB || u == 0x3053 || u == 0x4E8C ||
			  u == UC_EQUALS_SIGN)) {
				  if (categorymask & CJK_GB2312_A) {
					  if (decumaIsDigit(cjkContextGetPrevious(con))) {
						  cjkBestListInsertFirst(UC_EQUALS_SIGN, pSession);
					  }
					  else {
						  cjkBestListInsertFirst(0x4E8C, pSession);
					  }
					  return;
				  }
				  else {
					  if (CJK_STROKE_GET_Y(&s1, 2) <= CJK_STROKE_GET_Y(&s1, 1) &&
						  CJK_STROKE_GET_X(&s1, -4) >= CJK_STROKE_GET_X(&s1, -1)) {
							  /*if (CJK_STROKE_GET_X(&s2, -1) > CJK_STROKE_GET_X(&s1, -1) + 2) {
							  cjkBestListInsertFirst(0x30E6, pSession);
							  }
							  else {
							  cjkBestListInsertFirst(0x30B3, pSession);
							  }*/
							  return;
					  }
					  else if (decumaIsKatakanaAnySize(cjkContextGetPrevious(con))) {
						  cjkBestListInsertMany(uclist_30CB, pSession);
						  return;
					  }
					  else {
						  int n;
						  int d;

						  /* First test if stroke 1 ends in a back twitch */
						  CJK_ZOOMSTROKE s;

						  zm_create_zoom_stroke(c, 0, &s);
						  /* zm_remove_edge_noise(&s); */
						  /* l = zm_get_stroke_length(&s); */
						  d = s.pPoints[s.nEndIdx].x - s.pPoints[s.nEndIdx - 1].x;
						  /*if (d < - l / 20) { */
						  /*cjkBestListInsertFirst(0x30CB, pSession); // Could be U4e8c, how to handle? */
						  if (d < 0) {
							  cjkBestListInsertFirst(0x30CB, pSession); /* Could be U4e8c, how to handle? */
						  }
						  else {
							  /* Otherwise test if the second stroke is concave (for some approximation of concave) */
							  zm_create_zoom_stroke(c, 1, &s);
							  zm_remove_edge_noise(&s);

							  s.nStartIdx++;
							  s.nEndIdx--;

							  n = zm_npoints_right_of_line(&s, s.pPoints[s.nStartIdx - 1].x, 
								  s.pPoints[s.nStartIdx - 1].y, 
								  s.pPoints[s.nEndIdx + 1].x, 
								  s.pPoints[s.nEndIdx + 1].y);

							  if (s.nEndIdx - s.nStartIdx >= 0 && (n * 100) / (s.nEndIdx - s.nStartIdx + 1) > 50) {
								  /* More than half the nodes in the stroke are above the straight line between the start to end points */
								  /* This is concave enough for our purposes. */
								  cjkBestListInsertFirst(0x30CB, pSession); /* Could be U4e8c, how to handle? */
							  }
						  }
					  }
				  }
		  }
		  /*  */
		  /* If the vertical stroke starts above the first horizontal stroke */
		  /* then it is not 0049 (I) or 5DE5 (``work''). Problem: some people */
		  /* write the vertical first. The formula below is from a logistic */
		  /* regression dicriminant analysis. */
		  /*  */
		  if (nStrokes == 3) {

			  /*----------------------------------------------------- */
			  /* BEGIN CHUNK: check for shifting s1 and a2 */
			  /*  */
			  /* If the y-width of stroke 2 is smaller than the y-width of stroke 1 */
			  /* then the user must have started with the vertical stroke. We simply */
			  /* swap them in order to be able to continue the analysis. */
			  /*  */
			  if (CJK_STROKE_GET_Y(&s2, -1) - CJK_STROKE_GET_Y(&s2, 1) <
				  CJK_STROKE_GET_Y(&s1, -1) - CJK_STROKE_GET_Y(&s1, 1)) {
					  CJK_STROKE stmp;
					  stmp = s1;
					  s1 = s2;
					  s2 = stmp;
			  }
			  /* END CHUNK: check for shifting s1 and a2 */
			  /*----------------------------------------------------- */


			  if (CJK_STROKE_GET_Y(&s2, 1) >= CJK_STROKE_GET_Y(&s1, -1)) {
				  if (4 * (dltCCCompressGetXmax(c) - dltCCCompressGetXmin(c)) > 3 * (dltCCCompressGetYmax(c) - dltCCCompressGetYmin(c))) {
					  if (DLTDB_IS_JAPANESE(pSession->db)) {
						  goto CASE_0x30A8_ANY;
					  }
					  else {
						  cjkBestListInsertFirst(0x5DE5, pSession);
						  return;
					  }
				  }
				  cjkBestListInsertFirst(0x0049, pSession);
				  return;
			  }
			  else {
				  if ( (DLTDB_IS_JAPANESE(pSession->db)) && ((CJK_STROKE_GET_Y(&s2, -1) + 1 < CJK_STROKE_GET_Y(&s3, 2))) ) { /* Japanese DB */
					  cjkBestListInsertFirst(0x3055, pSession);
					  return;
				  }
				  if (843 * CJK_STROKE_GET_X(&s1, 1) + 66 * CJK_STROKE_GET_X(&s1, -1)
					  - 507 * CJK_STROKE_GET_X(&s2, 1) - 168 * CJK_STROKE_GET_X(&s2, -1) >= 1074) {
						  cjkBestListInsertFirst(0x4E0A, pSession);
						  return;
				  }
				  else if (CJK_STROKE_GET_X(&s1, -1) - CJK_STROKE_GET_X(&s1, 1) <
					  CJK_STROKE_GET_X(&s3, -1) - CJK_STROKE_GET_X(&s3, 1)) {
						  cjkBestListInsertFirst(0x571F, pSession);
						  return;
				  }
				  else {
					  cjkBestListInsertFirst(0x58EB, pSession);
					  return;
				  }
			  }
		  }
		  /* END CHUNK: check 4E8C anystrokes */
		  /*----------------------------------------------------- */


		  return;
		  /*  */
		  /*  */
		  /* \f{figure}[h!] */
		  /*  \begin{center} */
		  /*    \begin{tabular}{cccc} */
		  /*       \epsfig{file=../../figures/5df1.eps} & */
		  /*       \epsfig{file=../../figures/5df2.eps} & */
		  /*       \epsfig{file=../../figures/5df3.eps} */
		  /*    \end{tabular} */
		  /*  \end{center} */
		  /* \f} */
		  /*  */
		  /* Small e in three versions. This is optimized against the chinese */
		  /* big database. */
		  /*  */
	  case 0x5DF1:
	  case 0x5DF2:
	  case 0x5DF3:
		  if (nStrokes == 3) {
			  DECUMA_UINT32 y1 = CJK_STROKE_GET_Y(&s1, 1);
			  DECUMA_UINT32 y2 = CJK_STROKE_GET_Y(&s2, 1);
			  DECUMA_UINT32 y3 = CJK_STROKE_GET_Y(&s3, 1);
			  DECUMA_INT32 dA = y3 - y1;
			  DECUMA_INT32 dB = y2 - y3;
			  if (dA <= 1) {
				  cjkBestListInsertFirst(0x5DF3, pSession); /* Closed, unusal */
			  }
			  else if ((dA >= 4 && dB <= 1) || dB <= 0) {
				  cjkBestListInsertFirst(0x5DF1, pSession); /* Open, common */
			  }
			  else {
				  cjkBestListInsertFirst(0x5DF2, pSession); /* Half closed, common */
			  }
		  }
		  else if (nStrokes == 2) {
			  DECUMA_UINT32 y1 = CJK_STROKE_GET_Y(&s1, 1);
			  DECUMA_UINT32 y2 = CJK_GP_GET_Y(cjkStrokeGetMaxYGridpoint(&s1));
			  DECUMA_UINT32 y3 = CJK_STROKE_GET_Y(&s2, 1);
			  DECUMA_INT32 dA = y3 - y1;
			  DECUMA_INT32 dB = y2 - y3;
			  if (dA <= 1 && dB >= 2) {
				  cjkBestListInsertFirst(0x5DF3, pSession); /* Closed, unusal */
			  }
			  else if (dA >= 3 && dB <= 1) {
				  cjkBestListInsertFirst(0x5DF1, pSession); /* Open, common */
			  }
			  else {
				  cjkBestListInsertFirst(0x5DF2, pSession); /* Half closed, common */
			  }
		  }
		  return;
		  /*  */
		  /*  */
		  /* The component 8279 and the character 5EFE are different in height. */
		  /*  */
#ifdef CERTIFICATION
	  case 0x8279:
	  case 0x5EFE:
		  if (nStrokes == 3) {
			  if (5 * (dltCCCompressGetYmax(c) - dltCCCompressGetYmin(c)) < 3 * (dltCCCompressGetXmax(c) - dltCCCompressGetXmin(c))) {
				  cjkBestListInsertFirst(0x8279, pSession); /* Component small */
			  }
			  else {
				  cjkBestListInsertFirst(0x5EFE, pSession); /* Character tall */
			  }
		  }
		  return;
#endif
		  /*  */
		  /* Shujing complained that 8272 sometimes ended up as 9091. Let's */
		  /* fix it. */
		  /*  */
	  case 0x9091: {
		  int x1 = CJK_STROKE_GET_X(&s1, 1);
		  int x2 = CJK_STROKE_GET_X(&s1, 2);
		  int y1 = CJK_STROKE_GET_Y(&s1, 1);
		  int ytop = CJK_GP_GET_Y(cjkStrokeGetMinYGridpoint(&s1));
		  if (nStrokes >= 2) {
			  int ytop2 = CJK_GP_GET_Y(cjkStrokeGetMinYGridpoint(&s2));
			  if (ytop2 < ytop) {
				  ytop = ytop2;
			  }
		  }
		  if (x2 < x1 && y1 == ytop) {
			  cjkBestListInsertFirst(0x8272, pSession);
		  }
				   }
				   return;
				   /*  */
				   /*  */
	  case 0x5140:
	  case 0x5C22:
		  if (nStrokes == 3) {
			  CJK_STROKE *nextlast,*first;
			  DECUMA_INT32 nfirst;
			  nextlast = &s2;
			  first = &s1;
			  nfirst = CJK_STROKE_NPOINTS(first);
			  if (CJK_STROKE_GET_Y(nextlast, 1)
				  < (CJK_STROKE_GET_Y(first, 1)-(CJK_STROKE_GET_Y(first,1)-CJK_STROKE_GET_Y(first,nfirst))/2-1)) {
					  cjkBestListInsertFirst(0x5C22, pSession);
			  }
			  else{
				  cjkBestListInsertFirst(0x5140, pSession);
			  }

		  }
		  return;
		  /*  */
		  /* \f{figure}[h!] */
		  /*  \begin{center} */
		  /*         \begin{tabular}{cccc} */
		  /*                 \epsfig{file=../../figures/53f3.eps} & */
		  /*                 \epsfig{file=../../figures/77f3.eps} */
		  /*         \end{tabular} */
		  /*  \end{center} */
		  /* \f} */
		  /*  */
		  /* Stone. */
		  /*  */
	  case 0x53F3:
	  case 0x77F3:
		  if (nStrokes <= 4 && CJK_STROKE_NPOINTS(&s1) >= 3 &&
			  CJK_STROKE_GET_X(&s1, 1) < 8 && CJK_STROKE_GET_X(&s1, -1) < 8 &&
			  CJK_GP_GET_X(cjkStrokeGetMaxXGridpoint(&s1)) >= 8) {

				  /*----------------------------------------------------- */
				  /* BEGIN CHUNK: 53F3 first two strokes connected */
				  /*  */
				  if (cjkStrokeGetIntersectionCount(&s1, pSession) == 0) {
					  goto insert77F3;
				  }
				  goto insert53F3;
				  /* END CHUNK: 53F3 first two strokes connected */
				  /*----------------------------------------------------- */


		  }

		  /*----------------------------------------------------- */
		  /* BEGIN CHUNK: 53F3 first two strokes not connected */
		  /*  */
		  if (nStrokes > 3) {
			  if (CJK_STROKE_GET_Y(&s1, -1) < 8) {
				  /* First stroke ends in upper half. */
				  if (4 * CJK_STROKE_GET_Y(&s2, 1) <
					  CJK_STROKE_GET_Y(&s1, 1) + 3 * CJK_STROKE_GET_Y(&s1, -1) - 4) {
insert53F3:
						  cjkBestListInsertFirst(0x53F3, pSession);
						  return;
				  }
				  else {
insert77F3:
					  cjkBestListInsertFirst(0x77F3, pSession);
					  return;
				  }
			  }
			  else {
				  /* First stroke ends in lower half. */
				  if (4 * CJK_STROKE_GET_Y(&s1, 1) <
					  CJK_STROKE_GET_Y(&s2, 1) + 3 * CJK_STROKE_GET_Y(&s2, -1) - 4) {
						  goto insert53F3;
				  }
				  else {
					  goto insert77F3;
				  }
			  }
		  }
		  return;
		  /* END CHUNK: 53F3 first two strokes not connected */
		  /*----------------------------------------------------- */


		  /*  */
		  /*  */
		  /* \f{figure}[h!] */
		  /*  \begin{center} */
		  /*    \begin{tabular}{cccc} */
		  /*       \epsfig{file=../../figures/4e8e.eps} & */
		  /*       \epsfig{file=../../figures/5e72.eps} & */
		  /*       \epsfig{file=../../figures/4e8d.eps} */
		  /*    \end{tabular} */
		  /*  \end{center} */
		  /* \f} */
		  /*  */
		  /* Two horizontals on top of a long vertical. */
		  /*  */
CASE_0x5E72:
	  case 0x5E72:
	  case 0x4E8D:
	  case 0x4E8E:
	  case 0x30C6:
	  case 0x30AD:
		  if (nStrokes == 3) {

			  DECUMA_INT32 upstart = 0;
			  DECUMA_INT32 hashook;

			  CJK_ZOOMSTROKE zs1, zs2, zs3;

			  DECUMA_INT32 xmax, xmin, ymax, ymin, y;
			  DECUMA_INT32 i;

			  zm_create_zoom_stroke(c, 0, &zs1);
			  zm_create_zoom_stroke(c, 1, &zs2);
			  zm_create_zoom_stroke(c, 2, &zs3);

			  zm_get_stroke_extent(&zs2, &xmax, &xmin, &ymax, &ymin);

			  y = zs3.pPoints[0].y;
			  for (i = 1; i < zs3.nPoints; i++) {
				  if (zs3.pPoints[i].y > ymax) {
					  DECUMA_INT32 x = (zs3.pPoints[i - 1].x + zs3.pPoints[i].x) / 2;
					  if (xmax - ((xmax - xmin) / 10) < x) {
						  /* 90 % of stroke 2 is to the left of stroke 3; this is probably a U+30F2 */
						  cjkBestListInsertFirst(0x30F2, pSession);
						  return;
					  }
				  }
			  }


			  if (2 * CJK_STROKE_GET_Y(&s3, 1) <= CJK_STROKE_GET_Y(&s2, 1) + CJK_STROKE_GET_Y(&s2, -1) - 2) {
				  upstart = 1;
			  }
			  hashook = dltCCharHasHook(c, 3, &s3);
			  if (upstart) {
				  if (hashook) {
					  cjkBestListInsertFirst(0x4E8E, pSession);
				  }
				  else {
					  DECUMA_INT32 y3min, y3max;
					  zm_get_stroke_extent(&zs3, NULL, NULL, &y3max, &y3min);
					  /* Check if vertical stroke extends above top horizontal stroke */
					  if (20 * y3min < 20 * ymin - (y3max - y3min)) {
						  cjkBestListInsertFirst(0x30AD, pSession);
					  }
					  else {
						  cjkBestListInsertFirst(0x5E72, pSession);
					  }
				  }
			  }
			  else {
				  cjkBestListInsertFirst(0x4E8D, pSession);
			  }
		  }
		  return;

	  case UC_POSTAL_MARK:
		  if (DLTDB_IS_JAPANESE(pSession->db)) { /* Japanese DB */
			  if (nStrokes == 3) {
				  DECUMA_INT32 upstart = 0;
				  DECUMA_INT32 hashook;
				  if (2 * CJK_STROKE_GET_Y(&s3, 1) <=
					  CJK_STROKE_GET_Y(&s2, 1) + CJK_STROKE_GET_Y(&s2, -1) - 2) {
						  upstart = 1;
				  }
				  hashook = dltCCharHasHook(c, 3, &s3);
				  if (upstart) {
					  if (hashook) {
						  cjkBestListInsertFirst(0x4E8E, pSession);
					  }
					  else {
						  cjkBestListInsertFirst(0x5E72, pSession);
					  }
				  }
				  else {
					  /* Special UC_POSTAL_MARK always on second place */
					  cjkBestListInsertFirst(UC_POSTAL_MARK, pSession);
					  cjkBestListInsertFirst(0x30C6, pSession);
				  }
			  }
		  }
		  return;
		  /*  */
	  case 0x7530:
	  case 0x7531:
	  case 0x7532:
	  case 0x7533:
		  if (nStrokes == 5 || nStrokes == 4 || nStrokes == 3) {
			  CJK_STROKE startstroke, s, vertical;
			  DECUMA_INT32 maxvertical = 0;
			  cjkStrokeInit(&vertical);

			  /*----------------------------------------------------- */
			  /* BEGIN CHUNK: find middle verticle stroke */
			  /*  */
			  /* Note that we are guaranteed that the [[vertical]] */
			  /* and [[maxvertical]] will be written the first time */
			  /* since [[maxvertical]] was initialized to -1 above. */
			  /* In an earlier version of the program it was initialized */
			  /* to 0 and due to Murphy's law the program crashed a year */
			  /* after that code was written. */
			  /*  */
			  if (nStrokes == 5) {
				  startstroke = s3;
			  }
			  else {
				  startstroke = s2;
			  }
			  for (s = startstroke; CJK_STROKE_EXISTS(&s); cjkStrokeNext(&s, pSession)) {
				  if (CJK_STROKE_GET_Y(&s, -1) - CJK_STROKE_GET_Y(&s, 1) > maxvertical) {
					  vertical = s;
					  maxvertical = CJK_STROKE_GET_Y(&s, -1) - CJK_STROKE_GET_Y(&s, 1);
				  }
			  }
			  /* END CHUNK: find middle verticle stroke */
			  /*----------------------------------------------------- */


			  if CJK_STROKE_EXISTS(&vertical) {

				  /*----------------------------------------------------- */
				  /* BEGIN CHUNK: analyze middle verticle stroke */
				  /*  */
				  /*  */
				  if (CJK_STROKE_GET_Y(&vertical, 1) < CJK_GP_GET_Y(cjkStrokeGetMinYGridpoint(&s1)) - 1) {
					  if (CJK_STROKE_GET_Y(&vertical, -1) > CJK_GP_GET_Y(cjkStrokeGetMaxYGridpoint(&s1)) + 1) {
						  cjkBestListInsertMany(uclist_7533, pSession);
					  }
					  else {
						  cjkBestListInsertMany(uclist_7531, pSession);
					  }
				  }
				  else {
					  if (CJK_STROKE_GET_Y(&vertical, -1) > CJK_GP_GET_Y(cjkStrokeGetMaxYGridpoint(&s1)) + 1) {
						  cjkBestListInsertMany(uclist_7532, pSession);
					  }
					  else {
						  cjkBestListInsertMany(uclist_7530, pSession);
					  }
				  }
				  /* END CHUNK: analyze middle verticle stroke */
				  /*----------------------------------------------------- */


			  }
		  }
		  return;
		  /*  */
		  /*  */
		  /* \f{figure}[h!] */
		  /*  \begin{center} */
		  /*    \begin{tabular}{cccc} */
		  /*       \epsfig{file=../../figures/4e95.eps} & */
		  /*       \epsfig{file=../../figures/5f00.eps} */
		  /*    \end{tabular} */
		  /*  \end{center} */
		  /* \f} */
		  /*  */
		  /* The ladder confusion group. */
		  /* The symbol [[#]] is not very common. But the chinese/japanese character 0x4E95 */
		  /* is very common. Therefore always put [[#]] in second place. */
		  /*  */
	  case UC_NUMBER_SIGN:
	  case 0x4E95: /* S+T+J */
	  case 0x4E93: /* S+T, not J */
	  case 0x5F00: /* S, not T or J */
		  cjkBestListInsertFirst(0x4E95, pSession);
		  if (nStrokes == 4) {
#if defined(CERTIFICATION)
			  if (pSession->boxheight &&
				  (dltCCCompressGetYmax(c) - dltCCCompressGetYmin(c)) * 7 <= pSession->boxheight * 3 &&
				  (dltCCCompressGetXmax(c) - dltCCCompressGetXmin(c)) * 7 <= pSession->boxwidth * 3) {
					  cjkBestListInsertFirst(UC_NUMBER_SIGN, pSession);
			  }
			  else
#endif
				  if (CJK_STROKE_GET_Y(&s4, 1) < CJK_STROKE_GET_Y(&s1, -1)) {
					  cjkBestListInsertFirst(0x4E95, pSession);
				  }
				  else if ( (DLTDB_IS_SIMP(pSession->db)) && 
					  ( (CJK_STROKE_GET_Y(&s4, 1) < CJK_STROKE_GET_Y(&s2, -1) ||
					  CJK_STROKE_GET_Y(&s3, 1) < CJK_STROKE_GET_Y(&s2, 1)) ) ) { /* Simplified DB */
						  cjkBestListInsertFirst(0x5F00, pSession);
				  }
				  else if ( (DLTDB_IS_SIMP(pSession->db)) || (DLTDB_IS_TRAD(pSession->db)) ) { /* Simplified or Traditional DB */
					  cjkBestListInsertFirst(0x4E93, pSession);
				  }
		  }
		  else if (nStrokes == 1) {
			  DECUMA_INT32 y1 = CJK_STROKE_GET_Y(&s1, 1);
			  DECUMA_INT32 i;
			  DECUMA_INT32 n = CJK_STROKE_NPOINTS(&s1);
			  for (i = n / 2; i <= n; i++) {
				  if (CJK_STROKE_GET_Y(&s1, i) < y1) {
					  cjkBestListInsertFirst(0x4E95, pSession);
					  return;
				  }
			  }
			  if (DLTDB_IS_SIMP(pSession->db)) { /* Simplified DB */
				  cjkBestListInsertFirst(0x5F00, pSession);
			  }
		  }
		  return;
		  /*  */
		  /*  */
		  /* \f{figure}[h!] */
		  /*  \begin{center} */
		  /*    \begin{tabular}{cccc} */
		  /*       \epsfig{file=../../figures/6301.eps} & */
		  /*       \epsfig{file=../../figures/7279.eps} */
		  /*    \end{tabular} */
		  /*  \end{center} */
		  /* \f} */
		  /*  */
		  /* The character for ``special'' should not be confused with ``hold''. */
		  /*  */
	  case 0x6301:
	  case 0x7279:
		  if (CJK_STROKE_NPOINTS(&s1) > 1) {
			  if (CJK_STROKE_GET_X(&s1, 1) < CJK_STROKE_GET_X(&s1, 2)) {
				  cjkBestListInsertFirst(0x6301, pSession);
			  }
			  else {
				  cjkBestListInsertFirst(0x7279, pSession);
			  }
		  }
		  return;
		  /*  */
		  /* The characters 6322 and 6324 are very very similar. */
		  /* Experience shows that the confusion is one way due to */
		  /* the present allographs. */
		  /*  */
	  case 0x6324:
		  if (nStrokes >= 6 &&
			  CJK_STROKE_NPOINTS(&s_fromend[0]) <= 2 &&
			  CJK_STROKE_NPOINTS(&s_fromend[1]) <= 2 &&
			  CJK_STROKE_NPOINTS(&s_fromend[2]) <= 2) {
				  CJK_GRIDPOINT p1 = *cjkStrokeGetGridpoint(&s_fromend[3], -2);
				  CJK_GRIDPOINT p2 = *cjkStrokeGetGridpoint(&s_fromend[3], -1);
				  CJK_GRIDPOINT p3 = *cjkStrokeGetGridpoint(&s_fromend[2], 1);
				  DECUMA_INT32 x2 = CJK_GP_GET_X(p2) - CJK_GP_GET_X(p1);
				  DECUMA_INT32 y2 = CJK_GP_GET_Y(p2) - CJK_GP_GET_Y(p1);
				  DECUMA_INT32 x3 = CJK_GP_GET_X(p3) - CJK_GP_GET_X(p1);
				  DECUMA_INT32 y3 = CJK_GP_GET_Y(p3) - CJK_GP_GET_Y(p1);
				  if (x2 * y3 <= x3 * y2) {
					  cjkBestListInsertFirst(0x6322, pSession);
				  }
		  }
		  return;
		  /*  */
		  /*  */
		  /* Two similar problems are solved analogously. */
		  /* The right part comfusion is the same in both 6865 ending */
		  /* up as 6821 and 5A07 ending up as 59E3. */
		  /*  */
	  case 0x6821:
	  case 0x59E3:
		  if (nStrokes >= 6 &&
			  CJK_STROKE_NPOINTS(&s_fromend[0]) <= 3 &&
			  CJK_STROKE_NPOINTS(&s_fromend[1]) <= 3) {
				  if (CJK_STROKE_GET_X(&s_fromend[0], 1) >= CJK_STROKE_GET_X(&s_fromend[1], 1)) {
					  if (u == 0x6821) {
						  cjkBestListInsertFirst(0x6865, pSession);
					  }
					  else {
						  cjkBestListInsertFirst(0x5A07, pSession);
					  }
				  }
		  }
		  return;
		  /*  */
		  /*  */
		  /* \f{figure}[h!] */
		  /*  \begin{center} */
		  /*    \begin{tabular}{cc} */
		  /*       \epsfig{file=../../figures/80b2.eps} & */
		  /*       \epsfig{file=../../figures/9ad8.eps} \\ */
		  /*       \epsfig{file=../../figures/9999.eps} & */
		  /*       \epsfig{file=../../figures/97f3.eps} */
		  /*    \end{tabular} */
		  /*  \end{center} */
		  /* \f} */
		  /*  */
		  /* The four characters "educate", "tall", "fragrant" and "sound". */
		  /*  */
	  case 0x80B2:
	  case 0x9AD8:
	  case 0x9999:
	  case 0x97F3:
		  if (nStrokes > LATINLIMIT) {
			  if (CJK_STROKE_NPOINTS(&s1) == 2 &&
				  (CJK_STROKE_GET_X(&s1,1) - CJK_STROKE_GET_X(&s1,2) > 0)) {
					  cjkBestListInsertFirst(0x9999, pSession);
			  }
			  else if (nStrokes >= 4 &&
				  CJK_STROKE_NPOINTS(&s1) <= 2 &&
				  CJK_STROKE_NPOINTS(&s2) == 2 &&
				  CJK_STROKE_NPOINTS(&s3) >= 3 &&
				  CJK_STROKE_NPOINTS(&s4) == 2) {
					  cjkBestListInsertFirst(0x80B2, pSession);
			  }
		  }
		  return;
		  /*  */
		  /*  */
		  /* \f{figure}[h!] */
		  /*  \begin{center} */
		  /*    \begin{tabular}{ccc} */
		  /*       \epsfig{file=../../figures/76bf.eps} & */
		  /*       \epsfig{file=../../figures/56db.eps} & */
		  /*       \epsfig{file=../../figures/56da.eps} */
		  /*    \end{tabular} */
		  /*  \end{center} */
		  /* \f} */
		  /*  */
		  /* The characters 'household utensils', 'fourth' and 'prisoner'. */
		  /*  */
	  case 0x76BF:
	  case 0x56DB:
	  case 0x56DA:
		  if (nStrokes == 5) {
			  if (CJK_STROKE_GET_X(&s5,1) >= CJK_STROKE_GET_X(&s1,2) ||
				  CJK_STROKE_GET_X(&s5,-1) <= CJK_STROKE_GET_X(&s2,-1)) {
					  CJK_GRIDPOINT p1 = *cjkStrokeGetGridpoint(&s3, 1);
					  CJK_GRIDPOINT p2 = *cjkStrokeGetGridpoint(&s4, 1);
					  DECUMA_INT32 dx = CJK_GP_GET_X(p2) - CJK_GP_GET_X(p1);
					  DECUMA_INT32 dy = CJK_GP_GET_Y(p2) - CJK_GP_GET_Y(p1);
					  if (dx < 0 || (dy > 0 && dy - dx > 0)) {
						  cjkBestListInsertFirst(0x56DA, pSession);
					  }
					  else {
						  cjkBestListInsertFirst(0x56DB, pSession);
					  }
			  }
			  else {
				  cjkBestListInsertFirst(0x76BF, pSession);
			  }
		  }
		  else if (u == 0x56DB) {
			  CJK_STROKE sa, sb;
			  if (nStrokes == 3) {
				  sa = s2;
				  sb = s3;
				  goto RESTOF_0x56DB;
			  }
			  if (nStrokes == 4) {
				  sa = s3;
				  sb = s4;
RESTOF_0x56DB:
				  if (CJK_STROKE_NPOINTS(&sb) == 2 && CJK_STROKE_NPOINTS(&sa) >= 5) {
					  cjkBestListInsertFirst(0x56DE, pSession);
				  }
			  }
		  }
		  return;
		  /*  */
		  /* \f{figure}[h!] */
		  /*  \begin{center} */
		  /*    \begin{tabular}{cc} */
		  /*       \epsfig{file=../../figures/6613.eps} & */
		  /*       \epsfig{file=../../figures/754c.eps} */
		  /*    \end{tabular} */
		  /*  \end{center} */
		  /* \f} */
		  /*  */
		  /* When Magnus Nordenhake writes "easy", 6613, he gets it wrong without */
		  /* this fix. If stroke 4 starts horizontally and not vertically */
		  /* then it is 6613 instead of 754C. To complicate stuff there is a */
		  /* variant of 754C that connects two verticals. */
		  /*  */
	  case 0x6613:
	  case 0x754C:
		  if (nStrokes == 7 || nStrokes == 8) {
			  if (CJK_STROKE_GET_Y(&s4, 2) - CJK_STROKE_GET_Y(&s4, 1) <
				  CJK_STROKE_GET_X(&s4, 2) - CJK_STROKE_GET_X(&s4, 1) &&
				  CJK_STROKE_GET_Y(&s5, 1) >= CJK_STROKE_GET_Y(&s1, -1)) {
					  cjkBestListInsertFirst(0x6613, pSession);
			  }
		  }
		  return;
		  /*  */
		  /* The following six characters are analysed with respect to their */
		  /* right components. Note that the left component of four of them */
		  /* is analysed in the method [[cjkBestListThreeDropsCheck]]. */
		  /*  */
	  case 0x8A08: case 0x8A02:
		  u_cross = 0x8A08;
		  u_nocross = 0x8A02;
		  goto label_6C41;
	  case 0x8BA1: case 0x8BA2:
		  u_cross = 0x8BA1;
		  u_nocross = 0x8BA2;
		  goto label_6C41;
	  case 0x6C41: case 0x6C40:
		  u_cross = 0x6C41;
		  u_nocross = 0x6C40;
label_6C41:
		  if (nStrokes >= 4) {
			  CJK_STROKE * a = &s_fromend[1];
			  CJK_STROKE * b = &s_fromend[0];
			  DECUMA_INT32 na = CJK_STROKE_NPOINTS(a);
			  DECUMA_INT32 nb = CJK_STROKE_NPOINTS(b);
			  if (2 <= na && na <= 3 && 2 <= nb && nb <= 3) {
				  if (CJK_GP_GET_Y(cjkStrokeGetMinYGridpoint(b)) < CJK_GP_GET_Y(cjkStrokeGetMinYGridpoint(a))) {
					  cjkBestListInsertFirst(u_nocross, pSession);
					  cjkBestListInsertFirst(u_cross, pSession);
				  }
				  else {
					  cjkBestListInsertFirst(u_cross, pSession);
					  cjkBestListInsertFirst(u_nocross, pSession);
				  }
			  }
		  }
		  return;
		  /*  */
		  /*  */
		  /* The character 56DE ``return and 76EE are confused sometimes. */
		  /*  */
CASE_76EE_LE_FOUR: {
		  CJK_STROKE * s = NULL;
		  if (nStrokes == 2) {
			  s = &s2;
		  }
		  else if (nStrokes == 3) {
			  if (CJK_STROKE_NPOINTS(&s2) > CJK_STROKE_NPOINTS(&s3)) {
				  s = &s2;
			  }
			  else {
				  s = &s3;
			  }
		  }
		  else if (nStrokes == 4) {
			  s = &s3;
		  }
		  if (s != NULL && CJK_STROKE_NPOINTS(s) >= 5) {
			  /* if starts downwards or left, and aspect good enough... */
			  DECUMA_INT32 y1 = CJK_STROKE_GET_Y(s, 1);
			  DECUMA_INT32 y2 = CJK_STROKE_GET_Y(s, 2);
			  DECUMA_INT32 y3 = CJK_STROKE_GET_Y(s, 3);
			  DECUMA_INT32 dx = CJK_STROKE_GET_X(s, 2) - CJK_STROKE_GET_X(s, 1);
			  if (y2 - y1 > dx && y3 < y2 &&
				  4*(dltCCCompressGetXmax(c) - dltCCCompressGetXmin(c)) > 3*(dltCCCompressGetYmax(c) - dltCCCompressGetYmin(c))) {
					  cjkBestListInsertFirst(0x56DE, pSession);
					  return;
			  }
		  }
		  if (nStrokes == 1 && CJK_STROKE_NPOINTS(&s1) >= 11) {
			  cjkBestListInsertFirst(0x56DE, pSession);
		  }
		  if (dltCCCompressGetNbrPoints(c) <= 9 &&
			  6 * (dltCCCompressGetXmax(c) - dltCCCompressGetXmin(c)) > 7 * (dltCCCompressGetYmax(c) - dltCCCompressGetYmin(c))) {
				  cjkBestListInsertFirst(0x5668, pSession);
		  }
				   }
				   return;
				   /*  */
				   /*  */
				   /* \f{figure}[h!] */
				   /*  \begin{center} */
				   /*    \begin{tabular}{cc} */
				   /*       \epsfig{file=../../figures/5143.eps} & */
				   /*       \epsfig{file=../../figures/65e0.eps} */
				   /*    \end{tabular} */
				   /*  \end{center} */
				   /* \f} */
				   /*  */
	  case 0x5143:
	  case 0x65E0:
		  if (dltCCharGetIntersectionCount(c, pSession) > 0) {
			  cjkBestListInsertFirst(0x65E0, pSession);
		  }
		  else {
			  cjkBestListInsertFirst(0x5143, pSession);
		  }
		  return;
		  /*  */
		  /* The difference is the crossing line at the top. */
		  /*  */
	  case 0x5929:
	  case 0x592B:
		  if (nStrokes == 4) {
			  if ((CJK_STROKE_GET_Y(&s3,1) < CJK_STROKE_GET_Y(&s1,1) - 1) &&
				  (CJK_STROKE_GET_Y(&s3,1) < CJK_STROKE_GET_Y(&s1,-1) - 1)) {
					  cjkBestListInsertMany(uclist_592B, pSession);
			  }
			  else {
				  cjkBestListInsertMany(uclist_5929, pSession);
			  }
		  }
		  return;
		  /*  */
		  /* Shujing complained about a cursive form of */
		  /* 5806 that was interpreted as 63A8. Let's fix it. */
		  /*  */
	  case 0x63A8: {
		  DECUMA_INT32 n1 = CJK_STROKE_NPOINTS(&s1);
		  if (5 <= n1 && n1 <= 7 && cjkStrokeGetIntersectionCount(&s1, pSession) == 1) {
			  cjkBestListInsertFirst(0x5806, pSession);
		  }
				   }
				   return;
				   /*  */
				   /* Shujing complained about a cursive form of */
				   /* 73CD that was interpreted as 8BCA. Let's fix it. */
				   /*  */
	  case 0x8BCA: {
		  if (CJK_STROKE_NPOINTS(&s1) >= 5) {
			  cjkBestListInsertFirst(0x73CD, pSession);
		  }
				   }
				   return;
				   /*  */
				   /* \f{figure}[h!] */
				   /*  \begin{center} */
				   /*    \begin{tabular}{ccc} */
				   /*       \epsfig{file=../../figures/5411.eps} & */
				   /*       \epsfig{file=../../figures/95ee.eps} & */
				   /*       \epsfig{file=../../figures/4f55.eps} */
				   /*    \end{tabular} */
				   /*  \end{center} */
				   /* \f} */
				   /*  */
				   /* The characters "direction", "asking" and "what". */
				   /*  */
	  case 0x5411:
	  case 0x95EE:
	  case 0x4F55:
		  if (nStrokes > LATINLIMIT) {
			  if ((CJK_STROKE_NPOINTS(&s1) == 2) && (CJK_STROKE_NPOINTS(&s2) == 2)) {
				  /* The first two strokes not semicursive */
				  if (CJK_STROKE_GET_Y(&s1,-1) > 8) { /* 95EE  or 4F55 */
					  if (CJK_STROKE_GET_Y(&s2,-1) < 8) {
						  cjkBestListInsertFirst(0x95EE, pSession);
					  }
					  else {
						  cjkBestListInsertFirst(0x4F55, pSession);
					  }
				  }
				  else if (CJK_STROKE_GET_X(&s1,1) > CJK_STROKE_GET_X(&s1,-1)) { /*5411 or 4F55 */
					  if (CJK_STROKE_NPOINTS(&s3) > 2) {
						  cjkBestListInsertFirst(0x5411, pSession);
					  }
					  else if (CJK_STROKE_NPOINTS(&s3) == 2) {
						  cjkBestListInsertFirst(0x4F55, pSession);
					  }
				  }
				  else {
					  cjkBestListInsertFirst(0x95EE, pSession);
				  }
			  }
			  else if (CJK_STROKE_NPOINTS(&s1) == 3) { /*captures all cursive forms of 4F55 */
				  if (CJK_STROKE_GET_X(&s2,-1) > 8) {
					  if (CJK_STROKE_NPOINTS(&s2) == 2) { /*not cursive */
						  cjkBestListInsertFirst(0x4F55, pSession);
					  }
					  else if (CJK_STROKE_NPOINTS(&s2) == 3) {
						  cjkBestListInsertFirst(0x5411, pSession);
					  }
					  else if (CJK_STROKE_NPOINTS(&s2) == 4) {
						  cjkBestListInsertFirst(0x4F55, pSession);
					  }
				  }
				  else { /*sloppily written 4F55 */
					  cjkBestListInsertFirst(0x4F55, pSession);
				  }

			  }
			  else if ((CJK_STROKE_NPOINTS(&s1) == 2) && (CJK_STROKE_NPOINTS(&s2) > 3) &&
				  (CJK_STROKE_GET_X(&s2,-1) > 6)) { /* Cursive first and second 5411 */
					  cjkBestListInsertFirst(0x5411, pSession);
			  }
		  }
		  return;
		  /*  */
		  /* \f{figure}[h!] */
		  /*  \begin{center} */
		  /*    \begin{tabular}{ccc} */
		  /*       \epsfig{file=../../figures/5348.eps} & */
		  /*       \epsfig{file=../../figures/725b.eps} */
		  /*    \end{tabular} */
		  /*  \end{center} */
		  /* \f} */
		  /*  */
		  /* The two characters "cattle"(735B) and "noon"(5348). */
		  /*  */
	  case 0x5348:
CASE_0x5348_ANY:
		  if (nStrokes >= 4) {
			  if (CJK_STROKE_GET_Y(&s3,1) >= CJK_STROKE_GET_Y(&s2,1) &&
				  CJK_STROKE_GET_Y(&s4,1) >= CJK_STROKE_GET_Y(&s2,1)) {
					  cjkBestListInsertFirst(0x5348, pSession);
			  }
			  else {
				  cjkBestListInsertFirst(0x725B, pSession);
			  }
		  }
		  return;
		  /*  */
		  /*  */
		  /* \f{figure}[h!] */
		  /*  \begin{center} */
		  /*    \begin{tabular}{ccc} */
		  /*       \epsfig{file=../../figures/81ea.eps} & */
		  /*       \epsfig{file=../../figures/76ee.eps} & */
		  /*       \epsfig{file=../../figures/4e14.eps} */
		  /*    \end{tabular} */
		  /*  \end{center} */
		  /* \f} */
		  /*  */
		  /* When writing "self" (81EA) with the first two strokes connected, the engine */
		  /* always gives the answer "eye" (76EE). */
		  /* If the first stroke has more than two points or the first point on the */
		  /* first stroke is more than two points above the first point of the second */
		  /* stroke (this limit is tested by Ran Tau) the answer */
		  /* should be "self". Also 4E14 has to be involved as well as 65E6 and */
		  /* 95EB. */
		  /*  */
	  case 0x76EE:
	  case 0x4E14:
	  case 0x65E6:
	  case 0x81EA:
	  case 0x95EB:
		  if (nStrokes == 5) {
			  if (CJK_STROKE_GET_Y(&s1, 1) < CJK_STROKE_GET_Y(&s2, 1) - 2) {
				  cjkBestListInsertFirst(0x81EA, pSession);
			  }
			  else if (CJK_STROKE_NPOINTS(&s1) == 2) {
				  if (CJK_STROKE_GET_Y(&s4, 1) + CJK_STROKE_GET_Y(&s4, -1) >=
					  CJK_STROKE_GET_Y(&s1, -1) + CJK_STROKE_GET_Y(&s2, -1) - 2) {
						  cjkBestListInsertFirst(0x65E6, pSession);
				  }
				  else if (CJK_STROKE_GET_X(&s5, 1) >= CJK_STROKE_GET_X(&s1, 2) - 1 ||
					  CJK_STROKE_GET_X(&s5, -1) <= CJK_STROKE_GET_X(&s2, -1) + 1) {
						  cjkBestListInsertFirst(0x76EE, pSession);
				  }
				  else {
					  cjkBestListInsertFirst(0x4E14, pSession);
				  }
			  }
		  }
		  else if (nStrokes == 6 && CJK_STROKE_NPOINTS(&s1) <= 2) {
			  if (CJK_STROKE_GET_Y(&s1, -1) < 8 &&
				  CJK_STROKE_GET_Y(&s1, -1) >= CJK_STROKE_GET_Y(&s1, 1) &&
				  CJK_STROKE_GET_X(&s1, -1) >= CJK_STROKE_GET_X(&s1, 1)) {
					  cjkBestListInsertFirst(0x95EB, pSession);
			  }
		  }
		  else if (nStrokes <= 4 && u == 0x76EE) {
			  goto CASE_76EE_LE_FOUR;
		  }
		  return;


		  /**
		  * 
		  * \f{figure}[h!]
		  *  \begin{center}
		  *    \begin{tabular}{ccc}
		  *       \epsfig{file=../../figures/65e5.eps} &
		  *       \epsfig{file=../../figures/66f0.eps}
		  *    \end{tabular}
		  *  \end{center}
		  * \f}
		  */
	  case 0x65E5:
	  case 0x66F0:
		  if (nStrokes == 1) {
			  cjkBestListInsertFirst(0x65E5, pSession);
		  }
		  else {
			  if ((dltCCCompressGetYmax(c) - dltCCCompressGetYmin(c)) > (dltCCCompressGetXmax(c) - dltCCCompressGetXmin(c))) {
				  cjkBestListInsertFirst(0x65E5, pSession);
			  }
			  else {
				  cjkBestListInsertFirst(0x66F0, pSession);
			  }
		  }
		  return;
		  /*  */
		  /* Hiragana 3051 is easily comfused with latin H, when second stroke is */
		  /* horizontal. In case the horizontal stroke's last point is to the right of the */
		  /* rightmost vertical stroke, it is the hiragana character. If it is to the left */
		  /* it is latin H. If they have the same x position the first point of the */
		  /* vertical stroke is investigated. If it is a large gap to the leftmost vertical */
		  /* stroke, it is the hiragana 3051, otherwise it is latin H. */
		  /*  */
	  case 0x0048:
	  case 0x3051:
		  if (nStrokes == 3) {
			  if (ABS(CJK_STROKE_GET_Y(&s2, 1) - CJK_STROKE_GET_Y(&s2, -1)) < 4) {
				  DECUMA_INT32 leftmost, rightmost;
				  leftmost = ABS(CJK_STROKE_GET_X(&s1, -1) - CJK_STROKE_GET_X(&s1, 1)) / 2 +
					  MIN(CJK_STROKE_GET_X(&s1, -1), CJK_STROKE_GET_X(&s1, 1));
				  rightmost = ABS(CJK_STROKE_GET_X(&s3, -1) - CJK_STROKE_GET_X(&s3, 1)) / 2 +
					  MIN(CJK_STROKE_GET_X(&s3, -1), CJK_STROKE_GET_X(&s3, 1));

				  if (CJK_STROKE_GET_X(&s2, -1) > (rightmost + 1)) {
					  cjkBestListInsertFirst(0x3051, pSession);
				  }
				  else if (CJK_STROKE_GET_X(&s2, -1) < rightmost) {
					  cjkBestListInsertFirst(0x0048, pSession);
				  }
				  else {
					  if ((CJK_STROKE_GET_X(&s2, 1) < (leftmost + 3)) &&
						  (CJK_STROKE_GET_X(&s2, -1) < (rightmost + 1))) {
							  cjkBestListInsertFirst(0x0048, pSession);
					  }
					  else if (CJK_STROKE_GET_X(&s2, 1) < (leftmost + 1)) {
						  cjkBestListInsertFirst(0x0048, pSession);
					  }
					  else {
						  cjkBestListInsertFirst(0x3051, pSession);
					  }
				  }
			  }
		  }
		  return;
		  /*  */
		  /*  */
		  /* A three stroke F should not be recognized as a 4E0B. */
		  /*  */
	  case 0x4E0B:
		  if (nStrokes == 3 &&
			  CJK_STROKE_GET_Y(&s3, -1) <= CJK_STROKE_GET_Y(&s3, 1) &&
			  CJK_STROKE_GET_X(&s2, 2) <= CJK_STROKE_GET_X(&s1, 1) + 2) {
				  cjkBestListInsertFirst(0x0046, pSession);
		  }
		  return;
		  /*  */
		  /*  */
		  /* The character ``middle 4E2D should be on first place */
		  /* when written in one stroke and first ink is rightwards. */
		  /* The character 725B is sometimes confused with this one. */
		  /* There is another chunk that checks for confusion to 5348 */
		  /* for at least 4 strokes. This chunk checks for up to three */
		  /* strokes. */
		  /*  */
	  case 0x725B: {
		  DECUMA_INT32 x11 = CJK_STROKE_GET_X(&s1, 1);
		  if (nStrokes >= 4) {
			  goto CASE_0x5348_ANY;
		  }
		  if (CJK_STROKE_GET_X(&s1, 2) - x11 >= 0 &&
			  CJK_STROKE_GET_X(&s1, 3) - x11 >= 0) {
				  cjkBestListInsertFirst(0x4E2D, pSession);
		  }
				   }
				   return;
				   /*  */
				   /* \f{figure}[ht!] */
				   /*  \begin{center} */
				   /*         \begin{tabular}{ccc} */
				   /*                 {\Huge \textbf{Y \ y}} and & */
				   /*                 \epsfig{file=../../figures/4e2b.eps} */
				   /*         \end{tabular} */
				   /*  \end{center} */
				   /* \f} */
				   /*  */
	  case 0x0059:
	  case 0x0079:
	  case 0x4E2B:
		  if (nStrokes == 2 && CJK_STROKE_NPOINTS(&s2) > CJK_STROKE_NPOINTS(&s1)) {
			  if (CJK_STROKE_GET_X(&s2, -1) <= CJK_STROKE_GET_X(&s1, 1)) {
				  cjkBestListInsertFirst(0x0079, pSession); /* 'y' */
				  return;
			  }
		  }
		  if (nStrokes >= 2) {
			  if (cjkContextHasPrevious(con) && decumaIsLatin(cjkContextGetPrevious(con))) {
				  cjkBestListInsertFirst(0x0059, pSession); /* 'Y' */
			  }
			  else {
				  cjkBestListInsertFirst(0x4E2B, pSession);
			  }
		  }
		  return;
		  /*  */
		  /* The only thing that differs between 4F58 and 4F59 is if the */
		  /* vertical starts above the horizontal before it. */
		  /*  */
	  case 0x4F59:
	  case 0x4F58:
		  {
			  CJK_STROKE  horisontal, vertical;
			  if (nStrokes == 7) {
				  horisontal = s4;
				  vertical = s5;
				  goto case4F59body;
			  }
			  else if (nStrokes == 6) {
				  horisontal = s3;
				  vertical = s4;
case4F59body:
				  if (2 * CJK_STROKE_GET_Y(&vertical, 1) >= CJK_STROKE_GET_Y(&horisontal, 1)
					  + CJK_STROKE_GET_Y(&horisontal,-1)) {
						  cjkBestListInsertFirst(0x4F58, pSession);
				  }
				  else {
					  cjkBestListInsertFirst(0x4F59, pSession);
				  }
			  }
			  else if (nStrokes == 5 && CJK_STROKE_NPOINTS(&s5) >= 4) {
				  horisontal = s4;
				  vertical = s5;
				  goto case4F59body;
			  }
			  else if (DLTDB_IS_SIMP(pSession->db)) { /* Simplified DB */
				  goto CASE_0x4F1A_ANY;
			  }
		  }
		  return;
		  /*  */
		  /*  */
		  /* \f{figure}[ht!] */
		  /*  \begin{center} */
		  /*         \begin{tabular}{ccc} */
		  /*                 \epsfig{file=../../figures/30c1.eps}  & */
		  /*                 \epsfig{file=../../figures/30c6.eps}  & */
		  /*                 \epsfig{file=../../figures/5343.eps} */
		  /*         \end{tabular} */
		  /*  \end{center} */
		  /* \f} */
		  /*  */
		  /* 0x30C1 and 30C6 is katakana and 0x5343 is Kanji. They can be written with */
		  /* both two and three strokes. Four -- 0034 -- is also involved. */
		  /*  */
	  case 0x5343:
	  case 0x30C1:
		  if (nStrokes == 3) {
			  if (CJK_STROKE_GET_X(&s1, 1) < CJK_STROKE_GET_X(&s1, -1)) {
				  goto CASE_0x5E72;
			  }
			  if (decumaIsHan(cjkContextGetPrevious(con))) {
				  cjkBestListInsertFirst(0x5343, pSession);
			  }
			  else {
				  cjkBestListInsertFirst(0x30C1, pSession);
			  }
		  }
		  else if (nStrokes == 2) {
			  if (CJK_STROKE_GET_X(&s1, 1) <= CJK_STROKE_GET_X(&s2, 1) ||
				  decumaIsDigit(cjkContextGetPrevious(con))) {
					  cjkBestListInsertFirst(0x0034, pSession);
			  }
			  else if (CJK_STROKE_GET_X(&s2, -1) + 1 < CJK_GP_GET_X(cjkStrokeGetMaxXGridpoint(&s2)) ||
				  !decumaIsHan(cjkContextGetPrevious(con))) {
					  cjkBestListInsertFirst(0x30C1, pSession);
			  }
			  else {
				  cjkBestListInsertFirst(0x5343, pSession);
			  }
		  }
		  return;
		  /*  */
		  /* The character 0x5C38 looks like a P. The character 0x5362 has one */
		  /* short stroke over it and 0x6237 has two short strokes over it. */
		  /*  */
	  case 0x5C38:
	  case 0x6237:
	  case 0x5362:
		  if (nStrokes == 2 && CJK_STROKE_NPOINTS(&s2) <= 3) {
			  DECUMA_INT32 x1 = CJK_STROKE_GET_X(&s1, 1);
			  DECUMA_INT32 x2 = CJK_STROKE_GET_X(&s1, 2);
			  DECUMA_INT32 y21 = CJK_STROKE_GET_Y(&s2, 1);
			  CJK_GRIDPOINT gxmin = cjkStrokeGetMinXGridpoint(&s1);
			  if (x2 < x1 ||
				  (CJK_STROKE_NPOINTS(&s1) >= 6 &&
				  CJK_GP_GET_X(gxmin) < x1 &&
				  CJK_GP_GET_Y(gxmin) <= y21)) {
					  cjkBestListInsertFirst(0x6237, pSession);
			  }
			  else {
				  cjkBestListInsertFirst(0x5C38, pSession);
			  }
		  }
		  if (nStrokes >= 3) {
			  DECUMA_INT32 x11 = CJK_STROKE_GET_X(&s1, 1);
			  DECUMA_INT32 x12 = CJK_STROKE_GET_X(&s1, 2);
			  DECUMA_INT32 x13 = CJK_STROKE_GET_X(&s1, 3);
			  DECUMA_INT32 x21 = CJK_STROKE_GET_X(&s2, 1);
			  if (CJK_STROKE_NPOINTS(&s1) == 2) {
				  if (x21 >= x11) {
					  if (CJK_STROKE_NPOINTS(&s2) >= 5) {
						  cjkBestListInsertFirst(0x5362, pSession);
					  }
				  }
				  else if (CJK_STROKE_NPOINTS(&s2) >= 3) {
					  cjkBestListInsertFirst(0x6237, pSession);
				  }
			  }
			  else if (CJK_STROKE_NPOINTS(&s1) == 3 && x12 <= x11 && CJK_STROKE_NPOINTS(&s2) >= 4) {
				  if (x12 <= x11 && x13 > x11) {
					  cjkBestListInsertFirst(0x5362, pSession);
				  }
			  }
		  }
		  return;
		  /*  */
		  /*  */
	  case 0x4E08:
	  case 0x5927:
		  if (nStrokes == 3) {
			  if (dltCCharGetIntersectionCount(c, pSession) < 2 &&
				  CJK_STROKE_GET_X(&s3, 1) >= CJK_STROKE_GET_X(&s2, 1) - 2) {
					  cjkBestListInsertFirst(0x5927, pSession);
			  }
			  else {
				  cjkBestListInsertFirst(0x4E08, pSession);
			  }
		  }
		  return;
		  /*  */
		  /*  */
	  case 0x4E38:
	  case 0x51E1: {
		  if (nStrokes == 3) {
			  CJK_STROKE first, second;
			  DECUMA_INT32 npoint;
			  first = s1;
			  second = s2;
			  /*Right stroke order */
			  npoint = CJK_STROKE_NPOINTS(&first);
			  if (CJK_STROKE_GET_X(&first,npoint) < 8) {
				  if (CJK_STROKE_GET_X(&first,1) > (CJK_STROKE_GET_X(&second,1) +1)) {
					  cjkBestListInsertFirst(0x4E38, pSession);
				  }
				  else {
					  cjkBestListInsertFirst(0x51E1, pSession);
				  }
			  }
			  else { /*incorrect stroke order */
				  if (CJK_STROKE_GET_X(&second,1) > (CJK_STROKE_GET_X(&first,1) +1)) {
					  cjkBestListInsertFirst(0x4E38, pSession);
				  }
				  else {
					  cjkBestListInsertFirst(0x51E1, pSession);
				  }
			  }
		  }
				   }
				   return;
				   /*  */
	  case 0x633D:
	  case 0x6362: {
		  CJK_STROKE last;
		  dltCCharGetLastStroke(c, &last, pSession);
		  if (CJK_STROKE_NPOINTS(&last)<=2) {
			  cjkBestListInsertFirst(0x6362, pSession);
		  }
		  else
		  {
			  cjkBestListInsertFirst(0x633D, pSession);
		  }
				   }
				   return;
				   /*  */
	  case 0x5151:
	  case 0x514D: {
		  if (nStrokes > 6) {
			  if (nStrokes == 8) {
				  cjkBestListInsertFirst(0x514D, pSession);
			  }
			  else {
				  CJK_STROKE nexttolast = s2;
				  CJK_STROKE beforenexttolast = s1;
				  DECUMA_INT32 k = 3;
				  DECUMA_INT32 nPoints;
				  while (k < nStrokes) {
					  beforenexttolast = nexttolast;
					  cjkStrokeNext(&nexttolast,pSession);
					  k++;
				  }
				  nPoints = CJK_STROKE_NPOINTS(&beforenexttolast);
				  if (CJK_STROKE_GET_Y(&nexttolast,1) >= CJK_STROKE_GET_Y(&beforenexttolast,nPoints)) {
					  cjkBestListInsertFirst(0x5151, pSession);
				  }
				  else {
					  cjkBestListInsertFirst(0x514D, pSession);
				  }
			  }
		  }
				   }
				   return;
				   /*  */
				   /*  */
				   /* \f{figure}[ht!] */
				   /*  \begin{center} */
				   /*   \epsfig{file=../../figures/52ab.eps} */
				   /*  \end{center} */
				   /* \f} */
				   /*  */
	  case 0x52AB:
	  case 0x5374:
		  if (nStrokes >= 3) {
			  if (ABS(CJK_STROKE_GET_Y(&s3,1) - CJK_STROKE_GET_Y(&s3,2)) < 8) {
				  if (ABS(CJK_STROKE_GET_Y(&s1,1) - CJK_STROKE_GET_Y(&s1,2)) <
					  ABS(CJK_STROKE_GET_X(&s1,1) - CJK_STROKE_GET_X(&s1,2)) &&
					  ABS(CJK_STROKE_GET_Y(&s2,1) - CJK_STROKE_GET_Y(&s2,2)) <
					  ABS(CJK_STROKE_GET_X(&s2,1) - CJK_STROKE_GET_X(&s2,2))) {
						  /* the first two are horizontal. */
						  cjkBestListInsertFirst(0x52A8, pSession);
				  }
			  }
		  }
		  return;
		  /*  */
		  /*  */
		  /* \f{figure}[ht!] */
		  /*  \begin{center} */
		  /*   \epsfig{file=../../figures/6587.eps} */
		  /*  \end{center} */
		  /* \f} */
		  /*  */
		  /*  */
	  case 0x6587:
	  case 0x4E4B:
	  case 0x4E91:
		  if (nStrokes == 1 && u != 0x4E91) {
			  if (cjkStrokeGetIntersectionCount(&s1, pSession) == 1) {
				  cjkBestListInsertMany(uclist_6587, pSession);
			  }
			  else if (cjkStrokeGetIntersectionCount(&s1, pSession) == 0) {
				  cjkBestListInsertMany(uclist_4E4B, pSession);
			  }
		  }
		  else if (nStrokes == 2) {
			  if (categorymask & (CJK_HIRAGANA | CJK_HIRAGANASMALL)) {
				  cjkBestListInsertFirst(0x3048, pSession);
			  }
			  else {
				  if (cjkStrokeGetIntersectionCount(&s2, pSession) == 1) {
					  cjkBestListInsertMany(uclist_6587, pSession);
				  }
				  else if (cjkStrokeGetIntersectionCount(&s2, pSession) == 0) {
					  if (CJK_STROKE_GET_Y(&s2, -1) > CJK_STROKE_GET_Y(&s2, -3) + 2) {
						  cjkBestListInsertFirst(0x4E91, pSession);
					  }
					  else {
						  cjkBestListInsertMany(uclist_4E4B, pSession);
					  }
				  }
			  }
		  }
		  return;
		  /*  */
		  /* If a cursive 4E3B ends downwards it is probably a 6587. */
		  /*  */
	  case 0x4E3B: {
		  if (CJK_STROKE_NPOINTS(&s1) <= 3 && ((nStrokes == 2) ||
			  (nStrokes == 3 && CJK_STROKE_NPOINTS(&s2) <= 3))) {
				  DECUMA_INT32 x2 = CJK_STROKE_GET_X(&s_fromend[0], -1);
				  DECUMA_INT32 y2 = CJK_STROKE_GET_Y(&s_fromend[0], -1);
				  DECUMA_INT32 x1 = CJK_STROKE_GET_X(&s_fromend[0], -2);
				  DECUMA_INT32 y1 = CJK_STROKE_GET_Y(&s_fromend[0], -2);
				  if (dltCCCompressIsDense(c)) {
					  x1 = CJK_STROKE_GET_X(&s_fromend[0], -3);
					  y1 = CJK_STROKE_GET_Y(&s_fromend[0], -3);
				  }
				  if (y2 - y1 > 0 && x2 - x1 > 0) {
					  cjkBestListInsertFirst(0x6587, pSession);
				  }
		  }
				   }
				   /*  */
				   /* CJK_CONTEXT for 7 (0037), loo (chinese 4E86) and the almost identical */
				   /* twostroke katakana character 30AF. */
				   /* Also a onestroke loo or a 3 should mean a hiragana 308D or */
				   /* possibly hiragana 305D. */
				   /*  */
	  case 0x0037: case 0x0033: case 0x4E86: case 0x30AF:
	  case 0x305D: case 0x308B: case 0x308D:
		  if (nStrokes == 1) {
CASE_0x4E87_ONE:

			  /*----------------------------------------------------- */
			  /* BEGIN CHUNK: check 0037 onestroke */
			  /*  */
			  if (u == 0x4E86 && decumaIsDigit(cjkContextGetPrevious(con))) {
				  cjkBestListDiminish(0x4E86, pSession); /*probably 3 or 7 */
			  }
			  else if (u == 0x4E86 || u == 0x0033 || u == 0x308D) {
CASE_0x0033:
				  cjkBestListInsertFirst(0x0033, pSession);
				  cjkBestListInsertFirst(0x4E86, pSession);
				  if ( (DLTDB_IS_JAPANESE(pSession->db)) && (categorymask & CJK_HIRAGANA) ) {
					  cjkBestListInsertFirst(0x308D, pSession); /* hiragana ro. */
				  }
				  if (decumaIsDigit(cjkContextGetPrevious(con))) {
					  cjkBestListInsertFirst(0x0033, pSession);
				  }
			  }
			  else if (u == 0x0037 && (categorymask & CJK_HIRAGANA) &&
				  CJK_STROKE_GET_X(&s1, -1) > CJK_STROKE_GET_X(&s1, -2)) {
					  cjkBestListInsertFirst(0x3066, pSession);
			  }
			  else if ((u == 0x0037) && decumaIsHan(cjkContextGetPrevious(con))) {
				  cjkBestListInsertFirst(0x4E86, pSession); /* loo */
			  }
			  else if (u == 0x305D || u == 0x308B) {
				  if (CJK_STROKE_GET_X(&s1, -1) < CJK_STROKE_GET_X(&s1, -2)) {
					  goto CASE_0x0033;
				  }
			  }
			  else if (u == UC_RIGHT_PARENTHESIS ||
				  u == UC_RIGHT_SQUARE_BRACKET) {
					  DECUMA_INT32 i;
					  CJK_GRIDPOINT xi;
					  for (i = 2; i < CJK_STROKE_NPOINTS(&s1); i++) {
						  /* Check for left corner like in '3' */
						  xi = CJK_STROKE_GET_X(&s1, i);
						  if (CJK_STROKE_GET_X(&s1, i - 1) > xi && CJK_STROKE_GET_X(&s1, i + 1) > xi) {
							  goto CASE_0x0033;
						  }
					  }
			  }
			  /* END CHUNK: check 0037 onestroke */
			  /*----------------------------------------------------- */


		  }
		  else if (nStrokes == 2) {

			  /*----------------------------------------------------- */
			  /* BEGIN CHUNK: check 0037 twostrokes */
			  /*  */
			  if (u == 0x4E86 && CJK_STROKE_GET_X(&s1, -1) > CJK_STROKE_GET_X(&s1, -2)) {
				  cjkBestListInsertFirst(0x4E01, pSession);
			  }
			  else if ((DLTDB_IS_JAPANESE(pSession->db)) && cjkContextHasPrevious(con)) {
				  if (u == 0x30AF && decumaIsDigit(cjkContextGetPrevious(con))) {
					  cjkBestListInsertFirst(0x0037, pSession); /* 7 */
				  }
				  else if (u == 0x0037) {
					  cjkBestListInsertFirst(0x30AF, pSession);
				  }
			  }
			  /* END CHUNK: check 0037 twostrokes */
			  /*----------------------------------------------------- */


		  }
		  return;
		  /*  */
CASE_0x30A8_ANY:
		  if (nStrokes == 3) {
			  if (CJK_STROKE_GET_X(&s1, -1) >= CJK_STROKE_GET_X(&s1, -2) + 2) {
				  if (decumaIsKatakanaAnySize(cjkContextGetPrevious(con))) {
					  cjkBestListInsertMany(uclist_30A8_3st, pSession);
				  }
				  else {
					  cjkBestListInsertMany(uclist_5DE5_3st, pSession);
				  }
			  }
			  else {
				  cjkBestListInsertMany(uclist_30F1_3st, pSession);
			  }
		  }
		  return;
		  /*  */
		  /*  */
		  /* The katakana 30E5 and 30E6 are often interpreted as 30F1. If written */
		  /* with two strokes, demote the less common 30F1. */
	  case 0x30F1: {
		  if (nStrokes == 2) {
			  cjkBestListDiminish(0x30F1, pSession);
		  }
				   }
				   return;
				   /*  */
				   /*  */
				   /* Katkana 30C8 looks just like kanji 535C. Kanji is not common => always */
				   /* put katakana on first place. */
				   /*  */
	  case 0x535C:
		  cjkBestListInsertFirst(0x30C8, pSession);
		  return;
		  /*  */
		  /*  */
		  /* A threestroked 30B1 where stroke 3 starts well above stroke 2 must */
		  /* be a 3051. Likewise for 30B2 and 3052. */
		  /*  */
	  case 0x30B1:
	  case 0x30B2:
		  if (nStrokes >= 3) {
			  if (CJK_STROKE_GET_Y(&s3, 1) < CJK_STROKE_GET_Y(&s2, -1)) {
				  if (u == 0x30B1 && nStrokes == 3) {
					  cjkBestListInsertFirst(0x3051, pSession);
				  }
				  else if (u == 0x30B2 && nStrokes == 5) {
					  cjkBestListInsertFirst(0x3052, pSession);
				  }
			  }
		  }
		  return;
		  /*  */
		  /*  */
		  /* If 6E16 is on the first place and if the top right stroke has more */
		  /* than two points then we replace it with the more common 6D8C. */
		  /*  */
	  case 0x6E16:
		  if (nStrokes >= 8) {
			  CJK_STROKE s = s4;
			  if (CJK_STROKE_GET_Y(&s3, 1) < CJK_STROKE_GET_Y(&s1, 1)) {
				  s = s3;
			  }
			  if (CJK_STROKE_NPOINTS(&s) > 2) {
				  cjkBestListInsertFirst(0x6D8C, pSession);
			  }
		  }
		  return;
		  /*  */
		  /*  */
		  /* If 6CEA is on the first place and if the last stroke is */
		  /* long then it must be 6CAE (or possibly 8BC5). */
		  /* We pick the two bottom points of the verticals in */
		  /* the right part and then compare with the length of the last stroke. */
		  /*  */
	  case 0x6CEA:
		  if (nStrokes >= 4) {
			  if (dltCCompressGetNotThreeDrops(c)) {
				  cjkBestListInsertFirst(0x8BC5, pSession);
				  return;
			  }
		  }
		  if (nStrokes >= 7) {
			  DECUMA_INT32 x[2], n = 0;
			  CJK_STROKE s, s_last;
			  for(dltCCharGetFirstStroke(c, &s, pSession);
				  CJK_STROKE_EXISTS(&s);
				  cjkStrokeNext(&s, pSession)) {
					  s_last = s;
					  if (CJK_STROKE_GET_Y(&s, -2) < 6 && CJK_STROKE_GET_Y(&s, -1) > 9) {
						  if (n == 2) {
							  n = 0;
							  break;
						  }
						  x[n++] = CJK_STROKE_GET_X(&s, -1);
					  }
			  }
			  if (n == 2 && CJK_STROKE_NPOINTS(&s_last) == 2 &&
				  CJK_STROKE_GET_X(&s_last, 1) < x[0] && CJK_STROKE_GET_X(&s_last, 2) > x[1]) {
					  cjkBestListInsertFirst(0x6CAE, pSession);
			  }
		  }
		  return;
		  /*  */
	  case 0x8C9D:
	  case 0x898B: {
		  DECUMA_INT32 i;
		  CJK_STROKE s;
		  dltCCharGetFirstStroke(c, &s, pSession);
		  for (i = 1; i < nStrokes; i++){
			  cjkStrokeNext(&s, pSession);
		  }
		  /*s is now the last stroke. */
		  if (CJK_STROKE_GET_Y(&s,-1) <= CJK_STROKE_GET_Y(&s,-2)){
			  cjkBestListInsertFirst(0x898B, pSession);
		  }
		  else {
			  cjkBestListInsertFirst(0x8C9D, pSession);
		  }
				   }
				   return;
				   /*  */
				   /*  */
				   /* Two kanji that are (well... was!) mixed up. */
				   /*  */
	  case 0x5360:
	  case 0x53E4: {
		  if (nStrokes >= 3){
			  DECUMA_INT32 hor_x, vert_x1, vert_x2;
			  if (ABS( CJK_STROKE_GET_X(&s1, 1) - CJK_STROKE_GET_X(&s1, -1)) >
				  ABS( CJK_STROKE_GET_X(&s2, 1) - CJK_STROKE_GET_X(&s2, -1)) ){
					  /* s1 horizontal */
					  hor_x = CJK_STROKE_GET_X(&s1, 1);
					  vert_x1 = CJK_STROKE_GET_X(&s2, 1);
					  vert_x2 = CJK_STROKE_GET_X(&s2, -1);
			  }
			  else {
				  /* s2 horizontal */
				  hor_x = CJK_STROKE_GET_X(&s2, 1);
				  vert_x1 = CJK_STROKE_GET_X(&s1, 1);
				  vert_x2 = CJK_STROKE_GET_X(&s1, -1);
			  }
			  if (2 * hor_x >= vert_x1 + vert_x2 - 2){
				  cjkBestListInsertFirst(0x5360, pSession);
			  }
			  else {
				  cjkBestListInsertFirst(0x53E4, pSession);
			  }
		  }
				   }
				   return;
				   /*  */
	  case 0x53E8:
	  case 0x53FB: {
		  if (nStrokes >= 3) {
			  CJK_STROKE s_last, s_lastbutone;
			  DECUMA_INT32 i;

			  /*find the two last strokes */
			  dltCCharGetFirstStroke(c, &s_lastbutone, pSession);
			  for (i = 1; i < nStrokes - 1; i++) {
				  cjkStrokeNext(&s_lastbutone, pSession);
			  }
			  s_last = s_lastbutone;
			  cjkStrokeNext(&s_last, pSession);

			  /*find out what stroke is the vertical. */
			  if (CJK_STROKE_GET_X(&s_lastbutone, -1) < CJK_STROKE_GET_X(&s_last, -1)) {
				  CJK_STROKE tmp;
				  tmp = s_lastbutone; s_lastbutone = s_last; s_last = tmp;
			  }

			  /*decide! */
			  if (CJK_STROKE_GET_Y(&s_lastbutone, 1) + CJK_STROKE_GET_Y(&s_lastbutone, 2) <
				  (CJK_STROKE_GET_Y(&s_last, 1) + 1) * 2) {
					  cjkBestListInsertFirst(0x53E8, pSession);
			  }
			  else {
				  cjkBestListInsertFirst(0x53FB, pSession);
			  }
		  }
				   }
				   return;
				   /*  */
	  case 0x4EEC:
	  case 0x4ED9:
		  if (nStrokes == 1) {
			  cjkBestListInsertFirst(0x4EEC, pSession);
		  }
		  else if (nStrokes < 5) { /*at least one part cursive */
			  if (((CJK_STROKE_GET_Y(&s1,-1) - CJK_STROKE_GET_Y(&s1,1)) > 6) && (CJK_STROKE_NPOINTS(&s1) < 6)) {
				  if ((nStrokes == 3) && ((CJK_STROKE_GET_X(&s2,-1) - CJK_STROKE_GET_X(&s2,1)) > 5)) {
					  cjkBestListInsertFirst(0x4EEC, pSession);
				  }
				  else if (nStrokes == 3) { /*possibly point first in 4EEC */
					  if (CJK_STROKE_GET_Y(&s2,-1) < 7) {
						  cjkBestListInsertFirst(0x4EEC, pSession);
					  }
					  else { /*4EEC can be written without point */
						  if ((CJK_STROKE_GET_Y(&s3,2) - CJK_STROKE_GET_Y(&s3,1)) > 2) {
							  cjkBestListInsertFirst(0x4ED9, pSession);
						  }
						  else {
							  cjkBestListInsertFirst(0x4EEC, pSession);
						  }
					  }
				  }
				  else if ((nStrokes == 2) && (CJK_STROKE_NPOINTS(&s2) > 3)) {
					  if ((CJK_STROKE_GET_X(&s2,4) - CJK_STROKE_GET_X(&s2,1)) > 1) {
						  cjkBestListInsertFirst(0x4EEC, pSession);
					  }
					  else {
						  cjkBestListInsertFirst(0x4ED9, pSession);
					  }
				  }
			  }
			  else if (((CJK_STROKE_GET_Y(&s1,-1) - CJK_STROKE_GET_Y(&s1,1)) > 6) && (CJK_STROKE_GET_X(&s1,-1) < 8)) {
				  DECUMA_INT32 nextlastpoint;
				  /* possibly left human with 6 points or special 4EEC */
				  nextlastpoint = CJK_STROKE_NPOINTS(&s1) - 1;
				  if ((CJK_STROKE_GET_Y(&s1,-1) - CJK_STROKE_GET_Y(&s1,nextlastpoint)) < 3) { /*cursive human */
					  if (((CJK_STROKE_GET_Y(&s2,-1) - CJK_STROKE_GET_Y(&s2,1)) > 5) && (nStrokes > 2)) {
						  if ((CJK_STROKE_GET_Y(&s3,2) - CJK_STROKE_GET_Y(&s3,1)) > 2) {
							  if (CJK_STROKE_GET_Y(&s3,-1) < 7) {
								  cjkBestListInsertFirst(0x4EEC, pSession);
							  }
							  else {
								  cjkBestListInsertFirst(0x4ED9, pSession);
							  }
						  }
						  else {
							  cjkBestListInsertFirst(0x4EEC, pSession);
						  }
					  }
					  else if ((nStrokes == 2) && (CJK_STROKE_NPOINTS(&s2) > 3)) {
						  if ((CJK_STROKE_GET_X(&s2,4) - CJK_STROKE_GET_X(&s2,1)) > 1) {
							  cjkBestListInsertFirst(0x4EEC, pSession);
						  }
						  else {
							  cjkBestListInsertFirst(0x4ED9, pSession);
						  }
					  }
				  }
				  else { /*special 4EEC allograph */
					  cjkBestListInsertFirst(0x4EEC, pSession);
				  }
			  }
			  else if (nStrokes == 4) { /*human noncursive one other cursive part */
				  if ((CJK_STROKE_GET_Y(&s3,-1) - CJK_STROKE_GET_Y(&s3,1)) > 5) {
					  if (CJK_STROKE_GET_Y(&s4,1) < 7) {
						  cjkBestListInsertFirst(0x4EEC, pSession);
					  }
					  else {
						  cjkBestListInsertFirst(0x4ED9, pSession);
					  }
				  }
				  else {
					  cjkBestListInsertFirst(0x4EEC, pSession);
				  }
			  }
		  }
		  return;
		  /**
		   * \f{figure}[h!]
		   *  \begin{center}
		   *    \begin{tabular}{ccc}
		   *       \epsfig{file=../../figures/59D0.eps} &
		   *       \epsfig{file=../../figures/59B2.eps}
		   *    \end{tabular}
		   *  \end{center}
		   * \f}
		   * 
		   * 59D0 and 59B2 look very similar but 59D0 is much more common.
		   */
	  case 0x59B2: {
		  DECUMA_INT32 y_max1, y_max2, j, this_stroke_nr;
		  DECUMA_INT32 y_max3, y_max4;
		  CJK_STROKE this_stroke;
		  y_max2 = -1;
		  if ((CJK_STROKE_NPOINTS(&s1) < 4) && (nStrokes > 1)) { /*first stroke noncursive */
			  if ((CJK_STROKE_NPOINTS(&s2) < 5) && (nStrokes > 2)) { /*second */
				  if ((CJK_STROKE_NPOINTS(&s3) < 5) && (nStrokes > 3)) { /*third */

					  /*----------------------------------------------------- */
					  /* BEGIN CHUNK: get 4 largest y-values of remaining strokes */
					  /*  */
					  /* Go through the  rest of the points to check if there are max y-points close to */
					  /* the y-value of last two points */
					  /*  */
					  j = 1;
					  y_max1 = CJK_STROKE_GET_Y(&s4,j);
					  while ((j < CJK_STROKE_NPOINTS(&s4)) && (CJK_STROKE_GET_Y(&s4,j+1) > y_max1)) {
						  j++;
						  y_max1 = CJK_STROKE_GET_Y(&s4,j);
					  }
					  j++;
					  this_stroke = s4;
					  this_stroke_nr = 4;
					  while (this_stroke_nr < nStrokes) {

						  /*----------------------------------------------------- */
						  /* BEGIN CHUNK: get maxpoint on this stroke */
						  /*  */
						  /* Get the largest y-value in this stroke. */
						  /*  */
						  while (j <= CJK_STROKE_NPOINTS(&this_stroke)) {
							  if (CJK_STROKE_GET_Y(&this_stroke,j) > y_max2) {
								  y_max2 = CJK_STROKE_GET_Y(&this_stroke,j);
							  }
							  j++;
						  }
						  /* END CHUNK: get maxpoint on this stroke */
						  /*----------------------------------------------------- */


						  cjkStrokeNext(&this_stroke,pSession);
						  this_stroke_nr++;
						  j = 1;
					  }

					  /*----------------------------------------------------- */
					  /* BEGIN CHUNK: getpoints as if last stroke */
					  /*  */
					  /* Get the largest value (except for the last two) */
					  /*  */
					  while (j <= (CJK_STROKE_NPOINTS(&this_stroke) - 2)) {
						  if (CJK_STROKE_GET_Y(&this_stroke,j) > y_max2) {
							  y_max2 = CJK_STROKE_GET_Y(&this_stroke,j);
						  }
						  j++;
					  }
					  /* END CHUNK: getpoints as if last stroke */
					  /*----------------------------------------------------- */


					  y_max3 = CJK_STROKE_GET_Y(&this_stroke,j);
					  y_max4 = CJK_STROKE_GET_Y(&this_stroke,-1); /*last point */
					  /* END CHUNK: get 4 largest y-values of remaining strokes */
					  /*----------------------------------------------------- */


					  if (((y_max3 + y_max4) - (y_max1 + y_max2)) < 5) {
						  cjkBestListInsertFirst(0x59D0, pSession);
					  }
				  }
			  }
		  }
				   }
				   return;
	}
}
