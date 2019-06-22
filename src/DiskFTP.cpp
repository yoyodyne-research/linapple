/*
  Choose disk image for given slot number.

  Adapted for linapple - apple][ emulator for Linux by beom beotiger Nov 2007

  Original source from one of Brain Games (http://www.braingames.getput.com)
  game Super Transball
  2.(http://www.braingames.getput.com/stransball2/default.asp)

  Brain Games crew creates brilliant retro-remakes!
  Please visit their site to find out more.
*/

/* March 2012 AD by Krez, Beom Beotiger */

#include <math.h>
#include <sys/stat.h>
#include <time.h>

#include "DiskChoose.h"
#include "DiskFTP.h"
#include "SDL.h"
#include "ftpparse.h"
#include "list.h"
#include "md5.h"
#include "stdafx.h"

// how many file names we are able to see at once!
#define FILES_IN_SCREEN    21
// delay after key pressed (in milliseconds??)
#define KEY_DELAY    25
// define time when cache ftp dir.listing must be refreshed
#define RENEW_TIME  24*3600

// longest displayable site path in chars
#define FTP_DIR_MAX_LENGTH 60

/* storage for cache of FTP directory listings */
TCHAR g_sFTPDirListing[512] = TEXT("cache/ftp.");

typedef struct {
  int x;
  int y;
} SDL_Point;

typedef enum { FTPSTAT_ERROR, FTPSTAT_DIRECTORY, FTPSTAT_FILE } ftpstat_t;

int ftpstat(struct ftpparse *fp, int *size) {
  /* stat an FTP entry
   * param fp  entry to stat
   * return size  ftpstat_t
   */
  if (!fp->namelen)
    return FTPSTAT_ERROR;
  if (fp->flagtrycwd == 1)
    return 1; // can CWD, it is dir then

  if(fp->flagtryretr == 1) { // we're able to RETR, it's a file then?!
    if (size != NULL)
      *size = (int)(fp->size / 1024);
    return FTPSTAT_FILE;
  }
  return FTPSTAT_DIRECTORY;
}

void strcpy_ftp_dir(char *ftp_dir_text, const char *ftp_dir) {
  /* if possible, find string offset where site's local dir begins */
  int found = 0;
  int offset = 0;
  for (unsigned int i = 0; i < strlen(ftp_dir); i++) {
    if (ftp_dir[i] == '/') {
      found++;
    }
    if (found == 3) {
      offset = i;
      break;
    }
  }
  strncpy(ftp_dir_text, ftp_dir + offset, FTP_DIR_MAX_LENGTH);
}

