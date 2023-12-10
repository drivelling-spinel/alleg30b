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
 *      Datafile editing functions, for use by the datafile tools.
 *
 *      See readme.txt for copyright information.
 */


#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#include "allegro.h"
#include "datedit.h"
#include "internal.h"


PALLETE current_pallete;

char info_msg[] = "For internal use by the grabber";

DATAFILE info = { info_msg, DAT_INFO, sizeof(info_msg), NULL };

extern int _packfile_filesize, _packfile_datasize;
int file_datasize;

PALLETE last_read_pal;

void _unload_datafile_object(DATAFILE *dat);

#define GRAB_UNUSED long *size, int x, int y, int w, int h, int depth
#define SAVE_UNUSED int packed, int packkids, int strip, int verbose, int extra



/* functions for handling binary objects:
 */

int export_binary(DATAFILE *dat, char *filename)
{
   PACKFILE *f = pack_fopen(filename, F_WRITE);

   if (f) {
      pack_fwrite(dat->dat, dat->size, f);
      pack_fclose(f);
   }

   return (errno == 0);
}



void *grab_binary(char *filename, GRAB_UNUSED)
{
   void *mem;
   long sz = file_size(filename);
   PACKFILE *f;

   if (sz <= 0)
      return NULL;

   mem = malloc(sz);

   f = pack_fopen(filename, F_READ);
   if (!f) {
      free(mem);
      return NULL; 
   }

   pack_fread(mem, sz, f);
   pack_fclose(f);

   if (errno) {
      free(mem);
      return NULL;
   }

   *size = sz;
   return mem;
}



void save_binary(DATAFILE *dat, SAVE_UNUSED, PACKFILE *f)
{
   pack_fwrite(dat->dat, dat->size, f);
}



/* functions for handling datafile objects:
 */

int export_datafile(DATAFILE *dat, char *filename)
{
   return datedit_save_datafile((DATAFILE *)dat->dat, filename, -1, -1, FALSE, FALSE, FALSE, NULL);
}



/* loads object names from a header file, if they are missing */
void load_header(DATAFILE *dat, char *filename)
{
   char buf[160], buf2[160];
   int datsize, i, c, c2;
   PACKFILE *f;

   datsize = 0;
   while (dat[datsize].type != DAT_END)
      datsize++;

   f = pack_fopen(filename, F_READ);

   if (f) {
      while (pack_fgets(buf, 160, f) != 0) {
	 if (strncmp(buf, "#define ", 8) == 0) {
	    c2 = 0;
	    c = 8;

	    while ((buf[c]) && (buf[c] != ' '))
	       buf2[c2++] = buf[c++];

	    buf2[c2] = 0;
	    while (buf[c]==' ')
	       c++;

	    i = 0;
	    while ((buf[c] >= '0') && (buf[c] <= '9')) {
	       i *= 10;
	       i += buf[c] - '0';
	       c++;
	    }

	    if ((i < datsize) && (!*get_datafile_property(dat+i, DAT_NAME)))
	       datedit_set_property(dat+i, DAT_NAME, buf2);
	 }
      }

      pack_fclose(f);
   }
}



/* generates object names, if they are missing */
int generate_names(DATAFILE *dat, int n)
{
   int i;

   while (dat->type != DAT_END) {
      if (!*get_datafile_property(dat, DAT_NAME)) {
	 char tmp[32];

	 sprintf(tmp, "%03d_%c%c%c%c", n++,
			(dat->type>>24)&0xFF, (dat->type>>16)&0xFF,
			(dat->type>>8)&0xFF, dat->type&0xFF);

	 for (i=4; tmp[i]; i++)
	    if (!isalnum(tmp[i]))
	       tmp[i] = 0;

	 datedit_set_property(dat, DAT_NAME, tmp);
      }

      if (dat->type == DAT_FILE)
	 n = generate_names((DATAFILE *)dat->dat, n);

      dat++;
   }

   return n;
}



/* retrieves whatever grabber information is stored in a datafile */
DATAFILE *extract_info(DATAFILE *dat, int save)
{
   DATAFILE_PROPERTY *prop;
   int i;

   if (save) {
      prop = info.prop;
      while ((prop) && (prop->type != DAT_END)) {
	 if (prop->dat)
	    free(prop->dat);
	 prop++;
      }
      if (info.prop) {
	 free(info.prop);
	 info.prop = NULL;
      }
   }

   for (i=0; dat[i].type != DAT_END; i++) {
      if (dat[i].type == DAT_INFO) {
	 if (save) {
	    prop = dat[i].prop;
	    while ((prop) && (prop->type != DAT_END)) {
	       datedit_set_property(&info, prop->type, prop->dat);
	       prop++;
	    }
	 }

	 dat = datedit_delete(dat, i);
	 i--;
      }
   }

   return dat;
}



void *grab_datafile(char *filename, GRAB_UNUSED)
{
   DATAFILE *dat = load_datafile(filename);

   if (dat) {
      load_header(dat, datedit_pretty_name(filename, "h", TRUE));
      generate_names(dat, 0);
      dat = extract_info(dat, FALSE);
   }

   return dat;
}



int should_save_prop(int type, int strip)
{
   if (strip == 0)
      return TRUE;
   else if (strip >= 2)
      return FALSE;
   else
      return ((type != DAT_ORIG) &&
	      (type != DAT_DATE) && 
	      (type != DAT_XPOS) && 
	      (type != DAT_YPOS) &&
	      (type != DAT_XSIZ) && 
	      (type != DAT_YSIZ));
}



int percent(int a, int b)
{
   if (a)
      return (b * 100) / a;
   else
      return 0;
}



void save_object(DATAFILE *dat, int packed, int packkids, int strip, int verbose, PACKFILE *f)
{
   int i;
   DATAFILE_PROPERTY *prop;
   void (*save)();

   prop = dat->prop;
   datedit_sort_properties(prop);

   while ((prop) && (prop->type != DAT_END)) {
      if (should_save_prop(prop->type, strip)) {
	 pack_mputl(DAT_PROPERTY, f);
	 pack_mputl(prop->type, f);
	 pack_mputl(strlen(prop->dat), f);
	 pack_fwrite(prop->dat, strlen(prop->dat), f);
	 file_datasize += 12 + strlen(prop->dat);
      }

      prop++;
      if (errno)
	 return;
   }

   if (verbose)
      datedit_startmsg("%-28s", get_datafile_property(dat, DAT_NAME));

   pack_mputl(dat->type, f);
   f = pack_fopen_chunk(f, ((!packed) && (packkids) && (dat->type != DAT_FILE)));
   file_datasize += 12;

   save = NULL;

   for (i=0; object_info[i].type != DAT_END; i++) {
      if (object_info[i].type == dat->type) {
	 save = object_info[i].save;
	 break;
      }
   }

   if (!save)
      save = save_binary;

   if (dat->type == DAT_FILE) {
      if (verbose)
	 datedit_endmsg("");

      save((DATAFILE *)dat->dat, packed, packkids, strip, verbose, FALSE, f);

      if (verbose)
	 datedit_startmsg("End of %-21s", get_datafile_property(dat, DAT_NAME));
   }
   else
      save(dat, (packed || packkids), FALSE, strip, verbose, FALSE, f);

   f = pack_fclose_chunk(f);

   if (verbose) {
      if ((!packed) && (packkids) && (dat->type != DAT_FILE)) {
	 datedit_endmsg("%7d bytes into %-7d (%d%%)", 
			_packfile_datasize, _packfile_filesize, 
			percent(_packfile_datasize, _packfile_filesize));
      }
      else
	 datedit_endmsg("");
   }

   if (dat->type == DAT_FILE)
      file_datasize += 4;
   else
      file_datasize += _packfile_datasize;
}



void save_datafile(DATAFILE *dat, int packed, int packkids, int strip, int verbose, int extra, PACKFILE *f)
{
   int c, size;

   datedit_sort_datafile(dat);

   size = 0;
   while (dat[size].type != DAT_END)
      size++;

   pack_mputl(extra ? size+1 : size, f);

   for (c=0; c<size; c++) {
      save_object(dat+c, packed, packkids, strip, verbose, f);

      if (errno)
	 return;
   }
}



