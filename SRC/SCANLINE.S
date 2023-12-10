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
 *      Polygon scanline filler helpers (gouraud shading, tmapping, etc).
 *
 *      See readme.txt for copyright information.
 */


#include "asmdefs.inc"

.text



/* all these functions share the same parameters */

#define ADDR      ARG1
#define W         ARG2
#define INFO      ARG3



/* void _poly_scanline_flat(unsigned long addr, int w, POLYGON_SEGMENT *info);
 *  Fills a flat shaded polygon scanline.
 */
.globl __poly_scanline_flat
   .align 4
__poly_scanline_flat:
   pushl %ebp
   movl %esp, %ebp
   pushl %edi
   pushw %es

   movl INFO, %edi               /* load registers */
   movl POLYSEG_SEG(%edi), %eax
   movw %ax, %es
   movl POLYSEG_C(%edi), %eax
   movl W, %ecx
   movl ADDR, %edi

   testl $1, %edi                 /* are we byte aligned? */
   jz flat_byte_aligned

   stosb                         /* copy an odd byte */
   decl %ecx
   jle flat_done

flat_byte_aligned:
   testl $2, %edi                /* are we word aligned? */
   jz flat_word_aligned

   subl $2, %ecx 
   jl flat_no_odd_word
   stosw                         /* copy an odd word */
   jz flat_done

flat_word_aligned:
   movl %ecx, %edx               /* do we need a string copy? */
   shrl $2, %ecx
   jz flat_no_long

   rep ; stosl                   /* 32 bit copy */

flat_no_long:
   testl $2, %edx                /* is there a word left over? */
   jz flat_no_leftover_word

   stosw                         /* copy a leftover word */

flat_no_leftover_word:
   testl $1, %edx                /* is there a byte left over? */
   jz flat_done

   stosb                         /* copy a leftover byte */

flat_done:
   popw %es
   popl %edi
   movl %ebp, %esp
   popl %ebp
   ret                           /* end of _poly_scanline_flat() */

flat_no_odd_word:
   addl $2, %ecx
   movl %ecx, %edx
   jmp flat_no_long




/* void _poly_scanline_gcol(unsigned long addr, int w, POLYGON_SEGMENT *info);
 *  Fills a single-color gouraud shaded polygon scanline.
 */
.globl __poly_scanline_gcol
   .align 4
__poly_scanline_gcol:
   pushl %ebp
   movl %esp, %ebp
   pushl %ebx
   pushl %esi
   pushl %edi

   movl INFO, %edi               /* load registers */
   movl POLYSEG_C(%edi), %edx
   movl POLYSEG_DC(%edi), %esi
   movl W, %ecx
   movl ADDR, %edi

   .align 4, 0x90
gcol_odd_byte_loop:
   testl $3, %edi                /* deal with any odd pixels */
   jz gcol_no_odd_bytes

   movl %edx, %eax 
   shrl $16, %eax
   movb %al, %fs:(%edi)
   addl %esi, %edx
   incl %edi
   decl %ecx
   jg gcol_odd_byte_loop

   .align 4, 0x90
gcol_no_odd_bytes:
   movl %ecx, W                  /* inner loop expanded 4 times */
   shrl $2, %ecx 
   jz gcol_cleanup_leftovers

   .align 4, 0x90
gcol_loop:
   movl %edx, %eax               /* pixel 1 */
   shrl $16, %eax
   addl %esi, %edx
   movb %al, %bl

   movl %edx, %eax               /* pixel 2 */
   shrl $16, %eax
   addl %esi, %edx
   movb %al, %bh

   shll $16, %ebx

   movl %edx, %eax               /* pixel 3 */
   shrl $16, %eax
   addl %esi, %edx
   movb %al, %bl

   movl %edx, %eax               /* pixel 4 */
   shrl $16, %eax
   addl %esi, %edx
   movb %al, %bh

   roll $16, %ebx
   movl %ebx, %fs:(%edi)         /* write four pixels at a time */
   addl $4, %edi
   decl %ecx
   jg gcol_loop

gcol_cleanup_leftovers:
   movl W, %ecx                  /* deal with any leftover pixels */
   andl $3, %ecx
   jz gcol_done

   .align 4, 0x90
gcol_leftover_byte_loop:
   movl %edx, %eax 
   shrl $16, %eax
   movb %al, %fs:(%edi)
   addl %esi, %edx
   incl %edi
   decl %ecx
   jg gcol_leftover_byte_loop

gcol_done:
   popl %edi
   popl %esi
   popl %ebx
   movl %ebp, %esp
   popl %ebp
   ret                           /* end of _poly_scanline_gcol() */




/* void _poly_scanline_grgb(unsigned long addr, int w, POLYGON_SEGMENT *info);
 *  Fills an RGB gouraud shaded polygon scanline.
 */
.globl __poly_scanline_grgb
   .align 4
