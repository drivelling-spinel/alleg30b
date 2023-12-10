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
 *      The core GUI routines.
 *
 *      See readme.txt for copyright information.
 */


#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>

#include "allegro.h"
#include "internal.h"


/* if set, the input focus follows the mouse pointer */
int gui_mouse_focus = TRUE;


/* colors for the standard dialogs (alerts, file selector, etc) */
int gui_fg_color = 255;
int gui_mg_color = 8;
int gui_bg_color = 0;


/* font alignment value */
int gui_font_baseline = 0;


/* pointer to the currently active dialog and menu objects */
DIALOG *active_dialog = NULL;
MENU *active_menu = NULL;


/* Checking for double clicks is complicated. The user could release the
 * mouse button at almost any point, and I might miss it if I am doing some
 * other processing at the same time (eg. sending the single-click message).
 * To get around this I install a timer routine to do the checking for me,
 * so it will notice double clicks whenever they happen.
 */

static volatile int dclick_status, dclick_time;
static int dclick_install_count = 0;
static int dclick_install_time = 0;

#define DCLICK_START      0
#define DCLICK_RELEASE    1
#define DCLICK_AGAIN      2
#define DCLICK_NOT        3


/* dclick_check:
 *  Double click checking user timer routine.
 */
static void dclick_check()
{
   if (dclick_status==DCLICK_START) {              /* first click... */
      if (!mouse_b) {
	 dclick_status = DCLICK_RELEASE;           /* aah! released first */
	 dclick_time = 0;
	 return;
      }
   }
   else if (dclick_status==DCLICK_RELEASE) {       /* wait for second click */
      if (mouse_b) {
	 dclick_status = DCLICK_AGAIN;             /* yes! the second click */
	 dclick_time = 0;
	 return;
      }
   }
   else
      return;

   /* timeout? */
   if (dclick_time++ > 10)
      dclick_status = DCLICK_NOT;
}

static END_OF_FUNCTION(dclick_check);



/* centre_dialog:
 *  Moves all the objects in a dialog so that the dialog is centered in
 *  the screen.
 */
void centre_dialog(DIALOG *dialog)
{
   int min_x = INT_MAX;
   int min_y = INT_MAX;
   int max_x = INT_MIN;
   int max_y = INT_MIN;
   int xc, yc;
   int c;

   /* find the extents of the dialog */ 
   for (c=0; dialog[c].proc; c++) {
      if (dialog[c].x < min_x)
	 min_x = dialog[c].x;

      if (dialog[c].y < min_y)
	 min_y = dialog[c].y;

      if (dialog[c].x + dialog[c].w > max_x)
	 max_x = dialog[c].x + dialog[c].w;

      if (dialog[c].y + dialog[c].h > max_y)
	 max_y = dialog[c].y + dialog[c].h;
   }

   /* how much to move by? */
   xc = (SCREEN_W - (max_x - min_x)) / 2 - min_x;
   yc = (SCREEN_H - (max_y - min_y)) / 2 - min_y;

   /* move it */
   for (c=0; dialog[c].proc; c++) {
      dialog[c].x += xc;
      dialog[c].y += yc;
   }
}



/* set_dialog_color:
 *  Sets the foreground and background colors of all the objects in a dialog.
 */
void set_dialog_color(DIALOG *dialog, int fg, int bg)
{
   int c;

   for (c=0; dialog[c].proc; c++) {
      dialog[c].fg = fg;
      dialog[c].bg = bg;
   }
}



/* find_dialog_focus:
 *  Searches the dialog for the object which has the input focus, returning
 *  its index, or -1 if the focus is not set. Useful after do_dialog() exits
 *  if you need to know which object was selected.
 */
int find_dialog_focus(DIALOG *dialog)
{
   int c;

   for (c=0; dialog[c].proc; c++)
      if (dialog[c].flags & D_GOTFOCUS)
	 return c;

   return -1;
}



/* dialog_message:
 *  Sends a message to all the objects in a dialog. If any of the objects
 *  return values other than D_O_K, returns the value and sets obj to the 
 *  object which produced it.
 */
int dialog_message(DIALOG *dialog, int msg, int c, int *obj)
{
   int count;
   int res;
   int r;

   if (msg == MSG_DRAW)
      show_mouse(NULL);

   res = D_O_K;

   for (count=0; dialog[count].proc; count++) { 
      if (!(dialog[count].flags & D_HIDDEN)) {
	 r = SEND_MESSAGE(dialog+count, msg, c);
	 if (r != D_O_K) {
	    res |= r;
	    *obj = count;
	 }
      }
   }

   if (msg == MSG_DRAW)
      show_mouse(screen);

   return res;
}



/* broadcast_dialog_message:
 *  Broadcasts a message to all the objects in the active dialog. If any of
 *  the dialog procedures return values other than D_O_K, it returns that
 *  value.
 */
int broadcast_dialog_message(int msg, int c)
{
   int nowhere;

   if (active_dialog)
      return dialog_message(active_dialog, msg, c, &nowhere);
   else
      return D_O_K;
}



/* find_mouse_object:
 *  Finds which object the mouse is on top of.
 */
static int find_mouse_object(DIALOG *d)
{
   /* finds which object the mouse is on top of */

   int mouse_object = -1;
   int c;

   for (c=0; d[c].proc; c++)
      if ((mouse_x >= d[c].x) && (mouse_y >= d[c].y) &&
	  (mouse_x < d[c].x + d[c].w) && (mouse_y < d[c].y + d[c].h) &&
	  (!(d[c].flags & (D_HIDDEN | D_DISABLED))))
	 mouse_object = c;

   return mouse_object;
}



