/* fdisk -- 磁盘分区工具 */

#include "common.h"
#include "syscall.h"
#include "ata.h"
#include "kheap.h"
#include "fdisk.h"

/*将命令行参数解析到FDISK_PARAM结构体中*/
int parse_param(FDISK_PARAM * param, int argc, char * argv[])
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
					syscall_cmd_window_write("invalid -hd param, hd must be a valid ide_index \n");
					return -1;
				}
				param->setHD = TRUE;
			}
			else
			{
				syscall_cmd_window_write("invalid -hd param\n");
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
					syscall_cmd_window_write("invalid -pt param, pt must in 1 to 4 \n");
					return -1;
				}
				param->setPT = TRUE;
			}
			else
			{
				syscall_cmd_window_write("invalid -pt param\n");
				return -1;
			}
		}
		else if(strcmp(argv[i],"-type")==0)
		{
			if((i+1) < argc)
			{
				if(strcmp(argv[i+1],"zenglfs")==0)
					param->type = MBR_FS_TYPE_ZENGLFS;
				else if(strIsNum(argv[i+1]))
					param->type = (UINT8)strToUInt(argv[i+1]);
				else
				{
					syscall_cmd_window_write("invalid -type param, type now only support zenglfs or you can set number\n");
					return -1;
				}
				param->setTYPE = TRUE;
			}
			else
			{
				syscall_cmd_window_write("invalid -type param\n");
				return -1;
			}
		}
		else if(strcmp(argv[i],"-start")==0)
		{
			if((i+1) < argc && strIsNum(argv[i+1]))
			{
				param->start = strToUInt(argv[i+1]);
				param->setSTART = TRUE;
			}
			else
			{
				syscall_cmd_window_write("invalid -start param\n");
				return -1;
			}
		}
		else if(strcmp(argv[i],"-num")==0)
		{
			if((i+1) < argc && strIsNum(argv[i+1]))
			{
				param->num = strToUInt(argv[i+1]);
				param->setNUM = TRUE;
			}
			else
			{
				syscall_cmd_window_write("invalid -num param\n");
				return -1;
			}
		}
	}
	
	if(param->setHD == FALSE ||
		param->setPT == FALSE ||
		param->setTYPE == FALSE ||
		param->setSTART == FALSE ||
		param->setNUM == FALSE)
	{
		syscall_cmd_window_write("usage: fdisk [-l][-hd ide_index -pt ptnum -type fstype -start startLBA -num secNum]\n");
		return -1;
	}
	return 0;
}

/*对用户所设置的分区的起始扇区号，分区总扇区数进行检测，
当所设置的分区信息会与其他分区冲突时，
比如设置的分区位于其他的分区内，则返回-1表示参数检测不通过*/
int check_partition(MBR_PT * cur_pt, MBR_PT * start_pt)
{
	int i;
	MBR_PT * pt = start_pt;
	if(cur_pt->secNum == 0)
		return 0;

	for(i=0;i < 4;i++, pt++)
	{
		if(pt == cur_pt)
			continue;
		if(cur_pt->startLBA < pt->startLBA)
		{
			if((cur_pt->startLBA + cur_pt->secNum - 1) >= pt->startLBA)
			{
				syscall_cmd_window_write("your setting conflict with partition ");
				syscall_cmd_window_write_dec(i+1);
				syscall_cmd_window_write(" , please check your secNum\n");
				return -1;
			}
		}
		else
		{
			if(pt->secNum != 0 && cur_pt->startLBA <= (pt->startLBA + pt->secNum - 1))
			{
				syscall_cmd_window_write("your setting conflict with partition ");
				syscall_cmd_window_write_dec(i+1);
				syscall_cmd_window_write(" , please check your startLBA\n");
				return -1;
			}
		}
	}
	return 0;
}

