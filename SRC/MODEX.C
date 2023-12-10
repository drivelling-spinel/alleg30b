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
 *      Video driver for VGA tweaked modes (aka mode-X).
 *
 *      Jonathan Tarbox wrote the mode set code.
 *
 *      TBD/FeR added the 320x600 and 360x600 modes.
 *
 *      See readme.txt for copyright information.
 */


#include <stdlib.h>

#ifdef DJGPP
#include <pc.h>
#include <dpmi.h>
#include <go32.h>
#include <sys/segments.h>
#include <sys/farptr.h>
#endif

#include "allegro.h"
#include "internal.h"


extern void _x_draw_sprite_end();
extern void _x_blit_from_memory_end();
extern void _x_blit_to_memory_end();


/* table of functions for drawing onto the mode-X screen */
GFX_VTABLE __modex_vtable =
{
   BMP_TYPE_PLANAR,
   8,
   MASK_COLOR_8,

   _x_getpixel,
   _x_putpixel,
   _x_vline,
   _x_hline,
   _normal_line,
   _normal_rectfill,
   _x_draw_sprite,
   _x_draw_sprite,
   _x_draw_sprite_v_flip,
   _x_draw_sprite_h_flip,
   _x_draw_sprite_vh_flip,
   _x_draw_trans_sprite,
   _x_draw_lit_sprite,
   _x_draw_rle_sprite,
   _x_draw_trans_rle_sprite,
   _x_draw_lit_rle_sprite,
   _x_draw_character,
   _x_textout_fixed,
   _x_blit_from_memory,
   _x_blit_to_memory,
   _x_blit,
   _x_blit_forward,
   _x_blit_backward,
   _x_masked_blit,
   _x_clear_to_color,
   _x_blit_from_memory_end,
   _x_draw_sprite_end
};



static BITMAP *modex_init(int w, int h, int v_w, int v_h, int color_depth);
static int modex_scroll(int x, int y);



GFX_DRIVER gfx_modex =
{
   "Mode-X",
   "Unchained VGA mode",
   modex_init,
   NULL,
   modex_scroll,
   _vga_vsync,
   _vga_set_pallete_range,
   0, 0,                   /* width and height are variable */
   TRUE,                   /* no need for bank switches */
   0, 0,
   0x40000,                /* standard 256k VGA memory */
   0
};



#ifdef DJGPP               /* xtended mode not supported under linux */


static BITMAP *xtended_init(int w, int h, int v_w, int v_h, int color_depth);


GFX_DRIVER gfx_xtended =
{
   "Xtended mode",
   "Unchained SVGA 640x400 mode",
   xtended_init,
   NULL,
   NULL,                   /* no hardware scrolling */
   _vga_vsync,
   _vga_set_pallete_range,
   640, 400,
   TRUE,                   /* no need for bank switches */
   0, 0,
   0x40000,                /* standard 256k VGA memory */
   0
};


#endif                     /* ifdef djgpp */



/* VGA register contents for the various tweaked modes */
typedef struct VGA_REGISTER
{
   unsigned short port;
   unsigned char index;
   unsigned char value;
} VGA_REGISTER;



static VGA_REGISTER mode_256x200[] =
{
   { 0x3C2, 0x0,  0xE3 },  { 0x3D4, 0x0,  0x5F },  { 0x3D4, 0x1,  0x3F },
   { 0x3D4, 0x2,  0x40 },  { 0x3D4, 0x3,  0x82 },  { 0x3D4, 0x4,  0x4E },
   { 0x3D4, 0x5,  0x96 },  { 0x3D4, 0x6,  0xBF },  { 0x3D4, 0x7,  0x1F },
   { 0x3D4, 0x8,  0x0  },  { 0x3D4, 0x9,  0x41 },  { 0x3D4, 0x10, 0x9C },
   { 0x3D4, 0x11, 0x8E },  { 0x3D4, 0x12, 0x8F },  { 0x3D4, 0x14, 0x0  }, 
   { 0x3D4, 0x15, 0x96 },  { 0x3D4, 0x16, 0xB9 },  { 0x3D4, 0x17, 0xE3 }, 
   { 0x3C4, 0x1,  0x1  },  { 0x3CE, 0x5,  0x40 },  { 0x3CE, 0x6,  0x5  }, 
   { 0x3C0, 0x10, 0x41 },  { 0,     0,    0    } 
};



static VGA_REGISTER mode_256x224[] =
{
   { 0x3C2, 0x0,  0xE3 },  { 0x3D4, 0x0,  0x5F },  { 0x3D4, 0x1,  0x3F },
   { 0x3D4, 0x2,  0x40 },  { 0x3D4, 0x3,  0x82 },  { 0x3D4, 0x4,  0x4A },
   { 0x3D4, 0x5,  0x9A },  { 0x3D4, 0x6,  0xB  },  { 0x3D4, 0x7,  0x3E },
   { 0x3D4, 0x8,  0x0  },  { 0x3D4, 0x9,  0x41 },  { 0x3D4, 0x10, 0xDA },
   { 0x3D4, 0x11, 0x9C },  { 0x3D4, 0x12, 0xBF },  { 0x3D4, 0x14, 0x0  }, 
   { 0x3D4, 0x15, 0xC7 },  { 0x3D4, 0x16, 0x4  },  { 0x3D4, 0x17, 0xE3 }, 
   { 0x3C4, 0x1,  0x1  },  { 0x3CE, 0x5,  0x40 },  { 0x3CE, 0x6,  0x5  }, 
   { 0x3C0, 0x10, 0x41 },  { 0,     0,    0    } 
};



static VGA_REGISTER mode_256x240[] =
{
   { 0x3C2, 0x0,  0xE3 },  { 0x3D4, 0x0,  0x5F },  { 0x3D4, 0x1,  0x3F },
   { 0x3D4, 0x2,  0x40 },  { 0x3D4, 0x3,  0x82 },  { 0x3D4, 0x4,  0x4E },
   { 0x3D4, 0x5,  0x96 },  { 0x3D4, 0x6,  0xD  },  { 0x3D4, 0x7,  0x3E },
   { 0x3D4, 0x8,  0x0  },  { 0x3D4, 0x9,  0x41 },  { 0x3D4, 0x10, 0xEA },
   { 0x3D4, 0x11, 0xAC },  { 0x3D4, 0x12, 0xDF },  { 0x3D4, 0x14, 0x0  }, 
   { 0x3D4, 0x15, 0xE7 },  { 0x3D4, 0x16, 0x6  },  { 0x3D4, 0x17, 0xE3 }, 
   { 0x3C4, 0x1,  0x1  },  { 0x3CE, 0x5,  0x40 },  { 0x3CE, 0x6,  0x5  }, 
   { 0x3C0, 0x10, 0x41 },  { 0,     0,    0    } 
};



