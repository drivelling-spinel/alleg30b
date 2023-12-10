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
 *      Linux mouse routines.
 *
 *      See readme.txt for copyright information.
 */


#ifndef LINUX
#error This file should only be used by the linux version of Allegro
#endif

#include <stdlib.h>

#include "allegro.h"
#include "internal.h"


volatile int mouse_x = 0;                    /* mouse x pos */
volatile int mouse_y = 0;                    /* mouse y pos */
volatile int mouse_b = 0;                    /* mouse button state */

static int mouse_installed = FALSE;

static int mouse_x_focus = 1;                /* focus point in mouse sprite */
static int mouse_y_focus = 1;


static unsigned char mouse_pointer_data[256] =
{
   16,  16,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 
   16,  255, 16,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 
   16,  255, 255, 16,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 
   16,  255, 255, 255, 16,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 
   16,  255, 255, 255, 255, 16,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 
   16,  255, 255, 255, 255, 255, 16,  0,   0,   0,   0,   0,   0,   0,   0,   0, 
   16,  255, 255, 255, 255, 255, 255, 16,  0,   0,   0,   0,   0,   0,   0,   0, 
   16,  255, 255, 255, 255, 255, 255, 255, 16,  0,   0,   0,   0,   0,   0,   0, 
   16,  255, 255, 255, 255, 255, 255, 255, 255, 16,  0,   0,   0,   0,   0,   0, 
   16,  255, 255, 255, 255, 255, 16,  16,  16,  0,   0,   0,   0,   0,   0,   0, 
   16,  255, 255, 16,  255, 255, 16,  0,   0,   0,   0,   0,   0,   0,   0,   0, 
   16,  255, 16,  0,   16,  255, 255, 16,  0,   0,   0,   0,   0,   0,   0,   0, 
   0,   16,  0,   0,   16,  255, 255, 16,  0,   0,   0,   0,   0,   0,   0,   0, 
   0,   0,   0,   0,   0,   16,  255, 255, 16,  0,   0,   0,   0,   0,   0,   0, 
   0,   0,   0,   0,   0,   16,  255, 255, 16,  0,   0,   0,   0,   0,   0,   0, 
   0,   0,   0,   0,   0,   0,   16,  16,  0,   0,   0,   0,   0,   0,   0,   0
};

static BITMAP *mouse_pointer = NULL;         /* default mouse pointer */

static BITMAP *mouse_sprite = NULL;          /* mouse pointer */

BITMAP *_mouse_screen = NULL;                /* where to draw the pointer */

static int mx, my;                           /* previous mouse position */
static BITMAP *ms = NULL;                    /* previous screen data */
static BITMAP *mtemp = NULL;                 /* double-buffer drawing area */

static int mwidth, mheight;                  /* size of mouse sprite */



/* draw_mouse_doublebuffer:
 *  Eliminates mouse-cursor flicker by using an off-screen buffer for
 *  updating the cursor, and blitting only the final screen image.
 *  newx and newy contain the new cursor position, and mx and my are 
 *  assumed to contain previous cursor pos. This routine is called if 
 *  mouse cursor is to be erased and redrawn, and the two position overlap.
 */
static void draw_mouse_doublebuffer(int newx, int newy) 
{
   int x1, y1, w, h;

   /* grab bit of screen containing where we are and where we'll be */
   x1 = MIN(mx, newx);
   x1 -= mouse_x_focus;
   y1 = MIN(my, newy);
   y1 -= mouse_y_focus;

   /* get width of area */
   w = MAX(mx,newx) - x1 + mouse_sprite->w+1;
   h = MAX(my,newy) - y1 + mouse_sprite->h+1;

   /* make new co-ords relative to 'mtemp' bitmap co-ords */
   newx -= mouse_x_focus+x1;
   newy -= mouse_y_focus+y1;

   /* save screen image in 'mtemp' */
   blit(_mouse_screen, mtemp, x1, y1, 0, 0, w, h);

   /* blit saved image in 'ms' to corect place in this buffer */
   blit(ms, mtemp, 0, 0, mx-mouse_x_focus-x1, my-mouse_y_focus-y1,
	 mouse_sprite->w, mouse_sprite->h);

   /* draw mouse at correct place in 'mtemp' */
   blit(mtemp, ms, newx, newy, 0, 0, mouse_sprite->w, mouse_sprite->h);
   draw_sprite(mtemp, mouse_sprite, newx, newy);

   /* blit 'mtemp' to screen */
   blit(mtemp, _mouse_screen, 0, 0, x1, y1, w, h);
}



