/****************************************************************************/
/***                                                                      ***/
/***  AdamNet "Bus over IP" master for ADAMEm.   See AdamNet.h.           ***/
/***                                                                      ***/
/***  ADAMEm acts as the AdamNet master: for forwarded device IDs it runs ***/
/***  real AdamNet wire transactions over a TCP socket to fujinet-pc.     ***/
/***  The wire protocol mirrors fujinet's lib/bus/adamnet + device code.  ***/
/***                                                                      ***/
/****************************************************************************/

#include "AdamNet.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

/* --- AdamNet wire command/response nibbles (high nibble of the byte) --- */
#define MN_RESET   0x00
#define MN_STATUS  0x01
#define MN_ACK     0x02
#define MN_CLR     0x03   /* clear to send (CTS) */
#define MN_RECEIVE 0x04
#define MN_CANCEL  0x05
#define MN_SEND    0x06
#define MN_NACK    0x07
#define MN_READY   0x0D

#define NR_STATUS  0x08
#define NR_ACK     0x09
#define NR_CANCEL  0x0A
#define NR_SEND    0x0B
#define NR_NACK    0x0C

#define CMD(c,dev)  (unsigned char)(((c) << 4) | ((dev) & 0x0F))
#define RESP(r,dev) (unsigned char)(((r) << 4) | ((dev) & 0x0F))

#define ADAMNET_BLOCK_SIZE 1024

/* Timeouts (ms). Generous; localhost transactions complete in microseconds. */
#define TMO_ACK         300
#define TMO_DATA       8000
#define TMO_RECV_POLL     5    /* per RECEIVE re-poll while device seeks       */
#define TMO_RECV_TOTAL 8000    /* total budget waiting out a block seek (TNFS) */

extern int Verbose;            /* from Coleco.c                               */

static int an_enabled    = 0;
static int an_listen_fd  = -1;
static int an_conn_fd    = -1;

/* Default routing: forward the FujiNet-owned device IDs, keep keyboard/tape
   local. Bit N set => device id N is forwarded to fujinet. */
static unsigned long an_forward_mask =
    (1UL << 0x02) |                                   /* printer            */
    (1UL << 0x04) | (1UL << 0x05) | (1UL << 0x06) | (1UL << 0x07) | /* disks */
    (1UL << 0x09) | (1UL << 0x0A) | (1UL << 0x0B) |
    (1UL << 0x0C) | (1UL << 0x0D) | (1UL << 0x0E) |   /* network            */
    (1UL << 0x0F);                                    /* Fuji gateway       */

/****************************************************************************/
/*** Socket setup                                                         ***/
/****************************************************************************/
int AdamNet_Init (int port)
{
 struct sockaddr_in addr;
 int on=1;

 an_listen_fd=socket (AF_INET,SOCK_STREAM,0);
 if (an_listen_fd<0) { perror ("AdamNet: socket"); return -1; }

 setsockopt (an_listen_fd,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on));

 memset (&addr,0,sizeof(addr));
 addr.sin_family=AF_INET;
 addr.sin_addr.s_addr=htonl (INADDR_ANY);
 addr.sin_port=htons ((unsigned short)port);

 if (bind (an_listen_fd,(struct sockaddr *)&addr,sizeof(addr))<0)
 {
  perror ("AdamNet: bind");
  close (an_listen_fd); an_listen_fd=-1;
  return -1;
 }
 if (listen (an_listen_fd,4)<0)
 {
  perror ("AdamNet: listen");
  close (an_listen_fd); an_listen_fd=-1;
  return -1;
 }
 fcntl (an_listen_fd,F_SETFL,O_NONBLOCK);

 an_enabled=1;
 printf ("AdamNet: listening for fujinet-pc on TCP port %d\n",port);
 return 0;
}

void AdamNet_Shutdown (void)
{
 if (an_conn_fd>=0)   { close (an_conn_fd);   an_conn_fd=-1; }
 if (an_listen_fd>=0) { close (an_listen_fd); an_listen_fd=-1; }
 an_enabled=0;
}

int AdamNet_Enabled (void) { return an_enabled; }

int AdamNet_IsForwarded (int dev_id)
{
 if (!an_enabled) return 0;
 if (dev_id<0 || dev_id>31) return 0;
 return (an_forward_mask >> dev_id) & 1UL;
}

