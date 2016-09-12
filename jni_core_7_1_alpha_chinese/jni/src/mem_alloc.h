
#ifndef __MEM_ALLOC_H__
#define __MEM_ALLOC_H__

#include <stdlib.h>

namespace mocainput {

#ifdef USE_DEBUG_MEM_ALLOC

void* debug_calloc(size_t elements, size_t n, const char*, int line);
void* debug_malloc(size_t n, const char*, int line);
void debug_free(void *p, const char*, int line);

#define FREE(p) 		if ((p)) {debug_free((p), __FILE__, __LINE__);(p) = 0;}
#define CALLOC(e, n) 	debug_calloc(e, n, __FILE__, __LINE__)
#define MALLOC(n) 		debug_malloc(n, __FILE__, __LINE__)

#else

#define FREE(p) 		if ((p)) {free((p));(p) = 0;}
#define CALLOC(e, n) 	calloc(e, n)
#define MALLOC(n) 		malloc(n)

#endif

// c++ operator
#define DELETE(p) if (p) {delete p;p = 0;}

} // namespace xt9input

#endif