/* draw_mouse:
 *  Mouse pointer drawing routine. If remove is set, deletes the old mouse
 *  pointer. If add is set, draws a new one.
 */
static void draw_mouse(int remove, int add)
{
   int normal_draw = (remove ^ add); 

   int newmx = mouse_x;
   int newmy = mouse_y;

   int cf = _mouse_screen->clip;
   int cl = _mouse_screen->cl;
   int cr = _mouse_screen->cr;
   int ct = _mouse_screen->ct;
   int cb = _mouse_screen->cb;

   _mouse_screen->clip = TRUE;
   _mouse_screen->cl = _mouse_screen->ct = 0;
   _mouse_screen->cr = _mouse_screen->w;
   _mouse_screen->cb = _mouse_screen->h;

   if (!normal_draw) {
      if ((newmx <= mx-mwidth) || (newmx >= mx+mwidth) ||
	  (newmy <= my-mheight) || (newmy >= my+mheight))
	 normal_draw = 1;
   }

   if (normal_draw) {
      if (remove)
	 blit(ms, _mouse_screen, 0, 0, mx-mouse_x_focus, my-mouse_y_focus, mouse_sprite->w, mouse_sprite->h);

      if (add) {
	 blit(_mouse_screen, ms, newmx-mouse_x_focus, newmy-mouse_y_focus, 0, 0, mouse_sprite->w, mouse_sprite->h);
	 draw_sprite(_mouse_screen, mouse_sprite, newmx-mouse_x_focus, newmy-mouse_y_focus);
      }
   }
   else 
      draw_mouse_doublebuffer(newmx, newmy);

   mx = newmx;
   my = newmy;

   _mouse_screen->clip = cf;
   _mouse_screen->cl = cl;
   _mouse_screen->cr = cr;
   _mouse_screen->ct = ct;
   _mouse_screen->cb = cb;
}



/* mouse_move:
 *  Do we need to redraw the mouse pointer?
 */
static void mouse_move() 
{
   if ((mx != mouse_x - 1) || (my != mouse_y - 1))
      draw_mouse(TRUE, TRUE);
}



/* set_mouse_sprite:
 *  Sets the sprite to be used for the mouse pointer. If the sprite is
 *  NULL, restores the default arrow.
 */
void set_mouse_sprite(struct BITMAP *sprite)
{
   BITMAP *old_mouse_screen = _mouse_screen;

   if (_mouse_screen)
      show_mouse(NULL);

   if (sprite)
      mouse_sprite = sprite;
   else
      mouse_sprite = mouse_pointer;

   /* make sure the ms bitmap is big enough */
   if ((!ms) || (ms->w < mouse_sprite->w) || (ms->h < mouse_sprite->h)) {
      if (ms) {
	 destroy_bitmap(ms);
	 destroy_bitmap(mtemp);
      }
      ms = create_bitmap(mouse_sprite->w, mouse_sprite->h);
      mtemp = create_bitmap(mouse_sprite->w*2, mouse_sprite->h*2);
   }

   mouse_x_focus = mouse_y_focus = 1;
   mwidth = mouse_sprite->w;
   mheight = mouse_sprite->h;

   if (old_mouse_screen)
      show_mouse(old_mouse_screen);
}



/* set_mouse_sprite_focus:
 *  Sets co-ordinate (x, y) in the sprite to be the mouse location.
 *  Call after set_mouse_sprite(). Doesn't redraw the sprite.
 */
void set_mouse_sprite_focus(int x, int y) 
{
   mouse_x_focus = x;
   mouse_y_focus = y;
}



/* show_mouse:
 *  Tells Allegro to display a mouse pointer. This only works when the timer 
 *  module is active. The mouse pointer will be drawn onto the bitmap bmp, 
 *  which should normally be the hardware screen. To turn off the mouse 
 *  pointer, which you must do before you draw anything onto the screen, call 
 *  show_mouse(NULL). If you forget to turn off the mouse pointer when 
 *  drawing something, the SVGA bank switching code will become confused and 
 *  will produce garbage all over the screen.
 */
void show_mouse(BITMAP *bmp)
{
   if (!mouse_installed)
      return;

   remove_int(mouse_move);

   if (_mouse_screen)
      draw_mouse(TRUE, FALSE);

   _mouse_screen = bmp;

   if (bmp) {
      draw_mouse(FALSE, TRUE);
      install_int(mouse_move, 20);
   }
}



/* position_mouse:
 *  Moves the mouse to screen position x, y. This is safe to call even
 *  when a mouse pointer is being displayed.
 */
