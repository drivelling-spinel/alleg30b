/*         ______   ___    ___
 *        /\  _  \ /\_ \  /\_ \ 
 *        \ \ \L\ \\//\ \ \//\ \      __     __   _ __   ___ 
 *         \ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\/\`'__\/ __`\
 *          \ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \ \ \//\ \L\ \
 *           \ \_\ \_\/\____\/\____\ \____\ \____ \ \_\\ \____/
 *            \/_/\/_/\/____/\/____/\/____/\/___L\ \/_/ \/___/
 *                                           /\____/
 *                                           \_/__/
 *
 *      Hardware interrupt wrapper functions. Unlike the _go32_dpmi_* 
 *      functions, these can deal with reentrant interrupts.
 *
 *      By Shawn Hargreaves.
 *
 *      PIC handling routines added by Ove Kaaven.
 *
 *      See readme.txt for copyright information.
 */


#ifndef DJGPP
#error This file should only be used by the djgpp version of Allegro
#endif

#include <stdlib.h>
#include <stdio.h>
#include <dos.h>
#include <go32.h>
#include <dpmi.h>

#include "internal.h"
#include "asmdefs.inc"


#define MAX_IRQS     8           /* timer + keyboard + soundcard + spares */
#define STACK_SIZE   8*1024      /* an 8k stack should be plenty */

static int pic_virgin = TRUE;

static unsigned char default_pic1;
static unsigned char default_pic2;
static unsigned char altered_pic1;
static unsigned char altered_pic2;

static int irq_virgin = TRUE;

_IRQ_HANDLER _irq_handler[MAX_IRQS];

unsigned char *_irq_stack[IRQ_STACKS];

extern void _irq_wrapper_0(), _irq_wrapper_1(), 
	    _irq_wrapper_2(), _irq_wrapper_3(),
	    _irq_wrapper_4(), _irq_wrapper_5(), 
	    _irq_wrapper_6(), _irq_wrapper_7(),
	    _irq_wrapper_0_end();



/* shutdown_irq:
 *  atexit() routine for freeing the interrupt handler stacks.
 */
static void shutdown_irq()
{
   int c;

   for (c=0; c<IRQ_STACKS; c++) {
      if (_irq_stack[c]) {
	 _irq_stack[c] -= STACK_SIZE - 32;
	 free(_irq_stack[c]);
      }
   }
}



/* _install_irq:
 *  Installs a hardware interrupt handler for the specified irq, allocating
 *  an asm wrapper function which will save registers and handle the stack
 *  switching. The C function should return zero to exit the interrupt with 
 *  an iret instruction, and non-zero to chain to the old handler.
 */
int _install_irq(int num, int (*handler)())
{
   int c;
   __dpmi_paddr addr;

   if (irq_virgin) {                /* first time we've been called? */
      LOCK_VARIABLE(_irq_handler);
      LOCK_VARIABLE(_irq_stack);
      LOCK_FUNCTION(_irq_wrapper_0);

      for (c=0; c<MAX_IRQS; c++) {
	 _irq_handler[c].handler = NULL;
	 _irq_handler[c].number = 0;
      }

      for (c=0; c<IRQ_STACKS; c++) {
	 _irq_stack[c] = malloc(STACK_SIZE);
	 if (_irq_stack[c]) {
	    _go32_dpmi_lock_data(_irq_stack[c], STACK_SIZE);
	    _irq_stack[c] += STACK_SIZE - 32;   /* stacks grow downwards */
	 }
      }

      atexit(shutdown_irq);

      irq_virgin = FALSE;
   }

   for (c=0; c<MAX_IRQS; c++) {
      if (_irq_handler[c].handler == NULL) {

	 addr.selector = _my_cs();

	 switch (c) {
	    case 0: addr.offset32 = (long)_irq_wrapper_0; break;
	    case 1: addr.offset32 = (long)_irq_wrapper_1; break;
	    case 2: addr.offset32 = (long)_irq_wrapper_2; break;
	    case 3: addr.offset32 = (long)_irq_wrapper_3; break;
	    case 4: addr.offset32 = (long)_irq_wrapper_4; break;
	    case 5: addr.offset32 = (long)_irq_wrapper_5; break;
	    case 6: addr.offset32 = (long)_irq_wrapper_6; break;
	    case 7: addr.offset32 = (long)_irq_wrapper_7; break;
	    default: return -1;
	 }

	 _irq_handler[c].handler = handler;
	 _irq_handler[c].number = num;

	 __dpmi_get_protected_mode_interrupt_vector(num, 
						&_irq_handler[c].old_vector);

	 __dpmi_set_protected_mode_interrupt_vector(num, &addr);

	 return 0;
      }
   }

   return -1;
}



