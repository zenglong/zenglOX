/* a tool relate to ata(hard disk) and atapi(cd-rom) device */

#include "common.h"
#include "syscall.h"
#include "ata.h"
#include "kheap.h"

int main(VOID * task, int argc, char * argv[])
{
	UNUSED(task);

	if(argc == 2)
	{
		if(strcmp(argv[1],"-s")==0)
		{
			IDE_DEVICE * ide_devices = (IDE_DEVICE *)syscall_ata_get_ide_info();
			// 4- Print Summary:
			for (int i = 0; i < 4; i++)
			{
				switch(i)
				{
				case 0:
					syscall_monitor_write("ata0-master(ide_index:0):");
					break;
				case 1:
					syscall_monitor_write("ata0-slave:(ide_index:1)");
					break;
				case 2:
					syscall_monitor_write("ata1-master:(ide_index:2)");
					break;
				case 3:
					syscall_monitor_write("ata1-slave:(ide_index:3)");
					break;
				}
				if(ide_devices[i].Reserved == 1)
				{
					if(ide_devices[i].Type == IDE_ATA)
						syscall_monitor_write(" ATA Drive ");
					else
						syscall_monitor_write(" ATAPI Drive ");
					if(ide_devices[i].Type == IDE_ATA && !(ide_devices[i].CommandSets & (1 << 26)))
					{
						syscall_monitor_write_dec(ide_devices[i].Size * ATA_SECTOR_SIZE / 1024 / 1024);
						syscall_monitor_write("MB - ");
					}
					else if(ide_devices[i].Type == IDE_ATAPI)
					{
						ATAPI_READ_CAPACITY readCapacity;
						SINT32 ata_ret = syscall_atapi_drive_read_capacity(i,(UINT8 *)&readCapacity);
						if(ata_ret == -1)
							syscall_monitor_write("read capacity failed!");
						else
						{
							syscall_monitor_write("last LBA is ");
							syscall_monitor_write_dec(readCapacity.lastLBA);
							syscall_monitor_write(" block Size is ");
							syscall_monitor_write_dec(readCapacity.blockSize);
						}
						syscall_monitor_write(" - ");
					}
					else
						syscall_monitor_write(" - ");
					syscall_monitor_write((CHAR *)ide_devices[i].Model);
					syscall_monitor_put('\n');
				}
				else
					syscall_monitor_write(" no Drive here \n");
			}
			return 0;
		}
		else if(strcmp(argv[1],"-h")==0)
		{
			syscall_monitor_write("usage: ata [-h][-s][-r ide_index lba show_num]\n"
				"ata -h: to show the help about ata's usage\n"
				"ata -s: to show the ide infos\n"
				"ata -r ide_index lba show_num: from ide_index device to get lba block and shows the data with show_num\n");
			return 0;
		}
	}
	else if(argc == 5 && strcmp(argv[1],"-r")==0)
	{
		UINT8 *buffer = (UINT8 *)syscall_umalloc(ATAPI_SECTOR_SIZE+5);
		UINT32 ide_index = strToUInt(argv[2]);
		UINT32 lba = strToUInt(argv[3]);
		UINT32 show_num = strToUInt(argv[4]);
		SINT32 ata_ret = syscall_atapi_drive_read_sector(ide_index,lba,buffer);
		if(ata_ret == -1)
		{
			IDE_DEVICE * ide_devices = (IDE_DEVICE *)syscall_ata_get_ide_info();
			if((ide_index > 3) || (ide_devices[ide_index].Reserved == 0))
			{
				syscall_monitor_write("\nide_index [");
				syscall_monitor_write_dec(ide_index);
				syscall_monitor_write("] has no drive!\n");
			}
			else if(ide_devices[ide_index].Type == IDE_ATA)
			{
				syscall_monitor_write("\nata now can only read from atapi device , ide_index [");
				syscall_monitor_write_dec(ide_index);
				syscall_monitor_write("] is an ata device\n");
			}
			else
			{
				syscall_monitor_write("\natapi read sector failed for ide index [");
				syscall_monitor_write_dec(ide_index);
				syscall_monitor_write("]\n");
			}
		}
		else
		{
			syscall_monitor_put('\n');
			for(UINT32 i=0;i< show_num;i++)
			{
				syscall_monitor_write_hex((UINT32)(buffer[i]));
				syscall_monitor_put(' ');
			}
			syscall_monitor_put('\n');
		}
		syscall_ufree(buffer);
		return 0;
	}

	syscall_monitor_write("usage: ata [-h][-s][-r ide_index lba show_num]");
	return 0;
}

