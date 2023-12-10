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
 *      The grabber utility program.
 *
 *      See readme.txt for copyright information.
 */


#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <bios.h>
#include <stdio.h>
#include <dir.h>
#include <stdarg.h>
#include <ctype.h>
#include <conio.h>
#include <sys/stat.h>
#include <sys/exceptn.h>

#include "allegro.h"
#include "internal.h"
#include "datedit.h"



char *argv_0;

int fg, bg, mg;

#define NAME_MAX     64

typedef struct DATAITEM
{
   DATAFILE *dat;
   DATAFILE **parent;
   int i;
   char name[NAME_MAX];
} DATAITEM;

DATAFILE *datafile = NULL;
DATAITEM *data = NULL;
char *data_sel = NULL;
int data_count = 0;
int data_malloced = 0;

char data_file[256] = "";
char header_file[256] = "";
char prefix_string[256] = "";
char password[256];
char import_file[256];
char xgrid_string[16];
char ygrid_string[16];

BITMAP *graphic = NULL;
PALLETE g_pallete;
char graphic_origin[256] = "";
char graphic_date[256] = "";

int current_view_object = -1;
int current_property_object = -1;

int busy_mouse = FALSE;

int in_text = FALSE;


int view_proc(int, DIALOG *, int);
int list_proc(int, DIALOG *, int);
int prop_proc(int, DIALOG *, int);
int gg_text_proc(int, DIALOG *, int);
int gg_edit_proc(int, DIALOG *, int);
int droplist_proc(int, DIALOG *, int);
char *list_getter(int, int *);
char *pack_getter(int, int *);
char *prop_getter(int, int *);

int loader();
int saver();
int strip_saver();
int updater();
int griddler();
int reader();
int viewer();
int quitter();
int grabber();
int exporter();
int deleter();
int box_grabber();
int sheller();
int helper();
int sysinfo();
int about();
int new_bitmap();
int new_rle_sprite();
int new_c_sprite();
int new_xc_sprite();
int new_pallete();
int new_font();
int new_sample();
int new_midi();
int new_fli();
int new_datafile();
int new_binary();
int new_other();
int renamer();
int autocropper();
int ungrabber();
int property_delete();
int property_insert();
int property_change();
int changeto_bmp();
int changeto_rle();
int changeto_c();
int changeto_xc();
int changeto_8();
int changeto_15();
int changeto_16();
int changeto_24();
int changeto_32();

void rebuild_list(void *old, int clear);

void _handle_listbox_click(DIALOG *d);
void _unload_datafile_object(DATAFILE *dat);



MENU file_menu[] =
{
   { "&Load\t(ctrl+L)",             loader,           NULL },
   { "&Save\t(ctrl+S)",             saver,            NULL },
   { "Save S&tripped",              strip_saver,      NULL },
   { "&Update\t(ctrl+U)",           updater,          NULL },
   { "",                            NULL,             NULL },
   { "&Read Bitmap\t(ctrl+R)",      reader,           NULL },
   { "&View Bitmap\t(ctrl+V)",      viewer,           NULL },
   { "&Grab from Grid",             griddler,         NULL },
   { "",                            NULL,             NULL },
   { "&Quit\t(ctrl+Q)",             quitter,          NULL },
   { NULL,                          NULL,             NULL }
};


MENU new_menu[] =
{
   { "&Bitmap",                     new_bitmap,       NULL }, 
   { "&RLE Sprite",                 new_rle_sprite,   NULL }, 
   { "&Compiled Sprite",            new_c_sprite,     NULL }, 
   { "&X-Compiled Sprite",          new_xc_sprite,    NULL }, 
   { "&Palette",                    new_pallete,      NULL }, 
   { "F&ont",                       new_font,         NULL }, 
   { "&Sample",                     new_sample,       NULL }, 
   { "&MIDI File",                  new_midi,         NULL }, 
   { "&FLI/FLC",                    new_fli,          NULL }, 
   { "D&atafile",                   new_datafile,     NULL }, 
   { "B&inary Data",                new_binary,       NULL }, 
   { "O&ther",                      new_other,        NULL },
   { NULL,                          NULL,             NULL }
};


MENU retype_menu[] =
{
   { "To &Bitmap",                  changeto_bmp,     NULL },
   { "To &RLE Sprite",              changeto_rle,     NULL },
   { "To &Compiled Sprite",         changeto_c,       NULL },
   { "To &X-Compiled Sprite",       changeto_xc,      NULL },
   { NULL,                          NULL,             NULL }
};


MENU depth_menu[] =
{
   { "&256 color palette",          changeto_8,       NULL },
   { "1&5 bit hicolor",             changeto_15,      NULL },
   { "1&6 bit hicolor",             changeto_16,      NULL },
   { "2&4 bit truecolor",           changeto_24,      NULL },
   { "&32 bit truecolor",           changeto_32,      NULL },
   { NULL,                          NULL,             NULL }
};


MENU objc_menu[] =
{
   { "&Grab\t(ctrl+G)",             grabber,          NULL },
   { "&Export\t(ctrl+E)",           exporter,         NULL },
   { "&Delete\t(ctrl+D)",           deleter,          NULL },
   { "&Rename\t(ctrl+N)",           renamer,          NULL },
   { "Set &Property\t(ctrl+P)",     property_insert,  NULL },
   { "&Box Grab\t(ctrl+B)",         box_grabber,      NULL },
   { "&Autocrop",                   autocropper,      NULL },
   { "&Ungrab",                     ungrabber,        NULL },
   { "&Change Type...",             NULL,             retype_menu },
   { "C&olor Depth...",             NULL,             depth_menu },
   { "&Shell Edit\t(ctrl+Z)",       sheller,          NULL },
   { "",                            NULL,             NULL },
   { "&New...",                     NULL,             new_menu },
   { NULL,                          NULL,             NULL }
};


MENU help_menu[] =
{
   { "&Help\t(F1)",                 helper,           NULL },
   { "&System",                     sysinfo,          NULL },
   { "&About",                      about,            NULL },
   { NULL,                          NULL,             NULL }
};


MENU menu[] = 
{ 
   { "&File",                       NULL,             file_menu },
   { "&Object",                     NULL,             objc_menu },
   { "&Help",                       NULL,             help_menu },
   { NULL,                          NULL,             NULL }
};


MENU root_popup_menu[] =
{
   { "&New...",                     NULL,             new_menu },
   { NULL,                          NULL,             NULL }
};


MENU object_popup_menu[] =
{
   { "&Grab",                       grabber,          NULL },
   { "&Export",                     exporter,         NULL },
   { "&Delete",                     deleter,          NULL },
   { "&Rename",                     renamer,          NULL },
   { "&Shell Edit",                 sheller,          NULL },
   { "",                            NULL,             NULL },
   { "&New...",                     NULL,             new_menu },
   { NULL,                          NULL,             NULL }
};


MENU bitmap_object_popup_menu[] =
{
   { "&Grab",                       grabber,          NULL },
   { "&Export",                     exporter,         NULL },
   { "&Delete",                     deleter,          NULL },
   { "&Rename",                     renamer,          NULL },
   { "&Box Grab",                   box_grabber,      NULL },
   { "&Autocrop",                   autocropper,      NULL },
   { "&Ungrab",                     ungrabber,        NULL },
   { "&Change Type...",             NULL,             retype_menu },
   { "C&olor Depth...",             NULL,             depth_menu },
   { "&Shell Edit",                 sheller,          NULL },
   { "",                            NULL,             NULL },
   { "&New...",                     NULL,             new_menu },
   { NULL,                          NULL,             NULL }
};


MENU bitmap_num_object_popup_menu[] =
{
   { "&Grab",                       grabber,          NULL },
   { "&Export",                     exporter,         NULL },
   { "&Delete",                     deleter,          NULL },
   { "&Rename",                     renamer,          NULL },
   { "&Box Grab",                   box_grabber,      NULL },
   { "&Autocrop",                   autocropper,      NULL },
   { "&Ungrab",                     ungrabber,        NULL },
   { "&Change Type...",             NULL,             retype_menu },
   { "C&olor Depth...",             NULL,             depth_menu },
   { "Grab from Grid",              griddler,         NULL },
   { "&Shell Edit",                 sheller,          NULL },
   { "",                            NULL,             NULL },
   { "&New...",                     NULL,             new_menu },
   { NULL,                          NULL,             NULL }
};


MENU multi_object_popup_menu[] =
{
   { "&Delete",                     deleter,          NULL },
   { "",                            NULL,             NULL },
   { "&New...",                     NULL,             new_menu },
   { NULL,                          NULL,             NULL }
};


MENU multi_bitmap_object_popup_menu[] =
{
   { "&Delete",                     deleter,          NULL },
   { "&Autocrop",                   autocropper,      NULL },
   { "&Change Type...",             NULL,             retype_menu },
   { "C&olor Depth...",             NULL,             depth_menu },
   { "",                            NULL,             NULL },
   { "&New...",                     NULL,             new_menu },
   { NULL,                          NULL,             NULL }
};



#define C(x)      (x - 'a' + 1)


DIALOG main_dlg[] =
{
   /* (dialog proc)     (x)   (y)   (w)   (h)   (fg)  (bg)  (key)    (flags)     (d1)           (d2)     (dp) */
   { d_clear_proc,      0,    0,    640,  480,  0,    0,    0,       0,          0,             0,       NULL },
   { d_menu_proc,       0,    0,    0,    0,    0,    0,    0,       0,          0,             0,       menu },
   { d_text_proc,       20,   30,   0,    0,    0,    0,    0,       0,          0,             0,       "Editing:" },
   { d_edit_proc,       100,  30,   320,  8,    0,    0,    0,       0,          255,           0,       data_file },
   { d_text_proc,       20,   42,   0,    0,    0,    0,    0,       0,          0,             0,       "Header:" },
   { d_edit_proc,       100,  42,   320,  8,    0,    0,    0,       0,          255,           0,       header_file },
   { d_text_proc,       20,   54,   0,    0,    0,    0,    0,       0,          0,             0,       "Prefix:" },
   { d_edit_proc,       100,  54,   320,  8,    0,    0,    0,       0,          255,           0,       prefix_string },
   { d_text_proc,       20,   66,   0,    0,    0,    0,    0,       0,          0,             0,       "Password:" },
   { d_edit_proc,       100,  66,   320,  8,    0,    0,    0,       0,          255,           0,       password },
   { d_text_proc,       200,  10,   0,    0,    0,    0,    0,       0,          0,             0,       "X-grid:" },
   { d_edit_proc,       264,  10,   40,   8,    0,    0,    0,       0,          4,             0,       xgrid_string },
   { d_text_proc,       315,  10,   0,    0,    0,    0,    0,       0,          0,             0,       "Y-grid:" },
   { d_edit_proc,       379,  10,   40,   8,    0,    0,    0,       0,          4,             0,       ygrid_string },
   { d_check_proc,      430,  8,    82,   12,   0,    0,    0,       0,          4,             0,       "Backups:" },
   { droplist_proc,     430,  30,   194,  27,   0,    0,    0,       0,          0,             0,       pack_getter },
   { prop_proc,         260,  86,   364,  107,  0,    0,    0,       D_EXIT,     0,             0,       prop_getter },
   { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    C('l'),  0,          0,             0,       loader },
   { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    C('s'),  0,          0,             0,       saver },
   { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    C('u'),  0,          0,             0,       updater },
   { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    C('r'),  0,          0,             0,       reader },
   { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    C('v'),  0,          0,             0,       viewer },
   { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    C('q'),  0,          0,             0,       quitter },
   { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    C('g'),  0,          0,             0,       grabber },
   { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    C('e'),  0,          0,             0,       exporter },
   { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    C('d'),  0,          0,             0,       deleter },
   { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    C('n'),  0,          0,             0,       renamer },
   { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    C('p'),  0,          0,             0,       property_insert },
   { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    C('b'),  0,          0,             0,       box_grabber },
   { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    C('z'),  0,          0,             0,       sheller },
   { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    27,      0,          0,             0,       quitter },
   { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    0,       0,          KEY_F1,        0,       helper },
   { list_proc,         20,   86,   208,  387,  0,    0,    0,       D_EXIT,     0,             0,       list_getter },
   { view_proc,         260,  218,  0,    0,    0,    0,    0,       0,          0,             0,       NULL },
   { NULL }
};


#define DLG_FILENAME          3
#define DLG_HEADERNAME        5
#define DLG_PREFIXSTRING      7
#define DLG_PASSWORD          9
#define DLG_XGRIDSTRING       11
#define DLG_YGRIDSTRING       13
#define DLG_BACKUPCHECK       14
#define DLG_PACKLIST          15
#define DLG_PROP              16
#define DLG_FIRSTWHITE        17
#define DLG_LIST              32
#define DLG_VIEW              33

#define SELECTED_ITEM         main_dlg[DLG_LIST].d1
#define SELECTED_PROPERTY     main_dlg[DLG_PROP].d1


#define BOX_W     MIN(512, SCREEN_W-16)
#define BOX_H     MIN(256, (SCREEN_H-64)&0xFFF0)

#define BOX_L     ((SCREEN_W - BOX_W) / 2)
#define BOX_R     ((SCREEN_W + BOX_W) / 2)
#define BOX_T     ((SCREEN_H - BOX_H) / 2)
#define BOX_B     ((SCREEN_H + BOX_H) / 2)

int box_x = 0;
int box_y = 0;
int box_active = FALSE;



/* starts outputting a progress message */
void box_start()
{
   show_mouse(NULL);

   rectfill(screen, BOX_L, BOX_T, BOX_R, BOX_B, bg);
   rect(screen, BOX_L-1, BOX_T-1, BOX_R+1, BOX_B+1, fg);
   hline(screen, BOX_L, BOX_B+2, BOX_R+1, fg);
   vline(screen, BOX_R+2, BOX_T, BOX_B+2, fg);

   show_mouse(screen);

   box_x = box_y = 0;
   box_active = TRUE;
}



/* outputs text to the progress message */
void box_out(char *msg)
{
   if (box_active) {
      show_mouse(NULL);
      set_clip(screen, BOX_L+8, BOX_T+1, BOX_R-8, BOX_B-1);

      text_mode(bg);
      textout(screen, font, msg, BOX_L+(box_x+1)*8, BOX_T+(box_y+1)*8, fg);

      set_clip(screen, 0, 0, SCREEN_W-1, SCREEN_H-1);
      show_mouse(screen);

      box_x += strlen(msg);
   }
}



/* outputs text to the progress message */
void box_eol()
{
   if (box_active) {
      box_x = 0;
      box_y++;

      if ((box_y+2)*8 >= BOX_H) {
	 show_mouse(NULL);
	 blit(screen, screen, BOX_L+8, BOX_T+16, BOX_L+8, BOX_T+8, BOX_W-16, BOX_H-24);
	 rectfill(screen, BOX_L+8, BOX_T+BOX_H-16, BOX_L+BOX_W-8, BOX_T+BOX_H-8, bg);
	 show_mouse(screen);
	 box_y--;
      }
   }
}



/* ends output of a progress message */
void box_end(int pause)
{
   if (box_active) {
      if (pause) {
	 box_eol();
	 box_out("-- press a key --");

	 do {
	 } while (mouse_b);

	 do {
	 } while ((!keypressed()) && (!mouse_b));

	 do {
	 } while (mouse_b);

	 clear_keybuf();
      }

      box_active = FALSE;
   }
}



/* returns the currently selected object, or -1 if there is a multiple sel */
int single_selection()
{
   int sel = SELECTED_ITEM;
   int i;

   for (i=0; i<data_count; i++)
      if ((data_sel[i]) && (i != sel))
	 return -1;

   return sel;
}



