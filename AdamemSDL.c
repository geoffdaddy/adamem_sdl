/** ADAMEm: Coleco ADAM emulator ********************************************/
/**                                                                        **/
/**                           AdamemSDL.c                                  **/
/**                                                                        **/
/** This file contains the driver for the SDL support. It does not         **/
/** include the sound emulation code                                       **/
/**                                                                        **/
/** Copyright (C) Geoff Oltmans, Dale Wick 2006-2020                       **/
/****************************************************************************/

#include "Coleco.h"
#include "Sound.h"
#include "SDL_endian.h"
#include "AdamSDLSound_2.h"

#include "AdamemSDL.h"

#include "Bitmap.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#include "sms_ntsc.h"

/* Title for -help output */
char Title[]="ADAMEm SDL 2.1g";

char szBitmapFile[256];           /* Next screen shot file                  */
char szSnapshotFile[256];         /* Next snapshot file                     */
int  videomode;                   /* 0=1x1  1=2x1                           */
static int makesnap=0;            /* 1 if snapshot should be written        */
static int makeshot=0;            /* Save a screenshot                      */
SDL_Surface *screen;
SDL_Window *win;
SDL_Texture* sdlTexture;
//SDL_sem* initSem;
//SDL_Thread* eventThread;

Uint32 RGBcolors[16];
static char fullscreen = 0;

/* These functions are used to put a pixel on the image buffer */
SDL_Renderer* sdlRenderer = NULL;
sms_ntsc_t* ntsc = 0;

static void SDLPutPixel  (int offset,int c);
#define PutPixel(P,C)   SDLPutPixel(P,C)

static int ReadTimer (void);

/* Key mapping */
static int KEY_LEFT,KEY_RIGHT,KEY_UP,KEY_DOWN,
   KEY_BUTTONA,KEY_BUTTONB,KEY_BUTTONC,KEY_BUTTOND;


static byte VGA_Palette[16*3];    			/* Coleco Palette                         */
static int PalBuf[16],Pal0;       			/* Palette buffer                         */
static int default_mouse_sens=200;			/* Default mouse sensitivity              */
static int keyboardmode;          			/* 0=joystick, 1=keyboard                 */
static word MouseJoyState[2];     			/* Current mouse status                   */
static word JoystickJoyState[2];  			/* Current joystick status                */
static int OldTimer=0;            			/* Last value of timer                    */
static int NewTimer=0;            			/* New value of timer                     */
static int calloptions=0;         			/* If 1, OptionsDialogue() is called      */
static int width,height;	  	  			/* width & height of the frame buffer     */
static float windowWidth, windowHeight;		/* width & height of the rendering window */

SMS_NTSC_IN_T* DisplayBuf;					/* Screen buffer                          */
char szJoystickFileName[256];               /* File holding joystick information      */

char *szKeys="05004F05205101900601B01D";    /* Key scancodes                          */

int  mouse_sens=200;              			/* Mouse sensitivity                      */
int  keypadmode=0;                			/* 1 if keypad should be reversed         */
int  joystick=1;                  			/* Joystick support                       */
int  calibrate=0;                 			/* Set to 1 to force joystick calibration */
int  swapbuttons=0;               			/* 1=joystick, 2=keyboard, 4=mouse        */
int  expansionmode=0;                 	    /* Expansion module emulated              */
int  syncemu=1;                   			/* 0 if emulation shouldn't be synced     */
int  SaveCPU=1;                   			/* If 1, save CPU when focus is out       */


#ifdef TEXT80
int Text80;                       /* 1 if in Text80 mode                    */
int Text80Colour;                 /* Text80 foreground colour               */
int AutoText80;                   /* 1 if auto-switch to Text80 mode on     */

#endif

#define JOY_KEY1		0xFFF1
#define JOY_KEY2		0xFFF2
#define JOY_KEY3		0xFFF3
#define JOY_KEY4		0xFFF4
#define JOY_KEY5		0xFFF5
#define JOY_KEY6		0xFFF6
#define JOY_KEY7		0xFFF7
#define JOY_KEY8		0xFFF8
#define JOY_KEY9		0xFFF9
#define JOY_KEYSTAR		0xFFFB
#define JOY_KEY0		0xFFF0
#define JOY_KEYPOUND	0xFFFA
#define JOY_AIM			0xFFBF
#define JOY_FIRE		0xBFFF
#define JOY_BUTTON3		0xFFFC
#define JOY_BUTTON4		0xFFFD



static void JoystickEventHandler(SDL_Event *event);

static void PutImage (void)
{
	if (ntsc)
		sms_ntsc_blit( ntsc, DisplayBuf, width, width, height, (unsigned char*) screen->pixels, screen->pitch );

	SDL_UpdateTexture(sdlTexture, NULL, screen->pixels, screen->pitch);
	SDL_RenderClear(sdlRenderer);
	SDL_RenderCopy(sdlRenderer, sdlTexture, NULL, NULL);
	SDL_RenderPresent(sdlRenderer);
}


