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
 *      Graphics routines: pallete fading, circles, etc.
 *
 *      See readme.txt for copyright information.
 */


#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <limits.h>

#include "allegro.h"
#include "internal.h"



/* drawing_mode:
 *  Sets the drawing mode. This only affects routines like putpixel,
 *  lines, rectangles, triangles, etc, not the blitting or sprite
 *  drawing functions.
 */
void drawing_mode(int mode, BITMAP *pattern, int x_anchor, int y_anchor)
{
   _drawing_mode = mode;
   _drawing_pattern = pattern;
   _drawing_x_anchor = x_anchor;
   _drawing_y_anchor = y_anchor;

   if (pattern) {
      _drawing_x_mask = 1; 
      while (_drawing_x_mask < (unsigned)pattern->w)
	 _drawing_x_mask <<= 1;        /* find power of two greater than w */

      if (_drawing_x_mask > (unsigned)pattern->w)
	 _drawing_x_mask >>= 1;        /* round down if required */

      _drawing_x_mask--;               /* convert to AND mask */

      _drawing_y_mask = 1;
      while (_drawing_y_mask < (unsigned)pattern->h)
	 _drawing_y_mask <<= 1;        /* find power of two greater than h */

      if (_drawing_y_mask > (unsigned)pattern->h)
	 _drawing_y_mask >>= 1;        /* round down if required */

      _drawing_y_mask--;               /* convert to AND mask */
   }
   else
      _drawing_x_mask = _drawing_y_mask = 0;
}



/* set_blender_mode:
 *  Specifies a custom set of blender functions for interpolating between
 *  truecolor pixels. Provide a table of 256 interpolation routines (one for
 *  every alpha value), for each color depth you are going to use (the 24
 *  bit blender is shared between the 24 and 32 bit modes). Pass a NULL
 *  table for unused color depths (you must not draw translucent graphics 
 *  in modes without a table, though!). Your blender will be passed two
 *  32 bit colors in the appropriate format (5.5.5, 5.6.5, or 8.8.8), and
 *  should return the result of combining them. In translucent drawing modes,
 *  the two colors are taken from the source and destination images and the
 *  alpha is specified by this function. In lit modes, the alpha is specified
 *  when you call the drawing routine, and the interpolation is between the
 *  source color and the RGB values you pass to this function.
 */
void set_blender_mode(BLENDER_MAP *b15, BLENDER_MAP *b16, BLENDER_MAP *b24, int r, int g, int b, int a)
{
   _blender_map15 = b15;
   _blender_map16 = b16;
   _blender_map24 = b24;

   _blender_col_15 = makecol15(r, g, b);
   _blender_col_16 = makecol16(r, g, b);
   _blender_col_24 = makecol24(r, g, b);
   _blender_col_32 = makecol32(r, g, b);

   _blender_alpha = a;
}



/* xor_mode:
 *  Shortcut function for toggling XOR mode on and off.
 */
void xor_mode(int xor)
{
   drawing_mode(xor ? DRAW_MODE_XOR : DRAW_MODE_SOLID, NULL, 0, 0);
}



/* solid_mode:
 *  Shortcut function for selecting solid drawing mode.
 */
void solid_mode()
{
   drawing_mode(DRAW_MODE_SOLID, NULL, 0, 0);
}



/* clear:
 *  Clears the bitmap to color 0.
 */
void clear(BITMAP *bitmap)
{
   clear_to_color(bitmap, 0);
}



/* set_color:
 *  Sets a single pallete entry.
 */
void set_color(int index, RGB *p)
{
   set_pallete_range(p-index, index, index, FALSE);
}



/* set_pallete:
 *  Sets the entire color pallete.
 */
void set_pallete(PALLETE p)
{
   set_pallete_range(p, 0, PAL_SIZE-1, TRUE);
}



/* set_pallete_range:
 *  Sets a part of the color pallete.
 */
