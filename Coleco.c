/** ADAMEm: Coleco ADAM emulator ********************************************/
/**                                                                        **/
/**                                Coleco.c                                **/
/**                                                                        **/
/** This file contains the Coleco-specific emulation code                  **/
/**                                                                        **/
/** Copyright (C) Marcel de Kogel 1996,1997,1998,1999                      **/
/**     You are not allowed to distribute this software commercially       **/
/**     Please, notify me, if you make any changes to this file            **/
/****************************************************************************/

#include "Coleco.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>

#if defined(WIN32) || defined(MSDOS)
  #if defined(IDEHD)
    #include "IDE/HarddiskIDE.h"
  #endif
#else
  #if defined(IDEHD)
    #include "HarddiskIDE.h"
  #endif
#endif

#ifdef ZLIB
#include <zlib.h>
#endif

/* #define PRINT_IO  */           /* Print accesses to unused I/O ports     */
/* #define PRINT_MEM */           /* Print writes to ROM                    */

VDP_t VDP;                        /* VDP parameters                         */
int  Z80_IRQ=Z80_IGNORE_INT;      /* Z80 IRQ line status                    */
int  EmuMode=0;                   /* 0=ColecoVision, 1=ADAM                 */
int  sgmmode=0;                   /* Super Game Module support              */
int  ntscFilter = 0;              /* Blarggs SMS NTSC Filter                */      
int  UPeriod;                     /* Number of interrupts/screen update     */
int  IFreq;                       /* VDP Interrupt frequency                */
int  Verbose=1;                   /* Debug msgs ON/OFF                      */
int  JoyState[2];                 /* Joystick status                        */
int  SpinnerPosition[2];          /* Spinner positions [0..500]             */
int  PrnType=1;                   /* Type of printer attached               */
int  LastSprite[256];             /* Last sprite to be displayed in a row   */
int  Support5thSprite=0;          /* Show only 4 sprites per row            */
int  RAMPages=1;                  /* Number of 64K expansion RAM pages      */
int  SaveSnapshot=0;              /* If 1, auto-save snapshot               */
int  Cheats[16];                  /* Cheats to patch into game ROM          */
int  CheatCount;                  /* Number of cheats                       */
int  DiskSpeed=100;               /* Time in ms it takes to read one...     */
int  TapeSpeed=100;               /* ... block                              */
char *CartName   = "CART.rom";    /* Cartridge ROM file                     */
char *OS7File    = "OS7.rom";     /* ColecoVision ROM file                  */
char *EOSFile    = "EOS.rom";     /* EOS ROM file                           */
char *WPFile     = "WP.rom";      /* SmartWriter ROM file                   */
char *ExpRomFile = "EXP.rom";     /* Expansion ROM file                     */
                                  /* Hard Disk Image files                  */
#if defined(IDEHD)
char *HardDiskFile[2] =           /* Default hard drive filenames           */
 { "hd_ide1.img","hd_ide2.img" }; 
#endif
char *SoundName;                  /* Sound log file                         */
char *DiskName[4];                /* Disk images                            */
char *TapeName[4];                /* Tape images                            */
char *SnapshotName;               /* Snapshot file name                     */
char *PrnName;                    /* Printer log file                       */
char *LPTName;                    /* Parallel port log file                 */
byte *RAM;                        /* Main and expansion RAM                 */
byte *ROM,*OS7,*EOS,*WP,*EXPROM; /* ROM pointers                            */
byte *CART = 0;
byte *AddrTabl[256];              /* Currently mapped in pages              */
byte *WriteAddrTabl[256];         /* Used to write protect ROM              */
byte PCBTable[65536];             /* 1 if address is part of PCB            */
static byte DummyWriteTabl[256];  /* ROM                                    */
static byte DummyReadTabl[256];   /* Non-existent memory                    */
static byte CurrMemSel=0;         /* Current memory bank selection          */
static byte ADAMNet=0;            /* Value last written to ADAMNet          */
static int  RAMMask;              /* Mask for expansion RAM pages           */
static int  RAMPage;              /* Current expansion RAM page selected    */
static int  SpinnerJoyState[2];   /* Current spinner status                 */
static int  SpinnerRecur[2];      /* Generate INT when >=0x10000            */
static int  SpinnerParam[2];      /* Value to add to SpinnerRecur           */
static int  JoyMode=0;            /* Joyst. controller mode                 */
static int  RAMSize;              /* Size of RAM in KB                      */
static int  ROMSize;              /* Size of ROM in KB                      */
int  CartSize;                    /* Size of Cartridge ROM in KB            */
int  CartBanks;                   /* Number of banks in a bankswitched cart */
int  CurrCartBank;                /* The currently selected cart bank       */
static int  PCB=65216;            /* Current PCB address                    */
static int  UsePCB=0;             /* Set to 1 after ADAMNet has been reset  */
static int  NMI_Line=0;           /* Set to 1 if NMI is high                */
static int  numVDPInts;           /* Incremented every VDP interrupt        */
static int  IFreqRecur=0;         /* Generate VDP int when this overflows   */
static int  IFreqParam=0;         /* Add this to IFreqRecur every interrupt */
static FILE *PrnStream;           /* Printer stream                         */
static FILE *LPTStream;           /* Parallel port stream                   */
static FILE *DiskStream[4];       /* Disk image streams                     */
static FILE *TapeStream[4];       /* Tape image streams                     */
static FILE *SoundStream;         /* Sound log stream                       */
static char tempfiles[8][L_tmpnam]; /* Used for GZIPped disk/tape support   */
static int  DiskChanged[4];       /* Set to 1 if disk is written to         */
static int  TapeChanged[4];       /* Set to 1 if tape is written to         */
static int  DiskSize[4];          /* Size of disk images in kilobytes       */
static int  TapeSize[4];          /* Size of tape images in kilobytes       */
static int  DiskMask[4];          /* Mask for disk block nr                 */
static int  TapeMask[4];          /* Mask for tape block nr                 */
static byte disk_newstatus[4]=    /* Next status byte for disk drives       */
 { 0x80,0x80,0x80,0x80 };
static byte disk_status[4]=       /* Current status byte. Set to newstatus  */
 { 0x80,0x80,0x80,0x80 };         /* when requesting any DCB operation      */
static int  disk_lastblock[4]=    /* Last block read from disk drives       */
 { 0,0,0,0 };
static byte tape_newstatus[4]=    /* Next status byte for tape drives       */
 { 0x80,0x80,0x80,0x80 };
static byte tape_status[4]=       /* Current status byte. Set to newstatus  */
 { 0x80,0x80,0x80,0x80 };         /* when requesting any DCB operation      */
static int  tape_lastblock[4]=    /* Last block read from tape drives       */
 { 0,0,0,0 };
static int  disk_timeout[4]=      /* Number of ms the disk device is busy   */
 { 0,0,0,0 };
static int  tape_timeout[4]=      /* Number of ms the tape device is busy   */
 { 0,0,0,0 };
static int  sound_reg[8]=         /* Sound chip registers                   */
 { -1,-1,-1,-1,-1,-1,-1,-1 };
static int  sound_data;           /* Last value written to sound chip       */

int  PalNum;                      /* Palette number                         */

static byte usartCmdReg;
static byte usartModeReg[2];
static byte usartWMRIndex = 0;
static byte usartRMRIndex = 0;
static byte sgmmodereg = 0;

#ifdef SOUND_PSG
/* AY-8910 PSG Emulation */
static byte PSGControlReg;
byte PSGReg[16];
#endif


/* Hard Disk Emulation */
#ifdef IDEHD
harddiskController *IDE_Controller;
#endif

#if defined(IDEHD)
//static byte AltStatReg;
static byte DigInReg;
static word HardDiskWriteData;
#endif

/* Several palettes */
byte Palettes[NR_PALETTES][16*3] =
{
 /* Coleco ADAM's TMS9918 palette */
 {
    0,  0,  0,    0,  0,  0,   71,183, 59,  124,207,111,
   93, 78,255,  128,114,255,  182, 98, 71,   93,200,237,
  215,107, 72,  251,143,108,  195,205, 65,  211,218,118,
   62,159, 47,  182,100,199,  204,204,204,  255,255,255
 },
 /* Default V9938 palette */
 {
    0,  0,  0,    0,  0,  0,   32,192, 32,   96,224, 96,
   32, 32,224,   64, 96,224,  160, 32, 32,   64,192,224,
  224, 32, 32,  224, 96, 96,  192,192, 32,  192,192,128,
   32,128, 32,  192, 64,160,  160,160,160,  224,224,224
 },
 /* TMS9918 Black & White palette */
 {
    0,  0,  0,    0,  0,  0,  136,136,136,  172,172,172,
  102,102,102,  134,134,134,  120,120,120,  172,172,172,
  136,136,136,  172,172,172,  187,187,187,  205,205,205,
  118,118,118,  135,135,135,  204,204,204,  255,255,255
 },
 /* V9938 Black & White palette */
 {
    0,  0,  0,    0,  0,  0,  144,144,144,  195,195,195,
   60, 60, 60,  115,115,115,   80, 80, 80,  178,178,178,
  102,102,102,  153,153,153,  198,198,198,  211,211,211,
  100,100,100,  129,129,129,  182,182,182,  255,255,255
 }
};

#define DEFAULT_DISK_SIZE       (1440*1024)
#define DEFAULT_TAPE_SIZE       (256*1024)
#define MIN_DISK_SIZE           (160*1024)
#define MIN_TAPE_SIZE           (256*1024)

/* Table to convert key values to Coleco codes */
static const byte KeyCodes[16] =
{
 0x0A,0x0D,0x07,0x0C,0x02,0x03,0x0E,0x05,
 0x01,0x0B,0x06,0x09,0x08,0x04,0x0F,0x0F
};

static void ResetPCB (void);             /* Reset PCB table                 */
static void PrintChar (char c);          /* Send a character to the printer */
void ReadPCB (dword A);                  /* Called when is read from        */
void WritePCB (dword A);                 /* Called when PCB is written to   */

/****************************************************************************/
/*** This function is called when the PCB is relocated or ADAMnet is      ***/
/*** reset to initialise the PCB lookup table                             ***/
/****************************************************************************/
static void SetPCBTable (unsigned oldPCB)
{
 int i;
 PCBTable[oldPCB]=0;
 for (i=0;i<15;++i)
  PCBTable[(oldPCB+4+i*21)&0xFFFF]=0;
 PCBTable[PCB]=1;
 for (i=0;i<15;++i)
  PCBTable[(PCB+4+i*21)&0xFFFF]=i+1;
}

/****************************************************************************/
/*** Function to read/write memory. They check for memory protection and  ***/
/*** reads and writes to the PCB/DCB area                                 ***/
/****************************************************************************/
unsigned Z80_RDMEM (dword a)
{
 int i;
 int OldBank;
 int retval=AddrTabl[a>>8][a&0xFF];
 if (CartBanks > 1 && a>=0xFFC0 && !EmuMode)
 {
     OldBank = CurrCartBank;
     CurrCartBank = (a-0xffc0)%CartBanks;
     if (OldBank != CurrCartBank)
     {
         for (i = 0xC000; i<0xFFFF; i+=256) { //Update the upper bank contents
             AddrTabl[i>>8] = &CART[i-0xC000 + (CurrCartBank*16*1024) ];
         }
     }
 }
 if (PCBTable[a]) ReadPCB (a);
 return retval;
}
void Z80_WRMEM (dword a,byte v)
{
 WriteAddrTabl[a>>8][a&0xFF]=v;
 if (PCBTable[a]) WritePCB (a);
#ifdef PRINT_MEM
 if (WriteAddrTabl[a>>8]==DummyWriteTabl)
  printf ("Wrote %02X to %04X at %04X\n",v,a,Z80_GetPC());
#endif
}

/****************************************************************************/
/*** This function is called when ED FE occurs                            ***/
/****************************************************************************/
void Z80_Patch(Z80_Regs *R)
{
#ifdef DEBUG
 printf ("Unknown patch called at %04X\n",(R->PC.W.l-2)&0xFFFF);
 Z80_RegisterDump ();
#endif
}

/****************************************************************************/
/*** This function is called when a sound register is written to. It      ***/
/*** writes a new stamp to the sound log file if necessary and calles the ***/
/*** machine-dependent Sound() function                                   ***/
/****************************************************************************/
static void DoSound (byte R,word V)
{
 sound_reg[R]=V;
 Sound (R,V);
}

static void WriteIntStamp (void)
{
 if (numVDPInts)
 {
  if (numVDPInts==1)
   fputc (0xFF,SoundStream);
  else
   if (numVDPInts<256)
   {
    fputc (0xFE,SoundStream);
    fputc (numVDPInts&0xFF,SoundStream);
   }
   else
   {
    fputc (0xFD,SoundStream);
    fputc (numVDPInts&0xFF,SoundStream);
    fputc (numVDPInts>>8,SoundStream);
   }
  numVDPInts=0;
 }
}

void UpdateSoundStream (void)
{
 static int old_sound_reg[8]=
 { -1,-1,-1,-1,-1,-1,-1,-1 };
 int i;
 for (i=7;i>=0;--i)
 {
  if (old_sound_reg[i]!=sound_reg[i])
  {
   old_sound_reg[i]=sound_reg[i];
   WriteIntStamp ();
   if (i==0 || i==2 || i==4)
   {
    fputc ((i<<4)|((sound_reg[i]&0xF00)>>8),SoundStream);
    fputc (sound_reg[i]&0xFF,SoundStream);
   }
   else
    fputc ((i<<4)|(sound_reg[i]&0xF),SoundStream);
  }
 }
}

/****************************************************************************/
/*** Functions that write to I/O ports. They are only mapped in if        ***/
/*** necessary                                                            ***/
/****************************************************************************/
static void OutNo (byte Val)
{
}

static void Out02 (byte Val) // IDE - Number of sectors
{
#ifdef IDEHD
    if (IDE_Controller)
        controllerWriteRegister(IDE_Controller, 2, Val);
#else
    Z80_WRMEM((Z80_GetPC()-1)&65535,Val);
#endif
}

