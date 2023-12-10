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
 *      CPU detection routines, by Phil Frisbie.
 *
 *      Theuzifan improved the support for Cyrix chips.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "internal.h"



char cpu_vendor[16] = "NoVendorName";
int cpu_family = 0;
int cpu_model = 0;
int cpu_fpu = FALSE; 
int cpu_mmx = FALSE; 
int cpu_cpuid = FALSE; 



/* is_486:
 *  Returns TRUE for 486+, and FALSE for 386.
 */
static int is_486() 
{
   int result;

   asm (
      "  pushf ; "                     /* save EFLAGS */
      "  popl %%eax ; "                /* get EFLAGS */
      "  movl %%eax, %%ecx ; "         /* temp storage EFLAGS */
      "  xorl $0x40000, %%eax ; "      /* change AC bit in EFLAGS */
      "  pushl %%eax ; "               /* put new EFLAGS value on stack */
      "  popf ; "                      /* replace current EFLAGS value */
      "  pushf ; "                     /* get EFLAGS */
      "  popl %%eax ; "                /* save new EFLAGS in EAX */
      "  cmpl %%ecx, %%eax ; "         /* compare temp and new EFLAGS */
      "  jz 0f ;"
      "  movl $1, %%eax ; "            /* 80486 present */
      "  jmp 1f ; "
      " 0: "
      "  movl $0, %%eax ; "            /* 80486 not present */
      " 1: "
      "  pushl %%ecx ; "               /* get original EFLAGS */
      "  popf ; "                      /* restore EFLAGS */

   : "=a" (result)
   :
   : "eax", "ecx", "memory"
   );

   return result;
}



/* is_386DX:
 *  Returns TRUE for 386DX, and FALSE for 386SX.
 */
static int is_386DX()
{
   int result;

   asm (
      "  movl %%cr0, %%edx ; "         /* get CR0 */
      "  pushl %%edx ; "               /* save CR0 */
      "  andb $0xef, %%dl ; "          /* clear bit4 */
      "  movl %%edx, %%cr0 ; "         /* set CR0 */
      "  movl %%cr0, %%edx ; "         /* and read CR0 */
      "  andb $0x10, %%dl ; "          /* bit4 forced high? */
      "  popl %%edx ; "                /* restore reg w/ CR0 */
      "  movl %%edx, %%cr0 ; "         /* restore CR0 */
      "  movl $1, %%eax ; "            /* TRUE, 386DX */
      "  jz 0f ; "
      "  movl $0, %%eax ; "            /* FALSE, 386SX */
      " 0: "

   : "=a" (result)
   :
   : "%edx", "memory"
   );

   return result;
}



/* is_fpu:
 *  Returns TRUE is the CPU has floating point hardware.
 */
static int is_fpu()
{
   int result;

   asm (
      "  fninit ; "
      "  movl $0x5a5a, %%eax ; "
      "  fnstsw %%eax ; "
      "  cmpl $0, %%eax ; "
      "  jne 0f ; "
      "  movl $1, %%eax ; "
      "  jmp 1f ; "
      " 0: "
      "  movl $0, %%eax ; "
      " 1:"

   : "=a" (result)
   :
   : "%eax", "memory"
   );

   return result;
}



/* is_cyrix:
 *  Returns TRUE if this is a Cyrix processor.
 */
static int is_cyrix()
{
   int result;

   asm (
      "  xorw %%ax, %%ax ; "           /* clear eax */
      "  sahf ; "                      /* bit 1 is always 1 in flags */
      "  movw $5, %%ax ; "
      "  movw $2, %%bx ; "
      "  div %%bl ; "                  /* this does not change flags */
      "  lahf ; "                      /* get flags */
      "  cmpb $2, %%ah ; "             /* check for change in flags */
      "  jne 0f ; "                    /* flags changed not Cyrix */
      "  movl $1, %%eax ; "            /* TRUE Cyrix CPU */
      "  jmp 1f ; "
      " 0: "
      "  movl $0, %%eax ; "            /* FALSE NON-Cyrix CPU */
      " 1: "

   : "=a" (result)
   :
   : "%eax", "%ebx", "memory" );

   return result;
}



/* cx_w:
 *  Writes to a Cyrix register.
 */
static void cx_w(int index, int value)
{
   enter_critical();

   outportb(0x22, index);              /* tell CPU which config register */
   outportb(0x23, value);              /* write to CPU config register */

   exit_critical();
}



/* cx_r:
 *  Reads from a Cyrix register.
 */
static char cx_r(int index)
{
   char value;

   enter_critical();

   outportb(0x22, index);              /* tell CPU which config register */
   value = inportb(0x23);              /* read CPU config register */

   exit_critical();

   return value;
}



/* cyrix_type:
 *  Detects which type of Cyrix CPU is in use.
 */
