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
 *      Helper macros for constructing the asm sprite drawing routines.
 *
 *      See readme.txt for copyright information.
 */


#ifndef SPRITE_INC
#define SPRITE_INC



/* generic framework for constructing sprite drawing routines, shared 
 * between the 8, 16, 24, and 32 bit versions of the code...
 */


#define S_BMP       ARG1
#define S_SPRITE    ARG2
#define S_X         ARG3
#define S_Y         ARG4

#define S_TGAP   -4(%ebp)
#define S_LGAP   -8(%ebp)
#define S_SGAP   -12(%ebp)
#define S_W      -16(%ebp)
#define S_H      -20(%ebp)
#define S_C      -24(%ebp)
#define S_MASK   -28(%ebp)



/* sets up a sprite draw operation and handles the clipping */
#define START_SPRITE_DRAW(name)                                              \
   pushl %ebp                                                              ; \
   movl %esp, %ebp                                                         ; \
   subl $28, %esp                         /* seven local variables */      ; \
									   ; \
   pushl %edi                                                              ; \
   pushl %esi                                                              ; \
   pushl %ebx                                                              ; \
   pushw %es                                                               ; \
									   ; \
   movl S_BMP, %edx                       /* edx = bitmap pointer */       ; \
   movl S_SPRITE, %esi                    /* esi = sprite pointer */       ; \
									   ; \
   movw BMP_SEG(%edx), %es                /* segment selector */           ; \
									   ; \
   cmpl $0, BMP_CLIP(%edx)                /* test bmp->clip */             ; \
   jz name##_no_clip                                                       ; \
									   ; \
   movl BMP_CT(%edx), %eax                /* bmp->ct */                    ; \
   subl S_Y, %eax                         /* eax -= y */                   ; \
   jge name##_tgap_ok                                                      ; \
   xorl %eax, %eax                                                         ; \
name##_tgap_ok:                                                            ; \
   movl %eax, S_TGAP                      /* set tgap */                   ; \
									   ; \
   movl BMP_H(%esi), %ebx                 /* sprite->h */                  ; \
   movl BMP_CB(%edx), %ecx                /* bmp->cb */                    ; \
   subl S_Y, %ecx                         /* ecx -= y */                   ; \
   cmpl %ebx, %ecx                        /* check bottom clipping */      ; \
   jg name##_height_ok                                                     ; \
   movl %ecx, %ebx                                                         ; \
name##_height_ok:                                                          ; \
   subl %eax, %ebx                        /* height -= tgap */             ; \
   jle name##_done                                                         ; \
   movl %ebx, S_H                         /* set h */                      ; \
									   ; \
   movl BMP_CL(%edx), %eax                /* bmp->cl */                    ; \
   subl S_X, %eax                         /* eax -= x */                   ; \
   jge name##_lgap_ok                                                      ; \
   xorl %eax, %eax                                                         ; \
name##_lgap_ok:                                                            ; \
   movl %eax, S_LGAP                      /* set lgap */                   ; \
									   ; \
   movl BMP_W(%esi), %ebx                 /* sprite->w */                  ; \
   movl BMP_CR(%edx), %ecx                /* bmp->cr */                    ; \
   subl S_X, %ecx                         /* ecx -= x */                   ; \
   cmpl %ebx, %ecx                        /* check left clipping */        ; \
   jg name##_width_ok                                                      ; \
   movl %ecx, %ebx                                                         ; \
name##_width_ok:                                                           ; \
   subl %eax, %ebx                        /* width -= lgap */              ; \
   jle name##_done                                                         ; \
   movl %ebx, S_W                         /* set w */                      ; \
									   ; \
   jmp name##_clip_done                                                    ; \
									   ; \
   .align 4, 0x90                                                          ; \
