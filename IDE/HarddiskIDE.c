/*****************************************************************************
** $Source: /cvsroot/bluemsx/blueMSX/Src/IoDevice/HarddiskIDE.c,v $
**
** $Revision: 1.8 $
**
** $Date: 2006/06/15 00:26:20 $
**
** More info: http://www.bluemsx.com
**
** Copyright (C) 2003-2004 Daniel Vik
**
**  This software is provhdd 'as-is', without any express or implied
**  warranty.  In no event will the authors be held liable for any damages
**  arising from the use of this software.
**
**  Permission is granted to anyone to use this software for any purpose,
**  including commercial applications, and to alter it and redistribute it
**  freely, subject to the following restrictions:
**
**  1. The origin of this software must not be misrepresented; you must not
**     claim that you wrote the original software. If you use this software
**     in a product, an acknowledgment in the product documentation would be
**     appreciated but is not required.
**  2. Altered source versions must be plainly marked as such, and must not be
**     misrepresented as being the original software.
**  3. This notice may not be removed or altered from any source distribution.
**
******************************************************************************
** $Date: 2012/05/26 $
** Modifications by Doug Slopsema and Geoff Oltmans
**
**  Coding significantly modified to remove the idea of a hard drive and
**  replaced by the idea of a controller to handle multiple hard drives
**  that operate in the same manner as the actual hardware equivalents.
******************************************************************************
*/

#include "HarddiskIDE.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define STATUS_ERR  0x01
#define STATUS_DRQ  0x08
#define STATUS_DSC  0x10
#define STATUS_DRDY 0x40

#define MAX_HD_COUNT 6
#define MAXDRIVES (MAX_HD_COUNT)

static FILE* drives[MAXDRIVES];
enum { FD_DISK, HD_DISK } diskTypes;

struct Harddisk 
{
    int diskId;
    int fileSize;
    int tracks;
    int heads;
    int sectorsPerTrack;
    int changed;
    int diskType;
    int maxSector;
};

struct harddiskController 
{
    UInt8 hdtype;           // hdtype: 0 = IDE
    UInt8 featureReg;	    // command 1
    UInt8 sectorCountReg;   // command 2
    UInt8 sectorNumReg;     // command 3
    UInt8 cylinderLowReg;   // command 4
    UInt8 cylinderHighReg;  // command 5
    UInt8 devHeadReg;       // command 6
    UInt8 errorReg;
    UInt8 statusReg;
    UInt8 activeDisk;

    int transferRead;
    int transferWrite;
    UInt32 transferCount;
    UInt32 transferSectorNumber;

    int sectorDataOffset;
    UInt8 sectorData[512 * 256];
	
    Harddisk* hd1;
    Harddisk* hd2;
};

/* Controller routines */
harddiskController* controllerCreate(UInt8 hdtype)
{
    harddiskController* ctrl = malloc(sizeof(harddiskController));
    ctrl->hdtype             = hdtype;
    ctrl->featureReg         = 0x00;
    ctrl->sectorCountReg     = 0x01;
    ctrl->sectorNumReg       = 0x01;
    ctrl->cylinderLowReg     = 0x00;
    ctrl->cylinderHighReg    = 0x00;
    ctrl->devHeadReg         = 0x00;
    ctrl->errorReg           = 0x01;
    ctrl->statusReg          = STATUS_DSC | STATUS_DRDY;
    ctrl->transferRead       = 0;
    ctrl->transferWrite      = 0;
    ctrl->activeDisk         = 255;
    ctrl->hd1                = NULL;
    ctrl->hd2                = NULL;
    return ctrl;
}

UInt8 controllerAssign(harddiskController* ctrl, char *HardDiskFile, UInt8 value, int Verbose)
{
    switch(value) {
        case 0:
            ctrl->hd1 = harddiskCreate(0 + (2 * ctrl->hdtype));
            harddiskReset(ctrl->hd1);
            if (Verbose) printf("  Opening %s... ",HardDiskFile);
            controllerDiskChange(ctrl, ctrl->hd1, HardDiskFile);
            if (Verbose) puts((ctrl->hd1 && controllerDiskPresent(ctrl, 0 + (2 * ctrl->hdtype))) ? "OK" : "NOT FOUND");
            if (ctrl->hd1 && controllerDiskPresent(ctrl, 0 + (2 * ctrl->hdtype))) {
                ctrl->activeDisk = ctrl->hd1->diskId;
                return 0;
            } else {
                ctrl->hd1 = NULL;
                return 1;
            }
            break;
        case 1:
            ctrl->hd2 = harddiskCreate(1 + (2 * ctrl->hdtype));
            harddiskReset(ctrl->hd2);
            if (Verbose) printf("  Opening %s... ",HardDiskFile);
            controllerDiskChange(ctrl, ctrl->hd2, HardDiskFile);
            if (Verbose) puts((ctrl->hd2 && controllerDiskPresent(ctrl, 1 + (2 * ctrl->hdtype))) ? "OK" : "NOT FOUND");
            if (ctrl->hd2 && controllerDiskPresent(ctrl, 1 + (2 * ctrl->hdtype))) {
                return 0;
            } else {
                ctrl->hd2 = NULL;
                return 1;
            }
            break;
        default:
            return 1;
            break;
    }
}