static void Out03 (byte Val) // IDE - Sector number
{
#ifdef IDEHD
    if (IDE_Controller)
        controllerWriteRegister(IDE_Controller, 3, Val);
#else
    Z80_WRMEM((Z80_GetPC()-1)&65535,Val);
#endif
}

static void Out04 (byte Val) // IDE - Cylinder low byte
{
#ifdef IDEHD
    if (IDE_Controller)
        controllerWriteRegister(IDE_Controller, 4, Val);
#else
    Z80_WRMEM((Z80_GetPC()-1)&65535,Val);
#endif
}

static void Out05 (byte Val) // IDE - Cylinder high byte
{
#ifdef IDEHD
    if (IDE_Controller)
        controllerWriteRegister(IDE_Controller, 5, Val);
#else
    Z80_WRMEM((Z80_GetPC()-1)&65535,Val);
#endif
}

static void Out06 (byte Val) // IDE - Drive / Head
{
#ifdef IDEHD
    if (IDE_Controller)
        controllerWriteRegister(IDE_Controller, 6, Val);
#else
    Z80_WRMEM((Z80_GetPC()-1)&65535,Val);
#endif
}

static void Out07 (byte Val) // IDE - Command
{
#ifdef IDEHD
    if (IDE_Controller)
        controllerWriteRegister(IDE_Controller, 7, Val);
#else
    Z80_WRMEM((Z80_GetPC()-1)&65535,Val);
#endif
}

static void Out0C (byte Val)
{
#ifdef PRINT_IO
    printf("write 0c: %d\n",Val);
#endif
}

static void Out0D (byte Val)
{
#ifdef PRINT_IO
    printf("write 0d: %d\n",Val);
#endif
}

static void Out0E (byte Val)
{
#ifdef PRINT_IO
    printf("write 0e: %d\n",Val);
#endif
    usartModeReg[usartWMRIndex++] = Val;
    if (usartWMRIndex > 1)
        usartWMRIndex = 0;
}

static void Out0F (byte Val)
{
#ifdef PRINT_IO
    printf("write 0f: %d\n",Val);
#endif
    usartCmdReg = Val;
}

static void Out10 (byte Val)
{
#ifdef PRINT_IO
    printf("Write - MIB2 Serial Port 2 - %02X\n",Val);
#endif
}

static void Out18 (byte Val)
{
#ifdef PRINT_IO
    printf("Write - MIB2 Serial Port 1 - %02X\n",Val);
#endif
}

static void Out20 (byte Val)
{
 int i;
 if (Verbose&4)
  printf ("0x%02X written to ADAMNet at PC=%04X\n",Val,Z80_GetPC());
 if (Val==15) ResetPCB ();
 if ((CurrMemSel&3)==0 && (Val&2)!=(ADAMNet&2))
 {
  if (Val&2)
  {
   if (Verbose&4) puts (" Lower 32K:EOS");
   for (i=0;i<16384;i+=256)
   {
    AddrTabl[i>>8]=DummyReadTabl;
    WriteAddrTabl[i>>8]=DummyWriteTabl;
   }
   for (i=16384;i<24576;i+=256)
   {
    AddrTabl[i>>8]=AddrTabl[(i+8192)>>8]=&EOS[i-16384];
    WriteAddrTabl[i>>8]=WriteAddrTabl[(i+8192)>>8]=DummyWriteTabl;
   }
  }
  else
  {
   if (Verbose &4) puts (" Lower 32K:SmartWriter");
   for (i=0;i<32768;i+=256)
   {
    AddrTabl[i>>8]=&WP[i];
    WriteAddrTabl[i>>8]=DummyWriteTabl;
   }
  }
 }
 ADAMNet=Val;
}

static void Out40 (byte Val)
{
 if (LPTStream)
 {
  fputc (Val,LPTStream);
  if (Val==10) fflush (LPTStream);
 }
}

static void Out42 (byte Val)
{
 int a,i;
 a=Val&RAMMask;
 if (a>=RAMPages) a=-1;
 if (a!=RAMPage)
 {
  RAMPage=a;
  if (Verbose&4) printf ("Expansion RAM page %d selected\n",RAMPage);
  if ((CurrMemSel&3)==2)
  {
   if (RAMPage>=0)
   {
    for (i=0;i<32768;i+=256)
    {
     AddrTabl[i>>8]=&RAM[i+65536+RAMPage*65536];
     WriteAddrTabl[i>>8]=&RAM[i+65536+RAMPage*65536];
    }
   }
   else
   {
    for (i=0;i<32768;i+=256)
    {
     AddrTabl[i>>8]=DummyReadTabl;
     WriteAddrTabl[i>>8]=DummyWriteTabl;
    }
   }
  }
  if ((CurrMemSel&12)==8)
  {
   if (RAMPage>=0)
   {
    for (i=32768;i<65536;i+=256)
    {
     AddrTabl[i>>8]=&RAM[i+65536+RAMPage*65536];
     WriteAddrTabl[i>>8]=&RAM[i+65536+RAMPage*65536];
    }
   }
   else
   {
    for (i=32768;i<65536;i+=256)
    {
     AddrTabl[i>>8]=DummyReadTabl;
     WriteAddrTabl[i>>8]=DummyWriteTabl;
    }
   }
  }
 }
}

static void Out44 (byte Val)
{
#ifdef PRINT_IO
    printf("Write - Orphanware Serial Port 1 - %02X\n",Val);
#endif
}

static void Out4C (byte Val)
{
#ifdef PRINT_IO
    printf("Write - Orphanware Serial Port 2 - %02X\n",Val);
#endif
}

static void Out50 (byte Val)
{
#ifdef SOUND_PSG
    PSGControlReg = Val & 0x0F;
#endif
}

static void Out51 (byte Val)
{
#ifdef SOUND_PSG
    PSGReg[PSGControlReg] = Val;
    PSG_Sound(PSGControlReg,Val);
#endif
}

static void Out53 (byte Val)
{
    int i;
    sgmmodereg = Val;
    // Remap the memory...
    if (sgmmodereg & 0x01)
    {
        for (i=0x2000;i<0x8000;i+=256)
        {
            if (i>=0x2000 && i<0x8000)
            {
                AddrTabl[i>>8]=&RAM[i];
                WriteAddrTabl[i>>8]=&RAM[i];
            }
        }
    }
    else
    {
        for (i=0x2000;i<0x8000;i+=256)
        {
            if (i>=0x2000 && i<0x8000)
            {
                AddrTabl[i>>8]=&RAM[i&1023];
                WriteAddrTabl[i>>8]=&RAM[i&1023];
            }
        }
    
    }
}


static void Out54 (byte Val)
{
#ifdef PRINT_IO
    printf("Write - Orphanware Serial Port 3 - %02X\n",Val);
#endif
}

static void Out58 (byte Val) // IDE - Data low byte
{
#ifdef IDEHD
    if (IDE_Controller) {
        HardDiskWriteData |= Val;
        controllerWrite(IDE_Controller, HardDiskWriteData);
    }
#else
    Z80_WRMEM((Z80_GetPC()-1)&65535,Val);
#endif
}

static void Out59 (byte Val) // IDE - Data high byte
{
#ifdef IDEHD
    if (IDE_Controller) {
        HardDiskWriteData = Val << 8;
    }
#else
    Z80_WRMEM((Z80_GetPC()-1)&65535,Val);
#endif
}

static void Out5A (byte Val)
{
#ifdef IDEHD
    if (IDE_Controller) {
//        printf("FixedCtlReg %d\n",Val);
//        FixedCtlReg = Val;
    }
#else
    Z80_WRMEM((Z80_GetPC()-1)&65535,Val);
#endif
}

static void Out5C (byte Val)
{
#ifdef PRINT_IO
    printf("Write - Orphanware Serial Port 4 - %02X\n",Val);
#endif
}

static void Out5E (byte Val)
{
 ModemOut (0,Val);
}

static void Out5F (byte Val)
{
 ModemOut (1,Val);
}

static void Out60 (byte Val)
{
 int i;
 if (Verbose&4)
  printf ("0x%02X written to bank selector at PC=%04X\n",Val,Z80_GetPC());
 if ((Val&3)!=(CurrMemSel&3))
 {
  switch (Val&3)
  {
   case 0:
    if (ADAMNet&2)
    {
     if (Verbose&4) puts (" Lower 32K:EOS");
     for (i=0;i<16384;i+=256)
     {
      AddrTabl[i>>8]=DummyReadTabl;
      WriteAddrTabl[i>>8]=DummyWriteTabl;
     }
     for (i=16384;i<24576;i+=256)
     {
      AddrTabl[i>>8]=AddrTabl[(i+8192)>>8]=&EOS[i-16384];
      WriteAddrTabl[i>>8]=WriteAddrTabl[(i+8192)>>8]=DummyWriteTabl;
     }
    }
    else
    {
     if (Verbose&4) puts (" Lower 32K:SmartWriter");
     for (i=0;i<32768;i+=256)
     {
      AddrTabl[i>>8]=&WP[i];
      WriteAddrTabl[i>>8]=DummyWriteTabl;
     }
    }
    break;
   case 1:
    if (Verbose&4) puts (" Lower 32K:RAM");
    for (i=0;i<32768;i+=256)
    {
     AddrTabl[i>>8]=&RAM[i];
     WriteAddrTabl[i>>8]=&RAM[i];
    }
    break;
   case 2:
    if (Verbose&4) puts (" Lower 32K:Expansion RAM");
    if (RAMPage>=0)
    {
     for (i=0;i<32768;i+=256)
     {
      AddrTabl[i>>8]=&RAM[i+65536+RAMPage*65536];
      WriteAddrTabl[i>>8]=&RAM[i+65536+RAMPage*65536];
     }
    }
    else
    {
     for (i=0;i<32768;i+=256)
     {
      AddrTabl[i>>8]=DummyReadTabl;
      WriteAddrTabl[i>>8]=DummyWriteTabl;
     }
    }
    break;
   case 3:
    if (Verbose&4) puts (" Lower 32K:OS-7 plus 24K RAM");
    for (i=0;i<8192;i+=256)
    {
     AddrTabl[i>>8]=&OS7[i];
     WriteAddrTabl[i>>8]=DummyWriteTabl;
    }
    for (i=8192;i<32768;i+=256)
    {
     AddrTabl[i>>8]=&RAM[i];
     WriteAddrTabl[i>>8]=&RAM[i];
    }
    break;
  }
 }
 if ((Val&12)!=(CurrMemSel&12))
 {
  switch (Val&12)
  {
   case 0:
    if (Verbose&4) puts (" Upper 32K:RAM");
    for (i=32768;i<65536;i+=256)
    {
     AddrTabl[i>>8]=&RAM[i];
     WriteAddrTabl[i>>8]=&RAM[i];
    }
    break;
   case 4:
    if (Verbose&4) puts (" Upper 32K:Expansion ROM");
    for (i=32768;i<65536;i+=256)
    {
     AddrTabl[i>>8]=&EXPROM[i-32768];
     WriteAddrTabl[i>>8]=&EXPROM[i-32768];
    }
    break;
   case 8:
    if (Verbose&4) puts (" Upper 32K:Expansion RAM");
    if (RAMPage>=0)
    {
     for (i=32768;i<65536;i+=256)
     {
      AddrTabl[i>>8]=&RAM[i+65536+RAMPage*65536];
      WriteAddrTabl[i>>8]=&RAM[i+65536+RAMPage*65536];
     }
    }
    else
    {
     for (i=32768;i<65536;i+=256)
     {
      AddrTabl[i>>8]=DummyReadTabl;
      WriteAddrTabl[i>>8]=DummyWriteTabl;
     }
    }
    break;
   case 12:
    if (Verbose&4) puts (" Upper 32K:Cartridge ROM");
    for (i=32768;i<65536;i+=256)
    {
     AddrTabl[i>>8]=&CART[i-32768 + (CurrCartBank*16*1024)];
     WriteAddrTabl[i>>8]=DummyWriteTabl;
    }
    break;
  }
 }
 CurrMemSel=Val;
}

static void Out7F (byte Val)
{
    int i;

    if (EmuMode)
    {
        Out60(Val);
    } 
    else if (sgmmode)
    {
        if (!(Val & 0x02)) // For SGM, when this bit is 0, then we map in the SGM RAM to the lower 8K, replacing OS7.
        {
            for (i=0;i<0x2000;i+=256)
            {
                AddrTabl[i>>8]=&RAM[i];
                WriteAddrTabl[i>>8]=&RAM[i];
            }
        }
        else
        {
            for (i = 0; i < 0x2000; i += 256)
            {
                AddrTabl[i >> 8] = &OS7[i];
                WriteAddrTabl[i >> 8] = DummyWriteTabl;
            }
        }
    }
}

static void Out80 (byte Val)
{
 JoyMode=0;
}

static void OutA0 (byte Val)
{
 if (!VDP.Mode) ++VDP.Addr;
 if (VDP.VRAM[VDP.Addr&VDP.VRAMSize]!=Val)
 {
  VDP.VRAM[VDP.Addr&VDP.VRAMSize]=Val;
  VDP.ScreenChanged=1;
 }
 VDP.Addr++;
 VDP.VKey=1;
}

static void OutA1 (byte Val)
{
 if (VDP.VKey)
 {
  VDP.VR=Val;
  VDP.VKey=0;
 }
 else
 {
  VDP.VKey=1;
  switch(Val&0xC0)
  {
   case 0x80:
    Val&=7;
    if (Verbose&2)
     printf ("VDPOut(%02X,%02X) - PC=%04X\n",Val,VDP.VR,Z80_GetPC());
    VDP.ScreenChanged=1;
    switch (Val)
    {
     case  1: if ((VDP.VR&0x20)==0) NMI_Line=0;
              VDP.VRAMSize=(VDP.VR&0x80)? 0x3FFF:0x0FFF;
              break;
     case  7: VDP.FGColour=VDP.VR>>4;
              VDP.BGColour=VDP.VR&0x0F;
              ColourScreen ();
              break;
    }
    VDP.Reg[Val]=VDP.VR;
    break;
   case 0x40:
    VDP.Addr=VDP.VR+(Val&0x3F)*256;
    VDP.Mode=1;
    break;
   case 0x00:
    VDP.Addr=VDP.VR+(Val&0x3F)*256;
    VDP.Mode=0;
    break;
  }
 }
}

