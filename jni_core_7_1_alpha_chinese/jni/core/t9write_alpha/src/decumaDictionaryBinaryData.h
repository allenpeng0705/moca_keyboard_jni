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


/************************************************** Description ***\

  This file describes the interface to a Decumas HWR dictionary binary header
  which is used to identify the format of a dictionary binary.
    
\******************************************************************/


#ifndef DECUMA_DICTIONARY_BINARY_DATA_H
#define DECUMA_DICTIONARY_BINARY_DATA_H

#if defined(DEBUG) && !defined(DECUMA_DICTIONARY_BINARY_C)
#error Only decumaDictionaryBinary.c should need the data definition
#endif


#include "decumaBasicTypes.h"

#define DECUMA_DICTIONARY_BINARY_ID 0xDADC  /*"DecumA DiCtionary" */

#define DECUMA_DICTIONARY_BINARY_FORMAT_VERSION 1 /*Increase each time the data structure below is changed */

/*Note that this header must have the same size as  */
/*declared in the header file macro */
struct _DECUMA_DICTIONARY_BINARY_HEADER
{
	DECUMA_UINT32  decumaID;   /*Should be verified against DECUMA_HWR_DICTIONARY_ID  */
	/*--  4 bytes */
	DECUMA_UINT32  formatVersion; /*Should be verified against DECUMA_DICTIONARY_BINARY_FORMAT_VERSION  */
	/*--  8 bytes */
	DECUMA_UINT8   decumaDictionaryStructure; /*Trie, terse trie, etc. */
	DECUMA_UINT8   dummy1; 
	DECUMA_UINT16  dummy2; 
	/*-- 12 bytes */
	DECUMA_UINT32  dummy3;  
	/*-- 16 bytes */
	DECUMA_UINT32  dummy4;  
	/*-- 20 bytes */
	DECUMA_UINT32  dummy5;  
	/*-- 24 bytes */
	DECUMA_UINT32  dummy6;  
	/*-- 28 bytes */
	DECUMA_UINT32  dummy7;  
	/*-- 32 bytes	 */
};

/*Following the binary header is the dictionary header which */
/*is different for different types of dicitionaries */


#endif /*DECUMA_DICTIONARY_BINARY_DATA_H */

