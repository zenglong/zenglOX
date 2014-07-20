/* format.h the format header */

#ifndef _FORMAT_H_
#define _FORMAT_H_

#include "common.h"

#define SUPER_BLOCK_SIGN 0x53464c5a // ZLFS

#define INODE_TYPE_DIRECTORY 0x4000
#define INODE_TYPE_REGULAR 0x8000

typedef struct _FORMAT_PARAM
{
	BOOL setHD;
	BOOL setPT;
	BOOL setTYPE;
	UINT8 hd;
	UINT8 pt;
	UINT8 type;
} FORMAT_PARAM;

typedef struct _GROUP_INFO
{
	UINT32	allocBlocks;
	UINT32	allocInodes;
} GROUP_INFO;

typedef struct _SUPER_BLOCK
{
	UINT32	sign;
	UINT32 startLBA;
	UINT32 TotalBlock;
	UINT32 TotalInode;
	UINT32 GroupAddr;
	UINT32 GroupCount;
	UINT32 GroupBlocks;
	UINT32 BlockBitMapBlockAddr;
	UINT32 BlockMapBlocks;
	UINT32 InodeBitMapBlockAddr;
	UINT32 InodeMapBlocks;
	UINT32 InodeTableBlockAddr;
	UINT32 allocBlocks;
	UINT32 allocInodes;
} SUPER_BLOCK;

typedef struct _INODE_DATA
{
	UINT16	type;
	UINT32 size;
	UINT32 totalblk;
	UINT32 blockAddr[12];
	UINT32 SinglyBlockAddr;
	UINT32 DoublyBlockAddr;
	UINT32 TriplyBlockAddr;
	UINT8 reserve[58];
} INODE_DATA;

#endif // _FORMAT_H_

