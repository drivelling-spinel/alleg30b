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
 *      LBM reader by Adrian Oboroc.
 *
 *      See readme.txt for copyright information.
 */


#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <limits.h>

#include "allegro.h"
#include "internal.h"



/* load_lbm:
 *  Loads IFF ILBM/PBM files with up to 8 bits per pixel, returning
 *  a bitmap structure and storing the palette data in the specified
 *  palette (this should be an array of at least 256 RGB structures).
 */
BITMAP *load_lbm(char *filename, RGB *pal)
{
   #define IFF_FORM     0x4D524F46     /* 'FORM' - IFF FORM structure  */
   #define IFF_ILBM     0x4D424C49     /* 'ILBM' - interleaved bitmap  */
   #define IFF_PBM      0x204D4250     /* 'PBM ' - new DP2e format     */
   #define IFF_BMHD     0x44484D42     /* 'BMHD' - bitmap header       */
   #define IFF_CMAP     0x50414D43     /* 'CMAP' - color map (palette) */
   #define IFF_BODY     0x59444F42     /* 'BODY' - bitmap data         */

   /* BSWAPL, BSWAPW
    *  Byte swapping macros for convertion between
    *  Intel and Motorolla byte ordering.
    */

   #define BSWAPL(x)   ((((x) & 0x000000ff) << 24) + \
			(((x) & 0x0000ff00) <<  8) + \
			(((x) & 0x00ff0000) >>  8) + \
			(((x) & 0xff000000) >> 24))

   #define BSWAPW(x)    (((x) & 0x00ff) << 8) + (((x) & 0xff00) >> 8)

   PACKFILE *f;
   BITMAP *b = NULL;
   int w, h, i, x, y, bpl, ppl, pbm_mode;
   char ch, cmp_type, bit_plane, color_depth;
   unsigned char uc, check_flags, bit_mask, *line_buf;
   long id, len, l;
   int dest_depth = _color_load_depth(8);

   f = pack_fopen(filename, F_READ);
   if (!f)
      return NULL;

   id = pack_igetl(f);              /* read file header    */
   if (id != IFF_FORM) {            /* check for 'FORM' id */
      pack_fclose(f);
      return NULL;
   }

   pack_igetl(f);                   /* skip FORM length    */

   id = pack_igetl(f);              /* read id             */

   /* check image type ('ILBM' or 'PBM ') */
   if ((id != IFF_ILBM) && (id != IFF_PBM)) {
      pack_fclose(f);
      return NULL;
   }

   pbm_mode = id == IFF_PBM;

   id = pack_igetl(f);              /* read id               */
   if (id != IFF_BMHD) {            /* check for header      */
      pack_fclose(f);
      return NULL;
   }

   len = pack_igetl(f);             /* read header length    */
   if (len != BSWAPL(20)) {         /* check, if it is right */
      pack_fclose(f);
      return NULL;
   }

   i = pack_igetw(f);               /* read screen width  */
   w = BSWAPW(i);

   i = pack_igetw(f);               /* read screen height */
   h = BSWAPW(i);

   pack_igetw(f);                   /* skip initial x position  */
   pack_igetw(f);                   /* skip initial y position  */

   color_depth = pack_getc(f);      /* get image depth   */
   if (color_depth > 8) {
      pack_fclose(f);
      return NULL;
   }

   pack_getc(f);                    /* skip masking type */

   cmp_type = pack_getc(f);         /* get compression type */
   if ((cmp_type != 0) && (cmp_type != 1)) {
      pack_fclose(f);
      return NULL;
   }

   pack_getc(f);                    /* skip unused field        */
   pack_igetw(f);                   /* skip transparent color   */
   pack_getc(f);                    /* skip x aspect ratio      */
   pack_getc(f);                    /* skip y aspect ratio      */
   pack_igetw(f);                   /* skip default page width  */
   pack_igetw(f);                   /* skip default page height */

   check_flags = 0;

   do {  /* We'll use cycle to skip possible junk      */
	 /*  chunks: ANNO, CAMG, GRAB, DEST, TEXT etc. */
      id = pack_igetl(f);

      switch(id) {

	 case IFF_CMAP:
	    memset(pal, 0, 256 * 3);
	    l = pack_igetl(f);
	    len = BSWAPL(l) / 3;
	    for (i=0; i<len; i++) {
	       pal[i].r = pack_getc(f) >> 2;
	       pal[i].g = pack_getc(f) >> 2;
	       pal[i].b = pack_getc(f) >> 2;
	    }
	    check_flags |= 1;       /* flag "palette read" */
	    break;

	 case IFF_BODY:
	    pack_igetl(f);          /* skip BODY size */
	    b = create_bitmap_ex(8, w, h);
	    if (!b) {
	       pack_fclose(f);
	       return NULL;
	    }

	    memset(b->dat, 0, w * h);

	    if (pbm_mode)
	       bpl = w;
	    else {
	       bpl = w >> 3;        /* calc bytes per line  */
	       if (w & 7)           /* for finish bits      */
		  bpl++;
	    }
	    if (bpl & 1)            /* alignment            */
	       bpl++;
	    line_buf = malloc(bpl);
	    if (!line_buf) {
	       pack_fclose(f);
	       return NULL;
	    }

	    if (pbm_mode) {
	       for (y = 0; y < h; y++) {
		  if (cmp_type) {
		     i = 0;
		     while (i < bpl) {
			uc = pack_getc(f);
			if (uc < 128) {
			   uc++;
			   pack_fread(&line_buf[i], uc, f);
			   i += uc;
			}
			else if (uc > 128) {
			   uc = 257 - uc;
			   ch = pack_getc(f);
			   memset(&line_buf[i], ch, uc);
			   i += uc;
			}
			/* 128 (0x80) means NOP - no operation  */
		     }
		  }
		  else  /* pure theoretical situation */
		     pack_fread(line_buf, bpl, f);

		  memcpy(b->line[y], line_buf, bpl);
	       }
	    }
	    else {
	       for (y = 0; y < h; y++) {
		  for (bit_plane = 0; bit_plane < color_depth; bit_plane++) {
		     if (cmp_type) {
			i = 0;
			while (i < bpl) {
			   uc = pack_getc(f);
			   if (uc < 128) {
			      uc++;
			      pack_fread(&line_buf[i], uc, f);
			      i += uc;
			   }
			   else if (uc > 128) {
			      uc = 257 - uc;
			      ch = pack_getc(f);
			      memset(&line_buf[i], ch, uc);
			      i += uc;
			   }
			   /* 128 (0x80) means NOP - no operation  */
			}
		     }
		     else
			pack_fread(line_buf, bpl, f);

		     bit_mask = 1 << bit_plane;
		     ppl = bpl;     /* for all pixel blocks */
		     if (w & 7)     /*  may be, except the  */
			ppl--;      /*  the last            */

		     for (x = 0; x < ppl; x++) {
			if (line_buf[x] & 128)
			   b->line[y][x * 8]     |= bit_mask;
			if (line_buf[x] & 64)
			   b->line[y][x * 8 + 1] |= bit_mask;
			if (line_buf[x] & 32)
			   b->line[y][x * 8 + 2] |= bit_mask;
			if (line_buf[x] & 16)
			   b->line[y][x * 8 + 3] |= bit_mask;
			if (line_buf[x] & 8)
			   b->line[y][x * 8 + 4] |= bit_mask;
			if (line_buf[x] & 4)
			   b->line[y][x * 8 + 5] |= bit_mask;
			if (line_buf[x] & 2)
			   b->line[y][x * 8 + 6] |= bit_mask;
			if (line_buf[x] & 1)
			   b->line[y][x * 8 + 7] |= bit_mask;
		     }

		     /* last pixel block */
		     if (w & 7) {
			x = bpl - 1;

			/* no necessary to check if (w & 7) > 0 in */
			/* first condition, because (w & 7) != 0   */
			if (line_buf[x] & 128)
			   b->line[y][x * 8]     |= bit_mask;
			if ((line_buf[x] & 64) && ((w & 7) > 1))
			   b->line[y][x * 8 + 1] |= bit_mask;
			if ((line_buf[x] & 32) && ((w & 7) > 2))
			   b->line[y][x * 8 + 2] |= bit_mask;
			if ((line_buf[x] & 16) && ((w & 7) > 3))
			   b->line[y][x * 8 + 3] |= bit_mask;
			if ((line_buf[x] & 8)  && ((w & 7) > 4))
			   b->line[y][x * 8 + 4] |= bit_mask;
			if ((line_buf[x] & 4)  && ((w & 7) > 5))
			   b->line[y][x * 8 + 5] |= bit_mask;
			if ((line_buf[x] & 2)  && ((w & 7) > 6))
			   b->line[y][x * 8 + 6] |= bit_mask;
			if ((line_buf[x] & 1)  && ((w & 7) > 7))
			   b->line[y][x * 8 + 7] |= bit_mask;
		     }
		  }
	       }
	    }
	    free(line_buf);
	    check_flags |= 2;       /* flag "bitmap read" */
	    break;

	 default:                   /* skip useless chunks  */
	    l = pack_igetl(f);
	    len = BSWAPL(l);
	    if (len & 1)
	       len++;
	    for (l=0; l < (len >> 1); l++)
	       pack_igetw(f);
      }

      /* Exit from loop if we are at the end of file, */
      /* or if we loaded both bitmap and palette      */

   } while ((check_flags != 3) && (!pack_feof(f)));

   pack_fclose(f);

   if (check_flags != 3) {
      if (check_flags & 2)
	 destroy_bitmap(b);
      return FALSE;
   }

   if (errno) {
      destroy_bitmap(b);
      return FALSE;
   }

   if (dest_depth != 8) {
      BITMAP *b2 = b;

      b = create_bitmap_ex(dest_depth, b2->w, b2->h);
      if (!b) {
	 destroy_bitmap(b2);
	 return NULL;
      }

      select_palette(pal);
      blit(b2, b, 0, 0, 0, 0, b->w, b->h);
      unselect_palette();

      destroy_bitmap(b2);
   }

   return b;
}

