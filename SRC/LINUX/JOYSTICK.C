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
 *      Linux joystick routines.
 *
 *      See readme.txt for copyright information.
 */


#ifndef LINUX
#error This file should only be used by the linux version of Allegro
#endif

#include <stdlib.h>
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



/* initialise_joystick:
 *  Calibrates the joystick by reading the axis values when the joystick
 *  is centered. You should call this before using other joystick functions,
 *  and must make sure the joystick is centered when you do so. Returns
 *  non-zero if no joystick is present.
 */
int initialise_joystick()
{
   return -1;
}



/* calibrate_joystick_tl:
 *  For analogue access to the joystick, call this after 
 *  initialise_joystick(), with the joystick at the top left 
 *  extreme, and also call calibrate_joystick_br();
 */
int calibrate_joystick_tl()
{
   return -1;
}



/* calibrate_joystick_br:
 *  For analogue access to the joystick, call this after 
 *  initialise_joystick(), with the joystick at the bottom right 
 *  extreme, and also call calibrate_joystick_tl();
 */
int calibrate_joystick_br()
{
   return -1;
}



/* calibrate_joystick_throttle_min:
 *  For analogue access to the FSPro's throttle, call this after 
 *  initialise_joystick(), with the throttle at the "minimum" extreme
 *  (the user decides whether this is all the way forwards or all the
 *  way back), and also call calibrate_joystick_throttle_max().
 */
int calibrate_joystick_throttle_min()
{
   return -1;
}



/* calibrate_joystick_throttle_max:
 *  For analogue access to the FSPro's throttle, call this after 
 *  initialise_joystick(), with the throttle at the "maximum" extreme
 *  (the user decides whether this is all the way forwards or all the
 *  way back), and also call calibrate_joystick_throttle_min().
 */
int calibrate_joystick_throttle_max()
{
   return -1;
}



/* poll_joystick:
 *  Updates the global joystick position variables. You must call
 *  calibrate_joystick() before using this.
 */
void poll_joystick()
{
}



/* save_joystick_data:
 *  After calibrating a joystick, this function can be used to save the
 *  information into the specified file, from where it can later be 
 *  restored by calling load_joystick_data().
 */
int save_joystick_data(char *filename)
{
   return -1;
}



/* load_joystick_data:
 *  Restores a set of joystick calibration data previously saved by
 *  save_joystick_data().
 */
int load_joystick_data(char *filename)
{
   return -1;
}


