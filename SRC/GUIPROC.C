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
 *      The standard GUI dialog object procedures.
 *
 *      Radio button, icon, and slider objects by Chris La Mantia.
 *
 *      Scrolling d_edit_proc by VolkerOth.
 *
 *      Text box object by Doug Eleveld.
 *
 *      See readme.txt for copyright information.
 */


#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "allegro.h"
#include "internal.h"



/* typedef for the listbox callback functions */
typedef char *(*getfuncptr)(int, int *);



/* gui_textout:
 *  Wrapper function for drawing text to the screen, which interprets the
 *  & character as an underbar for displaying keyboard shortcuts. Returns
 *  the width of the output string in pixels.
 */
int gui_textout(BITMAP *bmp, unsigned char *s, int x, int y, int color, int centre)
{
   char tmp[160];
   int c;
   int len = 0;
   int pix_len;
   int hline_pos = -1;

   for (c=0; (s[c]) && (len<159); c++) {
      if (s[c] == '&') {
	 if (s[c+1] != '&')
	    hline_pos = len;
	 else {
	    tmp[len++] = '&';
	    c++;
	 }
      }
      else
	 tmp[len++] = s[c];
   }

   tmp[len] = 0;
   pix_len = text_length(font, tmp);

   if (centre)
      x -= pix_len / 2;

   if (bmp) {
      textout(bmp, font, tmp, x, y, color);

      if (hline_pos >= 0) {
	 c = tmp[hline_pos];
	 tmp[hline_pos] = 0;
	 hline_pos = text_length(font, tmp);
	 tmp[0] = c;
	 tmp[1] = 0;
	 c = text_length(font, tmp);
	 hline(bmp, x+hline_pos, y+text_height(font)-gui_font_baseline, x+hline_pos+c-1, color);
      }
   }

   return pix_len;
}



/* gui_strlen:
 *  Returns the length of a string in pixels, ignoring '&' characters.
 */
int gui_strlen(unsigned char *s)
{
   return gui_textout(NULL, s, 0, 0, 0, 0);
}



/* dotted_rect:
 *  Draws a dotted rectangle, for showing an object has the input focus.
 */
static void dotted_rect(int x1, int y1, int x2, int y2, int fg, int bg)
{
   int x = ((x1+y1) & 1) ? 1 : 0;
   int c;
/*
   for (c=x1; c<=x2; c++) {
      putpixel(screen, c, y1, (((c+y1) & 1) == x) ? fg : bg);
      putpixel(screen, c, y2, (((c+y2) & 1) == x) ? fg : bg);
   }

   for (c=y1+1; c<y2; c++) {
      putpixel(screen, x1, c, (((c+x1) & 1) == x) ? fg : bg);
      putpixel(screen, x2, c, (((c+x2) & 1) == x) ? fg : bg);
   }
*/
}



/* d_clear_proc:
 *  Simple dialog procedure which just clears the screen. Useful as the
 *  first object in a dialog.
 */
int d_clear_proc(int msg, DIALOG *d, int c)
{
   if (msg == MSG_DRAW) {
      set_clip(screen, 0, 0, SCREEN_W-1, SCREEN_H-1);
      clear_to_color(screen, d->bg);
   }

   return D_O_K;
}



/* d_box_proc:
 *  Simple dialog procedure: just draws a box.
 */
int d_box_proc(int msg, DIALOG *d, int c)
{
   if (msg==MSG_DRAW) {
      int fg = (d->flags & D_DISABLED) ? gui_mg_color : d->fg;
      rectfill(screen, d->x+1, d->y+1, d->x+d->w-1, d->y+d->h-1, d->bg);
      rect(screen, d->x, d->y, d->x+d->w, d->y+d->h, fg);
   }

   return D_O_K;
}



/* d_shadow_box_proc:
 *  Simple dialog procedure: draws a box with a shadow.
 */
int d_shadow_box_proc(int msg, DIALOG *d, int c)
{
   if (msg==MSG_DRAW) {
      int fg = (d->flags & D_DISABLED) ? gui_mg_color : d->fg;
      fg = 12;  // GB 2014

      rectfill(screen, d->x+1, d->y+1, d->x+d->w-2, d->y+d->h-2, 16); //d->bg);
      rect(screen, d->x, d->y, d->x+d->w-1, d->y+d->h-1, fg);
//    vline(screen, d->x+d->w, d->y+1, d->y+d->h, fg);
//    hline(screen, d->x+1, d->y+d->h, d->x+d->w, fg);
      vline(screen, d->x+d->w, d->y+1, d->y+d->h, 0); // GB 2014
      hline(screen, d->x+1, d->y+d->h, d->x+d->w, 0); // GB 2014
   }

   return D_O_K;
}



/* d_bitmap_proc:
 *  Simple dialog procedure: draws the bitmap which is pointed to by dp.
 */
int d_bitmap_proc(int msg, DIALOG *d, int c)
{
   BITMAP *b = (BITMAP *)d->dp;

   if (msg==MSG_DRAW)
      blit(b, screen, 0, 0, d->x, d->y, d->w, d->h);

   return D_O_K;
}



/* d_text_proc:
 *  Simple dialog procedure: draws the text string which is pointed to by dp.
 */
int d_text_proc(int msg, DIALOG *d, int c)
{
   if (msg==MSG_DRAW) {
      int fg = (d->flags & D_DISABLED) ? gui_mg_color : d->fg;
      fg = 7;  // GB 2014
      text_mode(d->bg);
      gui_textout(screen, d->dp, d->x, d->y, fg, FALSE);
   }

   return D_O_K;
}



/* d_ctext_proc:
 *  Simple dialog procedure: draws the text string which is pointed to by dp,
 *  centering it around the object's x coordinate.
 */
int d_ctext_proc(int msg, DIALOG *d, int c)
{
   if (msg==MSG_DRAW) {
      int fg = (d->flags & D_DISABLED) ? gui_mg_color : d->fg;
      fg = 7;  // GB 2014
//    text_mode(d->bg); GB 2014
	  text_mode(-1);
      gui_textout(screen, d->dp, d->x, d->y, fg, TRUE);
   }
   return D_O_K;
}



/* d_button_proc:
 *  A button object (the dp field points to the text string). This object
 *  can be selected by clicking on it with the mouse or by pressing its 
 *  keyboard shortcut. If the D_EXIT flag is set, selecting it will close 
 *  the dialog, otherwise it will toggle on and off.
 */
