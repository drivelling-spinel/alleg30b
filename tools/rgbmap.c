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
 *      RGB -> palette mapping table construction utility.
 *
 *      See readme.txt for copyright information.
 */


#include <stdlib.h>
#include <stdio.h>

#include "allegro.h"


void usage() __attribute__ ((noreturn));


char **__crt0_glob_function(char *_arg)
{
   /* don't let djgpp glob our command line arguments */
   return NULL;
}



void usage()
{
   printf("\nRGB map construction utility for Allegro " ALLEGRO_VERSION_STR);
   printf("\nBy Shawn Hargreaves, " ALLEGRO_DATE_STR "\n\n");
   printf("Usage: rgbmap palfile.[pcx|bmp] outputfile\n\n");
   printf("   Reads a palette from the input file, and writes out a 32k mapping\n");
   printf("   table for converting RGB values to this palette (in a suitable\n");
   printf("   format for use with the global rgb_map pointer).\n");

   exit(1);
}



void print_progress(int pos)
{
   if ((pos & 3) == 3) {
      printf("*");
      fflush(stdout);
   }
}



PALLETE the_pal;
RGB_MAP the_map;



int main(int argc, char *argv[])
{
   BITMAP *bmp;
   PACKFILE *f;

   if (argc != 3)
      usage();

   bmp = load_bitmap(argv[1], the_pal);
   if (!bmp) {
      printf("Error reading palette from '%s'\n", argv[1]);
      return 1;
   }
   destroy_bitmap(bmp);

   printf("Palette read from '%s'\n", argv[1]);
   printf("Creating RGB map\n");
   printf("<................................................................>\r<");

   create_rgb_table(&the_map, the_pal, print_progress);
   printf("\n");

   f = pack_fopen(argv[2], F_WRITE);
   if (!f) {
      printf("Error writing '%s'\n", argv[2]);
      return 1;
   }

   pack_fwrite(&the_map, sizeof(the_map), f);
   pack_fclose(f);

   printf("RGB mapping table written to '%s'\n", argv[2]);
   return 0;
}
