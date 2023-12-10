/*         ______   ___    ___ 
 *        /\  _  \ /\_ \  /\_ \ 
 *        \ \ \L\ \\//\ \ \//\ \      __     __   _ __   ___ 
 *         \ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\/\`'__\/ __`\
 *          \ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \ \ \//\ \L\ \
 *           \ \_\ \_\/\____\/\____\ \____\ \____ \ \_\\ \____/
 *            \/_/\/_/\/____/\/____/\/____/\/___L\ \/_/ \/___/
 *                                           /\____/
 *                                           \_/__/
 *
 *
 *       Gravis Ultrasound PnP drivers, 
 *       By G. Broers 1-2016
 *       Based on overhauled WSS Driver for Allegro / WSS Documentation. 
 *       Based on ID software's Quake 1 drive "snd_gus.c"
 *       
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

// CODEC ports 
#define GUSPNP_ADDR    (_sb_port)
#define GUSPNP_DATA    (_sb_port+1)
#define GUSPNP_STATUS  (_sb_port+2)

// GUSPNP_ADDR registers 
#define GUSPNP_LADC         0x00 // Left  ADC input
#define GUSPNP_RADC         0x01 // Right ADC input
#define GUSPNP_LDAC	        0x06 // Left  Master Volume
#define GUSPNP_RDAC	        0x07 // Right Master Volume
#define GUSPNP_FS		    0x08 // FS & Playback data format
#define GUSPNP_INTCON	    0x09 // Interface configuration
#define GUSPNP_PINCON		0x0A // Pin Control
#define GUSPNP_ERRSTAT	    0x0B // Error status and initialization
#define GUSPNP_MODE_ID		0x0C // Mode & ID
#define GUSPNP_LOOPBCK	    0x0D // Loopback control
#define GUSPNP_CNT_HI	    0x0E // Playback Upper Base Count
#define GUSPNP_CNT_LO	    0x0F // Playback Lower Base Count

#define NUMCODECRATES 14

static char guspnp_desc[80] = "not initialised";
static int  guspnp_detect();
static int  guspnp_digi_init(int voices);
static void guspnp_digi_exit();
static int  guspnp_mixer_volume(int volume);
static int  guspnp_usedrate = 0;
static int  guspnp_int = -1;                   
static int  guspnp_in_use = FALSE;
static int  guspnp_detected = FALSE;
static int  guspnp_dma_size = -1;               // size of dma transfer in bytes 
static int  guspnp_sel;                         // selector for the DMA buffer 
static int  guspnp_bufnum = 0;                  // the one currently in use 
static unsigned long guspnp_buf[2];             // pointers to the two buffers 
static volatile int  guspnp_semaphore = FALSE;  // reentrant interrupt? 
static int guspnp_dma_count = 0;                // need to resync with dma? 

DIGI_DRIVER digi_guspnp =     
{
   "Gravis Ultrasound PnP", 
   guspnp_desc,
   0, 0, MIXER_MAX_SFX, MIXER_DEF_SFX,
   guspnp_detect,
   guspnp_digi_init,
   guspnp_digi_exit,
   guspnp_mixer_volume,  
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


struct codec_rate_struct
{
   int freq;
   int divider;
};

// List of supported rates 
static struct codec_rate_struct codec_rates[NUMCODECRATES] =
{
   {  5512, 0x01 }, // 00 - XTAL2 bit1=1, Divide bit2+: 0 
   {  6620, 0x0F }, // 01 - XTAL2 bit1=1, Divide bit2+: 7
   {  8000, 0x00 }, // 02 - XTAL1 bit1=0, Divide bit2+: 0
   {  9600, 0x0E }, // 03 - XTAL1 bit1=0, Divide bit2+: 7
   { 11025, 0x03 }, // 04 - XTAL2 bit1=1, Divide bit2+: 1
   { 16000, 0x02 }, // 05 - XTAL1 bit1=0, Divide bit2+: 1
   { 18900, 0x05 }, // 06 - XTAL2 bit1=1, Divide bit2+: 2
   { 22050, 0x07 }, // 07 - XTAL2 bit1=1, Divide bit2+: 3
   { 27420, 0x04 }, // 08 - XTAL1 bit1=0, Divide bit2+: 2
   { 32000, 0x06 }, // 09 - XTAL1 bit1=0, Divide bit2+: 3
   { 33075, 0x0D }, // 10 - XTAL2 bit1=1, Divide bit2+: 6
   { 37800, 0x09 }, // 11 - XTAL2 bit1=1, Divide bit2+: 4
   { 44100, 0x0B }, // 12 - XTAL2 bit1=1, Divide bit2+: 5
   { 48000, 0x0C }  // 13 - XTAL1 bit1=0, Divide bit2+: 6
};


//-------------------------------------------------------------------------------------

// From WSS driver:
static inline void GUSPNP_OUT(unsigned char reg, unsigned char val)
{
    unsigned int timeout=65535;
    while((inportb(GUSPNP_ADDR)&0x80) && (--timeout));
    outportb(GUSPNP_ADDR, reg);
    outportb(GUSPNP_DATA, val);
}

//-------------------------------------------------------------------------------------

// IRQ handler. 
static int guspnp_interrupt(void)
{
   if (inportb(GUSPNP_STATUS) & 0x01) 
     outportb(GUSPNP_STATUS, 0x00);  // Acknowledge interrupt 
	 else {                       // Not a valid interrupt
       //outportb(0x20, 0x20);      // send EOI to PIC1
       //outportb(0xA0, 0x20);      // send EOI to PIC2
	   _eoi(_sb_irq);
       return -1;                 // This interrupt was either bugged/false or not for this handler, exit now.
     } 

  guspnp_dma_count++;
  if (guspnp_dma_count > 16) 
  {
    guspnp_bufnum = (_dma_todo(_sb_dma) > (unsigned)guspnp_dma_size) ? 1 : 0;
    guspnp_dma_count = 0;
  }

  if (!(guspnp_semaphore)) 
  {
     guspnp_semaphore = TRUE;
     ENABLE();                         
     _mix_some_samples(guspnp_buf[guspnp_bufnum], _dos_ds, TRUE); // mix some more samples 
     DISABLE();
     guspnp_semaphore = FALSE;
  }
  guspnp_bufnum = 1 - guspnp_bufnum;

  //outportb(0x20, 0x20);                     /* acknowledge interrupt */ // GB 2017 this sends EOI to PIC1
  //outportb(0xA0, 0x20);                                                 // GB 2017 this sends EOI to PIC2
  _eoi(_sb_irq); //  acknowledge interrupt 
  return 0;
}
static END_OF_FUNCTION(guspnp_interrupt);

