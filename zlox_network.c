// zlox_network.c -- 和网络模块相关的函数定义

#include "zlox_network.h"
#include "zlox_e1000.h"
#include "zlox_kheap.h"

#define ZLOX_NETWORK_RECEIVE_BUF_SIZE 2048
#define ZLOX_NETWORK_RECEIVE_ARR_SIZE 10
#define ZLOX_NET_RECV_TIMEOUT_TICKS 3000

typedef enum _ZLOX_NETWORK_DEV_TYPE{
	ZLOX_NET_DEV_TYPE_NONE,
	ZLOX_NET_DEV_TYPE_E1000,
} ZLOX_NETWORK_DEV_TYPE;

typedef struct _ZLOX_NETWORK_RECEIVE_MEMBER {
	ZLOX_TASK * used_task;
	ZLOX_UINT32 tick;
	ZLOX_UINT16 length;
	ZLOX_UINT8 buf[ZLOX_NETWORK_RECEIVE_BUF_SIZE];
} ZLOX_NETWORK_RECEIVE_MEMBER;

typedef struct _ZLOX_NETWORK_RECEIVE_ARRAY{
	ZLOX_BOOL isInit;
	ZLOX_SINT32 size;
	ZLOX_SINT32 count;
	ZLOX_NETWORK_RECEIVE_MEMBER * ptr;
} ZLOX_NETWORK_RECEIVE_ARRAY;

extern ZLOX_E1000_DEV e1000_dev;
// zlox_time.c
extern ZLOX_UINT32 tick;

ZLOX_BOOL network_isInit = ZLOX_FALSE;
ZLOX_NETWORK_DEV_TYPE network_dev_type = ZLOX_NET_DEV_TYPE_NONE;
ZLOX_TASK * network_focus_task = ZLOX_NULL;
ZLOX_NETWORK_RECEIVE_ARRAY network_recv_array = {0};
ZLOX_NETWORK_INFO network_info = {0};
ZLOX_ETH_ARP_PACKET network_arp_for_reply;

ZLOX_UINT16 zlox_net_swap_word(ZLOX_UINT16 val)
{
	ZLOX_UINT16 temp;
	temp = val << 8 | val >> 8;
	return temp;
}

ZLOX_SINT32 zlox_net_make_eth_header(ZLOX_ETH_HEADER * eth, ZLOX_UINT8 * hdr_smac, ZLOX_UINT8 * hdr_dmac, ZLOX_UINT16 hdr_type)
{
	zlox_memcpy(eth->hdr_dmac, hdr_dmac, 6);
	zlox_memcpy(eth->hdr_smac, hdr_smac, 6);
	eth->hdr_type = zlox_net_swap_word(hdr_type);
	return 0;
}

ZLOX_SINT32 zlox_net_make_arp(ZLOX_UINT8 * header_ptr,
			ZLOX_UINT8 * hdr_smac, ZLOX_UINT8 * hdr_dmac,
			ZLOX_UINT32 ip_src, ZLOX_UINT32 ip_dst,
			ZLOX_UINT16 op)
{
	ZLOX_ETH_HEADER * eth = (ZLOX_ETH_HEADER *)header_ptr;
	ZLOX_ETH_ARP_PACKET * arp = (ZLOX_ETH_ARP_PACKET *)header_ptr;
	zlox_net_make_eth_header(eth, hdr_smac, hdr_dmac, 0x0806); // 0x0806 -- arp数据报

	arp->hw_type = zlox_net_swap_word(0x01);
	arp->proto_type = zlox_net_swap_word(0x800);
	arp->hw_len = 0x06;
	arp->p_len = 0x04;
	arp->op = zlox_net_swap_word(op);
	zlox_memcpy(arp->tx_mac, hdr_smac, 6);
	zlox_memcpy(arp->tx_ip, (ZLOX_UINT8 *)&ip_src, 4);
	zlox_memcpy(arp->d_mac, hdr_dmac, 6);
	zlox_memcpy(arp->d_ip, (ZLOX_UINT8 *)&ip_dst, 4);
	return 0;
}

ZLOX_SINT32 zlox_network_getinfo(ZLOX_NETWORK_INFO * buf)
{
	if(buf == ZLOX_NULL)
		return -1;
	zlox_memcpy((ZLOX_UINT8 *)buf, (ZLOX_UINT8 *)&network_info, sizeof(ZLOX_NETWORK_INFO));
	return 0;
}

ZLOX_SINT32 zlox_network_setinfo(ZLOX_NETWORK_INFO * buf)
{
	if(buf == ZLOX_NULL)
		return -1;
	network_info.ip_addr = buf->ip_addr;
	network_info.net_mask = buf->net_mask;
	network_info.route_gateway = buf->route_gateway;
	return 0;
}

ZLOX_SINT32 zlox_network_set_focus_task(ZLOX_TASK * task)
{
	if(task == ZLOX_NULL)
		return -1;
	network_focus_task = task;
	return 0;
}

ZLOX_SINT32 zlox_network_send(ZLOX_UINT8 * buf, ZLOX_UINT16 length)
{
	if(buf == ZLOX_NULL)
		return -1;
	if(length == 0)
		return -1;
	if(network_dev_type == ZLOX_NET_DEV_TYPE_E1000)
	{
		return zlox_e1000_send(buf, length);
	}
	return 0;
}

