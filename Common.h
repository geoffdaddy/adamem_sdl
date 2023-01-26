/** ADAMEm: Coleco ADAM emulator ********************************************/
/**                                                                        **/
/**                                Common.h                                **/
/**                                                                        **/
/** This file contains the system-independent part of the screen refresh   **/
/** drivers and general modem drivers                                      **/
/**                                                                        **/
/** Copyright (C) Marcel de Kogel 1996,1997,1998,1999                      **/
/**     You are not allowed to distribute this software commercially       **/
/**     Please, notify me, if you make any changes to this file            **/
/****************************************************************************/

/****************************************************************************/
/* Uncomment any of these if you don't want to use a particular driver      */
/****************************************************************************/
/* #define NO_COMMON_SCREEN_REFRESH_DRIVERS */
/* #define NO_COMMON_MODEM_DRIVERS */

#ifndef NO_COMMON_SCREEN_REFRESH_DRIVERS
/****************************************************************************/
/* Uncomment this if you want to use lookup tables for the screen refresh   */
/* Using lookup tables greatly increases the emulation speed, but is only   */
/* compatible with 32-bit machines                                          */
/****************************************************************************/
/* #define USE_LOOKUP_TABLE */

/****************************************************************************/
/* Uncomment this if you're using an 8 bbp video device                     */
/****************************************************************************/
/* #define _8BPP */

#ifdef _8BPP
#define PutPixel(P,C)   DisplayBuf[P]=C
#endif

/****************************************************************************/
/** Refresh sprites                                                        **/
/****************************************************************************/

#define BigPixel(P,C)    {\
   PutPixel (P,C);        \
   PutPixel (P+1,C);      \
   PutPixel (P+WIDTH,C);  \
   PutPixel (P+WIDTH+1,C);}

#define BigPixelClipped(X,Y,P,C) { \
   if (X>=0 && X<256 && Y>=0 && Y<192) PutPixel(P,C); \
   if (X+1>=0 && X+1<256 && Y>=0 && Y<192) PutPixel(P+1,C); \
   if (X>=0 && X<256 && Y+1>=0 && Y+1<192) PutPixel(P+WIDTH,C); \
   if (X+1>=0 && X+1<256 && Y+1>=0 && Y+1<192) PutPixel(P+WIDTH+1,C); }
#define PixelClipped(X,P,C) { \
   if (X>=0 && X<256) PutPixel(P,C); }

static void BigSprite (int X,int Y,int P,int J,int C)
{
 if (J)
 {
  if (X>=0 && X<242 && Y>=0 && Y<191)
  {
   if(J&0x80) BigPixel (P,C);
   if(J&0x40) BigPixel (P+2,C);
   if(J&0x20) BigPixel (P+4,C);
   if(J&0x10) BigPixel (P+6,C);
   if(J&0x08) BigPixel (P+8,C);
   if(J&0x04) BigPixel (P+10,C);
   if(J&0x02) BigPixel (P+12,C);
   if(J&0x01) BigPixel (P+14,C);
  }
  else if (X>=-1 && X<256 && Y>=-1 && Y<192)
  {
   if(J&0x80) BigPixelClipped (X,Y,P,C);
   if(J&0x40) BigPixelClipped (X+2,Y,P+2,C);
   if(J&0x20) BigPixelClipped (X+4,Y,P+4,C);
   if(J&0x10) BigPixelClipped (X+6,Y,P+6,C);
   if(J&0x08) BigPixelClipped (X+8,Y,P+8,C);
   if(J&0x04) BigPixelClipped (X+10,Y,P+10,C);
   if(J&0x02) BigPixelClipped (X+12,Y,P+12,C);
   if(J&0x01) BigPixelClipped (X+14,Y,P+14,C);
  }
 }
}

