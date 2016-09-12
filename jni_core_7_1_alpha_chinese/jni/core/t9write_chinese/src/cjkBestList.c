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
#include "cjkDatabase.h"
#include "cjkSession_Types.h"
#include "cjkDatabaseFormat_Macros.h" /* DLTDB_GET_COMPONENT_DATA */
#include "decumaUnicodeMacros.h"
#include "decumaInterrupts.h"


/* --------------------------------------------------------------------------
 * Data definition, internal linkage
 * -------------------------------------------------------------------------- */

static const CJK_UNICHAR uclist_punctinit[] = {
	UC_COMMA,
	UC_IDEOGRAPHIC_FULL_STOP,
	UC_IDEOGRAPHIC_COMMA,
	UC_QUOTATION_MARK,
	UC_RIGHT_DOUBLE_QUOTATION_MARK,
	UC_LEFT_DOUBLE_QUOTATION_MARK,
	UC_FULL_STOP,
	UC_COLON,
	UC_SEMICOLON,
	UC_APOSTROPHE,
	UC_LEFT_SINGLE_QUOTATION_MARK,
	UC_RIGHT_SINGLE_QUOTATION_MARK,
	0
};

static const CJK_UNICHAR uclist_ldquot[] = {
	UC_LEFT_DOUBLE_QUOTATION_MARK,
	UC_RIGHT_DOUBLE_QUOTATION_MARK,
	UC_QUOTATION_MARK,
	UC_SEMICOLON,
	UC_COLON,
	0
};

static const CJK_UNICHAR uclist_rdquot[] = {
	UC_RIGHT_DOUBLE_QUOTATION_MARK,
	UC_LEFT_DOUBLE_QUOTATION_MARK,
	UC_QUOTATION_MARK,
	UC_SEMICOLON,
	UC_COLON,
	0
};

static const CJK_UNICHAR uclist_colon[] = {
	UC_COLON,
	UC_SEMICOLON,
	UC_QUOTATION_MARK,
	UC_RIGHT_DOUBLE_QUOTATION_MARK,
	UC_LEFT_DOUBLE_QUOTATION_MARK,
	0
};

static const CJK_UNICHAR uclist_semicolon[] = {
	UC_SEMICOLON,
	UC_COLON,
	UC_QUOTATION_MARK,
	UC_RIGHT_DOUBLE_QUOTATION_MARK,
	UC_LEFT_DOUBLE_QUOTATION_MARK,
	0
};

static const CJK_UNICHAR uclist_lsquot[] = {
	UC_GRAVE_ACCENT,
	UC_APOSTROPHE,
	UC_LEFT_SINGLE_QUOTATION_MARK,
	UC_RIGHT_SINGLE_QUOTATION_MARK,
	UC_DIACRITIC_HANDAKUTEN,
	UC_COMMA,
	UC_IDEOGRAPHIC_COMMA,
	UC_FULL_STOP,
	0
};

static const CJK_UNICHAR uclist_rsquot[] = {
	UC_APOSTROPHE,
	UC_GRAVE_ACCENT,
	UC_RIGHT_SINGLE_QUOTATION_MARK,
	UC_LEFT_SINGLE_QUOTATION_MARK,
	UC_DIACRITIC_HANDAKUTEN,
	UC_COMMA,
	UC_IDEOGRAPHIC_COMMA,
	UC_FULL_STOP,
	0
};

static const CJK_UNICHAR uclist_fullstop[] = {
	UC_FULL_STOP,
	UC_MIDDLE_DOT,
	UC_BULLET,
	UC_IDEOGRAPHIC_COMMA,
	UC_COMMA,
	UC_IDEOGRAPHIC_FULL_STOP,
	UC_APOSTROPHE,
	UC_LEFT_SINGLE_QUOTATION_MARK,
	UC_RIGHT_SINGLE_QUOTATION_MARK,
	0
};

static const CJK_UNICHAR uclist_circumflex[] = {
	UC_CIRCUMFLEX_ACCENT,
	UC_QUOTATION_MARK,
	UC_APOSTROPHE,
	UC_GRAVE_ACCENT,
	UC_COMMA,
	UC_IDEOGRAPHIC_COMMA,
	UC_FULL_STOP,
	0
};

static const CJK_UNICHAR uclist_middledot[] = {
	UC_MIDDLE_DOT,
	UC_BULLET,
	UC_FULL_STOP,
	UC_IDEOGRAPHIC_COMMA,
	UC_COMMA,
	UC_IDEOGRAPHIC_FULL_STOP,
	UC_LEFT_SINGLE_QUOTATION_MARK,
	UC_RIGHT_SINGLE_QUOTATION_MARK,
	0
};

static const CJK_UNICHAR uclist_ideofullstop[] = {
	UC_IDEOGRAPHIC_FULL_STOP,
	UC_FULL_STOP,
	UC_COMMA,
	UC_MIDDLE_DOT,
	UC_BULLET,
	UC_IDEOGRAPHIC_COMMA,
	UC_LATIN_SMALL_LETTER_O,
	UC_APOSTROPHE,
	UC_LEFT_SINGLE_QUOTATION_MARK,
	UC_RIGHT_SINGLE_QUOTATION_MARK,
	0
};

static const CJK_UNICHAR uclist_ideocomma[] = {
	UC_IDEOGRAPHIC_COMMA,
	UC_COMMA,
	UC_IDEOGRAPHIC_FULL_STOP,
	UC_FULL_STOP,
	UC_MIDDLE_DOT,
	UC_BULLET,
	UC_APOSTROPHE,
	UC_LEFT_SINGLE_QUOTATION_MARK,
	UC_RIGHT_SINGLE_QUOTATION_MARK,
	0
};

static const CJK_UNICHAR uclist_fwcomma[] = {
	UC_COMMA,
	UC_IDEOGRAPHIC_COMMA,
	UC_IDEOGRAPHIC_FULL_STOP,
	UC_FULL_STOP,
	UC_MIDDLE_DOT,
	UC_BULLET,
	UC_APOSTROPHE,
	UC_LEFT_SINGLE_QUOTATION_MARK,
	UC_RIGHT_SINGLE_QUOTATION_MARK,
	0
};


static const CJK_UNICHAR uclist_quot[] = {
	UC_QUOTATION_MARK,
	UC_LEFT_DOUBLE_QUOTATION_MARK,
	UC_RIGHT_DOUBLE_QUOTATION_MARK,
	0
};

static const CJK_UNICHAR uclist_dots[] = {
	UC_MIDDLE_DOT,
	UC_BULLET,
	UC_FULL_STOP,
	0
};


static const CJK_UNICHAR uclist_J[] = {
	0x004A,
	0x006A,
	0
};

static const CJK_UNICHAR uclist_j[] = {
	0x006A,
	0x004A,
	0
};


static const CJK_UNICHAR uclist_004B[] = {
	0x004B,
	0x006B,
	0
};
static const CJK_UNICHAR uclist_006B[] = {
	0x006B,
	0x004B,
	0
};

static const CJK_UNICHAR uclist_0070[] = {
	0x0070,
	0x0050,
	0
};

static const CJK_UNICHAR uclist_0050[] = {
	0x0050,
	0x0070,
	0
};

static const CJK_UNICHAR uclist_0079[] = {
	0x0079,
	0x0059,
	0
};

static const CJK_UNICHAR uclist_0059[] = {
	0x0059,
	0x0079,
	0
};

static const CJK_UNICHAR speak3drops[] = {
	0x8BDD, 0x6D3B, 0x8BA1, 0x6C41, 0x8BF7, 0x6E05, 0x8BBA, 0x6CA6,
	0x8BBE, 0x6CA1, 0x8BED, 0x6D6F, 0x8C08, 0x6DE1, 0x8BFB, 0x6E0E,
	0x8C01, 0x6DEE, 0x8BA2, 0x6C40, 0x8C13, 0x6E2D, 0x8BD1, 0x6CFD,
	0x8BAF, 0x6C5B, 0x8BE2, 0x6D35, 0x8BF8, 0x6E1A, 0x8BE6, 0x6D0B,
	0x8BDE, 0x6D8E, 0x8C2D, 0x6F6D, 0x8BF5, 0x6D8C, 0x8BAB, 0x6C54,
	0x8BC5, 0x6CAE, 0x8BDB, 0x6D19, 0x8BF2, 0x6D77, 0x8C0D, 0x6E2B,
	0x8C24, 0x6EC2, 0x8C29, 0x6F2B, 0x8C30, 0x6F9C, 0x8BA6, 0x6C57,
	0x8BA7, 0x6C5F, 0x8BAA, 0x6C55, 0x8BB4, 0x6CA4, 0x8BC2, 0x6CBD,
	0x8BC3, 0x6CB3, 0x8BCF, 0x6CBC, 0x8BD2, 0x6CBB, 0x8BD6, 0x6D3C,
	0x8BD8, 0x6D01, 0x8BDC, 0x6D17, 0x8BE8, 0x6D51, 0x8BEE, 0x6D88,
	0x8BF0, 0x6D69, 0x8BFC, 0x6DBF, 0x8C0C, 0x6E5B, 0x8C12, 0x6E34,
	0x8C15, 0x6E1D, 0x8C1F, 0x6F20, 0x8C25, 0x6EA2, 0x8C2A, 0x6EF4,
	0
};

/* --------------------------------------------------------------------------
 * Internal functions
 * -------------------------------------------------------------------------- */

static DECUMA_UINT32 ucIsInCatmask(CJK_UNICHAR u, CJK_SESSION * pSession);

static CJK_BOOLEAN ucIsAllowed(CJK_UNICHAR u, CJK_SESSION *pSession);

/* --------------------------------------------------------------------------
 * Exported functions
 * -------------------------------------------------------------------------- */


void cjkBestListInit(CJK_BESTLIST * const pBestlist)
{
	DECUMA_INT32 j;
	for (j = 0; j <= BESTLEN; j++) {
		pBestlist->index[j] = 0;
		pBestlist->unichar[j] = 0;
		pBestlist->dist[j] = MAXDIST;
		pBestlist->allowdupuc = 1;
	}
}


DECUMA_INT32  cjkBestListGetLength(CJK_SESSION * pSession)
{
	CJK_BESTLIST * bl = &pSession->db_lookup_bl;
	DECUMA_INT32 len = BESTLEN;
	DECUMA_INT32 j;

	for (j = 0; j <= BESTLEN; j++) {
		if (bl->unichar[j] == 0) {
			len = j;
		}
	}
	return len;
}



void cjkBestListInsert(CJK_BESTLIST * const bl, CJK_UNICHAR unichar, DLTDB_INDEX index, CJK_DISTANCE const dist, CJK_SESSION const * const pSession)
{
	DECUMA_INT32  j;

	if (DLTDB_IS_SIMP_TRAD(pSession->db)) { /* SimpTrad DB */
		unichar = cjkDbUCHan2Han(unichar, pSession);
	}

	if (dist >= bl->dist[BESTLEN - 1]) {
		return;
	}
	if (bl->allowdupuc) {

		/*----------------------------------------------------- */
		/* start at end of bl */
		/*  */
		/* if we allow duplicate unicodes we start at end of bestlist so that */
		/* the chunk [[<<sweep back and insert>>]] will work in the next step. */
		j = BESTLEN;


	}
	else {

		/*----------------------------------------------------- */
		/* sweep forward to match or till end */
		/*  */
		/* We sweep forward through the list until we reach the end or until we */
		/* find that [[unichar]] is already in the list. */
		/* If [[unichar]] is already in the list we replace its distance if it is larger */
		/* than the new distance [[dist]] and we exit if it is not. */

		j = 0;
		while (j < BESTLEN && bl->unichar[j] != unichar) {
			j++;
		}
		if (bl->unichar[j] == unichar) {
			if (dist < bl->dist[j]) {
				bl->dist[j] = dist;
				if (index == 0) {
					index = bl->index[j];
				}
			}
			else {
				return;
			}
		}


	}

	/*----------------------------------------------------- */
	/* sweep back and insert */
	/*  */
	/* Now, we are at the end of the list or we are at a position where we already */
	/* had found the [[unicode]]. */
	/* In either case we sweep backwards to insert the unicode, index, and distance */
	/* into the correct location in the list. We first make room by forcing a number */
	/* of candidates one step up the ladder. Then we write the present candidate to */
	/* the correct slot. Finally we write the teminating zero character to */
	/* the last slot since it could have been overwritten. */

	while (j > 0 && dist <= bl->dist[j - 1]) {
		bl->dist[j] = bl->dist[j - 1];
		bl->unichar[j] = bl->unichar[j - 1];
		bl->index[j] = bl->index[j - 1];
		j--;
	}
	bl->dist[j] = dist;
	bl->unichar[j] = unichar;
	bl->index[j] = index;
	bl->dist[BESTLEN] = MAXDIST;
	bl->unichar[BESTLEN] = 0;
	bl->index[BESTLEN] = 0;
}