static void OutC0 (byte Val)
{
 JoyMode=1;
}

static void OutE0 (byte Val)
{
 byte Port;
 if(Val&0x80)
 {
  Port=Val&0x70;
  if((Port==0x00)||(Port==0x20)||(Port==0x40))
  {
   sound_data=Val;
   DoSound(Port>>4,(Val&0x0F)|(sound_reg[Port>>4]&0x3F0));
  }
  else
   DoSound(Port>>4,Val&0x0F);
 }
 else
  DoSound((sound_data>>4)&0x07,(sound_data&0x0F)|((word)(Val&0x3F)<<4));
}

typedef void (*OutPortFn) (byte Val);
OutPortFn OutPort[256]=
{
    OutNo,OutNo,Out02,Out03,Out04,Out05,Out06,Out07,
    OutNo,OutNo,OutNo,OutNo,Out0C,Out0D,Out0E,Out0F, // 15
    Out10,Out10,Out10,Out10,OutNo,OutNo,OutNo,OutNo, // 23
    Out18,Out18,Out18,Out18,OutNo,OutNo,OutNo,OutNo, // 31
    OutNo,OutNo,OutNo,OutNo,OutNo,OutNo,OutNo,OutNo, // 39
    OutNo,OutNo,OutNo,OutNo,OutNo,OutNo,OutNo,OutNo, // 47
    OutNo,OutNo,OutNo,OutNo,OutNo,OutNo,OutNo,OutNo, // 55
    OutNo,OutNo,OutNo,OutNo,OutNo,OutNo,OutNo,OutNo, // 63
    OutNo,OutNo,OutNo,OutNo,Out44,Out44,Out44,Out44, // 71
    OutNo,OutNo,OutNo,OutNo,Out4C,Out4C,Out4C,Out4C, // 79
    OutNo,OutNo,OutNo,OutNo,Out54,Out54,Out54,Out54, // 87
    Out58,Out59,Out5A,OutNo,Out5C,Out5C,Out5C,Out5C, // 95
    OutNo,OutNo,OutNo,OutNo,OutNo,OutNo,OutNo,OutNo, // 103
    OutNo,OutNo,OutNo,OutNo,OutNo,OutNo,OutNo,OutNo, // 111
    OutNo,OutNo,OutNo,OutNo,OutNo,OutNo,OutNo,OutNo, // 119
    OutNo,OutNo,OutNo,OutNo,OutNo,OutNo,OutNo,OutNo, // 127
    Out80,Out80,Out80,Out80,Out80,Out80,Out80,Out80, // 135
    Out80,Out80,Out80,Out80,Out80,Out80,Out80,Out80, // 143
    Out80,Out80,Out80,Out80,Out80,Out80,Out80,Out80, // 151
    Out80,Out80,Out80,Out80,Out80,Out80,Out80,Out80,
    OutA0,OutA1,OutA0,OutA1,OutA0,OutA1,OutA0,OutA1,
    OutA0,OutA1,OutA0,OutA1,OutA0,OutA1,OutA0,OutA1,
    OutA0,OutA1,OutA0,OutA1,OutA0,OutA1,OutA0,OutA1,
    OutA0,OutA1,OutA0,OutA1,OutA0,OutA1,OutA0,OutA1,
    OutC0,OutC0,OutC0,OutC0,OutC0,OutC0,OutC0,OutC0,
    OutC0,OutC0,OutC0,OutC0,OutC0,OutC0,OutC0,OutC0,
    OutC0,OutC0,OutC0,OutC0,OutC0,OutC0,OutC0,OutC0,
    OutC0,OutC0,OutC0,OutC0,OutC0,OutC0,OutC0,OutC0,
    OutE0,OutE0,OutE0,OutE0,OutE0,OutE0,OutE0,OutE0,
    OutE0,OutE0,OutE0,OutE0,OutE0,OutE0,OutE0,OutE0,
    OutE0,OutE0,OutE0,OutE0,OutE0,OutE0,OutE0,OutE0,
    OutE0,OutE0,OutE0,OutE0,OutE0,OutE0,OutE0,OutE0
};

#if 1
void FASTCALL Z80_Out (unsigned Port,byte Val)
{
 OutPortFn fn;
 Port&=0xff;
 fn=OutPort[Port];
#ifdef PRINT_IO
 if (fn==OutNo)
 {
  printf ("Attempt to write %02X to port %02X at %04X\n",Val,Port,Z80_GetPC());
  Z80_RegisterDump ();
 }
#endif
 (*fn)(Val);
}
#endif

/****************************************************************************/
/*** Functions that read I/O ports. They are only mapped in if necessary  ***/
/****************************************************************************/
static byte InpNo (void)
{
 return Z80_RDMEM((Z80_GetPC()-1)&65535);
}

static byte Inp01 (void) // IDE - Feature Register
{
#ifdef IDEHD
    if (IDE_Controller) {
        return controllerReadRegister(IDE_Controller, 2);
    } else {
        return Z80_RDMEM((Z80_GetPC()-1)&65535);
    }
#else
    return Z80_RDMEM((Z80_GetPC()-1)&65535);
#endif
}

static byte Inp02 (void) // IDE - Number of sectors
{
#ifdef IDEHD
    if (IDE_Controller) {
        return controllerReadRegister(IDE_Controller, 2);
    } else {
        return Z80_RDMEM((Z80_GetPC()-1)&65535);
    }
#else
    return Z80_RDMEM((Z80_GetPC()-1)&65535);
#endif
}

static byte Inp03 (void) // IDE - Sector number
{
#ifdef IDEHD
    if (IDE_Controller) {
        return controllerReadRegister(IDE_Controller, 3);
    } else {
        return Z80_RDMEM((Z80_GetPC()-1)&65535);
    }
#else
    return Z80_RDMEM((Z80_GetPC()-1)&65535);
#endif
}

static byte Inp04 (void) // IDE - Cylinder low byte
{
#ifdef IDEHD
    if (IDE_Controller) {
        return controllerReadRegister(IDE_Controller, 4);
    } else {
        return Z80_RDMEM((Z80_GetPC()-1)&65535);
    }
#else
    return Z80_RDMEM((Z80_GetPC()-1)&65535);
#endif
}

static byte Inp05 (void) // IDE - Cylinder high byte
{
#ifdef IDEHD
    if (IDE_Controller) {
        return controllerReadRegister(IDE_Controller, 5);
    } else {
    return Z80_RDMEM((Z80_GetPC()-1)&65535);
    }
#else
    return Z80_RDMEM((Z80_GetPC()-1)&65535);
#endif
}

static byte Inp06 (void) // IDE - Drive / Head
{
#ifdef IDEHD
    if (IDE_Controller) {
        return controllerReadRegister(IDE_Controller, 6);
    } else {
        return Z80_RDMEM((Z80_GetPC()-1)&65535);
    }
#else
    return Z80_RDMEM((Z80_GetPC()-1)&65535);
#endif
}

static byte Inp07 (void) // IDE - Status
{
#ifdef IDEHD
    if (IDE_Controller) {
        return controllerReadRegister(IDE_Controller, 7);
    } else {
        return Z80_RDMEM((Z80_GetPC()-1)&65535);
    }
#else
    return Z80_RDMEM((Z80_GetPC()-1)&65535);
#endif
}

static byte Inp08 (void)
{
#ifdef PRINT_IO
    printf("in 08\n");
#endif
    return 0x80;
}

static byte Inp09 (void)
{
#ifdef PRINT_IO
    printf("in 09\n");
#endif
    return 0x80;
}

static byte Inp0A (void)
{
    byte temp;
    
#ifdef PRINT_IO
    printf("in 0a\n");
#endif
    temp = usartModeReg[usartRMRIndex++];
    if (usartRMRIndex > 1)
        usartRMRIndex = 0;
    return temp;
}

static byte Inp0B (void)
{
#ifdef PRINT_IO
    printf("in 0b\n");
#endif
    return usartCmdReg;
}

static byte Inp10 (void)
{
#ifdef PRINT_IO
    printf("Read - MIB2 Serial Port 2\n");
#endif
    return Z80_RDMEM((Z80_GetPC()-1)&65535);
}

static byte Inp18 (void)
{
#ifdef PRINT_IO
    printf("Read - MIB2 Serial Port 1\n");
#endif
    return Z80_RDMEM((Z80_GetPC()-1)&65535);
}

static byte Inp20 (void)
{
 return ADAMNet;
}

static byte Inp40 (void)
{
 return 0x41;
}

static byte Inp44 (void)
{
#ifdef PRINT_IO
    printf("Read - Orphanware Serial Port 1\n");
#endif
    return Z80_RDMEM((Z80_GetPC()-1)&65535);
}

static byte Inp4C (void)
{
#ifdef PRINT_IO
    printf("Read - Orphanware Serial Port 2\n");
#endif
    return Z80_RDMEM((Z80_GetPC()-1)&65535);
}

static byte Inp52 (void)
{
#ifdef SOUND_PSG
    
    return (byte)PSGReg[PSGControlReg];
#else
    return 0;
#endif
}

static byte Inp53 (void)
{
    return sgmmodereg;
}

static byte Inp54 (void)
{
#ifdef PRINT_IO
    printf("Read - Orphanware Serial Port 3\n");
#endif
    return Z80_RDMEM((Z80_GetPC()-1)&65535);
}

static byte Inp58 (void) // IDE - Data low byte
{
#ifdef IDEHD
    if (IDE_Controller) {
        return (byte)controllerPeek(IDE_Controller);
    } else {
        return Z80_RDMEM((Z80_GetPC()-1)&65535);
    }
#else
    return Z80_RDMEM((Z80_GetPC()-1)&65535);
#endif
}

static byte Inp59 (void) // IDE - Data high byte
{
#ifdef IDEHD
    if (IDE_Controller) {
        return controllerRead(IDE_Controller) >> 8;
    } else {
        return Z80_RDMEM((Z80_GetPC()-1)&65535);
    }
#else
    return Z80_RDMEM((Z80_GetPC()-1)&65535);
#endif
}

static byte Inp5A (void)
{
#ifdef IDEHD
    if (IDE_Controller) {
//        AltStatReg = 0x5A;
//        printf("AltStatReg %d\n",AltStatReg);
//        return AltStatReg;
        return controllerReadRegister(IDE_Controller, 7);
    } else {
        return Z80_RDMEM((Z80_GetPC()-1)&65535);
    }
#else
    return Z80_RDMEM((Z80_GetPC()-1)&65535);
#endif
}

static byte Inp5B (void)
{
#ifdef IDEHD
    printf("DigInReg\n");
    return DigInReg;
#else
    return Z80_RDMEM((Z80_GetPC()-1)&65535);
#endif
}

static byte Inp5C (void)
{
#ifdef PRINT_IO
    printf("Read - Orphanware Serial Port 4\n");
#endif
    return Z80_RDMEM((Z80_GetPC()-1)&65535);
}

static byte Inp5E (void)
{
 return ModemIn (0);
}

static byte Inp5F (void)
{
 return ModemIn (1);
}

static byte Inp60 (void)
{
 return CurrMemSel;
}

static byte InpA0 (void)
{
 byte retval;
 retval=VDP.VRAM[VDP.Addr&VDP.VRAMSize];
 VDP.Addr++;
 VDP.VKey=1;
 return retval;
}

static byte InpA1 (void)
{
 byte retval;
 retval=VDP.Status;
 if (VDP.Status & 0x80)
  VDP.Status&=0x3F;
 else
  VDP.Status=0x1F;
 VDP.VKey=1;
 NMI_Line=0;
 return retval;
}

static byte InpE0 (void)
{
 byte retval;
 if (JoyMode)
 {
  retval=JoyState[0]>>8;
  if ((retval&0x0A)==0)
   retval|=0x02;
  if ((retval&0x05)==0)
   retval|=0x01;
  return retval;
 }
 else
 {
  retval=JoyState[0]&0xFF;
  return (retval&0xF0)
          |KeyCodes[retval&0x0F];
 }
}

static byte InpE2 (void)
{
 byte retval;
 if (JoyMode)
 {
  retval=JoyState[1]>>8;
  if ((retval&0x0A)==0)
   retval|=0x02;
  if ((retval&0x05)==0)
   retval|=0x01;
  return retval;
 }
 else
 {
  retval=JoyState[1]&0xFF;
  return (retval&0xF0)
          |KeyCodes[retval&0x0F];
 }
}

