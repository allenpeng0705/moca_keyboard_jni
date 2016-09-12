/* Release C file */

#define DECUMA_HWR_PRIVATE static
#define DECUMA_HWR_PRIVATE_DATA_H static
#define DECUMA_HWR_PRIVATE_DATA_C static

/* Alphabetic source files */
#ifdef T9WRITE_ALPHABETIC
/* DON'T USE, CONFLICTING STATIC FUNCTIONS IF USED */
#include "aslArc.c"
#include "aslArcSession.c"
#include "asl.c"
#include "aslRG.c"
#include "aslSG.c"
#include "aslTools.c"
#include "database.c"
#include "databaseKEY.c"
#include "decumaCategories.c"
#include "decumaCategoriesHidden.c"
#include "decumaCategoryTranslation.c"
#include "decumaDictionaryBinary.c"
#include "decumaDictionary.c"
#include "decumaDictionaryXT9.c"
#include "decumaExtractLangCatTables.c"
#include "decumaHashHandler.c"
#include "decuma_hwr.c"
#include "decumaMath.c"
#include "decumaMemoryPool.c"
#include "decumaQCandHandler.c"
#include "decumaResamp.c"
#include "decumaSimTransf.c"
#include "decumaTerseTrie.c"
#include "decumaTrie.c"
#include "scrAlgorithm.c"
#include "scrAPI.c"
#include "scrCurve.c"
#include "scrFineSearch.c"
#include "scrFullSearch.c"
#include "scrIterators.c"
#include "scrlib.c"
#include "scrLigature.c"
#include "scrMeasureId.c"
#include "scrOutput.c"
#include "scrOutputHandler.c"
#include "scrProxCurve.c"
#include "scrZoom.c"
#include "udb_words.c"
#include "udmAccess.c"
#include "udmDynamic.c"
#include "udmEngineAccess.c"
#include "udmKey.c"

#endif

/* UDMLib source files, intermediate solution (will be replaced by proper dynamic database handling) */
#ifdef T9WRITE_UDMLIB


#endif

/* CJK source files */
#ifdef T9WRITE_CJK

#include "decuma_hwr.c"
#include "decumaIntegerMath.c"
#include "decumaKMeans.c"
#include "decumaQsort.c"
#include "decumaUnicode.c"
#include "cjkArcSession.c"
#include "cjkBestList.c"
#include "cjkBestListSpecialChecks.c"
#include "cjkClusterTree.c"
#include "cjkCoarseFeatures.c"
#include "cjkCompressedCharacter.c"
#include "cjkContext.c"
#include "cjkDatabase.c"
#include "cjkDatabaseCoarse.c"
#include "cjkDynamicDatabase.c"
#include "cjk.c"
#include "cjkMath.c"
#include "cjkSession.c"
#include "cjkStroke.c"

#endif

/* Source files common to both products */
#include "decumaCategoryCombinationData.c"
#include "decumaHwrSampler.c"
#include "decumaMemory.c"
#include "decumaRuntimeMalloc.c"
#include "decumaString.c"

