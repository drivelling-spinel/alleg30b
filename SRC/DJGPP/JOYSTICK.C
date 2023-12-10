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
 *      DOS joystick routines (the actual polling function is in misc.s).
 *
 *      Based on code provided by Jonathan Tarbox and Marcel de Kogel.
 *
 *      CH Flightstick Pro and Logitech Wingman Extreme
 *      support by Fabian Nunez.
 *
 *      Matthew Bowie added support for 4-button joysticks.
 *
 *      Richard Mitton added support for 6-button joysticks.
 *
 *      Stefan Eilert added support for dual joysticks.
 *
 *      See readme.txt for copyright information.
 */


#ifndef DJGPP
#error This file should only be used by the djgpp version of Allegro
#endif

#include <stdlib.h>
#include <dos.h>
#include <errno.h>

#include "allegro.h"
#include "internal.h"


/* global joystick position variables */
int joy_x = 0;
int joy_y = 0;
int joy_left = FALSE;
int joy_right = FALSE;
int joy_up = FALSE;
int joy_down = FALSE;
int joy_b1 = FALSE;
int joy_b2 = FALSE;

int joy_type = JOY_TYPE_STANDARD;

/* CH Flightstick Pro variables */
int joy_b3 = FALSE;
int joy_b4 = FALSE;
int joy_hat = JOY_HAT_CENTRE;
int joy_throttle = 0;

/* 6-button variables */
int joy_b5 = FALSE;
int joy_b6 = FALSE;

/* dual joystick variables */
int joy2_x = 0;
int joy2_y = 0;
int joy2_left = FALSE;
int joy2_right = FALSE;
int joy2_up = FALSE;
int joy2_down = FALSE;
int joy2_b1 = FALSE;
int joy2_b2 = FALSE;

/* joystick state information */
#define JOYSTICK_PRESENT             1
#define JOYSTICK_CALIB_TL            2
#define JOYSTICK_CALIB_BR            4
#define JOYSTICK_CALIB_THRTL_MIN     8
#define JOYSTICK_CALIB_THRTL_MAX    16
#define JOYSTICK_CALIB_HAT_CENTRE   32
#define JOYSTICK_CALIB_HAT_LEFT     64
#define JOYSTICK_CALIB_HAT_DOWN    128 
#define JOYSTICK_CALIB_HAT_RIGHT   256
#define JOYSTICK_CALIB_HAT_UP      512

#define JOYSTICK_CALIB_HAT    (JOYSTICK_CALIB_HAT_CENTRE |     \
			       JOYSTICK_CALIB_HAT_LEFT |       \
			       JOYSTICK_CALIB_HAT_DOWN |       \
			       JOYSTICK_CALIB_HAT_RIGHT |      \
			       JOYSTICK_CALIB_HAT_UP)

static int joystick_flags = 0;

/* calibrated position values */
static int joycentre_x, joycentre_y;
static int joyx_min, joyx_low_margin, joyx_high_margin, joyx_max;
static int joyy_min, joyy_low_margin, joyy_high_margin, joyy_max;

static int joycentre2_x, joycentre2_y;
static int joyx2_min, joyx2_low_margin, joyx2_high_margin, joyx2_max;
static int joyy2_min, joyy2_low_margin, joyy2_high_margin, joyy2_max;

static int joy_thr_min, joy_thr_max;

static int joy_hat_pos[5], joy_hat_threshold[4];

/* for filtering out bad input values */
static int joy_old_x, joy_old_y;
static int joy2_old_x, joy2_old_y;


#define MASK_1X      1
#define MASK_1Y      2
#define MASK_2X      4
#define MASK_2Y      8
#define MASK_PLAIN   (MASK_1X | MASK_1Y)
#define MASK_ALL     (MASK_1X | MASK_1Y | MASK_2X | MASK_2Y)
#define MASK_CALIB   ((joy_type == JOY_TYPE_2PADS) ? MASK_ALL : MASK_PLAIN)



/* poll_mask:
 *  Returns a mask indicating which axes to poll.
 */
