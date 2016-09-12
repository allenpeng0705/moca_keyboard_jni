/*******************************************************************************
;*******************************************************************************
;**                                                                           **
;**                    COPYRIGHT 2004-2011 NUANCE COMMUNICATIONS              **
;**                                                                           **
;**               NUANCE COMMUNICATIONS PROPRIETARY INFORMATION               **
;**                                                                           **
;**     This software is supplied under the terms of a license agreement      **
;**     or non-disclosure agreement with Nuance Communications and may not    **
;**     be copied or disclosed except in accordance with the terms of that    **
;**     agreement.                                                            **
;**                                                                           **
;**                                                                           **
;*******************************************************************************
;**                                                                           **
;**     FileName:  et9cstrie.c                                                **
;**                                                                           **
;**  Description:  Implementation of SBI trie                                 **
;**                                                                           **
;**                                                                           **
;*******************************************************************************
;******************************************************************************/

#include "et9api.h"
#include "et9cpldb.h"
#include "et9cpsys.h"
#include "et9misc.h"
#include "et9cppbuf.h"
#include "et9cpspel.h"
#include "et9cstrie.h"
#include "et9cptone.h"

#include "et9cpkey.h"
#include "et9cprdb.h"

#ifdef ET9_DIRECT_LDB_ACCESS
#define DECLARE_TRIE_NODE(pNode)   const ET9U8 ET9FARDATA *pNode
#define  TRIE_LOAD_NODE( pTrie, iNodeOffset, pTopNode, pbNodeSize )    Trie_LoadNode( pTrie, iNodeOffset, &(pTopNode), pbNodeSize )
#else
#define DECLARE_TRIE_NODE(pNode)   ET9U8 pNode[ET9_CS_LARGE_NODE_SIZE]
#define  TRIE_LOAD_NODE( pTrie, iNodeOffset, pTopNode, pbNodeSize )    Trie_LoadNode( pTrie, iNodeOffset, pTopNode, pbNodeSize )
#endif

/* SBI Udb Search */
typedef enum ET9_CP_EnumPrefixType_e {
    ePrefixNone = 0,    /* 1 and 2 do not match */
    ePrefix1    = 1,    /* 1 is prefix of 2 */
    ePrefix2    = 2,    /* 2 is prefix of 1 */
    ePrefixBoth = 3     /* 1 and 2 match each other (used as bit mask) */
} ET9_CP_EnumPrefixType;


static ET9STATUS ET9LOCALCALL ET9_CP_GetUdbSpellings(ET9CPLingInfo * pET9CPLingInfo, const ET9SymbInfo *pSymbInfo, ET9S32 iLen, ET9_CS_CandidateGroup* pCandGrp);
static ET9STATUS ET9LOCALCALL ET9_CS_AddUdbPrefix(ET9CPLingInfo * pET9CPLingInfo, const ET9SymbInfo *pSymbInfo, ET9S32 iLen);


#define ET9_CS_CONTAIN_UNUSED_FUNCTION  0
#define ET9_CS_PARTIAL_EXACT_THRESHOLD   1000
#define ET9_CS_MULTIPLE_SEG_THRESHOLD    750

#define ET9_CS_HEADER_SIZE_SBI_PREFIX  (sizeof(ET9U8) + sizeof(ET9S32))

#define ET9_CS_LARGE_NODE_SIZE   6
#define ET9_CS_SMALL_NODE_SIZE   4

#define  ET9_CS_MAX_SIBLING_DIS   0x00007FFE

#define ET9_CS_BPMF_FIRST_LOWER_IN_TRIE      0x21
#define ET9_CS_BPMF_FIRST_UPPER_IN_TRIE      0x51

typedef enum ET9_CS_EnumNodeType_e { eChar, eTail } ET9_CS_EnumNodeType;

#define ET9_CS_NODE_TYPE( pNode )          ( ( (pNode)[0] & 0x80)? eTail: eChar )
#define ET9_CS_END_OF_WORD( pNode )  ( ((pNode)[1] & 0x80) != 0 )
#define ET9_CS_GET_CHAR( pTrie, pNode )           (pTrie)->arrEncodingTrieToInternal[ (pNode)[0] & 0x7f ]
#define ET9_CS_TAIL_STRING_LENGTH( pNode )  ((ET9U32)(((pNode)[2] >> 4) & 0xFF) + 2)
#define ET9_CS_TAIL_STR_OFFS_INDEX( pNode )  (((ET9U32)((pNode)[0] & 0x3F) << 8) | (ET9U32)(pNode)[1])
#define ET9_CS_NODE_SIZE( pNode, eType )   (((eType) == eChar && ET9_CS_END_OF_WORD(pNode))? ET9_CS_LARGE_NODE_SIZE: ET9_CS_SMALL_NODE_SIZE)
#define ET9_CS_WORD_PROB( pNode, eType )   (((eType) == eTail)? (((ET9U32)((pNode)[2] & 0x0F) << 8) | ((ET9U32)(pNode)[3])): ET9_CS_END_OF_WORD(pNode)? (((ET9U32)((pNode)[4] & 0x0F) << 8) | (ET9U32)(pNode)[5]): 0)
#define ET9_CS_MAX_DES_PROB( pNode, eType ) (((eType) == eTail)? (((ET9U32)((pNode)[2] & 0x0F) << 8) | ((ET9U32)(pNode)[3])): (((ET9U32)(pNode)[3] << 4) | 0x0F))
#define ET9_CS_SIBLING_DIS_INDEX( pNode )   ( ( ((ET9U32)((pNode)[1] & 0x7F) << 8) | ((ET9U32)(pNode)[2] & 0xFE) ) >> 1 )  /* for eChar only */
#define ET9_CS_IS_LAST_SYLLABLE_OF_WORD( pNode )   ( (pNode)[2] & 0x01 )  /* for eChar only */
#define ET9_CS_CHILD_DISTANCE(pNode, eType)  (((eType) == eChar && ET9_CS_SIBLING_DIS_INDEX(pNode) >= 3)? ET9_CS_NODE_SIZE(pNode, eType): 0)


#define ET9_CS_LOG_PROB_SHIFT(pTrie)           ((pTrie)->iLogProbShift)
#define ET9_CS_PROB_PPRODUCT(pTrie, nLogProb1, nLogProb2 )  ((nLogProb1) + (nLogProb2) - ET9_CS_LOG_PROB_SHIFT(pTrie))

#define ET9_CS_LINGUISTIC_INFO(pTrie)    (pTrie)->pET9CPLingInfo
#define ET9_CS_WORDSYMBOL_INFO(pTrie)    (pTrie)->pET9CPLingInfo->Base.pWordSymbInfo

#define ET9_CS_LETTER_IS_C_S_Z(c) ((c) == 'C' || (c) == 'S' || (c) == 'Z')

#define ET9_CS_IsPartialSpellActive(pLing)  ( ET9CPIsPartialSpellActive(pLing) && !ET9_CP_InputHasTone(pLing) )
#define ET9_CS_IsCharInSymbInfo(c, pSymbInfo)   \
    ( (pSymbInfo)->sLockedSymb == 0? ET9_CP_IsSymbInSymbInfo((ET9SYMB)(c), (pSymbInfo)): \
          ET9_CP_IsBpmfLetter(c)? ET9_CP_BpmfExternalToInternal((pSymbInfo)->sLockedSymb) == (ET9SYMB)(c) : (pSymbInfo)->sLockedSymb == (ET9SYMB)(c) )

/*************************************************************************
 *
 * static functions
 *
 *************************************************************************/


ET9INLINE static ET9BOOL ET9LOCALCALL EndsWithLeadingLetter(ET9U8* pcStr, ET9S32  iLen)
{
    if ( iLen > 0 && (pcStr[iLen-1] == ET9CPSYLLABLEDELIMITER /* || ET9CPSymToCPTone(pcStr[iLen-1]) */ ) )
        iLen--;
    if ( iLen > 0 && ET9_CP_IsUpperCase(pcStr[iLen-1]) )
        return 1;
    if ( iLen > 1 && pcStr[iLen-1] == 'h' && ET9_CS_LETTER_IS_C_S_Z(pcStr[iLen-2]) )
        return 1;
    return 0;
}

static ET9BOOL ET9LOCALCALL ET9_CP_SymbInfoIsValid(const ET9SymbInfo *pSymbInfo)
{
    int i, j;
    ET9Assert(pSymbInfo != NULL);

    for ( i = 0; i < /*pSymbInfo->bNumBaseSyms*/1; i++ )
    {
        for ( j = 0; j < pSymbInfo->DataPerBaseSym[i].bNumSymsToMatch; j++ )
        {
            ET9SYMB symb = pSymbInfo->DataPerBaseSym[i].sChar[j];
            if ( ET9CPIsPhoneticSymbol(symb) || symb >= '0' && symb <= '9' )
                return 1;
            if ( ET9CPSYLLABLEDELIMITER == symb || ET9CPSymToCPTone(symb) )
            {
                if (1 == pSymbInfo->bNumBaseSyms && 1 == pSymbInfo->DataPerBaseSym[0].bNumSymsToMatch )
                    return 1;
                else
                    return 0; /* reject if delimiter or tone is not the only symb */
            }
        }
    }
    return 0;
}

static ET9BOOL ET9LOCALCALL ET9_CP_IsValidSBIInput(const ET9SymbInfo * pSymbInfo, ET9U8 bLen)
{
    ET9U8 i;
    ET9Assert(pSymbInfo);
    for (i = 0 ; i < bLen; i++) {
        if ( !ET9_CP_SymbInfoIsValid(pSymbInfo + i) ) {
            return 0;
        }
    }
    return 1;
}

static ET9BOOL ET9LOCALCALL ET9_CP_IsValidWSITone(
    ET9WordSymbInfo *pWSI,
    ET9BOOL *pbHasTone)
{
    ET9U8 i, b1stTone, bLen, bStart;
    ET9SymbInfo * pSymbInfo;
    ET9Assert(pWSI);
    ET9Assert(pbHasTone);

    bLen = pWSI->bNumSymbs;
    pSymbInfo = pWSI->SymbsInfo;
    b1stTone = 0;
    *pbHasTone = 0;
    for (i = 0 ; i < bLen; i++) {
        if (ET9_CP_SymbIsTone(pSymbInfo + i) ) {
            if ( i == 0 ) { /* First symbol is tone, invalid */
                return 0;
            }
            else if (0 == b1stTone) { /* mark the 1st tone position */
                b1stTone = i;
            }
            else { /* more than 1 tone ==> invalid */
                return 0;
            }
        }
    }
    if (b1stTone != 0) { /* has tone */
        ET9U8 bUpperCount;

        bUpperCount = 0;
        for (i = 0; i <= b1stTone; i++) {
            if (0 == (pSymbInfo + i)->sLockedSymb) {
                /* missing locked symb before 1st tone ==> invalid */
                return 0;
            }
            else if (ET9CPIsUpperCaseSymbol((pSymbInfo + i)->sLockedSymb) ) {
                bUpperCount++;
            }
        }
        if (bUpperCount > 1) { /* more than 1 Upper case in locked symb ==> 1st tone is not on 1st syl ==> invalid */
            return 0;
        }
    }
    bStart = (ET9U8)(b1stTone? b1stTone + 1: 0);
    for (i = bStart ; i < bLen; i++) {
        if ( (pSymbInfo + i)->sLockedSymb ) {
            /* clear all locked symb after the 1st tone for SBI search using locked symb */
            (pSymbInfo + i)->sLockedSymb = 0;
            pWSI->Private.ppEditionsList[ET9EDITION_CP]->bLockInvalidated[i] = 1;
        }
    }
    *pbHasTone = (ET9BOOL)(b1stTone != 0);
    return 1;
}

static ET9U8 ET9LOCALCALL read_u8( const ET9U8 * pcData )
{
    ET9Assert( pcData != NULL );
    return pcData[0];
}

static ET9BOOL ET9LOCALCALL write_u8( ET9U8 * pcData, ET9U8 u )
{
    ET9Assert( pcData != NULL );
    pcData[0] = u;
    return 1;
}

/* Add delimiter according pSymbInfo (nSrcLen is the length of pcSrc). */
ET9S8 ET9FARCALL ET9_CP_CopyAddDelimiter(
    ET9U8* pcTgt,
    ET9S32 iTgtSize,
    ET9S32* piCharCopied,
    const ET9U8* pcSrc,
    ET9S32 iSrcLen,
    const ET9SymbInfo *pSymbInfo,
    ET9S32 iSymbLen )
{
    ET9S32 iComsumed = 0, iSymbUsed = 0, iCopied;
    while ( iSrcLen && iSymbUsed < iSymbLen && iComsumed < iTgtSize)  {
        if ( iComsumed < iSymbLen
            && ET9CPSYLLABLEDELIMITER != *pcSrc
            && 0 == ET9CPSymToCPTone(*pcSrc)
            && ET9_CP_SymbIsToneOrDelim(pSymbInfo) )
        {
            *pcTgt++ = ET9_CP_GetSymbToneOrDelim(pSymbInfo);
        }
        else  {
            *pcTgt++ = *pcSrc++;
            iSrcLen--;
        }
        pSymbInfo++;
        iSymbUsed++;
        iComsumed++;
    }

    iCopied = iComsumed;
    while ( iSrcLen && iCopied < iTgtSize) {
        *pcTgt++ = *pcSrc++;
        iSrcLen--;
        iCopied++;
    }
    if (piCharCopied) {
        *piCharCopied = iCopied;
    }
    return (ET9S8)iComsumed;
}

static ET9S32 ET9LOCALCALL ET9_CP_CopyExcludeDelimiter(
    ET9U8 * pTgt,
    const ET9U8 * pSrc,
    ET9S32 iSrcLen )
{
    const ET9U8 *p;
    ET9U8 *t = pTgt;
    ET9Assert( pTgt != NULL && pSrc != NULL );
    for ( p = pSrc; iSrcLen > 0; p++, iSrcLen-- )  {
        if ( *p != ET9CPSYLLABLEDELIMITER && 0 == ET9CPSymToCPTone(*p) )
        {
            *t++ = *p;
        }
    }
    return t - pTgt;
}

/** \internal
 * Check if 2 symbs are equal under the set of Mohu rules in wMohu
 *
 * @param wMohu       bit flags of Mohu rules
 * @param sTrie       symb 1
 * @param sInput      symb 2
 *
 * @return 1 if the 2 symbs are equal under the set of Mohu rules
 */
static const ET9U32 gdwSymbMohuEq[] =
{
0x00020001, 0x00080004, 0x00200010, 0x00800040, 0x02000100, 0x08000400, 0x00001000, 0x00000000,
0x00020001, 0x00080004, 0x00200010, 0x00800140, 0x02000140, 0x08000400, 0x00001000, 0x00000000,
0x00020001, 0x00080004, 0x00200010, 0x00801040, 0x02000100, 0x08000400, 0x00001040, 0x00000000,
0x00020001, 0x00080004, 0x00200010, 0x00801140, 0x02000140, 0x08000400, 0x00001040, 0x00000000,
0x00020005, 0x00080005, 0x00200010, 0x00800040, 0x02000100, 0x08000400, 0x00001000, 0x00000000,
0x00020005, 0x00080005, 0x00200010, 0x00800140, 0x02000140, 0x08000400, 0x00001000, 0x00000000,
0x00020005, 0x00080005, 0x00200010, 0x00801040, 0x02000100, 0x08000400, 0x00001040, 0x00000000,
0x00020005, 0x00080005, 0x00200010, 0x00801140, 0x02000140, 0x08000400, 0x00001040, 0x00000000
};

#define ID_SymbMohuEq(wMohu, sTrie, sInput)    ( ( ((wMohu) & 0x0038) << 5 ) | ( ( (sTrie) - 'F' ) << 4 ) | ( (sInput) - 'F' ) )
#define SymbMohuEqual(wMohu, sTrie, sInput)   \
   ( ( (sTrie) < 'F' || (sTrie) > 'R' || (sInput) < 'F' || (sInput) > 'R' )? ( (sTrie) == (sInput) ):  \
     ( (gdwSymbMohuEq[ (ID_SymbMohuEq(wMohu, sTrie, sInput) >> 5) ] >> (ID_SymbMohuEq(wMohu, sTrie, sInput) & 0x1F) ) & 0x1 ) )

/** \internal
 * Check if ET9SymbInfo contains (in its lower case array and upper case array) a letter under Mohu rule wMohu
 *
 * @param wMohu       combination of Mohu flags
 * @param cInTrie     char (internal encoding)
 * @param pSymbInfo   an ET9SymbInfo
 *
 * @return matching letter in pSymbInfo if c is in pSymbInfo,  0 if c is not in pSymbInfo
 */
ET9INLINE static ET9U8 ET9LOCALCALL ET9_CP_CharSymbMohuMatch(ET9U16 wMohu, ET9U8 cInTrie, const ET9SymbInfo *pSymbInfo)
{
    ET9SYMB cUni;
    ET9U8 i, j;

    if (ET9_CP_IsBpmfLetter(cInTrie))
    {
        if ( pSymbInfo->sLockedSymb )
        {
            if ( ET9_CP_BpmfExternalToInternal(pSymbInfo->sLockedSymb) == cInTrie )
                return cInTrie;
        }
        else
        {
            cUni = ET9_CP_BpmfLetterToUnicode(cInTrie); /* ignore upper/lower case of cInTrie */
            for (i = 0; i < /*pSymbInfo->bNumBaseSyms*/1; i++)
            {
                for (j = 0; j < pSymbInfo->DataPerBaseSym[i].bNumSymsToMatch; j++)
                {   /* BPMF doesn't need mohu compare */
                    if (pSymbInfo->DataPerBaseSym[i].sChar[j] == cUni) {
                        /* cUni is Unicode equivalent of cInTrie */
                        return cInTrie;
                    }
                }
            }
        }
    }
    else /* Pinyin letter */
    {
        cUni = (ET9SYMB)cInTrie;
        if ( pSymbInfo->sLockedSymb )
        {
            if ( SymbMohuEqual(wMohu, cUni, pSymbInfo->sLockedSymb) )
                return (ET9U8)pSymbInfo->sLockedSymb;
        }
        else
        {
            ET9BOOL bIsUpper = (ET9BOOL)ET9_CP_IsPinyinUpperCase(cInTrie);
            if (bIsUpper) {
                for (i = 0; i < /*pSymbInfo->bNumBaseSyms*/1; i++)
                {
                    for (j = 0; j < pSymbInfo->DataPerBaseSym[i].bNumSymsToMatch; j++)
                    {
                        if (SymbMohuEqual(wMohu, cUni, pSymbInfo->DataPerBaseSym[i].sUpperCaseChar[j])) {
                            return (ET9U8)pSymbInfo->DataPerBaseSym[i].sUpperCaseChar[j];
                        }
                    }
                }
            }
            else {
                for (i = 0; i < /*pSymbInfo->bNumBaseSyms*/1; i++)
                {
                    for (j = 0; j < pSymbInfo->DataPerBaseSym[i].bNumSymsToMatch; j++)
                    {
                        if (cUni == pSymbInfo->DataPerBaseSym[i].sChar[j]){
                            /* lower case doesn't need mohu compare */
                            return (ET9U8)pSymbInfo->DataPerBaseSym[i].sChar[j];
                        }
                    }
                }
            }
        }
    }

    return 0;
}

/** \internal
 * Check if ET9SymbInfo contains (in its lower case array and upper case array) a letter
 * If c is BPMF letter, it is converted to Unicode, then do the checking
 *
 * @param c           a char
 * @param pSymbInfo   an ET9SymbInfo
 *
 * @return 1 if c is in pSymbInfo, 0 if c is not in pSymbInfo
 */
static ET9INT ET9LOCALCALL ET9_CP_CharSymbMatch(ET9U8 c, const ET9SymbInfo *pSymbInfo)
{
    ET9U8 i, j;
    ET9SYMB s;
    if ( pSymbInfo->sLockedSymb )
    {
        if (ET9_CP_IsBpmfLetter(c))
        {
            if ( ET9_CP_BpmfExternalToInternal(pSymbInfo->sLockedSymb) == c )
                return 1;
        }
        else if ( (ET9U8)pSymbInfo->sLockedSymb == c )
            return 1;

        return 0;
    }
    else
    {
        if (ET9_CP_IsBpmfLetter(c))
            s = ET9_CP_BpmfLetterToUnicode(c);
        else
            s = (ET9SYMB)c;
        for (i = 0; i < /*pSymbInfo->bNumBaseSyms*/1; i++)  {
            for (j = 0; j < pSymbInfo->DataPerBaseSym[i].bNumSymsToMatch; j++) {
                if (s ==  pSymbInfo->DataPerBaseSym[i].sChar[j] || s ==  pSymbInfo->DataPerBaseSym[i].sUpperCaseChar[j]) {
                    return 1;
                }
            }
        }
    }
    return 0;
}

static ET9_CP_EnumPrefixType ET9LOCALCALL ET9_CP_StrIsPrefixOf( const ET9U8 * pc1, ET9S32 iLen1, const ET9SymbInfo *pSymbInfo, ET9S32 iLen2 )
{
    ET9BOOL bExpectUpper = 0;
    ET9Assert( pc1 != NULL && pSymbInfo != NULL );

    while ( iLen1 > 0 )  {
        if ( iLen2 == 0 ) {
            return ePrefix2;
        }
        if ( *pc1 == ET9CP_SEGMENT_DELIMITER ) {
            iLen1--;
            pc1++;
            bExpectUpper = 1;
            continue;
        }
        if ( ET9_CP_SymbIsToneOrDelim(pSymbInfo) ) {
            if ((ET9SYMB)(*pc1) != pSymbInfo->DataPerBaseSym[0].sChar[0]) {
                return ePrefixNone;
            }
            bExpectUpper = 1;
        }
        else {
            if ( !ET9_CP_CharSymbMatch(*pc1, pSymbInfo) ) {
                return ePrefixNone;
            }
            if ( bExpectUpper ) {
                if ( !ET9_CP_IsUpperCase( *pc1 ) ) {
                    return ePrefixNone;
                }
                bExpectUpper = 0;
            }
        }
        pc1++;
        pSymbInfo++;
        iLen1--;
        iLen2--;
    }
    if ( iLen2 == 0 ) {
        return ePrefixBoth;
    }
    else {
        return ePrefix1;
    }
}