void controllerDestroy(harddiskController* ctrl)
{
    if (ctrl->hd1) harddiskDestroy(ctrl->hd1);
    if (ctrl->hd2) harddiskDestroy(ctrl->hd2);
    free(ctrl);
}

UInt8 controllerDiskChange(harddiskController* ctrl, Harddisk* hd, char* fileName)
{
    struct stat s;
    int rv;

    if (hd->diskId >= MAXDRIVES)
        return 0;

    // Close previous disk image
    if(drives[hd->diskId] != NULL) { 
        fclose(drives[hd->diskId]);
        drives[hd->diskId] = NULL; 
    }

    if(!fileName) {
        return 1;
    }

    rv = stat(fileName, &s);
    if (rv == 0) {
        if (s.st_mode & S_IFDIR) {
            return 0;
        }
    }

    drives[hd->diskId] = fopen(fileName, "r+b");
    if (drives[hd->diskId] == NULL) {
        return 0;
    }

    fseek(drives[hd->diskId],0,SEEK_END);
    hd->fileSize = ftell(drives[hd->diskId]);

    ctrl->activeDisk = hd->diskId;

    controllerDiskUpdateInfo(ctrl, hd);

    return 1;
}

int controllerDiskGetHeads(harddiskController* ctrl)
{
    if (ctrl->activeDisk < MAXDRIVES)
        return (ctrl->activeDisk & 0x01) == 0 ? ctrl->hd1->heads : ctrl->hd2->heads;

    return 0;
}

int controllerDiskGetSectorsPerTrack(harddiskController* ctrl)
{
    if (ctrl->activeDisk < MAXDRIVES)
        return (ctrl->activeDisk & 0x01) == 0 ? ctrl->hd1->sectorsPerTrack : ctrl->hd2->sectorsPerTrack;

    return 0;
}

int controllerDiskGetSectorOffset(harddiskController* ctrl, int sector, int head, int track, int density)
{
    int secSize;
    int offset;

    if (ctrl->activeDisk >= MAXDRIVES)
        return 0;

    secSize = 512;

    offset = sector - 1 + controllerDiskGetSectorsPerTrack(ctrl) * (track * controllerDiskGetHeads(ctrl) + head);
    offset *= secSize;
    return offset;
}

int controllerDiskGetTracks(harddiskController* ctrl)
{
    if (ctrl->activeDisk < MAXDRIVES)
        return (ctrl->activeDisk & 0x01) == 0 ? ctrl->hd1->tracks : ctrl->hd2->tracks;

    return 0;
}

UInt8 controllerDiskPresent(harddiskController* ctrl, UInt8 value)
{
    UInt8 diskId;
    UInt8 driveId;
    diskId = value & 0x01;
    driveId = diskId + (2 * ctrl->hdtype);
    
    return driveId >= 0 && driveId < MAXDRIVES && drives[driveId] != NULL;
}

void controllerDiskReadHdIdentifySector(harddiskController* ctrl, UInt8* buffer)
{
    UInt32 totalSectors;
    UInt16 heads;
    UInt16 sectors;
    UInt16 cylinders;

    UInt8 buf[512];
    int sectorSize;
    int rv;
    rv = controllerDiskReadSector(ctrl, buf, 1, 0, 0, 512, &sectorSize);
    if (!rv) {
        return;
    }

    cylinders = (buf[3] | buf[4] << 8);					// Adam HD spec - byte 04/03 contains high/low byte of cylinders
    sectors = ((buf[6] == 0) ? 1 : buf[6]);				// Adam HD spec - byte 06 contains sectors / track
    heads = (buf[7] / sectors);						    // Adam HD spec - byte 07 divided by 06 contains heads
    if ((ctrl->activeDisk & 0x02) == 2)                 // Adam MFM HD fix
        sectors *= 2;
    totalSectors = cylinders * heads * sectors;
    printf("diskReadHdIndentifySector %d\n",ctrl->activeDisk);
}

