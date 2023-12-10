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
 *      Adlib/FM driver for the MIDI player.
 *
 *      See readme.txt for copyright information.
 */

// GB 2014: heavily modified for authentic Doom sound


#ifndef DJGPP
#error This file should only be used by the djgpp version of Allegro
#endif

#include <stdlib.h>
#include <stdio.h>
#include <dos.h>

#include <string.h>

#include "allegro.h"
#include "internal.h"

// external interface to the Adlib driver 
static int  fm_detect();
static int  fm_init(int voices);
static void fm_exit();
static int  fm_mixer_volume(int volume);
static int  fm_load_patches(char *patches, char *drums);
static void fm_key_on(int inst, int note, int bend, int vol, int pan);
static void fm_key_off(int voice);
static void fm_set_volume(int voice, int vol);
static void fm_set_pitch(int voice, int note, int bend);

static char adlib_desc[80] = "not initialised";


MIDI_DRIVER midi_adlib =
{  "Adlib OPL Synth Family", 
   adlib_desc,
   0, 0, 0, 0, -1, -1,
   fm_detect,
   fm_init,
   fm_exit,
   fm_mixer_volume,
   NULL,
   fm_load_patches,
   _dummy_adjust_patches,
   fm_key_on,
   fm_key_off,
   fm_set_volume,
   fm_set_pitch,
   _dummy_noop2,
   _dummy_noop2
};


typedef struct FM_INSTRUMENT
{
   unsigned char characteristic1;
   unsigned char characteristic2;
   unsigned char level1;
   unsigned char level2;
   unsigned char attackdecay1;
   unsigned char attackdecay2;
   unsigned char sustainrelease1;
   unsigned char sustainrelease2;
   unsigned char wave1;
   unsigned char wave2;
   unsigned char feedback;
   unsigned char freq;
   unsigned char key;
   unsigned char type;
} FM_INSTRUMENT;


#define FM_HH     1
#define FM_CY     2
#define FM_TT     4
#define FM_SD     8
#define FM_BD     16


// include the GM patch set (static data) 
#include "fm_instr.h"

#ifdef FM_SECONDARY
static FM_INSTRUMENT fm_secondary[128];
#endif

// is the OPL in percussion mode? 
static int fm_drum_mode = FALSE;

// delays when writing to OPL registers 
static int fm_delay_1 = 6;
static int fm_delay_2 = 35;

// register offsets for each voice 
static int fm_offset[18] = {
   0x000, 0x001, 0x002, 0x008, 0x009, 0x00A, 0x010, 0x011, 0x012, 
   0x100, 0x101, 0x102, 0x108, 0x109, 0x10A, 0x110, 0x111, 0x112
};

// for converting midi note numbers to FM frequencies 
static int fm_freq[13] = {
   0x157, 0x16B, 0x181, 0x198, 0x1B0, 0x1CA,
   0x1E5, 0x202, 0x220, 0x241, 0x263, 0x287, 0x2AE
};

// logarithmic relationship between midi and FM volumes 
static int fm_vol_table[128] = {
   0,  11, 16, 19, 22, 25, 27, 29, 32, 33, 35, 37, 39, 40, 42, 43,
   45, 46, 48, 49, 50, 51, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62,
   64, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 75, 76, 77,
   78, 79, 80, 80, 81, 82, 83, 83, 84, 85, 86, 86, 87, 88, 89, 89,
   90, 91, 91, 92, 93, 93, 94, 95, 96, 96, 97, 97, 98, 99, 99, 100,
   101, 101, 102, 103, 103, 104, 104, 105, 106, 106, 107, 107, 108,
   109, 109, 110, 110, 111, 112, 112, 113, 113, 114, 114, 115, 115,
   116, 117, 117, 118, 118, 119, 119, 120, 120, 121, 121, 122, 122,
   123, 123, 124, 124, 125, 125, 126, 126, 127
};

// drum channel tables:          BD       SD       TT       CY       HH    
static int fm_drum_channel[] = { 6,       7,       8,       8,       7     };
static int fm_drum_op1[] =     { TRUE,    FALSE,   TRUE,    FALSE,   TRUE  };
static int fm_drum_op2[] =     { TRUE,    TRUE,    FALSE,   TRUE,    FALSE };
static int fm_drum_pitch[] =   { TRUE,    TRUE,    TRUE,    FALSE,   FALSE };

// cached information about the state of the drum channels 
static FM_INSTRUMENT *fm_drum_cached_inst1[5];
static FM_INSTRUMENT *fm_drum_cached_inst2[5];
static int fm_drum_cached_vol1[5];
static int fm_drum_cached_vol2[5];
static long fm_drum_cached_time[5];

// various bits of information about the current state of the FM chip 
static unsigned char fm_drum_mask;
static unsigned char fm_key[18];
static unsigned char fm_keyscale1[18];
static unsigned char fm_keyscale2[18];
static unsigned char fm_feedback[18];
static int fm_noteoffs[18];
#ifdef FM_SECONDARY
static int fm_finetune[18];
#endif
static int fm_level1[18];
static int fm_level2[18];
static int fm_patch[18];


#define VOICE_OFFSET(x)     ((x < 9) ? x : 0x100+x-9)

// ----------------------------------------------------------------------------------------------
// fm_write: Writes a byte to the specified register on the FM chip.
static void fm_write(int reg, unsigned char data)
{
   int i;
   int port = (reg & 0x100) ? _fm_port+2 : _fm_port;

   outportb(port, reg & 0xFF);      // write the register 

   for (i=0; i<fm_delay_1; i++)     // delay 
      inportb(port);

   outportb(port+1, data);          // write the data 

   for (i=0; i<fm_delay_2; i++)     // delay 
      inportb(port);
}
static END_OF_FUNCTION(fm_write);

