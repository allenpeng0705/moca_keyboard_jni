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

        File: scrLib.h
     Package: This file is part of the package SCRLIB 

   $Revision: 1.5 $
       $Date: 2011/02/14 11:41:14 $


  This file describes the interface to Decumas HWR engine for single 
  character recognition. 
  
\******************************************************************/

#ifndef SCRLIB_sdhkfusdijoasdguyasdbjhzxckjn
#define SCRLIB_sdhkfusdijoasdguyasdbjhzxckjn

#include "decuma_point.h"     /* definition of DECUMA_POINT */
#include "decumaUnicodeTypes.h"    /* included for DECUMA_UNICODE */
#include "databaseStatic.h"   /* definition of STATIC_DB_PTR */
#include "decumaCategoryTableType.h"   /* definition of CATEGORY_TABLE_PTR */
#include "decumaStatus.h"     /*definition of DECUMA_STATUS */
#include "decumaCurve.h"      /*definition of DECUMA_CURVE  */
#include "udmType.h"				/* definition of UDM_PTR */
#include "decumaCharacterSetType.h" /* definition of DECUMA_CHARACTER_SET */

#ifndef SCRLIB_API
#define SCRLIB_API
#endif



#ifndef FULL_ROTATION_IMPORTANCE
#define FULL_ROTATION_IMPORTANCE (1024)
#endif

/* Maximal proximity value returned by scrRecognize(), see SCR_RESULT below. */
#define MAX_PROXIMITY 1000             