int AdamNet_Connected (void)
{
 int fd,on=1;

 if (!an_enabled) return 0;
 if (an_conn_fd>=0) return 1;
 if (an_listen_fd<0) return 0;

 fd=accept (an_listen_fd,NULL,NULL);
 if (fd<0) return 0;                      /* nothing waiting (non-blocking)   */

 setsockopt (fd,IPPROTO_TCP,TCP_NODELAY,&on,sizeof(on));
 an_conn_fd=fd;
 printf ("AdamNet: fujinet-pc connected\n");
 return 1;
}

static void an_disconnect (void)
{
 if (an_conn_fd>=0) { close (an_conn_fd); an_conn_fd=-1; }
 printf ("AdamNet: fujinet-pc disconnected\n");
}

static long an_ms_since (struct timeval *t0)
{
 struct timeval t1;
 gettimeofday (&t1,NULL);
 return (t1.tv_sec-t0->tv_sec)*1000 + (t1.tv_usec-t0->tv_usec)/1000;
}

int AdamNet_WaitForConnection (int timeout_ms)
{
 struct timeval t0;
 unsigned char st;

 if (!an_enabled) return 0;
 gettimeofday (&t0,NULL);

 printf ("AdamNet: waiting up to %ds for a responsive fujinet-pc "
         "(start fujinet-pc with BoIP -> this port)...\n",timeout_ms/1000);
 fflush (stdout);

 while (an_ms_since (&t0)<timeout_ms)
 {
  struct timeval tp;

  /* Wait for a TCP connection. */
  if (!AdamNet_Connected ())
  {
   usleep (50000);                         /* 50 ms                           */
   continue;
  }

  /* The socket is up, but the peer might be another platform's fujinet that
     grabbed the default BoIP port (1985 is shared by Apple/ADAM), or a real
     ADAM fujinet still finishing its setup. Probe the Fuji device: if it
     answers, it speaks AdamNet and is servicing the bus. */
  printf ("AdamNet: probing fujinet bus...\n");
  fflush (stdout);
  gettimeofday (&tp,NULL);
  while (an_ms_since (&tp)<3000 && an_ms_since (&t0)<timeout_ms)
  {
   if (AdamNet_DiskStatus (0x0F,&st)==0)
   {
    printf ("AdamNet: fujinet bus is live; booting.\n");
    return 1;
   }
   usleep (50000);
  }

  /* Not answering AdamNet within the window: most likely a different fujinet
     hijacked the port. Drop it and wait for the real ADAM fujinet to connect. */
  printf ("AdamNet: peer isn't answering AdamNet (another fujinet on this port?); "
          "dropping it and waiting for the ADAM fujinet.\n");
  if (an_conn_fd>=0) { close (an_conn_fd); an_conn_fd=-1; }
  usleep (200000);
 }
 return 0;
}

/****************************************************************************/
/*** Low-level byte transport                                             ***/
/****************************************************************************/
static int an_send (const unsigned char *buf,int len)
{
 int off=0,n;
 if (an_conn_fd<0) return -1;
 while (off<len)
 {
  n=send (an_conn_fd,buf+off,len-off,0);
  if (n>0) { off+=n; continue; }
  if (n<0 && (errno==EINTR)) continue;
  an_disconnect ();
  return -1;
 }
 return 0;
}

static int an_send_byte (unsigned char b) { return an_send (&b,1); }

/* Receive exactly len bytes, with an overall timeout in ms. Returns 0 ok. */
static int an_recv (unsigned char *buf,int len,int timeout_ms)
{
 int got=0,n;
 fd_set rfds;
 struct timeval tv;

 if (an_conn_fd<0) return -1;
 while (got<len)
 {
  FD_ZERO (&rfds);
  FD_SET (an_conn_fd,&rfds);
  tv.tv_sec=timeout_ms/1000;
  tv.tv_usec=(timeout_ms%1000)*1000;
  n=select (an_conn_fd+1,&rfds,NULL,NULL,&tv);
  if (n<0) { if (errno==EINTR) continue; an_disconnect (); return -1; }
  if (n==0) return -1;                     /* timeout                         */
  n=recv (an_conn_fd,buf+got,len-got,0);
  if (n>0) { got+=n; continue; }
  if (n<0 && errno==EINTR) continue;
  an_disconnect ();                         /* peer closed or error            */
  return -1;
 }
 return 0;
}

