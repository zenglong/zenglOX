/*zlox_ata.c Define some functions relate to ide controller*/

#include "zlox_pci.h"
#include "zlox_monitor.h"
#include "zlox_kheap.h"
#include "zlox_ide.h"
#include "zlox_ata.h"

extern ZLOX_PCI_DEVCONF_LST pci_devconf_lst;
ZLOX_UINT32 ide_reg_base = 0;
ZLOX_UINT8 * ide_pr_mem_buf = ZLOX_NULL;
ZLOX_UINT8 * ide_pr_mem_buf2 = ZLOX_NULL;
ZLOX_UINT32 ide_pci_index;
ZLOX_BOOL ide_can_dma = ZLOX_FALSE;

// 通过PCI配置空间, 检测IDE控制器是否支持Bus Master, 如果支持，则对
// Bus Master IDE的status状态寄存器, Descriptor Table Pointer Register即物理内存区域描述符表指针寄存器
// 进行配置, 从而为DMA传输作准备
ZLOX_SINT32 zlox_ide_probe()
{
	for(ZLOX_UINT32 i = 0; i < pci_devconf_lst.count ;i++)
	{
		if(pci_devconf_lst.ptr[i].cfg_hdr.class_code == 0x01 &&
			pci_devconf_lst.ptr[i].cfg_hdr.sub_class == 0x01 &&
			((pci_devconf_lst.ptr[i].cfg_hdr.prog_if & 0x80) != 0))
		{
			//if(pci_devconf_lst.ptr[i].cfg_hdr.vend_id != 0x8086 || 
			//	pci_devconf_lst.ptr[i].cfg_hdr.dev_id != 0x7111)
			//	return -1;

			ide_reg_base = pci_devconf_lst.ptr[i].cfg_hdr.bars[4];
			if((ide_reg_base & 0x1) == 0)
			{
				return -1;
			}
			else
				ide_reg_base = ide_reg_base & (~0x03);
			ZLOX_UINT32 ide_reg_status = zlox_inb(ide_reg_base + 0x2);
			zlox_outb(ide_reg_base + 0x2, ide_reg_status | 0x60);
			ide_reg_status = zlox_inb(ide_reg_base + 0xa);
			zlox_outb(ide_reg_base + 0xa, ide_reg_status | 0x60);

			ZLOX_UINT16 cr = zlox_pci_reg_inw(pci_devconf_lst.ptr[i].cfg_addr, ZLOX_PCI_REG_COMMAND);
			if (!(cr & ZLOX_PCI_COMMAND_MAST_EN))
				zlox_pci_reg_outw(pci_devconf_lst.ptr[i].cfg_addr, ZLOX_PCI_REG_COMMAND, cr | ZLOX_PCI_COMMAND_MAST_EN);

			ZLOX_UINT32 prdt_phy = 0;
			ZLOX_UINT32 prdt_u32 = zlox_kmalloc_ap(8, &prdt_phy);
			ZLOX_UINT32 pr_mem_buf_phy = 0;
			ide_pr_mem_buf = (ZLOX_UINT8 *)zlox_kmalloc_ap(2048, &pr_mem_buf_phy);
			ZLOX_UINT32 * prdt_ptr = (ZLOX_UINT32 *)prdt_u32;
			prdt_ptr[0] = pr_mem_buf_phy;
			//prdt_ptr[1] = (2048 | 0x80000000); //如果将物理内存区域的大小设置为2048的话, 则bochs下运行时会出现问题
			prdt_ptr[1] = (1024 | 0x80000000);
			zlox_outl(ide_reg_base + 0x4, prdt_phy);

			prdt_phy = 0;
			prdt_u32 = zlox_kmalloc_ap(8, &prdt_phy);
			pr_mem_buf_phy = 0;
			ide_pr_mem_buf2 = (ZLOX_UINT8 *)zlox_kmalloc_ap(2048, &pr_mem_buf_phy);
			prdt_ptr = (ZLOX_UINT32 *)prdt_u32;
			prdt_ptr[0] = pr_mem_buf_phy;
			//prdt_ptr[1] = (2048 | 0x80000000);
			prdt_ptr[1] = (1024 | 0x80000000);
			zlox_outl(ide_reg_base + 0xC, prdt_phy);

			ide_pci_index = i;
			ide_can_dma = ZLOX_TRUE;
			return 0;
		}
	}
	return -1;
}

// 在发送ATA磁盘读写命令之前, 需要先将Bus Master IDE Command命令寄存器的读写位进行设置, 让IDE控制器知道
// 当前是读操作还是写操作, 另外还需要将Bus Master IDE Status状态寄存器里的中断位与错误位清零
ZLOX_SINT32 zlox_ide_before_send_command(ZLOX_UINT8 direction, ZLOX_UINT16 bus)
{
	if(ide_can_dma == ZLOX_FALSE)
		return -1;
	ZLOX_UINT32 cmd_port;
	if(bus == ZLOX_ATA_BUS_PRIMARY)
		cmd_port = ide_reg_base + 0x0;
	else
		cmd_port = ide_reg_base + 0x8;
	if(direction == 0)
		zlox_outb(cmd_port, 0x08);
	else
		zlox_outb(cmd_port, 0);
	ZLOX_UINT32 ide_reg_status;
	ZLOX_UINT16 status_port;
	if(bus == ZLOX_ATA_BUS_PRIMARY)
		status_port = ide_reg_base + 0x2;
	else
		status_port = ide_reg_base + 0xa;
	ide_reg_status = zlox_inb(status_port);
	ide_reg_status |= 0x6; // 只有向中断位和Error错误位写入1, 才能将其清零
	zlox_outb(status_port, ide_reg_status);
	return 0;
}