static VGA_REGISTER mode_256x256[] =
{
   { 0x3C2, 0x0,  0xE3 },  { 0x3D4, 0x0,  0x5F },  { 0x3D4, 0x1,  0x3F },
   { 0x3D4, 0x2,  0x40 },  { 0x3D4, 0x3,  0x82 },  { 0x3D4, 0x4,  0x4A },
   { 0x3D4, 0x5,  0x9A },  { 0x3D4, 0x6,  0x23 },  { 0x3D4, 0x7,  0xB2 },
   { 0x3D4, 0x8,  0x0  },  { 0x3D4, 0x9,  0x61 },  { 0x3D4, 0x10, 0xA  },
   { 0x3D4, 0x11, 0xAC },  { 0x3D4, 0x12, 0xFF },  { 0x3D4, 0x14, 0x0  }, 
   { 0x3D4, 0x15, 0x7  },  { 0x3D4, 0x16, 0x1A },  { 0x3D4, 0x17, 0xE3 }, 
   { 0x3C4, 0x1,  0x1  },  { 0x3CE, 0x5,  0x40 },  { 0x3CE, 0x6,  0x5  }, 
   { 0x3C0, 0x10, 0x41 },  { 0,     0,    0    } 
};



static VGA_REGISTER mode_320x200[] =
{
   { 0x3C2, 0x0,  0x63 },  { 0x3D4, 0x0,  0x5F },  { 0x3D4, 0x1,  0x4F },
   { 0x3D4, 0x2,  0x50 },  { 0x3D4, 0x3,  0x82 },  { 0x3D4, 0x4,  0x54 },
   { 0x3D4, 0x5,  0x80 },  { 0x3D4, 0x6,  0xBF },  { 0x3D4, 0x7,  0x1F },
   { 0x3D4, 0x8,  0x0  },  { 0x3D4, 0x9,  0x41 },  { 0x3D4, 0x10, 0x9C },
   { 0x3D4, 0x11, 0x8E },  { 0x3D4, 0x12, 0x8F },  { 0x3D4, 0x14, 0x0  }, 
   { 0x3D4, 0x15, 0x96 },  { 0x3D4, 0x16, 0xB9 },  { 0x3D4, 0x17, 0xE3 }, 
   { 0x3C4, 0x1,  0x1  },  { 0x3CE, 0x5,  0x40 },  { 0x3CE, 0x6,  0x5  }, 
   { 0x3C0, 0x10, 0x41 },  { 0,     0,    0    } 
};



static VGA_REGISTER mode_320x240[] =
{
   { 0x3C2, 0x0,  0xE3 },  { 0x3D4, 0x0,  0x5F },  { 0x3D4, 0x1,  0x4F },
   { 0x3D4, 0x2,  0x50 },  { 0x3D4, 0x3,  0x82 },  { 0x3D4, 0x4,  0x54 },
   { 0x3D4, 0x5,  0x80 },  { 0x3D4, 0x6,  0xD  },  { 0x3D4, 0x7,  0x3E },
   { 0x3D4, 0x8,  0x0  },  { 0x3D4, 0x9,  0x41 },  { 0x3D4, 0x10, 0xEA },
   { 0x3D4, 0x11, 0xAC },  { 0x3D4, 0x12, 0xDF },  { 0x3D4, 0x14, 0x0  }, 
   { 0x3D4, 0x15, 0xE7 },  { 0x3D4, 0x16, 0x6  },  { 0x3D4, 0x17, 0xE3 }, 
   { 0x3C4, 0x1,  0x1  },  { 0x3CE, 0x5,  0x40 },  { 0x3CE, 0x6,  0x5  }, 
   { 0x3C0, 0x10, 0x41 },  { 0,     0,    0    } 
};



static VGA_REGISTER mode_320x400[] =
{
   { 0x3C2, 0x0,  0x63 },  { 0x3D4, 0x0,  0x5F },  { 0x3D4, 0x1,  0x4F },
   { 0x3D4, 0x2,  0x50 },  { 0x3D4, 0x3,  0x82 },  { 0x3D4, 0x4,  0x54 },
   { 0x3D4, 0x5,  0x80 },  { 0x3D4, 0x6,  0xBF },  { 0x3D4, 0x7,  0x1F },
   { 0x3D4, 0x8,  0x0  },  { 0x3D4, 0x9,  0x40 },  { 0x3D4, 0x10, 0x9C },
   { 0x3D4, 0x11, 0x8E },  { 0x3D4, 0x12, 0x8F },  { 0x3D4, 0x14, 0x0  }, 
   { 0x3D4, 0x15, 0x96 },  { 0x3D4, 0x16, 0xB9 },  { 0x3D4, 0x17, 0xE3 }, 
   { 0x3C4, 0x1,  0x1  },  { 0x3CE, 0x5,  0x40 },  { 0x3CE, 0x6,  0x5  }, 
   { 0x3C0, 0x10, 0x41 },  { 0,     0,    0    } 
};



static VGA_REGISTER mode_320x480[] =
{
   { 0x3C2, 0x0,  0xE3 },  { 0x3D4, 0x0,  0x5F },  { 0x3D4, 0x1,  0x4F },
   { 0x3D4, 0x2,  0x50 },  { 0x3D4, 0x3,  0x82 },  { 0x3D4, 0x4,  0x54 },
   { 0x3D4, 0x5,  0x80 },  { 0x3D4, 0x6,  0xD  },  { 0x3D4, 0x7,  0x3E },
   { 0x3D4, 0x8,  0x0  },  { 0x3D4, 0x9,  0x40 },  { 0x3D4, 0x10, 0xEA },
   { 0x3D4, 0x11, 0xAC },  { 0x3D4, 0x12, 0xDF },  { 0x3D4, 0x14, 0x0  }, 
   { 0x3D4, 0x15, 0xE7 },  { 0x3D4, 0x16, 0x6  },  { 0x3D4, 0x17, 0xE3 }, 
   { 0x3C4, 0x1,  0x1  },  { 0x3CE, 0x5,  0x40 },  { 0x3CE, 0x6,  0x5  }, 
   { 0x3C0, 0x10, 0x41 },  { 0,     0,    0    } 
};



static VGA_REGISTER mode_320x600[] =
{
   { 0x3C2, 0x00, 0xE7 },  { 0x3D4, 0x00, 0x5F },  { 0x3D4, 0x01, 0x4F },
   { 0x3D4, 0x02, 0x50 },  { 0x3D4, 0x03, 0x82 },  { 0x3D4, 0x04, 0x54 },
   { 0x3D4, 0x05, 0x80 },  { 0x3D4, 0x06, 0x70 },  { 0x3D4, 0x07, 0xF0 },
   { 0x3D4, 0x08, 0x00 },  { 0x3D4, 0x09, 0x60 },  { 0x3D4, 0x10, 0x5B },
   { 0x3D4, 0x11, 0x8C },  { 0x3D4, 0x12, 0x57 },  { 0x3D4, 0x13, 0x28 },
   { 0x3D4, 0x14, 0x00 },  { 0x3D4, 0x15, 0x58 },  { 0x3D4, 0x16, 0x70 },
   { 0x3D4, 0x17, 0xE3 },  { 0x3C4, 0x01, 0x01 },  { 0x3C4, 0x04, 0x06 },
   { 0x3CE, 0x05, 0x40 },  { 0x3CE, 0x06, 0x05 },  { 0x3C0, 0x10, 0x41 },
   { 0x3C0, 0x13, 0x00 },  { 0,     0,    0    }
};