/* Receive a single byte; returns the byte 0-255, or -1 on timeout/error. */
static int an_recv_byte (int timeout_ms)
{
 unsigned char b;
 if (an_recv (&b,1,timeout_ms)) return -1;
 return b;
}

/* Discard any bytes already waiting in the socket. A slow (e.g. TNFS-backed)
   block read makes the device seek-stall while we re-poll CONTROL.RECEIVE; when
   it finally completes it can ACK several of those buffered re-polls, leaving
   duplicate ACKs queued. Dropping them keeps the data packet from being
   mis-read. Non-blocking; only ever sees bytes the device already sent. */
static void an_drain (void)
{
 unsigned char scratch[64];
 fd_set rfds;
 struct timeval tv;
 if (an_conn_fd<0) return;
 for (;;)
 {
  FD_ZERO (&rfds);
  FD_SET (an_conn_fd,&rfds);
  tv.tv_sec=0; tv.tv_usec=0;            /* poll, never block                   */
  if (select (an_conn_fd+1,&rfds,NULL,NULL,&tv)<=0) break;
  if (recv (an_conn_fd,scratch,sizeof scratch,0)<=0) break;
 }
}

static unsigned char an_checksum (const unsigned char *buf,int len)
{
 unsigned char ck=0;
 int i;
 for (i=0;i<len;++i) ck^=buf[i];
 return ck;
}

/****************************************************************************/
/*** AdamNet master transactions                                          ***/
/****************************************************************************/

/* Send a 5-byte "block number" SEND packet and wait for the device ACK. */
static int an_send_block_num (int dev,unsigned long block)
{
 unsigned char pkt[5];

 pkt[0]= block        & 0xFF;
 pkt[1]=(block >>  8) & 0xFF;
 pkt[2]=(block >> 16) & 0xFF;
 pkt[3]=(block >> 24) & 0xFF;
 pkt[4]=0x00;

 if (an_send_byte (CMD(MN_SEND,dev))) return -1;
 if (an_send_byte (0x00)) return -1;        /* length high (0x0005)            */
 if (an_send_byte (0x05)) return -1;        /* length low                      */
 if (an_send (pkt,5)) return -1;
 if (an_send_byte (an_checksum (pkt,5))) return -1;

 if (an_recv_byte (TMO_ACK)!=RESP(NR_ACK,dev)) return -1;
 return 0;
}

int AdamNet_DiskStatus (int dev,unsigned char *status_byte)
{
 unsigned char pkt[6];

 if (an_conn_fd<0) return -1;
 if (an_send_byte (CMD(MN_STATUS,dev))) return -1;

 /* Device replies with a 6-byte status packet:
    [0x80|dev], length(LE16), devtype, status, checksum. */
 if (an_recv (pkt,6,TMO_ACK)) return -1;
 if (pkt[0]!=RESP(NR_STATUS,dev)) return -1;
 if (status_byte) *status_byte=pkt[4];
 return 0;
}

