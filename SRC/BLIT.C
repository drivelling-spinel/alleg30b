/*         ______   ___    ___ 
 *        /\  _  \ /\_ \  /\_ \ 
 *        \ \ \L\ \\//\ \ \//\ \      __     __   _ __   ___ 
 *         \ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\/\`'__\/ __`\
 *          \ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \ \ \//\ \L\ \
 *           \ \_\ \_\/\____\/\____\ \____\ \____ \ \_\\ \____/
 *            \/_/\/_/\/____/\/____/\/____/\/___L\ \/_/ \/___/
 *                                           /\____/
 *                                           \_/__/
 *      By Shawn Hargreaves,
 *      1 Salisbury Road,
 *      Market Drayton,
 *      Shropshire,
 *      England, TF9 1AJ.
 *
 *      Blitting functions.
 *
 *      See readme.txt for copyright information.
 */


#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <limits.h>
#include <sys/farptr.h>
#include <sys/segments.h>

#include "allegro.h"
#include "internal.h"



/* get_bitmap_addr:
 *  Helper function for deciding which way round to do a blit. Returns
 *  an absolute address corresponding to a pixel in a bitmap, converting
 *  banked modes into a theoretical linear-style address.
 */
static inline unsigned long get_bitmap_addr(BITMAP *bmp, int x, int y)
{
   unsigned long ret;

   ret = (unsigned long)bmp->line[y];

   if (is_planar_bitmap(bmp))
      ret *= 4;

   ret += x;

   if (bmp->write_bank != _stub_bank_switch)
      ret += _gfx_bank[y+bmp->line_ofs] * gfx_driver->bank_size;

   return ret;
}



/* blit_from_256:
 *  Expand 256 color images onto a truecolor destination.
 */
static void blit_from_256(BITMAP *src, BITMAP *dest, int s_x, int s_y, int d_x, int d_y, int w, int h)
{
   int col256[256];
   int x, y, c;
   unsigned long s, d;
   unsigned char *ss;

   for (c=0; c<256; c++)
      col256[c] = makecol_depth(bitmap_color_depth(dest),
				_rgb_scale_6[_current_pallete[c].r], 
				_rgb_scale_6[_current_pallete[c].g], 
				_rgb_scale_6[_current_pallete[c].b]);

   switch (bitmap_color_depth(dest)) {

   #ifdef ALLEGRO_COLOR16

      case 15:
      case 16:
	 /* 256 -> hicolor */
	 if (src->seg == _my_ds()) {
	    _farsetsel(dest->seg);

	    for (y=0; y<h; y++) {
	       ss = src->line[s_y+y] + s_x;
	       d = bmp_write_line(dest, d_y+y) + d_x*2;

	       for (x=0; x<w; x++) {
		  _farnspokew(d, col256[*ss]);
		  ss++;
		  d += 2;
	       }
	    }
	 }
	 else {
	    for (y=0; y<h; y++) {
	       s = bmp_read_line(src, s_y+y) + s_x;
	       d = bmp_write_line(dest, d_y+y) + d_x*2;

	       for (x=0; x<w; x++) {
		  c = _farpeekb(src->seg, s);
		  _farpokew(dest->seg, d, col256[c]);
		  s++;
		  d += 2;
	       }
	    }
	 }
	 break;

   #endif

   #ifdef ALLEGRO_COLOR24

      case 24:
	 /* 256 -> 24 bit truecolor */
	 if (src->seg == _my_ds()) {
	    _farsetsel(dest->seg);

	    for (y=0; y<h; y++) {
	       ss = src->line[s_y+y] + s_x;
	       d = bmp_write_line(dest, d_y+y) + d_x*3;

	       for (x=0; x<w; x++) {
		  c = col256[*ss];
		  _farnspokew(d, c&0xFFFF);
		  _farnspokeb(d+2, c>>16);
		  ss++;
		  d += 3;
	       }
	    }
	 }
	 else {
	    for (y=0; y<h; y++) {
	       s = bmp_read_line(src, s_y+y) + s_x;
	       d = bmp_write_line(dest, d_y+y) + d_x*3;

	       for (x=0; x<w; x++) {
		  c = col256[_farpeekb(src->seg, s)];
		  _farsetsel(dest->seg);
		  _farnspokew(d, c&0xFFFF);
		  _farnspokeb(d+2, c>>16);
		  s++;
		  d += 3;
	       }
	    }
	 }
	 break;

   #endif

   #ifdef ALLEGRO_COLOR32

      case 32:
	 /* 256 -> 32 bit truecolor */
	 if (src->seg == _my_ds()) {
	    _farsetsel(dest->seg);

	    for (y=0; y<h; y++) {
	       ss = src->line[s_y+y] + s_x;
	       d = bmp_write_line(dest, d_y+y) + d_x*4;

	       for (x=0; x<w; x++) {
		  _farnspokel(d, col256[*ss]);
		  ss++;
		  d += 4;
	       }
	    }
	 }
	 else {
	    for (y=0; y<h; y++) {
	       s = bmp_read_line(src, s_y+y) + s_x;
	       d = bmp_write_line(dest, d_y+y) + d_x*4;

	       for (x=0; x<w; x++) {
		  c = _farpeekb(src->seg, s);
		  _farpokel(dest->seg, d, col256[c]);
		  s++;
		  d += 4;
	       }
	    }
	 }
	 break;

   #endif

   }
}



