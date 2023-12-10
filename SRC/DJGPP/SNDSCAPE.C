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
 *      Ensoniq Soundscape driver, by Andreas Kluge. (Adapted from allegro 3.12 by GB 2016)
 *
 *      Based on code by Andrew P. Weir. 
 *
 *      See readme.txt for copyright information.
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
#include <string.h>
#include <sys/farptr.h>

#include "internal.h"

/* The driver version numbers */
#define VER_MAJOR       1
#define VER_MINOR       00

/****************************************************************************
 VARS & DEFINES - all kinds of stuff ...
****************************************************************************/

/* Ensoniq gate-array chip defines ... */

#define ODIE            0                   /* ODIE gate array */
#define OPUS            1                   /* OPUS gate array */
#define MMIC            2                   /* MiMIC gate array */

static int soundscape_hw_ver = -1;          /* as reported by detection */
const char *EnsoniqGateArray[] = {"ODIE", "OPUS", "MiMIC"};

/* relevant direct register defines - offsets from base address */
#define GA_HOSTCTL_OFF  2                   /* host port ctrl/stat reg */
#define GA_ADDR_OFF     4                   /* indirect address reg */
#define GA_DATA_OFF     5                   /* indirect data reg */
#define GA_CODEC_OFF    8                   /* for some boards CoDec is fixed
					       from base */

/* relevant indirect register defines */
#define GA_DMAB_REG     3                   /* DMA chan B assign reg */
#define GA_INTCFG_REG   4                   /* interrupt configuration reg */
#define GA_DMACFG_REG   5                   /* DMA configuration reg */
#define GA_CDCFG_REG    6                   /* CD-ROM/CoDec config reg */
#define GA_HMCTL_REG    9                   /* host master control reg */

/* AD-1848 or compatible CoDec defines ... */
/* relevant direct register defines - offsets from base */
#define CD_ADDR_OFF     0                   /* indirect address reg */
#define CD_DATA_OFF     1                   /* indirect data reg */
#define CD_STATUS_OFF   2                   /* status register */

#define OUT_TO_ADDR(n)   outp(soundscape_waveport + CD_ADDR_OFF, n)
#define CODEC_MODE_CHANGE_ON   OUT_TO_ADDR(0x40)
#define CODEC_MODE_CHANGE_OFF  OUT_TO_ADDR(0x00)

/* relevant indirect register defines */
#define CD_ADCL_REG     0                   /* left DAC input control reg */
#define CD_ADCR_REG     1                   /* right DAC input control reg */
#define CD_CDAUXL_REG   2                   /* left DAC output control reg */
#define CD_CDAUXR_REG   3                   /* right DAC output control reg */
#define CD_DACL_REG     6                   /* left DAC output control reg */
#define CD_DACR_REG     7                   /* right DAC output control reg */
#define CD_FORMAT_REG   8                   /* clock and data format reg */
#define CD_CONFIG_REG   9                   /* interface config register */
#define CD_PINCTL_REG   10                  /* external pin control reg */
#define ENABLE_CODEC_IRQ   CdWrite(CD_PINCTL_REG, CdRead(CD_PINCTL_REG) | 0x02)
#define DISABLE_CODEC_IRQ  CdWrite(CD_PINCTL_REG, CdRead(CD_PINCTL_REG) & 0xfd);
#define CD_UCOUNT_REG   14                  /* upper count reg */
#define CD_LCOUNT_REG   15                  /* lower count reg */
#define CD_XFORMAT_REG  28                  /* extended format reg - 1845
					       record */
#define CD_XUCOUNT_REG  30                  /* extended upper count reg -
					       1845 record */
#define CD_XLCOUNT_REG  31                  /* extended lower count reg -
					       1845 record */

static short soundscape_dma;
static int soundscape_freq;
static unsigned short soundscape_baseport;    /* Gate Array/MPU-401 base port */
static unsigned short soundscape_waveport;    /* the AD-1848 base port */
static short soundscape_midiirq;            /* the MPU-401 IRQ */
static short soundscape_waveirq;            /* the PCM IRQ */

static int soundscape_detect();
static int soundscape_init(int voices);
static void soundscape_exit();
static int soundscape_mixer_volume(int volume);

static int soundscape_int = -1;             /* interrupt vector */
static int soundscape_dma_size = -1;        /* size of dma transfer in bytes */
static volatile int soundscape_semaphore = FALSE;       /* reentrant interrupt? */

