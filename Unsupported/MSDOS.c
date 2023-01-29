/** ADAMEm: Coleco ADAM emulator ********************************************/
/**                                                                        **/
/**                                 MSDOS.c                                **/
/**                                                                        **/
/** This file contains generic MS-DOS specific routines. It does not       **/
/** include the sound emulation code                                       **/
/**                                                                        **/
/** Copyright (C) Marcel de Kogel 1996,1997,1998,1999                      **/
/**     You are not allowed to distribute this software commercially       **/
/**     Please, notify me, if you make any changes to this file            **/
/****************************************************************************/

#include "Coleco.h"
#include "MSDOS.h"
#include "Bitmap.h"
#include "INT.h"
#include "Asm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/farptr.h>
#include <go32.h>
#include <dpmi.h>
#include <pc.h>
#include <dos.h>
#include <crt0.h>
#include <math.h>
#include <conio.h>

/* Title for -help output */
char Title[]="ADAMEm MS-DOS 1.81";

#define NUM_STACKS      8         /* Number of IRQ stacks                   */
#define STACK_SIZE      16384     /* Stack size                             */

/* DJGPP startup flags */
int _crt0_startup_flags = _CRT0_FLAG_NONMOVE_SBRK | _CRT0_FLAG_LOCK_MEMORY;

/* Key mapping */
static int KEY_LEFT,KEY_RIGHT,KEY_UP,KEY_DOWN,
           KEY_BUTTONA,KEY_BUTTONB,KEY_BUTTONC,KEY_BUTTOND;

/* Current VGA palette */
static byte VGA_Palette[17*3];

static byte PalBuf[16];           /* Palette buffer                         */
static int default_mouse_sens=200;/* Default mouse sensitivity              */
static int keyboardmode;          /* 0=joystick, 1=keyboard                 */
static word MouseJoyState[2];     /* Current mouse status                   */
static word JoystickJoyState[2];  /* Current joystick status                */
static int nOldVideoMode;         /* Videomode when ADAMem was started      */
static unsigned font2screen[16*16*16];
                                  /* Used by screen refresh driver          */
#ifdef DEBUG
static int dumpram=0;
#endif
static int makeshot=0;            /* 1 if screen shot should be taken       */
static int makesnap=0;            /* 1 if snapshot should be written        */
static int OldTimer=0;            /* Last value of timer                    */
static int NewTimer=0;            /* New value of timer                     */
static int calloptions=0;         /* If 1, OptionsDialogue() is called      */
static int in_options_dialogue=0; /* If 1, chain to old keyboard IRQ handler*/
static int SwitchVideoMode=0;     /* If 1, switch video mode in Keyboard()  */

word cs_alias;                    /* Our data selector                      */
word DosSelector;                 /* Selector of DOS memory                 */
byte DisplayBuf[WIDTH*HEIGHT];    /* Screen buffer                          */
char szBitmapFile[256];           /* Next screen shot file                  */
char szSnapshotFile[256];         /* Next snapshot file                     */
char szJoystickFileName[256];     /* File holding joystick information      */
char *szKeys="CBCDC8D0381D2A2C";  /* Key scancodes                          */
int  mouse_sens=200;              /* Mouse sensitivity                      */
int  keypadmode=0;                /* 1 if keypad should be reversed         */
int  joystick=1;                  /* Joystick support                       */
int  calibrate=0;                 /* Set to 1 to force joystick calibration */
int  swapbuttons=0;               /* 1=joystick, 2=keyboard, 4=mouse        */
int  expansionmode=0;             /* Expansion module emulated              */
int  useoverscan=1;               /* Overscan colour support                */
int  videomode=0;                 /* 0=320x200   1=256x192                  */
int  syncemu=1;                   /* 0 - Don't sync emulation               */
                                  /* 1 - Sync emulation to PC timer         */
int Text80;                       /* 1 if in Text80 mode                    */
int Text80Colour;                 /* Text80 foreground colour               */
int AutoText80;                   /* 1 if auto-switch to Text80 mode on     */

/****************************************************************************/
/** VGA routines                                                           **/
/****************************************************************************/
static void VGA_SetPalette (int n,unsigned char *pal)
{
 int i;
 __disable ();
 while ((inportb(0x3DA)&0x08)!=0);/* Wait until vertical retrace is off     */
 while ((inportb(0x3DA)&0x08)==0);/* Now wait until it is on                */
 outportb (0x3C8,0);              /* Start updating palette                 */
 for (i=0;i<n*3;++i)
  outportb(0x3C9,pal[i]/4);
 __enable ();
}