/* blit_from_15:
 *  Expand 15 bpp images onto some other destination format.
 */
static void blit_from_15(BITMAP *src, BITMAP *dest, int s_x, int s_y, int d_x, int d_y, int w, int h)
{
   #ifdef ALLEGRO_COLOR24

   int x, y, c, r, g, b;
   unsigned long s, d;

   switch (bitmap_color_depth(dest)) {

      case 8:
	 /* reduce 15 bit -> 256 color paletted image */
	 for (y=0; y<h; y++) {
	    s = bmp_read_line(src, s_y+y) + s_x*2;
	    d = bmp_write_line(dest, d_y+y) + d_x;

	    for (x=0; x<w; x++) {
	       c = _farpeekw(src->seg, s);
	       r = getr15(c);
	       g = getg15(c);
	       b = getb15(c);
	       _farpokeb(dest->seg, d, makecol8(r, g, b));
	       s += 2;
	       d++;
	    }
	 }
	 break;

      case 16:
	 /* 15 bit -> 16 bit */
	 for (y=0; y<h; y++) {
	    s = bmp_read_line(src, s_y+y) + s_x*2;
	    d = bmp_write_line(dest, d_y+y) + d_x*2;

	    for (x=0; x<w; x++) {
	       c = _farpeekw(src->seg, s);
	       r = getr15(c);
	       g = getg15(c);
	       b = getb15(c);
	       _farpokew(dest->seg, d, makecol16(r, g, b));
	       s += 2;
	       d += 2;
	    }
	 }
	 break;

   #ifdef ALLEGRO_COLOR24

      case 24:
	 /* 15 bit hicolor -> 24 bit truecolor */
	 for (y=0; y<h; y++) {
	    s = bmp_read_line(src, s_y+y) + s_x*2;
	    d = bmp_write_line(dest, d_y+y) + d_x*3;

	    for (x=0; x<w; x++) {
	       c = _farpeekw(src->seg, s);
	       r = getr15(c);
	       g = getg15(c);
	       b = getb15(c);
	       c = makecol24(r, g, b);
	       _farsetsel(dest->seg);
	       _farnspokew(d, c&0xFFFF);
	       _farnspokeb(d+2, c>>16);
	       s += 2;
	       d += 3;
	    }
	 }
	 break;

   #endif

   #ifdef ALLEGRO_COLOR32

      case 32:
	 /* 15 bit hicolor -> 32 bit truecolor */
	 for (y=0; y<h; y++) {
	    s = bmp_read_line(src, s_y+y) + s_x*2;
	    d = bmp_write_line(dest, d_y+y) + d_x*4;

	    for (x=0; x<w; x++) {
	       c = _farpeekw(src->seg, s);
	       r = getr15(c);
	       g = getg15(c);
	       b = getb15(c);
	       _farpokel(dest->seg, d, makecol32(r, g, b));
	       s += 2;
	       d += 4;
	    }
	 }
	 break;

   #endif

   }

   #endif   /* #ifdef ALLEGRO_COLOR16 */
}



