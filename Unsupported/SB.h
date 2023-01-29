/****************************************************************************/
/**                                                                        **/
/**                                   SB.h                                 **/
/**                                                                        **/
/** Software interface for MS-DOS/SoundBlaster                             **/
/**                                                                        **/
/** Copyright (C) Marcel de Kogel 1996,1997,1998                           **/
/**     You are not allowed to distribute this software commercially       **/
/**     Please, notify me, if you make any changes to this file            **/
/****************************************************************************/

#include "DMA.h"
#include "INT.h"
#include "Asm.h"

#include <conio.h>
#include <stdlib.h>
#include <go32.h>
#include <dpmi.h>
#include <string.h>
#include <stdio.h>
#include <dos.h>
#include <pc.h>
#include <sys/farptr.h>

static void Interrupt_Routine (void);   /* In Sound.c */

static int got_sb=0;
static int sb16=0;
static int sbpro=0;
static unsigned sb_dsp_version;
static word sb_port;
static byte sb_dma_channel;
static byte sb_irq;
static byte sb_irq_int;
static unsigned sb_buf[2];
static unsigned sb_phys;
static unsigned sb_sel;
static void *buffer;
static byte sb_default_pic1,sb_default_pic2;
static int sb_bufnum;
static _go32_dpmi_seginfo sbirq;
static int highspeed=0;
static int dma_size=0;
static int playbackrate;
static int sb_stereo=0;

static int sb_read_dsp()
{
 int x;
 for (x=0; x<0xfffff; x++)
  if (inportb(0x0E + sb_port) & 0x80)
   return inportb(0x0A+sb_port);
 return -1; 
}

static int sb_write_dsp(byte val)
{
 int x;
 for (x=0; x<0xfffff; x++)
 {
  if (!(inportb(0x0C+sb_port) & 0x80))
  {
   outportb(0x0C+sb_port, val);
   return 1;
  }
 }
 return 0;
}

static void sb_speaker_on (void)
{
 sb_write_dsp(0xD1);
}

static void sb_speaker_off (void)
{
 sb_write_dsp(0xD3);
}

static unsigned sb_set_sample_rate(unsigned rate)
{
 unsigned rateval,realfreq;
 if (!sb16 && sb_stereo)
 {
  rate*=2;
  outportb (sb_port+4,0x0E);
  outportb (sb_port+5,inportb(sb_port+5)|2);
 }
 rateval=256-1000000/rate;
 realfreq=1000000/(256-rateval);
 if (sb16)
 {
  sb_write_dsp (0x41);
  sb_write_dsp ((byte)(realfreq>>8));
  sb_write_dsp ((byte)(realfreq&0xFF));
 }
 else
 {
  sb_write_dsp(0x40);
  sb_write_dsp((byte)rateval);
 }
 highspeed=(rateval>211);
 if (!sb16 && sb_stereo)
  return realfreq/2;
 else
  return realfreq;
}

static int sb_reset_dsp()
{
 int i,j;
 for (i=0;i<16;++i)
 {
  outportb(0x06+sb_port, 1);
  for (j=0;j<10;++j) inportb (0x06+sb_port);
  outportb(0x06+sb_port, 0);
  if (sb_read_dsp() == 0xAA)
   return 1;
 }
 return 0;
}

static unsigned sb_read_dsp_version()
{
 unsigned hi, lo;
 sb_write_dsp(0xE1);
 hi = sb_read_dsp();
 lo = sb_read_dsp();
 return ((hi << 8) + lo);
}

static void sb_play_buffer(int size)
{
 if (sb16)
 {
  sb_write_dsp(0xB6);
  sb_write_dsp((sb_stereo)? 0x30:0x10);
  sb_write_dsp((size/2-1) & 0xff);
  sb_write_dsp((size/2-1) >> 8);
 }
 else
 {
  sb_write_dsp(0x48);
  sb_write_dsp((size-1) & 0xff);
  sb_write_dsp((size-1) >> 8);
  if (highspeed)
   sb_write_dsp(0x90);
  else
   sb_write_dsp(0x1C);
 }
}

static int sb_interrupt(void)
{
 static int int_busy=0;
 static int int_count=16;
 int i;
 unsigned offset;
 if (sb16)
  inportb(sb_port+0x0F);
 else
  inportb(sb_port+0x0E);
 nofunc();
 outportb(0x20, 0x20);
 nofunc();
 if (sb_irq>=8)
  outportb(0xA0, 0x20);
 if (!int_busy)
 {
  int_busy=1;
  __enable();
  if (!--int_count)
  {
   int tmp=_dma_todo (sb_dma_channel);
   if (sb16)
    sb_bufnum=(tmp<=(dma_size*2))? 0:1;
   else
    sb_bufnum=(tmp<=dma_size)? 0:1;
   int_count=16;
  }
  sound_update(buffer);
  _farsetsel (sb_sel);
  offset=sb_buf[sb_bufnum];
  if (sb16)
  {
   short *addr=(short*)buffer;
   for (i=0;i<num_samples;++i)
    _farnspokew (offset+i*2,*addr++);
  }
  else
  {
   char *addr=(char*)buffer;
   for (i=0;i<num_samples;++i)
    _farnspokeb (offset++,*addr++);
  }
  sb_bufnum=(sb_bufnum+1)&1;
  Interrupt_Routine ();
  int_busy=0;
 }
 else
 {
  sb_bufnum=(sb_bufnum+1)&1;
  __enable();
 }
 return 0;
}