#ifdef __cplusplus
extern "C" {
#endif

typedef const void DECUMA_DB_STORAGE * SCR_ANY_DB_PTR; /* Static or dynamic DB */

typedef struct tagSCR_SETTINGS
{
    /* The character set limits the set of characters from which the interpretation */
    /* is taken. The limitation has two dimensions, "symbol categories" and "languages". */
    /* */
    /* It is not suitable to search */
    /* among all the symbols at once, there are too many symbols  */
    /* that look too similar: 'C' and '(' for example. This is */
    /* the reason for symbol categories. */
    /* The languages is another dimension for limiting the character set. */
    /* Several categories and languages can be used for one call. Therefore */
    /* arrays are used. */
    /* Note that every category and language is not supported by every database */
    /* Therefore the functions scrDatabaseSupportsCategory/-Language can */
    /* be used. */

	 DECUMA_CHARACTER_SET characterSet;
    
    /*////// */
    /* It is possible to improve the interpretation by supplying an angle */
    /* that tells the algorithm what is considered to be "up". The angle */
    /* makes it possible to distinguish between 'c' and 'U' for example */
    /* */
    /* The normal angle is zero degrees, which implies that up is in the  */
    /* direction of negative Y axis. A full circle is 360 degrees. */
    /* */
    /*                (90 deg) */
    /*                  Y- */
    /*                  ^ */
    /*                  | */
    /*                  | */
    /* (180 deg)X- <----+----> X+ (0 deg) */
    /*                  | */
    /*                  | */
    /*                  v */
    /*                  Y+ */
    /*               (270 deg) */

    DECUMA_INT16 refangle;  /* Angle that is considered to be "up". Default */
                            /* is zero. */
    DECUMA_UINT16 refangleimportance; /* How important is the refangle. 0 => not  */
                            /* important. FULL_ROTATION_IMPORTANCE => important. */

    /*////// */
    /* It is possible to improve the interpretation results by supplying */
    /* a helpline and a baseline. This is only possible when the ref-angle */
    /* is zero. */
    /* Note that since positive Y is downwards, the helpline value should be */
    /* lower then the baseline value (if the values are supplied) */
    /* */
    /* If the baseline or helpline is not available, set both to zero. */
    /* */
    /* (In this 'htAj' example, the symbols are written overly correct) */
    /* */
    /*       *          *          ***   */
    /*       *          *         *   * */
    /*       *          *         *   *       * */
    /*  -----*--**----*****------*******---------   helpline */
    /*       * *  *     *        *     *      * */
    /*       **   *     *   *   *       *     *   */
    /*  -----*----*------***----*-------*-----*--   baseline */
    /*                                        *    */
    /*                                     *  *    */
    /*                                      **     */
    DECUMA_INT32 nBaseLineY;         /* Where on the Y axis the text baseline is. */
    DECUMA_INT32 nHelpLineY;         /* Where on the Y axis the text helpline is. */

    /* Note the coordinates for nBaseLineY and nHelpLineY shall be before */
    /* any offset and rightshift. */

    STATIC_DB_PTR pDB;  /* Pointer to the static database (must be set). */
    
    UDM_PTR pUDM;      /* Pointer to a User Database Modifier, or NULL if only. */
    								/* the static database should be used. */
    
    CATEGORY_TABLE_PTR pCatTable; /*Pointer to a personalized category table */
                                      /*or NULL if the default category table inluded */
                                      /*with the static database should be used. */
} SCR_SETTINGS;

typedef struct tagSCR_RESULT 
{
    DECUMA_UNICODE * pText;    /* A pointer to memory allocated by the caller. */
                        /* A zero-terminated string will be written in this */
                        /* memory by scrRecognize. The characters */
                        /* are encoded with UTF16. */
    DECUMA_UINT16 nMaxText;       /* The number of characters that the pText vector */
                                  /* can hold, including the terminating zero. */
    DECUMA_UINT16 proximity;      /* The proximity value set by scrRecognize (A value */
                                  /* between 0 and MAX_PROXIMITY) */
} SCR_RESULT;

    
/**
FUNCTION: scrGetMemoryBufferSize

This function returns the number of bytes that needs to be allocated for the
memory buffer that is to be passed as one of the parameters to scrRecognize.
*/

SCRLIB_API unsigned int scrGetMemoryBufferSize(void);




/**
FUNCTION: scrRecognize

The function evaluates a set of arcs (that are assumed to belong to the 
same symbol) and fills a vector with possible interpretations sorted by 
proximity value.

Parameters
    nArcs   The number of arcs that the character consists of. 
            (three for an capital A, for example). This is the same value
            as the number of strokes that the user used to produce
            the character.

  ppPoints  A pointer to a vector of pointers to arc Data. The vector 
            should be 'nArcs' elemement long. Each pointer in the vector shall
            point to a vector of arc data (points). Each arc have its length 
            defined in the pnArcLengths parameter. For example, ppPoints[2] 
            points to a vector that is pnArcLengths[2] long.

  pnArcLengths
            Pointer to a vector of arc length information. The length
            of the pnArcLengths vector shall be 'nArcs'.The n:th element 
            shall contain the length of the n:th arc. 

  pResults  Pointer to a vector of one or several SCR_RESULT objects that will 
            recieve the interpretation result. 

  nResults  The number of results we want in return. This is also the length
            of the pResults vector. If More than one return is wanted, the
            returned values will be sorted by their proximity.
            It is possible that less number of results than nResults are
            returned, see pnResultsReturned below. It is also possible that
            no symbols at all are returned if the chosen category has no 
            matches for the actual number of arcs. 

  pnResultsReturned
            Pointer to an integer where the number of returned symbols is
            written.

  pSettings Extra parameters to the interpretation engine. See documentation
            of SCR_SETTINGS for additional information.
            
  pMemoryBuffer
  				Pointer to a memory that is allocated and ready to be used by the SCR
  				engine. The memory needs to be at least the size returned by 
  				scrGetMemoryBufferSize(), and should preferably be located on the heap.

Return value:
    A value from the DECUMA_STATUS enumeration. A value of zero indicates that 
    there are no problems.
  */

SCRLIB_API DECUMA_STATUS scrRecognize(
			const DECUMA_CURVE * pCurve,
			SCR_RESULT * pResults, unsigned int nResults, 
			int * pnResultsReturned, 
			const SCR_SETTINGS * pSettings,
			void * pMemoryBuffer,
			const DECUMA_INTERRUPT_FUNCTIONS * pInterruptFunctions);


/**
FUNCTION: scrDatabaseSupportsSymbolCategory
Parameters: 
	pDB   Pointer to a static database or User Database Modifier
	cat   Symbol category to test.

Return value:
One if the category is supported by the database.
Zero if it is not supported.
**/
SCRLIB_API int scrDatabaseSupportsSymbolCategory(SCR_ANY_DB_PTR pDB, DECUMA_UINT32 cat);


/**
FUNCTION: scrDatabaseSupportsLanguage
Parameters: 
	pDB   Pointer to a static database or User Database Modifier
	lang  Language to test.

Return value:
One if the category is supported by the database.
Zero if it is not supported.
**/
SCRLIB_API int scrDatabaseSupportsLanguage(SCR_ANY_DB_PTR pDB, DECUMA_UINT32 lang);

/* pSymbol is a null-terminated symbol that will be searched for in the database */
/* pCharSet can be NULL. Then the whole database will be searched for the symbol */
/*          If non-NULL only the intersection of the provided character set specification */
/*          and the symbols contained in the database will be searched. */
SCRLIB_API DECUMA_STATUS scrDatabaseIncludesSymbol(
	SCR_ANY_DB_PTR pDB, 
	const DECUMA_CHARACTER_SET * pCharSet,
	const DECUMA_UNICODE * pSymbol,
	int * pbIncluded);

/**
FUNCTION: scrGetVersion
Parameters:
  (none)

Return value:
A pointer to a null terminated string with the version of the SCRLIB.
**/
SCRLIB_API const char * scrGetVersion(void);

/**
FUNCTION: scrCheckStaticDB
Parameters:
	pDB   Pointer to the static database

Return value:
    A value from the DECUMA_STATUS enumeration. A value of decumaNoError indicates 
	that this database is valid.
**/
SCRLIB_API DECUMA_STATUS scrCheckStaticDB(STATIC_DB_PTR pDB);

#ifdef __cplusplus
} /* extern "C" { */
#endif

#endif


