/** ADAMEm: Coleco ADAM emulator ********************************************/
/**                                                                        **/
/**                               SnapEdit.c                               **/
/**                                                                        **/
/** This program allows the user to change various parameters of snapshot  **/
/** files                                                                  **/
/**                                                                        **/
/** Copyright (C) Marcel de Kogel 1997,1998,1999                           **/
/**     You are not allowed to distribute this software commercially       **/
/**     Please, notify me, if you make any changes to this file            **/
/****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef ZLIB
#include <zlib.h>
#define fopen           gzopen
#define fclose          gzclose
#define fread(B,N,L,F)  gzread(F,B,(L)*(N))
#define fwrite(B,N,L,F) gzwrite(F,B,(L)*(N))
#endif

#define MIN_SNAP_SIZE   19842
#define MAX_SNAP_SIZE   (4*1024*1024)

static unsigned char snapdata[MAX_SNAP_SIZE];
static char cart[256],disk[4][256],tape[4][256];

int main (int argc,char *argv[])
{
 FILE *f;
 char buf[256];
 int i,len;
 if (argc!=2)
 {
  printf ("Usage: %s <ADAMEm snapshot file>\n",argv[0]);
  return 1;
 }
 f=fopen (argv[1],"rb");
 if (!f)
 {
  printf ("Cannot open %s\n",argv[1]);
  return 2;
 }
 len=fread (snapdata,1,MAX_SNAP_SIZE,f);
 if (fread(buf,1,1,f))
 {
  printf ("File %s is too large\n",argv[1]);
  return 3;
 }
 fclose (f);
 memcpy (buf,snapdata,16);
 buf[15]='\0';
 if (len<MIN_SNAP_SIZE || strcmp(buf,"ADAMEm snapshot"))
 {
  printf ("File %s is not an ADAMEm snapshot image\n",argv[1]);
  return 4;
 }
 i=snapdata[16]*256+snapdata[17];
 if (i>0x100)
 {
  printf ("File version not supported\n");
  return 5;
 }
 memcpy (cart,snapdata+20,256); cart[255]='\0';
 for (i=0;i<4;++i)
 {
  memcpy (disk[i],snapdata+20+256+i*256,256); disk[i][255]='\0';
  memcpy (tape[i],snapdata+20+256+1024+i*256,256); tape[i][255]='\0';
 }
 do
 {
  printf ("Cartridge ROM image: %s\n",(cart[0]=='\0')? "none":cart);
  for (i=0;i<4;++i)
   printf ("Disk %c image: %s\n",'A'+i,(disk[i][0]=='\0')? "none":disk[i]);
  for (i=0;i<4;++i)
   printf ("Tape %c image: %s\n",'A'+i,(tape[i][0]=='\0')? "none":tape[i]);
  printf ("\nc <filename>: Change cartridge image\n"
          "da <filename>: Change disk A image\n"
          "db <filename>: Change disk B image\n"
          "dc <filename>: Change disk C image\n"
          "dd <filename>: Change disk D image\n"
          "ta <filename>: Change tape A image\n"
          "tb <filename>: Change tape B image\n"
          "tc <filename>: Change tape C image\n"
          "td <filename>: Change tape D image\n"
          "s : Save changes\n"
          "q : Quit this program\n");
  memset (buf,'\0',sizeof(buf));
  fgets (buf,sizeof(buf),stdin); buf[strlen(buf)-1]='\0';
  switch (tolower(buf[0]))
  {
   case 'c':
    strcpy (cart,buf+2);
    break;
   case 'd':
    i=tolower(buf[1])-'a';
    if (i>=0 && i<4) strcpy (disk[i],buf+3);
    break;
   case 't':
    i=tolower(buf[1])-'a';
    if (i>=0 && i<4) strcpy (tape[i],buf+3);
    break;
   case 's':
    f=fopen (argv[1],"wb");
    if (!f)
     printf ("Cannot open %s\n",argv[1]);
    else
    {
     memcpy (snapdata+20,cart,256);
     for (i=0;i<4;++i)
     {
      memcpy (snapdata+20+256+i*256,disk[i],256);
      memcpy (snapdata+20+256+1024+i*256,tape[i],256);
     }
     i=fwrite (snapdata,1,len,f);
     if (i!=len)
      printf ("Write error\n");
     else
      printf ("Save successful\n");
     fclose (f);
    }
    break;
   case 'q':
    return 0;
  }
 }
 while (1);
}

