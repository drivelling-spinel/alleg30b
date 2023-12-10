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
 *      Table of functions for drawing onto 8 bit linear bitmaps.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "internal.h"



void _linear_draw_sprite8_end();
void _linear_blit8_end();


GFX_VTABLE __linear_vtable8 =
{
   BMP_TYPE_LINEAR,
   8,
   MASK_COLOR_8,

   _linear_getpixel8,
   _linear_putpixel8,
   _linear_vline8,
   _linear_hline8,
   _normal_line,
   _normal_rectfill,
   _linear_draw_sprite8,
   _linear_draw_sprite8,
   _linear_draw_sprite_v_flip8,
   _linear_draw_sprite_h_flip8,
   _linear_draw_sprite_vh_flip8,
   _linear_draw_trans_sprite8,
   _linear_draw_lit_sprite8,
   _linear_draw_rle_sprite8,
   _linear_draw_trans_rle_sprite8,
   _linear_draw_lit_rle_sprite8,
   _linear_draw_character8,
   _linear_textout_fixed8,
   _linear_blit8,
   _linear_blit8,
   _linear_blit8,
   _linear_blit8,
   _linear_blit_backward8,
   _linear_masked_blit8,
   _linear_clear_to_color8,
   _linear_draw_sprite8_end,
   _linear_blit8_end
};