__poly_scanline_grgb:
   pushl %ebp
   movl %esp, %ebp
   subl $12, %esp                /* three local variables: */

   #define DR     -4(%ebp)
   #define DG     -8(%ebp)
   #define DB     -12(%ebp)

   pushl %ebx
   pushl %esi
   pushl %edi

   movl INFO, %esi               /* load registers */

   movl POLYSEG_DR(%esi), %eax
   movl POLYSEG_DG(%esi), %ebx
   movl POLYSEG_DB(%esi), %ecx

   movl %eax, DR
   movl %ebx, DG
   movl %ecx, DB

   movl POLYSEG_R(%esi), %ebx
   movl POLYSEG_G(%esi), %ecx
   movl POLYSEG_B(%esi), %edx

   movl ADDR, %edi

   .align 4, 0x90
grgb_loop:
   movl %ecx, %eax               /* green */
   movl %ebx, %esi               /* red */
   shrl $19, %eax                /* green */
   shrl $19, %esi                /* red */
   shll $5, %eax                 /* green */
   shll $10, %esi                /* red */
   orl %eax, %esi                /* green */
   movl %edx, %eax               /* blue */
   shrl $19, %eax                /* blue */
   orl %eax, %esi                /* blue */

   movl _rgb_map, %eax           /* table lookup */
   movb (%eax, %esi), %al
   movb %al, %fs:(%edi)          /* write the pixel */
   incl %edi
   addl DR, %ebx
   addl DG, %ecx
   addl DB, %edx
   decl W
   jg grgb_loop

grgb_done:
   popl %edi
   popl %esi
   popl %ebx
   movl %ebp, %esp
   popl %ebp
   ret                           /* end of _poly_scanline_grgb() */




#define DU        -4(%ebp)
#define DV        -8(%ebp)
#define UMASK     -12(%ebp)
#define VMASK     -16(%ebp)
#define VSHIFT    -20(%ebp)
#define TMP       -24(%ebp)
#define COUNT     -28(%ebp)
#define C         -32(%ebp)
#define DC        -36(%ebp)
#define TEXTURE   -40(%ebp)




/* helper for setting up an affine texture mapping operation */
#define INIT_ATEX(extra...)                                                  \
   pushl %ebp                                                              ; \
   movl %esp, %ebp                                                         ; \
   subl $40, %esp                /* ten local variables */                 ; \
									   ; \
   pushl %ebx                                                              ; \
   pushl %esi                                                              ; \
   pushl %edi                                                              ; \
									   ; \
   movl INFO, %esi               /* load registers */                      ; \
									   ; \
   extra                                                                   ; \
									   ; \
   movl POLYSEG_DU(%esi), %eax                                             ; \
   movl POLYSEG_DV(%esi), %ebx                                             ; \
   movl POLYSEG_VSHIFT(%esi), %ecx                                         ; \
   movl POLYSEG_UMASK(%esi), %edx                                          ; \
   movl POLYSEG_VMASK(%esi), %edi                                          ; \
									   ; \
   shll %cl, %edi                /* adjust v mask and shift value */       ; \
   negl %ecx                                                               ; \
   addl $16, %ecx                                                          ; \
									   ; \
   movl %eax, DU                                                           ; \
   movl %ebx, DV                                                           ; \
   movl %ecx, VSHIFT                                                       ; \
   movl %edx, UMASK                                                        ; \
   movl %edi, VMASK                                                        ; \
									   ; \
   movl POLYSEG_U(%esi), %ebx                                              ; \
   movl POLYSEG_V(%esi), %edx                                              ; \
									   ; \
   movl ADDR, %edi                                                         ; \
   movl POLYSEG_TEXTURE(%esi), %esi




/* void _poly_scanline_atex(unsigned long addr, int w, POLYGON_SEGMENT *info);
 *  Fills an affine texture mapped polygon scanline.
 */
.globl __poly_scanline_atex
   .align 4
__poly_scanline_atex:
   INIT_ATEX()

   .align 4, 0x90
atex_odd_byte_loop:
   testl $3, %edi                /* deal with any odd pixels */
   jz atex_no_odd_bytes

   movl %edx, %eax               /* get v */ 
   movb VSHIFT, %cl
   sarl %cl, %eax
   andl VMASK, %eax

   movl %ebx, %ecx               /* get u */
   sarl $16, %ecx
   andl UMASK, %ecx
   addl %ecx, %eax

   movb (%esi, %eax), %al        /* read texel */
   movb %al, %fs:(%edi)          /* write the pixel */
   addl DU, %ebx
   addl DV, %edx
   incl %edi
   decl W
   jg atex_odd_byte_loop

   .align 4, 0x90
atex_no_odd_bytes:
   movl W, %ecx                  /* inner loop expanded 4 times */
   movl %ecx, COUNT
   shrl $2, %ecx 
   jz atex_cleanup_leftovers
   movl %ecx, W

   .align 4, 0x90
