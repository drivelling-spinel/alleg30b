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
 *      Sprite rotation, RLE, and compressed sprite routines.
 *
 *      See readme.txt for copyright information.
 */


#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>

#ifdef DJGPP
#include <sys/farptr.h>
#endif

#include "allegro.h"
#include "internal.h"
#include "opcodes.h"



/* rotate_sprite:
 *  Draws a sprite image onto a bitmap at the specified position, rotating 
 *  it by the specified angle. The angle is a fixed point 16.16 number in 
 *  the same format used by the fixed point trig routines, with 256 equal 
 *  to a full circle, 64 a right angle, etc. This function can draw onto
 *  both linear and mode-X bitmaps.
 */
void rotate_sprite(BITMAP *bmp, BITMAP *sprite, int x, int y, fixed angle)
{
   rotate_scaled_sprite(bmp, sprite, x, y, angle, itofix(1));
}



/* rotate_scaled_sprite:
 *  Draws a sprite image onto a bitmap at the specified position, rotating 
 *  it by the specified angle. The angle is a fixed point 16.16 number in 
 *  the same format used by the fixed point trig routines, with 256 equal 
 *  to a full circle, 64 a right angle, etc. This function can draw onto
 *  both linear and mode-X bitmaps.
 */
void rotate_scaled_sprite(BITMAP *bmp, BITMAP *sprite, int x, int y, fixed angle, fixed scale)
{
   fixed f1x, f1y, f1xd, f1yd;
   fixed f2x, f2y, f2xd, f2yd;
   fixed f3x, f3y;
   fixed w, h;
   fixed dir, dir2, dist;
   int x1, y1, x2, y2;
   int dx, dy;
   int sx, sy;
   unsigned long addr;
   int plane;
   int wgap = sprite->w;
   int hgap = sprite->h;
   int pixel;

   /* rotate the top left pixel of the sprite */
   w = itofix(wgap/2);
   h = itofix(hgap/2);
   dir = fatan2(h, w);
   dir2 = fatan2(h, -w);
   dist = fsqrt((wgap*wgap + hgap*hgap) << 6) << 4;
   f1x = w - fmul(dist, fcos(dir - angle));
   f1y = h - fmul(dist, fsin(dir - angle));

   /* map the destination y axis onto the sprite */
   f1xd = fcos(itofix(64) - angle);
   f1yd = fsin(itofix(64) - angle);

   /* map the destination x axis onto the sprite */
   f2xd = fcos(-angle);
   f2yd = fsin(-angle);

   /* scale the sprite? */
   if (scale != itofix(1)) {
      dist = fmul(dist, scale);
      wgap = fixtoi(fmul(itofix(wgap), scale));
      hgap = fixtoi(fmul(itofix(hgap), scale));
      scale = fdiv(itofix(1), scale);
      f1xd = fmul(f1xd, scale);
      f1yd = fmul(f1yd, scale);
      f2xd = fmul(f2xd, scale);
      f2yd = fmul(f2yd, scale);
   }

   /* adjust the size of the region to be scanned */
   x1 = fixtoi(fmul(dist, fcos(dir + angle)));
   y1 = fixtoi(fmul(dist, fsin(dir + angle)));

   x2 = fixtoi(fmul(dist, fcos(dir2 + angle)));
   y2 = fixtoi(fmul(dist, fsin(dir2 + angle)));

   x1 = MAX(ABS(x1), ABS(x2)) - wgap/2;
   y1 = MAX(ABS(y1), ABS(y2)) - hgap/2;

   x -= x1;
   wgap += x1 * 2;
   f1x -= f2xd * x1;
   f1y -= f2yd * x1;

   y -= y1;
   hgap += y1 * 2;
   f1x -= f1xd * y1;
   f1y -= f1yd * y1;

   /* clip the output rectangle */
   if (bmp->clip) {
      while (x < bmp->cl) {
	 x++;
	 wgap--;
	 f1x += f2xd;
	 f1y += f2yd;
      }

      while (y < bmp->ct) {
	 y++;
	 hgap--;
	 f1x += f1xd;
	 f1y += f1yd;
      }

      while (x+wgap > bmp->cr)
	 wgap--;

      while (y+hgap > bmp->cb)
	 hgap--;

      if ((wgap <= 0) || (hgap <= 0))
	 return;
   }

   /* select the destination segment */
   _farsetsel(bmp->seg);

   /* and trace a bunch of lines through the bitmaps */
   for (dy=0; dy<hgap; dy++) {
      f2x = f1x;
      f2y = f1y;

      if (is_linear_bitmap(bmp)) {           /* draw onto a linear bitmap */
	 switch (bitmap_color_depth(bmp)) {

	    case 8:
	    default:
	       /* 8 bit version */
	       addr = bmp_write_line(bmp, y+dy) + x;

	       for (dx=0; dx<wgap; dx++) {
		  sx = fixtoi(f2x);
		  sy = fixtoi(f2y);

		  if ((sx >= 0) && (sx < sprite->w) && (sy >= 0) && (sy < sprite->h))
		     if (sprite->line[sy][sx])
			_farnspokeb(addr, sprite->line[sy][sx]);

		  addr++;
		  f2x += f2xd;
		  f2y += f2yd;
	       }
	       break;

	    #ifdef ALLEGRO_COLOR16

	       case 15:
	       case 16:
		  /* 16 bit version */
		  addr = bmp_write_line(bmp, y+dy) + x*2;

		  for (dx=0; dx<wgap; dx++) {
		     sx = fixtoi(f2x);
		     sy = fixtoi(f2y);

		     if ((sx >= 0) && (sx < sprite->w) && (sy >= 0) && (sy < sprite->h)) {
			pixel = ((unsigned short *)sprite->line[sy])[sx];
			if (pixel != bmp->vtable->mask_color)
			   _farnspokew(addr, pixel);
		     }

		     addr += 2;
		     f2x += f2xd;
		     f2y += f2yd;
		  }
		  break;

	    #endif

	    #ifdef ALLEGRO_COLOR24

	       case 24:
		  /* 24 bit version */
		  addr = bmp_write_line(bmp, y+dy) + x*3;

		  for (dx=0; dx<wgap; dx++) {
		     sx = fixtoi(f2x);
		     sy = fixtoi(f2y);

		     if ((sx >= 0) && (sx < sprite->w) && (sy >= 0) && (sy < sprite->h)) {
			pixel = *((unsigned long *)(((unsigned char *)sprite->line[sy])+sx*3));
			pixel &= 0xFFFFFF;
			if (pixel != bmp->vtable->mask_color) {
			   _farnspokew(addr, pixel);
			   _farnspokeb(addr+2,(pixel>>16) & 0xff); 
			}
		     }

		     addr += 3;
		     f2x += f2xd;
		     f2y += f2yd;
		  }
		  break;

	    #endif

	    #ifdef ALLEGRO_COLOR32

	       case 32:
		  /* 32 bit version */
		  addr = bmp_write_line(bmp, y+dy) + x*4;

		  for (dx=0; dx<wgap; dx++) {
		     sx = fixtoi(f2x);
		     sy = fixtoi(f2y);

		     if ((sx >= 0) && (sx < sprite->w) && (sy >= 0) && (sy < sprite->h)) {
			pixel = ((unsigned long *)sprite->line[sy])[sx];
			if (pixel != bmp->vtable->mask_color)
			   _farnspokel(addr, pixel);
		     }

		     addr += 4;
		     f2x += f2xd;
		     f2y += f2yd;
		  }
		  break;

	    #endif
	 }
      }
      else {                                 /* draw onto a mode-X bitmap */
	 for (plane=0; plane<4; plane++) {
	    f3x = f2x;
	    f3y = f2y;
	    addr = (unsigned long)bmp->line[y+dy] + ((x+plane)>>2);
	    outportw(0x3C4, (0x100<<((x+plane)&3))|2);

	    for (dx=plane; dx<wgap; dx+=4) {
	       sx = fixtoi(f3x);
	       sy = fixtoi(f3y);

	       if ((sx >= 0) && (sx < sprite->w) && (sy >= 0) && (sy < sprite->h))
		  if (sprite->line[sy][sx])
		     _farnspokeb(addr, sprite->line[sy][sx]);

	       addr++;
	       f3x += (f2xd<<2);
	       f3y += (f2yd<<2);
	    }

	    f2x += f2xd;
	    f2y += f2yd;
	 }
      }

      f1x += f1xd;
      f1y += f1yd;
   }
}



