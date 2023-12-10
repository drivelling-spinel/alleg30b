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
 *      Optimised palette generation routines, by Michal Mertl.
 *
 *      See readme.txt for copyright information.
 */


#include <stdlib.h>

#include <allegro.h>


#define HASHTABLESIZE   1031
#define BITMASK15       0x7bde              /* 111 1011 1101 1110 */
#define BITMASK16       0xf79e              /* 1111 0111 1001 1110 */
#define BITMASK24       0xf0f0f0


typedef struct NODE
{
   struct NODE *next;
   int data;
   int count;
} NODE;


static NODE **hash_table;


static inline int hash(int data)
{
   return (data % HASHTABLESIZE);
}


static int imgdepth;



static NODE *sort_list(NODE * list)
{
   NODE *newlist, *tmp, *tmp0, *listbak;

   newlist = (NODE *)calloc(1, sizeof(NODE));
   listbak = list;
   list = list->next;

   while (list) {
      tmp = newlist;

      while (tmp->next) {
	 if (tmp->next->count < list->count)
	    break;
	 tmp = tmp->next;
      }

      tmp0 = (NODE *)calloc(1, sizeof(NODE));
      tmp0->count = list->count;
      tmp0->data = list->data;
      tmp0->next = tmp->next;
      tmp->next = tmp0;
      list = list->next;
   }

   while (listbak->next) {
      tmp = listbak;
      listbak = listbak->next;
      free(tmp);
   }

   return newlist;
}



static NODE *insert_node(int data)
{
   NODE *p, *p0;
   int bucket;
   int i = 0;

   bucket = hash(data);
   p = hash_table[bucket];

   while (p->next) {
      if (p->next->data == data) {
	 i = 1;
	 break;
      }
      p = p->next;
   }

   if (i) {                /* a node with the same data was found */
      p->next->count++;
      p0 = p->next;
   }
   else {
      p0 = (NODE *)calloc(1, sizeof(NODE));
      p0->data = data;
      p0->count = 1;
      p0->next = NULL;
      p->next = p0;
   }

   return p0;
}



static void delete_node(int data)
{
   NODE *p0, *p;
   int bucket;

   /* find node */
   p0 = NULL;
   bucket = hash(data);
   p = hash_table[bucket];

   while ((p) && (p->data != data)) {
      p0 = p;
      p = p->next;
   }

   if (!p)
      return;

   /* p designates node to delete, remove it from list */
   if (p0) {
      /* not first node, p0 points to previous node */
      if (p->count > 1)
	 p->count--;
      else {
	 p0->next = p->next;
	 free(p);
      }
   }
   else {
      if (p->count > 1) {
	 /* first node on chain */
	 p->count--;
      }
      else {
	 hash_table[bucket] = p->next;
	 free(p);
      }
   }
}



static NODE *find_node(int data)
{
   int bucket;
   NODE *p;

   bucket = hash(data);
   p = hash_table[bucket];

   while ((p) && (p->data != data))
      p = p->next;

   return p;
}



static void delete_list(NODE *list)
{
   NODE *tmp;

   while (list) {
      tmp = list;
      list = list->next;
      free(tmp);
   }
}



/* compare two high/true color values */
static int compare_cols(int num1, int num2)
{
   switch (imgdepth) {

      case 16:
	 return (abs(((num1 >> 11) & 0x1f) - ((num2 >> 11) & 0x1f))
		 + abs(((num1 >> 5) & 0x3f) - ((num2 >> 5) & 0x3f))
		 + abs((num1 & 0x1f) - (num2 & 0x1f)));

      case 15:
	 return (abs(((num1 >> 10) & 0x1f) - ((num2 >> 10) & 0x1f))
		 + abs(((num1 >> 5) & 0x1f) - ((num2 >> 5) & 0x1f))
		 + abs((num1 & 0x1f) - (num2 & 0x1f)));
      case 24:
      case 32:
	 return (abs((num1 >> 16) - (num2 >> 16))
		 + abs(((num1 >> 8) & 0xff) - ((num2 >> 8) & 0xff))
		 + abs((num1 & 0xff) - (num2 & 0xff)));

      default: 
	 /* should never happen, just to prevent compiler from warnings */
	 return 0;
   }
}



