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
 *      Datafile archiving utility for the Allegro library.
 *
 *      See readme.txt for copyright information.
 */


#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <dir.h>
#include <time.h>
#include <conio.h>

#include "allegro.h"
#include "datedit.h"


DATAFILE *datafile = NULL;

int err = 0;
int changed = 0;

int opt_command = 0;
int opt_compression = -1;
int opt_strip = -1;
int opt_verbose = FALSE;
int opt_keepnames = FALSE;
int opt_colordepth = -1;
int opt_gridx = -1;
int opt_gridy = -1;
int opt_gridw = -1;
int opt_gridh = -1;
char *opt_datafile = NULL;
char *opt_outputname = NULL;
char *opt_headername = NULL;
char *opt_dependencyfile = NULL;
char *opt_objecttype = NULL;
char *opt_prefixstring = NULL;
char *opt_password = NULL;
char *opt_palette = NULL;

#define MAX_FILES    256

char *opt_proplist[MAX_FILES];
int opt_numprops = 0;

char *opt_namelist[MAX_FILES];
int opt_usedname[MAX_FILES];
int opt_numnames = 0;


void usage() __attribute__ ((noreturn));


char **__crt0_glob_function(char *_arg)
{
   /* don't let djgpp glob our command line arguments */
   return NULL;
}



/* display help on the command syntax */
void usage()
{
   static char *msg[] =
   {
      "",
      "Datafile archiving utility for Allegro " ALLEGRO_VERSION_STR,
      "By Shawn Hargreaves, " ALLEGRO_DATE_STR,
      "",
      "Usage: dat [options] filename.dat [names]",
      "",
      "Options:",
      "\t'-a' adds the named files to the datafile",
      "\t'-bpp colordepth' grabs bitmaps in the specified format",
      "\t'-c0' no compression",
      "\t'-c1' compress objects individually",
      "\t'-c2' global compression on the entire datafile",
      "\t'-d' deletes the named objects from the datafile",
      "\t'-e' extracts the named objects from the datafile",
      "\t'-g x y w h' grabs bitmap data from a specific grid location",
      "\t'-h outputfile.h' sets the output header file",
      "\t'-k' keeps the original file names when grabbing objects",
      "\t'-l' lists the contents of the datafile",
      "\t'-m dependencyfile' outputs makefile dependencies",
      "\t'-o output' sets the output file or directory when extracting data",
      "\t'-p prefixstring' sets the prefix for the output header file",
      "\t'-pal objectname' specifies which palette to use",
      "\t'-s0' no strip: save everything",
      "\t'-s1' strip grabber specific information from the file",
      "\t'-s2' strip all object properties and names from the file",
      "\t'-t type' sets the object type when adding files",
      "\t'-u' updates the contents of the datafile",
      "\t'-v' selects verbose mode",
      "\t'-007 password' sets the file encryption key",
      "\t'PROP=value' sets object properties",
      NULL
   };

   int i;

   for (i=0; msg[i]; i++)
      fprintf(stderr, "%s\n", msg[i]);

   exit(1);
}



/* callback for outputting messages */
void datedit_msg(char *fmt, ...)
{
   va_list args;
   char buf[1024];

   va_start(args, fmt);
   vsprintf(buf, fmt, args);
   va_end(args);

   printf("%s\n", buf);
}



/* callback for starting a 2-part message output */
void datedit_startmsg(char *fmt, ...)
{
   va_list args;
   char buf[1024];

   va_start(args, fmt);
   vsprintf(buf, fmt, args);
   va_end(args);

   printf("%s", buf);
   fflush(stdout);
}



/* callback for ending a 2-part message output */
void datedit_endmsg(char *fmt, ...)
{
   va_list args;
   char buf[1024];

   va_start(args, fmt);
   vsprintf(buf, fmt, args);
   va_end(args);

   printf("%s\n", buf);
}



/* callback for printing errors */
void datedit_error(char *fmt, ...)
{
   va_list args;
   char buf[1024];

   va_start(args, fmt);
   vsprintf(buf, fmt, args);
   va_end(args);

   fprintf(stderr, "%s\n", buf);

   err = 1;
}



