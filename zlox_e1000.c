// zlox_e1000.c -- 和Intel Pro/1000 (e1000)网卡驱动相关的函数定义

#include "zlox_e1000.h"
#include "zlox_monitor.h"
#include "zlox_paging.h"
#include "zlox_isr.h"
#include "zlox_kheap.h"
#include "zlox_time.h"
#include "zlox_network.h"

#define ZLOX_E1000_VEND_ID 0x8086

#define ZLOX_E1000_82540EM_A 0x100E
#define ZLOX_E1000_82545EM_A 0x100F
#define ZLOX_E1000_82543GC_COPPER 0x1004
#define ZLOX_E1000_82541GI_LF 0x107C
#define ZLOX_E1000_ICH10_R_BM_LF 0x10CD
#define ZLOX_E1000_82574L 0x10D3
#define ZLOX_E1000_ICH10_D_BM_LM 0x10DE
#define ZLOX_E1000_82571EB_COPPER 0x105E

#define ZLOX_E1000_REG_CTRL 0x0000
#define ZLOX_E1000_REG_EERD 0x0014
#define ZLOX_E1000_REG_ICR 0x00C0
/** Interrupt Mask Set/Read Register. */
#define ZLOX_E1000_REG_IMS 0x000d0

#define ZLOX_E1000_REG_RDBAL 0x2800
#define ZLOX_E1000_REG_RDBAH 0x2804
#define ZLOX_E1000_REG_RDLEN 0x2808
#define ZLOX_E1000_REG_RDH 0x2810
#define ZLOX_E1000_REG_RDT 0x2818
#define ZLOX_E1000_REG_RCTL 0x0100

#define ZLOX_E1000_REG_TCTL 0x0400
#define ZLOX_E1000_REG_TDBAL 0x3800
#define ZLOX_E1000_REG_TDBAH 0x3804
#define ZLOX_E1000_REG_TDLEN 0x3808
#define ZLOX_E1000_REG_TDH 0x3810
#define ZLOX_E1000_REG_TDT 0x3818

/** Receive Address Low. */
#define ZLOX_E1000_REG_RAL 0x05400

/** Receive Address High. */
#define ZLOX_E1000_REG_RAH 0x05404

/** Flow Control Address Low. */
#define ZLOX_E1000_REG_FCAL 0x00028

/** Flow Control Address High. */
#define ZLOX_E1000_REG_FCAH 0x0002c

/** Flow Control Type. */
#define ZLOX_E1000_REG_FCT 0x00030

/** Flow Control Transmit Timer Value. */
#define ZLOX_E1000_REG_FCTTV 0x00170

/** Multicast Table Array. */
#define ZLOX_E1000_REG_MTA 0x05200

/** CRC Error Count. */
#define ZLOX_E1000_REG_CRCERRS	0x04000

/**
 * name Receive Control Register Bits.
 * @{
 */

/** Receive Enable. */
#define ZLOX_E1000_REG_RCTL_EN	(1 << 1)

/** Multicast Promiscious Enable. */
#define ZLOX_E1000_REG_RCTL_MPE	(1 << 4)

/** Receive Buffer Size. */
#define ZLOX_E1000_REG_RCTL_BSIZE ((1 << 16) | (1 << 17))

/**
 * @}
 */

/**
 * name Transmit Control Register Bits.
 * @{
 */

/** Transmit Enable. */
#define ZLOX_E1000_REG_TCTL_EN	(1 << 1)

/** Pad Short Packets. */
#define ZLOX_E1000_REG_TCTL_PSP	(1 << 3)

/**
 * @}
 */

/**
 * name Receive Address High Register Bits.
 * @{
 */

/** Receive Address Valid. */
#define ZLOX_E1000_REG_RAH_AV (1 << 31)

/**
 * @}
 */

/**
 * name Control Register Bits.
 * @{
 */

/** Auto-Speed Detection Enable. */
#define ZLOX_E1000_REG_CTRL_ASDE (1 << 5)

/** Link Reset. */
#define ZLOX_E1000_REG_CTRL_LRST (1 << 3)

/** Set Link Up. */
#define ZLOX_E1000_REG_CTRL_SLU (1 << 6)

/** Invert Los Of Signal. */
#define ZLOX_E1000_REG_CTRL_ILOS (1 << 7)

/** Device Reset. */
#define ZLOX_E1000_REG_CTRL_RST (1 << 26)

