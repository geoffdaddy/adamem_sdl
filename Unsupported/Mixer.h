/****************************************************************************/
/**                                                                        **/
/**                                 Mixer.h                                **/
/**                                                                        **/
/** General sound mixer functions                                          **/
/**                                                                        **/
/** Copyright (C) Marcel de Kogel 1996,1997,1998                           **/
/**     You are not allowed to distribute this software commercially       **/
/**     Please, notify me, if you make any changes to this file            **/
/****************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#ifdef UNIX
 /* pipe types */
 #define SETVOLUME		0
 #define SETPAN                 1
 #define SETFREQ                2
 #define VOICEON                3
 #define VOICEOFF               4
 #define SOUNDINIT              5
 #define FLUSHPIPE		-1
 static int pipe_fd[2];
#endif

#define NUM_SOUND_CHANNELS      32      /* Maximum number of soundchannels */
#define NUM_SAMPLES_MAX         1024    /* Maximum size of samplebuffer */

static int interpolate=0;       /* 1 if linear interpolation should be done */
static int _16bit=0;            /* 1 if using a 16 bit device */
static int playbackrate;        /* playbackrate in Hertz */
static int num_samples;         /* number of samples per buffer */
static int sound_stereo;        /* 1 if using stereo playback */
static int swapbytes;           /* 1 if bytes in sample should be swapped */

static int old_samples_reverb[32][NUM_SAMPLES_MAX];
static int old_sample_count;

struct sampleinfo_struct
{
 const short *ptr;              /* 0  */
 int len;                       /* 4  */
 int currlen;                   /* 8  */
 int ptradd;                    /* 12 */
 int recurparam;                /* 16 */
 int currrecurparam;            /* 20 */
 int volume;                    /* 24 */
 int orgvolume;                 /* 28 */
 int playing;                   /* 32 */
 int freq;                      /* 36 */
 int newfreq;                   /* 40 */
 int loopenable;                /* 44 */
 int looplen;                   /* 48 */
 int pan;                       /* 52 */
 int volume_left;               /* 56 */
 int volume_right;              /* 60 */
 int unused[(64-60)/4];
};

static struct sampleinfo_struct sampleinfo[NUM_SOUND_CHANNELS];

#ifdef UNIX
/*
static void writepipe (int type,int a,int b,int c,int d,int e,int f)
{
 static int pipebuf[7*64];
 static int bufcount=0;
 if (type!=FLUSHPIPE && bufcount<64)
 {
  pipebuf[bufcount*7+0]=type;
  pipebuf[bufcount*7+1]=a;
  pipebuf[bufcount*7+2]=b;
  pipebuf[bufcount*7+3]=c;
  pipebuf[bufcount*7+4]=d;
  pipebuf[bufcount*7+5]=e;
  pipebuf[bufcount*7+6]=f;
  ++bufcount;
 }
 if (type==FLUSHPIPE || bufcount>=64)
 {
  bufcount*=7*sizeof(int);
  if (bufcount)
   write (pipe_fd[1],pipebuf,bufcount);
  bufcount=0;
 }
}
*/
static void writepipe (int type,int a,int b,int c,int d,int e,int f)
{
 static int pipebuf[7];
 pipebuf[0]=type;
 pipebuf[1]=a;
 pipebuf[2]=b;
 pipebuf[3]=c;
 pipebuf[4]=d;
 pipebuf[5]=e;
 pipebuf[6]=e;
 write (pipe_fd[1],pipebuf,7*sizeof(int));
}
#endif

/****************************************************************************/
/** Voice Manipulation Routines                                            **/
/****************************************************************************/

static void _setvolume (int channel,int vol)
{
 int pan;
 if (vol<0) vol=0;
 if (vol>255) vol=255;
 sampleinfo[channel].volume=vol;
 pan=sampleinfo[channel].pan;
 if (pan<128)
  sampleinfo[channel].volume_left=vol;
 else
  sampleinfo[channel].volume_left=vol*(255-pan)/128;
 if (pan>128)
  sampleinfo[channel].volume_right=vol;
 else
  sampleinfo[channel].volume_right=vol*pan/128;
}

static void _setpan (int channel,int pan)
{
 if (pan<0) pan=0;
 if (pan>255) pan=255;
 sampleinfo[channel].pan=pan;
 _setvolume (channel,sampleinfo[channel].volume);
}