typedef struct ET9_CS_NodeInStack_S
{
    ET9S32 m_iNodeOffset;
    ET9S32 m_iChildDistance;
    ET9S32 m_iConsumed;
    ET9U8  m_bFlags;             /* Bit flags for ET9_CS_INSERT_INPUT_H, ET9_CS_IGNORE_INPUT_H, INSERT_NODE_G, IGNORE_NODE_G */
    ET9S32 m_iIndex;             /* index to both pSymbInfo and acBuf */
} ET9_CS_NodeInStack;

typedef struct ET9_CS_SearchPrivateData_S
{
    ET9U8                m_acBuf[ET9_CP_MAX_CANDIDATE_LENGTH];  /* 64 must be > ET9_CP_MAX_SPELL_LENGTH + 1 */
    ET9_CS_NodeInStack   m_node;
    ET9S32               m_iStackIndex;
    ET9_CS_NodeInStack   m_aStack[ ET9_CP_MAX_TREE_DEPTH ];
} ET9_CS_SearchPrivateData;

#define ET9_CS_ZERO_INIT_SearchPrivateData(data) _ET9ByteSet((ET9U8*)&data, sizeof(data), (ET9U8)0)

#define ET9_CS_INSERT_INPUT_H  1
#define ET9_CS_IGNORE_INPUT_H  2
#define ET9_CS_TRIED_RULES_H  (ET9_CS_INSERT_INPUT_H | ET9_CS_IGNORE_INPUT_H)
#define ET9_CS_INSERT_INPUT_G  4
#define ET9_CS_IGNORE_INPUT_G  8
#define ET9_CS_TRIED_RULES_G  (ET9_CS_INSERT_INPUT_G | ET9_CS_IGNORE_INPUT_G)
#define SHOULD_TRY_H_RULE(wMohu, c)                               \
        ( (wMohu & ET9CPMOHU_PAIR_C_CH_MASK) != 0 && c == 'C' ||  \
          (wMohu & ET9CPMOHU_PAIR_S_SH_MASK) != 0 && c == 'S' ||  \
          (wMohu & ET9CPMOHU_PAIR_Z_ZH_MASK) != 0 && c == 'Z' )

#define SHOULD_TRY_G_RULE(wMohu, c1, c2)                             \
      ( c2 == 'n' &&                                                 \
        ( (wMohu & ET9CPMOHU_PAIR_AN_ANG_MASK) != 0 && (c1 == 'a' || c1 == 'A') ||  \
          (wMohu & ET9CPMOHU_PAIR_EN_ENG_MASK) != 0 && (c1 == 'e' || c1 == 'E') ||  \
          (wMohu & ET9CPMOHU_PAIR_IN_ING_MASK) != 0 && c1 == 'i' ) )

static ET9UINT ET9LOCALCALL ET9_CP_FindNextUpperCase(const ET9U8 *pc, ET9S32 iLen)
{
    ET9UINT nSkipped;
    ET9S32 i;

    ET9Assert(pc && iLen);

    /* skip to the 1st upper case letter, which is the beginning of the next syl */
    nSkipped = 0;
    for ( i = 0; i < iLen; i++ )
    {
        if ( ET9_CP_IsUpperCase(pc[i]) )
            break;

        nSkipped++;
    }
    return nSkipped;
}

/*************************************************************************
 *
 * struct ET9_CP_SSBIPrefixBuffer
 *
 *************************************************************************/
static void ET9LOCALCALL PrefixBufferClear( ET9_CP_SSBIPrefixBuffer * pBuffer )
{
    ET9Assert( pBuffer != NULL );
    pBuffer->m_bPrefixCount = 0;
    pBuffer->m_pcPrefixBufUnused = pBuffer->m_acPrefixBuf;
}

ET9BOOL ET9_CS_StringGreaterThan(ET9INT fBPMF, const ET9U8 * pcSpell1, ET9U8 bLen1, const ET9U8 * pcSpell2, ET9U8 bLen2)
{
    int i;
    for ( i = 0; i < bLen1 && i < bLen2; i++ )
    {
        if ( fBPMF && ET9_CP_IsBpmfLowerCase(pcSpell1[i]) && ET9_CP_IsBpmfUpperCase(pcSpell2[i]) )
            return 1;
        if ( fBPMF && ET9_CP_IsBpmfUpperCase(pcSpell1[i]) && ET9_CP_IsBpmfLowerCase(pcSpell2[i]) )
            return 0;
        /* Now pcSpell1[i], pcSpell2[i] are both upper-case or both lower-case  */
        if ( pcSpell1[i] > pcSpell2[i] )
            return 1;
        else if ( pcSpell1[i] < pcSpell2[i] )
            return 0;
    }
    if ( bLen1 > bLen2 )
        return 1;
    return 0;
}

static ET9U8 *  ET9LOCALCALL ET9_CS_FindPrefix( ET9_CP_SSBIPrefixBuffer * pBuffer, ET9U8 * pcSpell, ET9U8 bLen)
{
    ET9U8 b;
    ET9U8 * pPrefixBlock = pBuffer->m_acPrefixBuf;
    for ( b = 0; b < pBuffer->m_bPrefixCount; b++ )
    {
        ET9U8 len = *pPrefixBlock;
        if (len == bLen)
        {
            ET9U8* pc = pPrefixBlock + ET9_CS_HEADER_SIZE_SBI_PREFIX;
            ET9U8 i;
            for ( i = 0; i < bLen; i++ )
            {
                if (pcSpell[i] != pc[i])
                    break;
            }
            if ( i == bLen )
                return pPrefixBlock;
        }

        pPrefixBlock += ET9_CS_HEADER_SIZE_SBI_PREFIX + len;
    }
    return NULL;
}

static void ET9LOCALCALL ET9_CS_SetValidPfxLen(ET9_CP_SSBITrie* pTrie)
{
    ET9U8 bGrpCount = pTrie->m_bSegCandGrpCount;
    ET9U8 abTailMatch[ET9_CP_MAX_SBI_KEY_NUMBER];
    ET9INT i;

    if (bGrpCount == 0)
        return;
    _ET9ByteCopy(abTailMatch, pTrie->m_aSegCandGrp[bGrpCount-1].m_abTailMatch, bGrpCount);

    _ET9ByteSet(pTrie->m_abValidPrefix, sizeof(pTrie->m_abValidPrefix), 0xFF);
    for ( i = 0; i < (ET9INT)bGrpCount; i++ )
    {
        if (abTailMatch[i] == eExactMatch || abTailMatch[i] == ePartialMatch)
            pTrie->m_abValidPrefix[i] = 1;
        else if (abTailMatch[i] == ET9CPSYLLABLEDELIMITER)
            pTrie->m_abValidPrefix[i] = ET9CPSYLLABLEDELIMITER;
        else
            pTrie->m_abValidPrefix[i] = eNoMatch;
    }

    if (bGrpCount == 1)
        return;

    for ( i = bGrpCount - 1; i >= 0; i-- )
    {
        ET9INT j;
        ET9U8 * pcLeadingMatch;
        if (abTailMatch[i] == eNoMatch || abTailMatch[i] == ET9CPSYLLABLEDELIMITER || abTailMatch[i] == 0xFF)
            continue;
        pcLeadingMatch = pTrie->m_aSegCandGrp[i-1].m_abTailMatch;
        for ( j = 0; j < i; j++ )
        {
            if (pTrie->m_abValidPrefix[j] == 1)
                continue;
            if (pcLeadingMatch[j] == eExactMatch /*|| pcLeadingMatch[j] == ePartialMatch */)
            {
                abTailMatch[j] = eExactMatch;
                pTrie->m_abValidPrefix[j] = 1;
            }
        }

    }
}

static ET9BOOL ET9LOCALCALL  ET9_CS_IsValidPrefixLen(ET9_CP_SSBITrie* pTrie, ET9U8 bPfxLen)
{
    ET9U8 bGrpCount;
    bGrpCount = pTrie->m_bSegCandGrpCount;
    if ( bPfxLen < bGrpCount )
    {
        if (pTrie->m_abValidPrefix[bPfxLen] == eNoMatch)
            return 0;
        if (pTrie->m_abValidPrefix[bPfxLen] == ET9CPSYLLABLEDELIMITER && bPfxLen + 1 < bGrpCount && pTrie->m_abValidPrefix[bPfxLen + 1] == eNoMatch)
            return 0;
    }
    return 1;
}

extern void ET9FARCALL ET9_CP_MoveBlockForward(ET9U8 *pbBuf, ET9U16 wLen, ET9U16 wMove);

static void ET9LOCALCALL PrefixBufferAddPrefix(ET9INT fBPMF, ET9_CP_SSBITrie* pTrie, ET9U8 * pcPrefix, ET9U8 bPfxLen, ET9S32 iLogProb )
{
    ET9U32 uTotalBytes;
    ET9U8 * pcDuplicate;
    ET9U8 acBuffer[ET9_CP_MAX_SPELL_LENGTH];
    ET9U8 bLen;
    ET9_CP_SSBIPrefixBuffer * pBuffer;

    ET9Assert( pTrie != NULL && pcPrefix != NULL );
    if ( !ET9_CS_IsValidPrefixLen(pTrie, bPfxLen) )
        return;

    pBuffer = &pTrie->m_PrefixBuffer;

    if ( ET9_CP_InputHasTone(ET9_CS_LINGUISTIC_INFO(pTrie)) )
    {
        ET9_CP_Spell sToneSpell;
        _ET9ByteCopy( sToneSpell.pbChars, pcPrefix, bPfxLen);
        sToneSpell.bLen = bPfxLen;
#if 1
        if ( eExactMatch != ET9_CP_ValidateToneSpell(ET9_CS_LINGUISTIC_INFO(pTrie), &sToneSpell, /* bNeedPartialPhrase */0) )
            return;
#else
        if ( !ET9_CP_ValidateToneSpell(ET9_CS_LINGUISTIC_INFO(pTrie), &sToneSpell, /* bNeedPartialSyl */0, /* bNeedPartialPhrase */0) )
            return;
#endif
    }

    bLen = (ET9U8)ET9_CP_CopyExcludeDelimiter( acBuffer, pcPrefix, bPfxLen );

    pcDuplicate = ET9_CS_FindPrefix( pBuffer, acBuffer, bLen);
    if (pcDuplicate)
    {
        ET9S32 iPfxProb = (ET9S32)ET9_CP_ReadU32( pcDuplicate + sizeof(ET9U8) );
        if ( iPfxProb < iLogProb )
            ET9_CP_WriteU32( pcDuplicate + sizeof(ET9U8), (ET9U32)iLogProb );
        return;
    }

    uTotalBytes = ET9_CS_HEADER_SIZE_SBI_PREFIX + bLen;
    ET9Assert(pBuffer->m_pcPrefixBufUnused + uTotalBytes < pBuffer->m_acPrefixBuf + sizeof(pBuffer->m_acPrefixBuf));
    if ( pBuffer->m_pcPrefixBufUnused + uTotalBytes >= pBuffer->m_acPrefixBuf + sizeof(pBuffer->m_acPrefixBuf) ) {
        return;
    }


    /*  BPMF is not natural order: Upper BPMF letter should be less than Lower BPMF letter
       It should be sorted as: e2, e2 e2, e2 99 (natural order is e2, e2 99, e2 e2) */
    /* To group BPMF by first syllable we need to use a special comparison function 'ET9_CS_StringGreaterThan'
       so that Uppercase < Lowercase, as in Pinyin */
    /* Use ET9_CS_StringGreaterThan() to make sure all internal Prefix are in increasing order */
    {
        ET9INT iTailSize;
        ET9U8* pcEntry = pBuffer->m_acPrefixBuf;
        ET9U8 b = 0;
        while ( b < pBuffer->m_bPrefixCount ) {
            ET9U8 bEntryLen = *pcEntry;
            if ( ET9_CS_StringGreaterThan(fBPMF, pcEntry+ET9_CS_HEADER_SIZE_SBI_PREFIX, bEntryLen, acBuffer, bLen) )
                break;
            pcEntry += (ET9_CS_HEADER_SIZE_SBI_PREFIX + bEntryLen);
            b++;
        }
        iTailSize = (pBuffer->m_pcPrefixBufUnused - pcEntry);
        ET9_CP_MoveBlockForward(pcEntry, (ET9U16)iTailSize, (ET9U16)uTotalBytes);
        write_u8( pcEntry, bLen );
        pcEntry++;
        ET9_CP_WriteU32( pcEntry, (ET9U32)iLogProb );
        pcEntry += 4;
        _ET9ByteCopy( pcEntry, acBuffer, bLen);
        pBuffer->m_pcPrefixBufUnused += uTotalBytes;
        pBuffer->m_bPrefixCount++;
    }
}

/*************************************************************************
 *
 * struct ET9_CS_CandidateGroup
 *
 *************************************************************************/
static void ET9LOCALCALL SegCandGrpClear0(ET9_CS_CandidateGroup* pCandGrp)
{
    ET9Assert( pCandGrp != NULL );
    pCandGrp->m_arrExactMatch.m_bCandidateCount = 0;
    pCandGrp->m_arrPartialMatch.m_bCandidateCount = 0;
    pCandGrp->m_eMatchType = eNoMatch;
}

static void ET9LOCALCALL SegCandGrpClear(ET9_CS_CandidateGroup* pCandGrp)
{
    ET9Assert( pCandGrp != NULL );
    pCandGrp->m_arrExactMatch.m_bCandidateCount = 0;
    pCandGrp->m_arrPartialMatch.m_bCandidateCount = 0;
    pCandGrp->m_eMatchType = eNoMatch;
    _ET9ByteSet(pCandGrp->m_abTailMatch, sizeof(pCandGrp->m_abTailMatch), 0xFF);
}

static ET9S32 ET9LOCALCALL GetCandidateCount(const ET9_CS_CandidateArray * pCandidateArray)
{
    ET9Assert( pCandidateArray != NULL );
    return pCandidateArray->m_bCandidateCount;
}

static const ET9_CS_Candidate*  ET9LOCALCALL GetCandidate(const ET9_CS_CandidateArray * pCandidateArray, ET9S32 i)
{
    ET9Assert( pCandidateArray != NULL );
    ET9Assert( i >= 0 && i < pCandidateArray->m_bCandidateCount );
    return pCandidateArray->m_pCandidates +i;
}

static ET9INT ET9LOCALCALL AddCandToArray(ET9_CP_SSBITrie* pTrie, ET9S32 iNodeOffset, ET9S32 iProb, const ET9U8 * pcSeg, ET9U32 uLen, ET9_CS_CandidateArray * pCandidateArray)
{
    ET9BOOL bAddCand; /* add candidate by default */

    ET9Assert( pTrie != NULL );

    /* add candidate if none exist or the new one should overwrite the existing one */
    bAddCand = (ET9BOOL)(0 == pCandidateArray->m_bCandidateCount || pCandidateArray->m_pCandidates[0].m_iSegProb < iProb);

    if (bAddCand)
    {
        if ( ET9_CP_InputHasTone(ET9_CS_LINGUISTIC_INFO(pTrie)) )
        {   /* input contains tone, reject multi-segment cand */
            ET9U8 i;
            for (i = 0; i < uLen; i++)
            {
                if (ET9CP_SEGMENT_DELIMITER == pcSeg[i]) {
                    return 0;
                }
            }
        }
        pCandidateArray->m_pCandidates[0].m_bSegLen = (ET9U8)uLen;
        pCandidateArray->m_pCandidates[0].m_iSegProb = iProb;
        pCandidateArray->m_pCandidates[0].m_iNodeOffset = iNodeOffset;
        _ET9ByteCopy( (ET9U8*)pCandidateArray->m_pCandidates[0].m_pcSeg, pcSeg, uLen);
        pCandidateArray->m_bCandidateCount = 1;
    }
    return 0;
}

static ET9INT ET9LOCALCALL SegCandGrpAdd(ET9_CP_SSBITrie* pTrie, ET9_CS_CandidateGroup* pCandGrp, ET9_CP_SpellMatch eType, ET9S32 iNodeOffset, ET9S32 iProb, const ET9U8 * pcSeg, ET9U32 uLen)
{
    ET9Assert( pTrie != NULL );
    ET9Assert( pCandGrp != NULL );
    ET9Assert( eType != eNoMatch );
    ET9Assert( pcSeg != NULL );

    if ( eType == eExactMatch )
        return AddCandToArray(pTrie, iNodeOffset, iProb, pcSeg, uLen, &pCandGrp->m_arrExactMatch );
    else
        return AddCandToArray(pTrie, iNodeOffset, iProb, pcSeg, uLen, &pCandGrp->m_arrPartialMatch );
}

static void ET9LOCALCALL SegCandGrpSet(ET9_CS_CandidateGroup* pCandGrp, ET9_CP_SpellMatch eType, ET9S32 iProb, const ET9U8 * pcSeg, ET9U32 uLen)
{
    ET9Assert( pCandGrp != NULL );
    ET9Assert( pcSeg != NULL );
    ET9Assert( eType != eNoMatch );

    pCandGrp->m_eMatchType = eType;
    pCandGrp->m_arrExactMatch.m_bCandidateCount = 1;
    pCandGrp->m_arrExactMatch.m_pCandidates[0].m_bSegLen = (ET9U8)uLen;
    pCandGrp->m_arrExactMatch.m_pCandidates[0].m_iSegProb = iProb;
    _ET9ByteCopy( (ET9U8*)pCandGrp->m_arrExactMatch.m_pCandidates[0].m_pcSeg, (ET9U8*)pcSeg, uLen );
}

static ET9_CP_SpellMatch ET9LOCALCALL SegCandGrpType(const ET9_CS_CandidateGroup* pCandGrp)
{
    ET9Assert( pCandGrp != NULL );
    return pCandGrp->m_eMatchType;
}

static void ET9LOCALCALL SegCandGrpCopy(ET9_CS_CandidateGroup* pTgt, ET9_CS_CandidateGroup* pSrc )
{
    ET9U8 i;
    ET9Assert( pTgt != NULL && pSrc != NULL && pTgt != pSrc );

    pTgt->m_eMatchType = pSrc->m_eMatchType;
    pTgt->m_arrExactMatch.m_bCandidateCount = (ET9U8)__ET9Min(pTgt->m_bCandBufferSize, pSrc->m_arrExactMatch.m_bCandidateCount);
    for ( i = 0; i < pTgt->m_arrExactMatch.m_bCandidateCount; i++ )
    {
        ET9U8 bLen = pSrc->m_arrExactMatch.m_pCandidates[i].m_bSegLen;
        pTgt->m_arrExactMatch.m_pCandidates[i].m_bSegLen = bLen;
        pTgt->m_arrExactMatch.m_pCandidates[i].m_iSegProb = pSrc->m_arrExactMatch.m_pCandidates[i].m_iSegProb;
        pTgt->m_arrExactMatch.m_pCandidates[i].m_iNodeOffset = pSrc->m_arrExactMatch.m_pCandidates[i].m_iNodeOffset;
        _ET9ByteCopy( (ET9U8*)pTgt->m_arrExactMatch.m_pCandidates[i].m_pcSeg, (ET9U8*)pSrc->m_arrExactMatch.m_pCandidates[i].m_pcSeg, bLen );
    }
    pTgt->m_arrPartialMatch.m_bCandidateCount = (ET9U8)__ET9Min(pTgt->m_bCandBufferSize, pSrc->m_arrPartialMatch.m_bCandidateCount);
    for ( i = 0; i < pTgt->m_arrPartialMatch.m_bCandidateCount; i++ )
    {
        ET9U8 bLen = pSrc->m_arrPartialMatch.m_pCandidates[i].m_bSegLen;
        pTgt->m_arrPartialMatch.m_pCandidates[i].m_bSegLen = bLen;
        pTgt->m_arrPartialMatch.m_pCandidates[i].m_iSegProb = pSrc->m_arrPartialMatch.m_pCandidates[i].m_iSegProb;
        pTgt->m_arrPartialMatch.m_pCandidates[i].m_iNodeOffset = pSrc->m_arrPartialMatch.m_pCandidates[i].m_iNodeOffset;
        _ET9ByteCopy( (ET9U8*)pTgt->m_arrPartialMatch.m_pCandidates[i].m_pcSeg, (ET9U8*)pSrc->m_arrPartialMatch.m_pCandidates[i].m_pcSeg, bLen );
    }
}

/*************************************************************************
 *
 * struct ET9_CP_SSBITrie
 *
 *************************************************************************/
/*************************************************************************
 *
 * private functions of ET9_CP_SSBITrie
 *
 *************************************************************************/

/*************************************************************************/
#if ET9_CS_CONTAIN_UNUSED_FUNCTION
static ET9BOOL ET9LOCALCALL Trie_HasChildren( ET9_CP_SSBITrie* pTrie, ET9U8* pNode, ET9_CS_EnumNodeType eType );
static ET9BOOL ET9LOCALCALL Trie_HasSibling( ET9_CP_SSBITrie* pTrie, ET9U8* pNode, ET9_CS_EnumNodeType eType );
#endif

static ET9BOOL ET9LOCALCALL Trie_BufMatchPrefix(ET9_CP_SSBITrie* pTrie, ET9U8 * pcBuf, ET9S32 iBufLen);
#if ET9_CS_CONTAIN_UNUSED_FUNCTION
static ET9BOOL ET9LOCALCALL Trie_DelSymb( ET9_CP_SSBITrie* pTrie );
#endif