/* callback for asking questions */
int datedit_ask(char *fmt, ...)
{
   va_list args;
   char buf[1024];
   int c;

   static int all = FALSE;

   if (all)
      return 'y';

   va_start(args, fmt);
   vsprintf(buf, fmt, args);
   va_end(args);

   printf("%s? (y/n/a)", buf);
   fflush(stdout);

   for (;;) {
      c = getch() & 0xFF;

      switch (c) {

	 case 'y':
	 case 'Y':
	    printf("%c\n", c);
	    return 'y';

	 case 'n':
	 case 'N':
	    printf("%c\n", c);
	    return 'n';

	 case 'a':
	 case 'A':
	    printf("%c\n", c);
	    all = TRUE;
	    return 'y';

	 case 27:
	    printf("\n");
	    return 27;
      }
   }
}



/* checks if a string is one of the names specified on the command line */
int is_name(char *name)
{
   int i;

   for (i=0; i<opt_numnames; i++) {
      if ((stricmp(name, opt_namelist[i]) == 0) ||
	  (strcmp(opt_namelist[i], "*") == 0)) {
	 opt_usedname[i] = TRUE;
	 return TRUE;
      }
   }

   return FALSE;
}



/* does a view operation */
void do_view(DATAFILE *dat, char *parentname)
{
   int i, j;
   char *name;
   DATAFILE_PROPERTY *prop;
   char tmp[256];

   for (i=0; dat[i].type != DAT_END; i++) {
      name = get_datafile_property(dat+i, DAT_NAME);
      strcpy(tmp, parentname);
      strcat(tmp, name);

      if ((opt_numnames <= 0) || (is_name(tmp))) {
	 printf("- %c%c%c%c - %s%-*s - %s\n", 
		  (dat[i].type>>24)&0xFF, (dat[i].type>>16)&0xFF,
		  (dat[i].type>>8)&0xFF, dat[i].type&0xFF,
		  parentname, 28-(int)strlen(parentname), name, 
		  datedit_desc(dat+i));

	 if (opt_verbose) {
	    prop = dat[i].prop;
	    if (prop) {
	       for (j=0; prop[j].type != DAT_END; j++) {
		  printf("  . %c%c%c%c '%s'\n", 
			   (prop[j].type>>24)&0xFF, (prop[j].type>>16)&0xFF,
			   (prop[j].type>>8)&0xFF, prop[j].type&0xFF,
			   prop[j].dat);
	       }
	    }
	    printf("\n");
	 }
      }

      if (dat[i].type == DAT_FILE) {
	 strcat(tmp, "/");
	 do_view((DATAFILE *)dat[i].dat, tmp);
      }
   }
}



/* does an export operation */
void do_export(DATAFILE *dat, char *parentname)
{
   int i;
   char *name;
   char tmp[256];

   for (i=0; dat[i].type != DAT_END; i++) {
      name = get_datafile_property(dat+i, DAT_NAME);
      strcpy(tmp, parentname);
      strcat(tmp, name);

      if (is_name(tmp)) {
	 if (!datedit_export(dat+i, opt_outputname)) {
	    err = 1;
	    return;
	 }
      }
      else {
	 if (dat[i].type == DAT_FILE) {
	    strcat(tmp, "/");
	    do_export((DATAFILE *)dat[i].dat, tmp);
	    if (err)
	       return;
	 }
      }
   }
}



/* deletes objects from the datafile */
void do_delete(DATAFILE **dat, char *parentname)
{
   int i;
   char *name;
   char tmp[256];

   for (i=0; (*dat)[i].type != DAT_END; i++) {
      name = get_datafile_property((*dat)+i, DAT_NAME);
      strcpy(tmp, parentname);
      strcat(tmp, name);

      if (is_name(tmp)) {
	 printf("deleting %s\n", tmp);
	 *dat = datedit_delete(*dat, i);
	 changed = TRUE;
	 i--;
      }
      else {
	 if ((*dat)[i].type == DAT_FILE) {
	    strcat(tmp, "/");
	    do_delete((DATAFILE **)(&((*dat)[i].dat)), tmp);
	 }
      }
   }
}



/* adds a file to the archive */
void do_add_file(char *filename, int attrib, int param)
{
   char fname[256];
   char name[80];
   int c;
   DATAFILE *d;

   strcpy(fname, filename);
   strlwr(fname);

   strcpy(name, get_filename(fname));

   if (!opt_keepnames) {
      strupr(name);

      for (c=0; name[c]; c++)
	 if (name[c] == '.')
	    name[c] = '_';
   }

   for (c=0; datafile[c].type != DAT_END; c++) {
      if (stricmp(name, get_datafile_property(datafile+c, DAT_NAME)) == 0) {
	 printf("Replacing %s -> %s\n", fname, name);
	 if (!datedit_grabreplace(datafile+c, fname, name, opt_objecttype, opt_colordepth, opt_gridx, opt_gridy, opt_gridw, opt_gridh))
	    errno = err = 1;
	 else
	    changed = TRUE;
	 return;
      }
   }

   printf("Inserting %s -> %s\n", fname, name);
   d = datedit_grabnew(datafile, fname, name, opt_objecttype, opt_colordepth, opt_gridx, opt_gridy, opt_gridw, opt_gridh);
   if (!d)
      errno = err = 1;
   else {
      datafile = d;
      changed = TRUE;
   }
}



