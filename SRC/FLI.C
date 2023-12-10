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
 *      FLI/FLC player.
 *
 *      Based on code provided by Jonathan Tarbox.
 *
 *      See readme.txt for copyright information.
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "allegro.h"
#include "internal.h"


#define FLI_MAGIC1            0xAF11      /* file header magic number */
#define FLI_MAGIC2            0xAF12      /* file magic number (Pro) */
#define FLI_FRAME_MAGIC       0xF1FA      /* frame header magic number */
#define FLI_FRAME_PREFIX      0xF100      /* FLC's prefix info */
#define FLI_FRAME_USELESS     0x00A1      /* FLC's garbage frame */



typedef struct FLI_HEADER
{
   long              size              __attribute__ ((packed));
   unsigned short    type              __attribute__ ((packed)); 
   unsigned short    frame_count       __attribute__ ((packed));
   unsigned short    width             __attribute__ ((packed));
   unsigned short    height            __attribute__ ((packed));
   unsigned short    bits_a_pixel      __attribute__ ((packed));
   unsigned short    flags             __attribute__ ((packed));
   unsigned short    speed             __attribute__ ((packed));
   long              next_head         __attribute__ ((packed));
   long              frames_in_table   __attribute__ ((packed));
   char              reserved[102]     __attribute__ ((packed)); 
} FLI_HEADER;



typedef struct FLI_FRAME
{
   unsigned long     size              __attribute__ ((packed));
   unsigned short    type              __attribute__ ((packed)); 
   unsigned short    chunks            __attribute__ ((packed));
   char              pad[8]            __attribute__ ((packed));
} FLI_FRAME;



typedef struct FLI_CHUNK 
{
   unsigned long     size              __attribute__ ((packed));
   unsigned short    type              __attribute__ ((packed));
} FLI_CHUNK;



static int fli_status = FLI_NOT_OPEN;  /* current state of the FLI player */

BITMAP *fli_bitmap = NULL;             /* current frame of the FLI */
PALLETE fli_pallete;                   /* current pallete the FLI is using */

int fli_bmp_dirty_from = INT_MAX;      /* what part of fli_bitmap is dirty */
int fli_bmp_dirty_to = INT_MIN;
int fli_pal_dirty_from = INT_MAX;      /* what part of fli_pallete is dirty */
int fli_pal_dirty_to = INT_MIN;

int fli_frame = 0;                     /* current frame number in the FLI */

volatile int fli_timer = 0;            /* for timing FLI playback */

static int fli_file = 0;               /* the file we are reading */

static void *fli_mem_data = NULL;      /* the memory FLI we are playing */
static int fli_mem_pos = 0;            /* position in the memory FLI */

static FLI_HEADER fli_header;          /* header structure */
static FLI_FRAME frame_header;         /* frame header structure */



/* fli_timer_callback:
 *  Timer interrupt handler for syncing FLI files.
 */
static void fli_timer_callback()
{
   fli_timer++;
}

static END_OF_FUNCTION(fli_timer_callback);



/* fli_read:
 *  Helper function to get a block of data from the FLI, which can read 
 *  from disk or a copy of the FLI held in memory. If buf is set, that is 
 *  where it stores the data, otherwise it uses the scratch buffer. Returns 
 *  a pointer to the data, or NULL on error.
 */
static void *fli_read(void *buf, int size)
{
   int result;

   if (fli_mem_data) {
      if (buf)
	 memcpy(buf, fli_mem_data+fli_mem_pos, size);
      else
	 buf = fli_mem_data+fli_mem_pos;

      fli_mem_pos += size;
   }
   else {
      if (!buf) {
	 _grow_scratch_mem(size);
	 buf = _scratch_mem;
      }

      FILE_READ(fli_file, buf, size, result);
      if (result != size)
	 return NULL;
   }

   return buf;
}



/* fli_seek:
 *  Helper function to move to a different part of the FLI file data.
 *  Pass offset and seek mode, as per the lseek() function.
 */
