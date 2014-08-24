// zlox_e1000.h -- 和Intel Pro/1000 (e1000)网卡驱动相关的定义

#ifndef _ZLOX_E1000_H_
#define _ZLOX_E1000_H_

#include "zlox_common.h"
#include "zlox_pci.h"

#define ZLOX_E1000_MMIO_ADDR 0xB0000000

#define ZLOX_NUM_RX_DESC 256
#define ZLOX_NUM_TX_DESC 256
#define ZLOX_RX_BUFFER_SIZE 2048
#define ZLOX_TX_BUFFER_SIZE 2048

#define ZLOX_CACHE_LINE_ALIGN 4096

struct _ZLOX_E1000_RX_DESC {
	ZLOX_UINT32 buffer;
	ZLOX_UINT32 buffer_h;
	ZLOX_UINT16 length;
	ZLOX_UINT16 checksum;
	ZLOX_UINT8 status;
	ZLOX_UINT8 errors;
	ZLOX_UINT16 special;
} __attribute__((packed));

typedef struct _ZLOX_E1000_RX_DESC ZLOX_E1000_RX_DESC;

struct _ZLOX_E1000_TX_DESC {
	ZLOX_UINT32 buffer;
	ZLOX_UINT32 buffer_h;
	ZLOX_UINT16 length;
	ZLOX_UINT8 cso;
	ZLOX_UINT8 cmd;
	ZLOX_UINT8 status;
	ZLOX_UINT8 css;
	ZLOX_UINT16 special;
} __attribute__((packed));

typedef struct _ZLOX_E1000_TX_DESC ZLOX_E1000_TX_DESC;

typedef struct _ZLOX_E1000_DEV
{
	ZLOX_BOOL isInit;
	ZLOX_PCI_DEVCONF * pci_devconf;
	ZLOX_UINT32 mmio_base;
	ZLOX_UINT8 mac[6];
	ZLOX_BOOL is_e;
	ZLOX_E1000_RX_DESC * rx_descs;
	ZLOX_UINT32 rx_descs_count;
	ZLOX_UINT32 rx_descs_p;
	ZLOX_UINT32 rx_buffer_size;
	ZLOX_UINT8 * rx_buffer;
	ZLOX_E1000_TX_DESC * tx_descs;
	ZLOX_UINT32 tx_descs_count;
	ZLOX_UINT32 tx_descs_p;
	ZLOX_UINT32 tx_buffer_size;
	ZLOX_UINT8 * tx_buffer;
}ZLOX_E1000_DEV;

ZLOX_SINT32 zlox_e1000_send(ZLOX_UINT8 * buf, ZLOX_UINT16 length);

ZLOX_VOID zlox_e1000_init();

#endif // _ZLOX_E1000_H_

