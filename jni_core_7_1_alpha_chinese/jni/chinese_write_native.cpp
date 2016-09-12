


#include "Log.h"

#include <jni.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>

#include "write_chinese.h"

#include "config_native.h"

//
// TODO: Refactor common code that are use by both Chinese and Alpha.
//

namespace mocainput {
static jobject g_sCnWriteContext = NULL;

static struct chinese_settings_field_id_offset_t {
    jfieldID recognitionMode;	// 0=scrMode,1=mcrMode,2=ucrMode
    jfieldID supportLineSet;	// 0=noSupprtlines,1=toplineOnly,2=baselineOnly,
								// 3=helplineOnly,4=baselineAndHelpline,5=baselineAndTopline
								// 6=helplineAndToline
    jfieldID writingDirection; 	// 0=left,1=right
    jfieldID inputGuide;		// 0=none,1=support lines,2=box
    jfieldID topline;
    jfieldID helpline;
    jfieldID baseline;
    jfieldID width;
    jfieldID height;

    jfieldID categories; 	// java.lang.list
} sChineseSettingFieldIDOffsets;

static struct listMethodIDOffsets_t {
    jmethodID get;
    jmethodID size;
} sListMethodIDOffsets;

static struct chinese_categoryMethodIDOffsets_t {
    jmethodID get;
} sChineseCategoryMethodIDOffsets;

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

    // cache Settings fields
    clazz = env->FindClass("com/moca/input/WriteChineseSettings");
    sChineseSettingFieldIDOffsets.recognitionMode = env->GetFieldID(clazz, "mRecognitionMode", "I");
    sChineseSettingFieldIDOffsets.supportLineSet = env->GetFieldID(clazz, "mSupportLineSet", "I");
    sChineseSettingFieldIDOffsets.writingDirection = env->GetFieldID(clazz, "mWritingDirection", "I");
    sChineseSettingFieldIDOffsets.topline = env->GetFieldID(clazz, "mTopline", "I");
    sChineseSettingFieldIDOffsets.helpline = env->GetFieldID(clazz, "mHelpline", "I");
    sChineseSettingFieldIDOffsets.baseline = env->GetFieldID(clazz, "mBaseline", "I");
    sChineseSettingFieldIDOffsets.width = env->GetFieldID(clazz, "mWidth", "I");
    sChineseSettingFieldIDOffsets.height = env->GetFieldID(clazz, "mHeight", "I");
    sChineseSettingFieldIDOffsets.inputGuide = env->GetFieldID(clazz, "mInputGuide", "I");
    sChineseSettingFieldIDOffsets.categories = env->GetFieldID(clazz, "mCategories", "Ljava/util/List;");

    LOGV("sChineseSettingFieldIDOffsets.recognitionMode = %d", sChineseSettingFieldIDOffsets.recognitionMode);
    LOGV("sChineseSettingFieldIDOffsets.supportLineSet = %d", sChineseSettingFieldIDOffsets.supportLineSet);
    LOGV("sChineseSettingFieldIDOffsets.writingDirection = %d", sChineseSettingFieldIDOffsets.writingDirection);
    LOGV("sChineseSettingFieldIDOffsets.topline = %d", sChineseSettingFieldIDOffsets.topline);
    LOGV("sChineseSettingFieldIDOffsets.helpline = %d", sChineseSettingFieldIDOffsets.helpline);
    LOGV("sChineseSettingFieldIDOffsets.baseline = %d", sChineseSettingFieldIDOffsets.baseline);
    LOGV("sChineseSettingFieldIDOffsets.width = %d", sChineseSettingFieldIDOffsets.width);
    LOGV("sChineseSettingFieldIDOffsets.height = %d", sChineseSettingFieldIDOffsets.height);
    LOGV("sChineseSettingFieldIDOffsets.inputGuide = %d", sChineseSettingFieldIDOffsets.inputGuide);
    LOGV("sChineseSettingFieldIDOffsets.categories = %d", sChineseSettingFieldIDOffsets.categories);