static int poll_mask()
{
   switch (joy_type) {

      case JOY_TYPE_6BUTTON:
	 return MASK_PLAIN | MASK_2X | MASK_2Y;

      case JOY_TYPE_FSPRO:
      case JOY_TYPE_WINGEX:
	 return MASK_PLAIN | MASK_2Y;

      case JOY_TYPE_2PADS:
	 return MASK_ALL;

      default:
	 return MASK_PLAIN;
   }
}



/* poll:
 *  Wrapper function for joystick polling.
 */
static int poll(int *x, int *y, int *x2, int *y2, int poll_mask)
{
   int ret;

   enter_critical();

   ret = _poll_joystick(x, y, x2, y2, poll_mask);

   exit_critical();

   return ret;
}



/* averaged_poll:
 *  For calibration it is crucial that we get the right results, so we
 *  average out several attempts.
 */
static int averaged_poll(int *x, int *y, int *x2, int *y2, int mask)
{
   #define AVERAGE_COUNT   4

   int x_tmp, y_tmp, x2_tmp, y2_tmp;
   int x_total, y_total, x2_total, y2_total;
   int c;

   x_total = y_total = x2_total = y2_total = 0;

   for (c=0; c<AVERAGE_COUNT; c++) {
      if (poll(&x_tmp, &y_tmp, &x2_tmp, &y2_tmp, mask) != 0)
	 return -1;

      x_total += x_tmp;
      y_total += y_tmp;
      x2_total += x2_tmp;
      y2_total += y2_tmp;
   }

   *x = x_total / AVERAGE_COUNT;
   *y = y_total / AVERAGE_COUNT;
   *x2 = x2_total / AVERAGE_COUNT;
   *y2 = y2_total / AVERAGE_COUNT;

   return 0;
}



/* initialise_joystick:
 *  Calibrates the joystick by reading the axis values when the joystick
 *  is centered. You should call this before using other joystick functions,
 *  and must make sure the joystick is centered when you do so. Returns
 *  non-zero if no joystick is present.
 */
int initialise_joystick()
{
   joy_old_x = joy_old_y = 0;
   joy2_old_x = joy2_old_y = 0;

   if (averaged_poll(&joycentre_x, &joycentre_y, &joycentre2_x, &joycentre2_y, poll_mask()) != 0) {
      joy_x = joy_y = 0;
      joy_left = joy_right = joy_up = joy_down = FALSE;
      joy_b1 = joy_b2 = joy_b3 = joy_b4 = joy_b5 = joy_b6 = FALSE;
      joy2_x = joy2_y = 0;
      joy2_left = joy2_right = joy2_up = joy2_down = FALSE;
      joy2_b1 = joy2_b2 = FALSE;
      joy_hat = JOY_HAT_CENTRE;
      joy_throttle = 0;
      joystick_flags = 0;
      return -1;
   }

   joystick_flags = JOYSTICK_PRESENT;

   return 0;
}



/* sort_out_middle_values:
 *  Sets up the values used by sort_out_analogue() to create a 'dead'
 *  region in the centre of the joystick range.
 */
static void sort_out_middle_values()
{
   joyx_low_margin  = joycentre_x - (joycentre_x - joyx_min) / 8;
   joyx_high_margin = joycentre_x + (joyx_max - joycentre_x) / 8;
   joyy_low_margin  = joycentre_y - (joycentre_y - joyy_min) / 8;
   joyy_high_margin = joycentre_y + (joyy_max - joycentre_y) / 8;

   if (joy_type == JOY_TYPE_2PADS) {
      joyx2_low_margin  = joycentre2_x - (joycentre2_x - joyx2_min) / 8;
      joyx2_high_margin = joycentre2_x + (joyx2_max - joycentre2_x) / 8;
      joyy2_low_margin  = joycentre2_y - (joycentre2_y - joyy2_min) / 8;
      joyy2_high_margin = joycentre2_y + (joyy2_max - joycentre2_y) / 8;
   }
}



/* calibrate_joystick_tl:
 *  For analogue access to the joystick, call this after 
 *  initialise_joystick(), with the joystick at the top left 
 *  extreme, and also call calibrate_joystick_br();
 */
