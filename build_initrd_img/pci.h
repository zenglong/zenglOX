// pci.h -- 和PCI相关的结构定义

#ifndef _PCI_H_
#define _PCI_H_

struct _PCI_CONF_HDR {
	UINT16 vend_id;
	UINT16 dev_id;
	UINT16 command;
	UINT16 status;
	UINT8 rev;
	UINT8 prog_if;
	UINT8 sub_class;
	UINT8 class_code;
	UINT8 cache_line_size;
	UINT8 latency_timer;
	UINT8 header_type;
	UINT8 bist;
	UINT32 bars[6];
	UINT32 reserved[2];
	UINT32 romaddr;
	UINT32 reserved2[2];
	UINT8 int_line;
	UINT8 int_pin;
	UINT8 min_grant;
	UINT8 max_latency;
	UINT8 data[192];
} __attribute__((packed));

typedef struct _PCI_CONF_HDR PCI_CONF_HDR;

union _UNI_PCI_CFG_ADDR {
	struct{
		UINT32 type:2;
		UINT32 reg:6;
		UINT32 function:3;
		UINT32 device:5;
		UINT32 bus:8;
		UINT32 reserved:7;
		UINT32 enable:1;
	};
	UINT32 val;
} __attribute__((packed));

typedef union _UNI_PCI_CFG_ADDR UNI_PCI_CFG_ADDR;

typedef struct _PCI_DEVCONF {
	UNI_PCI_CFG_ADDR cfg_addr;
	PCI_CONF_HDR cfg_hdr;
}PCI_DEVCONF;

typedef struct _PCI_DEVCONF_LST {
	UINT32 count;
	UINT32 size;
	PCI_DEVCONF * ptr;
}PCI_DEVCONF_LST;

#endif // _PCI_H_

