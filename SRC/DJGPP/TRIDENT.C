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
 *      Video driver for Trident graphics cards.
 *
 *      Support for newer Trident chipsets added by Mark Habersack.
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


static BITMAP *trident_init(int w, int h, int v_w, int v_h, int color_depth);
static int trident_scroll(int x, int y);

static int chip_id = 0;

static char *_descriptions[] =
{
  "TVGA 8900CL/D", "TVGA 9000i", "TVGA 9200CXr",
  "TVGA LCD9100B", "TVGA GUI9420", "TVGA LX8200", "TVGA 9400CXi",
  "TVGA LCD9320", "Oops! Don't know...", "TVGA GUI9420", "TVGA GUI9660",
  "TVGA GUI9440", "TVGA GUI9430", "TVGA 9000C"
};

#define TV_LAST   13



GFX_DRIVER gfx_trident = 
{
   "Trident",
   "",
   trident_init,
   NULL,
   trident_scroll,
   _vga_vsync,
   _vga_set_pallete_range,
   0, 0, FALSE, 
   0x10000,       /* 64k banks */
   0x10000,       /* 64k granularity */
   0, 0
};



static _GFX_MODE_INFO trident_mode_list[] =
{
   {  640,  400,  8,    0x5C,  0,   NULL  },
   {  640,  480,  8,    0x5D,  0,   NULL  },
   {  800,  600,  8,    0x5E,  0,   NULL  },
   {  1024, 768,  8,    0x62,  0,   NULL  },
   {  0,    0,    0,    0,     0,   NULL  }
};



/* trident_detect:
 *  Detects the presence of a Trident card.
 */
static int trident_detect()
{
   int old1, old2, val;

   old1 = _read_vga_register(0x3C4, 0x0B);
   _write_vga_register(0x3C4, 0x0B, 0);      /* force old mode registers */
   chip_id = inportb(0x3C5);                 /* now we have the ID */

   old2 = _read_vga_register(0x3C4, 0x0E);
   outportb(0x3C4+1, old2^0x55);
   val = inportb(0x3C5);
   outportb(0x3C5, old2);

   if (((val^old2) & 0x0F) == 7) {           /* if bit 2 is inverted... */
      outportb(0x3C5, old2^2);               /* we're dealing with Trident */

      if (chip_id <= 2)                      /* don't like 8800 chips */
	 return FALSE;

      val = _read_vga_register(_crtc, 0x1F); /* determine the memory size */
      switch (val & 3) {
	 case 0: gfx_trident.vid_mem = 256*1024; break;
	 case 1: gfx_trident.vid_mem = 512*1024; break;
	 case 2: gfx_trident.vid_mem = 768*1024; break;
	 case 3: if ((chip_id >= 0x33) && (val & 4))
		    gfx_trident.vid_mem = 2048*1024;
		 else
		    gfx_trident.vid_mem = 1024*1024;
		 break;
      }

      /* provide user with a description of the chip s/he has */
      if ((chip_id == 0x33) && (_read_vga_register(_crtc, 0x28) & 0x80))
	 /* is it TVGA 9000C */
	 gfx_trident.desc = _descriptions[TV_LAST]; 
      else if (chip_id >= 0x33)
	 gfx_trident.desc = _descriptions[((chip_id & 0xF0) >> 4) - 3];
      else {
	 switch (chip_id) {
	    case 3:     gfx_trident.desc = "TR 8900B";         break;
	    case 4:     gfx_trident.desc = "TVGA 8900C";       break;
	    case 0x13:  gfx_trident.desc = "TVGA 8900C";       break;
	    case 0x23:  gfx_trident.desc = "TR 9000";          break;
	    default:    gfx_trident.desc = "Unknown Trident";  break;
	 }
      } 

      return TRUE;
   }

   _write_vga_register(0x3C4, 0x0B, old1);
   return FALSE;
}



/* trident_init:
 *  Tries to enter the specified graphics mode, and makes a screen bitmap
 *  for it.
 */
static BITMAP *trident_init(int w, int h, int v_w, int v_h, int color_depth)
{
   BITMAP *b;

   b = _gfx_mode_set_helper(w, h, v_w, v_h, color_depth, &gfx_trident, 
			    trident_detect, trident_mode_list, NULL);
   if (!b)
      return NULL;

   LOCK_FUNCTION(_trident_bank);
   LOCK_FUNCTION(_trident_write_bank);
   LOCK_FUNCTION(_trident_read_bank);

   if (chip_id >= 0x33) {
      _write_vga_register(0x3CE, 0x0F, 5);         /* read/write banks */

      b->write_bank = _trident_write_bank;
      b->read_bank = _trident_read_bank;
   }
   else {
      _read_vga_register(0x3C4, 0x0B);             /* switch to new mode */

      b->write_bank = b->read_bank = _trident_bank;
   }

   return b;
}



/* trident_scroll:
 *  Hardware scrolling for Trident cards.
 */
static int trident_scroll(int x, int y)
{
   long addr;

   addr = (y * VIRTUAL_W + x) >> 2;
   _vsync_out_h();

   /* first set the standard CRT Start registers */
   outportw(0x3D4, (addr & 0xFF00) | 0x0C);
   outportw(0x3D4, ((addr & 0x00FF) << 8) | 0x0D);

   /* set bit 16 of the screen start address */
   outportb(0x3D4, 0x1E);
   outportb(0x3D5, (inportb(0x3D5) & 0xDF) | ((addr & 0x10000) >> 11) );

   /* bits 17-19 of the start address: uses the 8900CL/D+ register */
   outportb(0x3D4, 0x27);
   outportb(0x3D5, (inportb(0x3D5) & 0xF8) | ((addr & 0xE0000) >> 17) );

   /* set pel register */
   _write_hpp(x&3);

   return 0;
}


