// zlox_usb.c -- the C file relate to USB

#include "zlox_usb.h"
#include "zlox_uhci.h"
#include "zlox_usb_mouse.h"
#include "zlox_usb_hub.h"
#include "zlox_usb_keyboard.h"
#include "zlox_monitor.h"
#include "zlox_kheap.h"
#include "zlox_paging.h"
#include "zlox_time.h"

ZLOX_USB_HCD * usb_hcd_lst = ZLOX_NULL;
ZLOX_USB_DEVICE * g_usbDeviceList = ZLOX_NULL;
static ZLOX_UINT32 s_nextUsbAddr = 0;
// The current page directory;
extern ZLOX_PAGE_DIRECTORY * current_directory;

const ZLOX_USB_DRIVER g_usbDriverTable[] =
{
	{ zlox_usb_hub_init },
	{ zlox_usb_kbd_init },
	{ zlox_usb_mouse_init },
	{ ZLOX_NULL }
};

ZLOX_USB_HCD * zlox_usb_hcd_alloc(ZLOX_PCI_DEVCONF_LST * pciconf_lst, 
				ZLOX_UINT32 pciconf_lst_idx)
{
	ZLOX_USB_HCD * usb_hcd = (ZLOX_USB_HCD *)zlox_kmalloc(sizeof(ZLOX_USB_HCD));
	if(usb_hcd == ZLOX_NULL)
		return ZLOX_NULL;
	zlox_memset((ZLOX_UINT8 *)usb_hcd, 0, sizeof(ZLOX_USB_HCD));
	usb_hcd->pciconf_lst = pciconf_lst;
	usb_hcd->pciconf_lst_idx = pciconf_lst_idx;
	return usb_hcd;
}

ZLOX_USB_HCD * zlox_usb_hcd_lst_append(ZLOX_USB_HCD * usb_hcd)
{
	if(usb_hcd == ZLOX_NULL)
		return ZLOX_NULL;
	if(usb_hcd_lst == ZLOX_NULL)
	{
		usb_hcd_lst = usb_hcd;
		return usb_hcd;
	}
	ZLOX_USB_HCD * iter = usb_hcd_lst;
	while(ZLOX_TRUE)
	{
		if(iter->next == ZLOX_NULL)
		{
			iter->next = usb_hcd;
			return usb_hcd;
		}
		else
			iter = iter->next;
	}
	return ZLOX_NULL;
}

ZLOX_SINT32 zlox_usb_get_ext_info(ZLOX_USB_HCD * usb_hcd, 
				ZLOX_USB_GET_EXT_INFO * ext_info)
{
	if(usb_hcd == ZLOX_NULL)
		return -1;
	if(ext_info == ZLOX_NULL)
		return -1;
	ext_info->cfg_hdr = ZLOX_USB_GET_CFG_HDR(usb_hcd);
	ext_info->cfg_addr = ZLOX_USB_GET_CFG_ADDR(usb_hcd);
	return 0;
}

ZLOX_SINT32 zlox_usb_print_devinfo(ZLOX_USB_HCD * usb_hcd)
{
	if(usb_hcd == ZLOX_NULL)
		return -1;

	ZLOX_UNI_PCI_CFG_ADDR cfg_addr = ZLOX_USB_GET_CFG_ADDR(usb_hcd);

	zlox_monitor_write("USB bus [");
	zlox_monitor_write_dec((ZLOX_UINT32)cfg_addr.bus);
	zlox_monitor_write("] dev [");
	zlox_monitor_write_dec((ZLOX_UINT32)cfg_addr.device);
	zlox_monitor_write("] func [");
	zlox_monitor_write_dec((ZLOX_UINT32)cfg_addr.function);
	zlox_monitor_write("] ");

	switch(usb_hcd->type)
	{
	case ZLOX_USB_HCD_TYPE_UHCI:
		zlox_uhci_print_devinfo((ZLOX_UHCI_HCD *)usb_hcd->hcd_priv);
		break;
	default:
		break;
	}

	zlox_monitor_write("\n");
	return 0;
}

