// zlox_zenglfs.c -- zenglfs文件系统

/*
zenglfs 文件系统是参考ext2文件系统的，ext2文件系统可以参考 
http://wiki.osdev.org/Ext2
zenglfs去掉了ext2里的一些字段，同时调整了Group块组信息和位图信息

其结构中，第1部分是super block超级块，第2部分是Group组信息部分,
第3部分是block逻辑块的位图信息(位图里的二进制1表示已分配的逻辑块，二进制0表示未分配
的逻辑块)，第4部分是文件节点的位图信息(位图里的二进制1表示已分配的文件节点，二进制0表示
未分配的文件节点)，第5部分是文件节点数组，数组里的每一项都记录了一个文件节点的具体字段信息，
剩余的部分则都是用于存储文件或目录的内容的

文件系统里所使用的逻辑块都是1024字节的大小，相当于2个磁盘扇区的大小
super block超级块的逻辑块地址是1，0号逻辑块地址目前是预留的

另外，1号文件节点是root根目录文件节点，在之前format工具进行格式化时，会对其进行简单的设置

在mount工具挂载zenglfs文件系统时，会调用本模块里的zlox_mount_zenglfs函数来完成加载工作，
在file之类的工具对hd目录所在的zenglfs文件系统进行写目录操作时(比如创建子目录，或创建目录里的文件时)，
会调用本模块的zlox_zenglfs_writedir函数，
在file之类的工具对hd目录进行读写文件的操作时，
会调用本模块的zlox_zenglfs_read或zlox_zenglfs_write函数，
当对hd目录进行列表或查找文件及子目录的操作时, 
会调用本模块的zlox_zenglfs_readdir或zlox_zenglfs_finddir_ext函数，
当对hd目录进行删除文件或删除子目录的操作时，
会调用本模块的zlox_zenglfs_remove函数，
当对hd目录里的子文件或子目录进行重命名操作时，
会调用本模块的zlox_zenglfs_rename函数，
在unmount工具卸载zenglfs文件系统时，则会调用本模块的zlox_unmount_zenglfs函数。

在本模块的开头定义了类似zenglfs_cache_inode, zenglfs_cache_inode_blk,
zenglfs_cache_group之类的缓存结构，定义这些结构是为了能缓存磁盘里的数据，
以减少频繁的磁盘I/O读写操作

在文件节点里有SinglyBlockAddr，DoublyBlockAddr，TriplyBlockAddr字段，
这些字段是对blockAddr字段的补充，这些字段参考自ext2文件系统，
blockAddr[12]字段只能分配到12个逻辑块数据(相当于12KByte)，
SinglyBlockAddr对应的逻辑块里可以存储256个blockAddr(相当于256k的数据)，
DoublyBlockAddr的逻辑块里则可以存储256个SinglyBlock的逻辑块地址，这样就可以分配到
256 * 256k = 65536k = 64M的数据，
TriplyBlockAddr对应的逻辑块里可以存储256个DoublyBlock的逻辑块地址，这样就可以分配到
256 * 256 * 256k = 16777216k = 16384M = 16G的数据
因此，理论上，这种结构的单个文件的最大尺寸是16G多，
当然还要看具体的读写操作的算法，目前的读写算法是先将要写入的数据全部缓存到内存，
再将内存里的数据写入磁盘，因此，zenglfs目前可写入文件的实际大小是受限于内存大小的。
如果将读写算法调整为每次只写入一部分数据，分批将数据写入到磁盘的话，就可以让文件
达到理论上的最大尺寸。

*/

#include "zlox_kheap.h"
#include "zlox_zenglfs.h"
#include "zlox_ata.h"
#include "zlox_monitor.h"

static ZLOX_FS_NODE * zlox_zenglfs_finddir(ZLOX_FS_NODE *node, ZLOX_CHAR *name);

ZLOX_FS_NODE * zenglfs_root = ZLOX_NULL;
ZLOX_SINT32 zenglfs_ide_index = -1;
ZLOX_ZLFS_SUPER_BLOCK zenglfs_super_block = {0};
ZLOX_BOOL zenglfs_super_block_isDirty = ZLOX_FALSE;
ZLOX_UINT32 zenglfs_super_block_lba = 0;
ZLOX_FS_NODE zenglfs_fsnode;
ZLOX_DIRENT zenglfs_dirent;
ZLOX_CHAR zenglfs_tmp_name[128];

typedef struct _ZLOX_ZLFS_CACHE_INODE
{
	ZLOX_BOOL isDirty;
	ZLOX_INODE_DATA data;
	ZLOX_UINT32 ino;
} ZLOX_ZLFS_CACHE_INODE;

ZLOX_ZLFS_CACHE_INODE zenglfs_cache_inode = {0};

typedef struct _ZLOX_ZLFS_CACHE_INODE_BLK
{
	ZLOX_BOOL isGetData;
	ZLOX_INODE_DATA * ptr;
	ZLOX_UINT32 blk;
} ZLOX_ZLFS_CACHE_INODE_BLK;

ZLOX_ZLFS_CACHE_INODE_BLK zenglfs_cache_inode_blk = {0};

typedef struct _ZLOX_ZLFS_CACHE_GROUP
{
	ZLOX_BOOL isGetData;
	ZLOX_BOOL isDirty;
	ZLOX_GROUP_INFO * ptr;
	ZLOX_UINT32 idx;
} ZLOX_ZLFS_CACHE_GROUP;

ZLOX_ZLFS_CACHE_GROUP zenglfs_cache_group = {0};

typedef struct _ZLOX_ZLFS_CACHE_BLKMAP
{
	ZLOX_BOOL isGetData;
	ZLOX_BOOL isDirty;
	ZLOX_UINT32 * ptr;
	ZLOX_UINT32 idx;
} ZLOX_ZLFS_CACHE_BLKMAP;

ZLOX_ZLFS_CACHE_BLKMAP zenglfs_cache_blkmap = {0};

typedef struct _ZLOX_ZLFS_CACHE_INODEMAP
{
	ZLOX_BOOL isGetData;
	ZLOX_BOOL isDirty;
	ZLOX_UINT32 * ptr;
	ZLOX_UINT32 blk_idx;
	ZLOX_UINT32 ino_idx;
} ZLOX_ZLFS_CACHE_INODEMAP;

ZLOX_ZLFS_CACHE_INODEMAP zenglfs_cache_inode_map = {0};

typedef struct _ZLOX_ZLFS_CACHE_SINGLY
{
	ZLOX_BOOL isGetData;
	ZLOX_BOOL isDirty;
	ZLOX_UINT32 * ptr;
	ZLOX_UINT32 blockaddr;
} ZLOX_ZLFS_CACHE_SINGLY;

ZLOX_ZLFS_CACHE_SINGLY zenglfs_cache_singly = {0};

typedef struct _ZLOX_ZLFS_CACHE_DOUBLY
{
	ZLOX_BOOL isGetData;
	ZLOX_BOOL isDirty;
	ZLOX_UINT32 * ptr;
	ZLOX_UINT32 blockaddr;
} ZLOX_ZLFS_CACHE_DOUBLY;

ZLOX_ZLFS_CACHE_DOUBLY zenglfs_cache_doubly = {0};

typedef struct _ZLOX_ZLFS_CACHE_TRIPLY
{
	ZLOX_BOOL isGetData;
	ZLOX_BOOL isDirty;
	ZLOX_UINT32 * ptr;
	ZLOX_UINT32 blockaddr;
} ZLOX_ZLFS_CACHE_TRIPLY;

ZLOX_ZLFS_CACHE_TRIPLY zenglfs_cache_triply = {0};

// Macros used in the bitset algorithms.
#define ZLOX_INDEX_FROM_BIT(a) (a/(8*4))
#define ZLOX_OFFSET_FROM_BIT(a) (a%(8*4))

// Static function to set a bit in the bitset
static ZLOX_VOID zlox_zenglfs_set_bitmap(ZLOX_UINT32 * blocksmap, ZLOX_UINT32 block)
{
	ZLOX_UINT32 idx = ZLOX_INDEX_FROM_BIT(block);
	ZLOX_UINT32 off = ZLOX_OFFSET_FROM_BIT(block);
	blocksmap[idx] |= (0x1 << off);
}

static ZLOX_VOID zlox_zenglfs_clear_bitmap(ZLOX_UINT32 * blocksmap, ZLOX_UINT32 block)
{
	ZLOX_UINT32 idx = ZLOX_INDEX_FROM_BIT(block);
	ZLOX_UINT32 off = ZLOX_OFFSET_FROM_BIT(block);
	blocksmap[idx] &= ~(0x1 << off);
}

static ZLOX_UINT32 zlox_zenglfs_first_bitmap(ZLOX_UINT32 * blocksmap, ZLOX_UINT32 nbits)
{
	ZLOX_UINT32 i, j;
	for (i = 0; i < ZLOX_INDEX_FROM_BIT(nbits); i++)
	{
		if (blocksmap[i] != 0xFFFFFFFF) // nothing free, exit early.
		{
			// at least one bit is free here.
			for (j = 0; j < 32; j++)
			{
				ZLOX_UINT32 toTest = 0x1 << j;
				if ( !(blocksmap[i]&toTest) )
				{
					return i*4*8+j;
				}
			}
		}
	}
	return 0xFFFFFFFF;
}