int calibrate_joystick_tl()
{
   if (!(joystick_flags & JOYSTICK_PRESENT)) {
      joystick_flags &= (~JOYSTICK_CALIB_TL);
      return -1;
   }

   if (averaged_poll(&joyx_min, &joyy_min, &joyx2_min, &joyy2_min, MASK_CALIB) != 0) {
      joystick_flags &= (~JOYSTICK_CALIB_TL);
      return -1;
   }

   if (joystick_flags & JOYSTICK_CALIB_BR)
      sort_out_middle_values();

   joystick_flags |= JOYSTICK_CALIB_TL;

   return 0;
}



/* calibrate_joystick_br:
 *  For analogue access to the joystick, call this after 
 *  initialise_joystick(), with the joystick at the bottom right 
 *  extreme, and also call calibrate_joystick_tl();
 */
int calibrate_joystick_br()
{
   if (!(joystick_flags & JOYSTICK_PRESENT)) {
      joystick_flags &= (~JOYSTICK_CALIB_BR);
      return -1;
   }

   if (averaged_poll(&joyx_max, &joyy_max, &joyx2_max, &joyy2_max, MASK_CALIB) != 0) {
      joystick_flags &= (~JOYSTICK_CALIB_BR);
      return -1;
   }

   if (joystick_flags & JOYSTICK_CALIB_TL)
      sort_out_middle_values();

   joystick_flags |= JOYSTICK_CALIB_BR;

   return 0;
}



/* calibrate_joystick_throttle_min:
 *  For analogue access to the FSPro's throttle, call this after 
 *  initialise_joystick(), with the throttle at the "minimum" extreme
 *  (the user decides whether this is all the way forwards or all the
 *  way back), and also call calibrate_joystick_throttle_max().
 */
int calibrate_joystick_throttle_min()
{
   int dummy;

   if (!(joystick_flags & JOYSTICK_PRESENT)) {
      joystick_flags &= (~JOYSTICK_CALIB_THRTL_MIN);
      return -1;
   }

   if (averaged_poll(&dummy, &dummy, &dummy, &joy_thr_min, MASK_2Y) != 0) {
      joystick_flags &= (~JOYSTICK_CALIB_THRTL_MIN);
      return -1;
   }

   joystick_flags |= JOYSTICK_CALIB_THRTL_MIN;

   /* prevent division by zero errors if user miscalibrated */
   if ((joystick_flags & JOYSTICK_CALIB_THRTL_MAX) &&
       (joy_thr_min == joy_thr_max))
     joy_thr_min = 255 - joy_thr_max;

   return 0;
}



/* calibrate_joystick_throttle_max:
 *  For analogue access to the FSPro's throttle, call this after 
 *  initialise_joystick(), with the throttle at the "maximum" extreme
 *  (the user decides whether this is all the way forwards or all the
 *  way back), and also call calibrate_joystick_throttle_min().
 */
int calibrate_joystick_throttle_max()
{
   int dummy;

   if (!(joystick_flags & JOYSTICK_PRESENT)) {
      joystick_flags &= (~JOYSTICK_CALIB_THRTL_MAX);
      return -1;
   }

   if (averaged_poll(&dummy, &dummy, &dummy, &joy_thr_max, MASK_2Y) != 0) {
      joystick_flags &= (~JOYSTICK_CALIB_THRTL_MAX);
      return -1;
   }

   joystick_flags |= JOYSTICK_CALIB_THRTL_MAX;

   /* prevent division by zero errors if user miscalibrated */
   if ((joystick_flags & JOYSTICK_CALIB_THRTL_MIN) &&
       (joy_thr_min == joy_thr_max))
     joy_thr_max = 255 - joy_thr_min;

   return 0;
}



/* calibrate_joystick_hat:
 *  For access to the Wingman Extreme's hat (I this will also work on all
 *  Thrustmaster compatible joysticks), call this after initialise_joystick(), 
 *  passing the JOY_HAT constant you wish to calibrate.
 */