static void fli_seek(int offset, int mode)
{
   if (fli_mem_data) {
      if (mode == SEEK_CUR)
	 fli_mem_pos += offset;
      else
	 fli_mem_pos = offset;
   }
   else
      lseek(fli_file, offset, mode);
}



/* helpers for reading FLI chunk data */
#define READ_BYTE()     ((sz-- > 0)    ? *(((unsigned char *)p)++) : 0)
#define READ_CHAR()     ((sz-- > 0)    ? *(((signed char   *)p)++) : 0)
#define READ_SHORT()    (((sz-=2) > 0) ? *(((signed short  *)p)++) : 0)

#define READ_BLOCK(pos, size)             \
   {                                      \
      if (sz >= (size)) {                 \
	 memcpy(pos, p, size);            \
	 p += (size);                     \
	 sz -= (size);                    \
      }                                   \
      else {                              \
	 char *_p = (char *)(pos);        \
	 int _c;                          \
					  \
	 for (_c=0; _c < (size); _c++)    \
	    *(_p++) = READ_BYTE();        \
      }                                   \
   }

#define READ_RLE_BYTE(pos, size)          \
   {                                      \
      memset(pos, READ_BYTE(), size);     \
   }

#define READ_RLE_WORD(pos, size)          \
   {                                      \
      short *_p = (short *)(pos);         \
      short _v = READ_SHORT();            \
      int _c;                             \
					  \
      for (_c=0; _c < (size); _c++)       \
	 *(_p++) = _v;                    \
   }



/* do_fli_256_color:
 *  Processes an FLI 256_COLOR chunk
 */
static void do_fli_256_color(unsigned char *p, int sz)
{
   int packets;
   int c, c2;
   int offset;
   int length;

   offset = 0;
   packets = READ_SHORT();

   for (c=0; c<packets; c++) {
      offset += READ_BYTE();
      length = READ_BYTE();
      if (length == 0)
	 length = 256;

      for(c2=0; c2<length; c2++) {
	 fli_pallete[offset+c2].r = READ_BYTE() / 4;
	 fli_pallete[offset+c2].g = READ_BYTE() / 4;
	 fli_pallete[offset+c2].b = READ_BYTE() / 4;
      }

      fli_pal_dirty_from = MIN(fli_pal_dirty_from, offset);
      fli_pal_dirty_to = MAX(fli_pal_dirty_to, offset+length-1);

      offset += length;
   }
}



/* do_fli_delta:
 *  Processes an FLI DELTA chunk
 */
static void do_fli_delta(unsigned char *p, int sz)
{
   int lines;
   int packets;
   int size;
   int x, y;

   y = 0;
   lines = READ_SHORT();

   while (lines-- > 0) {                  /* for each line... */
      packets = READ_SHORT();

      while (packets < 0) {
	 if (packets & 0x4000)
	    y -= packets;
	 else
	    fli_bitmap->line[y][fli_bitmap->w-1] = packets & 0xFF;

	 packets = READ_SHORT();
      }

      x = 0;

      while (packets-- > 0) {
	 x += READ_BYTE();                /* skip bytes */
	 size = READ_CHAR();

	 if (size > 0) {                  /* copy size words */
	    READ_BLOCK(fli_bitmap->line[y]+x, size*2);
	    x += size*2;
	 }
	 else if (size < 0) {             /* repeat word -size times */
	    READ_RLE_WORD(fli_bitmap->line[y]+x, -size);
	    x -= size*2;
	 }
      }

      fli_bmp_dirty_from = MIN(fli_bmp_dirty_from, y);
      fli_bmp_dirty_to = MAX(fli_bmp_dirty_to, y);

      y++;
   }
}



/* do_fli_color:
 *  Processes an FLI COLOR chunk
 */
