
#include "Log.h"

#include <stdlib.h>
#include <sys/stat.h>
#include <stdio.h>

#include "write_chinese.h"
#include "load_file.h"
#include "mem_alloc.h"
#include "write_log.h"

namespace mocainput {

#ifdef ENABLED_LOGGING

static const char* gLogFileName = "/data/data/com.moca.input/files/chinese_hwr_userlog.txt";
static write_log* gWriteLog = 0;

static void _logThis(void* pUserData, const char* pLogString, DECUMA_UINT32 nLogStringLength)
{
	write_log* log = reinterpret_cast<write_log*>(pUserData);
	if (log != 0) {
		log->log(pLogString, nLogStringLength);
	}
}

#endif

static void* _thisMalloc(size_t size, void * pPrivate)
{
	return MALLOC(size);
}

static void* _thisCalloc(size_t elements, size_t size, void * pPrivate)
{
	return CALLOC(elements, size);
}

static void _thisFree(void * ptr, void * pPrivate)
{
	FREE(ptr);
}

static int wordCopy(unsigned short* dest, const unsigned short* src, int len, int maxLength) {
    int i = 0;
	for (i = 0; i < len && i < maxLength; i++) {
        dest[i] = src[i];
    }

    return i;
}

Write_Chinese::Write_Chinese(DBRegistry* dbRegistry, chinese_data* xt9_chinese_data)
{
    mSessionSettings = (DECUMA_SESSION_SETTINGS*)CALLOC(1, sizeof(DECUMA_SESSION_SETTINGS));
    mSessionSettings->charSet.pLanguages = (DECUMA_UINT32*)CALLOC(sizeof(DECUMA_UINT32), 4);
    mSessionSettings->charSet.pLanguages[0] = DECUMA_LANG_PRC;
    mSessionSettings->charSet.nLanguages = 1;

    mRecognitionSettings = (DECUMA_RECOGNITION_SETTINGS*)CALLOC(1, sizeof(DECUMA_RECOGNITION_SETTINGS));
    mSession = (DECUMA_SESSION*)CALLOC(decumaCJKGetSessionSize(), sizeof(DECUMA_UINT8));

    mRecognitionCandidates = (DECUMA_HWR_RESULT*)CALLOC(MAX_RECOGNIZE_CANDIDATES, sizeof(DECUMA_HWR_RESULT));
    for (int i = 0; i < MAX_RECOGNIZE_CANDIDATES; i++) {
    	mRecognitionCandidates[i].pChars = (DECUMA_UNICODE*)CALLOC(MAX_CHARACTERS + 1, sizeof(DECUMA_UNICODE));
    	mRecognitionCandidates[i].pSymbolStrokes = (DECUMA_INT16*)CALLOC(MAX_CHARACTERS + 1, sizeof(DECUMA_INT16));
    }

    mMemFunctions.pMalloc = _thisMalloc;
    mMemFunctions.pCalloc = _thisCalloc;
    mMemFunctions.pFree = _thisFree;
    mMemFunctions.pMemUserData = NULL;

    mDatabase = 0;
    mCurrentLanguageID = 0;

    mDbRegistry = dbRegistry;
    mXT9ChineseData = xt9_chinese_data;
}

Write_Chinese::~Write_Chinese()
{
	FREE(mSessionSettings->charSet.pSymbolCategories);
	FREE(mSessionSettings->charSet.pLanguages);
	FREE(mSessionSettings);
	FREE(mRecognitionSettings);
	FREE(mSession);

    for (int i = 0; i < MAX_RECOGNIZE_CANDIDATES; i++) {
    	FREE(mRecognitionCandidates[i].pSymbolStrokes);
    	FREE(mRecognitionCandidates[i].pChars);
    }
	FREE(mRecognitionCandidates);
}

int Write_Chinese::start(int languageID)
{
    LOGV("Write_Chinese::start(%X)...", languageID);

    int status = 0;

    mArcID = 0;

	status = mXT9ChineseData->setLanguage(languageID);

    if (mCurrentLanguageID != languageID) {
        mCurrentLanguageID = languageID;

        FILE *fp = fopen(mDbRegistry->get_hwr_db_template_path(mCurrentLanguageID), "rb");
        if (fp == 0) {
            LOGE("setDatabase()...FAILED to open file");
            status = decumaInvalidDatabase;
        }
        else {
            loadDatabase(fp, file_size(fp));
            fclose(fp);

        	decumaCJKEndSession(mSession);

            mSessionSettings->pStaticDB = (DECUMA_STATIC_DB_PTR)mDatabase;
            setLanguagesBaseOnCategories();
            status = decumaCJKBeginSession(mSession, mSessionSettings, &mMemFunctions);
        }
    }

#ifdef ENABLED_LOGGING
	gWriteLog = new write_log(gLogFileName);
	decumaCJKStartLogging(mSession, gWriteLog, _logThis);
#endif

    LOGV("Write_Chinese::start(%X)...status = %d", languageID, status);

    return status;
}

void Write_Chinese::setLanguagesBaseOnCategories()
{
    mSessionSettings->charSet.nLanguages = 0;

    bool needAlpha = false;
    bool needChinese = false;
    for (int k = 0; k < mSessionSettings->charSet.nSymbolCategories; k++) {
    	int cat = mSessionSettings->charSet.pSymbolCategories[k];

    	if  (cat <= DECUMA_CATEGORY_SUPPLEMENTAL_PUNCTUATIONS) {
    		needAlpha = true;
    	}
    	else if ((cat >= DECUMA_CATEGORY_GB2312_A && cat <= DECUMA_CATEGORY_BIGFIVE_LEVEL_2) ||
    			(cat == DECUMA_CATEGORY_BIGFIVE || cat == DECUMA_CATEGORY_GB2312_B || cat == DECUMA_CATEGORY_GB2312)) {
    		needChinese = true;
    	}
    }

    if (needChinese) {
    	// better if we don't use language id at all and just use categories to determine which language to use
		if (0x00E1 == mCurrentLanguageID) {
			// simplified
			mSessionSettings->charSet.pLanguages[mSessionSettings->charSet.nLanguages++] = DECUMA_LANG_PRC;
		}
		else {
			// traditional
			mSessionSettings->charSet.pLanguages[mSessionSettings->charSet.nLanguages++] = DECUMA_LANG_TW;
		}
    }

    if (needAlpha) {
        mSessionSettings->charSet.pLanguages[mSessionSettings->charSet.nLanguages++] = DECUMA_LANG_GSMDEFAULT;
        mSessionSettings->charSet.pLanguages[mSessionSettings->charSet.nLanguages++] = DECUMA_LANG_EN;
    }

    LOGV("Write_Chinese::setLanguageAndCategories()... num of categories = %d",  mSessionSettings->charSet.nSymbolCategories);
    LOGV("Write_Chinese::setLanguageAndCategories()... num of languages = %d", mSessionSettings->charSet.nLanguages);

    for (int i = 0; i < mSessionSettings->charSet.nSymbolCategories; i++) {
    	int supported = 0;
    	int status = decumaCJKDatabaseIsCategorySupported(mSessionSettings->pStaticDB, mSessionSettings->charSet.pSymbolCategories[i], &supported);
    	if (!supported || status != 0) {
    		LOGE("Write_Chinese::start()... cat(%d) is not supported", mSessionSettings->charSet.pSymbolCategories[i]);
    	}
    }
}

int Write_Chinese::applySettingChanges() {

	setLanguagesBaseOnCategories();

    int status = decumaCJKChangeSessionSettings(mSession, mSessionSettings);

    LOGV("Write_Chinese::applySettingChanges()...status = %d", status);
    return status;
}

int Write_Chinese::noteSelectedCandidate(int wordIndex)
{
	int status = 0;

#ifdef ENABLED_LOGGING
	LOGV("Write_Chinese::noteSelectedCandidate(%d)...", wordIndex);

	DECUMA_UNICODE word[MAX_CHARACTERS + 1];
	int len;
	int gesture;

	if ((status = getCandidate(wordIndex, word, MAX_CHARACTERS, len, gesture)) == 0) {
		decumaCJKLogAcceptedResult(mSession, word, len);
	}

	LOGV("Write_Chinese::noteSelectedCandidate(%d)...status = %d", wordIndex, status);

#endif

	return status;
}

int Write_Chinese::finish()
{
#ifdef ENABLED_LOGGING
    decumaCJKStopLogging(mSession);
#endif

	decumaCJKEndSession(mSession);
	mCurrentLanguageID = 0;
	mArcID = 0;
	FREE(mDatabase);

#ifdef ENABLED_LOGGING
	delete gWriteLog;
	gWriteLog = 0;
#endif

	return 0;
}

void Write_Chinese::loadDatabase(FILE* fp, int length)
{
    LOGV("loadDatabase()...");

    FREE(mDatabase);
    mDatabase = MALLOC(length);
    if (mDatabase) {
        if (fread(mDatabase, 1, length, fp) != (size_t)length) {
            LOGE("loadDatabase()...fread FAILED");
        }
    }

    LOGV("loadDatabase()...file size = %.00fKB", (float)(length/1024));
}

int Write_Chinese::beginArc()
{
    LOGV("Write_Alpha::beginArc(()...");

    int status = decumaCJKBeginArcAddition(mSession);

    // don't care
    if (status == decumaArcAdditionSeqAlreadyStarted) {
    	status = 0;
    }

    LOGV("Write_Alpha::beginArc(()...status = %d", status);

    return status;
}

int Write_Chinese::addPoint(int arcId, int x, int y)
{
    return decumaCJKAddPoint(mSession, x, y, arcId);
}

int Write_Chinese::startNewArc(int& arcId) {
    int status = decumaCJKStartNewArc(mSession, ++mArcID);
    arcId = mArcID;

    LOGV("Write_Chinese::startNewArc()...arcID = %d, status = %d", arcId, status);

    return status;
}

int Write_Chinese::commitArc(int ardID) {
	LOGV("Write_Chinese::commitArc(%d)...", ardID);

    int status = decumaCJKCommitArc(mSession, ardID);

    LOGV("Write_Chinese::commitArc()...status = %d", status);

    return status;
}

int Write_Chinese::endArc()
{
	LOGV("Write_Alpha::endArc(()...");

    mArcID = 0;

    int status = decumaCJKEndArcAddition(mSession);

    // don't care
    if (status == decumaArcAdditionSeqNotStarted) {
    	status = 0;
    }

    LOGV("Write_Alpha::endArc(()...status = %d", status);

    return status;
}

int Write_Chinese::recognize(unsigned short *startOfWord, int& resultCount)
{
    LOGD("Write_Chinese::recognize()...");

    int status = 0;
    mRecognitionCandidateCount = 0;
    mRecognitionSettings->pStringStart = (DECUMA_UNICODE*)startOfWord;

    mRecognitionSettings->stringCompleteness = canBeContinued;
    mRecognitionSettings->boostLevel = noBoost;
    status = decumaCJKRecognize(mSession, mRecognitionCandidates, MAX_RECOGNIZE_CANDIDATES, (DECUMA_UINT16*)&mRecognitionCandidateCount, MAX_CHARACTERS, mRecognitionSettings, NULL);
    resultCount = mRecognitionCandidateCount;

    return status;
}

int Write_Chinese::getCandidate(int index, DECUMA_UNICODE* word, int maxLength, int& length, int & bEndsWithInstGest)
{
	LOGV("Write_Chinese::getCandidate(%d)...", index);

	if (!(0 <= index && index < mRecognitionCandidateCount)) {
	    LOGE("Write_Alpha::getCandidate(%d)...error", index);
		return decumaInvalidIndex;
	}

	length = wordCopy(word, mRecognitionCandidates[index].pChars, mRecognitionCandidates[index].nChars, maxLength);
	bEndsWithInstGest = mRecognitionCandidates[index].bInstantGesture;

	LOGV("Write_Chinese::getCandidate(%d)...word length = %d", index, length);

	return 0;
}

const char* Write_Chinese::getVersion() const
{
    return (const char*)decumaCJKGetProductVersion();
}

const char* Write_Chinese::getDatabaseVersion() const
{
	return 0;
    // return (const char*)dltGetDatabaseVersion(mMemory);
}

int Write_Chinese::setContext(const unsigned short* context, int contextLen)
{
    return mXT9ChineseData->setContext(context, contextLen);
}

int Write_Chinese::getWord(int wordIndex, unsigned short* word, int& wordLen, int maxLen)
{
    LOGD("Write_Chinese::getWord(%d)...", wordIndex);
    mXT9ChineseData->clearAllKeys();
    int status =  (int)mXT9ChineseData->getWord(wordIndex, word, wordLen, maxLen);
    if (status == ET9STATUS_NEED_SELLIST_BUILD) {
        status =  (int)mXT9ChineseData->buildWordList();
        status =  (int)mXT9ChineseData->getWord(wordIndex, word, wordLen, maxLen);
    }
    LOGD("Write_Chinese::getWord(%d)...  status = %d", wordIndex, status);
    return status;
}

int Write_Chinese::setAttribute(int id, int value)
{
    int status;
    mXT9ChineseData->clearAllKeys();
    status = mXT9ChineseData->setAttribute(id, value);
    return status;
}

int Write_Chinese::setCommonChar()
{
	LOGD("WriteChinese::setCommonChar()");
	int status = mXT9ChineseData->setCommonChar();
	LOGD("WriteChinese::setCommonChar()... status = %d", status);
	return status;
}

int Write_Chinese::clearCommonChar()
{
	LOGD("WriteChinese::clearCommonChar()");
	int status = mXT9ChineseData->clearCommonChar();
	LOGD("WriteChinese::clearCommonChar()... status = %d", status);
	return status;
}

} // namespace mocainput