int d_button_proc(int msg, DIALOG *d, int c)
{
   int state1, state2;
   int swap;
   int g;

   switch (msg) {

      case MSG_DRAW:
	 if (d->flags & D_SELECTED) {
	    g = 1;
	    state1 = d->bg;
	    state2 = (d->flags & D_DISABLED) ? gui_mg_color : d->fg;
	 }
	 else {
	    g = 0; 
	    state1 = (d->flags & D_DISABLED) ? gui_mg_color : d->fg;
	    state2 = d->bg;
	 }

	 if (state1==255) state1=7;  // GB 2014

	 rectfill(screen, d->x+1+g, d->y+1+g, d->x+d->w-2+g, d->y+d->h-2+g, 16);//state2);
	 rect(screen, d->x+g, d->y+g, d->x+d->w-1+g, d->y+d->h-1+g, 12); //state1);
	 text_mode(-1);
	 gui_textout(screen, d->dp, d->x+d->w/2+g, d->y+d->h/2-text_height(font)/2+g, state1, TRUE);

	 if (d->flags & D_SELECTED) {
	    vline(screen, d->x, d->y, d->y+d->h-1, d->bg);
	    hline(screen, d->x, d->y, d->x+d->w-1, d->bg);
	 }
	 else {
	    vline(screen, d->x+d->w, d->y+1, d->y+d->h-1, 0);//d->fg); GB 2014
	    hline(screen, d->x+1, d->y+d->h, d->x+d->w,   0);   //d->fg);
	 }
	 if ((d->flags & D_GOTFOCUS) && 
	     (!(d->flags & D_SELECTED) || !(d->flags & D_EXIT)))
	    //dotted_rect(d->x+1+g, d->y+1+g, d->x+d->w-2+g, d->y+d->h-2+g, state1, state2);
	    rect(screen,d->x+g, d->y+g, d->x+d->w-1+g, d->y+d->h-1+g, 7); // GB 2014
	 break;

      case MSG_WANTFOCUS:
	 return D_WANTFOCUS;

      case MSG_KEY:
	 /* close dialog? */
	 if (d->flags & D_EXIT) {
	    return D_CLOSE;
	 }

	 /* or just toggle */
	 d->flags ^= D_SELECTED;
	 show_mouse(NULL);
	 SEND_MESSAGE(d, MSG_DRAW, 0);
	 show_mouse(screen);
	 break;

      case MSG_CLICK:
	 /* what state was the button originally in? */
	 state1 = d->flags & D_SELECTED;
	 if (d->flags & D_EXIT)
	    swap = FALSE;
	 else
	    swap = state1;

	 /* track the mouse until it is released */
	 while (mouse_b) {
	    state2 = ((mouse_x >= d->x) && (mouse_y >= d->y) &&
		     (mouse_x < d->x + d->w) && (mouse_y < d->y + d->h));
	    if (swap)
	       state2 = !state2;

	    /* redraw? */
	    if (((state1) && (!state2)) || ((state2) && (!state1))) {
	       d->flags ^= D_SELECTED;
	       state1 = d->flags & D_SELECTED;
	       show_mouse(NULL);
	       SEND_MESSAGE(d, MSG_DRAW, 0);
	       show_mouse(screen);
	    }

	    /* let other objects continue to animate */
	    broadcast_dialog_message(MSG_IDLE, 0);
	 }

	 /* should we close the dialog? */
	 if ((d->flags & D_SELECTED) && (d->flags & D_EXIT)) {
	    d->flags ^= D_SELECTED;
	    return D_CLOSE;
	 }
	 break; 
   }

   return D_O_K;
}



/* d_check_proc:
 *  Who needs C++ after all? This is derived from d_button_proc, 
 *  but overrides the drawing routine to provide a check box.
 */
int d_check_proc(int msg, DIALOG *d, int c)
{
   int x;
   int fg;

   if (msg==MSG_DRAW) {
      fg = (d->flags & D_DISABLED) ? gui_mg_color : d->fg;
      text_mode(d->bg);
      x = d->x + gui_textout(screen, d->dp, d->x, d->y+(d->h-(text_height(font)-gui_font_baseline))/2, fg, FALSE) + text_height(font)/2;
      rectfill(screen, x+1, d->y+1, x+d->h-1, d->y+d->h-1, d->bg);
      rect(screen, x, d->y, x+d->h, d->y+d->h, fg);
      if (d->flags & D_SELECTED) {
	 line(screen, x, d->y, x+d->h, d->y+d->h, fg);
	 line(screen, x, d->y+d->h, x+d->h, d->y, fg); 
      }
      if (d->flags & D_GOTFOCUS) 
	 dotted_rect(x+1, d->y+1, x+d->h-1, d->y+d->h-1, fg, d->bg);
      return D_O_K;
   } 

   return d_button_proc(msg, d, 0);
}



/* d_radio_proc:
 *  GUI procedure for radio buttons.
 *  Parameters: d1-button group number; d2-button style (0=circle,1=square);
 *  dp-text to appear as label to the right of the button.
 */
int d_radio_proc(int msg, DIALOG *d, int c)
{
   int x, center, r, ret, fg;

   switch(msg) {

      case MSG_DRAW:
	 fg = (d->flags & D_DISABLED) ? gui_mg_color : d->fg;
	 text_mode(d->bg);
	 gui_textout(screen, d->dp, d->x+d->h+text_height(font), d->y+(d->h-(text_height(font)-gui_font_baseline))/2, fg, FALSE);

	 x = d->x;
	 r = d->h/2;
	 center = x+r;
	 rectfill(screen, x+1, d->y+1, x+d->h-1, d->y+d->h-1, d->bg);

	 switch (d->d2) {

	    case 1:
	       rect(screen, x, d->y, x+d->h, d->y+d->h, fg);
	       if (d->flags & D_SELECTED)
		  rectfill(screen, x+r/2, d->y+r/2, x+d->h-r/2, d->y+d->h-r/2, fg);
	       break;

	    default:
	       circle(screen, center, d->y+r, r, fg);
	       if (d->flags & D_SELECTED)
		  circlefill(screen, center, d->y+r, r/2, fg);
	       break;
	 }

	 if (d->flags & D_GOTFOCUS)
	    dotted_rect(x+1, d->y+1, x+d->h-1, d->y+d->h-1, fg, d->bg);

	 return D_O_K;

      case MSG_KEY:
      case MSG_CLICK:
	 if (d->flags & D_SELECTED) {
	    return D_O_K;
	 }
      break;

      case MSG_RADIO:
	 if ((c == d->d1) && (d->flags & D_SELECTED)) {
	    d->flags &= ~D_SELECTED;
	    show_mouse(NULL);
	    SEND_MESSAGE(d, MSG_DRAW, 0);
	    show_mouse(screen);
	 }
	 break;
   }

   ret = d_button_proc(msg, d, 0);

   if (((msg==MSG_KEY) || (msg==MSG_CLICK)) &&
       (d->flags & D_SELECTED) && (!(d->flags & D_EXIT))) {
      d->flags &= ~D_SELECTED;
      broadcast_dialog_message(MSG_RADIO, d->d1);
      d->flags |= D_SELECTED;
   }

   return ret;
}