static void Sprite (int X,int Y,int P,int J,int C)
{
 if (Y>=0 && Y<192 && J)
 {
  if (X>=0 && X<248)
  {
   if (J&0x80) PutPixel (P+0,C);
   if (J&0x40) PutPixel (P+1,C);
   if (J&0x20) PutPixel (P+2,C);
   if (J&0x10) PutPixel (P+3,C);
   if (J&0x08) PutPixel (P+4,C);
   if (J&0x04) PutPixel (P+5,C);
   if (J&0x02) PutPixel (P+6,C);
   if (J&0x01) PutPixel (P+7,C);
  }
  else if (X>-8 && X<256)
  {
   if (J&0x80) PixelClipped (X+0,P+0,C);
   if (J&0x40) PixelClipped (X+1,P+1,C);
   if (J&0x20) PixelClipped (X+2,P+2,C);
   if (J&0x10) PixelClipped (X+3,P+3,C);
   if (J&0x08) PixelClipped (X+4,P+4,C);
   if (J&0x04) PixelClipped (X+5,P+5,C);
   if (J&0x02) PixelClipped (X+6,P+6,C);
   if (J&0x01) PixelClipped (X+7,P+7,C);
  }
 }
}

static void RefreshSprites(void)
{
 byte *T,*S;
 byte *SprGen;
 int SpriteSize,X,Y,C,N,P,y;
 SpriteSize=((Sprites16x16)? 16:8)*((BigSprites)? 2:1);
 SprGen=VDP.VRAM+(((VDP.Reg[6]&7)*2048)&VDP.VRAMSize);
 /* Get pointer to first sprite */
 S=VDP.VRAM+(((VDP.Reg[5]&127)*128)&VDP.VRAMSize);
 for(N=0;(N<32)&&(S[0]!=208);N++,S+=4); S-=4; N--;
 for (;N>=0;N--,S-=4)
 {
  C=S[3]&15;
  Y=S[0]; if (Y>=234) Y=Y-255; else ++Y;
  X=S[1]; if (S[3]&0x80) X-=32;
  if (X>-SpriteSize && X<256 && Y>-SpriteSize && Y<192 && C)
  {
   P=WIDTH*(HEIGHT-192)/2+(WIDTH-256)/2+X+Y*WIDTH;
   C=PalBuf[C];
   if (Sprites16x16)
   {
    T=SprGen+(S[2]&0xFC)*8;
    if (BigSprites)
    {
     for(y=16;y;y--,Y+=2,T++,P+=WIDTH*2)
     {
      if (LastSprite[Y&255]>=N)
      {
       BigSprite (X,Y,P,T[0],C);
       BigSprite (X+16,Y,P+16,T[16],C);
      }
     }
    }
    else
    {
     for(y=16;y;y--,Y++,T++,P+=WIDTH)
     {
      if (LastSprite[Y&255]>=N)
      {
       Sprite (X,Y,P,T[0],C);
       Sprite (X+8,Y,P+8,T[16],C);
      }
     }
    }
   }
   else
   {
    T=SprGen+S[2]*8;
    if (BigSprites)
     for (y=8;y;y--,Y+=2,T++,P+=WIDTH*2)
     {
      if (LastSprite[Y&255]>=N)
      {
       BigSprite (X,Y,P,T[0],C);
      }
     }
    else
     for (y=8;y;y--,Y++,T++,P+=WIDTH)
     {
      if (LastSprite[Y&255]>=N)
      {
       Sprite (X,Y,P,T[0],C);
      }
     }
   }
  }
 }
}

