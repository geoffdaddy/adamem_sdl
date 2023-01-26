/** ADAMEm: Coleco ADAM emulator ********************************************/
/**                                                                        **/
/**                                Speaker.c                               **/
/**                                                                        **/
/** Coleco sound hardware emulation via PC Speaker                         **/
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

#define MIN_VOL         0
#define NOISE_CH        3
#define MAXFREQ     10000

#define MAX_CHANNEL     4

static int playing=0;
static int invalidfreq[4];
static byte channel_on[4]= {1,1,1,1};
static unsigned channelfreq[4];
static unsigned channelvolume[4];
static unsigned ChannelFreq[4];
static byte sound_on=1;
int spk_channels[4]={4,3,2,1};
static int feedback=1;
static byte noise_output_channel_3=0;
static word NoiseFreqs[4]= {16,32,64,0};

static int IsNoiseChannel (int channel)
{
 return (channel==3);
}

static void UpdateSound (void)
{
 static int real_freq=0;
 static int totalvolume;
 static int totalfreq;
 int c,i;
 int new_freq;
 new_freq=0;
 if (sound_on)
 {
  totalvolume=totalfreq=0;
  for (i=0;i<4;++i)
  {
   c=spk_channels[i];
   if (c && !invalidfreq[c-1] && channelvolume[c-1]>MIN_VOL &&
       channel_on[c-1])
   {
/*
     totalvolume+=channelvolume[c-1];
     totalfreq+=channelfreq[c-1]*channelvolume[c-1];
*/
    totalfreq=channelfreq[c-1];
    totalvolume=1;
    break;
   }
  }
  if (totalvolume)
  {
   i=totalfreq/totalvolume;
   if (i)
    new_freq=1192380/i;
  }
 }
 if (new_freq!=real_freq)
 {
  if (new_freq)
  {
   playing=1;
   outportb (0x43,0xB6);
   outportb (0x42,new_freq&0xFF);
   outportb (0x42,new_freq>>8);
   if (!real_freq)
    outportb (0x61,inportb(0x61)|3);
  }
  else
  {
   playing=0;
   outportb (0x61,inportb(0x61)&0xFC);
  }
  real_freq=new_freq;
 }
}

static void Toggle (void)
{
 sound_on^=1;
 UpdateSound();
}

static void ToggleChannel (int channel)
{
 channel_on[channel]^=1;
 UpdateSound();
}

static void SetVolume (byte channel,int vol)
{
 channelvolume[channel]=(15-vol)*2;
 UpdateSound();
}

static void SetFreq (byte channel, word freq)
{
 int realfreq;
 if (!freq) freq=1024;
 ChannelFreq[channel]=freq;
 if (!freq)
  freq=1;
 realfreq=3579545/(32*freq);
 if (realfreq>MAXFREQ)
  invalidfreq[channel]=1;
 else
  invalidfreq[channel]=0;
 if (IsNoiseChannel(channel))
  realfreq/=4;
 channelfreq[channel]=realfreq;
 UpdateSound();
 if (channel==2 && noise_output_channel_3)
  SetFreq (3,freq);
}

static int Init (void)
{
 if (Verbose) puts ("    Using PC Speaker driver");
 return 1;
}

static void Reset (void)
{
 outportb (0x61,inportb(0x61)&0xFC);
}

static void SetMasterVolume (int newvolume)
{
}

static void NoiseFeedback_On (void)
{
 if (feedback==1)
  return;
 feedback=1;
}
static void NoiseFeedback_Off (void)
{
 if (feedback==0)
  return;
 feedback=0;
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

static void Stop (void)
{
 outportb (0x61,inportb(0x61)&0xFC);
}

static void Resume (void)
{
 UpdateSound();
}

struct SoundDriver_struct Speaker_SoundDriver=
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

