#pragma once

#include "SDL/SDL.h"

// delay after key pressed (in milliseconds??)
#define KEY_DELAY 25

bool ChooseAnImage(int sx, int sy, char *incoming_dir, int slot,
                   char **filename, bool *isdir, int *index_file);
int DiskChooseMaxEntries(SDL_Rect fm);
void DiskChoosePrintEntry(char *filename, const char *filesize, int y, SDL_Rect fm, double fs);
void DiskChoosePrintHeader(char *dir_text, int slot, SDL_Rect fm, double fs);
void DiskChoosePrintPrompt(SDL_Rect fm, double fs);