static VGA_REGISTER mode_360x200[] =
{
   { 0x3C2, 0x0,  0x67 },  { 0x3D4, 0x0,  0x6B },  { 0x3D4, 0x1,  0x59 },
   { 0x3D4, 0x2,  0x5A },  { 0x3D4, 0x3,  0x8E },  { 0x3D4, 0x4,  0x5E },
   { 0x3D4, 0x5,  0x8A },  { 0x3D4, 0x6,  0xBF },  { 0x3D4, 0x7,  0x1F },
   { 0x3D4, 0x8,  0x0  },  { 0x3D4, 0x9,  0x41 },  { 0x3D4, 0x10, 0x9C },
   { 0x3D4, 0x11, 0x8E },  { 0x3D4, 0x12, 0x8F },  { 0x3D4, 0x14, 0x0  }, 
   { 0x3D4, 0x15, 0x96 },  { 0x3D4, 0x16, 0xB9 },  { 0x3D4, 0x17, 0xE3 }, 
   { 0x3C4, 0x1,  0x1  },  { 0x3CE, 0x5,  0x40 },  { 0x3CE, 0x6,  0x5  }, 
   { 0x3C0, 0x10, 0x41 },  { 0,     0,    0    } 
};



static VGA_REGISTER mode_360x240[] =
{
   { 0x3C2, 0x0,  0xE7 },  { 0x3D4, 0x0,  0x6B },  { 0x3D4, 0x1,  0x59 },
   { 0x3D4, 0x2,  0x5A },  { 0x3D4, 0x3,  0x8E },  { 0x3D4, 0x4,  0x5E },
   { 0x3D4, 0x5,  0x8A },  { 0x3D4, 0x6,  0xD  },  { 0x3D4, 0x7,  0x3E },
   { 0x3D4, 0x8,  0x0  },  { 0x3D4, 0x9,  0x41 },  { 0x3D4, 0x10, 0xEA },
   { 0x3D4, 0x11, 0xAC },  { 0x3D4, 0x12, 0xDF },  { 0x3D4, 0x14, 0x0  }, 
   { 0x3D4, 0x15, 0xE7 },  { 0x3D4, 0x16, 0x6  },  { 0x3D4, 0x17, 0xE3 }, 
   { 0x3C4, 0x1,  0x1  },  { 0x3CE, 0x5,  0x40 },  { 0x3CE, 0x6,  0x5  }, 
   { 0x3C0, 0x10, 0x41 },  { 0,     0,    0    } 
};



static VGA_REGISTER mode_360x270[] =
{
   { 0x3C2, 0x0,  0xE7 },  { 0x3D4, 0x0,  0x6B },  { 0x3D4, 0x1,  0x59 },
   { 0x3D4, 0x2,  0x5A },  { 0x3D4, 0x3,  0x8E },  { 0x3D4, 0x4,  0x5E },
   { 0x3D4, 0x5,  0x8A },  { 0x3D4, 0x6,  0x30 },  { 0x3D4, 0x7,  0xF0 },
   { 0x3D4, 0x8,  0x0  },  { 0x3D4, 0x9,  0x61 },  { 0x3D4, 0x10, 0x20 },
   { 0x3D4, 0x11, 0xA9 },  { 0x3D4, 0x12, 0x1B },  { 0x3D4, 0x14, 0x0  }, 
   { 0x3D4, 0x15, 0x1F },  { 0x3D4, 0x16, 0x2F },  { 0x3D4, 0x17, 0xE3 }, 
   { 0x3C4, 0x1,  0x1  },  { 0x3CE, 0x5,  0x40 },  { 0x3CE, 0x6,  0x5  }, 
   { 0x3C0, 0x10, 0x41 },  { 0,     0,    0    } 
};



static VGA_REGISTER mode_360x360[] =
{
   { 0x3C2, 0x0,  0x67 },  { 0x3D4, 0x0,  0x6B },  { 0x3D4, 0x1,  0x59 },
   { 0x3D4, 0x2,  0x5A },  { 0x3D4, 0x3,  0x8E },  { 0x3D4, 0x4,  0x5E },
   { 0x3D4, 0x5,  0x8A },  { 0x3D4, 0x6,  0xBF },  { 0x3D4, 0x7,  0x1F },
   { 0x3D4, 0x8,  0x0  },  { 0x3D4, 0x9,  0x40 },  { 0x3D4, 0x10, 0x88 },
   { 0x3D4, 0x11, 0x85 },  { 0x3D4, 0x12, 0x67 },  { 0x3D4, 0x14, 0x0  }, 
   { 0x3D4, 0x15, 0x6D },  { 0x3D4, 0x16, 0xBA },  { 0x3D4, 0x17, 0xE3 }, 
   { 0x3C4, 0x1,  0x1  },  { 0x3CE, 0x5,  0x40 },  { 0x3CE, 0x6,  0x5  }, 
   { 0x3C0, 0x10, 0x41 },  { 0,     0,    0    } 
};



static VGA_REGISTER mode_360x400[] =
{
   { 0x3C2, 0x0,  0x67 },  { 0x3D4, 0x0,  0x6B },  { 0x3D4, 0x1,  0x59 },
   { 0x3D4, 0x2,  0x5A },  { 0x3D4, 0x3,  0x8E },  { 0x3D4, 0x4,  0x5E },
   { 0x3D4, 0x5,  0x8A },  { 0x3D4, 0x6,  0xBF },  { 0x3D4, 0x7,  0x1F },
   { 0x3D4, 0x8,  0x0  },  { 0x3D4, 0x9,  0x40 },  { 0x3D4, 0x10, 0x9C },
   { 0x3D4, 0x11, 0x8E },  { 0x3D4, 0x12, 0x8F },  { 0x3D4, 0x14, 0x0  }, 
   { 0x3D4, 0x15, 0x96 },  { 0x3D4, 0x16, 0xB9 },  { 0x3D4, 0x17, 0xE3 }, 
   { 0x3C4, 0x1,  0x1  },  { 0x3CE, 0x5,  0x40 },  { 0x3CE, 0x6,  0x5  }, 
   { 0x3C0, 0x10, 0x41 },  { 0,     0,    0    } 
};



static VGA_REGISTER mode_360x480[] =
{
   { 0x3C2, 0x0,  0xE7 },  { 0x3D4, 0x0,  0x6B },  { 0x3D4, 0x1,  0x59 },
   { 0x3D4, 0x2,  0x5A },  { 0x3D4, 0x3,  0x8E },  { 0x3D4, 0x4,  0x5E },
   { 0x3D4, 0x5,  0x8A },  { 0x3D4, 0x6,  0xD  },  { 0x3D4, 0x7,  0x3E },
   { 0x3D4, 0x8,  0x0  },  { 0x3D4, 0x9,  0x40 },  { 0x3D4, 0x10, 0xEA },
   { 0x3D4, 0x11, 0xAC },  { 0x3D4, 0x12, 0xDF },  { 0x3D4, 0x14, 0x0  }, 
   { 0x3D4, 0x15, 0xE7 },  { 0x3D4, 0x16, 0x6  },  { 0x3D4, 0x17, 0xE3 }, 
   { 0x3C4, 0x1,  0x1  },  { 0x3CE, 0x5,  0x40 },  { 0x3CE, 0x6,  0x5  }, 
   { 0x3C0, 0x10, 0x41 },  { 0,     0,    0    } 
};



