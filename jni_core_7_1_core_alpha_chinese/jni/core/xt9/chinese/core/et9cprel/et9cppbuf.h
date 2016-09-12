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
;**     FileName: et9cppbuf.h                                                 **
;**                                                                           **
;**  Description: Chinese XT9 phrase buffer read/save module header file.     **
;**               Conforming to the development version of Chinese XT9.       **
;**                                                                           **
;*******************************************************************************
;******************************************************************************/

#ifndef ET9CPPBUF_H
#define ET9CPPBUF_H 1

/* don't mangle the function name if compile under C++ */
#if defined(__cplusplus)
extern "C" {
#endif

/* phrase buffer section (PB) */
#define ET9_CP_PB_FREE_BYTES(pbPB)              ((pbPB)->wBufSize - (pbPB)->wUsedBufSize)

/* phrase buffer record section (PBR) */
#define ET9_CP_PBR_RECORD_LENGTH_SIZE           sizeof(ET9U8)
#define ET9_CP_PBR_PHRASE_LENGTH_SIZE           sizeof(ET9U8)
#define ET9_CP_PBR_SPELL_LENGTH_SIZE            sizeof(ET9U8)

#define ET9_CP_PBR_RECORD_LENGTH_OFFSET         0
#define ET9_CP_PBR_PHRASE_LENGTH_OFFSET         ET9_CP_PBR_RECORD_LENGTH_OFFSET + ET9_CP_PBR_RECORD_LENGTH_SIZE
#define ET9_CP_PBR_SPELL_LENGTH_OFFSET          ET9_CP_PBR_PHRASE_LENGTH_OFFSET + ET9_CP_PBR_PHRASE_LENGTH_SIZE
#define ET9_CP_PBR_HEADER_SIZE                  ET9_CP_PBR_SPELL_LENGTH_OFFSET + ET9_CP_PBR_SPELL_LENGTH_SIZE
#define ET9_CP_PBR_PHRASE_OFFSET                ET9_CP_PBR_HEADER_SIZE

#define ET9_CP_PBR_GET_RECORD_LENGTH(pbPBR)     ((pbPBR)[ET9_CP_PBR_RECORD_LENGTH_OFFSET])
#define ET9_CP_PBR_GET_PHRASE_LENGTH(pbPBR)     ((pbPBR)[ET9_CP_PBR_PHRASE_LENGTH_OFFSET])
#define ET9_CP_PBR_GET_SPELL_LENGTH(pbPBR)      ((pbPBR)[ET9_CP_PBR_SPELL_LENGTH_OFFSET])
#define ET9_CP_PBR_GET_SPELL_OFFSET(pbPBR)      ((pbPBR) + ET9_CP_PBR_PHRASE_OFFSET + \
                                                 sizeof(ET9SYMB) * ET9_CP_PBR_GET_PHRASE_LENGTH((pbPBR)))
#define ET9_CP_PBR_GET_NEXT(pbPBR)              ((pbPBR) + ET9_CP_PBR_GET_RECORD_LENGTH((pbPBR)))

#define ET9_CP_PBR_READ_PHRASE(pbPBR, pwDstPhrase) \
    (_ET9ByteCopy((ET9U8*)(pwDstPhrase), \
                  (pbPBR) + ET9_CP_PBR_PHRASE_OFFSET, \
                  sizeof(ET9SYMB) * ET9_CP_PBR_GET_PHRASE_LENGTH((pbPBR))))

#define ET9_CP_PBR_READ_SPELL(pbPBR, pbDstSpell) \
    (_ET9ByteCopy(pbDstSpell, \
                  ET9_CP_PBR_GET_SPELL_OFFSET((pbPBR)), \
                  ET9_CP_PBR_GET_SPELL_LENGTH((pbPBR))))

/* end PBR section */


#define ET9_CP_FREQ_MASK_EXACT    ((ET9U16)0x8000)
#define ET9_CP_FREQ_MASK_CONTEXT  ((ET9U16)0x4000)
#define ET9_CP_FREQ_MASK_UDB      ((ET9U16)0x2000)
#define ET9_CP_FREQ_MASK_NONMOHU  ((ET9U16)0x1000)
#define ET9_CP_FREQ_MASK_ALL      ((ET9U16)(ET9_CP_FREQ_MASK_EXACT | ET9_CP_FREQ_MASK_CONTEXT | ET9_CP_FREQ_MASK_UDB | ET9_CP_FREQ_MASK_NONMOHU))

