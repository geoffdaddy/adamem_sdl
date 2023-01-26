/****************************************************************************/
/**                                                                        **/
/**                                 GUSFn.h                                **/
/**                                                                        **/
/** General Gravis UltraSound routines                                     **/
/** Based on the official Gravis Ultrasound development kit                **/
/**                                                                        **/
/** Copyright (C) Marcel de Kogel 1996,1997,1998                           **/
/**     You are not allowed to distribute this software commercially       **/
/**     Please, notify me, if you make any changes to this file            **/
/****************************************************************************/

/* #define FAKE_GUS */
/* #define TEST */
/* #define TEST_PROGRAM */

#ifdef TEST_PROGRAM
#define GUS_NUMCHANNELS 14
#endif

#ifdef __TURBOC__
 #include <dos.h>
 #define outportw       outpw
#else
 #include <pc.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>

#ifdef TEST_PROGRAM
 typedef unsigned char byte;
 typedef unsigned short word;
#endif
typedef unsigned long ulong;

#define MSB(a)  ((byte)((a>>8)&0xFF))
#define LSB(a)  ((byte)(a&0xFF))
#define MSW(a)  ((word)((a>>16)&0xFFFF))
#define LSW(a)  ((word)(a&0xFFFF))

#define USE_ROLLOVER	0x01
/* Make GF1 address for direct chip i/o. */
#define	ADDR_HIGH(x) ((unsigned int)((unsigned int)((x>>7L)&0x1fffL)))
#define	ADDR_LOW(x)  ((unsigned int)((unsigned int)((x&0x7fL)<<9L)))