UInt8 controllerDiskReadSector(harddiskController* ctrl, UInt8* buffer, int sector, int head, int track, int density, int *sectorSize)
{
    int secSize;
    int offset;

    if (!controllerDiskPresent(ctrl, ctrl->activeDisk))
        return 0;

    if (((ctrl->activeDisk & 0x01) == 0 ? ctrl->hd1->diskType : ctrl->hd2->diskType) == HD_DISK && sector == -1) {
        controllerDiskReadHdIdentifySector(ctrl, buffer);
        return 1;
    }

    offset = controllerDiskGetSectorOffset(ctrl, sector, head, track, density);
    secSize = 512;

    if ((drives[ctrl->activeDisk] != NULL)) {
        if (0 == fseek(drives[ctrl->activeDisk], offset, SEEK_SET)) {
            UInt8 success = fread(buffer, 1, secSize, drives[ctrl->activeDisk]) == secSize;
            return success;
        }
    }
    return 0;
}

void controllerDiskUpdateInfo(harddiskController* ctrl, Harddisk* hd) 
{
    UInt8 buf[512];
    int secSize;
    int rv;

    hd->tracks          = 80;
    hd->heads           = 2;
    hd->sectorsPerTrack = 9;
    hd->changed         = 1;
    hd->diskType        = FD_DISK;
    hd->maxSector       = 2 * 9 * 81;
    
    if (hd->fileSize > 2 * 1024 * 1024) {
        // HD image
        rv = controllerDiskReadSector(ctrl, buf, 1, 0, 0, 512, &secSize);
        if (!rv) {
            return;
        }
        hd->tracks          = (buf[3] | buf[4] << 8); 				// Adam HD spec - byte 04/03 contains high/low byte of cylinders
        hd->sectorsPerTrack = ((buf[6] == 0) ? 1 : buf[6]);		    // Adam HD spec - byte 06 contains sectors / track
        hd->heads           = (buf[7] / hd->sectorsPerTrack);	    // Adam HD spec - byte 07 divided by 06 contains heads
        if ((hd->diskId & 0x02) == 2)                               // Adam MFM HD fix
            hd->sectorsPerTrack *= 2;
        hd->changed         = 1;
        hd->diskType        = HD_DISK;
        hd->maxSector       = hd->tracks * hd->heads * hd->sectorsPerTrack;
        return;
    }
}

UInt8 controllerDiskWriteSector(harddiskController* ctrl, UInt8 *buffer, int sector, int side, int track, int density)
{
    int secSize;
    int offset;

    if (!controllerDiskPresent(ctrl, ctrl->activeDisk))
        return 0;

    if (sector >= ((ctrl->activeDisk & 0x01) == 0 ? ctrl->hd1->maxSector : ctrl->hd2->maxSector))
        return 0;

    if (density == 0) {
        density = 512;
    }

    offset = controllerDiskGetSectorOffset(ctrl, sector, side, track, density);
    secSize = 512;

    if (drives[ctrl->activeDisk] != NULL) {
        if (0 == fseek(drives[ctrl->activeDisk], offset, SEEK_SET)) {
            UInt8 success = fwrite(buffer, 1, secSize, drives[ctrl->activeDisk]) == secSize;
            return success;
        }
    }
    return 0;
}

UInt32 controllerGetNumSectors(harddiskController* ctrl)
{
    return ctrl->sectorCountReg == 0 ? 256 : ctrl->sectorCountReg;
}

UInt32 controllerGetSectorNumber(harddiskController* ctrl)
{
    UInt32 temp;
    temp = ctrl->sectorNumReg * (ctrl->cylinderLowReg | ctrl->cylinderHighReg << 8) * ((ctrl->devHeadReg & 0x0f) + 1);
    return temp;
}

void controllerSetError(harddiskController* ctrl, UInt8 error)
{
    ctrl->errorReg = error;
    ctrl->statusReg |= STATUS_ERR;
    ctrl->statusReg &= ~STATUS_DRQ;
    ctrl->transferWrite = 0;
    ctrl->transferRead = 0;
}

