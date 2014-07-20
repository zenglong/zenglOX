/* fdisk.h the fdisk header */

#ifndef _FDISK_H_
#define _FDISK_H_

#include "common.h"

#define MBR_PT_START 0x1be

#define MBR_FS_TYPE_ZENGLFS 0x23
#define MBR_FS_TYPE_EMPTY	0x0

typedef struct _FDISK_PARAM
{
	BOOL setHD;
	BOOL setPT;
	BOOL setTYPE;
	BOOL setSTART;
	BOOL setNUM;
	UINT8 hd;
	UINT8 pt;
	UINT8 type;
	UINT32 start; //startLBA
	UINT32 num; //secNum
} FDISK_PARAM;

typedef struct _MBR_PT
{
	UINT8 flag;
	UINT8 head;
	UINT16 sec_cyl;
	UINT8 fs_type;
	UINT8 end_head;
	UINT16 end_sec_cyl;
	UINT32 startLBA;
	UINT32 secNum;
} MBR_PT;

#endif // _FDISK_H_

