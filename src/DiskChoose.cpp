/*
////////////////////////////////////////////////////////////////////////////
////////////  Choose disk image for given slot number? ////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
//// Adapted for linapple - apple][ emulator for Linux by beom beotiger Nov 2007   /////
///////////////////////////////////////////////////////////////////////////////////////
// Original source from one of Brain Games (http://www.braingames.getput.com)
//  game Super Transball 2.(http://www.braingames.getput.com/stransball2/default.asp)
//
// Brain Games crew creates brilliant retro-remakes! Please visit their site to find out more.
//
*/
#ifdef _WIN32
#include "windows.h"
#else
#include <stddef.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include "ctype.h"
#endif

#include "stdafx.h"

#include "list.h"

#include "DiskChoose.h"
#include <errno.h>
//#include "ctype.h"

//#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>
//#include "SDL/SDL.h"
//#include "SDL_mixer.h"
//#include "SDL_image.h"

/*


#include "auxiliar.h"

#include "tiles.h"
#include "maps.h"
#include "transball.h"

#include "encoder.h"
*/



// how many file names we are able to see at once!
#define FILES_IN_SCREEN    21

// delay after key pressed (in milliseconds??)
#define KEY_DELAY    25

#define MAX_FILENAME    36

char *g_sDiskChooseMessages[] = {"Load state",
                                 "Save state",
                                 "",
                                 "",
                                 "",
                                 "Insert image into 800KB floppy drive",
                                 "Insert image into 140KB floppy drive",
                                 "Attach image as hard disk"};

/////////////////////////////////////////////////////////////////////
/* FONT prev decls */
//bool fonts_initialization(void);
//void font_print(int x,int y,char *text,SDL_Surface *surface);
//void font_print_centered(int x,int y, char *text, SDL_Surface *surface);

/////////////////////////////////////////////////////////////////////
/* AUX funcs prev decls */

//void rectangle(SDL_Surface *surface, int x, int y, int w, int h, Uint32 pixel);
//void surface_fader(SDL_Surface *surface,float r_factor,float g_factor,float b_factor,float a_factor,SDL_Rect *r);


int DiskChooseMaxEntries(SDL_Rect fm) {
  /* return  max number of lines displayable in chooser */
  return g_ScreenHeight / fm.y - 7;
}

void DiskChoosePrintHeader(char *dir_text, int slot, SDL_Rect fm, double fs) {
  /* purpose for choosing file */
  font_print_centered(g_ScreenWidth / 2, 2 * fm.y, g_sDiskChooseMessages[slot],
                      screen, fs, fs);
  /* directory location */
  font_print_centered(g_ScreenWidth / 2, 4 * fm.y, dir_text, screen, fs, fs);
}

void DiskChoosePrintEntry(char *filename, const char *filesize, int y,
                          SDL_Rect fm, double fs) {
  /* print file name
   * fm  font metrics advance leading width height (pre-scaled)
   * fs  font scale
   */
  int margin = 2 * fm.x;
  unsigned int maxnamelength = g_ScreenWidth / fm.x - 10; // units: characters

  // truncate a long name
  int eos = 0;
  if (strlen(filename) > maxnamelength) {
    eos = filename[maxnamelength];
    filename[maxnamelength] = 0;
  }

  // show name
  font_print(margin, y, filename, screen, fs, fs);

  // show info (dir or size), right justified
  int x = g_ScreenWidth - strlen(filesize) * fm.w - margin;
  font_print(x, y, filesize, screen, fs, fs);

  // restore stomped char
  if (eos)
    filename[46] = eos;
}

void DiskChoosePrintPrompt(SDL_Rect fm, double fs) {
  font_print_centered(g_ScreenWidth / 2, g_ScreenHeight - 2 * fm.h,
                      "ENTER to choose, ESC to cancel", screen, fs, fs);
}