atex_loop:
   /* pixel 1 */
   movl %edx, %eax               /* get v */ 
   movb VSHIFT, %cl
   sarl %cl, %eax
   andl VMASK, %eax

   movl %ebx, %ecx               /* get u */
   sarl $16, %ecx
   andl UMASK, %ecx
   addl %ecx, %eax

   movb (%esi, %eax), %cl        /* read texel */
   addl DU, %ebx
   addl DV, %edx
   movb %cl, TMP

   /* pixel 2 */
   movl %edx, %eax               /* get v */ 
   movb VSHIFT, %cl
   sarl %cl, %eax
   andl VMASK, %eax

   movl %ebx, %ecx               /* get u */
   sarl $16, %ecx
   andl UMASK, %ecx
   addl %ecx, %eax

   movb (%esi, %eax), %cl        /* read texel */
   addl DU, %ebx
   addl DV, %edx
   movb %cl, 1+TMP

   /* pixel 3 */
   movl %edx, %eax               /* get v */ 
   movb VSHIFT, %cl
   sarl %cl, %eax
   andl VMASK, %eax

   movl %ebx, %ecx               /* get u */
   sarl $16, %ecx
   andl UMASK, %ecx
   addl %ecx, %eax

   movb (%esi, %eax), %cl        /* read texel */
   addl DU, %ebx
   addl DV, %edx
   movb %cl, 2+TMP

   /* pixel 4 */
   movl %edx, %eax               /* get v */ 
   movb VSHIFT, %cl
   sarl %cl, %eax
   andl VMASK, %eax

   movl %ebx, %ecx               /* get u */
   sarl $16, %ecx
   andl UMASK, %ecx
   addl %ecx, %eax

   movb (%esi, %eax), %cl        /* read texel */
   addl DU, %ebx
   addl DV, %edx
   movb %cl, 3+TMP

   movl TMP, %eax                /* write four pixels */
   movl %eax, %fs:(%edi) 
   addl $4, %edi
   decl W
   jg atex_loop

atex_cleanup_leftovers:
   movl COUNT, %ecx              /* deal with any leftover pixels */
   andl $3, %ecx
   jz atex_done
   movl %ecx, COUNT

   .align 4, 0x90
atex_leftover_byte_loop:
   movl %edx, %eax               /* get v */ 
   movb VSHIFT, %cl
   sarl %cl, %eax
   andl VMASK, %eax

   movl %ebx, %ecx               /* get u */
   sarl $16, %ecx
   andl UMASK, %ecx
   addl %ecx, %eax

   movb (%esi, %eax), %al        /* read texel */
   movb %al, %fs:(%edi)          /* write the pixel */
   addl DU, %ebx
   addl DV, %edx
   incl %edi
   decl COUNT
   jg atex_leftover_byte_loop

atex_done:
   popl %edi
   popl %esi
   popl %ebx
   movl %ebp, %esp
   popl %ebp
   ret                           /* end of _poly_scanline_atex() */




/* void _poly_scanline_atex_mask(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a masked affine texture mapped polygon scanline.
 */
.globl __poly_scanline_atex_mask
   .align 4
__poly_scanline_atex_mask:
   INIT_ATEX()

   .align 4, 0x90
atex_mask_loop:
   movl %edx, %eax               /* get v */ 
   movb VSHIFT, %cl
   sarl %cl, %eax
   andl VMASK, %eax

   movl %ebx, %ecx               /* get u */
   sarl $16, %ecx
   andl UMASK, %ecx
   addl %ecx, %eax

   movb (%esi, %eax), %al        /* read texel */
   orb %al, %al
   jz atex_mask_skip

   movb %al, %fs:(%edi)          /* write solid pixels */
   addl DU, %ebx
   addl DV, %edx
   incl %edi
   decl W
   jg atex_mask_loop
   jmp atex_mask_done

   .align 4, 0x90
atex_mask_skip:
   addl DU, %ebx                 /* skip zero pixels */
   addl DV, %edx
   incl %edi
   decl W
   jg atex_mask_loop

atex_mask_done:
   popl %edi
   popl %esi
   popl %ebx
   movl %ebp, %esp
   popl %ebp
   ret                           /* end of _poly_scanline_atex_mask() */




/* void _poly_scanline_atex_lit(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a lit affine texture mapped polygon scanline.
 */
.globl __poly_scanline_atex_lit
   .align 4
__poly_scanline_atex_lit:
   INIT_ATEX(
      movl POLYSEG_C(%esi), %eax
      movl POLYSEG_DC(%esi), %ebx
      sarl $8, %eax
      sarl $8, %ebx
      movl %eax, C
      movl %ebx, DC
   )

   movl %esi, TEXTURE

   .align 4, 0x90
atex_lit_odd_byte_loop:
   testl $3, %edi                /* deal with any odd pixels */
   jz atex_lit_no_odd_bytes

   movl %edx, %eax               /* get v */ 
   movb VSHIFT, %cl
   sarl %cl, %eax
   andl VMASK, %eax

   movl %ebx, %ecx               /* get u */
   sarl $16, %ecx
   andl UMASK, %ecx
   addl %ecx, %eax

   movl TEXTURE, %esi            /* read texel */
   movb (%esi, %eax), %al 
   addl DU, %ebx
   addl DV, %edx

   movb 1+C, %ah                 /* lookup in lighting table */
   movl _color_map, %esi
   movl DC, %ecx
   movb (%eax, %esi), %al
   addl %ecx, C

   movb %al, %fs:(%edi)          /* write the pixel */
   incl %edi
   decl W
   jg atex_lit_odd_byte_loop

   .align 4, 0x90