#define GF1_MIDI_CTRL	0x100		/* 3X0 */
#define GF1_MIDI_DATA	0x101		/* 3X1 */
#define GF1_PAGE        0x102           /* 3X2 */
#define GF1_REG_SELECT	0x103		/* 3X3 */
#define GF1_DATA_LOW	0x104		/* 3X4 */
#define GF1_DATA_HI     0x105           /* 3X5 */
#define GF1_IRQ_STAT    0x006           /* 2X6 */
#define GF1_DRAM        0x107           /* 3X7 */
#define	GF1_MIX_CTRL	0x000		/* 2X0 */
#define GF1_TIMER_CTRL	0x008		/* 2X8 */
#define GF1_TIMER_DATA	0x009		/* 2X9 */
#define GF1_IRQ_CTRL	0x00B		/* 2XB */
/* The GF1 Hardware clock. */
#define CLOCK_RATE		9878400L
/* Mixer control bits. */
#define	ENABLE_LINE		0x01
#define	ENABLE_DAC		0x02
#define	ENABLE_MIC		0x04
#define DMA_CONTROL             0x41
#define	SET_DMA_ADDRESS		0x42
#define SET_DRAM_LOW		0x43
#define SET_DRAM_HIGH		0x44
#define TIMER_CONTROL		0x45
#define TIMER1                  0x46
#define TIMER2                  0x47
#define	SET_SAMPLE_RATE		0x48
#define	SAMPLE_CONTROL		0x49
#define SET_JOYSTICK		0x4B
#define MASTER_RESET		0x4C
/* Voice register mapping. */
#define SET_CONTROL             0x00
#define	SET_FREQUENCY		0x01
#define	SET_START_HIGH		0x02
#define	SET_START_LOW		0x03
#define	SET_END_HIGH		0x04
#define SET_END_LOW             0x05
#define	SET_VOLUME_RATE		0x06
#define	SET_VOLUME_START	0x07
#define	SET_VOLUME_END		0x08
#define SET_VOLUME              0x09
#define	SET_ACC_HIGH		0x0a
#define SET_ACC_LOW             0x0b
#define SET_BALANCE             0x0c
#define	SET_VOLUME_CONTROL 	0x0d
#define SET_VOICES              0x0e
#define GET_CONTROL             0x80
#define	GET_FREQUENCY		0x81
#define	GET_START_HIGH		0x82
#define	GET_START_LOW		0x83
#define	GET_END_HIGH		0x84
#define GET_END_LOW             0x85
#define	GET_VOLUME_RATE		0x86
#define	GET_VOLUME_START	0x87
#define	GET_VOLUME_END		0x88
#define GET_VOLUME              0x89
#define	GET_ACC_HIGH		0x8a
#define GET_ACC_LOW             0x8b
#define GET_BALANCE             0x8c
#define	GET_VOLUME_CONTROL 	0x8d
#define GET_VOICES              0x8e
#define GET_IRQV                0x8f
#define MIDI_RESET              0x03
#define	MIDI_ENABLE_XMIT	0x20
#define	MIDI_ENABLE_RCV		0x80
#define	MIDI_RCV_FULL		0x01
#define MIDI_XMIT_EMPTY		0x02
#define MIDI_FRAME_ERR		0x10
#define MIDI_OVERRUN		0x20
#define MIDI_IRQ_PEND		0x80
#define JOY_POSITION		0x0f
#define JOY_BUTTONS             0xf0
/* GF1_IRQ_STATUS (port 3X6) */
#define MIDI_TX_IRQ             0x01         /* pending MIDI xmit IRQ */
#define MIDI_RX_IRQ             0x02         /* pending MIDI recv IRQ */
#define GF1_TIMER1_IRQ          0x04         /* general purpose timer */
#define GF1_TIMER2_IRQ          0x08         /* general purpose timer */
#define WAVETABLE_IRQ           0x20         /* pending wavetable IRQ */
#define ENVELOPE_IRQ            0x40         /* pending volume envelope IRQ */
#define DMA_TC_IRQ              0x80         /* pending dma tc IRQ */
/* GF1_MIX_CTRL (port 2X0) */
#define ENABLE_LINE_IN		0x01		/* 0=enable */
#define ENABLE_OUTPUT		0x02		/* 0=enable */
#define ENABLE_MIC_IN		0x04		/* 1=enable */
#define ENABLE_GF1_IRQ		0x08		/* 1=enable */
#define GF122                   0x10            /* ?? */
#define ENABLE_MIDI_LOOP	0x20		/* 1=enable loop back */
#define SELECT_GF1_REG		0x40		/* 0=irq latches */
/* SAMPLE control register */
#define ENABLE_ADC              0x01
#define ADC_MODE                0x02            /* 0=mono, 1=stereo */
#define ADC_DMA_WIDTH		0x04		/* 0=8 bit, 1=16 bit */
#define ADC_IRQ_ENABLE		0x20		/* 1=enable */
#define ADC_IRQ_PENDING		0x40		/* 1=irq pending */
#define ADC_TWOS_COMP		0x80		/* 1=do twos comp */
/* RESET control register */
#define GF1_MASTER_RESET	0x01		/* 0=hold in reset */
#define GF1_OUTPUT_ENABLE	0x02		/* enable output */
#define GF1_MASTER_IRQ		0x04		/* master IRQ enable */
/* ($0,$80) Voice control register */
#define VOICE_STOPPED           0x01           /* voice has stopped */
#define STOP_VOICE              0x02           /* stop voice */
#define VC_DATA_TYPE            0x04           /* 0=8 bit,1=16 bit */
#define VC_LOOP_ENABLE          0x08           /* 1=enable */
#define VC_BI_LOOP              0x10           /* 1=bi directional looping */
#define VC_WAVE_IRQ             0x20           /* 1=enable voice's wave irq */
#define VC_DIRECT               0x40           /* 0=increasing,1=decreasing */
#define VC_IRQ_PENDING          0x80           /* 1=wavetable irq pending */
/* ($1,$81) Frequency control */
/* Bit 0  - Unused */
/* Bits 1-9 - Fractional portion */
/* Bits 10-15 - Integer portion */
/* ($2,$82) Accumulator start address (high) */
/* Bits 0-11 - HIGH 12 bits of address */
/* Bits 12-15 - Unused */
/* ($3,$83) Accumulator start address (low) */
/* Bits 0-4 - Unused */
/* Bits 5-8 - Fractional portion */
/* Bits 9-15 - Low 7 bits of integer portion */
/* ($4,$84) Accumulator end address (high) */
/* Bits 0-11 - HIGH 12 bits of address */
/* Bits 12-15 - Unused */
/* ($5,$85) Accumulator end address (low) */
/* Bits 0-4 - Unused */
/* Bits 5-8 - Fractional portion */
/* Bits 9-15 - Low 7 bits of integer portion */
/* ($6,$86) Volume Envelope control register */
#define VL_RATE_MANTISSA		0x3f
#define VL_RATE_RANGE			0xC0
/* ($7,$87) Volume envelope start */
#define	VL_START_MANT			0x0F
#define VL_START_EXP			0xF0
/* ($8,$88) Volume envelope end */
#define VL_END_MANT                     0x0F
#define VL_END_EXP                      0xF0
/* ($9,$89) Current volume register */
/* Bits 0-3 are unused */
/* Bits 4-11 - Mantissa of current volume */
/* Bits 10-15 - Exponent of current volume */
/* ($A,$8A) Accumulator value (high) */
/* Bits 0-12 - HIGH 12 bits of current position (a19-a7) */
/* ($B,$8B) Accumulator value (low) */
/* Bits 0-8 - Fractional portion */
/* Bits 9-15 - Integer portion of low adress (a6-a0) */
/* ($C,$8C) Pan (balance) position */
/* Bits 0-3 - Balance position  0=full left, 0x0f=full right */
/* ($D,$8D) Volume control register */
#define VOLUME_STOPPED          0x01         /* volume has stopped */
#define STOP_VOLUME             0x02         /* stop volume */
#define VC_ROLLOVER             0x04         /* Roll PAST end & gen IRQ */
#define VL_LOOP_ENABLE          0x08         /* 1=enable */
#define VL_BI_LOOP              0x10         /* 1=bi directional looping */
#define VL_WAVE_IRQ             0x20         /* 1=enable voice's wave irq */
#define VL_DIRECT               0x40         /* 0=increasing,1=decreasing */
#define VL_IRQ_PENDING          0x80         /* 1=wavetable irq pending */
/* ($E,$8E) # of Active voices */
/* Bits 0-5 - # of active voices -1 */
/* ($F,$8F) - Sources of IRQs */
/* Bits 0-4 - interrupting voice number */
/* Bit 5 - Always a 1 */
#define VOICE_VOLUME_IRQ        0x40         /* individual voice irq bit */
#define VOICE_WAVE_IRQ          0x80         /* individual waveform irq bit */

static int gus_baseport=0x220;
static int gus_dram_dma=1;
static int gus_adc_dma=1;
static int gus_irq=11;
static int gus_midi_irq=5;
static int gus_mix_image=0x0B;
static int gus_numvoices;

