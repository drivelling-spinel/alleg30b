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
 *      Sound setup utility for the Allegro library.
 *
 *      Based on the code by David Calvin.
 *
 *      See readme.txt for copyright information.
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <process.h>
#include <libc/dosexec.h>

#include "allegro.h"
#include "internal.h"
#include "setupdat.h"



/* these can be customised to suit your program... */
//#define TITLE           "Allegro Setup " ALLEGRO_VERSION_STR " B"
//#define TITLE           "DOOM M.B.F. - Allegro Setup"
#define TITLE           "M.B.F. - Sound and Music Setup"
#define CFG_FILE        "setup.cfg"
#define KEYBOARD_FILE   "keyboard.dat"

#ifdef STARTGAME
#define XSTR(z) STR(z)
#define STR(z)  #z

#define ENGINE  XSTR(STARTGAME) ".EXE"
#endif 

static int rungame = 0;


/* we only need the VGA 13h, 256 color drivers */
DECLARE_GFX_DRIVER_LIST(GFX_DRIVER_VGA);
DECLARE_COLOR_DEPTH_LIST(COLOR_DEPTH_8);



/* info about a hardware driver */
typedef struct SOUNDCARD
{
   int id;
   char *name;
   char **param;
   char *desc;
   int present;
} SOUNDCARD;


SOUNDCARD *soundcard;



/* list which parameters are used by each driver */
char *digi_auto_param[]    = { "11", "digi_volume", NULL };
char *digi_sbm_param[]     = { "sb_port", "sb_dma", "sb_irq", "digi_volume", "digi_voices", "sb_freq", NULL };
char *digi_sbs_param[]     = { "sb_port", "sb_dma", "sb_irq", "flip_pan", "", "sb_freq", "digi_volume", "digi_voices", NULL };

char *midi_auto_param[]    = { "11", "midi_volume", NULL };
char *midi_adlib_param[]   = { "22", "fm_port", "midi_volume", "", "ibk_file", "ibk_drum_file", NULL };
char *midi_sb_param[]      = { "21", "sb_port", "midi_volume", NULL };
char *midi_mpu_param[]     = { "21", "mpu_port", "midi_volume", NULL };
char *midi_digmid_param[]  = { "22", "midi_voices", "midi_volume", "", "patches", NULL };
char *midi_awe_param[]     = { "21", "midi_voices", "midi_volume", NULL };



/* list of digital sound drivers */
SOUNDCARD digi_cards[] =
{
   /* id                name                    parameter list       desc */
   { DIGI_AUTODETECT,   "Autodetect",           digi_auto_param,     "Attempt to autodetect the digital sound hardware" },
   { DIGI_SB,           "Generic Sound Blaster",digi_sbs_param,      "Sound Blaster: autodetect the model" },
   { DIGI_SB10,         "Sound Blaster 1.0/1.5",digi_sbm_param,      "SB v1.0 or v1.5 DSP 1.xx, 8 bit mono, using single-shot dma" },
   { DIGI_SB15,         "Sound Blaster 1.5 DSP 2.00", digi_sbm_param,"SB v1.5 DSP 2.00, 8 bit mono, using auto-init dma" },
   { DIGI_SB20,         "Sound Blaster 2.0",    digi_sbm_param,      "SB v2.0, 8 bit mono, using auto-init dma, up to 44KHz" },
   { DIGI_SBPRO,        "Sound Blaster Pro",    digi_sbs_param,      "SB Pro, 8 bit stereo DAC" },
   { DIGI_SB16,         "Sound Blaster 16 or AWE32", digi_sbs_param, "SB 16, AWE32 or AWE64, 16 bit stereo DAC" },
   { DIGI_WSS,          "Windows Sound System", digi_sbs_param,      "Sound Card with WSS interface, 16 bit stereo DAC" },
   { DIGI_ESSAUDIO,     "ESS Audiodrive",       digi_sbs_param,      "ESS Audiodrive, 16 bit stereo DAC" },
   { DIGI_SNDSCAPE,     "Ensoniq Soundscape",   digi_sbs_param,      "Ensoniq Soundscape, 16 bit stereo DAC" },
   { DIGI_GUSPNP,       "Gravis Ultrasound PnP",digi_sbs_param,      "Gravis Ultrasound PnP with Interwave Chipset only!" },
   { DIGI_NONE,         "No Sound",             NULL,                "The Sound of Silence..." }
};



/* list of MIDI sound drivers */
SOUNDCARD midi_cards[] =
{
   /* id                name                    parameter list       desc */
   { MIDI_AUTODETECT,   "Autodetect",           midi_auto_param,     "Attempt to autodetect the MIDI hardware" },
   { MIDI_ADLIB,        "FM - Generic Adlib",        midi_adlib_param,    "OPL FM synth: autodetect the model" },
   { MIDI_OPL2,         "FM - Adlib (OPL2)",         midi_adlib_param,    "(mono) OPL2 FM synth (used in Adlib and standard SB cards)" },
   { MIDI_2XOPL2,       "FM - Adlib (dual OPL2)",    midi_adlib_param,    "(stereo) Two OPL2 FM synths (early SB Pro cards)" },
   { MIDI_OPL3,         "FM - Adlib (OPL3)",         midi_adlib_param,    "(stereo) OPL3 FM synth (Adlib Gold, later SB Pro boards, SB16)" },
   { MIDI_AWE32,        "MIDI - Sound Blaster AWE32",  midi_awe_param,      "Sound Blaster AWE32 or AWE64 (EMU8000 synth chip)" },
   { MIDI_SB_OUT,       "MIDI - Sound Blaster MIDI-out",          midi_sb_param, "Raw Sound Blaster MIDI output to an external synth module" },
   { MIDI_MPU,          "MIDI - MPU-401", midi_mpu_param,      "Raw MPU MIDI output to an external synth module" },
// { MIDI_GUS,          "Gravis Ultrasound",    NULL,                "*** not finished, do not use! ***" },
   { MIDI_DIGMID,       "MIDI - Digital MIDI",         midi_digmid_param,   "Software wavetable synthesis using the digital sound hardware" },
   { MIDI_NONE,         "No Sound",             NULL,                "The Sound of Silence..." }
};



/* different types of parameter */
typedef enum PARAM_TYPE
{
   param_none,
   param_int,
   param_hex,
   param_bool,
   param_file,
   param_list
} PARAM_TYPE;



/* info about a soundcard parameter */
typedef struct PARAMETER
{
   char *name;
   PARAM_TYPE type;
   char value[80];
   char *def;
   int *detect;
   char *label;
   char *e1, *e2;
   char *desc;
} PARAMETER;



