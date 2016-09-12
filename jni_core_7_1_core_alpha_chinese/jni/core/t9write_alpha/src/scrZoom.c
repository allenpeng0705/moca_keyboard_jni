/*******************************************************************************
;*******************************************************************************
;**                                                                           **
;**                    COPYRIGHT 2010 NUANCE COMMUNICATIONS                   **
;**                                                                           **
;**                NUANCE COMMUNICATIONS PROPRIETARY INFORMATION              **
;**                                                                           **
;**     This software is supplied under the terms of a license agreement      **
;**     or non-disclosure agreement with Nuance Communications and may not    **
;**     be copied or disclosed except in accordance with the terms of that    **
;**     agreement.                                                            **
;**                                                                           **
;*******************************************************************************
;******************************************************************************/
/*$Header: /data/cvsroot/seattle/t9write/alphabetic/core/src/scrZoom.c,v 1.5 2011/02/14 11:41:14 jianchun_meng Exp $*/

#include "scrZoom.h"
#include "scrCurve.h"
#include "scrOutput.h"
#include "database.h"
#include "scrProxCurve.h"
#include "scrAlgorithm.h"
#include "scrFineSearch.h"
#include "decumaMemory.h"
#include "decumaMath.h"
#include "decumaAssert.h"
#include "decumaDataTypes.h"
#include "scr_packet_switches.h"
#include "scrLigature.h"
#include "globalDefs.h"

#define EPS 1
#define SIN_ANGLE_SCALE 1000
#define SVM_CURVE_TRAINING_SIZE		(100)
#define SVM_ROUNDING_FACTOR			(256)
#define SVM_ZOOM_LIMIT  (100) /*JM : Note temporary hack */

/*#define DEBUG_OUTPUT */
#ifdef DEBUG_OUTPUT
#include <stdio.h>
extern FILE * g_fpDebug;
#endif

/*///////////////////////////// */
/*     Local prototypes      // */
/*///////////////////////////// */

static ZOOM_FUNC getZoomFunc(int zoomFuncNbr);

static int svmTest(const SCR_CURVE * pCurve, DECUMA_INT8_DB_PTR pNormal,
	DECUMA_INT32 bias, DECUMA_UINT32 normalScale, DECUMA_INT16 offsetForNormal, DECUMA_INT8 biasShift, DECUMA_INT8 normalOffsetShift);

static void		copy_arc(const SCR_ARC *pArc, DECUMA_INT16 *x, DECUMA_INT16 *y, DECUMA_INT16 *dx, DECUMA_INT16 *dy, int n);
static void		flip_arc(const SCR_ARC *pArc, DECUMA_INT16 *x, DECUMA_INT16 *y, DECUMA_INT16 *dx, DECUMA_INT16 *dy, int n);

static int		go_left(int i, const DECUMA_INT16 *x, const DECUMA_INT16 *y, const DECUMA_INT16 *dx, const DECUMA_INT16 *dy,
		DECUMA_INT16 xlimlow, DECUMA_INT16 xlimhigh, DECUMA_INT16 ylimlow,	DECUMA_INT16 ylimhigh, DECUMA_BOOL handleTwitch);
static int		go_right(int i, const DECUMA_INT16 *x, const DECUMA_INT16 *y, const DECUMA_INT16 *dx, const DECUMA_INT16 *dy,
		DECUMA_INT16 xlimlow, DECUMA_INT16 xlimhigh, DECUMA_INT16 ylimlow,	DECUMA_INT16 ylimhigh, DECUMA_BOOL handleTwitch);
static int		go_up(int i, const DECUMA_INT16 *x, const DECUMA_INT16 *y, const DECUMA_INT16 *dx, const DECUMA_INT16 *dy,
		DECUMA_INT16 xlimlow, DECUMA_INT16 xlimhigh, DECUMA_INT16 ylimlow,	DECUMA_INT16 ylimhigh, DECUMA_BOOL handleTwitch);
static int		go_down(int i, const DECUMA_INT16 *x, const DECUMA_INT16 *y, const DECUMA_INT16 *dx, const DECUMA_INT16 *dy,
		DECUMA_INT16 xlimlow, DECUMA_INT16 xlimhigh, DECUMA_INT16 ylimlow,	DECUMA_INT16 ylimhigh, DECUMA_BOOL handleTwitch);
static int		go_horisontal(int i, const DECUMA_INT16 *x, const DECUMA_INT16 *y, const DECUMA_INT16 *dx, const DECUMA_INT16 *dy,
		DECUMA_INT16 xlimlow, DECUMA_INT16 xlimhigh, DECUMA_INT16 ylimlow, DECUMA_INT16 ylimhigh,
							  DECUMA_BOOL handleTwitch, int dir);

static double	integrateArc(const DECUMA_INT16 *x, const DECUMA_INT16 *y, int si, int ei, DECUMA_BOOL rightHandedSystem, DECUMA_BOOL *error);
static double	triangleArea(const DECUMA_INT16 *x, const DECUMA_INT16 *y, int i1, int i2, int i3, DECUMA_BOOL *error);
static int      sinAngle(const DECUMA_INT16 *x, const DECUMA_INT16 *y, int a, int b, int c);
static DECUMA_INT32	findWidestHorisontal(const DECUMA_INT16 *x, const DECUMA_INT16 *y, int startInd, int stopInd, int centerInd);
static DECUMA_BOOL		peak(const DECUMA_INT16 *dx, const DECUMA_INT16 *dy, int i, unsigned int thresh);
static DECUMA_BOOL		reachedLim(DECUMA_INT16 x, DECUMA_INT16 y, DECUMA_INT16 xlimlow, DECUMA_INT16 xlimhigh, DECUMA_INT16 ylimlow, DECUMA_INT16 ylimhigh);
static DECUMA_BOOL		sameSignVec(const DECUMA_INT16 *n, int nLen);
static DECUMA_BOOL		sameSign(DECUMA_INT16 n1, DECUMA_INT16 n2);
static double	sign_double(double n);
static DECUMA_INT16	sign(DECUMA_INT16 n);

/******** Zoom functions **********/

static void testBetter_2_than_Z(ZOOM_IN * pZoomIn, ZOOM_OUT * pZoomOut);

static void testBetter_4_than_h(ZOOM_IN * pZoomIn, ZOOM_OUT * pZoomOut);

static void testBetter_n_than_h(ZOOM_IN * pZoomIn, ZOOM_OUT * pZoomOut);

static void testBetter_a_than_u(ZOOM_IN * pZoomIn, ZOOM_OUT * pZoomOut);

static void testBetter_g_than_y(ZOOM_IN * pZoomIn, ZOOM_OUT * pZoomOut);

static void testBetter_r_than_v(ZOOM_IN * pZoomIn, ZOOM_OUT * pZoomOut);

static void testBetter_u_than_v(ZOOM_IN * pZoomIn, ZOOM_OUT * pZoomOut);

static void testBetter_f_than_t(ZOOM_IN * pZoomIn, ZOOM_OUT * pZoomOut);

static void testBetter_dad_than_feh(ZOOM_IN * pZoomIn, ZOOM_OUT * pZoomOut);

static void testBetter_yeh_than_kaf(ZOOM_IN * pZoomIn, ZOOM_OUT * pZoomOut);

static void testBetter_dal_than_lam(ZOOM_IN * pZoomIn, ZOOM_OUT * pZoomOut);

static void testBetter_4_than_9(ZOOM_IN * pZoomIn, ZOOM_OUT * pZoomOut);

static void testBetter_seen_than_farsi_yeh(ZOOM_IN * pZoomIn, ZOOM_OUT * pZoomOut);

static void testBetter_dad_than_feh_c(ZOOM_IN * pZoomIn, ZOOM_OUT * pZoomOut); /* Cyrus version */

static void testBetter_thal_than_zain(ZOOM_IN * pZoomIn, ZOOM_OUT * pZoomOut);

static void testBetter_lam_than_yeh(ZOOM_IN * pZoomIn, ZOOM_OUT * pZoomOut);

static void testBetter_dal_than_reh(ZOOM_IN * pZoomIn, ZOOM_OUT * pZoomOut);

static void testBetter_teh_than_tteh(ZOOM_IN * pZoomIn, ZOOM_OUT * pZoomOut);

static void testBetter_teh_than_tteh_3arced(ZOOM_IN * pZoomIn, ZOOM_OUT * pZoomOut);

static void testBetter_ghain_than_khah(ZOOM_IN * pZoomIn, ZOOM_OUT * pZoomOut);

static void testBetter_aitwo_than_aithree(ZOOM_IN * pZoomIn, ZOOM_OUT * pZoomOut);

static void testBetter_1dot_than_3dots(ZOOM_IN * pZoomIn, ZOOM_OUT * pZoomOut);

static void testBetter_bet_than_final_kaf(ZOOM_IN * pZoomIn, ZOOM_OUT * pZoomOut);

static void testLongVertical(ZOOM_IN * pZoomIn, ZOOM_OUT * pZoomOut);

static void testNothing(ZOOM_IN * pZoomIn, ZOOM_OUT * pZoomOut);

static void zoomPartOfCurve(ZOOM_IN * pZoomIn, ZOOM_OUT * pZoomOut);

static void zoomPartOfCurveAllographSpecific(ZOOM_IN * pZoomIn, ZOOM_OUT * pZoomOut);

static void zoomMinCut(ZOOM_IN * pZoomIn, ZOOM_OUT * pZoomOut);

static void SVMZoom(ZOOM_IN * pZoomIn, ZOOM_OUT * pZoomOut);

static void testBetter_dalet_than_tsade(ZOOM_IN * pZoomIn, ZOOM_OUT * pZoomOut);

static void testBetter_final_tsade_than_final_mem(ZOOM_IN * pZoomIn, ZOOM_OUT * pZoomOut);

static void testBetter_final_tsade_than_final_fay(ZOOM_IN * pZoomIn, ZOOM_OUT * pZoomOut);

/*********** global code ****************/

void zoomFilter(scrOUTPUT * pOut, const int nOut, const SCR_CURVE * pCurve,
               int nRefAngle, int nRefAngleImportance, int nMaxCuts)
{
	/*
	The main zoom function that finds which zooming that should be done,
	calls the zoom function and adjusts the outputs based upon the outcome
	of the function.

	NOTE: that pOut must point to an array with the size >= 2
	*/

	int defenderIdx = 0, challengerIdx = 1;

	/* Has both expanded to orignal + alternative. If so adjust zoomees. */
	/* If e.g. oOuU test ou instead of oO for swap */
	if (nOut >= challengerIdx+1 && pOut[defenderIdx].symbol[0] == pOut[challengerIdx].symbol[0])
	{
		challengerIdx++;
	}

	if (nOut >= challengerIdx+1 &&
		databaseValidKID(pOut[defenderIdx].DBindex) &&
		databaseValidKID(pOut[challengerIdx].DBindex) &&
		nRefAngle==0 &&
		nRefAngleImportance > 0 &&
		/* Do not zoom if we have a conflict */
		!outputConflict(outputGetConflictData(&pOut[challengerIdx]), pOut) )
	{
		CH_PAIR_ZOOM_INFO_PTR pZoomInfo;

		DECUMA_INT8 likelyCandidateIfTestPasses;

		/*Find zoomFuncNumber and arguments from the database */
		pZoomInfo = kidGetZoomInfo(&pOut[defenderIdx].DBindex,
			&pOut[challengerIdx].DBindex, &likelyCandidateIfTestPasses);


		if (pZoomInfo)
		{
			/* It was found in the database that zooming should be done between */
			/* this pair of keys */
			ZOOM_FUNC zoomFunc = getZoomFunc( pZoomInfo->functionNr );

			if ( zoomFunc )
			{
				ZOOM_IN zoomIn;
				ZOOM_OUT zoomOut={0};

				/* build the temporary curve */
				SCR_CURVE curveCut;
				SCR_ARC curveArcs[MAX_NUMBER_OF_ARCS_IN_CURVE];
				int j;

				myMemSet(curveArcs,0,sizeof(curveArcs));
				myMemSet(&curveCut,0,sizeof(curveCut));
				for(j = 0 ; j < MAX_NUMBER_OF_ARCS_IN_CURVE; j++)
					curveCut.Arcs[j] = &curveArcs[j];
				curveCopy(&curveCut,pCurve);

				/*Fill zoom-in */
				zoomIn.likelyCandidateIfTestPasses = likelyCandidateIfTestPasses;
				zoomIn.functionArg1 = pZoomInfo->functionArg1;
				zoomIn.functionArg2 = pZoomInfo->functionArg2;
				zoomIn.pCurve = pCurve;
				zoomIn.pCurveCut = &curveCut;
				zoomIn.nMaxCuts = nMaxCuts;
				zoomIn.pOutput = pOut;

				/*Call the zoom function */
				zoomFunc( &zoomIn, &zoomOut );

				pOut[defenderIdx].zoomValue=1;
				pOut[challengerIdx].zoomValue=1;

				/*Adjust the outputs with respect to zoomOut */
				if (zoomOut.bestCandidate == 1)
				{
					int a, nOffset = 1, nAdjustments = 1;

					/* If e.g. oOuU swap OU too */
					if (nOut >= challengerIdx+2 &&
						challengerIdx == defenderIdx + 2 &&
						pOut[challengerIdx].symbol[0] == pOut[challengerIdx+1].symbol[0])
					{
						nAdjustments++;
					}

					/* If e.g. ouOU swap OU too */
					if (nOut >= challengerIdx+3 &&
						challengerIdx == defenderIdx + 1 &&
						pOut[challengerIdx].symbol[0] == pOut[challengerIdx+2].symbol[0])
					{
						nAdjustments++;
						nOffset = 2;
					}

					for (a = 0; a < nAdjustments; a++)
					{
						scrOUTPUT temp = pOut[defenderIdx+a*nOffset];

						pOut[defenderIdx+a*nOffset] = pOut[challengerIdx+a*nOffset];
						pOut[challengerIdx+a*nOffset] = temp;
						pOut[challengerIdx+a*nOffset].mu = pOut[defenderIdx+a*nOffset].mu;
						pOut[challengerIdx+a*nOffset].punish = pOut[defenderIdx+a*nOffset].punish;

						pOut[defenderIdx+a*nOffset].mu = temp.mu;
						pOut[defenderIdx+a*nOffset].punish = temp.punish;

						/*Add information that zooming has changed the candidate orders */
						pOut[defenderIdx+a*nOffset].zoomValue=2;
						pOut[challengerIdx+a*nOffset].zoomValue=2;
						/*swapped = 1; */
					}
				}
			}
			else
			{
				decumaAssert(0); /*Something is unsynchronized between database zoom info and scr */
				return;
			}
		}
	}
}


/************** local code ************************/

static ZOOM_FUNC getZoomFunc(int zoomFuncNbr)
{
	/* Microchip compiler did not like a case statement here... (?!) */
		     if ( zoomFuncNbr == 0)  return testBetter_a_than_u;
		else if ( zoomFuncNbr == 1)  return testBetter_g_than_y;
		else if ( zoomFuncNbr == 2)  return testBetter_r_than_v;
		else if ( zoomFuncNbr == 3)  return testBetter_u_than_v;
		else if ( zoomFuncNbr == 4)  return testLongVertical;
		else if ( zoomFuncNbr == 5)  return testNothing;
		else if ( zoomFuncNbr == 6)  return zoomPartOfCurve;
		else if ( zoomFuncNbr == 7)  return zoomPartOfCurveAllographSpecific;
		else if ( zoomFuncNbr == 8)  return zoomMinCut;
		else if ( zoomFuncNbr == 9)  return testBetter_n_than_h;
		else if ( zoomFuncNbr == 10) return testBetter_2_than_Z;
		else if ( zoomFuncNbr == 11) return testBetter_4_than_h;
		else if ( zoomFuncNbr == 12) return testBetter_f_than_t;
		else if ( zoomFuncNbr == 13) return testBetter_yeh_than_kaf;
		else if ( zoomFuncNbr == 14) return testBetter_dal_than_lam;
		else if ( zoomFuncNbr == 15) return testBetter_dad_than_feh;
		else if ( zoomFuncNbr == 16) return testBetter_4_than_9;
		else if ( zoomFuncNbr == 17) return testBetter_seen_than_farsi_yeh;
		else if ( zoomFuncNbr == 18) return testBetter_thal_than_zain;
		else if ( zoomFuncNbr == 19) return testBetter_lam_than_yeh;
		else if ( zoomFuncNbr == 20) return testBetter_dal_than_reh;
		else if ( zoomFuncNbr == 21) return testBetter_teh_than_tteh;
		else if ( zoomFuncNbr == 22) return testBetter_ghain_than_khah;
		else if ( zoomFuncNbr == 23) return testBetter_aitwo_than_aithree;
		else if ( zoomFuncNbr == 24) return testBetter_1dot_than_3dots;
		else if ( zoomFuncNbr == 25) return testBetter_bet_than_final_kaf;
		else if ( zoomFuncNbr == 26) return testBetter_dalet_than_tsade;
		else if ( zoomFuncNbr == 27) return testBetter_final_tsade_than_final_mem;
		else if ( zoomFuncNbr == 28) return testBetter_final_tsade_than_final_fay;
		else if ( zoomFuncNbr == 29) return SVMZoom;
		else return NULL; /* No zoom function with this index. */
}



/*//////////////// Different zoom functions /////////////////// */