static int soundscape_sel;                  /* selector for the DMA buffer */
static unsigned long soundscape_buf[2];     /* pointers to the two buffers */
static int soundscape_bufnum = 0;           /* the one currently in use */

static void soundscape_lock_mem();

static char soundscape_desc[120] = "not initialised";

short CdCfgSav;                             /* gate array register save area */
short DmaCfgSav;                            /* gate array register save area */
short IntCfgSav;                            /* gate array register save area */

short DacSavL;                              /* DAC left volume save */
short DacSavR;                              /* DAC right volume save */
short CdxSavL;                              /* CD/Aux left volume save */
short CdxSavR;                              /* CD/Aux right volume save */
short AdcSavL;                              /* ADC left volume save */
short AdcSavR;                              /* ADC right volume save */

static short const SsIrqs[4] = {9, 5, 7, 10};   /* Soundscape IRQs */
static short const RsIrqs[4] = {9, 7, 5, 15};   /* an older IRQ set */
static short const *soundscape_irqset;      /* pointer to one of the IRQ sets */
static int soundscape_detected = FALSE;     /* successful detection flag */

/* Some TypeDefs ... */
typedef struct {                            /* DMA controller registers ... */
   short addr;                              /* address register, lower/upper */
   short count;                             /* address register, lower/upper */
   short status;                            /* status register */
   short mask;                              /* single channel mask register */
   short mode;                              /* mode register */
   short clrff;                             /* clear flip-flop register */
   short page;                              /* fixed page register */
} DMAC_REGS;

DMAC_REGS const DmacRegs[4] = {             /* the DMAC regs for chans 0-3
					       ... */
   {0x00, 0x01, 0x08, 0x0a, 0x0b, 0x0c, 0x87},
   {0x02, 0x03, 0x08, 0x0a, 0x0b, 0x0c, 0x83},
   {0x04, 0x05, 0x08, 0x0a, 0x0b, 0x0c, 0x81},
   {0x06, 0x07, 0x08, 0x0a, 0x0b, 0x0c, 0x82}
};
DMAC_REGS const *DmacRegP;                  /* a pointer to a DMAC reg struct */

DIGI_DRIVER digi_sndscape =
{
   "Soundscape",
   soundscape_desc,
   0,
   0,
   MIXER_MAX_SFX,
   MIXER_DEF_SFX,

   /* setup routines */
   soundscape_detect,
   soundscape_init,
   soundscape_exit,
   soundscape_mixer_volume,

   /* voice control functions */
   _mixer_init_voice,
   _mixer_release_voice,
   _mixer_start_voice,
   _mixer_stop_voice,
   _mixer_loop_voice,

   /* position control functions */
   _mixer_get_position,
   _mixer_set_position,

   /* volume control functions */
   _mixer_get_volume,
   _mixer_set_volume,
   _mixer_ramp_volume,
   _mixer_stop_volume_ramp,

   /* pitch control functions */
   _mixer_get_frequency,
   _mixer_set_frequency,
   _mixer_sweep_frequency,
   _mixer_stop_frequency_sweep,

   /* pan control functions */
   _mixer_get_pan,
   _mixer_set_pan,
   _mixer_sweep_pan,
   _mixer_stop_pan_sweep,

   /* effect control functions */
   _mixer_set_echo,
   _mixer_set_tremolo,
   _mixer_set_vibrato,

   /* input functions */
/* 0,
   0,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL*/
};

/* ###################################################################### */

/*
  void CdWrite(unsigned short rnum, short value);
  This function is used to write the indirect addressed registers in the
  Ad-1848 or compatible CoDec. It will preserve the special function bits
  in the upper-nibble of the indirect address register.
  INPUTS:    rnum   - the numner of the indirect register to be read
	     value  - the byte value to be written to the indirect register
  RETURNS:   Nothing
*/
static void CdWrite(unsigned short rnum, short value)
{
   OUT_TO_ADDR((inp(soundscape_waveport + CD_ADDR_OFF) & 0xf0) | rnum);
   outp(soundscape_waveport + CD_DATA_OFF, value);
}