/* list of soundcard parameters */
PARAMETER parameters[] =
{
   /* name              type           value    default     detect            label       extra1      extra2   desc */
   { "digi_card",       param_int,     "",      "-1",       &digi_card,       "",         NULL,       NULL,    "" },
   { "midi_card",       param_int,     "",      "-1",       &midi_card,       "",         NULL,       NULL,    "" },
   { "digi_volume",     param_int,     "",      "-1",       NULL,             "Vol:",     NULL,       NULL,    "Digital sound volume (0 to 255)" },
   { "midi_volume",     param_int,     "",      "-1",       NULL,             "Vol:",     NULL,       NULL,    "MIDI music volume (0 to 255)" },
   { "digi_voices",     param_int,     "",      "-1",       NULL,             "Chan:",    NULL,       NULL,    "Number of channels reserved for playing digital sounds (higher values increase polyphony but degrade speed and quality)" },
   { "midi_voices",     param_int,     "",      "-1",       NULL,             "Chan:",    NULL,       NULL,    "Number of channels reserved for playing MIDI music (higher values increase polyphony but degrade speed and quality)" },
   { "flip_pan",        param_bool,    "",      "0",        NULL,             "Pan:",     NULL,       NULL,    "Reverses the left/right stereo placement" },
   { "sb_port",         param_hex,     "",      "-1",       &_sb_port,        "Port:",    NULL,       NULL,    "Port address (usually 220)" },
   { "sb_dma",          param_int,     "",      "-1",       &_sb_dma,         "DMA:",     NULL,       NULL,    "DMA channel (usually 1)" },
   { "sb_irq",          param_int,     "",      "-1",       &_sb_irq,         "IRQ:",     NULL,       NULL,    "IRQ number (usually 7)" },
   { "sb_freq",         param_list,    "",      "-1",       &_sb_freq,        "Freq:",    NULL,       NULL,    "Sample mixing frequency (higher values sound better but require more CPU processing time)" },
   { "fm_port",         param_hex,     "",      "-1",       &_fm_port,        "Port:",    NULL,       NULL,    "Port address (usually 388)" },
   { "mpu_port",        param_hex,     "",      "-1",       &_mpu_port,       "Port:",    NULL,       NULL,    "Port address (usually 330)" },
   { "ibk_file",        param_file,    "",      "",         NULL,             "IBK:",     "IBK",      NULL,    "Custom .IBK instrument patch set" },
   { "ibk_drum_file",   param_file,    "",      "",         NULL,             "drumIBK:", "IBK",      NULL,    "Custom .IBK percussion patch set" },
   { "patches",         param_file,    "",      "",         NULL,             "Patches:", "CFG;DAT",  NULL,    "MIDI patch set (GUS format default.cfg or Allegro format patches.dat)" },
   { NULL,              param_none,    "",      "",         NULL,             NULL,       NULL,       NULL,    NULL } 
};



/* in some places we need to double-buffer the display... */
BITMAP *buffer;



/* dialogs do fancy stuff as they slide on and off the screen */
typedef enum DIALOG_STATE
{
   state_start,
   state_slideon,
   state_active,
   state_slideoff,
   state_exit,
   state_chain,
   state_redraw
} DIALOG_STATE;



/* info about an active dialog */
typedef struct ACTIVE_DIALOG
{
   DIALOG_STATE state;
   int time;
   DIALOG *dialog;
   DIALOG_PLAYER *player;
   BITMAP *buffer;
   DIALOG_STATE (*handler)(int c);
} ACTIVE_DIALOG;



/* list of active dialogs */
ACTIVE_DIALOG dialogs[4];

int dialog_count = 0;

/* scrolly text message at the base of the screen */
volatile int scroller_time = 0;
//char scroller_msg[42];
char scroller_msg[56]; //  GB 2017
int scroller_pos = 0;
int scroller_alpha = 256;
char *scroller_string = "";
char *wanted_scroller = "";
int scroller_string_pos = 0;

/* timer interrupt handler */
void inc_scroller_time()
{
   scroller_time++;
}

END_OF_FUNCTION(inc_scroller_time);



/* dialog procedure for animating the scroller text */
int scroller_proc(int msg, DIALOG *d, int c)
{
   int redraw = FALSE;
   int a, i, x;

   if (msg == MSG_IDLE) {
      while (scroller_time > 0) {
	 scroller_pos--;
	 if (scroller_pos <= -8) {
            char space = 0;
            char sp = 1;
	    scroller_pos = 0;
	    for (i=0; i<(int)sizeof(scroller_msg)-1; i++)
            {
               scroller_msg[i] = scroller_msg[i+1];
               sp = sp && scroller_msg[i] == ' ';
            }
	    if (scroller_string[scroller_string_pos])
	       scroller_msg[i] = scroller_string[scroller_string_pos++];
	    else
            {
               scroller_msg[i] = ' ';
               space = sp;
            }
            if (wanted_scroller != scroller_string || space) {
	       scroller_alpha -= MIN(32, scroller_alpha);
	       if (scroller_alpha <= 0) {
		  memset(scroller_msg, ' ', sizeof(scroller_msg));
		  scroller_string = wanted_scroller;
		  scroller_string_pos = 0;
		  for (x=0; x<4; x++) {
		     if (scroller_string[scroller_string_pos]) {
			for (i=0; i<(int)sizeof(scroller_msg)-1; i++)
			   scroller_msg[i] = scroller_msg[i+1];
			scroller_msg[i] = scroller_string[scroller_string_pos];
			scroller_string_pos++;
		     }
		  }
		  scroller_alpha = 256;
	       }
	    }
	    else
	       scroller_alpha += MIN(16, 256-scroller_alpha);
	 }
	 redraw = TRUE;
	 scroller_time--;
      }
   }
   else if (msg == MSG_RADIO) {
      memset(scroller_msg, ' ', sizeof(scroller_msg));
      scroller_string = wanted_scroller;
      scroller_string_pos = strlen(scroller_string);
      scroller_alpha = 256;
      redraw = TRUE;
   }

   if (redraw) {
      freeze_mouse_flag = TRUE;
      text_mode(0);

      for (i=0; i<(int)sizeof(scroller_msg); i++) {
//	 x = i*8+scroller_pos;
	 x = i*6+scroller_pos; //  GB 2017
	 a = 16 + MID(0, 15-ABS(SCREEN_W/2-x)/10, 15) * scroller_alpha/256;

     text_mode(0); //  GB 2014
	  font = &monospaced; //  GB 2016

	 textprintf(screen, font, x, SCREEN_H-18, a, "%c", scroller_msg[i]);
      font = &setup_font; //  GB 2016
      }

      freeze_mouse_flag = FALSE;
   }

   return D_O_K;
}



/* helper for drawing a dialog onto a memory bitmap */
void draw_dialog(ACTIVE_DIALOG *d)
{
   BITMAP *oldscreen = screen;
   int nowhere;

   if (d->player->focus_obj >= 0) {
      SEND_MESSAGE(d->dialog+d->player->focus_obj, MSG_LOSTFOCUS, 0);
      d->dialog[d->player->focus_obj].flags &= ~D_GOTFOCUS;
      d->player->focus_obj = -1;
   }

   if (d->player->mouse_obj >= 0) {
      SEND_MESSAGE(d->dialog+d->player->mouse_obj, MSG_LOSTMOUSE, 0);
      d->dialog[d->player->mouse_obj].flags &= ~D_GOTMOUSE;
      d->player->mouse_obj = -1;
   }

   d->player->res &= ~D_WANTFOCUS;

   show_mouse(NULL);
   clear(d->buffer);
   screen = d->buffer; 
   dialog_message(d->dialog, MSG_DRAW, 0, &nowhere);
   show_mouse(NULL);
   screen = oldscreen;
}



/* start up another dialog */
void activate_dialog(DIALOG *dlg, DIALOG_STATE (*handler)(int c), int chain)
{
   ACTIVE_DIALOG *d = &dialogs[dialog_count];

   d->state = state_start;
   d->time = retrace_count;
   d->dialog = dlg;
   d->player = init_dialog(dlg, -1);
   d->buffer = create_bitmap(SCREEN_W, SCREEN_H);
   d->handler = handler;

   draw_dialog(d);

   if (dialog_count > 0) {
      draw_dialog(&dialogs[dialog_count-1]);
      dialogs[dialog_count-1].state = (chain ? state_chain : state_slideoff);
      dialogs[dialog_count-1].time = retrace_count;
   }
}



