/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2004 Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    Sam Lantinga
    slouken@libsdl.org
*/


/* This a stretch blit implementation based on ideas given to me by
   Tomasz Cejner - thanks! :)

   April 27, 2000 - Sam Lantinga
*/


/* This isn't ready for general consumption yet - it should be folded
   into the general blitting mechanism.
*/

//static unsigned char copy_row[4096];

#include "stdafx.h"
#include <SDL_image.h>


#define DEFINE_COPY_ROW(name, type)			\
void name(type *src, int src_w, type *dst, int dst_w)	\
{							\
	int i;						\
	int pos, inc;					\
	type pixel = 0;					\
							\
	pos = 0x10000;					\
	inc = (src_w << 16) / dst_w;			\
	for ( i=dst_w; i>0; --i ) {			\
		while ( pos >= 0x10000L ) {		\
			pixel = *src++;			\
			pos -= 0x10000L;		\
		}					\
		*dst++ = pixel;				\
		pos += inc;				\
	}						\
}
// the same as above, but pixels ORed with the destination
#define DEFINE_COPY_ROW_OR(name, type)			\
void name(type *src, int src_w, type *dst, int dst_w)	\
{							\
	int i;						\
	int pos, inc;					\
	type pixel = 0;					\
							\
	pos = 0x10000;					\
	inc = (src_w << 16) / dst_w;			\
	for ( i=dst_w; i>0; --i ) {			\
		while ( pos >= 0x10000L ) {		\
			pixel = *src++;			\
			pos -= 0x10000L;		\
}					\
		*dst++ |= pixel;			\
		pos += inc;				\
}						\
}

DEFINE_COPY_ROW(copy_row1, Uint8)
DEFINE_COPY_ROW(copy_row2, Uint16)
DEFINE_COPY_ROW(copy_row4, Uint32)

DEFINE_COPY_ROW_OR(copy_row_or1, Uint8)
DEFINE_COPY_ROW_OR(copy_row_or2, Uint16)
DEFINE_COPY_ROW_OR(copy_row_or4, Uint32)

/* The ASM code doesn't handle 24-bpp stretch blits */
void copy_row3(Uint8 *src, int src_w, Uint8 *dst, int dst_w)
{
	int i;
	int pos, inc;
	Uint8 pixel[3];

	pos = 0x10000;
	inc = (src_w << 16) / dst_w;
	for ( i=dst_w; i>0; --i ) {
		while ( pos >= 0x10000L ) {
			pixel[0] = *src++;
			pixel[1] = *src++;
			pixel[2] = *src++;
			pos -= 0x10000L;
		}
		*dst++ = pixel[0];
		*dst++ = pixel[1];
		*dst++ = pixel[2];
		pos += inc;
	}
}

/* Perform a stretch blit between two surfaces of the same format.
   NOTE:  This function is not safe to call from multiple threads!
*/
int SDL_SoftStretchMy(SDL_Surface *src, SDL_Rect *srcrect,
                    SDL_Surface *dst, SDL_Rect *dstrect)
{
	int src_locked;
	int dst_locked;
	int pos, inc;
	int dst_width;
	int dst_maxrow;
	int src_row, dst_row;
	Uint8 *srcp = NULL;
	Uint8 *dstp;
	SDL_Rect full_src;
	SDL_Rect full_dst;
	const int bpp = dst->format->BytesPerPixel;

 	if ( src->format->BitsPerPixel != dst->format->BitsPerPixel ) {
 		SDL_SetError("Only works with same format surfaces");
 		return(-1);
 	}

	/* Verify the blit rectangles */
	if ( !srcrect ){
		full_src.x = 0;
		full_src.y = 0;
		full_src.w = src->w;
		full_src.h = src->h;
		srcrect = &full_src;
	}
	if ( !dstrect ){
		full_dst.x = 0;
		full_dst.y = 0;
		full_dst.w = dst->w;
		full_dst.h = dst->h;
		dstrect = &full_dst;
	}

	/* Lock the destination if it's in hardware */
	dst_locked = 0;
	if ( SDL_MUSTLOCK(dst) ) {
		if(SDL_LockSurface(dst) < 0) return -1;
		dst_locked = 1;
	}
	/* Lock the source if it's in hardware */
	src_locked = 0;
	if ( SDL_MUSTLOCK(src) ) {
		if ( SDL_LockSurface(src) < 0 ) {
			if ( dst_locked ) SDL_UnlockSurface(dst);
//			SDL_SetError("Unable to lock source surface");
			return(-1);
		}
		src_locked = 1;
	}

	/* Set up the data... */
	pos = 0x10000;
	inc = (srcrect->h << 16) / dstrect->h;
	src_row = srcrect->y;
	dst_row = dstrect->y;
	dst_width = dstrect->w*bpp;


	/* Perform the stretch blit */
	for ( dst_maxrow = dst_row+dstrect->h; dst_row<dst_maxrow; ++dst_row ) {
		dstp = (Uint8 *)dst->pixels + (dst_row*dst->pitch)
		                            + (dstrect->x*bpp);
		while ( pos >= 0x10000L ) {
			srcp = (Uint8 *)src->pixels + (src_row*src->pitch)
			                            + (srcrect->x*bpp);
			++src_row;
			pos -= 0x10000L;
		}
		switch (bpp) {
		    case 1:
				copy_row1(srcp, srcrect->w, dstp, dstrect->w);
				break;
		    case 2:
				copy_row2((Uint16 *)srcp, srcrect->w,
				    (Uint16 *)dstp, dstrect->w);
				break;
		    case 3:
				copy_row3(srcp, srcrect->w, dstp, dstrect->w);
				break;
		    case 4:
				copy_row4((Uint32 *)srcp, srcrect->w,
			          (Uint32 *)dstp, dstrect->w);
				break;
		}
		pos += inc;
	}

	/* We need to unlock the surfaces if they're locked */
	if ( dst_locked ) {
		SDL_UnlockSurface(dst);
	}
	if ( src_locked ) {
		SDL_UnlockSurface(src);
	}
	return(0);
}