/*
我们可以使用类似fdisk -l 0的命令来查看0号ide硬盘里的分区信息，
原理就是通过读取该硬盘的0号扇区(也就是硬盘的第一个扇区, 也可以称之为MBR)，
对MBR里的分区表进行分析，就可以显示出每个分区的信息了。
要设置分区的话，可以使用类似如下的命令:
fdisk -hd 0 -pt 1 -type zenglfs -start 8 -num 123456
该命令表示将0号IDE硬盘的1号分区设置为zenglfs文件系统，该分区的
起始扇区号为8，分区拥有的扇区数为123456(相当于60M)
在执行该命令时，fdisk工具会通过check_partition函数来检测你设置的信息是否有误，
比如当-num设置的扇区数超过磁盘的总扇区数时，等等。
如果参数没问题，在fdisk将分区信息写入到MBR后，通过fdisk -l 0就可以看到类似如下的输出
zenglOX> fdisk -l 0
[1] start LBA: 8 Total Sectors: 123456 [60M] filesystem: zenglfs
[2] .............
[3] .............
[4] .............

如果要删除1号分区，则可以使用类似如下的命令:
fdisk -hd 0 -pt 1 -type 0 -start 0 -num 0
其实就是将相关信息设置为0，再使用fdisk -l 0就可以看到该分区被删除了

fdisk工具只是对MBR里的分区表进行写入操作，要使用该分区，还必须使用
format工具来格式化该分区，格式化好后，最后再通过mount工具加载分区

MBR的结构可以参考 http://wiki.osdev.org/MBR_%28x86%29
MBR里的分区表结构则可以参考 http://wiki.osdev.org/Partition_Table
*/
int main(VOID * task, int argc, char * argv[])
{
	UNUSED(task);
	
	if(argc == 3)
	{
		if(strcmp(argv[1],"-l")==0)
		{
			UINT32 ide_index = strToUInt(argv[2]);
			UINT32 lba = 0; // MBR
			UINT8 * buffer = (UINT8 *)syscall_umalloc(ATA_SECTOR_SIZE+5);
			IDE_DEVICE * ide_devices = (IDE_DEVICE *)syscall_ata_get_ide_info();
			if((ide_index > 3) || (ide_devices[ide_index].Reserved == 0))
			{
				syscall_cmd_window_write("\nide_index [");
				syscall_cmd_window_write_dec(ide_index);
				syscall_cmd_window_write("] has no drive!\n");
				syscall_ufree(buffer);
				return -1;
			}
			else if(ide_devices[ide_index].Type == IDE_ATAPI)
			{
				syscall_cmd_window_write("\natapi drive can't write data now! ide_index [");
				syscall_cmd_window_write_dec(ide_index);
				syscall_cmd_window_write("]\n");
				syscall_ufree(buffer);
				return -1;
			}
			
			/*
			通过syscall_ide_ata_access系统调用来读取0号扇区(MBR)的
			数据到buffer缓冲里，lba(逻辑块地址)在之前
			已经被设置为了0
			*/
			SINT32 ata_ret = syscall_ide_ata_access(0, ide_index, lba, 1, buffer); // read MBR
			if(ata_ret == -1)
			{
				syscall_cmd_window_write("\nata read sector failed for ide index [");
				syscall_cmd_window_write_dec(ide_index);
				syscall_cmd_window_write("]\n");
				syscall_ufree(buffer);
				return -1;
			}
			/*
			将buffer(即MBR缓冲)里的分区表的起始位置赋值给partition指针变量
			*/
			MBR_PT * partition = (MBR_PT *)(buffer + MBR_PT_START);
			SINT32 i;
			/*
			通过for循环，将4个分区表的信息给依次显示出来
			*/
			for(i=0;i < 4;i++,partition++)
			{
				syscall_cmd_window_write("[");
				syscall_cmd_window_write_dec(i+1);
				syscall_cmd_window_write("] start LBA: ");
				syscall_cmd_window_write_dec(partition->startLBA);
				syscall_cmd_window_write(" Total Sectors: ");
				syscall_cmd_window_write_dec(partition->secNum);
				if(partition->secNum != 0)
				{
					syscall_cmd_window_write(" [");
					/*
					通过分区的总扇区数来计算出分区的尺寸大小，
					并根据尺寸大小来设置具体的单位，比如MB，
					KB以及Byte
					*/
					if(partition->secNum * 512 / 1024 / 1024 != 0)
					{
						syscall_cmd_window_write_dec(partition->secNum * 512 / 1024 / 1024);
						syscall_cmd_window_write("MB]");
					}
					else if(partition->secNum * 512 / 1024 != 0)
					{
						syscall_cmd_window_write_dec(partition->secNum * 512 / 1024);
						syscall_cmd_window_write("KB]");
					}
					else
					{
						syscall_cmd_window_write_dec(partition->secNum * 512);
						syscall_cmd_window_write("Byte]");
					}
				}
				syscall_cmd_window_write(" filesystem: ");
				/*
				根据分区表的System ID字段即下面的
				partition->fs_type成员的值来判断分区所设置
				的文件系统类型，zenglfs文件系统的类型值为0x23，
				即下面MBR_FS_TYPE_ZENGLFS宏所对应的值。如果fs_type
				类型值为MBR_FS_TYPE_EMPTY即0的话，就说明该分区表是
				empty(空的)。其他的文件系统类型值，目前统一显示为other，
				以表示zenglOX目前不能识别和处理的分区类型。
				*/
				switch(partition->fs_type)
				{
				case MBR_FS_TYPE_ZENGLFS:
					syscall_cmd_window_write(" zenglfs");
					break;
				case MBR_FS_TYPE_EMPTY:
					syscall_cmd_window_write(" empty");
					break;
				default:
					syscall_cmd_window_write(" other");
					break;
				}
				syscall_cmd_window_write("\n");
			}

			syscall_ufree(buffer);
			return 0;
		}
	}
	else if(argc == 11)
	{
		FDISK_PARAM param = {0};
		/*
		通过parse_param函数，将用户的参数设置到param结构体变量中，
		例如，start参数的值会被设置到param.start成员，
		num参数的值会被设置到param.num成员
		*/
		if(parse_param(&param, argc, argv) == -1)
			return -1;
		UINT8 *buffer = (UINT8 *)syscall_umalloc(ATA_SECTOR_SIZE+5);
		UINT32 ide_index = param.hd;
		UINT32 lba = 0; // MBR
		IDE_DEVICE * ide_devices = (IDE_DEVICE *)syscall_ata_get_ide_info();
		if((ide_index > 3) || (ide_devices[ide_index].Reserved == 0))
		{
			syscall_cmd_window_write("\nide_index [");
			syscall_cmd_window_write_dec(ide_index);
			syscall_cmd_window_write("] has no drive!\n");
			syscall_ufree(buffer);
			return -1;
		}
		else if(ide_devices[ide_index].Type == IDE_ATAPI)
		{
			syscall_cmd_window_write("\natapi drive can't write data now! ide_index [");
			syscall_cmd_window_write_dec(ide_index);
			syscall_cmd_window_write("]\n");
			syscall_ufree(buffer);
			return -1;
		}

		/*
		通过syscall_ide_ata_access系统调用来读取0号扇区(MBR)的
		数据到buffer缓冲里，lba(逻辑块地址)在之前
		已经被设置为了0
		*/
		SINT32 ata_ret = syscall_ide_ata_access(0, ide_index, lba, 1, buffer); // read MBR
		if(ata_ret == -1)
		{
			syscall_cmd_window_write("\nata read sector failed for ide index [");
			syscall_cmd_window_write_dec(ide_index);
			syscall_cmd_window_write("]\n");
			syscall_ufree(buffer);
			return -1;
		}

		MBR_PT * partition = (MBR_PT *)(buffer + MBR_PT_START);
		/*
		通过用户设置的pt参数来定位到buffer缓冲中
		的指定分区
		*/
		partition += (param.pt - 1);
		/*
		将不需要的flag(分区引导标志)字段设置为0，
		以及将不需要的head(起始磁头号),sec_cyl(起始柱面号)等
		设置为0xff(二进制位的值全部设置为1，以表示无效的值)，
		fs_type即文件系统类型字段设置为用户type参数所指定的值，
		startLBA即起始逻辑块地址字段设置为用户start参数所指定
		的值，secNum即分区拥有的总扇区数字段设置为用户num参数
		所指定的值。
		*/
		partition->flag = 0;
		partition->head = partition->end_head = (param.num !=0) ? 0xff : 0;
		partition->sec_cyl = partition->end_sec_cyl = (param.num !=0) ? 0xffff : 0;
		partition->fs_type = param.type;
		partition->startLBA = param.start;
		partition->secNum = param.num;

		/*
		对用户设置的起始扇区号进行必要的调整，让其按照2个扇区
		进行对齐，因为zenglOX里使用的逻辑块都是2个磁盘扇区的大小
		*/
		partition->startLBA = (partition->startLBA % 2 == 0) ? partition->startLBA : 
						(partition->startLBA + 1);
		partition->secNum = (partition->secNum % 2 == 0) ? partition->secNum : 
						(partition->secNum + 1);

		/*
		对用户设置的参数进行必要的检测，不符合要求的设置则返回错误信息。
		*/
		if(partition->secNum != 0 && partition->startLBA == 0)
			partition->startLBA = 2;
		if(partition->secNum != 0 && 
			(partition->startLBA + partition->secNum - 1) > ide_devices[ide_index].Size)
		{
			syscall_cmd_window_write("your startLBA + secNum out of range, ide_index [");
			syscall_cmd_window_write_dec(ide_index);
			syscall_cmd_window_write("] Last LBA is ");
			syscall_cmd_window_write_dec(ide_devices[ide_index].Size);
			syscall_cmd_window_put('\n');
			syscall_ufree(buffer);
			return -1;
		}
		if(check_partition(partition, (MBR_PT *)(buffer + MBR_PT_START)) == -1)
		{
			syscall_ufree(buffer);
			return -1;
		}

		/*
		最后通过syscall_ide_ata_access系统调用将buffer缓冲区里设置过
		的分区表信息写入到硬盘的MBR里。
		*/
		ata_ret = syscall_ide_ata_access(1, ide_index, lba, 1, buffer);
		if(ata_ret == -1)
		{
			syscall_cmd_window_write("\nata write sector failed for ide index [");
			syscall_cmd_window_write_dec(ide_index);
			syscall_cmd_window_write("]\n");
			syscall_ufree(buffer);
			return -1;
		}
		else
			syscall_cmd_window_write("\nfdisk write MBR success , you can use \"fdisk -l ide_index\" to see it! \n");

		syscall_ufree(buffer);
		return 0;
	}

	syscall_cmd_window_write("usage: fdisk [-l ide_index][-hd ide_index -pt ptnum -type fstype -start startLBA -num secNum]\n");
	return 0;
}