/* offer_focus:
 *  Offers the input focus to a particular object.
 */
static int offer_focus(DIALOG *d, int obj, int *focus_obj, int force)
{
   int res = D_O_K;

   if ((obj == *focus_obj) || 
       ((obj >= 0) && (d[obj].flags & (D_HIDDEN | D_DISABLED))))
      return D_O_K;

   /* check if object wants the focus */
   if (obj >= 0) {
      res = SEND_MESSAGE(d+obj, MSG_WANTFOCUS, 0);
      if (res & D_WANTFOCUS)
	 res ^= D_WANTFOCUS;
      else
	 obj = -1;
   }

   if ((obj >= 0) || (force)) {
      /* take focus away from old object */
      if (*focus_obj >= 0) {
	 res |= SEND_MESSAGE(d+*focus_obj, MSG_LOSTFOCUS, 0);
	 if (res & D_WANTFOCUS) {
	    if (obj < 0)
	       return D_O_K;
	    else
	       res &= ~D_WANTFOCUS;
	 }
	 d[*focus_obj].flags &= ~D_GOTFOCUS;
	 show_mouse(NULL);
	 res |= SEND_MESSAGE(d+*focus_obj, MSG_DRAW, 0);
	 show_mouse(screen);
      }

      *focus_obj = obj;

      /* give focus to new object */
      if (obj >= 0) {
	 show_mouse(NULL);
	 d[obj].flags |= D_GOTFOCUS;
	 res |= SEND_MESSAGE(d+obj, MSG_GOTFOCUS, 0);
	 res |= SEND_MESSAGE(d+obj, MSG_DRAW, 0);
	 show_mouse(screen);
      }
   }

   return res;
}



#define MAX_OBJECTS     512

typedef struct OBJ_LIST
{
   int index;
   int diff;
} OBJ_LIST;



/* obj_list_cmp:
 *  Callback function for qsort().
 */
static int obj_list_cmp(const void *e1, const void *e2)
{
   return (((OBJ_LIST *)e1)->diff - ((OBJ_LIST *)e2)->diff);
}



/* cmp_tab:
 *  Comparison function for tab key movement.
 */
static int cmp_tab(DIALOG *d1, DIALOG *d2)
{
   int ret = (int)d2 - (int)d1;

   if (ret < 0)
      ret += 0x10000;

   return ret;
}



/* cmp_right:
 *  Comparison function for right arrow key movement.
 */
static int cmp_right(DIALOG *d1, DIALOG *d2)
{
   int ret = (d2->x - d1->x) + ABS(d1->y - d2->y) * 8;

   if (d1->x >= d2->x)
      ret += 0x10000;

   return ret;
}



/* cmp_left:
 *  Comparison function for left arrow key movement.
 */
static int cmp_left(DIALOG *d1, DIALOG *d2)
{
   int ret = (d1->x - d2->x) + ABS(d1->y - d2->y) * 8;

   if (d1->x <= d2->x)
      ret += 0x10000;

   return ret;
}



/* cmp_down:
 *  Comparison function for down arrow key movement.
 */
static int cmp_down(DIALOG *d1, DIALOG *d2)
{
   int ret = (d2->y - d1->y) + ABS(d1->x - d2->x) * 8;

   if (d1->y >= d2->y)
      ret += 0x10000;

   return ret;
}



/* cmp_up:
 *  Comparison function for up arrow key movement.
 */
static int cmp_up(DIALOG *d1, DIALOG *d2)
{
   int ret = (d1->y - d2->y) + ABS(d1->x - d2->x) * 8;

   if (d1->y <= d2->y)
      ret += 0x10000;

   return ret;
}



/* move_focus:
 *  Handles arrow key and tab movement through a dialog, deciding which
 *  object should be given the input focus.
 */
static int move_focus(DIALOG *d, long ch, int *focus_obj)
{
   int (*cmp)(DIALOG *d1, DIALOG *d2);
   OBJ_LIST obj[MAX_OBJECTS];
   int obj_count = 0;
   int fobj, c;
   int res = D_O_K;

   /* choose a comparison function */ 
   switch (ch >> 8) {
      case KEY_TAB:     cmp = cmp_tab;    break;
      case KEY_RIGHT:   cmp = cmp_right;  break;
      case KEY_LEFT:    cmp = cmp_left;   break;
      case KEY_DOWN:    cmp = cmp_down;   break;
      case KEY_UP:      cmp = cmp_up;     break;
      default:          return D_O_K;
   }

   /* fill temporary table */
   for (c=0; d[c].proc; c++) {
      if ((*focus_obj < 0) || (c != *focus_obj)) {
	 obj[obj_count].index = c;
	 if (*focus_obj >= 0)
	    obj[obj_count].diff = cmp(d+*focus_obj, d+c);
	 else
	    obj[obj_count].diff = c;
	 obj_count++;
	 if (obj_count >= MAX_OBJECTS)
	    break;
      }
   }

   /* sort table */
   qsort(obj, obj_count, sizeof(OBJ_LIST), obj_list_cmp);

   /* find an object to give the focus to */
   fobj = *focus_obj;
   for (c=0; c<obj_count; c++) {
      res |= offer_focus(d, obj[c].index, focus_obj, FALSE);
      if (fobj != *focus_obj)
	 break;
   } 

   return res;
}



#define MESSAGE(i, msg, c) {                       \
   r = SEND_MESSAGE(player->dialog+i, msg, c);     \
   if (r != D_O_K) {                               \
      player->res |= r;                            \
      player->obj = i;                             \
   }                                               \
}