/* functions for handling font objects:
 */

void get_font_desc(DATAFILE *dat, char *s)
{
   FONT *font = (FONT *)dat->dat;

   if (font->height < 0)
      strcpy(s, "proportional font");
   else
      sprintf(s, "8x%d font", font->height);
}



int export_font(DATAFILE *dat, char *filename)
{
   FONT *font = (FONT *)dat->dat;
   FONT_PROP *font_prop = NULL;
   BITMAP *b;
   char buf[2];
   int w, h, c;

   w = 0;
   h = 0;

   if (font->height < 0) {
      font_prop = font->dat.dat_prop;

      for (c=0; c<FONT_SIZE; c++) {
	 if (font_prop->dat[c]->w > w)
	    w = font_prop->dat[c]->w;
	 if (font_prop->dat[c]->h > h)
	    h = font_prop->dat[c]->h;
      }
   }
   else {
      w = 8;
      h = font->height;
   }

   w = (w+16) & 0xFFF0;
   h = (h+16) & 0xFFF0;

   b = create_bitmap_ex(8, 1+w*16, 1+h*((FONT_SIZE+15)/16));
   rectfill(b, 0, 0, b->w, b->h, 255);
   text_mode(0);

   for (c=0; c<FONT_SIZE; c++) {
      buf[0] = c + ' ';
      buf[1] = 0;

      textout(b, font, buf, 1+w*(c&15), 1+h*(c/16), font_prop ? -1 : 1);
   }

   save_bitmap(filename, b, desktop_pallete);
   destroy_bitmap(b);

   return (errno == 0);
}



/* GRX font file reader by Mark Wodrich.
 *
 * GRX FNT files consist of the header data (see struct below). If the font
 * is proportional, followed by a table of widths per character (unsigned 
 * shorts). Then, the data for each character follows. 1 bit/pixel is used,
 * with each line of the character stored in contiguous bytes. High bit of
 * first byte is leftmost pixel of line.
 *
 * Note : FNT files can have a variable number of characters, so we must
 *        check that the chars 32..127 exist.
 */


#define FONTMAGIC       0x19590214L


/* .FNT file header */
typedef struct {
   unsigned long  magic;
   unsigned long  bmpsize;
   unsigned short width;
   unsigned short height;
   unsigned short minchar;
   unsigned short maxchar;
   unsigned short isfixed;
   unsigned short reserved;
   unsigned short baseline;
   unsigned short undwidth;
   char           fname[16];
   char           family[16];
} FNTfile_header;


/* all we need is an array of bytes for each character */
typedef unsigned char * GRX_BITMAP;


#define GRX_TMP_SIZE    4096



void convert_grx_bitmap(int width, int height, GRX_BITMAP src, GRX_BITMAP dest) 
{
   unsigned short x, y, bytes_per_line;
   unsigned char bitpos, bitset;

   bytes_per_line = (width+7) >> 3;

   for (y=0; y<height; y++) {
      for (x=0; x<width; x++) {
	 bitpos = 7-(x&7);
	 bitset = !!(src[(bytes_per_line*y) + (x>>3)] & (1<<bitpos));
	 dest[y*width+x] = bitset;
      }
   }
}



GRX_BITMAP *load_grx_bmps(PACKFILE *f, FNTfile_header *hdr, int numchar, unsigned short *wtable) 
{
   int t, width, bmp_size;
   GRX_BITMAP temp;
   GRX_BITMAP *bmp;

   /* alloc array of bitmap pointers */
   bmp = malloc(sizeof(GRX_BITMAP) * numchar);

   /* assume it's fixed width for now */
   width = hdr->width;

   /* temporary working area to store FNT bitmap */
   temp = malloc(GRX_TMP_SIZE);

   for (t=0; t<numchar; t++) {
      /* if prop. get character width */
      if (!hdr->isfixed) 
	 width = wtable[t];

      /* work out how many bytes to read */
      bmp_size = ((width+7) >> 3) * hdr->height;

      /* oops, out of space! */
      if (bmp_size > GRX_TMP_SIZE) {
	 free(temp);
	 for (t--; t>=0; t--)
	    free(bmp[t]);
	 free(bmp);
	 return NULL;
      }

      /* alloc space for converted bitmap */
      bmp[t] = malloc(width*hdr->height);

      /* read data */
      pack_fread(temp, bmp_size, f);

      /* convert to 1 byte/pixel */
      convert_grx_bitmap(width, hdr->height, temp, bmp[t]);
   }

   free(temp);
   return bmp;
}



FONT *import_grx_font(char *fname)
{
   PACKFILE *f, *cf;
   FNTfile_header hdr;              /* GRX font header */
   int numchar;                     /* number of characters in the font */
   unsigned short *wtable = NULL;   /* table of widths for each character */
   GRX_BITMAP *bmp;                 /* array of font bitmaps */
   FONT *font = NULL;               /* the Allegro font */
   FONT_PROP *font_prop;
   int c, c2, start, width;
   char copyright[256];

   f = pack_fopen(fname, F_READ);
   if (!f)
      return NULL;

   pack_fread(&hdr, sizeof(hdr), f);      /* read the header structure */

   if (hdr.magic != FONTMAGIC) {          /* check magic number */
      pack_fclose(f);
      return NULL;
   }

   numchar = hdr.maxchar-hdr.minchar+1;

   if (!hdr.isfixed) {                    /* proportional font */
      wtable = malloc(sizeof(unsigned short) * numchar);
      pack_fread(wtable, sizeof(unsigned short) * numchar, f);
   }

   bmp = load_grx_bmps(f, &hdr, numchar, wtable);
   if (!bmp)
      goto get_out;

   if (pack_ferror(f))
      goto get_out;

   if ((hdr.minchar < ' ') || (hdr.maxchar >= ' '+FONT_SIZE))
      datedit_msg("Warning: font exceeds range 32..256. Characters will be lost in conversion");

   font = malloc(sizeof(FONT));
   font->height = -1;
   font->dat.dat_prop = font_prop = malloc(sizeof(FONT_PROP));

   start = 32 - hdr.minchar;
   width = hdr.width;

   for (c=0; c<FONT_SIZE; c++) {
      c2 = c+start;

      if ((c2 >= 0) && (c2 < numchar)) {
	 if (!hdr.isfixed)
	    width = wtable[c2];

	 font_prop->dat[c] = create_bitmap_ex(8, width, hdr.height);
	 memcpy(font_prop->dat[c]->dat, bmp[c2], width*hdr.height);
      }
      else {
	 font_prop->dat[c] = create_bitmap_ex(8, 8, hdr.height);
	 clear(font_prop->dat[c]);
      }
   }

   if (!pack_feof(f)) {
      strcpy(copyright, fname);
      strcpy(get_extension(copyright), "txt");
      c = datedit_ask("Save font copyright message into '%s'", copyright);
      if ((c != 27) && (c != 'n') && (c != 'N')) {
	 cf = pack_fopen(copyright, F_WRITE);
	 if (cf) {
	    while (!pack_feof(f)) {
	       pack_fgets(copyright, 255, f);
	       if (isspace(copyright[0])) {
		  pack_fputs(copyright, cf);
		  pack_fputs("\n", cf);
	       }
	       else if (!copyright[0])
		  pack_fputs("\n", cf);
	    }
	    pack_fclose(cf);
	 }
      }
   }

   get_out:

   pack_fclose(f);

   if (wtable)
      free(wtable);

   if (bmp) {
      for (c=0; c<numchar; c++)
	 free(bmp[c]);
      free(bmp);
   }

   return font;
}



