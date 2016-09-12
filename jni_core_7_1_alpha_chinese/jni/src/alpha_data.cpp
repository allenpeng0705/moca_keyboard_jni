
#include <Log.h>

#include <assert.h>
#include <string.h>

#include "alpha_data.h"
#include "load_file.h"
#include "ucs2.h"

#include "user_study_log.h"
#include "mem_alloc.h"

namespace mocainput {

// static
int alpha_data::instanceCount = 0;
// static
alpha_data* alpha_data::singletonAlphaDataInstance = 0;
// static
alpha_data* alpha_data::getInstance(DBRegistry* dbRegistry)
{
	if (alpha_data::singletonAlphaDataInstance == 0) {
		alpha_data::singletonAlphaDataInstance = new alpha_data(dbRegistry);
		alpha_data::singletonAlphaDataInstance->create();
	}

	++alpha_data::instanceCount;

	LOGV("alpha_data::getInstance() refCount = %d", instanceCount);

	return alpha_data::singletonAlphaDataInstance;
}

void alpha_data::deleteInstance()
{
	if (alpha_data::instanceCount > 0) {
		--alpha_data::instanceCount;
	}

	LOGV("alpha_data::deleteInstance() refCount = %d", instanceCount);

	if (alpha_data::singletonAlphaDataInstance != 0 && alpha_data::instanceCount == 0) {
		alpha_data::singletonAlphaDataInstance->destroy();
		delete alpha_data::singletonAlphaDataInstance;
		alpha_data::singletonAlphaDataInstance = 0;
	}
}

ET9STATUS alpha_data::LdbReadCallback(
					ET9AWLingInfo       *pLingInfo,
					ET9U8 * /*ET9FARDATA*/ 	*ppbSrc,
					ET9U32				*pdwSizeInBytes)
{
	alpha_data* _alpha_data = 0;

	_alpha_data = reinterpret_cast<alpha_data*>(pLingInfo->pPublicExtension);

     if (_alpha_data && _alpha_data->readLdb(
             (int)pLingInfo->pLingCmnInfo->wLdbNum,
             (char**)ppbSrc,
             (int*)pdwSizeInBytes))
     {
         return ET9STATUS_NONE;
     }

	return ET9STATUS_READ_DB_FAIL;
}

alpha_data::alpha_data(DBRegistry* dbRegistry) : data(dbRegistry)
{
	mLingInfo = 0;
	mLingCmnInfo = 0;
	mPrivateWordInfo = 0;
	mUdb = 0;
	mCdb = 0;
	mAsdb = 0;
	mMdb = 0;
}

alpha_data::~alpha_data()
{
	LOGV("alpha_data::~alpha_data()");
}

bool alpha_data::create()
{
	LOGV("alpha_data::create()...");

	if (mWordSymbInfo == 0 || mKdbInfo == 0) {

		LOGE("alpha_data::create()...FAILED because mWordSymbInfo or mKdbInfo is NULL");

		return false;
	}

	if (mPrivateWordInfo == 0) {
		if ((mPrivateWordInfo = (ET9AWPrivWordInfo*)CALLOC(ET9MAXSELLISTSIZE * sizeof(ET9AWPrivWordInfo), 1)) == 0) {
			LOGE("alpha_data::create()...failed to create mPrivateWordInfo");
			return false;
		}
	}

	if (mLingCmnInfo == 0) {
		if ((mLingCmnInfo = (ET9AWLingCmnInfo*)CALLOC(sizeof(ET9AWLingCmnInfo), 1)) == 0) {
			FREE(mPrivateWordInfo);
			LOGE("alpha_data::create()...failed to create mLingCmnInfo");
			return false;
		}
	}

	if (mLingInfo == 0) {
		if ((mLingInfo = (ET9AWLingInfo*)CALLOC(sizeof(ET9AWLingInfo), 1)) == 0) {
			FREE(mPrivateWordInfo);
			FREE(mLingCmnInfo);
			LOGE("alpha_data::create()...failed to create mLingInfo");
			return false;
		}
	}

	ET9STATUS status = ET9AWSysInit(mLingInfo, mLingCmnInfo, mWordSymbInfo, 1,
			ET9MAXSELLISTSIZE, mPrivateWordInfo, this);

	if (ET9STATUS_NONE == status) {
		status = ET9AWLdbInit(mLingInfo, &LdbReadCallback);
	}

	if (ET9STATUS_NONE == status) {
	    initDbs();
	}

	ET9AWSysSetExpandAutoSubstitutions(mLingInfo);

#if USER_STUDY_LOG
    if (m_pUserStudyLog) {
        m_pUserStudyLog->open();
        mDelKeyCount = 0;
    }
#endif
	LOGV("alpha_data::create()...done with status(%X)", status);

	LOGV("alpha_data::create()...done WSI = %d", sizeof(ET9WordSymbInfo));

	return status == ET9STATUS_NONE;
}

void alpha_data::destroy()
{
    LOGV("alpha_data::destroy()...");

	DELETE(mUdb);
    DELETE(mAsdb);
    DELETE(mCdb);
    DELETE(mMdb);

    FREE(mLingInfo);
    FREE(mPrivateWordInfo);
    FREE(mLingCmnInfo);

#if USER_STUDY_LOG
    if (m_pUserStudyLog) {
        m_pUserStudyLog->close();
    }
#endif
    LOGV("alpha_data::destroy()...done");
}

void alpha_data::initUdb()
{
    if (mUdb == 0) {
        mUdb  = new data::persistentDb(getDbRegistry()->get_udb_path(0x00FF));
    }

    if (mUdb) {
        mUdb->create(ET9MINRUDBDATABYTES);
        if (mUdb->buffer()) {
            ET9STATUS status = ET9AWRUDBInit(mLingInfo, (ET9AWRUDBInfo*)mUdb->buffer(), (ET9U16)mUdb->bufferSize(), 0);
            LOGV("alpha_data::initUdb(%p, %d) - status = 0x%X", mUdb->buffer(), mUdb->bufferSize(), status);
        }
    }
}

void alpha_data::initAsdb()
{
    if (mAsdb == 0) {
        mAsdb = new data::persistentDb(getDbRegistry()->get_asdb_path(0x00FF));
    }

    if (mAsdb) {
        mAsdb->create(ET9MINASDBDATABYTES);
        if (mAsdb->buffer()) {
            ET9STATUS status = ET9AWASDBInit(mLingInfo, (ET9AWASDBInfo*)mAsdb->buffer(), (ET9U16)mAsdb->bufferSize(), 0);
            LOGV("alpha_data::initAsdb(%p, %d) - status = 0x%X", mAsdb->buffer(), mAsdb->bufferSize(), status);
        }
    }
}

void alpha_data::initCdb()
{
    if (mCdb == 0) {
        mCdb  = new data::persistentDb(getDbRegistry()->get_cdb_path(0x00FF));
    }

    if (mCdb) {
        mCdb->create(ET9MINCDBDATABYTES);
        if (mCdb->buffer()) {
            ET9STATUS status = ET9AWCDBInit(mLingInfo, (ET9AWCDBInfo*)mCdb->buffer(), (ET9U16)mCdb->bufferSize(), 0);
            LOGV("alpha_data::initCdb(%p, %d) - status = 0x%X", mCdb->buffer(), mCdb->bufferSize(), status);
        }
    }
}

void alpha_data::initMdb()
{
    if (mMdb == 0) {
        mMdb  = new alpha_data::mdb(this);
    }
}

void alpha_data::initDbs()
{
	LOGV("alpha_data::initDbs()...");

    initUdb();
    initAsdb();
    initCdb();
    initMdb();

    LOGV("alpha_data::initDbs()...done");
}

bool alpha_data::start()
{
	LOGV("alpha_data::start()...");

	data::clearAllKeys();

	LOGV("alpha_data::start()...done");

	return true;
}

void alpha_data::finish()
{
	LOGV("alpha_data::finish()...");

    data::clearAllKeys();
    flushDbs();

    LOGV("alpha_data::finish()...done");
}

void alpha_data::flushDbs()
{
	LOGV("alpha_data::flushDbs()...");

	flushUdb();
    flushAsdb();
    flushCdb();

    LOGV("alpha_data::flushDbs()...done");
}

void alpha_data::flushAsdb()
{
	LOGV("alpha_data::flushAsdb()...");

	if (mAsdb != 0) {
        mAsdb->flush();
    }

	LOGV("alpha_data::flushAsdb()...done");

}

void alpha_data::flushUdb()
{
	LOGV("alpha_data::flushUdb()...");

    if (mUdb != 0) {
        mUdb->flush();
    }

    LOGV("alpha_data::flushUdb()...done");
}

void alpha_data::flushCdb()
{
	LOGV("alpha_data::flushCdb()...");

    if (mCdb != 0) {
        mCdb->flush();
    }

    LOGV("alpha_data::flushCdb()...done");
}

void alpha_data::getSecondaryLDBID(ET9U16 *pwLdbId)
{
	ET9U8 bSecondaryId = 0;
	ET9U32 dwSize = 0;
	ET9U16 wSavedLdbId = mLingInfo->pLingCmnInfo->wLdbNum;

	mLingInfo->pLingCmnInfo->wLdbNum = *pwLdbId;

	ET9U8 *pbData = &bSecondaryId;
	readLdb((int)mLingInfo->pLingCmnInfo->wLdbNum,
            (char**)&pbData,
            (int*)&dwSize);
	*pwLdbId &= ET9PLIDMASK;
	*pwLdbId |= (bSecondaryId << 8);

	mLingInfo->pLingCmnInfo->wLdbNum = wSavedLdbId;

}

int alpha_data::composeKDBId(unsigned short keyboardLayoutId)
{
    unsigned short kdbId;

    kdbId = keyboardLayoutId | (ET9PLIDMASK & mCurrentLdbId);

    // Check if there is a keyboard database file for this keyboard layout + language,
    // in our database config.
    if (!getDbRegistry()->isThereKdbPathFor(kdbId)) {
        // try the generic keyboard id
        kdbId = (kdbId & KEYBOARD_LAYOUT_MASK) | ET9PLIDNull;
        if (!getDbRegistry()->isThereKdbPathFor(kdbId)) {
            LOGE("data::composeKDBId()...failed - can not find kdb file for keyboard id(0x%X)", kdbId);
            return 0;
        }
    }

    return kdbId;
}

void alpha_data::getCoreSettings(void)
{
    LOGV("ET9WORDSTEMS_MODE=%s", ET9WORDSTEMS_MODE(mLingCmnInfo) ? "ON" : "OFF");
    LOGV("ET9WORDCOMPLETION_MODE=%s", ET9WORDCOMPLETION_MODE(mLingCmnInfo) ? "ON" : "OFF");
    LOGV("ET9NEXTWORDPREDICTION_MODE=%s", ET9NEXTWORDPREDICTION_MODE(mLingCmnInfo) ? "ON" : "OFF");
    LOGV("ET9EXACTINLIST=%s", ET9EXACTINLIST(mLingCmnInfo) ? "ON" : "OFF");
    LOGV("ET9EXACTLAST=%s", ET9EXACTLAST(mLingCmnInfo) ? "ON" : "OFF");
    LOGV("ET9USERDEFINEDAUTOSUBENABLED=%s", ET9USERDEFINEDAUTOSUBENABLED(mLingCmnInfo) ? "ON" : "OFF");
    LOGV("ET9LDBSUPPORTEDAUTOSUBENABLED=%s", ET9LDBSUPPORTEDAUTOSUBENABLED(mLingCmnInfo) ? "ON" : "OFF");
    LOGV("ET9AUTOAPPENDINLIST=%s", ET9AUTOAPPENDINLIST(mLingCmnInfo) ? "ON" : "OFF");
    LOGV("ET9QUDBSUPPORTENABLED=%s", ET9QUDBSUPPORTENABLED(mLingCmnInfo) ? "ON" : "OFF");
    LOGV("ET9LDBENABLED=%s", ET9LDBENABLED(mLingCmnInfo) ? "ON" : "OFF");
    LOGV("ET9CDBENABLED=%s", ET9CDBENABLED(mLingCmnInfo) ? "ON" : "OFF");
    LOGV("ET9RUDBENABLED=%s", ET9RUDBENABLED(mLingCmnInfo) ? "ON" : "OFF");
    LOGV("ET9ASDBENABLED=%s", ET9ASDBENABLED(mLingCmnInfo) ? "ON" : "OFF");
    LOGV("ET9MDBENABLED=%s", ET9MDBENABLED(mLingCmnInfo) ? "ON" : "OFF");
    LOGV("ET9DOWNSHIFTDEFAULTENABLED=%s", ET9DOWNSHIFTDEFAULTENABLED(mLingCmnInfo) ? "ON" : "OFF");
    LOGV("ET9DOWNSHIFTALLLDB=%s", ET9DOWNSHIFTALLLDB(mLingCmnInfo) ? "ON" : "OFF");
    LOGV("ET9EXACTISDEFAULT=%s", ET9EXACTISDEFAULT(mLingCmnInfo) ? "ON" : "OFF");
    LOGV("ET9ACTIVELANGSWITCHENABLED=%s", ET9ACTIVELANGSWITCHENABLED(mLingCmnInfo) ? "ON" : "OFF");
    LOGV("ET9INACTIVELANGSPELLCORRECTENABLED=%s", ET9INACTIVELANGSPELLCORRECTENABLED(mLingCmnInfo) ? "ON" : "OFF");
    LOGV("ET9POSTSORTENABLED=%s", ET9POSTSORTENABLED(mLingCmnInfo) ? "ON" : "OFF");
    LOGV("ET9LMENABLED=%s", ET9LMENABLED(mLingCmnInfo) ? "ON" : "OFF");
    LOGV("ET9BOOSTTOPCANDIDATE=%s", ET9BOOSTTOPCANDIDATE(mLingCmnInfo) ? "ON" : "OFF");
    LOGV("ET9EXPANDAUTOSUB=%s", ET9EXPANDAUTOSUB(mLingCmnInfo) ? "ON" : "OFF");
    LOGV("ET9AUTOSPACE=%s", ET9AUTOSPACE(mLingCmnInfo) ? "ON" : "OFF");

    LOGV("DefaultExpTermPuncts=%s", mLingInfo->pLingCmnInfo->Private.bTotalExpTermPuncts == 0 ? "YES" : "NO");
    LOGV("DefaultEmbeddedPunct=%s", mLingInfo->pLingCmnInfo->Private.sExpEmbeddedPunct == 0 ? "YES" : "NO");

    //::ET9AWLdbGetLanguage(mLingInfo, &(pSettings->wFirstLdbNum), &(pSettings->wSecondLdbNum)); /* Don't need to know lang for now */

    switch (ET9AWSys_GetSpellCorrectionMode(mLingInfo)) {
    case ET9ASPCMODE_OFF:
        LOGV("SpellCorrectionMode=ET9ASPCMODE_OFF");
        break;
    case ET9ASPCMODE_EXACT:
        LOGV("SpellCorrectionMode=ET9ASPCMODE_EXACT");
        break;
    case  ET9ASPCMODE_REGIONAL:
        LOGV("SpellCorrectionMode=ET9ASPCMODE_REGIONAL");
        break;
    }

    switch (ET9AWSys_GetSpellCorrectionSearchFilter(mLingInfo)) {
    case ET9ASPCSEARCHFILTER_UNFILTERED:
        LOGV("SpellCorrectionSearchFilter=ET9ASPCSEARCHFILTER_UNFILTERED");
        break;
    case ET9ASPCSEARCHFILTER_ONE_EXACT:
        LOGV("SpellCorrectionSearchFilter=ET9ASPCSEARCHFILTER_ONE_EXACT");
        break;
    case ET9ASPCSEARCHFILTER_ONE_REGIONAL:
        LOGV("SpellCorrectionSearchFilter=ET9ASPCSEARCHFILTER_ONE_REGIONAL");
        break;
    case ET9ASPCSEARCHFILTER_TWO_EXACT:
        LOGV("SpellCorrectionSearchFilter=ET9ASPCSEARCHFILTER_TWO_EXACT");
        break;
    case ET9ASPCSEARCHFILTER_TWO_REGIONAL:
        LOGV("SpellCorrectionSearchFilter=ET9ASPCSEARCHFILTER_TWO_REGIONAL");
        break;
    }

    LOGV("SpellCorrectionCount=%d", ET9AWSys_GetSpellCorrectionCount(mLingInfo));

    switch(ET9AWSys_GetSelectionListMode(mLingInfo)) {

    case ET9ASLCORRECTIONMODE_LOW:
        LOGV("SelectionListMode=ET9ASLCORRECTIONMODE_LOW");
        break;
    case ET9ASLCORRECTIONMODE_HIGH:
        LOGV("SelectionListMode=ET9ASLCORRECTIONMODE_HIGH");
        break;
    }

    LOGV("WordStemsPoint=%d", ET9AWSys_GetWordStemsPoint(mLingInfo));
    LOGV("WordCompletionPoint=%d", ET9AWSys_GetWordCompletionPoint(mLingInfo));
    LOGV("CompletionCount=%d", ET9AWSys_GetCompletionCount(mLingInfo));
    LOGV("PrimaryFence=%d", ET9AWSys_GetPrimaryFence(mLingInfo));
    LOGV("SecondaryFence=%d", ET9AWSys_GetSecondaryFence(mLingInfo));
    LOGV("BilingualSupported=%d", ET9AWSys_GetBilingualSupported(mLingInfo));

    ET9AEXACTINLIST eExactInList;
    ::ET9AWGetExactInList(mLingInfo, &eExactInList);
    switch(eExactInList) {
    case ET9AEXACTINLIST_OFF:
        LOGV("ET9AEXACTINLIST=ET9AEXACTINLIST_OFF");
        break;
    case ET9AEXACTINLIST_FIRST:
        LOGV("ET9AEXACTINLIST=ET9AEXACTINLIST_FIRST");
        break;
    case ET9AEXACTINLIST_LAST:
        LOGV("ET9AEXACTINLIST=ET9AEXACTINLIST_LAST");
        break;
    case ET9AEXACTINLIST_DEFAULT:
        LOGV("ET9AEXACTINLIST=ET9AEXACTINLIST_DEFAULT");
        break;
    }

    return;
}

int alpha_data::initTrace(int kdbId, int width, int height)
{
	int iResult = 0;

	iResult = data::initTrace(kdbId, width, height);

	#if USER_STUDY_LOG
		if ((m_pUserStudyLog) && (iResult == ET9STATUS_NONE))
		{
			char strTraceInfo[100] = {0, };
			int iStrPos = 0;

			iStrPos += sprintf(strTraceInfo + iStrPos, "KDB: %x, %ix%i", kdbId, width, height);

			m_pUserStudyLog->log("\n");
			m_pUserStudyLog->log(strTraceInfo);
		}
	#endif

	return iResult;
}

int alpha_data::isAutoSpaceBeforeTrace(int kdbId, ET9TracePoint* points, int numPoints)
{
	return data::isAutoSpaceBeforeTrace(kdbId, points, numPoints);
}

int alpha_data::processTap(int kdbId, int TapX, int TapY, int shiftState)
{
	return data::processTap(kdbId, TapX, TapY, shiftState);
}

int alpha_data::processTrace(int kdbId, ET9TracePoint* points, int numPoints, int shiftState)
{
	int iReturn = data::processTrace(kdbId, points, numPoints, shiftState);

	#if USER_STUDY_LOG
		if (m_pUserStudyLog && iReturn)
		{
			char* strPoints = (char*)MALLOC((numPoints * 15) + 1);
			int iStrPos = 0;

			if(strPoints)
			{
				for(int i = 0; i < numPoints; i++)
				{
					iStrPos += sprintf(strPoints + iStrPos, "(%i, %i), ", points[i].nX, points[i].nY);
				}

				m_pUserStudyLog->log(strPoints);
				FREE(strPoints);
			}
		}
	#endif

	/* Output the settings of core for debug info */
	getCoreSettings();

	return iReturn;
}

int alpha_data::processKey(int kdbId, int key, int shiftState)
{
#if USER_STUDY_LOG
	if (m_pUserStudyLog)
	{
		char strKey[20] = {0, };

		sprintf(strKey, "ProcessKey: %c", key);

		m_pUserStudyLog->log(strKey);
	}
#endif

	int iReturn = data::processKey(kdbId, key, shiftState);

    /* Output the settings of core for debug info */
    getCoreSettings();

    return iReturn;
}

int alpha_data::addExplicit(const unsigned short* word, int len, int shiftState)
{
    int iReturn = data::addExplicitString(word, len, shiftState);

    /* Output the settings of core for debug info */
    getCoreSettings();

    return iReturn;
}

int alpha_data::getKeyCount()
{
    return data::getKeyCount();
}

int alpha_data::clearKey()
{
#if USER_STUDY_LOG
    if (getKeyCount() > 0) {
        mDelKeyCount++;
    }

    if (m_pUserStudyLog)
    {
    	m_pUserStudyLog->log("Clear Key Pressed");
    }
#endif
	return data::clearKey();
}

int alpha_data::clearAllKeys()
{
	return data::clearAllKeys();
}

int alpha_data::buildWordList(int& defaultWordIndex)
{
	ET9U8 bDefaultWordIndex = 0;
	ET9U8 bTotalWords = 0;
	ET9STATUS wStatus;

	if (mLingInfo == 0) {
		return 0;
	}

	wStatus = ET9AWSelLstBuild(mLingInfo, &bTotalWords, &bDefaultWordIndex);

	// clear the last symbol if the selection list can't be built because of the last symbol entered
	if((bTotalWords == 0) && (mWordSymbInfo->bNumSymbs > 0))
	{
		clearKey();

		// don't rebuild if there is no active input
		if(mWordSymbInfo->bNumSymbs > 0)
		{
			wStatus = ET9AWSelLstBuild(mLingInfo, &bTotalWords, &bDefaultWordIndex);
		}
	}

	defaultWordIndex = bDefaultWordIndex;
#if USER_STUDY_LOG
    mDefaultWordIndex = defaultWordIndex;
#endif
    return bTotalWords;
}

ET9AWWordInfo* alpha_data::getWord(int wordIndex)
{
	ET9AWWordInfo * pWordInfo = 0;
	ET9STATUS wStatus;
	ET9U8 bIndex = static_cast<ET9U8>(wordIndex);

	if (mLingInfo == 0) {
		return pWordInfo;
	}

	wStatus = ET9AWSelLstGetWord(mLingInfo, &pWordInfo, bIndex);
	if (wStatus != ET9STATUS_NONE) {
		pWordInfo = 0;
	}

	return pWordInfo;
}

bool alpha_data::getWord(int wordIndex, unsigned short* word, unsigned short* sub, int& wordLen, int& wordCompLen, int& subLen, int maxLen)
{
	ET9AWWordInfo * pWordInfo = 0;
	ET9STATUS wStatus;
	ET9U8 bIndex = static_cast<ET9U8>(wordIndex);

	if (mLingInfo == 0) {
		return false;
	}

	wStatus = ET9AWSelLstGetWord(mLingInfo, &pWordInfo, bIndex);

	if (wStatus || pWordInfo == 0) {
		LOGE("alpha_data::getWord(%d)...failed with status = 0x%X, wordInfo = %p", bIndex, wStatus, pWordInfo);
		return false;
	}

	wordLen = pWordInfo->wWordLen;
	wordCompLen = pWordInfo->wWordCompLen;
	subLen = pWordInfo->wSubstitutionLen;

	if (wordLen > maxLen || subLen > maxLen) {
		return false;
	}

	data::wordCopy(word, (const unsigned short*)pWordInfo->sWord, wordLen);
    data::wordCopy(sub, (const unsigned short*)pWordInfo->sSubstitution, subLen);

	return true;
}

void alpha_data::lockWord(int wordIndex)
{
	if (mLingInfo == 0) {
		return;
	}

	ET9AWLockWord(mLingInfo, (ET9U8)wordIndex);
}

bool alpha_data::setLanguage(int langId)
{
	ET9STATUS wStatus;
	ET9U16 ldbId = (ET9U16) langId;

	if (mLingInfo == 0) {
		return false;
	}

	if ((ET9PLIDMASK & mCurrentLdbId) != (ldbId & ET9PLIDMASK)) {

		if ((langId & ET9PLIDMASK) != ET9PLIDNone) {

			getSecondaryLDBID(&ldbId);

			LOGV("alpha_data::setLanguage(0x%X):validate...", ldbId);

			wStatus = ET9AWLdbValidate(mLingInfo, ldbId, &LdbReadCallback);

			if (wStatus) {
				LOGE("alpha_data::setLanguage(0x%X):validate...failed with status(0x%X)", ldbId, wStatus);
				return false;
			}
			LOGV("alpha_data::setLanguage(0x%X):validate...ok", ldbId);


			LOGV("alpha_data::setLanguage(0x%X):set...", ldbId);

			wStatus = ET9AWLdbSetLanguage(mLingInfo, ldbId, 0);

			if (wStatus) {
				LOGE("alpha_data::setLanguage(0x%X):set...failed with status(0x%X)", ldbId, wStatus);
				return false;
			}

			mCurrentLdbId = ldbId;
		}

		mMdb->loadFile(getDbRegistry()->get_mdb_path(langId));

	}

	LOGV("alpha_data::setLanguage(0x%X):set...ok", ldbId);
	return true;
}

void alpha_data::wordSelected(int wordIndex)
{
    LOGV("alpha_data::wordSelected(0x%X)...", wordIndex);

    if (mLingInfo == 0) {
		return;
	}

	ET9STATUS status = ET9AWSelLstSelWord(mLingInfo, wordIndex);

#if USER_STUDY_LOG
    if (m_pUserStudyLog && status == ET9STATUS_NONE) {
        WordLogInfo wordlog;
        ET9AWWordInfo * pWordInfo = NULL;
        ET9STATUS status;

        memset(&wordlog, 0, sizeof(wordlog));
        //wordlog.m_eMode = ET9AWSys_GetSelectionListMode(mLingInfo);
        wordlog.m_nWCPoint = getWordCompletionPoint();
        wordlog.m_bBoosted = (mLingInfo->pLingCmnInfo->Private.bApplyBoosting != 0);
        wordlog.m_nKeyCount = getKeyCount();
        wordlog.m_nDelKeyCount = mDelKeyCount;

        // Selected word
        status = ET9AWSelLstGetWord(mLingInfo, &pWordInfo, (ET9U8)wordIndex);
        if (status || pWordInfo == NULL || !pWordInfo->bIsSpellCorr) {
            wordlog.m_bIsSpellCorr = false;
        }
        else {
            wordlog.m_bIsSpellCorr = true;
        }
        data::wordCopy(wordlog.m_awSelectedWord, (const unsigned short*)pWordInfo->sWord, pWordInfo->wWordLen);
        wordlog.m_awSelectedWord[pWordInfo->wWordLen] = 0;
        wordlog.m_nSelectedWordIndex = (int)wordIndex;

        // Default word
        status = ET9AWSelLstGetWord(mLingInfo, &pWordInfo, (ET9U8)mDefaultWordIndex);
        data::wordCopy(wordlog.m_awDefaultWord, (const unsigned short*)pWordInfo->sWord, pWordInfo->wWordLen);
        wordlog.m_awDefaultWord[pWordInfo->wWordLen] = 0;
        wordlog.m_nDefaultWordIndex = (int)mDefaultWordIndex;

        // Exact Word
        int wordLen = getExactType(wordlog.m_awExactWord, 64);
        wordlog.m_awExactWord[wordLen] = 0;

        m_pUserStudyLog->log(wordlog);

        mDelKeyCount = 0;
    }
#endif

	LOGV("alpha_data::wordSelected(0x%X)...status = 0x%X", wordIndex, status);
}

bool alpha_data::addWordToUserDictionary(const unsigned short* word, int wordLen)
{
	if (mLingInfo == 0) {
		return false;
	}

	ET9STATUS wStatus = ET9AWNotePhraseDone(mLingInfo, (ET9SYMB *)word, (ET9U16)wordLen);
	if (wStatus) {
		return false;
	}
	return true;
}

void alpha_data::doPostShift(int& wordIndex)
{
	int totalWords = 0;

	if (mLingInfo == 0) {
		return;
	}

	ET9AWSelLstPostShift(mLingInfo, ET9POSTSHIFTMODE_NEXT, (ET9U8 *)&totalWords, (ET9U8 *)&wordIndex);
}

void alpha_data::breakContext()
{
	if (mLingInfo == 0) {
		return;
	}

	setContext(NULL, 0);
}

void alpha_data::setContext(const unsigned short* newContext, int contextLen)
{
	if (mLingInfo != 0) {
		ET9AWFillContextBuffer(mLingInfo, (unsigned short*)newContext, contextLen);
	}
}

int alpha_data::addCustomSymbolSet(unsigned short* const symbols, int * const prob, const int symbolCount, int shiftState, int wordindex)
{
	return data::addCustomSymbolSet(symbols, prob, symbolCount, shiftState, wordindex);
}

bool alpha_data::ReCaptureWord(int kdbId, unsigned short * const word, int wordLen)
{
	return data::ReCaptureWord(kdbId, word, wordLen) == ET9STATUS_NONE;
}

int alpha_data::isUpperSymbol(const unsigned short symbol)
{
	return data::isUpperSymbol(symbol);
}

int alpha_data::isLowerSymbol(const unsigned short symbol)
{
	return data::isLowerSymbol(symbol);
}

unsigned short alpha_data::toLowerSymbol(const unsigned short symbol)
{
	return data::toLowerSymbol(symbol);
}

unsigned short alpha_data::toUpperSymbol(const unsigned short symbol)
{
	return data::toUpperSymbol(symbol);
}

void alpha_data::setWordCompletion(bool set)
{
    if (set) {
        ET9AWSetDBCompletion(mLingInfo);
    }
    else {
        ET9AWClearDBCompletion(mLingInfo);
    }
}

void alpha_data::setWordCompletionPoint(int value)
{
    ET9AWSysSetWordCompletionPoint(mLingInfo, (ET9U16)value);
}

int alpha_data::getWordCompletionPoint()
{
	return ET9AWSys_GetWordCompletionPoint(mLingInfo);
}

void alpha_data::setSpellCorrectionMode(ET9ASPCMODE mode)
{
    ET9AWSysSetSpellCorrectionMode(mLingInfo, mode, false);
}

void alpha_data::setAutoAppend(bool set)
{
    if (set) {
        ET9AWSetAutoAppendInList(mLingInfo);
    }
    else {
        ET9AWClearAutoAppendInList(mLingInfo);
    }
}

void alpha_data::setExactInlist(ET9AEXACTINLIST exact)
{
    ET9AWSetExactInList(mLingInfo, exact);
}

void alpha_data::setSelectionListMode(ET9ASLCORRECTIONMODE mode)
{
    ET9AWSysSetSelectionListMode(mLingInfo, mode);
}

ET9ASLCORRECTIONMODE alpha_data::getSelectionListMode()
{
	return ET9AWSys_GetSelectionListMode(mLingInfo);
}

void alpha_data::setRegionalCorrection(bool set)
{
    if (set) {
        ET9KDB_SetRegionalMode(mKdbInfo);
    }
    else {
        ET9KDB_SetDiscreteMode(mKdbInfo);
    }
}

void alpha_data::setDBStems(bool set)
{
    if (set) {
    	ET9AWSetDBStems(mLingInfo);
    }
    else {
    	ET9AWClearDBStems(mLingInfo);
    }
}

bool alpha_data::getDBStems()
{
	return ET9WORDSTEMS_MODE(mLingCmnInfo);
}

void alpha_data::setWordStemsPoint(int value)
{
	ET9AWSysSetWordStemsPoint(mLingInfo, value);
}

int alpha_data::getWordStemsPoint() {
	return ET9AWSys_GetWordStemsPoint(mLingInfo);
}

bool alpha_data::setBigramLangModel(bool bOn)
{
    ET9STATUS status;
    if (bOn) {
        status = ET9AWSysSetBigramLM(mLingInfo);
        LOGI("alpha_data::ET9AWSysSetBigramLM() - status = 0x%X", status);
    }
    else {
        status = ET9AWSysClearBigramLM(mLingInfo);
        LOGI("alpha_data::ET9AWSysClearBigramLM() - status = 0x%X", status);
    }
    return status == ET9STATUS_NONE;
}

bool alpha_data::setBoostingCandidate(bool bOn)
{
    ET9STATUS status;
    if (bOn) {
        status = ET9AWSysSetBoostTopCandidate(mLingInfo);
        LOGI("alpha_data::ET9AWSysSetBoostTopCandidate() - status = 0x%X", status);
    }
    else {
        status = ET9AWSysClearBoostTopCandidate(mLingInfo);
        LOGI("alpha_data::ET9AWSysClearBoostTopCandidate() - status = 0x%X", status);
    }
    return status == ET9STATUS_NONE;
}

bool alpha_data::setAutoSpacing(bool bOn)
{
	return ((bOn) ? ET9AWSysSetAutoSpace(mLingInfo) : ET9AWSysClearAutoSpace(mLingInfo)) == ET9STATUS_NONE;
}

bool alpha_data::setTraceFilter(ET9ASPCTRACESEARCHFILTER eFilter)
{
	LOGI("alpha_data::setTraceFilter() - value = 0x%X", eFilter);

	return ET9AWSysSetSpellCorrectionTraceSearchFilter(mLingInfo, eFilter) == ET9STATUS_NONE;
}

int alpha_data::getExactType(unsigned short* word, int maxLen)
{
    ET9SimpleWord simpleWord;
    int copy = 0;

    if (ET9GetExactWord(mWordSymbInfo, &simpleWord, 0, 0) == ET9STATUS_NONE) {
        copy = simpleWord.wLen < maxLen ? simpleWord.wLen : maxLen;
        data::wordCopy(word, simpleWord.sString, copy);
        return copy;
    }
    else
    {
    	int len;

        for(len = 0; (len < mWordSymbInfo->bNumSymbs) && (len < maxLen); len++)
        {
        	word[len] = mWordSymbInfo->SymbsInfo[len].DataPerBaseSym[0].sChar[0];
        }

        return len;
    }
}

int alpha_data::getInlineText(unsigned short* word, int maxLen)
{
    ET9SimpleWord simpleWord;
    ET9BOOL isUnknown;
    int copy = 0;

    if (ET9AWSelLstGetInlineWord(mLingInfo, &simpleWord, &isUnknown) == ET9STATUS_NONE) {
        copy = simpleWord.wLen < maxLen ? simpleWord.wLen : maxLen;
        data::wordCopy(word, simpleWord.sString, copy);
    }
    return copy;
}

bool alpha_data::isInlineKnown()
{
    ET9SimpleWord simpleWord;
    ET9BOOL isUnknown;

    return (ET9AWSelLstGetInlineWord(mLingInfo, &simpleWord, &isUnknown) == ET9STATUS_NONE) && (!isUnknown);
}

int alpha_data::getKdbId(bool primaryKeyboardId /* = true */)
{
    primaryKeyboardId = true; // not reference

    return data::getKdbId();
}

int alpha_data::getLanguageId(bool primaryLanguage /* = true */)
{
    primaryLanguage = true; // not reference

    return mCurrentLdbId;
}

bool alpha_data::isAlpha(unsigned short ch)
{
    return ET9GetSymbolClass(ch) == ET9SYMALPHA;
}

void alpha_data::changeDefaultWordIndex(int index)
{
    // change the default but not locking it
    mLingInfo->pLingCmnInfo->Private.bDefaultIndex = (ET9U8)index;
}

bool alpha_data::udb_add(const unsigned short* buffer, int len)
{
    bool ret = ET9AWUDBAddWord(mLingInfo, (ET9SYMB*)buffer, len) == ET9STATUS_NONE;
	if(ret) {
		flushUdb();
	}
    return ret;
}

bool alpha_data::udb_delete(const unsigned short* buffer, int len)
{
    bool ret = ET9AWUDBDeleteWord(mLingInfo, (ET9SYMB*)buffer, len) == ET9STATUS_NONE;
	if(ret) {
		flushUdb();
	}
    return ret;
}

bool alpha_data::udb_find(const unsigned short* buffer, int len)
{
    return ET9AWUDBFindWord(mLingInfo, (ET9SYMB*)buffer, len) == ET9STATUS_NONE;
}

bool alpha_data::udb_getNext(unsigned short* word, int& wordLen, int maxWordLen)
{
    return ET9AWUDBGetWord(mLingInfo, word, maxWordLen, (ET9U16*)&wordLen, 1) == ET9STATUS_NONE;
}

int alpha_data::udb_getSize()
{
    return (mLingCmnInfo->pRUDBInfo->wDataSize);
}

int alpha_data::udb_getRemainingMemory()
{
    return (mLingCmnInfo->pRUDBInfo->wRemainingMemory);
}

bool alpha_data::udb_scanBuf(const unsigned short* buffer, int len)
{
	return ET9AWScanBufForCustomWords(mLingInfo, (ET9SYMB*)buffer, len, 0) == ET9STATUS_NONE;
}

int  alpha_data::udb_count()
{
    int wordCount = 0;
    ET9AWUDBGetWordCount(mLingInfo, (ET9U16*)&wordCount);
    return (wordCount);
}

void alpha_data::udb_reset()
{
    ET9AWRUDBReset(mLingInfo);
    flushUdb();
}

bool alpha_data::asdb_add(const unsigned short* word, int wordLen, const unsigned short* subs, int subsLen)
{
    bool ret = ET9AWASDBAddEntry(mLingInfo, (ET9SYMB*)word, (ET9SYMB*)subs, wordLen, subsLen) == ET9STATUS_NONE;
	if(ret) {
		flushAsdb();
	}
    return ret;
}

bool alpha_data::asdb_delete(const unsigned short* buffer, int len)
{
    bool ret = ET9AWASDBDeleteEntry(mLingInfo, (ET9SYMB*)buffer, len) == ET9STATUS_NONE;
	if(ret) {
		flushAsdb();
	}
    return ret;
}

bool alpha_data::asdb_find(const unsigned short* word, int wordLen, const unsigned short* subs, int subsLen)
{
    ET9U16 dummy;
    return ET9AWASDBFindEntry(mLingInfo, (ET9SYMB*)word, wordLen, (ET9SYMB*)subs, subsLen, &dummy) == ET9STATUS_NONE;
}

bool alpha_data::asdb_getNext(unsigned short* word, int& wordLen,
        unsigned short* subs, int& subsLen, int maxLen)
{
    return ET9AWASDBGetEntry(mLingInfo, (ET9SYMB*)word, maxLen, (ET9U16*)&wordLen, (ET9SYMB*)subs, maxLen, (ET9U16*)&subsLen, 1) == ET9STATUS_NONE;
}

int  alpha_data::asdb_count()
{
    int count = 0;
    ET9AWASDBGetEntryCount(mLingInfo, (ET9U16*)&count);
    return count;
}

void alpha_data::asdb_reset()
{
    ET9AWASDBReset(mLingInfo);
    flushAsdb();
}
///
// MDB
void alpha_data::registerMdb(ET9MDBCALLBACK callback)
{
    ET9STATUS status;

    if (callback) {
        status = ET9AWRegisterMDB(mLingInfo, callback);
        LOGV("alpha_data::registerMdb() - status = 0x%X", status);
    }
    else {
        status = ET9AWUnregisterMDB(mLingInfo);
        LOGV("alpha_data::unregisterMdb() - status = 0x%X", status);
    }
}

//
// mdb
//
#define ISWHITESPACE(c) ((c) == '\t' || (c) == ' ' || (c) == '\n')

alpha_data::mdb::mdb(alpha_data* _alpha_data) :
    mBuffer(0), mAlpha_data(_alpha_data)
{}

alpha_data::mdb::~mdb()
{
    if (mBuffer) {
    	FREE(mBuffer);
    }
}

void alpha_data::mdb::loadFile(char* filename)
{
    LOGV("alpha_data::mdb::loadFile(\"%s\")...", filename ? filename : "null");

    // register mdb iff, file exist and file does contains word
    if (!filename || !*filename) {
        return;
    }

    unsigned int wordCount = 0;
    int fileSize = 0;
    int bufferSize = 0;
    char* buffer = 0;
    FILE* file;

    if (mBuffer) {
    	FREE(mBuffer);
        mBuffer = 0;
    }

    if ((file = fopen(filename, "r")) != 0) {
        fileSize = file_size(file);
    }

    if (fileSize) {

        // The actual memory buffer size is file size times 2. Two because we support
        // two-byte unicode, plus one extra byte to store the word len of each word. Since
        // the mdb text file already included one byte line-feed for each line, we don't need to add
        // one extra byte for each word. Add 2 extra bytes at the end for ending marker(0x0000).
        // Note, the end result of buffer size is bigger than we need, but this is faster to
        // do it this way.

        bufferSize = fileSize * (sizeof(unsigned short)) + WORD_COUNT_SIZE_IN_BYTES + 2;
        mBuffer = (char*)CALLOC(bufferSize, 1);

        LOGV("alpha_data::mdb::loadFile()... file size = %d, required buffer size = %d", fileSize, bufferSize);
    }

    if (mBuffer) {
        char lineBuffer[ET9MAXUDBWORDSIZE + 1];
        unsigned short mdbWord[ET9MAXUDBWORDSIZE + 1];

        char* buffer = mBuffer;
        char* bufferEnd = mBuffer + bufferSize;

        // set word count to zero
        wordCount = 0;
        memcpy(mBuffer, &wordCount, WORD_COUNT_SIZE_IN_BYTES);
        buffer = mBuffer + WORD_COUNT_SIZE_IN_BYTES;

        while (fgets(lineBuffer, ET9MAXUDBWORDSIZE, file)) {

            // chop line feed
            int len = strlen(lineBuffer);
            if (len && lineBuffer[len - 1] == '\n') {
                lineBuffer[len - 1] = '\0';
            }

            // Note, stripping leading and trailing white spaces are already done when we
            // create the mdb txt file.

            // convert utf8 to two-byte unicode
            unsigned short unicodeLen = 0;
            if ((unicodeLen = utf8ToUcs2(mdbWord, ET9MAXUDBWORDSIZE, lineBuffer)) > 0) {

                // ensure that we don't run pass the end of the buffer
                if (buffer + WORD_LEN_SIZE_IN_BYTES + unicodeLen * sizeof(unsigned short) > bufferEnd) {
                    LOGW("alpha_data::mdb::loadFile()...reached the end of buffer prematurely");
                    assert(0);
                    break;
                }

                // copy mdb word
                memcpy(buffer, &unicodeLen, WORD_LEN_SIZE_IN_BYTES);
                buffer = buffer + WORD_LEN_SIZE_IN_BYTES;
                memcpy(buffer, mdbWord, unicodeLen * sizeof(unsigned short));
                buffer = buffer + unicodeLen * sizeof(unsigned short);

                // In case this is the last entry, zero the word length so the search can
                // be terminated.
                memset(buffer, 0, WORD_LEN_SIZE_IN_BYTES);

                // update word count
                wordCount++;
                memcpy(mBuffer, &wordCount, WORD_COUNT_SIZE_IN_BYTES);

                //LOGV("alpha_data::mdb::init()..added %s %d, len = %d", lineBuffer, wordCount, unicodeLen);

            }
            else {
                LOGW("alpha_data::mdb::loadFile()...failed to convert %s to unicode", lineBuffer);
            }
        }
    }

    if (file) {
        fclose(file);
    }

    if (wordCount == 0) {
    	FREE(mBuffer);
    }

    mAlpha_data->registerMdb(mBuffer ? alpha_data::mdb::callback : 0);

    LOGV("alpha_data::mdb::loadFile(%s)...%d mdb word added", filename, wordCount);

}

//TODO: never called
void alpha_data::mdb::deInit()
{
    // unregister mdb
    mAlpha_data->registerMdb(0);
}

ET9STATUS alpha_data::mdb::read(
        ET9REQMDB eRequestType,     // request type
        ET9U16 wActiveWordLen,      // word len to search
        ET9U16 wMaxWordLen,         // max word len
        ET9SYMB* psBuffer,          // out buffer
        ET9U16*  pwBufferLen,       // out len of word copied (psBuffer)
        ET9U32*  pdwWordIndex)      // I/O word index
{
    if (mBuffer == 0) {
        return ET9STATUS_ERROR;
    }

    unsigned short wordLen = 0;
    unsigned int wordCount = 0;

    memcpy(&wordCount, mBuffer, WORD_COUNT_SIZE_IN_BYTES);

    if (*pdwWordIndex == 0) {
        // get the first word
        mCurrent = mBuffer + WORD_COUNT_SIZE_IN_BYTES;
    }
    else {
        // advance to the next word
        memcpy(&wordLen, mCurrent, WORD_LEN_SIZE_IN_BYTES);
        mCurrent = mCurrent + wordLen * sizeof(unsigned short) + WORD_LEN_SIZE_IN_BYTES;
    }

    // get the current word length
    memcpy(&wordLen, mCurrent, WORD_LEN_SIZE_IN_BYTES);

    while (wordLen) {               // zero word length indicates end of search/buffer

        // truncated
        if (wordLen > wMaxWordLen) {
            wordLen = wMaxWordLen;
        }

        if (wordLen >= wActiveWordLen) {
            (*pdwWordIndex)++;
            (*pwBufferLen) = wordLen;
            data::wordCopy((unsigned short*)psBuffer, (unsigned short*)(mCurrent + WORD_LEN_SIZE_IN_BYTES), wordLen);

            // LOGV("alpha_data::mdb::read()...found mdb word len = %d, index = %d", wordLen, (int)*pdwWordIndex);

            return (ET9STATUS_NONE);
        }

        // search for the next matching length
        mCurrent = mCurrent + wordLen * sizeof(unsigned short) + WORD_LEN_SIZE_IN_BYTES;
        memcpy(&wordLen, mCurrent, WORD_LEN_SIZE_IN_BYTES);
    }

    return (ET9STATUS_ERROR);
}

// static
ET9STATUS alpha_data::mdb::callback(
        ET9AWLingInfo* pLingInfo,
        ET9REQMDB eRequestType,     // request type
        ET9U16 wActiveWordLen,            // word len
        ET9U16 wMaxWordLen,         // max word len
        ET9SYMB* psBuffer,          // out buffer
        ET9U16*  pwBufferLen,       // out len of word copied (psBuffer)
        ET9U32*  pdwWordIndex)      // I/O word index
{
    alpha_data* _alpha_data = 0;
    _alpha_data = reinterpret_cast<alpha_data*>(pLingInfo->pPublicExtension);

    if (_alpha_data && _alpha_data->mMdb) {
        return _alpha_data->mMdb->read(eRequestType, wActiveWordLen, wMaxWordLen, psBuffer, pwBufferLen, pdwWordIndex);
    }

    return (ET9STATUS_ERROR);
}

} // namespace mocainput