static void cyrix_type()
{
   char orgc2, newc2, orgc3, newc3;
   int cr2_rw = FALSE, cr3_rw = FALSE, type;

   type = 0xFF;

   orgc2 = cx_r(0xC2);                 /* get current c2 value */
   newc2 = orgc2 ^ 4;                  /* toggle test bit */
   cx_w(0xC2, newc2);                  /* write test value to c2 */
   cx_r(0xC0);                         /* dummy read to change bus */
   if (cx_r(0xC2) != orgc2)            /* did test bit toggle */
      cr2_rw = TRUE;                   /* yes bit changed */
   cx_w(0xC2, orgc2);                  /* return c2 to original value */

   orgc3 = cx_r(0xC3);                 /* get current c3 value */
   newc3 = orgc3 ^ 0x80;               /* toggle test bit */
   cx_w(0xC3, newc3);                  /* write test value to c3 */
   cx_r(0xC0);                         /* dummy read to change bus */
   if (cx_r(0xC3) != orgc3)            /* did test bit change */
      cr3_rw = TRUE;                   /* yes it did */
   cx_w(0xC3, orgc3);                  /* return c3 to original value */

   if (((cr2_rw) && (cr3_rw)) || ((!cr2_rw) && (cr3_rw))) {
      type = cx_r(0xFE);               /* DEV ID register ok */
   }
   else if ((cr2_rw) && (!cr3_rw)) {
      type = 0xFE;                     /* Cx486S A step */
   }
   else if ((!cr2_rw) && (!cr3_rw)) {
      type = 0xFD;                     /* Pre ID Regs. Cx486SLC or DLC */
   }

   if ((type < 0x30) || (type > 0xFC)) {
      cpu_family = 4;                  /* 486 class-including 5x86 */
      cpu_model = 14;                  /* Cyrix */
   }
   else if (type < 0x50) {
      cpu_family = 5;                  /* Pentium class-6x86 and Media GX */
      cpu_model = 14;                  /* Cyrix */
   }
   else {
      cpu_family = 6;                  /* Pentium || class- 6x86MX */
      cpu_model = 14;                  /* Cyrix */
      cpu_mmx = TRUE;
   }
}



/* is_cpuid_supported:
 *  Checks whether the cpuid instruction is available.
 */
static int is_cpuid_supported()
{
   int result;

   asm (
      "  pushfl ; "                    /* get extended flags */
      "  popl %%eax ; "
      "  movl %%eax, %%ebx ; "         /* save current flags */
      "  xorl $0x200000, %%eax ; "     /* toggle bit 21 */
      "  pushl %%eax ; "               /* put new flags on stack */
      "  popfl ; "                     /* flags updated now in flags */
      "  pushfl ; "                    /* get extended flags */
      "  popl %%eax ; "
      "  xorl %%ebx, %%eax ; "         /* if bit 21 r/w then supports cpuid */
      "  jz 0f ; "
      "  movl $1, %%eax ; "
      "  jmp 1f ; "
      " 0: "
      "  movl $0, %%eax ; "
      " 1: "

   : "=a" (result)
   :
   : "%eax", "%ebx", "memory"
   );

   return result;
}



/* get_cpuid_info:
 *  This is so easy!
 */
static void get_cpuid_info(long cpuid_levels, long *eax, long *ebx, long *ecx, long *edx)
{ 
   long a, b, c, d;

   asm (
      " .byte 0x0F, 0xA2 ; "

   : "=a" (a), "=b" (b), "=c" (c), "=d" (d)
   : "0" (cpuid_levels)
   : "memory"
   );

   *eax = a;
   *ebx = b;
   *ecx = c;
   *edx = d;
}



/* check_cpu:
 *  This is the function to call to set the globals
 */
void check_cpu() 
{
   long cpuid_levels;
   long vendor_temp[3];
   long reg_eax, reg_ebx, reg_ecx, reg_edx;

   if (is_cpuid_supported()) {
      cpu_cpuid = TRUE;
      get_cpuid_info(0, &reg_eax, &reg_ebx, &reg_ecx, &reg_edx);
      cpuid_levels = reg_eax;
      vendor_temp[0] = reg_ebx;
      vendor_temp[1] = reg_edx;
      vendor_temp[2] = reg_ecx;
      memcpy(cpu_vendor, vendor_temp, 12);

      if (cpuid_levels > 0) {
	 reg_eax = reg_ebx = reg_ecx = reg_edx = 0;
	 get_cpuid_info(1, &reg_eax, &reg_ebx, &reg_ecx, &reg_edx);
	 cpu_family = (reg_eax & 0xF00) >> 8;
	 cpu_model = (reg_eax & 0xF0) >> 4;
	 cpu_fpu = (reg_edx & 1 ? TRUE : FALSE);
	 cpu_mmx = (reg_edx & 0x800000 ? TRUE: FALSE);
      }

      if (is_cyrix())
	 cpu_model=14;
   }
   else {
      cpu_fpu = is_fpu();
      if (!is_486()) {
	 if (is_386DX()) {
	    cpu_family = 3;
	    cpu_model = 0;
	 }
	 else {
	    cpu_family = 3;
	    cpu_model = 1;
	 }
      }
      else {
	 if (is_cyrix()) {
	    strcpy(cpu_vendor, "CyrixInstead");
	    cyrix_type();
	 }
	 else {
	    cpu_family = 4;
	    cpu_model = 15;
	 }
      }
   }
}