FONT *import_bios_font(char *fname)
{
   PACKFILE *f;
   FONT *font = NULL; 
   unsigned char data[256][16];
   int c, x, y;

   f = pack_fopen(fname, F_READ);
   if (!f)
      return NULL;

   pack_fread(data, sizeof(data), f); 
   pack_fclose(f);

   font = malloc(sizeof(FONT));
   font->height = -1;
   font->dat.dat_prop = malloc(sizeof(FONT_PROP));

   for (c=0; c<FONT_SIZE; c++) {
      font->dat.dat_prop->dat[c] = create_bitmap_ex(8, 8, 16);
      clear(font->dat.dat_prop->dat[c]);

      for (y=0; y<16; y++)
	 for (x=0; x<8; x++)
	    if (data[c + ' '][y] & (0x80 >> x))
	       font->dat.dat_prop->dat[c]->line[y][x] = 1;
   }

   return font;
}



void *import_bitmap_font(char *fname)
{
   PALLETE junk;
   BITMAP *bmp;
   FONT *f;
   int x, y, w, h, c;
   int max_h = 0;

   bmp = load_bitmap(fname, junk);
   if (!bmp)
      return NULL;

   if (bitmap_color_depth(bmp) != 8) {
      destroy_bitmap(bmp);
      return NULL;
   }

   f = malloc(sizeof(FONT));
   f->height = -1;
   f->dat.dat_prop = malloc(sizeof(FONT_PROP));
   for (c=0; c<FONT_SIZE; c++)
      f->dat.dat_prop->dat[c] = NULL;

   x = 0;
   y = 0;

   for (c=0; c<FONT_SIZE; c++) {
      datedit_find_character(bmp, &x, &y, &w, &h);

      if ((w <= 0) || (h <= 0)) {
	 w = 8;
	 h = 8;
      }

      f->dat.dat_prop->dat[c] = create_bitmap_ex(8, w, h);
      clear(f->dat.dat_prop->dat[c]);
      blit(bmp, f->dat.dat_prop->dat[c], x+1, y+1, 0, 0, w, h);

      max_h = MAX(max_h, h);
      x += w;
   }

   for (c=0; c<FONT_SIZE; c++) {
      if (f->dat.dat_prop->dat[c]->h < max_h) {
	 BITMAP *b = f->dat.dat_prop->dat[c];
	 f->dat.dat_prop->dat[c] = create_bitmap_ex(8, b->w, max_h);
	 clear(f->dat.dat_prop->dat[c]);
	 blit(b, f->dat.dat_prop->dat[c], 0, 0, 0, 0, b->w, b->h);
	 destroy_bitmap(b);
      }
   }

   destroy_bitmap(bmp);
   return f;
}



FONT *make_font8x8(FONT *f)
{
   FONT_PROP *fp = f->dat.dat_prop;
   FONT_8x8 *f8 = malloc(sizeof(FONT_8x8));
   BITMAP *bmp;
   int c, x, y;

   for (c=0; c<FONT_SIZE; c++) {
      bmp = fp->dat[c];

      for (y=0; y<8; y++) {
	 f8->dat[c][y] = 0;
	 for (x=0; x<8; x++)
	    if (bmp->line[y][x])
	       f8->dat[c][y] |= (0x80 >> x);
      }

      destroy_bitmap(bmp); 
   }

   free(fp);
   f->dat.dat_8x8 = f8;
   f->height = 8;
   return f;
}



FONT *make_font8x16(FONT *f)
{
   FONT_PROP *fp = f->dat.dat_prop;
   FONT_8x16 *f16 = malloc(sizeof(FONT_8x16));
   BITMAP *bmp;
   int c, x, y;

   for (c=0; c<FONT_SIZE; c++) {
      bmp = fp->dat[c];

      for (y=0; y<16; y++) {
	 f16->dat[c][y] = 0;
	 for (x=0; x<8; x++)
	    if (bmp->line[y][x])
	       f16->dat[c][y] |= (0x80 >> x);
      }

      destroy_bitmap(bmp); 
   }

   free(fp);
   f->dat.dat_8x16 = f16;
   f->height = 16;
   return f;
}



FONT *fixup_font(FONT *f)
{
   int c, w, h, x, y, n;
   int col = -1;

   if (!f)
      return NULL;

   w = f->dat.dat_prop->dat[0]->w;
   h = f->dat.dat_prop->dat[0]->h;

   for (c=1; c<FONT_SIZE; c++) {
      if ((f->dat.dat_prop->dat[c]->w != w) ||
	  (f->dat.dat_prop->dat[c]->h != h))
	 return f;

      for (y=0; y<h; y++) {
	 for (x=0; x<w; x++) {
	    n = f->dat.dat_prop->dat[c]->line[y][x];
	    if (n) {
	       if (col < 0)
		  col = n;
	       else if (col != n)
		  return f;
	    }
	 }
      }
   }

   if ((w == 8) && (h == 8))
      return make_font8x8(f);
   else if ((w == 8) && (h == 16))
      return make_font8x16(f);
   else
      return f;
}



void *grab_font(char *filename, GRAB_UNUSED)
{
   PACKFILE *f;
   int id;

   if ((stricmp(get_extension(filename), "bmp") == 0) ||
       (stricmp(get_extension(filename), "lbm") == 0) ||
       (stricmp(get_extension(filename), "pcx") == 0) ||
       (stricmp(get_extension(filename), "tga") == 0)) {
      return fixup_font(import_bitmap_font(filename));
   }
   else {
      f = pack_fopen(filename, F_READ);
      if (!f)
	 return NULL;

      id = pack_igetl(f);
      pack_fclose(f);

      if (id == FONTMAGIC)
	 return fixup_font(import_grx_font(filename));
      else
	 return fixup_font(import_bios_font(filename));
   }
}



void save_font(DATAFILE *dat, SAVE_UNUSED, PACKFILE *f)
{
   FONT *font = (FONT *)dat->dat;
   BITMAP *bmp;
   int c, x, y;

   pack_mputw(font->height, f);

   if (font->height > 0) {
      if (font->height == 8)
	 pack_fwrite(font->dat.dat_8x8, sizeof(FONT_8x8), f);
      else
	 pack_fwrite(font->dat.dat_8x16, sizeof(FONT_8x16), f);
   }
   else {
      for (c=0; c<FONT_SIZE; c++) {
	 bmp = font->dat.dat_prop->dat[c];

	 pack_mputw(bmp->w, f);
	 pack_mputw(bmp->h, f);

	 for (y=0; y<bmp->h; y++)
	    for (x=0; x<bmp->w; x++)
	       pack_putc(bmp->line[y][x], f);
      }
   }
}



/* functions for handling sample objects:
 */

void get_sample_desc(DATAFILE *dat, char *s)
{
   SAMPLE *sample = (SAMPLE *)dat->dat;
   long sec = (sample->len + sample->freq/2) * 10 / MAX(sample->freq, 1);

   sprintf(s, "sample (%d bit, %d hz, %ld.%ld sec)", sample->bits, sample->freq, sec/10, sec%10);
}



int export_sample(DATAFILE *dat, char *filename)
{
   SAMPLE *spl = (SAMPLE *)dat->dat;
   int len = spl->len * spl->bits / 8;
   int i;
   signed short s;
   PACKFILE *f;

   f = pack_fopen(filename, F_WRITE);

   if (f) {
      pack_fputs("RIFF", f);                 /* RIFF header */
      pack_iputl(36+len, f);                 /* size of RIFF chunk */
      pack_fputs("WAVE", f);                 /* WAV definition */
      pack_fputs("fmt ", f);                 /* format chunk */
      pack_iputl(16, f);                     /* size of format chunk */
      pack_iputw(1, f);                      /* PCM data */
      pack_iputw(1, f);                      /* mono data */
      pack_iputl(spl->freq, f);              /* sample frequency */
      pack_iputl(spl->freq, f);              /* avg. bytes per sec */
      pack_iputw(1, f);                      /* block alignment */
      pack_iputw(spl->bits, f);              /* bits per sample */
      pack_fputs("data", f);                 /* data chunk */
      pack_iputl(len, f);                    /* actual data length */

      if (spl->bits == 8) {
	 pack_fwrite(spl->data, len, f);     /* write the data */
      }
      else {
	 for (i=0; i<(int)spl->len; i++) {
	    s = ((signed short *)spl->data)[i];
	    pack_iputw(s^0x8000, f);
	 }
      }

      pack_fclose(f);
   }

   return (errno == 0);
}