static void VGA_Reset (void)      /* Reset old video mode                   */
{
 __dpmi_regs r;
 r.x.ax=(short)nOldVideoMode;
 __dpmi_int (0x10, &r);
}

static void VGA_SetTweakMode (int mode)
{
 static const byte crtc_regs_1[25]=
 {
  0x63,
  0x4F,0x3F,0x40,0x92,
  0x44,0x10,0xAF,0x1F,
  0x00,0x41,0x00,0x00,
  0x00,0x00,0x00,0x00,
  0x8C,0x8E,0x7F,0x20,
  0x40,0x86,0xA9,0xA3
 };
 static const byte crtc_regs_2[25]=
 {
  0xE3,
  0x4F,0x3F,0x40,0x92,
  0x44,0x10,0x0D,0x3E,
  0x00,0x41,0x00,0x00,
  0x00,0x00,0x00,0x00,
  0xEA,0xAC,0xDF,0x20,
  0x40,0xE7,0x06,0xA3
 };
 static const byte *crtc_regs;
 int i;
 crtc_regs = (mode) ? crtc_regs_2 : crtc_regs_1;
 __disable ();
 while (inportb(0x3DA)&8);
 while ((inportb(0x3DA)&8)==0);
 outportw (0x3C4,0x100);          /* sequencer reset                        */
 outportb (0x3C2,*crtc_regs++);   /* misc. output reg                       */
 outportw (0x3C4,0x300);          /* clear sequencer reset                  */
 outportw (0x3D4,((crtc_regs[0x11]&0x7F)<<8)+0x11); /* deprotect regs 0-7   */
 for (i=0;i<24;++i)
  outportw (0x3D4,(crtc_regs[i]<<8)+i);
 __enable ();
}

static void VGA_SetBkColour (unsigned char colour)
{
 static int currbkcol=-1;
 if (currbkcol==colour)
  return;
 currbkcol=colour;
 VGA_Palette[0]=VGA_Palette[3]=Coleco_Palette[colour*3+0];
 VGA_Palette[1]=VGA_Palette[4]=Coleco_Palette[colour*3+1];
 VGA_Palette[2]=VGA_Palette[5]=Coleco_Palette[colour*3+2];
 __disable ();
 while (inportb(0x3DA)&8);
 while ((inportb(0x3DA)&8)==0);
 if (useoverscan || Text80)
 {
  outportb (0x3C8,0);
  outportb (0x3C9,VGA_Palette[0]/4);
  outportb (0x3C9,VGA_Palette[1]/4);
  outportb (0x3C9,VGA_Palette[2]/4);
 }
 if (!Text80)
 {
  outportb (0x3C8,1);
  outportb (0x3C9,VGA_Palette[3]/4);
  outportb (0x3C9,VGA_Palette[4]/4);
  outportb (0x3C9,VGA_Palette[5]/4);
 }
 __enable ();
}

static int VGA_Init (void)        /* Get old mode and set mode 13h          */
{
 __dpmi_regs r;
 r.x.ax=0x0F00;
 __dpmi_int (0x10,&r);
 nOldVideoMode=(int)(r.h.al & 0x7F);
 r.x.ax=0x0013;
 __dpmi_int (0x10,&r);
 if (videomode)
  VGA_SetTweakMode (videomode-1);
 VGA_SetPalette (17,VGA_Palette);
 return 1;
}

static void PutImage (void)
{
 if (Text80)
 {
  int i,b1,b2;
  _farsetsel (DosSelector);
  b1=Text80Colour;
  b2=Text80Colour<<4;
  for (i=0;i<80*24;++i)
  {
   byte c;
   c=DisplayBuf[i];
   _farnspokeb (0xB8000+i*2,c&127);
   _farnspokeb (0xB8001+i*2,(c&128)? b2:b1);
  }
 }
 else
  PutImage_Asm ();
}

void InitText80 (void)
{
 __dpmi_regs r;
 r.x.ax=3;                           /* Set 80x25 text mode                 */
 __dpmi_int (0x10, &r);
 r.h.ah=1;                           /* Turn off cursor                     */
 r.x.cx=0x0F00;
 __dpmi_int (0x10, &r);
 VGA_SetPalette (16,VGA_Palette+3);  /* Set palette                         */
 Text80=1;
 RefreshScreen(1);                   /* Force a screen refresh              */
}

