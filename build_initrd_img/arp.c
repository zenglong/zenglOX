// arp.c -- 和arp工具相关的函数定义

#include "network.h"
#include "common.h"
#include "syscall.h"
#include "task.h"

ETH_ARP_PACKET test_packet;
NETWORK_INFO net_info;
ETH_ARP_PACKET * buf_packet;
UINT8 buf[2048];

int main(TASK * task, int argc, char * argv[])
{
	if(argc == 2)
	{
		int ret;
		int i;
		UINT32 origtick, frequency, time_out_ticks;
		TASK_MSG msg;
		syscall_network_getinfo(&net_info);
		if(net_info.isInit == FALSE)
		{
			syscall_cmd_window_write("network is not init!");
			return -1;
		}
		/*else if(net_info.ip_addr == 0)
		{
			syscall_cmd_window_write("you have no ip, you must use 'dhcp' or 'ipconf' command"
						" to set network address first!");
			return -1;
		}*/
		syscall_network_set_focus_task(task);
		syscall_set_input_focus(task);

		UINT8 hdr_dmac[6] = {0};
		memset(hdr_dmac, 0xff, 6);

		UINT8 dest_ip[4] = {0};
		net_getip_from_string(argv[1], dest_ip);

		net_make_arp((UINT8 *)&test_packet,
			net_info.mac, hdr_dmac,
			net_info.ip_addr, *((UINT32 *)dest_ip),
			0x01);

		if(syscall_network_send((UINT8 *)&test_packet, sizeof(test_packet)) == -1)
		{
			syscall_cmd_window_write("send arp request time out");
			return -1;
		}
		else
			syscall_cmd_window_write("send arp request and wait for reply\n");
		origtick = syscall_timer_get_tick();
		frequency = syscall_timer_get_frequency();
		time_out_ticks = 60 * frequency;
		while(TRUE)
		{
			ret = syscall_get_tskmsg(task,&msg,TRUE);
			if(ret != 1)
			{
				syscall_idle_cpu();
				UINT32 curtick = syscall_timer_get_tick();
				if((curtick - origtick) > time_out_ticks)
				{
					syscall_cmd_window_write("wait for arp reply time out curtick:");
					syscall_cmd_window_write_dec(curtick);
					syscall_cmd_window_write(" origtick:");
					syscall_cmd_window_write_dec(origtick);
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
			syscall_cmd_window_write("the dest ip [");
			for(i=0;i < 4;i++)
			{
				syscall_cmd_window_write_dec(buf_packet->tx_ip[i]);
				if(i < 3)
					syscall_cmd_window_write(".");
			}
			syscall_cmd_window_write("] mac is [");
			for(i=0;i < 6;i++)
			{
				syscall_cmd_window_write_hex(buf_packet->tx_mac[i]);
				if(i < 5)
					syscall_cmd_window_write(":");
			}
			syscall_cmd_window_write("]\n");
			break;
		}
		syscall_network_set_focus_task(NULL);
	}
	else
		syscall_cmd_window_write("usage: arp destip");
	return 0;
}

