
#ifndef __alpha_data_h__
#define __alpha_data_h__

#include "data.h"

namespace mocainput {

	class alpha_data : public data {

	public:
		static int instanceCount;
		static alpha_data* singletonAlphaDataInstance;
		static alpha_data* getInstance(DBRegistry* dbRegistry);
		static void deleteInstance();

	private:
		alpha_data(DBRegistry* dbRegistry);
		~alpha_data();

	protected:
		bool create();
		void destroy();

	public:

		// override methods
		bool start();
		void finish();

		int composeKDBId(unsigned short keyboardLayoutId);

        void getCoreSettings(void);
        int initTrace(int kdbId, int width, int height);
        int isAutoSpaceBeforeTrace(int kdbId, ET9TracePoint* points, int numPoints);
		int  processTap(int kdbId, int TapX, int TapY, int shiftState);
        int  processTrace(int kdbId, ET9TracePoint* points, int numPoints, int shiftState);
		int  processKey(int kdbId, int shiftState, int key);
		int  addExplicit(const unsigned short* word, int len, int shiftState);
		int  getKeyCount();
		int  clearKey();
		int  clearAllKeys();
		int  buildWordList(int& defaultWordIndex);
		bool getWord(int wordIndex, unsigned short* word, unsigned short* sub, int& wordLen, int& wordCompLen, int& subLen, int maxLen);
		ET9AWWordInfo* getWord(int wordIndex);
		void lockWord(int wordIndex);
		bool setLanguage(int langId);
		void wordSelected(int wordIndex);
		bool addWordToUserDictionary(const unsigned short* word, int wordLen);
		bool ReCaptureWord(int kdbId, unsigned short * const word, int wordLen);
		void doPostShift(int& wordIndex);
		void breakContext();
		void setContext(const unsigned short* newContext, int contextLen);
		int  addCustomSymbolSet(unsigned short* const symbols, int * const prob, const int symbolCount, int shiftState, int wordindex);
		int isUpperSymbol(const unsigned short symbol);
		int isLowerSymbol(const unsigned short symbol);
		unsigned short toLowerSymbol(const unsigned short symbol);
		unsigned short toUpperSymbol(const unsigned short symbol);

		void setWordCompletion(bool set);
		void setWordCompletionPoint(int value);
		int  getWordCompletionPoint();
		void setSpellCorrectionMode(ET9ASPCMODE mode);
		void setAutoAppend(bool set);
		int  getKdbId(bool primaryKeyboardId = true);
		int  getLanguageId(bool primaryLanguage = true);
		void setExactInlist(ET9AEXACTINLIST exact);
		void setSelectionListMode(ET9ASLCORRECTIONMODE mode);
		ET9ASLCORRECTIONMODE getSelectionListMode();

		void setRegionalCorrection(bool set);
		void changeDefaultWordIndex(int index);
		bool isAlpha(unsigned short ch);
		int  getExactType(unsigned short* word, int maxLen);
		int  getInlineText(unsigned short* word, int maxLen);
		bool isInlineKnown();
		void setDBStems(bool set);
		bool getDBStems();
		void setWordStemsPoint(int value);
		int  getWordStemsPoint();

        bool udb_add(const unsigned short* buffer, int len);
        bool udb_delete(const unsigned short* buffer, int len);
        bool udb_find(const unsigned short* buffer, int len);
        bool udb_getNext(unsigned short* word, int& wordLen, int maxWordLen);
        int  udb_getSize();
        int  udb_getRemainingMemory();
        bool udb_scanBuf(const unsigned short* buffer, int len);
        int  udb_count();
        void udb_reset();

        bool asdb_add(const unsigned short* word, int wordLen, const unsigned short* subs, int subsLen);
        bool asdb_delete(const unsigned short* buffer, int len);
        bool asdb_find(const unsigned short* word, int wordLen, const unsigned short* subs, int subsLen);
        bool asdb_getNext(unsigned short* word, int& wordLen,
                unsigned short* subs, int& subsLen, int maxSubsLen);
        int  asdb_count();
        void asdb_reset();

        bool setBigramLangModel(bool bOn);
        bool setBoostingCandidate(bool bOn);
        bool setAutoSpacing(bool bOn);
        bool setTraceFilter(ET9ASPCTRACESEARCHFILTER eFilter);

	public:
		const data::persistentDb* const getPersistentUDB() {return mUdb;}

	public:
	    void registerMdb(ET9MDBCALLBACK callback);

	private:
		ET9AWLingInfo* mLingInfo;
		ET9AWLingCmnInfo* mLingCmnInfo;
		ET9AWPrivWordInfo* mPrivateWordInfo;

	private:
		void getSecondaryLDBID(ET9U16 *pwLdbId);
		void initDbs();
        void initAsdb();
        void initUdb();
        void initCdb();
        void initMdb();
        void flushDbs();
        void flushUdb();
        void flushAsdb();
        void flushCdb();

	private:

	    // MDB is not a persistence
	    class mdb {
        public:
            mdb(alpha_data* _alpha_data);
            ~mdb();
            void loadFile(char* filename);
            void deInit();

        public:
            ET9STATUS read(
                    ET9REQMDB eRequestType,
                    ET9U16 wActiveWordLen,
                    ET9U16 wMaxWordLen,
                    ET9SYMB* psBuffer,
                    ET9U16*  pwBufferLen,
                    ET9U32*  pdwWordIndex);

            static ET9STATUS callback(
                    ET9AWLingInfo* lingInfo,
                    ET9REQMDB eRequestType,
                    ET9U16 wActiveWordLen,
                    ET9U16 wMaxWordLen,
                    ET9SYMB* psBuffer,
                    ET9U16* pwActiveWordLen,
                    ET9U32* pdwWordIndex);

        private:
            char*       mBuffer;  // [word_count][len1:word1][len2:word2]...[lenN:wordN]
            char*       mCurrent;
            alpha_data* mAlpha_data;

        private:
            static const int WORD_LEN_SIZE_IN_BYTES = sizeof(unsigned short);
            static const int WORD_COUNT_SIZE_IN_BYTES = sizeof(unsigned int);
	    };

	private:
	    data::persistentDb*  mAsdb;
	    data::persistentDb*  mUdb;
	    data::persistentDb*  mCdb;
        mdb*                 mMdb;

	private:
		static ET9STATUS LdbReadCallback(
							ET9AWLingInfo       *pLingInfo,
							ET9U8 * ET9FARDATA 	*ppbSrc,
							ET9U32				*pdwSizeInBytes
		);
	};

} // namespace mocainput

#endif // __alpha_data_h