static VGA_REGISTER mode_360x600[] =
{
   { 0x3C2, 0x00, 0xE7 },  { 0x3D4, 0x00, 0x5F },  { 0x3D4, 0x01, 0x59 },
   { 0x3D4, 0x02, 0x50 },  { 0x3D4, 0x03, 0x82 },  { 0x3D4, 0x04, 0x54 },
   { 0x3D4, 0x05, 0x80 },  { 0x3D4, 0x06, 0x70 },  { 0x3D4, 0x07, 0xF0 },
   { 0x3D4, 0x08, 0x00 },  { 0x3D4, 0x09, 0x60 },  { 0x3D4, 0x10, 0x5B },
   { 0x3D4, 0x11, 0x8C },  { 0x3D4, 0x12, 0x57 },  { 0x3D4, 0x13, 0x2D },
   { 0x3D4, 0x14, 0x00 },  { 0x3D4, 0x15, 0x58 },  { 0x3D4, 0x16, 0x70 },
   { 0x3D4, 0x17, 0xE3 },  { 0x3C4, 0x01, 0x01 },  { 0x3C4, 0x03, 0x00 },
   { 0x3C4, 0x04, 0x06 },  { 0x3CE, 0x05, 0x40 },  { 0x3CE, 0x06, 0x05 },
   { 0x3C0, 0x10, 0x41 },  { 0x3C0, 0x11, 0x00 },  { 0x3C0, 0x12, 0x0F },
   { 0x3C0, 0x13, 0x00 },  { 0x3C0, 0x14, 0x00 },  { 0,     0,    0    }
};



static VGA_REGISTER mode_376x282[] =
{
   { 0x3C2, 0x0,  0xE7 },  { 0x3D4, 0x0,  0x6E },  { 0x3D4, 0x1,  0x5D },
   { 0x3D4, 0x2,  0x5E },  { 0x3D4, 0x3,  0x91 },  { 0x3D4, 0x4,  0x62 },
   { 0x3D4, 0x5,  0x8F },  { 0x3D4, 0x6,  0x62 },  { 0x3D4, 0x7,  0xF0 },
   { 0x3D4, 0x8,  0x0  },  { 0x3D4, 0x9,  0x61 },  { 0x3D4, 0x10, 0x37 },
   { 0x3D4, 0x11, 0x89 },  { 0x3D4, 0x12, 0x33 },  { 0x3D4, 0x14, 0x0  }, 
   { 0x3D4, 0x15, 0x3C },  { 0x3D4, 0x16, 0x5C },  { 0x3D4, 0x17, 0xE3 }, 
   { 0x3C4, 0x1,  0x1  },  { 0x3CE, 0x5,  0x40 },  { 0x3CE, 0x6,  0x5  }, 
   { 0x3C0, 0x10, 0x41 },  { 0,     0,    0    } 
};



static VGA_REGISTER mode_376x308[] =
{
   { 0x3C2, 0x0,  0xE7 },  { 0x3D4, 0x0,  0x6E },  { 0x3D4, 0x1,  0x5D },
   { 0x3D4, 0x2,  0x5E },  { 0x3D4, 0x3,  0x91 },  { 0x3D4, 0x4,  0x62 },
   { 0x3D4, 0x5,  0x8F },  { 0x3D4, 0x6,  0x62 },  { 0x3D4, 0x7,  0x0F },
   { 0x3D4, 0x8,  0x0  },  { 0x3D4, 0x9,  0x40 },  { 0x3D4, 0x10, 0x37 },
   { 0x3D4, 0x11, 0x89 },  { 0x3D4, 0x12, 0x33 },  { 0x3D4, 0x14, 0x0  }, 
   { 0x3D4, 0x15, 0x3C },  { 0x3D4, 0x16, 0x5C },  { 0x3D4, 0x17, 0xE3 }, 
   { 0x3C4, 0x1,  0x1  },  { 0x3CE, 0x5,  0x40 },  { 0x3CE, 0x6,  0x5  }, 
   { 0x3C0, 0x10, 0x41 },  { 0,     0,    0    } 
};



static VGA_REGISTER mode_376x564[] =
{
   { 0x3C2, 0x0,  0xE7 },  { 0x3D4, 0x0,  0x6E },  { 0x3D4, 0x1,  0x5D },
   { 0x3D4, 0x2,  0x5E },  { 0x3D4, 0x3,  0x91 },  { 0x3D4, 0x4,  0x62 },
   { 0x3D4, 0x5,  0x8F },  { 0x3D4, 0x6,  0x62 },  { 0x3D4, 0x7,  0xF0 },
   { 0x3D4, 0x8,  0x0  },  { 0x3D4, 0x9,  0x60 },  { 0x3D4, 0x10, 0x37 },
   { 0x3D4, 0x11, 0x89 },  { 0x3D4, 0x12, 0x33 },  { 0x3D4, 0x14, 0x0  }, 
   { 0x3D4, 0x15, 0x3C },  { 0x3D4, 0x16, 0x5C },  { 0x3D4, 0x17, 0xE3 }, 
   { 0x3C4, 0x1,  0x1  },  { 0x3CE, 0x5,  0x40 },  { 0x3CE, 0x6,  0x5  }, 
   { 0x3C0, 0x10, 0x41 },  { 0,     0,    0    } 
};



static VGA_REGISTER mode_400x150[] =
{
   { 0x3C2, 0x0,  0xA7 },  { 0x3D4, 0x0,  0x71 },  { 0x3D4, 0x1,  0x63 },
   { 0x3D4, 0x2,  0x64 },  { 0x3D4, 0x3,  0x92 },  { 0x3D4, 0x4,  0x65 },
   { 0x3D4, 0x5,  0x82 },  { 0x3D4, 0x6,  0x46 },  { 0x3D4, 0x7,  0x1F },
   { 0x3D4, 0x8,  0x0  },  { 0x3D4, 0x9,  0x41 },  { 0x3D4, 0x10, 0x31 },
   { 0x3D4, 0x11, 0x80 },  { 0x3D4, 0x12, 0x2B },  { 0x3D4, 0x14, 0x0  }, 
   { 0x3D4, 0x15, 0x2F },  { 0x3D4, 0x16, 0x44 },  { 0x3D4, 0x17, 0xE3 }, 
   { 0x3C4, 0x1,  0x1  },  { 0x3C4, 0x2,  0xF  },  { 0x3CE, 0x5,  0x40 }, 
   { 0x3CE, 0x6,  0x5  },  { 0x3C0, 0x10, 0x41 },  { 0,     0,    0    }
};



static VGA_REGISTER mode_400x300[] =
{
   { 0x3C2, 0x0,  0xA7 },  { 0x3D4, 0x0,  0x71 },  { 0x3D4, 0x1,  0x63 },
   { 0x3D4, 0x2,  0x64 },  { 0x3D4, 0x3,  0x92 },  { 0x3D4, 0x4,  0x65 },
   { 0x3D4, 0x5,  0x82 },  { 0x3D4, 0x6,  0x46 },  { 0x3D4, 0x7,  0x1F },
   { 0x3D4, 0x8,  0x0  },  { 0x3D4, 0x9,  0x40 },  { 0x3D4, 0x10, 0x31 },
   { 0x3D4, 0x11, 0x80 },  { 0x3D4, 0x12, 0x2B },  { 0x3D4, 0x14, 0x0  }, 
   { 0x3D4, 0x15, 0x2F },  { 0x3D4, 0x16, 0x44 },  { 0x3D4, 0x17, 0xE3 }, 
   { 0x3C4, 0x1,  0x1  },  { 0x3C4, 0x2,  0xF  },  { 0x3CE, 0x5,  0x40 }, 
   { 0x3CE, 0x6,  0x5  },  { 0x3C0, 0x10, 0x41 },  { 0,     0,    0    }
};



