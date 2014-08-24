// ping.c -- 和ping工具相关的函数定义

#include "common.h"
#include "syscall.h"
#include "kheap.h"
#include "network.h"
#include "task.h"

#define ICMP_TYPE_ECHO_REQUEST 8

ETH_ARP_PACKET test_packet;
NETWORK_INFO net_info;
ETH_ICMP_PACKET icmp;
ETH_ARP_PACKET * buf_packet;
ETH_ICMP_PACKET * buf_icmp_packet;
UINT8 buf[2048];

int ping_get_gateway_mac(UINT8 * gw_mac, TASK * task, UINT32 timeout)
{
	UINT32 origtick;
	TASK_MSG msg;
	UINT8 hdr_dmac[6] = {0};
	memset(hdr_dmac, 0xff, 6);

	net_make_arp((UINT8 *)&test_packet,
		net_info.mac, hdr_dmac,
		net_info.ip_addr, net_info.route_gateway,
		0x01);

	int ret, i;

	if(syscall_network_send((UINT8 *)&test_packet, sizeof(test_packet)) == -1)
	{
		syscall_monitor_write("send arp request time out");
		return -1;
	}
	else
		syscall_monitor_write("send arp request to gateway and wait for reply\n");

	origtick = syscall_timer_get_tick();
	while(TRUE)
	{
		ret = syscall_get_tskmsg(task,&msg,TRUE);
		if(ret != 1)
		{
			syscall_idle_cpu();
			UINT32 curtick = syscall_timer_get_tick();
			if((curtick - origtick) > timeout)
			{
				syscall_monitor_write("wait for arp reply time out curtick:");
				syscall_monitor_write_dec(curtick);
				syscall_monitor_write(" origtick:");
				syscall_monitor_write_dec(origtick);
				return -1;
			}
			continue;
		}
		if(msg.type == MT_KEYBOARD &&
			msg.keyboard.type == MKT_ASCII &&
			msg.keyboard.ascii == 0x1B) // ESC
			break;
		if(msg.type != MT_NET_PACKET)
			continue;
		if(syscall_network_get_packet(task, msg.packet_idx, buf, 2048) == -1)
			continue;
		buf_packet = (ETH_ARP_PACKET *)buf;
		buf_packet->op = net_swap_word(buf_packet->op);
		if(buf_packet->op != 2)
			continue;
		syscall_monitor_write("the gateway ip [");
		for(i=0;i < 4;i++)
		{
			syscall_monitor_write_dec(buf_packet->tx_ip[i]);
			if(i < 3)
				syscall_monitor_write(".");
		}
		syscall_monitor_write("] mac is [");
		for(i=0;i < 6;i++)
		{
			syscall_monitor_write_hex(buf_packet->tx_mac[i]);
			if(i < 5)
				syscall_monitor_write(":");
		}
		syscall_monitor_write("]\n");
		memcpy(gw_mac, buf_packet->tx_mac, 6);
		return 1;
	}
	return 0;
}