void set_pallete_range(PALLETE p, int from, int to, int vsync)
{
   int c;

   _check_gfx_virginity();

   if (gfx_driver) {
      if (screen->vtable->color_depth == 8)
	 gfx_driver->set_pallete(p, from, to, vsync);
   }
   else
      _vga_set_pallete_range(p, from, to, vsync);

   for (c=from; c<=to; c++) {
      _current_pallete[c] = p[c];

      if (_color_depth != 8)
	 pallete_color[c] = makecol(_rgb_scale_6[p[c].r], _rgb_scale_6[p[c].g], _rgb_scale_6[p[c].b]);
   }
}



/* previous palette, so the image loaders can restore it when they are done */
static PALLETE prev_current_pallete;
static int prev_pallete_color[256];



/* select_pallete:
 *  Sets the aspects of the palette tables that are used for converting
 *  between different image formats, without altering the display settings.
 *  The previous settings are copied onto a one-deep stack, from where they
 *  can be restored by calling unselect_pallete().
 */
void select_pallete(PALLETE p)
{
   int c;

   for (c=0; c<256; c++) {
      prev_current_pallete[c] = _current_pallete[c];
      prev_pallete_color[c] = pallete_color[c];

      _current_pallete[c] = p[c];

      if (_color_depth != 8)
	 pallete_color[c] = makecol(_rgb_scale_6[p[c].r], _rgb_scale_6[p[c].g], _rgb_scale_6[p[c].b]);
   }
}



/* unselect_pallete:
 *  Restores pallete settings from before the last call to select_pallete().
 */
void unselect_pallete()
{
   int c;

   for (c=0; c<256; c++) {
      _current_pallete[c] = prev_current_pallete[c];

      if (_color_depth != 8)
	 pallete_color[c] = prev_pallete_color[c];
   }
}



/* generate_332_palette:
 *  Used when loading a truecolor image into an 8 bit bitmap, to generate
 *  a 3.3.2 RGB palette.
 */
void generate_332_palette(PALETTE pal)
{
   int c;

   for (c=0; c<256; c++) {
      pal[c].r = ((c>>5)&7) * 63/7;
      pal[c].g = ((c>>2)&7) * 63/7;
      pal[c].b = (c&3) * 63/3;
   }

   pal[0].r = 63;
   pal[0].g = 0;
   pal[0].b = 63;

   pal[254].r = pal[254].g = pal[254].b = 0;
}



/* get_color:
 *  Retrieves a single color from the pallete.
 */
void get_color(int index, RGB *p)
{
   get_pallete_range(p-index, index, index);
}



/* get_pallete:
 *  Retrieves the entire color pallete.
 */
void get_pallete(PALLETE p)
{
   get_pallete_range(p, 0, PAL_SIZE-1);
}



/* get_pallete_range:
 *  Retrieves a part of the color pallete.
 */
void get_pallete_range(PALLETE p, int from, int to)
{
   int c;

   _check_gfx_virginity();

   for (c=from; c<=to; c++)
      p[c] = _current_pallete[c];
}



/* fade_interpolate: 
 *  Calculates a pallete part way between source and dest, returning it
 *  in output. The pos indicates how far between the two extremes it should
 *  be: 0 = return source, 64 = return dest, 32 = return exactly half way.
 *  Only affects colors between from and to (inclusive).
 */
void fade_interpolate(PALLETE source, PALLETE dest, PALLETE output, int pos, int from, int to)
{
   int c;

   for (c=from; c<=to; c++) { 
      output[c].r = ((int)source[c].r * (63-pos) + (int)dest[c].r * pos) / 64;
      output[c].g = ((int)source[c].g * (63-pos) + (int)dest[c].g * pos) / 64;
      output[c].b = ((int)source[c].b * (63-pos) + (int)dest[c].b * pos) / 64;
   }
}



/* fade_from_range:
 *  Fades from source to dest, at the specified speed (1 is the slowest, 64
 *  is instantaneous). Only affects colors between from and to (inclusive,
 *  pass 0 and 255 to fade the entire pallete).
 */
