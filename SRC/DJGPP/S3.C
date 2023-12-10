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
 *      Video driver for S3 graphics cards.
 *
 *      See readme.txt for copyright information.
 */


#ifndef DJGPP
#error This file should only be used by the djgpp version of Allegro
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pc.h>
#include <dpmi.h>
#include <go32.h>
#include <sys/movedata.h>
#include <sys/farptr.h>

#include "allegro.h"
#include "internal.h"


static BITMAP *s3_init(int w, int h, int v_w, int v_h, int color_depth);
static int s3_scroll(int x, int y);



GFX_DRIVER gfx_s3 = 
{
   "S3",
   "",
   s3_init,
   NULL,
   s3_scroll,
   _vga_vsync,
   _vga_set_pallete_range,
   0, 0, FALSE, 
   0x10000,       /* 64k banks */
   0x10000,       /* 64k granularity */
   0, 0
};



static _GFX_MODE_INFO s3_mode_list[] =
{
   {  640,  400,  8,    0x100,  0x4F02,   NULL  },
   {  640,  480,  8,    0x101,  0x4F02,   NULL  },
   {  800,  600,  8,    0x103,  0x4F02,   NULL  },
   {  1024, 768,  8,    0x105,  0x4F02,   NULL  },
   {  0,    0,    0,    0,      0,        NULL  }
};



/* s3_detect:
 *  Detects the presence of a S3 card.
 */
static int s3_detect()
{
   int old;

   old = _read_vga_register(_crtc, 0x38);
   _write_vga_register(_crtc, 0x38, 0);                  /* disable ext. */
   if (!_test_vga_register(_crtc, 0x35, 0xF)) {          /* test */
      _write_vga_register(_crtc, 0x38, 0x48);            /* enable ext. */
      if (_test_vga_register(_crtc, 0x35, 0xF)) {        /* test again */
	 gfx_s3.vid_mem = _vesa_vidmem_check(1024*1024);
	 return TRUE;                                    /* found it */
      }
   }

   _write_vga_register(_crtc, 0x38, old);
   return FALSE;
}



/* s3_init:
 *  Tries to enter the specified graphics mode, and makes a screen bitmap
 *  for it.
 */
static BITMAP *s3_init(int w, int h, int v_w, int v_h, int color_depth)
{
   BITMAP *b;

   b = _gfx_mode_set_helper(w, h, v_w, v_h, color_depth, &gfx_s3, 
			    s3_detect, s3_mode_list, NULL);
   if (!b)
      return NULL;

   LOCK_FUNCTION(_s3_bank);

   b->read_bank = b->write_bank = _s3_bank;

   return b;
}



/* s3_scroll:
 *  Hardware scrolling for S3 cards.
 */
static int s3_scroll(int x, int y)
{
   long a = x + (y * VIRTUAL_W);

   DISABLE();

   _vsync_out_h();

   /* write high bits to S3-specific registers */
   _write_vga_register(_crtc, 0x38, 0x48);
   _alter_vga_register(_crtc, 0x31, 0x30, a>>14);
   _write_vga_register(_crtc, 0x38, 0);

   /* write to normal VGA address registers */
   _write_vga_register(_crtc, 0x0D, (a>>2) & 0xFF);
   _write_vga_register(_crtc, 0x0C, (a>>10) & 0xFF);

   ENABLE();

   _vsync_in();

   return 0;
}