/* worker function for iterating through all the selected objects */
int foreach_selection(int (*proc)(DATAFILE *, int *, int), int *count, int *param, int param2)
{
   int ret = D_O_K;
   int i;

   *count = 0;

   for (i=1; i<data_count; i++) {
      if ((i == SELECTED_ITEM) || (data_sel[i])) {
	 ret |= proc(data[i].dat, param, param2);
	 (*count)++;
      }
   }

   return ret;
}



/* tests whether two palletes are identical */
int compare_palletes(RGB *p1, RGB *p2)
{
   int c;

   for (c=0; c<PAL_SIZE; c++) {
      if ((p1[c].r != p2[c].r) || 
	  (p1[c].g != p2[c].g) || 
	  (p1[c].b != p2[c].b))
      return TRUE;
   }

   return FALSE;
}



char my_mouse_pointer_data[256] =
{
   2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
   2, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
   2, 1, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
   2, 1, 1, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
   2, 1, 1, 1, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
   2, 1, 1, 1, 1, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
   2, 1, 1, 1, 1, 1, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 
   2, 1, 1, 1, 1, 1, 1, 1, 2, 0, 0, 0, 0, 0, 0, 0, 
   2, 1, 1, 1, 1, 1, 1, 1, 1, 2, 0, 0, 0, 0, 0, 0, 
   2, 1, 1, 1, 1, 1, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 
   2, 1, 1, 2, 1, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
   2, 1, 2, 0, 2, 1, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 
   0, 2, 0, 0, 2, 1, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 
   0, 0, 0, 0, 0, 2, 1, 1, 2, 0, 0, 0, 0, 0, 0, 0, 
   0, 0, 0, 0, 0, 2, 1, 1, 2, 0, 0, 0, 0, 0, 0, 0, 
   0, 0, 0, 0, 0, 0, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0
};


char my_busy_pointer_data[256] =
{
   0, 0, 0, 0, 0, 0, 0, 2, 2, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 2, 2, 1, 1, 2, 2, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 2, 1, 1, 1, 1, 1, 1, 2, 0, 0, 0, 0,
   0, 0, 0, 2, 1, 1, 1, 0, 0, 1, 1, 1, 2, 0, 0, 0,
   0, 0, 2, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 2, 0, 0,
   0, 2, 1, 1, 0, 0, 0, 0, 0, 1, 1, 0, 1, 1, 2, 0,
   0, 2, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 2, 0,
   2, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 1, 1, 2,
   2, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 1, 1, 2,
   0, 2, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 2, 0,
   0, 2, 1, 1, 0, 0, 0, 0, 0, 1, 1, 0, 1, 1, 2, 0,
   0, 0, 2, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 2, 0, 0,
   0, 0, 0, 2, 1, 1, 1, 0, 0, 1, 1, 1, 2, 0, 0, 0,
   0, 0, 0, 0, 2, 1, 1, 1, 1, 1, 1, 2, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 2, 2, 1, 1, 2, 2, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 2, 2, 0, 0, 0, 0, 0, 0, 0
};


BITMAP *my_mouse_pointer = NULL;
BITMAP *my_busy_pointer = NULL;



/* selects a pallete, sorting out all the colors so things look ok */
void sel_pallete(RGB *pal)
{
   int c, x, y;
   int (*proc)(int, DIALOG *, int);

   memcpy(current_pallete, pal, sizeof(PALLETE));
   set_pallete(current_pallete);

   fg = makecol(0, 0, 0);
   mg = makecol(0x80, 0x80, 0x80);
   bg = makecol(0xFF, 0xFF, 0xFF);

   gui_fg_color = fg;
   gui_bg_color = bg;
   gui_mg_color = mg;

   proc = main_dlg[DLG_FIRSTWHITE].proc;
   main_dlg[DLG_FIRSTWHITE].proc = NULL;
   set_dialog_color(main_dlg, fg, mg);

   main_dlg[DLG_FIRSTWHITE].proc = proc;
   set_dialog_color(main_dlg+DLG_FIRSTWHITE, fg, bg);

   if (!my_mouse_pointer)
      my_mouse_pointer = create_bitmap(16, 16);

   if (!my_busy_pointer)
      my_busy_pointer = create_bitmap(16, 16);

   for (y=0; y<16; y++) {
      for (x=0; x<16; x++) {
	 switch (my_mouse_pointer_data[x+y*16]) {
	    case 1:  c = fg; break;
	    case 2:  c = bg; break;
	    default: c = screen->vtable->mask_color; break;
	 }
	 putpixel(my_mouse_pointer, x, y, c);

	 switch (my_busy_pointer_data[x+y*16]) {
	    case 1:  c = fg; break;
	    case 2:  c = bg; break;
	    default: c = screen->vtable->mask_color; break;
	 }
	 putpixel(my_busy_pointer, x, y, c);
      }
   }

   set_mouse_sprite(my_mouse_pointer);
   busy_mouse = FALSE;
}



/* display a font in full-screen mode */
void view_font(FONT *f)
{
   char buf[2];
   int page = 0;
   int c, x, y;

   show_mouse(NULL);
   text_mode(-1);
   buf[1] = 0;

   for (;;) {
      clear_to_color(screen, mg);

      for (c=0; c<128; c++) {
	 buf[0] = page+c;
	 x = (c&15) * SCREEN_W / 16;
	 y = (c/16) * SCREEN_H / 8;
	 textout(screen, font, buf, x, y, bg);
	 textout(screen, f, buf, x+8, y+8, fg);
      }

      do {
      } while (mouse_b);

      clear_keybuf();

      do {
      } while ((!mouse_b) && (!keypressed()));

      if (keypressed()) {
	 if ((readkey() & 0xFF) == 27)
	    break;
      }
      else {
	 if (mouse_b & 2)
	    break;
      }

      page = 128-page;
   }

   do {
   } while (mouse_b);
   clear_keybuf();

   show_mouse(screen);
}



/* displays a bitmap in full-screen mode */
void view_bitmap(BITMAP *b, RGB *pal)
{
   fixed scale = itofix(1);
   fixed prevscale = itofix(1);
   BITMAP *bc = NULL;
   int done = FALSE;
   int c;

   show_mouse(NULL);
   set_pallete(pal);
   clear_to_color(screen, mg);
   blit(b, screen, 0, 0, 0, 0, b->w, b->h);

   clear_keybuf();
   do {
   } while (mouse_b);

   do {
      if (scale != prevscale) {
	 clear_to_color(screen, mg);

	 if (!bc) {
	    bc = create_bitmap(b->w, b->h);
	    blit(b, bc, 0, 0, 0, 0, b->w, b->h);
	 }

	 stretch_blit(bc, screen, 0, 0, b->w, b->h, 0, 0, (b->w*scale)>>16, (b->h*scale)>>16);
	 prevscale = scale;
      }

      if (keypressed()) {
	 c = readkey();

	 switch (c >> 8) {

	    case KEY_DOWN:
	    case KEY_RIGHT:
	       if (scale < itofix(16))
		  scale *= 2;
	       break;

	    case KEY_UP:
	    case KEY_LEFT:
	       if (scale > itofix(1)/16)
		  scale /= 2;
	       break;

	    default:
	       switch (c & 0xFF) {

		  case '+':
		     if (scale < itofix(16))
			scale *= 2;
		     break;

		  case '-':
		     if (scale > itofix(1)/16)
			scale /= 2;
		     break;

		  default:
		     done = TRUE;
		     break;
	       }
	       break;
	 }
      }

      if (mouse_b)
	 done = TRUE;

   } while (!done);

   if (bc)
      destroy_bitmap(bc);

   clear_keybuf();
   do {
   } while (mouse_b);

   clear(screen);
   set_pallete(current_pallete);
   show_mouse(screen);
}



/* callback to quit out of the FLI player */
int fli_stopper()
{
   if ((keypressed()) || (mouse_b))
      return 1;
   else
      return 0;
}



/* handles double-clicking on an item in the object list */
int handle_dclick(DATAFILE *dat)
{
   RLE_SPRITE *spr;
   BITMAP *bmp;

   switch (dat->type) {

      case DAT_BITMAP:
      case DAT_C_SPRITE:
      case DAT_XC_SPRITE:
	 view_bitmap(dat->dat, current_pallete);
	 return D_REDRAW;

      case DAT_RLE_SPRITE:
	 spr = (RLE_SPRITE *)dat->dat;
	 bmp = create_bitmap_ex(spr->color_depth, spr->w, spr->h);
	 clear_to_color(bmp, bmp->vtable->mask_color);
	 draw_rle_sprite(bmp, spr, 0, 0);
	 view_bitmap(bmp, current_pallete);
	 destroy_bitmap(bmp);
	 return D_REDRAW;

      case DAT_PALLETE:
	 sel_pallete(dat->dat);
	 return D_REDRAW;

      case DAT_FONT:
	 view_font(dat->dat);
	 return D_REDRAW;

      case DAT_SAMPLE:
	 play_sample(dat->dat, 255, 127, 1000, FALSE);
	 return D_O_K;

      case DAT_MIDI:
	 play_midi(dat->dat, FALSE);
	 return D_O_K;

      case DAT_FLI:
	 show_mouse(NULL);
	 clear_to_color(screen, mg);
	 play_memory_fli(dat->dat, screen, TRUE, fli_stopper);
	 do {
	 } while (mouse_b);
	 clear_keybuf();
	 set_pallete(current_pallete);
	 show_mouse(screen);
	 return D_REDRAW;
   }

   return D_O_K;
}



void bitmap_view(int x, int y, BITMAP *b, int mask)
{
   int w = b->w;
   int h = b->h;
   fixed scale;

   if (w > SCREEN_W-x-8) {
      scale = itofix(SCREEN_W-x-8) / w;
      w = (w * scale) >> 16;
      h = (h * scale) >> 16;
   }

   if (h > SCREEN_H-y-40) {
      scale = itofix(SCREEN_H-y-40) / h;
      w = (w * scale) >> 16;
      h = (h * scale) >> 16;
   }

   rect(screen, x, y+32, x+w+1, y+32+h+1, fg);

   if ((bitmap_color_depth(screen) == 8) && (bitmap_color_depth(b) > 8)) {
      if ((w != b->w) || (h != b->h))
	 textout(screen, font, "<reduced color scaled preview>", x, y+18, fg);
      else
	 textout(screen, font, "<reduced color preview>", x, y+18, fg);
   }
   else if ((w != b->w) || (h != b->h))
      textout(screen, font, "<scaled preview>", x, y+18, fg);

   if (bitmap_color_depth(b) == bitmap_color_depth(screen)) {
      if (mask)
	 stretch_sprite(screen, b, x+1, y+33, w, h);
      else
	 stretch_blit(b, screen, 0, 0, b->w, b->h, x+1, y+33, w, h);
   }
   else {
      BITMAP *b2 = create_bitmap_ex(bitmap_color_depth(b), w, h);
      clear_to_color(b2, makecol_depth(bitmap_color_depth(b2), 0x80, 0x80, 0x80));

      if (mask) {
	 RGB tmprgb = current_pallete[0];

	 current_pallete[0].r = 63;
	 current_pallete[0].g = 0;
	 current_pallete[0].b = 63;
	 select_palette(current_pallete);

	 stretch_sprite(b2, b, 0, 0, w, h);
	 blit(b2, screen, 0, 0, x+1, y+33, w, h);

	 unselect_palette();
	 current_pallete[0] = tmprgb;
      }
      else {
	 stretch_blit(b, b2, 0, 0, b->w, b->h, 0, 0, w, h);
	 blit(b2, screen, 0, 0, x+1, y+33, w, h);
      }

      destroy_bitmap(b2);
   }
}



void rle_view(int x, int y, RLE_SPRITE *s)
{
   BITMAP *b = create_bitmap_ex(s->color_depth, s->w, s->h);
   clear_to_color(b, b->vtable->mask_color);
   draw_rle_sprite(b, s, 0, 0);
   bitmap_view(x, y, b, TRUE);
   destroy_bitmap(b);
}



/* dialog procedure for displaying the selected object */
int view_proc(int msg, DIALOG *d, int c)
{
   DATAFILE *dat;
   char buf[80];
   int c1, c2;

   if (msg == MSG_IDLE) {
      if (current_view_object != SELECTED_ITEM) {
	 show_mouse(NULL);
	 SEND_MESSAGE(d, MSG_DRAW, 0);
	 show_mouse(screen);
      }
   }
   else if (msg == MSG_DRAW) {
      current_view_object = SELECTED_ITEM;

      rectfill(screen, d->x, d->y, SCREEN_W, SCREEN_H, mg);

      if ((current_view_object > 0) && (current_view_object < data_count)) {
	 dat = data[current_view_object].dat;

	 text_mode(-1);
	 textout(screen, font, datedit_desc(dat), d->x, d->y, fg);

	 switch (dat->type) {

	    case DAT_FILE:
	       /* noop */
	       break;

	    case DAT_FONT:
	       textout(screen, dat->dat, " !\"#$%&'()*+,-./", d->x, d->y+32, fg);
	       textout(screen, dat->dat, "0123456789:;<=>?", d->x, d->y+64, fg);
	       textout(screen, dat->dat, "@ABCDEFGHIJKLMNO", d->x, d->y+96, fg);
	       textout(screen, dat->dat, "PQRSTUVWXYZ[\\]^_", d->x, d->y+128, fg);
	       textout(screen, dat->dat, "`abcdefghijklmno", d->x, d->y+160, fg);
	       textout(screen, dat->dat, "pqrstuvwxyz{|}~", d->x, d->y+192, fg);
	       break;

	    case DAT_BITMAP:
	       bitmap_view(d->x, d->y, dat->dat, FALSE);
	       break;

	    case DAT_C_SPRITE:
	    case DAT_XC_SPRITE:
	       bitmap_view(d->x, d->y, dat->dat, TRUE);
	       break;

	    case DAT_RLE_SPRITE:
	       rle_view(d->x, d->y, dat->dat);
	       break;

	    case DAT_PALLETE:
	       if (compare_palletes(dat->dat, current_pallete)) {
		  if (bitmap_color_depth(screen) > 8) {
		     select_palette(dat->dat);
		     for (c1=0; c1<PAL_SIZE; c1++)
			rectfill(screen, d->x+(c1&15)*8, d->y+(c1/16)*8+32, 
				 d->x+(c1&15)*8+7, d->y+(c1/16)*8+39, palette_color[c1]);
		     unselect_palette();
		     textout(screen, font, "A different pallete", d->x+160, d->y+32, fg);
		     textout(screen, font, "is currently in use.", d->x+160, d->y+40, fg);
		     textout(screen, font, "To select this one,", d->x+160, d->y+48, fg);
		     textout(screen, font, "double-click on it", d->x+160, d->y+56, fg);
		     textout(screen, font, "in the item list.", d->x+160, d->y+64, fg);
		  }
		  else {
		     textout(screen, font, "A different pallete is currently in use.", d->x, d->y+32, fg);
		     textout(screen, font, "To select this one, double-click on it", d->x, d->y+40, fg);
		     textout(screen, font, "in the item list.", d->x, d->y+48, fg);
		  }
	       }
	       else {
		  for (c1=0; c1<PAL_SIZE; c1++)
		     rectfill(screen, d->x+(c1&15)*8, d->y+(c1/16)*8+32, 
			      d->x+(c1&15)*8+7, d->y+(c1/16)*8+39, palette_color[c1]);
	       }
	       break;

	    case DAT_SAMPLE:
	       textout(screen, font, "Double-click in the item list to play it", d->x, d->y+32, fg);
	       break;

	    case DAT_MIDI:
	       textout(screen, font, "Double-click in the item list to play it", d->x, d->y+32, fg);
	       break;

	    case DAT_FLI:
	       textout(screen, font, "Double-click in the item list to play it", d->x, d->y+32, fg);
	       break;

	    default:
	       for (c1=0; c1<16; c1++) {
		  for (c2=0; c2<32; c2++) {
		     if ((c1*32+c2) >= dat->size)
			buf[c2] = ' ';
		     else
			buf[c2] = ((char *)dat->dat)[c1*32+c2];
		     if ((buf[c2] < 32) || (buf[c2] > 126))
			buf[c2] = ' ';
		  }
		  buf[32] = 0;
		  textout(screen, font, buf, d->x, d->y+32+c1*8, fg);
	       }
	       if (dat->size > 32*16)
		  textout(screen, font, "...", d->x+31*8, d->y+40+16*8, fg);
	       break;
	 }
      }
   }

   return D_O_K;
}



