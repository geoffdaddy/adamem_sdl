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

#include <sys/stat.h>

char SD_folder[512] = {""};

void get_homedir(char *dir)
{
#ifdef _WIN32
    snprintf(dir, MAX_PATH, "%s%s", getenv("HOMEDRIVE"), getenv("HOMEPATH"));
#else
    snprintf(dir, MAX_PATH, "%s", getenv("HOME"));
#endif
}


char *strupper(char *s)
{
    char *tmp = s;

    for (; *tmp; ++tmp)
    {
        *tmp = toupper((unsigned char)*tmp);
    }

    return s;
}


void UpdateFujiAppOpenKey(int mode, int device_id, unsigned dcb)
{
    unsigned int z80_addr;
    struct stat sb;
    byte *p;
    DCB *Dcb;
    int dcb_print = 0;

    Dcb = (DCB *)&RAM[dcb];

    switch (Dcb->status)
    {
    case DCB_CMD_STATUS: // 0x01
        if (fujinet_verbose)
            printf("FUJIAPPKEYOPEN: STATUS\n");
        break;
    case DCB_CMD_SOFT_RESET: // 0x02
        if (fujinet_verbose)
            printf("FUJIAPPKEYOPEN: SOFT RESET\n");
        break;
    case DCB_CMD_WRITE: // 0x03
        if (fujinet_verbose)
            printf("FUJIAPPKEYOPEN: WRITE\n");
        dcb_print = 1;
        break;
    case DCB_CMD_READ: // 0x04
        if (fujinet_verbose)
            printf("FUJIAPPKEYOPEN: READ\n");
        dcb_print = 1;
        break;
    default:
        break;
    }

    if (dcb_print)
        DisplayDCB("<<<<<<< FUJIAPPKEYOPEN >>>>>>", mode, device_id, dcb);

    // Acknowledge all messages
    Dcb->status = ACK;
}