typedef byte (*InPortFn) (void);
InPortFn InPort[256]=
{
    InpNo,Inp01,Inp02,Inp03,Inp04,Inp05,Inp06,Inp07,
    Inp08,Inp09,Inp0A,Inp0B,InpNo,InpNo,InpNo,InpNo, // 15
    Inp10,Inp10,InpNo,Inp10,InpNo,InpNo,InpNo,InpNo, // 23
    Inp18,Inp18,InpNo,Inp18,InpNo,InpNo,InpNo,InpNo, // 31
    InpNo,InpNo,InpNo,InpNo,InpNo,InpNo,InpNo,InpNo, // 39
    InpNo,InpNo,InpNo,InpNo,InpNo,InpNo,InpNo,InpNo, // 47
    InpNo,InpNo,InpNo,InpNo,InpNo,InpNo,InpNo,InpNo, // 55
    InpNo,InpNo,InpNo,InpNo,InpNo,InpNo,InpNo,InpNo, // 63
    InpNo,InpNo,InpNo,InpNo,Inp44,Inp44,Inp44,Inp44, // 71
    InpNo,InpNo,InpNo,InpNo,Inp4C,Inp4C,Inp4C,Inp4C, // 79
    InpNo,InpNo,InpNo,Inp53,Inp54,Inp54,Inp54,Inp54, // 87
    Inp58,Inp59,Inp5A,Inp5B,Inp5C,Inp5C,Inp5C,Inp5C, // 95
    InpNo,InpNo,InpNo,InpNo,InpNo,InpNo,InpNo,InpNo, // 103
    InpNo,InpNo,InpNo,InpNo,InpNo,InpNo,InpNo,InpNo, // 111
    InpNo,InpNo,InpNo,InpNo,InpNo,InpNo,InpNo,InpNo, // 119
    InpNo,InpNo,InpNo,InpNo,InpNo,InpNo,InpNo,InpNo, // 127
    InpNo,InpNo,InpNo,InpNo,InpNo,InpNo,InpNo,InpNo, // 135
    InpNo,InpNo,InpNo,InpNo,InpNo,InpNo,InpNo,InpNo, // 143
    InpNo,InpNo,InpNo,InpNo,InpNo,InpNo,InpNo,InpNo, // 151
    InpNo,InpNo,InpNo,InpNo,InpNo,InpNo,InpNo,InpNo,
    InpA0,InpA1,InpA0,InpA1,InpA0,InpA1,InpA0,InpA1,
    InpA0,InpA1,InpA0,InpA1,InpA0,InpA1,InpA0,InpA1,
    InpA0,InpA1,InpA0,InpA1,InpA0,InpA1,InpA0,InpA1,
    InpA0,InpA1,InpA0,InpA1,InpA0,InpA1,InpA0,InpA1,
    InpNo,InpNo,InpNo,InpNo,InpNo,InpNo,InpNo,InpNo,
    InpNo,InpNo,InpNo,InpNo,InpNo,InpNo,InpNo,InpNo,
    InpNo,InpNo,InpNo,InpNo,InpNo,InpNo,InpNo,InpNo,
    InpNo,InpNo,InpNo,InpNo,InpNo,InpNo,InpNo,InpNo,
    InpE0,InpE0,InpE2,InpE2,InpE0,InpE0,InpE2,InpE2,
    InpE0,InpE0,InpE2,InpE2,InpE0,InpE0,InpE2,InpE2,
    InpE0,InpE0,InpE2,InpE2,InpE0,InpE0,InpE2,InpE2,
    InpE0,InpE0,InpE2,InpE2,InpE0,InpE0,InpE2,InpE2
};

#if 1
byte FASTCALL Z80_In (unsigned Port)
{
 InPortFn fn;
 Port&=0xff;
 fn=InPort[Port];
#ifdef PRINT_IO
 if (fn==InpNo)
 {
  printf ("Attempt to read port %02X at %04X\n",Port,Z80_GetPC());
  Z80_RegisterDump ();
 }
#endif
 return (*fn)();
}
#endif

/****************************************************************************/
/*** Reset the Coleco                                                     ***/
/****************************************************************************/
void ResetColeco (int mode)
{
 static byte VDPInit[8] = { 0x00,0x80,0x06,0x80,0x00,0x36,0x07,0x00 };
 int i;

 Z80_Out (0x53,0);                   /* If SGM mode, disable addt'l RAM */
 if (mode==1)
 {
  Z80_Reset ();                       /* Reset the Z80 emulator             */
  Z80_Out (0x42,0);                   /* Select expansion RAM page 0        */
  Z80_Out (0x7F,(mode==1)? 15:0);     /* Select either WP or OS-7           */
  Z80_Out (0x3F,0);
  EmuMode = 0;
  return;
 }

 EmuMode = 1;
 for (i=0;i<8;++i) VDP.Reg[i]=VDPInit[i];
 VDP.VRAMSize=0x3FFF;
 VDP.VKey=1;
 VDP.Status=0;
 VDP.FGColour=VDP.BGColour=0;
 VDP.Addr=0;
 if (mode!=2) ColourScreen ();
 VDP.ScreenChanged=1;
 memset (VDP.VRAM,0,0x4000);          /* Clear RAM and VRAM                 */
 for (i=0;i<RAMSize;i+=2)
 {
  RAM[i]=0;
  RAM[i+1]=0xFF;
 }
 Z80_Reset ();                        /* Reset the Z80 emulator             */
 JoyMode=0;                           /* Select joystick mode               */
 NMI_Line=0;                          /* Clear NMI line                     */
 if (EmuMode)
 {
  Z80_Out (0x42,0);                   /* Select expansion RAM page 0        */
  Z80_Out (0x7F,(mode==1)? 15:0);     /* Select either WP or OS-7           */
  Z80_Out (0x3F,0);
  UsePCB=0;                           /* Reset the PCB table                */
  memset (PCBTable,0,sizeof(PCBTable));
  PCB=65216;
  /* Put a RET at the interrupt locations. Solves some problems with badly
     behaving 3rd party ADAM software */
  RAM[0x38]=RAM[0x66]=0xC9;
  Z80_Out (0x53,0);                   /* If SGM mode, disable addt'l RAM */
 }
}

static int InitDone=0;
static int PC;

static void ResetPrinter (void)
{
 if (PrnStream)
 {
  switch (PrnType)
  {
   case 1:
    break;
   case 2:
    fputc (27,PrnStream); fputc ('@',PrnStream);
    break;
   case 3:
    fputc (27,PrnStream); fputc (26,PrnStream); fputc ('I', PrnStream);
    break;
   default:
    break;
  }
  fflush (PrnStream);
 }
}

static FILE *CheckGZ (char *filename,char *tempname)
{
#ifdef ZLIB
 FILE *f,*f2;
 int a,b;
 char buf[4096];
 f=fopen (filename,"a+b");
 if (!f) return NULL;
 rewind (f);
 a=fgetc(f); b=fgetc(f);
 if (a!=0x1F || b!=0x8B)
 {
  rewind (f);
  return f;
 }
 fclose (f);
 tmpnam (tempname);
 f=gzopen (filename,"rb");
 if (!f) return NULL;
 f2=fopen (tempname,"a+b");
 if (!f2)
 {
  gzclose (f);
  return NULL;
 }
 while ((a=gzread(f,buf,sizeof(buf)))!=0)
  fwrite (buf,1,a,f2);
 gzclose (f);
 return f2;
#else
 FILE *f;
 f=fopen (filename,"r+b");
 return f;
#endif
}

static void GZ_Copy (FILE *f,char *filename)
{
#ifdef ZLIB
 FILE *f2;
 char buf[4096];
 int a;
 if (Verbose)
 {
  printf ("Writing image %s... ",filename);
  fflush (stdout);
 }
 f2=gzopen (filename,"wb");
 if (!f2) { puts ("FAILED"); return; }
 rewind (f);
 while ((a=fread(buf,1,sizeof(buf),f))!=0)
  gzwrite (f2,buf,a);
 gzclose (f2);
 puts ("OK");
#endif
}

static int GetMask (int size)
{
 int i;

 for (i=1;i<size;i<<=1);
 i-=1;

// i=0xFFFF;
 return i;
}

void DiskOpen (int i)
{
 if (DiskName[i])
 {
  if (Verbose)
  {
   printf ("Opening disk image %s... ",DiskName[i]);
   fflush (stdout);
  }
  tempfiles[i][0]='\0';
  DiskStream[i]=CheckGZ (DiskName[i],tempfiles[i]);
  if (Verbose) puts ((DiskStream[i])? "OK":"FAILED");
  if (!DiskStream[i])
   DiskName[i]=NULL;
  else
  {
   fseek (DiskStream[i],0,SEEK_END);
   DiskSize[i]=ftell (DiskStream[i]);
   fseek (DiskStream[i],0,SEEK_SET);
  }
  if (!DiskSize[i]) DiskSize[i]=DEFAULT_DISK_SIZE;
  if (DiskSize[i]<MIN_DISK_SIZE) DiskSize[i]=DEFAULT_DISK_SIZE;
  DiskSize[i]=(DiskSize[i]/1024)+((DiskSize[i]&1023)? 1:0);
  DiskMask[i]=GetMask (DiskSize[i]);
  DiskChanged[i]=0;
 }
}

void TapeOpen (int i)
{
 if (TapeName[i])
 {
  if (Verbose)
  {
   printf ("Opening tape image %s... ",TapeName[i]);
   fflush (stdout);
  }
  tempfiles[i+4][0]='\0';
  TapeStream[i]=CheckGZ (TapeName[i],tempfiles[i+4]);
  if (Verbose) puts ((TapeStream[i])? "OK":"FAILED");
  if (!TapeStream[i])
   TapeName[i]=NULL;
  else
  {
   fseek (TapeStream[i],0,SEEK_END);
   TapeSize[i]=ftell (TapeStream[i]);
   fseek (TapeStream[i],0,SEEK_SET);
  }
  if (!TapeSize[i]) TapeSize[i]=DEFAULT_TAPE_SIZE;
  if (TapeSize[i]<MIN_TAPE_SIZE) TapeSize[i]=DEFAULT_TAPE_SIZE;
  TapeSize[i]=(TapeSize[i]/1024)+((TapeSize[i]&1023)? 1:0);
  TapeMask[i]=GetMask (TapeSize[i]);
  TapeChanged[i]=0;
 }
}

void TapeClose (int i)
{
 int a;
 if (!TapeStream[i]) return;
 if (tempfiles[i+4][0]!='\0')
 {
  if (TapeChanged[i]) GZ_Copy (TapeStream[i],TapeName[i]);
  fclose (TapeStream[i]);
  if (Verbose) printf ("Removing temporary file %s... ",tempfiles[i+4]);
  a=remove (tempfiles[i+4]);
  if (Verbose) puts ((a>=0)? "OK":"FAILED");
 }
 else fclose (TapeStream[i]);
 TapeName[i]=NULL;
}

void DiskClose (int i)
{
 int a;
 if (!DiskStream[i]) return;
 if (tempfiles[i][0]!='\0')
 {
  if (DiskChanged[i]) GZ_Copy (DiskStream[i],DiskName[i]);
  fclose (DiskStream[i]);
  if (Verbose) printf ("Removing temporary file %s... ",tempfiles[i]);
  a=remove (tempfiles[i]);
  if (Verbose) puts ((a>=0)? "OK":"FAILED");
 }
 else fclose (DiskStream[i]);
 DiskName[i]=NULL;
}

static void OpenPrinter (void)
{
 if (PrnName)
 {
  if (Verbose) printf ("Opening printer stream %s... ",PrnName);
  PrnStream=fopen (PrnName,(PrnType==1)? "wt":"wb");
  if (Verbose) puts ((PrnStream)? "OK":"FAILED");
  if (PrnStream)
  {
   ResetPrinter ();
   if (PrnType==2)
   {
    /* Set linefeed distance to 6/72" */
    fputc (27,PrnStream); fputc ('A',PrnStream); fputc (6,PrnStream);
    fputc (27,PrnStream); fputc ('2',PrnStream);
   }
  }
  else PrnName=NULL;
 }
 else
 {
  if (Verbose) puts ("Logging printer output to stdout");
  PrnStream=stdout;
  PrnName="stdout";
 }
}

static void OpenLPT (void)
{
 if (LPTName)
 {
  if (Verbose) printf ("Opening parallel port stream %s... ",LPTName);
  LPTStream=fopen (LPTName,"wb");
  if (Verbose) puts ((LPTStream)? "OK":"FAILED");
  if (!LPTStream) LPTName=NULL;
 }
 else
 {
  if (Verbose) puts ("Logging parallel port output to stdout");
  LPTStream=stdout;
  LPTName="stdout";
 }
}

static void OpenSoundLog (void)
{
 if (SoundName)
 {
  if (Verbose) printf ("Opening sound log file %s... ",SoundName);
  SoundStream=fopen(SoundName,"wb");
  if (Verbose) puts ((SoundStream)? "OK":"FAILED");
  if (SoundStream)
  {
   fwrite ("Coleco Sound File",1,17,SoundStream);
   fputc (0x1A,SoundStream);          /* EOF                                */
   fputc (0x01,SoundStream);          /* Version number (0x101)             */
   fputc (0x01,SoundStream);          /* ...                                */
   fputc (IFreq&0xFF,SoundStream);    /* Interrupt frequency                */
  }
  else
  {
   SoundStream=NULL;
   SoundName=NULL;
  }
 }
 numVDPInts=0;
}

static void ClosePrinter (void)
{
 if (PrnStream)
 {
  PrintChar (0);
  ResetPrinter ();
  if (PrnStream!=stdout)
   fclose (PrnStream);
  PrnStream=NULL;
  PrnName=NULL;
 }
}

static void CloseLPT (void)
{
 if (LPTStream)
 {
  if (LPTStream!=stdout)
   fclose (LPTStream);
  LPTStream=NULL;
  LPTName=NULL;
 }
}

static void CloseSoundLog (void)
{
 if (SoundStream)
 {
  fclose (SoundStream);
  SoundStream=NULL;
  SoundName=NULL;
 }
}

#ifdef ZLIB
#define fopen           gzopen
#define fclose          gzclose
#define fread(B,N,L,F)  gzread(F,B,(L)*(N))
#define fwrite(B,N,L,F) gzwrite(F,B,(L)*(N))
#endif

/****************************************************************************/
/*** Get parameters from snapshot file                                    ***/
/****************************************************************************/
static int GetSnapshotParams (FILE *f)
{
 static char _TapeName[4][256],_DiskName[4][256],_CartName[256];
 byte buf[16];
 int i;
 if (fread(buf,1,16,f)!=16) return -1;          /* Read error               */
 buf[15]='\0';
 if (strcmp((char*)buf,"ADAMEm snapshot")) return -2;  /* Not a snapshot file      */
 if (fread(buf,1,2,f)!=2) return -1;
 i=buf[0]*256+buf[1];
 if (i>0x100) return -3;                        /* Version not supported    */
 if (fread(buf,1,1,f)!=1) return -1;
 EmuMode=buf[0];
 if (fread(buf,1,1,f)!=1) return -1;
 RAMPages=buf[0];
 if (fread(_CartName,1,256,f)!=256) return -1;
 if (_CartName[0])
 {
  _CartName[255]='\0'; CartName=_CartName;
 }
 else
  CartName=NULL;
 for (i=0;i<4;++i)
 {
  if (fread(_DiskName[i],1,256,f)!=256) return -1;
  if (_DiskName[i][0])
  {
   _DiskName[i][255]='\0'; DiskName[i]=_DiskName[i];
  }
  else DiskName[i]=NULL;
 }
 for (i=0;i<4;++i)
 {
  if (fread(_TapeName[i],1,256,f)!=256) return -1;
  if (_TapeName[i][0])
  {
   _TapeName[i][255]='\0'; TapeName[i]=_TapeName[i];
  }
  else
   TapeName[i]=NULL;
 }
 return 1;
}