static VGA_REGISTER mode_400x600[] =
{
   { 0x3C2, 0x0,  0xE7 },  { 0x3D4, 0x0,  0x70 },  { 0x3D4, 0x1,  0x63 },
   { 0x3D4, 0x2,  0x64 },  { 0x3D4, 0x3,  0x92 },  { 0x3D4, 0x4,  0x65 },
   { 0x3D4, 0x5,  0x82 },  { 0x3D4, 0x6,  0x70 },  { 0x3D4, 0x7,  0xF0 },
   { 0x3D4, 0x8,  0x0  },  { 0x3D4, 0x9,  0x60 },  { 0x3D4, 0x10, 0x5B },
   { 0x3D4, 0x11, 0x8C },  { 0x3D4, 0x12, 0x57 },  { 0x3D4, 0x14, 0x0  }, 
   { 0x3D4, 0x15, 0x58 },  { 0x3D4, 0x16, 0x70 },  { 0x3D4, 0x17, 0xE3 }, 
   { 0x3C4, 0x1,  0x1  },  { 0x3C4, 0x2,  0xF  },  { 0x3CE, 0x5,  0x40 }, 
   { 0x3CE, 0x6,  0x5  },  { 0x3C0, 0x10, 0x41 },  { 0,     0,    0    }
};



/* information about a mode-X resolution */
typedef struct TWEAKED_MODE
{
   int w, h;
   VGA_REGISTER *regs;
} TWEAKED_MODE;



static TWEAKED_MODE xmodes[] =
{
   {  256,  200,  mode_256x200  },
   {  256,  224,  mode_256x224  },
   {  256,  240,  mode_256x240  },
   {  256,  256,  mode_256x256  },
   {  320,  200,  mode_320x200  },
   {  320,  240,  mode_320x240  },
   {  320,  400,  mode_320x400  },
   {  320,  480,  mode_320x480  },
   {  320,  600,  mode_320x600  },
   {  360,  200,  mode_360x200  },
   {  360,  240,  mode_360x240  },
   {  360,  270,  mode_360x270  },
   {  360,  360,  mode_360x360  },
   {  360,  400,  mode_360x400  },
   {  360,  480,  mode_360x480  },
   {  360,  600,  mode_360x600  },
   {  376,  282,  mode_376x282  },
   {  376,  308,  mode_376x308  },
   {  376,  564,  mode_376x564  },
   {  400,  150,  mode_400x150  },
   {  400,  300,  mode_400x300  },
   {  400,  600,  mode_400x600  },
   {  0,    0,    NULL          }
};



extern void _x_draw_sprite_end();
extern void _x_blit_from_memory_end();
extern void _x_blit_to_memory_end();



/* modex_init:
 *  Selects a tweaked VGA mode and creates a screen bitmap for it.
 */
static BITMAP *modex_init(int w, int h, int v_w, int v_h, int color_depth)
{
   BITMAP *b;
   VGA_REGISTER *reg;
   TWEAKED_MODE *mode = xmodes;

   #ifdef DJGPP
      __dpmi_regs r;
   #endif

   /* check it is a valid resolution */
   if (color_depth != 8) {
      strcpy(allegro_error, "Mode-X only supports 8 bit color");
      return NULL;
   }

   /* search the mode table for the requested resolution */
   while ((mode->w != w) || (mode->h != h)) {
      if (!mode->regs) {
	 strcpy(allegro_error, "Not a VGA mode-X resolution");
	 return NULL;
      }

      mode++;
   }

   /* round up virtual size if required (v_w must be a multiple of eight) */
   v_w = MAX(w, (v_w+7)&0xFFF8);
   v_h = MAX(h, v_h);

   if (v_h > 0x40000/v_w) {
      strcpy(allegro_error, "Virtual screen size too large");
      return NULL;
   }

   /* calculate virtual height = vram / width */
   v_h = 0x40000/v_w;

   /* lock everything that is used to draw mouse pointers */
   LOCK_VARIABLE(__modex_vtable); 
   LOCK_FUNCTION(_x_draw_sprite);
   LOCK_FUNCTION(_x_blit_from_memory);
   LOCK_FUNCTION(_x_blit_to_memory);

   /* set mode 13h, then start tweaking things */
   #ifdef DJGPP
      /* djgpp graphics mode set */
      r.x.ax = 0x13; 
      __dpmi_int(0x10, &r);

   #else
      /* linux version */

   #endif

   outportw(0x3C4, 0x0100);                     /* synchronous reset */

   outportb(0x3D4, 0x11);                       /* enable crtc regs 0-7 */
   outportb(0x3D5, inportb(0x3D5) & 0x7F);

   outportw(0x3C4, 0x0604);                     /* disable chain-4 */

   for (reg=mode->regs; reg->port; reg++) {     /* set the VGA registers */
      if (reg->port == 0x3C0) {
	 inportb(0x3DA);
	 outportb(0x3C0, reg->index | 0x20);
	 outportb(0x3C0, reg->value);
      }
      else if (reg->port == 0x3C2) {
	 outportb(reg->port, reg->value);
      }
      else {
	 outportb(reg->port, reg->index); 
	 outportb(reg->port+1, reg->value);
      }
   }

   outportb(0x3D4, 0x13);                       /* set scanline length */
   outportb(0x3D5, v_w/8);

   outportw(0x3C4, 0x0300);                     /* restart sequencer */

   /* We only use 1/4th of the real width for the bitmap line pointers,
    * so they can be used directly for writing to vram, eg. the address
    * for pixel(x,y) is bmp->line[y]+(x/4). Divide the x coordinate, but
    * not the line pointer. The clipping position and bitmap width are 
    * stored in the linear pixel format, though, not as mode-X planes.
    */
   b = _make_bitmap(v_w/4, v_h, 0xA0000, &gfx_modex, 8, v_w/4);
   if (!b)
      return NULL;

   b->w = b->cr = v_w;
   b->vtable = &__modex_vtable;

   gfx_modex.w = w;
   gfx_modex.h = h;

   return b;
}



#ifdef DJGPP      /* xtended mode not supported under linux */


/* xtended_init:
 *  Selects the unchained 640x400 mode.
 */
