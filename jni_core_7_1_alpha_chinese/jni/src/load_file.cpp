
#include "Log.h"

#include <assert.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "mem_alloc.h"

namespace mocainput {


bool file_exist(const char* fileName)
{
    struct stat statBuf;
    if (stat(fileName, &statBuf) < 0) {
        return false;
    }
    return true;
}

int file_size(FILE* file)
{
    int size = 0;
    long pos = 0;

    if (file == 0) {
        return 0;
    }

    // save the current position
    pos = ftell(file);
    if (pos < 0) {
        return 0;
    }

    // get the file size
    fseek(file, 0, SEEK_SET);
    fseek(file, 0, SEEK_END);
    size = (int)ftell(file);

    // reset to its original position
    fseek(file, pos, SEEK_SET);

    return size;
}
void* load_bin_file(const char* fileName, int& size)
{
    char *data = 0;
    FILE* file;

    size = 0;

	LOGV("load_bin_file(%s)...", fileName);

    file = fopen(fileName, "rb");
    if (file == NULL) {
    	LOGE("load_bin_file(%s)...open - error(%d)", fileName, errno);
    	return 0;
    }

    if ((size = file_size(file)) == 0) {
    	fclose(file);
    	return 0;
    }

    if ((data = (char*) MALLOC(size)) == 0) {
    	fclose(file);
    	return 0;
    }

    if (fread(data, 1, size, file) != (size_t)size) {
    	FREE(data);
    	data = 0;

    	LOGE("load_bin_file(%s)...fread - error(%s)", fileName, strerror(errno));
    }

	fclose(file);

	LOGV("load_bin_file(%s)...size = %d", fileName, size);

    return data;
}

} // namespace mocainput {
