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
 *      File I/O and LZSS compression routines.
 *
 *      See readme.txt for copyright information.
 */

#include <errno.h>
#include <io.h>
#include <dir.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <stdio.h>
#include <ctype.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>

#include "allegro.h"
#include "internal.h"


#define N            4096           /* 4k buffers for LZ compression */
#define F            18             /* upper limit for LZ match length */
#define THRESHOLD    2              /* LZ encode string into pos and length
				       if match size is greater than this */


typedef struct PACK_DATA            /* stuff for doing LZ compression */
{
   int state;                       /* where have we got to in the pack? */
   int i, c, len, r, s;
   int last_match_length, code_buf_ptr;
   unsigned char mask;
   char code_buf[17];
   int match_position;
   int match_length;
   int lson[N+1];                   /* left children, */
   int rson[N+257];                 /* right children, */
   int dad[N+1];                    /* and parents, = binary search trees */
   unsigned char text_buf[N+F-1];   /* ring buffer, with F-1 extra bytes
				       for string comparison */
} PACK_DATA;


typedef struct UNPACK_DATA          /* for reading LZ files */
{
   int state;                       /* where have we got to? */
   int i, j, k, r, c;
   int flags;
   unsigned char text_buf[N+F-1];   /* ring buffer, with F-1 extra bytes
				       for string comparison */
} UNPACK_DATA;


static int refill_buffer(PACKFILE *f);
static int flush_buffer(PACKFILE *f, int last);
static void pack_inittree(PACK_DATA *dat);
static void pack_insertnode(int r, PACK_DATA *dat);
static void pack_deletenode(int p, PACK_DATA *dat);
static int pack_write(PACKFILE *file, PACK_DATA *dat, int size, unsigned char *buf, int last);
static int pack_read(PACKFILE *file, UNPACK_DATA *dat, int s, unsigned char *buf);

static char thepassword[256] = "";

int _packfile_filesize = 0;
int _packfile_datasize = 0;


#define FA_DAT_FLAGS  (FA_RDONLY | FA_ARCH)



/* get_filename:
 *  When passed a completely specified file path, this returns a pointer
 *  to the filename portion. Both '\' and '/' are recognized as directory
 *  separators.
 */
char *get_filename(char *path)
{
   int pos;

   for (pos=0; path[pos]; pos++)
      ; /* do nothing */ 

   while ((pos>0) && (path[pos-1] != '\\') && (path[pos-1] != '/'))
      pos--;

   return path+pos;
}



/* get_extension:
 *  When passed a complete filename (with or without path information)
 *  this returns a pointer to the file extension.
 */
char *get_extension(char *filename)
{
   int pos, end;

   for (end=0; filename[end]; end++)
      ; /* do nothing */

   pos = end;

   while ((pos>0) && (filename[pos-1] != '.') &&
	  (filename[pos-1] != '\\') && (filename[pos-1] != '/') &&
	  (filename[pos-1] != '#'))
      pos--;

   if (filename[pos-1] == '.')
      return filename+pos;

   return filename+end;
}



/* put_backslash:
 *  If the last character of the filename is not a '\' or '/', this routine
 *  will concatenate a '\' on to it.
 */
void put_backslash(char *filename)
{
   int i = strlen(filename);

   if (i>0)
      if ((filename[i-1]=='\\') || (filename[i-1]=='/'))
	 return;

   filename[i++] = '\\';
   filename[i] = 0;
}



/* check_floppy_drive:
 *  Helper function to check if it is safe to access a file on a floppy
 *  drive. This is needed to prevent accessing files on disks A: or B:
 *  when the drive doesn't exist, because that would make DOS bring up 
 *  the "swap disks" message, which is a Bad Thing when Allegro is in
 *  control of the keyboard.
 */
static int check_floppy_drive(char *filename)
{
   char ch = tolower(filename[0]);
   __dpmi_regs r;

   if (((ch == 'a') || (ch == 'b')) && (filename[1] == ':')) {
      r.x.ax = 0x440E;
      r.x.bx = 1;
      __dpmi_int(0x21, &r);

      if ((r.h.al != 0) && (r.h.al != (ch - 'a' + 1))) {
	 errno = EACCES;
	 return FALSE;
      }
   }

   return TRUE;
}



/* pack_fopen_exe_file:
 *  Helper to handle opening files that have been appended to the end of
 *  the program executable.
 */
static PACKFILE *pack_fopen_exe_file()
{
   PACKFILE *f;
   long size;

   /* open the file */
   f = pack_fopen(__crt0_argv[0], F_READ);
   if (!f)
      return NULL;

   /* seek to the end and check for the magic number */
   pack_fseek(f, f->todo-8);

   if (pack_mgetl(f) != F_EXE_MAGIC) {
      pack_fclose(f);
      errno = ENOTDIR;
      return NULL;
   }

   size = pack_mgetl(f);

   /* rewind */
   pack_fclose(f);
   f = pack_fopen(__crt0_argv[0], F_READ);
   if (!f)
      return NULL;

   /* seek to the start of the appended data */
   pack_fseek(f, f->todo-size);

   return pack_fopen_chunk(f, FALSE);
}



/* pack_fopen_datafile_object:
 *  Helper to handle opening member objects from datafiles, given a 
 *  fake filename in the form 'filename.dat#object_name'.
 */
