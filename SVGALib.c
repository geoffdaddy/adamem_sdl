/** ADAMEm: Coleco ADAM emulator ********************************************/
/**                                                                        **/
/**                                SVGALib.c                               **/
/**                                                                        **/
/** This file contains the SVGALib specific routines. It does not include  **/
/** the sound emulation code                                               **/
/**                                                                        **/
/** Copyright (C) Marcel de Kogel 1996,1997,1998,1999                      **/
/**     You are not allowed to distribute this software commercially       **/
/**     Please, notify me, if you make any changes to this file            **/
/****************************************************************************/

#include "Coleco.h"
#include "SVGALib.h"
#include "Bitmap.h"
#include "Unix.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>
#include <asm/io.h>
#include <linux/keyboard.h>
#include <vga.h>
#include <vgakeyboard.h>
#include <termio.h>

/* Title for -help output */
char Title[]="ADAMEm Linux/SVGALib 1.81";

/* Key mapping */
static int KEY_LEFT,KEY_RIGHT,KEY_UP,KEY_DOWN,
           KEY_BUTTONA,KEY_BUTTONB,KEY_BUTTONC,KEY_BUTTOND;
static byte VGA_Palette[17];      /* Coleco Palette                         */
static byte PalBuf[16],Pal0;      /* Palette buffer                         */
static int default_mouse_sens=200;/* Default mouse sensitivity              */
static int keyboardmode;          /* 0=joystick, 1=keyboard                 */
static word MouseJoyState[2];     /* Current mouse status                   */
static word JoystickJoyState[2];  /* Current joystick status                */
static unsigned font2screen[16*16*16];
                                  /* Used by screen refresh driver          */
static int makeshot=0;            /* 1 if screen shot should be taken       */
static int makesnap=0;            /* 1 if snapshot should be written        */
static int OldTimer=0;            /* Last value of timer                    */
static int NewTimer=0;            /* New value of timer                     */
static int calloptions=0;         /* If 1, OptionsDialogue() is called      */
static struct termios termold;    /* Original terminal settings             */

byte DisplayBuf[WIDTH*(HEIGHT+16)];    /* Screen buffer                          */
char szBitmapFile[256];           /* Next screen shot file                  */
char szSnapshotFile[256];         /* Next snapshot file                     */
char szJoystickFileName[256];     /* File holding joystick information      */
char *szKeys="696A676C381D2A2C";  /* Key scancodes                          */
int  mouse_sens=200;              /* Mouse sensitivity                      */
int  keypadmode=0;                /* 1 if keypad should be reversed         */
int  joystick=1;                  /* Joystick support                       */
int  calibrate=0;                 /* Set to 1 to force joystick calibration */
int  swapbuttons=0;               /* 1=joystick, 2=keyboard, 4=mouse        */
int  expansionmode=0;             /* Expansion module emulated              */
int  useoverscan=1;               /* Overscan colour support                */
int  videomode=0;                 /* 0=320x200   2=320x240                  */
int  syncemu=1;                   /* 0 if emulation shouldn't be synced     */
int  chipset=1;                   /* If 0, do not detect SVGA               */

static inline volatile void outportb (word _port,byte _data)
{
 outb (_data,_port);
}

static inline volatile void outportw (word _port,word _data)
{
 outw (_data,_port);
}

static inline volatile byte inportb (word _port)
{
 return inb (_port);
}

static void VGA_SetBkColour (unsigned char colour)
{
 static int currbkcol=-1;
 if (currbkcol==colour)
  return;
 currbkcol=colour;
 VGA_Palette[0]=VGA_Palette[3]=Coleco_Palette[currbkcol*3+0];
 VGA_Palette[1]=VGA_Palette[4]=Coleco_Palette[currbkcol*3+1];
 VGA_Palette[2]=VGA_Palette[5]=Coleco_Palette[currbkcol*3+2];
 if (useoverscan)
  vga_setpalette (0,VGA_Palette[0]/4,VGA_Palette[1]/4,VGA_Palette[2]/4);
 vga_setpalette (1,VGA_Palette[3]/4,VGA_Palette[4]/4,VGA_Palette[5]/4);
}

