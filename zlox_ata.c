/*zlox_ata.c Define some functions relate to ata*/

#include "zlox_ata.h"
#include "zlox_task.h"
#include "zlox_isr.h"
#include "zlox_monitor.h"
#include "zlox_kheap.h"
#include "zlox_ide.h"

extern ZLOX_TASK * current_task;
volatile ZLOX_TASK * ata_wait_task = ZLOX_NULL;
extern ZLOX_BOOL ide_can_dma;

ZLOX_IDE_DEVICE ide_devices[4];

// Read/Write From ATA Drive, direction == 0 is read, direction == 1 is write
ZLOX_SINT32 zlox_ide_ata_access(ZLOX_UINT8 direction, ZLOX_UINT8 ide_index, ZLOX_UINT32 lba, 
                             ZLOX_UINT8 numsects, ZLOX_UINT8 * buffer)
{
	ZLOX_UINT8 lba_mode /* 0: CHS, 1:LBA28, 2: LBA48 */, cmd;
	ZLOX_UINT8 lba_io[6];
	ZLOX_UINT32 words = 256; // Almost every ATA drive has a sector-size of 512-byte.
	ZLOX_UINT32 bus, drive;
	ZLOX_UINT16 cyl, i;
	ZLOX_UINT8 head, sect, control_orig, status;

	if(ide_index > 3)
		return -1;
	if((ide_devices[ide_index].Reserved == 0) || (ide_devices[ide_index].Type == ZLOX_IDE_ATAPI))
		return -1;
	if(lba > ide_devices[ide_index].Size)
		return -1;

	bus = ide_devices[ide_index].Bus;
	drive = ide_devices[ide_index].Drive;

	control_orig = zlox_inb(ZLOX_ATA_DCR(bus));
	zlox_outb(ZLOX_ATA_DCR(bus), control_orig & ((~2) & 0xff));
	
	// (I) Select one from LBA48, LBA28 or CHS;
	if(lba >= 0x10000000)
	{
		// if not support LBA48 , then return -1
		if(!(ide_devices[ide_index].CommandSets & (1 << 26)))
			return -1;

		// LBA48:
		lba_mode  = 2;
		lba_io[0] = (lba & 0x000000FF) >> 0;
		lba_io[1] = (lba & 0x0000FF00) >> 8;
		lba_io[2] = (lba & 0x00FF0000) >> 16;
		lba_io[3] = (lba & 0xFF000000) >> 24;
		lba_io[4] = 0; // lba is integer, so 32-bits are enough to access 2TB.
		lba_io[5] = 0; // lba is integer, so 32-bits are enough to access 2TB.
		head      = 0; // Lower 4-bits of HDDEVSEL are not used here.
	}
	else if (ide_devices[ide_index].Capabilities & 0x200)
	{
		// Drive supports LBA?
		// LBA28:
		lba_mode  = 1;
		lba_io[0] = (lba & 0x00000FF) >> 0;
		lba_io[1] = (lba & 0x000FF00) >> 8;
		lba_io[2] = (lba & 0x0FF0000) >> 16;
		lba_io[3] = 0; // These Registers are not used here.
		lba_io[4] = 0; // These Registers are not used here.
		lba_io[5] = 0; // These Registers are not used here.
		head      = (lba & 0xF000000) >> 24;
	} 
	else 
	{
		// CHS:
		lba_mode  = 0;
		sect      = (lba % 63) + 1;
		cyl       = (lba + 1  - sect) / (16 * 63);
		lba_io[0] = sect;
		lba_io[1] = (cyl >> 0) & 0xFF;
		lba_io[2] = (cyl >> 8) & 0xFF;
		lba_io[3] = 0;
		lba_io[4] = 0;
		lba_io[5] = 0;
		head      = (lba + 1  - sect) % (16 * 63) / (63); // Head number is written to HDDEVSEL lower 4-bits.
	}

	// (II) Wait if the drive is busy;
	while ((status = zlox_inb (ZLOX_ATA_COMMAND (bus))) & 0x80)     /* BUSY */
		asm volatile ("pause");

	// (III) Select Drive from the controller;
	if (lba_mode == 0)
		zlox_outb(ZLOX_ATA_DRIVE_SELECT (bus), ((drive & (1 << 4)) | 0xA0 | head)); // Drive & CHS.
	else
		zlox_outb(ZLOX_ATA_DRIVE_SELECT (bus), ((drive & (1 << 4)) | 0xE0 | head)); // Drive & LBA

	// (IV) Write Parameters;
	if (lba_mode == 2) {
		// enable HOB(high order byte) of control register
		zlox_outb(ZLOX_ATA_DCR(bus), zlox_inb(ZLOX_ATA_DCR(bus)) | 0x80);
		zlox_outb(ZLOX_ATA_SECTOR_COUNT(bus), 0);
		zlox_outb(ZLOX_ATA_ADDRESS1(bus), lba_io[3]);
		zlox_outb(ZLOX_ATA_ADDRESS2(bus), lba_io[4]);
		zlox_outb(ZLOX_ATA_ADDRESS3(bus), lba_io[5]);
		// disable HOB
		zlox_outb(ZLOX_ATA_DCR(bus), zlox_inb(ZLOX_ATA_DCR(bus)) & 0x7f);
	}
	zlox_outb(ZLOX_ATA_SECTOR_COUNT(bus), numsects);
	zlox_outb(ZLOX_ATA_ADDRESS1(bus), lba_io[0]);
	zlox_outb(ZLOX_ATA_ADDRESS2(bus), lba_io[1]);
	zlox_outb(ZLOX_ATA_ADDRESS3(bus), lba_io[2]);

	// (V) Select the command and send it;
	if (lba_mode == 0 && direction == 0)
		cmd = ide_can_dma ? 0xC8 : 0x20; // READ DMA OR READ SECTOR(S)
	if (lba_mode == 1 && direction == 0) 
		cmd = ide_can_dma ? 0xC8 : 0x20; // READ DMA OR READ SECTOR(S)
	if (lba_mode == 2 && direction == 0) 
		cmd = ide_can_dma ? 0x25 : 0x24; // READ DMA EXT or READ SECTOR(S) EXT command (support from ATA-ATAPI-6)
	if (lba_mode == 0 && direction == 1)
		cmd = ide_can_dma ? 0xCA : 0x30; // WRITE DMA or WRITE SECTOR(S) command
	if (lba_mode == 1 && direction == 1)
		cmd = ide_can_dma ? 0xCA : 0x30; // WRITE DMA or WRITE SECTOR(S) command
	if (lba_mode == 2 && direction == 1) 
		cmd = ide_can_dma ? 0x35 : 0x34; // WRITE DMA EXT or WRITE SECTOR(S) EXT command (support from ATA-ATAPI-6)

	if(ide_can_dma)
	{
		zlox_ide_before_send_command(direction, bus);
		if(direction == 1)
			zlox_ide_set_buffer_data(buffer, numsects * ZLOX_ATA_SECTOR_SIZE, bus);
	}

	zlox_outb (ZLOX_ATA_COMMAND (bus), cmd);

	if(ide_can_dma)
		zlox_ide_start_dma(bus);

	if(ide_can_dma)
	{
		// DMA Read.
		if (direction == 0)
		{
			if(zlox_ide_after_send_command(bus) == -1)
			{
				status = zlox_inb (ZLOX_ATA_COMMAND (bus));
				if (status & 0x1) {
					ZLOX_UINT8 error = zlox_inb (ZLOX_ATA_FEATURES (bus));
					zlox_monitor_write("ata error , error reg: ");
					zlox_monitor_write_hex((ZLOX_UINT32)error);
					zlox_monitor_write("\n");
					return -1;
				}
				return -1;
			}
			zlox_ide_get_buffer_data(buffer, numsects * ZLOX_ATA_SECTOR_SIZE, bus);
		}
		else // DMA Write.
		{
			if(zlox_ide_after_send_command(bus) == -1)
			{
				status = zlox_inb (ZLOX_ATA_COMMAND (bus));
				if (status & 0x1) {
					ZLOX_UINT8 error = zlox_inb (ZLOX_ATA_FEATURES (bus));
					zlox_monitor_write("ata error , error reg: ");
					zlox_monitor_write_hex((ZLOX_UINT32)error);
					zlox_monitor_write("\n");
					return -1;
				}
				return -1;
			}
		}
	}
	else
	{
		// PIO Read.
		if (direction == 0)
		{
			for (i = 0; i < numsects; i++) 
			{
				while ((status = zlox_inb (ZLOX_ATA_COMMAND (bus))) & 0x80)     /* BUSY */
					asm volatile ("pause");
				while (!((status = zlox_inb (ZLOX_ATA_COMMAND (bus))) & 0x8) && !(status & 0x1))
					asm volatile ("pause");
				/* DRQ or ERROR set */
				if (status & 0x1) {
					return -1;
				}
				/* Read data. */
				zlox_insw (ZLOX_ATA_DATA (bus), (ZLOX_UINT16 *)buffer, words);
				buffer += (words*2);
			}
		}
		else // PIO Write.
		{
			for (i = 0; i < numsects; i++) 
			{
				while ((status = zlox_inb (ZLOX_ATA_COMMAND (bus))) & 0x80)     /* BUSY */
					asm volatile ("pause");
				/* write data. */
				zlox_outsw (ZLOX_ATA_DATA (bus), (ZLOX_UINT16 *)buffer, words);
				buffer += (words*2);
			}
			zlox_outb (ZLOX_ATA_COMMAND (bus), (char []) {0xE7 /*FLUSH CACHE command*/,
		                0xE7 /*FLUSH CACHE command*/ ,
		                0xEA /*FLUSH CACHE EXT command*/}[lba_mode]);
			while ((status = zlox_inb (ZLOX_ATA_COMMAND (bus))) & 0x80)
				asm volatile ("pause");
		}
	}

	zlox_outb(ZLOX_ATA_DCR(bus), control_orig);
	return 0;
}