/* main dialog update routine */
int update()
{
   BITMAP *oldscreen = screen;
   ACTIVE_DIALOG *d;
   DIALOG_STATE state;
   PALETTE pal;
   int pos, ppos, pppos;

   if (dialog_count <= 0)
      return FALSE;

   d = &dialogs[dialog_count-1];

   if (d->state == state_active) {
      /* process the dialog */
      if (!update_dialog(d->player)) {
	 if (d->handler)
	    state = d->handler(d->player->obj);
	 else
	    state = state_exit;

	 if (state == state_exit) {
	    /* exit this dialog */
	    draw_dialog(d);
	    d->state = state_exit;
	    d->time = retrace_count;
	 }
	 else if (state == state_redraw) {
	    /* redraw the dialog */
	    d->player->res |= D_REDRAW;
	 }
      }
      else {
	 pos = find_dialog_focus(d->dialog);
	 if ((pos >= 0) && (d->dialog[pos].dp3))
	    wanted_scroller = d->dialog[pos].dp3;
         else 
            wanted_scroller = TITLE;
      }
   }
   else {
      /* sliding on or off */
      blit(&background, buffer, 0, 0, 0, 0, SCREEN_W, SCREEN_H);

//      text_mode(-1);
//      textout_centre(buffer, font, TITLE, SCREEN_W/2, 2, -1);

      wanted_scroller = "";
      screen = buffer;
      scroller_proc(MSG_IDLE, NULL, 0);
      screen = oldscreen;

      pos = retrace_count - d->time;

      if ((dialog_count == 1) && (d->state == state_start))
	 pos *= 64;
      else if ((dialog_count == 1) && (d->state == state_exit))
	 pos *= 48;
      else if (d->state == state_start)
	 pos *= 96;
      else
	 pos *= 128;

      pos = MID(0, 4096-pos, 4096);
      pppos = (4096 - pos * pos / 4096);
      pos = pppos / 16;

      /* draw the slide effect */
      switch (d->state) {

	 case state_start:
	    ppos = pos;
	    stretch_sprite(buffer, d->buffer, 0, 0, SCREEN_W+1024-pppos/4, SCREEN_H+1024-pppos/4);
	    break;

	 case state_slideon:
	    ppos = pos;
	    stretch_sprite(buffer, d->buffer, 0, 0, SCREEN_W*ppos/256, SCREEN_H*ppos/256);
	    break;

	 case state_slideoff:
	 case state_chain:
	    ppos = 256 - pos;
	    stretch_sprite(buffer, d->buffer, SCREEN_W/2-SCREEN_W*ppos/512, SCREEN_H/2-SCREEN_H*ppos/512, SCREEN_W*ppos/256, SCREEN_H*ppos/256);
	    break;

	 case state_exit:
	    ppos = 256 - pos;
	    stretch_sprite(buffer, d->buffer, SCREEN_W-SCREEN_W*ppos/256, SCREEN_H-SCREEN_H*ppos/256, SCREEN_W*ppos/256, SCREEN_H*ppos/256);
	    break;

	 default:
	    ppos = 0;
	    break;
      }

      if ((dialog_count == 1) && (d->state != state_slideon) && (d->state != state_slideoff) && (d->state != state_chain)) {
	 fade_interpolate(black_palette, setup_pal, pal, ppos/4, 0, 255);
	 set_palette(pal);
      }
      else
	 vsync();

      blit(buffer, screen, 0, 0, 0, 0, SCREEN_W, SCREEN_H);

      if (pos >= 256) {
	 /* finished the slide */
	 switch (d->state) {

	    case state_start:
	    case state_slideon:
	       /* become active */
	       d->state = state_active;
	       d->player->res |= D_REDRAW;
	       break;

	    case state_slideoff:
	       /* activate next dialog, and then get ready to slide back on */
	       dialogs[dialog_count].time = retrace_count;
	       dialog_count++;
	       d->state = state_slideon;
	       break;

	    case state_exit:
	       /* time to die! */
	       shutdown_dialog(d->player);
	       destroy_bitmap(d->buffer);
	       dialog_count--;
	       if (dialog_count > 0)
		  dialogs[dialog_count-1].time = retrace_count;
	       show_mouse(NULL);
	       break;

	    case state_chain:
	       /* kill myself, then activate the next dialog */
	       shutdown_dialog(d->player);
	       destroy_bitmap(d->buffer);
	       dialogs[dialog_count].time = retrace_count;
	       dialogs[dialog_count-1] = dialogs[dialog_count];
	       show_mouse(NULL);
	       break;

	    default:
	       break;
	 }
      }
   }

   return TRUE;
}



/* helper for checking which drivers are valid */
void detect_sound()
{
   int i;

   for (i=0; digi_cards[i].id != DIGI_NONE; i++) {
      if (detect_digi_driver(digi_cards[i].id) == 0)
	 digi_cards[i].present = FALSE;
      else
	 digi_cards[i].present = TRUE;
   }
   digi_cards[i].present = TRUE;

   for (i=0; midi_cards[i].id != MIDI_NONE; i++) {
      if (detect_midi_driver(midi_cards[i].id) == 0)
	 midi_cards[i].present = FALSE;
      else
	 midi_cards[i].present = TRUE;
   }
   midi_cards[i].present = TRUE;
}



/* helper for initialising the sound code */
int init_sound(char *msg)
{
   char b1[80], b2[80];
   int i;

   detect_sound();

   if (install_sound(DIGI_AUTODETECT, MIDI_AUTODETECT, NULL) == 0)
      return 0;

   if (strlen(allegro_error) <= 32) {
      strcpy(b1, allegro_error);
      b2[0] = 0;
   }
   else {
      for (i=strlen(allegro_error)*9/16; i>10; i--)
	 if (allegro_error[i] == ' ')
	    break;

      strncpy(b1, allegro_error, i);
      b1[i] = 0;

      strcpy(b2, allegro_error+i+1);
   }

   alert(msg, b1, b2, "Ok", NULL, 0, 0);

   return -1;
}



BITMAP *popup_bitmap = NULL;
BITMAP *popup_bitmap2 = NULL;



/* helper for displaying a popup message */
void popup(char *s1, char *s2)
{
   int w;

   if (!popup_bitmap) {
      for (w=512; w>=0; w-=2) {
	 line(screen, w, 16, 0, 16+w, 0);
	 if (!(w&15))
	    vsync();
      }
      for (w=0; w<512; w+=2) {
	 line(screen, w+1, 16, 0, 17+w, 0);
	 if (!(w&15))
	    vsync();
      }
      popup_bitmap = create_bitmap(SCREEN_W, SCREEN_H);
      popup_bitmap2 = create_bitmap(SCREEN_W, SCREEN_H);
      blit(screen, popup_bitmap, 0, 0, 0, 0, SCREEN_W, SCREEN_H);
   }

   blit(popup_bitmap, popup_bitmap2, 0, 0, 0, 0, SCREEN_W, SCREEN_H);

   w = MAX(text_length(font, s1), text_length(font, s2));

   rectfill(popup_bitmap2, (SCREEN_W-w)/2-15, SCREEN_H/2-31, (SCREEN_W+w)/2+15, SCREEN_H/2+31, 0);
   rect(popup_bitmap2, (SCREEN_W-w)/2-16, SCREEN_H/2-32, (SCREEN_W+w)/2+16, SCREEN_H/2+32, 255);

   text_mode(-1);
   textout_centre(popup_bitmap2, font, s1, SCREEN_W/2, SCREEN_H/2-20, -1);
   textout_centre(popup_bitmap2, font, s2, SCREEN_W/2, SCREEN_H/2+4, -1);

   blit(popup_bitmap2, screen, 0, 0, 0, 0, SCREEN_W, SCREEN_H);
}



/* ends the display of a popup message */
void end_popup()
{
   if (popup_bitmap) {
      blit(popup_bitmap, screen, 0, 0, 0, 0, SCREEN_W, SCREEN_H);
      destroy_bitmap(popup_bitmap);
      popup_bitmap = NULL;
      destroy_bitmap(popup_bitmap2);
      popup_bitmap2 = NULL;
   }
}



