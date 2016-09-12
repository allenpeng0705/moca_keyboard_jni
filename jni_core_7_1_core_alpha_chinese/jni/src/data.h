
#ifndef __data_h__
#define __data_h__

#include <stdlib.h>
#include <stdio.h>
#include "et9api.h"

#include "dbregistry.h"
#include "user_study_log.h"
#include "mem_alloc.h"

#define DEFAULT_KDB_NUM 0x09FF

#define MIN(a,b) ((a) < (b) ? (a) : (b))

namespace mocainput {

	enum {
	    KEYBOARD_LAYOUT_QWERTY = 0x0900,
	    KEYBOARD_LAYOUT_AZERTY = 0x0800,
	    KEYBOARD_LAYOUT_QWERTZ = 0x0700,
	    KEYBOARD_LAYOUT_PHONE_KEYPAD = 0x0600,
	    KEYBOARD_LAYOUT_REDUCED_QWERTY = 0x0A00,
	    KEYBOARD_LAYOUT_REDUCED_AZERTY = 0x0B00,
	    KEYBOARD_LAYOUT_REDUCED_QWERTZ = 0x0C00,

	    KEYBOARD_LAYOUT_NONE = 0xFFFF,
	    KEYBOARD_LAYOUT_MASK = 0xFF00,
	};

	class data {

	public:

	    // a simple class to create/open existing file into me
	    class persistentDb {
        public:
            persistentDb(char* dbFile);
            ~persistentDb();

            inline char* buffer() const {return mDbBuffer;}
            inline int bufferSize() const {return mDbBufferSize;}

            void create(int bufferSize);
            void close();
            void flush();

        protected:
            int     mDbBufferSize;
            char*   mDbBuffer;
            char*   mDbFile;
            FILE*   mFile;
        };

	    class xldb {

	    public:
	    	static const int MAX_PATH = 260;
	    	int mLdbId;
	    	char mFileName[MAX_PATH];
	    	char *mLdbData;
	    	int mLdbDataSize;
	    public:
	    	xldb();
	    	~xldb();
	    };

	public:
        static void wordCopy(unsigned short* dest, const unsigned short* src, int len) {
            // assuming max buffer length is enforced by the caller
            while (len--) {
                *dest++ = *src++;
            }
        }
        static int wordCmp(const unsigned short* a, const unsigned short* b, int len) {
            // assuming max buffer length is enforced by the caller
            for (int i = 0; i < len; i++) {
                if (a[i] < b[i]) {
                    return -1;
                }
                else if (a[i] > b[i]) {
                    return 1;
                }
            }
            return 0;
        }

	protected:

		data(DBRegistry* dbRegistry);
		virtual ~data();

		virtual bool create() = 0;
		virtual void destroy() = 0;

	public:
		virtual bool start() = 0;
		virtual void finish() = 0;

		virtual int composeKDBId(unsigned short keyboardLayoutId) = 0;

	public:
        bool readLdb(int kdbId, char** src, int* sizeInBytes);
        bool readKdb(int kdbId, char** src, int* sizeInBytes);
        bool readKdbText(int kdbId, int PageNum);
        bool readKdbXml(int kdbId, int PageNum);

	public:
        int initTrace(int kdbId, int width, int height);
        bool getKeyPositions(int kdbId, ET9KeyPoint* points, unsigned int maxPoints, unsigned int& numPoints);
        int isAutoSpaceBeforeTrace(int kdbId, ET9TracePoint* points, int numPoints);
        int processTrace(int kdbId, ET9TracePoint* points, int numPoints, int shiftState);
        bool isHasTraceInfo();
        void backupWordSymbolInfo();
        void restoreWordSymbolInfo();
		int processTap(int kdbId, int TapX, int TapY, int shiftState = ET9NOSHIFT);
		int processKey(int kdbId, int key, int shiftState = ET9NOSHIFT);
        int addExplicitKey(const unsigned short key, int shiftState = ET9NOSHIFT);
		int addExplicitString(const unsigned short* word, int len, int shiftState = ET9NOSHIFT);
		int clearKey();
		int clearAllKeys();
		int addCustomSymbolSet(unsigned short* const symbols, int * const prob, const int symbolCount, int shiftState, int wordindex);
        int getKeyCount();
        int getExactWord(unsigned short* buffer, int maxBufferLen);
        int ReCaptureWord(int kdbId, unsigned short * const word, int wordLen);
        int getKdbId();
        int isUpperSymbol(const unsigned short symbol);
        int isLowerSymbol(const unsigned short symbol);
        unsigned short toLowerSymbol(const unsigned short symbol);
        unsigned short toUpperSymbol(const unsigned short symbol);

