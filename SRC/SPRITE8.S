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
 *      256 color sprite drawing (written for speed, not readability :-)
 *
 *      See readme.txt for copyright information.
 */


#include "asmdefs.inc"
#include "sprite.inc"

.text



/* void _linear_draw_sprite8(BITMAP *bmp, BITMAP *sprite, int x, y);
 *  Draws a sprite onto a linear bitmap at the specified x, y position, 
 *  using a masked drawing mode where zero pixels are not output.
 */
.globl __linear_draw_sprite8

   /* inner loop that copies just the one byte */
   #define LOOP_ONLY_ONE_BYTE                                                \
      SPRITE_LOOP(only_one_byte)                                           ; \
      movb (%esi), %bl                       /* read pixel */              ; \
      testb %bl, %bl                         /* test */                    ; \
      jz only_one_byte_skip                                                ; \
      movb %bl, %es:(%eax)                   /* write */                   ; \
   only_one_byte_skip:                                                     ; \
      incl %esi                                                            ; \
      incl %eax                                                            ; \
      /* no x loop */                                                      ; \
      SPRITE_END_Y(only_one_byte)


   /* inner loop that copies just the one word */
   #define LOOP_ONLY_ONE_WORD                                                \
      SPRITE_LOOP(only_one_word)                                           ; \
      movw (%esi), %bx                       /* read two pixels */         ; \
      testb %bl, %bl                         /* test */                    ; \
      jz only_one_word_skip_1                                              ; \
      movb %bl, %es:(%eax)                   /* write */                   ; \
   only_one_word_skip_1:                                                   ; \
      testb %bh, %bh                         /* test */                    ; \
      jz only_one_word_skip_2                                              ; \
      movb %bh, %es:1(%eax)                  /* write */                   ; \
   only_one_word_skip_2:                                                   ; \
      addl $2, %esi                                                        ; \
      addl $2, %eax                                                        ; \
      /* no x loop */                                                      ; \
      SPRITE_END_Y(only_one_word)


   /* inner loop that copies a word at a time, plus a leftover byte */
   #define LOOP_WORDS_AND_BYTE                                               \
      SPRITE_LOOP(words_and_byte)                                          ; \
      movw (%esi), %bx                       /* read two pixels */         ; \
      testb %bl, %bl                         /* test */                    ; \
      jz words_and_byte_skip_1                                             ; \
      movb %bl, %es:(%eax)                   /* write */                   ; \
   words_and_byte_skip_1:                                                  ; \
      testb %bh, %bh                         /* test */                    ; \
      jz words_and_byte_skip_2                                             ; \
      movb %bh, %es:1(%eax)                  /* write */                   ; \
   words_and_byte_skip_2:                                                  ; \
      addl $2, %esi                                                        ; \
      addl $2, %eax                                                        ; \
      SPRITE_END_X(words_and_byte)           /* end of x loop */           ; \
      movb (%esi), %bl                       /* read pixel */              ; \
      testb %bl, %bl                         /* test */                    ; \
      jz words_and_byte_end_skip                                           ; \
      movb %bl, %es:(%eax)                   /* write */                   ; \
   words_and_byte_end_skip:                                                ; \
      incl %esi                                                            ; \
      incl %eax                                                            ; \
      SPRITE_END_Y(words_and_byte)


   /* inner loop that copies a long at a time */
   #define LOOP_LONGS_ONLY                                                   \
      SPRITE_LOOP(longs_only)                                              ; \
      movl (%esi), %ebx                      /* read four pixels */        ; \
      testb %bl, %bl                         /* test */                    ; \
      jz longs_only_skip_1                                                 ; \
      movb %bl, %es:(%eax)                   /* write */                   ; \
   longs_only_skip_1:                                                      ; \
      testb %bh, %bh                         /* test */                    ; \
      jz longs_only_skip_2                                                 ; \
      movb %bh, %es:1(%eax)                  /* write */                   ; \
   longs_only_skip_2:                                                      ; \
      shrl $16, %ebx                         /* access next two pixels */  ; \
      testb %bl, %bl                         /* test */                    ; \
      jz longs_only_skip_3                                                 ; \
      movb %bl, %es:2(%eax)                  /* write */                   ; \
   longs_only_skip_3:                                                      ; \
      testb %bh, %bh                         /* test */                    ; \
      jz longs_only_skip_4                                                 ; \
      movb %bh, %es:3(%eax)                  /* write */                   ; \
   longs_only_skip_4:                                                      ; \
      addl $4, %esi                                                        ; \
      addl $4, %eax                                                        ; \
      SPRITE_END_X(longs_only)                                             ; \
      /* no cleanup at end of line */                                      ; \
      SPRITE_END_Y(longs_only) 


   /* inner loop that copies a long at a time, plus a leftover word */
   #define LOOP_LONGS_AND_WORD                                               \
      SPRITE_LOOP(longs_and_word)                                          ; \
      movl (%esi), %ebx                      /* read four pixels */        ; \
      testb %bl, %bl                         /* test */                    ; \
      jz longs_and_word_skip_1                                             ; \
      movb %bl, %es:(%eax)                   /* write */                   ; \
   longs_and_word_skip_1:                                                  ; \
      testb %bh, %bh                         /* test */                    ; \
      jz longs_and_word_skip_2                                             ; \
      movb %bh, %es:1(%eax)                  /* write */                   ; \
   longs_and_word_skip_2:                                                  ; \
      shrl $16, %ebx                         /* access next two pixels */  ; \
      testb %bl, %bl                         /* test */                    ; \
      jz longs_and_word_skip_3                                             ; \
      movb %bl, %es:2(%eax)                  /* write */                   ; \
   longs_and_word_skip_3:                                                  ; \
      testb %bh, %bh                         /* test */                    ; \
      jz longs_and_word_skip_4                                             ; \
      movb %bh, %es:3(%eax)                  /* write */                   ; \
   longs_and_word_skip_4:                                                  ; \
      addl $4, %esi                                                        ; \
      addl $4, %eax                                                        ; \
      SPRITE_END_X(longs_and_word)           /* end of x loop */           ; \
      movw (%esi), %bx                       /* read two pixels */         ; \
      testb %bl, %bl                         /* test */                    ; \
      jz longs_and_word_end_skip_1                                         ; \
      movb %bl, %es:(%eax)                   /* write */                   ; \
   longs_and_word_end_skip_1:                                              ; \
      testb %bh, %bh                         /* test */                    ; \
      jz longs_and_word_end_skip_2                                         ; \
      movb %bh, %es:1(%eax)                  /* write */                   ; \
   longs_and_word_end_skip_2:                                              ; \
      addl $2, %esi                                                        ; \
      addl $2, %eax                                                        ; \
      SPRITE_END_Y(longs_and_word)


   /* the actual sprite drawing routine... */
   .align 4
