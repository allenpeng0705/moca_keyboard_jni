
#ifndef __LOG_H__
#define __LOG_H__

#ifdef NDK_BUILD
#include <android/log.h> // pulling Android API
#else
#include <cutils/logd.h> // pulling Android API
#endif

#ifdef __cplusplus
extern "C" {
#endif
//
// Wrapping native Android logging API to provide macro for logging.
// LOGD and LOGV will can be turn on and off.  LOGE, LOGW and LOGI will
// always log.  The log messages can be viewed by DDMS tool.
//

#ifndef LOG_TAG
#define LOG_TAG "mocainput"
#endif

// turn on or off debug messages
#define DEBUG_ON

#ifndef DEBUG_ON
#define LOGD(...) ((void)0)
#define LOGV(...) ((void)0)
#else
#define LOGD(...) (LOG(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__))
#define LOGV(...) (LOG(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__))
#endif

#define LOGI(...) (LOG(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__))
#define LOGE(...) (LOG(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__))
#define LOGW(...) (LOG(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__))

#define LOG(priority, tag, ...) log_print(priority, tag, __VA_ARGS__)
#define log_print(priority, tag, fmt...) __android_log_print(priority, tag, fmt)

#ifdef __cplusplus
}
#endif

#endif // __LOG_H__
