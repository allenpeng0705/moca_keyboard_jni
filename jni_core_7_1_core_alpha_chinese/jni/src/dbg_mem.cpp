
#include <stdlib.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "Log.h"

namespace mocainput {

static int total_allocated = 0;

/* Memory alignment is important */
typedef union { double d; struct {size_t n; const char *file; int line;} s; } DbgBlock;

void *debug_calloc(size_t elements, size_t n, const char* file, int line)
{
    char *rp;
    rp = (char*)malloc(sizeof(DbgBlock) + (n * elements));
    memset(rp, 0, sizeof(DbgBlock) + (n * elements));
    ((DbgBlock*)rp)->s.n = n * elements;
    ((DbgBlock*)rp)->s.file = file;
    ((DbgBlock*)rp)->s.line = line;
    total_allocated += n * elements;

	LOGD("debug_calloc() memory = %p, allocated = %d, total_allocated = %d", rp, n * elements, total_allocated);

    return (void*)(rp + sizeof(DbgBlock));
}

void *debug_malloc(size_t n, const char* file, int line)
{
    char *rp;
    rp = (char*)malloc(sizeof(DbgBlock)+n);
    total_allocated += n;
    ((DbgBlock*)rp)->s.n = n;
    ((DbgBlock*)rp)->s.file = file;
    ((DbgBlock*)rp)->s.line = line;

	LOGD("debug_malloc() memory = %p, allocated = %d, total_allocated = %d", rp, n, total_allocated);

    return (void*)(rp + sizeof(DbgBlock));
}

void debug_free(void *p, const char* file, int line)
{
	if (p != 0) {
		char *rp;
		rp = ((char*)p) - sizeof(DbgBlock);
		total_allocated -= ((DbgBlock*)rp)->s.n;

		const char* origin_file = ((DbgBlock*)rp)->s.file = file;
	    int origin_line = ((DbgBlock*)rp)->s.line = line;

		LOGD("debug_free() memory = %p, freed %d bytes, total_allocated = %d", rp, ((DbgBlock*)rp)->s.n, total_allocated);

		free(rp);
	}
}

} // namespace mocainput {