#ifdef ET9_DIRECT_LDB_ACCESS
ET9INLINE static ET9_CS_EnumNodeType ET9LOCALCALL Trie_LoadNode( ET9_CP_SSBITrie* pTrie, ET9S32 iNodeOffset, const ET9U8 ET9FARDATA **ppNode, ET9U8 * pbNodeSize )
{
    ET9U32 dwReadOffset;
    ET9_CS_EnumNodeType eType;

    dwReadOffset = pTrie->nTrieNodesOffset + iNodeOffset;
    if ( (ET9U32)(iNodeOffset + ET9_CS_LARGE_NODE_SIZE) <= pTrie->nNodeByteCount)
        ET9_CP_ReadLdbMultiBytes(ET9_CS_LINGUISTIC_INFO(pTrie), dwReadOffset, ET9_CS_LARGE_NODE_SIZE, ppNode);
    else
        ET9_CP_ReadLdbMultiBytes(ET9_CS_LINGUISTIC_INFO(pTrie), dwReadOffset, ET9_CS_SMALL_NODE_SIZE, ppNode);

    eType = ET9_CS_NODE_TYPE( *ppNode );
    *pbNodeSize = (ET9U8)ET9_CS_NODE_SIZE( *ppNode, eType );

    return eType;
}
#else
ET9INLINE static ET9_CS_EnumNodeType ET9LOCALCALL Trie_LoadNode( ET9_CP_SSBITrie* pTrie, ET9S32 iNodeOffset, ET9U8 * pNode, ET9U8 * pbNodeSize )
{
    ET9U32 dwReadOffset;
    ET9_CS_EnumNodeType eType;
    dwReadOffset = pTrie->nTrieNodesOffset + iNodeOffset;
    if ( (ET9U32)(iNodeOffset + ET9_CS_LARGE_NODE_SIZE) <= pTrie->nNodeByteCount)
        ET9_CP_ReadLdbMultiBytes(ET9_CS_LINGUISTIC_INFO(pTrie), dwReadOffset, ET9_CS_LARGE_NODE_SIZE, pNode);
    else
        ET9_CP_ReadLdbMultiBytes(ET9_CS_LINGUISTIC_INFO(pTrie), dwReadOffset, ET9_CS_SMALL_NODE_SIZE, pNode);

    eType = ET9_CS_NODE_TYPE( pNode );
    *pbNodeSize = (ET9U8)ET9_CS_NODE_SIZE( pNode, eType );

    return eType;
}
#endif /* NOT ET9_DIRECT_LDB_ACCESS */

static void ET9LOCALCALL Trie_LoadTailString( ET9_CP_SSBITrie* pTrie, ET9U32 uStrOffs, ET9INT iStrLen, ET9U8* pcBuf )
{
    ET9U32 dwReadOffset;
    ET9INT i;
#ifdef ET9_DIRECT_LDB_ACCESS
    const ET9U8 ET9FARDATA *pcLdbTailString;
    dwReadOffset = pTrie->nTailStringOffset + uStrOffs;
    ET9_CP_ReadLdbMultiBytes(ET9_CS_LINGUISTIC_INFO(pTrie), dwReadOffset, iStrLen, &pcLdbTailString);
    if (pTrie->nTrieType == ET9_CS_BPMF_TRIE)
    {
        /* convert BPMF letters from Trie encoding to Internal spell encoding during copy */
        for ( i = 0; i < iStrLen; i++ ) {
            pcBuf[i] = pTrie->arrEncodingTrieToInternal[pcLdbTailString[i]];
        }
    }
    else {
        /* byte copy */
        for ( i = 0; i < iStrLen; i++ ) {
            pcBuf[i] = pcLdbTailString[i];
        }
    }
#else
    dwReadOffset = pTrie->nTailStringOffset + uStrOffs;
    ET9_CP_ReadLdbMultiBytes(ET9_CS_LINGUISTIC_INFO(pTrie), dwReadOffset, iStrLen, pcBuf);
    if (pTrie->nTrieType == ET9_CS_BPMF_TRIE)
    {
        /* convert Trie encoding to Internal spell encoding during copy */
    for ( i = 0; i < iStrLen; i++ ) {
            pcBuf[i] = pTrie->arrEncodingTrieToInternal[pcBuf[i]];
        }
    }
#endif
}

#if ET9_CS_CONTAIN_UNUSED_FUNCTION
static ET9BOOL ET9LOCALCALL Trie_HasChildren( ET9_CP_SSBITrie* pTrie, ET9U8* pNode, ET9_CS_EnumNodeType eType )
{
    ET9U32 uSibDis;
    ET9S32 iNS;
    if ( eType == eTail )  {
        return 0;
    }
    uSibDis = ET9_CS_SIBLING_DISTANSE( pNode );
    iNS = ET9_CS_NODE_SIZE(pNode, eType);

    /* 0x00000000 :       No Sibling, No Children
       ET9_CS_MAX_SIBLING_DIS :  No Sibling, Has Children */
    if ( uSibDis == 0 || uSibDis == (ET9U32)iNS )  {
        return 0;
    }
    return 1;
}

static ET9BOOL ET9LOCALCALL Trie_HasSibling( ET9_CP_SSBITrie* pTrie, ET9U8* pNode, ET9_CS_EnumNodeType eType )
{
    if ( eType == eTail )  {
        return ( (pNode[0] & 0x40) != 0 );
    }
    else  {
        ET9U32 uSibDis = ET9_CS_SIBLING_DISTANSE( pNode );  /* eChar */
        /* uSibDis:  15 bits
           0x00000000 :       No Sibling, No Children
           ET9_CS_MAX_SIBLING_DIS :  No Sibling, Has Children  */
        if ( uSibDis == 0x00000000 || uSibDis == ET9_CS_MAX_SIBLING_DIS )  {
            return 0;
        }
        return 1;
    }
}
#endif

ET9INLINE static ET9S32 ET9LOCALCALL Trie_SiblingDistance( ET9_CP_SSBITrie* pTrie, const ET9U8 ET9FARDATA *pNode, ET9_CS_EnumNodeType eType )
{
    if ( eType == eChar ) {
        ET9U32 uSibDisIndex = ET9_CS_SIBLING_DIS_INDEX( pNode );
        if ( uSibDisIndex != 0x00000000 && uSibDisIndex != 3 )  /* Trie_HasSibling(iNodeOffset, eChar) */
        {
            /* uSibDisIndex 0: No Child, No Sibling */
            /* uSibDisIndex 1: No Child, Sibling distance 4 */
            /* uSibDisIndex 2: No Child, Sibling distance 6 */
            /* uSibDisIndex 3: HasChild, No Sibling */
            ET9U32 uSibDis, dwReadOffset = pTrie->nSiblingDisArrOffset + uSibDisIndex * sizeof(ET9U32);
            uSibDis = ET9_CP_LdbReadDWord( ET9_CS_LINGUISTIC_INFO(pTrie), dwReadOffset );
            return uSibDis;
        }
    }
    else {/* eTail */
        if ( (pNode[0] & 0x40) != 0 ) {      /* Trie_HasSibling( iNodeOffset, eTail ) */
            return ET9_CS_NODE_SIZE(pNode, eTail);  /* eTail:  NodeSize(iNodeOffset) == ET9_CS_SMALL_NODE_SIZE  --- 4; */
        }
    }
    return 0;  /* No sibling */
}

#ifdef ET9_DEBUG
static ET9S32 ET9LOCALCALL Trie_TailStringOffset( ET9_CP_SSBITrie* pTrie, const ET9U8 ET9FARDATA * pNode )
{
    /* Tail Node only */
    ET9U32 uTailStrOffsIndex = ET9_CS_TAIL_STR_OFFS_INDEX( pNode );
    ET9U32 dwReadOffset = pTrie->nTailStrOffsArrOffset + uTailStrOffsIndex * sizeof(ET9U32);
    ET9U32 uTailStrOffsset = ET9_CP_LdbReadDWord( ET9_CS_LINGUISTIC_INFO(pTrie), dwReadOffset );
    return uTailStrOffsset;
}
#else
#define Trie_TailStringOffset(pTrie, pNode)     (ET9S32)(ET9_CP_LdbReadDWord( ET9_CS_LINGUISTIC_INFO(pTrie), pTrie->nTailStrOffsArrOffset + ET9_CS_TAIL_STR_OFFS_INDEX(pNode) * sizeof(ET9U32) ) )
#endif


#ifdef ET9_DEBUG
static ET9S32 ET9LOCALCALL Trie_IsLastSyllableOfWord( const ET9U8* pNode )
{
    ET9Assert ( ET9_CS_NODE_TYPE( pNode ) == eChar );
    if ( ET9_CS_IS_LAST_SYLLABLE_OF_WORD( pNode ) )
        return 1;
    else
        return 0;
}
#else
#define Trie_IsLastSyllableOfWord(pNode)    (ET9S32)(ET9_CS_IS_LAST_SYLLABLE_OF_WORD( pNode ) )
#endif

static ET9INT ET9LOCALCALL ET9_CS_UpperCaseCount(ET9U8 * pc, ET9S32 iLen)
{
    int i, iCount = 0;
    for ( i = 0; i < iLen; i++ )
    {
        if ( ET9_CP_IsUpperCase(pc[i]) )
            iCount++;
    }
    return iCount;
}


static ET9U32 ET9LOCALCALL FindOffsetOfChildHoldingChar( ET9_CP_SSBITrie* pTrie, ET9S32 iOffset, ET9U8 cExpect)
{
    ET9_CS_EnumNodeType eType = eChar;
    DECLARE_TRIE_NODE(pNode_h);
    ET9U8 bNodeSize;
    ET9S32 iDis = 1;
    while ( iDis )
    {
        eType = TRIE_LOAD_NODE( pTrie, iOffset, pNode_h, &bNodeSize );
        if (eType == eChar) {
            ET9U8 c = (ET9U8)ET9_CS_GET_CHAR( pTrie, pNode_h );
            ET9Assert( c != 0 );
            if ( c == cExpect ) {
                ET9S32 d = ET9_CS_CHILD_DISTANCE(pNode_h, eType);
                return ((d << 24) | iOffset);
            }
        }
        else {  /* eTail */
            ET9U8  acTailStr[ET9_CP_MAX_SPELL_LENGTH];
            ET9S32 iOffs = Trie_TailStringOffset( pTrie, pNode_h );
            ET9S32 iLen  = ET9_CS_TAIL_STRING_LENGTH( pNode_h );
            Trie_LoadTailString( pTrie, iOffs, iLen, (ET9U8*)acTailStr );
            if ( acTailStr[0] == cExpect ) {
                ET9S32 d = ET9_CS_CHILD_DISTANCE(pNode_h, eType);
                return ((d << 24) | iOffset);
            }
        }
        iDis = Trie_SiblingDistance(pTrie, pNode_h, eType);
        iOffset += iDis;
    }
    return 0;
}

#if 1
#define ET9_CP_ShouldApplyGRule(wMohu, pcBuf, len)   ( len >= 2 && SHOULD_TRY_G_RULE(wMohu, pcBuf[len - 2], pcBuf[len - 1]) )
#else
static ET9BOOL ET9LOCALCALL ET9_CP_ShouldApplyGRule(ET9U16 wMohu, ET9U8* pcBuf, ET9S32 len)
{
    if ( len >= 2 && SHOULD_TRY_G_RULE(wMohu, pcBuf[len - 2], pcBuf[len - 1]) )
        return 1;
    if ( len >= 3
        && SHOULD_TRY_G_RULE(wMohu, pcBuf[len - 3], pcBuf[len - 2])
        && (pcBuf[len - 1] == ET9CPSYLLABLEDELIMITER
            || ET9CPSymToCPTone(pcBuf[len - 1]) ) )
        return 1;
    return 0;
}
#endif

static ET9BOOL ET9LOCALCALL ET9_CS_EndOfSyllable(
    ET9_CP_SSBITrie* pTrie,
    const ET9U8 ET9FARDATA *pNode,
    ET9_CS_EnumNodeType eType,
    ET9S32 iNodeOffset,
    ET9U8* pcConsumed,
    ET9S32 iConsumedIdx,
    ET9BOOL fChildNode)
{
    ET9U8 cInTrie = 0;
    ET9U32 iSiblingDistance = ET9_CS_CHILD_DISTANCE(pNode, eType);
    ET9U8 bNodeSize;
    DECLARE_TRIE_NODE(pChildNode);

    if (iSiblingDistance == 0)
    {
        return 1;
    }
    else if (ET9_CS_IsPartialSpellActive(ET9_CS_LINGUISTIC_INFO(pTrie))
             && EndsWithLeadingLetter(pcConsumed, iConsumedIdx) )
    {
        return 1;
    }

    if ( fChildNode)   /* check if the child node is Upper case */
    {
        /* if 1st child is Upper case, this node is EOS */
        eType = TRIE_LOAD_NODE( pTrie, iNodeOffset + iSiblingDistance, pChildNode, &bNodeSize );
        pNode = pChildNode;
    }
    /*  else check if this node is Upper case */

    if (eType == eChar)
    {
        cInTrie = (ET9U8)ET9_CS_GET_CHAR( pTrie, pNode );
    }
    else
    {
        ET9S32 iOffs = Trie_TailStringOffset( pTrie, pNode );
        Trie_LoadTailString( pTrie, iOffs, 1, (ET9U8*)&cInTrie);
    }
    return (ET9BOOL)ET9_CP_IsUpperCase(cInTrie);
}

#define ET9_CS_BIT_PARTIAL_PINYIN_ENABLED 0x0001
#define ET9_CS_BIT_CHECK_CONDITION        0x0002
#define ET9_CS_BIT_BPMF                   0x0004
#define ET9_CS_BIT_PREFIX_SEARCH          0x8000

#define ET9_CS_PARTIAL_ENABLED(wFlags)  (((wFlags) & ET9_CS_BIT_PARTIAL_PINYIN_ENABLED) != 0)
#define ET9_CS_PREFIX_SEARCH(wFlags)    (((wFlags) & ET9_CS_BIT_PREFIX_SEARCH) != 0)
#define ET9_CS_SEGMENT_SEARCH(wFlags)   (((wFlags) & ET9_CS_BIT_PREFIX_SEARCH) == 0)

static void ET9LOCALCALL ET9_CS_AddCandToGroup(ET9_CP_SSBITrie* pTrie, ET9U32 fCheckPrefix, ET9_CS_CandidateGroup* pCandGrp, ET9S32 fPartialPinyin, ET9S32 fEOW, ET9S32 fLSOW, ET9S32 iNodeOffset, ET9S32 iProb, ET9U8* pcSeg, ET9S32 iLen)
{
    if ( fEOW != 0 || fLSOW != 0 )
        pCandGrp->m_eMatchType = eExactMatch;
    else if (pCandGrp->m_eMatchType != eExactMatch)
        pCandGrp->m_eMatchType = ePartialMatch;

    if ( !fCheckPrefix || Trie_BufMatchPrefix(pTrie, pcSeg, iLen) )
    {
        if ( pCandGrp->m_eMatchType == eExactMatch )
        {
            if ( fPartialPinyin )
                iProb  = (iProb / 2); /* 50% penalty */
            else if ( fEOW == 0 && fLSOW != 0 )
                iProb  = (iProb * 3 / 4); /* 25% penalty */
        }
        else {
            iProb  = (iProb / 2); /* 50% penalty */
        }

        {
            ET9CPLingInfo *pLing;
            ET9BOOL bAddCand;
            ET9_CP_SpellMatch eMatchType = (fEOW || fLSOW)? eExactMatch: ePartialMatch;

            pLing = ET9_CS_LINGUISTIC_INFO(pTrie);
            bAddCand = 1;
            if ( ET9_CP_InputHasTone(pLing) )
            {   /* input contains tone, validate tone in phrase module */
                ET9_CP_Spell sToneSpell;
                _ET9ByteCopy(sToneSpell.pbChars, pcSeg, iLen);
                sToneSpell.bLen = (ET9U8)iLen;
#if 1
                eMatchType = ET9_CP_ValidateToneSpell(pLing, &sToneSpell, /* bNeedPartialPhrase */1);
                if ( eNoMatch == eMatchType ) {
                    bAddCand = 0;
                }
#else
                eMatchType = fEOW ? eExactMatch: ePartialMatch;
                if (eExactMatch == eMatchType)
                {
                    bAddCand = ET9_CP_ValidateToneSpell(pLing, &sToneSpell, /* bNeedPartialSyl */0, /* bNeedPartialPhrase */0);
                    if ( bAddCand == 0)
                    {
                        eMatchType = ePartialMatch;
                        bAddCand = ET9_CP_ValidateToneSpell(pLing, &sToneSpell, /* bNeedPartialSyl */0, /* bNeedPartialPhrase */1);
                    }
                }
                else
                {
                   bAddCand = ET9_CP_ValidateToneSpell(pLing, &sToneSpell, /* bNeedPartialSyl */0, /* bNeedPartialPhrase */1);
                }
#endif
            }
            if (bAddCand) {
                SegCandGrpAdd(pTrie, pCandGrp, eMatchType, iNodeOffset, iProb, pcSeg, iLen );
            }
        }
    }
}


static void ET9LOCALCALL StringSymbInfoMatch_Mohu(ET9_CP_SSBITrie* pTrie, ET9U16 wFlags, ET9U16 wMohu,
                                      ET9S32 iNodeOffset, ET9S32 iProb,
                                      ET9U8* pcWholeTailStr, ET9S32 iWholeTailLen, ET9S32 iTailStrStart,
                                      const ET9SymbInfo * pSymbInfo, ET9S32 iInputLen,
                                      ET9U8* pcNodeStack, ET9U8 *pcMatched, ET9S32 iStackIdx,
                                      ET9U8* pcConsumed, ET9S32 iConsumedIdx,
                                      ET9_CS_CandidateGroup* pCandGrp,
                                      ET9U8 cMatching,
                                      ET9S32 fPartialPinyin);

#define GET_PROB(pCandGrp)  ( ((pCandGrp)->m_arrExactMatch.m_bCandidateCount > 0)? (pCandGrp)->m_arrExactMatch.m_pCandidates[0].m_iSegProb: 0 )

