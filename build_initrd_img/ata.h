/* ata.h the ata header */

#ifndef _ATA_H_
#define _ATA_H_

#include "common.h"

/* The default and seemingly universal sector size for CD-ROMs. */
#define ATAPI_SECTOR_SIZE 2048

/* The default and seemingly universal sector size for hard disk. */
#define ATA_SECTOR_SIZE 512

// IDE type
#define IDE_ATA        0x00
#define IDE_ATAPI      0x01

#define IDE_ATA_READ	0
#define IDE_ATA_WRITE 1

typedef struct _ATAPI_READ_CAPACITY
{
	UINT32 lastLBA;
	UINT32 blockSize;
} ATAPI_READ_CAPACITY;

typedef struct _IDE_DEVICE {
	UINT8  Reserved; // 0 (Empty) or 1 (This Drive really exists).
	UINT16 Bus; // 0x1f0 (Primary Channel) or 0x170 (Secondary Channel).
	UINT8  Drive; // 0xA0 (Master Drive) or 0xB0 (Slave Drive).
	UINT16 Type; // 0: ATA, 1:ATAPI.
	UINT16 Signature; // Drive Signature
	UINT16 Capabilities;// Features.
	UINT32 CommandSets; // Command Sets Supported.
	UINT32 Size; // Size in Sectors.
	UINT8  Model[41]; // Model in string.
} IDE_DEVICE;

#endif // _ATA_H_