//-------------------------------------------------------------------------------------

static int guspnp_Probe_IO() // cannot use WSS_OUT!
{

   unsigned char orig1,orig2,check=0;
	
   // backup direct register:
   orig1=inportb(GUSPNP_ADDR);
   // backup indirect register 0:
   outportb(GUSPNP_ADDR, GUSPNP_LADC);
   orig2=inportb(GUSPNP_DATA);

   // probe, write:
   outportb(GUSPNP_ADDR, GUSPNP_LADC);
   outportb(GUSPNP_DATA, inportb(GUSPNP_DATA) | 0x0F); // set four bits: xxxx-1111b 
   // probe, read:
   outportb(GUSPNP_ADDR, GUSPNP_LADC);
   check=inportb(GUSPNP_DATA);
   if ((check & 0xF) ==0xF) 
   {
     // continue probe, write:
     outportb(GUSPNP_ADDR, GUSPNP_LADC);
     outportb(GUSPNP_DATA, inportb(GUSPNP_DATA) & 0xF0); // unset four bits: xxxx-1111b 
     // continue probe, read:
     outportb(GUSPNP_ADDR, GUSPNP_LADC);
     check=inportb(GUSPNP_DATA);
     if ((check & 0xF) ==0) check=1; else check=0;
   }
   else check=0;
   // restore:
   outportb(GUSPNP_ADDR, GUSPNP_LADC);
   outportb(GUSPNP_DATA, orig2);
   // restore:
   outportb(GUSPNP_ADDR, orig1);

   if (check==1) return 1; else return 0;
}

//-------------------------------------------------------------------------------------

