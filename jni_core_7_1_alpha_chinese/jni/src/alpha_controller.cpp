
#include "Log.h"

#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>

#include "alpha_controller.h"

namespace mocainput {

alpha_controller::alpha_controller(DBRegistry* dbRegistry) : controller(dbRegistry)
{
	LOGV("alpha_controller::alpha_controller start DBRegistry:0x%X", dbRegistry);
    mData = 0;
    mDefaultWordIndex = 0;
    mCurrentCorrectionMode = AUTO_CORRECTION_MODE_HIGH;
	LOGV("alpha_controller::alpha_controller done DBRegistry:0x%X", dbRegistry);
}

alpha_controller::~alpha_controller()
{
}

bool alpha_controller::create()
{
	LOGV("alpha_controller::create start");
	mData = alpha_data::getInstance(mDbRegistry);
	LOGV("alpha_controller::create done mData:0x%X", mData);
	return mData != 0;
}

void alpha_controller::destroy()
{
	alpha_data::deleteInstance();

	mData = 0;
	mDefaultWordIndex = 0;
}

bool alpha_controller::start()
{
	LOGV("alpha_controller::start start");
	if (mData == 0) {
		LOGV("alpha_controller::start fail mData==0");
		return false;
	}
	LOGV("alpha_controller::start mData:0x%X", mData);
	bool ret =  mData->start();
	LOGV("alpha_controller::start done");
	return ret;
}

void alpha_controller::finish()
{
	LOGV("alpha_controller::finish start");
	if (mData) {
		LOGV("alpha_controller::finish mData:0x%X", mData);
		mData->finish();
		LOGV("alpha_controller::finish done");
	} else {
		LOGV("alpha_controller::finish faile mData == 0");
	}

}

bool alpha_controller::initTrace(int kdbId, int width, int height)
{
	LOGV("alpha_controller::initTrace start kdbId:%d", kdbId);
	if (mData == 0) {
		LOGV("alpha_controller::initTrace fail mData == 0");
		return false;
	}

	LOGV("alpha_controller::initTrace mData->getKdbId kdbId:%d, mData:0x%X", kdbId, mData);
	if (kdbId != mData->getKdbId()) {
	    setExactInlistMode(kdbId);
	}

	LOGV("alpha_controller::initTrace mData->initTrace kdbId:%d, mData:0x%X", kdbId, mData);
	return mData->initTrace(kdbId, width, height) == 0;
}

bool alpha_controller::getKeyPositions(int kdbId, ET9KeyPoint* points, unsigned int maxPoints, unsigned int& numPoints)
{
	if(!this->mData)
	{
		return false;
	}

	if (kdbId != mData->getKdbId()) {
	    setExactInlistMode(kdbId);
	}

	return this->mData->getKeyPositions(kdbId, points, maxPoints, numPoints);
}

int alpha_controller::isAutoSpaceBeforeTrace(int kdbId, ET9TracePoint* points, int numPoints)
{
	if(!this->mData)
	{
		return false;
	}

	if (kdbId != mData->getKdbId()) {
	    setExactInlistMode(kdbId);
	}

	return this->mData->isAutoSpaceBeforeTrace(kdbId, points, numPoints);
}

bool alpha_controller::processTap(int kdbId, int TapX, int TapY, int shiftState)
{
	if (mData == 0) {
		return false;
	}

	if (kdbId != mData->getKdbId()) {
	    setExactInlistMode(kdbId);
	}

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
	    LOGE("alpha_controller::processTap() failed - 0x%X is not a valid keyboard layout", kdbId);
	    break;
	}

	return false;
}

bool alpha_controller::processTrace(int kdbId, ET9TracePoint* points, int numPoints, int shiftState)
{
	if(!this->mData)
	{
		return false;
	}

	if (kdbId != mData->getKdbId()) {
	    setExactInlistMode(kdbId);
	}

	return this->mData->processTrace(kdbId, points, numPoints, shiftState);
}

void alpha_controller::setExactInlistMode(int kdbId)
{
	if (KEYBOARD_LAYOUT_NONE != kdbId) {
		if (isReducedQwertyLayout(kdbId) || isPhoneKeypad(kdbId)) {
			mData->setExactInlist(ET9AEXACTINLIST_LAST);
		}
		else {
			mData->setExactInlist(ET9AEXACTINLIST_FIRST);
		}
	}
}

