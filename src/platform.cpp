
#include <sys/stat.h>
#include <stdio.h>
#include "wincompat.h"
#include "platform.h"

int getstat(char *catalog, char *fname, int * size)
{
	// gets file status and returns: 0 - special or error, 1 - file is a directory, 2 - file is a normal file
	// In: catalog - working directory, fname - file name
	struct stat info;
	char tempname[MAX_PATH];

	snprintf(tempname, MAX_PATH, "%s/%s", catalog, fname);	// get full path for the file
	if(stat(tempname, &info) == -1) return 0;
	if(S_ISDIR(info.st_mode)) return 1;	// seems to be directory
	if(S_ISREG(info.st_mode)) {
		if(size != NULL) *size = (int)(info.st_size / 1024);	// get file size in Kbytes?!
		return 2;	// regular file
	}

	return 0;
}