#ifdef 0
static void fm_dump_instruments(FILE * f, FM_INSTRUMENT * inst, int count)
{
  int i = 0;
  for(; i < count ; i ++, inst ++)
  {
   unsigned char characteristic1;
   unsigned char characteristic2;
   unsigned char level1;
   unsigned char level2;
   unsigned char attackdecay1;
   unsigned char attackdecay2;
   unsigned char sustainrelease1;
   unsigned char sustainrelease2;
   unsigned char wave1;
   unsigned char wave2;
   unsigned char feedback;
   unsigned char freq;
   unsigned char key;
   unsigned char type;

    fprintf(f, "[%03d]: t=%d, chr1=%02x scl1=%02x atk1=%02x, sus1=%02x, wav1=%02x, "
      "chr2=%02x scl2=%02x atk2=%02x, sus2=%02x, wav2=%02x, "
      "fd=%02x key=%02x freq=%02x\n",
      i, inst->type,
      inst->characteristic1, inst->level1, inst->attackdecay1, inst->sustainrelease1, inst->wave1,
      inst->characteristic2, inst->level2, inst->attackdecay2, inst->sustainrelease2, inst->wave2,
      inst->feedback, inst->key, inst->freq);
  }
}
static END_OF_FUNCTION(fm_dump_instruments);
#endif

// ----------------------------------------------------------------------------------------------
// fm_reset: Resets the FM chip. If enable is set, puts OPL3 cards into OPL3 mode,
//  otherwise puts them into OPL2 emulation mode.
static void fm_reset(int enable)
{
   int i;

   for (i=0xF5; i>0; i--)
      fm_write(i, 0);

   if (midi_card == MIDI_OPL3) {          // if we have an OPL3...
      fm_delay_1 = 1;
      fm_delay_2 = 2;

      fm_write(0x105, 1);                 // enable OPL3 mode 

      fm_write(0x104, 0);                 

      for (i=0x1F5; i>0x105; i--)
	 fm_write(i, 0);

      for (i=0x104; i>0x100; i--)
	 fm_write(i, 0);

      if (!enable)
	 fm_write(0x105, 0);              // turn OPL3 mode off again 
   }
   else {
      fm_delay_1 = 6;
      fm_delay_2 = 35;

      if (midi_card == MIDI_2XOPL2) {     // if we have a second OPL2... 

	 for (i=0x1F5; i>0x100; i--)
	    fm_write(i, 0);

	 fm_write(0x101, 0x20); 
//         fm_write(0x1BD, 0xC0); 
         fm_write(0x1BD, 0x0); 
      }
   }

   for (i=0; i<midi_adlib.voices; i++) {
      fm_key[i] = 0;
      fm_keyscale1[i] = 0;
      fm_keyscale2[i] = 0;
      fm_feedback[i] = 0;
      fm_noteoffs[i] = 0;
#ifdef FM_SECONDARY
      fm_finetune[i] = 0;
#endif
      fm_write(0x40+fm_offset[i], 63);                                  
      fm_write(0x43+fm_offset[i], 63);
   }

   for (i=0; i<5; i++) {
      fm_drum_cached_inst1[i] = NULL;
      fm_drum_cached_inst2[i] = NULL;
      fm_drum_cached_vol1[i] = -1;
      fm_drum_cached_vol2[i] = -1;
      fm_drum_cached_time[i] = 0;
   }


   fm_write(0x01, 0x20);                  // turn on wave form control 

   fm_write(0x08, 0x40);                  // turn off CSW mode

   fm_drum_mode = FALSE;
//   fm_drum_mask = 0xC0;
   fm_drum_mask = 0;
   fm_write(0xBD, fm_drum_mask);          // set AM and vibrato to high 

   midi_adlib.xmin = -1;
   midi_adlib.xmax = -1;
#ifdef 0
   {
     FILE * f = fopen("instruments.txt", "wt");
     if(f)
       {
         fprintf(f, "*** INSTRUMENTS ***\n");
         fm_dump_instruments(f, fm_instrument, 128);
         fprintf(f, "*** DRUMS ***\n");
         fm_dump_instruments(f, fm_drum, 47);
         fclose(f);
       }
   }
#endif

}                           
static END_OF_FUNCTION(fm_reset);

// ----------------------------------------------------------------------------------------------
// fm_set_drum_mode: Switches the OPL synth between normal and percussion modes.
static void fm_set_drum_mode(int usedrums)
{
   int i;

   fm_drum_mode = usedrums;
   fm_drum_mask = usedrums ? 0xE0 : 0xC0;

   midi_adlib.xmin = usedrums ? 6 : -1;
   midi_adlib.xmax = usedrums ? 8 : -1;

   for (i=6; i<9; i++)
      if (midi_card == MIDI_OPL3)
	 fm_write(0xC0+VOICE_OFFSET(i), 0x30);
      else
	 fm_write(0xC0+VOICE_OFFSET(i), 0);

   fm_write(0xBD, fm_drum_mask);
}
static END_OF_FUNCTION(fm_set_drum_mode);

// ----------------------------------------------------------------------------------------------
// fm_set_voice:
//  Sets the sound to be used for the specified voice, from a structure
//  containing eleven bytes of FM operator data. Note that it doesn't
//  actually set the volume: it just stores volume data in the fm_level
//  arrays for fm_set_volume() to use.
static inline void fm_set_voice(int voice, FM_INSTRUMENT *inst)
{

   //fm_write(0x40+fm_offset[voice],0x3F);
   //fm_write(0x43+fm_offset[voice],0x3F);

   // store some info 
   fm_keyscale1[voice] = inst->level1 & 0xC0;
   fm_level1[voice] = 63 - (inst->level1 & 63);
   fm_keyscale2[voice] = inst->level2 & 0xC0;
   fm_level2[voice] = 63 - (inst->level2 & 63);
   fm_feedback[voice] = inst->feedback;
   fm_noteoffs[voice] = 0;
#ifdef FM_SECONDARY
   fm_finetune[voice] = 0;
#endif
   if(!inst->type)
   {
      fm_noteoffs[voice] = (int)(char)(inst->freq);
#ifdef FM_SECONDARY
      fm_finetune[voice] = (int)(char)(inst->key);
#endif
   }

   // write the new data 
   fm_write(0x20+fm_offset[voice], inst->characteristic1);
   fm_write(0x23+fm_offset[voice], inst->characteristic2);
   fm_write(0x60+fm_offset[voice], inst->attackdecay1);
   fm_write(0x63+fm_offset[voice], inst->attackdecay2);
   fm_write(0x80+fm_offset[voice], inst->sustainrelease1);
   fm_write(0x83+fm_offset[voice], inst->sustainrelease2);
   fm_write(0xE0+fm_offset[voice], inst->wave1);
   fm_write(0xE3+fm_offset[voice], inst->wave2);

   // don't set operator1 level for additive synthesis sounds
   if (!(inst->feedback & 1))
      fm_write(0x40+fm_offset[voice], inst->level1);

   // on OPL3, 0xC0 contains pan info, so don't set it until fm_key_on()
   if (midi_card != MIDI_OPL3)
      fm_write(0xC0+VOICE_OFFSET(voice), inst->feedback | 0x30);
}

