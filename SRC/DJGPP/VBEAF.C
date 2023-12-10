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
 *      Video driver using the VBE/AF hardware accelerator API.
 *
 *      See readme.txt for copyright information.
 */


#ifndef DJGPP
#error This file should only be used by the djgpp version of Allegro
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dpmi.h>
#include <go32.h>
#include <sys/movedata.h>
#include <sys/farptr.h>
#include <sys/nearptr.h>
#include <sys/segments.h>

#include "allegro.h"
#include "internal.h"


static char vbeaf_desc[128] = "";

static BITMAP *vbeaf_init(int w, int h, int v_w, int v_h, int color_depth);
static void vbeaf_exit(BITMAP *b);
static void vbeaf_vsync();
static int vbeaf_scroll(int x, int y);
static void vbeaf_set_pallete_range(PALLETE p, int from, int to, int vsync);



GFX_DRIVER gfx_vbeaf = 
{
   "VBE/AF",
   vbeaf_desc,
   vbeaf_init,
   vbeaf_exit,
   vbeaf_scroll,
   vbeaf_vsync,
   vbeaf_set_pallete_range,
   0, 0, FALSE, 0, 0, 0, 0
};



typedef struct AF_DRIVER         /* VBE/AF driver structure */
{
   /* driver header */
   char           Signature[12]           __attribute__ ((packed));
   unsigned long  Version                 __attribute__ ((packed));
   unsigned long  DriverRev               __attribute__ ((packed));
   char           OemVendorName[80]       __attribute__ ((packed));
   char           OemCopyright[80]        __attribute__ ((packed));
   short          *AvailableModes         __attribute__ ((packed));
   unsigned long  TotalMemory             __attribute__ ((packed));
   unsigned long  Attributes              __attribute__ ((packed));
   unsigned long  BankSize                __attribute__ ((packed));
   unsigned long  BankedBasePtr           __attribute__ ((packed));
   unsigned long  LinearSize              __attribute__ ((packed));
   unsigned long  LinearBasePtr           __attribute__ ((packed));
   unsigned long  LinearGranularity       __attribute__ ((packed));
   unsigned short *IOPortsTable           __attribute__ ((packed));
   unsigned long  IOMemoryBase[4]         __attribute__ ((packed));
   unsigned long  IOMemoryLen[4]          __attribute__ ((packed));
   unsigned long  res1[10]                __attribute__ ((packed));

   /* near pointers mapped by application */
   void           *IOMemMaps[4]           __attribute__ ((packed));
   void           *BankedMem              __attribute__ ((packed));
   void           *LinearMem              __attribute__ ((packed));

   /* selectors allocated by application */
   unsigned long  Sel0000h                __attribute__ ((packed));
   unsigned long  Sel0040h                __attribute__ ((packed));
   unsigned long  SelA000h                __attribute__ ((packed));
   unsigned long  SelB000h                __attribute__ ((packed));
   unsigned long  SelC000h                __attribute__ ((packed));

   /* driver state variables */
   unsigned long  BufferEndX              __attribute__ ((packed));
   unsigned long  BufferEndY              __attribute__ ((packed));
   unsigned long  OriginOffset            __attribute__ ((packed));
   unsigned long  OffscreenOffset         __attribute__ ((packed));
   unsigned long  OffscreenStartY         __attribute__ ((packed));
   unsigned long  OffscreenEndY           __attribute__ ((packed));
   unsigned long  res2[10]                __attribute__ ((packed));

   /* relocatable 32 bit bank switch routine, for Windows (ugh!) */
   unsigned long  SetBank32Len            __attribute__ ((packed));
   void           *SetBank32              __attribute__ ((packed));

   /* callback functions provided by application */
   void           *Int86                  __attribute__ ((packed));
   void           *CallRealMode           __attribute__ ((packed));

   /* device driver functions */
   void           *InitDriver             __attribute__ ((packed));
   void           *GetVideoModeInfo       __attribute__ ((packed));
   void           *SetVideoMode           __attribute__ ((packed));
   void           *RestoreTextMode        __attribute__ ((packed));
   void           *SetBank                __attribute__ ((packed));
   void           *SetDisplayStart        __attribute__ ((packed));
   void           *SetActiveBuffer        __attribute__ ((packed));
   void           *SetVisibleBuffer       __attribute__ ((packed));
   void           *SetPaletteData         __attribute__ ((packed));
   void           *SetGammaCorrectData    __attribute__ ((packed));
   void           *WaitTillIdle           __attribute__ ((packed));
   void           *EnableDirectAccess     __attribute__ ((packed));
   void           *DisableDirectAccess    __attribute__ ((packed));
   void           *SetCursor              __attribute__ ((packed));
   void           *SetCursorPos           __attribute__ ((packed));
   void           *SetCursorColor         __attribute__ ((packed));
   void           *ShowCursor             __attribute__ ((packed));
   void           *SetMix                 __attribute__ ((packed));
   void           *Set8x8MonoPattern      __attribute__ ((packed));
   void           *Set8x8ColorPattern     __attribute__ ((packed));
   void           *SetLineStipple         __attribute__ ((packed));
   void           *SetClipRect            __attribute__ ((packed));
   void           *DrawScan               __attribute__ ((packed));
   void           *DrawPattScan           __attribute__ ((packed));
   void           *DrawColorPattScan      __attribute__ ((packed));
   void           *DrawScanList           __attribute__ ((packed));
   void           *DrawRect               __attribute__ ((packed));
   void           *DrawPattRect           __attribute__ ((packed));
   void           *DrawColorPattRect      __attribute__ ((packed));
   void           *DrawLine               __attribute__ ((packed));
   void           *DrawStippleLine        __attribute__ ((packed));
   void           *DrawTrap               __attribute__ ((packed));
   void           *DrawTri                __attribute__ ((packed));
   void           *DrawQuad               __attribute__ ((packed));
   void           *PutMonoImage           __attribute__ ((packed));
   void           *BitBlt                 __attribute__ ((packed));
   void           *BitBltLin              __attribute__ ((packed));
   void           *SrcTransBlt            __attribute__ ((packed));
   void           *SrcTransBltLin         __attribute__ ((packed));
   void           *DstTransBlt            __attribute__ ((packed));
   void           *DstTransBltLin         __attribute__ ((packed));
} AF_DRIVER;