/* get_rle_sprite:
 *  Creates a run length encoded sprite based on the specified bitmap.
 *  The returned sprite is likely to be a lot smaller than the original
 *  bitmap, and can be drawn to the screen with draw_rle_sprite().
 *
 *  The compression is done individually for each line of the image.
 *  Format is a series of command bytes, 1-127 marks a run of that many
 *  solid pixels, negative numbers mark a gap of -n pixels, and 0 marks
 *  the end of a line (since zero can't occur anywhere else in the data,
 *  this can be used to find the start of a specified line when clipping).
 *  For truecolor RLE sprites, the data and command bytes are both in the
 *  same format (16 or 32 bits, 24 bpp data is padded to 32 bit aligment), 
 *  and the mask color (bright pink) is used as the EOL marker.
 */
RLE_SPRITE *get_rle_sprite(BITMAP *bitmap)
{
   int depth = bitmap_color_depth(bitmap);
   RLE_SPRITE *s;
   signed char *p8;
   signed short *p16;
   signed long *p32;
   int x, y;
   int run;
   int c;

   #define WRITE_TO_SPRITE8(x) {                                             \
      _grow_scratch_mem(c+1);                                                \
      p8 = (signed char *)_scratch_mem;                                      \
      p8[c] = x;                                                             \
      c++;                                                                   \
   }

   #define WRITE_TO_SPRITE16(x) {                                            \
      _grow_scratch_mem((c+1)*2);                                            \
      p16 = (signed short *)_scratch_mem;                                    \
      p16[c] = x;                                                            \
      c++;                                                                   \
   }

   #define WRITE_TO_SPRITE32(x) {                                            \
      _grow_scratch_mem((c+1)*4);                                            \
      p32 = (signed long *)_scratch_mem;                                     \
      p32[c] = x;                                                            \
      c++;                                                                   \
   }

   c = 0;
   p8 = (signed char *)_scratch_mem;
   p16 = (signed short *)_scratch_mem;
   p32 = (signed long *)_scratch_mem;

   switch (depth) {

      case 8:
	 /* build a 256 color RLE sprite */
	 for (y=0; y<bitmap->h; y++) { 
	    run = -1;
	    for (x=0; x<bitmap->w; x++) { 
	       if (getpixel(bitmap, x, y)) {
		  if ((run >= 0) && (p8[run] > 0) && (p8[run] < 127))
		     p8[run]++;
		  else {
		     run = c;
		     WRITE_TO_SPRITE8(1);
		  }
		  WRITE_TO_SPRITE8(getpixel(bitmap, x, y));
	       }
	       else {
		  if ((run >= 0) && (p8[run] < 0) && (p8[run] > -128))
		     p8[run]--;
		  else {
		     run = c;
		     WRITE_TO_SPRITE8(-1);
		  }
	       }
	    }
	    WRITE_TO_SPRITE8(0);
	 }
	 break;


   #ifdef ALLEGRO_COLOR16

      case 15:
      case 16:
	 /* build a 15 or 16 bit hicolor RLE sprite */
	 for (y=0; y<bitmap->h; y++) { 
	    run = -1;
	    for (x=0; x<bitmap->w; x++) { 
	       if (getpixel(bitmap, x, y) != bitmap->vtable->mask_color) {
		  if ((run >= 0) && (p16[run] > 0) && (p16[run] < 127))
		     p16[run]++;
		  else {
		     run = c;
		     WRITE_TO_SPRITE16(1);
		  }
		  WRITE_TO_SPRITE16(getpixel(bitmap, x, y));
	       }
	       else {
		  if ((run >= 0) && (p16[run] < 0) && (p16[run] > -128))
		     p16[run]--;
		  else {
		     run = c;
		     WRITE_TO_SPRITE16(-1);
		  }
	       }
	    }
	    WRITE_TO_SPRITE16(bitmap->vtable->mask_color);
	 }
	 c *= 2;
	 break;

   #endif


   #if (defined ALLEGRO_COLOR24) || (defined ALLEGRO_COLOR32)

      case 24:
      case 32:
	 /* build a 24 or 32 bit truecolor RLE sprite */
	 for (y=0; y<bitmap->h; y++) { 
	    run = -1;
	    for (x=0; x<bitmap->w; x++) { 
	       if (getpixel(bitmap, x, y) != bitmap->vtable->mask_color) {
		  if ((run >= 0) && (p32[run] > 0) && (p32[run] < 127))
		     p32[run]++;
		  else {
		     run = c;
		     WRITE_TO_SPRITE32(1);
		  }
		  WRITE_TO_SPRITE32(getpixel(bitmap, x, y));
	       }
	       else {
		  if ((run >= 0) && (p32[run] < 0) && (p32[run] > -128))
		     p32[run]--;
		  else {
		     run = c;
		     WRITE_TO_SPRITE32(-1);
		  }
	       }
	    }
	    WRITE_TO_SPRITE32(bitmap->vtable->mask_color);
	 }
	 c *= 4;
	 break;

   #endif

   }

   s = malloc(sizeof(RLE_SPRITE) + c);

   if (s) {
      s->w = bitmap->w;
      s->h = bitmap->h;
      s->color_depth = depth;
      s->size = c;
      memcpy(s->dat, _scratch_mem, c);
   }

   return s;
}