/** VLAN Mode Enable. */
#define ZLOX_E1000_REG_CTRL_VME (1 << 30)

/** PHY Reset. */
#define ZLOX_E1000_REG_CTRL_PHY_RST (1 << 31)

/**
 * @}
 */

/**
 * @name Interrupt Mask Set/Read Register Bits.
 * @{
 */

/** Transmit Descripts Written Back. */
#define ZLOX_E1000_REG_IMS_TXDW (1 << 0)

/** Transmit Queue Empty. */
#define ZLOX_E1000_REG_IMS_TXQE (1 << 1)

/** Link Status Change. */
#define ZLOX_E1000_REG_IMS_LSC (1 << 2)

/** Receiver FIFO Overrun. */
#define ZLOX_E1000_REG_IMS_RXO (1 << 6)

/** Receiver Timer Interrupt. */
#define ZLOX_E1000_REG_IMS_RXT (1 << 7)

/**
 * @}
 */

/**
 * @name Transmit Command Field Bits.
 * @{
 */

/** End of Packet. */
#define ZLOX_E1000_TX_CMD_EOP (1 << 0)

/** Insert FCS/CRC. */
#define ZLOX_E1000_TX_CMD_FCS (1 << 1)

/** Report Status. */
#define ZLOX_E1000_TX_CMD_RS (1 << 3)

/**
 * @}
 */

extern ZLOX_PAGE_DIRECTORY * kernel_directory;

ZLOX_E1000_DEV e1000_dev = {0};

ZLOX_UINT32 zlox_e1000_reg_read(ZLOX_UINT32 reg)
{
	if(reg >= 0x1ffff)
		return 0;
	ZLOX_UINT32 value;
	value = *(volatile ZLOX_UINT32 *)(e1000_dev.mmio_base + reg);
	return value;
}

ZLOX_VOID zlox_e1000_reg_write(ZLOX_UINT32 reg, ZLOX_UINT32 value)
{
	if(reg >= 0x1ffff)
		return;
	*(volatile ZLOX_UINT32 *)(e1000_dev.mmio_base + reg) = value;
}

ZLOX_VOID zlox_e1000_reg_set(ZLOX_UINT32 reg, ZLOX_UINT32 value)
{
	ZLOX_UINT32 data;

    /* First read the current value. */
    data = zlox_e1000_reg_read(reg);
    
    /* Set value, and write back. */
    zlox_e1000_reg_write(reg, data | value);
}

ZLOX_VOID zlox_e1000_reg_unset(ZLOX_UINT32 reg, ZLOX_UINT32 value)
{
	ZLOX_UINT32 data;

	/* First read the current value. */
	data = zlox_e1000_reg_read(reg);

	/* Unset value, and write back. */
	zlox_e1000_reg_write(reg, data & ~value);
}

ZLOX_UINT32 zlox_e1000_eeprom_read(ZLOX_UINT8 addr)
{
	ZLOX_UINT32 val = 0;
	ZLOX_UINT32 test = 0;
	if(e1000_dev.is_e == ZLOX_FALSE)
		test = addr << 8;
	else
		test = addr << 2;

	zlox_e1000_reg_write(ZLOX_E1000_REG_EERD, test | 0x1);
	if(e1000_dev.is_e == ZLOX_FALSE)
		while(!((val = zlox_e1000_reg_read(ZLOX_E1000_REG_EERD)) & (1<<4))) // check if done !
			asm volatile ("pause");
	else
		while(!((val = zlox_e1000_reg_read(ZLOX_E1000_REG_EERD)) & (1<<1))) // check if done !
			asm volatile ("pause");
	val >>= 16;
	return val;
}

ZLOX_VOID zlox_e1000_eeprom_gettype()
{
	ZLOX_UINT32 val = 0;
	zlox_e1000_reg_write(ZLOX_E1000_REG_EERD, 0x1);
	for(ZLOX_UINT32 i = 0; i < 1000; i++)
	{
		val = zlox_e1000_reg_read(ZLOX_E1000_REG_EERD);
		if(val & 0x10)
			e1000_dev.is_e = ZLOX_FALSE;
		else
			e1000_dev.is_e = ZLOX_TRUE;	
	}
}