/* decides which popup menu to activate, depending on the selection */
MENU *which_menu(int sel)
{
   DATAFILE *dat;
   char *s;

   if (sel <= 0)
      return root_popup_menu;

   dat = data[sel].dat;

   if (single_selection() < 0) {
      if ((dat->type == DAT_BITMAP) || (dat->type == DAT_RLE_SPRITE) ||
	  (dat->type == DAT_C_SPRITE) || (dat->type == DAT_XC_SPRITE))
	 return multi_bitmap_object_popup_menu;
      else
	 return multi_object_popup_menu;
   }
   else {
      if ((dat->type == DAT_BITMAP) || (dat->type == DAT_RLE_SPRITE) ||
	  (dat->type == DAT_C_SPRITE) || (dat->type == DAT_XC_SPRITE)) {
	 s = get_datafile_property(dat, DAT_NAME);
	 if ((*s) && (isdigit(s[strlen(s)-1])))
	    return bitmap_num_object_popup_menu;
	 else
	    return bitmap_object_popup_menu;
      }
      else
	 return object_popup_menu;
   }
}



/* dialog procedure for a listbox with a shadow */
int droplist_proc(int msg, DIALOG *d, int c)
{
   if (msg == MSG_DRAW) {
      hline(screen, d->x+1, d->y+d->h+1, d->x+d->w+1, d->fg);
      vline(screen, d->x+d->w+1, d->y+1, d->y+d->h+1, d->fg);
   }

   return d_list_proc(msg, d, c);
}



/* dialog procedure for the main object list */
int list_proc(int msg, DIALOG *d, int c)
{
   int ret;

   switch (msg) {

      case MSG_CHAR:
	 switch (c >> 8) {

	    case KEY_ESC:
	       position_mouse(d->x+d->w/3, d->y+6+(d->d1-d->d2)*8);
	       if (do_menu(which_menu(d->d1), mouse_x, mouse_y) >= 0)
		  return D_REDRAW | D_USED_CHAR;
	       else
		  return D_USED_CHAR;

	    case KEY_DEL:
	    case KEY_BACKSPACE:
	       return deleter() | D_USED_CHAR;

	    case KEY_INSERT:
	       return new_other() | D_USED_CHAR;
	 }
	 break;

      case MSG_CLICK:
	 if (mouse_b == 2) {
	    _handle_listbox_click(d);
	    if (do_menu(which_menu(d->d1), mouse_x, mouse_y) >= 0)
	       return D_REDRAW;
	    else
	       return D_O_K;
	 }
	 break;
   }

   ret = droplist_proc(msg, d, c);

   if ((msg == MSG_DRAW) && (!keypressed())) {
      if (current_view_object != d->d1)
	 SEND_MESSAGE(main_dlg+DLG_VIEW, MSG_DRAW, 0);

      if (current_property_object != d->d1)
	 SEND_MESSAGE(main_dlg+DLG_PROP, MSG_DRAW, 0);
   }

   if (ret & D_CLOSE) {
      ret &= ~D_CLOSE;
      if ((d->d1 > 0) && (d->d1 < data_count))
	 ret |= handle_dclick(data[d->d1].dat);
   }

   return ret;
}



/* dialog callback for retrieving information about the object list */
char *list_getter(int index, int *list_size)
{
   if (index < 0) {
      if (list_size)
	 *list_size = data_count;
      return NULL;
   }

   return data[index].name;
}



#define MAX_PROPERTIES        64
#define MAX_PROPERTY_VALUE    256

char prop_string[MAX_PROPERTIES][MAX_PROPERTY_VALUE];
int num_props = 0;



/* dialog procedure for the property list */
int prop_proc(int msg, DIALOG *d, int c)
{
   DATAFILE *dat;
   int i;
   int name_pos;
   int ret;

   switch (msg) {

      case MSG_CHAR:
	 switch (c >> 8) {

	    case KEY_DEL:
	    case KEY_BACKSPACE:
	       return property_delete() | D_USED_CHAR;

	    case KEY_INSERT:
	       return property_insert() | D_USED_CHAR;
	 }
	 break;

      case MSG_IDLE:
	 if (current_property_object != SELECTED_ITEM) {
	    show_mouse(NULL);
	    SEND_MESSAGE(d, MSG_DRAW, 0);
	    show_mouse(screen);
	 }
	 break;

      case MSG_DRAW:
	 num_props = 0;
	 name_pos = 0;

	 if ((SELECTED_ITEM > 0) && (SELECTED_ITEM < data_count)) {
	    dat = data[SELECTED_ITEM].dat;
	    if (dat->prop) {
	       for (i=0; dat->prop[i].type != DAT_END; i++) {
		  if (i >= MAX_PROPERTIES)
		     break;

		  sprintf(prop_string[num_props++], "%c%c%c%c - %.200s",
			  (dat->prop[i].type >> 24) & 0xFF,
			  (dat->prop[i].type >> 16) & 0xFF,
			  (dat->prop[i].type >> 8) & 0xFF,
			  (dat->prop[i].type & 0xFF),
			  dat->prop[i].dat);

		  if (dat->prop[i].type == DAT_NAME)
		     name_pos = i;
	       } 
	    }
	 }

	 if (current_property_object != SELECTED_ITEM) {
	    current_property_object = SELECTED_ITEM;
	    d->d1 = name_pos;
	 }

	 if (d->d1 >= num_props)
	    d->d1 = num_props-1;
	 if (d->d1 < 0) 
	    d->d1 = 0;

	 break;
   }

   ret = droplist_proc(msg, d, c);

   if (ret & D_CLOSE) {
      ret &= ~D_CLOSE;
      if (SELECTED_ITEM > 0)
	 ret |= property_change();
   }

   return ret;
}



/* dialog callback for retrieving information about the property list */
char *prop_getter(int index, int *list_size)
{
   if (index < 0) {
      if (list_size)
	 *list_size = num_props;
      return NULL;
   }

   return prop_string[index];
}



/* selects the specified object property */
void select_property(int type)
{
   DATAFILE *dat;
   int i;

   SELECTED_PROPERTY = 0;

   if ((SELECTED_ITEM > 0) && (SELECTED_ITEM < data_count)) {
      dat = data[SELECTED_ITEM].dat;
      if (dat->prop) {
	 for (i=0; dat->prop[i].type != DAT_END; i++) {
	    if (dat->prop[i].type == type) {
	       SELECTED_PROPERTY = i;
	       break;
	    }
	    if (dat->prop[i].type == DAT_NAME)
	       SELECTED_PROPERTY = i;
	 }
      }
   }

   SEND_MESSAGE(main_dlg+DLG_PROP, MSG_START, 0);
}



/* checks whether an object name is valid */
void check_valid_name(char *val)
{
   int i;

   if (val) {
      for (i=0; val[i]; i++) {
	 if ((!isalnum(val[i])) && (val[i] != '_')) {
	    alert("Warning: name is not",
		  "a valid CPP identifier",
		  NULL, "Hmm...", NULL, 13, 0);
	    break;
	 }
      }
   }
}



/* helper for changing object properties */
void set_property(DATAITEM *dat, int type, char *val)
{
   DATAFILE *d = dat->dat;
   void *old = d->dat;

   datedit_set_property(d, type, val);
   datedit_sort_properties(d->prop);

   if (type == DAT_NAME) {
      check_valid_name(val);
      datedit_sort_datafile(*dat->parent);
      rebuild_list(old, TRUE);
   }

   select_property(type);
}



char prop_type_string[8];
char prop_value_string[256];



DIALOG prop_dlg[] =
{
   /* (dialog proc)     (x)   (y)   (w)   (h)   (fg)  (bg)  (key)    (flags)     (d1)           (d2)     (dp) */
   { d_shadow_box_proc, 0,    0,    400,  112,  0,    0,    0,       0,          0,             0,       NULL },
   { d_ctext_proc,      200,  8,    0,    0,    0,    0,    0,       0,          0,             0,       NULL },
   { d_text_proc,       16,   32,   0,    0,    0,    0,    0,       0,          0,             0,       NULL },
   { d_edit_proc,       72,   32,   40,   8,    0,    0,    0,       0,          4,             0,       prop_type_string },
   { d_text_proc,       16,   48,   0,    0,    0,    0,    0,       0,          0,             0,       NULL },
   { d_edit_proc,       72,   48,   320,  8,    0,    0,    0,       0,          255,           0,       prop_value_string },
   { d_button_proc,     112,  80,   80,   16,   0,    0,    13,      D_EXIT,     0,             0,       "OK" }, 
   { d_button_proc,     208,  80,   80,   16,   0,    0,    27,      D_EXIT,     0,             0,       "Cancel" }, 
   { NULL }
};


#define PROP_DLG_TITLE           1
#define PROP_DLG_TYPE_STRING     2
#define PROP_DLG_TYPE            3
#define PROP_DLG_VALUE_STRING    4
#define PROP_DLG_VALUE           5
#define PROP_DLG_OK              6
#define PROP_DLG_CANCEL          7



/* brings up the property/new object dialog */
int do_edit(char *title, char *type_string, char *value_string, int type, char *val, int change_type, int show_type)
{
   prop_dlg[PROP_DLG_TITLE].dp = title;
   prop_dlg[PROP_DLG_TYPE_STRING].dp = type_string;
   prop_dlg[PROP_DLG_VALUE_STRING].dp = value_string;

   if (show_type) {
      prop_dlg[PROP_DLG_TYPE_STRING].flags &= ~D_HIDDEN;
      prop_dlg[PROP_DLG_TYPE].flags &= ~D_HIDDEN;
   }
   else {
      prop_dlg[PROP_DLG_TYPE_STRING].flags |= D_HIDDEN;
      prop_dlg[PROP_DLG_TYPE].flags |= D_HIDDEN;
   }

   if (type)
      sprintf(prop_type_string, "%c%c%c%c", type>>24, (type>>16)&0xFF, (type>>8)&0xFF, type&0xFF);
   else
      prop_type_string[0] = 0;

   if (val)
      strcpy(prop_value_string, val);
   else
      prop_value_string[0] = 0;

   centre_dialog(prop_dlg);
   set_dialog_color(prop_dlg, fg, bg);

   if (change_type)
      prop_dlg[PROP_DLG_TYPE].proc = d_edit_proc;
   else
      prop_dlg[PROP_DLG_TYPE].proc = d_text_proc;

   return (do_dialog(prop_dlg, (change_type ? PROP_DLG_TYPE : PROP_DLG_VALUE)) == PROP_DLG_OK);
}



/* brings up the property editing dialog */
int edit_property(char *title, char *value_string, int type, char *val, int change_type, int show_type)
{
   int sel = single_selection();
   DATAITEM *dat;

   if ((sel > 0) && (sel < data_count)) {
      dat = data+sel;
   }
   else {
      if (sel < 0)
	 alert("Can't set properties for a multiple selection!", NULL, NULL, "OK", NULL, 13, 0);
      else
	 alert("Nothing to set properties for!", NULL, NULL, "OK", NULL, 13, 0);
      return D_O_K;
   }

   if (do_edit(title, "ID:", value_string, type, val, change_type, show_type))
      if (prop_type_string[0])
	 set_property(dat, datedit_clean_typename(prop_type_string), prop_value_string);

   return D_REDRAW;
}



/* handle the set property command */
int property_insert()
{
   return edit_property("Set Property", "Value:", 0, NULL, TRUE, TRUE);
}



/* handle the change property command */
int property_change()
{
   int sel = single_selection();
   DATAFILE *dat;
   int i;

   if ((sel > 0) && (sel < data_count)) {
      dat = data[sel].dat;
      if (dat->prop) {
	 for (i=0; dat->prop[i].type != DAT_END; i++) {
	    if (i == SELECTED_PROPERTY) {
	       return edit_property("Edit Property", "Value: ", dat->prop[i].type, dat->prop[i].dat, FALSE, TRUE);
	    }
	 }
      }
   }
   else {
      if (sel < 0)
	 alert("Can't set properties for a multiple selection!", NULL, NULL, "OK", NULL, 13, 0);
   }

   return D_O_K;
}



/* handle the rename command */
int renamer()
{
   int sel = single_selection();
   DATAFILE *dat;

   if ((sel <= 0) || (sel >= data_count)) {
      if (sel < 0)
	 alert("Can't rename a multiple selection!", NULL, NULL, "OK", NULL, 13, 0);
      else
	 alert("Nothing to rename!", NULL, NULL, "OK", NULL, 13, 0);
      return D_O_K;
   }

   dat = data[sel].dat;
   return edit_property("Rename", "Name:", DAT_NAME, get_datafile_property(dat, DAT_NAME), FALSE, FALSE);
}



/* helper for cropping bitmaps */
BITMAP *crop_bitmap(BITMAP *bmp)
{
   int tx, ty, tw, th, i, j, c;
   int changed = FALSE;

   tx = 0;
   ty = 0;
   tw = bmp->w;
   th = bmp->h;

   if ((tw > 0) && (th > 0)) {
      c = getpixel(bmp, 0, 0);

      for (j=ty; j<ty+th; j++) {       /* top of image */
	 for (i=tx; i<tx+tw; i++) {
	    if (getpixel(bmp, i, j) != c)
	       goto finishedtop;
	 }
	 ty++;
	 th--;
	 changed = TRUE;
      }

      finishedtop:

      for (j=ty+th-1; j>ty; j--) {     /* bottom of image */
	 for (i=tx; i<tx+tw; i++) {
	    if (getpixel(bmp, i, j) != c)
	       goto finishedbottom;
	 }
	 th--;
	 changed = TRUE;
      }

      finishedbottom:

      for (j=tx; j<tx+tw; j++) {       /* left of image */
	 for (i=ty; i<ty+th; i++) {
	    if (getpixel(bmp, j, i) != c)
	       goto finishedleft;
	 }
	 tx++;
	 tw--;
	 changed = TRUE;
      }

      finishedleft:

      for (j=tx+tw-1; j>tx; j--) {     /* right of image */
	 for (i=ty; i<ty+th; i++) {
	    if (getpixel(bmp, j, i) != c)
	       goto finishedright;
	 }
	 tw--;
	 changed = TRUE;
      }

      finishedright:
   }

   if ((tw != 0) && (th != 0) && (changed)) {
      BITMAP *b2 = create_bitmap_ex(bitmap_color_depth(bmp), tw, th);
      clear_to_color(b2, b2->vtable->mask_color);
      blit(bmp, b2, tx, ty, 0, 0, tw, th);
      destroy_bitmap(bmp);
      return b2;
   }
   else
      return bmp;
}



