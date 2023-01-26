/** ADAMEm: Coleco ADAM emulator ********************************************/
/**                                                                        **/
/**                                 Adlib.c                                **/
/**                                                                        **/
/** Coleco sound hardware emulation via Adlib                              **/
/**                                                                        **/
/** Copyright (C) Marcel de Kogel 1996,1997,1998,1999                      **/
/**     You are not allowed to distribute this software commercially       **/
/**     Please, notify me, if you make any changes to this file            **/
/****************************************************************************/

#ifdef SOUND

#include "Coleco.h"

#include <pc.h>
#include <stdio.h>
#include <string.h>

static byte channel_on[4]= {1,1,1,1};
static byte kbf[4];                     /* KEY_BLOCK_F-NUMBER */
static byte noise_output_channel_3=0;
static word ChannelFreq[4];
static word NoiseFreqs[4]= {16,32,64,0};
static word FMPoort=0x388;
static byte sound_on=1;
static byte vol_is_nul[4]= {0,0,0,0};
static byte channel_volume[4];
static byte feedback=1;
static byte InstrMelodic[11],InstrPNoise[11],InstrWNoise[11];

static byte InstrMelodic[11]=
{
  0x21,0xA1,    /* AM_VIB_EG-TYP_KSR_MULTI */
  0x18,0x80,    /* KSL_TL                  */
  0xFF,0xFF,    /* AR_DR                   */
  0x08,0x08,    /* SL_RR                   */
  0x02,0x00,    /* WS                      */
  0x06          /* FB_FM                   */
};
static byte InstrPNoise[11]=
{
  0x20,0x20,
  0x48,0x01,
  0xFF,0xFF,
  0x0A,0x0A,
  0x03,0x02,
  0x00
};
static byte InstrWNoise[11]=
{
  0x2E,0x2E,    /* AM_VIB_EG-TYP_KSR_MULTI */
  0x40,0x00,    /* KSL_TL                  */
  0xFF,0xFF,    /* AR_DR                   */
  0x0A,0x0A,    /* SL_RR                   */
  0x00,0x03,    /* WS                      */
  0x0E          /* FB_FM                   */
};

static word FMFreqs[] =
{
 #include "FMFreqs.h"
};

static void WriteFM (byte value,byte index)
{
 int i;
 outportb (FMPoort,index);
 for (i=0;i<6;++i)
  inportb (FMPoort);
 outportb (FMPoort+1,value);
 for (i=0;i<35;++i)
  inportb (FMPoort);
}

static byte ReadFM (void)
{
 return inportb (FMPoort);
}

static void ChannelOff (byte channel)
{
 if (channel==3)
  WriteFM (kbf[channel],0xB6+feedback);
 else
  WriteFM (kbf[channel],0xB0+channel);
}

static void ChannelOn (byte channel)
{
 if (!sound_on)
  return;
 if (vol_is_nul[channel])
  return;
 if (channel==3)
  WriteFM (kbf[channel]|32,0xB6+feedback);
 else
  WriteFM (kbf[channel]|32,0xB0+channel);
}

static void ToggleChannel (int channel)
{
 if (!sound_on)
  return;
 channel_on[channel]^=1;
 if (channel_on[channel])
  ChannelOn (channel);
 else
  ChannelOff (channel);
}

static void SoundOff (void)
{
 int i;
 for (i=0;i<4;++i)
  ChannelOff (i);
}

static void SoundOn (void)
{
 int i;
 if (!sound_on)
  return;
 for (i=0;i<4;++i)
  if (channel_on[i])
   ChannelOn (i);
}

static void Toggle (void)
{
 sound_on^=1;
 if (sound_on)
  SoundOn ();
 else
  SoundOff ();
}

static void WriteVolume (byte vol,byte reg,byte index)
{
 unsigned tl;
 if (vol>=15 || mastervolume==0)
  tl=63;
 else
 {
  tl=(reg&63)+vol*267/100;
  tl+=(15-mastervolume)*2;
  if (tl>63)
   tl=63;
 }
 WriteFM ((reg&0xC0)|tl,index);
}

