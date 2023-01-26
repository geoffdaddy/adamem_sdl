/** ADAMEm: Coleco ADAM emulator ********************************************/
/**                                                                        **/
/**                               makedata.c                               **/
/**                                                                        **/
/** Program to build adamem.snd                                            **/
/**                                                                        **/
/** Copyright (C) Marcel de Kogel 1996,1997,1998,1999                      **/
/**     You are not allowed to distribute this software commercially       **/
/**     Please, notify me, if you make any changes to this file            **/
/****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef ZLIB
#include <zlib.h>
#endif

static void ConvertSam (unsigned short *p,int len)
{
#ifndef LSB_FIRST
 int i;
 for (i=0;i<len;++i) p[i]=((p[i]<<8)&0xFF00)|((p[i]>>8)&0x00FF);
#endif
}

static int LoadSam (unsigned short **p,char *f,int *len)
{
 FILE *F;
 char s[20];
 strcpy (s,f);
 strcat (s,".pcm");
 printf ("Loading %s... ",s);
 F=fopen (s,"rb");
 if (!F)
 {
  puts ("NOT FOUND");
  return 0;
 }
 fseek(F,0,SEEK_END);
 *len=ftell(F)/2;
 fseek(F,0,SEEK_SET);
 *p=malloc ((*len)*2);
 if (!(*p))
 {
  puts ("OUT OF MEMORY");
  return 0;
 }
 fread (*p,2,*len,F);
 fclose (F);
 ConvertSam (*p,*len);
 printf ("%u words\n",*len);
 return 1;
}

int main (void)
{
 static char *sample_file[15]=
 { "MEL1","MEL2","MEL3","MEL4","MEL5",
   "WN1","WN2","WN3","WN4","WN5",
   "PN1","PN2","PN3","PN4","PN5"
 };
 static unsigned short *sample_ptr[15];
 static unsigned sample_params[15][5];
 FILE *f;
 int i;
 for (i=0;i<15;++i)
  if (LoadSam(&sample_ptr[i],sample_file[i],&sample_params[i][0])==0) return 1;
 printf ("Opening SamInfo... ");
 f=fopen ("SamInfo","rt");
 if (!f) { puts("NOT FOUND");return 0; }
 for (i=0;i<15;++i)
  fscanf (f,"%u,%u,%u,%u\n",&sample_params[i][1],&sample_params[i][2],
                            &sample_params[i][3],&sample_params[i][4]);
 fclose (f);
 printf ("OK\nWriting adamem.snd... ");
 fflush (stdout);
#ifdef ZLIB
#define fopen           gzopen
#define fclose          gzclose
#define fwrite(B,N,L,F) gzwrite(F,B,(L)*(N))
#endif
 f=fopen ("adamem.snd","wb");
 if (!f) { puts("FAILED");return 1; }
 fwrite (sample_params,15*5,sizeof(unsigned),f);
 for (i=0;i<15;++i)
  fwrite (sample_ptr[i],2,sample_params[i][0],f);
 fclose (f);
 puts ("OK");
 return 0;
}
