
#ifndef __chinese_controller_h__
#define __chinese_controller_h__

#include "controller.h"
#include "chinese_data.h"

// the number of spellings we will reorder based on stem holding
#define MAX_SPELL_REORDER_COUNT 16

namespace mocainput {

    class chinese_controller : public controller {
    public:
        chinese_controller(DBRegistry* dbRegistry);
        ~chinese_controller();

        bool create();
        void destroy();
        bool start();
        void finish();

        bool initTrace(int kdbId, int width, int height);
        int isAutoSpaceBeforeTrace(int kdbId, ET9TracePoint* points, int numPoints);

        bool setAttribute(int id, int value);

        bool setLanguage(int langId);

        int  getInputMode();
        bool setInputMode(int mode);

        bool processKey(int kdbId, int key);
        bool processTap(int kdbId, int TapX, int TapY, int shiftState);
        bool processTrace(int kdbId, ET9TracePoint* points, int numPoints, int shiftState);
        bool isHasTraceInfo();
        void backupWordSymbolInfo();
        void restoreWordSymbolInfo();

        bool clearKey();
        bool clearAllKeys();

        bool addDelimiter();
        bool addTone(int tone);
        bool cycleTone();

        int  getKeyCount();

        bool getWord(unsigned short wordIndex, unsigned short* word, int& wordLen, int maxLen);
        bool getSelection(unsigned short* str, int& strLen, int maxLen);

        bool selectWord(int index, unsigned short* insertText, int& insertTextLen, int maxLen);

        bool setContext(const unsigned short* newContext, int contextLen);
        bool breakContext();

        bool resetUserDictionary();
        bool addWordToUserDictionary(const unsigned short* word, int wordLen,
                                         const unsigned short* spelling, int spellingLen);
        bool getUserDictionaryWord(int index, unsigned short* word, int& wordLen, int maxLen);
        bool deleteUserDictionaryWord(const unsigned short* word, int wordLen);

        bool getSpell(unsigned short* spell, int& spellLen, int maxLen);

        int  getPrefixCount();
        bool getPrefix(int prefixIndex, unsigned short* prefix, int& prefixLen, int maxLen);
        int  getActivePrefixIndex();
        bool setActivePrefixIndex(int prefixIndex);

        bool udbAdd(const unsigned short* phrase, int phraseLen, const unsigned short* spell, int spellLen);
        bool udbDelete(const unsigned short* phrase, int len);
        bool udbGetNext(int index, unsigned short* phrase, int& phraseLen, int maxPhraseLen,
                unsigned short* spell, int& spellLen, int maxSpellLen);
        int  udbCount();
        void udbReset();

        int setCommonChar();
        int clearCommonChar();

        int setFullSentence();
        int clearFullSentence();
        bool isFullSentenceActive();

        int getHomophonePhraseCount(unsigned short *basePhrase, int basePhraseLen, int *count);
        int getHomophonePhrase(unsigned short *basePhrase, int basePhraseLen, int index, unsigned short *phrase, unsigned short *spell);

        int _build();

    private:
        chinese_data* mData;
        bool mFailedSelect;
        int mActivePrefixIndex;

        bool _tryBuild();
        bool _verifyBuild();
    };

} // namespace mocainput

#endif // __chinese_controller_h__
