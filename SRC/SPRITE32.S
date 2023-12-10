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
 *      32 bit sprite drawing (written for speed, not readability :-)
 *
 *      See readme.txt for copyright information.
 */


#include "asmdefs.inc"
#include "sprite.inc"

#ifdef ALLEGRO_COLOR32

.text



/* void _linear_draw_sprite32(BITMAP *bmp, BITMAP *sprite, int x, y);
 *  Draws a sprite onto a linear bitmap at the specified x, y position, 
 *  using a masked drawing mode where zero pixels are not output.
 */
.globl __linear_draw_sprite32

   .align 4
__linear_draw_sprite32:
   START_SPRITE_DRAW(sprite)

   movl BMP_W(%esi), %eax        /* sprite->w */
   subl S_W, %eax                /* - w */
   shll $2, %eax 
   movl %eax, S_SGAP             /* store sprite gap */

   movl S_LGAP, %eax
   addl %eax, S_X                /* X += lgap */

   movl S_TGAP, %eax 
   addl %eax, S_Y                /* Y += tgap */

   shll $2, S_X
   shll $2, S_LGAP

   movl BMP_LINE(%esi, %eax, 4), %esi
   addl S_LGAP, %esi             /* esi = sprite data ptr */

   .align 4, 0x90
   SPRITE_LOOP(draw_sprite) 
   movl (%esi), %ebx             /* read a pixel */ 
   cmpl $MASK_COLOR_32, %ebx     /* test */ 
   jz draw_sprite_skip 
   movl %ebx, %es:(%eax)         /* write */ 
   draw_sprite_skip: 
   addl $4, %esi 
   addl $4, %eax 
   SPRITE_END_X(draw_sprite) 
   SPRITE_END_Y(draw_sprite) 

   .align 4, 0x90
sprite_done:
   END_SPRITE_DRAW()
   ret                           /* end of _linear_draw_sprite32() */

.globl __linear_draw_sprite32_end
   .align 4
__linear_draw_sprite32_end:
   ret




/* void _linear_draw_256_sprite32(BITMAP *bmp, BITMAP *sprite, int x, y);
 *  Draws a 256 color sprite onto a linear bitmap at the specified x, y 
 *  position, using a masked drawing mode where zero pixels are not output.
 */
.globl __linear_draw_256_sprite32

   .align 4
__linear_draw_256_sprite32:
   START_SPRITE_DRAW(sprite256)

   movl BMP_W(%esi), %eax        /* sprite->w */
   subl S_W, %eax                /* - w */
   movl %eax, S_SGAP             /* store sprite gap */

   movl S_LGAP, %eax
   addl %eax, S_X                /* X += lgap */

   movl S_TGAP, %eax 
   addl %eax, S_Y                /* Y += tgap */

   shll $2, S_X

   movl BMP_LINE(%esi, %eax, 4), %esi
   addl S_LGAP, %esi             /* esi = sprite data ptr */

   movl $_pallete_color, %edi

   .align 4, 0x90
   SPRITE_LOOP(draw_sprite256) 
   movzbl (%esi), %ebx           /* read a pixel */ 
   orb %bl, %bl                  /* test */ 
   jz draw_256_sprite_skip 
   movl (%edi, %ebx, 4), %ebx    /* lookup in pallete table */
   movl %ebx, %es:(%eax)         /* write */ 
   draw_256_sprite_skip: 
   incl %esi 
   addl $4, %eax 
   SPRITE_END_X(draw_sprite256) 
   SPRITE_END_Y(draw_sprite256) 

   .align 4, 0x90
sprite256_done:
   END_SPRITE_DRAW()
   ret                           /* end of _linear_draw_256_sprite32() */




/* void _linear_draw_sprite_v_flip32(BITMAP *bmp, BITMAP *sprite, int x, y);
 *  Draws a sprite to a linear bitmap, flipping vertically.
 */
.globl __linear_draw_sprite_v_flip32
   .align 4