/* d_icon_proc:
 *  Allows graphic icons to be used as buttons.
 * 
 *  Parameters:
 *    fg = color dotted line showing focus will be drawn in
 *    bg = shadow color used to fill in top and left sides of
 *         button when "pressed"
 *    d1 = "push depth": number of pixels icon will be shifted
 *         to right and down when selected (default=2) if there is
 *         no "selected" image
 *    d2 = distance dotted line showing focus is indented (default=2)
 *    dp = pointer to a bitmap for the icon
 *    dp2 = pointer to a "selected" bitmap for the icon (OPTIONAL)
 *    dp3 = pointer to a "disabled" bitmap for the icon (OPTIONAL)
 */
int d_icon_proc(int msg, DIALOG *d, int c)
{
   BITMAP *butimage = (BITMAP *)d->dp;
   int butx;
   int buty;
   int index;
   int indent;
   int depth;

   if ((msg == MSG_DRAW) && (!(d->flags & D_HIDDEN))) {
      depth = 0;
      if ((d->dp2 == NULL) && (d->flags & D_SELECTED)) {
	 depth = d->d1;
	 if (depth<1)
	    depth = 2;
      }
      if ((d->dp2 != NULL) && (d->flags & D_SELECTED)) {
	 butimage = (BITMAP *)d->dp2;
      }
      if ((d->dp3 != NULL) && (d->flags & D_DISABLED)) {
	 butimage = (BITMAP *)d->dp3;
      }
      indent = d->d2;
      if (indent==0)
	 indent = 2;

      /* put the graphic on screen, scaled as needed */
      butx = butimage->w;
      buty = butimage->h;
      stretch_blit(butimage, screen, 0, 0, butx-depth, buty-depth, 
		   d->x+depth, d->y+depth, d->w-depth, d->h-depth);

      if ((d->flags & D_GOTFOCUS) &&
	  (!(d->flags & D_SELECTED) || !(d->flags & D_EXIT))) {
	 /* draw focus lines */
	 for (index=indent; index<d->w-(indent+1); index+=2) {
	    putpixel(screen, d->x+index+depth, d->y+indent+depth,d->fg);
	    putpixel(screen, d->x+index+depth, d->y+d->h-(indent+1)+depth, d->fg);
	 }
	 for (index=indent; index<d->h-(indent+1); index+=2) {
	    putpixel(screen, d->x+indent+depth, d->y+index+depth, d->fg);
	    putpixel(screen, d->x+d->w-(indent+1)+depth, d->y+index+depth, d->fg);
	 }
      }

      /* draw shadowing */
      for (index=0; index<depth; index++) {
	  hline(screen, d->x, d->y+index, d->x+d->w, d->bg);
	  vline(screen, d->x+index, d->y, d->y+d->h, d->bg);
      }

      return D_O_K;
   }

   return d_button_proc(msg, d, c);
}



/* d_keyboard_proc:
 *  Invisible object for implementing keyboard shortcuts. When its key
 *  is pressed, it calls the function pointed to by dp. This should return
 *  an integer, which will be passed back to the dialog manager. The key
 *  can be specified by putting an ASCII code in the key field or by
 *  putting scancodes in d1 and d2.
 */
int d_keyboard_proc(int msg, DIALOG *d, int c)
{
   int (*proc)();
   int ret = D_O_K;

   switch (msg) {

      case MSG_START:
	 d->w = d->h = 0;
	 break;

      case MSG_XCHAR:
	 if (((c>>8) != d->d1) && ((c>>8) != d->d2))
	    break;

	 ret |= D_USED_CHAR;
	 /* fall through */

      case MSG_KEY:
	 proc = d->dp;
	 ret |= (*proc)();
	 break;
   }

   return ret;
}



/* d_edit_proc:
 *  An editable text object (the dp field points to the string). When it
 *  has the input focus (obtained by clicking on it with the mouse), text
 *  can be typed into this object. The d1 field specifies the maximum
 *  number of characters that it will accept, and d2 is the text cursor 
 *  position within the string.
 */
