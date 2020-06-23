#pragma once

///////////////////////////////////////////////////////////////////////////
////// Some auxiliary drawing functions //////////////
///////////////////////////////////////////////////////////////////////////
#include "SDL.h"

void surface_fader(SDL_Surface *surface,float r_factor,float g_factor,float b_factor,float a_factor,SDL_Rect *r);
void putpixel(SDL_Surface *surface, int x, int y, Uint32 pixel);
void rectangle(SDL_Surface *surface, int x, int y, int w, int h, Uint32 pixel);
