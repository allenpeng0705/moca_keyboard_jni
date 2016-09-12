

#include "Log.h"

#include <stdlib.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "ucs2.h"
#include "write_alpha.h"
#include "load_file.h"
#include "xt9lang_id_to_t9write.h"
#include "ucs2.h"
#include "mem_alloc.h"
#include "write_log.h"

namespace mocainput {

#ifdef ENABLED_LOGGING

static const char* gLogFileName = "/data/data/com.moca.input/files/alpha_hwr_userlog.txt";
static write_log* gWriteLog = 0;

static void _thisLog(void* pUserData, const char* pLogString, DECUMA_UINT32 nLogStringLength)
{
	write_log* log = reinterpret_cast<write_log*>(pUserData);
	if (log != 0) {
		log->log(pLogString, nLogStringLength);
	}
}

#endif

static void* thisMalloc(size_t size, void * pPrivate)
{
	return MALLOC(size);
}

static void* thisCalloc(size_t elements, size_t size, void * pPrivate)
{
	return CALLOC(elements, size);
}

static void thisFree(void * ptr, void * pPrivate)
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

Write_Alpha::Write_Alpha(DBRegistry* dbRegistry, alpha_data* xt9_alpha_data)
{
	LOGV("Write_Alpha::Write_Alpha()...");

	mInstantGestureSettings.widthThreshold = 0;
	mInstantGestureSettings.heightThreshold = 0;

    mSessionSettings = (DECUMA_SESSION_SETTINGS*)CALLOC(1, sizeof(DECUMA_SESSION_SETTINGS));
    mSessionSettings->charSet.pLanguages = (DECUMA_UINT32*)CALLOC(sizeof(DECUMA_UINT32), 4);
    mSessionSettings->charSet.pLanguages[0] = DECUMA_LANG_EN;
    mSessionSettings->charSet.nLanguages = 1;

    mRecognitionSettings = (DECUMA_RECOGNITION_SETTINGS*)CALLOC(1, sizeof(DECUMA_RECOGNITION_SETTINGS));
    mSession = (DECUMA_SESSION*)CALLOC(decumaGetSessionSize(), sizeof(DECUMA_UINT8));
    mLanguageDatabase = 0;
    mTemplateDatabase = 0;
    mUDB = 0;

    mMemFunctions.pMalloc = thisMalloc;
    mMemFunctions.pCalloc = thisCalloc;
    mMemFunctions.pFree = thisFree;
    mMemFunctions.pMemUserData = NULL;

    // memory for mix candidate list that contain both result from hwr and xt9
    for (int i = 0; i < MAX_MIX_CANDIDATES; i++) {
    	mMixCandidates[i] = (Candidate*)CALLOC(1, sizeof(Candidate));
    	mMixCandidatePositions[i] = 0;
    }

    // memory just for hwr recognition list, not that word buffer is mix list word buffer
    mRecognitionCandidates = (DECUMA_HWR_RESULT*)CALLOC(MAX_RECOGNIZE_CANDIDATES, sizeof(DECUMA_HWR_RESULT));
    for (int i = 0; i < MAX_RECOGNIZE_CANDIDATES; i++) {
    	mRecognitionCandidates[i].pChars = &mMixCandidates[i]->word[0];
    	mRecognitionCandidates[i].pSymbolStrokes = (DECUMA_INT16*)CALLOC(MAX_CHARACTERS + 1, sizeof(DECUMA_INT16));
    }

    mDBRegistry = dbRegistry;
    mXT9AlphaData = xt9_alpha_data;

    mArcID = 0;
    mCurrentLanguageID = 0;
    mCurrentTemplateDatabaseID = 0;
    mMixCandidateCount = 0;
    mRecognitionCandidateCount = 0;

    LOGV("Write_Alpha::Write_Alpha()...done");
}

Write_Alpha::~Write_Alpha()
{
	LOGV("Write_Alpha::~Write_Alpha()...");

	FREE(mSessionSettings->charSet.pSymbolCategories);
	FREE(mSessionSettings->charSet.pLanguages);
	FREE(mSessionSettings);
	FREE(mRecognitionSettings);
	FREE(mSession);
	FREE(mLanguageDatabase);
	FREE(mTemplateDatabase);
	FREE(mUDB);

    for (int i = 0; i < MAX_MIX_CANDIDATES; i++) {
    	FREE(mMixCandidates[i]);
    }

    for (int i = 0; i < MAX_RECOGNIZE_CANDIDATES; i++) {
    	FREE(mRecognitionCandidates[i].pSymbolStrokes);
    }

    FREE(mRecognitionCandidates);

    LOGV("Write_Alpha::~Write_Alpha()...done");
}

int Write_Alpha::applySettingChanges() {

    int status = decumaChangeSessionSettings(mSession, mSessionSettings);

    LOGV("Write_Alpha::applySettingChanges()...status = %d", status);
    return status;
}

int Write_Alpha::start(int languageID)
{
    LOGV("Write_Alpha::start(%X)...", languageID);

    mXT9AlphaData->start();
	mMixCandidateCount = 0;
	mRecognitionCandidateCount = 0;
	mArcID = 0;

	int status = 0;

	if (mCurrentTemplateDatabaseID != languageID || mCurrentLanguageID != languageID) {
		// Basically, the database must be set first, the begin the session then can attach new language
		if ((status = setTemplateDatabase(languageID)) == 0) {
			if ((status = decumaBeginSession(mSession, mSessionSettings, &mMemFunctions)) == 0) {
				status = setLanguageDatabase(languageID);
			}
		}
	}

	convertXT9UserDictionary();

#ifdef ENABLED_LOGGING
	gWriteLog = new write_log(gLogFileName);
	decumaStartLogging(mSession, gWriteLog, _thisLog);
#endif

	LOGV("start(%X)...status = %d", languageID, status);

    return status;
}

int Write_Alpha::finish()
{
	LOGV("Write_Alpha::finish()...");

	if (mXT9AlphaData) {
		mXT9AlphaData->finish();
	}

    FREE(mTemplateDatabase);
    mTemplateDatabase = 0;
    mCurrentTemplateDatabaseID = 0;
    mCurrentLanguageID = 0;
    mArcID = 0;

    detachXT9UserDictionary();
    destroyXT9UserDictionary();
    detachXT9Language();
    destroyXT9Language();

#ifdef ENABLED_LOGGING
    decumaStopLogging(mSession);
#endif

    int status = decumaEndSession(mSession);

#ifdef ENABLED_LOGGING
	delete gWriteLog;
	gWriteLog = 0;
#endif

    LOGV("Write_Alpha::finish()...status = %d", status);

    return status;
}

int Write_Alpha::setTemplateDatabase(int languageID)
{
    LOGV("Write_Alpha::setTemplateDatabase(%X)...", languageID);

    if (mCurrentTemplateDatabaseID != languageID) {
        mCurrentTemplateDatabaseID = languageID;

        const char* databaseFile = mDBRegistry->get_hwr_db_template_path(languageID);
        if (databaseFile == NULL) {
            databaseFile = mDBRegistry->get_hwr_db_template_path(DEFAULT_DATABASE_ID);
        }

        mSessionSettings->charSet.pLanguages[0] = xt9LanguageID2T9Write(languageID);
        LOGV("Write_Alpha::setTemplateDatabase(%X)...charSet = %d", languageID, (int)mSessionSettings->charSet.pLanguages[0]);

        void* data = loadDatabase(databaseFile);

        if (data == 0) {
            LOGE("Write_Alpha::setTemplateDatabase()...FAILED to open file");
            return decumaInvalidDatabase;
        }

        FREE(mTemplateDatabase);
        mTemplateDatabase = data;
        mSessionSettings->pStaticDB = (DECUMA_STATIC_DB_PTR)mTemplateDatabase;
    }

    LOGV("Write_Alpha::setTemplateDatabase(%X)...done", languageID);

    return 0;
}

int Write_Alpha::setLanguageDatabase(int languageID)
{
    LOGV("Write_Alpha::setLanguageDatabase(%X)...", languageID);

    int status = 0;

    if (mCurrentLanguageID != languageID) {
        mCurrentLanguageID = languageID;

        void* data = 0;
        const char* databaseFile = mDBRegistry->get_hwr_dic_path(mCurrentLanguageID);

        // use the existing pre-converted dictionary if specified
        if (databaseFile != NULL && (databaseFile = mDBRegistry->get_hwr_dic_path(mCurrentLanguageID)) != NULL) {
        	data = loadDatabase(databaseFile);
        }
        else {

        	// using XT9 LDB and convert to t9write dictionary at runtime
        	databaseFile = mDBRegistry->get_ldb_path(mCurrentLanguageID);

        	if (databaseFile != NULL || (databaseFile = mDBRegistry->get_ldb_path(DEFAULT_DATABASE_ID)) != NULL) {
        		data = convertXT9Language(databaseFile);
        	}
        }

    	if (data != 0) {

			detachXT9Language();
			destroyXT9Language();

    		mLanguageDatabase = data;

    		LOGD("decumaAttachStaticDictionary() mLanguageDatabase = %p", mLanguageDatabase);

            if ((status = decumaAttachStaticDictionary(mSession, mLanguageDatabase)) == 0) {
                mRecognitionSettings->boostLevel = boostDictWords;

        		LOGD("decumaChangeSessionSettings() mSession = %p", mSession);

                decumaChangeSessionSettings(mSession, mSessionSettings);
            }
            else {
                LOGE("Write_Alpha::setLanguageDatabase() - decumaAttachStaticDictionary() FAILED with status = %d", status);
            }
    	}
    	else {
            LOGE("setDatabase()...FAILED to open file");
            status = decumaInvalidDatabase;
        }
    }

    if (!mXT9AlphaData->setLanguage(languageID)) {
        LOGE("Write_Alpha::setLanguageDatabase() failed to set moca ldb");
        status = 1;
    }

    LOGV("Write_Alpha::setLanguageDatabase(%X)...status = %d", languageID, status);

    return status;
}

void* Write_Alpha::loadDatabase(const char* fileName)
{
    LOGV("Write_Alpha::loadDatabase()...");

    int size;
    void* data = load_bin_file(fileName, size);

    LOGV("Write_Alpha::loadDatabase()...file size = %.00fKB", (float)(size/1024));

    return data;
}

void Write_Alpha::destroyXT9Language()
{
	LOGV("Write_Alpha::destroyXT9Language()...");

    if (mLanguageDatabase != 0) {
        int status = decumaDestroyUnpackedDictionary(&mLanguageDatabase, &mMemFunctions);
        if (status != 0) {
        	FREE(mLanguageDatabase);
        }
        mLanguageDatabase = 0;
    }

    LOGV("Write_Alpha::destroyXT9Language()...done");

}

void Write_Alpha::detachXT9Language() {
	if (mLanguageDatabase != 0) {
        decumaDetachStaticDictionary(mSession, mLanguageDatabase);
	}
}

void Write_Alpha::detachXT9UserDictionary() {
	if (mUDB != 0) {
		decumaDetachStaticDictionary(mSession, mUDB);
	}
}

void Write_Alpha::destroyXT9UserDictionary() {
	LOGV("Write_Alpha::destroyXT9UserDictionary()...");

	if (mUDB != 0) {
		int status = decumaDestroyUnpackedDictionary(&mUDB, &mMemFunctions);
		if (status != 0) {
			FREE(mUDB);
		}
		mUDB = 0;
	}

	LOGV("Write_Alpha::destroyXT9UserDictionary()...done");
}

void Write_Alpha::convertXT9UserDictionary() {

	int status = decumaNullPointer;

	LOGV("Write_Alpha::convertXT9UserDictionary()...");

	if (mXT9AlphaData != 0) {

		const data::persistentDb* const xt9UDB = mXT9AlphaData->getPersistentUDB();
		if (xt9UDB != 0) {

			detachXT9UserDictionary();
			destroyXT9UserDictionary();

			DECUMA_UINT32 unpacked_size = 0;
			mUDB = 0;

			LOGV("Write_Alpha::convertXT9UserDictionary()... xt9 buffer = %p", xt9UDB->buffer());
			LOGV("Write_Alpha::convertXT9UserDictionary()... xt9 buffer size = %d", xt9UDB->bufferSize());

			status = decumaUnpackXT9Dictionary(&mUDB,
					xt9UDB->buffer(), xt9UDB->bufferSize(),
						decumaXT9UDB, &unpacked_size, &mMemFunctions);

			if (status == 0 && mUDB != 0) {
				status = decumaAttachStaticDictionary(mSession, mUDB);
				if (status != 0) {
					LOGE("Write_Alpha::convertXT9UserDictionary()...decumaAttachStaticDictionary() failed, status = %d", status);
				}
			}
			else {
				LOGE("Write_Alpha::convertXT9UserDictionary()...decumaUnpackXT9Dictionary() failed, status = %d", status);
			}
		}
	}

	LOGV("Write_Alpha::convertXT9UserDictionary()...status = %d", status);
}

void* Write_Alpha::convertXT9Language(const char* xt9LdbName)
{
	void* xt9ldb = 0;
	void* dictionary = 0;
	int file_size = 0;
	DECUMA_UINT32 unpacked_size = 0;

	if ((xt9ldb = load_bin_file(xt9LdbName, file_size)) != 0 && file_size > 0) {

		DECUMA_STATUS status;
		clock_t start, end;

		start = clock();
		status = decumaUnpackXT9Dictionary(&dictionary, xt9ldb, file_size, decumaXT9LDB, &unpacked_size, &mMemFunctions);
		end = clock();

		LOGD("Conversion Status for %s:", xt9LdbName);
		LOGD("Error status = %d", status);
		LOGD("LDB size = %dKB", file_size/1024);
		LOGD("Dictionary size = %dKB", unpacked_size/1024);
		LOGD("Took %f second to do the conversion", (double)(end-start)/CLOCKS_PER_SEC);

		FREE(xt9ldb);
	}

	return dictionary;

}

int Write_Alpha::beginArc()
{
    LOGV("Write_Alpha::beginArc(()...");

    int status = decumaBeginArcAddition(mSession);

    // don't care
    if (status == decumaArcAdditionSeqAlreadyStarted) {
    	status = 0;
    }

    LOGV("Write_Alpha::beginArc(()...status = %d", status);

    return status;
}

int Write_Alpha::addPoint(int arcId, int x, int y)
{
    //LOGV("Write_Alpha::addPoint(%d, %d)...", x, y);
    return decumaAddPoint(mSession, x, y, arcId);
}

int Write_Alpha::startNewArc(int& arcId) {
    int status = decumaStartNewArc(mSession, ++mArcID);
    arcId = mArcID;

    LOGV("Write_Alpha::startNewArc()...arcID = %d, status = %d", arcId, status);

    return status;
}

int Write_Alpha::CommitArc(int ardID) {
	LOGV("Write_Alpha::CommitArc(%d)...", ardID);

    int status = decumaCommitArc(mSession, ardID);

    LOGV("Write_Alpha::CommitArc()...status = %d", status);

    return status;
}

int Write_Alpha::endArc()
{
	LOGV("Write_Alpha::endArc(()...");

	mArcID = 0;

    int status = decumaEndArcAddition(mSession);

    // don't care
    if (status == decumaArcAdditionSeqNotStarted) {
    	status = 0;
    }

    LOGV("Write_Alpha::endArc(()...status = %d", status);

    return status;
}

int Write_Alpha::recognize(DECUMA_UNICODE* startWord, int& resultCount)
{
	LOGV("Write_Alpha::recognize()...");

    int status;
    mMixCandidateCount = 0;
    mRecognitionCandidateCount = 0;
    mRecognitionSettings->pStringStart = (DECUMA_UNICODE*)startWord;

    mRecognitionSettings->stringCompleteness = canBeContinued;
    status = decumaRecognize(mSession, mRecognitionCandidates, MAX_RECOGNIZE_CANDIDATES, (DECUMA_UINT16*)&mRecognitionCandidateCount, MAX_CHARACTERS, mRecognitionSettings, NULL);

    resultCount = mixRecognitionCandidates();

    LOGV("Write_Alpha::recognize()...%d", status);

    return status;
}

int Write_Alpha::getInstantGesture() {
	// 0 - means no the current arc is not an in instance gesture
	// 1 - mean the current may be an instance gesture - should follow by a call recognize() to
	//     the gesture symbol.

	int instantGesture = 0;
	decumaIndicateInstantGesture(mSession, &instantGesture, &mInstantGestureSettings);
	return instantGesture;
}

int Write_Alpha::mixRecognitionCandidates() {

	LOGV("Write_Alpha::mixRecognitionCandidates() current candidates = %d ...", mRecognitionCandidateCount);

	// sync up the length for each candidate and default mix to the same recognize candidate list
    for (mMixCandidateCount = 0; mMixCandidateCount < mRecognitionCandidateCount; mMixCandidateCount++) {
        // Note that mRecognitionCandidates[ith].pChars points mMixCandidates[ith].word location
        mMixCandidates[mMixCandidateCount]->len = mRecognitionCandidates[mMixCandidateCount].nChars;
        mMixCandidates[mMixCandidateCount]->bInstantGesture = mRecognitionCandidates[mMixCandidateCount].bInstantGesture;
        mMixCandidatePositions[mMixCandidateCount] = mMixCandidates[mMixCandidateCount];
    }

    LOGV("Write_Alpha::mixRecognitionCandidates() mRecognitionCandidates[0].bInstantGesture= %d ...", mRecognitionCandidates[0].bInstantGesture);
    LOGV("Write_Alpha::mixRecognitionCandidates() mRecognitionCandidates[0].nChars= %d ...", mRecognitionCandidates[0].nChars);

    if (mMixCandidateCount > 1) {

        int defaultPosition = 0;
        DECUMA_HWR_RESULT* defaultRecognizeCandidate = &mRecognitionCandidates[defaultPosition];
        int defaultRecognizeCandidateLen = defaultRecognizeCandidate->nChars;

        if (mRecognitionCandidates[0].bInstantGesture) {
        	// Exclude the last gesture char when asking xt9 to do word complete/sp word list.
        	// Or we may choose not to do post xt9 word list build if instant gesture is detected.
        	defaultRecognizeCandidateLen--;
        }

        if (defaultRecognizeCandidateLen > 1) {
        	LOGV("Write_Alpha::mixRecognitionCandidates() word type = %d", defaultRecognizeCandidate->stringType);

#if 0
			char debugWord[MAX_CHARACTERS] = {0};
			defaultRecognizeCandidate->pChars[defaultRecognizeCandidate->nChars] = 0;
			int byteWrote = ucs2ToUtf8(debugWord, MAX_CHARACTERS, defaultRecognizeCandidate->pChars);
			LOGV("Write_Alpha::mixRecognitionCandidates() hwr default word", debugWord);
#endif

            bool buildWordListWithWordCompletion = false;
            bool buildWordListWithSpellCorrection = false;

            if (defaultRecognizeCandidate->stringType == notFromDictionary) {

            	LOGV("Write_Alpha::mixRecognitionCandidates() check for spell correction");

                mXT9AlphaData->clearAllKeys();
                int prob = 0xff;
                buildWordListWithSpellCorrection = true;
                for (int i = 0; i < defaultRecognizeCandidateLen; i++) {
                    if (mXT9AlphaData->addCustomSymbolSet((unsigned short*)&defaultRecognizeCandidate->pChars[i], &prob, 1,
                            mXT9AlphaData->isUpperSymbol(defaultRecognizeCandidate->pChars[i]) ? ET9SHIFT : ET9NOSHIFT, ET9_NO_ACTIVE_INDEX) != 0) {
                    	buildWordListWithSpellCorrection = false;
                        break;
                    }
                }
            }
            else {

				LOGV("Write_Alpha::mixRecognitionCandidates() check for word completion");

				mXT9AlphaData->clearAllKeys();
				buildWordListWithWordCompletion = true;
				for (int i = 0; i < defaultRecognizeCandidateLen; i++) {
					if (mXT9AlphaData->addExplicit((unsigned short*)&defaultRecognizeCandidate->pChars[i], 1,
							mXT9AlphaData->isUpperSymbol(defaultRecognizeCandidate->pChars[i]) ? ET9SHIFT : ET9NOSHIFT) != 0) {
						buildWordListWithWordCompletion = false;
						break;
					}
				}
            }

            // Build and copy the spell correction or completion words to the end of the of recognition list.
            if (buildWordListWithWordCompletion || buildWordListWithSpellCorrection) {

            	ET9ASLCORRECTIONMODE savedMode = mXT9AlphaData->getSelectionListMode();
            	bool savedDBWordStems = mXT9AlphaData->getDBStems();
            	int  savedDBWordStemsPoint = mXT9AlphaData->getWordStemsPoint();
            	int  savedWordCompletionPoint = mXT9AlphaData->getWordCompletionPoint();

            	mXT9AlphaData->setSpellCorrectionMode(ET9ASPCMODE_EXACT);
            	mXT9AlphaData->setRegionalCorrection(true);
            	mXT9AlphaData->setSelectionListMode(ET9ASLCORRECTIONMODE_LOW);
            	mXT9AlphaData->setDBStems(true);
            	mXT9AlphaData->setWordStemsPoint(defaultRecognizeCandidateLen < 4 ? defaultRecognizeCandidateLen : 4);
            	mXT9AlphaData->setWordCompletion(true);
            	mXT9AlphaData->setWordCompletionPoint(defaultRecognizeCandidateLen < 4 ? defaultRecognizeCandidateLen : 4);

            	int defaultWordIndex = 0;
                int wordListCount = 0;

            	wordListCount = mXT9AlphaData->buildWordList(defaultWordIndex);

            	if (wordListCount > 0) {

					LOGV("Write_Alpha::mixRecognitionCandidates() build word list, count = %d", wordListCount);

					int wordIndex = 0;
					while (wordIndex < wordListCount && mMixCandidateCount < MAX_MIX_CANDIDATES) {
						ET9AWWordInfo* pWord = mXT9AlphaData->getWord(wordIndex);
						if (pWord == 0) {
							break;
						}

						// avoid duplicated
						if (pWord->wWordLen != defaultRecognizeCandidateLen ||
								ucs2_strncmp((const ucs2_Char*)defaultRecognizeCandidate->pChars,
										(const ucs2_Char*)pWord->sWord, defaultRecognizeCandidateLen) != 0) {

							LOGV("Write_Alpha::mixRecognitionCandidates() added sp/wc (%d) word to location %d", wordIndex, mMixCandidateCount);
#if 0
							char debugWord[MAX_CHARACTERS] = {0};
							pWord->sWord[pWord->wWordLen] = 0;
							int byteWrote = ucs2ToUtf8(debugWord, MAX_CHARACTERS, pWord->sWord);
							LOGV("Write_Alpha::mixRecognitionCandidates() added sp/wc (%s) word to location %d", debugWord, mMixCandidateCount);
#endif

							Candidate* candidate = mMixCandidates[mMixCandidateCount];
							candidate->len = wordCopy(candidate->word, pWord->sWord, pWord->wWordLen, MAX_CHARACTERS);
							candidate->bInstantGesture = 0;
							mMixCandidatePositions[mMixCandidateCount] = candidate;
							mMixCandidateCount++;
						}
						else {
							LOGV("Write_Alpha::mixRecognitionCandidates() sp/wc (%d) word is the same - ignored", wordIndex);
						}

						wordIndex++;
					}
				}

            	// restore to the original mode and settings, must do after building xt9 word list
            	mXT9AlphaData->setSelectionListMode(savedMode);
            	mXT9AlphaData->setDBStems(savedDBWordStems);
            	mXT9AlphaData->setWordStemsPoint(savedDBWordStemsPoint);
            	mXT9AlphaData->setWordCompletion(savedWordCompletionPoint != 0);
            	mXT9AlphaData->setWordCompletionPoint(savedWordCompletionPoint);
            }
        }

    }

    LOGV("Write_Alpha::mixRecognitionCandidates() new candidates = %d ...done", mMixCandidateCount);

    return mMixCandidateCount;
}

int Write_Alpha::getCandidate(int index, DECUMA_UNICODE* word, int maxLength, int& length, int & bEndsWithInstGest)
{
	LOGV("Write_Alpha::getCandidate(%d)...", index);

	if (!(0 <= index && index < mMixCandidateCount)) {
	    LOGE("Write_Alpha::getCandidate(%d)...error", index);
		return decumaInvalidIndex;
	}

	length = wordCopy(word, mMixCandidatePositions[index]->word, mMixCandidatePositions[index]->len, maxLength);
	bEndsWithInstGest = mMixCandidatePositions[index]->bInstantGesture;

	LOGV("Write_Alpha::getCandidate(%d)...word length = %d", index, length);

	return 0;
}


int Write_Alpha::noteSelectedCandidate(int index)
{
	LOGV("Write_Alpha::noteSelectedCandidate(%d)...", index);

#ifdef ENABLED_LOGGING
	if (index >=0 && index < mRecognitionCandidateCount) {
		LOGV("Write_Alpha::noteSelectedCandidate(%d)...word = %s", index, mMixCandidatePositions[index]->word);
		decumaLogAcceptedResult(mSession, mMixCandidatePositions[index]->word, mMixCandidatePositions[index]->len);
	}
#endif

	int true_index;
	if (index < mRecognitionCandidateCount) {
		true_index = index;
	}
	else {
		// word index that is larger then mRecogntionCandidateCount is the result of sp/wc
		// of the default hwr candidate.
		true_index = 0;
	}

	int status = decumaNoteSelectedCandidate(mSession, true_index);

	LOGV("Write_Alpha::noteSelectedCandidate(%d)...status = %d", index, status);

	return status;
}

int Write_Alpha::addWordToUserDictionary(const unsigned short* word, int length)
{
	if (mXT9AlphaData == 0) {
		return decumaNullSessionPointer;
	}

	mXT9AlphaData->addWordToUserDictionary(word, length);
	mXT9AlphaData->breakContext();

	return 0;
}

const char* Write_Alpha::getVersion() const
{
	return (const char*)decumaGetEngineVersion();
}

const char* Write_Alpha::getDatabaseVersion() const
{
	static char databaseVersion[DECUMA_DATABASE_VERSION_STRING_LENGTH + 1];
	decumaDatabaseGetVersion(mSessionSettings->pStaticDB, databaseVersion, DECUMA_DATABASE_VERSION_STRING_LENGTH);
	return (const char*)databaseVersion;
}

} // namespace mocainput

