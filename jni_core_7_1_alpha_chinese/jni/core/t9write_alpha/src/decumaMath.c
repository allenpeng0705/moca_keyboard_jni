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


/* File:  decumaMath.c */

#include "decumaConfig.h"

/*#define _INCLUDE_MATH_LIB_bkjsdofidshfdrtkl */
#ifdef _INCLUDE_MATH_LIB_bkjsdofidshfdrtkl
#include<math.h>
#endif

#include "decumaMath.h"
#include "decumaAssert.h"
#include "globalDefs.h"

#undef DO_LOGGING
#ifdef DO_LOGGING
#include "t9write_log.h"
#endif


#ifndef ONPALM_ARMLET
static
#endif
const char VectorForComputingArctan []= {
	0,3,6,9,12,15,19,22,24,27,30,33,36,39,41,44,46,49,51,54,56,58,60,62,64,66,68,70,72,74,75,77,79};

/*INT16 VectorForComputingNorm [] = {0,1,3,7,13,20,28,38,50,63,77,93,109,127,146,166,187,209,232,256,281,306,332,359,386,414}; */

/*INT16 VectorForComputingNorm [] = {0,0,1,2,3,5,7,10,13,16,20,24,28,33,38,44,50,56,63,70,77,85,93,101,109,118,127,136,146,156,166,177,187,198,209,221,232,244,256,268,281,293,306,319,332,345,359,372,386,400,414}; */

/*  */
/* vma This printout was generated with angletest_vma test program in the generic/tests folder. */
/*
angle( 3, 0 ) = 0
angle( 1, 1 ) = -79
angle( 0, 3 ) = -157
angle( -1, 1 ) = -235
angle( -3, 0 ) = -314
angle( -1, -1 ) = 235
angle( 0, -3 ) = 157
angle( 1, -1 ) = 79
*/

DECUMA_UINT32 decumaSqrt(DECUMA_UINT32 x)
{
	/*This is taken from the internet. */
	/*Comments by JM */

	/*The function goes through the pairs of bits in x starting */
	/*with the most significant pair and then going down */
	/*in significance. For every pair a bit in the variable "a" will */
	/*be set if the pair is big enough. */

	DECUMA_UINT32 j;
	DECUMA_UINT32 a= 0, c = 0;

	decumaAssert(x<(DECUMA_UINT32) 0xffffffff);
	for(j = 0; j < 16; j++)
	{
		DECUMA_UINT32 b;

		/*c will make up a kind of "remainder"  */
		c <<= 2;
		c += x >> 30; /*Just interested in the two most significant bits. */
		a <<= 1; /*a will be shifted once, when x is shifted twice */
		b = (a << 1) | 1;
		if(c >= b)
		{
			c -= b;
			a++;
		}
		x <<= 2;
		x &= 0xffffffff;
	}
	return a;
}


int mostSignificantBit(DECUMA_UINT32 x)
{
	/*
	Let x >=0. The function computes the smallest n>=0 such that x <= 2^(n-1)
	or equivalently the smallest n>=0 such that x >> n = 0 (shift operator).

	This implementation is inspired by binary search in order to speed up
	search for large numbers.
	*/

	unsigned int pos = 0;
	unsigned int n = sizeof(DECUMA_INT32) << 2;
	DECUMA_UINT32 tmp;

	while(x)
	{
		tmp = x >> n;
		if ( tmp )
		{
			x = tmp;
			pos += n;
		}
		n >>= 1;
		x >>= 1;
		pos++;
	}
	return pos;
}

