/*****************************************************************************
* COPYRIGHT ZI AND SUBJECT TO CONFIDENTIALITY RESTRICTIONS                   *
*                                                                            *
* This file is the confidential and proprietary property of Zi Corporation   *
* of Canada, Inc. ("Zi") or one of its affiliates.                           *
******************************************************************************/

#pragma once

#ifndef CJK_BEST_LIST_H_
#define CJK_BEST_LIST_H_

/** 
 * @defgroup DOX_CJK_BESTLIST cjkBestList
 * @{
 */


#define BESTLEN 50
#define MAXDIST MAX_CJK_DISTANCE
#define LARGEDIST 9999

#include "dltConfig.h"
#include "cjkTypes.h"
#include "decumaBasicTypes.h"

/*
 * The datastructure @ref CJK_BESTLIST contains the best matches.
 * The arrays @c index and @c dist contains the indexes and distances,
 * respectively, of the best matches sorted such that the first element
 * contains the best match.
 * In the arrays there must be one extra character or index, that, is the null
 * terminator.
 * 
 * The @ref BESTLEN macro below is simply the maximum number of characters
 * in the list that will be returned. But, the actual number returned will
 * mostly be smaller. What happens is that @ref BESTLEN prototypes are
 * kept from the firs sweep, \em including those with duplicate Unicodes.
 * Then in a second sweep, using @ref dltCCharGetDistanceFinal, the duplicate 
 * Unicodes are removed and the list is shorter. In the simplified engine there
 * can be quite a few character varians included so this macro had to
 * be increased from 20 to 25 in order to get at least ten candidates
 * for most characters.
 * 
 * A distance is a typedefed unsigned 32 bit integer.
 * The @ref LARGEDIST constant is used for returning something that is
 * large but can be added up anyways in the loops that calculate sums of
 * distances. If for example @ref cjkStrokeGetDistanceDTW would return 
 * @ref MAXDIST -- instead of @ref LARGEDIST in the present implementation --
 * then we would get in trouble if we don't check for overflow when
 * adding up distances in routines like @ref dltCCharGetRawDistance. With this
 * design we can forget checking for overflow.
 */
typedef struct _tagCJK_BSTLIST {
   DLTDB_INDEX   index  [BESTLEN + 1];
   CJK_DISTANCE  dist   [BESTLEN + 1];
   CJK_UNICHAR   unichar[BESTLEN + 1];
   DECUMA_INT32  allowdupuc;
} CJK_BESTLIST;

#include "cjkCompressedCharacter.h"
#include "cjkSession.h"

/**
 * This method fills @ref CJK_BESTLIST with null characters and
 * very large distances. It is kind of a constructor method except
 * that the caller must allocate the memory.
 */ 
DECUMA_HWR_PRIVATE void  cjkBestListInit(CJK_BESTLIST * const pBestlist);


/**
 * This method returns the number of elements in bestlist,
 * maximum value: BESTLEN.
 */ 
DECUMA_HWR_PRIVATE DECUMA_INT32 cjkBestListGetLength(CJK_SESSION * pSession);


/**
 * For inserting the index into the list of best matches so far we first check
 * to see if the insert distance @ref dist is smaller than the largest distance
 * in the bestlist @ref bl. There will always be only one copy of any
 * @ref CJK_UNICHAR in the list. The @ref index is kept syncronized in the list
 * for debugging purposes.
 */ 
DECUMA_HWR_PRIVATE void cjkBestListInsert(CJK_BESTLIST * const bl, CJK_UNICHAR unichar, DLTDB_INDEX index, CJK_DISTANCE const dist, CJK_SESSION const * const pSession);


/**
 * This function inserts the index, which is input, at a specific position,
 * which is also given as an input. The input position is checked not to be
 * bigger than the length of the bestlist. There will always be only one copy
 * of any @ref CJK_UNICHAR in the list. In the case the index already is in the 
 * bestlist, the index is moved to the input position, otherwise it is 
 * inserted, and the other indices are forced one step down the list. To keep
 * the list ordered due to the distance, the insert distance @ref dist is 
 * changed to fit in the list.
 */ 
DECUMA_HWR_PRIVATE void  cjkBestListInsertAt(CJK_UNICHAR unichar, DLTDB_INDEX index, CJK_BESTLIST_POSITION pos, CJK_SESSION * pSession);


/**
 * This is a little function that saves code space and typing effort. It also copies
 * the distance measure of the first position into the newly added character.
 * 
 * The uncommented code could also be used but after a minor fix in @ref cjkBestListInsertAt
 * this was not necessary.
 */ 
DECUMA_HWR_PRIVATE void  cjkBestListInsertFirst(CJK_UNICHAR unichar, CJK_SESSION * pSession);


/**
 * This is a little function that is actually constructed for chinese character
 * 3005 for duplication of previous character. This only replaces the name.
 */
DECUMA_HWR_PRIVATE void  cjkBestListReplace(CJK_UNICHAR oldunichar, CJK_UNICHAR unichar, CJK_SESSION * pSession);


