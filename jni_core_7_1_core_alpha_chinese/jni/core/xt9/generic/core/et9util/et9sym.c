/*******************************************************************************
;*******************************************************************************
;**                                                                           **
;**                  COPYRIGHT 1998-2011 NUANCE COMMUNICATIONS                **
;**                                                                           **
;**               NUANCE COMMUNICATIONS PROPRIETARY INFORMATION               **
;**                                                                           **
;**     This software is supplied under the terms of a license agreement      **
;**     or non-disclosure agreement with Nuance Communications and may not    **
;**     be copied or disclosed except in accordance with the terms of that    **
;**     agreement.                                                            **
;**                                                                           **
;*******************************************************************************
;**                                                                           **
;**     FileName: et9sym.c                                                    **
;**                                                                           **
;**  Description: T9 symbol information.                                      **
;**                                                                           **
;*******************************************************************************
;******* 10 ****** 20 ****** 30 ****** 40 ****** 50 ****** 60 ****** 70 *******/

/*! \addtogroup et9sym Symbol Handling for XT9
* Symbol handling for generic XT9.
* @{
*/

#include "et9api.h"
#include "et9sym.h"
#include "et9misc.h"


#ifdef ET9SYMBOLENCODING_UNICODE
#ifdef _WIN32
#pragma message ("*** Symbol encoding: Unicode")
#endif
#endif
#ifdef ET9SYMBOLENCODING_SHIFTJIS
#ifdef _WIN32
#pragma message ("*** Symbol encoding: Shift-Jis")
#endif
#endif

#include "et9charprop.c"


/*---------------------------------------------------------------------------*/
/** \internal
 * Get the character class of a symbol.
 *
 * @param sSymb          Symbol to check.
 *
 * @return The character class.
 */

ET9SymbClass ET9FARCALL _ET9_GetSymbolClass (const ET9SYMB sSymb);

/*---------------------------------------------------------------------------*/
/** \internal
 * Convert symbol to lower case.
 *
 * @param sSymb          Symbol to check.
 * @param wLdbNum        Relevant language source for symbol.
 *
 * @return The lower case symbol (or unmodified).
 */

ET9SYMB ET9FARCALL _ET9SymToLower (const ET9SYMB sSymb, const ET9U16 wLdbNum);

/*---------------------------------------------------------------------------*/
/** \internal
 * Convert symbol to upper case.
 *
 * @param sSymb          Symbol to check.
 * @param wLdbNum        Relevant language source for symbol.
 *
 * @return The upper case symbol (or unmodified).
 */

ET9SYMB ET9FARCALL _ET9SymToUpper (const ET9SYMB sSymb, const ET9U16 wLdbNum);

/*---------------------------------------------------------------------------*/
/** \internal
 * Check if the symbol is an lower case symbol.
 *
 * @param sSymb          Symbol to check.
 * @param wLdbNum        Relevant language source for symbol.
 *
 * @return Non zero if symbol is lower case, zero if symbol is upper case or no case conversion defined.
 */

