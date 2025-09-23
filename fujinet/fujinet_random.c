/** Fujinet Device Emulation ************************************************/
/**                                                                        **/
/**                                fujinet.c                               **/
/**                                                                        **/
/** This file contains the Fujinet-specific emulation code                 **/
/**                                                                        **/
/** Copyleft Norman Davie 2024                                             **/
/**     You are free to distribute this one source file                    **/
/**     Please, notify me, if you make any changes to this file            **/
/****************************************************************************/

#include "../Coleco.h"
#include "fujinet.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>

#if defined(WIN32) || defined(MSDOS)
#if defined(IDEHD)
#include "../IDE/HarddiskIDE.h"
#endif
#else
#if defined(IDEHD)
#include "../IDE/HarddiskIDE.h"
#endif
#endif

#ifdef ZLIB
#include <zlib.h>
#endif

void UpdateFujiRandom(int mode, int device_id, unsigned dcb)
{
    static bool first_time = true;
    unsigned int z80_addr;
    short random;
    byte *p;
    DCB *Dcb;
    int dcb_print = 0;


    if (first_time)
    {
        first_time = false;
        srand(time(NULL));
    }

    Dcb = (DCB *)&RAM[dcb];

    switch (Dcb->status)
    {
    case DCB_CMD_STATUS: // 0x01
        if (fujinet_verbose)
            printf("FUJIRND: STATUS\n");
        break;
    case DCB_CMD_SOFT_RESET: // 0x02
        if (fujinet_verbose)
            printf("FUJIRND: SOFT RESET\n");
        break;
    case DCB_CMD_WRITE: // 0x03
        if (fujinet_verbose)
            printf("FUJIRND: WRITE\n");

        z80_addr = Z80AddrFromDCBBuffer(dcb);
        random = (short) rand();

        if (fujinet_verbose)
            printf("random: %d | %02x\n", random, random);

        Dcb->len = 2;

        p = (byte *)&random;
        for (int i = 0; i < 2; i++)
        {
            RAM[z80_addr + i + FUJI_CMD_DATA] = *p;
            p++;
        }

        dcb_print = 1;
        break;
    case DCB_CMD_READ: // 0x04
        if (fujinet_verbose)
            printf("FUJIRND: READ\n");
        dcb_print = 1;
        break;
    default:
        break;
    }

    if (dcb_print)
        DisplayDCB("<<<<<<< FUJIRND >>>>>>", mode, device_id, dcb);

    // Acknowledge all messages
    Dcb->status = ACK;
}