/* worker function for cropping an image */
int do_autocropper(DATAFILE *dat, int *param, int param2)
{
   BITMAP *bmp;
   RLE_SPRITE *spr;

   if ((dat->type != DAT_BITMAP) && (dat->type != DAT_RLE_SPRITE) &&
       (dat->type != DAT_C_SPRITE) && (dat->type != DAT_XC_SPRITE)) {
      (*param)++;
      return D_O_K;
   }

   if (dat->type == DAT_RLE_SPRITE) {
      spr = (RLE_SPRITE *)dat->dat;
      bmp = create_bitmap_ex(spr->color_depth, spr->w, spr->h);
      clear_to_color(bmp, bmp->vtable->mask_color);
      draw_rle_sprite(bmp, spr, 0, 0);
      destroy_rle_sprite(spr);
      bmp = crop_bitmap(bmp);
      dat->dat = get_rle_sprite(bmp);
      destroy_bitmap(bmp);
   }
   else
      dat->dat = crop_bitmap((BITMAP *)dat->dat);

   return D_REDRAW;
}



/* handle the crop command */
int autocropper()
{
   char buf[80];
   int ret, n;
   int p = 0;

   ret = foreach_selection(do_autocropper, &n, &p, 0);

   if (n <= 0) {
      alert ("Nothing to crop!", NULL, NULL, "OK", NULL, 13, 0);
   }
   else if (p > 0) {
      sprintf(buf, "%d non-bitmap object%s ignored", p, (p==1) ? " was" : "s were");
      alert(buf, NULL, NULL, "OK", NULL, 13, 0);
   }

   return ret;
}



/* handle the ungrab command */
int ungrabber()
{
   int sel = single_selection();
   DATAFILE *dat;

   if ((sel <= 0) || (sel >= data_count)) {
      if (sel < 0)
	 alert("Can't ungrab a multiple selection!", NULL, NULL, "OK", NULL, 13, 0);
      else
	 alert("Nothing to ungrab!", NULL, NULL, "OK", NULL, 13, 0);
      return D_O_K;
   }

   dat = data[sel].dat;

   if ((dat->type != DAT_BITMAP) && (dat->type != DAT_RLE_SPRITE) &&
       (dat->type != DAT_C_SPRITE) && (dat->type != DAT_XC_SPRITE)) {
      alert("Only bitmap objects can be ungrabbed!", NULL, NULL, "OK", NULL, 13, 0);
      return D_O_K;
   }

   if (graphic)
      destroy_bitmap(graphic);

   if (dat->type == DAT_RLE_SPRITE) {
      RLE_SPRITE *spr = (RLE_SPRITE *)dat->dat;
      graphic = create_bitmap_ex(spr->color_depth, spr->w, spr->h);
      clear(graphic);
      draw_rle_sprite(graphic, spr, 0, 0);
   }
   else {
      BITMAP *bmp = (BITMAP *)dat->dat;
      graphic = create_bitmap_ex(bitmap_color_depth(bmp), bmp->w, bmp->h);
      blit(bmp, graphic, 0, 0, 0, 0, bmp->w, bmp->h);
   }

   if (bitmap_color_depth(graphic) == 8)
      memcpy(g_pallete, current_pallete, sizeof(PALLETE));
   else
      generate_optimized_palette(graphic, g_pallete, NULL);

   alert("Bitmap data copied to the input buffer", NULL, NULL, "OK", NULL, 13, 0);

   return D_O_K;
}



/* handle the delete property command */
int property_delete()
{
   int sel = single_selection();
   DATAFILE *dat;
   int i;

   if ((sel > 0) && (sel < data_count)) {
      dat = data[sel].dat;
      if (dat->prop) {
	 for (i=0; dat->prop[i].type != DAT_END; i++) {
	    if (i == SELECTED_PROPERTY) {
	       set_property(data+SELECTED_ITEM, dat->prop[i].type, NULL);
	       return D_REDRAW;
	    }
	 }
      }
   }
   else {
      if (sel < 0)
	 alert("Can't delete properties from a multiple selection!", NULL, NULL, "OK", NULL, 13, 0);
   }

   return D_O_K;
}



/* helper for adding an item to the object list */
void add_to_list(DATAFILE *dat, DATAFILE **parent, int i, char *name, int clear)
{
   if (data_count >= data_malloced) {
      data_malloced += 16;
      data = realloc(data, data_malloced * sizeof(DATAITEM));
      data_sel = realloc(data_sel, data_malloced * sizeof(char));
      main_dlg[DLG_LIST].dp2 = data_sel;
   }

   data[data_count].dat = dat;
   data[data_count].parent = parent;
   data[data_count].i = i;
   strcpy(data[data_count].name, name);

   if (clear)
      data_sel[data_count] = FALSE;

   data_count++;
}



/* recursive helper used by rebuild list() */
void add_datafile_to_list(DATAFILE **dat, char *prefix, int clear)
{
   int i;
   char tmp[80];
   DATAFILE *d;

   for (i=0; (*dat)[i].type != DAT_END; i++) {
      d = (*dat)+i;

      sprintf(tmp, "%c%c%c%c %s%c %.32s", (d->type >> 24) & 0xFF,
	      (d->type >> 16) & 0xFF, (d->type >> 8) & 0xFF,
	      (d->type & 0xFF), prefix, 
	      (d->type == DAT_FILE) ? '+' : '-',
	      get_datafile_property(d, DAT_NAME));

      add_to_list(d, dat, i, tmp, clear);

      if (d->type == DAT_FILE) {
	 strcpy(tmp, prefix);
	 strcat(tmp, "|");
	 add_datafile_to_list((DATAFILE **)&d->dat, tmp, clear);
      }
   }
}



/* expands the datafile into a list of objects, for display in the listbox */
void rebuild_list(void *old, int clear)
{
   int i;

   data_count = 0;

   add_to_list(NULL, &datafile, 0, "<root>", clear);
   add_datafile_to_list(&datafile, "", clear);

   if (old) {
      SELECTED_ITEM = 0;

      for (i=0; i<data_count; i++) {
	 if ((data[i].dat) && (data[i].dat->dat == old)) {
	    SELECTED_ITEM = i;
	    break;
	 }
      }
   }

   SEND_MESSAGE(main_dlg+DLG_LIST, MSG_START, 0);
}



/* dialog callback for retrieving the contents of the compression type list */
char *pack_getter(int index, int *list_size)
{
   static char *s[] =
   {
      "No compression",
      "Individual compression",
      "Global compression"
   };

   static char *s2[] =
   {
      "Unpacked",
      "Per-object",
      "Compressed"
   };

   if (index < 0) {
      *list_size = 3;
      return NULL;
   }

   if (SCREEN_W < 640)
      return s2[index];
   else
      return s[index];
}



/* updates the info chunk with the current settings */
void update_info()
{
   char buf[8];

   datedit_set_property(&info, DAT_HNAM, header_file);
   datedit_set_property(&info, DAT_HPRE, prefix_string);
   datedit_set_property(&info, DAT_XGRD, xgrid_string);
   datedit_set_property(&info, DAT_YGRD, ygrid_string);

   datedit_set_property(&info, DAT_BACK, 
		  (main_dlg[DLG_BACKUPCHECK].flags & D_SELECTED) ? "y" : "n");

   sprintf(buf, "%d", main_dlg[DLG_PACKLIST].d1);
   datedit_set_property(&info, DAT_PACK, buf);
}



/* helper for recovering data stored in the info chunk */
void retrieve_property(int object, int type, char *def)
{
   char *p = get_datafile_property(&info, type);

   if ((p) && (*p))
      strcpy(main_dlg[object].dp, p);
   else
      strcpy(main_dlg[object].dp, def);

   main_dlg[object].d2 = strlen(main_dlg[object].dp);
}



/* do the actual work of loading a file */
void load(char *filename)
{
   int i;

   set_mouse_sprite(my_busy_pointer);
   busy_mouse = TRUE;

   if (datafile) {
      unload_datafile(datafile);
      datafile = NULL;
   }

   if (filename) {
      _fixpath(filename, data_file);
      strcpy(data_file, datedit_pretty_name(data_file, "dat", FALSE));
      for (i=0; data_file[i]; i++)
	 if (data_file[i] == '/')
	    data_file[i] = '\\';
   }
   else
      data_file[0] = 0;

   main_dlg[DLG_FILENAME].d2 = strlen(data_file);

   datafile = datedit_load_datafile(filename, FALSE, password);
   if (!datafile)
      datafile = datedit_load_datafile(NULL, FALSE, NULL);

   SELECTED_ITEM = 0;

   retrieve_property(DLG_HEADERNAME, DAT_HNAM, "");
   retrieve_property(DLG_PREFIXSTRING, DAT_HPRE, "");
   retrieve_property(DLG_XGRIDSTRING, DAT_XGRD, "16");
   retrieve_property(DLG_YGRIDSTRING, DAT_YGRD, "16");

   if (tolower(*get_datafile_property(&info, DAT_BACK)) == 'y')
      main_dlg[DLG_BACKUPCHECK].flags |= D_SELECTED;
   else
      main_dlg[DLG_BACKUPCHECK].flags &= ~D_SELECTED;

   main_dlg[DLG_PACKLIST].d1 = atoi(get_datafile_property(&info, DAT_PACK));
   if (main_dlg[DLG_PACKLIST].d1 > 2)
      main_dlg[DLG_PACKLIST].d1 = 2;
   else if (main_dlg[DLG_PACKLIST].d1 < 0)
      main_dlg[DLG_PACKLIST].d1 = 0;

   rebuild_list(NULL, TRUE);

   set_mouse_sprite(my_mouse_pointer);
   busy_mouse = FALSE;
}



/* handle the load command */
int loader()
{
   char buf[256];

   strcpy(buf, data_file);
   *get_filename(buf) = 0;

   if (file_select("Load datafile", buf, "DAT")) {
      strlwr(buf);
      load(buf);
   }

   return D_REDRAW;
}



/* do the actual work of saving a file */
int save(int strip)
{
   char buf[256], buf2[256];
   int err = FALSE;

   strcpy(buf, data_file);

   if (file_select("Save datafile", buf, "DAT")) {
      if ((stricmp(data_file, buf) != 0) && (exists(buf))) {
	 sprintf(buf2, "%s already exists, overwrite?", buf);
	 if (alert(buf2, NULL, NULL, "Yes", "Cancel", 'y', 27) != 1)
	    return D_REDRAW;
      }

      box_start();

      set_mouse_sprite(my_busy_pointer);
      busy_mouse = TRUE;

      strlwr(buf);
      strcpy(data_file, buf);
      main_dlg[DLG_FILENAME].d2 = strlen(data_file);

      update_info();

      if (!datedit_save_datafile(datafile, data_file, strip, -1, TRUE, FALSE, (main_dlg[DLG_BACKUPCHECK].flags & D_SELECTED), password))
	 err = TRUE;

      if ((header_file[0]) && (!err)) {
	 box_eol();

	 if ((!strchr(header_file, '\\')) && (!strchr(header_file, '/'))) {
	    strcpy(buf, data_file);
	    strcpy(get_filename(buf), header_file);
	 }
	 else
	    strcpy(buf, header_file);

	 if (!datedit_save_header(datafile, data_file, buf, "grabber", prefix_string, FALSE))
	    err = TRUE;
      }

      set_mouse_sprite(my_mouse_pointer);
      busy_mouse = FALSE;

      box_end(!err);
   }

   return D_REDRAW;
}



/* handle the save command */
int saver()
{
   return save(-1);
}



/* dialog callback for retrieving the contents of the strip mode list */
char *striplist_getter(int index, int *list_size)
{
   static char *str[] =
   {
      "Save everything",
      "Strip grabber information",
      "Strip all object properties"
   };

   if (index < 0) {
      if (list_size)
	 *list_size = 3;
      return NULL;
   }

   return str[index];
}



DIALOG strip_dlg[] =
{
   /* (dialog proc)     (x)   (y)   (w)   (h)   (fg)  (bg)  (key)    (flags)     (d1)           (d2)     (dp) */
   { d_shadow_box_proc, 0,    0,    300,  112,  0,    0,    0,       0,          0,             0,       NULL },
   { d_ctext_proc,      150,  8,    0,    0,    0,    0,    0,       0,          0,             0,       "Save Stripped" },
   { d_list_proc,       22,   32,   256,  27,   0,    0,    0,       D_EXIT,     0,             0,       striplist_getter },
   { d_button_proc,     62,   80,   80,   16,   0,    0,    13,      D_EXIT,     0,             0,       "OK" }, 
   { d_button_proc,     158,  80,   80,   16,   0,    0,    27,      D_EXIT,     0,             0,       "Cancel" }, 
   { NULL }
};


#define STRIP_DLG_LIST        2
#define STRIP_DLG_OK          3
#define STRIP_DLG_CANCEL      4



/* handle the save stripped command */
int strip_saver()
{
   centre_dialog(strip_dlg);
   set_dialog_color(strip_dlg, fg, bg);

   if (do_dialog(strip_dlg, STRIP_DLG_LIST) == STRIP_DLG_CANCEL)
      return D_REDRAW;

   return save(strip_dlg[STRIP_DLG_LIST].d1);
}



/* handle the update command */
int updater()
{
   int c;
   int nowhere;
   int err = FALSE;

   box_start();

   set_mouse_sprite(my_busy_pointer);
   busy_mouse = TRUE;

   for (c=1; c<data_count; c++) {
      if (data[c].dat->type != DAT_FILE) {
	 if (!datedit_update(data[c].dat, FALSE, &nowhere)) {
	    err = TRUE;
	    break;
	 }
	 datedit_sort_properties(data[c].dat->prop);
      }
   }

   set_mouse_sprite(my_mouse_pointer);
   busy_mouse = FALSE;

   if (!err) {
      box_out("Done!");
      box_eol();
   }

   box_end(!err);

   select_property(DAT_NAME);

   return D_REDRAW;
}



/* handle the read command */
int reader()
{
   char buf[256];

   strcpy(buf, import_file);
   *get_filename(buf) = 0;

   if (file_select("Read bitmap file (bmp;lbm;pcx;tga)", buf, "bmp;lbm;pcx;tga")) {
      strlwr(buf);
      set_mouse_sprite(my_busy_pointer);
      busy_mouse = TRUE;

      if (graphic)
	 destroy_bitmap(graphic);

      strcpy(import_file, buf);
      graphic = load_bitmap(import_file, g_pallete);

      set_mouse_sprite(my_mouse_pointer);
      busy_mouse = FALSE;

      if (graphic) {
	 if (bitmap_color_depth(graphic) > 8)
	    generate_optimized_palette(graphic, g_pallete, NULL);

	 view_bitmap(graphic, g_pallete);

	 strcpy(graphic_origin, import_file);
	 strcpy(graphic_date, datedit_ftime2asc(file_time(import_file)));
      }
      else {
	 alert("Error loading bitmap file", NULL, NULL, "Oh dear", NULL, 13, 0);
	 graphic_origin[0] = 0;
	 graphic_date[0] = 0;
      }
   }

   return D_REDRAW;
}



/* handle the view command */
int viewer()
{
   if (graphic) {
      view_bitmap(graphic, g_pallete);
      return D_REDRAW;
   }
   else {
      alert("Nothing to view!",
	    "First you must read in a bitmap file", 
	    NULL, "OK", NULL, 13, 0);
      return D_O_K;
   }
}



/* handle the quit command */
int quitter()
{
   if (alert("Really want to quit?", NULL, NULL, "Yes", "Cancel", 'y', 27) == 1)
      return D_CLOSE;
   else
      return D_O_K;
}