DECUMA_INT32 newNorm(DECUMA_INT32 a, DECUMA_INT32 b)
{
	const int n = 6;
	const char v[] = {64, 64, 65, 65, 66, 66, 67, 67, 68, 68, 69, 69, 70, 70, 71, 71, 72, 72, 72, 73, 73, 74, 74, 75, 75, 75, 76, 76, 77, 77, 78, 78, 78, 79, 79, 80, 80, 80, 81, 81, 82, 82, 82, 83, 83, 84, 84, 84, 85, 85, 85, 86, 86, 87, 87, 87, 88, 88, 88, 89, 89, 89, 90, 90};

	if(a < 0) a = -a;
	if(b < 0) b = -b;

	decumaAssert( (a < MAX_INT32_SQRT) && (b < MAX_INT32_SQRT));

	if(a == 0)
		return b;

	if(b == 0)
		return a;

	if(a == b)
		return (a*91) >> n;

	if(a > b)
	{
		DECUMA_UINT32 x = (b <<
			( n >> 1)) / a;
		x *= x;
		return (a*a + b*b)/((a*v[x]) >> n);
	}
	else
	{
		DECUMA_UINT32 x = (a <<
			( n >> 1)) / b;
		x *= x;
		return (a*a + b*b)/((b*v[x]) >> n);
	}
}

DECUMA_INT32 normFine(DECUMA_INT32 a, DECUMA_INT32 b)
{
	/*
	This function is checked against overflow and the overflow is handled
	to give reasonable output.
	*/

	DECUMA_UINT32 c, x;
	int m;

	if(a < 0) a = -a;
	if(b < 0) b = -b;

	if(a == 0)
		return b;

	if(b == 0)
		return a;

	m = mostSignificantBit(maxNUMB(a,b)) - mostSignificantBit(MAX_INT32_SQRT) + 1;

	if(m>0)
	{
		a >>= m;
		b >>= m;
	}

	c = a*a + b*b;
	x = newNorm(a,b);
	x = (x + c/x) >> 1;

#ifdef DO_LOGGING
	logVariable32u("c",c);
	logVariable32u("x",x);
	logVariable32("m",m);
#endif
	if(m>0)
		return (x + c/x) << (m - 1);
	return (x + c/x) >> 1;
}

/* This function converts x to the closest */
/* integer ( a normal cast only truncates the float). */
DECUMA_INT32 decumaRound( float x )
{
	if ( x > 0 )
	{
		decumaAssert( x < x + 0.5f );
		x += 0.5f;
		decumaAssert( x < MAX_DECUMA_INT32);
		return (DECUMA_INT32) x;
	}
	else
	{
		decumaAssert( x > x - 0.5f );
		x -= 0.5f;
		decumaAssert( x > MIN_DECUMA_INT32 );
		return (DECUMA_INT32) x;
	}
}

DECUMA_INT32 decumaLog2(DECUMA_INT32 x)
{
	int j = 0;
	int val = 1;
	x = decumaAbs(x);
	while(val <= x)
	{
		val *= 2;
		j++;
	}
	return j-1;
}


int angle(DECUMA_INT32 a, DECUMA_INT32 b)
{
	/* (a,b) is a point in the plane. angle computes the */
	/* angle of the point vector measured from the horizontal axis. */
	/* The output range from -314 to 314 in integers. (100*Pi). */
	/*Indata is multiplied by 25 which gives an upper limit for input. */

#ifdef _INCLUDE_MATH_LIB_bkjsdofidshfdrtkl
	double res;
	if(b == 0 && a == 0 )
		return 0;

	res = -atan2((double)b,(double)a)*100;
	if(res > 0)
		return (int) (res + .5);
	else
		return (int) (res - .5);
#else
	int scale = 64;
	DECUMA_INT32 temp1,temp2;

	if(b == 0 && a == 0 )
		return 0;

	if(decumaAbs(a) >= decumaAbs(b))
	{
		temp1 = decumaAbs(scale*b/a);
		temp2 = temp1/2;
		if(temp2*2 != temp1) /*Rounding has occured */
		{
			/*Interpolate */
			temp1 = (VectorForComputingArctan[temp2] + VectorForComputingArctan[temp2+1] + 1)/2;
		}
		else
		{
			temp1 = VectorForComputingArctan[temp2];
		}
	}
	else
	{
		temp1 = decumaAbs((scale*a)/b);
		temp2 = temp1/2;
		if(temp2*2 != temp1) /*Rounding has occured */
		{
			/*Interpolate */
			temp1 = PI/2 - (VectorForComputingArctan[temp2] + VectorForComputingArctan[temp2+1] + 1)/2;
		}
		else
		{
			temp1 = PI/2 - VectorForComputingArctan[temp2];
		}
	}

	if((b < 0) && (a > 0))
 		return (int) temp1;
	if((b < 0) && (a <= 0))
		return (int) (PI - temp1);
	if((b >= 0) && (a < 0))
		return (int) (temp1 - PI);

	return (int) -temp1;
#endif /* _INCLUDE_MATH_LIB_bkjsdofidshfdrtkl */
}