void *grab_sample(char *filename, GRAB_UNUSED)
{
   return load_sample(filename);
}



void save_sample(DATAFILE *dat, SAVE_UNUSED, PACKFILE *f)
{
   SAMPLE *spl = (SAMPLE *)dat->dat;

   pack_mputw(spl->bits, f);
   pack_mputw(spl->freq, f);
   pack_mputl(spl->len, f);
   pack_fwrite(spl->data, spl->len*spl->bits/8, f);
}



/* functions for handling MIDI file objects:
 */

int export_midi(DATAFILE *dat, char *filename)
{
   MIDI *midi = (MIDI *)dat->dat;
   PACKFILE *f;
   int num_tracks;
   int c;

   num_tracks = 0;
   for (c=0; c<MIDI_TRACKS; c++)
      if (midi->track[c].len > 0)
	 num_tracks++;

   f = pack_fopen(filename, F_WRITE);

   if (f) {
      pack_fputs("MThd", f);                 /* MIDI header */
      pack_mputl(6, f);                      /* size of header chunk */
      pack_mputw(1, f);                      /* type 1 */
      pack_mputw(num_tracks, f);             /* number of tracks */
      pack_mputw(midi->divisions, f);        /* beat divisions */

      for (c=0; c<MIDI_TRACKS; c++) {        /* for each track */
	 if (midi->track[c].len > 0) {
	    pack_fputs("MTrk", f);           /* write track data */
	    pack_mputl(midi->track[c].len, f); 
	    pack_fwrite(midi->track[c].data, midi->track[c].len, f);
	 }
      }

      pack_fclose(f);
   }

   return (errno == 0);
}



void *grab_midi(char *filename, GRAB_UNUSED)
{
   return load_midi(filename);
}



void save_midi(DATAFILE *dat, SAVE_UNUSED, PACKFILE *f)
{
   MIDI *midi = (MIDI *)dat->dat;
   int c;

   pack_mputw(midi->divisions, f);

   for (c=0; c<MIDI_TRACKS; c++) {
      pack_mputl(midi->track[c].len, f);
      if (midi->track[c].len > 0)
	 pack_fwrite(midi->track[c].data, midi->track[c].len, f);
   }
}



/* functions for handling patch objects:
 */

void get_patch_desc(DATAFILE *dat, char *s)
{
   sprintf(s, "MIDI instrument (%ld bytes)", dat->size);
}



/* functions for handling bitmap objects:
 */

void get_bitmap_desc(DATAFILE *dat, char *s)
{
   BITMAP *bmp = (BITMAP *)dat->dat;

   sprintf(s, "bitmap (%dx%d, %d bit)", bmp->w, bmp->h, bitmap_color_depth(bmp));
}



int export_bitmap(DATAFILE *dat, char *filename)
{
   return (save_bitmap(filename, (BITMAP *)dat->dat, current_pallete) == 0);
}



void *grab_bitmap(char *filename, long *size, int x, int y, int w, int h, int depth)
{
   BITMAP *bmp;

   if (depth > 0) {
      int oldcolordepth = _color_depth;

      _color_depth = depth;
      set_color_conversion(COLORCONV_TOTAL);

      bmp = load_bitmap(filename, last_read_pal);

      _color_depth = oldcolordepth;
      set_color_conversion(COLORCONV_NONE);
   }
   else
      bmp = load_bitmap(filename, last_read_pal);

   if (!bmp)
      return NULL;

   if ((x >= 0) && (y >= 0) && (w >= 0) && (h >= 0)) {
      BITMAP *b2 = create_bitmap_ex(bitmap_color_depth(bmp), w, h);
      clear_to_color(b2, bitmap_mask_color(b2));
      blit(bmp, b2, x, y, 0, 0, w, h);
      destroy_bitmap(bmp);
      return b2;
   }
   else 
      return bmp;
}



void save_datafile_bitmap(DATAFILE *dat, SAVE_UNUSED, PACKFILE *f)
{
   BITMAP *bmp = (BITMAP *)dat->dat;
   int x, y, c, r, g, b;
   unsigned short *p16;
   unsigned long *p32;
   int depth = bitmap_color_depth(bmp);

   pack_mputw(depth, f);
   pack_mputw(bmp->w, f);
   pack_mputw(bmp->h, f);

   switch (depth) {

      case 8:
	 /* 256 colors */
	 for (y=0; y<bmp->h; y++)
	    for (x=0; x<bmp->w; x++)
	       pack_putc(bmp->line[y][x], f);
	 break;

      case 15:
      case 16:
	 /* hicolor */
	 for (y=0; y<bmp->h; y++) {
	    p16 = (unsigned short *)bmp->line[y];

	    for (x=0; x<bmp->w; x++) {
	       c = p16[x];
	       r = getr_depth(depth, c);
	       g = getg_depth(depth, c);
	       b = getb_depth(depth, c);
	       c = ((r<<8)&0xF800) | ((g<<3)&0x7E0) | ((b>>3)&0x1F);
	       pack_iputw(c, f);
	    }
	 }
	 break;

      case 24:
	 /* 24 bit truecolor */
	 for (y=0; y<bmp->h; y++) {
	    for (x=0; x<bmp->w; x++) {
	       r = bmp->line[y][x*3+_rgb_r_shift_24/8];
	       g = bmp->line[y][x*3+_rgb_g_shift_24/8];
	       b = bmp->line[y][x*3+_rgb_b_shift_24/8];
	       pack_putc(r, f);
	       pack_putc(g, f);
	       pack_putc(b, f);
	    }
	 }
	 break;

      case 32:
	 /* 32 bit truecolor */
	 for (y=0; y<bmp->h; y++) {
	    p32 = (unsigned long *)bmp->line[y];

	    for (x=0; x<bmp->w; x++) {
	       c = p32[x];
	       r = getr32(c);
	       g = getg32(c);
	       b = getb32(c);
	       pack_putc(r, f);
	       pack_putc(g, f);
	       pack_putc(b, f);
	    }
	 }
	 break;
   }
}



/* functions for handling RLE sprite objects:
 */

void get_rle_sprite_desc(DATAFILE *dat, char *s)
{
   RLE_SPRITE *spr = (RLE_SPRITE *)dat->dat;

   sprintf(s, "RLE sprite (%dx%d, %d bit)", spr->w, spr->h, spr->color_depth);
}



int export_rle_sprite(DATAFILE *dat, char *filename)
{
   BITMAP *bmp;
   RLE_SPRITE *rle = (RLE_SPRITE *)dat->dat;
   int ret;

   bmp = create_bitmap_ex(rle->color_depth, rle->w, rle->h);
   clear_to_color(bmp, bmp->vtable->mask_color);
   draw_rle_sprite(bmp, rle, 0, 0);

   ret = (save_bitmap(filename, bmp, current_pallete) == 0);

   destroy_bitmap(bmp);
   return ret;
}



void *grab_rle_sprite(char *filename, long *size, int x, int y, int w, int h, int depth)
{
   BITMAP *bmp;
   RLE_SPRITE *spr;

   bmp = (BITMAP *)grab_bitmap(filename, size, x, y, w, h, depth);
   if (!bmp)
      return NULL;

   spr = get_rle_sprite(bmp);
   destroy_bitmap(bmp);

   return spr;
}