void ResetText80 (void)
{
 VGA_Init ();
 Text80=0;
 RefreshScreen(1);
}

/****************************************************************************/
/** Keyboard routines                                                      **/
/****************************************************************************/
static __volatile__ int PausePressed=0;
static __volatile__ byte keybstatus[256];
static _go32_dpmi_seginfo keybirq;

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

static const byte scan2ascii[256] =
{
 0,27,'1','2','3','4','5','6','7','8','9','0','-','=',8,
 9,'q','w','e','r','t','y','u','i','o','p','[',']',13,
 0,'a','s','d','f','g','h','j','k','l',';',39,'`',
 0,92,'z','x','c','v','b','n','m',',','.','/',0,
 '*',0,' ',0,129,130,131,132,133,134,144,145,0,0,
 0,0,171,160,168,'-',163,128,161,'+',170,162,169,148,151,
 0,0,0,0,0,0,0,0,0,0,0,0,
 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
 /* extended keys */
 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
 0,0,0,0,0,'/',0,0,0,0,0,0,0,0,0,0,
 0,0,0,0,0,0,0,146,160,150,0,163,0,161,0,147,
 162,149,148,151,0,0,0,0,0,0,0,0,0,0,0,0,
 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};
static const byte scan2ascii_shift[256] =
{
 0,27,'!','@','#','$','%','^','&','*','(',')','_','+',184,
 185,'Q','W','E','R','T','Y','U','I','O','P','{','}',13,
 0,'A','S','D','F','G','H','J','K','L',':',34,'~',
 0,'|','Z','X','C','V','B','N','M','<','>','?',0,
 '*',0,' ',0,137,138,139,140,141,142,152,153,0,0,
 0,0,0,0,0,'-',0,0,0,'+',0,0,0,156,159,
 0,0,0,0,0,0,0,0,0,0,0,0,
 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
 /* extended keys */
 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
 0,0,0,0,0,'/',0,0,0,0,0,0,0,0,0,0,
 0,0,0,0,0,0,0,154,0,158,0,0,0,0,0,155,
 0,157,156,159,0,0,0,0,0,0,0,0,0,0,0,0,
 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};
static const byte scan2ascii_ctrl[256] =
{
 0,27,0,255,0,0,0,30,0,0,0,0,31,0,127,
 0,17,23,5,18,20,25,21,9,15,16,27,29,13,
 0,1,19,4,6,7,8,10,11,12,0,0,0,
 0,28,26,24,3,22,2,14,13,0,0,0,0,
 0,0,32,0,0,0,0,0,0,0,0,0,0,0,
 0,0,0,164,0,0,167,0,165,0,0,166,0,0,0,
 0,0,0,0,0,0,0,0,0,0,0,0,
 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
 /* extended keys */
 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
 0,0,0,0,0,0,0,0,164,0,0,167,0,165,0,0,
 166,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};
