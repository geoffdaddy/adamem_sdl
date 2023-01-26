#ifndef _ADAMEM_SOUND_H
#define _ADAMEM_SOUND_H

/** ADAMEm: Coleco ADAM emulator ********************************************/
/**                                                                        **/
/**                                 Sound.h                                **/
/**                                                                        **/
/** This file contains Coleco sound harware related definitions            **/
/**                                                                        **/
/** Copyright (C) Marcel de Kogel 1996,1997,1998,1999                      **/
/**     You are not allowed to distribute this software commercially       **/
/**     Please, notify me, if you make any changes to this file            **/
/****************************************************************************/

int InitSound (void);                 /* Initialise the sound harware       */
void TrashSound (void);               /* Free resources taken by InitSound()*/
void StopSound (void);                /* Temporarily stop sound output      */
void ResumeSound (void);              /* Resume sound output                */
void Sound (int r,int v);             /* Write a value to a sound register  */
#ifdef SOUND_PSG
void PSG_Sound (int r,int v);         /* Write a value to a PSG register    */
#endif
void DecreaseSoundVolume (void);      /* Decrease master volume             */
void IncreaseSoundVolume (void);      /* Increase master volume             */
void ToggleSound (void);              /* Toggle sound on/off                */
void ToggleSoundChannel (int channel);/* Toggle a sound channel on/off      */

#ifdef SOUND

extern int soundmode;                 /* Sound mode to be used              */
extern int mastervolume;              /* 0=quiet, 15=maximum                */
extern int panning;                   /* 0=mono, 100=maximum                */

struct SoundDriver_struct             /* Sound driver definitions           */
{
 int  (*Init) (void);
 void (*Reset) (void);
 void (*WriteSoundReg) (int r,int v);
 void (*Toggle) (void);
 void (*ToggleChannel) (int channel);
 void (*SetMasterVolume) (int newvolume);
 void (*Stop) (void);
 void (*Resume) (void);
#ifdef SOUND_PSG
 void (*PSGWriteSoundReg) (int r,int v);
#endif
	
};

extern struct SoundDriver_struct SB_SoundDriver;
extern int soundquality;
#ifdef MSDOS
 extern struct SoundDriver_struct AWE_SoundDriver;
 extern struct SoundDriver_struct GUS_SoundDriver;
 extern struct SoundDriver_struct Combined_SoundDriver;
 extern struct SoundDriver_struct Adlib_SoundDriver;
 extern struct SoundDriver_struct Speaker_SoundDriver;
 extern int spk_channels[4];
#endif

extern int reverb;
extern int chorus;

#ifdef MSDOS
 struct _SB_Info                      /* Contains BLASTER settings          */
 {
  word baseport;
  byte irq;
  byte dma_low;
  byte dma_high;
  byte type;
  word emu_baseport;
  word mpu_baseport;
 };
 extern struct _SB_Info SB_Info;
#endif

struct sample_info                    /* Sample parameters                  */
{
 unsigned len;
 unsigned basefreq;
 unsigned maxfreq;
 unsigned loopstart;
 unsigned loopend;
};
extern struct sample_info sample_params[];
extern short *sample_ptr[];           /* Sample pointers                    */
extern char szSoundFileName[];        /* File containing the samples        */

int LoadSamples (void);               /* Load samples                       */
void FreeSamples (void);              /* Free sample memory                 */

#endif /* SOUND */

#endif