/****************************************************************************/
/** Refresh background                                                     **/
/****************************************************************************/
static void RefreshScreen0 (void)
{
 byte *S,*T,*PGT;
 int P,N,X,Y,J,K,BC,FC;
 P=WIDTH*(HEIGHT-192)/2+(WIDTH-240)/2;
 BC=PalBuf[VDP.BGColour];
 FC=PalBuf[VDP.FGColour];
 PGT=VDP.VRAM+(((VDP.Reg[4]&((VDP.Reg[0]&2)?4:7))*2048)&VDP.VRAMSize);
 T=VDP.VRAM+(((VDP.Reg[2]&15)*1024)&VDP.VRAMSize);
 for (N=0;N<3;++N)
 {
  for(Y=8;Y;Y--)
  {
   for (J=0;J<8;++J)
   {
    PutPixel(P-8,BC);PutPixel(P-7,BC);
    PutPixel(P-6,BC);PutPixel(P-5,BC);
    PutPixel(P-4,BC);PutPixel(P-3,BC);
    PutPixel(P-2,BC);PutPixel(P-1,BC);
    PutPixel(P+240,BC);PutPixel(P+241,BC);
    PutPixel(P+242,BC);PutPixel(P+243,BC);
    PutPixel(P+244,BC);PutPixel(P+245,BC);
    PutPixel(P+246,BC);PutPixel(P+247,BC);
    P+=WIDTH;
   }
   P-=8*WIDTH;
   for (X=40;X;X--)
   {
    S=PGT+T[0]*8;
    for (J=0;J<8;J++)
    {
     K=*S++;
     PutPixel (P+0,(K&0x80)? FC:BC);
     PutPixel (P+1,(K&0x40)? FC:BC);
     PutPixel (P+2,(K&0x20)? FC:BC);
     PutPixel (P+3,(K&0x10)? FC:BC);
     PutPixel (P+4,(K&0x08)? FC:BC);
     PutPixel (P+5,(K&0x04)? FC:BC);
     P+=WIDTH;
    }
    P+=6-WIDTH*8;
    T++;
   }
   P+=WIDTH*8-6*40;
  }
  if (VDP.Reg[0]&2)
  {
   if (N==0 && (VDP.Reg[4]&2)) PGT+=0x800;
   if (N==1)
   {
    switch (VDP.Reg[4]&3)
    {
     case 0: break;
     case 1: PGT+=0x1000; break;
     case 2: PGT-=0x800; break;
     case 3: PGT+=0x800; break;
    }
   }
  }
 }
}

#ifdef USE_LOOKUP_TABLE
static void RefreshScreen1 (void)
{
 int X,Y,K;
 byte *P,*S,*T,*PGT,*CLT;
 unsigned *s;
 P=DisplayBuf+WIDTH*(HEIGHT-192)/2+(WIDTH-256)/2;
 PGT=VDP.VRAM+(((VDP.Reg[4]&((VDP.Reg[0]&2)?4:7))*2048)&VDP.VRAMSize);
 CLT=VDP.VRAM+(((VDP.Reg[3]&((VDP.Reg[0]&2)?128:255))*64)&VDP.VRAMSize);
 T=VDP.VRAM+(((VDP.Reg[2]&15)*1024)&VDP.VRAMSize);
 for(Y=24;Y;Y--,P+=WIDTH*8-8*32)
  for(X=32;X;X--)
  {
   S=PGT+T[0]*8;
   s=font2screen+CLT[T[0]>>3]*16;
   K=S[0];
   *(unsigned*)(P+0+WIDTH*0)=s[K>>4];
   *(unsigned*)(P+4+WIDTH*0)=s[K&0x0F];
   K=S[1];
   *(unsigned*)(P+0+WIDTH*1)=s[K>>4];
   *(unsigned*)(P+4+WIDTH*1)=s[K&0x0F];
   K=S[2];
   *(unsigned*)(P+0+WIDTH*2)=s[K>>4];
   *(unsigned*)(P+4+WIDTH*2)=s[K&0x0F];
   K=S[3];
   *(unsigned*)(P+0+WIDTH*3)=s[K>>4];
   *(unsigned*)(P+4+WIDTH*3)=s[K&0x0F];
   K=S[4];
   *(unsigned*)(P+0+WIDTH*4)=s[K>>4];
   *(unsigned*)(P+4+WIDTH*4)=s[K&0x0F];
   K=S[5];
   *(unsigned*)(P+0+WIDTH*5)=s[K>>4];
   *(unsigned*)(P+4+WIDTH*5)=s[K&0x0F];
   K=S[6];
   *(unsigned*)(P+0+WIDTH*6)=s[K>>4];
   *(unsigned*)(P+4+WIDTH*6)=s[K&0x0F];
   K=S[7];
   *(unsigned*)(P+0+WIDTH*7)=s[K>>4];
   *(unsigned*)(P+4+WIDTH*7)=s[K&0x0F];
   P+=8;
   T++;
  }
}