static ZLOX_VOID zlox_zenglfs_get_inode(ZLOX_UINT32 inode)
{
	if(zenglfs_cache_inode_blk.ptr == ZLOX_NULL)
	{
		zenglfs_cache_inode_blk.ptr = (ZLOX_INODE_DATA *)zlox_kmalloc(ZLOX_ZLFS_BLK_SIZE);
		zenglfs_cache_inode_blk.blk = 0;
		zenglfs_cache_inode_blk.isGetData = ZLOX_FALSE;
	}
	if(zenglfs_cache_inode.ino != inode || zenglfs_cache_inode_blk.isGetData == ZLOX_FALSE)
	{
		ZLOX_UINT32 i_per_blk = ZLOX_ZLFS_BLK_SIZE / sizeof(ZLOX_INODE_DATA);
		ZLOX_UINT32 newblk;
		ZLOX_UINT32 idx, newidx;
		ZLOX_UINT32 lba, newlba;
		newblk = (inode - 1) / i_per_blk;
		newidx = (inode - 1) % i_per_blk;
		if(zenglfs_cache_inode.isDirty)
		{
			idx = (zenglfs_cache_inode.ino - 1) % i_per_blk;
			lba = zenglfs_super_block.startLBA + (zenglfs_super_block.InodeTableBlockAddr + 
									zenglfs_cache_inode_blk.blk) * 2;
			zenglfs_cache_inode_blk.ptr[idx] = zenglfs_cache_inode.data;
			if(newblk != zenglfs_cache_inode_blk.blk)
				zlox_ide_ata_access(ZLOX_IDE_ATA_WRITE, zenglfs_ide_index, lba, 2, 
								(ZLOX_UINT8 *)zenglfs_cache_inode_blk.ptr);
		}
		if(zenglfs_cache_inode_blk.isGetData == ZLOX_FALSE || 
			newblk != zenglfs_cache_inode_blk.blk)
		{
			newlba = zenglfs_super_block.startLBA + (zenglfs_super_block.InodeTableBlockAddr + newblk) * 2;		
			zlox_ide_ata_access(ZLOX_IDE_ATA_READ, zenglfs_ide_index, newlba, 2, 
							(ZLOX_UINT8 *)zenglfs_cache_inode_blk.ptr);
			zenglfs_cache_inode_blk.isGetData = ZLOX_TRUE;
			zenglfs_cache_inode_blk.blk = newblk;
		}
		zenglfs_cache_inode.data = zenglfs_cache_inode_blk.ptr[newidx];
		zenglfs_cache_inode.ino = inode;
		zenglfs_cache_inode.isDirty = ZLOX_FALSE;
	}
}

static ZLOX_VOID zlox_zenglfs_get_group(ZLOX_UINT32 idx)
{
	if(zenglfs_cache_group.ptr == ZLOX_NULL)
	{
		zenglfs_cache_group.ptr = (ZLOX_GROUP_INFO *)zlox_kmalloc(ZLOX_ZLFS_BLK_SIZE);
		zenglfs_cache_group.idx = 0;
	}
	if(zenglfs_cache_group.idx != idx || zenglfs_cache_group.isGetData == ZLOX_FALSE)
	{
		ZLOX_UINT32 lba;
		if(zenglfs_cache_group.isDirty)
		{
			lba = zenglfs_super_block.startLBA + 
					(zenglfs_super_block.GroupAddr + zenglfs_cache_group.idx) * 2;
			zlox_ide_ata_access(ZLOX_IDE_ATA_WRITE, zenglfs_ide_index, lba, 2, (ZLOX_UINT8 *)zenglfs_cache_group.ptr);
		}
		lba = zenglfs_super_block.startLBA + (zenglfs_super_block.GroupAddr + idx) * 2;
		zlox_ide_ata_access(ZLOX_IDE_ATA_READ, zenglfs_ide_index, lba, 2, (ZLOX_UINT8 *)zenglfs_cache_group.ptr);
		zenglfs_cache_group.idx = idx;
		zenglfs_cache_group.isGetData = ZLOX_TRUE;
		zenglfs_cache_group.isDirty = ZLOX_FALSE;
	}
}

static ZLOX_VOID zlox_zenglfs_get_blkmap(ZLOX_UINT32 idx)
{
	if(zenglfs_cache_blkmap.ptr == ZLOX_NULL)
	{
		zenglfs_cache_blkmap.ptr = (ZLOX_UINT32 *)zlox_kmalloc(ZLOX_ZLFS_BLK_SIZE);
		zenglfs_cache_blkmap.idx = 0;
	}
	if(zenglfs_cache_blkmap.idx != idx || zenglfs_cache_blkmap.isGetData == ZLOX_FALSE)
	{
		ZLOX_UINT32 lba;
		if(zenglfs_cache_blkmap.isDirty)
		{
			lba = zenglfs_super_block.startLBA + 
					(zenglfs_super_block.BlockBitMapBlockAddr + zenglfs_cache_blkmap.idx) * 2;
			zlox_ide_ata_access(ZLOX_IDE_ATA_WRITE, zenglfs_ide_index, lba, 2, (ZLOX_UINT8 *)zenglfs_cache_blkmap.ptr);
		}
		lba = zenglfs_super_block.startLBA + (zenglfs_super_block.BlockBitMapBlockAddr + idx) * 2;
		zlox_ide_ata_access(ZLOX_IDE_ATA_READ, zenglfs_ide_index, lba, 2, (ZLOX_UINT8 *)zenglfs_cache_blkmap.ptr);
		zenglfs_cache_blkmap.idx = idx;
		zenglfs_cache_blkmap.isGetData = ZLOX_TRUE;
		zenglfs_cache_blkmap.isDirty = ZLOX_FALSE;
	}
}

static ZLOX_VOID zlox_zenglfs_get_inode_map(ZLOX_UINT32 ino_total_idx)
{
	if(zenglfs_cache_inode_map.ptr == ZLOX_NULL)
	{
		zenglfs_cache_inode_map.ptr = (ZLOX_UINT32 *)zlox_kmalloc(ZLOX_ZLFS_BLK_SIZE);
		zenglfs_cache_inode_map.blk_idx = 0;
		zenglfs_cache_inode_map.ino_idx = 0;
	}
	ZLOX_UINT32 imap_per_blk = ZLOX_ZLFS_BLK_SIZE / ZLOX_ZLFS_INO_MAP_SIZE;
	ZLOX_UINT32 blk_idx = ino_total_idx / imap_per_blk;
	ZLOX_UINT32 ino_idx = ino_total_idx % imap_per_blk;
	if(zenglfs_cache_inode_map.blk_idx != blk_idx || zenglfs_cache_inode_map.isGetData == ZLOX_FALSE)
	{
		ZLOX_UINT32 lba;
		if(zenglfs_cache_inode_map.isDirty)
		{
			lba = zenglfs_super_block.startLBA + 
					(zenglfs_super_block.InodeBitMapBlockAddr + zenglfs_cache_inode_map.blk_idx) * 2;
			zlox_ide_ata_access(ZLOX_IDE_ATA_WRITE, zenglfs_ide_index, lba, 2, 
						(ZLOX_UINT8 *)zenglfs_cache_inode_map.ptr);
		}
		lba = zenglfs_super_block.startLBA + (zenglfs_super_block.InodeBitMapBlockAddr + blk_idx) * 2;
		zlox_ide_ata_access(ZLOX_IDE_ATA_READ, zenglfs_ide_index, lba, 2, (ZLOX_UINT8 *)zenglfs_cache_inode_map.ptr);
		zenglfs_cache_inode_map.blk_idx = blk_idx;
		zenglfs_cache_inode_map.ino_idx = ino_idx;
		zenglfs_cache_inode_map.isGetData = ZLOX_TRUE;
		zenglfs_cache_inode_map.isDirty = ZLOX_FALSE;
	}
}

static ZLOX_VOID zlox_zenglfs_get_singly(ZLOX_UINT32 blockaddr)
{
	if(zenglfs_cache_singly.ptr == ZLOX_NULL)
	{
		zenglfs_cache_singly.ptr = (ZLOX_UINT32 *)zlox_kmalloc(ZLOX_ZLFS_BLK_SIZE);
		zenglfs_cache_singly.blockaddr = 0;
	}
	if(zenglfs_cache_singly.blockaddr != blockaddr || zenglfs_cache_singly.isGetData == ZLOX_FALSE)
	{
		ZLOX_UINT32 lba;
		if(zenglfs_cache_singly.isDirty)
		{
			lba = zenglfs_super_block.startLBA + zenglfs_cache_singly.blockaddr * 2;
			zlox_ide_ata_access(ZLOX_IDE_ATA_WRITE, zenglfs_ide_index, lba, 2, (ZLOX_UINT8 *)zenglfs_cache_singly.ptr);
		}
		lba = zenglfs_super_block.startLBA + blockaddr * 2;
		zlox_ide_ata_access(ZLOX_IDE_ATA_READ, zenglfs_ide_index, lba, 2, (ZLOX_UINT8 *)zenglfs_cache_singly.ptr);
		zenglfs_cache_singly.blockaddr = blockaddr;
		zenglfs_cache_singly.isGetData = ZLOX_TRUE;
		zenglfs_cache_singly.isDirty = ZLOX_FALSE;
	}
}

static ZLOX_VOID zlox_zenglfs_get_doubly(ZLOX_UINT32 blockaddr)
{
	if(zenglfs_cache_doubly.ptr == ZLOX_NULL)
	{
		zenglfs_cache_doubly.ptr = (ZLOX_UINT32 *)zlox_kmalloc(ZLOX_ZLFS_BLK_SIZE);
		zenglfs_cache_doubly.blockaddr = 0;
	}
	if(zenglfs_cache_doubly.blockaddr != blockaddr || zenglfs_cache_doubly.isGetData == ZLOX_FALSE)
	{
		ZLOX_UINT32 lba;
		if(zenglfs_cache_doubly.isDirty)
		{
			lba = zenglfs_super_block.startLBA + zenglfs_cache_doubly.blockaddr * 2;
			zlox_ide_ata_access(ZLOX_IDE_ATA_WRITE, zenglfs_ide_index, lba, 2, (ZLOX_UINT8 *)zenglfs_cache_doubly.ptr);
		}
		lba = zenglfs_super_block.startLBA + blockaddr * 2;
		zlox_ide_ata_access(ZLOX_IDE_ATA_READ, zenglfs_ide_index, lba, 2, (ZLOX_UINT8 *)zenglfs_cache_doubly.ptr);
		zenglfs_cache_doubly.blockaddr = blockaddr;
		zenglfs_cache_doubly.isGetData = ZLOX_TRUE;
		zenglfs_cache_doubly.isDirty = ZLOX_FALSE;
	}
}