    // cache Java List class fields and methods
    clazz = env->FindClass("java/util/List");
    sListMethodIDOffsets.size  = env->GetMethodID(clazz, "size", "()I");
    sListMethodIDOffsets.get   = env->GetMethodID(clazz, "get", "(I)Ljava/lang/Object;");

    // cache Category methods
    clazz = env->FindClass("com/moca/input/WriteChineseCategory");
    sChineseCategoryMethodIDOffsets.get  = env->GetMethodID(clazz, "get", "()I");

    LOGV("acacheJavaFieldAndClassIDs()...done");
}

static void LogSettings(const DECUMA_SESSION_SETTINGS* settings)
{
    LOGV("DECUMA_SESSION_SETTINGS");
    LOGV("  recognitionMode = %d", settings->recognitionMode);
    LOGV("  supportLine = %d", settings->supportLineSet);
    LOGV("  baseLine = %d", (int)settings->baseline);
    LOGV("  helpLine = %d", (int)settings->helpline);
    LOGV("  topLine = %d", (int)settings->topline);
    LOGV("  writing direction = %d", settings->writingDirection);
}

static void readSettings(
    JNIEnv* env,
    jobject jsettings,
    DECUMA_SESSION_SETTINGS* settings)
{
    LOGV("readSettings()...");

    settings->recognitionMode = (RECOGNITION_MODE)env->GetIntField(jsettings, sChineseSettingFieldIDOffsets.recognitionMode);
    settings->supportLineSet = (SUPPORT_LINE_SET)env->GetIntField(jsettings, sChineseSettingFieldIDOffsets.supportLineSet);
    settings->writingDirection = (WRITING_DIRECTION)env->GetIntField(jsettings, sChineseSettingFieldIDOffsets.writingDirection);
    settings->topline = env->GetIntField(jsettings, sChineseSettingFieldIDOffsets.topline);
    settings->helpline = env->GetIntField(jsettings, sChineseSettingFieldIDOffsets.helpline);
    settings->baseline = env->GetIntField(jsettings, sChineseSettingFieldIDOffsets.baseline);
    settings->UIInputGuide = (UI_INPUT_GUIDE)env->GetIntField(jsettings, sChineseSettingFieldIDOffsets.inputGuide);

    // categories
    jobject categoryListObject = env->GetObjectField(jsettings, sChineseSettingFieldIDOffsets.categories);
    int categoryCount  = env->CallIntMethod(categoryListObject, sListMethodIDOffsets.size);

    if (settings->charSet.nSymbolCategories < categoryCount) {
    	FREE(settings->charSet.pSymbolCategories);
    	settings->charSet.pSymbolCategories = (DECUMA_UINT32*)CALLOC(sizeof(DECUMA_UINT32), categoryCount);
    }
    settings->charSet.nSymbolCategories = categoryCount;
    for (int i = 0; i < categoryCount; i++) {
        jobject categoryObject = env->CallObjectMethod(categoryListObject, sListMethodIDOffsets.get, i);
        int cat = env->CallIntMethod(categoryObject, sChineseCategoryMethodIDOffsets.get);
        settings->charSet.pSymbolCategories[i] = cat;

        LOGV("readSettings()::settings->charSet.pSymbolCategories[%d] = %d", i, settings->charSet.pSymbolCategories[i]);

        env->DeleteLocalRef(categoryObject);
    }

    LogSettings(settings);
    LOGV("readSettings()...done");
}

static jstring Write_Chinese_getVersion_impl(
    JNIEnv*  env,
    jobject  jthis,
    jint context)
{
    LOGV("Write_Chinese_getVersion()...");

    Write_Chinese* session = reinterpret_cast<Write_Chinese*>(context);
    if (session == 0) {
        return 0;
    }

    LOGV("Write_Chinese_getVersion()...%s", session->getVersion());

    return env->NewStringUTF(session->getVersion());
}