////////////////////////////////////////////////////////////////////////////////////////////
//		 MONO8
////////////////////////////////////////////////////////////////////////////////////////////
/* Perform a monochrome stretch blit between two surfaces of the 8bpp format!
   NOTE:  This function is not safe to call from multiple threads!
*/
void copy8mono(Uint8 *src, int src_w, Uint8 *dst, int dst_w, Uint8 brush)
{
	int i;
	int pos, inc;
	Uint8 pixel = 0;
	pos = 0x10000;
	inc = (src_w << 16) / dst_w;
	for ( i=dst_w; i>0; --i ) {
		while ( pos >= 0x10000L ) {
			pixel = *src++;
			pos -= 0x10000L;
		}
		//*dst++ = pixel;
		if(pixel) *dst++ = brush;
			else *dst++ = 0;
		pos += inc;
	}
}

int SDL_SoftStretchMono8(SDL_Surface *src, SDL_Rect *srcrect,
		    SDL_Surface *dst, SDL_Rect *dstrect, Uint8 brush)
{//brush - monochrome color (index from palette)
	int src_locked;
	int dst_locked;
	int pos, inc;
	int dst_width;
	int dst_maxrow;
	int src_row, dst_row;
	Uint8 *srcp = NULL;
	Uint8 *dstp;
	SDL_Rect full_src;
	SDL_Rect full_dst;
	const int bpp = dst->format->BytesPerPixel;

	/* Verify the blit rectangles */
	if ( !srcrect ){
		full_src.x = 0;
		full_src.y = 0;
		full_src.w = src->w;
		full_src.h = src->h;
		srcrect = &full_src;
	}
	if ( !dstrect ){
		full_dst.x = 0;
		full_dst.y = 0;
		full_dst.w = dst->w;
		full_dst.h = dst->h;
		dstrect = &full_dst;
	}

	/* Lock the destination if it's in hardware */
	dst_locked = 0;
	if ( SDL_MUSTLOCK(dst) ) {
		if(SDL_LockSurface(dst) < 0) return -1;
		dst_locked = 1;
	}
	/* Lock the source if it's in hardware */
	src_locked = 0;
	if ( SDL_MUSTLOCK(src) ) {
		if ( SDL_LockSurface(src) < 0 ) {
			if ( dst_locked ) SDL_UnlockSurface(dst);
//			SDL_SetError("Unable to lock source surface");
			return(-1);
		}
		src_locked = 1;
	}

	/* Set up the data... */
	pos = 0x10000;
	inc = (srcrect->h << 16) / dstrect->h;
	src_row = srcrect->y;
	dst_row = dstrect->y;
	dst_width = dstrect->w*bpp;


	/* Perform the stretch blit */
	for ( dst_maxrow = dst_row+dstrect->h; dst_row<dst_maxrow; ++dst_row ) {
		dstp = (Uint8 *)dst->pixels + (dst_row*dst->pitch)
				+ (dstrect->x*bpp);
		while ( pos >= 0x10000L ) {
			srcp = (Uint8 *)src->pixels + (src_row*src->pitch)
					+ (srcrect->x*bpp);
			++src_row;
			pos -= 0x10000L;
		}
		switch (bpp) {
			case 1:
				copy8mono(srcp, srcrect->w, dstp, dstrect->w, brush);
				 break;
			default: break;
/*			case 2:
				copy_row2((Uint16 *)srcp, srcrect->w,
					   (Uint16 *)dstp, dstrect->w);
				break;
			case 3:
				copy_row3(srcp, srcrect->w, dstp, dstrect->w);
				break;
			case 4:
				copy_row4((Uint32 *)srcp, srcrect->w,
					   (Uint32 *)dstp, dstrect->w);
				break;*/
		}
		pos += inc;
	}

	/* We need to unlock the surfaces if they're locked */
	if ( dst_locked ) {
		SDL_UnlockSurface(dst);
	}
	if ( src_locked ) {
		SDL_UnlockSurface(src);
	}
	return(0);
}
///////////////////////////////////////////////////////////////////////////////////////////////
/*	SDL_SoftStretchOr	- the same as SDL_SoftStretch, but ORed with destination
	NOTE: 24bpp does not support	-- beom beotiger 2007 November
*/