atex_lit_no_odd_bytes:
   movl W, %ecx                  /* inner loop expanded 4 times */
   movl %ecx, COUNT
   shrl $2, %ecx 
   jz atex_lit_cleanup_leftovers
   movl %ecx, W

   .align 4, 0x90
atex_lit_loop:
   movl TEXTURE, %esi            /* read four texels */

   /* pixel 1 texture */
   movl %edx, %eax               /* get v */ 
   movb VSHIFT, %cl
   sarl %cl, %eax
   andl VMASK, %eax

   movl %ebx, %ecx               /* get u */
   sarl $16, %ecx
   andl UMASK, %ecx
   addl %ecx, %eax

   movb (%esi, %eax), %cl        /* read texel */
   addl DU, %ebx
   addl DV, %edx
   movb %cl, TMP

   /* pixel 2 texture */
   movl %edx, %eax               /* get v */ 
   movb VSHIFT, %cl
   sarl %cl, %eax
   andl VMASK, %eax

   movl %ebx, %ecx               /* get u */
   sarl $16, %ecx
   andl UMASK, %ecx
   addl %ecx, %eax

   movb (%esi, %eax), %cl        /* read texel */
   addl DU, %ebx
   addl DV, %edx
   movb %cl, 1+TMP

   /* pixel 3 texture */
   movl %edx, %eax               /* get v */ 
   movb VSHIFT, %cl
   sarl %cl, %eax
   andl VMASK, %eax

   movl %ebx, %ecx               /* get u */
   sarl $16, %ecx
   andl UMASK, %ecx
   addl %ecx, %eax

   movb (%esi, %eax), %cl        /* read texel */
   addl DU, %ebx
   addl DV, %edx
   movb %cl, 2+TMP

   /* pixel 4 texture */
   movl %edx, %eax               /* get v */ 
   movb VSHIFT, %cl
   sarl %cl, %eax
   andl VMASK, %eax

   movl %ebx, %ecx               /* get u */
   sarl $16, %ecx
   andl UMASK, %ecx
   addl %ecx, %eax

   movb (%esi, %eax), %cl        /* read texel */
   addl DU, %ebx
   addl DV, %edx
   movb %cl, 3+TMP

   movl _color_map, %esi         /* light four pixels */
   xorl %ecx, %ecx
   pushl %ebx
   pushl %edx
   movl DC, %ebx
   movl C, %edx

   movb %dh, %ch                 /* light pixel 1 */
   movb TMP, %cl
   addl %ebx, %edx
   movb (%ecx, %esi), %al

   movb %dh, %ch                 /* light pixel 2 */
   movb 1+TMP, %cl
   addl %ebx, %edx
   movb (%ecx, %esi), %ah

   roll $16, %eax

   movb %dh, %ch                 /* light pixel 3 */
   movb 2+TMP, %cl
   addl %ebx, %edx
   movb (%ecx, %esi), %al

   movb %dh, %ch                 /* light pixel 4 */
   movb 3+TMP, %cl
   addl %ebx, %edx
   movb (%ecx, %esi), %ah

   movl %edx, C
   roll $16, %eax
   popl %edx
   popl %ebx
   movl %eax, %fs:(%edi)         /* write four pixels at a time */
   addl $4, %edi
   decl W
   jg atex_lit_loop

atex_lit_cleanup_leftovers:
   movl COUNT, %ecx              /* deal with any leftover pixels */
   andl $3, %ecx
   jz atex_lit_done
   movl %ecx, COUNT

   .align 4, 0x90
atex_lit_leftover_byte_loop:
   movl %edx, %eax               /* get v */ 
   movb VSHIFT, %cl
   sarl %cl, %eax
   andl VMASK, %eax

   movl %ebx, %ecx               /* get u */
   sarl $16, %ecx
   andl UMASK, %ecx
   addl %ecx, %eax

   movl TEXTURE, %esi            /* read texel */
   movb (%esi, %eax), %al 
   addl DU, %ebx
   addl DV, %edx

   movb 1+C, %ah                 /* lookup in lighting table */
   movl _color_map, %esi
   movl DC, %ecx
   movb (%eax, %esi), %al
   addl %ecx, C

   movb %al, %fs:(%edi)          /* write the pixel */
   incl %edi
   decl COUNT
   jg atex_lit_leftover_byte_loop

atex_lit_done:
   popl %edi
   popl %esi
   popl %ebx
   movl %ebp, %esp
   popl %ebp
   ret                           /* end of _poly_scanline_atex_lit() */




#undef DU
#undef DV
#undef UMASK
#undef VMASK
#undef VSHIFT
#undef TMP
#undef COUNT
#undef C
#undef DC
#undef TEXTURE
#undef INIT_ATEX




