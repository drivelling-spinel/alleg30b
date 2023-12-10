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
 *      The floodfill routine.
 *
 *      See readme.txt for copyright information.
 */


#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <limits.h>

#ifdef DJGPP
#include <sys/farptr.h>
#endif

#include "allegro.h"
#include "internal.h"


typedef struct FLOODED_LINE      /* store segments which have been flooded */
{
   short flags;                  /* status of the segment */
   short lpos, rpos;             /* left and right ends of segment */
   short y;                      /* y coordinate of the segment */
   short next;                   /* linked list if several per line */
} FLOODED_LINE;

static int flood_count;          /* number of flooded segments */

#define FLOOD_IN_USE             1
#define FLOOD_TODO_ABOVE         2
#define FLOOD_TODO_BELOW         4

#define FLOOD_LINE(c)            (((FLOODED_LINE *)_scratch_mem) + c)



/* flooder:
 *  Fills a horizontal line around the specified position, and adds it
 *  to the list of drawn segments. Returns the first x coordinate after 
 *  the part of the line which it has dealt with.
 */
static int flooder(BITMAP *bmp, int x, int y, int src_color, int dest_color)
{
   int c;
   FLOODED_LINE *p;
   int left, right;
   unsigned long addr;

   if (is_linear_bitmap(bmp)) {     /* use direct access for linear bitmaps */
      addr = bmp_read_line(bmp, y);
      _farsetsel(bmp->seg);

      #ifdef ALLEGRO_COLOR16

	 if ((bitmap_color_depth(bmp) == 15) || 
	     (bitmap_color_depth(bmp) == 16)) {

	    /* check start pixel */
	    if (_farnspeekw(addr+x*2) != src_color)
	       return x+1;

	    /* work left from starting point */ 
	    for (left=x-1; left>=bmp->cl; left--)
	       if (_farnspeekw(addr+left*2) != src_color)
		  break;

	    /* work right from starting point */ 
	    for (right=x+1; right<bmp->cr; right++)
	       if (_farnspeekw(addr+right*2) != src_color)
		  break;
	 }
	 else

      #endif

      #ifdef ALLEGRO_COLOR24

	 if (bitmap_color_depth(bmp) == 24) {

	    /* check start pixel */
	    if ((_farnspeekl(addr+x*3) & 0xFFFFFF) != (unsigned)src_color)
	       return x+1;

	    /* work left from starting point */ 
	    for (left=x-1; left>=bmp->cl; left--)
	       if ((_farnspeekl(addr+left*3) & 0xFFFFFF) != (unsigned)src_color)
		  break;

	    /* work right from starting point */ 
	    for (right=x+1; right<bmp->cr; right++)
	       if ((_farnspeekl(addr+right*3) & 0xFFFFFF) != (unsigned)src_color)
		  break;
	 }
	 else

      #endif

      #ifdef ALLEGRO_COLOR32

	 if (bitmap_color_depth(bmp) == 32) {

	    /* check start pixel */
	    if (_farnspeekl(addr+x*4) != (unsigned)src_color)
	       return x+1;

	    /* work left from starting point */ 
	    for (left=x-1; left>=bmp->cl; left--)
	       if (_farnspeekl(addr+left*4) != (unsigned)src_color)
		  break;

	    /* work right from starting point */ 
	    for (right=x+1; right<bmp->cr; right++)
	       if (_farnspeekl(addr+right*4) != (unsigned)src_color)
		  break;
	 }
	 else 

      #endif

      {
	 /* check start pixel */
	 if (_farnspeekb(addr+x) != src_color)
	    return x+1;

	 /* work left from starting point */ 
	 for (left=x-1; left>=bmp->cl; left--)
	    if (_farnspeekb(addr+left) != src_color)
	       break;

	 /* work right from starting point */ 
	 for (right=x+1; right<bmp->cr; right++)
	    if (_farnspeekb(addr+right) != src_color)
	       break;
      }
   }
   else {                           /* have to use getpixel() for mode-X */
      /* check start pixel */
      if (getpixel(bmp, x, y) != src_color)
	 return x+1;

      /* work left from starting point */ 
      for (left=x-1; left>=bmp->cl; left--)
	 if (getpixel(bmp, left, y) != src_color)
	    break;

      /* work right from starting point */ 
      for (right=x+1; right<bmp->cr; right++)
	 if (getpixel(bmp, right, y) != src_color)
	    break;
   } 

   left++;
   right--;

   /* draw the line */
   hline(bmp, left, y, right, dest_color);

   /* store it in the list of flooded segments */
   c = y;
   p = FLOOD_LINE(c);

   if (p->flags) {
      while (p->next) {
	 c = p->next;
	 p = FLOOD_LINE(c);
      }

      p->next = c = flood_count++;
      _grow_scratch_mem(sizeof(FLOODED_LINE) * flood_count);
      p = FLOOD_LINE(c);
   }

   p->flags = FLOOD_IN_USE;
   p->lpos = left;
   p->rpos = right;
   p->y = y;
   p->next = 0;

   if (y > bmp->ct)
      p->flags |= FLOOD_TODO_ABOVE;

   if (y+1 < bmp->cb)
      p->flags |= FLOOD_TODO_BELOW;

   return right+2;
}