int d_edit_proc(int msg, DIALOG *d, int c)
{
   int f, l, p, w, x, fg;
   int b; 
   int scroll; 
   char *s;
   char buf[2];

   s = d->dp;
   l = strlen(s);
   if (d->d2 > l) 
      d->d2 = l;

  /* calculate maximal number of displayable characters */
   b = x = 0;
   if (d->d2 == l)  {
      buf[0] = ' ';
      buf[1] = 0;
      x = text_length(font, buf);
   }

   buf[1] = 0;
   for (p=d->d2; p>=0; p--) {
      buf[0] = s[p];
      b++;
      x += text_length(font, buf);
      if (x > d->w) 
	 break;
   }

   if (x <= d->w) {
      b = l; 
      scroll = FALSE;
   }
   else {
      b--; 
      scroll = TRUE;
   }

   switch (msg) {

      case MSG_START:
	 d->d2 = l;
	 break;

      case MSG_DRAW:
	 fg = (d->flags & D_DISABLED) ? gui_mg_color : d->fg;
	 x = 0;

	 if (scroll) {
	    p = d->d2-b+1; 
	    b = d->d2; 
	 }
	 else 
	    p = 0; 

	 for (; p<=b; p++) {
	    buf[0] = s[p] ? s[p] : ' ';
	    w = text_length(font, buf);
	    if (x+w > d->w) 
	       break;
	    f = ((p == d->d2) && (d->flags & D_GOTFOCUS));
	    text_mode(f ? fg : d->bg);
	    textout(screen, font, buf, d->x+x, d->y, f ? d->bg : fg);
	    x += w;
	 }
	 if (x<d->w)
	    rectfill(screen, d->x+x, d->y, d->x+d->w-1, d->y+text_height(font)-1, d->bg);
	 break;

      case MSG_CLICK:
	 x = d->x;

	 if (scroll) {
	    p = d->d2-b+1; 
	    b = d->d2; 
	 }
	 else
	    p = 0; 

	 for (; p<b; p++) {
	    buf[0] = s[p];
	    x += text_length(font, buf);
	    if (x > mouse_x) 
	       break;
	 }
	 d->d2 = MID(0, p, l);
	 show_mouse(NULL);
	 SEND_MESSAGE(d, MSG_DRAW, 0);
	 show_mouse(screen);
	 break;

      case MSG_WANTFOCUS:
      case MSG_LOSTFOCUS:
      case MSG_KEY:
	 return D_WANTFOCUS;

      case MSG_CHAR:
	 if ((c >> 8) == KEY_LEFT) {
	    if (d->d2 > 0) d->d2--;
	 }
	 else if ((c >> 8) == KEY_RIGHT) {
	    if (d->d2 < l) d->d2++;
	 }
	 else if ((c >> 8) == KEY_HOME) {
	    d->d2 = 0;
	 }
	 else if ((c >> 8) == KEY_END) {
	    d->d2 = l;
	 }
	 else if ((c >> 8) == KEY_DEL) {
	    if (d->d2 < l)
	       for (p=d->d2; s[p]; p++) 
		  s[p] = s[p+1];
	 }
	 else if ((c >> 8) == KEY_BACKSPACE) {
	    if (d->d2 > 0) {
	       d->d2--;
	       for (p=d->d2; s[p]; p++) 
		  s[p] = s[p+1];
	    }
	 }
	 else if ((c >> 8) == KEY_ENTER) {
	    if (d->flags & D_EXIT) {
	       show_mouse(NULL);
	       SEND_MESSAGE(d, MSG_DRAW, 0);
	       show_mouse(screen);
	       return D_CLOSE;
	    }
	    else
	       return D_O_K;
	 }
	 else {
	    c &= 0xff;
	    if ((c >= 32) && (c <= 255)) {
	       if (l < d->d1) {
		  while (l >= d->d2) {
		     s[l+1] = s[l];
		     l--;
		  }
		  s[d->d2] = c;
		  d->d2++;
	       }
	    }
	    else
	       return D_O_K;
	 }

	 /* if we changed something, better redraw... */
	 show_mouse(NULL);
	 SEND_MESSAGE(d, MSG_DRAW, 0);
	 show_mouse(screen);
	 return D_USED_CHAR;
   }

   return D_O_K;
}



/* _handle_scrollable_click:
 *  Helper to process a click on a scrollable object.
 */
void _handle_scrollable_scroll_click(DIALOG *d, int listsize, int *offset)
{
   int xx, yy;
   int height = (d->h-3) / text_height(font);
   int hh = d->h-4;

   while (mouse_b) {
      int i = (hh * height + listsize/2) / listsize;
      int len = (hh * (*offset) + listsize/2) / listsize + 2;

      if ((mouse_y >= d->y+len) && (mouse_y <= d->y+len+i)) {
	 xx = mouse_y - len + 2;
	 while (mouse_b) {
	    yy = (listsize * (mouse_y - xx) + hh/2) / hh;
	    if (yy > listsize-height) 
	       yy = listsize-height;

	    if (yy < 0) 
	       yy = 0;

	    if (yy != *offset) {
	       *offset = yy;
	       show_mouse(NULL);
	       d->proc(MSG_DRAW, d, 0);
	       show_mouse(screen);
	    }

	    /* let other objects continue to animate */
	    broadcast_dialog_message(MSG_IDLE, 0);
	 }
      }
      else {
	 if (mouse_y <= d->y+len) 
	    yy = *offset - height;
	 else 
	    yy = *offset + height;

	 if (yy > listsize-height) 
	    yy = listsize-height;

	 if (yy < 0) 
	    yy = 0;

	 if (yy != *offset) {
	    *offset = yy;
	    show_mouse(NULL);
	    d->proc(MSG_DRAW, d, 0);
	    show_mouse(screen);
	 }
      }

      /* let other objects continue to animate */
      broadcast_dialog_message(MSG_IDLE, 0);
   }
}



/* _handle_scrollable_scroll:
 *  Helper function to scroll through a scrollable object.
 */
void _handle_scrollable_scroll(DIALOG *d, int listsize, int *index, int *offset)
{
   int height = (d->h-3) / text_height(font);

   if (listsize <= 0) {
      *index = *offset = 0;
      return;
   }

   /* check selected item */
   if (*index < 0) 
      *index = 0;
   else if (*index >= listsize)
      *index = listsize - 1;

   /* check scroll position */
   while ((*offset > 0) && (*offset + height > listsize))
      (*offset)--;

   if (*offset >= *index) {
      if (*index < 0) 
	 *offset = 0;
      else
	 *offset = *index;
   }
   else {
      while ((*offset + height - 1) < *index)
	 (*offset)++;
   }
}



/* idle_cb:
 *  rest_callback() routine to keep dialogs animating nice and smoothly.
 */
static void idle_cb()
{
   broadcast_dialog_message(MSG_IDLE, 0);
}



/* _handle_listbox_click:
 *  Helper to process a click on a listbox, doing hit-testing and moving
 *  the selection.
 */
void _handle_listbox_click(DIALOG *d)
{
   char *sel = d->dp2;
   int listsize, height;
   int i, j;

   (*(getfuncptr)d->dp)(-1, &listsize);
   if (!listsize)
      return;

   height = (d->h-3) / text_height(font);

   i = MID(0, ((mouse_y - d->y - 2) / text_height(font)), 
			   ((d->h-3) / text_height(font) - 1));
   i += d->d2;
   if (i < d->d2)
      i = d->d2;
   else {
      if (i > d->d2 + height-1)
	 i = d->d2 + height-1;
      if (i >= listsize)
	 i = listsize-1;
   }

   if (mouse_y <= d->y)
      i = MAX(i-1, 0);
   else if (mouse_y >= d->y+d->h)
      i = MIN(i+1, listsize-1);

   if (i != d->d1) {
      if (sel) {
	 if (key_shifts & (KB_SHIFT_FLAG | KB_CTRL_FLAG)) {
	    if ((key_shifts & KB_SHIFT_FLAG) || (d->flags & D_INTERNAL)) {
	       for (j=MIN(i, d->d1); j<=MAX(i, d->d1); j++)
		  sel[j] = TRUE;
	    }
	    else
	       sel[i] = TRUE;
	 }
      }

      d->d1 = i;
      i = d->d2;
      _handle_scrollable_scroll(d, listsize, &d->d1, &d->d2);

      show_mouse(NULL);
      SEND_MESSAGE(d, MSG_DRAW, 0);
      show_mouse(screen);

      if (i != d->d2)
	 rest_callback(MID(10, text_height(font)*16-d->h, 100), idle_cb);
   }
}