/* do_dialog:
 *  The basic dialog manager. The list of dialog objects should be
 *  terminated by one with a null dialog procedure. Returns the index of 
 *  the object which caused it to exit.
 */
int do_dialog(DIALOG *dialog, int focus_obj)
{
   void *player = init_dialog(dialog, focus_obj);

   while (update_dialog(player))
      ;

   return shutdown_dialog(player);
}



/* popup_dialog:
 *  Like do_dialog(), but it stores the data on the screen before drawing
 *  the dialog and restores it when the dialog is closed. The screen area
 *  to be stored is calculated from the dimensions of the first object in
 *  the dialog, so all the other objects should lie within this one.
 */
int popup_dialog(DIALOG *dialog, int focus_obj)
{
   BITMAP *bmp;
   int ret;
   int mouse_visible = (_mouse_screen == screen);

   bmp = create_bitmap(dialog->w+1, dialog->h+1); 

   if (bmp) {
      show_mouse(NULL);
      blit(screen, bmp, dialog->x, dialog->y, 0, 0, dialog->w+1, dialog->h+1);
   }
   else
      errno = ENOMEM;

   ret = do_dialog(dialog, focus_obj);

   if (bmp) {
      show_mouse(NULL);
      blit(bmp, screen, 0, 0, dialog->x, dialog->y, dialog->w+1, dialog->h+1);
      destroy_bitmap(bmp);
   }

   show_mouse(mouse_visible ? screen : NULL);

   return ret;
}



/* init_dialog:
 *  Sets up a dialog, returning a player object that can be used with
 *  the update_dialog() and shutdown_dialog() functions.
 */
DIALOG_PLAYER *init_dialog(DIALOG *dialog, int focus_obj)
{
   DIALOG_PLAYER *player = malloc(sizeof(DIALOG_PLAYER));
   int c;

   player->res = D_REDRAW;
   player->joy_on = TRUE;
   player->click_wait = FALSE;
   player->mouse_visible = (_mouse_screen == screen);
   player->dialog = dialog;

   /* set up the global  dialog pointer */
   player->previous_dialog = active_dialog;
   active_dialog = dialog;

   /* set up dclick checking code */
   if (dclick_install_count <= 0) {
      LOCK_VARIABLE(dclick_status);
      LOCK_VARIABLE(dclick_time);
      LOCK_FUNCTION(dclick_check);
      install_int(dclick_check, 20);
      dclick_install_count = 1;
      dclick_install_time = _allegro_count;
   }
   else
      dclick_install_count++;

   /* initialise the dialog */
   set_clip(screen, 0, 0, SCREEN_W-1, SCREEN_H-1);
   player->res |= dialog_message(dialog, MSG_START, 0, &player->obj);

   player->mouse_obj = find_mouse_object(dialog);
   if (player->mouse_obj >= 0)
      dialog[player->mouse_obj].flags |= D_GOTMOUSE;

   for (c=0; dialog[c].proc; c++)
      dialog[c].flags &= ~D_GOTFOCUS;

   if (focus_obj >= 0)
      c = focus_obj;
   else
      c = player->mouse_obj;

   if ((c >= 0) && ((SEND_MESSAGE(dialog+c, MSG_WANTFOCUS, 0)) & D_WANTFOCUS)) {
      dialog[c].flags |= D_GOTFOCUS;
      player->focus_obj = c;
   }
   else
      player->focus_obj = -1;

   return player;
}



/* update_dialog:
 *  Updates the status of a dialog object returned by init_dialog(),
 *  returning TRUE if it is still active or FALSE if it has finished.
 */