static void ET9LOCALCALL StringSymbInfoMatch(ET9_CP_SSBITrie* pTrie, ET9U16 wFlags, ET9U16 wMohu,
                                      ET9S32 iNodeOffset, ET9S32 iProb,
                                      ET9U8* pcWholeTailStr, ET9S32 iWholeTailLen, ET9S32 iTailStrStart,
                                      const ET9SymbInfo * pSymbInfo, ET9S32 iInputLen,
                                      ET9U8* pcNodeStack, ET9U8 *pcMatched, ET9S32 iStackIdx,
                                      ET9U8* pcConsumed, ET9S32 iConsumedIdx,
                                      ET9_CS_CandidateGroup* pCandGrp,
                                      ET9S32 fPartialPinyin)
{
    ET9U8 cMatching;
    ET9S32 fEOW, fLSOW;
    ET9U8* pcTailStr = pcWholeTailStr + iTailStrStart;
    ET9S32 iTailLen = iWholeTailLen - iTailStrStart;

    if ( iInputLen == 0 ) {
        if ( ET9_CS_SEGMENT_SEARCH(wFlags) ) {
            fEOW = ET9_CS_PARTIAL_ENABLED(wFlags)?  (ET9_CS_UpperCaseCount((ET9U8*)pcTailStr, iTailLen) <= 1): (iTailLen == 0);
            fLSOW = ( ET9_CS_UpperCaseCount((ET9U8*)pcTailStr, iTailLen) == 0 );
            ET9_CS_AddCandToGroup( pTrie, (wFlags & ET9_CS_BIT_CHECK_CONDITION)? 1: 0, pCandGrp, fPartialPinyin, fEOW, fLSOW, iNodeOffset, iProb, pcConsumed, iConsumedIdx );
        }
        return;
    }

    if (iTailLen <= 0) {
        if ( iInputLen == 1 && ET9_CS_SEGMENT_SEARCH(wFlags) ) {
            if ( ET9_CP_SymbIsToneOrDelim(pSymbInfo) ) {
                pcConsumed[iConsumedIdx++] = ET9_CP_GetSymbToneOrDelim(pSymbInfo);
                pcConsumed[iConsumedIdx] = 0;   /* 0-termination */
                fEOW = 1;
                fLSOW = 1;
                ET9_CS_AddCandToGroup( pTrie, (wFlags & ET9_CS_BIT_CHECK_CONDITION)? 1: 0, pCandGrp, fPartialPinyin, fEOW, fLSOW, iNodeOffset, iProb, pcConsumed, iConsumedIdx );
            }
        }
        return;
    }
    if ( ET9_CS_SEGMENT_SEARCH(wFlags) && iTailLen * 2 < iInputLen )  {
        return;
    }

    if ( pCandGrp && iProb <= GET_PROB(pCandGrp) )
        return;

    if ( ET9_CP_SymbIsToneOrDelim(pSymbInfo) && (ET9_CP_IsUpperCase(pcTailStr[0]) || ET9_CS_PARTIAL_ENABLED(wFlags) && EndsWithLeadingLetter(pcConsumed, iConsumedIdx)) )
    {
        pcConsumed[iConsumedIdx++] = ET9_CP_GetSymbToneOrDelim(pSymbInfo);
        pcConsumed[iConsumedIdx] = 0;   /* 0-termination */
        StringSymbInfoMatch(pTrie, wFlags, wMohu, iNodeOffset, iProb, pcWholeTailStr, iWholeTailLen, iTailStrStart, pSymbInfo + 1, iInputLen - 1, pcNodeStack, pcMatched, iStackIdx, pcConsumed, iConsumedIdx, pCandGrp, fPartialPinyin);
        return;
    }

#define PREV_COMSUMED_IS_UPPER(pcConsumed, iConsumedIdx) (ET9_CP_IsUpperCase((pcConsumed)[(iConsumedIdx) - 1]) || (pcConsumed)[(iConsumedIdx) - 1] == 'h')

    if ( ET9_CS_PARTIAL_ENABLED(wFlags)
        && !ET9_CP_IsUpperCase(*pcTailStr)
        && ( PREV_COMSUMED_IS_UPPER(pcConsumed, iConsumedIdx) || (ET9CPSYLLABLEDELIMITER == pcConsumed[iConsumedIdx - 1] && PREV_COMSUMED_IS_UPPER(pcConsumed, iConsumedIdx - 1) ) )/* iConsumedIdx > 0 since *pcTailStr is lower case */
        && !(iStackIdx >= 2 && SHOULD_TRY_H_RULE(wMohu, pcNodeStack[iStackIdx - 2]) && pcNodeStack[iStackIdx - 1] == 'h') )
    { /* when doing partial spelling, skip to the beginning of the next syllable */
        ET9S32 i, nSkipped = (ET9S32)ET9_CP_FindNextUpperCase((const ET9U8 *)pcTailStr, iTailLen);
        if ( nSkipped > 0 && nSkipped < iTailLen /* && EndsWithLeadingLetter(pcNodeStack, iStackIdx) */ )
        {
            for ( i = 0; i < nSkipped; i++ )
            {
                pcNodeStack[iStackIdx + i] = pcTailStr[i];
                pcMatched[iStackIdx + i] = 0;
            }
            StringSymbInfoMatch(pTrie, wFlags, wMohu, iNodeOffset, iProb, pcWholeTailStr, iWholeTailLen, iTailStrStart + nSkipped, pSymbInfo, iInputLen, pcNodeStack, pcMatched, iStackIdx + nSkipped, pcConsumed, iConsumedIdx, pCandGrp, /*fPartialPinyin*/1);
        }
    }
#undef PREV_COMSUMED_IS_UPPER

    cMatching = ET9_CP_CharSymbMohuMatch(wMohu, *pcTailStr, pSymbInfo);
    if (cMatching)
    {
        if ( iStackIdx > 0 && pcMatched[iStackIdx - 1] == 0 && !ET9_CP_IsUpperCase(pcNodeStack[iStackIdx - 1]) && !ET9_CP_IsUpperCase(cMatching))
            cMatching = 0;
        else if ( !ET9_CP_IsUpperCase(cMatching)
                  && (pcConsumed[iConsumedIdx - 1] == ET9CPSYLLABLEDELIMITER
                      || ET9CPSymToCPTone(pcConsumed[iConsumedIdx - 1]) ) )  /* iConsumedIdx > 0 since cMatching is lower case */
            cMatching = 0;
        else if ( (cMatching == 'h' || cMatching == 'g') && iConsumedIdx > 0 && pcConsumed[iConsumedIdx - 1] == cMatching )
            cMatching = 0;
    }

    pcNodeStack[iStackIdx] = *pcTailStr;
    if (cMatching)
    {
        pcMatched[iStackIdx] = 1;
        pcConsumed[iConsumedIdx++] = cMatching;
        if ( ET9_CS_PREFIX_SEARCH(wFlags) )
        {
            if ( iTailLen == 1 )
            {
                PrefixBufferAddPrefix((wFlags & ET9_CS_BIT_BPMF)? 1: 0, pTrie, (ET9U8*)pcConsumed, (ET9U8)iConsumedIdx, iProb );
                if ( iInputLen >= 2 && ET9_CP_ShouldApplyGRule(wMohu, pcNodeStack, iStackIdx + 1) && ET9_CS_IsCharInSymbInfo('g', pSymbInfo + 1) )
                {
                    pcConsumed[iConsumedIdx] = 'g';
                    PrefixBufferAddPrefix((wFlags & ET9_CS_BIT_BPMF)? 1: 0, pTrie, (ET9U8*)pcConsumed, (ET9U8)(iConsumedIdx + 1), iProb );
                }
            }
            else if ( iTailLen == 2 && pcTailStr[1] == 'g' && SHOULD_TRY_G_RULE(wMohu, pcTailStr[-1], pcTailStr[0]) )
                PrefixBufferAddPrefix((wFlags & ET9_CS_BIT_BPMF)? 1: 0, pTrie, (ET9U8*)pcConsumed, (ET9U8)iConsumedIdx, iProb );
            else if ( ET9_CS_PARTIAL_ENABLED(wFlags) && ET9_CS_UpperCaseCount((ET9U8*)pcTailStr + 1, iTailLen - 1) == 0 && EndsWithLeadingLetter(pcConsumed, iConsumedIdx) )
                PrefixBufferAddPrefix((wFlags & ET9_CS_BIT_BPMF)? 1: 0, pTrie, (ET9U8*)pcConsumed, (ET9U8)iConsumedIdx, iProb );
        }
    }
    else
    {
        pcMatched[iStackIdx] = 0;
        if (!EndsWithLeadingLetter(pcConsumed, iConsumedIdx) && !ET9_CP_IsUpperCase(cMatching))
            return;
    }

    iStackIdx++;
    if ( ET9_CS_SEGMENT_SEARCH(wFlags)
          && ( iInputLen == 1 ||
               ( iInputLen == 2 && ET9_CP_SymbIsToneOrDelim(pSymbInfo + 1)
              && ( iTailLen > 1 && ET9_CP_IsUpperCase(pcTailStr[1]) || ET9_CS_PARTIAL_ENABLED(wFlags) && EndsWithLeadingLetter(pcConsumed, iConsumedIdx) ) ) )
          && cMatching
          && ( ET9_CP_IsUpperCase(cMatching) || iStackIdx == 0 || pcMatched[iStackIdx - 1] != 0 ) )
    {
        if (iInputLen == 1)
            pcConsumed[iConsumedIdx] = 0;   /* 0-termination */
        else
        {
            pcConsumed[iConsumedIdx++] = ET9_CP_GetSymbToneOrDelim(pSymbInfo + 1);
            pcConsumed[iConsumedIdx] = 0;   /* 0-termination */
        }
        pcNodeStack[iStackIdx] = 0;    /* 0-termination */
        fEOW = 0;
        fLSOW = 0;
        if ( iTailLen == 1 )
        {
            fEOW = 1;
            fLSOW = 1;
        }
        else
        {
            if (ET9_CS_PARTIAL_ENABLED(wFlags) && ET9_CS_UpperCaseCount((ET9U8*)pcTailStr + 1, iTailLen - 1) == 0 && EndsWithLeadingLetter(pcConsumed, iConsumedIdx) )
                fLSOW = 1;
        }
        ET9_CS_AddCandToGroup( pTrie, (wFlags & ET9_CS_BIT_CHECK_CONDITION)? 1: 0, pCandGrp, fPartialPinyin, fEOW, fLSOW, iNodeOffset, iProb, pcConsumed, iConsumedIdx );
    }

    if ( cMatching != 0 || !ET9_CP_IsUpperCase(*pcTailStr) )
    {
        ET9INT nConsume;
        nConsume = ((cMatching && (iStackIdx == 0 || pcMatched[iStackIdx - 1] != 0))? 1: 0);
        if ( iInputLen > nConsume )
        {
            if ( ET9_CP_IsUpperCase(cMatching) || iStackIdx == 0 || ET9_CP_IsUpperCase(pcNodeStack[iStackIdx - 1]) || pcMatched[iStackIdx - 1] != 0 )
                StringSymbInfoMatch(pTrie, wFlags, wMohu, iNodeOffset, iProb, pcWholeTailStr, iWholeTailLen, iTailStrStart + 1, pSymbInfo + nConsume, iInputLen - nConsume, pcNodeStack, pcMatched, iStackIdx, pcConsumed, iConsumedIdx, pCandGrp, fPartialPinyin);
        }
    }
    if (wMohu)
        StringSymbInfoMatch_Mohu(pTrie, wFlags, wMohu, iNodeOffset, iProb, pcWholeTailStr, iWholeTailLen, iTailStrStart, pSymbInfo, iInputLen, pcNodeStack, pcMatched, iStackIdx, pcConsumed, iConsumedIdx, pCandGrp, cMatching, fPartialPinyin);
}

static void ET9LOCALCALL StringSymbInfoMatch_Mohu(ET9_CP_SSBITrie* pTrie, ET9U16 wFlags, ET9U16 wMohu,
                                      ET9S32 iNodeOffset, ET9S32 iProb,
                                      ET9U8* pcWholeTailStr, ET9S32 iWholeTailLen, ET9S32 iTailStrStart,
                                      const ET9SymbInfo * pSymbInfo, ET9S32 iInputLen,
                                      ET9U8* pcNodeStack, ET9U8 *pcMatched, ET9S32 iStackIdx,
                                      ET9U8* pcConsumed, ET9S32 iConsumedIdx,
                                      ET9_CS_CandidateGroup* pCandGrp,
                                      ET9U8 cMatching,
                                      ET9S32 fPartialPinyin)
{
    ET9S32 fEOW, fLSOW;
    ET9U8* pcTailStr = pcWholeTailStr + iTailStrStart;
    ET9S32 iTailLen = iWholeTailLen - iTailStrStart;


    /* G_Rule */
    if ( cMatching == 'n' && SHOULD_TRY_G_RULE(wMohu, pcTailStr[-1], pcTailStr[0]) )
    {
        if ( iTailLen >= 2 && pcTailStr[1] == 'g' )  /* pcTailStr is "ng......" */
        {   /* Ignore 'g' in pcTailStr */
            if ( iInputLen == 1 && ET9_CS_SEGMENT_SEARCH(wFlags) )  /* input is "n" */
            {
                fEOW = ( iTailLen == 2 )? 1: 0;  /* pcTailStr is "ng" then fEOW is 1, else fEOW is 0 */
                fLSOW = (ET9_CS_PARTIAL_ENABLED(wFlags) && ET9_CS_UpperCaseCount((ET9U8*)pcTailStr, iTailLen) == 0 )? 1: 0;
                ET9_CS_AddCandToGroup( pTrie, (wFlags & ET9_CS_BIT_CHECK_CONDITION)? 1: 0, pCandGrp, fPartialPinyin, fEOW, fLSOW, iNodeOffset, iProb, pcConsumed, iConsumedIdx );
            }
            else if (iTailLen > 2) /*  iInputLen > 1 && iTailLen > 2 */
            {
                pcNodeStack[iStackIdx] = 'g';
                pcMatched[iStackIdx] = pcMatched[iStackIdx - 1];
                StringSymbInfoMatch(pTrie, wFlags, wMohu, iNodeOffset, iProb, pcWholeTailStr, iWholeTailLen, iTailStrStart + 2, pSymbInfo + 1, iInputLen - 1, pcNodeStack, pcMatched, iStackIdx + 1, pcConsumed, iConsumedIdx, pCandGrp, fPartialPinyin);
            }
            /* pcTailStr is "ng" and input is "n...", do nothing */
        }
        if ( ET9_CS_IsCharInSymbInfo('g', pSymbInfo + 1) )
        {   /* Ignore 'g' in pSymbInfo */
            pcConsumed[iConsumedIdx] = 'g';
            if ( iInputLen == 2 && ET9_CS_SEGMENT_SEARCH(wFlags) )  /* input is "ng" */
            {
                fEOW = ( iTailLen == 1 )? 1: 0;
                fLSOW = (ET9_CS_PARTIAL_ENABLED(wFlags) && ET9_CS_UpperCaseCount((ET9U8*)pcTailStr, iTailLen) == 0 )? 1: 0;
                ET9_CS_AddCandToGroup( pTrie, (wFlags & ET9_CS_BIT_CHECK_CONDITION)? 1: 0, pCandGrp, fPartialPinyin, fEOW, fLSOW, iNodeOffset, iProb, pcConsumed, iConsumedIdx + 1 );
            }
            else
                StringSymbInfoMatch(pTrie, wFlags, wMohu, iNodeOffset, iProb, pcWholeTailStr, iWholeTailLen, iTailStrStart + 1, pSymbInfo + 2, iInputLen - 2, pcNodeStack, pcMatched, iStackIdx, pcConsumed, iConsumedIdx + 1, pCandGrp, fPartialPinyin);
        }
    }

    /* H_Rule */
    if ( cMatching != 0 && SHOULD_TRY_H_RULE(wMohu, pcTailStr[0]) )
    {
        if ( iTailLen >= 2 && pcTailStr[1] == 'h' )  /* pcTailStr is "Ch..., Sh..., Zh..." */
        {
            pcNodeStack[iStackIdx] = 'h';
            pcMatched[iStackIdx] = pcMatched[iStackIdx - 1];
            if ( iInputLen == 1 && ET9_CS_SEGMENT_SEARCH(wFlags) )  /* input is C|S|Z */
            {
                fEOW = 0;
                fLSOW = (ET9_CS_PARTIAL_ENABLED(wFlags) && ET9_CS_UpperCaseCount((ET9U8*)pcTailStr, iTailLen) == 0 )? 1: 0;
                ET9Assert( iTailLen > 2 );    /* pcTailStr is not "Ch|Sh|Zh" */
                ET9_CS_AddCandToGroup( pTrie, (wFlags & ET9_CS_BIT_CHECK_CONDITION)? 1: 0, pCandGrp, fPartialPinyin, fEOW, fLSOW, iNodeOffset, iProb, pcConsumed, iConsumedIdx );
            }
            else if (iTailLen > 2) /*  iInputLen > 1 && iTailLen > 2 */
                StringSymbInfoMatch(pTrie, wFlags, wMohu, iNodeOffset, iProb, pcWholeTailStr, iWholeTailLen, iTailStrStart + 2, pSymbInfo + 1, iInputLen - 1, pcNodeStack, pcMatched, iStackIdx + 1, pcConsumed, iConsumedIdx, pCandGrp, fPartialPinyin);
            /* pcTailStr is "ng" and input is "n...", do nothing */
        }
        if ( ET9_CS_IsCharInSymbInfo('h', pSymbInfo + 1) )
        {
            pcConsumed[iConsumedIdx] = 'h';
            if ( iInputLen == 2 && ET9_CS_SEGMENT_SEARCH(wFlags) )  /* input is "Ch,Sh, Zh" */
            {
                fEOW = ( iTailLen == 1 )? 1: 0;
                fLSOW = (ET9_CS_PARTIAL_ENABLED(wFlags) && ET9_CS_UpperCaseCount((ET9U8*)pcTailStr, iTailLen) == 0 )? 1: 0;
                ET9_CS_AddCandToGroup( pTrie, (wFlags & ET9_CS_BIT_CHECK_CONDITION)? 1: 0, pCandGrp, fPartialPinyin, fEOW, fLSOW, iNodeOffset, iProb, pcConsumed, iConsumedIdx + 1 );
            }
            else if ( iInputLen == 3 && ET9_CS_SEGMENT_SEARCH(wFlags) && ET9_CP_SymbIsToneOrDelim(pSymbInfo + 2) )
            {
                if ( ET9_CS_PARTIAL_ENABLED(wFlags) )
                {
                    pcConsumed[iConsumedIdx + 1] = ET9_CP_GetSymbToneOrDelim(pSymbInfo);;
                    fEOW = ( iTailLen == 1 )? 1: 0;
                    fLSOW = ET9_CS_UpperCaseCount((ET9U8*)pcTailStr, iTailLen) == 0? 1: 0;
                    ET9_CS_AddCandToGroup( pTrie, (wFlags & ET9_CS_BIT_CHECK_CONDITION)? 1: 0, pCandGrp, fPartialPinyin, fEOW, fLSOW, iNodeOffset, iProb, pcConsumed, iConsumedIdx + 2);
                }
                return;
            }
            else
                StringSymbInfoMatch(pTrie, wFlags, wMohu, iNodeOffset, iProb, pcWholeTailStr, iWholeTailLen, iTailStrStart + 1, pSymbInfo + 2, iInputLen - 2, pcNodeStack, pcMatched, iStackIdx, pcConsumed, iConsumedIdx + 1, pCandGrp, fPartialPinyin);
        }
    }
}