/* does an update operation */
void do_update(DATAFILE *dat, char *parentname)
{
   int i;
   char *name;
   char tmp[256];

   for (i=0; dat[i].type != DAT_END; i++) {
      name = get_datafile_property(dat+i, DAT_NAME);
      strcpy(tmp, parentname);
      strcat(tmp, name);

      if (dat[i].type == DAT_FILE) {
	 strcat(tmp, "/");
	 do_update((DATAFILE *)dat[i].dat, tmp);
	 if (err)
	    return;
      }
      else if ((opt_numnames <= 0) || (is_name(tmp))) {
	 if (!datedit_update(dat+i, opt_verbose, &changed)) {
	    err = 1;
	    return;
	 }
      }
   }
}



/* changes object properties */
void do_set_props(DATAFILE *dat, char *parentname)
{
   int i, j;
   char *name;
   char tmp[256];
   char propname[256], *propvalue;
   int type;

   for (i=0; dat[i].type != DAT_END; i++) {
      name = get_datafile_property(dat+i, DAT_NAME);
      strcpy(tmp, parentname);
      strcat(tmp, name);

      if (is_name(tmp)) {
	 for (j=0; j<opt_numprops; j++) {
	    strcpy(propname, opt_proplist[j]);
	    propvalue = strchr(propname, '=');
	    if (propvalue) {
	       *propvalue = 0;
	       propvalue++;
	    }
	    else
	       propvalue = "";

	    type = datedit_clean_typename(propname);

	    if (opt_verbose) {
	       if (*propvalue) {
		  printf("%s: setting property %c%c%c%c = '%s'\n", 
			 tmp, type>>24, (type>>16)&0xFF, 
			 (type>>8)&0xFF, type&0xFF, propvalue);
	       }
	       else {
		  printf("%s: clearing property %c%c%c%c\n", 
			 tmp, type>>24, (type>>16)&0xFF, 
			 (type>>8)&0xFF, type&0xFF);
	       }
	    } 

	    datedit_set_property(dat+i, type, propvalue);
	    changed = TRUE;
	 }
      }

      if (dat[i].type == DAT_FILE) {
	 strcat(tmp, "/");
	 do_set_props((DATAFILE *)dat[i].dat, tmp);
      }
   }
}



/* selects a specific palette */
void do_setpal(DATAFILE **dat, char *parentname)
{
   int i;
   char *name;
   char tmp[256];

   for (i=0; (*dat)[i].type != DAT_END; i++) {
      name = get_datafile_property((*dat)+i, DAT_NAME);
      strcpy(tmp, parentname);
      strcat(tmp, name);

      if (stricmp(tmp, opt_palette) == 0) {
	 if ((*dat)[i].type != DAT_PALETTE) {
	    printf("Error: %s is not a palette object\n", tmp);
	    err = 1;
	    return;
	 }
	 printf("using palette %s\n", tmp);
	 memcpy(current_pallete, (*dat)[i].dat, sizeof(PALLETE));
	 select_pallete(current_pallete);
	 return;
      }
      else {
	 if ((*dat)[i].type == DAT_FILE) {
	    strcat(tmp, "/");
	    do_setpal((DATAFILE **)(&((*dat)[i].dat)), tmp);
	 }
      }
   }

   printf("Error: %s not found\n", opt_palette);
   err = 1;
}



/* recursive helper for writing out datafile dependencies */
void save_dependencies(DATAFILE *dat, FILE *f, int *depth)
{
   char *orig;
   int c, i;
   int hasspace;

   for (c=0; dat[c].type != DAT_END; c++) {
      orig = get_datafile_property(dat+c, DAT_ORIG);

      if ((orig) && (orig[0])) {
	 if (*depth + strlen(orig) > 56) {
	    fprintf(f, " \\\n\t\t");
	    *depth = 0;
	 }
	 else {
	    fprintf(f, " ");
	    (*depth)++;
	 }

	 if (strchr(orig, ' ')) {
	    hasspace = TRUE;
	    fputc('"', f);
	    (*depth) += 2;
	 }
	 else
	    hasspace = FALSE;

	 for (i=0; orig[i]; i++) {
	    if (orig[i] == '\\')
	       fputc('/', f);
	    else
	       fputc(orig[i], f);
	    (*depth)++;
	 }

	 if (hasspace)
	    fputc('"', f);
      }

      if (dat[c].type == DAT_FILE)
	 save_dependencies((DATAFILE *)dat[c].dat, f, depth);
   }
}



