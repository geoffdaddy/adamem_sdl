/****************************************************************************/
/***                                                                      ***/
/***  AdamNet "Bus over IP" master for ADAMEm.                            ***/
/***                                                                      ***/
/***  ADAMEm normally emulates ADAM peripherals at the DCB level. This    ***/
/***  module lets ADAMEm instead act as the AdamNet *master* (the 6801    ***/
/***  network processor) for a configured set of device IDs, performing   ***/
/***  real AdamNet wire transactions over a TCP socket to fujinet-pc      ***/
/***  (built for the ADAM target with Bus-over-IP). ADAMEm listens;       ***/
/***  fujinet-pc connects in.                                             ***/
/***                                                                      ***/
/****************************************************************************/

#ifndef ADAMNET_BRIDGE_H
#define ADAMNET_BRIDGE_H

/* Default TCP port for the AdamNet-over-IP link, used when -fujinet is given
   without an explicit port. Matches fujinet-pc's ADAM CONFIG_DEFAULT_BOIP_PORT. */
#define ADAMNET_DEFAULT_PORT 65216

/* Open a listening TCP socket on the given port. Returns 0 on success. */
int  AdamNet_Init (int port);

/* Close listener and any peer connection. */
void AdamNet_Shutdown (void);

/* 1 if the bridge was initialised (a -fujinet port was given). */
int  AdamNet_Enabled (void);

/* Accept a pending fujinet connection if needed; returns 1 if a peer is
   currently connected, 0 otherwise. Cheap to call often. */
int  AdamNet_Connected (void);

/* Block (up to timeout_ms) until fujinet-pc connects, so the ADAM's first boot
   scan already sees the FujiNet drive. Returns 1 if connected, 0 on timeout. */
int  AdamNet_WaitForConnection (int timeout_ms);

/* 1 if DCB operations for this device id should be forwarded to fujinet
   rather than handled by ADAMEm's built-in emulation. */
int  AdamNet_IsForwarded (int dev_id);

/* --- AdamNet master transactions. Return 0 on success, -1 on error. --- */

/* Request a block device's status byte (low byte of the status packet). */
int  AdamNet_DiskStatus (int dev, unsigned char *status_byte);

/* Read a 1024-byte block from a block device into buf. */
int  AdamNet_ReadBlock (int dev, unsigned long block, unsigned char *buf /* 1024 */);

/* Non-blocking block read so the Z80 (and thus audio/VDP IRQs) keeps running
   while a slow (TNFS) read completes, instead of freezing the emulator.
   Begin kicks off the read; Ready is polled until it returns non-zero:
     1  = done, buf filled
     0  = still seeking (call again later)
    -1  = error/timeout
   Only one read may be in flight at a time. */
int  AdamNet_ReadBlockBegin (int dev, unsigned long block);
int  AdamNet_ReadBlockReady (int dev, unsigned char *buf /* 1024 */);

/* Write a 1024-byte block to a block device. */
int  AdamNet_WriteBlock (int dev, unsigned long block, const unsigned char *buf /* 1024 */);

/* --- Character device (Fuji gateway / network / printer) transactions. ---
   EXPERIMENTAL: the DCB-op -> wire mapping for char devices is not yet
   validated against EOS behaviour; disk + status are the proven path. */

/* Request a char device's status; returns status byte and pending length. */
int  AdamNet_CharStatus (int dev, unsigned char *status_byte, unsigned *pending_len);

/* Send len bytes to a char device (MN_SEND); waits for ACK. */
int  AdamNet_CharWrite (int dev, const unsigned char *buf, int len);

/* Pull a pending response buffer from a char device (MN_CLR). Stores up to
   maxlen bytes in buf and the actual length in *got. */
int  AdamNet_CharRead (int dev, unsigned char *buf, int maxlen, int *got);

/* Reset device (dev 0-15), or 0xFF to reset all forwarded devices. */
void AdamNet_ResetDevice (int dev);

#endif /* ADAMNET_BRIDGE_H */
