#pragma once

#include "wincompat.h"

extern TCHAR   g_sFTPDirListing[512]; // name for FTP-directory listing

/* Choose an image using FTP */
bool DiskFTPChooseAnImage(char *ftp_dir, int slot,
			  char **filename, bool *isdir, int *index_file);
