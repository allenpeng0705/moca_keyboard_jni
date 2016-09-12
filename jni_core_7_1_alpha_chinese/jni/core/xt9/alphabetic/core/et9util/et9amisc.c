/*******************************************************************************
;*******************************************************************************
;**                                                                           **
;**                  COPYRIGHT 2001-2011 NUANCE COMMUNICATIONS                **
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
;**     FileName: et9amisc.c                                                  **
;**                                                                           **
;**  Description: miscellaneous tools for ET9                                 **
;**                                                                           **
;*******************************************************************************
;******************************************************************************/

/*! \internal \addtogroup et9amisc Miscellaneous tools for alphabetic
* Miscellaneous tools for alphabetic XT9.
* @{
*/

#include "et9api.h"
#ifdef ET9_ALPHABETIC_MODULE
#include "et9amisc.h"


/*---------------------------------------------------------------------------*/
/** \internal
 * Check for existence of pLingCmnInfo AND good initialization setting.
 *
 * @param pLingCmnInfo   Pointer to alphabetic common information structure.
 *
 * @return ET9STATUS from call.
 */

ET9STATUS ET9FARCALL _ET9AWSys_BasicCmnValidityCheck(ET9AWLingCmnInfo *pLingCmnInfo)
{
    if (pLingCmnInfo == NULL) {
        return ET9STATUS_INVALID_MEMORY;
    }
    if (pLingCmnInfo->Private.wInfoInitOK != ET9GOODSETUP) {
        return ET9STATUS_NO_INIT;
    }
    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Check for existence of pLingInfo AND good initialization setting.
 *
 * @param pLingInfo      Pointer to alphabetic information structure.
 *
 * @return ET9STATUS from call.
 */

ET9STATUS ET9FARCALL _ET9AWSys_BasicValidityCheck(ET9AWLingInfo *pLingInfo)
{
    if (pLingInfo == NULL) {
        return ET9STATUS_INVALID_MEMORY;
    }
    if (pLingInfo->Private.wInfoInitOK != ET9GOODSETUP) {
        return ET9STATUS_NO_INIT;
    }
    if (pLingInfo->pLingCmnInfo == NULL) {
        return ET9STATUS_INVALID_MEMORY;
    }
    if (pLingInfo->pLingCmnInfo->Private.wInfoInitOK != ET9GOODSETUP) {
        return ET9STATUS_NO_INIT;
    }
    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Convert a simple word to a priv word.
 *
 * @param pSimple           Pointer to simple word.
 * @param pPriv             Pointer to priv word.
 *
 * @return Byte distance.
 */

void ET9FARCALL _ET9SimpleWordToPrivWord(ET9SimpleWord * const pSimple, ET9AWPrivWordInfo * const pPriv)
{
    ET9Assert(pPriv);
    ET9Assert(pSimple);

    _InitPrivWordInfo(pPriv);

    if (pSimple->wLen) {
        _ET9SymCopy(pPriv->Base.sWord, pSimple->sString, pSimple->wLen);
    }

    pPriv->Base.wWordLen = pSimple->wLen;
    pPriv->Base.wWordCompLen = pSimple->wCompLen;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * Convert a priv word to a simple word.
 *
 * @param pPriv             Pointer to priv word.
 * @param pSimple           Pointer to simple word.
 *
 * @return Byte distance.
 */

void ET9FARCALL _ET9PrivWordToSimpleWord(ET9AWPrivWordInfo * const pPriv, ET9SimpleWord * const pSimple)
{
    ET9Assert(pPriv);
    ET9Assert(pSimple);

    if (pPriv->Base.wWordLen) {
        _ET9SymCopy(pSimple->sString, pPriv->Base.sWord, pPriv->Base.wWordLen);
    }

    pSimple->wLen = pPriv->Base.wWordLen;
    pSimple->wCompLen = pPriv->Base.wWordCompLen;
}

#ifdef ET9CHECKCOMPILE
/*---------------------------------------------------------------------------*/
/**
 * T9AWCheckCompileParameters
 * Check the consistency between core data type and integration.
 *
 * @param pbET9U8             Xx
 *
 * @return Byte distance.
 */

ET9U32 ET9FARCALL ET9AWCheckCompileParameters(ET9U8   *pbET9U8,
                                              ET9U8   *pbET9U16,
                                              ET9U8   *pbET9U32,
                                              ET9U8   *pbET9UINT,
                                              ET9U8   *pbET9S8,
                                              ET9U8   *pbET9S16,
                                              ET9U8   *pbET9S32,
                                              ET9U8   *pbET9INT,
                                              ET9U8   *pbET9SYMB,
                                              ET9U8   *pbET9BOOL,
                                              ET9U8   *pbET9FARDATA,
                                              ET9U8   *pbET9FARCALL,
                                              ET9U8   *pbET9LOCALCALL,
                                              ET9U8   *pbVoidPtr,
                                              ET9U16  *pwET9SymbInfo,
                                              ET9UINT  *pwET9WordSymbInfo,
                                              ET9U8   *pbET9MAXSELLISTSIZE,
                                              ET9U8   *pbET9MAXWORDSIZE,
                                              ET9U8   *pbET9MAXUDBWORDSIZE,
                                              ET9U16  *pwET9AWWordInfo,
                                              ET9U16  *pwET9AWLingInfo,
                                              ET9U16  *pwET9AWLingCmnInfo)
{
    ET9U32 dwError;

    dwError = ET9_CheckCompileParameters(pbET9U8, pbET9U16, pbET9U32, pbET9UINT,
        pbET9S8, pbET9S16, pbET9S32, pbET9INT, pbET9SYMB, pbET9BOOL, pbET9FARDATA, pbET9FARCALL,
        pbET9LOCALCALL, pbVoidPtr, pwET9SymbInfo, pwET9WordSymbInfo);


    if ((dwError == (ET9U32)ET9NULL_POINTERS) ||
        !(pbET9MAXSELLISTSIZE && pbET9MAXWORDSIZE && pbET9MAXUDBWORDSIZE &&
          pwET9AWWordInfo && pwET9AWLingInfo && pwET9AWLingCmnInfo)) {
        return (ET9U32)ET9NULL_POINTERS;
    }

    if (*pbET9MAXSELLISTSIZE != ET9MAXSELLISTSIZE) {
        *pbET9MAXSELLISTSIZE = ET9MAXWORDSIZE;
        dwError |= (1L << ET9WRONGVALUE_ET9MAXSELLISTSIZE);
    }

    if (*pbET9MAXWORDSIZE != ET9MAXWORDSIZE) {
        *pbET9MAXWORDSIZE = ET9MAXWORDSIZE;
        dwError |= (1L << ET9WRONGVALUE_ET9MAXWORDSIZE);
    }

    if (*pbET9MAXUDBWORDSIZE != ET9MAXUDBWORDSIZE) {
        *pbET9MAXUDBWORDSIZE = ET9MAXUDBWORDSIZE;
        dwError |= (1L << ET9WRONGVALUE_ET9MAXUDBWORDSIZE);
    }

    if (sizeof(ET9AWWordInfo) != *pwET9AWWordInfo) {
        *pwET9AWWordInfo = sizeof(ET9AWWordInfo);
        dwError |= (1L << ET9WRONGSIZE_ET9AWWORDINFO);
    }

    if (sizeof(ET9AWLingInfo) != *pwET9AWLingInfo) {
        *pwET9AWLingInfo = sizeof(ET9AWLingInfo);
        dwError |= (1L << ET9WRONGSIZE_ET9AWLINGINFO);
    }

    if (sizeof(ET9AWLingCmnInfo) != *pwET9AWLingCmnInfo) {
        *pwET9AWLingCmnInfo = sizeof(ET9AWLingCmnInfo);
        dwError |= (1L << ET9WRONGSIZE_ET9AWLINGCMNINFO);
    }

    return dwError;
}
#endif


#endif /* ET9_ALPHABETIC_MODULE */
/*! @} */
/* ----------------------------------< eof >--------------------------------- */