	protected:
		static bool loadFile(const char* path, char*& data, int& size);
        bool loadLdb(int ldbId);

        xldb* getXLdb(int ldbId);
        bool loadXLdb(int ldbId, xldb *pXLdb);
        static bool loadXLdbFile(const char *path, xldb *pXLdb);

	protected:
        const DBRegistry* getDbRegistry() {return mDBRegistry;}

	private:
		bool loadKdb(int kdbId);
		void setShiftState(int shift);
		int  getShiftState();
		bool isChinesePhonepadKdb(int kdbId);

	protected:
		// common for both alpha and chinese, these are static pointers
		ET9KDBInfo* mKdbInfo;
		ET9WordSymbInfo* mWordSymbInfo;
#ifdef ET9_KDB_TRACE_MODULE
		ET9WordSymbInfo  sWordSymbInfoBackup;
#endif

	protected:
	    DBRegistry* mDBRegistry;
		int   mCurrentKdbId;
		char* mKdbData;
		int   mKdbSize;

        int   mCurrentLdbId;
        char* mLdbData;
        int   mLdbSize;
        static const int XLDBMAXCOUNT = 16;
        xldb  *mXLdbs[XLDBMAXCOUNT];
    #if USER_STUDY_LOG
        UserStudyLog * m_pUserStudyLog;
        int mDefaultWordIndex;
        int mDelKeyCount;
    #endif

	private:
	  static ET9STATUS KdbReadText(
              ET9KDBInfo       *pKDBInfo,
              ET9U16            wKdbNum,
              ET9U16            wPageNum);
      static ET9STATUS KdbReadXml(
              ET9KDBInfo       *pKDBInfo,
              ET9U16            wKdbNum,
              ET9U16            wPageNum);

		static ET9STATUS KdbReadCallback(
							ET9KDBInfo			*pKDBInfo,
							ET9U8 * ET9FARDATA 	*ppbSrc,
							ET9U32				*pdwSizeInBytes
		);
	};

	class wordSymbInfo {
	public:
		static ET9WordSymbInfo* create()
		{
			if (sWordSymbInfo == 0) {
				sWordSymbInfo = (ET9WordSymbInfo*)CALLOC(sizeof(*sWordSymbInfo), 1);
			}

			if (sWordSymbInfo) {
				++sRefCount;
			}

			return sWordSymbInfo;
		}

		static void destroy()
		{
			if (sWordSymbInfo && --sRefCount == 0) {
				FREE(sWordSymbInfo);
			}
		}

	private:
		wordSymbInfo(); // not allow
		wordSymbInfo(const wordSymbInfo&); // now allow

		// one and only one ET9WordSymbInfo and and is shared between alpha & chinese
		static ET9WordSymbInfo* sWordSymbInfo;
		static int sRefCount;
	};

	// TODO; implement 'creator' template
	class kdbInfo {
	public:
		static ET9KDBInfo* create()
		{
			if (sKdbInfo == 0) {
				sKdbInfo = (ET9KDBInfo*)CALLOC(sizeof(*sKdbInfo), 1);
			}

			if (sKdbInfo) {
				++sRefCount;
			}

			return sKdbInfo;
		}

		static void destroy()
		{
			if (sKdbInfo && --sRefCount == 0) {
				FREE(sKdbInfo);
			}
		}

	private:
		kdbInfo(); // not allow
		kdbInfo(const kdbInfo&); // now allow

		// one and only one ET9KDBInfo and and is shared between alpha & chinese
		static ET9KDBInfo* sKdbInfo;
		static int sRefCount;
	};

}

#endif // __data_h__
