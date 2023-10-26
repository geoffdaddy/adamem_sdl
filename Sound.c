/** ADAMEm: Coleco ADAM emulator ********************************************/
/**                                                                        **/
/**                                 Sound.c                                **/
/**                                                                        **/
/** This file contains the system-independent part of the Coleco sound     **/
/** hardware emulation routines                                            **/
/**                                                                        **/
/** Copyright (C) Marcel de Kogel 1996,1997,1998,1999                      **/
/**     You are not allowed to distribute this software commercially       **/
/**     Please, notify me, if you make any changes to this file            **/
/****************************************************************************/

#ifdef SOUND

#include "Coleco.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL_timer.h>

#ifdef ZLIB
#include <zlib.h>
#define fopen          gzopen
#define fclose         gzclose
#define fread(B,N,L,F) gzread(F,B,(L)*(N))
#endif

char szSoundFileName[256];

struct sample_info sample_params[15];
short *sample_ptr[15];

int soundmode=255;
int mastervolume=10;
int panning=0;
int chorus=0;
int reverb=7;

static struct SoundDriver_struct SoundDriver;

#ifdef MSDOS
 struct _SB_Info SB_Info;
 static void GetSBInfo (void)
 {
  char *blaster=getenv ("BLASTER");
  memset (&SB_Info,0,sizeof(SB_Info));
  if (blaster)
  {
   strupr (blaster);
   while (*blaster)
   {
    while (*blaster==' ' || *blaster=='\t')
     ++blaster;
    switch (*blaster++)
    {
     case 'A': SB_Info.baseport=(word)strtol(blaster,NULL,16);
               break;
     case 'I': SB_Info.irq=(byte)strtol(blaster,NULL,10);
               break;
     case 'D': SB_Info.dma_low=(byte)strtol(blaster,NULL,10);
               break;
     case 'H': SB_Info.dma_high=(byte)strtol(blaster,NULL,10);
               break;
     case 'T': SB_Info.type=(byte)strtol(blaster,NULL,10);
               break;
     case 'E': SB_Info.emu_baseport=(word)strtol(blaster,NULL,16);
               break;
     case 'P': SB_Info.mpu_baseport=(word)strtol(blaster,NULL,16);
               break;
    }
    while (*blaster && *blaster!=' ' && *blaster!='\t')
     ++blaster;
   }
  }
 }
#endif

int InitSound (void)
{
 int mode;
 int init_done=0;
 if (panning<0) panning=0;
 if (panning>100) panning=100;
 if (reverb>100) reverb=100;
 if (reverb<0) reverb=0;
 if (chorus>100) chorus=100;
 if (chorus<0) chorus=0;
 mode=soundmode;
 soundmode=0;
 if (!mode)
  return 1;
 if (mastervolume<0) mastervolume=0;
 if (mastervolume>15) mastervolume=15;
 if (Verbose)
 {
  printf ("  Detecting sound card...\n");
  fflush (stdout);
 }
#ifdef MSDOS
 GetSBInfo ();
 if (mode>6 || mode<0) mode=255;
 if (mode==255)
 {
  init_done=1;
  if (AWE_SoundDriver.Init())
  {
   SoundDriver=AWE_SoundDriver;
   mode=6;
  }
  else
   if (GUS_SoundDriver.Init())
   {
    SoundDriver=GUS_SoundDriver;
    mode=5;
   }
   else
    if (SB_SoundDriver.Init())
    {
     SoundDriver=SB_SoundDriver;
     mode=4;
    }
    else
     if (Adlib_SoundDriver.Init())
     {
      SoundDriver=Adlib_SoundDriver;
      mode=2;
     }
     else
      if (Speaker_SoundDriver.Init())
      {
       SoundDriver=Speaker_SoundDriver;
       mode=1;
      }
      else
      {
       init_done=0;
       mode=0;
      }
 }
 else
  switch (mode)
  {
   case 6:
    SoundDriver=AWE_SoundDriver;
    break;
   case 5:
    SoundDriver=GUS_SoundDriver;
    break;
   case 4:
    SoundDriver=SB_SoundDriver;
    break;
   case 3:
    SoundDriver=Combined_SoundDriver;
    break;
   case 2:
    SoundDriver=Adlib_SoundDriver;
    break;
   default:
    SoundDriver=Speaker_SoundDriver;
    break;
  }
#else
 SoundDriver=SB_SoundDriver;
#endif
 if (init_done)
  soundmode=1;
 else if (mode)
  soundmode=SoundDriver.Init();
 if (!soundmode)
  return 1;
 SoundDriver.SetMasterVolume (mastervolume);
 return 2;
}

void TrashSound (void)
{
 if (soundmode)
  SoundDriver.Reset ();
}

void StopSound (void)
{
 if (soundmode)
  SoundDriver.Stop ();
}

void ResumeSound (void)
{
 if (soundmode)
  SoundDriver.Resume ();
}

void Sound (int r,int v)
{
 if (soundmode)
  SoundDriver.WriteSoundReg (r,v);
 //should be 32us delay when writing!!!
 SDL_Delay(1);
}

#ifdef SOUND_PSG
void PSG_Sound (int r,int v)
{
 if (soundmode)
  SoundDriver.PSGWriteSoundReg (r,v);
}
#endif

void DecreaseSoundVolume (void)
{
 if (mastervolume)
  SoundDriver.SetMasterVolume (--mastervolume);
}

void IncreaseSoundVolume (void)
{
 if (mastervolume<15)
  SoundDriver.SetMasterVolume (++mastervolume);
}

void ToggleSound (void)
{
 if (soundmode)
  SoundDriver.Toggle ();
}

void ToggleSoundChannel (int channel)
{
 if (soundmode)
  SoundDriver.ToggleChannel (channel);
}

int LoadSamples (void)
{
 FILE *f;
 int i;
 if (Verbose) printf ("    Opening %s... ",szSoundFileName);
 f=fopen (szSoundFileName,"rb");
 if (!f) { if (Verbose) puts("NOT FOUND");return 0; }
 if (Verbose) printf ("OK\n    Loading samples... ");
 if (fread (sample_params,1,sizeof(sample_params),f)!=sizeof(sample_params))
 {
  if (Verbose) puts ("READ ERROR");
  return 0;
 }
 for (i=0;i<15;++i)
 {
  sample_ptr[i]=malloc (sample_params[i].len*2);
  if (!sample_ptr[i])
  {
   if (Verbose) puts ("OUT OF MEMORY");
   FreeSamples ();
   return 0;
  }
  if (fread (sample_ptr[i],1,sample_params[i].len*2,f)!=sample_params[i].len*2)
  {
   if (Verbose) puts ("READ ERROR");
   FreeSamples ();
   return 0;
  }
 }
 fclose (f);
 if (Verbose) puts ("OK");
 return 1;
}

void FreeSamples (void)
{
 int i;
 for (i=0;i<15;++i)
  if (sample_ptr[i])
  {
   free (sample_ptr[i]);
   sample_ptr[i]=0;
  }
}

#else /* SOUND */

int InitSound (void) { return 0; }
void TrashSound (void) { };
void StopSound (void) { };
void ResumeSound (void) { };
void Sound (int r,int v) { };
void DecreaseSoundVolume (void) { };
void IncreaseSoundVolume (void) { };
void ToggleSound (void) { };
void ToggleSoundChannel (int channel) { };

#endif
