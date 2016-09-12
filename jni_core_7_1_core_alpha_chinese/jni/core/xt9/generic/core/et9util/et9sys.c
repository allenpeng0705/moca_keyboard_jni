/*******************************************************************************
;*******************************************************************************
;**                                                                           **
;**                    COPYRIGHT 2001-2011 NUANCE COMMUNICATIONS              **
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
;**     FileName: et9sys.c                                                    **
;**                                                                           **
;**  Description: ET9 Generic System Module                                   **
;**                                                                           **
;*******************************************************************************
;******************************************************************************/

/*! \addtogroup et9sys System Functions for XT9
* System for generic XT9.
* @{
*/

#include "et9sys.h"

/* " hh:mm:ss mm dd yyyy" */
#define BUILD_STAMP " "__TIME__" "__DATE__


/*******************************************************************************
 **
 **          G L O B A L S   A N D   L O C A L   S T A T I C S
 **
 ** ET9 does not make use of any dynamic global or local static variables!!
 ** It is acceptable to make use of constant globals or local statics.
 ** If you need persistent dynamic memory in the ET9 core, it should be
 ** allocated in the ET9AWLingPrivate data structure and fogged through the definitions
 ** found in the et9asystm.h file.
 **
 ******************************************************************************/


/*---------------------------------------------------------------------------*/
/**
 * @brief Retrieves XT9 core and build version information.
 *
 * @param[in,out] psCodeVerBufIn        Pointer to a buffer where XT9 writes the version information. The buffer should be at least ET9MAXVERSIONSTR characters in length.
 * @param[in]     wBufMaxSize           The length in characters of the buffer pointed to by psCodeVerBuf.
 * @param[out]    pwBufSize             Pointer to a value indicating the size of the version information XT9 has written to psCodeVerBuf.
 *
 * @retval ET9STATUS_NONE               Function call was handled successfully.
 * @retval ET9STATUS_INVALID_MEMORY     At least one pointer passed as a function argument was set to null.
 * @retval ET9STATUS_NO_MEMORY          The buffer pointed to by psCodeVerBuf is too small to store the information. Ensure the buffer is at least ET9MAXVERSIONSTR bytes.
 *
 * @remarks The string is always 43 chars long. String format is defined as follows: <tt>ET9 VMM.mm.bf.qa hh:mm:ss mm dd yyyy xxxx</tt>, where:<br>
 * \c MM   = major version number<br>
 * \c mm   = minor version number<br>
 * \c bf   = patch/bugfix version number<br>
 * \c qa   = release candidate version number<br>
 * \c hh   = hour of release compile<br>
 * \c mm   = minutes of release compile<br>
 * \c ss   = seconds of release compile<br>
 * \c dd   = day of month<br>
 * \c mmm = month (Jan, Feb, Mar, Apr, May, Jun, Jul, Aug, Sep, Oct, Nov, Dec)<br>
 * \c yyyy = four-digit year<br>
 * \c xxxx = OEM ID in hex
 *
 */

ET9STATUS ET9FARCALL ET9GetCodeVersion(ET9SYMB * const  psCodeVerBufIn,
                                       const ET9U16     wBufMaxSize,
                                       ET9U16 * const   pwBufSize)
{
    ET9STATUS wStatus = ET9STATUS_NONE;
    ET9U8 *pbStr;
    ET9U8 pBuildStamp[] = BUILD_STAMP;
    ET9SYMB * psCodeVerBuf = psCodeVerBufIn;
    ET9INT i;

    if (psCodeVerBuf == NULL || pwBufSize == NULL) {
        wStatus = ET9STATUS_INVALID_MEMORY;
    }
    else {
        *pwBufSize = 0;

        if (wBufMaxSize < ET9MAXVERSIONSTR) {
            wStatus = ET9STATUS_NO_MEMORY;
        }
        else {
            pbStr = (ET9U8*)ET9COREVER;
            while (*pbStr) {
                *psCodeVerBuf++ = (ET9SYMB)*pbStr++;
                ++(*pwBufSize);
            }

            pbStr = pBuildStamp;

            while (*pbStr) {
                *psCodeVerBuf++ = (ET9SYMB)*pbStr++;
                ++(*pwBufSize);
            }
            *psCodeVerBuf++ = ' ';
            for (i = 12; i >= 0; i -= 4) {
                *psCodeVerBuf++ = (ET9SYMB)((((ET9U16)ET9OEMID >> i) & 0x000F) + '0');
            }
            *pwBufSize += 5;
        }
    }
    return wStatus;
}


/*! @} */
/* ----------------------------------< eof >--------------------------------- */