static void _setfreq (int channel,int freq)
{
 if (freq>44100*4) freq=44100*4;
 sampleinfo[channel].newfreq=freq;
}

static void _voiceon (int channel,const short *start,
                      const short *loopstart,const short *loopend,int looped)
{
 sampleinfo[channel].playing=0;
 sampleinfo[channel].ptr=start;
 sampleinfo[channel].len=loopend-start;
 sampleinfo[channel].currlen=0;
 sampleinfo[channel].ptradd=0;
 sampleinfo[channel].recurparam=0;
 sampleinfo[channel].currrecurparam=0;
 sampleinfo[channel].volume=0;
 sampleinfo[channel].orgvolume=0;
 sampleinfo[channel].freq=0;
 sampleinfo[channel].newfreq=0;
 sampleinfo[channel].loopenable=looped;
 sampleinfo[channel].looplen=loopend-loopstart;
 sampleinfo[channel].playing=1;
}

static void _voiceoff (int channel)
{
 sampleinfo[channel].playing=0;
}

static void setvolume (int channel,int vol)
{
 #ifdef UNIX
  writepipe (SETVOLUME,channel,vol,0,0,0,0);
 #else
  _setvolume (channel,vol);
 #endif
}

static void setpan (int channel,int pan)
{
 if (sound_stereo)
 {
  #ifdef UNIX
   writepipe (SETPAN,channel,pan,0,0,0,0);
  #else
   _setpan (channel,pan);
  #endif
 }
}

static void setfreq (int channel,int freq)
{
 #ifdef UNIX
  writepipe (SETFREQ,channel,freq,0,0,0,0);
 #else
  _setfreq (channel,freq);
 #endif
}

static void voiceon (int channel,const short *start,
		     const short *loopstart,const short *loopend,int looped)
{
 #ifdef UNIX
  writepipe (VOICEON,channel,(int)start,(int)loopstart,(int)loopend,looped,0);
 #else
  _voiceon (channel,start,loopstart,loopend,looped);
 #endif
}

static void voiceoff (int channel)
{
 #ifdef UNIX
  writepipe (VOICEOFF,channel,0,0,0,0,0);
 #else
  _voiceoff (channel);
 #endif
}

/****************************************************************************/
/** Internal Routines                                                      **/
/****************************************************************************/

static inline int getdither (void)
{
/* White noise generator
 static unsigned seed=151886886;
 seed*=0x15A4E35;
 seed+=1;
 return seed;
 static byte dither[]=
 {
*/
/* pre-calculated, optimised dither lookup table
  #include "samples/dither.h"
 };
 static int count=44100;
 if (--count<0) count=44099;
 return dither[count];
*/
 return 0;
}

static inline int swap16 (int sam)
{
 if (swapbytes) return ((sam&0xFF)<<8)|(sam>>8);
 return sam;
}

static inline int clip16 (int sam)
{
 sam+=getdither()<<1;
 sam>>=9;
 if (sam>=32767)
  return 32767;
 else
  if (sam<=-32768)
   return -32768;
  else
   return sam;
}

static inline int clip8 (int sam)
{
 sam+=getdither()<<9;
 sam>>=17;
 if (sam>=128)
  return 255;
 else
  if (sam<=-128)
   return 0;
  else
   return sam+0x80;
}

