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
 *      Grabber datafile -> asm converter for the Allegro library.
 *
 *      See readme.txt for copyright information.
 */


#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <crt0.h>

#include "allegro.h"
#include "datedit.h"


void usage() __attribute__ ((noreturn));


char **__crt0_glob_function(char *_arg)
{
   /* don't let djgpp glob our command line arguments */
   return NULL;
}



void usage()
{
   fprintf(stderr, "\nDatafile -> asm conversion utility for Allegro " ALLEGRO_VERSION_STR);
   fprintf(stderr, "\nBy Shawn Hargreaves, " ALLEGRO_DATE_STR "\n\n");
   fprintf(stderr, "Usage: dat2s [options] inputfile.dat\n\n");
   fprintf(stderr, "Options:\n");
   fprintf(stderr, "\t'-o outputfile.s' sets the output file (default stdout)\n");
   fprintf(stderr, "\t'-h outputfile.h' sets the output header file (default none)\n");
   fprintf(stderr, "\t'-p prefix' sets the object name prefix string\n");
   fprintf(stderr, "\t'-007 password' sets the datafile password\n");

   exit(1);
}



int err = 0;
int truecolor = FALSE;

char prefix[80] = "";

char *infilename = NULL;
char *outfilename = NULL;
char *outfilenameheader = NULL;
char *password = NULL;

DATAFILE *data = NULL;

FILE *outfile = NULL;
FILE *outfileheader = NULL;

void output_object(DATAFILE *object, char *name);


/* unused callbacks for datedit.c */
void datedit_msg(char *fmt, ...) { }
void datedit_startmsg(char *fmt, ...) { }
void datedit_endmsg(char *fmt, ...) { }
void datedit_error(char *fmt, ...) { }
int datedit_ask(char *fmt, ...) { return 0; }



void write_data(unsigned char *data, int size)
{
   int c;

   for (c=0; c<size; c++) {
      if ((c & 7) == 0)
	 fprintf(outfile, "\t.byte ");

      fprintf(outfile, "0x%02X", data[c]);

      if ((c<size-1) && ((c & 7) != 7))
	 fprintf(outfile, ", ");
      else
	 fprintf(outfile, "\n");
   }
}



void output_data(unsigned char *data, int size, char *name, char *type, int global)
{
   fprintf(outfile, "# %s (%d bytes)\n", type, size);
   if (global)
      fprintf(outfile, ".globl _%s%s\n", prefix, name);
   fprintf(outfile, ".align 4\n_%s%s:\n", prefix, name);

   write_data(data, size);

   fprintf(outfile, "\n");
}



void output_bitmap(BITMAP *bmp, char *name, int global)
{
   int bpp = bitmap_color_depth(bmp);
   int bypp = (bpp + 7) / 8;
   char buf[160];
   int c;

   if (bpp > 8)
      truecolor = TRUE;

   strcpy(buf, name);
   strcat(buf, "_data");

   output_data(bmp->line[0], bmp->w*bmp->h*bypp, buf, "bitmap data", FALSE);

   fprintf(outfile, "# bitmap\n");
   if (global)
      fprintf(outfile, ".globl _%s%s\n", prefix, name);
   fprintf(outfile, ".align 4\n_%s%s:\n", prefix, name);

   fprintf(outfile, "\t.long %-16d# w\n", bmp->w);
   fprintf(outfile, "\t.long %-16d# h\n", bmp->h);
   fprintf(outfile, "\t.long %-16d# clip\n", -1);
   fprintf(outfile, "\t.long %-16d# cl\n", 0);
   fprintf(outfile, "\t.long %-16d# cr\n", bmp->w);
   fprintf(outfile, "\t.long %-16d# ct\n", 0);
   fprintf(outfile, "\t.long %-16d# cb\n", bmp->h);
   fprintf(outfile, "\t.long ___linear_vtable%d\n", bpp);
   fprintf(outfile, "\t.long __stub_bank_switch\n");
   fprintf(outfile, "\t.long __stub_bank_switch\n");
   fprintf(outfile, "\t.long _%s%s_data\n", prefix, name);
   fprintf(outfile, "\t.long %-16d# bitmap_id\n", 0);
   fprintf(outfile, "\t.long %-16d# extra\n", 0);
   fprintf(outfile, "\t.long %-16d# line_ofs\n", 0);
   fprintf(outfile, "\t.long %-16d# seg\n", 0);

   for (c=0; c<bmp->h; c++)
      fprintf(outfile, "\t.long _%s%s_data + %d\n", prefix, name, bmp->w*c*bypp);

   fprintf(outfile, "\n");
}



