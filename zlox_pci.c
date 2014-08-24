// zlox_pci.c 和PCI相关的函数定义

#include "zlox_pci.h"
#include "zlox_monitor.h"
#include "zlox_kheap.h"

#define ZLOX_PCI_ADDRESS 0xCF8
#define ZLOX_PCI_DATA 0xCFC

#define ZLOX_PCI_DEVCONF_SZ 8

ZLOX_PCI_DEVCONF_LST pci_devconf_lst = {0};

ZLOX_VOID zlox_pci_reg_outw(ZLOX_UNI_PCI_CFG_ADDR saddr, ZLOX_UINT8 reg, ZLOX_UINT16 val)
{
	saddr.reg = reg;
	zlox_outl(ZLOX_PCI_ADDRESS, saddr.val);
	zlox_outw(ZLOX_PCI_DATA, val);
}

ZLOX_UINT16 zlox_pci_reg_inw(ZLOX_UNI_PCI_CFG_ADDR saddr, ZLOX_UINT8 reg)
{
	saddr.reg = reg;
	zlox_outl(ZLOX_PCI_ADDRESS, saddr.val);
	return zlox_inw(ZLOX_PCI_DATA);
}

ZLOX_VOID zlox_pci_devconf_lst_add(ZLOX_UNI_PCI_CFG_ADDR addr, ZLOX_PCI_CONF_HDR * hdr)
{
	if(pci_devconf_lst.ptr == ZLOX_NULL)
	{
		pci_devconf_lst.count = 0;
		pci_devconf_lst.size = ZLOX_PCI_DEVCONF_SZ;
		pci_devconf_lst.ptr = (ZLOX_PCI_DEVCONF *)zlox_kmalloc(sizeof(ZLOX_PCI_DEVCONF) * pci_devconf_lst.size);
		zlox_memset((ZLOX_UINT8 *)pci_devconf_lst.ptr, 0, sizeof(ZLOX_PCI_DEVCONF) * pci_devconf_lst.size);
	}
	else if(pci_devconf_lst.count == pci_devconf_lst.size)
	{
		ZLOX_PCI_DEVCONF * tmptr;
		pci_devconf_lst.size += ZLOX_PCI_DEVCONF_SZ;
		tmptr = (ZLOX_PCI_DEVCONF *)zlox_kmalloc(sizeof(ZLOX_PCI_DEVCONF) * pci_devconf_lst.size);
		zlox_memset((ZLOX_UINT8 *)tmptr, 0, sizeof(ZLOX_PCI_DEVCONF) * pci_devconf_lst.size);
		zlox_memcpy((ZLOX_UINT8 *)tmptr, (ZLOX_UINT8 *)pci_devconf_lst.ptr, 
				sizeof(ZLOX_PCI_DEVCONF) * pci_devconf_lst.count);
		zlox_kfree(pci_devconf_lst.ptr);
		pci_devconf_lst.ptr = tmptr;
	}
	pci_devconf_lst.ptr[pci_devconf_lst.count].cfg_addr = addr;
	zlox_memcpy((ZLOX_UINT8 *)&pci_devconf_lst.ptr[pci_devconf_lst.count].cfg_hdr, 
			(ZLOX_UINT8 *)hdr, sizeof(ZLOX_PCI_CONF_HDR));
	pci_devconf_lst.count++;
}

ZLOX_UNI_PCI_CFG_ADDR zlox_pci_device_read_config(ZLOX_PCI_CONF_HDR * hdr, ZLOX_UINT32 bus, 
							ZLOX_UINT32 device, ZLOX_UINT32 function)
{
	ZLOX_UNI_PCI_CFG_ADDR saddr;
	saddr.val = 0;
	saddr.bus = bus;
	saddr.device = device;
	saddr.function = function;
	saddr.enable = 1;

	// if you set saddr.reg < 0x40 then you will enter Infinite loop
	// Because the reg only has six bit!
	for(saddr.reg = 0; saddr.reg < 0x3f; saddr.reg++)
	{
		zlox_outl(ZLOX_PCI_ADDRESS, saddr.val);
		((ZLOX_UINT32 *)hdr)[saddr.reg] = zlox_inl(ZLOX_PCI_DATA);
	}

	saddr.reg = 0;

	return saddr;
}

ZLOX_UINT32 zlox_pci_get_bar_mem_amount(ZLOX_UNI_PCI_CFG_ADDR saddr, ZLOX_UINT32 bar_index)
{
	saddr.reg = 4 + bar_index;
	zlox_outl(ZLOX_PCI_ADDRESS, saddr.val);
	ZLOX_UINT32 orig_bar = zlox_inl(ZLOX_PCI_DATA);
	zlox_outl(ZLOX_PCI_DATA, 0xFFFFFFFF);
	ZLOX_UINT32 mem_amount = zlox_inl(ZLOX_PCI_DATA);
	zlox_outl(ZLOX_PCI_DATA, orig_bar);
	mem_amount = ~mem_amount;
	mem_amount += 1;
	return mem_amount;
}