void save_rle_sprite(DATAFILE *dat, SAVE_UNUSED, PACKFILE *f)
{
   RLE_SPRITE *spr = (RLE_SPRITE *)dat->dat;
   int x, y, c, r, g, b;
   signed short *p16;
   signed long *p32;
   unsigned int eol_marker;
   int depth = spr->color_depth;

   pack_mputw(depth, f);
   pack_mputw(spr->w, f);
   pack_mputw(spr->h, f);
   pack_mputl(spr->size, f);

   switch (depth) {

      case 8:
	 /* 256 colors, easy! */
	 pack_fwrite(spr->dat, spr->size, f);
	 break;

      case 15:
      case 16:
	 /* hicolor */
	 p16 = (signed short *)spr->dat;
	 eol_marker = (depth == 15) ? MASK_COLOR_15 : MASK_COLOR_16;

	 for (y=0; y<spr->h; y++) {
	    while ((unsigned short)*p16 != eol_marker) {
	       if (*p16 < 0) {
		  /* skip count */
		  pack_iputw(*p16, f);
		  p16++;
	       }
	       else {
		  /* solid run */
		  x = *p16;
		  p16++;

		  pack_iputw(x, f);

		  while (x-- > 0) {
		     c = (unsigned short)*p16;
		     r = getr_depth(depth, c);
		     g = getg_depth(depth, c);
		     b = getb_depth(depth, c);
		     c = ((r<<8)&0xF800) | ((g<<3)&0x7E0) | ((b>>3)&0x1F);
		     pack_iputw(c, f);
		     p16++;
		  }
	       }
	    }

	    /* end of line */
	    pack_iputw(MASK_COLOR_16, f);
	    p16++;
	 }
	 break;

      case 24:
      case 32:
	 /* truecolor */
	 p32 = (signed long *)spr->dat;
	 eol_marker = (depth == 24) ? MASK_COLOR_24 : MASK_COLOR_32;

	 for (y=0; y<spr->h; y++) {
	    while ((unsigned long)*p32 != eol_marker) {
	       if (*p32 < 0) {
		  /* skip count */
		  pack_iputl(*p32, f);
		  p32++;
	       }
	       else {
		  /* solid run */
		  x = *p32;
		  p32++;

		  pack_iputl(x, f);

		  while (x-- > 0) {
		     c = (unsigned long)*p32;
		     r = getr_depth(depth, c);
		     g = getg_depth(depth, c);
		     b = getb_depth(depth, c);
		     pack_putc(r, f);
		     pack_putc(g, f);
		     pack_putc(b, f);
		     p32++;
		  }
	       }
	    }

	    /* end of line */
	    pack_iputl(MASK_COLOR_32, f);
	    p32++;
	 }
	 break;
   }
}



/* functions for handling compiled sprite objects:
 */

void get_c_sprite_desc(DATAFILE *dat, char *s)
{
   BITMAP *bmp = (BITMAP *)dat->dat;

   sprintf(s, "compiled sprite (%dx%d, %d bit)", bmp->w, bmp->h, bitmap_color_depth(bmp));
}



/* functions for handling mode-X compiled sprite objects:
 */

void get_xc_sprite_desc(DATAFILE *dat, char *s)
{
   BITMAP *bmp = (BITMAP *)dat->dat;

   if (bitmap_color_depth(bmp) == 8)
      sprintf(s, "mode-X compiled sprite (%dx%d)", bmp->w, bmp->h);
   else
      sprintf(s, "!!! %d bit XC sprite not possible !!!", bitmap_color_depth(bmp));
}



/* functions for handling pallete objects:
 */

int export_pallete(DATAFILE *dat, char *filename)
{
   BITMAP *bmp;
   int ret;

   bmp = create_bitmap_ex(8, 32, 8);
   clear(bmp);
   text_mode(0);
   textout(bmp, font, "PAL.", 0, 0, 255);

   ret = (save_bitmap(filename, bmp, (RGB *)dat->dat) == 0);

   destroy_bitmap(bmp);
   return ret;
}



void *grab_pallete(char *filename, GRAB_UNUSED)
{
   int oldcolordepth = _color_depth;
   RGB *pal;
   BITMAP *bmp;

   _color_depth = 8;
   set_color_conversion(COLORCONV_TOTAL);

   pal = malloc(sizeof(PALLETE));
   bmp = load_bitmap(filename, pal);

   _color_depth = oldcolordepth;
   set_color_conversion(COLORCONV_NONE);

   if (!bmp) {
      free(pal);
      return NULL;
   }

   destroy_bitmap(bmp);

   *size = sizeof(PALLETE);
   return pal;
}



OBJECT_INFO object_info[] =
{
   {  DAT_FILE, 
      "datafile", 
      "dat", "dat",
      NULL, 
      export_datafile,
      grab_datafile,
      save_datafile
   },

   {  DAT_SAMPLE, 
      "sample", 
      "voc;wav", "wav",
      get_sample_desc,
      export_sample,
      grab_sample,
      save_sample
   },

   {  DAT_MIDI, 
      "MIDI file", 
      "mid", "mid",
      NULL,
      export_midi,
      grab_midi,
      save_midi
   },

   {  DAT_PATCH,
      "GUS patch",
      "pat", "pat",
      get_patch_desc,
      NULL,
      NULL,
      NULL
   },

   {  DAT_FLI, 
      "FLI/FLC animation", 
      "fli;flc", "flc",
      NULL,
      NULL,
      NULL
   },

   {  DAT_BITMAP, 
      "bitmap", 
      "bmp;lbm;pcx;tga", "bmp;pcx;tga",
      get_bitmap_desc,
      export_bitmap,
      grab_bitmap,
      save_datafile_bitmap
   },

   {  DAT_RLE_SPRITE, 
      "RLE sprite", 
      "bmp;lbm;pcx;tga", "bmp;pcx;tga",
      get_rle_sprite_desc,
      export_rle_sprite,
      grab_rle_sprite,
      save_rle_sprite
   },

   {  DAT_C_SPRITE, 
      "compiled sprite", 
      "bmp;lbm;pcx;tga", "bmp;pcx;tga",
      get_c_sprite_desc,
      export_bitmap,
      grab_bitmap,
      save_datafile_bitmap
   },

   {  DAT_XC_SPRITE, 
      "mode-X compiled sprite", 
      "bmp;lbm;pcx;tga", "bmp;pcx;tga",
      get_xc_sprite_desc,
      export_bitmap,
      grab_bitmap,
      save_datafile_bitmap
   },

   {  DAT_PALLETE, 
      "pallete", 
      "bmp;lbm;pcx;tga", "bmp;pcx;tga",
      NULL,
      export_pallete,
      grab_pallete,
      NULL
   },

   {  DAT_FONT, 
      "font", 
      "fnt;bmp;lbm;pcx;tga", "bmp;pcx;tga",
      get_font_desc,
      export_font,
      grab_font,
      save_font
   },

   {  DAT_END }
};



/* adds extensions to filenames if they are missing, or changes them */
char *datedit_pretty_name(char *name, char *ext, int force_ext)
{
   static char buf[256];
   char *s;

   strcpy(buf, name);

   s = get_extension(buf);
   if ((s > buf) && (*(s-1)=='.')) {
      if (force_ext)
	 strcpy(s, ext);
   }
   else {
      *s = '.';
      strcpy(s+1, ext);
   }

   strlwr(buf);
   return buf;
}



/* returns a description string for an object */
char *datedit_desc(DATAFILE *dat)
{
   static char buf[256];

   OBJECT_INFO *info = object_info;

   while (info->type != DAT_END) {
      if (info->type == dat->type) {
	 if (info->get_desc)
	    info->get_desc(dat, buf);
	 else
	    strcpy(buf, info->desc);

	 return buf;
      }

      info++;
   }

   sprintf(buf, "binary data (%ld bytes)", dat->size);
   return buf;
}



/* qsort callback for comparing datafile objects */
int dat_cmp(const void *e1, const void *e2)
{
   DATAFILE *d1 = (DATAFILE *)e1;
   DATAFILE *d2 = (DATAFILE *)e2;

   return stricmp(get_datafile_property(d1, DAT_NAME),
		  get_datafile_property(d2, DAT_NAME));
}



/* sorts a datafile */
void datedit_sort_datafile(DATAFILE *dat)
{
   int len;

   if (dat) {
      len = 0;
      while (dat[len].type != DAT_END)
	 len++;

      if (len > 1)
	 qsort(dat, len, sizeof(DATAFILE), dat_cmp);
   }
}



/* qsort callback for comparing datafile properties */
int prop_cmp(const void *e1, const void *e2)
{
   DATAFILE_PROPERTY *p1 = (DATAFILE_PROPERTY *)e1;
   DATAFILE_PROPERTY *p2 = (DATAFILE_PROPERTY *)e2;

   return p1->type - p2->type;
}