void cjkBestListInsertAt(CJK_UNICHAR unichar, DLTDB_INDEX index, CJK_BESTLIST_POSITION pos, CJK_SESSION * pSession)
{
	CJK_BESTLIST * bl = &pSession->db_lookup_bl;
	DECUMA_INT32 j, unichar_at_pos;
	CJK_DISTANCE dist;

	decumaAssert(unichar != 0);

	if (DLTDB_IS_SIMP_TRAD(pSession->db)) { /* SimpTrad DB */
		unichar = cjkDbUCHan2Han(unichar, pSession);
	}

	if (pSession->state == STATE_SPECIAL_CHECKS) {
		/* Currently ignore personal categories for special checks */
		if (!ucIsInCatmask(unichar, pSession)) {
			return;
		}
		if (!ucIsAllowed(unichar, pSession)) {
			return;
		}
	}

	unichar_at_pos = -1;
	if ((DECUMA_INT32) pos >= cjkBestListGetLength(pSession)) {
		pos = (CJK_BESTLIST_POSITION) cjkBestListGetLength(pSession);
	}

	/*----------------------------------------------------- */
	/* sweep forward to check if already in list */
	/*  */
	/*  */
	/* We sweep forward through the list until we reach the end or until we */
	/* find that [[unichar]] is already in the list. */
	/*  */
	for (j = 0; j < BESTLEN; j++) {
		if (bl->unichar[j] == unichar) {
			unichar_at_pos = j;
			break;
		}
	}


	if (unichar_at_pos >= 0) {
		if ((CJK_BESTLIST_POSITION) unichar_at_pos == pos) {
			return;
		}
		else {

			/*----------------------------------------------------- */
			/* pick index out and replace other elements */
			/*  */
			/* If [[unichar]] is found it is deleted from the list, and all other */
			/* elements in the list are moved one step to fill in the vacancy. */
			/*  */
			for (j = unichar_at_pos; j < (BESTLEN - 1); j++) {
				bl->dist[j] = bl->dist[j + 1];
				bl->unichar[j] = bl->unichar[j + 1];
				bl->index[j] = bl->index[j + 1];
			}
			bl->dist[BESTLEN - 1] = MAXDIST;
			bl->unichar[BESTLEN - 1] = 0;
			bl->index[BESTLEN - 1] = 0;



			/*----------------------------------------------------- */
			/* put index into list at the input position */
			/*  */
			/* The new element is put into the list at the input position. */
			/* The bl-$>$dist[pos] is set to the same distance as the following */
			/* element, in order to update the distance order in the list. */
			for (j = (BESTLEN - 1); (CJK_BESTLIST_POSITION) j > pos; j--) {
				bl->dist[j] = bl->dist[j - 1];
				bl->unichar[j] = bl->unichar[j - 1];
				bl->index[j] = bl->index[j - 1];
			}
			if (pos == 0) {
				decumaAssert(BESTLEN > 1);
				dist = bl->dist[pos+1];
			}
			else {
				dist = bl->dist[pos - 1];
			}
			bl->dist[pos] = dist;
			bl->unichar[pos] = unichar;
			bl->index[pos] = index;
		}
	}
	else {

		/*----------------------------------------------------- */
		/* put index into list at the input position */
		/*  */
		/* The new element is put into the list at the input position. */
		/* The bl-$>$dist[pos] is set to the same distance as the following */
		/* element, in order to update the distance order in the list. */
		for (j = (BESTLEN - 1); j > (DECUMA_INT32) pos; j--) {
			bl->dist[j] = bl->dist[j - 1];
			bl->unichar[j] = bl->unichar[j - 1];
			bl->index[j] = bl->index[j - 1];
		}
		if (pos == 0) {
			decumaAssert(BESTLEN > 1);
			dist = bl->dist[pos+1];
		}
		else {
			dist = bl->dist[pos - 1];
		}
		bl->dist[pos] = dist;
		bl->unichar[pos] = unichar;
		bl->index[pos] = index;
	}
}


void cjkBestListInsertFirst(CJK_UNICHAR unichar, CJK_SESSION * pSession)
{
	cjkBestListInsertAt(unichar, 0, 0, pSession);
	return;
}


void cjkBestListReplace(CJK_UNICHAR oldunichar, CJK_UNICHAR unichar, CJK_SESSION * pSession) {
	CJK_BESTLIST * bl = &pSession->db_lookup_bl;
	DECUMA_INT32 j, newpos, unichar_at_pos;
	newpos = -1;
	unichar_at_pos = -1;
	for (j = 0; j<BESTLEN; j++) {
		if (bl->unichar[j] == oldunichar) {
			newpos = j;
		}
	}
	if (newpos >= 0) {

		/*----------------------------------------------------- */
		/* sweep forward to check if already in list */
		/*  */
		/* We sweep forward through the list until we reach the end or until we */
		/* find that [[unichar]] is already in the list. */
		for (j = 0; j < BESTLEN; j++) {
			if (bl->unichar[j] == unichar) {
				unichar_at_pos = j;
				break;
			}
		}


		if (unichar_at_pos >= 0) {
			if (newpos < unichar_at_pos) { /*remove duplicate at unichar_at_pos */


				/*----------------------------------------------------- */
				/* pick index out and replace other elements */
				/*  */
				/* If [[unichar]] is found it is deleted from the list, and all other */
				/* elements in the list are moved one step to fill in the vacancy. */
				for (j = unichar_at_pos; j < (BESTLEN - 1); j++) {
					bl->dist[j] = bl->dist[j + 1];
					bl->unichar[j] = bl->unichar[j + 1];
					bl->index[j] = bl->index[j + 1];
				}
				bl->dist[BESTLEN - 1] = MAXDIST;
				bl->unichar[BESTLEN - 1] = 0;
				bl->index[BESTLEN - 1] = 0;


				bl->unichar[newpos] = unichar;
			}
			else {
				unichar_at_pos = newpos;

				/*----------------------------------------------------- */
				/* pick index out and replace other elements */
				/*  */
				/* If [[unichar]] is found it is deleted from the list, and all other */
				/* elements in the list are moved one step to fill in the vacancy. */
				for (j = unichar_at_pos; j < (BESTLEN - 1); j++) {
					bl->dist[j] = bl->dist[j + 1];
					bl->unichar[j] = bl->unichar[j + 1];
					bl->index[j] = bl->index[j + 1];
				}
				bl->dist[BESTLEN - 1] = MAXDIST;
				bl->unichar[BESTLEN - 1] = 0;
				bl->index[BESTLEN - 1] = 0;


			}
		}
		else {
			bl->unichar[newpos] = unichar;
		}
	}
}




void cjkBestListRemove(CJK_BESTLIST * bl, int index)
{
	int k;

	for (k=index; k < BESTLEN-1; k++) {
		bl->unichar[k] = bl->unichar[k+1];
		bl->dist[k] = bl->dist[k+1];
		bl->index[k] = bl->index[k+1];
		if (bl->unichar[k] == 0) {
			return;
		}
	}
	/* If for-loop is broken before then a nul-element */
	/* has already been shifted left. Otherwise we make */
	/* sure to shift in a nul element at the end. */
	bl->unichar[BESTLEN-1] = 0;
}																		



void cjkBestListRemoveGestures(CJK_BESTLIST * bl)
{
	int j, nNextEmptyIdx;

	for (j = 1, nNextEmptyIdx=1; j<BESTLEN && bl->unichar[j]; j++) {
		if (decumaIsGesture(bl->unichar[j])) {
			bl->unichar[j] = 0;
			continue;
		}
		else if (nNextEmptyIdx < j) {
			bl->unichar[nNextEmptyIdx] = bl->unichar[j];
			bl->dist[nNextEmptyIdx] = bl->dist[j];
			bl->index[nNextEmptyIdx] = bl->index[j];
			bl->unichar[j] = 0;
		}
		nNextEmptyIdx++;
	}
}




void cjkBestListInsertMany(const CJK_UNICHAR * unicharlist, CJK_SESSION * pSession)
{
	DECUMA_INT32 j = 0;
	CJK_UNICHAR unichar;

	unichar = unicharlist[j];
	while (unichar != 0) {
		j++;
		unichar = unicharlist[j];
	}
	while (j > 0) {
		j--;
		unichar = unicharlist[j];
		cjkBestListInsertFirst(unichar, pSession);
	}
}


void cjkBestListBoost(CJK_UNICHAR * uclist, CJK_BESTLIST_POSITION pos, CJK_SESSION * pSession)
{
	CJK_BESTLIST * bl = &pSession->db_lookup_bl;
	CJK_BESTLIST_POSITION insert = pos;
	int u, i;

	for (i=0; bl->unichar[i] && i < BESTLEN; i++) {
		for (u=0; uclist[u]; u++) {
			if (bl->unichar[i] == uclist[u]) {
				cjkBestListInsertAt(bl->unichar[i], bl->index[i], insert, pSession);
				insert++;
				break;
			}
		}
	}
}


void cjkBestListDiminish(CJK_UNICHAR unichar, CJK_SESSION * pSession)
{
	CJK_BESTLIST * bl = &pSession->db_lookup_bl;
	CJK_BESTLIST_POSITION i, pos;

	pos = cjkBestListGetPosition(unichar, pSession);
	if (pos == 0) {
		i = pos + 1;
		while (bl->unichar[i] && i < BESTLEN) {
			decumaAssert(i > 0);
			cjkBestListInsertAt(bl->unichar[i], bl->index[i],(CJK_BESTLIST_POSITION) (i - 1), pSession);
			i++;
		}
#if BESTLEN > 3
		cjkBestListInsertAt(unichar, 0, 3, pSession);
#else 
		cjkBestListInsertAt(unichar, 0, BESTLEN - 1, pSession);
#endif
	}
}



void cjkBestListSwap(CJK_SESSION * pSession, CJK_BESTLIST_POSITION pos1, CJK_BESTLIST_POSITION pos2)
{

	CJK_BESTLIST * bl = &pSession->db_lookup_bl;
	DECUMA_INT32 bl_len;
	DLTDB_INDEX tempindex;
	CJK_DISTANCE tempdist;
	CJK_UNICHAR tempunichar;

	decumaAssert(bl);
	bl_len = cjkBestListGetLength(pSession);
	if ( (DECUMA_INT32) pos1 >= bl_len || (DECUMA_INT32) pos2 >= bl_len ) {
		decumaAssert(0);
		return;
	}

	/*Swapping */
	tempindex   = bl->index[pos1];
	tempdist    = bl->dist[pos1];
	tempunichar = bl->unichar[pos1];

	bl->index[pos1]   = bl->index[pos2];
	bl->dist[pos1]    = bl->dist[pos2];
	bl->unichar[pos1] = bl->unichar[pos2];

	bl->index[pos2]   = tempindex;
	bl->dist[pos2]    = tempdist;
	bl->unichar[pos2] = tempunichar;

	return;
}


CJK_BESTLIST_POSITION cjkBestListGetPosition(CJK_UNICHAR unichar, CJK_SESSION * pSession)
{
	CJK_BESTLIST * bl = &pSession->db_lookup_bl;
	CJK_BESTLIST_POSITION i;
	for (i = 0; i < (BESTLEN - 1); i++) {
		if (bl->unichar[i] == unichar) {
			return i;
		}
	}
	return (CJK_BESTLIST_POSITION)-1; /* Cast to avoid some compiler warnings */
}