int update_dialog(DIALOG_PLAYER *player)
{
   int c, ch, r, ret, nowhere;

   if (player->res & D_CLOSE)
      return FALSE;

   /* need to reinstall the dclick handler? */
   if (dclick_install_time != _allegro_count) {
      install_int(dclick_check, 20);
      dclick_install_time = _allegro_count;
   }

   /* are we dealing with a mouse click? */
   if (player->click_wait) {
      if ((ABS(player->mouse_ox-mouse_x) > 8) || 
	  (ABS(player->mouse_oy-mouse_y) > 8))
	 dclick_status = DCLICK_NOT;

      /* waiting... */
      if ((dclick_status != DCLICK_AGAIN) && (dclick_status != DCLICK_NOT)) {
	 dialog_message(player->dialog, MSG_IDLE, 0, &nowhere);
	 return TRUE;
      }

      player->click_wait = FALSE;

      /* double click! */
      if ((dclick_status==DCLICK_AGAIN) &&
	  (mouse_x >= player->dialog[player->mouse_obj].x) && 
	  (mouse_y >= player->dialog[player->mouse_obj].y) &&
	  (mouse_x <= player->dialog[player->mouse_obj].x + player->dialog[player->mouse_obj].w) &&
	  (mouse_y <= player->dialog[player->mouse_obj].y + player->dialog[player->mouse_obj].h)) {
	 MESSAGE(player->mouse_obj, MSG_DCLICK, 0);
      }

      goto getout;
   }

   player->res &= ~D_USED_CHAR;

   /* need to draw it? */
   if (player->res & D_REDRAW) {
      player->res ^= D_REDRAW;
      player->res |= dialog_message(player->dialog, MSG_DRAW, 0, &player->obj);
   }

   /* need to give the input focus to someone? */
   if (player->res & D_WANTFOCUS) {
      player->res ^= D_WANTFOCUS;
      player->res |= offer_focus(player->dialog, player->obj, &player->focus_obj, FALSE);
   }

   /* has mouse object changed? */
   c = find_mouse_object(player->dialog);
   if (c != player->mouse_obj) {
      if (player->mouse_obj >= 0) {
	 player->dialog[player->mouse_obj].flags &= ~D_GOTMOUSE;
	 MESSAGE(player->mouse_obj, MSG_LOSTMOUSE, 0);
      }
      if (c >= 0) {
	 player->dialog[c].flags |= D_GOTMOUSE;
	 MESSAGE(c, MSG_GOTMOUSE, 0);
      }
      player->mouse_obj = c; 

      /* move the input focus as well? */
      if ((gui_mouse_focus) && (player->mouse_obj != player->focus_obj))
	 player->res |= offer_focus(player->dialog, player->mouse_obj, &player->focus_obj, TRUE);
   }

   /* deal with mouse button clicks */
   if (mouse_b) {
      player->res |= offer_focus(player->dialog, player->mouse_obj, &player->focus_obj, FALSE);

      if (player->mouse_obj >= 0) {
	 dclick_time = 0;
	 dclick_status = DCLICK_START;
	 player->mouse_ox = mouse_x;
	 player->mouse_oy = mouse_y;

	 /* send click message */
	 MESSAGE(player->mouse_obj, MSG_CLICK, mouse_b);

	 if (player->res==D_O_K)
	    player->click_wait = TRUE;
      }
      else
	 dialog_message(player->dialog, MSG_IDLE, 0, &nowhere);

      goto getout;
   }

   /* fake joystick input by converting it to key presses */
   if (player->joy_on)
      rest(20);

   poll_joystick();

   if (player->joy_on) {
      if ((!joy_left) && (!joy_right) && (!joy_up) && (!joy_down) &&
	  (!joy_b1) && (!joy_b2)) {
	 player->joy_on = FALSE;
	 rest(20);
      }
      ch = 0;
   }
   else {
      if (joy_left) {
	 ch = KEY_LEFT << 8;
	 player->joy_on = TRUE;
      }
      else if (joy_right) {
	 ch = KEY_RIGHT << 8;
	 player->joy_on = TRUE;
      }
      else if (joy_up) {
	 ch = KEY_UP << 8;
	 player->joy_on = TRUE;
      }
      else if (joy_down) {
	 ch = KEY_DOWN << 8;
	 player->joy_on = TRUE;
      }
      else if ((joy_b1) || (joy_b2)) {
	 ch = (KEY_SPACE << 8) + ' ';
	 player->joy_on = TRUE;
      }
      else
	 ch = 0;
   }

   /* deal with keyboard input */
   if ((ch) || (keypressed())) {
      if (!ch)
	 ch = readkey();

      /* let object deal with the key? */
      if (player->focus_obj >= 0) {
	 MESSAGE(player->focus_obj, MSG_CHAR, ch);
	 if (player->res & D_USED_CHAR)
	    goto getout;
      }

      /* keyboard shortcut? */
      if (ch & 0xff) {
	 for (c=0; player->dialog[c].proc; c++) {
	    if (((tolower(player->dialog[c].key) == tolower((ch & 0xff))) ||
		 (((ch & 0xff) == 0) && (player->dialog[c].key == ch))) && 
		(!(player->dialog[c].flags & (D_HIDDEN | D_DISABLED)))) {
	       MESSAGE(c, MSG_KEY, ch);
	       ch = 0;
	       break;
	    }
	 }
	 if (!ch)
	    goto getout;
      }

      /* broadcast in case any other objects want it */
      for (c=0; player->dialog[c].proc; c++) {
	 if (!(player->dialog[c].flags & (D_HIDDEN | D_DISABLED))) {
	    MESSAGE(c, MSG_XCHAR, ch);
	    if (player->res & D_USED_CHAR)
	       goto getout;
	 }
      }

      /* pass <CR> or <SPACE> to selected object? */
      if ((((ch & 0xff) == 10) || ((ch & 0xff) == 13) || 
	   ((ch & 0xff) == 32)) && (player->focus_obj >= 0)) {
	 MESSAGE(player->focus_obj, MSG_KEY, ch);
	 goto getout;
      }

      /* ESC closes dialog? */
      if ((ch & 0xff) == 27) {
	 player->res |= D_CLOSE;
	 player->obj = -1;
	 goto getout;
      }

      /* move focus around the dialog? */
      player->res |= move_focus(player->dialog, ch, &player->focus_obj);
   }

   /* send idle messages */
   player->res |= dialog_message(player->dialog, MSG_IDLE, 0, &player->obj);

   getout:

   ret = (!(player->res & D_CLOSE));
   player->res &= ~D_CLOSE;
   return ret;
}



/* shutdown_dialog:
 *  Destroys a dialog object returned by init_dialog(), returning the index
 *  of the object that caused it to exit.
 */
int shutdown_dialog(DIALOG_PLAYER *player)
{
   int obj;

   /* send the finish messages */
   dialog_message(player->dialog, MSG_END, 0, &player->obj);

   /* remove the double click handler */
   dclick_install_count--;
   if (dclick_install_count <= 0)
      remove_int(dclick_check);

   if (player->mouse_obj >= 0)
      player->dialog[player->mouse_obj].flags &= ~D_GOTMOUSE;

   show_mouse(player->mouse_visible ? screen : NULL);
   active_dialog = player->previous_dialog;

   obj = player->obj;

   free(player);

   return obj;
}



