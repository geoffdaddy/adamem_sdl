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


void UpdateFujiNetwork(int mode, int device_id, unsigned dcb)
{
    DCB *Dcb;
    int dcb_print = 0;
    char fujinet_num[20];

    sprintf(fujinet_num, "FUJINET%c", (char)('0' + device_id));

    Dcb = (DCB *)&RAM[dcb];

    switch (Dcb->status)
    {
    case DCB_CMD_STATUS:
        Dcb->status = ACK;
        break;
    case DCB_CMD_SOFT_RESET:
        Dcb->status = ACK;
        break;
    case DCB_CMD_WRITE:
        Dcb->status = ACK;
        break;
    case DCB_CMD_READ:
        Dcb->status = ACK;
        break;

    default:
        Dcb->status = ACK;
        break;
    }

    if (dcb_print)
        DisplayDCB(fujinet_num, mode, device_id, dcb);
}