/****************************************************************************/
/** Keyboard routines                                                      **/
/****************************************************************************/
int PausePressed=0;
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
	0x00,0x00,0x00,0x00,0x61,0x62,0x63,0x64,	/* 00 */
	0x65,0x66,0x67,0x68,0x69,0x6a,0x6b,0x6c,
	0x6d,0x6e,0x6f,0x70,0x71,0x72,0x73,0x74,	/* 10 */
	0x75,0x76,0x77,0x78,0x79,0x7a,0x31,0x32,
	0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x30,	/* 20 */
	0x0d,0x1b,0x08,0x09,0x20,0x2D,0x3d,0x5b,
	0x5d,0x5c,0x5c,0x3b,0x27,0x60,0x2c,0x2e,	/* 30 */
	0x2f,0x5d,0x81,0x82,0x83,0x84,0x85,0x86,
	0x90,0x91,0x00,0x00,0x00,0x00,0x00,0x00,	/* 40 */
	0x00,0x94,0x92,0x96,0x97,0x93,0x95,0xa1,
	0xa3,0xa2,0xa0,0x00,0x00,0x00,0x00,0x00,	/* 50 */
	0x00,0x00,0xa2,0x00,0xa3,0x80,0xa1,0x00,
	0xa8,0x00,0x00,0x00,0x00,0x00,0x00,0x3d,	/* 60 */
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* 70 */
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
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
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* 100 */
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* 110 */
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* 120 */
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* 130 */
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00    							/* 140 */
};

static const byte scan2ascii_shift[NR_KEYS] =
{
	0x00,0x00,0x00,0x00,0x41,0x42,0x43,0x44,	/* 00 */
	0x45,0x46,0x47,0x48,0x49,0x4a,0x4b,0x4c,
	0x4d,0x4e,0x4f,0x50,0x51,0x52,0x53,0x54,	/* 10 */
	0x55,0x56,0x57,0x58,0x59,0x5a,0x21,0x40,
	0x23,0x24,0x25,0x5e,0x26,0x2a,0x28,0x29,	/* 20 */
	0x00,0x1b,0xb8,0xb9,0x20,0x5f,0x2b,0x7b,
	0x7d,0x7c,0x00,0x3A,0x22,0x7e,0x3c,0x3e,	/* 30 */
	0x3f,0x00,0x89,0x8a,0x8b,0x8c,0x8d,0x8c,
	0x98,0x99,0x00,0x00,0x00,0x00,0x00,0x00,	/* 40 */
	0x00,0x9c,0x9a,0x9e,0x9f,0x9b,0x9d,0xa9,
	0xab,0xaa,0xa8,0x00,0x00,0x00,0x00,0x00,	/* 50 */
	0x00,0x00,0xaa,0x00,0xab,0x80,0xa9,0x00,
	0xa8,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* 60 */
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* 70 */
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
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
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* 100 */
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* 110 */
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* 120 */
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* 130 */
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00    							/* 140 */
};

static const byte scan2ascii_ctrl[NR_KEYS] =
{
	0x00,0x00,0x00,0x00,0x01,0x02,0x03,0x04,	/* 00 */
	0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,
	0x0d,0x0e,0x0f,0x10,0x11,0x12,0x13,0x14,	/* 10 */
	0x15,0x16,0x17,0x18,0x19,0x1a,0x31,0x00,
	0x33,0x34,0x35,0x1f,0x37,0x38,0x39,0x30,	/* 20 */
	0x0d,0x1b,0x08,0x09,0x20,0x2d,0x2b,0x1b,
	0x1d,0x1c,0x00,0x3b,0x27,0x2d,0x2c,0x2e,	/* 30 */
	0x2f,0x00,0x81,0x82,0x83,0x84,0x85,0x86,
	0x90,0x91,0x00,0x00,0x00,0x00,0x00,0x00,	/* 40 */
	0x00,0x94,0x92,0x96,0x7f,0x93,0x95,0xa5,
	0xa7,0xa6,0xa4,0x00,0x2f,0x2a,0x2d,0x2b,	/* 50 */
	0x00,0x00,0xa6,0x00,0xa7,0x80,0xa5,0x00,
	0xa4,0x00,0x00,0x00,0x00,0x00,0x00,0x3d,	/* 60 */
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* 70 */
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
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
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* 100 */
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* 110 */
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* 120 */
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* 130 */
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00     							/* 140 */
};

static int AltPressed (void)
{
	return keybstatus[SCANCODE_LEFTALT&0xffff] || keybstatus[SCANCODE_RIGHTALT&0xffff];
}

static int CtrlPressed (void)
{
	return keybstatus[SCANCODE_LEFTCONTROL&0xffff] || keybstatus[SCANCODE_RIGHTCONTROL&0xffff];
}