static int guspnp_detect()
{
   char *envptr = getenv("ULTRA16");
   int wss_ports[] ={ 0x530, 0x604, 0xE80, 0xF40 };
   int i;
   int orig_port = _sb_port;
   int orig_irq  = _sb_irq;
   int orig_dma  = _sb_dma;

        if (_sb_freq < 13000)  {guspnp_usedrate = 04; guspnp_dma_size =  512;} // actually 11025
   else if (_sb_freq < 18000)  {guspnp_usedrate = 05; guspnp_dma_size =  512;} // actually 16000
   else if (_sb_freq < 34000)  {guspnp_usedrate = 07; guspnp_dma_size = 1024;} // actually 22050
   else                        {guspnp_usedrate = 12; guspnp_dma_size = 2048;} // actually 44100 

   if (guspnp_detected)  
   {
      sprintf(guspnp_desc, "Gravis Ultrasound PnP (%d Hz) on port %Xh, using IRQ %d and DMA %d",
           codec_rates[guspnp_usedrate].freq, _sb_port, _sb_irq, _sb_dma);
	  return TRUE;
   }

   // parse ULTRA16 env, example: SET ULTRA16=34C,6,5,1,0
   if (envptr) 
   { 
	 if (_sb_port < 0) _sb_port = strtol(envptr,   NULL, 16);
	 if (_sb_dma  < 0) _sb_dma  = strtol(envptr+4, NULL, 10);
	 if (_sb_irq  < 0) _sb_irq  = strtol(envptr+6, NULL, 10);
   }   

   // Make sure numbers OK
   if ((_sb_port < 0) || (_sb_dma < 0) || (_sb_irq < 0)) // set as autodetect, but cannot do that currently: bail.
   {
       _sb_port = orig_port;
       _sb_irq  = orig_irq;
       _sb_dma  = orig_dma;
       return FALSE;
   }

   // Any detection messed up PCem v12 (configured for WSS), make sure we are not probing WSS addresses:
   for (i=0;i<4;i++)
   {
	  if ( _sb_port == wss_ports[i])
	  {
        _sb_port = orig_port;
        _sb_irq  = orig_irq;
        _sb_dma  = orig_dma;
        return FALSE; 
	  }
   }	

   if (guspnp_Probe_IO()==0) // WSS-like detection routine first.
   {  
	  _sb_port = orig_port;
      _sb_irq  = orig_irq;
      _sb_dma  = orig_dma;
      return FALSE; 
   } 

   // EARLIER DETECTION ROUTINE BELOW:
   // Make sure there is a CODEC at the CODEC base
   // Clear any pending IRQs
   inportb(GUSPNP_STATUS);
   outportb(GUSPNP_STATUS,0);

   // Wait for 'INIT' bit to clear
   for (i=0;i<0xFFFF;i++)
      if ((inportb(GUSPNP_ADDR) & 0x80) == 0)
         break;
   if (i==0xFFFF)
   {
        _sb_port = orig_port;
        _sb_irq  = orig_irq;
        _sb_dma  = orig_dma;
        return FALSE;
   }

   // Get chip revision - can not be zero
   outportb(GUSPNP_ADDR,GUSPNP_MODE_ID);
   if ((inportb(GUSPNP_ADDR) & 0x7F) != GUSPNP_MODE_ID)
   {
        _sb_port = orig_port;
        _sb_irq  = orig_irq;
		_sb_dma  = orig_dma;
        return FALSE;
   }
   if ((inportb(GUSPNP_DATA) & 0x0F) == 0)
   {
        _sb_port = orig_port;
        _sb_irq  = orig_irq;
        _sb_dma  = orig_dma;
        return FALSE;
   }

   sprintf(guspnp_desc, "Gravis Ultrasound PnP (%d Hz) on port %Xh, using IRQ %d and DMA %d",
           codec_rates[guspnp_usedrate].freq, _sb_port, _sb_irq, _sb_dma);
   
   digi_guspnp.desc = guspnp_desc;

   guspnp_detected = TRUE;

   return TRUE;
}

//-------------------------------------------------------------------------------------