/* _draw_scrollable_frame:
 *  Helper function to draw a frame for all objects with vertical scrollbars.
 */
void _draw_scrollable_frame(DIALOG *d, int listsize, int offset, int height, int fg_color, int bg)
{
   int i, len;
   BITMAP *pattern;
   int xx, yy;

   fg_color = 12; // GB 2014

   /* draw frame */
   rect(screen, d->x, d->y, d->x+d->w, d->y+d->h, fg_color);

   /* possibly draw scrollbar */
   if (listsize > height) {
      vline(screen, d->x+d->w-12, d->y+1, d->y+d->h-1, fg_color);

      /* scrollbar with focus */ 
      if (d->flags & D_GOTFOCUS) {
	 dotted_rect(d->x+1, d->y+1, d->x+d->w-13, d->y+d->h-1, fg_color, bg);
	 dotted_rect(d->x+d->w-11, d->y+1, d->x+d->w-1, d->y+d->h-1, fg_color, bg);
      }
      else {
	 rect(screen, d->x+1, d->y+1, d->x+d->w-13, d->y+d->h-1, bg);
	 rect(screen, d->x+d->w-11, d->y+1, d->x+d->w-1, d->y+d->h-1, bg);
      }

      /* create and draw the scrollbar */
      pattern = create_bitmap(2, 2);
      i = ((d->h-4) * height + listsize/2) / listsize;
      xx = d->x+d->w-10;
      yy = d->y+2;

      putpixel(pattern, 0, 1, bg);
      putpixel(pattern, 1, 0, bg);
      putpixel(pattern, 0, 0, fg_color);
      putpixel(pattern, 1, 1, fg_color);

      if (offset > 0) {
	 len = (((d->h-4) * offset) + listsize/2) / listsize;
	 rectfill(screen, xx, yy, xx+8, yy+len-1, bg);
	 yy += len;
      }
      if (yy+i < d->y+d->h-2) {
	 drawing_mode(DRAW_MODE_COPY_PATTERN, pattern, 0, 0);
	 rectfill(screen, xx, yy, xx+8, yy+i, 0);
	 solid_mode();
	 yy += i;
	 rectfill(screen, xx, yy, xx+8, d->y+d->h-2, bg);
      }
      else {
	 drawing_mode(DRAW_MODE_COPY_PATTERN, pattern, 0, 0);
	 rectfill(screen, xx, yy, xx+8, d->y+d->h-2, 0);
	 solid_mode();
      }
      destroy_bitmap(pattern);
   }
   else {
      /* no scrollbar necessary */
      if (d->flags & D_GOTFOCUS) 
	 dotted_rect(d->x+1, d->y+1, d->x+d->w-1, d->y+d->h-1, fg_color, bg);
      else
	 rect(screen, d->x+1, d->y+1, d->x+d->w-1, d->y+d->h-1, bg);
   }
}



/* draw_listbox:
 *  Helper function to draw a listbox object.
 */
void _draw_listbox(DIALOG *d)
{
   int height, listsize, i, len, bar, x, y, w;
   int fg_color, fg, bg;
   char *sel = d->dp2;
   char *s;
   char store;

   (*(getfuncptr)d->dp)(-1, &listsize);
   height = (d->h-3) / text_height(font);
   bar = (listsize > height);
   w = (bar ? d->w-14 : d->w-2);
   fg_color = (d->flags & D_DISABLED) ? gui_mg_color : d->fg;

   fg_color = 7;  // GB 2014 text only

   /* draw box contents */
   for (i=0; i<height; i++) {
      if (d->d2+i < listsize) {
	 if (d->d2+i == d->d1) {
	    fg = d->bg;
	    bg = fg_color;
	 } 
	 else if ((sel) && (sel[d->d2+i])) { 
	    fg = d->bg;
	    bg = gui_mg_color;
	 }
	 else {
	    fg = fg_color;
	    bg = d->bg;
	 }


	 s = (*(getfuncptr)d->dp)(i+d->d2, NULL);
	 x = d->x + 2;
	 y = d->y + 2 + i*text_height(font);
	 text_mode(bg);
	 rectfill(screen, x, y, x+7, y+text_height(font)-1, bg); 
	 x += 8;
	 len = strlen(s);
	 store = 0;
	 while (text_length(font, s) >= d->w - (bar ? 22 : 10)) {
	    s[len] = store;
	    len--;
	    store = s[len];
	    s[len] = 0;
	 }
	 textout(screen, font, s, x, y, fg); 
	 x += text_length(font, s);
	 s[len] = store;
	 if (x <= d->x+w) 
	    rectfill(screen, x, y, d->x+w, y+text_height(font)-1, bg);
      }
      else
	 rectfill(screen, d->x+2,  d->y+2+i*text_height(font), 
		  d->x+w, d->y+1+(i+1)*text_height(font), d->bg);
   }

   if (d->y+2+i*text_height(font) <= d->y+d->h-2)
      rectfill(screen, d->x+2, d->y+2+i*text_height(font), 
				       d->x+w, d->y+d->h-2, d->bg);

   /* draw frame, maybe with scrollbar */
   _draw_scrollable_frame(d, listsize, d->d2, height, fg_color, d->bg);
}



/* d_list_proc:
 *  A list box object. The dp field points to a function which it will call
 *  to obtain information about the list. This should follow the form:
 *     char *<list_func_name> (int index, int *list_size);
 *  If index is zero or positive, the function should return a pointer to
 *  the string which is to be displayed at position index in the list. If
 *  index is  negative, it should return null and list_size should be set
 *  to the number of items in the list. The list box object will allow the
 *  user to scroll through the list and to select items list by clicking
 *  on them, and if it has the input focus also by using the arrow keys. If 
 *  the D_EXIT flag is set, double clicking on a list item will cause it to 
 *  close the dialog. The index of the selected item is held in the d1 
 *  field, and d2 is used to store how far it has scrolled through the list.
 */
