LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := libt9write_chinese

LOCAL_SRC_FILES := t9write_rel.c

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH) \
	$(LOCAL_PATH)/../../include \
	$(LOCAL_PATH)/api \
	$(LOCAL_PATH)/src \

MY_LOCAL_CFLAGS := -finline-limit=300
LOCAL_CFLAGS += -Wfatal-errors $(MY_LOCAL_CFLAGS) -DT9WRITE_CJK -DDECUMA_HOSTED

LOCAL_PRELINK_MODULE := false
# Build Target: Shared Library ("libt9write_chinese.a")
include $(BUILD_STATIC_LIBRARY)