static void do_fli_color(unsigned char *p, int sz)
{
   int packets;
   int c, c2;
   int offset;
   int length;

   offset = 0;
   packets = READ_SHORT();

   for (c=0; c<packets; c++) {
      offset += READ_BYTE();
      length = READ_BYTE();
      if (length == 0)
	 length = 256;

      for(c2=0; c2<length; c2++) {
	 fli_pallete[offset+c2].r = READ_BYTE();
	 fli_pallete[offset+c2].g = READ_BYTE();
	 fli_pallete[offset+c2].b = READ_BYTE();
      }

      fli_pal_dirty_from = MIN(fli_pal_dirty_from, offset);
      fli_pal_dirty_to = MAX(fli_pal_dirty_to, offset+length-1);

      offset += length;
   }
}



/* do_fli_lc:
 *  Processes an FLI LC chunk
 */
static void do_fli_lc(unsigned char *p, int sz)
{
   int lines;
   int packets;
   int size;
   int x, y;

   y = READ_SHORT();
   lines = READ_SHORT();

   fli_bmp_dirty_from = MIN(fli_bmp_dirty_from, y);
   fli_bmp_dirty_to = MAX(fli_bmp_dirty_to, y+lines-1);

   while (lines-- > 0) {                     /* for each line... */
      packets = READ_BYTE();
      x = 0;

      while (packets-- > 0) {
	 x += READ_BYTE();                   /* skip bytes */
	 size = READ_CHAR();

	 if (size > 0) {                     /* copy size bytes */
	    READ_BLOCK(fli_bitmap->line[y]+x, size);
	    x += size;
	 }
	 else if (size < 0) {                /* repeat byte -size times */
	    READ_RLE_BYTE(fli_bitmap->line[y]+x, -size);
	    x -= size;
	 }
      }

      y++;
   }
}



/* do_fli_black:
 *  Processes an FLI BLACK chunk
 */
static void do_fli_black(unsigned char *p, int sz)
{
   clear(fli_bitmap);

   fli_bmp_dirty_from = 0;
   fli_bmp_dirty_to = fli_bitmap->h-1;
}



/* do_fli_brun:
 *  Processes an FLI BRUN chunk
 */
static void do_fli_brun(unsigned char *p, int sz)
{
   int packets;
   int size;
   int x, y;

   for (y=0; y<fli_bitmap->h; y++) {         /* for each line... */
      packets = READ_BYTE();
      x = 0;

      while (packets-- > 0) {
	 size = READ_CHAR();

	 if (size < 0) {                     /* copy -size bytes */
	    READ_BLOCK(fli_bitmap->line[y]+x, -size);
	    x -= size;
	 }
	 else if (size > 0) {                /* repeat byte size times */
	    READ_RLE_BYTE(fli_bitmap->line[y]+x, size);
	    x += size;
	 }
      }
   }

   fli_bmp_dirty_from = 0;
   fli_bmp_dirty_to = fli_bitmap->h-1;
}



/* do_fli_copy:
 *  Processes an FLI COPY chunk
 */
static void do_fli_copy(unsigned char *p, int sz)
{
   READ_BLOCK(fli_bitmap->dat, fli_bitmap->w * fli_bitmap->h);

   fli_bmp_dirty_from = 0;
   fli_bmp_dirty_to = fli_bitmap->h-1;
}



/* read_frame:
 *  Advances to the next frame in the FLI.
 */