__linear_draw_sprite_v_flip32:
   START_SPRITE_DRAW(sprite_v_flip)

   movl BMP_W(%esi), %eax        /* sprite->w */
   addl S_W, %eax                /* + w */
   negl %eax
   shll $2, %eax 
   movl %eax, S_SGAP             /* store sprite gap */

   movl S_LGAP, %eax
   addl %eax, S_X                /* X += lgap */

   movl S_TGAP, %eax 
   addl %eax, S_Y                /* Y += tgap */

   shll $2, S_X
   shll $2, S_LGAP

   negl %eax                     /* - tgap */
   addl BMP_H(%esi), %eax        /* + sprite->h */
   decl %eax
   movl BMP_LINE(%esi, %eax, 4), %esi
   addl S_LGAP, %esi             /* esi = sprite data ptr */

   .align 4, 0x90
   SPRITE_LOOP(v_flip) 
   movl (%esi), %ebx             /* read pixel */
   cmpl $MASK_COLOR_32, %ebx     /* test */
   je sprite_v_flip_skip 
   movl %ebx, %es:(%eax)         /* write */
sprite_v_flip_skip: 
   addl $4, %esi 
   addl $4, %eax 
   SPRITE_END_X(v_flip)
   SPRITE_END_Y(v_flip)

sprite_v_flip_done:
   END_SPRITE_DRAW()
   ret                           /* end of _linear_draw_sprite_v_flip32() */




/* void _linear_draw_sprite_h_flip32(BITMAP *bmp, BITMAP *sprite, int x, y);
 *  Draws a sprite to a linear bitmap, flipping horizontally.
 */
.globl __linear_draw_sprite_h_flip32
   .align 4
__linear_draw_sprite_h_flip32:
   START_SPRITE_DRAW(sprite_h_flip)

   movl BMP_W(%esi), %eax        /* sprite->w */
   addl S_W, %eax                /* + w */
   shll $2, %eax 
   movl %eax, S_SGAP             /* store sprite gap */

   movl S_LGAP, %eax
   addl %eax, S_X                /* X += lgap */

   movl S_TGAP, %eax 
   addl %eax, S_Y                /* Y += tgap */

   shll $2, S_X
   shll $2, S_LGAP

   movl BMP_W(%esi), %ecx 
   movl BMP_LINE(%esi, %eax, 4), %esi
   leal (%esi, %ecx, 4), %esi
   subl S_LGAP, %esi 
   subl $4, %esi                 /* esi = sprite data ptr */

   .align 4, 0x90
   SPRITE_LOOP(h_flip) 
   movl (%esi), %ebx             /* read pixel */
   cmpl $MASK_COLOR_32, %ebx     /* test  */
   je sprite_h_flip_skip 
   movl %ebx, %es:(%eax)         /* write */
sprite_h_flip_skip: 
   subl $4, %esi 
   addl $4, %eax 
   SPRITE_END_X(h_flip)
   SPRITE_END_Y(h_flip)

sprite_h_flip_done:
   END_SPRITE_DRAW()
   ret                           /* end of _linear_draw_sprite_h_flip32() */




/* void _linear_draw_sprite_vh_flip32(BITMAP *bmp, BITMAP *sprite, int x, y);
 *  Draws a sprite to a linear bitmap, flipping both vertically and horizontally.
 */
.globl __linear_draw_sprite_vh_flip32 
   .align 4