name##_no_clip:                                                            ; \
   movl $0, S_TGAP                                                         ; \
   movl $0, S_LGAP                                                         ; \
   movl BMP_W(%esi), %eax                                                  ; \
   movl %eax, S_W                         /* w = sprite->w */              ; \
   movl BMP_H(%esi), %eax                                                  ; \
   movl %eax, S_H                         /* h = sprite->h */              ; \
									   ; \
   .align 4, 0x90                                                          ; \
name##_clip_done:



/* cleans up the stack after a sprite draw operation */
#define END_SPRITE_DRAW()                                                    \
   popw %es                                                                ; \
   popl %ebx                                                               ; \
   popl %esi                                                               ; \
   popl %edi                                                               ; \
   movl %ebp, %esp                                                         ; \
   popl %ebp



/* sets up the inner sprite drawing loop, loads registers, etc */
#define SPRITE_LOOP(name)                                                    \
sprite_y_loop_##name:                                                      ; \
   movl S_Y, %eax                         /* load line */                  ; \
   WRITE_BANK()                           /* select bank */                ; \
   addl S_X, %eax                         /* add x offset */               ; \
   movl S_W, %ecx                         /* x loop counter */             ; \
									   ; \
   .align 4, 0x90                                                          ; \
sprite_x_loop_##name:



/* ends the inner (x) part of a sprite drawing loop */
#define SPRITE_END_X(name)                                                   \
   decl %ecx                                                               ; \
   jg sprite_x_loop_##name



/* ends the outer (y) part of a sprite drawing loop */
#define SPRITE_END_Y(name)                                                   \
   addl S_SGAP, %esi                      /* skip sprite bytes */          ; \
   incl S_Y                               /* next line */                  ; \
   decl S_H                               /* loop counter */               ; \
   jg sprite_y_loop_##name



/* sets up the inner translucent sprite drawing loop, loads registers, etc */
#define T_SPRITE_LOOP(name)                                                  \
sprite_y_loop_##name:                                                      ; \
   movl S_BMP, %edx                       /* load bitmap pointer */        ; \
   movl S_Y, %eax                         /* load line */                  ; \
   READ_BANK()                            /* select read bank */           ; \
   movl %eax, %ecx                        /* read address in ecx */        ; \
   movl S_Y, %eax                         /* reload line */                ; \
   WRITE_BANK()                           /* select write bank */          ; \
   subl %eax, %ecx                        /* convert ecx to offset */      ; \
   addl S_X, %eax                         /* add x offset */               ; \
   movl S_W, %edx                         /* x loop counter */             ; \
   movl %edx, S_C                         /* store */                      ; \
									   ; \
   .align 4, 0x90                                                          ; \
sprite_x_loop_##name:



/* sets up the inner truecolor translucent sprite drawing loop */
#define TT_SPRITE_LOOP(name)                                                 \
sprite_y_loop_##name:                                                      ; \
   movl S_BMP, %edx                       /* load bitmap pointer */        ; \
   movl S_Y, %eax                         /* load line */                  ; \
   READ_BANK()                            /* select read bank */           ; \
   movl %eax, %ecx                        /* read address in ecx */        ; \
   movl S_Y, %eax                         /* reload line */                ; \
   WRITE_BANK()                           /* select write bank */          ; \
   subl %eax, %ecx                        /* convert ecx to offset */      ; \
   movl S_W, %edx                         /* x loop counter */             ; \
   addl S_X, %eax                         /* add x offset */               ; \
   movl %edx, S_C                         /* store */                      ; \
   movl %eax, %ebx                        /* move dest address */          ; \
									   ; \
   .align 4, 0x90                                                          ; \
sprite_x_loop_##name:



/* sets up the inner truecolor lit sprite drawing loop */
#define LT_SPRITE_LOOP(name)                                                 \
sprite_y_loop_##name:                                                      ; \
   movl S_BMP, %edx                       /* load bitmap pointer */        ; \
   movl S_Y, %eax                         /* load line */                  ; \
   WRITE_BANK()                           /* select write bank */          ; \
   movl S_W, %edx                         /* x loop counter */             ; \
   addl S_X, %eax                         /* add x offset */               ; \
   movl %edx, S_C                         /* store */                      ; \
   movl %eax, %ebx                        /* move dest address */          ; \
									   ; \
   .align 4, 0x90                                                          ; \