/****************************************************************************/
/*** Read data from snapshot file                                         ***/
/****************************************************************************/
static int GetSnapshotData (FILE *f)
{
 Z80_Regs R;
 byte buf[256];
 int i;
 memset (&R,0,sizeof(R));
 if (fread(buf,1,31,f)!=31) return -1;
 R.AF.B.h=buf[0];   R.AF.B.l=buf[1];
 R.BC.B.h=buf[2];   R.BC.B.l=buf[3];
 R.DE.B.h=buf[4];   R.DE.B.l=buf[5];
 R.HL.B.h=buf[6];   R.HL.B.l=buf[7];
 R.IX.B.h=buf[8];   R.IX.B.l=buf[9];
 R.IY.B.h=buf[10];  R.IY.B.l=buf[11];
 R.PC.B.h=buf[12];  R.PC.B.l=buf[13];
 R.SP.B.h=buf[14];  R.SP.B.l=buf[15];
 R.AF2.B.h=buf[16]; R.AF2.B.l=buf[17];
 R.BC2.B.h=buf[18]; R.BC2.B.l=buf[19];
 R.DE2.B.h=buf[20]; R.DE2.B.l=buf[21];
 R.HL2.B.h=buf[22]; R.HL2.B.l=buf[23];
 R.IFF1=buf[24];    R.IFF2=buf[25];
 R.HALT=buf[26];    R.IM=buf[27];
 R.I=buf[28];       R.R=buf[29];
 R.R2=buf[30];
 Z80_SetRegs (&R);
 if (fread(buf,1,4,f)!=4) return -1;
 if (buf[0]&0x80) Z80_ICount=(-1)&(~0xFFFFFFFF); /* Sign extend             */
 else Z80_ICount=0;
 Z80_ICount|=(buf[0]<<24)|(buf[1]<<16)|(buf[2]<<8)|(buf[3]<<0);
 if (fread(buf,1,36,f)!=36) return -1;
 CurrMemSel=buf[0];
 ADAMNet=buf[1];
 RAMPage=buf[2];
 NMI_Line=buf[3];
 for (i=0;i<4;++i)
 {
  disk_newstatus[i]=buf[4+i];
  tape_newstatus[i]=buf[8+i];
  disk_lastblock[i]=buf[12+i];
  tape_lastblock[i]=buf[16+i];
  disk_status[i]=buf[20+i];
  tape_status[i]=buf[24+i];
  disk_timeout[i]=buf[28+i];
  tape_timeout[i]=buf[32+i];
 }
 if (fread(buf,1,14,f)!=14) return -1;
 for (i=0;i<8;++i) VDP.Reg[i]=buf[i];
 VDP.Status=buf[8];
 VDP.Mode=buf[9];
 VDP.VKey=buf[10];
 VDP.VR=buf[11];
 VDP.Addr=(buf[12]<<8)+buf[13];
 if (fread(buf,1,17,f)!=17) return -1;
 for (i=0;i<8;++i)
 {
  sound_reg[i]=(buf[i*2+0]<<8)+buf[i*2+1];
 }
 sound_data=buf[16];
 if (fread(buf,1,1,f)!=1) return -1;
 JoyMode=buf[0];
 if (fread(buf,1,4,f)!=4) return -1;
 IFreqRecur=(buf[0]<<24)+(buf[1]<<16)+(buf[2]<<8)+(buf[3]<<0);
 if (fread(buf,1,3,f)!=3) return -1;
 UsePCB=buf[0];
 PCB=(buf[1]<<8)+buf[2];
 if (fread(VDP.VRAM,1,16384,f)!=16384) return -1;
 if (fread(RAM,1,RAMSize,f)!=RAMSize) return -1;
 return 1;
}

/****************************************************************************/
/*** Save current emulation status to snapshot file                       ***/
/****************************************************************************/
static int _SaveSnapshotFile (FILE *f)
{
 Z80_Regs R;
 byte buf[256];
 int i;
 if (fwrite("ADAMEm snapshot\032",1,16,f)!=16)
  return -2;                                    /* Write error              */
 buf[0]=0x01; buf[1]=0x00;
 if (fwrite(buf,1,2,f)!=2) return -2;
 buf[0]=EmuMode;
 if (fwrite(buf,1,1,f)!=1) return -2;
 buf[0]=RAMPages;
 if (fwrite(buf,1,1,f)!=1) return -2;
 memset (buf,0,256);
 if (CartName) strcpy ((char*)buf,CartName);
 if (fwrite(buf,1,256,f)!=256) return -2;
 for (i=0;i<4;++i)
 {
  memset (buf,0,256);
  if (DiskName[i]) strcpy ((char*)buf,DiskName[i]);
  if (fwrite(buf,1,256,f)!=256) return -2;
 }
 for (i=0;i<4;++i)
 {
  memset (buf,0,256);
  if (TapeName[i]) strcpy ((char*)buf,TapeName[i]);
  if (fwrite(buf,1,256,f)!=256) return -2;
 }
 Z80_GetRegs (&R);
 buf[0]=R.AF.B.h;   buf[1]=R.AF.B.l;
 buf[2]=R.BC.B.h;   buf[3]=R.BC.B.l;
 buf[4]=R.DE.B.h;   buf[5]=R.DE.B.l;
 buf[6]=R.HL.B.h;   buf[7]=R.HL.B.l;
 buf[8]=R.IX.B.h;   buf[9]=R.IX.B.l;
 buf[10]=R.IY.B.h;  buf[11]=R.IY.B.l;
 buf[12]=R.PC.B.h;  buf[13]=R.PC.B.l;
 buf[14]=R.SP.B.h;  buf[15]=R.SP.B.l;
 buf[16]=R.AF2.B.h; buf[17]=R.AF2.B.l;
 buf[18]=R.BC2.B.h; buf[19]=R.BC2.B.l;
 buf[20]=R.DE2.B.h; buf[21]=R.DE2.B.l;
 buf[22]=R.HL2.B.h; buf[23]=R.HL2.B.l;
 buf[24]=R.IFF1;    buf[25]=R.IFF2;
 buf[26]=R.HALT;    buf[27]=R.IM;
 buf[28]=R.I;       buf[29]=R.R;
 buf[30]=R.R2;
 if (fwrite(buf,1,31,f)!=31) return -2;

 buf[0]=Z80_ICount>>24;
 buf[1]=Z80_ICount>>16;
 buf[2]=Z80_ICount>>8;
 buf[3]=Z80_ICount>>0;
 if (fwrite(buf,1,4,f)!=4) return -2;

 buf[0]=CurrMemSel;
 buf[1]=ADAMNet;
 buf[2]=RAMPage;
 buf[3]=NMI_Line;
 for (i=0;i<4;++i)
 {
  buf[4+i]=disk_newstatus[i];
  buf[8+i]=tape_newstatus[i];
  buf[12+i]=disk_lastblock[i];
  buf[16+i]=tape_lastblock[i];
  buf[20+i]=disk_status[i];
  buf[24+i]=tape_status[i];
  buf[28+i]=disk_timeout[i];
  buf[32+i]=tape_timeout[i];
 }
 if (fwrite(buf,1,36,f)!=36) return -2;
 for (i=0;i<8;++i) buf[i]=VDP.Reg[i];
 buf[8]=VDP.Status;
 buf[9]=VDP.Mode;
 buf[10]=VDP.VKey;
 buf[11]=VDP.VR;
 buf[12]=VDP.Addr>>8;
 buf[13]=VDP.Addr>>0;
 if (fwrite(buf,1,14,f)!=14) return -2;
 for (i=0;i<8;++i)
 {
  buf[i*2+0]=sound_reg[i]>>8;
  buf[i*2+1]=sound_reg[i]>>0;
 }
 buf[16]=sound_data;
 if (fwrite(buf,1,17,f)!=17) return -2;
 buf[0]=JoyMode;
 if (fwrite(buf,1,1,f)!=1) return -2;
 buf[0]=IFreqRecur>>24;
 buf[1]=IFreqRecur>>16;
 buf[2]=IFreqRecur>>8;
 buf[3]=IFreqRecur>>0;
 if (fwrite(buf,1,4,f)!=4) return -2;
 buf[0]=UsePCB;
 buf[1]=PCB>>8;
 buf[2]=PCB>>0;
 if (fwrite(buf,1,3,f)!=3) return -2;
 if (fwrite(VDP.VRAM,1,16384,f)!=16384) return -2;
 if (fwrite(RAM,1,RAMSize,f)!=RAMSize) return -2;
 return 1;
}

int SaveSnapshotFile (char *filename)
{
 int retval;
 FILE *f;
 f=fopen (filename,"wb");
 if (!f) return -1;                             /* File open error          */
 retval=_SaveSnapshotFile (f);
 fclose (f);
 return retval;
}