typedef struct MENU_INFO            /* information about a popup menu */
{
   MENU *menu;                      /* the menu itself */
   struct MENU_INFO *parent;        /* the parent menu, or NULL for root */
   int bar;                         /* set if it is a top level menu bar */
   int size;                        /* number of items in the menu */
   int sel;                         /* selected item */
   int x, y, w, h;                  /* screen position of the menu */
   int (*proc)();                   /* callback function */
   BITMAP *saved;                   /* saved what was underneath it */
} MENU_INFO;



/* get_menu_pos:
 *  Calculates the coordinates of an object within a top level menu bar.
 */
static void get_menu_pos(MENU_INFO *m, int c, int *x, int *y, int *w)
{
   int c2;

   if (m->bar) {
      *x = m->x+1;

      for (c2=0; c2<c; c2++)
	 *x += gui_strlen(m->menu[c2].text) + 16;

      *y = m->y+1;
      *w = gui_strlen(m->menu[c].text) + 16;
   }
   else {
      *x = m->x+1;
      *y = m->y+c*(text_height(font)+4)+1;
      *w = m->w-2;
   }
}



/* draw_menu_item:
 *  Draws an item from a popup menu onto the screen.
 */
static void draw_menu_item(MENU_INFO *m, int c)
{
   int fg, bg;
   int i, x, y, w;
   char buf[80], *tok;

   if (m->menu[c].flags & D_DISABLED) {
      if (c == m->sel) {
	 fg = gui_mg_color;
	 bg = gui_fg_color;
      }
      else {
	 fg = gui_mg_color;
	 bg = gui_bg_color;
      } 
   }
   else {
      if (c == m->sel) {
	 fg = gui_bg_color;
	 bg = gui_fg_color;
      }
      else {
	 fg = gui_fg_color;
	 bg = gui_bg_color;
      } 
   }

   get_menu_pos(m, c, &x, &y, &w);

   rectfill(screen, x, y, x+w-1, y+text_height(font)+3, bg);
   text_mode(bg);

   if (m->menu[c].text[0]) {
      for (i=0; (m->menu[c].text[i]) && (m->menu[c].text[i] != '\t'); i++)
	 buf[i] = m->menu[c].text[i];
      buf[i] = 0;

      gui_textout(screen, buf, x+8, y+1, fg, FALSE);

      if (m->menu[c].text[i] == '\t') {
	 tok = m->menu[c].text+i+1;
	 gui_textout(screen, tok, x+w-gui_strlen(tok)-8, y+1, fg, FALSE);
      }
   }
   else
      hline(screen, x, y+text_height(font)/2+2, x+w, fg);

   if (m->menu[c].flags & D_SELECTED) {
      line(screen, x+1, y+text_height(font)/2+1, x+3, y+text_height(font)+1, fg);
      line(screen, x+3, y+text_height(font)+1, x+6, y+2, fg);
   }
}



/* draw_menu:
 *  Draws a popup menu onto the screen.
 */
static void draw_menu(MENU_INFO *m)
{
   int c;

   rect(screen, m->x, m->y, m->x+m->w-1, m->y+m->h-1, gui_fg_color);
   vline(screen, m->x+m->w, m->y+1, m->y+m->h, gui_fg_color);
   hline(screen, m->x+1, m->y+m->h, m->x+m->w, gui_fg_color);

   for (c=0; m->menu[c].text; c++)
      draw_menu_item(m, c);
}



/* menu_mouse_object:
 *  Returns the index of the object the mouse is currently on top of.
 */
static int menu_mouse_object(MENU_INFO *m)
{
   int c;
   int x, y, w;

   for (c=0; c<m->size; c++) {
      get_menu_pos(m, c, &x, &y, &w);

      if ((mouse_x >= x) && (mouse_x < x+w) &&
	  (mouse_y >= y) && (mouse_y < y+(text_height(font)+4)))
	 return (m->menu[c].text[0]) ? c : -1;
   }

   return -1;
}



/* mouse_in_parent_menu:
 *  Recursively checks if the mouse is inside a menu or any of its parents.
 */
static int mouse_in_parent_menu(MENU_INFO *m) 
{
   int c;

   if (!m)
      return FALSE;

   c = menu_mouse_object(m);
   if ((c >= 0) && (c != m->sel))
      return TRUE;

   return mouse_in_parent_menu(m->parent);
}



/* fill_menu_info:
 *  Fills a menu info structure when initialising a menu.
 */
static void fill_menu_info(MENU_INFO *m, MENU *menu, MENU_INFO *parent, int bar, int x, int y, int minw, int minh)
{
   int c, i;
   int extra = 0;
   char buf[80], *tok;

   m->menu = menu;
   m->parent = parent;
   m->bar = bar;
   m->x = x;
   m->y = y;
   m->w = 2;
   m->h = (m->bar) ? (text_height(font)+6) : 2;
   m->proc = NULL;
   m->sel = -1;

   /* calculate size of the menu */
   for (m->size=0; m->menu[m->size].text; m->size++) {
      for (i=0; (m->menu[m->size].text[i]) && (m->menu[m->size].text[i] != '\t'); i++)
	 buf[i] = m->menu[m->size].text[i];
      buf[i] = 0;

      c = gui_strlen(buf);

      if (m->bar) {
	 m->w += c+16;
      }
      else {
	 m->h += text_height(font)+4;
	 m->w = MAX(m->w, c+16);
      }

      if (m->menu[m->size].text[i] == '\t') {
	 tok = m->menu[m->size].text+i+1;
	 c = gui_strlen(tok);
	 extra = MAX(extra, c);
      }
   }

   if (extra)
      m->w += extra+16;

   m->w = MAX(m->w, minw);
   m->h = MAX(m->h, minh);
}