/**
 * A function for removing a candidate from the bestlist.
 */
DECUMA_HWR_PRIVATE void cjkBestListRemove(CJK_BESTLIST * bl, int index);


/**
 * A function for removing recognized gestures from candidate list.
 */
DECUMA_HWR_PRIVATE void cjkBestListRemoveGestures(CJK_BESTLIST * bl);


/**
 * This function inserts a list of indices, which is input, at the top of the
 * bestlist, which is also given as an input. The function @ref cjkBestListInsertAt is
 * used for inputting the unichars into bestlist.
 */ 
DECUMA_HWR_PRIVATE void  cjkBestListInsertMany(const CJK_UNICHAR * unicharlist, CJK_SESSION * pSession);


/**
 * This function looks through the bestlist for the characters in the uclist and 
 * boost them to the specified position. 
 * 
 * NB!!!!! The uclist is expected to be null-terminated.
 */
DECUMA_HWR_PRIVATE void  cjkBestListBoost(CJK_UNICHAR * uclist, CJK_BESTLIST_POSITION pos, CJK_SESSION * pSession);


/**
 * Move the character with unicode @ref unichar if it is on first
 * place in bestlist. It is put down to 4th place in bestlist. The other unichars
 * are moved towards the top of the list.
 */
DECUMA_HWR_PRIVATE void  cjkBestListDiminish(CJK_UNICHAR unichar, CJK_SESSION * pSession);


/** 
 * Swap two elements in the bestlist.
 */ 
DECUMA_HWR_PRIVATE void  cjkBestListSwap(CJK_SESSION * pSession, CJK_BESTLIST_POSITION pos1, CJK_BESTLIST_POSITION pos2);


/**
 * This function returns the position of the character @ref unichar in the
 * bestlist. If @ref unichar is not in bestlist, -1 is returned.
 */
DECUMA_HWR_PRIVATE CJK_BESTLIST_POSITION cjkBestListGetPosition(CJK_UNICHAR unichar, CJK_SESSION * pSession);


/**
 * This function is called after a distance based bestlist has been computed.
 * It modifies each distance by increasing it proportional to the
 * frequency of the characters in the bestlist. A distance of 150 can become
 * 150 for a very frequent character and 180 for a very unfrequent character
 * after this mangling.
 */ 
DECUMA_HWR_PRIVATE void  cjkBestListFixFrequencies(CJK_SESSION * pSession);

/**
 * This function sets the element at pos explicitly and expects the caller to know what he/she is doing.
 */

DECUMA_HWR_PRIVATE void cjkBestListSetElement(CJK_BESTLIST * pBl, CJK_UNICHAR uc, DLTDB_INDEX index, CJK_DISTANCE dist, CJK_BESTLIST_POSITION pos);

/**
 * This function uses a more costly distance function to resort the top candidates in the bestlist.
 */

DECUMA_HWR_PRIVATE DECUMA_STATUS cjkBestListZoom(CJK_COMPRESSED_CHAR * c, CJK_SESSION * pSession, int bUseFreq);