static ZLOX_VOID zlox_zenglfs_get_triply(ZLOX_UINT32 blockaddr)
{
	if(zenglfs_cache_triply.ptr == ZLOX_NULL)
	{
		zenglfs_cache_triply.ptr = (ZLOX_UINT32 *)zlox_kmalloc(ZLOX_ZLFS_BLK_SIZE);
		zenglfs_cache_triply.blockaddr = 0;
	}
	if(zenglfs_cache_triply.blockaddr != blockaddr || zenglfs_cache_triply.isGetData == ZLOX_FALSE)
	{
		ZLOX_UINT32 lba;
		if(zenglfs_cache_triply.isDirty)
		{
			lba = zenglfs_super_block.startLBA + zenglfs_cache_triply.blockaddr * 2;
			zlox_ide_ata_access(ZLOX_IDE_ATA_WRITE, zenglfs_ide_index, lba, 2, (ZLOX_UINT8 *)zenglfs_cache_triply.ptr);
		}
		lba = zenglfs_super_block.startLBA + blockaddr * 2;
		zlox_ide_ata_access(ZLOX_IDE_ATA_READ, zenglfs_ide_index, lba, 2, (ZLOX_UINT8 *)zenglfs_cache_triply.ptr);
		zenglfs_cache_triply.blockaddr = blockaddr;
		zenglfs_cache_triply.isGetData = ZLOX_TRUE;
		zenglfs_cache_triply.isDirty = ZLOX_FALSE;
	}
}

static ZLOX_UINT32 zlox_zenglfs_make_baseblk()
{
	ZLOX_UINT32 idx,c=0,j, g_per_blk = ZLOX_ZLFS_BLK_SIZE / sizeof(ZLOX_GROUP_INFO), blkmap_idx;
	ZLOX_BOOL findblkmap = ZLOX_FALSE;
	ZLOX_GROUP_INFO * g = ZLOX_NULL;
	for(idx = 0;idx < zenglfs_super_block.GroupBlocks;idx++)
	{
		zlox_zenglfs_get_group(idx);
		g = zenglfs_cache_group.ptr;
		for(j=0;j < g_per_blk && c < zenglfs_super_block.GroupCount;c++,g++,j++)
		{
			if(g->allocBlocks < 8192)
			{
				findblkmap = ZLOX_TRUE;
				blkmap_idx = c;
				break;
			}
		}
		if(findblkmap == ZLOX_TRUE)
			break;
	}
	if(findblkmap == ZLOX_FALSE)
		return 0;
	zlox_zenglfs_get_blkmap(blkmap_idx);
	ZLOX_UINT32 bit_idx = zlox_zenglfs_first_bitmap(zenglfs_cache_blkmap.ptr, 8192);
	if(bit_idx == 0xFFFFFFFF)
		return 0;
	zlox_zenglfs_set_bitmap(zenglfs_cache_blkmap.ptr, bit_idx);
	zenglfs_cache_blkmap.isDirty = ZLOX_TRUE;
	g->allocBlocks = ((g->allocBlocks + 1) >= 8192) ? 8192 : (g->allocBlocks + 1);
	zenglfs_super_block.allocBlocks++;
	zenglfs_super_block_isDirty = ZLOX_TRUE;
	zenglfs_cache_group.isDirty = ZLOX_TRUE;
	return (blkmap_idx * 8192 + bit_idx);
}

static ZLOX_UINT32 zlox_zenglfs_free_baseblk(ZLOX_UINT32 blockAddr)
{
	ZLOX_UINT32 blkmap_idx = blockAddr / 8192, g_per_blk = ZLOX_ZLFS_BLK_SIZE / sizeof(ZLOX_GROUP_INFO);
	ZLOX_UINT32 bit_idx = blockAddr % 8192;
	zlox_zenglfs_get_blkmap(blkmap_idx);
	zlox_zenglfs_clear_bitmap(zenglfs_cache_blkmap.ptr, bit_idx);
	zenglfs_cache_blkmap.isDirty = ZLOX_TRUE;
	ZLOX_UINT32 gblk_idx = blkmap_idx / g_per_blk;
	ZLOX_UINT32 g_idx = blkmap_idx % g_per_blk;
	zlox_zenglfs_get_group(gblk_idx);
	zenglfs_cache_group.ptr[g_idx].allocBlocks = (zenglfs_cache_group.ptr[g_idx].allocBlocks != 0) ? 
							(zenglfs_cache_group.ptr[g_idx].allocBlocks - 1) : 0;
	zenglfs_cache_group.isDirty = ZLOX_TRUE;
	zenglfs_super_block.allocBlocks = (zenglfs_super_block.allocBlocks != 0) ? 
						(zenglfs_super_block.allocBlocks - 1) : 0;
	zenglfs_super_block_isDirty = ZLOX_TRUE;
	return blockAddr;
}

static ZLOX_UINT32 zlox_zenglfs_make_inode()
{
	ZLOX_UINT32 idx,c=0,j, g_per_blk = ZLOX_ZLFS_BLK_SIZE / sizeof(ZLOX_GROUP_INFO), imap_idx;
	ZLOX_BOOL findimap = ZLOX_FALSE;
	ZLOX_UINT32 imap_bitnum = ZLOX_ZLFS_INO_MAP_SIZE * 8;
	ZLOX_GROUP_INFO * g = ZLOX_NULL;
	for(idx = 0;idx < zenglfs_super_block.GroupBlocks;idx++)
	{
		zlox_zenglfs_get_group(idx);
		g = zenglfs_cache_group.ptr;
		for(j=0;j < g_per_blk && c < zenglfs_super_block.GroupCount;c++,g++,j++)
		{
			if(g->allocInodes < imap_bitnum)
			{
				findimap = ZLOX_TRUE;
				imap_idx = c;
				break;
			}
		}
		if(findimap == ZLOX_TRUE)
			break;
	}
	if(findimap == ZLOX_FALSE)
		return 0;
	zlox_zenglfs_get_inode_map(imap_idx);
	ZLOX_UINT32 * bitstart = zenglfs_cache_inode_map.ptr + 
					zenglfs_cache_inode_map.ino_idx * (ZLOX_ZLFS_INO_MAP_SIZE / 4);
	ZLOX_UINT32 bit_idx = zlox_zenglfs_first_bitmap(bitstart, imap_bitnum);
	if(bit_idx == 0xFFFFFFFF)
		return 0;
	zlox_zenglfs_set_bitmap(bitstart, bit_idx);
	zenglfs_cache_inode_map.isDirty = ZLOX_TRUE;
	g->allocInodes = ((g->allocInodes + 1) >= imap_bitnum) ? imap_bitnum : (g->allocInodes + 1);
	zenglfs_super_block.allocInodes++;
	zenglfs_super_block_isDirty = ZLOX_TRUE;
	zenglfs_cache_group.isDirty = ZLOX_TRUE;
	return (imap_idx * imap_bitnum + bit_idx + 1);
}

static ZLOX_UINT32 zlox_zenglfs_free_inode(ZLOX_UINT32 inode)
{
	ZLOX_UINT32 imap_bitnum = ZLOX_ZLFS_INO_MAP_SIZE * 8, g_per_blk = ZLOX_ZLFS_BLK_SIZE / sizeof(ZLOX_GROUP_INFO);
	ZLOX_UINT32 imap_idx = (inode - 1) / imap_bitnum;
	zlox_zenglfs_get_inode_map(imap_idx);
	ZLOX_UINT32 * bitstart = zenglfs_cache_inode_map.ptr + 
					zenglfs_cache_inode_map.ino_idx * (ZLOX_ZLFS_INO_MAP_SIZE / 4);
	ZLOX_UINT32 bit_idx = (inode - 1) % imap_bitnum;
	zlox_zenglfs_clear_bitmap(bitstart, bit_idx);
	zenglfs_cache_inode_map.isDirty = ZLOX_TRUE;
	ZLOX_UINT32 gblk_idx = imap_idx / g_per_blk;
	ZLOX_UINT32 g_idx = imap_idx % g_per_blk;
	zlox_zenglfs_get_group(gblk_idx);
	zenglfs_cache_group.ptr[g_idx].allocInodes = (zenglfs_cache_group.ptr[g_idx].allocInodes != 0) ? 
							(zenglfs_cache_group.ptr[g_idx].allocInodes - 1) : 0;
	zenglfs_cache_group.isDirty = ZLOX_TRUE;
	zenglfs_super_block.allocInodes = (zenglfs_super_block.allocInodes != 0) ? 
						(zenglfs_super_block.allocInodes - 1) : 0;
	zenglfs_super_block_isDirty = ZLOX_TRUE;
	return inode;
}