// 通过将Bus Master IDE Command命令寄存器的位0即Start/Stop Bus Master位设置为1, 就可以触发DMA总线读写操作
ZLOX_SINT32 zlox_ide_start_dma(ZLOX_UINT16 bus)
{
	if(ide_can_dma == ZLOX_FALSE)
		return -1;
	ZLOX_UINT16 cmd_port;
	if(bus == ZLOX_ATA_BUS_PRIMARY)
		cmd_port = ide_reg_base + 0x0;
	else
		cmd_port = ide_reg_base + 0x08;
	ZLOX_UINT32 ide_reg_command = zlox_inb(cmd_port);
	zlox_outb(cmd_port, ide_reg_command | 0x1);
	return 0;
}

// 在DMA总线读写操作完毕后, 需要通过Bus Master IDE status状态寄存器来检测中断位 或 Bus Master IDE Active位,
// 当中断位为1时(即ATA驱动器发送了中断信号), 则说明读写操作完成, 当然也有可能是发生了错误而中断了传输, 要判断是否发生错误
// 可以通过状态寄存器的Error错误位来进行检测
// 此外, 由于VirtualBox下, 当ATA与ATAPI驱动位于同一个channel通道时, 因为它们共用一个中断信号, 因此会导致对ATAPI读操作后, ATA驱动无法
// 检测到中断信号的情况, 这种情况下, 我们就需要通过Bus Master IDE status状态寄存器的Bus Master IDE Active位来进行检测了,
// 当Active位为0时, 则说明数据传输完毕, 或者发生错误而中断了传输, 同样也可以配合Error错误位来进行判断
ZLOX_SINT32 zlox_ide_after_send_command(ZLOX_UINT16 bus)
{
	if(ide_can_dma == ZLOX_FALSE)
		return -1;
	ZLOX_UINT8 ide_reg_status = 0;
	for(;;)
	{
		if(bus == ZLOX_ATA_BUS_PRIMARY)
			ide_reg_status = zlox_inb(ide_reg_base + 0x2);
		else
			ide_reg_status = zlox_inb(ide_reg_base + 0xa);
		if(((ide_reg_status & 0x4) != 0) || ((ide_reg_status & 0x1) == 0))
		{
			if((ide_reg_status & 0x2) != 0)
			{
				if(bus == ZLOX_ATA_BUS_PRIMARY)
					zlox_outb(ide_reg_base + 0x0, 0);
				else
					zlox_outb(ide_reg_base + 0x8, 0);
				ZLOX_UINT16 status = pci_devconf_lst.ptr[ide_pci_index].cfg_hdr.status;
				zlox_monitor_write("ide error , pci status: ");
				zlox_monitor_write_hex((ZLOX_UINT32)status);
				zlox_monitor_write("\n");
				zlox_monitor_write("ide error , pci dev_id: ");
				zlox_monitor_write_hex((ZLOX_UINT32)pci_devconf_lst.ptr[ide_pci_index].cfg_hdr.dev_id);
				zlox_monitor_write("\n");
				return -1;
			}
			if(bus == ZLOX_ATA_BUS_PRIMARY)
				zlox_outb(ide_reg_base + 0x0, 0);
			else
				zlox_outb(ide_reg_base + 0x8, 0);
			return 1;
		}
		else
			asm volatile ("pause");
	}
	return 0;
}

// 从物理内存区域中获取DMA传输的数据
ZLOX_SINT32 zlox_ide_get_buffer_data(ZLOX_UINT8 * buffer, ZLOX_UINT32 len, ZLOX_UINT16 bus)
{
	if(ide_can_dma == ZLOX_FALSE)
		return -1;
	if(bus == ZLOX_ATA_BUS_PRIMARY)
		zlox_memcpy(buffer, ide_pr_mem_buf, len);
	else
		zlox_memcpy(buffer, ide_pr_mem_buf2, len);
	return 0;
}

// 要写入ATA磁盘的数据, 需要先写入到物理内存区域中, 稍后再通过DMA总线传输到磁盘里
ZLOX_SINT32 zlox_ide_set_buffer_data(ZLOX_UINT8 * buffer, ZLOX_UINT32 len, ZLOX_UINT16 bus)
{
	if(ide_can_dma == ZLOX_FALSE)
		return -1;
	if(bus == ZLOX_ATA_BUS_PRIMARY)
		zlox_memcpy(ide_pr_mem_buf, buffer, len);
	else
		zlox_memcpy(ide_pr_mem_buf2, buffer, len);
	return 0;
}

