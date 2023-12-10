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
 *      Soundblaster driver. Supports DMA driven sample playback (mixing 
 *      up to eight samples at a time) and raw note output to the SB MIDI 
 *      port. The Adlib (FM synth) MIDI playing code is in adlib.c.
 *
 *      See readme.txt for copyright information.
 *
 *      ----------------------------------------------------------------------------------------
 *
 *      GB 2013: This is a modified Allegro V3.0 driver: SB.C + DMA.C + IRQ.C + INTERNDJ.C ( + SETUP.C )
 *      Uses DMA/IRQ routines of V3.12, but without the recording stuff.
 *      Has adjusted frequencies, because they were rather high, and it gave problems (CS4232 card).
 *      Has Adjusted SB 2.0 Init and Exit routines, to clear the DMA, for running games afterwards.
 *      Has Adjusted SB Pro Exit routine, to clear the DMA, for running games afterwards.
 *
 *      The current Allegro for DOS v4.2.3.1 would also benefit of the fixes here, 
 *      and in addition requires the SB.C fixes by Ron Novy on 6-10-2008 at allegro.cc forums.
 */

#ifndef DJGPP
#error This file should only be used by the djgpp version of Allegro
#endif

#include <stdlib.h>
#include <stdio.h>
#include <dos.h>
#include <go32.h>
#include <dpmi.h>
#include <limits.h>
#include <sys/farptr.h>

#include "allegro.h"
#include "internal.h"

// external interface to the digital SB driver 
static int  sb_detect();
static int  sb_init(int voices);
static void sb_exit();
static int  sb_mixer_volume(int volume);
static char sb_desc[80] = "not initialised";

DIGI_DRIVER digi_sb =
{ 
   "Sound Blaster Family", 
   sb_desc,
   0, 0, MIXER_MAX_SFX, MIXER_DEF_SFX,
   sb_detect,
   sb_init,
   sb_exit,
   sb_mixer_volume,
   _mixer_init_voice,
   _mixer_release_voice,
   _mixer_start_voice,
   _mixer_stop_voice,
   _mixer_loop_voice,
   _mixer_get_position,
   _mixer_set_position,
   _mixer_get_volume,
   _mixer_set_volume,
   _mixer_ramp_volume,
   _mixer_stop_volume_ramp,
   _mixer_get_frequency,
   _mixer_set_frequency,
   _mixer_sweep_frequency,
   _mixer_stop_frequency_sweep,
   _mixer_get_pan,
   _mixer_set_pan,
   _mixer_sweep_pan,
   _mixer_stop_pan_sweep,
   _mixer_set_echo,
   _mixer_set_tremolo,
   _mixer_set_vibrato
};

// external interface to the SB midi output driver 
static int  sb_midi_init(int voices);
static void sb_midi_exit();
static void sb_midi_output(unsigned char data);
static char sb_midi_desc[80] = "not initialised";

MIDI_DRIVER midi_sb_out =
{
   "SB MIDI interface", 
   sb_midi_desc,
   0, 0, 0xFFFF, 0, -1, -1,
   sb_detect,
   sb_midi_init,
   sb_midi_exit,
   NULL,
   sb_midi_output,
   _dummy_load_patches,
   _dummy_adjust_patches,
   _dummy_key_on,
   _dummy_noop1,
   _dummy_noop2,
   _dummy_noop3,
   _dummy_noop2,
   _dummy_noop2
};

static int sb_in_use = FALSE;             /* is SB being used? */
static int sb_stereo = FALSE;             /* in stereo mode? */
static int sb_16bit = FALSE;              /* in 16 bit mode? */
static int sb_int = -1;                   /* interrupt vector */
static int sb_dsp_ver = -1;               /* SB DSP version */
static int sb_hw_dsp_ver = -1;            /* as reported by autodetect */
static int sb_dma_size = -1;              /* size of dma transfer in bytes */
static int sb_dma_mix_size = -1;          /* number of samples to mix */
static int sb_dma_count = 0;              /* need to resync with dma? */
static volatile int sb_semaphore = FALSE; /* reentrant interrupt? */

static int sb_sel[2];                     /* selectors for the buffers */
static unsigned long sb_buf[2];           /* pointers to the two buffers */
static int sb_bufnum = 0;                 /* the one currently in use */

static int sb_dma8 = -1;                  /* 8-bit DMA channel (SB16) */
static int sb_dma16 = -1;                 /* 16-bit DMA channel (SB16) */