typedef struct AF_MODE_INFO      /* mode information structure */
{
   unsigned short Attributes              __attribute__ ((packed));
   unsigned short XResolution             __attribute__ ((packed));
   unsigned short YResolution             __attribute__ ((packed));
   unsigned short BytesPerScanLine        __attribute__ ((packed));
   unsigned short BitsPerPixel            __attribute__ ((packed));
   unsigned short MaxBuffers              __attribute__ ((packed));
   unsigned char  RedMaskSize             __attribute__ ((packed));
   unsigned char  RedFieldPosition        __attribute__ ((packed));
   unsigned char  GreenMaskSize           __attribute__ ((packed));
   unsigned char  GreenFieldPosition      __attribute__ ((packed));
   unsigned char  BlueMaskSize            __attribute__ ((packed));
   unsigned char  BlueFieldPosition       __attribute__ ((packed));
   unsigned char  RsvdMaskSize            __attribute__ ((packed));
   unsigned char  RsvdFieldPosition       __attribute__ ((packed));
   unsigned short MaxBytesPerScanLine     __attribute__ ((packed));
   unsigned short MaxScanLineWidth        __attribute__ ((packed));
   unsigned char  reserved[118]           __attribute__ ((packed));
} AF_MODE_INFO;



static AF_DRIVER *af_driver = NULL;    /* the VBE/AF driver */

static int in_af_mode = FALSE;         /* true if VBE/AF is in use */

static int vbeaf_xscroll = 0;          /* current display start address */
static int vbeaf_yscroll = 0;

static unsigned long af_memmap[4] = { 0, 0, 0, 0 };
static unsigned long af_banked_mem = 0;
static unsigned long af_linear_mem = 0;
static int af_sel0000h = 0;
static int af_sel0040h = 0;
static int af_sela000h = 0;
static int af_selb000h = 0;
static int af_selc000h = 0;

extern void _af_int86(), _af_call_rm(), _af_wrapper(), _af_wrapper_end();



/* call_vbeaf:
 *  Calls a VBE/AF function, passing the driver structure in ebx and 
 *  returning the output value from eax.
 */
static inline int call_vbeaf(void *proc)
{
   int ret;

   asm (
      "  call *%2 "

   : "=&a" (ret)                       /* return value in eax */

   : "b" (af_driver),                  /* VBE/AF driver in ds:ebx */
     "rm" (proc)                       /* function ptr in reg or mem */

   : "memory"                          /* assume everything is clobbered */
   );

   return ret;
}



/* call_vbeaf_ax_di:
 *  Calls a VBE/AF function, passing the driver structure in ebx and 
 *  parameters in eax and edi.
 */