/* handles grabbing data from a bitmap */
void grabbit(DATAFILE *dat, int box)
{
   int xgrid = MAX(atoi(xgrid_string), 1);
   int ygrid = MAX(atoi(ygrid_string), 1);
   int using_mouse;
   int x, y, w, h;
   int ox = -1, oy = -1, ow = -1, oh = -1;
   char buf[80];
   BITMAP *graphiccopy;
   BITMAP *pattern;
   BITMAP *bmp;
   RLE_SPRITE *spr;

   show_mouse(NULL);
   sel_pallete(g_pallete);
   if (bitmap_color_depth(screen) == 8)
      clear_to_color(screen, mg);
   set_mouse_sprite(my_busy_pointer);
   show_mouse(screen);
   graphiccopy = create_bitmap(graphic->w, graphic->h);
   blit(graphic, graphiccopy, 0, 0, 0, 0, graphic->w, graphic->h);
   show_mouse(NULL);
   set_mouse_sprite(my_mouse_pointer);
   if (bitmap_color_depth(screen) != 8)
      clear_to_color(screen, mg);
   blit(graphiccopy, screen, 0, 0, 0, 0, SCREEN_W, SCREEN_H);

   pattern = create_bitmap(2, 2);
   putpixel(pattern, 0, 0, bg);
   putpixel(pattern, 1, 1, bg);
   putpixel(pattern, 0, 1, fg);
   putpixel(pattern, 1, 0, fg);

   do {
   } while (mouse_b);

   set_mouse_range(0, 0, graphic->w-1, graphic->h-1);

   do {
      x = mouse_x;
      y = mouse_y;

      if ((x >= graphic->w) || (y >= graphic->h)) {
	 x = MID(0, x, graphic->w-1);
	 y = MID(0, y, graphic->h-1);
	 position_mouse(x, y);
      }

      if (!box) {
	 x = (x / xgrid) * xgrid;
	 y = (y / ygrid) * ygrid;
      }

      if ((x != ox) || (y != oy)) {
	 blit(graphiccopy, screen, 0, 0, 0, 0, graphic->w, graphic->h);
	 hline(screen, 0, graphic->h, graphic->w, 0);
	 vline(screen, graphic->w, 0, graphic->h, 0);

	 drawing_mode(DRAW_MODE_COPY_PATTERN, pattern, 0, 0);

	 if (box) {
	    hline(screen, 0, y-1, graphic->w, 0);
	    vline(screen, x-1, 0, graphic->h, 0);
	 }
	 else {
	    hline(screen, x-1, y-1, graphic->w, 0);
	    vline(screen, x-1, y-1, graphic->h, 0);
	 }

	 drawing_mode(DRAW_MODE_SOLID, NULL, 0, 0);

	 sprintf(buf, "%d, %d", x, y);
	 text_mode(-1);
	 rectfill(screen, 0, SCREEN_H-8, SCREEN_W, SCREEN_H, fg);
	 textout(screen, font, buf, 0, SCREEN_H-8, bg);

	 ox = x;
	 oy = y;
      }

      if (keypressed()) {
	 switch (readkey() >> 8) {

	    case KEY_UP:
	       position_mouse(mouse_x, MAX(mouse_y-ygrid, 0));
	       break;

	    case KEY_DOWN:
	       position_mouse(mouse_x, mouse_y+ygrid);
	       break;

	    case KEY_RIGHT:
	       position_mouse(mouse_x+xgrid, mouse_y);
	       break;

	    case KEY_LEFT:
	       position_mouse(MAX(mouse_x-xgrid, 0), mouse_y);
	       break;

	    case KEY_ENTER:
	    case KEY_SPACE:
	       goto gottheposition;

	    case KEY_ESC:
	       goto getmeoutofhere;
	 }
      }
   } while (!mouse_b);

   if (mouse_b & 2)
      goto getmeoutofhere;

   gottheposition:

   if (box) {
      int xx, yy, ww, hh;

      xx = 0;
      yy = 0;

      datedit_find_character(graphic, &xx, &yy, &ww, &hh);

      while ((ww > 0) && (hh > 0)) {
	 if ((x > xx) && (x <= xx+ww) &&
	     (y > yy) && (y <= yy+hh)) {
	    x = xx+1;
	    y = yy+1;
	    w = ww;
	    h = hh;
	    goto gotthesize;
	 }

	 xx += ww;
	 datedit_find_character(graphic, &xx, &yy, &ww, &hh);
      }

      x = 0;
      y = 0;
      w = graphic->w;
      h = graphic->h;
   }
   else {
      using_mouse = (mouse_b & 1);

      do {
	 w = mouse_x;
	 h = mouse_y;

	 if ((w < x) || (w > graphic->w) || ( h < y) || (h > graphic->h)) {
	    w = MID(x, w, graphic->w);
	    h = MID(y, h, graphic->h);
	    position_mouse(w, h);
	 }

	 w = ((w - x) / xgrid + 1) * xgrid;
	 if (x+w > graphic->w)
	    w = graphic->w - x;

	 h = ((h - y) / ygrid + 1) * ygrid;
	 if (y+h > graphic->h)
	    h = graphic->h - y;

	 if ((w != ow) || (h != oh)) {
	    blit(graphiccopy, screen, 0, 0, 0, 0, graphic->w, graphic->h);
	    hline(screen, 0, graphic->h, graphic->w, 0);
	    vline(screen, graphic->w, 0, graphic->h, 0);

	    drawing_mode(DRAW_MODE_COPY_PATTERN, pattern, 0, 0);
	    hline(screen, x-1, y-1, x+w, 0);
	    hline(screen, x-1, y+h, x+w, 0);
	    vline(screen, x-1, y-1, y+h, 0);
	    vline(screen, x+w, y-1, y+h, 0);
	    drawing_mode(DRAW_MODE_SOLID, NULL, 0, 0);

	    sprintf(buf, "%d, %d (%dx%d)", x, y, w, h);
	    text_mode(-1);
	    rectfill(screen, 0, SCREEN_H-8, SCREEN_W, SCREEN_H, fg);
	    textout(screen, font, buf, 0, SCREEN_H-8, bg);

	    ow = w;
	    oh = h;
	 }

	 if (keypressed()) {
	    switch (readkey() >> 8) {

	       case KEY_UP:
		  position_mouse(mouse_x, MAX(mouse_y-ygrid, 0));
		  break;

	       case KEY_DOWN:
		  position_mouse(mouse_x, mouse_y+ygrid);
		  break;

	       case KEY_RIGHT:
		  position_mouse(mouse_x+xgrid, mouse_y);
		  break;

	       case KEY_LEFT:
		  position_mouse(MAX(mouse_x-xgrid, 0), mouse_y);
		  break;

	       case KEY_ENTER:
	       case KEY_SPACE:
		  goto gotthesize;

	       case KEY_ESC:
		  goto getmeoutofhere;
	    }
	 }

	 if (mouse_b & 2)
	    goto getmeoutofhere;

      } while (((mouse_b) && (using_mouse)) || ((!mouse_b) && (!using_mouse)));
   }

   gotthesize:

   if ((w > 0) && (h > 0)) {
      bmp = create_bitmap_ex(bitmap_color_depth(graphic), w, h);
      clear_to_color(bmp, bmp->vtable->mask_color);
      blit(graphic, bmp, x, y, 0, 0, w, h);

      if (dat->type == DAT_RLE_SPRITE) {
	 spr = get_rle_sprite(bmp);
	 destroy_bitmap(bmp);
	 destroy_rle_sprite(dat->dat);
	 dat->dat = spr;
      }
      else {
	 destroy_bitmap(dat->dat);
	 dat->dat = bmp;
      }

      sprintf(buf, "%d", x);
      datedit_set_property(dat, DAT_XPOS, buf);

      sprintf(buf, "%d", y);
      datedit_set_property(dat, DAT_YPOS, buf);

      sprintf(buf, "%d", w);
      datedit_set_property(dat, DAT_XSIZ, buf);

      sprintf(buf, "%d", h);
      datedit_set_property(dat, DAT_YSIZ, buf);

      datedit_set_property(dat, DAT_ORIG, graphic_origin);
      datedit_set_property(dat, DAT_DATE, graphic_date);
   }

   getmeoutofhere:

   if (mouse_b)
      clear_to_color(screen, mg);

   set_mouse_range(0, 0, SCREEN_W-1, SCREEN_H-1);

   destroy_bitmap(graphiccopy);
   destroy_bitmap(pattern);
   show_mouse(screen);

   do {
   } while (mouse_b);
}



/* handle the grab command */
int grabber()
{
   int sel = single_selection();
   DATAFILE *dat;
   char *desc = "binary data";
   char *ext = NULL;
   char buf[256], name[256], type[8];
   int i;

   if ((sel <= 0) || (sel >= data_count)) {
      if (sel < 0)
	 alert("Can't grab to a multiple selection!", NULL, NULL, "OK", NULL, 13, 0);
      else
	 alert("You must create an object to contain", "the data before you can grab it", NULL, "OK", NULL, 13, 0);
      return D_O_K;
   }

   dat = data[sel].dat;

   if ((dat->type == DAT_BITMAP) || (dat->type == DAT_RLE_SPRITE) ||
       (dat->type == DAT_C_SPRITE) || (dat->type == DAT_XC_SPRITE) ||
       (dat->type == DAT_PALLETE)) {
      if (!graphic) {
	 alert("You must read in a bitmap file",
	       "before you can grab data from it",
	       NULL, "OK", NULL, 13, 0);
	 return D_O_K;
      }

      if (dat->type == DAT_PALLETE) {
	 memcpy(dat->dat, g_pallete, sizeof(PALLETE));
	 datedit_set_property(dat, DAT_ORIG, graphic_origin);
	 datedit_set_property(dat, DAT_DATE, graphic_date);
	 alert("Palette data grabbed from the bitmap file",
	       NULL, NULL, "OK", NULL, 13, 0);
	 sel_pallete(dat->dat);
      }
      else {
	 grabbit(dat, FALSE);
      }
   }
   else {
      for (i=0; object_info[i].type != DAT_END; i++) {
	 if (object_info[i].type == dat->type) {
	    desc = object_info[i].desc;
	    ext = object_info[i].ext;
	    break;
	 }
      }

      strcpy(name, get_datafile_property(dat, DAT_ORIG));
      if (!name[0]) {
	 strcpy(name, import_file);
	 *get_filename(name) = 0;
      }

      if (ext)
	 sprintf(buf, "Grab %s (%s)", desc, ext);
      else
	 sprintf(buf, "Grab %s", desc);

      if (file_select(buf, name, ext)) {
	 strlwr(name);
	 set_mouse_sprite(my_busy_pointer);
	 busy_mouse = TRUE;

	 strcpy(import_file, name);
	 sprintf(type, "%c%c%c%c", dat->type>>24, (dat->type>>16)&0xFF, (dat->type>>8)&0xFF, dat->type&0xFF);

	 datedit_grabreplace(dat, name, get_datafile_property(dat, DAT_NAME), type, -1, -1, -1, -1, -1);

	 if (dat->type == DAT_FILE)
	    rebuild_list(NULL, TRUE);

	 set_mouse_sprite(my_mouse_pointer);
	 busy_mouse = FALSE;
      }
   }

   datedit_sort_properties(dat->prop);
   select_property(DAT_NAME);

   return D_REDRAW;
}



/* handle the box grab command */
int box_grabber()
{
   int sel = single_selection();
   DATAFILE *dat;

   if ((sel <= 0) || (sel >= data_count)) {
      if (sel < 0)
	 alert("Can't grab to a multiple selection!", NULL, NULL, "OK", NULL, 13, 0);
      else
	 alert("You must create an object to contain", "the data before you can grab it", NULL, "OK", NULL, 13, 0);
      return D_O_K;
   }

   dat = data[sel].dat;

   if ((dat->type != DAT_BITMAP) && (dat->type != DAT_RLE_SPRITE) &&
       (dat->type != DAT_C_SPRITE) && (dat->type != DAT_XC_SPRITE)) {
      alert("You can only box grab to a bitmap object!", NULL, NULL, "OK", NULL, 13, 0);
      return D_O_K;
   }

   if (!graphic) {
      alert("You must read in a bitmap file",
	    "before you can grab data from it",
	    NULL, "OK", NULL, 13, 0);
      return D_O_K;
   }

   grabbit(dat, TRUE);

   datedit_sort_properties(dat->prop);
   select_property(DAT_NAME);

   return D_REDRAW;
}



/* checks whether a bitmap contains any data */
int bitmap_is_empty(BITMAP *bmp)
{
   int x, y;
   int c = getpixel(bmp, 0, 0);

   for (y=0; y<bmp->h; y++)
      for (x=0; x<bmp->w; x++)
	 if (getpixel(bmp, x, y) != c)
	    return FALSE;

   return TRUE;
}



/* helper for grabbing images from a grid */
void *griddlit(DATAFILE **parent, char *name, int c, int type, int skipempty, int autocrop, int depth, int x, int y, int w, int h)
{
   DATAFILE *dat;
   BITMAP *bmp;
   void *v;
   char buf[256];
   RGB tmprgb = current_pallete[0];

   if ((type == DAT_RLE_SPRITE) || (type == DAT_C_SPRITE) || (type == DAT_XC_SPRITE)) {
      current_pallete[0].r = 63;
      current_pallete[0].g = 0;
      current_pallete[0].b = 63;
   }
   select_palette(current_pallete);

   bmp = create_bitmap_ex(depth, w, h);
   clear_to_color(bmp, bmp->vtable->mask_color);
   blit(graphic, bmp, x, y, 0, 0, w, h);

   unselect_palette(current_pallete);
   current_pallete[0] = tmprgb;

   if ((skipempty) && (bitmap_is_empty(bmp))) {
      destroy_bitmap(bmp);
      return NULL;
   }

   if (autocrop)
      bmp = crop_bitmap(bmp);

   if (type == DAT_RLE_SPRITE) {
      v = get_rle_sprite(bmp);
      destroy_bitmap(bmp);
   }
   else
      v = bmp;

   sprintf(buf, "%s%03d", name, c);

   *parent = datedit_insert(*parent, &dat, buf, type, v, 0);

   sprintf(buf, "%d", x);
   datedit_set_property(dat, DAT_XPOS, buf);

   sprintf(buf, "%d", y);
   datedit_set_property(dat, DAT_YPOS, buf);

   sprintf(buf, "%d", w);
   datedit_set_property(dat, DAT_XSIZ, buf);

   sprintf(buf, "%d", h);
   datedit_set_property(dat, DAT_YSIZ, buf);

   datedit_set_property(dat, DAT_ORIG, graphic_origin);
   datedit_set_property(dat, DAT_DATE, graphic_date);

   datedit_sort_properties(dat->prop);

   return v;
}



/* grabs images from a grid, using boxes of color #255 */
void *box_griddle(DATAFILE **parent, char *name, int type, int skipempty, int autocrop, int depth)
{
   void *ret = NULL;
   void *item = NULL;
   int x, y, w, h;
   int c = 0;

   x = 0;
   y = 0;

   datedit_find_character(graphic, &x, &y, &w, &h);

   while ((w > 0) && (h > 0)) {
      item = griddlit(parent, name, c, type, skipempty, autocrop, depth, x+1, y+1, w, h);
      if (item) {
	 c++;
	 if (!ret)
	    ret = item;
      }

      x += w;
      datedit_find_character(graphic, &x, &y, &w, &h);
   }

   return ret;
}



/* grabs images from a regular grid */
void *grid_griddle(DATAFILE **parent, char *name, int type, int skipempty, int autocrop, int depth, int xgrid, int ygrid)
{
   void *ret = NULL;
   void *item = NULL;
   int x, y;
   int c = 0;

   for (y=0; y+ygrid<=graphic->h; y+=ygrid) {
      for (x=0; x+xgrid<=graphic->w; x+=xgrid) {
	 item = griddlit(parent, name, c, type, skipempty, autocrop, depth, x, y, xgrid, ygrid);
	 if (item) {
	    c++;
	    if (!ret)
	       ret = item;
	 }
      } 
   }

   return ret;
}