bool alpha_controller::processKey(int kdbId, int key, int shiftState)
{
    bool ret = false;

	if (mData == 0) {
		return ret;
	}

	if (kdbId != mData->getKdbId()) {
	    setExactInlistMode(kdbId);
	}

	if (KEYBOARD_LAYOUT_NONE == kdbId) {
	    ret = addExplicit((unsigned short*)&key, 1, shiftState);
	}
	else {
	    ret = mData->processKey(kdbId, key, shiftState) == ET9STATUS_NONE;
	}

	return ret;
}

bool alpha_controller::addExplicit(const unsigned short* buffer, int len, int shiftState)
{
	if (mData == 0) {
		return false;
	}

	return mData->addExplicit(buffer, len, shiftState) == ET9STATUS_NONE;
}

int alpha_controller::getKeyCount()
{
    if (mData == 0) {
        return 0;
    }

    return mData->getKeyCount();
}

bool alpha_controller::clearKey()
{
	LOGV("alpha_controller::clearKey start mData:0x%X", mData);
	if (mData == 0) {
		LOGV("alpha_controller::clearKey fail mData== 0");
		return false;
	}
	LOGV("alpha_controller::clearKey mData->clearKey mData:0x%X", mData);
	return mData->clearKey() == ET9STATUS_NONE;
}

bool alpha_controller::clearAllKeys()
{
	LOGV("alpha_controller::clearAllKeys start mData:0x%X", mData);
	if (mData == 0) {
		LOGV("alpha_controller::clearAllKeys fail mData== 0");
		return false;
	}
	LOGV("alpha_controller::clearAllKeys mData->clearAllKeys mData:0x%X", mData);
	return mData->clearAllKeys() == ET9STATUS_NONE;
}

bool alpha_controller::isReducedQwertyLayout(int kdbId)
{
    int layout = (kdbId & KEYBOARD_LAYOUT_MASK);

    return (layout == KEYBOARD_LAYOUT_REDUCED_QWERTY ||
            layout == KEYBOARD_LAYOUT_REDUCED_AZERTY ||
            layout == KEYBOARD_LAYOUT_REDUCED_QWERTZ);
}

bool alpha_controller::isQwertyLayout(int kdbId)
{
    int layout = (kdbId & KEYBOARD_LAYOUT_MASK);

    return (layout == KEYBOARD_LAYOUT_QWERTY ||
            layout == KEYBOARD_LAYOUT_AZERTY ||
            layout == KEYBOARD_LAYOUT_QWERTZ);
}

bool alpha_controller::isPhoneKeypad(int kdbId)
{
    int layout = (kdbId & KEYBOARD_LAYOUT_MASK);
    return layout == KEYBOARD_LAYOUT_PHONE_KEYPAD;
}

int alpha_controller::buildWordList()
{
    int wordCount = 0;
    unsigned short word[2];

    if (mData == 0) {
		return wordCount;
	}

    if (isPhoneKeypad(mData->getKdbId()) && getKeyCount() < mData->getWordCompletionPoint()) {
		mData->setDBStems(true);
		mData->setWordStemsPoint(2);
	}
	else {
		mData->setDBStems(false);
		mData->setWordStemsPoint(0);
	}

	wordCount = mData->buildWordList(mDefaultWordIndex);

	// Will always set the exact char as a default for single letter in reduced QWERTY layouts or
	// auto correction is off, which may have only word completion
	if (wordCount && mDefaultWordIndex != 0 && isReducedQwertyLayout(mData->getKdbId()) &&
	        mData->getExactWord(word, 2) == 1 && mData->isAlpha(word[0]) ||
	        mCurrentCorrectionMode == AUTO_CORRECTION_MODE_OFF) {

		unsigned short inlineWord[ET9MAXWORDSIZE] = {0, };
		unsigned short defaultWord[ET9MAXWORDSIZE] = {0, };
		unsigned short ignoreWord[ET9MAXWORDSIZE] = {0, };
		int defaultWordLen = 0;
		int ignoreWordLen = 0;

		int inlineWordLen = this->getInlineText(inlineWord, ET9MAXWORDSIZE + 1);
		this->getWord(mDefaultWordIndex, defaultWord, ignoreWord, defaultWordLen, ignoreWordLen, ignoreWordLen, ET9MAXWORDSIZE);

		// only change the default candidate when the inline word doesn't match the default word
		// this prevents changing of the default candidate durring recapture
		if((defaultWordLen != inlineWordLen) || (data::wordCmp(inlineWord, defaultWord, defaultWordLen)))
		{
			mDefaultWordIndex = 0;
			mData->changeDefaultWordIndex(mDefaultWordIndex);
		}
	}

	return wordCount;
}