DECUMA_INT32 norm(DECUMA_INT32 a, DECUMA_INT32 b)
{
	/* Computes an integer approximation to the norm of the vector (a,b). */
	/* In order to increase accuracy the VectorForComputingNorm has */
	/* to be longer. */

	if(a < 0) a = -a;
	if(b < 0) b = -b;

    if(a == 0)
		return b;

    if(b == 0)
		return a;

	if(a > b)
	{
		/*Linear approximation. It should really */
		/*be a + (b*b)/(2*a). The term a is for rounding */
		/*to closest integer. */
		return a + (b*b + a)/(a << 1);
	}
	else
	{
		return b + (a*a + b)/(b << 1); /*Same here */
	}
}

DECUMA_INT32 invNorm(DECUMA_INT32 a, DECUMA_INT32 b)
{
	/*Computes the length of remaining kateter given the */
	/*hypotenusa 'a' and kateter 'b'. Note that abs(a) > abs(b). */

	if(a < 0) a = -a;
	if(b < 0) b = -b;

	if(b == 0)
		return a;

	/*Linear approximation. It should really */
	/*be a + (b*b)/(2*a). The term a is for rounding */
	/*to closest integer. */
	return a - (b*b - a)/(a << 1);
}

int mod_sym(int a, int b)
{
	/*Finds c such that a + b*c/2 is in the interval [-b/2,b/2] */
	/*It is assumed that b>=0. */
	int c = b >> 1; /*divide by 2 */

	if(b == 0)
		return 0;

	a %= b;

	if(a>c)
		return a - b;

	if(a <= -c)
		return a + b;

	return a;
}

int indexToMin(const DECUMA_INT16* p, int nElements)
{
	/*Returns index to the minimum element in */
	/*the list p, with nElements elements. */
	DECUMA_INT16 min = *(p++);
	int index = 0,j;
	for(j = 1; j < nElements; j++)
	{
		if(*p < min)
		{
			min = *p;
			index = j;
		}
		p++;
	}
	return index;
}

int lastIndexToMin(const DECUMA_INT16* p, int nElements)
{
	/* Returns last index to the minimum element in */
	/* the list p, with nElements elements. */
	DECUMA_INT16 min = *(p++);
	int index = 0,j;
	for(j = 1; j < nElements; j++)
	{
		if( *p <= min)
		{
			min = *p;
			index = j;
		}
		p++;
	}
	return index;
}

int indexToMax(const DECUMA_INT16* p, int nElements)
{
	/*Returns index to the minimum element in */
	/*the list p, with nElements elements. */
	DECUMA_INT16 max = *(p++);
	int index = 0,j;
	for(j = 1; j < nElements; j++)
	{
		if(*p > max)
		{
			max = *p;
			index = j;
		}
		p++;
	}
	return index;
}

int lastIndexToMax(const DECUMA_INT16* p, int nElements)
{
	/*Returns index to the minimum element in */
	/*the list p, with nElements elements. */
	DECUMA_INT16 max = *(p++);
	int index = 0,j;
	for(j = 1; j < nElements; j++)
	{
		if(*p > max)
		{
			max = *p;
			index = j;
		}
		p++;
	}
	return index;
}

int indexToMin32(const DECUMA_INT32* p, int nElements)
{
	/*Returns index to the minimum element in */
	/*the list p, with nElements elements. */
	DECUMA_INT32 min = *(p++);
	int index = 0,j;
	for(j = 1; j < nElements; j++)
	{
		if(*p < min)
		{
			min = *p;
			index = j;
		}
		p++;
	}
	return index;
}

