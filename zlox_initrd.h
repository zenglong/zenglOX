// zlox_initrd.h -- Defines the interface and structures relating to the initial ramdisk.

#ifndef _ZLOX_INITRD_H_
#define _ZLOX_INITRD_H_

#include "zlox_common.h"
#include "zlox_fs.h"

typedef struct _ZLOX_INITRD_HEADER
{
	ZLOX_UINT32 nfiles; // The number of files in the ramdisk.
}  ZLOX_INITRD_HEADER;

typedef struct _ZLOX_INITRD_FILE_HEADER
{
	ZLOX_UINT8 magic; // Magic number, for error checking.
	ZLOX_CHAR name[64]; // Filename.
	ZLOX_UINT32 offset; // Offset in the initrd that the file starts.
	ZLOX_UINT32 length; // Length of the file.
} ZLOX_INITRD_FILE_HEADER;

ZLOX_FS_NODE * zlox_initialise_initrd(ZLOX_UINT32 location);

ZLOX_VOID zlox_klog_init();

ZLOX_VOID zlox_klog_write(ZLOX_CHAR ch);

#endif //_ZLOX_INITRD_H_

