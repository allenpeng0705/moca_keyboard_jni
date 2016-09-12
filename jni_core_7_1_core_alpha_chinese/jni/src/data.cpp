
#include "Log.h"

#include <assert.h>
#include <string.h>
#include <errno.h>
#include "data.h"
#include "load_file.h"
#include "mem_alloc.h"

namespace mocainput {

//dynamic kdb (text)
ET9STATUS data::KdbReadText(
                    ET9KDBInfo          *pKDBInfo,
                    ET9U16               wKdbNum,
                    ET9U16               wPageNum)
{
	LOGV("data::KdbReadText() start");
    data* _data = reinterpret_cast<data*>(pKDBInfo->pPublicExtension);

    if (_data && _data->readKdbText((int)wKdbNum, (int)wPageNum)) {
    	LOGV("data::KdbReadText() Done");
        return ET9STATUS_NONE;
    }
    LOGV("data::KdbReadText() Fail");
    return (ET9STATUS_READ_DB_FAIL);
}
//dynamic kdb (xml)
ET9STATUS data::KdbReadXml(
                    ET9KDBInfo          *pKDBInfo,
                    ET9U16               wKdbNum,
                    ET9U16               wPageNum)
{
	LOGV("data::KdbReadXml() start");
    data* _data = reinterpret_cast<data*>(pKDBInfo->pPublicExtension);

    if (_data && _data->readKdbXml((int)wKdbNum, (int)wPageNum)) {
    	LOGV("data::KdbReadXml() done");
        return ET9STATUS_NONE;
    }
    LOGV("data::KdbReadXml() fail");
    return (ET9STATUS_READ_DB_FAIL);
}

//static
ET9STATUS data::KdbReadCallback(
                    ET9KDBInfo          *pKDBInfo,
                    ET9U8 * /*ET9FARDATA*/  *ppbSrc,
                    ET9U32              *pdwSizeInBytes)
{
	LOGV("data::KdbReadCallback() start");
    data* _data = reinterpret_cast<data*>(pKDBInfo->pPublicExtension);

    if (_data && _data->readKdb((int)pKDBInfo->wKdbNum, (char**)ppbSrc, (int*)pdwSizeInBytes)) {
    	LOGV("data::KdbReadCallback() done");
        return ET9STATUS_NONE;
    }
    LOGV("data::KdbReadCallback() fail");
    return (ET9STATUS_READ_DB_FAIL);
}

/*bool data::readLdb(int ldbId, char** src, int* sizeInBytes)
{
    if (mCurrentLdbId != ldbId) {
        if (!loadLdb(ldbId)) {
            return false;
        }
    }

    if (mLdbData) {
        assert(mLdbSize);
        *src = mLdbData;
        *sizeInBytes = mLdbSize;
        return true;
    }

    return false;
}*/

bool data::readLdb(int ldbId, char** src, int* sizeInBytes)
{
	LOGV("data::readLdb() start ldbID:%d this:0x%X", ldbId, this);
	xldb *pXLdb = getXLdb(ldbId);

	if (!pXLdb) {
		pXLdb = getXLdb(0);
		if (!pXLdb) {
			LOGV("data::readLdb() fail ldbID:%d this:0x%X", ldbId, this);
			return false;
		}

		pXLdb->mLdbId = ldbId;
		if (!loadXLdb(ldbId, pXLdb)) {
			LOGV("data::readLdb() fail ldbID:%d this:0x%X", ldbId, this);
			return false;
		}
	}

	if (pXLdb) {
		*src = pXLdb->mLdbData;
		*sizeInBytes = pXLdb->mLdbDataSize;
		LOGV("data::readLdb() done ldbID:%d this:0x%X", ldbId, this);
		return true;
	}
	LOGV("data::readLdb() fail ldbID:%d this:0x%X", ldbId, this);
	return false;
}

bool data::readKdb(int kdbId, char** src, int* sizeInBytes)
{
	LOGV("data::readKdb() start kdbID: %d this:0x%X", kdbId, this);
    if (mCurrentKdbId != kdbId) {
        if (!loadKdb(kdbId)) {
        	LOGV("data::readKdb() fail kdbID: %d this:0x%X", kdbId, this);
            return false;
        }
    }

    if (mKdbData) {
        assert(mKdbSize);
        *src = mKdbData;
        *sizeInBytes = mKdbSize;
        mCurrentKdbId = kdbId;
    	LOGV("data::readKdb() done kdbID: %d this:0x%X", kdbId, this);
        return true;
    }
	LOGV("data::readKdb() fail kdbID: %d this:0x%X", kdbId, this);
    return false;
}

bool data::readKdbText(int kdbId, int PageNum)
{
	LOGV("data::readKdbText() start kdbID: %d this:0x%X", kdbId, this);
	  ET9STATUS status = ET9STATUS_NONE;

    if (mCurrentKdbId != kdbId) {
        if (!loadKdb(kdbId)) {
        	LOGV("data::readKdbText() fail kdbID: %d this:0x%X", kdbId, this);
            return false;
        }
    }

    if (mKdbData) {
        assert(mKdbSize);
        if ((status = ET9KDB_Load_TextKDB(mKdbInfo, (ET9U16) PageNum, (ET9U8 *) mKdbData, (ET9U32) mKdbSize, NULL)) == ET9STATUS_NONE) {
	          mCurrentKdbId = kdbId;
	          LOGV("data::readKdbText() done kdbID: %d this:0x%X", kdbId, this);
	          return true;
        }
    }
    LOGV("data::readKdbText() fail kdbID: %d this:0x%X", kdbId, this);
    return false;
}
bool data::readKdbXml(int kdbId, int PageNum)
{
	LOGV("data::readKdbXml() start kdbID: %d this:0x%X", kdbId, this);
      ET9UINT nErrorLine;
      ET9STATUS status = ET9STATUS_NONE;

    if (mCurrentKdbId != kdbId) {
        if (!loadKdb(kdbId)) {
        	LOGV("data::readKdbXml() fail kdbID: %d this:0x%X", kdbId, this);
            return false;
        }
    }
    
    if (mKdbData) {
        assert(mKdbSize);
        if ((status = ET9KDB_Load_XmlKDB(mKdbInfo, 0, 0, (ET9U16) PageNum, (ET9U8 *) mKdbData, (ET9U32) mKdbSize, NULL, NULL, &nErrorLine)) == ET9STATUS_NONE) {
              mCurrentKdbId = kdbId;
              LOGV("data::readKdbXml() done kdbID: %d this:0x%X", kdbId, this);
              return true;
        }
    }
    LOGV("data::readKdbXml() fail kdbID: %d this:0x%X", kdbId, this);
    return false;
}

/**
 * Load a file into memory, freeing data if overwriting
 */
bool data::loadFile(const char* path, char*& data, int& size) {
	LOGV("data::loadFile() start path: %s", path);
    if (!path) {
    	LOGV("data::loadFile() fail path: %s", path);
        return false;
    }

    char* tmpData = 0;
    int tmpSize;

    tmpData = (char*)load_bin_file(path, tmpSize);
    if (!tmpData) {
    	LOGV("data::loadFile() fail path: %s", path);
        return false;
    }

    if (data != 0) {
    	LOGV("data::loadFile() freed previous ldb mem buffer");
    	FREE(data);
    }

    data = tmpData;
    size = tmpSize;
    LOGV("data::loadFile() done path: %s", path);
    return true;
}

bool data::loadXLdbFile(const char *path, xldb *pXLdb) {
	LOGV("data::loadXLdbFile() start path: %s", path);
    if (!path) {
    	LOGV("data::loadXLdbFile() fail path: %s", path);
        return false;
    }

    char* tmpData = 0;
    int tmpSize;

    tmpData = (char*)load_bin_file(path, tmpSize);
    if (!tmpData) {
    	LOGV("data::loadXLdbFile() fail path: %s", path);
        return false;
    }

    pXLdb->mLdbData = tmpData;
    pXLdb->mLdbDataSize = tmpSize;
    LOGV("data::loadXLdbFile() done path: %s", path);
    return true;
}

bool data::loadKdb(int kdbId)
{
	LOGV("data::loadKdb() start kdbId: %d this:0x%X", kdbId, this);
    const char* kdbPath;

    kdbPath = mDBRegistry->get_kdb_path(kdbId);
    if (!kdbPath) {
    	LOGV("data::loadKdb() fail kdbId: %d path:%s this:0x%X", kdbId, kdbPath, this);
        return false;
    }

    if (loadFile(kdbPath, mKdbData, mKdbSize)) {
        mCurrentKdbId = kdbId;
        LOGV("data::loadKdb() done kdbId: %d path:%s this:0x%X", kdbId, kdbPath, this);
        return true;
    }
    LOGV("data::loadKdb() fail kdbId: %d path:%s this:0x%X", kdbId, kdbPath, this);
    return false;
}

bool data::loadLdb(int ldbId)
{
    const char* ldbPath;

    LOGV("data::loadLdb start ldbId:(0x%X) this:0x%X", ldbId,this);

    ldbPath = mDBRegistry->get_ldb_path(ldbId);
    if (!ldbPath) {
    	LOGV("data::loadLdb fail ldbId:%d path:%s this:0x%X", ldbId, ldbPath, this);
        return false;
    }

    if (loadFile(ldbPath, mLdbData, mLdbSize)) {
        mCurrentLdbId = ldbId;
        LOGV("data::loadLdb(0x%X)...done", ldbId);
        return true;
    }

    LOGV("data::loadLdb fail ldbId:%d path:%s this:0x%X", ldbId, ldbPath, this);
    return false;
}

bool data::loadXLdb(int ldbId, xldb *pXLdb) {
    const char* ldbPath;

    LOGV("data::loadXLdb(0x%X)... this:0x%X", ldbId,this);

    ldbPath = mDBRegistry->get_ldb_path(ldbId);
    if (!ldbPath) {
    	LOGE("data::loadXLdb(0x%X)...failed path:%s this:0x%X", ldbId, ldbPath, this);
        return false;
    }

    strcpy(pXLdb->mFileName, ldbPath);

    if (loadXLdbFile(ldbPath, pXLdb)) {
        LOGV("data::loadXLdb(0x%X)...ok this:0x%X", ldbId, this);
        return true;
    }

    LOGE("data::loadXLdb(0x%X)...failed path:%s this:0x%X", ldbId, ldbPath, this);
    memset(pXLdb, 0, sizeof(pXLdb));
    return false;
}

data::xldb* data::getXLdb(int ldbId) {
	xldb *pXLdb = NULL;

	for (int i = 0; i < XLDBMAXCOUNT; i++) {
		if (mXLdbs[i]->mLdbId == ldbId) {
			pXLdb = mXLdbs[i];
			break;
		}
	}

	return pXLdb;
}

data::data(DBRegistry* dbRegistry)
{
	LOGV("data::data(DBRegistry:0x%X) start this:0x%X", dbRegistry, this);

	ET9STATUS status = ET9STATUS_NO_KEY;
    mLdbData = 0;
    mKdbData = 0;
    mKdbSize = 0;
    mCurrentKdbId = 0;
    mCurrentLdbId = 0;
    // share WSI between Alpha and Chinese has some issue, so now they use separate ones
    // the major problem is the setting the slots value of pWSI->Private.ppEditionsList
	//mWordSymbInfo = wordSymbInfo::create();
    mWordSymbInfo = (ET9WordSymbInfo*)CALLOC(sizeof(ET9WordSymbInfo), 1);

    mKdbInfo = kdbInfo::create();
    mDBRegistry = dbRegistry;

    // Initialize with the default kdb.  We must have an init before we can use generic setting
    // APIs.  Otherwise the setting APIs will fail.
    if ((status = ET9KDB_Init(mKdbInfo, DEFAULT_KDB_NUM, 0, 0, 0, data::KdbReadText, NULL, 0, this)) != ET9STATUS_NONE) {
    	LOGE("data::kdbinit(0x%X)...failed to initialize keyboard module - status(%X)", DEFAULT_KDB_NUM, status);
    }

#if USER_STUDY_LOG
    m_pUserStudyLog = new UserStudyLog;
    if (m_pUserStudyLog == NULL)  {
        LOGE("new UserStudyLog failed, m_pUserStudyLog == NULL");
    }
#endif

    for (int i = 0; i < XLDBMAXCOUNT; i++) {
    	mXLdbs[i] = new xldb();
    }

    LOGV("data::data(DBRegistry:0x%X) done this:0x%X", dbRegistry, this);
}

data::~data()
{
	LOGV("data::~data()...");

	FREE(mKdbData);
	FREE(mLdbData);

    //wordSymbInfo::destroy();
	FREE(mWordSymbInfo);

    kdbInfo::destroy();

#if USER_STUDY_LOG
    if (m_pUserStudyLog)  {
        delete m_pUserStudyLog;
        m_pUserStudyLog = NULL;
    }
#endif

    for (int i = 0; i < XLDBMAXCOUNT; i++) {
    	FREE(mXLdbs[i]);
    	mXLdbs[i] = NULL;
    }
	LOGV("data::~data()...done");

}

int data::getKdbId()
{
    return mKdbInfo->wKdbNum;
}

int data::initTrace(int kdbId, int width, int height)
{
#ifndef ET9_KDB_TRACE_MODULE

    LOGE("data::initTrace - ERROR: Using TRACE while ET9_KDB_TRACE_MODULE is not define");

	return ET9STATUS_ERROR;

#else
    ET9STATUS status = ET9STATUS_NONE;

	// verifying the parameters are valid
	if((!width) || (!height)) {
		// one ore more invalid parameters
		return ET9STATUS_INVALID_MEMORY;
	}

	// see processKey below for comment
	if (mKdbInfo->wKdbNum != kdbId && kdbId != 0xFFFF) {
		if (isChinesePhonepadKdb(kdbId)) {
			if ((status = ET9KDB_Init(mKdbInfo, kdbId, 0, 0, 0, (kdbId == 0x0E04)?data::KdbReadXml:data::KdbReadText, NULL, 0, this)) != ET9STATUS_NONE) {
				LOGW("data::initTrace(0x%X)...failed to initialize keyboard module - status(%X)", kdbId, status);
			}
		} else {
            if ((status = ET9KDB_Init(mKdbInfo, kdbId, 0, 0, 0, data::KdbReadText, NULL, 0, this)) != ET9STATUS_NONE) {
                LOGE("data::initTrace(0x%X)...failed to initialize keyboard module - status(%X)", kdbId, status);
            }
		}
		if ((status = ET9KDB_SetKdbNum(mKdbInfo, kdbId, 0, 0, 0)) != ET9STATUS_NONE) {
			LOGW("data::initTrace(0x%X)...failed to set keyboard - status(%X)", kdbId, status);
		}
	}

	// setting the keyboard's size
	status = ET9KDB_SetKeyboardSize(this->mKdbInfo, width, height);

	LOGV("data::initTrace(0x%X, 0x%X, 0x%X...status(%X)", kdbId, width, height, status);

	return status;

#endif
}

bool data::getKeyPositions(int kdbId, ET9KeyPoint* points, unsigned int maxPoints, unsigned int& numPoints)
{
    ET9STATUS status = ET9STATUS_NONE;

    // verifying the parameters are valid
	if(!points)
	{
		// one or more invalid parameters
		return 0;
	}

	// see processKey below for comment
	if (mKdbInfo->wKdbNum != kdbId && kdbId != 0xFFFF) {
		if (isChinesePhonepadKdb(kdbId)) {
			if ((status = ET9KDB_Init(mKdbInfo, kdbId, 0, 0, 0, (kdbId == 0x0E04)?data::KdbReadXml:data::KdbReadText, NULL, 0, this)) != ET9STATUS_NONE) {
				LOGW("data::getKeyPositions(0x%X)...failed to initialize keyboard module - status(%X)", kdbId, status);
			}
		} else {
            if ((status = ET9KDB_Init(mKdbInfo, kdbId, 0, 0, 0, data::KdbReadText, NULL, 0, this)) != ET9STATUS_NONE) {
                LOGE("data::getKeyPositions(0x%X)...failed to initialize keyboard module - status(%X)", kdbId, status);
            }
		}
		if ((status = ET9KDB_SetKdbNum(mKdbInfo, kdbId, 0, 0, 0)) != ET9STATUS_NONE) {
			LOGW("data::getKeyPositions(0x%X)...failed to set keyboard - status(%X)", kdbId, status);
		}
	}

	status = ET9KDB_GetKeyPositions(mKdbInfo, points, maxPoints, &numPoints);

	LOGV("data::getKeyPositions(0x%X) - status(%X)", kdbId, status);

	return status == ET9STATUS_NONE;
}

int data::isAutoSpaceBeforeTrace(int kdbId, ET9TracePoint* points, int numPoints)
{
#ifndef ET9_KDB_TRACE_MODULE

    LOGE("data::isAutoSpaceBeforeTrace - ERROR: Using TRACE while ET9_KDB_TRACE_MODULE is not define");

	return false;

#else

    ET9STATUS status = ET9STATUS_NONE;

    // verifying the parameters are valid
	if((!points) || (!numPoints)) {
		/*// auto accept when tapping in words that start with a trace
		if((mWordSymbInfo->bNumSymbs > 0) && ((mWordSymbInfo->SymbsInfo[0].bTraceIndex != 0) || (mWordSymbInfo->SymbsInfo[0].bTraceProbability != 0)))
		{
			return true;
		}*/

		// don't auto space
		return false;
	}

	// see processKey below for comment
	if (mKdbInfo->wKdbNum != kdbId && kdbId != 0xFFFF) {

		if (isChinesePhonepadKdb(kdbId)) {
			if ((status = ET9KDB_Init(mKdbInfo, kdbId, 0, 0, 0, (kdbId == 0x0E04)?data::KdbReadXml:data::KdbReadText, NULL, 0, this)) != ET9STATUS_NONE) {
				LOGW("data::isAutoSpaceBeforeTrace(0x%X)...failed to initialize keyboard module - status(%X)", kdbId, status);
			}
		}
		else {
			if ((status = ET9KDB_SetKdbNum(mKdbInfo, kdbId, 0, 0, 0)) != ET9STATUS_NONE) {
				LOGW("data::isAutoSpaceBeforeTrace(0x%X)...failed to set keyboard - status(%X)", kdbId, status);
			}
		}
	}

	/*// auto accept when tap then trace
	if((mWordSymbInfo->bNumSymbs > 0) && (mWordSymbInfo->SymbsInfo[0].bTraceIndex == 0) && (mWordSymbInfo->SymbsInfo[0].bTraceProbability == 0))
	{
		return true;
	}*/

	ET9BOOL bAddSpace = false;
	ET9BOOL bAccept = ET9KDB_IsAutoAcceptBeforeTrace(mKdbInfo, mWordSymbInfo, points, numPoints, &bAddSpace);
	LOGW("data::isAutoSpaceBeforeTrace bAccept(0x%X)", bAccept);
	return ((!bAccept) ? 0 : ((bAccept && !bAddSpace) ? 1 : 2));

#endif

}

int data::processTrace(int kdbId, ET9TracePoint* points, int numPoints, int shiftState)
{
#ifndef ET9_KDB_TRACE_MODULE

	LOGE("data::isAutoSpaceBeforeTrace - ERROR: Using TRACE while ET9_KDB_TRACE_MODULE is not define");

	return ET9STATUS_ERROR;

#else

	ET9STATUS status = ET9STATUS_NONE;

    // verifying the parameters are valid
	if((!points) || (!numPoints))
	{
		// one or more invalid parameters
		return 0;
	}

	// see processKey below for comment
	if (mKdbInfo->wKdbNum != kdbId && kdbId != 0xFFFF) {
		if (isChinesePhonepadKdb(kdbId)) {
			if ((status = ET9KDB_Init(mKdbInfo, kdbId, 0, 0, 0, (kdbId == 0x0E04)?data::KdbReadXml:data::KdbReadText, NULL, 0, this)) != ET9STATUS_NONE) {
				LOGW("data::processTrace(0x%X)...failed to initialize keyboard module - status(%X)", kdbId, status);
			}
		} else {
            if ((status = ET9KDB_Init(mKdbInfo, kdbId, 0, 0, 0, data::KdbReadText, NULL, 0, this)) != ET9STATUS_NONE) {
                LOGE("data::processTrace(0x%X)...failed to initialize keyboard module - status(%X)", kdbId, status);
            }
		}
		if ((status = ET9KDB_SetKdbNum(mKdbInfo, kdbId, 0, 0, 0)) != ET9STATUS_NONE) {
			LOGW("data::processTrace(0x%X)...failed to set keyboard - status(%X)", kdbId, status);
		}
	}

	// change shift state if different
	if (shiftState != getShiftState()) {
		setShiftState(shiftState);
	}

	ET9SYMB sTmp;
	status = ET9KDB_ProcessTrace(mKdbInfo, mWordSymbInfo, points, numPoints, ET9_NO_ACTIVE_INDEX, &sTmp);

	LOGV("bNumSymbs: %X", mWordSymbInfo->bNumSymbs);

	LOGV("data::processTrace(0x%X, 0x%X, 0x%X...status(%X))", kdbId, numPoints, shiftState, status);

	return (status == ET9STATUS_NONE || status == ET9STATUS_FULL || status == ET9STATUS_NO_KEY) && mWordSymbInfo->bNumSymbs > 0;

#endif

}

bool data::isHasTraceInfo()
{
#ifndef ET9_KDB_TRACE_MODULE

    LOGE("data::isHasTraceInfo - ERROR: Using TRACE while ET9_KDB_TRACE_MODULE is not define");

    return false;

#else
    ET9UINT nCount;
    ET9SymbInfo const *pSymbInfo;

    if (!mWordSymbInfo || mWordSymbInfo->wInitOK != ET9GOODSETUP) {
        return false;
    }

    pSymbInfo = mWordSymbInfo->SymbsInfo;
    for (nCount = mWordSymbInfo->bNumSymbs; nCount; --nCount, ++pSymbInfo) {
        if (pSymbInfo->bTraceIndex) {
            return true;
        }
    }
    return false;
#endif
}
void data::backupWordSymbolInfo() 
{
#ifdef ET9_KDB_TRACE_MODULE
    if(mWordSymbInfo != NULL) {
    LOGV("data::backupWordSymbolInfo");
        sWordSymbInfoBackup = *mWordSymbInfo;
    }
#endif    
}
void data::restoreWordSymbolInfo()
{
#ifdef ET9_KDB_TRACE_MODULE
    if(mWordSymbInfo != NULL) {
    LOGV("data::restoreWordSymbolInfo");
        *mWordSymbInfo = sWordSymbInfoBackup;
    }
#endif    
}

int data::processTap(int kdbId, int TapX, int TapY, int shiftState)
{
    ET9STATUS status = ET9STATUS_NO_KEY;
    ET9U16 dummy;

    // see processKey below for comment
    if (mKdbInfo->wKdbNum != kdbId && kdbId != 0xFFFF) {
		if (isChinesePhonepadKdb(kdbId)) {
			if ((status = ET9KDB_Init(mKdbInfo, kdbId, 0, 0, 0, (kdbId == 0x0E04)?data::KdbReadXml:data::KdbReadText, NULL, 0, this)) != ET9STATUS_NONE) {
				LOGW("data::processTap(0x%X)...failed to initialize keyboard module - status(%X)", kdbId, status);
			}
		} else {
            if ((status = ET9KDB_Init(mKdbInfo, kdbId, 0, 0, 0, data::KdbReadText, NULL, 0, this)) != ET9STATUS_NONE) {
                LOGE("data::processTap(0x%X)...failed to initialize keyboard module - status(%X)", kdbId, status);
            }
		}
		if ((status = ET9KDB_SetKdbNum(mKdbInfo, kdbId, 0, 0, 0)) != ET9STATUS_NONE) {
			LOGW("data::processTap(0x%X)...failed to set keyboard - status(%X)", kdbId, status);
		}
    }

    // change shift state if different
    if (shiftState != getShiftState()) {
        setShiftState(shiftState);
    }

    // process tap
    status = ET9KDB_ProcessTap(mKdbInfo, mWordSymbInfo, TapX, TapY, ET9_NO_ACTIVE_INDEX, &dummy);

    // if could not find the key, log error
    LOGV("data::processTap(0x%X, 0x%X, 0x%X, 0x%X...status(%X)", kdbId, TapX, TapY, shiftState, status);

    return status;
}

int data::processKey(int kdbId, int key, int shiftState)
{
    ET9STATUS status = ET9STATUS_NO_KEY;
    ET9U16 dummy;

    if (mKdbInfo->wKdbNum != kdbId && kdbId != 0xFFFF) {
		if (isChinesePhonepadKdb(kdbId)) {
			if ((status = ET9KDB_Init(mKdbInfo, kdbId, 0, 0, 0, (kdbId == 0x0E04)?data::KdbReadXml:data::KdbReadText, NULL, 0, this)) != ET9STATUS_NONE) {
				LOGW("data::processKey(0x%X)...failed to initialize keyboard module - status(%X)", kdbId, status);
			}
		} else {
            if ((status = ET9KDB_Init(mKdbInfo, kdbId, 0, 0, 0, data::KdbReadText, NULL, 0, this)) != ET9STATUS_NONE) {
                LOGE("data::processKey(0x%X)...failed to initialize keyboard module - status(%X)", kdbId, status);
            }
		}
		if ((status = ET9KDB_SetKdbNum(mKdbInfo, kdbId, 0, 0, 0)) != ET9STATUS_NONE) {
			LOGW("data::processKey(0x%X)...failed to set keyboard - status(%X)", kdbId, status);
		}
	}

    // change shift state if different
    if (shiftState != getShiftState()) {
        setShiftState(shiftState);
    }

    if(kdbId != 0xFFFF)
    {
		// process key by symbol
		status = ET9KDB_ProcessKeyBySymbol(mKdbInfo, mWordSymbInfo, key, ET9_NO_ACTIVE_INDEX, &dummy, 1);
	}

    // if could not find the key index for the key, try to add explicitly
    if (status) {
        status = ET9AddExplicitSymb(mWordSymbInfo, key, (ET9INPUTSHIFTSTATE)shiftState, ET9_NO_ACTIVE_INDEX);
    }

    LOGV("data::processKey(0x%X, 0x%X, 0x%X...status(%X)", kdbId, key, shiftState, status);

    return status;
}

int data::addExplicitKey(const unsigned short key, int shiftState)
{
    return ET9AddExplicitSymb(mWordSymbInfo, (ET9SYMB)key, (ET9INPUTSHIFTSTATE)shiftState, ET9_NO_ACTIVE_INDEX);;
}

int data::addExplicitString(const unsigned short* word, int len, int shiftState)
{
    int status = 0;

    for (int i = 0; i < len; i++) {
        status = addExplicitKey(word[i], (ET9INPUTSHIFTSTATE)shiftState);
        if (status) {
            return status;
        }
    }

    return status;
}

void data::setShiftState(int shift)
{
    if (shift == ET9CAPSLOCK) {
        ET9SetCapsLock(mWordSymbInfo);
    }
    else if (shift == ET9SHIFT) {
        ET9SetShift(mWordSymbInfo);
    }
    else {
        ET9SetUnShift(mWordSymbInfo);
    }
}

int data::getShiftState()
{
    if (ET9CAPS_MODE(mWordSymbInfo->dwStateBits)) {
        return ET9CAPSLOCK;
    }
    else if (ET9SHIFT_MODE(mWordSymbInfo->dwStateBits)) {
        return ET9SHIFT;
    }
    return ET9NOSHIFT;
}

bool data::isChinesePhonepadKdb(int kdbId)
{
	return ((kdbId & 0x00FF) == 0x04);
}

int data::getKeyCount()
{
    if (mWordSymbInfo && mWordSymbInfo->bNumSymbs) {
        return mWordSymbInfo->bNumSymbs;
    }

    return 0;
}

int data::getExactWord(unsigned short* buffer, int maxBufferLen)
{
    ET9SimpleWord word;

    if (ET9GetExactWord(mWordSymbInfo, &word, 0, 0) == ET9STATUS_NONE) {
        int len = word.wLen > maxBufferLen ? maxBufferLen : word.wLen;
        data::wordCopy(buffer, word.sString, len);
        return len;
    }

    return 0;
}

int data::clearKey()
{
	LOGV("data::clearKey start, this:0x%X", this);
    if (mWordSymbInfo && mWordSymbInfo->bNumSymbs) {
        ET9ClearOneSymb(mWordSymbInfo);
        LOGV("data::clearKey done ET9STATUS_NONE this:0x%X", this);
        return ET9STATUS_NONE;
    }
    LOGV("data::clearKey done ET9STATUS_EMPTY this:0x%X", this);
    return ET9STATUS_EMPTY;
}

int data::clearAllKeys()
{
	LOGV("data::clearAllKeys start this:0x%X", this);
    if (mWordSymbInfo && mWordSymbInfo->bNumSymbs) {
        ET9ClearAllSymbs(mWordSymbInfo);
        LOGV("data::clearAllKeys done ET9STATUS_NONE this:0x%X", this);
        return ET9STATUS_NONE;
    }
    LOGV("data::clearAllKeys done ET9STATUS_EMPTY this:0x%X", this);
    return ET9STATUS_EMPTY;
}

int data::addCustomSymbolSet(unsigned short* const symbols, int * const prob, const int symbolCount, int shiftState, int wordindex)
{
	LOGV("data::addCustomSymbolSet start this:0x%X", this);
    ET9STATUS wStatus = ET9AddCustomSymbolSet(mWordSymbInfo, (ET9SYMB *)symbols, (ET9U8 *)prob, symbolCount, (ET9INPUTSHIFTSTATE)shiftState, wordindex);
    LOGV("data::addCustomSymbolSet done this:0x%X", this);
    return wStatus;
}

int data::ReCaptureWord(int kdbId, unsigned short * const word, int wordLen)
{
	LOGV("data::ReCaptureWord start this:0x%X", this);
	ET9STATUS status = ET9STATUS_NO_KEY;

	if (mKdbInfo->wKdbNum != kdbId && kdbId != 0xFFFF) {
		if ((status = ET9KDB_SetKdbNum(mKdbInfo, kdbId, 0, 0, 0)) != ET9STATUS_NONE) {
			LOGE("data::ReCaptureWord(0x%X)...failed to change keyboard database - status(%X)", kdbId, status);
			return status;
		}
	}
	LOGV("data::ReCaptureWord end this:0x%X", this);
	return ET9KDB_ReselectWord(mKdbInfo, mWordSymbInfo, (ET9SYMB *)word, (ET9U16) wordLen);
}

int data::isUpperSymbol(const unsigned short symbol)
{
	return ET9SymIsUpper(symbol);
}

int data::isLowerSymbol(const unsigned short symbol)
{
	return ET9SymIsLower(symbol);
}

unsigned short data::toLowerSymbol(const unsigned short symbol)
{
	return ET9SymToLower(symbol);
}

unsigned short data::toUpperSymbol(const unsigned short symbol)
{
	return ET9SymToUpper(symbol);
}

////
// Persistent DB stuffs
//
data::persistentDb::persistentDb(char* dbFile) :
    mDbBufferSize(0), mDbBuffer(0), mDbFile(0), mFile(0)
{
	LOGV("data::persistentDb::persistentDb start DBFile path:%s this:0x%X", dbFile, this);
    if (dbFile) {
        mDbFile = (char*)CALLOC(strlen(dbFile) + 1, 1);
        if (mDbFile) {
            strcpy(mDbFile, dbFile);
        }
    }
	LOGV("data::persistentDb::persistentDb done DBFile path:%s this:0x%X", dbFile, this);
}
data::persistentDb::~persistentDb()
{
    close();
}

void data::persistentDb::create(int bufferSize)
{
    LOGV("data::persistentDb::create(%s)...this:0x%X", mDbFile, this);

    if (mDbBuffer != 0) {
        return;
    }

    mDbBufferSize = bufferSize;

    // open for r/w if exist, else create new file or r/w
    mFile = fopen(mDbFile, file_exist(mDbFile) ? "r+b" : "w+b");

    if (0 == mFile) {
        LOGE("data::persistentDb::create(%s)...failed to open new file for writing this:0x%X", mDbFile, this);
        return;
    }

    mDbBuffer = (char*)CALLOC(mDbBufferSize, 1);
    if (0 == mDbBuffer) {
        LOGE("data::persistentDb::create(%s)...failed to allocate %d buffer size this:0x%X", mDbFile, mDbBufferSize, this);
        return;
    }

    // Either this is the first time file is created (file size is 0) or existing file, if
    // the db buffer size is not the same as the file size, we set the db file content to zero's
    int size = 0;
    if (mDbBufferSize != (size = file_size(mFile))) {
        LOGV("data::persistentDb::create(%s) - zero out file content, file size(%d), buffer size(%d) this:0x%X", mDbFile, size, mDbBufferSize, this);
        // do nothing
    }
    else {
        // read content from file
        size_t read = 0;
        if ((read = fread(mDbBuffer, 1, mDbBufferSize, mFile)) != (size_t)mDbBufferSize) {
            LOGW("data::persistentDb::create(%s)...failed to read file, expecting %d, read %d this:0x%X", mDbFile, mDbBufferSize, read, this);
            memset(mDbBuffer, 0, mDbBufferSize);
        }
    }

    LOGV("data::persistentDb::create(%s)...buffer size = %d this:0x%X", mDbFile, mDbBufferSize, this);
    return;
}

void data::persistentDb::close()
{
    // flush to file
    if (mFile) {
        flush();
        fclose(mFile);
    }

    FREE(mDbBuffer);
    FREE(mDbFile);

    mFile = 0;
    mDbBufferSize = 0;
}

void data::persistentDb::flush()
{
    if (mFile) {
       fseek(mFile, SEEK_SET, 0);
       fwrite(mDbBuffer, 1, mDbBufferSize, mFile);
    }
}

data::xldb::xldb() {
	mLdbId = 0;
	mFileName[0] = '\0';
	mLdbData = NULL;
	mLdbDataSize = 0;
}

data::xldb::~xldb() {
	if (mLdbData != NULL) {
		FREE(mLdbData);
		mLdbData = NULL;
	}
}

////////////////////////////////////////////////////////////////////////////////
// static
ET9WordSymbInfo* wordSymbInfo::sWordSymbInfo = 0;
int wordSymbInfo::sRefCount = 0;

// static
ET9KDBInfo* kdbInfo::sKdbInfo = 0;
int kdbInfo::sRefCount = 0;

} // namespace mocainput