__linear_draw_sprite8:
   START_SPRITE_DRAW(sprite)

   movl BMP_W(%esi), %eax        /* sprite->w */
   subl S_W, %eax                /* - w */
   movl %eax, S_SGAP             /* store sprite gap */

   movl S_LGAP, %eax
   addl %eax, S_X                /* X += lgap */

   movl S_TGAP, %eax 
   addl %eax, S_Y                /* Y += tgap */

   movl BMP_LINE(%esi, %eax, 4), %esi
   addl S_LGAP, %esi             /* esi = sprite data ptr */

   shrl $1, S_W                  /* halve counter for word copies */
   jz sprite_only_one_byte
   jnc sprite_even_bytes

   .align 4, 0x90
   LOOP_WORDS_AND_BYTE           /* word at a time, plus leftover byte */
   jmp sprite_done

   .align 4, 0x90
sprite_even_bytes: 
   shrl $1, S_W                  /* halve counter again, for long copies */
   jz sprite_only_one_word
   jnc sprite_even_words

   .align 4, 0x90
   LOOP_LONGS_AND_WORD           /* long at a time, plus leftover word */
   jmp sprite_done

   .align 4, 0x90
sprite_even_words: 
   LOOP_LONGS_ONLY               /* copy a long at a time */
   jmp sprite_done

   .align 4, 0x90
sprite_only_one_byte: 
   LOOP_ONLY_ONE_BYTE            /* copy just the one byte */
   jmp sprite_done

   .align 4, 0x90
sprite_only_one_word: 
   LOOP_ONLY_ONE_WORD            /* copy just the one word */

   .align 4, 0x90
sprite_done:
   END_SPRITE_DRAW()
   ret                           /* end of _linear_draw_sprite8() */

.globl __linear_draw_sprite8_end
   .align 4
__linear_draw_sprite8_end:
   ret




/* void _linear_draw_sprite_v_flip8(BITMAP *bmp, BITMAP *sprite, int x, y);
 *  Draws a sprite to a linear bitmap, flipping vertically.
 */
.globl __linear_draw_sprite_v_flip8
   .align 4
__linear_draw_sprite_v_flip8:
   START_SPRITE_DRAW(sprite_v_flip)

   movl BMP_W(%esi), %eax        /* sprite->w */
   addl S_W, %eax                /* + w */
   negl %eax
   movl %eax, S_SGAP             /* store sprite gap */

   movl S_LGAP, %eax
   addl %eax, S_X                /* X += lgap */

   movl S_TGAP, %eax 
   addl %eax, S_Y                /* Y += tgap */

   negl %eax                     /* - tgap */
   addl BMP_H(%esi), %eax        /* + sprite->h */
   decl %eax
   movl BMP_LINE(%esi, %eax, 4), %esi
   addl S_LGAP, %esi             /* esi = sprite data ptr */

   .align 4, 0x90
   SPRITE_LOOP(v_flip) 
   movb (%esi), %bl              /* read pixel */
   testb %bl, %bl                /* test */
   jz sprite_v_flip_skip 
   movb %bl, %es:(%eax)          /* write */