/* joystick test display */
void plot_joystick_state(BITMAP *bmp)
{
   static char *hat_strings[] = { "centre", "left", "down", "right", "up" };
   int x = (joy_x+128)*SCREEN_W/256;
   int y = (joy_y+128)*SCREEN_H/256;
   int x2 = (joy2_x+128)*SCREEN_W/256;
   int y2 = (joy2_y+128)*SCREEN_H/256;
   char buf[80];
   int c = 0;

   hline(bmp, x-12, y, x+12, 15);
   vline(bmp, x, y-12, y+12, 15);
   circle(bmp, x, y, 12, 1);
   circlefill(bmp, x, y, 4, 31);

   if (joy_type == JOY_TYPE_2PADS) {
      hline(bmp, x2-12, y2, x2+12, 15);
      vline(bmp, x2, y2-12, y2+12, 15);
      circle(bmp, x2, y2, 12, 1);
      circlefill(bmp, x2, y2, 4, 31);
   }

   text_mode(-1);

   #define WRITE_MSG()                                \
      textout(bmp, font, buf, 64, 48+c*20, -1);       \
      c++

   #define WRITE_BOOL(msg, val)                       \
      sprintf(buf, msg ": %s", (val) ? "*" : "");     \
      WRITE_MSG()

   WRITE_BOOL("Button 1", joy_b1);
   WRITE_BOOL("Button 2", joy_b2);

   if (joy_type == JOY_TYPE_2PADS) {
      WRITE_BOOL("Stick 2 Button 1", joy2_b1);
      WRITE_BOOL("Stick 2 Button 2", joy2_b2);
   }

   if ((joy_type == JOY_TYPE_4BUTTON) || (joy_type == JOY_TYPE_6BUTTON) ||
       (joy_type == JOY_TYPE_FSPRO) || (joy_type == JOY_TYPE_WINGEX)) {
      WRITE_BOOL("Button 3", joy_b3);
      WRITE_BOOL("Button 4", joy_b4);
   }

   if (joy_type == JOY_TYPE_6BUTTON) {
      WRITE_BOOL("Button 5", joy_b5);
      WRITE_BOOL("Button 6", joy_b6);
   }

   if ((joy_type == JOY_TYPE_FSPRO) || (joy_type == JOY_TYPE_WINGEX)) {
      sprintf(buf, "Hat: %s", hat_strings[joy_hat]);
      WRITE_MSG();
   }

   if (joy_type == JOY_TYPE_FSPRO) {
      sprintf(buf, "Throttle: %d", joy_throttle);
      WRITE_MSG();
   }

   textout_centre(bmp, font, "- press a key to accept -", SCREEN_W/2, SCREEN_H-16, 255);
}



/* helper for calibrating the joystick */
void joystick_proc()
{
   static char *hat_strings[] = { "", "left", "down", "right", "up" };
   char buf[80];
   int i;

   scroller_proc(MSG_RADIO, NULL, 0);
   show_mouse(NULL);

   if (initialise_joystick() != 0) {
      alert("Joystick not found", NULL, NULL, "Ok", NULL, 0, 0);
      return;
   }

   popup("Centre the joystick", "and press a button");

   rest(10);
   do {
      poll_joystick();
   } while ((!joy_b1) && (!joy_b2));

   initialise_joystick();

   rest(10);
   do {
      poll_joystick();
   } while ((joy_b1) || (joy_b2));

   popup("Move the joystick to the top", "left corner and press a button");

   rest(10);
   do {
      poll_joystick();
   } while ((!joy_b1) && (!joy_b2));

   calibrate_joystick_tl();

   rest(10);
   do {
      poll_joystick();
   } while ((joy_b1) || (joy_b2));

   popup("Move the joystick to the bottom", "right corner and press a button");

   rest(10);
   do {
      poll_joystick();
   } while ((!joy_b1) && (!joy_b2));

   calibrate_joystick_br();

   rest(10);
   do {
      poll_joystick();
   } while ((joy_b1) || (joy_b2));

   if (joy_type == JOY_TYPE_FSPRO) {
      popup("Move the throttle to the minimum", "setting and press a button");

      rest(10);
      do {
	 poll_joystick();
      } while ((!joy_b1) && (!joy_b2));

      calibrate_joystick_throttle_min();

      rest(10);
      do {
	 poll_joystick();
      } while ((joy_b1) || (joy_b2));

      popup("Move the throttle to the maximum", "setting and press a button");

      rest(10);
      do {
	 poll_joystick();
      } while ((!joy_b1) && (!joy_b2));

      calibrate_joystick_throttle_max();

      rest(10);
      do {
	 poll_joystick();
      } while ((joy_b1) || (joy_b2));
   }

   if (joy_type == JOY_TYPE_WINGEX) {
      calibrate_joystick_hat(0);

      for (i=JOY_HAT_LEFT; i<=JOY_HAT_UP; i++) {
	 sprintf(buf, "Move the hat %s", hat_strings[i]);
	 popup(buf, "and press a button");

	 rest(10);
	 do {
	    poll_joystick();
	 } while ((!joy_b1) && (!joy_b2));

	 calibrate_joystick_hat(i);

	 rest(10);
	 do {
	    poll_joystick();
	 } while ((joy_b1) || (joy_b2));
      }
   }

   do {
   } while (mouse_b);

   clear_keybuf();

   while ((!keypressed()) && (!mouse_b)) {
      poll_joystick();

      blit(popup_bitmap, popup_bitmap2, 0, 0, 0, 0, SCREEN_W, SCREEN_H);
      plot_joystick_state(popup_bitmap2);
      blit(popup_bitmap2, screen, 0, 0, 0, 0, SCREEN_W, SCREEN_H);
   }

   end_popup();

   do {
   } while (mouse_b);

   clear_keybuf();

   show_mouse(screen);
}



/* dialog callback for retrieving the SB frequency list */
char *freq_getter(int index, int *list_size)
{
   static char *freq[] =
   {
//      "11111 Hz", 
//      "16129 Hz",
//      "22222 Hz",
//      "43478 Hz"
      "11 kHz",
      "16 kHz",
      "22 kHz",
      "44 kHz"
   };

   if (index < 0) {
      if (list_size) {
	 switch (atoi(parameters[0].value)) {

	    case DIGI_SB10:
	    case DIGI_SB15:
	       *list_size = 3;
	       break;

	    case DIGI_SBPRO:
	       *list_size = 3; // has 44KHz High-speed mono mode too, but not in stereo. GB 2017
	       break;

	    case DIGI_SB: 
	    case DIGI_SB20: // has 44KHz High-speed mono mode. GB 2017
	    case DIGI_SB16:
	    case DIGI_WSS:
	    case DIGI_ESSAUDIO:
	    case DIGI_SNDSCAPE:
	    case DIGI_GUSPNP:
		default:
	       *list_size = 4;
	       break;
	 }
      }
      return NULL;
   }

   return freq[index];
}



/* dialog callback for retrieving information about the soundcard list */
char *card_getter(int index, int *list_size)
{
   static char buf[80];
   int i;

   if (index < 0) {
      i = 0;
      while (soundcard[i].id)
	 i++;
      if (list_size)
	 *list_size = i+1;
      return NULL;
   }

   if (soundcard[index].present)
      sprintf(buf, "%c %s", 251, soundcard[index].name);
   else
      sprintf(buf, "  %s", soundcard[index].name);

   return buf;
}



char keyboard_type[256] = "";

int num_keyboard_layouts = 0;
char *keyboard_layouts[256];



/* dialog callback for retrieving information about the keyboard list */
char *keyboard_getter(int index, int *list_size)
{
   if (index < 0) {
      if (list_size)
	 *list_size = num_keyboard_layouts;
      return NULL;
   }

   return keyboard_layouts[index];
}



/* dialog callback for retrieving information about the joystick list */
char *joystick_getter(int index, int *list_size)
{
   static char *s[] =
   {
      "Standard Joystick",
      "CH Flightstick Pro",
      "4-Button Joystick",
      "6-Button Joystick",
      "Dual Joysticks",
      "Wingman Extreme"
   };

   if (index < 0) {
      *list_size = sizeof(s) / sizeof(char *);
      return NULL;
   }

   return s[index];
}



/* dialog procedure for the soundcard selection listbox */
int card_proc(int msg, DIALOG *d, int c)
{
   int ret = d_list_proc(msg, d, c);
   d->dp3 = soundcard[d->d1].desc;
   return ret;
}