static ulong convert_to_16bit(ulong address);
static int UltraGetCfg (void);
static int UltraOpen (int voices);
static int UltraClose(void);
static byte UltraPeekData (ulong address);
static void UltraPokeData (ulong address, byte data);
static void UltraSetFrequency(int voice,ulong speed);
/* 2011-01-23 functions unused, so commented out.
static void UltraSetLoopMode(int voice,byte mode);
static void UltraSetVoice(int voice,ulong location);
*/
static byte UltraPrimeVoice
 (int voice,ulong begin,ulong start,ulong end,byte mode);
static void UltraGoVoice(int voice,byte mode);
static void UltraStartVoice
        (int voice,ulong begin,ulong start,ulong end,byte mode);
/* 2011-01-23 functions unused, so commented out.
static void UltraStopVoice(int voice);
*/
static void UltraSetVolume(int voice,unsigned volume);
static int UltraPing(unsigned port);
static int UltraProbe(unsigned port);
static void UltraSetInterface(int dram,int adc,int gf1,int midi);
static int UltraReset(int voices);
/* 2011-01-23 functions unused, so commented out.
static int UltraGetOutput(void);
*/
static void UltraEnableOutput(void);
static void UltraDisableOutput(void);
/* 2011-01-23 functions unused, so commented out.
static int UltraGetLineIn(void);
static void UltraEnableLineIn(void);
*/
static void UltraDisableLineIn(void);
/* 2011-01-23 functions unused, so commented out.
static int UltraGetMicIn(void);
static void UltraEnableMicIn(void);
*/
static void UltraDisableMicIn(void);
static void gf1_delay(void);
/* 2011-01-23 functions unused, so commented out.
static void UltraSetBalance(int voice,int data);
*/
static int UltraSizeDram(void);

static ulong convert_to_16bit(ulong address)
/* ulong address                20 bit ultrasound dram address */
{
 ulong hold_address;
 hold_address = address;
 /* Convert to 16 translated address. */
 address = address >> 1;
 /* Zero out bit 17. */
 address &= 0x0001ffffL;
 /* Reset bits 18 and 19. */
 address |= (hold_address & 0x000c0000L);
 return (address);
}

static int UltraGetCfg (void)
{
 char *ptr;
 gus_baseport=0;
 ptr = getenv("ULTRASND");
 if (ptr)
 {
  sscanf(ptr,"%x,%d,%d,%d,%d",&gus_baseport,
                              &gus_dram_dma,
                              &gus_adc_dma,
                              &gus_irq,
                              &gus_midi_irq);
 }
 return(gus_baseport);
}

static int UltraOpen (int voices)
{
 gus_numvoices=voices;
 if (!UltraProbe(gus_baseport))
  return 0;
 UltraDisableLineIn();
 UltraDisableMicIn();
 UltraDisableOutput();
 if (!UltraReset(voices))
  return 0;
 UltraSetInterface(gus_dram_dma,gus_adc_dma,
                   gus_irq,gus_midi_irq);
 UltraEnableOutput();
 return 1;
}

static int UltraClose(void)
{
 UltraDisableOutput();
 UltraDisableLineIn();
 UltraDisableMicIn();
 UltraReset(14);
 return 1;
}

static byte UltraPeekData (ulong address)
{
 outportb(gus_baseport+GF1_REG_SELECT,SET_DRAM_LOW);
 outportw(gus_baseport+GF1_DATA_LOW,LSW(address));      /* 16 bits */
 outportb(gus_baseport+GF1_REG_SELECT,SET_DRAM_HIGH);
 outportb(gus_baseport+GF1_DATA_HI,LSB(MSW(address)));   /* 8 bits */
 return (inportb(gus_baseport+GF1_DRAM));
}

static void UltraPokeData (ulong address, byte data)
{
 outportb(gus_baseport+GF1_REG_SELECT,SET_DRAM_LOW);
 outportw(gus_baseport+GF1_DATA_LOW,LSW(address));      /* 16 bits */
 outportb(gus_baseport+GF1_REG_SELECT,SET_DRAM_HIGH);
 outportb(gus_baseport+GF1_DATA_HI,LSB(MSW(address)));   /* 8 bits */
 outportb(gus_baseport+GF1_DRAM,data);
}

/* The formula for this table is:
	1,000,000 / (1.619695497 * # of active voices)

	The 1.619695497 is calculated by knowing that 14 voices
		gives exactly 44.1 Khz. Therefore, 
		1,000,000 / (X * 14) = 44100
		X = 1.619695497
*/

static ulong freq_divisor[19] = {
	44100,		/* 14 active voices */
	41160,		/* 15 active voices */
	38587,		/* 16 active voices */
	36317,		/* 17 active voices */
	34300,		/* 18 active voices */
	32494,		/* 19 active voices */
	30870,		/* 20 active voices */
	29400,		/* 21 active voices */
	28063,		/* 22 active voices */
	26843,		/* 23 active voices */
	25725,		/* 24 active voices */
	24696,		/* 25 active voices */
	23746,		/* 26 active voices */
	22866,		/* 27 active voices */
	22050,		/* 28 active voices */
	21289,		/* 29 active voices */
	20580,		/* 30 active voices */
	19916,		/* 31 active voices */
	19293}		/* 32 active voices */