__linear_draw_sprite_vh_flip32:
   START_SPRITE_DRAW(sprite_vh_flip)

   movl S_W, %eax                /* w */
   subl BMP_W(%esi), %eax        /* - sprite->w */
   shll $2, %eax 
   movl %eax, S_SGAP             /* store sprite gap */

   movl S_LGAP, %eax
   addl %eax, S_X                /* X += lgap */

   movl S_TGAP, %eax 
   addl %eax, S_Y                /* Y += tgap */

   shll $2, S_X
   shll $2, S_LGAP

   negl %eax                     /* - tgap */
   addl BMP_H(%esi), %eax        /* + sprite->h */
   decl %eax
   movl BMP_W(%esi), %ecx 
   movl BMP_LINE(%esi, %eax, 4), %esi
   leal (%esi, %ecx, 4), %esi
   subl S_LGAP, %esi 
   subl $4, %esi                 /* esi = sprite data ptr */

   .align 4, 0x90
   SPRITE_LOOP(vh_flip) 
   movl (%esi), %ebx             /* read pixel */
   cmpl $MASK_COLOR_32, %ebx     /* test  */
   je sprite_vh_flip_skip 
   movl %ebx, %es:(%eax)         /* write */
sprite_vh_flip_skip: 
   subl $4, %esi 
   addl $4, %eax 
   SPRITE_END_X(vh_flip)
   SPRITE_END_Y(vh_flip)

sprite_vh_flip_done:
   END_SPRITE_DRAW()
   ret                           /* end of _linear_draw_sprite_vh_flip32() */




/* void _linear_draw_trans_sprite32(BITMAP *bmp, BITMAP *sprite, int x, y);
 *  Draws a translucent sprite onto a linear bitmap.
 */
.globl __linear_draw_trans_sprite32
   .align 4
__linear_draw_trans_sprite32:
   START_SPRITE_DRAW(trans_sprite)

   movl BMP_W(%esi), %eax        /* sprite->w */
   subl S_W, %eax                /* - w */
   shll $2, %eax
   movl %eax, S_SGAP             /* store sprite gap */

   movl S_LGAP, %eax
   addl %eax, S_X                /* X += lgap */

   movl S_TGAP, %eax 
   addl %eax, S_Y                /* Y += tgap */

   shll $2, S_X
   shll $2, S_LGAP

   movl BMP_LINE(%esi, %eax, 4), %esi
   addl S_LGAP, %esi             /* esi = sprite data ptr */

   movl __blender_map24, %edi
   movl __blender_alpha, %eax
   movl (%edi, %eax, 4), %edi    /* edi = blender function */

   TT_SPRITE_LOOP(trans) 
   movl (%esi), %eax             /* read source pixel */
   cmpl $MASK_COLOR_32, %eax
   jz trans_sprite_skip
   pushl %ecx
   pushl %es:(%ebx, %ecx)        /* read memory pixel */
   pushl %eax
   call *%edi                    /* blend */
   addl $8, %esp
   popl %ecx
   movl %eax, %es:(%ebx)         /* write the result */
trans_sprite_skip:
   addl $4, %ebx 
   addl $4, %esi 
   T_SPRITE_END_X(trans) 
   /* no cleanup at end of line */ 
   SPRITE_END_Y(trans) 

trans_sprite_done:
   END_SPRITE_DRAW()
   ret                           /* end of _linear_draw_trans_sprite32() */




/* void _linear_draw_lit_sprite32(BITMAP *bmp, BITMAP *sprite, int x, y, color);
 *  Draws a lit sprite onto a linear bitmap.
 */
.globl __linear_draw_lit_sprite32

   #define ALPHA     ARG5

   .align 4
__linear_draw_lit_sprite32:
   START_SPRITE_DRAW(lit_sprite)

   movl BMP_W(%esi), %eax        /* sprite->w */
   subl S_W, %eax                /* - w */
   shll $2, %eax
   movl %eax, S_SGAP             /* store sprite gap */

   movl S_LGAP, %eax
   addl %eax, S_X                /* X += lgap */

   movl S_TGAP, %eax 
   addl %eax, S_Y                /* Y += tgap */

   shll $2, S_X
   shll $2, S_LGAP

   movl BMP_LINE(%esi, %eax, 4), %esi
   addl S_LGAP, %esi             /* esi = sprite data ptr */

   movl __blender_map24, %edi
   movl ALPHA, %eax
   movl (%edi, %eax, 4), %edi    /* edi = blender function */

   .align 4, 0x90
   LT_SPRITE_LOOP(lit_sprite) 
   movl (%esi), %eax             /* read pixel */
   cmpl $MASK_COLOR_32, %eax
   jz lit_sprite_skip
   pushl %eax
   pushl __blender_col_32
   call *%edi                    /* blend */
   addl $8, %esp
   movl %eax, %es:(%ebx)         /* write pixel */