// ----------------------------------------------------------------------------------------------
// fm_set_drum_op1: Sets the sound for operator #1 of a drum channel.
static inline void fm_set_drum_op1(int voice, FM_INSTRUMENT *inst)
{
   fm_write(0x20+fm_offset[voice], inst->characteristic1);
   fm_write(0x60+fm_offset[voice], inst->attackdecay1);
   fm_write(0x80+fm_offset[voice], inst->sustainrelease1);
   fm_write(0xE0+fm_offset[voice], inst->wave1);
}

// ----------------------------------------------------------------------------------------------
// fm_set_drum_op2: Sets the sound for operator #2 of a drum channel.
static inline void fm_set_drum_op2(int voice, FM_INSTRUMENT *inst)
{
   fm_write(0x23+fm_offset[voice], inst->characteristic2);
   fm_write(0x63+fm_offset[voice], inst->attackdecay2);
   fm_write(0x83+fm_offset[voice], inst->sustainrelease2);
   fm_write(0xE3+fm_offset[voice], inst->wave2);
}

// ----------------------------------------------------------------------------------------------
// fm_set_drum_vol_op1: Sets the volume for operator #1 of a drum channel.
static inline void fm_set_drum_vol_op1(int voice, int vol)
{
   vol = 63 * fm_vol_table[vol] / 128;
   fm_write(0x40+fm_offset[voice], (63-vol));
}

// ----------------------------------------------------------------------------------------------
// fm_set_drum_vol_op2: Sets the volume for operator #2 of a drum channel.
static inline void fm_set_drum_vol_op2(int voice, int vol)
{
   vol = 63 * fm_vol_table[vol] / 128;
   fm_write(0x43+fm_offset[voice], (63-vol));
}

// ----------------------------------------------------------------------------------------------
// fm_set_drum_pitch: Sets the pitch of a drum channel.
static inline void fm_set_drum_pitch(int voice, FM_INSTRUMENT *drum)
{
   fm_write(0xA0+VOICE_OFFSET(voice), drum->freq);
   fm_write(0xB0+VOICE_OFFSET(voice), drum->key & 0x1F);
}

// ----------------------------------------------------------------------------------------------
// fm_trigger_drum: Triggers a note on a drum channel.
static inline void fm_trigger_drum(int inst, int vol)
{
   FM_INSTRUMENT *drum = fm_drum+inst;
   int d;

   if (!fm_drum_mode)
      fm_set_drum_mode(TRUE);

        if (drum->type == FM_BD)
      d = 0;
   else if (drum->type == FM_SD)
      d = 1;
   else if (drum->type == FM_TT)
      d = 2;
   else if (drum->type == FM_CY)
      d = 3;
   else
      d = 4;

   // don't let drum sounds come too close together 
   if (fm_drum_cached_time[d] == _midi_tick) return;

   fm_drum_cached_time[d] = _midi_tick;

   fm_drum_mask &= (~drum->type);
   fm_write(0xBD, fm_drum_mask);

   vol = vol*3/4;

   if (fm_drum_op1[d]) {
      if (fm_drum_cached_inst1[d] != drum) {
	 fm_drum_cached_inst1[d] = drum;
	 fm_set_drum_op1(fm_drum_channel[d], drum);
      }

      if (fm_drum_cached_vol1[d] != vol) {
	 fm_drum_cached_vol1[d] = vol;
	 fm_set_drum_vol_op1(fm_drum_channel[d], vol);
      }
   }

   if (fm_drum_op2[d]) {
      if (fm_drum_cached_inst2[d] != drum) {
	 fm_drum_cached_inst2[d] = drum;
	 fm_set_drum_op2(fm_drum_channel[d], drum);
      }

      if (fm_drum_cached_vol2[d] != vol) {
	 fm_drum_cached_vol2[d] = vol;
	 fm_set_drum_vol_op2(fm_drum_channel[d], vol);
      }
   }

   fm_set_drum_pitch(fm_drum_channel[d], drum);

   fm_drum_mask |= drum->type;
   fm_write(0xBD, fm_drum_mask);
}