void fade_from_range(PALLETE source, PALLETE dest, int speed, int from, int to)
{
   PALLETE temp;
   int c;

   for (c=0; c<PAL_SIZE; c++)
      temp[c] = source[c];

   for (c=0; c<64; c+=speed) {
      fade_interpolate(source, dest, temp, c, from, to);
      set_pallete_range(temp, from, to, TRUE);
      set_pallete_range(temp, from, to, TRUE);
   }

   set_pallete_range(dest, from, to, TRUE);
}



/* fade_in_range:
 *  Fades from a solid black pallete to p, at the specified speed (1 is
 *  the slowest, 64 is instantaneous). Only affects colors between from and 
 *  to (inclusive, pass 0 and 255 to fade the entire pallete).
 */
void fade_in_range(PALLETE p, int speed, int from, int to)
{
   fade_from_range(black_pallete, p, speed, from, to);
}



/* fade_out_range:
 *  Fades from the current pallete to a solid black pallete, at the 
 *  specified speed (1 is the slowest, 64 is instantaneous). Only affects 
 *  colors between from and to (inclusive, pass 0 and 255 to fade the 
 *  entire pallete).
 */
void fade_out_range(int speed, int from, int to)
{
   PALLETE temp;

   get_pallete(temp);
   fade_from_range(temp, black_pallete, speed, from, to);
}



/* fade_from:
 *  Fades from source to dest, at the specified speed (1 is the slowest, 64
 *  is instantaneous).
 */
void fade_from(PALLETE source, PALLETE dest, int speed)
{
   fade_from_range(source, dest, speed, 0, PAL_SIZE-1);
}



/* fade_in:
 *  Fades from a solid black pallete to p, at the specified speed (1 is
 *  the slowest, 64 is instantaneous).
 */
void fade_in(PALLETE p, int speed)
{
   fade_in_range(p, speed, 0, PAL_SIZE-1);
}



/* fade_out:
 *  Fades from the current pallete to a solid black pallete, at the 
 *  specified speed (1 is the slowest, 64 is instantaneous).
 */
void fade_out(int speed)
{
   fade_out_range(speed, 0, PAL_SIZE-1);
}



/* rect:
 *  Draws an outline rectangle.
 */
void rect(BITMAP *bmp, int x1, int y1, int x2, int y2, int color)
{
   if (x2 < x1) {
      x1 ^= x2;
      x2 ^= x1;
      x1 ^= x2;
   }

   if (y2 < y1) {
      y1 ^= y2;
      y2 ^= y1;
      y1 ^= y2;
   }

   hline(bmp, x1, y1, x2, color);

   if (y2 > y1)
      hline(bmp, x1, y2, x2, color);

   if (y2-1 >= y1+1) {
      vline(bmp, x1, y1+1, y2-1, color);

      if (x2 > x1)
	 vline(bmp, x2, y1+1, y2-1, color);
   }
}



/* _normal_rectfill:
 *  Draws a solid filled rectangle, using hline() to do the work.
 */
void _normal_rectfill(BITMAP *bmp, int x1, int y1, int x2, int y2, int color)
{
   int t;

   if (y1 > y2) {
      t = y1;
      y1 = y2;
      y2 = t;
   }

   while (y1 <= y2) {
      hline(bmp, x1, y1, x2, color);
      y1++;
   };
}



/* _normal_line:
 *  Draws a line from x1, y1 to x2, y2, using putpixel() to do the work.
 */
void _normal_line(BITMAP *bmp, int x1, int y1, int x2, int y2, int color)
{
   if (x1 == x2)
      vline(bmp, x1, y1, y2, color);
   else if (y1 == y2)
      hline(bmp, x1, y1, x2, color);
   else
      do_line(bmp, x1, y1, x2, y2, color, bmp->vtable->putpixel);
}



/* do_circle:
 *  Helper function for the circle drawing routines. Calculates the points
 *  in a circle of radius r around point x, y, and calls the specified 
 *  routine for each one. The output proc will be passed first a copy of
 *  the bmp parameter, then the x, y point, then a copy of the d parameter
 *  (so putpixel() can be used as the callback).
 */
