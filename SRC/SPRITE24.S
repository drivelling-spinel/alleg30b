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
 *      This file written by Michal Mertl.
 *
 *      24 bit sprite drawing (written for speed, not readability :-)
 *
 *      See readme.txt for copyright information.
 */


#include "asmdefs.inc"
#include "sprite.inc"

#ifdef ALLEGRO_COLOR24

.text



/* void _linear_draw_sprite24(BITMAP *bmp, BITMAP *sprite, int x, y);
 *  Draws a sprite onto a linear bitmap at the specified x, y position, 
 *  using a masked drawing mode where zero pixels are not output.
 */
.globl __linear_draw_sprite24

   .align 4
__linear_draw_sprite24:
   START_SPRITE_DRAW(sprite)

   movl BMP_W(%esi), %eax        /* sprite->w */
   subl S_W, %eax                /* - w */
   leal (%eax, %eax, 2), %eax
   movl %eax, S_SGAP             /* store sprite gap */

   movl S_LGAP, %eax
   addl %eax, S_X                /* X += lgap */

   movl S_TGAP, %eax 
   addl %eax, S_Y                /* Y += tgap */

   movl S_X, %ebx
   leal (%ebx, %ebx, 2), %ebx    /* X *= 3 */
   movl %ebx, S_X

   movl S_LGAP, %ebx
   leal (%ebx, %ebx, 2), %ebx
   movl %ebx, S_LGAP

   movl BMP_LINE(%esi, %eax, 4), %esi
   addl S_LGAP, %esi             /* esi = sprite data ptr */

   .align 4, 0x90
   SPRITE_LOOP(draw_sprite) 
   movl (%esi), %ebx             /* read a pixel */
   andl $0xFFFFFF,%ebx
   cmpl $MASK_COLOR_24, %ebx     /* test */ 
   jz draw_sprite_skip 
   movw %bx, %es:(%eax)          /* write */ 
   shrl $16, %ebx
   movb %bl, %es:2(%eax)
   .align 4, 0x90
   draw_sprite_skip: 
   addl $3, %esi 
   addl $3, %eax 
   SPRITE_END_X(draw_sprite) 
   SPRITE_END_Y(draw_sprite) 

   .align 4, 0x90
sprite_done:
   END_SPRITE_DRAW()
   ret                           /* end of _linear_draw_sprite24() */

.globl __linear_draw_sprite24_end
   .align 4
__linear_draw_sprite24_end:
   ret




/* void _linear_draw_256_sprite24(BITMAP *bmp, BITMAP *sprite, int x, y);
 *  Draws a 256 color sprite onto a linear bitmap at the specified x, y 
 *  position, using a masked drawing mode where zero pixels are not output.
 */
.globl __linear_draw_256_sprite24

   .align 4
__linear_draw_256_sprite24:
   START_SPRITE_DRAW(sprite256)

   movl BMP_W(%esi), %eax        /* sprite->w */
   subl S_W, %eax                /* - w */
   movl %eax, S_SGAP             /* store sprite gap */

   movl S_LGAP, %eax
   addl %eax, S_X                /* X += lgap */

   movl S_TGAP, %eax 
   addl %eax, S_Y                /* Y += tgap */

   movl S_X,%ebx
   leal (%ebx, %ebx, 2), %ebx
   movl %ebx,S_X

   movl BMP_LINE(%esi, %eax, 4), %esi
   addl S_LGAP, %esi             /* esi = sprite data ptr */

   movl $_pallete_color, %edi

   .align 4, 0x90
   SPRITE_LOOP(draw_sprite256) 
   movzbl (%esi), %ebx           /* read a pixel */ 

   orb %bl, %bl                  /* test */ 
   jz draw_256_sprite_skip 
   movl (%edi, %ebx, 4), %ebx    /* lookup in pallete table */
   movw %bx, %es:(%eax)          /* write */ 
   shrl $16, %ebx
   movb %bl, %es:2(%eax) 
   .align 4, 0x90
   draw_256_sprite_skip: 
   addl $3, %eax 
   SPRITE_END_X(draw_sprite256) 
   SPRITE_END_Y(draw_sprite256) 

   .align 4, 0x90
sprite256_done:
   END_SPRITE_DRAW()
   ret                           /* end of _linear_draw_256_sprite24() */