static int CmdPressed(void)
{
	return keybstatus[SCANCODE_LEFTCMD&0xffff] || keybstatus[SCANCODE_RIGHTCMD&0xffff];
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

			if (CtrlPressed() && (code==SCANCODE_F11 || code==SCANCODE_F12))
			{
				if (code==SCANCODE_F11)
					PausePressed=2;
				else
					PausePressed=1;
				SDL_ShowCursor(SDL_ENABLE);
			}
			else {
				PausePressed=0;
				SDL_ShowCursor(SDL_DISABLE);
			}

			if (AltPressed() && (code==SDL_SCANCODE_RETURN)) {
				setFullScreen(!fullscreen);
			}
			
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
#ifdef SOUND
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
#endif
			case SCANCODE_F8:
				if (!keyboardmode || AltPressed())
					makeshot=1;
			break;
			case SCANCODE_F7:
				if (!keyboardmode || AltPressed())
					makesnap=1;
#ifdef DEBUG
		        if (CtrlPressed())
				{
					Z80_Trace=!Z80_Trace;
				}
#endif
			break;
#ifdef SOUND			
			case SCANCODE_F11:
			 if (!PausePressed && !AltPressed()) DecreaseSoundVolume ();
			 break;
			case SCANCODE_F12:
			 if (!PausePressed && !AltPressed()) IncreaseSoundVolume ();
			 break;
#endif
			case SCANCODE_F10:
				Z80_Running=0;
			break;
			case SCANCODE_F9:
#ifdef DEBUG
				if (keybstatus[SCANCODE_LEFTSHIFT] || keybstatus[SCANCODE_RIGHTSHIFT])
				{
					Z80_Trace=!Z80_Trace;
					break;
				}
#endif
//				if (EmuMode)
//				{
					if (CtrlPressed())
						calloptions=1;
					else
						keyboardmode=(keyboardmode)? 0:1;
//				}
				break;
			}
		}
		if (keyboardmode && !AltPressed())  /* Modify keyboard buffer           */
		{
			/* Check for HOME+Cursor key and Cursor key combinations */
			if (code==SCANCODE_KEYPAD5)
			{
				if (keybstatus[SCANCODE_CURSORLEFT] || keybstatus[SCANCODE_CURSORBLOCKLEFT])
					{ code=175; goto put_char_in_buf; }
				if (keybstatus[SCANCODE_CURSORRIGHT] ||	keybstatus[SCANCODE_CURSORBLOCKRIGHT])
					{ code=173; goto put_char_in_buf; }
				if (keybstatus[SCANCODE_CURSORUP] || keybstatus[SCANCODE_CURSORBLOCKUP])
					{ code=172; goto put_char_in_buf; }
				if (keybstatus[SCANCODE_CURSORDOWN] || keybstatus[SCANCODE_CURSORBLOCKDOWN])
					{ code=174; goto put_char_in_buf; }
			}
			if (keybstatus[SCANCODE_KEYPAD5])
			{
				if (code==SCANCODE_CURSORLEFT || code == SCANCODE_CURSORBLOCKLEFT)
					{ code=175; goto put_char_in_buf; }
				if (code==SCANCODE_CURSORRIGHT || code == SCANCODE_CURSORBLOCKRIGHT)
					{ code=173; goto put_char_in_buf; }
				if (code==SCANCODE_CURSORUP || code == SCANCODE_CURSORBLOCKUP)
					{ code=172; goto put_char_in_buf; }
				if (code==SCANCODE_CURSORDOWN || code == SCANCODE_CURSORBLOCKDOWN)
					{ code=174; goto put_char_in_buf; }
			}
			if (code==SCANCODE_CURSORUP || code==SCANCODE_CURSORBLOCKUP)
			{
				if (keybstatus[SCANCODE_CURSORRIGHT] || keybstatus[SCANCODE_CURSORBLOCKRIGHT])
					{ code=168; goto put_char_in_buf; }
				if (keybstatus[SCANCODE_CURSORLEFT] || keybstatus[SCANCODE_CURSORBLOCKLEFT])
					{ code=171; goto put_char_in_buf; }
			}
			if (code==SCANCODE_CURSORRIGHT || code==SCANCODE_CURSORBLOCKRIGHT)
			{
				if (keybstatus[SCANCODE_CURSORUP] || keybstatus[SCANCODE_CURSORBLOCKUP])
					{ code=168; goto put_char_in_buf; }
				if (keybstatus[SCANCODE_CURSORDOWN] || keybstatus[SCANCODE_CURSORBLOCKDOWN])
					{ code=169; goto put_char_in_buf; }
			}
			if (code==SCANCODE_CURSORDOWN || code==SCANCODE_CURSORBLOCKDOWN)
			{
				if (keybstatus[SCANCODE_CURSORRIGHT] || keybstatus[SCANCODE_CURSORBLOCKRIGHT])
					{ code=169; goto put_char_in_buf; }
				if (keybstatus[SCANCODE_CURSORLEFT] || keybstatus[SCANCODE_CURSORBLOCKLEFT])
					{ code=170; goto put_char_in_buf; }
			}
			if (code==SCANCODE_CURSORLEFT || code==SCANCODE_CURSORBLOCKLEFT)
			{
				if (keybstatus[SCANCODE_CURSORUP] || keybstatus[SCANCODE_CURSORBLOCKUP])
					{ code=171; goto put_char_in_buf; }
				if (keybstatus[SCANCODE_CURSORDOWN] || keybstatus[SCANCODE_CURSORBLOCKDOWN])
					{ code=170; goto put_char_in_buf; }
			}

			if (keybstatus[SCANCODE_LEFTCONTROL] || keybstatus[SCANCODE_RIGHTCONTROL])
				code=scan2ascii_ctrl[code];
			else if (keybstatus[SCANCODE_LEFTSHIFT] || keybstatus[SCANCODE_RIGHTSHIFT])
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
	SDL_Event event;
	//int run = 1;

	//SDL_SemWait(initSem);

	//while (run)
	//{
		while (SDL_PollEvent(&event)) {
			switch (event.type) {

			case SDL_JOYBUTTONDOWN:
			case SDL_JOYBUTTONUP:
			case SDL_JOYAXISMOTION:
				JoystickEventHandler(&event);
				break;
			case SDL_KEYDOWN:
				keyb_handler(event.key.keysym.scancode, 1);
				break;
			case SDL_KEYUP:
				keyb_handler(event.key.keysym.scancode, 0);
				break;
			case SDL_QUIT:
				keyb_handler(SCANCODE_F10, 1);
				//run = 0;
				break;
				//			case SDL_VIDEOEXPOSE :
				//				PutImage();
				break;
			default:
				if (Verbose) printf("Unhandled SDL Event 0x%x\n", event.type);
				fflush(stdout);
				break;
			}
		}
	//}
}