static void RefreshScreen2 (void)
{
 int X,Y,K,N;
 byte *P,*S,*T,*PGT,*CLT,*C;
 unsigned *s;
 P=DisplayBuf+WIDTH*(HEIGHT-192)/2+(WIDTH-256)/2;
 PGT=VDP.VRAM+(((VDP.Reg[4]&((VDP.Reg[0]&2)?4:7))*2048)&VDP.VRAMSize);
 CLT=VDP.VRAM+(((VDP.Reg[3]&((VDP.Reg[0]&2)?128:255))*64)&VDP.VRAMSize);
 T=VDP.VRAM+(((VDP.Reg[2]&15)*1024)&VDP.VRAMSize);
 for(N=0;N<3;N++)
 {
  for(Y=8;Y;Y--)
  {
   for(X=32;X;X--)
   {
    S=PGT+T[0]*8;C=CLT+T[0]*8;
    K=S[0]; s=font2screen+C[0]*16;
    *(unsigned*)(P+0+WIDTH*0)=s[K>>4];
    *(unsigned*)(P+4+WIDTH*0)=s[K&0x0F];
    K=S[1]; s=font2screen+C[1]*16;
    *(unsigned*)(P+0+WIDTH*1)=s[K>>4];
    *(unsigned*)(P+4+WIDTH*1)=s[K&0x0F];
    K=S[2]; s=font2screen+C[2]*16;
    *(unsigned*)(P+0+WIDTH*2)=s[K>>4];
    *(unsigned*)(P+4+WIDTH*2)=s[K&0x0F];
    K=S[3]; s=font2screen+C[3]*16;
    *(unsigned*)(P+0+WIDTH*3)=s[K>>4];
    *(unsigned*)(P+4+WIDTH*3)=s[K&0x0F];
    K=S[4]; s=font2screen+C[4]*16;
    *(unsigned*)(P+0+WIDTH*4)=s[K>>4];
    *(unsigned*)(P+4+WIDTH*4)=s[K&0x0F];
    K=S[5]; s=font2screen+C[5]*16;
    *(unsigned*)(P+0+WIDTH*5)=s[K>>4];
    *(unsigned*)(P+4+WIDTH*5)=s[K&0x0F];
    K=S[6]; s=font2screen+C[6]*16;
    *(unsigned*)(P+0+WIDTH*6)=s[K>>4];
    *(unsigned*)(P+4+WIDTH*6)=s[K&0x0F];
    K=S[7]; s=font2screen+C[7]*16;
    *(unsigned*)(P+0+WIDTH*7)=s[K>>4];
    *(unsigned*)(P+4+WIDTH*7)=s[K&0x0F];
    P+=8;
    T++;
   }
   P+=WIDTH*8-8*32;
  }
  switch (N)
  {
   case 0:
    if (VDP.Reg[4]&2) PGT+=0x800;
    if (VDP.Reg[3]&0x40) CLT+=0x800;
    break;
   case 1:
    switch (VDP.Reg[4]&3)
    {
     case 0: break;
     case 1: PGT+=0x1000; break;
     case 2: PGT-=0x800; break;
     case 3: PGT+=0x800; break;
    }
    switch (VDP.Reg[3]&0x60)
    {
     case 0: break;
     case 0x20: CLT+=0x1000; break;
     case 0x40: CLT-=0x800; break;
     case 0x60: CLT+=0x800; break;
    }
    break;
  }
 }
}