void cjkBestListFixFrequencies(CJK_SESSION * pSession)
{
	CJK_BESTLIST * bl = &pSession->db_lookup_bl;
	DECUMA_INT32  j;
	CJK_DISTANCE dist = MAXDIST;

	decumaAssert(bl);
	decumaAssert(pSession);
	for (j = 0; bl->unichar[j] != 0 && j < BESTLEN; j++) {
		if (bl->dist[j] < MAXDIST - LARGEDIST)
			bl->dist[j] = bl->dist[j] + LARGEDIST;
		else 
			bl->dist[j] = MAXDIST;
	}
	for (j = 0; bl->unichar[j] != 0 && j < BESTLEN; j++) {
		if (bl->dist[j] < MAXDIST) {
			dist =  bl->dist[j] - LARGEDIST;
			dist = (75 * dist +
				dist * (15 - (DLTDB_GET_FREQUENCY(&pSession->db, bl->index[j])))) / 75;
		}
		cjkBestListInsert(bl, bl->unichar[j], bl->index[j], dist, pSession);
	}
}

/**
	This function sets the information given to the element at pos in bestlist pBl.

*/

void cjkBestListSetElement(CJK_BESTLIST * pBl, CJK_UNICHAR uc, DLTDB_INDEX index, CJK_DISTANCE dist, CJK_BESTLIST_POSITION pos)
{
	decumaAssert(pos < BESTLEN);
	pBl->unichar[pos] = uc;
	pBl->index[pos] = index;
	pBl->dist[pos] = dist;
}

/**
*	This function uses a more expensive distance function to resort among the
*	highest ranked candidates in the best list - to maintain comparability with 
*	the rest of best list, the old scores are swapped instead of using the new 
*	scores.
*/
#define CJK_ZOOM_SZ BESTLEN

DECUMA_STATUS cjkBestListZoom(CJK_COMPRESSED_CHAR * c, CJK_SESSION * pSession, int bUseFreq)
{
	CJK_BESTLIST * bl = &pSession->db_lookup_bl;
	CJK_DISTANCE dist;
	CJK_BESTLIST blZoom;
	int i;

	decumaAssert(CJK_ZOOM_SZ <= BESTLEN);
	cjkBestListInit(&blZoom);
	blZoom.allowdupuc = 0;
	
	for (i=0; i < CJK_ZOOM_SZ && bl->unichar[i] != 0 && i < BESTLEN; i++) {
		dist = cjkDbGetDistanceToIndex(pSession, c, bl->index[i], 0, 1 /*Use final dist*/, bUseFreq);
		cjkBestListInsert(&blZoom, bl->unichar[i], bl->index[i], dist, pSession);
		if (TEST_ABORT_RECOGNITION_EVERY(5,i,pSession->pInterruptFunctions))
			return decumaRecognitionAborted;
	}
	/* Now set the elements in the session bestlist with the distance of their position */
	for (i=0; i < CJK_ZOOM_SZ && bl->unichar[i] != 0 && i < BESTLEN; i++) {
		cjkBestListSetElement(bl, blZoom.unichar[i], blZoom.index[i], blZoom.dist[i], i);
	}
	return decumaNoError;
}



DECUMA_INT32 cjkBestListPunctuationCheck(CJK_COMPRESSED_CHAR * c, CJK_SESSION * pSession)
{
	CJK_STROKE s1, s2;
	CJK_CONTEXT * con      = &pSession->con;
	DECUMA_UINT32 categorymask = pSession->sessionCategories;

	dltCCharGetFirstStroke(c, &s1, pSession);

	/*----------------------------------------------------- */
	/* BEGIN CHUNK: initiate bestlist for punctuation */
	/*  */
	/* This chunk initiates the bestlist and inserts a default list of */
	/* punctuations. */
	/*  */
	cjkBestListInit(&pSession->db_lookup_bl);
	cjkBestListInsertMany(uclist_punctinit, pSession);
	/* END CHUNK: initiate bestlist for punctuation */
	/*----------------------------------------------------- */


	decumaAssert(CJK_STROKE_EXISTS(&s1));
	if (CJK_STROKE_EXISTS(&s1)) {

		/*----------------------------------------------------- */
		/* BEGIN CHUNK: get information of s1 */
		/*  */
		/* The euclidean distance between the first and last sample point is */
		/* calculated and put into the variable [[s1_len]]. */
		/*  */
		CJK_GRIDPOINT s1_firstgp, s1_lastgp;
		CJK_DISTANCE s1_len;
		DECUMA_INT32 s1_lastgp_x, s1_firstgp_y, s1_lastgp_y;
		s1_lastgp = *cjkStrokeGetGridpoint(&s1, -1);
		s1_firstgp = *cjkStrokeGetGridpoint(&s1, 1);
		s1_len = CJK_GP_GET_SQ_DISTANCE_TRUE(s1_lastgp, s1_firstgp);
		s1_lastgp_x = CJK_STROKE_GET_X(&s1, -1);
		s1_firstgp_y = CJK_STROKE_GET_Y(&s1, 1);
		s1_lastgp_y = CJK_STROKE_GET_Y(&s1, -1);
		/* END CHUNK: get information of s1 */
		/*----------------------------------------------------- */


		if (dltCCCompressGetNbrStrokes(c) >= 2) {

			/*----------------------------------------------------- */
			/* BEGIN CHUNK: punctuations with more than one stroke */
			/*  */
			/* First all punctuations with more than one stroke are investigated. */
			/*  */
			s2 = s1;
			cjkStrokeNext(&s2, pSession);
			decumaAssert(CJK_STROKE_EXISTS(&s2));
			if (dltCCCompressGetNbrStrokes(c) == 4) {
				cjkBestListInsertFirst(UC_NUMBER_SIGN, pSession);
			}
			else if (dltCCCompressGetNbrStrokes(c) == 2) {

				/*----------------------------------------------------- */
				/* BEGIN CHUNK: get information of s2 */
				/*  */
				/* The euclidean distance between the first and last sample point is calculated */
				/* and put into the variable [[s2_len]]. */
				/*  */
				CJK_GRIDPOINT s2_firstgp, s2_lastgp;
				CJK_DISTANCE s2_len;
				DECUMA_INT32 s2_firstgp_y, s2_lastgp_y;
				s2_lastgp = *cjkStrokeGetGridpoint(&s2, -1);
				s2_firstgp = *cjkStrokeGetGridpoint(&s2, 1);
				s2_len = CJK_GP_GET_SQ_DISTANCE_TRUE(s2_lastgp, s2_firstgp);
				s2_firstgp_y = CJK_STROKE_GET_Y(&s2, 1);
				s2_lastgp_y = CJK_STROKE_GET_Y(&s2, -1);
				/* END CHUNK: get information of s2 */
				/*----------------------------------------------------- */


				if (dltCCharGetIntersectionCount(c, pSession) == 1) {
					return 0; /* This is plus sign or han, no punctuation. */
				}
				else if ((s1_lastgp_y > s2_firstgp_y) &&
					(s1_firstgp_y < s2_lastgp_y)) {

						/*----------------------------------------------------- */
						/* BEGIN CHUNK: double quotation mark */
						/*  */
						/* For quotation mark, other two stroked punctuation symbols are also */
						/* put in bestlist. */
						/*  */
						if (cjkContextGetDoubleQuoteCount(con)) {
							cjkBestListInsertMany(uclist_rdquot, pSession);
						}
						else {
							if (CJK_STROKE_GET_X(&s1, -1) <= CJK_STROKE_GET_X(&s1, -2)) {
								cjkBestListInsertMany(uclist_rdquot, pSession);
							}
							else {
								cjkBestListInsertMany(uclist_ldquot, pSession);
							}
							if (decumaIsKatakanaAnySize(cjkContextGetPrevious(con))) {
								cjkBestListInsertFirst(UC_DIACRITIC_DAKUTEN, pSession);
							}
						}
						/* END CHUNK: double quotation mark */
						/*----------------------------------------------------- */


				}
				else if (dltCCCompressGetXmax(c) - dltCCCompressGetXmin(c) >= dltCCCompressGetYmax(c) - dltCCCompressGetYmin(c)) {
					return 0; /* This is equal sign or han, no punctuation. */
				}
				else if (s1_lastgp_y > s2_firstgp_y) {

					/*----------------------------------------------------- */
					/* BEGIN CHUNK: semicolon or colon lower stroke first */
					/*  */
					/* For semi colon, other two stroked punctuation symbols are also */
					/* put in bestlist. */
					/* The threshold value T=10, is determined after testing in dltface. */
					/* It is used for comparing the length of the lower stroke, to */
					/* interpret a semicolon. */
					/*  */
					if ((s1_len > 10) && (s1_len > s2_len)) {
						cjkBestListInsertMany(uclist_semicolon, pSession);
					}
					else {
						cjkBestListInsertMany(uclist_colon, pSession);
					}
					/* END CHUNK: semicolon or colon lower stroke first */
					/*----------------------------------------------------- */


				}
				else {

					/*----------------------------------------------------- */
					/* BEGIN CHUNK: semicolon or colon upper stroke first */
					/*  */
					/* This chunk is as the above one, but with different stroke order. */
					/*  */
					if ((s2_len > 10) && (s2_len > s1_len)) {
						cjkBestListInsertMany(uclist_semicolon, pSession);
					}
					else {
						cjkBestListInsertMany(uclist_colon, pSession);
					}
					/* END CHUNK: semicolon or colon upper stroke first */
					/*----------------------------------------------------- */


				}
			}
			/* END CHUNK: punctuations with more than one stroke */
			/*----------------------------------------------------- */


		}
		else {

			/*----------------------------------------------------- */
			/* BEGIN CHUNK: punctuations with one stroke */
			/*  */
			/* This chunk sorts out which one of the one-stroked punctuation symbols */
			/* that the user has written. */
			/* First, the full stop symbol is detected. If the written character is */
			/* smaller than 1/20 of the height and width of the input square it is */
			/* interpreted as a full stop. [[dltCCCompressGetYmax(c)]] and other similar variables have */
			/* values of the original coordinates. */
			/*  */

			/*----------------------------------------------------- */
			/* BEGIN CHUNK: get information of the middle gridpoint */
			/*  */
			/* This chunk gets some information of the first stroke's middle point. */
			/*  */
			CJK_GRIDPOINT s1_middlegp;
			s1_middlegp = *cjkStrokeGetGridpoint(&s1, (CJK_STROKE_NPOINTS(&s1) + 1) / 2);
			/* END CHUNK: get information of the middle gridpoint */
			/*----------------------------------------------------- */


			if ((((dltCCCompressGetYmax(c) - dltCCCompressGetYmin(c)) * 20 < pSession->boxheight) &&
				(((dltCCCompressGetXmax(c) - dltCCCompressGetXmin(c)) * 20 < pSession->boxwidth) &&
				(categorymask & CJK_HAN))) ||
				((((dltCCCompressGetXmax(c) - dltCCCompressGetXmin(c)) * 10 < pSession->boxwidth) &&
				!(categorymask & CJK_HAN)) &&
				((dltCCCompressGetYmin(c) - pSession->boxybase) * 3 >= pSession->boxheight))) {
					if (((dltCCCompressGetYmax(c) - pSession->boxybase) * 3 > pSession->boxheight * 2)) {
						cjkBestListInsertMany(uclist_fullstop, pSession);
					}
					else if (((dltCCCompressGetYmax(c) - pSession->boxybase) * 3 > pSession->boxheight)){
						cjkBestListInsertMany(uclist_middledot, pSession);
					}
					else {
						cjkBestListInsertMany(uclist_rsquot, pSession);
					}
			}
			else if (5 * (dltCCCompressGetYmin(c) - pSession->boxybase) < 2 * pSession->boxheight) {
				/*upper half of input square */

				/*----------------------------------------------------- */
				/* BEGIN CHUNK: upper punctuation */
				/*  */
				/* One stroked symbols, written in the upper part of the box, is input */
				/* in bestlist by this chunk. A variable keeps the information if a */
				/* quotation mark is recently written. In that case the opposite */
				/* quotation mark is put on first place. */
				/*  */
				decumaAssert(CJK_STROKE_EXISTS(&s1));
				if (CJK_STROKE_GET_Y(&s1, 1) >= 9 &&
					CJK_GP_GET_Y(cjkStrokeGetMinYGridpoint(&s1)) <= 6 &&
					CJK_STROKE_GET_Y(&s1, -1) >= 9) {
						cjkBestListInsertMany(uclist_circumflex, pSession);
				}
				else if (CJK_STROKE_GET_X(&s1, -1) <= CJK_STROKE_GET_X(&s1, -2)) {
					cjkBestListInsertMany(uclist_rsquot, pSession);
					if (decumaIsKatakanaAnySize(cjkContextGetPrevious(con))) {
						cjkBestListInsertAt(UC_DIACRITIC_HANDAKUTEN, 0, 2, pSession);
					}
				}
				else {
					cjkBestListInsertMany(uclist_lsquot, pSession);
					if (decumaIsKatakanaAnySize(cjkContextGetPrevious(con))) {
						cjkBestListInsertAt(UC_DIACRITIC_HANDAKUTEN, 0, 2, pSession);
					}
				}
				if (decumaIsKatakanaAnySize(cjkContextGetPrevious(con))) {
					if (ABS(CJK_STROKE_GET_Y(&s1, 1) - CJK_STROKE_GET_Y(&s1, -1)) < 3 &&
						ABS(CJK_STROKE_GET_X(&s1, 1) - CJK_STROKE_GET_X(&s1, -1)) < 3) {
							cjkBestListInsertFirst(UC_DIACRITIC_HANDAKUTEN, pSession);
					}
				}
				/* END CHUNK: upper punctuation */
				/*----------------------------------------------------- */


			}
			else if (CJK_GP_GET_SQ_DISTANCE_TRUE(s1_middlegp, s1_firstgp) >
				CJK_GP_GET_SQ_DISTANCE_TRUE(s1_firstgp, s1_lastgp)) {
					cjkBestListInsertMany(uclist_ideofullstop, pSession);
			}
			else if (3 * (dltCCCompressGetYmax(c) - dltCCCompressGetYmin(c)) < (dltCCCompressGetXmax(c) - dltCCCompressGetXmin(c))) {
				cjkBestListInsertMany(uclist_ideocomma, pSession);
				cjkBestListInsertFirst(UC_HYPHEN_MINUS, pSession);
				if (3 * (dltCCCompressGetYmin(c) - pSession->boxybase) > 2 * pSession->boxheight) {
					cjkBestListInsertFirst(UC_LOW_LINE, pSession);
				}
				else {
					return 0; /* This is minus sign or han, no punctuation. */
				}
			}
			else if ((CJK_STROKE_GET_X(&s1, -2) < s1_lastgp_x) && (s1_firstgp_y < s1_lastgp_y)) {
				cjkBestListInsertMany(uclist_ideocomma, pSession);
			}
			else {
				cjkBestListInsertMany(uclist_fwcomma, pSession);
			}
			/* END CHUNK: punctuations with one stroke */
			/*----------------------------------------------------- */


		}
	}
	return 1; /* taken! */
}