/****************************************************************************/
/** Mouse routines                                                         **/
/****************************************************************************/
static int mouse_buttons=0;       /* mouse buttons pressed                  */
static int mouse_x=0;             /* horizontal position (-500 - 500)       */
static int mouse_y=0;             /* vertical position (-500 - 500)         */
static int mouse_x_global_state = 0;
static int mouse_y_global_state = 0;

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
	unsigned char tmp_buttonstate;
	int mouse_x_last = mouse_x_global_state;
	int mouse_y_last = mouse_y_global_state;

	tmp_buttonstate = SDL_GetGlobalMouseState(&mouse_x_global_state,&mouse_y_global_state);
	mouse_x = ((mouse_x_last-mouse_x_global_state)*2);
	mouse_y = ((mouse_y_last-mouse_y_global_state)*2);
	if (mouse_x > 500)
		mouse_x = 500;
	if (mouse_x < -500)
		mouse_x = -500;
	if (mouse_y > 500)
		mouse_y = 500;
	if (mouse_y < -500)
		mouse_y = -500;

	mouse_buttons = 0;
	if ( tmp_buttonstate&SDL_BUTTON(1) )
	{
		mouse_buttons |= 1;
	}
	if ( tmp_buttonstate&SDL_BUTTON(2) )
	{
		mouse_buttons |= 2;
	}
	if ( tmp_buttonstate&SDL_BUTTON(3) )
	{
		mouse_buttons |= 4;
	}
	if ( tmp_buttonstate&SDL_BUTTON(4) )
	{
		mouse_buttons |= 8;
	}

}

static void Mouse_SetPosition (int x,int y)
{
	if (expansionmode > 3)
	{
//		SDL_WarpMouse(x, y);
		SDL_ShowCursor(SDL_DISABLE);
	}
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
	
	if (PausePressed ) {
		return;
	}

	Mouse_GetPosition ();
	tmp=mouse_buttons;
	if (mouse_x || mouse_y)
		Mouse_SetPosition (250,250);

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
		case 8:							/* emulate spinner on both ports with mouse */
			mouse_x /=4;
			mouse_y /=4;
			mouse_buttons=0;
		break;
	}

	if (swapbuttons&4)
		mouse_buttons=(mouse_buttons&(~3))|((mouse_buttons&1)<<1)|((mouse_buttons&2)>>1);
	Mouse_SetJoyState();
	mouse_buttons=tmp;
}

static void Mouse_Init (void)
{
	//Mouse_SetPosition (250,250);
	//return;
}

/****************************************************************************/
/** Joystick routines                                                      **/
/****************************************************************************/
typedef struct _joystick_states
{
	SDL_Joystick *joy;
	Sint16 x;
	Sint16 y;
	Sint16 center_x;
	Sint16 center_y;
	Uint8 buttons[NR_JOYBUTTONS]; //Just an array of all the buttons on the joystick.
	Uint8 shiftButton; //The index of the joystick button used to shift the keypad (this is the 'aim' button)
	Uint8 shiftComputerReset;
	Uint8 shiftGameReset;
	Uint8 shiftPause;
	Uint8 shiftQuit;
	Uint32 shifttime;
} joystick_states;