static void RefreshScreen3 (void)
{
 byte *P,*T,*C,*PGT;
 int N,X,Y,C1,C2;
 P=DisplayBuf+WIDTH*(HEIGHT-192)/2+(WIDTH-256)/2;
 PGT=VDP.VRAM+(((VDP.Reg[4]&((VDP.Reg[0]&2)?4:7))*2048)&VDP.VRAMSize);
 T=VDP.VRAM+(((VDP.Reg[2]&15)*1024)&VDP.VRAMSize);
 for (N=0;N<3;++N)
 {
  for(Y=0;Y<8;Y++)
  {
   for(X=32;X;X--)
   {
    C=PGT+T[0]*8+((Y&3)<<1);
    C1=PalBuf[C[0]>>4]  *0x01010101L;
    C2=PalBuf[C[0]&0x0F]*0x01010101L;
    *(unsigned*)P=C1;P+=4;*(unsigned*)P=C2;P+=WIDTH-4;
    *(unsigned*)P=C1;P+=4;*(unsigned*)P=C2;P+=WIDTH-4;
    *(unsigned*)P=C1;P+=4;*(unsigned*)P=C2;P+=WIDTH-4;
    *(unsigned*)P=C1;P+=4;*(unsigned*)P=C2;P+=WIDTH-4;
    C1=PalBuf[C[1]>>4]  *0x01010101L;
    C2=PalBuf[C[1]&0x0F]*0x01010101L;
    *(unsigned*)P=C1;P+=4;*(unsigned*)P=C2;P+=WIDTH-4;
    *(unsigned*)P=C1;P+=4;*(unsigned*)P=C2;P+=WIDTH-4;
    *(unsigned*)P=C1;P+=4;*(unsigned*)P=C2;P+=WIDTH-4;
    *(unsigned*)P=C1;P+=4;*(unsigned*)P=C2;
    T++;
    P+=4-7*WIDTH;
   }
   P+=WIDTH*8-8*32;
  }
  if (VDP.Reg[0]&2)
  {
   if (N==0 && (VDP.Reg[4]&2)) PGT+=0x800;
   if (N==1)
   {
    switch (VDP.Reg[4]&3)
    {
     case 0: break;
     case 1: PGT+=0x1000; break;
     case 2: PGT-=0x800; break;
     case 3: PGT+=0x800; break;
    }
   }
  }
 }
}
#else
static void RefreshScreen1 (void)
{
 int X,Y,K,I,BC,FC,P;
 byte *S,*T,*PGT,*CLT;
 P=WIDTH*(HEIGHT-192)/2+(WIDTH-256)/2;
 PGT=VDP.VRAM+(((VDP.Reg[4]&((VDP.Reg[0]&2)?4:7))*2048)&VDP.VRAMSize);
 CLT=VDP.VRAM+(((VDP.Reg[3]&((VDP.Reg[0]&2)?128:255))*64)&VDP.VRAMSize);
 T=VDP.VRAM+(((VDP.Reg[2]&15)*1024)&VDP.VRAMSize);
 for(Y=24;Y;Y--)
 {
  for(X=32;X;X--)
  {
   S=PGT+T[0]*8;
   BC=CLT[T[0]>>3];
   FC=PalBuf[BC>>4]; BC=PalBuf[BC&0x0F];
   for(I=0;I<8;I++,P+=WIDTH)
   {
    K=*S++;
    PutPixel (P+0,(K&0x80)? FC:BC);
    PutPixel (P+1,(K&0x40)? FC:BC);
    PutPixel (P+2,(K&0x20)? FC:BC);
    PutPixel (P+3,(K&0x10)? FC:BC);
    PutPixel (P+4,(K&0x08)? FC:BC);
    PutPixel (P+5,(K&0x04)? FC:BC);
    PutPixel (P+6,(K&0x02)? FC:BC);
    PutPixel (P+7,(K&0x01)? FC:BC);
   }
   T++;
   P+=8-WIDTH*8;
  }
  P+=WIDTH*8-8*32;
 }
}

