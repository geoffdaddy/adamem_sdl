/** ADAMEm: Coleco ADAM emulator ********************************************/
/**                                                                        **/
/**                                   X.h                                  **/
/**                                                                        **/
/** This file contains X-Windows specific definitions. It does not include **/
/** the sound emulation definitions                                        **/
/**                                                                        **/
/** Copyright (C) Marcel de Kogel 1996,1997,1998,1999                      **/
/**     You are not allowed to distribute this software commercially       **/
/**     Please, notify me, if you make any changes to this file            **/
/****************************************************************************/

/* Screen width and height */
#define WIDTH  256
#define HEIGHT 212

extern byte *DisplayBuf;          /* Screen buffer                          */
extern char szJoystickFileName[]; /* File holding joystick information      */
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
#ifdef MITSHM
extern int  UseSHM;               /* Should we use SHM extensions ?         */
#endif

#define NR_KEYS		512

#define SCANCODE_LEFTCONTROL		(XK_Control_L&511)
#define SCANCODE_RIGHTCONTROL		(XK_Control_R&511)
#define SCANCODE_INSERT			(XK_Insert&511)
#define SCANCODE_HOME			(XK_Home&511)
#define SCANCODE_PAGEUP			(XK_Page_Up&511)
#define SCANCODE_REMOVE			(XK_Delete&511)
#define SCANCODE_END			(XK_End&511)
#define SCANCODE_PAGEDOWN		(XK_Page_Down&511)
#define SCANCODE_CAPSLOCK		(XK_Caps_Lock&511)
#define SCANCODE_F1			(XK_F1&511)
#define SCANCODE_F2			(XK_F2&511)
#define SCANCODE_F3			(XK_F3&511)
#define SCANCODE_F4			(XK_F4&511)
#define SCANCODE_F5			(XK_F5&511)
#define SCANCODE_F6			(XK_F6&511)
#define SCANCODE_F7			(XK_F7&511)
#define SCANCODE_F8			(XK_F8&511)
#define SCANCODE_F9			(XK_F9&511)
#define SCANCODE_F10			(XK_F10&511)
#define SCANCODE_F11			(XK_F11&511)
#define SCANCODE_F12			(XK_F12&511)
#define SCANCODE_LEFTSHIFT		(XK_Shift_L&511)
#define SCANCODE_RIGHTSHIFT		(XK_Shift_R&511)
#define SCANCODE_KEYPAD0		(XK_KP_0&511)
#define SCANCODE_KEYPAD1		(XK_KP_1&511)
#define SCANCODE_KEYPAD2		(XK_KP_2&511)
#define SCANCODE_KEYPAD3		(XK_KP_3&511)
#define SCANCODE_KEYPAD4		(XK_KP_4&511)
#define SCANCODE_KEYPAD5		(XK_KP_5&511)
#define SCANCODE_KEYPAD6		(XK_KP_6&511)
#define SCANCODE_KEYPAD7		(XK_KP_7&511)
#define SCANCODE_KEYPAD8		(XK_KP_8&511)
#define SCANCODE_KEYPAD9		(XK_KP_9&511)
#define SCANCODE_KEYPADPERIOD		(XK_KP_Decimal&511)
#define SCANCODE_KEYPADENTER		(XK_KP_Enter&511)
#define SCANCODE_KEYPADPLUS		(XK_KP_Add&511)
#define SCANCODE_KEYPADMINUS		(XK_KP_Subtract&511)
#define SCANCODE_CURSORLEFT		(XK_KP_Left&511)
#define SCANCODE_CURSORBLOCKLEFT	(XK_Left&511)
#define SCANCODE_CURSORRIGHT		(XK_KP_Right&511)
#define SCANCODE_CURSORBLOCKRIGHT	(XK_Right&511)
#define SCANCODE_CURSORUP		(XK_KP_Up&511)
#define SCANCODE_CURSORBLOCKUP		(XK_Up&511)
#define SCANCODE_CURSORDOWN		(XK_KP_Down&511)
#define SCANCODE_CURSORBLOCKDOWN	(XK_Down&511)
#define SCANCODE_0			(XK_0&511)
#define SCANCODE_1			(XK_1&511)
#define SCANCODE_2			(XK_2&511)
#define SCANCODE_3			(XK_3&511)
#define SCANCODE_4			(XK_4&511)
#define SCANCODE_5			(XK_5&511)
#define SCANCODE_6			(XK_6&511)
#define SCANCODE_7			(XK_7&511)
#define SCANCODE_8			(XK_8&511)
#define SCANCODE_9			(XK_9&511)
#define SCANCODE_EQUAL			(XK_equal&511)
#define SCANCODE_MINUS			(XK_minus&511)
#define SCANCODE_LEFTALT		(XK_Alt_L&511)
#define SCANCODE_RIGHTALT		(XK_Alt_R&511)

