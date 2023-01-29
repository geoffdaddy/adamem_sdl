/** ADAMEm: Coleco ADAM emulator ********************************************/
/**                                                                        **/
/**                                 Unix.c                                 **/
/**                                                                        **/
/** This file contains general Unix routines used by both the SVGALib and  **/
/** the X-Windows versions of ADAMEm                                       **/
/**                                                                        **/
/** Copyright (C) Marcel de Kogel 1996,1997,1998,1999                      **/
/**     You are not allowed to distribute this software commercially       **/
/**     Please, notify me, if you make any changes to this file            **/
/****************************************************************************/

#include "Coleco.h"
#include "Unix.h"
#include <stdio.h>
#include <time.h>

#ifdef JOYSTICK
#include <linux/joystick.h>
#include <unistd.h>
#include <fcntl.h>

static int joy_fd;

int InitJoystick (int mode)
{
 if (!mode) return 0;
 if (Verbose)
  printf ("  Initialising joystick...\n    Opening /dev/js0... ");
 joy_fd=open ("/dev/js0",O_RDONLY);
 if (Verbose) puts ((joy_fd>=0)? "OK":"FAILED");
 return  (joy_fd>=0);
}

void TrashJoystick (void)
{
 if (joy_fd>=0) close (joy_fd);
}

void ReadJoystick (joypos_t *jp)
{
 struct JS_DATA_TYPE js;
 if (joy_fd>=0)
 {
  read (joy_fd,&js,JS_RETURN);
  jp->buttons=js.buttons;
  jp->x=js.x;
  jp->y=js.y;
 }
}
#else
int InitJoystick (int mode)
{
 return 0;
}

void TrashJoystick (void)
{
}

void ReadJoystick (joypos_t *jp)
{
 jp->buttons=0;
 jp->x=500;
 jp->y=500;
}
#endif

/* Return the time elapsed in microseconds */
int ReadTimer (void)
{
#ifdef HAVE_CLOCK
 return (int)((float)clock()*1000000.0/(float)CLOCKS_PER_SEC);
#else
 /* If clock() is unavailable, just return a large number */
 static int a=0;
 a+=1000000;
 return a;
#endif
}