void controllerExecuteCommand(harddiskController* ctrl, UInt8 cmd)
{
    ctrl->statusReg &= ~(STATUS_DRQ | STATUS_ERR);
    ctrl->transferRead = 0;
    ctrl->transferWrite = 0;
    switch (cmd) {
/*
        case 0xf8: { 
            // 248
            UInt32 sectorCount = diskGetSectorsPerTrack(ctrl);
            #ifdef DEBUG_IO
            printf("Cmd %02Xh, HD %d, Cylinder %04d, Head %d, Sector %02d Count %d\n",cmd,ctrl->activeDisk,(ctrl->cylinderLowReg | ctrl->cylinderHighReg << 8),ctrl->devHeadReg,ctrl->sectorNumReg,ctrl->sectorCountReg);
            #endif
            ctrl->sectorNumReg    = (UInt8)((sectorCount >>  0) & 0xff);
            ctrl->cylinderLowReg  = (UInt8)((sectorCount >>  8) & 0xff);
            ctrl->cylinderHighReg = (UInt8)((sectorCount >> 16) & 0xff);
            ctrl->devHeadReg      = (UInt8)((sectorCount >> 24) & 0x0f);
            break;
        }
*/
        case 0xef: { 
            // 239 Set Feature
            #ifdef DEBUG_IO
            printf("Cmd %02Xh, HD %d, Cylinder %04d, Head %d, Sector %02d Count %d\n",cmd,ctrl->activeDisk,(ctrl->cylinderLowReg | ctrl->cylinderHighReg << 8),ctrl->devHeadReg,ctrl->sectorNumReg,ctrl->sectorCountReg);
            #endif
            if (ctrl->featureReg != 0x03) {
                controllerSetError(ctrl, 0x04);
            }
            break;
        }
        case 0xec: { 
            // 236 ATA Identify Device
            #ifdef DEBUG_IO
            printf("Cmd %02Xh, HD %d, Cylinder %04d, Head %d, Sector %02d Count %d\n",cmd,ctrl->activeDisk,(ctrl->cylinderLowReg | ctrl->cylinderHighReg << 8),ctrl->devHeadReg,ctrl->sectorNumReg,ctrl->sectorCountReg);
            #endif
            if (!controllerDiskReadSector(ctrl, ctrl->sectorData, -1, 0, 0, 0, NULL)) {
                controllerSetError(ctrl, 0x44);
                break;
            }
            ctrl->transferCount = 512/2;
            ctrl->sectorDataOffset = 0;
            ctrl->transferRead = 1;
            ctrl->statusReg |= STATUS_DRQ;
            break;
        }
        case 0x91: { 
            // 145 Initialize Drive Parameters
            #ifdef DEBUG_IO
            printf("Initialize (%02Xh), HD %d, Cylinder %04d, Head %d, Sector %02d Count %d\n",cmd,ctrl->activeDisk,(ctrl->cylinderLowReg | ctrl->cylinderHighReg << 8),ctrl->devHeadReg,ctrl->sectorNumReg,ctrl->sectorCountReg);
            #endif
            break;
        }
        case 0x70: case 0x71: case 0x77: case 0x7F: { 
            // HD Seek - IDE (112), MFM (113, 119, 127)
            #ifdef DEBUG_IO
            printf("Seek HD    (%02Xh), HD %d, Cylinder %04d, Head %d, Sector %02d Count %d\n",cmd,ctrl->activeDisk,(ctrl->cylinderLowReg | ctrl->cylinderHighReg << 8),ctrl->devHeadReg,ctrl->sectorNumReg,ctrl->sectorCountReg);
            #endif
            break;
        }
        case 0x30: { 
            // 48 Write Sector with Retry
            int sectorNumber = controllerGetSectorNumber(ctrl);
            int numSectors = controllerGetNumSectors(ctrl);
            #ifdef DEBUG_IO
            printf("Write HD   (%02Xh), HD %d, Cylinder %04d, Head %d, Sector %02d Count %d\n",cmd,ctrl->activeDisk,(ctrl->cylinderLowReg | ctrl->cylinderHighReg << 8),ctrl->devHeadReg,ctrl->sectorNumReg,ctrl->sectorCountReg);
            #endif
            if ((sectorNumber + numSectors) > ((ctrl->activeDisk & 0x01) == 0 ? ctrl->hd1->maxSector : ctrl->hd2->maxSector)) {
                controllerSetError(ctrl, 0x14);
                break;
            }
            ctrl->transferSectorNumber = sectorNumber;
            ctrl->transferCount = 512/2 * numSectors;
            ctrl->sectorDataOffset = 0;
            ctrl->transferWrite = 1;
            ctrl->statusReg |= STATUS_DRQ;
            break;
        }
        case 0x20: { 
            // 32 Read Sector with Retry
            int sectorNumber = controllerGetSectorNumber(ctrl);
            int numSectors = controllerGetNumSectors(ctrl);
            int i;
    
            #ifdef DEBUG_IO
            printf("Read HD    (%02Xh), HD %d, Cylinder %04d, Head %d, Sector %02d Count %d\n",cmd,ctrl->activeDisk,(ctrl->cylinderLowReg | ctrl->cylinderHighReg << 8),ctrl->devHeadReg,ctrl->sectorNumReg,ctrl->sectorCountReg);
            #endif
            if ((sectorNumber + numSectors) > ((ctrl->activeDisk & 0x01) == 0 ? ctrl->hd1->maxSector : ctrl->hd2->maxSector)) {
                controllerSetError(ctrl, 0x14);
                break;
            }
            if ((ctrl->devHeadReg > 15) || (ctrl->devHeadReg > controllerDiskGetHeads(ctrl))) {
                controllerSetError(ctrl, 0x14);
                break;
            }
            if ((ctrl->cylinderLowReg | ctrl->cylinderHighReg << 8) > controllerDiskGetTracks(ctrl)) {
                controllerSetError(ctrl, 0x14);
                break;
            }
            for (i = 0; i < numSectors; i++) {
                if (!controllerDiskReadSector(ctrl, ctrl->sectorData + i * 512, ctrl->sectorNumReg, ctrl->devHeadReg, (ctrl->cylinderLowReg | ctrl->cylinderHighReg << 8), 0, NULL)) {
                    break;
                }
                controllerNextSector(ctrl);
            }
            if (i != numSectors) {
                controllerSetError(ctrl, 0x44);
                break;
            }

            ctrl->transferCount = 512/2 * numSectors;
            ctrl->sectorDataOffset = 0;
            ctrl->transferRead = 1;
            ctrl->statusReg |= STATUS_DRQ;
            break;
        }
        case 0x10: case 0x17: case 0x1F: {
            // Recalibrate - IDE (16), MFM (23,31)
            #ifdef DEBUG_IO
            printf("HD Parameters:\n%d Tracks, %d Heads, %d Sectors/Track\n\n",controllerDiskGetTracks(ctrl),controllerDiskGetHeads(ctrl),controllerDiskGetSectorsPerTrack(ctrl));
            printf("Calibrate  (%02Xh), HD %d, Cylinder %04d, Head %d, Sector %02d Count %d\n",cmd,ctrl->activeDisk,(ctrl->cylinderLowReg | ctrl->cylinderHighReg << 8),ctrl->devHeadReg,ctrl->sectorNumReg,ctrl->sectorCountReg);
            #endif
            break;
        }
        default: { 
            // all others
            #ifdef DEBUG_IO
            printf("Cmd %02Xh, HD %d, Cylinder %04d, Head %d, Sector %02d Count %d\n",cmd,ctrl->activeDisk,(ctrl->cylinderLowReg | ctrl->cylinderHighReg << 8),ctrl->devHeadReg,ctrl->sectorNumReg,ctrl->sectorCountReg);
            #endif
            controllerSetError(ctrl, 0x04);
        }
    }
}