static BITMAP *xtended_init(int w, int h, int v_w, int v_h, int color_depth)
{
   BITMAP *b;
   __dpmi_regs r;

   /* check it is a valid resolution */
   if (color_depth != 8) {
      strcpy(allegro_error, "Xtended mode only supports 8 bit color");
      return NULL;
   }

   if ((w != 640) || (h != 400) || (v_w > 640) || (v_h > 400)) {
      strcpy(allegro_error, "Xtended mode only supports 640x400");
      return NULL;
   }

   /* lock everything that is used to draw mouse pointers */
   LOCK_VARIABLE(__modex_vtable); 
   LOCK_FUNCTION(_x_draw_sprite);
   LOCK_FUNCTION(_x_blit_from_memory);
   LOCK_FUNCTION(_x_blit_to_memory);

   /* set VESA mode 0x100 */
   r.x.ax = 0x4F02;
   r.x.bx = 0x100;
   __dpmi_int(0x10, &r);
   if (r.h.ah) {
      strcpy(allegro_error, "VESA mode 0x100 not available");
      return NULL;
   }

   outportw(0x3C4, 0x0604);         /* disable chain-4 */

   /* we only use 1/4th of the width for the bitmap line pointers */
   b = _make_bitmap(640/4, 400, 0xA0000, &gfx_xtended, 8, 640/4);
   if (!b)
      return NULL;

   b->w = b->cr = 640;
   b->vtable = &__modex_vtable;

   return b;
}


#endif            /* ifdef djgpp */



/* modex_scroll:
 *  Hardware scrolling routine for mode-X.
 */
static int modex_scroll(int x, int y)
{
   long a = x + (y * VIRTUAL_W);

   DISABLE();

   _vsync_out_h();

   /* write to VGA address registers */
   _write_vga_register(_crtc, 0x0D, (a>>2) & 0xFF);
   _write_vga_register(_crtc, 0x0C, (a>>10) & 0xFF);

   ENABLE();

   /* write low 2 bits to horizontal pan register */
   _write_hpp((a&3)<<1);

   return 0;
}



/* request_modex_scroll:
 *  Requests a scroll but doesn't wait for it to be competed: just writes
 *  to some VGA registers and some variables used by the vertical retrace
 *  interrupt simulator, and then returns immediately. The scroll will
 *  actually take place during the next vertical retrace, so this function
 *  can be used together with poll_modex_scroll to implement triple buffered
 *  animation systems (see examples/ex20.c).
 */
void request_modex_scroll(int x, int y)
{
   long a = x + (y * VIRTUAL_W);

   if (gfx_driver != &gfx_modex)
      return;

   DISABLE();

   _vsync_out_h();

   /* write to VGA address registers */
   _write_vga_register(_crtc, 0x0D, (a>>2) & 0xFF);
   _write_vga_register(_crtc, 0x0C, (a>>10) & 0xFF);

   ENABLE();

   if (_timer_use_retrace) {
      /* store low 2 bits where the timer handler will find them */
      _retrace_hpp_value = (a&3)<<1;
   }
   else {
      /* change instantly */
      _write_hpp((a&3)<<1);
   }
}



/* poll_modex_scroll:
 *  Returns non-zero if there is a scroll waiting to take place, previously
 *  set by request_modex_scroll(). Only works if vertical retrace interrupts 
 *  are enabled.
 */
int poll_modex_scroll()
{
   if ((_retrace_hpp_value < 0) || (!_timer_use_retrace))
      return FALSE;

   return TRUE;
}



/* split_modex_screen:
 *  Enables a horizontal split screen at the specified line.
 *  Based on code from Paul Fenwick's XLIBDJ, which was in turn based 
 *  on Michael Abrash's routines in PC Techniques, June 1991.
 */
void split_modex_screen(int line)
{
   extern int _modex_split_position;

   if (gfx_driver != &gfx_modex)
      return;

   if (line < 0)
      line = 0;
   else if (line >= SCREEN_H)
      line = 0;

   _modex_split_position = line;

   /* adjust the line for double-scanned modes */
   if (SCREEN_H < 300)
      line <<= 1;

   /* disable panning of the split screen area */
   _alter_vga_register(0x3C0, 0x30, 0x20, 0x20);

   /* set the line compare registers */
   _write_vga_register(0x3D4, 0x18, (line-1) & 0xFF);
   _alter_vga_register(0x3D4, 7, 0x10, ((line-1) & 0x100) >> 4);
   _alter_vga_register(0x3D4, 9, 0x40, ((line-1) & 0x200) >> 3);
}



/* x_write:
 *  Inline helper for the C drawing functions: writes a pixel onto a
 *  mode-X bitmap, performing clipping but ignoring the drawing mode.
 */
static inline void x_write(BITMAP *bmp, int x, int y, int color)
{
   if ((x >= bmp->cl) && (x < bmp->cr) && (y >= bmp->ct) && (y < bmp->cb)) {
      outportw(0x3C4, (0x100<<(x&3))|2);
      _farnspokeb((unsigned long)bmp->line[y]+(x>>2), color);
   }
}



/* _x_vline:
 *  Draws a vertical line onto a mode-X screen.
 */
void _x_vline(BITMAP *bmp, int x, int y1, int y2, int color)
{
   int c;

   if (y1 > y2) {
      c = y1;
      y1 = y2;
      y2 = c;
   }

   for (c=y1; c<=y2; c++)
      _x_putpixel(bmp, x, c, color);
}



/* _x_draw_sprite_v_flip:
 *  Draws a vertically flipped sprite onto a mode-X screen.
 */
void _x_draw_sprite_v_flip(BITMAP *bmp, BITMAP *sprite, int x, int y)
{
   int c1, c2;

   _farsetsel(bmp->seg);

   for (c1=0; c1<sprite->h; c1++)
      for (c2=0; c2<sprite->w; c2++)
	 if (sprite->line[sprite->h-1-c1][c2])
	    x_write(bmp, x+c2, y+c1, sprite->line[sprite->h-1-c1][c2]);
}



/* _x_draw_sprite_h_flip:
 *  Draws a horizontally flipped sprite onto a mode-X screen.
 */
void _x_draw_sprite_h_flip(BITMAP *bmp, BITMAP *sprite, int x, int y)
{
   int c1, c2;

   _farsetsel(bmp->seg);

   for (c1=0; c1<sprite->h; c1++)
      for (c2=0; c2<sprite->w; c2++)
	 if (sprite->line[c1][sprite->w-1-c2])
	    x_write(bmp, x+c2, y+c1, sprite->line[c1][sprite->w-1-c2]);
}



/* _x_draw_sprite_vh_flip:
 *  Draws a diagonally flipped sprite onto a mode-X screen.
 */
void _x_draw_sprite_vh_flip(BITMAP *bmp, BITMAP *sprite, int x, int y)
{
   int c1, c2;

   _farsetsel(bmp->seg);

   for (c1=0; c1<sprite->h; c1++)
      for (c2=0; c2<sprite->w; c2++)
	 if (sprite->line[sprite->h-1-c1][sprite->w-1-c2])
	    x_write(bmp, x+c2, y+c1, sprite->line[sprite->h-1-c1][sprite->w-1-c2]);
}



/* _x_draw_trans_sprite:
 *  Draws a translucent sprite onto a mode-X screen.
 */