// ----------------------------------------------------------------------------------------------
// fm_key_on:
//  Triggers the specified voice. The instrument is specified as a GM
//  patch number, pitch as a midi note number, and volume from 0-127.
//  The bend parameter is _not_ expressed as a midi pitch bend value.
//  It ranges from 0 (no pitch change) to 0xFFF (almost a semitone sharp).
//  Drum sounds are indicated by passing an instrument number greater than
//  128, in which case the sound is GM percussion key #(inst-128).
static void fm_key_on(int inst, int note, int bend, int vol, int pan)
{
   int voice, druminst=-1;

          if (inst > 127 && inst < 1024) 
      {
		 druminst=inst-163;
         if (druminst < 0)       druminst = 0;
         else if (druminst > 46) druminst = 46;
	  }

/*   if (inst > 127) {                               // drum sound? 
      inst -= 163;
      if (inst < 0)
	 inst = 0;
      else if (inst > 46)
	 inst = 46;
      fm_trigger_drum(inst, vol);
   }
   else */

   {                                         // regular instrument 
      if (midi_card == MIDI_2XOPL2) {
	 // the SB Pro-1 has fixed pan positions per voice... 
	 if (pan < 64)
	    voice = _midi_allocate_voice(0, 5);
	 else
	    voice = _midi_allocate_voice(9, midi_driver->voices-1);
      }
      else
	 // on other cards we can use any voices 
	 voice = _midi_allocate_voice(-1, -1);

      if (voice < 0) return;

      // make sure the voice isn't sounding 
      fm_write(0x43+fm_offset[voice], 63);
//      if (fm_feedback[voice] & 1)
	  fm_write(0x40+fm_offset[voice], 63);


	  // make sure the voice is set up with the right sound 
      if (inst != fm_patch[voice]) {
     	 if (druminst==-1)
#ifdef FM_SECONDARY
           if (inst >= 1024)
             fm_set_voice(voice, fm_secondary + inst - 1024);
           else
#endif               
         fm_set_voice(voice, fm_instrument+inst); // GB 2014, not for drums
         else fm_set_voice(voice, fm_drum+druminst); // GB 2014, not for drums
    	 fm_patch[voice] = inst;
      }

      // set pan position 
      if (midi_card == MIDI_OPL3) 
	  {
             pan -= 64;
             if (pan <  -36) pan = 0x10; // left 
             else if (pan > 36) pan = 0x20; // right
             else                pan = 0x30; // both
         fm_write(0xC0+VOICE_OFFSET(voice), pan | fm_feedback[voice]);
      }

	  // GB 2014: Doom drums are normal instruments with fixed note (except two entries...)
	  if (inst > 127) 
      {
		 if (fm_drum[druminst].type>0) // fixed pitch=given note: thus freq+key already predefined
		 {
     	   fm_key[voice] = fm_drum[druminst].key; 
		   //set pitch locally:
		   fm_write(0xA0+VOICE_OFFSET(voice), fm_drum[druminst].freq);  
           fm_write(0xB0+VOICE_OFFSET(voice), fm_key[voice] | 0x20);
		   //fm_set_pitch(voice, fm_drum[druminst].type, 0);
		   //printf("DRUM: %02d  f=%02xh  k=%02xh v=%d  b=%d\n", druminst,fm_drum[druminst].freq, fm_drum[druminst].key, vol, bend);
    	 }
                 else fm_set_pitch(voice, note, bend);
                 fm_set_volume(voice, vol);
		 //fm_set_drum_vol_op1(voice, vol);
 	     return;
      }
      //printf("INST: %02d  v=%d  b=%d\n",inst, vol, bend);
      // and play the note 
      fm_set_pitch(voice, note, bend);
      fm_set_volume(voice, vol);
   }
}
static END_OF_FUNCTION(fm_key_on);

// ----------------------------------------------------------------------------------------------
// fm_key_off: Hey, guess what this does :-)
static void fm_key_off(int voice)
{
   fm_write(0xB0+VOICE_OFFSET(voice), fm_key[voice] & 0xDF);
}
static END_OF_FUNCTION(fm_key_off);

// ----------------------------------------------------------------------------------------------
// fm_set_volume: Sets the volume of the specified voice (vol range 0-127).
static void fm_set_volume(int voice, int vol)
{
   int _vol = fm_level2[voice] * fm_vol_table[vol] / 128;
   fm_write(0x43+fm_offset[voice], (63-_vol) | fm_keyscale2[voice]);
   _vol = fm_level1[voice] * fm_vol_table[vol] / 128;
   if (fm_feedback[voice] & 1)
      fm_write(0x40+fm_offset[voice], (63-vol) | fm_keyscale1[voice]);
   else
      fm_write(0x40+fm_offset[voice], (63-fm_level1[voice]) | fm_keyscale1[voice]);

}
static END_OF_FUNCTION(fm_set_volume);

// ----------------------------------------------------------------------------------------------

static unsigned short  freqtable[] = {					         // note # 
	345, 365, 387, 410, 435, 460, 488, 517, 547, 580, 615, 651,  //  0 
	690, 731, 774, 820, 869, 921, 975, 517, 547, 580, 615, 651,  // 12 
	690, 731, 774, 820, 869, 921, 975, 517, 547, 580, 615, 651,  // 24 
	690, 731, 774, 820, 869, 921, 975, 517, 547, 580, 615, 651,  // 36 
	690, 731, 774, 820, 869, 921, 975, 517, 547, 580, 615, 651,  // 48 
	690, 731, 774, 820, 869, 921, 975, 517, 547, 580, 615, 651,  // 60 
	690, 731, 774, 820, 869, 921, 975, 517, 547, 580, 615, 651,  // 72 
	690, 731, 774, 820, 869, 921, 975, 517, 547, 580, 615, 651,  // 84 
	690, 731, 774, 820, 869, 921, 975, 517, 547, 580, 615, 651,  // 96 
	690, 731, 774, 820, 869, 921, 975, 517, 547, 580, 615, 651,  // 108 
	690, 731, 774, 820, 869, 921, 975, 517};		             // 120 

static unsigned char octavetable[] = {			     // note # 
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,			     //  0 
	0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1,			     // 12 
	1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2,			     // 24 
	2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3,			     // 36 
	3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4,			     // 48 
	4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5,			     // 60 
	5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6,			     // 72 
	6, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7,			     // 84 
	7, 7, 7, 7, 7, 7, 7, 8, 8, 8, 8, 8,			     // 96 
	8, 8, 8, 8, 8, 8, 8, 9, 9, 9, 9, 9,			     // 108 
	9, 9, 9, 9, 9, 9, 9,10};				         // 120 