void output_sample(SAMPLE *spl, char *name)
{
   char buf[160];

   strcpy(buf, name);
   strcat(buf, "_data");

   output_data(spl->data, spl->len, buf, "waveform data", FALSE);

   fprintf(outfile, "# sample\n.globl _%s%s\n", prefix, name);
   fprintf(outfile, ".align 4\n_%s%s:\n", prefix, name);
   fprintf(outfile, "\t.long %-16d# bits\n", spl->bits);
   fprintf(outfile, "\t.long %-16d# freq\n", spl->freq);
   fprintf(outfile, "\t.long %-16d# priority\n", spl->priority);
   fprintf(outfile, "\t.long %-16ld# length\n", spl->len);
   fprintf(outfile, "\t.long %-16ld# loop_start\n", spl->loop_start);
   fprintf(outfile, "\t.long %-16ld# loop_end\n", spl->loop_end);
   fprintf(outfile, "\t.long %-16ld# param\n", spl->param);
   fprintf(outfile, "\t.long _%s%s_data\n\n", prefix, name);
}



void output_midi(MIDI *midi, char *name)
{
   char buf[160];
   int c;

   for (c=0; c<MIDI_TRACKS; c++) {
      if (midi->track[c].data) {
	 sprintf(buf, "%s_track_%d", name, c);
	 output_data(midi->track[c].data, midi->track[c].len, buf, "midi track", FALSE);
      }
   }

   fprintf(outfile, "# midi file\n.globl _%s%s\n", prefix, name);
   fprintf(outfile, ".align 4\n_%s%s:\n", prefix, name);
   fprintf(outfile, "\t.long %-16d# divisions\n", midi->divisions);

   for (c=0; c<MIDI_TRACKS; c++)
      if (midi->track[c].data)
	 fprintf(outfile, "\t.long _%s%s_track_%d, %d\n", prefix, name, c, midi->track[c].len);
      else
	 fprintf(outfile, "\t.long 0, 0\n");

   fprintf(outfile, "\n");
}



void output_font(FONT *font, char *name)
{
   char buf[160];
   int c;

   if (font->height > 0) {
      strcpy(buf, name);
      strcat(buf, "_data");
      if (font->height == 8)
	 output_data((unsigned char *)font->dat.dat_8x8, sizeof(FONT_8x8), buf, "8x8 font", FALSE);
      else
	 output_data((unsigned char *)font->dat.dat_8x16, sizeof(FONT_8x16), buf, "8x16 font", FALSE);
   }
   else {
      for (c=0; c<FONT_SIZE; c++) {
	 sprintf(buf, "%s_char_%d", name, c);
	 output_bitmap(font->dat.dat_prop->dat[c], buf, FALSE);
      }

      fprintf(outfile, "# proportional font\n");
      fprintf(outfile, ".align 4\n_%s%s_data:\n", prefix, name);
      for (c=0; c<FONT_SIZE; c++)
	 fprintf(outfile, "\t.long _%s%s_char_%d\n", prefix, name, c);
      fprintf(outfile, "\n");
   }

   fprintf(outfile, "# font\n.globl _%s%s\n", prefix, name);
   fprintf(outfile, ".align 4\n_%s%s:\n", prefix, name);
   fprintf(outfile, "\t.long %-16d# height\n", font->height);
   fprintf(outfile, "\t.long _%s%s_data\n", prefix, name);
   fprintf(outfile, "\n");
}



void output_rle_sprite(RLE_SPRITE *sprite, char *name)
{
   int bpp = sprite->color_depth;

   if (bpp > 8)
      truecolor = TRUE;

   fprintf(outfile, "# RLE sprite\n.globl _%s%s\n", prefix, name);
   fprintf(outfile, ".align 4\n_%s%s:\n", prefix, name);
   fprintf(outfile, "\t.long %-16d# w\n", sprite->w);
   fprintf(outfile, "\t.long %-16d# h\n", sprite->h);
   fprintf(outfile, "\t.long %-16d# color depth\n", bpp);
   fprintf(outfile, "\t.long %-16d# size\n", sprite->size);

   write_data(sprite->dat, sprite->size);

   fprintf(outfile, "\n");
}