static jstring Write_Chinese_getDatabaseVersion_impl(
    JNIEnv*  env,
    jobject  jthis,
    jlong context)
{
    LOGV("Write_Chinese_getDatabaseVersion()...");

    Write_Chinese* session = reinterpret_cast<Write_Chinese*>(context);
    if (session == 0) {
        return 0;
    }

    LOGV("Write_Chinese_getDatabaseVersion()...%s", session->getDatabaseVersion());
    return env->NewStringUTF(session->getDatabaseVersion());
}


static jlong Write_Chinese_create_impl(JNIEnv*  env, jobject jthis, jstring jsDatabaseConfigFile)
{
    LOGV("Write_Chinese_create()...");

    const char* databaseConfigFile = 0;
    if (jsDatabaseConfigFile != 0) {
        databaseConfigFile = (const char*)env->GetStringUTFChars(jsDatabaseConfigFile, 0);
    }

    DBRegistry* dbRegistry = DBRegistry::getInstance(databaseConfigFile);

    Write_Chinese* session = new Write_Chinese(dbRegistry, chinese_data::getInstance(dbRegistry));

    if (jsDatabaseConfigFile != 0) {
        env->ReleaseStringUTFChars(jsDatabaseConfigFile, databaseConfigFile);
    }

    LOGV("Write_Chinese_create()...%p", session);

    // return cookie. It will be cast back to object every time
    return (jlong)session;
}

static jint Write_Chinese_destroy_impl(
    JNIEnv*  env,
    jobject  jthis,
    jlong     context)
{
    LOGV("Write_Chinese_destroy()...");

    Write_Chinese* session  = reinterpret_cast<Write_Chinese*>(context);
    if (session) {
        delete session;
    }

    DBRegistry::deleteInstance();
    chinese_data::deleteInstance();

    LOGV("Write_Chinese_destroy()...done");

    return decumaNoError;
}

static jint Write_Chinese_changeSettings_impl(
    JNIEnv*  env,
    jobject  jthis,
    jlong     context,
    jobject jsettings)
{
    LOGV("Write_Chinese_setSetting()...");

    Write_Chinese* session  = reinterpret_cast<Write_Chinese*>(context);
    if (session == 0 || session->mSessionSettings == NULL) {
        return decumaNullSessionPointer;
    }

    readSettings(env, jsettings, session->mSessionSettings);

    session->applySettingChanges();

    LOGV("Write_Chinese_setSetting()...done");
    return decumaNoError;
}


static jint Write_Chinese_start_impl(
    JNIEnv* env,
    jobject jthis,
    jlong context,
    jobject jsettings,
    jobject androidContext,	
    jint languageID)
{
    LOGV("Write_Chinese_start()...");

    Write_Chinese* session = reinterpret_cast<Write_Chinese*>(context);
    if (session == 0 || session->mSessionSettings == NULL) {
        return decumaNullSessionPointer;
    }
    g_sCnWriteContext = androidContext;
    if (! config_init(env, jthis, g_sCnWriteContext)) {
		LOGD("Chinse Write config_init(context(0x%X))...auth failed", context);
		return decumaNullSessionPointer;
	}

    readSettings(env, jsettings, session->mSessionSettings);

    int status =  session->start(languageID);

    LOGV("Write_Chinese_start()...%d", status);
    return status;
}

static jint Write_Chinese_finish_impl(
    JNIEnv* env,
    jobject jthis,
    jlong context)
{
    LOGV("Write_Chinese_finish()...");

    Write_Chinese* session  = reinterpret_cast<Write_Chinese*>(context);
    if (session == 0) {
        return decumaNullSessionPointer;
    }

    int status =  session->finish();
    g_sCnWriteContext = NULL;
    LOGV("Write_Chinese_finish()...%d", status);
    return status;
}