#define ET9_CP_UEDITDEFLENFREQ    (0x0FFFFFFF)

#define ET9_CP_MAX_SMART_PUNCTS_PER_CHAR  4
#define ET9_CP_BITS_PER_SMART_PUNCT      4
#define ET9_CP_SMART_PUNCT_MASK          0x0f

/* smart punct ldb table sizes, in bytes */
#define ET9_CP_SMART_PUNCT_HEADER_SIZE       2
#define ET9_CP_SMART_PUNCT_ID_ENTRY_SIZE     2
#define ET9_CP_SMART_PUNCT_TAIL_ENTRY_SIZE   6

#define ET9_CP_MAX_SPELL_FREQ     ((ET9U16)(0x1000))
/* this is for encoding spell to a stream of bits used in spell buffer */
#define ET9_CP_NUM_BITS_PER_LETTER 3

#define ET9_CP_IsPhraseBufEmpty(pPhraseBuf) ((pPhraseBuf)->bTestClear || 0 == (pPhraseBuf)->wTotal)

/**
 * Callback to fill the phrase buffer depending on mode
 */
typedef ET9STATUS (ET9FARCALL *ET9_CP_PHRASE_SEARCH_CALLBACK)(
    ET9CPLingInfo *pET9CPLingInfo   /**< Pointer to Chinese linguistic info. */
);


/*----------------------------------------------------------------------------
 *  Define the internal phrase buffer save and read function prototypes.
 *----------------------------------------------------------------------------*/
#define ET9_CP_INIT_PHRASE_BUF(buf)  { ET9_CP_ZeroPhraseBuffer(&(buf).sPhraseBuf); (buf).sPhraseBuf.wBufSize = sizeof((buf).pbDataBuf); }

/*
    This function fills up the phrase buffer based on the PID ranges that are set in
    ET9CPLingInfo.  The phrases include context predictions.
 */
#define ET9_CP_GetMainPhraseBuf(pLing)   ((ET9_CP_PhraseBuf*)&((pLing)->CommonInfo.sStdPhraseBuf))

ET9UINT ET9FARCALL ET9_CP_PhraseBufEndsWithSingleChar(ET9_CP_PhraseBuf *pPhraseBuf);

void ET9FARCALL ET9_CP_FillPhraseBufOnRanges(
    ET9CPLingInfo *pLing,
    ET9BOOL bActIsJianpin,
    ET9U8 *pbTones);

#ifndef ET9CP_DISABLE_STROKE
ET9UINT ET9FARCALL ET9_CP_ExpandDelimiter(ET9CPLingInfo *pET9CPLingInfo);
#endif
void ET9FARCALL ET9_CP_ZeroPhraseBuffer(ET9_CP_PhraseBuf *psPhraseBuf);
void ET9FARCALL ET9_CP_ClearPhraseBuf(ET9_CP_PhraseBuf *pPhraseBuf);
void ET9FARCALL ET9_CP_RestorePhraseBuf(ET9_CP_PhraseBuf *pPhraseBuf);
void ET9FARCALL ET9_CP_SetPageSize(ET9_CP_PhraseBuf *psPhraseBuf, ET9UINT nNewPageSize);

ET9STATUS ET9FARCALL ET9_CP_AdjustPointerFillPhraseBuf(ET9CPLingInfo *pET9CPLingInfo, ET9U16 wPhraseIndex, ET9_CP_PHRASE_SEARCH_CALLBACK FctFill);
void ET9FARCALL ET9_CP_ConvertPhraseToUnicode(ET9CPLingInfo *pET9CPLingInfo, ET9CPPhrase *psPhrase, ET9_CP_IDEncode eEnc);

#define ET9_CP_PhraseBufPageFillDone(psPhraseBuf) (          \
    (psPhraseBuf)->wTotal > (psPhraseBuf)->wLastTotal &&    \
    0 == ((psPhraseBuf)->wTotal % (psPhraseBuf)->bPageSize) \
    )

#define ET9_CP_PhraseLastFreqInPage(psPhraseBuf) (((psPhraseBuf)->bTestClear || \
    !(psPhraseBuf)->wTotal || \
    ((psPhraseBuf)->wTotal == (psPhraseBuf)->wLastTotal) || \
    ((psPhraseBuf)->wTotal % (psPhraseBuf)->bPageSize))? \
         0:((psPhraseBuf)->pwFreq[(psPhraseBuf)->bPageSize - 1]))