;

/* speed is in Hz */
static void UltraSetFrequency(int voice,ulong speed)
{
 unsigned fc;
 unsigned integer_portion;
 unsigned fractional_portion;
 ulong temp;
 /* FC is calculated based on the # of active voices ... */
 temp = (unsigned long)freq_divisor[gus_numvoices-14];
 integer_portion=speed/temp;
 fractional_portion=((speed%temp)<<16)/temp;
 fc=((integer_portion<<10)+(fractional_portion>>6))&(~1);
 outportb(gus_baseport+GF1_PAGE,voice);
 outportb(gus_baseport+GF1_REG_SELECT,SET_FREQUENCY);
 outportw(gus_baseport+GF1_DATA_LOW,fc);
}

/* 2011-01-23 functions unused, so commented out.
static void UltraSetLoopMode(int voice,byte mode)
{
 byte data;
 byte vmode;
 outportb(gus_baseport+GF1_PAGE,voice);
 // set/reset the rollover bit as per user request
 outportb(gus_baseport+GF1_REG_SELECT,GET_VOLUME_CONTROL);
 vmode = inportb (gus_baseport+GF1_DATA_HI);
 if (mode & USE_ROLLOVER)
  vmode |= VC_ROLLOVER;
 else
  vmode &= ~VC_ROLLOVER;
 outportb(gus_baseport+GF1_REG_SELECT,SET_VOLUME_CONTROL);
 outportb(gus_baseport+GF1_DATA_HI,vmode);
 gf1_delay();
 outportb(gus_baseport+GF1_DATA_HI,vmode);
 outportb(gus_baseport+GF1_REG_SELECT,GET_CONTROL);
 data = inportb(gus_baseport+GF1_DATA_HI);
 data &= ~(VC_WAVE_IRQ|VC_BI_LOOP|VC_LOOP_ENABLE); // isolate the bits
 mode &= VC_WAVE_IRQ|VC_BI_LOOP|VC_LOOP_ENABLE;    // no bad bits passed in
 data |= mode;                                     // turn on proper bits ...
 outportb(gus_baseport+GF1_REG_SELECT,SET_CONTROL);
 outportb(gus_baseport+GF1_DATA_HI,data);
 gf1_delay();
 outportb(gus_baseport+GF1_REG_SELECT,SET_CONTROL);
 outportb(gus_baseport+GF1_DATA_HI,data);
}

static void UltraSetVoice(int voice,ulong location)
{
 ulong phys_loc;
 byte data;
 // Make sure was are talking to proper voice
 outportb(gus_baseport+GF1_PAGE,voice);
 outportb(gus_baseport+GF1_REG_SELECT,GET_CONTROL);
 data = inportb(gus_baseport+GF1_DATA_HI);
 if (data & VC_DATA_TYPE)
  phys_loc   = convert_to_16bit(location);
 else
  phys_loc = location;
 // First set accumulator to beginning of data
 outportb(gus_baseport+GF1_REG_SELECT,SET_ACC_HIGH);
 outportw(gus_baseport+GF1_DATA_LOW,ADDR_HIGH(phys_loc));
 outportb(gus_baseport+GF1_REG_SELECT,SET_ACC_LOW);
 outportw(gus_baseport+GF1_DATA_LOW,ADDR_LOW(phys_loc));
}
*/

static byte UltraPrimeVoice
 (int voice,ulong begin,ulong start,ulong end,byte mode)
/* voice        voice to start */
/* begin        start location in ultra DRAM */
/* start        start loop location in ultra DRAM */
/* end          end location in ultra DRAM */
/* mode         mode to run the voice (loop etc) */
{
 ulong phys_start,phys_end;
 ulong phys_begin;
 ulong temp;
 byte vmode;
 /* if start is greater than end, flip 'em and turn on */
 /* decrementing addresses */
 if (start > end)
 {
  temp = start;
  start = end;
  end = temp;
  mode |= VC_DIRECT;
 }
 /* if 16 bit data, must convert addresses */
 if (mode & VC_DATA_TYPE)
 {
  phys_begin = convert_to_16bit(begin);
  phys_start = convert_to_16bit(start);
  phys_end   = convert_to_16bit(end);
 }
 else
 {
  phys_begin = begin;
  phys_start = start;
  phys_end = end;
 }
 /* Make sure we are talking to proper voice */
 outportb(gus_baseport+GF1_PAGE,voice);
 /* set/reset the rollover bit as per user request */
 outportb(gus_baseport+GF1_REG_SELECT,GET_VOLUME_CONTROL);
 vmode = inportb (gus_baseport+GF1_DATA_HI);
 if (mode & USE_ROLLOVER)
  vmode |= VC_ROLLOVER;
 else
  vmode &= ~VC_ROLLOVER;
 outportb(gus_baseport+GF1_REG_SELECT,SET_VOLUME_CONTROL);
 outportb(gus_baseport+GF1_DATA_HI,vmode);
 gf1_delay();
 outportb(gus_baseport+GF1_DATA_HI,vmode);
 /* First set accumulator to beginning of data */
 outportb(gus_baseport+GF1_REG_SELECT,SET_ACC_LOW);
 outportw(gus_baseport+GF1_DATA_LOW,ADDR_LOW(phys_begin));
 outportb(gus_baseport+GF1_REG_SELECT,SET_ACC_HIGH);
 outportw(gus_baseport+GF1_DATA_LOW,ADDR_HIGH(phys_begin));
 /* Set start loop address of buffer */
 outportb(gus_baseport+GF1_REG_SELECT,SET_START_HIGH);
 outportw(gus_baseport+GF1_DATA_LOW,ADDR_HIGH(phys_start));
 outportb(gus_baseport+GF1_REG_SELECT,SET_START_LOW);
 outportw(gus_baseport+GF1_DATA_LOW,ADDR_LOW(phys_start));
 /* Set end address of buffer */
 outportb(gus_baseport+GF1_REG_SELECT,SET_END_HIGH);
 outportw(gus_baseport+GF1_DATA_LOW,ADDR_HIGH(phys_end));
 outportb(gus_baseport+GF1_REG_SELECT,SET_END_LOW);
 outportw(gus_baseport+GF1_DATA_LOW,ADDR_LOW(phys_end));
 return(mode);
}

