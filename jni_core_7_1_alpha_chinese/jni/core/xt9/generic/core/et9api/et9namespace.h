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
;**     FileName:  et9namespace.h                                             **
;**                                                                           **
;**  Description:  Modifying namespace for ET9 generic                        **
;**                                                                           **
;**                                                                           **
;*******************************************************************************
;******************************************************************************/

/* Namespace modifier for ET9 generic */

#ifdef _WIN32
#pragma message ("*** Modifying namespace for ET9 generic ***")
#endif

#define                   _ET9ByteCopy                   _ET9ByteCopy_xx_
#define                    _ET9SymCopy                    _ET9SymCopy_xx_
#define                   _ET9ByteMove                   _ET9ByteMove_xx_
#define                    _ET9SymMove                    _ET9SymMove_xx_
#define                   _ET9ClearMem                   _ET9ClearMem_xx_
#define                    _ET9ByteSet                    _ET9ByteSet_xx_
#define                    _ET9WordSet                    _ET9WordSet_xx_
#define                _ET9BinaryToHex                _ET9BinaryToHex_xx_
#define      _ET9CheckFundamentalTypes      _ET9CheckFundamentalTypes_xx_
#define              ET9GetSymbolClass              ET9GetSymbolClass_xx_
#define                 _ET9SymIsUpper                 _ET9SymIsUpper_xx_
#define                 _ET9SymToLower                 _ET9SymToLower_xx_
#define                 _ET9SymToUpper                 _ET9SymToUpper_xx_
#define                 _ET9SymToOther                 _ET9SymToOther_xx_
#define  _ET9_GetFullSymbolKeyAndClass  _ET9_GetFullSymbolKeyAndClass_xx_
#define                 _ET9_IsUnknown                 _ET9_IsUnknown_xx_
#define              _ET9_IsWhiteSpace              _ET9_IsWhiteSpace_xx_
#define               _ET9_IsPunctChar               _ET9_IsPunctChar_xx_
#define                 _ET9_IsNumeric                 _ET9_IsNumeric_xx_
#define          _ET9_IsPunctOrNumeric          _ET9_IsPunctOrNumeric_xx_
#define           _ET9_IsNumericString           _ET9_IsNumericString_xx_
#define       _ET9FindSpacesAndUnknown       _ET9FindSpacesAndUnknown_xx_
#define       _ET9StringLikelyEmoticon       _ET9StringLikelyEmoticon_xx_
#define                  ET9SymToLower                  ET9SymToLower_xx_
#define                  ET9SymToUpper                  ET9SymToUpper_xx_
#define                  ET9SymToOther                  ET9SymToOther_xx_
#define                  ET9SymIsUpper                  ET9SymIsUpper_xx_
#define                  ET9SymIsLower                  ET9SymIsLower_xx_
#define          ET9_GetSymbolEncoding          ET9_GetSymbolEncoding_xx_
#define              ET9GetCodeVersion              ET9GetCodeVersion_xx_
#define                ET9ClearOneSymb                ET9ClearOneSymb_xx_
#define               ET9ClearAllSymbs               ET9ClearAllSymbs_xx_
#define                 ET9DeleteSymbs                 ET9DeleteSymbs_xx_
#define                   ET9MoveSymbs                   ET9MoveSymbs_xx_
#define             ET9AddExplicitSymb             ET9AddExplicitSymb_xx_
#define          ET9AddCustomSymbolSet          ET9AddCustomSymbolSet_xx_
#define                   _ET9LockWord                   _ET9LockWord_xx_
#define                    ET9LockWord                    ET9LockWord_xx_
#define                ET9GetExactWord                ET9GetExactWord_xx_
#define         ET9GetAutoCapSituation         ET9GetAutoCapSituation_xx_
#define               _ET9ImminentSymb               _ET9ImminentSymb_xx_
#define          _ET9InvalidateOneSymb          _ET9InvalidateOneSymb_xx_
#define            _ET9ValidateOneSymb            _ET9ValidateOneSymb_xx_
#define          _ET9ValidateOneSymbAW          _ET9ValidateOneSymbAW_xx_
#define          _ET9InvalidateOneLock          _ET9InvalidateOneLock_xx_
#define         _ET9InvalidateSymbInfo         _ET9InvalidateSymbInfo_xx_
#define          _ET9InvalidateSelList          _ET9InvalidateSelList_xx_
#define               _ET9WordSymbInit               _ET9WordSymbInit_xx_
#define           _ET9IsMagicStringKey           _ET9IsMagicStringKey_xx_
#define                   _ET9SaveWord                   _ET9SaveWord_xx_
#define              _ET9ExplicifyWord              _ET9ExplicifyWord_xx_
#define              ET9SetNextLocking              ET9SetNextLocking_xx_
#define            ET9ClearNextLocking            ET9ClearNextLocking_xx_
#define               ET9KDB_SetKdbNum               ET9KDB_SetKdbNum_xx_
#define              ET9KDB_SetPageNum              ET9KDB_SetPageNum_xx_
#define              ET9KDB_GetPageNum              ET9KDB_GetPageNum_xx_
#define                    ET9KDB_Init                    ET9KDB_Init_xx_
#define                ET9KDB_Validate                ET9KDB_Validate_xx_
#define           ET9KDB_GetKdbVersion           ET9KDB_GetKdbVersion_xx_
#define                  ET9SetUnShift                  ET9SetUnShift_xx_
#define                    ET9SetShift                    ET9SetShift_xx_
#define                 ET9SetCapsLock                 ET9SetCapsLock_xx_
#define       ET9KDB_FindSubRegion4Tap       ET9KDB_FindSubRegion4Tap_xx_
#define              ET9KDB_ProcessTap              ET9KDB_ProcessTap_xx_
#define         ET9KDB_GetTapKeyRegion         ET9KDB_GetTapKeyRegion_xx_
#define               ET9KDB_GetKdbNum               ET9KDB_GetKdbNum_xx_
#define             _ET9KDB_FindSymbol             _ET9KDB_FindSymbol_xx_
#define      ET9KDB_ProcessKeyBySymbol      ET9KDB_ProcessKeyBySymbol_xx_
#define              ET9KDB_ProcessKey              ET9KDB_ProcessKey_xx_
#define                 ET9KDB_TimeOut                 ET9KDB_TimeOut_xx_
#define            ET9KDB_SetAmbigMode            ET9KDB_SetAmbigMode_xx_
#define         ET9KDB_SetMultiTapMode         ET9KDB_SetMultiTapMode_xx_
#define            ET9KDB_ReselectWord            ET9KDB_ReselectWord_xx_
#define           ET9KDB_NextDiacritic           ET9KDB_NextDiacritic_xx_
#define         ET9KDB_SetRegionalMode         ET9KDB_SetRegionalMode_xx_
#define         ET9KDB_SetDiscreteMode         ET9KDB_SetDiscreteMode_xx_
#define          ET9KDB_SetConvertSymb          ET9KDB_SetConvertSymb_xx_
#define     ET9KDB_GetMultiTapSequence     ET9KDB_GetMultiTapSequence_xx_
#define            ET9SymbolEncodingQA            ET9SymbolEncodingQA_xx_
#define                ET9WordSymbInit                ET9WordSymbInit_xx_