static void RefreshScreen2 (void)
{
 int X,Y,K,I,N,FC,BC,P;
 byte *S,*T,*PGT,*CLT,*C;
 P=WIDTH*(HEIGHT-192)/2+(WIDTH-256)/2;
 PGT=VDP.VRAM+(((VDP.Reg[4]&((VDP.Reg[0]&2)?4:7))*2048)&VDP.VRAMSize);
 CLT=VDP.VRAM+(((VDP.Reg[3]&((VDP.Reg[0]&2)?128:255))*64)&VDP.VRAMSize);
 T=VDP.VRAM+(((VDP.Reg[2]&15)*1024)&VDP.VRAMSize);
 for(N=0;N<3;N++)
 {
  for(Y=8;Y;Y--)
  {
   for(X=32;X;X--)
   {
    S=PGT+T[0]*8; C=CLT+T[0]*8;
    for(I=0;I<8;I++)
    {
     BC=PalBuf[C[0]&0x0F];
     FC=PalBuf[C[0]>>4];
     ++C;
     K=*S++;
     PutPixel (P+0,(K&0x80)? FC:BC);
     PutPixel (P+1,(K&0x40)? FC:BC);
     PutPixel (P+2,(K&0x20)? FC:BC);
     PutPixel (P+3,(K&0x10)? FC:BC);
     PutPixel (P+4,(K&0x08)? FC:BC);
     PutPixel (P+5,(K&0x04)? FC:BC);
     PutPixel (P+6,(K&0x02)? FC:BC);
     PutPixel (P+7,(K&0x01)? FC:BC);
     P+=WIDTH;
    }
    T++;
    P+=8-WIDTH*8;
   }
   P+=WIDTH*8-8*32;
  }
  if (N==0 && (VDP.Reg[4]&2)) PGT+=0x800;
  if (N==0 && (VDP.Reg[3]&0x40)) CLT+=0x800;
  if (N==1)
  {
   switch (VDP.Reg[4]&3)
   {
    case 0: break;
    case 1: PGT+=0x1000; break;
    case 2: PGT-=0x800; break;
    case 3: PGT+=0x800; break;
   }
   switch (VDP.Reg[3]&0x60)
   {
    case 0: break;
    case 0x20: CLT+=0x1000; break;
    case 0x40: CLT-=0x800; break;
    case 0x60: CLT+=0x800; break;
   }
  }
 }
}

