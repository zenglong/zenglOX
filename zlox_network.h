// zlox_network.h -- 和网络模块相关的定义

#ifndef _ZLOX_NETWORK_H_
#define _ZLOX_NETWORK_H_

#include "zlox_common.h"
#include "zlox_task.h"

typedef struct _ZLOX_NETWORK_INFO {
	ZLOX_BOOL isInit;
	ZLOX_UINT32 ip_addr;
	ZLOX_UINT32 net_mask;
	ZLOX_UINT32 route_gateway;
	ZLOX_UINT8 mac[6];
} ZLOX_NETWORK_INFO;

struct _ZLOX_ETH_HEADER {
	ZLOX_UINT8 hdr_dmac[6];
	ZLOX_UINT8 hdr_smac[6];
	ZLOX_UINT16 hdr_type;
} __attribute__((packed));

typedef struct _ZLOX_ETH_HEADER ZLOX_ETH_HEADER;

struct _ZLOX_ETH_ARP_PACKET {
	ZLOX_ETH_HEADER eth;
	ZLOX_UINT16 hw_type;
	ZLOX_UINT16 proto_type;
	ZLOX_UINT8 hw_len;
	ZLOX_UINT8 p_len;
	ZLOX_UINT16 op;
	ZLOX_UINT8 tx_mac[6];
	ZLOX_UINT8 tx_ip[4];
	ZLOX_UINT8 d_mac[6];
	ZLOX_UINT8 d_ip[4];
} __attribute__((packed));

typedef struct _ZLOX_ETH_ARP_PACKET ZLOX_ETH_ARP_PACKET;

ZLOX_SINT32 zlox_network_getinfo(ZLOX_NETWORK_INFO * buf);

ZLOX_SINT32 zlox_network_setinfo(ZLOX_NETWORK_INFO * buf);

ZLOX_SINT32 zlox_network_set_focus_task(ZLOX_TASK * task);

ZLOX_SINT32 zlox_network_send(ZLOX_UINT8 * buf, ZLOX_UINT16 length);

ZLOX_SINT32 zlox_network_received(ZLOX_UINT8 * buf, ZLOX_UINT16 length);

ZLOX_SINT32 zlox_network_get_packet(ZLOX_TASK * recv_task, ZLOX_SINT32 index, ZLOX_UINT8 * buf, 
			ZLOX_UINT16 length);

ZLOX_VOID zlox_network_free_packets(ZLOX_TASK * recv_task);

ZLOX_BOOL zlox_network_init();

#endif // _ZLOX_NETWORK_H_