static inline int AltPressed (void)
{
 return keybstatus[VK_Alt]||keybstatus[VK_Alt+128];
}
static inline int CtrlPressed (void)
{
 return keybstatus[VK_Ctrl]||keybstatus[VK_Ctrl+128];
}
static int keyb_interrupt (void)
{
 unsigned code;
 static int extkey;
 static int CapsLock=0;
 code=inportb (0x60);             /* get scancode                           */
 if (code<0xE0)                   /* ignore codes >0xE0                     */
 {
  if (code & 0x80)                /* key is released                        */
  {
   code&=0x7F;
   if (extkey)
    keybstatus[code+128]=0;
   else
    keybstatus[code]=0;
  }
  else                            /* key is pressed                         */
  {
   if (extkey)
   {
    if (!keybstatus[code+128])
    {
     keybstatus[code+128]=1;
     if (in_options_dialogue)     /* jump to old interrupt handler          */
      return 1;
     PausePressed=0;
     switch (code)
     {
      case VK_Insert:
       if (!keyboardmode || AltPressed())
        joystick=1;
       break;
      case VK_Home:
       if (!keyboardmode || AltPressed())
        joystick=2;
       break;
      case VK_PageUp:
       if (!keyboardmode || AltPressed())
        joystick=3;
       break;
      case VK_Del:
       if (!keyboardmode || AltPressed())
        swapbuttons^=1;
       break;
      case VK_End:
       if (!keyboardmode || AltPressed())
        swapbuttons^=2;
       break;
      case VK_PageDown:
       if (!keyboardmode || AltPressed())
        swapbuttons^=4;
       break;
     }
    }
   }
   else
   {
    if (!keybstatus[code])
    {
     keybstatus[code]=1;
     if (in_options_dialogue)
      return 1;
     if (CtrlPressed() &&
         (code==VK_F11 || code==VK_F12))
     {
      if (code==VK_F11)
       PausePressed=2;
      else
       PausePressed=1;
     }
     else
      PausePressed=0;
     switch (code)
     {
      case VK_CapsLock:
       CapsLock^=1;
       break;
      case VK_F1:
       if (!keyboardmode || AltPressed())
        ToggleSoundChannel (0);
       break;
      case VK_F2:
       if (!keyboardmode || AltPressed())
        ToggleSoundChannel (1);
       break;
      case VK_F3:
       if (!keyboardmode || AltPressed())
        ToggleSoundChannel (2);
       break;
      case VK_F4:
       if (!keyboardmode || AltPressed())
        ToggleSoundChannel (3);
       break;
      case VK_F5:
       if (!keyboardmode || AltPressed())
        ToggleSound ();
       break;
      case VK_F11:
       if (!PausePressed && !AltPressed())
        DecreaseSoundVolume ();
       break;
      case VK_F12:
       if (!PausePressed && !AltPressed())
        IncreaseSoundVolume ();
       break;
      case VK_F10:
       Z80_Running=0;
       break;
      case VK_F7:
#ifdef DEBUG
       if (CtrlPressed())
       {
        Z80_Trace=!Z80_Trace;
        break;
       }
#endif
       if (!keyboardmode || AltPressed())
        makesnap=1;
       break;
      case VK_F8:
#ifdef DEBUG
       if (CtrlPressed())
        dumpram=1;
       else
#endif
       if (!keyboardmode || AltPressed())
        makeshot=1;
       break;
      case VK_F9:
       if (EmuMode)
       {
        if (AltPressed())
         SwitchVideoMode=1;
        else
         if (CtrlPressed())
          calloptions=1;
         else
          keyboardmode=(keyboardmode)? 0:1;
       }
       break;
     }
    }
   }
   if (keyboardmode && !AltPressed())  /* Modify keyboard buffer            */
   {
    /* Check for HOME+Cursor key and Cursor key combinations */
    if (code==VK_NumPad5 && !extkey)
    {
     if (keybstatus[VK_Left] ||
         keybstatus[VK_Left+128]) { code=175; goto put_char_in_buf; }
     if (keybstatus[VK_Right] ||
         keybstatus[VK_Right+128]) { code=173; goto put_char_in_buf; }
     if (keybstatus[VK_Up] ||
         keybstatus[VK_Up+128]) { code=172; goto put_char_in_buf; }
     if (keybstatus[VK_Down] ||
         keybstatus[VK_Down+128]) { code=170; goto put_char_in_buf; }
    }
    if (keybstatus[VK_NumPad5])
    {
     if (code==VK_Left) { code=175; goto put_char_in_buf; }
     if (code==VK_Right) { code=173; goto put_char_in_buf; }
     if (code==VK_Up) { code=172; goto put_char_in_buf; }
     if (code==VK_Down) { code=170; goto put_char_in_buf; }
    }
    if (code==VK_Up)
    {
     if (keybstatus[VK_Right] ||
         keybstatus[VK_Right+128]) { code=168; goto put_char_in_buf; }
     if (keybstatus[VK_Left] ||
         keybstatus[VK_Left+128]) { code=171; goto put_char_in_buf; }
    }
    if (code==VK_Right)
    {
     if (keybstatus[VK_Up] ||
         keybstatus[VK_Up+128]) { code=168; goto put_char_in_buf; }
     if (keybstatus[VK_Down] ||
         keybstatus[VK_Down+128]) { code=169; goto put_char_in_buf; }
    }
    if (code==VK_Down)
    {
     if (keybstatus[VK_Right] ||
         keybstatus[VK_Right+128]) { code=169; goto put_char_in_buf; }
     if (keybstatus[VK_Left] ||
         keybstatus[VK_Left+128]) { code=170; goto put_char_in_buf; }
    }
    if (code==VK_Left)
    {
     if (keybstatus[VK_Up] ||
         keybstatus[VK_Up+128]) { code=171; goto put_char_in_buf; }
     if (keybstatus[VK_Down] ||
         keybstatus[VK_Down+128]) { code=170; goto put_char_in_buf; }
    }
    if (extkey) code+=128;
    if (keybstatus[VK_Ctrl] || keybstatus[VK_Ctrl+128])
     code=scan2ascii_ctrl[code];
    else if (keybstatus[VK_RightShift] || keybstatus[VK_LeftShift])
     code=scan2ascii_shift[code];
    else
     code=scan2ascii[code];
put_char_in_buf:
    if (CapsLock)
    {
     if (code>='a' && code<='z') code+='A'-'a';
     else if (code>='A' && code<='Z') code+='a'-'A';
    }
    if (code) LocalAddToKeyboardBuffer (code);
   }
  }
  extkey=0;
 }
 else
  if (code==0xE0)
   extkey=1;
 if (in_options_dialogue)
  return 1;
 code=inportb (0x61);             /* acknowledge interrupt                  */
 outportb (0x61,code | 0x80);
 outportb (0x61,code);
 outportb (0x20,0x20);
 return 0;                        /* Do not chain to old handler            */
}