static void sb_install_interrupts()
{
 sbirq.pm_offset = (unsigned long)sb_interrupt;
 sbirq.pm_selector = _my_cs();
 SetInt (sb_irq_int, &sbirq);
}

/****************************************************************************/
/** Initialisation Routines                                                **/
/****************************************************************************/

static int sb_detect()
{
 sb_port=SB_Info.baseport;
 if (sb_port==0)
  return 0;
 if (!sb_reset_dsp())
  return 0;
 sb_dsp_version=sb_read_dsp_version();
 if (sb_dsp_version<0x200)
  return 0;
 if (sb_dsp_version>=0x300)
  sbpro=1;
 if (sb_dsp_version>=0x400)
  sb16=1;
 sb_dma_channel=(sb16)? SB_Info.dma_high:SB_Info.dma_low;
 sb_irq=SB_Info.irq;
 sb_irq_int=(sb_irq<8)? sb_irq+8 : sb_irq+0x70-8;
 got_sb=1;
 return 1;
}

static int sound_hw_init(int *numbits,int *numsamples,
                         int *soundfreq,int *intfreq,int *swap,int *stereo)
{
 int dma_len,i;
 if (!got_sb)
 {
  if (Verbose)
   printf ("    Checking for a SoundBlaster at %X... ",SB_Info.baseport);
  if (!sb_detect())
  {
   if (Verbose) puts ("Not found");
   return 0;
  }
  if (Verbose)
   printf ("Sound Blaster %s found\n",(sb16)? "16":((sbpro)? "Pro":"2.0"));
 }
 if (*stereo) {
  if (!sb16 && !sbpro) *stereo=0;
  else if (!sb16 && *soundfreq>22500) *soundfreq=22500;
 }
 sb_stereo=*stereo;
 *swap=0;
 if (*soundfreq<4000) *soundfreq=4000;
 if (*soundfreq>44100) *soundfreq=44100;
 if (*intfreq<5) *intfreq=5;
 if (*intfreq>500) *intfreq=500;
 *soundfreq=sb_set_sample_rate(*soundfreq);
 dma_size=(*soundfreq)/(*intfreq);
 if (dma_size&3)
  dma_size=(dma_size&(~3))+4;
 if (dma_size>NUM_SAMPLES_MAX)
  dma_size=NUM_SAMPLES_MAX;
 *numsamples=dma_size;
 if (sb_stereo) dma_size*=2;
 *numbits=(sb16)? 16:8;
 *intfreq=(*soundfreq)/dma_size;
 dma_len=(sb16)? dma_size*4:dma_size*2;
 if (Verbose) printf ("    Allocating buffers... ");
 buffer=malloc (dma_len/2);
 if (!buffer)
 {
  if (Verbose) puts ("Failed");
  return 0;
 }
 memset (buffer,0,dma_len/2);
 if (_dma_allocate_mem(dma_len, &sb_sel, &sb_buf[0], &sb_phys) == 0)
 {
  if (Verbose) puts ("Failed");
  free (buffer);
  return 0;
 }
 if (Verbose) puts ("OK");
 sb_default_pic1 = inportb(0x21);
 if (sb_irq > 7)
 {
  nofunc();
  outportb(0x21, sb_default_pic1 & 0xFB); 
  nofunc();
  sb_default_pic2 = inportb(0xA1);
  outportb(0xA1, sb_default_pic2 & (~(1<<(sb_irq-8))));
 }
 else
 {
  nofunc();
  outportb(0x21, sb_default_pic1 & (~(1<<sb_irq)));
 }
 sb_install_interrupts();
 sb_speaker_on();
 sb_buf[1] = sb_buf[0] + dma_len/2;
 _farsetsel (sb_sel);
 if (sb16)
  for (i=0;i<(*numsamples)*4;++i)
   _farnspokeb (sb_buf[0]+i,0);
 else
  for (i=0;i<(*numsamples)*2;++i)
   _farnspokeb (sb_buf[0]+i,0x80);
 sb_bufnum = 0;
 _dma_start(sb_dma_channel, sb_phys, dma_len, 1);
 sb_play_buffer(dma_len/2);
 return 1;
}

static void sound_hw_reset (void)
{
 if (!got_sb)
  return;
 if (!sb16 && sb_stereo)
 {
  outportb (sb_port+4,0x0E);
  outportb (sb_port+5,inportb(sb_port+5)&0xFD);
 }
 sb_speaker_off ();
 sb_write_dsp (0xD0);
 _dma_stop(sb_dma_channel);
 sb_reset_dsp();
 ResetInt (sb_irq_int, &sbirq);
 outportb(0x21, sb_default_pic1);
 nofunc();
 if (sb_irq>7)
  outportb(0xA1, sb_default_pic2);
 _dma_free_mem(sb_sel);
 free (buffer);
}