/* writes out a makefile dependency rule */
int do_save_dependencies(DATAFILE *dat, char *srcname, char *depname)
{
   char *pretty_name;
   char tm[80];
   time_t now;
   FILE *f;
   int c;

   pretty_name = datedit_pretty_name(srcname, "dat", FALSE);
   datedit_msg("Writing makefile dependencies into %s", depname);

   f = fopen(depname, "w");
   if (f) {
      time(&now);
      strcpy(tm, asctime(localtime(&now)));
      for (c=0; tm[c]; c++)
	 if ((tm[c] == '\r') || (tm[c] == '\n'))
	    tm[c] = 0;

      fprintf(f, "# Allegro datafile make dependencies, produced by dat v" ALLEGRO_VERSION_STR "\n");
      fprintf(f, "# Datafile: %s\n", pretty_name);
      fprintf(f, "# Date: %s\n", tm);
      fprintf(f, "# Do not hand edit!\n\n");

      if (opt_headername)
	 fprintf(f, "%s %s :", pretty_name, opt_headername);
      else
	 fprintf(f, "%s :", pretty_name);

      c = 0xFF;
      save_dependencies(dat, f, &c);

      fprintf(f, " \\\n\n\tdat -u %s", pretty_name);

      if (opt_headername)
	 fprintf(f, " -h %s", opt_headername);

      if (opt_password)
	 fprintf(f, " -007 %s", opt_password);

      fprintf(f, "\n");
      fclose(f);
   }
   else {
      datedit_error("Error writing %s", depname);
      return FALSE;
   }

   return TRUE;
}



