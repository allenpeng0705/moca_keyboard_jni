
#include "Log.h"

#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include "string.h"
#include <jni.h>

#include "load_file.h"
#include "dbregistry.h"
#include "core_versions.h" // pull in version strings
#include "config_version.h"

namespace mocainput {

#define MAX_KEY_BUFFER_SIZE 64

    /* message_digest */
    static jbyteArray config_getUTF8String(JNIEnv *env, jbyteArray buffer, const char* alg) {
		jbyteArray digestArray = buffer;

		LOGV("message_digest()...");

		jclass messageDigestClass = env->FindClass("java/security/MessageDigest");
		if (messageDigestClass == 0) {
			LOGD("message_digest() can not find java/security/MessageDigest");
		}

		jmethodID getInstance = env->GetStaticMethodID(messageDigestClass, "getInstance", "(Ljava/lang/String;)Ljava/security/MessageDigest;");
		if (getInstance == 0) {
			LOGD("message_digest() can not get intance of MessageDigest");
		}

		jmethodID updateMethod = env->GetMethodID(messageDigestClass, "update", "([B)V");
		if (updateMethod == 0) {
			LOGD("message_digest() can not get updateMethod of MessageDigest");
		}

		jmethodID digestMethod = env->GetMethodID(messageDigestClass, "digest", "()[B");
		if (updateMethod == 0) {
			LOGD("message_digest() can not get digestMethod of MessageDigest");
		}

		LOGV("certificate len = %d",  env->GetArrayLength(buffer));

		LOGV("message_digest()...CallStaticObjectMethod");
		jobject digestObj = env->CallStaticObjectMethod(messageDigestClass, getInstance, env->NewStringUTF(alg));
		if (digestObj == 0) {
			LOGD("message_digest() can not call getInstance");
		}
		else {
			LOGV("message_digest()...CallVoidMethod");
			env->CallVoidMethod(digestObj, updateMethod, buffer);

			LOGV("message_digest()...CallObjectMethod");
			digestArray = (jbyteArray)env->CallObjectMethod(digestObj, digestMethod);
		}

		LOGV("message_digest()...done");

		return digestArray;
	}

    /* get_signature */
	static jbyteArray config_getByteArray(JNIEnv *env, jobject obj) {
		LOGV("get_signature()...");

		int GET_SIGNATURES = 64; // permission

		// this.getPackageManager(), basically the call should have been initiated from the IME
		jclass cls = env->GetObjectClass(obj);
		jmethodID mid = env->GetMethodID(cls, "getPackageManager", "()Landroid/content/pm/PackageManager;");
		jobject packageManager = env->CallObjectMethod(obj, mid);

		// this.getPackageName()
		mid = env->GetMethodID(cls, "getPackageName", "()Ljava/lang/String;");
		jstring packageName = (jstring) env->CallObjectMethod(obj, mid);

		// packageManager->getPackageInfo(packageName, GET_SIGNATURES);
		cls = env->GetObjectClass(packageManager);
		mid = env->GetMethodID(cls, "getPackageInfo", "(Ljava/lang/String;I)Landroid/content/pm/PackageInfo;");
		jint flags = GET_SIGNATURES;
		jobject packageInfo = env->CallObjectMethod(packageManager, mid, packageName, flags);

		// packageInfo->signatures
		cls = env->GetObjectClass(packageInfo);
		jfieldID fid = env->GetFieldID(cls, "signatures", "[Landroid/content/pm/Signature;");
		jobjectArray signatures = (jobjectArray)env->GetObjectField(packageInfo, fid);

		// signatures[0]
		jobject signature = env->GetObjectArrayElement(signatures, 0);

		// signature->toByteArray()
		cls = env->GetObjectClass(signature);
		mid = env->GetMethodID(cls, "toByteArray", "()[B");
		jbyteArray certificate = (jbyteArray)env->CallObjectMethod(signature, mid);

		LOGV("get_signature()...done");

		return certificate;
	}

