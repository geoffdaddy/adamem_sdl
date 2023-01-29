/** ADAMEm: Coleco ADAM emulator ********************************************/
/**                                                                        **/
/**                                 AWEFn.h                                **/
/**                                                                        **/
/** General SB-AWE32 interface functions                                   **/
/** Based on `The Unofficial Sound Blaster AWE32 Programming Guide'        **/
/** written by Vince Vu a.k.a. Judge Dredd                                 **/
/**                                                                        **/
/** Copyright (C) Marcel de Kogel 1996,1997,1998,1999                      **/
/**     You are not allowed to distribute this software commercially       **/
/**     Please, notify me, if you make any changes to this file            **/
/****************************************************************************/

#include <stdio.h>
#include <pc.h>
#include <stdlib.h>
#include <conio.h>

typedef int int32;
typedef short int16;
typedef unsigned uint32;
typedef unsigned short uint16;

static int16 AWE_Port[6]= {0x620,0x622,0xA20,0xA22,0xE20,0xE22};
static int base_port=0x620;
#define AWE_IndexPort   AWE_Port[5]
#define AWE_RAMSTART    0x200000

static void AWE_SetBasePort (int port)
{
 int i;
 for (i=0;i<6;++i)
  AWE_Port[i]=AWE_Port[i]-base_port+port;
 base_port=port;
}

static void AWE_Delay (void)
{
 inportw (AWE_IndexPort);
}

static void AWE_outportw (int port,int index,uint16 data)
{
 outportw (AWE_IndexPort,index);
 AWE_Delay ();
 outportw (AWE_Port[port],data);
}

static void AWE_outportl (int port,int index,uint32 data)
{
 outportw (AWE_IndexPort,index);
 AWE_Delay ();
 outportw (AWE_Port[port],data);
 AWE_Delay ();
 outportw (AWE_Port[port+1],data>>16);
}

static uint16 AWE_inportw (int port,int index)
{
 outportw (AWE_IndexPort,index);
 AWE_Delay ();
 return inportw (AWE_Port[port]);
}

static uint32 AWE_inportl (int port,int index)
{
 uint32 tmp1,tmp2;
 outportw (AWE_IndexPort,index);
 AWE_Delay ();
 tmp1=inportw (AWE_Port[port]);
 AWE_Delay ();
 tmp2=inportw (AWE_Port[port+1]);
 return (tmp1&0xFFFF)|(tmp2<<16);
}

static int AWE_Detect(void)
{
 if ((AWE_inportw(4,0xE0)&0x0C)!=0x0C)
  return 0;
 if ((AWE_inportw(2,0x3D)&0x58)!=0x58)
  return 0;
 if ((AWE_inportw(2,0x3E)&0x03)!=0x03)
  return 0;
 return 1;
}

static unsigned AWE_DoDetectDRAM (void)
{
 unsigned dramsize=0;
 AWE_outportl (2,32+22,AWE_RAMSTART);   /* Set write offset */
 outportw (AWE_IndexPort,32+26);        /* Read/Write data reg */
 outportw (AWE_Port[2],0xFFFF);         /* Write some stuff */
 outportw (AWE_Port[2],0xAAAA);
 outportw (AWE_Port[2],0x5555);
 AWE_outportl (2,32+20,AWE_RAMSTART);   /* Set read offset */
 AWE_inportw (2,32+26);                 /* Ignore first read */
 if (inportw(AWE_Port[2])!=0xFFFF)      /* Check if written ok */
  return 0;
 if (inportw(AWE_Port[2])!=0xAAAA)
  return 0;
 if (inportw(AWE_Port[2])!=0x5555)
  return 0;
 while(dramsize<0xdf8000)
 {
  dramsize += 0x20000;            /*Everything checks out*/
                                  /*so add 0x20000 to dramsize*/
  AWE_outportl(2,32+22,dramsize+AWE_RAMSTART);
  outportw (AWE_IndexPort,32+26);
  outportw (AWE_Port[2],0x1234);
  outportw (AWE_Port[2],0x1234);
  outportw (AWE_Port[2],0x1234);
  AWE_outportl (2,32+20,AWE_RAMSTART);
  AWE_inportw (2,32+26);
  if (inportw(AWE_Port[2])!=0xFFFF)
   return dramsize;                     /* Adress wrapped around */
  AWE_outportl (2,32+20,dramsize+AWE_RAMSTART);      
  AWE_inportw (2,32+26);
  if (inportw(AWE_Port[2])!=0x1234)
   return dramsize;
  if (inportw(AWE_Port[2])!=0x1234)
   return dramsize;
  if (inportw(AWE_Port[2])!=0x1234)
   return dramsize;
 }
 return dramsize;
}

