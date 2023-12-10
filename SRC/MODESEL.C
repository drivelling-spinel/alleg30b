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
 *      GUI routines.
 *
 *      The graphics mode selection dialog.
 *
 *      See readme.txt for copyright information.
 */


#include <stdlib.h>
#include <string.h>

#include "allegro.h"



typedef struct GFX_MODE_DATA
{
   int w;
   int h;
   char *s;
} GFX_MODE_DATA;



static GFX_MODE_DATA gfx_mode_data[] =
{
   { 320,   200,  "320x200"   },
   { 320,   240,  "320x240"   },
   { 640,   400,  "640x400"   },
   { 640,   480,  "640x480"   },
   { 800,   600,  "800x600"   },
   { 1024,  768,  "1024x768"  },
   { 1280,  1024, "1280x1024" },
   { 1600,  1200, "1600x1200" },
   { 256,   200,  "256x200"   },
   { 256,   224,  "256x224"   },
   { 256,   240,  "256x240"   },
   { 256,   256,  "256x256"   },
   { 320,   100,  "320x100"   },
   { 320,   400,  "320x400"   },
   { 320,   480,  "320x480"   },
   { 320,   600,  "320x600"   },
   { 360,   200,  "360x200"   },
   { 360,   240,  "360x240"   },
   { 360,   270,  "360x270"   },
   { 360,   360,  "360x360"   },
   { 360,   400,  "360x400"   },
   { 360,   480,  "360x480"   },
   { 360,   600,  "360x600"   },
   { 376,   282,  "376x282"   },
   { 376,   308,  "376x308"   },
   { 376,   564,  "376x564"   },
   { 400,   150,  "400x150"   },
   { 400,   300,  "400x300"   },
   { 400,   600,  "400x600"   },
   { 0,     0,    NULL        }
};



/* gfx_mode_getter:
 *  Listbox data getter routine for the graphics mode list.
 */
static char *gfx_mode_getter(int index, int *list_size)
{
   if (index < 0) {
      if (list_size)
	 *list_size = sizeof(gfx_mode_data) / sizeof(GFX_MODE_DATA) - 1;
      return NULL;
   }

   return gfx_mode_data[index].s;
}



/* gfx_card_getter:
 *  Listbox data getter routine for the graphics card list.
 */
static char *gfx_card_getter(int index, int *list_size)
{
   static char *card[] = {
      "Autodetect",
      "VGA mode 13h",
      "Mode-X",

   #ifdef DJGPP
	 /* djgpp drivers */
	 "VESA 1.x",
	 "VESA 2.0 (banked)",
	 "VESA 2.0 (linear)",
	 "VBE/AF",
	 "Xtended mode",
	 "ATI 18800/28800",
	 "ATI mach64",
	 "Cirrus 64xx",
	 "Cirrus 54xx",
	 "Paradise",
	 "S3",
	 "Trident",
	 "Tseng ET3000",
	 "Tseng ET4000",
	 "Video-7"

   #else
	 /* linux drivers */
	 "SVGALIB"
   #endif
   };

   if (index < 0) {
      if (list_size)
	 *list_size = sizeof(card) / sizeof(char *);
      return NULL;
   }

   return card[index];
}



/* gfx_depth_getter:
 *  Listbox data getter routine for the color depth list.
 */
static char *gfx_depth_getter(int index, int *list_size)
{
   static char *depth[] = {
      " 8 bpp (256 color)",
      "15 bpp (32K color)",
      "16 bpp (64K color)",
      "24 bpp (16M color)",
      "32 bpp (16M color)"
   };

   if (index < 0) {
      if (list_size)
	 *list_size = sizeof(depth) / sizeof(char *);
      return NULL;
   }

   return depth[index];
}



static DIALOG gfx_mode_dialog[] =
{
   /* (dialog proc)     (x)   (y)   (w)   (h)   (fg)  (bg)  (key) (flags)  (d1)  (d2)  (dp) */
   { d_shadow_box_proc, 0,    0,    312,  166,  0,    0,    0,    0,       0,    0,    NULL },
   { d_ctext_proc,      156,  8,    1,    1,    0,    0,    0,    0,       0,    0,    "Graphics Mode" },
   { d_button_proc,     196,  113,  100,  16,   0,    0,    0,    D_EXIT,  0,    0,    "OK" },
   { d_button_proc,     196,  135,  100,  16,   0,    0,    27,   D_EXIT,  0,    0,    "Cancel" },
   { d_list_proc,       16,   28,   164,  123,  0,    0,    0,    D_EXIT,  0,    0,    gfx_card_getter },
   { d_list_proc,       196,  28,   100,  75,   0,    0,    0,    D_EXIT,  3,    0,    gfx_mode_getter },
   { NULL }
};