/* blit_from_16:
 *  Expand 16 bpp images onto some other destination format.
 */
static void blit_from_16(BITMAP *src, BITMAP *dest, int s_x, int s_y, int d_x, int d_y, int w, int h)
{
   #ifdef ALLEGRO_COLOR24

   int x, y, c, r, g, b;
   unsigned long s, d;

   switch (bitmap_color_depth(dest)) {

      case 8:
	 /* reduce 16 bit -> 256 color paletted image */
	 for (y=0; y<h; y++) {
	    s = bmp_read_line(src, s_y+y) + s_x*2;
	    d = bmp_write_line(dest, d_y+y) + d_x;

	    for (x=0; x<w; x++) {
	       c = _farpeekw(src->seg, s);
	       r = getr16(c);
	       g = getg16(c);
	       b = getb16(c);
	       _farpokeb(dest->seg, d, makecol8(r, g, b));
	       s += 2;
	       d++;
	    }
	 }
	 break;

      case 15:
	 /* 16 bit -> 15 bit */
	 for (y=0; y<h; y++) {
	    s = bmp_read_line(src, s_y+y) + s_x*2;
	    d = bmp_write_line(dest, d_y+y) + d_x*2;

	    for (x=0; x<w; x++) {
	       c = _farpeekw(src->seg, s);
	       r = getr16(c);
	       g = getg16(c);
	       b = getb16(c);
	       _farpokew(dest->seg, d, makecol15(r, g, b));
	       s += 2;
	       d += 2;
	    }
	 }
	 break;

   #ifdef ALLEGRO_COLOR24

      case 24:
	 /* 16 bit hicolor -> 24 bit truecolor */
	 for (y=0; y<h; y++) {
	    s = bmp_read_line(src, s_y+y) + s_x*2;
	    d = bmp_write_line(dest, d_y+y) + d_x*3;

	    for (x=0; x<w; x++) {
	       c = _farpeekw(src->seg, s);
	       r = getr16(c);
	       g = getg16(c);
	       b = getb16(c);
	       c = makecol24(r, g, b);
	       _farsetsel(dest->seg);
	       _farnspokew(d, c&0xFFFF);
	       _farnspokeb(d+2, c>>16);
	       s += 2;
	       d += 3;
	    }
	 }
	 break;

   #endif

   #ifdef ALLEGRO_COLOR32

      case 32:
	 /* 16 bit hicolor -> 32 bit truecolor */
	 for (y=0; y<h; y++) {
	    s = bmp_read_line(src, s_y+y) + s_x*2;
	    d = bmp_write_line(dest, d_y+y) + d_x*4;

	    for (x=0; x<w; x++) {
	       c = _farpeekw(src->seg, s);
	       r = getr16(c);
	       g = getg16(c);
	       b = getb16(c);
	       _farpokel(dest->seg, d, makecol32(r, g, b));
	       s += 2;
	       d += 4;
	    }
	 }
	 break;

   #endif

   }

   #endif   /* #ifdef ALLEGRO_COLOR16 */
}



/* blit_from_24:
 *  Expand 24 bpp images onto some other destination format.
 */
