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
 *      Video driver for Tseng ET3000 and ET4000 graphics cards.
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


static BITMAP *et3000_init(int w, int h, int v_w, int v_h, int color_depth);
static BITMAP *et4000_init(int w, int h, int v_w, int v_h, int color_depth);
static int tseng_scroll(int x, int y);


#define ET_NONE         0
#define ET_3000         1
#define ET_4000         2

static int tseng_type = ET_NONE;



GFX_DRIVER gfx_et3000 = 
{
   "Tseng ET3000",
   "",
   et3000_init,
   NULL,
   tseng_scroll,
   _vga_vsync,
   _vga_set_pallete_range,
   0, 0, FALSE, 
   0x10000,       /* 64k banks */
   0x10000,       /* 64k granularity */
   0, 0
};



static _GFX_MODE_INFO et3000_mode_list[] =
{
   {  640,  350,  8,    0x2D,  0,   NULL  },
   {  640,  480,  8,    0x2E,  0,   NULL  },
   {  800,  600,  8,    0x30,  0,   NULL  },
   {  0,    0,    0,    0,     0,   NULL  }
};



GFX_DRIVER gfx_et4000 = 
{
   "Tseng ET4000",
   "",
   et4000_init,
   NULL,
   tseng_scroll,
   _vga_vsync,
   _vga_set_pallete_range,
   0, 0, FALSE, 
   0x10000,       /* 64k banks */
   0x10000,       /* 64k granularity */
   0, 0
};



static _GFX_MODE_INFO et4000_mode_list[] =
{
   {  640,  350,  8,    0x2D,  0,   NULL  },
   {  640,  480,  8,    0x2E,  0,   NULL  },
   {  640,  400,  8,    0x2F,  0,   NULL  },
   {  800,  600,  8,    0x30,  0,   NULL  },
   {  1024, 768,  8,    0x38,  0,   NULL  },
   {  0,    0,    0,    0,     0,   NULL  }
};



/* tseng_detect:
 *  Detects the presence of a Tseng card. Returns one of the ET_* constants.
 */
static int tseng_detect()
{
   int old1, old2;

   old1 = inportb(0x3BF);
   old2 = inportb(_crtc+4);

   outportb(0x3BF, 3);
   outportb(_crtc+4, 0xA0);                           /* enable extensions */

   if (_test_register(0x3CD, 0x3F)) {
      if (!_test_vga_register(_crtc, 0x33, 0x0F))
	 return ET_3000;
      else
	 return ET_4000;
   }

   outportb(0x3BF, old1);
   outportb(_crtc+4, old2);
   return ET_NONE;
}



/* et3000 detect:
 *  Detects the presence of a Tseng ET3000.
 */
static int et3000_detect()
{
   if (tseng_detect() != ET_3000)
      return FALSE;

   gfx_et3000.vid_mem = _vesa_vidmem_check(512*1024);
   return TRUE;
}



/* et4000 detect:
 *  Detects the presence of a Tseng ET4000.
 */
static int et4000_detect()
{
   if (tseng_detect() != ET_4000)
      return FALSE;

   gfx_et4000.vid_mem = _vesa_vidmem_check(1024*1024);
   return TRUE;
}



/* et3000_init:
 *  Tries to enter the specified graphics mode, and makes a screen bitmap
 *  for it.
 */
static BITMAP *et3000_init(int w, int h, int v_w, int v_h, int color_depth)
{
   BITMAP *b;

   b = _gfx_mode_set_helper(w, h, v_w, v_h, color_depth, &gfx_et3000, 
			    et3000_detect, et3000_mode_list, NULL);
   if (!b)
      return NULL;

   LOCK_FUNCTION(_et3000_write_bank);
   LOCK_FUNCTION(_et3000_read_bank);

   b->write_bank = _et3000_write_bank;
   b->read_bank = _et3000_read_bank;

   return b;
}



/* et4000_init:
 *  Tries to enter the specified graphics mode, and makes a screen bitmap
 *  for it.
 */
static BITMAP *et4000_init(int w, int h, int v_w, int v_h, int color_depth)
{
   BITMAP *b;

   b = _gfx_mode_set_helper(w, h, v_w, v_h, color_depth, &gfx_et4000, 
			    et4000_detect, et4000_mode_list, NULL);
   if (!b)
      return NULL;

   LOCK_FUNCTION(_et4000_write_bank);
   LOCK_FUNCTION(_et4000_read_bank);

   b->write_bank = _et4000_write_bank;
   b->read_bank =_et4000_read_bank;

   return b;
}



/* tseng_scroll:
 *  Hardware scrolling for Tseng cards.
 */
static int tseng_scroll(int x, int y)
{
   long a = x + (y * VIRTUAL_W);

   DISABLE();

   _vsync_out_h();

   /* write high bit(s) to Tseng-specific registers */
   if (tseng_type == ET_3000)
      _alter_vga_register(_crtc, 0x23, 2, a>>17);
   else
      _alter_vga_register(_crtc, 0x33, 3, a>>18);

   /* write to normal VGA address registers */
   _write_vga_register(_crtc, 0x0D, (a>>2) & 0xFF);
   _write_vga_register(_crtc, 0x0C, (a>>10) & 0xFF);

   ENABLE();

   /* write low 2 bits to VGA horizontal pan register */
   _write_hpp(a&3);

   return 0;
}