sprite_v_flip_skip: 
   incl %esi 
   incl %eax 
   SPRITE_END_X(v_flip)
   SPRITE_END_Y(v_flip)

sprite_v_flip_done:
   END_SPRITE_DRAW()
   ret                           /* end of _linear_draw_sprite_v_flip8() */




/* void _linear_draw_sprite_h_flip8(BITMAP *bmp, BITMAP *sprite, int x, y);
 *  Draws a sprite to a linear bitmap, flipping horizontally.
 */
.globl __linear_draw_sprite_h_flip8
   .align 4
__linear_draw_sprite_h_flip8:
   START_SPRITE_DRAW(sprite_h_flip)

   movl BMP_W(%esi), %eax        /* sprite->w */
   addl S_W, %eax                /* + w */
   movl %eax, S_SGAP             /* store sprite gap */

   movl S_LGAP, %eax
   addl %eax, S_X                /* X += lgap */

   movl S_TGAP, %eax 
   addl %eax, S_Y                /* Y += tgap */

   movl BMP_W(%esi), %ecx 
   movl BMP_LINE(%esi, %eax, 4), %esi
   addl %ecx, %esi
   subl S_LGAP, %esi 
   decl %esi                     /* esi = sprite data ptr */

   .align 4, 0x90
   SPRITE_LOOP(h_flip) 
   movb (%esi), %bl              /* read pixel */
   testb %bl, %bl                /* test  */
   jz sprite_h_flip_skip 
   movb %bl, %es:(%eax)          /* write */
sprite_h_flip_skip: 
   decl %esi 
   incl %eax 
   SPRITE_END_X(h_flip)
   SPRITE_END_Y(h_flip)

sprite_h_flip_done:
   END_SPRITE_DRAW()
   ret                           /* end of _linear_draw_sprite_h_flip8() */




/* void _linear_draw_sprite_vh_flip8(BITMAP *bmp, BITMAP *sprite, int x, y);
 *  Draws a sprite to a linear bitmap, flipping both vertically and horizontally.
 */
.globl __linear_draw_sprite_vh_flip8 
   .align 4
__linear_draw_sprite_vh_flip8:
   START_SPRITE_DRAW(sprite_vh_flip)

   movl S_W, %eax                /* w */
   subl BMP_W(%esi), %eax        /* - sprite->w */
   movl %eax, S_SGAP             /* store sprite gap */

   movl S_LGAP, %eax
   addl %eax, S_X                /* X += lgap */

   movl S_TGAP, %eax 
   addl %eax, S_Y                /* Y += tgap */

   negl %eax                     /* - tgap */
   addl BMP_H(%esi), %eax        /* + sprite->h */
   decl %eax
   movl BMP_W(%esi), %ecx 
   movl BMP_LINE(%esi, %eax, 4), %esi
   addl %ecx, %esi
   subl S_LGAP, %esi 
   decl %esi                     /* esi = sprite data ptr */

   .align 4, 0x90
   SPRITE_LOOP(vh_flip) 
   movb (%esi), %bl              /* read pixel */
   testb %bl, %bl                /* test  */
   jz sprite_vh_flip_skip 
   movb %bl, %es:(%eax)          /* write */
sprite_vh_flip_skip: 
   decl %esi 
   incl %eax 
   SPRITE_END_X(vh_flip)
   SPRITE_END_Y(vh_flip)

sprite_vh_flip_done:
   END_SPRITE_DRAW()
   ret                           /* end of _linear_draw_sprite_vh_flip8() */




/* void _linear_draw_trans_sprite8(BITMAP *bmp, BITMAP *sprite, int x, y);
 *  Draws a translucent sprite onto a linear bitmap.
 */