static int sb_master_vol = -1;            /* stored mixer settings */
static int sb_digi_vol = -1;
static int sb_fm_vol = -1;

//-------------------------------------------------------------------------------------

// sb_read_dsp: Reads a byte from the SB DSP chip. Returns -1 if it times out.
static inline volatile int sb_read_dsp()
{
   int x;

   for (x=0; x<0xFFFF; x++)
      if (inportb(0x0E + _sb_port) & 0x80)
	 return inportb(0x0A+_sb_port);

   return -1; 
}

//-------------------------------------------------------------------------------------

// sb_write_dsp: Writes a byte to the SB DSP chip. Returns -1 if it times out.
static inline volatile int sb_write_dsp(unsigned char byte)
{
   int x;

   for (x=0; x<0xFFFF; x++) {
      if (!(inportb(0x0C+_sb_port) & 0x80)) {
	 outportb(0x0C+_sb_port, byte);
	 return 0;
      }
   }
   return -1; 
}

//-------------------------------------------------------------------------------------

// _sb_voice: Turns the SB speaker on or off.
void _sb_voice(int state)
{
   if (state) {
      sb_write_dsp(0xD1);

      if (sb_hw_dsp_ver >= 0x300) {       /* set up the mixer */

	 if (sb_master_vol < 0) {
	    outportb(_sb_port+4, 0x22);   /* store master volume */
	    sb_master_vol = inportb(_sb_port+5);
	 }

	 if (sb_digi_vol < 0) {
	    outportb(_sb_port+4, 4);      /* store DAC level */
	    sb_digi_vol = inportb(_sb_port+5);
	 }

	 if (sb_fm_vol < 0) {
	    outportb(_sb_port+4, 0x26);   /* store FM level */
	    sb_fm_vol = inportb(_sb_port+5);
	 }
      }
   }
   else {
      sb_write_dsp(0xD3);

      if (sb_hw_dsp_ver >= 0x300) {       /* reset previous mixer settings */

	 outportb(_sb_port+4, 0x22);      /* restore master volume */
	 outportb(_sb_port+5, sb_master_vol);

	 outportb(_sb_port+4, 4);         /* restore DAC level */
	 outportb(_sb_port+5, sb_digi_vol);

	 outportb(_sb_port+4, 0x26);      /* restore FM level */
	 outportb(_sb_port+5, sb_fm_vol);
      }
   }
}

//-------------------------------------------------------------------------------------

// _sb_set_mixer: Alters the SB-Pro hardware mixer.
int _sb_set_mixer(int digi_volume, int midi_volume)
{
   if (sb_hw_dsp_ver < 0x300)
      return -1;

   if (digi_volume >= 0) {                   /* set DAC level */
      outportb(_sb_port+4, 4);
      outportb(_sb_port+5, (digi_volume & 0xF0) | (digi_volume >> 4));
   }

   if (midi_volume >= 0) {                   /* set FM level */
      outportb(_sb_port+4, 0x26);
      outportb(_sb_port+5, (midi_volume & 0xF0) | (midi_volume >> 4));
   }

   return 0;
}

//-------------------------------------------------------------------------------------

// sb_mixer_volume: Sets the SB mixer volume for playing digital samples.
static int sb_mixer_volume(int volume)
{
   return _sb_set_mixer(volume, -1);
}

//-------------------------------------------------------------------------------------

// sb_stereo_mode: Enables or disables stereo output for SB-Pro.
static void sb_stereo_mode(int enable)
{
   outportb(_sb_port+0x04, 0x0E); 
   outportb(_sb_port+0x05, (enable ? 2 : 0));
}

//-------------------------------------------------------------------------------------

// sb_set_sample_rate: The parameter is the rate to set in Hz (samples per second).
static void sb_set_sample_rate(unsigned int rate)
{
   if (sb_16bit) {
      sb_write_dsp(0x41);
      sb_write_dsp(rate >> 8);
      sb_write_dsp(rate & 0xFF);
   }
   else {
      if (sb_stereo)
		 rate *= 2;
      /* GB 2013: over 43500 gives problems on some compatibles (CS4232) */
      if (rate > 43478) 
		  rate=43478;
      sb_write_dsp(0x40);
      sb_write_dsp((unsigned char)(256-1000000/rate));
   }
}

//-------------------------------------------------------------------------------------