#define TMP       -4(%ebp)
#define UMASK     -8(%ebp)
#define VMASK     -12(%ebp)
#define VSHIFT    -16(%ebp)
#define DU1       -20(%ebp)
#define DV1       -24(%ebp)
#define DZ1       -28(%ebp)
#define DU4       -32(%ebp)
#define DV4       -36(%ebp)
#define DZ4       -40(%ebp)
#define UDIFF     -44(%ebp)
#define VDIFF     -48(%ebp)
#define PREVU     -52(%ebp)
#define PREVV     -56(%ebp)
#define COUNT     -60(%ebp)
#define C         -64(%ebp)
#define DC        -68(%ebp)
#define TEXTURE   -72(%ebp)




/* helper for starting an fpu 1/z division */
#define START_FP_DIV()                                                       \
   fld1                                                                    ; \
   fdiv %st(3), %st(0)



/* helper for ending an fpu division, returning corrected u and v values */
#define END_FP_DIV(ureg, vreg)                                               \
   fld %st(0)                    /* duplicate the 1/z value */             ; \
									   ; \
   fmul %st(3), %st(0)           /* divide u by z */                       ; \
   fxch %st(1)                                                             ; \
									   ; \
   fmul %st(2), %st(0)           /* divide v by z */                       ; \
									   ; \
   fistpl TMP                    /* store v */                             ; \
   movl TMP, vreg                                                          ; \
									   ; \
   fistpl TMP                    /* store u */                             ; \
   movl TMP, ureg



/* helper for updating the u, v, and z values */
#define UPDATE_FP_POS(n)                                                     \
   fadds DV##n                   /* update v coord */                      ; \
   fxch %st(1)                   /* swap vuz stack to uvz */               ; \
									   ; \
   fadds DU##n                   /* update u coord */                      ; \
   fxch %st(2)                   /* swap uvz stack to zvu */               ; \
									   ; \
   fadds DZ##n                   /* update z value */                      ; \
   fxch %st(2)                   /* swap zvu stack to uvz */               ; \
   fxch %st(1)                   /* swap uvz stack back to vuz */




/* main body of the perspective-correct texture mapping routine */
#define DO_PTEX(name)                                                        \
   pushl %ebp                                                              ; \
   movl %esp, %ebp                                                         ; \
   subl $72, %esp                /* eighteen local variables */            ; \
									   ; \
   pushl %ebx                                                              ; \
   pushl %esi                                                              ; \
   pushl %edi                                                              ; \
									   ; \
   movl INFO, %esi               /* load registers */                      ; \
									   ; \
   flds POLYSEG_Z(%esi)          /* z at bottom of fpu stack */            ; \
   flds POLYSEG_FU(%esi)         /* followed by u */                       ; \
   flds POLYSEG_FV(%esi)         /* followed by v */                       ; \
									   ; \
   flds POLYSEG_DFU(%esi)        /* multiply diffs by four */              ; \
   flds POLYSEG_DFV(%esi)                                                  ; \
   flds POLYSEG_DZ(%esi)                                                   ; \
   fxch %st(2)                   /* u v z */                               ; \
   fadd %st(0), %st(0)           /* 2u v z */                              ; \
   fxch %st(1)                   /* v 2u z */                              ; \
   fadd %st(0), %st(0)           /* 2v 2u z */                             ; \
   fxch %st(2)                   /* z 2u 2v */                             ; \
   fadd %st(0), %st(0)           /* 2z 2u 2v */                            ; \
   fxch %st(1)                   /* 2u 2z 2v */                            ; \
   fadd %st(0), %st(0)           /* 4u 2z 2v */                            ; \
   fxch %st(2)                   /* 2v 2z 4u */                            ; \
   fadd %st(0), %st(0)           /* 4v 2z 4u */                            ; \
   fxch %st(1)                   /* 2z 4v 4u */                            ; \
   fadd %st(0), %st(0)           /* 4z 4v 4u */                            ; \
   fxch %st(2)                   /* 4u 4v 4z */                            ; \
   fstps DU4                                                               ; \
   fstps DV4                                                               ; \
   fstps DZ4                                                               ; \
									   ; \
   START_FP_DIV()                /* prime the 1/z calculation */           ; \
									   ; \
   movl POLYSEG_DFU(%esi), %eax  /* copy diff values onto the stack */     ; \
   movl POLYSEG_DFV(%esi), %ebx                                            ; \
   movl POLYSEG_DZ(%esi), %ecx                                             ; \
   movl %eax, DU1                                                          ; \
   movl %ebx, DV1                                                          ; \
   movl %ecx, DZ1                                                          ; \
									   ; \
   movl POLYSEG_VSHIFT(%esi), %ecx                                         ; \
   movl POLYSEG_VMASK(%esi), %edx                                          ; \
   movl POLYSEG_UMASK(%esi), %eax                                          ; \
									   ; \
   shll %cl, %edx                /* adjust v mask and shift value */       ; \
   negl %ecx                                                               ; \
   movl %eax, UMASK                                                        ; \
   addl $16, %ecx                                                          ; \
   movl %edx, VMASK                                                        ; \
   movl %ecx, VSHIFT                                                       ; \
									   ; \
   INIT()                                                                  ; \
									   ; \
   movl ADDR, %edi                                                         ; \
									   ; \
   .align 4, 0x90                                                          ; \
