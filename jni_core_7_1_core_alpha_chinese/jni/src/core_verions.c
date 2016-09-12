
#include "et9api.h" // pull in version string
#include "xxet9oem.h"

#ifdef T9WRITE_ALPHA
#include "t9write_alpha_version.h"	// pull in version string
#endif

#ifdef T9WRITE_CHINESE
#include "t9write_cjk_version.h" // pull in version string
#endif


const char* getXT9CoreVersion()
{
	return ET9COREVER;
}

const char* getT9TraceVersion()
{

#ifdef ET9_KDB_TRACE_MODULE
	return ET9_TRACE_VER;
#endif

	return "null";
}

const char* getT9WriteAlphaVersion()
{

#ifdef T9WRITE_ALPHA
	return T9WRITEALPHACOREVER;
#endif

	return "null";
}

const char* getT9WriteChineseVersion()
{

#ifdef T9WRITE_CHINESE
	return T9WRITECJKCOREVER;
#endif

	return "null";
}