// _sb_reset_dsp: Resets the SB DSP chip, returning -1 on error.
int _sb_reset_dsp(int data)
{
   int x;

   if ((_sb_port < 0) || (_sb_port>0x400)) return -1; // GB 2015

   outportb(0x06+_sb_port, data);

   for (x=0; x<8; x++)
      inportb(0x06+_sb_port);

   outportb(0x06+_sb_port, 0);

   if (sb_read_dsp() != 0xAA)
      return -1;

   return 0;
}

//-------------------------------------------------------------------------------------

// _sb_read_dsp_version: Reads the version number of the SB DSP chip, returning -1 on error.
int _sb_read_dsp_version()
{
   int major, minor;

   if (sb_hw_dsp_ver > 0)
      return sb_hw_dsp_ver;

   if (_sb_port <= 0)
      _sb_port = 0x220;

   if (_sb_reset_dsp(1) != 0) {
      sb_hw_dsp_ver = -1;
   }
   else {
      sb_write_dsp(0xE1);
      major = sb_read_dsp();
      minor = sb_read_dsp();
      //sb_hw_dsp_ver = ((major << 8) | minor);
	  sb_hw_dsp_ver = (major << 8);  // GB 2017, above not working?
	  sb_hw_dsp_ver += minor;
   }

   return sb_hw_dsp_ver;
}

//-------------------------------------------------------------------------------------

// sb_play_buffer: Starts a dma transfer of size bytes. On cards capable of it, the transfer
// will use auto-initialised dma, so there is no need to call this routine more than once. 
// On older cards it must be called from the end-of-buffer handler to switch to the new buffer.
static void sb_play_buffer(int size)
{
   if (sb_dsp_ver < 0x200) {               /* 8 bit single-shot */    // SB 1.0 and older SB 1.5
      sb_write_dsp(0x14); // 14h = 8-bit OCM output, mono single cycle.
      sb_write_dsp((size-1) & 0xFF);
      sb_write_dsp((size-1) >> 8);
   }                                       
   else if ((sb_dsp_ver < 0x300) && (_sb_freq<=22222)) { 
	  sb_write_dsp(0x48);                  // GB 2013, Quake one method for v2 DSP, now use instead of above. SB 1.5 new and SB 2.0  
	  sb_write_dsp((size-1) & 0xFF);	   /* # of samples - 1 */
	  sb_write_dsp((size-1) >> 8);
	  sb_write_dsp(0x1C);                  /* normal speed 8 bit mono */
   }
/* else if (sb_dsp_ver < 0x300) {          // GB 2017, for SB 1.5 new and SB 2.0  
	  sb_write_dsp(0x48);
	  sb_write_dsp((size-1) & 0xFF);	   // # of samples - 1 
	  sb_write_dsp((size-1) >> 8);
	  sb_write_dsp(0x91);                  // 91h = High-speed 8 bit mono 
   }*/
   else if (sb_dsp_ver < 0x400) {          /* 8 bit auto-initialised */
      sb_write_dsp(0x48);
      sb_write_dsp((size-1) & 0xFF);
      sb_write_dsp((size-1) >> 8);
      sb_write_dsp(0x90); // 90h = 8-bit PCM High-speed output
   }
   else {                                  /* 16 bit */
      size /= 2;
      sb_write_dsp(0xB6);
      sb_write_dsp(0x30);
      sb_write_dsp((size-1) & 0xFF);
      sb_write_dsp((size-1) >> 8);
   }
}
static END_OF_FUNCTION(sb_play_buffer);

//-------------------------------------------------------------------------------------