/* check_flood_line:
 *  Checks a line segment, using the scratch buffer is to store a list of 
 *  segments which have already been drawn in order to minimise the required 
 *  number of tests.
 */
static int check_flood_line(BITMAP *bmp, int y, int left, int right, int src_color, int dest_color)
{
   int c;
   FLOODED_LINE *p;
   int ret = FALSE;

   while (left <= right) {
      c = y;

      do {
	 p = FLOOD_LINE(c);
	 if ((left >= p->lpos) && (left <= p->rpos)) {
	    left = p->rpos+2;
	    goto no_flood;
	 }

	 c = p->next;

      } while (c);

      left = flooder(bmp, left, y, src_color, dest_color);
      ret = TRUE;

      no_flood:
   }

   return ret;
}



/* floodfill:
 *  Fills an enclosed area (starting at point x, y) with the specified color.
 */
void floodfill(BITMAP *bmp, int x, int y, int color)
{
   int src_color;
   int c, done;
   FLOODED_LINE *p;

   /* make sure we have a valid starting point */ 
   if ((x < bmp->cl) || (x >= bmp->cr) || (y < bmp->ct) || (y >= bmp->cb))
      return;

   /* what color to replace? */
   src_color = getpixel(bmp, x, y);
   if (src_color == color)
      return;

   /* set up the list of flooded segments */
   _grow_scratch_mem(sizeof(FLOODED_LINE) * bmp->cb);
   flood_count = bmp->cb;
   p = _scratch_mem;
   for (c=0; c<flood_count; c++) {
      p[c].flags = 0;
      p[c].lpos = SHRT_MAX;
      p[c].rpos = SHRT_MIN;
      p[c].y = y;
      p[c].next = 0;
   }

   /* start up the flood algorithm */
   flooder(bmp, x, y, src_color, color);

   /* continue as long as there are some segments still to test */
   do {
      done = TRUE;

      /* for each line on the screen */
      for (c=0; c<flood_count; c++) {

	 p = FLOOD_LINE(c);

	 /* check below the segment? */
	 if (p->flags & FLOOD_TODO_BELOW) {
	    p->flags &= ~FLOOD_TODO_BELOW;
	    if (check_flood_line(bmp, p->y+1, p->lpos, p->rpos, src_color, color)) {
	       done = FALSE;
	       p = FLOOD_LINE(c);
	    }
	 }

	 /* check above the segment? */
	 if (p->flags & FLOOD_TODO_ABOVE) {
	    p->flags &= ~FLOOD_TODO_ABOVE;
	    if (check_flood_line(bmp, p->y-1, p->lpos, p->rpos, src_color, color)) {
	       done = FALSE;
	       /* special case shortcut for going backwards */
	       if ((c < bmp->cb) && (c > 0))
		  c -= 2;
	    }
	 }
      }

   } while (!done);
}

