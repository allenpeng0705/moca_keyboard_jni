/*
**
*/

#include "Log.h"

#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <jni.h>

#include "xxet9oem.h"
#include "config_native.h"
#include "alpha_native.h"

#ifdef ET9_CHINESE_MODULE
#include "chinese_native.h"
#endif

#ifdef T9WRITE_ALPHA
#include "alpha_write_native.h"
#endif

#ifdef T9WRITE_CHINESE
#include "chinese_write_native.h"
#endif

using namespace mocainput;

// ----------------------------------------------------------------------------

//
// helper function to throw an exception
//
static void throwException(JNIEnv *env, const char* ex, const char* fmt, int data)
{
    if (jclass cls = env->FindClass(ex)) {
        char msg[1000];
        sprintf(msg, fmt, data);
        env->ThrowNew(cls, msg);
        env->DeleteLocalRef(cls);
    }
}


static int registerNatives(JNIEnv *env)
{
	int ret;

	ret = registerConfigNative(env);

	ret = registerAlphaNative(env);


#ifdef ET9_CHINESE_MODULE
	ret = registerChineseNative(env);
#endif

#ifdef T9WRITE_ALPHA
    ret = registerAlphaWriteNative(env);
#endif

#ifdef T9WRITE_CHINESE
    ret = registerChineseWriteNative(env);
#endif

	return ret;
}

/*
 * Returns the JNI version on success, -1 on failure.
 */
extern "C" jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
    JNIEnv* env = NULL;

    chdir("/data/data/com.moca.input/files/");

    if (vm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK) {
        fprintf(stderr, "ERROR: GetEnv failed\n");
        return -1;
    }
    assert(env != NULL);

    if (registerNatives(env) != JNI_TRUE) {
        fprintf(stderr, "ERROR: moca native registration failed\n");
        return -1;
    }

    /* success -- return valid version number */
    return JNI_VERSION_1_4;
}
