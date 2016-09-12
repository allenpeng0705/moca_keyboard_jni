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
;**     FileName: et9asym.c                                                   **
;**                                                                           **
;**  Description: Case, Class, Key assignment routines source file.           **
;**                                                                           **
;*******************************************************************************
;******* 10 ****** 20 ****** 30 ****** 40 ****** 50 ****** 60 ****** 70 *******/

/*! \internal \addtogroup et9asym Symbol handling for alphabetic
* Symbol handling for alphabetic XT9.
* @{
*/

#include "et9api.h"
#ifdef ET9_ALPHABETIC_MODULE
#include "et9sym.h"
#include "et9asym.h"
#include "et9amisc.h"


#define NUM_STANDARD_TERM_PUNCT_CHARS   9       /**< \internal number of term punct chars for standard */
#define NUM_SPANISH_TERM_PUNCT_CHARS    11      /**< \internal number of term punct chars for spanish */
#define NUM_GREEK_TERM_PUNCT_CHARS      9       /**< \internal number of term punct chars for greek */
#define NUM_ARABIC_TERM_PUNCT_CHARS     9       /**< \internal number of term punct chars for arabic */
#define NUM_FARSI_TERM_PUNCT_CHARS      10      /**< \internal number of term punct chars for farsi */
#define NUM_PASHTO_TERM_PUNCT_CHARS     6       /**< \internal number of term punct chars for pashto */
#define NUM_THAI_TERM_PUNCT_CHARS       10      /**< \internal number of term punct chars for thai */
#define NUM_CATALAN_TERM_PUNCT_CHARS    12      /**< \internal number of term punct chars for catalan */
#define NUM_KHMER_TERM_PUNCT_CHARS      7       /**< \internal number of term punct chars for khmer */
#define NUM_HEBREW_TERM_PUNCT_CHARS     10      /**< \internal number of term punct chars for hebrew */
#define NUM_AMHARIC_TERM_PUNCT_CHARS    8       /**< \internal number of term punct chars for amharic */
#define NUM_ARMENIAN_TERM_PUNCT_CHARS   9       /**< \internal number of term punct chars for armenian */


/** \internal  english french italian german        . ? ! , - ' @ : /      */
static const ET9SYMB StandardTermPunct[] = {0x002e, 0x003f, 0x0021, 0x002c, 0x002d, 0x0027, 0x0040, 0x003a, 0x002f};

/** \internal  spanish        . ¿ ? ¡ ! , - ' @ : /     */
static const ET9SYMB  SpanishTermPunct[] = {0x002e, 0x00bf, 0x003f, 0x00a1, 0x0021, 0x002c, 0x002d, 0x0027, 0x0040, 0x003a, 0x002f};

/** \internal  greek        . ; ! , - ' @ : /      */
static const ET9SYMB GreekTermPunct[] = {0x002e, 0x003b, 0x0021, 0x002c, 0x002d, 0x0027, 0x0040, 0x003a, 0x002f};

/** \internal  arabic    . arabicquestionmark ! arabiccomma arabicsemicolon doublequote @ : /          */
static const ET9SYMB ArabicTermPunct[] = {0x002e, 0x061f, 0x0021, 0x060c, 0x061b, 0x0022, 0x0040, 0x003a, 0x002f};

/** \internal  farsi    */
static const ET9SYMB FarsiTermPunct[] = {0x002e, 0x061f, 0x0021, 0x060c, 0x061b, 0x0022, 0x002d, 0x003a, 0x002f, 0x0040};

/** \internal  pashto    . arabicquestionmark : arabiccomma !       */
static const ET9SYMB PashtoTermPunct[] = {0x002e, 0x061f, 0x003a, 0x060c, 0x0021, 0x061B};

/** \internal  Thai   space   . ? ! , - ' @ : /           */
static const ET9SYMB ThaiTermPunct[] = {0x0020, 0x002e, 0x003f, 0x0021, 0x002c, 0x002d, 0x0027, 0x0040, 0x003a, 0x002f};

/** \internal  Catalan  (spanish appended with ·) . ? ! , - · ' @ : ¿ ¡ / ·      */
static const ET9SYMB CatalanTermPunct[] = {0x002e, 0x00bf, 0x003f, 0x00a1, 0x0021, 0x002c, 0x002d, 0x0027, 0x0040, 0x003a, 0x002f, 0x00b7};