/*
  short CdRead(unsigned short rnum);
  This function is used to read the indirect addressed registers in the
  AD-1848 or compatible CoDec. It will preserve the special function bits
  in the upper-nibble of the indirect address register.
  INPUTS:    rnum  - the numner of the indirect register to be read
  RETURNS:   the contents of the indirect register are returned
*/
static short CdRead(unsigned short rnum)
{
   OUT_TO_ADDR((inp(soundscape_waveport + CD_ADDR_OFF) & 0xf0) | rnum);
   return inp(soundscape_waveport + CD_DATA_OFF);
}

/*
  void GaWrite(unsigned short rnum, short value);
  This function is used to write the indirect addressed registers in the
  Ensoniq Soundscape gate array.
  INPUTS:    rnum   - the numner of the indirect register to be read
	     value  - the byte value to be written to the indirect register
  RETURNS:   Nothing
*/
static void GaWrite(unsigned short rnum, short value)
{
   outp(soundscape_baseport + GA_ADDR_OFF, rnum);
   outp(soundscape_baseport + GA_DATA_OFF, value);
}

/*
  short GaRead(unsigned short rnum);
  This function is used to read the indirect addressed registers in the
  Ensoniq Soundscape gate array.
  INPUTS:     rnum  - the numner of the indirect register to be read
  RETURNS:    the contents of the indirect register are returned
*/
static short GaRead(unsigned short rnum)
{
   outp(soundscape_baseport + GA_ADDR_OFF, rnum);
   return inp(soundscape_baseport + GA_DATA_OFF);
}

/*
  void SetDacVol(short lvol, short rvol);
  This function sets the left and right DAC output level in the CoDec.
  INPUTS:    lvol  - left volume, 0-127
	     rvol  - right volume, 0-127
  RETURNS:   Nothing
*/
static void SetDacVol(short lvol, short rvol)
{
   CdWrite(CD_DACL_REG, ~(lvol >> 1) & 0x3f);
   CdWrite(CD_DACR_REG, ~(rvol >> 1) & 0x3f);
}

/*
  void SetCdRomVol(short lvol, short rvol);
  This function sets the left and right CD-ROM output level in the CoDec.
  INPUTS:    lvol  - left volume, 0-127
	     rvol  - right volume, 0-127
  RETURNS:   Nothing
*/
#ifdef CDROM_USED
static void SetCdRomVol(short lvol, short rvol)
{
   CdWrite(CD_CDAUXL_REG, ~(lvol >> 2) & 0x1f);
   CdWrite(CD_CDAUXR_REG, ~(rvol >> 2) & 0x1f);
}

#endif

/*
  void SetAdcVol(short lvol, short rvol);
  This function sets the left and right ADC input level in the CoDec.
  INPUTS:    lvol  - left volume, 0-127
	     rvol  - right volume, 0-127
  RETURNS:   Nothing
*/
static void SetAdcVol(short lvol, short rvol)
{
   CdWrite(CD_ADCL_REG, (CdRead(CD_ADCL_REG) & 0xf0) | (lvol & 0x7f) >> 3);
   CdWrite(CD_ADCR_REG, (CdRead(CD_ADCR_REG) & 0xf0) | (rvol & 0x7f) >> 3);
}

/*
  void PauseCoDec(void);
  This function will pause the CoDec auto-restart DMA process.
  Don't use this to stop the CoDec - use the StopCoDec function to do
  that; it will cleanup DRQs.
  INPUTS:        None
  RETURNS:       Nothing
*/
#ifdef SOMEONE_NEEDS_THAT
static void PauseCoDec(void)
{
   CdWrite(CD_CONFIG_REG, CdRead(CD_CONFIG_REG) & 0xfc);
}

#endif

/*
  void ResumeCoDec(short direction);
  This function will resume the CoDec auto-restart DMA process.
  INPUTS:    direction  - 0 for playback, 1 for record
  RETURNS:   Nothing
*/
static void ResumeCoDec(short direction)
{
   CdWrite(CD_CONFIG_REG, direction ? 0x02 : 0x01);
}

/* ######################################################################## */