ZLOX_VOID zlox_e1000_getmac()
{
	ZLOX_UINT32 temp = 0;
	temp = zlox_e1000_eeprom_read(0);
	e1000_dev.mac[0] = temp & 0xff;
	e1000_dev.mac[1] = temp >> 8;
	temp = zlox_e1000_eeprom_read(1);
	e1000_dev.mac[2] = temp & 0xff;
	e1000_dev.mac[3] = temp >> 8;
	temp = zlox_e1000_eeprom_read(2);
	e1000_dev.mac[4] = temp & 0xff;
	e1000_dev.mac[5] = temp >> 8;
}

ZLOX_VOID zlox_e1000_linkup()
{
	zlox_e1000_reg_set(ZLOX_E1000_REG_CTRL, ZLOX_E1000_REG_CTRL_SLU);
}

ZLOX_SINT32 zlox_e1000_send(ZLOX_UINT8 * buf, ZLOX_UINT16 length)
{
	ZLOX_UINT32 tail = zlox_e1000_reg_read(ZLOX_E1000_REG_TDT);
	ZLOX_E1000_TX_DESC * desc;
	desc = &e1000_dev.tx_descs[tail];

	if(length > ZLOX_TX_BUFFER_SIZE)
		length = ZLOX_TX_BUFFER_SIZE;
	zlox_memcpy(e1000_dev.tx_buffer + (tail * ZLOX_TX_BUFFER_SIZE), buf, length);
	/* Mark this descriptor ready. */
	desc->status = 0;
	desc->cmd = 0;
	desc->length = length;

	/* Marks End-of-Packet. */
	desc->cmd = ZLOX_E1000_TX_CMD_EOP | ZLOX_E1000_TX_CMD_FCS | ZLOX_E1000_TX_CMD_RS;

	/* Move to next descriptor. */
	tail = (tail + 1) % e1000_dev.tx_descs_count;

	/* Increment tail. Start transmission. */
	zlox_e1000_reg_write(ZLOX_E1000_REG_TDT,  tail);

	// 5 seconds time out!
	for(ZLOX_UINT32 j = 0; j < 10 * 5; j++)
	{
		for(ZLOX_UINT32 i=0;i < 10000 && !(desc->status & 0xf); i++)
		{
			asm("pause");
		}
		if(desc->status & 0xf)
			break;
		zlox_timer_sleep(5, ZLOX_FALSE); // 5 * 20ms = 100ms
	}
	if(desc->status & 0xf)
		return (ZLOX_SINT32)length;
	else
		return -1;
	return 0;
}

ZLOX_VOID zlox_e1000_received()
{
	ZLOX_UINT32 tail = zlox_e1000_reg_read(ZLOX_E1000_REG_RDT);
	ZLOX_UINT32 cur = (tail + 1) % e1000_dev.rx_descs_count;
	ZLOX_UINT32 old_cur;
	while(e1000_dev.rx_descs[cur].status & 0x1)
	{
		ZLOX_UINT8 * buf = e1000_dev.rx_buffer + cur * ZLOX_RX_BUFFER_SIZE;
		ZLOX_UINT16 length = e1000_dev.rx_descs[cur].length;
		zlox_network_received(buf, length);
		//zlox_monitor_write("\n### receive packet! ###\n");
		e1000_dev.rx_descs[cur].status = 0;
		old_cur = cur;
		cur = (cur + 1) % ZLOX_NUM_RX_DESC;
		zlox_e1000_reg_write(ZLOX_E1000_REG_RDT, old_cur);
	}
}

static ZLOX_VOID zlox_e1000_callback()
{
	ZLOX_UINT32 icr = zlox_e1000_reg_read(ZLOX_E1000_REG_ICR);

	if(icr & 0x04)
	{
		zlox_e1000_linkup();
	}

	if(icr & 0x10)
	{
		zlox_monitor_write("\n### e1000 warning: Receive Descriptor Minimum Threshold Reached ###\n");
	}
	
	// Receiver Timer Interrupt
	if(icr & 0x80)
	{
		zlox_e1000_received();
	}

	//zlox_monitor_write("\n### e1000 irq's icr is : [");
	//zlox_monitor_write_hex(icr);
	//zlox_monitor_write("] ###\n");
}

/*===========================================================================*
 *				zlox_e1000_init_addr				     *
 *===========================================================================*/
