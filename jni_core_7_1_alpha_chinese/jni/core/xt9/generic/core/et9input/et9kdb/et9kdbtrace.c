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
;**     FileName: et9kdbtrace.c                                               **
;**                                                                           **
;**  Description: Keyboard Database Trace input module module for ET9         **
;**                                                                           **
;*******************************************************************************
;******************************************************************************/

/*! \addtogroup et9trace Functions for XT9 Keyboard Trace Module
 * Trace input for generic XT9.
 * @{
 */

#ifdef ET9_KDB_TRACE_MODULE


#ifndef ET9API_H
#error Bad file to compile separately
#endif

/*---------------------------------------------------------------------------*/
/** \internal
 * Product identifier.
 */

const ET9U8 _pbXt9Trace[] = { 'c', 'o', 'm', '.', 'n', 'u', 'a', 'n', 'c', 'e', '.', 'x', 't', '9', '.', 't', 'r', 'a', 'c', 'e', 0 };


/* ************************************************************************************************************** */
/* * DEFINES / CONSTANTS **************************************************************************************** */
/* ************************************************************************************************************** */

#ifndef ET9_TRACE_COMPRESSION_POINT
#define ET9_TRACE_COMPRESSION_POINT     8
#endif

static const ET9BOOL    bUndefQualityID = 0;                        /**< \internal xxx */

static const ET9UINT    nHighQuality = 1000;                        /**< \internal xxx */

static const ET9FLOAT   fLevelOfDetailQuality = 10.0f;              /**< \internal xxx */
static const ET9FLOAT   fLevelOfDetailEndPoint = 1.25f;             /**< \internal xxx */
static const ET9FLOAT   fLevelOfDetailQualityDistance = 2.0f;       /**< \internal xxx */


/* ************************************************************************************************************** */
/* * PRIVATE **************************************************************************************************** */
/* ************************************************************************************************************** */

#ifdef ET9_PRIVATE_TRACE_LINE_SEGMENT_ACCESS

extern ET9UINT nTraceLineSegmentCount;
extern ET9TracePoint pTraceLineSegmentPoints[];

#endif


#ifndef ET9_PRIVATE_TRACE_FILTER_AVGL1
#define ET9_PRIVATE_TRACE_FILTER_AVGL1      9
#endif

#ifndef ET9_PRIVATE_TRACE_FILTER_AVGL2
#define ET9_PRIVATE_TRACE_FILTER_AVGL2      17
#endif

#ifndef ET9_PRIVATE_TRACE_FILTER_AVGF
#define ET9_PRIVATE_TRACE_FILTER_AVGF       0.25f
#endif

#ifndef ET9_PRIVATE_TRACE_FILTER_HQT
#define ET9_PRIVATE_TRACE_FILTER_HQT        0.88f
#endif


#ifndef ET9_PRIVATE_TRACE_FILTER_GPC
#define ET9_PRIVATE_TRACE_FILTER_GPC        1.0f
#endif

#ifndef ET9_PRIVATE_TRACE_FILTER_DEVC
#define ET9_PRIVATE_TRACE_FILTER_DEVC       0.7f
#endif


/* ************************************************************************************************************** */
/* * PROTOTYPES ************************************************************************************************* */
/* ************************************************************************************************************** */

static ET9FLOAT ET9LOCALCALL __TraceCalculateLevelOfDetailIntegration (ET9KDBInfo     const * const pKDBInfo,
                                                                       const ET9FLOAT               fFactor);

/*---------------------------------------------------------------------------*/
/** \internal
 *
 */

#ifdef ET9_DEBUGLOG6

static void ET9LOCALCALL __TraceLog (ET9KDBInfo     const * const pKDBInfo,
                                     ET9TracePointExt     * const pPoints,
                                     const ET9UINT                nPointCount,
                                     char           const * const pcComment)
{
    ET9UINT nIndex;

    fprintf(pLogFile6, "\nlog-trace - %s\n", pcComment);

    for (nIndex = 0; nIndex < nPointCount; ++nIndex) {

        fprintf(pLogFile6, "  %3u : %8.1f %8.1f   angle %6.1f dist %5.1f density %2u, quality %5u, qid %3u, removed %d, merge %d",
                nIndex,
                pPoints[nIndex].sPoint.fX,
                pPoints[nIndex].sPoint.fY,
                pPoints[nIndex].fAngle,
                pPoints[nIndex].fDist,
                pPoints[nIndex].nDensity,
                pPoints[nIndex].nQuality,
                pPoints[nIndex].bQualityId,
                pPoints[nIndex].bRemoved,
                pPoints[nIndex].bMergeWithPrev);

        if (pPoints[nIndex].wKey == ET9_KDB_KEY_UNDEFINED) {
            fprintf(pLogFile6, ", key UNDEF");
        }
        else if (pPoints[nIndex].wKey == ET9_KDB_KEY_OUTOFBOUND) {
            fprintf(pLogFile6, ", key OUT");
        }
        else {
            fprintf(pLogFile6, ", key %2u", pPoints[nIndex].wKey);
        }

        fprintf(pLogFile6, "\n");
    }

    fflush(pLogFile6);

#ifdef ET9_DEBUGLOG6_CSV
    {
        char pcFileName[500];

        strcpy(pcFileName, pcComment);
        strcat(pcFileName, ".csv");

        FILE *pf = fopen(pcFileName, "w");

        if (pf) {

            fprintf(pf, "id,x,y,angle,dist,density,quality,removed,merge,key\n");

            for (nIndex = 0; nIndex < nPointCount; ++nIndex) {

                fprintf(pf, "%3u,%5.1f,%5.1f,%5.1f,%5.1f,%2u,%5u,%d,%d",
                        nIndex,
                        pPoints[nIndex].sPoint.fX,
                        pPoints[nIndex].sPoint.fY,
                        pPoints[nIndex].fAngle,
                        pPoints[nIndex].fDist,
                        pPoints[nIndex].nDensity,
                        pPoints[nIndex].nQuality,
                        pPoints[nIndex].bRemoved,
                        pPoints[nIndex].bMergeWithPrev);

                if (pPoints[nIndex].wKey == ET9_KDB_KEY_UNDEFINED) {
                    fprintf(pf, ",UNDEF");
                }
                else if (pPoints[nIndex].wKey == ET9_KDB_KEY_OUTOFBOUND) {
                    fprintf(pf, ",OUT");
                }
                else {
                    fprintf(pf, ",%2u", pPoints[nIndex].wKey);
                }

                fprintf(pf, "\n");

                {
                    const ET9FLOAT fLoD = __TraceCalculateLevelOfDetailIntegration(pKDBInfo, fLevelOfDetailQuality);

                    ET9FLOAT fGap;

                    for (fGap = pPoints[nIndex].fDist; fGap > 1.5 * fLoD; fGap -= fLoD) {
                        fprintf(pf, ",,,0,%5.1f\n", fLoD);
                    }
                }
            }

            fclose(pf);
        }
    }
#else
    ET9_UNUSED(pKDBInfo);
#endif
}

#else

#define __TraceLog(pKDBInfo, pPoints, nPointCount, pcComment)   { ET9_UNUSED(pKDBInfo); ET9_UNUSED(pPoints); ET9_UNUSED(nPointCount); ET9_UNUSED(pcComment); }

#endif

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pTp1                      .
 * @param pTp2                      .
 *
 * @return Xxx
 */