lit_sprite_skip:
   addl $4, %esi
   addl $4, %ebx
   T_SPRITE_END_X(lit_sprite)
   SPRITE_END_Y(lit_sprite)

lit_sprite_done:
   END_SPRITE_DRAW()
   ret                           /* end of _linear_draw_lit_sprite32() */




/* void __linear_draw_character32(BITMAP *bmp, BITMAP *sprite, int x, y, color);
 *  For proportional font output onto a linear bitmap: uses the sprite as 
 *  a mask, replacing all set pixels with the specified color.
 */
.globl __linear_draw_character32 

   #define COLOR  ARG5

   .align 4
__linear_draw_character32:
   START_SPRITE_DRAW(draw_char)

   movl BMP_W(%esi), %eax        /* sprite->w */
   subl S_W, %eax                /* - w */
   movl %eax, S_SGAP             /* store sprite gap */

   movl S_LGAP, %eax
   addl %eax, S_X                /* X += lgap */

   movl S_TGAP, %eax 
   addl %eax, S_Y                /* Y += tgap */

   shll $2, S_X

   movl BMP_LINE(%esi, %eax, 4), %esi
   addl S_LGAP, %esi             /* esi = sprite data ptr */

   movl COLOR, %ebx              /* bx = text color */
   movl __textmode, %edi         /* di = background color */
   cmpl $0, %edi
   jl draw_masked_char

   /* opaque (text_mode >= 0) character output */
   .align 4, 0x90
   SPRITE_LOOP(draw_opaque_char) 
   cmpb $0, (%esi)               /* test pixel */
   jz draw_opaque_background
   movl %ebx, %es:(%eax)         /* write pixel */
   jmp draw_opaque_done
draw_opaque_background: 
   movl %edi, %es:(%eax)         /* write background */
draw_opaque_done:
   incl %esi 
   addl $4, %eax 
   SPRITE_END_X(draw_opaque_char)
   SPRITE_END_Y(draw_opaque_char)
   jmp draw_char_done

   /* masked (text_mode -1) character output */
   .align 4, 0x90
draw_masked_char:
   SPRITE_LOOP(draw_masked_char) 
   cmpb $0, (%esi)               /* test pixel */
   jz draw_masked_skip
   movl %ebx, %es:(%eax)         /* write pixel */
draw_masked_skip:
   incl %esi 
   addl $4, %eax 
   SPRITE_END_X(draw_masked_char)
   SPRITE_END_Y(draw_masked_char)

draw_char_done:
   END_SPRITE_DRAW()
   ret                           /* end of _linear_draw_character32() */




/* void _linear_textout_fixed32(BITMAP *bmp, void *font, int height,
 *                              char *str, int x, y, color);
 *  Fast text output routine for fixed size fonts onto linear bitmaps.
 */
.globl __linear_textout_fixed32

   .align 4
__linear_textout_fixed32:
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
      leal (a, b, 4), a

   /* writes ax to the destination */
   #define PUTA()                                                            \
      movl %eax, %es:(%edi)

   /* writes bx to the destination */
   #define PUTB()                                                            \
      movl %ebx, %es:(%edi)

   /* increments the destination */
   #define NEXTDEST()                                                        \
      addl $4, %edi

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
   ret                           /* end of _textout_fixed32() */




/* void _linear_draw_rle_sprite32(BITMAP *bmp, RLE_SPRITE *sprite, int x, y)
 *  Draws an RLE sprite onto a linear bitmap at the specified position.
 */
