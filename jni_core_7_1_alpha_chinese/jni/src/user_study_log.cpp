
#include "Log.h"

#include "user_study_log.h"
#include "load_file.h"
#include "ucs2.h"
#include <wchar.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>

namespace mocainput {

#if USER_STUDY_LOG

UserStudyLog::UserStudyLog()
{
    mLog = NULL;
}
UserStudyLog::~UserStudyLog()
{
    close();
}

bool UserStudyLog::log(const char * pcMsg)
{
    InternalOpen();
    if (mLog != NULL && pcMsg != NULL) {
    	if(!strcmp(pcMsg, "\n"))
    	{
    		fprintf(mLog, "\n");
    	}
    	else
    	{
			char acTime[26];
			time_t t;
			time(&t);
			struct tm * p = localtime(&t);
			sprintf(acTime, "%02d/%02d/%4d %02d:%02d:%02d", p->tm_mon + 1, p->tm_mday, p->tm_year + 1900, p->tm_hour, p->tm_min, p->tm_sec);
			fprintf(mLog, "%s,  %s\n", acTime, pcMsg);
			fflush(mLog);
    	}
        return true;
    }
    return false;
}

bool UserStudyLog::log(const WordLogInfo& wordlog)
{
    char acBuf[1024] = {0}, acUtf8[129];
    int nLen;

    // Completion Promoted or not, Word completion point,
    sprintf(acBuf + strlen(acBuf), "%d,%s", wordlog.m_nWCPoint, (wordlog.m_bBoosted? "Boost=yes": "Boost=no"));
    // Key sequence
    sprintf(acBuf + strlen(acBuf), ", %d,%d", wordlog.m_nKeyCount, wordlog.m_nDelKeyCount);

    // Selected word
    ucs2ToUtf8(acUtf8, sizeof(acUtf8), wordlog.m_awSelectedWord);
    nLen = 0;
    while (wordlog.m_awSelectedWord[nLen]) {
        nLen++;
    }
    sprintf(acBuf + strlen(acBuf), ", %d,%s,%d", nLen, acUtf8, wordlog.m_nSelectedWordIndex);

    // Default word
    ucs2ToUtf8(acUtf8, sizeof(acUtf8), wordlog.m_awDefaultWord);
    sprintf(acBuf + strlen(acBuf), ", %s,%d", acUtf8, wordlog.m_nDefaultWordIndex);

    // Exact Word
    ucs2ToUtf8(acUtf8, sizeof(acUtf8), wordlog.m_awExactWord);
    sprintf(acBuf + strlen(acBuf), ", %s", acUtf8);

    //Spell corrected?
    sprintf(acBuf + strlen(acBuf), ", %s", wordlog.m_bIsSpellCorr? "SC=yes": "SC=no");
    log(acBuf);
    return true;
}

bool UserStudyLog::InternalOpen()
{
    static const char * pcFileName = "/data/data/com.moca.input/files/UserStudyLog.txt";
    if (!file_exist(pcFileName) && mLog) {
        fclose(mLog);
        mLog = NULL;
    }
    if (mLog == NULL)
    {
        mLog = fopen(pcFileName, "at");
        if (mLog == NULL)  {
            LOGE("Failed to open data/moca/UserStudyLog.txt");
        }
        else if (file_size(mLog) > 0) {
            fprintf(mLog, "\n");
            fflush(mLog);
        }
        else {
            fprintf(mLog, "### Line format\n");
            fprintf(mLog, "WC Point, Boosting, WSI Symb Count, Num of del key, Selected word len, Selected word, Selected word index, Default word, Default word index, Exact Word, Spell correction\n");
            //fprintf(mLog, "### where SelList Mode: Cl -- Classic,  CP -- Completion Promoted, MX -- Mixed\n###=====================================================\n");
            fflush(mLog);
        }

        LOGV("UserStudyLog::InternalOpen() - Success");
    }
    return (mLog != NULL);
}

bool UserStudyLog::open()
{
    LOGI("UserStudyLog::open()");
    return InternalOpen();
}

void UserStudyLog::close()
{
    LOGI("UserStudyLog::close()");
    if (mLog != NULL) {
        fclose(mLog);
        mLog = NULL;
    }
}

#endif  // USER_STUDY_LOG

} // namespace mocainput