static PACKFILE *pack_fopen_datafile_object(char *fname, char *objname)
{
   PACKFILE *f;
   int type, size;
   int use_next = FALSE;
   char buf[256];

   /* open the file */
   f = pack_fopen(fname, F_READ_PACKED);
   if (!f)
      return NULL;

   type = pack_mgetl(f);

   if (type != DAT_MAGIC) {
      pack_fclose(f);
      errno = ENOTDIR;
      return NULL;
   }

   pack_mgetl(f);

   /* search for the requested object */
   while (!pack_feof(f)) {
      type = pack_mgetl(f);

      if (type == DAT_PROPERTY) {
	 type = pack_mgetl(f);
	 size = pack_mgetl(f);
	 if (type == DAT_ID('N','A','M','E')) {
	    /* examine name property */
	    pack_fread(buf, size, f);
	    buf[size] = 0;
	    if (stricmp(buf, objname) == 0)
	       use_next = TRUE;
	 }
	 else {
	    /* skip property */
	    pack_fseek(f, size);
	 }
      }
      else {
	 if (use_next) {
	    /* found it! */
	    return pack_fopen_chunk(f, FALSE);
	 }
	 else {
	    /* skip unwanted object */
	    size = pack_mgetl(f);
	    pack_fseek(f, size+4);
	 }
      }
   }

   /* oh dear, the object isn't there... */
   pack_fclose(f);
   errno = ENOENT;
   return NULL; 
}



/* pack_fopen_special_file:
 *  Helper to handle opening psuedo-files, ie. datafile objects and data
 *  that has been appended to the end of the executable.
 */
static PACKFILE *pack_fopen_special_file(char *filename, char *mode)
{
   char fname[256], objname[256];
   char *p;

   /* special files are read-only */
   while (*mode) {
      if ((*mode == 'w') || (*mode == 'W')) {
	 errno = EROFS;
	 return NULL;
      }
      mode++;
   }

   if (strcmp(filename, "#") == 0) {
      /* read appended executable data */
      return pack_fopen_exe_file();
   }
   else {
      if (filename[0] == '#') {
	 /* read object from an appended datafile */
	 strcpy(fname, "#");
	 strcpy(objname, filename+1);
      }
      else {
	 /* read object from a regular datafile */
	 strcpy(fname, filename);
	 p = strchr(fname, '#');
	 *p = 0;
	 strcpy(objname, p+1);
      }

      return pack_fopen_datafile_object(fname, objname);
   }
}



/* file_exists:
 *  Checks whether a file matching the given name and attributes exists,
 *  returning non zero if it does. The file attribute may contain any of
 *  the FA_* constants from dir.h. If aret is not null, it will be set 
 *  to the attributes of the matching file. If an error occurs the system 
 *  error code will be stored in errno.
 */
int file_exists(char *filename, int attrib, int *aret)
{
   FILE_SEARCH_STRUCT dta;

   if (strchr(filename, '#')) {
      PACKFILE *f = pack_fopen_special_file(filename, F_READ);
      if (f) {
	 pack_fclose(f);
	 if (aret)
	    *aret = FA_DAT_FLAGS;
	 return ((attrib & FA_DAT_FLAGS) == FA_DAT_FLAGS) ? -1 : 0;
      }
      else
	 return 0;
   }

   if (!check_floppy_drive(filename))
      return 0;

   errno = FILE_FINDFIRST(filename, attrib, &dta);

   if (aret)
      *aret = dta.FILE_ATTRIB;

   return ((errno) ? 0 : -1);
}



/* exists:
 *  Shortcut version of file_exists().
 */
int exists(char *filename)
{
   return file_exists(filename, FA_ARCH | FA_RDONLY, NULL);
}



/* file_size:
 *  Returns the size of a file, in bytes.
 *  If the file does not exist or an error occurs, it will return zero
 *  and store the system error code in errno.
 */
long file_size(char *filename)
{
   FILE_SEARCH_STRUCT dta;

   if (strchr(filename, '#')) {
      PACKFILE *f = pack_fopen_special_file(filename, F_READ);
      if (f) {
	 long ret = f->todo;
	 pack_fclose(f);
	 return ret;
      }
      else
	 return 0;
   }

   if (!check_floppy_drive(filename))
      return 0;

   errno = FILE_FINDFIRST(filename, FA_RDONLY | FA_HIDDEN | FA_ARCH, &dta);

   if (errno)
      return 0;
   else
      return dta.FILE_SIZE;
}



/* file_time:
 *  Returns a file time-stamp.
 *  If the file does not exist or an error occurs, it will return zero
 *  and store the system error code in errno.
 */
long file_time(char *filename)
{
   FILE_SEARCH_STRUCT dta;

   if (strchr(filename, '#')) {
      errno = ENOSYS;
      return 0;
   }

   if (!check_floppy_drive(filename))
      return 0;

   errno = FILE_FINDFIRST(filename, FA_RDONLY | FA_HIDDEN | FA_ARCH, &dta);

   if (errno)
      return 0L;
   else
      return (((long)dta.FILE_DATE << 16) | (long)dta.FILE_TIME);
}



/* delete_file:
 *  Removes a file from the disk.
 */
int delete_file(char *filename)
{
   if (strchr(filename, '#')) {
      errno = EROFS;
      return errno;
   }

   if (!check_floppy_drive(filename))
      return errno;

   unlink(filename);
   return errno;
}



/* for_each_file:
 *  Finds all the files on the disk which match the given wildcard
 *  specification and file attributes, and executes callback() once for
 *  each. callback() will be passed three arguments, the first a string
 *  which contains the completed filename, the second being the attributes
 *  of the file, and the third an int which is simply a copy of param (you
 *  can use this for whatever you like). If an error occurs an error code
 *  will be stored in errno, and callback() can cause for_each_file() to
 *  abort by setting errno itself. Returns the number of successful calls
 *  made to callback(). The file attribute may contain any of the FA_* 
 *  flags from dir.h.
 */
