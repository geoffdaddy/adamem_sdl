/*****************************************************************************
** $Source: /cvsroot/bluemsx/blueMSX/Src/IoDevice/HarddiskIDE.h,v $
**
** $Revision: 1.5 $
**
** $Date: 2006/06/13 17:13:27 $
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
#ifndef HARDDISK_IDE_H
#define HARDDISK_IDE_H

#include "MsxTypes.h"

typedef struct Harddisk Harddisk;
typedef struct harddiskController harddiskController;

harddiskController* controllerCreate(UInt8 hdtype);
UInt8 controllerAssign(harddiskController* ctrl, char *HardDiskFile, UInt8 value, int Verbose);
void controllerDestroy(harddiskController* ctrl);

UInt8 controllerDiskChange(harddiskController* ctrl, Harddisk* hd, char* fileName);
UInt8 controllerDiskPresent(harddiskController* ctrl, UInt8 value);
UInt8 controllerDiskReadSector(harddiskController* ctrl, UInt8* buffer, int sector, int head, int track, int density, int *sectorSize);
void controllerDiskUpdateInfo(harddiskController* ctrl, Harddisk* hd);
UInt8 controllerDiskWriteSector(harddiskController* ctrl, UInt8 *buffer, int sector, int side, int track, int density);

UInt8 controllerReadRegister(harddiskController* ctrl, UInt8 reg);
void controllerWriteRegister(harddiskController* ctrl, UInt8 reg, UInt8 value);

UInt16 controllerRead(harddiskController* ctrl);
UInt16 controllerPeek(harddiskController* ctrl);
void controllerWrite(harddiskController* ctrl, UInt16 value);
void controllerNextSector(harddiskController* ctrl);

Harddisk* harddiskCreate(int diskId);
void harddiskDestroy(Harddisk* hd);
void harddiskReset (Harddisk* hd);

#endif