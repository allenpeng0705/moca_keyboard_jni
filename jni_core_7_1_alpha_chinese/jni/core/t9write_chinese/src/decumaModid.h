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

#ifndef DECUMA_MODID_H
#define DECUMA_MODID_H

/** @name Module Setup
 *  @{
 *
 *  Specify the modules that will be using various routines.
 *
 *  Because each module implemented its own set of slightly incompatible
 *  definitions of MODULE_ID and g_ppModuleName[], this facility was created
 *  to provide a single definition. This makes, for instance, decumaDebugMemory
 *  more robust since it does not risk crashing hard when reporting memory
 *  errors.
 *
 *  It is normally safe to include this header in a release but omit the C file;
 *  only internal tools needs to actually look up the module name.
 */

/**
 * @showinitializer
 */

typedef enum {
	MODID_GENERIC,      /**< Module GENERIC */
	MODID_HWRLIB,       /**< Module HWRLIB   Don't call this something else like (ASLLIB) since this constant is used in some
                            of the tools using the decuma_hwr API (like decuma_hwr_verify) */
	MODID_APPLICATION,  /**< Module APPLICATION */
	MODID_DICTIONARY,   /**< Module DICTIONARY_CONVERSION */
	MODID_UDMLIB,       /**< Module UDMLIB */
	MODULE_COUNT        /**< Number of Modules */
} MODULE_ID;

/**
 * @param modid MODULE_ID to get name for
 * @returns a const string with the name corresponding to module @c modid
 * @hideinitializer
 */
const char * MODULE_NAME(MODULE_ID modid);

/** @}
 */

#endif /* DECUMA_MODID_H */