/* Perform a stretch blit between two surfaces of the same format.
   NOTE:  This function is not safe to call from multiple threads!
*/
int SDL_SoftStretchOr(SDL_Surface *src, SDL_Rect *srcrect,
		    SDL_Surface *dst, SDL_Rect *dstrect)
{
	int src_locked;
	int dst_locked;
	int pos, inc;
	int dst_width;
	int dst_maxrow;
	int src_row, dst_row;
	Uint8 *srcp = NULL;
	Uint8 *dstp;
	SDL_Rect full_src;
	SDL_Rect full_dst;
	const int bpp = dst->format->BytesPerPixel;

 	if ( src->format->BitsPerPixel != dst->format->BitsPerPixel ) {
 		SDL_SetError("Only works with same format surfaces");
 		return(-1);
 	}

	/* Verify the blit rectangles */
	if ( !srcrect ){
		full_src.x = 0;
		full_src.y = 0;
		full_src.w = src->w;
		full_src.h = src->h;
		srcrect = &full_src;
	}
	if ( !dstrect ){
		full_dst.x = 0;
		full_dst.y = 0;
		full_dst.w = dst->w;
		full_dst.h = dst->h;
		dstrect = &full_dst;
	}

	/* Lock the destination if it's in hardware */
	dst_locked = 0;
	if ( SDL_MUSTLOCK(dst) ) {
		if(SDL_LockSurface(dst) < 0) return -1;
		dst_locked = 1;
	}
	/* Lock the source if it's in hardware */
	src_locked = 0;
	if ( SDL_MUSTLOCK(src) ) {
		if ( SDL_LockSurface(src) < 0 ) {
			if ( dst_locked ) SDL_UnlockSurface(dst);
//			SDL_SetError("Unable to lock source surface");
			return(-1);
		}
		src_locked = 1;
	}

	/* Set up the data... */
	pos = 0x10000;
	inc = (srcrect->h << 16) / dstrect->h;
	src_row = srcrect->y;
	dst_row = dstrect->y;
	dst_width = dstrect->w*bpp;


	/* Perform the stretch blit */
	for ( dst_maxrow = dst_row+dstrect->h; dst_row<dst_maxrow; ++dst_row ) {
		dstp = (Uint8 *)dst->pixels + (dst_row*dst->pitch)
				+ (dstrect->x*bpp);
		while ( pos >= 0x10000L ) {
			srcp = (Uint8 *)src->pixels + (src_row*src->pitch)
					+ (srcrect->x*bpp);
			++src_row;
			pos -= 0x10000L;
		}
		switch (bpp) {
			case 1:
				copy_row_or1(srcp, srcrect->w, dstp, dstrect->w);
				break;
			case 2:
				copy_row_or2((Uint16 *)srcp, srcrect->w,
					   (Uint16 *)dstp, dstrect->w);
				break;
			case 3:
				copy_row3(srcp, srcrect->w, dstp, dstrect->w);
				break;
			case 4:
				copy_row_or4((Uint32 *)srcp, srcrect->w,
					   (Uint32 *)dstp, dstrect->w);
				break;
		}
		pos += inc;
	}

	/* We need to unlock the surfaces if they're locked */
	if ( dst_locked ) {
		SDL_UnlockSurface(dst);
	}
	if ( src_locked ) {
		SDL_UnlockSurface(src);
	}
	return(0);
}
