/*
**
*/


#include "Log.h"

#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>

#include <jni.h>
#include "config_native.h"
#include "chinese_controller.h"

namespace mocainput {
static JPoint g_scPointClass = {0, };
static JList g_scListClass = {0, };
static jobject g_sCnXt9Context = NULL;
static jlong chinese_create(JNIEnv *env, jobject object, jstring jsDatabaseConfigFile)
{
    const char* databaseConfigFile = 0;
    if (jsDatabaseConfigFile != 0) {
        databaseConfigFile = (const char*)env->GetStringUTFChars(jsDatabaseConfigFile, 0);
    }

	chinese_controller* controller = new chinese_controller(DBRegistry::getInstance(databaseConfigFile));

	if (controller) {
	    controller->create();
	}

    if (databaseConfigFile != 0) {
        env->ReleaseStringUTFChars(jsDatabaseConfigFile, databaseConfigFile);
    }

	return (jlong)controller;
}

static void chinese_destroy(JNIEnv *env, jobject object, jlong context)
{
	chinese_controller* controller = (chinese_controller*)context;

	if (controller) {
		controller->destroy();
		delete controller;
	}

	DBRegistry::deleteInstance();
}

static bool chinese_start(JNIEnv* env, jobject object, jlong context, jobject androidContext)
{
    LOGV("chinese_start(context(0x%X))...", context);

    chinese_controller* controller = (chinese_controller*)context;
    bool ret;

    if (NULL == controller) {
        LOGE("chinese_start(context(0x%X))...failed null check", context);
        return false;
    }
	g_sCnXt9Context = androidContext;
    if (! config_init(env, object, g_sCnXt9Context))  {
        LOGD("config_init(context(0x%X))...authenticate failed", g_sCnXt9Context);
        return false;
    }
    ret = controller->start();

    LOGV("chinese_start(context0x%X))...%s", context, ret ? "succeeded" : "FAILED");

    return ret;
}

static void chinese_finish(JNIEnv* env, jobject object, jlong context)
{
    LOGV("chinese_finish(context(0x%X))...", context);

    chinese_controller* controller = (chinese_controller*)context;

    if (NULL == controller) {
        LOGE("chinese_finish(context(0x%X))...failed null check", context);
        return ;
    }
    controller->finish();
	g_sCnXt9Context = NULL;
}

static void chinese_initTrace(JNIEnv * env, jobject object, jlong context, jint keyboardLayoutId, jint width, jint height)
{
    LOGV("chinese_initTrace(context(0x%X), kdbId(%X), width(%X), height(%X))...", context, keyboardLayoutId, width, height);

    chinese_controller* controller = (chinese_controller*)context;

    if (controller == 0) {
        LOGE("chinese_initTrace(context(0x%X), kdbId(%X), width(%X), height(%X))...", context, keyboardLayoutId, width, height);
        return;
    }

    // verifying the list class has been initialised
    if(!g_scListClass.clazz)
    {
        g_scListClass.clazz = env->FindClass("java/util/List");
        g_scListClass.size  = env->GetMethodID(g_scListClass.clazz, "size", "()I");
        g_scListClass.get   = env->GetMethodID(g_scListClass.clazz, "get", "(I)Ljava/lang/Object;");
        g_scListClass.add   = env->GetMethodID(g_scListClass.clazz, "add", "(Ljava/lang/Object;)Z");
    }

    // verifying the point class has been initialised
    if(!g_scPointClass.clazz)
    {
        g_scPointClass.clazz = env->FindClass("android/graphics/Point");
        g_scPointClass.construct = env->GetMethodID(g_scPointClass.clazz, "<init>", "(II)V");
        g_scPointClass.x = env->GetFieldID(g_scPointClass.clazz, "x", "I");
        g_scPointClass.y = env->GetFieldID(g_scPointClass.clazz, "y", "I");
    }

    // attempting to initialise the trace
    controller->initTrace(keyboardLayoutId, width, height);

    LOGV("Destroy keys array");

    LOGV("chinese_initTrace()...done");
}
  static int chinese_isAutoSpaceBeforeTrace(JNIEnv *env, jobject object, jlong context, jint keyboardLayoutId, jobject trace)
{
    LOGV("chinese_isAutoSpaceBeforeTrace(context(0x%X), kdbId(%X))...", context, keyboardLayoutId);

    int ret = 0;
    chinese_controller* controller = (chinese_controller*)context;

    if (controller == 0) {
        LOGE("chinese_isAutoSpaceBeforeTrace(context(0x%X), kdbId(0x%X))...failed", context, keyboardLayoutId);
        return ret;
    }

    // verifying the list class has been initialised
    if(!g_scListClass.clazz)
    {
        g_scListClass.clazz = env->FindClass("java/util/List");
        g_scListClass.size  = env->GetMethodID(g_scListClass.clazz, "size", "()I");
        g_scListClass.get   = env->GetMethodID(g_scListClass.clazz, "get", "(I)Ljava/lang/Object;");
        g_scListClass.add   = env->GetMethodID(g_scListClass.clazz, "add", "(Ljava/lang/Object;)Z");
    }

    // verifying the point class has been initialised
    if(!g_scPointClass.clazz)
    {
        g_scPointClass.clazz = env->FindClass("android/graphics/Point");
        g_scPointClass.construct = env->GetMethodID(g_scPointClass.clazz, "<init>", "(II)V");
        g_scPointClass.x = env->GetFieldID(g_scPointClass.clazz, "x", "I");
        g_scPointClass.y = env->GetFieldID(g_scPointClass.clazz, "y", "I");
    }

    // attempting to retrieve the number of points in the trace
    int iNumPoints = env->CallIntMethod(trace, g_scListClass.size);

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
            LOGE("chinese_isAutoSpaceBeforeTrace(context(0x%X), kdbId(0x%X))...failed", context, keyboardLayoutId);

            // failed to allocate memory for the trace points
            return false;
        }

        jobject point;