void position_mouse(int x, int y)
{
   BITMAP *old_mouse_screen = _mouse_screen;

   if (_mouse_screen)
      show_mouse(NULL);

   /*
   r.x.ax = 4;
   r.x.cx = x * 8;
   r.x.dx = y * 8;
   __dpmi_int(0x33, &r); 
   */

   mouse_x = x;
   mouse_y = y;

   if (old_mouse_screen)
      show_mouse(old_mouse_screen);
}



/* set_mouse_range:
 *  Sets the screen area within which the mouse can move. Pass the top left 
 *  corner and the bottom right corner (inclusive). If you don't call this 
 *  function the range defaults to (0, 0, SCREEN_W-1, SCREEN_H-1).
 */
void set_mouse_range(int x1, int y1, int x2, int y2)
{
   BITMAP *old_mouse_screen = _mouse_screen;

   if (!mouse_installed)
      return;

   if (_mouse_screen)
      show_mouse(NULL);

   /*
   r.x.ax = 7;
   r.x.cx = x1 * 8;
   r.x.dx = x2 * 8;
   __dpmi_int(0x33, &r);*/         /* set horizontal range */

   /*
   r.x.ax = 8;
   r.x.cx = y1 * 8;
   r.x.dx = y2 * 8;
   __dpmi_int(0x33, &r);*/         /* set vertical range */

   /*
   r.x.ax = 3; 
   __dpmi_int(0x33, &r);         check the position 
   mouse_x = r.x.cx / 8;
   mouse_y = r.x.dx / 8;*/

   if (old_mouse_screen)
      show_mouse(old_mouse_screen);
}



/* set_mouse_speed:
 *  Sets the mouse speed. Larger values of xspeed and yspeed represent 
 *  slower mouse movement: the default for both is 2.
 */
void set_mouse_speed(int xspeed, int yspeed)
{
   if (!mouse_installed)
      return;

   /*
   r.x.ax = 15;
   r.x.cx = xspeed;
   r.x.dx = yspeed;
   __dpmi_int(0x33, &r); 
   */
}



/* _set_mouse_range:
 *  Called when the graphics mode changes, to alter the mouse range.
 */
void _set_mouse_range()
{
   if (!mouse_installed)
      return;

   if (!gfx_driver)
      return;

   set_mouse_range(0, 0, SCREEN_W-1, SCREEN_H-1);
   set_mouse_speed(2, 2);
   position_mouse(SCREEN_W/2, SCREEN_H/2);
}



/* install_mouse:
 *  Installs the Allegro mouse handler. You must do this before using any
 *  other mouse functions. Return -1 if it can't find a mouse driver,
 *  otherwise the number of buttons on the mouse.
 */
int install_mouse()
{
   int x, num_buttons;

   if (mouse_installed)
      return -1;

   /*
   r.x.ax = 0;
   __dpmi_int(0x33, &r);         initialise mouse driver 

   if (r.x.ax == 0)
      return -1;

   num_buttons = r.x.bx;
   if (num_buttons == 0xffff)
     num_buttons = 2;
   */

   mouse_pointer->w = mouse_pointer->cr = 16;
   mouse_pointer->h = mouse_pointer->cb = 16;
   mouse_pointer->cl = mouse_pointer->ct = 0;
   mouse_pointer->clip = TRUE;
   mouse_pointer->vtable = &_linear_vtable;
   mouse_pointer->read_bank = mouse_pointer->write_bank = _stub_bank_switch;
   mouse_pointer->dat = NULL;
   mouse_pointer->bitmap_id = 0;
   mouse_pointer->line_ofs = 0;
   mouse_pointer->seg = _my_ds();
   for (x=0; x<16; x++)
      mouse_pointer->line[x] = mouse_pointer_data + x*16;

   set_mouse_sprite(mouse_pointer);

   /* dos version installs a mouse driver callback here */

   mouse_installed = TRUE;
   _set_mouse_range();
   _add_exit_func(remove_mouse);

   return num_buttons;
}



/* remove_mouse:
 *  Removes the mouse handler. You don't normally need to call this, because
 *  allegro_exit() will do it for you.
 */
void remove_mouse()
{
   if (!mouse_installed)
      return;

   /* remove callback, etc */

   mouse_x = mouse_y = mouse_b = 0;

   if (mouse_pointer) {
      destroy_bitmap(mouse_pointer);
      mouse_pointer = NULL;
   }

   if (ms) {
      destroy_bitmap(ms);
      ms = NULL;
   }

   _remove_exit_func(remove_mouse);
   mouse_installed = FALSE;
}