ZLOX_VOID zlox_usb_print_device_desc(ZLOX_USB_DEVICE_DESC * desc)
{
	zlox_monitor_write("USB: Version=");
	zlox_monitor_write_dec(desc->usbVer >> 8);
	zlox_monitor_write(".");
	zlox_monitor_write_dec((desc->usbVer >> 4) & 0xf);
	zlox_monitor_write(" Vendor ID=");
	zlox_monitor_write_hex(desc->vendorId);
	zlox_monitor_write(" Product ID=");
	zlox_monitor_write_hex(desc->productId);
	zlox_monitor_write(" Configs=");
	zlox_monitor_write_dec(desc->confCount);
	zlox_monitor_write("\n");
}

ZLOX_VOID zlox_usb_print_conf_desc(ZLOX_USB_CONF_DESC * desc)
{
	zlox_monitor_write("  Conf: totalLen=");
	zlox_monitor_write_dec(desc->totalLen);
	zlox_monitor_write(" intfCount=");
	zlox_monitor_write_dec(desc->intfCount);
	zlox_monitor_write(" confValue=");
	zlox_monitor_write_dec(desc->confValue);
	zlox_monitor_write(" confStr=");
	zlox_monitor_write_dec(desc->confStr);
	zlox_monitor_write("\n");
}

ZLOX_VOID zlox_usb_print_intf_desc(ZLOX_USB_INTF_DESC * desc)
{
	zlox_monitor_write("  Intf: altSetting=");
	zlox_monitor_write_dec(desc->altSetting);
	zlox_monitor_write(" endpCount=");
	zlox_monitor_write_dec(desc->endpCount);
	zlox_monitor_write(" class=");
	zlox_monitor_write_dec(desc->intfClass);
	zlox_monitor_write(" subclass=");
	zlox_monitor_write_dec(desc->intfSubClass);
	zlox_monitor_write(" protocol=");
	zlox_monitor_write_dec(desc->intfProtocol);
	zlox_monitor_write(" str=");
	zlox_monitor_write_dec(desc->intfStr);
	zlox_monitor_write("\n");
}

ZLOX_VOID zlox_usb_print_endp_desc(ZLOX_USB_ENDP_DESC * desc)
{
	zlox_monitor_write("  Endp: addr=");
	zlox_monitor_write_hex(desc->addr);
	zlox_monitor_write(" attributes=");
	zlox_monitor_write_dec(desc->attributes);
	zlox_monitor_write(" maxPacketSize=");
	zlox_monitor_write_dec(desc->maxPacketSize);
	zlox_monitor_write(" interval=");
	zlox_monitor_write_dec(desc->interval);
	zlox_monitor_write("\n");
}

ZLOX_USB_DEVICE * zlox_usb_dev_create()
{
	ZLOX_USB_DEVICE * dev = (ZLOX_USB_DEVICE *)zlox_kmalloc(sizeof(ZLOX_USB_DEVICE));
	if(dev == ZLOX_NULL)
		return ZLOX_NULL;

	zlox_memset((ZLOX_UINT8 *)dev, 0, sizeof(ZLOX_USB_DEVICE));
	dev->next = g_usbDeviceList;
	g_usbDeviceList = dev;
	return dev;
}

ZLOX_BOOL zlox_usb_detect_phys_continuous(ZLOX_UINT32 data_addr, 
				ZLOX_UINT32 data_len)
{
	if(data_addr == 0 || data_len == 0)
		return ZLOX_FALSE;

	if(data_len > 4096)
		return ZLOX_FALSE;
	ZLOX_PAGE * page = zlox_get_page(data_addr, 0, current_directory);
	ZLOX_UINT32 start_frame = page->frame;
	ZLOX_UINT32 data_end_addr = data_addr + data_len - 1;
	page = zlox_get_page(data_end_addr, 0, current_directory);
	ZLOX_UINT32 end_frame = page->frame;
	if((end_frame == start_frame) || 
	   (end_frame == (start_frame + 1))
	  )
	{
		return ZLOX_TRUE;
	}
	return ZLOX_FALSE;
}

ZLOX_VOID * zlox_usb_get_cont_phys(ZLOX_UINT32 data_len)
{
	if(data_len == 0)
		return ZLOX_NULL;

	if(data_len > 4096)
		return ZLOX_NULL;
	ZLOX_UINT32 ret = (ZLOX_UINT32)zlox_kmalloc(data_len);
	if(ret == 0)
		return ZLOX_NULL;
	if(!zlox_usb_detect_phys_continuous(ret, data_len))
	{
		zlox_kfree((ZLOX_VOID *)ret);
		ret = (ZLOX_UINT32)zlox_kmalloc_a(data_len);
	}
	return (ZLOX_VOID *)ret;
}