void cjkBestListPreCheck(CJK_COMPRESSED_CHAR * c, CJK_SESSION * pSession)
{
	CJK_BESTLIST * bl = &pSession->db_lookup_bl;
	DECUMA_INT32 big = 0;
	DECUMA_INT32 parenthesis_big = 0;
	DECUMA_INT32 nStrokes = dltCCCompressGetNbrStrokes(c);
	DECUMA_INT32 i;
	CJK_UNICHAR uc = bl->unichar[0];
	DECUMA_INT32 height = (dltCCCompressGetYmax(c) - dltCCCompressGetYmin(c));
	DECUMA_UINT32 categorymask = pSession->sessionCategories;

	if (nStrokes != 2) {
		cjkBestListDiminish(0x0069, pSession); /* i */
		cjkBestListDiminish(0x006A, pSession); /* j */
		cjkBestListDiminish(UC_COLON, pSession);
		cjkBestListDiminish(UC_SEMICOLON, pSession);
		cjkBestListDiminish(UC_EXCLAMATION_MARK, pSession);
		cjkBestListDiminish(UC_QUESTION_MARK, pSession);
	}

	if (height * 12 > (pSession->boxheight) * 5) {
		big = 1;
	}

	if (pSession->boxheight && big && (
		uc == UC_COMMA ||
		uc == UC_FULL_STOP ||
		uc == UC_LEFT_SINGLE_QUOTATION_MARK ||
		uc == UC_RIGHT_SINGLE_QUOTATION_MARK ||
		uc == UC_LEFT_DOUBLE_QUOTATION_MARK ||
		uc == UC_RIGHT_DOUBLE_QUOTATION_MARK ||
		uc == UC_IDEOGRAPHIC_COMMA ||
		uc == UC_IDEOGRAPHIC_FULL_STOP ||
		uc == UC_APOSTROPHE ||
		uc == UC_CIRCUMFLEX_ACCENT ||
		uc == UC_GRAVE_ACCENT)) {
			cjkBestListDiminish(UC_COMMA, pSession);
			cjkBestListDiminish(UC_FULL_STOP, pSession);
			cjkBestListDiminish(UC_LEFT_SINGLE_QUOTATION_MARK, pSession);
			cjkBestListDiminish(UC_RIGHT_SINGLE_QUOTATION_MARK, pSession);
			cjkBestListDiminish(UC_LEFT_DOUBLE_QUOTATION_MARK, pSession);
			cjkBestListDiminish(UC_RIGHT_DOUBLE_QUOTATION_MARK, pSession);
			cjkBestListDiminish(UC_IDEOGRAPHIC_COMMA, pSession);
			cjkBestListDiminish(UC_IDEOGRAPHIC_FULL_STOP, pSession);
			cjkBestListDiminish(UC_APOSTROPHE, pSession);
			cjkBestListDiminish(UC_CIRCUMFLEX_ACCENT, pSession);
			cjkBestListDiminish(UC_GRAVE_ACCENT, pSession);
	}
	if (pSession->boxheight && (big ||
		((dltCCCompressGetYmax(c) - pSession->boxybase) * 2 > pSession->boxheight)) &&
		((uc == UC_QUOTATION_MARK) ||
		(uc == UC_TILDE))) {
			cjkBestListDiminish(UC_QUOTATION_MARK, pSession);
	}
	if ((height * 2 > pSession->boxheight) &&
		((dltCCCompressGetYmin(c) - pSession->boxybase) * 6 <= pSession->boxheight)) {
			parenthesis_big = 1;
	}
	if (pSession->boxheight && !parenthesis_big && (
		uc == UC_LEFT_ANGLE_BRACKET ||
		uc == UC_RIGHT_ANGLE_BRACKET ||
		uc == UC_LEFT_PARENTHESIS ||
		uc == UC_RIGHT_PARENTHESIS ||
		uc == UC_LEFT_SQUARE_BRACKET ||
		uc == UC_RIGHT_SQUARE_BRACKET ||
		uc == UC_LEFT_CURLY_BRACKET ||
		uc == UC_RIGHT_CURLY_BRACKET ||
		uc == UC_SOLIDUS ||
		uc == UC_REVERSE_SOLIDUS )) {
			cjkBestListDiminish(UC_LEFT_ANGLE_BRACKET, pSession);
			cjkBestListDiminish(UC_RIGHT_ANGLE_BRACKET, pSession);
			cjkBestListDiminish(UC_LEFT_PARENTHESIS, pSession);
			cjkBestListDiminish(UC_RIGHT_PARENTHESIS, pSession);
			cjkBestListDiminish(UC_LEFT_SQUARE_BRACKET, pSession);
			cjkBestListDiminish(UC_RIGHT_SQUARE_BRACKET, pSession);
			cjkBestListDiminish(UC_LEFT_CURLY_BRACKET, pSession);
			cjkBestListDiminish(UC_RIGHT_CURLY_BRACKET, pSession);
			cjkBestListDiminish(UC_REVERSE_SOLIDUS, pSession);
			cjkBestListDiminish(UC_SOLIDUS, pSession);
	}
	if (pSession->boxheight && (uc == UC_REVERSE_SOLIDUS ||
		uc == UC_IDEOGRAPHIC_COMMA ||
		uc == UC_GRAVE_ACCENT) ) {
			if (big) {
				cjkBestListInsertFirst(UC_REVERSE_SOLIDUS, pSession);
			}
			else if ((dltCCCompressGetYmin(c) - pSession->boxybase) * 2 > pSession->boxheight) {
				cjkBestListInsertFirst(UC_IDEOGRAPHIC_COMMA, pSession);
			}
			else if ((dltCCCompressGetYmax(c) - pSession->boxybase) * 2 <= pSession->boxheight) {
				cjkBestListInsertFirst(UC_GRAVE_ACCENT, pSession);
			}
	}
	if (pSession->boxheight && uc == 0x0072 && (height * 2 > pSession->boxheight) &&
		!(categorymask & CJK_HAN)) {
			cjkBestListDiminish(0x0072, pSession);
	}
	/*
	This code should only be used if reflines are available-
	possibly reinstitute with such conditional check./JS    

	if (pSession->boxheight && (uc == 0x0073 || uc == 0x0053) &&
	((dltCCCompressGetYmax(c) - pSession->boxybase) * 5 > pSession->boxheight * 4) &&
	!(categorymask & CJK_HAN)) {
	cjkBestListDiminish(0x0073, pSession); //trick for diminish both
	cjkBestListDiminish(0x0053, pSession);
	cjkBestListDiminish(0x0073, pSession);
	}*/
	if ((uc == 0x50CD) &&
		((bl->unichar[1] == 0x4EC7) || (bl->unichar[2] == 0x4EC7))) {
			cjkBestListDiminish(0x50CD, pSession);
	}
	if ((nStrokes == 2) && (pSession->boxheight) &&
		((dltCCCompressGetYmax(c) - pSession->boxybase) * 2 < pSession->boxheight)) {
			for (i = 0; bl->unichar[i] != 0 && i < BESTLEN; i++) {
				if (bl->unichar[i] == UC_QUOTATION_MARK) {
					cjkBestListInsertFirst(UC_QUOTATION_MARK, pSession);
				}
			}
	}
	/* For really small input enter the dots */
	if ((dltCCCompressGetYmax(c)-dltCCCompressGetYmin(c)) * 10 < pSession->boxheight &&
		(dltCCCompressGetXmax(c)-dltCCCompressGetXmin(c)) * 10 < pSession->boxwidth) {
			cjkBestListInsertMany(uclist_dots, pSession);
	}
}