int d_list_proc(int msg, DIALOG *d, int c)
{
   int listsize, i, bottom, height, bar, orig;
   char *sel = d->dp2;
   int redraw = FALSE;

   switch (msg) {

      case MSG_START:
	 (*(getfuncptr)d->dp)(-1, &listsize);
	 _handle_scrollable_scroll(d, listsize, &d->d1, &d->d2);
	 break;

      case MSG_DRAW:
	 _draw_listbox(d);
	 break;

      case MSG_CLICK:
	 (*(getfuncptr)d->dp)(-1, &listsize);
	 height = (d->h-3) / text_height(font);
	 bar = (listsize > height);
	 if ((!bar) || (mouse_x < d->x+d->w-10)) {
	    if ((sel) && (!(key_shifts & KB_CTRL_FLAG))) {
	       for (i=0; i<listsize; i++) {
		  if (sel[i]) {
		     redraw = TRUE;
		     sel[i] = FALSE;
		  }
	       }
	       if (redraw) {
		  show_mouse(NULL);
		  SEND_MESSAGE(d, MSG_DRAW, 0);
		  show_mouse(screen);
	       }
	    }
	    _handle_listbox_click(d);
	    while (mouse_b) {
	       broadcast_dialog_message(MSG_IDLE, 0);
	       d->flags |= D_INTERNAL;
	       _handle_listbox_click(d);
	       d->flags &= ~D_INTERNAL;
	    }
	 }
	 else {
	    _handle_scrollable_scroll_click(d, listsize, &d->d2);
	 }
	 break;

      case MSG_DCLICK:
	 (*(getfuncptr)d->dp)(-1, &listsize);
	 height = (d->h-3) / text_height(font);
	 bar = (listsize > height);
	 if ((!bar) || (mouse_x < d->x+d->w-10)) {
	    if (d->flags & D_EXIT) {
	       if (listsize) {
		  i = d->d1;
		  SEND_MESSAGE(d, MSG_CLICK, 0);
		  if (i == d->d1) 
		     return D_CLOSE;
	       }
	    }
	 }
	 break;

      case MSG_KEY:
	 (*(getfuncptr)d->dp)(-1, &listsize);
	 if ((listsize) && (d->flags & D_EXIT))
	    return D_CLOSE;
	 break;

      case MSG_WANTFOCUS:
	 return D_WANTFOCUS;

      case MSG_CHAR:
	 (*(getfuncptr)d->dp)(-1,&listsize);
	 if (listsize) {
	    c >>= 8;

	    bottom = d->d2 + (d->h-3)/text_height(font) - 1;
	    if (bottom >= listsize-1)
	       bottom = listsize-1;

	    orig = d->d1;

	    if (c == KEY_UP)
	       d->d1--;
	    else if (c == KEY_DOWN)
	       d->d1++;
	    else if (c == KEY_HOME)
	       d->d1 = 0;
	    else if (c == KEY_END)
	       d->d1 = listsize-1;
	    else if (c == KEY_PGUP) {
	       if (d->d1 > d->d2)
		  d->d1 = d->d2;
	       else
		  d->d1 -= (bottom - d->d2);
	    }
	    else if (c == KEY_PGDN) {
	       if (d->d1 < bottom)
		  d->d1 = bottom;
	       else
		  d->d1 += (bottom - d->d2);
	    } 
	    else 
	       return D_O_K;

	    if (sel) {
	       if (!(key_shifts & (KB_SHIFT_FLAG | KB_CTRL_FLAG))) {
		  for (i=0; i<listsize; i++)
		     sel[i] = FALSE;
	       }
	       else if (key_shifts & KB_SHIFT_FLAG) {
		  for (i=MIN(orig, d->d1); i<=MAX(orig, d->d1); i++) {
		     if (key_shifts & KB_CTRL_FLAG)
			sel[i] = (i != d->d1);
		     else
			sel[i] = TRUE;
		  }
	       }
	    }

	    /* if we changed something, better redraw... */ 
	    _handle_scrollable_scroll(d, listsize, &d->d1, &d->d2);
	    show_mouse(NULL);
	    SEND_MESSAGE(d, MSG_DRAW, 0);
	    show_mouse(screen); 
	    return D_USED_CHAR;
	 }
	 break;
   }

   return D_O_K;
}



/* _draw_textbox:
 *  Helper function to draw a textbox object.
 */
void _draw_textbox(char *thetext, int *listsize, int draw, int offset,
		   int wword, int tabsize, int x, int y, int w, int h,
		   int disabled, int fore, int deselect, int disable)
{
   int fg = fore;
   int y1 = y+4;
   int x1;
   int len;
   int ww = w-4;

   char *s = ".";

   const char *text = " ";

   const char *printed = text;
   const char *scanned = text;
   const char *oldscan = text;
   const char *ignore = NULL;
   int width;
   int line = 0;

   int i = 0;

   /* find the correct text */
   if (thetext != NULL) {
      printed = thetext;
      scanned = thetext;
   }

   /* do some drawing setup */
   if (draw) {
      /* initial start blanking at the top */
      rectfill(screen, x+2, y+2, x+w-1, y1-1, deselect);
   }

   /* choose the text color */
   if (disabled) 
      fg = disable;

   text_mode(deselect);

   /* loop over the entire string */
   while (1) {
      width = 0;

      /* find the next break */
      while (scanned[0]) {
	 /* check for a forced break */
	 if (scanned[0] == '\n') {
	    scanned++;

	    /* we are done parsing the line end */
	    break;
	 }

	 /* the next character length */
	 s[0] = scanned[0];
	 len = text_length(font, s);

	 /* modify length if its a tab */
	 if (s[0] == '\t') 
	    len = tabsize * text_length(font, " ");

	 /* check for the end of a line by excess width of next char */
	 if (width+len >= ww) {
	    /* we have reached end of line do we go back to find start */
	    if (wword) {
	       /* remember where we were */
	       oldscan = scanned;

	       /* go backwards looking for start of word */
	       while (!isspace(scanned[0])) {
		  /* don't wrap too far */
		  if (scanned == printed) {
		     /* the whole line is filled, so stop here */
		     scanned = oldscan;
		     break;
		  }
		  /* look further backwards to wrap */
		  scanned--;
	       }
	       /* put the space at the end of the line */
	       ignore = scanned;
	       scanned++;

	       /* check for endline at the convenient place */
	       if (scanned[0] == '\n') 
		  scanned++;
	    }
	    /* we are done parsing the line end */
	    break;
	 }
	 /* the character can be added */
	 scanned++;
	 width += len;
      }
      /* check if we are to print it */
      if ((draw) && (line >= offset) && (y1+text_height(font) < (y+h-1))) {
	 x1 = x+4;

	 /* the initial blank bit */
	 rectfill(screen, x+2, y1, x1-1, y1+text_height(font), deselect);

	 /* print up to the marked character */
	 while (printed != scanned) {
	    /* do special stuff for each charater */
	    switch (printed[0]) {

	       case '\n':
		  /* don't print and endlines in the text, we'll do that ourselves */
		  break;

	       /* possibly expand the tabs */
	       case '\t':
		  for (i=0; i<tabsize; i++) {
		     s[0] = ' ';
		     textout(screen, font, s, x1, y1, fg);
		     x1 += text_length(font, s);
		  }
		  break;

	       /* print a normal charater */
	       default:
		  if (printed != ignore) {
		     s[0] = printed[0];
		     textout(screen, font, s, x1, y1, fg);
		     x1 += text_length(font, s);
		  }
	    }
	    /* goto the next character */
	    printed++;
	 }
	 /* the last blank bit */
	 if (x1 < x+w-1) 
	    rectfill(screen, x1, y1, x+w-1, y1+text_height(font)-1, deselect);

	 /* print the line end */
	 y1 += text_height(font);
      }
      printed = scanned;

      /* we have done a line */
      line++;

      /* check if we are at the end of the string */
      if (*printed == '\0') {
	 /* the under blank bit */
	 if (draw) 
	    rectfill(screen, x+1, y1, x+w-1, y+h-1, deselect);

	 /* tell how many lines we found */
	 *listsize = line;
	 return;
      }
   }
}



