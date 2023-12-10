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
 *      List of available graphics drivers, kept in a seperate file so that
 *      they can be overriden by user programs.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"



DECLARE_GFX_DRIVER_LIST
(
   GFX_DRIVER_VGA
   GFX_DRIVER_MODEX
   GFX_DRIVER_VBEAF
   GFX_DRIVER_VESA2L
   GFX_DRIVER_VESA2B
   GFX_DRIVER_XTENDED
   GFX_DRIVER_ATI
   GFX_DRIVER_MACH64
   GFX_DRIVER_CIRRUS64
   GFX_DRIVER_CIRRUS54
   GFX_DRIVER_PARADISE
   GFX_DRIVER_S3
   GFX_DRIVER_TRIDENT
   GFX_DRIVER_ET3000
   GFX_DRIVER_ET4000
   GFX_DRIVER_VIDEO7
   GFX_DRIVER_VESA1
)

