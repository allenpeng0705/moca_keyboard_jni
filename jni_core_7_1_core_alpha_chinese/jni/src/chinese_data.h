
#ifndef __chinese_data_h__
#define __chinese_data_h__

#include "et9cpapi.h"
#include "et9awapi.h"
#include "data.h"

namespace mocainput {
    enum {
        CHINESE_NAME_INPUT = 100,
        CHINESE_MOHU_PINYIN = 101,
    };
    enum {
        CHINESE_NAME_INPUT_OFF = 0,
        CHINESE_NAME_INPUT_ON = 1,
    };

    class chinese_data : public data {
    public:
		static int instanceCount;
		static chinese_data* singletonChineseDataInstance;
		static chinese_data* getInstance(DBRegistry* dbRegistry);
		static void deleteInstance();

	private:
        chinese_data(DBRegistry* dbRegistry);
        ~chinese_data();

	protected:
        // override methods
		bool create();
		void destroy();

	public:
        // override methods
        bool start();
        void finish();

		int initTrace(int kdbId, int width, int height);
  //      int isAutoSpaceBeforeTrace(int kdbId, ET9TracePoint* points, int numPoints);

        int composeKDBId(unsigned short keyboardLayoutId);

        int initUdb(int langId);

        int setLanguage(int langId);
        int getInputMode();
        int setInputMode(int mode);

        int setAttribute(int id, int value);

        int addDelimiter();
        int addTone(int tone, unsigned short * spell, int spellLen);

        int buildWordList();

        int getWord(unsigned short wordIndex, unsigned short* word, int& wordLen, int maxLen);
        unsigned int getToneOptions();
        bool haveBuild();

        int selectWord(int wordIndex);
        int unselectWord();

        int getSelection(unsigned short* selection, int& selectionLen, int maxLen);
        int commitSelection();

        int setContext(const unsigned short * context, int contextLen);
        int breakContext();

        int resetUserDictionary();
        int addWordToUserDictionary(const unsigned short* word, int wordLen,
                                        const unsigned short*spelling, int spellingLen);
        int getUserDictionaryWord(int index, unsigned short* word, int& wordLen, int maxLen);
        int deleteUserDictionaryWord(const unsigned short* word, int wordLen);

        int getSpell(unsigned short* spell, int& spellLen, int maxLen);

        int getPrefixCount();
        int getPrefix(int prefixIndex, unsigned short* prefix, int& prefixLen, int maxLen);
        int getActivePrefixIndex();
        int setActivePrefixIndex(int prefixIndex);
        int clearActivePrefixIndexIfNecessary();

        bool udbAdd(const unsigned short* phrase, int phraseLen, const unsigned short* spell, int spellLen);
        bool udbDelete(const unsigned short* phrase, int len);
        bool udbGetNext(int index, unsigned short* phrase, int& phraseLen, int maxPhraseLen,
                unsigned short* spell, int& spellLen, int maxSpellLen);
        int  udbCount();
        void udbReset();

        static int symbToTone(unsigned short);

        int setCommonChar();
        int clearCommonChar();

        int setFullSentence();
        int clearFullSentence();
        bool isFullSentenceActive();
        
        int getHomophonePhraseCount(unsigned short *basePhrase, int basePhraseLen, int *count);
        int getHomophonePhrase(unsigned short *basePhrase, int basePhraseLen, int index, unsigned short *phrase, unsigned short *spell);

    private:
        ET9CPLingInfo* mLingInfo;
        data::persistentDb*   mUdb;
#ifdef ET9_KDB_TRACE_MODULE
		ET9AWLingInfo *      sCTAWLingInfo;
		ET9AWLingCmnInfo *   sCTAWLingCmnInfo;
		ET9AWPrivWordInfo   pCTAWWordList[ET9MAXSELLISTSIZE];
#endif

    private:
        static ET9CPPhrase sPhrase;
        static ET9CPSpell sSpell;

        static ET9STATUS LdbReadCallback(
				ET9CPLingInfo *pLingInfo,
				ET9U8         **ppbSrc,
				ET9U32        *pdwSizeInBytes
        );
        static ET9STATUS AWLdbReadCallback(
				ET9AWLingInfo *pLingInfo,
				ET9U8         **ppbSrc,
				ET9U32        *pdwSizeInBytes
        );
    private:
        enum {
            MODE_PINYIN = 0,        /**< 0 -- PinYin mode */
            MODE_BPMF,              /**< 1 -- BoPoMoFo mode */
            MODE_STROKE,            /**< 2 -- Stroke mode */
            MODE_CANGJIE,           /**< 3 -- CangJie mode */
            MODE_QUICK_CANGJIE      /**< 4 -- Quick CangJie mode */
        };
    };
} // namespace mocainput

#endif // __chinese_data_h