int for_each_file(char *name, int attrib, void (*callback)(), int param)
{
   char buf[256];
   FILE_SEARCH_STRUCT dta;
   int c = 0;

   if (strchr(name, '#')) {
      errno = ENOTDIR;
      return 0;
   }

   if (!check_floppy_drive(name))
      return 0;

   errno = FILE_FINDFIRST(name, attrib, &dta);
   if (errno != 0)
      return 0;

   do {
      strcpy(buf, name);
      strcpy(get_filename(buf), dta.FILE_NAME);
      (*callback)(buf, dta.FILE_ATTRIB, param);
      if (errno != 0)
	 break;
      c++;
   } while ((errno = FILE_FINDNEXT(&dta)) == 0);

   errno = 0;
   return c;
}



/* packfile_password:
 *  Sets the password to be used by all future read/write operations.
 */
void packfile_password(char *password)
{
   if (password) {
      strncpy(thepassword, password, 255);
      thepassword[255] = 0;
   }
   else
      thepassword[0] = 0;
}



/* encrypt:
 *  Helper for encrypting magic numbers, using the current password.
 */
static long encrypt(long x)
{
   long mask = 0;
   int i;

   for (i=0; thepassword[i]; i++)
      mask ^= ((long)thepassword[i] << ((i&3) * 8));

   return x ^ mask;
}



/* pack_fopen:
 *  Opens a file according to mode, which may contain any of the flags:
 *  'r': open file for reading.
 *  'w': open file for writing, overwriting any existing data.
 *  'p': open file in 'packed' mode. Data will be compressed as it is
 *       written to the file, and automatically uncompressed during read
 *       operations. Files created in this mode will produce garbage if
 *       they are read without this flag being set.
 *  '!': open file for writing in normal, unpacked mode, but add the value
 *       F_NOPACK_MAGIC to the start of the file, so that it can be opened
 *       in packed mode and Allegro will automatically detect that the
 *       data does not need to be decompressed.
 *
 *  Instead of these flags, one of the constants F_READ, F_WRITE,
 *  F_READ_PACKED, F_WRITE_PACKED or F_WRITE_NOPACK may be used as the second 
 *  argument to fopen().
 *
 *  On success, fopen() returns a pointer to a file structure, and on error
 *  it returns NULL and stores an error code in errno. An attempt to read a 
 *  normal file in packed mode will cause errno to be set to EDOM.
 */
PACKFILE *pack_fopen(char *filename, char *mode)
{
   PACKFILE *f, *f2;
   FILE_SEARCH_STRUCT dta;
   int c;
   long header = FALSE;

   if (strchr(filename, '#'))
      return pack_fopen_special_file(filename, mode);

   if (!check_floppy_drive(filename))
      return NULL;

   errno = 0;

   if ((f = malloc(sizeof(PACKFILE))) == NULL) {
      errno = ENOMEM;
      return NULL;
   }

   f->buf_pos = f->buf;
   f->flags = 0;
   f->buf_size = 0;
   f->filename = NULL;
   f->password = thepassword;

   for (c=0; mode[c]; c++) {
      switch (mode[c]) {
	 case 'r': case 'R': f->flags &= ~PACKFILE_FLAG_WRITE; break;
	 case 'w': case 'W': f->flags |= PACKFILE_FLAG_WRITE; break;
	 case 'p': case 'P': f->flags |= PACKFILE_FLAG_PACK; break;
	 case '!': f->flags &= ~PACKFILE_FLAG_PACK; header = TRUE; break;
      }
   }

   if (f->flags & PACKFILE_FLAG_WRITE) {
      if (f->flags & PACKFILE_FLAG_PACK) {
	 /* write a packed file */
	 PACK_DATA *dat = malloc(sizeof(PACK_DATA));
	 if (!dat) {
	    errno = ENOMEM;
	    free(f);
	    return NULL;
	 }
	 if ((f->parent = pack_fopen(filename, F_WRITE)) == NULL) {
	    free(dat);
	    free(f);
	    return NULL;
	 }
	 pack_mputl(encrypt(F_PACK_MAGIC), f->parent);
	 f->todo = 4;
	 for (c=0; c < N - F; c++)
	    dat->text_buf[c] = 0; 
	 dat->state = 0;
	 f->pack_data = dat;
      }
      else {
	 /* write a 'real' file */
	 f->parent = NULL;
	 f->pack_data = NULL;

	 FILE_CREATE(filename, f->hndl);
	 if (f->hndl < 0) {
	    free(f);
	    return NULL;
	 }
	 errno = 0;
	 f->todo = 0;
      }
      if (header)
	 pack_mputl(encrypt(F_NOPACK_MAGIC), f); 
   }
   else {                        /* must be a read */
      if (f->flags & PACKFILE_FLAG_PACK) {
	 /* read a packed file */
	 UNPACK_DATA *dat = malloc(sizeof(UNPACK_DATA));
	 if (!dat) {
	    errno = ENOMEM;
	    free(f);
	    return NULL;
	 }
	 if ((f->parent = pack_fopen(filename, F_READ)) == NULL) {
	    free(dat);
	    free(f);
	    return NULL;
	 }
	 header = pack_mgetl(f->parent);
	 if (header == encrypt(F_PACK_MAGIC)) {
	    for (c=0; c < N - F; c++)
	       dat->text_buf[c] = 0; 
	    dat->state = 0;
	    f->todo = LONG_MAX;
	    f->pack_data = (char *)dat;
	 }
	 else {
	    if (header == encrypt(F_NOPACK_MAGIC)) {
	       f2 = f->parent;
	       free(dat);
	       free(f);
	       return f2;
	    }
	    else {
	       pack_fclose(f->parent);
	       free(dat);
	       free(f);
	       if (errno == 0)
		  errno = EDOM;
	       return NULL;
	    }
	 }
      }
      else {
	 /* read a 'real' file */
	 f->parent = NULL;
	 f->pack_data = NULL;
	 errno = FILE_FINDFIRST(filename, FA_RDONLY | FA_HIDDEN | FA_ARCH, &dta);
	 if (errno != 0) {
	    free(f);
	    return NULL;
	 }
	 f->todo = dta.FILE_SIZE;

	 FILE_OPEN(filename, f->hndl);
	 if (f->hndl < 0) {
	    errno = f->hndl;
	    free(f);
	    return NULL;
	 }
      }
   }
   return f;
}



