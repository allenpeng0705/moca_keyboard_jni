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


#include "decumaSimTransf.h"
#include "decumaMath.h"
#include "decumaAssert.h"
#include "decumaMemory.h"
#include "globalDefs.h"

/* This function is implemented as a macro in decumaMath.h
void simtransfCopy(SIM_TRANSF *pTo, SIM_TRANSF *pFrom)
{
	pTo->theta = pFrom->theta;
	pTo->scale = pFrom->scale;
	pTo->delta.x = pFrom->delta.x;
	pTo->delta.y = pFrom->delta.y;
	pTo->symPoint.x = pFrom->symPoint.x;
	pTo->symPoint.y = pFrom->symPoint.y;
}*/

void simtransfScale(SIM_TRANSF *pTransf, int scale_num, int scale_denom)
{
	/*Scale all parameters of the similarity transformation even */
	/*pFrom->symPoint. */
	/*It is NOT assumed that 'scale_num' and 'scale_denom' are scaled by */
	/*'SIM_TRANSF_SCALE_FACTOR'. */

	pTransf->scale = ( pTransf->scale * scale_num ) / scale_denom;
	pTransf->delta.x *= scale_num/scale_denom;
	pTransf->delta.y *= scale_num/scale_denom;
}

void simtransfComposedWithInverse(INT32_POINT *pTo, const INT32_POINT *pFrom,
	const SIM_TRANSF *pTransf2,const SIM_TRANSF *pTransf1)
{
	/*Computes pTo = pTransf2 ( inv(pTransf1) ( pFrom)) */
	/*It is assumed that all elements except theta and scale in SIM_TRANSF are */
	/*scaled by 'SIM_TRANSF_ROUNDING_FACTOR'. Note that */
	/*It is assumed that scale in SIM_TRANSF is */
	/*scaled by 'SIM_TRANSF_SCALE_FACTOR'. Note that */
	/*pTransf1->symPoint and pTransf2->symPoint are considered as being the origin. */
	/*We allow pTo == pFrom */

	DECUMA_INT32 theta = pTransf2->theta - pTransf1->theta;
	const float cosScaled = ((float)decumaCos(theta)) * pTransf2->scale / pTransf1->scale;
	const float sinScaled = ((float)decumaSin(theta)) * pTransf2->scale / pTransf1->scale;
	float x = ((float)(pFrom->x * SIM_TRANSF_ROUNDING_FACTOR - pTransf1->delta.x));
	float y = ((float)(pFrom->y * SIM_TRANSF_ROUNDING_FACTOR - pTransf1->delta.y));

	pTo->x = (DECUMA_INT32)(((cosScaled * x + sinScaled * y) / scaleFactorForSin + pTransf2->delta.x)
		/ SIM_TRANSF_ROUNDING_FACTOR);
	pTo->y = (DECUMA_INT32)(((-sinScaled * x + cosScaled * y) / scaleFactorForSin + pTransf2->delta.y)
		/ SIM_TRANSF_ROUNDING_FACTOR);
}


void simtransfPoint(INT32_POINT *pTo, const INT32_POINT *pFrom, const SIM_TRANSF *pTransf)
{
	/*Transforms 'pFrom' to 'pTo' by the similarity transformation */
	/*'pTransf'. Rotation is performed around pTransf->symPoint. */
	/*It is assumed that all elements except theta and scale in pTransf are */
	/*scaled by 'SIM_TRANSF_ROUNDING_FACTOR'. Note that */
	/*It is assumed that scale in pTransf is */
	/*scaled by 'SIM_TRANSF_SCALE_FACTOR'. */
	/*It is also assumed that pFrom is scaled by 'SIM_TRANSF_ROUNDING_FACTOR' */
	/*pTo will also be scaled by this factor */
	/*We allow pTo = pFrom. */

	DECUMA_INT32 theta = pTransf->theta;
	float cosScaled = ((float)decumaCos(theta)) * pTransf->scale / SIM_TRANSF_SCALE_FACTOR;
	float sinScaled = ((float)decumaSin(theta)) * pTransf->scale / SIM_TRANSF_SCALE_FACTOR;

	DECUMA_INT32 x = pFrom->x - pTransf->symPoint.x;
	DECUMA_INT32 y = pFrom->y - pTransf->symPoint.y;
	pTo->x = (DECUMA_INT32)(cosScaled * x + sinScaled * y ) / scaleFactorForSin + pTransf->delta.x;
	pTo->y = (DECUMA_INT32)(-sinScaled * x + cosScaled * y) / scaleFactorForSin + pTransf->delta.y;
}


void inverseSimTransf(SIM_TRANSF *pInvTransf, const SIM_TRANSF *pTransf)
{
	decumaAssert(pTransf->scale!=0);
	if (pTransf->scale==0)
	{
		pInvTransf->scale=MAX_SIM_TRANSF_SCALE;
	}
	else
	{
		pInvTransf->scale = ((DECUMA_UINT32) SIM_TRANSF_SCALE_FACTOR * (DECUMA_UINT32) SIM_TRANSF_SCALE_FACTOR) /pTransf->scale; /*DIVISION */
		pInvTransf->scale = maxNUMB(pInvTransf->scale, MIN_SIM_TRANSF_SCALE);
	}
	pInvTransf->theta = -pTransf->theta;
	pInvTransf->symPoint = pTransf->delta;
	pInvTransf->delta = pTransf->symPoint;
}

void simtransfInit(SIM_TRANSF * pTransf)
{
	decumaAssert( pTransf );
	myMemSet(pTransf, 0, sizeof(SIM_TRANSF));
	pTransf->scale = 1;
}

int simtransfAlmostEqual(SIM_TRANSF * pTransf1, SIM_TRANSF * pTransf2)
{
	return (
		decumaAbs(pTransf1->delta.x - pTransf2->delta.x)<=1 &&
		decumaAbs(pTransf1->delta.y - pTransf2->delta.y)<=1 &&
		decumaAbs(pTransf1->symPoint.x - pTransf2->symPoint.x)<=1 &&
		decumaAbs(pTransf1->symPoint.y - pTransf2->symPoint.y)<=1 &&
		decumaAbs(pTransf1->theta - pTransf2->theta)<=1 &&
		decumaAbs(pTransf1->scale - pTransf2->scale)<=1 ); /*TODO: Casting error */
}

int simtransfEqual(SIM_TRANSF * pTransf1, SIM_TRANSF * pTransf2)
{
	return (memcmp(pTransf1,pTransf2,sizeof(SIM_TRANSF))==0);
}

#if DEBUG_DUMP_SCR_DATA
/* Writes the data in the struct SIM_TRANSF to a file. */
void simTransfDump(FILE * pf, SIM_TRANSF * pT)
{
	fprintf(pf, "Sim transf(s t)= %d %d \n", pT->scale, pT->theta);
	fprintf(pf, "Sim transf(d s)= %d %d %d %d\n", pT->delta.x, pT->delta.y,
		pT->symPoint.x, pT->symPoint.y);
} /*simTransfDump() */
#endif /* DEBUG_DUMP_SCR_DATA */