.globl __linear_draw_trans_sprite8

   /* inner loop that copies just the one byte */
   #define TRANS_LOOP_ONLY_ONE_BYTE                                          \
      T_SPRITE_LOOP(trans_only_one_byte)                                   ; \
      movb %es:(%eax, %ecx), %bl             /* read a pixel */            ; \
      movb (%esi), %bh                       /* lookup pixel */            ; \
      movb (%edi, %ebx), %dl                                               ; \
      incl %esi                                                            ; \
      movb %dl, %es:(%eax)                   /* write the pixel */         ; \
      incl %eax                                                            ; \
      /* no x loop */                                                      ; \
      SPRITE_END_Y(trans_only_one_byte)


   /* inner loop that copies just the one word */
   #define TRANS_LOOP_ONLY_ONE_WORD                                          \
      T_SPRITE_LOOP(trans_only_one_word)                                   ; \
      movw %es:(%eax, %ecx), %dx             /* read two pixels */         ; \
      movb %dl, %bl                          /* lookup pixel 1 */          ; \
      movb (%esi), %bh                                                     ; \
      movb (%edi, %ebx), %dl                                               ; \
      incl %esi                                                            ; \
      movb %dh, %bl                          /* lookup pixel 2 */          ; \
      movb (%esi), %bh                                                     ; \
      movb (%edi, %ebx), %dh                                               ; \
      incl %esi                                                            ; \
      movw %dx, %es:(%eax)                   /* write two pixels */        ; \
      addl $2, %eax                                                        ; \
      /* no x loop */                                                      ; \
      SPRITE_END_Y(trans_only_one_word)


   /* inner loop that copies a word at a time, plus a leftover byte */
   #define TRANS_LOOP_WORDS_AND_BYTE                                         \
      T_SPRITE_LOOP(trans_words_and_byte)                                  ; \
      movw %es:(%eax, %ecx), %dx             /* read two pixels */         ; \
      movb %dl, %bl                          /* lookup pixel 1 */          ; \
      movb (%esi), %bh                                                     ; \
      movb (%edi, %ebx), %dl                                               ; \
      incl %esi                                                            ; \
      movb %dh, %bl                          /* lookup pixel 2 */          ; \
      movb (%esi), %bh                                                     ; \
      movb (%edi, %ebx), %dh                                               ; \
      incl %esi                                                            ; \
      movw %dx, %es:(%eax)                   /* write two pixels */        ; \
      addl $2, %eax                                                        ; \
      T_SPRITE_END_X(trans_words_and_byte)   /* end of x loop */           ; \
      movb %es:(%eax, %ecx), %bl             /* read a pixel */            ; \
      movb (%esi), %bh                       /* lookup pixel */            ; \
      movb (%edi, %ebx), %dl                                               ; \
      incl %esi                                                            ; \
      movb %dl, %es:(%eax)                   /* write the pixel */         ; \
      incl %eax                                                            ; \
      SPRITE_END_Y(trans_words_and_byte)


   /* inner loop that copies a long at a time */
   #define TRANS_LOOP_LONGS_ONLY                                             \
      T_SPRITE_LOOP(trans_longs_only)                                      ; \
      movl %es:(%eax, %ecx), %edx            /* read four pixels */        ; \
      movb %dl, %bl                          /* lookup pixel 1 */          ; \
      movb (%esi), %bh                                                     ; \
      movb (%edi, %ebx), %dl                                               ; \
      incl %esi                                                            ; \
      movb %dh, %bl                          /* lookup pixel 2 */          ; \
      movb (%esi), %bh                                                     ; \
      movb (%edi, %ebx), %dh                                               ; \
      incl %esi                                                            ; \
      roll $16, %edx                                                       ; \
      movb %dl, %bl                          /* lookup pixel 3 */          ; \
      movb (%esi), %bh                                                     ; \
      movb (%edi, %ebx), %dl                                               ; \
      incl %esi                                                            ; \
      movb %dh, %bl                          /* lookup pixel 4 */          ; \
      movb (%esi), %bh                                                     ; \
      movb (%edi, %ebx), %dh                                               ; \
      incl %esi                                                            ; \
      roll $16, %edx                                                       ; \
      movl %edx, %es:(%eax)                  /* write four pixels */       ; \
      addl $4, %eax                                                        ; \
      T_SPRITE_END_X(trans_longs_only)                                     ; \
      /* no cleanup at end of line */                                      ; \
      SPRITE_END_Y(trans_longs_only) 


   /* inner loop that copies a long at a time, plus a leftover word */
   #define TRANS_LOOP_LONGS_AND_WORD                                         \
      T_SPRITE_LOOP(trans_longs_and_word)                                  ; \
      movl %es:(%eax, %ecx), %edx            /* read four pixels */        ; \
      movb %dl, %bl                          /* lookup pixel 1 */          ; \
      movb (%esi), %bh                                                     ; \
      movb (%edi, %ebx), %dl                                               ; \
      incl %esi                                                            ; \
      movb %dh, %bl                          /* lookup pixel 2 */          ; \
      movb (%esi), %bh                                                     ; \
      movb (%edi, %ebx), %dh                                               ; \
      incl %esi                                                            ; \
      roll $16, %edx                                                       ; \
      movb %dl, %bl                          /* lookup pixel 3 */          ; \
      movb (%esi), %bh                                                     ; \
      movb (%edi, %ebx), %dl                                               ; \
      incl %esi                                                            ; \
      movb %dh, %bl                          /* lookup pixel 4 */          ; \
      movb (%esi), %bh                                                     ; \
      movb (%edi, %ebx), %dh                                               ; \
      incl %esi                                                            ; \
      roll $16, %edx                                                       ; \
      movl %edx, %es:(%eax)                  /* write four pixels */       ; \
      addl $4, %eax                                                        ; \
      T_SPRITE_END_X(trans_longs_and_word)   /* end of x loop */           ; \
      movw %es:(%eax, %ecx), %dx             /* read two pixels */         ; \
      movb %dl, %bl                          /* lookup pixel 1 */          ; \
      movb (%esi), %bh                                                     ; \
      movb (%edi, %ebx), %dl                                               ; \
      incl %esi                                                            ; \
      movb %dh, %bl                          /* lookup pixel 2 */          ; \
      movb (%esi), %bh                                                     ; \
      movb (%edi, %ebx), %dh                                               ; \
      incl %esi                                                            ; \
      movw %dx, %es:(%eax)                   /* write two pixels */        ; \
      addl $2, %eax                                                        ; \
      SPRITE_END_Y(trans_longs_and_word)


   /* the actual translucent sprite drawing routine... */
   .align 4
