/** ADAMEm: Coleco ADAM emulator ********************************************/
/**                                                                        **/
/**                                 keys.c                                 **/
/**                                                                        **/
/** Program to get alternative keyboard mappings for ADAMEm MS-DOS         **/
/**                                                                        **/
/** Copyright (C) Marcel de Kogel 1996,1997,1998,1999                      **/
/**     You are not allowed to distribute this software commercially       **/
/**     Please, notify me, if you make any changes to this file            **/
/****************************************************************************/

#include <stdio.h>

#ifdef MSDOS
#include <sys/farptr.h>
#include <dpmi.h>
#include <go32.h>
#include <crt0.h>
#include <pc.h>

int _crt0_startup_flags = _CRT0_FLAG_NONMOVE_SBRK |
                          _CRT0_FLAG_LOCK_MEMORY  |
                          _CRT0_FLAG_DROP_EXE_SUFFIX;


static __volatile__ int key=0;
static _go32_dpmi_seginfo newirqkeyb,oldirqkeyb;

void keyb_interrupt (void)
{
 unsigned code;
 static int extkey;
 code=inportb (0x60);           /* get scancode */
 if (code<0xE0)                 /* ignore codes >0xE0 */
 {
  if (code & 0x80)              /* key is released */
   key=0;
  else                          /* key is pressed */
  {
   if (extkey)
    key=code+128;
   else
    key=code;
  }
  extkey=0;
 }
 else
  if (code==0xE0)
   extkey=1;
}

static int keyb_init (void)
{ 
 _go32_dpmi_get_protected_mode_interrupt_vector (9,&oldirqkeyb);
 newirqkeyb.pm_offset=(int)keyb_interrupt;
 newirqkeyb.pm_selector=_go32_my_cs();
 _go32_dpmi_chain_protected_mode_interrupt_vector (9,&newirqkeyb);
 return 1;
}

static void keyb_exit (void)
{
 _go32_dpmi_set_protected_mode_interrupt_vector (9,&oldirqkeyb);
}

static int _getkey (char *s)
{
 printf ("Press key for %s\n",s);
 key=0;
 while (!key);
 _farpokew (_dos_ds,0x41A,_farpeekw(_dos_ds,0x41C));    /* clear buffer */
 return key;
}
#endif

#ifdef LINUX_SVGA
#include <linux/keyboard.h>
#include <vgakeyboard.h>

static int key=0;

void keyb_handler (int code,int newstatus)
{
 if (code<0 || code>NR_KEYS) return;
 if (newstatus) key=code;
}

static int keyb_init (void)
{ 
 if (keyboard_init())
 {
  puts ("Could not initialise keyboard handler");
  return 0;
 }
 keyboard_translatekeys (DONT_CATCH_CTRLC);
 keyboard_seteventhandler (keyb_handler);
 return 1;
}

static void keyb_exit (void)
{
 keyboard_close ();
}

static int _getkey (char *s)
{
 printf ("Press key for %s\n",s);
 key=0;
 while (!key) keyboard_update();
 return key;
}
#endif

#if defined(MSDOS) || defined(LINUX_SVGA)
int main (void)
{
 int left,right,up,down,firea,fireb,start,select;
 if (!keyb_init()) return 1;
 left=_getkey ("LEFT");
 right=_getkey ("RIGHT");
 up=_getkey ("UP");
 down=_getkey ("DOWN");
 firea=_getkey ("BUTTON A");
 fireb=_getkey ("BUTTON B");
 start=_getkey ("BUTTON C");
 select=_getkey ("BUTTON D");
 printf ("\n-keys %02X%02X%02X%02X%02X%02X%02X%02X\n",
         left,right,up,down,firea,fireb,start,select);
 keyb_exit ();
 return 0;
}
#else
int main (void)
{
 puts ("Sorry, redefining keys is currently only supported "
       "in the MS-DOS and Linux/SVGALib versions");
 return 0;
}
#endif