ET9BOOL ET9FARCALL _ET9SymIsLower(const ET9SYMB sSymb, const ET9U16 wLdbNum)
{
    const ET9SYMB sUpper = _ET9SymToUpper(sSymb, wLdbNum);

    return (ET9BOOL)(sUpper != sSymb);
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Check if the symbol is an upper case symbol.
 *
 * @param sSymb          Symbol to check.
 * @param wLdbNum        Relevant language source for symbol.
 *
 * @return Non zero if symbol is upper case, zero if symbol is lower case or no case conversion defined.
 */

ET9BOOL ET9FARCALL _ET9SymIsUpper(const ET9SYMB sSymb, const ET9U16 wLdbNum)
{
    const ET9SYMB sLower = _ET9SymToLower(sSymb, wLdbNum);

    return (ET9BOOL)(sLower != sSymb);
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Return symbol of opposite case.
 *
 * @param sSymb          Symbol to check.
 * @param wLdbNum        Relevant language source for symbol.
 *
 * @return Symbol of opposite case.
 */

ET9SYMB ET9FARCALL _ET9SymToOther(const ET9SYMB sSymb, const ET9U16 wLdbNum)
{
    const ET9SYMB sLower = _ET9SymToLower(sSymb, wLdbNum);

    if (sLower != sSymb) {
        return sLower;
    }

    return _ET9SymToUpper(sSymb, wLdbNum);
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Get full symbol key and class.
 * Deprecated.
 *
 * @param sSymb             Symbol to be translated.
 * @param pbKey             Required pointer to key information to be returned.
 * @param pbClass           Optional pointer to class information to be returned.
 *
 * @return Zero if symb has no key, non zero if symb has key. This must return zero if the symbol is whitespace or unknown.
 */

ET9BOOL ET9FARCALL _ET9_GetFullSymbolKeyAndClass(const ET9SYMB      sSymb,
                                                 ET9U8      * const pbKey,
                                                 ET9U8      * const pbClass)
{
    const ET9SymbClass eClass = _ET9_GetSymbolClass(sSymb);

    if (pbKey) {
        *pbKey = 0;
    }

    if (pbClass) {
        *pbClass = (ET9U8)eClass;
    }

    return (ET9BOOL)(eClass != ET9_WhiteSymbClass && eClass != ET9_UnassSymbClass);
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Check if the symbol is unknwon.
 *
 * @param sSymb             Symbol to check.
 *
 * @return Non zero if unknown, otherwise zero.
 */

ET9BOOL ET9FARCALL _ET9_IsUnknown(const ET9SYMB sSymb)
{
    const ET9SymbClass eClass = _ET9_GetSymbolClass(sSymb);

    return (ET9BOOL)(eClass == ET9_UnassSymbClass);
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Check if the symbol is whitespace.
 *
 * @param sSymb             Symbol to check.
 *
 * @return Non zero if whitespace, otherwise zero.
 */

ET9BOOL ET9FARCALL _ET9_IsWhiteSpace(const ET9SYMB sSymb)
{
    const ET9SymbClass eClass = _ET9_GetSymbolClass(sSymb);

    return (ET9BOOL)(eClass == ET9_WhiteSymbClass);
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Check if the symbol is punctuation.
 *
 * @param sSymb             Symbol to check.
 *
 * @return Non zero if punctuation, otherwise zero.
 */

ET9BOOL ET9FARCALL _ET9_IsPunctChar(const ET9SYMB sSymb)

{
    const ET9SymbClass eClass = _ET9_GetSymbolClass(sSymb);

    return (ET9BOOL)(eClass == ET9_PunctSymbClass);
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Check if the symbol is numeric.
 *
 * @param sSymb             Symbol to check.
 *
 * @return Non zero if numeric, otherwise zero.
 */

ET9BOOL ET9FARCALL _ET9_IsNumeric(const ET9SYMB sSymb)

{
    const ET9SymbClass eClass = _ET9_GetSymbolClass(sSymb);

    return (ET9BOOL)(eClass == ET9_NumbrSymbClass);
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Check if the symbol is punctuation or numeric.
 *
 * @param sSymb             Symbol to check.
 *
 * @return Non zero if punctuation or numeric, otherwise zero.
 */

ET9BOOL ET9FARCALL _ET9_IsPunctOrNumeric(const ET9SYMB sSymb)

{
    const ET9SymbClass eClass = _ET9_GetSymbolClass(sSymb);

    return (ET9BOOL)(eClass == ET9_PunctSymbClass || eClass == ET9_NumbrSymbClass);
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Check if the string is all numeric.
 *
 * @param psString          Pointer to string.
 * @param nStrLen           String length.
 *
 * @return Non zero if all numeric, otherwise zero.
 */

ET9BOOL ET9FARCALL _ET9_IsNumericString(ET9SYMB       const * const psString,
                                        const ET9UINT               nStrLen)

{
    ET9UINT nCount;
    ET9SYMB const * psSymb;

    psSymb = psString;
    for (nCount = nStrLen; nCount; --nCount, ++psSymb) {
        if (!_ET9_IsNumeric(*psSymb)) {
            return 0;
        }
    }

    return 1;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Find white spaces / unknown chars in a buffer of text.
 *
 * @param psString          Pointer to string.
 * @param nStrLen           String length.
 *
 * @return Non zero if spaces/unknown chars are found, otherwise zero.
 */

ET9BOOL ET9FARCALL _ET9FindSpacesAndUnknown(ET9SYMB       const * const psString,
                                            const ET9UINT               nStrLen)
{
    ET9UINT nCount;
    ET9SYMB const * psSymb;

    psSymb = psString;
    for (nCount = nStrLen; nCount; --nCount, ++psSymb) {

        const ET9SymbClass eClass = _ET9_GetSymbolClass(*psSymb);

        if (eClass == ET9_WhiteSymbClass || eClass == ET9_UnassSymbClass) {
            return 1;
        }

        /* special check for NULL symbs */

        if (!*psSymb) {
            return 1;
        }
    }

    return 0;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Check if a string is a likely emoticon.
 * Checks to see if word has the following characteristics:<P>
 * 1) number non-alpha character equal to or greater than the number of alpha chars.
 *
 * @param psString          Pointer to string.
 * @param nStrLen           String length.
 *
 * @return Non zero if looks like emoticon, otherwise zero.
 */

ET9BOOL ET9FARCALL _ET9StringLikelyEmoticon(ET9SYMB       const * const psString,
                                            const ET9UINT               nStrLen)
{
    ET9UINT nCount;
    ET9UINT nAlphaCount;
    ET9SYMB const * psSymb;

    nAlphaCount = 0;

    psSymb = psString;
    for (nCount = nStrLen; nCount; --nCount, ++psSymb) {

        switch (_ET9_GetSymbolClass(*psSymb))
        {
            case ET9_WhiteSymbClass:
            case ET9_UnassSymbClass:
                return 0;
            case ET9_AlphaSymbClass:
                ++nAlphaCount;
                break;
            case ET9_PunctSymbClass:
            case ET9_NumbrSymbClass:
                break;
        }
    }

    return (ET9BOOL)((nStrLen > 1) && (2 * nAlphaCount) <= nStrLen);
}

/*---------------------------------------------------------------------------*/
/**
 * If the symbol is a SHIFTED symbol, convert to lower case.
 * Given a character return the lower case version.  If the input character is already
 * lower case, or has no case variant, then the input will be returned as output.
 *
 * @param sData             Symbol to check.
 *
 * @return Symbol of lower case.
 */

ET9SYMB ET9FARCALL ET9SymToLower(const ET9SYMB sData)
{
    return _ET9SymToLower(sData, ET9PLIDNone);
}

/*---------------------------------------------------------------------------*/
/**
 * If the symbol is lower case, convert to upper case.
 * Given a character return the upper case version.  If the input character is already
 * lower case, or has no case variant, then the input will be returned as output.
 *
 * @param sData             Symbol to check.
 *
 * @return Symbol of upper case.
 */

ET9SYMB ET9FARCALL ET9SymToUpper(const ET9SYMB sData)
{
    return _ET9SymToUpper(sData, ET9PLIDNone);
}

/*---------------------------------------------------------------------------*/
/**
 * Return symbol of opposite case.
 * Given a character return the other case version.  If the input character
 * has no case variant, then the input will be returned as output.
 *
 * @param sData             Symbol to check.
 *
 * @return Symbol of other case.
 */

ET9SYMB ET9FARCALL ET9SymToOther(const ET9SYMB sData)
{
    return _ET9SymToOther(sData, ET9PLIDNone);
}

/*---------------------------------------------------------------------------*/
/**
 * Check if the symbol is an upper case symbol.
 *
 * @param sData             Symbol to check.
 *
 * @return Non zero is upper case, otherwise zero.
 */

ET9UINT ET9FARCALL ET9SymIsUpper(const ET9SYMB sData)
{
    return (ET9UINT)_ET9SymIsUpper(sData, ET9PLIDNone);
}

/*---------------------------------------------------------------------------*/
/**
 * Check if the symbol is a lower case symbol.
 *
 * @param sData             Symbol to check.
 *
 * @return Non zero is lower case, otherwise zero.
 */

ET9UINT ET9FARCALL ET9SymIsLower(const ET9SYMB sData)
{
    return (ET9UINT)_ET9SymIsLower(sData, ET9PLIDNone);
}

/*---------------------------------------------------------------------------*/
/**
 * Get the character class of a symbol.
 *
 * @param sData             Character to classify.
 *
 * @return Character class, ET9SYMWHITE, ET9SYMPUNCT, ET9SYMNUMBR, ET9SYMALPHA, ET9SYMUNKN.
 */

ET9U8 ET9FARCALL ET9GetSymbolClass(const ET9SYMB sData)
{
    ET9Assert(ET9SYMWHITE == ET9_WhiteSymbClass);
    ET9Assert(ET9SYMPUNCT == ET9_PunctSymbClass);
    ET9Assert(ET9SYMNUMBR == ET9_NumbrSymbClass);
    ET9Assert(ET9SYMALPHA == ET9_AlphaSymbClass);
    ET9Assert(ET9SYMUNKN  == ET9_UnassSymbClass);

    return (ET9U8)_ET9_GetSymbolClass(sData);
}


#ifdef ET9_DEBUG

/*---------------------------------------------------------------------------*/
/** \internal
 * QA use...
 */

ET9SYMBOL_ENCODING ET9SymbolEncodingQA = ET9SYMBOL_ENCODING_LAST;

#endif

/*---------------------------------------------------------------------------*/
/** \internal
 * Get the current symbol encoding.
 * This is a static compile time value.
 * This is potentially a new api function.
 *
 * @return Current symbol encoding.
 */

ET9SYMBOL_ENCODING ET9FARCALL ET9_GetSymbolEncoding(void)
{
#ifdef ET9_DEBUG
    if (ET9SymbolEncodingQA < ET9SYMBOL_ENCODING_LAST) {
        return ET9SymbolEncodingQA;
    }
#endif

#ifdef ET9SYMBOLENCODING_UNICODE
    return ET9SYMBOL_ENCODING_UNICODE;
#endif
#ifdef ET9SYMBOLENCODING_SHIFTJIS
    return ET9SYMBOL_ENCODING_SHIFTJIS;
#endif
}


/*! @} */
/* ----------------------------------< eof >--------------------------------- */