int ping_icmp_send_wait(UINT8 * gw_mac, char * dest_ip, TASK * task, UINT32 ping_num, UINT32 timeout)
{
	UINT32 origtick, curtick;
	TASK_MSG msg;
	UINT16 icmp_length = sizeof(ETH_ICMP_PACKET) - sizeof(ETH_HEADER) - sizeof(ETH_IP);
	UINT32 ip_dst;
	ETH_HEADER * eth = (ETH_HEADER *)&icmp;
	ETH_IP * ip_header = (ETH_IP *)((UINT8 *)eth + sizeof(ETH_HEADER));
	net_getip_from_string(dest_ip, (UINT8 *)&ip_dst);
	net_make_eth_header(eth, net_info.mac, gw_mac, 0x800); // 0x800 -- ip数据报
	net_make_ip_header(ip_header, icmp_length, NET_IP_PROTO_ICMP, net_info.ip_addr, ip_dst);

	icmp.type = ICMP_TYPE_ECHO_REQUEST;
	icmp.code = 0;
	icmp.checksum = 0;
	icmp.rest = 0;

	UINT8 * icmp_ptr = (UINT8 *)&icmp + sizeof(ETH_HEADER) + sizeof(ETH_IP);
	icmp.checksum = net_cksum((UINT16 *)icmp_ptr, (SINT32)icmp_length);

	int ret, i;
	BOOL infinite = FALSE;
	if(ping_num == 0)
		infinite = TRUE;

ping_send:

	if(syscall_network_send((UINT8 *)&icmp, sizeof(icmp)) == -1)
	{
		syscall_monitor_write("send icmp request time out");
		return -1;
	}
	else
	{
		syscall_monitor_write("sending ... ");
	}

	origtick = syscall_timer_get_tick();
	while(TRUE)
	{
		ret = syscall_get_tskmsg(task,&msg,TRUE);
		if(ret != 1)
		{
			syscall_idle_cpu();
			curtick = syscall_timer_get_tick();
			if((curtick - origtick) > timeout)
			{
				syscall_monitor_write("reply time out curtick:");
				syscall_monitor_write_dec(curtick);
				syscall_monitor_write(" origtick:");
				syscall_monitor_write_dec(origtick);
				syscall_monitor_write("\n");
				if(infinite == TRUE)
					goto ping_send;
				ping_num--;
				if(ping_num == 0)
					return -1;
				else
					goto ping_send;
			}
			continue;
		}
		if(msg.type == MT_KEYBOARD &&
			msg.keyboard.type == MKT_ASCII &&
			msg.keyboard.ascii == 0x1B) // ESC
			break;
		if(msg.type != MT_NET_PACKET)
			continue;
		if(syscall_network_get_packet(task, msg.packet_idx, buf, 2048) == -1)
			continue;
		buf_icmp_packet = (ETH_ICMP_PACKET *)buf;
		buf_icmp_packet->eth.hdr_type = net_swap_word(buf_icmp_packet->eth.hdr_type);
		if(buf_icmp_packet->eth.hdr_type != 0x800)
			continue;
		if(buf_icmp_packet->ip.ip_p != NET_IP_PROTO_ICMP)
			continue;
		if(buf_icmp_packet->type == 3)
		{
			syscall_monitor_write("Destination Unreachable\n");
			if(infinite == TRUE)
				goto ping_send;
			ping_num--;
			if(ping_num == 0)
				return -1;
			else
				goto ping_send;
		}
		if(buf_icmp_packet->type != 0 || buf_icmp_packet->code != 0)
			continue;
		
		syscall_monitor_write("Reply from [");
		for(i=0;i < 4;i++)
		{
			syscall_monitor_write_dec(((UINT8 *)&buf_icmp_packet->ip.ip_src)[i]);
			if(i < 3)
				syscall_monitor_write(".");
		}
		syscall_monitor_write("] bytes=");
		UINT32 bytes;
		buf_icmp_packet->ip.ip_len = net_swap_word(buf_icmp_packet->ip.ip_len);
		bytes = buf_icmp_packet->ip.ip_len - buf_icmp_packet->ip.ip_hl*4;
		syscall_monitor_write_dec(bytes);
		syscall_monitor_write(" ticks=");
		curtick = syscall_timer_get_tick();
		syscall_monitor_write_dec(curtick - origtick);
		syscall_monitor_write(" TTL=");
		syscall_monitor_write_dec(buf_icmp_packet->ip.ip_ttl);
		syscall_monitor_write("\n");
		if(infinite == TRUE)
			goto ping_send;
		ping_num--;
		if(ping_num == 0)
			return 1;
		else
			goto ping_send;
		return 1;
	}
	return 0;
}

int main(TASK * task, int argc, char * argv[])
{
	if(argc == 4)
	{
		syscall_network_getinfo(&net_info);
		if(net_info.isInit == FALSE)
		{
			syscall_monitor_write("network is not init!");
			return -1;
		}
		else if(net_info.ip_addr == 0)
		{
			syscall_monitor_write("you have no ip, you must use 'dhcp' or 'ipconf' command"
						" to set network address first!");
			return -1;
		}
		else if(net_info.route_gateway == 0)
		{
			syscall_monitor_write("you have no route gateway ip, you must use 'dhcp' or 'ipconf' command"
						" to set gateway address first!");
			return -1;
		}
		syscall_network_set_focus_task(task);
		syscall_set_input_focus(task);

		UINT32 ping_num = strToUInt(argv[1]);
		UINT32 timeout = strToUInt(argv[2]);
		timeout = (timeout * 1000) / 20;
		UINT8 gw_mac[6] = {0};
		int ret;
		ret = ping_get_gateway_mac(gw_mac, task, timeout);
		if(ret == -1 || ret == 0)
			return ret;
		ping_icmp_send_wait(gw_mac, argv[3], task, ping_num, timeout);
		syscall_network_set_focus_task(NULL);
	}
	else
		syscall_monitor_write("usage: ping num timeout_sec destip");
	return 0;
}