void do_circle(BITMAP *bmp, int x, int y, int radius, int d, void (*proc)())
{
   int cx = 0;
   int cy = radius;
   int df = 1 - radius; 
   int d_e = 3;
   int d_se = -2 * radius + 5;

   do {
      proc(bmp, x+cx, y+cy, d); 

      if (cx) 
	 proc(bmp, x-cx, y+cy, d); 

      if (cy) 
	 proc(bmp, x+cx, y-cy, d);

      if ((cx) && (cy)) 
	 proc(bmp, x-cx, y-cy, d); 

      if (cx != cy) {
	 proc(bmp, x+cy, y+cx, d); 

	 if (cx) 
	    proc(bmp, x+cy, y-cx, d);

	 if (cy) 
	    proc(bmp, x-cy, y+cx, d); 

	 if (cx && cy) 
	    proc(bmp, x-cy, y-cx, d); 
      }

      if (df < 0)  {
	 df += d_e;
	 d_e += 2;
	 d_se += 2;
      }
      else { 
	 df += d_se;
	 d_e += 2;
	 d_se += 4;
	 cy--;
      } 

      cx++; 

   } while (cx <= cy);
}



/* circle:
 *  Draws a circle.
 */
void circle(BITMAP *bmp, int x, int y, int radius, int color)
{
   do_circle(bmp, x, y, radius, color, bmp->vtable->putpixel);
}



/* circlefill:
 *  Draws a filled circle.
 */
void circlefill(BITMAP *bmp, int x, int y, int radius, int color)
{
   int cx = 0;
   int cy = radius;
   int df = 1 - radius; 
   int d_e = 3;
   int d_se = -2 * radius + 5;

   do {
      hline(bmp, x-cy, y-cx, x+cy, color);

      if (cx)
	 hline(bmp, x-cy, y+cx, x+cy, color);

      if (df < 0)  {
	 df += d_e;
	 d_e += 2;
	 d_se += 2;
      }
      else { 
	 if (cx != cy) {
	    hline(bmp, x-cx, y-cy, x+cx, color);

	    if (cy)
	       hline(bmp, x-cx, y+cy, x+cx, color);
	 }

	 df += d_se;
	 d_e += 2;
	 d_se += 4;
	 cy--;
      } 

      cx++; 

   } while (cx <= cy);
}



/* do_ellipse:
 *  Helper function for the ellipse drawing routines. Calculates the points
 *  in an ellipse of radius rx and ry around point x, y, and calls the 
 *  specified routine for each one. The output proc will be passed first a 
 *  copy of the bmp parameter, then the x, y point, then a copy of the d 
 *  parameter (so putpixel() can be used as the callback).
 */
void do_ellipse(BITMAP *bmp, int x, int y, int rx, int ry, int d, void (*proc)())
{
   int ix, iy;
   int h, i, j, k;
   int oh, oi, oj, ok;

   if (rx < 1) 
      rx = 1; 

   if (ry < 1) 
      ry = 1;

   h = i = j = k = 0xFFFF;

   if (rx > ry) {
      ix = 0; 
      iy = rx * 64;

      do {
	 oh = h;
	 oi = i;
	 oj = j;
	 ok = k;

	 h = (ix + 32) >> 6; 
	 i = (iy + 32) >> 6;
	 j = (h * ry) / rx; 
	 k = (i * ry) / rx;

	 if (((h != oh) || (k != ok)) && (h < oi)) {
	    proc(bmp, x+h, y+k, d); 
	    if (h) 
	       proc(bmp, x-h, y+k, d);
	    if (k) {
	       proc(bmp, x+h, y-k, d); 
	       if (h)
		  proc(bmp, x-h, y-k, d);
	    }
	 }

	 if (((i != oi) || (j != oj)) && (h < i)) {
	    proc(bmp, x+i, y+j, d); 
	    if (i)
	       proc(bmp, x-i, y+j, d);
	    if (j) {
	       proc(bmp, x+i, y-j, d); 
	       if (i)
		  proc(bmp, x-i, y-j, d);
	    }
	 }

	 ix = ix + iy / rx; 
	 iy = iy - ix / rx;

      } while (i > h);
   } 
   else {
      ix = 0; 
      iy = ry * 64;

      do {
	 oh = h;
	 oi = i;
	 oj = j;
	 ok = k;

	 h = (ix + 32) >> 6; 
	 i = (iy + 32) >> 6;
	 j = (h * rx) / ry; 
	 k = (i * rx) / ry;

	 if (((j != oj) || (i != oi)) && (h < i)) {
	    proc(bmp, x+j, y+i, d); 
	    if (j)
	       proc(bmp, x-j, y+i, d);
	    if (i) {
	       proc(bmp, x+j, y-i, d); 
	       if (j)
		  proc(bmp, x-j, y-i, d);
	    }
	 }

	 if (((k != ok) || (h != oh)) && (h < oi)) {
	    proc(bmp, x+k, y+h, d); 
	    if (k)
	       proc(bmp, x-k, y+h, d);
	    if (h) {
	       proc(bmp, x+k, y-h, d); 
	       if (k)
		  proc(bmp, x-k, y-h, d);
	    }
	 }

	 ix = ix + iy / ry; 
	 iy = iy - ix / ry;

      } while(i > h);
   }
}