int main(int argc, char *argv[])
{
   int c;

   allegro_init();
   set_color_conversion(COLORCONV_NONE);

   for (c=0; c<PAL_SIZE; c++)
      current_pallete[c] = desktop_pallete[c];

   for (c=1; c<argc; c++) {
      if (argv[c][0] == '-') {

	 switch (tolower(argv[c][1])) {

	    case 'a':
	    case 'd':
	    case 'e':
	    case 'l':
	    case 'u':
	    case 'x':
	       if (opt_command)
		  usage();
	       opt_command = tolower(argv[c][1]);
	       break;

	    case 'b':
	       if ((opt_colordepth > 0) || (c >= argc-1))
		  usage();
	       opt_colordepth = atoi(argv[++c]);
	       if ((opt_colordepth != 8) && (opt_colordepth != 15) &&
		   (opt_colordepth != 16) && (opt_colordepth != 24) &&
		   (opt_colordepth != 32))
		  usage();
	       break;

	    case 'c':
	       if ((opt_compression >= 0) || 
		   (argv[c][2] < '0') || (argv[c][2] > '2'))
		  usage();
	       opt_compression = argv[c][2] - '0'; 
	       break;

	    case 'g':
	       if ((opt_gridx > 0) || (c >= argc-4))
		  usage();
	       opt_gridx = atoi(argv[++c]);
	       opt_gridy = atoi(argv[++c]);
	       opt_gridw = atoi(argv[++c]);
	       opt_gridh = atoi(argv[++c]);
	       if ((opt_gridx <= 0) || (opt_gridy <= 0) ||
		   (opt_gridw <= 0) || (opt_gridh <= 0))
		  usage();
	       break;

	    case 'h':
	       if ((opt_headername) || (c >= argc-1))
		  usage();
	       opt_headername = argv[++c];
	       break;

	    case 'k':
	       opt_keepnames = TRUE;
	       break;

	    case 'm':
	       if ((opt_dependencyfile) || (c >= argc-1))
		  usage();
	       opt_dependencyfile = argv[++c];
	       break;

	    case 'o':
	       if ((opt_outputname) || (c >= argc-1))
		  usage();
	       opt_outputname = argv[++c];
	       break;

	    case 'p':
	       if ((tolower(argv[c][2]) == 'a') && 
		   (tolower(argv[c][3]) == 'l') &&
		   (argv[c][4] == 0)) {
		  if ((opt_palette) || (c >= argc-1))
		     usage();
		  opt_palette = argv[++c];
	       }
	       else {
		  if ((opt_prefixstring) || (c >= argc-1))
		     usage();
		  opt_prefixstring = argv[++c];
	       }
	       break;

	    case 's':
	       if ((opt_strip >= 0) || 
		   (argv[c][2] < '0') || (argv[c][2] > '2'))
		  usage();
	       opt_strip = argv[c][2] - '0'; 
	       break;

	    case 't':
	       if ((opt_objecttype) || (c >= argc-1))
		  usage();
	       opt_objecttype = argv[++c];
	       break;

	    case 'v':
	       opt_verbose = TRUE;
	       break;

	    case '0':
	       if ((opt_password) || (c >= argc-1))
		  usage();
	       opt_password = argv[++c];
	       break;

	    default:
	       fprintf(stderr, "Unknown option '%s'\n", argv[c]);
	       return 1;
	 }
      }
      else {
	 if (strchr(argv[c], '=')) {
	    if (opt_numprops < MAX_FILES)
	       opt_proplist[opt_numprops++] = argv[c];
	 }
	 else {
	    if (!opt_datafile)
	       opt_datafile = argv[c];
	    else {
	       if (opt_numnames < MAX_FILES) {
		  opt_namelist[opt_numnames] = argv[c];
		  opt_usedname[opt_numnames] = FALSE;
		  opt_numnames++;
	       }
	    }
	 }
      }
   }

   if ((!opt_datafile) || 
       ((!opt_command) && 
	(opt_compression < 0) && 
	(opt_strip < 0) && 
	(!opt_numprops) &&
	(!opt_headername) &&
	(!opt_dependencyfile)))
      usage();

   datafile = datedit_load_datafile(opt_datafile, FALSE, opt_password);

   if (datafile) {

      if (opt_palette)
	 do_setpal(&datafile, "");

      if (!err) {
	 switch (opt_command) {

	    case 'a':
	       if (!opt_numnames) {
		  fprintf(stderr, "No files specified for addition\n");
		  err = 1;
	       }
	       else {
		  for (c=0; c<opt_numnames; c++) {
		     if (for_each_file(opt_namelist[c], FA_ARCH | FA_RDONLY, do_add_file, 0) <= 0) {
			if (errno)
			   fprintf(stderr, "Error: %s not found\n", opt_namelist[c]);
			err = 1;
			break;
		     }
		     else
			opt_usedname[c] = TRUE;
		  }
	       }
	       break;

	    case 'd':
	       if (!opt_numnames) {
		  fprintf(stderr, "No objects specified for deletion\n");
		  err = 1;
	       }
	       else
		  do_delete(&datafile, "");
	       break;

	    case 'e':
	    case 'x':
	       if (!opt_numnames) {
		  fprintf(stderr, "No objects specified: use '*' to extract everything\n");
		  err = 1;
	       }
	       else
		  do_export(datafile, "");
	       break;

	    case 'l':
	       printf("\n");
	       do_view(datafile, "");
	       printf("\n");
	       break;

	    case 'u':
	       do_update(datafile, "");
	       break;
	 }
      }

      if (!err) {
	 if (opt_command) {
	    for (c=0; c<opt_numnames; c++) {
	       if (!opt_usedname[c]) {
		  fprintf(stderr, "Error: %s not found\n", opt_namelist[c]);
		  err = 1;
	       }
	    }
	 }

	 if (opt_numprops > 0) {
	    if (!opt_numnames) {
	       fprintf(stderr, "No objects specified for setting properties\n");
	       err = 1;
	    }
	    else {
	       for (c=0; c<opt_numnames; c++)
		  opt_usedname[c] = FALSE;

	       do_set_props(datafile, "");

	       for (c=0; c<opt_numnames; c++) {
		  if (!opt_usedname[c]) {
		     fprintf(stderr, "Error: %s not found\n", opt_namelist[c]);
		     err = 1;
		  }
	       }
	    }
	 }
      }

      if ((!err) && ((changed) || (opt_compression >= 0) || (opt_strip >= 0)))
	 if (!datedit_save_datafile(datafile, opt_datafile, opt_strip, opt_compression, opt_verbose, TRUE, FALSE, opt_password))
	    err = 1;

      if ((!err) && (opt_headername))
	 if (!datedit_save_header(datafile, opt_datafile, opt_headername, "dat", opt_prefixstring, opt_verbose))
	    err = 1;

      if ((!err) && (opt_dependencyfile))
	 if (!do_save_dependencies(datafile, opt_datafile, opt_dependencyfile))
	    err = 1;

      unload_datafile(datafile);
   }

   return err;
}

