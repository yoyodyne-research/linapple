#include "Common.h"
#include "font.h"
#include "main.h"

#include "chooser.h"

void ChooserDrawMessages(SDL_Surface *screen, int slot) {
	// Draw all supporting text around the file list,
	// according to the slot the selection will use.
	//
	// Draws over entire screen.
	//
	// The slot is 1-based and maps to the hardware.
	// Some slots are virtual.
	
	char *slotprompt[] = {
		"Load a savestate",                      // virtual, move me
		"Save a savestate",                      // virtual, move me
		"(look slot #2 up)",
		"(unavailable, 80-column card)",
		"(unavailable, mouse or Mockingboard",
		"(unavailable, no 800K drive)",
		"Insert a 140K floppy disk (.dsk)",
		"Connect a hard disk (.hdv)",
		"",                                      // virtual, spacer
		"Save emulator state (.awss)",           // virtual, NYI
		"Restore emulator state (.awss)",        // virtual, NYI
	};

 	// Solid dark grey background.
	SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 4, 4, 4));
	
	font_print_centered(g_ScreenWidth / 2, g_ScreenHeight * 0.05,
			    slotprompt[slot], screen);
	font_print_centered(g_ScreenWidth / 2, g_ScreenHeight * 0.9,
			    "Press ENTER to select, or ESC to cancel", screen);
}