static int numjoysticks = 0;
static joystick_states joystates[2];
static Uint16 joystick_convert_array[NR_JOYBUTTONS] = 
{
	JOY_FIRE, //Fire
	JOY_AIM, //Aim
	JOY_BUTTON3, //SAC Button 3
	JOY_BUTTON4,  //SAC Button 4
	0xFFFF, 
	0xFFFF, 
	0xFFFF, 
	0xFFFF, 
	JOY_KEY1, //1
	JOY_KEY2, //2
	JOY_KEY3, //3
	JOY_KEY4, //4
	JOY_KEY5, //5
	JOY_KEY6, //6
	JOY_KEY7, //7
	JOY_KEY8, //8
	JOY_KEY9, //9
	JOY_KEYSTAR, //*
	JOY_KEY0, //0
	JOY_KEYPOUND, //#
	0xFFFF, 
	0xFFFF, 
	0xFFFF, 
	0xFFFF, 
	0xFFFF, 
	0xFFFF, 
	0xFFFF, 
	0xFFFF, 
	0xFFFF, 
	0xFFFF 
};

//Now encode the event into the JoystickJoyState.
//On a real Coleco Vision or ADAM, the joystick state is read with two different 8-bit ports for 
//each of the two joysticks. The first port contains the direction + fire, the other is the 
//keypad matrix and the aim button. These are encoded into a single 16 bit word, JoystickJoyState
//that's used by the emulator when these ports are read by the software. The most significant byte is
//the direction and fire, the least significant is the keyboard matrix and aim.
static void TranslateJoystickStates(uint8_t which)
{
	int i;
	JoystickJoyState[which] |= 0x7f7f;
	
	if (joystates[which].x<(joystates[which].center_x*3/4))
	{
		JoystickJoyState[which]&=0xF7FF;
	}
	else if (joystates[which].x>(joystates[which].center_x*5/4))
	{
		JoystickJoyState[which]&=0xFDFF;
	}
	if (joystates[which].y<(joystates[which].center_y*3/4))
	{
		JoystickJoyState[which]&=0xFEFF;
	}
	else if (joystates[which].y>(joystates[which].center_y*5/4))
	{
		JoystickJoyState[which]&=0xFBFF;
	}
	
	// Translate the buttons using the translation matrix...
	for (i = 0; i < NR_JOYBUTTONS ; i++)
	{
		if (joystates[which].buttons[i]) {
			JoystickJoyState[which] &= joystick_convert_array[i];
		}
	}
}

static void JoystickEventHandler(SDL_Event *event)
{
	uint8_t pressed = 0;
	uint8_t which;
	
	
	switch (event->type) {
		case SDL_JOYBUTTONDOWN:
			pressed = 1;
		case SDL_JOYBUTTONUP:
			if (event->jbutton.which > 1) {
				return;
			}
			which = event->jbutton.which;
			if (event->jbutton.button < NR_JOYBUTTONS)
			{
				joystates[which].buttons[event->jbutton.button] = pressed;
				if (which == 0 && event->jbutton.button == joystates[which].shiftButton)
				{
					joystates[which].shifttime = pressed ? SDL_GetTicks() : 0;
				}
			}
			else
			{
				printf("Button out of range, not decoded.\n");
				return;
			}
			break;
		case SDL_JOYAXISMOTION:
			if (event->jaxis.which > 1)
			{
				return;
			}

			which = event->jaxis.which;

			if (event->jaxis.axis == 0)
			{
				joystates[event->jaxis.which].x = event->jaxis.value/512+64;
			} 
			else 
			{
				joystates[event->jaxis.which].y = event->jaxis.value/512+64;
			}
			break;
		default:
			printf("Unhandled joystick event\n");
			return;
			break;
	}
	
	TranslateJoystickStates(which);
	
}

//Format of file is Joystick button #, Emulated Coleco Button (0-9,*,#,a,f,c,d)
static char ParseJoystickConfigFile(const char *name)
{
	FILE *theFile = NULL;
	char filebuf[20];
	int joyButton;
	char cvButton;
	int i;
	
	theFile = fopen(name, "r");
	
	if (theFile == NULL)
		return -1;
	
	while (fgets(filebuf, sizeof(filebuf), theFile))
	{
		i = sscanf(filebuf, "%d,%c",&joyButton,&cvButton);
		if (joyButton < NR_JOYBUTTONS && i == 2) {
		switch (cvButton) {
			case '1':
				joystick_convert_array[joyButton] = JOY_KEY1;
				break;
			case '2':
				joystick_convert_array[joyButton] = JOY_KEY2;
				break;
			case '3':
				joystick_convert_array[joyButton] = JOY_KEY3;
				break;
			case '4':
				joystick_convert_array[joyButton] = JOY_KEY4;
				break;
			case '5':
				joystick_convert_array[joyButton] = JOY_KEY5;
				break;
			case '6':
				joystick_convert_array[joyButton] = JOY_KEY6;
				break;
			case '7':
				joystick_convert_array[joyButton] = JOY_KEY7;
				break;
			case '8':
				joystick_convert_array[joyButton] = JOY_KEY8;
				break;
			case '9':
				joystick_convert_array[joyButton] = JOY_KEY9;
				break;
			case '*':
				joystick_convert_array[joyButton] = JOY_KEYSTAR;
				break;
			case '0':
				joystick_convert_array[joyButton] = JOY_KEY0;
				break;
			case '#':
				joystick_convert_array[joyButton] = JOY_KEYPOUND;
				break;
			case 'a':
				joystick_convert_array[joyButton] = JOY_AIM;
				break;
			case 'f':
				joystick_convert_array[joyButton] = JOY_FIRE;
				break;
			case 'c':
				joystick_convert_array[joyButton] = JOY_BUTTON3;
				break;
			case 'd':
				joystick_convert_array[joyButton] = JOY_BUTTON4;
				break;
			case 'q': // denote the quit button
				joystates[0].shiftQuit = joyButton;
				break;
			case 'p': // denote the computer pause button
				joystates[0].shiftPause = joyButton;
				break;
			case 'r': // denote the computer reset button
				joystates[0].shiftComputerReset = joyButton;
				break;
			case 'g': // denote the game reset button
				joystates[0].shiftGameReset = joyButton;
				break;
			case 's': // denote the shift button
				joystates[0].shiftButton = joyButton;
				break;
			default:
				goto stop_parse;
				break;
		}
		}
	};

stop_parse:	
	fclose(theFile);
	return 1;
}

