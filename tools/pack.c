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
 *      File compression utility for the Allegro library.
 *
 *      See readme.txt for copyright information.
 */


#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <crt0.h>

#include "allegro.h"


void usage() __attribute__ ((noreturn));
void err() __attribute__ ((noreturn));


char **__crt0_glob_function(char *_arg)
{
   /* don't let djgpp glob our command line arguments */
   return NULL;
}



void usage()
{
   printf("\nFile compression utility for Allegro " ALLEGRO_VERSION_STR);
   printf("\nBy Shawn Hargreaves, " ALLEGRO_DATE_STR "\n\n");
   printf("Usage: 'pack <in> <out>' to pack a file\n");
   printf("       'pack u <in> <out>' to unpack a file\n");

   exit(1);
}



void err(char *s1, char *s2)
{
   printf("\nError %d", errno);

   if (s1)
      printf(": %s", s1);

   if (s2)
      printf(s2);

   printf("\n");

   if (errno==EDOM)
      printf("Not a packed file\n");

   exit(1);
}



int main(int argc, char *argv[])
{
   char *f1, *f2;
   char *m1, *m2;
   long s1, s2;
   char *t;
   PACKFILE *in, *out;
   int c;

   if (argc==3) {
      f1 = argv[1];
      f2 = argv[2];
      m1 = F_READ;
      m2 = F_WRITE_PACKED;
      t = "Pack";
   }
   else if ((argc==4) && (argv[1][1]==0) &&
	    ((argv[1][0]=='u') || (argv[1][0]=='U'))) {
      f1 = argv[2];
      f2 = argv[3];
      m1 = F_READ_PACKED;
      m2 = F_WRITE;
      t = "Unpack";
   }
   else
      usage();

   s1 = file_size(f1);

   in = pack_fopen(f1, m1);
   if (!in)
      err("can't open ", f1);

   out = pack_fopen(f2, m2);
   if (!out) {
      delete_file(f2);
      pack_fclose(in);
      err("can't create ", f2);
   }

   printf("%sing %s into %s...\n", t, f1, f2);

   while (!pack_feof(in)) {
      c = pack_getc(in);
      if (pack_putc(c, out) != c)
	 break;
   }

   pack_fclose(in);
   pack_fclose(out);

   if (errno) {
      delete_file(f2);
      err(NULL, NULL);
   }

   if (s1 > 0) {
      s2 = file_size(f2);
      printf("\nInput size: %ld\nOutput size: %ld\n%ld%%\n",
					      s1, s2, (s2*100+(s1>>1))/s1);
   }

   return 0;
}
