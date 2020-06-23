#pragma once

#include "SDL.h"

// how many file names we are able to see at once!
#define FILES_IN_SCREEN		21

// delay after key pressed (in milliseconds??)
#define KEY_DELAY		25

// file sizes that indicate navigation
#define TOKEN_DIR "DIR"
#define TOKEN_UP "UP"

void ChooserDrawMessages(SDL_Surface *s, int slot);
