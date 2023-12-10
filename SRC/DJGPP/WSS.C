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
 *      Windows Sound System driver.
 *      Supports DMA playback and sampling rates to up to 48kHz.
 *
 *      The WSS CODEC is more common than you might think.
 *      Supported chips: AD1848, CS4231/A, CS4232 and compatibles.
 *
 *      By Antti Koskipaa and G. Broers.
 *
 *      See readme.txt for copyright information.
 *
 *      The WSS chips (all of them, clones too) have a bug which causes them
 *      to "swap" the LSB/MSB of each sample (or something like that) causing
 *      nothing but a loud fuzz from the speakers on "rare" occasions.
 *      An errata sheet on the CS4231 chip exists, but I haven't found it.
 *      If you fix this problem, email me and tell how...
 *
 *      Should I support recording? naah... =)
 *      AD1848 can't do full duplex, and this driver is meant to be AD1848 compatible.
 *      So, if you plan to support full duplex, check the chip revision first!
 *
 *      
 *      - MAJOR OVERHAUL, NEAR TOTAL REWRITE - 
 *		G. Broers 1-2016 
 *		References: Crystal Semiconductor documentation + Allegro Soundscape driver.
 *
 *      This driver is actually compatible with GUS PnP except one has to substract 4
 *      from the port address, to compensate for the port address offset used here.
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
#define WSS_ADDR    (_sb_port+4)
#define WSS_DATA    (_sb_port+5)
#define WSS_STATUS  (_sb_port+6)

// WSS_ADDR bits 
#define WSS_INIT    0x80 // Init bit (read only)
#define WSS_MCE     0x40 // Mode Change Enable bit (1 when at change)
                         // Not needef for CEN and PEN
#define TRD         0x20 // DMA Transfer disable bit (disable=1)

// WSS_ADDR registers 
#define WSS_LADC    0x00 // Left  ADC input
#define WSS_RADC    0x01 // Right ADC input
#define WSS_LAUX1   0x02 // Left  AUX 1 input
#define WSS_RAUX1   0x03 // Right AUX 1 input
#define WSS_LAUX2   0x04 // Left  AUX 2 input
#define WSS_RAUX2   0x05 // Right AUX 2 input
#define WSS_LDAC    0x06 // Left  Master Volume, value 0x80 is mute, 0=full
#define WSS_RDAC    0x07 // Right Master Volume, value 0x80 is mute, 0=full
#define WSS_FS      0x08 // FS & Playback data format
#define WSS_INTCON  0x09 // Interface configuration: PEN, CEN, SDC, CAL1+2, PPIO, CPIO   
#define WSS_PINCON  0x0A // Pin Control
#define WSS_ERRSTAT 0x0B // Error status and initialization
#define WSS_MODE_ID 0x0C // Mode & ID
#define WSS_LOOPBCK 0x0D // Loopback control
#define WSS_CNT_HI  0x0E // Playback Upper Base Count    
#define WSS_CNT_LO  0x0F // Playback Lower Base Count    

#define WSS_CHIP_ID 0x19 // Versions & Chip ID (extended only)

// registers after 0x0F only available in MODE2:
#define WSS_ALT_FT1 0x10 // Alternate Feature Enable 1
#define WSS_ALT_FT2 0x0F // Alternate Feature Enable 2
// etc...

#define NUMCODECRATES 14

static char wss_desc[80] = "not initialised";
static int  wss_detect();
static int  wss_init(int voices);
static void wss_exit();
static int  wss_mixer_volume(int volume);
static int  wss_usedrate = 0;
static int  wss_int = -1;                   
static int  wss_stereo = TRUE;
static int  wss_16bits = TRUE;
static int  wss_in_use = FALSE;
static int  wss_detected = FALSE;
static int  wss_dma_size = -1;               // size of dma transfer in bytes 
static int  wss_sel;                         // selector for the DMA buffer 
static int  wss_bufnum = 0;                  // the one currently in use 
static unsigned long wss_buf[2];             // pointers to the two buffers 
static volatile int wss_semaphore = FALSE;   // reentrant interrupt? 
static unsigned int wss_ports[] ={ 0x530, 0x604, 0xE80, 0xF40 }; // GB 2015, removed 0x32c,
static int wss_dma_count = 0;                // need to resync with dma? 