static jint Write_Chinese_beginArc_impl(
    JNIEnv*  env,
    jobject  jthis,
    jlong     context)
{
    LOGV("Write_Chinese_beginArc()...");

    Write_Chinese* session  = reinterpret_cast<Write_Chinese*>(context);
    if (session == 0) {
        return decumaNullSessionPointer;
    }
    if (! config_init(env, jthis, g_sCnWriteContext)) {
		LOGD("Chinse Write config_init(context(0x%X))...auth failed", context);
		return decumaNullSessionPointer;
	}

    int status =  session->beginArc();

    LOGV("Write_Chinese_begingArc()...%d", status);
    return status;
}

static jint Write_Chinese_addArc_impl (
    JNIEnv*  env,
    jobject  jthis,
    jlong     context,
    jobject  listOfPoints)
{

    LOGV("Write_Chinese_addArc()...");

    Write_Chinese* session  = reinterpret_cast<Write_Chinese*>(context);
    if (session == 0) {
        return decumaNullSessionPointer;
    }
    if (! config_init(env, jthis, g_sCnWriteContext)) {
		LOGD("Chinse Write config_init(context(0x%X))...auth failed", context);
		return decumaNullSessionPointer;
	}

    int status;
    int count = 0;
    int arcId = 0;

    if ((status = session->startNewArc(arcId)) == 0) {

        count = env->CallIntMethod(listOfPoints, sListMethodIDOffsets.size);
        for (int i = 0; i < count; i++) {
            jobject point = env->CallObjectMethod(listOfPoints, sListMethodIDOffsets.get, i);
            int x = env->GetIntField(point, sPointFieldIDOffsets.x);
            int y = env->GetIntField(point, sPointFieldIDOffsets.y);
            if ((status = session->addPoint(arcId, x, y)) != 0) {
                break;
            }
            env->DeleteLocalRef(point);
        }

        status = session->commitArc(arcId);
    }

    LOGV("Write_Chinese_addArc()...points = %d, status %d", count, status);
    return status;
}

static jint Write_Chinese_endArc_impl(
    JNIEnv*  env,
    jobject  jthis,
    jlong     context)

{
    LOGV("Write_Chinese_endArc()...");

    Write_Chinese* session  = reinterpret_cast<Write_Chinese*>(context);
    if (session == 0) {
        return decumaNullSessionPointer;
    }
    if (! config_init(env, jthis, g_sCnWriteContext)) {
		LOGD("Chinse Write config_init(context(0x%X))...auth failed", context);
		return decumaNullSessionPointer;
	}

    int status = session->endArc();

    LOGV("Write_Chinese_endArc()...%d", status);

    return status;
}

static jint Write_Chinese_noteSelectedCandidate_impl (
	JNIEnv*  env,
	jobject  jthis,
	jlong     context,
	jint 	wordIndex)
{
    Write_Chinese* session  = reinterpret_cast<Write_Chinese*>(context);
    if (session == 0) {
        return decumaNullSessionPointer;
    }
    if (! config_init(env, jthis, g_sCnWriteContext)) {
		LOGD("Chinse Write config_init(context(0x%X))...auth failed", context);
		return decumaNullSessionPointer;
	}

    session->noteSelectedCandidate(wordIndex);

    return 0;
}

static jint Write_Chinese_recognize_impl(
    JNIEnv*  env,
    jobject  jthis,
    jlong     context,
    jcharArray arrayStartWord,
    jintArray  resultCount)
{
    LOGV("Write_Chinese_recognize()...");

    Write_Chinese* session  = reinterpret_cast<Write_Chinese*>(context);
    if (session == 0) {
        return decumaNullSessionPointer;
    }
    if (! config_init(env, jthis, g_sCnWriteContext)) {
		LOGD("Chinse Write config_init(context(0x%X))...auth failed", context);
		return decumaNullSessionPointer;
	}

    jchar* startWord = env->GetCharArrayElements(arrayStartWord, 0);;
    jint*  recognizedCount = env->GetIntArrayElements(resultCount, 0);

    int status = session->recognize(startWord, recognizedCount[0]);

    env->ReleaseCharArrayElements(arrayStartWord, startWord, 0);
    env->ReleaseIntArrayElements(resultCount, recognizedCount, 0);

    LOGV("Write_Chinese_recognize()...%d", status);

    return status;
}