#define ET9_CP_PhraseFreqGroupDiff(psPhraseBuf) ((ET9U16)(ET9_CP_IsPhraseBufEmpty(psPhraseBuf)?0xffff: \
    ((psPhraseBuf)->wPrevFreq & ET9_CP_FREQ_MASK_ALL) - \
    (ET9_CP_PhraseLastFreqInPage(psPhraseBuf) & ET9_CP_FREQ_MASK_ALL)))
/* use the following to decide if certain group of phrases need to be added to the phrase buffer */
/* need to search for a group if prev page & current page in diff group or in the same requested group */
#define ET9_CP_PhraseNeedExact(psPhraseBuf) ((ET9_CP_PhraseFreqGroupDiff(psPhraseBuf) >= ET9_CP_FREQ_MASK_EXACT) || \
    ((psPhraseBuf)->wPrevFreq & ET9_CP_FREQ_MASK_EXACT))
#define ET9_CP_PhraseNeedPartial(psPhraseBuf) ((ET9_CP_PhraseFreqGroupDiff(psPhraseBuf) >= ET9_CP_FREQ_MASK_EXACT) || \
    (~(psPhraseBuf)->wPrevFreq & ET9_CP_FREQ_MASK_EXACT))
#define ET9_CP_PhraseNeedContext(psPhraseBuf) ((ET9_CP_PhraseFreqGroupDiff(psPhraseBuf) >= ET9_CP_FREQ_MASK_CONTEXT) || \
    ((psPhraseBuf)->wPrevFreq & ET9_CP_FREQ_MASK_CONTEXT))
#define ET9_CP_PhraseNeedNonContext(psPhraseBuf) ((ET9_CP_PhraseFreqGroupDiff(psPhraseBuf) >= ET9_CP_FREQ_MASK_CONTEXT) || \
    (~(psPhraseBuf)->wPrevFreq & ET9_CP_FREQ_MASK_CONTEXT))
#define ET9_CP_PhraseNeedUdb(psPhraseBuf) ((ET9_CP_PhraseFreqGroupDiff(psPhraseBuf) >= ET9_CP_FREQ_MASK_UDB) || \
    ((psPhraseBuf)->wPrevFreq & ET9_CP_FREQ_MASK_UDB))
#define ET9_CP_PhraseNeedLdb(psPhraseBuf) ((ET9_CP_PhraseFreqGroupDiff(psPhraseBuf) >= ET9_CP_FREQ_MASK_UDB) || \
    (~(psPhraseBuf)->wPrevFreq & ET9_CP_FREQ_MASK_UDB))

/* This function is to get a phrase from the specified position, the index is 1-based */
void ET9FARCALL ET9_CP_GetPhraseFromBuf(ET9_CP_PhraseBuf *psPhraseBuf,
                                        ET9CPPhrase *psPhraseInfo,
                                        ET9_CP_Spell *pSpell,
                                        ET9UINT nIndex);

ET9U16 ET9FARCALL ET9_CP_EncodeFreq(ET9CPLingInfo *pET9CPLingInfo,
                                    ET9SYMB * psPhrase,
                                    ET9U8 bPhraseLen,
                                    ET9U16 wFreq,
                                    ET9U8 bIsExact,
                                    ET9U8 bIsFromContext,
                                    ET9U8 bIsFromUdb,
                                    ET9UINT * pfSuppress);
ET9UINT ET9FARCALL ET9_CP_AddPhraseToBuf(ET9CPLingInfo *pET9CPLingInfo,
                                         ET9_CP_PhraseBuf *psPhraseBuf,
                                         const ET9SYMB *pwPhrase,
                                         ET9U8 bPhraseLen,
                                         const ET9U8 *pbSpell,
                                         ET9U8 bSpellLen,
                                         ET9_CP_IDEncode eEnc,
                                         ET9U16 wFreq);

void ET9FARCALL ET9_CP_GetSmartPuncts(ET9CPLingInfo *pET9CPLingInfo);
void ET9FARCALL ET9_CP_GetCommonChar(ET9CPLingInfo *   pET9CPLingInfo);

/* End don't mangle the function name if compile under C++ */
#if defined(__cplusplus)
    }
#endif

#endif  /* #ifndef ET9CPPBUF_H */

/* ----------------------------------< eof >--------------------------------- */