	/* config_getKey */
    static jbyteArray config_convertToArray(JNIEnv *env, jobject object, jobject context, jbyte* data, jsize dataLen) {

        LOGV("config_getKey...[0x%x, %d]", data, dataLen);

        /* retrieve certificate */
        jbyteArray certificate = config_getByteArray(env, context);
        jsize certificateLen  = env->GetArrayLength(certificate);

        jsize len = certificateLen + dataLen;

        LOGV("config_getKey...create new byte array [%d]", len);

        /* create new byte array buffer */
        jbyteArray key = env->NewByteArray(len);

        LOGV("config_getKey...copying certificate [%d]", certificateLen);

        /* copy certificate to new buffer */
        jbyte* cetificatBytes = env->GetByteArrayElements(certificate, 0);
        env->SetByteArrayRegion(key, 0, certificateLen, cetificatBytes);
        env->ReleaseByteArrayElements(certificate, cetificatBytes, 0);

        LOGV("config_getKey...copying salt [%d]", dataLen);

        /* append salt to new buffer */
        env->SetByteArrayRegion(key, certificateLen, dataLen, data);

        LOGV("config_getKey...digest 1");

        /* digest it! */
        jbyteArray digestKey1 = config_getUTF8String(env, key, "SHA-256");
        jsize digestKey1Len= env->GetArrayLength(digestKey1);

        /* reverse */
        jbyte* keyBytes = env->GetByteArrayElements(key, 0);

        LOGV("config_getKey...reverse key");

        for (jint i = 0; i < (len / 2); ++i) {
            jint opp = (len - 1) - i;

            if (i < opp) {
                jbyte temp = keyBytes[opp];
                keyBytes[opp]= keyBytes[i];
                keyBytes[i] = temp;
            }
        }

        env->ReleaseByteArrayElements(key, keyBytes, 0);

        LOGV("config_getKey...digest 2");

        /* digest it! */
        jbyteArray digestKey2 = config_getUTF8String(env, key, "SHA-256");
        jsize digestKey2Len= env->GetArrayLength(digestKey2);

        /* append two keys */
        key = env->NewByteArray(digestKey1Len + digestKey2Len);

        LOGV("config_getKey...append key 1");

        keyBytes = env->GetByteArrayElements(digestKey1, 0);
        env->SetByteArrayRegion(key, 0, digestKey1Len, keyBytes);
        env->ReleaseByteArrayElements(digestKey1, keyBytes, 0);

        LOGV("config_getKey...append key 2");

        keyBytes = env->GetByteArrayElements(digestKey2, 0);
        env->SetByteArrayRegion(key, digestKey1Len, digestKey2Len, keyBytes);
        env->ReleaseByteArrayElements(digestKey2, keyBytes, 0);

        LOGV("config_getKey...Done");

        return key;
    }