// sb_interrupt: The SB end-of-buffer interrupt handler. Swaps to the other buffer 
//  if the card doesn't have auto-initialised dma, and then refills the buffer that just finished playing.
static int sb_interrupt()
{
   int timeout=0;

   // GB 2017 This is an attempt to fix the situation when using a SB-16 together with its buggy MPU-401 interface.
   // See also "mpu_output" in mpu.c. 
   if (sb_hw_dsp_ver >= 0x400) {           // DSP 4.xx only 
     outportb(_sb_port+4, 0x82);           // SB mixer interrupt status register (82h)
     if ((inportb(_sb_port+5) & 3) == 0) { // Not a 8- or 16-bit DSP interrupt, but maybe mpu-401 instead?
	   _eoi(_sb_irq);
       return -1;                          // This interrupt was either bugged/false or not for this handler, exit now.
	 }
   }// End of fix. 

/* // GB 2017, initial hanging note fix: works well but risky -  DMA buffer of 4096 already fixed things. 
   if ((sb_hw_dsp_ver >= 0x40B) && (sb_hw_dsp_ver <= 0x40D)) // DSP 4.11, 4.12 or 4.13 only 
   {
     while ((inportb(0x330+1) & 0x40) && timeout<0x7FFF) timeout++;
     outportb(0x330, 0xF8); // Send midi clock byte, which is inaudible in case it hangs. (idea: newrisingsun)
   }*/

   if (sb_dsp_ver < 0x200) {                /* not auto-initialised */ // GB 2017 exclude DSP 2.00
     _dma_start(_sb_dma, sb_buf[1-sb_bufnum], sb_dma_size, FALSE, FALSE);
     sb_play_buffer(sb_dma_size);
   }
   else {                                    /* poll dma position */
     sb_dma_count++;
     if (sb_dma_count > 16) {
	 sb_bufnum = (_dma_todo(_sb_dma) > (unsigned)sb_dma_size) ? 1 : 0;
	 sb_dma_count = 0;
     }
   }

   if (!sb_semaphore) {
      sb_semaphore = TRUE;
      ENABLE();                              /* mix some more samples */
	  if (sb_16bit) _mix_some_samples(sb_buf[sb_bufnum], _dos_ds, TRUE);  // Signed, added GB 2017.
	  else          _mix_some_samples(sb_buf[sb_bufnum], _dos_ds, FALSE); // Unsigned
      DISABLE();
      sb_semaphore = FALSE;
   } 

   sb_bufnum = 1 - sb_bufnum; 

   if (sb_16bit)                             /* acknowlege SB */
      inportb(_sb_port+0x0F);
   else
      inportb(_sb_port+0x0E);

   //outportb(0x20, 0x20);    /* acknowledge interrupt */ // GB 2017 this sends EOI to PIC1
   //outportb(0xA0, 0x20);                                // GB 2017 this sends EOI to PIC2
   _eoi(_sb_irq); //  acknowledge interrupt 
   return 0;
}
static END_OF_FUNCTION(sb_interrupt);


//-------------------------------------------------------------------------------------