int calibrate_joystick_hat(int direction)
{
   static int pos_const[] = 
   { 
      JOYSTICK_CALIB_HAT_CENTRE,
      JOYSTICK_CALIB_HAT_LEFT,
      JOYSTICK_CALIB_HAT_DOWN,
      JOYSTICK_CALIB_HAT_RIGHT,
      JOYSTICK_CALIB_HAT_UP
   };

   int dummy, value;

   if ((direction > JOY_HAT_UP) || !(joystick_flags & JOYSTICK_PRESENT))
      return -1;

   if (averaged_poll(&dummy, &dummy, &dummy, &value, MASK_2Y) != 0) {
      joystick_flags &= ~(pos_const[direction]);
      return -1;
   }

   joy_hat_pos[direction] = value;
   joystick_flags |= pos_const[direction];

   /* when all directions have been calibrated, calculate deltas */
   if ((joystick_flags & JOYSTICK_CALIB_HAT) == JOYSTICK_CALIB_HAT) {
      joy_hat_threshold[0] = (joy_hat_pos[0] + joy_hat_pos[1]) / 2;
      joy_hat_threshold[1] = (joy_hat_pos[1] + joy_hat_pos[2]) / 2;
      joy_hat_threshold[2] = (joy_hat_pos[2] + joy_hat_pos[3]) / 2;
      joy_hat_threshold[3] = (joy_hat_pos[3] + joy_hat_pos[4]) / 2;
   }

   return 0;
}



/* sort_out_analogue:
 *  There are a couple of problems with reading analogue input from the PC
 *  joystick. For one thing, joysticks tend not to centre repeatably, so
 *  we need a small 'dead' zone in the middle. Also a lot of joysticks aren't
 *  linear, so the positions less than centre need to be handled differently
 *  to those above the centre.
 */
static int sort_out_analogue(int x, int min, int low_margin, int high_margin, int max)
{
   if (x < min) {
      return -128;
   }
   else if (x < low_margin) {
      return -128 + (x - min) * 128 / (low_margin - min);
   }
   else if (x <= high_margin) {
      return 0;
   }
   else if (x <= max) {
      return 128 - (max - x) * 128 / (max - high_margin);
   }
   else
      return 128;
}



/* poll_joystick:
 *  Updates the global joystick position variables. You must call
 *  calibrate_joystick() before using this.
 */