/****************************************************************************/
/*** This function allocates all resources necessary for the              ***/
/*** hardware-independent part of the code and starts the emulation. In   ***/
/*** case of a failure, this function returns 0                           ***/
/****************************************************************************/
int StartColeco(void)
{
 FILE *f,*f_snap;
 int *T,i,j;
 char *P;

 T=(int *)"\01\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";
#ifdef LSB_FIRST
 if(*T!=1)
 {
  printf("********** This machine is high-endian. **********\n"
         "Take #define LSB_FIRST out and compile ADAMem again\n");
  return 0;
 }
#else
 if(*T==1)
 {
  printf("********* This machine is low-endian. **********\n"
         "Insert #define LSB_FIRST and compile ADAMem again\n");
  return 0;
 }
#endif

 if (DiskSpeed<1) DiskSpeed=1;
 if (DiskSpeed>10000) DiskSpeed=10000;
 if (TapeSpeed<1) TapeSpeed=1;
 if (TapeSpeed>10000) TapeSpeed=10000;
 f_snap=NULL;
 if (SnapshotName)
 {
  if (Verbose) printf ("Opening snapshot %s... ",SnapshotName);
  f_snap=fopen (SnapshotName,"rb");
  if (!f_snap)
  {
   if (Verbose) puts ("FAILED");
  }
  else
  {
   if (Verbose) printf ("OK - Reading parameters... ");
   i=GetSnapshotParams (f_snap);
   if (i<0)
   {
    fclose (f_snap);
    if (Verbose)
    {
     printf ("FAILED:");
     switch (i)
     {
      case -1: puts ("Read error");
               break;
      case -2: puts ("File type not supported");
               break;
      case -3: puts ("File version not supported");
               break;
      default: printf ("Unknown error code %d\n",i);
     }
    }
    return 0;
   }
   if (Verbose) puts ("OK");
  }
 }
 if (EmuMode)
 {
  ROMSize=8+8+32+32;                  /* OS-7, EOS, WP, EXPROM                */
  if (RAMPages>255) RAMPages=255;
  if (RAMPages<0) RAMPages=0;
  if (RAMPages)                       /* RAMPages should be a power of 2    */
  {
   for (j=1;j<RAMPages;j*=2);
   RAMPages=j;
  }
  if (!RAMPages) RAMPage=-1;          /* Select an invalid RAMPage if no    */
  else RAMPage=0;                     /* expander is present                */
  if (RAMPages==0 || RAMPages==1)     /* RAMMask should be set to 0 if a    */
   RAMMask=0;                         /* standard 64K expander is present   */
  else                                /* In case of larger ones, mapping    */
   RAMMask=0xFF;                      /* in higher pages will select non-   */
                                      /* existing memory                    */
  RAMSize=64+RAMPages*64;             /* 64K base mem + expander            */
 }
 else
 {
  ROMSize=8;                          /* OS-7                               */
  if (sgmmode )
    RAMSize=64;                       /* Super Game Module adds 32k, we'll burn a full 64k */
  else
    RAMSize=1;                        /* 1K mapped into an 8K slot          */
 }
 if (Verbose) printf ("Allocating %ukB ROM... ",ROMSize);
 ROMSize*=1024;
 ROM=malloc(ROMSize);
 if (Verbose) puts ((ROM)?"OK":"FAILED");
 if (!ROM) return 0;
 memset (ROM,0,ROMSize);
 OS7=ROM;
// CART=ROM+8*1024;
 if (!CART)
 {
     CART=malloc(1024*1024);
 }
    
 if (EmuMode)
 {
  EOS=ROM+8*1024;
  WP=ROM+(8+8)*1024;
  EXPROM=ROM+(8+8+32)*1024;
 }
 if (Verbose) printf ("Allocating %ukB RAM... ",RAMSize);
 RAMSize*=1024;
 RAM=malloc(RAMSize);
 if (Verbose) puts ((RAM)?"OK":"FAILED");
 if (!RAM) return 0;
 memset (DummyReadTabl,0,sizeof(DummyReadTabl));

 if (Verbose) printf ("Allocating 16kB VRAM... ");
 VDP.VRAM=malloc(16*1024);
 if (Verbose) puts ((VDP.VRAM)?"OK":"FAILED");
 if (!VDP.VRAM) return 0;

 if (Verbose) puts ("Loading ROMs:");

 if (Verbose) printf("  Opening %s... ",OS7File);
 j=0;
 f=fopen(OS7File,"rb");
 if(f)
 {
  if (fread(OS7,1,0x2000,f)==0x2000) j=1;
  fclose (f);
 }
 if(Verbose) puts((j)? "OK":"FAILED");
 if (!j) return 0;
 if (IFreq==60) OS7[0x0069]=60;       /* PAL/NTSC switch                    */
 else OS7[0x0069]=50;
 IFreqParam=(0x100000*IFreq)/1000;
 if (CartName)
 {
  if (Verbose) printf("  Opening %s... ",CartName);
  j=0; P=NULL;
  f=fopen(CartName,"rb");
  if (f)
  {
   fseek(f, 0L, SEEK_END);
   CartSize = ftell(f);
 
   if(CartSize > 1024*1024)
   {
       if(Verbose) printf("FAILED, Cartridge image is too big! (>1024kB)");
   } else {
     fseek(f, 0L, SEEK_SET);
     j=fread(CART,1,CartSize,f);

     if (CartSize > 0x8000) // Bank Switching Cartridge 
     {
         CartBanks = CartSize/(16*1024);
         CurrCartBank = CartBanks-1;
         if (!((CART[CartSize - 0x4000]==0x55)&&(CART[CartSize - 0x3FFF]==0xAA)) &&
             !((CART[CartSize - 0x4000]==0xAA)&&(CART[CartSize - 0x3FFF]==0x55)))
         { j=1;P="WARNING: NOT A VALID CARTRIDGE"; }
         
     } else {
         if (!((CART[0]==0x55)&&(CART[1]==0xAA)) &&
             !((CART[0]==0xAA)&&(CART[1]==0x55)))
         { j=1;P="WARNING: NOT A VALID CARTRIDGE"; }
     }

   }
   fclose(f);
  }
  else P="NOT FOUND";
  if (Verbose) {
   if (j>1) {
    printf("%d bytes loaded\n",j);
   } else {
    puts(P);
   }
  }
  if (j && CheatCount)
  {
   if (Verbose) printf ("  Patching cheats into cartridge ROM code...");
   for (i=0;i<CheatCount;++i)
   {
    j=(Cheats[i]>>8)&0x7FFF;
    if (Verbose) printf ("%X...",j);
    CART[j]=Cheats[i]&255;
   }
   if (Verbose) puts("");
  }
 }
 if (EmuMode)
 {
  if(Verbose) printf("  Opening %s... ",EOSFile);
  j=0;
  f=fopen(EOSFile,"rb");
  if (f)
  {
   if(fread(EOS,1,0x2000,f)==0x2000) j=1;
   fclose (f);
  }
  if (Verbose) puts(j? "OK":"FAILED");
  if (!j) return 0;
  if (Verbose) printf("  Opening %s... ",WPFile);
  j=0;
  f=fopen(WPFile,"rb");
  if (f)
  {
   if (fread(WP,1,0x8000,f)==0x8000) j=1;
   fclose(f);
  }
  if (Verbose) puts(j? "OK":"FAILED");
  if (!j) return(0);
  if (ExpRomFile)
  {
   if (Verbose) printf("  Opening %s... ",ExpRomFile);
   j=0; P=NULL;
   f=fopen(ExpRomFile,"rb");
   if (f)
   {
    j=fread(EXPROM,1,0x8000,f);
    fclose(f);
   }
   else P="NOT FOUND";
   if (Verbose) {
    if (j>1) {
     printf("%d bytes loaded\n",j);
    } else {
     puts(P);
    }
   }
  }

#ifdef IDEHD
    if (Verbose) puts("Loading IDE hard disk image(s):");
    IDE_Controller = controllerCreate(0);
    // initialize IDE Master
    if (IDE_Controller && HardDiskFile[0]) {
        if (controllerAssign(IDE_Controller, HardDiskFile[0], 0, Verbose)) {
            controllerDestroy(IDE_Controller);
            IDE_Controller = NULL;
        }
    }
    // initialize IDE Slave
    if (IDE_Controller && HardDiskFile[1])
        controllerAssign(IDE_Controller, HardDiskFile[1], 1, Verbose);
#endif
 }

 if (EmuMode)
 {
  for (i=0;i<4;++i)
  {
   DiskOpen (i);
   TapeOpen (i);
  }
  OpenPrinter ();
  OpenLPT ();
 }
 OpenSoundLog ();

 if (Verbose) puts ("Initialising memory mapper...");
 if (EmuMode)
 {
  for (i=0x20;i<0x40;++i)
  {
   OutPort[i]=Out20;
   InPort[i]=Inp20;
  }
  for (i=0x60;i<0x80;++i)
  {
   OutPort[i]=Out60;
   InPort[i]=Inp60;
  }
  if (RAMPages>1)                     /* Only map in RAM page selector if   */
  OutPort[0x42]=Out42;                /* expander is >64KB                  */
  OutPort[0x5E]=Out5E;
  OutPort[0x5F]=Out5F;
  InPort[0x5E]=Inp5E;
  InPort[0x5F]=Inp5F;
  OutPort[0x40]=Out40;
  InPort[0x40]=Inp40;
 }
 for (i=0;i<65536;i+=256)
 {
  if (EmuMode)
  {
   if (i<0x8000)
   {
    AddrTabl[i>>8]=&WP[i];
    WriteAddrTabl[i>>8]=DummyWriteTabl;
   }
   else
   {
    AddrTabl[i>>8]=&RAM[i];
    WriteAddrTabl[i>>8]=&RAM[i];
   }
  }
  else
  {
   if (i>=0x6000 && i<0x8000)
   {
//       if (sgmmode)
//       {
//           AddrTabl[i>>8]=&RAM[i];
//           WriteAddrTabl[i>>8]=&RAM[i];
//       }
//       else
//       {
           AddrTabl[i>>8]=&RAM[i&1023];
           WriteAddrTabl[i>>8]=&RAM[i&1023];
//       }
   }
   else if (i<0x2000)
   {
    AddrTabl[i>>8]=&OS7[i];
    WriteAddrTabl[i>>8]=DummyWriteTabl;
   }
   else if (i<0x6000)
   {
          AddrTabl[i>>8]=DummyReadTabl;
          WriteAddrTabl[i>>8]=DummyWriteTabl;
   }
   else
   {
    if (CartSize > 0x8000) { // Bank Switch Cartridge
        if (i<0xC000) {
            AddrTabl[i>>8]=&CART[CartSize-0x4000 + (i-0x8000)];
        }
        else {
            AddrTabl[i>>8]=&CART[CartSize-0x4000 + (i-0xc000)];
        }
    } else {
        AddrTabl[i>>8]=&CART[i-0x8000];
    }
    WriteAddrTabl[i>>8]=DummyWriteTabl;
   }
  }
 }

 if (sgmmode)
 {
     OutPort[0x50]=Out50;
     OutPort[0x51]=Out51;
     OutPort[0x53]=Out53;
     OutPort[0x7F]=Out7F;
     InPort[0x52]=Inp52;
     InPort[0x53]=Inp53;
 }
 if (Verbose) printf("Initialising VDP and CPU...\n");
 ResetColeco (2);

 if (f_snap)
 {
  if (Verbose) printf ("Reading snapshot data... ");
  i=GetSnapshotData (f_snap);
  if (i<0)
  {
   if (Verbose)
   {
    printf ("FAILED:");
    switch (i)
    {
     case -1: puts ("Read error");
              break;
     default: printf ("Unknown error code %d\n",i);
    }
   }
   return 0;
  }
  if (Verbose) puts ("OK");
 }
#ifdef MSDOS
 if (!InitMachine())
  return 0;
#endif
 if (f_snap)
 {
  VDP.VRAMSize=(VDP.Reg[1]&0x80)? 0x3FFF:0x0FFF;
  VDP.FGColour=VDP.Reg[7]>>4;
  VDP.BGColour=VDP.Reg[7]&0x0F;
  ColourScreen ();
  VDP.ScreenChanged=1;
  for (i=0;i<8;++i) DoSound (i,sound_reg[i]);
  i=ADAMNet; ADAMNet=0; Z80_Out (0x3F,i);
  i=RAMPage; RAMPage=0; Z80_Out (0x42,i);
  i=CurrMemSel; CurrMemSel=0; Z80_Out (0x7F,i);
  if (UsePCB) SetPCBTable(0);
 }
 InitDone=1;
 PC=Z80 ();
 return 1;
}
#ifdef ZLIB
#undef fopen
#undef fclose
#undef fread
#undef fwrite
#endif

/****************************************************************************/
/*** Free memory allocated with StartColeco()                             ***/
/****************************************************************************/
void TrashColeco(void)
{
 int i;
#ifdef MSDOS
 if (InitDone)
  TrashMachine ();
#else
 if (Verbose) puts ("\n\nShutting down...\n");
#endif
 if (SaveSnapshot && SnapshotName && InitDone)
 {
  if (Verbose)
  {
   printf ("Writing snapshot %s...",SnapshotName);
   fflush (stdout);
  }
  i=SaveSnapshotFile(SnapshotName);
  if (Verbose)
  {
   if (i<0)
   {
    printf ("FAILED: ");
    switch (i)
    {
     case -1: puts ("Cannot open file");
              break;
     case -2: puts ("Write error (disk full ?)");
              break;
     default: printf ("Unknown error code %d\n",i);
    }
   }
   else puts ("OK");
  }
 }
 if (RAM) free (RAM);
 if (ROM) free (ROM);
 if (CART) free (CART);
 if (VDP.VRAM) free (VDP.VRAM);
 for (i=0;i<4;++i)
 {
  DiskClose (i);
  TapeClose (i);
 }
 ClosePrinter ();
 CloseLPT ();
 CloseSoundLog ();
#ifdef IDEHD
 if (IDE_Controller) controllerDestroy(IDE_Controller);
#endif
 if(Verbose && InitDone) printf("EXITED at PC = %Xh.\n",PC);
}

/****************************************************************************/
/*** Keyboard emulation routines                                          ***/
/****************************************************************************/
static byte keyboard_buffer[16];
static byte keyboard_buffer_int_count[16];
static int keyboard_buffer_count=0;
#define MAX_KEYBOARD_INT_COUNT        16
void AddToKeyboardBuffer (byte ch)
{
 if (ch==0) ch=255;                   /* If NUL is passed, change to 255    */
 keyboard_buffer[keyboard_buffer_count]=ch;
 keyboard_buffer_int_count[keyboard_buffer_count]=0;
 keyboard_buffer_count=(keyboard_buffer_count+1)&15;
 keyboard_buffer[keyboard_buffer_count]=0;
}
static int GetKeyboardChar (void)
{
 int retval;
 keyboard_buffer_count=(keyboard_buffer_count-1)&15;
 retval=keyboard_buffer[keyboard_buffer_count];
 keyboard_buffer[keyboard_buffer_count]=0;
 return retval;
}
static void UpdateKeyboard (int mode,unsigned DCB)
{
 int addr,count,i,a;
 static byte newkbdstatus=0x80;
 if (!mode)
 {
  RAM[DCB]=newkbdstatus;
  return;
 }
 switch (RAM[DCB])
 {
  case 1:
   RAM[DCB]=0x80;
   newkbdstatus=0x80;
   break;
  case 2:
   RAM[DCB]=0x80;
   newkbdstatus=0x80;
   break;
  case 3:
   RAM[DCB]=0x9B;
   newkbdstatus=0x80;
   break;
  case 4:
   addr=RAM[(DCB+1)&0xFFFF]+RAM[(DCB+2)&0xFFFF]*256;
   count=RAM[(DCB+3)&0xFFFF]+RAM[(DCB+4)&0xFFFF]*256;
   newkbdstatus=0x80;
   for (i=0;i<count;++i)
   {
    a=GetKeyboardChar ();
    if (a)
    {
     RAM[(addr++)&0xFFFF]=(a==255)? 0:a;
    }
    else
    {
     newkbdstatus=140;
     break;
    }
   }
   RAM[DCB]=newkbdstatus;
   break;
 }
}

/****************************************************************************/
/*** ADAM printer emulation routines                                      ***/
/****************************************************************************/
static void TextPrintChar (char c)
{
 static char linebuffer[1024];
 static int linecount=0;
 static int backwards=0;
 static int flushbuf=0;
 static int numchars=0;
 int i;
 switch (c)
 {
  case 8:
   if (backwards) ++linecount;
   else --linecount;
   break;
  case 11:
   if (flushbuf)
   {
    flushbuf=0;
    break;
   }
  case 10:
   flushbuf=1;
   break;
  case 13:
   linecount=0;
   break;
  case 14:
   backwards=1;
   break;
  case 15:
   backwards=0;
   break;
  default:
   if ((c>=32 && (c&128)==0) || c==9)
   {
    if (c>32)
    {
     flushbuf=0;
     if (linecount+1>numchars) numchars=linecount+1;
    }
    if (linebuffer[linecount]==32) linebuffer[linecount]=c;
    if (backwards) linecount--; else linecount++;
   }
   break;
 }
 if (linecount<0) linecount=0;
 if (linecount>=1024) linecount=1023;
 if (c==0 || (flushbuf&1))
 {
  for (i=0;i<numchars;++i) fputc (linebuffer[i],PrnStream);
  if (flushbuf&1)
  {
   fputc ('\n',PrnStream);
   flushbuf=2;
   numchars=0;
  }
  fflush (PrnStream);
  memset (linebuffer,32,sizeof(linebuffer));
  for (i=0;i<linecount;++i) linebuffer[i]=32;
 }
}

static void IBMPrintChar (char c)
{
 static int backwards=0;
 switch (c)
 {
  case 8:
   if (backwards) fputc (32,PrnStream);
   else fputc (8,PrnStream);
   break;
  case 10:
   fputc (10,PrnStream);
  case 11:
   fputc (10,PrnStream);
   break;
  case 13:
   fputc (13,PrnStream);
   break;
  case 14:
   backwards=1;
   break;
  case 15:
   backwards=0;
   break;
  default:
   if (c && (c&128)==0)
   {
    fputc (c,PrnStream);
    if (backwards)
    {
     fputc (8,PrnStream);
     fputc (8,PrnStream);
    }
   }
   break;
 }
 fflush (PrnStream);
}

static void QumePrintChar (char c)
{
 switch (c)
 {
  case 10:
   fputc (27,PrnStream); fputc ('U',PrnStream);
  case 11:
   fputc (27,PrnStream); fputc ('U',PrnStream);
   break;
  case 14:
   fputc (27,PrnStream); fputc ('6',PrnStream);
   break;
  case 15:
   fputc (27,PrnStream); fputc ('5',PrnStream);
   break;
  default:
   if (c && (c&128)==0) fputc (c,PrnStream);
   break;
 }
 fflush (PrnStream);
}

static void PrintChar (char c)
{
 if (PrnStream)
 {
  switch (PrnType)
  {
   case 1:
    TextPrintChar (c);
    break;
   case 2:
    IBMPrintChar (c);
    break;
   case 3:
    QumePrintChar (c);
    break;
   default:
    fputc (c,PrnStream);
    fflush (PrnStream);
  }
 }
}