/** \internal  Khmer space Khan Bariyoosan 'Camnuc Pii Kuuh' 'Lek Too' . 'Currency Symbol Riel' */
static const ET9SYMB KhmerTermPunct[] = {0x0020, 0x17D4, 0x17D5, 0x17D6, 0x17D7, 0x002E, 0x17DB};

/** \internal   Hebrew       . ? ! , - '  doublequote    @ : /                  */
static const ET9SYMB HebrewTermPunct[] = {0x002e, 0x003f, 0x0021, 0x002c, 0x002d, 0x0027, 0x0022, 0x0040, 0x003a, 0x002f};

/** \internal   Amharic      */
static const ET9SYMB AmharicTermPunct[] = {0x1362, 0x1363, 0x003f, 0x0021, 0x002f, 0x1364, 0x1365, 0x1366};

/** \internal   Armenian      */
static const ET9SYMB ArmenianTermPunct[] = {0x0589, 0x055E, 0x055D, 0x055B, 0x055C, 0x002e, 0x003f, 0x0021, 0x002c};


static const ET9SYMB sPeriodEmbPunct = 0x002e;
static const ET9SYMB sHyphenEmbPunct = 0x002d;
static const ET9SYMB sDandaEmbPunct  = 0x0964;
static const ET9SYMB sSpaceEmbPunct  = 0x0020;
static const ET9SYMB sArmQuestionEmbPunct  = 0x055E;

/*---------------------------------------------------------------------------*/
/** \internal
 * Check if a language has embedded hyphen.
 *
 * @param n                 Language id.
 *
 * @return Non zero if it has, otherwise zero.
 */

#define IS_EMBEDDED_HYPHEN(n) ( \
n == ET9PLIDBasque || \
n == ET9PLIDBelarusian || \
n == ET9PLIDCatalan || \
n == ET9PLIDFrench || \
n == ET9PLIDGalician || \
n == ET9PLIDGreek || \
n == ET9PLIDIndonesian || \
n == ET9PLIDIrish || \
n == ET9PLIDJavanese || \
n == ET9PLIDKirghiz || \
n == ET9PLIDLithuanian || \
n == ET9PLIDMalay || \
n == ET9PLIDPolish || \
n == ET9PLIDMongolian || \
n == ET9PLIDTurkmen || \
n == ET9PLIDPortuguese || \
n == ET9PLIDRomanian || \
n == ET9PLIDSundanese || \
n == ET9PLIDTagalog || \
n == ET9PLIDTajik || \
n == ET9PLIDXhosa || \
n == ET9PLIDZulu)

#define IS_EMBEDDED_DANDA(n) (n == ET9PLIDBengali)

#define IS_EMBEDDED_SPACE(n) (n == ET9PLIDThai || n == ET9PLIDKhmer || n == ET9PLIDLao || n == ET9PLIDAmharic)

#define IS_EMBEDDED_ARMQUESTION(n) (n == ET9PLIDArmenian)

/*---------------------------------------------------------------------------*/
/** \internal
 * Get the embedded punctuation symbol.
 *
 * @param pLingInfo                 Pointer to alphabetic information structure.
 * @param wLdbNum                   LDB number.
 *
 * @return Embedded punctuation character.
 */

ET9SYMB ET9FARCALL _ET9_GetEmbPunctChar(ET9AWLingInfo   * const pLingInfo,
                                        const ET9U16            wLdbNum)
{
    ET9AWLingCmnInfo * const pLingCmnInfo = pLingInfo->pLingCmnInfo;
    ET9SYMB sHolder;
    ET9U16 wLang = wLdbNum & ET9PLIDMASK;

    ET9Assert(pLingInfo);

    if (pLingCmnInfo->Private.sExpEmbeddedPunct) {
        sHolder = pLingCmnInfo->Private.sExpEmbeddedPunct;
    }
    else if (IS_EMBEDDED_HYPHEN(wLang)) {
        sHolder = sHyphenEmbPunct;
    }
    else if (IS_EMBEDDED_DANDA(wLang)) {
        sHolder = sDandaEmbPunct;
    }
    else if (IS_EMBEDDED_SPACE(wLang)) {
        sHolder = sSpaceEmbPunct;
    }
    else if (IS_EMBEDDED_ARMQUESTION(wLang)) {
        sHolder = sArmQuestionEmbPunct;
    }
    else {
        sHolder = sPeriodEmbPunct;
    }
    return sHolder;
}