void poll_joystick()
{
   int x, y, x2, y2;
   unsigned char status;

   if (joystick_flags & JOYSTICK_PRESENT) {
      /* read the hardware */
      poll(&x, &y, &x2, &y2, poll_mask());

      status = inportb(0x201);

      /* stick 1 position */
      if ((ABS(x-joy_old_x) < x/4) && (ABS(y-joy_old_y) < y/4)) {
	 if ((joystick_flags & JOYSTICK_CALIB_TL) && 
	     (joystick_flags & JOYSTICK_CALIB_BR)) {
	    joy_x = sort_out_analogue(x, joyx_min, joyx_low_margin, joyx_high_margin, joyx_max);
	    joy_y = sort_out_analogue(y, joyy_min, joyy_low_margin, joyy_high_margin, joyy_max);
	 }
	 else {
	    joy_x = x - joycentre_x;
	    joy_y = y - joycentre_y;
	 }

	 joy_left  = (x < (joycentre_x/2));
	 joy_right = (x > (joycentre_x*3/2));
	 joy_up    = (y < (joycentre_y/2));
	 joy_down  = (y > ((joycentre_y*3)/2));
      }

      if (joy_type == JOY_TYPE_2PADS) {
	 /* stick 2 position */
	 if ((ABS(x2-joy2_old_x) < x2/4) && (ABS(y2-joy2_old_y) < y2/4)) {
	    if ((joystick_flags & JOYSTICK_CALIB_TL) && 
		(joystick_flags & JOYSTICK_CALIB_BR)) {
	       joy2_x = sort_out_analogue(x2, joyx2_min, joyx2_low_margin, joyx2_high_margin, joyx2_max);
	       joy2_y = sort_out_analogue(y2, joyy2_min, joyy2_low_margin, joyy2_high_margin, joyy2_max);
	    }
	    else {
	       joy2_x = x2 - joycentre2_x;
	       joy2_y = y2 - joycentre2_y;
	    }

	    joy2_left  = (x2 < (joycentre2_x/2));
	    joy2_right = (x2 > (joycentre2_x*3/2));
	    joy2_up    = (y2 < (joycentre2_y/2));
	    joy2_down  = (y2 > ((joycentre2_y*3)/2));
	 }
      }

      /* button state */
      joy_b1 = ((status & 0x10) == 0);
      joy_b2 = ((status & 0x20) == 0);

      if (joy_type == JOY_TYPE_FSPRO) {
	 /* throttle */
	 if ((joystick_flags & JOYSTICK_CALIB_THRTL_MIN) && 
	     (joystick_flags & JOYSTICK_CALIB_THRTL_MAX)) {
	    joy_throttle = (y2 - joy_thr_min) * 255 / (joy_thr_max - joy_thr_min);
	    if (joy_throttle < 0) joy_throttle = 0;
	    if (joy_throttle > 255) joy_throttle = 255;
	 } 

	 /* extra buttons */
	 joy_b3 = ((status & 0x40) == 0);
	 joy_b4 = ((status & 0x80) == 0);

	 /* hat */
	 if ((status & 0x30) == 0) {
	    joy_b1 = joy_b2 = joy_b3 = joy_b4 = FALSE;

	    joy_hat = 1 + ((status ^ 0xFF) >> 6);
	 }
	 else
	    joy_hat = JOY_HAT_CENTRE;
      } 
      else if (joy_type == JOY_TYPE_WINGEX) {
	 /* read the y2 pot to get the position of the hat */
	 if ((joystick_flags & JOYSTICK_CALIB_HAT) == JOYSTICK_CALIB_HAT) {
	    if (y2 >= joy_hat_threshold[0])
	       joy_hat = JOY_HAT_CENTRE;
	    else if (y2 >= joy_hat_threshold[1])
	       joy_hat = JOY_HAT_LEFT;
	    else if (y2 >= joy_hat_threshold[2])
	       joy_hat = JOY_HAT_DOWN;
	    else if (y2 >= joy_hat_threshold[3])
	       joy_hat = JOY_HAT_RIGHT;
	    else 
	       joy_hat = JOY_HAT_UP;
	 } 
	 else 
	    joy_hat = JOY_HAT_CENTRE;

	 /* extra buttons */
	 joy_b3 = ((status & 0x40) == 0);
	 joy_b4 = ((status & 0x80) == 0);
      } 
      else if (joy_type == JOY_TYPE_4BUTTON) {
	 /* four button state */
	 joy_b3 = ((status & 0x40) == 0);
	 joy_b4 = ((status & 0x80) == 0);
      }
      else if (joy_type == JOY_TYPE_6BUTTON) {
	 /* six button state */
	 joy_b3 = ((status & 0x40) == 0);
	 joy_b4 = ((status & 0x80) == 0);
	 joy_b5 = (x2 < 128);
	 joy_b6 = (y2 < 128);
      }
      else if (joy_type == JOY_TYPE_2PADS) {
	 /* second pad button state */
	 joy2_b1 = ((status & 0x40) == 0);
	 joy2_b2 = ((status & 0x80) == 0);
      }

      joy_old_x = x;
      joy_old_y = y;

      joy2_old_x = x2;
      joy2_old_y = y2;
   }
}



/* save_joystick_data:
 *  After calibrating a joystick, this function can be used to save the
 *  information into the specified file, from where it can later be 
 *  restored by calling load_joystick_data().
 */
