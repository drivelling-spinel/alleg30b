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
 *      Driver for Paradise graphics cards.
 *
 *      Contributed by Francois Charton.
 *
 *      See readme.txt for copyright information.
 */


#ifndef DJGPP
#error This file should only be used by the djgpp version of Allegro
#endif

#include "allegro.h"
#include "internal.h"


static BITMAP *paradise_init(int w, int h, int v_w, int v_h, int color_depth);
static int paradise_scroll(int x, int y);


#define PVGA1  1
#define WD90C  2

static int paradise_type = 0;


GFX_DRIVER gfx_paradise = 
{
   "Paradise",
   "Any WD card, at the moment",
   paradise_init,
   NULL,
   paradise_scroll,
   _vga_vsync,
   _vga_set_pallete_range,
   0, 0, FALSE, 
   0x10000,       /* 64k banks */
   0x1000,        /* 4k granularity */
   0, 0
};


static _GFX_MODE_INFO paradise_mode_list[] =
{
   {  640,  400,  8,    0x5E,  0,   NULL  },
   {  640,  480,  8,    0x5F,  0,   NULL  },
   {  800,  600,  8,    0x5C,  0,   NULL  },
   {  0,    0,    0,    0,     0,   NULL  }
};



/* paradise_detect:
 *  Detects the presence of a Paradise card.
 */
static int paradise_detect()
{
   int old, old2;
   __dpmi_regs r;

   old = _read_vga_register(0x3CE, 0xF);
   _write_vga_register(0x3CE, 0xF, old | 0x17); /* lock extended registers */

   if (_test_vga_register(0x3CE, 0x9, 0x7F)) {  /* not a Paradise card! */
      _write_vga_register(0x3CE, 0xF, old);
      return FALSE;
   }

   _alter_vga_register(0x3CE, 0xF, 0x17, 5);    /* unlock extended regs */

   if (!_test_vga_register(0x3CE, 0x9, 0x7F)) { /* not a Paradise card! */
      _write_vga_register(0x3CE, 0xF, old);
      return FALSE;
   }

   old2 = _read_vga_register(0x3D4, 0x29);
   _alter_vga_register(0x3D4, 0x29, 0x8F, 0x85); 

   if (!_test_vga_register(0x3D4, 0x2B, 0xFF)) {
      paradise_type = PVGA1;
      gfx_paradise.desc = "PVGA1";
      goto end;
   }

   _write_vga_register(0x3C4, 0x06, 0x48);

   if (!_test_vga_register(0x3C4, 0x7, 0xF0)) {
      paradise_type = PVGA1;
      gfx_paradise.desc = "WD90C0x";
      goto end;
   }

   if (!_test_vga_register(0x3C4, 0x10, 0xFF)) {
      paradise_type = PVGA1;
      gfx_paradise.desc = "WD90C2x";
      _write_vga_register(0x3D4, 0x34, 0xA6);
      if (_read_vga_register(0x3D4, 0x32) & 0x20) 
	 _write_vga_register(0x3D4, 0x34, 0); 
      goto end;
   }

   paradise_type = WD90C;
   gfx_paradise.desc = "WD90C1x or 24+";

   end:

   _write_vga_register(0x3D4, 0x29, old2);
   _write_vga_register(0x3CE, 0xF, old);

   r.x.ax = 0x007F; 
   r.h.bh = 0x02;
   __dpmi_int(0x10, &r);
   gfx_paradise.vid_mem = r.h.ch * 64 * 1024;

   return TRUE;
}



/* paradise_init:
 *  Tries to enter the specified graphics mode, and makes a screen bitmap
 *  for it.
 */
static BITMAP *paradise_init(int w, int h, int v_w, int v_h, int color_depth)
{
   BITMAP *b;

   b = _gfx_mode_set_helper(w, h, v_w, v_h, color_depth, &gfx_paradise, 
			    paradise_detect, paradise_mode_list, NULL);
   if (!b)
      return NULL;

   if(paradise_type == PVGA1) { 
      /* Paradise and Cirrus use the same bank switch mechanism */ 
      LOCK_FUNCTION(_cirrus54_bank); 
      b->write_bank = b->read_bank = _cirrus54_bank;
   }
   else {
      LOCK_FUNCTION(_paradise_read_bank);
      LOCK_FUNCTION(_paradise_write_bank);

      _write_vga_register(0x3C4, 0x06, 0x48);

      _alter_vga_register(0x3C4, 0x11, 0x80, 0x80);
      _alter_vga_register(0x3CE, 0x0B, 0x80, 0x80);

      _write_vga_register(0x3C4, 0x06, 0x00); 

      b->write_bank = _paradise_write_bank;
      b->read_bank = _paradise_read_bank;
   }

   return b;
}



/* paradise_scroll:
 *  Hardware scrolling for Paradise cards.
 */
static int paradise_scroll(int x, int y)
{
   long a = x + (y * VIRTUAL_W);

   DISABLE();

   _vsync_out_h();

   /* write high bits to Paradise register 3CE indx 0xD, bits 3-4 */
   _alter_vga_register(0x3CE, 0x0D, 0x18, a>>15);

   /* write to normal VGA address registers */
   _write_vga_register(_crtc, 0x0D, (a>>2) & 0xFF);
   _write_vga_register(_crtc, 0x0C, (a>>10) & 0xFF);

   ENABLE();

   _vsync_in();

   return 0;
}