static unsigned short  pitchtable[] = {				    // pitch wheel 
	 29193U,29219U,29246U,29272U,29299U,29325U,29351U,29378U,  // -128 
	 29405U,29431U,29458U,29484U,29511U,29538U,29564U,29591U,  // -120 
	 29618U,29644U,29671U,29698U,29725U,29752U,29778U,29805U,  // -112 
	 29832U,29859U,29886U,29913U,29940U,29967U,29994U,30021U,  // -104 
	 30048U,30076U,30103U,30130U,30157U,30184U,30212U,30239U,  //  -96 
	 30266U,30293U,30321U,30348U,30376U,30403U,30430U,30458U,  //  -88 
	 30485U,30513U,30541U,30568U,30596U,30623U,30651U,30679U,  //  -80 
	 30706U,30734U,30762U,30790U,30817U,30845U,30873U,30901U,  //  -72 
	 30929U,30957U,30985U,31013U,31041U,31069U,31097U,31125U,  //  -64 
	 31153U,31181U,31209U,31237U,31266U,31294U,31322U,31350U,  //  -56 
	 31379U,31407U,31435U,31464U,31492U,31521U,31549U,31578U,  //  -48 
	 31606U,31635U,31663U,31692U,31720U,31749U,31778U,31806U,  //  -40 
	 31835U,31864U,31893U,31921U,31950U,31979U,32008U,32037U,  //  -32 
	 32066U,32095U,32124U,32153U,32182U,32211U,32240U,32269U,  //  -24 
	 32298U,32327U,32357U,32386U,32415U,32444U,32474U,32503U,  //  -16 
	 32532U,32562U,32591U,32620U,32650U,32679U,32709U,32738U,  //   -8 
	 32768U,32798U,32827U,32857U,32887U,32916U,32946U,32976U,  //    0 
	 33005U,33035U,33065U,33095U,33125U,33155U,33185U,33215U,  //    8 
	 33245U,33275U,33305U,33335U,33365U,33395U,33425U,33455U,  //   16 
	 33486U,33516U,33546U,33576U,33607U,33637U,33667U,33698U,  //   24 
	 33728U,33759U,33789U,33820U,33850U,33881U,33911U,33942U,  //   32 
	 33973U,34003U,34034U,34065U,34095U,34126U,34157U,34188U,  //   40 
	 34219U,34250U,34281U,34312U,34343U,34374U,34405U,34436U,  //   48 
	 34467U,34498U,34529U,34560U,34591U,34623U,34654U,34685U,  //   56 
	 34716U,34748U,34779U,34811U,34842U,34874U,34905U,34937U,  //   64 
	 34968U,35000U,35031U,35063U,35095U,35126U,35158U,35190U,  //   72 
	 35221U,35253U,35285U,35317U,35349U,35381U,35413U,35445U,  //   80 
	 35477U,35509U,35541U,35573U,35605U,35637U,35669U,35702U,  //   88 
	 35734U,35766U,35798U,35831U,35863U,35895U,35928U,35960U,  //   96 
	 35993U,36025U,36058U,36090U,36123U,36155U,36188U,36221U,  //  104 
	 36254U,36286U,36319U,36352U,36385U,36417U,36450U,36483U,  //  112 
	 36516U,36549U,36582U,36615U,36648U,36681U,36715U,36748U}; //  120 

// ----------------------------------------------------------------------------------------------

// GB 2014: from MUSlib MLOPL.C, sounds more authentic.
static void fm_set_pitch(int voice, int note, int bend)
{
   unsigned int freq = 0;
   unsigned int octave = 0;

   note += fm_noteoffs[voice];
   if(note < 0)
     note = 0;
   else if(note > 127)
     note = 127;

   freq = freqtable[note];
   octave = octavetable[note];

   bend >>= 6;
#ifdef FM_SECONDARY
   if(fm_finetune[voice])
     bend += fm_finetune[voice];
#endif

   if (bend) //Allegro: bend ranges from 0 (no pitch change) to 0xFFF (almost a semitone sharp) = 4096
   {
	  // original muslib method:
	  if (bend > 127) bend = 127;
      else if (bend < -128) bend = -128;
	  freq = ((unsigned long)freq * pitchtable[bend + 128]) >> 15;
      // allegro method, but done after octave divide?
	  //freq += (freqtable[note+1] - freqtable[note]) * bend / 0x1000;

	  if (freq >= 1024)
	  {
	      freq >>= 1;
	      octave++;
	  }
   }
   if (octave > 7) octave = 7;
    
   fm_key[voice] = (octave<<2) | (freq >> 8);

   fm_write(0xA0+VOICE_OFFSET(voice), freq & 0xFF); 
   fm_write(0xB0+VOICE_OFFSET(voice), fm_key[voice] | 0x20);
}
static END_OF_FUNCTION(fm_set_pitch);

/*
// ----------------------------------------------------------------------------------------------
// GB 2014: modified as to the example of the Duke Nukem 3D adlib driver, 
// Cannot tell why exactly, but the original procedure sounded bugged for sure.
// bugged section: note-=24; while (note>=12) {note-=12; oct++;}
static void fm_set_pitch(int voice, int note, int bend)
{
   int octave=1;
   int pitch;
   int freq;

   //note -= 24;       // won't sound right
   octave = note / 12; // add lookup table for speed? does not seam to cause any fps drop on a 486
   note   = note % 12; // add lookup table for speed? does not seam to cause any fps drop on a 486

   freq = fm_freq[note]; // 0..12: 0x157, 0x16B, 0x181, 0x198, 0x1B0, 0x1CA, 0x1E5, 0x202, 0x220, 0x241, 0x263, 0x287, 0x2AE

   if (bend)
     freq += (fm_freq[note+1] - fm_freq[note]) * bend / 0x1000;

   fm_key[voice] = (octave<<2) | (freq >> 8);

   fm_write(0xA0+VOICE_OFFSET(voice), freq & 0xFF); 
   fm_write(0xB0+VOICE_OFFSET(voice), fm_key[voice] | 0x20);

}
static END_OF_FUNCTION(fm_set_pitch);
*/
/*
// ----------------------------------------------------------------------------------------------
// GB 2014: this is working, very similar to Duke 3D, but does not process 'bend'.
static unsigned OctavePitch[8] ={0x0000, 0x0400, 0x0800, 0x0C00, 0x1000, 0x1400, 0x1800, 0x1C00};
static void fm_set_pitch_alt(int voice, int note, int bend)
{
   int octave=1;
   int pitch;

   octave = note / 12; // add lookup table for speed?
   note   = note % 12; // add lookup table for speed?

   pitch = OctavePitch[ octave ] | fm_freq[ note ];

   fm_write(0xA0+VOICE_OFFSET(voice), pitch & 0xFF); 
   fm_write(0xB0+VOICE_OFFSET(voice), (pitch >> 8) | 0x20);
}
static END_OF_FUNCTION(fm_set_pitch);


// fm_set_pitch: Sets the pitch of the specified voice.
static void fm_set_pitch(int voice, int note, int bend)
{
   int oct = 1;
   int freq;

   note -= 24;
   while (note >= 12) {
      note -= 12;
      oct++;
   }

   freq = fm_freq[note]; 

   if (bend)
     freq += (fm_freq[note+1] - fm_freq[note]) * bend / 0x1000;

   fm_key[voice] = (oct<<2) | (freq >> 8);


   fm_write(0xA0+VOICE_OFFSET(voice), freq & 0xFF); 
   fm_write(0xB0+VOICE_OFFSET(voice), fm_key[voice] | 0x20);
   
}
static END_OF_FUNCTION(fm_set_pitch);
*/


