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
 *      SVGA bank switching code. These routines will be called with
 *      a line number in %eax and a pointer to the bitmap in %edx. The
 *      bank switcher should select the appropriate bank for the line,
 *      and replace %eax with a pointer to the start of the line.
 *
 *      See readme.txt for copyright information.
 */


#ifndef DJGPP
#error This file should only be used by the djgpp version of Allegro
#endif

#include "asmdefs.inc"

.text



#define SETUP(name)
#define CLEANUP(name)


/* Template for bank switchers. Produces a framework that checks which 
 * bank is currently selected and only executes the provided code if it
 * needs to change bank. Restores %eax: if the switch routine touches any 
 * other registers it must save and restore them itself.
 */
#define BANK_SWITCHER(name, cache, code...)                                  \
.globl name                                                                ; \
   .align 4                                                                ; \
name:                                                                      ; \
   SETUP(name)                                                             ; \
									   ; \
   pushl %eax                                                              ; \
									   ; \
   addl BMP_LINEOFS(%edx), %eax              /* add line offset */         ; \
									   ; \
   shll $2, %eax                                                           ; \
   addl __gfx_bank, %eax                     /* lookup which bank */       ; \
   movl (%eax), %eax                                                       ; \
   cmpl cache, %eax                          /* need to change? */         ; \
   je name##_done                                                          ; \
									   ; \
   movl %eax, cache                          /* store the new bank */      ; \
   code                                      /* and change it */           ; \
									   ; \
   .align 4, 0x90                                                          ; \
name##_done:                                                               ; \
   popl %eax                                                               ; \
   movl BMP_LINE(%edx, %eax, 4), %eax        /* load line address */       ; \
									   ; \
   CLEANUP(name)                                                           ; \
   ret                                                                     ; \
									   ; \
.globl name##_end                                                          ; \
   .align 4                                                                ; \
name##_end:                                                                ; \
   ret 




/* Uses VESA function 05h (real mode interrupt) to select the bank in %eax.
 * Restores all registers except %eax, which is handled by BANK_SWITCHER.
 */
#define SET_VESA_BANK_RM(window)                                             \
   pushal                                    /* store registers */         ; \
   pushw %es                                                               ; \
									   ; \
   movw ___djgpp_ds_alias, %bx                                             ; \
   movw %bx, %es                                                           ; \
									   ; \
   movl $0x10, %ebx                          /* call int 0x10 */           ; \
   movl $0, %ecx                             /* no stack required */       ; \
   movl $__dpmi_reg, %edi                    /* register structure */      ; \
									   ; \
   movw $0x4F05, DPMI_AX(%edi)               /* VESA function 05h */       ; \
   movw $window, DPMI_BX(%edi)               /* which window? */           ; \
   movw %ax, DPMI_DX(%edi)                   /* which bank? */             ; \
   movw $0, DPMI_SP(%edi)                    /* zero stack */              ; \
   movw $0, DPMI_SS(%edi)                                                  ; \
   movw $0, DPMI_FLAGS(%edi)                 /* and zero flags */          ; \
									   ; \
   movl $0x0300, %eax                        /* simulate RM interrupt */   ; \
   int $0x31                                                               ; \
									   ; \
   popw %es                                  /* restore registers */       ; \
   popal




/* Uses the VESA 2.0 protected mode interface to select the bank in %eax.
 * Restores all registers except %eax, which is handled by BANK_SWITCHER.
 */
#define SET_VESA_BANK_PM(window)                                             \
   pushal                                    /* store registers */         ; \
									   ; \
   movw $window, %bx                         /* which window? */           ; \
   movw %ax, %dx                             /* which bank? */             ; \
   movl __pm_vesa_switcher, %eax                                           ; \
   call *%eax                                /* do it! */                  ; \
									   ; \
   popal                                     /* restore registers */




/* Uses the VESA 2.0 protected mode interface to select the bank in %eax,
 * passing a selector for memory mapped io in %es. Restores all registers 
 * except %eax, which is handled by BANK_SWITCHER.
 */