static DIALOG gfx_mode_ex_dialog[] =
{
   /* (dialog proc)     (x)   (y)   (w)   (h)   (fg)  (bg)  (key) (flags)  (d1)  (d2)  (dp) */
   { d_shadow_box_proc, 0,    0,    312,  166,  0,    0,    0,    0,       0,    0,    NULL },
   { d_ctext_proc,      156,  8,    1,    1,    0,    0,    0,    0,       0,    0,    "Graphics Mode" },
   { d_button_proc,     196,  113,  100,  16,   0,    0,    0,    D_EXIT,  0,    0,    "OK" },
   { d_button_proc,     196,  135,  100,  16,   0,    0,    27,   D_EXIT,  0,    0,    "Cancel" },
   { d_list_proc,       16,   28,   164,  75,   0,    0,    0,    D_EXIT,  0,    0,    gfx_card_getter },
   { d_list_proc,       196,  28,   100,  75,   0,    0,    0,    D_EXIT,  3,    0,    gfx_mode_getter },
   { d_list_proc,       16,   113,  164,  43,   0,    0,    0,    D_EXIT,  0,    0,    gfx_depth_getter },
   { NULL }
};


#define GFX_CANCEL         3
#define GFX_DRIVER_LIST    4
#define GFX_MODE_LIST      5
#define GFX_DEPTH_LIST     6



/* gfx_mode_select:
 *  Displays the Allegro graphics mode selection dialog, which allows the
 *  user to select a screen mode and graphics card. Stores the selection
 *  in the three variables, and returns zero if it was closed with the 
 *  Cancel button, or non-zero if it was OK'd.
 */
int gfx_mode_select(int *card, int *w, int *h)
{
   int ret;

   clear_keybuf();

   do {
   } while (mouse_b);

   centre_dialog(gfx_mode_dialog);
   set_dialog_color(gfx_mode_dialog, gui_fg_color, gui_bg_color);
   ret = do_dialog(gfx_mode_dialog, GFX_DRIVER_LIST);

   *card = gfx_mode_dialog[GFX_DRIVER_LIST].d1;

   *w = gfx_mode_data[gfx_mode_dialog[GFX_MODE_LIST].d1].w;
   *h = gfx_mode_data[gfx_mode_dialog[GFX_MODE_LIST].d1].h;

   if (ret == GFX_CANCEL)
      return FALSE;
   else 
      return TRUE;
}



/* gfx_mode_select_ex:
 *  Extended version of the graphics mode selection dialog, which allows the 
 *  user to select the color depth as well as the resolution and hardware 
 *  driver. This version of the function reads the initial values from the 
 *  parameters when it activates, so you can specify the default values.
 */
int gfx_mode_select_ex(int *card, int *w, int *h, int *color_depth)
{
   static int depth_list[] = { 8, 15, 16, 24, 32, 0 };
   int i, ret;

   clear_keybuf();

   do {
   } while (mouse_b);

   gfx_mode_ex_dialog[GFX_DRIVER_LIST].d1 = *card;

   for (i=0; gfx_mode_data[i].s; i++) {
      if ((gfx_mode_data[i].w == *w) && (gfx_mode_data[i].h == *h)) {
	 gfx_mode_ex_dialog[GFX_MODE_LIST].d1 = i;
	 break; 
      }
   }

   for (i=0; depth_list[i]; i++) {
      if (depth_list[i] == *color_depth) {
	 gfx_mode_ex_dialog[GFX_DEPTH_LIST].d1 = i;
	 break;
      }
   }

   centre_dialog(gfx_mode_ex_dialog);
   set_dialog_color(gfx_mode_ex_dialog, gui_fg_color, gui_bg_color);
   ret = do_dialog(gfx_mode_ex_dialog, GFX_DRIVER_LIST);

   *card = gfx_mode_ex_dialog[GFX_DRIVER_LIST].d1;

   *w = gfx_mode_data[gfx_mode_ex_dialog[GFX_MODE_LIST].d1].w;
   *h = gfx_mode_data[gfx_mode_ex_dialog[GFX_MODE_LIST].d1].h;

   *color_depth = depth_list[gfx_mode_ex_dialog[GFX_DEPTH_LIST].d1];

   if (ret == GFX_CANCEL)
      return FALSE;
   else 
      return TRUE;
}

