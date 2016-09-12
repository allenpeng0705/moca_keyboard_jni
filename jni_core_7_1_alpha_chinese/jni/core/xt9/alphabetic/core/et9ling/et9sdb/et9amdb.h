/*******************************************************************************
;*******************************************************************************
;**                                                                           **
;**                  COPYRIGHT 1997-2011 NUANCE COMMUNICATIONS                **
;**                                                                           **
;**               NUANCE COMMUNICATIONS PROPRIETARY INFORMATION               **
;**                                                                           **
;**     This software is supplied under the terms of a license agreement      **
;**     or non-disclosure agreement with Nuance Communications and may not    **
;**     be copied or disclosed except in accordance with the terms of that    **
;**     agreement.                                                            **
;**                                                                           **
;**     T9 Export Control Classification Number ECCN: EAR99                   **
;**                                                                           **
;*******************************************************************************
;**                                                                           **
;**     FileName: ET9amdb.h                                                   **
;**                                                                           **
;**  Description: Manufacture database (MDB) access header File.              **
;**               Conforming to version 5.2                                   **
;**                                                                           **
;*******************************************************************************
;******************************************************************************/

#ifndef ET9AMDB_H
#define ET9AMDB_H

#include "et9api.h"
#ifdef ET9_ALPHABETIC_MODULE

#if defined(__cplusplus)
extern "C" {
#endif

ET9STATUS ET9FARCALL _ET9AWMdbWordsSearch(ET9AWLingInfo          *pLingInfo,
                                          ET9U16                 wIndex,
                                          ET9U16                 wLength,
                                          ET9_FREQ_DESIGNATION   bFreqIndicator);

ET9STATUS ET9FARCALL _ET9AWMdbFind(ET9AWLingInfo                * const pLingInfo,
                                   ET9AWPrivWordInfo      const * const pWord);


#if defined(__cplusplus)
}
#endif

#endif /* ET9_ALPHABETIC_MODULE */
#endif /* #ifndef ET9AMDB_H */