/* void _linear_draw_sprite_v_flip24(BITMAP *bmp, BITMAP *sprite, int x, y);
 *  Draws a sprite to a linear bitmap, flipping vertically.
 */
.globl __linear_draw_sprite_v_flip24
   .align 4
__linear_draw_sprite_v_flip24:
   START_SPRITE_DRAW(sprite_v_flip)

   movl BMP_W(%esi), %eax        /* sprite->w */
   addl S_W, %eax                /* + w */
   negl %eax
   leal (%eax, %eax, 2), %eax
   movl %eax, S_SGAP             /* store sprite gap */

   movl S_LGAP, %eax
   addl %eax, S_X                /* X += lgap */

   movl S_TGAP, %eax 
   addl %eax, S_Y                /* Y += tgap */

   movl S_X, %ebx
   leal (%ebx, %ebx, 2), %ebx
   movl %ebx, S_X
   movl S_LGAP, %ebx
   leal (%ebx, %ebx, 2), %ebx
   movl %ebx,S_LGAP

   negl %eax                     /* - tgap */
   addl BMP_H(%esi), %eax        /* + sprite->h */
   decl %eax
   movl BMP_LINE(%esi, %eax, 4), %esi
   addl S_LGAP, %esi             /* esi = sprite data ptr */

   .align 4, 0x90
   SPRITE_LOOP(v_flip) 
   movl (%esi), %ebx             /* read pixel */
   andl $0xFFFFFF, %ebx
   cmpl $MASK_COLOR_24, %ebx     /* test */
   je sprite_v_flip_skip 
   movw %bx, %es:(%eax)         /* write */
   shrl $16, %ebx
   movb %bl, %es:2(%eax)
   .align 4, 0x90
sprite_v_flip_skip: 
   addl $3, %esi 
   addl $3, %eax 
   SPRITE_END_X(v_flip)
   SPRITE_END_Y(v_flip)

sprite_v_flip_done:
   END_SPRITE_DRAW()
   ret                           /* end of _linear_draw_sprite_v_flip24() */




/* void _linear_draw_sprite_h_flip24(BITMAP *bmp, BITMAP *sprite, int x, y);
 *  Draws a sprite to a linear bitmap, flipping horizontally.
 */
.globl __linear_draw_sprite_h_flip24
   .align 4
__linear_draw_sprite_h_flip24:
   START_SPRITE_DRAW(sprite_h_flip)

   movl BMP_W(%esi), %eax        /* sprite->w */
   addl S_W, %eax                /* + w */
   leal (%eax, %eax, 2), %eax
   movl %eax, S_SGAP             /* store sprite gap */

   movl S_LGAP, %eax
   addl %eax, S_X                /* X += lgap */

   movl S_TGAP, %eax 
   addl %eax, S_Y                /* Y += tgap */


   movl S_X, %ebx
   leal (%ebx, %ebx, 2), %ebx
   movl %ebx, S_X
   movl S_LGAP, %ebx
   leal (%ebx, %ebx, 2), %ebx
   movl %ebx, S_LGAP

   movl BMP_W(%esi), %ecx 
   movl BMP_LINE(%esi, %eax, 4), %esi
   leal (%ecx, %ecx, 2), %ecx
   leal (%esi, %ecx), %esi
   subl S_LGAP, %esi 
   subl $3, %esi                 /* esi = sprite data ptr */

   .align 4, 0x90
   SPRITE_LOOP(h_flip) 
   movl (%esi), %ebx             /* read pixel */
   andl $0xFFFFFF, %ebx
   cmpl $MASK_COLOR_24, %ebx     /* test  */
   je sprite_h_flip_skip
   movw %bx,%es:(%eax)
   shrl $16, %ebx
   movb %bl, %es:2(%eax)         /* write */
sprite_h_flip_skip: 
   subl $3, %esi 
   addl $3, %eax 
   SPRITE_END_X(h_flip)
   SPRITE_END_Y(h_flip)

sprite_h_flip_done:
   END_SPRITE_DRAW()
   ret                           /* end of _linear_draw_sprite_h_flip24() */




/* void _linear_draw_sprite_vh_flip24(BITMAP *bmp, BITMAP *sprite, int x, y);
 *  Draws a sprite to a linear bitmap, flipping both vertically and horizontally.
 */
