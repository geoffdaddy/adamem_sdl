/** ADAMEm: Coleco ADAM emulator ********************************************/
/**                                                                        **/
/**                                  DSP.c                                 **/
/**                                                                        **/
/** Coleco sound hardware emulation via a CODEC                            **/
/**                                                                        **/
/** Copyright (C) Marcel de Kogel 1996,1997,1998,1999                      **/
/**     You are not allowed to distribute this software commercially       **/
/**     Please, notify me, if you make any changes to this file            **/
/****************************************************************************/

#ifdef SOUND

#include "Coleco.h"

#include <stdio.h>
#include <string.h>
#include <values.h>

static int initialised=0;
static int stereo=0;

/* General mixer routines */
#include "Mixer.h"
/* Hardware dependent code */
#ifdef MSDOS
 #include "SB.h"                   /* MS-DOS / SoundBlaster                 */
#else
 #include "dev_dsp.h"              /* UNIX USS / dev/dsp                    */
#endif

#define NOISECH1        15         /* Oscillator used for "white" noise     */
#define NOISECH2        20         /* Oscillator used for "periodic" noise  */
#define MAXFREQ         1400000    /* In cHertz, freq. divider=8            */

static int basefreqtable[25];
static int maxfreqtable[25];

int soundquality=3;
static int invalidfreq[4];
static int channels[4]={0,5,10,NOISECH1};
static int feedback=1;
static byte channel_on[4]= {1,1,1,1};
static byte noise_output_channel_3=0;
static unsigned channelfreq[4];
static unsigned channelvolume[4];
static unsigned ChannelFreq[4];
static unsigned NoiseFreqs[4]= {16,32,64,0};
static byte sound_on=1;

static int GetMusChannel (int freq)
{
 if (freq<(maxfreqtable[0]*100))
  return 0;
 if (freq<(maxfreqtable[1]*100))
  return 1;
 if (freq<(maxfreqtable[2]*100)) 
  return 2;
 if (freq<(maxfreqtable[3]*100))
  return 3;
 return 4;
}

static int GetPeriodicNoiseChannel (int freq)
{
 if (freq<(maxfreqtable[NOISECH2]*100))
  return 0;
 if (freq<(maxfreqtable[NOISECH2+1]*100))
  return 1;
 if (freq<(maxfreqtable[NOISECH2+2]*100))
  return 2;
 if (freq<(maxfreqtable[NOISECH2+3]*100))
  return 3;
 return 4;
}

static int GetWhiteNoiseChannel (int freq)
{
 if (freq<(maxfreqtable[NOISECH1]*100))
  return 0;
 if (freq<(maxfreqtable[NOISECH1+1]*100))
  return 1;
 if (freq<(maxfreqtable[NOISECH1+2]*100))
  return 2;
 if (freq<(maxfreqtable[NOISECH1+3]*100))
  return 3;
 return 4;
}

static void WriteVolume (int channel,int volume)
{
 if (!sound_on ||
     !channel_on[channel] || invalidfreq[channel])
  volume=0;
 volume=volume*mastervolume/15;
 setvolume (channels[channel],volume);
}

static void WriteFreq (int channel,int freq)
{
 int tmp1,tmp2,i,pan;

 if (freq>MAXFREQ)
  freq=MAXFREQ;

 if (freq>(MAXINT/441)) freq=MAXINT/441;        /* avoid overflows */
 if (channel==3)
 {
  for (i=0;i<5;++i)
   setfreq (NOISECH1+i,freq*441/basefreqtable[NOISECH1+i]);
  for (i=0;i<5;++i)
   setfreq (NOISECH2+i,freq*441/basefreqtable[NOISECH2+i]);
 }
 else
 {
  for (i=0;i<5;++i)
   setfreq (channel*5+i,freq*441/basefreqtable[channel*5+i]);
 }
 if (channel==3)
  if (feedback)
   tmp1=NOISECH1+GetWhiteNoiseChannel (freq);
  else
   tmp1=NOISECH2+GetPeriodicNoiseChannel (freq);
 else
  tmp1=channel*5+GetMusChannel (freq);
 if (tmp1!=channels[channel])
 {
  tmp2=channels[channel];
  channels[channel]=tmp1;
  WriteVolume (channel,channelvolume[channel]);
  channels[channel]=tmp2;
  WriteVolume (channel,0);
  channels[channel]=tmp1;
 }
 if (freq<5800) pan=200;
 else if (freq>100000) pan=255;
 else if (freq>40000) pan=(freq-40000)*255/(100000-40000);
 else pan=200-(freq-5800)*200/(40000-5800);
 if (channel==3)
  pan=(128-pan)*panning/500+128;
 else
  pan=(128-pan)*panning/100+128;
 setpan (channels[channel],pan);
}

static void ChannelOff (byte channel)
{
 WriteVolume (channel,0);
}