static void read_frame()
{
   unsigned char *p;
   FLI_CHUNK *chunk;
   int c, sz;

   if (fli_status != FLI_OK)
      return;

   get_another_frame:

   /* read the frame header */ 
   if (!fli_read(&frame_header, sizeof(FLI_FRAME))) {
      fli_status = FLI_ERROR;
      return;
   }

   /* skip FLC's useless frame */
   if ((frame_header.type == FLI_FRAME_PREFIX) || (frame_header.type == FLI_FRAME_USELESS)) {
      fli_seek(frame_header.size-sizeof(FLI_FRAME), SEEK_CUR);
      fli_frame++;

      goto get_another_frame;
   }

   if (frame_header.type != FLI_FRAME_MAGIC) {
      fli_status = FLI_ERROR;
      return;
   }

   /* return if there is no data in the frame */
   if (frame_header.size == sizeof(FLI_FRAME)) {
      fli_frame++;
      return;
   }

   /* read the frame data */
   p = fli_read(NULL, frame_header.size-sizeof(FLI_FRAME));
   if (!p) {
      fli_status = FLI_ERROR;
      return;
   }

   /* now to decode it */
   for (c=0; c<frame_header.chunks; c++) {
      chunk = (FLI_CHUNK *)p;
      sz = chunk->size - sizeof(FLI_CHUNK);
      p += sizeof(FLI_CHUNK);

      switch (chunk->type) {

	 case 4: 
	    do_fli_256_color(p, sz);
	    break;

	 case 7:
	    do_fli_delta(p, sz);
	    break;

	 case 11: 
	    do_fli_color(p, sz);
	    break;

	 case 12:
	    do_fli_lc(p, sz);
	    break;

	 case 13:
	    do_fli_black(p, sz);
	    break;

	 case 15:
	    do_fli_brun(p, sz);
	    break;

	 case 16:
	    do_fli_copy(p, sz);
	    break;

	 default:
	    /* should we return an error? nah... */
	    break;
      }

      p = ((unsigned char *)chunk) + chunk->size;
   }

   /* move on to the next frame */
   fli_frame++;
}



/* do_play_fli:
 *  Worker function used by play_fli() and play_memory_fli().
 *  This is complicated by the fact that it puts the timing delay between
 *  reading a frame and displaying it, rather than between one frame and
 *  the next, in order to make the playback as smooth as possible.
 */
static int do_play_fli(BITMAP *bmp, int loop, int (*callback)())
{
   int ret;

   ret = next_fli_frame(loop);

   while (ret == FLI_OK) {
      /* update the pallete */
      if (fli_pal_dirty_from <= fli_pal_dirty_to)
	 set_pallete_range(fli_pallete, fli_pal_dirty_from, fli_pal_dirty_to, TRUE);

      /* update the screen */
      if (fli_bmp_dirty_from <= fli_bmp_dirty_to) {
	 vsync();
	 blit(fli_bitmap, bmp, 0, fli_bmp_dirty_from, 0, fli_bmp_dirty_from,
			fli_bitmap->w, 1+fli_bmp_dirty_to-fli_bmp_dirty_from);
      }

      reset_fli_variables();

      if (callback) {
	 ret = (*callback)();
	 if (ret != FLI_OK)
	    break;
      }

      ret = next_fli_frame(loop);

      do {
	 /* wait a bit */
      } while (fli_timer <= 0);
   }

   close_fli();

   return (ret == FLI_EOF) ? FLI_OK : ret;
}



/* play_fli:
 *  Top level FLI playing function. Plays the specified file, displaying 
 *  it at the top left corner of the specified bitmap. If the callback 
 *  function is not NULL it will be called for each frame in the file, and 
 *  can return zero to continue playing or non-zero to stop the FLI. If 
 *  loop is non-zero the player will cycle through the animation (in this 
 *  case the callback function is the only way to stop the player). Returns 
 *  one of the FLI status constants, or the value returned by the callback 
 *  if this is non-zero. If you need to distinguish between the two, the 
 *  callback should return positive integers, since the FLI status values 
 *  are zero or negative.
 */
int play_fli(char *filename, BITMAP *bmp, int loop, int (*callback)())
{
   if (open_fli(filename) != FLI_OK)
      return FLI_ERROR;

   return do_play_fli(bmp, loop, callback);
}



/* play_memory_fli:
 *  Like play_fli(), but for files which have already been loaded into 
 *  memory. Pass a pointer to the memory containing the FLI data.
 */
int play_memory_fli(void *fli_data, BITMAP *bmp, int loop, int (*callback)())
{
   if (open_memory_fli(fli_data) != FLI_OK)
      return FLI_ERROR;

   return do_play_fli(bmp, loop, callback);
}



/* do_open_fli:
 *  Worker function used by open_fli() and open_memory_fli().
 */
