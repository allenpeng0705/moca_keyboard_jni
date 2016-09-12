
#ifndef __alpha_controller_h__
#define __alpha_controller_h__

#include "alpha_data.h"
#include "controller.h"

namespace mocainput {

    // DO NOT change these ENUM unless you have to make the same changes in
    // Attribute.java file.

    enum {
        AUTO_CORRECTION_MODE = 100,
        WORD_COMPLETION_POINT = 101,
        BIGRAM_LANG_MODEL     = 102,
        BOOST_CANDIDATE       = 103,
        AUTO_SPACE			  = 104,
        TRACE_FILTER		  = 105,
    };

    enum {
    	TRACE_FILTER_LOW = 0,
    	TRACE_FILTER_HIGH = 1,
    };

    enum {
        AUTO_CORRECTION_MODE_OFF = 0,
        AUTO_CORRECTION_MODE_LOW = 1,
        AUTO_CORRECTION_MODE_HIGH = 2
    };
    enum {
        WORD_COMPLETION_POINT_OFF = 0,
        WORD_COMPLETION_POINT_AFTER_ONE_KEY = 1,
        WORD_COMPLETION_POINT_AFTER_TWO_KEYS = 2,
        WORD_COMPLETION_POINT_AFTER_THREE_KEYS = 3,
        WORD_COMPLETION_POINT_AFTER_FOUR_KEYS = 4,
        WORD_COMPLETION_POINT_AFTER_FIVE_KEYS = 5,
        WORD_COMPLETION_POINT_AFTER_SIX_KEYS = 6,
    };

    class alpha_controller : public controller {
    public:
        alpha_controller(DBRegistry* dbRegistry);
        ~alpha_controller();

        bool create();
        void destroy();
        bool start();
        void finish();

        bool initTrace(int kdbId, int width, int height);
        bool getKeyPositions(int kdbId, ET9KeyPoint* points, unsigned int maxPoints, unsigned int& numPoints);
        int isAutoSpaceBeforeTrace(int kdbId, ET9TracePoint* points, int numPoints);
        bool processTap(int kdbId, int TapX, int TapY, int shiftState);
        bool processTrace(int kdbId, ET9TracePoint* points, int numPoints, int shiftState);
        bool processKey(int kdbId, int key, int shiftState);
        bool addExplicit(const unsigned short* buffer, int len, int shiftState);
        int  getKeyCount();
        bool clearKey();
        bool clearAllKeys();
        int  buildWordList();
        int  getDefaultWordIndex();
        bool getWord(int wordIndex, unsigned short* word, unsigned short* sub, int& wordLen, int& wordCompLen, int& subLen, int maxLen);
        void lockWord(int wordIndex);
        bool setLanguage(int langId);
        void wordSelected(int wordIndex);
        bool addWordToUserDictionary(const unsigned short* word, int wordLen);
        void breakContext();
        void setContext(const unsigned short* newContext, int contextLen);
        bool setAttribute(int id, int value);
        bool ReCaptureWord(int kdbId, unsigned short * const word, int wordLen);
        int  getExactType(unsigned short* word, int maxLen);
        int  getInlineText(unsigned short* word, int maxLen);
        bool isInlineKnown();
        bool isUpperSymbol(const unsigned short symbol);
        bool isLowerSymbol(const unsigned short symbol);
        void toLowerSymbol(const unsigned short symbol, unsigned short * const lowerSymbol);
        void toUpperSymbol(const unsigned short symbol, unsigned short * const upperSymbol);

        // UDB and ASDB
        bool udb_add(const unsigned short* buffer, int len);
        bool udb_delete(const unsigned short* buffer, int len);
        bool udb_find(const unsigned short* buffer, int len);
        bool udb_getNext(unsigned short* word, int& wordLen, int maxLen);
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

    private:
        bool setWordCompletionPoint(int value);
        bool setAutoCorrectionMode(int value);
        bool isReducedQwertyLayout(int kdbId);
        bool isQwertyLayout(int kdbId);
        bool isPhoneKeypad(int kdbId);
        void setExactInlistMode(int kdbId);

    private:
        alpha_data* mData;
        int mDefaultWordIndex;
        int mCurrentCorrectionMode;
    };

} // namespace mocainput

#endif // __alpha_controller_h__