int AdamNet_ReadBlock (int dev,unsigned long block,unsigned char *buf)
{
 unsigned char hdr[3];
 int r,len;
 struct timeval t0,t1;

 if (an_conn_fd<0) return -1;

 /* 1. Tell the device which block we want. */
 if (an_send_block_num (dev,block)) return -1;

 /* 2. CONTROL.RECEIVE: device reads the block and ACKs. During seek emulation
       it stays silent and we re-poll until it is ready. */
 gettimeofday (&t0,NULL);
 for (;;)
 {
  if (an_send_byte (CMD(MN_RECEIVE,dev))) return -1;
  r=an_recv_byte (TMO_RECV_POLL);
  if (r==RESP(NR_ACK,dev)) break;
  if (r==RESP(NR_NACK,dev)) return -1;
  if (r<0)                              /* silent (seeking) -> re-poll         */
  {
   gettimeofday (&t1,NULL);
   if ((t1.tv_sec-t0.tv_sec)*1000+(t1.tv_usec-t0.tv_usec)/1000 > TMO_RECV_TOTAL)
    return -1;
   continue;
  }
  return -1;                            /* unexpected byte                     */
 }

 /* A slow seek makes the device ACK several of our buffered RECEIVE re-polls;
    drop the surplus ACKs before asking for data so the packet header lines up. */
 an_drain ();

 /* 3. CONTROL.CLR (CTS): device streams the data packet
       [0xB0|dev], length(BE16=1024), <1024 bytes>, checksum. Tolerate any stray
       ACK byte that slips in just ahead of the header. */
 if (an_send_byte (CMD(MN_CLR,dev))) return -1;
 do { if (an_recv (hdr,1,TMO_DATA)) return -1; } while (hdr[0]==RESP(NR_ACK,dev));
 if (hdr[0]!=RESP(NR_SEND,dev)) return -1;
 if (an_recv (hdr+1,2,TMO_DATA)) return -1;
 len=(hdr[1]<<8)|hdr[2];                 /* big-endian length                  */
 if (len!=ADAMNET_BLOCK_SIZE) return -1;
 if (an_recv (buf,ADAMNET_BLOCK_SIZE,TMO_DATA)) return -1;
 if (an_recv_byte (TMO_DATA)<0) return -1;   /* checksum byte                   */

 return 0;
}

/* --- Non-blocking block read (keeps the Z80 running during a slow seek) --- */

static struct timeval an_rb_t0;     /* read start (overall timeout)            */
static struct timeval an_rb_last;   /* last RECEIVE re-poll send               */

int AdamNet_ReadBlockBegin (int dev,unsigned long block)
{
 if (an_conn_fd<0) return -1;
 an_drain ();                               /* resync from any prior transaction */
 if (an_send_block_num (dev,block)) return -1;
 if (an_send_byte (CMD(MN_RECEIVE,dev))) return -1;
 gettimeofday (&an_rb_t0,NULL);
 an_rb_last=an_rb_t0;
 return 0;
}

int AdamNet_ReadBlockReady (int dev,unsigned char *buf)
{
 unsigned char hdr[3];
 struct timeval now;
 int r,len;

 if (an_conn_fd<0) return -1;

 r=an_recv_byte (0);                        /* non-blocking peek for the ACK     */
 if (r==RESP(NR_ACK,dev))
 {
  an_drain ();                              /* drop duplicate seek-poll ACKs     */
  if (an_send_byte (CMD(MN_CLR,dev))) return -1;
  do { if (an_recv (hdr,1,TMO_DATA)) return -1; } while (hdr[0]==RESP(NR_ACK,dev));
  if (hdr[0]!=RESP(NR_SEND,dev)) return -1;
  if (an_recv (hdr+1,2,TMO_DATA)) return -1;
  len=(hdr[1]<<8)|hdr[2];
  if (len!=ADAMNET_BLOCK_SIZE) return -1;
  if (an_recv (buf,ADAMNET_BLOCK_SIZE,TMO_DATA)) return -1;
  if (an_recv_byte (TMO_DATA)<0) return -1;
  return 1;                                 /* done                              */
 }
 if (r==RESP(NR_NACK,dev)) return -1;
 if (r>=0) return -1;                       /* unexpected byte                   */

 /* Still seeking. Honour the overall budget, and re-poll RECEIVE at most every
    couple of ms (the EOS status loop calls us far more often than that). */
 gettimeofday (&now,NULL);
 if ((now.tv_sec-an_rb_t0.tv_sec)*1000+(now.tv_usec-an_rb_t0.tv_usec)/1000 > TMO_RECV_TOTAL)
  return -1;
 if ((now.tv_sec-an_rb_last.tv_sec)*1000+(now.tv_usec-an_rb_last.tv_usec)/1000 >= 2)
 {
  if (an_send_byte (CMD(MN_RECEIVE,dev))) return -1;
  an_rb_last=now;
 }
 return 0;                                  /* pending                           */
}

