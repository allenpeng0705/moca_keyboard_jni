
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>

#ifndef __WRITE_LOG_H__
#define __WRITE_LOG_H__

namespace mocainput {

class write_log {
public:
	write_log(const char* logFile) : mFile(0)
	{
		mFile = fopen(logFile, "at");
		if (mFile != 0) {
			char acTime[50];
			time_t t;
			time(&t);
			struct tm * p = localtime(&t);
			sprintf(acTime, "\n=== %02d/%02d/%4d %02d:%02d:%02d ===\n", p->tm_mon + 1, p->tm_mday, p->tm_year + 1900, p->tm_hour, p->tm_min, p->tm_sec);

			//fprintf(mFile, acTime);

			int len = sizeof(acTime);
			fwrite(acTime, 1, len, mFile);
			fflush(mFile);
		}
	}

	~write_log()
	{
		if (mFile != 0) {
			fclose(mFile);
		}
	}

	void log(const char* msg)
	{
		if (mFile != 0) {
			//fprintf(mFile, msg);
			int len = sizeof(msg);
			fwrite(msg, 1, len, mFile);
			fflush(mFile);
		}
	}

	void log(const char*msg, int len)
	{
		if (mFile != 0) {
			fwrite(msg, 1, len, mFile);
			fflush(mFile);
		}
	}

private:
	FILE* mFile;

private:
	write_log();
};

}

#endif
