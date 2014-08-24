// network.h -- 和网络相关的结构定义

#ifndef _NETWORK_H_
#define _NETWORK_H_

#include "common.h"

#define NET_IP_PROTO_UDP 17
#define NET_IP_PROTO_ICMP 1

typedef struct _NETWORK_INFO {
	BOOL isInit;
	UINT32 ip_addr;
	UINT32 net_mask;
	UINT32 route_gateway;
	UINT8 mac[6];
} NETWORK_INFO;

struct _ETH_HEADER {
	UINT8 hdr_dmac[6];
	UINT8 hdr_smac[6];
	UINT16 hdr_type;
} __attribute__((packed));

typedef struct _ETH_HEADER ETH_HEADER;

struct _ETH_IP {
	UINT8 ip_hl:4; /* both fields are 4 bytes */
	UINT8 ip_v:4;
	UINT8 ip_tos;
	UINT16 ip_len;
	UINT16 ip_id;
	UINT16 ip_off;
	UINT8 ip_ttl;
	UINT8 ip_p;
	UINT16 ip_sum;
	UINT32 ip_src;
	UINT32 ip_dst;
} __attribute__((packed));

typedef struct _ETH_IP ETH_IP;

struct _ETH_UDP {
	UINT16 src_port;
	UINT16 dst_port;
	UINT16 length;
	UINT16 checksum;
} __attribute__((packed));

typedef struct _ETH_UDP ETH_UDP;

struct _ETH_DHCP_PACKET{
	ETH_HEADER eth;
	ETH_IP ip;
	ETH_UDP udp;
	UINT8 op;
	UINT8 htype;
	UINT8 hlen;
	UINT8 hops;
	UINT32 xid;
	UINT16 secs;
	UINT16 flags;
	UINT32 ciaddr;
	UINT32 yiaddr;
	UINT32 siaddr;
	UINT32 giaddr;
	UINT8 chaddr[16];
	UINT8 sname[64];
	UINT8 file[128];
	UINT8 magic[4];
} __attribute__((packed));

typedef struct _ETH_DHCP_PACKET ETH_DHCP_PACKET;

struct _ETH_ARP_PACKET {
	ETH_HEADER eth;
	UINT16 hw_type;
	UINT16 proto_type;
	UINT8 hw_len;
	UINT8 p_len;
	UINT16 op;
	UINT8 tx_mac[6];
	UINT8 tx_ip[4];
	UINT8 d_mac[6];
	UINT8 d_ip[4];
} __attribute__((packed));

typedef struct _ETH_ARP_PACKET ETH_ARP_PACKET;

struct _ETH_ICMP_PACKET {
	ETH_HEADER eth;
	ETH_IP ip;
	UINT8 type;
	UINT8 code;
	UINT16 checksum;
	UINT32 rest;	
} __attribute__((packed));

typedef struct _ETH_ICMP_PACKET ETH_ICMP_PACKET;

UINT16 net_cksum(UINT16 *buf, SINT32 nbytes);

UINT16 net_swap_word(UINT16 val);

UINT32 net_getip_from_string(char * str, UINT8 * ip);

SINT32 net_make_eth_header(ETH_HEADER * eth, UINT8 * hdr_smac, UINT8 * hdr_dmac, UINT16 hdr_type);

SINT32 net_make_ip_header(ETH_IP * ip_header, UINT16 data_length, UINT8 ip_p, 
				UINT32 ip_src, UINT32 ip_dst);

SINT32 net_make_udp_header(ETH_UDP * udp, UINT16 src_port, UINT16 dst_port, 
				UINT16 data_length, ETH_IP * ip_header, UINT8 * buf);

SINT32 net_make_udp(UINT8 * header_ptr,
			UINT8 * hdr_smac, UINT8 * hdr_dmac,
			UINT32 ip_src, UINT32 ip_dst,
			UINT16 src_port, UINT16 dst_port, 
			UINT16 data_length, UINT8 * cksum_buf);

SINT32 net_make_arp(UINT8 * header_ptr,
			UINT8 * hdr_smac, UINT8 * hdr_dmac,
			UINT32 ip_src, UINT32 ip_dst,
			UINT16 op);

#endif // _NETWORK_H_

