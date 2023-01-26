/** ADAMEm: Coleco ADAM emulator ********************************************/
/**                                                                        **/
/**                             AdamemSDL.h	                           **/
/**                                                                        **/
/** This file contains Simple DirectMedia Layer specific definitions.      **/
/** It does not include the sound emulation definitions.                   **/
/**                                                                        **/
/** Copyright (C) Geoff Oltmans 2006                                       **/
/**     You are not allowed to distribute this software commercially       **/
/**     Please, notify me, if you make any changes to this file            **/
/****************************************************************************/

#ifndef _ADAMEMSDL_H
#define _ADAMEMSDL_H
#include <SDL_stdinc.h>
#include "sms_ntsc.h"

/* Screen width and height */
#define WIDTH  256
#define HEIGHT 212


//extern Uint32 *DisplayBuf;          /* Screen buffer                          */
extern SMS_NTSC_IN_T *DisplayBuf;
extern char szJoystickFileName[]; /* File holding joystick information      */
extern char szBitmapFile[];       /* Next screen shot file                  */
extern char szSnapshotFile[];     /* Next snapshot file                     */
extern char *szKeys;              /* Key scancodes                          */
extern int  mouse_sens;           /* Mouse/Joystick sensitivity             */
extern int  keypadmode;           /* 1 if keypad should be reversed         */
extern int  joystick;             /* Joystick support                       */
extern int  calibrate;            /* Set to 1 to force joystick calibration */
extern int  swapbuttons;          /* 1=joystick, 2=keyboard, 4=mouse        */
extern int  expansionmode;        /* Expansion module emulated              */
extern int  syncemu;              /* 0 if emulation shouldn't be synced     */
extern int  SaveCPU;              /* If 1, CPU is saved when focus is out   */
extern int  videomode;            /* 0=1x1  1=2x1                           */
extern int  PausePressed;
extern int  AutoText80;           /* 1 if auto-switch to Text80 mode on     */

void setFullScreen(int on);

#define NR_JOYBUTTONS	30

#define NR_KEYS		SDL_NUM_SCANCODES