static void UltraGoVoice(int voice,byte mode)
{
 /* Make sure we are talking to proper voice */
 outportb(gus_baseport+GF1_PAGE,voice);
 mode &= ~(VOICE_STOPPED|STOP_VOICE);    /* turn 'stop' bits off ... */
 /* NOTE: no irq's from the voice ... */
 outportb(gus_baseport+GF1_REG_SELECT,SET_CONTROL);
 outportb(gus_baseport+GF1_DATA_HI,mode);
 gf1_delay();
 outportb(gus_baseport+GF1_DATA_HI,mode);
}

/**********************************************************************
 *
 * This function will start playing a wave out of DRAM. It assumes
 * the playback rate, volume & balance have been set up before ...
 *
 *********************************************************************/

static void UltraStartVoice
        (int voice,ulong begin,ulong start,ulong end,byte mode)
/* voice        voice to start */
/* begin        start location in ultra DRAM */
/* start        start loop location in ultra DRAM */
/* end          end location in ultra DRAM */
/* mode         mode to run the voice (loop etc) */
{
 mode = UltraPrimeVoice(voice,begin,start,end,mode);
 UltraGoVoice(voice,mode);
}

/***************************************************************
 * This function will stop a given voices output. Note that a delay
 * is necessary after the stop is issued to ensure the self-
 * modifying bits aren't a problem.
 ***************************************************************/

/* 2011-01-23 functions unused, so commented out.
static void UltraStopVoice(int voice)
{
 byte data;
 outportb(gus_baseport+GF1_PAGE,voice);                     // select the proper voice
                                                            // turn off the roll over bit first ...
 outportb(gus_baseport+GF1_REG_SELECT,GET_VOLUME_CONTROL);
 data = inportb (gus_baseport+GF1_DATA_HI);
 data &= ~VC_ROLLOVER;
 outportb(gus_baseport+GF1_REG_SELECT,SET_VOLUME_CONTROL);
 outportb(gus_baseport+GF1_DATA_HI,data);                   // turn it off
 gf1_delay();
 outportb(gus_baseport+GF1_DATA_HI,data);                   // turn it off
                                                            // Now stop the voice
 outportb(gus_baseport+GF1_REG_SELECT,GET_CONTROL);
 data = inportb (gus_baseport+GF1_DATA_HI);
 data &= ~VC_WAVE_IRQ;                                      // disable irq's & stop voice ..
 data |= VOICE_STOPPED|STOP_VOICE;
 outportb(gus_baseport+GF1_REG_SELECT,SET_CONTROL);
 outportb(gus_baseport+GF1_DATA_HI,data);                   // turn it off
 gf1_delay();
 outportb(gus_baseport+GF1_DATA_HI,data);                   // turn it off
}
*/

static void UltraSetVolume(int voice,unsigned volume)
{
 outportb(gus_baseport+GF1_PAGE,voice);
 outportb(gus_baseport+GF1_REG_SELECT,SET_VOLUME);
 outportw(gus_baseport+GF1_DATA_LOW,volume<<4);
}

static int UltraPing(unsigned port)
{
 byte val0,val1;
 byte save_val0,save_val1;
 gus_baseport=port;
 /* save current values ... */
 save_val0 = UltraPeekData(0L);
 save_val1 = UltraPeekData(1L);
 UltraPokeData(0L,0xAA);
 UltraPokeData(1L,0x55);
 val0 = UltraPeekData(0L);
 val1 = UltraPeekData(1L);
 /* restore data to old values ... */
 UltraPokeData(0L,save_val0);
 UltraPokeData(1L,save_val1);
 if ((val0 == (byte)0xAA) && (val1 == (byte)0x55))
  return 1;
 else
  return 0;
}

static int UltraProbe(unsigned port)
{
 int val;
 /* Pull a reset on the GF1 */
 outportb(gus_baseport+GF1_REG_SELECT,MASTER_RESET);
 outportb(gus_baseport+GF1_DATA_HI,0x00);
 /* Wait a little while ... */
 gf1_delay();
 gf1_delay();
 /* Release Reset */
 outportb(gus_baseport+GF1_REG_SELECT,MASTER_RESET);
 outportb(gus_baseport+GF1_DATA_HI,GF1_MASTER_RESET);
 gf1_delay();
 gf1_delay();
 val = UltraPing(port);
 return(val);
}

