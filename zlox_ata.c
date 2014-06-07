/*zlox_ata.c Define some functions relate to ata*/

#include "zlox_ata.h"
#include "zlox_task.h"
#include "zlox_isr.h"
#include "zlox_monitor.h"
#include "zlox_kheap.h"

extern ZLOX_TASK * current_task;
volatile ZLOX_TASK * ata_wait_task = ZLOX_NULL;

ZLOX_IDE_DEVICE ide_devices[4];

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
	current_task->status = ZLOX_TS_ATA_WAIT;
	ata_wait_task = current_task;
	/* Wait for IRQ that says the data is ready. */
	zlox_switch_task();
	/* Read actual size */
	size = (((ZLOX_SINT32) zlox_inb (ZLOX_ATA_ADDRESS3 (bus))) << 8) | 
		(ZLOX_SINT32) (zlox_inb (ZLOX_ATA_ADDRESS2 (bus)));
	/* This example code only supports the case where the data transfer
	* of one sector is done in one step. */
	ZLOX_ASSERT (size == ZLOX_ATAPI_SECTOR_SIZE);
	/* Read data. */
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
	current_task->status = ZLOX_TS_ATA_WAIT;
	ata_wait_task = current_task;
	/* Wait for IRQ that says the data is ready. */
	zlox_switch_task();
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
			if(ide_devices[i].Type == ZLOX_IDE_ATA && !(ide_devices[i].CommandSets & (1 << 26)))
			{
				zlox_monitor_write_dec(ide_devices[i].Size * ZLOX_ATA_SECTOR_SIZE / 1024 / 1024);
				zlox_monitor_write("MB - ");
			}
			else
				zlox_monitor_write(" - ");
			zlox_monitor_write((ZLOX_CHAR *)ide_devices[i].Model);
			zlox_monitor_put('\n');
		}
	}
	
	zlox_register_interrupt_callback(ZLOX_IRQ14,&zlox_ata_callback);
	zlox_register_interrupt_callback(ZLOX_IRQ15,&zlox_ata_callback);
}