void output_compiled_sprite(COMPILED_SPRITE *sprite, char *name)
{
   char buf[160];
   int c;

   if (sprite->color_depth != 8) {
      fprintf(stderr, "\nError: truecolor compiled sprites not supported (%s)\n", name);
      err = 1;
      return;
   }

   for (c=0; c<4; c++) {
      if (sprite->proc[c].draw) {
	 sprintf(buf, "%s_plane_%d", name, c);
	 output_data(sprite->proc[c].draw, sprite->proc[c].len, buf, "compiled sprite code", FALSE);
      }
   } 

   fprintf(outfile, "# compiled sprite\n.globl _%s%s\n", prefix, name);
   fprintf(outfile, ".align 4\n_%s%s:\n", prefix, name);
   fprintf(outfile, "\t.short %-15d# planar\n", sprite->planar);
   fprintf(outfile, "\t.short %-15d# color depth\n", sprite->color_depth);
   fprintf(outfile, "\t.short %-15d# w\n", sprite->w);
   fprintf(outfile, "\t.short %-15d# h\n", sprite->h);

   for (c=0; c<4; c++) {
      if (sprite->proc[c].draw) {
	 fprintf(outfile, "\t.long _%s%s_plane_%d\n", prefix, name, c);
	 fprintf(outfile, "\t.long %-16d# len\n", sprite->proc[c].len);
      }
      else {
	 fprintf(outfile, "\t.long 0\n");
	 fprintf(outfile, "\t.long %-16d# len\n", 0);
      }
   }

   fprintf(outfile, "\n");
}



void get_object_name(char *buf, char *name, DATAFILE *dat, int root)
{
   if (!root) {
      strcpy(buf, name);
      strcat(buf, "_");
   }
   else
      buf[0] = 0;

   strcat(buf, get_datafile_property(dat, DAT_NAME));
   strlwr(buf);
}



void output_datafile(DATAFILE *dat, char *name, int root)
{
   char buf[160];
   int c;

   for (c=0; (dat[c].type != DAT_END) && (!err); c++) {
      get_object_name(buf, name, dat+c, root);
      output_object(dat+c, buf);
   }

   fprintf(outfile, "# datafile\n.globl _%s%s\n", prefix, name);
   fprintf(outfile, ".align 4\n_%s%s:\n", prefix, name);

   if (outfileheader)
      fprintf(outfileheader, "extern DATAFILE %s%s[];\n", prefix, name);

   for (c=0; dat[c].type != DAT_END; c++) {
      get_object_name(buf, name, dat+c, root);
      fprintf(outfile, "\t.long _%s%s\n", prefix, buf);
      fprintf(outfile, "\t.long %-16d# %c%c%c%c\n", dat[c].type,
		       (dat[c].type>>24) & 0xFF, (dat[c].type>>16) & 0xFF,
		       (dat[c].type>>8) & 0xFF, dat[c].type & 0xFF);
      fprintf(outfile, "\t.long %-16ld# size\n", dat[c].size);
      fprintf(outfile, "\t.long %-16d# properties\n", 0);
   }

   fprintf(outfile, "\t.long 0\n\t.long -1\n\t.long 0\n\t.long 0\n\n");
}



void output_object(DATAFILE *object, char *name)
{
   switch (object->type) {

      default:
      case DAT_DATA:
	 if (outfileheader)
	    fprintf(outfileheader, "extern unsigned char %s%s[];\n", prefix, name);

	 output_data(object->dat, object->size, name, "binary data", TRUE);
	 break;

      case DAT_FONT:
	 if (outfileheader)
	    fprintf(outfileheader, "extern FONT %s%s;\n", prefix, name);

	 output_font((FONT *)object->dat, name);
	 break;

      case DAT_BITMAP:
	 if (outfileheader)
	    fprintf(outfileheader, "extern BITMAP %s%s;\n", prefix, name);

	 output_bitmap((BITMAP *)object->dat, name, TRUE);
	 break;

      case DAT_PALLETE:
	 if (outfileheader)
	    fprintf(outfileheader, "extern PALLETE %s%s;\n", prefix, name);

	 output_data(object->dat, sizeof(PALLETE), name, "pallete", TRUE);
	 break;

      case DAT_SAMPLE:
	 if (outfileheader)
	    fprintf(outfileheader, "extern SAMPLE %s%s;\n", prefix, name);

	 output_sample((SAMPLE *)object->dat, name);
	 break;

      case DAT_MIDI:
	 if (outfileheader)
	    fprintf(outfileheader, "extern MIDI %s%s;\n", prefix, name);

	 output_midi((MIDI *)object->dat, name);
	 break;

      case DAT_PATCH:
	 printf("Compiled GUS patch objects are not supported!\n");
	 break;

      case DAT_RLE_SPRITE:
	 if (outfileheader)
	    fprintf(outfileheader, "extern RLE_SPRITE %s%s;\n", prefix, name);

	 output_rle_sprite((RLE_SPRITE *)object->dat, name);
	 break;

      case DAT_FLI:
	 if (outfileheader)
	    fprintf(outfileheader, "extern unsigned char %s%s[];\n", prefix, name);

	 output_data(object->dat, object->size, name, "FLI/FLC animation", TRUE);
	 break;

      case DAT_C_SPRITE:
      case DAT_XC_SPRITE:
	 if (outfileheader)
	    fprintf(outfileheader, "extern COMPILED_SPRITE %s%s;\n", prefix, name);

	 output_compiled_sprite((COMPILED_SPRITE *)object->dat, name);
	 break;

      case DAT_FILE:
	 output_datafile((DATAFILE *)object->dat, name, FALSE);
	 break;
   }
}