static void PutImage (void)
{
 char *p,*z;
 int i,j;
 p=vga_getgraphmem ();
 z=DisplayBuf;
 switch (videomode)
 {
  case 0:
   p+=320*(200-192)/2+(320-256)/2;
   for (i=0;i<192;i++,p+=320,z+=256)
    for (j=0;j<256;j+=4)
     *(unsigned*)(p+j)=*(unsigned*)(z+j);
   break;
  case 1:
   p+=(320/4)*(240-192)/2+(320-256)/8;
   for (i=0;i<192;++i,p+=320/4,z+=256)
   {
    outportw (0x3C4,0x102);
    for (j=0;j<64;++j)
     p[j]=z[j*4+0];
    outportw (0x3C4,0x202);
    for (j=0;j<64;++j)
     p[j]=z[j*4+1];
    outportw (0x3C4,0x402);
    for (j=0;j<64;++j)
     p[j]=z[j*4+2];
    outportw (0x3C4,0x802);
    for (j=0;j<64;++j)
     p[j]=z[j*4+3];
   }
 }
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
 0,27,'1','2','3','4','5','6','7','8','9','0','-','=',8,
 9,'q','w','e','r','t','y','u','i','o','p','[',']',13,
 0,'a','s','d','f','g','h','j','k','l',';',39,'`',
 0,92,'z','x','c','v','b','n','m',',','.','/',0,
 '*',0,' ',0,129,130,131,132,133,134,144,145,0,0,
 0,0,171,160,168,'-',163,128,161,'+',170,162,169,148,151,
 0,0,0,0,0,0,0,0,0,0,0,0,
 0,0,'/',0,0,0,146,160,150,163,161,147,162,149,148,151,
 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};
static const byte scan2ascii_shift[NR_KEYS] =
{
 0,27,'!','@','#','$','%','^','&','*','(',')','_','+',184,
 185,'Q','W','E','R','T','Y','U','I','O','P','{','}',13,
 0,'A','S','D','F','G','H','J','K','L',':',34,'~',
 0,'|','Z','X','C','V','B','N','M','<','>','?',0,
 '*',0,' ',0,137,138,139,140,141,142,152,153,0,0,
 0,0,0,0,0,'-',0,0,0,'+',0,0,0,156,159,
 0,0,0,0,0,0,0,0,0,0,0,0,
 0,0,'/',0,0,0,154,0,158,0,0,155,0,157,156,159,
 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};
static const byte scan2ascii_ctrl[NR_KEYS] =
{
 0,27,0,255,0,0,0,30,0,0,0,0,31,0,127,
 0,17,23,5,18,20,25,21,9,15,16,27,29,13,
 0,1,19,4,6,7,8,10,11,12,0,0,0,
 0,28,26,24,3,22,2,14,13,0,0,0,0,
 0,0,32,0,0,0,0,0,0,0,0,0,0,0,
 0,0,0,164,0,0,167,0,165,0,0,166,0,0,0,
 0,0,0,0,0,0,0,0,0,0,0,0,
 0,0,0,0,0,0,0,164,0,167,165,0,166,0,0,0,
 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
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
 static int CapsLock=0;
 if (code<0 || code>=NR_KEYS)
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
    case SCANCODE_CAPSLOCK:
     CapsLock^=1;
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
    case SCANCODE_F11:
     if (!PausePressed && !AltPressed())
      DecreaseSoundVolume ();
     break;
    case SCANCODE_F12:
     if (!PausePressed && !AltPressed())
      IncreaseSoundVolume ();
     break;
    case SCANCODE_F10:
     Z80_Running=0;
     break;
    case SCANCODE_F8:
     if (!keyboardmode || AltPressed())
      makeshot=1;
     break;
    case SCANCODE_F7:
     if (!keyboardmode || AltPressed())
      makesnap=1;
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
  if (keyboardmode && !AltPressed())  /* Modify keyboard buffer             */
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
   if (CapsLock)
   {
    if (code>='a' && code<='z') code+='A'-'a';
    else if (code>='A' && code<='Z') code+='a'-'A';
   }
   if (code) LocalAddToKeyboardBuffer (code);
  }
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
  vga_flip ();
  fprintf (stderr,"Move joystick to top left and press CR\n");
  fflush (stdin); fgetc (stdin);
  ReadJoystick (&joymin);
  fprintf (stderr,"Move joystick to bottom right and press CR\n");
  fflush (stdin); fgetc (stdin);
  vga_flip ();
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
 keyboard_close ();
 vga_setmode (TEXT);
 tcsetattr (0,0,&termold);
 TrashSound ();
 TrashJoystick ();
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

static void signal_fatal_handler(int s)
{
 printf("Signal %s (%d) received\n",strsignal(s),s);
 signal (s,signal_fatal_handler);
 TrashMachine ();
 raise (SIGKILL);
}

static void signal_quit_handler(int s)
{
 signal (s,signal_quit_handler);
 Z80_Running=0;
}