sprite_x_loop_##name:



/* ends the inner (x) part of a translucent sprite drawing loop */
#define T_SPRITE_END_X(name)                                                 \
   decl S_C                                                                ; \
   jg sprite_x_loop_##name




/* generic framework for constructing RLE sprite drawing routines, shared 
 * between the 8, 16, 24, and 32 bit versions of the code...
 */


#define R_BMP           ARG1
#define R_SPRITE        ARG2
#define R_X             ARG3
#define R_Y             ARG4
#define R_COLOR         ARG5

#define R_LGAP          -4(%ebp)
#define R_W             -8(%ebp)
#define R_H             -12(%ebp)
#define R_TMP           -16(%ebp)
#define R_TMP2          -20(%ebp)



/* helper macro for drawing RLE sprites */
#define DO_RLE(name, bpp, suf, areg, eolmarker)                              \
   pushl %ebp                                                              ; \
   movl %esp, %ebp                                                         ; \
   subl $20, %esp                                                          ; \
									   ; \
   pushl %ebx                                                              ; \
   pushl %esi                                                              ; \
   pushl %edi                                                              ; \
   pushw %es                                                               ; \
									   ; \
   movl $0, R_LGAP               /* normally zero gap on left */           ; \
   movl R_SPRITE, %esi           /* esi = sprite pointer */                ; \
   movl RLE_W(%esi), %eax        /* read sprite width */                   ; \
   movl %eax, R_W                                                          ; \
   movl RLE_H(%esi), %eax        /* read sprite height */                  ; \
   movl %eax, R_H                                                          ; \
   addl $RLE_DAT, %esi           /* points to start of RLE data */         ; \
									   ; \
   movl R_BMP, %edx              /* edx = bitmap pointer */                ; \
   movw BMP_SEG(%edx), %es       /* select segment */                      ; \
   cld                                                                     ; \
									   ; \
   cmpl $0, BMP_CLIP(%edx)       /* test clip flag */                      ; \
   je name##_noclip                                                        ; \
									   ; \
   movl R_Y, %ecx                /* ecx = Y */                             ; \
									   ; \
name##_clip_top:                                                           ; \
   cmpl %ecx, BMP_CT(%edx)       /* test top clipping */                   ; \
   jle name##_top_ok                                                       ; \
									   ; \
   incl %ecx                     /* increment Y */                         ; \
   decl R_H                      /* decrement height */                    ; \
   jle name##_done                                                         ; \
									   ; \
   .align 4, 0x90                                                          ; \
name##_clip_top_loop:                                                      ; \
   lods##suf                     /* find zero EOL marker in RLE data */    ; \
   cmp##suf eolmarker, areg                                                ; \
   jne name##_clip_top_loop                                                ; \
									   ; \
   jmp name##_clip_top                                                     ; \
									   ; \
   .align 4, 0x90                                                          ; \
name##_top_ok:                                                             ; \
   movl %ecx, R_Y                /* store clipped Y */                     ; \
									   ; \
   addl R_H, %ecx                /* ecx = Y + height */                    ; \
   subl BMP_CB(%edx), %ecx       /* test bottom clipping */                ; \
   jl name##_bottom_ok                                                     ; \
									   ; \
   subl %ecx, R_H                /* clip on the bottom */                  ; \
   jle name##_done                                                         ; \
									   ; \
   .align 4, 0x90                                                          ; \