name##_odd_byte_loop:                                                      ; \
   testl $3, %edi                /* deal with any odd pixels */            ; \
   jz name##_no_odd_bytes                                                  ; \
									   ; \
   END_FP_DIV(%ebx, %edx)        /* get corrected u/v position */          ; \
   UPDATE_FP_POS(1)              /* update u/v/z values */                 ; \
   START_FP_DIV()                /* start up the next divide */            ; \
									   ; \
   movb VSHIFT, %cl              /* shift and mask v coordinate */         ; \
   sarl %cl, %edx                                                          ; \
   andl VMASK, %edx                                                        ; \
									   ; \
   sarl $16, %ebx                /* shift and mask u coordinate */         ; \
   andl UMASK, %ebx                                                        ; \
   addl %ebx, %edx                                                         ; \
									   ; \
   SINGLE_PIXEL(1)               /* process the pixel */                   ; \
									   ; \
   decl W                                                                  ; \
   jg name##_odd_byte_loop                                                 ; \
									   ; \
   .align 4, 0x90                                                          ; \
name##_no_odd_bytes:                                                       ; \
   movl W, %ecx                  /* inner loop expanded 4 times */         ; \
   movl %ecx, COUNT                                                        ; \
   shrl $2, %ecx                                                           ; \
   jg name##_prime_x4                                                      ; \
									   ; \
   END_FP_DIV(%ebx, %edx)        /* get corrected u/v position */          ; \
   UPDATE_FP_POS(1)              /* update u/v/z values */                 ; \
   START_FP_DIV()                /* start up the next divide */            ; \
									   ; \
   movl %ebx, PREVU              /* store initial u/v pos */               ; \
   movl %edx, PREVV                                                        ; \
									   ; \
   jmp name##_cleanup_leftovers                                            ; \
									   ; \
   .align 4, 0x90                                                          ; \
name##_prime_x4:                                                           ; \
   movl %ecx, W                                                            ; \
									   ; \
   END_FP_DIV(%ebx, %edx)        /* get corrected u/v position */          ; \
   UPDATE_FP_POS(4)              /* update u/v/z values */                 ; \
   START_FP_DIV()                /* start up the next divide */            ; \
									   ; \
   movl %ebx, PREVU              /* store initial u/v pos */               ; \
   movl %edx, PREVV                                                        ; \
									   ; \
   jmp name##_start_loop                                                   ; \
									   ; \
   .align 4, 0x90                                                          ; \
name##_last_time:                                                          ; \
   END_FP_DIV(%eax, %ecx)        /* get corrected u/v position */          ; \
   UPDATE_FP_POS(1)              /* update u/v/z values */                 ; \
   jmp name##_loop_contents                                                ; \
									   ; \
   .align 4, 0x90                                                          ; \
name##_loop:                                                               ; \
   END_FP_DIV(%eax, %ecx)        /* get corrected u/v position */          ; \
   UPDATE_FP_POS(4)              /* update u/v/z values */                 ; \
									   ; \
