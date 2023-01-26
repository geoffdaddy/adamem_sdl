/** ADAMEm: Coleco ADAM emulator ********************************************/
/**                                                                        **/
/**                                   X.c                                  **/
/**                                                                        **/
/** This file contains the X-Windows specific routines. It does not        **/
/** include the sound emulation code                                       **/
/**                                                                        **/
/** Copyright (C) Marcel de Kogel 1996,1997,1998,1999                      **/
/**     You are not allowed to distribute this software commercially       **/
/**     Please, notify me, if you make any changes to this file            **/
/****************************************************************************/

#include "Coleco.h"
#include "X.h"
#include "Bitmap.h"
#include "Unix.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

/* Title for -help output */
char Title[]="ADAMEm Unix/X 1.81";

char szSnapshotFile[256];         /* Next snapshot file                     */
int  videomode;                   /* 0=1x1  1=2x1                           */
static int makesnap=0;            /* 1 if snapshot should be written        */
static int bpp;			  /* Bits per pixel of the display          */
static Display *Dsp;		  /* Default display                        */
static Window Wnd;		  /* Our window				    */
static Colormap DefaultCMap;	  /* The display's default colours          */
static XImage *Img;		  /* Our image                              */
static GC DefaultGC;		  /* The default graphics context	    */
static int White,Black;		  /* White and black colour values	    */
#ifdef MITSHM			  /* SHM extensions			    */
#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/extensions/XShm.h>
XShmSegmentInfo SHMInfo;
int UseSHM=1;
#endif

/* These functions are used to put a pixel on the image buffer */
typedef void (*PutPixelProcFn) (int offset,int c)	FASTCALL;
static void PutPixel32_0 (int offset,int c)       FASTCALL;
static void PutPixel16_0 (int offset,int c)       FASTCALL;
static void PutPixel8_0  (int offset,int c)       FASTCALL;
static void PutPixel32_1 (int offset,int c)       FASTCALL;
static void PutPixel16_1 (int offset,int c)       FASTCALL;
static void PutPixel8_1  (int offset,int c)       FASTCALL;
static void PutPixel32_2 (int offset,int c)       FASTCALL;
static void PutPixel16_2 (int offset,int c)       FASTCALL;
static void PutPixel8_2  (int offset,int c)       FASTCALL;
static void (*PutPixelProc)(int offset,int c)   FASTCALL;
#define PutPixel(P,C)   (*PutPixelProc)(P,C)

static PutPixelProcFn PutPixelProcTable[][3]=
{
 { PutPixel8_0, PutPixel16_0, PutPixel32_0 },
 { PutPixel8_1, PutPixel16_1, PutPixel32_1 },
 { PutPixel8_2, PutPixel16_2, PutPixel32_2 }
};

/* Key mapping */
static int KEY_LEFT,KEY_RIGHT,KEY_UP,KEY_DOWN,
           KEY_BUTTONA,KEY_BUTTONB,KEY_BUTTONC,KEY_BUTTOND;

static int PalBuf[16],Pal0;       /* Palette buffer                         */
static int default_mouse_sens=200;/* Default mouse sensitivity              */
static int keyboardmode;          /* 0=joystick, 1=keyboard                 */
static word MouseJoyState[2];     /* Current mouse status                   */
static word JoystickJoyState[2];  /* Current joystick status                */
static int OldTimer=0;            /* Last value of timer                    */
static int NewTimer=0;            /* New value of timer                     */
static int calloptions=0;         /* If 1, OptionsDialogue() is called      */
static int width,height;	  /* width & height of the window           */

byte *DisplayBuf;                 /* Screen buffer                          */
char szJoystickFileName[256];     /* File holding joystick information      */
char *szKeys="1511531521540201E31E107A";
				  /* Key scancodes                          */
int  mouse_sens=200;              /* Mouse sensitivity                      */
int  keypadmode=0;                /* 1 if keypad should be reversed         */
int  joystick=1;                  /* Joystick support                       */
int  calibrate=0;                 /* Set to 1 to force joystick calibration */
int  swapbuttons=0;               /* 1=joystick, 2=keyboard, 4=mouse        */
int  expansionmode=0;             /* Expansion module emulated              */
int  syncemu=1;                   /* 0 if emulation shouldn't be synced     */
int  SaveCPU=1;                   /* If 1, save CPU when focus is out       */

static void PutImage (void)
{
#ifdef MITSHM
 if (UseSHM) XShmPutImage (Dsp,Wnd,DefaultGC,Img,0,0,0,0,width,height,False);
 else
#endif
 XPutImage (Dsp,Wnd,DefaultGC,Img,0,0,0,0,width,height);
 XFlush (Dsp);
}

/****************************************************************************/
/** Keyboard routines                                                      **/
/****************************************************************************/
static int PausePressed=0;
static byte keybstatus[NR_KEYS];

static byte keyboard_buffer[16];
static int keyboard_buffer_count=0;
static void LocalAddToKeyboardBuffer (byte ch)
{
 keyboard_buffer[keyboard_buffer_count]=ch;
 keyboard_buffer_count=(keyboard_buffer_count+1)&15;
 keyboard_buffer[keyboard_buffer_count]=0;
}
static int LocalGetKeyboardChar (void)
{
 int retval;
 keyboard_buffer_count=(keyboard_buffer_count-1)&15;
 retval=keyboard_buffer[keyboard_buffer_count];
 keyboard_buffer[keyboard_buffer_count]=0;
 return retval;
}