#define SET_VESA_BANK_PM_ES(window)                                          \
   pushal                                    /* store registers */         ; \
   pushw %es                                                               ; \
									   ; \
   movw $window, %bx                         /* which window? */           ; \
   movw %ax, %dx                             /* which bank? */             ; \
   movw __mmio_segment, %ax                  /* load selector into %es */  ; \
   movw %ax, %es                                                           ; \
   movl __pm_vesa_switcher, %eax                                           ; \
   call *%eax                                /* do it! */                  ; \
									   ; \
   popw %es                                  /* restore registers */       ; \
   popal




/* VESA window 1 switching routines */
BANK_SWITCHER( __vesa_window_1, __last_bank_1, SET_VESA_BANK_RM(0) )
BANK_SWITCHER( __vesa_pm_window_1, __last_bank_1, SET_VESA_BANK_PM(0) )
BANK_SWITCHER( __vesa_pm_es_window_1, __last_bank_1, SET_VESA_BANK_PM_ES(0) )



/* VESA window 2 switching routines */
#undef CLEANUP
#define CLEANUP(name) addl __window_2_offset, %eax
BANK_SWITCHER( __vesa_window_2, __last_bank_2, SET_VESA_BANK_RM(1) )
BANK_SWITCHER( __vesa_pm_window_2, __last_bank_2, SET_VESA_BANK_PM(1) )
BANK_SWITCHER( __vesa_pm_es_window_2, __last_bank_2, SET_VESA_BANK_PM_ES(1) )
#undef CLEANUP
#define CLEANUP(name)



.globl __af_wrapper
__af_wrapper:



/* callback for VBE/AF to access real mode interrupts (DPMI 0x300) */
.globl __af_int86
   .align 4
__af_int86:
   pushal
   pushw %es
   pushw %ds
   popw %es

   xorb %bh, %bh
   xorl %ecx, %ecx

   movl $0x300, %eax
   int $0x31 

   popw %es
   popal
   ret



/* callback for VBE/AF to access real mode functions (DPMI 0x301) */
.globl __af_call_rm
   .align 4
__af_call_rm:
   pushal
   pushw %es
   pushw %ds
   popw %es

   xorb %bh, %bh
   xorl %ecx, %ecx

   movl $0x301, %eax
   int $0x31 

   popw %es
   popal
   ret



.globl __af_wrapper_end
__af_wrapper_end:



/* helper for VBE/AF bank switches: calls the EnableDirectAccess and 
 * WaitTillIdle functions as required...
 */
#undef SETUP
#define SETUP(name)                                                          \
   cmpl $0, __af_active       /* is the accelerator active? */             ; \
   jz name##_inactive                                                      ; \
									   ; \
   pushal                     /* save registers */                         ; \
   movl __af_driver, %ebx     /* load /AF driver into ebx */               ; \
									   ; \
   movl __af_enable_direct_access, %ecx                                    ; \
   orl %ecx, %ecx                                                          ; \
   jz name##_no_direct_access                                              ; \
									   ; \
   call *%ecx                 /* call the EnableDirectAccess function */   ; \
   jmp name##_now_inactive                                                 ; \
									   ; \
   .align 4, 0x90                                                          ; \
name##_no_direct_access:                                                   ; \
   movl __af_wait_till_idle, %ecx                                          ; \
   orl %ecx, %ecx                                                          ; \
   jz name##_now_inactive                                                  ; \
									   ; \
   call *%ecx                 /* call the WaitTillIdle function */         ; \
									   ; \
name##_now_inactive:                                                       ; \
   popal                      /* restore registers */                      ; \
   movl $0, __af_active       /* flag accelerator is inactive */           ; \
									   ; \
   .align 4, 0x90                                                          ; \
name##_inactive:




/* VBE/AF bank selector */
BANK_SWITCHER( __vbeaf_bank, __last_bank_1,
   pushal
   movl %eax, %edx
   movl __af_driver, %ebx 
   movl __af_set_bank, %eax 
   call *%eax                 /* call the VBE/AF bank switch function */
   popal
)