/* destroy_rle_sprite:
 *  Destroys an RLE sprite structure returned by get_rle_sprite().
 */
void destroy_rle_sprite(RLE_SPRITE *sprite)
{
   if (sprite)
      free(sprite);
}



/* compile_sprite:
 *  Helper function for making compiled sprites.
 */
static void *compile_sprite(BITMAP *b, int l, int planar, int *len)
{
   int x, y;
   int offset;
   int run;
   int run_pos;
   int compiler_pos = 0;
   int xc = planar ? 4 : 1;
   unsigned long *addr;
   void *p;

   #ifdef ALLEGRO_COLOR16
      unsigned short *p16;
   #endif

   #if defined ALLEGRO_COLOR24 || defined ALLEGRO_COLOR32
      unsigned long *p32;
   #endif

   for (y=0; y<b->h; y++) {

      /* for linear bitmaps, time for some bank switching... */
      if (!planar) {
	 COMPILER_MOV_EDI_EAX();
	 COMPILER_CALL_ESI();
	 COMPILER_ADD_ECX_EAX();
      }

      offset = 0;
      x = l;

      /* compile a line of the sprite */
      while (x < b->w) {

	 switch (bitmap_color_depth(b)) {

	    #ifdef ALLEGRO_COLOR16

	    case 15:
	    case 16:
	       /* 16 bit version */
	       p16 = (unsigned short *)b->line[y];

	       if (p16[x] != b->vtable->mask_color) {
		  run = 0;
		  run_pos = x;

		  while ((run_pos<b->w) && (p16[run_pos] != b->vtable->mask_color)) {
		     run++;
		     run_pos++;
		  }

		  while (run >= 2) {
		     COMPILER_MOVL_IMMED(offset, ((int)p16[x]) |
						 ((int)p16[x+1] << 16));
		     x += 2;
		     offset += 4;
		     run -= 2;
		  }

		  if (run > 0) {
		     COMPILER_MOVW_IMMED(offset, p16[x]);
		     x++;
		     offset += 2;
		     run--;
		  }
	       }
	       else {
		  x++;
		  offset += 2;
	       }
	       break;

	    #endif


	    #ifdef ALLEGRO_COLOR24

	    case 24:
	       /* 24 bit version */
	       p32 = (unsigned long *)b->line[y];

	       if (((*((unsigned long *)(((unsigned char *)p32)+x*3)))&0xFFFFFF) != (unsigned)b->vtable->mask_color) {
		  run = 0;
		  run_pos = x;

		  while ((run_pos<b->w) && (((*((unsigned long *)(((unsigned char *)p32)+run_pos*3)))&0xFFFFFF) != (unsigned)b->vtable->mask_color)) {
		     run++;
		     run_pos++;
		  }

		  while (run >= 4) {
		     addr = ((unsigned long *)(((unsigned char *)p32)+x*3));
		     COMPILER_MOVL_IMMED(offset, addr[0]);
		     offset += 4;
		     COMPILER_MOVL_IMMED(offset, addr[1]);
		     offset += 4;
		     COMPILER_MOVL_IMMED(offset, addr[2]);
		     offset += 4;
		     x += 4;
		     run -= 4;
		  }

		  switch (run) {

		     case 3:
			addr = ((unsigned long *)(((unsigned char *)p32)+x*3));
			COMPILER_MOVL_IMMED(offset, addr[0]);
			offset += 4;
			COMPILER_MOVL_IMMED(offset, addr[1]);
			offset += 4;
			COMPILER_MOVB_IMMED(offset, *(((unsigned short *)addr)+4));
			offset++;
			x += 3;
			run -= 3;
			break;

		     case 2:
			addr = ((unsigned long *)(((unsigned char *)p32)+x*3));
			COMPILER_MOVL_IMMED(offset, addr[0]);
			offset += 4;
			COMPILER_MOVW_IMMED(offset, *(((unsigned short *)addr)+2));
			offset += 2;
			x += 2;
			run -= 2;
			break;

		     case 1:
			addr = ((unsigned long *)((unsigned char *)p32+x*3));
			COMPILER_MOVW_IMMED(offset, (unsigned short )addr[0]);
			offset += 2;
			COMPILER_MOVB_IMMED(offset, *((unsigned char *)(((unsigned short *)addr)+1)));
			offset++;
			x++;
			run--;
			break;

		     default:
			break;
		  } 
	       }
	       else {
		  x++;
		  offset += 3;
	       }
	       break;

	    #endif


	    #ifdef ALLEGRO_COLOR32

	    case 32:
	       /* 32 bit version */
	       p32 = (unsigned long *)b->line[y];

	       if (p32[x] != (unsigned)b->vtable->mask_color) {
		  run = 0;
		  run_pos = x;

		  while ((run_pos<b->w) && (p32[run_pos] != (unsigned)b->vtable->mask_color)) {
		     run++;
		     run_pos++;
		  }

		  while (run > 0) {
		     COMPILER_MOVL_IMMED(offset, p32[x]);
		     x++;
		     offset += 4;
		     run--;
		  }
	       }
	       else {
		  x++;
		  offset += 4;
	       }
	       break;

	    #endif


	    default:
	       /* 8 bit version */
	       if (b->line[y][x]) {
		  run = 0;
		  run_pos = x;

		  while ((run_pos<b->w) && (b->line[y][run_pos])) {
		     run++;
		     run_pos += xc;
		  }

		  while (run >= 4) {
		     COMPILER_MOVL_IMMED(offset, ((int)b->line[y][x]) |
						 ((int)b->line[y][x+xc] << 8) |
						 ((int)b->line[y][x+xc*2] << 16) |
						 ((int)b->line[y][x+xc*3] << 24));
		     x += xc*4;
		     offset += 4;
		     run -= 4;
		  }

		  if (run >= 2) {
		     COMPILER_MOVW_IMMED(offset, ((int)b->line[y][x]) |
						 ((int)b->line[y][x+xc] << 8));
		     x += xc*2;
		     offset += 2;
		     run -= 2;
		  }

		  if (run > 0) {
		     COMPILER_MOVB_IMMED(offset, b->line[y][x]);
		     x += xc;
		     offset++;
		  }
	       }
	       else {
		  x += xc;
		  offset++;
	       }
	       break;
	 }
      }

      /* move on to the next line */
      if (y+1 < b->h) {
	 if (planar) {
	    COMPILER_ADD_ECX_EAX();
	 }
	 else {
	    COMPILER_INC_EDI();
	 }
      }
   }

   COMPILER_RET();

   p = malloc(compiler_pos);
   if (p) {
      memcpy(p, _scratch_mem, compiler_pos);
      *len = compiler_pos;
   }

   return p;
}