/* menu_key_shortcut:
 *  Returns true if c is indicated as a keyboard shortcut by a '&' character
 *  in the specified string.
 */
static int menu_key_shortcut(int c, char *s)
{
   while (*s) {
      if (*s == '&') {
	 s++;
	 if ((*s != '&') && (tolower(*s) == tolower(c & 0xff)))
	    return TRUE;
      }
      s++;
   }

   return FALSE;
}



/* menu_alt_key:
 *  Searches a menu for keyboard shortcuts, for the alt+letter to bring
 *  up a menu.
 */
int menu_alt_key(int k, MENU *m)
{
   char *s;
   int c;

   if (k & 0xff)
      return 0;

   k = key_ascii_table[k>>8];

   for (c=0; m[c].text; c++) {
      s = m[c].text;
      while (*s) {
	 if (*s == '&') {
	    s++;
	    if ((*s != '&') && (tolower(*s) == tolower(k)))
	       return k;
	 }
	 s++;
      }
   }

   return 0;
}



/* _do_menu:
 *  The core menu control function, called by do_menu() and d_menu_proc().
 */
int _do_menu(MENU *menu, MENU_INFO *parent, int bar, int x, int y, int repos, int *dret, int minw, int minh)
{
   MENU_INFO m;
   MENU_INFO *i;
   int c, c2;
   int ret = -1;
   int mouse_on = mouse_b;
   int old_sel;
   int mouse_sel;
   int _x, _y;
   int redraw = TRUE;
   int mouse_visible = (_mouse_screen == screen);

   show_mouse(NULL);

   fill_menu_info(&m, menu, parent, bar, x, y, minw, minh);

   if (repos) {
      m.x = MID(0, m.x, SCREEN_W-m.w-1);
      m.y = MID(0, m.y, SCREEN_H-m.h-1);
   }

   /* save screen under the menu */
   m.saved = create_bitmap(m.w+1, m.h+1); 

   if (m.saved)
      blit(screen, m.saved, m.x, m.y, 0, 0, m.w+1, m.h+1);
   else
      errno = ENOMEM;

   m.sel = mouse_sel = menu_mouse_object(&m);
   if ((m.sel < 0) && (!mouse_b) && (!bar))
      m.sel = 0;

   show_mouse(screen);

   do {
      old_sel = m.sel;

      c = menu_mouse_object(&m);
      if ((mouse_b) || (c != mouse_sel))
	 m.sel = mouse_sel = c;

      if (mouse_b) {                                  /* if button pressed */
	 if ((mouse_x < m.x) || (mouse_x > m.x+m.w) ||
	     (mouse_y < m.y) || (mouse_y > m.y+m.h)) {
	    if (!mouse_on)                            /* dismiss menu? */
	       break;

	    if (mouse_in_parent_menu(m.parent))       /* back to parent? */
	       break;
	 }

	 if ((m.sel >= 0) && (m.menu[m.sel].child))   /* bring up child? */
	    ret = m.sel;

	 mouse_on = TRUE;
	 clear_keybuf();
      }
      else {                                          /* button not pressed */
	 if (mouse_on)                                /* selected an item? */
	    ret = m.sel;

	 mouse_on = FALSE;

	 if (keypressed()) {                          /* keyboard input */
	    c = readkey();

	    if ((c & 0xff) == 27) {
	       ret = -1;
	       goto getout;
	    }

	    switch (c >> 8) {

	       case KEY_LEFT:
		  if (m.parent) {
		     if (m.parent->bar) {
			simulate_keypress(KEY_LEFT<<8);
			simulate_keypress(KEY_DOWN<<8);
		     }
		     ret = -1;
		     goto getout;
		  }
		  /* fall through */

	       case KEY_UP:
		  if ((((c >> 8) == KEY_LEFT) && (m.bar)) ||
		      (((c >> 8) == KEY_UP) && (!m.bar))) {
		     c = m.sel;
		     do {
			c--;
			if (c < 0)
			   c = m.size - 1;
		     } while ((!(m.menu[c].text[0])) && (c != m.sel));
		     m.sel = c;
		  }
		  break;

	       case KEY_RIGHT:
		  if (((m.sel < 0) || (!m.menu[m.sel].child)) &&
		      (m.parent) && (m.parent->bar)) {
		     simulate_keypress(KEY_RIGHT<<8);
		     simulate_keypress(KEY_DOWN<<8);
		     ret = -1;
		     goto getout;
		  }
		  /* fall through */

	       case KEY_DOWN:
		  if ((m.sel >= 0) && (m.menu[m.sel].child) &&
		      ((((c >> 8) == KEY_RIGHT) && (!m.bar)) ||
		       (((c >> 8) == KEY_DOWN) && (m.bar)))) {
		     ret = m.sel;
		  }
		  else if ((((c >> 8) == KEY_RIGHT) && (m.bar)) ||
			   (((c >> 8) == KEY_DOWN) && (!m.bar))) {
		     c = m.sel;
		     do {
			c++;
			if (c >= m.size)
			   c = 0;
		     } while ((!(m.menu[c].text[0])) && (c != m.sel));
		     m.sel = c;
		  }
		  break;

	       case KEY_SPACE:
	       case KEY_ENTER:
		  if (m.sel >= 0)
		     ret = m.sel;
		  break;

	       default:
		  if ((!m.parent) && ((c & 0xff) == 0))
		     c = menu_alt_key(c, m.menu);
		  for (c2=0; m.menu[c2].text; c2++) {
		     if (menu_key_shortcut(c, m.menu[c2].text)) {
			ret = m.sel = c2;
			break;
		     }
		  }
		  if (m.parent) {
		     i = m.parent;
		     for (c2=0; i->parent; c2++)
			i = i->parent;
		     c = menu_alt_key(c, i->menu);
		     if (c) {
			while (c2-- > 0)
			   simulate_keypress(27);
			simulate_keypress(c);
			ret = -1;
			goto getout;
		     }
		  }
		  break;
	    }
	 }
      }

      if ((redraw) || (m.sel != old_sel)) {           /* selection changed? */
	 show_mouse(NULL);

	 if (redraw) {
	    draw_menu(&m);
	    redraw = FALSE;
	 }
	 else {
	    if (old_sel >= 0)
	       draw_menu_item(&m, old_sel);

	    if (m.sel >= 0)
	       draw_menu_item(&m, m.sel);
	 }

	 show_mouse(screen);
      }

      if ((ret >= 0) && (m.menu[ret].flags & D_DISABLED))
	 ret = -1;

      if (ret >= 0) {                                 /* child menu? */
	 if (m.menu[ret].child) {
	    if (m.bar) {
	       get_menu_pos(&m, ret, &_x, &_y, &c);
	       _x += 6;
	       _y += text_height(font)+7;
	    }
	    else {
	       _x = m.x+m.w*2/3;
	       _y = m.y + (text_height(font)+4)*ret + text_height(font)/4+2;
	    }
	    c = _do_menu(m.menu[ret].child, &m, FALSE, _x, _y, TRUE, NULL, 0, 0);
	    if (c < 0) {
	       ret = -1;
	       mouse_on = FALSE;
	       mouse_sel = menu_mouse_object(&m);
	    }
	 }
      }

      if ((m.bar) && (!mouse_b) && (!keypressed()) &&
	  ((mouse_x < m.x) || (mouse_x > m.x+m.w) ||
	   (mouse_y < m.y) || (mouse_y > m.y+m.h)))
	 break;

   } while (ret < 0);

   getout:

   if (dret)
      *dret = 0;

   /* callback function? */
   if ((!m.proc) && (ret >= 0)) {
      active_menu = &m.menu[ret];
      m.proc = active_menu->proc;
   }

   if (ret >= 0) {
      if (parent)
	 parent->proc = m.proc;
      else  {
	 if (m.proc) {
	    c = m.proc();
	    if (dret)
	       *dret = c;
	 }
      }
   }

   /* restore screen */
   if (m.saved) {
      show_mouse(NULL);
      blit(m.saved, screen, 0, 0, m.x, m.y, m.w+1, m.h+1);
      destroy_bitmap(m.saved);
   }

   show_mouse(mouse_visible ? screen : NULL);

   return ret;
}



