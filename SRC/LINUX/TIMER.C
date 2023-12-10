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
 *      Linux timer routines. 
 *
 *      See readme.txt for copyright information.
 */


#ifndef LINUX
#error This file should only be used by the linux version of Allegro
#endif

#include <stdlib.h>

#include "allegro.h"
#include "internal.h"


#define MAX_TIMERS      8

int i_love_bill = FALSE;                  /* for compatibility */

static int timer_installed = FALSE;

volatile int retrace_count = 0;           /* vertical retrace counter */
void (*retrace_proc)() = NULL;            /* retrace callback function */

int _timer_use_retrace = FALSE;           /* for compatibility */
volatile int _retrace_hpp_value = -1; 

static struct {
   void ((*proc)());
   long speed;
   long counter;
} my_int_queue[MAX_TIMERS];



static volatile long rest_count;

static void rest_int()
{
   rest_count--;
}

static END_OF_FUNCTION(rest_int);



/* rest:
 *  Waits for time milliseconds.
 */
void rest(long time)
{
   rest_count = time;

   if (install_int(rest_int, 1) < 0)
      return;

   do {
   } while (rest_count > 0);

   remove_int(rest_int);
}



/* timer_calibrate_retrace:
 *  Not supported under linux, present for compatibility.
 */
static void timer_calibrate_retrace()
{
}



/* timer_simulate_retrace:
 *  Not supported under linux, present for compatibility.
 */
void timer_simulate_retrace(int enable)
{
}



/* install_timer:
 *  Installs the timer interrupt handler. You must do this before installing
 *  any user timer routines. You must set up the timer before trying to 
 *  display a mouse pointer or using any of the GUI routines.
 */
int install_timer()
{
   int x;

   if (timer_installed)
      return -1;

   for (x=0; x<MAX_TIMERS; x++) {
      my_int_queue[x].proc = NULL;
      my_int_queue[x].speed = 0;
      my_int_queue[x].counter = 0;
   }

   /* do some stuff */

   _add_exit_func(remove_timer);
   timer_installed = TRUE;
   return 0;
}



/* remove_timer:
 *  Removes our timer handler and resets the BIOS clock. You don't normally
 *  need to call this, because allegro_exit() will do it for you.
 */
void remove_timer()
{
   if (!timer_installed)
      return;

   /* do some stuff */

   _remove_exit_func(remove_timer);
   timer_installed = FALSE;
}



/* find_timer_slot:
 *  Searches the list of user timer callbacks for a specified function, 
 *  returning the position at which it was found, or -1 if it isn't there.
 */
static int find_timer_slot(void (*proc)())
{
   int x;

   for (x=0; x<MAX_TIMERS; x++)
      if (my_int_queue[x].proc == proc)
	 return x;

   return -1;
}



/* install_int_ex:
 *  Installs a function into the list of user timers, or if it is already 
 *  installed, adjusts its speed. This function will be called once every 
 *  speed timer ticks. Returns a negative number if there was no room to 
 *  add a new routine (there is only room for eight). Note that your 
 *  routine is called by the Allegro interrupt handler and not directly 
 *  by the processor, so you should return normally rather than using an 
 *  iret. Your interrupt routine must finish quickly, and you should not 
 *  use large amounts of stack or make any calls to the operating system. 
 */
int install_int_ex(void (*proc)(), long speed)
{
   int x = find_timer_slot(proc);      /* find the handler position */

   if (x < 0)                          /* if not there, find free slot */
      x = find_timer_slot(NULL);

   if (x < 0)                          /* are there any free slots? */
      return -1;

   if (proc != my_int_queue[x].proc) {
      my_int_queue[x].counter = speed;
      my_int_queue[x].proc = proc; 
   }
   else
      my_int_queue[x].counter += speed - my_int_queue[x].speed;

   my_int_queue[x].speed = speed;

   return 0;
}



/* install_int:
 *  Wrapper for install_int_ex, which takes the speed in milliseconds.
 */
int install_int(void (*proc)(), long speed)
{
   return install_int_ex(proc, MSEC_TO_TIMER(speed));
}



/* remove_int:
 *  Removes a function from the list of user timers.
 */
void remove_int(void (*proc)())
{
   int x = find_timer_slot(proc);

   if (x >= 0) {
      my_int_queue[x].proc = NULL;
      my_int_queue[x].speed = 0;
      my_int_queue[x].counter = 0;
   }
}