UInt8 controllerReadRegister(harddiskController* ctrl, UInt8 reg)
{
    if (!controllerDiskPresent(ctrl, ctrl->activeDisk)) {
        return 0x7f;
    }

    switch (reg) {
    case 1:
        return ctrl->featureReg;
    case 2: 
        return ctrl->sectorCountReg;
    case 3: 
        return ctrl->sectorNumReg;
    case 4: 
        return ctrl->cylinderLowReg;
    case 5: 
        return ctrl->cylinderHighReg;
    case 6: 
        return ctrl->devHeadReg;
    case 7: 
        return ctrl->statusReg;
    }

    return 0x7f;
}

void controllerWriteRegister(harddiskController* ctrl, UInt8 reg, UInt8 value)
{
    //UInt8 driveNumber=0;

    if (!controllerDiskPresent(ctrl, ctrl->activeDisk)) {
        return;
    }

    switch (reg) {
    case 1:
        ctrl->featureReg = value;
        break;
    case 2:
        ctrl->sectorCountReg = value;
        break;
    case 3:
        ctrl->sectorNumReg = value;
        break;
    case 4:
        ctrl->cylinderLowReg = value;
        break;
    case 5:
        ctrl->cylinderHighReg = value;
        break;
    case 6:
        if ((value & 0xB0) == 0xA0 && ctrl->hdtype == 0) {  // change active HD file to IDE Master
            if (ctrl->hd1)
                ctrl->activeDisk = ctrl->hd1->diskId;
            value = value - 160;
        } 
        if ((value & 0xB0) == 0xB0 && ctrl->hdtype == 0) {  // change active HD file to IDE Slave
            if (ctrl->hd2)
                ctrl->activeDisk = ctrl->hd2->diskId;
            value = value - 176;
        }
        ctrl->devHeadReg = value;
        break;
    case 7:
        controllerExecuteCommand(ctrl, value);
        break;
    }
}