ET9INLINE static ET9FLOAT ET9LOCALCALL __TraceDistance (ET9TracePoint_f const * const pTp1,
                                                        ET9TracePoint_f const * const pTp2)
{
    const ET9FLOAT fL1 = pTp1->fX - pTp2->fX;
    const ET9FLOAT fL2 = pTp1->fY - pTp2->fY;

    ET9FLOAT fDist = _ET9sqrt_f(fL1*fL1 + fL2*fL2);

    return fDist;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pTp1                      .
 * @param pTp2                      .
 *
 * @return Xxx
 */

ET9INLINE static ET9FLOAT ET9LOCALCALL __TraceDistanceSQ (ET9TracePoint_f const * const pTp1,
                                                          ET9TracePoint_f const * const pTp2)
{
    const ET9FLOAT fL1 = pTp1->fX - pTp2->fX;
    const ET9FLOAT fL2 = pTp1->fY - pTp2->fY;

    ET9FLOAT fDistSQ = (ET9FLOAT)(fL1*fL1 + fL2*fL2);

    return fDistSQ;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pTp1                      .
 * @param pTp2                      .
 * @param pTp3                      .
 *
 * @return Xxx
 */

ET9INLINE static ET9FLOAT ET9LOCALCALL __TraceMidAngle (ET9TracePoint_f const * const pTp1,
                                                        ET9TracePoint_f const * const pTp2,
                                                        ET9TracePoint_f const * const pTp3)
{
    return __LinesAngle(pTp2, pTp1, pTp2, pTp3);
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pKDBInfo                  .
 * @param fFactor                   .
 *
 * @return Xxx
 */

static ET9FLOAT ET9LOCALCALL __TraceCalculateLevelOfDetailKDB (ET9KDBInfo     const * const pKDBInfo,
                                                               const ET9FLOAT               fFactor)
{
    ET9KdbLayoutInfo * const pLayoutInfo = pKDBInfo->Private.pCurrLayoutInfo;

    const ET9FLOAT fKeyWidth = (ET9FLOAT)pLayoutInfo->nMedianKeyWidth;
    const ET9FLOAT fKeyHeight = (ET9FLOAT)pLayoutInfo->nMedianKeyHeight;

    const ET9FLOAT fMinSide = (ET9FLOAT)__ET9Min(fKeyWidth, fKeyHeight);

    const ET9FLOAT fMinDetail = (ET9FLOAT)(fMinSide / fFactor);

    return fMinDetail;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pKDBInfo                  .
 * @param fFactor                   .
 *
 * @return Xxx
 */

static ET9FLOAT ET9LOCALCALL __TraceCalculateLevelOfDetailIntegration (ET9KDBInfo     const * const pKDBInfo,
                                                                       const ET9FLOAT               fFactor)
{
    ET9KdbLayoutInfo * const pLayoutInfo = pKDBInfo->Private.pCurrLayoutInfo;

    const ET9FLOAT fKeyWidthScale = (ET9FLOAT)(!pKDBInfo->Private.wScaleToLayoutWidth ? 1.0f : (ET9FLOAT)pKDBInfo->Private.wScaleToLayoutWidth / (ET9FLOAT)pKDBInfo->Private.pCurrLayoutInfo->wLayoutWidth);
    const ET9FLOAT fKeyHeightScale = (ET9FLOAT)(!pKDBInfo->Private.wScaleToLayoutHeight ? 1.0f : (ET9FLOAT)pKDBInfo->Private.wScaleToLayoutHeight / (ET9FLOAT)pKDBInfo->Private.pCurrLayoutInfo->wLayoutHeight);

    const ET9FLOAT fKeyWidth = (ET9FLOAT)pLayoutInfo->nMedianKeyWidth * fKeyWidthScale;
    const ET9FLOAT fKeyHeight = (ET9FLOAT)pLayoutInfo->nMedianKeyHeight * fKeyHeightScale;

    const ET9FLOAT fMinSide = (ET9FLOAT)__ET9Min(fKeyWidth, fKeyHeight);

    const ET9FLOAT fMinDetail = (ET9FLOAT)(fMinSide / fFactor);

    return fMinDetail;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pPoint                    .
 * @param nQuality                  .
 *
 * @return Xxx
 */

ET9INLINE static void ET9LOCALCALL __TraceInitPoint (ET9TracePointExt     * const pPoint,
                                                     const ET9UINT                nQuality)
{
    pPoint->fAngle = 0;
    pPoint->fDist = 0;
    pPoint->wKey = ET9_KDB_KEY_UNDEFINED;
    pPoint->sSymb = 0;
    pPoint->nPos = 0xFFFF;
    pPoint->nDensity = 1;
    pPoint->nQuality = nQuality;
    pPoint->bQualityId = bUndefQualityID;
    pPoint->bRemoved = 0;
    pPoint->bMergeWithPrev = 0;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pKDBInfo                  .
 * @param pPoints                   .
 * @param nPointCount               .
 *
 * @return Xxx
 */

static void ET9LOCALCALL __TraceInitExt (ET9KDBInfo     const * const pKDBInfo,
                                         ET9TracePointExt     * const pPoints,
                                         const ET9UINT                nPointCount)
{
    ET9UINT nIndex;

    WLOG6(fprintf(pLogFile6, "\n__TraceInitExt\n");)

    for (nIndex = 0; nIndex < nPointCount; ++nIndex) {
        __TraceInitPoint(&pPoints[nIndex], 1);
    }

    __TraceLog(pKDBInfo, pPoints, nPointCount, "after-init");
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pKDBInfo                  .
 * @param pPoint                    .
 *
 * @return Xxx
 */

ET9INLINE static ET9BOOL ET9LOCALCALL __TraceIsPointInsideIntegrationArea_f (ET9KDBInfo       const * const pKDBInfo,
                                                                             ET9TracePoint_f  const * const pPoint)
{
    const ET9FLOAT fWidth = (ET9FLOAT)(pKDBInfo->Private.wScaleToLayoutWidth ? pKDBInfo->Private.wScaleToLayoutWidth :  pKDBInfo->Private.pCurrLayoutInfo->wLayoutWidth);
    const ET9FLOAT fHeight = (ET9FLOAT)(pKDBInfo->Private.wScaleToLayoutHeight ? pKDBInfo->Private.wScaleToLayoutHeight :  pKDBInfo->Private.pCurrLayoutInfo->wLayoutHeight);

    const ET9FLOAT fULX = (ET9FLOAT)pKDBInfo->Private.wLayoutOffsetX;
    const ET9FLOAT fULY = (ET9FLOAT)pKDBInfo->Private.wLayoutOffsetY;

    const ET9FLOAT fLRX = fULX + fWidth - 1.0f;
    const ET9FLOAT fLRY = fULY + fHeight - 1.0f;

    if (pPoint->fX < fULX || pPoint->fX > fLRX) {
        return 0;
    }

    if (pPoint->fY < fULY || pPoint->fY > fLRY) {
        return 0;
    }

    return 1;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pKDBInfo                  .
 * @param pPoint                    .
 *
 * @return Xxx
 */

ET9INLINE static ET9BOOL ET9LOCALCALL __TraceIsPointInsideIntegrationArea (ET9KDBInfo     const * const pKDBInfo,
                                                                           ET9TracePoint  const * const pPoint)
{
    ET9TracePoint_f sPointF;

    sPointF.fX = (ET9FLOAT)pPoint->nX;
    sPointF.fY = (ET9FLOAT)pPoint->nY;

    return __TraceIsPointInsideIntegrationArea_f(pKDBInfo, &sPointF);
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pKDBInfo                  .
 * @param pdfX                       .
 * @param pdfY                       .
 *
 * @return Xxx
 */

ET9INLINE static void ET9LOCALCALL __TraceAssureCoordinateInsideIntegrationArea (ET9KDBInfo   const * const pKDBInfo,
                                                                                 ET9DOUBLE          * const pdfX,
                                                                                 ET9DOUBLE          * const pdfY)
{
    const ET9DOUBLE dfMinX = pKDBInfo->Private.wLayoutOffsetX;
    const ET9DOUBLE dfMinY = pKDBInfo->Private.wLayoutOffsetY;

    const ET9DOUBLE dfMaxX = dfMinX + (pKDBInfo->Private.wScaleToLayoutWidth  ? pKDBInfo->Private.wScaleToLayoutWidth  : pKDBInfo->Private.pCurrLayoutInfo->wLayoutWidth) - 1;
    const ET9DOUBLE dfMaxY = dfMinY + (pKDBInfo->Private.wScaleToLayoutHeight ? pKDBInfo->Private.wScaleToLayoutHeight : pKDBInfo->Private.pCurrLayoutInfo->wLayoutHeight) - 1;

    ET9DOUBLE dfX = *pdfX;
    ET9DOUBLE dfY = *pdfY;

    if (dfX < dfMinX) {
        dfX = dfMinX;
    }

    if (dfX > dfMaxX) {
        dfX = dfMaxX;
    }

    if (dfY < dfMinY) {
        dfY = dfMinY;
    }

    if (dfY > dfMaxY) {
        dfY = dfMaxY;
    }

    if (dfX == *pdfX && dfY == *pdfY) {
        return;
    }

    WLOG6(fprintf(pLogFile6, "    assure inside intgration - x %5.1f (%5.1f) y %5.1f (%5.1f)\n", dfX, *pdfX, dfY, *pdfY);)

    *pdfX = dfX;
    *pdfY = dfY;
}

#if 0

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pPoint1                   .
 * @param pPoint2                   .
 * @param pPoint3                   .
 * @param pPoint4                   .
 * @param pPointIntersect           .
 *
 * @return Xxx
 */

ET9INLINE static ET9BOOL ET9LOCALCALL __CalculateLineLineIntersection (ET9TracePoint_f    const * const pPoint1,
                                                                       ET9TracePoint_f    const * const pPoint2,
                                                                       ET9TracePoint_f    const * const pPoint3,
                                                                       ET9TracePoint_f    const * const pPoint4,
                                                                       ET9TracePoint_f          * const pPointIntersect)
{
    const ET9FLOAT fUNum = (pPoint4->fX - pPoint3->fX) * (pPoint1->fY - pPoint3->fY) - (pPoint4->fY - pPoint3->fY) * (pPoint1->fX - pPoint3->fX);
    const ET9FLOAT fUDen = (pPoint4->fY - pPoint3->fY) * (pPoint2->fX - pPoint1->fX) - (pPoint4->fX - pPoint3->fX) * (pPoint2->fY - pPoint1->fY);

    if (fUNum == 0.0f || fUDen == 0.0f) {
        return 0;
    }

    {
        const ET9FLOAT fU = fUNum / fUDen;

        const ET9FLOAT fX = pPoint1->fX + fU * (pPoint2->fX - pPoint1->fX);
        const ET9FLOAT fY = pPoint1->fY + fU * (pPoint2->fY - pPoint1->fY);

        pPointIntersect->fX = fX;
        pPointIntersect->fY = fY;
    }

    return 1;
}

#endif

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pKDBInfo                  .
 * @param pPointsDst                .
 * @param pnDstCount                .
 * @param nDstSize                  .
 * @param pPointsSrc                .
 * @param nSrcCount                 .
 *
 * @return Xxx
 */

static ET9STATUS ET9LOCALCALL __TraceCopyToExt (ET9KDBInfo           * const pKDBInfo,
                                                ET9TracePointExt     * const pPointsDst,
                                                ET9UINT              * const pnDstCount,
                                                const ET9UINT                nDstSize,
                                                ET9TracePoint  const * const pPointsSrc,
                                                const ET9UINT                nSrcCount)
{
    const ET9DOUBLE dfMaxDeviation = ET9_PRIVATE_TRACE_FILTER_DEVC;   /* total = 2x */
    const ET9DOUBLE dfMaxDeviationAdjustmentSQ = (dfMaxDeviation * 1.1) * (dfMaxDeviation * 1.1);

    WLOG6(fprintf(pLogFile6, "__TraceCopyToExt, nSrcCount %u, nDstSize %u, fMaxDeviation %5.1f\n", nSrcCount, nDstSize, dfMaxDeviation);)

    if (!nSrcCount) {
        WLOG6(fprintf(pLogFile6, "  empty trace\n");)
        *pnDstCount = 0;
        return ET9STATUS_NONE;
    }

    if (nDstSize < 10) {
        WLOG6(fprintf(pLogFile6, "  too limited destination\n");)
        *pnDstCount = 0;
        return ET9STATUS_ERROR;
    }

    WLOG6({
        ET9UINT nIndex;

        fprintf(pLogFile6, "\n");
        fprintf(pLogFile6, "const ET9TracePoint pRawTraceData[] =\n");
        fprintf(pLogFile6, "{\n");

        for (nIndex = 0; nIndex < nSrcCount; ++nIndex) {

            fprintf(pLogFile6, "    { %3u, %3u }", pPointsSrc[nIndex].nX, pPointsSrc[nIndex].nY);

            if (nIndex + 1 < nSrcCount) {
                fprintf(pLogFile6, ",");
            }

            fprintf(pLogFile6, "\n");
        }

        fprintf(pLogFile6, "};\n");
        fprintf(pLogFile6, "\n");
    })

    WLOG6(fprintf(pLogFile6, "  First point, x %3u, y %3u\n", pPointsSrc[0].nX, pPointsSrc[0].nY);)
    WLOG6(fprintf(pLogFile6, "  Last point,  x %3u, y %3u\n", pPointsSrc[nSrcCount - 1].nX, pPointsSrc[nSrcCount - 1].nY);)

    {
        ET9UINT nDstCount;

        nDstCount = 0;

        /* filter and copy */

        {
            ET9UINT nIndex;

            for (nIndex = 0; nIndex < nSrcCount; ++nIndex) {

                WLOG6(fprintf(pLogFile6, "  [%3u] - source index\n", nIndex);)

                if (nDstCount + 1 > nDstSize) {
                    WLOG6(fprintf(pLogFile6, "    too much trace info, skipping the rest\n");)
                    break;
                }

                if (!nIndex && nSrcCount < 3) {

                    WLOG6(fprintf(pLogFile6, "    adding first %3u - less than 3 total\n", nDstCount);)

                    pPointsDst[nDstCount].sPoint.fX = (ET9FLOAT)pPointsSrc[nIndex].nX;
                    pPointsDst[nDstCount].sPoint.fY = (ET9FLOAT)pPointsSrc[nIndex].nY;
                    ++nDstCount;
                    continue;
                }

                if (nSrcCount - nIndex < 2) {

                    WLOG6(fprintf(pLogFile6, "    adding last %3u - less than 2 left\n", nDstCount);)

                    pPointsDst[nDstCount].sPoint.fX = (ET9FLOAT)pPointsSrc[nIndex].nX;
                    pPointsDst[nDstCount].sPoint.fY = (ET9FLOAT)pPointsSrc[nIndex].nY;
                    ++nDstCount;
                    continue;
                }

                /* at least 3 points to work with here */

                {
                    ET9DOUBLE dfFirstX;
                    ET9DOUBLE dfFirstY;

                    ET9DOUBLE dfCount = 0.0f;
                    ET9DOUBLE dfSumX = 0.0f;
                    ET9DOUBLE dfSumY = 0.0f;
                    ET9DOUBLE dfSumXY = 0.0f;
                    ET9DOUBLE dfSumXSQ = 0.0f;

                    ET9UINT nLookBest = 0;

                    ET9DOUBLE dfCountBest = 0.0f;
                    ET9DOUBLE dfSumXBest = 0.0f;
                    ET9DOUBLE dfSumYBest = 0.0f;
                    ET9DOUBLE dfSumXYBest = 0.0f;
                    ET9DOUBLE dfSumXSQBest = 0.0f;

                    ET9DOUBLE dfCurrLengthSQ = 0.0f;

                    ET9BOOL bCurrValidPoint = __TraceIsPointInsideIntegrationArea(pKDBInfo, &pPointsSrc[nIndex]);

                    /* including previous point */

                    if (nIndex && nDstCount) {

                        WLOG6(fprintf(pLogFile6, "    including previous dst %u\n", nDstCount - 1);)

                        dfFirstX = pPointsDst[nDstCount - 1].sPoint.fX;
                        dfFirstY = pPointsDst[nDstCount - 1].sPoint.fY;

                        dfCount += 1.0f;

                        dfSumX += dfFirstX;
                        dfSumY += dfFirstY;
                        dfSumXY += dfFirstX * dfFirstY;
                        dfSumXSQ += dfFirstX * dfFirstX;
                    }
                    else {

                        dfFirstX = (ET9DOUBLE)pPointsSrc[0].nX;
                        dfFirstY = (ET9DOUBLE)pPointsSrc[0].nY;
                    }

                    {
                        ET9UINT nLook;

                        for (nLook = nIndex; nLook < nSrcCount; ++nLook) {

                            const ET9DOUBLE dfX = (ET9FLOAT)pPointsSrc[nLook].nX;
                            const ET9DOUBLE dfY = (ET9FLOAT)pPointsSrc[nLook].nY;

                            const ET9DOUBLE dfLenSQ = (dfX - dfFirstX) * (dfX - dfFirstX) + (dfY - dfFirstY) * (dfY - dfFirstY);

                            const ET9BOOL bValidPoint = __TraceIsPointInsideIntegrationArea(pKDBInfo, &pPointsSrc[nLook]);

                            WLOG6(fprintf(pLogFile6, "    {%3u} - look index - x %3.0f y %3.0f valid %u len %7.3f (%7.3f)\n", nLook, dfX, dfY, bValidPoint, _ET9sqrt_f((ET9FLOAT)dfLenSQ), dfLenSQ);)

                            if (dfLenSQ < dfCurrLengthSQ) {
                                WLOG6(fprintf(pLogFile6, "      shorter length - breaking (%7.3f < %7.3f)\n", _ET9sqrt_f((ET9FLOAT)dfLenSQ), _ET9sqrt_f((ET9FLOAT)dfCurrLengthSQ));)
                                break;
                            }

                            if (nLook > nIndex) {

                                const ET9DOUBLE dfPrevX = (ET9DOUBLE)pPointsSrc[nLook - 1].nX;
                                const ET9DOUBLE dfPrevY = (ET9DOUBLE)pPointsSrc[nLook - 1].nY;

                                const ET9DOUBLE dfLenToPrevSQ = (dfX - dfPrevX) * (dfX - dfPrevX) + (dfY - dfPrevY) * (dfY - dfPrevY);

                                if (dfLenToPrevSQ > dfLenSQ) {
                                    WLOG6(fprintf(pLogFile6, "      direction change - breaking (%7.3f > %7.3f)\n", _ET9sqrt_f((ET9FLOAT)dfLenToPrevSQ), _ET9sqrt_f((ET9FLOAT)dfLenSQ));)
                                    break;
                                }
                            }

                            dfCurrLengthSQ = dfLenSQ;

                            if (bCurrValidPoint && !bValidPoint) {
                                WLOG6(fprintf(pLogFile6, "     leaving integration area - breaking\n");)
                                break;
                            }

                            dfCount += 1.0;
                            dfSumX += dfX;
                            dfSumY += dfY;
                            dfSumXY += dfX * dfY;
                            dfSumXSQ += dfX * dfX;

                            {

                                ET9DOUBLE dfMinSqDiv = dfCount * dfSumXSQ - dfSumX * dfSumX;

                                if (nLook == nIndex) {
                                    WLOG6(fprintf(pLogFile6, "      skipping deviation check (nLook %3u nIndex %3u) - but ok\n", nLook, nIndex);)
                                }

                                else if (dfMinSqDiv == 0.0) {
                                    WLOG6(fprintf(pLogFile6, "      skipping deviation check (fMinSqDiv == 0) - but ok\n");)
                                }

                                else {

                                    const ET9DOUBLE dfMinSqA = (dfSumY  * dfSumXSQ - dfSumX * dfSumXY) / dfMinSqDiv;
                                    const ET9DOUBLE dfMinSqB = (dfCount * dfSumXY  - dfSumX * dfSumY)  / dfMinSqDiv;

                                    const ET9DOUBLE dfDistA = dfMinSqB;
                                    const ET9DOUBLE dfDistB = -1.0;
                                    const ET9DOUBLE dfDistC = dfMinSqA;
                                    const ET9DOUBLE dfDistDiv = _ET9sqrt_f((ET9FLOAT)(dfDistA * dfDistA + dfDistB * dfDistB));

                                    if (dfDistDiv == 0.0) {
                                        WLOG6(fprintf(pLogFile6, "      skipping deviation check (fDistDiv == 0) - should not really happen\n");)
                                        continue;
                                    }

                                    /* check first point */

                                    {
                                        const ET9DOUBLE dfDeviation = __ET9Abs(dfDistA * dfFirstX + dfDistB * dfFirstY + dfDistC) / dfDistDiv;

                                        if (dfDeviation > dfMaxDeviation) {
                                            WLOG6(fprintf(pLogFile6, "      first point deviates too much (%7.3f > %7.3f)\n", dfDeviation, dfMaxDeviation);)
                                            continue;
                                        }
                                    }

                                    /* check all real points */

                                    {
                                        ET9UINT nCheck;

                                        for (nCheck = nIndex; nCheck <= nLook; ++nCheck) {

                                            const ET9DOUBLE dfX = (ET9FLOAT)pPointsSrc[nCheck].nX;
                                            const ET9DOUBLE dfY = (ET9FLOAT)pPointsSrc[nCheck].nY;

                                            {
                                                const ET9DOUBLE dfDeviation = __ET9Abs(dfDistA * dfX + dfDistB * dfY + dfDistC) / dfDistDiv;

                                                if (dfDeviation > dfMaxDeviation) {
                                                    WLOG6(fprintf(pLogFile6, "      point %u (%u) deviates too much (%7.3f > %7.3f)\n", nCheck - nIndex, nLook, dfDeviation, dfMaxDeviation);)
                                                    break;
                                                }
                                            }
                                        }

                                        if (nCheck <= nLook) {
                                            WLOG6(fprintf(pLogFile6, "      real point deviates too much\n");)
                                            break;  /* 'continue' if exploring further could yield a better fit */
                                        }
                                    }
                                }

                                /* new best path */

                                WLOG6(fprintf(pLogFile6, "      new best path\n");)

                                nLookBest = nLook;

                                dfCountBest = dfCount;
                                dfSumXBest = dfSumX;
                                dfSumYBest = dfSumY;
                                dfSumXYBest = dfSumXY;
                                dfSumXSQBest = dfSumXSQ;
                            }

                            if (!bCurrValidPoint && bValidPoint) {
                                WLOG6(fprintf(pLogFile6, "      entering integration area - keep + breaking\n");)
                                break;
                            }
                        }
                    }

                    /* add point(s) */

                    if (nLookBest >= nIndex) {

                        /*
                           The goal here is to adjust the ends of the found line segment (end points), so they are on the calculated line.
                           If it's the first segment both points are new from "src", otherwise the first points is the last added one.
                           Even if the point is the last added one it needs to be adjusted according to the new segment to get the proper value.
                           The previously added point can then be adjusted to the new segment intersection point.
                        */

                        const ET9DOUBLE dfMinSqDiv = dfCountBest * dfSumXSQBest - dfSumXBest * dfSumXBest;

                        const ET9DOUBLE dfMinSqA = (dfMinSqDiv == 0.0) ? 0.0 : (dfSumYBest  * dfSumXSQBest - dfSumXBest * dfSumXYBest) / dfMinSqDiv;
                        const ET9DOUBLE dfMinSqB = (dfMinSqDiv == 0.0) ? 0.0 : (dfCountBest * dfSumXYBest  - dfSumXBest * dfSumYBest)  / dfMinSqDiv;

                        ET9TracePoint_f pAdjustedPoints[2];

                        ET9UINT nAdjustIndex;

                        for (nAdjustIndex = 0; nAdjustIndex < 2; ++nAdjustIndex) {

                            const ET9UINT nSrcIndex = nAdjustIndex ? nLookBest : 0;
                            const ET9BOOL bUseSrcIndex = (nAdjustIndex || !nDstCount) ? 1 : 0;

                            const ET9DOUBLE dfX = bUseSrcIndex ? (ET9DOUBLE)pPointsSrc[nSrcIndex].nX : pPointsDst[nDstCount - 1].sPoint.fX;
                            const ET9DOUBLE dfY = bUseSrcIndex ? (ET9DOUBLE)pPointsSrc[nSrcIndex].nY : pPointsDst[nDstCount - 1].sPoint.fY;

                            ET9DOUBLE dfAdjustedX = (dfMinSqB == 0.0) ? dfX : ((dfY + dfX / dfMinSqB - dfMinSqA) / (dfMinSqB + 1.0 / dfMinSqB));
                            ET9DOUBLE dfAdjustedY = (dfMinSqB == 0.0) ? dfY : (dfY + dfX / dfMinSqB - dfAdjustedX / dfMinSqB);

                            if (dfMinSqDiv == 0.0) {

                                WLOG6(fprintf(pLogFile6, "    fMinSqDiv == 0 - can't adjust points\n");)

                                dfAdjustedX = dfX;
                                dfAdjustedY = dfY;
                            }
                            else if (dfMinSqB == 0.0) {

                                WLOG6(fprintf(pLogFile6, "    fMinSqB == 0 - can't adjust points\n");)

                                dfAdjustedX = dfX;
                                dfAdjustedY = dfY;
                            }
                            else if ((((dfX - dfAdjustedX)) * (dfX - dfAdjustedX) + ((dfY - dfAdjustedY) * (dfY - dfAdjustedY))) >= dfMaxDeviationAdjustmentSQ) {

                                WLOG6(fprintf(pLogFile6, "    HUGE ADJUSTMENT - reverting\n");)

                                dfAdjustedX = dfX;
                                dfAdjustedY = dfY;
                            }

                            if ( bUseSrcIndex && __TraceIsPointInsideIntegrationArea(pKDBInfo, &pPointsSrc[nSrcIndex]) ||
                                !bUseSrcIndex && __TraceIsPointInsideIntegrationArea_f(pKDBInfo, &pPointsDst[nDstCount - 1].sPoint)) {
                                __TraceAssureCoordinateInsideIntegrationArea(pKDBInfo, &dfAdjustedX, &dfAdjustedY);
                            }
                            else {
                                WLOG6(fprintf(pLogFile6, "    not assuring inside integration area\n");)
                            }

                            WLOG6(fprintf(pLogFile6, "    adjusted point %u (%3u) - x %7.3f (%7.3f) y %7.3f (%7.3f) dist %7.3f\n", nAdjustIndex, (bUseSrcIndex ? nSrcIndex : nIndex - 1), dfAdjustedX, dfX, dfAdjustedY, dfY, _ET9sqrt_f((ET9FLOAT)((dfX - dfAdjustedX) * (dfY - dfAdjustedY))));)

                            pAdjustedPoints[nAdjustIndex].fX = (ET9FLOAT)dfAdjustedX;
                            pAdjustedPoints[nAdjustIndex].fY = (ET9FLOAT)dfAdjustedY;
                        }

                        /* transfer adjusted points - first */

                        if (!nIndex && nLookBest) {

                            WLOG6(fprintf(pLogFile6, "    adding point %3u - x %7.3f y %7.3f\n", nDstCount, pAdjustedPoints[0].fX, pAdjustedPoints[0].fY);)

                            pPointsDst[nDstCount++].sPoint = pAdjustedPoints[0];
                        }
                        else if (nDstCount >= 2) {

                            ET9TracePoint_f sIntersectionPoint;

                            sIntersectionPoint.fX = (pPointsDst[nDstCount - 1].sPoint.fX + pAdjustedPoints[0].fX) / 2.0f;
                            sIntersectionPoint.fY = (pPointsDst[nDstCount - 1].sPoint.fY + pAdjustedPoints[0].fY) / 2.0f;

                            WLOG6(fprintf(pLogFile6, "    intersection point %3u - x %7.3f (%7.3f %7.3f) y %7.3f (%7.3f %7.3f) - dist %7.3f\n", nDstCount - 1, sIntersectionPoint.fX, pPointsDst[nDstCount - 1].sPoint.fX, pAdjustedPoints[0].fX, sIntersectionPoint.fY, pPointsDst[nDstCount - 1].sPoint.fY, pAdjustedPoints[0].fY, __TraceDistance(&sIntersectionPoint, &pPointsDst[nDstCount - 1].sPoint));)

                            pPointsDst[nDstCount - 1].sPoint = sIntersectionPoint;
                        }

                        /* transfer adjusted points - second */

                        WLOG6(fprintf(pLogFile6, "    adding point %3u - x %7.3f y %7.3f\n", nDstCount, pAdjustedPoints[1].fX, pAdjustedPoints[1].fY);)

                        pPointsDst[nDstCount++].sPoint = pAdjustedPoints[1];

                        /* continue after look index */

                        nIndex = nLookBest;
                    }
                    else {
                        WLOG6(fprintf(pLogFile6, "    no best - no points to add\n");)
                        ET9Assert(0);
                    }
                }
            }
        }

        /* potential line segmentt access */

#ifdef ET9_PRIVATE_TRACE_LINE_SEGMENT_ACCESS
        {
            ET9UINT nIndex;

            for (nIndex = 0; nIndex < nDstCount; ++nIndex) {
                pTraceLineSegmentPoints[nIndex].nX = (ET9UINT)(pPointsDst[nIndex].sPoint.fX + 0.5f);
                pTraceLineSegmentPoints[nIndex].nY = (ET9UINT)(pPointsDst[nIndex].sPoint.fY + 0.5f);
            }

            nTraceLineSegmentCount = nDstCount;
        }
#endif

        /* scale to KDB */

        {
            ET9UINT nCount;
            ET9TracePointExt *pPointExt;

            pPointExt = pPointsDst;
            for (nCount = nDstCount; nCount; --nCount, ++pPointExt) {

                const ET9FLOAT fScaledX = __ScaleCoordinateToKdbX_f(pKDBInfo, pPointExt->sPoint.fX);
                const ET9FLOAT fScaledY = __ScaleCoordinateToKdbY_f(pKDBInfo, pPointExt->sPoint.fY);

                WLOG6(fprintf(pLogFile6, "  scaling point %3u - x %7.3f (%7.3f) y %7.3f (%7.3f)\n", nDstCount - nCount, fScaledX, pPointExt->sPoint.fX, fScaledY, pPointExt->sPoint.fY);)

                pPointExt->sPoint.fX = fScaledX;
                pPointExt->sPoint.fY = fScaledY;
            }
        }

        *pnDstCount = nDstCount;
    }

    ET9Assert(*pnDstCount <= ET9_TRACE_MAX_POINTS);

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pPointsDst                .
 * @param pnDstCount                .
 * @param pPointsSrc                .
 * @param nSrcCount                 .
 *
 * @return Xxx
 */

static void ET9LOCALCALL __TraceCopyExt (ET9TracePointExt       * const pPointsDst,
                                         ET9UINT                * const pnDstCount,
                                         ET9TracePointExt const * const pPointsSrc,
                                         const ET9UINT                  nSrcCount)
{
    ET9UINT nIndex;

    for (nIndex = 0; nIndex < nSrcCount; ++nIndex) {
        pPointsDst[nIndex] = pPointsSrc[nIndex];
    }

    if (pnDstCount) {
        *pnDstCount = nSrcCount;
    }
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pPoints                   .
 * @param nPointCount               .
 *
 * @return Xxx
 */

static void ET9LOCALCALL __TraceSetSparsePos (ET9TracePointExt   * const pPoints,
                                              const ET9UINT              nPointCount)
{
    ET9UINT nIndex;

    for (nIndex = 0; nIndex < nPointCount; ++nIndex) {
        pPoints[nIndex].nPos = (nIndex + 1) * 100;
    }
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pPoints                   .
 * @param nPointCount               .
 *
 * @return Xxx
 */

static void ET9LOCALCALL __TracePosSortTrace (ET9TracePointExt   * const pPoints,
                                              const ET9UINT              nPointCount)
{
    ET9BOOL bDone;
    ET9UINT nIndex;

    for (bDone = 0; !bDone; ) {

        bDone = 1;

        for (nIndex = 0; nIndex + 1 < nPointCount; ++nIndex) {

            if (pPoints[nIndex].nPos > pPoints[nIndex + 1].nPos) {

                ET9TracePointExt sTmp = pPoints[nIndex];
                pPoints[nIndex] = pPoints[nIndex + 1];
                pPoints[nIndex + 1] = sTmp;

                bDone = 0;
            }
        }
    }
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pPoints                   .
 * @param pnPointCount              .
 *
 * @return Xxx
 */

ET9INLINE static ET9UINT ET9LOCALCALL __TraceAddPoint (ET9TracePointExt   * const pPoints,
                                                       ET9UINT            * const pnPointCount)
{
    if (*pnPointCount >= ET9_TRACE_MAX_POINTS) {
        return *pnPointCount;
    }

    __TraceInitPoint(&pPoints[*pnPointCount], 0);

    return (*pnPointCount)++;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pPoints                   .
 * @param pnPointCount              .
 * @param nIndex                    .
 * @param nCount                    .
 *
 * @return Xxx
 */

ET9INLINE static void ET9LOCALCALL __TraceRemovePoint (ET9TracePointExt   * const pPoints,
                                                       ET9UINT            * const pnPointCount,
                                                       const ET9UINT              nIndex,
                                                       const ET9UINT              nCount)
{
    if (nIndex >= *pnPointCount) {
        return;
    }

    __TraceCopyExt(&pPoints[nIndex], NULL, &pPoints[nIndex + nCount], (*pnPointCount - nIndex - nCount));

    *pnPointCount -= nCount;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pPoints                   .
 * @param pnPointCount              .
 *
 * @return Xxx
 */

static void ET9LOCALCALL __TraceRemoveDeleted (ET9TracePointExt   * const pPoints,
                                               ET9UINT            * const pnPointCount)
{
    ET9UINT nIndex;

    for (nIndex = 0; nIndex < *pnPointCount; ++nIndex) {

        ET9UINT nLook;

        if (!pPoints[nIndex].bRemoved) {
            continue;
        }

        for (nLook = nIndex + 1; nLook < *pnPointCount; ++nLook) {

            if (!pPoints[nLook].bRemoved) {
                break;
            }
        }

        __TraceRemovePoint(pPoints, pnPointCount, nIndex, nLook - nIndex);
    }
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pPoints                   .
 * @param nPointCount               .
 * @param nIndex                    .
 * @param nCount                    .
 * @param bKeepHighQuality          .
 *
 * @return Xxx
 */

static void ET9LOCALCALL __TraceClusterPoints (ET9TracePointExt     * const pPoints,
                                               const ET9UINT                nPointCount,
                                               const ET9UINT                nIndex,
                                               const ET9UINT                nCount,
                                               const ET9BOOL                bKeepHighQuality)
{
    ET9UINT nLook;

    if (nCount < 2) {
        return;
    }

    /* different approach depending on if keeping high quality points */

    if (bKeepHighQuality) {

        for (nLook = nIndex; nLook < nIndex + nCount; ++nLook) {
            if (pPoints[nLook].nQuality >= nHighQuality) {
                break;
            }
        }

        /* will keep all high's and discard the rest */

        if (nLook < nIndex + nCount) {

            for (nLook = nIndex; nLook < nIndex + nCount; ++nLook) {
                if (pPoints[nLook].nQuality < nHighQuality) {
                    pPoints[nLook].bRemoved = 1;
                }
            }

            /* done */

            return;
        }
    }

    /* no high quality points or not keeping them */

    {
        ET9UINT             nTotQuality = pPoints[nIndex].nQuality;
        ET9UINT             nAverageCount = pPoints[nIndex].nDensity;
        ET9TracePoint_f     sAveragePoint = pPoints[nIndex].sPoint;

        sAveragePoint.fX *= nAverageCount;
        sAveragePoint.fY *= nAverageCount;

        for (nLook = nIndex + 1; nLook < nIndex + nCount; ++nLook) {
            pPoints[nLook].bRemoved = 1;
            nTotQuality += pPoints[nLook].nQuality;
            nAverageCount += pPoints[nLook].nDensity;
            sAveragePoint.fX += pPoints[nLook].sPoint.fX * pPoints[nLook].nDensity;
            sAveragePoint.fY += pPoints[nLook].sPoint.fY * pPoints[nLook].nDensity;
        }

        sAveragePoint.fX /= nAverageCount;
        sAveragePoint.fY /= nAverageCount;

        /* don't cluster away first/last coordinates */

        {
            ET9UINT nKeepIndex;

            if (!nIndex) {
                nKeepIndex = nIndex;
            }
            else if (nIndex + nCount == nPointCount) {
                nKeepIndex = nIndex + nCount - 1;
                pPoints[nIndex].bRemoved = 1;
                pPoints[nKeepIndex].bRemoved = 0;
            }
            else {
                nKeepIndex = nIndex;
                pPoints[nKeepIndex].sPoint = sAveragePoint;
            }

            ET9Assert(!pPoints[nKeepIndex].bRemoved);

            pPoints[nKeepIndex].nDensity = nAverageCount;
            pPoints[nKeepIndex].nQuality = nTotQuality;
        }
    }
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pKDBInfo                  .
 * @param pPoints                   .
 * @param pnPointCount              .
 *
 * @return Xxx
 */

static void ET9LOCALCALL __TraceTidyQualityIds (ET9KDBInfo       const * const pKDBInfo,
                                                ET9TracePointExt       * const pPoints,
                                                ET9UINT                * const pnPointCount)
{
    ET9UINT nIndex;

    ET9U8 bPrevQualityID = bUndefQualityID;

    WLOG6(fprintf(pLogFile6, "\n__TraceTidyQualityIds\n");)

    for (nIndex = 0; nIndex + 1 < *pnPointCount; ++nIndex) {

        ET9UINT nLook;

        if (pPoints[nIndex].bQualityId == bUndefQualityID) {
            continue;
        }

        if (pPoints[nIndex].bQualityId == bPrevQualityID) {
            continue;
        }

        bPrevQualityID = pPoints[nIndex].bQualityId;

        for (nLook = nIndex + 1; nLook < *pnPointCount; ++nLook) {

            if (pPoints[nLook].bQualityId != pPoints[nIndex].bQualityId) {
                break;
            }
        }

        /* handle one ID */

        {
            const ET9UINT nEnd = nLook;

            ET9FLOAT fBestAngle = -1.0f;
            ET9UINT nBestIndex = 0xFFFF;

            if ((nEnd - nIndex) < 2) {
                continue;
            }

            /* find best point */

            for (nLook = nIndex; nLook < nEnd; ++nLook) {

                const ET9FLOAT fAngle = __ET9Abs(pPoints[nLook].fAngle);

                if (nLook == nIndex || fBestAngle < fAngle) {
                    nBestIndex = nLook;
                    fBestAngle = fAngle;
                }
            }

            WLOG6(fprintf(pLogFile6, "  best %3u %5.1f\n", nBestIndex, fBestAngle);)

            ET9Assert(nBestIndex != 0xFFFF);

            /* assure best is high quality and demote the rest */

            for (nLook = nIndex; nLook < nEnd; ++nLook) {

                if (nLook == nBestIndex) {
                    continue;
                }

                pPoints[nLook].nQuality = 0;
            }

            if (pPoints[nBestIndex].nQuality < nHighQuality) {
                pPoints[nBestIndex].nQuality = nHighQuality;
            }
        }
    }

    __TraceLog(pKDBInfo, pPoints, *pnPointCount, "after-tidy-quality-ids");
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pKDBInfo                  .
 * @param pPoints                   .
 * @param pnPointCount              .
 *
 * @return Xxx
 */

static void ET9LOCALCALL __TraceEndPointQuality (ET9KDBInfo       const * const pKDBInfo,
                                                 ET9TracePointExt       * const pPoints,
                                                 ET9UINT                * const pnPointCount)
{
    WLOG6(fprintf(pLogFile6, "\n__TraceEndPointQuality\n");)

    pPoints[0].nQuality += nHighQuality;

    if (*pnPointCount > 1) {
        pPoints[*pnPointCount - 1].nQuality += nHighQuality;
    }

    __TraceLog(pKDBInfo, pPoints, *pnPointCount, "after-end-point-quality");
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pKDBInfo                  .
 * @param pPoints                   .
 * @param pnPointCount              .
 *
 * @return Xxx
 */

static void ET9LOCALCALL __TraceTidyQualityDistance (ET9KDBInfo       const * const pKDBInfo,
                                                     ET9TracePointExt       * const pPoints,
                                                     ET9UINT                * const pnPointCount)
{
    ET9UINT nIndex;

    const ET9FLOAT fClusterDist = __TraceCalculateLevelOfDetailKDB(pKDBInfo, fLevelOfDetailQualityDistance);

    const ET9FLOAT fClusterDistSQ = (ET9FLOAT)(fClusterDist * fClusterDist);

    WLOG6(fprintf(pLogFile6, "\n__TraceTidyQualityDistance (%5.1f)\n", fClusterDist);)

    /* applicable? */

    if (*pnPointCount < 4) {
        return;
    }

    /* iterate - ignoring end points */

    for (nIndex = 1; nIndex + 2 < *pnPointCount; ++nIndex) {

        ET9UINT nLook;

        if (pPoints[nIndex].nQuality < nHighQuality) {
            continue;
        }

        for (nLook = nIndex + 1; nLook + 1 < *pnPointCount; ++nLook) {

            const ET9FLOAT fDistSQ = __TraceDistanceSQ(&pPoints[nIndex].sPoint, &pPoints[nLook].sPoint);

            if (fDistSQ > fClusterDistSQ) {
                break;
            }

            if (pPoints[nLook].nQuality >= nHighQuality) {

                WLOG6(fprintf(pLogFile6, "  demoting %3u\n", nLook);)

                pPoints[nLook].nQuality = 0;

                break;
            }
        }
    }

    __TraceLog(pKDBInfo, pPoints, *pnPointCount, "after-tidy-quality-points");
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pKDBInfo                  .
 * @param pPoints                   .
 * @param pnPointCount              .
 *
 * @return Xxx
 */

static void ET9LOCALCALL __TraceTidyEndPoints (ET9KDBInfo       const * const pKDBInfo,
                                               ET9TracePointExt       * const pPoints,
                                               ET9UINT                * const pnPointCount)
{
    ET9UINT nIndex;

    const ET9FLOAT fClusterDist = __TraceCalculateLevelOfDetailKDB(pKDBInfo, fLevelOfDetailEndPoint);

    const ET9FLOAT fClusterDistSQ = (ET9FLOAT)(fClusterDist * fClusterDist);

    WLOG6(fprintf(pLogFile6, "\n__TraceTidyEndPoints (%5.1f)\n", fClusterDist);)

    /* applicable? */

    if (*pnPointCount < 3) {
        return;
    }

    /* start point */

    {
        const ET9UINT nEndPoint = 0;

        ET9FLOAT fTotDistSQ = 0;

        for (nIndex = nEndPoint + 1; nIndex + 1 < *pnPointCount; ++nIndex) {

            const ET9FLOAT fDistSQ = __TraceDistanceSQ(&pPoints[nIndex].sPoint, &pPoints[nIndex - 1].sPoint);

            fTotDistSQ += fDistSQ;

            if (fTotDistSQ > fClusterDistSQ) {
                break;
            }

            if (pPoints[nIndex].nQuality < nHighQuality) {
                continue;
            }

            WLOG6(fprintf(pLogFile6, "  start point, demoting %3u\n", nIndex);)

            pPoints[nIndex].nQuality = 0;
        }
    }

    /* end point */

    {
        const ET9UINT nEndPoint = *pnPointCount - 1;

        ET9FLOAT fTotDistSQ = 0;

        for (nIndex = nEndPoint - 1; nIndex > 0; --nIndex) {

            const ET9FLOAT fDistSQ = __TraceDistanceSQ(&pPoints[nIndex].sPoint, &pPoints[nIndex + 1].sPoint);

            fTotDistSQ += fDistSQ;

            if (fTotDistSQ > fClusterDistSQ) {
                break;
            }

            if (pPoints[nIndex].nQuality < nHighQuality) {
                continue;
            }

            WLOG6(fprintf(pLogFile6, "  end point, demoting %3u\n", nIndex);)

            pPoints[nIndex].nQuality = 0;
        }
    }

    __TraceLog(pKDBInfo, pPoints, *pnPointCount, "after-tidy-end-points");
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pKDBInfo                  .
 * @param pPoints                   .
 * @param pnPointCount              .
 *
 * @return Xxx
 */

static void ET9LOCALCALL __TraceCalculateAngleAndDistance (ET9KDBInfo         * const pKDBInfo,
                                                           ET9TracePointExt   * const pPoints,
                                                           ET9UINT            * const pnPointCount)
{
    ET9UINT nIndex;

    WLOG6(fprintf(pLogFile6, "\n__TraceCalculateAngleAndDistance\n");)

    for (nIndex = 0; nIndex < *pnPointCount; ++nIndex) {

        /* angle */

        if (!nIndex || nIndex + 1 >= *pnPointCount) {

            pPoints[nIndex].fAngle = 0;

            WLOG6(fprintf(pLogFile6, "  %3u angle %5.1f (end point)\n", nIndex, pPoints[nIndex].fAngle);)
        }
        else if (pPoints[nIndex - 1].wKey == ET9_KDB_KEY_OUTOFBOUND ||
                 pPoints[nIndex + 0].wKey == ET9_KDB_KEY_OUTOFBOUND ||
                 pPoints[nIndex + 1].wKey == ET9_KDB_KEY_OUTOFBOUND) {

            pPoints[nIndex].fAngle = 0;

            WLOG6(fprintf(pLogFile6, "  %3u angle %5.1f (out-of-bound)\n", nIndex, pPoints[nIndex].fAngle);)
        }
        else {

            ET9TracePoint_f pTmpPoint[3];

            const ET9FLOAT fAngle = __TraceMidAngle(__ScaleToIntegration_f(pKDBInfo, &pPoints[nIndex - 1].sPoint, &pTmpPoint[0]),
                                                    __ScaleToIntegration_f(pKDBInfo, &pPoints[nIndex + 0].sPoint, &pTmpPoint[1]),
                                                    __ScaleToIntegration_f(pKDBInfo, &pPoints[nIndex + 1].sPoint, &pTmpPoint[2]));

            pPoints[nIndex].fAngle = (ET9FLOAT)(fAngle >= 0 ? (180.0f - fAngle): -(180.0f + fAngle));

            WLOG6(fprintf(pLogFile6, "  %3u angle %5.1f (%5.1f)\n", nIndex, pPoints[nIndex].fAngle, fAngle);)
        }

        /* distance */

        if (nIndex + 1 >= *pnPointCount) {

            pPoints[nIndex].fDist = 0;

            WLOG6(fprintf(pLogFile6, "      distance %5.1f (end point)\n", pPoints[nIndex].fDist);)
        }
        else if (pPoints[nIndex + 0].wKey == ET9_KDB_KEY_OUTOFBOUND ||
                 pPoints[nIndex + 1].wKey == ET9_KDB_KEY_OUTOFBOUND) {

            pPoints[nIndex].fDist = 0;

            WLOG6(fprintf(pLogFile6, "      distance %5.1f (out-of-bound)\n", pPoints[nIndex].fDist);)
        }
        else {

            ET9TracePoint_f pIntPoints[2];

            pPoints[nIndex].fDist = __TraceDistance(__ScaleToIntegration_f(pKDBInfo, &pPoints[nIndex + 0].sPoint, &pIntPoints[0]),
                                                    __ScaleToIntegration_f(pKDBInfo, &pPoints[nIndex + 1].sPoint, &pIntPoints[1]));

            WLOG6(fprintf(pLogFile6, "      distance %5.1f\n", pPoints[nIndex].fDist);)
        }
    }

    __TraceLog(pKDBInfo, pPoints, *pnPointCount, "after-angle-and-distance");
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 */

#ifndef ET9_TRACE_QUALITY_PIPE_SIZE
#define ET9_TRACE_QUALITY_PIPE_SIZE 17
#endif

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 */

typedef struct __QualityPipeItem_s
{
    ET9BOOL             bEndPoint;                              /**< xxx */
    ET9UINT             nIndex;                                 /**< xxx */
    ET9FLOAT            fValue;                                 /**< xxx */
    ET9TracePointExt    *pOwner;                                /**< xxx */
} __QualityPipeItem;                                            /**< xxx */

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 */

typedef struct __QualityPipeAvgList_s
{
    ET9FLOAT            fAvg;                                   /**< xxx */
    ET9UINT             nValueCount;                            /**< xxx */
    ET9UINT             nCurrIndex;                             /**< xxx */
    ET9UINT             nOwnerCount;                            /**< xxx */
    __QualityPipeItem   pItems[ET9_TRACE_QUALITY_PIPE_SIZE];    /**< xxx */
} __QualityPipeAvgList;                                         /**< xxx */

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 */

typedef struct __QualityPipe_s
{
    ET9FLOAT                fQuality;                           /**< xxx */

    ET9U8                   bCurrQualityId;                     /**< xxx */
    ET9FLOAT                fHighQualityTreshold;               /**< xxx */

    __QualityPipeAvgList    sAvgH;                              /**< xxx */
    __QualityPipeAvgList    sAvgB;                              /**< xxx */
    __QualityPipeAvgList    sAvgT;                              /**< xxx */

} __QualityPipe;                                                /**< xxx */

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pAvgList                  .
 * @param nSize                     .
 *
 * @return Xxx
 */

ET9INLINE static void ET9LOCALCALL __InitPipeAvgList (__QualityPipeAvgList      * const pAvgList,
                                                      const ET9UINT                     nSize)
{
    pAvgList->fAvg = 0.0f;
    pAvgList->nValueCount = nSize;
    pAvgList->nCurrIndex = 0;
    pAvgList->nOwnerCount = 0;

    ET9Assert(pAvgList->nValueCount % 2 == 1);
    ET9Assert(pAvgList->nValueCount <= ET9_TRACE_QUALITY_PIPE_SIZE);

    {
        ET9UINT nIndex;

        for (nIndex = 0; nIndex < pAvgList->nValueCount; ++nIndex) {
            pAvgList->pItems[nIndex].fValue = 0;
            pAvgList->pItems[nIndex].pOwner = NULL;
            pAvgList->pItems[nIndex].nIndex = 0xFFFF;
            pAvgList->pItems[nIndex].bEndPoint = 0;
        }
    }
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pKDBInfo                  .
 * @param pPipe                     .
 * @param bSoftReset                .
 *
 * @return Xxx
 */

ET9INLINE static void ET9LOCALCALL __InitPipe (ET9KDBInfo       const * const pKDBInfo,
                                               __QualityPipe          * const pPipe,
                                               const ET9BOOL                  bSoftReset)
{
    ET9_UNUSED(pKDBInfo);

    WLOG6(fprintf(pLogFile6, "__InitPipe, bSoftReset %u\n", bSoftReset);)

    if (!bSoftReset) {
        pPipe->bCurrQualityId = bUndefQualityID + 1;
    }

    pPipe->fHighQualityTreshold = ET9_PRIVATE_TRACE_FILTER_HQT;

    pPipe->fQuality = 0.0f;

    __InitPipeAvgList(&pPipe->sAvgB, ET9_PRIVATE_TRACE_FILTER_AVGL1);
    __InitPipeAvgList(&pPipe->sAvgH, ET9_PRIVATE_TRACE_FILTER_AVGL2);
    __InitPipeAvgList(&pPipe->sAvgT, ET9_PRIVATE_TRACE_FILTER_AVGL2);
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pAvgList                  .
 * @param pItem                     .
 *
 * @return Xxx
 */

ET9INLINE static void ET9LOCALCALL __HandleAvgItem (__QualityPipeAvgList      * const pAvgList,
                                                    __QualityPipeItem   const * const pItem)
{
    __QualityPipeItem * const pCurrItem = &pAvgList->pItems[pAvgList->nCurrIndex];

    if (pCurrItem->pOwner) {
        --pAvgList->nOwnerCount;
    }

    pAvgList->fAvg -= pCurrItem->fValue;

    *pCurrItem = *pItem;

    pAvgList->fAvg += pCurrItem->fValue;

    if (pCurrItem->pOwner) {
        ++pAvgList->nOwnerCount;
    }

    if (++pAvgList->nCurrIndex >= pAvgList->nValueCount) {
        pAvgList->nCurrIndex = 0;
    }
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pKDBInfo                  .
 * @param pPoint                    .
 * @param nIndex                    .
 * @param bEndPoint                 .
 * @param pPipe                     .
 *
 * @return Xxx
 */

ET9INLINE static void ET9LOCALCALL __HandlePipeValue (ET9KDBInfo       const * const pKDBInfo,
                                                      ET9TracePointExt       * const pPoint,
                                                      const ET9UINT                  nIndex,
                                                      const ET9BOOL                  bEndPoint,
                                                      __QualityPipe          * const pPipe)
{
    ET9_UNUSED(pKDBInfo);

    /* add new value */

    {
        const ET9FLOAT fValue = pPoint ? pPoint->fAngle : 0.0f;

        {
            __QualityPipeItem sItem;

            sItem.fValue = fValue;
            sItem.pOwner = pPoint;
            sItem.nIndex = nIndex;
            sItem.bEndPoint = bEndPoint;

            __HandleAvgItem(&pPipe->sAvgT, &pPipe->sAvgB.pItems[pPipe->sAvgB.nCurrIndex]);
            __HandleAvgItem(&pPipe->sAvgB, &pPipe->sAvgH.pItems[pPipe->sAvgH.nCurrIndex]);
            __HandleAvgItem(&pPipe->sAvgH, &sItem);
        }
    }

    /* calculate quality */

    {
        const ET9FLOAT fQualityPrev = pPipe->fQuality;

        const ET9FLOAT fAvgF = ET9_PRIVATE_TRACE_FILTER_AVGF;

        const ET9FLOAT fAvgH = pPipe->sAvgH.fAvg / pPipe->sAvgH.nValueCount;
        const ET9FLOAT fAvgB = pPipe->sAvgB.fAvg / pPipe->sAvgB.nValueCount;
        const ET9FLOAT fAvgT = pPipe->sAvgT.fAvg / pPipe->sAvgT.nValueCount;

        pPipe->fQuality = __ET9Abs(fAvgB) - fAvgF * (__ET9Abs(fAvgH) + __ET9Abs(fAvgT));

        if (pPipe->fQuality < 0) {
            pPipe->fQuality = 0;
        }

        pPipe->fQuality *= pPipe->fQuality;

        if (pPipe->fQuality >= pPipe->fHighQualityTreshold) {

            if (fQualityPrev < pPipe->fHighQualityTreshold) {

                ++pPipe->bCurrQualityId;

                if (pPipe->bCurrQualityId == bUndefQualityID) {
                    ++pPipe->bCurrQualityId;
                }
            }
        }

        WLOG6(fprintf(pLogFile6, "  fAvgB %8.3f (%8.3f %3u), fQuality %6.3f (%6.3f)\n", (pPipe->sAvgB.fAvg / pPipe->sAvgB.nValueCount), pPipe->sAvgB.fAvg, pPipe->sAvgB.nValueCount, pPipe->fQuality, fQualityPrev);)
    }

    /* handle mid value */

    {
        const ET9UINT nCenter = (pPipe->sAvgB.nCurrIndex + (pPipe->sAvgB.nValueCount / 2)) % pPipe->sAvgB.nValueCount;

        ET9TracePointExt * const pCenterPoint = pPipe->sAvgB.pItems[nCenter].pOwner;

        if (pCenterPoint) {

            if (pPipe->sAvgB.pItems[nCenter].bEndPoint) {

                pCenterPoint->bQualityId = bUndefQualityID;

                WLOG6(fprintf(pLogFile6, "    end point (untouched)\n");)
            }
            else if (pPipe->fQuality >= pPipe->fHighQualityTreshold) {

                pCenterPoint->nQuality += nHighQuality;
                pCenterPoint->bQualityId = pPipe->bCurrQualityId;

                WLOG6(fprintf(pLogFile6, "    high quality (id %u)\n", pPipe->bCurrQualityId);)
            }
            else {

                pCenterPoint->nQuality = 0;
                pCenterPoint->bQualityId = bUndefQualityID;

                WLOG6(fprintf(pLogFile6, "    low quality\n");)
            }

            ET9Assert(pPipe->sAvgB.nOwnerCount);
        }
    }
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pKDBInfo                  .
 * @param pPipe                     .
 *
 * @return Xxx
 */

ET9INLINE static void ET9LOCALCALL __EmptyPipe (ET9KDBInfo       const * const pKDBInfo,
                                                __QualityPipe          * const pPipe)
{
    WLOG6(fprintf(pLogFile6, "__EmptyPipe\n");)

    while (pPipe->sAvgH.nOwnerCount || pPipe->sAvgB.nOwnerCount) {
        __HandlePipeValue(pKDBInfo, NULL, 0xFFFE, 0, pPipe);
    }

    __InitPipe(pKDBInfo, pPipe, 1);
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pKDBInfo                  .
 * @param pPoints                   .
 * @param pnPointCount              .
 *
 * @return Xxx
 */

static void ET9LOCALCALL __TraceCalculateQuality (ET9KDBInfo       const * const pKDBInfo,
                                                  ET9TracePointExt       * const pPoints,
                                                  ET9UINT                * const pnPointCount)
{
    const ET9FLOAT fLoD = __TraceCalculateLevelOfDetailIntegration(pKDBInfo, fLevelOfDetailQuality);

    const ET9FLOAT fGapStepSize = fLoD * ET9_PRIVATE_TRACE_FILTER_GPC;

    __QualityPipe sPipe;

    ET9UINT nIndex;

    WLOG6(fprintf(pLogFile6, "\n__TraceCalculateQuality\n");)

    __InitPipe(pKDBInfo, &sPipe, 0);

    for (nIndex = 0; nIndex < *pnPointCount; ++nIndex) {

        if (pPoints[nIndex].wKey == ET9_KDB_KEY_OUTOFBOUND) {

            __EmptyPipe(pKDBInfo, &sPipe);

            pPoints[nIndex].nQuality = 0;
            pPoints[nIndex].bQualityId = bUndefQualityID;

            WLOG6(fprintf(pLogFile6, "  %3u quality %5.1f (out-of-bound)\n", nIndex, pPoints[nIndex].nQuality);)
        }
        else {

            __HandlePipeValue(pKDBInfo, &pPoints[nIndex], nIndex, ((!nIndex || nIndex + 1 == *pnPointCount) ? 1 : 0), &sPipe);

            {
                ET9FLOAT fGap;

                for (fGap = pPoints[nIndex].fDist; fGap > 1.5f * fGapStepSize; fGap -= fGapStepSize) {
                    __HandlePipeValue(pKDBInfo, NULL, 0xFFFD, 0, &sPipe);
                }
            }
        }
    }

    __EmptyPipe(pKDBInfo, &sPipe);

    __TraceLog(pKDBInfo, pPoints, *pnPointCount, "after-calculate-quality");
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pKDBInfo                  .
 * @param fX                        .
 * @param fY                        .
 * @param pnKey                     .
 *
 * @return Xxx
 */

ET9INLINE static ET9BOOL ET9LOCALCALL __TraceGetKey (ET9KDBInfo   const * const pKDBInfo,
                                                     const ET9FLOAT             fX,
                                                     const ET9FLOAT             fY,
                                                     ET9UINT            * const pnKey)
{
    return __KeyAreasFindArea(pKDBInfo, fX, fY, pnKey);
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pKDBInfo                  .
 * @param pPoints                   .
 * @param pnPointCount              .
 *
 * @return Xxx
 */

static void ET9LOCALCALL __TraceGetKeys (ET9KDBInfo         * const pKDBInfo,
                                         ET9TracePointExt   * const pPoints,
                                         ET9UINT            * const pnPointCount)
{
    ET9UINT nIndex;

    WLOG6(fprintf(pLogFile6, "\n__TraceGetKeys\n");)

    for (nIndex = 0; nIndex < *pnPointCount; ++nIndex) {

        ET9UINT nID;

        if (pPoints[nIndex].sPoint.fX < 0.0f || pPoints[nIndex].sPoint.fX >= pKDBInfo->Private.pCurrLayoutInfo->wLayoutWidth ||
            pPoints[nIndex].sPoint.fY < 0.0f || pPoints[nIndex].sPoint.fY >= pKDBInfo->Private.pCurrLayoutInfo->wLayoutHeight) {

            pPoints[nIndex].wKey = ET9_KDB_KEY_OUTOFBOUND;
        }
        else if (__KeyAreasFindArea(pKDBInfo, pPoints[nIndex].sPoint.fX, pPoints[nIndex].sPoint.fY, &nID)) {
            pPoints[nIndex].wKey = (ET9U16)nID;
        }
        else {
            pPoints[nIndex].wKey = ET9_KDB_KEY_UNDEFINED;
        }

        WLOG6(fprintf(pLogFile6, "  %3u got key %2u\n", nIndex, pPoints[nIndex].wKey);)
    }

    __TraceLog(pKDBInfo, pPoints, *pnPointCount, "after-keys");
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param wKey                   .
 *
 * @return Xxx
 */

ET9INLINE static ET9BOOL ET9LOCALCALL __IsNonKey (ET9U16 wKey)
{
    if (wKey == ET9_KDB_KEY_UNDEFINED || wKey == ET9_KDB_KEY_OUTOFBOUND) {
        return 1;
    }
    else {
        return 0;
    }
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pKDBInfo                  .
 * @param pPoints                   .
 * @param pnPointCount              .
 *
 * @return Xxx
 */

static void ET9LOCALCALL __TraceTidyNonKeys (ET9KDBInfo       const * const pKDBInfo,
                                             ET9TracePointExt       * const pPoints,
                                             ET9UINT                * const pnPointCount)
{
    ET9UINT nIndex;

    WLOG6(fprintf(pLogFile6, "\n__TraceTidyNonKeys\n");)

    if (!*pnPointCount) {
        return;
    }

    for (nIndex = 0; (nIndex < *pnPointCount) && __IsNonKey(pPoints[nIndex].wKey); ++nIndex) {
        pPoints[nIndex].bRemoved = 1;
    }

    for (nIndex = *pnPointCount - 1; nIndex && __IsNonKey(pPoints[nIndex].wKey); --nIndex) {
        pPoints[nIndex].bRemoved = 1;
    }

    for (nIndex = 0; nIndex + 2 < *pnPointCount; ++nIndex) {

        ET9UINT nLook;

        if (!__IsNonKey(pPoints[nIndex].wKey) || pPoints[nIndex].bRemoved) {
            continue;
        }

        for (nLook = nIndex + 1; nLook + 1 < *pnPointCount; ++nLook) {

            /* the set must be all undef or all out-of-bound */

            if (pPoints[nIndex].wKey != pPoints[nLook].wKey) {
                break;
            }

            if (__IsNonKey(pPoints[nLook + 1].wKey)) {
                pPoints[nLook].bRemoved = 1;
            }
        }

        nIndex = nLook - 1;
    }

    __TraceRemoveDeleted(pPoints, pnPointCount);

    __TraceLog(pKDBInfo, pPoints, *pnPointCount, "after-tidy-non-keys");
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pKDBInfo                  .
 * @param pPoints                   .
 * @param pnPointCount              .
 *
 * @return Xxx
 */

static void ET9LOCALCALL __TraceRemoveNonKeys (ET9KDBInfo       const * const pKDBInfo,
                                               ET9TracePointExt       * const pPoints,
                                               ET9UINT                * const pnPointCount)
{
    ET9UINT nIndex;

    WLOG6(fprintf(pLogFile6, "\n__TraceRemoveNonKeys\n");)

    for (nIndex = 0; nIndex < *pnPointCount; ++nIndex) {

        if (__IsNonKey(pPoints[nIndex].wKey)) {
            pPoints[nIndex].bRemoved = 1;
        }
    }

    __TraceRemoveDeleted(pPoints, pnPointCount);

    __TraceLog(pKDBInfo, pPoints, *pnPointCount, "after-remove-non-keys");
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pKDBInfo                  .
 * @param pPoints                   .
 * @param pnPointCount              .
 *
 * @return Xxx
 */

static void ET9LOCALCALL __TraceRemoveStraddleKeys (ET9KDBInfo       const * const pKDBInfo,
                                                    ET9TracePointExt   * const pPoints,
                                                    ET9UINT            * const pnPointCount)
{
    const ET9FLOAT fMaxRunDist = 2.0f * __TraceCalculateLevelOfDetailKDB(pKDBInfo, 1.0f);

    const ET9FLOAT fMaxRunDistSQ = fMaxRunDist * fMaxRunDist;

    ET9UINT nIndex;

    WLOG6(fprintf(pLogFile6, "\n__TraceRemoveStraddleKeys\n");)

    /* calculate distances */

    for (nIndex = 0; nIndex < *pnPointCount; ++nIndex) {

        if (nIndex + 1 < *pnPointCount) {
            pPoints[nIndex].fDist = __TraceDistanceSQ(&pPoints[nIndex].sPoint, &pPoints[nIndex + 1].sPoint);
        }
        else {
            pPoints[nIndex].fDist = 0.0f;
        }

        ET9Assert(!__IsNonKey(pPoints[nIndex].wKey));
    }

    /* iterate runs */

    for (nIndex = 0; nIndex + 1 < *pnPointCount; ++nIndex) {

        ET9UINT nLook;
        ET9FLOAT fRunDist;

        if (pPoints[nIndex].nQuality >= nHighQuality) {
            continue;
        }

        fRunDist = 0.0f;

        for (nLook = nIndex; nLook < *pnPointCount; ++nLook) {

            if (pPoints[nLook].nQuality >= nHighQuality) {
                break;
            }

            if (fRunDist + pPoints[nLook].fDist > fMaxRunDistSQ) {
                break;
            }

            if (nLook - nIndex > 10) {
                break;
            }

            fRunDist += pPoints[nLook].fDist;
        }

        {
            const ET9UINT nEnd = nLook;

            if (nEnd - nIndex < 2) {
                continue;
            }

            for (nLook = nIndex + 1; nLook < nEnd; ++nLook) {

                ET9UINT nCmpIndex;

                for (nCmpIndex = nIndex; nCmpIndex < nLook; ++nCmpIndex) {

                    if (pPoints[nLook].wKey == pPoints[nCmpIndex].wKey) {

                        if (!pPoints[nLook].bRemoved) {

                            WLOG6(fprintf(pLogFile6, "  removing %3u\n", nLook);)

                            pPoints[nLook].bRemoved = 1;
                        }

                        break;
                    }
                }
            }
        }
    }

    /* reset distances */

    for (nIndex = 0; nIndex < *pnPointCount; ++nIndex) {
        pPoints[nIndex].fDist = 0.0f;
    }

    /* post */

    __TraceRemoveDeleted(pPoints, pnPointCount);

    __TraceLog(pKDBInfo, pPoints, *pnPointCount, "after-remove-straddle-keys");
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pKDBInfo                  .
 * @param pPoints                   .
 * @param pnPointCount              .
 *
 * @return Xxx
 */

static void ET9LOCALCALL __TraceRemoveDupeKeys (ET9KDBInfo       const * const pKDBInfo,
                                                ET9TracePointExt   * const pPoints,
                                                ET9UINT            * const pnPointCount)
{
    ET9UINT nIndex;

    WLOG6(fprintf(pLogFile6, "\n__TraceRemoveDupeKeys\n");)

    for (nIndex = 0; nIndex + 1 < *pnPointCount; ++nIndex) {

        ET9UINT nLook;

        if (pPoints[nIndex].bRemoved) {
            continue;
        }

        ET9Assert(!__IsNonKey(pPoints[nIndex].wKey));

        for (nLook = nIndex + 1; nLook < *pnPointCount; ++nLook) {

            if (pPoints[nLook].wKey != pPoints[nIndex].wKey) {
                break;
            }
        }

        {
            const ET9UINT nCount = nLook - nIndex;

            if (nCount < 2) {
                continue;
            }

            __TraceClusterPoints(pPoints, *pnPointCount, nIndex, nCount, 1);
        }
    }

    __TraceRemoveDeleted(pPoints, pnPointCount);

    __TraceLog(pKDBInfo, pPoints, *pnPointCount, "after-remove-dupe-keys");
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pKDBInfo                  .
 * @param pP1                       .
 * @param pP2                       .
 * @param pPoints                   .
 * @param pnPointCount              .
 *
 * @return Xxx
 */

static ET9UINT ET9LOCALCALL __TraceAddLineKeys (ET9KDBInfo        const * const pKDBInfo,
                                                ET9TracePointExt  const * const pP1,
                                                ET9TracePointExt  const * const pP2,
                                                ET9TracePointExt        * const pPoints,
                                                ET9UINT                 * const pnPointCount)
{
    ET9UINT nAddCount = 0;

    const ET9FLOAT fKeySize = __TraceCalculateLevelOfDetailKDB(pKDBInfo, 1.0f);
    const ET9FLOAT fLoD = __TraceCalculateLevelOfDetailKDB(pKDBInfo, fLevelOfDetailQuality);

    ET9TracePoint_f const * const pStart = &pP1->sPoint;
    ET9TracePoint_f const * const pEnd = &pP2->sPoint;

    const ET9UINT nStartKey = pP1->wKey;
    const ET9UINT nEndKey = pP2->wKey;

    ET9UINT nCurrKey = nStartKey;
    ET9UINT nCurrKeyCount = 0;
    ET9FLOAT fCurrKeyX = 0;
    ET9FLOAT fCurrKeyY = 0;

    const ET9FLOAT fDistX = pEnd->fX - pStart->fX;
    const ET9FLOAT fDistY = pEnd->fY - pStart->fY;

    const ET9FLOAT fDist = _ET9sqrt_f(fDistX * fDistX + fDistY * fDistY);

    const ET9UINT nSteps = (ET9UINT)(5.0f * fDist / fKeySize) + 1;

    ET9UINT nPos = pP1->nPos;

    ET9UINT nStep;

    if (fDist < fLoD) {
        return 0;
    }

    if (nStartKey == ET9_KDB_KEY_OUTOFBOUND || nEndKey == ET9_KDB_KEY_OUTOFBOUND) {
        return 0;
    }
    else if (nStartKey == ET9_KDB_KEY_UNDEFINED || nEndKey == ET9_KDB_KEY_UNDEFINED) {
    }
    else if (nStartKey == nEndKey) {
        return 0;
    }

    for (nStep = 1; nStep <= nSteps && nStep < 1000; ++nStep) {

        const ET9FLOAT fX = (pStart->fX + nStep * (fDistX / nSteps));
        const ET9FLOAT fY = (pStart->fY + nStep * (fDistY / nSteps));

        ET9UINT nKey;
        ET9UINT nAddIndex;
        ET9TracePointExt *pNewPoint;

        if (!__TraceGetKey(pKDBInfo, fX, fY, &nKey)) {
            continue;
        }

        if (nCurrKey != nKey) {

            if (nCurrKey != nStartKey && nCurrKey != nEndKey) {

                nAddIndex = __TraceAddPoint(pPoints, pnPointCount);

                if (nAddIndex >= *pnPointCount) {
                    break;
                }

                pNewPoint = &pPoints[nAddIndex];

                pNewPoint->nPos = ++nPos;
                pNewPoint->wKey = (ET9U16)nCurrKey;
                pNewPoint->sPoint.fX = fCurrKeyX / nCurrKeyCount;
                pNewPoint->sPoint.fY = fCurrKeyY / nCurrKeyCount;

                ++nAddCount;
            }

            nCurrKey = nKey;
            nCurrKeyCount = 0;
        }

        if (!nCurrKeyCount) {
            fCurrKeyX = 0;
            fCurrKeyY = 0;
        }

        fCurrKeyX += fX;
        fCurrKeyY += fY;

        ++nCurrKeyCount;
    }

    return nAddCount;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pKDBInfo                  .
 * @param pPoints                   .
 * @param pnPointCount              .
 *
 * @return Xxx
 */

static void ET9LOCALCALL __TraceAddTubeKeys (ET9KDBInfo         * const pKDBInfo,
                                             ET9TracePointExt   * const pPoints,
                                             ET9UINT            * const pnPointCount)
{
    ET9UINT nIndex;
    ET9UINT nAddCount = 0;

    const ET9UINT nEndCount = *pnPointCount;

    WLOG6(fprintf(pLogFile6, "\n__TraceAddTubeKeys\n");)

    __TraceSetSparsePos(pPoints, *pnPointCount);

    for (nIndex = 0; nIndex + 1 < nEndCount; ++nIndex) {
        nAddCount += __TraceAddLineKeys(pKDBInfo, &pPoints[nIndex], &pPoints[nIndex + 1], pPoints, pnPointCount);
    }

    if (nAddCount) {

        __TracePosSortTrace(pPoints, *pnPointCount);

        __TraceLog(pKDBInfo, pPoints, *pnPointCount, "after-add-tube-keys");
    }
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pKDBInfo                  .
 * @param pPoints                   .
 * @param pnPointCount              .
 *
 * @return Xxx
 */

static void ET9LOCALCALL __TraceAddSegmentKeys (ET9KDBInfo         * const pKDBInfo,
                                                ET9TracePointExt   * const pPoints,
                                                ET9UINT            * const pnPointCount)
{
    ET9UINT nIndex;
    ET9UINT nAddCount = 0;

    const ET9UINT nEndCount = *pnPointCount;

    WLOG6(fprintf(pLogFile6, "\n__TraceAddSegmentKeys\n");)

    __TraceSetSparsePos(pPoints, *pnPointCount);

    for (nIndex = 0; nIndex + 1 < nEndCount; ++nIndex) {

        const ET9UINT nStartKey = pPoints[nIndex].wKey;
        const ET9UINT nEndKey = pPoints[nIndex + 1].wKey;

        if (nStartKey == ET9_KDB_KEY_UNDEFINED || nEndKey == ET9_KDB_KEY_UNDEFINED) {

            nAddCount += __TraceAddLineKeys(pKDBInfo, &pPoints[nIndex], &pPoints[nIndex + 1], pPoints, pnPointCount);
        }
    }

    if (nAddCount) {

        __TracePosSortTrace(pPoints, *pnPointCount);

        __TraceLog(pKDBInfo, pPoints, *pnPointCount, "after-add-segment-keys");
    }
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pKDBInfo                  .
 * @param pPoints                   .
 * @param nPointCount               .
 *
 * @return Xxx
 */

static void ET9LOCALCALL __TraceCompress (ET9KDBInfo       const * const pKDBInfo,
                                          ET9TracePointExt       * const pPoints,
                                          const ET9UINT                  nPointCount)
{
    ET9UINT nIndex;
    ET9UINT nSymbCount;
    ET9UINT nQualityCount;

    WLOG6(fprintf(pLogFile6, "\n__TraceCompress\n");)

    if (nPointCount < ET9_TRACE_COMPRESSION_POINT) {
        WLOG6(fprintf(pLogFile6, "  skipping - not many points (%u < %u)\n", nPointCount, ET9_TRACE_COMPRESSION_POINT);)
        return;
    }

    /* identify merge seq's */

    for (nIndex = 0; nIndex < nPointCount; ++nIndex) {

        ET9UINT nLook;

        WLOG6(fprintf(pLogFile6, "  @ index %u, quality %u\n", nIndex, pPoints[nIndex].nQuality);)

        if (!pPoints[nIndex].nQuality) {
            WLOG6(fprintf(pLogFile6, "  zero quality\n");)
            continue;
        }

        for (nLook = nIndex + 1; nLook < nPointCount; ++nLook) {
            if (pPoints[nLook].nQuality) {
                break;
            }
        }

        if (nLook - nIndex - 1 < 2) {
            continue;
        }

        WLOG6(fprintf(pLogFile6, "  found sequence %u : %u -> %u\n", nIndex, nIndex + 2, nLook - 1);)

        {
            ET9U8 bMergeState;
            ET9UINT nMergeIndex;

            bMergeState = 0;
            for (nMergeIndex = nIndex + 1; nMergeIndex < nLook; ++nMergeIndex) {
                pPoints[nMergeIndex].bMergeWithPrev = bMergeState;
                bMergeState = (bMergeState + 1) % ET9MAXBASESYMBS;
            }
        }
    }

    __TraceLog(pKDBInfo, pPoints, nPointCount, "after-full-compress");

    /* potentially break long seq's */

    nSymbCount = 0;
    nQualityCount = 0;
    for (nIndex = 0; nIndex < nPointCount; ++nIndex) {
        if (!pPoints[nIndex].bMergeWithPrev) {
            ++nSymbCount;
        }
        if (pPoints[nIndex].nQuality) {
            ++nQualityCount;
        }
    }

    if (nQualityCount <= 5 && nSymbCount < ET9MAXWORDSIZE) {

        for (nIndex = 0; nIndex < nPointCount && nSymbCount < ET9MAXWORDSIZE; ++nIndex) {

            const ET9UINT nTargetSeqLen = 5;

            ET9UINT nLook;
            ET9UINT nSeqLen;
            ET9UINT nNextIndex;

            if (!pPoints[nIndex].bMergeWithPrev) {
                continue;
            }

            nSeqLen = 1;
            for (nLook = nIndex + 1; nLook < nPointCount; ++nLook) {
                if (pPoints[nLook].bMergeWithPrev) {
                    ++nSeqLen;
                }
                else {
                    break;
                }
            }

            nNextIndex = nLook;

            if (nSeqLen <= nTargetSeqLen) {
                nIndex = nNextIndex;
                continue;
            }

            /* break seq */

            WLOG6(fprintf(pLogFile6, "  found break sequence %u -> %u\n", nIndex, nLook - 1);)

            {
                ET9UINT nBreaks = (nSeqLen / nTargetSeqLen) - 1 + ((nSeqLen % nTargetSeqLen) ? 1 : 0);

                if (nSymbCount + nBreaks > ET9MAXWORDSIZE) {
                    nBreaks = ET9MAXWORDSIZE - nSymbCount;
                }

                ET9Assert(nBreaks);

                {
                    const ET9UINT nBreakSize = nSeqLen / (nBreaks + 1);

                    ET9UINT nBreakIndex;

                    for (nBreakIndex = 0; nBreakIndex < nBreaks; ++nBreakIndex) {

                        const ET9UINT nBreakPoint = nIndex + (nBreakIndex + 1) * nBreakSize;

                        WLOG6(fprintf(pLogFile6, "  breaking seq at %u\n", nBreakPoint);)

                        ET9Assert(pPoints[nBreakPoint].bMergeWithPrev);

                        pPoints[nBreakPoint].bMergeWithPrev = 0;
                        ++nSymbCount;
                    }
                }
            }

            /* to next seq */

            nIndex = nNextIndex;
        }

        /* done */

        __TraceLog(pKDBInfo, pPoints, nPointCount, "after-compress-break");
    }
    else {
        WLOG6(fprintf(pLogFile6, "skipped compress break\n");)
    }
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pKDBInfo                  .
 * @param pPoints                   .
 * @param nPointCount               .
 * @param pfX                       .
 * @param pfY                       .
 *
 * @return Xxx
 */

static ET9BOOL ET9LOCALCALL __TraceIsSingleKey(ET9KDBInfo             * const pKDBInfo,
                                               ET9TracePointExt       * const pPoints,
                                               const ET9UINT                  nPointCount,
                                               ET9FLOAT               * const pfX,
                                               ET9FLOAT               * const pfY)
{
    const ET9FLOAT fMedianKeySize = ((ET9FLOAT)pKDBInfo->Private.pCurrLayoutInfo->nMedianKeyWidth + (ET9FLOAT)pKDBInfo->Private.pCurrLayoutInfo->nMedianKeyHeight) / 2;

    const ET9FLOAT fMinDist = fMedianKeySize / 2;

    ET9FLOAT fMinX;
    ET9FLOAT fMaxX;
    ET9FLOAT fMinY;
    ET9FLOAT fMaxY;

    ET9UINT nIndex;
    ET9TracePointExt const * pPoint;

    if (!nPointCount) {
        return 0;
    }

    pPoint = pPoints;

    fMinX = pPoint->sPoint.fX;
    fMaxX = fMinX;
    fMinY = pPoint->sPoint.fY;
    fMaxY = fMinY;

    ++pPoint;
    for (nIndex = 1; nIndex < nPointCount; ++nIndex, ++pPoint) {
        if (fMinX > pPoint->sPoint.fX) {
            fMinX = pPoint->sPoint.fX;
        }
        if (fMaxX < pPoint->sPoint.fX) {
            fMaxX = pPoint->sPoint.fX;
        }
        if (fMinY > pPoint->sPoint.fY) {
            fMinY = pPoint->sPoint.fY;
        }
        if (fMaxY < pPoint->sPoint.fY) {
            fMaxY = pPoint->sPoint.fY;
        }
    }

    if (fMaxX - fMinX > fMinDist || fMaxY - fMinY > fMinDist) {
        return 0;
    }

    if (pfX) {
        *pfX = (fMaxX + fMinX) / 2;
    }

    if (pfY) {
        *pfY = (fMaxY + fMinY) / 2;
    }

    return 1;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pKDBInfo                  .
 * @param pWordSymbInfo             .
 * @param bCurrIndexInList          .
 * @param pPoints                   .
 * @param nPointCount               .
 * @param peStatus                  .
 * @param psFunctionKey             .
 *
 * @return Xxx
 */

static ET9BOOL ET9LOCALCALL __TraceSingleKey(ET9KDBInfo             * const pKDBInfo,
                                             ET9WordSymbInfo        * const pWordSymbInfo,
                                             const ET9U8                    bCurrIndexInList,
                                             ET9TracePointExt       * const pPoints,
                                             const ET9UINT                  nPointCount,
                                             ET9STATUS              * const peStatus,
                                             ET9SYMB                * const psFunctionKey)
{
    ET9FLOAT fX;
    ET9FLOAT fY;

    if (!__TraceIsSingleKey(pKDBInfo, pPoints, nPointCount, &fX, &fY)) {
        *peStatus = ET9STATUS_NONE;
        return 0;
    }

    {
        ET9DirectedPos sDirectedPos;

        __InitDirectedPos(&sDirectedPos);

        sDirectedPos.bForceGeneric = 1;

        sDirectedPos.sPos.nX = (ET9UINT)fX;
        sDirectedPos.sPos.nY = (ET9UINT)fY;

        *peStatus = __ProcessTap(pKDBInfo,
                                 pWordSymbInfo,
                                 &sDirectedPos,
                                 bCurrIndexInList,
                                 psFunctionKey);

        if (!*peStatus) {
            return 1;
        }
    }

    {
        ET9UINT nID;

        if (!__KeyAreasFindArea(pKDBInfo, fX, fY, &nID)) {
            *peStatus = ET9STATUS_NO_KEY;
            return 1;
        }

        *peStatus = ET9KDB_ProcessKey(pKDBInfo,
                                      pWordSymbInfo,
                                      (ET9U16)nID,
                                      bCurrIndexInList,
                                      psFunctionKey);
    }

    return 1;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pKDBInfo                  .
 * @param pPoints                   .
 * @param pnPointCount              .
 *
 * @return Xxx
 */

static ET9STATUS ET9LOCALCALL __TraceCalculate(ET9KDBInfo         * const pKDBInfo,
                                               ET9TracePointExt   * const pPoints,
                                               ET9UINT            * const pnPointCount)
{
    WLOG6(fprintf(pLogFile6, "\n__TraceCalculate, nPointCount %u\n", *pnPointCount);)

    __TraceInitExt(pKDBInfo, pPoints, *pnPointCount);

    __TraceGetKeys(pKDBInfo, pPoints, pnPointCount);

    __TraceAddSegmentKeys(pKDBInfo, pPoints, pnPointCount);

    __TraceTidyNonKeys(pKDBInfo, pPoints, pnPointCount);

    __TraceEndPointQuality(pKDBInfo, pPoints, pnPointCount);

    __TraceCalculateAngleAndDistance(pKDBInfo, pPoints, pnPointCount);

    __TraceCalculateQuality(pKDBInfo, pPoints, pnPointCount);

    __TraceTidyQualityIds(pKDBInfo, pPoints, pnPointCount);

    __TraceTidyQualityDistance(pKDBInfo, pPoints, pnPointCount);

    __TraceTidyEndPoints(pKDBInfo, pPoints, pnPointCount);

    __TraceAddTubeKeys(pKDBInfo, pPoints, pnPointCount);

    __TraceRemoveNonKeys(pKDBInfo, pPoints, pnPointCount);

    __TraceRemoveStraddleKeys(pKDBInfo, pPoints, pnPointCount);

    __TraceRemoveDupeKeys(pKDBInfo, pPoints, pnPointCount);

    __TraceCompress(pKDBInfo, pPoints, *pnPointCount);

    __TraceLog(pKDBInfo, pPoints, *pnPointCount, "final");

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pWordSymbInfo             .
 *
 * @return Xxx
 */

static ET9U8 ET9LOCALCALL __TraceNewTraceID (ET9WordSymbInfo        * const pWordSymbInfo)
{
    ET9UINT nCount;
    ET9UINT nIndex;
    ET9SymbInfo *pSymbInfo;
    ET9U8 pbUsed[0x100];

    _ET9ClearMem((ET9U8*)pbUsed, sizeof(pbUsed));

    pSymbInfo = pWordSymbInfo->SymbsInfo;
    for (nCount = pWordSymbInfo->bNumSymbs; nCount; --nCount, ++pSymbInfo) {
        pbUsed[pSymbInfo->bTraceIndex] = 1;
    }

    for (nIndex = 1; nIndex < 0x100; ++nIndex) {
        if (!pbUsed[nIndex]) {
            return (ET9U8)nIndex;
        }
    }

    ET9Assert(0);

    return 1;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pSymbInfo                 .
 *
 * @return Xxx
 */

static ET9BOOL ET9LOCALCALL __SymbIsRegonal (ET9SymbInfo * const pSymbInfo)
{
    return (ET9BOOL)!(pSymbInfo->eInputType == ET9DISCRETEKEY ||
                      pSymbInfo->eInputType == ET9MULTITAPKEY ||
                      pSymbInfo->eInputType == ET9CUSTOMSET ||
                      pSymbInfo->bSymbType == ET9KTSMARTPUNCT);
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pWordSymbInfo             .
 * @param pPoint                    .
 *
 * @return Xxx
 */

static void ET9LOCALCALL __TraceCompressSingleTap (ET9WordSymbInfo        * const pWordSymbInfo,
                                                   ET9TracePointExt       * const pPoint)
{
    ET9SymbInfo * const pSymbInfo = &pWordSymbInfo->SymbsInfo[pWordSymbInfo->bNumSymbs - 1];

    WLOG6(fprintf(pLogFile6, "__TraceCompressSingleTap\n");)

    if (pPoint->nQuality) {
        return;
    }

    if (!(pSymbInfo->bTraceIndex && !pSymbInfo->bTraceProbability)) {
        ET9Assert(0);
        return;
    }

    if (!__SymbIsRegonal(pSymbInfo)) {
        return;
    }

    WLOG6(fprintf(pLogFile6, "  made discrete (%u -> 1)\n", pSymbInfo->bNumBaseSyms);)

    pSymbInfo->eInputType = ET9DISCRETEKEY;
    pSymbInfo->bNumBaseSyms = 1;
    pSymbInfo->DataPerBaseSym[0].bSymFreq = 254;

    if (!pPoint->bMergeWithPrev) {
        return;
    }

    if (pWordSymbInfo->bNumSymbs < 2) {
        ET9Assert(0);
        return;
    }

    WLOG6(fprintf(pLogFile6, "  merging\n");)

    {
        ET9SymbInfo * const pPrevSymbInfo = &pWordSymbInfo->SymbsInfo[pWordSymbInfo->bNumSymbs - 2];

        if (pPrevSymbInfo->bNumBaseSyms >= ET9MAXBASESYMBS) {
            WLOG6(fprintf(pLogFile6, "    skip, prev symb is full\n");)
            return;
        }

        pPrevSymbInfo->DataPerBaseSym[pPrevSymbInfo->bNumBaseSyms] = pSymbInfo->DataPerBaseSym[0];
        ++pPrevSymbInfo->bNumBaseSyms;

        --pWordSymbInfo->bNumSymbs;
    }
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pKDBInfo                  .
 *
 * @return Xxx
 */

static void ET9LOCALCALL __SetAmbigMode (ET9KDBInfo * const pKDBInfo)
{
    pKDBInfo->dwStateBits &= ~(ET9_KDB_MULTITAP_MODE_MASK);
    pKDBInfo->dwStateBits |= ET9_KDB_AMBIGUOUS_MODE_MASK;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pKDBInfo                  .
 *
 * @return Xxx
 */

static void ET9LOCALCALL __SetMultitapMode (ET9KDBInfo * const pKDBInfo)
{
    pKDBInfo->dwStateBits &= ~(ET9_KDB_AMBIGUOUS_MODE_MASK);
    pKDBInfo->dwStateBits |= ET9_KDB_MULTITAP_MODE_MASK;
}

/*---------------------------------------------------------------------------*/
/** \internal
 * xxx.
 *
 * @param pKDBInfo                  .
 * @param pWordSymbInfo             .
 * @param pPoints                   .
 * @param nPointCount               .
 * @param bCurrIndexInList          .
 *
 * @return Xxx
 */

static ET9STATUS ET9LOCALCALL __TraceTap (ET9KDBInfo             * const pKDBInfo,
                                          ET9WordSymbInfo        * const pWordSymbInfo,
                                          ET9TracePointExt       * const pPoints,
                                          const ET9UINT                  nPointCount,
                                          const ET9U8                    bCurrIndexInList)
{
    ET9UINT             nCount;
    ET9TracePointExt    *pPoint;
    ET9BOOL             bFirstTap = 1;
    ET9U16              wInputIndex = pWordSymbInfo->bNumSymbs ? (pWordSymbInfo->SymbsInfo[pWordSymbInfo->bNumSymbs - 1].wInputIndex + 1) : 1;

    const ET9U8         bTraceID = __TraceNewTraceID(pWordSymbInfo);

    const ET9BOOL       bWasMultitapMode = (ET9BOOL)ET9_KDB_MULTITAP_MODE(pKDBInfo->dwStateBits);

    const ET9UINT       nWmID = pKDBInfo->Private.nWmUseID;

    WLOG6(fprintf(pLogFile6, "\n__TraceTap\n");)

    if (bWasMultitapMode) {
        __SetAmbigMode(pKDBInfo);
    }

    pPoint = pPoints;
    for (nCount = nPointCount; nCount; --nCount, ++pPoint) {

        ET9STATUS eStatus;
        ET9U8 bIndex;
        ET9SYMB sFunctionKey;

        const ET9U8 bPreNumSymbs = pWordSymbInfo->bNumSymbs;

        /* tap */

        {
            ET9SYMB sFunctionKey;
            ET9DirectedPos sDirectedPos;

            WLOG6(fprintf(pLogFile6, "  ProcessTapDirected, %5.1f, %5.1f\n", pPoint->sPoint.fX, pPoint->sPoint.fY);)

            __InitDirectedPos(&sDirectedPos);

            sDirectedPos.bForceGeneric = 1;

            sDirectedPos.sPos.nX = (ET9UINT)(pPoint->sPoint.fX + 0.5f);
            sDirectedPos.sPos.nY = (ET9UINT)(pPoint->sPoint.fY + 0.5f);

            WLOG6(fprintf(pLogFile6, "    nX %3u nY %3u\n", sDirectedPos.sPos.nX, sDirectedPos.sPos.nY);)

            if (bFirstTap && nCount > 1) {
                sDirectedPos.sL1.nX = (ET9UINT)((pPoint + 0)->sPoint.fX + 0.5f);
                sDirectedPos.sL1.nY = (ET9UINT)((pPoint + 0)->sPoint.fY + 0.5f);
                sDirectedPos.sL2.nX = (ET9UINT)((pPoint + 1)->sPoint.fX + 0.5f);
                sDirectedPos.sL2.nY = (ET9UINT)((pPoint + 1)->sPoint.fY + 0.5f);
            }
            else if (!bFirstTap) {
                sDirectedPos.sL1.nX = (ET9UINT)((pPoint - 1)->sPoint.fX + 0.5f);
                sDirectedPos.sL1.nY = (ET9UINT)((pPoint - 1)->sPoint.fY + 0.5f);
                sDirectedPos.sL2.nX = (ET9UINT)((pPoint - 0)->sPoint.fX + 0.5f);
                sDirectedPos.sL2.nY = (ET9UINT)((pPoint - 0)->sPoint.fY + 0.5f);
            }
            else {
                sDirectedPos.sL1.nX = 0;
                sDirectedPos.sL1.nY = 0;
                sDirectedPos.sL2.nX = 0;
                sDirectedPos.sL2.nY = 0;
            }

            eStatus = __ProcessTap(pKDBInfo,
                                   pWordSymbInfo,
                                   &sDirectedPos,
                                   bFirstTap ? bCurrIndexInList : ET9_NO_ACTIVE_INDEX,
                                   &sFunctionKey);

            if (sFunctionKey) {
                eStatus = ET9STATUS_ERROR;
            }

            if (pKDBInfo->Private.nWmUseID != nWmID) {
                return ET9STATUS_KDB_WM_ERROR;
            }
        }

        if (eStatus) {

            WLOG6(fprintf(pLogFile6, "    returned %u\n", eStatus);)

            /* if tap fails try key */

            WLOG6(fprintf(pLogFile6, "  ProcessKey, %u\n", pPoint->wKey);)

            eStatus = ET9KDB_ProcessKey(pKDBInfo,
                                        pWordSymbInfo,
                                        pPoint->wKey,
                                        bFirstTap ? bCurrIndexInList : ET9_NO_ACTIVE_INDEX,
                                        &sFunctionKey);

            if (eStatus || sFunctionKey) {
                WLOG6(fprintf(pLogFile6, "    returned %u, fkey %u\n", eStatus, sFunctionKey);)
            }

            if (eStatus) {

                if (bWasMultitapMode) {
                    __SetMultitapMode(pKDBInfo);
                }

                return eStatus;
            }

            if (pKDBInfo->Private.nWmUseID != nWmID) {
                return ET9STATUS_KDB_WM_ERROR;
            }
        }

        /* */

        bFirstTap = 0;

        /* add additional symb info */

        for (bIndex = bPreNumSymbs; bIndex < pWordSymbInfo->bNumSymbs; ++bIndex) {

            ET9SymbInfo * const pSymbInfo = &pWordSymbInfo->SymbsInfo[bIndex];

            pSymbInfo->wInputIndex = wInputIndex;

            pSymbInfo->bTraceIndex = bTraceID;

            if (pPoint->nQuality <= 0xFF) {
                pSymbInfo->bTraceProbability = (ET9U8)pPoint->nQuality;
            }
            else {
                pSymbInfo->bTraceProbability = 0xFF;
            }

            if (pSymbInfo->wTapX == ET9UNDEFINEDTAPVALUE && pSymbInfo->wTapY == ET9UNDEFINEDTAPVALUE) {

                /* e.g. process key won't record trace pos - add here */

                pSymbInfo->wTapX = (ET9U16)pPoint->sPoint.fX;
                pSymbInfo->wTapY = (ET9U16)pPoint->sPoint.fY;
            }
        }

        /* */

        __TraceCompressSingleTap(pWordSymbInfo, pPoint);
    }

    /* possibly restore MT */

    if (bWasMultitapMode) {
        __SetMultitapMode(pKDBInfo);
    }

    /* turn off correction inhibit */

    pWordSymbInfo->Private.bRequiredInhibitCorrection = 0;

    /* done */

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \ingroup et9trace
 * Informs the Keyboard Input Module that the user has entered trace-based input.
 * This function should be called with all "touch based" input. If the points given yield a key press rather than a word trace,
 * then it will generate a key press automatically.
 * It should be possible to trace a single key as well, it gives a different behaviour than just pressing it.<p>
 * The integration layer might want to apply appropriate trace point/coordinate filtering if an excessive amount of points are generated.
 * The target number of points that is aquired for a long trace is ET9_TRACE_MAX_POINTS, but it's possibly to submit more points than this.
 *
 * @param[in]     pKDBInfo          Pointer to the Keyboard Input Module Information Data Structure (ET9KDBInfo).
 * @param[in,out] pWordSymbInfo     Pointer to the Word Symbol Data Structure (ET9WordSymbInfo).
 *                                  The Keyboard Input Module writes to this structure information about the input the user has provided.
 * @param[in]     pPoints           Pointer to a list of trace points to process, coordinates should be in the (potentially scaled) KDB coordinate space.
 * @param[in]     nPointCount       The number of points in the list.
 * @param[in]     bCurrIndexInList  0-based index value that indicates which word in the selection list is currently selected. XT9 locks this word before it adds the specified input value.<br>
 *                                  If there is no current input sequence, set the value of this parameter to
 *                                  ET9_NO_ACTIVE_INDEX.<br>
 *                                  If the integration layer passes as an invalid argument for this parameter, XT9 will
 *                                  still add the new input, but it will not lock the word. Also, it will not return an error status.
 * @param[out]    psFunctionKey     Pointer to a buffer where the Keyboard Input Module stores information about the tapped/pressed key if it is a function key requiring action by the integration layer (for example, changing the shift state or input mode).<br>
 *                                  Function key values are part of the ET9KDBKEYDEF enumeration.<br>
 *                                  If the key that was pressed or tapped is not a function key, XT9 sets the value to 0.
 *
 * @retval ET9STATUS_NONE               Function call was handled successfully.
 * @retval ET9STATUS_INVALID_MEMORY     At least one pointer passed as a function argument was set to null.
 * @retval ET9STATUS_NO_INIT            The Keyboard Input Module has not been initialized. Call ET9KDB_Init().
 * @retval ET9STATUS_ERROR              General error status.
 * @retval ET9STATUS_OUT_OF_RANGE       The value specified by wKeyIndex is not valid.
 * @retval ET9STATUS_FULL               The active word is already the maximum size allowed (ET9MAXWORDSIZE).
 * @retval ET9STATUS_READ_DB_FAIL       The XT9 core cannot read data from the keyboard. This typically indicates a problem with the ET9KDBREADCALLBACK() function you have implemented.
 *
 * @remarks This function applies to ambiguous input. Before calling this function, the integration layer must have previously initialized the Keyboard Input Module by calling ET9KDB_Init().
 * When the integration layer calls ET9KDB_ProcessTrace, the Keyboard Input Module stores information about the key that was traced in the instance of ET9WordSymbInfo pointed to by pWordSymbInfo.
 */

ET9STATUS ET9FARCALL ET9KDB_ProcessTrace(ET9KDBInfo             * const pKDBInfo,
                                         ET9WordSymbInfo        * const pWordSymbInfo,
                                         ET9TracePoint    const * const pPoints,
                                         const ET9UINT                  nPointCount,
                                         const ET9U8                    bCurrIndexInList,
                                         ET9SYMB                * const psFunctionKey)
{
    ET9STATUS eStatus;

    WLOG6(fprintf(pLogFile6, "ET9KDB_ProcessTrace\n");)

    eStatus = __ET9KDB_BasicValidityCheck(pKDBInfo, 1);

    if (eStatus) {
        return eStatus;
    }

    if (!psFunctionKey || !pWordSymbInfo || pWordSymbInfo->wInitOK != ET9GOODSETUP) {
        return ET9STATUS_INVALID_MEMORY;
    }

    if (!pKDBInfo->Private.pCurrLayoutInfo->nKeyAreaCount) {
        return ET9STATUS_KDB_HAS_NO_TRACEABLE_KEYS;
    }

    if (!pPoints) {
        return ET9STATUS_INVALID_MEMORY;
    }

    if (!nPointCount) {
        return ET9STATUS_NONE;
    }

    /* init */

    ++pKDBInfo->Private.nWmUseID;

    *psFunctionKey = 0;

    /* assure some key values */

    ET9Assert(pKDBInfo->Private.pCurrLayoutInfo->nKeyAreaCount);
    ET9Assert(pKDBInfo->Private.pCurrLayoutInfo->nRadius);
    ET9Assert(pKDBInfo->Private.pCurrLayoutInfo->nMinKeyWidth);
    ET9Assert(pKDBInfo->Private.pCurrLayoutInfo->nMinKeyHeight);
    ET9Assert(pKDBInfo->Private.pCurrLayoutInfo->nMedianKeyWidth);
    ET9Assert(pKDBInfo->Private.pCurrLayoutInfo->nMedianKeyHeight);

    /* copy and scale */

    eStatus = __TraceCopyToExt(pKDBInfo,
                               pKDBInfo->Private.wm.traceEvent.pTraceExtPoints,
                               &pKDBInfo->Private.wm.traceEvent.nTraceExtPointCount,
                               ET9_TRACE_MAX_POINTS,
                               pPoints,
                               nPointCount);

    if (eStatus) {
        return eStatus;
    }

    /* really just a key press? */

    if (__TraceSingleKey(pKDBInfo,
                         pWordSymbInfo,
                         bCurrIndexInList,
                         pKDBInfo->Private.wm.traceEvent.pTraceExtPoints,
                         pKDBInfo->Private.wm.traceEvent.nTraceExtPointCount,
                         &eStatus,
                         psFunctionKey)) {

        if (eStatus) {
            return eStatus;
        }

        if (!*psFunctionKey) {
            _ET9TrackInputEvents(pWordSymbInfo, ET9InputEvent_add);
        }

        return ET9STATUS_NONE;

    }

    /* calculate trace */

    eStatus = __TraceCalculate(pKDBInfo,
                               pKDBInfo->Private.wm.traceEvent.pTraceExtPoints,
                               &pKDBInfo->Private.wm.traceEvent.nTraceExtPointCount);

    if (eStatus) {
        return eStatus;
    }

    /* 'tap' the trace */

    eStatus = __TraceTap(pKDBInfo,
                         pWordSymbInfo,
                         pKDBInfo->Private.wm.traceEvent.pTraceExtPoints,
                         pKDBInfo->Private.wm.traceEvent.nTraceExtPointCount,
                         bCurrIndexInList);

    if (eStatus) {
        return eStatus;
    }

    _ET9TrackInputEvents(pWordSymbInfo, ET9InputEvent_add);

    /* done */

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal \ingroup et9trace
 * Get information about the last trace entered. This function is inteded for debug use and might become obsoleted.
 *
 * @param[in]     pKDBInfo          Pointer to the Keyboard Input Module Information Data Structure (ET9KDBInfo).
 * @param[in]     pWordSymbInfo     Pointer to the Word Symbol Data Structure (ET9WordSymbInfo).
 * @param[in,out] pPoints           Pointer to a list of trace points to receive trace positions.
 * @param[in]     nMaxPointCount    Max number of points that the pPoints array can hold.
 * @param[out]    pnPointCount      Pointer to receive the number of points in the list.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL _ET9KDB_GetLastTrace(ET9KDBInfo             * const pKDBInfo,
                                          ET9WordSymbInfo        * const pWordSymbInfo,
                                          ET9TracePoint          * const pPoints,
                                          const ET9UINT                  nMaxPointCount,
                                          ET9UINT                * const pnPointCount)
{
    ET9STATUS    eStatus;

    WLOG6(fprintf(pLogFile6, "_ET9KDB_GetLastTrace\n");)

    eStatus = __ET9KDB_BasicValidityCheck(pKDBInfo, 1);

    if (eStatus) {
        return eStatus;
    }

    if (!pWordSymbInfo || pWordSymbInfo->wInitOK != ET9GOODSETUP) {
        return ET9STATUS_INVALID_MEMORY;
    }

    if (!pPoints || !pnPointCount) {
        return ET9STATUS_INVALID_MEMORY;
    }

    if (nMaxPointCount < ET9MAXWORDSIZE) {
        return ET9STATUS_BAD_PARAM;
    }

    /* get trace points */

    {
        ET9U8         bCount;
        ET9U8         bCurrID;
        ET9SymbInfo   *pSymbInfo;
        ET9TracePoint *pPoint;

        *pnPointCount = 0;

        bCurrID = 0;
        pPoint = NULL;
        pSymbInfo = pWordSymbInfo->SymbsInfo;
        for (bCount = pWordSymbInfo->bNumSymbs; bCount ; --bCount, ++pSymbInfo) {

            if (pSymbInfo->bTraceIndex != bCurrID) {
                pPoint = pPoints;
                *pnPointCount = 0;
                bCurrID = pSymbInfo->bTraceIndex;
            }

            if (pSymbInfo->bTraceIndex && pSymbInfo->bTraceProbability) {

                ET9Assert(pPoint);

                if (pSymbInfo->wTapX == ET9UNDEFINEDTAPVALUE || pSymbInfo->wTapY == ET9UNDEFINEDTAPVALUE) {
                    __KeyAreasGetKeyCenter(pKDBInfo, pSymbInfo->wKeyIndex, &pPoint->nX, &pPoint->nY);
                }
                else {
                    pPoint->nX = pSymbInfo->wTapX;
                    pPoint->nY = pSymbInfo->wTapY;
                }

                pPoint->nX = (ET9UINT)__ScaleCoordinateToIntegrationX(pKDBInfo, pPoint->nX);
                pPoint->nY = (ET9UINT)__ScaleCoordinateToIntegrationY(pKDBInfo, pPoint->nY);

                ++pPoint;
                ++*pnPointCount;
            }
        }
    }

    /* done */

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \internal \ingroup et9trace
 * Get information about the last trace entered. This function is inteded for debug use and might become obsoleted.
 *
 * @param[in]     pKDBInfo          Pointer to the Keyboard Input Module Information Data Structure (ET9KDBInfo).
 * @param[in,out] pPoints           Pointer to a list of trace points to receive trace positions.
 * @param[in]     nMaxPointCount    Max number of points that the pPoints array can hold.
 * @param[out]    pnPointCount      Pointer to receive the number of points in the list.
 *
 * @return ET9STATUS_NONE on success, otherwise return ET9 error code.
 */

ET9STATUS ET9FARCALL _ET9KDB_GetLastTraceDbg(ET9KDBInfo             * const pKDBInfo,
                                             ET9TracePointDbg       * const pPoints,
                                             const ET9UINT                  nMaxPointCount,
                                             ET9UINT                * const pnPointCount)
{
    ET9STATUS    eStatus;

    WLOG6(fprintf(pLogFile6, "_ET9KDB_GetLastTraceDbg\n");)

    eStatus = __ET9KDB_BasicValidityCheck(pKDBInfo, 1);

    if (eStatus) {
        return eStatus;
    }

    if (!pPoints || !pnPointCount) {
        return ET9STATUS_INVALID_MEMORY;
    }

    if (nMaxPointCount < ET9_TRACE_MAX_POINTS) {
        return ET9STATUS_BAD_PARAM;
    }

    if (nMaxPointCount < pKDBInfo->Private.wm.traceEvent.nTraceExtPointCount) {
        return ET9STATUS_ERROR;
    }

    /* get trace points */

    {
        ET9UINT             nCount;
        ET9TracePointExt    *pPointExt;
        ET9TracePointDbg    *pPointDbg;

        *pnPointCount = pKDBInfo->Private.wm.traceEvent.nTraceExtPointCount;

        pPointDbg = pPoints;
        pPointExt = pKDBInfo->Private.wm.traceEvent.pTraceExtPoints;
        for (nCount = pKDBInfo->Private.wm.traceEvent.nTraceExtPointCount; nCount ; --nCount, ++pPointExt, ++pPointDbg) {

            pPointDbg->nX = (ET9UINT)(__ScaleCoordinateToIntegrationX_f(pKDBInfo, pPointExt->sPoint.fX) + 0.5);
            pPointDbg->nY = (ET9UINT)(__ScaleCoordinateToIntegrationY_f(pKDBInfo, pPointExt->sPoint.fY) + 0.5);
            pPointDbg->nQuality = pPointExt->nQuality;
            pPointDbg->bMergeWithPrev = pPointExt->bMergeWithPrev;
        }
    }

    /* done */

    return ET9STATUS_NONE;
}

/*---------------------------------------------------------------------------*/
/** \ingroup et9trace
 * Should be called just before calling ET9KDB_ProcessTrace to check if the current active word should be accepted and potentially a space should be inserted after the accepted word.
 * The pWordSymbInfo must still contain the information from previous word entry.
 *
 * @param[in]     pKDBInfo          Pointer to the Keyboard Input Module Information Data Structure (ET9KDBInfo).
 * @param[in]     pWordSymbInfo     Pointer to the Word Symbol Data Structure (ET9WordSymbInfo).
 * @param[in]     pPoints           Pointer to a list of trace points to process.
 * @param[in]     nPointCount       The number of points to handle.
 * @param[out]    pbAddSpace        If a space should be inserted after accepting.
 *
 * @return Non zero to insert space, otherwise zero.
 */

ET9BOOL ET9FARCALL ET9KDB_IsAutoAcceptBeforeTrace(ET9KDBInfo             * const pKDBInfo,
                                                  ET9WordSymbInfo        * const pWordSymbInfo,
                                                  ET9TracePoint    const * const pPoints,
                                                  const ET9UINT                  nPointCount,
                                                  ET9BOOL                * const pbAddSpace)
{
    if (__ET9KDB_BasicValidityCheck(pKDBInfo, 1)) {
        return 0;
    }

    if (!pWordSymbInfo || pWordSymbInfo->wInitOK != ET9GOODSETUP) {
        return 0;
    }

    if (!pbAddSpace) {
        return 0;
    }

    /* default to no space */

    *pbAddSpace = 0;

    /* not on empty incoming trace */

    if (!pPoints) {
        return 0;
    }

    /* not on empty WSI */

    if (!pWordSymbInfo->bNumSymbs) {
        return 0;
    }

    /* not during inhibit */

    if (pWordSymbInfo->Private.bRequiredInhibitCorrection) {
        return 0;
    }

    /* copy and scale */

    __TraceCopyToExt(pKDBInfo,
                     pKDBInfo->Private.wm.traceEvent.pTraceExtPoints,
                     &pKDBInfo->Private.wm.traceEvent.nTraceExtPointCount,
                     ET9_TRACE_MAX_POINTS,
                     pPoints,
                     nPointCount);

    /* not on empty trace */

    if (!pKDBInfo->Private.wm.traceEvent.nTraceExtPointCount) {
        return 0;
    }

    /* not if trace is a single key */

    {
        const ET9BOOL bIsSingleKey = __TraceIsSingleKey(pKDBInfo,
                                                        pKDBInfo->Private.wm.traceEvent.pTraceExtPoints,
                                                        pKDBInfo->Private.wm.traceEvent.nTraceExtPointCount,
                                                        NULL,
                                                        NULL);

        if (bIsSingleKey) {
            return 0;
        }
    }

    /* not if trace starts on punct */

    {
        ET9UINT nKey;
        ET9TracePoint_f const * const pPoint = &pKDBInfo->Private.wm.traceEvent.pTraceExtPoints[0].sPoint;

        const ET9BOOL bKeyFound = __TraceGetKey(pKDBInfo, pPoint->fX, pPoint->fY, &nKey);

        if (bKeyFound) {

            ET9KdbAreaInfo const * pArea = __GetKeyAreaFromKey_Generic(pKDBInfo, (ET9U16)nKey);

            if (pArea && (pArea->eKeyType == ET9KTPUNCTUATION || pArea->eKeyType == ET9KTSMARTPUNCT)) {
                return 0;
            }
        }
    }

    /* otherwise yes */

    *pbAddSpace = 1;    /* for now we suggest space in pair with accept */

    return 1;
}


#endif  /* ET9_KDB_TRACE_MODULE */

/*! @} */
/* ----------------------------------< eof >--------------------------------- */