void _x_draw_trans_sprite(BITMAP *bmp, BITMAP *sprite, int x, int y)
{
   int sx, sy, sy2;
   int dx, dy;
   int xlen, ylen;
   int plane;
   unsigned char *src;
   unsigned long addr;

   sx = sy = 0;

   if (bmp->clip) {
      if (x < bmp->cl) {
	 sx = bmp->cl - x;
	 x = bmp->cl;
      }
      if (y < bmp->ct) {
	 sy = bmp->ct - y;
	 y = bmp->ct;
      }
      xlen = MIN(sprite->w - sx, bmp->cr - x);
      ylen = MIN(sprite->h - sy, bmp->cb - y);
   }
   else {
      xlen = sprite->w;
      ylen = sprite->h;
   }

   if ((xlen <= 0) || (ylen <= 0))
      return;

   _farsetsel(bmp->seg);

   sy2 = sy;

   for (plane=0; plane<4; plane++) {
      sy = sy2;

      outportw(0x3C4, (0x100<<((x+plane)&3))|2);
      outportw(0x3CE, (((x+plane)&3)<<8)|4);

      for (dy=0; dy<ylen; dy++) {
	 src = sprite->line[sy]+sx+plane;
	 addr = (unsigned long)bmp->line[y+dy]+((x+plane)>>2);

	 for (dx=plane; dx<xlen; dx+=4) {
	    _farnspokeb(addr, color_map->data[*src][_farnspeekb(addr)]);

	    addr++;
	    src+=4;
	 }

	 sy++;
      }
   }
}



/* _x_draw_lit_sprite:
 *  Draws a lit sprite onto a mode-X screen.
 */
void _x_draw_lit_sprite(BITMAP *bmp, BITMAP *sprite, int x, int y, int color)
{
   int sx, sy, sy2;
   int dx, dy;
   int xlen, ylen;
   int plane;
   unsigned char *src;
   unsigned long addr;

   sx = sy = 0;

   if (bmp->clip) {
      if (x < bmp->cl) {
	 sx = bmp->cl - x;
	 x = bmp->cl;
      }
      if (y < bmp->ct) {
	 sy = bmp->ct - y;
	 y = bmp->ct;
      }
      xlen = MIN(sprite->w - sx, bmp->cr - x);
      ylen = MIN(sprite->h - sy, bmp->cb - y);
   }
   else {
      xlen = sprite->w;
      ylen = sprite->h;
   }

   if ((xlen <= 0) || (ylen <= 0))
      return;

   _farsetsel(bmp->seg);

   sy2 = sy;

   for (plane=0; plane<4; plane++) {
      sy = sy2;

      outportw(0x3C4, (0x100<<((x+plane)&3))|2);

      for (dy=0; dy<ylen; dy++) {
	 src = sprite->line[sy]+sx+plane;
	 addr = (unsigned long)bmp->line[y+dy]+((x+plane)>>2);

	 for (dx=plane; dx<xlen; dx+=4) {
	    if (*src)
	       _farnspokeb(addr, color_map->data[color][*src]);

	    addr++;
	    src+=4;
	 }

	 sy++;
      }
   }
}



/* _x_draw_rle_sprite:
 *  Draws an RLE sprite onto a mode-X screen.
 */
void _x_draw_rle_sprite(BITMAP *bmp, RLE_SPRITE *sprite, int x, int y)
{
   signed char *p = sprite->dat;
   int c;
   int x_pos, y_pos;
   int lgap, width;
   unsigned long addr;

   _farsetsel(bmp->seg);
   y_pos = 0;

   /* clip on the top */
   while (y+y_pos < bmp->ct) {
      y_pos++;
      if ((y_pos >= sprite->h) || (y+y_pos >= bmp->cb))
	 return;

      while (*p)
	 p++;

      p++; 
   }

   /* x axis clip */
   lgap = MAX(bmp->cl-x, 0);
   width = MIN(sprite->w, bmp->cr-x);
   if (width <= lgap)
      return;

   /* draw it */
   while ((y_pos<sprite->h) && (y+y_pos < bmp->cb)) {
      addr = (unsigned long)bmp->line[y+y_pos];
      x_pos = 0;
      c = *(p++);

      /* skip pixels on the left */
      while (x_pos < lgap) {
	 if (c > 0) {
	    /* skip a run of solid pixels */
	    if (c > lgap-x_pos) {
	       p += lgap-x_pos;
	       c -= lgap-x_pos;
	       x_pos = lgap;
	       break;
	    }

	    x_pos += c;
	    p += c;
	 }
	 else {
	    /* skip a run of zeros */
	    if (-c > lgap-x_pos) {
	       c += lgap-x_pos;
	       x_pos = lgap;
	       break;
	    }

	    x_pos -= c;
	 }

	 c = *(p++);
      }

      /* draw a line of the sprite */
      for (;;) {
	 if (c > 0) {
	    /* draw a run of solid pixels */
	    c = MIN(c, width-x_pos);
	    while (c > 0) {
	       outportw(0x3C4, (0x100<<((x+x_pos)&3))|2);
	       _farnspokeb(addr+((x+x_pos)>>2), *p);
	       x_pos++;
	       p++;
	       c--;
	    }
	 }
	 else {
	    /* skip a run of zeros */
	    x_pos -= c;
	 }

	 if (x_pos < width)
	    c = (*p++);
	 else
	    break;
      }

      /* skip pixels on the right */
      if (x_pos < sprite->w) {
	 while (*p)
	    p++;

	 p++;
      }

      y_pos++;
   }
}



/* _x_draw_trans_rle_sprite:
 *  Draws an RLE sprite onto a mode-X screen.
 */
void _x_draw_trans_rle_sprite(BITMAP *bmp, RLE_SPRITE *sprite, int x, int y)
{
   signed char *p = sprite->dat;
   int c;
   int x_pos, y_pos;
   int lgap, width;
   unsigned long addr, a;

   _farsetsel(bmp->seg);
   y_pos = 0;

   /* clip on the top */
   while (y+y_pos < bmp->ct) {
      y_pos++;
      if ((y_pos >= sprite->h) || (y+y_pos >= bmp->cb))
	 return;

      while (*p)
	 p++;

      p++; 
   }

   /* x axis clip */
   lgap = MAX(bmp->cl-x, 0);
   width = MIN(sprite->w, bmp->cr-x);
   if (width <= lgap)
      return;

   /* draw it */
   while ((y_pos<sprite->h) && (y+y_pos < bmp->cb)) {
      addr = (unsigned long)bmp->line[y+y_pos];
      x_pos = 0;
      c = *(p++);

      /* skip pixels on the left */
      while (x_pos < lgap) {
	 if (c > 0) {
	    /* skip a run of solid pixels */
	    if (c > lgap-x_pos) {
	       p += lgap-x_pos;
	       c -= lgap-x_pos;
	       x_pos = lgap;
	       break;
	    }

	    x_pos += c;
	    p += c;
	 }
	 else {
	    /* skip a run of zeros */
	    if (-c > lgap-x_pos) {
	       c += lgap-x_pos;
	       x_pos = lgap;
	       break;
	    }

	    x_pos -= c;
	 }

	 c = *(p++);
      }

      /* draw a line of the sprite */
      for (;;) {
	 if (c > 0) {
	    /* draw a run of solid pixels */
	    c = MIN(c, width-x_pos);
	    while (c > 0) {
	       outportw(0x3C4, (0x100<<((x+x_pos)&3))|2);
	       outportw(0x3CE, (((x+x_pos)&3)<<8)|4);
	       a = addr+((x+x_pos)>>2);
	       _farnspokeb(a, color_map->data[*p][_farnspeekb(a)]);
	       x_pos++;
	       p++;
	       c--;
	    }
	 }
	 else {
	    /* skip a run of zeros */
	    x_pos -= c;
	 }

	 if (x_pos < width)
	    c = (*p++);
	 else
	    break;
      }

      /* skip pixels on the right */
      if (x_pos < sprite->w) {
	 while (*p)
	    p++;

	 p++;
      }

      y_pos++;
   }
}



