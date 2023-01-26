/*** Z80Em: Portable Z80 emulator *******************************************/
/***                                                                      ***/
/***                                Z80IO.h                               ***/
/***                                                                      ***/
/*** This file contains the prototypes for the functions accessing memory ***/
/*** and I/O                                                              ***/
/***                                                                      ***/
/*** Copyright (C) Marcel de Kogel 1996,1997,1998                         ***/
/***     You are not allowed to distribute this software commercially     ***/
/***     Please, notify me, if you make any changes to this file          ***/
/****************************************************************************/

#define INLINE_OP                     /* Inline Z80_RDOP()                  */
#define INLINE_MEM                    /* Inline Z80_RDMEM() and Z80_WRMEM() */

#if defined(__GNUC__) && !defined(PSP) && !defined(MSDOS) && !defined(__APPLE__)
 #define FASTCALL        __attribute__ ((regparm(3)))
 #ifdef INLINE_MEM
  #define INLINE_MEM_GNU
 #endif
#else
 #define FASTCALL
#endif

/****************************************************************************/
/* Input a byte from given I/O port                                         */
/****************************************************************************/
byte FASTCALL Z80_In (unsigned Port);

/****************************************************************************/
/* Output a byte to given I/O port                                          */
/****************************************************************************/
void FASTCALL Z80_Out (unsigned Port,byte Value);

#ifndef INLINE_MEM_GNU
 /***************************************************************************/
 /* Read a byte from given memory location                                  */
 /***************************************************************************/
 unsigned FASTCALL Z80_RDMEM(dword a);
 /***************************************************************************/
 /* Write a byte to given memory location                                   */
 /***************************************************************************/
 void FASTCALL Z80_WRMEM(dword a,byte v);
#else
 extern byte *AddrTabl[256];
 extern byte *WriteAddrTabl[256];
 extern byte PCBTable[65536];
 void ReadPCB (dword a);
 void WritePCB (dword a);
 extern __inline__ unsigned Z80_RDMEM (dword a)
 {
  int retval=AddrTabl[a>>8][a&0xFF];
  if (PCBTable[a]) ReadPCB (a);
  return retval;
 }
 extern __inline__ void Z80_WRMEM (dword a,byte v)
 {
  WriteAddrTabl[a>>8][a&0xFF]=v;
  if (PCBTable[a]) WritePCB (a);
 }
#endif

#ifdef INLINE_OP
 extern byte *AddrTabl[256];
 extern byte *WriteAddrTabl[256];
 #define Z80_RDOP(a) AddrTabl[(a)>>8][(a)&0xFF]
 #define Z80_RDSTACK(a) AddrTabl[(a)>>8][(a)&0xFF]
 #define Z80_WRSTACK(a,v) WriteAddrTabl[(a)>>8][(a)&0xFF]=v
#else
 #define Z80_RDOP(a) Z80_RDMEM(a)
 #define Z80_RDSTACK(a) Z80_RDMEM(a)
 #define Z80_WRSTACK(a,v) Z80_WRMEM(a,v)
#endif

#define Z80_RDOP_ARG(a) Z80_RDOP(a)

