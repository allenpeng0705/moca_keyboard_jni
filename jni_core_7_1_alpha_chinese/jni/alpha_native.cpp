/*
**
*/


#include "Log.h"

#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <jni.h>
#include <ctype.h>

#include "mem_alloc.h"

#include "alpha_controller.h"
#include "config_native.h"

namespace mocainput {

// android.graphics.Point
struct JPoint
{
	jclass clazz;
	jmethodID construct;
	jfieldID x;
	jfieldID y;
};

// java.util.List
struct JList
{
	jclass clazz;
	jmethodID add;
	jmethodID get;
	jmethodID size;
};

static JPoint g_sPointClass = {0, };
static JList g_sListClass = {0, };
static jobject g_sContext = NULL;

static jlong alpha_create(JNIEnv *env, jobject object, jstring jsDatabaseConfigFile)
{
	LOGV("alpha_create()...");

	const char* databaseConfigFile = 0;

	if (jsDatabaseConfigFile != 0) {
	    databaseConfigFile = (const char*)env->GetStringUTFChars(jsDatabaseConfigFile, 0);
	}

	alpha_controller* controller = new alpha_controller(DBRegistry::getInstance(databaseConfigFile));

	if (controller) {
		controller->create();
	}

	if (jsDatabaseConfigFile != 0) {
	    env->ReleaseStringUTFChars(jsDatabaseConfigFile, databaseConfigFile);
	}

	LOGV("alpha_create()...context = 0x%X", controller);

	return (jlong)controller;
}

static void alpha_destroy(JNIEnv *env, jobject object, jlong context)
{
    LOGV("alpha_destroy()...");

	alpha_controller* controller = (alpha_controller*)context;

	if (controller) {
		controller->destroy();
		delete controller;
	}

	DBRegistry::deleteInstance();

	LOGV("alpha_destroy()...done");
}

static jboolean alpha_start(JNIEnv *env, jobject object, jlong context, jobject androidContext)
{
    LOGV("alpha_start(context(0x%X))...", context);

	bool ret = false;
	alpha_controller* controller = (alpha_controller*)context;

    if (controller == 0) {
        LOGE("alpha_start(context(0x%X))...failed", context);
        return ret;
    }

    g_sContext = androidContext;

    if (! config_init(env, object, g_sContext))  {
        LOGD("config_init(context(0x%X))...authenticate failed", context);
        return false;
    }

    ret = controller->start();

	LOGV("alpha_start()...%s", ret ? "success" : "failed");

	return ret;
}

static void alpha_finish(JNIEnv *env, jobject object, jlong context)
{
    LOGV("alpha_finish()...");

	alpha_controller* controller = (alpha_controller*)context;

	if (controller) {
		controller->finish();
	}

    g_sContext = NULL;

    LOGV("alpha_finish()...success");
}

static void alpha_initTrace(JNIEnv *env, jobject object, jlong context, jint keyboardLayoutId, jint width, jint height)
{
	LOGV("alpha_initTrace(context(0x%X), kdbId(%X), width(%X), height(%X))...", context, keyboardLayoutId, width, height);

	alpha_controller* controller = (alpha_controller*)context;

	if (controller == 0) {
		LOGE("alpha_initTrace(context(0x%X), kdbId(%X), width(%X), height(%X))...", context, keyboardLayoutId, width, height);
		return;
	}

    if (! config_init(env, object, g_sContext))  {
        LOGD("config_init(context(0x%X))...authenticate failed", context);
        return;
    }

    // verifying the list class has been initialised
    if(!g_sListClass.clazz)
    {
        g_sListClass.clazz = env->FindClass("java/util/List");
        g_sListClass.size  = env->GetMethodID(g_sListClass.clazz, "size", "()I");
        g_sListClass.get   = env->GetMethodID(g_sListClass.clazz, "get", "(I)Ljava/lang/Object;");
        g_sListClass.add   = env->GetMethodID(g_sListClass.clazz, "add", "(Ljava/lang/Object;)Z");
    }

	// verifying the point class has been initialised
	if(!g_sPointClass.clazz)
	{
		g_sPointClass.clazz = env->FindClass("android/graphics/Point");
		g_sPointClass.construct = env->GetMethodID(g_sPointClass.clazz, "<init>", "(II)V");
		g_sPointClass.x = env->GetFieldID(g_sPointClass.clazz, "x", "I");
		g_sPointClass.y = env->GetFieldID(g_sPointClass.clazz, "y", "I");
	}

	// attempting to initialise the trace
	controller->initTrace(keyboardLayoutId, width, height);

	LOGV("Destroy keys array");

	LOGV("alpha_initTrace()...done");
}

static bool alpha_getKeyPositions(JNIEnv *env, jobject object, jlong context, jint keyboardLayoutId, jobject points)
{
	LOGV("alpha_getKeyPositions(context(0x%X), kdbId(%X))...", context, keyboardLayoutId);

	ET9KeyPoint sPoints[255];
	unsigned int iNumPoints = 0;

	memset(sPoints, 0, sizeof(sPoints));

	bool ret = false;
	alpha_controller* controller = (alpha_controller*)context;

	if (controller == 0) {
		LOGE("alpha_getKeyPositions(context(0x%X), kdbId(0x%X))...failed", context, keyboardLayoutId);
		return ret;
	}

    if (! config_init(env, object, g_sContext))  {
        LOGD("config_init(context(0x%X))...authenticate failed", context);
        return false;
    }

    // verifying the list class has been initialised
    if(!g_sListClass.clazz)
    {
        g_sListClass.clazz = env->FindClass("java/util/List");
        g_sListClass.size  = env->GetMethodID(g_sListClass.clazz, "size", "()I");
        g_sListClass.get   = env->GetMethodID(g_sListClass.clazz, "get", "(I)Ljava/lang/Object;");
        g_sListClass.add   = env->GetMethodID(g_sListClass.clazz, "add", "(Ljava/lang/Object;)Z");
    }

	// verifying the point class has been initialised
	if(!g_sPointClass.clazz)
	{
		g_sPointClass.clazz = env->FindClass("android/graphics/Point");
		g_sPointClass.construct = env->GetMethodID(g_sPointClass.clazz, "<init>", "(II)V");
		g_sPointClass.x = env->GetFieldID(g_sPointClass.clazz, "x", "I");
		g_sPointClass.y = env->GetFieldID(g_sPointClass.clazz, "y", "I");
	}

	// attempting to retrieve the key positions
	if(!controller->getKeyPositions(keyboardLayoutId, sPoints, 255, iNumPoints))
	{
		LOGE("alpha_getKeyPositions(context(0x%X), kdbId(0x%X))...failed", context, keyboardLayoutId);
		return ret;
	}

	jobject jPoint;

	for(int i = 0; i < iNumPoints; i++)
	{
		jPoint = env->NewObject(g_sPointClass.clazz, g_sPointClass.construct, sPoints[i].nX, sPoints[i].nY);

		if(!jPoint)
		{
			LOGE("alpha_getKeyPositions(context(0x%X), kdbId(0x%X))...failed", context, keyboardLayoutId);
			return ret;
		}

		env->CallBooleanMethod(points, g_sListClass.add, jPoint);
	}

	// successfully added the points to the provided list
	return true;
}

static int alpha_isAutoSpaceBeforeTrace(JNIEnv *env, jobject object, jlong context, jint keyboardLayoutId, jobject trace)
{
	LOGV("alpha_isAutoSpaceBeforeTrace(context(0x%X), kdbId(%X))...", context, keyboardLayoutId);

	int ret = 0;
	alpha_controller* controller = (alpha_controller*)context;

	if (controller == 0) {
		LOGE("alpha_isAutoSpaceBeforeTrace(context(0x%X), kdbId(0x%X))...failed", context, keyboardLayoutId);
		return ret;
	}

    if (! config_init(env, object, g_sContext))  {
        LOGD("config_init(context(0x%X))...authenticate failed", context);
        return false;
    }

    // verifying the list class has been initialised
    if(!g_sListClass.clazz)
    {
        g_sListClass.clazz = env->FindClass("java/util/List");
        g_sListClass.size  = env->GetMethodID(g_sListClass.clazz, "size", "()I");
        g_sListClass.get   = env->GetMethodID(g_sListClass.clazz, "get", "(I)Ljava/lang/Object;");
        g_sListClass.add   = env->GetMethodID(g_sListClass.clazz, "add", "(Ljava/lang/Object;)Z");
    }

	// verifying the point class has been initialised
	if(!g_sPointClass.clazz)
	{
		g_sPointClass.clazz = env->FindClass("android/graphics/Point");
		g_sPointClass.construct = env->GetMethodID(g_sPointClass.clazz, "<init>", "(II)V");
		g_sPointClass.x = env->GetFieldID(g_sPointClass.clazz, "x", "I");
		g_sPointClass.y = env->GetFieldID(g_sPointClass.clazz, "y", "I");
	}

	// attempting to retrieve the number of points in the trace
	int iNumPoints = env->CallIntMethod(trace, g_sListClass.size);

	/*// only use up to the max points
	if(iNumPoints > ET9_TRACE_MAX_POINTS)
	{
		iNumPoints = ET9_TRACE_MAX_POINTS;
	}*/

	// verifying the number of points is valid
	if(iNumPoints)
	{
		// attempting to allocate memory for the trace points
		ET9TracePoint* pPoints = (ET9TracePoint*)MALLOC(sizeof(ET9TracePoint) * iNumPoints);

		// verifying the memory was allocated successfully
		if(!pPoints)
		{
			LOGE("alpha_isAutoSpaceBeforeTrace(context(0x%X), kdbId(0x%X))...failed", context, keyboardLayoutId);

			// failed to allocate memory for the trace points
			return false;
		}

		jobject point;

		// retrieving the position of all the points in the trace
		for(int i = 0; i < iNumPoints; i++)
		{
			point = env->CallObjectMethod(trace, g_sListClass.get, i);
			pPoints[i].nX = env->GetIntField(point, g_sPointClass.x);
			pPoints[i].nY = env->GetIntField(point, g_sPointClass.y);
			env->DeleteLocalRef(point);
		}

		ret = controller->isAutoSpaceBeforeTrace(keyboardLayoutId, pPoints, iNumPoints);

		// destroying the trace points
		FREE(pPoints);
	}
	else
	{
		ret = controller->isAutoSpaceBeforeTrace(keyboardLayoutId, NULL, 0);
	}

	LOGV("alpha_isAutoSpaceBeforeTrace()...done - %s", ret ? "success" : "failed");

	return ret;
}

static bool alpha_processKey(JNIEnv *env, jobject object, jlong context, jint keyboardLayoutId, jint key, jint shiftState)
{
	LOGV("alpha_processKey(context(0x%X), kdbId(%X), key(%X))...", context, keyboardLayoutId, key);

	bool ret = false;
	alpha_controller* controller = (alpha_controller*)context;

	if (controller == 0) {
		LOGE("alpha_processKey(context(0x%X), kdbId(0x%X), key(0x%X))...failed", context, keyboardLayoutId, key);
		return ret;
	}

    if (! config_init(env, object, g_sContext))  {
        LOGD("config_init(context(0x%X))...authenticate failed", context);
        return false;
    }

    ret = controller->processKey(keyboardLayoutId, key, shiftState);

	LOGV("alpha_processKey()...done - %s", ret ? "success" : "failed");

	return ret;
}

static bool alpha_processTap(JNIEnv *env, jobject object, jlong context, jint keyboardLayoutId, jint TapX, jint TapY, jint shiftState)
{
	LOGV("alpha_processTap(context(0x%X), kdbId(%X), TapX(%X), TapY(%X))...", context, keyboardLayoutId, TapX, TapY);

	bool ret = false;
	alpha_controller* controller = (alpha_controller*)context;

    if (controller == 0) {
        LOGE("alpha_processTap(context(0x%X), kdbId(0x%X), TapX(0x%X), TapY(0x%X))...failed", context, keyboardLayoutId, TapX, TapY);
        return ret;
    }

    if (! config_init(env, object, g_sContext))  {
        LOGD("config_init(context(0x%X))...authenticate failed", context);
        return false;
    }

	ret = controller->processTap(keyboardLayoutId, TapX, TapY, shiftState);

	LOGV("alpha_processTap()...done - %s", ret ? "success" : "failed");

	return ret;
}

static bool alpha_processTrace(JNIEnv *env, jobject object, jlong context, jint keyboardLayoutId, jobject trace, jint shiftState)
{
	LOGV("alpha_processTrace(context(0x%X), kdbId(%X))...", context, keyboardLayoutId);

	bool ret = false;
	alpha_controller* controller = (alpha_controller*)context;

	if (controller == 0) {
		LOGE("alpha_processTrace(context(0x%X), kdbId(0x%X))...failed", context, keyboardLayoutId);
		return ret;
	}

    if (! config_init(env, object, g_sContext))  {
        LOGD("config_init(context(0x%X))...authenticate failed", context);
        return false;
    }

    // verifying the list class has been initialised
    if(!g_sListClass.clazz)
    {
        g_sListClass.clazz = env->FindClass("java/util/List");
        g_sListClass.size  = env->GetMethodID(g_sListClass.clazz, "size", "()I");
        g_sListClass.get   = env->GetMethodID(g_sListClass.clazz, "get", "(I)Ljava/lang/Object;");
        g_sListClass.add   = env->GetMethodID(g_sListClass.clazz, "add", "(Ljava/lang/Object;)Z");
    }

	// verifying the point class has been initialised
	if(!g_sPointClass.clazz)
	{
		g_sPointClass.clazz = env->FindClass("android/graphics/Point");
		g_sPointClass.construct = env->GetMethodID(g_sPointClass.clazz, "<init>", "(II)V");
		g_sPointClass.x = env->GetFieldID(g_sPointClass.clazz, "x", "I");
		g_sPointClass.y = env->GetFieldID(g_sPointClass.clazz, "y", "I");
	}

	// attempting to retrieve the number of points in the trace
	int iNumPoints = env->CallIntMethod(trace, g_sListClass.size);

	// verifying the number of points is valid
	if(!iNumPoints)
	{
		LOGE("alpha_processTrace(context(0x%X), kdbId(0x%X))...failed", context, keyboardLayoutId);

		// invalid number of points
		return false;
	}

	LOGV("numPoints: %i", iNumPoints);

	/*// only use up to the max points
	if(iNumPoints > ET9_TRACE_MAX_POINTS)
	{
		iNumPoints = ET9_TRACE_MAX_POINTS;
	}*/

	// attempting to allocate memory for the trace points
	ET9TracePoint* pPoints = (ET9TracePoint*)MALLOC(sizeof(ET9TracePoint) * iNumPoints);

	// verifying the memory was allocated successfully
	if(!pPoints)
	{
		LOGE("alpha_processTrace(context(0x%X), kdbId(0x%X))...failed", context, keyboardLayoutId);

		// failed to allocate memory for the trace points
		return false;
	}

	LOGV("Allocated point memory");

	jobject point;

	// retrieving the position of all the points in the trace
	for(int i = 0; i < iNumPoints; i++)
	{
		point = env->CallObjectMethod(trace, g_sListClass.get, i);
		pPoints[i].nX = env->GetIntField(point, g_sPointClass.x);
		pPoints[i].nY = env->GetIntField(point, g_sPointClass.y);
		env->DeleteLocalRef(point);
	}

	LOGV("Copied point data");

	ret = controller->processTrace(keyboardLayoutId, pPoints, iNumPoints, shiftState);

	LOGV("Processed Trace");

	// destroying the trace points
	FREE(pPoints);

	LOGV("alpha_processTrace()...done - %s", ret ? "success" : "failed");

	return ret;
}

static bool alpha_addExplicit(JNIEnv* env, jobject object, jlong context,
		jcharArray charArray, jint len, jint shiftState)
{
	LOGV("alpha_addExplicit(context(0x%X))...", context);

	alpha_controller* controller = (alpha_controller*)context;

    if (controller == 0) {
        LOGE("alpha_addExplicit(context(0x%X))...failed", context);
        return false;
    }

    if (! config_init(env, object, g_sContext))  {
        LOGD("config_init(context(0x%X))...authenticate failed", context);
        return false;
    }

    jchar* word;
    bool ret;

	word = env->GetCharArrayElements(charArray, NULL);
	ret = controller->addExplicit((const unsigned short*)word, (int)len, (int)shiftState);
    env->ReleaseCharArrayElements(charArray, word, JNI_ABORT);

    LOGV("alpha_addExplicit(context(0x%X))...%s", context, ret ? "success" : "failed");

    return ret;
}

static int alpha_getKeyCount(JNIEnv* env, jobject object, jlong context)
{
    alpha_controller* controller = (alpha_controller*)context;

    if (controller == 0) {
        return 0;
    }

    if (! config_init(env, object, g_sContext))  {
        LOGD("config_init(context(0x%X))...authenticate failed", context);
        return false;
    }

    return controller->getKeyCount();
}

static bool alpha_clearKey(JNIEnv* env, jobject object, jlong context, jint kdbId, jint wordIndex)
{
	LOGV("alpha_clearKey(context(0x%X))...", context);

	bool ret = false;
	alpha_controller* controller = (alpha_controller*)context;

    if (controller == 0) {
        LOGE("alpha_clearKey(context(0x%X))...failed", context);
        return false;
    }

    if (! config_init(env, object, g_sContext))  {
        LOGD("config_init(context(0x%X))...authenticate failed", context);
        return false;
    }

	ret = controller->clearKey();

	LOGV("alpha_clearKey(context(0x%X))...%s", context, "done");

	return ret;
}

static bool alpha_clearAllKeys(JNIEnv* env, jobject, jlong context)
{
	LOGV("alpha_clearAllKeys(context(0x%X))...", context);

	bool ret;
	alpha_controller* controller = (alpha_controller*)context;

	if (controller == 0) {
		LOGE("alpha_clearAllKeys(context(0x%X))...failed", context);
		return false;
	}

	ret = controller->clearAllKeys();

	LOGV("alpha_clearKey(context(0x%X))...%s", context, "done");

	return ret;
}

static jint buildSelectionList(JNIEnv *env, jobject object, jlong context)
{
	LOGV("buildSelectionList(context(0x%X))...", context);

	int wordListCount = 0;
	alpha_controller* controller = (alpha_controller*)context;

	if (controller == 0) {
		LOGE("buildSelectionList(context(0x%X))...failed", context);
		return 0;
	}

    if (! config_init(env, object, g_sContext))  {
        LOGD("config_init(context(0x%X))...authenticate failed", context);
        return false;
    }

    wordListCount = controller->buildWordList();

	LOGV("buildSelectionList(context(0x%X))...%d", context, wordListCount);

	return wordListCount;
}

static jint alpha_getDefaultWordIndex(JNIEnv *env, jobject object, jlong context)
{
	LOGV("alpha_getDefaultWordIndex(context(0x%X))...", context);

	int wordIndex = 0;
	alpha_controller* controller = (alpha_controller*)context;

    if (controller == 0) {
        LOGE("alpha_getDefaultWordIndex(context(0x%X))...failed", context);
        return 0;
    }

    if (! config_init(env, object, g_sContext))  {
        LOGD("config_init(context(0x%X))...authenticate failed", context);
        return false;
    }

	wordIndex = controller->getDefaultWordIndex();

	LOGV("alpha_getDefaultWordIndex(context(0x%X))...%d", context, wordIndex);

	return wordIndex;
}

static jboolean alpha_getWord(JNIEnv *env, jobject object, jlong context, jint wordIndex, jcharArray wordCharArray, jcharArray subCharArray, jintArray wordLenResultsArray, jint maxWordLen)
{
	LOGV("alpha_getWord(context(0x%X))...", context);

	alpha_controller* controller = (alpha_controller*)context;
	jchar* word;
	jchar* sub;
	int* results; // array of three int's, 0 = wordLen, 1 = wordCompLen, 2 = subLen;
	bool success;

	if (controller == 0) {
		LOGE("alpha_getWord(context(0x%X))...failed", context);
		return false;
	}

    results = env->GetIntArrayElements(wordLenResultsArray, NULL);
    word = env->GetCharArrayElements(wordCharArray, NULL);
	sub  = env->GetCharArrayElements(subCharArray, NULL);

	success = controller->getWord(wordIndex, (unsigned short*)word, (unsigned short*)sub, results[0], results[1], results[2], maxWordLen);

    env->ReleaseIntArrayElements(wordLenResultsArray, results, 0);
    env->ReleaseCharArrayElements(wordCharArray, word, 0);
    env->ReleaseCharArrayElements(subCharArray, sub, 0);

	LOGV("alpha_getWord(context(0x%X))...%s", context, success ? "success" : "failed");

    return success;
}

static jboolean alpha_setLanguage(JNIEnv *env, jobject object, jlong context, jint languageId)
{
    LOGV("alpha_setLanguage(context(0x%X))...", context);

	bool ret = false;
	alpha_controller* controller = (alpha_controller*)context;

	if (controller == 0) {
		LOGE("alpha_setLanguage(context(0x%X))...failed", context);
		return false;
	}

    if (! config_init(env, object, g_sContext))  {
        LOGD("config_init(context(0x%X))...authenticate failed", context);
        return false;
    }

    ret = controller->setLanguage(languageId);

	LOGV("alpha_setLanguage(context(0x%X))...%s", context, ret ? "success" : "failed");

	return ret;
}

static jboolean alpha_setAttribute(JNIEnv *env, jobject object, jlong context, jint id, jint value)
{
	LOGV("alpha_setAttribute(context(%X))...", context);

	alpha_controller* controller = (alpha_controller*)context;

    if (controller == 0) {
        LOGE("alpha_setAttribute(context(%X))...failed", context);
        return false;
    }

    if (! config_init(env, object, g_sContext))  {
        LOGD("config_init(context(0x%X))...authenticate failed", context);
        return false;
    }

	bool ret = controller->setAttribute((int)id, (int)value);

	LOGV("alpha_setAttribute(context(%X))...%s", context, ret ? "success" : "failed");

	return ret;
}

static void alpha_wordSelected(JNIEnv* env, jobject object, jlong context, jint wordIndex)
{
	LOGV("alpha_wordSelected(context(%X))...", context);

	alpha_controller* controller = (alpha_controller*)context;

	if (controller == 0) {
		LOGE("alpha_wordSelected(context(%X))...failed", context);
	}

    if (! config_init(env, object, g_sContext))  {
        LOGD("config_init(context(0x%X))...authenticate failed", context);
        return;
    }

    controller->wordSelected(wordIndex);

	LOGV("alpha_wordSelected(context(%X))...done", context);
}

static bool alpha_addWordToUserDictionary(JNIEnv* env, jobject object, jlong context, jcharArray wordCharArray, jint wordLen)
{
	LOGV("alpha_addWordToUserDictionary(context(%X))...", context);

	bool success;
	alpha_controller* controller = (alpha_controller*)context;

    if (controller == 0) {
        LOGE("alpha_addWordToUserDictionary(context(%X))...failed", context);
        return false;
    }

    if (! config_init(env, object, g_sContext))  {
        LOGD("config_init(context(0x%X))...authenticate failed", context);
        return false;
    }

	jchar* word = env->GetCharArrayElements(wordCharArray, NULL);
	success = controller->addWordToUserDictionary((const unsigned short*)word, (int)wordLen);
	env->ReleaseCharArrayElements(wordCharArray, word, JNI_COMMIT);

	LOGV("alpha_addWordToUserDictionary(context(%X))...%s", context, success ? "success" : "failed");

	return success;
}

static void alpha_breakContext(JNIEnv* env, jobject object, jlong context)
{
	LOGV("alpha_breakContext(context(%X))...", context);

	alpha_controller* controller = (alpha_controller*)context;

	if (controller == 0) {
		LOGE("alpha_breakContextcontext(%X))...failed", context);
	}

    if (! config_init(env, object, g_sContext))  {
        LOGD("config_init(context(0x%X))...authenticate failed", context);
        return;
    }

    controller->breakContext();

	LOGV("alpha_breakContext(context(%X))...done", context);
}

static void alpha_setContext(JNIEnv* env, jobject object, jlong context, jcharArray charArray, jint len)
{
	LOGV("alpha_setContext(context(0x%X))...", context);

	alpha_controller* controller = (alpha_controller*)context;

	if (controller == 0) {
		LOGE("alpha_setContext(context(0x%X))...failed", context);
		return;
	}

    if (! config_init(env, object, g_sContext))  {
        LOGD("config_init(context(0x%X))...authenticate failed", context);
        return;
    }

    jchar* word;
    word = env->GetCharArrayElements(charArray, NULL);

	controller->setContext(word, len);

    env->ReleaseCharArrayElements(charArray, word, JNI_ABORT);

    LOGV("alpha_setContext(context(0x%X))...done", context);

}

static bool alpha_recaptureWord(JNIEnv* env, jobject object, jlong context,
		jint kdbId, jcharArray charArray, jint len)
{
	LOGV("alpha_recaptureWord(context(0x%X))...", context);

	alpha_controller* controller = (alpha_controller*)context;

    if (controller == 0) {
        LOGE("alpha_recaptureWord(context(0x%X))...failed", context);
        return false;
    }

    if (! config_init(env, object, g_sContext))  {
        LOGD("config_init(context(0x%X))...authenticate failed", context);
        return false;
    }

    jchar* word;
    bool ret;

	word = env->GetCharArrayElements(charArray, NULL);
	ret = controller->ReCaptureWord(kdbId, (unsigned short* const)word, (int)len);
    env->ReleaseCharArrayElements(charArray, word, JNI_ABORT);

    LOGV("alpha_recaptureWord(context(0x%X))...%s", context, ret ? "success" : "failed");

    return ret;

}

static int alpha_getExactType(JNIEnv* env, jobject object, jlong context,
        jcharArray charArray, jint len)
{
    alpha_controller* controller = (alpha_controller*)context;
    if (controller == 0) {
        return 0;
    }

    if (! config_init(env, object, g_sContext))  {
        LOGD("config_init(context(0x%X))...authenticate failed", context);
        return 0;
    }

    jchar* word;
    int count;

    word = env->GetCharArrayElements(charArray, NULL);
    count = controller->getExactType((unsigned short*)word, (int)len);
    env->ReleaseCharArrayElements(charArray, word, 0);
    return count;
}

static int alpha_getInlineText(JNIEnv* env, jobject object, jlong context,
        jcharArray charArray, jint len)
{
    alpha_controller* controller = (alpha_controller*)context;
    if (controller == 0) {
        return 0;
    }

    if (! config_init(env, object, g_sContext))  {
        LOGD("config_init(context(0x%X))...authenticate failed", context);
        return 0;
    }

    jchar* word;
    int count;

    word = env->GetCharArrayElements(charArray, NULL);
    count = controller->getInlineText((unsigned short*)word, (int)len);
    env->ReleaseCharArrayElements(charArray, word, 0);
    return count;
}

static bool alpha_isInlineKnown(JNIEnv* env, jobject object, jlong context)
{
    alpha_controller* controller = (alpha_controller*)context;
    if (controller == 0) {
        return 0;
    }

    if (! config_init(env, object, g_sContext))  {
        LOGD("config_init(context(0x%X))...authenticate failed", context);
        return false;
    }

    return controller->isInlineKnown();
}

static bool alpha_isSymbolUpperCase(JNIEnv* env, jobject object, jlong context,
		jchar symbol)
{
	LOGV("alpha_isUpperSymbol(context(0x%X))...", context);

	alpha_controller* controller = (alpha_controller*)context;

    if (controller == 0) {
        LOGE("alpha_isUpperSymbol(context(0x%X))...failed", context);
        return false;
    }

    if (! config_init(env, object, g_sContext))  {
        LOGD("config_init(context(0x%X))...authenticate failed", context);
        return false;
    }

	bool ret = controller->isUpperSymbol((const unsigned short)symbol);

    LOGV("alpha_isUpperSymbol(context(0x%X))...%s", context, ret ? "success" : "failed");

    return ret;

}

static bool alpha_isSymbolLowerCase(JNIEnv* env, jobject object, jlong context,
		jchar symbol)
{
	LOGV("alpha_isLowerSymbol(context(0x%X))...", context);

	alpha_controller* controller = (alpha_controller*)context;

	if (controller == 0) {
		LOGE("alpha_isLowerSymbol(context(0x%X))...failed", context);
		return false;
	}

    if (! config_init(env, object, g_sContext))  {
        LOGD("config_init(context(0x%X))...authenticate failed", context);
        return false;
    }

    bool ret = controller->isLowerSymbol((const unsigned short)symbol);

    LOGV("alpha_isLowerSymbol(context(0x%X))...%s", context, ret ? "success" : "failed");

    return ret;

}

static jchar alpha_toLowerSymbol(JNIEnv* env, jobject object, jlong context, jchar symbol)
{
	LOGV("alpha_toLowerSymbol(context(0x%X))...", context);

	alpha_controller* controller = (alpha_controller*)context;
	unsigned short lowerSymbol = (unsigned short)symbol;

    if (controller == 0) {
        LOGE("alpha_toLowerSymbol(context(0x%X))...failed", context);
        return lowerSymbol;
    }

    if (! config_init(env, object, g_sContext))  {
        LOGD("config_init(context(0x%X))...authenticate failed", context);
        return lowerSymbol;
    }

	controller->toLowerSymbol((const unsigned short)symbol, (unsigned short * const) &lowerSymbol);

	LOGV("alpha_toLowerSymbol(context(0x%X))...%c", context, lowerSymbol);

	return lowerSymbol;
}

static jchar alpha_toUpperSymbol(JNIEnv* env, jobject object, jlong context, jchar symbol)
{
	LOGV("alpha_toUpperSymbol(context(0x%X))...", context);

	alpha_controller* controller = (alpha_controller*)context;
    unsigned short upperSymbol = (unsigned short)symbol;

	if (controller == 0) {
		LOGE("alpha_toUpperSymbol(context(0x%X))...failed", context);
		return upperSymbol;
	}

    if (! config_init(env, object, g_sContext))  {
        LOGD("config_init(context(0x%X))...authenticate failed", context);
        return upperSymbol;
    }

    controller->toUpperSymbol((const unsigned short)symbol, (unsigned short * const) &upperSymbol);

	LOGV("alpha_toUpperSymbol(context(0x%X))...%c", context, upperSymbol);

	return upperSymbol;
}

//
// UDB
//
static bool alpha_udb_add(JNIEnv* env, jobject object, jlong context, jcharArray charArray, jint len)
{
    alpha_controller* controller = (alpha_controller*)context;
    if (controller == 0) {
        return false;
    }

    if (! config_init(env, object, g_sContext))  {
        LOGD("config_init(context(0x%X))...authenticate failed", context);
        return false;
    }

    bool ret;
    jchar* word = env->GetCharArrayElements(charArray, NULL);
    ret = controller->udb_add((const unsigned short*)word, (int)len);
    env->ReleaseCharArrayElements(charArray, word, 0);

    return ret;
}

static bool alpha_udb_delete(JNIEnv* env, jobject object, jlong context, jcharArray charArray, jint len)
{
    alpha_controller* controller = (alpha_controller*)context;
    if (controller == 0) {
        return false;
    }

    if (! config_init(env, object, g_sContext))  {
        LOGD("config_init(context(0x%X))...authenticate failed", context);
        return false;
    }

    bool ret;
    jchar* word = env->GetCharArrayElements(charArray, NULL);
    ret = controller->udb_delete((const unsigned short*)word, (int)len);
    env->ReleaseCharArrayElements(charArray, word, 0);

    return ret;
}

static bool alpha_udb_find(JNIEnv* env, jobject object, jlong context, jcharArray charArray, jint len)
{
    alpha_controller* controller = (alpha_controller*)context;
    if (controller == 0) {
        return false;
    }

    if (! config_init(env, object, g_sContext))  {
        LOGD("config_init(context(0x%X))...authenticate failed", context);
        return false;
    }

    bool ret;
    jchar* word = env->GetCharArrayElements(charArray, NULL);
    ret = controller->udb_find((const unsigned short*)word, (int)len);
    env->ReleaseCharArrayElements(charArray, word, 0);

    return ret;
}

static bool alpha_udb_getNext(JNIEnv* env, jobject object, jlong context,
        jcharArray charArrayWord, jintArray resultWordLen, jint maxWordLen)
{
    alpha_controller* controller = (alpha_controller*)context;
    if (controller == 0) {
        return false;
    }

    if (! config_init(env, object, g_sContext))  {
        LOGD("config_init(context(0x%X))...authenticate failed", context);
        return false;
    }

    jchar* word = env->GetCharArrayElements(charArrayWord, NULL);
    jint*  wordLen = env->GetIntArrayElements(resultWordLen, NULL);

    bool ret;
    int localWordLen = (int)wordLen[0];

    ret = controller->udb_getNext((unsigned short*)word, localWordLen, (int)maxWordLen);
    wordLen[0] = localWordLen;

    env->ReleaseCharArrayElements(charArrayWord, word, 0);
    env->ReleaseIntArrayElements(resultWordLen, wordLen, 0);

    return ret;
}

static jint alpha_udb_count(JNIEnv* env, jobject object, jlong context)
{
    alpha_controller* controller = (alpha_controller*)context;
    if (controller == 0) {
        return false;
    }

    return controller->udb_count();
}

static void alpha_udb_reset(JNIEnv* env, jobject object, jlong context)
{
    alpha_controller* controller = (alpha_controller*)context;
    if (controller) {
        controller->udb_reset();
    }
}

static int alpha_udb_getSize(JNIEnv* env, jobject object, jlong context)
{
    alpha_controller* controller = (alpha_controller*)context;
    if (controller == 0) {
        return 0;
    }

    return controller->udb_getSize();
}

static int alpha_udb_getRemainingMemory(JNIEnv* env, jobject object, jlong context)
{
    alpha_controller* controller = (alpha_controller*)context;
    if (controller == 0) {
        return 0;
    }

    return controller->udb_getRemainingMemory();
}

static bool alpha_udb_scanBuf(JNIEnv* env, jobject object, jlong context, jcharArray charArray, jint len)
{
    alpha_controller* controller = (alpha_controller*)context;
    if (controller == 0) {
        return false;
    }

    bool ret;
    jchar* word = env->GetCharArrayElements(charArray, NULL);
    ret = controller->udb_scanBuf((const unsigned short*)word, (int)len);
    env->ReleaseCharArrayElements(charArray, word, 0);

    return ret;
}

//
// ASDB
//
static bool alpha_asdb_add(JNIEnv* env, jobject object, jlong context,
        jcharArray charArrayWord, jint wordLen, jcharArray charArraySubs, jint subsLen)
{
    alpha_controller* controller = (alpha_controller*)context;
    if (controller == 0) {
        return false;
    }

    if (! config_init(env, object, g_sContext))  {
        LOGD("config_init(context(0x%X))...authenticate failed", context);
        return false;
    }

    bool ret;
    jchar* word = env->GetCharArrayElements(charArrayWord, NULL);
    jchar* subs = env->GetCharArrayElements(charArraySubs, NULL);

    ret = controller->asdb_add((const unsigned short*)word, (int)wordLen, (const unsigned short*)subs, (int)subsLen);

    env->ReleaseCharArrayElements(charArrayWord, word, 0);
    env->ReleaseCharArrayElements(charArraySubs, subs, 0);

    return ret;
}

static bool alpha_asdb_delete(JNIEnv* env, jobject object, jlong context, jcharArray charArray, jint len)
{
    alpha_controller* controller = (alpha_controller*)context;
    if (controller == 0) {
        return false;
    }

    if (! config_init(env, object, g_sContext))  {
        LOGD("config_init(context(0x%X))...authenticate failed", context);
        return false;
    }

    bool ret;
    jchar* word = env->GetCharArrayElements(charArray, NULL);

    ret = controller->asdb_delete((const unsigned short*)word, (int)len);

    env->ReleaseCharArrayElements(charArray, word, 0);

    return ret;
}

static bool alpha_asdb_find(JNIEnv* env, jobject object, jlong context,
        jcharArray charArrayWord, jint wordLen, jcharArray charArraySubs, jint subsLen)
{
    alpha_controller* controller = (alpha_controller*)context;
    if (controller == 0) {
        return false;
    }

    if (! config_init(env, object, g_sContext))  {
        LOGD("config_init(context(0x%X))...authenticate failed", context);
        return false;
    }

    bool ret;
    jchar* word = env->GetCharArrayElements(charArrayWord, NULL);
    jchar* subs = env->GetCharArrayElements(charArraySubs, NULL);

    ret = controller->asdb_find((const unsigned short*)word, (int)wordLen, (const unsigned short*)subs, (int)subsLen);

    env->ReleaseCharArrayElements(charArrayWord, word, 0);
    env->ReleaseCharArrayElements(charArraySubs, subs, 0);

    return ret;
}

static bool alpha_asdb_getNext(JNIEnv* env, jobject object, jlong context,
        jcharArray charArrayWord, jintArray resultWordLen,
        jcharArray charArraySubs, jintArray resultSubsLen, jint maxWordLen)
{
    alpha_controller* controller = (alpha_controller*)context;
    if (controller == 0) {
        return false;
    }

    if (! config_init(env, object, g_sContext))  {
        LOGD("config_init(context(0x%X))...authenticate failed", context);
        return false;
    }

    jchar* word = env->GetCharArrayElements(charArrayWord, NULL);
    jint*  wordLen = env->GetIntArrayElements(resultWordLen, NULL);
    jchar* subs = env->GetCharArrayElements(charArraySubs, NULL);
    jint*  subsLen = env->GetIntArrayElements(resultSubsLen, NULL);

    bool ret;
    int localWordLen = (int)wordLen[0];
    int localSubsLen = (int)subsLen[0];

    ret = controller->asdb_getNext((unsigned short*)word, localWordLen, (unsigned short*)subs, localSubsLen, (int)maxWordLen);
    wordLen[0] = localWordLen;
    subsLen[0] = localSubsLen;

    env->ReleaseCharArrayElements(charArrayWord, word, 0);
    env->ReleaseIntArrayElements(resultWordLen, wordLen, 0);
    env->ReleaseCharArrayElements(charArraySubs, subs, 0);
    env->ReleaseIntArrayElements(resultSubsLen, subsLen, 0);

    return ret;
}

static jint alpha_asdb_count(JNIEnv* env, jobject object, jlong context)
{
    alpha_controller* controller = (alpha_controller*)context;
    if (controller == 0) {
        return false;
    }

    if (! config_init(env, object, g_sContext))  {
        LOGD("config_init(context(0x%X))...authenticate failed", context);
        return false;
    }

    return controller->asdb_count();
}

static void alpha_asdb_reset(JNIEnv* env, jobject object, jlong context)
{
    alpha_controller* controller = (alpha_controller*)context;

    if (! config_init(env, object, g_sContext))  {
        LOGD("config_init(context(0x%X))...authenticate failed", context);
        return;
    }

    if (controller) {
        controller->asdb_reset();
    }
}

// ----------------------------------------------------------------------------

static JNINativeMethod gAlphaMethods[] = {
    {"mocainput_alpha_create", "(Ljava/lang/String;)J",  (void*)alpha_create},
    {"mocainput_alpha_destroy", "(J)V",  (void*)alpha_destroy},
    {"mocainput_alpha_start", "(JLandroid/content/Context;)Z",  (void*)alpha_start},
    {"mocainput_alpha_finish", "(J)V",  (void*)alpha_finish},
    {"mocainput_alpha_initTrace", "(JIII)V",  (void*)alpha_initTrace},
    {"mocainput_alpha_getKeyPositions", "(JILjava/util/List;)Z",  (void*)alpha_getKeyPositions},
    {"mocainput_alpha_isAutoSpaceBeforeTrace", "(JILjava/util/List;)I",  (void*)alpha_isAutoSpaceBeforeTrace},
    {"mocainput_alpha_getDefaultWordIndex", "(J)I",  (void*)alpha_getDefaultWordIndex},
    {"mocainput_alpha_getWord", "(JI[C[C[II)Z",  (void*)alpha_getWord},
    {"mocainput_alpha_setLanguage", "(JI)Z",  (void*)alpha_setLanguage},
	{"mocainput_alpha_processKey", "(JIII)Z",  (void*)alpha_processKey},
	{"mocainput_alpha_processTap", "(JIIII)Z",  (void*)alpha_processTap},
	{"mocainput_alpha_processTrace", "(JILjava/util/List;I)Z",  (void*)alpha_processTrace},
	{"mocainput_alpha_addExplicit", "(J[CII)Z",  (void*)alpha_addExplicit},
	{"mocainput_alpha_clearKey", "(J)Z",  (void*)alpha_clearKey},
	{"mocainput_alpha_clearAllKeys", "(J)Z",  (void*)alpha_clearAllKeys},
	{"mocainput_alpha_buildSelectionList", "(J)I",  (void*)buildSelectionList},
	{"mocainput_alpha_wordSelected", "(JI)V",  (void*)alpha_wordSelected},
	{"mocainput_alpha_addWordToUserDictionary", "(J[CI)Z",  (void*)alpha_addWordToUserDictionary},
	{"mocainput_alpha_breakContext", "(J)V",  (void*)alpha_breakContext},
	{"mocainput_alpha_setContext", "(J[CI)V", (void*)alpha_setContext},
	{"mocainput_alpha_getKeyCount", "(J)I", (void*)alpha_getKeyCount},
	{"mocainput_alpha_setAttribute", "(JII)Z",  (void*)alpha_setAttribute},
	{"mocainput_alpha_recaptureWord", "(JI[CI)Z",  (void*)alpha_recaptureWord},
    {"mocainput_alpha_getExactType", "(J[CI)I",  (void*)alpha_getExactType},
    {"mocainput_alpha_getInlineText", "(J[CI)I",  (void*)alpha_getInlineText},
    {"mocainput_alpha_isInlineKnown", "(J)Z",  (void*)alpha_isInlineKnown},
    {"mocainput_alpha_isUpperSymbol", "(JC)Z",  (void*)alpha_isSymbolUpperCase},
    {"mocainput_alpha_isLowerSymbol", "(JC)Z",  (void*)alpha_isSymbolLowerCase},
    {"mocainput_alpha_toUpperSymbol", "(JC)C",  (void*)alpha_toUpperSymbol},
    {"mocainput_alpha_toLowerSymbol", "(JC)C",  (void*)alpha_toLowerSymbol},

    {"mocainput_alpha_udb_add",     "(J[CI)Z", (void*)alpha_udb_add},
    {"mocainput_alpha_udb_delete",  "(J[CI)Z", (void*)alpha_udb_delete},
    {"mocainput_alpha_udb_find",  "(J[CI)Z", (void*)alpha_udb_find},
    {"mocainput_alpha_udb_getNext",   "(J[C[II)Z", (void*)alpha_udb_getNext},
    {"mocainput_alpha_udb_getSize",     "(J)I", (void*)alpha_udb_getSize},
    {"mocainput_alpha_udb_getRemainingMemory",     "(J)I", (void*)alpha_udb_getRemainingMemory},
    {"mocainput_alpha_udb_scanBuf",     "(J[CI)Z", (void*)alpha_udb_scanBuf},

    {"mocainput_alpha_udb_count",  "(J)I", (void*)alpha_udb_count},
    {"mocainput_alpha_udb_reset",  "(J)V", (void*)alpha_udb_reset},

    {"mocainput_alpha_asdb_add",     "(J[CI[CI)Z", (void*)alpha_asdb_add},
    {"mocainput_alpha_asdb_delete",  "(J[CI)Z", (void*)alpha_asdb_delete},
    {"mocainput_alpha_asdb_find",  "(J[CI[CI)Z", (void*)alpha_asdb_find},
    {"mocainput_alpha_asdb_getNext",   "(J[C[I[C[II)Z", (void*)alpha_asdb_getNext},
    {"mocainput_alpha_asdb_count",  "(J)I", (void*)alpha_asdb_count},
    {"mocainput_alpha_asdb_reset",  "(J)V", (void*)alpha_asdb_reset},
};

int registerAlphaNative(JNIEnv *env)
{
	const char* className = "com/moca/input/NativeAlphaInput";
    jclass clazz;
    clazz = env->FindClass(className);
    if (clazz == NULL) {
        fprintf(stderr,
            "Alpha Native registration unable to find class '%s'\n", className);
        return JNI_FALSE;
    }
    if (env->RegisterNatives(clazz, gAlphaMethods, sizeof(gAlphaMethods) / sizeof(gAlphaMethods[0])) < 0) {
        fprintf(stderr, "Alpha RegisterNatives failed for '%s'\n", className);
        return JNI_FALSE;
    }

    return JNI_TRUE;
}

} // namespace mocainput