ZLOX_VOID zlox_e1000_init_addr()
{
	ZLOX_SINT32 i;
	zlox_e1000_eeprom_gettype();
	zlox_e1000_getmac();

	/*
	 * Set Receive Address.
	 */
	zlox_e1000_reg_write(ZLOX_E1000_REG_RAL, *(ZLOX_UINT32 *)(&e1000_dev.mac[0]));
	zlox_e1000_reg_write(ZLOX_E1000_REG_RAH, *(ZLOX_UINT16 *)(&e1000_dev.mac[4]));
	zlox_e1000_reg_set(ZLOX_E1000_REG_RAH, ZLOX_E1000_REG_RAH_AV);
	zlox_e1000_reg_set(ZLOX_E1000_REG_RCTL, ZLOX_E1000_REG_RCTL_MPE);

	zlox_monitor_write("e1000 mac address: ");
	for(i=0;i < 6;i++)
	{
		zlox_monitor_write_hex(e1000_dev.mac[i]);
		zlox_monitor_write(" ");
	}
	zlox_monitor_write("\n");
}

ZLOX_VOID zlox_e1000_init_buf()
{
	ZLOX_UINT32 rx_buff_p;
	ZLOX_UINT32 tx_buff_p;
	ZLOX_SINT32 i;
	
	e1000_dev.rx_descs_count = ZLOX_NUM_RX_DESC;
	e1000_dev.tx_descs_count = ZLOX_NUM_TX_DESC;
	
	e1000_dev.rx_descs = (ZLOX_E1000_RX_DESC *)zlox_kmalloc_ap(e1000_dev.rx_descs_count * sizeof(ZLOX_E1000_RX_DESC), &e1000_dev.rx_descs_p);
	zlox_memset((ZLOX_UINT8 *)e1000_dev.rx_descs, 0, e1000_dev.rx_descs_count * sizeof(ZLOX_E1000_RX_DESC));
	/*
	* Allocate 2048-byte buffers.
	*/
	e1000_dev.rx_buffer_size = ZLOX_NUM_RX_DESC * ZLOX_RX_BUFFER_SIZE;
	e1000_dev.rx_buffer = (ZLOX_UINT8 *)zlox_kmalloc_ap(e1000_dev.rx_buffer_size, &rx_buff_p);
	/* Setup receive descriptors. */
	for(i = 0; i < ZLOX_NUM_RX_DESC; i++)
	{
		e1000_dev.rx_descs[i].buffer = rx_buff_p + (i * ZLOX_RX_BUFFER_SIZE);
	}
	/*
     * Then, allocate transmit descriptors.
     */
	e1000_dev.tx_descs = (ZLOX_E1000_TX_DESC *)zlox_kmalloc_ap(e1000_dev.tx_descs_count * sizeof(ZLOX_E1000_TX_DESC), &e1000_dev.tx_descs_p);
	zlox_memset((ZLOX_UINT8 *)e1000_dev.tx_descs, 0, e1000_dev.tx_descs_count * sizeof(ZLOX_E1000_TX_DESC));
	/*
	* Allocate 2048-byte buffers.
	*/
	e1000_dev.tx_buffer_size = ZLOX_NUM_TX_DESC * ZLOX_TX_BUFFER_SIZE;
	
	/* Attempt to allocate. */
	e1000_dev.tx_buffer = (ZLOX_UINT8 *)zlox_kmalloc_ap(e1000_dev.tx_buffer_size, &tx_buff_p);
	/* Setup transmit descriptors. */
	for(i = 0; i < ZLOX_NUM_TX_DESC; i++)
	{
		e1000_dev.tx_descs[i].buffer = tx_buff_p + (i * ZLOX_TX_BUFFER_SIZE);
	}

	/*
	 * Setup the receive ring registers.
	 */
	zlox_e1000_reg_write(ZLOX_E1000_REG_RDBAL, e1000_dev.rx_descs_p);
	zlox_e1000_reg_write(ZLOX_E1000_REG_RDBAH, 0);
	zlox_e1000_reg_write(ZLOX_E1000_REG_RDLEN, (e1000_dev.rx_descs_count * sizeof(ZLOX_E1000_RX_DESC)));
	zlox_e1000_reg_write(ZLOX_E1000_REG_RDH, 0);
	zlox_e1000_reg_write(ZLOX_E1000_REG_RDT, (e1000_dev.rx_descs_count - 1));
	zlox_e1000_reg_unset(ZLOX_E1000_REG_RCTL, ZLOX_E1000_REG_RCTL_BSIZE);
	zlox_e1000_reg_set(ZLOX_E1000_REG_RCTL, ZLOX_E1000_REG_RCTL_EN);

	/*
	 * Setup the transmit ring registers.
	 */
	zlox_e1000_reg_write(ZLOX_E1000_REG_TDBAL, e1000_dev.tx_descs_p);
	zlox_e1000_reg_write(ZLOX_E1000_REG_TDBAH, 0);
	zlox_e1000_reg_write(ZLOX_E1000_REG_TDLEN, e1000_dev.tx_descs_count * sizeof(ZLOX_E1000_TX_DESC));
	zlox_e1000_reg_write(ZLOX_E1000_REG_TDH, 0);
	zlox_e1000_reg_write(ZLOX_E1000_REG_TDT, 0);
	zlox_e1000_reg_set(ZLOX_E1000_REG_TCTL, ZLOX_E1000_REG_TCTL_EN | ZLOX_E1000_REG_TCTL_PSP);
}