/* dialog procedure for the filename selection objects */
int filename_proc(int msg, DIALOG *d, int c)
{
   PARAMETER *p = d->dp2;
   char buf[256];
   char buf2[256];
   int ret;
   int i;

   if (msg == MSG_START) {
      if (!p->e2)
	 p->e2 = malloc(80);
      strcpy(p->e2, p->value);
   }
   else if (msg == MSG_END) {
      if (p->e2) {
	 free(p->e2);
	 p->e2 = NULL;
      }
   }

   ret = d_check_proc(msg, d, c);

   if (ret & D_CLOSE) {
      if (p->value[0]) {
	 strcpy(p->e2, p->value);
	 p->value[0] = 0;
      }
      else {
	 scroller_proc(MSG_RADIO, NULL, 0);

	 strcpy(buf2, p->desc);

	 for (i=1; buf2[i]; i++) {
	    if (buf2[i] == '(') {
	       buf2[i-1] = 0;
	       break;
	    }
	 }

	 strcpy(buf, p->e2);

	 if (file_select(buf2, buf, p->e1)) {
	    strcpy(p->value, buf);
	    strcpy(p->e2, buf);
	 }

	 show_mouse(NULL);
	 blit(&background, screen, 0, 0, 0, 0, SCREEN_W, SCREEN_H);
//         text_mode(0);
 //        textout_centre(screen, font, TITLE, SCREEN_W/2, 2, -1);
      }

      if (p->value[0])
	 d->flags |= D_SELECTED;
      else
	 d->flags &= ~D_SELECTED;

      ret &= ~D_CLOSE;
      ret |= D_REDRAW;
   }

   return ret;
}



/* wrapper for d_list_proc() to free up the dp2 parameter */
int d_xlist_proc(int msg, DIALOG *d, int c)
{
   void *old_dp2;
   int ret;

   old_dp2 = d->dp2;
   d->dp2 = NULL;

   ret = d_list_proc(msg, d, c);

   d->dp2 = old_dp2;

   return ret;
}



char backup_str[] =  "Go back to the previous menu";
char midi_desc[160];
char digi_desc[160];



DIALOG main_dlg[] =
{
   /* (dialog proc)     (x)   (y)   (w)   (h)   (fg)  (bg)  (key)    (flags)     (d1)           (d2)     (dp)                          (p)      (help message) */
   { d_button_proc,     30,   32,   124,  22,   -1,   16,   0,       D_EXIT,     0,             0,       "Autodetect",                 NULL,    "Attempt to autodetect your Sound Card" },
   { d_button_proc,     166,  32,   124,  22,   -1,   16,   0,       D_EXIT,     0,             0,       "Test",                       NULL,    "Test the current settings" },
   { d_button_proc,     30,   60,   124,  22,   -1,   16,   0,       D_EXIT,     0,             0,       "Digital Driver",             NULL,    "Manually select a driver for playing digital samples" },
   { d_button_proc,     166,  60,   124,  22,   -1,   16,   0,       D_EXIT,     0,             0,       "Midi Driver",                NULL,    "Manually select a driver for playing MIDI music" },
   { d_button_proc,     30,   88,   124,  22,   -1,   16,   0,       D_EXIT,     0,             0,       "Keyboard",                   NULL,    "Select a keyboard layout" },
   { d_button_proc,     166,  88,   124,  22,   -1,   16,   0,       D_EXIT,     0,             0,       "Joystick",                   NULL,    "Calibrate your joystick" },
#ifndef STARTGAME
   { d_button_proc,     30,   116,  260,  22,   -1,   16,   0,       D_EXIT,     0,             0,       "Exit - Save Settings",       NULL,    "Exit from the program, saving the current settings into the file '" CFG_FILE "'" },
   { d_button_proc,     30,   144,  260,  22,   -1,   16,   0,       D_EXIT,     0,             0,       "Exit - Do not Save",         NULL,    "Exit from the program, without saving the current settings" },
#else
   { d_button_proc,     30,   144,  124,  22,   -1,   16,   0,       D_EXIT,     0,             0,       "Save && Exit",                NULL,    "Exit from the program, saving the current settings into the file '" CFG_FILE "'" },
   { d_button_proc,     166,  144,  124,  22,   -1,   16,   0,       D_EXIT,     0,             0,       "Abort",            NULL,    "Exit from the program, without saving the current settings" },
#endif
   { scroller_proc,     0,    0,    0,    0,    0,    0,    0,       0,          0,             0,       NULL,                         NULL,    NULL },
#ifdef STARTGAME
   { d_button_proc,     30,   116,  260,  22,   -1,   16,   0,       D_EXIT,     0,             0,       "Start game",                 NULL,    "Save current settings into the file '" CFG_FILE "' and start the game" },
#endif
   { NULL }
};



DIALOG test_dlg[] =
{
   /* (dialog proc)     (x)   (y)   (w)   (h)   (fg)  (bg)  (key)    (flags)     (d1)           (d2)     (dp)                          (p)      (help message) */
   { d_button_proc,     100,  50,   120,  22,   -1,   16,   0,       D_EXIT,     0,             0,       "MIDI",                       NULL,    midi_desc },
   { d_button_proc,     30,   87,   80,   22,   -1,   16,   0,       D_EXIT,     0,             0,       "Left",                       NULL,    digi_desc },
   { d_button_proc,     120,  87,   80,   22,   -1,   16,   0,       D_EXIT,     0,             0,       "Centre",                     NULL,    digi_desc },
   { d_button_proc,     210,  87,   80,   22,   -1,   16,   0,       D_EXIT,     0,             0,       "Right",                      NULL,    digi_desc },
   { d_button_proc,     100,  124,  120,  22,   -1,   16,   0,       D_EXIT,     0,             0,       "Exit",                       NULL,    backup_str },
   { scroller_proc,     0,    0,    0,    0,    0,    0,    0,       0,          0,             0,       NULL },
   { NULL }
};

DIALOG test_dlg_mono[] = // GB 2017
{
   /* (dialog proc)     (x)   (y)   (w)   (h)   (fg)  (bg)  (key)    (flags)     (d1)           (d2)     (dp)                          (p)      (help message) */
   { d_button_proc,     100,  50,   120,  22,   -1,   16,   0,       D_EXIT,     0,             0,       "MIDI",                       NULL,    midi_desc },
   { d_button_proc,     120,  87,   80,   22,   -1,   16,   0,       D_EXIT,     0,             0,       "Centre",                     NULL,    digi_desc },
   { d_button_proc,     100,  124,  120,  22,   -1,   16,   0,       D_EXIT,     0,             0,       "Exit",                       NULL,    backup_str },
   { scroller_proc,     0,    0,    0,    0,    0,    0,    0,       0,          0,             0,       NULL },
   { NULL }
};



DIALOG card_dlg[] =
{
   /* (dialog proc)     (x)   (y)   (w)   (h)   (fg)  (bg)  (key)    (flags)     (d1)           (d2)     (dp)                          (p)      (help message) */
   { d_button_proc,     30,   138,  124,  22,   -1,   16,   0,       D_EXIT,     0,             0,       "OK",                         NULL,    "Use this driver" },
   { d_button_proc,     166,  138,  124,  22,   -1,   16,   0,       D_EXIT,     0,             0,       "Cancel",                     NULL,    backup_str },
   { card_proc,         30,   36,   260,  94,   255,  16,   0,       D_EXIT,     0,             0,       card_getter,                  NULL,    NULL },
   { scroller_proc,     0,    0,    0,    0,    0,    0,    0,       0,          0,             0,       NULL },
   { NULL }
};



