/** ADAMEm: Coleco ADAM emulator ********************************************/
/**                                                                        **/
/**                                  GUS.c                                 **/
/**                                                                        **/
/** Coleco sound hardware emulation via Gravis UltraSound                  **/
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

#define GUS_NUMCHANNELS 15
#include "GUSFn.h"

#define MIN_DRAM        (512*1024) /* 512KB */
#define NOISECH1        9          /* oscillator used for "white" noise */
#define NOISECH2        12         /* oscillator used for "periodic" noise */
#define MAXFREQ         2200000    /* in cHertz */

#define MAX_CHANNEL     15

static int basefreqtable[15];
static int maxfreqtable[15];

static int invalidfreq[4];
static int channels[4]={0,3,6,NOISECH1};
static int feedback=1;
static byte channel_on[4]= {1,1,1,1};
static byte noise_output_channel_3=0;
static unsigned channelfreq[4];
static unsigned channelvolume[4];
static unsigned ChannelFreq[4];
static unsigned NoiseFreqs[4]= {16,32,64,0};
static byte sound_on=1;
static byte sound_is_on=1;

static int GetMusChannel (int freq)
{
 if (freq<(maxfreqtable[0]*100))
  return 0;
 if (freq<(maxfreqtable[1]*100))
  return 1;
 return 2;
}

static int GetPeriodicNoiseChannel (int freq)
{
 if (freq<(maxfreqtable[NOISECH2]*100))
  return 0;
 if (freq<(maxfreqtable[NOISECH2+1]*100))
  return 1;
 return 2;
}

static int GetWhiteNoiseChannel (int freq)
{
 if (freq<(maxfreqtable[NOISECH1]*100))
  return 0;
 if (freq<(maxfreqtable[NOISECH1+1]*100))
  return 1;
 return 2;
}

static void WriteVolume (int channel,int volume)
{
 if (!sound_on || !sound_is_on ||
     !channel_on[channel] || invalidfreq[channel])
  volume=0;
 if (mastervolume)
 {
  volume-=((int)(15-mastervolume)*256/3);
  if (volume<0)
   volume=0;
 }
 else
  volume=0;
 GUS_SetVolume (channels[channel],volume);
}

static void WriteFreq (int channel,int freq)
{
 int tmp1,tmp2,i;
 if (freq>MAXFREQ)
  freq=MAXFREQ;
 if (channel==3)
 {
  for (i=0;i<3;++i)
   GUS_SetPitch (NOISECH1+i,freq*441/basefreqtable[NOISECH1+i]);
  for (i=0;i<3;++i)
   GUS_SetPitch (NOISECH2+i,freq*441/basefreqtable[NOISECH2+i]);
 }
 else
 {
  for (i=0;i<3;++i)
   GUS_SetPitch (channel*3+i,freq*441/basefreqtable[channel*3+i]);
 }
 if (channel==3)
  if (feedback)
   tmp1=NOISECH1+GetWhiteNoiseChannel (freq);
  else
   tmp1=NOISECH2+GetPeriodicNoiseChannel (freq);
 else
  tmp1=channel*3+GetMusChannel (freq);
 if (tmp1!=channels[channel])
 {
  tmp2=channels[channel];
  channels[channel]=tmp1;
  WriteVolume (channel,channelvolume[channel]);
  channels[channel]=tmp2;
  WriteVolume (channel,0);
  channels[channel]=tmp1;
 }
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
 if (!sound_is_on)
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
 sound_is_on=0;
 for (i=0;i<4;++i)
  ChannelOff (i);
}

static void SoundOn (void)
{
 int i;
 if (!sound_on)
  return;
 sound_is_on=1;
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
 channelvolume[channel]=(vol>=15)? 0:4095-(vol*256/3);
 WriteVolume (channel,channelvolume[channel]);
}

static void SetFreq (byte channel, word freq)
{
 if (!freq) freq=1024;
 ChannelFreq[channel]=freq;
 channelfreq[channel]=(freq)? (357954500/(32*(int)freq)):0;
 if (channel_on[channel])
  ChannelOn (channel);
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

static int SetVoices (void)
{
 unsigned ramptr=0;
 int i,channel;

 for (channel=0;channel<MAX_CHANNEL;++channel)
 {
  GUS_SetPitch (channel,0);
  GUS_SetVolume (channel,0);
 }

 for (i=0;i<3;++i)
 {
  GUS_UploadSample (ramptr,(const char*)sample_ptr[i+1],sample_params[i+1].len*2);
  basefreqtable[i]=basefreqtable[i+3]=basefreqtable[i+6]=sample_params[i+1].basefreq;
  maxfreqtable[i]=maxfreqtable[i+3]=maxfreqtable[i+6]=sample_params[i+1].maxfreq;
  for (channel=0;channel<3;++channel)
   GUS_VoiceOn (channel*3+i,ramptr+sample_params[i+1].loopstart*2,
                           ramptr+sample_params[i+1].loopend*2);
  ramptr+=sample_params[i+1].len*2;
 }
 for (i=9;i<12;++i)
 {
  GUS_UploadSample (ramptr,(const char*)sample_ptr[i-3],sample_params[i-3].len*2);
  basefreqtable[i]=sample_params[i-3].basefreq;
  maxfreqtable[i]=sample_params[i-3].maxfreq;
  GUS_VoiceOn (i,ramptr+sample_params[i-3].loopstart*2,
                 ramptr+sample_params[i-3].loopend*2);
  ramptr+=sample_params[i-3].len*2;
 }
 for (i=12;i<15;++i)
 {
  GUS_UploadSample (ramptr,(const char*)sample_ptr[i-1],sample_params[i-1].len*2);
  basefreqtable[i]=sample_params[i-1].basefreq;
  maxfreqtable[i]=sample_params[i-1].maxfreq;
  GUS_VoiceOn (i,ramptr+sample_params[i-1].loopstart*2,
                 ramptr+sample_params[i-1].loopend*2);
  ramptr+=sample_params[i-1].len*2;
 }

 return 1;
}

static int Init (void)
{
 if (!GUS_Init())
  return 0;
 if (Verbose) printf ("    Checking available GUS RAM... ");
 if (GUS_DetectDRAM()<MIN_DRAM)
 {
  printf ("Failed - Less than %uKB available\n",MIN_DRAM/1024);
  return 0;
 }
 if (Verbose) puts ("OK");
 if (!LoadSamples()) return 0;
 if (!SetVoices ())
 {
  FreeSamples ();
  return 0;
 }
 FreeSamples ();
 return 1;
}

static void Reset (void)
{
 int i;
 for (i=0;i<32;++i)
  GUS_SetVolume(i,0);
 GUS_Exit ();
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
  WriteVolume (i,channelvolume[i]);
}

static void Stop (void)
{
 SoundOff ();
}

static void Resume (void)
{
 SoundOn ();
}

struct SoundDriver_struct GUS_SoundDriver=
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