name##_bottom_ok:                                                          ; \
   movl BMP_CL(%edx), %eax       /* check left clipping */                 ; \
   subl R_X, %eax                                                          ; \
   jle name##_left_ok                                                      ; \
									   ; \
   movl %eax, R_LGAP             /* clip on the left */                    ; \
   addl %eax, R_X                                                          ; \
   subl %eax, R_W                                                          ; \
   jle name##_done                                                         ; \
									   ; \
   .align 4, 0x90                                                          ; \
name##_left_ok:                                                            ; \
   movl R_X, %eax                /* check right clipping */                ; \
   addl R_W, %eax                                                          ; \
   subl BMP_CR(%edx), %eax                                                 ; \
   jle name##_no_right_clip                                                ; \
									   ; \
   subl %eax, R_W                                                          ; \
   jl name##_done                                                          ; \
   jmp name##_clip_y_loop                                                  ; \
									   ; \
   .align 4, 0x90                                                          ; \
name##_no_right_clip:                                                      ; \
   cmpl $0, R_LGAP               /* can we use the fast noclip drawer? */  ; \
   je name##_noclip                                                        ; \
									   ; \
									   ; \
   /* slower version of the drawer for sprites that need clipping */       ; \
   .align 4, 0x90                                                          ; \
name##_clip_y_loop:                                                        ; \
   INIT_RLE_LINE()                                                         ; \
									   ; \
   movl R_W, %ebx                                                          ; \
   movl R_LGAP, %ecx                                                       ; \
									   ; \
name##_clip_lgap_loop:                                                     ; \
   lods##suf                     /* read a command byte */                 ; \
   test##suf areg, areg          /* and test it */                         ; \
   js name##_clip_lgap_zeros                                               ; \
									   ; \
   RLE_ZEX_EAX()                 /* skip a solid run */                    ; \
   leal (%esi, %eax, bpp), %esi                                            ; \
   subl %eax, %ecx                                                         ; \
   jge name##_clip_lgap_loop                                               ; \
									   ; \
   leal (%esi, %ecx, bpp), %esi  /* oops, we overshot */                   ; \
   negl %ecx                                                               ; \
   movl %ecx, %eax                                                         ; \
   jmp name##_clip_x_loop                                                  ; \
									   ; \
   .align 4, 0x90                                                          ; \
name##_clip_lgap_zeros:                                                    ; \
   RLE_SEX_EAX()                 /* skip a run of zeros */                 ; \
   addl %eax, %ecx                                                         ; \
   jge name##_clip_lgap_loop                                               ; \
									   ; \
   movl %ecx, %eax               /* oops, we overshot */                   ; \
									   ; \
   .align 4, 0x90                                                          ; \
