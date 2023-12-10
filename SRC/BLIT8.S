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
 *      256 color bitmap blitting (written for speed, not readability :-)
 *
 *      See readme.txt for copyright information.
 */


#include "asmdefs.inc"
#include "blit.inc"

.text



/* void _linear_clear_to_color8(BITMAP *bitmap, int color);
 *  Fills a linear bitmap with the specified color.
 */
.globl __linear_clear_to_color8 
   .align 4
__linear_clear_to_color8:
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
   movl %eax, %edi 
   addl BMP_CL(%edx), %edi       /* get line address  */

   movb ARG2, %al                /* duplicate color 4 times */
   movb %al, %ah
   shll $16, %eax
   movb ARG2, %al 
   movb %al, %ah

   movl %esi, %ecx               /* width to clear */
   shrl $1, %ecx                 /* halve for 16 bit clear */
   jnc clear_no_byte
   stosb                         /* clear an odd byte */

clear_no_byte:
   shrl $1, %ecx                 /* halve again for 32 bit clear */
   jnc clear_no_word
   stosw                         /* clear an odd word */

clear_no_word:
   jz clear_no_long 

   .align 4, 0x90
clear_x_loop:
   rep ; stosl                   /* clear the line */

clear_no_long:
   incl %ebx
   cmpl %ebx, BMP_CB(%edx)
   jg clear_loop                 /* and loop */

   popw %es
   popl %edi
   popl %esi
   popl %ebx
   movl %ebp, %esp
   popl %ebp
   ret                           /* end of _linear_clear_to_color8() */




/* void _linear_blit8(BITMAP *source, BITMAP *dest, int source_x, source_y, 
 *                                    int dest_x, dest_y, int width, height);
 *  Normal forwards blitting routine for linear bitmaps.
 */
.globl __linear_blit8 
   .align 4
__linear_blit8:
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

   shrl $1, B_WIDTH              /* halve counter for word copies */
   jz blit_only_one_byte
   jnc blit_even_bytes

   .align 4, 0x90
   BLIT_LOOP(words_and_byte, 1,  /* word at a time, plus leftover byte */
      rep ; movsw
      movsb
   )
   jmp blit_done

   .align 4, 0x90
blit_even_bytes: 
   shrl $1, B_WIDTH              /* halve counter again, for long copies */
   jz blit_only_one_word
   jnc blit_even_words

   .align 4, 0x90
   BLIT_LOOP(longs_and_word, 1,  /* long at a time, plus leftover word */
      rep ; movsl
      movsw
   )
   jmp blit_done

   .align 4, 0x90
blit_even_words: 
   BLIT_LOOP(even_words, 1,      /* copy a long at a time */
      rep ; movsl
   )
   jmp blit_done

   .align 4, 0x90
blit_only_one_byte: 
   BLIT_LOOP(only_one_byte, 1,   /* copy just the one byte */
      movsb
   )
   jmp blit_done

   .align 4, 0x90
blit_only_one_word: 
   BLIT_LOOP(only_one_word, 1,   /* copy just the one word */
      movsw
   )

   .align 4, 0x90
blit_done:
   popl %ebx
   popl %esi
   popl %edi
   popw %es
   movl %ebp, %esp
   popl %ebp
   ret                           /* end of _linear_blit8() */




/* void _linear_blit_backward8(BITMAP *source, BITMAP *dest, int source_x, 
 *                      int source_y, int dest_x, dest_y, int width, height);
 *  Reverse blitting routine, for overlapping linear bitmaps.
 */
.globl __linear_blit_backward8
   .align 4
__linear_blit_backward8:
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
   addl B_DEST_X, %eax           /* x offset */
   movl %eax, %edi

   movl B_SOURCE, %edx           /* source bitmap */
   movl B_SOURCE_Y, %eax         /* line number */
   READ_BANK()                   /* select bank */
   addl B_SOURCE_X, %eax         /* x offset */
   movl %eax, %esi

   movl B_WIDTH, %ecx            /* x loop counter */
   movw BMP_SEG(%edx), %ds       /* load data segment */
   std                           /* backwards */
   rep ; movsb                   /* copy the line */

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
   ret                           /* end of _linear_blit_backward8() */



.globl __linear_blit8_end
   .align 4
__linear_blit8_end:
   ret




/* void _linear_masked_blit8(BITMAP *source, *dest, int source_x, source_y, 
 *                           int dest_x, dest_y, int width, height);
 *  Masked (skipping zero pixels) blitting routine for linear bitmaps.
 */
.globl __linear_masked_blit8
   .align 4
__linear_masked_blit8:
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
   BLIT_LOOP(masked, 1,

      .align 4, 0x90
   masked_blit_x_loop:
      movb (%esi), %al           /* read a byte */
      incl %esi

      orb %al, %al               /* test it */
      jz masked_blit_skip

      movb %al, %es:(%edi)       /* write the pixel */
      incl %edi
      decl %ecx
      jg masked_blit_x_loop
      jmp masked_blit_x_loop_done

      .align 4, 0x90
   masked_blit_skip:
      incl %edi                  /* skip zero pixels */
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
   ret                           /* end of _linear_masked_blit8() */