__linear_draw_trans_sprite8:
   START_SPRITE_DRAW(trans_sprite)

   movl BMP_W(%esi), %eax        /* sprite->w */
   subl S_W, %eax                /* - w */
   movl %eax, S_SGAP             /* store sprite gap */

   movl S_LGAP, %eax
   addl %eax, S_X                /* X += lgap */

   movl S_TGAP, %eax 
   addl %eax, S_Y                /* Y += tgap */

   movl BMP_LINE(%esi, %eax, 4), %esi
   addl S_LGAP, %esi             /* esi = sprite data ptr */

   xorl %ebx, %ebx
   movl _color_map, %edi         /* edi = color mapping table */

   shrl $1, S_W                  /* halve counter for word copies */
   jz trans_sprite_only_one_byte
   jnc trans_sprite_even_bytes

   .align 4, 0x90
   TRANS_LOOP_WORDS_AND_BYTE     /* word at a time, plus leftover byte */
   jmp trans_sprite_done

   .align 4, 0x90
trans_sprite_even_bytes: 
   shrl $1, S_W                  /* halve counter again, for long copies */
   jz trans_sprite_only_one_word
   jnc trans_sprite_even_words

   .align 4, 0x90
   TRANS_LOOP_LONGS_AND_WORD     /* long at a time, plus leftover word */
   jmp trans_sprite_done

   .align 4, 0x90
trans_sprite_even_words: 
   TRANS_LOOP_LONGS_ONLY         /* copy a long at a time */
   jmp trans_sprite_done

   .align 4, 0x90
trans_sprite_only_one_byte: 
   TRANS_LOOP_ONLY_ONE_BYTE      /* copy just the one byte */
   jmp trans_sprite_done

   .align 4, 0x90
trans_sprite_only_one_word: 
   TRANS_LOOP_ONLY_ONE_WORD      /* copy just the one word */

   .align 4, 0x90
trans_sprite_done:
   END_SPRITE_DRAW()
   ret                           /* end of _linear_draw_trans_sprite8() */




/* void _linear_draw_lit_sprite8(BITMAP *bmp, BITMAP *sprite, int x, y, color);
 *  Draws a lit sprite onto a linear bitmap.
 */
.globl __linear_draw_lit_sprite8

   #define COLOR     ARG5

   .align 4
__linear_draw_lit_sprite8:
   START_SPRITE_DRAW(lit_sprite)

   movl BMP_W(%esi), %eax        /* sprite->w */
   subl S_W, %eax                /* - w */
   movl %eax, S_SGAP             /* store sprite gap */

   movl S_LGAP, %eax
   addl %eax, S_X                /* X += lgap */

   movl S_TGAP, %eax 
   addl %eax, S_Y                /* Y += tgap */

   movl BMP_LINE(%esi, %eax, 4), %esi
   addl S_LGAP, %esi             /* esi = sprite data ptr */

   xorl %ebx, %ebx
   movb COLOR, %bh               /* store color in high byte */
   movl _color_map, %edi         /* edi = color mapping table */

   .align 4, 0x90
   SPRITE_LOOP(lit_sprite) 
   movb (%esi), %bl              /* read pixel into low byte */
   orb %bl, %bl
   jz lit_sprite_skip
   movb (%edi, %ebx), %bl        /* color table lookup */
   movb %bl, %es:(%eax)          /* write pixel */
lit_sprite_skip:
   incl %esi
   incl %eax
   SPRITE_END_X(lit_sprite)
   SPRITE_END_Y(lit_sprite)

lit_sprite_done:
   END_SPRITE_DRAW()
   ret                           /* end of _linear_draw_lit_sprite8() */




/* void __linear_draw_character8(BITMAP *bmp, BITMAP *sprite, int x, y, color);
 *  For proportional font output onto a linear bitmap: uses the sprite as 
 *  a mask, replacing all set pixels with the specified color.
 */
.globl __linear_draw_character8 

   #undef COLOR
   #define COLOR  ARG5

   .align 4
__linear_draw_character8:
   START_SPRITE_DRAW(draw_char)

   movl BMP_W(%esi), %eax        /* sprite->w */
   subl S_W, %eax                /* - w */
   movl %eax, S_SGAP             /* store sprite gap */

   movl S_LGAP, %eax
   addl %eax, S_X                /* X += lgap */

   movl S_TGAP, %eax 
   addl %eax, S_Y                /* Y += tgap */

   movl BMP_LINE(%esi, %eax, 4), %esi
   addl S_LGAP, %esi             /* esi = sprite data ptr */

   movb COLOR, %bl               /* bl = text color */
   movb __textmode, %bh          /* bh = background color */
   cmpl $0, __textmode
   jl draw_masked_char

   /* opaque (text_mode >= 0) character output */
   .align 4, 0x90
   SPRITE_LOOP(draw_opaque_char) 
   cmpb $0, (%esi)               /* test pixel */
   jz draw_opaque_background
   movb %bl, %es:(%eax)          /* write pixel */
   jmp draw_opaque_done
