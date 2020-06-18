
#include <cstring>
#include <getopt.h>
#include <stdio.h>

#include "cli.h"

void printHelp() {
  printf("usage: gala [options]\n"
         "\n"
         "Gala is an emulator of the Apple ][, Apple ][+, Apple //e,\n"
	 "and Enhanced Apple //e family of computers.\n"
         "\n"
         "  -h|--help          show this help message\n"
         "  --drive1 FILE      insert disk image FILE into first drive\n"
         "  --drive2 FILE      insert disk image FILE into second drive\n"
         "  -b|--boot          boot/reset at startup\n"
         "  -f|--fullscreen    start fullscreen\n"
         "  --log              write log to 'AppleWin.log'\n"
         "  --benchmark        benchmark and quit\n"
         "\n");
}

int parseCommandLine(int argc, char *argv[], cli_t *cli) {
  cli->enablelogging = NULL;
  cli->fullscreen = NULL;
  cli->boot = NULL;
  cli->benchmark = NULL;
  cli->conffile = NULL;
  cli->hardfile = NULL;
  cli->imagefile1 = NULL;
  cli->imagefile2 = NULL;
  cli->statefile = NULL;

  int opt;
  int longindex;
  const char *optname;
  static struct option longopts[] = {{"boot", 0, 0, 0},
                                     {"conf", required_argument, 0, 0},
                                     {"drive1", required_argument, 0, 0},
                                     {"drive2", required_argument, 0, 0},
                                     {"fullscreen", required_argument, 0, 0},
                                     {"help", 0, 0, 0},
                                     {"state", required_argument, 0, 0},
                                     {0, 0, 0, 0}};

  while ((opt = getopt_long(argc, argv, "1:2:afhlr:", longopts, &longindex)) !=
         -1) {
    switch (opt) {
    case '1':
      cli->imagefile1 = optarg;
      break;

    case '2':
      cli->imagefile2 = optarg;
      break;

    case 'f':
      cli->fullscreen = "true";
      break;

    case 'h':
      printHelp();
      return ERROR_NONE;

    case 'l':
      cli->enablelogging = "true";
      break;

    default:
      optname = longopts[longindex].name;
      printf("%d %d %s\n", optind, longindex, optname);

      if (!strcmp(optname, "boot")) {
        cli->boot = "true";

      } else if (!strcmp(optname, "benchmark")) {
        cli->benchmark = "true";

      } else if (!strcmp(optname, "conf")) {
        cli->conffile = optarg;

      } else if (!strcmp(optname, "drive1")) {
        cli->imagefile1 = optarg;

      } else if (!strcmp(optname, "drive2")) {
        cli->imagefile2 = optarg;

      } else if (!strcmp(optname, "fullscreen")) {
        cli->fullscreen = optarg;

      } else if (!strcmp(optname, "help")) {
        printHelp();
        return ERROR_NONE;

      } else if (!strcmp(optname, "state")) {
        cli->statefile = optarg;
      }
    }
  }

  // The remainder are arguments proper.
  int floppycount = 0;
  for (int index = longindex; index < argc; index++) {
    char *value = argv[index];
    if (strlen(value) > 4 && !strcmp(value + strlen(value) - 4, ".dsk")) {
      if (floppycount)
	cli->imagefile2 = value;
      else
	cli->imagefile1 = value;
      floppycount++;
    } else if (strlen(value) > 5 && !strcmp(value + strlen(value) - 5, ".conf"))
      cli->conffile = value;
    else if (strlen(value) > 5 && !strcmp(value + strlen(value) - 5, ".awss"))
      cli->statefile = value;
    else if (strlen(value) > 5 && !strcmp(value + strlen(value) - 4, ".hdv"))
      cli->hardfile = value;
    else
      return ERROR_UNKNOWN_FILETYPE;
  }

  return ERROR_NONE;
}