static void SVMZoom(ZOOM_IN * pZoomIn, ZOOM_OUT * pZoomOut)
{
	/* This function calculates the projection of the zoom pair on to the
	SVM hyperplane, represented by a normal and a bias.
	Reference to the parameter struct is in pZoomIn->MeasureID
	*/
	PROPERTY_DB_HEADER_PTR pPropDB = pZoomIn->pOutput[0].DBindex.pPropDB;

	SVM_PARAMETERS_STRUCT_PTR pSVMParametersStruct =
		propDBGetSvmParameterStruct( pPropDB, pZoomIn->functionArg1);

	int nArcs = pZoomIn->pCurve->noArcs;

	SVM_NORMAL_LIST_PTR pSVMNormalList =
		propDBGetSvmNormalList(pPropDB, nArcs);

	DECUMA_INT8_DB_PTR pNormal = (DECUMA_INT8_DB_PTR)
		((DECUMA_DB_ADDRESS) pPropDB + pSVMNormalList->normalOffset);

	int normalIndex = pSVMParametersStruct->normalIndex;

	DECUMA_INT32 bias = pSVMParametersStruct->bias;
	DECUMA_UINT32 normalScale = pSVMParametersStruct->normalScale;
	DECUMA_INT16 offsetForNormal = pSVMParametersStruct->offsetForNormal;
	DECUMA_INT8 biasShift = pSVMParametersStruct->biasShift;
	DECUMA_INT8 normalOffsetShift = pSVMParametersStruct->normalOffsetShift;

	int testResult;

	decumaAssert( normalIndex >= 0);
	decumaAssert( normalIndex < pSVMNormalList->nNormals);


	pZoomOut->zoomQuality = 0; /*Not used; */
	pZoomOut->bestCandidate = 0;

	if ( pZoomIn->pOutput[1].mu + pZoomIn->pOutput[1].punish
		- (pZoomIn->pOutput[0].mu + pZoomIn->pOutput[0].punish)
		> SVM_ZOOM_LIMIT)
	{
		/*Don't do SVM since proximity measure was quite discriminating */
		/*Quick hack to avoid bad zooming functions */

		pZoomOut->bestCandidate = 0; /*Don't change order */
		return;
	}

	pNormal += normalIndex*(NUMBER_OF_POINTS_IN_ARC*4*nArcs);

	/* SVM-TEST */
	testResult = svmTest( pZoomIn->pCurve, pNormal, bias, normalScale, offsetForNormal, biasShift, normalOffsetShift);

	/*/// Write out data ///// */

	if (testResult == 1)
	{
		pZoomOut->bestCandidate = pZoomIn->likelyCandidateIfTestPasses;
	}
	else
	{
		pZoomOut->bestCandidate = !pZoomIn->likelyCandidateIfTestPasses;
	}
}