.globl __linear_draw_rle_sprite32

   .align 4
__linear_draw_rle_sprite32:

   /* bank switch routine */
   #define INIT_RLE_LINE()                                                   \
      movl R_Y, %eax                                                       ; \
      WRITE_BANK()                                                         ; \
      movl R_X, %edi                                                       ; \
      leal (%eax, %edi, 4), %edi


   /* copy a clipped pixel run */
   #define SLOW_RLE_RUN(n)                                                   \
      rep ; movsl


   /* no special initialisation required */
   #define INIT_FAST_RLE_LOOP()


   /* copy a run of solid pixels */
   #define FAST_RLE_RUN()                                                    \
      rep ; movsl


   /* tests an RLE command byte */
   #define TEST_RLE_COMMAND(done, skip)                                      \
      cmpl $MASK_COLOR_32, %eax                                            ; \
      je done                                                              ; \
      testl %eax, %eax                                                     ; \
      js skip


   /* adds the offset in %eax onto the destination address */
   #define ADD_EAX_EDI()                                                     \
      leal (%edi, %eax, 4), %edi


   /* no zero extend required */
   #define RLE_ZEX_EAX()


   /* this can be a simple copy... */
   #define RLE_ZEX_ECX()                                                     \
      movl %eax, %ecx


   /* no sign extend required */
   #define RLE_SEX_EAX()


   /* do it! */
   DO_RLE(rle, 4, l, %eax, $MASK_COLOR_32)
   ret

   #undef INIT_RLE_LINE
   #undef SLOW_RLE_RUN
   #undef INIT_FAST_RLE_LOOP
   #undef FAST_RLE_RUN




/* void _linear_draw_trans_rle_sprite32(BITMAP *bmp, RLE_SPRITE *sprite, 
 *                                      int x, int y)
 *  Draws a translucent RLE sprite onto a linear bitmap.
 */
.globl __linear_draw_trans_rle_sprite32

   .align 4
__linear_draw_trans_rle_sprite32:

   /* bank switch routine */
   #define INIT_RLE_LINE()                                                   \
      movl R_BMP, %edx                                                     ; \
      movl R_Y, %eax                                                       ; \
      READ_BANK()                   /* select read bank */                 ; \
      movl %eax, R_TMP                                                     ; \
      movl R_Y, %eax                                                       ; \
      WRITE_BANK()                  /* select write bank */                ; \
      movl R_X, %edi                                                       ; \
      leal (%eax, %edi, 4), %edi                                           ; \
      subl %eax, R_TMP              /* calculate read/write diff */


   /* copy a clipped pixel run */
   #define SLOW_RLE_RUN(n)                                                   \
      pushl %ebx                                                           ; \
      movl %ecx, R_TMP2                                                    ; \
									   ; \
      movl __blender_map24, %eax                                           ; \
      movl __blender_alpha, %ebx                                           ; \
      movl (%eax, %ebx, 4), %ebx    /* ebx = blender function */           ; \
									   ; \
   trans_rle_clipped_run_loop##n:                                          ; \
      movl R_TMP, %edx                                                     ; \
      pushl %es:(%edi, %edx)        /* read memory pixel */                ; \
      pushl (%esi)                  /* read sprite pixel */                ; \
      call *%ebx                    /* blend */                            ; \
      addl $8, %esp                                                        ; \
      movl %eax, %es:(%edi)         /* write the pixel */                  ; \
      addl $4, %esi                                                        ; \
      addl $4, %edi                                                        ; \
      decl R_TMP2                                                          ; \
      jg trans_rle_clipped_run_loop##n                                     ; \
									   ; \
      popl %ebx


   /* initialise the drawing loop */
   #define INIT_FAST_RLE_LOOP()                                              \
      movl __blender_map24, %eax                                           ; \
      movl __blender_alpha, %ebx                                           ; \
      movl (%eax, %ebx, 4), %ebx    /* ebx = blender function */


   /* copy a run of solid pixels */
   #define FAST_RLE_RUN()                                                    \
      movl %ecx, R_TMP2                                                    ; \
									   ; \
   trans_rle_run_loop:                                                     ; \
      movl R_TMP, %edx                                                     ; \
      pushl %es:(%edi, %edx)        /* read memory pixel */                ; \
      pushl (%esi)                  /* read sprite pixel */                ; \
      call *%ebx                    /* blend */                            ; \
      addl $8, %esp                                                        ; \
      movl %eax, %es:(%edi)         /* write the pixel */                  ; \
      addl $4, %esi                                                        ; \
      addl $4, %edi                                                        ; \
      decl R_TMP2                                                          ; \
      jg trans_rle_run_loop


   /* do it! */
   DO_RLE(rle_trans, 4, l, %eax, $MASK_COLOR_32)
   ret

   #undef INIT_RLE_LINE
   #undef SLOW_RLE_RUN
   #undef INIT_FAST_RLE_LOOP
   #undef FAST_RLE_RUN