static int Keyb_Init (void)       /* get old handler and install our own    */
{
 keybirq.pm_offset=(int)keyb_interrupt;
 keybirq.pm_selector=_go32_my_cs();
 SetInt (9,&keybirq);
 return 1;
}

static void Keyb_Reset (void)     /* install the old handler                */
{
 ResetInt (9,&keybirq);
}

/****************************************************************************/
/** Mouse routines                                                         **/
/****************************************************************************/
static _go32_dpmi_seginfo mouse_seginfo;
static _go32_dpmi_registers mouse_regs;
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

static void Mouse_Callback (_go32_dpmi_registers *r)
{
 mouse_buttons=r->x.bx&7;
 mouse_xpos=r->x.cx;
 mouse_ypos=r->x.dx;
}

static void Mouse_Check (void)
{
 __dpmi_regs r;
 int tmp;
 if (!got_mouse)
  return;
 mouse_x=mouse_xpos-500;
 mouse_y=mouse_ypos-500;
 tmp=mouse_buttons;
 if (mouse_x || mouse_y)
 {
  r.x.ax=0x04;
  r.x.cx=500;
  r.x.dx=500;
  __dpmi_int (0x33,&r);
  mouse_xpos=mouse_ypos=500;
 }
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
   mouse_x=(-mouse_x)/4;
   mouse_y=-mouse_x;
   mouse_buttons=0;
   break;
  case 6:                         /* emulate SA speed roller on port 1      */
   mouse_x=(-mouse_x)/4;
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
 __dpmi_regs r;
 r.x.ax=0;
 __dpmi_int (0x33,&r);
 if (r.x.ax==0)
  return 0;
 got_mouse=1;
 return 1;
}

static void Mouse_Init (void)
{
 __dpmi_regs r;
 if (!got_mouse)
  return;
 r.x.ax=0x07;                     /* set horizontal range                   */
 r.x.cx=0;
 r.x.dx=1000;
 __dpmi_int (0x33,&r);
 r.x.ax=0x08;                     /* set vertical range                     */
 r.x.cx=0;
 r.x.dx=1000;
 __dpmi_int (0x33,&r);
 r.x.ax=0x04;                     /* set position                           */
 r.x.cx=500;
 r.x.dx=500;
 __dpmi_int (0x33,&r);
 r.x.ax=0x0F;                     /* set sensitivity                        */
 r.x.cx=3;
 r.x.dx=3;
 __dpmi_int (0x33,&r);
 mouse_seginfo.pm_selector=_my_cs();
 mouse_seginfo.pm_offset=(unsigned)Mouse_Callback;
 _go32_dpmi_allocate_real_mode_callback_retf (&mouse_seginfo,&mouse_regs);
 r.x.ax=0x0C;
 r.x.cx=0x1F;
 r.x.dx=mouse_seginfo.rm_offset;
 r.x.es=mouse_seginfo.rm_segment;
 __dpmi_int (0x33,&r);
 return;
}

static void Mouse_Exit (void)
{
 __dpmi_regs r;
 if (!got_mouse)
  return;
 r.x.ax=0x0C;
 r.x.cx=0;
 r.x.dx=0;
 r.x.es=0;
 __dpmi_int (0x33,&r);
 r.x.ax=0;
 __dpmi_int (0x33,&r);
 _go32_dpmi_free_real_mode_callback (&mouse_seginfo);
}