int indexToMax32(const DECUMA_INT32* p, int nElements)
{
	/*Returns index to the minimum element in */
	/*the list p, with nElements elements. */
	DECUMA_INT32 max = *(p++);
	int index = 0,j;
	for(j = 1; j < nElements; j++)
	{
		if(*p > max)
		{
			max = *p;
			index = j;
		}
		p++;
	}
	return index;
}

DECUMA_INT16 minNUMBER(const DECUMA_INT16* p, int nElements)
{
	/*Returns the minimum element in */
	/*the list p, with nElements elements. */

	DECUMA_INT16 out = *(p++);
	int j;

	for(j = 1; j < nElements; j++)
	{
		if(*p < out)
		{
			out = *p;
		}
		p++;
	}
	return out;
}

DECUMA_INT16 maxNUMBER(const DECUMA_INT16* p, int nElements)
{
	/*Returns the minimum element in */
	/*the list p, with nElements elements. */

	DECUMA_INT16 out = *(p++);
	int j;

	for(j = 1; j < nElements; j++)
	{
		if(*p > out)
		{
			out = *p;
		}
		p++;
	}
	return out;
}

DECUMA_UINT32 decumaScale(DECUMA_UINT32 a, DECUMA_UINT32 b, int scaleFactorForRoundingEffects)
{
	/*Computes sqrt(a/b)*scaleFactorForRoundingEffects */
	/*It is assumed that a > 0 and b > 0. */

	/* vma fix for overflow. */
	/* If it is possible to multiply 'a' with '(scaleFactorForRoundingEffects * scaleFactorForRoundingEffects)' without overflow, set the 'localScaleFactor' to 'scaleFactorForRoundingEffects' */
	/* otherwise 'a' is too large, and using scaling would cause a overflow, so use 1 for localScaleFactor instead. */
	/* If you dont care about overflow, set 'localScaleFactor' to 'scaleFactorForRoundingEffects' */
/*	const int alimit = ( sizeof(int) == 4
			? ((int)0x7fffffff/(scaleFactorForRoundingEffects*scaleFactorForRoundingEffects))
			: ((int)0x7fff/(scaleFactorForRoundingEffects*scaleFactorForRoundingEffects)) );
	int localScaleFactor = a > alimit
		? 1
		: scaleFactorForRoundingEffects;*/


	DECUMA_UINT32 square;
	const int localScaleFactor = 16;
	DECUMA_UINT32 out;
	int j;


	while ( a > 0x00ffffff )
	{
		a >>= 1;
		b >>= 1;
	}

	if ( b == 0 )
		b = 1; /* To avoid div by zero. */

	decumaAssert( a < a * localScaleFactor * localScaleFactor);

	square = (a * localScaleFactor * localScaleFactor ) / b;
	out = (localScaleFactor + localScaleFactor*a/b)/2;



	for(j = 0; j < 2; j++)
	{
		out = (out + square/out)/2;
	}
	return (int) out * (scaleFactorForRoundingEffects / localScaleFactor);
}

DECUMA_UINT32 decumaScaleFloat(float a, float b, int scaleFactorForRoundingEffects)
{
	/*Computes sqrt(a/b)*scaleFactorForRoundingEffects */
	/*It is assumed that a > 0 and b > 0. */

	int j;

	if ( b == 0.0 )
		return MAX_DECUMA_UINT32;

	a /= b;
	b = (1 + a)/2.0f;

	for(j = 0; j < 4; j++)
	{
		b = (b + a/b)/2;
	}
	return (DECUMA_UINT32) (b * scaleFactorForRoundingEffects);
}

#ifndef ONPALM_ARMLET
static
#endif
const DECUMA_INT16 VectorForComputingProxSqrt [] = {1000,980,959,938,
	917,894,872,849,825,800,775,748,721,693,663,632,600,566,529,
	490,447,400,346,283,200,100};