static void CalibrateJoystick (void)
{
	Sint16 x, y;
	int i;
	SDL_JoystickUpdate();
	
	for (i = 0; i < numjoysticks; i++)
	{
		x = SDL_JoystickGetAxis(joystates[i].joy,0)/512+64;
		y = SDL_JoystickGetAxis(joystates[i].joy,1)/512+64;
		
		joystates[i].center_x = x;
		joystates[i].center_y = y;
		
	}
}


static int Joy_Init (void)
{
	int numbuttons;
	int i;
	char success = 0;
	
	memset(joystates,0,sizeof(joystates));
	joystates[0].shiftButton = joystates[1].shiftButton = 1;
	joystates[0].shiftComputerReset = joystates[1].shiftComputerReset = 8;
	joystates[0].shiftGameReset = joystates[1].shiftGameReset = 10;
	joystates[0].shiftPause = joystates[1].shiftPause = 9;
	joystates[0].shiftQuit = joystates[1].shiftQuit = 12;
	
	numjoysticks = SDL_NumJoysticks();
	printf("Initializing Joystick support\n");
	
	if (numjoysticks >= 2) 
	{
		numjoysticks = 2;
		joystick = 2;
	} 
	else if (numjoysticks == 0)
	{
		printf("  No joysticks found\n");
		joystick = 0;
		return 0;
	}

	printf("  Opening joystick config file %s...",szJoystickFileName);
	success = ParseJoystickConfigFile(szJoystickFileName);
	if (success == 1)
	{
		printf("OK\n");
	}
	else if (success == -1)
	{
		printf("FAILED\n    ...file not found, so using defaults.");
	}
	else 
	{
		printf("FAILED\n    File doesn't conform to format.");
		joystick = 0;
		return 0;
	}

	printf("  Number of joysticks: %d\n",numjoysticks);

	for (i = 0; i < numjoysticks; i++)
	{
		// Open joystick
		joystates[i].joy = SDL_JoystickOpen(i);

		if(joystates[i].joy)
		{
			printf("  Opened Joystick %d\n",i);
			printf("  Name: %s\n", SDL_JoystickName(joystates[i].joy));
			printf("  Number of Axes: %d\n", SDL_JoystickNumAxes(joystates[i].joy));
			numbuttons = SDL_JoystickNumButtons(joystates[i].joy);
			printf("  Number of Buttons: %d\n", numbuttons);
			printf("  Number of Balls: %d\n", SDL_JoystickNumBalls(joystates[i].joy));
			joystates[i].center_x=64;
			joystates[i].center_y=64;
		}
		else
		{
			printf("  Couldn't open Joystick %d, defaulting to no joysticks.\n",i);
			numjoysticks = 0;
			joystick = 0;
			return 0;
		}
	}
	CalibrateJoystick();
	return 1;
}