char griddle_xgrid[8] = "32";
char griddle_ygrid[8] = "32";
char griddle_name[256] = "";



/* dialog callback for retrieving the contents of the object type list */
char *typelist_getter(int index, int *list_size)
{
   static char *str[] =
   {
      "Bitmap",
      "RLE Sprite",
      "Compiled Sprite",
      "Mode-X Compiled Sprite"
   };

   if (index < 0) {
      if (list_size)
	 *list_size = 4;
      return NULL;
   }

   return str[index];
}



/* dialog callback for retrieving the contents of the color depth list */
char *depthlist_getter(int index, int *list_size)
{
   static char *str[] =
   {
      "256 color palette",
      "15 bit hicolor",
      "16 bit hicolor",
      "24 bit truecolor",
      "32 bit truecolor"
   };

   if (index < 0) {
      if (list_size)
	 *list_size = 5;
      return NULL;
   }

   return str[index];
}



DIALOG griddle_dlg[] =
{
   /* (dialog proc)     (x)   (y)   (w)   (h)   (fg)  (bg)  (key)    (flags)     (d1)           (d2)     (dp) */
   { d_shadow_box_proc, 0,    0,    276,  304,  0,    0,    0,       0,          0,             0,       NULL },
   { d_ctext_proc,      138,  8,    0,    0,    0,    0,    0,       0,          0,             0,       "Grab from Grid" },
   { d_radio_proc,      16,   32,   120,  12,   0,    0,    0,       D_SELECTED, 0,             0,       "Use col #255" },
   { d_radio_proc,      16,   56,   120,  12,   0,    0,    0,       0,          0,             0,       "Regular grid" },
   { gg_text_proc,      160,  58,   0,    0,    0,    0,    0,       0,          0,             0,       "X-grid:" },
   { gg_edit_proc,      224,  58,   40,   8,    0,    0,    0,       0,          4,             0,       griddle_xgrid },
   { gg_text_proc,      160,  80,   0,    0,    0,    0,    0,       0,          0,             0,       "Y-grid:" },
   { gg_edit_proc,      224,  82,   40,   8,    0,    0,    0,       0,          4,             0,       griddle_ygrid },
   { d_check_proc,      16,   82,   122,  12,   0,    0,    0,       0,          0,             0,       "Skip empties:" },
   { d_check_proc,      16,   106,  90,   12,   0,    0,    0,       0,          0,             0,       "Autocrop:" },
   { d_text_proc,       16,   138,  0,    0,    0,    0,    0,       0,          0,             0,       "Name:" },
   { d_edit_proc,       64,   138,  204,  8,    0,    0,    0,       0,          255,           0,       griddle_name },
   { d_text_proc,       16,   160,  0,    0,    0,    0,    0,       0,          0,             0,       "Type:" },
   { d_list_proc,       64,   160,  196,  35,   0,    0,    0,       0,          0,             0,       typelist_getter },
   { d_text_proc,       16,   208,  0,    0,    0,    0,    0,       0,          0,             0,       "Cols:" },
   { d_list_proc,       64,   208,  196,  43,   0,    0,    0,       0,          0,             0,       depthlist_getter },
   { d_button_proc,     50,   272,  80,   16,   0,    0,    13,      D_EXIT,     0,             0,       "OK" }, 
   { d_button_proc,     146,  272,  80,   16,   0,    0,    27,      D_EXIT,     0,             0,       "Cancel" }, 
   { NULL }
};


#define GRIDDLE_DLG_BOXES        2
#define GRIDDLE_DLG_GRID         3
#define GRIDDLE_DLG_XGRID        5
#define GRIDDLE_DLG_YGRID        7
#define GRIDDLE_DLG_EMPTIES      8
#define GRIDDLE_DLG_AUTOCROP     9
#define GRIDDLE_DLG_NAME         11
#define GRIDDLE_DLG_TYPE         13
#define GRIDDLE_DLG_DEPTH        15
#define GRIDDLE_DLG_OK           16
#define GRIDDLE_DLG_CANCEL       17



/* wrapper for d_text_proc, that greys out when invalid */
int gg_text_proc(int msg, DIALOG *d, int c)
{
   if (msg == MSG_IDLE) {
      int valid = (griddle_dlg[GRIDDLE_DLG_BOXES].flags & D_SELECTED) ? 0 : 1;
      int enabled = (d->flags & D_DISABLED) ? 0 : 1;

      if (valid != enabled) {
	 if (valid)
	    d->flags &= ~D_DISABLED;
	 else
	    d->flags |= D_DISABLED;

	 show_mouse(NULL);
	 SEND_MESSAGE(d, MSG_DRAW, 0);
	 show_mouse(screen);
      }

      return D_O_K;
   }
   else
      return d_text_proc(msg, d, c);
}



/* wrapper for d_edit_proc, that greys out when invalid */
int gg_edit_proc(int msg, DIALOG *d, int c)
{
   if (msg == MSG_IDLE) {
      int valid = (griddle_dlg[GRIDDLE_DLG_BOXES].flags & D_SELECTED) ? 0 : 1;
      int enabled = (d->flags & D_DISABLED) ? 0 : 1;

      if (valid != enabled) {
	 if (valid)
	    d->flags &= ~D_DISABLED;
	 else
	    d->flags |= D_DISABLED;

	 show_mouse(NULL);
	 SEND_MESSAGE(d, MSG_DRAW, 0);
	 show_mouse(screen);
      }

      return D_O_K;
   }
   else
      return d_edit_proc(msg, d, c);
}



/* checks whether an object name matches the grid grab base name */
int grid_name_matches(char *gn, char *n)
{
   char *s;

   if (strncmp(gn, n, strlen(gn)) != 0)
      return FALSE;

   s = n + strlen(gn);
   if (*s == 0)
      return FALSE;

   while (*s) {
      if (!isdigit(*s))
	 return FALSE;
      s++;
   }

   return TRUE;
}



/* handle the griddle command */
int griddler()
{
   DATAFILE *dat;
   DATAFILE **parent;
   void *selitem;
   char *s;
   int c;
   int xgrid, ygrid;
   int done, type, skipempty, autocrop;
   int depth = 8;

   if (!graphic) {
      alert("You must read in a bitmap file",
	    "before you can grab data from it",
	    NULL, "OK", NULL, 13, 0);
      return D_O_K;
   }

   if ((SELECTED_ITEM <= 0) || (SELECTED_ITEM >= data_count)) {
      dat = NULL;
      parent = &datafile;
      griddle_name[0] = 0;
      depth = bitmap_color_depth(graphic);
   }
   else {
      dat = data[SELECTED_ITEM].dat;
      if (dat->type == DAT_FILE)
	 parent = (DATAFILE **)&dat->dat;
      else
	 parent = data[SELECTED_ITEM].parent;

      strcpy(griddle_name, get_datafile_property(dat, DAT_NAME));
      s = griddle_name + strlen(griddle_name) - 1;
      while ((s >= griddle_name) && (isdigit(*s))) {
	 *s = 0;
	 s--;
      }

      if (dat->type == DAT_BITMAP) {
	 griddle_dlg[GRIDDLE_DLG_TYPE].d1 = 0;
	 depth = bitmap_color_depth(dat->dat);
      }
      else if (dat->type == DAT_RLE_SPRITE) {
	 griddle_dlg[GRIDDLE_DLG_TYPE].d1 = 1;
	 depth = ((RLE_SPRITE *)dat->dat)->color_depth;
      }
      else if (dat->type == DAT_C_SPRITE) {
	 griddle_dlg[GRIDDLE_DLG_TYPE].d1 = 2;
	 depth = bitmap_color_depth(dat->dat);
      }
      else if (dat->type == DAT_XC_SPRITE) {
	 griddle_dlg[GRIDDLE_DLG_TYPE].d1 = 3;
	 depth = 8;
      }
   }

   if (depth == 8)
      griddle_dlg[GRIDDLE_DLG_DEPTH].d1 = 0;
   else if (depth == 15)
      griddle_dlg[GRIDDLE_DLG_DEPTH].d1 = 1;
   else if (depth == 16)
      griddle_dlg[GRIDDLE_DLG_DEPTH].d1 = 2;
   else if (depth == 24)
      griddle_dlg[GRIDDLE_DLG_DEPTH].d1 = 3;
   else if (depth == 32)
      griddle_dlg[GRIDDLE_DLG_DEPTH].d1 = 4;

   centre_dialog(griddle_dlg);
   set_dialog_color(griddle_dlg, fg, bg);

   if (do_dialog(griddle_dlg, GRIDDLE_DLG_NAME) == GRIDDLE_DLG_CANCEL)
      return D_REDRAW;

   set_mouse_sprite(my_busy_pointer);
   busy_mouse = TRUE;

   sel_pallete(g_pallete);

   do {
      done = TRUE;

      for (c=0; (*parent)[c].type != DAT_END; c++) {
	 if (grid_name_matches(griddle_name, get_datafile_property((*parent)+c, DAT_NAME))) {
	    *parent = datedit_delete(*parent, c);
	    done = FALSE;
	    break;
	 }
      }
   } while (!done);

   switch (griddle_dlg[GRIDDLE_DLG_TYPE].d1) {

      case 0:
      default:
	 type = DAT_BITMAP;
	 break;

      case 1:
	 type = DAT_RLE_SPRITE;
	 break;

      case 2:
	 type = DAT_C_SPRITE;
	 break;

      case 3:
	 type = DAT_XC_SPRITE;
	 break;
   }

   switch (griddle_dlg[GRIDDLE_DLG_DEPTH].d1) {

      case 0:
      default:
	 depth = 8;
	 break;

      case 1:
	 depth = 15;
	 break;

      case 2:
	 depth = 16;
	 break;

      case 3:
	 depth = 24;
	 break;

      case 4:
	 depth = 32;
	 break;
   }

   skipempty = griddle_dlg[GRIDDLE_DLG_EMPTIES].flags & D_SELECTED;
   autocrop = griddle_dlg[GRIDDLE_DLG_AUTOCROP].flags & D_SELECTED;

   if (griddle_dlg[GRIDDLE_DLG_BOXES].flags & D_SELECTED)
      selitem = box_griddle(parent, griddle_name, type, skipempty, autocrop, depth);
   else {
      xgrid = MID(1, atoi(griddle_xgrid), 0xFFFF);
      ygrid = MID(1, atoi(griddle_ygrid), 0xFFFF);
      selitem = grid_griddle(parent, griddle_name, type, skipempty, autocrop, depth, xgrid, ygrid);
   }

   datedit_sort_datafile(*parent);
   rebuild_list(selitem, TRUE);
   select_property(DAT_NAME);

   set_mouse_sprite(my_mouse_pointer);
   busy_mouse = FALSE;

   if (selitem) 
      sel_pallete(g_pallete);
   else
      alert("No grid found - nothing grabbed!", NULL, NULL, "Hmm...", NULL, 13, 0);

   return D_REDRAW;
}



/* handle the export command */
int exporter()
{
   int sel = single_selection();
   char *desc = "binary data";
   char *ext = NULL;
   char buf[256], name[256];
   DATAFILE *dat;
   int i;

   if ((sel <= 0) || (sel >= data_count)) {
      if (sel < 0)
	 alert("Can't export a multiple selection!", NULL, NULL, "OK", NULL, 13, 0);
      else
	 alert("Nothing to export!", NULL, NULL, "OK", NULL, 13, 0);
      return D_O_K;
   }

   dat = data[sel].dat;

   for (i=0; object_info[i].type != DAT_END; i++) {
      if (object_info[i].type == dat->type) {
	 desc = object_info[i].desc;
	 ext = object_info[i].exp_ext;
	 break;
      }
   }

   strcpy(name, import_file);
   *get_filename(name) = 0;

   if (*get_datafile_property(dat, DAT_ORIG)) {
      datedit_export_name(dat, NULL, ext, buf);
      strcat(name, get_filename(buf));
   }

   if (ext)
      sprintf(buf, "Export %s (%s)", desc, ext);
   else
      sprintf(buf, "Export %s", desc);

   if (file_select(buf, name, ext)) {
      strlwr(name);
      set_mouse_sprite(my_busy_pointer);
      busy_mouse = TRUE;

      strcpy(import_file, name);
      update_info();
      datedit_export(dat, name);

      set_mouse_sprite(my_mouse_pointer);
      busy_mouse = FALSE;
   }

   return D_REDRAW;
}



/* handle the delete command */
int deleter()
{
   void *dat;
   void **todel = NULL;
   int todel_count = 0;
   int todel_alloc = 0;
   char buf[256];
   char *name = "";
   int first = 0;
   int i;

   for (i=1; i<data_count; i++) {
      if ((i == SELECTED_ITEM) || (data_sel[i])) {
	 if (todel_count == 0) {
	    name = get_datafile_property(data[i].dat, DAT_NAME);
	    first = i;
	 }
	 if (todel_count >= todel_alloc) {
	    todel_alloc += 16;
	    todel = realloc(todel, todel_alloc * sizeof(void *));
	 }
	 todel[todel_count++] = data[i].dat->dat;
      }
   }

   if (todel_count <= 0) {
      alert("Nothing to delete!", NULL, NULL, "OK", NULL, 13, 0);
      return D_O_K;
   }

   if (todel_count == 1)
      sprintf(buf, "%s?", name);
   else
      sprintf(buf, "these %d items?", todel_count);

   if (alert("Really delete", buf, NULL, "Yes", "Cancel", 'y', 27) != 1)
      return D_O_K;

   SELECTED_ITEM = first;

   while (todel_count > 0) {
      dat = todel[--todel_count];
      for (i=1; i<data_count; i++) { 
	 if (data[i].dat->dat == dat) {
	    *data[i].parent = datedit_delete(*data[i].parent, data[i].i);
	    rebuild_list(NULL, TRUE);
	    break;
	 }
      }
   }

   free(todel);

   return D_REDRAW;
}



/* help system by Doug Eleveld */

char *grabber_last_help = NULL;
char *grabber_help_text = NULL;
char *grabber_help_text_section[8];


int set_grabber_help_main()
{
   grabber_help_text = grabber_help_text_section[0];
   return D_O_K;
}


int set_grabber_help_using_grabber()
{
   grabber_help_text = grabber_help_text_section[1];
   return D_O_K;
}


int set_grabber_help_using_archiver()
{
   grabber_help_text = grabber_help_text_section[2];
   return D_O_K;
}


int set_grabber_help_misc()
{
   grabber_help_text = grabber_help_text_section[3];
   return D_O_K;
}


int set_grabber_help_accessing_datafiles()
{
   grabber_help_text = grabber_help_text_section[4];
   return D_O_K;
}


int set_grabber_help_compiling_datafiles()
{
   grabber_help_text = grabber_help_text_section[5];
   return D_O_K;
}


int set_grabber_help_custom_objects()
{
   grabber_help_text = grabber_help_text_section[6];
   return D_O_K;
}


int set_grabber_help_file_format()
{
   grabber_help_text = grabber_help_text_section[7];
   return D_O_K;
}


int do_grabber_help_exit(void)
{
   return D_CLOSE;
}



MENU grabber_help_datafiles[] =
{
   { "&Accessing",      set_grabber_help_accessing_datafiles,  NULL },
   { "&Compiling",      set_grabber_help_compiling_datafiles,  NULL },
   { NULL,              NULL,                                  NULL }
};