static inline int call_vbeaf_ax_di(void *proc, int ax, void *di)
{
   int ret;

   asm (
      "  call *%2 "

   : "=a" (ret)                        /* return value in eax */

   : "b" (af_driver),                  /* VBE/AF driver in ds:ebx */
     "rm" (proc),                      /* function ptr in reg or mem */
     "a" (ax),                         /* load eax parameter */
     "D" (di)                          /* load edi parameter */

   : "memory"                          /* assume everything is clobbered */
   );

   return ret;
}



/* set_vbeaf_mode:
 *  Selects a VBE/AF graphics mode.
 */
static inline int set_vbeaf_mode(int mode, int w, int h, int v_w, int v_h, int *width_ret)
{
   int ret;

   if (gfx_vbeaf.linear)
      mode |= 0x4000;

   if ((v_w > w) || (v_h > h))
      mode |= 0x1000;

   asm (
      "  call *%3 "

   : "=a" (ret),                       /* return value in eax */
     "=c" (*width_ret)                 /* return scanline length in ecx */

   : "b" (af_driver),                  /* VBE/AF driver in ds:ebx */
     "rm" (af_driver->SetVideoMode),   /* mode set function in reg or mem */
     "a" (mode),                       /* mode number in eax */
     "c" (-1),                         /* scanline length in ecx */
     "d" (v_w),                        /* virtual x res in edx */
     "S" (v_h)                         /* virtual y res in esi */

   : "memory"                          /* assume everything is clobbered */
   );

   _af_active = TRUE;

   return ret;
}



/* load_vbeaf_driver:
 *  Tries to load the specified VBE/AF driver file, returning TRUE on 
 *  success. Allocates memory and reads the driver into it.
 */
static int load_vbeaf_driver(char *filename)
{
   long size;
   PACKFILE *f;

   size = file_size(filename);
   if (size <= 0)
      return FALSE;

   f = pack_fopen(filename, F_READ);
   if (!f)
      return FALSE;

   af_driver = _af_driver = malloc(size);

   if (pack_fread(af_driver, size, f) != size) {
      free(af_driver);
      af_driver = _af_driver = NULL;
      return FALSE;
   }

   pack_fclose(f);

   _go32_dpmi_lock_data(af_driver, size);

   return TRUE;
}



/* initialise_vbeaf_driver:
 *  Sets up the DPMI memory mappings required by the VBE/AF driver, 
 *  returning zero on success.
 */
static int initialise_vbeaf_driver()
{
   int c;

   if (__djgpp_nearptr_enable() == 0)
      return -1;

   /* create mapping for MMIO ports */
   for (c=0; c<4; c++) {
      if (af_driver->IOMemoryBase[c]) {
	 if (_create_linear_mapping(af_memmap+c, af_driver->IOMemoryBase[c], 
					     af_driver->IOMemoryLen[c]) != 0)
	    return -1;

	 af_driver->IOMemMaps[c] = (void *)(af_memmap[c]-__djgpp_base_address);
      }
   }

   /* create mapping for banked video RAM */
   if (af_driver->BankedBasePtr) {
      if (_create_linear_mapping(&af_banked_mem, af_driver->BankedBasePtr, 
							       0x10000) != 0)
	 return -1;

      af_driver->BankedMem = (void *)(af_banked_mem-__djgpp_base_address);
   }

   /* create mapping for linear video RAM */
   if (af_driver->LinearBasePtr) {
      if (_create_linear_mapping(&af_linear_mem, af_driver->LinearBasePtr, 
					  af_driver->LinearSize*1024) != 0)
	 return -1;

      af_driver->LinearMem  = (void *)(af_linear_mem-__djgpp_base_address);
   }

   /* create selectors for accessing real mode memory */
   if (_create_selector(&af_sel0000h, 0x00000L, 0x100000L) != 0)
      return -1;

   if (_create_selector(&af_sel0040h, 0x00400L, 0x10000L) != 0)
      return -1;

   if (_create_selector(&af_sela000h, 0xA0000L, 0x10000L) != 0)
      return -1;

   if (_create_selector(&af_selb000h, 0xB0000L, 0x10000L) != 0)
      return -1;

   if (_create_selector(&af_selc000h, 0xC0000L, 0x10000L) != 0)
      return -1;

   af_driver->Sel0000h = af_sel0000h;
   af_driver->Sel0040h = af_sel0040h;
   af_driver->SelA000h = af_sela000h;
   af_driver->SelB000h = af_selb000h;
   af_driver->SelC000h = af_selc000h;

   /* callback functions: why are these needed? ugly, IMHO */
   af_driver->Int86 = _af_int86;
   af_driver->CallRealMode = _af_call_rm;

   return 0;
}