////////////////////////////////////////////////////////////////////////////////////////
#ifndef _WIN32
int getstat(char *catalog, char *fname, int * size)
{
  // gets file status and returns: 0 - special or error, 1 - file is a directory, 2 - file is a normal file
  // In: catalog - working directory, fname - file name
  struct stat info;
  char tempname[MAX_PATH];

  snprintf(tempname, MAX_PATH, "%s/%s", catalog, fname);  // get full path for the file
  if(stat(tempname, &info) == -1) return 0;
  if(S_ISDIR(info.st_mode)) return 1;  // seems to be directory
  if(S_ISREG(info.st_mode)) {
    if(size != NULL)
            *size = (int)(info.st_size / 1024);  // get file size in Kbytes?!
    return 2;  // regular file
  }

  return 0;
}
#endif
////////////////////////////////////////////////////////////////////////////////////////
bool ChooseAnImage(int sx, int sy, char *incoming_dir, int slot,
                   char **filename, bool *isdir, int *index_file) {
  /*  Input:
 sx, sy - window size,
 incoming_dir - in what dir find files,
      slot         - in what slot should an image go;
      #5 for 800Kb floppy disks,
      #6 for 5.25' 140Kb floppy disks,
      #7 for hard-disks.
      index_file   - from which file we should start cursor
      (should be static and 0 when changing dir)

      Output:
      filename  - chosen file name (or dir name)
  isdir    - if chosen name is a directory
*/

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

  /* Surface: */
  SDL_Surface *my_screen;  // for background

  if(font_sfc == NULL)
    if(!fonts_initialization()) return false;  //if we don't have a fonts, we just can do none

  List<char> files;    // our files
  List<char> sizes;    // and their sizes (or 'dir' for directories)

  int act_file;    // current file
  int first_file;    // from which we output files

//  printf("[disk  ] cd %s\n", incoming_dir);

//  files.Delete();
//  sizes.Delete();
#ifndef _WIN32
/* POSIX specific routines of reading directory structure */
    DIR *dp;

    dp = opendir (incoming_dir);  // open and read incoming directory
    char *tmp;

    int i;  // for cycles, beginning and end of list

// build prev dir
  if(strcmp(incoming_dir, "/")) {
#if 0  
    files.Add("..");
    sizes.Add("<UP>");  // add sign of directory
#else
    tmp = new char[3];
    strcpy(tmp, "..");
    files.Add(tmp);
    tmp = new char[5];
    strcpy(tmp, "<UP>");
    sizes.Add(tmp);  // add sign of directory
#endif    
  }
    if (dp != NULL)
    {
            {
                struct dirent **namelist;
                int n;

                n = scandir(incoming_dir, &namelist, NULL, alphasort);
                if (n < 0)
                    perror("scandir");
                else {
                    i = 0;
                    while (i < n) {
                        int what = getstat(incoming_dir, namelist[i]->d_name, NULL);
                        if (namelist[i]->d_name && strlen(namelist[i]->d_name) > 0 &&
                            namelist[i]->d_name[0] != '.' && what == 1) // is directory!
                        {
                            tmp = new char[strlen(namelist[i]->d_name)+1];  // add this entity to list
                            strcpy(tmp, namelist[i]->d_name);
                            files.Add(tmp);
                            tmp = new char[6];
                            strcpy(tmp, "<DIR>");
                            sizes.Add(tmp);  // add sign of directory
                        }
                        i++;
                    }
                    free(namelist);
               }
            }
    }
#else
/* Windows specific functions of reading directory structure */
    /* Find subdirs: */
  if(strcmp(incoming_dir, "/")) {
  // we are not in upper direcory
    tmp = new char[3];
    strcpy(tmp, "..");
    files.Add(tmp);
    tmp = new char[5];
    strcpy(tmp, "<UP>");
    sizes.Add(tmp);  // add sign of directory
  }


    WIN32_FIND_DATA finfo;
    HANDLE h;



    h=FindFirstFile(incoming_dir,&finfo);

    if (h!=INVALID_HANDLE_VALUE) {
      char *tmp;
      if(finfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY != 0) {
      // add this entry if it is directory
        tmp=new char[strlen(finfo.cFileName)+1];
        strcpy(tmp,finfo.cFileName);
        files.Add(tmp);
        tmp = new char[6];
        strcpy(tmp, "<DIR>");
        sizes.Add(tmp);  // add sign of directory
      }
      while(FindNextFile(h,&finfo)==TRUE) {
        if(finfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY != 0) {
        // add this entry if it is directory
          tmp=new char[strlen(finfo.cFileName)+1];
          strcpy(tmp,finfo.cFileName);
          files.Add(tmp);
          tmp = new char[6];
          strcpy(tmp, "<DIR>");
          sizes.Add(tmp);  // add sign of directory
        }
      } /* while */
    } /* if */

#endif
#ifndef _WIN32
/* POSIX specific routines of reading directory structure */

      if( dp ) {
        rewinddir (dp);  // to the start
              {
                  struct dirent **namelist;
                  int n, fsize = 0;

                  n = scandir(incoming_dir, &namelist, NULL, alphasort);
                  if (n < 0)
                      perror("scandir");
                  else {
                      int i = 0;
                      while (i < n) {
                          if (strlen(namelist[i]->d_name) > 4 && namelist[i]->d_name[0] != '.'
                              && (getstat(incoming_dir, namelist[i]->d_name, &fsize) == 2)) // is normal file!
                          {
                              tmp = new char[strlen(namelist[i]->d_name)+1];  // add this entity to list
                              strcpy(tmp, namelist[i]->d_name);
                              files.Add(tmp);
                              tmp = new char[10];  // 1400000KB
                              if (1000 > fsize) {
                                  snprintf(tmp, 9, "%4dK", fsize);
                              } else if (1000000 > fsize) {
                                  snprintf(tmp, 9, "%4dM", (int) (fsize / 1000));
                              } else {
                                  snprintf(tmp, 9, "%4dG", (int) (fsize / 1000000));
                              }
                              sizes.Add(tmp);  // add this size to list
                          }
                          i++;
                      }
                      free(namelist);
                 }
              }

        closedir (dp);
      }
#else
/* Windows specific functions of reading directory structure */
    /* Find files: */

    h=FindFirstFile(incoming_dir,&finfo);

    if (h!=INVALID_HANDLE_VALUE) {
//      char *tmp; - must be defined in previous section, when searching subdirs
      if(finfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY == 0) {
      // add this entry if it is NOT directory!
        tmp=new char[strlen(finfo.cFileName)+1];
        strcpy(tmp,finfo.cFileName);
        files.Add(tmp);
        tmp = new char[10];  // 1400000KB
        snprintf(tmp, 9, "%dKB",
          ((finfo.nFileSizeHigh * (MAXDWORD+1)) + finfo.nFileSizeLow));
        sizes.Add(tmp);  // add this size to list
      }
      while(FindNextFile(h,&finfo)==TRUE) {
        if(finfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY == 0) {
        // add this entry if it is NOT directory!
          tmp=new char[strlen(finfo.cFileName)+1];
          strcpy(tmp,finfo.cFileName);
          files.Add(tmp);
          tmp = new char[10];  // 1400000KB
          snprintf(tmp, 9, "%dKB",
            ((finfo.nFileSizeHigh * (MAXDWORD+1)) + finfo.nFileSizeLow));
          sizes.Add(tmp);  // add this size to list
        }
      } /* while */
    } /* if */

#endif
// do sorting for files

//  Count out cursor position and file number output
  act_file = *index_file;
  if (act_file >= files.Length())
    act_file = 0; // cannot be more than files in list
  first_file = act_file - (files_in_screen / 2);
  if (first_file < 0)
    first_file = 0; // cannot be negativ...

// Show all directories (first) and files then
//  char *tmp;
  char *siz = NULL;
//  int i;

// prepare screen
  
  SDL_Surface *tempSurface = NULL;
  if(!g_WindowResized) {
    if(g_nAppMode == MODE_LOGO) tempSurface = g_hLogoBitmap;  // use logobitmap
      else tempSurface = g_hDeviceBitmap;
  }
  else tempSurface = g_origscreen;

  if(tempSurface == NULL)
    tempSurface = screen;  // use screen, if none available

  my_screen = SDL_CreateRGBSurface(SDL_SWSURFACE, tempSurface->w, tempSurface->h, tempSurface->format->BitsPerPixel, 0, 0, 0, 0);
  if(tempSurface->format->palette && my_screen->format->palette)
    SDL_SetColors(my_screen, tempSurface->format->palette->colors,
            0, tempSurface->format->palette->ncolors);

  surface_fader(my_screen, 0.2F, 0.2F, 0.2F, -1, 0);  // fade it out to 20% of normal
  SDL_BlitSurface(tempSurface, NULL, my_screen, NULL);

  while(true)
  {

    SDL_BlitSurface(my_screen, NULL, screen, NULL);    // show background

    DiskChoosePrintHeader(incoming_dir, slot, fm, fs);

    files.Rewind();  // from start
    sizes.Rewind();
    i = 0;

//    printf("We've printed some messages, go to file list!\n");
// show all fetched dirs and files
// topX of first fiel visible

    while(files.Iterate(tmp)) {

      sizes.Iterate(siz);  // also fetch size string

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
      i++;    // next item
    } /* while */

    DiskChoosePrintPrompt(fm, fs);

  SDL_Flip(screen);  // show the screen
  SDL_Delay(KEY_DELAY);  // wait some time to be not too fast

  //////////////////////////////////
  // Wait for keypress
  //////////////////////////////////
  SDL_Event event;  // event
  Uint8 *keyboard;  // key state

  event.type = 0;
  while(event.type != SDL_KEYDOWN) {  // wait for key pressed
        // GPH Honor quit even if we're in the diskchoose state.
        if (SDL_QUIT == event.type) {
      files.Delete();
      sizes.Delete();
      SDL_FreeSurface(my_screen);

        SDL_PushEvent(&event);// push quit event
            return false;
        }
    SDL_Delay(10);
    SDL_PollEvent(&event);
  }

// control cursor
    keyboard = SDL_GetKeyState(NULL);  // get current state of pressed (and not pressed) keys

    if (keyboard[SDLK_UP] || keyboard[SDLK_LEFT]) {
      if (act_file > 0)
        act_file--; // up one position
      if (act_file < first_file)
        first_file = act_file;
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
      *filename = strdup(files[act_file]);
//      printf("files[act_file]=%s, *filename=%s\n\n", files[act_file], *filename);
      if(!strcmp(sizes[act_file], "<DIR>") || !strcmp(sizes[act_file], "<UP>"))
             *isdir = true;
        else *isdir = false;  // this is directory (catalog in Apple][ terminology)
      *index_file = act_file;  // remember current index
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
      first_file = act_file - files_in_screen + 1;
      if (first_file < 0)
        first_file = 0;
    } /* if */

  }
} /* ChooseAnImage */
