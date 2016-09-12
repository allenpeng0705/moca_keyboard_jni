LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

# build chinese core if set to true
INCLUDE_CHINESE := true

CORE_HEADERS := \
	$(LOCAL_PATH)/generic/core/et9api \
	$(LOCAL_PATH)/generic/core/et9input/et9imu \
	$(LOCAL_PATH)/generic/core/et9input/et9kdb \
	$(LOCAL_PATH)/generic/core/et9util \
	$(LOCAL_PATH)/alphabetic/core/et9API \
	$(LOCAL_PATH)/alphabetic/core/et9input/et9imu \
	$(LOCAL_PATH)/alphabetic/core/et9ling/et9ldb \
	$(LOCAL_PATH)/alphabetic/core/et9ling/et9list \
	$(LOCAL_PATH)/alphabetic/core/et9ling/et9sdb \
	$(LOCAL_PATH)/alphabetic/core/et9util

#
# Pulling Chinese source code and header files
#
ifeq ($(INCLUDE_CHINESE),true)
LOCAL_CFLAGS += -DET9_CHINESE_MODULE

CORE_HEADERS += \
	$(LOCAL_PATH)/chinese/core/et9cprel \
	$(LOCAL_PATH)/chinese/core/et9cpgrel
endif

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH) \
	$(LOCAL_PATH)/../../include \
	$(CORE_HEADERS)

## et9rel.c will also include core if ET9_CHINESE_MODULE is defined - above
LOCAL_SRC_FILES := \
	generic/core/et9api/et9rel.c	

LOCAL_OPTIMIZATION_FLAGS := \
	-Winline \
	-finline-functions \
	-finline-functions-called-once \
	-finline-limit=4294967296 \
	-fomit-frame-pointer \
	-O3 \

LOCAL_CFLAGS += -Wfatal-errors -nostdlib $(LOCAL_OPTIMIZATION_FLAGS)
LOCAL_PRELINK_MODULE := false
LOCAL_MODULE := libxt9core
include $(BUILD_STATIC_LIBRARY)