/* find_vbeaf_mode:
 *  Tries to find a VBE/AF mode number for the specified screen size.
 */
static int find_vbeaf_mode(int w, int h, AF_MODE_INFO *mode_info)
{
   unsigned short *mode = af_driver->AvailableModes;

   /* search the list of modes */
   while (*mode != 0xFFFF) {
      if (call_vbeaf_ax_di(af_driver->GetVideoModeInfo, *mode, mode_info) == 0) {
	 if ((mode_info->XResolution == w) && 
	     (mode_info->YResolution == h) && 
	     (mode_info->BitsPerPixel == 8)) 
	    return *mode;
      } 
      mode++;
   }

   strcpy(allegro_error, "Resolution not supported");
   return 0;
}



/* vbeaf_init:
 *  Tries to enter the specified graphics mode, and makes a screen bitmap
 *  for it.
 */
static BITMAP *vbeaf_init(int w, int h, int v_w, int v_h, int color_depth)
{
   char filename[80];
   char *p;
   int width, height;
   int mode;
   AF_MODE_INFO mode_info;
   BITMAP *b;

   if (color_depth != 8) {
      strcpy(allegro_error, "VBE/AF only supports 8 bit color");
      return NULL;
   }

   /* look for driver in the default location */
   if (load_vbeaf_driver("C:\\VBEAF.DRV"))
      goto found_it;

   /* check the environment for a location */
   p = getenv("VBEAF_PATH");
   if (p) {
      strcpy(filename, p);
      put_backslash(filename);
      strcat(filename, "VBEAF.DRV");
      if (load_vbeaf_driver(filename))
	 goto found_it;
   }

   /* oops, no driver */
   strcpy(allegro_error, "Can't find VBEAF.DRV");
   return NULL;

   /* got it! */ 
   found_it:

   LOCK_VARIABLE(af_driver);
   LOCK_VARIABLE(_af_driver);
   LOCK_VARIABLE(_af_active);
   LOCK_VARIABLE(_af_set_bank);
   LOCK_VARIABLE(_af_wait_till_idle);
   LOCK_VARIABLE(_af_enable_direct_access);
   LOCK_FUNCTION(_af_wrapper);
   LOCK_FUNCTION(_vbeaf_bank);
   LOCK_FUNCTION(_vbeaf_linear_lookup);

   /* check the driver ID string */
   if (strcmp(af_driver->Signature, "VBEAF.DRV") != 0) {
      vbeaf_exit(NULL);
      strcpy(allegro_error, "Bad VBE/AF driver ID string");
      return NULL;
   }

   /* deal with all that DPMI memory mapping crap */
   if (initialise_vbeaf_driver() != 0) {
      vbeaf_exit(NULL);
      strcpy(allegro_error, "Can't map memory for VBE/AF");
      return NULL;
   }

   if (call_vbeaf((char *)af_driver+(long)af_driver->InitDriver) != 0) {
      vbeaf_exit(NULL);
      strcpy(allegro_error, "VBE/AF device not present");
      return NULL;
   }

   /* get ourselves a mode number */
   mode = find_vbeaf_mode(w, h, &mode_info);
   if (mode == 0) {
      vbeaf_exit(NULL);
      return NULL;
   }

   gfx_vbeaf.vid_mem = af_driver->TotalMemory * 1024;

   if (mode_info.Attributes & 8) {
      gfx_vbeaf.linear = TRUE;
      gfx_vbeaf.bank_size = gfx_vbeaf.bank_gran = 0;
   }
   else {
      gfx_vbeaf.linear = FALSE;
      gfx_vbeaf.bank_size = 64*1024;
      gfx_vbeaf.bank_gran = af_driver->BankSize * 1024;
   }

   width = MAX(mode_info.BytesPerScanLine, v_w);
   height = MAX(h, v_h);
   _sort_out_virtual_width(&width, &gfx_vbeaf);

   if (set_vbeaf_mode(mode, w, h, width, height, &width) != 0) {
      strcpy(allegro_error, "Failed to set VBE/AF mode");
      vbeaf_exit(NULL);
      return NULL;
   }

   in_af_mode = TRUE;

   if ((width < v_w) || (width < w) || (height < v_h) || (height < h)) {
      strcpy(allegro_error, "Virtual screen size too large");
      vbeaf_exit(NULL);
      return NULL;
   }

   b = _make_bitmap(width, height, gfx_vbeaf.linear ? 
		    (unsigned long)af_driver->LinearMem : 
		    (unsigned long)af_driver->BankedMem, 
		    &gfx_vbeaf, color_depth, width);
   if (!b) {
      vbeaf_exit(NULL);
      return NULL;
   }

   gfx_vbeaf.vid_phys_base = (gfx_vbeaf.linear ? af_driver->LinearBasePtr : 
						 af_driver->BankedBasePtr);

   b->seg = _my_ds();

   gfx_vbeaf.w = b->cr = w;
   gfx_vbeaf.h = b->cb = h;

   _af_set_bank = af_driver->SetBank;
   _af_wait_till_idle = af_driver->WaitTillIdle;
   _af_enable_direct_access = af_driver->EnableDirectAccess;

   if (gfx_vbeaf.linear)
      b->write_bank = b->read_bank = _vbeaf_linear_lookup;
   else
      b->write_bank = b->read_bank = _vbeaf_bank;

   /* build the VBE/AF description string */
   sprintf(vbeaf_desc, "VBE/AF %ld.%ld (%s)", af_driver->Version>>8,
			af_driver->Version&0xFF, af_driver->OemVendorName);

   if (!(mode_info.Attributes & 0x10))
      strcat(vbeaf_desc, ", not accelerated");

   vbeaf_xscroll = vbeaf_yscroll = 0;

   return b;
}



