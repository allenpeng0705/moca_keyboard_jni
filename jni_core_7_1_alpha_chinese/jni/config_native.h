
#ifndef __CONFIG_NATIVE_H__
#define __CONFIG_NATIVE_H__

namespace mocainput {

bool config_init(JNIEnv *env, jobject obj, jobject context);

int registerConfigNative(JNIEnv *env);

}

#endif // __CONFIG_NATIVE_H__