/* Returns 1000*sqrt(1-x) */
DECUMA_INT16 SquareRootVerySpecial(float x, const DECUMA_INT16 maxProxValue)
{
	/* Approximate square-root in integers */
	const int lengthOfVectorForComputingProxSqrt =
		sizeof(VectorForComputingProxSqrt) / sizeof(VectorForComputingProxSqrt[0]);
	int k = (int)( (float) (lengthOfVectorForComputingProxSqrt-1) * x + 0.5f);
	int r;
	DECUMA_INT32 a, b;

	/* Note, that this is not only a sanity check, it is neccesary because */
	/* we may recieve coordinates that have caused overflow, and then  */
	/* x can be really strange. */

	if(x < 0.0)
		return maxProxValue;
	else if(x >= 1.0)
		return 0;

	decumaAssert(k >= 0);
	decumaAssert(k < lengthOfVectorForComputingProxSqrt);

	a = VectorForComputingProxSqrt[k];

	if(a != 0)
    {
		b =  (DECUMA_INT32) ( (float) maxProxValue * maxProxValue * (1.0f-x));

		for(r = 0; r < 2; r++)
		{
			a = (a + b/a + 1)/2;
		}
	}
	return (DECUMA_INT16) a;
} /*SquareRootVerySpecial() */

void derivative(const DECUMA_INT16 *pPointer, DECUMA_INT16 * derivPointer, int delta)
{
	int j;
	for(j = 0; j < delta-1; j++)
	{
		*derivPointer = - *(pPointer++);
		decumaAssert((DECUMA_INT32)*derivPointer + *pPointer <= MAX_DECUMA_INT16);
		decumaAssert((DECUMA_INT32)*derivPointer + *pPointer >= MIN_DECUMA_INT16);
		*derivPointer = (DECUMA_INT16) *derivPointer + *pPointer; /* The asserts above guarantee no truncation */
		derivPointer++;
	}
	*derivPointer = *(derivPointer-1);/* just copy the last element */
}

int deltaAngle(int a, int aRef)
{
	/* Compute the difference between two angles */
	/* Put the resulting angle in the interval +- pi. */

	int delta = a - aRef;

	if( delta < -PI)
		delta += 2*PI;
	else
		if(delta > PI)
			delta -= 2*PI;

	return delta;
}

#ifndef ONPALM_ARMLET
static
#endif
const int vectorForComputingSin[] = {0,13,25,38,50,62,74,86,98,109,121,132,142,
	152,162,172,181,190,198,206,213,220,226,231,237,241,245,248,251,253,255,256,256};
/* UINT16 scaleFactorForSin = 256; */
/* UINT8 shiftForSin = 8; */

DECUMA_INT32 decumaSin(DECUMA_INT32 x)
{

#ifdef _INCLUDE_MATH_LIB_bkjsdofidshfdrtkl
	double res = sin(((double)x)/100)*scaleFactorForSin;
	if(res > 0)
		return (DECUMA_INT32) (res + .5);
	else
		return (DECUMA_INT32) (res - .5);
#else
	/*returns 256*sin(x) */
	x = mod_sym(x,2*PI);

	if(x >= 0)
	{
		DECUMA_INT32 y = (x*64 + PI/2 )/PI; /* + PI/2 for rounding */
		if(y < 33)
		{
			return vectorForComputingSin[y] + (vectorForComputingSin[32 - y] * (x - (y*PI)/64))/100;
		}
		else
			return vectorForComputingSin[64 - y] - (vectorForComputingSin[y - 32] * (x - (y*PI)/64))/100;
	}
	else
	{
		DECUMA_INT32 y = (x*64 - PI/2)/PI; /* - PI/2 for rounding */
		if(y > -33)
		{
			return -vectorForComputingSin[-y] + (vectorForComputingSin[32 + y] * (x - (y*PI)/64))/100;;
		}
		else
			return -vectorForComputingSin[64 + y] - (vectorForComputingSin[-32 - y] * (x - (y*PI)/64))/100;
	}
#endif /* _INCLUDE_MATH_LIB_bkjsdofidshfdrtkl */
} /* decumaSin() */

DECUMA_INT32 decumaCos(DECUMA_INT32 x)
{
	/*returns 256*cos(x) */
	return decumaSin(x + PI/2);
}