static unsigned AWE_DetectDRAM (void)
{
 unsigned dramsize;
 unsigned i;
 /* Setup 15 Oscillators for DRAM writing, and 15 for read */
 for(i=0; i<30; i++)
 {
  AWE_outportw (2,160+i,0x80);          /* Disable EG2 */
  AWE_outportl (0,96+i,0);              /* Set RawFilter/Raw Volume */
  AWE_outportl (0,64+i,0);              /* Purpose unknown */
  AWE_outportl (0,192+i,0);             /* Set pan & loopstart */
  AWE_outportl (0,224+i,0);             /* Set chorus & loopend */
  AWE_outportl (0,32+i,0x40000000);     /* Set frequency to ? */
  AWE_outportl (0,0+i,0x40000000);      /* Purpose unknown */
  AWE_outportl (2,0+i,(i&1)? 0x06000000:0x04000000);    /* Set mode */
 }
 dramsize=AWE_DoDetectDRAM ();
 /* Reset the oscillators */
 for(i=0; i<30; i++)
 {
  AWE_outportl (2,0+i,0);
  AWE_outportw (2,160+i,0x807F);
 }
 return dramsize;
}

/* i is the oscillator number (0-31) */
static void AWE_SetOscillator (int channel, int mode)
{
 AWE_outportw (2,160+channel,0x80);
 AWE_outportl (0,96+channel,0);         /* Set RawFilter/Raw Volume to 0 */
 AWE_outportl (0,64+channel,0);         /* Purpose unknown */
 AWE_outportl (0,192+channel,0);        /* Set pan & loopstart */
 AWE_outportl (0,224+channel,0);        /* Set chorus & loopend */
 AWE_outportl (0,32+channel,0x40000000);/* Set frequency to ??? */
 AWE_outportl (0,0+channel,0x40000000); /* Purpose unknown */
 AWE_outportl (2,0+channel,(mode)? 0x06000000:0x04000000);
}

/* Samples are 16-bit intel,signed,PCM */
static void AWE_UploadSample
        (unsigned write_offset,const short *sample,unsigned no_samples)
{
 int i;
 AWE_outportl (2,32+22,write_offset);
 outportw (AWE_IndexPort,32+26);
 for(i=0; i<no_samples; i++)
  outportw(AWE_Port[2],sample[i]);
}

/* 2011-01-23 functions unused, so commented out.
static void AWE_DownloadSample
        (unsigned read_offset,short *sample,unsigned no_samples)
{
 unsigned i;
 AWE_outportl (2,32+22,read_offset);
 AWE_inportw (2,32+26);
 for(i=0; i<no_samples; i++)
  sample[i]=inportw(AWE_Port[2]);
}

static void AWE_EnableDRAM(void)
{
 int k;
 unsigned Scratch;
 AWE_outportw(3,0x3E,0x20);
 for(k=0;k<30;k++)
 {
  AWE_outportw(3,0xA0+k,0x80);
  AWE_outportl(0,0x60+k,0);
  AWE_outportl(0,0x40+k,0);
  AWE_outportl(0,0xC0+k,0);
  AWE_outportl(0,0xE0+k,0);
  AWE_outportl(0,0x20+k,0x40000000);
  AWE_outportl(0,0+k,0x40000000);
  Scratch = (((k&1)<<9)+0x400);
  Scratch = Scratch <<16;
  AWE_outportl(2,0+k,Scratch);
 }
}
    
static void AWE_DisableDRAM(void)
{
 int k;
 for(k=0;k<30;k++)
 {
  AWE_outportw(2,k,0);
  AWE_outportw(2,0xA0+k,0x807F);
 }
}
*/

static void AWE_SetFilterQPlayPosition (int channel,int FilterQ,int PlayPos)
{
 /* DRAMControl=Normal */
 AWE_outportl (2,channel,(FilterQ<<24)+PlayPos);
}

static void AWE_SetInitialPitch (int channel,int pitch)
{
 if (pitch<0)
  pitch=0;
 if (pitch>65535)
  pitch=65535;
 AWE_outportw (4,channel,pitch);
}

