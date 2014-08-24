// zlox_pci.h -- 和PCI相关的定义

#ifndef _ZLOX_PCI_H_
#define _ZLOX_PCI_H_

#include "zlox_common.h"

#define ZLOX_PCI_BAR_MEM 0x0
#define ZLOX_PCI_BAR_IO 0x1
#define ZLOX_PCI_BAR_NONE 0x3

#define ZLOX_PCI_REG_COMMAND 0x1
#define ZLOX_PCI_COMMAND_MAST_EN 0x0004	/* Enable Busmaster Access */

struct _ZLOX_PCI_CONF_HDR {
	ZLOX_UINT16 vend_id;
	ZLOX_UINT16 dev_id;
	ZLOX_UINT16 command;
	ZLOX_UINT16 status;
	ZLOX_UINT8 rev;
	ZLOX_UINT8 prog_if;
	ZLOX_UINT8 sub_class;
	ZLOX_UINT8 class_code;
	ZLOX_UINT8 cache_line_size;
	ZLOX_UINT8 latency_timer;
	ZLOX_UINT8 header_type;
	ZLOX_UINT8 bist;
	ZLOX_UINT32 bars[6];
	ZLOX_UINT32 reserved[2];
	ZLOX_UINT32 romaddr;
	ZLOX_UINT32 reserved2[2];
	ZLOX_UINT8 int_line;
	ZLOX_UINT8 int_pin;
	ZLOX_UINT8 min_grant;
	ZLOX_UINT8 max_latency;
	ZLOX_UINT8 data[192];
} __attribute__((packed));

typedef struct _ZLOX_PCI_CONF_HDR ZLOX_PCI_CONF_HDR;

union _ZLOX_UNI_PCI_CFG_ADDR {
	struct{
		ZLOX_UINT32 type:2;
		ZLOX_UINT32 reg:6;
		ZLOX_UINT32 function:3;
		ZLOX_UINT32 device:5;
		ZLOX_UINT32 bus:8;
		ZLOX_UINT32 reserved:7;
		ZLOX_UINT32 enable:1;
	};
	ZLOX_UINT32 val;
} __attribute__((packed));

typedef union _ZLOX_UNI_PCI_CFG_ADDR ZLOX_UNI_PCI_CFG_ADDR;

typedef struct _ZLOX_PCI_DEVCONF {
	ZLOX_UNI_PCI_CFG_ADDR cfg_addr;
	ZLOX_PCI_CONF_HDR cfg_hdr;
}ZLOX_PCI_DEVCONF;

typedef struct _ZLOX_PCI_DEVCONF_LST {
	ZLOX_UINT32 count;
	ZLOX_UINT32 size;
	ZLOX_PCI_DEVCONF * ptr;
}ZLOX_PCI_DEVCONF_LST;

ZLOX_VOID zlox_pci_reg_outw(ZLOX_UNI_PCI_CFG_ADDR saddr, ZLOX_UINT8 reg, ZLOX_UINT16 val);

ZLOX_UINT16 zlox_pci_reg_inw(ZLOX_UNI_PCI_CFG_ADDR saddr, ZLOX_UINT8 reg);

ZLOX_UINT32 zlox_pci_get_bar_mem_amount(ZLOX_UNI_PCI_CFG_ADDR saddr, ZLOX_UINT32 bar_index);

ZLOX_PCI_DEVCONF * zlox_pci_get_devconf(ZLOX_UINT16 vend_id, ZLOX_UINT16 dev_id);

ZLOX_UINT32 zlox_pci_get_bar(ZLOX_PCI_DEVCONF * pci_devconf, ZLOX_UINT8 type);

ZLOX_VOID zlox_pci_list();

ZLOX_SINT32 zlox_pci_get_devconf_lst(ZLOX_PCI_DEVCONF_LST * devconf_lst);

ZLOX_VOID zlox_pci_init();

#endif // _ZLOX_PCI_H_