int alpha_controller::getDefaultWordIndex()
{
	if (mData == 0) {
		return 0;
	}

	return mDefaultWordIndex;
}

bool alpha_controller::getWord(int wordIndex, unsigned short* word, unsigned short* sub, int& wordLen, int& wordCompLen, int& subLen, int maxLen)
{
    bool ret = false;

	if (mData == 0) {
		return ret;
	}

	ret = mData->getWord(wordIndex, word, sub, wordLen, wordCompLen, subLen, maxLen);

	return ret;
}

void alpha_controller::lockWord(int wordIndex)
{
	if (mData) {
		mData->lockWord(wordIndex);
	}
}

bool alpha_controller::setLanguage(int langId)
{
	if (mData == 0) {
		return false;
	}

	return mData->setLanguage(langId);
}

void alpha_controller::wordSelected(int wordIndex)
{
	if (mData) {
		mData->wordSelected(wordIndex);
	}
}

bool alpha_controller::addWordToUserDictionary(const unsigned short* word, int wordLen)
{
	if (mData == 0) {
		return false;
	}

	return mData->addWordToUserDictionary(word, wordLen);
}

void alpha_controller::breakContext()
{
	if (mData == 0) {
		return;
	}

	mData->breakContext();
}

void alpha_controller::setContext(const unsigned short* newContext, int contextLen)
{
	if (mData != 0) {
		mData->setContext(newContext, contextLen);
	}
}

bool alpha_controller::setAttribute(int id, int value)
{
    bool ret = false;

    if (mData == 0) {
        return ret;
    }

    switch (id) {

    case AUTO_CORRECTION_MODE:
        ret = setAutoCorrectionMode(value);
        break;

    case WORD_COMPLETION_POINT:
        ret = setWordCompletionPoint(value);
        break;

    case BIGRAM_LANG_MODEL:
        ret = mData->setBigramLangModel(value != 0);
        break;

    case BOOST_CANDIDATE:
        ret = mData->setBoostingCandidate(value != 0);
        break;

    case AUTO_SPACE:
    	ret = mData->setAutoSpacing(value != 0);
    	break;

    case TRACE_FILTER:

    	switch (value) {

    	case TRACE_FILTER_LOW:
    		ret = mData->setTraceFilter(ET9ASPCTRACESEARCHFILTER_ONE_EXACT);
    		break;

    	case TRACE_FILTER_HIGH:
    		ret = mData->setTraceFilter(ET9ASPCTRACESEARCHFILTER_ONE_REGIONAL);
    		break;

    	default:
    		LOGE("%d is not a valid trace filter value, default to %d", value, TRACE_FILTER_HIGH);
    		ret = mData->setTraceFilter(ET9ASPCTRACESEARCHFILTER_ONE_REGIONAL);
    	}

    	break;

    default:
        break;
    }

    return ret;
}

bool alpha_controller::setWordCompletionPoint(int value)
{
    if (value < WORD_COMPLETION_POINT_OFF || value > WORD_COMPLETION_POINT_AFTER_SIX_KEYS) {
        LOGE("alpha_controller::setWordCompletionPoint() %d is not a valid value", value);
        return false;
    }

    mData->setWordCompletionPoint(value);
    mData->setWordCompletion(value != WORD_COMPLETION_POINT_OFF);

    return true;
}

bool alpha_controller::setAutoCorrectionMode(int value)
{
    if (value < AUTO_CORRECTION_MODE_OFF || value > AUTO_CORRECTION_MODE_HIGH) {
        LOGE("alpha_controller::setAutoCorrectionMode() %d is not a valid value", value);
        return false;
    }

    mCurrentCorrectionMode = value;

    mData->setAutoAppend(false);
    setExactInlistMode(mData->getKdbId());

    switch (value) {

    case AUTO_CORRECTION_MODE_OFF:
        // turn off both spell and regional correction and auto-append
        mData->setSpellCorrectionMode(ET9ASPCMODE_OFF);
        mData->setRegionalCorrection(false);
        mData->setSelectionListMode(ET9ASLCORRECTIONMODE_LOW);
        break;

    case AUTO_CORRECTION_MODE_LOW:
        // only spell correction is turn on, no regional correction nor auto-append
        mData->setSpellCorrectionMode(ET9ASPCMODE_EXACT);
        mData->setRegionalCorrection(true);
        mData->setSelectionListMode(ET9ASLCORRECTIONMODE_LOW);
        break;

    case AUTO_CORRECTION_MODE_HIGH:
        // spell and regional correction and/or auto-append is turn off and set select list
        // mode to classic.
        mData->setSpellCorrectionMode(ET9ASPCMODE_REGIONAL);
        mData->setRegionalCorrection(true);
        mData->setSelectionListMode(ET9ASLCORRECTIONMODE_HIGH);
        break;

    default:
        break;
    }

    return true;
}