/* do_menu:
 *  Displays and animates a popup menu at the specified screen position,
 *  returning the index of the item that was selected, or -1 if it was
 *  dismissed. If the menu crosses the edge of the screen it will be moved.
 */
int do_menu(MENU *menu, int x, int y)
{
   int ret = _do_menu(menu, NULL, FALSE, x, y, TRUE, NULL, 0, 0);

   do {
   } while (mouse_b);

   return ret;
}



/* d_menu_proc:
 *  Dialog procedure for adding drop down menus to a GUI dialog. This 
 *  displays the top level menu items as a horizontal bar (eg. across the
 *  top of the screen), and pops up child menus when they are clicked.
 *  When it executes one of the menu callback routines, it passes the
 *  return value back to the dialog manager, so these can return D_O_K,
 *  D_CLOSE, D_REDRAW, etc.
 */
int d_menu_proc(int msg, DIALOG *d, int c)
{ 
   MENU_INFO m;
   int ret = D_O_K;
   int x;

   switch (msg) {

      case MSG_START:
	 fill_menu_info(&m, d->dp, NULL, TRUE, d->x-1, d->y-1, d->w+2, d->h+2);
	 d->w = m.w-2;
	 d->h = m.h-2;
	 break;

      case MSG_DRAW:
	 fill_menu_info(&m, d->dp, NULL, TRUE, d->x-1, d->y-1, d->w+2, d->h+2);
	 draw_menu(&m);
	 break;

      case MSG_XCHAR:
	 x = menu_alt_key(c, d->dp);
	 if (!x)
	    break;

	 ret |= D_USED_CHAR;
	 simulate_keypress(x);
	 /* fall through */

      case MSG_GOTMOUSE:
      case MSG_CLICK:
	 _do_menu(d->dp, NULL, TRUE, d->x-1, d->y-1, FALSE, &x, d->w+2, d->h+2);
	 ret |= x;
	 do {
	 } while (mouse_b);
	 break;
   }

   return ret;
}



static DIALOG alert_dialog[] =
{
   /* (dialog proc)     (x)   (y)   (w)   (h)   (fg)  (bg)  (key) (flags)  (d1)  (d2)  (dp) */
   { d_shadow_box_proc, 0,    0,    0,    0,    0,    0,    0,    0,       0,    0,    NULL },
   { d_ctext_proc,      0,    0,    0,    0,    0,    0,    0,    0,       0,    0,    NULL },
   { d_ctext_proc,      0,    0,    0,    0,    0,    0,    0,    0,       0,    0,    NULL },
   { d_ctext_proc,      0,    0,    0,    0,    0,    0,    0,    0,       0,    0,    NULL },
   { d_button_proc,     0,    0,    0,    0,    0,    0,    0,    D_EXIT,  0,    0,    NULL },
   { d_button_proc,     0,    0,    0,    0,    0,    0,    0,    D_EXIT,  0,    0,    NULL },
   { d_button_proc,     0,    0,    0,    0,    0,    0,    0,    D_EXIT,  0,    0,    NULL },
   { NULL }
};