// sb_detect: SB detection routine. Uses the BLASTER environment variable,
// or 'sensible' guesses if that doesn't exist.
static int sb_detect()
{
   char *blaster = getenv("BLASTER");
   char *msg;
   int cmask;
   int max_freq;
   int default_freq;
 
// return FALSE;

   /* what breed of SB are we looking for? */
   switch (digi_card) {

      case DIGI_SB10:
	 sb_dsp_ver = 0x100;
	 break;

      case DIGI_SB15:
	 sb_dsp_ver = 0x200;
	 break;

      case DIGI_SB20:
	 sb_dsp_ver = 0x201;
	 break;

      case DIGI_SBPRO:
	 sb_dsp_ver = 0x300;
	 break;

      case DIGI_SB16:
	 sb_dsp_ver = 0x400;
	 break;

      default:
	 sb_dsp_ver = -1;
	 break;
   } 

   /* parse BLASTER env */
   if (blaster) { 
      while (*blaster) {
	 while ((*blaster == ' ') || (*blaster == '\t'))
	    blaster++;

	 if (*blaster) {
	    switch (*blaster) {

	       case 'a': case 'A':
		  if (_sb_port < 0)
		     _sb_port = strtol(blaster+1, NULL, 16);
		  break;

	       case 'i': case 'I':
		  if (_sb_irq < 0)
		     _sb_irq = strtol(blaster+1, NULL, 10);
		  break;

	       case 'd': case 'D':
		  sb_dma8 = strtol(blaster+1, NULL, 10);
		  break;

	       case 'h': case 'H':
		  sb_dma16 = strtol(blaster+1, NULL, 10);
		  break;
	    }

	    while ((*blaster) && (*blaster != ' ') && (*blaster != '\t'))
	       blaster++;
	 }
      }
   }


   /* make sure we got a good port address */
   if (_sb_reset_dsp(1) != 0) { 
      static int bases[] = { 0x210, 0x220, 0x230, 0x240, 0x250, 0x260, 0 };
      int i;

      for (i=0; bases[i]; i++) {
	 _sb_port = bases[i];
	 if (_sb_reset_dsp(1) == 0)
	    break;
      }
   }

   /* check if the card really exists */
   _sb_read_dsp_version();
   if (sb_hw_dsp_ver < 0) {
    strcpy(allegro_error, "Sound Blaster not found");
      return FALSE;
   }

   if (sb_dsp_ver < 0) {
      sb_dsp_ver = sb_hw_dsp_ver;
   }
   else {
      if (sb_dsp_ver > sb_hw_dsp_ver) {
	 sb_hw_dsp_ver = sb_dsp_ver = -1;
	 strcpy(allegro_error, "Older SB version detected");
	 return FALSE;
      }
   }

   if (sb_dsp_ver >= 0x400) {
      /* read configuration from SB16 card */
      if (_sb_irq < 0) {
	 outportb(_sb_port+4, 0x80);
	 cmask = inportb(_sb_port+5);
	 if (cmask&1) _sb_irq = 2; /* or 9? */
	 if (cmask&2) _sb_irq = 5;
	 if (cmask&4) _sb_irq = 7;
	 if (cmask&8) _sb_irq = 10;
      }
      if ((sb_dma8 < 0) || (sb_dma16 < 0)) {
	 outportb(_sb_port+4, 0x81);
	 cmask = inportb(_sb_port+5);
	 if (sb_dma8 < 0) {
	    if (cmask&1) sb_dma8 = 0;
	    if (cmask&2) sb_dma8 = 1;
	    if (cmask&8) sb_dma8 = 3;
	 }
	 if (sb_dma16 < 0) {
	    sb_dma16 = sb_dma8;
	    if (cmask&0x20) sb_dma16 = 5;
	    if (cmask&0x40) sb_dma16 = 6;
	    if (cmask&0x80) sb_dma16 = 7;
	 }
     }
   }

   /* if nothing else works */
   if (_sb_irq < 0)
      _sb_irq = 7;

   if (sb_dma8 < 0)
      sb_dma8 = 1;

   if (sb_dma16 < 0)
      sb_dma16 = 5;

   /* figure out the hardware interrupt number */
   sb_int = _map_irq(_sb_irq);

   /* what breed of SB? */
   if (sb_dsp_ver >= 0x400) {
      msg = "SB 16";
      max_freq = 43478;
      default_freq = 22222;
      strcpy(digi_sb.name,"Sound Blaster 16/AWE"); // GB 2016, do not exceed this string size!
   }
   else if (sb_dsp_ver >= 0x300) {
      msg = "SB Pro";
      max_freq = 22222;
      default_freq = 22222;
      strcpy(digi_sb.name,"Sound Blaster Pro"); // GB 2016
      digi_sb.name[19]='#'; // GB 2017, marker for recognition in sound.c. To reverse pan issue.
   }
   else if (sb_dsp_ver >= 0x201) {
      msg = "SB 2.0";
      max_freq = 43478;
      default_freq = 22222;
      strcpy(digi_sb.name,"Sound Blaster 2.0"); // GB 2016
   }
   else if (sb_dsp_ver == 0x200) {
      msg = "SB 1.5";
      max_freq = 22222;     // changed from 16129, GB 2017
	  default_freq = 22222; // changed from 16129, GB 2017
      strcpy(digi_sb.name,"Sound Blaster 1.5"); // GB 2016
   }
   else {
      msg = "SB 1.0";
      max_freq = 22222;     // changed from 16129, GB 2017
	  default_freq = 22222; // changed from 16129, GB 2017
      strcpy(digi_sb.name,"Sound Blaster 1.0"); // GB 2016
   } 

   /* set up the playback frequency */
   if (_sb_freq <= 0)
      _sb_freq = default_freq;

   if (_sb_freq < 15000) {
      _sb_freq = 11111;
      sb_dma_size = 128;
   }
   else if (MIN(_sb_freq, max_freq) < 20000) {
      _sb_freq = 16129;
      sb_dma_size = 128;
   }
   else if (MIN(_sb_freq, max_freq) < 40000) {
      _sb_freq = 22222;
      sb_dma_size = 256;
   }
   else {
      _sb_freq = 43478;
      sb_dma_size = 512;
   }

   if (sb_dsp_ver < 0x200) sb_dma_size *= 4; // SB 1.0 and older SB 1.5, now exclude DSP 2.00, GB 2017
   sb_dma_mix_size = sb_dma_size;

   /* can we handle 16 bit sound? */
   if (sb_dsp_ver >= 0x400) { 
      if (_sb_dma < 0)
	 _sb_dma = sb_dma16;
      else
	 sb_dma16 = _sb_dma;
      sb_16bit = TRUE;
      sb_dma_size <<= 1;
   }
   else { 
      if (_sb_dma < 0)
	 _sb_dma = sb_dma8;
      else
	 sb_dma8 = _sb_dma;
      sb_16bit = FALSE;
  }

   /* can we handle stereo? */
   if (sb_dsp_ver >= 0x300) {
      sb_stereo = TRUE;
      sb_dma_size <<= 1;
      sb_dma_mix_size <<= 1;
   }
   else
      sb_stereo = FALSE;

   // GB 2017, on SB16: increase buffer size -> minimize DMA interrupts -> less MPU interrupt problems.
   if (sb_hw_dsp_ver >= 0x400) while (sb_dma_size < 2048) { sb_dma_mix_size *= 2; sb_dma_size *= 2; }
   // GB 2017, DSP 4.11, 4.12 or 4.13 are affected by hanging notes after digitalsound interrupt.
   if ((sb_hw_dsp_ver >= 0x40B) && (sb_hw_dsp_ver <= 0x40D)) 
      { sb_dma_mix_size *= 2; sb_dma_size *= 2; } // 4096 buffer, problems gone :)

   /* set up the card description. */
   sprintf(sb_desc, "%s (%d Hz) on port %Xh, using IRQ %d and DMA channel %d",
			msg, _sb_freq, _sb_port, _sb_irq, _sb_dma);

   return TRUE;
}

