// zlox_uhci.c -- the C file relate to UHCI

#include "zlox_uhci.h"
#include "zlox_monitor.h"
#include "zlox_kheap.h"
#include "zlox_time.h"
#include "zlox_isr.h"

//debug
ZLOX_UHCI_HCD * g_test_uhci = 0;

static ZLOX_UINT16 zlox_uhci_readw(ZLOX_UHCI_HCD * uhci_hcd, ZLOX_UINT16 reg)
{
	return zlox_inw(uhci_hcd->io_addr + reg);
}

// from linux: <file>linux-3.2.0-src/drivers/usb/host/uhci-hcd.c : 
// <function>uhci_count_ports
/* Determines number of ports on controller */
static ZLOX_SINT32 zlox_uhci_count_ports(ZLOX_UHCI_HCD * uhci_hcd)
{
	ZLOX_UINT32 io_size = uhci_hcd->io_len;
	ZLOX_UINT32 port;

	/* The UHCI spec says devices must have 2 ports, and goes on to say
	 * they may have more but gives no way to determine how many there
	 * are.  However according to the UHCI spec, Bit 7 of the port
	 * status and control register is always set to 1.  So we try to
	 * use this to our advantage.  Another common failure mode when
	 * a nonexistent register is addressed is to return all ones, so
	 * we test for that also.
	 */
	for (port = 0; port < (io_size - ZLOX_UHCI_REG_PORT1) / 2; port++) 
	{
		ZLOX_UINT16 portstatus;

		portstatus = zlox_uhci_readw(uhci_hcd, 
				ZLOX_UHCI_REG_PORT1 + (port * 2));
		if (!(portstatus & 0x0080) || portstatus == 0xffff)
			break;
	}

	/* Anything greater than 7 is weird so we'll ignore it. */
	if (port > ZLOX_UHCI_RH_MAXCHILD)
	{
		port = 2;
	}

	return port;
}

ZLOX_BOOL zlox_uhci_detect(ZLOX_PCI_CONF_HDR * cfg_hdr)
{
	if(cfg_hdr == ZLOX_NULL)
		return ZLOX_FALSE;

	ZLOX_UINT8 class_code = cfg_hdr->class_code;
	ZLOX_UINT8 sub_class = cfg_hdr->sub_class;
	ZLOX_UINT8 prog_if = cfg_hdr->prog_if;
	if(class_code == ZLOX_UHCI_CLASS && 
	   sub_class == ZLOX_UHCI_SUB_CLASS && 
	   prog_if == ZLOX_UHCI_PROG_IF)
	{
		return ZLOX_TRUE;
	}
	return ZLOX_FALSE;
}

ZLOX_SINT32 zlox_uhci_print_devinfo(ZLOX_UHCI_HCD * uhci_hcd)
{
	if(uhci_hcd == ZLOX_NULL)
		return -1;

	zlox_monitor_write("UHCI irq:");
	zlox_monitor_write_dec(uhci_hcd->irq);
	zlox_monitor_write(" irq_pin:");
	zlox_monitor_write_dec(uhci_hcd->irq_pin);
	zlox_monitor_write(" io_addr:");
	zlox_monitor_write_hex(uhci_hcd->io_addr);
	zlox_monitor_write(" io_len:");
	zlox_monitor_write_dec(uhci_hcd->io_len);
	zlox_monitor_write(" numports:");
	zlox_monitor_write_dec(uhci_hcd->rh_numports);
	return 0;
}

static ZLOX_UHCI_TD * zlox_uhci_alloc_td(ZLOX_UHCI_HCD * uhci_hcd)
{
	ZLOX_UHCI_TD * end = uhci_hcd->tdPool + ZLOX_UHCI_MAX_TD;
	for (ZLOX_UHCI_TD * td = uhci_hcd->tdPool; td != end; ++td)
	{
		if (!td->active)
		{
			td->active = 1;
			return td;
		}
	}

	return ZLOX_NULL;
}

static ZLOX_UHCI_QH * zlox_uhci_alloc_qh(ZLOX_UHCI_HCD * uhci_hcd)
{
	ZLOX_UHCI_QH * end = uhci_hcd->qhPool + ZLOX_UHCI_MAX_QH;
	for (ZLOX_UHCI_QH * qh = uhci_hcd->qhPool; qh != end; ++qh)
	{
		if (!qh->active)
		{
			qh->active = 1;
			return qh;
		}
	}

	return ZLOX_NULL;
}