/* sorts a list of object properties */
void datedit_sort_properties(DATAFILE_PROPERTY *prop)
{
   int len;

   if (prop) {
      len = 0;
      while (prop[len].type != DAT_END)
	 len++;

      if (len > 1)
	 qsort(prop, len, sizeof(DATAFILE_PROPERTY), prop_cmp);
   }
}



/* splits bitmaps into sub-sprites, using regions bounded by col #255 */
void datedit_find_character(BITMAP *bmp, int *x, int *y, int *w, int *h)
{
   int c1;
   int c2;

   if (bitmap_color_depth(bmp) == 8) {
      c1 = 255;
      c2 = 255;
   }
   else {
      c1 = makecol_depth(bitmap_color_depth(bmp), 255, 255, 0);
      c2 = makecol_depth(bitmap_color_depth(bmp), 0, 255, 255);
   }

   /* look for top left corner of character */
   while ((getpixel(bmp, *x, *y) != c1) || 
	  (getpixel(bmp, *x+1, *y) != c2) ||
	  (getpixel(bmp, *x, *y+1) != c2) ||
	  (getpixel(bmp, *x+1, *y+1) == c1) ||
	  (getpixel(bmp, *x+1, *y+1) == c2)) {
      (*x)++;
      if (*x >= bmp->w) {
	 *x = 0;
	 (*y)++;
	 if (*y >= bmp->h) {
	    *w = 0;
	    *h = 0;
	    return;
	 }
      }
   }

   /* look for right edge of character */
   *w = 0;
   while ((getpixel(bmp, *x+*w+1, *y) == c2) &&
	  (getpixel(bmp, *x+*w+1, *y+1) != c2) &&
	  (*x+*w+1 <= bmp->w))
      (*w)++;

   /* look for bottom edge of character */
   *h = 0;
   while ((getpixel(bmp, *x, *y+*h+1) == c2) &&
	  (getpixel(bmp, *x+1, *y+*h+1) != c2) &&
	  (*y+*h+1 <= bmp->h))
      (*h)++;
}



/* cleans up an object type string, and packs it */
int datedit_clean_typename(char *type)
{
   int c1, c2, c3, c4;

   if (!type)
      return 0;

   strupr(type);

   c1 = (*type) ? *(type++) : ' ';
   c2 = (*type) ? *(type++) : ' ';
   c3 = (*type) ? *(type++) : ' ';
   c4 = (*type) ? *(type++) : ' ';

   return DAT_ID(c1, c2, c3, c4);
}



/* sets an object property string */
void datedit_set_property(DATAFILE *dat, int type, char *value)
{
   int i, size, pos;

   if (dat->prop) {
      pos = -1;
      for (size=0; dat->prop[size].type != DAT_END; size++)
	 if (dat->prop[size].type == type)
	    pos = size;

      if ((value) && (strlen(value) > 0)) {
	 if (pos >= 0) {
	    dat->prop[pos].dat = realloc(dat->prop[pos].dat, strlen(value)+1);
	    strcpy(dat->prop[pos].dat, value);
	 }
	 else {
	    dat->prop = realloc(dat->prop, sizeof(DATAFILE_PROPERTY)*(size+2));
	    dat->prop[size+1] = dat->prop[size];
	    dat->prop[size].type = type;
	    dat->prop[size].dat = malloc(strlen(value)+1);
	    strcpy(dat->prop[size].dat, value);
	 }
      }
      else {
	 if (pos >= 0) {
	    free(dat->prop[pos].dat);
	    for (i=pos; i<size; i++)
	       dat->prop[i] = dat->prop[i+1];
	    dat->prop = realloc(dat->prop, sizeof(DATAFILE_PROPERTY)*size);
	 }
      }
   }
   else {
      if ((value) && (strlen(value) > 0)) {
	 dat->prop = malloc(sizeof(DATAFILE_PROPERTY) * 2);
	 dat->prop[0].type = type;
	 dat->prop[0].dat = malloc(strlen(value)+1);
	 strcpy(dat->prop[0].dat, value);
	 dat->prop[1].type = DAT_END;
	 dat->prop[1].dat = NULL;
      }
   }
}



/* loads a datafile */
DATAFILE *datedit_load_datafile(char *name, int compile_sprites, char *password)
{
   char *pretty_name;
   DATAFILE *datafile;

   extern int _compile_sprites;
   _compile_sprites = compile_sprites;

   if (!compile_sprites) {
      typedef void (*DF)(void *);
      register_datafile_object(DAT_C_SPRITE, NULL, (DF)destroy_bitmap);
      register_datafile_object(DAT_XC_SPRITE, NULL, (DF)destroy_bitmap);
   }

   if (name)
      pretty_name = datedit_pretty_name(name, "dat", FALSE);
   else
      pretty_name = NULL;

   if ((pretty_name) && (exists(pretty_name))) {
      datedit_msg("Reading %s", pretty_name);

      packfile_password(password);

      datafile = load_datafile(pretty_name);

      packfile_password(NULL);

      if (!datafile) {
	 datedit_error("Error reading %s", pretty_name);
	 return NULL;
      }
      else {
	 load_header(datafile, datedit_pretty_name(name, "h", TRUE));
	 generate_names(datafile, 0);
      }
   }
   else {
      if (pretty_name)
	 datedit_msg("%s not found: creating new datafile", pretty_name);

      datafile = malloc(sizeof(DATAFILE));
      datafile->dat = NULL;
      datafile->type = DAT_END;
      datafile->size = 0;
      datafile->prop = NULL;
   }

   datafile = extract_info(datafile, TRUE);

   return datafile;
}



/* works out what name to give an exported object */
void datedit_export_name(DATAFILE *dat, char *name, char *ext, char *buf)
{
   char *obname = get_datafile_property(dat, DAT_NAME);
   char *oborig = get_datafile_property(dat, DAT_ORIG);
   char tmp[32];
   int i;

   if (name)
      strcpy(buf, name);
   else
      strcpy(buf, oborig);

   if (*get_filename(buf) == 0) {
      if (*oborig) {
	 strcat(buf, get_filename(oborig));
      }
      else {
	 strcat(buf, obname);
	 get_filename(buf)[8] = 0;
      }
   }

   if (ext) {
      strcpy(tmp, ext);
      for (i=0; tmp[i]; i++) {
	 if (tmp[i] == ';') {
	    tmp[i] = 0;
	    break;
	 }
      }
      strcpy(buf, datedit_pretty_name(buf, tmp, 
			      ((name == NULL) || (!*get_extension(name)))));
   }
   else
      strlwr(buf);
}



/* exports a datafile object */
int datedit_export(DATAFILE *dat, char *name)
{
   char *obname = get_datafile_property(dat, DAT_NAME);
   OBJECT_INFO *info = object_info;
   int (*export)(DATAFILE *dat, char *filename) = NULL;
   char *ext = NULL;
   char buf[256];
   int i;

   while (info->type != DAT_END) {
      if (info->type == dat->type) {
	 export = info->export;
	 ext = info->exp_ext;
	 break;
      }
      info++;
   }

   datedit_export_name(dat, name, ext, buf);

   if (exists(buf)) {
      i = datedit_ask("%s already exists, overwrite", buf);
      if (i == 27)
	 return FALSE;
      else if ((i == 'n') || (i == 'N'))
	 return TRUE;
   }

   datedit_msg("Exporting %s -> %s", obname, buf);

   if (!export)
      export = export_binary;

   if (!export(dat, buf)) {
      delete_file(buf);
      datedit_error("Error writing %s", buf);
      return FALSE;
   }

   return TRUE;
}



/* deletes a datafile object */
DATAFILE *datedit_delete(DATAFILE *dat, int i)
{
   _unload_datafile_object(dat+i);

   do {
      dat[i] = dat[i+1];
      i++;
   } while (dat[i].type != DAT_END);

   return realloc(dat, sizeof(DATAFILE)*i);
}



/* fixup function for the strip options */
int datedit_striptype(int strip)
{
   if (strip >= 0)
      return strip;
   else
      return 0;
}



