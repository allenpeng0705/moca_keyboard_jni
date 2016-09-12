


#include "Log.h"

#include <jni.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "write_alpha.h"
#include "dbregistry.h"
#include "mem_alloc.h"

#include "config_native.h"

namespace mocainput {

//
// TODO: Refactor common code that are use by both Chinese and Alpha.
//
static jobject g_sAlphaWriteContext = NULL;
static struct file_descriptor_field_id_offsets_t{
    jfieldID descriptor;
} sFileDescriptorFieldIDOffsets;


static struct settings_field_id_offset_t {
    jfieldID recognitionMode;	// 0=scrMode,1=mcrMode,2=ucrMode
    jfieldID supportLineSet;	// 0=noSupprtlines,1=toplineOnly,2=baselineOnly,
								// 3=helplineOnly,4=baselineAndHelpline,5=baselineAndTopline
								// 6=helplineAndToline
    jfieldID writingDirection; 	// 0=left,1=right

    jfieldID topline;
    jfieldID helpline;
    jfieldID baseline;
    jfieldID width;
    jfieldID height;

    jfieldID categories; 	// java.lang.list
} sSettingFieldIDOffsets;

static struct listMethodIDOffsets_t {
    jmethodID get;
    jmethodID size;
} sListMethodIDOffsets;

static struct categoryMethodIDOffsets_t {
    jmethodID get;
} sCategoryMethodIDOffsets;

//android.graphics.Point
static struct pointFieldIDOffsets_t {
	jfieldID x;
	jfieldID y;
} sPointFieldIDOffsets;

static void cacheJavaFieldAndClassIDs(JNIEnv* env)
{
    LOGV("cacheJavaFieldAndClassIDs()...");
    jclass clazz;

    // cache android.graphics.Point field IDs
    clazz = env->FindClass("android/graphics/Point");
    sPointFieldIDOffsets.x = env->GetFieldID(clazz, "x", "I");
    sPointFieldIDOffsets.y = env->GetFieldID(clazz, "y", "I");

//    // cache Java FileDescriptor's descriptor for file access later on
//    clazz = env->FindClass("java/io/FileDescriptor");
//    sFileDescriptorFieldIDOffsets.descriptor = env->GetFieldID(clazz, "descriptor", "I");

    // cache Settings fields
    clazz = env->FindClass("com/moca/input/WriteAlphaSettings");
    sSettingFieldIDOffsets.recognitionMode = env->GetFieldID(clazz, "mRecognitionMode", "I");
    sSettingFieldIDOffsets.supportLineSet = env->GetFieldID(clazz, "mSupportLineSet", "I");
    sSettingFieldIDOffsets.writingDirection = env->GetFieldID(clazz, "mWritingDirection", "I");
    sSettingFieldIDOffsets.topline = env->GetFieldID(clazz, "mTopline", "I");
    sSettingFieldIDOffsets.helpline = env->GetFieldID(clazz, "mHelpline", "I");
    sSettingFieldIDOffsets.baseline = env->GetFieldID(clazz, "mBaseline", "I");
    sSettingFieldIDOffsets.width = env->GetFieldID(clazz, "mWidth", "I");
    sSettingFieldIDOffsets.height = env->GetFieldID(clazz, "mHeight", "I");

    sSettingFieldIDOffsets.categories = env->GetFieldID(clazz, "mCategories", "Ljava/util/List;");

    // cache Java List class fields and methods
    clazz = env->FindClass("java/util/List");
    sListMethodIDOffsets.size  = env->GetMethodID(clazz, "size", "()I");
    sListMethodIDOffsets.get   = env->GetMethodID(clazz, "get", "(I)Ljava/lang/Object;");

    // cache Category methods
    clazz = env->FindClass("com/moca/input/WriteAlphaCategory");
    sCategoryMethodIDOffsets.get  = env->GetMethodID(clazz, "get", "()I");

    LOGV("acacheJavaFieldAndClassIDs()...done");
}

static void LogSettings(const DECUMA_SESSION_SETTINGS* settings, DECUMA_INSTANT_GESTURE_SETTINGS* instantSettings)
{
    LOGV("DECUMA_SESSION_SETTINGS");
    LOGV("  recognitionMode = %d", settings->recognitionMode);
    LOGV("  supportLine = %d", settings->supportLineSet);
    LOGV("  baseLine = %d", (int)settings->baseline);
    LOGV("  helpLine = %d", (int)settings->helpline);
    LOGV("  topLine = %d", (int)settings->topline);
    LOGV("  writing direction = %d", settings->writingDirection);
    LOGV("  gesture width threshold = %d", instantSettings->widthThreshold);
    LOGV("  gesture threshold height = %d", instantSettings->heightThreshold);
}

static void readSettings(
    JNIEnv* env,
    jobject jsettings,
    DECUMA_SESSION_SETTINGS* settings,
    DECUMA_INSTANT_GESTURE_SETTINGS* instantSettings)
{
    LOGV("readSettings()...");

    settings->recognitionMode = (RECOGNITION_MODE)env->GetIntField(jsettings, sSettingFieldIDOffsets.recognitionMode);
    settings->supportLineSet = (SUPPORT_LINE_SET)env->GetIntField(jsettings, sSettingFieldIDOffsets.supportLineSet);
    settings->writingDirection = (WRITING_DIRECTION)env->GetIntField(jsettings, sSettingFieldIDOffsets.writingDirection);
    settings->topline = env->GetIntField(jsettings, sSettingFieldIDOffsets.topline);
    settings->helpline = env->GetIntField(jsettings, sSettingFieldIDOffsets.helpline);
    settings->baseline = env->GetIntField(jsettings, sSettingFieldIDOffsets.baseline);

    instantSettings->widthThreshold  = (int)((env->GetIntField(jsettings, sSettingFieldIDOffsets.width) * 3) / 4);
    instantSettings->heightThreshold = (int)(env->GetIntField(jsettings, sSettingFieldIDOffsets.height) / 4);

    // categories
    jobject categoryListObject = env->GetObjectField(jsettings, sSettingFieldIDOffsets.categories);
    int categoryCount  = env->CallIntMethod(categoryListObject, sListMethodIDOffsets.size);

    if (settings->charSet.nSymbolCategories < categoryCount) {
    	FREE(settings->charSet.pSymbolCategories);
    	settings->charSet.pSymbolCategories = (DECUMA_UINT32*)CALLOC(sizeof(DECUMA_UINT32), categoryCount);
    }
    settings->charSet.nSymbolCategories = categoryCount;
    for (int i = 0; i < categoryCount; i++) {
        jobject categoryObject = env->CallObjectMethod(categoryListObject, sListMethodIDOffsets.get, i);
        int cat = env->CallIntMethod(categoryObject, sCategoryMethodIDOffsets.get);
        settings->charSet.pSymbolCategories[i] = cat;

        env->DeleteLocalRef(categoryObject);
    }

    LogSettings(settings, instantSettings);
    LOGV("readSettings()...done");
}

static jstring Write_Alpha_getVersion_impl(
    JNIEnv*  env,
    jobject  jthis,
    jint context)
{
    LOGV("Write_Alpha_getVersion()...");

    Write_Alpha* session = reinterpret_cast<Write_Alpha*>(context);
    if (session == 0) {
        return 0;
    }

    LOGV("Write_Alpha_getVersion()...%s", session->getVersion());
    return env->NewStringUTF(session->getVersion());
}

static jstring Write_Alpha_getDatabaseVersion_impl(
    JNIEnv*  env,
    jobject  jthis,
    jint context)
{
    LOGV("Write_Alpha_getDatabaseVersion()...");

    Write_Alpha* session = reinterpret_cast<Write_Alpha*>(context);
    if (session == 0) {
        return 0;
    }

    LOGV("Write_Alpha_getDatabaseVersion()...%s", session->getDatabaseVersion());
    return env->NewStringUTF(session->getDatabaseVersion());
}

static jlong Write_Alpha_create_impl(JNIEnv*  env, jobject jthis, jstring jsDatabaseConfigFile)
{
    LOGV("Write_Alpha_create()...");

    const char* databaseConfigFilePath = 0;
    if (jsDatabaseConfigFile != 0) {
        databaseConfigFilePath = (const char*)env->GetStringUTFChars(jsDatabaseConfigFile, 0);
    }

    DBRegistry* dbRegistry = DBRegistry::getInstance(databaseConfigFilePath);
    Write_Alpha* session = new Write_Alpha(dbRegistry, alpha_data::getInstance(dbRegistry));

    if (jsDatabaseConfigFile != 0) {
        env->ReleaseStringUTFChars(jsDatabaseConfigFile, databaseConfigFilePath);
    }

    LOGV("Write_Alpha_create(databaseConfigFile = %s)...%p", databaseConfigFilePath, session);

    // return cookie. It will be cast back to object every time
    return (jlong)session;
}

static jint Write_Alpha_destroy_impl(
    JNIEnv*  env,
    jobject  jthis,
    jlong     context)
{
    LOGV("Write_Alpha_destroy()...");

    Write_Alpha* session  = reinterpret_cast<Write_Alpha*>(context);
    if (session) {
        delete session;
    }

    DBRegistry::deleteInstance();
    alpha_data::deleteInstance();

    LOGV("Write_Alpha_destroy()...done");
    return decumaNoError;
}

static jint Write_Alpha_changeSettings_impl(
    JNIEnv*  env,
    jobject  jthis,
    jlong     context,
    jobject jsettings)
{
    LOGV("Write_Alpha_setSetting()...");

    Write_Alpha* session  = reinterpret_cast<Write_Alpha*>(context);
    if (session == 0) {
        return decumaNullSessionPointer;
    }

    readSettings(env, jsettings, session->mSessionSettings, &session->mInstantGestureSettings);
    session->applySettingChanges();

    LOGV("Write_Alpha_setSetting()...done");
    return decumaNoError;
}

static jint Write_Alpha_start_impl(
    JNIEnv* env,
    jobject jthis,
    jlong context,
    jobject jsettings,
    jobject androidContext,
    jint languageID)
{
    LOGV("Write_Alpha_start()...");

    Write_Alpha* session = reinterpret_cast<Write_Alpha*>(context);
    if (session == 0) {
        return decumaNullSessionPointer;
    }
    g_sAlphaWriteContext = androidContext;
    if (! config_init(env, jthis, g_sAlphaWriteContext)) {
		LOGD("config_init(context(0x%X))...auth failed", context);
		return decumaNullSessionPointer;
	}

    readSettings(env, jsettings, session->mSessionSettings, &session->mInstantGestureSettings);
    int status = session->start(languageID);

    LOGV("Write_Alpha_start()...%d", status);

    return status;
}

static jint Write_Alpha_finish_impl(
    JNIEnv* env,
    jobject jthis,
    jlong context)
{
    LOGV("Write_Alpha_finish()...");

    Write_Alpha* session  = reinterpret_cast<Write_Alpha*>(context);
    if (session == 0) {
        return decumaNullSessionPointer;
    }

    int status =  session->finish();
	g_sAlphaWriteContext = NULL;
    LOGV("Write_Alpha_finish()...%d", status);

    return status;
}

static jint Write_Alpha_beginArc_impl(
    JNIEnv*  env,
    jobject  jthis,
    jlong     context)
{
    LOGV("Write_Alpha_beginArc()...");

    Write_Alpha* session  = reinterpret_cast<Write_Alpha*>(context);
    if (session == 0) {
        return decumaNullSessionPointer;
    }
    if (! config_init(env, jthis, g_sAlphaWriteContext)) {
		LOGD("config_init(context(0x%X))...auth failed", context);
		return decumaNullSessionPointer;
	}

    int status =  session->beginArc();

    LOGV("Write_Alpha_begingArc()...%d", status);

    return status;
}

static int Write_Alpha_addPoints(
	JNIEnv*  env,
	Write_Alpha* session,
    jobject  listOfPoints,
    int arcId)
{
	LOGV("Write_Alpha_addPoints()...");

	int status = 0;
	int pointAdded = 0;
    int count = env->CallIntMethod(listOfPoints, sListMethodIDOffsets.size);

    for (pointAdded = 0; pointAdded < count && pointAdded < Write_Alpha::MAX_POINTS; pointAdded++) {
        jobject point = env->CallObjectMethod(listOfPoints, sListMethodIDOffsets.get, pointAdded);
        int x = env->GetIntField(point, sPointFieldIDOffsets.x);
        int y = env->GetIntField(point, sPointFieldIDOffsets.y);
        if ((status = session->addPoint(arcId, x, y)) != 0) {
            break;
        }
        env->DeleteLocalRef(point);
    }

    LOGV("Write_Alpha_addPoints()...status = %d, points added = %d", status, pointAdded);

    return pointAdded;
}

static jint Write_Alpha_addArc_impl (
    JNIEnv*  env,
    jobject  jthis,
    jlong     context,
    jobject  listOfPoints1,
    jobject  listOfPoints2,
    jintArray arrayInstantGesture)
{
    LOGV("Write_Alpha_addArc()...");

    Write_Alpha* session  = reinterpret_cast<Write_Alpha*>(context);
    if (session == 0) {
        return decumaNullSessionPointer;
    }
    if (! config_init(env, jthis, g_sAlphaWriteContext)) {
		LOGD("config_init(context(0x%X))...auth failed", context);
		return decumaNullSessionPointer;
	}

    int status = 0;
    int arcId1 = 0;
    int arcId2 = 0;

    if (listOfPoints1 != 0) {
    	if (env->CallIntMethod(listOfPoints1, sListMethodIDOffsets.size) > 0) {
    		status = session->startNewArc(arcId1);
			Write_Alpha_addPoints(env, session, listOfPoints1, arcId1);
    	}
    }

    if (listOfPoints2 != 0) {
    	if (env->CallIntMethod(listOfPoints2, sListMethodIDOffsets.size) > 0) {
    		status = session->startNewArc(arcId2);
			Write_Alpha_addPoints(env, session, listOfPoints2, arcId2);
    	}
    }

    // for multi-touch gesturing, these commits have to called last

    if (arcId1 != 0) {
    	status = session->CommitArc(arcId1);
    }

    if (arcId2 != 0) {
    	status = session->CommitArc(arcId2);
    }

    jint*  instantGesture = env->GetIntArrayElements(arrayInstantGesture, 0);
    instantGesture[0] = session->getInstantGesture();
    env->ReleaseIntArrayElements(arrayInstantGesture, instantGesture, 0);

    LOGV("Write_Alpha_addArc()...%d", status);

    return status;
}

static jint Write_Alpha_endArc_impl(
    JNIEnv*  env,
    jobject  jthis,
    jlong     context)
{
    LOGV("Write_Alpha_endArc()...");

    Write_Alpha* session  = reinterpret_cast<Write_Alpha*>(context);

    if (session == 0) {
        return decumaNullSessionPointer;
    }
    if (! config_init(env, jthis, g_sAlphaWriteContext)) {
		LOGD("config_init(context(0x%X))...auth failed", context);
		return decumaNullSessionPointer;
	}

    int status = session->endArc();

    LOGV("Write_Alpha_endArc()...%d", status);

    return status;
}

static jint Write_Alpha_recognize_impl(
    JNIEnv*  env,
    jobject  jthis,
    jlong     context,
    jcharArray arrayStartWord,
    jintArray  arrayRecognizeCount)
{
    LOGV("Write_Alpha_recognize()...");

    Write_Alpha* session  = reinterpret_cast<Write_Alpha*>(context);
    if (session == 0) {
        return decumaNullSessionPointer;
    }
    if (! config_init(env, jthis, g_sAlphaWriteContext)) {
		LOGD("config_init(context(0x%X))...auth failed", context);
		return decumaNullSessionPointer;
	}

    jchar* startWord = env->GetCharArrayElements(arrayStartWord, 0);
    jint*  recognizedCount = env->GetIntArrayElements(arrayRecognizeCount, 0);
    int status = session->recognize(startWord, recognizedCount[0]);
    env->ReleaseCharArrayElements(arrayStartWord, startWord, 0);
    env->ReleaseIntArrayElements(arrayRecognizeCount, recognizedCount, 0);

    LOGV("Write_Alpha_recognize()...%d", status);

    return status;
}

static jint Write_Alpha_getRecognitionCandidate_impl(
	    JNIEnv*  env,
	    jobject  jthis,
	    jlong     context,
	    jint	index,
	    jcharArray arrayWord,
	    jint maxLength,
	    jintArray arrayLength,
	    jintArray arrayInstantGesture)
{
	Write_Alpha* session  = reinterpret_cast<Write_Alpha*>(context);
    if (session == 0) {
        return decumaNullSessionPointer;
    }
    if (! config_init(env, jthis, g_sAlphaWriteContext)) {
		LOGD("config_init(context(0x%X))...auth failed", context);
		return decumaNullSessionPointer;
	}

    jint*  wordLength = env->GetIntArrayElements(arrayLength, 0);
	jchar* word = env->GetCharArrayElements(arrayWord, 0);
	jint * instantGesture = env->GetIntArrayElements(arrayInstantGesture, 0);

	int status = session->getCandidate(index, word, maxLength, wordLength[0], instantGesture[0]);

	env->ReleaseIntArrayElements(arrayLength, wordLength, 0);
	env->ReleaseCharArrayElements(arrayWord, word, 0);
	env->ReleaseIntArrayElements(arrayInstantGesture, instantGesture, 0);

	return status;
}

static int Write_Alpha_addWordToUserDictionary_impl (
	    JNIEnv*  env,
	    jobject  jthis,
	    jlong     context,
	    jcharArray arrayWord,
	    jint length)
{
	Write_Alpha* session  = reinterpret_cast<Write_Alpha*>(context);
    if (session == 0) {
        return decumaNullSessionPointer;
    }
    if (! config_init(env, jthis, g_sAlphaWriteContext)) {
		LOGD("config_init(context(0x%X))...auth failed", context);
		return decumaNullSessionPointer;
	}

	jchar* word = env->GetCharArrayElements(arrayWord, 0);
	int len = length;
	int status;

	status =  session->addWordToUserDictionary((const unsigned short*)word, (int)len);

	env->ReleaseCharArrayElements(arrayWord, word, 0);

	return status;

}

static int Write_Alpha_noteSelectedCandidate_impl(
	    JNIEnv*  env,
	    jobject  jthis,
	    jlong     context,
	    jint	index)
{
	Write_Alpha* session  = reinterpret_cast<Write_Alpha*>(context);
    if (session == 0) {
        return decumaNullSessionPointer;
    }
    if (! config_init(env, jthis, g_sAlphaWriteContext)) {
		LOGD("config_init(context(0x%X))...auth failed", context);
		return decumaNullSessionPointer;
	}

    return session->noteSelectedCandidate(index);
}

static JNINativeMethod gAlphaWriteMethods[] =
{
   {"Write_Alpha_create",  "(Ljava/lang/String;)J",                                  (void*)Write_Alpha_create_impl},
   {"Write_Alpha_destroy", "(J)I",                                 (void*)Write_Alpha_destroy_impl},
   {"Write_Alpha_changeSettings",   "(JLcom/moca/input/WriteAlphaSettings;)I",   (void*)Write_Alpha_changeSettings_impl},
   {"Write_Alpha_start",   "(JLcom/moca/input/WriteAlphaSettings;Landroid/content/Context;I)I", (void*)Write_Alpha_start_impl},
   {"Write_Alpha_finish",  "(J)I",                                 (void*)Write_Alpha_finish_impl},
   {"Write_Alpha_getVersion",        "(J)Ljava/lang/String;",    (void*)Write_Alpha_getVersion_impl},
   {"Write_Alpha_getDatabaseVersion", "(J)Ljava/lang/String;",    (void*)Write_Alpha_getDatabaseVersion_impl},
   {"Write_Alpha_beginArc",       "(J)I",                         (void*)Write_Alpha_beginArc_impl},
   {"Write_Alpha_addArc",        "(JLjava/util/List;Ljava/util/List;[I)I", (void*)Write_Alpha_addArc_impl},
   {"Write_Alpha_endArc",         "(J)I",                         (void*)Write_Alpha_endArc_impl},
   {"Write_Alpha_recognize",      "(J[C[I)I",                        (void*)Write_Alpha_recognize_impl},
   {"Write_Alpha_getRecognitionCandidate", "(JI[CI[I[I)I", (void*)Write_Alpha_getRecognitionCandidate_impl},
   {"Write_Alpha_noteSelectedCandidate",   "(JI)I",	(void*)Write_Alpha_noteSelectedCandidate_impl},
   {"Write_Alpha_addWordToUserDictionary", "(J[CI)I", (void*)Write_Alpha_addWordToUserDictionary_impl},

};

int registerAlphaWriteNative(JNIEnv *env)
{
    const char* const kAlphaWriteClassPathName = "com/moca/input/NativeWriteAlpha";
    jclass clazz;

    clazz = env->FindClass(kAlphaWriteClassPathName);
    if (clazz == NULL) {
        fprintf(stderr,
            "Alpha Write Native registration unable to find class '%s'\n", kAlphaWriteClassPathName);
        return JNI_FALSE;
    }
    if (env->RegisterNatives(clazz, gAlphaWriteMethods, sizeof(gAlphaWriteMethods) / sizeof(gAlphaWriteMethods[0])) < 0) {
        fprintf(stderr, "Alpha Write Register Natives failed for '%s'\n", kAlphaWriteClassPathName);
        return JNI_FALSE;
    }

    // Caching field and method IDs
    cacheJavaFieldAndClassIDs(env);

    return JNI_TRUE;
}

} //  mocainput namespace


