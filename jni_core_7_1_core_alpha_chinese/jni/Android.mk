LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

T9WRITE_ALPHABETIC_CORE = core/t9write_alpha
T9WRITE_CHINESE_CORE = core/t9write_chinese
XT9_GENERIC_CORE = core/xt9/generic/core
XT9_ALPHABETIC_CORE = core/xt9/alphabetic/core
XT9_CHINESE_CORE = core/xt9/chinese/core

LOCAL_SRC_FILES := \
	src/dbregistry.cpp \
	src/user_study_log.cpp \
	src/load_file.cpp \
	src/ucs2.c \
	src/core_verions.c \
	src/dbg_mem.cpp \
	config_native.cpp \
	config_version.cpp \
	alpha_chinese_input_source.cpp \
	com_moca_input.cpp \
	alpha_write_source.cpp \
	chinese_write_source.cpp \
	
ifeq ($(HOST_OS),cygwin)
	LOCAL_CFLAGS += -DNDK_BUILD 
	LOCAL_LDLIBS := -ldl -llog
else
	ifdef NDK_ROOT
		LOCAL_CFLAGS += -DNDK_BUILD
		LOCAL_LDLIBS := -ldl -llog
	else
		LOCAL_SHARED_LIBRARIES := \
			libcutils
	endif
endif

LOCAL_STATIC_LIBRARIES := \
	libxt9core \
	libt9write_alpha \
	libt9write_chinese \
	
LOCAL_PRELINK_MODULE := false
LOCAL_CFLAGS += -Wfatal-errors 

##
## NEED TO TURN ON FOR ALL OEM/CUSTOMER BUILDS
##
#OEM_FLAGS=-DOEM_BUILD

### FOR DEVL DEBUG BUILD ONLY
##DEV_FLAGS=-DDEV_DEBUG_TEST_WITHOUT_KEY_XYZ
 
LOCAL_CFLAGS += ${OEM_FLAGS}
LOCAL_CFLAGS += ${DEV_FLAGS}

LOCAL_C_INCLUDES := \
	$(JNI_H_INCLUDE) \
	$(LOCAL_PATH) \
	$(LOCAL_PATH)/src \
	$(LOCAL_PATH)/include \
	$(LOCAL_PATH)/core/include \
	$(LOCAL_PATH)/$(XT9_GENERIC_CORE)/et9api \
	$(LOCAL_PATH)/$(XT9_GENERIC_CORE)/et9input/et9imu \
	$(LOCAL_PATH)/$(XT9_GENERIC_CORE)/et9input/et9kdb \
	$(LOCAL_PATH)/$(XT9_GENERIC_CORE)/et9util \
	$(LOCAL_PATH)/$(XT9_ALPHABETIC_CORE)/et9API \
	$(LOCAL_PATH)/$(XT9_ALPHABETIC_CORE)/et9input/et9imu \
	$(LOCAL_PATH)/$(XT9_ALPHABETIC_CORE)/et9ling/et9ldb \
	$(LOCAL_PATH)/$(XT9_ALPHABETIC_CORE)/et9ling/et9list \
	$(LOCAL_PATH)/$(XT9_ALPHABETIC_CORE)/et9ling/et9sdb \
	$(LOCAL_PATH)/$(XT9_ALPHABETIC_CORE)/et9util \
	$(LOCAL_PATH)/$(XT9_CHINESE_CORE)/et9cprel \
	$(LOCAL_PATH)/$(T9WRITE_ALPHABETIC_CORE)/api \
	$(LOCAL_PATH)/$(T9WRITE_ALPHABETIC_CORE)/api/unix \
	$(LOCAL_PATH)/$(T9WRITE_ALPHABETIC_CORE)/src \
	$(LOCAL_PATH)/$(T9WRITE_CHINESE_CORE)/include \
	$(LOCAL_PATH)/$(T9WRITE_CHINESE_CORE)/api \
	$(LOCAL_PATH)/$(T9WRITE_CHINESE_CORE)/api/unix \

LOCAL_MODULE := libjni_mocakeyboard

# Build Target: Shared Library ("libjni_mocakeyboard.so")
include $(BUILD_SHARED_LIBRARY)
include $(call all-makefiles-under,$(LOCAL_PATH))

