/** ADAMEm: Coleco ADAM emulator ********************************************/
/**                                                                        **/
/**                                 MSDOS.c                                **/
/**                                                                        **/
/** This file contains generic MS-DOS specific definitions. It does not    **/
/** include the sound emulation definitions                                **/
/**                                                                        **/
/** Copyright (C) Marcel de Kogel 1996,1997,1998,1999                      **/
/**     You are not allowed to distribute this software commercially       **/
/**     Please, notify me, if you make any changes to this file            **/
/****************************************************************************/

/* Screen width and height */
#define WIDTH  256
#define HEIGHT 192

#define TEXT80                    /* 80x24 TDOS mode supported              */

extern word cs_alias;             /* Our data selector                      */
extern word DosSelector;          /* Selector of DOS memory                 */
extern byte OPL3;                 /* If 0, do not detect OPL3               */
extern byte DisplayBuf[];         /* Screen buffer                          */
extern char szBitmapFile[];       /* Next screen shot file                  */
extern char szSnapshotFile[];     /* Next snapshot file                     */
extern char szJoystickFileName[]; /* File holding joystick information      */
extern char *szKeys;              /* Key scancodes                          */
extern int  mouse_sens;           /* Mouse sensitivity                      */
extern int  keypadmode;           /* 1 if keypad should be reversed         */
extern int  joystick;             /* Joystick support                       */
extern int  calibrate;            /* Set to 1 to force joystick calibration */
extern int  swapbuttons;          /* 1=joystick, 2=keyboard, 4=mouse        */
extern int  expansionmode;        /* Expansion module emulated              */
extern int  useoverscan;          /* Overscan colour support                */
extern int  videomode;            /* 0=320x200   1=256x192                  */
extern int  syncemu;              /* 0 if emulation shouldn't be synced     */
extern int AutoText80;            /* 1 if auto-switch to Text80 mode on     */

/* Various scan codes */
#define VK_Escape       0x01
#define VK_F1           0x3B
#define VK_F2           0x3C
#define VK_F3           0x3D
#define VK_F4           0x3E
#define VK_F5           0x3F
#define VK_F6           0x40
#define VK_F7           0x41
#define VK_F8           0x42
#define VK_F9           0x43
#define VK_F10          0x44
#define VK_F11          0x57
#define VK_F12          0x58
#define VK_Pause        VK_F9
#define VK_Alt          0x38
#define VK_Ctrl         0x1D
#define VK_Space        0x39
#define VK_Down         0x50
#define VK_Up           0x48
#define VK_Left         0x4B
#define VK_Right        0x4D
#define VK_0            0x0B
#define VK_1            0x02
#define VK_2            0x03
#define VK_3            0x04
#define VK_4            0x05
#define VK_5            0x06
#define VK_6            0x07
#define VK_7            0x08
#define VK_8            0x09
#define VK_9            0x0A
#define VK_Minus        0x0C
#define VK_Equal        0x0D
#define VK_LeftShift    0x2A
#define VK_RightShift   0x36
#define VK_Insert       0x52
#define VK_Home         0x47
#define VK_PageUp       0x49
#define VK_Del          0x53
#define VK_End          0x4F
#define VK_PageDown     0x51
#define VK_Enter        0x1C
#define VK_NumPad5      0x4C
#define VK_Z            0x2C
#define VK_A            0x1E
#define VK_Q            0x10
#define VK_CapsLock     0x3A
#define VK_MinusNumPad  0x4A
#define VK_PlusNumPad   0x4E
#define VK_Tab          0x0F