static void AWE_SetReverbSend (int channel,int reverb)
{
 /* hiword: constantly updated by the envelope engine */
 /* lobyte(loword): auxilliary data */
 int c=AWE_inportl (0,channel+32);
 AWE_outportl (0,channel+32,((reverb&0xFF)<<8) | (c&0xFFFF00FF));
}

/* 44100Hz counter control */
/* 2011-01-23 functions unused, so commented out.
static void AWE_SetCounter (unsigned value)
{
 AWE_outportw (3,0x3B,value);
}

static unsigned AWE_ReadCounter (void)
{
 return AWE_inportw (3,0x3B);
}
*/

static void AWE_SetInitialFilterInitialVolume
        (int channel,int filter,int volume)
{
 /* volume=0: silent, volume=255:+96db */
 /* Initial filter cutoff=0-100Hz, 255-8059Hz */
 AWE_outportw (4,channel+32,(filter<<8)+(volume&0xFF));
}

static void AWE_SetEG1PitchFilterCutoff (int channel,int pitch,int cutoff)
{
 AWE_outportw (4,64+channel,(pitch<<8)+(cutoff&0xFF));
}

static void AWE_SetOverallVolumeOverallFilterCutoff
        (int channel,int volume,int cutoff)
{
 AWE_outportl (0,96+channel,(volume<<16)+(cutoff&0xFFFF));
}

static void AWE_SetLFO1PitchFilterCutoff (int channel,int pitch,int cutoff)
{
 AWE_outportw (4,96+channel,(pitch<<8)+(cutoff&0xFF));
}

static void AWE_SetEG2DelayTime (int channel,int delay)
{
 /* delay=32768-(ms*1000/725) */
 AWE_outportw (2,128+channel,delay);
}

static void AWE_SetEG2HoldAttack (int channel,int hold,int attack)
{
 /* hold=127-ms/96
    attack= if >=360ms: 11878/ms-1
            if <360 and !0: 32+(16/log(1/2))*log(360/ms)
            if 0: 0x7F */
 AWE_outportw (3,128+channel,((hold&0x7F)<<8)+(attack&0x7F));
}

static void AWE_SetLFO1VolumeFrequency (int channel,int volume,int freq)
{
 /* volume=db*12/128
    freq=hz*21.44/256 */
 AWE_outportw (4,128+channel,(volume<<8)+(freq&0xFF));
}

static void AWE_SetEG2SustainDecay (int channel,int sustain,int decay)
{
 /* sustain=db*4/3
    decay=2*log(1/2)*log(23756/ms) */
 AWE_outportw (2,160+channel,((sustain&0xFF)<<8)+(decay&0x7F));
}

static void AWE_SetLFO1DelayTime (int channel,int delay)
{
 /* delay=32768-(ms*1000/725) */
 AWE_outportw (3,160+channel,delay);
}

static void AWE_SetLFO2PitchFreq (int channel,int pitch,int freq)
{
 AWE_outportw (4,160+channel,(pitch<<8)+(freq&0xFF));
}

static void AWE_SetPanLoopStart (int channel,int pan,int loopstart)
{
 AWE_outportl (0,192+channel,(pan<<24)+loopstart);
}

static void AWE_SetEG1DelayTime (int channel,int delay)
{
 /* delay=32768-(ms*1000/725) */
 AWE_outportw (2,192+channel,delay);
}

static void AWE_SetEG1HoldAttack (int channel,int hold,int attack)
{
 /* hold=127-ms/96
    attack= if >=360ms: 11878/ms-1
            if <360 and !0: 32+(16/log(1/2))*log(360/ms)
            if 0: 0x7F */
 AWE_outportw (3,192+channel,((hold&0x7F)<<8)+(attack<<8));
}

static void AWE_SetChorusLoopEnd (int channel,int chorus,int loopend)
{
 AWE_outportl (0,224+channel,(chorus<<24)+loopend);
}

static void AWE_SetEG1SustainDecay (int channel,int sustain,int decay)
{
 /* sustain=db*4/3
    decay=2*log(1/2)*log(23756/ms) */
 AWE_outportw (2,224+channel,((sustain&0x7F)<<8)+(decay&0x7F));
}

static void AWE_SetLFO2DelayTime (int channel,int delay)
{
 /* delay=32768-(ms*1000/725) */
 AWE_outportw (3,224+channel,delay);
}

static void AWE_ResetOscillator (int channel)
{
 AWE_outportl (2,channel,0);
 AWE_outportw (2,160+channel,0x807F);
}