/* _remove_irq:
 *  Removes a hardware interrupt handler, restoring the old vector.
 */
void _remove_irq(int num)
{
   int c;

   for (c=0; c<MAX_IRQS; c++) {
      if (_irq_handler[c].number == num) {
	 __dpmi_set_protected_mode_interrupt_vector(num, 
						&_irq_handler[c].old_vector);
	 _irq_handler[c].number = 0;
	 _irq_handler[c].handler = NULL;
	 break;
      }
   }
}



/* exit_irq:
 *  Restores the default hardware interrupt masks.
 */
static void exit_irq(void)
{
   if (!pic_virgin) {
      outportb(0x21, default_pic1);
      outportb(0xA1, default_pic2);
      _remove_exit_func(exit_irq);
      pic_virgin = TRUE;
   }
}



/* init_irq:
 *  Reads the default hardware interrupt masks.
 */
static void init_irq(void)
{
   if (pic_virgin) {
      default_pic1 = inportb(0x21);
      default_pic2 = inportb(0xA1);
      altered_pic1 = 0;
      altered_pic2 = 0;
      _add_exit_func(exit_irq);
      pic_virgin = FALSE;
   }
}



/* _restore_irq:
 *  Restores default masking for a hardware interrupt.
 */
void _restore_irq(int num)
{
   unsigned char pic;

   if (!pic_virgin) {
      if (num>7) {
	 pic = inportb(0xA1) & (~(1<<(num-8)));
	 outportb(0xA1, pic | (default_pic2 & (1<<(num-8))));
	 altered_pic2 &= ~(1<<(num-8));
	 if (altered_pic2) 
	    return;
	 num = 2; /* if no high IRQs remain, also restore Cascade (IRQ2) */
      }
      pic = inportb(0x21) & (~(1<<num));
      outportb(0x21, pic | (default_pic1 & (1<<num)));
      altered_pic1 &= ~(1<<num);
   }
}



/* _enable_irq:
 *  Unmasks a hardware interrupt.
 */
void _enable_irq(int num)
{
   unsigned char pic;

   init_irq();

   pic = inportb(0x21);

   if (num > 7) {
      outportb(0x21, pic & 0xFB);   /* unmask Cascade (IRQ2) interrupt */
      pic = inportb(0xA1);
      outportb(0xA1, pic & (~(1<<(num-8)))); /* unmask PIC-2 interrupt */
      altered_pic2 |= 1<<(num-8);
   } 
   else {
      outportb(0x21, pic & (~(1<<num)));     /* unmask PIC-1 interrupt */
      altered_pic1 |= 1<<num;
   }
}



/* _disable_irq:
 *  Masks a hardware interrupt.
 */
void _disable_irq(int num)
{
   unsigned char pic;

   init_irq();

   if (num > 7) {
      pic = inportb(0xA1);
      outportb(0xA1, pic & (1<<(num-8))); /* mask PIC-2 interrupt */
      altered_pic2 |= 1<<(num-8);
   } 
   else {
      pic = inportb(0x21);
      outportb(0x21, pic & (1<<num));     /* mask PIC-1 interrupt */
      altered_pic1 |= 1<<num;
   }
}