bool DiskFTPChooseAnImage(char *ftp_dir, int slot, char **name, bool *isdir,
                          int *startindex) {
  /* Parameters:
   * sx, g_ScreenHeight      - windaow size,
   * ftp_dir     - what FTP directory to use,
   * slot - in what slot should an image go?
   *   5 for 3.5 800Kb floppy disks,
   *   6 for 5.25 140Kb floppy disks,
   *   7 for hard disks.
   * startindex  - from which file we should start cursor
   *   (should be static and 0 when changing dir)
   *
   * Out:
   * name        - chosen file name (or dir name)
   * isdir       - if chosen name is a directory
*/

  if (font_sfc == NULL)
    if (!fonts_initialization())
      return false;

  /* screen scale ratios */
    double facx = double(g_ScreenWidth) / double(SCREEN_WIDTH);
    double facy = double(g_ScreenHeight) / double(SCREEN_HEIGHT);

  /* uniform scaling, in scaled space, to pass to font drawing functions */
  double fs = min(facx, facy) * FONT_SCALE;

  /* store font size metrics in scaled screen space */
  SDL_Rect fm;
  fm.w = FONT_SIZE_X * fs; /* character width */
  fm.h = FONT_SIZE_Y * fs; /* character height */
  fm.x = fm.w;             /* advance */
  fm.y = fm.h * 1.25;      /* leading */

  int files_in_screen = DiskChooseMaxEntries(fm);
  
  /* substring of ftp_dir to actually display */
  char ftp_dir_text[FTP_DIR_MAX_LENGTH];

  SDL_Surface *my_screen;  // for background
  struct ftpparse FTP_PARSE; // for parsing ftp directories
  struct stat info;

  char tmpstr[512];
  char ftpdirpath [MAX_PATH];
  snprintf(ftpdirpath, MAX_PATH, "%s/%s%s", g_sFTPLocalDir, g_sFTPDirListing, md5str(ftp_dir));  // get path for FTP dir listing
//  printf("Dir: %s, MD5(dir)=%s\n",ftp_dir,ftpdirpath);

  List<char> files;    // our files
  List<char> sizes;    // and their sizes (or 'dir' for directories)

  int act_file;    // current file
  int first_file;    // from which we output files

// prepare screen
  SDL_Surface *tempSurface;

  if(!g_WindowResized) {
    if(g_nAppMode == MODE_LOGO) tempSurface = g_hLogoBitmap;  // use logobitmap
      else tempSurface = g_hDeviceBitmap;
  }
  else tempSurface = g_origscreen;

  my_screen = SDL_CreateRGBSurface(SDL_SWSURFACE, tempSurface->w, tempSurface->h, tempSurface->format->BitsPerPixel, 0, 0, 0, 0);
  if(tempSurface->format->palette && my_screen->format->palette)
    SDL_SetColors(my_screen, tempSurface->format->palette->colors,
            0, tempSurface->format->palette->ncolors);

  surface_fader(my_screen, 0.2F, 0.2F, 0.2F, -1,
                0); // fade it out to 20% of normal
  SDL_BlitSurface(tempSurface, NULL, my_screen, NULL);
  SDL_BlitSurface(my_screen, NULL, screen, NULL); // show background

  /* format a substring of the FTP path for printing */
  strcpy_ftp_dir(ftp_dir_text, ftp_dir);
  DiskChoosePrintHeader(ftp_dir_text, slot, fm, fs);

  font_print_centered(g_ScreenWidth / 2, g_ScreenHeight - 2 * fm.h,
                      "Connecting to FTP server, please wait...", screen, fs,
                      fs);
  SDL_Flip(screen); // show the screen

  bool OKI;
  if (stat(ftpdirpath, &info) == 0 && info.st_mtime > time(NULL) - RENEW_TIME) {
    OKI = false; // use this file
  } else {
    OKI = ftp_get(ftp_dir, ftpdirpath); // get ftp dir listing
  }

  if (OKI) { // error
    printf("Failed getting FTP directory %s to %s\n", ftp_dir, ftpdirpath);
    font_print_centered(g_ScreenWidth / 2, 30 * facy, "Failure! Press any key.",
                        screen, fs, fs);
    SDL_Flip(screen);     // show the screen
    SDL_Delay(KEY_DELAY); // wait some time to be not too fast

    SDL_Event event; // event
    event.type = SDL_QUIT;
    while (event.type != SDL_KEYDOWN) {
      /* wait for key pressed */
          SDL_Delay(100);
          SDL_PollEvent(&event);
    }
    SDL_FreeSurface(my_screen);    
    return false;  
   }

    FILE *fdir = fopen(ftpdirpath,"r");
  
    char *tmp;
    int i,j, B, N;  // for cycles, beginning and end of list

// build prev dir
  if(strcmp(ftp_dir, "ftp://")) {
    tmp = new char[3];
    strcpy(tmp, "..");
    files.Add(tmp);
    tmp = new char[5];
    strcpy(tmp, "<UP>");
    sizes.Add(tmp);  // add sign of directory
    B = 1;
  }
  else  B = 0;  // for sorting dirs 

      while ((tmp = fgets(tmpstr,512,fdir))) // first looking for directories
      {
        // clear and then try to fill in FTP_PARSE struct
        memset(&FTP_PARSE,0,sizeof(FTP_PARSE));
        ftpparse(&FTP_PARSE, tmp, strlen(tmp));
        
    int what = ftpstat(&FTP_PARSE, NULL);
        
        if (strlen(FTP_PARSE.name) > 0 &&  what == 1) // is directory!
        {
          tmp = new char[strlen(FTP_PARSE.name)+1];  // add entity to list
          strcpy(tmp, FTP_PARSE.name);
          files.Add(tmp);
          tmp = new char[6];
          strcpy(tmp, "<DIR>");
          sizes.Add(tmp);  // add sign of directory
        } /* if */

      }
// sort directories. Please, don't laugh at my bubble sorting - it the simplest thing I've ever seen --bb
      if(files.Length() > 2)
      {
        N = files.Length() - 1;
//        B = 1; - defined above
        for(i = N; i > B; i--)
          for(j = B; j < i; j++)
            if(strcasecmp(files[j], files[j + 1]) > 0)
            {
              files.Swap(j,j + 1);
              sizes.Swap(j,j + 1);
            }

      }
      B = files.Length();  // start for files

      (void) rewind (fdir);  // to the start
        // now get all regular files
      while ((tmp = fgets(tmpstr,512,fdir)))
      {
        int fsize;
        // clear and then try to fill in FTP_PARSE struct
        memset(&FTP_PARSE,0,sizeof(FTP_PARSE));
        ftpparse(&FTP_PARSE, tmp, strlen(tmp));

    if ((ftpstat(&FTP_PARSE, &fsize) == 2)) // is normal file!
        {
          tmp = new char[strlen(FTP_PARSE.name)+1];  // add this entity to list
          strcpy(tmp, FTP_PARSE.name);
          files.Add(tmp);
          tmp = new char[10];  // 1400000KB
          snprintf(tmp, 9, "%dKB", fsize);
          sizes.Add(tmp);  // add this size to list
        } /* if */
      }
      (void) fclose (fdir);
      // do sorting for files
      if(files.Length() > 2 && B < files.Length())
      {
        N = files.Length() - 1;
        //B = 1;
        for(i = N; i > B; i--)
          for(j = B; j < i; j++)
            if(strcasecmp(files[j], files[j + 1]) > 0)
            {
              files.Swap(j,j + 1);
              sizes.Swap(j,j + 1);
            }
      }
//  Count out cursor position and file number output
  act_file = *startindex;
  if (act_file >= files.Length())
    act_file = 0; // cannot be more than files in list
  first_file = act_file - (files_in_screen / 2);
  if (first_file < 0)
    first_file = 0; // cannot be negativ...

// Show all directories (first) and files then
  char *siz = NULL;

  while(true)
  {
    SDL_BlitSurface(my_screen, NULL, screen, NULL);    // show background

    strcpy_ftp_dir(ftp_dir_text, ftp_dir);
    DiskChoosePrintHeader(ftp_dir_text, slot, fm, fs);

    files.Rewind();  // from start
    sizes.Rewind();
    i = 0;

    while(files.Iterate(tmp)) {
      sizes.Iterate(siz);  // also fetch size string

      /* there are files_in_screen items on screen at a time */
      if (i >= first_file && i < first_file + files_in_screen) {
        unsigned int y = (i - first_file + 5) * fm.y;

        if (i == act_file) {
          /* highlight row under cursor */
          SDL_Rect r;
          r.x = 0;
          /* highlight is 4 pixels taller than text */
          r.y = y - 2;
          r.w = g_ScreenWidth;
          /* highlight is 4 pixels taller than text */
          r.h = (fm.h + 4);
          SDL_FillRect(screen, &r, SDL_MapRGB(screen->format, 63, 63, 63));
        } /* if */

        DiskChoosePrintEntry(tmp, siz, y, fm, fs);

      } /* if */
      i++;
    } /* while */

    DiskChoosePrintPrompt(fm, fs);

  SDL_Flip(screen);  // show the screen
  SDL_Delay(KEY_DELAY);  // wait some time to be not too fast

  SDL_Event event;  // event
    event.type = SDL_QUIT;
  Uint8 *keyboard;  // key state

    while (event.type != SDL_KEYDOWN) {
      /* wait for keypress */
        SDL_Delay(10);
        SDL_PollEvent(&event);
  }

// control cursor
    keyboard = SDL_GetKeyState(NULL);  // get current state of pressed (and not pressed) keys
    if (keyboard[SDLK_UP] || keyboard[SDLK_LEFT]) {
      if (act_file>0) act_file--;  // up one position
      if (act_file<first_file) first_file=act_file;
    } /* if */

    if (keyboard[SDLK_DOWN] || keyboard[SDLK_RIGHT]) {
      if (act_file < (files.Length() - 1))
        act_file++;
      if (act_file >= (first_file + files_in_screen))
        first_file = act_file - files_in_screen + 1;
    } /* if */

    if (keyboard[SDLK_PAGEUP]) {
      act_file -= files_in_screen;
      if (act_file < 0)
        act_file = 0;
      if (act_file < first_file)
        first_file = act_file;
    } /* if */

    if (keyboard[SDLK_PAGEDOWN]) {
      act_file += files_in_screen;
      if (act_file >= files.Length())
        act_file = (files.Length() - 1);
      if (act_file >= (first_file + files_in_screen))
        first_file = act_file - files_in_screen + 1;
    } /* if */

    // choose an item?
    if (keyboard[SDLK_RETURN]) {
      // dup string from selected file name
      *name = strdup(php_trim(files[act_file], strlen(files[act_file])));
      //      printf("files[act_file]=%s, *name=%s\n\n", files[act_file],
      //      *name);
      if(!strcmp(sizes[act_file], "<DIR>") || !strcmp(sizes[act_file], "<UP>"))
             *isdir = true;
      else
        *isdir = false; // this is directory (catalog in Apple][ terminology)
      *startindex = act_file; // remember current index
      files.Delete();
      sizes.Delete();
      SDL_FreeSurface(my_screen);

      return true;
    } /* if */

    if (keyboard[SDLK_ESCAPE]) {
      files.Delete();
      sizes.Delete();
      SDL_FreeSurface(my_screen);
      return false;    // ESC has been pressed
    } /* if */

    if (keyboard[SDLK_HOME]) {  // HOME?
      act_file=0;
      first_file=0;
    } /* if */
    if (keyboard[SDLK_END]) {  // END?
      act_file=files.Length() - 1;  // go to the last possible file in list
      first_file=act_file - FILES_IN_SCREEN + 1;
      if(first_file < 0) first_file = 0;
    } /* if */

  }
} /* ChooseAnImageFTP */
