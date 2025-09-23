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

void UpdateFujiClock(int mode, int device_id, unsigned dcb)
{
    unsigned int z80_addr;
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    char year[5];
    byte *p;
    DCB *Dcb;
    FUJI_TIME ft;
    int dcb_print = 0;

    Dcb = (DCB *)&RAM[dcb];

    switch (Dcb->status)
    {
    case DCB_CMD_STATUS: // 0x01
        if (fujinet_verbose)
            printf("FUJICLK: STATUS\n");
        break;
    case DCB_CMD_SOFT_RESET: // 0x02
        if (fujinet_verbose)
            printf("FUJICLK: SOFT RESET\n");
        break;
    case DCB_CMD_WRITE: // 0x03
        if (fujinet_verbose)
            printf("FUJICLK: WRITE\n");
        dcb_print = 1;
        break;
    case DCB_CMD_READ: // 0x04
        if (fujinet_verbose)
            printf("FUJICLK: READ\n");
        z80_addr = Z80AddrFromDCBBuffer(dcb);

        /*
        sprintf(year, "%04d", tm.tm_year + 1900);
        ft.year = atoi(&year[2]);
        year[2] = '\0';
        ft.century = atoi(year);
        ft.month = tm.tm_mon;
        ft.day = tm.tm_mday;
        ft.hour = tm.tm_hour;
        ft.minute = tm.tm_min;
        ft.second = tm.tm_sec;

        if (fujinet_verbose)
            printf("now: %d-%02d-%02d %02d:%02d:%02d\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

        Dcb->len = sizeof(ft);

        p = (byte *)&ft;
        for (int i = 0; i < Dcb->len; i++)
        {
            RAM[z80_addr + i] = *p;
            p++;
        }
        */

        fujinet_clock(&ft);
        Dcb->len = sizeof(ft);

        p = (byte *)&ft;
        for (int i = 0; i < Dcb->len; i++)
        {
            RAM[z80_addr + i] = *p;
            p++;
        }

        dcb_print = 1;
        break;
    default:
        break;
    }

    if (dcb_print)
        DisplayDCB("<<<<<<< FUJICLK >>>>>>", mode, device_id, dcb);

    // Acknowledge all messages
    Dcb->status = ACK;
}
