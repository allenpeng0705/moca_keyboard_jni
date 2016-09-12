/* This file was sent by Erland Unruh, but can also be found in the Tegic CVS:
 *  http://se-cvs01/viewvc/et9/udbwords/
 */

/*******************************************************************************
;*******************************************************************************
;**                                                                           **
;**                  COPYRIGHT 2006-2009 NUANCE COMMUNICATIONS                **
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
;**     FileName: et9api.h                                                    **
;**                                                                           **
;**  Description: Mini version of the main ET9 header file.                   **
;**                                                                           **
;**                                                                           **
;*******************************************************************************
;******************************************************************************/

#ifndef ET9API_H
#define ET9API_H


/* ******************************************************************** */

#include "xxet9oem.h"

/* ******************************************************************** */

#ifndef NULL
#ifdef __cplusplus
#define NULL    0
#else
#define NULL    ((void*)0)
#endif
#endif

/* ******************************************************************** */

#ifndef ET9BOOL
#define ET9BOOL    char                 /**< Anything that fits the architecture. */
#endif
#ifndef ET9S8
#define ET9S8      signed char          /**< Signed  8-bit quantity. */
#endif
#ifndef ET9S16
#define ET9S16     signed short         /**< Signed 16-bit quantity. */
#endif
#ifndef ET9S32
#define ET9S32     signed long          /**< Signed 32-bit quantity. */
#endif
#ifndef ET9U8
#define ET9U8      unsigned char        /**< Unsigned  8-bit quantity. */
#endif
#ifndef ET9U16
#define ET9U16     unsigned short       /**< Unsigned 16-bit quantity. */
#endif
#ifndef ET9U32
#define ET9U32     unsigned long        /**< Unsigned 32-bit quantity. */
#endif

#define ET9SYMB  ET9U16
#define ET9SYMBOLWIDTH   2              /**< Width of XT9 symbols. */

#ifndef ET9INT
#define ET9INT      signed int          /**< Should be the most natural compiler type ('int' is 16 bits or more by ANSI definition). */
#endif
#ifndef ET9UINT
#define ET9UINT     unsigned int        /**< Should be the most natural compiler type ('int' is 16 bits or more by ANSI definition). */
#endif

/* ******************************************************************** */ 

#define ET9FARCALL
#define ET9LOCALCALL
#define ET9FARDATA

/* ******************************************************************** */ 

typedef enum ET9STATUS_e {
    ET9STATUS_NONE = 0,                     /**< 00 : No errors encountered (guaranteed to be zero) */
    ET9STATUS_ERROR,                        /**< 01 : General error status */
    ET9STATUS_INVALID_MEMORY,               /**< 09 : Invalid memory */
    ET9STATUS_READ_DB_FAIL,                 /**< 10 : Unable to read DB */
    ET9STATUS_LDB_VERSION_ERROR,            /**< 14 : LDB Version mismatch */
    ET9STATUS_CORRUPT_DB,                   /**< 22 : Corrupted DB */
    ET9STATUS_INVALID_SIZE,                 /**< 26 : Invalid DB data size */
    ET9STATUS_BAD_PARAM,                    /**< 27 : Bad Parameter passed */
    ET9STATUS_ALREADY_INITIALIZED,          /**< 28 : DB already initialized */
    ET9STATUS_TYPE_ERROR,                   /**< 50 : Type error, e.g. with fundamental type definitions */

    ET9STATLAST                             /**< \internal to have a max stat number and to have an entry which ends without a comma */

} ET9STATUS;

/* ******************************************************************** */ 

/*----------------------------------------------------------------------------
 *  Primary language data base indentifier enumeration
 *----------------------------------------------------------------------------*/

#define ET9PLIDMASK         ((ET9U16)0x00FF)