int main(int argc, char *argv[])
{
   int c;
   char tm[80];
   time_t now;

   time(&now);
   strcpy(tm, asctime(localtime(&now)));
   for (c=0; tm[c]; c++)
      if ((tm[c] == '\r') || (tm[c] == '\n'))
	 tm[c] = 0;

   for (c=1; c<argc; c++) {
      if (stricmp(argv[c], "-o") == 0) {
	 if ((outfilename) || (c >= argc-1))
	    usage();
	 outfilename = argv[++c];
      }
      else if (stricmp(argv[c], "-h") == 0) {
	 if ((outfilenameheader) || (c >= argc-1))
	    usage();
	 outfilenameheader = argv[++c];
      }
      else if (stricmp(argv[c], "-p") == 0) {
	 if ((prefix[0]) || (c >= argc-1))
	    usage();
	 strcpy(prefix, argv[++c]);
      }
      else if (stricmp(argv[c], "-007") == 0) {
	 if ((password) || (c >= argc-1))
	    usage();
	 password = argv[++c];
      }
      else {
	 if ((argv[c][0] == '-') || (infilename))
	    usage();
	 infilename = argv[c];
      }
   }

   if (!infilename)
      usage();

   if ((prefix[0]) && (prefix[strlen(prefix)-1] != '_'))
      strcat(prefix, "_");

   set_color_conversion(COLORCONV_NONE);

   data = datedit_load_datafile(infilename, TRUE, password);
   if (!data) {
      fprintf(stderr, "\nError reading %s\n", infilename);
      err = 1; 
      goto ohshit;
   }

   if (outfilename) {
      outfile = fopen(outfilename, "w");
      if (!outfile) {
	 fprintf(stderr, "\nError writing %s\n", outfilename);
	 err = 1; 
	 goto ohshit;
      }
   }
   else
      outfile = stdout;

   fprintf(outfile, "/* Compiled Allegro data file, produced by dat2s v" ALLEGRO_VERSION_STR " */\n");
   fprintf(outfile, "/* Input file: %s */\n", infilename);
   fprintf(outfile, "/* Date: %s */\n", tm);
   fprintf(outfile, "/* Do not hand edit! */\n\n.data\n\n");

   if (outfilenameheader) {
      outfileheader = fopen(outfilenameheader, "w");
      if (!outfileheader) {
	 fprintf(stderr, "\nError writing %s\n", outfilenameheader);
	 err = 1; 
	 goto ohshit;
      }
      fprintf(outfileheader, "/* Allegro data file definitions, produced by dat2s v" ALLEGRO_VERSION_STR " */\n");
      fprintf(outfileheader, "/* Input file: %s */\n", infilename);
      fprintf(outfileheader, "/* Date: %s */\n", tm);
      fprintf(outfileheader, "/* Do not hand edit! */\n\n");
   }

   if (outfilename)
      printf("converting %s to %s...\n", infilename, outfilename);

   output_datafile(data, "data", TRUE);

   fprintf(outfile, ".text\n");
   fprintf(outfile, ".align 4\n");
   fprintf(outfile, "__construct_me:\n");
   fprintf(outfile, "\tpushl %%ebp\n");
   fprintf(outfile, "\tmovl %%esp, %%ebp\n");
   fprintf(outfile, "\tpushl $_%sdata\n", prefix);
   fprintf(outfile, "\tcall __construct_datafile\n");
   fprintf(outfile, "\taddl $4, %%esp\n");
   fprintf(outfile, "\tleave\n");
   fprintf(outfile, "\tret\n\n");
   fprintf(outfile, ".section .ctor\n");
   fprintf(outfile, "\t.long __construct_me\n");

   ohshit:

   if ((outfile) && (outfile != stdout))
      fclose(outfile);

   if (outfileheader)
      fclose(outfileheader);

   if (data)
      unload_datafile(data);

   if (err) {
      if (outfilename)
	 delete_file(outfilename);

      if (outfilenameheader)
	 delete_file(outfilenameheader);
   }
   else if (truecolor)
      printf("data contains truecolor images: call fixup_datafile() before using it!\n");

   return err;
}