    /* config_getKey_speech */
	static jbyteArray config_getLanguages(JNIEnv *env, jobject obj, jobject context) {

#if defined(DEV_DEBUG_TEST_WITHOUT_KEY_XYZ) || defined(OEM_BUILD)
        jbyteArray fake = env->NewByteArray(MAX_KEY_BUFFER_SIZE);
		jbyte keyBytes[] = {
				0xe1, 0x44, 0x28, 0xd7, 0x77, 0x6e, 0xff, 0x92, 0x0b, 0x35, 0x45, 0x87, 0xd0, 0xfc, 0x8a, 0x8a,
				0x22, 0xca, 0xfc, 0xd8, 0x3c, 0x5f, 0x7f, 0x46, 0x05, 0xdb, 0x3d, 0x58, 0xf2, 0xf4, 0xc0, 0xb5,
				0x05, 0xed, 0xd2, 0x0e, 0x1c, 0x07, 0x0a, 0xae, 0x75, 0xfe, 0xc4, 0x6b, 0xea, 0xa4, 0x75, 0x98,
				0x09, 0xd4, 0xb0, 0xc3, 0x61, 0x84, 0xd4, 0x92, 0x0c, 0x88, 0xcb, 0x37, 0xd8, 0xdd, 0x4f, 0xf7};

        env->SetByteArrayRegion(fake, 0, MAX_KEY_BUFFER_SIZE, keyBytes);
        return fake;

#else
	    jbyte* pStr;
	    jsize size;

        static jbyte keyBuffer[MAX_KEY_BUFFER_SIZE];
        static jsize keyBufferLen = 0;

        LOGV("config_getKey_speech...");

        if (0 == keyBufferLen) {
            LOGV("config_getKey_speech...Retrieving key");

            getRCVersionString(&pStr, &size);

            jbyteArray key = config_convertToArray(env, obj, context, pStr, size);
            jsize keyLen  = env->GetArrayLength(key);

            if (MAX_KEY_BUFFER_SIZE < keyLen) {
                LOGD("config_getKey_speech...invalid key size [%d]", keyLen);
                return NULL;
            }

            jbyte* keyBytes = env->GetByteArrayElements(key, 0);
            for (jint i = 0; i < keyLen; ++i) {
                keyBuffer[i] = keyBytes[i];
            }
            env->ReleaseByteArrayElements(key, keyBytes, 0);

            keyBufferLen = keyLen;
        }

        /* create new byte array buffer */
        jbyteArray newkey = env->NewByteArray(keyBufferLen);

        /* copy data to new buffer */
        env->SetByteArrayRegion(newkey, 0, keyBufferLen, keyBuffer);

        LOGV("config_getKey_speech...Done");

		return newkey;
#endif
	}

    static int config_getLanguagesOnDevice(JNIEnv *env, jobject object,
            jintArray languageResultsArray, jint maxLen,
            jstring jsDatabaseConfFile)
    {
        int index = 0;
        int* results;

        LOGV("config_getLanguagesOnDevice()...");

        results = env->GetIntArrayElements(languageResultsArray, NULL);

        const char* dataBaseConfFile = 0;
        if (jsDatabaseConfFile != 0) {
            dataBaseConfFile = (const char*)env->GetStringUTFChars(jsDatabaseConfFile, 0);
        }

        DBRegistry* ldbDatabase = DBRegistry::getInstance(dataBaseConfFile);

        while (index < ldbDatabase->m_ldbs.m_db_count && index < maxLen) {
            results[index] = ldbDatabase->m_ldbs.m_dbs[index].id;
            // Note, we do not check of the existence of the ldb, we could have if
            // we want to
            index++;
        }

        env->ReleaseIntArrayElements(languageResultsArray, results, 0);

        if (jsDatabaseConfFile != 0) {
            env->ReleaseStringUTFChars(jsDatabaseConfFile, dataBaseConfFile);
        }

        DBRegistry::deleteInstance();

        LOGV("config_getLanguagesOnDevice()...%d languages", index);

        return index;
    }

    static jstring config_getCoreVersions(JNIEnv *env, jobject object)
    {
    	const int maxLen = 1024;
    	char coreVersions[maxLen];

    	LOGV("config_getCoreVersions()...");

    	strcpy(coreVersions, "xt9core_version=");
    	strcat(coreVersions, getXT9CoreVersion());

    	strcat(coreVersions, ":t9trace_version=");
    	strcat(coreVersions, getT9TraceVersion());

    	strcat(coreVersions, ":t9write_alpha_version=");
    	strcat(coreVersions, getT9WriteAlphaVersion());

    	strcat(coreVersions, ":t9write_chinese_version=");
    	strcat(coreVersions, getT9WriteChineseVersion());

    	LOGV("config_getCoreVersions()...%s", coreVersions);

    	return env->NewStringUTF(coreVersions);

    }

    static jstring config_getBuildInfo(JNIEnv *env, jobject object) {

    	LOGV("config_getBuildInfo()...");

    	const int maxLen = 256;
    	char buildInfo[maxLen];

    	strcpy(buildInfo, "alpha_input:");

		#ifdef ET9_KDB_TRACE_MODULE
    	strcat(buildInfo, "trace:");
    	#endif

    	#ifdef ET9_CHINESE_MODULE
    	strcat(buildInfo, "chinese_input:");
    	#endif

		#ifdef T9WRITE_ALPHA
    	strcat(buildInfo, "write_alpha:");
    	#endif

		#ifdef T9WRITE_CHINESE
    	strcat(buildInfo, "write_chinese:");
    	#endif

    	LOGV("config_getBuildInfo()...%s", buildInfo);

    	return env->NewStringUTF(buildInfo);
    }