/*---------------------------------------------------------------------------*/
/** \internal
 * Get the terminal punctuation symbols.
 *
 * @param pLingInfo                 Pointer to alphabetic information structure.
 * @param wLdbNum                   LDB number.
 * @param nSymbIndex                Index to get.
 *
 * @return Requested terminal punct char.
 */

ET9SYMB ET9FARCALL _ET9_GetTermPunctChar(ET9AWLingInfo  * const pLingInfo,
                                         const ET9U16           wLdbNum,
                                         const ET9UINT          nSymbIndex)
{
    ET9AWLingCmnInfo * const pLingCmnInfo = pLingInfo->pLingCmnInfo;

    const ET9SYMB *pTable;
    ET9UINT val;

    ET9Assert(pLingInfo);

    if (pLingCmnInfo->Private.bTotalExpTermPuncts) {
        pTable = pLingCmnInfo->Private.sExpTermPuncts;
        val = pLingCmnInfo->Private.bTotalExpTermPuncts;
    }
    else {
        switch (wLdbNum & ET9PLIDMASK)
        {
            case ET9PLIDSpanish:
            case ET9PLIDGalician:
                pTable = SpanishTermPunct;
                val = NUM_SPANISH_TERM_PUNCT_CHARS;
                break;
            case ET9PLIDGreek:
                pTable = GreekTermPunct;
                val = NUM_GREEK_TERM_PUNCT_CHARS;
                break;
            case ET9PLIDArabic:
            case ET9PLIDUrdu:
                pTable = ArabicTermPunct;
                val = NUM_ARABIC_TERM_PUNCT_CHARS;
                break;
            case ET9PLIDFarsi:
                pTable = FarsiTermPunct;
                val = NUM_FARSI_TERM_PUNCT_CHARS;
                break;
            case ET9PLIDPashto:
                pTable = PashtoTermPunct;
                val = NUM_PASHTO_TERM_PUNCT_CHARS;
                break;
            case ET9PLIDThai:
            case ET9PLIDLao:
                pTable = ThaiTermPunct;
                val = NUM_THAI_TERM_PUNCT_CHARS;
                break;
            case ET9PLIDCatalan:
                pTable = CatalanTermPunct;
                val = NUM_CATALAN_TERM_PUNCT_CHARS;
                break;
            case ET9PLIDKhmer:
                pTable = KhmerTermPunct;
                val = NUM_KHMER_TERM_PUNCT_CHARS;
                break;
            case ET9PLIDHebrew:
                pTable = HebrewTermPunct;
                val = NUM_HEBREW_TERM_PUNCT_CHARS;
                break;
            case ET9PLIDAmharic:
                pTable = AmharicTermPunct;
                val = NUM_AMHARIC_TERM_PUNCT_CHARS;
                break;
            case ET9PLIDArmenian:
                pTable = ArmenianTermPunct;
                val = NUM_ARMENIAN_TERM_PUNCT_CHARS;
                break;
           default:
                pTable = StandardTermPunct;
                val = NUM_STANDARD_TERM_PUNCT_CHARS;
                break;
        }
    }

    ET9Assert(val);
    ET9Assert(pTable[nSymbIndex >= val ? 0 : nSymbIndex]);

    return pTable[nSymbIndex >= val ? 0 : nSymbIndex];
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Get the number of terminal punctuation symbols.
 *
 * @param pLingInfo                 Pointer to alphabetic information structure.
 * @param wLdbNum                   LDB number.
 *
 * @return Number of terminal punct characters.
 */

ET9UINT ET9FARCALL _ET9_GetNumTermPunct(ET9AWLingInfo   * const pLingInfo,
                                        const ET9U16            wLdbNum)
{
    ET9AWLingCmnInfo * const pLingCmnInfo = pLingInfo->pLingCmnInfo;
    ET9UINT val;

    ET9Assert(pLingInfo != NULL);
    ET9Assert(pLingCmnInfo);

    if (pLingCmnInfo->Private.bTotalExpTermPuncts) {
        val = pLingCmnInfo->Private.bTotalExpTermPuncts;
    }
    else {
        switch (wLdbNum & ET9PLIDMASK)
        {
            case ET9PLIDSpanish:
            case ET9PLIDGalician:
                val = NUM_SPANISH_TERM_PUNCT_CHARS;
                break;
            case ET9PLIDGreek:
                val = NUM_GREEK_TERM_PUNCT_CHARS;
                break;
            case ET9PLIDArabic:
            case ET9PLIDUrdu:
                val = NUM_ARABIC_TERM_PUNCT_CHARS;
                break;
            case ET9PLIDFarsi:
                val = NUM_FARSI_TERM_PUNCT_CHARS;
                break;
            case ET9PLIDPashto:
                val = NUM_PASHTO_TERM_PUNCT_CHARS;
                break;
            case ET9PLIDThai:
                val = NUM_THAI_TERM_PUNCT_CHARS;
                break;
            case ET9PLIDCatalan:
                val = NUM_CATALAN_TERM_PUNCT_CHARS;
                break;
            case ET9PLIDKhmer:
                val = NUM_KHMER_TERM_PUNCT_CHARS;
                break;
            case ET9PLIDHebrew:
                val = NUM_HEBREW_TERM_PUNCT_CHARS;
                break;
            case ET9PLIDAmharic:
                val = NUM_AMHARIC_TERM_PUNCT_CHARS;
                break;
            case ET9PLIDArmenian:
                val = NUM_ARMENIAN_TERM_PUNCT_CHARS;
                break;
            default:
                val = NUM_STANDARD_TERM_PUNCT_CHARS;
                break;
        }
    }

    return val;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Is the symb a terminal punctuation.
 *
 * @param pLingInfo                 Pointer to alphabetic information structure.
 * @param wLdbNum                   LDB number.
 * @param sSymb                     Input symbol.
 *
 * @return TRUE/FALSE
 */

ET9BOOL ET9FARCALL _ET9_IsTermPunct(ET9AWLingInfo   * const pLingInfo,
                                    const ET9U16            wLdbNum,
                                    const ET9SYMB           sSymb)
{
    const ET9UINT nTotalPuncts = _ET9_GetNumTermPunct(pLingInfo, wLdbNum);

    ET9UINT i;

    ET9Assert(pLingInfo);

    for (i = 0; i < nTotalPuncts; ++i) {
        if (_ET9_GetTermPunctChar(pLingInfo, wLdbNum, i) == sSymb) {
            return 1;
        }
    }

    return 0;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Get the number of terminal punctuation symbols.
 *
 * @param pLingInfo                 Pointer to alphabetic information structure.
 * @param pWord                     Word (and possibly substitution word) to be OTFMapped.
 *
 * @return Passing through possible error codes from the conversion function.
 */

ET9STATUS ET9FARCALL _ET9_ConvertBuildBuf(ET9AWLingInfo * const pLingInfo,
                                          ET9AWWordInfo * const pWord)
{
    ET9U16      wCount;
    ET9SYMB    *psSymb;
    ET9STATUS   wStatus = ET9STATUS_NONE;

    ET9Assert(pWord != NULL);
    ET9Assert(pLingInfo != NULL);

    /* if the host provides a character conversion (onthefly mapping)*/

    if (!pLingInfo->Private.pConvertSymb) {
        return wStatus;
    }

    wCount = pWord->wWordLen;
    psSymb = pWord->sWord;
    for (; wCount; --wCount, ++psSymb) {

        wStatus = pLingInfo->Private.pConvertSymb(pLingInfo->Private.pConvertSymbInfo, psSymb);

        if (wStatus) {
            break;
        }
    }

    if (!wStatus) {

        wCount = pWord->wSubstitutionLen;
        psSymb = pWord->sSubstitution;
        for (; wCount; --wCount, ++psSymb) {

            wStatus = pLingInfo->Private.pConvertSymb(pLingInfo->Private.pConvertSymbInfo, psSymb);

            if (wStatus) {
                break;
            }
        }
    }

    return wStatus;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Check if spell correction should start at length 2.
 *
 * @param pLingInfo                 Pointer to alphabetic information structure.
 *
 * @return Non zero if rules should be applied, otherwise zero.
 */

ET9BOOL ET9FARCALL _ET9_LanguageSpecific_ApplySpcLenTwo(ET9AWLingInfo * const pLingInfo)
{
    /* Intentionally just checking the first language */

    const ET9U16 wLdbNum  = ET9PLIDMASK & pLingInfo->pLingCmnInfo->wFirstLdbNum;

    switch (wLdbNum)
    {
        case ET9PLIDNone:
        case ET9PLIDFrench:
        case ET9PLIDEnglish:
            return 1;
        default:
            return 0;
    }
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Check if dynamic regionality should be applied.
 *
 * @param pLingInfo                 Pointer to alphabetic information structure.
 *
 * @return Non zero if rules should be applied, otherwise zero.
 */

ET9BOOL ET9FARCALL _ET9_LanguageSpecific_ApplyDynamicRegionality(ET9AWLingInfo * const pLingInfo)
{
    /* Intentionally just checking the first language */

    const ET9U16 wLdbNum  = ET9PLIDMASK & pLingInfo->pLingCmnInfo->wFirstLdbNum;

    switch (wLdbNum)
    {
        case ET9PLIDKorean:
            return 1;
        default:
            return 0;
    }
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Check if capitalization rules should be applied.
 *
 * @param pLingInfo                 Pointer to alphabetic information structure.
 * @param pWord                     Word that is the target for the rules.
 *
 * @return Non zero if rules should be applied, otherwise zero.
 */

ET9BOOL ET9FARCALL _ET9_LanguageSpecific_ApplyCapsRules(ET9AWLingInfo               * const pLingInfo,
                                                        ET9AWPrivWordInfo     const * const pWord)
{
    const ET9U16 wLdbNum  = ET9PLIDMASK & ((pWord->Base.bLangIndex == ET9AWSECOND_LANGUAGE) ? pLingInfo->pLingCmnInfo->wSecondLdbNum : pLingInfo->pLingCmnInfo->wFirstLdbNum);

    switch (wLdbNum)
    {
        case ET9PLIDGerman:
        case ET9PLIDKorean:
        case ET9PLIDJapanese:
            return 0;
        default:
            if (wLdbNum >= ET9PLIDChineseReservePoint) {
                return 0;
            }
            return 1;
    }
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Check if acronym rules should be applied.
 *
 * @param pLingInfo                 Pointer to alphabetic information structure.
 * @param pWord                     Word that is the target for the rules.
 *
 * @return Non zero if rules should be applied, otherwise zero.
 */

ET9BOOL ET9FARCALL _ET9_LanguageSpecific_ApplyAcronymRules(ET9AWLingInfo               * const pLingInfo,
                                                           ET9AWPrivWordInfo     const * const pWord)
{
    const ET9U16 wLdbNum  = ET9PLIDMASK & ((pWord->Base.bLangIndex == ET9AWSECOND_LANGUAGE) ? pLingInfo->pLingCmnInfo->wSecondLdbNum : pLingInfo->pLingCmnInfo->wFirstLdbNum);

    switch (wLdbNum)
    {
        case ET9PLIDKorean:
        case ET9PLIDJapanese:
            return 0;
        default:
            if (wLdbNum >= ET9PLIDChineseReservePoint) {
                return 0;
            }
            return 1;
    }
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Check if shifting should be applied.
 *
 * @param pLingInfo                 Pointer to alphabetic information structure.
 * @param pWord                     Word that is the target for the rules.
 *
 * @return Non zero if shifting should be applied, otherwise zero.
 */

ET9BOOL ET9FARCALL _ET9_LanguageSpecific_ApplyShifting(ET9AWLingInfo               * const pLingInfo,
                                                       ET9AWPrivWordInfo     const * const pWord)
{
    const ET9U16 wLdbNum  = ET9PLIDMASK & ((pWord->Base.bLangIndex == ET9AWSECOND_LANGUAGE) ? pLingInfo->pLingCmnInfo->wSecondLdbNum : pLingInfo->pLingCmnInfo->wFirstLdbNum);

    switch (wLdbNum)
    {
        case ET9PLIDKorean:
        case ET9PLIDJapanese:
            return 0;
        default:
            if (wLdbNum >= ET9PLIDChineseReservePoint) {
                return 0;
            }
            return 1;
    }
}

#endif /* ET9_ALPHABETIC_MODULE */
/*! @} */
/* ----------------------------------< eof >--------------------------------- */
