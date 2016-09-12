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


#include "scrMeasureId.h"
#include "globalDefs.h"
#include "decumaAssert.h"

/**
Here folows the templates for how measure IDs shall be translated
into INT16 vectors of inclusion/exclusion numbers.

  The vectors shall be interpreted like this:
const char mid1IncExcl[]  =  { 1 , 0 };

  This means that the first 1/2 will be filled with 1, and the second
  half will be filled with 0.

const char mid6IncExcl[]  =  { 0 , 1 , 0,  0};

  This means that the first quarte will be set to zero, the second quarter
  set to 1, and the last half of the array will be set to 0.
*/


typedef struct _tagMEASURE_ID_DATA
{
	const DECUMA_UINT8 nSections;
	const DECUMA_UINT8 bitMask;
} MEASURE_ID_DATA;


#ifdef ONPALM_ARMLET
/* ARMlets cannot have static const data */
#define STATIC_SPECIFIER
#else
#define STATIC_SPECIFIER static
#endif
/*
// measure id 0
STATIC_SPECIFIER const char midIncExcl0[]  =					{ 1 };

// measure id 1
STATIC_SPECIFIER const char midIncExcl1[]  =				{ 1 , 0 };

// measure id 2
STATIC_SPECIFIER const char midIncExcl2[]  =				{ 0 , 1 };

// measure id 3
STATIC_SPECIFIER const char midIncExcl3[]  =			{ 0 , 1 , 0 };

// measure id 4
STATIC_SPECIFIER const char midIncExcl4[]  =			{ 1 , 0 , 0 };

// measure id 5
STATIC_SPECIFIER const char midIncExcl5[]  =		{ 1 , 0 , 0 , 0 };

// measure id 6
STATIC_SPECIFIER const char midIncExcl6[]  =		{ 0 , 1 , 0 , 0};

// measure id 7
STATIC_SPECIFIER const char midIncExcl7[]  =			{ 1 , 1 , 0};

// measure id 8
STATIC_SPECIFIER const char midIncExcl8[]  =		{ 0 , 0 , 0 , 1};

// measure id 9
STATIC_SPECIFIER const char midIncExcl9[]  =			{ 0 , 0 , 1};

// measure id 10
STATIC_SPECIFIER const char midIncExcl10[] ={  0 , 1 , 0,  0 , 0 , 0};



#define MEASURE_ID_ENTRY( MID ) { sizeof( midIncExcl##MID) / sizeof(midIncExcl##MID[0]) , midIncExcl##MID }

const MEASURE_ID_DESCRIPTOR measureid_midd [] = {
	MEASURE_ID_ENTRY(0),
	MEASURE_ID_ENTRY(1),
	MEASURE_ID_ENTRY(2),
	MEASURE_ID_ENTRY(3),
	MEASURE_ID_ENTRY(4),
	MEASURE_ID_ENTRY(5),
	MEASURE_ID_ENTRY(6),
	MEASURE_ID_ENTRY(7),
	MEASURE_ID_ENTRY(8),
	MEASURE_ID_ENTRY(9),
	MEASURE_ID_ENTRY(10),
};
*/
#define BIT_0 0x01
#define BIT_1 0x02
#define BIT_2 0x04
#define BIT_3 0x08
#define BIT_4 0x10
#define BIT_5 0x20
#define BIT_6 0x40
#define BIT_7 0x80

#define OFF_0 0
#define OFF_1 0
#define OFF_2 0
#define OFF_3 0
#define OFF_4 0
#define OFF_5 0
#define OFF_6 0
#define OFF_7 0