static int signals_fatal[] =
{
 SIGHUP,  SIGINT,
 SIGILL,  SIGIOT,
 SIGBUS,  SIGFPE,
 SIGSEGV, SIGPIPE,
 SIGXCPU, SIGXFSZ,
 SIGPROF, SIGPWR,
 0
};

static int signals_quit[] =
{
 SIGQUIT, SIGTRAP,
 SIGALRM, SIGTERM,
 SIGVTALRM,
 0
};

/****************************************************************************/
/** Allocate resources needed by Linux/SVGALib-dependent code              **/
/****************************************************************************/
int InitMachine(void)
{
 int i,j,c;
 FILE *bitmapfile,*snapshotfile;
 memset (VGA_Palette,0,sizeof(VGA_Palette));
 memcpy (VGA_Palette+3,Coleco_Palette,16*3);
 if (Verbose) printf ("Initialising Linux/SVGALib drivers...\n");
 tcgetattr (0,&termold);
 if (!chipset) vga_setchipset (VGA);
 vga_init ();
 if (Verbose) printf ("  Setting VGA mode... ");
 switch (videomode)
 {
  case 1:  i=G320x240x256;
           break;
  default: i=G320x200x256;
           videomode=0;
           break;
 }
 i=vga_setmode (i);
 if (i) { if (Verbose) puts ("FAILED"); return 0; }
 if (videomode)
 {
  outportw (0x3C4,0xF02);
  memset (vga_getgraphmem(),0,65536);
 }
 for (i=0;i<17;++i)
  vga_setpalette (i,VGA_Palette[i*3+0]/4,
                    VGA_Palette[i*3+1]/4,
                    VGA_Palette[i*3+2]/4);
 JoyState[0]=JoyState[1]=MouseJoyState[0]=MouseJoyState[1]=
 JoystickJoyState[0]=JoystickJoyState[1]=0x7F7F;
 memset (DisplayBuf,0,sizeof(DisplayBuf));
 for (i=0;i<16;++i)
  PalBuf[i]=i+1;
 Pal0=2;
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
 sscanf (szKeys,"%02X%02X%02X%02X%02X%02X%02X%02X",
         &KEY_LEFT,&KEY_RIGHT,&KEY_UP,&KEY_DOWN,
         &KEY_BUTTONA,&KEY_BUTTONB,&KEY_BUTTONC,&KEY_BUTTOND);
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
 if (Verbose) printf ("  Initialising keyboard... ");
 if (keyboard_init()) { if (Verbose) puts ("FAILED"); return 0; }
 keyboard_translatekeys (DONT_CATCH_CTRLC);
 keyboard_seteventhandler (keyb_handler);
 if (Verbose) puts ("OK");
 VGA_SetBkColour (0);
 Mouse_Init ();
 keyboardmode=(EmuMode)? 1:0;
 for (i=0;signals_fatal[i];++i) signal (i,signal_fatal_handler);
 for (i=0;signals_quit[i];++i) signal (i,signal_quit_handler);
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
  for (i=SCANCODE_1;i<=SCANCODE_EQUAL;++i)
   if (keybstatus[i])
   {
    tmp=numkeypressed(i);
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
 }
 while (tmp);
 /* Check mouse and joystick events */
 Joysticks ();
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
  while (PausePressed) keyboard_update();
  if (tmp)
  {
   VGA_SetBkColour (PalBuf[0]-1);
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
  WriteBitmap (szBitmapFile,8,17,WIDTH,256,192,
               DisplayBuf+(WIDTH-256)/2+(HEIGHT-192)*WIDTH,VGA_Palette);
  NextBitmapFile ();
  makeshot--;
 }
 if (makesnap)
 {
  SaveSnapshotFile (szSnapshotFile);
  NextSnapshotFile ();
  makesnap--;
 }
 /* Check if OptionsDialogue() should be called */
 if (calloptions)
 {
  struct termios term; 
  calloptions=0;
  StopSound ();
  vga_flip ();
  keyboard_close ();
  tcgetattr (0,&term);
  tcsetattr (0,0,&termold);
  fflush (stdin);
  OptionsDialogue ();
  tcsetattr (0,0,&term);
  keyboard_init ();
  keyboard_translatekeys (DONT_CATCH_CTRLC);
  keyboard_seteventhandler (keyb_handler);
  vga_flip ();
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

/****************************************************************************/
/** Screen refresh drivers                                                 **/
/****************************************************************************/
#define USE_LOOKUP_TABLE              /* Use font2screen for faster refresh */
#define _8BPP                         /* We have an 8 bpp video device      */

#include "Common.h"
