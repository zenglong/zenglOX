/* format -- 磁盘格式化工具 */

#include "common.h"
#include "syscall.h"
#include "ata.h"
#include "kheap.h"
#include "fdisk.h"
#include "format.h"

// Macros used in the bitset algorithms.
#define INDEX_FROM_BIT(a) (a/(8*4))
#define OFFSET_FROM_BIT(a) (a%(8*4))

/*将命令行参数解析到FORMAT_PARAM结构体中*/
int parse_param(FORMAT_PARAM * param, int argc, char * argv[])
{
	int i;
	for(i=0;i < argc;i++)
	{
		if(strcmp(argv[i],"-hd")==0)
		{
			if((i+1) < argc && strIsNum(argv[i+1]))
			{
				param->hd = (UINT8)strToUInt(argv[i+1]);
				if(param->hd > 3)
				{
					syscall_monitor_write("invalid -hd param, hd must be a valid ide_index \n");
					return -1;
				}
				param->setHD = TRUE;
			}
			else
			{
				syscall_monitor_write("invalid -hd param\n");
				return -1;
			}
		}
		else if(strcmp(argv[i],"-pt")==0)
		{
			if((i+1) < argc && strIsNum(argv[i+1]))
			{
				param->pt = (UINT8)strToUInt(argv[i+1]);
				if(param->pt < 1 || param->pt > 4)
				{
					syscall_monitor_write("invalid -pt param, pt must in 1 to 4 \n");
					return -1;
				}
				param->setPT = TRUE;
			}
			else
			{
				syscall_monitor_write("invalid -pt param\n");
				return -1;
			}
		}
		else if(strcmp(argv[i],"-type")==0)
		{
			if((i+1) < argc)
			{
				if(strcmp(argv[i+1],"zenglfs")==0)
					param->type = MBR_FS_TYPE_ZENGLFS;
				else
				{
					syscall_monitor_write("invalid -type param, type now only support zenglfs \n");
					return -1;
				}
				param->setTYPE = TRUE;
			}
			else
			{
				syscall_monitor_write("invalid -type param\n");
				return -1;
			}
		}
	}
	
	if(param->setHD == FALSE ||
		param->setPT == FALSE ||
		param->setTYPE == FALSE)
	{
		syscall_monitor_write("usage: format [-l ide_index ptnum [show_group_info_num]][-hd ide_index -pt ptnum -type fstype] \n");
		return -1;
	}
	return 0;
}

// Static function to set a bit in the bitset
static VOID set_bitmap(UINT32 * blocksmap, UINT32 block)
{
	UINT32 idx = INDEX_FROM_BIT(block);
	UINT32 off = OFFSET_FROM_BIT(block);
	blocksmap[idx] |= (0x1 << off);
}