static void SetVolume (byte channel,byte vol)
{
 channel_volume[channel]=vol;
 if (vol==15)
 {
  vol_is_nul[channel]=1;
  if (channel_on[channel])
   ChannelOff (channel);
  return;
 }
 if (vol_is_nul[channel])
 {
  vol_is_nul[channel]=0;
  if (channel_on[channel])
   ChannelOn (channel);
 }
 if (channel==3)
 {
  if (feedback)
  {
   if (InstrWNoise[10]&1)  /* additive synthesis */
    WriteVolume (vol,InstrWNoise[2],0x51);
   WriteVolume (vol,InstrWNoise[3],0x54);
  }
  else
  {
   if (InstrPNoise[10]&1)  /* additive synthesis */
    WriteVolume (vol,InstrPNoise[2],0x50);
   WriteVolume (vol,InstrPNoise[3],0x53);
  }
 }
 else
 {
  if (InstrMelodic[10]&1)  /* additive synthesis */
   WriteVolume (vol,InstrMelodic[2],0x40+channel);
  WriteVolume (vol,InstrMelodic[3],0x43+channel);
 }
}

static void SetFreq (byte channel, word freq)
{
 int block,f;
 if (!freq) freq=1024;
 ChannelFreq[channel]=freq;
 if (channel==3 && feedback) freq=930;
 f=FMFreqs[freq*2+1];
 block=FMFreqs[freq*2];
 if (channel==3 && !feedback)
 {
  block-=3;
  if (block<0)
  {
   block=0;
   f=0;
  }
 }
 if (f==0 && channel==3 && freq!=0)
 {
  f=1023;
  block=7;
 }
 if (channel==3)
  WriteFM (f&0xFF,0xA6+feedback);
 else
 {
  WriteFM (f&0xFF,0xA0+channel);
 }
 kbf[channel]=(f>>8)+(block<<2);
 if (channel_on[channel])
 {
  if (f)
   ChannelOn (channel);
  else
   ChannelOff (channel);
 }
 if (channel==2 && noise_output_channel_3)
  SetFreq (3,freq);
}

static void SetVoices (void)
{
 int i;
 memcpy (InstrMelodic,InstrMelodic,11);
 memcpy (InstrPNoise,InstrPNoise,11);
 memcpy (InstrWNoise,InstrWNoise,11);
 for (i=0;i<3;++i)
 {
  WriteFM (InstrMelodic[0],0x20+i);
  WriteFM (InstrMelodic[1],0x23+i);
  WriteFM (InstrMelodic[2],0x40+i);
  WriteFM (InstrMelodic[3],0x43+i);
  WriteFM (InstrMelodic[4],0x60+i);
  WriteFM (InstrMelodic[5],0x63+i);
  WriteFM (InstrMelodic[6],0x80+i);
  WriteFM (InstrMelodic[7],0x83+i);
  WriteFM (InstrMelodic[8],0xE0+i);
  WriteFM (InstrMelodic[9],0xE3+i);
  WriteFM (InstrMelodic[10],0xC0+i);
  SetVolume (i,0);        /* 0=Max, 0x0F=Min */
 }
 WriteFM (InstrPNoise[0],0x30);
 WriteFM (InstrPNoise[1],0x33);
 WriteFM (InstrPNoise[2],0x50);
 WriteFM (InstrPNoise[3],0x53);
 WriteFM (InstrPNoise[4],0x70);
 WriteFM (InstrPNoise[5],0x73);
 WriteFM (InstrPNoise[6],0x90);
 WriteFM (InstrPNoise[7],0x93);
 WriteFM (InstrPNoise[8],0xF0);
 WriteFM (InstrPNoise[9],0xF3);
 WriteFM (InstrPNoise[10],0xC6);
 WriteFM (InstrWNoise[0],0x31);
 WriteFM (InstrWNoise[1],0x34);
 WriteFM (InstrWNoise[2],0x51);
 WriteFM (InstrWNoise[3],0x54);
 WriteFM (InstrWNoise[4],0x71);
 WriteFM (InstrWNoise[5],0x74);
 WriteFM (InstrWNoise[6],0x91);
 WriteFM (InstrWNoise[7],0x94);
 WriteFM (InstrWNoise[8],0xF1);
 WriteFM (InstrWNoise[9],0xF4);
 WriteFM (InstrWNoise[10],0xC7);
 SetVolume (3,0);
}