//-------------------------------------------------------------------------------------

// sb_midi_output: Writes a byte to the SB midi interface.
static void sb_midi_output(unsigned char data)
{
   sb_write_dsp(0x38);
   sb_write_dsp(data);
}
static END_OF_FUNCTION(sb_midi_output);

//-------------------------------------------------------------------------------------

// sb_lock_mem: Locks all the memory touched by parts of the SB code that are executed in an interrupt context.
static void sb_lock_mem()
{
   LOCK_VARIABLE(digi_sb);
   LOCK_VARIABLE(_sb_freq);
   LOCK_VARIABLE(_sb_port);
   LOCK_VARIABLE(_sb_dma);
   LOCK_VARIABLE(_sb_irq);
   LOCK_VARIABLE(sb_int);
   LOCK_VARIABLE(sb_in_use);
   LOCK_VARIABLE(sb_16bit);
   LOCK_VARIABLE(sb_dsp_ver);
   LOCK_VARIABLE(sb_hw_dsp_ver);
   LOCK_VARIABLE(sb_dma_size);
   LOCK_VARIABLE(sb_dma_mix_size);
   LOCK_VARIABLE(sb_sel);
   LOCK_VARIABLE(sb_buf);
   LOCK_VARIABLE(sb_bufnum);
   LOCK_VARIABLE(sb_dma_count);
   LOCK_VARIABLE(sb_semaphore);
   LOCK_FUNCTION(sb_play_buffer);
   LOCK_FUNCTION(sb_interrupt);
   LOCK_VARIABLE(midi_sb_out);
   LOCK_FUNCTION(sb_midi_output);
}

//-------------------------------------------------------------------------------------