static void UltraSetInterface(int dram,int adc,int gf1,int midi)
/* dram         dram dma chan */
/* adc          adc dma chan */
/* gf1          gf1 irq # */
/* midi         midi irq # */
{
 byte _gf1_irq[16]= { 0,0,1,3,0,2,0,4,0,0,0,5,6,0,0,7 };
 byte _gf1_dma[8]= { 0,1,0,2,0,3,4,5 };
 byte gf1_irq, midi_irq, dram_dma=0, adc_dma=0;
 byte irq_control, dma_control;
 byte mix_image;
 /* Don't need to check for 0 irq #. Its latch entry = 0 */
 gf1_irq = _gf1_irq[gf1];
 midi_irq = _gf1_irq[midi];
 midi_irq <<= 3;
 /* Set latch to 0 if dma channel is 0 */
 /* This takes this channel off line ... */
 if (dram != 0) 
  dram_dma = _gf1_dma[dram];
 if (adc != 0)
  adc_dma = _gf1_dma[adc];
 adc_dma <<= 3;
 irq_control = dma_control = 0x0;
 mix_image = gus_mix_image;/* init image to everything disabled */
 irq_control |= gf1_irq;
 if((gf1 == midi) && (gf1 != 0))
  irq_control |= 0x40;
 else
  irq_control |= midi_irq;
 dma_control |= dram_dma;
 if((dram == adc) && (dram != 0))
  dma_control |= 0x40;
 else
  dma_control |= adc_dma;
 /* Set up for Digital ASIC */
 outportb(gus_baseport+0x0f,0x5);
 outportb(gus_baseport+GF1_MIX_CTRL,mix_image);
 outportb(gus_baseport+GF1_IRQ_CTRL,0x0);
 outportb(gus_baseport+0x0f,0x0);
 /* First do DMA control register */
 outportb( gus_baseport+GF1_MIX_CTRL, mix_image );             
 outportb( gus_baseport+GF1_IRQ_CTRL, dma_control|0x80 );
 /* IRQ CONTROL REG */
 outportb( gus_baseport+GF1_MIX_CTRL, mix_image|0x40 );             
 outportb( gus_baseport+GF1_IRQ_CTRL, irq_control );
 /* First do DMA control register */
 outportb( gus_baseport+GF1_MIX_CTRL, mix_image );             
 outportb( gus_baseport+GF1_IRQ_CTRL, dma_control );
 /* IRQ CONTROL REG */
 outportb( gus_baseport+GF1_MIX_CTRL, mix_image|0x40 );             
 outportb( gus_baseport+GF1_IRQ_CTRL, irq_control );
 /* IRQ CONTROL, ENABLE IRQ */
 /* just to Lock out writes to irq\dma register ... */
 outportb( gus_baseport+GF1_PAGE, 0 );
 /* enable output & irq, disable line & mic input */
 mix_image |= 0x09;
 outportb( gus_baseport+GF1_MIX_CTRL, mix_image );             
 /* just to Lock out writes to irq\dma register ... */
 outportb( gus_baseport+GF1_PAGE, 0x0 );
 /* put image back .... */
 gus_mix_image = mix_image;
}

/***************************************************************
 * This function performs a full reset & initialization on the 
 * UltraSound card.
 ***************************************************************/
