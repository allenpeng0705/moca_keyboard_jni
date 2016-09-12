
#ifndef __WRITE_ALPHA_H__
#define __WRITE_ALPHA_H__

#include "decuma_hwr.h"
#include "et9api.h"
#include "dbregistry.h"
#include "alpha_controller.h"

namespace mocainput {

class Write_Alpha {
public:
    static const int MAX_POINTS = 300;
    static const int MAX_ARCS   = 64;
    static const int MAX_CHARACTERS = 64;
    static const int DEFAULT_DATABASE_ID = 0x01FF;

    static const int MAX_RECOGNIZE_CANDIDATES = 2; 	// number of handwriting candidates
    static const int MAX_PREDICTION_CANDIDATES = 8; // number of prediction candidates from moca
    static const int MAX_MIX_CANDIDATES = MAX_RECOGNIZE_CANDIDATES + MAX_PREDICTION_CANDIDATES;

    struct Candidate {
    	int len;
    	DECUMA_UNICODE word[MAX_CHARACTERS + 1];
    	int bInstantGesture;
    };

private:
    Write_Alpha(); // error if use

public:

    Write_Alpha(DBRegistry* dbRegistry, alpha_data* xt9_alpha_data);
    ~Write_Alpha();

    int applySettingChanges();
    int start(int languageID);
    int finish();

    int beginArc();
    int startNewArc(int& arcId);
    int addPoint(int arcId, int x, int y);
    int CommitArc(int arcId);
    int recognize(DECUMA_UNICODE* startWord, int &resultCount);
    int getCandidate(int index, DECUMA_UNICODE* word, int maxLength, int& length, int & bEndsWithInstGest);
    int noteSelectedCandidate(int index);
    int endArc();
    int getInstantGesture();
    int addWordToUserDictionary(const unsigned short*word, int length);
    const char* getVersion() const;
    const char* getDatabaseVersion() const;

public:
	DECUMA_ARC					 mArc;
	DECUMA_SESSION_SETTINGS*     mSessionSettings;
	DECUMA_RECOGNITION_SETTINGS* mRecognitionSettings;
	DECUMA_INSTANT_GESTURE_SETTINGS mInstantGestureSettings;

private:
    int setLanguageDatabase(int languageID);
    int setTemplateDatabase(int languageID);
	void* loadDatabase(const char* fileName);
	void* convertXT9Language(const char* xt9LdbName);
	void  convertXT9UserDictionary();
	void destroyXT9UserDictionary();
	void destroyXT9Language();
	void detachXT9Language();
	void detachXT9UserDictionary();

	int mixRecognitionCandidates();

private:
	int mCurrentLanguageID;
	int mCurrentTemplateDatabaseID;

    DBRegistry* mDBRegistry;
    alpha_data* mXT9AlphaData;
	int mArcID;
	DECUMA_HWR_RESULT* 	mRecognitionCandidates;
	int mRecognitionCandidateCount;
	DECUMA_SESSION* 	mSession;
    DECUMA_MEM_FUNCTIONS mMemFunctions;
    void*           	mLanguageDatabase;
    void*				mTemplateDatabase;
    void*				mUDB;

	int mMixCandidateCount;
    Candidate* mMixCandidatePositions[MAX_MIX_CANDIDATES]; // index to to mix list
    Candidate* mMixCandidates[MAX_MIX_CANDIDATES]; // mix candidate list that contain both result from hwr and xt9
};

} // namespace xt9input

#endif // __WRITE_ALPHA_H__
