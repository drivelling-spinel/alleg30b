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
 *      Table of functions for drawing onto 16 bit linear bitmaps.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "internal.h"



#ifdef ALLEGRO_COLOR16


void _linear_draw_sprite16_end();
void _linear_blit16_end();


GFX_VTABLE __linear_vtable16 =
{
   BMP_TYPE_LINEAR,
   16,
   MASK_COLOR_16,

   _linear_getpixel16,
   _linear_putpixel16,
   _linear_vline16,
   _linear_hline16,
   _normal_line,
   _normal_rectfill,
   _linear_draw_sprite16,
   _linear_draw_256_sprite16,
   _linear_draw_sprite_v_flip16,
   _linear_draw_sprite_h_flip16,
   _linear_draw_sprite_vh_flip16,
   _linear_draw_trans_sprite16,
   _linear_draw_lit_sprite16,
   _linear_draw_rle_sprite16,
   _linear_draw_trans_rle_sprite16,
   _linear_draw_lit_rle_sprite16,
   _linear_draw_character16,
   _linear_textout_fixed16,
   _linear_blit16,
   _linear_blit16,
   _linear_blit16,
   _linear_blit16,
   _linear_blit_backward16,
   _linear_masked_blit16,
   _linear_clear_to_color16,
   _linear_draw_sprite16_end,
   _linear_blit16_end
};


#endif