static void RefreshScreen3 (void)
{
 byte *T,*C,*PGT;
 int X,Y,C1,C2,N,P;
 P=WIDTH*(HEIGHT-192)/2+(WIDTH-256)/2;
 PGT=VDP.VRAM+(((VDP.Reg[4]&((VDP.Reg[0]&2)?4:7))*2048)&VDP.VRAMSize);
 T=VDP.VRAM+(((VDP.Reg[2]&15)*1024)&VDP.VRAMSize);
 for (N=0;N<3;++N)
 {
  for(Y=0;Y<8;Y++)
  {
   for(X=32;X;X--)
   {
    C=PGT+T[0]*8+((Y&3)<<1);
    C1=PalBuf[C[0]>>4]; C2=PalBuf[C[0]&0x0F];
    PutPixel(P+0,C1); PutPixel(P+1,C1); PutPixel(P+2,C1); PutPixel(P+3,C1);
    PutPixel(P+4,C2); PutPixel(P+5,C2); PutPixel(P+6,C2); PutPixel(P+7,C2);
    P+=WIDTH;
    PutPixel(P+0,C1); PutPixel(P+1,C1); PutPixel(P+2,C1); PutPixel(P+3,C1);
    PutPixel(P+4,C2); PutPixel(P+5,C2); PutPixel(P+6,C2); PutPixel(P+7,C2);
    P+=WIDTH;
    PutPixel(P+0,C1); PutPixel(P+1,C1); PutPixel(P+2,C1); PutPixel(P+3,C1);
    PutPixel(P+4,C2); PutPixel(P+5,C2); PutPixel(P+6,C2); PutPixel(P+7,C2);
    P+=WIDTH;
    PutPixel(P+0,C1); PutPixel(P+1,C1); PutPixel(P+2,C1); PutPixel(P+3,C1);
    PutPixel(P+4,C2); PutPixel(P+5,C2); PutPixel(P+6,C2); PutPixel(P+7,C2);
    P+=WIDTH;
    C1=PalBuf[C[1]>>4]; C2=PalBuf[C[1]&0x0F];
    PutPixel(P+0,C1); PutPixel(P+1,C1); PutPixel(P+2,C1); PutPixel(P+3,C1);
    PutPixel(P+4,C2); PutPixel(P+5,C2); PutPixel(P+6,C2); PutPixel(P+7,C2);
    P+=WIDTH;
    PutPixel(P+0,C1); PutPixel(P+1,C1); PutPixel(P+2,C1); PutPixel(P+3,C1);
    PutPixel(P+4,C2); PutPixel(P+5,C2); PutPixel(P+6,C2); PutPixel(P+7,C2);
    P+=WIDTH;
    PutPixel(P+0,C1); PutPixel(P+1,C1); PutPixel(P+2,C1); PutPixel(P+3,C1);
    PutPixel(P+4,C2); PutPixel(P+5,C2); PutPixel(P+6,C2); PutPixel(P+7,C2);
    P+=WIDTH;
    PutPixel(P+0,C1); PutPixel(P+1,C1); PutPixel(P+2,C1); PutPixel(P+3,C1);
    PutPixel(P+4,C2); PutPixel(P+5,C2); PutPixel(P+6,C2); PutPixel(P+7,C2);
    T++;
    P+=8-7*WIDTH;
   }
   P+=WIDTH*8-8*32;
  }
  if (VDP.Reg[0]&2)
  {
   if (N==0 && (VDP.Reg[4]&2)) PGT+=0x800;
   if (N==1)
   {
    switch (VDP.Reg[4]&3)
    {
     case 0: break;
     case 1: PGT+=0x1000; break;
     case 2: PGT-=0x800; break;
     case 3: PGT+=0x800; break;
    }
   }
  }
 }
}
#endif

static void RefreshScreenConflict (void)
{
 int X,Y,BC,FC,P;
 P=WIDTH*(HEIGHT-192)/2+(WIDTH-256)/2;
 FC=PalBuf[VDP.FGColour];
 BC=PalBuf[VDP.BGColour];
 for(Y=192;Y;Y--,P+=WIDTH)
 {
  for(X=16;X;X--)
  {
   PutPixel(P+0,FC); PutPixel(P+1,FC); PutPixel(P+2,FC); PutPixel(P+3,FC);
   PutPixel(P+4,FC); PutPixel(P+5,FC); PutPixel(P+6,FC); PutPixel(P+7,FC);
   P+=8;
   PutPixel(P+0,BC); PutPixel(P+1,BC); PutPixel(P+2,BC); PutPixel(P+3,BC);
   PutPixel(P+4,BC); PutPixel(P+5,BC); PutPixel(P+6,BC); PutPixel(P+7,BC);
   P+=8;
  }
 }
}

