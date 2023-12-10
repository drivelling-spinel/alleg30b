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


#ifndef DATEDIT_H
#define DATEDIT_H


#define DAT_INFO  DAT_ID('i','n','f','o')
#define DAT_ORIG  DAT_ID('O','R','I','G')
#define DAT_DATE  DAT_ID('D','A','T','E')
#define DAT_XPOS  DAT_ID('X','P','O','S')
#define DAT_YPOS  DAT_ID('Y','P','O','S')
#define DAT_XSIZ  DAT_ID('X','S','I','Z')
#define DAT_YSIZ  DAT_ID('Y','S','I','Z')
#define DAT_PACK  DAT_ID('P','A','C','K')
#define DAT_HNAM  DAT_ID('H','N','A','M')
#define DAT_HPRE  DAT_ID('H','P','R','E')
#define DAT_BACK  DAT_ID('B','A','C','K')
#define DAT_XGRD  DAT_ID('X','G','R','D')
#define DAT_YGRD  DAT_ID('Y','G','R','D')

typedef struct OBJECT_INFO
{
   int type;
   char *desc;
   char *ext;
   char *exp_ext;
   void (*get_desc)(DATAFILE *dat, char *s);
   int (*export)(DATAFILE *dat, char *filename);
   void *(*grab)(char *filename, long *size, int x, int y, int w, int h, int depth);
   void (*save)(DATAFILE *dat, int packed, int packkids, int strip, int verbose, int extra, PACKFILE *f);
} OBJECT_INFO;

extern OBJECT_INFO object_info[];

extern PALLETE current_pallete;
extern DATAFILE info;

void datedit_msg(char *fmt, ...);
void datedit_startmsg(char *fmt, ...);
void datedit_endmsg(char *fmt, ...);
void datedit_error(char *fmt, ...);
int datedit_ask(char *fmt, ...);

char *datedit_pretty_name(char *name, char *ext, int force_ext);
int datedit_clean_typename(char *type);
void datedit_set_property(DATAFILE *dat, int type, char *value);
void datedit_find_character(BITMAP *bmp, int *x, int *y, int *w, int *h);
char *datedit_desc(DATAFILE *dat);
void datedit_sort_datafile(DATAFILE *dat);
void datedit_sort_properties(DATAFILE_PROPERTY *prop);
long datedit_asc2ftime(char *time);
char *datedit_ftime2asc(long time);
int datedit_numprop(DATAFILE *dat, int type);

DATAFILE *datedit_load_datafile(char *name, int compile_sprites, char *password);
int datedit_save_datafile(DATAFILE *dat, char *name, int strip, int pack, int verbose, int write_msg, int backup, char *password);
int datedit_save_header(DATAFILE *dat, char *name, char *headername, char *progname, char *prefix, int verbose);

void datedit_export_name(DATAFILE *dat, char *name, char *ext, char *buf);
int datedit_export(DATAFILE *dat, char *name);
DATAFILE *datedit_delete(DATAFILE *dat, int i);
int datedit_grabreplace(DATAFILE *dat, char *filename, char *name, char *type, int colordepth, int x, int y, int w, int h);
int datedit_grabupdate(DATAFILE *dat, char *filename, int x, int y, int w, int h);
DATAFILE *datedit_grabnew(DATAFILE *dat, char *filename, char *name, char *type, int colordepth, int x, int y, int w, int h);
DATAFILE *datedit_insert(DATAFILE *dat, DATAFILE **ret, char *name, int type, void *v, long size);
int datedit_update(DATAFILE *dat, int verbose, int *changed);


#endif
