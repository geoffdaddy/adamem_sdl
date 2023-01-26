/****************************************************************************/
/**                                                                        **/
/**                                dev_dsp.h                               **/
/**                                                                        **/
/** Software interface for Unix-/dev/dsp                                   **/
/**                                                                        **/
/** Copyright (C) Marcel de Kogel 1996,1997,1998                           **/
/**     You are not allowed to distribute this software commercially       **/
/**     Please, notify me, if you make any changes to this file            **/
/****************************************************************************/

#ifdef SOUND

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/soundcard.h>

static pid_t childpid;

#define NUM_BUFS	8

static int audio_fd;
static int buf_size;
static int got_dsp=0;
static pid_t childpid;

#ifdef UNIX_X
void SoundSignal(int sig)
{
 static volatile int ok_to_continue=0;
 switch (sig)
 {
  case SIGUSR1:
   /* Suspend execution, until SIGUSR2 catched */
   signal (SIGUSR1,SoundSignal);
   ok_to_continue=0;
   while (!ok_to_continue) pause();
   break;
  case SIGUSR2:
   signal (SIGUSR2,SoundSignal);
   ok_to_continue=1;
   break;
 }
}
#endif

static void SoundMainLoop (void)
{
 static short buffer[NUM_SAMPLES_MAX];
 pid_t parent=getppid();
#ifdef UNIX_X
 signal(SIGUSR1,SoundSignal);
 signal(SIGUSR2,SoundSignal);
#endif
 while (1)
 {
  if (parent!=getppid())
   exit (-1);
  if (sound_update(buffer))
   write (audio_fd,buffer,buf_size);
 }
}

/****************************************************************************/
/** Initialisation Routines                                                **/
/****************************************************************************/

static int sound_hw_init(int *numbits,int *numsamples,
                         int *soundfreq,int *intfreq,int *swap,int *stereo)
{
 int format;
 int buf_bits;
 int tmp;
 if (Verbose) printf ("    Opening /dev/dsp... ");
 audio_fd=open ("/dev/dsp",O_WRONLY,0);
 if (audio_fd==-1)
 {
  if (Verbose) printf ("FAILED\n");
  return 0;
 }
 if (Verbose) printf ("OK\n");
 format=AFMT_S16_LE;
#ifdef LSB_FIRST
 *swap=0;
#else
 *swap=1;
#endif
 if (Verbose) printf ("    Setting format... ");
 if (ioctl(audio_fd,SNDCTL_DSP_SETFMT,&format)==-1)
 {
  if (Verbose) printf ("FAILED\n");
  close (audio_fd);
  return 0;
 }
 if (format!=AFMT_S16_LE)
 {
  format=AFMT_S16_BE;
 #ifdef LSB_FIRST
  *swap=1;
 #else
  *swap=0;
 #endif
  if (ioctl(audio_fd,SNDCTL_DSP_SETFMT,&format)==-1)
  {
   if (Verbose) printf ("FAILED\n");
   close (audio_fd);
   return 0;
  }
  if (format!=AFMT_S16_BE)
  {
   format=AFMT_U8;
   if (ioctl(audio_fd,SNDCTL_DSP_SETFMT,&format)==-1)
   {
    if (Verbose) printf ("FAILED\n");
    close (audio_fd);
    return 0;
   }
  }
  if (format!=AFMT_U8)
  {
   if (Verbose) printf ("FAILED\n");
   close (audio_fd);
   return 0;
  }
 }
 *numbits=(format==AFMT_U8)? 8:16;
 if (Verbose) printf ("%u bits\n    Setting mode... ",
                      *numbits);
 if (ioctl(audio_fd,SNDCTL_DSP_STEREO,stereo)==-1)
 {
  if (Verbose) printf ("FAILED\n");
  close (audio_fd);
  return 0;
 }
 if (*soundfreq<4000) *soundfreq=4000;
 if (*soundfreq>44100) *soundfreq=44100;
 if (Verbose) printf ("%s\n    Setting sampling frequency to %uHz... ",
 		      (*stereo)? "stereo":"mono",*soundfreq);
 if (ioctl(audio_fd,SNDCTL_DSP_SPEED,soundfreq)==-1)
 {
  if (Verbose) printf ("FAILED\n");
  close (audio_fd);
  return 0;
 }
 if (Verbose) printf ("got %uHz\n",*soundfreq);
 if (*intfreq<5) *intfreq=5;
 if (*intfreq>500) *intfreq=500;
 buf_size=(*soundfreq)/(*intfreq);
 for (buf_bits=0;buf_size>(1<<buf_bits);++buf_bits);
 if (*numbits==16) ++buf_bits;
 buf_size=1<<buf_bits;
 if (buf_size>NUM_SAMPLES_MAX)
  buf_size=NUM_SAMPLES_MAX;
 if (Verbose) printf ("    Setting %u buffers of %u bytes each... ",NUM_BUFS,buf_size);
 tmp=(NUM_BUFS<<16)|(buf_bits);
 if (ioctl(audio_fd,SNDCTL_DSP_SETFRAGMENT,&tmp)==-1)
 {
  if (Verbose) printf ("FAILED\n");
  close (audio_fd);
  return 0;
 }
 buf_size=1<<(tmp&0xFFFF);
 if (buf_size>NUM_SAMPLES_MAX)
 {
  if (Verbose) printf ("FAILED\n");
  close (audio_fd);
  return 0;
 }
 *numsamples=buf_size*8/(*numbits);
 *intfreq=(*soundfreq)/(*numsamples);
 if (Verbose) printf ("OK\n    Opening pipe... ");
 if (pipe(pipe_fd)==-1)
 {
  if (Verbose) printf ("FAILED\n");
  close (audio_fd);
  return 0;
 }
 fcntl (pipe_fd[0],F_SETFL,O_NDELAY);
 fcntl (pipe_fd[1],F_SETFL,O_NDELAY);
 if (Verbose) printf ("OK\n    Forking... ");
 switch (childpid=fork())
 {
  case -1: if (Verbose) printf ("FAILED\n");
           close (audio_fd);
           return 0;
  case 0:  close (pipe_fd[1]);
  	   SoundMainLoop ();
           break;
  default: close (pipe_fd[0]);
  	   if (Verbose) printf ("OK\n");
           break;
 }
 got_dsp=1;
 return 1;
}

static void sound_hw_reset (void)
{
 int i;
 if (!got_dsp)
  return;
 kill (childpid,SIGKILL);
 wait (&i);
 close (audio_fd);
 got_dsp=0;
}

#else		/* SOUND */

static int sound_hw_init(int *numbits,int *numsamples,
                         int *soundfreq,int *intfreq,int *swap,int *stereo)
{
 return 0;
}

static void sound_hw_reset (void)
{
}

#endif