static ZLOX_UINT32 zlox_zenglfs_get_inode_blk(ZLOX_UINT32 blkidx, ZLOX_BOOL makeflag)
{
	ZLOX_UINT32 sizeblk = zenglfs_cache_inode.data.size / ZLOX_ZLFS_BLK_SIZE + 
				((zenglfs_cache_inode.data.size % ZLOX_ZLFS_BLK_SIZE) ? 1 : 0);
	ZLOX_BOOL isnewblk = ZLOX_FALSE;
	if(makeflag == ZLOX_FALSE)
	{
		if(blkidx >= sizeblk)
			return 0;
	}
	if(blkidx < 12)
	{
		if(zenglfs_cache_inode.data.blockAddr[blkidx] != 0)
			return zenglfs_cache_inode.data.blockAddr[blkidx];
		else if(makeflag)
		{
			zenglfs_cache_inode.data.blockAddr[blkidx] = zlox_zenglfs_make_baseblk();
			zenglfs_cache_inode.data.totalblk++;
			zenglfs_cache_inode.isDirty = ZLOX_TRUE; 
			return zenglfs_cache_inode.data.blockAddr[blkidx];
		}
		else
			return 0;
	}
	else if(blkidx < (12 + 256))
	{
		if(zenglfs_cache_inode.data.SinglyBlockAddr == 0)
		{
			if(makeflag == ZLOX_FALSE)
				return 0;
			zenglfs_cache_inode.data.SinglyBlockAddr = zlox_zenglfs_make_baseblk();
			zenglfs_cache_inode.data.totalblk++;
			isnewblk = ZLOX_TRUE;
			zenglfs_cache_inode.isDirty = ZLOX_TRUE;
		}
		zlox_zenglfs_get_singly(zenglfs_cache_inode.data.SinglyBlockAddr);
		if(isnewblk == ZLOX_TRUE)
		{
			zlox_memset((ZLOX_UINT8 *)zenglfs_cache_singly.ptr, 0, ZLOX_ZLFS_BLK_SIZE);
			isnewblk = ZLOX_FALSE;
		}
		ZLOX_UINT32 sidx = (blkidx - 12) % 256;
		if(zenglfs_cache_singly.ptr[sidx] != 0)
			return zenglfs_cache_singly.ptr[sidx];
		else if(makeflag)
		{
			zenglfs_cache_singly.ptr[sidx] = zlox_zenglfs_make_baseblk();
			zenglfs_cache_inode.data.totalblk++;
			zenglfs_cache_singly.isDirty = ZLOX_TRUE;
			zenglfs_cache_inode.isDirty = ZLOX_TRUE;
			return zenglfs_cache_singly.ptr[sidx];
		}
		else
			return 0;
	}
	else if(blkidx < (12 + 256 + 256 * 256))
	{
		if(zenglfs_cache_inode.data.DoublyBlockAddr == 0)
		{
			if(makeflag == ZLOX_FALSE)
				return 0;
			zenglfs_cache_inode.data.DoublyBlockAddr = zlox_zenglfs_make_baseblk();
			zenglfs_cache_inode.data.totalblk++;
			isnewblk = ZLOX_TRUE;
			zenglfs_cache_inode.isDirty = ZLOX_TRUE;
		}
		zlox_zenglfs_get_doubly(zenglfs_cache_inode.data.DoublyBlockAddr);
		if(isnewblk == ZLOX_TRUE)
		{
			zlox_memset((ZLOX_UINT8 *)zenglfs_cache_doubly.ptr, 0, ZLOX_ZLFS_BLK_SIZE);
			isnewblk = ZLOX_FALSE;
		}
		ZLOX_UINT32 d_idx = (blkidx - 12 - 256) / 256;
		ZLOX_UINT32 sidx = (blkidx - 12 - 256) % 256;
		if(zenglfs_cache_doubly.ptr[d_idx] == 0)
		{
			if(makeflag == ZLOX_FALSE)
				return 0;
			zenglfs_cache_doubly.ptr[d_idx] = zlox_zenglfs_make_baseblk();
			zenglfs_cache_inode.data.totalblk++;
			isnewblk = ZLOX_TRUE;
			zenglfs_cache_doubly.isDirty = ZLOX_TRUE;
			zenglfs_cache_inode.isDirty = ZLOX_TRUE;
		}
		zlox_zenglfs_get_singly(zenglfs_cache_doubly.ptr[d_idx]);
		if(isnewblk == ZLOX_TRUE)
		{
			zlox_memset((ZLOX_UINT8 *)zenglfs_cache_singly.ptr, 0, ZLOX_ZLFS_BLK_SIZE);
			isnewblk = ZLOX_FALSE;
		}
		if(zenglfs_cache_singly.ptr[sidx] != 0)
			return zenglfs_cache_singly.ptr[sidx];
		else if(makeflag)
		{
			zenglfs_cache_singly.ptr[sidx] = zlox_zenglfs_make_baseblk();
			zenglfs_cache_inode.data.totalblk++;
			zenglfs_cache_singly.isDirty = ZLOX_TRUE;
			zenglfs_cache_inode.isDirty = ZLOX_TRUE;
			return zenglfs_cache_singly.ptr[sidx];
		}
		else
			return 0;
	}
	else if(blkidx < (12 + 256 + 256 * 256 + 256 * 256 * 256))
	{
		if(zenglfs_cache_inode.data.TriplyBlockAddr == 0)
		{
			if(makeflag == ZLOX_FALSE)
				return 0;
			zenglfs_cache_inode.data.TriplyBlockAddr = zlox_zenglfs_make_baseblk();
			zenglfs_cache_inode.data.totalblk++;
			isnewblk = ZLOX_TRUE;
			zenglfs_cache_inode.isDirty = ZLOX_TRUE;
		}
		zlox_zenglfs_get_triply(zenglfs_cache_inode.data.TriplyBlockAddr);
		if(isnewblk == ZLOX_TRUE)
		{
			zlox_memset((ZLOX_UINT8 *)zenglfs_cache_triply.ptr, 0, ZLOX_ZLFS_BLK_SIZE);
			isnewblk = ZLOX_FALSE;
		}
		ZLOX_UINT32 t_idx = (blkidx - 12 - 256 - 256 * 256) / (256 * 256);
		ZLOX_UINT32 t_mod = (blkidx - 12 - 256 - 256 * 256) % (256 * 256);
		ZLOX_UINT32 d_idx = t_mod / 256;
		ZLOX_UINT32 sidx = t_mod % 256;
		if(zenglfs_cache_triply.ptr[t_idx] == 0)
		{
			if(makeflag == ZLOX_FALSE)
				return 0;
			zenglfs_cache_triply.ptr[t_idx] = zlox_zenglfs_make_baseblk();
			zenglfs_cache_inode.data.totalblk++;
			isnewblk = ZLOX_TRUE;
			zenglfs_cache_triply.isDirty = ZLOX_TRUE;
			zenglfs_cache_inode.isDirty = ZLOX_TRUE;
		}
		zlox_zenglfs_get_doubly(zenglfs_cache_triply.ptr[t_idx]);
		if(isnewblk == ZLOX_TRUE)
		{
			zlox_memset((ZLOX_UINT8 *)zenglfs_cache_doubly.ptr, 0, ZLOX_ZLFS_BLK_SIZE);
			isnewblk = ZLOX_FALSE;
		}
		if(zenglfs_cache_doubly.ptr[d_idx] == 0)
		{
			if(makeflag == ZLOX_FALSE)
				return 0;
			zenglfs_cache_doubly.ptr[d_idx] = zlox_zenglfs_make_baseblk();
			zenglfs_cache_inode.data.totalblk++;
			isnewblk = ZLOX_TRUE;
			zenglfs_cache_doubly.isDirty = ZLOX_TRUE;
			zenglfs_cache_inode.isDirty = ZLOX_TRUE;
		}
		zlox_zenglfs_get_singly(zenglfs_cache_doubly.ptr[d_idx]);
		if(isnewblk == ZLOX_TRUE)
		{
			zlox_memset((ZLOX_UINT8 *)zenglfs_cache_singly.ptr, 0, ZLOX_ZLFS_BLK_SIZE);
			isnewblk = ZLOX_FALSE;
		}
		if(zenglfs_cache_singly.ptr[sidx] != 0)
			return zenglfs_cache_singly.ptr[sidx];
		else if(makeflag)
		{
			zenglfs_cache_singly.ptr[sidx] = zlox_zenglfs_make_baseblk();
			zenglfs_cache_inode.data.totalblk++;
			zenglfs_cache_singly.isDirty = ZLOX_TRUE;
			zenglfs_cache_inode.isDirty = ZLOX_TRUE;
			return zenglfs_cache_singly.ptr[sidx];
		}
		else
			return 0;
	}

	return 0;
}

static ZLOX_BOOL zlox_zenglfs_detect_mid_blk_empty(ZLOX_UINT32 * ptr, ZLOX_UINT32 totalnum)
{
	ZLOX_UINT32 i;
	for(i=0;i < totalnum;i++)
	{
		if(ptr[i] != 0)
			return ZLOX_FALSE;
	}
	return ZLOX_TRUE;
}