static jint Write_Chinese_getRecognitionCandidate_impl(
	    JNIEnv*  env,
	    jobject  jthis,
	    jlong     context,
	    jint	index,
	    jcharArray arrayWord,
	    jint maxLength,
	    jintArray arrayLength,
	    jintArray arrayInstantGesture)
{
    Write_Chinese* session  = reinterpret_cast<Write_Chinese*>(context);
    if (session == 0) {
        return decumaNullSessionPointer;
    }
    if (! config_init(env, jthis, g_sCnWriteContext)) {
		LOGD("Chinse Write config_init(context(0x%X))...auth failed", context);
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

static bool Write_Chinese_setContext_impl(JNIEnv* env, jobject jthis, jlong context, jcharArray newContextArray, jint contextLen)
{
    LOGV("Write_Chinese_setContext_impl(context(0x%X))...", context);

    Write_Chinese* session  = reinterpret_cast<Write_Chinese*>(context);
    if (session == 0) {
        return false;
    }
    if (! config_init(env, jthis, g_sCnWriteContext)) {
		LOGD("Chinse Write config_init(context(0x%X))...auth failed", context);
		return false;
	}

    jchar* newContext = env->GetCharArrayElements(newContextArray, NULL);
    int status = session->setContext((const unsigned short*)newContext, contextLen);
    env->ReleaseCharArrayElements(newContextArray, newContext, NULL);

    LOGV("Write_Chinese_setContext_impl(context0x%X))...%s", context, (status == 0)? "Succeeded": "FAILED");

    return (status == 0);
}

static bool Write_Chinese_getWord_impl(JNIEnv* env, jobject jthis, jlong context, jint wordIndex, jcharArray wordArray, jintArray wordLenArray, jint maxLen)
{
    LOGV("Write_Chinese_getWord_impl(context(0x%X))...", context);

    Write_Chinese* session  = reinterpret_cast<Write_Chinese*>(context);
    if (session == 0) {
        return false;
    }

    jchar* word;
    jint* wordLen;
    if (! config_init(env, jthis, g_sCnWriteContext)) {
		LOGD("Chinse Write config_init(context(0x%X))...auth failed", context);
		return false;
	}

    word = env->GetCharArrayElements(wordArray, NULL);
    wordLen = env->GetIntArrayElements(wordLenArray, NULL);

    int status = session->getWord(wordIndex, (unsigned short*)word, wordLen[0], maxLen);

    env->ReleaseCharArrayElements(wordArray, word, NULL);
    env->ReleaseIntArrayElements(wordLenArray, wordLen, NULL);

    LOGV("Write_Chinese_getWord_impl(context0x%X))...%s", context, (status == 0)? "succeeded": "FAILED");

    return (status == 0);
}

static bool Write_Chinese_setAttribute_impl(JNIEnv* env, jobject object, jlong context, jint id, jint value)
{
    LOGV("Write_Chinese_setAttribute_impl(context(0x%X))...", context);

    Write_Chinese* session  = reinterpret_cast<Write_Chinese*>(context);
    if (! config_init(env, object, g_sCnWriteContext)) {
		LOGD("Chinse Write config_init(context(0x%X))...auth failed", context);
		return false;
	}

    int status = session->setAttribute(id, value);

    LOGV("Write_Chinese_setAttribute_impl(context0x%X))...%s", context, (status == 0)? "succeeded" : "FAILED");

    return (status == 0);
}

static int Write_Chinese_setCommonChar_impl(JNIEnv* env, jobject jthis, jlong context)
{
    LOGV("Write_Chinese_setContext_impl(context(0x%X))...", context);

    Write_Chinese* session  = reinterpret_cast<Write_Chinese*>(context);
    if (session == 0) {
        return -1;
    }
    if (! config_init(env, jthis, g_sCnWriteContext)) {
		LOGD("Chinse Write config_init(context(0x%X))...auth failed", context);
		return -1;
	}

    return session->setCommonChar();
}

static int Write_Chinese_clearCommonChar_impl(JNIEnv* env, jobject jthis, jlong context)
{
    LOGV("Write_Chinese_clearCommonChar_impl(context(0x%X))...", context);

    Write_Chinese* session  = reinterpret_cast<Write_Chinese*>(context);
    if (session == 0) {
        return -1;
    }
    if (! config_init(env, jthis, g_sCnWriteContext)) {
		LOGD("Chinse Write config_init(context(0x%X))...auth failed", context);
		return -1;
	}

    return session->clearCommonChar();
}

static JNINativeMethod gWriteChineseMethods[] =
{
   {"Write_Chinese_create",  "(Ljava/lang/String;)J", (void*)Write_Chinese_create_impl},
   {"Write_Chinese_destroy", "(J)I", (void*)Write_Chinese_destroy_impl},
   {"Write_Chinese_start",   "(JLcom/moca/input/WriteChineseSettings;Landroid/content/Context;I)I", (void*)Write_Chinese_start_impl},
   {"Write_Chinese_finish",  "(J)I", (void*)Write_Chinese_finish_impl},
   {"Write_Chinese_changeSettings", "(JLcom/moca/input/WriteChineseSettings;)I",(void*)Write_Chinese_changeSettings_impl},
   {"Write_Chinese_getVersion",     "(J)Ljava/lang/String;",         (void*)Write_Chinese_getVersion_impl},
   {"Write_Chinese_getDatabaseVersion", "(J)Ljava/lang/String;",     (void*)Write_Chinese_getDatabaseVersion_impl},
   {"Write_Chinese_beginArc",       "(J)I",                         (void*)Write_Chinese_beginArc_impl},
   {"Write_Chinese_addArc",       "(JLjava/util/List;)I",            (void*)Write_Chinese_addArc_impl},
   {"Write_Chinese_endArc",         "(J)I",                         (void*)Write_Chinese_endArc_impl},
   {"Write_Chinese_recognize",      "(J[C[I)I",                      (void*)Write_Chinese_recognize_impl},
   {"Write_Chinese_getRecognitionCandidate", "(JI[CI[I[I)I", (void*)Write_Chinese_getRecognitionCandidate_impl},
   {"Write_Chinese_setContext",      "(J[CI)Z",                      (void*)Write_Chinese_setContext_impl},
   {"Write_Chinese_getWord", "(JI[C[II)Z", (void*)Write_Chinese_getWord_impl},
   {"Write_Chinese_setAttribute", "(JII)Z", (void*)Write_Chinese_setAttribute_impl},

   {"Write_Chinese_noteSelectedCandidate", "(JI)I", (void*)Write_Chinese_noteSelectedCandidate_impl},

   {"Write_Chinese_setCommonChar", "(J)I", (void*)Write_Chinese_setCommonChar_impl},
   {"Write_Chinese_clearCommonChar", "(J)I", (void*)Write_Chinese_clearCommonChar_impl},
};

int registerChineseWriteNative(JNIEnv *env)
{
    const char* const kChineseWriteClassPathName = "com/moca/input/NativeWriteChinese";
    jclass clazz;

    clazz = env->FindClass(kChineseWriteClassPathName);
    if (clazz == NULL) {
        fprintf(stderr,
            "Chinese Write Native registration unable to find class '%s'\n", kChineseWriteClassPathName);
        return JNI_FALSE;
    }
    if (env->RegisterNatives(clazz, gWriteChineseMethods, sizeof(gWriteChineseMethods) / sizeof(gWriteChineseMethods[0])) < 0) {
        fprintf(stderr, "Chinese Write Register Natives failed for '%s'\n", kChineseWriteClassPathName);
        return JNI_FALSE;
    }

    // Caching field and method IDs
    cacheJavaFieldAndClassIDs(env);

    return JNI_TRUE;
}

} // namespace mocainput



