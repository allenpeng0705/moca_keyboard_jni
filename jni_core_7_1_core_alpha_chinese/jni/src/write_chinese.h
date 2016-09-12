
#ifndef __WRITE_CHINESE_H__
#define __WRITE_CHINESE_H__

#include "../core/t9write_chinese/api/decuma_hwr_cjk.h"
#include "../core/t9write_chinese/api/decumaLanguages.h"
#include "../core/t9write_chinese/api/decumaSymbolCategories.h"

#include "dbregistry.h"
#include "chinese_data.h"

namespace mocainput {

class Write_Chinese {
public:
    static const int MAX_RECOGNIZE_CANDIDATES = 20;
    static const int MAX_CHARACTERS = 2;

public:
    Write_Chinese(DBRegistry* dbRegistry, chinese_data* xt9_chinese_data);
    ~Write_Chinese();

    int start(int databaseID);
    int finish();
    int beginArc();
    int startNewArc(int& arcId);
    int addPoint(int arcId, int x, int y);
    int commitArc(int ardID);
    int endArc();
    int recognize(unsigned short *startOfWord, int& resultCount);
    int getCandidate(int index, DECUMA_UNICODE* word, int maxLength, int& length, int& bEndsWithInstGest);
    int applySettingChanges();
    int noteSelectedCandidate(int wordIndex);

    const char* getVersion() const;
    const char* getDatabaseVersion() const;

    int setContext(const unsigned short* context, int contextLen);
    int setAttribute(int id, int value);
    int getWord(int wordIndex, unsigned short* word, int& wordLen, int maxLen);

    int setCommonChar();
    int clearCommonChar();

public:
	DECUMA_SESSION* 			mSession;
	DECUMA_SESSION_SETTINGS*   	mSessionSettings;
	DECUMA_RECOGNITION_SETTINGS* mRecognitionSettings;
	DECUMA_MEM_FUNCTIONS 		 mMemFunctions;
	DECUMA_HWR_RESULT* 	mRecognitionCandidates;

private:
    void loadDatabase(FILE* fp, int length);
    void setLanguagesBaseOnCategories();

private:
    int             mCurrentLanguageID;
    DBRegistry*     mDbRegistry;
    chinese_data*   mXT9ChineseData;
    void*           mDatabase;
    int				mArcID;
    int				mRecognitionCandidateCount;
};

} // namespace mocainput

#endif // __WRITE_CHINESE_H__