static void blit_from_24(BITMAP *src, BITMAP *dest, int s_x, int s_y, int d_x, int d_y, int w, int h)
{
   #ifdef ALLEGRO_COLOR24

   int x, y, c, r, g, b;
   unsigned long s, d;

   switch (bitmap_color_depth(dest)) {

      case 8:
	 /* reduce 24 bit -> 256 color paletted image */
	 for (y=0; y<h; y++) {
	    s = bmp_read_line(src, s_y+y) + s_x*3;
	    d = bmp_write_line(dest, d_y+y) + d_x;

	    for (x=0; x<w; x++) {
	       c = _farpeekl(src->seg, s) & 0xFFFFFF;
	       r = getr24(c);
	       g = getg24(c);
	       b = getb24(c);
	       _farpokeb(dest->seg, d, makecol8(r, g, b));
	       s += 3;
	       d++;
	    }
	 }
	 break;

   #ifdef ALLEGRO_COLOR16

      case 15:
	 /* 24 bit truecolor -> 15 bit hicolor */
	 for (y=0; y<h; y++) {
	    s = bmp_read_line(src, s_y+y) + s_x*3;
	    d = bmp_write_line(dest, d_y+y) + d_x*2;

	    for (x=0; x<w; x++) {
	       c = _farpeekl(src->seg, s) & 0xFFFFFF;
	       r = getr24(c);
	       g = getg24(c);
	       b = getb24(c);
	       _farpokew(dest->seg, d, makecol15(r, g, b));
	       s += 3;
	       d += 2;
	    }
	 }
	 break;

      case 16:
	 /* 24 bit truecolor -> 16 bit hicolor */
	 for (y=0; y<h; y++) {
	    s = bmp_read_line(src, s_y+y) + s_x*3;
	    d = bmp_write_line(dest, d_y+y) + d_x*2;

	    for (x=0; x<w; x++) {
	       c = _farpeekl(src->seg, s) & 0xFFFFFF;
	       r = getr24(c);
	       g = getg24(c);
	       b = getb24(c);
	       _farpokew(dest->seg, d, makecol16(r, g, b));
	       s += 3;
	       d += 2;
	    }
	 }
	 break;

   #endif

   #ifdef ALLEGRO_COLOR32

      case 32:
	 /* 24 bit truecolor -> 32 bit truecolor */
	 for (y=0; y<h; y++) {
	    s = bmp_read_line(src, s_y+y) + s_x*3;
	    d = bmp_write_line(dest, d_y+y) + d_x*4;

	    for (x=0; x<w; x++) {
	       c = _farpeekl(src->seg, s) & 0xFFFFFF;
	       r = getr24(c);
	       g = getg24(c);
	       b = getb24(c);
	       c = makecol32(r, g, b);
	       _farpokel(dest->seg, d, c);
	       s += 3;
	       d += 4;
	    }
	 }
	 break;

   #endif

   }

   #endif   /* #ifdef ALLEGRO_COLOR24 */
}



/* blit_from_32:
 *  Expand 32 bpp images onto some other destination format.
 */
static void blit_from_32(BITMAP *src, BITMAP *dest, int s_x, int s_y, int d_x, int d_y, int w, int h)
{
   #ifdef ALLEGRO_COLOR32

   int x, y, c, r, g, b;
   unsigned long s, d;

   switch (bitmap_color_depth(dest)) {

      case 8:
	 /* reduce 32 bit -> 256 color paletted image */
	 for (y=0; y<h; y++) {
	    s = bmp_read_line(src, s_y+y) + s_x*4;
	    d = bmp_write_line(dest, d_y+y) + d_x;

	    for (x=0; x<w; x++) {
	       c = _farpeekl(src->seg, s);
	       r = getr32(c);
	       g = getg32(c);
	       b = getb32(c);
	       _farpokeb(dest->seg, d, makecol8(r, g, b));
	       s += 4;
	       d++;
	    }
	 }
	 break;

   #ifdef ALLEGRO_COLOR16

      case 15:
	 /* 32 bit truecolor -> 15 bit hicolor */
	 for (y=0; y<h; y++) {
	    s = bmp_read_line(src, s_y+y) + s_x*4;
	    d = bmp_write_line(dest, d_y+y) + d_x*2;

	    for (x=0; x<w; x++) {
	       c = _farpeekl(src->seg, s);
	       r = getr32(c);
	       g = getg32(c);
	       b = getb32(c);
	       _farpokew(dest->seg, d, makecol15(r, g, b));
	       s += 4;
	       d += 2;
	    }
	 }
	 break;

      case 16:
	 /* 32 bit truecolor -> 16 bit hicolor */
	 for (y=0; y<h; y++) {
	    s = bmp_read_line(src, s_y+y) + s_x*4;
	    d = bmp_write_line(dest, d_y+y) + d_x*2;

	    for (x=0; x<w; x++) {
	       c = _farpeekl(src->seg, s);
	       r = getr32(c);
	       g = getg32(c);
	       b = getb32(c);
	       _farpokew(dest->seg, d, makecol16(r, g, b));
	       s += 4;
	       d += 2;
	    }
	 }
	 break;

   #endif

   #ifdef ALLEGRO_COLOR24

      case 24:
	 /* 32 bit truecolor -> 24 bit truecolor */
	 for (y=0; y<h; y++) {
	    s = bmp_read_line(src, s_y+y) + s_x*4;
	    d = bmp_write_line(dest, d_y+y) + d_x*3;

	    for (x=0; x<w; x++) {
	       c = _farpeekl(src->seg, s);
	       r = getr32(c);
	       g = getg32(c);
	       b = getb32(c);
	       c = makecol24(r, g, b);
	       _farsetsel(dest->seg);
	       _farnspokew(d, c&0xFFFF);
	       _farnspokeb(d+2, c>>16);
	       s += 4;
	       d += 3;
	    }
	 }
	 break;

   #endif

   }

   #endif   /* #ifdef ALLEGRO_COLOR32 */
}