static void ET9LOCALCALL ET9_CS_TrieSearch(ET9_CP_SSBITrie* pTrie, ET9U16 wFlags, ET9U16 wMohu,
                                   const ET9U8 ET9FARDATA *pNode,
                                   ET9S32 iNodeOffset,
                                   const ET9SymbInfo * pSymbInfo, ET9S32 iInputLen,
                                   ET9U8* pcNodeStack, ET9U8 *pcMatched, ET9S32 iStackIdx,
                                   ET9U8* pcConsumed, ET9S32 iConsumedIdx,
                                   ET9_CS_CandidateGroup* pCandGrp,
                                   ET9S32 fLowercaseNodeIgnored,
                                   ET9S32 fPartialPinyin)
{
    ET9_CS_EnumNodeType eType;
    ET9S32 fEOW = 0, fLSOW = 0, iProb, iSiblingDistance, iChildDistance;
    ET9U8 cInTrie = 0, cMatching = 0;
    ET9INT fNextDelimTone;

    eType = ET9_CS_NODE_TYPE( pNode );
    if (iInputLen <= 0) {
        return;
    }

    if ( pCandGrp && (ET9S32)ET9_CS_MAX_DES_PROB( pNode, eType ) <= GET_PROB(pCandGrp) )
        return;

    if ( ET9_CP_SymbIsToneOrDelim(pSymbInfo) && ET9_CS_EndOfSyllable(pTrie, pNode, eType, iNodeOffset, pcConsumed, iConsumedIdx, /*fChildNode*/0 ) )
    {
        pcConsumed[iConsumedIdx++] = ET9_CP_GetSymbToneOrDelim(pSymbInfo);
        pcConsumed[iConsumedIdx] = 0;   /* 0-termination */
        if ( iInputLen == 1 && ET9_CS_SEGMENT_SEARCH(wFlags) )
        {
            if (eType == eTail )
            {
                ET9U8 acTailStr[ET9_CP_MAX_SPELL_LENGTH];
                ET9S32 iOffs = Trie_TailStringOffset( pTrie, pNode );
                ET9S32 iTailLen  = ET9_CS_TAIL_STRING_LENGTH( pNode );
                Trie_LoadTailString( pTrie, iOffs, iTailLen, acTailStr);
                acTailStr[iTailLen] = 0;   /* 0-termination */

                iProb = ET9_CS_WORD_PROB( pNode, eType );
                fEOW = 0;
                if ( ET9_CS_PARTIAL_ENABLED(wFlags) && !(fEOW && !fPartialPinyin) ) {
                    fLSOW = ET9_CS_UpperCaseCount(acTailStr, iTailLen);
                }
            }
            else
            {
                fEOW = ET9_CS_END_OF_WORD( pNode );  /* eChar */
                iProb = fEOW? ET9_CS_WORD_PROB( pNode, eType ): ET9_CS_MAX_DES_PROB( pNode, eType );
                if ( ET9_CS_PARTIAL_ENABLED(wFlags) && !(fEOW && !fPartialPinyin) )
                    fLSOW = Trie_IsLastSyllableOfWord(pNode);
                else
                    fLSOW = 0;
            }
            ET9_CS_AddCandToGroup(pTrie, (wFlags & ET9_CS_BIT_CHECK_CONDITION)? 1: 0, pCandGrp, fPartialPinyin, fEOW, fLSOW, iNodeOffset, iProb, pcConsumed, iConsumedIdx);
            return;
        }
        else
        {
            ET9_CS_TrieSearch(pTrie, wFlags, wMohu, pNode, iNodeOffset, pSymbInfo + 1, iInputLen - 1, pcNodeStack, pcMatched, iStackIdx, pcConsumed, iConsumedIdx, pCandGrp, /*fLowercaseNodeIgnored*/0, fPartialPinyin);
        }
        return;
    }

    if (eType == eTail)
    {
        ET9U8 acTailStr[ET9_CP_MAX_SPELL_LENGTH];
        ET9S32 iOffs = Trie_TailStringOffset( pTrie, pNode );
        ET9S32 iTailLen  = ET9_CS_TAIL_STRING_LENGTH( pNode );
        ET9U8 * pcTailStr = (ET9U8 *)(acTailStr + 2);

        acTailStr[0] = acTailStr[1] = 0;
        if (iStackIdx >= 1)
            acTailStr[1] = pcNodeStack[iStackIdx - 1];
        if (iStackIdx >= 2)
            acTailStr[0] = pcNodeStack[iStackIdx - 2];

        Trie_LoadTailString( pTrie, iOffs, iTailLen, pcTailStr);
        pcTailStr[iTailLen] = 0;   /* 0-termination */
        iProb = ET9_CS_WORD_PROB( pNode, eType );
        StringSymbInfoMatch(pTrie, wFlags, wMohu, iNodeOffset, iProb, pcTailStr, iTailLen, 0, pSymbInfo, iInputLen, pcNodeStack, pcMatched, iStackIdx, pcConsumed, iConsumedIdx, pCandGrp, fPartialPinyin);
        if ( pcTailStr[0] == 'h' && SHOULD_TRY_H_RULE(wMohu, pcTailStr[-1]) && pcConsumed[iConsumedIdx-1] != 'h' )
        {   /* Ignore 'h' in pcTailStr */
            pcMatched[iStackIdx] = pcMatched[iStackIdx - 1];
            pcNodeStack[iStackIdx] = 'h';
            StringSymbInfoMatch(pTrie, wFlags, wMohu, iNodeOffset, iProb, pcTailStr, iTailLen, 1, pSymbInfo, iInputLen, pcNodeStack, pcMatched, iStackIdx + 1, pcConsumed, iConsumedIdx, pCandGrp, fPartialPinyin);
        }
        if ( pcTailStr[0] == 'g' && SHOULD_TRY_G_RULE(wMohu, pcTailStr[-2], pcTailStr[-1]) && pcConsumed[iConsumedIdx-1] != 'g')
        {   /* Ignore 'g' in pcTailStr */
            pcMatched[iStackIdx] = pcMatched[iStackIdx - 1];
            pcNodeStack[iStackIdx] = 'g';
            StringSymbInfoMatch(pTrie, wFlags, wMohu, iNodeOffset, iProb, pcTailStr, iTailLen, 1, pSymbInfo, iInputLen, pcNodeStack, pcMatched, iStackIdx + 1, pcConsumed, iConsumedIdx, pCandGrp, fPartialPinyin);
        }
        return;
    }

    /* Now  eType == eChar */
    cInTrie = (ET9U8)ET9_CS_GET_CHAR( pTrie, pNode );
    cMatching = ET9_CP_CharSymbMohuMatch(wMohu, cInTrie, pSymbInfo);
    if (cMatching)
    {
        if ( iStackIdx > 0 && pcMatched[iStackIdx - 1] == 0 && !ET9_CP_IsUpperCase(pcNodeStack[iStackIdx - 1]) && !ET9_CP_IsUpperCase(cMatching) )
            cMatching = 0;
        else if ( iConsumedIdx > 0
                  && !ET9_CP_IsUpperCase(cMatching)
                  && (pcConsumed[iConsumedIdx - 1] == ET9CPSYLLABLEDELIMITER
                      || ET9CPSymToCPTone(pcConsumed[iConsumedIdx - 1]) ) )
            cMatching = 0;
        else if ( (cMatching == 'h' || cMatching == 'g') && iConsumedIdx > 0 && pcConsumed[iConsumedIdx - 1] == cMatching )
            cMatching = 0;
        else if ( ET9_CP_IsLowerCase(cMatching) && iStackIdx > 0 &&
                  (pcNodeStack[iStackIdx - 1] == cMatching ||    /* 12 key SLBS for ShengLaoBingSi */
                   fLowercaseNodeIgnored && ET9_CP_IsLowerCase(pcNodeStack[iStackIdx - 1])) )   /* 12 key "964" don't generate "Yng" */
        {
            cMatching = 0;
        }
    }
    pcNodeStack[iStackIdx] = cInTrie;
    if (cMatching)
    {
        pcMatched[iStackIdx++] = 1;
        pcConsumed[iConsumedIdx++] = cMatching;
    }
    else
    {
        pcMatched[iStackIdx++] = 0;
        if (!EndsWithLeadingLetter(pcConsumed, iConsumedIdx))
            return;
    }
    fEOW = ET9_CS_END_OF_WORD( pNode );  /* eChar */
    if ( ET9_CS_PARTIAL_ENABLED(wFlags) && !(fEOW && !fPartialPinyin) )
        fLSOW = Trie_IsLastSyllableOfWord(pNode);
    else
        fLSOW = 0;
    iProb = fEOW? ET9_CS_WORD_PROB( pNode, eType ): ET9_CS_MAX_DES_PROB( pNode, eType );
    if ( ET9_CS_PREFIX_SEARCH(wFlags) && cMatching )
    {
        if ( fEOW || fLSOW )
        {
            PrefixBufferAddPrefix((wFlags & ET9_CS_BIT_BPMF)? 1: 0, pTrie, (ET9U8*)pcConsumed, (ET9U8)iConsumedIdx, iProb );
            if ( iInputLen >= 2 && ET9_CP_ShouldApplyGRule(wMohu, pcNodeStack, iStackIdx) && ET9_CS_IsCharInSymbInfo('g', pSymbInfo + 1) )
            {
                pcConsumed[iConsumedIdx] = 'g';
                PrefixBufferAddPrefix((wFlags & ET9_CS_BIT_BPMF)? 1: 0, pTrie, (ET9U8*)pcConsumed, (ET9U8)(iConsumedIdx + 1), iProb );
            }
            else if ( iInputLen >= 2 && SHOULD_TRY_H_RULE(wMohu, pcNodeStack[iStackIdx - 1]) && ET9_CS_IsCharInSymbInfo('h', pSymbInfo + 1) )
            {
                pcConsumed[iConsumedIdx] = 'h';
                PrefixBufferAddPrefix((wFlags & ET9_CS_BIT_BPMF)? 1: 0, pTrie, (ET9U8*)pcConsumed, (ET9U8)(iConsumedIdx + 1), iProb );
            }
        }
        else if (cMatching == 'n' && ET9_CP_ShouldApplyGRule(wMohu, pcNodeStack, iStackIdx) )
        {
            ET9S32 fChildEOW = 0, fChildLSOW = 0, iChildProb;
            ET9U32 iOffsetEtc;
            iChildDistance = ET9_CS_CHILD_DISTANCE(pNode, eType);
            iOffsetEtc = FindOffsetOfChildHoldingChar(pTrie, iNodeOffset + iChildDistance, 'g');
            if ( iOffsetEtc != 0 )
            {
                ET9_CS_EnumNodeType eChildType;
                ET9U8 bNodeSize;
                DECLARE_TRIE_NODE(pChildNode);
                eChildType = TRIE_LOAD_NODE( pTrie, (iOffsetEtc & 0xFFFFFF), pChildNode, &bNodeSize );
                pcNodeStack[iStackIdx] = 'g';
                pcMatched[iStackIdx] = 1;
                if ( eChildType == eTail )
                    fChildEOW = 0;    /* First letter in the TailNode, not EOW */
                else
                    fChildEOW = ET9_CS_END_OF_WORD( pChildNode );    /* eChar */
                iChildProb = fChildEOW? ET9_CS_WORD_PROB( pChildNode, eChildType ): ET9_CS_MAX_DES_PROB( pChildNode, eChildType );
                if ( ET9_CS_PARTIAL_ENABLED(wFlags) && !(fChildEOW & !fPartialPinyin) && eChildType == eChar )
                    fChildLSOW = Trie_IsLastSyllableOfWord(pChildNode);
                else
                    fChildLSOW = 0;
            }
            if ( fChildEOW || fChildLSOW )
                PrefixBufferAddPrefix((wFlags & ET9_CS_BIT_BPMF)? 1: 0, pTrie, (ET9U8*)pcConsumed, (ET9U8)iConsumedIdx, iProb );
        }
    }

    fNextDelimTone = 0;
    if ( ET9_CS_SEGMENT_SEARCH(wFlags) )
    {
        if (iInputLen > 1) {
            fNextDelimTone = ET9_CP_SymbIsToneOrDelim(pSymbInfo + 1);
        }

        if ( (iInputLen == 1 || (iInputLen == 2 && fNextDelimTone) ) &&
              cMatching && (ET9_CP_IsUpperCase(cMatching) || iStackIdx == 0 || pcMatched[iStackIdx - 1] != 0) )
        {
            ET9BOOL bIgnoreG = (ET9BOOL)( cMatching == 'n' && ET9_CP_ShouldApplyGRule(wMohu, pcNodeStack, iStackIdx) );
            if (iInputLen == 1)
            {
                pcConsumed[iConsumedIdx] = 0;   /* 0-termination */
                pcNodeStack[iStackIdx] = 0;   /* 0-termination */
                ET9_CS_AddCandToGroup(pTrie, (wFlags & ET9_CS_BIT_CHECK_CONDITION)? 1: 0, pCandGrp, fPartialPinyin, fEOW, fLSOW, iNodeOffset, iProb, pcConsumed, iConsumedIdx);
            }
            else /* iInputLen == 2 && fNextDelimTone */
            {
                pcConsumed[iConsumedIdx++] = ET9_CP_GetSymbToneOrDelim(pSymbInfo + 1);
                pcConsumed[iConsumedIdx] = 0;   /* 0-termination */
                if ( fEOW || ET9_CS_EndOfSyllable(pTrie, pNode, eType, iNodeOffset, pcConsumed, iConsumedIdx, /*fChildNode*/1) )
                {
                    pcNodeStack[iStackIdx] = 0;   /* 0-termination */
                    ET9_CS_AddCandToGroup(pTrie, (wFlags & ET9_CS_BIT_CHECK_CONDITION)? 1: 0, pCandGrp, fPartialPinyin, fEOW, fLSOW, iNodeOffset, iProb, pcConsumed, iConsumedIdx);
                }
                else if ( !bIgnoreG )
                {
                    return;
                }
            }
            if (bIgnoreG)
            {
                ET9U32 iOffsetEtc;
                ET9S32 fChildEOW = 0, fChildLSOW = 0, iChildProb;
                iChildDistance = ET9_CS_CHILD_DISTANCE(pNode, eType);
                iOffsetEtc = FindOffsetOfChildHoldingChar(pTrie, iNodeOffset + iChildDistance, 'g');
                if ( iOffsetEtc != 0 )
                {
                    ET9_CS_EnumNodeType eChildType;
                    ET9U8 bNodeSize;
                    DECLARE_TRIE_NODE(pChildNode);
                    eChildType = TRIE_LOAD_NODE( pTrie, (iOffsetEtc & 0xFFFFFF), pChildNode, &bNodeSize );
                    pcNodeStack[iStackIdx] = 'g';
                    pcMatched[iStackIdx] = 1;
                    iStackIdx++;
                    if ( eChildType == eTail )
                        fChildEOW = 0;    /* First letter in the TailNode, not EOW */
                    else
                        fChildEOW = ET9_CS_END_OF_WORD( pChildNode );   /* eChar */
                    iChildProb = fChildEOW? ET9_CS_WORD_PROB( pChildNode, eChildType ): ET9_CS_MAX_DES_PROB( pChildNode, eChildType );
                    if ( ET9_CS_PARTIAL_ENABLED(wFlags) && !(fChildEOW & !fPartialPinyin) && eChildType == eChar )
                        fChildLSOW = Trie_IsLastSyllableOfWord(pChildNode);
                    else
                        fChildLSOW = 0;
                    ET9_CS_AddCandToGroup(pTrie, (wFlags & ET9_CS_BIT_CHECK_CONDITION)? 1: 0, pCandGrp, fPartialPinyin, fChildEOW, fChildLSOW, iNodeOffset, iChildProb, pcConsumed, iConsumedIdx);
                }
            }
            return;
        }
        if (iInputLen == 2 && cMatching == 'n' && ET9_CP_ShouldApplyGRule(wMohu, pcNodeStack, iStackIdx) && ET9_CS_IsCharInSymbInfo('g', pSymbInfo + 1))
        {
            pcConsumed[iConsumedIdx] = 'g';
            ET9_CS_AddCandToGroup(pTrie, (wFlags & ET9_CS_BIT_CHECK_CONDITION)? 1: 0, pCandGrp, fPartialPinyin, fEOW, fLSOW, iNodeOffset, iProb, pcConsumed, iConsumedIdx + 1);
        }
    }   /* ENDOF if ( ET9_CS_SEGMENT_SEARCH(wFlags) ) */


    iChildDistance = ET9_CS_CHILD_DISTANCE(pNode, eType);
    if (iChildDistance == 0)
        return;

    if ( cMatching != 0 || !ET9_CP_IsUpperCase(cInTrie) )
    {
        DECLARE_TRIE_NODE(pChildNode);
        ET9U8 bNodeSize;
        ET9_CS_EnumNodeType eChildType;
        ET9S32 iChildOffset = iNodeOffset;
        ET9S32 iIgnoreSymb = 0;

        if ( fNextDelimTone && cMatching && (ET9_CP_IsUpperCase(cMatching) || iStackIdx == 0 || pcMatched[iStackIdx - 1] != 0) )
        {
            pcConsumed[iConsumedIdx++] = ET9_CP_GetSymbToneOrDelim(pSymbInfo + 1);
            pcConsumed[iConsumedIdx] = 0;   /* 0-termination */
            iIgnoreSymb = 1;
        }
        iSiblingDistance = iChildDistance;
        while (iSiblingDistance != 0)
        {
            iChildOffset += iSiblingDistance;
            eChildType = TRIE_LOAD_NODE( pTrie, iChildOffset, pChildNode, &bNodeSize );
            if ( cMatching != 0 && iInputLen > 1 ) { /* cMatching != 0 */
                ET9_CS_TrieSearch(pTrie, wFlags, wMohu, pChildNode, iChildOffset, pSymbInfo + 1 + iIgnoreSymb, iInputLen - 1 - iIgnoreSymb, pcNodeStack, pcMatched, iStackIdx, pcConsumed, iConsumedIdx, pCandGrp, /*fLowercaseNodeIgnored*/0, fPartialPinyin);
            }
            else if ( iIgnoreSymb == 0 && ET9_CS_PARTIAL_ENABLED(wFlags) && cMatching == 0 ) {  /* cMatching == 0 && cInTrie is lower case */
                ET9_CS_TrieSearch(pTrie, wFlags, wMohu, pChildNode, iChildOffset, pSymbInfo, iInputLen, pcNodeStack, pcMatched, iStackIdx, pcConsumed, iConsumedIdx, pCandGrp, /*fLowercaseNodeIgnored*/0, /*fPartialPinyin*/1);
            }
            if ( iIgnoreSymb == 0 && cMatching != 0 && iInputLen > 1  && ET9_CS_PARTIAL_ENABLED(wFlags) &&
                 iStackIdx >= 2 && ET9_CP_IsUpperCase(pcNodeStack[iStackIdx - 2]) && ET9_CP_IsLowerCase(pcNodeStack[iStackIdx - 1]) &&
                 !(SHOULD_TRY_H_RULE(wMohu, pcNodeStack[iStackIdx - 2]) && pcNodeStack[iStackIdx - 1] == 'h') )  {
                ET9U8 cSave = pcConsumed[iConsumedIdx - 1];
                ET9U8 fMatched = pcMatched[iStackIdx - 1];
                pcMatched[iStackIdx - 1] = (ET9U8)((iConsumedIdx >=2 && pcNodeStack[iStackIdx - 1] == pcConsumed[iConsumedIdx - 2])? 1: 0);
                ET9_CS_TrieSearch(pTrie, wFlags, wMohu, pChildNode, iChildOffset, pSymbInfo, iInputLen, pcNodeStack, pcMatched, iStackIdx, pcConsumed, iConsumedIdx - 1, pCandGrp, /*fLowercaseNodeIgnored*/1, /*fPartialPinyin*/1);
                pcMatched[iStackIdx - 1] = fMatched;
                pcConsumed[iConsumedIdx - 1] = cSave;
            }
            iSiblingDistance = Trie_SiblingDistance(pTrie, pChildNode, eChildType);
        }

        if (iInputLen >= 1 && cMatching == 'n' && ET9_CP_ShouldApplyGRule(wMohu, pcNodeStack, iStackIdx) )
        {
            if (iInputLen > 1 && ET9_CS_IsCharInSymbInfo('g', pSymbInfo + 1))
            {   /* ignore input 'g' */
                pcConsumed[iConsumedIdx] = 'g';
                iChildOffset = iNodeOffset;
                iSiblingDistance = iChildDistance;
                while (iSiblingDistance != 0)
                {
                    iChildOffset += iSiblingDistance;
                    eChildType = TRIE_LOAD_NODE( pTrie, iChildOffset, pChildNode, &bNodeSize );
                    ET9_CS_TrieSearch(pTrie, wFlags, wMohu, pChildNode, iChildOffset, pSymbInfo + 2, iInputLen - 2, pcNodeStack, pcMatched, iStackIdx, pcConsumed, iConsumedIdx + 1, pCandGrp, /*fLowercaseNodeIgnored*/0, fPartialPinyin);
                    iSiblingDistance = Trie_SiblingDistance(pTrie, pChildNode, eChildType);
                }
            }

            {   /* ignore child node 'g' */
                ET9U32 iOffsetEtc = FindOffsetOfChildHoldingChar(pTrie, iNodeOffset + iChildDistance, 'g');
                if ( iOffsetEtc != 0 )
                {
                    iChildOffset = (iOffsetEtc & 0xFFFFFF);
                    iSiblingDistance = iChildDistance = (iOffsetEtc >> 24);
                    pcNodeStack[iStackIdx] = 'g';
                    pcMatched[iStackIdx] = 1;
                    if ( iInputLen == 1 && ET9_CS_SEGMENT_SEARCH(wFlags) )
                    {
                        eChildType = TRIE_LOAD_NODE( pTrie, (iOffsetEtc & 0xFFFFFF), pChildNode, &bNodeSize );
                        if ( eChildType == eTail )
                            fEOW = 0;    /* First letter in the TailNode, not EOW */
                        else
                            fEOW = ET9_CS_END_OF_WORD( pChildNode );   /* eChar */
                        iProb = fEOW? ET9_CS_WORD_PROB( pChildNode, eChildType ): ET9_CS_MAX_DES_PROB( pChildNode, eChildType );
                        if ( ET9_CS_PARTIAL_ENABLED(wFlags) && !(fEOW && !fPartialPinyin) && eChildType == eChar )
                            fLSOW = Trie_IsLastSyllableOfWord(pChildNode);
                        else
                            fLSOW = 0;
                        ET9_CS_AddCandToGroup(pTrie, (wFlags & ET9_CS_BIT_CHECK_CONDITION)? 1: 0, pCandGrp, fPartialPinyin, fEOW, fLSOW, iNodeOffset, iProb, pcConsumed, iConsumedIdx);
                    }
                    else
                    {
                        while (iSiblingDistance != 0)
                        {
                            iChildOffset += iSiblingDistance;
                            eChildType = TRIE_LOAD_NODE( pTrie, iChildOffset, pChildNode, &bNodeSize );
                            ET9_CS_TrieSearch(pTrie, wFlags, wMohu, pChildNode, iChildOffset, pSymbInfo + 1, iInputLen - 1, pcNodeStack, pcMatched, iStackIdx + 1, pcConsumed, iConsumedIdx, pCandGrp, /*fLowercaseNodeIgnored*/0, fPartialPinyin);
                            iSiblingDistance = Trie_SiblingDistance(pTrie, pChildNode, eChildType);
                        }
                    }
                }
            }
        }  /* END OF if (iInputLen > 1 && cMatching == 'n' && ET9_CP_ShouldApplyGRule(wMohu, pcNodeStack, iStackIdx) )  */
    }
    if (iInputLen > 1 && cMatching != 0 && SHOULD_TRY_H_RULE(wMohu, pcNodeStack[iStackIdx - 1]) )
    {
        DECLARE_TRIE_NODE(pChildNode);
        ET9U8 bNodeSize;
        ET9_CS_EnumNodeType eChildType;
        ET9S32 iChildOffset;
        if (ET9_CS_IsCharInSymbInfo('h', pSymbInfo + 1))
        {   /* ignore input 'h' */
            pcConsumed[iConsumedIdx] = 'h';
            if ( iInputLen == 2 && ET9_CS_SEGMENT_SEARCH(wFlags) )
            {
                fEOW = ET9_CS_END_OF_WORD( pNode );  /* eChar */
                iProb = fEOW? ET9_CS_WORD_PROB( pNode, eType ): ET9_CS_MAX_DES_PROB( pNode, eType );
                if ( ET9_CS_PARTIAL_ENABLED(wFlags) && !(fEOW && !fPartialPinyin) )
                    fLSOW = Trie_IsLastSyllableOfWord(pNode);
                else
                    fLSOW = 0;
                ET9_CS_AddCandToGroup(pTrie, (wFlags & ET9_CS_BIT_CHECK_CONDITION)? 1: 0, pCandGrp, fPartialPinyin, fEOW, fLSOW, iNodeOffset, iProb, pcConsumed, iConsumedIdx + 1);
            }
            else if ( iInputLen == 3 && ET9_CS_SEGMENT_SEARCH(wFlags) && ET9_CP_SymbIsToneOrDelim(pSymbInfo + 2) )
            {
                if ( ET9_CS_PARTIAL_ENABLED(wFlags) )
                {
                    pcConsumed[iConsumedIdx + 1] = ET9_CP_GetSymbToneOrDelim(pSymbInfo);;
                    fEOW = ET9_CS_END_OF_WORD( pNode );   /* eChar */
                    iProb = fEOW? ET9_CS_WORD_PROB( pNode, eType ): ET9_CS_MAX_DES_PROB( pNode, eType );
                    if ( !(fEOW && !fPartialPinyin) )
                        fLSOW = Trie_IsLastSyllableOfWord(pNode);
                    else
                        fLSOW = 0;
                    ET9_CS_AddCandToGroup(pTrie, (wFlags & ET9_CS_BIT_CHECK_CONDITION)? 1: 0, pCandGrp, fPartialPinyin, fEOW, fLSOW, iNodeOffset, iProb, pcConsumed, iConsumedIdx + 2);
                }
                return;
            }
            else
            {
                iChildOffset = iNodeOffset;
                iSiblingDistance = iChildDistance;
                while (iSiblingDistance != 0)
                {
                    iChildOffset += iSiblingDistance;
                    eChildType = TRIE_LOAD_NODE( pTrie, iChildOffset, pChildNode, &bNodeSize );
                    if ( eChildType != eChar || (ET9U8)ET9_CS_GET_CHAR( pTrie, pChildNode ) != 'h' )
                        ET9_CS_TrieSearch(pTrie, wFlags, wMohu, pChildNode, iChildOffset, pSymbInfo + 2, iInputLen - 2, pcNodeStack, pcMatched, iStackIdx, pcConsumed, iConsumedIdx + 1, pCandGrp, /*fLowercaseNodeIgnored*/0, fPartialPinyin);
                    iSiblingDistance = Trie_SiblingDistance(pTrie, pChildNode, eChildType);
                }
            }
        }
        {   /* ignore child node 'h' */
            ET9U32 iOffsetEtc = FindOffsetOfChildHoldingChar(pTrie, iNodeOffset + iChildDistance, 'h');
            if ( iOffsetEtc != 0 )
            {
                iChildOffset = (iOffsetEtc & 0xFFFFFF);
                iSiblingDistance = iChildDistance = (iOffsetEtc >> 24);
                pcNodeStack[iStackIdx] = 'h';
                pcMatched[iStackIdx] = 1;
                while (iSiblingDistance != 0)
                {
                    iChildOffset += iSiblingDistance;
                    eChildType = TRIE_LOAD_NODE( pTrie, iChildOffset, pChildNode, &bNodeSize );
                    ET9_CS_TrieSearch(pTrie, wFlags, wMohu, pChildNode, iChildOffset, pSymbInfo + 1, iInputLen - 1, pcNodeStack, pcMatched, iStackIdx + 1, pcConsumed, iConsumedIdx, pCandGrp, /*fLowercaseNodeIgnored*/0, fPartialPinyin);
                    iSiblingDistance = Trie_SiblingDistance(pTrie, pChildNode, eChildType);
                }
            }
        }
    }
}