/* fixup function for the pack options */
int datedit_packtype(int pack)
{
   if (pack >= 0) {
      char buf[80];

      sprintf(buf, "%d", pack);
      datedit_set_property(&info, DAT_PACK, buf);

      return pack;
   }
   else {
      char *p = get_datafile_property(&info, DAT_PACK);

      if ((p) && (*p))
	 return atoi(p);
      else
	 return 0;
   }
}



/* saves a datafile */
int datedit_save_datafile(DATAFILE *dat, char *name, int strip, int pack, int verbose, int write_msg, int backup, char *password)
{
   char *pretty_name;
   char backup_name[256];
   PACKFILE *f;

   packfile_password(password);

   strip = datedit_striptype(strip);
   pack = datedit_packtype(pack);

   strcpy(backup_name, datedit_pretty_name(name, "bak", TRUE));
   pretty_name = datedit_pretty_name(name, "dat", FALSE);

   if (write_msg)
      datedit_msg("Writing %s", pretty_name);

   delete_file(backup_name);
   rename(pretty_name, backup_name);

   f = pack_fopen(pretty_name, (pack >= 2) ? F_WRITE_PACKED : F_WRITE_NOPACK);

   if (f) {
      pack_mputl(DAT_MAGIC, f);
      file_datasize = 12;

      save_datafile(dat, (pack >= 2), (pack >= 1), strip, verbose, (strip <= 0), f);

      if (strip <= 0) {
	 datedit_set_property(&info, DAT_NAME, "GrabberInfo");
	 save_object(&info, FALSE, FALSE, FALSE, FALSE, f);
      }

      pack_fclose(f); 
   }

   if (errno) {
      delete_file(pretty_name);
      datedit_error("Error writing %s", pretty_name);
      packfile_password(NULL);
      return FALSE;
   }
   else {
      if (!backup)
	 delete_file(backup_name);

      if (verbose) {
	 int file_filesize = file_size(pretty_name);
	 datedit_msg("%-28s%7d bytes into %-7d (%d%%)", "- GLOBAL COMPRESSION -",
		     file_datasize, file_filesize, percent(file_datasize, file_filesize));
      }
   }

   packfile_password(NULL);
   return TRUE;
}



/* writes object definitions into a header file */
void save_header(DATAFILE *dat, FILE *f, char *prefix)
{
   int c;

   fprintf(f, "\n");

   for (c=0; dat[c].type != DAT_END; c++) {
      fprintf(f, "#define %s%-*s %-8d /* %c%c%c%c */\n", 
	      prefix, 32-(int)strlen(prefix),
	      get_datafile_property(dat+c, DAT_NAME), c,
	      (dat[c].type>>24), (dat[c].type>>16)&0xFF, 
	      (dat[c].type>>8)&0xFF, (dat[c].type&0xFF));

      if (dat[c].type == DAT_FILE) {
	 char p[256];

	 strcpy(p, prefix);
	 strcat(p, get_datafile_property(dat+c, DAT_NAME));
	 strcat(p, "_");

	 save_header((DATAFILE *)dat[c].dat, f, p);
      }
   }

   if (*prefix)
      fprintf(f, "#define %s%-*s %d\n", prefix, 32-(int)strlen(prefix), "COUNT", c);

   fprintf(f, "\n");
}



/* helper for renaming files (works across drives) */
int rename_file(char *oldname, char *newname)
{
   PACKFILE *oldfile, *newfile;
   int c;

   oldfile = pack_fopen(oldname, F_READ);
   if (!oldfile)
      return -1;

   newfile = pack_fopen(newname, F_WRITE);
   if (!newfile) {
      pack_fclose(oldfile);
      return -1;
   }

   c = pack_getc(oldfile);

   while (c != EOF) {
      pack_putc(c, newfile);
      c = pack_getc(oldfile);
   } 

   pack_fclose(oldfile);
   pack_fclose(newfile);

   delete_file(oldname); 

   return 0;
}



/* checks whether the header needs updating */
int cond_update_header(char *tn, char *n, int verbose)
{
   PACKFILE *f1, *f2;
   char b1[256], b2[256];
   int i;
   int differ = FALSE;

   if (!exists(n)) {
      if (rename_file(tn, n) != 0)
	 return FALSE;
   }
   else {
      f1 = pack_fopen(tn, F_READ);
      if (!f1)
	 return FALSE;

      f2 = pack_fopen(n, F_READ);
      if (!f2) {
	 pack_fclose(f1);
	 return FALSE;
      }

      for (i=0; i<4; i++) {
	 /* skip date, which may differ */
	 pack_fgets(b1, 255, f1);
	 pack_fgets(b2, 255, f2);
      }

      while ((!pack_feof(f1)) && (!pack_feof(f2)) && (!differ)) {
	 pack_fgets(b1, 255, f1);
	 pack_fgets(b2, 255, f2);
	 if (strcmp(b1, b2) != 0)
	    differ = TRUE;
      }

      if ((!pack_feof(f1)) || (!pack_feof(f2)))
	 differ = TRUE;

      pack_fclose(f1);
      pack_fclose(f2);

      if (differ) {
	 if (verbose)
	    datedit_msg("%s has changed: updating", n);

	 delete_file(n);
	 rename_file(tn, n);
      }
      else {
	 if (verbose)
	    datedit_msg("%s has not changed: no update", n);

	 delete_file(tn);
      }
   }

   return TRUE;
}



/* exports a datafile header */
int datedit_save_header(DATAFILE *dat, char *name, char *headername, char *progname, char *prefix, int verbose)
{
   char *pretty_name, *tmp_name;
   char tm[80];
   char p[80];
   time_t now;
   FILE *f;
   int c;

   tmp_name = tmpnam(NULL);

   if (prefix)
      strcpy(p, prefix);
   else
      strcpy(p, get_datafile_property(&info, DAT_HPRE));

   if ((p[0]) && ((strlen(p)-1) != '_'))
      strcat(p, "_");

   pretty_name = datedit_pretty_name(headername, "h", FALSE);
   datedit_msg("Writing ID's into %s", pretty_name);

   f = fopen(tmp_name, "w");
   if (f) {
      time(&now);
      strcpy(tm, asctime(localtime(&now)));
      for (c=0; tm[c]; c++)
	 if ((tm[c] == '\r') || (tm[c] == '\n'))
	    tm[c] = 0;

      fprintf(f, "/* Allegro datafile object indexes, produced by %s v" ALLEGRO_VERSION_STR " */\n", progname);
      fprintf(f, "/* Datafile: %s */\n", name);
      fprintf(f, "/* Date: %s */\n", tm);
      fprintf(f, "/* Do not hand edit! */\n");

      save_header(dat, f, p);

      fclose(f);

      if (!cond_update_header(tmp_name, pretty_name, verbose)) {
	 datedit_error("Error writing %s", pretty_name);
	 return FALSE;
      }
   }
   else {
      datedit_error("Error writing %s", pretty_name);
      return FALSE;
   }

   return TRUE;
}



/* converts a file timestamp from ASCII to integer representation */
long datedit_asc2ftime(char *time)
{
   long r = 0;
   char *tok;
   static char *sep = "-,: ";
   char tmp[256];

   strcpy(tmp, time);
   tok = strtok(tmp, sep);

   if (tok) {
      r |= atoi(tok) << 21;
      tok = strtok(NULL, sep);
      if (tok) {
	 r |= atoi(tok) << 16;
	 tok = strtok(NULL, sep);
	 if (tok) {
	    r |= (atoi(tok) - 1980) << 25;
	    tok = strtok(NULL, sep);
	    if (tok) {
	       r |= atoi(tok) << 11;
	       tok = strtok(NULL, sep);
	       if (tok) {
		  r |= atoi(tok) << 5;
	       }
	    }
	 }
      }
   }

   return r;
}



/* converts a file timestamp from integer to ASCII representation */
char *datedit_ftime2asc(long time)
{
   static char buf[80];

   sprintf(buf, "%ld-%02ld-%ld, %ld:%02ld",
		(time >> 21) & 0xF,
		(time >> 16) & 0x1F,
		((time >> 25) & 0x7F) + 1980,
		(time >> 11) & 0x1F,
		(time >> 5) & 0x3F);

   return buf;
}