/* pack_fclose:
 *  Closes a file after it has been read or written.
 *  Returns zero on success. On error it returns an error code which is
 *  also stored in errno. This function can fail only when writing to
 *  files: if the file was opened in read mode it will always succeed.
 */
int pack_fclose(PACKFILE *f)
{
   if (f) {
      if (f->flags & PACKFILE_FLAG_WRITE) {
	 if (f->flags & PACKFILE_FLAG_CHUNK)
	    return pack_fclose(pack_fclose_chunk(f));

	 flush_buffer(f, TRUE);
      }

      if (f->pack_data)
	 free(f->pack_data);

      if (f->parent)
	 pack_fclose(f->parent);
      else
	 FILE_CLOSE(f->hndl);

      free(f);
      return errno;
   }

   return 0;
}



/* pack_fseek:
 *  Like the stdio fseek() function, but only supports forward seeks 
 *  relative to the current file position.
 */
int pack_fseek(PACKFILE *f, int offset)
{
   int i;

   if (f->flags & PACKFILE_FLAG_WRITE)
      return -1;

   errno = 0;

   /* skip forward through the buffer */
   if (f->buf_size > 0) {
      i = MIN(offset, f->buf_size);
      f->buf_size -= i;
      f->buf_pos += i;
      offset -= i;
      if ((f->buf_size <= 0) && (f->todo <= 0))
	 f->flags |= PACKFILE_FLAG_EOF;
   }

   /* need to seek some more? */
   if (offset > 0) {
      i = MIN(offset, f->todo);

      if (f->flags & PACKFILE_FLAG_PACK) {
	 /* for compressed files, we just have to read through the data */
	 while (i > 0) {
	    pack_getc(f);
	    i--;
	 }
      }
      else {
	 if (f->parent) {
	    /* pass the seek request on to the parent file */
	    pack_fseek(f->parent, i);
	 }
	 else {
	    /* do a real seek */
	    lseek(f->hndl, i, SEEK_CUR);
	 }
	 f->todo -= i;
	 if (f->todo <= 0)
	    f->flags |= PACKFILE_FLAG_EOF;
      }
   }

   return errno;
}



/* pack_fopen_chunk: 
 *  Opens a sub-chunk of the specified file, for reading or writing depending
 *  on the type of the file. The returned file pointer describes the sub
 *  chunk, and replaces the original file, which will no longer be valid.
 *  When writing to a chunk file, data is sent to the original file, but
 *  is prefixed with two length counts (32 bit, big-endian). For uncompressed 
 *  chunks these will both be set to the length of the data in the chunk.
 *  For compressed chunks, created by setting the pack flag, the first will
 *  contain the raw size of the chunk, and the second will be the negative
 *  size of the uncompressed data. When reading chunks, the pack flag is
 *  ignored, and the compression type is detected from the sign of the
 *  second size value. The file structure used to read chunks checks the
 *  chunk size, and will return EOF if you try to read past the end of
 *  the chunk. If you don't read all of the chunk data, when you call
 *  pack_fclose_chunk(), the parent file will advance past the unused data.
 *  When you have finished reading or writing a chunk, you should call
 *  pack_fclose_chunk() to return to your original file.
 */
PACKFILE *pack_fopen_chunk(PACKFILE *f, int pack)
{
   int c;
   char *name;
   PACKFILE *chunk;

   if (f->flags & PACKFILE_FLAG_WRITE) {
      /* write a sub-chunk */ 
      name = tmpnam(NULL);
      chunk = pack_fopen(name, (pack ? F_WRITE_PACKED : F_WRITE_NOPACK));

      if (chunk) {
	 chunk->filename = malloc(strlen(name) + 1);
	 strcpy(chunk->filename, name);

	 if (pack)
	    chunk->parent->parent = f;
	 else
	    chunk->parent = f;

	 chunk->flags |= PACKFILE_FLAG_CHUNK;
      }
   }
   else {
      /* read a sub-chunk */
      _packfile_filesize = pack_mgetl(f);
      _packfile_datasize = pack_mgetl(f);

      if ((chunk = malloc(sizeof(PACKFILE))) == NULL) {
	 errno = ENOMEM;
	 return NULL;
      }

      chunk->buf_pos = chunk->buf;
      chunk->flags = PACKFILE_FLAG_CHUNK;
      chunk->buf_size = 0;
      chunk->filename = NULL;
      chunk->parent = f;
      chunk->password = f->password;
      f->password = thepassword;

      if (_packfile_datasize < 0) {
	 /* read a packed chunk */
	 UNPACK_DATA *dat = malloc(sizeof(UNPACK_DATA));
	 if (!dat) {
	    errno = ENOMEM;
	    free(chunk);
	    return NULL;
	 }
	 for (c=0; c < N - F; c++)
	    dat->text_buf[c] = 0; 
	 dat->state = 0;
	 _packfile_datasize = -_packfile_datasize;
	 chunk->todo = _packfile_datasize;
	 chunk->pack_data = (char *)dat;
	 chunk->flags |= PACKFILE_FLAG_PACK;
      }
      else {
	 /* read an uncompressed chunk */
	 chunk->todo = _packfile_datasize;
	 chunk->pack_data = NULL;
      }
   }

   return chunk;
}



/* pack_fclose_chunk:
 *  Call after reading or writing a sub-chunk. This closes the chunk file,
 *  and returns a pointer to the original file structure (the one you
 *  passed to pack_fopen_chunk()), to allow you to read or write data 
 *  after the chunk.
 */
