/** ADAMEm: Coleco ADAM emulator ********************************************/
/**                                                                        **/
/**                                 AWE32.c                                **/
/**                                                                        **/
/** Coleco sound hardware emulation via SB AWE32                           **/
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
#include <math.h>
#include "AWEFn.h"

#define AWEFREQ(a)      ((unsigned)(log(a)*5909.27-5850.98))
#define FINETUNE(a)     (57344-AWEFREQ(a))

#define MIN_DRAM        (256*1024) /* 512KB                                 */
#define NOISECH1        15         /* oscillator used for "white" noise     */
#define NOISECH2        20         /* oscillator used for "periodic" noise  */
#define MAXFREQ         0xC57C     /* Frequency divider=8                   */

static int loopstart[25];

static int finetunetable[25];
static int maxfreqtable[25];

static int invalidfreq[4];
static int channels[4]={0,5,10,NOISECH1};
static int feedback=1;
static byte channel_on[4]= {1,1,1,1};
static byte noise_output_channel_3=0;
static word channelfreq[4]= {0xFFFF,0xFFFF,0xFFFF,0xFFFF};
static word channelvolume[4];
static word ChannelFreq[4];
static word NoiseFreqs[4]= {16,32,64,0};
static word AWEPoort=0;
static byte sound_on=1;

static word Freqs[] =
{
 #include "AWEFreqs.h"
};

static int GetMusChannel (int freq)
{
 if (freq<=maxfreqtable[0])
  return 0;
 if (freq<=maxfreqtable[1])
  return 1;
 if (freq<=maxfreqtable[2])
  return 2;
 if (freq<=maxfreqtable[3])
  return 3;
 return 4;
}

static int GetPeriodicNoiseChannel (int freq)
{
 if (freq<=maxfreqtable[NOISECH2+0])
  return 0;
 if (freq<=maxfreqtable[NOISECH2+1])
  return 1;
 if (freq<=maxfreqtable[NOISECH2+2])
  return 2;
 if (freq<=maxfreqtable[NOISECH2+3])
  return 3;
 return 4;
}

static int GetWhiteNoiseChannel (int freq)
{
 if (freq<=maxfreqtable[NOISECH1+0])
  return 0;
 if (freq<=maxfreqtable[NOISECH1+1])
  return 1;
 if (freq<=maxfreqtable[NOISECH1+2])
  return 2;
 if (freq<=maxfreqtable[NOISECH1+3])
  return 3;
 return 4;
}

static void WriteVolume (int channel,int volume)
{
 static int realvolume[30]=
 { -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
   -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
   -1,-1,-1,-1,-1,-1,-1,-1,-1,-1, };
 if (!sound_on ||
     !channel_on[channel] || invalidfreq[channel] || !mastervolume)
  volume=255;
 volume+=(15-mastervolume)*8;
 if (volume>255)
  volume=255;
 if (volume<0)
  volume=0;
 if (realvolume[channels[channel]]==volume)
  return;
 realvolume[channels[channel]]=volume;
 AWE_SetInitialFilterInitialVolume (channels[channel],255,volume);
}

static void WriteFreq (int channel,int freq)
{
 int tmp1,tmp2,i,pan;
 if (freq<18168) pan=200;
 else if (freq>40000) pan=255;
 else if (freq>28288) pan=(freq-30336)*255/(40000-30336);
 else pan=200-(freq-18167)*200/(30336-18167);
 pan=(128-pan)*panning/100+128;
 if (channel==3)
 {
  pan=(128-pan)/2+128;
  for (i=0;i<5;++i)
  {
   AWE_SetInitialPitch (NOISECH1+i,freq+finetunetable[NOISECH1+i]);
   AWE_SetPanLoopStart (NOISECH1+i,pan,loopstart[NOISECH1+i]);
   AWE_SetInitialPitch (NOISECH2+i,freq+finetunetable[NOISECH2+i]);
   AWE_SetPanLoopStart (NOISECH2+i,pan,loopstart[NOISECH2+i]);
  }
 }
 else
 {
  for (i=0;i<5;++i)
  {
   AWE_SetInitialPitch (channel*5+i,freq+finetunetable[channel*5+i]);
   AWE_SetPanLoopStart (channel*5+i,pan,loopstart[channel*5+i]);
  }
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
  WriteVolume (channel,255);
  channels[channel]=tmp1;
 }
}

static void ChannelOff (byte channel)
{
 WriteVolume (channel,255);
}

