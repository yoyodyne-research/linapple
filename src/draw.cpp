///////////////////////////////////////////////////////////////////////////
////// Some auxiliary drawing functions //////////////
///////////////////////////////////////////////////////////////////////////
#include "draw.h"

void surface_fader(SDL_Surface *surface,float r_factor,float g_factor,float b_factor,float a_factor,SDL_Rect *r)
{

// my rebiuld for 8BPP palettized surfaces! --bb

	SDL_Rect r2;
	int i/*,x,y,offs*/;
//	Uint8 rtable[256],gtable[256],btable[256],atable[256];
	SDL_Color mycolors[256];	// faded colors
	SDL_Color *colors;
//	SDL_Surface *tmp;

	if (r==0) {
		r2.x=0;
		r2.y=0;
		r2.w=surface->w;
		r2.h=surface->h;
		r=&r2;
	} /* if */

// fading just for 8BPP surfaces!
	if (surface->format->BytesPerPixel != 1) return;

	colors = (SDL_Color *) surface->format->palette->colors; // get pointer to origin colors
	for(i=0;i<256;i++) {
		mycolors[i].r=(Uint8)(colors[i].r * r_factor);
		mycolors[i].g=(Uint8)(colors[i].g * g_factor);
		mycolors[i].b=(Uint8)(colors[i].b * b_factor);
	} /* for */

	SDL_SetColors(surface, mycolors, 0 ,256);

// 	if ((surface->flags&SDL_HWSURFACE)!=0) {
	// 		/* HARDWARE SURFACE!!!: */
// 		tmp=SDL_CreateRGBSurface(SDL_SWSURFACE,surface->w,surface->h,32,0,0,0,0);
// 		SDL_BlitSurface(surface,0,tmp,0);
// 		SDL_LockSurface(tmp);
// 		pixels = (Uint8 *)(tmp->pixels);
// 	} else {
// 		SDL_LockSurface(surface);
// 		pixels = (Uint8 *)(surface->pixels);
	// 	} /* if */
	//
// 	for(y=r->y;y<r->y+r->h && y<surface->h;y++) {
// 		for(x=r->x,offs=y*surface->pitch+r->x*4;x<r->x+r->w && x<surface->w;x++,offs+=4) {
// 			pixels[offs+ROFFSET]=rtable[pixels[offs+ROFFSET]];
// 			pixels[offs+GOFFSET]=gtable[pixels[offs+GOFFSET]];
// 			pixels[offs+BOFFSET]=btable[pixels[offs+BOFFSET]];
// 			pixels[offs+AOFFSET]=atable[pixels[offs+AOFFSET]];
	// 		} /* for */
	// 	} /* for */
	//
// 	if ((surface->flags&SDL_HWSURFACE)!=0) {
	// 		/* HARDWARE SURFACE!!!: */
// 		SDL_UnlockSurface(tmp);
// 		SDL_BlitSurface(tmp,0,surface,0);
// 		SDL_FreeSurface(tmp);
// 	} else {
// 		SDL_UnlockSurface(surface);
	// 	} /* if */


} /* surface_fader */

void putpixel(SDL_Surface *surface, int x, int y, Uint32 pixel)
{
	SDL_Rect clip;
	int bpp = surface->format->BytesPerPixel;

	SDL_GetClipRect(surface,&clip);

	if (x<clip.x || x>=clip.x+clip.w ||
		   y<clip.y || y>=clip.y+clip.h) return;

	if (x<0 || x>=surface->w ||
		   y<0 || y>=surface->h) return;

	Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;

	switch(bpp) {
		case 1:
			*p = pixel;
			break;

		case 2:
			*(Uint16 *)p = pixel;
			break;

		case 3:
			if(SDL_BYTEORDER == SDL_BIG_ENDIAN) {
				p[0] = (pixel >> 16) & 0xff;
				p[1] = (pixel >> 8) & 0xff;
				p[2] = pixel & 0xff;
			} else {
				p[0] = pixel & 0xff;
				p[1] = (pixel >> 8) & 0xff;
				p[2] = (pixel >> 16) & 0xff;
			}
			break;

		case 4:
			*(Uint32 *)p = pixel;
			break;
	}
} /* putpixel */


void rectangle(SDL_Surface *surface, int x, int y, int w, int h, Uint32 pixel)
{
	int i;

	for(i=0;i<w;i++) {
		putpixel(surface,x+i,y,pixel);
		putpixel(surface,x+i,y+h,pixel);
	} /* for */
	for(i=0;i<=h;i++) {
		putpixel(surface,x,y+i,pixel);
		putpixel(surface,x+w,y+i,pixel);
	} /* for */
} /* rectangle */
