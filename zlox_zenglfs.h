/*zlox_zenglfs.h -- the zenglfs header*/

#ifndef _ZLOX_ZENGLFS_H_
#define _ZLOX_ZENGLFS_H_

#include "zlox_common.h"
#include "zlox_fs.h"

#define ZLOX_ZLFS_SUPER_BLOCK_SIGN 0x53464c5a // ZLFS

#define ZLOX_ZLFS_INODE_TYPE_DIRECTORY 0x4000
#define ZLOX_ZLFS_INODE_TYPE_REGULAR 0x8000

#define ZLOX_ZLFS_BLK_SIZE 1024

#define ZLOX_ZLFS_INO_MAP_SIZE 128

#define ZLOX_MBR_PT_START 0x1be

#define ZLOX_MBR_FS_TYPE_ZENGLFS 0x23
#define ZLOX_MBR_FS_TYPE_EMPTY 0x0

typedef struct _ZLOX_ZLFS_SUPER_BLOCK
{
	ZLOX_UINT32 sign;
	ZLOX_UINT32 startLBA;
	ZLOX_UINT32 TotalBlock;
	ZLOX_UINT32 TotalInode;
	ZLOX_UINT32 GroupAddr;
	ZLOX_UINT32 GroupCount;
	ZLOX_UINT32 GroupBlocks;
	ZLOX_UINT32 BlockBitMapBlockAddr;
	ZLOX_UINT32 BlockMapBlocks;
	ZLOX_UINT32 InodeBitMapBlockAddr;
	ZLOX_UINT32 InodeMapBlocks;
	ZLOX_UINT32 InodeTableBlockAddr;
	ZLOX_UINT32 allocBlocks;
	ZLOX_UINT32 allocInodes;
} ZLOX_ZLFS_SUPER_BLOCK;

typedef struct _ZLOX_MBR_PT
{
	ZLOX_UINT8 flag;
	ZLOX_UINT8 head;
	ZLOX_UINT16 sec_cyl;
	ZLOX_UINT8 fs_type;
	ZLOX_UINT8 end_head;
	ZLOX_UINT16 end_sec_cyl;
	ZLOX_UINT32 startLBA;
	ZLOX_UINT32 secNum;
} ZLOX_MBR_PT;

struct _ZLOX_INODE_DATA
{
	ZLOX_UINT16 type;
	ZLOX_UINT32 size;
	ZLOX_UINT32 totalblk;
	ZLOX_UINT32 dirent_block;
	ZLOX_UINT32 dirent_idx;
	ZLOX_UINT32 dir_inode;
	ZLOX_UINT32 dir_item_num;
	ZLOX_UINT32 blockAddr[12];
	ZLOX_UINT32 SinglyBlockAddr;
	ZLOX_UINT32 DoublyBlockAddr;
	ZLOX_UINT32 TriplyBlockAddr;
	ZLOX_UINT8 reserve[42];
} __attribute__((packed));

typedef struct _ZLOX_INODE_DATA ZLOX_INODE_DATA;

struct _ZLOX_ZLFS_DIR_ENTRY
{
	ZLOX_UINT32 inode;
	ZLOX_UINT16 type;
	ZLOX_UINT8 namelength;
	ZLOX_CHAR name[57];
} __attribute__((packed));

typedef struct _ZLOX_ZLFS_DIR_ENTRY ZLOX_ZLFS_DIR_ENTRY;

typedef struct _ZLOX_GROUP_INFO
{
	ZLOX_UINT32	allocBlocks;
	ZLOX_UINT32	allocInodes;
} ZLOX_GROUP_INFO;

ZLOX_FS_NODE * zlox_unmount_zenglfs();

// 挂载硬盘分区到hd目录下
ZLOX_FS_NODE * zlox_mount_zenglfs(ZLOX_UINT32 ide_index, ZLOX_UINT32 pt);

#endif // _ZLOX_ZENGLFS_H_