/* blit_to_or_from_modex:
 *  Converts between truecolor and planar mode-X bitmaps. This function is 
 *  painfully slow, but I don't think it is something that people will need
 *  to do very often...
 */
static void blit_to_or_from_modex(BITMAP *src, BITMAP *dest, int s_x, int s_y, int d_x, int d_y, int w, int h)
{
   int x, y, c, r, g, b;
   int src_depth = bitmap_color_depth(src);
   int dest_depth = bitmap_color_depth(dest);

   for (y=0; y<h; y++) {
      for (x=0; x<w; x++) {
	 c = getpixel(src, s_x+x, s_y+y);
	 r = getr_depth(src_depth, c);
	 g = getg_depth(src_depth, c);
	 b = getb_depth(src_depth, c);
	 c = makecol_depth(dest_depth, r, g, b);
	 putpixel(dest, d_x+x, d_y+y, c);
      }
   }
}



/* blit_between_formats:
 *  Blits an (already clipped) region between two bitmaps of different
 *  color depths, doing the appopriate format conversions.
 */
static void blit_between_formats(BITMAP *src, BITMAP *dest, int s_x, int s_y, int d_x, int d_y, int w, int h)
{
   if ((is_planar_bitmap(src)) || (is_planar_bitmap(dest))) {
      blit_to_or_from_modex(src, dest, s_x, s_y, d_x, d_y, w, h);
   }
   else {
      switch (bitmap_color_depth(src)) {

	 case 8:
	    blit_from_256(src, dest, s_x, s_y, d_x, d_y, w, h);
	    break;

	 case 15:
	    blit_from_15(src, dest, s_x, s_y, d_x, d_y, w, h);
	    break;

	 case 16:
	    blit_from_16(src, dest, s_x, s_y, d_x, d_y, w, h);
	    break;

	 case 24:
	    blit_from_24(src, dest, s_x, s_y, d_x, d_y, w, h);
	    break;

	 case 32:
	    blit_from_32(src, dest, s_x, s_y, d_x, d_y, w, h);
	    break;
      }
   }
}



/* helper for clipping a blit rectangle */
#define BLIT_CLIP()                                                          \
   /* check for ridiculous cases */                                          \
   if ((s_x >= src->w) || (s_y >= src->h) ||                                 \
       (d_x >= dest->cr) || (d_y >= dest->cb))                               \
      return;                                                                \
									     \
   /* clip src left */                                                       \
   if (s_x < 0) {                                                            \
      w += s_x;                                                              \
      d_x -= s_x;                                                            \
      s_x = 0;                                                               \
   }                                                                         \
									     \
   /* clip src top */                                                        \
   if (s_y < 0) {                                                            \
      h += s_y;                                                              \
      d_y -= s_y;                                                            \
      s_y = 0;                                                               \
   }                                                                         \
									     \
   /* clip src right */                                                      \
   if (s_x+w > src->w)                                                       \
      w = src->w - s_x;                                                      \
									     \
   /* clip src bottom */                                                     \
   if (s_y+h > src->h)                                                       \
      h = src->h - s_y;                                                      \
									     \
   /* clip dest left */                                                      \
   if (d_x < dest->cl) {                                                     \
      d_x -= dest->cl;                                                       \
      w += d_x;                                                              \
      s_x -= d_x;                                                            \
      d_x = dest->cl;                                                        \
   }                                                                         \
									     \
   /* clip dest top */                                                       \
   if (d_y < dest->ct) {                                                     \
      d_y -= dest->ct;                                                       \
      h += d_y;                                                              \
      s_y -= d_y;                                                            \
      d_y = dest->ct;                                                        \
   }                                                                         \
									     \
   /* clip dest right */                                                     \
   if (d_x+w > dest->cr)                                                     \
      w = dest->cr - d_x;                                                    \
									     \
   /* clip dest bottom */                                                    \
   if (d_y+h > dest->cb)                                                     \
      h = dest->cb - d_y;                                                    \
									     \
   /* bottle out if zero size */                                             \
   if ((w <= 0) || (h <= 0))                                                 \
      return;