static ZLOX_UINT32 zlox_zenglfs_free_inode_blk(ZLOX_UINT32 blkidx)
{
	ZLOX_UINT32 sizeblk = zenglfs_cache_inode.data.size / ZLOX_ZLFS_BLK_SIZE + 
				((zenglfs_cache_inode.data.size % ZLOX_ZLFS_BLK_SIZE) ? 1 : 0);
	ZLOX_UINT32 ret = 0;
	if(blkidx >= sizeblk)
		return 0;

	if(blkidx < 12)
	{
		if(zenglfs_cache_inode.data.blockAddr[blkidx] != 0)
		{
			zlox_zenglfs_free_baseblk(zenglfs_cache_inode.data.blockAddr[blkidx]);
			ret = zenglfs_cache_inode.data.blockAddr[blkidx];
			zenglfs_cache_inode.data.blockAddr[blkidx] = 0;
			zenglfs_cache_inode.data.totalblk = (zenglfs_cache_inode.data.totalblk != 0) ? 
								(zenglfs_cache_inode.data.totalblk - 1) : 0;
			zenglfs_cache_inode.isDirty = ZLOX_TRUE; 
			return ret;
		}
		else
			return 0;
	}
	else if(blkidx < (12 + 256))
	{
		if(zenglfs_cache_inode.data.SinglyBlockAddr == 0)
			return 0;
		zlox_zenglfs_get_singly(zenglfs_cache_inode.data.SinglyBlockAddr);
		ZLOX_UINT32 sidx = (blkidx - 12) % 256;
		if(zenglfs_cache_singly.ptr[sidx] == 0)
			return 0;
		zlox_zenglfs_free_baseblk(zenglfs_cache_singly.ptr[sidx]);
		zenglfs_cache_inode.data.totalblk = (zenglfs_cache_inode.data.totalblk != 0) ? 
							(zenglfs_cache_inode.data.totalblk - 1) : 0;
		ret = zenglfs_cache_singly.ptr[sidx];
		zenglfs_cache_singly.ptr[sidx] = 0;
		zenglfs_cache_singly.isDirty = ZLOX_TRUE;
		if(zlox_zenglfs_detect_mid_blk_empty(zenglfs_cache_singly.ptr, 256) == ZLOX_TRUE)
		{
			zlox_zenglfs_free_baseblk(zenglfs_cache_inode.data.SinglyBlockAddr);
			zenglfs_cache_inode.data.totalblk = (zenglfs_cache_inode.data.totalblk != 0) ? 
							(zenglfs_cache_inode.data.totalblk - 1) : 0;
			zenglfs_cache_inode.data.SinglyBlockAddr = 0;
			zenglfs_cache_inode.isDirty = ZLOX_TRUE;
		}
		return ret;
	}
	else if(blkidx < (12 + 256 + 256 * 256))
	{
		if(zenglfs_cache_inode.data.DoublyBlockAddr == 0)
			return 0;
		zlox_zenglfs_get_doubly(zenglfs_cache_inode.data.DoublyBlockAddr);
		ZLOX_UINT32 d_idx = (blkidx - 12 - 256) / 256;
		ZLOX_UINT32 sidx = (blkidx - 12 - 256) % 256;
		if(zenglfs_cache_doubly.ptr[d_idx] == 0)
			return 0;
		zlox_zenglfs_get_singly(zenglfs_cache_doubly.ptr[d_idx]);
		if(zenglfs_cache_singly.ptr[sidx] == 0)
			return 0;
		zlox_zenglfs_free_baseblk(zenglfs_cache_singly.ptr[sidx]);
		zenglfs_cache_inode.data.totalblk = (zenglfs_cache_inode.data.totalblk != 0) ? 
							(zenglfs_cache_inode.data.totalblk - 1) : 0;
		ret = zenglfs_cache_singly.ptr[sidx];
		zenglfs_cache_singly.ptr[sidx] = 0;
		zenglfs_cache_singly.isDirty = ZLOX_TRUE;
		if(zlox_zenglfs_detect_mid_blk_empty(zenglfs_cache_singly.ptr, 256) == ZLOX_TRUE)
		{
			zlox_zenglfs_free_baseblk(zenglfs_cache_doubly.ptr[d_idx]);
			zenglfs_cache_inode.data.totalblk = (zenglfs_cache_inode.data.totalblk != 0) ? 
							(zenglfs_cache_inode.data.totalblk - 1) : 0;
			zenglfs_cache_doubly.ptr[d_idx] = 0;
			zenglfs_cache_doubly.isDirty = ZLOX_TRUE;
		}
		if(zlox_zenglfs_detect_mid_blk_empty(zenglfs_cache_doubly.ptr, 256) == ZLOX_TRUE)
		{
			zlox_zenglfs_free_baseblk(zenglfs_cache_inode.data.DoublyBlockAddr);
			zenglfs_cache_inode.data.totalblk = (zenglfs_cache_inode.data.totalblk != 0) ? 
							(zenglfs_cache_inode.data.totalblk - 1) : 0;
			zenglfs_cache_inode.data.DoublyBlockAddr = 0;
			zenglfs_cache_inode.isDirty = ZLOX_TRUE;
		}
		return ret;
	}
	else if(blkidx < (12 + 256 + 256 * 256 + 256 * 256 * 256))
	{
		if(zenglfs_cache_inode.data.TriplyBlockAddr == 0)
			return 0;
		zlox_zenglfs_get_triply(zenglfs_cache_inode.data.TriplyBlockAddr);
		ZLOX_UINT32 t_idx = (blkidx - 12 - 256 - 256 * 256) / (256 * 256);
		ZLOX_UINT32 t_mod = (blkidx - 12 - 256 - 256 * 256) % (256 * 256);
		ZLOX_UINT32 d_idx = t_mod / 256;
		ZLOX_UINT32 sidx = t_mod % 256;
		if(zenglfs_cache_triply.ptr[t_idx] == 0)
			return 0;
		zlox_zenglfs_get_doubly(zenglfs_cache_triply.ptr[t_idx]);
		if(zenglfs_cache_doubly.ptr[d_idx] == 0)
			return 0;
		zlox_zenglfs_get_singly(zenglfs_cache_doubly.ptr[d_idx]);
		if(zenglfs_cache_singly.ptr[sidx] == 0)
			return 0;
		zlox_zenglfs_free_baseblk(zenglfs_cache_singly.ptr[sidx]);
		zenglfs_cache_inode.data.totalblk = (zenglfs_cache_inode.data.totalblk != 0) ? 
							(zenglfs_cache_inode.data.totalblk - 1) : 0;
		ret = zenglfs_cache_singly.ptr[sidx];
		zenglfs_cache_singly.ptr[sidx] = 0;
		zenglfs_cache_singly.isDirty = ZLOX_TRUE;
		if(zlox_zenglfs_detect_mid_blk_empty(zenglfs_cache_singly.ptr, 256) == ZLOX_TRUE)
		{
			zlox_zenglfs_free_baseblk(zenglfs_cache_doubly.ptr[d_idx]);
			zenglfs_cache_inode.data.totalblk = (zenglfs_cache_inode.data.totalblk != 0) ? 
							(zenglfs_cache_inode.data.totalblk - 1) : 0;
			zenglfs_cache_doubly.ptr[d_idx] = 0;
			zenglfs_cache_doubly.isDirty = ZLOX_TRUE;
		}
		if(zlox_zenglfs_detect_mid_blk_empty(zenglfs_cache_doubly.ptr, 256) == ZLOX_TRUE)
		{
			zlox_zenglfs_free_baseblk(zenglfs_cache_triply.ptr[t_idx]);
			zenglfs_cache_inode.data.totalblk = (zenglfs_cache_inode.data.totalblk != 0) ? 
							(zenglfs_cache_inode.data.totalblk - 1) : 0;
			zenglfs_cache_triply.ptr[t_idx] = 0;
			zenglfs_cache_triply.isDirty = ZLOX_TRUE;
		}
		if(zlox_zenglfs_detect_mid_blk_empty(zenglfs_cache_triply.ptr, 256) == ZLOX_TRUE)
		{
			zlox_zenglfs_free_baseblk(zenglfs_cache_inode.data.TriplyBlockAddr);
			zenglfs_cache_inode.data.totalblk = (zenglfs_cache_inode.data.totalblk != 0) ? 
							(zenglfs_cache_inode.data.totalblk - 1) : 0;
			zenglfs_cache_inode.data.TriplyBlockAddr = 0;
			zenglfs_cache_inode.isDirty = ZLOX_TRUE;
		}
		return ret;
	}
	return 0;
}

static ZLOX_VOID zlox_zenglfs_sync_cache_to_disk()
{
	ZLOX_UINT8 * buffer = (ZLOX_UINT8 *)zlox_kmalloc(ZLOX_ZLFS_BLK_SIZE);
	ZLOX_UINT32 i_per_blk = ZLOX_ZLFS_BLK_SIZE / sizeof(ZLOX_INODE_DATA);
	ZLOX_UINT32 idx;
	ZLOX_UINT32 lba;
	if(zenglfs_cache_inode.isDirty)
	{
		idx = (zenglfs_cache_inode.ino - 1) % i_per_blk;
		lba = zenglfs_super_block.startLBA + (zenglfs_super_block.InodeTableBlockAddr + 
								zenglfs_cache_inode_blk.blk) * 2;
		zenglfs_cache_inode_blk.ptr[idx] = zenglfs_cache_inode.data;
		zlox_ide_ata_access(ZLOX_IDE_ATA_WRITE, zenglfs_ide_index, lba, 2, 
							(ZLOX_UINT8 *)zenglfs_cache_inode_blk.ptr);
		zenglfs_cache_inode.isDirty = ZLOX_FALSE;
	}
	if(zenglfs_cache_group.isDirty)
	{
		lba = zenglfs_super_block.startLBA + 
				(zenglfs_super_block.GroupAddr + zenglfs_cache_group.idx) * 2;
		zlox_ide_ata_access(ZLOX_IDE_ATA_WRITE, zenglfs_ide_index, lba, 2, (ZLOX_UINT8 *)zenglfs_cache_group.ptr);
		zenglfs_cache_group.isDirty = ZLOX_FALSE;
	}
	if(zenglfs_cache_blkmap.isDirty)
	{
		lba = zenglfs_super_block.startLBA + 
				(zenglfs_super_block.BlockBitMapBlockAddr + zenglfs_cache_blkmap.idx) * 2;
		zlox_ide_ata_access(ZLOX_IDE_ATA_WRITE, zenglfs_ide_index, lba, 2, (ZLOX_UINT8 *)zenglfs_cache_blkmap.ptr);
		zenglfs_cache_blkmap.isDirty = ZLOX_FALSE;
	}
	if(zenglfs_cache_inode_map.isDirty)
	{
		lba = zenglfs_super_block.startLBA + 
				(zenglfs_super_block.InodeBitMapBlockAddr + zenglfs_cache_inode_map.blk_idx) * 2;
		zlox_ide_ata_access(ZLOX_IDE_ATA_WRITE, zenglfs_ide_index, lba, 2, 
					(ZLOX_UINT8 *)zenglfs_cache_inode_map.ptr);
		zenglfs_cache_inode_map.isDirty = ZLOX_FALSE;
	}
	if(zenglfs_cache_singly.isDirty)
	{
		lba = zenglfs_super_block.startLBA + zenglfs_cache_singly.blockaddr * 2;
		zlox_ide_ata_access(ZLOX_IDE_ATA_WRITE, zenglfs_ide_index, lba, 2, (ZLOX_UINT8 *)zenglfs_cache_singly.ptr);
		zenglfs_cache_singly.isDirty = ZLOX_FALSE;
	}
	if(zenglfs_cache_doubly.isDirty)
	{
		lba = zenglfs_super_block.startLBA + zenglfs_cache_doubly.blockaddr * 2;
		zlox_ide_ata_access(ZLOX_IDE_ATA_WRITE, zenglfs_ide_index, lba, 2, (ZLOX_UINT8 *)zenglfs_cache_doubly.ptr);
		zenglfs_cache_doubly.isDirty = ZLOX_FALSE;
	}
	if(zenglfs_cache_triply.isDirty)
	{
		lba = zenglfs_super_block.startLBA + zenglfs_cache_triply.blockaddr * 2;
		zlox_ide_ata_access(ZLOX_IDE_ATA_WRITE, zenglfs_ide_index, lba, 2, (ZLOX_UINT8 *)zenglfs_cache_triply.ptr);
		zenglfs_cache_triply.isDirty = ZLOX_FALSE;
	}
	if(zenglfs_super_block_isDirty)
	{
		zlox_memset(buffer, 0, ZLOX_ZLFS_BLK_SIZE);
		zlox_memcpy(buffer, (ZLOX_UINT8 *)&zenglfs_super_block, sizeof(ZLOX_ZLFS_SUPER_BLOCK));
		zlox_ide_ata_access(ZLOX_IDE_ATA_WRITE, zenglfs_ide_index, zenglfs_super_block_lba, 2, buffer);
		zenglfs_super_block_isDirty = ZLOX_FALSE;
	}
	zlox_kfree(buffer);
}