// uhci Revision 1.1 spec --- 2.1.7 PORTSC -- PORT STATUS AND CONTROL REGISTER
// at page 16 (PDF top page is 22)
static ZLOX_VOID zlox_uhci_port_set(ZLOX_UINT32 port, ZLOX_UINT16 data)
{
	ZLOX_UINT16 status = zlox_inw(port);
	status |= data;
	// 位1(Connect Status Change)与位3(Port Enable Change) 是由硬件来置位的
	// 不应由软件来置位，不过软件可以通过对这两位写入1，来将这两位清零
	status &= ~ZLOX_UHCI_PORT_RWC;
	zlox_outw(port, status);
}

static ZLOX_VOID zlox_uhci_port_clr(ZLOX_UINT32 port, ZLOX_UINT16 data)
{
	ZLOX_UINT16 status = zlox_inw(port);
	status &= ~ZLOX_UHCI_PORT_RWC;
	status &= ~data;
	status |= ZLOX_UHCI_PORT_RWC & data;
	zlox_outw(port, status);
}

static ZLOX_VOID zlox_uhci_init_td(ZLOX_UHCI_HCD * uhci_hcd, 
			ZLOX_UHCI_TD * td, ZLOX_UHCI_TD * prev,
			ZLOX_UINT32 speed, ZLOX_UINT32 addr, ZLOX_UINT32 endp, 
			ZLOX_UINT32 toggle, ZLOX_UINT32 packetType,
			ZLOX_UINT32 len, ZLOX_UINT32 data_phys)
{
	// uhci Revision 1.1 spec --- 3.2.3 TD TOKEN (DWORD 2: 08-0Bh) ---
	// <Maximum Length (MaxLen)> at page 24 (PDF top page is 30)
	len = (len - 1) & 0x7ff;

	ZLOX_UINT32 td_offset = 0;
	ZLOX_UINT32 td_phys = 0;
	if (prev)
	{
		td_offset = (ZLOX_UINT32)td - (ZLOX_UINT32)uhci_hcd->tdPool;
		td_phys = uhci_hcd->tdPoolPhys + td_offset;
		prev->link = td_phys | ZLOX_UHCI_TD_PTR_DEPTH;
		prev->tdNext = (ZLOX_UINT32)td;
	}

	// uhci Revision 1.1 spec --- 3.2.1 TD LINK POINTER (DWORD 0: 00-03h)
	// <Link Pointer (LP)> at page 21 (PDF top page is 27)
	td->link = ZLOX_UHCI_TD_PTR_TERMINATE;
	td->tdNext = 0;

	// uhci Revision 1.1 spec --- 3.2.2 TD CONTROL AND STATUS (DWORD 1: 04-07h)
	// at page 22 (PDF top page is 28)
	// the ZLOX_UHCI_TD_CS_ERROR_SHIFT is bit <28:27>
	// and the ZLOX_UHCI_TD_CS_ACTIVE is bit <23 (Active.)>
	td->cs = (3 << ZLOX_UHCI_TD_CS_ERROR_SHIFT) | 
			ZLOX_UHCI_TD_CS_ACTIVE;
	// the ZLOX_UHCI_TD_CS_LOW_SPEED is bit <26 (Low Speed Device (LS).)>
	if (speed == ZLOX_USB_LOW_SPEED)
	{
		td->cs |= ZLOX_UHCI_TD_CS_LOW_SPEED;
	}

	// toggle is used to guarantee data sequence synchronization 
	// between data transmitter and receiver
	// see USB Revision 1.1 spec --- 8.6 Data Toggle Synchronization and Retry
	// at page 168 (PDF top page is 184)
	td->token =
		(len << ZLOX_UHCI_TD_TOK_MAXLEN_SHIFT) |
		(toggle << ZLOX_UHCI_TD_TOK_D_SHIFT) |
		(endp << ZLOX_UHCI_TD_TOK_ENDP_SHIFT) |
		(addr << ZLOX_UHCI_TD_TOK_DEVADDR_SHIFT) |
		packetType;

	// uhci Revision 1.1 spec --- 3.2.4 TD BUFFER POINTER (DWORD 3: 0C-0Fh)
	// at page 25 (PDF top page is 31)
	// The data physical buffer may be byte-aligned.
	td->buffer = data_phys;
}