int save_joystick_data(char *filename)
{
   if (filename) {
      push_config_state();
      set_config_file(filename);
   }

   set_config_int("joystick", "joytype",           joy_type);
   set_config_int("joystick", "joystick_flags",    joystick_flags);

   set_config_int("joystick", "joycentre_x",       joycentre_x);
   set_config_int("joystick", "joycentre_y",       joycentre_y);
   set_config_int("joystick", "joyx_min",          joyx_min);
   set_config_int("joystick", "joyx_low_margin",   joyx_low_margin);
   set_config_int("joystick", "joyx_high_margin",  joyx_high_margin);
   set_config_int("joystick", "joyx_max",          joyx_max);
   set_config_int("joystick", "joyy_min",          joyy_min);
   set_config_int("joystick", "joyy_low_margin",   joyy_low_margin);
   set_config_int("joystick", "joyy_high_margin",  joyy_high_margin);
   set_config_int("joystick", "joyy_max",          joyy_max);

   set_config_int("joystick", "joycentre2_x",      joycentre2_x);
   set_config_int("joystick", "joycentre2_y",      joycentre2_y);
   set_config_int("joystick", "joyx2_min",         joyx2_min);
   set_config_int("joystick", "joyx2_low_margin",  joyx2_low_margin);
   set_config_int("joystick", "joyx2_high_margin", joyx2_high_margin);
   set_config_int("joystick", "joyx2_max",         joyx2_max);
   set_config_int("joystick", "joyy2_min",         joyy2_min);
   set_config_int("joystick", "joyy2_low_margin",  joyy2_low_margin);
   set_config_int("joystick", "joyy2_high_margin", joyy2_high_margin);
   set_config_int("joystick", "joyy2_max",         joyy2_max);

   set_config_int("joystick", "joythr_min",        joy_thr_min);
   set_config_int("joystick", "joythr_max",        joy_thr_max);

   set_config_int("joystick", "joyhat_0",          joy_hat_threshold[0]);
   set_config_int("joystick", "joyhat_1",          joy_hat_threshold[1]);
   set_config_int("joystick", "joyhat_2",          joy_hat_threshold[2]);
   set_config_int("joystick", "joyhat_3",          joy_hat_threshold[3]);

   if (filename)
      pop_config_state();

   return 0;
}



/* load_joystick_data:
 *  Restores a set of joystick calibration data previously saved by
 *  save_joystick_data().
 */
int load_joystick_data(char *filename)
{
   if (filename) {
      push_config_state();
      set_config_file(filename);
   }

   joy_type = get_config_int("joystick", "joytype", -1);
   if (joy_type == -1) {
      joystick_flags = 0;
      return -1;
   }

   joystick_flags    = get_config_int("joystick", "joystick_flags", 0);

   joycentre_x       = get_config_int("joystick", "joycentre_x", 0);
   joycentre_y       = get_config_int("joystick", "joycentre_y", 0);
   joyx_min          = get_config_int("joystick", "joyx_min", 0);
   joyx_low_margin   = get_config_int("joystick", "joyx_low_margin", 0);
   joyx_high_margin  = get_config_int("joystick", "joyx_high_margin", 0);
   joyx_max          = get_config_int("joystick", "joyx_max", 0);
   joyy_min          = get_config_int("joystick", "joyy_min", 0);
   joyy_low_margin   = get_config_int("joystick", "joyy_low_margin", 0);
   joyy_high_margin  = get_config_int("joystick", "joyy_high_margin", 0);
   joyy_max          = get_config_int("joystick", "joyy_max", 0);

   joycentre2_x      = get_config_int("joystick", "joycentre2_x", 0);
   joycentre2_y      = get_config_int("joystick", "joycentre2_y", 0);
   joyx2_min         = get_config_int("joystick", "joyx2_min", 0);
   joyx2_low_margin  = get_config_int("joystick", "joyx_low2_margin", 0);
   joyx2_high_margin = get_config_int("joystick", "joyx_high2_margin", 0);
   joyx2_max         = get_config_int("joystick", "joyx2_max", 0);
   joyy2_min         = get_config_int("joystick", "joyy2_min", 0);
   joyy2_low_margin  = get_config_int("joystick", "joyy2_low_margin", 0);
   joyy2_high_margin = get_config_int("joystick", "joyy2_high_margin", 0);
   joyy2_max         = get_config_int("joystick", "joyy2_max", 0);

   joy_thr_min       = get_config_int("joystick", "joythr_min", 0);
   joy_thr_max       = get_config_int("joystick", "joythr_max", 0);

   joy_hat_threshold[0] = get_config_int("joystick", "joyhat_0", 0);
   joy_hat_threshold[1] = get_config_int("joystick", "joyhat_1", 0);
   joy_hat_threshold[2] = get_config_int("joystick", "joyhat_2", 0);
   joy_hat_threshold[3] = get_config_int("joystick", "joyhat_3", 0);

   joy_old_x = joy_old_y = 0;

   if (filename)
      pop_config_state();

   return 0;
}