#define ET9PLIDNone         ((ET9U16)0x0000)
#define ET9PLIDArabic       ((ET9U16)0x0001)
#define ET9PLIDBulgarian    ((ET9U16)0x0002)
#define ET9PLIDCatalan      ((ET9U16)0x0003)
#define ET9PLIDChinese      ((ET9U16)0x0004)
#define ET9PLIDCzech        ((ET9U16)0x0005)
#define ET9PLIDDanish       ((ET9U16)0x0006)
#define ET9PLIDGerman       ((ET9U16)0x0007)
#define ET9PLIDGreek        ((ET9U16)0x0008)
#define ET9PLIDEnglish      ((ET9U16)0x0009)
#define ET9PLIDSpanish      ((ET9U16)0x000A)
#define ET9PLIDFinnish      ((ET9U16)0x000B)
#define ET9PLIDFrench       ((ET9U16)0x000C)
#define ET9PLIDHebrew       ((ET9U16)0x000D)
#define ET9PLIDHungarian    ((ET9U16)0x000E)
#define ET9PLIDIcelandic    ((ET9U16)0x000F)
#define ET9PLIDItalian      ((ET9U16)0x0010)
#define ET9PLIDJapanese     ((ET9U16)0x0011)
#define ET9PLIDKorean       ((ET9U16)0x0012)
#define ET9PLIDDutch        ((ET9U16)0x0013)
#define ET9PLIDNorwegian    ((ET9U16)0x0014)
#define ET9PLIDPolish       ((ET9U16)0x0015)
#define ET9PLIDPortuguese   ((ET9U16)0x0016)
#define ET9PLIDRhaetoRomance ((ET9U16)0x0017)
#define ET9PLIDRomanian     ((ET9U16)0x0018)
#define ET9PLIDRussian      ((ET9U16)0x0019)
#define ET9PLIDSerboCroatian ((ET9U16)0x001A)
#define ET9PLIDSlovak       ((ET9U16)0x001B)
#define ET9PLIDAlbanian     ((ET9U16)0x001C)
#define ET9PLIDSwedish      ((ET9U16)0x001D)
#define ET9PLIDThai         ((ET9U16)0x001E)
#define ET9PLIDTurkish      ((ET9U16)0x001F)
#define ET9PLIDUrdu         ((ET9U16)0x0020)
#define ET9PLIDIndonesian   ((ET9U16)0x0021)
#define ET9PLIDUkrainian    ((ET9U16)0x0022)
#define ET9PLIDBelarusian   ((ET9U16)0x0023)
#define ET9PLIDSlovenian    ((ET9U16)0x0024)
#define ET9PLIDEstonian     ((ET9U16)0x0025)
#define ET9PLIDLatvian      ((ET9U16)0x0026)
#define ET9PLIDLithuanian   ((ET9U16)0x0027)
#define ET9PLIDMaori        ((ET9U16)0x0028)
#define ET9PLIDFarsi        ((ET9U16)0x0029)
#define ET9PLIDVietnamese   ((ET9U16)0x002A)
#define ET9PLIDLao          ((ET9U16)0x002B)
#define ET9PLIDKhmer        ((ET9U16)0x002C)
#define ET9PLIDBasque       ((ET9U16)0x002D)
#define ET9PLIDSorbian      ((ET9U16)0x002E)
#define ET9PLIDMacedonian   ((ET9U16)0x002F)
#define ET9PLIDSutu         ((ET9U16)0x0030)
#define ET9PLIDTsonga       ((ET9U16)0x0031)
#define ET9PLIDTswana       ((ET9U16)0x0032)
#define ET9PLIDVenda        ((ET9U16)0x0033)
#define ET9PLIDXhosa        ((ET9U16)0x0034)
#define ET9PLIDZulu         ((ET9U16)0x0035)
#define ET9PLIDAfrikaans    ((ET9U16)0x0036)
#define ET9PLIDFaeroese     ((ET9U16)0x0038)
#define ET9PLIDHindi        ((ET9U16)0x0039)
#define ET9PLIDMaltese      ((ET9U16)0x003A)
#define ET9PLIDSami         ((ET9U16)0x003B)
#define ET9PLIDScotsGaelic  ((ET9U16)0x003C)
#define ET9PLIDMalay        ((ET9U16)0x003E)
#define ET9PLIDSwahili      ((ET9U16)0x0041)
#define ET9PLIDAfar         ((ET9U16)0x0042)
#define ET9PLIDAbkhazian    ((ET9U16)0x0043)
#define ET9PLIDAmharic      ((ET9U16)0x0044)
#define ET9PLIDAssamese     ((ET9U16)0x0045)
#define ET9PLIDAymara       ((ET9U16)0x0046)
#define ET9PLIDAzerbaijani  ((ET9U16)0x0047)
#define ET9PLIDBashkir      ((ET9U16)0x0048)
#define ET9PLIDBihari       ((ET9U16)0x0049)
#define ET9PLIDBislama      ((ET9U16)0x004a)
#define ET9PLIDBengali      ((ET9U16)0x004b)
#define ET9PLIDTibetan      ((ET9U16)0x004c)
#define ET9PLIDBreton       ((ET9U16)0x004d)
#define ET9PLIDCorsican     ((ET9U16)0x004e)
#define ET9PLIDWelsh        ((ET9U16)0x004f)
#define ET9PLIDBhutani      ((ET9U16)0x0050)
#define ET9PLIDEsperanto    ((ET9U16)0x0051)
#define ET9PLIDFiji         ((ET9U16)0x0052)
#define ET9PLIDFrisian      ((ET9U16)0x0053)
#define ET9PLIDIrish        ((ET9U16)0x0054)
#define ET9PLIDGalician     ((ET9U16)0x0055)
#define ET9PLIDGuarani      ((ET9U16)0x0056)
#define ET9PLIDGujarati     ((ET9U16)0x0057)
#define ET9PLIDHausa        ((ET9U16)0x0058)
#define ET9PLIDCroatian     ((ET9U16)0x0059)
#define ET9PLIDArmenian     ((ET9U16)0x005a)
#define ET9PLIDInterlingua  ((ET9U16)0x005b)
#define ET9PLIDInterlingue  ((ET9U16)0x005c)
#define ET9PLIDInupiak      ((ET9U16)0x005d)
#define ET9PLIDInuktitut    ((ET9U16)0x005e)
#define ET9PLIDJavanese     ((ET9U16)0x005f)
#define ET9PLIDGeorgian     ((ET9U16)0x0060)
#define ET9PLIDKazakh       ((ET9U16)0x0061)
#define ET9PLIDGreenlandic  ((ET9U16)0x0062)
#define ET9PLIDKannada      ((ET9U16)0x0063)
#define ET9PLIDKashmiri     ((ET9U16)0x0064)
#define ET9PLIDKurdish      ((ET9U16)0x0065)
#define ET9PLIDKirghiz      ((ET9U16)0x0066)
#define ET9PLIDLatin        ((ET9U16)0x0067)
#define ET9PLIDLingala      ((ET9U16)0x0068)
#define ET9PLIDMalagasy     ((ET9U16)0x0069)
#define ET9PLIDMalayalam    ((ET9U16)0x006a)
#define ET9PLIDMongolian    ((ET9U16)0x006b)
#define ET9PLIDMoldavian    ((ET9U16)0x006c)
#define ET9PLIDMarathi      ((ET9U16)0x006d)
#define ET9PLIDBurmese      ((ET9U16)0x006e)
#define ET9PLIDNauru        ((ET9U16)0x006f)
#define ET9PLIDNepali       ((ET9U16)0x0070)
#define ET9PLIDOccitan      ((ET9U16)0x0071)
#define ET9PLIDOromo        ((ET9U16)0x0072)
#define ET9PLIDOriya        ((ET9U16)0x0073)
#define ET9PLIDPunjabi      ((ET9U16)0x0074)
#define ET9PLIDPashto       ((ET9U16)0x0075)
#define ET9PLIDQuechua      ((ET9U16)0x0076)
#define ET9PLIDKirundi      ((ET9U16)0x0077)
#define ET9PLIDKiyarwanda   ((ET9U16)0x0078)
#define ET9PLIDSanskrit     ((ET9U16)0x0079)
#define ET9PLIDSindhi       ((ET9U16)0x007a)
#define ET9PLIDSangho       ((ET9U16)0x007b)
#define ET9PLIDSinhala      ((ET9U16)0x007c)
#define ET9PLIDSamoan       ((ET9U16)0x007d)
#define ET9PLIDShona        ((ET9U16)0x007e)
#define ET9PLIDSomali       ((ET9U16)0x007f)
#define ET9PLIDSerbian      ((ET9U16)0x0080)
#define ET9PLIDSiswati      ((ET9U16)0x0081)
#define ET9PLIDSesotho      ((ET9U16)0x0082)
#define ET9PLIDSudanese     ((ET9U16)0x0083)
#define ET9PLIDTamil        ((ET9U16)0x0084)
#define ET9PLIDTelugu       ((ET9U16)0x0085)
#define ET9PLIDTajik        ((ET9U16)0x0086)
#define ET9PLIDTigrinya     ((ET9U16)0x0087)
#define ET9PLIDTurkmen      ((ET9U16)0x0088)
#define ET9PLIDTagalog      ((ET9U16)0x0089)
#define ET9PLIDSetswana     ((ET9U16)0x008a)
#define ET9PLIDTonga        ((ET9U16)0x008b)
#define ET9PLIDTatar        ((ET9U16)0x008c)
#define ET9PLIDTwi          ((ET9U16)0x008d)
#define ET9PLIDUyghur       ((ET9U16)0x008e)
#define ET9PLIDUzbek        ((ET9U16)0x008f)
#define ET9PLIDVolapuk      ((ET9U16)0x0090)
#define ET9PLIDWolof        ((ET9U16)0x0091)
#define ET9PLIDYiddish      ((ET9U16)0x0092)
#define ET9PLIDYoruba       ((ET9U16)0x0093)
#define ET9PLIDZhuang       ((ET9U16)0x0094)
#define ET9PLIDIgbo         ((ET9U16)0x0095)
#define ET9PLIDTamazight    ((ET9U16)0x0096)
#define ET9PLIDBosnian      ((ET9U16)0x0096)
#define ET9PLIDDari         ((ET9U16)0x0096)

/* language ID from 0x0097 through 009F are reserved for future */

#define ET9PLIDHinglish     ((ET9U16)0x00D0)
#define ET9PLIDSpanglish    ((ET9U16)0x00D1)

#define ET9PLIDChineseTraditional ((ET9U16)0x00E0)
#define ET9PLIDChineseSimplified  ((ET9U16)0x00E1)
#define ET9PLIDChineseHongkong    ((ET9U16)0x00E2)
#define ET9PLIDChineseSingapore   ((ET9U16)0x00E3)

#define ET9PLIDNull         ((ET9U16)0x00FF)

/*----------------------------------------------------------------------------
 *  Secondary language data base indentifier definitions
 *----------------------------------------------------------------------------*/

#define ET9SLIDMASK     ((ET9U16)0xFF00)
#define ET9SLIDNone     ((ET9U16)0x0000)
#define ET9SLIDDEFAULT  ((ET9U16)0x0100)
#define ET9SLIDCHINESE  ((ET9U16)0x0400)


#endif /* ET9API_H */
/* ----------------------------------< eof >--------------------------------- */