draw_opaque_background: 
   movb %bh, %es:(%eax)          /* write background */
draw_opaque_done:
   incl %esi 
   incl %eax 
   SPRITE_END_X(draw_opaque_char)
   SPRITE_END_Y(draw_opaque_char)
   jmp draw_char_done

   /* masked (text_mode -1) character output */
   .align 4, 0x90
draw_masked_char:
   SPRITE_LOOP(draw_masked_char) 
   cmpb $0, (%esi)               /* test pixel */
   jz draw_masked_skip
   movb %bl, %es:(%eax)          /* write pixel */
draw_masked_skip:
   incl %esi 
   incl %eax 
   SPRITE_END_X(draw_masked_char)
   SPRITE_END_Y(draw_masked_char)

draw_char_done:
   END_SPRITE_DRAW()
   ret                           /* end of _linear_draw_character8() */




/* void _linear_textout_fixed8(BITMAP *bmp, void *font, int height,
 *                            char *str, int x, y, color);
 *  Fast text output routine for fixed size fonts onto linear bitmaps.
 */
.globl __linear_textout_fixed8 

   .align 4
__linear_textout_fixed8:
   pushl %ebp
   movl %esp, %ebp
   subl $28, %esp

   pushl %edi
   pushl %esi
   pushl %ebx
   pushw %es

   /* initialises the inner drawing loop */
   #define START_X_LOOP()                                                    \
      movl T_COLOR, %eax                                                   ; \
      movl __textmode, %ebx

   /* cleans up after the inner drawing loop */
   #define END_X_LOOP()

   /* offsets an address by a number of pixels */
   #define GET_ADDR(a, b)                                                    \
      addl b, a

   /* writes ax to the destination */
   #define PUTA()                                                            \
      movb %al, %es:(%edi)

   /* writes bx to the destination */
   #define PUTB()                                                            \
      movb %bl, %es:(%edi)

   /* increments the destination */
   #define NEXTDEST()                                                        \
      incl %edi

   DRAW_TEXT()

   #undef START_X_LOOP
   #undef END_X_LOOP
   #undef GET_ADDR
   #undef PUTA
   #undef PUTB
   #undef NEXTDEST

   popw %es
   popl %ebx
   popl %esi
   popl %edi
   movl %ebp, %esp
   popl %ebp
   ret                           /* end of _textout_fixed8() */




/* void _linear_draw_rle_sprite8(BITMAP *bmp, RLE_SPRITE *sprite, int x, int y)
 *  Draws an RLE sprite onto a linear bitmap at the specified position.
 */
.globl __linear_draw_rle_sprite8

   .align 4
__linear_draw_rle_sprite8:

   /* bank switch routine */
   #define INIT_RLE_LINE()                                                   \
      movl R_Y, %eax                                                       ; \
      WRITE_BANK()                                                         ; \
      movl %eax, %edi                                                      ; \
      addl R_X, %edi


   /* copy a clipped pixel run */
   #define SLOW_RLE_RUN(n)                                                   \
      rep ; movsb


   /* no special initialisation required */
   #define INIT_FAST_RLE_LOOP()


   /* copy a run of solid pixels */
   #define FAST_RLE_RUN()                                                    \
      shrl $1, %ecx                                                        ; \
      jnc rle_noclip_no_byte                                               ; \
      movsb                      /* copy odd byte? */                      ; \
   rle_noclip_no_byte:                                                     ; \
      jz rle_noclip_x_loop                                                 ; \
      shrl $1, %ecx                                                        ; \
      jnc rle_noclip_no_word                                               ; \
      movsw                      /* copy odd word? */                      ; \
   rle_noclip_no_word:                                                     ; \
      jz rle_noclip_x_loop                                                 ; \
      rep ; movsl                /* 32 bit string copy */


   /* tests an RLE command byte */
   #define TEST_RLE_COMMAND(done, skip)                                      \
      testb %al, %al                                                       ; \
      jz done                                                              ; \
      js skip


   /* adds the offset in %eax onto the destination address */
   #define ADD_EAX_EDI()                                                     \
      addl %eax, %edi


   /* zero extend %al into %eax */
   #define RLE_ZEX_EAX()                                                     \
      movzbl %al, %eax


   /* zero extend %al into %ecx */
   #define RLE_ZEX_ECX()                                                     \
      movzbl %al, %ecx 


   /* sign extend %al into %eax */
   #define RLE_SEX_EAX()                                                     \
      movsbl %al, %eax


   /* do it! */
   DO_RLE(rle, 1, b, %al, $0)
   ret

   #undef INIT_RLE_LINE
   #undef SLOW_RLE_RUN
   #undef INIT_FAST_RLE_LOOP
   #undef FAST_RLE_RUN