// ----------------------------------------------------------------------------------------------
// fm_load_patches: Called before starting to play a MIDI file, to check if we need to be
// in rhythm mode or not.
static int fm_load_patches(char *patches, char *drums)
{
   int i;
   int usedrums = FALSE;

   for (i=6; i<9; i++) {
      fm_key[i] = 0;
      fm_keyscale1[i] = 0;
      fm_keyscale2[i] = 0;
      fm_feedback[i] = 0;
      fm_noteoffs[i] = 0;
#ifdef FM_SECONDARY
      fm_finetune[i] = 0;
#endif
      fm_level1[i] = 0;
      fm_level2[i] = 0;
      fm_patch[i] = -1;
      fm_write(0x40+fm_offset[i], 63);
      fm_write(0x43+fm_offset[i], 63);
   }

   for (i=0; i<5; i++) {
      fm_drum_cached_inst1[i] = NULL;
      fm_drum_cached_inst2[i] = NULL;
      fm_drum_cached_vol1[i] = -1;
      fm_drum_cached_vol2[i] = -1;
      fm_drum_cached_time[i] = 0;
   }

   for (i=0; i<128; i++) {
      if (drums[i]) {
	 usedrums = TRUE;
	 break;
      }
   }

   usedrums = FALSE; // GB 2014, doom does not use drums mode

   fm_set_drum_mode(usedrums);

   return 0;
}

static END_OF_FUNCTION(fm_load_patches);



// ----------------------------------------------------------------------------------------------
// fm_mixer_volume: For SB-Pro cards, sets the mixer volume for FM output.
static int fm_mixer_volume(int volume)
{
   return _sb_set_mixer(-1, volume);
}

// ----------------------------------------------------------------------------------------------
// fm_is_there: Checks for the presence of an OPL synth at the current port.
static int fm_is_there()
{
   fm_write(1, 0);                        // init test register 

   fm_write(4, 0x60);                     // reset both timers 
   fm_write(4, 0x80);                     // enable interrupts 

   if (inportb(_fm_port) & 0xE0)
      return FALSE;

   fm_write(2, 0xFF);                     // write 0xFF to timer 1 
   fm_write(4, 0x21);                     // start timer 1

   rest(100);

   if ((inportb(_fm_port) & 0xE0) != 0xC0)
      return FALSE;

   fm_write(4, 0x60);                     // reset both timers
   fm_write(4, 0x80);                     // enable interrupts

   return TRUE;
}

// ----------------------------------------------------------------------------------------------
// fm_detect: Adlib detection routine.
static int fm_detect()
{

   char *envptr = getenv("ULTRASND"); // Ultrasound bugfix
   int ultrasoundport= 0x999;
   static int ports[] = { 0x210, 0x220, 0x230, 0x240, 0x250, 0x260, 0x388, 0 };
   int i;
   char *s;
   int opl_type;

   // Prevent Gravis Ultrasound corruption: parse ULTRASND env, example: SET ULTRASND=240,7,6,5,5. GB 2017
   if (envptr) ultrasoundport = strtol(envptr, NULL, 16);   

   if (_fm_port < 0) 
   {
      if (midi_card == MIDI_OPL2) 
	  {
	    _fm_port = 0x388;
  	    if (fm_is_there())
	    goto found_it;
      }

      for (i=0; ports[i]; i++)           // find the card 
	  {
	    _fm_port = ports[i];
	    if (_fm_port != ultrasoundport)  // GB 2017, Prevent Gravis Ultrasound corruption
	    {
          if (fm_is_there()) goto found_it;
        }
	  }
   }

   if (!fm_is_there()) {
      strcpy(allegro_error, "OPL synth not found");
      return FALSE;
   }

   found_it:

   if ((inportb(_fm_port) & 6) == 0) {    // check for OPL3 
      opl_type = MIDI_OPL3;
      _sb_read_dsp_version();
   }
   else {                                 // check for second OPL2 
      if (_sb_read_dsp_version() >= 0x300)
	 opl_type = MIDI_2XOPL2;
      else
	 opl_type = MIDI_OPL2;
   }

   if (midi_card == MIDI_OPL3) {
      if (opl_type != MIDI_OPL3) {
	 strcpy(allegro_error, "OPL3 synth not found");
	 return FALSE;
      }
   }
   else if (midi_card == MIDI_2XOPL2) {
      if (opl_type != MIDI_2XOPL2) {
	 strcpy(allegro_error, "Second OPL2 synth not found");
	 return FALSE;
      }
   }
   else if (midi_card != MIDI_OPL2)
      midi_card = opl_type;

   if (midi_card == MIDI_OPL2)
      s = "Adlib / OPL2 FM synth";  // GB 2016
   else if (midi_card == MIDI_2XOPL2)
      s = "Dual OPL2 FM synths";
   else
	  s = "OPL3 FM synth"; 
   
   strcpy(midi_adlib.name,s); // GB 2016, do not exceed original string length!
   sprintf(adlib_desc, "%s on port %Xh", s, _fm_port);
   
   midi_adlib.voices = (midi_card == MIDI_OPL2) ? 9 : 18;
   midi_adlib.def_voices = midi_adlib.max_voices = midi_adlib.voices;

   return TRUE;
}