MENU grabber_help_choose_topic[] =
{
   { "&Main",           set_grabber_help_main,                 NULL },
   { "&Grabber",        set_grabber_help_using_grabber,        NULL },
   { "&Archiver",       set_grabber_help_using_archiver,       NULL },
   { "&Datafiles",      NULL,                                  grabber_help_datafiles },
   { "&Custom Objects", set_grabber_help_custom_objects,       NULL },
   { "&File format",    set_grabber_help_file_format,          NULL },
   { "M&isc",           set_grabber_help_misc,                 NULL },
   { NULL,              NULL,                                  NULL }
};


MENU grabber_help_topic[] =
{
   { "&Topic",          NULL,                                  grabber_help_choose_topic },
   { "E&xit",           do_grabber_help_exit,                  NULL },
   { NULL,              NULL,                                  NULL }
};



int d_helptext_proc(int msg, DIALOG *d, int c)
{
   if (d->dp == NULL)
      d->dp = grabber_help_text;

   /* if the text needs changing changed, redraw */
   if ((msg == MSG_IDLE) && (grabber_help_text != grabber_last_help)) {
      d->dp = grabber_help_text;
      grabber_last_help = grabber_help_text;

      d->d2 = 0;

      show_mouse(NULL);
      d_textbox_proc(MSG_DRAW, d, c);
      show_mouse(screen);

      return D_O_K;
   }
   else 
      return d_textbox_proc(msg, d, c);
};



DIALOG grabber_help[] =
{
   /* (dialog proc)      (x)   (y)   (w)   (h)   (fg)  (bg)  (key) (flags)  (d1)  (d2)  (dp) */
   { d_clear_proc,       0,    0,    0,    0,    8,    8,    0,    0,       0,    0,    NULL },
   { d_text_proc,        0,    4,    0,    0,    255,  8,    0,    0,       0,    0,    "Help viewer by Doug Eleveld" },
   { d_menu_proc,        0,    0,    0,    0,    255,  0,    0,    0,       0,    0,    grabber_help_topic },
   { d_helptext_proc,    0,    20,   0,    0,    255,  0,    0,    0,       0,    0,    NULL },
   { NULL,               0,    0,    0,    0,    0,    0,    0,    0,       0,    0,    NULL }
};



/* handle the help command */
int helper()
{
   char buf[256];
   PACKFILE *f;
   char *last, *s; 
   int size, i, j;

   strcpy(buf, argv_0);
   strcpy(get_filename(buf), "grabber.txt");

   size = file_size(buf);
   if (size <= 0) {
      alert("Error reading grabber.txt", NULL, NULL, "Oh dear", NULL, 13, 0);
      return D_REDRAW;
   }

   grabber_help_text = malloc(size+1); 

   f = pack_fopen(buf, F_READ);
   if (!f) {
      alert("Error reading grabber.txt", NULL, NULL, "Oh dear", NULL, 13, 0);
      return D_REDRAW;
   }

   pack_fread(grabber_help_text, size, f);
   pack_fclose(f);

   grabber_help_text[size] = 0;
   grabber_help_text_section[0] = grabber_help_text;

   last = grabber_help_text;

   while ((s = strstr(last, " \r\n")) != NULL) {
      last = s+3;
      if ((SCREEN_W >= 640) && (isspace(*last)))
	 continue;
      j = 0;
      while (isspace(last[j]))
	 j++;
      s++;
      memmove(s, s+2+j, size - ((long)s-(long)grabber_help_text) - 1 - j);
      size -= 2+j;
      last = s;
   }

   last = grabber_help_text;

   for (i=1; i<8; i++) {
      s = strstr(last , "\r\n\r\n====");

      if (s) {
	 grabber_help_text_section[i] = last = s+4;
	 s[3] = 0;
      }
      else
	 grabber_help_text_section[i] = NULL;
   }

   grabber_last_help = NULL;

   set_dialog_color(grabber_help, fg, bg);
   grabber_help[0].fg = mg;
   grabber_help[0].bg = mg;
   grabber_help[1].bg = mg;

   grabber_help[3].d1 = 0;
   grabber_help[3].d2 = 0;
   grabber_help[3].dp = grabber_help_text;

   if (SCREEN_W > 512)
      grabber_help[1].x = SCREEN_W - 220;
   else
      grabber_help[1].x = SCREEN_W;

   grabber_help[3].w = SCREEN_W;
   grabber_help[3].h = SCREEN_H - 20;

   do_dialog(grabber_help, 3);

   free(grabber_help_text);

   return D_REDRAW;
}



/* handle the system info command */
int sysinfo()
{
   char b1[256], b2[256], b3[256];

   check_cpu();

   sprintf(b1, "%s %d86", cpu_vendor, cpu_family);

   if (cpu_fpu)
      strcat(b1, ", FPU");

   if (cpu_mmx)
      strcat(b1, ", MMX");

   sprintf(b2, "%s, %dx%d, %d bpp", gfx_driver->name, SCREEN_W, SCREEN_H, bitmap_color_depth(screen));
   sprintf(b3, "%s / %s", digi_driver->name, midi_driver->name);

   alert(b1, b2, b3, "OK", NULL, 13, 0);

   return D_O_K;
}



/* handle the about command */
int about()
{
   alert("Allegro Datafile Editor, version " ALLEGRO_VERSION_STR,
	 "By Shawn Hargreaves, " ALLEGRO_DATE_STR,
	 NULL, "OK", NULL, 13, 0);

   return D_O_K;
}



void *makenew_data(long *size)
{
   static char msg[] = "Binary Data";

   void *v = malloc(sizeof(msg));

   strcpy(v, msg);
   *size = sizeof(msg);

   return v;
}



void *makenew_file()
{
   DATAFILE *dat = malloc(sizeof(DATAFILE));

   dat->dat = NULL;
   dat->type = DAT_END;
   dat->size = 0;
   dat->prop = NULL;

   return dat;
}



void *makenew_font()
{
   FONT *f = malloc(sizeof(FONT));

   f->height = 8;
   f->dat.dat_8x8 = malloc(sizeof(FONT_8x8));
   memcpy(f->dat.dat_8x8, font->dat.dat_8x8, sizeof(FONT_8x8));

   return f;
}



void *makenew_sample()
{
   SAMPLE *spl;
   int c;

   spl = malloc(sizeof(SAMPLE));
   spl->bits = 8;
   spl->freq = 11025;
   spl->priority = 255;
   spl->len = 1024;
   spl->loop_start = 0;
   spl->loop_end = 1024;
   spl->param = -1;
   spl->data = malloc(1024);

   for (c=0; c<1024; c++)
      ((unsigned char *)spl->data)[c] = c & 0xFF;

   return spl;
}



void *makenew_midi()
{
   MIDI *mid;
   int c;

   mid = malloc(sizeof(MIDI));
   mid->divisions = 120;

   for (c=0; c<MIDI_TRACKS; c++) {
      mid->track[c].data = NULL;
      mid->track[c].len = 0;
   }

   return mid;
}



void *makenew_fli(long *size)
{
   char *v = malloc(1);

   *v = 0;
   *size = 1;

   return v;
}



void *makenew_bitmap()
{
   BITMAP *bmp = create_bitmap_ex(8, 32, 32);

   clear(bmp);
   text_mode(-1);
   textout_centre(bmp, font, "Hi!", 16, 12, 1);

   return bmp;
}



void *makenew_rle_sprite()
{
   BITMAP *bmp = makenew_bitmap();
   RLE_SPRITE *spr = get_rle_sprite(bmp);

   destroy_bitmap(bmp);

   return spr;
}



void *makenew_pallete(long *size)
{
   RGB *pal = malloc(sizeof(PALLETE));

   memcpy(pal, desktop_pallete, sizeof(PALLETE));
   *size = sizeof(PALLETE);

   return pal;
}



/* handle the new object command */
int new_object(int type)
{
   DATAITEM *dat;
   DATAFILE **df;
   void *v;
   long size = 0;

   if ((SELECTED_ITEM >= 0) && (SELECTED_ITEM < data_count))
      dat = data+SELECTED_ITEM;
   else
      return D_O_K;

   if (do_edit("New Object", "Type:", "Name:", type, NULL, (type == 0), TRUE)) {
      if (prop_type_string[0]) {
	 type = datedit_clean_typename(prop_type_string);
	 check_valid_name(prop_value_string);

	 switch (type) {

	    case DAT_FILE:
	       v = makenew_file();
	       break;

	    case DAT_FONT:
	       v = makenew_font();
	       break;

	    case DAT_SAMPLE:
	       v = makenew_sample();
	       break;

	    case DAT_MIDI:
	       v = makenew_midi();
	       break;

	    case DAT_FLI:
	       v = makenew_fli(&size);
	       break;

	    case DAT_BITMAP:
	    case DAT_C_SPRITE:
	    case DAT_XC_SPRITE:
	       v = makenew_bitmap();
	       break;

	    case DAT_RLE_SPRITE:
	       v = makenew_rle_sprite();
	       break;

	    case DAT_PALLETE:
	       v = makenew_pallete(&size);
	       break;

	    default:
	       v = makenew_data(&size);
	       break;
	 }

	 if ((dat->dat) && (dat->dat->type == DAT_FILE))
	    df = (DATAFILE **)&dat->dat->dat;
	 else
	    df = dat->parent;

	 *df = datedit_insert(*df, NULL, prop_value_string, type, v, size);
	 datedit_sort_datafile(*df);
	 rebuild_list(v, TRUE);
	 select_property(DAT_NAME);
      }
   }

   return D_REDRAW;
}



int new_bitmap()
{
   return new_object(DAT_BITMAP);
}



int new_rle_sprite()
{
   return new_object(DAT_RLE_SPRITE);
}



int new_c_sprite()
{
   return new_object(DAT_C_SPRITE);
}



int new_xc_sprite()
{
   return new_object(DAT_XC_SPRITE);
}



int new_pallete()
{
   return new_object(DAT_PALLETE);
}



int new_font()
{
   return new_object(DAT_FONT);
}



int new_sample()
{
   return new_object(DAT_SAMPLE);
}



int new_midi()
{
   return new_object(DAT_MIDI);
}



int new_fli()
{
   return new_object(DAT_FLI);
}



int new_datafile()
{
   return new_object(DAT_FILE);
}



int new_binary()
{
   return new_object(DAT_DATA);
}



int new_other()
{
   return new_object(0);
}



/* worker function for changing the type of an image */
int do_changeto(DATAFILE *dat, int *param, int type)
{
   BITMAP *bmp;
   RLE_SPRITE *spr;

   if ((dat->type != DAT_BITMAP) && (dat->type != DAT_RLE_SPRITE) &&
       (dat->type != DAT_C_SPRITE) && (dat->type != DAT_XC_SPRITE)) {
      (*param)++;
      return D_O_K;
   }

   if (dat->type == type)
      return D_O_K;

   if (dat->type == DAT_RLE_SPRITE) {
      spr = (RLE_SPRITE *)dat->dat;
      bmp = create_bitmap_ex(spr->color_depth, spr->w, spr->h);
      clear_to_color(bmp, bmp->vtable->mask_color);
      draw_rle_sprite(bmp, spr, 0, 0);
      dat->dat = bmp;
      destroy_rle_sprite(spr);
   }
   else if (type == DAT_RLE_SPRITE) {
      bmp = (BITMAP *)dat->dat;
      spr = get_rle_sprite(bmp);
      dat->dat = spr;
      destroy_bitmap(bmp);
   }

   dat->type = type;

   return D_REDRAW;
}



/* changes the type of bitmap data */
int changeto(int type)
{
   char buf[80];
   int ret, n;
   int p = 0;

   ret = foreach_selection(do_changeto, &n, &p, type);

   if (n <= 0) {
      alert ("Nothing to re-type!", NULL, NULL, "OK", NULL, 13, 0);
   }
   else if (p > 0) {
      sprintf(buf, "%d non-bitmap object%s ignored", p, (p==1) ? " was" : "s were");
      alert(buf, NULL, NULL, "OK", NULL, 13, 0);
   }

   if (n > p)
      rebuild_list(NULL, FALSE);

   return ret;
}



int changeto_bmp()
{
   return changeto(DAT_BITMAP);
}



int changeto_rle()
{
   return changeto(DAT_RLE_SPRITE);
}



int changeto_c()
{
   return changeto(DAT_C_SPRITE);
}



int changeto_xc()
{
   return changeto(DAT_XC_SPRITE);
}



/* worker function for changing the color depth of bitmap data*/
int do_changedepth(DATAFILE *dat, int *param, int depth)
{
   BITMAP *bmp, *bmp2;
   RLE_SPRITE *spr;
   RGB tmprgb = current_pallete[0];

   if ((dat->type != DAT_BITMAP) && (dat->type != DAT_RLE_SPRITE) &&
       (dat->type != DAT_C_SPRITE) && (dat->type != DAT_XC_SPRITE)) {
      (*param)++;
      return D_O_K;
   }

   if (dat->type == DAT_RLE_SPRITE) {
      spr = (RLE_SPRITE *)dat->dat;
      if (spr->color_depth == depth)
	 return D_O_K;

      set_mouse_sprite(my_busy_pointer);
      bmp = create_bitmap_ex(spr->color_depth, spr->w, spr->h);
      clear_to_color(bmp, bmp->vtable->mask_color);
      draw_rle_sprite(bmp, spr, 0, 0);
      bmp2 = create_bitmap_ex(depth, bmp->w, bmp->h);

      current_pallete[0].r = 63;
      current_pallete[0].g = 0;
      current_pallete[0].b = 63;
      select_palette(current_pallete);

      blit(bmp, bmp2, 0, 0, 0, 0, bmp->w, bmp->h);

      unselect_palette(current_pallete);
      current_pallete[0] = tmprgb;

      dat->dat = get_rle_sprite(bmp2);
      destroy_bitmap(bmp);
      destroy_bitmap(bmp2);
      destroy_rle_sprite(spr);
      set_mouse_sprite(my_mouse_pointer);
   }
   else {
      bmp = (BITMAP *)dat->dat;
      if (bitmap_color_depth(bmp) == depth)
	 return D_O_K;
      set_mouse_sprite(my_busy_pointer);
      bmp2 = create_bitmap_ex(depth, bmp->w, bmp->h);

      if ((dat->type == DAT_C_SPRITE) || (dat->type == DAT_XC_SPRITE)) {
	 current_pallete[0].r = 63;
	 current_pallete[0].g = 0;
	 current_pallete[0].b = 63;
      }
      select_palette(current_pallete);

      blit(bmp, bmp2, 0, 0, 0, 0, bmp->w, bmp->h);

      unselect_palette(current_pallete);
      current_pallete[0] = tmprgb;

      dat->dat = bmp2;
      destroy_bitmap(bmp);
      set_mouse_sprite(my_mouse_pointer);
   }

   return D_REDRAW;
}



/* changes the color depth of bitmap data */
int changedepth(int depth)
{
   char buf[80];
   int ret, n;
   int p = 0;

   ret = foreach_selection(do_changedepth, &n, &p, depth);

   if (n <= 0) {
      alert ("Nothing to re-format!", NULL, NULL, "OK", NULL, 13, 0);
   }
   else if (p > 0) {
      sprintf(buf, "%d non-bitmap object%s ignored", p, (p==1) ? " was" : "s were");
      alert(buf, NULL, NULL, "OK", NULL, 13, 0);
   }

   return ret;
}



int changeto_8()
{
   return changedepth(8);
}



int changeto_15()
{
   return changedepth(15);
}



int changeto_16()
{
   return changedepth(16);
}



int changeto_24()
{
   return changedepth(24);
}



int changeto_32()
{
   return changedepth(32);
}



