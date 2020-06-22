
#pragma once

namespace cli {

typedef struct {
	char *executable;
	char *enablelogging;
	char *fullscreen;
	char *benchmark;
	char *conffile;
	char *hardfile;
	char *imagefile;
	char *imagefile2;
	char *statefile;
} cli_t;

enum cli_error_code {
	ERROR_NONE = 0,
	ERROR_UNKNOWN_OPTION = 1,
	ERROR_UNKNOWN_OPTARG = 2,
	ERROR_UNKNOWN_FILETYPE = 4,
	ERROR_USAGE = 128
};

int parseCommandLine(int argc, char *argv[], cli_t *cli);

}