/****************************************************************************/
/** Deallocate all resources taken by InitMachine()                        **/
/****************************************************************************/
void TrashMachine(void)
{
	SDL_DestroyTexture(sdlTexture);
	SDL_DestroyRenderer(sdlRenderer);
	if (ntscFilter)
	{
		free(ntsc);
		free(DisplayBuf);
	}
	SDL_FreeSurface(screen);


  	SDL_Quit();
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

static int NextBitmapFile (void)
{
	return NextFile (szBitmapFile);
}

static int NextSnapshotFile (void)
{
	return NextFile (szSnapshotFile);
}

/****************************************************************************/
/** Allocate resources needed by SDL driver code                           **/
/****************************************************************************/
int InitMachine(void)
{
	int i;
	FILE *snapshotfile;
	int posX = 100, posY = 100;
	//initSem = SDL_CreateSemaphore(0);
	//SDL_CreateThread(keyboard_update, "EventThread", (void*)NULL);

	//if (!initSem)
	//{
	//	printf("Couldn't create semaphore\n");
	//}
	

    //Blarggs SMS NTSC Filter setup
	if (ntscFilter)
	{
		sms_ntsc_setup_t setup = sms_ntsc_composite;

		ntsc = (sms_ntsc_t*)malloc(sizeof(sms_ntsc_t));
		if (!ntsc) {
			printf("Out of memory");
			return 0;
		}
		sms_ntsc_init(ntsc, &setup);
	}
	memset (VGA_Palette,0,sizeof(VGA_Palette));
	memcpy (VGA_Palette,Coleco_Palette,16*3);

	width=WIDTH;
	height = ntscFilter ? HEIGHT * 2 : HEIGHT;
//	Black = 0;
//	White = 0xff;
	switch (videomode)
	{
		case 0:
			windowWidth = ntscFilter ? SMS_NTSC_OUT_WIDTH(width) : width * 2;
			windowHeight = ntscFilter ? height : height * 2;
			break;
		case 1:
			windowWidth = ntscFilter ? SMS_NTSC_OUT_WIDTH(width) * 2 : width * 3;
			windowHeight = ntscFilter ? height*2 : height*3;
			break;
		case 2:
			windowWidth=1280;
			windowHeight=720;
		break;
		case 3:
			windowWidth=1920;
			windowHeight=1080;
			break;
		default:
			if (Verbose) printf("Wrong video mode\n");
			return 0;
	}

	if (Verbose) printf ("Initialising SDL drivers...");
	SDL_SetMainReady();

	if ( SDL_Init(SDL_INIT_AUDIO|SDL_INIT_VIDEO|SDL_INIT_JOYSTICK) < 0 )
	{
		if (Verbose) printf("FAILED: %s\n", SDL_GetError());
		return 0;
	}
	if (Verbose) printf ("OK\n  Opening display... ");
	if (Verbose) printf ("OK\n  Opening window... ");


	//SDL_EnableKeyRepeat(150, 50);
	win = SDL_CreateWindow(Title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, windowWidth, windowHeight, 0);
	sdlRenderer = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);

	screen = SDL_CreateRGBSurface(0, ntscFilter ? SMS_NTSC_OUT_WIDTH(width) : width, height, 16, 0, 0, 0, 0);
	if (!screen) //Couldn't create window?
	{
		printf("Couldn't create screen\n"); //Output to stderr and quit
		return 0;
	}
	sdlTexture = SDL_CreateTexture(sdlRenderer,
		SDL_PIXELFORMAT_RGB565,
		SDL_TEXTUREACCESS_STREAMING,
	    screen->w, screen->h);

	if (!sdlTexture) //Couldn't create window?
	{
		printf("Couldn't create texture: %s\n",SDL_GetError()); //Output to stderr and quit
		return 0;
	}

	if (ntscFilter)
	{
		DisplayBuf = malloc(width * height * sizeof(SMS_NTSC_IN_T));
		memset(DisplayBuf, 0, width * height * sizeof(SMS_NTSC_IN_T));
	}
	else
	{
		DisplayBuf = screen->pixels;
	}

	if (Verbose) printf ("OK\n  Allocating colours... ");
	for (i=0;i<16;i++)
	{
		RGBcolors[i] = SDL_MapRGB(screen->format, Palettes[PalNum][ i * 3 + 0], Palettes[PalNum][i * 3 + 1], Palettes[PalNum][i * 3 + 2]);
		PalBuf[i] = i;
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

#ifdef SOUND
	InitSound ();
#endif
//	if (expansionmode==2 || expansionmode==3 || (expansionmode!=1 && expansionmode!=4 && joystick))
		Joy_Init ();

//	if (expansionmode==1 || expansionmode==4 ||
//		expansionmode==5 || expansionmode==6 ||
//		expansionmode==7)
//		i=Mouse_Detect ();
	if (mouse_sens<=0 || mouse_sens>1000)
		mouse_sens=default_mouse_sens;

	CalibrateJoystick();

	switch (expansionmode)
	{
		case 2:
		case 3:
			if (!numjoysticks)
			{
				expansionmode=0;
				break;
			}
		break;
//		case 1:
//		case 4:
//		case 5:
//		case 6:
//		case 7:
//				expansionmode=0;
//		break;
//		default:
//			expansionmode=0;
//			default_mouse_sens*=5;
//		break;
	}
	if (syncemu)
		OldTimer=ReadTimer ();

	/* Parse keyboard mapping string */
	sscanf (szKeys,"%03X%03X%03X%03X%03X%03X%03X%03X", &KEY_LEFT,&KEY_RIGHT,&KEY_UP,&KEY_DOWN,
												       &KEY_BUTTONA,&KEY_BUTTONB,&KEY_BUTTONC,&KEY_BUTTOND);
	Mouse_Init ();
	keyboardmode=(EmuMode)? 1:0;

	//Confine the keyboard and mouse input to the emulator window
	SDL_ShowCursor(SDL_DISABLE);

	//Ignore various events...
//	SDL_EventState(SDL_ACTIVEEVENT,SDL_IGNORE);
	SDL_EventState(SDL_MOUSEMOTION,SDL_IGNORE);
	SDL_EventState(SDL_MOUSEBUTTONDOWN,SDL_IGNORE);
	SDL_EventState(SDL_MOUSEBUTTONUP,SDL_IGNORE);
	SDL_EventState(SDL_TEXTINPUT, SDL_IGNORE);

	//SDL_SemPost(initSem);
	return 1;
}

/****************************************************************************/
/** Routines to modify the Coleco gameport status                          **/
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

	JoyState[0]=(MouseJoyState[0] & JoystickJoyState[0]);
	JoyState[1]=(MouseJoyState[1] & JoystickJoyState[1]);
	
	if (joystates[0].buttons[joystates[0].shiftButton] && joystates[0].shifttime + 3000 <= SDL_GetTicks() ) { 
		if (joystates[0].buttons[joystates[0].shiftPause]) 
		{
			PausePressed = PausePressed ? 0 : 2;
			joystates[0].shifttime = SDL_GetTicks();
		} 
		else if (joystates[0].buttons[joystates[0].shiftGameReset]) 
		{
			ResetColeco(1);
			joystates[0].shifttime = SDL_GetTicks();
		} 
		else if (joystates[0].buttons[joystates[0].shiftComputerReset]) 
		{
			ResetColeco(0);		
			joystates[0].shifttime = SDL_GetTicks();
		} 
		else if (joystates[0].buttons[joystates[0].shiftQuit])
		{
			keyb_handler(SCANCODE_F10,1);
			joystates[0].shifttime = SDL_GetTicks();
		}
	}

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
		for (i=SCANCODE_1;i<=SCANCODE_0;i++)
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
	} while (tmp);

	/* Check mouse and joystick events */
	Joysticks ();

	/* Check is PAUSE is pressed */
	if (PausePressed)
	{
#ifdef SOUND		
		StopSound ();
#endif
		tmp=0;
		while (PausePressed) 
		{
			keyboard_update();
			Joysticks();
		}
#ifdef SOUND
		ResumeSound ();
#endif
		if (syncemu)
			OldTimer=ReadTimer ();
	}

	 /* Check if a screen shot should be taken */
	if (makeshot)
	{
		WriteBitmap (szBitmapFile,8,17,width,width,height,
					 (char *)DisplayBuf,(char *)VGA_Palette);
		NextBitmapFile ();
		makeshot--;
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
#ifdef SOUND
		StopSound ();
#endif
		OptionsDialogue ();
#ifdef SOUND
		ResumeSound ();
#endif
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

static void SDLPutPixel (int offset,int c)
{
	if (ntscFilter)
		offset=(offset&0x00FF)+((offset&0xFF00)<<1);
    SDL_LockSurface(screen);
    DisplayBuf[offset]=RGBcolors[c];
	if (ntscFilter)
		DisplayBuf[(offset+0x0100)]=RGBcolors[c];
    SDL_UnlockSurface(screen);
}

/* Return the time elapsed in microseconds */
static int ReadTimer (void)
{
	return SDL_GetTicks() * 1000;
}

void setFullScreen(int on)
{
	int i=0;
	if (on && !fullscreen) {
		SDL_SetWindowFullscreen(win,SDL_WINDOW_FULLSCREEN);
		fullscreen = 1;
	} else {
		SDL_SetWindowFullscreen(win,0);
		fullscreen = 0;
	}
	
	ColourScreen();
	VDP.ScreenChanged=1;
	RefreshScreen(VDP.ScreenChanged);

}

int WindowManagerSetPause(int pauseVal)
{
	if (pauseVal) {
		SDL_ShowCursor(SDL_ENABLE);
		SDL_EventState(SDL_KEYDOWN,SDL_IGNORE);
		SDL_EventState(SDL_KEYUP,SDL_IGNORE);


	} else {
		SDL_ShowCursor(SDL_DISABLE);
		SDL_EventState(SDL_KEYDOWN,SDL_ENABLE);
		SDL_EventState(SDL_KEYUP,SDL_ENABLE);
	}

	PausePressed = pauseVal;
	return PausePressed;
}

/* Bogus for now... just getting TEXT80 support added for ADAMEmMam support*/
#ifdef TEXT80
void InitText80 (void)
{
// __dpmi_regs r;
// r.x.ax=3;                           /* Set 80x25 text mode                 */
// __dpmi_int (0x10, &r);
// r.h.ah=1;                           /* Turn off cursor                     */
// r.x.cx=0x0F00;
// __dpmi_int (0x10, &r);
// VGA_SetPalette (16,VGA_Palette+3);  /* Set palette                         */
 Text80=1;
 RefreshScreen(1);                   /* Force a screen refresh              */
}

void ResetText80 (void)
{
// VGA_Init ();
 Text80=0;
 RefreshScreen(1);
}
#endif

/****************************************************************************/
/** Screen refresh drivers                                                 **/
/****************************************************************************/
#include "Common.h"