int AdamNet_WriteBlock (int dev,unsigned long block,const unsigned char *buf)
{
 if (an_conn_fd<0) return -1;

 /* 1. Block number. */
 if (an_send_block_num (dev,block)) return -1;

 /* 2. SEND the 1024-byte block data; device ACKs. */
 if (an_send_byte (CMD(MN_SEND,dev))) return -1;
 if (an_send_byte (0x04)) return -1;        /* length high (0x0400 = 1024)     */
 if (an_send_byte (0x00)) return -1;        /* length low                      */
 if (an_send (buf,ADAMNET_BLOCK_SIZE)) return -1;
 if (an_send_byte (an_checksum (buf,ADAMNET_BLOCK_SIZE))) return -1;

 if (an_recv_byte (TMO_ACK)!=RESP(NR_ACK,dev)) return -1;
 return 0;
}

/* --- Character device transactions (EXPERIMENTAL, see AdamNet.h) --- */

int AdamNet_CharStatus (int dev,unsigned char *status_byte,unsigned *pending_len)
{
 unsigned char pkt[6];
 if (an_conn_fd<0) return -1;
 if (an_send_byte (CMD(MN_STATUS,dev))) return -1;
 if (an_recv (pkt,6,TMO_ACK)) return -1;
 if (pkt[0]!=RESP(NR_STATUS,dev)) return -1;
 if (pending_len)  *pending_len=(unsigned)(pkt[1] | (pkt[2]<<8)); /* LE16 */
 if (status_byte)  *status_byte=pkt[4];
 return 0;
}

int AdamNet_CharWrite (int dev,const unsigned char *buf,int len)
{
 if (an_conn_fd<0 || len<0) return -1;
 if (an_send_byte (CMD(MN_SEND,dev))) return -1;
 if (an_send_byte ((unsigned char)((len>>8)&0xFF))) return -1;  /* length hi (BE) */
 if (an_send_byte ((unsigned char)(len&0xFF))) return -1;       /* length lo      */
 if (len && an_send (buf,len)) return -1;
 if (an_send_byte (an_checksum (buf,len))) return -1;
 if (an_recv_byte (TMO_ACK)!=RESP(NR_ACK,dev)) return -1;
 return 0;
}

int AdamNet_CharRead (int dev,unsigned char *buf,int maxlen,int *got)
{
 unsigned char hdr[3];
 int len,take,r;
 if (got) *got=0;
 if (an_conn_fd<0) return -1;

 /* 1. CONTROL.RECEIVE: the device stages its pending data (e.g. a JSON query
       result) into its response buffer and ACKs, or NACKs if nothing is ready.
       This step is required before CLR -- without it response_len stays 0 and
       the device NACKs the CLR (empty read). */
 if (an_send_byte (CMD(MN_RECEIVE,dev))) return -1;
 r=an_recv_byte (TMO_ACK);
 if (r==RESP(NR_NACK,dev)) { if (got) *got=0; return 0; }  /* nothing available */
 if (r!=RESP(NR_ACK,dev)) return -1;

 /* 2. CONTROL.CLR (CTS): device streams its staged response, or NACKs if none. */
 if (an_send_byte (CMD(MN_CLR,dev))) return -1;
 if (an_recv (hdr,1,TMO_DATA)) return -1;
 if (hdr[0]==RESP(NR_NACK,dev)) { if (got) *got=0; return 0; } /* nothing pending */
 if (hdr[0]!=RESP(NR_SEND,dev)) return -1;
 if (an_recv (hdr+1,2,TMO_DATA)) return -1;
 len=(hdr[1]<<8)|hdr[2];                  /* big-endian length               */
 take=(len<maxlen)?len:maxlen;
 if (take>0 && an_recv (buf,take,TMO_DATA)) return -1;
 /* drain any remainder we couldn't store, plus the checksum byte */
 {
  unsigned char scratch[64];
  int rem=len-take,n;
  while (rem>0)
  {
   n=(rem<(int)sizeof(scratch))?rem:(int)sizeof(scratch);
   if (an_recv (scratch,n,TMO_DATA)) return -1;
   rem-=n;
  }
 }
 if (an_recv_byte (TMO_DATA)<0) return -1;     /* checksum                    */
 if (got) *got=take;
 return 0;
}

void AdamNet_ResetDevice (int dev)
{
 int d;
 if (an_conn_fd<0) return;
 if (dev==0xFF)
 {
  for (d=0;d<16;++d)
   if (AdamNet_IsForwarded (d)) an_send_byte (CMD(MN_RESET,d));
 }
 else
  an_send_byte (CMD(MN_RESET,dev));
}
