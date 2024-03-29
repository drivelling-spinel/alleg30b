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
 *      32 bit bitmap blitting (written for speed, not readability :-)
 *
 *      See readme.txt for copyright information.
 */


#include "asmdefs.inc"
#include "blit.inc"

#ifdef ALLEGRO_COLOR32

.text



/* void _linear_clear_to_color32(BITMAP *bitmap, int color);
 *  Fills a linear bitmap with the specified color.
 */
.globl __linear_clear_to_color32 
   .align 4
__linear_clear_to_color32:
   pushl %ebp
   movl %esp, %ebp
   pushl %ebx
   pushl %esi
   pushl %edi
   pushw %es 

   movl ARG1, %edx               /* edx = bmp */
   movl BMP_CT(%edx), %ebx       /* line to start at */

   movw BMP_SEG(%edx), %es       /* select segment */

   movl BMP_CR(%edx), %esi       /* width to clear */
   subl BMP_CL(%edx), %esi
   cld

   .align 4, 0x90
clear_loop:
   movl %ebx, %eax
   WRITE_BANK()                  /* select bank */
   movl BMP_CL(%edx), %edi 
   leal (%eax, %edi, 4), %edi    /* get line address  */

   movl ARG2, %eax 
   movl %esi, %ecx 
   rep ; stosl                   /* clear the line */

   incl %ebx
   cmpl %ebx, BMP_CB(%edx)
   jg clear_loop                 /* and loop */

   popw %es
   popl %edi
   popl %esi
   popl %ebx
   movl %ebp, %esp
   popl %ebp
   ret                           /* end of _linear_clear_to_color() */




/* void _linear_blit32(BITMAP *source, BITMAP *dest, int source_x, source_y, 
 *                                     int dest_x, dest_y, int width, height);
 *  Normal forwards blitting routine for linear bitmaps.
 */
.globl __linear_blit32 
   .align 4
__linear_blit32:
   pushl %ebp
   movl %esp, %ebp
   pushw %es 
   pushl %edi
   pushl %esi
   pushl %ebx

   movl B_DEST, %edx
   movw BMP_SEG(%edx), %es       /* load destination segment */
   movw %ds, %bx                 /* save data segment selector */
   cld                           /* for forward copy */

   .align 4, 0x90
   BLIT_LOOP(blitter, 4,         /* copy the data */
      rep ; movsl
   )

   popl %ebx
   popl %esi
   popl %edi
   popw %es
   movl %ebp, %esp
   popl %ebp
   ret                           /* end of _linear_blit32() */




/* void _linear_blit_backward32(BITMAP *source, BITMAP *dest, int source_x, 
 *                      int source_y, int dest_x, dest_y, int width, height);
 *  Reverse blitting routine, for overlapping linear bitmaps.
 */
.globl __linear_blit_backward32
   .align 4
__linear_blit_backward32:
   pushl %ebp
   movl %esp, %ebp
   pushw %es 
   pushl %edi
   pushl %esi
   pushl %ebx

   movl B_HEIGHT, %eax           /* y values go from high to low */
   decl %eax
   addl %eax, B_SOURCE_Y
   addl %eax, B_DEST_Y

   movl B_WIDTH, %eax            /* x values go from high to low */
   decl %eax
   addl %eax, B_SOURCE_X
   addl %eax, B_DEST_X

   movl B_DEST, %edx
   movw BMP_SEG(%edx), %es       /* load destination segment */
   movw %ds, %bx                 /* save data segment selector */

   .align 4
blit_backwards_loop:
   movl B_DEST, %edx             /* destination bitmap */
   movl B_DEST_Y, %eax           /* line number */
   WRITE_BANK()                  /* select bank */
   movl B_DEST_X, %edi           /* x offset */
   leal (%eax, %edi, 4), %edi

   movl B_SOURCE, %edx           /* source bitmap */
   movl B_SOURCE_Y, %eax         /* line number */
   READ_BANK()                   /* select bank */
   movl B_SOURCE_X, %esi         /* x offset */
   leal (%eax, %esi, 4), %esi

   movl B_WIDTH, %ecx            /* x loop counter */
   movw BMP_SEG(%edx), %ds       /* load data segment */
   std                           /* backwards */
   rep ; movsl                   /* copy the line */

   movw %bx, %ds                 /* restore data segment */
   decl B_SOURCE_Y
   decl B_DEST_Y
   decl B_HEIGHT
   jg blit_backwards_loop        /* and loop */

   cld                           /* finished */

   popl %ebx
   popl %esi
   popl %edi
   popw %es
   movl %ebp, %esp
   popl %ebp
   ret                           /* end of _linear_blit_backward32() */



.globl __linear_blit32_end
   .align 4
__linear_blit32_end:
   ret




/* void _linear_masked_blit32(BITMAP *source, *dest, int source_x, source_y, 
 *                            int dest_x, dest_y, int width, height);
 *  Masked (skipping zero pixels) blitting routine for linear bitmaps.
 */
.globl __linear_masked_blit32
   .align 4
__linear_masked_blit32:
   pushl %ebp
   movl %esp, %ebp
   pushw %es 
   pushl %edi
   pushl %esi
   pushl %ebx

   movl B_DEST, %edx
   movw BMP_SEG(%edx), %es 
   movw %ds, %bx 
   cld 

   .align 4, 0x90
   BLIT_LOOP(masked, 4,

      .align 4, 0x90
   masked_blit_x_loop:
      movl (%esi), %eax          /* read a byte */
      addl $4, %esi

      cmpl $MASK_COLOR_32, %eax  /* test it */
      je masked_blit_skip

      movl %eax, %es:(%edi)      /* write the pixel */
      addl $4, %edi
      decl %ecx
      jg masked_blit_x_loop
      jmp masked_blit_x_loop_done

      .align 4, 0x90
   masked_blit_skip:
      addl $4, %edi              /* skip zero pixels */
      decl %ecx
      jg masked_blit_x_loop

   masked_blit_x_loop_done:
   )

   popl %ebx
   popl %esi
   popl %edi
   popw %es
   movl %ebp, %esp
   popl %ebp
   ret                           /* end of _linear_masked_blit32() */




#endif      /* ifdef ALLEGRO_COLOR32 */

