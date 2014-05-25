/*zlox_ata.h the ata header*/

#ifndef _ZLOX_ATA_H_
#define _ZLOX_ATA_H_

#include "zlox_common.h"

/* The default and seemingly universal sector size for CD-ROMs. */
#define ZLOX_ATAPI_SECTOR_SIZE 2048

/* The default and seemingly universal sector size for hard disk. */
#define ZLOX_ATA_SECTOR_SIZE 512

// read capacity size
#define ZLOX_ATAPI_CAPACITY_SIZE 8

// IDE type
#define ZLOX_IDE_ATA        0x00
#define ZLOX_IDE_ATAPI      0x01

// identification space
#define ZLOX_ATA_IDENT_DEVICETYPE   0
#define ZLOX_ATA_IDENT_CYLINDERS    2
#define ZLOX_ATA_IDENT_HEADS        6
#define ZLOX_ATA_IDENT_SECTORS      12
#define ZLOX_ATA_IDENT_SERIAL       20
#define ZLOX_ATA_IDENT_MODEL        54
#define ZLOX_ATA_IDENT_CAPABILITIES 98
#define ZLOX_ATA_IDENT_FIELDVALID   106
#define ZLOX_ATA_IDENT_MAX_LBA      120
#define ZLOX_ATA_IDENT_COMMANDSETS  164
#define ZLOX_ATA_IDENT_MAX_LBA_EXT  200

/* The necessary I/O ports, indexed by "bus". */
#define ZLOX_ATA_DATA(x)         (x)
#define ZLOX_ATA_FEATURES(x)     (x+1)
#define ZLOX_ATA_SECTOR_COUNT(x) (x+2)
#define ZLOX_ATA_ADDRESS1(x)     (x+3)
#define ZLOX_ATA_ADDRESS2(x)     (x+4)
#define ZLOX_ATA_ADDRESS3(x)     (x+5)
#define ZLOX_ATA_DRIVE_SELECT(x) (x+6)
#define ZLOX_ATA_COMMAND(x)      (x+7)
#define ZLOX_ATA_DCR(x)          (x+0x206)   /* device control register */
 
/* valid values for "bus" */
#define ZLOX_ATA_BUS_PRIMARY     0x1F0
#define ZLOX_ATA_BUS_SECONDARY   0x170
/* valid values for "drive" */
#define ZLOX_ATA_DRIVE_MASTER    0xA0
#define ZLOX_ATA_DRIVE_SLAVE     0xB0
 
/* ATA specifies a 400ns delay after drive switching -- often
 * implemented as 4 Alternative Status queries. */
#define ZLOX_ATA_SELECT_DELAY(bus) \
  {zlox_inb(ZLOX_ATA_DCR(bus));zlox_inb(ZLOX_ATA_DCR(bus));zlox_inb(ZLOX_ATA_DCR(bus));zlox_inb(ZLOX_ATA_DCR(bus));}

typedef struct _ZLOX_ATAPI_READ_CAPACITY
{
	ZLOX_UINT32 lastLBA;
	ZLOX_UINT32 blockSize;
}ZLOX_ATAPI_READ_CAPACITY;

typedef struct _ZLOX_IDE_DEVICE {
	ZLOX_UINT8  Reserved; // 0 (Empty) or 1 (This Drive really exists).
	ZLOX_UINT16 Bus; // 0x1f0 (Primary Channel) or 0x170 (Secondary Channel).
	ZLOX_UINT8  Drive; // 0xA0 (Master Drive) or 0xB0 (Slave Drive).
	ZLOX_UINT16 Type; // 0: ATA, 1:ATAPI.
	ZLOX_UINT16 Signature; // Drive Signature
	ZLOX_UINT16 Capabilities;// Features.
	ZLOX_UINT32 CommandSets; // Command Sets Supported.
	ZLOX_UINT32 Size; // Size in Sectors.
	ZLOX_UINT8  Model[41]; // Model in string.
} ZLOX_IDE_DEVICE;

/* Use the ATAPI protocol to read a single sector from the given
 * ide_index into the buffer using logical block address lba. */
ZLOX_SINT32 zlox_atapi_drive_read_sector (ZLOX_UINT32 ide_index, ZLOX_UINT32 lba, ZLOX_UINT8 *buffer);

/* read capacity from the given ide_index into the buffer */
ZLOX_SINT32 zlox_atapi_drive_read_capacity (ZLOX_UINT32 ide_index, ZLOX_UINT8 *buffer);

ZLOX_UINT32 zlox_ata_get_ide_info();

ZLOX_VOID zlox_init_ata();

#endif // _ZLOX_ATA_H_

