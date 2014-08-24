// network.c -- 和libnetwork.so动态库文件相关的函数定义

#include "network.h"

//taken from www.netfor2.com/ipsum.htm
UINT16 net_cksum(UINT16 *buf, SINT32 nbytes)
{
	UINT32 sum, oddbyte;
	UINT16 * ptr = buf;

	sum = 0;
	while (nbytes > 1) {
		sum += *ptr++;
		nbytes -= 2;
	}
	if (nbytes == 1) {
		oddbyte = 0;
		((UINT8 *) & oddbyte)[0] = *(UINT8 *) ptr;
		((UINT8 *) & oddbyte)[1] = 0;
		sum += oddbyte;
	}
	sum = (sum >> 16) + (sum & 0xffff);
	sum += (sum >> 16);
	return (~sum);
}

UINT16 net_swap_word(UINT16 val)
{
	UINT16 temp;
	temp = val << 8 | val >> 8;
	return temp;
}

UINT32 net_getip_from_string(char * str, UINT8 * ip)
{
	int i=0, j=0;
	ip[0]= ip[1]= ip[2] = ip[3] = 0;

	while (str[i]){
		if (str[i]=='.') 
			j++;
		else 
		{
			if(str[i] >= 48 && str[i] <= 57)
				ip[j] = ip[j]*10 + str[i] - '0';
			else
				ip[j] = ip[j]*10 + 0;
		}
		i++;
	}
	return *((UINT32 *)ip);
}

SINT32 net_make_eth_header(ETH_HEADER * eth, UINT8 * hdr_smac, UINT8 * hdr_dmac, UINT16 hdr_type)
{
	memcpy(eth->hdr_dmac, hdr_dmac, 6);
	memcpy(eth->hdr_smac, hdr_smac, 6);
	eth->hdr_type = net_swap_word(hdr_type);
	return 0;
}

SINT32 net_make_ip_header(ETH_IP * ip_header, UINT16 data_length, UINT8 ip_p, 
				UINT32 ip_src, UINT32 ip_dst)
{
	ip_header->ip_v = 4;
	ip_header->ip_hl = 5;
	ip_header->ip_len = net_swap_word(data_length + ip_header->ip_hl*4);
	ip_header->ip_p = ip_p;
	ip_header->ip_ttl = 64;
	ip_header->ip_off = 0x40; //should not fragment
	ip_header->ip_src = ip_src;
	ip_header->ip_dst = ip_dst;
	ip_header->ip_sum = net_cksum((UINT16 *)ip_header, ip_header->ip_hl * 4);
	return 0;
}

SINT32 net_make_udp_header(ETH_UDP * udp, UINT16 src_port, UINT16 dst_port, 
				UINT16 data_length, ETH_IP * ip_header, UINT8 * buf)
{
	UINT16	udp_length = sizeof(ETH_UDP) + data_length;
	udp->src_port = net_swap_word(src_port);
	udp->dst_port = net_swap_word(dst_port);
	udp->length = net_swap_word(udp_length);
	udp->checksum = 0;

	UINT8 * tmp_buf = buf;
	memcpy(tmp_buf, (UINT8 *)&ip_header->ip_src, 4);
	tmp_buf += 4;
	memcpy(tmp_buf, (UINT8 *)&ip_header->ip_dst, 4);
	tmp_buf += 4;
	tmp_buf[0] = 0;
	tmp_buf[1] = ip_header->ip_p;
	tmp_buf += 2;
	memcpy(tmp_buf, (UINT8 *)&udp->length, 2);
	tmp_buf += 2;
	memcpy(tmp_buf, (UINT8 *)udp, udp_length);
	udp->checksum = net_cksum((UINT16 *)buf, udp_length + 12);
	return 0;
}

SINT32 net_make_udp(UINT8 * header_ptr,
			UINT8 * hdr_smac, UINT8 * hdr_dmac,
			UINT32 ip_src, UINT32 ip_dst,
			UINT16 src_port, UINT16 dst_port, 
			UINT16 data_length, UINT8 * cksum_buf)
{
	ETH_HEADER * eth = (ETH_HEADER *)header_ptr;
	ETH_IP * ip_header = (ETH_IP *)(header_ptr + sizeof(ETH_HEADER));
	ETH_UDP * udp = (ETH_UDP *)(header_ptr + sizeof(ETH_HEADER) + sizeof(ETH_IP));
	UINT16	udp_length = sizeof(ETH_UDP) + data_length;

	net_make_eth_header(eth, hdr_smac, hdr_dmac, 0x800); // 0x800 -- ip数据报
	net_make_ip_header(ip_header, udp_length, NET_IP_PROTO_UDP, ip_src, ip_dst);
	net_make_udp_header(udp, src_port, dst_port, data_length, ip_header, cksum_buf);
	return 0;
}

SINT32 net_make_arp(UINT8 * header_ptr,
			UINT8 * hdr_smac, UINT8 * hdr_dmac,
			UINT32 ip_src, UINT32 ip_dst,
			UINT16 op)
{
	ETH_HEADER * eth = (ETH_HEADER *)header_ptr;
	ETH_ARP_PACKET * arp = (ETH_ARP_PACKET *)header_ptr;
	net_make_eth_header(eth, hdr_smac, hdr_dmac, 0x0806); // 0x0806 -- arp数据报

	arp->hw_type = net_swap_word(0x01);
	arp->proto_type = net_swap_word(0x800);
	arp->hw_len = 0x06;
	arp->p_len = 0x04;
	arp->op = net_swap_word(op);
	memcpy(arp->tx_mac, hdr_smac, 6);
	memcpy(arp->tx_ip, (UINT8 *)&ip_src, 4);
	memcpy(arp->d_mac, hdr_dmac, 6);
	memcpy(arp->d_ip, (UINT8 *)&ip_dst, 4);
	return 0;
}

