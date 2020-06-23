#pragma once
  /* ----------------------------------------------------------------*/
 /* ---------------------- FONT routines ---------------------------*/
/* ----------------------------------------------------------------*/
#include "SDL.h"

// dimensions of a character in the browser font bitmap
// (not the machine charset bitmap!)
#define CHAR_SIZE_X	6
#define CHAR_SIZE_Y	8
// chars in row in font bitmap
#define CHARS_IN_ROW	45

extern SDL_Surface *font_sfc;

bool fonts_initialization(void);
void fonts_termination(void);
void font_print(int x,int y,const char *text,SDL_Surface *surface);
void font_print_right(int x,int y,const char *text,SDL_Surface *surface);
void font_print_centered(int x, int y, const char *text, SDL_Surface *surface);
float font_set_scale(float s);