/* _x_draw_lit_rle_sprite:
 *  Draws a tinted RLE sprite onto a mode-X screen.
 */
void _x_draw_lit_rle_sprite(BITMAP *bmp, RLE_SPRITE *sprite, int x, int y, int color)
{
   signed char *p = sprite->dat;
   int c;
   int x_pos, y_pos;
   int lgap, width;
   unsigned long addr;

   _farsetsel(bmp->seg);
   y_pos = 0;

   /* clip on the top */
   while (y+y_pos < bmp->ct) {
      y_pos++;
      if ((y_pos >= sprite->h) || (y+y_pos >= bmp->cb))
	 return;

      while (*p)
	 p++;

      p++; 
   }

   /* x axis clip */
   lgap = MAX(bmp->cl-x, 0);
   width = MIN(sprite->w, bmp->cr-x);
   if (width <= lgap)
      return;

   /* draw it */
   while ((y_pos<sprite->h) && (y+y_pos < bmp->cb)) {
      addr = (unsigned long)bmp->line[y+y_pos];
      x_pos = 0;
      c = *(p++);

      /* skip pixels on the left */
      while (x_pos < lgap) {
	 if (c > 0) {
	    /* skip a run of solid pixels */
	    if (c > lgap-x_pos) {
	       p += lgap-x_pos;
	       c -= lgap-x_pos;
	       x_pos = lgap;
	       break;
	    }

	    x_pos += c;
	    p += c;
	 }
	 else {
	    /* skip a run of zeros */
	    if (-c > lgap-x_pos) {
	       c += lgap-x_pos;
	       x_pos = lgap;
	       break;
	    }

	    x_pos -= c;
	 }

	 c = *(p++);
      }

      /* draw a line of the sprite */
      for (;;) {
	 if (c > 0) {
	    /* draw a run of solid pixels */
	    c = MIN(c, width-x_pos);
	    while (c > 0) {
	       outportw(0x3C4, (0x100<<((x+x_pos)&3))|2);
	       _farnspokeb(addr+((x+x_pos)>>2), color_map->data[color][*p]);
	       x_pos++;
	       p++;
	       c--;
	    }
	 }
	 else {
	    /* skip a run of zeros */
	    x_pos -= c;
	 }

	 if (x_pos < width)
	    c = (*p++);
	 else
	    break;
      }

      /* skip pixels on the right */
      if (x_pos < sprite->w) {
	 while (*p)
	    p++;

	 p++;
      }

      y_pos++;
   }
}



/* _x_draw_character:
 *  Draws a character from a proportional font onto a mode-X screen.
 */
void _x_draw_character(BITMAP *bmp, BITMAP *sprite, int x, int y, int color)
{
   int c1, c2;

   extern int _textmode;

   _farsetsel(bmp->seg);

   for (c1=0; c1<sprite->h; c1++) {
      for (c2=0; c2<sprite->w; c2++) {
	 if (sprite->line[c1][c2])
	    x_write(bmp, x+c2, y+c1, color);
	 else {
	    if (_textmode >= 0)
	       x_write(bmp, x+c2, y+c1, _textmode);
	 }
      }
   }
}



/* _x_textout_fixed:
 *  Draws a fixed size text string onto a mode-X screen.
 */
void _x_textout_fixed(BITMAP *bmp, void *f, int height, unsigned char *str, int x, int y, int color)
{
   unsigned char *dat;
   unsigned long addr;
   unsigned char *s;
   int c, plane;
   int x_pos;
   int y_count, y_min, y_max;

   extern int _textmode;

   /* clip y coordinates */
   y_min = MAX(0, bmp->ct - y);
   y_max = MIN((1 << height), bmp->cb - y);
   if (y_min >= y_max)
      return;

   /* trivially reject characters off the left */
   while (x+8 <= bmp->cl) {
      x += 8;
      str++;
      if (!*str)
	 return;
   }

   _farsetsel(bmp->seg);

   /* draw the string a plane at a time */
   for (plane=0; plane<4; plane++) {
      s = str;
      x_pos = x;

      /* set the write plane */
      outportw(0x3C4, (0x100<<((x_pos+plane)&3))|2);

      /* work through the string */
      while (*s) {
	 c = (int)*s - ' ';
	 if ((c < 0) || (c >= FONT_SIZE))
	    c = 0;

	 dat = (char *)f + (c << height) + y_min;

	 if ((x_pos+plane >= bmp->cl) && (x_pos+plane+4 < bmp->cr)) {
	    /* draw both bytes of the char (2 bytes * 4 planes = a font) */
	    for (y_count=y_min; y_count<y_max; y_count++) {
	       addr = (unsigned long)bmp->line[y+y_count] + ((x_pos+plane)>>2);

	       /* first pixel */
	       if (*dat & (0x80 >> plane))
		  _farnspokeb(addr, color);
	       else
		  if (_textmode >= 0)
		     _farnspokeb(addr, _textmode);

	       /* second pixel */
	       if (*dat & (0x80 >> (plane+4)))
		  _farnspokeb(addr+1, color);
	       else
		  if (_textmode >= 0)
		     _farnspokeb(addr+1, _textmode);

	       dat++;
	    }
	 }
	 else if ((x_pos+plane >= bmp->cl) && (x_pos+plane < bmp->cr)) {
	    /* draw only the leftmost byte of the character */
	    for (y_count=y_min; y_count<y_max; y_count++) {
	       addr = (unsigned long)bmp->line[y+y_count] + ((x_pos+plane)>>2);

	       if (*dat & (0x80 >> plane))
		  _farnspokeb(addr, color);
	       else
		  if (_textmode >= 0)
		     _farnspokeb(addr, _textmode);

	       dat++;
	    }
	 }
	 else if ((x_pos+plane+4 >= bmp->cl) && (x_pos+plane+4 < bmp->cr)) {
	    /* draw only the rightmost byte of the character */
	    for (y_count=y_min; y_count<y_max; y_count++) {
	       addr = (unsigned long)bmp->line[y+y_count] + ((x_pos+plane)>>2);

	       if (*dat & (0x80 >> (plane+4)))
		  _farnspokeb(addr+1, color);
	       else
		  if (_textmode >= 0)
		     _farnspokeb(addr+1, _textmode);

	       dat++;
	    }
	 }

	 /* next character */
	 x_pos += 8;
	 if (x_pos >= bmp->cr)
	    break;

	 s++;
      }
   }
}



/* _x_clear_to_color:
 *  Clears a mode-X screen.
 */
void _x_clear_to_color(BITMAP *bitmap, int color)
{
   int o_cl = bitmap->cl;
   int o_cr = bitmap->cr;

   outportw(0x3C4, 0x0F02);      /* enable all planes */

   bitmap->cl /= 4;              /* fake the linear code into clearing */
   bitmap->cr /= 4;              /* the correct amount of memory */

   _linear_clear_to_color8(bitmap, color);

   bitmap->cl = o_cl;
   bitmap->cr = o_cr;
}


