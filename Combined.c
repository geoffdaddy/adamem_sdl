/** ADAMEm: Coleco ADAM emulator ********************************************/
/**                                                                        **/
/**                               Combined.c                               **/
/**                                                                        **/
/** Coleco sound hardware emulation via Adlib+SoundBlaster                 **/
/**                                                                        **/
/** Copyright (C) Marcel de Kogel 1996,1997,1998,1999                      **/
/**     You are not allowed to distribute this software commercially       **/
/**     Please, notify me, if you make any changes to this file            **/
/****************************************************************************/

#ifdef SOUND

#include "Coleco.h"

static int Init (void)
{
 if (!Adlib_SoundDriver.Init ())
  return 0;
 if (!SB_SoundDriver.Init ())
 {
  Adlib_SoundDriver.Reset ();
  return 0;
 }
 return 1;
}

static void Reset (void)
{
 SB_SoundDriver.Reset ();
 Adlib_SoundDriver.Reset ();
}

static void WriteSoundReg (int r,int v)
{
 switch (r)
 {
  case 0:
  case 1:
  case 2:
  case 3:
  case 5:
   Adlib_SoundDriver.WriteSoundReg (r,v);
   break;
  case 4:
   Adlib_SoundDriver.WriteSoundReg (r,v);
   SB_SoundDriver.WriteSoundReg (r,v);
   break;
  case 6:
  case 7:
   SB_SoundDriver.WriteSoundReg (r,v);
   break;
 }
}

static void Toggle (void)
{
 SB_SoundDriver.Toggle ();
 Adlib_SoundDriver.Toggle ();
}

static void ToggleChannel (int channel)
{
 SB_SoundDriver.ToggleChannel (channel);
 Adlib_SoundDriver.ToggleChannel (channel);
}

static void SetMasterVolume (int newvolume)
{
 SB_SoundDriver.SetMasterVolume (newvolume);
 Adlib_SoundDriver.SetMasterVolume (newvolume);
}

static void Stop (void)
{
 SB_SoundDriver.Stop ();
 Adlib_SoundDriver.Stop ();
}

static void Resume (void)
{
 SB_SoundDriver.Resume ();
 Adlib_SoundDriver.Resume ();
}

struct SoundDriver_struct Combined_SoundDriver=
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