/* void _linear_draw_trans_rle_sprite8(BITMAP *bmp, RLE_SPRITE *sprite, 
 *                                   int x, int y)
 *  Draws a translucent RLE sprite onto a linear bitmap.
 */
.globl __linear_draw_trans_rle_sprite8

   .align 4
__linear_draw_trans_rle_sprite8:

   /* bank switch routine */
   #define INIT_RLE_LINE()                                                   \
      movl R_BMP, %edx                                                     ; \
      movl R_Y, %eax                                                       ; \
      READ_BANK()                /* select read bank */                    ; \
      movl %eax, R_TMP                                                     ; \
      movl R_Y, %eax                                                       ; \
      WRITE_BANK()               /* select write bank */                   ; \
      movl %eax, %edi                                                      ; \
      movl R_TMP, %edx           /* calculate read/write diff */           ; \
      subl %edi, %edx                                                      ; \
      addl R_X, %edi


   /* copy a clipped pixel run */
   #define SLOW_RLE_RUN(n)                                                   \
      pushl %ebx                                                           ; \
      movl _color_map, %ebx                                                ; \
      xorl %eax, %eax                                                      ; \
									   ; \
   trans_rle_clipped_run_loop##n:                                          ; \
      movb (%esi), %ah           /* read sprite pixel */                   ; \
      movb %es:(%edi, %edx), %al /* read destination pixel */              ; \
      movb (%ebx, %eax), %al     /* blend */                               ; \
      movb %al, %es:(%edi)       /* write the pixel */                     ; \
      incl %esi                                                            ; \
      incl %edi                                                            ; \
      decl %ecx                                                            ; \
      jg trans_rle_clipped_run_loop##n                                     ; \
									   ; \
      popl %ebx


   /* initialise the drawing loop */
   #define INIT_FAST_RLE_LOOP()                                              \
      movl _color_map, %ebx


   /* copy a run of solid pixels */
   #define FAST_RLE_RUN()                                                    \
      xorl %eax, %eax                                                      ; \
									   ; \
      shrl $1, %ecx                                                        ; \
      jnc trans_rle_run_no_byte                                            ; \
									   ; \
      movb (%esi), %ah           /* read sprite pixel */                   ; \
      movb %es:(%edi, %edx), %al /* read destination pixel */              ; \
      movb (%ebx, %eax), %al     /* blend */                               ; \
      movb %al, %es:(%edi)       /* write the pixel */                     ; \
      incl %esi                                                            ; \
      incl %edi                                                            ; \
									   ; \
   trans_rle_run_no_byte:                                                  ; \
      orl %ecx, %ecx                                                       ; \
      jz trans_rle_run_done                                                ; \
									   ; \
      shrl $1, %ecx                                                        ; \
      jnc trans_rle_run_no_word                                            ; \
									   ; \
      pushl %ecx                                                           ; \
      xorl %ecx, %ecx                                                      ; \
      movw (%esi), %ax           /* read two sprite pixels */              ; \
      movw %ax, R_TMP                                                      ; \
      movw %es:(%edi, %edx), %ax /* read two destination pixels */         ; \
      movb %al, %cl                                                        ; \
      movb R_TMP, %ch                                                      ; \
      movb (%ebx, %ecx), %al     /* blend pixel 1 */                       ; \
      movb %ah, %cl                                                        ; \
      movb 1+R_TMP, %ch                                                    ; \
      movb (%ebx, %ecx), %ah     /* blend pixel 2 */                       ; \
      movw %ax, %es:(%edi)       /* write two pixels */                    ; \
      addl $2, %esi                                                        ; \
      addl $2, %edi                                                        ; \
      popl %ecx                                                            ; \
									   ; \
   trans_rle_run_no_word:                                                  ; \
      orl %ecx, %ecx                                                       ; \
      jz trans_rle_run_done                                                ; \
									   ; \
      movl %ecx, R_TMP2                                                    ; \
      xorl %ecx, %ecx                                                      ; \
									   ; \
   trans_rle_run_loop:                                                     ; \
      movl (%esi), %eax          /* read four sprite pixels */             ; \
      movl %eax, R_TMP                                                     ; \
      movl %es:(%edi, %edx), %eax   /* read four destination pixels */     ; \
      movb %al, %cl                                                        ; \
      movb R_TMP, %ch                                                      ; \
      movb (%ebx, %ecx), %al     /* blend pixel 1 */                       ; \
      movb %ah, %cl                                                        ; \
      movb 1+R_TMP, %ch                                                    ; \
      movb (%ebx, %ecx), %ah     /* blend pixel 2 */                       ; \
      roll $16, %eax                                                       ; \
      movb %al, %cl                                                        ; \
      movb 2+R_TMP, %ch                                                    ; \
      movb (%ebx, %ecx), %al     /* blend pixel 3 */                       ; \
      movb %ah, %cl                                                        ; \
      movb 3+R_TMP, %ch                                                    ; \
      movb (%ebx, %ecx), %ah     /* blend pixel 4 */                       ; \
      roll $16, %eax                                                       ; \
      movl %eax, %es:(%edi)      /* write four pixels */                   ; \
      addl $4, %esi                                                        ; \
      addl $4, %edi                                                        ; \
      decl R_TMP2                                                          ; \
      jg trans_rle_run_loop                                                ; \
									   ; \
   trans_rle_run_done:


   /* do it! */
   DO_RLE(rle_trans, 1, b, %al, $0)
   ret 

   #undef INIT_RLE_LINE
   #undef SLOW_RLE_RUN
   #undef INIT_FAST_RLE_LOOP
   #undef FAST_RLE_RUN