ZLOX_SINT32 zlox_network_received(ZLOX_UINT8 * buf, ZLOX_UINT16 length)
{
	ZLOX_SINT32 index = -1;
	if(buf == ZLOX_NULL)
		return -1;
	if(length == 0)
		return -1;

	ZLOX_ETH_ARP_PACKET * darp = (ZLOX_ETH_ARP_PACKET *)buf;
	if(zlox_net_swap_word(darp->eth.hdr_type) == 0x0806 &&
		zlox_net_swap_word(darp->op) == 0x1)
	{
		zlox_net_make_arp((ZLOX_UINT8 *)&network_arp_for_reply, network_info.mac, darp->tx_mac, 
			network_info.ip_addr, *((ZLOX_UINT32 *)darp->d_ip), 0x2);
		zlox_network_send((ZLOX_UINT8 *)&network_arp_for_reply, sizeof(ZLOX_ETH_ARP_PACKET));
	}

	if(network_focus_task == ZLOX_NULL)
		return 0;
	if(network_recv_array.isInit == ZLOX_FALSE)
	{
		network_recv_array.size = ZLOX_NETWORK_RECEIVE_ARR_SIZE;
		network_recv_array.count = 0;
		network_recv_array.ptr = 
			(ZLOX_NETWORK_RECEIVE_MEMBER *)zlox_kmalloc(network_recv_array.size * sizeof(ZLOX_NETWORK_RECEIVE_MEMBER));
		zlox_memset((ZLOX_UINT8 *)network_recv_array.ptr, 0, 
				network_recv_array.size * sizeof(ZLOX_NETWORK_RECEIVE_MEMBER));
		network_recv_array.isInit = ZLOX_TRUE;
	}
	else if(network_recv_array.count == network_recv_array.size)
	{
		network_recv_array.size += ZLOX_NETWORK_RECEIVE_ARR_SIZE;
		ZLOX_NETWORK_RECEIVE_MEMBER * tmp = 
			(ZLOX_NETWORK_RECEIVE_MEMBER *)zlox_kmalloc(network_recv_array.size * sizeof(ZLOX_NETWORK_RECEIVE_MEMBER));
		zlox_memcpy((ZLOX_UINT8 *)tmp, (ZLOX_UINT8 *)network_recv_array.ptr, 
				network_recv_array.count * sizeof(ZLOX_NETWORK_RECEIVE_MEMBER));
		zlox_memset((ZLOX_UINT8 *)(tmp + network_recv_array.count), 0, 
				ZLOX_NETWORK_RECEIVE_ARR_SIZE * sizeof(ZLOX_NETWORK_RECEIVE_MEMBER));
		zlox_kfree(network_recv_array.ptr);
		network_recv_array.ptr = tmp;
	}
	if(length > ZLOX_NETWORK_RECEIVE_BUF_SIZE)
		length = ZLOX_NETWORK_RECEIVE_BUF_SIZE;
	for(ZLOX_SINT32 i = 0; i < network_recv_array.size ; i++)
	{
		if((network_recv_array.ptr[i].used_task == ZLOX_NULL) || 
			((tick - network_recv_array.ptr[i].tick) > ZLOX_NET_RECV_TIMEOUT_TICKS))
		{
			if(network_recv_array.ptr[i].used_task == ZLOX_NULL)
				network_recv_array.count++;
			index = i;
			zlox_memcpy(network_recv_array.ptr[index].buf, buf, length);
			network_recv_array.ptr[index].used_task = network_focus_task;
			network_recv_array.ptr[index].tick = tick;
			network_recv_array.ptr[index].length = length;
			break;
		}
	}
	if(index == -1)
		return -1;
	ZLOX_TASK_MSG msg = {0};
	msg.type = ZLOX_MT_NET_PACKET;
	msg.packet_idx = index;
	zlox_send_tskmsg(network_focus_task,&msg);
	return index;
}

ZLOX_SINT32 zlox_network_get_packet(ZLOX_TASK * recv_task, ZLOX_SINT32 index, ZLOX_UINT8 * buf, 
			ZLOX_UINT16 length)
{
	if(recv_task == ZLOX_NULL)
		return -1;
	if(buf == ZLOX_NULL)
		return -1;
	if(length == 0)
		return -1;
	if(index < 0)
		return -1;
	if(network_recv_array.ptr[index].used_task != recv_task)
		return -1;
	if(length > network_recv_array.ptr[index].length)
		length = network_recv_array.ptr[index].length;
	zlox_memcpy(buf, network_recv_array.ptr[index].buf, length);
	network_recv_array.ptr[index].used_task = ZLOX_NULL;
	network_recv_array.ptr[index].tick = 0;
	network_recv_array.ptr[index].length = 0;
	network_recv_array.count--;
	return (ZLOX_SINT32)length;
}

ZLOX_VOID zlox_network_free_packets(ZLOX_TASK * recv_task)
{
	for(ZLOX_SINT32 i = 0; i < network_recv_array.size;i++)
	{
		if(network_recv_array.ptr[i].used_task == recv_task)
		{
			network_recv_array.ptr[i].used_task = ZLOX_NULL;
			network_recv_array.ptr[i].tick = 0;
			network_recv_array.ptr[i].length = 0;
			network_recv_array.count--;
		}
	}
	return ;
}

ZLOX_BOOL zlox_network_init()
{
	zlox_e1000_init();
	if(e1000_dev.isInit == ZLOX_TRUE)
	{
		network_info.isInit = network_isInit = ZLOX_TRUE;
		zlox_memcpy(network_info.mac, e1000_dev.mac, 6);
		network_dev_type = ZLOX_NET_DEV_TYPE_E1000;
		return ZLOX_TRUE;
	}
	else
	{
		network_isInit = ZLOX_FALSE;
		network_dev_type = ZLOX_NET_DEV_TYPE_NONE;
	}
	return ZLOX_FALSE;
}