static ZLOX_VOID zlox_uhci_init_qh(ZLOX_UHCI_HCD * uhci_hcd, 
				ZLOX_UHCI_QH * qh, 
				ZLOX_USB_TRANSFER * t, 
				ZLOX_UHCI_TD * td)
{
	ZLOX_UINT32 td_offset = (ZLOX_UINT32)td - (ZLOX_UINT32)uhci_hcd->tdPool;
	ZLOX_UINT32 td_phys = uhci_hcd->tdPoolPhys + td_offset;
	qh->transfer = t;
	qh->tdHead = (ZLOX_UINT32)td;
	qh->element = td_phys;
}

static ZLOX_VOID zlox_uhci_insert_qh(ZLOX_UHCI_HCD * uhci_hcd, 
				ZLOX_UHCI_QH * qh)
{
	ZLOX_UHCI_QH * list = uhci_hcd->asyncQH;
	ZLOX_UHCI_QH * end = ZLOX_USB_LINK_DATA(list->qhLink.prev, ZLOX_UHCI_QH, qhLink);

	ZLOX_UINT32 qh_phys = zlox_usb_get_phys((ZLOX_UINT32)qh);
	qh->head = ZLOX_UHCI_TD_PTR_TERMINATE;
	end->head = qh_phys | ZLOX_UHCI_TD_PTR_QH;

	zlox_usb_link_before(&list->qhLink, &qh->qhLink);
}

static ZLOX_VOID zlox_uhci_remove_qh(ZLOX_UHCI_QH * qh)
{
	ZLOX_UHCI_QH * prev = ZLOX_USB_LINK_DATA(qh->qhLink.prev, ZLOX_UHCI_QH, qhLink);

	prev->head = qh->head;
	zlox_usb_link_remove(&qh->qhLink);
}

static ZLOX_VOID zlox_uhci_free_td(ZLOX_UHCI_TD * td)
{
	td->active = 0;
}

static ZLOX_VOID zlox_uhci_free_qh(ZLOX_UHCI_QH * qh)
{
	qh->active = 0;
}

static ZLOX_VOID zlox_uhci_process_qh(ZLOX_UHCI_HCD * uhci_hcd, ZLOX_UHCI_QH * qh)
{
	ZLOX_USB_TRANSFER * t = qh->transfer;

	ZLOX_UINT32 td_phys = (qh->element & ~0xf);
	ZLOX_UINT32 td_offset = td_phys - uhci_hcd->tdPoolPhys;
	ZLOX_UHCI_TD * td = (ZLOX_UHCI_TD *)((ZLOX_UINT32)uhci_hcd->tdPool + td_offset);
	if (!td_phys)
	{
		t->success = ZLOX_TRUE;
		t->complete = ZLOX_TRUE;
	}
	else if (~td->cs & ZLOX_UHCI_TD_CS_ACTIVE)
	{
		if (td->cs & ZLOX_UHCI_TD_CS_NAK)
		{
			zlox_monitor_write("UHCI NAK\n");
		}
		if (td->cs & ZLOX_UHCI_TD_CS_STALLED)
		{
			zlox_monitor_write("UHCI TD is stalled\n");
			t->success = ZLOX_FALSE;
			t->complete = ZLOX_TRUE;
		}

		if (td->cs & ZLOX_UHCI_TD_CS_DATABUFFER)
		{
			zlox_monitor_write("UHCI TD data buffer error\n");
		}
		if (td->cs & ZLOX_UHCI_TD_CS_BABBLE)
		{
			zlox_monitor_write("UHCI TD babble error\n");
		}
		if (td->cs & ZLOX_UHCI_TD_CS_CRC_TIMEOUT)
		{
			zlox_monitor_write("UHCI TD timeout error\n");
		}
		if (td->cs & ZLOX_UHCI_TD_CS_BITSTUFF)
		{
			zlox_monitor_write("UHCI TD bitstuff error\n");
		}
	}

	if (t->complete)
	{
		// Clear transfer from queue
		qh->transfer = 0;

		// Update endpoint toggle state
		if (t->success && t->endp)
		{
			t->endp->toggle ^= 1;
		}

		// Remove queue from schedule
		zlox_uhci_remove_qh(qh);

		// Free transfer descriptors
		ZLOX_UHCI_TD * td = (ZLOX_UHCI_TD *)qh->tdHead;
		while (td)
		{
			ZLOX_UHCI_TD * next = (ZLOX_UHCI_TD *)td->tdNext;
			zlox_uhci_free_td(td);
			td = next;
		}

		// Free queue head
		zlox_uhci_free_qh(qh);
	}
}