PACKFILE *pack_fclose_chunk(PACKFILE *f)
{
   PACKFILE *parent = f->parent;
   PACKFILE *tmp;
   char *name = f->filename;
   int header;

   if (f->flags & PACKFILE_FLAG_WRITE) {
      /* finish writing a chunk */
      _packfile_datasize = f->todo + f->buf_size - 4;

      if (f->flags & PACKFILE_FLAG_PACK) {
	 parent = parent->parent;
	 f->parent->parent = NULL;
      }
      else
	 f->parent = NULL;

      f->flags &= ~PACKFILE_FLAG_CHUNK;
      pack_fclose(f);

      tmp = pack_fopen(name, F_READ);
      _packfile_filesize = tmp->todo - 4;
      header = pack_mgetl(tmp);

      pack_mputl(_packfile_filesize, parent);

      if (header == encrypt(F_PACK_MAGIC))
	 pack_mputl(-_packfile_datasize, parent);
      else
	 pack_mputl(_packfile_datasize, parent);

      while (!pack_feof(tmp))
	 pack_putc(pack_getc(tmp), parent);

      pack_fclose(tmp);

      delete_file(name);
      free(name);
   }
   else {
      /* finish reading a chunk */
      while (f->todo > 0)
	 pack_getc(f);

      parent->password = f->password;

      if (f->pack_data)
	 free(f->pack_data);

      free(f);
   }

   return parent;
}



/* pack_igetw:
 *  Reads a 16 bit word from a file, using intel byte ordering.
 */
int pack_igetw(PACKFILE *f)
{
   int b1, b2;

   if ((b1 = pack_getc(f)) != EOF)
      if ((b2 = pack_getc(f)) != EOF)
	 return ((b2 << 8) | b1);

   return EOF;
}



/* pack_igetl:
 *  Reads a 32 bit long from a file, using intel byte ordering.
 */
long pack_igetl(PACKFILE *f)
{
   int b1, b2, b3, b4;

   if ((b1 = pack_getc(f)) != EOF)
      if ((b2 = pack_getc(f)) != EOF)
	 if ((b3 = pack_getc(f)) != EOF)
	    if ((b4 = pack_getc(f)) != EOF)
	       return (((long)b4 << 24) | ((long)b3 << 16) |
		       ((long)b2 << 8) | (long)b1);

   return EOF;
}



/* pack_iputw:
 *  Writes a 16 bit int to a file, using intel byte ordering.
 */
int pack_iputw(int w, PACKFILE *f)
{
   int b1, b2;

   b1 = (w & 0xFF00) >> 8;
   b2 = w & 0x00FF;

   if (pack_putc(b2,f)==b2)
      if (pack_putc(b1,f)==b1)
	 return w;

   return EOF;
}



/* pack_iputw:
 *  Writes a 32 bit long to a file, using intel byte ordering.
 */
long pack_iputl(long l, PACKFILE *f)
{
   int b1, b2, b3, b4;

   b1 = (int)((l & 0xFF000000L) >> 24);
   b2 = (int)((l & 0x00FF0000L) >> 16);
   b3 = (int)((l & 0x0000FF00L) >> 8);
   b4 = (int)l & 0x00FF;

   if (pack_putc(b4,f)==b4)
      if (pack_putc(b3,f)==b3)
	 if (pack_putc(b2,f)==b2)
	    if (pack_putc(b1,f)==b1)
	       return l;

   return EOF;
}



/* pack_mgetw:
 *  Reads a 16 bit int from a file, using motorola byte-ordering.
 */
int pack_mgetw(PACKFILE *f)
{
   int b1, b2;

   if ((b1 = pack_getc(f)) != EOF)
      if ((b2 = pack_getc(f)) != EOF)
	 return ((b1 << 8) | b2);

   return EOF;
}



/* pack_mgetl:
 *  Reads a 32 bit long from a file, using motorola byte-ordering.
 */
long pack_mgetl(PACKFILE *f)
{
   int b1, b2, b3, b4;

   if ((b1 = pack_getc(f)) != EOF)
      if ((b2 = pack_getc(f)) != EOF)
	 if ((b3 = pack_getc(f)) != EOF)
	    if ((b4 = pack_getc(f)) != EOF)
	       return (((long)b1 << 24) | ((long)b2 << 16) |
		       ((long)b3 << 8) | (long)b4);

   return EOF;
}



/* pack_mputw:
 *  Writes a 16 bit int to a file, using motorola byte-ordering.
 */
int pack_mputw(int w, PACKFILE *f)
{
   int b1, b2;

   b1 = (w & 0xFF00) >> 8;
   b2 = w & 0x00FF;

   if (pack_putc(b1,f)==b1)
      if (pack_putc(b2,f)==b2)
	 return w;

   return EOF;
}



/* pack_mputl:
 *  Writes a 32 bit long to a file, using motorola byte-ordering.
 */
long pack_mputl(long l, PACKFILE *f)
{
   int b1, b2, b3, b4;

   b1 = (int)((l & 0xFF000000L) >> 24);
   b2 = (int)((l & 0x00FF0000L) >> 16);
   b3 = (int)((l & 0x0000FF00L) >> 8);
   b4 = (int)l & 0x00FF;

   if (pack_putc(b1,f)==b1)
      if (pack_putc(b2,f)==b2)
	 if (pack_putc(b3,f)==b3)
	    if (pack_putc(b4,f)==b4)
	       return l;

   return EOF;
}



/* pack_fread:
 *  Reads n bytes from f and stores them at memory location p. Returns the 
 *  number of items read, which will be less than n if EOF is reached or an 
 *  error occurs. Error codes are stored in errno.
 */