static ZLOX_UINT32 zlox_zenglfs_read(ZLOX_FS_NODE * node, ZLOX_UINT32 arg_offset, ZLOX_UINT32 size, ZLOX_UINT8 * buffer)
{
	ZLOX_UNUSED(arg_offset);

	if((node->flags & 0x7) != ZLOX_FS_FILE)
		return ZLOX_NULL;
	if(size == 0)
		return 0;
	if(buffer == ZLOX_NULL)
		return 0;
	if(zenglfs_cache_inode.ino != node->inode)
		zlox_zenglfs_get_inode(node->inode);

	ZLOX_UINT32 tmp_off = 0, blk, lba, i , ret = 0;
	ZLOX_BOOL need_memcpy = ZLOX_FALSE;
	if(size % ZLOX_ZLFS_BLK_SIZE != 0)
		need_memcpy = ZLOX_TRUE;
	for(i=0;tmp_off < size && (blk = zlox_zenglfs_get_inode_blk(i, ZLOX_FALSE));i++, tmp_off += ZLOX_ZLFS_BLK_SIZE)
	{
		lba = zenglfs_super_block.startLBA + blk * 2;
		if(need_memcpy == ZLOX_TRUE && ((size - tmp_off) / ZLOX_ZLFS_BLK_SIZE == 0))
		{
			ZLOX_UINT8 * tmp_buffer = (ZLOX_UINT8 *)zlox_kmalloc(ZLOX_ZLFS_BLK_SIZE);
			zlox_ide_ata_access(ZLOX_IDE_ATA_READ, zenglfs_ide_index, lba, 2, tmp_buffer);
			zlox_memcpy((buffer + tmp_off), tmp_buffer, (size - tmp_off));
			zlox_kfree(tmp_buffer);
		}
		else
			zlox_ide_ata_access(ZLOX_IDE_ATA_READ, zenglfs_ide_index, lba, 2, (buffer + tmp_off));
	}
	if(zenglfs_cache_inode.data.size > size)
		ret = size;
	else
		ret = zenglfs_cache_inode.data.size;
	return ret;
}

static ZLOX_UINT32 zlox_zenglfs_write(ZLOX_FS_NODE * node, ZLOX_UINT32 arg_offset, ZLOX_UINT32 size, ZLOX_UINT8 * buffer)
{
	ZLOX_UNUSED(arg_offset);

	if((node->flags & 0x7) != ZLOX_FS_FILE)
		return 0;
	if(size == 0)
		return 0;
	if(buffer == ZLOX_NULL)
		return 0;
	if(zenglfs_cache_inode.ino != node->inode)
		zlox_zenglfs_get_inode(node->inode);

	ZLOX_UINT32 tmp_off = 0, /*before_totalblk,*/ blk, lba, i, nwrites = 0;
	ZLOX_BOOL need_memcpy = ZLOX_FALSE;
	if(size % ZLOX_ZLFS_BLK_SIZE != 0)
		need_memcpy = ZLOX_TRUE;
	//before_totalblk = zenglfs_cache_inode.data.totalblk;
	for(i=0;tmp_off < size;i++, tmp_off += ZLOX_ZLFS_BLK_SIZE)
	{
		blk = zlox_zenglfs_get_inode_blk(i, ZLOX_TRUE);
		if(blk == 0)
			return 0;
		nwrites++;
		lba = zenglfs_super_block.startLBA + blk * 2;
		if(need_memcpy == ZLOX_TRUE && ((size - tmp_off) / ZLOX_ZLFS_BLK_SIZE == 0))
		{
			ZLOX_UINT8 * tmp_buffer = (ZLOX_UINT8 *)zlox_kmalloc(ZLOX_ZLFS_BLK_SIZE);
			zlox_memset(tmp_buffer, 0 , ZLOX_ZLFS_BLK_SIZE);
			zlox_memcpy(tmp_buffer, (buffer + tmp_off), (size - tmp_off));
			zlox_ide_ata_access(ZLOX_IDE_ATA_WRITE, zenglfs_ide_index, lba, 2, tmp_buffer);
			zlox_kfree(tmp_buffer);
		}
		else
			zlox_ide_ata_access(ZLOX_IDE_ATA_WRITE, zenglfs_ide_index, lba, 2, (buffer + tmp_off));
	}
	//if(before_totalblk > zenglfs_cache_inode.data.totalblk)
	for(i=nwrites;(blk =zlox_zenglfs_free_inode_blk(i));i++)
		;

	zenglfs_cache_inode.data.size = size;
	zenglfs_cache_inode.isDirty = ZLOX_TRUE;
	zlox_zenglfs_sync_cache_to_disk();
	return size;
}

static ZLOX_UINT32 zlox_zenglfs_rename(ZLOX_FS_NODE *node, ZLOX_CHAR * rename)
{
	if(node == ZLOX_NULL)
		return 0;
	if(rename == ZLOX_NULL)
		return 0;
	if(zlox_strlen(rename)==0)
		return 0;
	if(zenglfs_cache_inode.ino != node->inode)
		zlox_zenglfs_get_inode(node->inode);
	ZLOX_UINT32 node_namelength = zlox_strlen(node->name);
	zlox_zenglfs_get_inode(zenglfs_cache_inode.data.dir_inode);

	ZLOX_UINT8 *buffer = (ZLOX_UINT8 *)zlox_kmalloc(ZLOX_ZLFS_BLK_SIZE);
	ZLOX_UINT32 i,j,blk,lba, dir_per_blk = ZLOX_ZLFS_BLK_SIZE / sizeof(ZLOX_ZLFS_DIR_ENTRY);
	for(i=0;(blk = zlox_zenglfs_get_inode_blk(i, ZLOX_FALSE));i++)
	{
		lba = zenglfs_super_block.startLBA + blk * 2;
		zlox_ide_ata_access(ZLOX_IDE_ATA_READ, zenglfs_ide_index, lba, 2, buffer);
		ZLOX_ZLFS_DIR_ENTRY * d = (ZLOX_ZLFS_DIR_ENTRY *)buffer;
		for(j=0;j < dir_per_blk;j++,d++)
		{
			if(d->namelength == node_namelength && zlox_strcmp(d->name, node->name) == 0)
			{
				ZLOX_UINT32 len = (ZLOX_UINT32)zlox_strlen(rename);
				d->namelength = (len > 55 ? 55 : len);
				zlox_memcpy((ZLOX_UINT8 *)d->name, (ZLOX_UINT8 *)rename, d->namelength);
				d->name[d->namelength] = '\0';
				ZLOX_CHAR * c = d->name;
				for(;(*c)!=0;c++)
				{
					if((*c) == '/')
						(*c) = '_';
				}
				zlox_ide_ata_access(ZLOX_IDE_ATA_WRITE, zenglfs_ide_index, lba, 2, buffer);
				zlox_kfree(buffer);
				return node->inode;
			}
		}
	}
	zlox_kfree(buffer);
	return 0;
}

static ZLOX_UINT32 zlox_zenglfs_remove(ZLOX_FS_NODE *node)
{
	if(node == ZLOX_NULL)
		return 0;
	if(zenglfs_cache_inode.ino != node->inode)
		zlox_zenglfs_get_inode(node->inode);
	if((node->flags & 0x7) == ZLOX_FS_DIRECTORY && zenglfs_cache_inode.data.dir_item_num != 0)
		return 0;

	ZLOX_UINT32 i, blk;
	for(i=0;(blk =zlox_zenglfs_free_inode_blk(i));i++)
		;
	zlox_zenglfs_free_inode(node->inode);
	ZLOX_ZLFS_DIR_ENTRY * tmp_buffer = (ZLOX_ZLFS_DIR_ENTRY *)zlox_kmalloc(ZLOX_ZLFS_BLK_SIZE);
	ZLOX_UINT32 lba = zenglfs_super_block.startLBA + zenglfs_cache_inode.data.dirent_block * 2;
	zlox_ide_ata_access(ZLOX_IDE_ATA_READ, zenglfs_ide_index, lba, 2, (ZLOX_UINT8 *)tmp_buffer);
	zlox_memset((ZLOX_UINT8 *)&tmp_buffer[zenglfs_cache_inode.data.dirent_idx], 0, 
				sizeof(ZLOX_ZLFS_DIR_ENTRY));
	zlox_ide_ata_access(ZLOX_IDE_ATA_WRITE, zenglfs_ide_index, lba, 2, (ZLOX_UINT8 *)tmp_buffer);
	zlox_kfree(tmp_buffer);
	ZLOX_UINT32 dir_inode = zenglfs_cache_inode.data.dir_inode;
	zlox_memset((ZLOX_UINT8 *)&zenglfs_cache_inode.data, 0, sizeof(ZLOX_INODE_DATA));
	zenglfs_cache_inode.isDirty = ZLOX_TRUE;
	zlox_zenglfs_get_inode(dir_inode);
	zenglfs_cache_inode.data.dir_item_num = ((zenglfs_cache_inode.data.dir_item_num != 0) ? 
						 	(zenglfs_cache_inode.data.dir_item_num - 1) : 0);
	zenglfs_cache_inode.isDirty = ZLOX_TRUE;
	zlox_zenglfs_sync_cache_to_disk();
	return node->inode;
}

