LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

# magic number to get t9write to recognize long word
MY_LOCAL_CFLAGS := -finline-limit=300
LOCAL_CFLAGS += -Wfatal-errors $(MY_LOCAL_CFLAGS) -DT9WRITE_ALPHABETIC

LOCAL_SRC_FILES := \
	src/aslArc.c \
	src/aslArcSession.c \
	src/asl.c \
	src/aslRG.c \
	src/aslSG.c \
	src/aslTools.c \
	src/database.c \
	src/databaseKEY.c \
	src/decumaCategories.c \
	src/decumaCategoriesHidden.c \
	src/decumaCategoryCombinationData.c \
	src/decumaCategoryTranslation.c \
	src/decumaDictionaryBinary.c \
	src/decumaDictionary.c \
	src/decumaDictionaryXT9.c \
	src/decumaExtractLangCatTables.c \
	src/decumaHashHandler.c \
	src/decuma_hwr.c \
	src/decumaHwrSampler.c \
	src/decumaMath.c \
	src/decumaMemory.c \
	src/decumaMemoryPool.c \
	src/decumaQCandHandler.c \
	src/decumaResamp.c \
	src/decumaRuntimeMalloc.c \
	src/decumaSimTransf.c \
	src/decumaString.c \
	src/decumaTerseTrie.c \
	src/decumaTrie.c \
	src/scrAlgorithm.c \
	src/scrAPI.c \
	src/scrCurve.c \
	src/scrFineSearch.c \
	src/scrFullSearch.c \
	src/scrIterators.c \
	src/scrlib.c \
	src/scrLigature.c \
	src/scrMeasureId.c \
	src/scrOutput.c \
	src/scrOutputHandler.c \
	src/scrProxCurve.c \
	src/scrZoom.c \
	src/t9wl.c \
	src/udb_words.c \
	src/udmAccess.c \
	src/udmDynamic.c \
	src/udmEngineAccess.c \
	src/udmKey.c \

	
LOCAL_C_INCLUDES := \
	$(LOCAL_PATH) \
	$(LOCAL_PATH)/../../include \
	$(LOCAL_PATH)/api \
	$(LOCAL_PATH)/src

LOCAL_MODULE := libt9write_alpha
LOCAL_PRELINK_MODULE := false
# Build Target: Shared Library ("libt9write_alpha.a")
include $(BUILD_STATIC_LIBRARY)