/*
  SetFormat(unsigned short srate, short stereo, short size16bit, short direction);

  This function sets the CoDec audio data format for record or playback.

  INPUTS:
	srate      - the audio sample rate in Hertz
	stereo     - 0 for mono data, 1 for L/R interleaved stereo data
	size16bit  - 0 for 8-bit unsigned data, 1 for 16-bit signed data
	direction  - FALSE for playback, TRUE for record

  RETURNS:
	TRUE       - if successful
	FALSE      - if the sample rate is unacceptable

  Original notes about the buffersize:
   The following define is the digital audio buffer size, in bytes. This
   buffer will be logically divided in half by the driver; the application
   will fill the audio buffer in BUFFERSIZE/2 byte chunks. Generally,
   BUFFERSIZE/2 should be a multiple of 4 and should represent approximately
   1/16th sec of audio for the given format for continuous audio streaming,
   or smaller for interactive, multi-channel mixing applications. If multiple
   formats are required, this size can be adjusted dynamically. The buffersize
   5512U is set for 22050 Hz, 8-bit, stereo data.
*/

static int SetFormat(unsigned short srate, short stereo, short size16bit, short direction)
{
   short format;
   unsigned long i;

   /* init the format register value */
   format = 0;

   /* first, find the sample rate ... */

   switch (srate) {
      case 5512:
	 format = 0x01;
	 soundscape_dma_size = 512;
	 break;
      case 6615:
	 format = 0x0f;
	 soundscape_dma_size = 512;
	 break;
      case 8000:
	 format = 0x00;
	 soundscape_dma_size = 512;
	 break;
      case 9600:
	 format = 0x0e;
	 soundscape_dma_size = 512;
	 break;
      case 11025:
	 format = 0x03;
	 soundscape_dma_size = 512;
	 break;                             /* 11363 */
      case 16000:
	 format = 0x02;
	 soundscape_dma_size = 512;
	 break;                             /* 17046 */
      case 18900:
	 format = 0x05;
	 soundscape_dma_size = 1024;
	 break;
      case 22050:
	 format = 0x07;
	 soundscape_dma_size = 1024;
	 break;                             /* 22729 */
      case 27428:
	 format = 0x04;
	 soundscape_dma_size = 1024;
	 break;
      case 32000:
	 format = 0x06;
	 soundscape_dma_size = 2048;
	 break;
      case 33075:
	 format = 0x0d;
	 soundscape_dma_size = 2048;
	 break;
      case 37800:
	 format = 0x09;
	 soundscape_dma_size = 2048;
	 break;
      case 44100:
	 format = 0x0b;
	 soundscape_dma_size = 2048;
	 break;                             /* 44194 */
      case 48000:
	 format = 0x0c;
	 soundscape_dma_size = 2048;
	 break;
      default:
	 return FALSE;
   }

   /* set other format bits ... */
   if (stereo) {
      format |= 0x10;
   }
   if (size16bit) {
      format |= 0x40;
      soundscape_dma_size *= 2;             /* make a larger buffer */
   }

   soundscape_freq = srate;

   CODEC_MODE_CHANGE_ON;

   /* and write the format register */
   CdWrite(CD_FORMAT_REG, format);

   /* if not using ODIE and recording, setup extended format register */
   if (soundscape_hw_ver != ODIE && direction)
      CdWrite(CD_XFORMAT_REG, format & 0x70);

   /* delay for internal re-sync */
   for (i = 0; i < 200000; ++i)
      inp(soundscape_baseport + GA_ADDR_OFF);

   CODEC_MODE_CHANGE_OFF;

   return TRUE;
}

/*
  void StopCoDec(void);
  This function will stop the CoDec auto-restart DMA process.
  INPUTS:        None
  RETURNS:       Nothing
*/
static void StopCoDec(void)
{
   unsigned short i;

   CdWrite(CD_CONFIG_REG, CdRead(CD_CONFIG_REG) & 0xfc);
   /* Let the CoDec receive its last DACK(s). The DMAC must not be */
   /* masked while the CoDec has DRQs pending. */
   for (i = 0; i < 64 /* 256 */ ; i++)
      if (!(inp(DmacRegP->status) & (0x10 << soundscape_dma)))
	 break;
}

/*
  int GetIniConfigEntry(char *entry, char *dst, FILE *fp);

  This function parses a file (SNDSCAPE.INI) for a left-hand string and,
  if found, writes its associated right-hand value to a destination buffer.
  This function is case-insensitive.

  INPUTS:
	fp  - a file pointer to the open SNDSCAPE.INI config file
	dst - the destination buffer pointer
	entry - a pointer to the right-hand string

  RETURNS:
	0   - if successful
	-1  - if the right-hand string is not found or has no equate
	      in this case fp will be closed

  used only by GetIniConfig
*/

