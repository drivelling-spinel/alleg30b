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
 *      Video driver for Cirrus 64xx and 54xx graphics cards.
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


static BITMAP *cirrus_64_init(int w, int h, int v_w, int v_h, int color_depth);
static int cirrus_64_scroll(int x, int y);

static BITMAP *cirrus_54_init(int w, int h, int v_w, int v_h, int color_depth);
static int cirrus_54_scroll(int x, int y);



GFX_DRIVER gfx_cirrus64 = 
{
   "Cirrus 64xx",
   "",
   cirrus_64_init,
   NULL,
   cirrus_64_scroll,
   _vga_vsync,
   _vga_set_pallete_range,
   0, 0, FALSE, 
   0x10000,       /* 64k banks */
   0x1000,        /* 4k granularity */
   0, 0
};



static _GFX_MODE_INFO cirrus_64_mode_list[] =
{
   {  640,  400,  8,    0x2D,  0,   NULL  },
   {  640,  480,  8,    0x2E,  0,   NULL  },
   {  800,  600,  8,    0x30,  0,   NULL  },
   {  0,    0,    0,    0,     0,   NULL  }
};



GFX_DRIVER gfx_cirrus54 = 
{
   "Cirrus 54xx",
   "",
   cirrus_54_init,
   NULL,
   cirrus_54_scroll,
   _vga_vsync,
   _vga_set_pallete_range,
   0, 0, FALSE, 
   0x10000,       /* 64k banks */
   0x1000,        /* 4k granularity */
   0, 0
};



static _GFX_MODE_INFO cirrus_54_mode_list[] =
{
   {  640,  480,  8,    0x5F,  0,   NULL  },
   {  800,  600,  8,    0x5C,  0,   NULL  },
   {  1024, 768,  8,    0x60,  0,   NULL  },
   {  0,    0,    0,    0,     0,   NULL  }
};



/* cirrus_64_detect:
 *  Detects the presence of a Cirrus 64xx card.
 */
static int cirrus_64_detect()
{
   int old;

   old = _read_vga_register(0x3CE, 0xA);
   _write_vga_register(0x3CE, 0xA, 0xCE);             /* disable extensions */
   if (_read_vga_register(0x3CE, 0xA) == 0) {
      _write_vga_register(0x3CE, 0xA, 0xEC);          /* enable extensions */
      if (_read_vga_register(0x3CE, 0xA) == 1) {
	 gfx_cirrus64.vid_mem = _vesa_vidmem_check(1024*1024);
	 return TRUE;
      }
   }

   _write_vga_register(0x3CE, 0xA, old);
   return FALSE;
}



/* cirrus_64_init:
 *  Tries to enter the specified graphics mode, and makes a screen bitmap
 *  for it.
 */
static BITMAP *cirrus_64_init(int w, int h, int v_w, int v_h, int color_depth)
{
   BITMAP *b;

   b = _gfx_mode_set_helper(w, h, v_w, v_h, color_depth, &gfx_cirrus64, 
			    cirrus_64_detect, cirrus_64_mode_list, NULL);
   if (!b)
      return NULL;

   LOCK_FUNCTION(_cirrus64_read_bank);
   LOCK_FUNCTION(_cirrus64_write_bank);

   _alter_vga_register(0x3CE, 0xD, 4, 4);       /* enable read/write banks */

   b->write_bank = _cirrus64_write_bank;
   b->read_bank = _cirrus64_read_bank;

   return b;
}



/* cirrus_64_scroll:
 *  Hardware scrolling for Cirrus 64xx cards.
 */
static int cirrus_64_scroll(int x, int y)
{
   long a = x + (y * VIRTUAL_W);

   DISABLE();

   _vsync_out_h();

   /* write high bits to Cirrus 64xx registers */
   _write_vga_register(0x3CE, 0x7C, a>>18);

   /* write to normal VGA address registers */
   _write_vga_register(_crtc, 0x0D, (a>>2) & 0xFF);
   _write_vga_register(_crtc, 0x0C, (a>>10) & 0xFF);

   ENABLE();

   _vsync_in();

   return 0;
}



/* cirrus_54_detect:
 *  Detects the presence of a Cirrus 54xx card.
 */
static int cirrus_54_detect()
{
   int old;

   old = _read_vga_register(0x3C4, 6);
   _write_vga_register(0x3C4, 6, 0);                  /* disable extensions */
   if (_read_vga_register(0x3C4, 6) == 0xF) {
      _write_vga_register(0x3C4, 6, 0x12);            /* enable extensions */
      if ((_read_vga_register(0x3C4, 6) == 0x12) &&
	  (_test_vga_register(0x3C4, 0x1E, 0x3F))) {
	 gfx_cirrus54.vid_mem = _vesa_vidmem_check(1024*1024);
	 return TRUE;
      }
   }

   _write_vga_register(0x3C4, 6, old);
   return FALSE;
}



/* cirrus_54_init:
 *  Tries to enter the specified graphics mode, and makes a screen bitmap
 *  for it.
 */
static BITMAP *cirrus_54_init(int w, int h, int v_w, int v_h, int color_depth)
{
   BITMAP *b;

   b = _gfx_mode_set_helper(w, h, v_w, v_h, color_depth, &gfx_cirrus54, 
			    cirrus_54_detect, cirrus_54_mode_list, NULL);
   if (!b)
      return NULL;

   LOCK_FUNCTION(_cirrus54_bank);

   _alter_vga_register(0x3CE, 0xB, 33, 0);      /* 4k banks, single page */

   b->read_bank = b->write_bank = _cirrus54_bank;

   return b;
}



/* cirrus_54_scroll:
 *  Hardware scrolling for Cirrus 54xx cards.
 */
static int cirrus_54_scroll(int x, int y)
{
   long a = x + (y * VIRTUAL_W);
   long t;

   DISABLE();

   _vsync_out_h();

   /* write high bits to Cirrus 54xx registers */
   t = a >> 18;
   t += (t&6);    /* funny format, uses bits 0, 2, and 3 */
   _alter_vga_register(_crtc, 0x1B, 0xD, t);

   /* write to normal VGA address registers */
   _write_vga_register(_crtc, 0x0D, (a>>2) & 0xFF);
   _write_vga_register(_crtc, 0x0C, (a>>10) & 0xFF);

   ENABLE();

   /* write low 2 bits to VGA horizontal pan register */
   _write_hpp(a&3);

   return 0;
}

