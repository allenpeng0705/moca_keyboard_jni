
#ifndef __load_file_h__
#define __load_file_h__

namespace mocainput {
	void* load_bin_file(const char* fileName, int& size);
	int   file_size(FILE* file);
	bool  file_exist(const char* fileName);
}

#endif // __load_file_h__
