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
 *      Bezier spline plotter, contributed by Seymour Shlien.
 *
 *      We use the Castelau Algorithm described in the book:
 * 
 *      Interactive Computer Graphics
 *      by Peter Burger and Duncan Gillies
 *      Addison-Wesley Publishing Co 1989
 *      ISBN 0-201-17439-1
 * 
 *      Method: Given a ratio mu, a recursion replaces the
 *      4 knots with 3 knots, where the position of the 3
 *      knots depend on mu. Repeat again reducing 3 knots to
 *      2 and one more time.
 * 
 *      The 4 th order Bezier curve is a cubic curve passing
 *      through the first and fourth point. The curve does
 *      not pass through the middle two points. They are merely
 *      guide points which control the shape of the curve. The
 *      curve is tangent to the lines joining points 1 and 2
 *      and points 3 and 4.
 *
 *      See readme.txt for copyright information.
 */


#include <stdlib.h>

#include "allegro.h"
#include "internal.h"



/* bez_split:
 *  Calculates a weighted average between x1 and x2.
 */
static fixed bez_split(fixed mu, fixed x1, fixed x2)
{
   return fmul((itofix(1.0)-mu), x1) + fmul(mu, x2);
}



/* bezval:
 *  Calculates a point on a bezier curve.
 */
static fixed bezval(fixed mu, int *coor)
{
   fixed work[4];
   int i; 
   int j;

   for (i=0; i<4; i++) 
      work[i] = itofix(coor[i*2]);

   for (j=0; j<3; j++)
      for (i=0; i<3-j; i++)
	 work[i] = bez_split(mu, work[i], work[i+1]);

   return work[0];
}



/* calc_spline:
 *  Calculates a set of pixels for the bezier spline defined by the four
 *  points specified in the points array. The required resolution
 *  is specified by the npts parameter, which controls how many output
 *  pixels will be stored in the x and y arrays.
 */
void calc_spline(int points[8], int npts, int *x, int *y)
{
   int i;
   fixed denom;

   for (i=0; i<npts; i++) {
      denom = fdiv(itofix(i), itofix(npts-1));
      x[i] = fixtoi(bezval(denom, points));
      y[i] = fixtoi(bezval(denom, points+1));
   }
}



/* spline:
 *  Draws a bezier spline onto the specified bitmap in the specified color.
 */
void spline(BITMAP *bmp, int points[8], int color)
{
   #define NPTS   64

   int xpts[NPTS], ypts[NPTS];
   int i;

   calc_spline(points, NPTS, xpts, ypts);

   for (i=1; i<NPTS; i++) {
      line(bmp, xpts[i-1], ypts[i-1], xpts[i], ypts[i], color);

      if (_drawing_mode == DRAW_MODE_XOR)
	 putpixel(bmp, xpts[i], ypts[i], color);
   }
}