ZLOX_VOID zlox_usb_free_cont_phys(ZLOX_VOID * p)
{
	zlox_kfree(p);
}

ZLOX_UINT32 zlox_usb_get_phys(ZLOX_UINT32 addr)
{
	ZLOX_PAGE * page = zlox_get_page(addr, 0, current_directory);
	ZLOX_UINT32 phys_addr = (page->frame * 0x1000) + (addr & 0xFFF);
	return phys_addr;
}

ZLOX_VOID zlox_usb_link_before(ZLOX_USB_LINK * a, ZLOX_USB_LINK * x)
{
	ZLOX_USB_LINK * p = a->prev;
	ZLOX_USB_LINK * n = a;
	n->prev = x;
	x->next = n;
	x->prev = p;
	p->next = x;
	return;
}

ZLOX_VOID zlox_usb_link_remove(ZLOX_USB_LINK * x)
{
	ZLOX_USB_LINK * p = x->prev;
	ZLOX_USB_LINK * n = x->next;
	n->prev = p;
	p->next = n;
	x->next = 0;
	x->prev = 0;
	return;
}

ZLOX_BOOL zlox_usb_dev_request(ZLOX_USB_DEVICE * dev,
    ZLOX_UINT32 type, ZLOX_UINT32 request,
    ZLOX_UINT32 value, ZLOX_UINT32 index,
    ZLOX_UINT32 len, ZLOX_VOID * data)
{
	if(!(data == ZLOX_NULL && len == 0))
	{
		if(!zlox_usb_detect_phys_continuous((ZLOX_UINT32)data, len))
		{
			return ZLOX_FALSE;
		}
	}
	ZLOX_USB_DEV_REQ req;
	ZLOX_USB_DEV_REQ * req_ptr = ZLOX_NULL;
	req_ptr = &req;
	if(!zlox_usb_detect_phys_continuous((ZLOX_UINT32)req_ptr, 
			sizeof(ZLOX_USB_DEV_REQ)))
	{
		req_ptr = zlox_usb_get_cont_phys(sizeof(ZLOX_USB_DEV_REQ));
		if(req_ptr == ZLOX_NULL)
			return ZLOX_FALSE;
	}

	req_ptr->type = type;
	req_ptr->req = request;
	req_ptr->value = value;
	req_ptr->index = index;
	req_ptr->len = len;

	ZLOX_USB_TRANSFER t;
	t.endp = 0;
	t.req = req_ptr;
	t.data = data;
	t.len = len;
	t.complete = ZLOX_FALSE;
	t.success = ZLOX_FALSE;

	dev->hcControl(dev, &t);

	if(req_ptr != (&req))
	{
		zlox_usb_free_cont_phys((ZLOX_VOID *)req_ptr);
	}
	return t.success;
}

ZLOX_BOOL zlox_usb_dev_get_langs(ZLOX_USB_DEVICE * dev, ZLOX_UINT16 * langs)
{
	ZLOX_BOOL ret = ZLOX_FALSE;
	ZLOX_USB_STRING_DESC * desc = (ZLOX_USB_STRING_DESC *)zlox_usb_get_cont_phys(256);

	if(desc == ZLOX_NULL)
		return ZLOX_FALSE;

	langs[0] = 0;

	// Get length
	if (!zlox_usb_dev_request(dev,
		ZLOX_USB_RT_DEV_TO_HOST | ZLOX_USB_RT_STANDARD | ZLOX_USB_RT_DEV,
		ZLOX_USB_REQ_GET_DESC, (ZLOX_USB_DESC_TYPE_STRING << 8) | 0, 0,
		1, desc))
	{
		goto failed;
	}

	// Get lang data
	if (!zlox_usb_dev_request(dev,
		ZLOX_USB_RT_DEV_TO_HOST | ZLOX_USB_RT_STANDARD | ZLOX_USB_RT_DEV,
		ZLOX_USB_REQ_GET_DESC, (ZLOX_USB_DESC_TYPE_STRING << 8) | 0, 0,
		desc->len, desc))
	{
		goto failed;
	}

	ZLOX_UINT32 langLen = (desc->len - 2) / 2;
	for (ZLOX_UINT32 i = 0; i < langLen; ++i)
	{
		langs[i] = desc->str[i];
	}

	langs[langLen] = 0;
	ret = ZLOX_TRUE;

failed:
	if(desc != ZLOX_NULL)
	{
		zlox_usb_free_cont_phys((ZLOX_VOID *)desc);
	}
	return ret;
}