#ifdef TEXT80
static int CheckTDOS (void)             /* Return offset of screenbuffer    */
{
 int addr;
 addr=RAM[0x01]+RAM[0x02]*256+0x6D-3;   /* Read block buffer routine        */
 addr=RAM[addr]+RAM[addr+1]*256;        /* Get the address                  */
 if (RAM[addr+0]==0xF5 &&               /* Check routine code               */
     RAM[addr+1]==0xC5 &&
     RAM[addr+2]==0xD5 &&
     RAM[addr+3]==0xCD &&
     RAM[addr+6]==0x30 &&
     RAM[addr+8]==0xE1 &&
     RAM[addr+9]==0x11 &&
     RAM[addr+12]==0x01 &&
     RAM[addr+13]==0x00 &&
     RAM[addr+14]==0x04 &&
     RAM[addr+15]==0xED &&
     RAM[addr+16]==0xB0 &&
     (VDP.Reg[0]&0x02)==0x00 &&         /* Check video mode                 */
     (VDP.Reg[1]&0x18)==0x10)
  return RAM[addr+10]+RAM[addr+11]*256+0x400;
 return 0;
}
#endif

void RefreshScreen (int ScreenChanged)
{
 int i;
 if (ScreenON)
 {
#ifdef TEXT80
  int addr;
  addr=CheckTDOS ();
  if (AutoText80)                       /* Switch to/from Text80 mode if    */
  {                                     /* necessary                        */
   if (addr)
   { if (!Text80) InitText80(); }
   else
   { if (Text80) ResetText80(); }
  }
  if (Text80)
  {
   int num_lines;
   i=RAM[0x01]+RAM[0x02]*256+0x64-3;               /* Get number of lines   */
   i=RAM[i]+RAM[i+1]*256;                          /* ... to display        */
   i=RAM[i+3]+RAM[i+4]*256;
   num_lines=RAM[i]+1;
   if (num_lines>24) num_lines=24;                 /* Just in case ...      */
   memcpy(DisplayBuf,RAM+addr,num_lines*80);       /* Copy offscreen buffer */
   addr=((VDP.Reg[2]&15)*1024)&VDP.VRAMSize;
   for (i=num_lines;i<24;++i)
   {
    memcpy(DisplayBuf+i*80,VDP.VRAM+addr+i*40,40); /* Copy smartkey display */
    memset(DisplayBuf+i*80+40,' ',40);             /* Clear rest of buffer  */
   }
   Text80Colour=VDP.FGColour;
  }
  else
#endif
  {
   if (!ScreenChanged) return;
   switch((VDP.Reg[0]&0x02)|(VDP.Reg[1]&0x18))
   {
    case 0x12:
    case 0x10:
     RefreshScreen0();
     break;
    case 0x00:
     RefreshScreen1();
     RefreshSprites();
     break;
    case 0x02:
     RefreshScreen2();
     RefreshSprites();
     break;
    case 0x0A:
    case 0x08:
     RefreshScreen3();
     RefreshSprites();
     break;
    default:
     RefreshScreenConflict();
     break;
   }
  }
 }
 else
 {
#ifdef TEXT80
  if (Text80)
  {
   for (i=0;i<80*24;++i)
    DisplayBuf[i]=' ';
  }
  else
#endif
  for (i=0;i<WIDTH*HEIGHT;++i) PutPixel (i,PalBuf[VDP.BGColour]);
 }
 PutImage ();
}

/****************************************************************************/
/** Function to be called when the palette changes                         **/
/****************************************************************************/
void ColourScreen (void)
{
#if defined(MSDOS) || defined (LINUX_SVGA)
 VGA_SetBkColour (VDP.BGColour);
#else
 int I;
 static int OldBGColour=-1;
 if (OldBGColour==VDP.BGColour) return;
 OldBGColour=VDP.BGColour;
 PalBuf[0]=VDP.BGColour? PalBuf[VDP.BGColour]:Pal0;
 for (I=0;I<WIDTH*HEIGHT;++I) PutPixel (I,PalBuf[0]);
#endif
}
#endif  /* NO_COMMON_SCREEN_REFRESH_DRIVERS */

#ifndef NO_COMMON_MODEM_DRIVERS
int ModemIn (int reg)
{
 return -1;
}

void ModemOut (int reg,int value)
{
}
#endif  /* NO_COMMON_MODEM_DRIVERS */
