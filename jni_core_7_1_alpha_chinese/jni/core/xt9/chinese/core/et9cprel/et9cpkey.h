/*******************************************************************************
;*******************************************************************************
;**                                                                           **
;**                    COPYRIGHT 2004-2011 NUANCE COMMUNICATIONS              **
;**                                                                           **
;**               NUANCE COMMUNICATIONS PROPRIETARY INFORMATION               **
;**                                                                           **
;**     This software is supplied under the terms of a license agreement      **
;**     or non-disclosure agreement with Nuance Communications and may not    **
;**     be copied or disclosed except in accordance with the terms of that    **
;**     agreement.                                                            **
;**                                                                           **
;**                                                                           **
;*******************************************************************************
;**                                                                           **
;**     FileName: et9cpkey.h                                                  **
;**                                                                           **
;**  Description: Chinese Phrase Text Input  key module header file.          **
;**               Conforming to the development version of Chinese XT9.       **
;**                                                                           **
;*******************************************************************************
;******************************************************************************/

#ifndef ET9CPKEY_H
#define ET9CPKEY_H 1

/* don't mangle the function name if compile under C++ */
#if defined(__cplusplus)
extern "C" {
#endif

#define ET9_CP_TONE_DEL_KEY_MASK       0xE0    /* mask for phonetic tone or delimiter in
                                             the key buffer. 0010 - 1010 for
                                             tones 1 - 5. 1110 for delimiter.
                                           */
#define ET9_CP_PHONETIC_KEY_MASK      0x1F    /* mask for phonetic key in the key buffer.
                                             For the 9-key, 4 bits are enough. For
                                             explicit input, we need 5 bits.
                                           */
#define ET9_CP_DELIMITER_KEY_MASK     ET9_CP_TONE_DEL_KEY_MASK    /* mask for phonetic delimiter in
                                             the key buffer.
                                           */
#define ET9_CP_TONE_DEL_MASK_SHIFT       5     /* number of shift in the key encoding
                                             to get the tone or delimilter
                                           */

#define ET9_CP_IsKeyToneOrDelim(pET9CPLingInfo, bKey)           \
    (ET9CPIsExplInputActive(pET9CPLingInfo) ? \
    ET9CPSymToCPTone((bKey)) || ET9CPSYLLABLEDELIMITER == (bKey) : (bKey) & ET9_CP_TONE_DEL_KEY_MASK)

#define ET9_CP_IsKeyDelim(pET9CPLingInfo, bKey)           \
    (ET9CPIsExplInputActive(pET9CPLingInfo) ? \
    ET9CPSYLLABLEDELIMITER == (bKey) : (ET9_CP_DELIMITER_KEY_MASK == ((bKey) & ET9_CP_TONE_DEL_KEY_MASK)))

#define ET9_CP_IsKeyTone(pET9CPLingInfo, bKey)           \
    (ET9CPIsExplInputActive(pET9CPLingInfo) ? \
    ET9CPSymToCPTone((bKey)) : (bKey) & ET9_CP_TONE_DEL_KEY_MASK && ET9_CP_DELIMITER_KEY_MASK != (bKey) & ET9_CP_TONE_DEL_KEY_MASK)

#define ET9_CP_IsToneOrDelim(b) (ET9CPSymToCPTone((b)) || ET9CPSYLLABLEDELIMITER == (b) || ET9CP_SEGMENT_DELIMITER == (b))

/* End don't mangle the function name if compile under C++ */
#if defined(__cplusplus)
    }
#endif

#endif  /* #ifndef ET9CPKEY_H */

/* ----------------------------------< eof >--------------------------------- */