void cjkBestListCapsCheck(CJK_COMPRESSED_CHAR * c, CJK_SESSION * pSession)
{
	CJK_BESTLIST * bl = &pSession->db_lookup_bl;
	CJK_STROKE s1, s2, s3;
	CJK_UNICHAR u;

	DECUMA_INT32 height;
	DECUMA_INT32 boxheight, boxybase;
	DECUMA_INT32 midpoint;
	CJK_BESTLIST_POSITION pos;

	height = dltCCCompressGetYmax(c) - dltCCCompressGetYmin(c);
	midpoint = dltCCCompressGetYmin(c) - pSession->boxybase + height/2;

	boxheight = pSession->boxheight;
	boxybase = pSession->boxybase;

	dltCCharGetFirstStroke(c, &s1, pSession);
	s2 = s1; cjkStrokeNext(&s2, pSession);
	s3 = s2; if (CJK_STROKE_EXISTS(&s2)) cjkStrokeNext(&s3, pSession);

	/*-----------------------------------------------------
    	 * Special hack to put HIRAGANASMALL and KATAKANASMALL into bestlist
	 * despite fact that they are not in db TODO: Fix TTP # 33557 and add
	 * these characters to db instead
	 *----------------------------------------------------- */
	if (pSession->sessionCategories & (CJK_HIRAGANASMALL | CJK_KATAKANASMALL)) {
		for (pos = 0; pos < BESTLEN - 1;){
			CJK_BESTLIST_POSITION addpos = pos + 1;
			u = bl->unichar[pos];
			if (u == 0x3042 || u == 0x3044 || u == 0x3046 ||
				u == 0x3048 || u == 0x304A || u == 0x3064 ||
				u == 0x3084 || u == 0x3086 || u == 0x3088 ||
				u == 0x308F || u == 0x30A2 || u == 0x30A4 ||
				u == 0x30A6 || u == 0x30A8 || u == 0x30AA ||
				u == 0x30E4 || u == 0x30C4 || u == 0x30E6 ||
				u == 0x30E8 || u == 0x30EF || u == 0x30AB ||
				u == 0x30B1) {

				if (pSession->boxheight > 0 && pSession->boxwidth > 0) {
					DECUMA_INT32 ymin = (100 * (dltCCCompressGetYmin(c) - pSession->boxybase)) / pSession->boxheight;
					DECUMA_INT32 xmax = (100 * (dltCCCompressGetXmax(c) - pSession->boxxbase)) / pSession->boxwidth;
					if ((ymin > 40 && xmax < 60) || ymin >= 50) {
						addpos = pos;
					}
				}
				if (u == 0x30AB){
					if (pSession->sessionCategories & CJK_KATAKANA) {
						cjkBestListInsertAt(0x30F5, 0, (CJK_BESTLIST_POSITION) addpos, pSession);
						/* Skips the added character as well */
						pos++;
					}
					else {
						/* Just replace */
						bl->unichar[pos] = 0x30F5;
					}
				}
				else if (u == 0x30B1){
					if (pSession->sessionCategories & CJK_KATAKANA) {
						cjkBestListInsertAt(0x30F6, 0, (CJK_BESTLIST_POSITION) addpos, pSession);
						/* Skips the added character as well */
						pos++;
					}
					else {
						/* Just replace */
						bl->unichar[pos] = 0x30F6;
					}
				}
				else if ((decumaIsKatakana(u) && (pSession->sessionCategories & CJK_KATAKANA)) ||
						 (decumaIsHiragana(u) && (pSession->sessionCategories & CJK_HIRAGANA))) {
					decumaAssert(u > 0);
					cjkBestListInsertAt((CJK_UNICHAR) (u - 1), 0, (CJK_BESTLIST_POSITION) addpos, pSession);
					/* Skips the added character as well */
					pos++;
				}
				else {
					/* "LARGE" not in category just replace */
					bl->unichar[pos] = u-1;
				}
			}
			/* Should not happen if DB is correct but currently remove non-small hiragana/katakana */
			else if (!(pSession->sessionCategories & (CJK_HIRAGANA | CJK_KATAKANA)) && (decumaIsHiragana(u) || decumaIsKatakana(u))) {
				cjkBestListRemove(bl, pos);
				continue; /* Skip pos++ */
			}
			pos++;
		}
	} /* END OF HIRAGANASMALL / KATAKANASMALL check */

	u = bl->unichar[0];

	if (u == 0x004B || u == 0x006B) { /* K k */

		/*----------------------------------------------------- */
		/* BEGIN CHUNK: check K k */
		/*  */
		if (dltCCCompressGetNbrStrokes(c) == 1) {
			cjkBestListInsertMany(uclist_006B, pSession); /*k */
			return;
		}
		else if (dltCCCompressGetNbrStrokes(c) == 2) {
			if ( CJK_STROKE_GET_Y(&s2,-1) - CJK_STROKE_GET_Y(&s2,1) <= 7 ) {
				cjkBestListInsertMany(uclist_006B, pSession); /*k */
				return;
			}
			else {
				cjkBestListInsertMany(uclist_004B, pSession); /*K */
				return;
			}
		}
		else {
			if (u == 0x004B) {
				cjkBestListInsertAt(0x006B, 0, 1, pSession);
			}
			else {
				cjkBestListInsertAt(0x004B, 0, 1, pSession);
			}
		}
		/* END CHUNK: check K k */
		/*----------------------------------------------------- */


		return;
	}
	if (u == 0x0050 || u == 0x0070) { /* P p */

		/*----------------------------------------------------- */
		/* BEGIN CHUNK: check P p */
		/*  */
		if (pSession->boxheight) {
			if ((dltCCCompressGetYmin(c) - boxybase) * 4 < boxheight) {
				cjkBestListInsertMany(uclist_0050, pSession); /* P high up */
				return;
			}
			else if (midpoint * 2 > boxheight) {
				cjkBestListInsertMany(uclist_0070, pSession); /* p low down */
				return;
			}
			else {
				cjkBestListInsertMany(uclist_0050, pSession); /* otherwise P */
				return;
			}
		}
		/* END CHUNK: check P p */
		/*----------------------------------------------------- */


	}
	if (u == 0x0059 || u == 0x0079) { /* Y y */

		/*----------------------------------------------------- */
		/* BEGIN CHUNK: check Y y */
		/*  */
		if (dltCCCompressGetNbrStrokes(c) == 1) {
			cjkBestListInsertMany(uclist_0079, pSession); /*y */
			return;
		}
		else {
			if (midpoint * 2 > boxheight) {
				cjkBestListInsertMany(uclist_0079, pSession); /*y */
				return;
			}
			else {
				cjkBestListInsertMany(uclist_0059, pSession); /*Y */
				return;
			}
		}
		/* END CHUNK: check Y y */
		/*----------------------------------------------------- */


	}
	if (u == 0x0043 || u == 0x0063 ||
		u == 0x004F || u == 0x006F ||
		u == 0x004D || u == 0x006D ||
		u == 0x0053 || u == 0x0073 ||
		(u >= 0x0055 && u <= 0x0058) ||
		(u >= 0x0075 && u <= 0x0078) ||
		u == 0x005A || u == 0x007A) {
			/* (C c) (O o) (M m) (S s) (U u) (V v) (W w) (X x) (Z z) */

			/*----------------------------------------------------- */
			/* BEGIN CHUNK: check size */
			/*  */
			CJK_UNICHAR capital_u, small_u;
			small_u = (u |= 0x20);
			capital_u = (u &= 0xDF);

			if (boxheight) {
				if (height * 2 > boxheight) {
					if (((u == 0x007A) || (u == 0x005A)) &&
						((dltCCCompressGetYmin(c) - boxybase) * 3 >= boxheight) &&
						!(pSession->sessionCategories & CJK_GB2312_A)) {
							cjkBestListInsertFirst(capital_u, pSession);
							cjkBestListInsertFirst(small_u, pSession);
							return;
					}
					cjkBestListInsertFirst(small_u, pSession);
					cjkBestListInsertFirst(capital_u, pSession);
					return;
				}
				else if (height * 3 < boxheight) {
					cjkBestListInsertFirst(capital_u, pSession);
					cjkBestListInsertFirst(small_u, pSession);
					return;
				}
				else {
					if ((dltCCCompressGetYmin(c) - pSession->boxybase) * 4 > boxheight) {
						cjkBestListInsertFirst(capital_u, pSession);
						cjkBestListInsertFirst(small_u, pSession);
						return;
					}
					else {
						cjkBestListInsertFirst(small_u, pSession);
						cjkBestListInsertFirst(capital_u, pSession);
						return;
					}
				}
			}
			/* END CHUNK: check size */
			/*----------------------------------------------------- */


	}
}




void cjkBestListThreeDropsCheck(CJK_COMPRESSED_CHAR * c, CJK_SESSION * pSession)
{
	CJK_BESTLIST * bl = &pSession->db_lookup_bl;
	CJK_STROKE s;
	DECUMA_INT32 y2, y2end, y3, x4;
	DECUMA_INT32 i = 0;

	if (dltCCCompressGetNbrStrokes(c) < 4) {
		return;
	}

	while(speak3drops[i] != bl->unichar[0]) {
		if (speak3drops[i] == 0) {
			return;
		}
		i++;
	}
	dltCCharGetFirstStroke(c, &s, pSession);
	if (CJK_STROKE_NPOINTS(&s) > 3) {
		return;
	}
	cjkStrokeNext(&s, pSession);
	y2 = CJK_STROKE_GET_Y(&s, 1);
	if (CJK_STROKE_NPOINTS(&s) <= 2) {
		y2end = CJK_STROKE_GET_Y(&s, -1);
		cjkStrokeNext(&s, pSession);
		y3= CJK_STROKE_GET_Y(&s, 1);
		cjkStrokeNext(&s, pSession);
		x4 = CJK_STROKE_GET_X(&s, 1);
		if (y3 > y2) { /* safeguarding for twoparted speaking */
			if (x4 > 5) { /* safeguard for traditional */
				/* if speaking suggested first then change to threedrops */
				if ((i & 1) == 0) {
					cjkBestListInsertFirst(speak3drops[i + 1], pSession);
				}
			}
		}
		else if ((y2end - y2) < 4) {
			/* if threedrops suggested first then change to speaking */
			if (i & 1) {
				cjkBestListInsertFirst(speak3drops[i - 1], pSession);
			}
		}
	}
	else {
		if ((CJK_STROKE_GET_X(&s, 2) > CJK_STROKE_GET_X(&s, 1)) &&
			(CJK_STROKE_GET_Y(&s, 3) > 8) && (CJK_STROKE_GET_Y(&s, 2) < 9)) {
				/* if threedrops suggested first then change to speaking */
				if (i & 1) {
					cjkBestListInsertFirst(speak3drops[i - 1], pSession);
				}
		}
	}
	return;
}