static int UltraReset(int voices)
{
 int v;
 int select,data_low,data_hi;
 if (voices<14)
  voices=14;
 if (voices>32)
  voices=32;
 gus_numvoices = voices;
 select = gus_baseport+GF1_REG_SELECT;
 data_low = gus_baseport+GF1_DATA_LOW;
 data_hi = gus_baseport+GF1_DATA_HI;
 /* Set these to zero so the they don't get summed in for voices that are */
 /* not running. If their volumes are not at zero, whatever value they */
 /* are pointing at, will get summed into the output. By setting that */
 /* location to 0, that voice will have no contribution to the output */
 /* (2 locations are done in case voice is set to 16 bits ... ) */
 UltraPokeData(0L,0);
 UltraPokeData(1L,0);
 /* Pull a reset on the GF1 */
 outportb(select,MASTER_RESET);
 outportb(data_hi,0x00);
 /* Wait a little while ... */
 for (v=0;v<10;v++)
  gf1_delay();
 /* Release Reset */
 outportb(select,MASTER_RESET);
 outportb(data_hi,GF1_MASTER_RESET);
 /* Wait a little while ... */
 for (v=0;v<10;v++)
  gf1_delay();
 /* Reset the MIDI port also */
 outportb(gus_baseport+GF1_MIDI_CTRL,MIDI_RESET);
 for (v=0;v<10;v++)
  gf1_delay();
 outportb(gus_baseport+GF1_MIDI_CTRL,0x00);
 /* Clear all interrupts. */
 outportb(select,DMA_CONTROL);
 outportb(data_hi,0x00);
 outportb(select,TIMER_CONTROL);
 outportb(data_hi,0x00);
 outportb(select,SAMPLE_CONTROL);
 outportb(data_hi,0x00);
 /* Set the number of active voices */
 outportb(select,SET_VOICES); 
 outportb(data_hi,(char)((voices-1) | 0xC0));
 /* Clear interrupts on voices. */
 /* Reading the status ports will clear the irqs. */
 inportb (gus_baseport+GF1_IRQ_STAT); 
 outportb(select,DMA_CONTROL);
 inportb (data_hi);
 outportb(select,SAMPLE_CONTROL);
 inportb (data_hi);
 outportb(select,GET_IRQV);
 inportb (data_hi);
 for (v=0;v<voices;v++)
 {
  /* Select the proper voice */
  outportb(gus_baseport+GF1_PAGE,v);
  /* Stop the voice and volume */
  outportb(select,SET_CONTROL);
  outportb(data_hi,VOICE_STOPPED|STOP_VOICE);
  outportb(select,SET_VOLUME_CONTROL);
  outportb(data_hi,VOLUME_STOPPED|STOP_VOLUME);
  gf1_delay(); /* Wait 4.8 micos. or more. */
  /* Initialize each voice specific registers. This is not */
  /* really necessary, but is nice for completeness sake .. */
  /* Each application will set up these to whatever values */
  /* it needs. */
  outportb(select,SET_FREQUENCY);
  outportw(data_low,0x0400);
  outportb(select,SET_START_HIGH);
  outportw(data_low,0);
  outportb(select,SET_START_LOW);
  outportw(data_low,0);
  outportb(select,SET_END_HIGH);
  outportw(data_low,0);
  outportb(select,SET_END_LOW);
  outportw(data_low,0);
  outportb(select,SET_VOLUME_RATE);
  outportb(data_hi,0x01);
  outportb(select,SET_VOLUME_START);
  outportb(data_hi,0x10);
  outportb(select,SET_VOLUME_END);
  outportb(data_hi,0xe0);
  outportb(select,SET_VOLUME);
  outportw(data_low,0x0000);
  outportb(select,SET_ACC_HIGH);
  outportw(data_low,0);
  outportb(select,SET_ACC_LOW);
  outportw(data_low,0);
  outportb(select,SET_BALANCE);
  outportb(data_hi,0x07);
 }
 inportb (gus_baseport+GF1_IRQ_STAT); 
 outportb(select,DMA_CONTROL);
 inportb (data_hi);
 outportb(select,SAMPLE_CONTROL);
 inportb (data_hi);
 outportb(select,GET_IRQV);
 inportb (data_hi);
 /* Set up GF1 Chip for interrupts & enable DACs. */
 outportb(select,MASTER_RESET);
 outportb(data_hi,GF1_MASTER_RESET|GF1_OUTPUT_ENABLE|GF1_MASTER_IRQ);
 return 1;
}

/* 2011-01-23 functions unused, so commented out.
static int UltraGetOutput(void)
{
 return (!(gus_mix_image & ENABLE_OUTPUT));
}
*/

static void UltraEnableOutput(void)
{
 gus_mix_image &= ~ENABLE_OUTPUT;
 outportb(gus_baseport+GF1_MIX_CTRL,gus_mix_image);
}

static void UltraDisableOutput(void)
{
 gus_mix_image |= ENABLE_OUTPUT;
 outportb(gus_baseport+GF1_MIX_CTRL,gus_mix_image);
}

/* 2011-01-23 functions unused, so commented out.
static int UltraGetLineIn(void)
{
 return(!(gus_mix_image & ENABLE_LINE_IN));
}

static void UltraEnableLineIn(void)
{
 gus_mix_image &= ~ENABLE_LINE_IN;
 outportb(gus_baseport+GF1_MIX_CTRL,gus_mix_image);
}
*/

static void UltraDisableLineIn(void)
{
 gus_mix_image |= ENABLE_LINE_IN;
 outportb(gus_baseport+GF1_MIX_CTRL,gus_mix_image);
}

/* 2011-01-23 functions unused, so commented out.
static int UltraGetMicIn(void)
{
 return(gus_mix_image & ENABLE_MIC_IN);
}

static void UltraEnableMicIn(void)
{
 gus_mix_image |= ENABLE_MIC_IN;
 outportb(gus_baseport+GF1_MIX_CTRL,gus_mix_image);
}
*/

static void UltraDisableMicIn(void)
{
 gus_mix_image &= ~ENABLE_MIC_IN;
 outportb(gus_baseport+GF1_MIX_CTRL,gus_mix_image);
}

/***************************************************************
 * This function is used as a 1.6*3 microsecond (or longer) delay.
 * This is needed when trying to change any of the 'self-modifying
 * bits in the voice registers.
 ***************************************************************/
static void gf1_delay(void)
{
 int i;
 for (i=0;i<7;i++)
  inportb(gus_baseport+GF1_DRAM);       /* pick a port .... */
}

/* 2011-01-23 functions unused, so commented out.
static void UltraSetBalance(int voice,int data)
{
 outportb(gus_baseport+GF1_PAGE,voice);
 outportb(gus_baseport+GF1_REG_SELECT,SET_BALANCE);
 outportb(gus_baseport+GF1_DATA_HI,data&0x0F);
}
*/

/***************************************************************
 * This function returns the # of K found on the card
 ***************************************************************/
static int UltraSizeDram(void)
{
 ulong i;
 ulong loc;
 byte val;
 byte save0;
 byte save1;
 /* save first location ... */
 save0 = UltraPeekData(0L);
 /* See if there is first block there.... */
 UltraPokeData(0L,0xaa);
 val = UltraPeekData(0L);
 if (val != 0xaa)
  return(0);
 /* Now zero it out so that I can check for mirroring .. */
 UltraPokeData(0L,0x00);
 for (i=1L;i<1024L;i++)
 {
  /* check for mirroring ... */
  val = UltraPeekData(0L);
  if (val != 0)
   break;
  loc = i<<10;
  /* save location so its a non-destructive sizing */
  save1 = UltraPeekData(loc);
  UltraPokeData(loc,0xaa);
  val = UltraPeekData(loc);
  if (val != 0xaa)
   break;
  UltraPokeData(loc,save1);
 }
 /* Now restore location zero ... */
 UltraPokeData(0L,save0);
 return((int)i);
}