DIALOG keyboard_dlg[] =
{
   /* (dialog proc)     (x)   (y)   (w)   (h)   (fg)  (bg)  (key)    (flags)     (d1)           (d2)     (dp)                          (p)      (help message) */
   { d_button_proc,     30,   138,  124,  22,   -1,   16,   0,       D_EXIT,     0,             0,       "OK",                         NULL,    "Use this keyboard layout" },
   { d_button_proc,     166,  138,  124,  22,   -1,   16,   0,       D_EXIT,     0,             0,       "Cancel",                     NULL,    backup_str },
   { d_list_proc,       60,   36,   260,  94,   255,  16,   0,       D_EXIT,     0,             0,       keyboard_getter,              NULL,    "Select a keyboard layout" },
   { scroller_proc,     0,    0,    0,    0,    0,    0,    0,       0,          0,             0,       NULL },
   { NULL }
};



DIALOG joystick_dlg[] =
{
   /* (dialog proc)     (x)   (y)   (w)   (h)   (fg)  (bg)  (key)    (flags)     (d1)           (d2)     (dp)                          (p)      (help message) */
   { d_button_proc,     30,   132,  124,  22,   -1,   16,   0,       D_EXIT,     0,             0,       "OK",                         NULL,    "Use this joystick type" },
   { d_button_proc,     166,  132,  124,  22,   -1,   16,   0,       D_EXIT,     0,             0,       "Cancel",                     NULL,    backup_str },
   { d_list_proc,       60,   36,   200,  83,   255,  16,   0,       D_EXIT,     0,             0,       joystick_getter,              NULL,    "Select a type of joystick" },
   { scroller_proc,     0,    0,    0,    0,    0,    0,    0,       0,          0,             0,       NULL },
   { NULL }
};



/* this one is generated depending on which parameters we need */
DIALOG param_dlg[32];

int param_ok;



/* handle input from the parameter dialog */
DIALOG_STATE param_handler(int c)
{
   PARAMETER *p;
   DIALOG *d = param_dlg;
   int i;

   if (c == param_ok) {
      /* save the changes */ 
      while (d->proc) {
	 p = d->dp2;

	 if (p) {
	    switch (p->type) {

	       case param_int:
		  if (p->value[0])
		     i = strtol(p->value, NULL, 0);
		  else
		     i = -1;
		  set_config_int("sound", p->name, i);
		  strcpy(p->value, get_config_string("sound", p->name, ""));
		  break;

	       case param_hex:
		  if (p->value[0])
		     i = strtol(p->value, NULL, 16);
		  else
		     i = -1;
		  set_config_hex("sound", p->name, i);
		  strcpy(p->value, get_config_string("sound", p->name, ""));
		  break;

	       case param_bool:
		  set_config_int("sound", p->name, (d->flags & D_SELECTED) ? 1 : 0);
		  strcpy(p->value, get_config_string("sound", p->name, ""));
		  break;

	       case param_file:
		  set_config_string("sound", p->name, p->value);
		  strcpy(p->value, get_config_string("sound", p->name, ""));
		  break;

	       case param_list:
		  i = strtol(freq_getter(d->d1, NULL), NULL, 0);
          if      (i==11)    i=11111; // GB 1-2016
		  else if (i==16)    i=16129;
		  else if (i==22)    i=22222; // GB bugfix 2017
		  else               i=43478;
   
		  set_config_int("sound", p->name, i);
		  strcpy(p->value, get_config_string("sound", p->name, ""));
		  break;

	       default:
		  break;
	    }

	 }

	 d++;
      }
   }
   else {
      /* discard the changes */ 
      while (d->proc) {
	 p = d->dp2;

	 if (p)
	    strcpy(p->value, get_config_string("sound", p->name, ""));

	 d++;
      }
   }

   return state_exit;
}



/* sets up the soundcard parameter dialog box */
void setup_param_dialog()
{
   PARAMETER *p;
   DIALOG *d = param_dlg;
   char **c = soundcard->param;
   char *s;
   int pos = 0;
   int xo = 0;
   int yo = 0;
   int i, x, y, f, g;

   #define DLG(_p, _x, _y, _w, _h, _f, _b, _k, _l, _d1, _d2, _dp, _pa, _m) \
   {                                                                       \
      d->proc = _p;                                                        \
      d->x = _x;                                                           \
      d->y = _y;                                                           \
      d->w = _w;                                                           \
      d->h = _h;                                                           \
      d->fg = _f;                                                          \
      d->bg = _b;                                                          \
      d->key = _k;                                                         \
      d->flags = _l;                                                       \
      d->d1 = _d1;                                                         \
      d->d2 = _d2;                                                         \
      d->dp = _dp;                                                         \
      d->dp2 = _pa;                                                        \
      d->dp3 = _m;                                                         \
      d++;                                                                 \
   }

   while (*c) {
      if ((isdigit((*c)[0])) && (isdigit((*c)[1]))) {
	 xo = (*c)[0] - '0';
	 if (xo)
	    xo = 100 / xo;

	 yo = (*c)[1] - '0';
	 if (yo)
	    yo = 38 / yo;
      }
      else {
	 x = 16 + (pos%3) * 100 + xo;
	 y = 30 + (pos/3) * 38 + yo;
	 pos++;

	 p = NULL;

	 for (i=0; parameters[i].name; i++) {
	    if (stricmp(parameters[i].name, *c) == 0) {
	       p = &parameters[i];
	       break;
	    }
	 }

	 if (p) {
	    switch (p->type) {

	       case param_int:

		  /* (dialog proc)     (x)   (y)   (w)   (h)   (fg)  (bg)  (key) (flags)  (d1)  (d2)  (dp)           (p)         (help) */
		  DLG(d_box_proc,      x,    y,    88,   21,   255,  16,   0,    0,       0,    0,    NULL,          NULL,       NULL);
		  DLG(d_text_proc,     x+4,  y+3,  0,    0,    255,  16,   0,    0,       0,    0,    p->label,      NULL,       NULL);
		  DLG(d_edit_proc,     x+54, y+3,  32,   16,   255,  16,   0,    0,       3,    0,    p->value,      p,          p->desc);
		  break;
          
	       case param_hex:
		  if (stricmp(p->value, "FFFFFFFF") == 0)
		     strcpy(p->value, "-1");

		  /* (dialog proc)     (x)   (y)   (w)   (h)   (fg)  (bg)  (key) (flags)  (d1)  (d2)  (dp)           (p)         (help) */
		  DLG(d_box_proc,      x,    y,    88,   21,   255,  16,   0,    0,       0,    0,    NULL,          NULL,       NULL);
		  DLG(d_text_proc,     x+4,  y+3,  0,    0,    255,  16,   0,    0,       0,    0,    p->label,      NULL,       NULL);
		  DLG(d_edit_proc,     x+50, y+3,  36,   16,   255,  16,   0,    0,       3,    0,    p->value,      p,          p->desc);
		  break;

          // Stereo flip pan toggle:
           case param_bool:
		  if (strtol(p->value, NULL, 0) != 0)
		     f = D_SELECTED;
		  else
		     f = 0;
		  //if (atoi(parameters[0].value) == DIGI_SBPRO) f = D_SELECTED; // GB 2017, SBPro is reversed in hardware. but rather do this in driver.

		  /* (dialog proc)     (x)   (y)   (w)   (h)   (fg)  (bg)  (key) (flags)  (d1)  (d2)  (dp)           (p)         (help) */
		  DLG(d_box_proc,      x,    y,    88,   21,   255,  16,   0,    0,       0,    0,    NULL,          NULL,       NULL);
		  DLG(d_text_proc,     x+4,  y+3,  0,    0,    255,  16,   0,    0,       0,    0,    p->label,      NULL,       NULL);
		  DLG(d_check_proc,    x+54, y+3,  31,   15,   255,  16,   0,    f,       0,    0,    " ",           p,          p->desc);
		  break;

	       case param_file:
		  if (p->value[0])
		     f = D_SELECTED | D_EXIT;
		  else
		     f = D_EXIT;

		  /* (dialog proc)     (x)   (y)   (w)   (h)   (fg)  (bg)  (key) (flags)  (d1)  (d2)  (dp)           (p)         (help) */
		  DLG(d_box_proc,      x,    y,    88,   21,   255,  16,   0,    0,       0,    0,    NULL,          NULL,       NULL);
		  DLG(d_text_proc,     x+4,  y+3,  0,    0,    255,  16,   0,    0,       0,    0,    p->label,      NULL,       NULL);
		  DLG(filename_proc,   x+62, y+3,  31,   15,   255,  16,   0,    f,       0,    0,    "",            p,          p->desc);
		  break;

          // GB 2017, i figure this is to translate frequency value to the proper selection entry in the listbox (0..3).
		  case param_list:
		  i = strtol(p->value, NULL, 0); 
		  i = i /1000; // GB 2017, bugfix for kHz instead of Hz.
		  freq_getter(-1, &f);
		  if (i > 0) {
		     for (g=0; g<f; g++) {
			s = freq_getter(g, NULL);
			if (i <= strtol(s, NULL, 0))
			   break;
		     }
		  }
		  else
		     g = 2;


		  /* (dialog proc)     (x)   (y)   (w)   (h)   (fg)  (bg)  (key) (flags)  (d1)  (d2)  (dp)           (p)         (help) */
		  DLG(d_xlist_proc,    x,    y-8,  88,   67,   255,  16,   0,    0,       g,    0,    freq_getter,   p,          p->desc);
		  break;

           default:
		  break;
	    }
	 }
      }

      c++;
   }

   param_ok = ((int)d - (int)param_dlg) / sizeof(DIALOG);

   /* (dialog proc)     (x)   (y)   (w)   (h)   (fg)  (bg)  (key) (flags)  (d1)  (d2)  (dp)        (p)                        (help) */
   DLG(d_button_proc,   30,   142,  124,  24,   -1,   16,   13,   D_EXIT,  0,    0,    "OK",       NULL,                      "Use these parameters");
   DLG(d_button_proc,   166,  142,  124,  24,   -1,   16,   0,    D_EXIT,  0,    0,    "Cancel",   NULL,                      backup_str);
   DLG(scroller_proc,   0,    0,    0,    0,    0,    0,    0,    0,       0,    0,    NULL,       NULL,                      NULL);
   DLG(NULL,            0,    0,    0,    0,    0,    0,    0,    0,       0,    0,    NULL,       NULL,                      NULL);

   activate_dialog(param_dlg, param_handler, TRUE);
}