// Locks all the memory touched by parts of the SB code that are executed in an interrupt context.
static void guspnp_lock_mem()
{
   LOCK_VARIABLE(digi_guspnp);
   LOCK_VARIABLE(_sb_freq);
   LOCK_VARIABLE(_sb_port);
   LOCK_VARIABLE(_sb_dma);
   LOCK_VARIABLE(_sb_irq);
   LOCK_VARIABLE(guspnp_int);
   LOCK_VARIABLE(guspnp_in_use);
   LOCK_VARIABLE(guspnp_dma_size);
   LOCK_VARIABLE(guspnp_sel);
   LOCK_VARIABLE(guspnp_buf);
   LOCK_VARIABLE(guspnp_bufnum);
   LOCK_VARIABLE(guspnp_semaphore);
   LOCK_FUNCTION(guspnp_interrupt);
   LOCK_VARIABLE(guspnp_dma_count);
}

//-------------------------------------------------------------------------------------

// guspnp_digi_init, Called once at startup to initialise the digital sample playing code.
static int guspnp_digi_init(int voices)
{
   int i,j; 
   unsigned short tmp;
  
   if (guspnp_in_use)    return -1;
   if (!guspnp_detected) guspnp_detect();  // Parameters not yet set? 
   if (!guspnp_detected) return -1;        // If still no detection, fail 

   if ((_sb_port>0xF40) || (_sb_port<0x220)  || (_sb_irq<0) || (_sb_dma<0)) {guspnp_detected=0; guspnp_detect();} // Should not be necessary....

   guspnp_int = _map_irq(_sb_irq);
     
   if (_dma_allocate_mem(guspnp_dma_size*2, &guspnp_sel, &guspnp_buf[0]) != 0) return -1;

   guspnp_buf[1] = guspnp_buf[0] + guspnp_dma_size;

   guspnp_lock_mem();

   digi_guspnp.voices = voices;

   if (_mixer_init(guspnp_dma_size/2, codec_rates[guspnp_usedrate].freq, TRUE, TRUE, &digi_guspnp.voices) != 0) return -1;

   _mix_some_samples(guspnp_buf[0], _dos_ds, TRUE); // GB 2017 set to TRUE
   _mix_some_samples(guspnp_buf[1], _dos_ds, TRUE);

   guspnp_bufnum = 0;

   _enable_irq(_sb_irq);
   _install_irq(guspnp_int, guspnp_interrupt);

   _sb_voice(1);

   // Begin Codec initialization 
   GUSPNP_OUT(GUSPNP_LDAC,    0x80);   // Mute Master
   GUSPNP_OUT(GUSPNP_RDAC,    0x80);   // Mute Master

   // Port Communication Below from Quake 1 source by ID software.

   // Clear any pending IRQs
   inportb(GUSPNP_STATUS);
   outportb(GUSPNP_STATUS,0);

   // Set mode to 2
   outportb(GUSPNP_ADDR,GUSPNP_MODE_ID);
   outportb(GUSPNP_DATA,0xC0);

   // Stop any playback or capture which may be happening
   outportb(GUSPNP_ADDR,GUSPNP_INTCON);
   outportb(GUSPNP_DATA,inportb(GUSPNP_DATA) & 0xFC);

   // Set FS
   outportb(GUSPNP_ADDR,GUSPNP_FS | 0x40);
   outportb(GUSPNP_DATA,codec_rates[guspnp_usedrate].divider | 0x50); // Or in stereo and 16 bit bits

   // Wait a bit
   for (i=0;i<10;i++)
      inportb(GUSPNP_DATA);

   // Routine 1 to counter CODEC bug - wait for init bit to clear and then a
   // bit longer (i=min loop count, j=timeout
   for (i=0,j=0;i<1000 && j<0x7FFFF;j++)
      if ((inportb(GUSPNP_ADDR) & 0x80)==0)
         i++;

   // Routine 2 to counter CODEC bug - this is from Forte's code. For me it
   // does not seem to cure the problem, but is added security
   // Waits till we can modify index register
   for (j=0;j<0x7FFFF;j++)
   {
      outportb(GUSPNP_ADDR,GUSPNP_INTCON | 0x40);
      if (inportb(GUSPNP_ADDR)==(GUSPNP_INTCON | 0x40))
         break;
   }

   // Perform ACAL
   outportb(GUSPNP_ADDR,GUSPNP_INTCON | 0x40);
   outportb(GUSPNP_DATA,0x08);

   // Clear MCE bit - this makes ACAL happen
   outportb(GUSPNP_ADDR,GUSPNP_INTCON);

   // Wait for ACAL to finish
   for (j=0;j<0x7FFFF;j++)
   {
      if ((inportb(GUSPNP_ADDR) & 0x80) != 0)
         continue;
      outportb(GUSPNP_ADDR,GUSPNP_ERRSTAT);
      if ((inportb(GUSPNP_DATA) & 0x20) == 0)
         break;
   }

   // Clear ACAL bit
   outportb(GUSPNP_ADDR,GUSPNP_INTCON | 0x40);
   outportb(GUSPNP_DATA,0x00);
   outportb(GUSPNP_ADDR,GUSPNP_INTCON);

   // Set some other junk
   outportb(GUSPNP_ADDR,GUSPNP_LOOPBCK);
   outportb(GUSPNP_DATA,0x00);
// outportb(GUSPNP_ADDR,GUSPNP_PINCON);  // GB 2016 - removed because we use interrupts.
// outportb(GUSPNP_DATA,0x08); // IRQ is disabled in PIN control

   // Unmute outputs 
   GUSPNP_OUT(GUSPNP_RDAC, 0);
   GUSPNP_OUT(GUSPNP_LDAC, 0);

   // Set count (it doesn't really matter what value we stuff in here
   tmp = guspnp_dma_size / 4 - 1;       // always stereo, always 16 bit
   outportb(GUSPNP_ADDR, GUSPNP_CNT_LO);
   outportb(GUSPNP_DATA, (tmp) & 0xFF);
   outportb(GUSPNP_ADDR, GUSPNP_CNT_HI);
   outportb(GUSPNP_DATA, (tmp) >> 8);

   // Start playback
   _dma_start(_sb_dma, guspnp_buf[0], guspnp_dma_size*2, TRUE, FALSE);

// GUSPNP_OUT(GUSPNP_INTCON, 0x01); // GB 2016 - replaced because we use interrupts.
   GUSPNP_OUT(GUSPNP_INTCON, 0x05); // interface control bit1: PEN - Playback Enable!  + bit3: Single DMA Channel
   GUSPNP_OUT(GUSPNP_PINCON, 0x02); // Bit 2: Enable interrupts in Pin Control reg. 
      
   //_eoi(_sb_irq); // issue an end of interrupt

   guspnp_in_use = TRUE;
   
   //rest(500); // G2017 Wait for end-of-DMA interrupt

   return 0;
}