static int GetIniConfigEntry(char *entry, char *dest, FILE * fp)
{
   char str[83];
   char tokstr[33];
   char *p;

   /* make a local copy of the entry, upper-case it */
   strcpy(tokstr, entry);
   strupr(tokstr);

   /* rewind the file and try to find it ... */
   rewind(fp);
   for (;;) {
      /* get the next string from the file */
      fgets(str, 83, fp);
      if (feof(fp)) {
	 fclose(fp);
	 return -1;
      }

      /* properly terminate the string */
      for (p = str; *p != '\0'; ++p) {
	 if (*p == ' ' || *p == '\t' || *p == 0x0a || *p == 0x0d) {
	    *p = '\0';
	    break;
	 }
      }

      /* see if it's an 'equate' string; if so, zero the '=' */
      p = strchr(str, '=');
      if (!p)
	 continue;
      *p = '\0';

      /* upper-case the current string and test it */
      strupr(str);
      if (strcmp(str, tokstr))
	 continue;

      /* it's our string - copy the right-hand value to buffer */
      for (p = str + strlen(str) + 1; (*dest++ = *p++) != '\0';);
      break;
   }
   return 0;
}

/*
  int GetIniConfig(void);

  This function gets all parameters from a file (SNDSCAPE.INI)
  This function is case-insensitive.

  INPUTS:
	NONE

  RETURNS:
	TRUE   - if successful
	FALSE  - if not successful

  used only by _DetectSoundscape
*/

static int GetIniConfig(void)
{
   char str[78];
   char *ep;
   FILE *fp = NULL;

   /* get the environment var and build the filename; then open it */
   if (!(ep = getenv("SNDSCAPE")))
      return FALSE;

   strcpy(str, ep);
   if (str[strlen(str) - 1] == '\\')
      str[strlen(str) - 1] = '\0';
   strcat(str, "\\SNDSCAPE.INI");
   if (!(fp = fopen(str, "r")))
      return FALSE;

   /* read all of the necessary config info ... */
   if (GetIniConfigEntry("Product", str, fp))
      return FALSE;

   /* if an old product name is read, set the IRQs accordingly */
   strupr(str);
   if (strstr(str, "SOUNDFX") || strstr(str, "MEDIA_FX"))
      soundscape_irqset = RsIrqs;
   else
      soundscape_irqset = SsIrqs;

   if (GetIniConfigEntry("Port", str, fp))
      return FALSE;
   soundscape_baseport = (unsigned short) strtol(str, NULL, 16);

   if (GetIniConfigEntry("WavePort", str, fp))
      return FALSE;
   soundscape_waveport = (unsigned short) strtol(str, NULL, 16);

   if (GetIniConfigEntry("IRQ", str, fp))
      return FALSE;
   soundscape_midiirq = (short) strtol(str, NULL, 10);
   if (soundscape_midiirq == 2)
      soundscape_midiirq = 9;

   if (GetIniConfigEntry("SBIRQ", str, fp))
      return FALSE;
   soundscape_waveirq = (short) strtol(str, NULL, 10);
   if (soundscape_waveirq == 2)
      soundscape_waveirq = 9;

   if (GetIniConfigEntry("DMA", str, fp))
      return FALSE;
   soundscape_dma = (short) strtol(str, NULL, 10);

   fclose(fp);
   return TRUE;
}

/*
  int _DetectSoundscape(void);

  This function is used to detect the presence of a Soundscape card in a
  system. It will read the hardware config info from the SNDSCAPE.INI file,
  the path to which is indicated by the SNDSCAPE environment variable. This
  config info will be stored in global variable space for the other driver
  functions to reference. Once the config settings have been determined, a
  hardware test will be performed to see if the Soundscape card is actually
  present. If this function is not explicitly called by the application, it
  it will be called by the OpenSoundscape function.

  INPUTS:
	None

  RETURNS:
	TRUE  - if Soundscape detection was successful
	FALSE - if
		The SNDSCAPE environment variable was not found
		The SNDSCAPE.INI file cannot be opened
		The SNDSCAPE.INI file is missing information or is corrupted
		The Soundscape hardware is not detected

   used only by soundscape_detect
*/