static ZLOX_DIRENT * zlox_zenglfs_readdir(ZLOX_FS_NODE *node, ZLOX_UINT32 index)
{
	if((node->flags & 0x7) != ZLOX_FS_DIRECTORY)
		return ZLOX_NULL;
	if(zenglfs_cache_inode.ino != node->inode)
		zlox_zenglfs_get_inode(node->inode);
	if(zenglfs_cache_inode.data.size == 0)
		return ZLOX_NULL;

	ZLOX_UINT8 *buffer = (ZLOX_UINT8 *)zlox_kmalloc(ZLOX_ZLFS_BLK_SIZE);
	ZLOX_UINT32 i,j,blk,lba,idx = 0, dir_per_blk = ZLOX_ZLFS_BLK_SIZE / sizeof(ZLOX_ZLFS_DIR_ENTRY);
	for(i=0;(blk = zlox_zenglfs_get_inode_blk(i, ZLOX_FALSE));i++)
	{
		lba = zenglfs_super_block.startLBA + blk * 2;
		zlox_ide_ata_access(ZLOX_IDE_ATA_READ, zenglfs_ide_index, lba, 2, buffer);
		ZLOX_ZLFS_DIR_ENTRY * d = (ZLOX_ZLFS_DIR_ENTRY *)buffer;
		for(j=0;j < dir_per_blk;j++,d++,idx++)
		{
			if(idx == index)
			{
				zlox_memcpy((ZLOX_UINT8 *)zenglfs_dirent.name, (ZLOX_UINT8 *)d->name, 
					d->namelength);
				zenglfs_dirent.name[d->namelength] = '\0';
				zenglfs_dirent.ino = d->inode;
				zlox_kfree(buffer);
				return &zenglfs_dirent;
			}
		}
	}
	zlox_kfree(buffer);
	return ZLOX_NULL;
}

static ZLOX_FS_NODE * zlox_zenglfs_writedir(ZLOX_FS_NODE *node, ZLOX_CHAR *name, ZLOX_UINT16 type)
{
	if((node->flags & 0x7) != ZLOX_FS_DIRECTORY)
		return ZLOX_NULL;
	if(type != ZLOX_FS_FILE && type != ZLOX_FS_DIRECTORY)
		return ZLOX_NULL;
	if(name == ZLOX_NULL)
		return ZLOX_NULL;
	if(zlox_strlen(name) == 0)
		return ZLOX_NULL;
	if(zenglfs_cache_inode.ino != node->inode)
		zlox_zenglfs_get_inode(node->inode);
	ZLOX_UINT32 i,j,blk,lba, dir_per_blk = ZLOX_ZLFS_BLK_SIZE / sizeof(ZLOX_ZLFS_DIR_ENTRY),len,before_totalblk;
	ZLOX_UINT8 *buffer = (ZLOX_UINT8 *)zlox_kmalloc(ZLOX_ZLFS_BLK_SIZE);
	ZLOX_BOOL isfind_dir = ZLOX_FALSE, isnewblk = ZLOX_FALSE;
	ZLOX_UINT32 dirent_block, dirent_idx, newinode;
	for(i=0;;i++)
	{
		before_totalblk = zenglfs_cache_inode.data.totalblk;
		blk = zlox_zenglfs_get_inode_blk(i, ZLOX_TRUE);
		if(before_totalblk < zenglfs_cache_inode.data.totalblk)
		{
			isnewblk = ZLOX_TRUE;
			zenglfs_cache_inode.data.size += ZLOX_ZLFS_BLK_SIZE;
			zenglfs_cache_inode.isDirty = ZLOX_TRUE;
		}
		if(blk == 0)
		{
			zlox_kfree(buffer);
			return ZLOX_NULL;
		}
		lba = zenglfs_super_block.startLBA + blk * 2;
		zlox_ide_ata_access(ZLOX_IDE_ATA_READ, zenglfs_ide_index, lba, 2, buffer);
		if(isnewblk == ZLOX_TRUE)
		{
			zlox_memset(buffer, 0, ZLOX_ZLFS_BLK_SIZE);
			isnewblk = ZLOX_FALSE;
		}
		ZLOX_ZLFS_DIR_ENTRY * d = (ZLOX_ZLFS_DIR_ENTRY *)buffer;
		for(j=0;j < dir_per_blk;j++,d++)
		{
			if(d->inode == 0)
			{
				d->inode = newinode = zlox_zenglfs_make_inode();
				d->type = ((type == ZLOX_FS_FILE) ? ZLOX_ZLFS_INODE_TYPE_REGULAR : ZLOX_ZLFS_INODE_TYPE_DIRECTORY);
				len = (ZLOX_UINT32)zlox_strlen(name);
				d->namelength = (len > 55 ? 55 : len);
				zlox_memcpy((ZLOX_UINT8 *)d->name, (ZLOX_UINT8 *)name, d->namelength);
				d->name[d->namelength] = '\0';
				ZLOX_CHAR * c = d->name;
				for(;(*c)!=0;c++)
				{
					if((*c) == '/')
						(*c) = '_';
				}
				dirent_block = blk;
				dirent_idx = j;
				isfind_dir = ZLOX_TRUE;
				break;
			}
		}
		if(isfind_dir == ZLOX_TRUE)
			break;
	}
	if(isfind_dir == ZLOX_FALSE)
	{
		zlox_kfree(buffer);
		return ZLOX_NULL;
	}
	zlox_ide_ata_access(ZLOX_IDE_ATA_WRITE, zenglfs_ide_index, lba, 2, buffer);
	zlox_kfree(buffer);
	zenglfs_cache_inode.data.dir_item_num++;
	zenglfs_cache_inode.isDirty = ZLOX_TRUE;
	zlox_zenglfs_get_inode(newinode);
	zlox_memset((ZLOX_UINT8 *)&zenglfs_cache_inode.data, 0 , sizeof(ZLOX_INODE_DATA));
	zenglfs_cache_inode.data.type = ((type == ZLOX_FS_FILE) ? ZLOX_ZLFS_INODE_TYPE_REGULAR : ZLOX_ZLFS_INODE_TYPE_DIRECTORY);
	zenglfs_cache_inode.data.dirent_block = dirent_block;
	zenglfs_cache_inode.data.dirent_idx = dirent_idx;
	zenglfs_cache_inode.data.dir_inode = node->inode;
	zenglfs_cache_inode.isDirty = ZLOX_TRUE;
	zlox_zenglfs_sync_cache_to_disk();
	zlox_memset((ZLOX_UINT8 *)&zenglfs_fsnode,0,sizeof(ZLOX_FS_NODE));
	zlox_strcpy(zenglfs_fsnode.name, name);
	zenglfs_fsnode.flags = type;
	if(type == ZLOX_FS_DIRECTORY)
	{
		zenglfs_fsnode.readdir = &zlox_zenglfs_readdir;
		zenglfs_fsnode.writedir = &zlox_zenglfs_writedir;
		zenglfs_fsnode.finddir = &zlox_zenglfs_finddir;
		zenglfs_fsnode.remove = &zlox_zenglfs_remove;
		zenglfs_fsnode.rename = &zlox_zenglfs_rename;
	}
	else
	{
		zenglfs_fsnode.read = &zlox_zenglfs_read;
		zenglfs_fsnode.write = &zlox_zenglfs_write;
		zenglfs_fsnode.remove = &zlox_zenglfs_remove;
		zenglfs_fsnode.rename = &zlox_zenglfs_rename;
	}
	zenglfs_fsnode.length = zenglfs_cache_inode.data.size;
	zenglfs_fsnode.inode = newinode;
	return &zenglfs_fsnode;
}

ZLOX_FS_NODE * zlox_zenglfs_finddir_ext(ZLOX_FS_NODE *node, ZLOX_CHAR *name)
{
	ZLOX_SINT32 name_len = zlox_strlen(name),j;
	ZLOX_BOOL flag = ZLOX_FALSE;
	if(name_len <= 0)
		return 0;
	zlox_strcpy(zenglfs_tmp_name,name);
	for(j=0;j < name_len;j++)
	{
		if(zenglfs_tmp_name[j] == '/')
		{
			zenglfs_tmp_name[j] = '\0';
			if(!flag)
				flag = ZLOX_TRUE;
		}
	}
	if(!flag)
		return zlox_zenglfs_finddir(node, name);

	ZLOX_SINT32 tmp_name_offset = 0;
	ZLOX_CHAR * tmp_name = zenglfs_tmp_name + tmp_name_offset;
	ZLOX_FS_NODE * ret_fsnode = ZLOX_NULL, * tmp_node = node;
	while(ZLOX_TRUE)
	{
		ret_fsnode = zlox_zenglfs_finddir(tmp_node, tmp_name);
		if(ret_fsnode == ZLOX_NULL)
		{
			return ZLOX_NULL;
		}
		tmp_node = ret_fsnode;
		tmp_name_offset += zlox_strlen(tmp_name) + 1;
		if(tmp_name_offset < name_len + 1)
		{
			tmp_name = zenglfs_tmp_name + tmp_name_offset;
		}
		else
			break;
	}
	return ret_fsnode;
}