static void ET9LOCALCALL Trie_DepthFirstSearch( ET9_CP_SSBITrie* pTrie, const ET9SymbInfo *pSymbInfo, ET9S32 iInputLen, ET9_CS_CandidateGroup* pCandGrp, ET9INT fCheckPrefix )
{
    ET9S32 iNodeOffset;
    ET9U32 iSiblingDistance;
    ET9_CS_EnumNodeType eType;
    ET9U16 wMohu = ET9_CP_GetMohuFlags(ET9_CS_LINGUISTIC_INFO(pTrie));  /* For trad Chinese, wMohu == 0 */
    ET9U16 wFlags;

    ET9U8 acNodeStack[64] = {0};  /* consider ET9CPMAXSPELLSIZE */
    ET9U8 acMatched[64] = {0};
    ET9U8 acConsumed[ET9_CP_MAX_SBI_KEY_NUMBER + 1] = {0};
    ET9U8 bNodeSize;
    DECLARE_TRIE_NODE(pTopNode);

    wFlags = 0;
    if ( ET9_CS_IsPartialSpellActive(ET9_CS_LINGUISTIC_INFO(pTrie)) )
        wFlags |= ET9_CS_BIT_PARTIAL_PINYIN_ENABLED;
    if ( pTrie->nTrieType == ET9_CS_BPMF_TRIE )
        wFlags |= ET9_CS_BIT_BPMF;
    if ( fCheckPrefix )
        wFlags |= ET9_CS_BIT_CHECK_CONDITION;

    ET9Assert( pSymbInfo != NULL );
    if (ET9_CP_SymbIsToneOrDelim(pSymbInfo))
    {
        pSymbInfo++;
        iInputLen--;
    }
    if (iInputLen <= 0)
        return;

    iSiblingDistance = iNodeOffset = 4;
    while (iSiblingDistance != 0)
    {
        eType = TRIE_LOAD_NODE( pTrie, iNodeOffset, pTopNode, &bNodeSize );
        ET9_CS_TrieSearch(pTrie, wFlags, wMohu, pTopNode, iNodeOffset, pSymbInfo, iInputLen, acNodeStack, acMatched, /*iStackIdx*/0, acConsumed, /*iConsumedIdx*/0, pCandGrp, /*fLowercaseNodeIgnored*/0, /*fPartialPinyin*/0);
        iSiblingDistance = Trie_SiblingDistance(pTrie, pTopNode, eType);
        iNodeOffset += iSiblingDistance;
    }
}

static ET9BOOL ET9LOCALCALL Trie_BufMatchPrefix(ET9_CP_SSBITrie* pTrie, ET9U8 * pcBuf, ET9S32 iBufLen)
{
    const ET9_CS_Candidate* pCandidate;
    ET9S32 i, iConditionLen = (ET9S32)pTrie->m_bSBIConditionLen;
    const ET9U8* pcPrefix;
    ET9U8  c;

    if ( iConditionLen == 0 )
        return 1;
    if ( iBufLen < iConditionLen )
        return 0;

    pCandidate = GetCandidate( &pTrie->m_aSegCandGrp[iConditionLen - 1].m_arrExactMatch, 0 );
    pcPrefix = (const ET9U8*)pCandidate->m_pcSeg;
    for ( i = 0; i < iConditionLen; i++ ) {
        if ( pcBuf[i] != pcPrefix[i] )
            return 0;
    }
    if ( iBufLen == (ET9S32)iConditionLen )
        return 1;

    c = pcBuf[iConditionLen];  /* Now,  iBufLen > (ET9S32)pTrie->m_bSBIConditionLen */
    if ( c == ET9CPSYLLABLEDELIMITER || ET9CPSymToCPTone(c) || ET9_CP_IsUpperCase(c) )
        return 1;

    return 0;
}


static void ET9LOCALCALL Trie_AddCandidate(ET9_CP_SSBITrie* pTrie, ET9S32 iKeyIndex, ET9_CS_CandidateGroup* pTailGrp )
{
    ET9S32 iCount1, iTailExact, iTailPartial;
    const ET9_CS_Candidate* pSegment1, * pSegment2;
    ET9_CS_CandidateGroup* pDstCandGrp;
    ET9U8 acTmp[ET9_CP_MAX_CANDIDATE_LENGTH];
    ET9S32 iLen, iNewProb;

    iTailExact = GetCandidateCount( &pTailGrp->m_arrExactMatch );
    iTailPartial = GetCandidateCount( &pTailGrp->m_arrPartialMatch );
    if ( iTailExact == 0 && iTailPartial == 0 ) {
        return;
    }

    pDstCandGrp = pTrie->m_aSegCandGrp + pTrie->m_bSegCandGrpCount;
    if ( iKeyIndex < 1 ) {
        ET9S32 i;
        for ( i = 0; i < iTailExact; i++ ) {
            const ET9_CS_Candidate* pSegment = GetCandidate(&pTailGrp->m_arrExactMatch, i);
            ET9Assert( pSegment != NULL );
            iNewProb = pSegment->m_iSegProb;
            iLen = ET9_CP_CopyExcludeDelimiter( acTmp, (const ET9U8 *)pSegment->m_pcSeg, (ET9S32)pSegment->m_bSegLen );
            SegCandGrpAdd(pTrie, pDstCandGrp, eExactMatch, pSegment->m_iNodeOffset, iNewProb, acTmp, (ET9U8)iLen );
        }

        for ( i = 0; i < iTailPartial; i++ ) {
            const ET9_CS_Candidate* pSegment = GetCandidate(&pTailGrp->m_arrPartialMatch, i);
            ET9Assert( pSegment != NULL );
            iNewProb = pSegment->m_iSegProb;
            iLen = ET9_CP_CopyExcludeDelimiter( acTmp, (const ET9U8 *)pSegment->m_pcSeg, (ET9S32)pSegment->m_bSegLen );
            SegCandGrpAdd(pTrie, pDstCandGrp, ePartialMatch, pSegment->m_iNodeOffset, iNewProb, acTmp, (ET9U8)iLen );
        }
        return;
    }

    /* Now nKeyIndex >= 1 */
    iCount1 = GetCandidateCount( &pTrie->m_aSegCandGrp[iKeyIndex - 1].m_arrExactMatch );
    {
        ET9S32 i1, i2;
        for (i1 = 0; i1 < pTailGrp->m_bCandBufferSize; i1++ )
        {
            for ( i2 = 0; i2 <= i1; i2++ )
            {
                if ( (i1 - i2) >= iCount1 || i2 >= iTailExact )
                    continue;
                pSegment1 = GetCandidate(&pTrie->m_aSegCandGrp[iKeyIndex - 1].m_arrExactMatch, (i1 - i2));
                pSegment2 = GetCandidate(&pTailGrp->m_arrExactMatch, i2);
                ET9Assert( pSegment1 != NULL && pSegment2 != NULL );
                iNewProb = ET9_CS_PROB_PPRODUCT(pTrie, pSegment1->m_iSegProb, pSegment2->m_iSegProb) - ET9_CS_MULTIPLE_SEG_THRESHOLD;
                iLen = ET9_CP_CopyExcludeDelimiter( acTmp, (const ET9U8 *)pSegment1->m_pcSeg, (ET9S32)pSegment1->m_bSegLen );
                acTmp[iLen] = ET9CP_SEGMENT_DELIMITER;
                iLen += 1;
                iLen += ET9_CP_CopyExcludeDelimiter( acTmp + iLen, (const ET9U8 *)pSegment2->m_pcSeg, (ET9S32)pSegment2->m_bSegLen );
                SegCandGrpAdd(pTrie, pDstCandGrp, eExactMatch, pSegment2->m_iNodeOffset, iNewProb, acTmp, (ET9U8)iLen );
            }
        }
        for (i1 = 0; i1 < pTailGrp->m_bCandBufferSize; i1++ )
        {
            for ( i2 = 0; i2 <= i1; i2++ )
            {
                if ( (i1 - i2) >= iCount1 || i2 >= iTailPartial )
                    continue;
                pSegment1 = GetCandidate(&pTrie->m_aSegCandGrp[iKeyIndex - 1].m_arrExactMatch, (i1 - i2));
                pSegment2 = GetCandidate(&pTailGrp->m_arrPartialMatch, i2);
                ET9Assert( pSegment1 != NULL && pSegment2 != NULL );
                iNewProb = ET9_CS_PROB_PPRODUCT(pTrie, pSegment1->m_iSegProb, pSegment2->m_iSegProb) - ET9_CS_MULTIPLE_SEG_THRESHOLD;
                iLen = ET9_CP_CopyExcludeDelimiter( acTmp, (const ET9U8 *)pSegment1->m_pcSeg, (ET9S32)pSegment1->m_bSegLen );
                acTmp[iLen] = ET9CP_SEGMENT_DELIMITER;
                iLen += 1;
                iLen += ET9_CP_CopyExcludeDelimiter( acTmp + iLen, (const ET9U8 *)pSegment2->m_pcSeg, (ET9S32)pSegment2->m_bSegLen );
                SegCandGrpAdd(pTrie, pDstCandGrp, ePartialMatch, pSegment2->m_iNodeOffset, iNewProb, acTmp, (ET9U8)iLen );
            }
        }
    }
}

static ET9BOOL ET9LOCALCALL Trie_AddSymb( ET9_CP_SSBITrie* pTrie, const ET9SymbInfo *pSI )
{
    const ET9SymbInfo *pSymbInfo;
    ET9S32 iInputLen;
    ET9INT fCheckPrefix;
    ET9_CS_Candidate  aXMatch[ET9_CP_SBI_CANDIDATE_COUNT];
    ET9_CS_Candidate  aPMatch[ET9_CP_SBI_CANDIDATE_COUNT];
    ET9_CS_CandidateGroup objCandGrp;
    ET9BOOL fAddDelimiter, fCandAdded = 0;

    if ( pTrie->m_bSegCandGrpCount >= ET9_CP_MAX_SBI_KEY_NUMBER )
    {
        return 0;
    }
    SegCandGrpClear( pTrie->m_aSegCandGrp + pTrie->m_bSegCandGrpCount );
    objCandGrp.m_bCandBufferSize = ET9_CP_SBI_CANDIDATE_COUNT;
    objCandGrp.m_arrExactMatch.m_pCandidates = aXMatch;
    objCandGrp.m_arrPartialMatch.m_pCandidates = aPMatch;

    fAddDelimiter = (ET9BOOL)ET9_CP_SymbIsDelim(pSI);
    if ( fAddDelimiter )    /* Delimiter at the end of the input  */
    {
        const ET9SymbInfo * pPrev = (pSI == ET9_CS_WORDSYMBOL_INFO(pTrie)->SymbsInfo)? NULL: (pSI - 1);
        ET9_CP_SpellMatch eMatchType = pTrie->m_bSegCandGrpCount == 0? eNoMatch: SegCandGrpType( pTrie->m_aSegCandGrp + (pTrie->m_bSegCandGrpCount - 1) );
              /* leading # is invalid */       /* "##" is invalid   */
        if ( (pPrev && ET9_CP_SymbIsToneOrDelim(pPrev)) || eMatchType == eNoMatch )
        {
            SegCandGrpClear( pTrie->m_aSegCandGrp + pTrie->m_bSegCandGrpCount );
            pTrie->m_bSegCandGrpCount++;
            return 0;
        }
        ET9Assert(pTrie->m_bSegCandGrpCount > 0);
        if ( eMatchType == eExactMatch )
        {
            SegCandGrpCopy( pTrie->m_aSegCandGrp + pTrie->m_bSegCandGrpCount,  pTrie->m_aSegCandGrp + (pTrie->m_bSegCandGrpCount - 1) );
            pTrie->m_aSegCandGrp[pTrie->m_bSegCandGrpCount].m_arrPartialMatch.m_bCandidateCount = 0;
            if ( ET9_CS_IsPartialSpellActive(ET9_CS_LINGUISTIC_INFO(pTrie)) )
            {   /* if Partial pinyin is enabled, Done */
                _ET9ByteCopy( pTrie->m_aSegCandGrp[pTrie->m_bSegCandGrpCount].m_abTailMatch, pTrie->m_aSegCandGrp[pTrie->m_bSegCandGrpCount - 1].m_abTailMatch, pTrie->m_bSegCandGrpCount );
                pTrie->m_aSegCandGrp[pTrie->m_bSegCandGrpCount].m_abTailMatch[pTrie->m_bSegCandGrpCount] = ET9CPSYLLABLEDELIMITER;
                pTrie->m_bSegCandGrpCount++;
                return 1;
            }
            /* if Partial pinyin is disabled, pass throug to re-compute the m_abTailMatch */
            fCandAdded = 1;
        }
        /* else pass throug:  eMatchType == ePartialMatch */
    }
    {
        ET9S32 nEnd, nStart;
        nEnd = pTrie->m_bSegCandGrpCount + 1;
        nStart = 0x7FFFFFFF;
        if ( pTrie->m_bSegCandGrpCount == 0 )
        {
            nStart = 0;
        }
        else
        {
            ET9U32 uLetterCount = 0;
            nStart = pTrie->m_bSegCandGrpCount;
            while ( nStart > 0 )
            {
                if ( !ET9_CP_SymbIsToneOrDelim(&ET9_CS_WORDSYMBOL_INFO(pTrie)->SymbsInfo[nStart]) )
                {
                    uLetterCount++;
                    if ( uLetterCount >= pTrie->nMaxSpellLen )
                    {
                        break;
                    }
                }
                nStart--;
            }
        }

        ET9Assert( nStart != 0x7FFFFFFF );
        {
            ET9S32 i, iTailExact, iTailPartial;
            ET9WordSymbInfo * pWSI = ET9_CS_WORDSYMBOL_INFO(pTrie);

            for ( i = 0; i < nEnd; i++ )
            {
                ET9_CS_CandidateGroup * pLastCandGrp = NULL;
                if ( i > 0 )
                {
                    pLastCandGrp = pTrie->m_aSegCandGrp + (i - 1);
                }
                if ( i > 0  && SegCandGrpType( pLastCandGrp ) == eNoMatch )
                {
                    continue;
                }
                SegCandGrpClear( &objCandGrp );
                fCheckPrefix = (ET9BOOL)((i==0)? 1: 0);
                pSymbInfo = pWSI->SymbsInfo + ET9_CP_SelectionHistUnselectedStart(&pTrie->pET9CPLingInfo->SelHistory) + i;
                iInputLen = nEnd - i;

                ET9_CP_GetUdbSpellings(ET9_CS_LINGUISTIC_INFO(pTrie), pSymbInfo, iInputLen, &objCandGrp);
                if ( i >= nStart )
                {
                    Trie_DepthFirstSearch( pTrie, pSymbInfo, iInputLen, &objCandGrp, fCheckPrefix );
                }
                if ( !fCandAdded && (i == 0  || SegCandGrpType(pLastCandGrp) == eExactMatch) )
                {   /* if (fCandAdded) , Candidate is already added to pTrie->m_aSegCandGrp, just re-compute m_abTailMatch */
                    Trie_AddCandidate( pTrie, i, &objCandGrp );
                }
                if (ET9_CP_SymbIsToneOrDelim(pSymbInfo))
                {
                    pTrie->m_aSegCandGrp[pTrie->m_bSegCandGrpCount].m_abTailMatch[i] = ET9CPSYLLABLEDELIMITER;
                }
                else
                {
                    pTrie->m_aSegCandGrp[pTrie->m_bSegCandGrpCount].m_abTailMatch[i] = (ET9U8)objCandGrp.m_eMatchType;
                }
                if (ET9_CP_InputHasTone(pTrie->pET9CPLingInfo)) {
                    break; /* if input has tone, reject multi-segment by only trying i == 0 */
                }
            }
            iTailExact = GetCandidateCount(&pTrie->m_aSegCandGrp[pTrie->m_bSegCandGrpCount].m_arrExactMatch);
            iTailPartial = GetCandidateCount(&pTrie->m_aSegCandGrp[pTrie->m_bSegCandGrpCount].m_arrPartialMatch);
            if ( iTailExact > 0 )
            {
                pTrie->m_aSegCandGrp[pTrie->m_bSegCandGrpCount].m_eMatchType = eExactMatch;
            }
            else if ( iTailPartial > 0 )
            {
                pTrie->m_aSegCandGrp[pTrie->m_bSegCandGrpCount].m_eMatchType = ePartialMatch;
            }
            else
            {
                pTrie->m_aSegCandGrp[pTrie->m_bSegCandGrpCount].m_eMatchType = eNoMatch;
            }
        }
        pTrie->m_bSegCandGrpCount++;
    }
    return 1;
}

static void ET9LOCALCALL Trie_AddSymbols( ET9_CP_SSBITrie* pTrie, const ET9WordSymbInfo *pWSI, ET9U8 bStart, ET9U8 bEnd )
{
    ET9U8 b;
    const ET9SymbInfo *pSymbInfo;

    if (bStart >= bEnd)
        return;

    ET9_FUNC_HIT("Trie_AddSymbols");
    pSymbInfo = pWSI->SymbsInfo;
    for ( b = bStart; b < bEnd; b++ )
    {
        Trie_AddSymb(pTrie, &pSymbInfo[b]);
    }
}

#if ET9_CS_CONTAIN_UNUSED_FUNCTION
static ET9BOOL ET9LOCALCALL Trie_DelSymb( ET9_CP_SSBITrie* pTrie )
{
    ET9Assert( pTrie != NULL );
    if ( ET9_CP_SelectionHistGet(&pTrie->pET9CPLingInfo->SelHistory, NULL, NULL, &spell) != ET9STATUS_NONE)
        return 0;
    if ( pTrie->m_bSegCandGrpCount == 0 ) {
        return 0;
    }
    pTrie->m_bSegCandGrpCount--;
    if ( pTrie->m_bSBIConditionLen ) {
        ET9_CS_SetCondition( pTrie, NULL, 0, 0 );
    }
    return 1;
}
#endif

static ET9_CP_SpellMatch ET9LOCALCALL Trie_RebuildCandidates(ET9_CP_SSBITrie* pTrie, ET9U8 fAllSymbols)
{
    ET9_CP_SpellMatch eType;
    ET9U8 bUnselectedStart = ET9_CP_SelectionHistUnselectedStart(&pTrie->pET9CPLingInfo->SelHistory);
    ET9U8 bSymbolEnd = ET9_CS_WORDSYMBOL_INFO(pTrie)->bNumSymbs;

    if ( !fAllSymbols )
    {
        bSymbolEnd = (ET9U8)__ET9Min( ET9_CS_WORDSYMBOL_INFO(pTrie)->bNumSymbs, (bUnselectedStart + pTrie->m_bSegCandGrpCount) );
    }
    pTrie->m_bSBIConditionLen = 0;
    pTrie->m_bSegCandGrpCount = 0;
    Trie_AddSymbols(pTrie, ET9_CS_WORDSYMBOL_INFO(pTrie), bUnselectedStart, bSymbolEnd);

    eType = SegCandGrpType( pTrie->m_aSegCandGrp + (pTrie->m_bSegCandGrpCount - 1) );
    return eType;
}

/*************************************************************************
 *
 * public functions  of ET9_CP_SSBITrie
 *
 *************************************************************************/
ET9STATUS ET9FARCALL ET9_CS_SysInit( ET9_CP_SSBITrie* pTrie, ET9CPLingInfo * pET9CPLingInfo )
{
    ET9INT i;
    ET9_CS_Candidate * pExact, * pPartial;

    ET9Assert( pTrie != NULL && pET9CPLingInfo != NULL );
    if (pET9CPLingInfo->Private.wInfoInitOK != ET9GOODSETUP) {
        return ET9STATUS_NO_INIT;
    }

    ET9_CS_LINGUISTIC_INFO(pTrie) = pET9CPLingInfo;

    pTrie->nNodeCount = 0;
    pTrie->nTrieNodesOffset = 0;   /* Offset in the whole LDB */
    pTrie->nTailStringOffset = 0;  /* Offset in the whole LDB */
    pTrie->nSiblingDisCount = 0;
    pTrie->nSiblingDisArrOffset = 0;
    pTrie->nTailStrOffsCount = 0;
    pTrie->nTailStrOffsArrOffset = 0;
    pTrie->nNodeByteCount = 0;
    pTrie->nStrLength = 0;
    pTrie->nTrieVersion   = 0;
    pTrie->iLogProbShift = 0;
    pExact = pTrie->m_aExactMatchPool;
    pPartial = pTrie->m_aPartialMatchPool;

    for ( i = 0; i < sizeof(pTrie->m_aSegCandGrp) / sizeof(pTrie->m_aSegCandGrp[0]); i++ )
    {
        SegCandGrpClear( pTrie->m_aSegCandGrp + i );
        pTrie->m_aSegCandGrp[i].m_arrExactMatch.m_pCandidates = pExact;
        pTrie->m_aSegCandGrp[i].m_arrPartialMatch.m_pCandidates = pPartial;
        pExact += ET9_CP_SBI_CANDIDATE_COUNT;
        pPartial += ET9_CP_SBI_CANDIDATE_COUNT;
    }

    ET9_CS_ResetSBI( pTrie );
    pTrie->wInitOK = 0;

    pET9CPLingInfo->CommonInfo.bPrefixSortMethod = ET9_CP_PREFIX_SORT_BY_LENGTH;

    return ET9STATUS_NONE;
}