/*===========================================================================*
 *				zlox_e1000_reset_hw				     *
 *===========================================================================*/
ZLOX_VOID zlox_e1000_reset_hw()
{
    /* Assert a Device Reset signal. */
    zlox_e1000_reg_set(ZLOX_E1000_REG_CTRL, ZLOX_E1000_REG_CTRL_RST);

    for(ZLOX_UINT32 i=0;i < 20000;i++)
		asm("pause");
}

ZLOX_VOID zlox_e1000_start()
{
	ZLOX_SINT32 i;
	/* Reset hardware. */
	zlox_e1000_reset_hw();

	/*
	 * Initialize appropriately, according to section 14.3 General Configuration
	 * of Intel's Gigabit Ethernet Controllers Software Developer's Manual.
	 */
	zlox_e1000_reg_set(ZLOX_E1000_REG_CTRL, ZLOX_E1000_REG_CTRL_ASDE | ZLOX_E1000_REG_CTRL_SLU);
	zlox_e1000_reg_unset(ZLOX_E1000_REG_CTRL, ZLOX_E1000_REG_CTRL_LRST);
	zlox_e1000_reg_unset(ZLOX_E1000_REG_CTRL, ZLOX_E1000_REG_CTRL_PHY_RST);
	zlox_e1000_reg_unset(ZLOX_E1000_REG_CTRL, ZLOX_E1000_REG_CTRL_ILOS);
	zlox_e1000_reg_write(ZLOX_E1000_REG_FCAL, 0);
	zlox_e1000_reg_write(ZLOX_E1000_REG_FCAH, 0);
	zlox_e1000_reg_write(ZLOX_E1000_REG_FCT,  0);
	zlox_e1000_reg_write(ZLOX_E1000_REG_FCTTV, 0);
	zlox_e1000_reg_unset(ZLOX_E1000_REG_CTRL, ZLOX_E1000_REG_CTRL_VME);

	//have to clear out the multicast filter
	for (i = 0; i < 128; i++)
	{
		zlox_e1000_reg_write(ZLOX_E1000_REG_MTA + i, 0);
	}

	/* Initialize statistics registers. */
	for (i = 0; i < 64; i++)
    {
		zlox_e1000_reg_write(ZLOX_E1000_REG_CRCERRS + (i * 4), 0);
	}
	/*
	 * Aquire MAC address and setup RX/TX buffers.
	 */
	zlox_e1000_init_addr();
	zlox_e1000_init_buf();
	/* Enable interrupts. */
    zlox_e1000_reg_set(ZLOX_E1000_REG_IMS, ZLOX_E1000_REG_IMS_LSC  |
				      ZLOX_E1000_REG_IMS_RXO  |
				      ZLOX_E1000_REG_IMS_RXT  |
				      ZLOX_E1000_REG_IMS_TXQE |
				      ZLOX_E1000_REG_IMS_TXDW);
}