/****************************************************************************/
/** Joystick routines                                                      **/
/****************************************************************************/
typedef struct joyposstruct
{
 int x,y;
} joypos;

static int gotjoy=0;              /* 1 if joystick was properly initialised */

static joypos joycentre;          /* joystick centre position               */
static joypos joymin;             /* left-upper corner position             */
static joypos joymax;             /* right-lower corner position            */
static joypos joy_lowmargin;      /* start of 'dead' region                 */
static joypos joy_highmargin;     /* end of 'dead' region                   */

static int _JoyGetPos (joypos *jp)
{
 unsigned tmp;
 tmp=JoyGetPos ();
 jp->x=(unsigned) (tmp&0xFFFF);
 if (jp->x>=10000)
  return 0;
 jp->y=(unsigned) (tmp>>16);
 if (jp->y>=10000)
  return 0;
 return 1;
}

static int Joy_Init (void)
{
 joypos jp;
 if (!_JoyGetPos (&jp))
  return 0;
 joycentre.x=jp.x;
 joycentre.y=jp.y;
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
  fprintf (stderr,"Move joystick to top left and press any key\n");
  getch ();
  _JoyGetPos (&joymin);
  fprintf (stderr,"Move joystick to bottom right and press any key\n");
  getch ();
  _JoyGetPos (&joymax);
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
static void JoySortOutAnalogue (joypos *jp)
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
 joypos jp;
 unsigned char joystatus;
 if (!gotjoy)
  return;
 _JoyGetPos (&jp);
 joystatus=inportb (0x201);
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
   if ((joystatus&0x10)==0)
    JoystickJoyState[0]&=(swapbuttons&1)? 0xFFBF:0xBFFF;
   if ((joystatus&0x20)==0) 
    JoystickJoyState[0]&=(swapbuttons&1)? 0xBFFF:0xFFBF;
   if ((joystatus&0x40)==0)
    JoystickJoyState[0]=(JoyState[0]&0xFFF0)|12;
   if ((joystatus&0x80)==0)
    JoystickJoyState[0]=(JoyState[0]&0xFFF0)|13;
   break;
  case 2:
   JoySortOutAnalogue (&jp);
   mouse_x=jp.x;
   mouse_y=jp.y;
   mouse_buttons=0;
   if ((joystatus&0x10)==0)
    mouse_buttons|=(swapbuttons&1)? 2:1;
   if ((joystatus&0x20)==0) 
    mouse_buttons|=(swapbuttons&1)? 1:2;
   if ((joystatus&0x40)==0)
    mouse_buttons|=4;
   if ((joystatus&0x80)==0)
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
   if ((joystatus&0x10)==0)
    JoystickJoyState[1]&=(swapbuttons&1)? 0xFBFF:0xFEFF;
   if ((joystatus&0x20)==0) 
    JoystickJoyState[1]&=(swapbuttons&1)? 0xFEFF:0xFBFF;
   if ((joystatus&0x40)==0)
    JoystickJoyState[1]&=0xF7FF;
   if ((joystatus&0x80)==0)
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
 VGA_Reset ();
 if(Verbose) printf("\n\nShutting down...\n");
 TrashSound ();
 RestoreTimer ();
 Keyb_Reset ();
 ExitStacks ();
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
/** Allocate resources needed by MS-DOS-dependent code                     **/
/****************************************************************************/
int InitMachine(void)
{
 int i,j,c;
 FILE *bitmapfile,*snapshotfile;
 if (Verbose) printf ("Initialising MSDOS drivers...\n");
 if (videomode>2 || videomode<0)
  videomode=0;
 JoyState[0]=JoyState[1]=MouseJoyState[0]=MouseJoyState[1]=
 JoystickJoyState[0]=JoystickJoyState[1]=0x7F7F;
 cs_alias=_my_ds();
 DosSelector=_dos_ds;
 if (Verbose)
  printf ("  Allocating stack space... ");
 i=InitStacks (NUM_STACKS,STACK_SIZE);
 if (!i)
 {
  if (Verbose) puts ("FAILED\n");
  return 0;
 }
 memset (DisplayBuf,0,sizeof(DisplayBuf));
 for (i=0;i<16;++i)
  PalBuf[i]=i+1;
 if (Verbose)
  printf ("OK\n  Initialising conversion buffer... ");
 for (i=0;i<16;++i)
  for (j=0;j<16;++j)
   for (c=0;c<16;++c)
   {
    unsigned a;
    a=0;
#ifdef LSB_FIRST
    if (c&0x08) a|=PalBuf[i]; else a|=PalBuf[j];
    if (c&0x04) a|=(PalBuf[i]<<8); else a|=(PalBuf[j]<<8);
    if (c&0x02) a|=(PalBuf[i]<<16); else a|=(PalBuf[j]<<16);
    if (c&0x01) a|=(PalBuf[i]<<24); else a|=(PalBuf[j]<<24);
#else
    if (c&0x01) a|=PalBuf[i]; else a|=PalBuf[j];
    if (c&0x02) a|=(PalBuf[i]<<8); else a|=(PalBuf[j]<<8);
    if (c&0x04) a|=(PalBuf[i]<<16); else a|=(PalBuf[j]<<16);
    if (c&0x08) a|=(PalBuf[i]<<24); else a|=(PalBuf[j]<<24);
#endif
    font2screen[i*16*16+j*16+c]=a;
   }
 if (Verbose) puts ("OK");
 InitSound ();
 if (expansionmode==2 || expansionmode==3 ||
     (expansionmode!=1 && expansionmode!=4 && joystick))
 {
  if (Verbose)
   printf ("  Detecting joystick... ");
  i=Joy_Init ();
  if (Verbose)
   printf ((i)? "Found\n" : "Not found\n");
 }
 if (expansionmode==1 || expansionmode==4 ||
     expansionmode==5 || expansionmode==6 ||
     expansionmode==7)
 {
  if (Verbose)
   printf ("  Detecting mouse... ");
  i=Mouse_Detect ();
  if (Verbose)
   printf ((i)? "Found\n" : "Not found\n");
 }
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
 StartTimer ();
 if (syncemu)
  OldTimer=ReadTimer ();
 /* Parse keyboard mapping string */
 sscanf (szKeys,"%02X%02X%02X%02X%02X%02X%02X%02X",
         &KEY_LEFT,&KEY_RIGHT,&KEY_UP,&KEY_DOWN,
         &KEY_BUTTONA,&KEY_BUTTONB,&KEY_BUTTONC,&KEY_BUTTOND);
 Keyb_Init ();
 while ((bitmapfile=fopen(szBitmapFile,"rb"))!=NULL)
 {
  fclose (bitmapfile);
  if (!NextBitmapFile())
   break;
 }
 while ((snapshotfile=fopen(szSnapshotFile,"rb"))!=NULL)
 {
  fclose (snapshotfile);
  if (!NextSnapshotFile())
   break;
 }
 if (Verbose)
 {
  printf ("  Next screenshot will be %s\n"
          "  Next snapshot will be %s\n",
          szBitmapFile,szSnapshotFile);
 }
 if (Verbose)
 {
  fprintf (stderr,"Press space to run rom code...\n");
  while (!keybstatus[VK_Space]);
 }
 makeshot=0; makesnap=0;
 for (i=0;i<16*3;++i) VGA_Palette[i+3]=Coleco_Palette[i];
 VGA_Init ();
 VGA_SetBkColour (0);
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
  case VK_1:
  case VK_End:
   nOr=1;
   break;
  case VK_2:
  case VK_Down:
   nOr=2;
   break;
  case VK_3:
  case VK_PageDown:
   nOr=3;
   break;
  case VK_4:
  case VK_Left:
   nOr=4;
   break;
  case VK_5:
  case VK_NumPad5:
   nOr=5;
   break;
  case VK_6:
  case VK_Right:
   nOr=6;
   break;
  case VK_7:
  case VK_Home:
   nOr=7;
   break;
  case VK_8:
  case VK_Up:
   nOr=8;
   break;
  case VK_9:
  case VK_PageUp:
   nOr=9;
   break;
  case VK_Equal:
  case VK_Enter:
   nOr=10;
   break;
  case VK_Minus:
  case VK_Del:
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
  for (i=VK_1;i<=VK_Equal;++i)
   if (keybstatus[i])
   {
    tmp=numkeypressed(i);
    JoyState[0]=(JoyState[0]&0xFFF0)|tmp;
   }
  if (keybstatus[KEY_BUTTONC])
   JoyState[0]=(JoyState[0]&0xFFF0)|12;
  if (keybstatus[KEY_BUTTOND])
   JoyState[0]=(JoyState[0]&0xFFF0)|13;
  for (i=VK_Home;i<=VK_PageUp;++i)
   if (keybstatus[i])
   {
    tmp=numkeypressed((keypadmode)? (i-VK_Home+VK_End) : i);
    JoyState[1]=(JoyState[1]&0xFFF0)|tmp;
   }
  for (i=VK_Left;i<=VK_Right;++i)
   if (keybstatus[i])
   {
    tmp=numkeypressed(i);
    JoyState[1]=(JoyState[1]&0xFFF0)|tmp;
   }
  for (i=VK_End;i<=VK_PageDown;++i)
   if (keybstatus[i])
   {
    tmp=numkeypressed((keypadmode)? (i-VK_End+VK_Home) : i);
    JoyState[1]=(JoyState[1]&0xFFF0)|tmp;
   }
  if (keybstatus[VK_Insert])
  {
   tmp=numkeypressed((keypadmode)? VK_Del : VK_Insert);
   JoyState[1]=(JoyState[1]&0xFFF0)|tmp;
  }
  if (keybstatus[VK_Del])
  {
   tmp=numkeypressed((keypadmode)? VK_Insert : VK_Del);
   JoyState[1]=(JoyState[1]&0xFFF0)|tmp;
  }
  if (keybstatus[VK_Enter+128])
  {
   tmp=numkeypressed(VK_Enter);
   JoyState[1]=(JoyState[1]&0xFFF0)|tmp;
  }
  if (keybstatus[VK_PlusNumPad])
   JoyState[1]=(JoyState[1]&0xFFF0)|13;
  if (keybstatus[VK_MinusNumPad])
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
extern byte *RAM;
void Keyboard (void)
{
 int tmp;
 /* Check if reset combination is pressed */
 if (AltPressed())
 {
  if (keybstatus[VK_F12]) ResetColeco (0);
  if (keybstatus[VK_F11]) ResetColeco (1);
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
 /* Check if video mode should be switched */
 if (SwitchVideoMode)
 {
  SwitchVideoMode=0;
  AutoText80=0;
  if (Text80)
   ResetText80();
  else
   InitText80();
 }
 /* Check is PAUSE is pressed */
 if (PausePressed)
 {
  StopSound ();
  tmp=0;
  if (PausePressed==2)
  {
   tmp=1;
   VGA_SetBkColour (1);
   inportb (0x3BA);
   inportb (0x3DA);
   outportb (0x3C0,0);
  }
  while (PausePressed);
  if (tmp)
  {
   VGA_SetBkColour (VDP.BGColour);
   inportb (0x3BA);
   inportb (0x3DA);
   outportb (0x3C0,0x20);
  }
  ResumeSound ();
  if (syncemu)
   OldTimer=ReadTimer ();
 }
 /* Check if a screen shot should be taken */
 if (makeshot)
 {
  sound (440);
  WriteBitmap (szBitmapFile,8,17,WIDTH,256,192,
               (char*)DisplayBuf+(WIDTH-256)/2+(HEIGHT-192)*WIDTH,(char*)VGA_Palette);
  NextBitmapFile ();
  makeshot--;
  nosound ();
 }
 if (makesnap)
 {
  sound (220);
  SaveSnapshotFile (szSnapshotFile);
  NextSnapshotFile ();
  nosound ();
  makesnap--;
 }
#ifdef DEBUG
 if (dumpram)
 {
  FILE *f;
  char buf[20];
  static int dumpram_count=0;
  sprintf (buf,"RAM.%03d",dumpram_count);
  dumpram_count++; if (dumpram_count>999) dumpram_count=0;
  f=fopen (buf,"wb");
  for (tmp=0;tmp<256;++tmp) fwrite (AddrTabl[tmp],1,256,f);
  fclose (f);
  dumpram--;
 }
#endif
 /* Check if OptionsDialogue() should be called */
 if (calloptions)
 {
  in_options_dialogue=1;
  calloptions=0;
  StopSound ();
  VGA_Reset ();
  fflush (stdin);
  OptionsDialogue ();
  VGA_Init ();
  if (Text80) InitText80();
  in_options_dialogue=0;
  ResumeSound ();
  RefreshScreen (1);
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
  OldTimer+=1192380/IFreq;
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

/****************************************************************************/
/** Screen refresh drivers                                                 **/
/****************************************************************************/
#define USE_LOOKUP_TABLE              /* Use font2screen for faster refresh */
#define _8BPP                         /* We have an 8 bpp video device      */

#include "Common.h"