//-------------------------------------------------------------------------------------

//  gus driver cleanup routine, removes ints, stops dma, frees buffers, etc.
static void guspnp_digi_exit()
{
   _sb_voice(0);

   outportb(GUSPNP_ADDR,GUSPNP_INTCON);
   outportb(GUSPNP_DATA,0x01);
   rest (100);                 // delay to ensure DMA transfer finishes
   _dma_stop(_sb_dma);       
   GUSPNP_OUT(GUSPNP_LDAC,   0x80);  // Mute Master
   GUSPNP_OUT(GUSPNP_RDAC,   0x80);  // Mute Master

   _remove_irq(guspnp_int);
   _restore_irq(_sb_irq);
   // free conventional memory buffer 
   __dpmi_free_dos_memory(guspnp_sel);
   _mixer_exit();

   guspnp_in_use = FALSE;
}

//-------------------------------------------------------------------------------------

// From WSS driver:
static int guspnp_mixer_volume(int volume)
{
   if (volume > 255)
      volume = 255;

   if (volume < 0)
      volume = 0;

   // WSS has only attenuation regs, so we totally mute outputs to get
   //   silence w/volume 0 

   DISABLE();

   if (!volume) {
      GUSPNP_OUT(GUSPNP_RDAC, 0x80);
      GUSPNP_OUT(GUSPNP_LDAC, 0x80);
   }
   else {
      GUSPNP_OUT(GUSPNP_RDAC, 0x3F-volume/4);
      GUSPNP_OUT(GUSPNP_LDAC, 0x3F-volume/4);
	  // for CS4231, CS4232 and AD1848 : 0x00h is the maximum setting: 0dB. 0x3F is minimum: -94,5 dB
	  // for CS4236 it is the same, but there are higher dB settings between 0x47 and 0x40
   }

   ENABLE();
   return 0;
}