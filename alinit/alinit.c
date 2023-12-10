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

#include <unistd.h>

#include "allegro.h"
#include "internal.h"
#include "setupdat.h"

/* info about a hardware driver */
typedef struct SOUNDCARD
{
   int id;
   char *name;
   char **param;
   char *desc;
   int present;
} SOUNDCARD;

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


int main(int argc, char *argv[])
{
   allegro_init();
   detect_sound();

   if (install_sound(DIGI_AUTODETECT, MIDI_AUTODETECT, NULL) != 0)
   {
      printf("%s\n", allegro_error);
      allegro_exit();
      return 0;
   }

   if(argc > 1)
   {
     play_sample(&test_sample, 255, 128, 1000, FALSE);
     sleep(1);
   }

   allegro_exit();
   return 0;
}