/* vbeaf_vsync:
 *  VBE/AF vsync routine, needed for cards that don't emulate the VGA 
 *  blanking registers. VBE/AF doesn't provide a vsync function, but we 
 *  can emulate it by altering the display start address with the vsync 
 *  flag set.
 */
static void vbeaf_vsync()
{
   vbeaf_scroll(vbeaf_xscroll, vbeaf_yscroll);
}



/* vbeaf_scroll:
 *  Hardware scrolling routine.
 */
static int vbeaf_scroll(int x, int y)
{
   vbeaf_xscroll = x;
   vbeaf_yscroll = y;

   if (af_driver->WaitTillIdle)
      call_vbeaf(af_driver->WaitTillIdle);

   asm (
      "  call *%1 "

   :                                      /* no outputs */

   : "b" (af_driver),                     /* VBE/AF driver in ds:ebx */
     "rm" (af_driver->SetDisplayStart),   /* scroll function in reg or mem */
     "a" (1),                             /* retrace flag in eax */
     "c" (x),                             /* x in ecx */
     "d" (y)                              /* y in edx */

   : "memory"                             /* assume everything is clobbered */
   );

   return 0;
}



/* vbeaf_set_pallete_range:
 *  Uses VBE/AF functions to set the pallete.
 */
static void vbeaf_set_pallete_range(PALLETE p, int from, int to, int vsync)
{
   int c;
   PALLETE tmp;

   /* swap the pallete into the funny order VBE/AF uses */
   for (c=from; c<=to; c++) {
      tmp[c].r = p[c].b << 2;
      tmp[c].g = p[c].g << 2;
      tmp[c].b = p[c].r << 2;
   }

   asm (
      "  call *%1 "

   :                                      /* no outputs */

   : "b" (af_driver),                     /* VBE/AF driver in ds:ebx */
     "rm" (af_driver->SetPaletteData),    /* pallete function in reg or mem */
     "a" (vsync ? 1 : 0),                 /* retrace flag in eax */
     "c" (to-from+1),                     /* how many colors in ecx */
     "d" (from),                          /* first color in edx */
     "S" (tmp+from)                       /* pallete data pointer in esi */

   : "memory"                             /* assume everything is clobbered */
   );
}



/* vbeaf_exit:
 *  Shuts down the VBE/AF driver.
 */
static void vbeaf_exit(BITMAP *b)
{
   int c;

   if (in_af_mode) {
      call_vbeaf(af_driver->RestoreTextMode);
      in_af_mode = FALSE;
   }

   for (c=0; c<4; c++)
      _remove_linear_mapping(af_memmap+c);

   _remove_linear_mapping(&af_banked_mem);
   _remove_linear_mapping(&af_linear_mem);

   _remove_selector(&af_sel0000h);
   _remove_selector(&af_sel0040h);
   _remove_selector(&af_sela000h);
   _remove_selector(&af_selb000h);
   _remove_selector(&af_selc000h);

   if (af_driver) {
      free(af_driver);
      af_driver = _af_driver = NULL;
   }
}