DIGI_DRIVER digi_wss =
{
   "Windows Sound System",
   wss_desc,
   0, 0, MIXER_MAX_SFX, MIXER_DEF_SFX,
   wss_detect,
   wss_init,
   wss_exit,
   wss_mixer_volume,  
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

// From DOSWSS, Reset some bits in a specific register 
static inline void WSS_OUT(unsigned char reg, unsigned char val)
{
    unsigned int timeout=65535;
    while((inportb(WSS_ADDR)&0x80) && (--timeout));
    outportb(WSS_ADDR, reg);
    outportb(WSS_DATA, val);
}
static inline unsigned char WSS_IN(unsigned char reg)
{
    unsigned int timeout=65535;
    while((inportb(WSS_ADDR)&0x80) && (--timeout));
	outportb(WSS_ADDR, reg);
	return inportb(WSS_DATA);
}

//-------------------------------------------------------------------------------------

// WSS busy wait, wait at least until counter i is zero, then wait until init bit 0x80 gets cleared.
static void wss_wait(void) 
{
  int i = 0xFFFF;
  // Wait for WSS_INIT bit to clear: 
  while ((inportb(WSS_ADDR) & WSS_INIT) || (i-- > 0));
}

//-------------------------------------------------------------------------------------

// IRQ handler. 
static int wss_interrupt(void)
{
   if (inportb(WSS_STATUS) & 0x01) 
     outportb(WSS_STATUS, 0x00);  // Acknowledge WSS interrupt 
	 else {                       // Not a valid interrupt
       //outportb(0x20, 0x20);      // send EOI to PIC1
       //outportb(0xA0, 0x20);      // send EOI to PIC2
	   _eoi(_sb_irq);
       return -1;                 // This interrupt was either bugged/false or not for this handler, exit now.
     } 

  wss_dma_count++;
  if (wss_dma_count > 16) 
  {
    wss_bufnum = (_dma_todo(_sb_dma) > (unsigned)wss_dma_size) ? 1 : 0;
    wss_dma_count = 0;
  }

  if (!(wss_semaphore)) 
  {
    wss_semaphore = TRUE;
    ENABLE();                         
    _mix_some_samples(wss_buf[wss_bufnum], _dos_ds, TRUE); // mix some more samples 
    DISABLE();
    wss_semaphore = FALSE;
  }
  wss_bufnum = 1 - wss_bufnum;

  //outportb(0x20, 0x20);                     /* acknowledge interrupt */ // GB 2017 this sends EOI to PIC1
  //outportb(0xA0, 0x20);                                                 // GB 2017 this sends EOI to PIC2
  _eoi(_sb_irq); //  acknowledge interrupt 
  return 0;
}
static END_OF_FUNCTION(wss_interrupt);

//-------------------------------------------------------------------------------------

static int WSS_Probe_IO() // cannot use WSS_OUT!
{

   unsigned char orig1,orig2,check=0;
	
   // backup direct register:
   orig1=inportb(WSS_ADDR);
   // backup indirect register 0:
   outportb(WSS_ADDR, WSS_LADC);
   orig2=inportb(WSS_DATA);

   // probe, write:
   outportb(WSS_ADDR, WSS_LADC);
   outportb(WSS_DATA, inportb(WSS_DATA) | 0x0F); // set four bits: xxxx-1111b 
   // probe, read:
   outportb(WSS_ADDR, WSS_LADC);
   check=inportb(WSS_DATA);
   if ((check & 0xF) ==0xF) 
   {
     // continue probe, write:
     outportb(WSS_ADDR, WSS_LADC);
     outportb(WSS_DATA, inportb(WSS_DATA) & 0xF0); // unset four bits: xxxx-1111b 
     // continue probe, read:
     outportb(WSS_ADDR, WSS_LADC);
     check=inportb(WSS_DATA);
     if ((check & 0xF) ==0) check=1; else check=0;
   }
   else check=0;
   // restore:
   outportb(WSS_ADDR, WSS_LADC);
   outportb(WSS_DATA, orig2);
   // restore:
   outportb(WSS_ADDR, orig1);

   if (check==1) return 1; else return 0;
}

//-------------------------------------------------------------------------------------

static int wss_detect()
{
   char *envptr = getenv("WSS");
   unsigned char c;
   static char *endptr;

   int i=0;
   int orig_port = _sb_port;
   int orig_irq  = _sb_irq;
   int orig_dma  = _sb_dma;

        if (_sb_freq < 13000)  {wss_usedrate = 04; wss_dma_size =  512;} // actually 11025
   else if (_sb_freq < 18000)  {wss_usedrate = 05; wss_dma_size =  512;} // actually 16000
   else if (_sb_freq < 34000)  {wss_usedrate = 07; wss_dma_size = 1024;} // actually 22050
   else                        {wss_usedrate = 12; wss_dma_size = 2048;} // actually 44100  // GB 2017 bugfix

   if (wss_detected)  
   {
      sprintf(wss_desc, "Windows Sound System (%d Hz) on port %Xh, using IRQ %d and DMA %d",
	  codec_rates[wss_usedrate].freq, _sb_port, _sb_irq, _sb_dma);
	  return TRUE;
   }
 
   if(envptr) while (1)
   {
      // skip whitespace
      do c=*(envptr++); while(c==' ' || c=='\t');

      // reached end of string? -> exit
      if (c==0) break;

      switch(c)
      {   case 'a':
          case 'A':
                  _sb_port = strtol(envptr,&endptr,16);
                  break;

          case 'i':
          case 'I':
                  _sb_irq = strtol(envptr,&endptr,10);
                  break;

          case 'd':
          case 'D':
                  _sb_dma = strtol(envptr,&endptr,10);
                  break;

          default:
                  strtol(envptr,&endptr,16);
                  break;
      }
      envptr = endptr;
   }


   // Make sure numbers OK
   if ((_sb_dma < 0) || (_sb_irq < 0)) // cannot detect these, have to bail. 
   {
       _sb_port = orig_port;
       _sb_irq  = orig_irq;
       _sb_dma  = orig_dma;
       return FALSE;
   }

   // GB 2015, Detect I/O Port
   if (_sb_port < 0x32C) // Port not properly specified; detect
   {
     do
     {
		_sb_port = wss_ports[i];
		if (WSS_Probe_IO()==1) i=5;
		i++;
     }
     while (i<4);
     if (i==4) 
     {
        _sb_port = orig_port;
        _sb_irq  = orig_irq;
        _sb_dma  = orig_dma;
        return FALSE; 
     }
   }
   else  // Just check:
   {
	 if (WSS_Probe_IO()==0) 
	 {  
		_sb_port = orig_port;
        _sb_irq  = orig_irq;
        _sb_dma  = orig_dma;
        return FALSE; 
     } 
   }

   // Todo: detect IRQ and DMA

   sprintf(wss_desc, "Windows Sound System (%d Hz) on port %Xh, using IRQ %d and DMA %d",
	   codec_rates[wss_usedrate].freq, _sb_port, _sb_irq, _sb_dma);
   
   digi_wss.desc = wss_desc;

   wss_detected = TRUE;

   return TRUE;
}

//-------------------------------------------------------------------------------------
// Locks all the memory touched by parts of the SB code that are executed in an interrupt context.
static void wss_lock_mem()
{
   LOCK_VARIABLE(digi_wss);
   LOCK_VARIABLE(_sb_freq);
   LOCK_VARIABLE(_sb_port);
   LOCK_VARIABLE(_sb_dma);
   LOCK_VARIABLE(_sb_irq);
   LOCK_VARIABLE(wss_int);
   LOCK_VARIABLE(wss_in_use);
   LOCK_VARIABLE(wss_dma_size);
   LOCK_VARIABLE(wss_sel);
   LOCK_VARIABLE(wss_buf);
   LOCK_VARIABLE(wss_bufnum);
   LOCK_VARIABLE(wss_semaphore);
   LOCK_FUNCTION(wss_interrupt);
   LOCK_VARIABLE(wss_dma_count);
}

//-------------------------------------------------------------------------------------

static int wss_init(int voices)
{

   int i;
   unsigned short tmp;
   unsigned char Card_ID, Card_ID_X=0, Chip_ID;

   if (wss_in_use)    return -1;
   if (!wss_detected) wss_detect();  // Parameters not yet set? 
   if (!wss_detected) return -1;     // If still no detection, fail 

   if ((_sb_port>0xF40) || (_sb_port<0x32C) || (_sb_irq<0) || (_sb_dma<0)) {wss_detected=0; wss_detect();} // Should not be necessary....

   wss_int = _map_irq(_sb_irq);
     
   if (_dma_allocate_mem(wss_dma_size*2, &wss_sel, &wss_buf[0]) != 0) return -1;

   wss_buf[1] = wss_buf[0] + wss_dma_size;

   wss_lock_mem();

   digi_wss.voices = voices;

   if (_mixer_init(wss_dma_size/2, codec_rates[wss_usedrate].freq, TRUE, TRUE, &digi_wss.voices) != 0) return -1;

   _mix_some_samples(wss_buf[0], _dos_ds, TRUE); // GB 2017 set to TRUE
   _mix_some_samples(wss_buf[1], _dos_ds, TRUE);

   wss_bufnum = 0;

   _enable_irq(_sb_irq);
   _install_irq(wss_int, wss_interrupt);

   _sb_voice(1);

   // Begin Codec initialization 
   WSS_OUT(WSS_LDAC,    0x80);   // Mute Master
   WSS_OUT(WSS_RDAC,    0x80);   // Mute Master


   // Set Playback Format:
   outportb(WSS_STATUS, 0);               // ack any unresolved IRQs
   outportb(WSS_ADDR, WSS_MCE | WSS_FS);  // Mode Change Enable 0x40 + FS & Playback data format 0x08 
   i = codec_rates[wss_usedrate].divider; // Predetermined XTAL/Divider value
   if (wss_stereo) i |= 0x10;             // Bit5: stereo(1)/mono(0)
   if (wss_16bits) i |= 0x40;             // Linear, 16-bit two's complement, Little Endian (FMT0/D6 bit)
   outportb(WSS_DATA, i);
   outportb(WSS_ADDR, WSS_FS);            // Disable MCE (Mode Change Enable)
   wss_wait();                            // GB 2015
   rest(55);                              // GB 2015, wait at least 55ms

// DEBUG 2017 for OPL3SAX:
   Card_ID = WSS_IN(WSS_MODE_ID) & 0xF; // lower 4 bits
   if (WSS_IN(WSS_MODE_ID) & 0x80) Card_ID_X=1; // first bit, not officially described, but we need an identification method
   if (Card_ID==0xA || Card_ID==0x1) // 0x1 = Rev B CS4248/CS4231 // 0xA (1010b)= later crystal chips like 4232 and 4236 or OPL3SAx.
     if (Card_ID_X) // Crystal Semiconductor or Yamaha
     {
       if (Card_ID==0xA)
  	   { 
		 Chip_ID = WSS_IN(WSS_CHIP_ID); // 2017: OPL3Sax cards have to be filtered out, chipID of cs4232 seems to be 162...
         if (Chip_ID==162) 
		 {
		   // Fix for CS4232: fixes sound output problems in case the card's 'SoundBlaster Pro' mode was used beforehand.
    	   WSS_OUT(WSS_MODE_ID,Card_ID | 0xC0); // Set card to enhanced mode 2, enable register 22
           WSS_OUT(22,  0); // Alt. Sample Freq. register, reset. 
		   //WSS_OUT(WSS_MODE_ID, Card_ID | 0x80);   // Default mode, CS4248 "look-alike" 
		   //2017 disabled the above 'default mode',  makes registers above 15 mirror the first? 
		 }
  	   }
  	   WSS_OUT(WSS_LOOPBCK, 0);      // Do not mix ADC with DAC, this is the default 
       //WSS_OUT(WSS_MODE_ID, Card_ID | 0x80);   // Default mode, CS4248 "look-alike" // 2017 cannot do this to OPL3SAx, move
     } 

/* // Calibration:  Disabled because it makes a popping sound, and does not make the sound quality any better.
   outportb(WSS_ADDR, WSS_MCE | WSS_INTCON);  // Mode Change Enable + Interface Control
   outportb(WSS_DATA, 0x18);    // Enable full ACAL (Calibration): Bit Cal1+Cal2
   outportb(WSS_ADDR, 0);       // Disable MCE (Mode Change Enable)
   wss_wait();                  // GB 2015
   outportb(WSS_ADDR,WSS_ERRSTAT);
   while ((inportb(WSS_DATA) & 0x20)>0); // GB 2015, Wait for ACAL to finish 
   outportb(WSS_STATUS, 0); // ack any unresolved IRQs */

   // Unmute outputs 
   WSS_OUT(WSS_RDAC, 0);
   WSS_OUT(WSS_LDAC, 0);

   // Set transfer size: CoDec interrupt count - sample frames per half-buffer
   tmp = wss_dma_size / 4 - 1;       // always stereo, always 16 bit
   outportb(WSS_ADDR, WSS_CNT_LO);
   outportb(WSS_DATA, tmp & 0xFF);   // send lower byte of value
   outportb(WSS_ADDR, WSS_CNT_HI);
   outportb(WSS_DATA, tmp >> 8);     // send upper byte of value 

   // Start playback
   _dma_start(_sb_dma, wss_buf[0], wss_dma_size*2, TRUE, FALSE);

   WSS_OUT(WSS_INTCON, 0x05); // reg 9 interface control bit1: PEN - Playback Enable!  + bit3: Single DMA Channel
   WSS_OUT(WSS_PINCON, 0x02); // Bit 2: Enable interrupts in Pin Control reg 10. 

   // _eoi(_sb_irq); // issue an end of interrupt
   
   wss_in_use = TRUE;

   //rest(500); // G2017 Wait for end-of-DMA interrupt

   return 0;
}

//-------------------------------------------------------------------------------------

static void wss_exit()
{
   if (!wss_in_use)  return;
   _sb_voice(0);

   // Stop playback 
   WSS_OUT(WSS_PINCON, 0x00);  // Bit 2: Enable interrupts in Pin Control reg. 
   outportb(WSS_STATUS,   0);  // ack any unresolved IRQs
   WSS_OUT(WSS_INTCON, 0x04);  // interface control bit1: PEN Off, Bit4: remains at single.
   rest (100);                 // delay to ensure DMA transfer finishes
   _dma_stop(_sb_dma);         // GB 2015
   WSS_OUT(WSS_LDAC,   0x80);  // Mute Master
   WSS_OUT(WSS_RDAC,   0x80);  // Mute Master
 
   _remove_irq(wss_int);
   _restore_irq(_sb_irq);
   // free conventional memory buffer 
   __dpmi_free_dos_memory(wss_sel);
   _mixer_exit();

   // Playback format reset:
   //outportb(WSS_ADDR, WSS_MCE | WSS_FS); // Mode Change Enable + FS & Playback data format
   //outportb(WSS_DATA, 0);
   //outportb(WSS_ADDR, 0);                // Disable MCE (Mode Change Enable)

   wss_in_use = FALSE;
}

//-------------------------------------------------------------------------------------

static int wss_mixer_volume(int volume)
{
   if (volume > 255)
      volume = 255;

   if (volume < 0)
      volume = 0;

   // WSS has only attenuation regs, so we totally mute outputs to get
   //   silence w/volume 0 

   DISABLE();

   if (!volume) {
      WSS_OUT(WSS_RDAC, 0x80);
      WSS_OUT(WSS_LDAC, 0x80);
   }
   else {
      WSS_OUT(WSS_RDAC, 0x3F-volume/4);
      WSS_OUT(WSS_LDAC, 0x3F-volume/4);
	  // for CS4231, CS4232 and AD1848 : 0x00h is the maximum setting: 0dB. 0x3F is minimum: -94,5 dB
	  // for CS4236 it is the same, but there are higher dB settings between 0x47 and 0x40
   }

   ENABLE();
   return 0;
}