/* menu callback to activate an external shell tool */
int sheller()
{
   __dpmi_regs r;
   DATAFILE *dat;
   char buf[256], cmd[256], ext[256], filename[256];
   char *s, *s2;
   int sel = single_selection();
   int oldw = SCREEN_W;
   int oldh = SCREEN_H;
   int export, delfile;
   int ret, c, i;

   if ((sel <= 0) || (sel >= data_count)) {
      if (sel < 0)
	 alert("Can't Shell Edit a multiple selection!", NULL, NULL, "OK", NULL, 13, 0);
      else
	 alert("Nothing to Shell Edit!", NULL, NULL, "OK", NULL, 13, 0);
      return D_O_K;
   }

   dat = data[sel].dat;

   if (dat->type == DAT_FILE) {
      alert("Can't Shell Edit a datafile object!", NULL, NULL, "OK", NULL, 13, 0);
      return D_O_K;
   }

   allegro_exit();

   in_text = TRUE;

   buf[0] = (dat->type >> 24) & 0xFF;
   buf[1] = (dat->type >> 16) & 0xFF;
   buf[2] = (dat->type >> 8) & 0xFF;
   buf[3] = dat->type & 0xFF;
   buf[4] = 0;

   for (i=3; (i>0) && (buf[i] == ' '); i--)
      buf[i] = 0;

   s = get_config_string("grabber", buf, "");

   if ((!s) || (!s[0])) {
      printf("No shell association for this object type!\n");
      printf("Add a \"%s=command\" line to the [grabber] section in allegro.cfg\n", buf);
      printf("\nPress a key to continue...\n");

      while (kbhit())
	 getkey();
      getkey();

      delfile = FALSE;
      goto ohwellitwasaniceidea;
   }

   strcpy(filename, get_datafile_property(dat, DAT_ORIG));

   if (filename[0]) {
      export = !exists(filename);
      delfile = FALSE;
   }
   else {
      strcpy(ext, "tmp");
      for (i=0; object_info[i].type != DAT_END; i++) {
	 if (object_info[i].type == dat->type) {
	    strcpy(ext, object_info[i].exp_ext);
	    for (c=0; ext[c]; c++) {
	       if (ext[c] == ';') {
		  ext[c] = 0;
		  break;
	       }
	    }
	    break;
	 }
      }

      s2 = getenv("TEMPDIR");
      if (s2)
	 strcpy(buf, s2);
      else {
	 s2 = getenv("TEMP");
	 if (s2)
	    strcpy(buf, s2);
	 else {
	    s2 = getenv("TMP");
	    if (s2)
	       strcpy(buf, s2);
	    else
	       strcpy(buf, "c:\\");
	 }
      }
      put_backslash(buf);

      i = 0;
      do {
	 sprintf(filename, "%sgrab%04d.%s", buf, i, ext);
	 i++;
      } while (exists(filename));

      export = TRUE;
      delfile = TRUE;
   }

   strlwr(filename);

   if (export) {
      if (!datedit_export(dat, filename)) {
	 printf("\nPress a key to continue...\n");

	 while (kbhit())
	    getkey();
	 getkey();

	 goto ohwellitwasaniceidea;
      }
   }

   sprintf(cmd, "%s %s", s, filename);
   printf("%s\n", cmd);

   if (os_type == OSTYPE_WIN95)
      /* fool Windows into noticing that the sound hardware is now free */
      system("command.com /c");

   ret = system(cmd);

   if (os_type == OSTYPE_WIN95) {
      r.x.ax = 0x168B;
      r.x.bx = 0;
      __dpmi_int(0x2F, &r);
   }

   if (ret) {
      printf("\nTool returned an error status! (exit code %d)\n", ret);
      c = datedit_ask("Do you still want to regrab the modified data");
      printf("\n");
      if ((c == 27) || (c == 'n') || (c == 'N'))
	 goto ohwellitwasaniceidea;
   }

   datedit_msg("Updating %s -> %s", filename, get_datafile_property(dat, DAT_NAME));

   if (!datedit_grabupdate(dat, filename, datedit_numprop(dat, DAT_XPOS), 
					  datedit_numprop(dat, DAT_YPOS), 
					  datedit_numprop(dat, DAT_XSIZ), 
					  datedit_numprop(dat, DAT_YSIZ))) {
      printf("\nPress a key to continue...\n");
      getkey();
   }

   datedit_sort_properties(dat->prop);
   select_property(DAT_NAME);

   ohwellitwasaniceidea:

   if (delfile) {
      printf("Deleting %s\n", filename);
      delete_file(filename);
   }

   while (kbhit())
      getkey();

   allegro_init();
   install_keyboard();
   install_mouse();
   install_timer();

   in_text = FALSE;

   if (set_gfx_mode(GFX_AUTODETECT, oldw, oldh, 0, 0) != 0) {
      destroy_bitmap(my_mouse_pointer);
      my_mouse_pointer = NULL;
      destroy_bitmap(my_busy_pointer);
      my_busy_pointer = NULL;
      set_color_depth(8);
      set_gfx_mode(GFX_VGA, 320, 200, 0, 0);
      sel_pallete(current_pallete);
      alert("bad, Bad, BAD error!",
	    "Unable to restore the graphics mode...",
	    NULL, "Shit", NULL, 13, 0);
   }
   else
      sel_pallete(current_pallete);

   if (install_sound(DIGI_AUTODETECT, MIDI_AUTODETECT, NULL) != 0) {
      alert("bad, Bad, BAD error!",
	    "Unable to reset the soundcard...",
	    NULL, "Shit", NULL, 13, 0);
      install_sound(DIGI_NONE, MIDI_NONE, NULL);
   }

   clear_keybuf();

   return D_REDRAW;
}



/* callback for the datedit functions to display a message */
void datedit_msg(char *fmt, ...)
{
   va_list args;
   char buf[1024];

   va_start(args, fmt);
   vsprintf(buf, fmt, args);
   va_end(args);

   if (in_text) {
      printf("%s\n", buf);
   }
   else {
      box_out(buf);
      box_eol();
   }
}



/* callback for the datedit functions to start a multi-part message */
void datedit_startmsg(char *fmt, ...)
{
   va_list args;
   char buf[1024];

   va_start(args, fmt);
   vsprintf(buf, fmt, args);
   va_end(args);

   if (in_text) {
      printf("%s", buf);
      fflush(stdout);
   }
   else {
      box_out(buf);
   }
}



/* callback for the datedit functions to end a multi-part message */
void datedit_endmsg(char *fmt, ...)
{
   va_list args;
   char buf[1024];

   va_start(args, fmt);
   vsprintf(buf, fmt, args);
   va_end(args);

   if (in_text) {
      printf("%s\n", buf);
   }
   else {
      box_out(buf);
      box_eol();
   }
}



/* callback for the datedit functions to report an error */
void datedit_error(char *fmt, ...)
{
   va_list args;
   char buf[1024];

   va_start(args, fmt);
   vsprintf(buf, fmt, args);
   va_end(args);

   if (in_text) {
      fprintf(stderr, "%s\n", buf);
   }
   else {
      set_mouse_sprite(my_mouse_pointer);

      alert(buf, NULL, NULL, "Oh dear", NULL, 13, 0);

      if (busy_mouse)
	 set_mouse_sprite(my_busy_pointer);
   }
}



/* callback for the datedit functions to ask a question */
int datedit_ask(char *fmt, ...)
{
   va_list args;
   char buf[1024];
   int ret;
   int c;

   va_start(args, fmt);
   vsprintf(buf, fmt, args);
   va_end(args);

   if (in_text) {
      printf("%s? (y/n)", buf);
      fflush(stdout);

      while (kbhit())
	 getkey();

      for (;;) {
	 c = getkey() & 0xFF;

	 switch (c) {

	    case 'y':
	    case 'Y':
	       printf("%c\n", c);
	       return 'y';

	    case 'n':
	    case 'N':
	       printf("%c\n", c);
	       return 'n';

	    case 27:
	       printf("\n");
	       return 27;
	 }
      }
   }
   else {
      strcat(buf, "?");

      set_mouse_sprite(my_mouse_pointer);

      ret = alert(buf, NULL, NULL, "Yes", "Cancel", 'y', 27);

      if (busy_mouse)
	 set_mouse_sprite(my_busy_pointer);

      if (ret == 1)
	 return 'y';
      else
	 return 'n';
   }
}



int main(int argc, char *argv[])
{
   int i;
   int ret = -1;
   int bpp = -1;
   int w = 640;
   int h = 480;
   char *s, tmp[256];
   char *fname = NULL;
   static int color_depths[] = { 32, 16, 15, 8, 0 };

   argv_0 = argv[0];

   for (i=1; i<argc; i++) {
      if ((argv[i][0] == '-') || (argv[i][0] == '/')) {
	 if ((rindex(argv[i], 'x')) || (rindex(argv[i], 'X'))) {
	    strcpy(tmp, argv[i]+1);
	    s = strtok(tmp, "xX");
	    if (s) {
	       w = atoi(s);
	       s = strtok(NULL, "xX");
	       if (s)
		  h = atoi(s);
	       else
		  h = 0;
	    }
	    else 
	       w = 0;
	    if ((w < 320) || (h < 200)) {
	       printf("\nInvalid display resolution '%s'\n\n", argv[i]+1);
	       return 1;
	    }
	 }
	 else {
	    bpp = atoi(argv[i]+1);
	    if ((bpp != 8) && (bpp != 15) && (bpp != 16) && (bpp != 24) && (bpp != 32)) {
	       printf("\nInvalid color depth '%s'\n\n", argv[i]+1);
	       return 1;
	    }
	 }
      }
      else
	 fname = argv[i];
   }

   __djgpp_set_ctrl_c(0);
   setcbrk(0);

   allegro_init();
   install_keyboard();
   install_mouse();
   install_timer();

   set_color_conversion(COLORCONV_NONE);

   if (bpp > 0) {
      set_color_depth(bpp);
      ret = set_gfx_mode(GFX_AUTODETECT, w, h, 0, 0);
   }
   else {
      for (i=0; color_depths[i]; i++) {
	 bpp = color_depths[i];
	 set_color_depth(color_depths[i]);
	 ret = set_gfx_mode(GFX_AUTODETECT, w, h, 0, 0);
	 if (ret == 0)
	    break;
      }
   }

   if (ret != 0) {
      allegro_exit();
      printf("\nError setting %dx%d %d bpp graphics mode\n%s\n\n", w, h, bpp, allegro_error);
      return 1;
   }

   if (install_sound(DIGI_AUTODETECT, MIDI_AUTODETECT, NULL) != 0) {
      allegro_exit();
      printf("Error initialising sound\n%s\n\n", allegro_error);
      return 1;
   }

   if (SCREEN_W < 512) {
      prop_dlg[0].w = prop_dlg[0].w * 3/4;
      for (i=1; prop_dlg[i].proc; i++) {
	 if (prop_dlg[i].x > 100)
	    prop_dlg[i].x = prop_dlg[i].x * 3/4;
	 if (prop_dlg[i].w > 100)
	    prop_dlg[i].w = prop_dlg[i].w * 2/3;
	 else if (prop_dlg[i].w > 50)
	    prop_dlg[i].w = prop_dlg[i].w * 3/4;
      }
   }

   if (SCREEN_H < 400) {
      griddle_dlg[0].h = griddle_dlg[0].h * 2/3;
      for (i=1; griddle_dlg[i].proc; i++) {
	 griddle_dlg[i].y = griddle_dlg[i].y * 2/3;
	 if (griddle_dlg[i].h > 32)
	    griddle_dlg[i].h -= 8;
      }
   }

   main_dlg[0].w = SCREEN_W;
   main_dlg[0].h = SCREEN_H;

   if (SCREEN_W < 640) {
      for (i=DLG_FILENAME-1; i<DLG_PASSWORD; i+=2)
	 main_dlg[i].x = main_dlg[i].x * SCREEN_W / 640;

      main_dlg[DLG_FILENAME].w = SCREEN_W / 3;
      main_dlg[DLG_HEADERNAME].w = SCREEN_W / 3;
      main_dlg[DLG_PREFIXSTRING].w = SCREEN_W / 3;
      main_dlg[DLG_PASSWORD].w = SCREEN_W / 3;

      main_dlg[DLG_XGRIDSTRING-1].dp = "X:";
      main_dlg[DLG_XGRIDSTRING-1].x -= 16;
      main_dlg[DLG_XGRIDSTRING].x -= 56;

      main_dlg[DLG_YGRIDSTRING-1].dp = "Y:";
      main_dlg[DLG_YGRIDSTRING-1].x -= 64;
      main_dlg[DLG_YGRIDSTRING].x -= 104;

      main_dlg[DLG_BACKUPCHECK].x = main_dlg[DLG_BACKUPCHECK].x * SCREEN_W / 640;
      main_dlg[DLG_BACKUPCHECK].y += 20;

      main_dlg[DLG_PACKLIST].x = main_dlg[DLG_PACKLIST].x * SCREEN_W / 640;
      main_dlg[DLG_PACKLIST].w = main_dlg[DLG_PACKLIST].w * SCREEN_W / 640;
      main_dlg[DLG_PACKLIST].y += 18;

      for (i=DLG_PROP; main_dlg[i].proc; i++) {
	 main_dlg[i].x = main_dlg[i].x * SCREEN_W / 640;
	 main_dlg[i].w = main_dlg[i].w * SCREEN_W / 640;
      }

      while (main_dlg[DLG_LIST].y + main_dlg[DLG_LIST].h >= SCREEN_H)
	 main_dlg[DLG_LIST].h -= 8;

      if (SCREEN_H < 400) {
	 main_dlg[DLG_PROP].h -= 80;
	 main_dlg[DLG_VIEW].y -= 80;
      }
   }
   else {
      while (main_dlg[DLG_LIST].y + main_dlg[DLG_LIST].h < SCREEN_H)
	 main_dlg[DLG_LIST].h += 8;

      while (main_dlg[DLG_LIST].y + main_dlg[DLG_LIST].h >= SCREEN_H)
	 main_dlg[DLG_LIST].h -= 8;

      for (i=DLG_BACKUPCHECK; main_dlg[i].proc; i++) {
	 if (i != DLG_LIST)
	    main_dlg[i].x = main_dlg[i].x * SCREEN_W / 640;
	 main_dlg[i].w = main_dlg[i].w * SCREEN_W / 640;
      }
   }

   memcpy(current_pallete, desktop_pallete, sizeof(PALLETE));
   current_pallete[0] = black_rgb;
   sel_pallete(current_pallete);
   clear_to_color(screen, mg);
   show_mouse(screen);

   strcpy(password, get_config_string("grabber", "password", ""));

   if (fname)
      load(fname);
   else
      load(NULL);

   if (!fname) {
      sprintf(xgrid_string, "%d", get_config_int("grabber", "xgrid", 16));
      sprintf(ygrid_string, "%d", get_config_int("grabber", "ygrid", 16));

      if (strpbrk(get_config_string("grabber", "backups", ""), "yY1"))
	 main_dlg[DLG_BACKUPCHECK].flags |= D_SELECTED;
      else
	 main_dlg[DLG_BACKUPCHECK].flags &= ~D_SELECTED;
   }

   do_dialog(main_dlg, DLG_LIST);

   if (datafile)
      unload_datafile(datafile);

   if (data)
      free(data);

   if (data_sel)
      free(data_sel);

   set_config_string("grabber", "password", password);
   set_config_string("grabber", "xgrid", xgrid_string);
   set_config_string("grabber", "ygrid", ygrid_string);

   if (main_dlg[DLG_BACKUPCHECK].flags & D_SELECTED)
      set_config_string("grabber", "backups", "y");
   else
      set_config_string("grabber", "backups", "n");

   return 0;
}