/* get_compiled_sprite:
 *  Creates a compiled sprite based on the specified bitmap.
 */
COMPILED_SPRITE *get_compiled_sprite(BITMAP *bitmap, int planar)
{
   COMPILED_SPRITE *s;
   int plane;

   s = malloc(sizeof(COMPILED_SPRITE));
   if (!s)
      return NULL;

   s->planar = planar;
   s->color_depth = bitmap_color_depth(bitmap);
   s->w = bitmap->w;
   s->h = bitmap->h;

   for (plane=0; plane<4; plane++) {
      s->proc[plane].draw = NULL;
      s->proc[plane].len = 0;
   }

   for (plane=0; plane < (planar ? 4 : 1); plane++) {
      s->proc[plane].draw = compile_sprite(bitmap, plane, planar, &s->proc[plane].len);

      if (!s->proc[plane].draw) {
	 destroy_compiled_sprite(s);
	 return NULL;
      }
   }

   return s;
}



/* destroy_compiled_sprite:
 *  Destroys a compiled sprite structure returned by get_compiled_sprite().
 */
void destroy_compiled_sprite(COMPILED_SPRITE *sprite)
{
   int plane;

   if (sprite) {
      for (plane=0; plane<4; plane++)
	 if (sprite->proc[plane].draw)
	    free(sprite->proc[plane].draw);

      free(sprite);
   }
}