static ZLOX_FS_NODE * zlox_zenglfs_finddir(ZLOX_FS_NODE *node, ZLOX_CHAR *name)
{
	if((node->flags & 0x7) != ZLOX_FS_DIRECTORY)
		return ZLOX_NULL;
	if(zenglfs_cache_inode.ino != node->inode)
		zlox_zenglfs_get_inode(node->inode);
	if(zenglfs_cache_inode.data.size == 0)
		return ZLOX_NULL;

	ZLOX_UINT32 namelength = zlox_strlen(name);
	if(namelength == 0)
		return ZLOX_NULL;

	ZLOX_UINT8 *buffer = (ZLOX_UINT8 *)zlox_kmalloc(ZLOX_ZLFS_BLK_SIZE);
	ZLOX_UINT32 i,j,blk,lba,idx = 0, dir_per_blk = ZLOX_ZLFS_BLK_SIZE / sizeof(ZLOX_ZLFS_DIR_ENTRY);
	for(i=0;(blk = zlox_zenglfs_get_inode_blk(i, ZLOX_FALSE));i++)
	{
		lba = zenglfs_super_block.startLBA + blk * 2;
		zlox_ide_ata_access(ZLOX_IDE_ATA_READ, zenglfs_ide_index, lba, 2, buffer);
		ZLOX_ZLFS_DIR_ENTRY * d = (ZLOX_ZLFS_DIR_ENTRY *)buffer;
		for(j=0;j < dir_per_blk;j++,d++,idx++)
		{
			if(d->namelength == namelength && zlox_strcmp(d->name , name) == 0)
			{
				zlox_memset((ZLOX_UINT8 *)&zenglfs_fsnode,0,sizeof(ZLOX_FS_NODE));
				zlox_strcpy(zenglfs_fsnode.name, d->name);				
				if(d->type == ZLOX_ZLFS_INODE_TYPE_DIRECTORY)
				{
					zenglfs_fsnode.flags = ZLOX_FS_DIRECTORY;
					zenglfs_fsnode.readdir = zlox_zenglfs_readdir;
					zenglfs_fsnode.writedir = &zlox_zenglfs_writedir;
					zenglfs_fsnode.finddir = &zlox_zenglfs_finddir;
					zenglfs_fsnode.remove = &zlox_zenglfs_remove;
					zenglfs_fsnode.rename = &zlox_zenglfs_rename;
				}
				else
				{
					zenglfs_fsnode.flags = ZLOX_FS_FILE;
					zenglfs_fsnode.read = &zlox_zenglfs_read;
					zenglfs_fsnode.write = &zlox_zenglfs_write;
					zenglfs_fsnode.remove = &zlox_zenglfs_remove;
					zenglfs_fsnode.rename = &zlox_zenglfs_rename;
				}
				zenglfs_fsnode.inode = d->inode;
				zlox_zenglfs_get_inode(d->inode);
				zenglfs_fsnode.length = zenglfs_cache_inode.data.size;
				zlox_kfree(buffer);
				return &zenglfs_fsnode;
			}
		}
	}
	zlox_kfree(buffer);
	return ZLOX_NULL;
}

ZLOX_FS_NODE * zlox_unmount_zenglfs()
{
	zlox_zenglfs_sync_cache_to_disk();
	if(zenglfs_root != ZLOX_NULL)
	{
		zlox_kfree(zenglfs_root);
		zenglfs_root = ZLOX_NULL;
		zenglfs_ide_index = -1;
	}
	zlox_memset((ZLOX_UINT8 *)&zenglfs_super_block, 0, sizeof(ZLOX_ZLFS_SUPER_BLOCK));
	zlox_memset((ZLOX_UINT8 *)&zenglfs_cache_inode, 0, sizeof(ZLOX_ZLFS_CACHE_INODE));
	zenglfs_super_block_isDirty = ZLOX_FALSE;
	zenglfs_super_block_lba = 0;
	if(zenglfs_cache_inode_blk.ptr != ZLOX_NULL)
	{
		zlox_kfree(zenglfs_cache_inode_blk.ptr);
		zlox_memset((ZLOX_UINT8 *)&zenglfs_cache_inode_blk, 0, sizeof(ZLOX_ZLFS_CACHE_INODE_BLK));
	}
	if(zenglfs_cache_group.ptr != ZLOX_NULL)
	{
		zlox_kfree(zenglfs_cache_group.ptr);
		zlox_memset((ZLOX_UINT8 *)&zenglfs_cache_group, 0, sizeof(ZLOX_ZLFS_CACHE_GROUP));
	}
	if(zenglfs_cache_blkmap.ptr != ZLOX_NULL)
	{
		zlox_kfree(zenglfs_cache_blkmap.ptr);
		zlox_memset((ZLOX_UINT8 *)&zenglfs_cache_blkmap, 0, sizeof(ZLOX_ZLFS_CACHE_BLKMAP));
	}
	if(zenglfs_cache_inode_map.ptr != ZLOX_NULL)
	{
		zlox_kfree(zenglfs_cache_inode_map.ptr);
		zlox_memset((ZLOX_UINT8 *)&zenglfs_cache_inode_map, 0, sizeof(ZLOX_ZLFS_CACHE_INODEMAP));
	}
	if(zenglfs_cache_singly.ptr != ZLOX_NULL)
	{
		zlox_kfree(zenglfs_cache_singly.ptr);
		zlox_memset((ZLOX_UINT8 *)&zenglfs_cache_singly, 0, sizeof(ZLOX_ZLFS_CACHE_SINGLY));
	}
	if(zenglfs_cache_doubly.ptr != ZLOX_NULL)
	{
		zlox_kfree(zenglfs_cache_doubly.ptr);
		zlox_memset((ZLOX_UINT8 *)&zenglfs_cache_doubly, 0, sizeof(ZLOX_ZLFS_CACHE_DOUBLY));
	}
	if(zenglfs_cache_triply.ptr != ZLOX_NULL)
	{
		zlox_kfree(zenglfs_cache_triply.ptr);
		zlox_memset((ZLOX_UINT8 *)&zenglfs_cache_triply, 0, sizeof(ZLOX_ZLFS_CACHE_TRIPLY));
	}
	return zenglfs_root;
}

// 挂载硬盘分区到hd目录下
ZLOX_FS_NODE * zlox_mount_zenglfs(ZLOX_UINT32 ide_index, ZLOX_UINT32 pt)
{
	ZLOX_UINT8 *buffer = (ZLOX_UINT8 *)zlox_kmalloc(ZLOX_ATA_SECTOR_SIZE * 2);
	ZLOX_UINT32 lba = 0; // MBR
	if(zenglfs_super_block.sign != 0 || zenglfs_root != ZLOX_NULL)
	{
		zlox_monitor_write("hd has been mounted , you must unmount it first , then mount it again \n");
		zlox_kfree(buffer);
		return ZLOX_NULL;
	}

	if(pt < 1 || pt > 4)
	{
		zlox_monitor_write("partition number must in 1 to 4 \n");
		zlox_kfree(buffer);
		return ZLOX_NULL;
	}
	ZLOX_IDE_DEVICE * ide_devices = (ZLOX_IDE_DEVICE *)zlox_ata_get_ide_info();
	if((ide_index > 3) || (ide_devices[ide_index].Reserved == 0))
	{
		zlox_monitor_write("\nide_index [");
		zlox_monitor_write_dec(ide_index);
		zlox_monitor_write("] has no drive!\n");
		zlox_kfree(buffer);
		return ZLOX_NULL;
	}
	else if(ide_devices[ide_index].Type == ZLOX_IDE_ATAPI)
	{
		zlox_monitor_write("\natapi drive can't write data now! ide_index [");
		zlox_monitor_write_dec(ide_index);
		zlox_monitor_write("]\n");
		zlox_kfree(buffer);
		return ZLOX_NULL;
	}

	ZLOX_SINT32 ata_ret = zlox_ide_ata_access(ZLOX_IDE_ATA_READ, ide_index, lba, 1, buffer); // read MBR
	if(ata_ret == -1)
	{
		zlox_monitor_write("\nata read sector failed for ide index [");
		zlox_monitor_write_dec(ide_index);
		zlox_monitor_write("]\n");
		zlox_kfree(buffer);
		return ZLOX_NULL;
	}

	ZLOX_MBR_PT * partition_ptr = (ZLOX_MBR_PT *)(buffer + ZLOX_MBR_PT_START);
	partition_ptr += (pt - 1);
	ZLOX_MBR_PT partition = (*partition_ptr);
	if(partition.fs_type != ZLOX_MBR_FS_TYPE_ZENGLFS)
	{
		zlox_monitor_write("this partition is not zenglfs \n");
		zlox_kfree(buffer);
		return ZLOX_NULL;
	}
	lba = partition.startLBA + 2;
	zenglfs_super_block_lba = lba;
	zlox_ide_ata_access(ZLOX_IDE_ATA_READ, ide_index, lba, 2, buffer); // read superblock
	ZLOX_ZLFS_SUPER_BLOCK * superblock_ptr = (ZLOX_ZLFS_SUPER_BLOCK *)buffer;
	if(superblock_ptr->sign != ZLOX_ZLFS_SUPER_BLOCK_SIGN)
	{
		zlox_monitor_write("this partition is not formated \n");
		zlox_kfree(buffer);
		return ZLOX_NULL;
	}
	zenglfs_super_block = (*superblock_ptr);
	zenglfs_root = (ZLOX_FS_NODE *)zlox_kmalloc(sizeof(ZLOX_FS_NODE));
	zlox_memset((ZLOX_UINT8 *)zenglfs_root, 0 , sizeof(ZLOX_FS_NODE));
	zlox_strcpy(zenglfs_root->name, "hd");
	zenglfs_root->mask = zenglfs_root->uid = zenglfs_root->gid = zenglfs_root->length = 0;
	zenglfs_root->inode = 1;
	zenglfs_root->flags = ZLOX_FS_DIRECTORY;
	zenglfs_root->read = 0;
	zenglfs_root->write = 0;
	zenglfs_root->open = 0;
	zenglfs_root->close = 0;
	zenglfs_root->readdir = &zlox_zenglfs_readdir;
	zenglfs_root->writedir = &zlox_zenglfs_writedir;
	zenglfs_root->finddir = &zlox_zenglfs_finddir;
	zenglfs_root->ptr = 0;
	zenglfs_root->impl = 0;

	zenglfs_ide_index = ide_index;
	zlox_kfree(buffer);
	return zenglfs_root;
}