/* Use the ATAPI protocol to read a single sector from the given
 * ide_index into the buffer using logical block address lba. */
ZLOX_SINT32 zlox_atapi_drive_read_sector (ZLOX_UINT32 ide_index, ZLOX_UINT32 lba, ZLOX_UINT8 *buffer)
{
	/* 0xA8 is READ SECTORS command byte. */
	ZLOX_UINT8 read_cmd[12] = { 0xA8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	ZLOX_UINT8 status;
	ZLOX_SINT32 size;
	ZLOX_UINT32 bus, drive;
	if(ide_index > 3)
		return -1;
	if((ide_devices[ide_index].Reserved == 0) || (ide_devices[ide_index].Type == ZLOX_IDE_ATA))
		return -1;

	bus = ide_devices[ide_index].Bus;
	drive = ide_devices[ide_index].Drive;

	/* Select drive (only the slave-bit is set) */
	zlox_outb (ZLOX_ATA_DRIVE_SELECT (bus),(drive & (1 << 4)));
	ZLOX_ATA_SELECT_DELAY (bus);       /* 400ns delay */
	zlox_outb (ZLOX_ATA_FEATURES (bus),0x0);       /* PIO mode */
	zlox_outb (ZLOX_ATA_ADDRESS2 (bus), (ZLOX_ATAPI_SECTOR_SIZE & 0xFF));
	zlox_outb (ZLOX_ATA_ADDRESS3 (bus), (ZLOX_ATAPI_SECTOR_SIZE >> 8));
	zlox_outb (ZLOX_ATA_COMMAND (bus), 0xA0);       /* ATA PACKET command */
	while ((status = zlox_inb (ZLOX_ATA_COMMAND (bus))) & 0x80)     /* BUSY */
		asm volatile ("pause");
	while (!((status = zlox_inb (ZLOX_ATA_COMMAND (bus))) & 0x8) && !(status & 0x1))
		asm volatile ("pause");
	/* DRQ or ERROR set */
	if (status & 0x1) {
		size = -1;
		goto end;
	}
	read_cmd[9] = 1;              	/* 1 sector */
	read_cmd[2] = (lba >> 0x18) & 0xFF;   /* most sig. byte of LBA */
	read_cmd[3] = (lba >> 0x10) & 0xFF;
	read_cmd[4] = (lba >> 0x08) & 0xFF;
	read_cmd[5] = (lba >> 0x00) & 0xFF;   /* least sig. byte of LBA */

	/* Send ATAPI/SCSI command */
	zlox_outsw (ZLOX_ATA_DATA (bus), (ZLOX_UINT16 *) read_cmd, 6);
	/* Tell the scheduler that this process is using the ATA subsystem. */
	//current_task->status = ZLOX_TS_ATA_WAIT;
	//ata_wait_task = current_task;
	/* Wait for IRQ that says the data is ready. */
	//zlox_switch_task();

	while ((status = zlox_inb (ZLOX_ATA_COMMAND (bus))) & 0x80)     /* BUSY */
		asm volatile ("pause");
	while (!((status = zlox_inb (ZLOX_ATA_COMMAND (bus))) & 0x8) && !(status & 0x1))
		asm volatile ("pause");
	/* DRQ or ERROR set */
	if (status & 0x1) {
		size = -1;
		goto end;
	}

	/* Read actual size */
	size = (((ZLOX_SINT32) zlox_inb (ZLOX_ATA_ADDRESS3 (bus))) << 8) | 
		(ZLOX_SINT32) (zlox_inb (ZLOX_ATA_ADDRESS2 (bus)));
	/* This example code only supports the case where the data transfer
	* of one sector is done in one step. */
	ZLOX_ASSERT (size == ZLOX_ATAPI_SECTOR_SIZE);

	zlox_insw (ZLOX_ATA_DATA (bus), (ZLOX_UINT16 *)buffer, size / 2);
	/* Wait for BSY and DRQ to clear, indicating Command Finished */
	while ((status = zlox_inb (ZLOX_ATA_COMMAND (bus))) & 0x88)
		asm volatile ("pause");

end:
	if(ata_wait_task != ZLOX_NULL)
		ata_wait_task = ZLOX_NULL;
	return size;
}

ZLOX_SINT32 zlox_atapi_drive_read_sectors (ZLOX_UINT32 ide_index, ZLOX_UINT32 lba, ZLOX_UINT32 lba_num, 
					ZLOX_UINT8 *buffer)
{
	ZLOX_UINT32 i;
	ZLOX_SINT32 size = 0;
	for(i=0;i<lba_num;i++)
	{
		size += zlox_atapi_drive_read_sector(ide_index,(lba+i),buffer+(ZLOX_ATAPI_SECTOR_SIZE * i));

		zlox_isr_detect_proc_irq();
	}
	return size;
}

/* read capacity from the given ide_index into the buffer */
ZLOX_SINT32 zlox_atapi_drive_read_capacity (ZLOX_UINT32 ide_index, ZLOX_UINT8 *buffer)
{
	/* 0xA8 is READ SECTORS command byte. */
	ZLOX_UINT8 read_cmd[12] = { 0x25, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	ZLOX_UINT8 status;
	ZLOX_SINT32 size;
	ZLOX_UINT32 bus, drive;
	if(ide_index > 3)
		return -1;
	if((ide_devices[ide_index].Reserved == 0) || (ide_devices[ide_index].Type == ZLOX_IDE_ATA))
		return -1;

	bus = ide_devices[ide_index].Bus;
	drive = ide_devices[ide_index].Drive;

	/* Select drive (only the slave-bit is set) */
	zlox_outb (ZLOX_ATA_DRIVE_SELECT (bus),(drive & (1 << 4)));
	ZLOX_ATA_SELECT_DELAY (bus);       /* 400ns delay */
	zlox_outb (ZLOX_ATA_FEATURES (bus),0x0);       /* PIO mode */
	zlox_outb (ZLOX_ATA_ADDRESS2 (bus), (ZLOX_ATAPI_CAPACITY_SIZE & 0xFF));
	zlox_outb (ZLOX_ATA_ADDRESS3 (bus), (ZLOX_ATAPI_CAPACITY_SIZE >> 8));
	zlox_outb (ZLOX_ATA_COMMAND (bus), 0xA0);       /* ATA PACKET command */
	while ((status = zlox_inb (ZLOX_ATA_COMMAND (bus))) & 0x80)     /* BUSY */
		asm volatile ("pause");
	while (!((status = zlox_inb (ZLOX_ATA_COMMAND (bus))) & 0x8) && !(status & 0x1))
		asm volatile ("pause");
	/* DRQ or ERROR set */
	if (status & 0x1) {
		size = -1;
		goto end;
	}
	
	/* Send ATAPI/SCSI command */
	zlox_outsw (ZLOX_ATA_DATA (bus), (ZLOX_UINT16 *) read_cmd, 6);
	/* Tell the scheduler that this process is using the ATA subsystem. */
	//current_task->status = ZLOX_TS_ATA_WAIT;
	//ata_wait_task = current_task;
	/* Wait for IRQ that says the data is ready. */
	//zlox_switch_task();
	while ((status = zlox_inb (ZLOX_ATA_COMMAND (bus))) & 0x80)     /* BUSY */
		asm volatile ("pause");
	while (!((status = zlox_inb (ZLOX_ATA_COMMAND (bus))) & 0x8) && !(status & 0x1))
		asm volatile ("pause");
	/* DRQ or ERROR set */
	if (status & 0x1) {
		size = -1;
		goto end;
	}
	/* Read actual size */
	size = (((ZLOX_SINT32) zlox_inb (ZLOX_ATA_ADDRESS3 (bus))) << 8) | 
		(ZLOX_SINT32) (zlox_inb (ZLOX_ATA_ADDRESS2 (bus)));
	/* This example code only supports the case where the data transfer
	* of one sector is done in one step. */
	ZLOX_ASSERT (size == ZLOX_ATAPI_CAPACITY_SIZE);
	/* Read data. */
	zlox_insw (ZLOX_ATA_DATA (bus), (ZLOX_UINT16 *)buffer, size / 2);
	/* Wait for BSY and DRQ to clear, indicating Command Finished */
	while ((status = zlox_inb (ZLOX_ATA_COMMAND (bus))) & 0x88)
		asm volatile ("pause");

	ZLOX_SINT32 i,j,t,t2,t3;
	for(i=0;i<7;i+=4)
	{
		t3 = (i==0 ? 3 : 11);
		for(j=0;j<2;j++)
		{
			t = ((j==0) ? i : i+1);
			t2 = buffer[t];
			buffer[t] = buffer[t3 - t];
			buffer[t3-t] = t2;
		}
	}
end:
	ata_wait_task = ZLOX_NULL;
	return size;
}

ZLOX_UINT32 zlox_ata_get_ide_info()
{
	return (ZLOX_UINT32)ide_devices;
}

static ZLOX_VOID zlox_ata_callback(/*ZLOX_ISR_REGISTERS * regs*/)
{
	if(ata_wait_task != ZLOX_NULL)
	{
		ata_wait_task->status = ZLOX_TS_RUNNING;
		ata_wait_task = ZLOX_NULL;
	}
}

ZLOX_VOID zlox_init_ata()
{
	ZLOX_SINT32 i, j, k, count = 0;
	ZLOX_UINT16 bus,drive;
	ZLOX_UINT8 * ide_buf = (ZLOX_UINT8 *)zlox_kmalloc(256);
	for (i = 0; i < 2; i++)
	{
		if(i == 0)
			bus = ZLOX_ATA_BUS_PRIMARY;
		else
			bus = ZLOX_ATA_BUS_SECONDARY;
 		for (j = 0; j < 2; j++)
		{
			if(j == 0)
				drive = ZLOX_ATA_DRIVE_MASTER;
			else
				drive = ZLOX_ATA_DRIVE_SLAVE;
			ZLOX_UINT8 err = 0, type = ZLOX_IDE_ATA, status;
			ide_devices[count].Reserved = 0; // Assuming that no drive here.
			// (I) Select Drive:
			zlox_outb (ZLOX_ATA_DRIVE_SELECT (bus),(drive & (1 << 4)));
			ZLOX_ATA_SELECT_DELAY (bus);       /* 400ns delay */
			zlox_outb (ZLOX_ATA_COMMAND (bus), 0xEC);      /* ATA IDENTIFY command */
			ZLOX_ATA_SELECT_DELAY (bus);       /* 400ns delay */
			if(zlox_inb (ZLOX_ATA_COMMAND (bus)) == 0)
				goto inner_end; // If Status = 0, No Device.
			
			while(ZLOX_TRUE)
			{
				status = zlox_inb (ZLOX_ATA_COMMAND (bus));
				if((status & 0x1))
				{
					err = 1; // If Err, Device is not ATA.
					break;
				}
				if(!(status & 0x80) && (status & 0x8))
					break; // Everything is right.
			}

			// (IV) Probe for ATAPI Devices:
			if (err != 0) {
				ZLOX_UINT8 cl = zlox_inb (ZLOX_ATA_ADDRESS2 (bus));
				ZLOX_UINT8 ch = zlox_inb (ZLOX_ATA_ADDRESS3 (bus));
				
				if (cl == 0x14 && ch ==0xEB)
					type = ZLOX_IDE_ATAPI;
				else if (cl == 0x69 && ch == 0x96)
					type = ZLOX_IDE_ATAPI;
				else
					goto inner_end; // Unknown Type (may not be a device).
				
				zlox_outb (ZLOX_ATA_COMMAND (bus), 0xA1); /* ATA IDENTIFY PACKET command */
				ZLOX_ATA_SELECT_DELAY (bus); /* 400ns delay */
			}
			
			// (V) Read Identification Space of the Device:
			zlox_insw (ZLOX_ATA_DATA (bus), (ZLOX_UINT16 *)ide_buf, 256 / 2);
			
			// (VI) Read Device Parameters:
			ide_devices[count].Reserved = 1;
			ide_devices[count].Type = type;
			ide_devices[count].Bus = bus;
			ide_devices[count].Drive = drive;
			ide_devices[count].Signature = *((ZLOX_UINT16 *)(ide_buf + ZLOX_ATA_IDENT_DEVICETYPE));
			ide_devices[count].Capabilities = *((ZLOX_UINT16 *)(ide_buf + ZLOX_ATA_IDENT_CAPABILITIES));
			ide_devices[count].CommandSets = *((ZLOX_UINT32 *)(ide_buf + ZLOX_ATA_IDENT_COMMANDSETS));

			// (VII) Get Size:
			if (ide_devices[count].CommandSets & (1 << 26))
				// Device uses 48-Bit Addressing:
				ide_devices[count].Size   = *((ZLOX_UINT32 *)(ide_buf + ZLOX_ATA_IDENT_MAX_LBA_EXT));
			else
				// Device uses CHS or 28-bit Addressing:
				ide_devices[count].Size   = *((ZLOX_UINT32 *)(ide_buf + ZLOX_ATA_IDENT_MAX_LBA));

			// (VIII) String indicates model of device (like Western Digital HDD and SONY DVD-RW...):
			for(k = 0; k < 40; k += 2) 
			{
				ide_devices[count].Model[k] = ide_buf[ZLOX_ATA_IDENT_MODEL + k + 1];
				ide_devices[count].Model[k + 1] = ide_buf[ZLOX_ATA_IDENT_MODEL + k];
				if(ide_devices[count].Model[k] == 0) //将0变为空格
					ide_devices[count].Model[k] = 0x20;
				if(ide_devices[count].Model[k + 1] == 0) //将0变为空格
					ide_devices[count].Model[k + 1] = 0x20;
			}
			ide_devices[count].Model[40] = 0; // Terminate String.
inner_end:
			count++;
		} // for (j = 0; j < 2; j++)
	} // for (i = 0; i < 2; i++)
	
	zlox_kfree(ide_buf);

	// 4- Print Summary:
	for (i = 0; i < 4; i++)
	{
		if(ide_devices[i].Reserved == 1)
		{
			if(ide_devices[i].Type == ZLOX_IDE_ATA)
				zlox_monitor_write("Found ATA Drive ");
			else
				zlox_monitor_write("Found ATAPI Drive ");
			// if(ide_devices[i].Type == ZLOX_IDE_ATA && !(ide_devices[i].CommandSets & (1 << 26)))
			if(ide_devices[i].Type == ZLOX_IDE_ATA && (ide_devices[i].Size != 0))
			{
				zlox_monitor_write_dec((ide_devices[i].Size + 1) * ZLOX_ATA_SECTOR_SIZE / 1024 / 1024);
				zlox_monitor_write("MB and Last LBA is ");
				zlox_monitor_write_dec(ide_devices[i].Size);
				zlox_monitor_write(" - ");
			}
			else
				zlox_monitor_write(" - ");
			zlox_monitor_write((ZLOX_CHAR *)ide_devices[i].Model);
			zlox_monitor_put('\n');
		}
	}
	
	zlox_register_interrupt_callback(ZLOX_IRQ14,&zlox_ata_callback);
	zlox_register_interrupt_callback(ZLOX_IRQ15,&zlox_ata_callback);

	zlox_ide_probe();
	ZLOX_UINT32 ata_first_idx = 0xff, atapi_first_idx = 0xff, ata_num = 0, atapi_num = 0;
	for (i = 0; i < 4; i++)
	{
		if(ide_devices[i].Reserved == 1)
		{
			if(ide_devices[i].Type == ZLOX_IDE_ATA)
			{
				if(ata_first_idx == 0xff)
					ata_first_idx = (ZLOX_UINT32)i;
				ata_num++;
			}
			else if(ide_devices[i].Type == ZLOX_IDE_ATAPI)
			{
				if(atapi_first_idx == 0xff)
					atapi_first_idx = (ZLOX_UINT32)i;
				atapi_num++;
			}
		}
	}
	if(ide_can_dma)
	{		
		// 下面的注释只是开发过程中的测试代码
		/*if(zlox_strcmpn((ZLOX_CHAR *)ide_devices[ata_first_idx].Model, "VMware Virtual IDE", 18) != 0)
		{
			ide_can_dma = ZLOX_FALSE;
			if(ata_num == 1 && atapi_num == 1)
			{
				if(ata_first_idx == 0 || ata_first_idx == 1)
				{
					if(atapi_first_idx == 2 || atapi_first_idx == 3)
						ide_can_dma = ZLOX_TRUE;
				}
				else if(ata_first_idx == 2 || ata_first_idx == 3)
				{
					if(atapi_first_idx == 0 || atapi_first_idx == 1)
						ide_can_dma = ZLOX_TRUE;
				}
			}
		}*/
	}
	// VMware下，当ATA与ATAPI驱动位于同一个channel通道时, 需要先将ATA数据端口里的数据预先读取一次，才能确保之后的ATAPI读操作时不会出错
	if(atapi_first_idx != 0xff)
	{
		ZLOX_UINT8 * ata_tmp_buffer = (ZLOX_UINT8 *)zlox_kmalloc(ZLOX_ATAPI_SECTOR_SIZE);
		bus = ide_devices[atapi_first_idx].Bus;
		zlox_insw (ZLOX_ATA_DATA (bus), (ZLOX_UINT16 *)ata_tmp_buffer, ZLOX_ATAPI_SECTOR_SIZE / 2);
		zlox_kfree(ata_tmp_buffer);
	}
	if(ide_can_dma)
		zlox_monitor_write("ATA Drive will use DMA mode ");
	else
		zlox_monitor_write("ATA Drive will use PIO mode ");
	zlox_monitor_write("and ATAPI Drive will use PIO mode\n");
}

