  /* ----------------------------------------------------------------*/
 /* ---------------------- FONT routines ---------------------------*/
/* ----------------------------------------------------------------*/
#include "SDL_image.h"

#include "charset40.xpm"
#include "font.xpm"
#include "stretch.h"
#include "font.h"

SDL_Surface *font_sfc = NULL;	// used for font

// Uniform scaling only;
// possibly change this given better source assets.
// But first consider that it usually looks like aesthetic cheese.
float scale;

bool fonts_initialization(void)
{
        
	SDL_Surface *temp_surface;
	temp_surface = IMG_ReadXPMFromArray (font_xpm);
	font_sfc = SDL_DisplayFormat(temp_surface);
	
	SDL_FreeSurface(temp_surface);
	/* Transparant color is BLACK: */
	SDL_SetColorKey(font_sfc,SDL_SRCCOLORKEY,SDL_MapRGB(font_sfc->format,0,0,0));

	// TODO select an integral value based on imaging parameters.
	scale = 2.0;

	return true;
} /* fonts_initialization */


void fonts_termination(void)
{
	SDL_FreeSurface(font_sfc);
	font_sfc = NULL;
} /* fonts_termination */


float font_set_scale(float s) {
	float prev = scale;
	scale = s;
	return prev;
}


void font_print_char(char c, SDL_Surface *surface, int x, int y)
{
	SDL_Rect s, d;

	if(c > 127 || c < 0) c = '?';	// cut-off non-ASCII chars

	int row = c / CHARS_IN_ROW;

	s.x = (c - (row * CHARS_IN_ROW)) * (CHAR_SIZE_X + 1) + 1;
	s.y = (row) * (CHAR_SIZE_Y  + 1) + 1;
	s.w = CHAR_SIZE_X;
	s.h = CHAR_SIZE_Y;

	d.x = x;
	d.y = y;
	d.w = s.w * scale;
	d.h = s.h * scale;

	SDL_SoftStretchOr(font_sfc, &s, surface, &d);
}


void font_print(int x, int y, const char *text, SDL_Surface *surface)
{
	int advance = CHAR_SIZE_X * scale;
	for(int i = 0; text[i] != 0 && x < surface->w; i++, x += advance)
		font_print_char(text[i], surface, x, y);
}


void font_print_right(int x, int y, const char *text, SDL_Surface *surface)
{	
	x -= strlen(text) * (CHAR_SIZE_X * scale);
	font_print(x, y, text, surface);
}


void font_print_centered(int x, int y, const char *text, SDL_Surface *surface)
{
	x -= strlen(text) * (CHAR_SIZE_X * scale) / 2;
	font_print(x, y, text, surface);
}