/* grabs an object from a disk file */
DATAFILE *datedit_grab(char *filename, char *name, int type, int x, int y, int w, int h, int colordepth)
{
   static DATAFILE dat;
   void *(*grab)(char *filename, long *size, int x, int y, int w, int h, int depth) = NULL;
   char *ext = get_extension(filename);
   char *tok;
   char tmp[256];
   int c;

   dat.dat = NULL;
   dat.size = 0;
   dat.prop = NULL;

   if (type)
      dat.type = type;
   else {
      dat.type = DAT_DATA;

      if ((ext) && (*ext)) {
	 for (c=0; object_info[c].type != DAT_END; c++) {
	    strcpy(tmp, object_info[c].ext);
	    tok = strtok(tmp, ";");
	    while (tok) {
	       if (stricmp(tok, ext) == 0) {
		  dat.type = object_info[c].type;
		  goto found_type;
	       }
	       tok = strtok(NULL, ";");
	    }
	 }
      }
   }

   found_type:

   for (c=0; object_info[c].type != DAT_END; c++) {
      if (object_info[c].type == dat.type) {
	 grab = object_info[c].grab;
	 break;
      }
   }

   if (!grab)
      grab = grab_binary;

   dat.dat = grab(filename, &dat.size, x, y, w, h, colordepth);

   if (dat.dat) {
      datedit_set_property(&dat, DAT_NAME, name);
      datedit_set_property(&dat, DAT_ORIG, filename);
      datedit_set_property(&dat, DAT_DATE, datedit_ftime2asc(file_time(filename)));
      errno = 0;
   }
   else
      datedit_error("Error reading %s as type %c%c%c%c", filename, 
		    dat.type>>24, (dat.type>>16)&0xFF,
		    (dat.type>>8)&0xFF, dat.type&0xFF);

   return &dat;
}



/* grabs an object over the top of an existing one */
int datedit_grabreplace(DATAFILE *dat, char *filename, char *name, char *type, int colordepth, int x, int y, int w, int h)
{
   DATAFILE *tmp = datedit_grab(filename, name, 
				datedit_clean_typename(type), 
				x, y, w, h, colordepth);

   if (tmp->dat) {
      _unload_datafile_object(dat);
      *dat = *tmp;
      return TRUE;
   }
   else
      return FALSE;
}



/* updates an object in-place */
int datedit_grabupdate(DATAFILE *dat, char *filename, int x, int y, int w, int h)
{
   DATAFILE *tmp = datedit_grab(filename, "dummyname", dat->type, x, y, w, h, -1);
   DATAFILE_PROPERTY *tmp_prop;

   if (tmp->dat) {
      /* exchange properties */
      tmp_prop = tmp->prop;
      tmp->prop = dat->prop;
      dat->prop = tmp_prop;

      datedit_set_property(tmp, DAT_DATE, get_datafile_property(dat, DAT_DATE));

      /* adjust color depth? */
      if ((dat->type == DAT_BITMAP) || (dat->type == DAT_RLE_SPRITE) ||
	  (dat->type == DAT_C_SPRITE) || (dat->type == DAT_XC_SPRITE)) {

	 int src_depth, dest_depth;

	 if (dat->type == DAT_RLE_SPRITE) {
	    dest_depth = ((RLE_SPRITE *)dat->dat)->color_depth;
	    src_depth = ((RLE_SPRITE *)tmp->dat)->color_depth;
	 }
	 else {
	    dest_depth = bitmap_color_depth(dat->dat);
	    src_depth = bitmap_color_depth(tmp->dat);
	 }

	 if (src_depth != dest_depth) {
	    BITMAP *b1, *b2;

	    if (dat->type == DAT_RLE_SPRITE) {
	       RLE_SPRITE *spr = (RLE_SPRITE *)tmp->dat;
	       b1 = create_bitmap_ex(src_depth, spr->w, spr->h);
	       clear_to_color(b1, b1->vtable->mask_color);
	       draw_rle_sprite(b1, spr, 0, 0);
	       destroy_rle_sprite(spr);
	    }
	    else
	       b1 = (BITMAP *)tmp->dat;

	    if (dest_depth == 8)
	       datedit_msg("Warning: lossy conversion from truecolor to 256 colors!");

	    if ((dat->type == DAT_RLE_SPRITE) ||
		(dat->type == DAT_C_SPRITE) || (dat->type == DAT_XC_SPRITE)) {
	       last_read_pal[0].r = 63;
	       last_read_pal[0].g = 0;
	       last_read_pal[0].b = 63;
	    }

	    select_palette(last_read_pal);
	    b2 = create_bitmap_ex(dest_depth, b1->w, b1->h);
	    blit(b1, b2, 0, 0, 0, 0, b1->w, b1->h);
	    unselect_palette();

	    if (dat->type == DAT_RLE_SPRITE) {
	       tmp->dat = get_rle_sprite(b2);
	       destroy_bitmap(b1);
	       destroy_bitmap(b2);
	    }
	    else {
	       tmp->dat = b2;
	       destroy_bitmap(b1);
	    }
	 }
      }

      _unload_datafile_object(dat);
      *dat = *tmp;
      return TRUE;
   }
   else
      return FALSE;
}



/* grabs a new object, inserting it into the datafile */
DATAFILE *datedit_grabnew(DATAFILE *dat, char *filename, char *name, char *type, int colordepth, int x, int y, int w, int h)
{
   DATAFILE *tmp = datedit_grab(filename, name, 
				datedit_clean_typename(type), 
				x, y, w, h, colordepth);
   int len;

   if (tmp->dat) {
      len = 0;
      while (dat[len].type != DAT_END)
	 len++;

      dat = realloc(dat, sizeof(DATAFILE)*(len+2));
      dat[len+1] = dat[len];
      dat[len] = *tmp;
      return dat;
   }
   else
      return NULL;
}



/* inserts a new object into the datafile */
DATAFILE *datedit_insert(DATAFILE *dat, DATAFILE **ret, char *name, int type, void *v, long size)
{
   int len;

   len = 0;
   while (dat[len].type != DAT_END)
      len++;

   dat = realloc(dat, sizeof(DATAFILE)*(len+2));
   dat[len+1] = dat[len];

   dat[len].dat = v;
   dat[len].type = type;
   dat[len].size = size;
   dat[len].prop = NULL;
   datedit_set_property(dat+len, DAT_NAME, name);

   if (ret)
      *ret = dat+len;

   return dat;
}



/* wrapper for examining numeric property values */
int datedit_numprop(DATAFILE *dat, int type)
{
   char *p = get_datafile_property(dat, type);

   if (*p)
      return atoi(p);
   else 
      return -1;
}



/* conditionally update an out-of-date object */
int datedit_update(DATAFILE *dat, int verbose, int *changed)
{
   char *name = get_datafile_property(dat, DAT_NAME);
   char *origin = get_datafile_property(dat, DAT_ORIG);
   char *date = get_datafile_property(dat, DAT_DATE);

   if (!*origin) {
      datedit_msg("%s has no origin data - skipping", name);
      return TRUE;
   }

   if (!exists(origin)) {
      datedit_msg("%s: %s not found - skipping", name, origin);
      return TRUE;
   }

   if (*date) {
      if ((file_time(origin) & 0xFFFFFFE0) <= 
	  (datedit_asc2ftime(date) & 0xFFFFFFE0)) {
	 if (verbose)
	    datedit_msg("%s: %s has not changed - skipping", name, origin);
	 return TRUE;
      }
   }

   datedit_msg("Updating %s -> %s", origin, name);

   if (changed)
      *changed = TRUE;

   return datedit_grabupdate(dat, origin, 
			     datedit_numprop(dat, DAT_XPOS), 
			     datedit_numprop(dat, DAT_YPOS), 
			     datedit_numprop(dat, DAT_XSIZ), 
			     datedit_numprop(dat, DAT_YSIZ));
}