static void _mix_some_samples (void *addr)
{
 int i,j,*p1,*p2,*p3,*p4;
 static int buffer[NUM_SAMPLES_MAX];
 memset (buffer,0,num_samples*sizeof(int));
 for (j=0;j<NUM_SOUND_CHANNELS;++j)
 {
  register struct sampleinfo_struct *saminfo=&sampleinfo[j];
  if (saminfo->playing) {
   if (saminfo->volume)
   {
    register int currlen=saminfo->currlen;
    register int currrecurparam=saminfo->currrecurparam;
    if (!sound_stereo)
    {
     if (interpolate)
     {
      for (i=0;i<num_samples;++i)
      {
       int tmp;
       tmp=(((int)*(saminfo->ptr+currlen))*
           (65536-currrecurparam))>>16;
       tmp+=(((int)*(saminfo->ptr+currlen+1))*
            currrecurparam)>>16;
       buffer[i]+=tmp*saminfo->volume;
       currlen+=saminfo->ptradd;
       currrecurparam+=saminfo->recurparam;
       currlen+=(currrecurparam>>16);
       currrecurparam&=0x0000FFFF;
       if (currlen>=saminfo->len) {
        if (saminfo->loopenable)
        {
         saminfo->len=saminfo->looplen;
         while (currlen>=saminfo->len) currlen-=saminfo->len;
        }
        else
        {
         saminfo->playing=0;
         i=num_samples;
        }
       }
      }
     }
     else
     {
      for (i=0;i<num_samples;++i)
      {
       int tmp;
       tmp=(int)*(saminfo->ptr+currlen);
       buffer[i]+=tmp*saminfo->volume;
       currlen+=saminfo->ptradd;
       currrecurparam+=saminfo->recurparam;
       currlen+=(currrecurparam>>16);
       currrecurparam&=0x0000FFFF;
       if (currlen>=saminfo->len) {
        if (saminfo->loopenable)
        {
         saminfo->len=saminfo->looplen;
         while (currlen>=saminfo->len) currlen-=saminfo->len;
        }
        else
        {
         saminfo->playing=0;
         i=num_samples;
        }
       }
      }
     }
    }
    else
    {
     if (interpolate)
     {
      for (i=0;i<num_samples;i+=2)
      {
       int tmp;
       tmp=(((int)*(saminfo->ptr+currlen))*
           (65536-currrecurparam))>>16;
       tmp+=(((int)*(saminfo->ptr+currlen+1))*
            currrecurparam)>>16;
       buffer[i]+=tmp*saminfo->volume_left;
       buffer[i+1]+=tmp*saminfo->volume_right;
       currlen+=saminfo->ptradd;
       currrecurparam+=saminfo->recurparam;
       currlen+=(currrecurparam>>16);
       currrecurparam&=0x0000FFFF;
       if (currlen>=saminfo->len) {
        if (saminfo->loopenable)
        {
         saminfo->len=saminfo->looplen;
         while (currlen>=saminfo->len) currlen-=saminfo->len;
        }
        else
        {
         saminfo->playing=0;
         i=num_samples;
        }
       }
      }
     }
     else
     {
      for (i=0;i<num_samples;i+=2)
      {
       int tmp;
       tmp=(int)*(saminfo->ptr+currlen);
       buffer[i]+=tmp*saminfo->volume_left;
       buffer[i+1]+=tmp*saminfo->volume_right;
       currlen+=saminfo->ptradd;
       currrecurparam+=saminfo->recurparam;
       currlen+=(currrecurparam>>16);
       currrecurparam&=0x0000FFFF;
       if (currlen>=saminfo->len) {
        if (saminfo->loopenable)
        {
         saminfo->len=saminfo->looplen;
         while (currlen>=saminfo->len) currlen-=saminfo->len;
        }
        else
        {
         saminfo->playing=0;
         i=num_samples;
        }
       }
      }
     }
    }
    saminfo->currlen=currlen;
    saminfo->currrecurparam=currrecurparam;
   }
   else
   {
    saminfo->currrecurparam+=num_samples*saminfo->recurparam;
    saminfo->currlen+=num_samples*saminfo->ptradd;
    saminfo->currlen+=(saminfo->currrecurparam>>16);
    saminfo->currrecurparam&=0x0000FFFF;
    if (saminfo->loopenable)
     while (saminfo->currlen>=saminfo->len)
     {
      saminfo->len=saminfo->looplen;
      saminfo->currlen-=saminfo->len;
     }
     else if (saminfo->currlen>=saminfo->len)
           saminfo->playing=0;
   }
  }
 }
 if (reverb)
 {
  for (i=0;i<num_samples;++i)
   old_samples_reverb[old_sample_count][i]=buffer[i]*reverb/64;
  if (sound_stereo)
  {
   p1=old_samples_reverb[(old_sample_count-5)&31];
   p2=old_samples_reverb[(old_sample_count-13)&31];
   p3=old_samples_reverb[(old_sample_count-21)&31];
   p4=old_samples_reverb[(old_sample_count-29)&31];
   for (i=0;i<num_samples;i+=2)
   {
    j=p1[i]+(p2[i]>>1)+(p3[i]>>2)+(p4[i]>>3);
    buffer[i]+=j;
    buffer[i+1]+=j>>1;
   }
   p1=old_samples_reverb[(old_sample_count-7)&31];
   p2=old_samples_reverb[(old_sample_count-15)&31];
   p3=old_samples_reverb[(old_sample_count-23)&31];
   p4=old_samples_reverb[(old_sample_count-31)&31];
   for (i=0;i<num_samples;i+=2)
   {
    j=p1[i]+(p2[i]>>1)+(p3[i]>>2)+(p4[i]>>3);
    buffer[i+1]+=j;
    buffer[i]+=j>>1;
   }
  }
  else
  {
   p1=old_samples_reverb[(old_sample_count-7)&31];
   p2=old_samples_reverb[(old_sample_count-15)&31];
   p3=old_samples_reverb[(old_sample_count-23)&31];
   p4=old_samples_reverb[(old_sample_count-31)&31];
   for (i=0;i<num_samples;++i)
   {
    buffer[i]+=p1[i]+(p2[i]>>1)+(p3[i]>>2)+(p4[i]>>3);
   }
  }
  old_sample_count=(old_sample_count+1)&31;
 }
 if (_16bit)
 {
  short *offset=(short*)addr;
  for (i=0;i<num_samples;++i)
   *offset++=swap16(clip16(buffer[i]));
 }
 else
 {
  char *offset=(char*)addr;
  for (i=0;i<num_samples;++i)
   *offset++=clip8(buffer[i]);
 }
}