/* ellipse:
 *  Draws an ellipse.
 */
void ellipse(BITMAP *bmp, int x, int y, int rx, int ry, int color)
{
   do_ellipse(bmp, x, y, rx, ry, color, bmp->vtable->putpixel);
}



/* ellipsefill:
 *  Draws a filled ellipse.
 */
void ellipsefill(BITMAP *bmp, int cx, int cy, int rx, int ry, int color)
{
   int x, y;
   int a, b, c, d;
   int da, db, dc, dd;
   int na, nb, nc, nd;

   if (rx < 1)
      rx = 1;

   if (ry < 1) 
      ry = 1;

   if (rx > ry) {
      dc = -1;
      dd = 0xFFFF;
      x = 0; 
      y = rx * 64;
      na = 0; 
      nb = (y + 32) >> 6;
      nc = 0; 
      nd = (nb * ry) / rx;

      do {
	 a = na; 
	 b = nb; 
	 c = nc; 
	 d = nd;

	 x = x + (y / rx);
	 y = y - (x / rx);
	 na = (x + 32) >> 6; 
	 nb = (y + 32) >> 6;
	 nc = (na * ry) / rx; 
	 nd = (nb * ry) / rx;

	 if ((c > dc) && (c < dd)) {
	    hline(bmp, cx-b, cy+c, cx+b, color);
	    if (c)
	       hline(bmp, cx-b, cy-c, cx+b, color);
	    dc = c;
	 }

	 if ((d < dd) && (d > dc)) { 
	    hline(bmp, cx-a, cy+d, cx+a, color);
	    hline(bmp, cx-a, cy-d, cx+a, color);
	    dd = d;
	 }

      } while(b > a);
   } 
   else {
      da = -1;
      db = 0xFFFF;
      x = 0; 
      y = ry * 64; 
      na = 0; 
      nb = (y + 32) >> 6;
      nc = 0; 
      nd = (nb * rx) / ry;

      do {
	 a = na; 
	 b = nb; 
	 c = nc; 
	 d = nd; 

	 x = x + (y / ry); 
	 y = y - (x / ry);
	 na = (x + 32) >> 6; 
	 nb = (y + 32) >> 6;
	 nc = (na * rx) / ry; 
	 nd = (nb * rx) / ry;

	 if ((a > da) && (a < db)) {
	    hline(bmp, cx-d, cy+a, cx+d, color); 
	    if (a)
	       hline(bmp, cx-d, cy-a, cx+d, color);
	    da = a;
	 }

	 if ((b < db) && (b > da)) { 
	    hline(bmp, cx-c, cy+b, cx+c, color);
	    hline(bmp, cx-c, cy-b, cx+c, color);
	    db = b;
	 }

      } while(b > a);
   }
}