ZLOX_BOOL zlox_usb_dev_get_string(ZLOX_USB_DEVICE * dev, ZLOX_CHAR * str, 
				ZLOX_UINT32 langId, ZLOX_UINT32 strIndex)
{
	str[0] = ZLOX_NULL;
	if (!strIndex)
	{
		return ZLOX_TRUE;
	}

	ZLOX_BOOL ret = ZLOX_FALSE;
	ZLOX_USB_STRING_DESC * desc = (ZLOX_USB_STRING_DESC *)zlox_usb_get_cont_phys(256);

	if(desc == ZLOX_NULL)
		return ZLOX_FALSE;

	// Get string length
	if (!zlox_usb_dev_request(dev,
		ZLOX_USB_RT_DEV_TO_HOST | ZLOX_USB_RT_STANDARD | ZLOX_USB_RT_DEV,
		ZLOX_USB_REQ_GET_DESC, (ZLOX_USB_DESC_TYPE_STRING << 8) | strIndex, langId,
		1, desc))
	{
		goto failed;
	}

	// Get string data
	if (!zlox_usb_dev_request(dev,
		ZLOX_USB_RT_DEV_TO_HOST | ZLOX_USB_RT_STANDARD | ZLOX_USB_RT_DEV,
		ZLOX_USB_REQ_GET_DESC, (ZLOX_USB_DESC_TYPE_STRING << 8) | strIndex, langId,
		desc->len, desc))
	{
		goto failed;
	}

	// Dumb Unicode to ASCII conversion
	ZLOX_UINT32 strLen = (desc->len - 2) / 2;
	for (ZLOX_UINT32 i = 0; i < strLen; ++i)
	{
		str[i] = desc->str[i];
	}

	str[strLen] = ZLOX_NULL;
	ret = ZLOX_TRUE;

failed:
	if(desc != ZLOX_NULL)
	{
		zlox_usb_free_cont_phys((ZLOX_VOID *)desc);
	}
	return ret;
}