static ZLOX_VOID zlox_uhci_wait_for_qh(ZLOX_UHCI_HCD * uhci_hcd, ZLOX_UHCI_QH * qh)
{
	ZLOX_USB_TRANSFER * t = qh->transfer;

	while (!t->complete)
	{
		zlox_uhci_process_qh(uhci_hcd, qh);
	}
}

static ZLOX_UINT32 zlox_uhci_reset_port(ZLOX_UHCI_HCD * uhci_hcd, ZLOX_UINT32 port)
{
	ZLOX_UINT32 reg = ZLOX_UHCI_REG_PORT1 + port * 2;

	// Reset the port
	zlox_uhci_port_set(uhci_hcd->io_addr + reg, ZLOX_UHCI_PORT_RESET);
	//zlox_timer_sleep(50, ZLOX_FALSE); // if use 1ms / tick
	zlox_timer_sleep(3, ZLOX_FALSE); // if use 20ms / tick, so 3 is 3 * 20 = 60ms
	zlox_uhci_port_clr(uhci_hcd->io_addr + reg, ZLOX_UHCI_PORT_RESET);

	// Wait 100ms for port to enable (TODO - what is appropriate length of time?)
	ZLOX_UINT32 status = 0;
	// for (ZLOX_UINT32 i = 0; i < 10; ++i) // if use 1ms / tick
	for (ZLOX_UINT32 i = 0; i < 5; ++i)
	{
		// Delay
		//zlox_timer_sleep(10, ZLOX_FALSE); // if use 1ms / tick
		zlox_timer_sleep(1, ZLOX_FALSE); // if use 20ms / tick, so 1 is 1 * 20 = 20ms

		// Get current status
		status = zlox_inw(uhci_hcd->io_addr + reg);

		// Check if device is attached to port
		if (~status & ZLOX_UHCI_PORT_CONNECTION)
		{
			break;
		}

		// Acknowledge change in status
		if (status & (ZLOX_UHCI_PORT_ENABLE_CHANGE | 
				ZLOX_UHCI_PORT_CONNECTION_CHANGE))
		{
			zlox_uhci_port_clr(uhci_hcd->io_addr + reg, 
				ZLOX_UHCI_PORT_ENABLE_CHANGE | 
				ZLOX_UHCI_PORT_CONNECTION_CHANGE);
			continue;
		}

		// Check if device is enabled
		if (status & ZLOX_UHCI_PORT_ENABLE)
		{
			break;
		}

		// Enable the port
		zlox_uhci_port_set(uhci_hcd->io_addr + reg, 
				ZLOX_UHCI_PORT_ENABLE);
	}

	return status;
}