static void ChannelOn (byte channel)
{
 if (!sound_on)
  return;
 if ((channel==3 && channelfreq[channel]!=0xFFFF) ||
     channelfreq[channel]<=MAXFREQ)
 {
  invalidfreq[channel]=0;
  WriteFreq (channel,channelfreq[channel]);
  WriteVolume (channel,channelvolume[channel]);
 }
 else
 {
  WriteVolume (channel,255);
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
 channelvolume[channel]=(vol==15)? 255:(vol*16/3);
 WriteVolume (channel,channelvolume[channel]);
}

static void SetFreq (byte channel, word freq)
{
 if (!freq) freq=1024;
 ChannelFreq[channel]=freq;
 channelfreq[channel]=Freqs[freq];
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
 WriteVolume (3,255);
 channels[3]=NOISECH1+GetWhiteNoiseChannel(channelfreq[3]);
 WriteVolume (3,channelvolume[3]);
 feedback=1;
}
static void NoiseFeedback_Off (void)
{
 if (feedback==0)
  return;
 channels[3]=NOISECH1+GetWhiteNoiseChannel(channelfreq[3]);
 WriteVolume (3,255);
 channels[3]=NOISECH2+GetPeriodicNoiseChannel(channelfreq[3]);
 WriteVolume (3,channelvolume[3]);
 feedback=0;
}

static int SetVoices (void)
{
 unsigned ramptr;
 int channel;
 int i;
 /* Upload samples to AWE RAM */
 ramptr=0;
 for (i=0;i<30;++i) AWE_SetOscillator(i,1);
 for (i=15;i<25;++i)
 {
  loopstart[i]=AWE_RAMSTART+ramptr+sample_params[i-10].loopstart;
  finetunetable[i]=FINETUNE(sample_params[i-10].basefreq);
  maxfreqtable[i]=AWEFREQ(sample_params[i-10].maxfreq);
  AWE_UploadSample (AWE_RAMSTART+ramptr,sample_ptr[i-10],sample_params[i-10].len);
  ramptr+=sample_params[i-10].len;
 }
 for (i=0;i<5;++i)
 {
  loopstart[i]=loopstart[i+5]=loopstart[i+10]=
   AWE_RAMSTART+ramptr+sample_params[i].loopstart;
  finetunetable[i]=finetunetable[i+5]=finetunetable[i+10]=
   FINETUNE(sample_params[i].basefreq);
  maxfreqtable[i]=maxfreqtable[i+5]=maxfreqtable[i+10]=
   AWEFREQ(sample_params[i].maxfreq);
  AWE_UploadSample (AWE_RAMSTART+ramptr,sample_ptr[i],sample_params[i].len);
  ramptr+=sample_params[i].len;
 }
 /* Reset all oscillators */
 for (channel=0;channel<30;++channel)
 {
  AWE_ResetOscillator(channel);
  AWE_SetInitialPitch (channel,57344);
  AWE_SetPanLoopStart (channel,128,10);
  AWE_SetChorusLoopEnd (channel,0,100);
  AWE_SetFilterQPlayPosition (channel,0,10);
  AWE_SetEG2HoldAttack (channel,0x7F,0x7F);
  AWE_SetEG2SustainDecay (channel,0x7F,0x7F);
  AWE_SetInitialFilterInitialVolume (channel,255,255);
  AWE_SetOverallVolumeOverallFilterCutoff (channel,0,0);
  AWE_SetEG1PitchFilterCutoff (channel,0,0);
  AWE_SetLFO1PitchFilterCutoff (channel,0,0);
  AWE_SetEG2DelayTime (channel,32768);
  AWE_SetLFO1VolumeFrequency (channel,0,0);
  AWE_SetLFO1DelayTime (channel,32768);
  AWE_SetLFO2PitchFreq (channel,0,0);
  AWE_SetEG1DelayTime (channel,32768);
  AWE_SetEG1HoldAttack (channel,0,0);
  AWE_SetEG1SustainDecay (channel,0,0x7F);
  AWE_SetLFO2DelayTime (channel,32768);
  AWE_SetReverbSend (channel,reverb);
 }
 /* Set loopstart, loopend and playposition */
 ramptr=0;
 for (i=15;i<25;++i)
 {
  AWE_SetPanLoopStart (i,128,loopstart[i]);
  AWE_SetChorusLoopEnd (i,chorus,AWE_RAMSTART+ramptr+sample_params[i-10].loopend);
  AWE_SetFilterQPlayPosition (i,0,AWE_RAMSTART+ramptr);
  ramptr+=sample_params[i-10].len;
 }
 for (i=0;i<5;++i)
 {
  for (channel=0;channel<3;++channel)
  {
   AWE_SetPanLoopStart (channel*5+i,128,loopstart[i]);
   AWE_SetChorusLoopEnd (channel*5+i,chorus,AWE_RAMSTART+ramptr+sample_params[i].loopend);
   AWE_SetFilterQPlayPosition (channel*5+i,0,AWE_RAMSTART+ramptr);
  }
  ramptr+=sample_params[i].len;
 }
 /* Return success */
 return 1;
}

static int Init (void)
{
 reverb=reverb*255/100;
 chorus=chorus*255/100;
 if (SB_Info.baseport)
 {
  if (SB_Info.type>=6)
   AWEPoort=SB_Info.emu_baseport;
 }
 if (!AWEPoort)
  return 0;
 if (Verbose) printf ("    Checking for an AWE32 at %X... ",AWEPoort);
 AWE_SetBasePort (AWEPoort);
 if (!AWE_Detect())
 {
  if (Verbose) puts ("Not found");
  return 0;
 }
 if (Verbose) printf ("OK\n    Checking available AWE RAM... ");
 if (AWE_DetectDRAM()<MIN_DRAM)
 {
  if (Verbose) printf ("Failed - Less than %uKB\n",MIN_DRAM*2/1024);
  return 0;
 }
 if (Verbose) puts ("OK");
 if (!LoadSamples())
  return 0;
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
 for (i=0;i<30;++i)
  AWE_SetInitialFilterInitialVolume(i,255,255);
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
 SoundOff ();
}

static void Resume (void)
{
 SoundOn ();
}

struct SoundDriver_struct AWE_SoundDriver=
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

