
#include "Log.h"

#include <wchar.h>
#include <assert.h>

#include "chinese_data.h"
#include "chinese_controller.h"

namespace mocainput {

chinese_controller::chinese_controller(DBRegistry* dbRegistry) : controller(dbRegistry)
{
    mData = 0;
    mFailedSelect = false;
    mActivePrefixIndex = 0;
}

chinese_controller::~chinese_controller()
{
}

bool chinese_controller::create()
{
	mData = chinese_data::getInstance(mDbRegistry);
	LOGV("chinese_controller::create mData(0x%X)...", mData);
    return mData != 0;
}

void chinese_controller::destroy()
{
    chinese_data::deleteInstance();
    mData = 0;
}

bool chinese_controller::start()
{
    if (mData == 0) {
        return false;
    }

    return mData->start();
}

void chinese_controller::finish()
{
	LOGV("chinese_controller::finish mData(0x%X)...", mData);
    if (mData) {
    	LOGV("mData start finish(0x%X)...", mData);
        mData->finish();
    	LOGV("mData end finish(0x%X)...", mData);
    }
}


bool chinese_controller::initTrace(int kdbId, int width, int height)
{
	if (mData == 0) {
		return false;
	}

	//if (kdbId != mData->getKdbId()) {
	//    setExactInlistMode(kdbId);
	//}

	return mData->initTrace(kdbId, width, height) == 0;
}
int chinese_controller::isAutoSpaceBeforeTrace(int kdbId, ET9TracePoint* points, int numPoints)
{
	if(!this->mData)
	{
		return false;
	}

	//if (kdbId != mData->getKdbId()) {
	//    setExactInlistMode(kdbId);
	//}

	return this->mData->isAutoSpaceBeforeTrace(kdbId, points, numPoints);
}

bool chinese_controller::processKey(int kdbId, int key)
{
    if (mData == 0) {
        return false;
    }

    int status;

    status = mData->processKey(kdbId, key);
    LOGV("chinese_controller::processKey status:%x", status);

    if (ET9STATUS_NONE != status) {
        return false;
    }
    return _tryBuild();
}

bool chinese_controller::processTap(int kdbId, int TapX, int TapY, int shiftState)
{
	if (mData == 0) {
		return false;
	}

	//if (kdbId != mData->getKdbId()) {
	//    setExactInlistMode(kdbId);
	//}

	switch ((kdbId & KEYBOARD_LAYOUT_MASK)) {
	case KEYBOARD_LAYOUT_QWERTY:
	case KEYBOARD_LAYOUT_AZERTY:
	case KEYBOARD_LAYOUT_QWERTZ:
        return mData->processTap(kdbId, TapX, TapY, shiftState) == ET9STATUS_NONE;
	    break;
	case KEYBOARD_LAYOUT_REDUCED_QWERTY:
	case KEYBOARD_LAYOUT_REDUCED_AZERTY:
	case KEYBOARD_LAYOUT_REDUCED_QWERTZ:
    case KEYBOARD_LAYOUT_PHONE_KEYPAD:
        //mData->setRegionalCorrection(false);
        // QN: don't need to return off regional since the keyboard should has been
        // built with regional turn off.
	    return mData->processTap(kdbId, TapX, TapY, shiftState) == ET9STATUS_NONE;
	    break;

    default:
	    LOGE("chinese_controller::processTap() failed - 0x%X is not a valid keyboard layout", kdbId);
	    break;
	}

	return false;
}

bool chinese_controller::processTrace(int kdbId, ET9TracePoint* points, int numPoints, int shiftState)
{
	if(!this->mData)
	{
		return false;
	}

	//if (kdbId != mData->getKdbId()) {
	//    setExactInlistMode(kdbId);
	//}

	return this->mData->processTrace(kdbId, points, numPoints, shiftState);
}

bool chinese_controller::isHasTraceInfo()
{
	if(!this->mData)
	{
		return false;
	}

	//if (kdbId != mData->getKdbId()) {
	//    setExactInlistMode(kdbId);
	//}

	return this->mData->isHasTraceInfo();
}
void chinese_controller::backupWordSymbolInfo()
{
	if(!this->mData)
	{
		return;
	}

	//if (kdbId != mData->getKdbId()) {
	//    setExactInlistMode(kdbId);
	//}

	this->mData->backupWordSymbolInfo();
}
void chinese_controller::restoreWordSymbolInfo()
{
	if(!this->mData)
	{
		return;
	}

	//if (kdbId != mData->getKdbId()) {
	//    setExactInlistMode(kdbId);
	//}

	this->mData->restoreWordSymbolInfo();
}

bool chinese_controller::clearKey()
{
    if (mData == 0) {
        return false;
    }

    int status;

    status = mData->clearKey();

    if (ET9STATUS_NONE == status) {
        status = _build();
        if (ET9STATUS_ALL_SYMB_SELECTED == status) {
            status = mData->unselectWord();
        }
        status = mData->clearActivePrefixIndexIfNecessary();
        if (status == ET9STATUS_NONE) {
        	mActivePrefixIndex = 0xFF;
        }
    }

    return _verifyBuild();
}

bool chinese_controller::clearAllKeys()
{
    if (mData == 0) {
        return false;
    }

    int nRet = mData->clearAllKeys();
    if (nRet ==  ET9STATUS_NONE || nRet == ET9STATUS_EMPTY)
        return  _verifyBuild();
    else
        return false;
}

int chinese_controller::getKeyCount()
{
    if (mData == 0) {
        return 0;
    }

    return mData->getKeyCount();
}

bool chinese_controller::setAttribute(int id, int value)
{
    int status;

    if (mData == 0) {
        return false;
    }

    status = mData->setAttribute(id, value);

    return ET9STATUS_NONE == status && clearAllKeys();
}

bool chinese_controller::setLanguage(int langId)
{
    int status;

    if (mData == 0) {
        return false;
    }

    status = mData->setLanguage(langId);

    return ET9STATUS_NONE == status && clearAllKeys();
}

int chinese_controller::getInputMode()
{
    if (mData == 0) {
        return 0;
    }

    return mData->getInputMode();
}

bool chinese_controller::setInputMode(int mode)
{
    int status;

    if (mData == 0) {
        return false;
    }

    status = mData->setInputMode(mode);

    return ET9STATUS_NONE == status && clearAllKeys();
}

bool chinese_controller::addDelimiter()
{
    int status;

    if (mData == 0) {
        return false;
    }

    status = mData->addDelimiter();

    return ET9STATUS_NONE == status && _tryBuild();
}

bool chinese_controller::addTone(int tone)
{
    int status;

    if (mData == 0) {
        return false;
    }

    unsigned short spell[ET9CPMAXSPELLSIZE];
    int spellLen;
    status = mData->getSpell(spell, spellLen, ET9CPMAXSPELLSIZE);
    if (status != ET9STATUS_NONE) {
        return false;
    }

    status = mData->addTone(tone, spell, spellLen);

    return ET9STATUS_NONE == status && _tryBuild();
}

bool chinese_controller::cycleTone()
{
    int status;
    unsigned short lastSymb;
    int tone;

    unsigned short spell[ET9CPMAXSPELLSIZE];
    int spellLen;

    unsigned int toneMask;

    if (mData == 0) {
        return false;
    }

    toneMask = mData->getToneOptions();

    status = mData->getSpell(spell, spellLen, ET9CPMAXSPELLSIZE);
    if (status) {
        return false;
    }
    assert(spellLen > 0);
    lastSymb = spell[spellLen - 1];
    LOGV("cycleTone - len: %d\tlast: %04x", spellLen, lastSymb);

    tone = mData->symbToTone(lastSymb);
    if (ET9CPSYLLABLEDELIMITER == lastSymb || tone) {
        status = mData->clearKey();
        assert(ET9STATUS_NONE == status);
        _build();
        assert(ET9STATUS_NONE == status);

        while (!((1 << tone) & toneMask) &&
                tone < 5)
        {
            tone++;
        }
    }
    else {
        tone = 5; /* add a delimiter */
        mActivePrefixIndex = mData->getActivePrefixIndex();
        LOGE("getActivePrefixIndex(%d)", mActivePrefixIndex);
    }

    if (5 == tone) {
    	if (mActivePrefixIndex < mData->getPrefixCount())
    	{
    		status = mData->setActivePrefixIndex(mActivePrefixIndex);
    		LOGE("cycleTone...set cached active prefix index to (%d)...status(%d)", mActivePrefixIndex, status);
    	}
        status = mData->addDelimiter();
    }
    else {
        status = mData->addTone(1+tone, spell, spellLen-1);
    }

    return ET9STATUS_NONE == status && _tryBuild();
}

bool chinese_controller::getWord(unsigned short wordIndex, unsigned short* word, int& wordLen, int maxLen)
{
    int status;

    if (mData == 0) {
        return false;
    }

    status = mData->getWord(wordIndex, word, wordLen, maxLen);

    return ET9STATUS_NONE == status;
}

bool chinese_controller::getSelection(unsigned short* str, int& strLen, int maxLen)
{
    LOGV("getSelection...");

    strLen = 0;

    int status;

    if (mData == 0) {
        return false;
    }

    status = mData->getSelection(str, strLen, maxLen);

    return ET9STATUS_NONE == status;
}

bool chinese_controller::selectWord(int index, unsigned short* insertText, int& insertTextLen, int maxLen)
{
    unsigned short word[ET9CPMAXPHRASESIZE];
    int wordLen;

    insertTextLen = 0;

    int status;

    if (mData == 0) {
        return false;
    }

    // We may need to know what component was selected
    status = mData->getWord(index, word, wordLen, ET9CPMAXPHRASESIZE);
    if (status) {
        return false;
    }

    status = mData->selectWord(index);
    if (ET9STATUS_SELECTED_CHINESE_COMPONENT == status) {
        mData->addExplicitKey(word[0]);
    }
    else if (ET9STATUS_ALL_SYMB_SELECTED == status) {
        mData->getSelection(word, wordLen, ET9CPMAXPHRASESIZE);
        if (wordLen > maxLen) {
            return false;
        }
        mData->commitSelection();
        mData->clearAllKeys();
        mData->wordCopy(insertText, word, wordLen);
        insertTextLen = wordLen;
    }
    else if (ET9STATUS_NONE != status) {
        //LOGE("chinese_controller::selectWord(%d) FAILED", index, )
        return false;
    }

    status = _build();
    // select can sometimes leave an invalid key sequence.
    // in this case, allow loyal spell until the keyseq is valid again.
    if (ET9STATUS_NO_MATCHING_WORDS == status) {
        mFailedSelect = true;
    }
    return true;
}

bool chinese_controller::setContext(const unsigned short * context, int contextLen)
{
    int status;

    if (mData == 0) {
        return false;
    }

    status = mData->setContext(context, contextLen);

    //todo: UI: allow moving cursor with keys? could break build
    return ET9STATUS_NONE == status && clearAllKeys();
}

bool chinese_controller::breakContext()
{
    int status;

    if (mData == 0) {
        return false;
    }

    status = mData->breakContext();

    //todo: ditto above
    return ET9STATUS_NONE == status && clearAllKeys();
}

bool chinese_controller::resetUserDictionary()
{
    int status;

    if (mData == 0) {
        return false;
    }

    status = mData->resetUserDictionary();

    return ET9STATUS_NONE == status && clearAllKeys();
}

bool chinese_controller::addWordToUserDictionary(const unsigned short* word, int wordLen,
                                                        const unsigned short*spelling, int spellingLen)
{
    int status;

    if (mData == 0) {
        return false;
    }

    status = mData->addWordToUserDictionary(word, wordLen, spelling, spellingLen);

    return ET9STATUS_NONE == status && clearAllKeys();
}

bool chinese_controller::getUserDictionaryWord(int index, unsigned short* word, int& wordLen, int maxLen)
{
    int status;

    if (mData == 0) {
        return false;
    }

    status = mData->getUserDictionaryWord(index, word, wordLen, maxLen);

    return ET9STATUS_NONE == status;
}

bool chinese_controller::deleteUserDictionaryWord(const unsigned short* word, int wordLen)
{
    int status;

    if (mData == 0) {
        return false;
    }

    status = mData->deleteUserDictionaryWord(word, wordLen);

    return ET9STATUS_NONE == status && clearAllKeys();
}

bool chinese_controller::getSpell(unsigned short* spell, int& spellLen, int maxLen)
{
    int status;

    if (mData == 0) {
        return false;
    }

    LOGV("getSpell - getting spell");
    status = mData->getSpell(spell, spellLen, maxLen);

    return ET9STATUS_NONE == status;
}

int chinese_controller::getPrefixCount()
{
    if (mData == 0) {
        return 0;
    }

    int nRet = mData->getPrefixCount();
    return nRet;
}

bool chinese_controller::getPrefix(int prefixIndex, unsigned short* prefix, int& prefixLen, int maxLen)
{
    int status;

    if (mData == 0) {
        return false;
    }

    status = mData->getPrefix(prefixIndex, prefix, prefixLen, maxLen);

    return ET9STATUS_NONE == status;
}

int  chinese_controller::getActivePrefixIndex()
{
    if (mData == 0) {
        return 0;
    }

    int nRet = mData->getActivePrefixIndex();
    return nRet;
}

bool chinese_controller::setActivePrefixIndex(int prefixIndex)
{
    if (mData == 0) {
        return 0;
    }

    int ret = mData->setActivePrefixIndex(prefixIndex);

    if (ret == ET9STATUS_NONE) {
        unsigned short spell[ET9CPMAXSPELLSIZE];
        int spellLen;
        int status_spell;
        status_spell = mData->getPrefix(prefixIndex, spell, spellLen, ET9CPMAXSPELLSIZE);
        mActivePrefixIndex = prefixIndex;
    }

    return ret == ET9STATUS_NONE;
}

bool chinese_controller::udbAdd(const unsigned short* phrase, int phraseLen, const unsigned short* spell, int spellLen)
{
    if (mData == 0) {
        return false;
    }

    return mData->udbAdd(phrase, phraseLen, spell, spellLen);
}

bool chinese_controller::udbDelete(const unsigned short* phrase, int phraseLen)
{
    if (mData == 0) {
        return false;
    }

    return mData->udbDelete(phrase, phraseLen);
}

bool chinese_controller::chinese_controller::udbGetNext(int index,
        unsigned short* phrase, int& phraseLen, int maxPhraseLen,
        unsigned short* spell, int& spellLen, int maxSpellLen)
{
    if (mData == 0) {
        return false;
    }

    return mData->udbGetNext(index, phrase, phraseLen, maxPhraseLen, spell, spellLen, maxSpellLen);
}

int  chinese_controller::udbCount()
{
    if (mData == 0) {
        return 0;
    }

    return mData->udbCount();
}

void chinese_controller::udbReset()
{
    if (mData == 0) {
        return;
    }

    mData->udbReset();
}

int chinese_controller::setCommonChar()
{
	if (mData == 0) {
		return -1;
	}

	return mData->setCommonChar();
}

int chinese_controller::clearCommonChar()
{
	if (mData == 0) {
		return -1;
	}

	return mData->clearCommonChar();
}

int chinese_controller::setFullSentence()
{
	if (mData == 0) {
		return -1;
	}

	return mData->setFullSentence();
}

int chinese_controller::clearFullSentence()
{
	if (mData == 0) {
		return -1;
	}

	return mData->clearFullSentence();
}

bool chinese_controller::isFullSentenceActive()
{
    if (mData == 0) {
        return -1;
    }

    return mData->isFullSentenceActive();
}

int chinese_controller::getHomophonePhraseCount(unsigned short *basePhrase, int basePhraseLen, int *count)
{
	if (mData == 0)
	{
		return -1;
	}

	return mData->getHomophonePhraseCount(basePhrase, basePhraseLen, count);
}

int chinese_controller::getHomophonePhrase(unsigned short *basePhrase, int basePhraseLen, int index, unsigned short *phrase, unsigned short *spell)
{
	if (mData == 0)
	{
		return -1;
	}

	return mData->getHomophonePhrase(basePhrase, basePhraseLen, index, phrase, spell);
}

/* Build. Set active spell to stem spell, or clear stem spell when applicable
 *
 * return: status int from data::buildWordList()
 * postconditions: active index has been set to match stem when possible. */
int chinese_controller::_build()
{
    int status;
    LOGV("chinese_controller::_build start...");
    LOGV("chinese_controller::_build mData->buildWordList() start 0x%X...", mData);
    status = mData->buildWordList();
    LOGV("chinese_controller::_build done...");
    return status;
}

/* Build. If there is a failure, clear and build again, then verify success.
 *
 * return: false if the first build failed, success otherwise.
 * postconditions: mLingInfo has built successfully and has phrases available. */
bool chinese_controller::_tryBuild()
{
    int status;
    LOGV("chinese_controller::_tryBuild start...");
    status = _build();
    if (ET9STATUS_NONE != status) {
        if (!(mFailedSelect && ET9STATUS_NO_MATCHING_WORDS == status)) {
            mData->clearKey();
            _verifyBuild();
            LOGV("chinese_controller::_tryBuild fail...");
            return false;
        }
    }
    else {
        mFailedSelect = false;
    }
    LOGV("chinese_controller::_tryBuild done...");
    return true;
}

/* Build and verify success. If there is a failure, clear all keys and rebuild.
 * preconditions: the success of buildWordList is expected.
 * postconditions: mLingInfo has built successfully and has phrases available. */
bool chinese_controller::_verifyBuild()
{
    int status;
    LOGV("chinese_controller::_verifyBuild start...");
    status = _build();

    if (ET9STATUS_NONE != status) {
        if (!(mFailedSelect && ET9STATUS_NO_MATCHING_WORDS == status)) {
            LOGE("chinese_controller::_verifyBuild failed with status %d\n", status);
            mData->clearAllKeys();
            status = _build();
            if (ET9STATUS_NONE != status) {
                LOGE("chinese_controller::_verifyBuild recover failed with status %d\n", status);
            }
            else {
                mFailedSelect = false;
            }
            LOGV("chinese_controller::_verifyBuild fail...");
            return false;
        }
    }
    else {
        mFailedSelect = false;
    }
    LOGE("chinese_controller::_verifyBuild done with status %d\n", status);
    return true;
}

} // namespace mocainput