/****************************************************************************/
/** Initialisation Routines                                                **/
/****************************************************************************/

static int sound_init_done=0;
static void _sound_init (int numbits,int numsamples,
                         int soundfreq,int interpol,int swap,int stereo)
{
 memset (old_samples_reverb,0,sizeof(num_samples));
 old_sample_count=0;
 playbackrate=soundfreq;
 sound_stereo=stereo;
 num_samples=numsamples;
 if (sound_stereo) num_samples*=2;
 swapbytes=swap;
 _16bit=(numbits==16);
 interpolate=interpol;
 sound_init_done=1;
}

void sound_reset(void)
{
}

static int sound_init (int numbits,int numsamples,
                       int soundfreq,int interpol,int swap,int stereo)
{
 #ifdef UNIX
  writepipe (SOUNDINIT,numbits,numsamples,soundfreq,interpol,swap,stereo);
 #else
  _sound_init (numbits,numsamples,soundfreq,interpol,swap,stereo);
 #endif
 return 1;
}

/****************************************************************************/
/** Update Sound Buffer                                                    **/
/****************************************************************************/

#ifdef UNIX
static void readpipe (void)
{
 int pipebuf[7];
 while (read(pipe_fd[0],pipebuf,7*sizeof(int))>0)
 {
  switch (pipebuf[0])
  {
   case SETVOLUME:
    _setvolume (pipebuf[1],pipebuf[2]);
    break;
   case SETFREQ:
    _setfreq (pipebuf[1],pipebuf[2]);
    break;
   case SETPAN:
    _setpan (pipebuf[1],pipebuf[2]);
    break;
   case VOICEON:
    _voiceon (pipebuf[1],(const short*)pipebuf[2],(const short*)pipebuf[3],
              (const short*)pipebuf[4],pipebuf[5]);
    break;
   case VOICEOFF:
    _voiceoff (pipebuf[1]);
    break;
   case SOUNDINIT:
    _sound_init (pipebuf[1],pipebuf[2],pipebuf[3],
                 pipebuf[4],pipebuf[5],pipebuf[6]);
    break;
   default:
    break;
  }
 }
}
#endif

int sound_update (void *addr)
{
 int i;
 #ifdef UNIX
  readpipe ();
 #endif
 if (sound_init_done)
 {
  /* Check for frequency changes */
  for (i=0;i<NUM_SOUND_CHANNELS;++i)
   if (sampleinfo[i].newfreq!=sampleinfo[i].freq)
   {
    sampleinfo[i].ptradd=sampleinfo[i].newfreq/playbackrate;
    sampleinfo[i].recurparam=(sampleinfo[i].newfreq%playbackrate)<<16;
    sampleinfo[i].recurparam=((unsigned)sampleinfo[i].recurparam)/((unsigned)playbackrate);
    sampleinfo[i].freq=sampleinfo[i].newfreq;
   } 
  /* Fill the buffer */
  _mix_some_samples(addr);
  return 1;
 }
 return 0;
}