#define A_S1  1
#define A_S2  2
#define A_S3  3
#define A_B1  4
#define A_B2  5
#define A_B3  6



/* alert3:
 *  Displays a simple alert box, containing three lines of text (s1-s3),
 *  and with either one, two, or three buttons. The text for these buttons 
 *  is passed in b1, b2, and b3 (NULL for buttons which are not used), and
 *  the keyboard shortcuts in c1 and c2. Returns 1, 2, or 3 depending on 
 *  which button was selected.
 */
int alert3(char *s1, char *s2, char *s3, char *b1, char *b2, char *b3, int c1, int c2, int c3)
{
   int maxlen = 0;
   int len1, len2, len3;
   int avg_w = text_length(font, " ");
   int avg_h = text_height(font);
   int buttons = 0;
   int b[3];
   int c;

   #define SORT_OUT_BUTTON(x) {                                            \
      if (b##x) {                                                          \
	 alert_dialog[A_B##x].flags &= ~D_HIDDEN;                          \
	 alert_dialog[A_B##x].key = c##x;                                  \
	 alert_dialog[A_B##x].dp = b##x;                                   \
	 len##x = gui_strlen(b##x);                                        \
	 b[buttons++] = A_B##x;                                            \
      }                                                                    \
      else {                                                               \
	 alert_dialog[A_B##x].flags |= D_HIDDEN;                           \
	 len##x = 0;                                                       \
      }                                                                    \
   }

   alert_dialog[A_S1].dp = alert_dialog[A_S2].dp = alert_dialog[A_S3].dp = 
			   alert_dialog[A_B1].dp = alert_dialog[A_B2].dp = "";

   if (s1) {
      alert_dialog[A_S1].dp = s1;
      maxlen = text_length(font, s1);
   }

   if (s2) {
      alert_dialog[A_S2].dp = s2;
      len1 = text_length(font, s2);
      if (len1 > maxlen)
	 maxlen = len1;
   }

   if (s3) {
      alert_dialog[A_S3].dp = s3;
      len1 = text_length(font, s3);
      if (len1 > maxlen)
	 maxlen = len1;
   }

   SORT_OUT_BUTTON(1);
   SORT_OUT_BUTTON(2);
   SORT_OUT_BUTTON(3);

   len1 = MAX(len1, MAX(len2, len3)) + avg_w*3;
   if (len1*buttons > maxlen)
      maxlen = len1*buttons;

   maxlen += avg_w*4;
   alert_dialog[0].w = maxlen;
   alert_dialog[A_S1].x = alert_dialog[A_S2].x = alert_dialog[A_S3].x = 
						alert_dialog[0].x + maxlen/2;

   alert_dialog[A_B1].w = alert_dialog[A_B2].w = alert_dialog[A_B3].w = len1;

   alert_dialog[A_B1].x = alert_dialog[A_B2].x = alert_dialog[A_B3].x = 
				       alert_dialog[0].x + maxlen/2 - len1/2;

   if (buttons == 3) {
      alert_dialog[b[0]].x = alert_dialog[0].x + maxlen/2 - len1*3/2 - avg_w;
      alert_dialog[b[2]].x = alert_dialog[0].x + maxlen/2 + len1/2 + avg_w;
   }
   else if (buttons == 2) {
      alert_dialog[b[0]].x = alert_dialog[0].x + maxlen/2 - len1 - avg_w;
      alert_dialog[b[1]].x = alert_dialog[0].x + maxlen/2 + avg_w;
   }

   alert_dialog[0].h = avg_h*8;
   alert_dialog[A_S1].y = alert_dialog[0].y + avg_h;
   alert_dialog[A_S2].y = alert_dialog[0].y + avg_h*2;
   alert_dialog[A_S3].y = alert_dialog[0].y + avg_h*3;
   alert_dialog[A_S1].h = alert_dialog[A_S2].h = alert_dialog[A_S2].h = avg_h;

   alert_dialog[A_B1].y = alert_dialog[A_B2].y = alert_dialog[A_B3].y = 
						alert_dialog[0].y + avg_h*5;

   alert_dialog[A_B1].h = alert_dialog[A_B2].h = alert_dialog[A_B3].h = avg_h*2;

   centre_dialog(alert_dialog);
   set_dialog_color(alert_dialog, gui_fg_color, gui_bg_color);

   clear_keybuf();

   do {
   } while (mouse_b);

   c = popup_dialog(alert_dialog, A_B1);

   if (c == A_B1)
      return 1;
   else if (c == A_B2)
      return 2;
   else
      return 3;
}



/* alert:
 *  Displays a simple alert box, containing three lines of text (s1-s3),
 *  and with either one or two buttons. The text for these buttons is passed
 *  in b1 and b2 (b2 may be null), and the keyboard shortcuts in c1 and c2.
 *  Returns 1 or 2 depending on which button was selected.
 */
int alert(char *s1, char *s2, char *s3, char *b1, char *b2, int c1, int c2)
{
   int ret;

   ret = alert3(s1, s2, s3, b1, b2, NULL, c1, c2, 0);

   if (ret > 2)
      ret = 2;

   return ret;
}