ZLOX_VOID zlox_e1000_init()
{
	if(e1000_dev.isInit == ZLOX_TRUE)
		return ;
	// 82540EM can be used in bochs and VirtualBox
	e1000_dev.pci_devconf = zlox_pci_get_devconf(ZLOX_E1000_VEND_ID, ZLOX_E1000_82540EM_A);
	// 82545EM can be used in VirtualBox and Vmware
	if(e1000_dev.pci_devconf == ZLOX_NULL)
		e1000_dev.pci_devconf = zlox_pci_get_devconf(ZLOX_E1000_VEND_ID, ZLOX_E1000_82545EM_A);
	// 82543GC can be used in VirtualBox
	if(e1000_dev.pci_devconf == ZLOX_NULL)
		e1000_dev.pci_devconf = zlox_pci_get_devconf(ZLOX_E1000_VEND_ID, ZLOX_E1000_82543GC_COPPER);
	// the follow card is from Minix3's e1000.conf
	if(e1000_dev.pci_devconf == ZLOX_NULL)
		e1000_dev.pci_devconf = zlox_pci_get_devconf(ZLOX_E1000_VEND_ID, ZLOX_E1000_82541GI_LF);
	if(e1000_dev.pci_devconf == ZLOX_NULL)
		e1000_dev.pci_devconf = zlox_pci_get_devconf(ZLOX_E1000_VEND_ID, ZLOX_E1000_ICH10_R_BM_LF);
	if(e1000_dev.pci_devconf == ZLOX_NULL)
		e1000_dev.pci_devconf = zlox_pci_get_devconf(ZLOX_E1000_VEND_ID, ZLOX_E1000_82574L);
	if(e1000_dev.pci_devconf == ZLOX_NULL)
		e1000_dev.pci_devconf = zlox_pci_get_devconf(ZLOX_E1000_VEND_ID, ZLOX_E1000_ICH10_D_BM_LM);
	if(e1000_dev.pci_devconf == ZLOX_NULL)
		e1000_dev.pci_devconf = zlox_pci_get_devconf(ZLOX_E1000_VEND_ID, ZLOX_E1000_82571EB_COPPER);
	if(e1000_dev.pci_devconf == ZLOX_NULL)
		return ;

	zlox_monitor_write("Intel Pro/1000 Ethernet adapter Rev: [");
	zlox_monitor_write_hex(e1000_dev.pci_devconf->cfg_hdr.rev);
	zlox_monitor_write("]\n");

	ZLOX_UINT32 phy_bar = (e1000_dev.pci_devconf->cfg_hdr.bars[0] & 0xFFFFFFF0);
	e1000_dev.mmio_base = ZLOX_E1000_MMIO_ADDR + (phy_bar & 0x0FFFFFFF);
	ZLOX_UINT32 mmio_mem_amount = zlox_pci_get_bar_mem_amount(e1000_dev.pci_devconf->cfg_addr, 0);
	ZLOX_UINT32 i,j;
	for (i = e1000_dev.mmio_base,j = phy_bar; 
		i < e1000_dev.mmio_base + 0x20000; 
		i += 0x1000, j += 0x1000)
		zlox_alloc_frame_do_ext(zlox_get_page(i, 1, kernel_directory), j, 0, 0);
	zlox_page_Flush_TLB();

	zlox_monitor_write("MMIO linear address: [");
	zlox_monitor_write_hex(e1000_dev.mmio_base);
	zlox_monitor_write("] MMIO physical memory: [");
	zlox_monitor_write_hex(phy_bar);
	zlox_monitor_write("] MMIO memory amount: [");
	zlox_monitor_write_hex(mmio_mem_amount);
	zlox_monitor_write("]\n");
	zlox_monitor_write("PCI interrupt line: [");
	zlox_monitor_write_hex(e1000_dev.pci_devconf->cfg_hdr.int_line);
	zlox_monitor_write("] PCI interrupt pin: [");
	zlox_monitor_write_hex(e1000_dev.pci_devconf->cfg_hdr.int_pin);
	zlox_monitor_write("] PCI vend id: [");
	zlox_monitor_write_hex(e1000_dev.pci_devconf->cfg_hdr.vend_id);
	zlox_monitor_write("] PCI dev id: [");
	zlox_monitor_write_hex(e1000_dev.pci_devconf->cfg_hdr.dev_id);
	zlox_monitor_write("]\n");

	/* FIXME: enable DMA bus mastering if necessary. This is disabled by
	 * default on VMware. Eventually, the PCI driver should deal with this.
	 */
	ZLOX_UINT16 cr = zlox_pci_reg_inw(e1000_dev.pci_devconf->cfg_addr, ZLOX_PCI_REG_COMMAND);
	if (!(cr & ZLOX_PCI_COMMAND_MAST_EN))
		zlox_pci_reg_outw(e1000_dev.pci_devconf->cfg_addr, ZLOX_PCI_REG_COMMAND, cr | ZLOX_PCI_COMMAND_MAST_EN);

	zlox_register_interrupt_callback(ZLOX_IRQ0 + e1000_dev.pci_devconf->cfg_hdr.int_line, 
						&zlox_e1000_callback);

	zlox_e1000_start();

	e1000_dev.isInit = ZLOX_TRUE;
}