long pack_fread(void *p, long n, PACKFILE *f)
{
   long c;                       /* counter of bytes read */
   int i;
   unsigned char *cp = (unsigned char *)p;

   for (c=0; c<n; c++) {
      if (--(f->buf_size) > 0)
	 *(cp++) = *(f->buf_pos++);
      else {
	 i = _sort_out_getc(f);
	 if (i == EOF)
	    return c;
	 else
	    *(cp++) = i;
      }
   }

   return n;
}



/* pack_fwrite:
 *  Writes n bytes to the file f from memory location p. Returns the number 
 *  of items written, which will be less than n if an error occurs. Error 
 *  codes are stored in errno.
 */
long pack_fwrite(void *p, long n, PACKFILE *f)
{
   long c;                       /* counter of bytes written */
   unsigned char *cp = (unsigned char *)p;

   for (c=0; c<n; c++) {
      if (++(f->buf_size) >= F_BUF_SIZE) {
	 if (_sort_out_putc(*cp,f) != *cp)
	    return c;
	 cp++;
      }
      else
	 *(f->buf_pos++)=*(cp++);
   }

   return n;
}



/* pack_fgets:
 *  Reads a line from a text file, storing it at location p. Stops when a
 *  linefeed is encountered, or max characters have been read. Returns a
 *  pointer to where it stored the text, or NULL on error. The end of line
 *  is handled by detecting '\n' characters: '\r' is simply ignored.
 */
char *pack_fgets(char *p, int max, PACKFILE *f)
{
   int c;

   if (pack_feof(f)) {
      p[0] = 0;
      return NULL;
   }

   for (c=0; c<max-1; c++) {
      p[c] = pack_getc(f);
      if (p[c] == '\r')
	 c--;
      else if (p[c] == '\n')
	 break;
   }

   p[c] = 0;

   if (errno)
      return NULL;
   else
      return p;
}



/* pack_fputs:
 *  Writes a string to a text file, returning zero on success, -1 on error.
 *  EOL ('\n') characters are expanded to CR/LF ('\r\n') pairs.
 */
int pack_fputs(char *p, PACKFILE *f)
{
   while (*p) {
      if (*p == '\n') {
	 pack_putc('\r', f);
	 pack_putc('\n', f);
      }
      else
	 pack_putc(*p, f);

      p++;
   }

   if (errno)
      return -1;
   else
      return 0;
}



/* _sort_out_getc:
 *  Helper function for the pack_getc() macro.
 */
int _sort_out_getc(PACKFILE *f)
{
   if (f->buf_size == 0) {
      if (f->todo <= 0)
	 f->flags |= PACKFILE_FLAG_EOF;
      return *(f->buf_pos++);
   }
   return refill_buffer(f);
}



/* refill_buffer:
 *  Refills the read buffer. The file must have been opened in read mode,
 *  and the buffer must be empty.
 */
static int refill_buffer(PACKFILE *f)
{
   int sz;

   if ((f->flags & PACKFILE_FLAG_EOF) || (f->todo <= 0)) {     /* EOF */
      f->flags |= PACKFILE_FLAG_EOF;
      return EOF;
   }

   if (f->parent) {
      if (f->flags & PACKFILE_FLAG_PACK) {
	 f->buf_size = pack_read(f->parent, (UNPACK_DATA *)f->pack_data, MIN(F_BUF_SIZE, f->todo), f->buf);
      }
      else {
	 f->buf_size = pack_fread(f->buf, MIN(F_BUF_SIZE, f->todo), f->parent);
      } 
      if (f->parent->flags & PACKFILE_FLAG_EOF)
	 f->todo = 0;
      if (f->parent->flags & PACKFILE_FLAG_ERROR)
	 goto err;
   }
   else {
      f->buf_size = MIN(F_BUF_SIZE, f->todo);
      FILE_READ(f->hndl, f->buf, f->buf_size, sz);
      if (sz != f->buf_size)
	 goto err;
   }

   f->todo -= f->buf_size;
   f->buf_pos = f->buf;
   f->buf_size--;
   if (f->buf_size <= 0)
      if (f->todo <= 0)
	 f->flags |= PACKFILE_FLAG_EOF;

   return *(f->buf_pos++);

   err:
   errno=EFAULT;
   f->flags |= PACKFILE_FLAG_ERROR;
   return EOF;
}



/* _sort_out_putc:
 *  Helper function for the pack_putc() macro.
 */
int _sort_out_putc(int c, PACKFILE *f)
{
   f->buf_size--;

   if (flush_buffer(f, FALSE))
      return EOF;

   f->buf_size++;
   return (*(f->buf_pos++)=c);
}



/* flush_buffer:
 * flushes a file buffer to the disk. The file must be open in write mode.
 */
static int flush_buffer(PACKFILE *f, int last)
{
   int sz;

   if (f->buf_size > 0) {
      if (f->flags & PACKFILE_FLAG_PACK) {
	 if (pack_write(f->parent, (PACK_DATA *)f->pack_data, f->buf_size, f->buf, last))
	    goto err;
      }
      else {
	 FILE_WRITE(f->hndl, f->buf, f->buf_size, sz);
	 if (sz != f->buf_size)
	    goto err;
      }
      f->todo += f->buf_size;
   }
   f->buf_pos = f->buf;
   f->buf_size = 0;
   return 0;

   err:
   errno=EFAULT;
   f->flags |= PACKFILE_FLAG_ERROR;
   return EOF;
}



