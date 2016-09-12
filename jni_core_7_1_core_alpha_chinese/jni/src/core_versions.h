
#ifndef __CORE_VERSION_H__
#define __CORE_VERSION_H__

#include "et9api.h"

#if defined(__cplusplus)
extern "C" {
#endif

const char* getXT9CoreVersion();
const char* getT9TraceVersion();
const char* getT9WriteAlphaVersion();
const char* getT9WriteChineseVersion();

#if defined(__cplusplus)
}
#endif

#endif
