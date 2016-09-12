
#include "Log.h"
#include <assert.h>

//#include "et9cpapi.h"
#include "chinese_data.h"
#include "load_file.h"
#include "mem_alloc.h"
namespace mocainput {

//static
int chinese_data::instanceCount = 0;
chinese_data* chinese_data::singletonChineseDataInstance = 0;

chinese_data* chinese_data::getInstance(DBRegistry* dbRegistry)
{
	if (chinese_data::singletonChineseDataInstance == 0) {
		chinese_data::singletonChineseDataInstance = new chinese_data(dbRegistry);
		chinese_data::singletonChineseDataInstance->create();
	}

	++chinese_data::instanceCount;

	LOGV("chinese_data::getInstance() refCount = %d chinese_data:0x%X", chinese_data::instanceCount, chinese_data::singletonChineseDataInstance);

	return chinese_data::singletonChineseDataInstance;
}

void chinese_data::deleteInstance()
{
	LOGV("chinese_data::deleteInstance() refCount = %d", chinese_data::instanceCount);

	if (chinese_data::singletonChineseDataInstance != 0 && --chinese_data::instanceCount == 0) {
		chinese_data::singletonChineseDataInstance->destroy();
		delete chinese_data::singletonChineseDataInstance;
		chinese_data::singletonChineseDataInstance = 0;
	}
}

ET9CPPhrase chinese_data::sPhrase;
ET9CPSpell  chinese_data::sSpell;

ET9STATUS chinese_data::LdbReadCallback(ET9CPLingInfo *pLingInfo,
										ET9U8         **ppbSrc,
										ET9U32        *pdwSizeInBytes)
{
	chinese_data* _this = 0;

	char * data;
	int size;

	_this = reinterpret_cast<chinese_data*> (pLingInfo->pPublicExtension);
	LOGV("chinese_data::LdbReadCallback ldbNum(%x)", pLingInfo->wLdbNum);
	if (_this && _this->readLdb((int) pLingInfo->wLdbNum, (char**) &data,
			(int*) &size)) {
		*ppbSrc = (ET9U8*) data;
		*pdwSizeInBytes = size;
		return ET9STATUS_NONE;
	}

	return ET9STATUS_READ_DB_FAIL;
}

/*---------------------------------------------------------------------------
 *
 *   Function: ET9AWLdbReadData
 *
 *   Synopsis: This function reads LDB data - using direct access
 *
 *      Input: pLingInfo            = Pointer to Alpha information structure.
 *             ppbSrc               = Pointer to the variable that receives the data pointer
 *             pdwSizeInBytes       = Pointer to the variable that receives the data size in bytes
 *
 *     Return: ET9STATUS_NONE on success, otherwise return ET9 error code.
 *
 *---------------------------------------------------------------------------*/
ET9STATUS chinese_data::AWLdbReadCallback(
                    ET9AWLingInfo       *pLingInfo,
                    ET9U8 * /*ET9FARDATA*/  *ppbSrc,
                    ET9U32              *pdwSizeInBytes)
{
    alpha_data* _this = 0;

    _this = reinterpret_cast<alpha_data*>(pLingInfo->pPublicExtension);
    LOGV("chinese_data::AWLdbReadCallback ldbNum(%x),this(%x)",pLingInfo->pLingCmnInfo->wLdbNum,_this);

     if (_this && _this->readLdb(
             (int)pLingInfo->pLingCmnInfo->wLdbNum,
             (char**)ppbSrc,
             (int*)pdwSizeInBytes))
     {
         return ET9STATUS_NONE;
     }

    return ET9STATUS_READ_DB_FAIL;
}

chinese_data::chinese_data(DBRegistry* dbRegistry) : data(dbRegistry)
{
	LOGV("chinese_data::chinese_data()");

    mLingInfo = 0;
    mUdb = 0;
    mCurrentLdbId = 0;

#ifdef ET9_KDB_TRACE_MODULE
    sCTAWLingInfo = (ET9AWLingInfo*)CALLOC(sizeof(ET9AWLingInfo), 1);
    sCTAWLingCmnInfo = (ET9AWLingCmnInfo*)CALLOC(sizeof(ET9AWLingCmnInfo), 1);
#endif
}

chinese_data::~chinese_data()
{
	LOGV("chinese_data::~chinese_data()");
#ifdef ET9_KDB_TRACE_MODULE
	FREE(sCTAWLingInfo);
	FREE(sCTAWLingCmnInfo);
#endif
}

bool chinese_data::create()
{
    LOGV("chinese_data::create()...");

    if (mWordSymbInfo == 0 || mKdbInfo == 0) {
    	LOGE("chinese_data::create() ... mWordSymbInfo or mKdbInfo is null");
        return false;
    }

    if (mLingInfo == 0) {
        if ((mLingInfo = (ET9CPLingInfo*)CALLOC(1, sizeof(*mLingInfo))) == 0) {
            LOGE("create()...failed to create mLingInfo");
            return false;
        }
        memset(mLingInfo, 0, sizeof(ET9CPLingInfo));
    }

    ET9STATUS status = ET9CPSysInit(mLingInfo, mWordSymbInfo, this);

    LOGV("chinese_data::create()...done with status(%X)", status);

    return status == ET9STATUS_NONE;
}

void chinese_data::destroy()
{
    LOGV("chinese_data::destroy()...");

    FREE(mLingInfo);

    mCurrentKdbId = 0;
    mCurrentLdbId = 0;

    DELETE(mUdb);

    LOGV("chinese_data::destroy()...done");
}

bool chinese_data::start()
{
	LOGV("chinese_data::start()");

    return true;
}

void chinese_data::finish()
{
	LOGV("chinese_data::finish()...");

    if (mUdb != 0) {
        mUdb->flush();
    }
    data::clearAllKeys();

    LOGV("chinese_data::finish()...done");
}

int chinese_data::initTrace(int kdbId, int width, int height)
{
#ifndef ET9_KDB_TRACE_MODULE

    LOGE("data::initTrace - ERROR: Using TRACE while ET9_KDB_TRACE_MODULE is not define");

	return ET9STATUS_ERROR;
#else
	int status;
	status = data::initTrace(kdbId, width, height);
    assert(ET9STATUS_NONE == status);
    
	memset(sCTAWLingInfo,0,sizeof(ET9AWLingInfo));
	memset(sCTAWLingCmnInfo,0,sizeof(ET9AWLingCmnInfo));
	ET9AWLingInfo * pAWLingInfo = sCTAWLingInfo;

    /* Set up AWLingInfo */
	status = ET9AWSysInit(pAWLingInfo, 
		sCTAWLingCmnInfo, 
		mWordSymbInfo, 
		1, 
		ET9MAXSELLISTSIZE, 
		pCTAWWordList,
		this);
	LOGV("chinese_data::initTrace()...ET9AWSysInit(%x)",status);
	assert(ET9STATUS_NONE == status);

	status = ET9AWLdbInit(pAWLingInfo, &AWLdbReadCallback);
	assert(ET9STATUS_NONE == status);
	LOGV("chinese_data::initTrace()...ET9AWLdbInit(%x)",status);

//	status = ET9AWLdbSetLanguage(pAWLingInfo, ( 1 * 256) + 211, 0);
//	assert(ET9STATUS_NONE == status);
//	LOGV("chinese_data::initTrace()...ET9AWLdbSetLanguage(%x)",status);

    /* Recommended AW settings */
//	status = ET9ClearDownshiftDefault(pAWLingInfo);
//	assert(ET9STATUS_NONE == status);
//	LOGV("chinese_data::initTrace()...ET9ClearDownshiftDefault(%x)",status);
    
        //BAD!
        //_ET9AWSysSetSelectionListMode(pAWLingInfo, ET9ASLMODE_COMPLETIONSPROMOTED);
//        ET9AWSysSetWordCompletionPoint(pAWLingInfo, 1);
    
    /* Pass AWLingInfo to Chinese core */
	status = ET9CPTraceInit(mLingInfo, pAWLingInfo);
	assert(ET9STATUS_NONE == status);
	LOGV("chinese_data::initTrace()...ET9CPTraceInit(%x)",status);

//    status = ET9CPTraceUdbActivate(mLingInfo);
//	assert(ET9STATUS_NONE == status);
//	LOGV("chinese_data::initTrace()...ET9CPTraceUdbActivate(%x)",status);

	return status;
#endif
}
int chinese_data::composeKDBId(unsigned short keyboardLayoutId)
{
    unsigned short kdbId = 0;

    // keyboard id is composed of keyboard id layout (the upper byte) and language
    // primary id (lower byte)
    if (ET9SKIDPhonePadPinyin == (KEYBOARD_LAYOUT_MASK & keyboardLayoutId)) {
        int mode = getInputMode();
        if (ET9CPMODE_PINYIN == mode || ET9CPMODE_BPMF == mode) {
            return (ET9PLIDChinese | ET9SKIDPhonePadPinyin);
        }
    }
    return 0;
}

int chinese_data::addTone(int tone, unsigned short * spell, int spellLen)
{
    ET9STATUS status;
    ET9CPSpell sSpell;

    if (!spell || spellLen == 0) {
    	status = ET9CPGetSpell(mLingInfo, &sSpell);
    	assert(ET9STATUS_NONE == status);
    } else {
    	data::wordCopy(sSpell.pSymbs, spell, spellLen);
    	sSpell.bLen = spellLen;
    }

    status = ET9CPAddToneSymb(mWordSymbInfo, &sSpell, (ET9CPSYMB)(ET9CPTONE1 - 1 + tone));
    return status;
}

int chinese_data::addDelimiter()
{
    return ET9AddExplicitSymb(mWordSymbInfo, ET9CPSYLLABLEDELIMITER, ET9NOSHIFT, ET9_NO_ACTIVE_INDEX);
}

int chinese_data::buildWordList()
{
    ET9STATUS status;

    status = ET9CPBuildSelectionList(mLingInfo);

    LOGD("buildWordList(%d keys)...  status(%d)", mWordSymbInfo->bNumSymbs, status);
    return status;
}

int chinese_data::getWord(unsigned short wordIndex, unsigned short* word, int& wordLen, int maxLen)
{
    ET9STATUS status;
    ET9CPPhrase sPhrase;

    LOGD("chinese_data::getWord(%d)...", wordIndex);

    status = ET9CPGetPhrase(mLingInfo, (ET9U16)wordIndex, &sPhrase, NULL);
    LOGD("chinese_data::getWord ET9CPGetPhrase done");
    if (status == ET9STATUS_NEED_SELLIST_BUILD) {
    	buildWordList();
    	LOGD("chinese_data::getWord buildWordList done");
    	status = ET9CPGetPhrase(mLingInfo, (ET9U16)wordIndex, &sPhrase, NULL);
    	LOGD("chinese_data::getWord ET9CPGetPhrase done");
    }
    if (status) {
    	LOGD("chinese_data::getWord return status done");
        return status;
    }
    if (sPhrase.bLen > maxLen) {
    	LOGD("chinese_data::getWord return ET9STATUS_BUFFER_TOO_SMALL done");
        return ET9STATUS_BUFFER_TOO_SMALL;
    }

    wordLen = sPhrase.bLen;
    data::wordCopy(word, (const unsigned short*)sPhrase.pSymbs, wordLen);

    LOGD("chinese_data::getWord done(%d)...status %d", wordIndex, status);
    return status;
}

int chinese_data::getSelection(unsigned short* selection, int& selectionLen, int maxLen)
{
    ET9STATUS status;
    ET9CPPhrase sPhrase;

    status = ET9CPGetSelection(mLingInfo, &sPhrase, 0, 0);
    if (ET9STATUS_EMPTY == status) {
        sPhrase.bLen = 0;
        status = ET9STATUS_NONE;
    }
    if (status) {
        return status;
    }
    if (sPhrase.bLen > maxLen) {
        return ET9STATUS_BUFFER_TOO_SMALL;
    }

    selectionLen = sPhrase.bLen;
    data::wordCopy(selection, (const unsigned short*)sPhrase.pSymbs, selectionLen);

    return status;
}

unsigned int chinese_data::getToneOptions()
{
    ET9STATUS status;
    ET9U8 bToneMask;

    status = ET9CPGetToneOptions(mLingInfo, &bToneMask);
    if (ET9STATUS_NONE == status) {
        return bToneMask;
    }
    return 0;
}

bool chinese_data::haveBuild()
{
	// TODO which function should be used to determine the status
	// ET9CPGetActivePrefixIndex or ET9CPGetSpell or ET9CPGetPhrase?
    return ET9STATUS_NEED_SELLIST_BUILD != ET9CPGetActivePrefixIndex(mLingInfo, 0);
}

int chinese_data::setLanguage(int langId)
{
	LOGD("chinese_data::setLanguage(%04X), currentLanguage = %04X ...", langId, mCurrentLdbId);

    ET9STATUS status = ET9STATUS_NONE;
    ET9U16 ldbId = (ET9U16) langId;

    if (mCurrentLdbId != (ldbId & ET9PLIDMASK)) {

        if ((langId & ET9PLIDMASK) != ET9PLIDNone) {

            status = ET9CPLdbValidate(mLingInfo, langId, &LdbReadCallback);

            if (status) {
                LOGE("setLanguage(%X)..ET9CPLdbValidate .failed with status(%X)", ldbId, status);
                return false;
            }

            mLingInfo->pUdb = NULL;
            mLingInfo->pfUdbWrite = NULL;

            status = ET9CPLdbInit(mLingInfo, ldbId, &LdbReadCallback);

            if (status) {
                LOGE("setLanguage(%X)..ET9CPLdbInit .failed with status(%X)", ldbId, status);
                return false;
            }

            //todo: multiple languages -> multiple UDBs?
            if (ET9STATUS_NONE == status) {
                if (mUdb != 0) {
                    delete mUdb;
                    mUdb = NULL;
                }
                initUdb(ldbId);
            }

            mCurrentLdbId = ldbId;
        }
    }

	LOGD("chinese_data::setLanguage(%04X), currentLanguage = %04X ...status = %d", langId, mCurrentLdbId, status);

    status = ET9CPSetPartialSpell(mLingInfo);

    LOGE("chinese_data::setLanguage() ET9CPSetPartialSpell()=> done with status(%X)", status);

    return status;
}

int chinese_data::getInputMode()
{
    int mode;

    if (ET9CPIsModePinyin(mLingInfo)) {
        mode = MODE_PINYIN;
    }
    else if (ET9CPIsModeBpmf(mLingInfo)) {
        mode = MODE_BPMF;
    }
    else if (ET9CPIsModeStroke(mLingInfo)) {
        mode = MODE_STROKE;
    }
    else if (ET9CPIsModeCangJie(mLingInfo)) {
        mode = MODE_CANGJIE;
    }
    else if (ET9CPIsModeQuickCangJie(mLingInfo)) {
        mode = MODE_QUICK_CANGJIE;
    }
    else {
        mode = -1;
    }

    return mode;
}

int chinese_data::setInputMode(int mode)
{
    ET9CPMode eMode;
    LOGD("setInputMode(%02X)", mode);

    switch (mode) {
    case MODE_PINYIN:
        eMode = ET9CPMODE_PINYIN;
        break;
    case MODE_BPMF:
        eMode = ET9CPMODE_BPMF;
        break;
    case MODE_STROKE:
        eMode = ET9CPMODE_STROKE;
        break;
    case MODE_CANGJIE:
        eMode = ET9CPMODE_CANGJIE;
        break;
    case MODE_QUICK_CANGJIE:
        eMode = ET9CPMODE_QUICK_CANGJIE;
        break;
    default:
        return -1;
    }

    int status;
    status = ET9CPSetInputMode(mLingInfo, eMode);
    return status;
}

int chinese_data::setAttribute(int id, int value)
{
    int status;
    if (id == CHINESE_NAME_INPUT)
    {
        if (value == CHINESE_NAME_INPUT_OFF)
            status = ET9CPClearNameInput(mLingInfo);
        else
            status = ET9CPSetNameInput(mLingInfo);
    }
    else if (id == CHINESE_MOHU_PINYIN)
    {
        status = ET9CPSetMohuPairs(mLingInfo, (ET9U16)value);
    }
    else
    {
        status = ET9STATUS_ERROR;
        LOGE("Invalid Id");
    }
    return status;
}

int chinese_data::selectWord(int wordIndex)
{
    ET9STATUS status;

    status = ET9CPSelectPhrase(mLingInfo, wordIndex, 0);

    if (ET9STATUS_NONE == status && mWordSymbInfo->bNumSymbs == 0) {
        status = ET9STATUS_ALL_SYMB_SELECTED;
    }

    return status;
}

int chinese_data::unselectWord()
{
    ET9STATUS status;

    status = ET9CPUnselectPhrase(mLingInfo);

    return status;
}

int chinese_data::setContext(const unsigned short * context, int contextLen)
{
    return ET9CPSetContext(mLingInfo, (ET9SYMB*)context, contextLen);
}

int chinese_data::breakContext()
{
    return ET9CPClearContext(mLingInfo);
}

int chinese_data::commitSelection()
{
    return ET9CPCommitSelection(mLingInfo);
}

int chinese_data::initUdb(int langId)
{
    if (mUdb == 0) {
        mUdb  = new data::persistentDb(getDbRegistry()->get_udb_path(langId));
    }
    ET9STATUS status = ET9STATUS_ERROR;

    if (mUdb) {
        mUdb->create(ET9CPUDBMINSIZE);
        if (mUdb->buffer()) {
            status = ET9CPUdbActivate(mLingInfo, 0, (ET9CPUdbInfo *)mUdb->buffer(), (ET9U16)mUdb->bufferSize());
            LOGV("chinese_data::initUdb(%p, %d) - status = 0x%X", mUdb->buffer(), mUdb->bufferSize(), status);
        }
    }
    return status;
}

int chinese_data::resetUserDictionary()
{
    return ET9CPUdbReset(mLingInfo);
}

int chinese_data::addWordToUserDictionary(const unsigned short* word, int wordLen,
                                                const unsigned short*spelling, int spellingLen)
{
    int status;
    ET9CPPhrase sPhrase;

    if (wordLen > ET9CPMAXPHRASESIZE) {
        return ET9STATUS_INVALID_INPUT;
    }

    data::wordCopy(sPhrase.pSymbs, word, wordLen);
    sPhrase.bLen = wordLen;

    status = ET9CPUdbAddPhrase(mLingInfo, &sPhrase, (unsigned short*)spelling, spellingLen);

    return status;
}

int chinese_data::getUserDictionaryWord(int index, unsigned short* word, int& wordLen, int maxLen)
{
    ET9STATUS status;
    ET9CPPhrase sPhrase;

    status = ET9CPUdbGetPhrase(mLingInfo, ET9CPUdbPhraseType_User_MASK, (ET9U16)index, &sPhrase, 0);

    if (status) {
        return status;
    }
    if (sPhrase.bLen > maxLen) {
        return ET9STATUS_BUFFER_TOO_SMALL;
    }

    wordLen = sPhrase.bLen;
    data::wordCopy(word, (const unsigned short*)sPhrase.pSymbs, wordLen);

    return status;
}

int chinese_data::deleteUserDictionaryWord(const unsigned short* word, int wordLen)
{
    int status;
    ET9CPPhrase sPhrase;

    if (wordLen > ET9CPMAXPHRASESIZE) {
        return ET9STATUS_INVALID_INPUT;
    }

    data::wordCopy(sPhrase.pSymbs, word, wordLen);
    sPhrase.bLen = wordLen;

    status = ET9CPUdbDeletePhrase(mLingInfo, &sPhrase);

    return status;
}

int chinese_data::symbToTone(unsigned short symb) {
    return ET9CPSymToCPTone(symb);
}

int chinese_data::getSpell(unsigned short* spell, int& spellLen, int maxLen)
{
    LOGV("getSpell...");
    ET9STATUS status;
    ET9CPSpell sSpell;

    status = ET9CPGetSpell(mLingInfo, &sSpell);

    if (status == ET9STATUS_NONE && sSpell.bLen > maxLen) {
        return ET9STATUS_BUFFER_TOO_SMALL;
    }
    else if (status != ET9STATUS_NONE) {
        return status;
    }
    else {
        spellLen = sSpell.bLen;
        data::wordCopy(spell, (const unsigned short*)sSpell.pSymbs, spellLen);//
    }

    LOGV("getSpell...status(%d) len=%d 1stchar=0x%04x", status, spellLen, spellLen ? sSpell.pSymbs[0] : '?');
    return status;
}

int chinese_data::getPrefixCount()
{
    return (int)ET9CPGetPrefixCount(mLingInfo);
}

int chinese_data::getPrefix(int prefixIndex, unsigned short* prefix, int& prefixLen, int maxLen)
{
    ET9STATUS status;
    ET9CPSpell sSpell;

    status = ET9CPGetPrefix(mLingInfo, (ET9U16)prefixIndex, &sSpell);

    if (status == ET9STATUS_NONE && sSpell.bLen > maxLen) {
        return ET9STATUS_BUFFER_TOO_SMALL;
    }
    else if (status) {
        return status;
    }

    prefixLen = sSpell.bLen;
    data::wordCopy(prefix, (const unsigned short*)sSpell.pSymbs, sSpell.bLen);

    return status;
}

int  chinese_data::getActivePrefixIndex()
{
    ET9STATUS status;
    ET9U8 bIndex = 0;

    status = ET9CPGetActivePrefixIndex(mLingInfo, &bIndex);
    LOGV("chinese_data::getActivePrefixIndex status=%x bIndex=%x", status, bIndex);

    if (status == ET9STATUS_EMPTY) {
    	bIndex = 0xFF;
    }

    return (int)bIndex;
}

int chinese_data::setActivePrefixIndex(int prefixIndex)
{
    ET9STATUS status;
    status = ET9CPSetActivePrefix(mLingInfo, (ET9U8)prefixIndex);

    return status;
}

int chinese_data::clearActivePrefixIndexIfNecessary()
{
	ET9U8 bPrefixIndex = 0;
	ET9STATUS status = ET9STATUS_NONE;
	status = ET9CPGetActivePrefixIndex(mLingInfo, &bPrefixIndex);
	if (status == ET9STATUS_NONE && 0xFF != bPrefixIndex)
	{
		status = ET9CPClearActivePrefix(mLingInfo);
	}

	return status;
}

bool chinese_data::udbAdd(const unsigned short* phrase, int phraseLen, const unsigned short* spell, int spellLen)
{
    chinese_data::sPhrase.bLen = (ET9U8)MIN(phraseLen, ET9CPMAXPHRASESIZE);
    data::wordCopy(chinese_data::sPhrase.pSymbs, phrase, chinese_data::sPhrase.bLen);
    bool ret = ET9CPUdbAddPhrase(mLingInfo, &chinese_data::sPhrase, (ET9U16*)spell, (ET9U8)spellLen) == ET9STATUS_NONE;
    if (mUdb != 0 && ret) {
        mUdb->flush();
    }
    return ret;
}

bool chinese_data::udbDelete(const unsigned short* phrase, int phraseLen)
{
    chinese_data::sPhrase.bLen = (ET9U8)MIN(phraseLen, ET9CPMAXPHRASESIZE);
    data::wordCopy(chinese_data::sPhrase.pSymbs, phrase, chinese_data::sPhrase.bLen);
    bool ret  = ET9CPUdbDeletePhrase(mLingInfo, &sPhrase) == ET9STATUS_NONE;
    if (mUdb != 0 && ret) {
        mUdb->flush();
    }
    return ret;
}

bool chinese_data::udbGetNext(int index,
        unsigned short* phrase, int& phraseLen, int maxPhraseLen,
        unsigned short* spell, int& spellLen, int maxSpellLen)
{
    if (ET9CPUdbGetPhrase(mLingInfo, ET9CPUdbPhraseType_User_MASK, (ET9U16)index, &chinese_data::sPhrase, &chinese_data::sSpell) == ET9STATUS_NONE) {
        phraseLen = MIN(maxPhraseLen, chinese_data::sPhrase.bLen);
        spellLen = MIN(maxSpellLen, chinese_data::sSpell.bLen);
        data::wordCopy(phrase, chinese_data::sPhrase.pSymbs, phraseLen);
        data::wordCopy(spell, chinese_data::sSpell.pSymbs, spellLen);
        return true;
    }

    return false;
}

int  chinese_data::udbCount()
{
    ET9U16 count = 0;
    if (ET9CPUdbGetPhraseCount(mLingInfo, ET9CPUdbPhraseType_User_MASK, &count) != ET9STATUS_NONE) {
        count = 0;
    }
    return count;
}

void chinese_data::udbReset()
{
    ET9CPUdbReset(mLingInfo);
    if (mUdb != 0) {
        mUdb->flush();
    }
}

int chinese_data::setCommonChar()
{
	return ET9CPSetCommonChar(mLingInfo);
}

int chinese_data::clearCommonChar()
{
	return ET9CPClearCommonChar(mLingInfo);
}

int chinese_data::setFullSentence()
{
	return ET9CPSetFullSentence(mLingInfo);
}

int chinese_data::clearFullSentence()
{
	return ET9CPClearFullSentence(mLingInfo);
}

bool chinese_data::isFullSentenceActive()
{
    return ET9CPIsFullSentenceActive(mLingInfo);
}

int chinese_data::getHomophonePhraseCount(unsigned short *basePhrase, int basePhraseLen, int *count)
{
    ET9STATUS status;
    ET9CPPhrase sPhrase;

    LOGD("Enter getHomophonePhraseCount()... with %s", (char*)basePhrase);

    chinese_data::sPhrase.bLen = (ET9U8)MIN(basePhraseLen, ET9CPMAXPHRASESIZE);
    data::wordCopy(chinese_data::sPhrase.pSymbs, basePhrase, chinese_data::sPhrase.bLen);

    status = ET9CPGetHomophonePhraseCount(mLingInfo, &chinese_data::sPhrase, (unsigned short *)count);

    LOGD("getHomophonePhraseCount(%d)...status %d", *count, status);
    return status;
}

int chinese_data::getHomophonePhrase(unsigned short *basePhrase, int basePhraseLen, int index, unsigned short *phrase, unsigned short *spell)
{
    ET9STATUS status;
    ET9CPPhrase sPhrase;
    ET9CPSpell sSpell;

    LOGD("Enter getHomophonePhrase(%d)... with %s", index, (char*)basePhrase);

    chinese_data::sPhrase.bLen = (ET9U8)MIN(basePhraseLen, ET9CPMAXPHRASESIZE);
    data::wordCopy(chinese_data::sPhrase.pSymbs, basePhrase, chinese_data::sPhrase.bLen);

    status = ET9CPGetHomophonePhrase(mLingInfo, &chinese_data::sPhrase, (ET9U16)index, &sPhrase, &sSpell);

    if (status == ET9STATUS_NONE)
    {
        data::wordCopy(phrase, (const unsigned short*)sPhrase.pSymbs, sPhrase.bLen);
        if (spell != NULL)
        {
        	data::wordCopy(spell, (const unsigned short*)sSpell.pSymbs, sSpell.bLen);
        }
    }

    LOGD("getHomophonePhrase(%d)...status %d", index, status);
    return status;
}

}
// namespace mocainput