// sb_init: SB init routine: returns zero on success, -1 on failure.
static int sb_init(int voices)
{
   if (sb_in_use) {
      strcpy(allegro_error, "Can't use SB MIDI interface and DSP at the same time");
      return -1;
   }

   if ((digi_card == DIGI_SB) || (digi_card == DIGI_AUTODETECT)) { // Modified GB 2017, to fix SB 1.5 confusion
           if (sb_dsp_ver <  0x200)  digi_card = DIGI_SB10; // SB v1.0 or v1.5 DSP 1.xx, 8 bit mono, using single-shot dma
      else if (sb_dsp_ver == 0x200)	 digi_card = DIGI_SB15; // SB v1.5, DSP 2.00,        8 bit mono, using auto-init dma
      else if (sb_dsp_ver <  0x300)  digi_card = DIGI_SB20; // SB v2.0, DSP 2.01,        8 bit mono, using auto-init dma, up to 44KHz 
      else if (sb_dsp_ver <  0x400)  digi_card = DIGI_SBPRO;// SB Pro,  DSP 3.0x,        8 bit stereo DAC, up to 22KHz Stereo
      else                           digi_card = DIGI_SB16; // SB 16, AWE32 or AWE64, DSP 4.xx, 16 bit stereo DAC
   }

   if (sb_dsp_ver < 0x200) {       /* two conventional mem buffers */ // SB 1.0 and older SB 1.5
      if ((_dma_allocate_mem(sb_dma_size, &sb_sel[0], &sb_buf[0]) != 0) ||
	  (_dma_allocate_mem(sb_dma_size, &sb_sel[1], &sb_buf[1]) != 0))
	 return -1;
   }
   else {                           /* auto-init dma, one big buffer */
      if (_dma_allocate_mem(sb_dma_size*2, &sb_sel[0], &sb_buf[0]) != 0)
	  return -1;
      sb_sel[1] = sb_sel[0];
      sb_buf[1] = sb_buf[0] + sb_dma_size;
   }

   sb_lock_mem();

   digi_sb.voices = voices;

   if (_mixer_init(sb_dma_mix_size, _sb_freq, sb_stereo, sb_16bit, &digi_sb.voices) != 0) return -1;

   _mix_some_samples(sb_buf[0], _dos_ds, TRUE);
   _mix_some_samples(sb_buf[1], _dos_ds, TRUE);

   sb_bufnum = 0;

   _enable_irq(_sb_irq);
   _install_irq(sb_int, sb_interrupt);

   _sb_voice(1);

   sb_set_sample_rate(_sb_freq);

   if ((sb_dsp_ver >= 0x300) && (sb_dsp_ver < 0x400))
      sb_stereo_mode(sb_stereo);

   if (sb_dsp_ver < 0x200) // GB 2017 exclude DSP 2.00
      _dma_start(_sb_dma, sb_buf[0], sb_dma_size, FALSE, FALSE);
   else
      _dma_start(_sb_dma, sb_buf[0], sb_dma_size*2, TRUE, FALSE);

   sb_play_buffer(sb_dma_size);

   sb_in_use = TRUE;

   //rest(500); // G2017 Wait for end-of-DMA interrupt

   return 0;
}

//-------------------------------------------------------------------------------------

// sb_exit: SB driver cleanup routine, removes ints, stops dma, frees buffers, etc.
static void sb_exit()
{
   int i;

   /* halt sound output */
   _sb_voice(0);

   /* GB 2013: Reset for stubborn SBPro compatibles, seems to empty remains in DMA buffer? 
      from quake 1 source, where they put it in at init. 
      Without it WSS mode gets stuck on some cards, but only after using the allegro driver 
      as if the allegro driver DMA routine has bug somewhere? 
      normally not used for v2 DSP, but otherwise this one gives similar problems. older ones are OK */
    if ((sb_dsp_ver >= 0x200) && (sb_dsp_ver < 0x400))
	{  
      _sb_reset_dsp(1);
      
	  sb_write_dsp(0x14);			/* send one byte */
	  sb_write_dsp(0x0);
	  sb_write_dsp(0x0);

	  for (i=0 ; i<0x10000 ; i++)
		inportb(_sb_port+0xe);		/* ack the dsp */
    }

   /* stop dma transfer */
   _dma_stop(_sb_dma); 

   if (sb_dsp_ver <= 0x0200)
      sb_write_dsp(0xD0); 

   _sb_reset_dsp(1);

   _remove_irq(sb_int);
   _restore_irq(_sb_irq);

   __dpmi_free_dos_memory(sb_sel[0]);
   if (sb_sel[1] != sb_sel[0])
      __dpmi_free_dos_memory(sb_sel[1]);

   _mixer_exit();

   sb_hw_dsp_ver = sb_dsp_ver = -1;
   sb_in_use = FALSE;
}

//-------------------------------------------------------------------------------------

// sb_midi_init: Initialises the SB midi interface, returning zero on success.
static int sb_midi_init(int voices)
{
   if (sb_in_use) {
      strcpy(allegro_error, "Can't use SB MIDI interface and DSP at the same time");
      return -1;
   }

   sb_dsp_ver = -1;

   sb_lock_mem();

   sprintf(sb_midi_desc, "Sound Blaster MIDI interface on port %Xh", _sb_port);

   sb_in_use = TRUE;
   return 0;
}

//-------------------------------------------------------------------------------------

// sb_midi_exit: Resets the SB midi interface when we are finished.
static void sb_midi_exit()
{
   _sb_reset_dsp(1);
   sb_in_use = FALSE;
}