void cjkBestListFinalCheck(CJK_SESSION * pSession) {
	CJK_BESTLIST * bl = &pSession->db_lookup_bl;
	DECUMA_INT32 i;
	CJK_CONTEXT * con      = &pSession->con;
	CJK_UNICHAR firstuc = bl->unichar[0];
	DECUMA_UINT32 categorymask = pSession->sessionCategories;


	/* Remove all gesture candidates which are not the first one and also */
	/* remove all other candidates if the first candidate is a gesture. */
	if (decumaIsGesture(firstuc)) {
		DLTDB_INDEX firstIndex = bl->index[0];
		CJK_DISTANCE firstDist = bl->dist[0];
		/* Remove all other candidates */
		cjkBestListInit(&pSession->db_lookup_bl);
		bl->unichar[0] = firstuc;
		bl->dist[0] = firstDist;
		bl->index[0] = firstIndex;
	}
	else {
		/* Remove all other gesture candidates  */
		cjkBestListRemoveGestures(bl);
	}


	/* If first character is chinese one, add latin -. */
	if (firstuc == 0x4E00 || firstuc == 0x30FC) {
		cjkBestListInsertAt(UC_HYPHEN_MINUS, 0, 1, pSession);
	}



	/* If U/u is the best candidates, also V/v should be added in the bestlist, */
	/* and vice versa. */
	if (firstuc == 0x0055) {
		cjkBestListInsertAt(0x0076, 0, 2, pSession);
		cjkBestListInsertAt(0x0056, 0, 2, pSession);
		return;
	}
	else if (firstuc == 0x0056) {
		cjkBestListInsertAt(0x0075, 0, 2, pSession);
		cjkBestListInsertAt(0x0055, 0, 2, pSession);
		return;
	}
	else if (firstuc == 0x0075) {
		cjkBestListInsertAt(0x0056, 0, 2, pSession);
		cjkBestListInsertAt(0x0076, 0, 2, pSession);
		return;
	}
	else if (firstuc == 0x0076) {
		cjkBestListInsertAt(0x0055, 0, 2, pSession);
		cjkBestListInsertAt(0x0075, 0, 2, pSession);
		return;
	}


	/* Capital E could be written as a Z with a bar, in that case we add */
	/* Z to bestlist. */
	if ((firstuc == 0x0045) && (bl->unichar[1] == 0x007A)) {
		cjkBestListInsertAt(0x005A, 0, 1, pSession);
		return;
	}



	/* This finalcheck is for Hiragana / Katakana. */
	/* If one character with diacritic sign is on first place, also add the */
	/* corresponding character (ring <-> fnutt). */
	if (firstuc == 0x3070 ||
		firstuc == 0x3073 ||
		firstuc == 0x3076 ||
		firstuc == 0x3079 ||
		firstuc == 0x307C ||
		firstuc == 0x30D0 ||
		firstuc == 0x30D3 ||
		firstuc == 0x30D6 ||
		firstuc == 0x30D9 ||
		firstuc == 0x30DC) {
			cjkBestListInsertAt((CJK_UNICHAR) (firstuc +  1), 0, 1, pSession);
			if (firstuc == 0x3079) {
				cjkBestListInsertAt(0x30D9, 0, 2, pSession);
				cjkBestListInsertAt(0x30DA, 0, 3, pSession);
			}
			else if (firstuc == 0x30D9) {
				cjkBestListInsertAt(0x3079, 0, 2, pSession);
				cjkBestListInsertAt(0x307A, 0, 3, pSession);
			}
	}
	else if (firstuc == 0x3071 ||
		firstuc == 0x3074 ||
		firstuc == 0x3077 ||
		firstuc == 0x307A ||
		firstuc == 0x307D ||
		firstuc == 0x30D1 ||
		firstuc == 0x30D4 ||
		firstuc == 0x30D7 ||
		firstuc == 0x30DA ||
		firstuc == 0x30DD) {
			cjkBestListInsertAt((CJK_UNICHAR) (firstuc - 1), 0, 1, pSession);
			if (firstuc == 0x307A) {
				cjkBestListInsertAt(0x30DA, 0, 2, pSession);
				cjkBestListInsertAt(0x30D9, 0, 3, pSession);
			}
			else if (firstuc == 0x30DA) {
				cjkBestListInsertAt(0x307A, 0, 2, pSession);
				cjkBestListInsertAt(0x3079, 0, 3, pSession);
			}
	}



	/* These two characters can be written the same way. */
	if (firstuc == 0x308A) {
		cjkBestListInsertAt(0x30EA, 0, 1, pSession);
	}
	else if (firstuc == 0x30EA ) {
		cjkBestListInsertAt(0x308A, 0, 1, pSession);
	}
	if (firstuc == 0x3078) {
		cjkBestListInsertAt(0x30D8, 0, 1, pSession);
	}
	else if (firstuc == 0x30D8 ) {
		cjkBestListInsertAt(0x3078, 0, 1, pSession);
	}



	/* Chinese "dagger" and Chinese "seven" are added if one of them are at the */
	/* first place in the candidate list. */
	if (firstuc == 0x5315) {
		cjkBestListInsertAt(0x4E03, 0, 1, pSession);
	}
	else if (firstuc == 0x4E03 ) {
		cjkBestListInsertAt(0x5315, 0, 1, pSession);
	}


	if (categorymask & CJK_JIS0) {

		/* If the first candidate is a parantheseis or a corner bracket, add the rest */
		/* of the paranthesis used in japanese. */
		if (firstuc == UC_LEFT_PARENTHESIS ||
			firstuc == UC_LEFT_CORNER_BRACKET) {
				if (firstuc == UC_LEFT_PARENTHESIS) {
					cjkBestListInsertAt(UC_LEFT_CORNER_BRACKET, 0, 1, pSession);
				}
				else {
					cjkBestListInsertAt(UC_LEFT_PARENTHESIS, 0, 1, pSession);
				}
				cjkBestListInsertAt(UC_LEFT_WHITE_CORNER_BRACKET, 0, 2, pSession);
				cjkBestListInsertAt(UC_LEFT_ANGLE_BRACKET, 0, 3, pSession);
				cjkBestListInsertAt(UC_LEFT_DOUBLE_ANGLE_BRACKET, 0, 4, pSession);
				cjkBestListInsertAt(UC_LEFT_BLACK_LENTICULAR_BRACKET, 0, 5, pSession);
				return;
		}
		else if (firstuc == UC_RIGHT_PARENTHESIS ||
			firstuc == UC_RIGHT_CORNER_BRACKET) {
				if (firstuc == UC_RIGHT_PARENTHESIS) {
					cjkBestListInsertAt(UC_RIGHT_CORNER_BRACKET, 0, 1, pSession);
				}
				else {
					cjkBestListInsertAt(UC_RIGHT_PARENTHESIS, 0, 1, pSession);
				}
				cjkBestListInsertAt(UC_RIGHT_WHITE_CORNER_BRACKET, 0, 2, pSession);
				cjkBestListInsertAt(UC_RIGHT_ANGLE_BRACKET, 0, 3, pSession);
				cjkBestListInsertAt(UC_RIGHT_DOUBLE_ANGLE_BRACKET, 0, 4, pSession);
				cjkBestListInsertAt(UC_RIGHT_BLACK_LENTICULAR_BRACKET, 0, 5, pSession);
				return;
		}


	}

	/* Some characters are simplified form of others, Japanese itaiji. In case the */
	/* simplified form is on first place, the traditional character is forced into */
	/* second place in bestlist. */
	if (firstuc == 0x8FBA) {
		cjkBestListInsertAt(0x908A, 0, 1, pSession);
	}
	if (firstuc == 0x30ED || firstuc == 0x53E3) {
		cjkBestListInsertAt(0x56FD, 0, 2, pSession);
	}



	/* add quotation mark */
	if ((firstuc != UC_LEFT_DOUBLE_QUOTATION_MARK) &&
		(firstuc != UC_RIGHT_DOUBLE_QUOTATION_MARK) &&
		(bl->unichar[1] != UC_LEFT_DOUBLE_QUOTATION_MARK) &&
		(bl->unichar[1] != UC_RIGHT_DOUBLE_QUOTATION_MARK)) {
			for (i = 0; i < (BESTLEN - 1); i++) {
				if (bl->unichar[i] == UC_QUOTATION_MARK) {
					cjkBestListInsertAt(UC_QUOTATION_MARK, 0, (CJK_BESTLIST_POSITION) i, pSession);
					cjkBestListInsertAt(UC_LEFT_DOUBLE_QUOTATION_MARK, 0, (CJK_BESTLIST_POSITION) i, pSession);
					cjkBestListInsertAt(UC_RIGHT_DOUBLE_QUOTATION_MARK, 0, (CJK_BESTLIST_POSITION) i, pSession);
					return;
				}
			}
	}



	/* replace 3005 in bestlist - this should also be done outside in UI ? */
	if (!(categorymask & CJK_JIS0)) {
		CJK_UNICHAR utemp = cjkContextGetPrevious(con);
		if (utemp!=0) {
			cjkBestListReplace(UC_IDEOGRAPHIC_ITERATION_MARK, utemp, pSession);
		}
		else {
			/*replace with something similar */
			cjkBestListReplace(UC_IDEOGRAPHIC_ITERATION_MARK, 0x53E3, pSession);
		}
	}

}

/* categories for unicodes considered in special checks. */
typedef struct _tagUNICODE_TO_CATEGORY {
	DECUMA_UINT32 unicode;
	DECUMA_UINT32 category;
} UNICODE_TO_CATEGORY;