/* void _linear_draw_lit_rle_sprite32(BITMAP *bmp, RLE_SPRITE *sprite, 
 *                                    int x, int y, int color)
 *  Draws a tinted RLE sprite onto a linear bitmap.
 */
.globl __linear_draw_lit_rle_sprite32

   .align 4
__linear_draw_lit_rle_sprite32:

   /* bank switch routine */
   #define INIT_RLE_LINE()                                                   \
      movl R_BMP, %edx                                                     ; \
      movl R_Y, %eax                                                       ; \
      WRITE_BANK()                                                         ; \
      movl R_X, %edi                                                       ; \
      leal (%eax, %edi, 4), %edi


   /* copy a clipped pixel run */
   #define SLOW_RLE_RUN(n)                                                   \
      pushl %ebx                                                           ; \
      movl __blender_map24, %eax                                           ; \
      movl R_COLOR, %ebx                                                   ; \
      movl (%eax, %ebx, 4), %ebx                                           ; \
      movl %ecx, R_TMP                                                     ; \
									   ; \
   lit_rle_clipped_run_loop##n:                                            ; \
      pushl (%esi)                  /* read sprite pixel */                ; \
      pushl __blender_col_32                                               ; \
      call *%ebx                    /* blend */                            ; \
      addl $8, %esp                                                        ; \
      movl %eax, %es:(%edi)         /* write the pixel */                  ; \
      addl $4, %esi                                                        ; \
      addl $4, %edi                                                        ; \
      decl R_TMP                                                           ; \
      jg lit_rle_clipped_run_loop##n                                       ; \
									   ; \
      popl %ebx


   /* initialise the drawing loop */
   #define INIT_FAST_RLE_LOOP()                                              \
      movl __blender_map24, %eax                                           ; \
      movl R_COLOR, %ebx                                                   ; \
      movl (%eax, %ebx, 4), %ebx


   /* copy a run of solid pixels */
   #define FAST_RLE_RUN()                                                    \
      movl %ecx, R_TMP                                                     ; \
									   ; \
   lit_rle_run_loop:                                                       ; \
      pushl (%esi)                  /* read sprite pixel */                ; \
      pushl __blender_col_32                                               ; \
      call *%ebx                    /* blend */                            ; \
      addl $8, %esp                                                        ; \
      movl %eax, %es:(%edi)         /* write the pixel */                  ; \
      addl $4, %esi                                                        ; \
      addl $4, %edi                                                        ; \
      decl R_TMP                                                           ; \
      jg lit_rle_run_loop


   /* do it! */
   DO_RLE(rle_lit, 4, l, %eax, $MASK_COLOR_32)
   ret

   #undef INIT_RLE_LINE
   #undef SLOW_RLE_RUN
   #undef INIT_FAST_RLE_LOOP
   #undef FAST_RLE_RUN




#endif      /* ifdef ALLEGRO_COLOR32 */