/* generate_optimized_palette:
 *  Calculates a suitable palette for color reducing the specified truecolor
 *  image. If the rsvdcols parameter is not NULL, it contains an array of
 *  256 flags specifying which colors the palette is allowed to use.
 */
int generate_optimized_palette(BITMAP *image, PALETTE pal, char rsvdcols[256])
{
   NODE *help_table[HASHTABLESIZE]; 
   int *common, *common2;
   int imgsize, i, max;
   int j, tmp, big2 = 0, big, best = 0, best2 = 0, rsvdcnt = 0, start;
   char tmprsvd[256];

   if (!rsvdcols) {
      pal[0].r = 63;
      pal[0].g = 0;
      pal[0].b = 63;

      tmprsvd[0] = TRUE;

      for (i=1; i<256; i++)
	 tmprsvd[i] = FALSE;

      rsvdcols = tmprsvd;
   }

   hash_table = (NODE **)calloc(HASHTABLESIZE, sizeof(NODE *));
   for (i=0; i<HASHTABLESIZE; i++)
      hash_table[i] = (NODE *)calloc(1, sizeof(NODE));

   imgsize = image->h * image->w;
   imgdepth = image->vtable->color_depth;

   /* fill the 'hash_table' with 4bit per RGB color values */
   switch (imgdepth) {

      case 32:
	 for (i=0; i<imgsize; i++)
	    insert_node((((int *)image->dat)[i] & BITMASK24) >> 4);
	 break;

      case 24:
	 for (i=0; i<imgsize; i++) {
	    insert_node(((*((int *)(((char *)image->dat)+i*3))) & BITMASK24) >> 4);
	 }
	 break;

      case 16:
	 for (i=0; i<imgsize; i++)
	    insert_node((((short *)image->dat)[i] & BITMASK16) >> 1);
	 break;

      case 15:
	 for (i=0; i<imgsize; i++)
	    insert_node((((short *)image->dat)[i] & BITMASK15) >> 1);
	 break;

      default:
	 return -1;
   }

   /* sort hash_table's lists with the biggest count first */
   for (i=0; i<HASHTABLESIZE; i++)
      hash_table[i] = sort_list(hash_table[i]);

   /* Merge the most common values to 'common' table. Note that to search
    * for the biggest 'count' it's enough to check first entry in each list
    * in hash_table because the lists are already sorted by the 'count' value.
    * We'll fill in first 512 values because if we fill directly palette here
    * it could miss some rare but very different color (I had it this way, but
    * it gave poor results for some images).
    *
    * Several pallete entries can be saved by filling the r,g,b values in
    * advance and in this routine skipping over them.
    */

   for (i=0; i<HASHTABLESIZE; i++)
      help_table[i] = hash_table[i]->next;

   /* first items are helpers with no values so we can skip over them */
   for (i=0; i<256; i++)
      if (rsvdcols[i])
	 rsvdcnt++;

   common = (int *)calloc((512 + rsvdcnt), sizeof(int));
   common2 = (int *)calloc((512 + rsvdcnt), sizeof(int));

   /* store the reserved entries in common to be able to count with them */
   switch (imgdepth) {

      case 32:
      case 24:
	 for (i=0, j=0; i<256; i++) {
	    if (rsvdcols[i]) {
	       common[j] = (pal[i].r>>2)<<16 | (pal[i].g>>2)<<8 | (pal[i].b>>2);
	       j++;
	    }
	 }
	 break;

      case 16:
	 for (i=0, j=0; i<256; i++) {
	    if (rsvdcols[i]) {
	       common[j] = (pal[i].r>>2)<<11 | (pal[i].g>>2)<<5 | (pal[i].b>>2);
	       j++;
	    }
	 }
	 break;

      case 15:
	 for (i=0, j=0; i<256; i++) {
	    if (rsvdcols[i]) {
	       common[j] = (pal[i].r>>2)<<10 | (pal[i].g>>2)<<5 | (pal[i].b>>2);
	       j++;
	    }
	 }
	 break;
   }

   for (i=rsvdcnt, max=rsvdcnt+512; i<max; i++) {
      int curmax = 0, big = 0, j;

      common[i] = 0;

      for (j=0; j<HASHTABLESIZE; j++) {
	 if ((help_table[j]) && (help_table[j]->count > curmax)) {
	    curmax = help_table[j]->count;
	    big = j;
	 }
      }

      if (!help_table[big]) {
	 max = i;
	 break;
      }

      common[i] = help_table[big]->data;
      help_table[big] = help_table[big]->next;
   }

   /* Compare each value we'll possibly use to replace with all already used
    * colors (up to START) to find the most different ones. Lot of colors are
    * just so much similar (difference smaller than some small value (here 3)
    * than it's useless to continue comparing, it speeds the process a lot too.
    */
   start = 256 - (256 - rsvdcnt) / 10;
   best = max;

   for (i=0; i<rsvdcnt; i++) {
      for (j=rsvdcnt; j<rsvdcnt+100; j++) {     /* +100 should be enough */
	 if ((tmp = compare_cols(common[i], common[j])) == 0) {
	    memcpy(&common[j], &common[j + 1], (max - j) * 4);
	    j--;
	    max--;
	 }
      }
   }

   if ((rsvdcnt > 150) && (best > max + 50))
      start = rsvdcnt;

   for (i=start; i<max; i++) {
      for (j=0, big=10000; j<start; j++) {
	 tmp = compare_cols(common[i], common[j]);
	 if (tmp < big) {
	    big = tmp;
	    best = i;
	    if (tmp < 3)
	       break;
	 }
      }
      common2[i] = big;         /* place the smallest difference in there */

      /* find the biggest difference color to be used for color START
       * (save one run in the next loop).
       */
      if (big > big2) {
	 big2 = big;
	 best2 = best;
      }
   }

   common[start] = common[best2];

   /* replace other 'common' >START && <256 with very different colors */
   for (i=start+1; i<256; i++) {

      /* Look if the last added value (i-1) doesn't change the differences 
       * (e.g. image is full of green shadows and several red pixels. If we 
       * are happy (and in this case we must be) to 4bit per component we use 
       * fiter them to lot less so in first 512 'common' is also the red 
       * color. But we already placed the red color to 'common' table so to 
       * get there another shade of red is now of lot less importance.
       */
      for (j=i+1; j<max; j++) {
	 tmp = compare_cols(common[j], common[i]);
	 if (tmp < common2[j])
	    common2[j] = tmp;
      }

      /* find the biggest 'common[1]' which is the color the most different 
       * from these already used.
       */
      for (j=i+1, big=0; j<max; j++) {
	 if (common2[j] > big) {
	    big = common2[j];
	    best = j;
	 }
      }

      /* place it into 'common' and set it's difference to 0 to prevent it 
       * from being included more times.
       */
      common[i] = common[best];
      common2[best] = 0;
   }

   /* Max is at most 512 but could be lot less (e.g. image is all white). If
    * the image use more than 256 colors we must use only the 256 (the pallete
    * size). We than remap our reduced color values back to 6 bits per RGB
    * component (the standard VGA DAC resolution).
    */
   if (max > 256)
      max = 256;

   switch (image->vtable->color_depth) {

      case 32:
      case 24:
	 for (i=0, j=rsvdcnt; i<max; i++) {
	    if (!rsvdcols[i]) {
	       pal[i].r = getr24(common[j]) << 2;
	       pal[i].g = getg24(common[j]) << 2;
	       pal[i].b = getb24(common[j]) << 2;
	       j++;
	    }
	 }
	 break;

      case 16:
	 for (i=0, j=rsvdcnt; i<max; i++) {
	    if (!rsvdcols[i]) {
	       pal[i].r = getr16(common[j]) >> 1;
	       pal[i].g = getg16(common[j]) >> 1;
	       pal[i].b = getb16(common[j]) >> 1;
	       j++;
	    }
	 }
	 break;

      case 15:
	 for (i=0, j=rsvdcnt; i<max; i++) {
	    if (!rsvdcols[i]) {
	       pal[i].r = getr15(common[j]) >> 1;
	       pal[i].g = getg15(common[j]) >> 1;
	       pal[i].b = getb15(common[j]) >> 1;
	       j++;
	    }
	 }
	 break;
   }

   /* free all dynamically allocated memory */
   for (i=0; i<HASHTABLESIZE; i++)
      delete_list(hash_table[i]);

   for (i=0; i<HASHTABLESIZE; i++)
      free(hash_table[i]);

   free(hash_table);

   return 0;
}

