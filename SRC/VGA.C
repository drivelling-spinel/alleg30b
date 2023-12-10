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
 *      Video driver for VGA mode 13h (320x200x256).
 *
 *      320x100 mode contributed by Salvador Eduardo Tropea.
 *
 *      See readme.txt for copyright information.
 */


#include <stdlib.h>

#ifdef DJGPP
#include <dpmi.h>
#include <go32.h>
#include <sys/segments.h>
#endif

#include "allegro.h"
#include "internal.h"


static BITMAP *vga_init(int w, int h, int v_w, int v_h, int color_depth);
static int vga_scroll(int x, int y);



GFX_DRIVER gfx_vga = 
{
   "VGA", 
   "Mode 13h", 
   vga_init,
   NULL,
   NULL,          /* can't hardware scroll in 13h */
   _vga_vsync,
   _vga_set_pallete_range,
   320, 200,
   TRUE,          /* no need for bank switches */
   0, 0,
   0x10000,
   0
};



/* vga_init:
 *  Selects mode 13h and creates a screen bitmap for it.
 */
static BITMAP *vga_init(int w, int h, int v_w, int v_h, int color_depth)
{
   BITMAP *b;

   #ifdef DJGPP
      __dpmi_regs r;
   #endif

   /* check it is a valid resolution */
   if (color_depth != 8) {
      strcpy(allegro_error, "VGA mode 13h only supports 8 bit color");
      return NULL;
   }

   if ((w != 320) || ((h != 200) && (h != 100)) || (v_w > 320) || (v_h > 200)) {
      strcpy(allegro_error, "VGA mode 13h only supports 320x200 and 320x100");
      return NULL;
   }

   b = _make_bitmap(320, 200, 0xA0000, &gfx_vga, 8, 320);
   if (!b)
      return NULL;

   gfx_vga.h = h;

   #ifdef DJGPP
      /* djgpp graphics mode set */
      r.x.ax = 0x13;
      __dpmi_int(0x10, &r);

      if (h == 100) {
	 outportb(0x3D4, 9);
	 outportb(0x3D5, inportb(0x3D5) | 0x80);
	 gfx_vga.scroll = vga_scroll;
      }
      else
	 gfx_vga.scroll = NULL;

   #else
      /* linux version */

   #endif

   return b;
}



/* vga_scroll:
 *  Hardware scrolling routine for standard VGA modes.
 */
static int vga_scroll(int x, int y)
{
   long a = (x + (y * VIRTUAL_W)) / 4;

   DISABLE();

   _vsync_out_h();

   /* write to VGA address registers */
   _write_vga_register(_crtc, 0x0D, a & 0xFF);
   _write_vga_register(_crtc, 0x0C, (a>>8) & 0xFF);

   ENABLE();

   _vsync_in();

   return 0;
}

