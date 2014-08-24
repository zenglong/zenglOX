// ipconf.c -- 和ipconf工具相关的函数定义

#include "common.h"
#include "syscall.h"
#include "kheap.h"
#include "network.h"
#include "task.h"

NETWORK_INFO net_info;

int main(TASK * task, int argc, char * argv[])
{
	UNUSED(task);
	if(argc == 2 && !strcmp(argv[1], "-a"))
	{
		syscall_network_getinfo(&net_info);
		if(net_info.isInit == FALSE)
		{
			syscall_monitor_write("network is not init!");
			return -1;
		}
		int i;
		syscall_monitor_write("mac address: ");
		for(i=0;i < 6;i++)
		{
			syscall_monitor_write_hex(net_info.mac[i]);
			if(i < 5)
				syscall_monitor_write(":");
		}
		syscall_monitor_write("\nip addr: ");
		for(i=0;i < 4;i++)
		{
			syscall_monitor_write_dec(((UINT8 *)&net_info.ip_addr)[i]);
			if(i < 3)
				syscall_monitor_write(".");
		}
		syscall_monitor_write("\nnet mask: ");
		for(i=0;i < 4;i++)
		{
			syscall_monitor_write_dec(((UINT8 *)&net_info.net_mask)[i]);
			if(i < 3)
				syscall_monitor_write(".");
		}
		syscall_monitor_write("\nroute gateway: ");
		for(i=0;i < 4;i++)
		{
			syscall_monitor_write_dec(((UINT8 *)&net_info.route_gateway)[i]);
			if(i < 3)
				syscall_monitor_write(".");
		}
	}
	else if(argc == 5 && !strcmp(argv[1], "-set"))
	{
		net_getip_from_string(argv[2] ,(UINT8 *)&net_info.ip_addr);
		net_getip_from_string(argv[3] ,(UINT8 *)&net_info.net_mask);
		net_getip_from_string(argv[4] ,(UINT8 *)&net_info.route_gateway);
		syscall_network_setinfo(&net_info);
		syscall_monitor_write("network config success! you can use 'ipconf -a' to see your network configure");
	}
	else
		syscall_monitor_write("usage: ipconf [-a][-set ipaddr netmask gateway]");
	return 0;
}