static const UNICODE_TO_CATEGORY uc2cat[] = {
	{ 0x0008, CJK_GESTURE },
	{ 0x0009, CJK_GESTURE },
	{ 0x000D, CJK_GESTURE },
	{ 0x0020, CJK_GESTURE },
	{ 0x0021, CJK_PUNCTUATION },
	{ 0x0022, CJK_PUNCTUATION },
	{ 0x0023, CJK_SYMBOL },
	{ 0x0024, CJK_SYMBOL },
	{ 0x0025, CJK_SYMBOL },
	{ 0x0026, CJK_SYMBOL },
	{ 0x0027, CJK_PUNCTUATION },
	{ 0x0028, CJK_PUNCTUATION },
	{ 0x0029, CJK_PUNCTUATION },
	{ 0x002A, CJK_SYMBOL },
	{ 0x002B, CJK_SYMBOL },
	{ 0x002C, CJK_PUNCTUATION },
	{ 0x002D, CJK_SYMBOL },
	{ 0x002E, CJK_PUNCTUATION },
	{ 0x002F, CJK_PUNCTUATION },
	{ 0x0030, CJK_DIGIT },
	{ 0x0031, CJK_DIGIT },
	{ 0x0032, CJK_DIGIT },
	{ 0x0033, CJK_DIGIT },
	{ 0x0034, CJK_DIGIT },
	{ 0x0035, CJK_DIGIT },
	{ 0x0036, CJK_DIGIT },
	{ 0x0037, CJK_DIGIT },
	{ 0x0039, CJK_DIGIT },
	{ 0x003A, CJK_PUNCTUATION },
	{ 0x003B, CJK_PUNCTUATION },
	{ 0x003C, CJK_SYMBOL },
	{ 0x003D, CJK_SYMBOL },
	{ 0x003E, CJK_SYMBOL },
	{ 0x003F, CJK_PUNCTUATION },
	{ 0x0040, CJK_SYMBOL },
	{ 0x0042, CJK_LATIN_UPPER },
	{ 0x0043, CJK_LATIN_UPPER },
	{ 0x0044, CJK_LATIN_UPPER },
	{ 0x0045, CJK_LATIN_UPPER },
	{ 0x0046, CJK_LATIN_UPPER },
	{ 0x0047, CJK_LATIN_UPPER },
	{ 0x0048, CJK_LATIN_UPPER },
	{ 0x0049, CJK_LATIN_UPPER },
	{ 0x004A, CJK_LATIN_UPPER },
	{ 0x004B, CJK_LATIN_UPPER },
	{ 0x004C, CJK_LATIN_UPPER },
	{ 0x004D, CJK_LATIN_UPPER },
	{ 0x004E, CJK_LATIN_UPPER },
	{ 0x004F, CJK_LATIN_UPPER },
	{ 0x0050, CJK_LATIN_UPPER },
	{ 0x0052, CJK_LATIN_UPPER },
	{ 0x0053, CJK_LATIN_UPPER },
	{ 0x0054, CJK_LATIN_UPPER },
	{ 0x0055, CJK_LATIN_UPPER },
	{ 0x0056, CJK_LATIN_UPPER },
	{ 0x0058, CJK_LATIN_UPPER },
	{ 0x0059, CJK_LATIN_UPPER },
	{ 0x005A, CJK_LATIN_UPPER },
	{ 0x005B, CJK_PUNCTUATION },
	{ 0x005C, CJK_PUNCTUATION },
	{ 0x005D, CJK_PUNCTUATION },
	{ 0x005E, CJK_SYMBOL },
	{ 0x005F, CJK_PUNCTUATION },
	{ 0x0060, CJK_PUNCTUATION },
	{ 0x0061, CJK_LATIN_LOWER },
	{ 0x0062, CJK_LATIN_LOWER },
	{ 0x0064, CJK_LATIN_LOWER },
	{ 0x0067, CJK_LATIN_LOWER },
	{ 0x0068, CJK_LATIN_LOWER },
	{ 0x0069, CJK_LATIN_LOWER },
	{ 0x006A, CJK_LATIN_LOWER },
	{ 0x006B, CJK_LATIN_LOWER },
	{ 0x006C, CJK_LATIN_LOWER },
	{ 0x006E, CJK_LATIN_LOWER },
	{ 0x006F, CJK_LATIN_LOWER },
	{ 0x0070, CJK_LATIN_LOWER },
	{ 0x0071, CJK_LATIN_LOWER },
	{ 0x0072, CJK_LATIN_LOWER },
	{ 0x0073, CJK_LATIN_LOWER },
	{ 0x0074, CJK_LATIN_LOWER },
	{ 0x0075, CJK_LATIN_LOWER },
	{ 0x0076, CJK_LATIN_LOWER },
	{ 0x0078, CJK_LATIN_LOWER },
	{ 0x0079, CJK_LATIN_LOWER },
	{ 0x007A, CJK_LATIN_LOWER },
	{ 0x007B, CJK_PUNCTUATION },
	{ 0x007C, CJK_PUNCTUATION },
	{ 0x007D, CJK_PUNCTUATION },
	{ 0x007E, CJK_PUNCTUATION },
	{ 0x00A3, CJK_SYMBOL },
	{ 0x00A5, CJK_SYMBOL },
	{ 0x2014, CJK_SYMBOL },
	{ 0x2018, CJK_PUNCTUATION },
	{ 0x2019, CJK_PUNCTUATION },
	{ 0x201C, CJK_PUNCTUATION },
	{ 0x201D, CJK_PUNCTUATION },
	{ 0x2022, CJK_PUNCTUATION },
	{ 0x2026, CJK_PUNCTUATION },
	{ 0x3001, CJK_PUNCTUATION },
	{ 0x3002, CJK_PUNCTUATION },
	{ 0x3005, CJK_POPULARFORM },
	{ 0x3006, CJK_SYMBOL },
	{ 0x3008, CJK_PUNCTUATION },
	{ 0x3009, CJK_PUNCTUATION },
	{ 0x300A, CJK_PUNCTUATION },
	{ 0x300B, CJK_PUNCTUATION },
	{ 0x300C, CJK_PUNCTUATION },
	{ 0x300D, CJK_PUNCTUATION },
	{ 0x3012, CJK_PUNCTUATION },
	{ 0x3041, CJK_HIRAGANASMALL },
	{ 0x3042, CJK_HIRAGANASMALL | CJK_HIRAGANA },
	{ 0x3043, CJK_HIRAGANASMALL },
	{ 0x3044, CJK_HIRAGANASMALL | CJK_HIRAGANA },
	{ 0x3045, CJK_HIRAGANASMALL },
	{ 0x3046, CJK_HIRAGANASMALL | CJK_HIRAGANA },
	{ 0x3047, CJK_HIRAGANASMALL },
	{ 0x3048, CJK_HIRAGANASMALL | CJK_HIRAGANA },
	{ 0x3049, CJK_HIRAGANASMALL },
	{ 0x304D, CJK_HIRAGANA },
	{ 0x304F, CJK_HIRAGANA },
	{ 0x3051, CJK_HIRAGANASMALL | CJK_HIRAGANA },
	{ 0x3052, CJK_HIRAGANA },
	{ 0x3053, CJK_HIRAGANA },
	{ 0x3055, CJK_HIRAGANA },
	{ 0x3056, CJK_HIRAGANA },
	{ 0x3057, CJK_HIRAGANA },
	{ 0x3059, CJK_HIRAGANA },
	{ 0x305D, CJK_HIRAGANA },
	{ 0x3061, CJK_HIRAGANA },
	{ 0x3063, CJK_HIRAGANASMALL },
	{ 0x3064, CJK_HIRAGANASMALL | CJK_HIRAGANA },
	{ 0x3066, CJK_HIRAGANA },
	{ 0x306C, CJK_HIRAGANA },
	{ 0x306D, CJK_HIRAGANA },
	{ 0x3070, CJK_HIRAGANA },
	{ 0x3071, CJK_HIRAGANA },
	{ 0x3072, CJK_HIRAGANA },
	{ 0x3073, CJK_HIRAGANA },
	{ 0x3074, CJK_HIRAGANA },
	{ 0x3075, CJK_HIRAGANA },
	{ 0x3076, CJK_HIRAGANA },
	{ 0x3077, CJK_HIRAGANA },
	{ 0x3078, CJK_HIRAGANA },
	{ 0x3079, CJK_HIRAGANA },
	{ 0x307A, CJK_HIRAGANA },
	{ 0x307C, CJK_HIRAGANA },
	{ 0x307D, CJK_HIRAGANA },
	{ 0x307E, CJK_HIRAGANA },
	{ 0x307F, CJK_HIRAGANA },
	{ 0x3082, CJK_HIRAGANA },
	{ 0x3083, CJK_HIRAGANASMALL },
	{ 0x3084, CJK_HIRAGANASMALL | CJK_HIRAGANA },
	{ 0x3085, CJK_HIRAGANASMALL },
	{ 0x3087, CJK_HIRAGANASMALL },
	{ 0x3089, CJK_HIRAGANA },
	{ 0x308A, CJK_HIRAGANA },
	{ 0x308C, CJK_HIRAGANA },
	{ 0x308D, CJK_HIRAGANA },
	{ 0x308E, CJK_HIRAGANASMALL },
	{ 0x308F, CJK_HIRAGANASMALL | CJK_HIRAGANA },
	{ 0x3093, CJK_HIRAGANA },
	{ 0x3095, CJK_HIRAGANASMALL },
	{ 0x3096, CJK_HIRAGANASMALL },
	{ 0x309E, CJK_HIRAGANA },
	{ 0x30A1, CJK_KATAKANASMALL },
	{ 0x30A2, CJK_KATAKANASMALL | CJK_KATAKANA },
	{ 0x30A3, CJK_KATAKANASMALL },
	{ 0x30A4, CJK_KATAKANASMALL | CJK_KATAKANA },
	{ 0x30A5, CJK_KATAKANASMALL },
	{ 0x30A6, CJK_KATAKANASMALL | CJK_KATAKANA },
	{ 0x30A7, CJK_KATAKANASMALL },
	{ 0x30A8, CJK_KATAKANASMALL | CJK_KATAKANA },
	{ 0x30A9, CJK_KATAKANASMALL },
	{ 0x30AA, CJK_KATAKANASMALL | CJK_KATAKANA },
	{ 0x30AB, CJK_KATAKANASMALL | CJK_KATAKANA },
	{ 0x30AD, CJK_KATAKANA },
	{ 0x30AF, CJK_KATAKANA },
	{ 0x30B1, CJK_KATAKANASMALL | CJK_KATAKANA },
	{ 0x30B2, CJK_KATAKANA },
	{ 0x30B3, CJK_KATAKANA },
	{ 0x30B9, CJK_KATAKANA },
	{ 0x30BB, CJK_KATAKANA },
	{ 0x30BC, CJK_KATAKANA },
	{ 0x30BD, CJK_KATAKANA },
	{ 0x30BF, CJK_KATAKANA },
	{ 0x30C1, CJK_KATAKANA },
	{ 0x30C2, CJK_KATAKANA },
	{ 0x30C3, CJK_KATAKANASMALL },
	{ 0x30C6, CJK_KATAKANA },
	{ 0x30C7, CJK_KATAKANA },
	{ 0x30C8, CJK_KATAKANA },
	{ 0x30C9, CJK_KATAKANA },
	{ 0x30CA, CJK_KATAKANA },
	{ 0x30CB, CJK_KATAKANA },
	{ 0x30CC, CJK_KATAKANA },
	{ 0x30CE, CJK_KATAKANA },
	{ 0x30CF, CJK_KATAKANA },
	{ 0x30D0, CJK_KATAKANA },
	{ 0x30D1, CJK_KATAKANA },
	{ 0x30D2, CJK_KATAKANA },
	{ 0x30D3, CJK_KATAKANA },
	{ 0x30D4, CJK_KATAKANA },
	{ 0x30D5, CJK_KATAKANA },
	{ 0x30D6, CJK_KATAKANA },
	{ 0x30D7, CJK_KATAKANA },
	{ 0x30D8, CJK_KATAKANA },
	{ 0x30D9, CJK_KATAKANA },
	{ 0x30DA, CJK_KATAKANA },
	{ 0x30DC, CJK_KATAKANA },
	{ 0x30DD, CJK_KATAKANA },
	{ 0x30DE, CJK_KATAKANA },
	{ 0x30E0, CJK_KATAKANA },
	{ 0x30E1, CJK_KATAKANA },
	{ 0x30E2, CJK_KATAKANA },
	{ 0x30E3, CJK_KATAKANASMALL },
	{ 0x30E4, CJK_KATAKANASMALL | CJK_KATAKANA },
	{ 0x30E5, CJK_KATAKANASMALL },
	{ 0x30E6, CJK_KATAKANASMALL | CJK_KATAKANA },
	{ 0x30E7, CJK_KATAKANASMALL },
	{ 0x30E8, CJK_KATAKANASMALL | CJK_KATAKANA },
	{ 0x30E9, CJK_KATAKANA },
	{ 0x30EA, CJK_KATAKANA },
	{ 0x30EB, CJK_KATAKANA },
	{ 0x30EC, CJK_KATAKANA },
	{ 0x30ED, CJK_KATAKANA },
	{ 0x30EE, CJK_KATAKANASMALL },
	{ 0x30EF, CJK_KATAKANASMALL | CJK_KATAKANA },
	{ 0x30F1, CJK_KATAKANA },
	{ 0x30F2, CJK_KATAKANA },
	{ 0x30F5, CJK_KATAKANASMALL },
	{ 0x30F6, CJK_KATAKANASMALL },
	{ 0x30FC, CJK_KATAKANA },
	{ 0x4E00, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x4E01, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x4E03, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x4E08, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x4E0A, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x4E0B, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x4E14, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x4E28, CJK_HKSCS | CJK_GB2312_B_RADICALS },
	{ 0x4E2B, CJK_BIGFIVE_LEVEL_1 | CJK_GB2312_A },
	{ 0x4E2D, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x4E36, CJK_HKSCS | CJK_GB2312_B_RADICALS | CJK_JIS_LEVEL_2 },
	{ 0x4E38, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x4E3B, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x4E3F, CJK_HKSCS | CJK_JIS_LEVEL_2 | CJK_GB2312_B_RADICALS },
	{ 0x4E4B, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x4E59, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x4E5D, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x4E60, CJK_GB2312_A },
	{ 0x4E86, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x4E87, CJK_HKSCS },
	{ 0x4E8C, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x4E8D, CJK_BIGFIVE_LEVEL_2 | CJK_GB2312_B },
	{ 0x4E8E, CJK_BIGFIVE_LEVEL_1 | CJK_GB2312_A | CJK_JIS_LEVEL_2 },
	{ 0x4E91, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x4E93, CJK_BIGFIVE_LEVEL_2 | CJK_GB2312_B },
	{ 0x4E94, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x4E95, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x4EBA, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x4EC5, CJK_GB2312_A },
	{ 0x4EC7, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x4EC9, CJK_BIGFIVE_LEVEL_2 | CJK_GB2312_B },
	{ 0x4ED9, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x4EEC, CJK_GB2312_A },
	{ 0x4F1A, CJK_JIS_LEVEL_1 | CJK_HKSCS | CJK_GB2312_A },
	{ 0x4F55, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x4F58, CJK_BIGFIVE_LEVEL_2 | CJK_GB2312_B },
	{ 0x4F59, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x4F65, CJK_GB2312_B },
	{ 0x50CD, CJK_JIS_LEVEL_1 | CJK_HKSCS },
	{ 0x513F, CJK_BIGFIVE_LEVEL_1 | CJK_GB2312_A | CJK_JIS_LEVEL_2 },
	{ 0x5140, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_2 | CJK_GB2312_B },
	{ 0x5143, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x514D, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x5151, CJK_GB2312_A },
	{ 0x5165, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x5168, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x516B, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x5199, CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x51E0, CJK_BIGFIVE_LEVEL_1 | CJK_GB2312_A | CJK_JIS_LEVEL_2 },
	{ 0x51E1, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x5200, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x529B, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x52A8, CJK_HKSCS | CJK_GB2312_A },
	{ 0x52AB, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x52FA, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x5315, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_2 | CJK_GB2312_B },
	{ 0x5341, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x5343, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x5348, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x535C, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x5360, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x5362, CJK_GB2312_A },
	{ 0x5374, CJK_JIS_LEVEL_1 | CJK_HKSCS | CJK_GB2312_A },
	{ 0x53C8, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x53CA, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x53CB, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x53CD, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x53E3, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x53E4, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x53E8, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_2 | CJK_GB2312_B },
	{ 0x53EF, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x53F3, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x53FB, CJK_BIGFIVE_LEVEL_1 | CJK_GB2312_B },
	{ 0x5408, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x5411, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x5668, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_POPULARFORM | CJK_GB2312_A },
	{ 0x56DA, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x56DB, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x56DE, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x56FD, CJK_JIS_LEVEL_1 | CJK_POPULARFORM | CJK_HKSCS | CJK_TRADSIMPDUAL | CJK_GB2312_A },
	{ 0x571F, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x5728, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x5806, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x58EB, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x5915, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x5927, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x5929, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x592B, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x597D, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x59B2, CJK_BIGFIVE_LEVEL_2 | CJK_JIS_LEVEL_2 | CJK_GB2312_B },
	{ 0x59D0, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x59E3, CJK_BIGFIVE_LEVEL_1 | CJK_GB2312_B },
	{ 0x5A07, CJK_GB2312_A },
	{ 0x5B50, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x5B51, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_2 | CJK_GB2312_B },
	{ 0x5B53, CJK_BIGFIVE_LEVEL_1 | CJK_GB2312_B },
	{ 0x5B78, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_2 },
	{ 0x5BA2, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x5BB9, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x5C0F, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x5C22, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_2 | CJK_GB2312_B },
	{ 0x5C38, CJK_BIGFIVE_LEVEL_1 | CJK_GB2312_A | CJK_JIS_LEVEL_2 },
	{ 0x5C71, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x5DE5, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x5DF1, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x5DF2, CJK_BIGFIVE_LEVEL_1 | CJK_GB2312_A | CJK_JIS_LEVEL_2 },
	{ 0x5DF3, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x5E72, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x5EFE, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_2 | CJK_GB2312_B },
	{ 0x5F00, CJK_GB2312_A },
	{ 0x5F62, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x5F71, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_POPULARFORM | CJK_GB2312_A },
	{ 0x5FC3, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x6237, CJK_HKSCS | CJK_GB2312_A },
	{ 0x624D, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x6301, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x6322, CJK_GB2312_B },
	{ 0x6324, CJK_GB2312_A },
	{ 0x633D, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x6362, CJK_GB2312_A },
	{ 0x63A8, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x6587, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x65E0, CJK_HKSCS | CJK_GB2312_A | CJK_JIS_LEVEL_2 },
	{ 0x65E5, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x65E6, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x6613, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_POPULARFORM | CJK_GB2312_A },
	{ 0x66F0, CJK_BIGFIVE_LEVEL_1 | CJK_GB2312_A | CJK_JIS_LEVEL_2 },
	{ 0x6746, CJK_BIGFIVE_LEVEL_1 | CJK_GB2312_A | CJK_JIS_LEVEL_2 },
	{ 0x6821, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x6865, CJK_HKSCS | CJK_GB2312_A },
	{ 0x6A5F, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 },
	{ 0x6C40, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x6C41, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x6C49, CJK_HKSCS | CJK_GB2312_A },
	{ 0x6C54, CJK_BIGFIVE_LEVEL_2 | CJK_GB2312_B },
	{ 0x6C55, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_2 | CJK_GB2312_A },
	{ 0x6C57, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x6C5B, CJK_BIGFIVE_LEVEL_1 | CJK_GB2312_A },
	{ 0x6C5F, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x6CA1, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x6CA4, CJK_GB2312_A },
	{ 0x6CA6, CJK_GB2312_A },
	{ 0x6CAE, CJK_BIGFIVE_LEVEL_1 | CJK_GB2312_A | CJK_JIS_LEVEL_2 },
	{ 0x6CB3, CJK_BIGFIVE_LEVEL_1 | CJK_GB2312_A | CJK_JIS_LEVEL_1 },
	{ 0x6CBB, CJK_BIGFIVE_LEVEL_1 | CJK_GB2312_A | CJK_JIS_LEVEL_1 },
	{ 0x6CBC, CJK_BIGFIVE_LEVEL_1 | CJK_GB2312_A | CJK_JIS_LEVEL_1 },
	{ 0x6CBD, CJK_BIGFIVE_LEVEL_1 | CJK_GB2312_A | CJK_JIS_LEVEL_2 },
	{ 0x6CEA, CJK_HKSCS | CJK_GB2312_A | CJK_JIS_LEVEL_2 },
	{ 0x6CFD, CJK_GB2312_A },
	{ 0x6D01, CJK_BIGFIVE_LEVEL_2 | CJK_GB2312_A },
	{ 0x6D0B, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x6D17, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x6D19, CJK_BIGFIVE_LEVEL_2 | CJK_JIS_LEVEL_2 | CJK_GB2312_B },
	{ 0x6D35, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_2 | CJK_GB2312_B },
	{ 0x6D3B, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x6D3C, CJK_BIGFIVE_LEVEL_2 | CJK_GB2312_A },
	{ 0x6D51, CJK_GB2312_A },
	{ 0x6D69, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x6D6F, CJK_BIGFIVE_LEVEL_2 | CJK_GB2312_B },
	{ 0x6D77, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x6D88, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x6D8C, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x6D8E, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_2 | CJK_GB2312_A },
	{ 0x6DBF, CJK_BIGFIVE_LEVEL_1 | CJK_GB2312_B },
	{ 0x6DE1, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x6DEE, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_2 | CJK_GB2312_A },
	{ 0x6E05, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x6E0E, CJK_GB2312_B },
	{ 0x6E16, CJK_GB2312_B },
	{ 0x6E1A, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_B },
	{ 0x6E1D, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_2 | CJK_GB2312_A },
	{ 0x6E2B, CJK_BIGFIVE_LEVEL_2 | CJK_JIS_LEVEL_2 | CJK_GB2312_B },
	{ 0x6E2D, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_2 | CJK_GB2312_A },
	{ 0x6E34, CJK_BIGFIVE_LEVEL_1 | CJK_GB2312_A },
	{ 0x6E5B, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x6EC2, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_2 | CJK_GB2312_B },
	{ 0x6EA2, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x6EF4, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x6F20, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x6F2B, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x6F6D, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_2 | CJK_GB2312_A },
	{ 0x6F9C, CJK_GB2312_A },
	{ 0x725B, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x7279, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x738B, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x73CD, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x751F, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x7529, CJK_BIGFIVE_LEVEL_1 | CJK_GB2312_A },
	{ 0x7530, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x7531, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x7532, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x7533, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x7535, CJK_HKSCS | CJK_GB2312_A },
	{ 0x754C, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x76BF, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x76E5, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_2 | CJK_GB2312_B },
	{ 0x76EE, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x77F3, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x793A, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x7EA8, CJK_GB2312_B },
	{ 0x7EAB, CJK_GB2312_A },
	{ 0x80B2, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x81EA, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x8272, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x8279, CJK_HKSCS | CJK_GB2312_B_RADICALS },
	{ 0x898B, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 },
	{ 0x8A02, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 },
	{ 0x8A08, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 },
	{ 0x8BA1, CJK_GB2312_A },
	{ 0x8BA2, CJK_GB2312_A },
	{ 0x8BA6, CJK_GB2312_B },
	{ 0x8BA7, CJK_GB2312_B },
	{ 0x8BAA, CJK_GB2312_B },
	{ 0x8BAB, CJK_GB2312_A },
	{ 0x8BAF, CJK_GB2312_A },
	{ 0x8BB4, CJK_GB2312_B },
	{ 0x8BBA, CJK_GB2312_A },
	{ 0x8BBE, CJK_HKSCS | CJK_GB2312_A },
	{ 0x8BC2, CJK_GB2312_B },
	{ 0x8BC3, CJK_GB2312_B },
	{ 0x8BC5, CJK_GB2312_A },
	{ 0x8BCA, CJK_GB2312_A },
	{ 0x8BCF, CJK_GB2312_B },
	{ 0x8BD1, CJK_GB2312_A },
	{ 0x8BD2, CJK_GB2312_B },
	{ 0x8BD6, CJK_GB2312_B },
	{ 0x8BD8, CJK_GB2312_B },
	{ 0x8BDB, CJK_GB2312_A },
	{ 0x8BDC, CJK_GB2312_B },
	{ 0x8BDD, CJK_GB2312_A },
	{ 0x8BDE, CJK_GB2312_A },
	{ 0x8BE2, CJK_HKSCS | CJK_GB2312_A },
	{ 0x8BE6, CJK_GB2312_A },
	{ 0x8BE8, CJK_GB2312_B },
	{ 0x8BED, CJK_GB2312_A },
	{ 0x8BEE, CJK_GB2312_B },
	{ 0x8BF0, CJK_GB2312_B },
	{ 0x8BF2, CJK_GB2312_A },
	{ 0x8BF5, CJK_GB2312_A },
	{ 0x8BF7, CJK_GB2312_A },
	{ 0x8BF8, CJK_GB2312_A },
	{ 0x8BFB, CJK_GB2312_A },
	{ 0x8BFC, CJK_GB2312_B },
	{ 0x8C01, CJK_GB2312_A },
	{ 0x8C08, CJK_GB2312_A },
	{ 0x8C0C, CJK_GB2312_B },
	{ 0x8C0D, CJK_GB2312_A },
	{ 0x8C12, CJK_GB2312_B },
	{ 0x8C13, CJK_GB2312_A },
	{ 0x8C15, CJK_GB2312_B },
	{ 0x8C1F, CJK_GB2312_B },
	{ 0x8C24, CJK_GB2312_A },
	{ 0x8C25, CJK_GB2312_B },
	{ 0x8C29, CJK_GB2312_A },
	{ 0x8C2A, CJK_GB2312_B },
	{ 0x8C2D, CJK_GB2312_A },
	{ 0x8C30, CJK_GB2312_A },
	{ 0x8C9D, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 },
	{ 0x8F66, CJK_HKSCS | CJK_GB2312_A },
	{ 0x8FBA, CJK_JIS_LEVEL_1 | CJK_HKSCS },
	{ 0x908A, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_2 },
	{ 0x9091, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x91CC, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x958B, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 },
	{ 0x958C, CJK_BIGFIVE_LEVEL_2 },
	{ 0x95EB, CJK_GB2312_B },
	{ 0x95EE, CJK_GB2312_A },
	{ 0x97F3, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x9999, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x9A6C, CJK_HKSCS | CJK_GB2312_A },
	{ 0x9AD8, CJK_BIGFIVE_LEVEL_1 | CJK_JIS_LEVEL_1 | CJK_GB2312_A },
	{ 0x9CB4, CJK_GB2312_B },
	{ 0x9CB7, CJK_GB2312_B },
	{ 0x9FA5, CJK_HKSCS },
	{ 0xAC00, CJK_HANGUL_A }
};