UInt16 controllerRead(harddiskController* ctrl)
{
    UInt16 value;

    if (!ctrl->transferRead || !controllerDiskPresent(ctrl, ctrl->activeDisk)) {
        return 0x7f7f;
    }

    value  = ctrl->sectorData[ctrl->sectorDataOffset++];
    value |= ctrl->sectorData[ctrl->sectorDataOffset++] << 8;
    if (--ctrl->transferCount == 0) {
        ctrl->transferRead = 0;
        ctrl->statusReg &= ~STATUS_DRQ;
    }
    return value;
}

UInt16 controllerPeek(harddiskController* ctrl)
{
    UInt16 value;

    if (!ctrl->transferRead || !controllerDiskPresent(ctrl, ctrl->activeDisk)) {
        return 0x7f7f;
    }

    value  = ctrl->sectorData[ctrl->sectorDataOffset];
    value |= ctrl->sectorData[ctrl->sectorDataOffset + 1] << 8;

    return value;
}

void controllerWrite(harddiskController* ctrl, UInt16 value)
{
    if (!ctrl->transferWrite || !controllerDiskPresent(ctrl, ctrl->activeDisk)) {
        if (!ctrl->transferWrite)     
            printf("!ctrl->transferWrite, %d\n",(int)ctrl->transferCount);
        if (!controllerDiskPresent(ctrl, ctrl->activeDisk)) 
            printf("!controllerDiskPresent(ctrl, hd->diskId)\n");
		return;
    }

    ctrl->sectorData[ctrl->sectorDataOffset++] = value & 0xff;
    ctrl->sectorData[ctrl->sectorDataOffset++] = value >> 8;
    ctrl->transferCount--;

    if (((ctrl->transferCount & 255) == 0) && (ctrl->hdtype == 0)) {
        if (!controllerDiskWriteSector(ctrl, ctrl->sectorData, ctrl->sectorNumReg, ctrl->devHeadReg, (ctrl->cylinderLowReg | ctrl->cylinderHighReg << 8), 0)) {
            controllerSetError(ctrl, 0x44);
            ctrl->transferWrite = 0;
            return;
        }
        controllerNextSector(ctrl);
        ctrl->sectorDataOffset = 0;
	}

    if ((ctrl->transferCount == 0) && (ctrl->hdtype == 0)) { // IDE
        ctrl->transferWrite = 0;
        ctrl->statusReg &= ~STATUS_DRQ;
    }
}

void controllerNextSector(harddiskController* ctrl)
{
    ctrl->sectorNumReg++;
    if (ctrl->sectorNumReg > controllerDiskGetSectorsPerTrack(ctrl)) {
        ctrl->sectorNumReg = ctrl->sectorNumReg - controllerDiskGetSectorsPerTrack(ctrl);
        ctrl->devHeadReg++;
    }
    if (ctrl->devHeadReg > (controllerDiskGetHeads(ctrl) - 1)) {
        ctrl->devHeadReg = ctrl->devHeadReg - (controllerDiskGetHeads(ctrl));
        if (ctrl->cylinderLowReg == 255) {
            ctrl->cylinderLowReg = ctrl->cylinderLowReg - (256 - 1);
            ctrl->cylinderHighReg++;
        }
        else {
            ctrl->cylinderLowReg++;
        }
    }
}

// Harddisk routines
Harddisk* harddiskCreate(int diskId)
{
    Harddisk* hd = malloc(sizeof(Harddisk));

    hd->diskId = diskId;

    harddiskReset(hd);

    return hd;
}

void harddiskDestroy(Harddisk* hd)
{
    free(hd);
}

void harddiskReset (Harddisk* hd)
{
}