static int Reset_Adlib (void)
{
 byte tmp1,tmp2;
 int i;
 WriteFM (0,1);
 WriteFM (0x60,4);
 WriteFM (0x80,4);
 tmp1=ReadFM ();
 WriteFM (0xFF,2);
 WriteFM (0x21,4);
 for (i=0;i<300;++i)
  ReadFM ();
 tmp2=ReadFM ();
 WriteFM (0x60,4);
 WriteFM (0x80,4);
 if ((tmp1&0xE0)!=0)
  return 0;
 if ((tmp2&0xE0)!=0xC0)
  return 0;
 WriteFM (0,1);
 return 1;
}

static int Init (void)
{
 int i;
 if (Verbose)
  printf ("    Checking for an Adlib compatible at %X... ",FMPoort);
 if (!Reset_Adlib())
 {
  if (Verbose) puts ("Not found");
  return 0;
 }
 if (Verbose) printf ("found\n");
 for (i=0xB0;i<0xB9;++i)
  WriteFM (0,i);
 WriteFM (0,8);
 WriteFM (0x00,0xBD);
 SetVoices ();
 return 1;
}

static void Reset (void)
{
 int i;
 WriteFM (0,0xB0);
 WriteFM (0,0xB1);
 WriteFM (0,0xB2);
 WriteFM (0,0xB3);
 WriteFM (0,0xB4);
 WriteFM (0,0xB5);
 WriteFM (0,0xB6);
 Reset_Adlib ();
 for (i=0xB0;i<0xB9;++i)
  WriteFM (0,i);
 WriteFM (0,8);
}

static void NoiseFeedback_On (void)
{
 if (!feedback)
 {
  ChannelOff (3);
  feedback=1;
  SetFreq (3,ChannelFreq[3]);
 }
}

static void NoiseFeedback_Off (void)
{
 if (feedback)
 {
  ChannelOff (3);
  feedback=0;
  SetFreq (3,ChannelFreq[3]);
 }
}

static void NoiseControl (word v)
{
 if (v & 4)
  NoiseFeedback_On ();
 else
  NoiseFeedback_Off ();
 if ((v&3)==3)
 {
  if (noise_output_channel_3==0)
  {
   noise_output_channel_3=1;
   SetFreq (3,ChannelFreq[2]);
  }
 }
 else
 {
  noise_output_channel_3=0;
  SetFreq (3,NoiseFreqs[v&3]);
 }
}

static void WriteSoundReg (int r,int v)
{
 switch (r)
 {
  case 0:
   SetFreq (0,v);
   break;
  case 1:
   SetVolume (0,v);
   break;
  case 2:
   SetFreq (1,v);
   break;
  case 3:
   SetVolume (1,v);
   break;
  case 4:
   SetFreq (2,v);
   break;
  case 5:
   SetVolume (2,v);
   break;
  case 6:
   NoiseControl (v);
   break;
  case 7:
   SetVolume (3,v);
 }
}

static void SetMasterVolume (int newvolume)
{
 int i;
 for (i=0;i<4;++i)
  SetVolume (i,channel_volume[i]);
}

static void Stop (void)
{
 SoundOn ();
}

static void Resume (void)
{
 SoundOff ();
}

struct SoundDriver_struct Adlib_SoundDriver=
{
 Init,
 Reset,
 WriteSoundReg,
 Toggle,
 ToggleChannel,
 SetMasterVolume,
 Stop,
 Resume
};

#endif