/* stub bank switch for linear VBE/AF modes, to handle WaitTillIdle etc. */
.globl __vbeaf_linear_lookup
   .align 4
__vbeaf_linear_lookup:
   SETUP(__vbeaf_linear_lookup) 
   movl BMP_LINE(%edx, %eax, 4), %eax 
   ret 

.globl __vbeaf_linear_lookup_end
   .align 4 
__vbeaf_linear_lookup_end:
   ret 



#undef SETUP
#define SETUP(name)



/* ATI bank selector */
BANK_SWITCHER( __ati_bank, __last_bank_1,
   pushl %edx
   movb %al, %ah              /* save al into ah */

   movl __ati_port, %edx      /* read port 1CE index 0xB2 */
   movb $0xB2, %al
   outb %al, %dx 
   incl %edx
   inb %dx, %al
   decl %edx

   andb $0xE1, %al            /* mask out bits 1-4 */
   shlb $1, %ah               /* shift bank number */
   orb %al, %ah 

   movb $0xB2, %al            /* write to port 1CE index 0xB2 */
   outb %al, %dx
   incl %edx
   movb %ah, %al
   outb %al, %dx

   popl %edx
)



/* mach64 write bank selector */
BANK_SWITCHER( __mach64_write_bank, __last_bank_1,
   pushl %edx

   movb %al, %ah              /* two 32k apertures, set bank and bank+1 */
   incb %ah
   shll $8, %eax
   shrw $8, %ax

   movl __mach64_wp_sel, %edx
   outl %eax, %dx             /* write it */

   popl %edx
)



/* mach64 read bank selector */
BANK_SWITCHER( __mach64_read_bank, __last_bank_2,
   pushl %edx

   movb %al, %ah              /* two 32k apertures, set bank and bank+1 */
   incb %ah
   shll $8, %eax
   shrw $8, %ax

   movl __mach64_rp_sel, %edx
   outl %eax, %dx             /* write it */

   popl %edx
)



/* Cirrus 64xx write bank selector */
BANK_SWITCHER( __cirrus64_write_bank, __last_bank_1,
   pushl %edx
   pushl %eax

   movl $0x3CE, %edx          /* 3CE index 0xF */
   movb $0xF, %al
   outb %al, %dx

   popl %eax                  /* write bank number */
   incl %edx
   outb %al, %dx

   popl %edx
)



/* Cirrus 64xx read bank selector */
BANK_SWITCHER( __cirrus64_read_bank, __last_bank_2, 
   pushl %edx
   pushl %eax

   movl $0x3CE, %edx          /* 3CE index 0xE */
   movb $0xE, %al
   outb %al, %dx

   popl %eax                  /* write bank number */
   incl %edx
   outb %al, %dx

   popl %edx
)



/* Cirrus 54xx bank selector */
BANK_SWITCHER( __cirrus54_bank, __last_bank_1,
   pushl %edx
   pushl %eax

   movl $0x3CE, %edx          /* 3CE index 0x9 */
   movb $0x9, %al
   outb %al, %dx

   popl %eax                  /* write bank number */
   incl %edx
   outb %al, %dx

   popl %edx
)



/* Paradise 24+ write bank selector */
BANK_SWITCHER( __paradise_write_bank, __last_bank_1,
   pushl %edx
   pushl %eax

   movl $0x3CE, %edx          /* 3CE index 0xA */
   movb $0xA, %al
   outb %al, %dx

   popl %eax                  /* write bank number */
   incl %edx
   outb %al, %dx

   popl %edx
)



/* Paradise 24+ read bank selector */
BANK_SWITCHER( __paradise_read_bank, __last_bank_2, 
   pushl %edx
   pushl %eax

   movl $0x3CE, %edx          /* 3CE index 0x9 */
   movb $0x9, %al
   outb %al, %dx

   popl %eax                  /* write bank number */
   incl %edx
   outb %al, %dx

   popl %edx
)