int _DetectSoundscape(void)
{
   unsigned char tmp;
   int status = TRUE;

   int orig_dma = _sb_dma;

   if (GetIniConfig() == FALSE)
      return FALSE;

   /* see if Soundscape is there by reading HW ... */
   if ((inp(soundscape_baseport + GA_HOSTCTL_OFF) & 0x78) != 0x00)
      return FALSE;
   if ((inp(soundscape_baseport + GA_ADDR_OFF) & 0xf0) == 0xf0)
      return FALSE;

   outp(soundscape_baseport + GA_ADDR_OFF, 0xf5);
   tmp = inp(soundscape_baseport + GA_ADDR_OFF);
   if ((tmp & 0xf0) == 0xf0)
      return FALSE;
   if ((tmp & 0x0f) != 0x05)
      return FALSE;

   /* formulate the chip ID */
   if ((tmp & 0x80) != 0x00)
      soundscape_hw_ver = MMIC;
   else if ((tmp & 0x70) != 0x00)
      soundscape_hw_ver = OPUS;
   else
      soundscape_hw_ver = ODIE;

   /* now do a quick check to make sure the CoDec is there too */
   if ((inp(soundscape_waveport) & 0x80) != 0x00)
      return FALSE;

   soundscape_detected = TRUE;
   return TRUE;
}

/* ######################################################################## */

/* soundscape_mixer_volume:
 *  Sets the Soundscape mixer volume for playing digital samples.
 */
static int soundscape_mixer_volume(int volume)
{
   if (volume >= 0)
      SetDacVol(volume, volume);            /* set DAC level */
   return 0;
}

/* soundscape_interrupt:
 *  The Soundscape end-of-buffer interrupt handler.
 */
static int soundscape_interrupt()
{

   /* that's part of the original code, do we need this ? */
   /* With fix interrupt numbers ? */

   /* make sure we're not getting garbage IRQ edges ... */
   /* if ( soundscape_waveirq == 7 ) { outp(0x20, 0x0b); inp(0x21); if (
      !(inp(0x20) & 0x80) ) return 0; } else if ( soundscape_waveirq == 15 )
      { outp(0xa0, 0x0b); inp(0xa1); if ( !(inp(0xa0) & 0x80) ) return 0; } */

   /* if the CoDec is interrupting ... */
   if (inp(soundscape_waveport + CD_STATUS_OFF) & 0x01) {
      /* clear the AD-1848 interrupt */
      outp(soundscape_waveport + CD_STATUS_OFF, 0x00);

      if (!(soundscape_semaphore)) {
	 soundscape_semaphore = TRUE;

	 ENABLE();                          /* mix some more samples */
	 _mix_some_samples(soundscape_buf[soundscape_bufnum], _dos_ds, TRUE);
	 DISABLE();

	 soundscape_semaphore = FALSE;
      }

      soundscape_bufnum = 1 - soundscape_bufnum;
   }

   /* acknowledge interrupt */
   _eoi(soundscape_waveirq);
   return 0;
}
static END_OF_FUNCTION(soundscape_interrupt);

/* soundscape_detect:
 * Soundscape detection routine.
 * This function is used to detect the presence of a Soundscape card in a
 * system. It will read the hardware config info from the SNDSCAPE.INI file,
 * the path to which is indicated by the SNDSCAPE environment variable. This
 * config info will be stored in global variable space for the other driver
 * functions to reference. Once the config settings have been determined, a
 * hardware test will be performed to see if the Soundscape card is actually
 * present. If this function is not explicitly called by the application, it
 * it will be called by the OpenSoundscape function.
 *
 * INPUTS:
 *       None
 *
 * RETURNS:
 *       TRUE  - if Soundscape detection was successful
 *       FALSE - if
 *               The SNDSCAPE environment variable was not found
 *               The SNDSCAPE.INI file cannot be opened
 *               The SNDSCAPE.INI file is missing information or is corrupted
 *               The Soundscape hardware is not detected
 */