/*
通过format工具可以对fdisk工具所分的区进行格式化操作，
可以使用类似如下的命令:
format -hd 0 -pt 1 -type zenglfs
该命令表示将0号IDE硬盘的1号分区格式化为zenglfs文件系统
如果格式化成功，则可以使用format -l命令来查看格式化后的
zenglfs文件系统的相关信息:
zenglOX> format -l 0 1
super block info:
..................
Group Info:
..................
如果不想显示Group Info信息，或者想控制Group Info所显示的Group信息数量，
则可以在末尾再添加一个数字，例如:
format -l 0 1 0 最后一个0表示显示的Group信息数为0,即不显示Group Info信息。

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

另外，1号文件节点是root根目录文件节点，在format格式化时，会对其进行简单的设置
*/
int main(VOID * task, int argc, char * argv[])
{
	UNUSED(task);

	if(argc == 4 || argc == 5)
	{
		if(strcmp(argv[1],"-l")==0)
		{
			UINT32 ide_index = strToUInt(argv[2]);
			UINT32 pt = strToUInt(argv[3]);
			if(pt < 1 || pt > 4)
			{
				syscall_monitor_write("partition number must in 1 to 4 \n");
				return -1;
			}
			UINT32 lba = 0; // MBR
			IDE_DEVICE * ide_devices = (IDE_DEVICE *)syscall_ata_get_ide_info();
			if((ide_index > 3) || (ide_devices[ide_index].Reserved == 0))
			{
				syscall_monitor_write("\nide_index [");
				syscall_monitor_write_dec(ide_index);
				syscall_monitor_write("] has no drive!\n");
				return -1;
			}
			else if(ide_devices[ide_index].Type == IDE_ATAPI)
			{
				syscall_monitor_write("\natapi drive can't write data now! ide_index [");
				syscall_monitor_write_dec(ide_index);
				syscall_monitor_write("]\n");
				return -1;
			}

			UINT8 *buffer = (UINT8 *)syscall_umalloc(ATA_SECTOR_SIZE * 2);
			SINT32 ata_ret = syscall_ide_ata_access(IDE_ATA_READ, ide_index, lba, 1, buffer); // read MBR
			if(ata_ret == -1)
			{
				syscall_monitor_write("\nata read sector failed for ide index [");
				syscall_monitor_write_dec(ide_index);
				syscall_monitor_write("]\n");
				syscall_ufree(buffer);
				return -1;
			}

			MBR_PT * partition_ptr = (MBR_PT *)(buffer + MBR_PT_START);
			partition_ptr += (pt - 1);
			MBR_PT partition = (*partition_ptr);
			lba = partition.startLBA + 2;
			syscall_ide_ata_access(IDE_ATA_READ, ide_index, lba, 2, buffer); // read superblock
			SUPER_BLOCK * superblock = (SUPER_BLOCK *)buffer;
			if(superblock->sign != SUPER_BLOCK_SIGN)
			{
				syscall_monitor_write("this partition is not formated \n");
				syscall_ufree(buffer);
				return -1;
			}
			syscall_monitor_write("super block info: \n");
			syscall_monitor_write("startLBA: ");
			syscall_monitor_write_dec(superblock->startLBA);
			syscall_monitor_write(" TotalBlock: ");
			syscall_monitor_write_dec(superblock->TotalBlock);
			syscall_monitor_write(" TotalInode: ");
			syscall_monitor_write_dec(superblock->TotalInode);
			syscall_monitor_write("\nGroupAddr: ");
			syscall_monitor_write_dec(superblock->GroupAddr);
			syscall_monitor_write(" GroupCount: ");
			syscall_monitor_write_dec(superblock->GroupCount);
			syscall_monitor_write(" GroupBlocks: ");
			syscall_monitor_write_dec(superblock->GroupBlocks);
			syscall_monitor_write("\nBlockBitMapBlockAddr: ");
			syscall_monitor_write_dec(superblock->BlockBitMapBlockAddr);
			syscall_monitor_write(" BlockMapBlocks: ");
			syscall_monitor_write_dec(superblock->BlockMapBlocks);
			syscall_monitor_write("\nInodeBitMapBlockAddr: ");
			syscall_monitor_write_dec(superblock->InodeBitMapBlockAddr);
			syscall_monitor_write(" InodeMapBlocks: ");
			syscall_monitor_write_dec(superblock->InodeMapBlocks);
			syscall_monitor_write("\nInodeTableBlockAddr: ");
			syscall_monitor_write_dec(superblock->InodeTableBlockAddr);
			syscall_monitor_write("\nallocBlocks: ");
			syscall_monitor_write_dec(superblock->allocBlocks);
			syscall_monitor_write(" allocInodes: ");
			syscall_monitor_write_dec(superblock->allocInodes);
			syscall_monitor_put('\n');
			UINT32 i, j , startLBA =  superblock->startLBA , GroupAddr = superblock->GroupAddr , 
					GroupCount = superblock->GroupCount, 
					GroupBlocks = superblock->GroupBlocks;
			UINT32 c = GroupCount, gsz = (1024 / sizeof(GROUP_INFO));
			GROUP_INFO * groupinfo;
			UINT32 a = 0, totala;
			if(argc == 5)
				totala = strToUInt(argv[4]);
			else
				totala = GroupCount;
			if(totala != 0)
				syscall_monitor_write("Groups Info: \n");
			for(i = 0; i < GroupBlocks;i++)
			{
				lba = (GroupAddr + i) * 2 + startLBA;
				syscall_ide_ata_access(IDE_ATA_READ, ide_index, lba, 2, buffer);
				groupinfo = (GROUP_INFO *)buffer;
				if(c == 0)
					break;
				for(j=0;c > 0 && j < gsz && a < totala;j++,groupinfo++,c--,a++)
				{
					if(groupinfo->allocBlocks != 0 || groupinfo->allocInodes != 0)
					{
						syscall_monitor_write("[");
						syscall_monitor_write_dec(i * gsz + j);
						syscall_monitor_write("] ");
						syscall_monitor_write("allocBlocks: ");
						syscall_monitor_write_dec(groupinfo->allocBlocks);
						syscall_monitor_write(" allocInodes: ");
						syscall_monitor_write_dec(groupinfo->allocInodes);
						syscall_monitor_put('\n');
					}
				}
			}
			syscall_ufree(buffer);
			return 0;
		}
	}
	else if(argc == 7)
	{
		FORMAT_PARAM param = {0};
		if(parse_param(&param, argc, argv) == -1)
			return -1;
		UINT8 *buffer = (UINT8 *)syscall_umalloc(ATA_SECTOR_SIZE * 2);
		UINT32 ide_index = param.hd;
		UINT32 lba = 0; // MBR
		IDE_DEVICE * ide_devices = (IDE_DEVICE *)syscall_ata_get_ide_info();
		if((ide_index > 3) || (ide_devices[ide_index].Reserved == 0))
		{
			syscall_monitor_write("\nide_index [");
			syscall_monitor_write_dec(ide_index);
			syscall_monitor_write("] has no drive!\n");
			syscall_ufree(buffer);
			return -1;
		}
		else if(ide_devices[ide_index].Type == IDE_ATAPI)
		{
			syscall_monitor_write("\natapi drive can't write data now! ide_index [");
			syscall_monitor_write_dec(ide_index);
			syscall_monitor_write("]\n");
			syscall_ufree(buffer);
			return -1;
		}
		
		SINT32 ata_ret = syscall_ide_ata_access(IDE_ATA_READ, ide_index, lba, 1, buffer); // read MBR
		if(ata_ret == -1)
		{
			syscall_monitor_write("\nata read sector failed for ide index [");
			syscall_monitor_write_dec(ide_index);
			syscall_monitor_write("]\n");
			syscall_ufree(buffer);
			return -1;
		}

		MBR_PT * partition_ptr = (MBR_PT *)(buffer + MBR_PT_START);
		partition_ptr += (param.pt - 1);
		MBR_PT partition = (*partition_ptr);
		if(partition.secNum < 450)
		{
			syscall_monitor_write("this partition is too small , must at least have 450 sectors \n");
			syscall_ufree(buffer);
			return -1;
		}
		else if(partition.fs_type != MBR_FS_TYPE_ZENGLFS)
		{
			syscall_monitor_write("this partition is not zenglfs \n");
			syscall_ufree(buffer);
			return -1;
		}
		lba = partition.startLBA + 2;
		syscall_ide_ata_access(IDE_ATA_READ, ide_index, lba, 2, buffer); // read superblock
		SUPER_BLOCK * superblock_ptr = (SUPER_BLOCK *)buffer;
		superblock_ptr->sign = SUPER_BLOCK_SIGN;
		superblock_ptr->startLBA = partition.startLBA;
		superblock_ptr->TotalBlock = partition.secNum / 2;
		UINT32 group = superblock_ptr->TotalBlock / (1024 * 8) + ((superblock_ptr->TotalBlock % (1024 * 8) != 0) ? 1 : 0);
		superblock_ptr->TotalInode = group * 128 * 8;
		superblock_ptr->GroupAddr = 2;
		superblock_ptr->GroupCount = group;
		superblock_ptr->GroupBlocks = (group / 128) + ((group % 128 !=0) ? 1 : 0);
		superblock_ptr->BlockBitMapBlockAddr = superblock_ptr->GroupAddr + superblock_ptr->GroupBlocks;
		superblock_ptr->BlockMapBlocks = group;
		superblock_ptr->InodeBitMapBlockAddr = superblock_ptr->BlockBitMapBlockAddr + group;
		superblock_ptr->InodeMapBlocks = (group / 8) + ((group % 8 !=0) ? 1 : 0);
		superblock_ptr->InodeTableBlockAddr = superblock_ptr->InodeBitMapBlockAddr + (group / 8) + ((group % 8 !=0) ? 1 : 0);
		superblock_ptr->allocBlocks = 2 + superblock_ptr->GroupBlocks + superblock_ptr->BlockMapBlocks + 
						superblock_ptr->InodeMapBlocks + group * 128;
		superblock_ptr->allocInodes = 1;
		syscall_ide_ata_access(IDE_ATA_WRITE, ide_index, lba, 2, buffer);
		SUPER_BLOCK superblock = (*superblock_ptr);
		UINT32 i,j,t = superblock.allocBlocks, gsz = (1024 / sizeof(GROUP_INFO));
		GROUP_INFO * groupinfo;
		BOOL needClear = FALSE;
		memset(buffer, 0, ATA_SECTOR_SIZE * 2);
		for(i = 0;i < superblock.GroupBlocks;i++)
		{
			lba = (superblock.GroupAddr + i) * 2 + superblock.startLBA;
			groupinfo = (GROUP_INFO *)buffer;
			if(i == 0)
			{
				groupinfo->allocInodes = 1;
				needClear = TRUE;
			}
			if(t != 0)
			{
				for(j=0;t != 0 && j < gsz;j++,groupinfo++)
				{
					if(t >= 8192)
					{
						groupinfo->allocBlocks = 8192;
						t -= 8192;
					}
					else if(t < 8192)
					{
						groupinfo->allocBlocks = t;
						t = 0;
					}
				}
				needClear = TRUE;
			}
			syscall_ide_ata_access(IDE_ATA_WRITE, ide_index, lba, 2, buffer);
			if(needClear == TRUE)
			{
				memset(buffer, 0, ATA_SECTOR_SIZE * 2);
				needClear = FALSE;
			}
		}
		t = superblock.allocBlocks;
		for(i = 0;i < superblock.BlockMapBlocks;i++)
		{
			lba = (superblock.BlockBitMapBlockAddr + i) * 2 + superblock.startLBA;
			if(t >= 8192)
			{
				memset(buffer, 0xff, ATA_SECTOR_SIZE * 2);
				t -= 8192;
				needClear = TRUE;
			}
			else if(t > 0 && t < 8192)
			{
				for(j = 0;j < t;j++)
					set_bitmap((UINT32 *)buffer, j);
				t = 0;
				needClear = TRUE;
			}
			syscall_ide_ata_access(IDE_ATA_WRITE, ide_index, lba, 2, buffer);
			if(needClear == TRUE)
			{
				memset(buffer, 0, ATA_SECTOR_SIZE * 2);
				needClear = FALSE;
			}
		}
		for(i = 0;i < superblock.InodeMapBlocks;i++)
		{
			lba = (superblock.InodeBitMapBlockAddr + i) * 2 + superblock.startLBA;
			if(i == 0)
			{
				set_bitmap((UINT32 *)buffer, 0);
				needClear = TRUE;
			}
			syscall_ide_ata_access(IDE_ATA_WRITE, ide_index, lba, 2, buffer);
			if(needClear == TRUE)
			{
				memset(buffer, 0, ATA_SECTOR_SIZE * 2);
				needClear = FALSE;
			}
		}
		INODE_DATA * root_inode = (INODE_DATA *)buffer;
		root_inode->type = INODE_TYPE_DIRECTORY;
		lba = superblock.InodeTableBlockAddr * 2 + superblock.startLBA;
		syscall_ide_ata_access(IDE_ATA_WRITE, ide_index, lba, 2, buffer); // write root inode
		syscall_ufree(buffer);
		syscall_monitor_write("\nformat success , you can use \"format -l ide_index ptnum\" to see it! \n");
		return 0;
	}

	syscall_monitor_write("usage: format [-l ide_index ptnum [show_group_info_num]][-hd ide_index -pt ptnum -type fstype] \n");
	return 0;
}