/* Measure ID 0: 1 */
#define measureMask_00 ( BIT_0 )
#define measureSize_00 ( 1 )
/* Measure ID 1: 10 */
#define measureMask_01 ( BIT_0 | OFF_1 )
#define measureSize_01 ( 2 )
/* Measure ID 2: 01 */
#define measureMask_02 ( OFF_0 | BIT_1 )
#define measureSize_02 ( 2 )
/* Measure ID 3: 010 */
#define measureMask_03 ( OFF_0 | BIT_1 | OFF_2 )
#define measureSize_03 ( 3 )
/* Measure ID 4: 100 */
#define measureMask_04 ( BIT_0 | OFF_1 | OFF_2 )
#define measureSize_04 ( 3 )
/* Measure ID 5: 1000 */
#define measureMask_05 ( BIT_0 | OFF_1 | OFF_2 | OFF_3 )
#define measureSize_05 ( 4 )
/* Measure ID 6: 0100 */
#define measureMask_06 ( OFF_0 | BIT_1 | OFF_2 | OFF_3 )
#define measureSize_06 ( 4 )
/* Measure ID 7: 110 */
#define measureMask_07 ( BIT_0 | BIT_1 | OFF_2 )
#define measureSize_07 ( 3 )
/* Measure ID 8: 0001 */
#define measureMask_08 ( OFF_0 | OFF_1 | OFF_2 | BIT_0 )
#define measureSize_08 ( 4 )
/* Measure ID 9: 001 */
#define measureMask_09 ( OFF_0 | OFF_1 | BIT_2 )
#define measureSize_09 ( 3 )
/* Measure ID 10: 010000 */
#define measureMask_10 ( OFF_0 | BIT_1 | OFF_2 | OFF_3 | OFF_4 | OFF_5 )
#define measureSize_10 ( 6 )


STATIC_SPECIFIER const MEASURE_ID_DATA measureid_midd [] = {
	{measureSize_00, measureMask_00},
	{measureSize_01, measureMask_01},
	{measureSize_02, measureMask_02},
	{measureSize_03, measureMask_03},
	{measureSize_04, measureMask_04},
	{measureSize_05, measureMask_05},
	{measureSize_06, measureMask_06},
	{measureSize_07, measureMask_07},
	{measureSize_08, measureMask_08},
	{measureSize_09, measureMask_09},
	{measureSize_10, measureMask_10}
};


#ifdef ONPALM_ARMLET
#include "dataReloc.h"
void measureIdReloc(void * pArmlet)
{
/*
	int n;
	for ( n = 0; n < sizeof(measureid_midd)/sizeof(measureid_midd[0]); n++) {
		dataReloc(pArmlet, (void*)&measureid_midd[n].pIncExcl);
	}
*/
}
#endif

int GetMeasureIncExcl(int mid, int totlen, int checkme)
{
	const MEASURE_ID_DATA midData = measureid_midd[ mid ];

	decumaAssert( mid < sizeof( measureid_midd ) / sizeof( measureid_midd[0]) );

	decumaAssert( totlen != 0 );

	if ( mid == 0 ) {
		/* All the bits for measure ID 0 shall be 1. */
		/* By returning constant 1 we avoid divisions for the most common case. */
		/* Lets check it.. */
		decumaAssert( (midData.bitMask >> ((checkme * midData.nSections) / totlen) ) & BIT_0 );
		return 1;
	}

	return (midData.bitMask >> ((checkme * midData.nSections) / totlen) ) & BIT_0;
}

int GetMeasureSection(int measure_id, DECUMA_INT16 * pTarget, int nTotalLength, int nStartAt, int nExtractLength)
{
/*	int section; */
	int nMass = 0;
	int i = 0;

	for(i = nStartAt ; i < nExtractLength + nStartAt ; i++)
	{
		nMass += pTarget[i-nStartAt] = GetMeasureIncExcl(measure_id, nTotalLength, i);
	}
	/*


	for(section = (nStartAt * midd[measure_id].nSections ) / nTotalLength ; section < midd[measure_id].nSections ; section++)
	{
		// vma Note that we have to recalculate the start and stop element each time. It is not
		// sufficient to just add the nTotalLength/midd[measure_id].nSections.
		// consider the case of three sections and 32 points. The third section would then wrongly
		// be calculated to begin at offset 20. (0 , 10, 20)
		int nFirstElement = ( nTotalLength * section ) / midd[measure_id].nSections;
		int nFirstElementNextSection = ( nTotalLength * (section + 1) ) / midd[measure_id].nSections;
		int nIteratorElement ;

		for(nIteratorElement = nFirstElement ; nIteratorElement < nFirstElementNextSection ; nIteratorElement++)
		{
			if(nIteratorElement < nStartAt)
				continue; // We are not in the data extration window yet.
			if(nIteratorElement >= nStartAt + nExtractLength)
				return nMass; // we have passed the data extraction window.
			pTarget[nIteratorElement - nStartAt] = (INT16) midd[measure_id].pIncExcl[section];
			nMass += midd[measure_id].pIncExcl[section];
		}
	}
	*/
	return nMass;
}