void UpdateFujiAppReadKey(int mode, int device_id, unsigned dcb)
{
    unsigned int z80_addr;
    struct stat sb;
    int dcb_print = 0;
    DCB *Dcb;
    char homedir[512];
    char appkey_filename[512];
    FUJI_APP_DATA fad;
    char *p;


    Dcb = (DCB *)&RAM[dcb];

    switch (Dcb->status)
    {
    case DCB_CMD_STATUS: // 0x01
        if (fujinet_verbose)
            printf("FUJIAPPKEYREAD: STATUS\n");
        break;
    case DCB_CMD_SOFT_RESET: // 0x02
        if (fujinet_verbose)
            printf("FUJIAPPKEYREAD: SOFT RESET\n");
        break;
    case DCB_CMD_WRITE: // 0x03
        if (fujinet_verbose)
            printf("FUJIAPPKEYREAD: WRITE\n");
        z80_addr = Z80AddrFromDCBBuffer(dcb);

        p = (byte *)&fad;
        for (int i = 0; i < 2; i++)
        {
            *p = RAM[z80_addr + i];
            p++;
        }

        if (strcmp(SD_folder, "") == 0)
        {
            get_homedir(homedir);
            sprintf(SD_folder, "%s/%s", homedir, FUJINET_SD_DIR);
        }

        if (!(stat(SD_folder, &sb) == 0 && S_ISDIR(sb.st_mode)))
        {
            if (fujinet_verbose)
                printf("FUJIAPPKEYREAD: Directory '%s' didn't exist -- creating...\n", SD_folder);
            mkdir(SD_folder, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        }

        sprintf(appkey_filename, "%s%04hx%02hhx%02hhx", FUJINET_SD_DIR, fad.creator, fad.app, fad.key);
        strupper(appkey_filename);
        strcat(appkey_filename, ".key");
        if (fujinet_verbose)
            printf("FUJIAPPKEYREAD: Write filename '%s'\n", appkey_filename);
        dcb_print = 1;
        break;
    case DCB_CMD_READ: // 0x04
        if (fujinet_verbose)
            printf("FUJIAPPKEYREAD: READ\n");
        z80_addr = Z80AddrFromDCBBuffer(dcb);

        p = (byte *)&fad;
        for (int i = 0; i < Dcb->len; i++)
        {
            *p = RAM[z80_addr + i];
            p++;
        }

        if (strcmp(SD_folder, "") == 0)
        {
            get_homedir(homedir);
            sprintf(SD_folder, "%s/%s", homedir, FUJINET_SD_DIR);
        }

        if (!(stat(SD_folder, &sb) == 0 && S_ISDIR(sb.st_mode)))
        {
            if (fujinet_verbose)
                printf("FUJIAPPKEYREAD: Directory '%s' didn't exist -- creating...\n", SD_folder);
            mkdir(SD_folder, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        }

        sprintf(appkey_filename, "%s%04hx%02hhx%02hhx", FUJINET_SD_DIR, fad.creator, fad.app, fad.key);
        strupper(appkey_filename);
        strcat(appkey_filename, ".key");
        if (fujinet_verbose)
            printf("FUJIAPPKEYREAD: Read filename '%s'\n", appkey_filename);

        dcb_print = 1;
        break;
    default:
        break;
    }

    if (dcb_print)
        DisplayDCB("<<<<<<< FUJIAPPKEYREAD >>>>>>", mode, device_id, dcb);

    // Acknowledge all messages
    Dcb->status = ACK;
}

void UpdateFujiAppWriteKey(int mode, int device_id, unsigned dcb)
{
    unsigned int z80_addr;
    byte *p;
    DCB *Dcb;
    char homedir[512];
    char appkey_filename[512];
    int dcb_print = 0;

    Dcb = (DCB *)&RAM[dcb];

    switch (Dcb->status)
    {
    case DCB_CMD_STATUS: // 0x01
        if (fujinet_verbose)
            printf("FUJIAPPKEYWRITE: STATUS\n");
        break;
    case DCB_CMD_SOFT_RESET: // 0x02
        if (fujinet_verbose)
            printf("FUJIAPPKEYWRITE: SOFT RESET\n");
        break;
    case DCB_CMD_WRITE: // 0x03
        if (fujinet_verbose)
            printf("FUJIAPPKEYWRITE: WRITE\n");

        if (strcmp(SD_folder, "") == 0)
        {
            get_homedir(homedir);
            sprintf(SD_folder, "%s/%s", homedir, FUJINET_SD_DIR);
        }

        dcb_print = 1;
        break;
    case DCB_CMD_READ: // 0x04
        if (fujinet_verbose)
            printf("FUJIAPPKEYWRITE: READ\n");
        z80_addr = Z80AddrFromDCBBuffer(dcb);

        /*
        p = (byte *)&ft;
        for (int i = 0; i < Dcb->len; i++)
        {
            RAM[z80_addr + i] = *p;
            p++;
        }
        */
        dcb_print = 1;
        break;
    default:
        break;
    }

    if (dcb_print)
        DisplayDCB("<<<<<<< FUJIAPPKEYWRITE >>>>>>", mode, device_id, dcb);

    // Acknowledge all messages
    Dcb->status = ACK;
}

void UpdateFujiAppCloseKey(int mode, int device_id, unsigned dcb)
{
    unsigned int z80_addr;
    DCB *Dcb;
    int dcb_print = 0;

    Dcb = (DCB *)&RAM[dcb];

    switch (Dcb->status)
    {
    case DCB_CMD_STATUS: // 0x01
        if (fujinet_verbose)
            printf("FUJIAPPKEYCLOSE: STATUS\n");
        break;
    case DCB_CMD_SOFT_RESET: // 0x02
        if (fujinet_verbose)
            printf("FUJIAPPKEYCLOSE: SOFT RESET\n");
        break;
    case DCB_CMD_WRITE: // 0x03
        if (fujinet_verbose)
            printf("FUJIAPPKEYCLOSE: WRITE\n");
        dcb_print = 1;
        break;
    case DCB_CMD_READ: // 0x04
        if (fujinet_verbose)
            printf("FUJIAPPKEYCLOSE: READ\n");
        z80_addr = Z80AddrFromDCBBuffer(dcb);
        dcb_print = 1;
        break;
    default:
        break;
    }

    if (dcb_print)
        DisplayDCB("<<<<<<< FUJIAPPKEYCLOSE >>>>>>", mode, device_id, dcb);

    // Acknowledge all messages
    Dcb->status = ACK;
}