/** 
 * OBS! This function has been obsolete and instead everything is included in the 
 * database. The special checks of this function have therefore to some extent been 
 * translated into @ref cjkBestListSpecialCheck.
 * 
 * This method takes an empty bestlist and fills it with unicodes for
 * punctuation symbols. The decision on which punctuation on first place is
 * based on the shape of the written character @ref c.
 * 
 * The punctuation symbols treated in this function is listed in
 * Table \ref{punclist}.
 *
 * \f{table}
 * \begin{center}
 * \begin{tabular}{|c|c|l|p{50mm}|}
 * \hline
 * symbol & unicode & name & comment \\
 * \hline
 * \hline
 * , & 002C & fullwidth comma & \\
 * \hline
 * . & 002e & full stop & it is $<$narrow$>$ because it could be in an
 * email-address\\
 * \hline
 * " & ff02 & full width quotation mark & This quotation mark has no direction.\\
 * \hline
 *   & 201c & left double quotation mark & This quotation mark opens a
 * quotation.\\
 * \hline
 *   & 201d & right double quotation mark & This quotation mark finishes a
 * quotation.\\
 * \hline
 *   & 2018 & left single quotation mark & This quotation mark opens a
 * quotation inside another quotation.\\
 * \hline
 *   & 2019 & right single quotation mark & This quotation mark finishes a
 * quotation inside another quotation.\\
 * \hline
 *  & 3001 & ideographic comma & This symbol is $<$narrow$>$. There is
 * a halfwidth ideographic comma at unicode ff64. That symbol is only
 * used in Japan, and is not within this project.\\
 * \hline
 *  & 3002 & ideographic full stop & This symbol is $<$narrow$>$. There
 * is a halfwidth ideographic full stop at unicode ff61. That symbol is
 * only used in Japan, and is not within this project.\\
 * \hline
 * ; & ff1b & fullwidth semicolon & The symbol is treated both as a big
 * and a small punctuation symbol, since the user sometimes write it big
 * and sometimes small.\\
 * \hline
 * : & ff1a & fullwidth colon & The symbol is treated both as a big and
 * a small punctuation symbol, since the user sometimes write it big and
 * sometimes small.\\
 * \hline
 * \end{tabular}
 * \end{center}
 * \caption{The punctuation symbols treated in the function cjkBestListPunctuationCheck}
 * \label{punclist}
 * \f}
 *
 * Examples of the small symbols are stored in
 * db/d_punctuation_small.arcs.
 * \\
 * \\
 * Other punctuation symbols, that are of bigger size, and treated as
 * ordinary characters are presented in Table \ref{bigpunclist}.
 *
 * \f{table}
 * \begin{center}
 * \begin{tabular}{|c|c|l|p{50mm}|}
 * \hline
 * symbol & unicode & name & comment \\
 * \hline
 * \hline
 * ! & ff01 & fullwidth exclamation mark & \\
 * \hline
 * ! & ff03 & fullwidth number sign & \\
 * \hline
 * ? & ff1f & fullwidth question mark & \\
 * \hline
 * $@$ & 0040 & commercial at & The size is not fullwidth, but $<$wide$>$,
 * because it is mostly used together with latin characters. \\
 * \hline
 * ; & ff1b & full width semicolon & The symbol is treated both as a big
 * and a small punctuation symbol, since the user sometimes write it big
 * and sometimes small.\\
 * \hline
 * : & ff1a & full width colon & The symbol is treated both as a big and
 * a small punctuation symbol, since the user sometimes write it big and
 * sometimes small.\\
 * \hline
 * ( & ff08 & full width left paranthesis & \\
 * \hline
 * ) & ff09 & full width right paranthesis & \\
 * \hline
 * , & 002C & fullwidth comma & The symbol is treated both as a big and
 * a small punctuation symbol, since the user sometimes write it big and
 * sometimes small.\\
 * \hline
 * " & ff02 & full width quotation mark & This quotation mark has no direction.
 * The symbol is treated both as a big and a small punctuation symbol, since the
 * user sometimes write it big and sometimes small.\\
 * \hline
 *   & 201c & left double quotation mark & This quotation mark opens a
 * quotation. The symbol is treated both as a big and a small punctuation symbol,
 *  since the user sometimes write it big and sometimes small.\\
 * \hline
 *   & 201d & right double quotation mark & This quotation mark finishes a
 * quotation. The symbol is treated both as a big and a small punctuation symbol,
 *  since the user sometimes write it big and sometimes small.\\
 * \hline
 * \end{tabular}
 * \end{center}
 * \caption{Other punctuation symbols, that are of bigger size, and
 * treated as ordinary characters.}
 * \label{bigpunclist}
 * \f}
 *
 * Other punctuations and symbols that are desirable in the final program
 * are listed in file punctuations.txt.
 * For every punctuation there is a predefined bestlist.
 * 
 * First all small punctuations are put into the bestlist.
 * Thereafter special checks are done to put the best candidate first.
 */
DECUMA_HWR_PRIVATE DECUMA_INT32 cjkBestListPunctuationCheck(CJK_COMPRESSED_CHAR * c, CJK_SESSION * pSession);


/**
 * This method takes a bestlist and checks if the first choice in bestlist
 * is obviously wrong. In that case that character is removed from the
 * bestlist.
 */ 
DECUMA_HWR_PRIVATE void  cjkBestListPreCheck(CJK_COMPRESSED_CHAR * c, CJK_SESSION * pSession);


DECUMA_HWR_PRIVATE void  cjkBestListSpecialCheck(CJK_COMPRESSED_CHAR * c, CJK_SESSION * pSession);


/**
 * This method takes a bestlist and the corresponding input character and
 * checks if the character is a CAPITAL or a small letter.
 * Local features are analysed for some characters.
 * It fills in the bestlist, with the new proposals.
 */
DECUMA_HWR_PRIVATE void  cjkBestListCapsCheck(CJK_COMPRESSED_CHAR * c, CJK_SESSION * pSession);


/**
 * There are a number of pairs of charachers that are exactly identical
 * except that the radical is either "speaking" or "three drops". Here
 * they are. The even numbers are "speaking" and the odd numbers are the
 * corresponding "three drops". They are from both simplified and traditional
 * chinese. The list is null terminated so that we easily can search to the end.
 * 
 * If one kind is found then we investigate if it is not the other
 * kind. One problem is that a speaking might have been written
 * using a traditional speaking so the code must guard for that
 * too.
 */ 
DECUMA_HWR_PRIVATE void  cjkBestListThreeDropsCheck(CJK_COMPRESSED_CHAR * c, CJK_SESSION * pSession);


/**
 * This method takes a bestlist and checks if there are any characters in
 * the bestlist that need to be complemented by other characters. This is
 * the absolutely final check. No changes shall be done in the bestlist after
 * this check. Special care is needed in order not to "rechange" fixes.
 */ 
DECUMA_HWR_PRIVATE void  cjkBestListFinalCheck(CJK_SESSION * pSession);


#endif /* CJK_BEST_LIST_H_ */