/* void _linear_draw_lit_rle_sprite8(BITMAP *bmp, RLE_SPRITE *sprite, 
 *                                  int x, int y, int color)
 *  Draws a tinted RLE sprite onto a linear bitmap.
 */
.globl __linear_draw_lit_rle_sprite8

   .align 4
__linear_draw_lit_rle_sprite8:

   /* bank switch routine */
   #define INIT_RLE_LINE()                                                   \
      movl R_BMP, %edx                                                     ; \
      movl R_Y, %eax                                                       ; \
      WRITE_BANK()                                                         ; \
      movl %eax, %edi                                                      ; \
      addl R_X, %edi                                                       ; \
      movl _color_map, %edx


   /* copy a clipped pixel run */
   #define SLOW_RLE_RUN(n)                                                   \
      xorl %eax, %eax                                                      ; \
      movb R_COLOR, %ah          /* store color in high byte */            ; \
									   ; \
   lit_rle_clipped_run_loop##n:                                            ; \
      movb (%esi), %al           /* read a pixel */                        ; \
      movb (%edx, %eax), %al     /* lookup in color table */               ; \
      movb %al, %es:(%edi)       /* write the pixel */                     ; \
      incl %esi                                                            ; \
      incl %edi                                                            ; \
      decl %ecx                                                            ; \
      jg lit_rle_clipped_run_loop##n


   /* initialise the drawing loop */
   #define INIT_FAST_RLE_LOOP()                                              \
      xorl %ebx, %ebx                                                      ; \
      movb R_COLOR, %bh          /* store color in high byte */


   /* copy a run of solid pixels */
   #define FAST_RLE_RUN()                                                    \
      shrl $1, %ecx                                                        ; \
      jnc lit_rle_run_no_byte                                              ; \
									   ; \
      movb (%esi), %bl           /* read pixel into low byte */            ; \
      movb (%edx, %ebx), %bl     /* lookup in lighting table */            ; \
      movb %bl, %es:(%edi)       /* write the pixel */                     ; \
      incl %esi                                                            ; \
      incl %edi                                                            ; \
									   ; \
   lit_rle_run_no_byte:                                                    ; \
      orl %ecx, %ecx                                                       ; \
      jz lit_rle_run_done                                                  ; \
									   ; \
      shrl $1, %ecx                                                        ; \
      jnc lit_rle_run_no_word                                              ; \
									   ; \
      movw (%esi), %ax           /* read two pixels */                     ; \
      movb %al, %bl                                                        ; \
      movb (%edx, %ebx), %al     /* lookup pixel 1 */                      ; \
      movb %ah, %bl                                                        ; \
      movb (%edx, %ebx), %ah     /* lookup pixel 2 */                      ; \
      movw %ax, %es:(%edi)       /* write two pixels */                    ; \
      addl $2, %esi                                                        ; \
      addl $2, %edi                                                        ; \
									   ; \
   lit_rle_run_no_word:                                                    ; \
      orl %ecx, %ecx                                                       ; \
      jz lit_rle_run_done                                                  ; \
									   ; \
   lit_rle_run_loop:                                                       ; \
      movl (%esi), %eax          /* read four pixels */                    ; \
      movb %al, %bl                                                        ; \
      movb (%edx, %ebx), %al     /* lookup pixel 1 */                      ; \
      movb %ah, %bl                                                        ; \
      movb (%edx, %ebx), %ah     /* lookup pixel 2 */                      ; \
      roll $16, %eax                                                       ; \
      movb %al, %bl                                                        ; \
      movb (%edx, %ebx), %al     /* lookup pixel 3 */                      ; \
      movb %ah, %bl                                                        ; \
      movb (%edx, %ebx), %ah     /* lookup pixel 4 */                      ; \
      roll $16, %eax                                                       ; \
      movl %eax, %es:(%edi)      /* write four pixels */                   ; \
      addl $4, %esi                                                        ; \
      addl $4, %edi                                                        ; \
      decl %ecx                                                            ; \
      jg lit_rle_run_loop                                                  ; \
									   ; \
   lit_rle_run_done:


   /* do it! */
   DO_RLE(rle_lit, 1, b, %al, $0)
   ret 

   #undef INIT_RLE_LINE
   #undef SLOW_RLE_RUN
   #undef INIT_FAST_RLE_LOOP
   #undef FAST_RLE_RUN