ET9STATUS ET9FARCALL ET9_CS_SBIInit( ET9_CP_SSBITrie* pTrie )
{
    ET9U8  bSBI_Count = 0xFF, bSBIType = 0xFF;
    ET9U32 nSBIOffset = 0;

    ET9U32 uTotal, dwReadOffset;
    ET9_CP_LdbOffsets *pLdbOffset;
    ET9Assert( pTrie != NULL );

    ET9_CS_ResetSBI(pTrie);
    pTrie->wInitOK = 0;

    ET9_CP_CHECK_LINGINFO(ET9_CS_LINGUISTIC_INFO(pTrie));

    pLdbOffset = &(ET9_CS_LINGUISTIC_INFO(pTrie)->CommonInfo.sOffsets);
    if ( pLdbOffset->dwSBITrieOffset == 0 ) {
        return ET9STATUS_INVALID_MODE;
    }

    /* Reading header */
    uTotal = 0;
    dwReadOffset = pLdbOffset->dwSBITrieOffset;

    bSBI_Count = ET9_CP_LdbReadByte(ET9_CS_LINGUISTIC_INFO(pTrie), dwReadOffset++);
    {
        ET9U8 bExpectType = (ET9U8)(ET9CPIsModePinyin(ET9_CS_LINGUISTIC_INFO(pTrie))? ET9_CS_PINYIN_TRIE:
                                    ET9CPIsModeBpmf(ET9_CS_LINGUISTIC_INFO(pTrie))? ET9_CS_BPMF_TRIE:
                                    0xFF);
        ET9U8 b, bFound = 0;

        for ( b = 0; b < bSBI_Count; b++ )
        {
            bSBIType = ET9_CP_LdbReadByte(ET9_CS_LINGUISTIC_INFO(pTrie), dwReadOffset++);
            nSBIOffset = ET9_CP_LdbReadDWord(ET9_CS_LINGUISTIC_INFO(pTrie), dwReadOffset);
            dwReadOffset += 4;
            if ( bSBIType == bExpectType )
            {
                bFound = 1;
                break;
            }
        }
        if ( bFound == 0 )
            return ET9STATUS_INVALID_MODE;
    }

    dwReadOffset = pLdbOffset->dwSBITrieOffset + nSBIOffset;

    pTrie->nTrieVersion   = ET9_CP_LdbReadWord(ET9_CS_LINGUISTIC_INFO(pTrie), dwReadOffset);
    dwReadOffset += 2;
    uTotal += 2;

    pTrie->nTrieType = ET9_CP_LdbReadByte(ET9_CS_LINGUISTIC_INFO(pTrie), dwReadOffset++);
    uTotal += 1;

    pTrie->nMaxSpellLen = (ET9U32)ET9_CP_LdbReadByte(ET9_CS_LINGUISTIC_INFO(pTrie), dwReadOffset++);
    uTotal += 1;

    ET9Assert( pTrie->nTrieVersion == ET9_CS_SBI_VERSION );
    ET9Assert( pTrie->nTrieType == bSBIType );
    ET9Assert( pTrie->nMaxSpellLen > 0 && pTrie->nMaxSpellLen <= ET9_CP_MAX_SPELL_LENGTH );

    pTrie->nNodeCount = ET9_CP_LdbReadDWord(ET9_CS_LINGUISTIC_INFO(pTrie), dwReadOffset);
    dwReadOffset += 4;
    uTotal += 4;

    pTrie->nNodeByteCount = ET9_CP_LdbReadDWord(ET9_CS_LINGUISTIC_INFO(pTrie), dwReadOffset);
    dwReadOffset += 4;
    uTotal += 4;

    pTrie->nStrLength = ET9_CP_LdbReadDWord(ET9_CS_LINGUISTIC_INFO(pTrie), dwReadOffset);
    dwReadOffset += 4;
    uTotal += 4;

    pTrie->iLogProbShift = ET9_CP_LdbReadDWord(ET9_CS_LINGUISTIC_INFO(pTrie), dwReadOffset);
    dwReadOffset += 4;
    uTotal += 4;
    /* End of Reading header */

    pTrie->nTrieNodesOffset = pLdbOffset->dwSBITrieOffset + nSBIOffset + uTotal;
    pTrie->nTailStringOffset = pTrie->nTrieNodesOffset + pTrie->nNodeByteCount;

    if ( pTrie->nTrieVersion == 3 )
    {
        dwReadOffset = pTrie->nTailStringOffset + pTrie->nStrLength;
        pTrie->nSiblingDisCount = ET9_CP_LdbReadDWord(ET9_CS_LINGUISTIC_INFO(pTrie), dwReadOffset);
        dwReadOffset += 4;
        pTrie->nSiblingDisArrOffset = dwReadOffset;

        dwReadOffset += pTrie->nSiblingDisCount * 4;
        pTrie->nTailStrOffsCount = ET9_CP_LdbReadDWord(ET9_CS_LINGUISTIC_INFO(pTrie), dwReadOffset);
        dwReadOffset += 4;
        pTrie->nTailStrOffsArrOffset = dwReadOffset;
    }

    {  /* BPMF support */
        ET9INT i;
        ET9U8 bThrshd = (ET9U8)(pTrie->nTrieType == ET9_CS_BPMF_TRIE? ET9_CP_BPMF_SHORT_KEY_THRSHD: ET9_CP_PINYIN_SHORT_KEY_THRSHD);

        for ( i = 0; i < 256; i++ )
        {
            pTrie->arrEncodingTrieToInternal[i] = (ET9U8)i;
        }
        if (bSBIType == ET9_CS_BPMF_TRIE)
        {
            for ( i = 0; i <= ET9CPBpmfLetterCount; i++ )
            {
                pTrie->arrEncodingTrieToInternal[ET9_CS_BPMF_FIRST_LOWER_IN_TRIE + i] = (ET9U8)(ET9_CP_BPMFFIRSTLOWERLETTER + i);
                pTrie->arrEncodingTrieToInternal[ET9_CS_BPMF_FIRST_UPPER_IN_TRIE + i] = (ET9U8)(ET9_CP_BPMFFIRSTUPPERLETTER + i);
            }
        }

        for ( i = 0; i < (ET9INT)bThrshd; i++ )
        {
            SegCandGrpClear( pTrie->m_aSegCandGrp + i );
            pTrie->m_aSegCandGrp[i].m_bCandBufferSize = ET9_CP_SBI_CANDIDATE_COUNT;
        }
        for ( ; i < sizeof(pTrie->m_aSegCandGrp) / sizeof(pTrie->m_aSegCandGrp[0]); i++ )
        {
            SegCandGrpClear( pTrie->m_aSegCandGrp + i );
            pTrie->m_aSegCandGrp[i].m_bCandBufferSize = ET9_CP_SBI_CANDIDATE_COUNT;
        }
    }
    pTrie->wInitOK = ET9GOODSETUP;

    return ET9STATUS_NONE;
}

void ET9FARCALL ET9_CS_ClearPrefixBuf( ET9_CP_SSBITrie* pTrie )
{
    pTrie->pET9CPLingInfo->CommonInfo.bActivePrefix = 0xFF;
    pTrie->pET9CPLingInfo->CommonInfo.bSyllablePrefixCount = 0;
    PrefixBufferClear(&pTrie->m_PrefixBuffer);
}

void ET9FARCALL ET9_CS_PhrasalPrefix( ET9_CP_SSBITrie* pTrie )
{
    ET9SymbInfo *pSymbInfo;
    ET9S32 iInputLen;
    ET9U16 wMohu, wFlags;
    ET9_CS_EnumNodeType eType;
    ET9U8 acNodeStack[64] = {0};   /* consider ET9CPMAXSPELLSIZE */
    ET9U8 acMatched[64] = {0};
    ET9U8 acConsumed[ET9_CP_MAX_SBI_KEY_NUMBER + 1] = {0};
    DECLARE_TRIE_NODE(pTopNode);
    ET9U8 bNodeSize;
    ET9S32 iSiblingDistance, iNodeOffset;
    ET9Assert( pTrie != NULL );
    if (pTrie->wInitOK != ET9GOODSETUP) {
        return;
    }

    pSymbInfo = ET9_CS_WORDSYMBOL_INFO(pTrie)->SymbsInfo + ET9_CP_SelectionHistUnselectedStart(&pTrie->pET9CPLingInfo->SelHistory);
    iInputLen = (ET9S32)(ET9_CS_WORDSYMBOL_INFO(pTrie)->bNumSymbs - ET9_CP_SelectionHistUnselectedStart(&pTrie->pET9CPLingInfo->SelHistory));
    wMohu = ET9_CP_GetMohuFlags(ET9_CS_LINGUISTIC_INFO(pTrie) );  /* For trad Chinese, wMohu == 0 */
    wFlags = 0;
    if ( ET9_CS_IsPartialSpellActive(ET9_CS_LINGUISTIC_INFO(pTrie)) )
        wFlags |= ET9_CS_BIT_PARTIAL_PINYIN_ENABLED;
    if ( pTrie->nTrieType == ET9_CS_BPMF_TRIE )
        wFlags |= ET9_CS_BIT_BPMF;
    wFlags |= ET9_CS_BIT_PREFIX_SEARCH;

    ET9_CS_AddUdbPrefix(ET9_CS_LINGUISTIC_INFO(pTrie), pSymbInfo, iInputLen);
    iSiblingDistance = iNodeOffset = 4;
    while (iSiblingDistance != 0)
    {
        eType = TRIE_LOAD_NODE( pTrie, iNodeOffset, pTopNode, &bNodeSize );
        ET9_CS_TrieSearch(pTrie, wFlags, wMohu, pTopNode, iNodeOffset, pSymbInfo, iInputLen, acNodeStack, acMatched, /*iStackIdx*/0, acConsumed, /*iConsumedIdx*/0, NULL, /*fLowercaseNodeIgnored*/0, /*fPartialPinyin*/0);
        iSiblingDistance = Trie_SiblingDistance(pTrie, pTopNode, eType);
        iNodeOffset += iSiblingDistance;
    }
}

void ET9FARCALL ET9_CS_ResetSBI( ET9_CP_SSBITrie* pTrie )
{
    ET9Assert( pTrie != NULL );
    pTrie->m_bSegCandGrpCount = 0;
    pTrie->m_bSBIConditionLen = 0;
    PrefixBufferClear(&pTrie->m_PrefixBuffer);
}

/* return ET9STATUS_NONE:          Ok
          ET9STATUS_EMPTY:         No Condition */
ET9STATUS ET9FARCALL ET9_CS_GetCondition(ET9_CP_SSBITrie* pTrie, ET9_CS_Prefix * pPfx)
{
    const ET9_CS_Candidate* pCandidate;
    if (pTrie->m_bSBIConditionLen == 0)
        return ET9STATUS_EMPTY;
    pCandidate = GetCandidate( &pTrie->m_aSegCandGrp[pTrie->m_bSBIConditionLen - 1].m_arrExactMatch, 0 );
    _ET9ByteCopy(pPfx->m_pcPfx, pCandidate->m_pcSeg, pCandidate->m_bSegLen);
    pPfx->m_bPfxLen = pCandidate->m_bSegLen;
    pPfx->m_iPfxProb = pCandidate->m_iSegProb;
    return ET9STATUS_NONE;
}

/* pcPrefix is a syllable, contains no ET9CP_SEGMENT_DELIMITER, '_'  */
/* return ET9STATUS_NONE:          Ok
          ET9STATUS_NO_INIT:       Not init'ed
          ET9STATUS_INVALID_INPUT: pcPrefix is invalid
          ET9STATUS_NO_MATCH:      pWSI is invalid */
ET9STATUS ET9FARCALL ET9_CS_SetCondition(ET9_CP_SSBITrie* pTrie, const ET9U8 * pcCondition, ET9S32 iConditionLen, ET9S32 iPfxProb)
{
    const ET9SymbInfo *pSymbInfo;
    ET9U8 bUnselectedStart;
    ET9U32 i, uKeyTotal;

    ET9Assert( pTrie != NULL );
    ET9Assert(pTrie->wInitOK == ET9GOODSETUP);

    bUnselectedStart = ET9_CP_SelectionHistUnselectedStart(&pTrie->pET9CPLingInfo->SelHistory);
    pSymbInfo = ET9_CS_WORDSYMBOL_INFO(pTrie)->SymbsInfo + bUnselectedStart;

    if ( pcCondition == NULL || iConditionLen == 0 ) {  /* Clear Condition */
        ET9_CP_SpellMatch eType;
        if ( pTrie->m_bSBIConditionLen == 0 ) {
            return ET9STATUS_NO_OPERATION;
        }
        ET9Assert( ET9_CS_WORDSYMBOL_INFO(pTrie)->bNumSymbs > 0 && pTrie->m_bSegCandGrpCount > 0 );
        for ( i = bUnselectedStart; i < (ET9U32)(bUnselectedStart + pTrie->m_bSegCandGrpCount); i++ ) {
            SegCandGrpClear( pTrie->m_aSegCandGrp + i );
        }
        eType = Trie_RebuildCandidates( pTrie, 0 );
        return eType == eNoMatch? ET9STATUS_NO_MATCH: ET9STATUS_NONE;
    }

    if ( (ET9_CP_StrIsPrefixOf((const ET9U8 *)pcCondition, iConditionLen, pSymbInfo, pTrie->m_bSegCandGrpCount) & ePrefix1) == 0 ) {
        return ET9STATUS_INVALID_INPUT;
    }

    for ( i = 0; i < (ET9U32)iConditionLen; i++ ) {
        SegCandGrpClear0( pTrie->m_aSegCandGrp + i );
    }
    SegCandGrpSet( pTrie->m_aSegCandGrp + (iConditionLen - 1), eExactMatch, iPfxProb, (const ET9U8 *)pcCondition, (ET9U32)iConditionLen);

    uKeyTotal = pTrie->m_bSegCandGrpCount;
    pTrie->m_bSBIConditionLen = (ET9U8)iConditionLen;
    pTrie->m_bSegCandGrpCount = (ET9U8)iConditionLen;
    Trie_AddSymbols(pTrie, ET9_CS_WORDSYMBOL_INFO(pTrie), (ET9U8)(bUnselectedStart + pTrie->m_bSegCandGrpCount), (ET9U8)(bUnselectedStart + uKeyTotal));

    return ET9STATUS_NONE;
}  /* END OF ET9_CS_SetCondition() */

static ET9U8 ET9LOCALCALL CopyAddDelimiterTone( ET9U8* pcTgt, const ET9U8* pcSrc, ET9S32 iSrcLen, const ET9SymbInfo *pSymbInfo, ET9S32 iSymbLen )
{
    ET9S32 iCopied = 0;
    while ( iSrcLen && iSymbLen )  {
        if ( ET9_CP_SymbIsToneOrDelim(pSymbInfo) ) {
            *pcTgt++ = ET9_CP_GetSymbToneOrDelim(pSymbInfo);
            pSymbInfo++;
            iSymbLen--;
            iCopied++;
        }
        else if ( *pcSrc == ET9CP_SEGMENT_DELIMITER ) {
            *pcTgt++ = *pcSrc++;
            iSrcLen--;
            iCopied++;
        }
        else  {
            *pcTgt++ = *pcSrc++;
            iSrcLen--;
            pSymbInfo++;
            iSymbLen--;
            iCopied++;
        }
    }
    if ( ET9_CP_SymbIsToneOrDelim(pSymbInfo) ) {
        *pcTgt++ = ET9_CP_GetSymbToneOrDelim(pSymbInfo);
        pSymbInfo++;
        iSymbLen--;
        iCopied++;
    }
    *pcTgt = 0;
    return (ET9U8)iCopied;
}

/* fill pSpell with the default segmentation. returns segmentation's match type */
ET9_CP_SpellMatch ET9FARCALL ET9_CP_SegmentationToSpell(ET9CPLingInfo * const pET9CPLingInfo, ET9_CP_Spell * pSpell)
{
    ET9_CP_SpellMatch eMatchType;
    ET9_CP_SSBITrie * pTrie = &pET9CPLingInfo->SBI;
    ET9_CS_Candidate cand;
    const ET9SymbInfo * pSymbInfo = pET9CPLingInfo->Base.pWordSymbInfo->SymbsInfo + ET9_CP_SelectionHistUnselectedStart(&pET9CPLingInfo->SelHistory);

    eMatchType = ET9_CS_GetCandidate(pTrie, 0, &cand);

    if (eNoMatch == eMatchType) {
        return eMatchType;
    }

    pSpell->bLen = CopyAddDelimiterTone(pSpell->pbChars, cand.m_pcSeg, cand.m_bSegLen, pSymbInfo, pTrie->m_bSegCandGrpCount);

    return eMatchType;
}

ET9_CP_SpellMatch ET9FARCALL ET9_CS_GetCandidate( const ET9_CP_SSBITrie* pTrie, ET9S32 i, ET9_CS_Candidate * pCandidate )
{
    ET9S32 iCountExact, iCountPartial, iTotal = ET9_CS_GetCandidateCount(pTrie);
    const ET9_CS_Candidate* pSeg;
    ET9_CP_SpellMatch eType = eNoMatch;
    const ET9_CS_CandidateGroup * pGrp = NULL;

    ET9Assert( pTrie != NULL );
    ET9Assert( pCandidate != NULL );
    if ( i < 0 || i >= iTotal ) { /* pTrie->m_bSegCandGrpCount == 0 ==> iTotal == 0 */
        return eNoMatch;
    }

    pGrp = &pTrie->m_aSegCandGrp[pTrie->m_bSegCandGrpCount - 1];


    /* Now, pTrie->m_bSegCandGrpCount > 0 */
    iCountExact = GetCandidateCount(&pGrp->m_arrExactMatch);
    iCountPartial = GetCandidateCount(&pGrp->m_arrPartialMatch);

    if (iCountExact == 0)
    {
        pSeg = GetCandidate( &pGrp->m_arrPartialMatch, i );
        eType = ePartialMatch;
    }
    else if (iCountPartial == 0)
    {
        pSeg = GetCandidate( &pGrp->m_arrExactMatch, i );
        eType = eExactMatch;
    }
    else
    {
        const ET9_CS_Candidate* pSegX = GetCandidate( &pGrp->m_arrExactMatch, 0 );
        const ET9_CS_Candidate* pSegP = GetCandidate( &pGrp->m_arrPartialMatch, 0 );
        if ( pSegP->m_iSegProb - ET9_CS_PARTIAL_EXACT_THRESHOLD >= pSegX->m_iSegProb )
        {
            pSeg = pSegP;
            eType = ePartialMatch;
        }
        else {
            pSeg = pSegX;
            eType = eExactMatch;
        }
    }

    ET9Assert(pSeg != NULL);
    pCandidate->m_bSegLen = pSeg->m_bSegLen;
    pCandidate->m_iSegProb = pSeg->m_iSegProb;
    _ET9ByteCopy( (ET9U8*)pCandidate->m_pcSeg, (ET9U8*)pSeg->m_pcSeg, pSeg->m_bSegLen );

    return eType;
}

ET9S32 ET9FARCALL ET9_CS_GetCandidateCount(const ET9_CP_SSBITrie* pTrie)
{
    ET9S32 iCountExact, iCountPartial, iCount;
    const ET9_CS_CandidateGroup * pGrp = NULL;

    ET9Assert( pTrie != NULL );
    if ( pTrie->m_bSegCandGrpCount == 0 ) {
        return 0;
    }

    pGrp = &pTrie->m_aSegCandGrp[pTrie->m_bSegCandGrpCount - 1];
    iCountExact = GetCandidateCount(&pGrp->m_arrExactMatch);
    if ( pGrp->m_eMatchType == eExactMatch)
    {
        return iCountExact;
    }

    iCountPartial = GetCandidateCount(&pGrp->m_arrPartialMatch);
    iCount = iCountPartial + iCountExact;
    if ( iCount > pGrp->m_bCandBufferSize )
        iCount = pGrp->m_bCandBufferSize;
    return iCount;
}

static ET9BOOL ET9LOCALCALL CheckValidity(const ET9_CP_SSBITrie* pTrie)
{
    ET9U8 i;
    ET9BOOL fPrevIsDelim = 1;
    ET9U8 bUnselectedStart = ET9_CP_SelectionHistUnselectedStart(&pTrie->pET9CPLingInfo->SelHistory);
    const ET9WordSymbInfo * pWSI = ET9_CS_WORDSYMBOL_INFO(pTrie);
    if ( pWSI == NULL )
        return 0;
    for ( i = bUnselectedStart; i < pWSI->bNumSymbs; i++ ) {
        if ( ET9_CP_SymbIsToneOrDelim(&pWSI->SymbsInfo[i]) )
        {
            if ( fPrevIsDelim )
                return 0;  /* First symbol is delimiter/Tone OR two adjacent symbols are both delimiter/Tone */
            fPrevIsDelim = 1;
        }
        else
            fPrevIsDelim = 0;
    }
    return 1;
}

ET9_CP_SpellMatch ET9FARCALL ET9_CS_BuildCandidates( ET9_CP_SSBITrie* pTrie )
{
    ET9U32 nUnchanged, bLastSegCandGrpIndex;
    ET9_CP_SpellMatch eType;
    const ET9WordSymbInfo *pWordSymbInfo;
    ET9U8   bUnselectedStart;

    ET9Assert( pTrie != NULL );
    if (pTrie->wInitOK != ET9GOODSETUP) {
        return eNoMatch;
    }

    pWordSymbInfo = ET9_CS_WORDSYMBOL_INFO(pTrie);
    ET9Assert( pWordSymbInfo != NULL );

    if ( pWordSymbInfo->bNumSymbs > ET9_CP_MAX_SBI_KEY_NUMBER ) {
        return eNoMatch;
    }

    if ( !CheckValidity(pTrie) )  {
        return eNoMatch;
    }

    bUnselectedStart = ET9_CP_SelectionHistUnselectedStart(&pTrie->pET9CPLingInfo->SelHistory);
    nUnchanged = ET9_CP_WSIValidLen(pTrie->pET9CPLingInfo, pWordSymbInfo);
    if ( (ET9U32)(bUnselectedStart + pTrie->m_bSegCandGrpCount) < nUnchanged )  { /* pWordSymbInfo and pTrie is out of sync, rebuild  */
        eType = Trie_RebuildCandidates( pTrie, 1 );
        return eType;
    }

    /* We should have nUnchanged > bUnselectedStart if go through API */
    if ( nUnchanged < (ET9U32)(bUnselectedStart + pTrie->m_bSBIConditionLen) )  {
        pTrie->m_bSBIConditionLen = 0;
        nUnchanged = bUnselectedStart;
    }

    pTrie->m_bSegCandGrpCount = (ET9U8)(nUnchanged - bUnselectedStart);
    bLastSegCandGrpIndex = pTrie->m_bSegCandGrpCount;
    Trie_AddSymbols( pTrie, pWordSymbInfo, (ET9U8)nUnchanged, pWordSymbInfo->bNumSymbs );

    eType = SegCandGrpType( pTrie->m_aSegCandGrp + (pTrie->m_bSegCandGrpCount - 1) );
    return eType;
}

/*  pcSelect contains no DELEMITER, '_'  */
/* return ET9STATUS_NONE:          Ok
          ET9STATUS_FULL:  SelectionHistory full,
          ET9STATUS_INVALID_INPUT: pcSelect is invalid
          ET9STATUS_NO_MATCH:      pWSI is invalid */