static void UpdatePrinter (int mode,unsigned DCB)
{
 int addr,count,i,a;
 if (mode)
  switch (RAM[DCB])
  {
   case 1:
    RAM[DCB]=0x80;
    break;
   case 2:
    RAM[DCB]=0x80;
    break;
   case 3:
    if (PrnStream)
    {
     addr=RAM[(DCB+1)&0xFFFF]+RAM[(DCB+2)&0xFFFF]*256;
     count=RAM[(DCB+3)&0xFFFF]+RAM[(DCB+4)&0xFFFF]*256;
     for (i=0;i<count;++i)
     {
      a=RAM[(addr++)&0xFFFF];
      PrintChar (a);
     }
    }
    RAM[DCB]=0x80;
    break;
   case 4:
    RAM[DCB]=0x9B;
    break;
  }
 else
  RAM[DCB]=0x80;
}

/****************************************************************************/
/*** Diskette and tape emulation routines                                 ***/
/****************************************************************************/
static byte io_buffer[512];

static int diskread (unsigned p,int len,int block,FILE *f)
{
 int i;
 int retval;
 if (fseek(f,block*512,SEEK_SET))
  return 0;
 retval=fread(io_buffer,1,len,f);
 for (i=0;i<retval;++i) RAM[(p+i)&0xFFFF]=io_buffer[i];
 return (retval==len);
}
static int diskwrite (unsigned p,int len,int block,FILE *f)
{
 int i;
 if (fseek(f,block*512,SEEK_SET))
  return 0;
 for (i=0;i<len;++i) io_buffer[i]=RAM[(p+i)&0xFFFF];
 return (fwrite(io_buffer,1,len,f)==len);
}

static void UpdateDisk (int mode,int nr,unsigned DCB)
{
 static const byte interleavetable[8]= { 0,5,2,7,4,1,6,3 };
 static int addr=0,len=0;
 int block,i,j,blocksize;
 blocksize=1024;
 if (mode&127)
 {
  /* missing media */
  if (!DiskStream[nr]) RAM[(DCB+20)&0xFFFF]=(RAM[(DCB+20)&0xFFFF]&0xF0)|0x03;
  else RAM[(DCB+20)&0xFFFF]=RAM[(DCB+20)&0xFFFF]&0xF0;
  switch (RAM[DCB])
  {
   case 0:
    if (Verbose&8) printf ("Disk %d: Clearing DCB\n",nr+1);
    disk_lastblock[nr]=-1;
    disk_status[nr]=disk_newstatus[nr]=0x80;
    break;
   case 1:
    if (Verbose&8) printf ("Disk %d: Requesting status\n",nr+1);
    RAM[DCB]=disk_status[nr]=disk_newstatus[nr];
/*  disk_newstatus[nr]=0x80; */
    RAM[(DCB+17)]=blocksize&255;
    RAM[(DCB+18)]=blocksize>>8;
    break;
   case 2:
    if (Verbose&8) printf ("Disk %d: Soft reset\n",nr+1);
    RAM[DCB]=disk_status[nr]=disk_newstatus[nr]=0x80;
    break;
   case 3:
   case 4:
    disk_status[nr]=disk_newstatus[nr];
    block=RAM[(DCB+5)&0xFFFF]+RAM[(DCB+6)&0xFFFF]*256+
          RAM[(DCB+7)&0xFFFF]*65536+RAM[(DCB+8)&0xFFFF]*256*65536;
    block&=DiskMask[nr];
    if (disk_status[nr]==0x9B)
    {
     if (Verbose&8)
      printf ("Disk %d: %s block %d failed - device busy\n",
               nr+1,(RAM[DCB]==4)?"Reading":"Writing",block);
     RAM[DCB]=0x9B;
    }
    else if (DiskStream[nr])
    {
     if (block>=DiskSize[nr])
     {
      RAM[(DCB+20)&0xFFFF]|=2;
      len=0;
      disk_lastblock[nr]=-1;
      break;
     }
     if (block==disk_lastblock[nr] || RAM[DCB]==3)
     {
      addr=RAM[(DCB+1)&0xFFFF]+RAM[(DCB+2)&0xFFFF]*256;
      len=RAM[(DCB+3)&0xFFFF]+RAM[(DCB+4)&0xFFFF]*256;
      if (len>blocksize) len=blocksize;
      if (Verbose&8)
       printf ("Disk %d: %s %d bytes at %04X starting at block %d\n",
               nr+1,(RAM[DCB]==4)? "Reading":"Writing",len,addr,block);
      #ifdef DEBUG_IO
      printf("%s Disk %d, Block %d, length %d\n",((RAM[DCB]==4)? "Read":"Write"),nr+1,block,len);
      #endif
      block*=blocksize/512;
      for (i=0,j=0;i<len;i+=512,j++)
      {
       if (!((RAM[DCB]==4)? diskread(addr+i,(len-i<512)? len-i:512,
                                    (block&(~7))|interleavetable[block&7],
                                     DiskStream[nr]) :
                            diskwrite(addr+i,(len-i<512)? len-i:512,
                                      (block&(~7))|interleavetable[block&7],
                                      DiskStream[nr])))
       {
        RAM[(DCB+20)&0xFFFF]|=6;
        len=0;
        disk_lastblock[nr]=-1;
        break;
       }
       block++;
      }
      if (RAM[DCB]==3)
      {
       disk_lastblock[nr]=-1;
       DiskChanged[nr]=1;
      }
      RAM[DCB]=/*0;*/
      disk_status[nr]=disk_newstatus[nr]=0x80;
     }
     else
     {
      if (Verbose&8)
       printf ("Disk %d: Caching block %d\n",nr+1,block);
      RAM[DCB]=/*0;*/
      disk_status[nr]=disk_newstatus[nr]=0x9B;
      disk_timeout[nr]=DiskSpeed;
      disk_lastblock[nr]=block;
     }
    }
    break;
   default:
    if (Verbose&8)
     printf ("Disk %d: Unknown operation %d\n",nr+1,RAM[DCB]);
  }
 }
 else
 {
  if (Verbose&8) printf ("Disk %d: Reading status (0x%02X)\n",nr+1,RAM[DCB]);
  RAM[DCB]=disk_status[nr];
 }
}

static void UpdateTape (int mode,int nr,unsigned DCB)
{
 static int addr=0,len=0;
 int block,i,j,nodeshift;
 if (mode&127)
 {
  if (nr&1)
   nodeshift=4;
  else
   nodeshift=0;
  RAM[(DCB+20)&0xFFFF]=0;
  if (!TapeStream[nr&2]) RAM[(DCB+20)&0xFFFF]|=0x03;
  if (!TapeStream[(nr&2)+1]) RAM[(DCB+20)&0xFFFF]|=0x30;
  switch (RAM[DCB])
  {
   case 0:
    if (Verbose&8) printf ("Tape %d: Clearing DCB\n",nr+1);
    tape_lastblock[nr]=-1;
    tape_status[nr]=tape_newstatus[nr]=0x80;
    break;
   case 1:
    if (Verbose&8) printf ("Tape %d: Requesting status\n",nr+1);
    RAM[DCB]=tape_status[nr]=tape_newstatus[nr];
/*  tape_newstatus[nr]=0x80; */
    RAM[(DCB+17)]=1024&255;
    RAM[(DCB+18)]=1024>>8;
    break;
   case 2:
    if (Verbose&8) printf ("Tape %d: Soft reset\n",nr+1);
    RAM[DCB]=tape_status[nr]=tape_newstatus[nr]=0x80;
    break;
   case 3:
   case 4:
    tape_status[nr]=tape_newstatus[nr];
    block=RAM[(DCB+5)&0xFFFF]+RAM[(DCB+6)&0xFFFF]*256+
          RAM[(DCB+7)&0xFFFF]*65536+RAM[(DCB+8)&0xFFFF]*256*65536;
    block&=TapeMask[nr];
    if (tape_status[nr]==0x9B)
    {
     if (Verbose&8)
      printf ("Tape %d: %s block %d failed - device busy\n",
               nr+1,(RAM[DCB]==4)?"Reading":"Writing",block);
     RAM[DCB]=0x9B;
    }
    else if (TapeStream[nr])
    {
     if (block>=TapeSize[nr])
     {
      RAM[(DCB+20)&0xFFFF]|=2;
      len=0;
      tape_lastblock[nr]=-1;
      break;
     }
     if (block==tape_lastblock[nr] || RAM[DCB]==3)
     {
      addr=RAM[(DCB+1)&0xFFFF]+RAM[(DCB+2)&0xFFFF]*256;
      len=RAM[(DCB+3)&0xFFFF]+RAM[(DCB+4)&0xFFFF]*256;
      if (len>1024) len=1024;
      if (Verbose&8)
       printf ("Tape %d: %s %d bytes at %04X starting at block %d\n",
               nr+1,(RAM[DCB]==4)? "Reading":"Writing",len,addr,block);
      #ifdef DEBUG_IO
      printf("%s Tape %d, Block %d, length %d\n",((RAM[DCB]==4)? "Read":"Write"),nr+1,block,len);
      #endif
      block*=2;
      for (i=0,j=0;i<len;i+=512,j++)
      {
       if (!((RAM[DCB]==4)? diskread(addr+i,(len-i<512)? len-i:512,
                                     block,TapeStream[nr]) :
                            diskwrite(addr+i,(len-i<512)? len-i:512,
                                      block,TapeStream[nr])))
       {
        RAM[(DCB+20)&0xFFFF]|=2;
        len=0;
        tape_lastblock[nr]=-1;
        break;
       }
       block++;
      }
      if (RAM[DCB]==3)
      {
       tape_lastblock[nr]=-1;
       TapeChanged[nr]=1;
      }
      RAM[DCB]=/*0;*/
      tape_status[nr]=tape_newstatus[nr]=0x80;
     }
     else
     {
      if (Verbose&8)
       printf ("Tape %d: Caching block %d\n",nr+1,block);
      RAM[DCB]=/*0;*/
      tape_status[nr]=tape_newstatus[nr]=0x9B;
      tape_timeout[nr]=TapeSpeed;
      tape_lastblock[nr]=block;
     }
    }
    break;
   default:
    if (Verbose&8)
     printf ("Tape %d: Unknown operation %d\n",nr+1,RAM[DCB]);
  }
 }
 else
 {
  if (Verbose&8) printf ("Tape %d: Reading status (0x%02X)\n",nr+1,RAM[DCB]);
  RAM[DCB]=tape_status[nr];
 }
}

/****************************************************************************/
/*** This function is called when a DCB is read from or written to        ***/
/****************************************************************************/
static void UpdateDCB (int mode,unsigned DCB)
{
 int dev_id;
/* if (mode && (RAM[DCB]>=0x80 || RAM[DCB]==0)) return; */
 dev_id=(RAM[(DCB+9)&0xFFFF]<<4)+(RAM[(DCB+16)&0xFFFF]&0x0F);
 switch (dev_id)
 {
  case 1:
   UpdateKeyboard (mode,DCB);
   break;
  case 2:
   UpdatePrinter (mode,DCB);
   break;
  case 4:
  case 5:
  case 6:
  case 7:
   UpdateDisk (mode,dev_id-4,DCB);
   break;
  case 8:
  case 9:
  case 24:
  case 25:
   UpdateTape (mode,dev_id/16+((dev_id&1)*2),DCB);
   break;
  default:
   RAM[DCB]=0x9B;                     /* Unknown device - return time out   */
 }
}

/****************************************************************************/
/*** This function is called when the PCB is read from or written to      ***/
/****************************************************************************/
static void UpdatePCB (int mode)
{
 unsigned oldPCB;
 if (mode)
 {
  switch (RAM[PCB])
  {
   case 1:                            /* Synchronise Z80A                   */
    if (Verbose&16) puts ("Z80A synchronised");
    RAM[PCB]=129;
    break;
   case 2:                            /* Synchronise master 6801            */
    if (Verbose&16) puts ("Master 6801 synchronised");
    RAM[PCB]=130;
    break;
   case 3:                            /* Relocate PCB                       */
    /* Set status byte */
    RAM[PCB]=131;
    /* Get the new PCB address and update the PCB lookup table */
    oldPCB=PCB;
    PCB=RAM[(PCB+1)&0xFFFF]+RAM[(PCB+2)&0xFFFF]*256;
    if (Verbose&16) printf ("PCB relocated to %04X\n",PCB);
    SetPCBTable (oldPCB);
    break;
   default:
    if (RAM[PCB]!=0 && RAM[PCB]<0x80)
     if (Verbose&16) printf ("Unimplemented PCB operation %d\n",RAM[PCB]);
  }
 }
 else
 {
  if (Verbose&16) printf ("PCB: Reading status (%02X)\n",RAM[PCB]);
 }
}

/****************************************************************************/
/*** Check if a given address is in the PCB/DCB area, given that          ***/
/*** PCBTable[address]==1                                                 ***/
/****************************************************************************/
static int IsInPCB (dword a)
{
 /* Check if PCB is mapped in */
 if (a<8192 && (CurrMemSel&3)!=1) return 0;
 if (a<32768 && (CurrMemSel&3)!=1 && (CurrMemSel&3)!=3) return 0;
 if (a>32767 && (CurrMemSel&12)!=0) return 0;
 /* Check number of active devices */
 if (a>=(dword)(PCB+4+RAM[(PCB+3)&0xFFFF]*21)) return 0;
 return 1;
}

/****************************************************************************/
/*** This function is called when the PCB is read from                    ***/
/****************************************************************************/
void ReadPCB (dword A)
{
 if (!IsInPCB(A)) return;
 if (A==PCB) UpdatePCB (0);
 else if (A<(dword)(PCB+4+RAM[(PCB+3)&0xFFFF]*21)) UpdateDCB (0,A);
}

/****************************************************************************/
/*** This function is called when the PCB is written to                   ***/
/****************************************************************************/
void WritePCB (dword A)
{
 if (!IsInPCB(A)) return;
 if (A==PCB) UpdatePCB (1);
 else if (A<(dword)(PCB+4+RAM[(PCB+3)&0xFFFF]*21)) UpdateDCB (1,A);
}