.globl __linear_draw_sprite_vh_flip24 
   .align 4
__linear_draw_sprite_vh_flip24:
   START_SPRITE_DRAW(sprite_vh_flip)

   movl S_W, %eax                /* w */
   subl BMP_W(%esi), %eax        /* - sprite->w */
   leal (%eax, %eax, 2), %eax
   movl %eax, S_SGAP             /* store sprite gap */

   movl S_LGAP, %eax
   addl %eax, S_X                /* X += lgap */

   movl S_TGAP, %eax 
   addl %eax, S_Y                /* Y += tgap */

   movl S_X, %ebx
   leal (%ebx, %ebx, 2), %ebx
   movl %ebx, S_X
   movl S_LGAP, %ebx
   leal (%ebx, %ebx, 2), %ebx
   movl %ebx, S_LGAP

   negl %eax                     /* - tgap */
   addl BMP_H(%esi), %eax        /* + sprite->h */
   decl %eax
   movl BMP_W(%esi), %ecx 
   movl BMP_LINE(%esi, %eax, 4), %esi
   leal (%ecx, %ecx, 2), %ecx
   leal (%esi, %ecx), %esi
   subl S_LGAP, %esi 
   subl $3, %esi                 /* esi = sprite data ptr */

   .align 4, 0x90
   SPRITE_LOOP(vh_flip) 
   movl (%esi), %ebx             /* read pixel */
   andl $0xFFFFFF, %ebx
   cmpl $MASK_COLOR_24, %ebx     /* test  */
   je sprite_vh_flip_skip
   movw %bx, %es:(%eax)
   shrl $16, %ebx
   movb %bl, %es:2(%eax)         /* write */
sprite_vh_flip_skip: 
   subl $3, %esi 
   addl $3, %eax 
   SPRITE_END_X(vh_flip)
   SPRITE_END_Y(vh_flip)

sprite_vh_flip_done:
   END_SPRITE_DRAW()
   ret                           /* end of _linear_draw_sprite_vh_flip24() */




/* void _linear_draw_trans_sprite24(BITMAP *bmp, BITMAP *sprite, int x, y);
 *  Draws a translucent sprite onto a linear bitmap.
 */
.globl __linear_draw_trans_sprite24

   .align 4
__linear_draw_trans_sprite24:
   START_SPRITE_DRAW(trans_sprite)

   movl BMP_W(%esi), %eax        /* sprite->w */
   subl S_W, %eax                /* - w */
   leal (%eax, %eax, 2), %eax
   movl %eax, S_SGAP             /* store sprite gap */

   movl S_LGAP, %eax
   addl %eax, S_X                /* X += lgap */

   movl S_TGAP, %eax 
   addl %eax, S_Y                /* Y += tgap */

   movl S_X, %edi
   leal (%edi, %edi, 2), %edi
   movl %edi, S_X

   movl S_LGAP, %edi
   leal (%edi, %edi, 2), %edi
   movl %edi, S_LGAP

   movl BMP_LINE(%esi, %eax, 4), %esi
   addl S_LGAP, %esi             /* esi = sprite data ptr */

   movl __blender_map24, %edi
   movl __blender_alpha, %eax
   movl (%edi, %eax, 4), %edi    /* edi = blender function */

   TT_SPRITE_LOOP(trans) 
   movl (%esi), %eax             /* read source pixel */
   andl $0xFFFFFF, %eax
   cmpl $MASK_COLOR_32, %eax
   jz trans_sprite_skip
   pushl %ecx
   pushl %es:(%ebx, %ecx)        /* read memory pixel */
   pushl %eax
   call *%edi                    /* blend */
   addl $8, %esp
   popl %ecx
   movw %ax, %es:(%ebx)          /* write the result */
   shrl $16, %eax
   movb %al, %es:2(%ebx)
trans_sprite_skip:
   addl $3, %ebx 
   addl $3, %esi 
   T_SPRITE_END_X(trans) 
   /* no cleanup at end of line */ 
   SPRITE_END_Y(trans) 

trans_sprite_done:
   END_SPRITE_DRAW()
   ret                           /* end of _linear_draw_trans_sprite24() */




/* void _linear_draw_lit_sprite24(BITMAP *bmp, BITMAP *sprite, int x, y, color);
 *  Draws a lit sprite onto a linear bitmap.
 */