// USB Revision 1.1 spec --- 5.5 Control Transfers
// at page 36 (PDF top page is 52)
// and see 8.5.2 Control Transfers
// at page 164 (PDF top page is 180)
static ZLOX_VOID zlox_uhci_dev_control(ZLOX_USB_DEVICE * dev, 
					ZLOX_USB_TRANSFER * t)
{
	ZLOX_UHCI_HCD * uhci_hcd = (ZLOX_UHCI_HCD *)dev->hcd;
	ZLOX_USB_DEV_REQ * req = t->req;

	if(!zlox_usb_detect_phys_continuous((ZLOX_UINT32)req, 
			sizeof(ZLOX_USB_DEV_REQ)))
	{
		return;
	}

	if(!(t->data == ZLOX_NULL && req->len == 0))
	{
		if(!zlox_usb_detect_phys_continuous((ZLOX_UINT32)t->data, req->len))
		{
			return;
		}
	}

	ZLOX_UINT32 req_phys = zlox_usb_get_phys((ZLOX_UINT32)req);

	// Determine transfer properties
	ZLOX_UINT32 speed = dev->speed;
	ZLOX_UINT32 addr = dev->addr;
	ZLOX_UINT32 endp = 0;
	ZLOX_UINT32 maxSize = dev->maxPacketSize;
	ZLOX_UINT32 type = req->type;
	ZLOX_UINT32 len = req->len;

	// Create queue of transfer descriptors
	ZLOX_UHCI_TD * td = zlox_uhci_alloc_td(uhci_hcd);
	if (!td)
	{
		return;
	}

	ZLOX_UHCI_TD * head = td;
	ZLOX_UHCI_TD * prev = ZLOX_NULL;

	// Setup packet
	ZLOX_UINT32 toggle = 0;
	// USB Revision 1.1 spec --- 8.3.1 Packet Identifier Field
	// at page 155 (PDF top page is 171)
	// and see <Table 8-1. PID Types> at page 156 (PDF top page is 172)
	ZLOX_UINT32 packetType = ZLOX_UHCI_TD_PACKET_SETUP;
	ZLOX_UINT32 packetSize = sizeof(ZLOX_USB_DEV_REQ);
	zlox_uhci_init_td(uhci_hcd, 
			td, prev, speed, addr, endp, 
			toggle, packetType,
			packetSize, req_phys);
	prev = td;

	// Data in/out packets
	packetType = (type & ZLOX_USB_RT_DEV_TO_HOST) ? ZLOX_UHCI_TD_PACKET_IN : 
			ZLOX_UHCI_TD_PACKET_OUT;

	ZLOX_UINT8 *it = (ZLOX_UINT8 *)t->data;
	ZLOX_UINT8 *end = it + len;
	ZLOX_UINT32 it_phys = 0;
	while (it < end)
	{
		td = zlox_uhci_alloc_td(uhci_hcd);
		if (!td)
		{
			return;
		}

		toggle ^= 1;
		packetSize = end - it;
		if (packetSize > maxSize)
		{
			packetSize = maxSize;
		}

		it_phys = zlox_usb_get_phys((ZLOX_UINT32)it);
		zlox_uhci_init_td(uhci_hcd, 
			td, prev, speed, addr, endp, 
			toggle, packetType,
			packetSize, it_phys);

		it += packetSize;
		prev = td;
	}

	// Status packet
	td = zlox_uhci_alloc_td(uhci_hcd);
	if (!td)
	{
		return;
	}

	toggle = 1;
	packetType = (type & ZLOX_USB_RT_DEV_TO_HOST) ? ZLOX_UHCI_TD_PACKET_OUT : 
				ZLOX_UHCI_TD_PACKET_IN;
	zlox_uhci_init_td(uhci_hcd, 
			td, prev, speed, addr, endp, 
			toggle, packetType,
			0, 0);

	// Initialize queue head
	ZLOX_UHCI_QH * qh = zlox_uhci_alloc_qh(uhci_hcd);
	zlox_uhci_init_qh(uhci_hcd, qh, t, head);

	// Wait until queue has been processed
	zlox_uhci_insert_qh(uhci_hcd, qh);
	zlox_uhci_wait_for_qh(uhci_hcd, qh);
}

static ZLOX_VOID zlox_uhci_dev_intr(ZLOX_USB_DEVICE * dev, 
					ZLOX_USB_TRANSFER * t)
{
	ZLOX_UHCI_HCD * uhci_hcd = (ZLOX_UHCI_HCD *)dev->hcd;

	// Determine transfer properties
	ZLOX_UINT32 speed = dev->speed;
	ZLOX_UINT32 addr = dev->addr;
	ZLOX_UINT32 endp = dev->endp.desc.addr & 0xf;

	// Create queue of transfer descriptors
	ZLOX_UHCI_TD * td = zlox_uhci_alloc_td(uhci_hcd);
	if (!td)
	{
		t->success = ZLOX_FALSE;
		t->complete = ZLOX_TRUE;
		return;
	}

	ZLOX_UHCI_TD * head = td;
	ZLOX_UHCI_TD * prev = ZLOX_NULL;

	// Data in/out packets
	ZLOX_UINT32 toggle = dev->endp.toggle;
	ZLOX_UINT32 packetType = ZLOX_UHCI_TD_PACKET_IN;
	ZLOX_UINT32 packetSize = t->len;
	ZLOX_UINT32 data_phys = zlox_usb_get_phys((ZLOX_UINT32)t->data);

	zlox_uhci_init_td(uhci_hcd, 
			td, prev, speed, addr, endp, 
			toggle, packetType,
			packetSize, data_phys);

	// Initialize queue head
	ZLOX_UHCI_QH * qh = zlox_uhci_alloc_qh(uhci_hcd);
	zlox_uhci_init_qh(uhci_hcd, qh, t, head);

	// Schedule queue
	zlox_uhci_insert_qh(uhci_hcd, qh);
}