bool alpha_controller::ReCaptureWord(int kdbId, unsigned short * const word, int wordLen)
{
	if (mData == 0) {
		return false;
	}

	if (kdbId != mData->getKdbId()) {
		setExactInlistMode(kdbId);
	}
	return mData->ReCaptureWord(kdbId, word, wordLen);
}

bool alpha_controller::isUpperSymbol(const unsigned short symbol)
{
	if (mData == 0 || mData->isUpperSymbol(symbol) == 0) {
		return false;
	}

	return true;
}

bool alpha_controller::isLowerSymbol(const unsigned short symbol)
{
	if (mData == 0 || mData->isLowerSymbol(symbol) == 0) {
		return false;
	}

	return true;
}

void alpha_controller::toLowerSymbol(const unsigned short symbol, unsigned short * const lowerSymbol)
{
	if (mData) {
		*lowerSymbol = mData->toLowerSymbol(symbol);
	}
}

void alpha_controller::toUpperSymbol(const unsigned short symbol, unsigned short * const upperSymbol)
{
	if (mData) {
		*upperSymbol = mData->toUpperSymbol(symbol);
	}
}

int alpha_controller::getExactType(unsigned short* word, int maxLen)
{
    if (mData == 0) {
        return 0;
    }

    return mData->getExactType(word, maxLen);
}

int alpha_controller::getInlineText(unsigned short* word, int maxLen)
{
    if (mData == 0) {
        return 0;
    }

    return mData->getInlineText(word, maxLen);
}

bool alpha_controller::isInlineKnown()
{
    if (mData == 0) {
        return 0;
    }

    return mData->isInlineKnown();
}

bool alpha_controller::udb_add(const unsigned short* buffer, int len)
{
    if (mData == 0) {
        return false;
    }

    return mData->udb_add(buffer, len);
}

bool alpha_controller::udb_delete(const unsigned short* buffer, int len)
{
    if (mData == 0) {
        return false;
    }

    return mData->udb_delete(buffer, len);
}

bool alpha_controller::udb_find(const unsigned short* buffer, int len)
{
    if (mData == 0) {
        return false;
    }

    return mData->udb_find(buffer, len);
}

bool alpha_controller::udb_getNext(unsigned short* word, int& wordLen, int maxLen)
{
    if (mData == 0) {
        return false;
    }

    return mData->udb_getNext(word, wordLen, maxLen);
}

int alpha_controller::udb_getSize()
{
    if (mData == 0) {
        return 0;
    }

    return mData->udb_getSize();
}

int alpha_controller::udb_getRemainingMemory()
{
    if (mData == 0) {
        return 0;
    }

    return mData->udb_getRemainingMemory();
}

bool alpha_controller::udb_scanBuf(const unsigned short* buffer, int len)
{
    if (mData == 0) {
        return false;
    }

    return mData->udb_scanBuf(buffer, len);
}


int alpha_controller::udb_count()
{
    if (mData == 0) {
        return 0;
    }

    return mData->udb_count();
}

void alpha_controller::udb_reset()
{
    if (mData == 0) {
    }

    mData->udb_reset();
}

bool alpha_controller::asdb_add(const unsigned short* word, int wordLen, const unsigned short* subs, int subsLen)
{
    if (mData == 0) {
        return false;
    }

    return mData->asdb_add(word, wordLen, subs, subsLen);
}

bool alpha_controller::asdb_delete(const unsigned short* buffer, int len)
{
    if (mData == 0) {
        return false;
    }

    return mData->asdb_delete(buffer, len);
}

bool alpha_controller::asdb_find(const unsigned short* word, int wordLen, const unsigned short* subs, int subsLen)
{
    if (mData == 0) {
        return false;
    }

    return mData->asdb_find(word, wordLen, subs, subsLen);
}

bool alpha_controller::asdb_getNext(unsigned short* word, int& wordLen,
        unsigned short* subs, int& subsLen, int maxLen)
{
    if (mData == 0) {
        return false;
    }

    return mData->asdb_getNext(word, wordLen, subs, subsLen, maxLen);
}

int alpha_controller::asdb_count()
{
    if (mData == 0) {
        return 0;
    }

    return mData->asdb_count();
}

void alpha_controller::asdb_reset()
{
    if (mData == 0) {
        return;
    }

    mData->asdb_reset();
}

} // namespace mocainput