        // retrieving the position of all the points in the trace
        for(int i = 0; i < iNumPoints; i++)
        {
            point = env->CallObjectMethod(trace, g_scListClass.get, i);
            pPoints[i].nX = env->GetIntField(point, g_scPointClass.x);
            pPoints[i].nY = env->GetIntField(point, g_scPointClass.y);
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

    LOGV("chinese_isAutoSpaceBeforeTrace()...done - %s", ret ? "success" : "failed");

    return ret;
}

static bool chinese_setAttribute(JNIEnv* env, jobject object, jlong context, jint id, jint value)
{
    LOGV("chinese_setAttribute(context(0x%X))...", context);

    chinese_controller* controller = (chinese_controller*)context;
    bool ret;

    if (NULL == controller) {
        LOGE("chinese_setAttribute(context(0x%X))...failed null check", context);
        return false;
    }
    if (! config_init(env, object, g_sCnXt9Context))  {
        LOGD("config_init(context(0x%X))...authenticate failed", g_sCnXt9Context);
        return false;
    }	
    ret = controller->setAttribute(id, value);

    LOGV("chinese_setAttribute(context0x%X))...%s", context, ret ? "succeeded" : "FAILED");

    return ret;
}

static bool chinese_setLanguage(JNIEnv* env, jobject object, jlong context, jint langId)
{
    LOGV("chinese_setLanguage(context(0x%X))...", context);

    chinese_controller* controller = (chinese_controller*)context;
    bool ret;

    if (NULL == controller) {
        LOGE("chinese_setLanguage(context(0x%X))...failed null check", context);
        return false;
    }
    if (! config_init(env, object, g_sCnXt9Context))  {
        LOGD("config_init(context(0x%X))...authenticate failed", g_sCnXt9Context);
        return false;
    }	
    ret = controller->setLanguage(langId);

    LOGV("chinese_setLanguage(context0x%X))...%s", context, ret ? "succeeded" : "FAILED");

    return ret;
}

static int chinese_getInputMode(JNIEnv* env, jobject object, jlong context)
{
    LOGV("chinese_getInputMode(context(0x%X))...", context);

    chinese_controller* controller = (chinese_controller*)context;
    int ret;

    if (NULL == controller) {
        LOGE("chinese_getInputMode(context(0x%X))...failed null check", context);
        return 0;
    }
    if (! config_init(env, object, g_sCnXt9Context))  {
        LOGD("config_init(context(0x%X))...authenticate failed", g_sCnXt9Context);
        return false;
    }	
    ret = controller->getInputMode();

    LOGV("chinese_getInputMode(context0x%X))...%d", context, ret);

    return ret;
}

static bool chinese_setInputMode(JNIEnv* env, jobject object, jlong context, jint mode)
{
    LOGV("chinese_setInputMode(context(0x%X))...", context);

    chinese_controller* controller = (chinese_controller*)context;
    bool ret;

    if (NULL == controller) {
        LOGE("chinese_setInputMode(context(0x%X))...failed null check", context);
        return false;
    }
    if (! config_init(env, object, g_sCnXt9Context))  {
        LOGD("config_init(context(0x%X))...authenticate failed", g_sCnXt9Context);
        return false;
    }	
    ret = controller->setInputMode(mode);

    LOGV("chinese_setInputMode(context0x%X))...%s", context, ret ? "succeeded" : "FAILED");

    return ret;
}

static bool chinese_processKey(JNIEnv* env, jobject object, jlong context, jint kdbId, jint key)
{
    LOGV("chinese_processKey(context(0x%X))...", context);

    chinese_controller* controller = (chinese_controller*)context;
    bool ret;

    if (NULL == controller) {
        LOGE("chinese_processKey(context(0x%X))...failed null check", context);
        return false;
    }
    if (! config_init(env, object, g_sCnXt9Context))  {
        LOGD("config_init(context(0x%X))...authenticate failed", g_sCnXt9Context);
        return false;
    }	
    ret = controller->processKey(kdbId, key);

    LOGV("chinese_processKey(context0x%X))...%s", context, ret ? "succeeded" : "FAILED");

    return ret;
}

static bool chinese_processTap(JNIEnv *env, jobject object, jlong context, jint keyboardLayoutId, jint TapX, jint TapY, jint shiftState)
{
	LOGV("chinese_processTap(context(0x%X), kdbId(%X), TapX(%X), TapY(%X))...", context, keyboardLayoutId, TapX, TapY);

	bool ret = false;
	chinese_controller* controller = (chinese_controller*)context;

	if (controller == 0) {
		LOGE("chinese_processTap(context(0x%X), kdbId(0x%X), TapX(0x%X), TapY(0x%X))...failed", context, keyboardLayoutId, TapX, TapY);
		return ret;
	}

	ret = controller->processTap(keyboardLayoutId, TapX, TapY, shiftState);

	LOGV("chinese_processTap()...done - %s", ret ? "success" : "failed");

	return ret;
}

static bool chinese_processTrace(JNIEnv *env, jobject object, jlong context, jint keyboardLayoutId, jobject trace, jint shiftState)
{
	LOGV("chinese_processTrace(context(0x%X), kdbId(%X))...", context, keyboardLayoutId);

	bool ret = false;
	chinese_controller* controller = (chinese_controller*)context;

	if (controller == 0) {
		LOGE("chinese_processTrace(context(0x%X), kdbId(0x%X))...failed", context, keyboardLayoutId);
		return ret;
	}

	// verifying the list class has been initialised
	if(!g_scListClass.clazz)
	{
		g_scListClass.clazz = env->FindClass("java/util/List");
		g_scListClass.size  = env->GetMethodID(g_scListClass.clazz, "size", "()I");
		g_scListClass.get   = env->GetMethodID(g_scListClass.clazz, "get", "(I)Ljava/lang/Object;");
		g_scListClass.add   = env->GetMethodID(g_scListClass.clazz, "add", "(Ljava/lang/Object;)Z");
	}

	// verifying the point class has been initialised
	if(!g_scPointClass.clazz)
	{
		g_scPointClass.clazz = env->FindClass("android/graphics/Point");
		g_scPointClass.construct = env->GetMethodID(g_scPointClass.clazz, "<init>", "(II)V");
		g_scPointClass.x = env->GetFieldID(g_scPointClass.clazz, "x", "I");
		g_scPointClass.y = env->GetFieldID(g_scPointClass.clazz, "y", "I");
	}

	// attempting to retrieve the number of points in the trace
	int iNumPoints = env->CallIntMethod(trace, g_scListClass.size);

	// verifying the number of points is valid
	if(!iNumPoints)
	{
		LOGE("chinese_processTrace(context(0x%X), kdbId(0x%X))...failed", context, keyboardLayoutId);

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
		LOGE("chinese_processTrace(context(0x%X), kdbId(0x%X))...failed", context, keyboardLayoutId);

		// failed to allocate memory for the trace points
		return false;
	}

	LOGV("Allocated point memory");

	jobject point;

	// retrieving the position of all the points in the trace
	for(int i = 0; i < iNumPoints; i++)
	{
		point = env->CallObjectMethod(trace, g_scListClass.get, i);
		pPoints[i].nX = env->GetIntField(point, g_scPointClass.x);
		pPoints[i].nY = env->GetIntField(point, g_scPointClass.y);
		env->DeleteLocalRef(point);
	}

	LOGV("Copied point data");

	ret = controller->processTrace(keyboardLayoutId, pPoints, iNumPoints, shiftState);

	LOGV("Processed Trace");

	// destroying the trace points
	FREE(pPoints);

	LOGV("chinese_processTrace()...done - %s", ret ? "success" : "failed");

	return ret;
}

static bool isHasTraceInfo(JNIEnv* env, jobject object, jlong context)
{
    LOGV("isHasTraceInfo(context(0x%X))...", context);
    chinese_controller* controller = (chinese_controller*)context;
    bool ret;

    if (NULL == controller) {
        LOGE("isHasTraceInfo(context(0x%X))...failed null check", context);
        return false;
    }
    if (! config_init(env, object, g_sCnXt9Context))  {
        LOGD("config_init(context(0x%X))...authenticate failed", g_sCnXt9Context);
        return false;
    }	
    ret = controller->isHasTraceInfo();

    LOGV("isHasTraceInfo(context0x%X))...%s", context, ret ? "true" : "false");

    return ret;

}
static void backupWordSymbolInfo(JNIEnv* env, jobject object, jlong context)
{
    LOGV("backupWordSymbolInfo(context(0x%X))...", context);
    chinese_controller* controller = (chinese_controller*)context;

    if (NULL == controller) {
        LOGE("backupWordSymbolInfo(context(0x%X))...failed null check", context);
        return;
    }
    if (! config_init(env, object, g_sCnXt9Context))  {
        LOGD("config_init(context(0x%X))...authenticate failed", g_sCnXt9Context);
        return;
    }	
    controller->backupWordSymbolInfo();

    LOGV("backupWordSymbolInfo(context0x%X))..End", context);

    return;

}
static void restoreWordSymbolInfo(JNIEnv* env, jobject object, jlong context)
{
    LOGV("restoreWordSymbolInfo(context(0x%X))...", context);
    chinese_controller* controller = (chinese_controller*)context;
    //bool ret;

    if (NULL == controller) {
        LOGE("restoreWordSymbolInfo(context(0x%X))...failed null check", context);
        return;
    }
    if (! config_init(env, object, g_sCnXt9Context))  {
        LOGD("config_init(context(0x%X))...authenticate failed", g_sCnXt9Context);
        return;
    }	
    controller->restoreWordSymbolInfo();

    LOGV("restoreWordSymbolInfo(context0x%X))..End", context);

    return;

}

static bool chinese_clearKey(JNIEnv* env, jobject object, jlong context)
{
    LOGV("chinese_clearKey(context(0x%X))...", context);

    chinese_controller* controller = (chinese_controller*)context;
    bool ret;

    if (NULL == controller) {
        LOGE("chinese_clearKey(context(0x%X))...failed null check", context);
        return false;
    }
    if (! config_init(env, object, g_sCnXt9Context))  {
        LOGD("config_init(context(0x%X))...authenticate failed", g_sCnXt9Context);
        return false;
    }	
    ret = controller->clearKey();

    LOGV("chinese_clearKey(context0x%X))...%s", context, ret ? "succeeded" : "FAILED");

    return ret;
}

static bool chinese_clearAllKeys(JNIEnv* env, jobject object, jlong context)
{
    LOGV("chinese_clearAllKeys(context(0x%X))...", context);

    chinese_controller* controller = (chinese_controller*)context;
    bool ret;

    if (NULL == controller) {
        LOGE("chinese_clearAllKeys(context(0x%X))...failed null check", context);
        return false;
    }
    if (! config_init(env, object, g_sCnXt9Context))  {
        LOGD("config_init(context(0x%X))...authenticate failed", g_sCnXt9Context);
        return false;
    }	
    ret = controller->clearAllKeys();

    LOGV("chinese_clearAllKeys(context0x%X))...%s", context, ret ? "succeeded" : "FAILED");

    return ret;
}

static jint chinese_buildSelectionList(JNIEnv *env, jobject object, jlong context)
{
	LOGV("chinese_buildSelectionList(context(0x%X))...", context);

	int status = 0;
	chinese_controller* controller = (chinese_controller*)context;

	if (controller == 0) {
		LOGE("chinese_buildSelectionList(context(0x%X))...failed", context);
		return 0;
	}

	status = controller->_build();

	LOGV("chinese_buildSelectionList(context(0x%X))...%d", context, status);

	return status;
}
static bool chinese_addDelimiter(JNIEnv* env, jobject object, jlong context)
{
    LOGV("chinese_addDelimiter(context(0x%X))...", context);

    chinese_controller* controller = (chinese_controller*)context;
    bool ret;

    if (NULL == controller) {
        LOGE("chinese_addDelimiter(context(0x%X))...failed null check", context);
        return false;
    }
    ret = controller->addDelimiter();

    LOGV("chinese_addDelimiter(context0x%X))...%s", context, ret ? "succeeded" : "FAILED");

    return ret;
}

static bool chinese_addTone(JNIEnv* env, jobject object, jlong context, jint tone)
{
    LOGV("chinese_addTone(context(0x%X))...", context);

    chinese_controller* controller = (chinese_controller*)context;
    bool ret;

    if (NULL == controller) {
        LOGE("chinese_addTone(context(0x%X))...failed null check", context);
        return false;
    }
    if (! config_init(env, object, g_sCnXt9Context))  {
        LOGD("config_init(context(0x%X))...authenticate failed", g_sCnXt9Context);
        return false;
    }	
    ret = controller->addTone(tone);

    LOGV("chinese_addTone(context0x%X))...%s", context, ret ? "succeeded" : "FAILED");

    return ret;
}

static bool chinese_cycleTone(JNIEnv* env, jobject object, jlong context)
{
    LOGV("chinese_cycleTone(context(0x%X))...", context);

    chinese_controller* controller = (chinese_controller*)context;
    bool ret;

    if (NULL == controller) {
        LOGE("chinese_cycleTone(context(0x%X))...failed null check", context);
        return false;
    }
    ret = controller->cycleTone();

    LOGV("chinese_cycleTone(context0x%X))...%s", context, ret ? "succeeded" : "FAILED");

    return ret;
}

static int chinese_getKeyCount(JNIEnv* env, jobject object, jlong context)
{
    LOGV("chinese_getKeyCount(context(0x%X))...", context);

    chinese_controller* controller = (chinese_controller*)context;
    int ret;

    if (NULL == controller) {
        LOGE("chinese_getKeyCount(context(0x%X))...failed null check", context);
        return 0;
    }
    if (! config_init(env, object, g_sCnXt9Context))  {
        LOGD("config_init(context(0x%X))...authenticate failed", g_sCnXt9Context);
        return false;
    }	
    ret = controller->getKeyCount();

    LOGV("chinese_getKeyCount(context0x%X))...%d", context, ret);

    return ret;
}

static bool chinese_getWord(JNIEnv* env, jobject object, jlong context, jint wordIndex, jcharArray wordArray, jintArray wordLenArray, jint maxLen)
{
    LOGV("chinese_getWord(context(0x%X))...", context);

    chinese_controller* controller = (chinese_controller*)context;
    bool ret;

    if (NULL == controller) {
        LOGE("chinese_getWord(context(0x%X))...failed null check", context);
        return false;
    }
    jchar* word;
    jint* wordLen;
    if (! config_init(env, object, g_sCnXt9Context))  {
        LOGD("config_init(context(0x%X))...authenticate failed", g_sCnXt9Context);
        return false;
    }	

    word = env->GetCharArrayElements(wordArray, NULL);
    wordLen = env->GetIntArrayElements(wordLenArray, NULL);

    ret = controller->getWord((unsigned short)wordIndex, (unsigned short*)word, wordLen[0], maxLen);

    env->ReleaseCharArrayElements(wordArray, word, NULL);
    env->ReleaseIntArrayElements(wordLenArray, wordLen, NULL);

    LOGV("chinese_getWord(context0x%X))...%s", context, ret ? "succeeded" : "FAILED");

    return ret;
}

static bool chinese_getSelection(JNIEnv* env, jobject object, jlong context, jcharArray strArray, jintArray strLenArray, jint maxLen)
{
    LOGV("chinese_getSelection(context(0x%X))...", context);

    chinese_controller* controller = (chinese_controller*)context;
    bool ret;

    if (NULL == controller) {
        LOGE("chinese_getSelection(context(0x%X))...failed null check", context);
        return false;
    }
    jchar* str;
    jint* strLen;
    if (! config_init(env, object, g_sCnXt9Context))  {
        LOGD("config_init(context(0x%X))...authenticate failed", g_sCnXt9Context);
        return false;
    }	

    str = env->GetCharArrayElements(strArray, NULL);
    strLen = env->GetIntArrayElements(strLenArray, NULL);

    ret = controller->getSelection((unsigned short*)str, strLen[0], maxLen);

    env->ReleaseCharArrayElements(strArray, str, NULL);
    env->ReleaseIntArrayElements(strLenArray, strLen, NULL);

    LOGV("chinese_getSelection(context0x%X))...%s", context, ret ? "succeeded" : "FAILED");

    return ret;
}

static bool chinese_selectWord(JNIEnv* env, jobject object, jlong context, jint index, jcharArray insertTextArray, jintArray insertTextLenArray, jint maxLen)
{
    LOGV("chinese_selectWord(context(0x%X))...", context);

    chinese_controller* controller = (chinese_controller*)context;
    bool ret;

    if (NULL == controller) {
        LOGE("chinese_selectWord(context(0x%X))...failed null check", context);
        return false;
    }
    jchar* insertText;
    jint* insertTextLen;
    if (! config_init(env, object, g_sCnXt9Context))  {
        LOGD("config_init(context(0x%X))...authenticate failed", g_sCnXt9Context);
        return false;
    }	

    insertText = env->GetCharArrayElements(insertTextArray, NULL);
    insertTextLen = env->GetIntArrayElements(insertTextLenArray, NULL);

    ret = controller->selectWord(index, (unsigned short*)insertText, insertTextLen[0], maxLen);

    env->ReleaseCharArrayElements(insertTextArray, insertText, NULL);
    env->ReleaseIntArrayElements(insertTextLenArray, insertTextLen, NULL);

    LOGV("chinese_selectWord(context0x%X))...%s", context, ret ? "succeeded" : "FAILED");

    return ret;
}

static bool chinese_setContext(JNIEnv* env, jobject object, jlong context, jcharArray newContextArray, jint contextLen)
{
    LOGV("chinese_setContext(context(0x%X))...", context);

    chinese_controller* controller = (chinese_controller*)context;
    bool ret;

    if (NULL == controller) {
        LOGE("chinese_setContext(context(0x%X))...failed null check", context);
        return false;
    }
    jchar* newContext;
    if (! config_init(env, object, g_sCnXt9Context))  {
        LOGD("config_init(context(0x%X))...authenticate failed", g_sCnXt9Context);
        return false;
    }	

    newContext = env->GetCharArrayElements(newContextArray, NULL);

    ret = controller->setContext((const unsigned short*)newContext, contextLen);

    env->ReleaseCharArrayElements(newContextArray, newContext, NULL);

    LOGV("chinese_setContext(context0x%X))...%s", context, ret ? "succeeded" : "FAILED");

    return ret;
}

static bool chinese_breakContext(JNIEnv* env, jobject object, jlong context)
{
    LOGV("chinese_breakContext(context(0x%X))...", context);

    chinese_controller* controller = (chinese_controller*)context;
    bool ret;

    if (NULL == controller) {
        LOGE("chinese_breakContext(context(0x%X))...failed null check", context);
        return false;
    }
    if (! config_init(env, object, g_sCnXt9Context))  {
        LOGD("config_init(context(0x%X))...authenticate failed", g_sCnXt9Context);
        return false;
    }	
    ret = controller->breakContext();

    LOGV("chinese_breakContext(context0x%X))...%s", context, ret ? "succeeded" : "FAILED");

    return ret;
}

static bool chinese_resetUserDictionary(JNIEnv* env, jobject object, jlong context)
{
    LOGV("chinese_resetUserDictionary(context(0x%X))...", context);

    chinese_controller* controller = (chinese_controller*)context;
    bool ret;

    if (NULL == controller) {
        LOGE("chinese_resetUserDictionary(context(0x%X))...failed null check", context);
        return false;
    }
    if (! config_init(env, object, g_sCnXt9Context))  {
        LOGD("config_init(context(0x%X))...authenticate failed", g_sCnXt9Context);
        return false;
    }	
    ret = controller->resetUserDictionary();

    LOGV("chinese_resetUserDictionary(context0x%X))...%s", context, ret ? "succeeded" : "FAILED");

    return ret;
}

static bool chinese_addWordToUserDictionary(JNIEnv* env, jobject object, jlong context, jcharArray wordArray, jint wordLen, jcharArray spellingArray, jint spellingLen)
{
    LOGV("chinese_addWordToUserDictionary(context(0x%X))...", context);

    chinese_controller* controller = (chinese_controller*)context;
    bool ret;

    if (NULL == controller) {
        LOGE("chinese_addWordToUserDictionary(context(0x%X))...failed null check", context);
        return false;
    }
    jchar* word;
    jchar* spelling;

    word = env->GetCharArrayElements(wordArray, NULL);
    spelling = env->GetCharArrayElements(spellingArray, NULL);

    ret = controller->addWordToUserDictionary((const unsigned short*)word, wordLen, (const unsigned short*)spelling, spellingLen);

    env->ReleaseCharArrayElements(wordArray, word, NULL);
    env->ReleaseCharArrayElements(spellingArray, spelling, NULL);

    LOGV("chinese_addWordToUserDictionary(context0x%X))...%s", context, ret ? "succeeded" : "FAILED");

    return ret;
}

static bool chinese_getUserDictionaryWord(JNIEnv* env, jobject object, jlong context, jint index, jcharArray wordArray, jintArray wordLenArray, jint maxLen)
{
    LOGV("chinese_getUserDictionaryWord(context(0x%X))...", context);

    chinese_controller* controller = (chinese_controller*)context;
    bool ret;

    if (NULL == controller) {
        LOGE("chinese_getUserDictionaryWord(context(0x%X))...failed null check", context);
        return false;
    }
    jchar* word;
    jint* wordLen;

    word = env->GetCharArrayElements(wordArray, NULL);
    wordLen = env->GetIntArrayElements(wordLenArray, NULL);

    ret = controller->getUserDictionaryWord(index, (unsigned short*)word, wordLen[0], maxLen);

    env->ReleaseCharArrayElements(wordArray, word, NULL);
    env->ReleaseIntArrayElements(wordLenArray, wordLen, NULL);

    LOGV("chinese_getUserDictionaryWord(context0x%X))...%s", context, ret ? "succeeded" : "FAILED");

    return ret;
}

static bool chinese_deleteUserDictionaryWord(JNIEnv* env, jobject object, jlong context, jcharArray wordArray, jint wordLen)
{
    LOGV("chinese_deleteUserDictionaryWord(context(0x%X))...", context);

    chinese_controller* controller = (chinese_controller*)context;
    bool ret;

    if (NULL == controller) {
        LOGE("chinese_deleteUserDictionaryWord(context(0x%X))...failed null check", context);
        return false;
    }
    jchar* word;

    word = env->GetCharArrayElements(wordArray, NULL);

    ret = controller->deleteUserDictionaryWord((const unsigned short*)word, wordLen);

    env->ReleaseCharArrayElements(wordArray, word, NULL);

    LOGV("chinese_deleteUserDictionaryWord(context0x%X))...%s", context, ret ? "succeeded" : "FAILED");

    return ret;
}

// TODO need to remove the "spellIndex" argument
static bool chinese_getSpell(JNIEnv* env, jobject object, jlong context, jint spellIndex, jcharArray spellArray, jintArray spellLenArray, jint maxLen)
{
    LOGV("chinese_getSpell(context(0x%X))...", context);

    chinese_controller* controller = (chinese_controller*)context;
    bool ret;

    if (NULL == controller) {
        LOGE("chinese_getSpell(context(0x%X))...failed null check", context);
        return false;
    }
    jchar* spell;
    jint* spellLen;
    if (! config_init(env, object, g_sCnXt9Context))  {
        LOGD("config_init(context(0x%X))...authenticate failed", g_sCnXt9Context);
        return false;
    }	

    spell = env->GetCharArrayElements(spellArray, NULL);
    spellLen = env->GetIntArrayElements(spellLenArray, NULL);

    ret = controller->getSpell((unsigned short*)spell, spellLen[0], maxLen);

    env->ReleaseCharArrayElements(spellArray, spell, NULL);
    env->ReleaseIntArrayElements(spellLenArray, spellLen, NULL);

    LOGV("chinese_getSpell(context0x%X))...%s", context, ret ? "succeeded" : "FAILED");

    return ret;
}

static int chinese_getPrefixCount(JNIEnv* env, jobject object, jlong context)
{
    LOGV("chinese_getPrefixCount(context(0x%X))...", context);

    chinese_controller* controller = (chinese_controller*)context;
    int ret;

    if (NULL == controller) {
        LOGE("chinese_getPrefixCount(context(0x%X))...failed null check", context);
        return 0;
    }
    if (! config_init(env, object, g_sCnXt9Context))  {
        LOGD("config_init(context(0x%X))...authenticate failed", g_sCnXt9Context);
        return false;
    }	
    ret = controller->getPrefixCount();

    LOGV("chinese_getPrefixCount(context0x%X))...%d", context, ret);

    return ret;
}

static bool chinese_getPrefix(JNIEnv* env, jobject object, jlong context, jint prefixIndex, jcharArray prefixArray, jintArray prefixLenArray, jint maxLen)
{
    LOGV("chinese_getPrefix(context(0x%X))...", context);

    chinese_controller* controller = (chinese_controller*)context;
    bool ret;

    if (NULL == controller) {
        LOGE("chinese_getPrefix(context(0x%X))...failed null check", context);
        return false;
    }
    jint* prefixLen;
    jchar * prefix;
    if (! config_init(env, object, g_sCnXt9Context))  {
        LOGD("config_init(context(0x%X))...authenticate failed", g_sCnXt9Context);
        return false;
    }	

    prefix = env->GetCharArrayElements(prefixArray, NULL);
    prefixLen = env->GetIntArrayElements(prefixLenArray, NULL);

    ret = controller->getPrefix(prefixIndex, (unsigned short*)prefix, prefixLen[0], maxLen);

    env->ReleaseCharArrayElements(prefixArray, prefix, NULL);
    env->ReleaseIntArrayElements(prefixLenArray, prefixLen, NULL);

    LOGV("chinese_getPrefix(context0x%X))...%s", context, ret ? "succeeded" : "FAILED");

    return ret;
}

static int chinese_getActivePrefixIndex(JNIEnv* env, jobject object, jlong context)
{
    LOGV("chinese_getActivePrefixIndex(context(0x%X))...", context);

    chinese_controller* controller = (chinese_controller*)context;
    int ret;

    if (NULL == controller) {
        LOGE("chinese_getActivePrefixIndex(context(0x%X))...failed null check", context);
        return 0;
    }
    if (! config_init(env, object, g_sCnXt9Context))  {
        LOGD("config_init(context(0x%X))...authenticate failed", g_sCnXt9Context);
        return false;
    }	
    ret = controller->getActivePrefixIndex();

    LOGV("chinese_getActivePrefixIndex(context0x%X))...%d", context, ret);

    return ret;
}

static bool chinese_setActivePrefixIndex(JNIEnv* env, jobject object, jlong context, jint prefixIndex)
{
    LOGV("chinese_setActivePrefixIndex(context(0x%X))...", context);

    chinese_controller* controller = (chinese_controller*)context;
    bool ret;

    if (NULL == controller) {
        LOGE("chinese_setActivePrefixIndex(context(0x%X))...failed null check", context);
        return false;
    }
    if (! config_init(env, object, g_sCnXt9Context))  {
        LOGD("config_init(context(0x%X))...authenticate failed", g_sCnXt9Context);
        return false;
    }	
    ret = controller->setActivePrefixIndex(prefixIndex);

    LOGV("chinese_setActivePrefixIndex(context0x%X))...%s", context, ret ? "succeeded" : "FAILED");

    return ret;
}

// TODO this function will be removed in the future
static bool chinese_isPrefixBufferActive(JNIEnv* env, jobject object, jlong context)
{
    LOGV("chinese_isPrefixBufferActive(context(0x%X))...", context);
#if 0
    chinese_controller* controller = (chinese_controller*)context;
    bool ret;

    if (NULL == controller) {
        LOGE("chinese_isPrefixBufferActive(context(0x%X))...failed null check", context);
        return 0;
    }
    ret = controller->isPrefixBufferActive();

    LOGV("chinese_isPrefixBufferActive(context0x%X))...%d", context, ret);

    return ret;
#endif
    return true;
}

static bool chinese_udbAdd(JNIEnv* env, jobject object, jlong context,
        jcharArray phraseArray, jint phraseLen, jcharArray spellArray, jint spellLen)
{
    chinese_controller* controller = (chinese_controller*)context;

    if (controller == 0) {
        return false;
    }

    jchar* phrase = env->GetCharArrayElements(phraseArray, NULL);
    jchar* spell  = env->GetCharArrayElements(spellArray, NULL);

    bool ret = controller->udbAdd(phrase, phraseLen, spell, spellLen);

    env->ReleaseCharArrayElements(phraseArray, phrase, NULL);
    env->ReleaseCharArrayElements(spellArray, spell, NULL);

    return ret;
}

static bool chinese_udbDelete(JNIEnv* env, jobject object, jlong context, jcharArray phraseArray, jint len)
{
    chinese_controller* controller = (chinese_controller*)context;

    if (controller == 0) {
        return false;
    }

    jchar* phrase = env->GetCharArrayElements(phraseArray, NULL);

    bool ret = controller->udbDelete(phrase, len);

    env->ReleaseCharArrayElements(phraseArray, phrase, NULL);

    return ret;
}

static bool chinese_udbGetNext(JNIEnv* env, jobject object, jlong context, jint index,
        jcharArray phraseArray, jintArray phraseLengthArray, int maxPhraseLen,
        jcharArray spellArray, jintArray spellLengthArray, int maxSpellLen)
{
    chinese_controller* controller = (chinese_controller*)context;

    if (controller == 0) {
        return false;
    }

    jchar* phrase = env->GetCharArrayElements(phraseArray, NULL);
    jint*  phraseLen = env->GetIntArrayElements(phraseLengthArray, NULL);
    jchar* spell = env->GetCharArrayElements(spellArray, NULL);
    jint*  spellLen = env->GetIntArrayElements(spellLengthArray, NULL);

    phraseLen[0] = 0;
    spellLen[0] = 0;

    bool ret = controller->udbGetNext(index, phrase, phraseLen[0], maxPhraseLen,
            spell, spellLen[0], maxSpellLen);

    env->ReleaseCharArrayElements(phraseArray, phrase, NULL);
    env->ReleaseIntArrayElements(phraseLengthArray, phraseLen, NULL);
    env->ReleaseCharArrayElements(spellArray, spell, NULL);
    env->ReleaseIntArrayElements(spellLengthArray, spellLen, NULL);

    return ret;
}

static int chinese_udbCount(JNIEnv* env, jobject object, jlong context)
{
    chinese_controller* controller = (chinese_controller*)context;
    if (controller == 0) {
        return 0;
    }

    return controller->udbCount();
}

static void chinese_udbReset(JNIEnv* env, jobject object, jlong context)
{
    chinese_controller* controller = (chinese_controller*)context;
    if (controller == 0) {
        return;
    }

    controller->udbReset();
}

static int chinese_setCommonChar(JNIEnv* env, jobject object, jlong context)
{
	chinese_controller* controller = (chinese_controller*)context;
	if (controller == 0) {
		return -1;
	}
    if (! config_init(env, object, g_sCnXt9Context))  {
        LOGD("config_init(context(0x%X))...authenticate failed", g_sCnXt9Context);
        return false;
    }	

	return controller->setCommonChar();
}

static int chinese_clearCommonChar(JNIEnv* env, jobject object, jlong context)
{
	chinese_controller* controller = (chinese_controller*)context;
	if (controller == 0) {
		return -1;
	}
    if (! config_init(env, object, g_sCnXt9Context))  {
        LOGD("config_init(context(0x%X))...authenticate failed", g_sCnXt9Context);
        return false;
    }	

	return controller->clearCommonChar();
}

static int chinese_setFullSentence(JNIEnv* env, jobject object, jlong context)
{
	chinese_controller* controller = (chinese_controller*)context;
	if (controller == 0) {
		return -1;
	}
    if (! config_init(env, object, g_sCnXt9Context))  {
        LOGD("config_init(context(0x%X))...authenticate failed", g_sCnXt9Context);
        return false;
    }

	return controller->setFullSentence();
}

static int chinese_clearFullSentence(JNIEnv* env, jobject object, jlong context)
{
	chinese_controller* controller = (chinese_controller*)context;
	if (controller == 0) {
		return -1;
	}
    if (! config_init(env, object, g_sCnXt9Context))  {
        LOGD("config_init(context(0x%X))...authenticate failed", g_sCnXt9Context);
        return false;
    }

	return controller->clearFullSentence();
}

static bool chinese_isFullSentenceActive(JNIEnv* env, jobject object, jlong context)
{
    chinese_controller* controller = (chinese_controller*)context;
    if (controller == 0) {
        return false;
    }
    if (! config_init(env, object, g_sCnXt9Context))  {
        LOGD("config_init(context(0x%X))...authenticate failed", g_sCnXt9Context);
        return false;
    }

    return controller->isFullSentenceActive();
}

int chinese_getHomophonePhraseCount(JNIEnv *env, jobject object, jlong context, jcharArray basePhraseArray, jint basePhraseLen)
{
	chinese_controller* controller = (chinese_controller*)context;
	if (controller == 0) {
		return -1;
	}
    if (! config_init(env, object, g_sCnXt9Context))  {
        LOGD("config_init(context(0x%X))...authenticate failed", g_sCnXt9Context);
        return false;
    }

    jchar* basePhrase = env->GetCharArrayElements(basePhraseArray, NULL);

    int count = 0;
    int status = controller->getHomophonePhraseCount(basePhrase, basePhraseLen, &count);

    env->ReleaseCharArrayElements(basePhraseArray, basePhrase, NULL);

    if (status) {
    	return -1;
    } else {
    	return count;
    }
}

int chinese_getHomophonePhrase(JNIEnv *env, jobject object, jlong context, jcharArray basePhraseArray, jint basePhraseLen, jint index, jcharArray phraseArray, jcharArray spellArray)
{
	chinese_controller* controller = (chinese_controller*)context;
	if (controller == 0) {
		return -1;
	}
    if (! config_init(env, object, g_sCnXt9Context))  {
        LOGD("config_init(context(0x%X))...authenticate failed", g_sCnXt9Context);
        return false;
    }

    jchar* basePhrase = env->GetCharArrayElements(basePhraseArray, NULL);
    jchar* phrase = env->GetCharArrayElements(phraseArray, NULL);
    jchar *spell = NULL;
    if (spellArray != NULL)
    {
    	spell = env->GetCharArrayElements(spellArray, NULL);
    }

    int status = controller->getHomophonePhrase(basePhrase, basePhraseLen, index, phrase, spell);

    env->ReleaseCharArrayElements(basePhraseArray, basePhrase, NULL);
    env->ReleaseCharArrayElements(phraseArray, phrase, NULL);
    if (spellArray != NULL)
    {
    	env->ReleaseCharArrayElements(spellArray, spell, NULL);
    }

	return status;
}

// ----------------------------------------------------------------------------

static JNINativeMethod gChineseMethods[] = {
    {"mocainput_chinese_create", "(Ljava/lang/String;)J", (void*)chinese_create},
    {"mocainput_chinese_destroy", "(J)V", (void*)chinese_destroy},
    {"mocainput_chinese_start", "(JLandroid/content/Context;)Z", (void*)chinese_start},
    {"mocainput_chinese_finish", "(J)V", (void*)chinese_finish},
    {"mocainput_chinese_initTrace","(JIII)V",(void*)chinese_initTrace},
    {"mocainput_chinese_isAutoSpaceBeforeTrace","(JILjava/util/List;)I",(void*)chinese_isAutoSpaceBeforeTrace},
    {"mocainput_chinese_setAttribute", "(JII)Z", (void*)chinese_setAttribute},
    {"mocainput_chinese_setLanguage", "(JI)Z", (void*)chinese_setLanguage},
    {"mocainput_chinese_getInputMode", "(J)I", (void*)chinese_getInputMode},
    {"mocainput_chinese_setInputMode", "(JI)Z", (void*)chinese_setInputMode},
    {"mocainput_chinese_processKey", "(JII)Z", (void*)chinese_processKey},
    {"mocainput_chinese_processTap", "(JIIII)Z",(void*)chinese_processTap},
    {"mocainput_chinese_processTrace", "(JILjava/util/List;I)Z", (void*)chinese_processTrace},
    {"mocainput_isHasTraceInfo", "(J)Z", (void*)isHasTraceInfo},
    {"mocainput_backupWordSymbolInfo", "(J)V", (void*)backupWordSymbolInfo},
    {"mocainput_restoreWordSymbolInfo", "(J)V", (void*)restoreWordSymbolInfo},
    {"mocainput_chinese_clearKey", "(J)Z", (void*)chinese_clearKey},
    {"mocainput_chinese_clearAllKeys", "(J)Z", (void*)chinese_clearAllKeys},
	{"mocainput_chinese_buildSelectionList", "(J)I",  (void*)chinese_buildSelectionList},
    {"mocainput_chinese_addDelimiter", "(J)Z", (void*)chinese_addDelimiter},
    {"mocainput_chinese_addTone", "(JI)Z", (void*)chinese_addTone},
    {"mocainput_chinese_cycleTone", "(J)Z", (void*)chinese_cycleTone},
    {"mocainput_chinese_getKeyCount", "(J)I", (void*)chinese_getKeyCount},
    {"mocainput_chinese_getWord", "(JI[C[II)Z", (void*)chinese_getWord},
    {"mocainput_chinese_getSelection", "(J[C[II)Z", (void*)chinese_getSelection},
    {"mocainput_chinese_selectWord", "(JI[C[II)Z", (void*)chinese_selectWord},
    {"mocainput_chinese_setContext", "(J[CI)Z", (void*)chinese_setContext},
    {"mocainput_chinese_breakContext", "(J)Z", (void*)chinese_breakContext},
    {"mocainput_chinese_resetUserDictionary", "(J)Z", (void*)chinese_resetUserDictionary},
    {"mocainput_chinese_addWordToUserDictionary", "(J[CI[CI)Z", (void*)chinese_addWordToUserDictionary},
    {"mocainput_chinese_getUserDictionaryWord", "(JI[C[II)Z", (void*)chinese_getUserDictionaryWord},
    {"mocainput_chinese_deleteUserDictionaryWord", "(J[CI)Z", (void*)chinese_deleteUserDictionaryWord},
    {"mocainput_chinese_getSpell", "(JI[C[II)Z", (void*)chinese_getSpell},
    {"mocainput_chinese_getPrefix", "(JI[C[II)Z", (void*)chinese_getPrefix},
    {"mocainput_chinese_getActivePrefixIndex", "(J)I", (void*)chinese_getActivePrefixIndex},
    {"mocainput_chinese_setActivePrefixIndex", "(JI)Z", (void*)chinese_setActivePrefixIndex},
    {"mocainput_chinese_getPrefixCount", "(J)I", (void*)chinese_getPrefixCount},
    {"mocainput_chinese_isPrefixBufferActive", "(J)Z", (void*)chinese_isPrefixBufferActive},

    {"mocainput_chinese_udbAdd", "(J[CI[CI)Z", (void*)chinese_udbAdd},
    {"mocainput_chinese_udbDelete", "(J[CI)Z", (void*)chinese_udbDelete},
    {"mocainput_chinese_udbGetNext", "(JI[C[II[C[II)Z", (void*)chinese_udbGetNext},
    {"mocainput_chinese_udbCount", "(J)I", (void*)chinese_udbCount},
    {"mocainput_chinese_udbReset", "(J)V", (void*)chinese_udbReset},

    {"mocainput_chinese_setCommonChar", "(J)I", (void*)chinese_setCommonChar},
    {"mocainput_chinese_clearCommonChar", "(J)I", (void*)chinese_clearCommonChar},

    {"mocainput_chinese_setFullSentence", "(J)I", (void*)chinese_setFullSentence},
    {"mocainput_chinese_clearFullSentence", "(J)I", (void*)chinese_clearFullSentence},
    {"mocainput_chinese_isFullSentenceActive", "(J)Z", (void*)chinese_isFullSentenceActive},

    {"mocainput_chinese_getHomophonePhraseCount", "(J[CI)I", (void*)chinese_getHomophonePhraseCount},
    {"mocainput_chinese_getHomophonePhrase", "(J[CII[C[C)I", (void*)chinese_getHomophonePhrase}
};

int registerChineseNative(JNIEnv *env)
{
	const char* className = "com/moca/input/NativeChineseInput";
    jclass clazz = env->FindClass(className);
    if (clazz == NULL) {
        fprintf(stderr, "Chinese Native registration unable to find class '%s'\n", className);
        return JNI_FALSE;
    }

    if (env->RegisterNatives(clazz, gChineseMethods, sizeof(gChineseMethods) / sizeof(gChineseMethods[0])) < 0) {
        fprintf(stderr, "Chinese RegisterNatives failed for '%s'\n", className);
        return JNI_FALSE;
    }

    return JNI_TRUE;
}

} // namespace mocainput