name##_loop_contents:                                                      ; \
   START_FP_DIV()                /* start up the next divide */            ; \
									   ; \
   movl PREVU, %ebx              /* read the previous u/v pos */           ; \
   movl PREVV, %edx                                                        ; \
									   ; \
   movl %eax, PREVU              /* store u/v for the next cycle */        ; \
   movl %ecx, PREVV                                                        ; \
									   ; \
   subl %ebx, %eax               /* get u/v diffs for the four pixels */   ; \
   subl %edx, %ecx                                                         ; \
   sarl $2, %eax                                                           ; \
   sarl $2, %ecx                                                           ; \
   movl %eax, UDIFF                                                        ; \
   movl %ecx, VDIFF                                                        ; \
									   ; \
   PREPARE_FOUR_PIXELS()                                                   ; \
									   ; \
   /* pixel 1 */                                                           ; \
   movl %edx, %eax               /* shift and mask v coordinate */         ; \
   movb VSHIFT, %cl                                                        ; \
   sarl %cl, %eax                                                          ; \
   andl VMASK, %eax                                                        ; \
									   ; \
   movl %ebx, %ecx               /* shift and mask u coordinate */         ; \
   sarl $16, %ecx                                                          ; \
   andl UMASK, %ecx                                                        ; \
   addl %ecx, %eax                                                         ; \
									   ; \
   FOUR_PIXELS(0)                /* process pixel 1 */                     ; \
									   ; \
   addl UDIFF, %ebx                                                        ; \
   addl VDIFF, %edx                                                        ; \
									   ; \
   /* pixel 2 */                                                           ; \
   movl %edx, %eax               /* shift and mask v coordinate */         ; \
   movb VSHIFT, %cl                                                        ; \
   sarl %cl, %eax                                                          ; \
   andl VMASK, %eax                                                        ; \
									   ; \
   movl %ebx, %ecx               /* shift and mask u coordinate */         ; \
   sarl $16, %ecx                                                          ; \
   andl UMASK, %ecx                                                        ; \
   addl %ecx, %eax                                                         ; \
									   ; \
   FOUR_PIXELS(1)                /* process pixel 2 */                     ; \
									   ; \
   addl UDIFF, %ebx                                                        ; \
   addl VDIFF, %edx                                                        ; \
									   ; \
   /* pixel 3 */                                                           ; \
   movl %edx, %eax               /* shift and mask v coordinate */         ; \
   movb VSHIFT, %cl                                                        ; \
   sarl %cl, %eax                                                          ; \
   andl VMASK, %eax                                                        ; \
									   ; \
   movl %ebx, %ecx               /* shift and mask u coordinate */         ; \
   sarl $16, %ecx                                                          ; \
   andl UMASK, %ecx                                                        ; \
   addl %ecx, %eax                                                         ; \
									   ; \
   FOUR_PIXELS(2)                /* process pixel 3 */                     ; \
									   ; \
   addl UDIFF, %ebx                                                        ; \
   addl VDIFF, %edx                                                        ; \
									   ; \
   /* pixel 4 */                                                           ; \
   movl %edx, %eax               /* shift and mask v coordinate */         ; \
   movb VSHIFT, %cl                                                        ; \
   sarl %cl, %eax                                                          ; \
   andl VMASK, %eax                                                        ; \
									   ; \
   movl %ebx, %ecx               /* shift and mask u coordinate */         ; \
   sarl $16, %ecx                                                          ; \
   andl UMASK, %ecx                                                        ; \
   addl %ecx, %eax                                                         ; \
									   ; \
   FOUR_PIXELS(3)                /* process pixel 4 */                     ; \
									   ; \
   WRITE_FOUR_PIXELS()           /* write four pixels at a time */         ; \
									   ; \
name##_start_loop:                                                         ; \
   decl W                                                                  ; \
   jg name##_loop                                                          ; \
   jz name##_last_time                                                     ; \
									   ; \
name##_cleanup_leftovers:                                                  ; \
   movl COUNT, %ecx              /* deal with any leftover pixels */       ; \
   andl $3, %ecx                                                           ; \
   jz name##_done                                                          ; \
									   ; \
   movl %ecx, COUNT                                                        ; \
   movl PREVU, %ebx                                                        ; \
   movl PREVV, %edx                                                        ; \
									   ; \
   .align 4, 0x90                                                          ; \
name##_cleanup_loop:                                                       ; \
   movb VSHIFT, %cl              /* shift and mask v coordinate */         ; \
   sarl %cl, %edx                                                          ; \
   andl VMASK, %edx                                                        ; \
									   ; \
   sarl $16, %ebx                /* shift and mask u coordinate */         ; \
   andl UMASK, %ebx                                                        ; \
   addl %ebx, %edx                                                         ; \
									   ; \
   SINGLE_PIXEL(2)               /* process the pixel */                   ; \
									   ; \
   decl COUNT                                                              ; \
   jz name##_done                                                          ; \
									   ; \
   END_FP_DIV(%ebx, %edx)        /* get corrected u/v position */          ; \
   UPDATE_FP_POS(1)              /* update u/v/z values */                 ; \
   START_FP_DIV()                /* start up the next divide */            ; \
   jmp name##_cleanup_loop                                                 ; \
									   ; \
name##_done:                                                               ; \
   fstp %st(0)                   /* pop fpu stack */                       ; \
   fstp %st(0)                                                             ; \
   fstp %st(0)                                                             ; \
   fstp %st(0)                                                             ; \
									   ; \
   popl %edi                                                               ; \
   popl %esi                                                               ; \
   popl %ebx                                                               ; \
   movl %ebp, %esp                                                         ; \
   popl %ebp




/* void _poly_scanline_ptex(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a perspective correct texture mapped polygon scanline.
 */
.globl __poly_scanline_ptex
   .align 4
__poly_scanline_ptex:

   #define INIT()                                                            \
      movl POLYSEG_TEXTURE(%esi), %esi


   #define SINGLE_PIXEL(n)                                                   \
      movb (%esi, %edx), %al     /* read texel */                          ; \
      movb %al, %fs:(%edi)       /* write the pixel */                     ; \
      incl %edi


   #define PREPARE_FOUR_PIXELS()                                             \
      /* noop */


   #define FOUR_PIXELS(n)                                                    \
      movb (%esi, %eax), %al     /* read texel */                          ; \
      movb %al, n+TMP            /* store the pixel */


   #define WRITE_FOUR_PIXELS()                                               \
      movl TMP, %eax             /* write four pixels at a time */         ; \
      movl %eax, %fs:(%edi)                                                ; \
      addl $4, %edi


   DO_PTEX(ptex)


   #undef INIT
   #undef SINGLE_PIXEL
   #undef PREPARE_FOUR_PIXELS
   #undef FOUR_PIXELS
   #undef WRITE_FOUR_PIXELS

   ret                           /* end of _poly_scanline_ptex() */