static void ChannelOn (byte channel)
{
 if (!sound_on)
  return;
 if ((channel==3 && channelfreq[channel]!=0) ||
     (channelfreq[channel]!=0 && channelfreq[channel]<=MAXFREQ))
 {
  invalidfreq[channel]=0;
  WriteFreq (channel,channelfreq[channel]);
  WriteVolume (channel,channelvolume[channel]);
 }
 else
 {
  WriteVolume (channel,0);
  invalidfreq[channel]=1;
 }
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

static void SetVolume (byte channel,byte vol)
{
 const byte voltabel[16]=
 { 255,202,161,128,101,80,64,51,40,32,25,20,16,13,10,0 };
 channelvolume[channel]=(vol>=15)? 0:voltabel[vol];
 WriteVolume (channel,channelvolume[channel]);
}

static void SetFreq (byte channel, word freq)
{
 if (!freq) freq=1024;
 ChannelFreq[channel]=freq;
 channelfreq[channel]=(freq)? (357954500/(32*(int)freq)):0;
 if (channel_on[channel])
 {
  ChannelOn (channel);
 }
 if (channel==2 && noise_output_channel_3)
  SetFreq (3,freq);
}

static void NoiseFeedback_On (void)
{
 if (feedback==1)
  return;
 channels[3]=NOISECH2+GetPeriodicNoiseChannel(channelfreq[3]);
 WriteVolume (3,0);
 channels[3]=NOISECH1+GetWhiteNoiseChannel(channelfreq[3]);
 WriteVolume (3,channelvolume[3]);
 feedback=1;
}
static void NoiseFeedback_Off (void)
{
 if (feedback==0)
  return;
 channels[3]=NOISECH1+GetWhiteNoiseChannel(channelfreq[3]);
 WriteVolume (3,0);
 channels[3]=NOISECH2+GetPeriodicNoiseChannel(channelfreq[3]);
 WriteVolume (3,channelvolume[3]);
 feedback=0;
}

static void SetVoices (void)
{
 int i,channel;

 for (i=0;i<5;++i)
 {
  basefreqtable[i]=basefreqtable[i+5]=basefreqtable[i+10]=sample_params[i].basefreq;
  maxfreqtable[i]=maxfreqtable[i+5]=maxfreqtable[i+10]=sample_params[i].maxfreq;
  for (channel=0;channel<3;++channel)
   voiceon (channel*5+i,sample_ptr[i],sample_ptr[i]+sample_params[i].loopstart,
                        sample_ptr[i]+sample_params[i].loopend,1);
 }
 for (i=15;i<25;++i)
 {
  basefreqtable[i]=sample_params[i-10].basefreq;
  maxfreqtable[i]=sample_params[i-10].maxfreq;
  voiceon (i,sample_ptr[i-10],sample_ptr[i-10]+sample_params[i-10].loopstart,
                       sample_ptr[i-10]+sample_params[i-10].loopend,1);
 }
}

static int Init (void)
{
 int rate,freq,interpol,num_samples,num_bits,swap;
 if (initialised)
  return 1;
 switch (soundquality)
 {
  case 1:
   rate=11025;
   freq=100;
   interpol=0;
   break;
  case 2:
   rate=16000;
   freq=110;
   interpol=0;
   break;
  case 4:
   rate=32000;
   freq=130;
   interpol=0;
   break;
  case 5:
   rate=44100;
   freq=140;
   interpol=1;
   break;
  default:
   rate=22050;
   freq=120;
   interpol=0;
   break;
 }
#ifdef UNIX
 if (!LoadSamples())
  return 0;
#endif
 if (panning) stereo=1;
 if (!sound_hw_init (&num_bits,&num_samples,&rate,&freq,&swap,&stereo))
  return 0;
 if (!sound_init(num_bits,num_samples,rate,interpol,swap,stereo))
 {
  sound_hw_reset();
  return 0;
 }
#ifndef UNIX
 if (!LoadSamples())
 {
  sound_hw_reset ();
  sound_reset ();
  return 0;
 }
#endif
 SetVoices ();
 initialised=1;
 return 1;
}

static void Reset (void)
{
 int i;
 for (i=0;i<32;++i)
  voiceoff(i);
 sound_hw_reset ();
 sound_reset ();
 FreeSamples ();
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
  if (channel_on[i])
   ChannelOn (i);
}

static void Stop (void)
{
#ifdef UNIX_X
  kill(childpid,SIGUSR1);
#else
 SoundOff ();
#endif
}

static void Resume (void)
{
#ifdef UNIX_X
  kill(childpid,SIGUSR2);
#else
 SoundOn ();
#endif
}

#ifdef MSDOS
static void Interrupt_Routine (void)
{
}
#endif

struct SoundDriver_struct SB_SoundDriver=
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

