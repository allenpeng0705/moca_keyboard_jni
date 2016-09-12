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


#ifndef databaseKID_h
#define databaseKID_h

#include "decumaDataTypes.h"
#include "databaseFormat.h"
#include "decumaCategoryTableType.h" /* Definition of CATEGORY_TABLE_PTR */

typedef DECUMA_INT16 KEYINDEX;
typedef DECUMA_INT8 KEY_DIACRITIC_INDEX;

typedef struct tagSCR_DATABASE_1 *DBID;

typedef struct tagKID
{
	KEY_DB_HEADER_PTR pKeyDB;
	PROPERTY_DB_HEADER_PTR pPropDB;
	CATEGORY_TABLE_PTR pCatTable;

	KEYINDEX baseKeyIndex; /* 0 is first index */
	DECUMA_UINT8 noBaseArcs;
	KEY_DIACRITIC_INDEX diacKeyIndex; /* 0 is first index */
	DECUMA_UINT8 noDiacArcs;
	
	DECUMA_INT16 diacKeyXOffset; /* The X-offset for the diacritic KEY. */
	/* diacKeyXOffset is set in fullsearch. For the moment it is not used, hence set to zero in fullsearch. */
	/* Using diacKeyXOffset requires a database with diacritic keys centered around the origin. All files */
	/* and macros (except the database) are prepared for this.  EM */
	DECUMA_INT16 diacKeyYOffset; /* The Y-offset for the diacritic KEY. */

	KEY_PTR pKey;
	KEY_DIACRITIC_PTR pDiacKey;
}KID;

#endif /* databaseKID_h */