// ----------------------------------------------------------------------------------------------
// load_ibk: Reads in a .IBK patch set file, for use by the Adlib driver
int load_ibk(char *filename, int drums)
{
   char sig[4];
   FM_INSTRUMENT *inst;
   int c, note, oct, skip, count, trans;

   PACKFILE *f = pack_fopen(filename, F_READ);
   if (!f)
      return -1;

   pack_fread(sig, 4, f);
   if (memcmp(sig, "IBK\x1A", 4) != 0) {
      pack_fclose(f);
      return -1;
   }

   if (drums) {
      inst = fm_drum;
      skip = 35;
      count = 47;
   }
   else {
      inst = fm_instrument;
      skip = 0;
      count = 128;
#ifdef FM_SECONDARY
      memset(fm_secondary, 0, sizeof(FM_INSTRUMENT) * (128 + 47));
#endif
   }

   for (c=0; c<skip*16; c++)
      pack_getc(f);

   for (c=0; c<count; c++) {
      inst->characteristic1 = pack_getc(f);
      inst->characteristic2 = pack_getc(f);
      inst->level1 = pack_getc(f);
      inst->level2 = pack_getc(f);
      inst->attackdecay1 = pack_getc(f);
      inst->attackdecay2 = pack_getc(f);
      inst->sustainrelease1 = pack_getc(f);
      inst->sustainrelease2 = pack_getc(f);
      inst->wave1 = pack_getc(f);
      inst->wave2 = pack_getc(f);
      inst->feedback = pack_getc(f);

      if (drums) {
	 switch (pack_getc(f)) {
	    case 6:  inst->type = FM_BD;  break;
	    case 7:  inst->type = FM_HH;  break;
	    case 8:  inst->type = FM_TT;  break;
	    case 9:  inst->type = FM_SD;  break;
	    case 10: inst->type = FM_CY;  break;
	    default: inst->type = 0;      break;
	 }

         inst->freq = (char)pack_getc(f) - 12;
         inst->key = 0;
         note = pack_getc(f);
	 oct = 1;

         if(inst->type)
         {
           note += (int)(char)inst->freq;
           oct  = note / 12; // GB 2014, Duke 3d method, not the MUSlib method!
           note = note % 12; 

           // note -= 24;
           // while (note >= 12) {
           //    note -= 12;
           //    oct++;
           // }

           inst->freq = fm_freq[note];
           inst->key = (oct<<2) | (fm_freq[note] >> 8);
         }
      }
      else {
	 inst->type = 0;
	 inst->key = 0;

         pack_getc(f);
         inst->freq = (char)pack_getc(f) - 12;
	 pack_getc(f);
      }

      pack_getc(f);
      pack_getc(f);

      inst++;
   }
   pack_fclose(f);
   return 0;
}


int load_op2_buffer(char *data, char * secondaries)
{
   FM_INSTRUMENT *inst;
#ifdef FM_SECONDARY
   FM_INSTRUMENT *secondary;
#endif

   int c, count, scale;

   if (memcmp(data, "#OPL_II##", 8) != 0) {
      return -1;
   }

   data += 8;

   inst = fm_instrument;
#ifdef FM_SECONDARY
   secondary = fm_secondary;
#endif
   count = 128 + 47;

#ifdef FM_SECONDARY
   memset(fm_secondary, 0, sizeof(FM_INSTRUMENT) * 128);
#endif
   memset(fm_instrument, 0, sizeof(FM_INSTRUMENT) * 128);
   memset(fm_drum, 0, sizeof(FM_INSTRUMENT) * 47);

   for (c=0; c<count; c++) {
      short offs = 0;
      unsigned char note;
      char tune;
      int has_secondary;

      if(count < 128) secondaries[count] = 0;

      inst->type = (*data)&1;
#ifdef FM_SECONDARY
      has_secondary = (*data)&4;
#endif
      data += 2;
      tune = *(((char*)data++));
      note = *(data++);

      inst->characteristic1 = *(data++);
      inst->attackdecay1 = *(data++);
      inst->sustainrelease1 = *(data++);
      inst->wave1 = *(data++);
      scale = *(data++); //keyscale
      inst->level1 = (*(data++) & 63) | (scale);

      inst->feedback = *(data++);

      inst->characteristic2 = *(data++);
      inst->attackdecay2 = *(data++);
      inst->sustainrelease2 = *(data++);
      inst->wave2 = *(data++);
      scale = *(data++); //keyscale
      inst->level2 = (*(data++) & 63) | (scale);

      data++; //unused

      offs = *(data++);
      offs |= (*(data++) << 8);

      inst->key = 0;
      inst->freq = offs;
      if(c < 128)
      {
        inst->type = 0;
      }
      else
      {
        if(inst->type)
        {
          int oct = 1;
          int _note = note + offs;
          oct  = _note / 12; // GB 2014, Duke 3d method, not the MUSlib method!
          _note = _note % 12; 

          inst->freq = fm_freq[_note];
          inst->key = (oct<<2) | (fm_freq[_note] >> 8);
        }
      }

#ifdef FM_SECONDARY
      if(has_secondary && c < 128)
      {
        secondaries[c] = 1;
        secondary->characteristic1 = *(data++);
        secondary->attackdecay1 = *(data++);
        secondary->sustainrelease1 = *(data++);
        secondary->wave1 = *(data++);
        scale = *(data++); //keyscale
        secondary->level1 = (*(data++) & 63) | (scale);
     
        secondary->feedback = *(data++);

        secondary->characteristic2 = *(data++);
        secondary->attackdecay2 = *(data++);
        secondary->sustainrelease2 = *(data++);
        secondary->wave2 = *(data++);
        scale = *(data++); //keyscale
        secondary->level2 = (*(data++) & 63) | (scale);

        data++; //unused

        offs = *(data++);
        offs |= (*(data++) << 8);

        secondary->key = tune - 0x80;
        secondary->freq = offs;
        secondary->type = 0;
      }
      else
      {
        if(c < 128) memcpy(secondary, inst, sizeof(*inst));
        data += 16;
      }
#else
      data += 16;
#endif
      inst++;
#ifdef FM_SECONDARY
      secondary++;
#endif
      if(c == 127)
        inst = fm_drum;
   }
   return 0;
}