/* 
 * A function to check if a specific unicode is in category. The database can not be used directly for this since categories are store per index and
 * and not per unicode. Only used in special checks.
 *
*/ 


static DECUMA_UINT32 ucIsInCatmask(CJK_UNICHAR u, CJK_SESSION * pSession)
{
	DECUMA_UINT32 const categorymask = pSession->sessionCategories;
	int high, low, middle;

	/* Look in personal category if available */
	if (pSession->pSessionSettings->pCharSetExtension && pSession->nPersonalCategories > 0) {
		int n = 0;
		while (pSession->pSessionSettings->pCharSetExtension[n] != 0) {
			/* Skip multicode symbols */
			if (pSession->pSessionSettings->pCharSetExtension[n+1] != 0) {
				/* Loop until end of multicode symbol */
				while (pSession->pSessionSettings->pCharSetExtension[n] != 0) {
					n++;					
				}
				/* Go to start of next symbol */
				n++;
				continue;
			}
			if (u == pSession->pSessionSettings->pCharSetExtension[n]) {
				return 1;
			}
			/* Go to start of next symbol */
			n += 2;
		}
	}

	/* Look for uc in the special checks uc2cat map by binary search */

	high   = sizeof(uc2cat) / sizeof(uc2cat[0]) - 1;
	low    = 0;
	middle = (high + low) / 2;

	while (high > low+1) {
		if (u > uc2cat[middle].unicode) {
			low = middle;
		}
		else if (u < uc2cat[middle].unicode) {
			high = middle;
		}
		else { /* Found */
			return categorymask & uc2cat[middle].category;
		}
		middle = (high + low) / 2;
	}

	if (u == uc2cat[low].unicode)
		return categorymask & uc2cat[low].category;

	if (u == uc2cat[high].unicode) 
		return categorymask & uc2cat[low].category;

	return 0;
}



static CJK_BOOLEAN ucIsAllowed(CJK_UNICHAR u, CJK_SESSION *pSession) 
{
	return (DLTDB_UNICODE_IS_IN_DB(&pSession->db, u) || decumaIsHiraganaSmall(u) || decumaIsKatakanaSmall(u));
}

/** @} */