/***************************************************
 ************ LZSS compression routines ************
 ***************************************************

   This compression algorithm is based on the ideas of Lempel and Ziv,
   with the modifications suggested by Storer and Szymanski. The algorithm 
   is based on the use of a ring buffer, which initially contains zeros. 
   We read several characters from the file into the buffer, and then 
   search the buffer for the longest string that matches the characters 
   just read, and output the length and position of the match in the buffer.

   With a buffer size of 4096 bytes, the position can be encoded in 12
   bits. If we represent the match length in four bits, the <position,
   length> pair is two bytes long. If the longest match is no more than
   two characters, then we send just one character without encoding, and
   restart the process with the next letter. We must send one extra bit
   each time to tell the decoder whether we are sending a <position,
   length> pair or an unencoded character, and these flags are stored as
   an eight bit mask every eight items.

   This implementation uses binary trees to speed up the search for the
   longest match.

   Original code by Haruhiko Okumura, 4/6/1989.
   12-2-404 Green Heights, 580 Nagasawa, Yokosuka 239, Japan.

   Modified for use in the Allegro filesystem by Shawn Hargreaves.

   Use, distribute, and modify this code freely.
*/



/* pack_inittree:
 *  For i = 0 to N-1, rson[i] and lson[i] will be the right and left 
 *  children of node i. These nodes need not be initialized. Also, dad[i] 
 *  is the parent of node i. These are initialized to N, which stands for 
 *  'not used.' For i = 0 to 255, rson[N+i+1] is the root of the tree for 
 *  strings that begin with character i. These are initialized to N. Note 
 *  there are 256 trees.
 */
static void pack_inittree(PACK_DATA *dat)
{
   int i;

   for (i=N+1; i<=N+256; i++)
      dat->rson[i] = N;

   for (i=0; i<N; i++)
      dat->dad[i] = N;
}



/* pack_insertnode:
 *  Inserts a string of length F, text_buf[r..r+F-1], into one of the trees 
 *  (text_buf[r]'th tree) and returns the longest-match position and length 
 *  via match_position and match_length. If match_length = F, then removes 
 *  the old node in favor of the new one, because the old one will be 
 *  deleted sooner. Note r plays double role, as tree node and position in 
 *  the buffer. 
 */
static void pack_insertnode(int r, PACK_DATA *dat)
{
   int i, p, cmp;
   unsigned char *key;
   unsigned char *text_buf = dat->text_buf;

   cmp = 1;
   key = &text_buf[r];
   p = N + 1 + key[0];
   dat->rson[r] = dat->lson[r] = N;
   dat->match_length = 0;

   for (;;) {

      if (cmp >= 0) {
	 if (dat->rson[p] != N)
	    p = dat->rson[p];
	 else {
	    dat->rson[p] = r;
	    dat->dad[r] = p;
	    return;
	 }
      }
      else {
	 if (dat->lson[p] != N)
	    p = dat->lson[p];
	 else {
	    dat->lson[p] = r;
	    dat->dad[r] = p;
	    return;
	 }
      }

      for (i = 1; i < F; i++)
	 if ((cmp = key[i] - text_buf[p + i]) != 0)
	    break;

      if (i > dat->match_length) {
	 dat->match_position = p;
	 if ((dat->match_length = i) >= F)
	    break;
      }
   }

   dat->dad[r] = dat->dad[p];
   dat->lson[r] = dat->lson[p];
   dat->rson[r] = dat->rson[p];
   dat->dad[dat->lson[p]] = r;
   dat->dad[dat->rson[p]] = r;
   if (dat->rson[dat->dad[p]] == p)
      dat->rson[dat->dad[p]] = r;
   else
      dat->lson[dat->dad[p]] = r;
   dat->dad[p] = N;                 /* remove p */
}



/* pack_deletenode:
 *  Removes a node from a tree.
 */
static void pack_deletenode(int p, PACK_DATA *dat)
{
   int q;

   if (dat->dad[p] == N)
      return;     /* not in tree */

   if (dat->rson[p] == N)
      q = dat->lson[p];
   else
      if (dat->lson[p] == N)
	 q = dat->rson[p];
      else {
	 q = dat->lson[p];
	 if (dat->rson[q] != N) {
	    do {
	       q = dat->rson[q];
	    } while (dat->rson[q] != N);
	    dat->rson[dat->dad[q]] = dat->lson[q];
	    dat->dad[dat->lson[q]] = dat->dad[q];
	    dat->lson[q] = dat->lson[p];
	    dat->dad[dat->lson[p]] = q;
	 }
	 dat->rson[q] = dat->rson[p];
	 dat->dad[dat->rson[p]] = q;
      }

   dat->dad[q] = dat->dad[p];
   if (dat->rson[dat->dad[p]] == p)
      dat->rson[dat->dad[p]] = q;
   else
      dat->lson[dat->dad[p]] = q;

   dat->dad[p] = N;
}



/* pack_write:
 *  Called by flush_buffer(). Packs size bytes from buf, using the pack 
 *  information contained in dat. Returns 0 on success.
 */