static ZLOX_VOID zlox_uhci_probe(ZLOX_UHCI_HCD * uhci_hcd)
{
	ZLOX_UINT32 portCount = uhci_hcd->rh_numports;
	for (ZLOX_UINT32 port = 0; port < portCount; ++port)
	{
		// Reset port
		ZLOX_UINT32 status = zlox_uhci_reset_port(uhci_hcd, port);

		if (~status & ZLOX_UHCI_PORT_ENABLE)
			continue;

		ZLOX_UINT32 speed = (status & ZLOX_UHCI_PORT_LSDA) ? 
					ZLOX_USB_LOW_SPEED : ZLOX_USB_FULL_SPEED;

		ZLOX_USB_DEVICE * dev = zlox_usb_dev_create();
		if(dev == ZLOX_NULL)
			continue;

		dev->parent = 0;
		dev->hcd = uhci_hcd;
		dev->port = port;
		dev->speed = speed;
		dev->maxPacketSize = 8;

		dev->hcControl = zlox_uhci_dev_control;
		dev->hcIntr = zlox_uhci_dev_intr;

		if (!zlox_usb_dev_init(dev))
		{
			// TODO - cleanup
		}
	}
}

ZLOX_VOID zlox_uhci_poll(ZLOX_USB_HCD * usb_hcd)
{
	ZLOX_UHCI_HCD * uhci_hcd = (ZLOX_UHCI_HCD *)usb_hcd->hcd_priv;
	ZLOX_UHCI_QH *qh;
	ZLOX_UHCI_QH *next;
	ZLOX_USB_LIST_FOR_EACH_SAFE(qh, next, uhci_hcd->asyncQH->qhLink, qhLink)
	{
		if (qh->transfer)
		{
			zlox_uhci_process_qh(uhci_hcd, qh);
		}
	}
}