ZLOX_BOOL zlox_usb_dev_init(ZLOX_USB_DEVICE * dev)
{
	ZLOX_BOOL ret = ZLOX_FALSE;
	ZLOX_USB_DEVICE_DESC devDesc;
	ZLOX_USB_DEVICE_DESC * devDesc_ptr;
	ZLOX_UINT16 * langs = ZLOX_NULL;
	ZLOX_UINT8 * configBuf = ZLOX_NULL;
	devDesc_ptr = &devDesc;
	if(!zlox_usb_detect_phys_continuous((ZLOX_UINT32)devDesc_ptr, 
			sizeof(ZLOX_USB_DEVICE_DESC)))
	{
		devDesc_ptr = zlox_usb_get_cont_phys(sizeof(ZLOX_USB_DEVICE_DESC));
		if(devDesc_ptr == ZLOX_NULL)
			return ZLOX_FALSE;
	}

	// USB Revision 1.1 spec --- Table 9-3. Standard Device Requests
	// at page 186 (PDF top page is 202)
	// the wValue include <Descriptor Type> and <Descriptor Index>
	// so the follow is (ZLOX_USB_DESC_TYPE_DEVICE << 8) | 0
	// Get first 8 bytes of device descriptor
	if (!zlox_usb_dev_request(dev,
		ZLOX_USB_RT_DEV_TO_HOST | ZLOX_USB_RT_STANDARD | ZLOX_USB_RT_DEV,
		ZLOX_USB_REQ_GET_DESC, (ZLOX_USB_DESC_TYPE_DEVICE << 8) | 0, 0,
		8, devDesc_ptr))
	{
		goto failed;
	}

	dev->maxPacketSize = devDesc_ptr->maxPacketSize;

	// Set address
	ZLOX_UINT32 addr = ++s_nextUsbAddr;

	if (!zlox_usb_dev_request(dev,
		ZLOX_USB_RT_HOST_TO_DEV | ZLOX_USB_RT_STANDARD | ZLOX_USB_RT_DEV,
		ZLOX_USB_REQ_SET_ADDR, addr, 0,
		0, 0))
	{
		goto failed;
	}

	dev->addr = addr;

	// TODO   PitWait(2);
	zlox_timer_sleep(1, ZLOX_FALSE);    // Set address recovery time

	// Read entire descriptor
	if (!zlox_usb_dev_request(dev,
		ZLOX_USB_RT_DEV_TO_HOST | ZLOX_USB_RT_STANDARD | ZLOX_USB_RT_DEV,
		ZLOX_USB_REQ_GET_DESC, (ZLOX_USB_DESC_TYPE_DEVICE << 8) | 0, 0,
		sizeof(ZLOX_USB_DEVICE_DESC), devDesc_ptr))
	{
		goto failed;
	}

	// Dump descriptor
	zlox_usb_print_device_desc(devDesc_ptr);

	// String Info
	langs = (ZLOX_UINT16 *)zlox_kmalloc(
					ZLOX_USB_STRING_SIZE * 
					sizeof(ZLOX_UINT16));

	if(langs == ZLOX_NULL)
		goto failed;

	zlox_usb_dev_get_langs(dev, langs);

	ZLOX_UINT32 langId = langs[0];

	if (langId)
	{
		ZLOX_CHAR tmpstr[ZLOX_USB_STRING_SIZE];
		zlox_usb_dev_get_string(dev, tmpstr, langId, devDesc_ptr->productStr);
		zlox_monitor_write(" Product='");
		zlox_monitor_write(tmpstr);
		zlox_usb_dev_get_string(dev, tmpstr, langId, devDesc_ptr->vendorStr);
		zlox_monitor_write("' Vendor='");
		zlox_monitor_write(tmpstr);
		zlox_usb_dev_get_string(dev, tmpstr, langId, devDesc_ptr->serialStr);
		zlox_monitor_write("' Serial='");
		zlox_monitor_write(tmpstr);
		zlox_monitor_write("'\n");
	}

	// Pick configuration and interface - grab first for now
	configBuf = zlox_usb_get_cont_phys(ZLOX_USB_CONF_BUF_SIZE);
	ZLOX_UINT32 pickedConfValue = 0;
	ZLOX_USB_INTF_DESC * pickedIntfDesc = ZLOX_NULL;
	ZLOX_USB_ENDP_DESC * pickedEndpDesc = ZLOX_NULL;

	for (ZLOX_UINT32 confIndex = 0; confIndex < devDesc_ptr->confCount; ++confIndex)
	{
		// Get configuration total length
		if (!zlox_usb_dev_request(dev,
			ZLOX_USB_RT_DEV_TO_HOST | ZLOX_USB_RT_STANDARD | ZLOX_USB_RT_DEV,
			ZLOX_USB_REQ_GET_DESC, (ZLOX_USB_DESC_TYPE_CONF << 8) | confIndex, 0,
			4, configBuf))
		{
			continue;
		}

		// Only static size supported for now
		ZLOX_USB_CONF_DESC * confDesc = (ZLOX_USB_CONF_DESC *)configBuf;
		if (confDesc->totalLen > ZLOX_USB_CONF_BUF_SIZE)
		{
			zlox_monitor_write("   Configuration total length ");
			zlox_monitor_write_dec(confDesc->totalLen);
			zlox_monitor_write(" greater than ");
			zlox_monitor_write_dec(ZLOX_USB_CONF_BUF_SIZE);
			zlox_monitor_write(" bytes");
			continue;
		}

		// Read all configuration data
		if (!zlox_usb_dev_request(dev,
			ZLOX_USB_RT_DEV_TO_HOST | ZLOX_USB_RT_STANDARD | ZLOX_USB_RT_DEV,
			ZLOX_USB_REQ_GET_DESC, (ZLOX_USB_DESC_TYPE_CONF << 8) | confIndex, 0,
			confDesc->totalLen, configBuf))
		{
			continue;
		}

		zlox_usb_print_conf_desc(confDesc);

		if (!pickedConfValue)
		{
			pickedConfValue = confDesc->confValue;
		}

		// Parse configuration data
		ZLOX_UINT8 * data = configBuf + confDesc->len;
		ZLOX_UINT8 * end = configBuf + confDesc->totalLen;

		while (data < end)
		{
			ZLOX_UINT8 len = data[0];
			ZLOX_UINT8 type = data[1];

			switch (type)
			{
			case ZLOX_USB_DESC_TYPE_INTF:
				{
					ZLOX_USB_INTF_DESC * intfDesc = (ZLOX_USB_INTF_DESC *)data;
					zlox_usb_print_intf_desc(intfDesc);

					if (!pickedIntfDesc)
					{
						pickedIntfDesc = intfDesc;
					}
				}
				break;

			case ZLOX_USB_DESC_TYPE_ENDP:
				{
					ZLOX_USB_ENDP_DESC * endp_desc = (ZLOX_USB_ENDP_DESC *)data;
					zlox_usb_print_endp_desc(endp_desc);

					if (!pickedEndpDesc)
					{
						pickedEndpDesc = endp_desc;
					}
				}
				break;
			}

			data += len;
		}
	}

	// Configure device
	if (pickedConfValue && pickedIntfDesc && pickedEndpDesc)
	{
		if (!zlox_usb_dev_request(dev,
			ZLOX_USB_RT_HOST_TO_DEV | ZLOX_USB_RT_STANDARD | ZLOX_USB_RT_DEV,
			ZLOX_USB_REQ_SET_CONF, pickedConfValue, 0,
			0, 0))
		{
			goto failed;
		}

		dev->intfDesc = *pickedIntfDesc;
		dev->endp.desc = *pickedEndpDesc;

		// Initialize driver
		const ZLOX_USB_DRIVER * driver = g_usbDriverTable;
		while (driver->init)
		{
			if (driver->init(dev))
			{
				break;
			}
			++driver;
		}
	}

	ret = ZLOX_TRUE;

failed:
	if(devDesc_ptr != (&devDesc))
	{
		zlox_usb_free_cont_phys((ZLOX_VOID *)devDesc_ptr);
	}
	if(langs != ZLOX_NULL)
	{
		zlox_kfree((ZLOX_VOID *)langs);
	}
	if(configBuf != ZLOX_NULL)
	{
		zlox_usb_free_cont_phys((ZLOX_VOID *)configBuf);
	}
	return ret;
}