.globl __linear_draw_lit_sprite24

   #define ALPHA     ARG5

   .align 4
__linear_draw_lit_sprite24:
   START_SPRITE_DRAW(lit_sprite)

   movl BMP_W(%esi), %eax        /* sprite->w */
   subl S_W, %eax                /* - w */
   leal (%eax, %eax, 2), %eax
   movl %eax, S_SGAP             /* store sprite gap */

   movl S_LGAP, %eax
   addl %eax, S_X                /* X += lgap */

   movl S_TGAP, %eax 
   addl %eax, S_Y                /* Y += tgap */

   movl S_X, %edi
   leal (%edi, %edi, 2), %edi
   movl %edi, S_X

   movl S_LGAP, %edi
   leal (%edi, %edi, 2), %edi
   movl %edi, S_LGAP

   movl BMP_LINE(%esi, %eax, 4), %esi
   addl S_LGAP, %esi             /* esi = sprite data ptr */

   movl __blender_map24, %edi
   movl ALPHA, %eax
   movl (%edi, %eax, 4), %edi    /* edi = blender function */

   .align 4, 0x90
   LT_SPRITE_LOOP(lit_sprite) 
   movl (%esi), %eax             /* read pixel */
   andl $0xFFFFFF, %eax
   cmpl $MASK_COLOR_32, %eax
   jz lit_sprite_skip
   pushl %eax
   pushl __blender_col_32
   call *%edi                    /* blend */
   addl $8, %esp
   movw %ax, %es:(%ebx)          /* write pixel */
   shrl $16, %eax
   movb %al, %es:2(%ebx)
lit_sprite_skip:
   addl $3, %esi
   addl $3, %ebx
   T_SPRITE_END_X(lit_sprite)
   SPRITE_END_Y(lit_sprite)

lit_sprite_done:
   END_SPRITE_DRAW()
   ret                           /* end of _linear_draw_lit_sprite24() */




/* void __linear_draw_character24(BITMAP *bmp, BITMAP *sprite, int x, y, color);
 *  For proportional font output onto a linear bitmap: uses the sprite as 
 *  a mask, replacing all set pixels with the specified color.
 */
.globl __linear_draw_character24 

   #define COLOR  ARG5

   /* sets up the inner sprite drawing loop, loads registers, etc */
   #define SPRITE_LOOP24(name)                                               \
   sprite_y_loop_##name:                                                   ; \
      movl S_Y, %eax                /* load line */                        ; \
      movl S_BMP, %edx              /* load bitmap pointer */              ; \
      WRITE_BANK()                  /* select bank */                      ; \
      addl S_X, %eax                /* add x offset */                     ; \
      movl S_W, %ecx                /* x loop counter */                   ; \
      movl S_MASK, %edx             /* color value */                      ; \
      .align 4, 0x90                                                       ; \
   sprite_x_loop_##name:

   .align 4
__linear_draw_character24:
   START_SPRITE_DRAW(draw_char)

   movl BMP_W(%esi), %eax        /* sprite->w */
   subl S_W, %eax                /* - w */
   movl %eax, S_SGAP             /* store sprite gap */

   movl S_LGAP, %eax
   addl %eax, S_X                /* X += lgap */

   movl S_TGAP, %eax 
   addl %eax, S_Y                /* Y += tgap */

   movl S_X, %ebx
   leal (%ebx, %ebx, 2), %ebx
   movl %ebx, S_X

   movl BMP_LINE(%esi, %eax, 4), %esi
   addl S_LGAP, %esi             /* esi = sprite data ptr */

   movl COLOR, %ebx              /* bx = text color */
   movl __textmode, %edi         /* di = background color */
   movb 2+COLOR, %dl             /* dl = text color high byte */
   movb __textmode+2, %dh        /* dh = background color high byte */
   movl %edx, S_MASK             /* store */

   cmpl $0, %edi
   jl draw_masked_char

   /* opaque (text_mode >= 0) character output */
   .align 4, 0x90
   SPRITE_LOOP24(draw_opaque_char) 
   cmpb $0, (%esi)               /* test pixel */
   jz draw_opaque_background
   movw %bx, %es:(%eax)          /* write pixel */
   movb %dl, %es:2(%eax)
   jmp draw_opaque_done