/* void blit(BITMAP *src, BITMAP *dest, int s_x, s_y, int d_x, d_y, int w, h);
 *  Copies an area of the source bitmap to the destination bitmap. s_x and 
 *  s_y give the top left corner of the area of the source bitmap to copy, 
 *  and d_x and d_y give the position in the destination bitmap. w and h 
 *  give the size of the area to blit. This routine respects the clipping 
 *  rectangle of the destination bitmap, and will work correctly even when 
 *  the two memory areas overlap (ie. src and dest are the same). 
 */
void blit(BITMAP *src, BITMAP *dest, int s_x, int s_y, int d_x, int d_y, int w, int h)
{
   BITMAP *tmp;
   unsigned long s_low, s_high, d_low, d_high;

   BLIT_CLIP();

   /* if the bitmaps are the same... */
   if (is_same_bitmap(src, dest)) {
      /* with single-banked cards we have to use a temporary bitmap */
      if ((dest->write_bank == dest->read_bank) && 
	  (dest->write_bank != _stub_bank_switch)) {
	 tmp = create_bitmap(w, h);
	 if (tmp) {
	    src->vtable->blit_to_memory(src, tmp, s_x, s_y, 0, 0, w, h);
	    dest->vtable->blit_from_memory(tmp, dest, 0, 0, d_x, d_y, w, h);
	    destroy_bitmap(tmp);
	 }
      }
      else {
	 /* check which way round to do the blit */
	 s_low = get_bitmap_addr(src, s_x, s_y);
	 s_high = get_bitmap_addr(src, s_x+w, s_y+h-1);
	 d_low = get_bitmap_addr(dest, d_x, d_y);
	 d_high = get_bitmap_addr(dest, d_x+w, d_y+h-1);

	 if ((s_low > d_high) || (d_low > s_high))
	    dest->vtable->blit_to_self(src, dest, s_x, s_y, d_x, d_y, w, h);
	 else if (s_low >= d_low)
	    dest->vtable->blit_to_self_forward(src, dest, s_x, s_y, d_x, d_y, w, h);
	 else
	    dest->vtable->blit_to_self_backward(src, dest, s_x, s_y, d_x, d_y, w, h);
      } 
   }
   else {
      /* if the bitmaps are different, check which vtable to use... */
      if (src->vtable->color_depth != dest->vtable->color_depth)
	 blit_between_formats(src, dest, s_x, s_y, d_x, d_y, w, h);
      else if (src->vtable->bitmap_type == BMP_TYPE_LINEAR)
	 dest->vtable->blit_from_memory(src, dest, s_x, s_y, d_x, d_y, w, h);
      else
	 src->vtable->blit_to_memory(src, dest, s_x, s_y, d_x, d_y, w, h);
   }
}

END_OF_FUNCTION(blit);



/* void masked_blit(BITMAP *src, BITMAP *dest, int s_x, s_y, d_x, d_y, w, h);
 *  Version of blit() that skips zero pixels. The source must be a memory
 *  bitmap, and the source and dest regions must not overlap.
 */
void masked_blit(BITMAP *src, BITMAP *dest, int s_x, int s_y, int d_x, int d_y, int w, int h)
{
   BLIT_CLIP();

   dest->vtable->masked_blit(src, dest, s_x, s_y, d_x, d_y, w, h);
}