#define SCANCODE_LEFTCONTROL		SDL_SCANCODE_LCTRL
#define SCANCODE_RIGHTCONTROL		SDL_SCANCODE_RCTRL
#define SCANCODE_INSERT			SDL_SCANCODE_INSERT//SDL_SCANCODE_INSERT
#define SCANCODE_HOME			SDL_SCANCODE_HOME//SDL_SCANCODE_HOME
#define SCANCODE_PAGEUP			SDL_SCANCODE_PAGEUP
#define SCANCODE_REMOVE			SDL_SCANCODE_DELETE//SDL_SCANCODE_DELETE
#define SCANCODE_END			SDL_SCANCODE_END//SDL_SCANCODE_END
#define SCANCODE_PAGEDOWN		SDL_SCANCODE_PAGEDOWN//SDL_SCANCODE_PAGEDOWN
#define SCANCODE_CAPSLOCK		SDL_SCANCODE_CAPSLOCK//SDL_SCANCODE_CAPSLOCK
#define SCANCODE_F1			SDL_SCANCODE_F1//SDL_SCANCODE_F1
#define SCANCODE_F2			SDL_SCANCODE_F2//SDL_SCANCODE_F2
#define SCANCODE_F3			SDL_SCANCODE_F3//SDL_SCANCODE_F3
#define SCANCODE_F4			SDL_SCANCODE_F4//SDL_SCANCODE_F4
#define SCANCODE_F5			SDL_SCANCODE_F5//SDL_SCANCODE_F5
#define SCANCODE_F6			SDL_SCANCODE_F6//SDL_SCANCODE_F6
#define SCANCODE_F7			SDL_SCANCODE_F7//SDL_SCANCODE_F7
#define SCANCODE_F8			SDL_SCANCODE_F8//SDL_SCANCODE_F8
#define SCANCODE_F9			SDL_SCANCODE_F9//SDL_SCANCODE_F9
#define SCANCODE_F10			SDL_SCANCODE_F10//SDL_SCANCODE_F10
#define SCANCODE_F11			SDL_SCANCODE_F11//SDL_SCANCODE_F11
#define SCANCODE_F12			SDL_SCANCODE_F12//SDL_SCANCODE_F12
#define SCANCODE_LEFTSHIFT		SDL_SCANCODE_LSHIFT//SDL_SCANCODE_LSHIFT
#define SCANCODE_RIGHTSHIFT		SDL_SCANCODE_RSHIFT//SDL_SCANCODE_RSHIFT
#define SCANCODE_KEYPAD0		SDL_SCANCODE_KP_0
#define SCANCODE_KEYPAD1		SDL_SCANCODE_KP_1
#define SCANCODE_KEYPAD2		SDL_SCANCODE_KP_2
#define SCANCODE_KEYPAD3		SDL_SCANCODE_KP_3
#define SCANCODE_KEYPAD4		SDL_SCANCODE_KP_4
#define SCANCODE_KEYPAD5		SDL_SCANCODE_KP_5
#define SCANCODE_KEYPAD6		SDL_SCANCODE_KP_6
#define SCANCODE_KEYPAD7		SDL_SCANCODE_KP_7
#define SCANCODE_KEYPAD8		SDL_SCANCODE_KP_8
#define SCANCODE_KEYPAD9		SDL_SCANCODE_KP_9
#define SCANCODE_KEYPADPERIOD		SDL_SCANCODE_KP_PERIOD
#define SCANCODE_KEYPADENTER		SDL_SCANCODE_KP_ENTER
#define SCANCODE_KEYPADPLUS		SDL_SCANCODE_KP_PLUS
#define SCANCODE_KEYPADMINUS		SDL_SCANCODE_KP_MINUS
#define SCANCODE_CURSORLEFT		SDL_SCANCODE_LEFT
#define SCANCODE_CURSORBLOCKLEFT	SDL_SCANCODE_LEFT
#define SCANCODE_CURSORRIGHT		SDL_SCANCODE_RIGHT
#define SCANCODE_CURSORBLOCKRIGHT	SDL_SCANCODE_RIGHT
#define SCANCODE_CURSORUP		SDL_SCANCODE_UP
#define SCANCODE_CURSORBLOCKUP		SDL_SCANCODE_UP
#define SCANCODE_CURSORDOWN		SDL_SCANCODE_DOWN
#define SCANCODE_CURSORBLOCKDOWN	SDL_SCANCODE_DOWN
#define SCANCODE_0			SDL_SCANCODE_0
#define SCANCODE_1			SDL_SCANCODE_1
#define SCANCODE_2			SDL_SCANCODE_2
#define SCANCODE_3			SDL_SCANCODE_3
#define SCANCODE_4			SDL_SCANCODE_4
#define SCANCODE_5			SDL_SCANCODE_5
#define SCANCODE_6			SDL_SCANCODE_6
#define SCANCODE_7			SDL_SCANCODE_7
#define SCANCODE_8			SDL_SCANCODE_8
#define SCANCODE_9			SDL_SCANCODE_9
#define SCANCODE_EQUAL			SDL_SCANCODE_EQUALS
#define SCANCODE_MINUS			SDL_SCANCODE_MINUS
#define SCANCODE_LEFTALT		SDL_SCANCODE_LALT
#define SCANCODE_RIGHTALT		SDL_SCANCODE_RALT
#define SCANCODE_LEFTCMD		SDL_SCANCODE_LGUI
#define SCANCODE_RIGHTCMD		SDL_SCANCODE_RGUI
#define SCANCODE_PERIOD			SDL_SCANCODE_PERIOD//SDL_SCANCODE_PERIOD
#endif // _ADAMEM_SDL_H