/* d_textbox_proc:
 *  A text box object. The dp field points to a char * which is the text
 *  to be displayed in the text box. If the text is long, there will be
 *  a vertical scrollbar on the right hand side of the object which can
 *  be used to scroll through the text. The default is to print the text
 *  with word wrapping, but if the D_SELECTED flag is set, the text will
 *  be printed with character wrapping. The d1 field is used internally
 *  to store the number of lines of text, and d2 is used to store how far
 *  it has scrolled through the text.
 */
int d_textbox_proc(int msg, DIALOG *d, int c)
{
   int height, bar, ret = D_O_K;
   int start, top, bottom,l;
   int used;
// int fg_color = (d->flags & D_DISABLED) ? gui_mg_color : d->fg;
   int fg_color = 7;  // GB 2014

   /* calculate the actual height */
   height = (d->h-4) / text_height(font);

   switch (msg) {

      case MSG_START:
	 /* measure how many lines of text we contain */
	 _draw_textbox(d->dp, &d->d1, 
		       0, /* DONT DRAW anything */
		       d->d2, !(d->flags & D_SELECTED), 8,
		       d->x, d->y, d->w, d->h,
		       (d->flags & D_DISABLED),
		       0, 0, 0);
	 break;

      case MSG_DRAW:
	 /* tell the object to sort of draw, but only calculate the listsize */
	 _draw_textbox(d->dp, &d->d1, 
		       0, /* DONT DRAW anything */
		       d->d2, !(d->flags & D_SELECTED), 8,
		       d->x, d->y, d->w, d->h,
		       (d->flags & D_DISABLED),
		       0, 0, 0);

	 if (d->d1 > height) {
	    bar = 12;
	 }
	 else {
	    bar = 0;
	    d->d2 = 0;
	 }

	 /* now do the actual drawing */
	 _draw_textbox(d->dp, &d->d1, 1, d->d2,
		       !(d->flags & D_SELECTED), 8,
		       d->x, d->y, d->w-bar-1, d->h,
		       (d->flags & D_DISABLED),
		       fg_color, d->bg, gui_mg_color);

	 /* draw the frame around */
	 _draw_scrollable_frame(d, d->d1, d->d2, height, fg_color, d->bg);
	 break;

      case MSG_CLICK:
	 /* figure out if it's on the text or the scrollbar */
	 bar = (d->d1 > height);

	 if ((!bar) || (mouse_x < d->x+d->w-12)) {
	    /* clicked on the text area */
	    ret = D_O_K;
	 }
	 else {
	    /* clicked on the scroll area */
	    _handle_scrollable_scroll_click(d, d->d1, &d->d2);
	 }
	 break;

      case MSG_CHAR:
	 start = d->d2;
	 used = D_USED_CHAR;

	 if (d->d1 > 0) {
	    if (d->d2 > 0) 
	       top = d->d2+1;
	    else 
	       top = 0;

	    l = (d->h-3)/text_height(font);

	    bottom = d->d2 + l - 1;
	    if (bottom >= d->d1-1) 
	       bottom = d->d1-1;
	    else 
	       bottom--;

	    if ((c>>8) == KEY_UP) 
	       d->d2--;
	    else if ((c>>8) == KEY_DOWN) 
	       d->d2++;
	    else if ((c>>8) == KEY_HOME) 
	       d->d2 = 0;
	    else if ((c>>8) == KEY_END) 
	       d->d2 = d->d1-l;
	    else if ((c>>8) == KEY_PGUP) 
	       d->d2 = d->d2-(bottom-top);
	    else if ((c>>8) == KEY_PGDN) 
	       d->d2 = d->d2+(bottom-top);
	    else 
	       used = D_O_K;

	    /* make sure that the list stays in bounds */
	    if (d->d2 > d->d1-l) 
	       d->d2 = d->d1-l;
	    if (d->d2 < 0) 
	       d->d2 = 0;
	 }
	 else 
	    used = D_O_K;

	 /* if we changed something, better redraw... */
	 if (d->d2 != start) {
	    show_mouse(NULL);
	    d->proc(MSG_DRAW, d, 0);
	    show_mouse(screen);
	 }
	 ret = used;
	 break;

      case MSG_WANTFOCUS:
	 /* if we don't have a scrollbar we can't do anything with the focus */
	 if (d->d1 > height) 
	    ret = D_WANTFOCUS;
	 break;

      default:
	 ret = D_O_K;
   }

   return ret;
}



/* d_slider_proc:
 *  A slider control object. This object returns a value in d2, in the
 *  range from 0 to d1. It will display as a vertical slider if h is
 *  greater than or equal to w; otherwise, it will display as a horizontal
 *  slider. dp can contain an optional bitmap to use for the slider handle; 
 *  dp2 can contain an optional callback function, which is called each 
 *  time d2 changes. The callback function should have the following
 *  prototype:
 *
 *  int function(void *dp3, int d2);
 *
 *  The d_slider_proc object will return the value of the callback function.
 */