/****************************************************************************/
/*** This function is called when ADAMnet has been reset                  ***/
/****************************************************************************/
static void ResetPCB (void)
{
 if (!UsePCB)
 {
  UsePCB=1;
  SetPCBTable(0);
 }
}

/****************************************************************************/
/*** Check for sprite collisions and 5th sprite in a row                  ***/
/*** If mode is 0, do not update VDP status byte                          ***/
/****************************************************************************/
static void CheckSprites(int mode)
{
 byte *T,*S1,*S2,*P1,*P2;
 byte *SprGen;
 int mask1,mask2,maskT;
 int SpriteCount[256];
 int SpriteSize,ScreenSize,X1,Y1,X2,Y2,N1,N2,V,H;
 for (Y1=0;Y1<255;++Y1)
 {
  SpriteCount[Y1]=0;
  LastSprite[Y1]=32;
 }
 SpriteSize=(Sprites16x16)? 16:8;
 ScreenSize=SpriteSize*((BigSprites)? 2:1);
 SprGen=VDP.VRAM+(((VDP.Reg[6]&7)*2048)&VDP.VRAMSize);
 S1=VDP.VRAM+(((VDP.Reg[5]&127)*128)&VDP.VRAMSize);
 for (N1=0;N1<32 && S1[0]!=208;N1++,S1+=4)
 {
  Y1=S1[0]; if (Y1>=234) Y1=Y1-255; else ++Y1;
  for (N2=0,Y2=(Y1&255);N2<ScreenSize;N2++,Y2=(Y2+1)&255)
  {
   if (Y2<192 && ++SpriteCount[Y2]==5)
   {
    LastSprite[Y2]=N1-1;
    if (mode && (VDP.Status&0x40)==0)
    {
     VDP.Status=(VDP.Status&0xA0)|0x40|N1;
    }
   }
  }
 }
 S1=VDP.VRAM+(((VDP.Reg[5]&127)*128)&VDP.VRAMSize);
 for (N1=0;N1<32 && S1[0]!=208;N1++,S1+=4)
 {
  Y1=S1[0]; if (Y1>=234) Y1=Y1-255; else ++Y1;
  X1=S1[1]; if (S1[3]&0x80) X1-=32;
  if (mode && X1>-ScreenSize && X1<256 && Y1>-ScreenSize && Y1<192)
  {
   mask1=(SpriteSize==16)? 0xFFFF:0xFF;
   if (X1<0)
    mask1>>=((-X1)/((BigSprites)?2:1));
   else if (X1+ScreenSize>=256)
    mask1<<=((257-X1)/((BigSprites)?2:1));
   for (N2=N1+1,S2=S1+4;N2<32 && S2[0]!=208 && (VDP.Status&0x20)==0;N2++,S2+=4)
   {
    Y2=S2[0]; if (Y2>=234) Y2=Y2-255; else ++Y2;
    X2=S2[1]; if (S2[3]&0x80) X2-=32;
    if (X2>-ScreenSize && X2<256 && Y2>-ScreenSize && Y2<192)
    {
     mask2=(SpriteSize==16)? 0xFFFF:0xFF;
     if (X2<0)
      mask2>>=((-X2)/((BigSprites)?2:1));
     else if (X2+ScreenSize>=256)
      mask2<<=((257-X2)/((BigSprites)?2:1));
     V=abs(Y2-Y1); H=abs(X2-X1);
     if (BigSprites)
     {
      V=V/2+(V&1);
      H=H/2+(H&1);
     }
     if (V<SpriteSize && H<SpriteSize)
     {
      if (SpriteSize==8)
      {
       P1=SprGen+S1[2]*8;
       P2=SprGen+S2[2]*8;
       if (Y2-Y1>=0) { P1+=V; } else { P2+=V; Y2=Y1; }
       if (X2-X1>=0)
       {
        T=P1;P1=P2;P2=T;
        maskT=mask1;mask1=mask2;mask2=maskT;
       }
       for (V=8-V;V;--V,P1++,P2++,Y2++)
       {
        if (LastSprite[Y2]>=N1 && LastSprite[Y2]>=N2)
        {
         X2=P2[0]&mask2;
         if (X2&((P1[0]&mask1)>>H))
         {
          VDP.Status|=0x20;
          break;
         }
        }
       }
      }
      else
      {
       P1=SprGen+(S1[2]&0xFC)*8;
       P2=SprGen+(S2[2]&0xFC)*8;
       if (Y2-Y1>=0) { P1+=V; } else { P2+=V; Y2=Y1; }
       if (X2-X1>=0)
       {
        T=P1;P1=P2;P2=T;
        maskT=mask1;mask1=mask2;mask2=maskT;
       }
       for (V=16-V;V;--V,P1++,P2++,Y2++)
       {
        if (LastSprite[Y2]>=N1 && LastSprite[Y2]>=N2)
        {
         X2=(P2[0]*256+P2[16])&mask2;
         if (X2&(((P1[0]*256+P1[16])&mask1)>>H))
         {
          VDP.Status|=0x20;
          break;
         }
        }
       }
      }
     }
    }
   }
  }
 }
 if (!Support5thSprite)
  for (Y1=0;Y1<255;++Y1)
  {
   SpriteCount[Y1]=0;
   LastSprite[Y1]=32;
  }
}

/****************************************************************************/
/*** Refresh screen, check keyboard and sprites                           ***/
/****************************************************************************/
static int VDP_Interrupt(void)
{
 static int UCount=1;
 int dorefresh,i;
 /* Increase interrupt count */
 ++numVDPInts;
 /* Update sound log file */
 if (SoundStream) UpdateSoundStream();
 /* Check for keyboard events */
 Keyboard ();
 /* Check keyboard buffer and delete events that are too old */
 for (i=0;i<16;++i)
  if (keyboard_buffer[i])
   if (++keyboard_buffer_int_count[i]>MAX_KEYBOARD_INT_COUNT)
   {
    keyboard_buffer[i]=0;
    keyboard_buffer_int_count[i]=0;
   }
 /* Check spinner events */
 if (SpinnerPosition[0]<-1000) SpinnerPosition[0]=-1000;
 if (SpinnerPosition[0]>1000) SpinnerPosition[0]=1000;
 if (SpinnerPosition[1]<-1000) SpinnerPosition[1]=-1000;
 if (SpinnerPosition[1]>1000) SpinnerPosition[1]=1000;
 if (SpinnerPosition[0]<0)
  SpinnerJoyState[0]=0x2000;
 else if (SpinnerPosition[0]>0)
  SpinnerJoyState[0]=0x0000;
 else
  SpinnerJoyState[0]=0x3000;
 if (SpinnerPosition[1]>0)
  SpinnerJoyState[1]=0x2000;
 else if (SpinnerPosition[1]<0)
  SpinnerJoyState[1]=0x0000;
 else
  SpinnerJoyState[1]=0x3000;
 SpinnerParam[0]=(0x10000*abs(SpinnerPosition[0]))/1000;
 SpinnerParam[1]=(0x10000*abs(SpinnerPosition[1]))/1000;
 if (SpinnerParam[0]) SpinnerRecur[0]=0x10000;
 if (SpinnerParam[1]) SpinnerRecur[1]=0x10000;
 /* Check if a screen refresh should be done */
 switch (CheckScreenRefresh())
 {
  case 0:  
   dorefresh=0;
   break;
  case 1:
   dorefresh=1;
   break;
  default:
   if (!--UCount)
   {
    if (!UPeriod)
     UPeriod=2;
    UCount=UPeriod;
    dorefresh=1;
   }
   else
    dorefresh=0;
   break;
 }
 if (VDP.VKey)
 {
  /* Update VDP status reg */
  VDP.Status=0x9F;
 }
 switch((VDP.Reg[0]&0x02)|(VDP.Reg[1]&0x18))
 {
  case 0x00: case 0x02: case 0x0A: case 0x08:
   CheckSprites(VDP.VKey);
 }
 if (dorefresh)
 {
  RefreshScreen(VDP.ScreenChanged);
  VDP.ScreenChanged=0;
 }
 /* Don't process interrupt if VDP is being accessed */
 if (!VDP.VKey) return Z80_IGNORE_INT;
 /* Return interrupt id */
 if (VDP.Reg[1]&0x20)
 {
  if (NMI_Line)
   return Z80_IGNORE_INT;
  else
  {
   NMI_Line=1;
   return Z80_NMI_INT;
  }
 }
 else
  return Z80_IGNORE_INT;
}

static int SpinnerInterrupt (void)
{
 int doint=0;
 SpinnerRecur[0]+=SpinnerParam[0];
 SpinnerRecur[1]+=SpinnerParam[1];
 JoyState[0]|=0x3000;
 JoyState[1]|=0x3000;
 if (SpinnerRecur[0]>0xFFFF)
 {
  SpinnerRecur[0]&=0xFFFF;
  JoyState[0]=(JoyState[0]&0xCFFF)|SpinnerJoyState[0];
  doint=1;
 }
 if (SpinnerRecur[1]>0xFFFF)
 {
  SpinnerRecur[1]&=0xFFFF;
  JoyState[1]=(JoyState[1]&0xCFFF)|SpinnerJoyState[1];
  doint=1;
 }
 return (doint)? 0x00FF:Z80_IGNORE_INT;
}

/****************************************************************************/
/*** This function is called every interrupt. It checks if a VDP or a     ***/
/*** spinner interrupt should occur                                       ***/
/****************************************************************************/
int Z80_Interrupt (void)
{
 int i;
 for (i=0;i<4;++i)
 {
  if (disk_timeout[i])
  {
   if (!--disk_timeout[i]) disk_newstatus[i]=0x80;
  }
  if (tape_timeout[i])
  {
   if (!--tape_timeout[i]) tape_newstatus[i]=0x80;
  }
 }
 IFreqRecur+=IFreqParam;
 if (IFreqRecur>0xFFFFF)
 {
  IFreqRecur&=0xFFFFF;
  return VDP_Interrupt ();
 }
 return SpinnerInterrupt();
}

void Z80_Retn (void) { };
void Z80_Reti (void) { };

/****************************************************************************/
/*** Change disk and tape images, printer log file, etc.                  ***/
/****************************************************************************/
#ifndef NO_OPTIONS_DIALOGUE
void OptionsDialogue (void)
{
 static char _TapeName[4][256],_DiskName[4][256],
             _PrnName[256],_LPTName[256],_SoundName[256];
 char buf[256];
 char *p;
 int tmp;
 fflush (stdin);
 do
 {
  fprintf (stderr,
           "Options currently in use are:\n"
           "Disk A image           - %s\n"
           "Disk B image           - %s\n"
           "Disk C image           - %s\n"
           "Disk D image           - %s\n"
           "Tape A image           - %s\n"
           "Tape B image           - %s\n"
           "Tape C image           - %s\n"
           "Tape D image           - %s\n"
           "Printer log file       - %s\n"
           "Parallel port log file - %s\n"
           "Sound log file         - %s\n"
           "Interrupt frequency    - %dHz\n"
           "Z80 CPU speed          - %u%%\n"
           "Available commands are:\n"
           "da,db,dc,dd <filename> - Change disk image\n"
           "ta,tb,tc,td <filename> - Change tape image\n"
           "p <filename>           - Change printer log file\n"
           "l <filename>           - Change parallel port log file\n"
           "s <filename>           - Change sound log file\n"
           "i <value>              - Change interrupt frequency\n"
           "c <percentage>         - Change Z80 CPU speed\n"
           "b                      - Back\n"
           "q                      - Quit emulator\n> ",
           (DiskStream[0])? DiskName[0]:"none",
           (DiskStream[1])? DiskName[1]:"none",
           (DiskStream[2])? DiskName[2]:"none",
           (DiskStream[3])? DiskName[3]:"none",
           (TapeStream[0])? TapeName[0]:"none",
           (TapeStream[1])? TapeName[1]:"none",
           (TapeStream[2])? TapeName[2]:"none",
           (TapeStream[3])? TapeName[3]:"none",
           (PrnStream)? PrnName:"none",
           (LPTStream)? LPTName:"none",
           (SoundStream)? SoundName:"none",
           IFreq,
           Z80_IPeriod*100*1000/3579545);
  buf[2]=buf[3]='\0';
  fflush(stdout);
  fgets (buf,sizeof(buf),stdin);
  p=strchr (buf,'\n');
  if (p) *p='\0';
  fflush (stdin);
  switch (buf[0])
  {
   case 'd':
    tmp=buf[1]-'a';
    if (tmp>=0 && tmp<4)
    {
     DiskClose (tmp);
     strcpy (_DiskName[tmp],buf+3);
     DiskName[tmp]=_DiskName[tmp];
     DiskOpen (tmp);
    }
    break;
   case 't':
    tmp=buf[1]-'a';
    if (tmp>=0 && tmp<4)
    {
     TapeClose (tmp);
     strcpy (_TapeName[tmp],buf+3);
     TapeName[tmp]=_TapeName[tmp];
     TapeOpen (tmp);
    }
    break;
   case 'p':
    ClosePrinter ();
    strcpy (_PrnName,buf+2);
    PrnName=_PrnName;
    OpenPrinter ();
    break;
   case 'l':
    CloseLPT ();
    strcpy (_LPTName,buf+2);
    LPTName=_LPTName;
    OpenLPT ();
    break;
   case 's':
    CloseSoundLog ();
    strcpy (_SoundName,buf+2);
    SoundName=_SoundName;
    OpenSoundLog ();
    break;
   case 'i':
    IFreq=atoi(buf+2);
    if (IFreq<10) IFreq=10;
    if (IFreq>200) IFreq=200;
    IFreqParam=(0x100000*IFreq)/1000;
    if (SoundStream)
    {
     fputc (0xFC,SoundStream);
     fputc (IFreq,SoundStream);
    }
    break;
   case 'c':
    tmp=atoi(buf+2);
    if (tmp<10) tmp=10;
    if (tmp>1000) tmp=1000;
    Z80_IPeriod=(3579545*tmp)/(100*1000);
    break;
   case 'q':
    Z80_Running=0;
    break;
  }
 }
 while (buf[0]!='q' && buf[0]!='b');
 VDP.ScreenChanged=1;
}
#endif /* NO_OPTIONS_DIALOGUE */