// ----------------------------------------------------------------------------------------------
// load_op2: Reads in a .OP2 patch set file, for use by the Adlib driver
int load_op2_extra(char *filename, char *secondaries)
{
   char sig[8];
   FM_INSTRUMENT *inst;
   int c, count, scale;
   PACKFILE *f;

   if(*filename == '#')
     return load_op2_buffer(filename, secondaries);

   f = pack_fopen(filename, F_READ);
   if (!f)
      return -1;

   pack_fread(sig, 8, f);
   if (memcmp(sig, "#OPL_II##", 8) != 0) {
      pack_fclose(f);
      return -1;
   }

   inst = fm_instrument;
   count = 128 + 47;

#ifdef FM_SECONDARY
   memset(fm_secondary, 0, sizeof(FM_INSTRUMENT) * (128 + 47));
#endif
   memset(fm_instrument, 0, sizeof(FM_INSTRUMENT) * 128);
   memset(fm_drum, 0, sizeof(FM_INSTRUMENT) * 47);

   for (c=0; c<count; c++) {
      short offs = 0;
      unsigned char note;

      inst->type = 1 &  pack_getc(f); //header
      pack_getc(f);

      pack_getc(f); //finetune

      note = pack_getc(f); //note


      inst->characteristic1 = pack_getc(f);
      inst->attackdecay1 = pack_getc(f);
      inst->sustainrelease1 = pack_getc(f);
      inst->wave1 = pack_getc(f);
      scale = pack_getc(f); //keyscale
      inst->level1 = (pack_getc(f) & 63) | (scale);

      inst->feedback = pack_getc(f);

      inst->characteristic2 = pack_getc(f);
      inst->attackdecay2 = pack_getc(f);
      inst->sustainrelease2 = pack_getc(f);
      inst->wave2 = pack_getc(f);
      scale = pack_getc(f); //keyscale
      inst->level2 = (pack_getc(f) & 63) | (scale);

      pack_getc(f); //unused

      offs = pack_getc(f);
      offs |= (pack_getc(f) << 8);

      inst->key = 0;
      inst->freq = offs;
      if(c < 128)
      {
        inst->type = 0;
      }
      else
      {
        if(inst->type)
        {
          int oct = 1;
          int _note = note + offs;
          oct  = _note / 12; // GB 2014, Duke 3d method, not the MUSlib method!
          _note = _note % 12; 

          inst->freq = fm_freq[_note];
          inst->key = (oct<<2) | (fm_freq[_note] >> 8);
        }
      }

      pack_getc(f);
      pack_getc(f);
      pack_getc(f);
      pack_getc(f);
      pack_getc(f);
      pack_getc(f);

      pack_getc(f);

      pack_getc(f);
      pack_getc(f);
      pack_getc(f);
      pack_getc(f);
      pack_getc(f);
      pack_getc(f);

      pack_getc(f);

      pack_getc(f);
      pack_getc(f);

      inst++;
      if(c == 127)
        inst = fm_drum;
   }
   pack_fclose(f);
   return 0;
}

int load_op2(char *filename)
{
  char secondaries[256];
  return load_op2_extra(filename, secondaries);
}

// ----------------------------------------------------------------------------------------------
// fm_init: Setup the adlib driver.
static int fm_init(int voices)
{
   char *s;
   int i;

   fm_reset(1);

#ifdef FM_SECONDARY
   memset(fm_secondary, 0, sizeof(FM_INSTRUMENT) * (128 + 47));
#endif

   for (i=0; i<2; i++) 
   {
      s = get_config_string("sound", ((i == 0) ? "ibk_file" : "ibk_drum_file"), NULL);
      if ((s) && (s[0])) 
	  {
	     if (load_ibk(s, (i > 0)) != 0) 
		 {
	        sprintf(allegro_error, "Error reading .IBK file '%s'", s);
	        return -1;
  	     }
      }
   }

   LOCK_VARIABLE(midi_adlib);
   LOCK_VARIABLE(fm_instrument);
   LOCK_VARIABLE(fm_drum);
#ifdef FM_SECONDARY
   LOCK_VARIABLE(fm_secondary);
#endif
   LOCK_VARIABLE(_fm_port);
   LOCK_VARIABLE(fm_offset);
   LOCK_VARIABLE(fm_freq);
   LOCK_VARIABLE(fm_vol_table);
   LOCK_VARIABLE(fm_drum_channel);
   LOCK_VARIABLE(fm_drum_op1);
   LOCK_VARIABLE(fm_drum_op2);
   LOCK_VARIABLE(fm_drum_pitch);
   LOCK_VARIABLE(fm_drum_cached_inst1);
   LOCK_VARIABLE(fm_drum_cached_inst2);
   LOCK_VARIABLE(fm_drum_cached_vol1);
   LOCK_VARIABLE(fm_drum_cached_vol2);
   LOCK_VARIABLE(fm_drum_cached_time);
   LOCK_VARIABLE(fm_drum_mask);
   LOCK_VARIABLE(fm_drum_mode);
   LOCK_VARIABLE(fm_key);
   LOCK_VARIABLE(fm_keyscale1);
   LOCK_VARIABLE(fm_keyscale2);
   LOCK_VARIABLE(fm_feedback);
   LOCK_VARIABLE(fm_noteoffs);
#ifdef FM_SECONDARY
   LOCK_VARIABLE(fm_finetune);
#endif
   LOCK_VARIABLE(fm_level1);
   LOCK_VARIABLE(fm_level2);
   LOCK_VARIABLE(fm_patch);
   LOCK_VARIABLE(fm_delay_1);
   LOCK_VARIABLE(fm_delay_2);
   LOCK_FUNCTION(fm_write);
   LOCK_FUNCTION(fm_reset);
   LOCK_FUNCTION(fm_set_drum_mode);
   LOCK_FUNCTION(fm_key_on);
   LOCK_FUNCTION(fm_key_off);
   LOCK_FUNCTION(fm_set_volume);
   LOCK_FUNCTION(fm_set_pitch);
   LOCK_FUNCTION(fm_load_patches);

   return 0;
}

// ----------------------------------------------------------------------------------------------
// fm_exit: Cleanup when we are finished.
static void fm_exit()
{
   fm_reset(0);
}