static int GUS_Init (void)
{
 if (!UltraGetCfg())
 {
#ifdef TEST
  printf ("ULTRASND environment variable not set\n");
#endif
  return 0;
 }
 if (Verbose)
  printf ("    Checking for a Gravis Ultrasound at %X... ",gus_baseport);
 if (!UltraOpen(GUS_NUMCHANNELS))
 {
  if (Verbose) puts ("Not found");
#ifdef FAKE_GUS
 return 1;
#endif
  return 0;
 }
 if (Verbose) puts ("OK");
 return 1;
}

static void GUS_Exit (void)
{
 UltraClose ();
}

static void GUS_UploadSample
        (ulong addr,const char *sample,ulong num_samples)
{
 ulong i;
#ifdef TEST
 printf ("Uploading %lu bytes at adress %lu\n",num_samples,addr);
#endif
 for (i=0;i<num_samples;++i)
  UltraPokeData (addr++,*sample++);
}

static void GUS_SetPitch (int voice,ulong freq)
{
 UltraSetFrequency (voice,freq);
}

static void GUS_VoiceOn
        (int voice,ulong loopstart,ulong loopend)
{
 /* Loop enable, 16-bits */
#ifdef TEST
 printf ("Voice %d: loopstart=%lu, loopend=%lu\n",voice,loopstart,loopend);
#endif
 UltraStartVoice (voice,loopstart,loopstart,loopend,0x0C);
}

/* 2011-01-23 functions unused, so commented out.
static void GUS_VoiceOff (int voice)
{
 UltraStopVoice (voice);
}
*/

static void GUS_SetVolume (int voice,unsigned volume)
{
 /* volume: 0x100=+6db */
 UltraSetVolume (voice,volume);
}

static ulong GUS_DetectDRAM (void)
{
 ulong retval;
 retval=(ulong)UltraSizeDram()*1024;
#ifdef FAKE_GUS
 retval=1024*1024;
#endif
 return retval;
}

#ifdef TEST_PROGRAM
#define LOOPSTART       100
#define LOOPEND         200

static ulong GetFreq (int n)
{
 /* note=C-n */
 ulong retval;
 int i;
 retval=44100;
 /* yuk! */
 for (i=0;i<n;++i)
  retval=retval*9449/10000;
 return retval;
}

static unsigned GetVol (int n)
{
 /* return -2n db */
 if (n==0)
  return 0;
 return 4096-n*256/3;
}

void main (void)
{
 ulong a=0;
 FILE *infile=NULL;
 char *samptr=NULL;
 ulong samlen=0;
 if (!GUS_Init())
 {
  printf ("GUS not detected\n");
  return;
 }
 printf ("GUS DRAM size:%lu bytes\n",GUS_DetectDRAM());
 infile=fopen ("GUSSAM","rb");
 if (infile)
 {
  samptr=malloc (512*1024);
  if (samptr)
   samlen=fread (samptr,1,512*1024,infile);
  fclose (infile);
 }
 if (samlen)
 {
  printf ("Uploading %lu bytes...\n",samlen);
  GUS_UploadSample (0,samptr,samlen);
  printf ("Setting volume to 0...\n");
  GUS_SetVolume (0,0);
  printf ("Calling GUS_VoiceOn()...\n");
  GUS_VoiceOn (0,LOOPSTART*2,LOOPEND*2);
  printf ("Calling GUS_SetPitch()...\n");
  GUS_SetPitch (0,GetFreq(0));
 }
 else
 {
  printf ("Error reading GUSSAM\n");
  return;
 }
 free (samptr);
 printf ("Use 1-0 for volume control\n");
 printf ("Use Z-/ for pitch control\n");
 printf ("Press ESC to quit\n");
 while ((a=getch())!=27)
 {
  switch (a)
  {
   case '0': case '1': case '2': case '3': case '4': case '5':
   case '6': case '7': case '8': case '9':
    a-='0';
    GUS_SetVolume (0,GetVol (a));
    break;
   case 'z':
    GUS_SetPitch (0,GetFreq(0));
    break;
   case 'x':
    GUS_SetPitch (0,GetFreq(1));
    break;
   case 'c':
    GUS_SetPitch (0,GetFreq(2));
    break;
   case 'v':
    GUS_SetPitch (0,GetFreq(3));
    break;
   case 'b':
    GUS_SetPitch (0,GetFreq(4));
    break;
   case 'n':
    GUS_SetPitch (0,GetFreq(5));
    break;
   case 'm':
    GUS_SetPitch (0,GetFreq(6));
    break;
   case ',':
    GUS_SetPitch (0,GetFreq(7));
    break;
   case '.':
    GUS_SetPitch (0,GetFreq(8));
    break;
   case '/':
    GUS_SetPitch (0,GetFreq(9));
    break;
  }
 }
 GUS_SetVolume (0,0);
 GUS_VoiceOff (0);
 GUS_Exit ();
}
#endif          /* TEST_PROGRAM */