/* void _poly_scanline_ptex_mask(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a masked perspective correct texture mapped polygon scanline.
 */
.globl __poly_scanline_ptex_mask
   .align 4
__poly_scanline_ptex_mask:

   #define INIT()                                                            \
      movl POLYSEG_TEXTURE(%esi), %esi


   #define SINGLE_PIXEL(n)                                                   \
      movb (%esi, %edx), %al     /* read texel */                          ; \
      orb %al, %al                                                         ; \
      jz ptex_mask_skip_left_##n                                           ; \
      movb %al, %fs:(%edi)       /* write the pixel */                     ; \
   ptex_mask_skip_left_##n:                                                ; \
      incl %edi


   #define PREPARE_FOUR_PIXELS()                                             \
      /* noop */


   #define FOUR_PIXELS(n)                                                    \
      movb (%esi, %eax), %al     /* read texel */                          ; \
      orb %al, %al                                                         ; \
      jz ptex_mask_skip_##n                                                ; \
      movb %al, %fs:(%edi)       /* write the pixel */                     ; \
   ptex_mask_skip_##n:                                                     ; \
      incl %edi


   #define WRITE_FOUR_PIXELS()                                               \
      /* noop */


   DO_PTEX(ptex_mask)


   #undef INIT
   #undef SINGLE_PIXEL
   #undef PREPARE_FOUR_PIXELS
   #undef FOUR_PIXELS
   #undef WRITE_FOUR_PIXELS

   ret                           /* end of _poly_scanline_ptex_mask() */




/* void _poly_scanline_ptex_lit(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a lit perspective correct texture mapped polygon scanline.
 */
.globl __poly_scanline_ptex_lit
   .align 4
__poly_scanline_ptex_lit:

   #define INIT()                                                            \
      movl POLYSEG_C(%esi), %eax                                           ; \
      movl POLYSEG_DC(%esi), %ebx                                          ; \
      movl POLYSEG_TEXTURE(%esi), %ecx                                     ; \
      sarl $8, %eax                                                        ; \
      sarl $8, %ebx                                                        ; \
      movl %eax, C                                                         ; \
      movl %ebx, DC                                                        ; \
      movl %ecx, TEXTURE


   #define SINGLE_PIXEL(n)                                                   \
      xorl %eax, %eax                                                      ; \
      movl TEXTURE, %esi         /* read texel */                          ; \
      movb (%esi, %edx), %al                                               ; \
									   ; \
      movb 1+C, %ah              /* lookup in lighting table */            ; \
      movl _color_map, %esi                                                ; \
      movl DC, %ecx                                                        ; \
      movb (%eax, %esi), %al                                               ; \
      addl %ecx, C                                                         ; \
									   ; \
      movb %al, %fs:(%edi)       /* write the pixel */                     ; \
      incl %edi


   #define PREPARE_FOUR_PIXELS()                                             \
      movl TEXTURE, %esi


   #define FOUR_PIXELS(n)                                                    \
      movl TEXTURE, %esi                                                   ; \
      movb (%esi, %eax), %al     /* read texel */                          ; \
      movb %al, n+TMP            /* store the pixel */


   #define WRITE_FOUR_PIXELS()                                               \
      movl _color_map, %esi      /* light four pixels */                   ; \
      xorl %ecx, %ecx                                                      ; \
      movl DC, %ebx                                                        ; \
      movl C, %edx                                                         ; \
									   ; \
      movb %dh, %ch              /* light pixel 1 */                       ; \
      movb TMP, %cl                                                        ; \
      addl %ebx, %edx                                                      ; \
      movb (%ecx, %esi), %al                                               ; \
									   ; \
      movb %dh, %ch              /* light pixel 2 */                       ; \
      movb 1+TMP, %cl                                                      ; \
      addl %ebx, %edx                                                      ; \
      movb (%ecx, %esi), %ah                                               ; \
									   ; \
      roll $16, %eax                                                       ; \
									   ; \
      movb %dh, %ch              /* light pixel 3 */                       ; \
      movb 2+TMP, %cl                                                      ; \
      addl %ebx, %edx                                                      ; \
      movb (%ecx, %esi), %al                                               ; \
									   ; \
      movb %dh, %ch              /* light pixel 4 */                       ; \
      movb 3+TMP, %cl                                                      ; \
      addl %ebx, %edx                                                      ; \
      movb (%ecx, %esi), %ah                                               ; \
									   ; \
      movl %edx, C                                                         ; \
      roll $16, %eax                                                       ; \
      movl %eax, %fs:(%edi)      /* write four pixels at a time */         ; \
      addl $4, %edi


   DO_PTEX(ptex_lit)


   #undef INIT
   #undef SINGLE_PIXEL
   #undef PREPARE_FOUR_PIXELSS
   #undef FOUR_PIXELS
   #undef WRITE_FOUR_PIXELSS

   ret                           /* end of _poly_scanline_ptex_lit() */