static int do_open_fli()
{
   long speed;

   /* read the header */
   if (!fli_read(&fli_header, sizeof(FLI_HEADER))) {
      close_fli();
      return FLI_ERROR;
   }

   /* check magic numbers */
   if (((fli_header.bits_a_pixel != 8) && (fli_header.bits_a_pixel != 0)) ||
       ((fli_header.type != FLI_MAGIC1) && (fli_header.type != FLI_MAGIC2))) {
      close_fli();
      return FLI_ERROR;
   }

   if (fli_header.width == 0)
      fli_header.width = 320;

   if (fli_header.height == 0)
      fli_header.height = 200;

   /* create the frame bitmap */
   fli_bitmap = create_bitmap_ex(8, fli_header.width, fli_header.height);
   if (!fli_bitmap) {
      close_fli();
      return FLI_ERROR;
   }

   reset_fli_variables();
   fli_frame = 0;
   fli_timer = 2;
   fli_status = FLI_OK;

   /* install the timer handler */
   LOCK_VARIABLE(fli_timer);
   LOCK_FUNCTION(fli_timer_callback);

   if (fli_header.type == FLI_MAGIC1)
      speed = BPS_TO_TIMER(70) * (long)fli_header.speed;
   else
      speed = MSEC_TO_TIMER((long)fli_header.speed);

   if (speed == 0)
      speed = BPS_TO_TIMER(70);

   install_int_ex(fli_timer_callback, speed);

   return fli_status;
}



/* open_fli:
 *  Opens an FLI file ready for playing.
 */
int open_fli(char *filename)
{
   if (fli_status != FLI_NOT_OPEN)
      return FLI_ERROR;

   FILE_OPEN(filename, fli_file);
   if (fli_file < 0) {
      fli_file = 0;
      return FLI_ERROR;
   }

   return do_open_fli();
}



/* open_memory_fli:
 *  Like open_fli(), but for files which have already been loaded into 
 *  memory. Pass a pointer to the memory containing the FLI data.
 */
int open_memory_fli(void *fli_data)
{
   if (fli_status != FLI_NOT_OPEN)
      return FLI_ERROR;

   fli_mem_data = fli_data;
   fli_mem_pos = 0;

   return do_open_fli();
}



/* close_fli:
 *  Shuts down the FLI player at the end of the file.
 */
void close_fli()
{
   remove_int(fli_timer_callback);

   if (fli_file) {
      FILE_CLOSE(fli_file);
      fli_file = 0;
   }

   if (fli_bitmap) {
      destroy_bitmap(fli_bitmap);
      fli_bitmap = NULL;
   }

   fli_mem_data = NULL;
   fli_mem_pos = 0;

   reset_fli_variables();

   fli_status = FLI_NOT_OPEN;
}



/* next_fli_frame:
 *  Advances to the next frame of the FLI, leaving the changes in the 
 *  fli_bitmap and fli_pallete. If loop is non-zero, it will cycle if 
 *  it reaches the end of the animation. Returns one of the FLI status 
 *  constants.
 */
int next_fli_frame(int loop)
{
   if (fli_status != FLI_OK)
      return fli_status;

   fli_timer--;

   /* end of file? should we loop? */
   if (fli_frame >= fli_header.frame_count) {
      if (loop) {
	 fli_seek(sizeof(FLI_HEADER), SEEK_SET);
	 fli_frame = 0;
      }
      else {
	 fli_status = FLI_EOF;
	 return fli_status;
      }
   }

   /* read the next frame */
   read_frame();

   return fli_status;
}



/* reset_fli_variables:
 *  Clears the information about which parts of the FLI bitmap and pallete
 *  are dirty, after the screen hardware has been updated.
 */
void reset_fli_variables()
{
   fli_bmp_dirty_from = INT_MAX;
   fli_bmp_dirty_to = INT_MIN;
   fli_pal_dirty_from = INT_MAX;
   fli_pal_dirty_to = INT_MIN;
}