int d_slider_proc(int msg, DIALOG *d, int c)
{
   BITMAP *slhan = NULL;
   int oldpos, newpos;
   int sfg;                /* slider foreground color */
   int vert = TRUE;        /* flag: is slider vertical? */
   int hh = 7;             /* handle height (width for horizontal sliders) */
   int hmar;               /* handle margin */
   int slp;                /* slider position */
   int mp;                 /* mouse position */
   int irange;
   int slx, sly, slh, slw;
   int msx, msy;
   int retval = D_O_K;
   int upkey, downkey;
   int pgupkey, pgdnkey;
   int homekey, endkey;
   int delta;
   fixed slratio, slmax, slpos;
   int (*proc)(void *cbpointer, int d2value);

   /* check for slider direction */
   if (d->h < d->w)
      vert = FALSE;

   /* set up the metrics for the control */
   if (d->dp != NULL) {
      slhan = (BITMAP *)d->dp;
      if (vert)
	 hh = slhan->h;
      else
	 hh = slhan->w;
   }

   hmar = hh/2;
   irange = (vert) ? d->h : d->w;
   slmax = itofix(irange-hh);
   slratio = slmax / (d->d1);
   slpos = slratio * d->d2;
   slp = fixtoi(slpos);

   switch (msg) {

      case MSG_DRAW:
	 sfg = (d->flags & D_DISABLED) ? gui_mg_color : d->fg;

	 if (vert) {
	    rectfill(screen, d->x, d->y, d->x+d->w/2-2, d->y+d->h, d->bg);
	    rectfill(screen, d->x+d->w/2-1, d->y, d->x+d->w/2+1, d->y+d->h, sfg);
	    rectfill(screen, d->x+d->w/2+2, d->y, d->x+d->w, d->y+d->h, d->bg);
	 }
	 else {
	    rectfill(screen, d->x, d->y, d->x+d->w, d->y+d->h/2-2, d->bg);
	    rectfill(screen, d->x, d->y+d->h/2-1, d->x+d->w, d->y+d->h/2+1, sfg);
	    rectfill(screen, d->x, d->y+d->h/2+2, d->x+d->w, d->y+d->h, d->bg);
	 }

	 /* okay, background and slot are drawn, now draw the handle */
	 if (slhan) {
	    if (vert) {
	       slx = d->x+(d->w/2)-(slhan->w/2);
	       sly = d->y+d->h-(hh+slp);
	    } 
	    else {
	       slx = d->x+slp;
	       sly = d->y+(d->h/2)-(slhan->h/2);
	    }
	    draw_sprite(screen, slhan, slx, sly);
	 } 
	 else {
	    /* draw default handle */
	    if (vert) {
	       slx = d->x;
	       sly = d->y+d->h-(hh+slp);
	       slw = d->w;
	       slh = hh;
	    } else {
	       slx = d->x+slp;
	       sly = d->y;
	       slw = hh;
	       slh = d->h;
	    }

	    /* draw body */
	    rectfill(screen, slx+2, sly, slx+(slw-2), sly+slh, sfg);
	    vline(screen, slx+1, sly+1, sly+slh-1, sfg);
	    vline(screen, slx+slw-1, sly+1, sly+slh-1, sfg);
	    vline(screen, slx, sly+2, sly+slh-2, sfg);
	    vline(screen, slx+slw, sly+2, sly+slh-2, sfg);
	    vline(screen, slx+1, sly+2, sly+slh-2, d->bg);
	    hline(screen, slx+2, sly+1, slx+slw-2, d->bg);
	    putpixel(screen, slx+2, sly+2, d->bg);
	 }

	 if (d->flags & D_GOTFOCUS) 
	    dotted_rect(d->x, d->y, d->x+d->w, d->y+d->h, sfg, d->bg);
	 break;

      case MSG_WANTFOCUS:
      case MSG_LOSTFOCUS:
	 return D_WANTFOCUS;

      case MSG_KEY:
	 if (!(d->flags & D_GOTFOCUS))
	    return D_WANTFOCUS;
	 else
	    return D_O_K;

      case MSG_CHAR:
	 /* handle movement keys to move slider */
	 c >>= 8;

	 if (vert) {
	    upkey = KEY_UP;
	    downkey = KEY_DOWN;
	    pgupkey = KEY_PGUP;
	    pgdnkey = KEY_PGDN;
	    homekey = KEY_END;
	    endkey = KEY_HOME;
	 } 
	 else {
	    upkey = KEY_RIGHT;
	    downkey = KEY_LEFT;
	    pgupkey = KEY_PGDN;
	    pgdnkey = KEY_PGUP;
	    homekey = KEY_HOME;
	    endkey = KEY_END;
	 }

	 if (c == upkey)
	    delta = 1;
	 else if (c == downkey)
	    delta = -1;
	 else if (c == pgdnkey)
	    delta = -d->d1 / 16;
	 else if (c == pgupkey)
	    delta = d->d1 / 16;
	 else if (c == homekey)
	    delta = -d->d2;
	 else if (c == endkey)
	    delta = d->d1 - d->d2;
	 else
	    delta = 0;

	 if (delta) {
	    oldpos = slp;

	    while (1) {
	       d->d2 = d->d2+delta;
	       slpos = slratio*d->d2;
	       slp = fixtoi(slpos);
	       if ((slp != oldpos) || (d->d2 <= 0) || (d->d2 >= d->d1))
		  break;
	    }

	    if (d->d2 < 0)
	       d->d2 = 0;
	    if (d->d2 > d->d1)
	       d->d2 = d->d1;

	    retval = D_USED_CHAR;

	    /* call callback function here */
	    if (d->dp2) {
	       proc = d->dp2;
	       retval |= (*proc)(d->dp3, d->d2);
	    }

	    show_mouse(NULL);
	    SEND_MESSAGE(d, MSG_DRAW, 0);
	    show_mouse(screen);
	 }
	 break;

      case MSG_CLICK:
	 /* track the mouse until it is released */
	 mp = slp;

	 while (mouse_b) {
	    msx = mouse_x;
	    msy = mouse_y;
	    oldpos = d->d2;
	    if (vert)
	       mp = (d->y+d->h-hmar)-msy;
	    else
	       mp = msx-(d->x+hmar);
	    if (mp < 0)
	       mp = 0;
	    if (mp > irange-hh)
	       mp = irange-hh;
	    slpos = itofix(mp);
	    slmax = fdiv(slpos, slratio);
	    newpos = fixtoi(slmax);
	    if (newpos != oldpos) {
	       d->d2 = newpos;

	       /* call callback function here */
	       if (d->dp2 != NULL) {
		  proc = d->dp2;
		  retval |= (*proc)(d->dp3, d->d2);
	       }

	       if (d->d2 != oldpos) {
		  show_mouse(NULL);
		  SEND_MESSAGE(d, MSG_DRAW, 0);
		  show_mouse(screen);
	       }
	    }

	    /* let other objects continue to animate */
	    broadcast_dialog_message(MSG_IDLE, 0);
	 }
	 break;
   }

   return retval;
}