ZLOX_VOID * zlox_uhci_init(ZLOX_USB_HCD * usb_hcd)
{
	if(usb_hcd == ZLOX_NULL)
		return ZLOX_NULL;

	ZLOX_USB_GET_EXT_INFO ext_info;
	zlox_usb_get_ext_info(usb_hcd, &ext_info);

	ZLOX_UHCI_HCD * uhci_hcd = (ZLOX_UHCI_HCD *)zlox_kmalloc(sizeof(ZLOX_UHCI_HCD));
	if(uhci_hcd == ZLOX_NULL)
		return ZLOX_NULL;

	zlox_pci_set_master(ext_info.cfg_addr, ZLOX_TRUE);

	zlox_memset((ZLOX_UINT8 *)uhci_hcd, 0, sizeof(ZLOX_UHCI_HCD));
	uhci_hcd->io_addr = ext_info.cfg_hdr->bars[ZLOX_UHCI_BAR_IDX] & 
					ZLOX_PCI_BASE_ADDRESS_IO_MASK;
	uhci_hcd->irq = usb_hcd->irq = (ZLOX_UINT32)ext_info.cfg_hdr->int_line;
	uhci_hcd->irq_pin = usb_hcd->irq_pin = (ZLOX_UINT32)ext_info.cfg_hdr->int_pin;
	uhci_hcd->io_len = zlox_pci_get_bar_size(ext_info.cfg_addr, 
					ZLOX_UHCI_BAR_IDX) + 1;
	uhci_hcd->rh_numports = zlox_uhci_count_ports(uhci_hcd);
	uhci_hcd->usb_hcd = (ZLOX_VOID *)usb_hcd;

	uhci_hcd->frameList = (ZLOX_UINT32 *)zlox_kmalloc_ap(1024 * sizeof(ZLOX_UINT32), 
					&uhci_hcd->frameListPhys);
	uhci_hcd->qhPool = (ZLOX_UHCI_QH *)zlox_kmalloc_ap(sizeof(ZLOX_UHCI_QH) * 
					ZLOX_UHCI_MAX_QH, 
					&uhci_hcd->qhPoolPhys);
	uhci_hcd->tdPool = (ZLOX_UHCI_TD *)zlox_kmalloc_ap(sizeof(ZLOX_UHCI_TD) * 
					ZLOX_UHCI_MAX_TD, 
					&uhci_hcd->tdPoolPhys);
	zlox_memset((ZLOX_UINT8 *)uhci_hcd->qhPool, 0, sizeof(ZLOX_UHCI_QH) * 
					ZLOX_UHCI_MAX_QH);
	zlox_memset((ZLOX_UINT8 *)uhci_hcd->tdPool, 0, sizeof(ZLOX_UHCI_TD) * 
					ZLOX_UHCI_MAX_TD);

	// Frame list setup
	ZLOX_UHCI_QH * qh = zlox_uhci_alloc_qh(uhci_hcd);
	// 在4K大小的页范围内，都可以直接通过线性地址之间的偏移值来确定物理地址
	ZLOX_UINT32 qh_offset = (ZLOX_UINT32)qh - (ZLOX_UINT32)uhci_hcd->qhPool;
	ZLOX_UINT32 qh_phys = uhci_hcd->qhPoolPhys + qh_offset;
	qh->head = ZLOX_UHCI_TD_PTR_TERMINATE;
	qh->element = ZLOX_UHCI_TD_PTR_TERMINATE;
	qh->transfer = 0;
	qh->qhLink.prev = &qh->qhLink;
	qh->qhLink.next = &qh->qhLink;

	uhci_hcd->asyncQH = qh;
	uhci_hcd->asyncQH_Phys = qh_phys;
	for (ZLOX_UINT32 i = 0; i < 1024; ++i)
	{
		uhci_hcd->frameList[i] = ZLOX_UHCI_TD_PTR_QH | qh_phys;
	}

	// Turn off PIRQ enable and SMI enable.  (This also turns off the
	// BIOS's USB Legacy Support.)  Turn off all the R/WC bits too.
	// uhci Revision 1.1 spec --- 5.2.1 LEGSUP -- LEGACY SUPPORT REGISTER 
	// at page 39 (PDF top page is 45)
	zlox_pci_reg_outw(ext_info.cfg_addr, ZLOX_UHCI_PCI_REG_LEGSUP,
					ZLOX_UHCI_PCI_LEGSUP_RWC);

	// Disable interrupts
	// uhci Revision 1.1 spec --- 2.1.3 USBINTR -- USB INTERRUPT ENABLE REGISTER
	// at page 14 (PDF top page is 20)
	zlox_outw(uhci_hcd->io_addr + ZLOX_UHCI_REG_INTR, 0);

	// Assign frame list
	// uhci Revision 1.1 spec --- 2.1.4 FRNUM -- FRAME NUMBER REGISTER
	// at page 14 (PDF top page is 20)
	zlox_outw(uhci_hcd->io_addr + ZLOX_UHCI_REG_FRNUM, 0);
	// uhci Revision 1.1 spec --- 2.1.5 FLBASEADD -- FRAME LIST BASE ADDRESS REGISTER
	// at page 15 (PDF top page is 21)
	zlox_outl(uhci_hcd->io_addr + ZLOX_UHCI_REG_FRBASEADD, 
			uhci_hcd->frameListPhys);
	// uhci Revision 1.1 spec --- 2.1.6 START OF FRAME (SOF) MODIFY REGISTER
	// at page 15 (PDF top page is 21)
	zlox_outb(uhci_hcd->io_addr + ZLOX_UHCI_REG_SOFMOD, 0x40);

	// Clear status
	// uhci Revision 1.1 spec --- 2.1.2 USBSTS -- USB STATUS REGISTER
	// at page 13 (PDF top page is 19)
	zlox_outw(uhci_hcd->io_addr + ZLOX_UHCI_REG_STS, 0xffff);

	// Enable controller
	// uhci Revision 1.1 spec --- 2.1.1 USBCMD -- USB COMMAND REGISTER
	// at page 11 (PDF top page is 17)
	zlox_outw(uhci_hcd->io_addr + ZLOX_UHCI_REG_CMD, ZLOX_UHCI_CMD_RS);

	// Probe devices
	zlox_uhci_probe(uhci_hcd);

	usb_hcd->poll = zlox_uhci_poll;
	return (ZLOX_VOID *)uhci_hcd;
}

