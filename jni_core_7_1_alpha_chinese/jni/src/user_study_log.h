#ifndef __User_Study_Log_H__
#define __User_Study_Log_H__

#include <stdio.h>
#include "et9api.h"

namespace mocainput {

//#define USER_STUDY_LOG 1

#ifdef USER_STUDY_LOG
    struct WordLogInfo
    {
        //ET9ASLMODE m_eMode;
        int m_nWCPoint;

        int m_nKeyCount;
        int m_nDelKeyCount;
        bool m_bBoosted;

        bool m_bIsSpellCorr;
        int m_nSelectedWordIndex;
        ET9U16 m_awSelectedWord[64+1];

        int m_nDefaultWordIndex;
        ET9U16 m_awDefaultWord[64+1];

        ET9U16 m_awExactWord[64+1];
    };

    class UserStudyLog
    {
    private:
        FILE * mLog;
        bool InternalOpen();
    public:
        UserStudyLog();
        ~UserStudyLog();
        bool open();
        void close();
        bool log(const char * pcMsg);
        bool log(const WordLogInfo& wordlog);
    };
#endif  // USER_STUDY_LOG

} // namespace mocainput

#endif
