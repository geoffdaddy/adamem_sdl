/** ADAMEm: Coleco ADAM emulator ********************************************/
/**                                                                        **/
/**                                SVGALib.h                               **/
/**                                                                        **/
/** This file contains SVGALib specific definitions. It does not include   **/
/** the sound emulation definitions                                        **/
/**                                                                        **/
/** Copyright (C) Marcel de Kogel 1996,1997,1998,1999                      **/
/**     You are not allowed to distribute this software commercially       **/
/**     Please, notify me, if you make any changes to this file            **/
/****************************************************************************/

/* Screen width and height */
#define WIDTH  256
#define HEIGHT 192

extern byte DisplayBuf[];         /* Screen buffer                          */
extern char szBitmapFile[];       /* Next screen shot file                  */
extern char szSnapshotFile[];     /* Next snapshot file                     */
extern char szJoystickFileName[]; /* File holding joystick information      */
extern char *szKeys;              /* Key scancodes                          */
extern int  mouse_sens;           /* Mouse/Joystick sensitivity             */
extern int  keypadmode;           /* 1 if keypad should be reversed         */
extern int  joystick;             /* Joystick support                       */
extern int  calibrate;            /* Set to 1 to force joystick calibration */
extern int  swapbuttons;          /* 1=joystick, 2=keyboard, 4=mouse        */
extern int  expansionmode;        /* Expansion module emulated              */
extern int  videomode;            /* 0=320x200   1=320x240                  */
extern int  useoverscan;          /* Overscan colour support                */
extern int  syncemu;              /* 0 if emulation shouldn't be synced     */
extern int  chipset;              /* If 0, do not detect SVGA               */