static int svmTest(const SCR_CURVE * pCurve, DECUMA_INT8_DB_PTR pNormal,
	DECUMA_INT32 bias, DECUMA_UINT32 normalScale, DECUMA_INT16 offsetForNormal,
	DECUMA_INT8 biasShift, DECUMA_INT8 normalOffsetShift)
{



	double scalar = 0;
	double testResult;
	DECUMA_INT16 * px;
	DECUMA_INT16 * py;
	DECUMA_INT16 * pdx;
	DECUMA_INT16 * pdy;
	int calc;
	int i,j;
	DECUMA_INT16 xMin, xMax, yMin, yMax;
	DECUMA_INT32 width, height,maxSize;
	double curveScaleFactor;
	double normalOffset = ((double)offsetForNormal/(1<<normalOffsetShift))/normalScale;
	double shiftedBias;

	curveMinMaxCalculate(pCurve, pCurve->noArcs,
		&xMin, &xMax, &yMin, &yMax);

	width = xMax-xMin;
	height = yMax-yMin;
	maxSize = width > height ? width : height;

	curveScaleFactor = (double) SVM_CURVE_TRAINING_SIZE/maxSize;

	for(i = 0 ; i < curveGetNoArcs(pCurve); i++){

		int alpha_x = pCurve->Arcs[i]->alpha_x;
		int alpha_y = pCurve->Arcs[i]->alpha_y;
		int alpha_dx = pCurve->Arcs[i]->alpha_dx;
		int alpha_dy = pCurve->Arcs[i]->alpha_dy;

		px = &pCurve->Arcs[i]->x[0];
		py = &pCurve->Arcs[i]->y[0];

		calc = 2*i*NUMBER_OF_POINTS_IN_ARC;

		for(j = 0; j < NUMBER_OF_POINTS_IN_ARC; j++)
		{
			double normal_x = (double)pNormal[j+calc]/normalScale+normalOffset;
			double normal_y = (double)pNormal[j+calc+32]/normalScale+normalOffset;

			scalar += (alpha_x*alpha_x*normal_x*(double)px[j]*curveScaleFactor+
				alpha_y*alpha_y*normal_y*(double)py[j]*curveScaleFactor) / 64;
		}

		 /* Derivate */
		pdx = &pCurve->Arcs[i]->dx[0];
		pdy = &pCurve->Arcs[i]->dy[0];

		for(j = 0; j < NUMBER_OF_POINTS_IN_ARC; j++){
			double normal_dx = (double)pNormal[j+64+calc]/normalScale+normalOffset;
			double normal_dy = (double)pNormal[j+calc+96]/normalScale+normalOffset;

			scalar += (alpha_dx*alpha_dx*normal_dx*(double)pdx[j]*curveScaleFactor+
				alpha_dy*alpha_dy*normal_dy*(double)pdy[j]*curveScaleFactor) / 64;
		}

	}

	shiftedBias = (double)bias /(1<<biasShift);
	testResult = scalar - shiftedBias;
	if(testResult > 0)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

static void zoomPartOfCurve(ZOOM_IN * pZoomIn, ZOOM_OUT * pZoomOut)
{
	/* This takes the argument two arguments from the zoomtable in
	the database: measureId = pZoomIn->functionArg1 and alpha = pZoomIn->functionArg2.
	The measureID says which part of the curve that a proximity measurement
	should be done on.
	Then alpha says how much the derivative should be weighed into the measure.
	It first cuts the curve with a maximal of pZoomIn->nMaxCuts cuts.
	*/


	int j;
	DECUMA_INT16 measureId;
	DECUMA_INT32 alpha;
	scrOUTPUT proxData[2];

	pZoomOut->zoomQuality = 0; /*Not used; */
	pZoomOut->bestCandidate = 0; /*No swap; */

	measureId = pZoomIn->functionArg1;
	alpha = pZoomIn->functionArg2;

	/* Set curve measureId and alpha according to zoomInfo specification */
	for(j = 0; j < pZoomIn->pCurveCut->noArcs; j++)
	{
		if (alpha != pZoomIn->pCurveCut->Arcs[j]->alpha_dx * pZoomIn->pCurveCut->Arcs[j]->alpha_dx / 64 ||
			alpha != pZoomIn->pCurveCut->Arcs[j]->alpha_dy * pZoomIn->pCurveCut->Arcs[j]->alpha_dy / 64)
		{
			/* TODO fix db format to be more specific */
			pZoomIn->pCurveCut->Arcs[j]->alpha_dx = decumaSqrt(64 * alpha);
			pZoomIn->pCurveCut->Arcs[j]->alpha_dy = decumaSqrt(64 * alpha);
		}
	}


	for(j = 0; j < 2; j++)
	{
		const KID * kid = &pZoomIn->pOutput[j].DBindex;
		int arcToCut=0;

		proxData[j] = pZoomIn->pOutput[j];
		proxData[j].punish = 0;

		/* Let GausNewton trim the curve ligatures, and build the new curve in curveCut */
		GaussNewton(&proxData[j], measureId,
			arcToCut, pZoomIn->pCurve, pZoomIn->pCurveCut,
			NULL,0,0,0,pZoomIn->nMaxCuts);

		proxCurveKeyWithoutPreCalculatedKey(&proxData[j].mu,
			&proxData[j].simTransf, pZoomIn->pCurveCut,
			kid, measureId,
			proxData[j].arcOrder);
	}

	if((proxData[0].mu + proxData[0].punish > proxData[1].mu + proxData[1].punish))
	{
		pZoomOut->bestCandidate = 1;
	}
}

static void testLongVertical(ZOOM_IN * pZoomIn, ZOOM_OUT * pZoomOut)
{
}

static void zoomPartOfCurveAllographSpecific( ZOOM_IN * pZoomIn, ZOOM_OUT * pZoomOut )
{
}

static void testNothing( ZOOM_IN * pZoomIn, ZOOM_OUT * pZoomOut)
{
	/*This is a stupid function that just switches if one */
	/*symbol is preferred to another. If possible this switching */
	/*criterion should be replaced. */

	pZoomOut->zoomQuality = 0; /*Not used; */
	pZoomOut->bestCandidate = pZoomIn->likelyCandidateIfTestPasses;
}

static void zoomMinCut(ZOOM_IN * pZoomIn, ZOOM_OUT * pZoomOut)
{
	int j;
	DECUMA_INT16 measureId;
	DECUMA_INT32 alpha;
	scrOUTPUT proxData[2];
	const int minCutLeft = maxNUMB(minNUMB(pZoomIn->pOutput[0].nCutLeft,
		pZoomIn->pOutput[1].nCutLeft),0);
	const int minCutRight = maxNUMB(minNUMB(pZoomIn->pOutput[0].nCutRight,
		pZoomIn->pOutput[1].nCutRight),0);
	pZoomOut->zoomQuality = 0; /*Not used; */
	pZoomOut->bestCandidate = 0; /*No swap; */

	measureId = pZoomIn->functionArg1;
	alpha = pZoomIn->functionArg2;

	/* Set curve measureId and alpha according to zoomInfo specification */
	for(j = 0; j < pZoomIn->pCurveCut->noArcs; j++)
	{
		if (alpha != pZoomIn->pCurveCut->Arcs[j]->alpha_dx * pZoomIn->pCurveCut->Arcs[j]->alpha_dx / 64 ||
			alpha != pZoomIn->pCurveCut->Arcs[j]->alpha_dy * pZoomIn->pCurveCut->Arcs[j]->alpha_dy / 64)
		{
			/* TODO fix db format to be more specific */
			pZoomIn->pCurveCut->Arcs[j]->alpha_dx = decumaSqrt(64 * alpha);
			pZoomIn->pCurveCut->Arcs[j]->alpha_dy = decumaSqrt(64 * alpha);
		}
	}

	for(j = 0; j < 2; j++)
	{
		int arcToCutK = 0; /*KEYindexed */
		int arcToCutC; /*CURVEindexed */
		const KID * kid;
		int scaledCutLeft, scaledCutRight;

		proxData[j] = pZoomIn->pOutput[j];
		proxData[j].punish = 0;

		proxData[j].nCutLeft = minNUMB(proxData[j].nCutLeft,minCutLeft);
		proxData[j].nCutRight = minNUMB(proxData[j].nCutRight,minCutRight);

		scaledCutLeft = (int)(proxData[j].nCutLeft * ACCURACY_IN_INTERPOLATE +
			(NUMBER_OF_POINTS_IN_ARC - 1)/2)/(NUMBER_OF_POINTS_IN_ARC - 1);
		scaledCutRight= (int)(proxData[j].nCutRight * ACCURACY_IN_INTERPOLATE +
			(NUMBER_OF_POINTS_IN_ARC - 1)/2)/(NUMBER_OF_POINTS_IN_ARC - 1);

		kid = &proxData[j].DBindex;

		arcToCutC = getArcNr( proxData[j].arcOrder,arcToCutK );

		InterpolateArc(pZoomIn->pCurveCut->Arcs[ arcToCutC ],
			pZoomIn->pCurve->Arcs[ arcToCutC ], scaledCutLeft,
			ACCURACY_IN_INTERPOLATE - scaledCutRight);
		buildCurveMutation(pZoomIn->pCurveCut, measureId,proxData[j].arcOrder);
		proxCurveKeyWithoutPreCalculatedKey(&proxData[j].mu, NULL, pZoomIn->pCurveCut, kid,
			measureId, proxData[j].arcOrder);
	}

	if((proxData[0].mu + proxData[0].punish > proxData[1].mu + proxData[1].punish))
	{
		pZoomOut->bestCandidate = 1;
	}
}

/* Functions of new zoom */

static void testBetter_2_than_Z(ZOOM_IN * pZoomIn, ZOOM_OUT * pZoomOut)
{
	DECUMA_INT16 xlimlow, xlimhigh, ylimlow, ylimhigh;
	int i, zoomInd1, zoomInd2, zoomInd3;
	double area1, area2;
	DECUMA_BOOL handleTwitch, error;

	DECUMA_INT16 x[NUMBER_OF_POINTS_IN_ARC];
	DECUMA_INT16 y[NUMBER_OF_POINTS_IN_ARC];
	DECUMA_INT16 dx[NUMBER_OF_POINTS_IN_ARC];
	DECUMA_INT16 dy[NUMBER_OF_POINTS_IN_ARC];

	SCR_ARC * pArc = pZoomIn->pCurve->Arcs[0];

	pZoomOut->zoomQuality = 0; /*Not used; */
	pZoomOut->bestCandidate = 0;

	if( !pArc )
	{
		decumaAssert(0); /* NULL-pointers supported but might be used unintended. */
						 /* Avoid this in debug mode for now. */
		return;
	}

	if( pZoomIn->pCurve->noArcs > 1 )
	{
		/* This case is not handled by this function */
		return;
	}

	flip_arc(pArc, x, y, dx, dy,
		NUMBER_OF_POINTS_IN_ARC); /* Go backwards */

	i = 0;

	xlimlow  = MIN_DECUMA_INT16;
	xlimhigh = MAX_DECUMA_INT16;
	ylimlow  = MIN_DECUMA_INT16;
	ylimhigh = MAX_DECUMA_INT16;

	handleTwitch = TRUE; /* Handle twitches */

	/* Remove peak in the end to avoid algorithm crash */
	while( peak(dx, dy, i + 1, 50) )
	{
		i += 1;
	}

	i = go_right(i, x, y, dx, dy,
			  xlimlow, xlimhigh, ylimlow,
			  ylimhigh, handleTwitch);

	i = go_left(i, x, y, dx, dy,
			  xlimlow, xlimhigh, ylimlow,
			  ylimhigh, handleTwitch);

	zoomInd3 = i;

	i = go_right(i, x, y, dx, dy,
			  xlimlow, xlimhigh, ylimlow,
			  ylimhigh, handleTwitch);

	zoomInd2 = i;

	i = go_left(i, x, y, dx, dy,
		  	    xlimlow, xlimhigh, ylimlow,
			    ylimhigh, handleTwitch);

	zoomInd1 = i;

	if( (zoomInd1 < 0) || (zoomInd1 >= NUMBER_OF_POINTS_IN_ARC) ) return;
	if( (zoomInd2 < 0) || (zoomInd2 >= NUMBER_OF_POINTS_IN_ARC) ) return;
	if( (zoomInd3 < 0) || (zoomInd3 >= NUMBER_OF_POINTS_IN_ARC) ) return;

	if( !(zoomInd1 > zoomInd2 && zoomInd2 > zoomInd3) )
	{
		/* Strange... Do not try to zoom this curve */
		decumaAssert(0);
		return;
	}

	if( y[zoomInd1] <= y[zoomInd3] || y[zoomInd2] <= y[zoomInd3] )
	{
		/* Strange... Do not try to zoom this curve */
		decumaAssert(0);
		return;
	}

	if( x[zoomInd2] <= x[zoomInd1] || x[zoomInd2] <= x[zoomInd3] )
	{
		/* Strange... Do not try to zoom this curve */
		decumaAssert(0);
		return;
	}

	area1 = integrateArc(x,y,zoomInd2,zoomInd1,FALSE,&error);
	area2 = triangleArea(x,y,zoomInd3,zoomInd2,zoomInd1,&error);

	if( error )
	{
		decumaAssert(0);
		return;
	}

	if(area2 <= 0.0 )
	{
		/* Strange... */
		decumaAssert(0);
		return;
	}

	if( area1 / area2 >= 0.4 )
	{
		/*2 */
		pZoomOut->bestCandidate = pZoomIn->likelyCandidateIfTestPasses;
	}

	if( area1 / area2 < 0.1 )
	{
		/*Z */
		pZoomOut->bestCandidate = !pZoomIn->likelyCandidateIfTestPasses;
	}
}

static void testBetter_4_than_h(ZOOM_IN * pZoomIn, ZOOM_OUT * pZoomOut)
{
	DECUMA_INT16 xlimlow, xlimhigh, ylimlow, ylimhigh;
	int i, zoomInd1, zoomInd2, zoomInd3, zoomInd4;
	DECUMA_BOOL handleTwitch;

	DECUMA_INT16 x[NUMBER_OF_POINTS_IN_ARC];
	DECUMA_INT16 y[NUMBER_OF_POINTS_IN_ARC];
	DECUMA_INT16 dx[NUMBER_OF_POINTS_IN_ARC];
	DECUMA_INT16 dy[NUMBER_OF_POINTS_IN_ARC];

	SCR_ARC * pArc = pZoomIn->pCurve->Arcs[0];

	pZoomOut->zoomQuality = 0; /*Not used; */
	pZoomOut->bestCandidate = 0;

	if( !pArc )
	{
		decumaAssert(0); /* NULL-pointers supported but might be used unintended. */
						 /* Avoid this in debug mode for now. */
		return;
	}

	if( pZoomIn->pCurve->noArcs > 1 )
	{
		/* This case is not handled by this function */
		return;
	}

	flip_arc(pArc, x, y, dx, dy,
		NUMBER_OF_POINTS_IN_ARC); /* Go backwards */

	i = 0;

	xlimlow  = MIN_DECUMA_INT16;
	xlimhigh = MAX_DECUMA_INT16;
	ylimlow  = MIN_DECUMA_INT16;
	ylimhigh = MAX_DECUMA_INT16;

	handleTwitch = TRUE; /* Handle twitches */

	/* Remove peak in the end to avoid algorithm crash */
	while( peak(dx, dy, i + 1, 50) )
	{
		i += 1;
	}

	i = go_down(i, x, y, dx, dy,
			  xlimlow, xlimhigh, ylimlow,
			  ylimhigh, handleTwitch);

	zoomInd4 = i;

	i = go_up(i, x, y, dx, dy,
			  xlimlow, xlimhigh, ylimlow,
			  ylimhigh, handleTwitch);

	zoomInd3 = i;

	i = go_down(i, x, y, dx, dy,
			  xlimlow, xlimhigh, ylimlow,
			  ylimhigh, handleTwitch);

	zoomInd2 = i;

	i = go_up(i, x, y, dx, dy,
		  	    xlimlow, xlimhigh, ylimlow,
			    ylimhigh, handleTwitch);

	zoomInd1 = i;

	if( (zoomInd1 < 0) || (zoomInd1 >= NUMBER_OF_POINTS_IN_ARC) ) return;
	if( (zoomInd2 < 0) || (zoomInd2 >= NUMBER_OF_POINTS_IN_ARC) ) return;
	if( (zoomInd3 < 0) || (zoomInd3 >= NUMBER_OF_POINTS_IN_ARC) ) return;
	if( (zoomInd4 < 0) || (zoomInd4 >= NUMBER_OF_POINTS_IN_ARC) ) return;

	if( !(zoomInd1 > zoomInd2 && zoomInd2 > zoomInd3 && zoomInd3 > zoomInd4) )
	{
		/* Strange... Do not try to zoom this curve */
		return;
	}

	if( y[zoomInd1] <= y[zoomInd2] || y[zoomInd1] <= y[zoomInd4] )
	{
		/* Strange... Do not try to zoom this curve */
		return;
	}

	if( y[zoomInd3] <= y[zoomInd2] || y[zoomInd3] <= y[zoomInd4] )
	{
		/* Strange... Do not try to zoom this curve */
		return;
	}

	if( y[zoomInd2] - y[zoomInd4] > y[zoomInd1] - y[zoomInd3] ) /* Chosen rule */
	{
		/*4 */
		pZoomOut->bestCandidate = pZoomIn->likelyCandidateIfTestPasses;
	}
	else
	{
		/*h */
		pZoomOut->bestCandidate = !pZoomIn->likelyCandidateIfTestPasses;
	}
}

static void testBetter_n_than_h(ZOOM_IN * pZoomIn, ZOOM_OUT * pZoomOut)
{
	DECUMA_INT16 xlimlow, xlimhigh, ylimlow, ylimhigh;
	int i, zoomInd1, zoomInd2, zoomInd3, zoomInd4;
	DECUMA_INT32 dist1, dist2;
	DECUMA_BOOL handleTwitch;

	DECUMA_INT16 x[NUMBER_OF_POINTS_IN_ARC];
	DECUMA_INT16 y[NUMBER_OF_POINTS_IN_ARC];
	DECUMA_INT16 dx[NUMBER_OF_POINTS_IN_ARC];
	DECUMA_INT16 dy[NUMBER_OF_POINTS_IN_ARC];

	SCR_ARC * pArc = pZoomIn->pCurve->Arcs[0];

	pZoomOut->zoomQuality = 0; /*Not used; */
	pZoomOut->bestCandidate = 0;

	if( !pArc )
	{
		decumaAssert(0); /* NULL-pointers supported but might be used unintended. */
						 /* Avoid this in debug mode for now. */
		return;
	}

	if( pZoomIn->pCurve->noArcs > 1 )
	{
		/* This case is not handled by this function */
		return;
	}

	flip_arc(pArc, x, y, dx, dy,
		NUMBER_OF_POINTS_IN_ARC); /* Go backwards */

	i = 0;

	xlimlow  = MIN_DECUMA_INT16;
	xlimhigh = MAX_DECUMA_INT16;
	ylimlow  = MIN_DECUMA_INT16;
	ylimhigh = MAX_DECUMA_INT16;

	handleTwitch = TRUE; /* Handle twitches */

	/* Remove peak in the end to avoid algorithm crash */
	while( peak(dx, dy, i + 1, 50) )
	{
		i += 1;
	}

	i = go_down(i, x, y, dx, dy,
			  xlimlow, xlimhigh, ylimlow,
			  ylimhigh, handleTwitch);

	zoomInd4 = i;

	i = go_up(i, x, y, dx, dy,
			  xlimlow, xlimhigh, ylimlow,
			  ylimhigh, handleTwitch);

	zoomInd3 = i;

	i = go_down(i, x, y, dx, dy,
			  xlimlow, xlimhigh, ylimlow,
			  ylimhigh, handleTwitch);
	zoomInd2 = i;


	i = go_up(i, x, y, dx, dy,
		  	    xlimlow, xlimhigh, ylimlow,
			    ylimhigh, handleTwitch);
	zoomInd1 = i;

	if( (zoomInd1 < 0) || (zoomInd1 >= NUMBER_OF_POINTS_IN_ARC) ) return;
	if( (zoomInd2 < 0) || (zoomInd2 >= NUMBER_OF_POINTS_IN_ARC) ) return;
	if( (zoomInd3 < 0) || (zoomInd3 >= NUMBER_OF_POINTS_IN_ARC) ) return;
	if( (zoomInd4 < 0) || (zoomInd4 >= NUMBER_OF_POINTS_IN_ARC) ) return;

	if( !(zoomInd1 > zoomInd2 && zoomInd2 > zoomInd3 && zoomInd3 > zoomInd4) )
	{
		/* Strange... Do not try to zoom this curve */
		return;
	}

	if( y[zoomInd1] <= y[zoomInd2] || y[zoomInd1] <= y[zoomInd4] )
	{
		/* Strange... Do not try to zoom this curve */
		return;
	}

	if( y[zoomInd3] <= y[zoomInd2] || y[zoomInd3] <= y[zoomInd4] )
	{
		/* Strange... Do not try to zoom this curve */
		return;
	}

	if( x[zoomInd4] <= x[zoomInd2] )
	{
		/* Strange... Do not try to zoom this curve */
		return;
	}

	dist1 = y[zoomInd3] - y[zoomInd2];
	dist2 = y[zoomInd1] - y[zoomInd2];


	if( dist2 == 0 )
	{
		return;
	}
	if( 100 * dist1 / dist2 > 68 ) /* Optimized for balance */
	{
		/*n */
		pZoomOut->bestCandidate = pZoomIn->likelyCandidateIfTestPasses;
	}
	else
	{
		/*h */
		pZoomOut->bestCandidate = !pZoomIn->likelyCandidateIfTestPasses;
	}
}

static void testBetter_a_than_u(ZOOM_IN * pZoomIn, ZOOM_OUT * pZoomOut)
{
	/*
	Approximately:

	Removes short peak in the end of the curve
	Traverses the curve backwards until it finds a minimum (removes continuation)
	Then traverses until a maximum (P1)
	Then traverses until a new minimum (BOTTOM)
	Then traverses until a "rightimum" (P2)
	Finds the biggest horizontal distance (DIST) between P1 and P2

            P2
           **
          **    P1
         **     *
        **      **
        *       **
       **-DIST--**
      * *       * *  *
         *     *   **
          *****    *
		  BOTTOM

	If P1-P2 is small compared to DIST it is considered an "a"
	*/

	DECUMA_INT16 xlimlow, xlimhigh, ylimlow, ylimhigh;
	int i, zoomInd1, zoomInd2, bottomInd;
	DECUMA_INT32 dist1, dist2;
	DECUMA_BOOL handleTwitch;

	DECUMA_INT16 x[NUMBER_OF_POINTS_IN_ARC];
	DECUMA_INT16 y[NUMBER_OF_POINTS_IN_ARC];
	DECUMA_INT16 dx[NUMBER_OF_POINTS_IN_ARC];
	DECUMA_INT16 dy[NUMBER_OF_POINTS_IN_ARC];

	SCR_ARC * pArc = pZoomIn->pCurve->Arcs[0];

	pZoomOut->zoomQuality = 0; /*Not used; */
	pZoomOut->bestCandidate = 0;

	if( !pArc )
	{
		decumaAssert(0); /* NULL-pointers supported but might be used unintended. */
						 /* Avoid this in debug mode for now. */
		return;
	}

	flip_arc(pArc, x, y, dx, dy,
		NUMBER_OF_POINTS_IN_ARC); /* Go backwards */

	i = 0;

	xlimlow  = MIN_DECUMA_INT16;
	xlimhigh = MAX_DECUMA_INT16;
	ylimlow  = MIN_DECUMA_INT16;
	ylimhigh = MAX_DECUMA_INT16;

	handleTwitch = TRUE; /* Handle twitches */

	/* Remove peak in the end to avoid algorithm crash */
	while( peak(dx, dy, i + 1, 50) )
	{
		i += 1;
	}

	i = go_down(i, x, y, dx, dy,
			  xlimlow, xlimhigh, ylimlow,
			  ylimhigh, handleTwitch);
	i = go_up(i, x, y, dx, dy,
			  xlimlow, xlimhigh, ylimlow,
			  ylimhigh, handleTwitch);

	if( (i < 0) || (i >= NUMBER_OF_POINTS_IN_ARC - 1) )
	{
		return;
	}

	/* Clean up peaks */
	while( peak(dx, dy, i + 1, 50) )
	{
		i += 1;
	}

	if( (i < 0) || (i >= NUMBER_OF_POINTS_IN_ARC) )
	{
		return;
	}

	zoomInd1 = i;

	i = go_down(i, x, y, dx, dy,
			  xlimlow, xlimhigh, ylimlow,
			  ylimhigh, handleTwitch);

	bottomInd = i;

	ylimhigh = (DECUMA_INT16)(7 * ((DECUMA_INT32)y[zoomInd1]-y[i]) / 10 + y[i]); /* Reach top part before setting xlimhigh */

	i = go_up(i, x, y, dx, dy,
		  	    xlimlow, xlimhigh, ylimlow,
			    ylimhigh, handleTwitch);

	ylimhigh = MAX_DECUMA_INT16;	/* Reset */
	xlimhigh = x[zoomInd1]; /* Do not pass first zoom point horisontally */

	i = go_up(i, x, y, dx, dy,
			    xlimlow, xlimhigh, ylimlow,
			    ylimhigh, handleTwitch);

	if( !peak(dx, dy, i, 80) ) /* u-ligature downward right */
	{
		i = go_right(i, x, y, dx, dy,
				    xlimlow, xlimhigh, ylimlow,
					ylimhigh, handleTwitch);
	}

	/* u-ligature from the upper left may have been missed */
	/* Check if angle is more than 135 degrees and less    */
	/* than 180 degrees.                                   */
	while( (DECUMA_INT32)dx[i] < 0 && (DECUMA_INT32)dy[i] > 0 && /* 2nd quadrant */
		   10000 * decumaAbs( (DECUMA_INT32)dy[i] ) / ( 100 * decumaAbs( (DECUMA_INT32)dx[i] ) + EPS ) < 100 ) /* dx is INT16, i.e. no overflow risk */
	{
		i--;

		if( i < 0 )
		{
			return;
		}
	}

	zoomInd2 = i;

	if( y[zoomInd1] <= y[bottomInd] || y[zoomInd2] <= y[bottomInd] )
	{
		/*decumaAssert(0); // Something (arc or algorithm?) is not right (au consistent) */
		return;
	}

	if( zoomInd1 >= bottomInd || bottomInd >= zoomInd2 )
	{
		/*decumaAssert(0); // Something is wrong with the algorithm? */
		return;
	}

	if( 2 * zoomInd2 < NUMBER_OF_POINTS_IN_ARC ) /* Second zoom point in first half?! */
	{
		return;
	}

	/* More verification tests here could reduce the number of induced errors! */

	dist1 = decumaAbs( x[zoomInd2] - x[zoomInd1] );

	dist2 = findWidestHorisontal(x, y, zoomInd1, zoomInd2, bottomInd);

	if( dist2 == 0 )
	{
		return;
	}
	if( 100 * dist1 / dist2 <= 55 ) /* dist1 and dist2 are INT32 types with INT16 values, i.e. no overflow risk */
	{
		/*a */
		pZoomOut->bestCandidate = pZoomIn->likelyCandidateIfTestPasses;
	}
	else
	{
		/*u */
		pZoomOut->bestCandidate = !pZoomIn->likelyCandidateIfTestPasses;
	}
}

static void testBetter_g_than_y(ZOOM_IN * pZoomIn, ZOOM_OUT * pZoomOut)
{
	/*
	Approximately:

	Removes short peak in the end of the curve
	Traverses the curve backwards until it finds the minimum (removes even advanced continuation)
	Then traverses until a maximum (P1)
	Then traverses until a new minimum (BOTTOM)
	Then traverses until a "rightimum" (P2)
	Finds the biggest horizontal distance (DIST) between P1 and P2

            P2
           **
          **    P1
         **     *
        **      **
        *       **
       **-DIST--**
      * *       **
         *     * *
          *****  *
		  BOTTOM *
                 *
				 *
				 *
		  *		 *
		 *		 *
         *       *
          **   *
            ***

	If P1-P2 is small compared to DIST it is considered a "g"
	*/

	DECUMA_INT16 xlimlow, xlimhigh, ylimlow, ylimhigh;
	int i, zoomInd1, zoomInd2, bottomInd, min_y_index, max_y_index;
	DECUMA_INT32 dist1, dist2;
	DECUMA_BOOL handleTwitch;

	DECUMA_INT16 x[NUMBER_OF_POINTS_IN_ARC];
	DECUMA_INT16 y[NUMBER_OF_POINTS_IN_ARC];
	DECUMA_INT16 dx[NUMBER_OF_POINTS_IN_ARC];
	DECUMA_INT16 dy[NUMBER_OF_POINTS_IN_ARC];

	SCR_ARC * pArc = pZoomIn->pCurve->Arcs[0];

	pZoomOut->zoomQuality = 0; /*Not used; */
	pZoomOut->bestCandidate = 0;

	if( !pArc )
	{
		decumaAssert(0); /* NULL-pointers supported but might be used unintended. */
						 /* Avoid this in debug mode for now. */
		return;
	}

	flip_arc(pArc, x, y, dx, dy, NUMBER_OF_POINTS_IN_ARC); /* Go backwards */

	i = 0;

	min_y_index = indexToMin(y, NUMBER_OF_POINTS_IN_ARC);
	max_y_index = indexToMax(y, NUMBER_OF_POINTS_IN_ARC);

	decumaAssert(y[max_y_index] >= y[min_y_index]); /* Just check */

	xlimlow  = MIN_DECUMA_INT16;
	xlimhigh = MAX_DECUMA_INT16;
	ylimlow  = MIN_DECUMA_INT16;
	ylimhigh = MAX_DECUMA_INT16;

	handleTwitch = TRUE; /* Handle twitches at first */

	/* Remove peak in the end to avoid algorithm crash */
	while( i<NUMBER_OF_POINTS_IN_ARC-2 && (peak(dx, dy, i + 1, 50) || peak(dx, dy, i + 2, 50)) )
	{
		i += 1;
	}

	i = go_down(i, x, y, dx, dy,
			  xlimlow, xlimhigh, ylimlow,
			  ylimhigh, handleTwitch);
	i = go_left(i, x, y, dx, dy,
			  xlimlow, xlimhigh, ylimlow,
			  ylimhigh, handleTwitch);
	i = go_down(i, x, y, dx, dy,
			  xlimlow, xlimhigh, ylimlow,
			  ylimhigh, handleTwitch);

	ylimhigh = (DECUMA_INT16)(3 * ((DECUMA_INT32)y[max_y_index]-y[min_y_index]) / 10 + y[min_y_index]); /* Go up a bit before handling twitches */

	i = go_up(i, x, y, dx, dy,
			  xlimlow, xlimhigh, ylimlow,
			  ylimhigh, handleTwitch);

    while( i<NUMBER_OF_POINTS_IN_ARC && (10000 * (DECUMA_INT32)(y[i] - y[min_y_index]) / ( 1000 * (DECUMA_INT32)decumaAbs(y[max_y_index] - y[min_y_index]) + EPS ) < 4) ) /* Still low? No overflow probl. */
    {
		i++;
    }

	ylimhigh = MAX_DECUMA_INT16; /* Reset */

	handleTwitch = FALSE; /* Turn off twitch handling. The local max for g might be a twitch. */

	i = go_up(i, x, y, dx, dy,
			  xlimlow, xlimhigh, ylimlow,
			  ylimhigh, handleTwitch);

	handleTwitch = TRUE; /* Handle twitches again. */

	zoomInd1 = i;

	i = go_down(i, x, y, dx, dy,
			  xlimlow, xlimhigh, ylimlow,
			  ylimhigh, handleTwitch);

	bottomInd = i;

	ylimhigh = (DECUMA_INT16)(7 * ((DECUMA_INT32)y[zoomInd1]-y[i]) / 10 + y[i]); /* Reach top part before setting xlimhigh */

	i = go_up(i, x, y, dx, dy,
		  	    xlimlow, xlimhigh, ylimlow,
			    ylimhigh, handleTwitch);

	ylimhigh = MAX_INT16;	/* Reset */
	xlimhigh = x[zoomInd1]; /* Do not pass first zoom point horisontally */

	i = go_up(i, x, y, dx, dy,
			    xlimlow, xlimhigh, ylimlow,
			    ylimhigh, handleTwitch);

	if( !peak(dx, dy, i, 80) )
	{
		i = go_right(i, x, y, dx, dy,
				    xlimlow, xlimhigh, ylimlow,
					ylimhigh, handleTwitch);
	}

	/* u-ligature from the upper left may have been missed */
	/* Check if angle is more than 135 degrees and less    */
	/* than 180 degrees.                                   */
	while( (DECUMA_INT32)dx[i] < 0 && (DECUMA_INT32)dy[i] > 0 && /* 2nd quadrant */
		   10000 * decumaAbs( (DECUMA_INT32)dy[i] ) / ( 100 * decumaAbs( (DECUMA_INT32)dx[i] ) + EPS ) < 100 ) /* dx is INT16, i.e. no overflow risk */
	{
		i--;

		if( i < 0 )
		{
			return;
		}
	}

	zoomInd2 = i;

	if( y[zoomInd1] <= y[bottomInd] || y[zoomInd2] <= y[bottomInd] )
	{
		return; /* Certain s-like g's might cause this situation */
	}

	if( zoomInd1 > bottomInd || bottomInd >= zoomInd2 )
	{
		return; /* Certain s-like g's might cause this situation */
	}

	if( 2 * zoomInd2 < NUMBER_OF_POINTS_IN_ARC ) /* Second zoom point in first half?! */
	{
		return;
	}

	/* More verification tests here could reduce the number of induced errors! */

	dist1 = decumaAbs( x[zoomInd2] - x[zoomInd1] );

	dist2 = findWidestHorisontal(x, y, zoomInd1, zoomInd2, bottomInd);

	if( dist2 == 0 )
	{
		return;
	}
	if( 100 * dist1 / dist2 <= 60 ) /* dist1 and dist2 are INT32 types with INT16 values, i.e. no overflow risk */
	{
		/*g */
		pZoomOut->bestCandidate = pZoomIn->likelyCandidateIfTestPasses;
	}
	else
	{
		/*y */
		pZoomOut->bestCandidate = !pZoomIn->likelyCandidateIfTestPasses;
	}
}

static void testBetter_r_than_v(ZOOM_IN * pZoomIn, ZOOM_OUT * pZoomOut)
/* Returns					*/
/*			0 for error		*/
/*			1 for r			*/
/*			2 for v			*/
{
	/*
	Approximately:

	Removes short peak in the beginning of the curve
	Traverses the curve until it finds a maximum: startindex (SI) (removes beginnings)
	Then traverses until the minimum: bottomindex (BI)
	Then takes the last point endindex (EI)

       SI
       *           ***** EI
      **         **
     * *       **
    *   *     *
        *    *
        *   *
        *  *
        *  *
        * *
         **
         *
        BI

    Then we find a v-area and an r-area and compare these to each other.
	- The v-area is the area inside the triangle SI-EI-BI and above the curve.
	- The r-area is the area that is tothe right of the curve and to the left of a
	  straight line between BI-EI.
	  Plus 0.5 * The area left of the CURVE between SI-BI and right of a straight LINE
	  between SI-BI.
	  (This has shown to be good in tests, maybe a different constant could be used than 0.5)

    If the r-area is big compared to the v-area it is considered an 'r'.
	*/

	DECUMA_INT16 xlimlow, xlimhigh, ylimlow, ylimhigh;
	DECUMA_BOOL handleTwitch, error;
	double wholeArea, leftArea, rightArea, r_area, v_area;
	int i, si, bi, ei;

	DECUMA_INT16 x[NUMBER_OF_POINTS_IN_ARC];
	DECUMA_INT16 y[NUMBER_OF_POINTS_IN_ARC];
	DECUMA_INT16 dx[NUMBER_OF_POINTS_IN_ARC];
	DECUMA_INT16 dy[NUMBER_OF_POINTS_IN_ARC];

	SCR_ARC * pArc = pZoomIn->pCurve->Arcs[0];

	pZoomOut->zoomQuality = 0; /*Not used; */
	pZoomOut->bestCandidate = 0;

	if( !pArc )
	{
		decumaAssert(0); /* NULL-pointers supported but might be used unintended. */
						 /* Avoid this in debug mode for now. */
		return;
	}

	copy_arc(pArc, x, y, dx, dy, NUMBER_OF_POINTS_IN_ARC);

	i = 0;

	xlimlow  = MIN_DECUMA_INT16;
	xlimhigh = MAX_DECUMA_INT16;
	ylimlow  = MIN_DECUMA_INT16;
	ylimhigh = MAX_DECUMA_INT16;

	handleTwitch = TRUE; /* Handle twitches */

	/* Remove peak in the beginning to avoid algorithm crash */
	while( peak(dx, dy, i + 1, 50) )
	{
		i += 1;
	}

	i = go_up(i, x, y, dx, dy,
			  xlimlow, xlimhigh, ylimlow,
			  ylimhigh, handleTwitch);

	si = i;

	bi = i + lastIndexToMin(&y[i], NUMBER_OF_POINTS_IN_ARC - i); /* Get the rightmost min */

	ei = NUMBER_OF_POINTS_IN_ARC - 1; /* Go to last point (ignore v-ligature possibility) */

	if( y[bi] > y[si] || y[bi] > y[ei] )
	{
		/*decumaAssert(0); // Something (arc or algorithm?) is not right (rv consistent) */
		return;
	}
	if( x[si] >= x[ei] )
	{
		/*decumaAssert(0); // Something (arc or algorithm?) is not right (rv consistent) */
		return;
	}
	if( !(bi > si && ei > bi) )
	{
		decumaAssert(0); /* Something is wrong with the algorithm? */
		return;
	}

	/* More verification tests here could reduce the number of induced errors! */

	wholeArea = triangleArea(x,y,si,bi,ei,&error);
	leftArea = integrateArc(x,y,si,bi,TRUE,&error);
	rightArea = integrateArc(x,y,bi,ei,TRUE,&error);

	if( error )
	{
		return;
	}

	r_area = rightArea;
	v_area = wholeArea - rightArea;

	if( leftArea > 0 )
	{
		r_area += 0.5 * leftArea;
		v_area -= leftArea;
	}

	if( v_area <= 0 )
	{
		/*r */
		pZoomOut->bestCandidate = pZoomIn->likelyCandidateIfTestPasses;
	}
	else if( (r_area / v_area > 0.38) && (rightArea > 0) )
	{
		/* r */
		pZoomOut->bestCandidate = pZoomIn->likelyCandidateIfTestPasses;
	}
	else
	{
		/* v */
		pZoomOut->bestCandidate = !pZoomIn->likelyCandidateIfTestPasses;
	}
}

static void testBetter_u_than_v(ZOOM_IN * pZoomIn, ZOOM_OUT * pZoomOut)
/* Returns					*/
/*			0 for error		*/
/*			1 for u			*/
/*			2 for v			*/
{
	/*
	Approximately:

	Removes short peak in the beginning of the curve
	Traverses the curve until it finds a maximum: startindex (SI) (removes beginnings)
	Then traverses until the minimum: bottomindex (BI)
	Then traverses until a new maximum: endindex (EI) (removes continuations)

       SI
       *            EI
      **            **
     * *           *  *
    *   *          *   *
        *          *
        *          *
        *         *
        *        *
         *      *
          **  **
            **
            BI

    Then we find a v-area and a u-area and compare these to each other.
	- The v-area is the area inside the triangle SI-EI-BI and above the curve.
	- The u-area is the whole area above and "inside" the curve.

    If the u-area is much bigger than the v-area it is considered a 'u'.
	*/

	DECUMA_INT16 xlimlow, xlimhigh, ylimlow, ylimhigh;
	DECUMA_BOOL handleTwitch, error;
	double leftArea, rightArea, vArea, uArea;
	int i, si, bi, ei;
	DECUMA_INT32 height;

	DECUMA_INT16 x[NUMBER_OF_POINTS_IN_ARC];
	DECUMA_INT16 y[NUMBER_OF_POINTS_IN_ARC];
	DECUMA_INT16 dx[NUMBER_OF_POINTS_IN_ARC];
	DECUMA_INT16 dy[NUMBER_OF_POINTS_IN_ARC];

	SCR_ARC * pArc = pZoomIn->pCurve->Arcs[0];

	pZoomOut->zoomQuality = 0; /*Not used; */
	pZoomOut->bestCandidate = 0;

	if( !pArc )
	{
		decumaAssert(0); /* NULL-pointers supported but might be used unintended. */
						 /* Avoid this in debug mode for now. */
		return;
	}

	copy_arc(pArc, x, y, dx, dy, NUMBER_OF_POINTS_IN_ARC);

	i = 0;

	xlimlow  = MIN_DECUMA_INT16;
	xlimhigh = MAX_DECUMA_INT16;
	ylimlow  = MIN_DECUMA_INT16;
	ylimhigh = MAX_DECUMA_INT16;

	handleTwitch = TRUE; /* Handle twitches */

	/*Remove peak in the beginning to avoid algorithm crash */
	while( peak(dx, dy, i + 1, 50) )
	{
		i += 1;
	}

	i = go_up(i, x, y, dx, dy,
			  xlimlow, xlimhigh, ylimlow,
			  ylimhigh, handleTwitch);

	si = i;

	i += lastIndexToMin(&y[i], NUMBER_OF_POINTS_IN_ARC - i); /* Move to the rightmost min */
	bi = i;

	height = maxNUMBER(y, NUMBER_OF_POINTS_IN_ARC) - y[bi];

	if( !height )
	{
		/*decumaAssert(0); */
		return;
	}

	while( (double)(y[i] - y[bi]) / (double)height < 0.5 ) /* First go half way up to avoid bottom noise */
	{
		if( i == NUMBER_OF_POINTS_IN_ARC - 1 )
		{
			break;
		}
		i++;
	}

	i = go_up(i, x, y, dx, dy,
			  xlimlow, xlimhigh, ylimlow,
			  ylimhigh, handleTwitch);  /* Pot. rest is ligature */

	ei = i;

	if( y[bi] > y[si] || y[bi] > y[ei] )
	{
		/*decumaAssert(0); // Something (arc or algorithm?) is not right (uv consistent) */
		return;
	}
	if( x[si] >= x[ei] )
	{
		/*decumaAssert(0); // Something (arc or algorithm?) is not right (uv consistent) */
		return;
	}
	if( !(bi > si && ei > bi) )
	{
/*		decumaAssert(0); // Something is wrong with the algorithm? */
		return;
	}

	/* More verification tests here could reduce the number of induced errors! */

	vArea = triangleArea(x,y,si,bi,ei,&error);
	leftArea = integrateArc(x,y,si,bi,FALSE,&error);
	rightArea = integrateArc(x,y,bi,ei,FALSE,&error);
	uArea = vArea + leftArea + rightArea;

	if( error || !vArea )
	{
		return;
	}

	if( (uArea / vArea > 1.5) && (leftArea > 0) && (rightArea > 0) )
	{
		/* u */
		pZoomOut->bestCandidate = pZoomIn->likelyCandidateIfTestPasses;
	}
	else
	{
		/* v */
		pZoomOut->bestCandidate = !pZoomIn->likelyCandidateIfTestPasses;
	}
}


static void testBetter_f_than_t(ZOOM_IN * pZoomIn, ZOOM_OUT * pZoomOut)
{
  /*
    Method:

    Assume there are only two arcs.
    Find the middle point of the vertical arc.
    Draw a line from that point to each end and integrate the areas enclosed.
    If the top area is large enough in comparison with the bottom, it is an f.
    If the bottom bend is towards the left, it is more likely to be an f.


            * *                  *
	       *    *               *
	      *                     *
	      *                     *
	   *******               *******
	      *                     *
	      *                     *
	      *                     *
	      *                      *    *
	     *                         **
 */

  /*#define OLD_FUNC */

#ifdef OLD_FUNC
  zoomPartOfCurve(pZoomIn, pZoomOut);
  return;
#else

  DECUMA_BOOL error;
  double topArea, bottomArea, height;
  double topArcLength, bottomArcLength;
  int i, upperMaxXIndex, lowerMinXIndex, middleIndex, maxYIndex, minYIndex;

  DECUMA_INT16 cross_mean;
  DECUMA_INT16 old_x, old_y;

  DECUMA_INT16 x[NUMBER_OF_POINTS_IN_ARC];
  DECUMA_INT16 y[NUMBER_OF_POINTS_IN_ARC];
  DECUMA_INT16 dx[NUMBER_OF_POINTS_IN_ARC];
  DECUMA_INT16 dy[NUMBER_OF_POINTS_IN_ARC];

  SCR_ARC * pArc;

  topArea = 0;
  bottomArea = 0;
  pZoomOut->zoomQuality = 0; /*Not used; */
  pZoomOut->bestCandidate = 0;

  /*Assume the letter consists of only two arcs */
  /*Get the first arc */
  pArc = pZoomIn->pCurve->Arcs[0];

  if (!pArc)
  {
    decumaAssert(0); /* NULL-pointers supported but might be used unintended. */
    /* Avoid this in debug mode for now. */
    return;
  }

  copy_arc(pArc, x, y, dx, dy, NUMBER_OF_POINTS_IN_ARC);
  minYIndex = indexToMin(y, NUMBER_OF_POINTS_IN_ARC);
  maxYIndex = indexToMax(y, NUMBER_OF_POINTS_IN_ARC);
  height = decumaAbs(y[maxYIndex] - y[minYIndex]);

  /* If there is a second arc */
  if( pZoomIn->pCurve->noArcs > 1)
  {
    pArc = pZoomIn->pCurve->Arcs[1];
    copy_arc(pArc, x, y, dx, dy, NUMBER_OF_POINTS_IN_ARC);
    minYIndex = indexToMin(y, NUMBER_OF_POINTS_IN_ARC);
    maxYIndex = indexToMax(y, NUMBER_OF_POINTS_IN_ARC);
    /*Compare sizes */
    if (decumaAbs((DECUMA_INT32)y[maxYIndex] - y[minYIndex]) > height)
    {
      /*Current arc is the one. Calculate height */
      height = decumaAbs((DECUMA_INT32)y[maxYIndex] - y[minYIndex]);
    }
    else
    {
      /*Get the first arc, it was the vertical one */
      /*Height already calculated */
	  pArc = pZoomIn->pCurve->Arcs[0];
      copy_arc(pArc, x, y, dx, dy, NUMBER_OF_POINTS_IN_ARC);
	  /* Re-calculate index */
	  minYIndex = indexToMin(y, NUMBER_OF_POINTS_IN_ARC);
      maxYIndex = indexToMax(y, NUMBER_OF_POINTS_IN_ARC);
    }

  }

  cross_mean = (DECUMA_INT16)((double)y[maxYIndex] + (double)y[minYIndex])/2;

  /* Guess in which direction the arc is drawn */
  if (y[0] < y[NUMBER_OF_POINTS_IN_ARC-1])
  {
    flip_arc(pArc, x, y, dx, dy, NUMBER_OF_POINTS_IN_ARC);
	/* Re-calculate index */
	minYIndex = indexToMin(y, NUMBER_OF_POINTS_IN_ARC);
    maxYIndex = indexToMax(y, NUMBER_OF_POINTS_IN_ARC);
  }

  if (maxYIndex > minYIndex)
  {
	/* Unexpected. Propably two characters tested as one (assert). Abort. */
	decumaAssert(pZoomIn->pCurve->noArcs > 1);
	return;
  }

  /* Now we know where the upper half is. Find the upper part of the */
  /* arc with largest horisontal distance to the middle. Also, find */
  /* the lower part of the arc furthest to the left. */
  /* Calculate areas. */
  i = maxYIndex;
  while ( (decumaAbs((double)(y[i] - y[minYIndex])) > height*0.5) && (i < NUMBER_OF_POINTS_IN_ARC) )
  {
    i++;
  }
  middleIndex = i;
  upperMaxXIndex = indexToMax(x, NUMBER_OF_POINTS_IN_ARC - middleIndex);
  lowerMinXIndex = indexToMin(&x[middleIndex], NUMBER_OF_POINTS_IN_ARC - middleIndex) + middleIndex;

  old_x = x[maxYIndex];
  old_y = y[maxYIndex];
  topArcLength = 0;
  for (i = maxYIndex; i > -1; i--)
  {
    /* OBS: Risk of overflow? */
    topArcLength += decumaSqrt(((DECUMA_INT32)old_x - x[i])*((DECUMA_INT32)old_x - x[i]) +
		((DECUMA_INT32)old_y - y[i])*((DECUMA_INT32)old_y - y[i]));
	old_x = x[i];
	old_y = y[i];
  }

  old_x = x[minYIndex];
  old_y = y[minYIndex];
  bottomArcLength = 0;
  for (i = minYIndex; i < NUMBER_OF_POINTS_IN_ARC; i++)
  {
    /* OBS: Risk of overflow? */
    bottomArcLength += decumaSqrt(((DECUMA_INT32)old_x - x[i])*((DECUMA_INT32)old_x - x[i]) +
		((DECUMA_INT32)old_y - y[i])*((DECUMA_INT32)old_y - y[i]));
	old_x = x[i];
	old_y = y[i];
  }

  topArea = integrateArc(x, y, upperMaxXIndex, middleIndex, FALSE, &error);
  bottomArea = integrateArc(x, y, middleIndex, NUMBER_OF_POINTS_IN_ARC-1, FALSE, &error);

  #ifdef DEBUG_OUTPUT
  	fprintf(g_fpDebug,"upperMaxXIndex: %d,middleIndex: %d\n",
  		upperMaxXIndex,middleIndex);
  	fprintf(g_fpDebug,"topArea: %f,bottomArea: %f, error:%d \n",
  		topArea,bottomArea,error);
  	fprintf(g_fpDebug,"topArcLength: %f,bottomArcLength:%f,height: %f \n",
  		topArcLength,bottomArcLength,height);
  	fflush(g_fpDebug);
  #endif


  if (error)
  {
    return;
  }

  #ifdef DEBUG_OUTPUT
  	fprintf(g_fpDebug,"(decumaAbs(x[upperMaxXIndex] - x[middleIndex]):%d\n",
  		decumaAbs(x[upperMaxXIndex] - x[middleIndex]));
  	fprintf(g_fpDebug,"(x[middleIndex] - x[lowerMinXIndex]):%d \n",
  		x[middleIndex] - x[lowerMinXIndex]);
  	fflush(g_fpDebug);
  #endif

  /* If there is risk of severe instability */
  if (decumaAbs(bottomArea) < 1)
  {
    /* If there is a bend, check that it is long enough to be an f. */
    if ( topArcLength > 0.2*height )
    {
      pZoomOut->bestCandidate = pZoomIn->likelyCandidateIfTestPasses;
    }
    /* If the top max x is far enough to the right it is an f */
    else if ( decumaAbs((DECUMA_INT32)x[upperMaxXIndex] - x[middleIndex]) > 0.15*height )
    {
      pZoomOut->bestCandidate = pZoomIn->likelyCandidateIfTestPasses;
    }
    /* If there is a bend to the right in the top and to the left at */
    /* the bottom it may be an f */
    else if ( (decumaAbs((DECUMA_INT32)x[upperMaxXIndex] - x[middleIndex]) > 0.1*height) &&
	      ( ((DECUMA_INT32)x[middleIndex] - x[lowerMinXIndex]) < -0.05*height))

    {
      pZoomOut->bestCandidate = pZoomIn->likelyCandidateIfTestPasses;
    }
    else
    {
      pZoomOut->bestCandidate = !pZoomIn->likelyCandidateIfTestPasses;
    }
  }
  else
  {
		if (decumaAbs(topArea)+decumaAbs(bottomArea)<(height*height/50))
		{
			/*Both areas are small. This is a 't' */
			pZoomOut->bestCandidate = !pZoomIn->likelyCandidateIfTestPasses;
		}
		else if (topArea>bottomArea) {   /*Consider sign */
			/*f */
			pZoomOut->bestCandidate = pZoomIn->likelyCandidateIfTestPasses;
		} else {
			/*t */
			pZoomOut->bestCandidate = !pZoomIn->likelyCandidateIfTestPasses;
		}
  }
 #endif
}

static void testBetter_yeh_than_kaf(ZOOM_IN * pZoomIn, ZOOM_OUT * pZoomOut)
{
	/*
	Approximately:

	Removes short peak in the beginning of the curve
	Traverses the curve until it finds a startindex (ai) (removes beginnings)
	Then traverses until it finds bi, ci, di and finally ei.


				  ****
                 *    *
                *      *
             bi*       ai
   ei          *
	*           *
	*            **
    **             *
     **             *ci
      **           *
      di***    ****
          ******

	Statistics have shown that for the arc to be a YEH type (e.g. Unicode 0x626, 0x649 or 0x64A),
	x[ai] > x[bi] and x[ci] > x[bi] and x[ci] > x[ei] and y[bi] > y[ci] but also that
	5% of (x[ci] - x[di]) < (x[ai] - x[bi]).

	In any other case the result is most likely a KAF type.
	*/

	int i, ai, bi, ci, di, ei;

	DECUMA_BOOL handleTwitch;

	DECUMA_INT16 x[NUMBER_OF_POINTS_IN_ARC];
	DECUMA_INT16 y[NUMBER_OF_POINTS_IN_ARC];
	DECUMA_INT16 dx[NUMBER_OF_POINTS_IN_ARC];
	DECUMA_INT16 dy[NUMBER_OF_POINTS_IN_ARC];

	SCR_ARC * pArc = pZoomIn->pCurve->Arcs[0];

	if( !pArc )
	{
		decumaAssert(0); /* NULL-pointers supported but might be used unintended. */
						 /* Avoid this in debug mode for now. */
		return;
	}

	copy_arc(pArc, x, y, dx, dy, NUMBER_OF_POINTS_IN_ARC);

	handleTwitch = TRUE; /* Handle twitches */

	/* Remove peak in the beginning to avoid algorithm crash */
	i = 0;
	while( peak(dx, dy, i + 1, 25) ) i++;
	ai = i;

	i = go_left(i, x, y, dx, dy, MIN_DECUMA_INT16, MAX_DECUMA_INT16, MIN_DECUMA_INT16, MAX_DECUMA_INT16, handleTwitch); bi = i;
	i = go_right(i, x, y, dx, dy, MIN_DECUMA_INT16, MAX_DECUMA_INT16, MIN_DECUMA_INT16, MAX_DECUMA_INT16, handleTwitch); ci = i;
	i = go_left(i, x, y, dx, dy, MIN_DECUMA_INT16, MAX_DECUMA_INT16, MIN_DECUMA_INT16, MAX_DECUMA_INT16, handleTwitch); di = i;

	ei = NUMBER_OF_POINTS_IN_ARC - 1;

	pZoomOut->zoomQuality = 0; /*Not used; */
	pZoomOut->bestCandidate = 0;

 	if ( x[ai] > x[bi] && x[ci] > x[bi] && x[ci] > x[ei] && y[bi] > y[ci] )
	{
		/*Must be furfilled to be a YEH type, but it doesn't mean it can not be a KAF type */
		if ( 5*((DECUMA_INT32)x[ci] - x[bi]) > 2*((DECUMA_INT32)x[ci] - x[di]) )
		{
			/*Most likely a YEH type */
			pZoomOut->bestCandidate = pZoomIn->likelyCandidateIfTestPasses;
		}

		else
		{
			/*Could possibly be a KAF type */
			/*Do another comparison */
			if ( 20*((DECUMA_INT32)x[ai] - x[bi]) <= ((DECUMA_INT32)x[ci] - x[di]) )
			{
				/*Most likely a KAF type */
				pZoomOut->bestCandidate = !pZoomIn->likelyCandidateIfTestPasses;
			}

			else
			{
				/*Most likely a YEH type */
				pZoomOut->bestCandidate = pZoomIn->likelyCandidateIfTestPasses;
			}
		}
	}

	else
	{
		/*Actually undefined but most likely NOT a YEH type */
		pZoomOut->bestCandidate = !pZoomIn->likelyCandidateIfTestPasses;
	}

}

static void testBetter_dal_than_lam(ZOOM_IN * pZoomIn, ZOOM_OUT * pZoomOut)
{

	/*
                           ai
                           **
                           * *
                           *
                           **
                            *
                            *
                            *
                            *
                            *
                            *
             ei            **ri
             *             *
             *            **
              **         **
               **       **
                 *******
                   bi

	IF {y[ei] - y[bi]} / {y[ai] - y[bi]} < {3/20} --------------> DAL
	ELSE -------------------------------------------------------> LAM
	*/

	/* JM: Problem noted with dal, type 2 which also has a J-looking bend in the end */

	int i, si, bi, ri, ei;
	DECUMA_BOOL handleTwitch;
	DECUMA_INT32 lhs, rhs;
	const int minMuDiffForZoom = 100;

	DECUMA_INT16 x[NUMBER_OF_POINTS_IN_ARC];
	DECUMA_INT16 y[NUMBER_OF_POINTS_IN_ARC];
	DECUMA_INT16 dx[NUMBER_OF_POINTS_IN_ARC];
	DECUMA_INT16 dy[NUMBER_OF_POINTS_IN_ARC];

	SCR_ARC * pArc = pZoomIn->pCurve->Arcs[0];

	pZoomOut->zoomQuality = 0; /*Not used; */
	pZoomOut->bestCandidate = 0;

	if (pZoomIn->pOutput[0].mu + pZoomIn->pOutput[0].punish + minMuDiffForZoom <
		pZoomIn->pOutput[1].mu + pZoomIn->pOutput[1].punish)
	{
		/* The mu-value is quite distinguishing. Use that and don't do zooming*/
		return;
	}

	if( !pArc )
	{
		decumaAssert(0); /* NULL-pointers supported but might be used unintended. */
						 /* Avoid this in debug mode for now. */
		return;
	}

	copy_arc(pArc, x, y, dx, dy, NUMBER_OF_POINTS_IN_ARC);

	i = 0;
	handleTwitch = TRUE; /* Handle twitches */

	/*Remove peak in the beginning to avoid algorithm crash */
	while( peak(dx, dy, i + 1, 50) ) i++;

	i = go_up(i, x, y, dx, dy, MIN_DECUMA_INT16, MAX_DECUMA_INT16, MIN_DECUMA_INT16, MAX_DECUMA_INT16, handleTwitch);
	si = i;
	bi = i + indexToMin(&y[i], NUMBER_OF_POINTS_IN_ARC - i);
	ri = i + indexToMax(&x[i], NUMBER_OF_POINTS_IN_ARC - i);

	ei = NUMBER_OF_POINTS_IN_ARC - 1; /* Go to last point */

	lhs = 20 * ( (DECUMA_INT32)y[ei] - y[bi] );
	rhs = 3 * ( (DECUMA_INT32)y[si] - y[bi] );

	if( lhs < rhs )
	{
		pZoomOut->bestCandidate = pZoomIn->likelyCandidateIfTestPasses;
	}
	else
	{
		pZoomOut->bestCandidate = !pZoomIn->likelyCandidateIfTestPasses;
	}

}

static void testBetter_dad_than_feh_c(ZOOM_IN * pZoomIn, ZOOM_OUT * pZoomOut)
{
	int i, ai, bi, ci, di, mi;

	DECUMA_INT16 x[NUMBER_OF_POINTS_IN_ARC];
	DECUMA_INT16 y[NUMBER_OF_POINTS_IN_ARC];
	DECUMA_INT16 dx[NUMBER_OF_POINTS_IN_ARC];
	DECUMA_INT16 dy[NUMBER_OF_POINTS_IN_ARC];

	SCR_ARC * pArc = pZoomIn->pCurve->Arcs[0];

	if( !pArc )
	{
		decumaAssert(0); /* NULL-pointers supported but might be used unintended. */
						 /* Avoid this in debug mode for now. */
		return;
	}

	/*The arc's indices are flipped to enable easier calculation of key points */
	flip_arc(pArc, x, y, dx, dy, NUMBER_OF_POINTS_IN_ARC);

	i = 0;
	/* Remove peak in the beginning to avoid algorithm crash */
	while( peak(dx, dy, i + 1, 50) ) i++;

	ai = i + indexToMin(&x[i], NUMBER_OF_POINTS_IN_ARC - i);
	decumaAssert(i < NUMBER_OF_POINTS_IN_ARC - i - 2);

	bi = ai + indexToMin(&y[ai], NUMBER_OF_POINTS_IN_ARC - ai - 2); /*Skip 2 last points */
	di = bi + indexToMax(&x[bi], NUMBER_OF_POINTS_IN_ARC - bi);
	decumaAssert(di - 3 > bi);

	ci = bi + indexToMax(&y[bi], di - bi - 3);
	decumaAssert(di > ci);

	mi = ci + lastIndexToMin(&y[ci], di - ci);

	pZoomOut->zoomQuality = 0; /*Not used; */
	pZoomOut->bestCandidate = 0;

	if ( ai < bi && bi < ci && ci < di )
	{
		if ( x[ai] < x[bi] &&
			 x[bi] < x[ci] &&
			 x[ci] < x[di] &&
			 y[bi] < y[ci] &&
			 x[ci] < x[mi] )
		{
			/*Most likely DAD type */
			pZoomOut->bestCandidate = pZoomIn->likelyCandidateIfTestPasses;
		}

		else
		{
			/*Most likely FEH type */
			pZoomOut->bestCandidate = !pZoomIn->likelyCandidateIfTestPasses;
		}
	}

	else
	{
		/*Something is really wrong with arc */
		/*Return */
		return;
	}
}

static void testBetter_4_than_9(ZOOM_IN * pZoomIn, ZOOM_OUT * pZoomOut)
{
	int i, ai, ci, di;

	DECUMA_INT16 x[NUMBER_OF_POINTS_IN_ARC];
	DECUMA_INT16 y[NUMBER_OF_POINTS_IN_ARC];
	DECUMA_INT16 dx[NUMBER_OF_POINTS_IN_ARC];
	DECUMA_INT16 dy[NUMBER_OF_POINTS_IN_ARC];

	SCR_ARC * pArc = pZoomIn->pCurve->Arcs[0];

	if( !pArc )
	{
		decumaAssert(0); /* NULL-pointers supported but might be used unintended. */
						 /* Avoid this in debug mode for now. */
		return;
	}

	copy_arc(pArc, x, y, dx, dy, NUMBER_OF_POINTS_IN_ARC);

	/* Skip possible peak at start of arc */
	i = 0;
	while( peak(dx, dy, i + 1, 50) ) i++;

	ai = i;
	ci = i + lastIndexToMin(&x[i], NUMBER_OF_POINTS_IN_ARC - i - 5); /* Skip last 5 points */
	di = i + indexToMax(&y[i], NUMBER_OF_POINTS_IN_ARC - i);

	/* If either of these conditions hold, use zoom function testBetter_a_than_u */
	if ( ai == di || ai == ci || x[di] > x[ai] || (ai < di && di < ci) )
	{
		testBetter_a_than_u(pZoomIn, pZoomOut);
	}

	/* Otherwise do nothing */

}

static void testBetter_seen_than_farsi_yeh(ZOOM_IN * pZoomIn, ZOOM_OUT * pZoomOut)
{
	int i, peakCounter, startY, startsWithUpBow, startIndex;

	DECUMA_INT16 x[NUMBER_OF_POINTS_IN_ARC];
	DECUMA_INT16 y[NUMBER_OF_POINTS_IN_ARC];
	DECUMA_INT16 dx[NUMBER_OF_POINTS_IN_ARC];
	DECUMA_INT16 dy[NUMBER_OF_POINTS_IN_ARC];

	SCR_ARC * pArc = pZoomIn->pCurve->Arcs[0];

	const int minMuDiffForZoom = 100;

	if (pZoomIn->pOutput[0].mu + pZoomIn->pOutput[0].punish + minMuDiffForZoom <
		pZoomIn->pOutput[1].mu + pZoomIn->pOutput[1].punish)
	{
		/* The mu-value is quite distinguishing. Use that and don't do zooming*/
		return;
	}

	if( !pArc )
	{
		decumaAssert(0); /* NULL-pointers supported but might be used unintended. */
						 /* Avoid this in debug mode for now. */
		return;
	}

	copy_arc(pArc, x, y, dx, dy, NUMBER_OF_POINTS_IN_ARC);

	/* Remove peak at the start */
	startIndex = 0;
	while( peak(dx, dy, startIndex + 1, 50) && startIndex < 5) startIndex++;

	/* Count peaks, works for more or less correctly written seen. */
	peakCounter = 0;
	startY = y[startIndex];
	startsWithUpBow = 0;
	for (i = startIndex; i < NUMBER_OF_POINTS_IN_ARC; i++)
	{
		if (peak(dx, dy, i, 40)) peakCounter++;
		if (i < NUMBER_OF_POINTS_IN_ARC/2 && y[i] > startY) startsWithUpBow = 1;
	}


	/* Check if the starting point is lower than any point in the first half */


	if (peakCounter > 2 || !startsWithUpBow)
		pZoomOut->bestCandidate = pZoomIn->likelyCandidateIfTestPasses;
	else
		pZoomOut->bestCandidate = !pZoomIn->likelyCandidateIfTestPasses;
}

static void testBetter_dad_than_feh(ZOOM_IN * pZoomIn, ZOOM_OUT * pZoomOut)
{
	int i;
	DECUMA_INT32 height, width;
	DECUMA_INT16 yMin, yMax, xMin, xMax;

	DECUMA_INT16 x[NUMBER_OF_POINTS_IN_ARC];
	DECUMA_INT16 y[NUMBER_OF_POINTS_IN_ARC];
	DECUMA_INT16 dx[NUMBER_OF_POINTS_IN_ARC];
	DECUMA_INT16 dy[NUMBER_OF_POINTS_IN_ARC];

	SCR_ARC * pArc = pZoomIn->pCurve->Arcs[0];

	if( !pArc )
	{
		decumaAssert(0); /* NULL-pointers supported but might be used unintended. */
						 /* Avoid this in debug mode for now. */
		return;
	}

	/* First arc */
	flip_arc(pArc, x, y, dx, dy, NUMBER_OF_POINTS_IN_ARC);
	yMax = maxNUMBER(y, NUMBER_OF_POINTS_IN_ARC);
	yMin = minNUMBER(y, NUMBER_OF_POINTS_IN_ARC);
	height = (DECUMA_INT32)yMax - yMin;

	/* Second arc */
	pArc = pZoomIn->pCurve->Arcs[1];
	flip_arc(pArc, x, y, dx, dy, NUMBER_OF_POINTS_IN_ARC);
	yMax = maxNUMBER(y, NUMBER_OF_POINTS_IN_ARC);
	yMin = minNUMBER(y, NUMBER_OF_POINTS_IN_ARC);

	/*Compare sizes */
	if ((DECUMA_INT32)yMax - yMin < height)
	{
	  pArc = pZoomIn->pCurve->Arcs[0];
	  flip_arc(pArc, x, y, dx, dy, NUMBER_OF_POINTS_IN_ARC);
	  yMax = maxNUMBER(y, NUMBER_OF_POINTS_IN_ARC);
	  yMin = minNUMBER(y, NUMBER_OF_POINTS_IN_ARC);
	}

	xMax = maxNUMBER(x, NUMBER_OF_POINTS_IN_ARC);
	xMin = minNUMBER(x, NUMBER_OF_POINTS_IN_ARC);

	width = (DECUMA_INT32)xMax - xMin;

	/* Find index for spatial middle */
	i = 0;
	while ( i < NUMBER_OF_POINTS_IN_ARC && x[i] < x[0] + (width)/2 )
	{
		i++;
	}

	yMin = minNUMBER(y, i);
	/* Compare width and height of half the char, should be closer to equal for dad and not for feh */
	/* Compare height of starting point with height of the end of the first half of the char. */
	if ( 1.0*((DECUMA_INT32)y[i] - yMin) >= ((DECUMA_INT32)y[0] - yMin)*0.5 && 1.0*((DECUMA_INT32)y[0] - yMin) >= (x[i] - x[0])*0.5)
		pZoomOut->bestCandidate = pZoomIn->likelyCandidateIfTestPasses;
	else
		pZoomOut->bestCandidate = !pZoomIn->likelyCandidateIfTestPasses;
}

static void testBetter_thal_than_zain(ZOOM_IN * pZoomIn, ZOOM_OUT * pZoomOut)
{
	int i;
	DECUMA_INT32 height;
	DECUMA_INT16 yMin, yMax, xMin, xMax;

	DECUMA_INT16 x[NUMBER_OF_POINTS_IN_ARC];
	DECUMA_INT16 y[NUMBER_OF_POINTS_IN_ARC];
	DECUMA_INT16 dx[NUMBER_OF_POINTS_IN_ARC];
	DECUMA_INT16 dy[NUMBER_OF_POINTS_IN_ARC];

	double area, length, lineLength;

	DECUMA_BOOL error;

	SCR_ARC * pArc = pZoomIn->pCurve->Arcs[0];

	const int minMuDiffForZoom = 100;

	if (pZoomIn->pOutput[0].mu + pZoomIn->pOutput[0].punish + minMuDiffForZoom <
		pZoomIn->pOutput[1].mu + pZoomIn->pOutput[1].punish)
	{
		/* The mu-value is quite distinguishing. Use that and don't do zooming*/
		return;
	}

	if( !pArc )
	{
		decumaAssert(0); /* NULL-pointers supported but might be used unintended. */
						 /* Avoid this in debug mode for now. */
		return;
	}

	/* First arc */
	copy_arc(pArc, x, y, dx, dy, NUMBER_OF_POINTS_IN_ARC);
	yMax = maxNUMBER(y, NUMBER_OF_POINTS_IN_ARC);
	yMin = minNUMBER(y, NUMBER_OF_POINTS_IN_ARC);
	height = (DECUMA_INT32)yMax - yMin;

	/* Second arc */
	pArc = pZoomIn->pCurve->Arcs[1];
	copy_arc(pArc, x, y, dx, dy, NUMBER_OF_POINTS_IN_ARC);
	yMax = maxNUMBER(y, NUMBER_OF_POINTS_IN_ARC);
	yMin = minNUMBER(y, NUMBER_OF_POINTS_IN_ARC);

	/*Compare sizes */
	if ((DECUMA_INT32)yMax - yMin < height)
	{
	  pArc = pZoomIn->pCurve->Arcs[0];
	  copy_arc(pArc, x, y, dx, dy, NUMBER_OF_POINTS_IN_ARC);
	  yMax = maxNUMBER(y, NUMBER_OF_POINTS_IN_ARC);
	  yMin = minNUMBER(y, NUMBER_OF_POINTS_IN_ARC);
	}

	xMax = maxNUMBER(x, NUMBER_OF_POINTS_IN_ARC);
	xMin = minNUMBER(x, NUMBER_OF_POINTS_IN_ARC);

	length = 0;
	for (i = 1; i < NUMBER_OF_POINTS_IN_ARC; i++)
	{
		length += decumaSqrt(((DECUMA_INT32)x[i-1] - x[i])*((DECUMA_INT32)x[i-1] - x[i]) +
			((DECUMA_INT32)y[i-1] - y[i])*((DECUMA_INT32)y[i-1] - y[i]));
	}

	area = integrateArc(x, y, 0, NUMBER_OF_POINTS_IN_ARC-1, TRUE, &error);
	if (error) return;

	/* Check if the area between the ends of the bend is a big enough part of the enclosing area. */
	lineLength = decumaSqrt(((DECUMA_INT32)x[NUMBER_OF_POINTS_IN_ARC-1] - x[0])*((DECUMA_INT32)x[NUMBER_OF_POINTS_IN_ARC-1] - x[0]) +
		((DECUMA_INT32)y[NUMBER_OF_POINTS_IN_ARC-1] - y[0])*((DECUMA_INT32)y[NUMBER_OF_POINTS_IN_ARC-1] - y[0]));
	if ( (length >= 1.1*lineLength) && decumaAbs(area) >= 0.28*((DECUMA_INT32)yMax-yMin)*((DECUMA_INT32)xMax-xMin) )
		pZoomOut->bestCandidate = pZoomIn->likelyCandidateIfTestPasses;
	else
		pZoomOut->bestCandidate = !pZoomIn->likelyCandidateIfTestPasses;
}


static void testBetter_lam_than_yeh(ZOOM_IN * pZoomIn, ZOOM_OUT * pZoomOut)
{
	/*


				  ****
                 *    *
                *      *
               *
               *
	*           *
	*            **
    **             *
     **             *
      **           *
        ***    ****
          ******


                   *
                   *
                   *
                   *
                   *
                   *
                   *
    *             **
	*             *
	**            *
	 **          **
	  ***      ***
	    ********

	*/

	DECUMA_INT16 yMin, yMax, xMax, xMin;

	DECUMA_INT16 x[NUMBER_OF_POINTS_IN_ARC];
	DECUMA_INT16 y[NUMBER_OF_POINTS_IN_ARC];
	DECUMA_INT16 dx[NUMBER_OF_POINTS_IN_ARC];
	DECUMA_INT16 dy[NUMBER_OF_POINTS_IN_ARC];

	int i;

	SCR_ARC * pArc = pZoomIn->pCurve->Arcs[0];

	if( !pArc )
	{
		decumaAssert(0); /* NULL-pointers supported but might be used unintended. */
						 /* Avoid this in debug mode for now. */
		return;
	}

	flip_arc(pArc, x, y, dx, dy, NUMBER_OF_POINTS_IN_ARC);

	/* From the left endpoint. */
	/* Find the upward bend. */
	i = indexToMin(y, NUMBER_OF_POINTS_IN_ARC);
	while ( i < NUMBER_OF_POINTS_IN_ARC && dy[i] < 2*dx[i]) {i++;};

	xMin = minNUMBER(&x[i], NUMBER_OF_POINTS_IN_ARC-i);
	xMax = maxNUMBER(&x[i], NUMBER_OF_POINTS_IN_ARC-i);
	yMin = minNUMBER(&y[i], NUMBER_OF_POINTS_IN_ARC-i);
	yMax = maxNUMBER(&y[i], NUMBER_OF_POINTS_IN_ARC-i);

	/* If the right half is wide enough with respect to its height, it is a yeh */
	if ( 3.8*((DECUMA_INT32)xMax-xMin) >= ((DECUMA_INT32)yMax-yMin))
		pZoomOut->bestCandidate = !pZoomIn->likelyCandidateIfTestPasses;
	else
		pZoomOut->bestCandidate = pZoomIn->likelyCandidateIfTestPasses;
}


static void testBetter_dal_than_reh(ZOOM_IN * pZoomIn, ZOOM_OUT * pZoomOut)
{
	DECUMA_INT16 xMin, xMax;

	DECUMA_INT16 x[NUMBER_OF_POINTS_IN_ARC];
	DECUMA_INT16 y[NUMBER_OF_POINTS_IN_ARC];
	DECUMA_INT16 dx[NUMBER_OF_POINTS_IN_ARC];
	DECUMA_INT16 dy[NUMBER_OF_POINTS_IN_ARC];

	SCR_ARC * pArc = pZoomIn->pCurve->Arcs[0];

	if( !pArc )
	{
		decumaAssert(0); /* NULL-pointers supported but might be used unintended. */
						 /* Avoid this in debug mode for now. */
		return;
	}

	copy_arc(pArc, x, y, dx, dy, NUMBER_OF_POINTS_IN_ARC);

	xMax = maxNUMBER(x, NUMBER_OF_POINTS_IN_ARC);
	xMin = minNUMBER(x, NUMBER_OF_POINTS_IN_ARC);

	/* Check if the top part of the bend is streching back far enough. */
	if ( x[0] < xMin+((DECUMA_INT32)xMax-xMin)*0.7 )
		pZoomOut->bestCandidate = pZoomIn->likelyCandidateIfTestPasses;
	else
		pZoomOut->bestCandidate = !pZoomIn->likelyCandidateIfTestPasses;
}


static void testBetter_teh_than_tteh(ZOOM_IN * pZoomIn, ZOOM_OUT * pZoomOut)
{
	/* Check that the bounding boxes of the diacritic marks doesn't overlap.
	   Check height of the diacritic arc compared to the height of the same. */
	DECUMA_INT32 height;
	DECUMA_INT16 yMin, yMax, xMin, xMax;

	DECUMA_INT16 x[NUMBER_OF_POINTS_IN_ARC];
	DECUMA_INT16 y[NUMBER_OF_POINTS_IN_ARC];
	DECUMA_INT16 dx[NUMBER_OF_POINTS_IN_ARC];
	DECUMA_INT16 dy[NUMBER_OF_POINTS_IN_ARC];

	SCR_ARC * pArc;

	if (pZoomIn->pCurve->noArcs == 3)
	{
		testBetter_teh_than_tteh_3arced(pZoomIn, pZoomOut);
		return;
	}

	pArc = pZoomIn->pCurve->Arcs[0];
	if( !pArc )
	{
		decumaAssert(0); /* NULL-pointers supported but might be used unintended. */
						 /* Avoid this in debug mode for now. */
		return;
	}

	/* First arc */
	copy_arc(pArc, x, y, dx, dy, NUMBER_OF_POINTS_IN_ARC);
	yMax = maxNUMBER(y, NUMBER_OF_POINTS_IN_ARC);
	yMin = minNUMBER(y, NUMBER_OF_POINTS_IN_ARC);
	height = (DECUMA_INT32)yMax - yMin;

	/* Second arc */
	pArc = pZoomIn->pCurve->Arcs[1];
	copy_arc(pArc, x, y, dx, dy, NUMBER_OF_POINTS_IN_ARC);
	yMax = maxNUMBER(y, NUMBER_OF_POINTS_IN_ARC);
	yMin = minNUMBER(y, NUMBER_OF_POINTS_IN_ARC);

	/*Compare sizes */
	if ((DECUMA_INT32)yMax - yMin < height)
	{
	  pArc = pZoomIn->pCurve->Arcs[0];
	  copy_arc(pArc, x, y, dx, dy, NUMBER_OF_POINTS_IN_ARC);
	  yMax = maxNUMBER(y, NUMBER_OF_POINTS_IN_ARC);
	  yMin = minNUMBER(y, NUMBER_OF_POINTS_IN_ARC);
	}

	xMax = maxNUMBER(x, NUMBER_OF_POINTS_IN_ARC);
	xMin = minNUMBER(x, NUMBER_OF_POINTS_IN_ARC);

	if ( ((DECUMA_INT32)xMax-xMin) > 2*((DECUMA_INT32)yMax-yMin))
		pZoomOut->bestCandidate = pZoomIn->likelyCandidateIfTestPasses;
	else
		pZoomOut->bestCandidate = !pZoomIn->likelyCandidateIfTestPasses;
}

static void testBetter_teh_than_tteh_3arced(ZOOM_IN * pZoomIn, ZOOM_OUT * pZoomOut)
{
	/* Compare 3-arced teh and tteh.
	   Check that the bounding boxes of the diacritic marks doesn't overlap.
	   Check height of the diacritic arc compared to the height of the same. */
	int i;
	DECUMA_INT16 yMin[2], yMax[2], xMin[2], xMax[2];
	DECUMA_INT32 height[3];

	DECUMA_INT16 x[NUMBER_OF_POINTS_IN_ARC];
	DECUMA_INT16 y[NUMBER_OF_POINTS_IN_ARC];
	DECUMA_INT16 dx[NUMBER_OF_POINTS_IN_ARC];
	DECUMA_INT16 dy[NUMBER_OF_POINTS_IN_ARC];

	SCR_ARC * pArc[2];

	for (i = 0; i < 3; i++)
	{
		pArc[0] = pZoomIn->pCurve->Arcs[i];
		if( !pArc[0] )
		{
			decumaAssert(0); /* NULL-pointers supported but might be used unintended. */
						 /* Avoid this in debug mode for now. */
			return;
		}
		copy_arc(pArc[0], x, y, dx, dy, NUMBER_OF_POINTS_IN_ARC);
		yMax[0] = maxNUMBER(y, NUMBER_OF_POINTS_IN_ARC);
		yMin[0] = minNUMBER(y, NUMBER_OF_POINTS_IN_ARC);
		height[i] = (DECUMA_INT32)yMax[0] - yMin[0];

	}


	/* Find the two small arcs */
	if (height[2] < height[0])
	{
		if (height[0] < height[1])
		{
			/* height[0] and height[2] */
			pArc[0] =  pZoomIn->pCurve->Arcs[0];
			pArc[1] =  pZoomIn->pCurve->Arcs[2];
		}
		else /* (height[0] > height[1]) */
		{
			/* height[1] and height[2] */
			pArc[0] =  pZoomIn->pCurve->Arcs[1];
			pArc[1] =  pZoomIn->pCurve->Arcs[2];
		}
	}
	else /* height[2] > height[0] */
	{
		if (height[2] < height[1])
		{
			/* height[0] and height[2] */
			pArc[0] =  pZoomIn->pCurve->Arcs[0];
			pArc[1] =  pZoomIn->pCurve->Arcs[2];
		}
		else /* (height[0] > height[1]) */
		{
			/* height[1] and height[0] */
			pArc[0] =  pZoomIn->pCurve->Arcs[0];
			pArc[1] =  pZoomIn->pCurve->Arcs[1];
		}
	}

	for (i = 0; i < 2; i++)
	{
		copy_arc(pArc[i], x, y, dx, dy, NUMBER_OF_POINTS_IN_ARC);
		yMax[i] = maxNUMBER(y, NUMBER_OF_POINTS_IN_ARC);
		yMin[i] = minNUMBER(y, NUMBER_OF_POINTS_IN_ARC);
		xMax[i] = maxNUMBER(x, NUMBER_OF_POINTS_IN_ARC);
		xMin[i] = minNUMBER(x, NUMBER_OF_POINTS_IN_ARC);
	}

	/* Check if bounding boxes overlap */
	if ( ((xMax[0] > xMin[1]) && (xMin[0] < xMin[1])) ||
			((xMax[1] > xMin[0]) && (xMax[1] < xMax[0])) )
		pZoomOut->bestCandidate = !pZoomIn->likelyCandidateIfTestPasses;
	else
		pZoomOut->bestCandidate = pZoomIn->likelyCandidateIfTestPasses;
}

static void testBetter_ghain_than_khah(ZOOM_IN * pZoomIn, ZOOM_OUT * pZoomOut)
{
	/*     khah                          ghain
	(i0)xxxxxxxxxxxxx (i1)            xxxxxxx(i0,i1)
		x           x                x       xx
		 x         x                x
		  x       x           (i2) x
				 x                  x
			   xx                    xx
			 xx                        xxxx
		   xx                              xxx (i3)
		  x                             xxx
	 (i2) x         x (i3)            xx
		  x        x                 x
		   xxxxxxxx                 x
									x           x
									 x          x
									  x       xx
									   xxxxxxx

	If i3 is far away along the curve, it is khah.
	JM: Note this is not stable if i0 (which is selected as the point with idx=3)
	    is found before the first right turn on a khah. Then i1,i2 and i3 will be
		at completely different positions and the outcome will probably be wrong.
	*/



	/* Check that the bounding boxes of the diacritic marks doesn't overlap.
	   Check height of the diacritic arc compared to the height of the same. */

	DECUMA_INT32 height;
	DECUMA_INT16 yMax, yMin;
	DECUMA_BOOL handleTwitch;

	DECUMA_INT16 xlimlow, xlimhigh, ylimlow, ylimhigh;
	int i, index1, index2, index3;

	DECUMA_INT16 x[NUMBER_OF_POINTS_IN_ARC];
	DECUMA_INT16 y[NUMBER_OF_POINTS_IN_ARC];
	DECUMA_INT16 dx[NUMBER_OF_POINTS_IN_ARC];
	DECUMA_INT16 dy[NUMBER_OF_POINTS_IN_ARC];

	SCR_ARC * pArc;

	pArc = pZoomIn->pCurve->Arcs[0];
	if( !pArc )
	{
		decumaAssert(0); /* NULL-pointers supported but might be used unintended. */
						 /* Avoid this in debug mode for now. */
		return;
	}

	/* First arc */
	copy_arc(pArc, x, y, dx, dy, NUMBER_OF_POINTS_IN_ARC);
	yMax = maxNUMBER(y, NUMBER_OF_POINTS_IN_ARC);
	yMin = minNUMBER(y, NUMBER_OF_POINTS_IN_ARC);
	height = (DECUMA_INT32)yMax - yMin;

	/* Second arc */
	pArc = pZoomIn->pCurve->Arcs[1];
	copy_arc(pArc, x, y, dx, dy, NUMBER_OF_POINTS_IN_ARC);
	yMax = maxNUMBER(y, NUMBER_OF_POINTS_IN_ARC);
	yMin = minNUMBER(y, NUMBER_OF_POINTS_IN_ARC);

	/*Compare sizes */
	if ((DECUMA_INT32)yMax - yMin < height)
	{
	  pArc = pZoomIn->pCurve->Arcs[0];
	  copy_arc(pArc, x, y, dx, dy, NUMBER_OF_POINTS_IN_ARC);
	}

	i = 3;

	xlimlow  = MIN_DECUMA_INT16;
	xlimhigh = MAX_DECUMA_INT16;
	ylimlow  = MIN_DECUMA_INT16;
	ylimhigh = MAX_DECUMA_INT16;

	handleTwitch = TRUE; /* Handle twitches */

	/* Remove peak in the end to avoid algorithm crash */
	while( peak(dx, dy, i + 1, 50) )
	{
		i += 1;
	}

	i = go_right(i, x, y, dx, dy,
			  xlimlow, xlimhigh, ylimlow,
			  ylimhigh, handleTwitch);
	index1 = i;

	i = go_left(i, x, y, dx, dy,
			  xlimlow, xlimhigh, ylimlow,
			  ylimhigh, handleTwitch);
	index2 = i;


	i = go_right(i, x, y, dx, dy,
			  xlimlow, xlimhigh, ylimlow,
			  ylimhigh, handleTwitch);
	index3 = i;


	/* Check if we are far enough along the curve by this point for it to be a khah. */
	if (index3 > NUMBER_OF_POINTS_IN_ARC-5)
		pZoomOut->bestCandidate = !pZoomIn->likelyCandidateIfTestPasses;
	else
		pZoomOut->bestCandidate = pZoomIn->likelyCandidateIfTestPasses;
}


static void testBetter_aitwo_than_aithree(ZOOM_IN * pZoomIn, ZOOM_OUT * pZoomOut)
{
	/* Basically walk along the curve and check how far we have come when
	   we have gone down-up. If it is a "two" we should be near the end. */
	DECUMA_INT16 xMax, xMin;
	DECUMA_BOOL handleTwitch;

	DECUMA_INT16 xlimlow, xlimhigh, ylimlow, ylimhigh;
	int i, index1, index2, index3;

	DECUMA_INT16 x[NUMBER_OF_POINTS_IN_ARC];
	DECUMA_INT16 y[NUMBER_OF_POINTS_IN_ARC];
	DECUMA_INT16 dx[NUMBER_OF_POINTS_IN_ARC];
	DECUMA_INT16 dy[NUMBER_OF_POINTS_IN_ARC];

	SCR_ARC * pArc;

	pArc = pZoomIn->pCurve->Arcs[0];
	if( !pArc )
	{
		decumaAssert(0); /* NULL-pointers supported but might be used unintended. */
						 /* Avoid this in debug mode for now. */
		return;
	}

	/* First arc */
	copy_arc(pArc, x, y, dx, dy, NUMBER_OF_POINTS_IN_ARC);
	xMax = maxNUMBER(x, NUMBER_OF_POINTS_IN_ARC);
	xMin = minNUMBER(x, NUMBER_OF_POINTS_IN_ARC);

	i = 4;

	xlimlow  = MIN_DECUMA_INT16;
	xlimhigh = MAX_DECUMA_INT16;
	ylimlow  = MIN_DECUMA_INT16;
	ylimhigh = MAX_DECUMA_INT16;

	handleTwitch = TRUE; /* Handle twitches */

	/* Remove peak in the end to avoid algorithm crash */
	while( peak(dx, dy, i + 1, 50) )
	{
		i += 1;
	}

	i = go_down(i, x, y, dx, dy,
			  xlimlow, xlimhigh, ylimlow,
			  ylimhigh, handleTwitch);
	index1 = i;

	i = go_up(i, x, y, dx, dy,
			  xlimlow, xlimhigh, ylimlow,
			  ylimhigh, handleTwitch);
	index2 = i;


	i = go_down(i, x, y, dx, dy,
			  xlimlow, xlimhigh, ylimlow,
			  ylimhigh, handleTwitch);
	index3 = i;


	/* Check if we are far enough along the curve by this point for it to be a two. */
	if ( x[index2] < xMin + ((DECUMA_INT32)xMax - xMin)/3)
		pZoomOut->bestCandidate = pZoomIn->likelyCandidateIfTestPasses;
	else
		pZoomOut->bestCandidate = !pZoomIn->likelyCandidateIfTestPasses;
}

static void testBetter_1dot_than_3dots(ZOOM_IN * pZoomIn, ZOOM_OUT * pZoomOut)
{
	/* Sometimes 3 dots are written as a circle. This creates conflicts when the circle is dot-like.
	   Compare length of stroke to the area it covers. */

  DECUMA_INT16 yMin, yMax, xMin, xMax;
  DECUMA_INT32 height;

  DECUMA_INT16 x[NUMBER_OF_POINTS_IN_ARC];
  DECUMA_INT16 y[NUMBER_OF_POINTS_IN_ARC];
  DECUMA_INT16 dx[NUMBER_OF_POINTS_IN_ARC];
  DECUMA_INT16 dy[NUMBER_OF_POINTS_IN_ARC];

  DECUMA_BOOL error;
  double area, arcLength;

  int i;
  SCR_ARC * pArc;

  area = 0;
  pZoomOut->zoomQuality = 0; /*Not used; */
  pZoomOut->bestCandidate = 0;

  /*Assume the letter consists of only two arcs */
  /*Get the first arc */
  pArc = pZoomIn->pCurve->Arcs[0];

  if (!pArc)
  {
    decumaAssert(0); /* NULL-pointers supported but might be used unintended. */
    /* Avoid this in debug mode for now. */
    return;
  }

  /* Measure vertical size */

  /* First arc */
  copy_arc(pArc, x, y, dx, dy, NUMBER_OF_POINTS_IN_ARC);
  height = (DECUMA_INT32)maxNUMBER(y, NUMBER_OF_POINTS_IN_ARC) - minNUMBER(y, NUMBER_OF_POINTS_IN_ARC);

  /* Second arc */
  pArc = pZoomIn->pCurve->Arcs[1];
  copy_arc(pArc, x, y, dx, dy, NUMBER_OF_POINTS_IN_ARC);
  yMax = maxNUMBER(y, NUMBER_OF_POINTS_IN_ARC);
  yMin = minNUMBER(y, NUMBER_OF_POINTS_IN_ARC);

  if (height < ((DECUMA_INT32)yMax - yMin) )
  {
    pArc = pZoomIn->pCurve->Arcs[0];
    copy_arc(pArc, x, y, dx, dy, NUMBER_OF_POINTS_IN_ARC);
	yMax = maxNUMBER(y, NUMBER_OF_POINTS_IN_ARC);
	yMin = minNUMBER(y, NUMBER_OF_POINTS_IN_ARC);
  }

  xMax = maxNUMBER(x, NUMBER_OF_POINTS_IN_ARC);
  xMin = minNUMBER(x, NUMBER_OF_POINTS_IN_ARC);

  area = integrateArc(x, y, 0, NUMBER_OF_POINTS_IN_ARC-1, FALSE, &error);

  arcLength = 0;
  for (i = 1; i < NUMBER_OF_POINTS_IN_ARC; i++)
  {
    /* OBS: Risk of overflow? */
    arcLength += decumaSqrt(((DECUMA_INT32)x[i-1] - x[i])*((DECUMA_INT32)x[i-1] - x[i]) +
		((DECUMA_INT32)y[i-1] - y[i])*((DECUMA_INT32)y[i-1] - y[i]));
  }

  if (error)
  {
    return;
  }

  if (area == 0 || (xMax-xMin)*(yMax-yMin) == 1)
  	pZoomOut->bestCandidate = pZoomIn->likelyCandidateIfTestPasses;
  else
	  if (decumaAbs( arcLength/( (xMax-xMin)*(yMax-yMin) ) ) >= 0.35)
		pZoomOut->bestCandidate = pZoomIn->likelyCandidateIfTestPasses;
	else
		pZoomOut->bestCandidate = !pZoomIn->likelyCandidateIfTestPasses;
}


/* Hebrew Bet & Final Kaf
   Check that the upper area is triangular enough to be a Bet


      **********
     * ........ **
         ...****
         ***
         *
         *    Bet, 0x05D1
          *
          *
          *
         *


        ******
      **......*
     *.........*
         **....*
         * ****
         *
          *   Final Kaf, 0x05DA
          *
          *
         *

 */
static void testBetter_bet_than_final_kaf(ZOOM_IN * pZoomIn, ZOOM_OUT * pZoomOut)
{
	DECUMA_BOOL handleTwitch;
	DECUMA_BOOL err;

	DECUMA_INT16 xlimlow, xlimhigh, ylimlow, ylimhigh;
	int i, j, index1, index2, index3;

	DECUMA_INT16 x[NUMBER_OF_POINTS_IN_ARC];
	DECUMA_INT16 y[NUMBER_OF_POINTS_IN_ARC];
	DECUMA_INT16 dx[NUMBER_OF_POINTS_IN_ARC];
	DECUMA_INT16 dy[NUMBER_OF_POINTS_IN_ARC];

	DECUMA_INT32 dist_squared[NUMBER_OF_POINTS_IN_ARC];

	SCR_ARC * pArc;

	double real_area;
	double triangle_area;

	pZoomOut->zoomQuality = 0; /*Not used; */

	/*Get the arc */
	pArc = pZoomIn->pCurve->Arcs[0];

	if( !pArc )
	{
		decumaAssert(0); /* NULL-pointers supported but might be used unintended. */
						 /* Avoid this in debug mode for now. */
		return;
	}

	copy_arc(pArc, x, y, dx, dy, NUMBER_OF_POINTS_IN_ARC);

	i = 0;

	xlimlow  = MIN_DECUMA_INT16;
	xlimhigh = MAX_DECUMA_INT16;
	ylimlow  = MIN_DECUMA_INT16;
	ylimhigh = MAX_DECUMA_INT16;

	handleTwitch = TRUE; /* Handle twitches */

	/* Find the left turning point. Might very well be the starting point. */
	i = go_left(i, x, y, dx, dy,
			    xlimlow, xlimhigh, ylimlow,
			    ylimhigh, handleTwitch);
	index1 = i;

	/* Find the right turning point. */
	i = go_right(i, x, y, dx, dy,
			     xlimlow, xlimhigh, ylimlow,
			     ylimhigh, handleTwitch);
	index2 = i;

	/* Find the point after right turning point that is closest to the starting point. */
	for (j = i; j < NUMBER_OF_POINTS_IN_ARC; j++) {
		DECUMA_INT32 d_x = (DECUMA_INT32)x[0] - x[j];
		DECUMA_INT32 d_y = (DECUMA_INT32)y[0] - y[j];
		dist_squared[j] = d_x * d_x + d_y * d_y;
	}

	index3 = i + indexToMin32(&dist_squared[i], NUMBER_OF_POINTS_IN_ARC - i);

	real_area     = integrateArc(x, y, index1, index3, 1, &err);
	triangle_area = triangleArea(x, y, index1, index2, index3, &err);

	/* Check if area is close enough to corresponding triangle to be a "Bet" */
	if (triangle_area != 0 && real_area / triangle_area < 1.35)
		pZoomOut->bestCandidate = pZoomIn->likelyCandidateIfTestPasses;
	else
		pZoomOut->bestCandidate = !pZoomIn->likelyCandidateIfTestPasses;

}


static void testBetter_dalet_than_tsade(ZOOM_IN * pZoomIn, ZOOM_OUT * pZoomOut)
{
/* tsade        dalet
    ____        _____
   /    \      /     \
         |         _  |
		|         / \/   ^
	   <          \_/\   B
	    |             |  ^
		 |            |
   \____/           _/

               <-A->
   A - Checks the distance between the end-points of the character.
   B - Checks the height of the middle loop.

*/

  double endWidth, width;
  int maxXIndex, minXIndex;
  int searchPts[3];
  int middleYmin, middleYmax;

  DECUMA_INT16 xlimlow, xlimhigh, ylimlow, ylimhigh;
  DECUMA_INT16 x[NUMBER_OF_POINTS_IN_ARC];
  DECUMA_INT16 y[NUMBER_OF_POINTS_IN_ARC];
  DECUMA_INT16 dx[NUMBER_OF_POINTS_IN_ARC];
  DECUMA_INT16 dy[NUMBER_OF_POINTS_IN_ARC];

  SCR_ARC * pArc;

  xlimlow  = MIN_DECUMA_INT16;
  xlimhigh = MAX_DECUMA_INT16;
  ylimlow  = MIN_DECUMA_INT16;
  ylimhigh = MAX_DECUMA_INT16;

  pZoomOut->zoomQuality = 0; /*Not used; */
  pZoomOut->bestCandidate = 0;

  /*Get the arc */
  pArc = pZoomIn->pCurve->Arcs[0];

  if (!pArc)
  {
    decumaAssert(0); /* NULL-pointers supported but might be used unintended. */
    /* Avoid this in debug mode for now. */
    return;
  }

  copy_arc(pArc, x, y, dx, dy, NUMBER_OF_POINTS_IN_ARC);
  minXIndex = indexToMin(x, NUMBER_OF_POINTS_IN_ARC);
  maxXIndex = indexToMax(x, NUMBER_OF_POINTS_IN_ARC);
  width = decumaAbs(x[maxXIndex] - x[minXIndex]);
  endWidth = decumaAbs((DECUMA_INT32)x[0] - x[NUMBER_OF_POINTS_IN_ARC-1]);

  searchPts[0] = go_right(5, x, y, dx, dy, xlimlow, xlimhigh, ylimlow, ylimhigh, 1);
  searchPts[1] = go_left(searchPts[0], x, y, dx, dy, xlimlow, xlimhigh, ylimlow, ylimhigh, 0);
  searchPts[2] = go_right(searchPts[1], x, y, dx, dy, xlimlow, xlimhigh, ylimlow, ylimhigh, 0);

  middleYmin = go_down(searchPts[0], x, y, dx, dy, xlimlow, xlimhigh, y[searchPts[2]]-5, y[searchPts[0]]+5, 0);
  middleYmax = go_up(searchPts[1], x, y, dx, dy, xlimlow, xlimhigh, y[searchPts[2]]-5, y[searchPts[0]]+5, 0);

  /* If the middle part is round enough it might be a dalet */
  if (middleYmin != searchPts[1] && middleYmax != searchPts[1])
  {
	if (decumaAbs((DECUMA_INT32)y[middleYmax] - y[middleYmin]) > 0.1*((DECUMA_INT32)maxNUMBER(y, NUMBER_OF_POINTS_IN_ARC) - minNUMBER(y, NUMBER_OF_POINTS_IN_ARC)))
	{
		pZoomOut->bestCandidate = pZoomIn->likelyCandidateIfTestPasses;
		return; /*yes, ugly */
	}
  }
  if (endWidth > 0.3*width)
	{
		pZoomOut->bestCandidate = pZoomIn->likelyCandidateIfTestPasses;
	}
	else
	{
		pZoomOut->bestCandidate = !pZoomIn->likelyCandidateIfTestPasses;
	}
}



static void testBetter_final_tsade_than_final_mem(ZOOM_IN * pZoomIn, ZOOM_OUT * pZoomOut)
{
  double endHeight, height;
  int lowerEndIndex = 0;

  DECUMA_INT16 x[NUMBER_OF_POINTS_IN_ARC];
  DECUMA_INT16 y[NUMBER_OF_POINTS_IN_ARC];
  DECUMA_INT16 dx[NUMBER_OF_POINTS_IN_ARC];
  DECUMA_INT16 dy[NUMBER_OF_POINTS_IN_ARC];

  SCR_ARC * pArc;

  pZoomOut->zoomQuality = 0; /*Not used; */
  pZoomOut->bestCandidate = 0;

  /*Get the arc */
  pArc = pZoomIn->pCurve->Arcs[0];

  if (!pArc)
  {
    decumaAssert(0); /* NULL-pointers supported but might be used unintended. */
    /* Avoid this in debug mode for now. */
    return;
  }

  copy_arc(pArc, x, y, dx, dy, NUMBER_OF_POINTS_IN_ARC);
  if (y[NUMBER_OF_POINTS_IN_ARC-1] < y[0]) lowerEndIndex = NUMBER_OF_POINTS_IN_ARC-1;

  endHeight = decumaAbs((DECUMA_INT32)y[lowerEndIndex]-minNUMBER(y, NUMBER_OF_POINTS_IN_ARC));
  height = (DECUMA_INT32)maxNUMBER(y, NUMBER_OF_POINTS_IN_ARC) - minNUMBER(y, NUMBER_OF_POINTS_IN_ARC);

  if (endHeight > 0.1*height)
  {
	pZoomOut->bestCandidate = pZoomIn->likelyCandidateIfTestPasses;
  }
  else
  {
	pZoomOut->bestCandidate = !pZoomIn->likelyCandidateIfTestPasses;
  }
}

static void testBetter_final_tsade_than_final_fay(ZOOM_IN * pZoomIn, ZOOM_OUT * pZoomOut)
{
  int higherEndIndex = 0;

  DECUMA_INT16 x[NUMBER_OF_POINTS_IN_ARC];
  DECUMA_INT16 y[NUMBER_OF_POINTS_IN_ARC];
  DECUMA_INT16 dx[NUMBER_OF_POINTS_IN_ARC];
  DECUMA_INT16 dy[NUMBER_OF_POINTS_IN_ARC];

  SCR_ARC * pArc;

  /* JM: This zoom function seems unstable
     First the higherEndIndex is perhaps not always true.
	 Then from the templates it seems like both
	 unicodes could have and normally has right-pointing endings. */

  pZoomOut->zoomQuality = 0; /*Not used; */
  pZoomOut->bestCandidate = 0;

  /*Get the arc */
  pArc = pZoomIn->pCurve->Arcs[0];

  if (!pArc)
  {
    decumaAssert(0); /* NULL-pointers supported but might be used unintended. */
    /* Avoid this in debug mode for now. */
    return;
  }

  copy_arc(pArc, x, y, dx, dy, NUMBER_OF_POINTS_IN_ARC);
  if (y[NUMBER_OF_POINTS_IN_ARC-1] > y[0]) higherEndIndex = NUMBER_OF_POINTS_IN_ARC-1;


	/* Remove peak in the end to avoid algorithm crash */
  while( peak(dx, dy, higherEndIndex--, 50) ) {};

  /* if the end of the stroke is pointing leftwards or is to the left of the minimum it is a final tsade */
  if (dx[higherEndIndex] < 0 || (dx[higherEndIndex] > 0 && x[higherEndIndex] < x[indexToMin(y, NUMBER_OF_POINTS_IN_ARC)]))
  {
	pZoomOut->bestCandidate = pZoomIn->likelyCandidateIfTestPasses;
  }
  else
  {
	pZoomOut->bestCandidate = !pZoomIn->likelyCandidateIfTestPasses;
  }
}

/*/////////////////////////////////////// */
/* Implementation of help functions    // */
/*/////////////////////////////////////// */

static void copy_arc(const SCR_ARC *pArc, DECUMA_INT16 *x, DECUMA_INT16 *y, DECUMA_INT16 *dx, DECUMA_INT16 *dy, int n)
/* Copy pArc and change y and dy sign, so that the y-axis */
/* point upwards. In case of NULL-pointer arguments		  */
/* nothing is done.										  */
{
	int i;

	if( !(pArc && x && y && dx && dy) )
	{
		decumaAssert(0); /* Cannot produce output. Very serious. */
		return;
	}

	if( !(pArc->x && pArc->y && pArc->dx && pArc->dy) )
	{
		decumaAssert(0);  /* Cannot produce output. Very serious. */
		return;
	}

	for( i = 0; i < n; i++)
	{
		x[i]  = pArc->x[i];
		y[i]  = (DECUMA_INT16)-maxNUMB(pArc->y[i],MIN_DECUMA_INT16+1); /* The y-values are measure in a system with the y-axis pointing downwards */
		dx[i] = pArc->dx[i];
		dy[i] = (DECUMA_INT16)-maxNUMB(pArc->dy[i],MIN_DECUMA_INT16+1); /* change derivative sign too */
	}
}


static void flip_arc(const SCR_ARC *pArc, DECUMA_INT16 *x, DECUMA_INT16 *y, DECUMA_INT16 *dx, DECUMA_INT16 *dy, int n)
/* Copy and flip pArc and change y and dy sign, so that the y-axis */
/* point upwards. Flip means that the time order is reversed.      */
/* In case of NULL-pointer arguments nothing is done.			   */
{
	int i;

	if( !(pArc && x && y && dx && dy) )
	{
		decumaAssert(0);  /* Cannot produce output. Very serious. */
		return;
	}

	if( !(pArc->x && pArc->y && pArc->dx && pArc->dy) )
	{
		decumaAssert(0);  /* Cannot produce output. Very serious. */
		return;
	}

	x[n-1]  = pArc->x[0];
	y[n-1]  = (DECUMA_INT16)-maxNUMB(pArc->y[0],MIN_DECUMA_INT16+1);  /* The y-values are measure in a system with the y-axis pointing downwards */
	dx[n-1] = (DECUMA_INT16)-maxNUMB(pArc->dx[0],MIN_DECUMA_INT16+1); /* Flip derivative sign too. Last derivative is copied. */
	dy[n-1] = pArc->dy[0];								/* Double sign change. Last derivative is copied. */
	for( i = 1; i < n; i++)
	{
		x[n-1-i]  = pArc->x[i];
		y[n-1-i]  = (DECUMA_INT16)-maxNUMB(pArc->y[i],MIN_DECUMA_INT16+1);
		dx[n-1-i] = (DECUMA_INT16)-maxNUMB(pArc->dx[i-1],MIN_DECUMA_INT16+1);
		dy[n-1-i] = pArc->dy[i-1];
	}
}

static int go_left(
	int i,
	const DECUMA_INT16 *x, const DECUMA_INT16 *y, const DECUMA_INT16 *dx, const DECUMA_INT16 *dy,
	DECUMA_INT16 xlimlow, DECUMA_INT16 xlimhigh, DECUMA_INT16 ylimlow,
	DECUMA_INT16 ylimhigh, DECUMA_BOOL handleTwitch)
/* Go to rightturn. See go_horisontal. */
{
	return go_horisontal(i, x, y, dx, dy,
				   xlimlow, xlimhigh, ylimlow,
				   ylimhigh, handleTwitch,-1);
}

static int go_right(
	int i,
	const DECUMA_INT16 *x, const DECUMA_INT16 *y, const DECUMA_INT16 *dx, const DECUMA_INT16 *dy,
	DECUMA_INT16 xlimlow, DECUMA_INT16 xlimhigh, DECUMA_INT16 ylimlow,
	DECUMA_INT16 ylimhigh, DECUMA_BOOL handleTwitch)
/* Go to leftturn. See go_horisontal. */
{
	return go_horisontal(i, x, y, dx, dy,
				   xlimlow, xlimhigh, ylimlow,
				   ylimhigh, handleTwitch,1);
}

static int go_up(
	int i,
	const DECUMA_INT16 *x, const DECUMA_INT16 *y, const DECUMA_INT16 *dx, const DECUMA_INT16 *dy,
	DECUMA_INT16 xlimlow, DECUMA_INT16 xlimhigh, DECUMA_INT16 ylimlow,
	DECUMA_INT16 ylimhigh, DECUMA_BOOL handleTwitch)
/* Go to downturn. See go_horisontal. */
{
	/* Switch dx and dy and call go_right. */
	return go_right(i, x, y, dy, dx,
				   xlimlow, xlimhigh, ylimlow,
				   ylimhigh, handleTwitch);
}

static int go_down(
	int i,
	const DECUMA_INT16 *x, const DECUMA_INT16 *y, const DECUMA_INT16 *dx, const DECUMA_INT16 *dy,
	DECUMA_INT16 xlimlow, DECUMA_INT16 xlimhigh, DECUMA_INT16 ylimlow,
	DECUMA_INT16 ylimhigh, DECUMA_BOOL handleTwitch)
/* Go to upturn. See go_horisontal. */
{
	/* Switch dx and dy and call go_left. */
	return go_left(i, x, y, dy, dx,
				   xlimlow, xlimhigh, ylimlow,
				   ylimhigh, handleTwitch);
}

static int go_horisontal(
	int i,
	const DECUMA_INT16 *x, const DECUMA_INT16 *y, const DECUMA_INT16 *dx, const DECUMA_INT16 *dy,
	DECUMA_INT16 xlimlow, DECUMA_INT16 xlimhigh, DECUMA_INT16 ylimlow,
	DECUMA_INT16 ylimhigh, DECUMA_BOOL handleTwitch, int dir)
/* Takes an arc index and travels in specified direction    */
/* until the arc turn to the opposite direction.			*/
/* dir = 1  -> go right										*/
/* dir = -1 -> go left										*/
/* new i is returned at horisontal turn (dx shift sign)		*/
/* If handleTwitch is TRUE short twitches will not be		*/
/* consider a local optimum (turning point).				*/
/* In case of NULL-pointer arguments i is returned back		*/
/* (with one exception). The returnvalue is always less		*/
/* than NUMBER_OF_POINTS_IN_ARC and at least 0.				*/
{
	DECUMA_BOOL keep_going;
	DECUMA_BOOL untilbreak = TRUE;

	if( !(x && y && dx && dy) )
	{
		untilbreak = FALSE;
		decumaAssert(0); /* NULL-pointers supported but might be used unintended. */
						 /* Avoid this in debug mode for now. */
	}

	if( i < 0 )
	{
		decumaAssert(0); /* Bad index parameters supported but might be used unintended. */
						 /* Avoid this in debug mode for now. */
		return 0;
	}

	/* Check x derivative for sign change up to second last point */
	while( untilbreak )
	{
		keep_going = i < NUMBER_OF_POINTS_IN_ARC - 1;
		if( !keep_going ) break;

		keep_going = !reachedLim(x[i],y[i],xlimlow,xlimhigh,ylimlow,ylimhigh);
		if( !keep_going ) break;

		keep_going = dir*dx[i] >= 0; /* Shifted sign? 0 is not considerd signshift */
		if( !keep_going && handleTwitch ) /* Maybe it was just a twitch. Check this? */
		{
			keep_going = (i > 0) && (i < NUMBER_OF_POINTS_IN_ARC - 2);
			if( !keep_going ) break;

			/* Back to original direction after short (hardcoded) twitch? */
			keep_going = sameSign(dx[i-1],dx[i+1]) &&
						 !sameSign(dx[i-1],dx[i]) &&    /* Redundant check? Better safe than... */
						 sameSignVec(&dy[i-1],3) &&
						 sign(dx[i-1]); /* Are we really moving? */

			if( i < NUMBER_OF_POINTS_IN_ARC - 3 ) /* Should there be a long twitch option for this test?? */
			{
				/* Back to original direction after long (hardcoded) twitch? */
				keep_going |= sameSign(dx[i-1],dx[i+2]) &&
							  !sameSign(dx[i-1],dx[i]) &&    /* Redundant check? Better safe than... */
							  sameSignVec(&dy[i-1],4) &&
							  sign(dx[i-1]); /* Are we really moving? */
			}
		}

		if( !keep_going ) break;

		i++;
	}

	return minNUMB(i,NUMBER_OF_POINTS_IN_ARC - 1);
}

static double integrateArc(const DECUMA_INT16 *x, const DECUMA_INT16 *y, int si, int ei, DECUMA_BOOL rightHandedSystem, DECUMA_BOOL *error)
/* Integrates the arc in (x[],y[]) from index si to ei along the line  */
/* through si and ei. The boolean rightHandedSystem determines the     */
/* sign of the integral area. If rightHandedSystem is true the area to */
/* the left of the line through s and e, standing at s looking at e,   */
/* is considered positive. If failure due to bad arguments error is    */
/* set to TRUE else it is set to FALSE.								   */

/* Example of a positive integral when */
/* rightHandedSystem is TRUE.		   */
/*			   /					   */
/*	    	e / 					   */
/*        ___o__  					   */
/*      _/xx/						   */
/*     /xxx/						   */
/*    |xxx/							   */
/*    /xx/							   */
/*   |xx/							   */
/*   |x/							   */
/*   |/								   */
/*   o s							   */
/*	/|								   */
/* / |								   */
/* The x's indicate the area returned  */
/* by integrateArc.	                   */

{
	double area = 0.0; /* Return 0 if case of failure */

	double baseX, baseY, vecX, vecY;
	double projX, projY, ortX, ortY, third_cross_comp;
	double baseLen, projLen, ortLen, prevProjLen, prevOrtLen;
	double scalP, dB, dH, dA, area_sign;
	int i;

	*error = TRUE;  /* Have not passed initial tests */
					/* Never ever crash the SCR in zoom */
					/* by potential memory errors etc. */

	if( !(x && y) )
	{
		decumaAssert(0); /* NULL-pointers supported but might be used unintended. */
						 /* Avoid this in debug mode for now. */
		return area;
	}

	if( si < 0 || ei >= NUMBER_OF_POINTS_IN_ARC )
	{
		decumaAssert(0); /* Bad index parameters supported but might be used unintended. */
						 /* Avoid this in debug mode for now. */
		return area;
	}

	/* Put base arc start in origo */
	baseX = (DECUMA_INT32)x[ei]-x[si];
	baseY = (DECUMA_INT32)y[ei]-y[si];

	if( (baseX == 0) && (baseY == 0) ) /* Would give div by zero */
	{
		return area;
	}

	*error = FALSE; /* Have passed initial tests */

	/* NOTE: decumaSqrt uses integers. */
	baseLen = decumaSqrt( (DECUMA_UINT32) (baseX * baseX + baseY * baseY) );

	prevProjLen = 0.0;
	prevOrtLen = 0.0;

	for( i = si + 1; i <= ei; i++ )
	{
		/* Put arc start in origo */
		vecX = x[i] - x[si];
		vecY = y[i] - y[si];

		scalP = (vecX * baseX + vecY * baseY);

		projLen = scalP / baseLen; /* Might be negative */

		 /* Outside integration area or projection is zero */
		if( projLen <= 0  || projLen > baseLen )
		{
			projLen = prevProjLen;
			ortLen = prevOrtLen;
			dA = 0;
		}
		else
		{
			projX = baseX * projLen / baseLen;
			projY = baseY * projLen / baseLen;

			ortX = vecX - projX;
			ortY = vecY - projY;

			/* Third component of cross product */
			third_cross_comp = (projX * ortY - projY * ortX);

			ortLen = third_cross_comp / projLen; /* Might be negative */

			area_sign = sign_double( ortLen + prevOrtLen );

			dB = decumaAbs(projLen - prevProjLen);		/* Along 1st base vector */
			dH = decumaAbs(ortLen + prevOrtLen) / 2;	/* Mean along 2nd base vector */

			if ( !rightHandedSystem ) /* leftHandedSystem */
			{
				area_sign = -area_sign; /* 2nd base vector has switched sign */
			}

			dA = area_sign * dH * dB;
		}

		area = area + dA;

		prevProjLen = projLen;
		prevOrtLen = ortLen;
	}

	return area;
}

static double triangleArea(const DECUMA_INT16 *x, const DECUMA_INT16 *y, int i1, int i2, int i3, DECUMA_BOOL *error)
/* Calculates the area of the triangle constituted by the points (indices) */
/* i1, i2 and i3. In case of bad indices error is set to TRUE.			*/
{
	double area = 0.0;

	*error = TRUE;  /* Have not passed initial tests */
					/* Never ever crash the SCR in zoom */
					/* by potential memory errors etc. */

	if( !(x && y) )
	{
		decumaAssert(0); /* NULL-pointers supported but might be used unintended. */
						 /* Avoid this in debug mode for now. */
		return area;
	}

	if( i1 < 0 || i2 < 0 || i3 < 0 )
	{
		decumaAssert(0); /* Bad index parameters supported but might be used unintended. */
						 /* Avoid this in debug mode for now. */
		return area;
	}

	if( i1 >= NUMBER_OF_POINTS_IN_ARC || i2 >= NUMBER_OF_POINTS_IN_ARC
		|| i3 >= NUMBER_OF_POINTS_IN_ARC )
	{
		decumaAssert(0); /* Bad index parameters supported but might be used unintended. */
						 /* Avoid this in debug mode for now. */
		return area;
	}

	*error = FALSE; /* Have passed initial tests */

	area = 0.5 * decumaAbs( (DECUMA_INT32)x[i1]*y[i2] + (DECUMA_INT32)x[i2]*y[i3] + (DECUMA_INT32)x[i3]*y[i1] -
							(DECUMA_INT32)x[i2]*y[i1] - (DECUMA_INT32)x[i3]*y[i2] - (DECUMA_INT32)x[i1]*y[i3] );

	return area;
}

/**
 * Calculate sin for an angle formed by three points.
 * @returns SIN_ANGLE_SCALE * sin(ABC), where in the angle ABC A = (x[a], y[a]) etc.
 */
static int sinAngle(const DECUMA_INT16 *x, const DECUMA_INT16 *y, int a, int b, int c)
{
	DECUMA_INT32 BAx, BAy, BCx, BCy;
	DECUMA_INT32 BAlen, BClen;
	DECUMA_INT32 sin_angle;

	if( a < 0 || b < 0 || b < 0 ) {
		decumaAssert(0); /* Bad index parameters supported but might be used unintended. */
						 /* Avoid this in debug mode for now. */
		return 0;
	}

	if( a >= NUMBER_OF_POINTS_IN_ARC ||
		b >= NUMBER_OF_POINTS_IN_ARC ||
		c >= NUMBER_OF_POINTS_IN_ARC )
	{
		decumaAssert(0); /* Bad index parameters supported but might be used unintended. */
						 /* Avoid this in debug mode for now. */
		return 0;
	}

	BAx = (DECUMA_INT32)x[a] - x[b];
	BAy = (DECUMA_INT32)y[a] - y[b];
	BCx = (DECUMA_INT32)x[c] - x[b];
	BCy = (DECUMA_INT32)y[c] - y[b];

	BAlen = decumaSqrt (BAx * BAx + BAy * BAy);
	BClen = decumaSqrt (BCx * BCx + BCy * BCy);

	sin_angle  = SIN_ANGLE_SCALE * (BAx * BCy - BAy * BCx);
	sin_angle /= (BAlen * BClen);

	/* Assure that we can have this when int is 16-bit */
	decumaAssert(sin_angle <= MAX_DECUMA_INT16);
	decumaAssert(sin_angle >= MIN_DECUMA_INT16);
	return (int)sin_angle;
}

static DECUMA_INT32 findWidestHorisontal(const DECUMA_INT16 *x, const DECUMA_INT16 *y,
								  int startInd, int stopInd, int centerInd)
/* Time consuming N^2 function, but it gives the			  */
/* discriminating information needed. Returns the widest      */
/* distance in the horisontal direction between two points in */
/* [(x(startInd),y(startInd)),...,(x(stopInd),y(stopInd))].   */
/* In case of bad arguments 0 is returned.					  */
/* To make the function more efficient (N^2/4) a centerInd is */
/* required. In case of bad input 0 is returned.			  */
{
	int i, j;
	DECUMA_INT32 x_dist, y_dist, max_x_dist, min_y_dist;
	int min_y_dist_ind;

	max_x_dist = 0;

	if( !(x && y) )
	{
		decumaAssert(0); /* NULL-pointers supported but might be used unintended. */
						 /* Avoid this in debug mode for now. */
		return max_x_dist;
	}

	if( (startInd < 0) || (stopInd >= NUMBER_OF_POINTS_IN_ARC) )
	{
		decumaAssert(0); /* Bad index parameters supported but might be used unintended. */
						 /* Avoid this in debug mode for now. */
		return max_x_dist;
	}
	if( startInd > stopInd )
	{
		return max_x_dist;
	}

	if( stopInd <= centerInd || startInd >= centerInd ) /* What?! No bottom?! */
	{
		return max_x_dist;
	}

	for( i = startInd; i < centerInd; i++)
	{
		/* Find the point on other side of bottom with the smallest */
		/* y distance to point i.									*/
		min_y_dist = MAX_DECUMA_INT32;
		min_y_dist_ind = 0; /* Initialize to something to avoid compiler warning */
		for( j = centerInd + 1; j <= stopInd; j++ )
		{
			y_dist = decumaAbs( (DECUMA_INT32)y[i] - y[j] );
			if( y_dist < min_y_dist )
			{
				min_y_dist = y_dist;
				min_y_dist_ind = j;
			}
		}
		/* Measure the x distance to this point */
		x_dist = decumaAbs( (DECUMA_INT32)x[i] - x[min_y_dist_ind] );
		if( x_dist > max_x_dist )
		{
			max_x_dist = x_dist;
		}
	}
	return max_x_dist;
}

static DECUMA_BOOL peak(const DECUMA_INT16 *dx, const DECUMA_INT16 *dy, int i, unsigned int thresh)
/* Return TRUE if point i is a peak according    */
/* to thresh. A peak is a sharp curvature point. */
/* Thresh should be in the intervall 0 to 100    */
/* If dx or dy is NULL-pointers or i is out of   */
/* bounds FALSE is returned.                     */
{
	DECUMA_INT32 ddx, ddy;
	DECUMA_UINT32 s, ds, s1_2, s2_2, ds_2;
	DECUMA_UINT32 scaleFactor = 4096; /* 2^12 */
	DECUMA_UINT32 maxScalableValue = 262144; /* 2^18 */
	DECUMA_UINT32 scaleFactorSqrt = 64; /* 2^(12/2) */

	if( !(dx && dy) )
	{
		decumaAssert(0); /* NULL-pointers supported but might be used unintended. */
						 /* Avoid this in debug mode for now. */
		return FALSE;
	}

	if( (i < 0) || (i >= NUMBER_OF_POINTS_IN_ARC) )
	{
		decumaAssert(0); /* Bad index parameters supported but might be used unintended. */
						 /* Avoid this in debug mode for now. */
		return FALSE;
	}

	if( (i == 0) || (i == NUMBER_OF_POINTS_IN_ARC - 1) )
	{
		return FALSE; /* First or last point cannot be a peak. */
					  /* No left and right derivative resp. exist for these points. */
	}

	ddx = (DECUMA_INT32)dx[i] - (DECUMA_INT32)dx[i-1];
	ddy = (DECUMA_INT32)dy[i] - (DECUMA_INT32)dy[i-1];

	s1_2 = ((DECUMA_INT32)dx[i-1] * (DECUMA_INT32)dx[i-1] + (DECUMA_INT32)dy[i-1] * (DECUMA_INT32)dy[i-1]);
	s2_2 = ((DECUMA_INT32)dx[i] * (DECUMA_INT32)dx[i] + (DECUMA_INT32)dy[i] * (DECUMA_INT32)dy[i]);
	ds_2 = ((DECUMA_INT32)ddx * (DECUMA_INT32)ddx + (DECUMA_INT32)ddy * (DECUMA_INT32)ddy);

	while( (s1_2 > maxScalableValue) ||
		   (s2_2 > maxScalableValue) ||
		   (ds_2 > maxScalableValue) )
	{
		scaleFactor /= 4;
		scaleFactorSqrt /= 2;
		maxScalableValue *= 4;
		if( scaleFactorSqrt == 0 )
		{
			return FALSE; /* Somethings wrong. No peak found. */
		}
	}

	s = decumaSqrt( scaleFactor * s1_2 ) + decumaSqrt( scaleFactor * s2_2 );

	ds = decumaSqrt( scaleFactor * ds_2 );

	if( s == 0 ) /* Everythings zero? */
	{
		return FALSE; /* No peak. */
	}

	return ( scaleFactorSqrt * ds / s ) > thresh;
}

static DECUMA_BOOL reachedLim(DECUMA_INT16 x, DECUMA_INT16 y,
		DECUMA_INT16 xlimlow, DECUMA_INT16 xlimhigh, DECUMA_INT16 ylimlow, DECUMA_INT16 ylimhigh)
/* Return TRUE if (x,y) is outside the specified limits. */
{
	DECUMA_BOOL insideBound = TRUE;

	insideBound &= x > xlimlow;
	insideBound &= x < xlimhigh;
	insideBound &= y > ylimlow;
	insideBound &= y < ylimhigh;

	return !insideBound;
}

static DECUMA_BOOL sameSignVec(const DECUMA_INT16 *n, int nLen)
/* Returns TRUE if all elements of n have the same sign.*/
/* 0 is considered neither positive nor negative, it is */
/* a "sign" of its own. If n is a NULL-pointer FALSE is */
/* If nLen is 0 TRUE is returned.						*/
{
	int i;
	DECUMA_BOOL same = TRUE;

	if( !n )
	{
		decumaAssert(0); /* NULL-pointers supported but might be used unintended. */
						 /* Avoid this in debug mode for now. */
		return !same;
	}

	for( i = 1; i <  nLen; i++ )
	{
		same &= sign(n[i]) == sign(n[i-1]);
		if( !same ) break;
	}

	return same;
}

static DECUMA_BOOL sameSign(DECUMA_INT16 n1, DECUMA_INT16 n2)
/* Returns TRUE if n1 and n2 have the same sign.        */
/* 0 is considered neither positive nor negative, it is */
/* a "sign" of its own.                                 */
{
	return sign(n1) == sign(n2);
}

static double sign_double(double n)
{
	if( n < 0 )
	{
		return -1.0;
	}
	if( n > 0 )
	{
		return 1.0;
	}

	return 0.0; /* n == 0 */
}


static DECUMA_INT16 sign(DECUMA_INT16 n)
{
	if( n < 0 )
	{
		return -1;
	}
	if( n > 0 )
	{
		return 1;
	}

	return 0; /* n == 0 */
}

#undef SIN_ANGLE_SCALE
#undef SVM_CURVE_TRAINING_SIZE
#undef SVM_ROUNDING_FACTOR
#undef SVM_ZOOM_LIMIT
#undef EPS

/************************************************** End of file ***/