static int soundscape_detect()
{
   int status;

   status = _DetectSoundscape();
   if (status == FALSE)
      strcpy(allegro_error, "Soundscape not found");
   else {
      /* figure out the hardware interrupt number */
      soundscape_int = _map_irq(soundscape_waveirq);

      /* Get desired frequency from _sb-variable */
      soundscape_freq = _sb_freq;
      if (soundscape_freq == -1)
	 soundscape_freq = 48000;

      /* Adjust SB-frequencies for this CoDec */
      if (soundscape_freq < 12000)             /* 11906 -> 11025 */
	 soundscape_freq = 11025;
      else if (soundscape_freq < 18000)        /* 16129 -> 16000 */
	 soundscape_freq = 16000;
      else if (soundscape_freq < 24000)        /* 22727 -> 22050 */
	 soundscape_freq = 22050;
      else                                     /* 45454 -> 48000 */
	 soundscape_freq = 44100;  // GB 2016, was 48000

      /* set up the card description */
      sprintf(soundscape_desc, "Soundscape %s (%d Hz) on port %Xh, using IRQ %d, DMA %d, waveport %Xh",
	      EnsoniqGateArray[soundscape_hw_ver], soundscape_freq, soundscape_baseport, soundscape_waveirq, soundscape_dma, soundscape_waveport);
   }
   return status;
}

/* soundscape_init:
 *  Soundscape init routine: returns zero on succes, -1 on failure.
 *
 *  This function opens the Soundscape driver. It will setup the Soundscape
 *  hardware for Native PCM mode.
 *  It will first call the soundscape_detect function. It determines that it
 *  has not already been called.
 *
 *  RETURNS:
 *       0      - if successful
 *       -1     - if the soundscape_detect function needed to be called and
 *                returned unsuccessful, or if DOS mem could not be allocated
 */
static int soundscape_init(int voices)
{
   short windx,
    mindx;
   unsigned short tmp;

   /* see if we need to detect first */
   if (!soundscape_detected)
      if (soundscape_detect(0) == FALSE)
	 return -1;

//   if (input)                               /* input isn't supported yet */
//      return -1;

   /* Set DMA controller register set pointer based on channel */
   DmacRegP = &DmacRegs[soundscape_dma];

   /* In case the CoDec is running, stop it */
   StopCoDec();

   /* Clear possible CoDec and SoundBlaster emulation interrupts */
   outp(soundscape_waveport + CD_STATUS_OFF, 0x00);
   inp(0x22e);

   /* Set format, always stereo, 16 bit */
   SetFormat(soundscape_freq, 1, 1, 0);

   if (_dma_allocate_mem(soundscape_dma_size * 2, &soundscape_sel, &soundscape_buf[0]) != 0)
      return -1;

   soundscape_buf[1] = soundscape_buf[0] + soundscape_dma_size;

   soundscape_lock_mem();

   digi_sndscape.voices = voices;

   if (_mixer_init(soundscape_dma_size / 2, soundscape_freq, TRUE, TRUE, &digi_sndscape.voices) != 0)
      return -1;

   _mix_some_samples(soundscape_buf[0], _dos_ds, TRUE);
   _mix_some_samples(soundscape_buf[1], _dos_ds, TRUE);
   soundscape_bufnum = 0;

   _enable_irq(soundscape_waveirq);
   _install_irq(soundscape_int, soundscape_interrupt);

   /* If necessary, save some regs, do some resource re-routing */
   if (soundscape_hw_ver != MMIC) {
      /* derive the MIDI and Wave IRQ indices (0-3) for reg writes */
      for (mindx = 0; mindx < 4; mindx++)
	 if (soundscape_midiirq == *(soundscape_irqset + mindx))
	    break;
      for (windx = 0; windx < 4; windx++)
	 if (soundscape_waveirq == *(soundscape_irqset + windx))
	    break;

      /* setup the CoDec DMA polarity */
      GaWrite(GA_DMACFG_REG, 0x50);

      /* give the CoDec control of the DMA and Wave IRQ resources */
      CdCfgSav = GaRead(GA_CDCFG_REG);
      GaWrite(GA_CDCFG_REG, 0x89 | (soundscape_dma << 4) | (windx << 1));

      /* pull the Sound Blaster emulation off of those resources */
      DmaCfgSav = GaRead(GA_DMAB_REG);
      GaWrite(GA_DMAB_REG, 0x20);
      IntCfgSav = GaRead(GA_INTCFG_REG);
      GaWrite(GA_INTCFG_REG, 0xf0 | (mindx << 2) | mindx);
   }

   /* Save all volumes that we might use */
   DacSavL = CdRead(CD_DACL_REG);
   DacSavR = CdRead(CD_DACR_REG);
   CdxSavL = CdRead(CD_CDAUXL_REG);
   CdxSavR = CdRead(CD_CDAUXL_REG);
   AdcSavL = CdRead(CD_ADCL_REG);
   AdcSavR = CdRead(CD_ADCR_REG);

   SetDacVol(127, 127);

   /* Select the mic/line input to the record mux; */
   /* if not ODIE, set the mic gain bit too */
   CdWrite(CD_ADCL_REG, (CdRead(CD_ADCL_REG) & 0x3f) | (soundscape_hw_ver == ODIE ? 0x80 : 0xa0));
   CdWrite(CD_ADCR_REG, (CdRead(CD_ADCR_REG) & 0x3f) | (soundscape_hw_ver == ODIE ? 0x80 : 0xa0));

   /* Put the CoDec into mode change state */
   CODEC_MODE_CHANGE_ON;

   /* Setup CoDec mode - single DMA chan, AutoCal on */
   CdWrite(CD_CONFIG_REG, 0x0c);

   ENABLE_CODEC_IRQ;

/*  DISABLE(); */
   _dma_start(soundscape_dma, soundscape_buf[0], soundscape_dma_size * 2, TRUE, FALSE);
/*  ENABLE(); */

   /* Write the CoDec interrupt count - sample frames per half-buffer. */
   tmp = soundscape_dma_size / 4 - 1;       /* always stereo, always 16 bit */
   CdWrite(CD_LCOUNT_REG, tmp);
   CdWrite(CD_UCOUNT_REG, tmp >> 8);

   CODEC_MODE_CHANGE_OFF;

   ResumeCoDec(0);                          /* start the CoDec for output */
   rest(100);
   return 0;
}