ZLOX_VOID zlox_pci_bus_scan(ZLOX_UINT32 bus)
{
	ZLOX_UNI_PCI_CFG_ADDR ret_addr;
	ZLOX_PCI_CONF_HDR * hdr = (ZLOX_PCI_CONF_HDR *)zlox_kmalloc(sizeof(ZLOX_PCI_CONF_HDR));	
	
	for(ZLOX_UINT32 i = 0; i <  0x20; i++)
	{
		ret_addr =  zlox_pci_device_read_config(hdr, bus, i, 0);
		
		if(hdr->vend_id == 0xFFFF)
		{
			continue;
		}	
	
		zlox_pci_devconf_lst_add(ret_addr, hdr);

		if((hdr->header_type & 0x80) != 0)
		{
			for(ZLOX_UINT32 j = 1; j < 8; j++)
			{
				ret_addr = zlox_pci_device_read_config(hdr, bus, i, j);
				
				if(hdr->vend_id == 0xFFFF)
					continue;
	
				zlox_pci_devconf_lst_add(ret_addr, hdr);
			}
		}
	}

	zlox_kfree(hdr);
}

ZLOX_PCI_DEVCONF * zlox_pci_get_devconf(ZLOX_UINT16 vend_id, ZLOX_UINT16 dev_id)
{
	for(ZLOX_UINT32 i = 0; i < pci_devconf_lst.count ;i++)
	{
		if(pci_devconf_lst.ptr[i].cfg_hdr.vend_id == vend_id &&
			pci_devconf_lst.ptr[i].cfg_hdr.dev_id == dev_id)
			return &pci_devconf_lst.ptr[i];
	}
	return ZLOX_NULL;
}

ZLOX_UINT32 zlox_pci_get_bar(ZLOX_PCI_DEVCONF * pci_devconf, ZLOX_UINT8 type)
{
	ZLOX_UINT32 bar = 0;
	for(ZLOX_UINT32 i = 0; i < 6; i++)
	{
		bar = pci_devconf->cfg_hdr.bars[i];
		if((bar & 0x1) == type)
			return bar;
	}
	
	return 0xFFFFFFFF;
}

ZLOX_VOID zlox_pci_list()
{
	if(pci_devconf_lst.ptr == ZLOX_NULL)
	{
		zlox_monitor_write("no pci device detected");
		return;
	}

	zlox_monitor_write("PCI dev list:\n");
	for(ZLOX_UINT32 i = 0; i < pci_devconf_lst.count ;i++)
	{
		zlox_monitor_write("bus [");
		zlox_monitor_write_dec((ZLOX_UINT32)pci_devconf_lst.ptr[i].cfg_addr.bus);
		zlox_monitor_write("] dev [");
		zlox_monitor_write_dec((ZLOX_UINT32)pci_devconf_lst.ptr[i].cfg_addr.device);
		zlox_monitor_write("] func [");
		zlox_monitor_write_dec((ZLOX_UINT32)pci_devconf_lst.ptr[i].cfg_addr.function);
		zlox_monitor_write("] vend id: ");
		zlox_monitor_write_hex((ZLOX_UINT32)pci_devconf_lst.ptr[i].cfg_hdr.vend_id);
		zlox_monitor_write(" dev id: ");
		zlox_monitor_write_hex((ZLOX_UINT32)pci_devconf_lst.ptr[i].cfg_hdr.dev_id);
		zlox_monitor_write("\n");
	}
	zlox_monitor_write("pci device total count: ");
	zlox_monitor_write_dec(pci_devconf_lst.count);
	//zlox_monitor_write(" total size: "); //debug
	//zlox_monitor_write_dec(pci_devconf_lst.size); //debug
	zlox_monitor_write("\n");
}

ZLOX_SINT32 zlox_pci_get_devconf_lst(ZLOX_PCI_DEVCONF_LST * devconf_lst)
{
	if(devconf_lst == ZLOX_NULL)
		return -1;
	zlox_memcpy((ZLOX_UINT8 *)devconf_lst, (ZLOX_UINT8 *)&pci_devconf_lst, sizeof(ZLOX_PCI_DEVCONF_LST));
	return 0;
}

ZLOX_VOID zlox_pci_init()
{
	ZLOX_UNI_PCI_CFG_ADDR saddr, saddr2;
	saddr.val = 0;
	saddr.enable = 1;

	for(ZLOX_UINT32 i = 0; i < 0xFF; i++)
	{
		saddr.bus = i;
		zlox_outl(ZLOX_PCI_ADDRESS, saddr.val);
		saddr2.val = zlox_inl(ZLOX_PCI_DATA);
		if(saddr2.val == 0xFFFFFFFF)
			continue;
		zlox_pci_bus_scan(i);
	}
}