/* S3 bank selector */
BANK_SWITCHER( __s3_bank, __last_bank_1,
   pushl %edx
   pushl %eax
   movl __crtc, %edx

   movb $0x38, %al            /* enable extensions */
   outb %al, %dx
   incl %edx
   movb $0x48, %al
   outb %al, %dx
   decl %edx

   movb $0x31, %al            /* read register 0x31 */
   outb %al, %dx 
   incl %edx
   inb %dx, %al
   decl %edx

   movb %al, %ah              /* set bank write and vid mem > 256K flags */
   movb $0x31, %al
   outb %al, %dx
   movb %ah, %al
   orb $9, %al 
   incl %edx
   outb %al, %dx
   decl %edx

   movb $0x35, %al            /* write the bank number */
   outb %al, %dx
   incl %edx
   popl %eax
   outb %al, %dx 
   decl %edx

   movb $0x38, %al            /* disable extensions */
   outb %al, %dx
   incl %edx
   xorb %al, %al
   outb %al, %dx
   decl %edx

   popl %edx
)



/* Trident bank selector (for old cards) */
BANK_SWITCHER( __trident_bank, __last_bank_1,
   pushl %edx
   movb %al, %ah              /* save al into ah */

   movl $0x3C4, %edx          /* read port 3C4 register 0xE */
   movb $0xE, %al
   outb %al, %dx 
   incl %edx
   inb %dx, %al
   decl %edx

   andb $0xF0, %al            /* mask low four bits */
   xorb $2, %ah               /* xor bank number with 2 */
   orb %al, %ah

   movb $0xE, %al             /* write to port 3C4 register 0xE */
   outb %al, %dx
   incl %edx
   movb %ah, %al
   outb %al, %dx

   popl %edx
)



/* Trident write bank selector (for newer cards) */
BANK_SWITCHER( __trident_write_bank, __last_bank_1,
   pushl %edx

   movl $0x3D8, %edx
   outb %al, %dx

   popl %edx
)



/* Trident read bank selector (for newer cards) */
BANK_SWITCHER( __trident_read_bank, __last_bank_2,
   pushl %edx

   movl $0x3D9, %edx
   outb %al, %dx

   popl %edx
)



/* Tseng ET3000 write bank selector */
BANK_SWITCHER( __et3000_write_bank, __last_bank_1,
   pushl %edx

   movb __last_bank_2, %dl
   shlb $3, %dl
   orb %dl, %al               /* mask with read bank */
   orb $0x40, %al             /* select 64k segments */

   movl $0x3CD, %edx
   outb %al, %dx              /* write to the card */

   popl %edx
)



/* Tseng ET3000 read bank selector */
BANK_SWITCHER( __et3000_read_bank, __last_bank_2, 
   pushl %edx

   shlb $3, %al
   orb __last_bank_1, %al     /* mask with write bank */
   orb $0x40, %al             /* select 64k segments */

   movl $0x3CD, %edx
   outb %al, %dx              /* write to the card */

   popl %edx
)



/* Tseng ET4000 write bank selector */
BANK_SWITCHER( __et4000_write_bank, __last_bank_1,
   pushl %edx

   movb __last_bank_2, %dl
   shlb $4, %dl
   orb %dl, %al               /* mask with read bank */

   movl $0x3CD, %edx
   outb %al, %dx              /* write to the card */

   popl %edx
)



/* Tseng ET4000 read bank selector */
BANK_SWITCHER( __et4000_read_bank, __last_bank_2, 
   pushl %edx

   shlb $4, %al
   orb __last_bank_1, %al     /* mask with write bank */

   movl $0x3CD, %edx
   outb %al, %dx              /* write to the card */

   popl %edx
)



/* Video7 bank selector */
BANK_SWITCHER( __video7_bank, __last_bank_1,
   pushl %edx

   movb %al, %ah
   shlb $4, %ah               /* bank number */
   movb $0xE8, %al
   movl $0x3C4, %edx
   outw %ax, %dx              /* 3C4 index 0xE8 */

   popl %edx
)