/* soundscape_exit:
 *  Soundscape driver cleanup routine, removes ints, stops dma,
 *  frees buffers, etc.
 *  This function will make sure the CoDec is stopped, return the Soundscape
 *  hardware to it's default Sound Blaster emulation mode, de-install the PCM
 *  interrupt, restore the PIC mask, and do other cleanup.
 */
static void soundscape_exit()
{
   /* in case the CoDec is running, stop it */
   StopCoDec();

   /* stop dma transfer */
   _dma_stop(soundscape_dma);

      /* disable the CoDec interrupt pin */
      DISABLE_CODEC_IRQ;

      /* restore all volumes ... */
      CdWrite(CD_DACL_REG, DacSavL);
      CdWrite(CD_DACR_REG, DacSavR);
      CdWrite(CD_CDAUXL_REG, CdxSavL);
      CdWrite(CD_CDAUXL_REG, CdxSavR);
      CdWrite(CD_ADCL_REG, AdcSavL);
      CdWrite(CD_ADCR_REG, AdcSavR);

      /* if necessary, restore gate array resource registers */
      if (soundscape_hw_ver != MMIC) {
	 GaWrite(GA_INTCFG_REG, IntCfgSav);
	 GaWrite(GA_DMAB_REG, DmaCfgSav);
	 GaWrite(GA_CDCFG_REG, CdCfgSav);
      }

      _remove_irq(soundscape_int);          /* restore interrupts */
      _restore_irq(soundscape_waveirq);     /* reset PIC channels */
      __dpmi_free_dos_memory(soundscape_sel);   /* free conventional memory
						   buffer */
      _mixer_exit();
      soundscape_hw_ver = -1;
      soundscape_detected = FALSE;
}

/* soundscape_lock_mem:
 *  Locks all the memory touched by parts of the Soundscape code that are
 *  executed in an interrupt context.
 */
static void soundscape_lock_mem()
{
   LOCK_VARIABLE(digi_sndscape);
   LOCK_VARIABLE(soundscape_freq);
   LOCK_VARIABLE(soundscape_baseport);
   LOCK_VARIABLE(soundscape_waveport);
   LOCK_VARIABLE(soundscape_dma);
   LOCK_VARIABLE(soundscape_midiirq);
   LOCK_VARIABLE(soundscape_waveirq);
   LOCK_VARIABLE(soundscape_int);
   LOCK_VARIABLE(soundscape_hw_ver);
   LOCK_VARIABLE(soundscape_dma_size);
   LOCK_VARIABLE(soundscape_sel);
   LOCK_VARIABLE(soundscape_buf);
   LOCK_VARIABLE(soundscape_bufnum);
   LOCK_VARIABLE(soundscape_semaphore);
   LOCK_FUNCTION(soundscape_interrupt);
}