ZLOX_VOID zlox_usb_poll()
{
	for (ZLOX_USB_HCD * usb_hcd = usb_hcd_lst; usb_hcd != ZLOX_NULL; 
		usb_hcd = usb_hcd->next)
	{
		if (usb_hcd->poll)
		{
			usb_hcd->poll(usb_hcd);
		}
	}

	for (ZLOX_USB_DEVICE * dev = g_usbDeviceList; dev!= ZLOX_NULL; 
		dev = dev->next)
	{
		if (dev->drvPoll)
		{
			dev->drvPoll(dev);
		}
	}
}

ZLOX_SINT32 zlox_usb_init()
{
	ZLOX_PCI_DEVCONF_LST * pciconf_lst = zlox_pci_get_devconf_lst_for_kernel();
	if(pciconf_lst->ptr == ZLOX_NULL)
	{
		return -1;
	}
	for(ZLOX_UINT32 i = 0; i < pciconf_lst->count ;i++)
	{
		ZLOX_PCI_CONF_HDR * cfg_hdr = &(pciconf_lst->ptr[i].cfg_hdr);
		if(zlox_uhci_detect(cfg_hdr))
		{
			ZLOX_USB_HCD * usb_hcd = zlox_usb_hcd_alloc(pciconf_lst, i);
			if(usb_hcd == ZLOX_NULL)
			{
				continue;
			}
			ZLOX_VOID * hcd = zlox_uhci_init(usb_hcd);
			if(hcd == ZLOX_NULL)
			{
				zlox_kfree(usb_hcd);
				continue;
			}
			usb_hcd->hcd_priv = hcd;
			usb_hcd->type = ZLOX_USB_HCD_TYPE_UHCI;
			zlox_usb_hcd_lst_append(usb_hcd);
			zlox_usb_print_devinfo(usb_hcd);
		}
	}
	return 0;
}