draw_opaque_background: 
   movw %di, %es:(%eax)          /* write background */
   movb %dh, %es:2(%eax)
draw_opaque_done:
   incl %esi 
   addl $3, %eax 
   SPRITE_END_X(draw_opaque_char)
   SPRITE_END_Y(draw_opaque_char)
   jmp draw_char_done

   /* masked (text_mode -1) character output */
   .align 4, 0x90
draw_masked_char:
   SPRITE_LOOP24(draw_masked_char) 
   cmpb $0, (%esi)               /* test pixel */
   jz draw_masked_skip
   movw %bx, %es:(%eax)          /* write pixel */
   movb %dl, %es:2(%eax)
draw_masked_skip:
   incl %esi 
   addl $3, %eax 
   SPRITE_END_X(draw_masked_char)
   SPRITE_END_Y(draw_masked_char)

draw_char_done:
   END_SPRITE_DRAW()
   ret                           /* end of _linear_draw_character24() */




/* void _linear_textout_fixed24(BITMAP *bmp, void *font, int height,
 *                              char *str, int x, y, color);
 *  Fast text output routine for fixed size fonts onto linear bitmaps.
 */
.globl __linear_textout_fixed24

   .align 4
__linear_textout_fixed24:
   pushl %ebp
   movl %esp, %ebp
   subl $28, %esp

   pushl %edi
   pushl %esi
   pushl %ebx
   pushw %es

   /* initialises the inner drawing loop */
   #define START_X_LOOP()                                                    \
      pushl %esi                                                           ; \
      movl T_COLOR, %esi         /* text color in %si */                   ; \
      movb 2+T_COLOR, %al        /* 3rd byte of text color in %al */       ; \
      movl __textmode, %ebx      /* background color in %bx */             ; \
      movb __textmode+2, %ah     /* third byte of bk color in %ah */

   /* cleans up after the inner drawing loop */
   #define END_X_LOOP()                                                      \
      popl %esi

   /* offsets an address by a number of pixels */
   #define GET_ADDR(a, b)                                                    \
      leal (a, b, 2), a                                                    ; \
      addl b, a

   /* writes ax to the destination */
   #define PUTA()                                                            \
      movw %si, %es:(%edi)                                                 ; \
      movb %al, %es:2(%edi)

   /* writes bx to the destination */
   #define PUTB()                                                            \
      movw %bx, %es:(%edi)                                                 ; \
      movb %ah, %es:2(%edi)

   /* increments the destination */
   #define NEXTDEST()                                                        \
      addl $3, %edi

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
   ret                           /* end of _textout_fixed24() */




/* void _linear_draw_rle_sprite24(BITMAP *bmp, RLE_SPRITE *sprite, int x, y)
 *  Draws an RLE sprite onto a linear bitmap at the specified position.
 */
.globl __linear_draw_rle_sprite24

   .align 4
__linear_draw_rle_sprite24:

   /* bank switch routine */
   #define INIT_RLE_LINE()                                                   \
      movl R_Y, %eax                                                       ; \
      WRITE_BANK()                                                         ; \
      movl R_X, %edi                                                       ; \
      leal (%edi, %edi, 2), %edi                                           ; \
      addl %eax, %edi


   /* copy a clipped pixel run */
   #define SLOW_RLE_RUN(n)                                                   \
      rle24_loop_##n:                                                      ; \
	 lodsl                                                             ; \
	 movw %ax, %es:(%edi)                                              ; \
	 shrl $16, %eax                                                    ; \
	 movb %al, %es:2(%edi)                                             ; \
	 addl $3, %edi                                                     ; \
	 decl %ecx                                                         ; \
	 jg rle24_loop_##n


   /* no special initialisation required */
   #define INIT_FAST_RLE_LOOP()


   /* copy a run of solid pixels */
   #define FAST_RLE_RUN()                                                    \
      SLOW_RLE_RUN(fast)


   /* tests an RLE command byte */
   #define TEST_RLE_COMMAND(done, skip)                                      \
      cmpl $MASK_COLOR_24, %eax                                            ; \
      je done                                                              ; \
      testl %eax, %eax                                                     ; \
      js skip


   /* adds the offset in %eax onto the destination address */
   #define ADD_EAX_EDI()                                                     \
      leal (%edi, %eax, 2), %edi                                           ; \
      addl %eax, %edi


   /* no zero extend required */
   #define RLE_ZEX_EAX()


   /* this can be a simple copy... */
   #define RLE_ZEX_ECX()                                                     \
      movl %eax, %ecx


   /* no sign extend required */
   #define RLE_SEX_EAX()


   /* do it! */
   DO_RLE(rle, 4, l, %eax, $MASK_COLOR_24)
   ret

   #undef INIT_RLE_LINE
   #undef SLOW_RLE_RUN
   #undef INIT_FAST_RLE_LOOP
   #undef FAST_RLE_RUN




