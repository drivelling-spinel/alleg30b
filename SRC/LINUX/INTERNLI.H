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
 *      Some definitions for internal use by the library code.
 *      This should not be included by user programs.
 *
 *      See readme.txt for copyright information.
 */


#ifndef INTERNLI_H
#define INTERNLI_H

#ifndef LINUX
#error This file should only be used by the linux version of Allegro
#endif


#include <dir.h>


/* file access macros */
#define FILE_OPEN(filename, handle)             handle = open(filename, O_RDONLY | O_BINARY, S_IRUSR | S_IWUSR)
#define FILE_CREATE(filename, handle)           handle = open(filename, O_WRONLY | O_BINARY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR)
#define FILE_CLOSE(handle)                      close(handle)
#define FILE_READ(handle, buf, size, sz)        sz = read(handle, buf, size)
#define FILE_WRITE(handle, buf, size, sz)       sz = write(handle, buf, size) 
#define FILE_SEARCH_STRUCT                      struct ffblk
#define FILE_FINDFIRST(filename, attrib, dta)   findfirst(filename, dta, attrib)
#define FILE_FINDNEXT(dta)                      findnext(dta)
#define FILE_ATTRIB                             ff_attrib
#define FILE_SIZE                               ff_fsize
#define FILE_NAME                               ff_name


/* under linux, alias these to nothing */
#define DISABLE()
#define ENABLE()

#define _go32_dpmi_lock_data(addr, size) 


/* these will need to be rewritten! can probably be copied from djgpp */
#define _my_ds()                  0

#define _farsetsel(seg)
#define _farnspokeb(addr, x)
#define _farnspokew(addr, x)
#define _farnspokel(addr, x)
#define _farnspeekb(addr, x)     0
#define _farnspeekw(addr, x)     0
#define _farnspeekl(addr, x)     0



#endif          /* ifndef INTERNLI_H */
