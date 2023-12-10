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
 *      Video driver running on top of linux's svgalib.
 *
 *      See readme.txt for copyright information.
 */


#ifndef LINUX
#error This file should only be used by the linux version of Allegro
#endif

#include <stdlib.h>
#include <stdio.h>

#include "allegro.h"
#include "internal.h"


static char svgalib_desc[128] = "";

static BITMAP *svgalib_init(int w, int h, int v_w, int v_h);
static void svgalib_exit(BITMAP *b);
static void svgalib_vsync();
static int svgalib_scroll(int x, int y);
static void svgalib_set_pallete_range(PALLETE p, int from, int to, int vsync);



GFX_DRIVER gfx_svgalib = 
{
   "SVGALIB",
   svgalib_desc,
   svgalib_init,
   svgalib_exit,
   svgalib_scroll,
   svgalib_vsync,
   svgalib_set_pallete_range,
   0, 0, FALSE, 0, 0, 0, 0
};



/* svgalib_init:
 *  Tries to enter the specified graphics mode, and makes a screen bitmap
 *  for it.
 */
static BITMAP *svgalib_init(int w, int h, int v_w, int v_h)
{
   return NULL;
}



/* svgalib_scroll:
 *  Hardware scrolling routine.
 */
static int svgalib_scroll(int x, int y)
{
   return 0;
}



/* svgalib_vsync:
 *  svgalib vsync routine.
 */
static void svgalib_vsync()
{
}



/* svgalib_set_pallete_range:
 *  Uses svgalib functions to set the pallete.
 */
static void svgalib_set_pallete_range(PALLETE p, int from, int to, int vsync)
{
}



/* svgalib_exit:
 *  Shuts down the svgalib driver.
 */
static void svgalib_exit(BITMAP *b)
{
}