static int pack_write(PACKFILE *file, PACK_DATA *dat, int size, unsigned char *buf, int last)
{
   int i = dat->i;
   int c = dat->c;
   int len = dat->len;
   int r = dat->r;
   int s = dat->s;
   int last_match_length = dat->last_match_length;
   int code_buf_ptr = dat->code_buf_ptr;
   unsigned char mask = dat->mask;
   int ret = 0;

   if (dat->state==2)
      goto pos2;
   else
      if (dat->state==1)
	 goto pos1;

   dat->code_buf[0] = 0;
      /* code_buf[1..16] saves eight units of code, and code_buf[0] works
	 as eight flags, "1" representing that the unit is an unencoded
	 letter (1 byte), "0" a position-and-length pair (2 bytes).
	 Thus, eight units require at most 16 bytes of code. */

   code_buf_ptr = mask = 1;

   s = 0;
   r = N - F;
   pack_inittree(dat);

   for (len=0; (len < F) && (size > 0); len++) {
      dat->text_buf[r+len] = *(buf++);
      if (--size == 0) {
	 if (!last) {
	    dat->state = 1;
	    goto getout;
	 }
      }
      pos1:
	 ; 
   }

   if (len == 0)
      goto getout;

   for (i=1; i <= F; i++)
      pack_insertnode(r-i,dat);
	    /* Insert the F strings, each of which begins with one or
	       more 'space' characters. Note the order in which these
	       strings are inserted. This way, degenerate trees will be
	       less likely to occur. */

   pack_insertnode(r,dat);
	    /* Finally, insert the whole string just read. match_length
	       and match_position are set. */

   do {
      if (dat->match_length > len)
	 dat->match_length = len;  /* match_length may be long near the end */

      if (dat->match_length <= THRESHOLD) {
	 dat->match_length = 1;  /* not long enough match: send one byte */
	 dat->code_buf[0] |= mask;    /* 'send one byte' flag */
	 dat->code_buf[code_buf_ptr++] = dat->text_buf[r]; /* send uncoded */
      }
      else {
	 /* send position and length pair. Note match_length > THRESHOLD */
	 dat->code_buf[code_buf_ptr++] = (unsigned char) dat->match_position;
	 dat->code_buf[code_buf_ptr++] = (unsigned char)
				     (((dat->match_position >> 4) & 0xF0) |
				      (dat->match_length - (THRESHOLD + 1)));
      }

      if ((mask <<= 1) == 0) {               /* shift mask left one bit */
	 if (*file->password) {
	    dat->code_buf[0] ^= *file->password;
	    file->password++;
	    if (!*file->password)
	       file->password = thepassword;
	 };

	 for (i=0; i<code_buf_ptr; i++)         /* send at most 8 units of */
	    pack_putc(dat->code_buf[i], file);  /* code together */
	    if (pack_ferror(file)) {
	       ret = EOF;
	       goto getout;
	    }
	    dat->code_buf[0] = 0;
	    code_buf_ptr = mask = 1;
      }

      last_match_length = dat->match_length;

      for (i=0; (i < last_match_length) && (size > 0); i++) {
	 c = *(buf++);
	 if (--size == 0) {
	    if (!last) {
	       dat->state = 2;
	       goto getout;
	    }
	 }
	 pos2:
	 pack_deletenode(s,dat);    /* delete old strings and */
	 dat->text_buf[s] = c;      /* read new bytes */
	 if (s < F-1)
	    dat->text_buf[s+N] = c; /* if the position is near the end of
				       buffer, extend the buffer to make
				       string comparison easier */
	 s = (s+1) & (N-1);
	 r = (r+1) & (N-1);         /* since this is a ring buffer,
				       increment the position modulo N */

	 pack_insertnode(r,dat);    /* register the string in
				       text_buf[r..r+F-1] */
      }

      while (i++ < last_match_length) {   /* after the end of text, */
	 pack_deletenode(s,dat);          /* no need to read, but */
	 s = (s+1) & (N-1);               /* buffer may not be empty */
	 r = (r+1) & (N-1);
	 if (--len)
	    pack_insertnode(r,dat); 
      }

   } while (len > 0);   /* until length of string to be processed is zero */

   if (code_buf_ptr > 1) {         /* send remaining code */
      if (*file->password) {
	 dat->code_buf[0] ^= *file->password;
	 file->password++;
	 if (!*file->password)
	    file->password = thepassword;
      };

      for (i=0; i<code_buf_ptr; i++) {
	 pack_putc(dat->code_buf[i], file);
	 if (pack_ferror(file)) {
	    ret = EOF;
	    goto getout;
	 }
      }
   }

   dat->state = 0;

   getout:

   dat->i = i;
   dat->c = c;
   dat->len = len;
   dat->r = r;
   dat->s = s;
   dat->last_match_length = last_match_length;
   dat->code_buf_ptr = code_buf_ptr;
   dat->mask = mask;

   return ret;
}



/* pack_read:
 *  Called by refill_buffer(). Unpacks from dat into buf, until either
 *  EOF is reached or s bytes have been extracted. Returns the number of
 *  bytes added to the buffer
 */
static int pack_read(PACKFILE *file, UNPACK_DATA *dat, int s, unsigned char *buf)
{
   int i = dat->i;
   int j = dat->j;
   int k = dat->k;
   int r = dat->r;
   int c = dat->c;
   unsigned int flags = dat->flags;
   int size = 0;

   if (dat->state==2)
      goto pos2;
   else
      if (dat->state==1)
	 goto pos1;

   r = N-F;
   flags = 0;

   for ( ; ; ) {
      if (((flags >>= 1) & 256) == 0) {
	 if ((c = pack_getc(file)) == EOF)
	    break;

	 if (*file->password) {
	    c ^= *file->password;
	    file->password++;
	    if (!*file->password)
	       file->password = thepassword;
	 };

	 flags = c | 0xFF00;        /* uses higher byte to count eight */
      }

      if (flags & 1) {
	 if ((c = pack_getc(file)) == EOF)
	    break;
	 dat->text_buf[r++] = c;
	 r &= (N - 1);
	 *(buf++) = c;
	 if (++size >= s) {
	    dat->state = 1;
	    goto getout;
	 }
	 pos1:
	    ; 
      }
      else {
	 if ((i = pack_getc(file)) == EOF)
	    break;
	 if ((j = pack_getc(file)) == EOF)
	    break;
	 i |= ((j & 0xF0) << 4);
	 j = (j & 0x0F) + THRESHOLD;
	 for (k=0; k <= j; k++) {
	    c = dat->text_buf[(i + k) & (N - 1)];
	    dat->text_buf[r++] = c;
	    r &= (N - 1);
	    *(buf++) = c;
	    if (++size >= s) {
	       dat->state = 2;
	       goto getout;
	    }
	    pos2:
	       ; 
	 }
      }
   }

   dat->state = 0;

   getout:

   dat->i = i;
   dat->j = j;
   dat->k = k;
   dat->r = r;
   dat->c = c;
   dat->flags = flags;

   return size;
}