/* handle input from the test dialog */
DIALOG_STATE test_handler(int c)
{

   // Apply volume, new GB 2017 
   int i;
   int digi_vol = -1;
   int midi_vol = -1;

   for (i=0; parameters[i].name; i++) { // GB 2017
     if (stricmp(parameters[i].name,"digi_volume")==0) digi_vol=strtol(parameters[i].value, NULL, 0); 
     if (stricmp(parameters[i].name,"midi_volume")==0) midi_vol=strtol(parameters[i].value, NULL, 0); 
   }

   if (digi_vol==-1) // no Digi volume set.
	 set_volume(220,-1); // 220 GB 2017, Digi, higher volume because of background noise.
   else 
	 set_volume(digi_vol,-1);

   if (midi_vol==-1) 
   {
       //if ((midi_driver->name[0]=='M') && (midi_driver->name[1]=='I')) 
	   if (midi_driver == &midi_mpu401)
       set_volume(-1,255); // GB 2017, Midi, higher volume because of background noise.
	   // GB 2017, WSS MIXER GETS ALL WEIRD AFTER SETTING FM VOLUME!!?? Let it be for now.
	   //else if (digi_driver != &digi_wss) 
       //set_volume(-1,200); // GB 2017, FM, higher volume because of background noise.
   }
   else set_volume(-1,midi_vol);
   

   // Mono Test Handler Translation, new GB 2017
   if ((digi_driver->name[0]=='S') && (digi_driver->name[15]=='.')) //GB 2017, SB 1.0 + 1.5 + 2.0 
   {
     if (c==2) c=4; // exit
     if (c==1) c=2; // test digi center
   }   

   // Main Handler
   switch (c) {

       case 0:
	   /* MIDI test */
	   play_midi(&test_midi, FALSE);
	   return state_redraw;

        case 1:
	   /* left pan */
	   // int play_sample(SAMPLE *spl, int vol, int pan, int freq, int loop)
	   play_sample(&test_sample, 255, 0, 1000, FALSE);
	   return state_redraw;

        case 2:
	   /* centre pan */
	   play_sample(&test_sample, 255, 128, 1000, FALSE);
	   return state_redraw;

        case 3:
	   /* right pan */
	   play_sample(&test_sample, 255, 255, 1000, FALSE);
	   return state_redraw;

        default:
	   /* quit */
	   remove_sound();
	   return state_exit;
   }
   
   return state_active;
}


/* handle input from the card selection dialog */
DIALOG_STATE card_handler(int c)
{
   int i;

   switch (c) {

      case 0:
      case 2:
	 /* select driver */
	 i = (soundcard == digi_cards) ? 0 : 1;
	 soundcard += card_dlg[2].d1;
	 set_config_int("sound", parameters[i].name, soundcard->id);
	 strcpy(parameters[i].value, get_config_string("sound", parameters[i].name, ""));
	 if (soundcard->param)
	    setup_param_dialog();
	 else
	    return state_exit;
	 break;

      default:
	 /* quit */
	 return state_exit;
   }

   return state_active;
}



/* handle input from the keyboard selection dialog */
DIALOG_STATE keyboard_handler(int c)
{
   switch (c) {

      case 0:
      case 2:
	 /* select driver */
	 strcpy(keyboard_type, keyboard_layouts[keyboard_dlg[2].d1]);
	 return state_exit;

      default:
	 /* quit */
	 return state_exit;
   }

   return state_active;
}



/* handle input from the joystick selection dialog */
DIALOG_STATE joystick_handler(int c)
{
   switch (c) {

      case 0:
      case 2:
	 /* select joystick */
	 joy_type = joystick_dlg[2].d1;
	 joystick_proc();
	 return state_exit;

      default:
	 /* quit */
	 return state_exit;
   }

   return state_active;
}