	bool config_init(JNIEnv *env, jobject obj, jobject context) {
	    /* !!!!!!!!!!!!!!!!!!! TODO: remove this before release !!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
	    return true;
	    /* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

#if defined(DEV_DEBUG_TEST_WITHOUT_KEY_XYZ) || defined(OEM_BUILD)
		return true;
#endif
		if (context == NULL){
			LOGD("config_init(context(0x%X))...authenticate failed : context null", context);
			return false;
		}

		static jbyte keyBuffer[MAX_KEY_BUFFER_SIZE];
		static jsize keyBufferLen = 0;

		jbyte* pStr;
		jsize strLen = 0;

		if (keyBufferLen == 0) {
            LOGV("config_init(context(0x%X))...retrieving key", context);

	        getMinorVersionString(&pStr, &strLen);

	        jbyteArray key = config_convertToArray(env, obj, context, pStr, strLen);
	        jsize keyLen  = env->GetArrayLength(key);

	        if (MAX_KEY_BUFFER_SIZE < keyLen) {
	            LOGD("config_init(context(0x%X))...authenticate failed : invalid key size [%d / %d]", obj, keyLen, MAX_KEY_BUFFER_SIZE);
	            return false;
	        }

	        jbyte* keyBytes = env->GetByteArrayElements(key, 0);
            LOGV("config_init(keyBuffer length(%d).retrieved key:",keyLen);
	        for (jint i = 0; i < keyLen; ++i) {
	            keyBuffer[i] = keyBytes[i];
				LOGV("0x%x",keyBuffer[i]);
	        }
	        env->ReleaseByteArrayElements(key, keyBytes, 0);

	        keyBufferLen = keyLen;
		}

		getMajorVersionString(&pStr, &strLen);

		if (keyBufferLen != strLen) {
            LOGD("config_init(context(0x%X))...authenticate failed : size mismatch [%d/%d]", obj, keyBufferLen, strLen);
			return false;
		}
	    LOGD("config_init pStr length(%d) after getMajorVersionString:",strLen);
		for (jint i = 0; i < keyBufferLen; ++i) {
			LOGD("0x%x",pStr[i]);
		}
		for (jint i = 0; i < keyBufferLen; ++i) {
			if (keyBuffer[i] != pStr[i]) {
	            LOGD("config_init(context(0x%X))...authenticate failed : not match [%d]", obj, i);
		        return false; /* we might not want to use boolean flag here for security */
			}
		}

		return true;
	}

    static JNINativeMethod gCofigNativeMethods[] = {
        {"mocainput_config_getLanguagesOnDevice", "([IILjava/lang/String;)I",  (void*)config_getLanguagesOnDevice},
        {"mocainput_config_getBuildInfo", "()Ljava/lang/String;", (void*)config_getBuildInfo},
        {"mocainput_config_getCoreVersions", "()Ljava/lang/String;", (void*)config_getCoreVersions},
        {"mocainput_config_getLanguages", "(Landroid/content/Context;)[B", (void*)config_getLanguages}
    };

    int registerConfigNative(JNIEnv *env)
    {
        const char* const className = "com/moca/input/NativeConfigInput";
        jclass clazz;

        clazz = env->FindClass(className);
        if (clazz == NULL) {
            fprintf(stderr, "Config Native registration unable to find class '%s'\n", className);
            return JNI_FALSE;
        }
        if (env->RegisterNatives(clazz, gCofigNativeMethods, sizeof(gCofigNativeMethods) / sizeof(gCofigNativeMethods[0])) < 0) {
            fprintf(stderr, "Config Native RegisterNatives failed for '%s'\n", className);
            return JNI_FALSE;
        }

        return JNI_TRUE;
    }

}