/* void _linear_draw_trans_rle_sprite24(BITMAP *bmp, RLE_SPRITE *sprite, 
 *                                      int x, int y)
 *  Draws a translucent RLE sprite onto a linear bitmap.
 */
.globl __linear_draw_trans_rle_sprite24

   .align 4
__linear_draw_trans_rle_sprite24:

   /* bank switch routine */
   #define INIT_RLE_LINE()                                                   \
      movl R_BMP, %edx                                                     ; \
      movl R_Y, %eax                                                       ; \
      READ_BANK()                   /* select read bank */                 ; \
      movl %eax, R_TMP                                                     ; \
      movl R_Y, %eax                                                       ; \
      WRITE_BANK()                  /* select write bank */                ; \
      movl R_X, %edi                                                       ; \
      leal (%edi, %edi, 2), %edi                                           ; \
      addl %eax, %edi                                                      ; \
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
      movw %ax, %es:(%edi)          /* write the pixel */                  ; \
      shrl $16, %eax                                                       ; \
      movb %al, %es:2(%edi)                                                ; \
      addl $4, %esi                                                        ; \
      addl $3, %edi                                                        ; \
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
      movw %ax, %es:(%edi)          /* write the pixel */                  ; \
      shrl $16, %eax                                                       ; \
      movb %al, %es:2(%edi)                                                ; \
      addl $4, %esi                                                        ; \
      addl $3, %edi                                                        ; \
      decl R_TMP2                                                          ; \
      jg trans_rle_run_loop


   /* do it! */
   DO_RLE(rle_trans, 4, l, %eax, $MASK_COLOR_24)
   ret

   #undef INIT_RLE_LINE
   #undef SLOW_RLE_RUN
   #undef INIT_FAST_RLE_LOOP
   #undef FAST_RLE_RUN




/* void _linear_draw_lit_rle_sprite24(BITMAP *bmp, RLE_SPRITE *sprite, 
 *                                    int x, int y, int color)
 *  Draws a tinted RLE sprite onto a linear bitmap.
 */
.globl __linear_draw_lit_rle_sprite24

   .align 4
__linear_draw_lit_rle_sprite24:

   /* bank switch routine */
   #define INIT_RLE_LINE()                                                   \
      movl R_BMP, %edx                                                     ; \
      movl R_Y, %eax                                                       ; \
      WRITE_BANK()                                                         ; \
      movl R_X, %edi                                                       ; \
      leal (%edi, %edi, 2), %edi                                           ; \
      addl %eax, %edi


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
      pushl __blender_col_24                                               ; \
      call *%ebx                    /* blend */                            ; \
      addl $8, %esp                                                        ; \
      movw %ax, %es:(%edi)          /* write the pixel */                  ; \
      shrl $16, %eax                                                       ; \
      movb %al, %es:2(%edi)                                                ; \
      addl $4, %esi                                                        ; \
      addl $3, %edi                                                        ; \
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
      pushl __blender_col_24                                               ; \
      call *%ebx                    /* blend */                            ; \
      addl $8, %esp                                                        ; \
      movw %ax, %es:(%edi)          /* write the pixel */                  ; \
      shrl $16, %eax                                                       ; \
      movb %al, %es:2(%edi)                                                ; \
      addl $4, %esi                                                        ; \
      addl $3, %edi                                                        ; \
      decl R_TMP                                                           ; \
      jg lit_rle_run_loop


   /* do it! */
   DO_RLE(rle_lit, 4, l, %eax, $MASK_COLOR_24)
   ret

   #undef INIT_RLE_LINE
   #undef SLOW_RLE_RUN
   #undef INIT_FAST_RLE_LOOP
   #undef FAST_RLE_RUN




#endif      /* ifdef ALLEGRO_COLOR24 */