static const byte scan2ascii[NR_KEYS] =
{
 0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,	/* 00 */
 0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,
 0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,	/* 10 */
 0x18,0x19,0x1A,0x1B,0x1C,0x1D,0x1E,0x1F,
 0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,	/* 20 */
 0x28,0x29,0x2A,0x2B,0x2C,0x2D,0x2E,0x2F,
 0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,	/* 30 */
 0x38,0x39,0x3A,0x3B,0x3C,0x3D,0x3E,0x3F,
 0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,	/* 40 */
 0x48,0x49,0x4A,0x4B,0x4C,0x4D,0x4E,0x4F,
 0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,	/* 50 */
 0x58,0x59,0x5A,0x5B,0x5C,0x5D,0x5E,0x5F,
 0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67,	/* 60 */
 0x68,0x69,0x6A,0x6B,0x6C,0x6D,0x6E,0x6F,
 0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,	/* 70 */
 0x78,0x79,0x7A,0x7B,0x7C,0x7D,0x7E,0x7F,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* 80 */
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* 90 */
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* A0 */
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* B0 */
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* C0 */
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* D0 */
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* E0 */
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* F0 */
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* FF00 */
    8,   9,0x00,0x00,0x00,  13,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* FF10 */
 0x00,0x00,0x00,  27,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* FF20 */
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* FF30 */
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* FF40 */
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  146, 163, 160, 161, 162, 150, 149, 147,	/* FF50 */
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00, 148,0x00,0x00,0x00,0x00,	/* FF60 */
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* FF70 */
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* FF80 */
 0x00,0x00,0x00,0x00,0x00,  13,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* FF90 */
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* FFA0 */
 0x00,0x00, '*', '+',0x00, '-', '.', '/',
  '0', 170, 162, 169, 163, 128, 161, 171,	/* FFB0 */
  160, 168,0x00,0x00,0x00,0x00, 129, 130,
  131, 132, 133, 134, 144, 145,0x00,0x00,	/* FFC0 */
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* FFD0 */
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* FFE0 */
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* FFF0 */
 0x00,0x00,0x00,0x00,0x00,0x00,0x00, 151
};
static const byte scan2ascii_shift[NR_KEYS] =
{
 0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,	/* 00 */
 0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,
 0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,	/* 10 */
 0x18,0x19,0x1A,0x1B,0x1C,0x1D,0x1E,0x1F,
 0x20,0x21,0x22,0x23,0x24,0x25,0x26,  34,	/* 20 */
 0x28,0x29,0x2A,0x2B, '<', '_', '>', '?',
  ')', '!', '@', '#', '$', '%', '^', '&',	/* 30 */
  '*', '(',0x3A, ':',0x3C, '+',0x3E,0x3F,
 0x40, 'a', 'b', 'c', 'd', 'e', 'f', 'g',	/* 40 */
  'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',
  'p', 'q', 'r', 's', 't', 'u', 'v', 'w',	/* 50 */
  'x', 'y', 'z', '{', '|', '}',0x5E,0x5F,
  '~', 'A', 'B', 'C', 'D', 'E', 'F', 'G',	/* 60 */
  'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
  'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W',	/* 70 */
  'X', 'Y', 'Z',0x7B,0x7C,0x7D,0x7E,0x7F,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* 80 */
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* 90 */
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* A0 */
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* B0 */
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* C0 */
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* D0 */
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* E0 */
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* F0 */
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* FF00 */
  184, 185,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* FF10 */
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* FF20 */
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* FF30 */
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* FF40 */
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  154,0x00,0x00,0x00,0x00, 158, 157, 155,	/* FF50 */
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00, 156,0x00,0x00,0x00,0x00,	/* FF60 */
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* FF70 */
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* FF80 */
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* FF90 */
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* FFA0 */
 0x00,0x00, '*', '+',0x00, '-', '.', '/',
  '0',0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* FFB0 */
 0x00,0x00,0x00,0x00,0x00,0x00, 137, 138,
  139, 140, 141, 142, 152, 153,0x00,0x00,	/* FFC0 */
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* FFD0 */
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* FFE0 */
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* FFF0 */
 0x00,0x00,0x00,0x00,0x00,0x00,0x00, 159
};
static const byte scan2ascii_ctrl[NR_KEYS] =
{
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* 00 */
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* 10 */
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* 20 */
 0x00,0x00,0x00,0x00,0x00,  31,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,  30,0x00,	/* 30 */
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,   1,   2,   3,   4,   5,   6,   7,	/* 40 */
    8,   9,  10,  11,  12,  13,  14,  15,
   16,  17,  18,  19,  20,  21,  22,  23,	/* 50 */
   24,  25,  26,  27,  28,  29,  30,  31,
 0x00,   1,   2,   3,   4,   5,   6,   7,	/* 60 */
    8,   9,  10,  11,  12,  13,  14,  15,
   16,  17,  18,  19,  20,  21,  22,  23,	/* 70 */
   24,  25,  26,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* 80 */
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* 90 */
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* A0 */
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* B0 */
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* C0 */
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* D0 */
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* E0 */
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* F0 */
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* FF00 */
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* FF10 */
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* FF20 */
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* FF30 */
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* FF40 */
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00, 167, 164, 165, 166,0x00,0x00,0x00,	/* FF50 */
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* FF60 */
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* FF70 */
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* FF80 */
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* FF90 */
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* FFA0 */
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00, 166,0x00, 167,0x00, 165,0x00,	/* FFB0 */
  164,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* FFC0 */
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* FFD0 */
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* FFE0 */
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* FFF0 */
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

static inline int AltPressed (void)
{
 return keybstatus[SCANCODE_LEFTALT] || keybstatus[SCANCODE_RIGHTALT];
}
static inline int CtrlPressed (void)
{
 return keybstatus[SCANCODE_LEFTCONTROL] || keybstatus[SCANCODE_RIGHTCONTROL];
}
static void keyb_handler (int code,int newstatus)
{
 if (code<0 || code>NR_KEYS)
  return;
 if (newstatus) newstatus=1;
 if (!newstatus)
  keybstatus[code]=0;
 else
 {
  if (!keybstatus[code])
  {
   keybstatus[code]=1;
   if (CtrlPressed() &&
       (code==SCANCODE_F11 || code==SCANCODE_F12))
   {
    if (code==SCANCODE_F11)
     PausePressed=2;
    else
     PausePressed=1;
   }
   else
    PausePressed=0;
   switch (code)
   {
    case SCANCODE_INSERT:
     if (!keyboardmode || AltPressed())
      joystick=1;
     break;
    case SCANCODE_HOME:
     if (!keyboardmode || AltPressed())
      joystick=2;
     break;
    case SCANCODE_PAGEUP:
     if (!keyboardmode || AltPressed())
      joystick=3;
     break;
    case SCANCODE_REMOVE:
     if (!keyboardmode || AltPressed())
      swapbuttons^=1;
     break;
    case SCANCODE_END:
     if (!keyboardmode || AltPressed())
      swapbuttons^=2;
     break;
    case SCANCODE_PAGEDOWN:
     if (!keyboardmode || AltPressed())
      swapbuttons^=4;
     break;
    case SCANCODE_F1:
     if (!keyboardmode || AltPressed())
      ToggleSoundChannel (0);
     break;
    case SCANCODE_F2:
     if (!keyboardmode || AltPressed())
      ToggleSoundChannel (1);
     break;
    case SCANCODE_F3:
     if (!keyboardmode || AltPressed())
      ToggleSoundChannel (2);
     break;
    case SCANCODE_F4:
     if (!keyboardmode || AltPressed())
      ToggleSoundChannel (3);
     break;
    case SCANCODE_F5:
     if (!keyboardmode || AltPressed())
      ToggleSound ();
     break;
    case SCANCODE_F8:
     if (!keyboardmode || AltPressed())
      makesnap=1;
     break;
    case SCANCODE_F11:
     if (!PausePressed && !AltPressed()) DecreaseSoundVolume ();
     break;
    case SCANCODE_F12:
     if (!PausePressed && !AltPressed()) IncreaseSoundVolume ();
     break;
    case SCANCODE_F10:
     Z80_Running=0;
     break;
    case SCANCODE_F9:
#ifdef DEBUG
     if (keybstatus[SCANCODE_LEFTSHIFT] || keybstatus[SCANCODE_RIGHTSHIFT])
     {
      Trace=!Trace;
      break;
     }
#endif
     if (EmuMode)
      if (CtrlPressed())
       calloptions=1;
      else
       keyboardmode=(keyboardmode)? 0:1;
     break;
   }
  }
  if (keyboardmode && !AltPressed())  /* Modify keyboard buffer           */
  {
   /* Check for HOME+Cursor key and Cursor key combinations */
   if (code==SCANCODE_KEYPAD5)
   {
    if (keybstatus[SCANCODE_CURSORLEFT] ||
        keybstatus[SCANCODE_CURSORBLOCKLEFT])
    { code=175; goto put_char_in_buf; }
    if (keybstatus[SCANCODE_CURSORRIGHT] ||
        keybstatus[SCANCODE_CURSORBLOCKRIGHT])
    { code=173; goto put_char_in_buf; }
    if (keybstatus[SCANCODE_CURSORUP] ||
        keybstatus[SCANCODE_CURSORBLOCKUP])
    { code=172; goto put_char_in_buf; }
    if (keybstatus[SCANCODE_CURSORDOWN] ||
        keybstatus[SCANCODE_CURSORBLOCKDOWN])
    { code=170; goto put_char_in_buf; }
   }
   if (keybstatus[SCANCODE_KEYPAD5])
   {
    if (code==SCANCODE_CURSORLEFT) { code=175; goto put_char_in_buf; }
    if (code==SCANCODE_CURSORRIGHT) { code=173; goto put_char_in_buf; }
    if (code==SCANCODE_CURSORUP) { code=172; goto put_char_in_buf; }
    if (code==SCANCODE_CURSORDOWN) { code=170; goto put_char_in_buf; }
   }
   if (code==SCANCODE_CURSORUP || code==SCANCODE_CURSORBLOCKUP)
   {
    if (keybstatus[SCANCODE_CURSORRIGHT] ||
        keybstatus[SCANCODE_CURSORBLOCKRIGHT])
    { code=168; goto put_char_in_buf; }
    if (keybstatus[SCANCODE_CURSORLEFT] ||
        keybstatus[SCANCODE_CURSORBLOCKLEFT])
    { code=171; goto put_char_in_buf; }
   }
   if (code==SCANCODE_CURSORRIGHT || code==SCANCODE_CURSORBLOCKRIGHT)
   {
    if (keybstatus[SCANCODE_CURSORUP] ||
        keybstatus[SCANCODE_CURSORBLOCKUP])
    { code=168; goto put_char_in_buf; }
    if (keybstatus[SCANCODE_CURSORDOWN] ||
        keybstatus[SCANCODE_CURSORBLOCKDOWN])
    { code=169; goto put_char_in_buf; }
   }
   if (code==SCANCODE_CURSORDOWN || code==SCANCODE_CURSORBLOCKDOWN)
   {
    if (keybstatus[SCANCODE_CURSORRIGHT] ||
        keybstatus[SCANCODE_CURSORBLOCKRIGHT])
    { code=169; goto put_char_in_buf; }
    if (keybstatus[SCANCODE_CURSORLEFT] ||
        keybstatus[SCANCODE_CURSORBLOCKLEFT])
    { code=170; goto put_char_in_buf; }
   }
   if (code==SCANCODE_CURSORLEFT)
   {
    if (keybstatus[SCANCODE_CURSORUP] ||
        keybstatus[SCANCODE_CURSORBLOCKUP])
    { code=171; goto put_char_in_buf; }
    if (keybstatus[SCANCODE_CURSORDOWN] ||
        keybstatus[SCANCODE_CURSORBLOCKDOWN])
    { code=170; goto put_char_in_buf; }
   }
   if (keybstatus[SCANCODE_LEFTCONTROL] ||
       keybstatus[SCANCODE_RIGHTCONTROL])
    code=scan2ascii_ctrl[code];
   else if (keybstatus[SCANCODE_LEFTSHIFT] ||
            keybstatus[SCANCODE_RIGHTSHIFT])
    code=scan2ascii_shift[code];
   else
    code=scan2ascii[code];
put_char_in_buf:
   if (keybstatus[SCANCODE_CAPSLOCK])
   {
    if (code>='a' && code<='z') code+='A'-'a';
    else if (code>='A' && code<='Z') code+='a'-'A';
   }
   if (code) LocalAddToKeyboardBuffer (code);
  }
 }
}

/* Look for keyboard events and pass them to the keyboard handler */
static void keyboard_update (void)
{
 XEvent E;
 int i;
 while (XCheckWindowEvent(Dsp,Wnd,KeyPressMask|KeyReleaseMask,&E))
 {
  i=XLookupKeysym ((XKeyEvent*)&E,0);
  if ((i&0xFFFFFF00)==0 || (i&0xFFFFFF00)==0xFF00)
   keyb_handler (i&511,E.type==KeyPress);
 }
}

/****************************************************************************/
/** Mouse routines                                                         **/
/****************************************************************************/
static int mouse_buttons=0;       /* mouse buttons pressed                  */
static int mouse_xpos=500;        /* horizontal position (0 - 1000)         */
static int mouse_ypos=500;        /* vertical position (0 - 1000)           */
static int mouse_x=0;             /* horizontal position (-500 - 500)       */
static int mouse_y=0;             /* vertical position (-500 - 500)         */
static int got_mouse=0;           /* 1 if mouse was properly initialised    */

static void Mouse_SetJoyState ()
{
 if (mouse_buttons&1)
  MouseJoyState[0]&=0xBFFF;
 if (mouse_buttons&2)
  MouseJoyState[0]&=0xFFBF;
 if (mouse_buttons&4)
  MouseJoyState[1]&=0xBFFF;
 if (mouse_buttons&8)
  MouseJoyState[1]&=0xFFBF;
 SpinnerPosition[0]=(default_mouse_sens*mouse_x*(abs(mouse_x)+mouse_sens))/
                    (mouse_sens*mouse_sens);
 SpinnerPosition[1]=(default_mouse_sens*mouse_y*(abs(mouse_y)+mouse_sens))/
                    (mouse_sens*mouse_sens);
}

static void Mouse_GetPosition (void)
{
}

static void Mouse_SetPosition (int x,int y)
{
}

static void Mouse_SetHorizontalRange (int x,int y)
{
}

static void Mouse_SetVerticalRange (int x,int y)
{
}

static void Mouse_Check (void)
{
 int tmp;
 if (!got_mouse)
  return;
 Mouse_GetPosition ();
 mouse_x=mouse_xpos-500;
 mouse_y=mouse_ypos-500;
 tmp=mouse_buttons;
 if (mouse_x || mouse_y)
  Mouse_SetPosition (500,500);
 switch (expansionmode)
 {
  case 4:                         /* emulate driving module                 */
   if (mouse_buttons&7)
    MouseJoyState[0]&=0xFEFF;
   mouse_x/=4;
   mouse_y=0;
   mouse_buttons=0;
   break;
  case 5:                         /* emulate SA speed roller on both ports  */
   mouse_x/=4;
   mouse_y=mouse_x;
   mouse_buttons=0;
   break;
  case 6:                         /* emulate SA speed roller on port 1      */
   mouse_x/=4;
   mouse_y=0;
   mouse_buttons=0;
   break;
  case 7:                         /* emulate SA speed roller on port 2      */
   mouse_y=mouse_x/4;
   mouse_x=0;
   mouse_buttons=0;
   break;
 }
 if (swapbuttons&4)
  mouse_buttons=(mouse_buttons&(~3))|
                ((mouse_buttons&1)<<1)|
                ((mouse_buttons&2)>>1);
 Mouse_SetJoyState ();
 mouse_buttons=tmp;
}

static int Mouse_Detect (void)
{
 if (Verbose) puts ("  Detecting mouse... "
                    "Sorry, mouse support is not implemented yet");
 return 0;
}

static void Mouse_Init (void)
{
 if (!got_mouse)
 Mouse_SetHorizontalRange (0,1000);
 Mouse_SetVerticalRange (0,1000);
 Mouse_SetPosition (500,500);
 return;
}

static void Mouse_Exit (void)
{
}

/****************************************************************************/
/** Joystick routines                                                      **/
/****************************************************************************/
static int gotjoy=0;              /* 1 if joystick was properly initialised */
static joypos_t joycentre;        /* joystick centre position               */
static joypos_t joymin;           /* left-upper corner position             */
static joypos_t joymax;           /* right-lower corner position            */
static joypos_t joy_lowmargin;    /* start of 'dead' region                 */
static joypos_t joy_highmargin;   /* end of 'dead' region                   */

static int Joy_Init (void)
{
 if (!InitJoystick(1))
  return 0;
 ReadJoystick (&joycentre);
 gotjoy=1;
 return 1;
}

static void CalibrateJoystick (void)
{
 FILE *joyfile=NULL;
 if (!calibrate)
  joyfile=fopen(szJoystickFileName,"rb");
 if (!joyfile)
 {
  fprintf (stderr,"Move joystick to top left and press enter\n");
  fflush (stdin); fgetc (stdin);
  ReadJoystick (&joymin);
  fprintf (stderr,"Move joystick to bottom right and press enter\n");
  fflush (stdin); fgetc (stdin);
  ReadJoystick (&joymax);
  joyfile=fopen(szJoystickFileName,"wb");
  if (joyfile)
  {
   fwrite (&joymin,sizeof(joymin),1,joyfile);
   fwrite (&joymax,sizeof(joymax),1,joyfile);
   fclose (joyfile);
  }
 }
 else
 {
  fread (&joymin,sizeof(joymin),1,joyfile);
  fread (&joymax,sizeof(joymax),1,joyfile);
  fclose (joyfile);
 }
 joy_lowmargin.x=joycentre.x-(joycentre.x-joymin.x)/8;
 joy_lowmargin.y=joycentre.y-(joycentre.y-joymin.y)/8;
 joy_highmargin.x=joycentre.x+(joymax.x-joycentre.x)/8;
 joy_highmargin.y=joycentre.y+(joymax.y-joycentre.y)/8;
 /* prevent a divide by zero in JoySortOutAnalogue() */
 if (joy_lowmargin.x-joymin.x==0)
  joy_lowmargin.x++;
 if (joymax.x-joy_highmargin.x==0)
  joy_highmargin.x--;
 if (joy_lowmargin.y-joymin.y==0)
  joy_lowmargin.y++;
 if (joymax.y-joy_highmargin.y==0)
  joy_highmargin.y--;
}

#define JOY_RANGE       128
static void JoySortOutAnalogue (joypos_t *jp)
{
 if (jp->x < joymin.x)
  jp->x=-JOY_RANGE;
 else
  if (jp->x > joymax.x)
   jp->x=JOY_RANGE;
  else
   if ((jp->x > joy_lowmargin.x) && (jp->x < joy_highmargin.x))
    jp->x=0;
   else
    if (jp->x < joy_lowmargin.x)
     jp->x=-JOY_RANGE+(jp->x-joymin.x)*JOY_RANGE/(joy_lowmargin.x-joymin.x);
    else
     jp->x=JOY_RANGE-(joymax.x-jp->x)*JOY_RANGE/(joymax.x-joy_highmargin.x);

 if (jp->y < joymin.y)
  jp->y=-JOY_RANGE;
 else
  if (jp->y > joymax.y)
   jp->y=JOY_RANGE;
  else
   if ((jp->y > joy_lowmargin.y) && (jp->y < joy_highmargin.y))
    jp->y=0;
   else
    if (jp->y < joy_lowmargin.y)
     jp->y=-JOY_RANGE+(jp->y-joymin.y)*JOY_RANGE/(joy_lowmargin.y-joymin.y);
    else
     jp->y=JOY_RANGE-(joymax.y-jp->y)*JOY_RANGE/(joymax.y-joy_highmargin.y);
}

static void Joy_Check (void)
{
 joypos_t jp;
 if (!gotjoy)
  return;
 ReadJoystick (&jp);
 switch (expansionmode)
 {
  case 0:
  case 5:
  case 6:
  case 7:
   if (jp.x<(joycentre.x*3/4))
    JoystickJoyState[0]&=0xF7FF;
   else
    if (jp.x>(joycentre.x*5/4))
     JoystickJoyState[0]&=0xFDFF;
   if (jp.y<(joycentre.y*3/4))
    JoystickJoyState[0]&=0xFEFF;
   else
    if (jp.y>(joycentre.y*5/4))
     JoystickJoyState[0]&=0xFBFF;
   if (jp.buttons&1)
    JoystickJoyState[0]&=(swapbuttons&1)? 0xFFBF:0xBFFF;
   if (jp.buttons&2) 
    JoystickJoyState[0]&=(swapbuttons&1)? 0xBFFF:0xFFBF;
   if (jp.buttons&4)
    JoystickJoyState[0]=(JoyState[0]&0xFFF0)|12;
   if (jp.buttons&8)
    JoystickJoyState[0]=(JoyState[0]&0xFFF0)|13;
   break;
  case 2:
   JoySortOutAnalogue (&jp);
   mouse_x=jp.x;
   mouse_y=jp.y;
   mouse_buttons=0;
   if (jp.buttons&1)
    mouse_buttons|=(swapbuttons&1)? 2:1;
   if (jp.buttons&2) 
    mouse_buttons|=(swapbuttons&1)? 1:2;
   if (jp.buttons&4)
    mouse_buttons|=4;
   if (jp.buttons&8)
    mouse_buttons|=8;
   Mouse_SetJoyState ();
   break;
  case 3:
   if (jp.y<(joycentre.y*3/4))
    JoystickJoyState[0]&=0xFEFF;
   JoySortOutAnalogue (&jp);
   mouse_x=(jp.x/2);
   mouse_y=0;
   Mouse_SetJoyState ();
   if (jp.buttons&1)
    JoystickJoyState[1]&=(swapbuttons&1)? 0xFBFF:0xFEFF;
   if (jp.buttons&2) 
    JoystickJoyState[1]&=(swapbuttons&1)? 0xFEFF:0xFBFF;
   if (jp.buttons&4)
    JoystickJoyState[1]&=0xF7FF;
   if (jp.buttons&8)
    JoystickJoyState[1]&=0xFDFF;
   break;
 }
}

/****************************************************************************/
/** Deallocate all resources taken by InitMachine()                        **/
/****************************************************************************/
void TrashMachine(void)
{
 Mouse_Exit ();
 TrashSound ();
 if (Dsp && Wnd)
 {
#ifdef MITSHM
  if (UseSHM)
  {
   XShmDetach (Dsp,&SHMInfo);
   if (SHMInfo.shmaddr) shmdt (SHMInfo.shmaddr);
   if (SHMInfo.shmid>=0) shmctl (SHMInfo.shmid,IPC_RMID,0);
  }
  else
#endif
  if (Img) XDestroyImage (Img);
 }
 if (Dsp) XCloseDisplay (Dsp);
}

static unsigned SwapBytes (unsigned val,unsigned depth)
{
 if (depth==8)
  return val;
 if (depth==16)
  return ((val>>8)&0xFF)+((val<<8)&0xFF00);
 return ((val>>24)&0xFF)+((val>>8)&0xFF00)+
        ((val<<8)&0xFF0000)+((val<<24)&0xFF000000);
}

static int NextFile (char *s)
{
 char *p;
 p=s+strlen(s)-1;
 if (*p=='9')
 {
  *p='0';
  --p;
  if (*p=='9')
  {
   (*p)++;
   if (*p=='0')
    return 0;
  }
  else
   (*p)++;
 }
 else
  (*p)++;
 return 1;
}
static int NextSnapshotFile (void)
{
 return NextFile (szSnapshotFile);
}

/****************************************************************************/
/** Allocate resources needed by X-Windows-dependent code                  **/
/****************************************************************************/
int InitMachine(void)
{
 int i;
 Screen *Scr;
 XSizeHints Hints;
 XWMHints WMHints;
 XColor Colour;
 FILE *snapshotfile;
 width=WIDTH; height=HEIGHT;
 switch (videomode)
 {
  case 1:  width*=2;
           break;
  case 2:  width*=2;
  	   height*=2;
  	   break;
  default: videomode=0;
 }
 if (Verbose) printf ("Initialising Unix/X drivers...\n");
 if (Verbose) printf ("  Opening display... ");
 Dsp=XOpenDisplay (NULL);
 if (!Dsp)
 {
  if (Verbose) printf ("FAILED\n");
  return 0;
 }
 Scr=DefaultScreenOfDisplay (Dsp);
 White=WhitePixelOfScreen (Scr);
 Black=BlackPixelOfScreen (Scr);
 DefaultGC=DefaultGCOfScreen (Scr);
 DefaultCMap=DefaultColormapOfScreen (Scr);
 bpp=DefaultDepthOfScreen (Scr);
 if (bpp!=8 && bpp!=16 && bpp!=32)
 {
  printf ("FAILED - Only 8,16 and 32 bpp displays are supported\n");
  return 0;
 }
 if (bpp==32 && sizeof(unsigned)!=4)
 {
  printf ("FAILED - 32 bpp displays are only supported on 32 bit machines\n");
  return 0;
 }
 if (Verbose) printf ("OK\n  Opening window... ");
 Wnd=XCreateSimpleWindow (Dsp,RootWindowOfScreen(Scr),
 			  0,0,width,height,0,White,Black);
 if (!Wnd)
 {
  if (Verbose) printf ("FAILED\n");
  return 0;
 }
 Hints.flags=PSize|PMinSize|PMaxSize;
 Hints.min_width=Hints.max_width=Hints.base_width=width;
 Hints.min_height=Hints.max_height=Hints.base_height=height;
 WMHints.input=True;
 WMHints.flags=InputHint;
 XSetWMHints (Dsp,Wnd,&WMHints);
 XSetWMNormalHints (Dsp,Wnd,&Hints);
 XStoreName (Dsp,Wnd,Title);
 XSelectInput (Dsp,Wnd,FocusChangeMask|ExposureMask|KeyPressMask|KeyReleaseMask);
 XMapRaised (Dsp,Wnd);
 XClearWindow (Dsp,Wnd);
 if (Verbose) printf ("OK\n");
#ifdef MITSHM
 if (UseSHM)
 {
  if (Verbose) printf ("  Using shared memory:\n    Creating image... ");
  Img=XShmCreateImage (Dsp,DefaultVisualOfScreen(Scr),bpp,
                       ZPixmap,NULL,&SHMInfo,width,height);
  if (!Img)
  {
   if (Verbose) printf ("FAILED\n");
   return 0;
  }
  if (Verbose) printf ("OK\n    Getting SHM info... ");
  SHMInfo.shmid=shmget (IPC_PRIVATE,Img->bytes_per_line*Img->height,
  			IPC_CREAT|0777);
  if (SHMInfo.shmid<0)
  {
   if (Verbose) printf ("FAILED\n");
   return 0;
  }
  if (Verbose) printf ("OK\n    Allocating SHM... ");
  Img->data=SHMInfo.shmaddr=shmat(SHMInfo.shmid,0,0);
  DisplayBuf=Img->data;
  if (!DisplayBuf)
  {
   if (Verbose) printf ("FAILED\n");
   return 0;
  }
  SHMInfo.readOnly=False;
  if (Verbose) printf ("OK\n    Attaching SHM... ");
  if (!XShmAttach(Dsp,&SHMInfo))
  {
   if (Verbose) printf ("FAILED\n");
   return 0;
  }
 }
 else
#endif
 {
  if (Verbose) printf ("  Allocating screen buffer... ");
  DisplayBuf=malloc(bpp*width*height/8);
  if (!DisplayBuf)
  {
   if (Verbose) printf ("FAILED\n");
   return 0;
  }
  if (Verbose) printf ("OK\n  Creating image... ");
  Img=XCreateImage (Dsp,DefaultVisualOfScreen(Scr),bpp,ZPixmap,
  		    0,DisplayBuf,width,height,8,0);
  if (!Img)
  {
   if (Verbose) printf ("FAILED\n");
   return 0;
  }
 }
 switch (bpp)
 {
  case 8:  PutPixelProc=PutPixelProcTable[videomode][0];
           break;
  case 16: PutPixelProc=PutPixelProcTable[videomode][1];
           break;
  case 32: PutPixelProc=PutPixelProcTable[videomode][2];
           break;
 }
 if (Verbose) printf ("OK\n  Allocating colours... ");
 for (i=0;i<16;++i)
 {
  if (Coleco_Palette[i*3+0]==0 &&
      Coleco_Palette[i*3+1]==0 &&
      Coleco_Palette[i*3+2]==0)
   PalBuf[i]=Black;
  else if (
      Coleco_Palette[i*3+0]==255 && 
      Coleco_Palette[i*3+1]==255 && 
      Coleco_Palette[i*3+2]==255)
   PalBuf[i]=White;
  else
  {
   Colour.flags=DoRed|DoGreen|DoBlue;
   Colour.red=Coleco_Palette[i*3+0]<<8;
   Colour.green=Coleco_Palette[i*3+1]<<8;
   Colour.blue=Coleco_Palette[i*3+2]<<8;
   if (XAllocColor(Dsp,DefaultCMap,&Colour))
#ifdef LSB_FIRST
    if (BitmapBitOrder(Dsp)==LSBFirst)
#else
    if (BitmapBitOrder(Dsp)==MSBFirst)
#endif
     PalBuf[i]=Colour.pixel;
    else
     PalBuf[i]=SwapBytes (Colour.pixel,bpp);
   else
   {
    if (Verbose) puts ("FAILED");
    return 0;
   }
  }
 }
 if (Verbose) puts ("OK");
 while ((snapshotfile=fopen(szSnapshotFile,"rb"))!=NULL)
 {
  fclose (snapshotfile);
  if (!NextSnapshotFile())
   break;
 }
 if (Verbose)
  printf ("  Next snapshot will be %s\n",szSnapshotFile);
 JoyState[0]=JoyState[1]=MouseJoyState[0]=MouseJoyState[1]=
 JoystickJoyState[0]=JoystickJoyState[1]=0x7F7F;
 InitSound ();
 if (expansionmode==2 || expansionmode==3 ||
     (expansionmode!=1 && expansionmode!=4 && joystick))
  Joy_Init ();
 if (expansionmode==1 || expansionmode==4 ||
     expansionmode==5 || expansionmode==6 ||
     expansionmode==7)
  i=Mouse_Detect ();
 if (mouse_sens<=0 || mouse_sens>1000)
  mouse_sens=default_mouse_sens;
 switch (expansionmode)
 {
  case 2:
  case 3:
   if (!gotjoy)
   {
    expansionmode=0;
    break;
   }
   CalibrateJoystick ();
   break;
  case 1:
  case 4:
  case 5:
  case 6:
  case 7:
   if (!got_mouse)
    expansionmode=0;
   break;
  default:
   expansionmode=0;
   default_mouse_sens*=5;
   break;
 }
 if (syncemu)
  OldTimer=ReadTimer ();
 /* Parse keyboard mapping string */
 sscanf (szKeys,"%03X%03X%03X%03X%03X%03X%03X%03X",
         &KEY_LEFT,&KEY_RIGHT,&KEY_UP,&KEY_DOWN,
         &KEY_BUTTONA,&KEY_BUTTONB,&KEY_BUTTONC,&KEY_BUTTOND);
 Mouse_Init ();
 keyboardmode=(EmuMode)? 1:0;
 return 1;
}

/****************************************************************************/
/** Routines to modify the Coleco gamport status                           **/
/****************************************************************************/
static int numkeypressed (int nScanCode)
{
 int nOr;
 switch (nScanCode)
 {
  case SCANCODE_1:
  case SCANCODE_KEYPAD1:
   nOr=1;
   break;
  case SCANCODE_2:
  case SCANCODE_KEYPAD2:
   nOr=2;
   break;
  case SCANCODE_3:
  case SCANCODE_KEYPAD3:
   nOr=3;
   break;
  case SCANCODE_4:
  case SCANCODE_KEYPAD4:
   nOr=4;
   break;
  case SCANCODE_5:
  case SCANCODE_KEYPAD5:
   nOr=5;
   break;
  case SCANCODE_6:
  case SCANCODE_KEYPAD6:
   nOr=6;
   break;
  case SCANCODE_7:
  case SCANCODE_KEYPAD7:
   nOr=7;
   break;
  case SCANCODE_8:
  case SCANCODE_KEYPAD8:
   nOr=8;
   break;
  case SCANCODE_9:
  case SCANCODE_KEYPAD9:
   nOr=9;
   break;
  case SCANCODE_EQUAL:
  case SCANCODE_KEYPADENTER:
   nOr=10;
   break;
  case SCANCODE_MINUS:
  case SCANCODE_KEYPADPERIOD:
   nOr=11;
   break;
  default:
   nOr=0;
 }
 return nOr;
}

static void Joysticks (void)
{
 int i,tmp,tmp2;
 MouseJoyState[0]|=0x7F7F;
 MouseJoyState[1]|=0x7F7F;
 Mouse_Check ();
 JoystickJoyState[0]|=0x7F7F;
 JoystickJoyState[1]|=0x7F7F;
 Joy_Check ();
 JoyState[1]=(MouseJoyState[0] & JoystickJoyState[0]);
 JoyState[0]=(MouseJoyState[1] & JoystickJoyState[1]);
 if (!keyboardmode)
 {
  if (keybstatus[KEY_BUTTONA])
   JoyState[0]&=(swapbuttons&2)? 0xFFBF:0xBFFF;
  if (keybstatus[KEY_BUTTONB])
   JoyState[0]&=(swapbuttons&2)? 0xBFFF:0xFFBF;
  if (keybstatus[KEY_DOWN])
   JoyState[0]&=0xFBFF;
  if (keybstatus[KEY_UP])
   JoyState[0]&=0xFEFF;
  if (keybstatus[KEY_LEFT])
   JoyState[0]&=0xF7FF;
  if (keybstatus[KEY_RIGHT])
   JoyState[0]&=0xFDFF;
  for (i=SCANCODE_0;i<=SCANCODE_9;++i)
   if (keybstatus[i])
   {
    tmp=numkeypressed(i);
    JoyState[0]=(JoyState[0]&0xFFF0)|tmp;
   }
  if (keybstatus[SCANCODE_MINUS])
  {
   tmp=numkeypressed(SCANCODE_MINUS);
   JoyState[0]=(JoyState[0]&0xFFF0)|tmp;
  }
  if (keybstatus[SCANCODE_EQUAL])
  {
   tmp=numkeypressed(SCANCODE_EQUAL);
   JoyState[0]=(JoyState[0]&0xFFF0)|tmp;
  }
  if (keybstatus[KEY_BUTTONC])
   JoyState[0]=(JoyState[0]&0xFFF0)|12;
  if (keybstatus[KEY_BUTTOND])
   JoyState[0]=(JoyState[0]&0xFFF0)|13;
  for (i=SCANCODE_KEYPAD7;i<=SCANCODE_KEYPAD9;++i)
   if (keybstatus[i])
   {
    tmp=numkeypressed((keypadmode)? (i-SCANCODE_KEYPAD7+SCANCODE_KEYPAD1) : i);
    JoyState[1]=(JoyState[1]&0xFFF0)|tmp;
   }
  for (i=SCANCODE_KEYPAD4;i<=SCANCODE_KEYPAD6;++i)
   if (keybstatus[i])
   {
    tmp=numkeypressed(i);
    JoyState[1]=(JoyState[1]&0xFFF0)|tmp;
   }
  for (i=SCANCODE_KEYPAD1;i<=SCANCODE_KEYPAD3;++i)
   if (keybstatus[i])
   {
    tmp=numkeypressed((keypadmode)? (i-SCANCODE_KEYPAD1+SCANCODE_KEYPAD7) : i);
    JoyState[1]=(JoyState[1]&0xFFF0)|tmp;
   }
  if (keybstatus[SCANCODE_KEYPAD0])
  {
   tmp=numkeypressed((keypadmode)? SCANCODE_KEYPADPERIOD : SCANCODE_KEYPAD0);
   JoyState[1]=(JoyState[1]&0xFFF0)|tmp;
  }
  if (keybstatus[SCANCODE_KEYPADPERIOD])
  {
   tmp=numkeypressed((keypadmode)? SCANCODE_KEYPAD0 : SCANCODE_KEYPADPERIOD);
   JoyState[1]=(JoyState[1]&0xFFF0)|tmp;
  }
  if (keybstatus[SCANCODE_KEYPADENTER])
  {
   tmp=numkeypressed(SCANCODE_KEYPADENTER);
   JoyState[1]=(JoyState[1]&0xFFF0)|tmp;
  }
  if (keybstatus[SCANCODE_KEYPADPLUS])
   JoyState[1]=(JoyState[1]&0xFFF0)|13;
  if (keybstatus[SCANCODE_KEYPADMINUS])
   JoyState[1]=(JoyState[1]&0xFFF0)|12;
 }
 switch (expansionmode)
 {
  case 1:                         /* emulate roller controller with mouse   */
  case 2:                         /* emulate RC with joystick               */
   if ((JoyState[1]&0x0F)==12 || (JoyState[1]&0x0F)==13)
    JoyState[1]|=0x0F;
   if ((JoyState[0]&0x0F)==12 || (JoyState[0]&0x0F)==13)
    JoyState[0]|=0x0F;
   tmp=JoyState[1];
   JoyState[1]=(JoyState[0]&0x707F)|0x0F00;
   JoyState[0]=(tmp&0x7F7F);
   if ((JoyState[1]&0xF)!=0xF)
    JoyState[0]=(JoyState[0]&0xFFF0)|(JoyState[1]&0x0F);
   JoyState[1]=(JoyState[1]&0xFFF0)|(JoyState[0]&0x0F);
   break;
  case 3:                         /* emulate driving module with joystick   */
  case 4:                         /* emulate driving module with mouse      */
   if ((JoyState[1]&0x0F)==12 || (JoyState[1]&0x0F)==13)
    JoyState[1]|=0x0F;
   if ((JoyState[0]&0x0F)==12 || (JoyState[0]&0x0F)==13)
    JoyState[0]|=0x0F;
   if ((JoyState[1]&0xF)!=0xF)
    JoyState[0]=(JoyState[0]&0xFFF0)|(JoyState[1]&0x0F);
   if ((JoyState[1]&0x0100)==0)
    JoyState[1]&=0xBFFF;
   else
    JoyState[1]|=0x4000;
   JoyState[1]|=0x0F7F;
   tmp=JoyState[1];
   JoyState[1]=JoyState[0];
   JoyState[0]=tmp|0x0040;
   break;
  default:
   switch (joystick)
   {
    case 1:                       /* Joystick 1=Joystick 2                  */
     tmp=JoyState[0]&0x0F;
     tmp2=JoyState[1]&0x0F;
     if (tmp==12 || tmp==13)
      JoyState[1]=(JoyState[1]&0xFFF0)|(JoyState[0]&0xF);
     else
      if (tmp2==12 || tmp2==13)
       JoyState[0]=(JoyState[0]&0xFFF0)|(JoyState[1]&0xF);
      else
       if (tmp2!=15)
       {
        JoyState[0]=(JoyState[0]&0xFFF0)|(JoyState[1]&0x0F);
        JoyState[1]|=15;
       }
     JoyState[0]&=(JoyState[1]|0xF);
     JoyState[1]&=(JoyState[0]|0xF);
     break;
    case 2:                       /* Joystick 1=keyb, Joystick 2=joystick   */
     break;
    case 3:                       /* Joystick 1=joystick, Joystick 2=keyb   */
     tmp=JoyState[0];
     JoyState[0]=JoyState[1];
     JoyState[1]=tmp;
     break;
   }
   break;
 }
}

/****************************************************************************/
/*** Parse keyboard events                                                ***/
/****************************************************************************/
void Keyboard (void)
{
 int tmp;
 XEvent E;
 keyboard_update ();
 /* Check if reset combination is pressed */
 if (AltPressed())
 {
  if (keybstatus[SCANCODE_F12]) ResetColeco (0);
  if (keybstatus[SCANCODE_F11]) ResetColeco (1);
 }
 /* Update keyboard buffer */
 do
 {
  tmp=LocalGetKeyboardChar ();
  if (tmp) AddToKeyboardBuffer (tmp);
 }
 while (tmp);
 /* Check mouse and joystick events */
 Joysticks ();
 /* Check is PAUSE is pressed */
 if (PausePressed)
 {
  StopSound ();
  tmp=0;
  while (PausePressed) keyboard_update();
  ResumeSound ();
  if (syncemu)
   OldTimer=ReadTimer ();
 }
 /* If saving CPU and focus is out, sleep */
 tmp=0;
 while (XCheckWindowEvent(Dsp,Wnd,FocusChangeMask,&E))
  tmp=(E.type==FocusOut);
 if(SaveCPU&&tmp)
 {
  StopSound ();
  while(!XCheckWindowEvent(Dsp,Wnd,FocusChangeMask,&E)&&Z80_Running)
  {
   if(XCheckWindowEvent(Dsp,Wnd,ExposureMask,&E)) PutImage();
   XPeekEvent(Dsp,&E);
  }
  ResumeSound ();
  OldTimer=ReadTimer ();
 }
/* Check if a snapshot should be taken */
 if (makesnap)
 {
  SaveSnapshotFile (szSnapshotFile);
  NextSnapshotFile ();
  makesnap--;
 }
 /* Check if OptionsDialogue() should be called */
 if (calloptions)
 {
  calloptions=0;
  StopSound ();
  OptionsDialogue ();
  ResumeSound ();
  if (syncemu) OldTimer=ReadTimer();
 }
}

/****************************************************************************/
/** Interrupt routines                                                     **/
/****************************************************************************/
/* Gets called 50 times per second */
int CheckScreenRefresh (void)
{
 static int skipped=0;
 if (syncemu)
 {
  NewTimer=ReadTimer ();
  OldTimer+=1000000/IFreq;
  if ((OldTimer-NewTimer)>0)
  {
   do
    NewTimer=ReadTimer ();
   while ((NewTimer-OldTimer)<0);
   skipped=0;
   return 1;
  }
  else
   if (++skipped>=UPeriod)
   {
    OldTimer=ReadTimer ();
    skipped=0;
    return 1;
   }
   else
    return 0;
 }
 return 2;
}

static void PutPixel32_0 (int offset,int c)
{
 *(unsigned*)(DisplayBuf+offset*4)=c;
}

static void PutPixel16_0 (int offset,int c)
{
 *(unsigned short*)(DisplayBuf+offset*2)=c;
}

static void PutPixel8_0 (int offset,int c)
{
 DisplayBuf[offset]=c;
}

static void PutPixel32_1 (int offset,int c)
{
 *(unsigned*)(DisplayBuf+offset*4*2)=
 *(unsigned*)(DisplayBuf+offset*4*2+1)=
 c;
}

static void PutPixel16_1 (int offset,int c)
{
 *(unsigned short*)(DisplayBuf+offset*2*2)=
 *(unsigned short*)(DisplayBuf+offset*2*2+1)=
 c;
}

static void PutPixel8_1 (int offset,int c)
{
 DisplayBuf[offset*2]=DisplayBuf[offset*2+1]=c;
}

static void PutPixel32_2 (int offset,int c)
{
 offset=(offset&0x00FF)+((offset&0xFF00)<<1);
 *(unsigned*)(DisplayBuf+offset*4*2)=
 *(unsigned*)(DisplayBuf+offset*4*2+1)=
 c;
}

static void PutPixel16_2 (int offset,int c)
{
 offset=(offset&0x00FF)+((offset&0xFF00)<<1);
 *(unsigned short*)(DisplayBuf+offset*2*2)=
 *(unsigned short*)(DisplayBuf+offset*2*2+1)=
 c;
}

static void PutPixel8_2 (int offset,int c)
{
 offset=(offset&0x00FF)+((offset&0xFF00)<<1);
 DisplayBuf[offset*2]=DisplayBuf[offset*2+1]=c;
}

/****************************************************************************/
/** Screen refresh drivers                                                 **/
/****************************************************************************/
#include "Common.h"