name##_clip_x_loop:                                                        ; \
   TEST_RLE_COMMAND(name##_clip_x_done, name##_clip_skip_zeros)            ; \
									   ; \
   RLE_ZEX_ECX()                 /* write a string of pixels */            ; \
   subl %ecx, %ebx                                                         ; \
   jle name##_clip_string                                                  ; \
									   ; \
   SLOW_RLE_RUN(0)                                                         ; \
   lods##suf                     /* read next command byte */              ; \
   jmp name##_clip_x_loop                                                  ; \
									   ; \
   .align 4, 0x90                                                          ; \
name##_clip_string:                                                        ; \
   addl %ebx, %ecx               /* only write part of the string */       ; \
   jle name##_clip_altogether                                              ; \
   SLOW_RLE_RUN(1)                                                         ; \
name##_clip_altogether:                                                    ; \
   negl %ebx                                                               ; \
   leal (%esi, %ebx, bpp), %esi                                            ; \
   jmp name##_clip_skip_rgap                                               ; \
									   ; \
   .align 4, 0x90                                                          ; \
name##_clip_skip_zeros:                                                    ; \
   RLE_SEX_EAX()                 /* skip over a string of zeros */         ; \
   negl %eax                                                               ; \
   ADD_EAX_EDI()                                                           ; \
   subl %eax, %ebx                                                         ; \
   jle name##_clip_skip_rgap                                               ; \
									   ; \
   lods##suf                     /* read next command byte */              ; \
   jmp name##_clip_x_loop                                                  ; \
									   ; \
   .align 4, 0x90                                                          ; \
name##_clip_skip_rgap:                                                     ; \
   lods##suf                     /* skip forward to zero EOL marker */     ; \
   cmp##suf eolmarker, areg                                                ; \
   jne name##_clip_skip_rgap                                               ; \
									   ; \
name##_clip_x_done:                                                        ; \
   incl R_Y                                                                ; \
   decl R_H                                                                ; \
   jg name##_clip_y_loop                                                   ; \
   jmp name##_done                                                         ; \
									   ; \
									   ; \
   /* fast drawer for sprites that don't need clipping */                  ; \
   .align 4, 0x90                                                          ; \
name##_noclip:                                                             ; \
   INIT_FAST_RLE_LOOP()                                                    ; \
									   ; \
   .align 4, 0x90                                                          ; \
name##_noclip_y_loop:                                                      ; \
   INIT_RLE_LINE()                                                         ; \
									   ; \
   .align 4, 0x90                                                          ; \
name##_noclip_x_loop:                                                      ; \
   lods##suf                     /* read a command byte */                 ; \
   TEST_RLE_COMMAND(name##_noclip_x_done, name##_noclip_skip_zeros)        ; \
									   ; \
   RLE_ZEX_ECX()                 /* write a string of pixels */            ; \
   FAST_RLE_RUN()                                                          ; \
   jmp name##_noclip_x_loop                                                ; \
									   ; \
   .align 4, 0x90                                                          ; \
name##_noclip_skip_zeros:                                                  ; \
   neg##suf areg                 /* skip over a string of zeros */         ; \
   RLE_ZEX_EAX()                                                           ; \
   ADD_EAX_EDI()                                                           ; \
   jmp name##_noclip_x_loop                                                ; \
									   ; \
   .align 4, 0x90                                                          ; \
name##_noclip_x_done:                                                      ; \
   incl R_Y                                                                ; \
   decl R_H                                                                ; \
   jg name##_noclip_y_loop                                                 ; \
									   ; \
									   ; \
name##_done:                                                               ; \
   popw %es                                                                ; \
   popl %edi                                                               ; \
   popl %esi                                                               ; \
   popl %ebx                                                               ; \
									   ; \
   movl %ebp, %esp                                                         ; \
   popl %ebp                     /* finshed drawing an RLE sprite */




/* generic framework for constructing text drawing routines, shared 
 * between the 8, 16, 24, and 32 bit versions of the code...
 */


#define T_BMP           ARG1
#define T_FONT          ARG2
#define T_CHARHEIGHT    ARG3
#define T_STR           ARG4
#define T_X             ARG5
#define T_Y             ARG6
#define T_COLOR         ARG7

#define T_TGAP          -4(%ebp)
#define T_HEIGHT        -8(%ebp)
#define T_LGAP          -12(%ebp)
#define T_RGAP          -16(%ebp)
#define T_CHAR_W        -20(%ebp)
#define T_C             -24(%ebp)
#define T_FONT_H        -28(%ebp)



/* does the work of drawing a text string onto a bitmap */
#define DRAW_TEXT()                                                          \
   movl $1, %eax                                                           ; \
   movl T_CHARHEIGHT, %ecx       /* load font height shift value */        ; \
   shll %cl, %eax                                                          ; \
   movl %eax, T_FONT_H           /* store character height */              ; \
									   ; \
   movl T_BMP, %esi              /* esi = bmp */                           ; \
   movw BMP_SEG(%esi), %es       /* segment selector */                    ; \
									   ; \
   cmpl $0, BMP_CLIP(%esi)       /* test bmp->clip */                      ; \
   jz text_no_clip                                                         ; \
									   ; \
   movl T_Y, %eax                /* eax = y */                             ; \
   movl BMP_CT(%esi), %edx       /* edx = bmp->ct */                       ; \
   subl %eax, %edx               /* edx -= y */                            ; \
   cmpl T_FONT_H, %edx                                                     ; \
   jge text_done                                                           ; \
   orl %edx, %edx                                                          ; \
   jge text_tgap_pve                                                       ; \
   xorl %edx, %edx                                                         ; \
text_tgap_pve:                                                             ; \
   movl %edx, T_TGAP             /* set tgap */                            ; \
   addl %edx, %eax               /* y += tgap */                           ; \
   movl %eax, T_Y                /* store y */                             ; \
									   ; \
   movl BMP_CB(%esi), %ebx       /* ebx = bmp->cb */                       ; \
   subl %eax, %ebx               /* ebx -= y */                            ; \
   jle text_done                                                           ; \
									   ; \
   negl %edx                                                               ; \
   addl T_FONT_H, %edx           /* edx = font height - tgap */            ; \
   cmpl %ebx, %edx               /* if height > font height - tgap */      ; \
   jg text_height_small                                                    ; \
   movl %edx, %ebx                                                         ; \
text_height_small:                                                         ; \
   movl %ebx, T_HEIGHT           /* store height */                        ; \
									   ; \
   movl BMP_CL(%esi), %eax       /* eax = bmp->cl */                       ; \
   movl T_X, %edx                /* edx = x */                             ; \
   subl %edx, %eax               /* eax = bmp->cl - x */                   ; \
   jge text_lgap_pve                                                       ; \
   xorl %eax, %eax                                                         ; \
text_lgap_pve:                                                             ; \
   movl T_STR, %ebx              /* ebx = str */                           ; \
text_lclip_loop:                                                           ; \
   cmpl $8, %eax                 /* while eax >= 8 */                      ; \
   jl text_lclip_done                                                      ; \
   cmpb $0, (%ebx)               /* if !*str */                            ; \
   jz text_done                                                            ; \
   incl %ebx                                                               ; \
   addl $8, T_X                                                            ; \
   subl $8, %eax                                                           ; \
   jmp text_lclip_loop                                                     ; \
									   ; \
   .align 4, 0x90                                                          ; \
text_lclip_done:                                                           ; \
   movl %ebx, T_STR              /* store str */                           ; \
   movl %eax, T_LGAP             /* store lgap */                          ; \
									   ; \
   movl T_X, %eax                /* x */                                   ; \
   movl BMP_CR(%esi), %edx       /* bmp->cr */                             ; \
   subl %eax, %edx                                                         ; \
   jl text_done                                                            ; \
   movl %edx, T_RGAP             /* set rgap */                            ; \
   jmp text_char_loop_start                                                ; \
									   ; \
   .align 4, 0x90                                                          ; \
text_no_clip:                                                              ; \
   movl $0, T_TGAP                                                         ; \
   movl T_FONT_H, %eax                                                     ; \
   movl %eax, T_HEIGHT                                                     ; \
   movl $0, T_LGAP                                                         ; \
   movl $0x7fff, T_RGAP                                                    ; \
   jmp text_char_loop_start                                                ; \
									   ; \
   .align 4, 0x90                                                          ; \
text_char_loop:                  /* for each char (already in %al) */      ; \
   andl $0xff, %eax                                                        ; \
   subl $32, %eax                /* convert char to table offset */        ; \
   jge text_char_sorted                                                    ; \
									   ; \
   xorl %eax, %eax               /* oops - not ASCII */                    ; \
									   ; \
   .align 4, 0x90                                                          ; \
text_char_sorted:                                                          ; \
   movl T_FONT, %esi             /* esi = font */                          ; \
   movl T_CHARHEIGHT, %ecx                                                 ; \
   shll %cl, %eax                                                          ; \
   addl %eax, %esi                                                         ; \
   addl T_TGAP, %esi             /* esi = position in font bitmap */       ; \
									   ; \
   movl T_RGAP, %eax             /* rgap */                                ; \
   cmpl $0, %eax                                                           ; \
   jle text_done                 /* have we gone off the right? */         ; \
   cmpl $8, %eax                                                           ; \
   jle text_rgap_ok                                                        ; \
   movl $8, %eax                 /* dont want chars wider than 8! */       ; \
text_rgap_ok:                                                              ; \
   movl %eax, T_CHAR_W           /* set char width */                      ; \
   xorl %ebx, %ebx                                                         ; \
   jmp text_y_loop_start                                                   ; \
									   ; \
   .align 4, 0x90                                                          ; \
text_y_loop:                     /* for each y... */                       ; \
   addl T_Y, %ebx                /* add y, c will already be in ebx */     ; \
									   ; \
   movl T_BMP, %edx              /* bmp */                                 ; \
   movl %ebx, %eax               /* line number */                         ; \
   WRITE_BANK()                  /* get bank */                            ; \
   movl %eax, %edi                                                         ; \
									   ; \
   movl T_X, %edx                /* x */                                   ; \
   GET_ADDR(%edi, %edx)                                                    ; \
									   ; \
   movb (%esi), %dl              /* dl = bit mask */                       ; \
   incl %esi                                                               ; \
									   ; \
   movl T_LGAP, %ecx             /* lgap */                                ; \
   orl %ecx, %ecx                                                          ; \
   jz text_no_lgap               /* do we need to clip on the left? */     ; \
									   ; \
   shlb %cl, %dl                 /* shift the mask */                      ; \
   GET_ADDR(%edi, %ecx)          /* move the screen position */            ; \
   negl %ecx                                                               ; \
									   ; \
   .align 4, 0x90                                                          ; \
text_no_lgap:                                                              ; \
   addl T_CHAR_W, %ecx           /* ecx = x loop counter */                ; \
   jle text_no_x_loop                                                      ; \
									   ; \
   START_X_LOOP()                                                          ; \
									   ; \
   .align 4, 0x90                                                          ; \
text_x_loop:                     /* for each x... */                       ; \
   shlb $1, %dl                  /* shift the mask */                      ; \
   jc text_put_bit                                                         ; \
									   ; \
   orl %ebx, %ebx                                                          ; \
   jl text_put_done                                                        ; \
   PUTB()                        /* draw background pixel */               ; \
   jmp text_put_done                                                       ; \
									   ; \
   .align 4, 0x90                                                          ; \
text_put_bit:                                                              ; \
   PUTA()                        /* draw a pixel */                        ; \
									   ; \
text_put_done:                                                             ; \
   NEXTDEST()                                                              ; \
   decl %ecx                                                               ; \
   jg text_x_loop                /* and loop */                            ; \
									   ; \
   END_X_LOOP()                                                            ; \
									   ; \
text_no_x_loop:                                                            ; \
   movl T_C, %ebx                /* increment loop counter */              ; \
   incl %ebx                                                               ; \
text_y_loop_start:                                                         ; \
   movl %ebx, T_C                                                          ; \
   cmpl T_HEIGHT, %ebx                                                     ; \
   jl text_y_loop                                                          ; \
									   ; \
text_y_loop_done:                                                          ; \
   movl $0, T_LGAP               /* sort out a load of variables */        ; \
   subl $8, T_RGAP                                                         ; \
   addl $8, T_X                                                            ; \
									   ; \
   incl T_STR                    /* move on to the next character */       ; \
text_char_loop_start:                                                      ; \
   movl T_STR, %ebx              /* read a char into al */                 ; \
   movb (%ebx), %al                                                        ; \
   orb %al, %al                                                            ; \
   jz text_done                                                            ; \
   jmp text_char_loop                                                      ; \
									   ; \
text_done:




#endif          /* ifndef SPRITE_INC */