/* handle input from the main dialog */
DIALOG_STATE main_handler(int c)
{
   char b1[80], b2[80];
   int i,dsp_ver;
   char *s, *s2;
   DATAFILE *keyboard_data;

   switch (c) {

      case 0:
	 /* autodetect */
	 scroller_proc(MSG_RADIO, NULL, 0);

	 for (i=0; parameters[i].name; i++) {
	    set_config_string("sound", parameters[i].name, "");
	    parameters[i].value[0] = 0;
	 }

	 if (init_sound("Unable to autodetect!") != 0)
	    return state_redraw;

	 sprintf(b1, "Digital: %s", digi_driver->name);
	 sprintf(b2, "MIDI: %s", midi_driver->name);
	 alert("- detected hardware -", b1, b2, "Ok", NULL, 0, 0);

	 for (i=0; parameters[i].name; i++) {
	    if (parameters[i].detect) {
	       switch (parameters[i].type) {

		  case param_int:
		  case param_bool:
		  case param_list:
		     set_config_int("sound", parameters[i].name, *parameters[i].detect);
		     break;

		  case param_hex:
		     set_config_hex("sound", parameters[i].name, *parameters[i].detect);
		     break;

		  default:
		     break;
	       }
	    }
	    else
	       set_config_string("sound", parameters[i].name, parameters[i].def);

	    strcpy(parameters[i].value, get_config_string("sound", parameters[i].name, ""));
	 }

	 remove_sound();
	 return state_redraw;

      case 1:
	 /* test */
	 scroller_proc(MSG_RADIO, NULL, 0);
	 if (init_sound("Sound initialization failed!") != 0)
	    return state_redraw;

	 sprintf(midi_desc, "Driver: %s        Description: %s", midi_driver->name, midi_driver->desc);

	 if ((digi_driver->desc[0]=='S') && (digi_driver->desc[1]=='B')) // Sound Blasters: Detect DSP version, may be interesting. GB 2017
	 {
	   dsp_ver=_sb_read_dsp_version();
	   sprintf(digi_desc, "Driver: %s        Description: %s        Detected DSP: v%d.%02d", digi_driver->name, digi_driver->desc, (dsp_ver >> 8), (dsp_ver & 0xFF)); //160 chars max
     }
	 else sprintf(digi_desc, "Driver: %s        Description: %s", digi_driver->name, digi_driver->desc);


     if ((digi_driver->name[0]=='S') && (digi_driver->name[15]=='.')) //GB 2017, SB 1.0 + 1.5 + 2.0 
          activate_dialog(test_dlg_mono, test_handler, FALSE);
	 else activate_dialog(test_dlg,      test_handler, FALSE);
     
	 break;

      case 2:
	 /* choose digital driver */
	 soundcard = digi_cards;
	 for (i=0; soundcard[i].id; i++)
	    if (soundcard[i].id == get_config_int("sound", "digi_card", DIGI_AUTODETECT))
	       break;
	 card_dlg[2].d1 = i;
	 activate_dialog(card_dlg, card_handler, FALSE);
	 break;

      case 3:
	 /* choose MIDI driver */
	 soundcard = midi_cards;
	 for (i=0; soundcard[i].id; i++)
	    if (soundcard[i].id == get_config_int("sound", "midi_card", MIDI_AUTODETECT))
	       break;
	 card_dlg[2].d1 = i;
	 activate_dialog(card_dlg, card_handler, FALSE);
	 break;

      case 4:
	 /* select keyboard */
	 if (num_keyboard_layouts <= 0) {
	    keyboard_data = load_datafile(KEYBOARD_FILE);
	    if (!keyboard_data) {
	       s = getenv("ALLEGRO");
	       if (s) {
		  strcpy(b1, s);
		  strlwr(b1);
		  put_backslash(b1);
		  strcat(b1, KEYBOARD_FILE);
		  keyboard_data = load_datafile(b1);
	       }
	    }
	    if (!keyboard_data) {
	       scroller_proc(MSG_RADIO, NULL, 0);
	       alert("Error reading " KEYBOARD_FILE, NULL, NULL, "Ok", NULL, 0, 0);
	    }
	    else {
	       for (i=0; keyboard_data[i].type != DAT_END; i++) {
		  s = get_datafile_property(keyboard_data+i, DAT_ID('N','A','M','E'));
		  if (s) {
		     s2 = strstr(s, "_CFG");
		     if ((s2) && (s2[4]==0)) {
			s2 = keyboard_layouts[num_keyboard_layouts] = malloc(strlen(s)+1);
			strcpy(s2, s);
			s2[strlen(s2)-4] = 0;
			num_keyboard_layouts++;
		     }
		  }
	       }
	    }
	 }
	 if (num_keyboard_layouts > 0) {
	    for (i=0; i<num_keyboard_layouts; i++)
	       if (stricmp(keyboard_type, keyboard_layouts[i]) == 0)
		  break;
	    if (i>=num_keyboard_layouts) {
	       sprintf(b1, "(%s)", keyboard_type);
	       scroller_proc(MSG_RADIO, NULL, 0);
	       alert("Warning: current layout", b1, "not found in keyboard.dat", "Ok", NULL, 0, 0);
	       keyboard_layouts[num_keyboard_layouts] = malloc(strlen(keyboard_type)+1);
	       strcpy(keyboard_layouts[num_keyboard_layouts], keyboard_type);
	       num_keyboard_layouts++;
	    }
	    keyboard_dlg[2].d1 = i;
	    activate_dialog(keyboard_dlg, keyboard_handler, FALSE);
	 }
	 break;

      case 5:
	 /* calibrate joystick */
	 joystick_dlg[2].d1 = joy_type;
	 activate_dialog(joystick_dlg, joystick_handler, FALSE);
	 break;

      case 9: 
         rungame = 1;
      case 6:
	 /* save settings and quit */
	 set_config_file(CFG_FILE);
	 for (i=0; parameters[i].name; i++) {
	    if (parameters[i].value[0])
	       set_config_string("sound", parameters[i].name, parameters[i].value);
	    else
	       set_config_string("sound", parameters[i].name, " ");
	 }
	 set_config_string(NULL, "keyboard", keyboard_type);
	 save_joystick_data(NULL);
         return state_exit;

      default:
	 /* quit */
	 return state_exit;
   }

   return state_active;
}



int main()
{
   int i;

   allegro_init();
   install_mouse();
   install_keyboard();
   install_timer();

   fade_out(4);
   set_gfx_mode(GFX_VGA, 320, 200, 0, 0);
   set_pallete(black_palette);

   //gui_bg_color=0;
   //gui_fg_color=0;
   //gui_mg_color=0;

   set_mouse_range(0, 20, SCREEN_W-1, SCREEN_H-32);

   font = &setup_font;

   memset(scroller_msg, ' ', sizeof(scroller_msg));

   buffer = create_bitmap(SCREEN_W, SCREEN_H);

   LOCK_VARIABLE(scroller_time);
   LOCK_FUNCTION(inc_scroller_time);
   install_int_ex(inc_scroller_time, BPS_TO_TIMER(160));

   set_config_file(CFG_FILE);
   for (i=0; parameters[i].name; i++) {
      strcpy(parameters[i].value, get_config_string("sound", parameters[i].name, parameters[i].def));
      if (!parameters[i].value[0])
	 strcpy(parameters[i].value, parameters[i].def);
   }

   strcpy(keyboard_type, get_config_string(NULL, "keyboard", ""));
   for (i=0; keyboard_type[i]; i++)
      if (!isspace(keyboard_type[i]))
	 break;
   if (!keyboard_type[i])
      strcpy(keyboard_type, "us");

   load_joystick_data(NULL);

   set_config_data("", 0);
   for (i=0; parameters[i].name; i++)
      set_config_string("sound", parameters[i].name, parameters[i].value);

   detect_sound();

   activate_dialog(main_dlg, main_handler, FALSE);
   dialog_count++;

   do {
   } while (update());

   destroy_bitmap(buffer);

   for (i=0; i<num_keyboard_layouts; i++)
      free(keyboard_layouts[i]);

   set_mouse_range(0, 0, SCREEN_W-1, SCREEN_H-1);

#ifdef STARTGAME
   if(rungame)
   {
     char ** env = (char **) calloc(sizeof(char *), 16);
     char ** args = (char **) calloc(sizeof(char *), 2);
     const char * engine = strdup(ENGINE);
     int c = 0;


     *args = strdup(ENGINE);


#define envvar(name)  \
          (strcat( \
            strcpy( \
              calloc(1, strlen(getenv(name)) + 20) \
              , strdup(name "=") \
            ), strdup( \
              getenv(name) \
            ) \
          ))

     if(getenv("PATH")) env[c++] = envvar("PATH");
     if(getenv("TEMP")) env[c++] = envvar("TEMP");
     if(getenv("TMP")) env[c++] = envvar("TMP");
     if(getenv("PROMPT")) env[c++] = envvar("PROMPT");
     if(getenv("COMPSEC")) env[c++] = envvar("COMSPEC"); 
     if(getenv("BLASTER")) env[c++] = envvar("BLASTER");
     if(getenv("ULTRAMID")) env[c++] = envvar("ULTRAMID");
     if(getenv("ULTRADIR")) env[c++] = envvar("ULTRADIR");
     if(getenv("DOOMWADDIR")) env[c++] = envvar("DOOMWADDIR");
     env[c++] = NULL;

     set_config_file(CFG_FILE);
     fade_out(4);
     set_gfx_mode(GFX_TEXT, 320, 200, 0, 0);
     allegro_exit();

     return __dosexec_command_exec(engine, args, env);
   } 
#endif
   return 0;
}