ET9STATUS ET9FARCALL ET9_CS_SelectSegment(ET9_CP_SSBITrie* pTrie,
                                         const ET9SYMB * psPhrase,
                                         ET9U8 bPraseLen,
                                         const ET9U8 * pcSpell,
                                         ET9U8 bSpellLen )
{
    ET9STATUS status;
    ET9U8  bConsumed;
    ET9S32 iCopied;
    const ET9SymbInfo *pSymbInfo;
    ET9U8   bUnselectedStart, bUnselectedLength;
    ET9U8   acSelSpell[ET9_CP_MAX_CANDIDATE_LENGTH];
    ET9CPPhrase phraseUnicode;

    ET9Assert( pTrie != NULL && pcSpell != NULL );

    bUnselectedStart = ET9_CP_SelectionHistUnselectedStart(&pTrie->pET9CPLingInfo->SelHistory);
    bUnselectedLength = (ET9U8)(ET9_CS_WORDSYMBOL_INFO(pTrie)->bNumSymbs - bUnselectedStart);

    pSymbInfo = ET9_CS_WORDSYMBOL_INFO(pTrie)->SymbsInfo + bUnselectedStart;

    if ( (ET9_CP_StrIsPrefixOf( (const ET9U8 *)pcSpell, bSpellLen, pSymbInfo, pTrie->m_bSegCandGrpCount) & ePrefix1) == 0 ) {
        return ET9STATUS_INVALID_INPUT;
    }

    bConsumed = (ET9U8)ET9_CP_CopyAddDelimiter((ET9U8*)acSelSpell, sizeof(acSelSpell) - 1, &iCopied, (ET9U8*)pcSpell, bSpellLen, pSymbInfo, bUnselectedLength);
    if ( bConsumed < bUnselectedLength && ET9_CP_SymbIsToneOrDelim(&pSymbInfo[bConsumed]) ) {
        acSelSpell[iCopied++] = ET9_CP_GetSymbToneOrDelim(&pSymbInfo[bConsumed]);
        bConsumed++;
    }

    _ET9SymCopy(phraseUnicode.pSymbs, (ET9SYMB *)psPhrase, bPraseLen);
    phraseUnicode.bLen = bPraseLen;
    ET9_CP_ConvertPhraseToUnicode(pTrie->pET9CPLingInfo, &phraseUnicode, ET9_CP_IDEncode_PID);

    if (ET9_CP_PhraseIsAllChn(pTrie->pET9CPLingInfo, psPhrase, bPraseLen)) {
        status = ET9_CP_SelectionHistAdd(&pTrie->pET9CPLingInfo->SelHistory, phraseUnicode.pSymbs, psPhrase, bPraseLen, bConsumed);
    }
    else {
        /*  NULL in the phraseEncoded position means do not add to ContextBuf */
        status = ET9_CP_SelectionHistAdd(&pTrie->pET9CPLingInfo->SelHistory, phraseUnicode.pSymbs, NULL, bPraseLen, bConsumed);
    }
    if (status != ET9STATUS_NONE) {
        return status;
    }

    pTrie->m_bSBIConditionLen = 0;
    pTrie->m_bSegCandGrpCount = 0;
    return ET9STATUS_NONE;
}

ET9S32 ET9FARCALL ET9_CS_GetPrefixCount(ET9_CP_SSBITrie* pTrie)
{
    ET9Assert( pTrie != NULL );
    if (pTrie->wInitOK != ET9GOODSETUP) {
        return 0;
    }
    return (ET9S32)pTrie->m_PrefixBuffer.m_bPrefixCount;
}

/* return ET9STATUS_NONE:          Ok
          ET9STATUS_NO_INIT:       Not init'ed
          ET9STATUS_OUT_OF_RANGE:  iIndex is invalid ( probably pTrie->m_PrefixBuffer.m_bPrefixCount == 0 ) */
ET9STATUS ET9FARCALL ET9_CS_GetPrefix(const ET9_CP_SSBITrie* pTrie, ET9S32 iIndex, ET9_CS_Prefix* pPrefix )
{
    ET9S32 n = iIndex;
    const ET9U8* pc;
    ET9Assert( pTrie != NULL && pPrefix != NULL );
    if (pTrie->wInitOK != ET9GOODSETUP) {
        return ET9STATUS_NO_INIT;
    }
    if ( iIndex >= (ET9S32)pTrie->m_PrefixBuffer.m_bPrefixCount ) {
        return ET9STATUS_OUT_OF_RANGE;
    }

    pc = pTrie->m_PrefixBuffer.m_acPrefixBuf;
    while ( n ) {
        ET9U8 bLen = *pc;
        pc += (ET9_CS_HEADER_SIZE_SBI_PREFIX + bLen);
        n--;
    }

    pPrefix->m_bPfxLen = read_u8( pc );
    pc += sizeof(ET9U8);
    pPrefix->m_iPfxProb = (ET9S32)ET9_CP_ReadU32( pc );
    pc += sizeof(ET9S32);
    _ET9ByteCopy( (ET9U8*)pPrefix->m_pcPfx, (ET9U8*)pc, (ET9U32)pPrefix->m_bPfxLen );
    return ET9STATUS_NONE;
}

static ET9STATUS ET9LOCALCALL SBI_BuildCandAndPrefix(ET9CPLingInfo * pET9CPLingInfo)
{
    ET9_CP_SpellMatch eSBIType = ET9_CS_BuildCandidates(&pET9CPLingInfo->SBI);
    if ( eSBIType != eNoMatch )
    {
        /* Set Spelling */
        ET9_CP_SegmentationToSpell(pET9CPLingInfo, &pET9CPLingInfo->CommonInfo.sActiveSpell);

        /* Build Prefix */
        ET9_CS_ClearPrefixBuf(&pET9CPLingInfo->SBI);
        ET9_CS_SetValidPfxLen(&pET9CPLingInfo->SBI);
        ET9_CS_PhrasalPrefix(&pET9CPLingInfo->SBI);
        ET9_CP_SortPrefixGrp(pET9CPLingInfo);
        return ET9STATUS_NONE;
    }
    pET9CPLingInfo->CommonInfo.sActiveSpell.bLen = 0; /* clear active spell */
    return ET9STATUS_INVALID_INPUT;
}

ET9STATUS ET9FARCALL ET9_CP_BuildSBISpellings(ET9CPLingInfo * const pLing)
{
    ET9STATUS status = ET9STATUS_INVALID_INPUT;
    ET9WordSymbInfo *pWSI = pLing->Base.pWordSymbInfo;
    ET9U8   bUnselectedStart, bUnselectedLength;
    ET9BOOL fValidInput;

    ET9Assert(ET9CPIsModePhonetic(pLing));

    if ( pWSI->bNumSymbs > ET9_CP_MAX_SBI_KEY_NUMBER )
    {
        return ET9STATUS_INVALID_INPUT;
    }

    bUnselectedStart = ET9_CP_SelectionHistUnselectedStart(&pLing->SelHistory);
    bUnselectedLength = (ET9U8)(pWSI->bNumSymbs - bUnselectedStart);
    fValidInput = ET9_CP_IsValidSBIInput(&pWSI->SymbsInfo[bUnselectedStart], bUnselectedLength);
    if (fValidInput && ET9_CP_IsValidWSITone(pWSI, &pLing->Private.PPrivate.bInputHasTone) ) {
        status = SBI_BuildCandAndPrefix(pLing); /* assume invalid input, change the status if build succeeds */
    }
    ET9_CP_ClearPhraseBuf(ET9_CP_GetMainPhraseBuf(pLing));
    return status;
}

static ET9STATUS ET9LOCALCALL ET9_CS_AddUdbPrefix(ET9CPLingInfo * pET9CPLingInfo, const ET9SymbInfo *pSymbInfo, ET9S32 iLen)
{
    ET9_CP_RUdbObj sObj;

    ET9CPUdbInfo ET9FARDATA * pUdb;
    ET9U8 ET9FARDATA *pbStart;
    ET9U8 ET9FARDATA *pbEnd;
    ET9U8 ET9FARDATA *pbCurrent;
    ET9U8 acPinyin[ET9CPMAXSPELLSIZE + 3] = {0,0,0};
    ET9U8 * pcPinyin = acPinyin + 3;
    ET9U8 cFirstLetter, i, bZoneCount;
    ET9U16 wMohu;
    ET9BOOL bTradPinyin = 0;
    ET9BOOL bIsBpmf;

    ET9_CP_CHECK_LINGINFO(pET9CPLingInfo);

    pUdb = pET9CPLingInfo->pUdb; /* abbreviate */

    if (!pUdb)
        return ET9STATUS_NO_RUDB;

    wMohu = ET9_CP_GetMohuFlags(pET9CPLingInfo);
    bIsBpmf = ET9CPIsModeBpmf(pET9CPLingInfo);

    if (ET9CPGetChineseLdbNum(pET9CPLingInfo) == ET9PLIDChineseSimplified) {
        cFirstLetter = ET9_CP_FIRSTUPLETTER; /* 'A' */
        bZoneCount = ET9_CP_UDBPINYINZONES;  /* 26 */
    }
    else {/* ET9PLIDChineseTraditional || ET9PLIDChineseHongkong) */
        cFirstLetter = ET9_CP_BPMFFIRSTLOWERLETTER; /* Lower case Bo */
        bZoneCount = ET9_CP_UDBBPMFZONES;    /* 37 */
        if (ET9CPIsModePinyin(pET9CPLingInfo) ) {
            bTradPinyin = 1;
        }
    }

    for ( i = 0; i < bZoneCount; i++ )
    {
        if (bTradPinyin) {
            if ( !ET9_CP_CharSymbMohuMatch(wMohu, ET9_CP_Bpmf_Letter_To_Pinyin[i], pSymbInfo) ) {
                continue;
            }
        }
        else if ( !ET9_CP_CharSymbMohuMatch(wMohu, (ET9U8)(cFirstLetter + i), pSymbInfo) ) {
            continue;
        }

        pbStart = ET9_CP_UdbOffsetToPtr(pUdb, pUdb->wZoneOffset[i]);
        pbEnd = ET9_CP_UdbOffsetToPtr(pUdb, pUdb->wZoneOffset[i+1]);

        pbCurrent = pbStart;
        while ( pbCurrent < pbEnd )
        {
            ET9_CP_GetEntryInfo(pUdb, pbCurrent, &sObj, ET9_CP_GET_TYPE | ET9_CP_GET_ENTRY_SIZE | ET9_CP_GET_ID | ET9_CP_GET_FREQ /* | ET9_CP_GET_CHECKSUM*/ );

            ET9Assert( sObj.eType == ET9_CP_RDBPHONETIC || sObj.eType == ET9_CP_UDBPHONETIC || sObj.eType == ET9_CP_AUDBPHONETIC || sObj.eType == ET9_CP_FREE);

            if (sObj.eType != ET9_CP_FREE) {
                ET9U8 acConsumed[ET9_CP_MAX_SBI_KEY_NUMBER + 1];
                ET9U8 bWordSize = ET9_CP_EntrySizeToWordSize(sObj.wEntrySize);
                ET9U8 i, len, bSylLen;
                ET9U8 acNodeStack[ET9CPMAXSPELLSIZE] = {0};
                ET9U8 acMatched[ET9CPMAXSPELLSIZE] = {0};
                ET9S32 iProb = (sObj.eType == ET9_CP_RDBPHONETIC)? (sObj.wFreq): (sObj.wFreq | 0x1000);

                ET9U16 wFlags;

                len = 0;
                for (i = 0; i < bWordSize; i++ )
                {
                    bSylLen = 0;
                    ET9_CP_PidBidToSyllable(pET9CPLingInfo, sObj.pwID[i], pcPinyin + len, &bSylLen, bIsBpmf);
                    len = (ET9U8)(len + bSylLen);
                }
                pcPinyin[len] = 0;  /* 0-termination */

                wFlags = (ET9_CS_BIT_CHECK_CONDITION | ET9_CS_BIT_PREFIX_SEARCH);
                if ( ET9_CS_IsPartialSpellActive(pET9CPLingInfo) )
                    wFlags |= ET9_CS_BIT_PARTIAL_PINYIN_ENABLED;
                if ( pET9CPLingInfo->SBI.nTrieType == ET9_CS_BPMF_TRIE )
                    wFlags |= ET9_CS_BIT_BPMF;
                StringSymbInfoMatch(&pET9CPLingInfo->SBI, wFlags, wMohu, /* iNodeOffset */0, iProb, pcPinyin, len, 0, pSymbInfo, iLen, acNodeStack, acMatched, /*iStackIdx*/0, acConsumed, /*iConsumedIdx*/0, NULL, /*fPartialPinyin*/0);
            }
            /* move to next entry */
            pbCurrent = ET9_CP_MoveUdbPR(pUdb, pbCurrent, sObj.wEntrySize);
        }
    }
    return ET9STATUS_NONE;
}


static ET9STATUS ET9LOCALCALL ET9_CP_GetUdbSpellings(ET9CPLingInfo * pET9CPLingInfo, const ET9SymbInfo *pSymbInfo, ET9S32 iLen, ET9_CS_CandidateGroup* pCandGrp)
{
    ET9_CP_RUdbObj sObj;

    ET9CPUdbInfo ET9FARDATA * pUdb;
    ET9U8 ET9FARDATA *pbStart;
    ET9U8 ET9FARDATA *pbEnd;
    ET9U8 ET9FARDATA *pbCurrent;
    ET9U8 acPinyin[ET9CPMAXSPELLSIZE + 3] = {0,0,0};
    ET9U8 * pcPinyin = acPinyin + 3;
    ET9U8 cFirstLetter, i, bZoneCount;
    ET9U16 wMohu;
    ET9BOOL bTradPinyin = 0;
    ET9BOOL bIsBpmf;

    SegCandGrpClear(pCandGrp);
    ET9_CP_CHECK_LINGINFO(pET9CPLingInfo);

    pUdb = pET9CPLingInfo->pUdb; /* abbreviate */

    if (!pUdb)
        return ET9STATUS_NO_RUDB;

    wMohu = ET9_CP_GetMohuFlags(pET9CPLingInfo);
    bIsBpmf = ET9CPIsModeBpmf(pET9CPLingInfo);

    if (ET9CPGetChineseLdbNum(pET9CPLingInfo) == ET9PLIDChineseSimplified) {
        cFirstLetter = ET9_CP_FIRSTUPLETTER; /* 'A' */
        bZoneCount = ET9_CP_UDBPINYINZONES;  /* 26 */
    }
    else {/* ET9PLIDChineseTraditional || ET9PLIDChineseHongkong) */
        cFirstLetter = ET9_CP_BPMFFIRSTLOWERLETTER; /* Lower case Bo */
        bZoneCount = ET9_CP_UDBBPMFZONES;    /* 37 */
        if (ET9CPIsModePinyin(pET9CPLingInfo) ) {
            bTradPinyin = 1;
        }
    }

    for ( i = 0; i < bZoneCount; i++ )
    {
        if (bTradPinyin) {
            if ( !ET9_CP_CharSymbMohuMatch(wMohu, ET9_CP_Bpmf_Letter_To_Pinyin[i], pSymbInfo) ) {
                continue;
            }
        }
        else if ( !ET9_CP_CharSymbMohuMatch(wMohu, (ET9U8)(cFirstLetter + i), pSymbInfo) ) {
            continue;
        }

        pbStart = ET9_CP_UdbOffsetToPtr(pUdb, pUdb->wZoneOffset[i]);
        pbEnd = ET9_CP_UdbOffsetToPtr(pUdb, pUdb->wZoneOffset[i+1]);

        pbCurrent = pbStart;
        while ( pbCurrent < pbEnd )
        {
            ET9_CP_GetEntryInfo(pUdb, pbCurrent, &sObj, ET9_CP_GET_TYPE | ET9_CP_GET_ENTRY_SIZE | ET9_CP_GET_ID | ET9_CP_GET_FREQ /* | ET9_CP_GET_CHECKSUM*/ );

            ET9Assert( sObj.eType == ET9_CP_RDBPHONETIC || sObj.eType == ET9_CP_UDBPHONETIC || sObj.eType == ET9_CP_AUDBPHONETIC || sObj.eType == ET9_CP_FREE);

            if (sObj.eType != ET9_CP_FREE) {
                ET9U8 acConsumed[ET9_CP_MAX_SBI_KEY_NUMBER + 1];
                ET9U8 bWordSize = ET9_CP_EntrySizeToWordSize(sObj.wEntrySize);
                ET9U8 j, len, bSylLen;
                ET9U8 acNodeStack[ET9CPMAXSPELLSIZE] = {0};
                ET9U8 acMatched[ET9CPMAXSPELLSIZE] = {0};
                ET9S32 iProb = /*(sObj.eType == ET9_CP_RDBPHONETIC)? (sObj.wFreq):*/ ((sObj.wFreq >> 3) | 0x1000);

                ET9U16 wFlags;

                len = 0;
                for (j = 0; j < bWordSize; j++ )
                {
                    bSylLen = 0;
                    ET9_CP_PidBidToSyllable(pET9CPLingInfo, sObj.pwID[j], pcPinyin + len, &bSylLen, bIsBpmf);
                    len = (ET9U8)(len + bSylLen);
                }
                pcPinyin[len] = 0;   /* 0-termination */

                wFlags = ET9_CS_BIT_CHECK_CONDITION;
                if ( ET9_CS_IsPartialSpellActive(pET9CPLingInfo) )
                    wFlags |= ET9_CS_BIT_PARTIAL_PINYIN_ENABLED;
                if ( pET9CPLingInfo->SBI.nTrieType == ET9_CS_BPMF_TRIE )
                    wFlags |= ET9_CS_BIT_BPMF;
                StringSymbInfoMatch(&pET9CPLingInfo->SBI, wFlags, wMohu, /* iNodeOffset */0, iProb, pcPinyin, len, 0, pSymbInfo, iLen, acNodeStack, acMatched, /*iStackIdx*/0, acConsumed, /*iConsumedIdx*/0, pCandGrp, /*fPartialPinyin*/0);
            }
            /* move to next entry */
            pbCurrent = ET9_CP_MoveUdbPR(pUdb, pbCurrent, sObj.wEntrySize);
        }
    }
    return ET9STATUS_NONE;
}

ET9U16 ET9FARCALL ET9_CP_SBI_ScorePhrase(ET9_CP_SpellData * psSpellData,
                                         ET9_CP_CommonInfo * pCommon,
                                         ET9SYMB * psPhrase,
                                         ET9U8 bPhraseLen,
                                         ET9U16 wFreq,
                                         ET9U8 bIsFromContext,
                                         ET9U8 bIsFromUdb,
                                         ET9UINT * pfSuppress)
{
    /* score SBI phrase */

    ET9_CP_SBISpellData * psSBISpellData = &psSpellData->u.sSBI;
    ET9_CP_Spell * psSpell = &psSpellData->sSpell;

    ET9INT iFreq = wFreq;

    /* which kinds of completion were used */
    ET9BOOL fInitialExpansion, fSyllableCompletion, fPhraseCompletion;

    ET9_CP_MatchType(pCommon, psPhrase, bPhraseLen,
        psSBISpellData->fEndsWithInitial,
        &fInitialExpansion,
        &fSyllableCompletion,
        &fPhraseCompletion);

    /* filter partial when not searching segmentation length */
    if (fSyllableCompletion && !psSBISpellData->fSearchingLastSegment) {
        ET9Assert(pfSuppress);
        *pfSuppress = 1;
        return 0;
    }

    if (psSBISpellData->fSearchingSegment) {
        psSBISpellData->fSegmentFull = (ET9BOOL)(psSBISpellData->fSegmentFull || (!fSyllableCompletion && !fInitialExpansion) );
    }

    if (bIsFromUdb) {
        iFreq = iFreq >> 3;
        iFreq += 0x100;
    }

    /* put exact phrases well above syllable completion phrases */
    if (!fSyllableCompletion) {
        /* do not put single char phrases that did not pass the spelling filter into exact bucket */
        if (psSBISpellData->fSearchingSetPrefix ||
            psSBISpellData->fSearchingLastSegment ||
            psSBISpellData->fSearchingSegmentLen ||
            psSBISpellData->fSearchingSegment1stSylLen ||
            psSBISpellData->fPrefixSyllablesAligned)
        {
            iFreq = iFreq | ET9_CP_FREQ_MASK_EXACT;
        }
        ET9Assert((iFreq & ET9_CP_FREQ_MASK_EXACT) || 1 == psSBISpellData->nSyllableCount);
    }

    if (bIsFromContext) {
        iFreq += 0x60;
    }

    if (psSBISpellData->fSearchingLastSegment) {
        iFreq = iFreq + 0x15 * (4 * psSpell->bLen);
    }
    else {
        if (psSBISpellData->fSearchingSegment) {
            iFreq += 0x30;
        }
        else if (psSBISpellData->fSearchingSegmentLen) {
            iFreq += 0x20;
        }
    }

    if (fInitialExpansion) {
        if (1 == psSBISpellData->nSyllableCount) {
            if (!(psSBISpellData->fSearchingSetPrefix || psSBISpellData->fSearchingSegmentLen)) {
                if (wFreq >= 0x90 && psSBISpellData->fSearchingSegment1stSylLen)
                {
                    iFreq = wFreq;
                }
                else 
                {
                    ET9Assert(pfSuppress);
                    *pfSuppress = 1;
                    return 0;
                }
            }
        }
        {
            ET9INT iPartialPenalty = 0x20 * (1 + psSBISpellData->nFirstPartialPinyin);
            iFreq = iFreq - iPartialPenalty;
        }
    }
    iFreq = (iFreq + 0x10 * psSpell->bLen);

    if (fInitialExpansion &&
        psSBISpellData->fSegmentFull &&
        (psSBISpellData->wSegPhraseFreq > (ET9U16)__ET9Pin(0, iFreq, 0xFFFF) ||
        !psSBISpellData->fSearchingLastSegment))
    {
        ET9Assert(pfSuppress);
        *pfSuppress = 1;
        return 0;
    }

    /* demote long phrases when input has only 1 or 2 syllables */
    {
        ET9UINT nSylCount = pCommon->bSylCount;
        if (0 < nSylCount && nSylCount <= 2 && bPhraseLen > nSylCount) {
            iFreq = (wFreq | 0x100);
        }
    }

    return (ET9U16)__ET9Pin(0, iFreq, 0xFFFF);
}

/* ----------------------------------< eof >--------------------------------- */
