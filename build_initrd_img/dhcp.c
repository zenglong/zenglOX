// dhcp.c -- 和dhcp工具相关的函数定义

#include "common.h"
#include "syscall.h"
#include "kheap.h"
#include "network.h"
#include "task.h"

#define DHCP_OP_REQUEST 1
#define DHCP_OP_REPLY 2

#define DHCP_DISCOVER 1
#define DHCP_OFFER 2
#define DHCP_REQUEST 3
#define DHCP_DECLINE 4
#define DHCP_ACK 5
#define DHCP_NAK 6
#define DHCP_RELEASE 7

UINT8 magic[] = {0x63, 0x82, 0x53, 0x63};
NETWORK_INFO net_info;
UINT8 buf[2048];

int dhcp_getoption(ETH_DHCP_PACKET * dhcp, UINT8 msg, UINT8 * out, UINT32 len)
{
	UINT8 *options = ((UINT8 *)dhcp) + sizeof(ETH_DHCP_PACKET);
	int found = 0;
	while(!found)
	{
		if(*options == msg)
		{
			found = 1;
			break;	
		}else if(*options == 0xff){
			return -1;	
		}
		options += options[1] + 2;
	}
	if(len == 0)
		len = options[1];
	memcpy(out, options + 2, len);
	return 0;
}

VOID dhcp_populate_header(ETH_DHCP_PACKET * dhcp, UINT8 ** options, 
				UINT8 op, UINT8 msg)
{
	dhcp->op = op;
	dhcp->htype = 1;
	dhcp->hlen = 6;
	memcpy((UINT8 *)&dhcp->xid, &net_info.mac[2], 4);
	memcpy(dhcp->chaddr, net_info.mac, 6);
	memcpy(dhcp->magic, magic, 4);
	
	*(*options)++ = 53;
	*(*options)++ = 1;
	*(*options)++ = msg;
}

VOID dhcp_end_header(UINT8 **options)
{
	*(*options)++ = 0xff;
}

SINT32 dhcp_send(ETH_DHCP_PACKET * dhcp, UINT8 * dhcp_start, UINT8 * options_end)
{
	UINT32 dhcp_len = options_end - dhcp_start;
	UINT8 hdr_dmac[6] = {0};
	memset(hdr_dmac, 0xff, 6);

	net_make_udp((UINT8 *)dhcp,
			net_info.mac, hdr_dmac,
			0, 0xffffffff,
			68, 67, 
			dhcp_len, buf);
	
	return syscall_network_send((UINT8 *)dhcp, (options_end - (UINT8 *)dhcp));
}

SINT32 dhcp_send_discover()
{
	ETH_DHCP_PACKET * dhcp = (ETH_DHCP_PACKET *)syscall_umalloc(sizeof(ETH_DHCP_PACKET) + 256);
	memset((UINT8 *)dhcp, 0, sizeof(ETH_DHCP_PACKET) + 256);
	UINT8 * options = ((UINT8 *)dhcp) + sizeof(ETH_DHCP_PACKET);

	dhcp_populate_header(dhcp, &options, DHCP_OP_REQUEST, DHCP_DISCOVER);
	dhcp_end_header(&options);
	UINT8 * dhcp_start = (UINT8 *)dhcp + sizeof(ETH_HEADER) + 
				sizeof(ETH_IP) + sizeof(ETH_UDP);
	SINT32 ret = dhcp_send(dhcp, dhcp_start, options);
	syscall_ufree(dhcp);
	return ret;
}

SINT32 dhcp_send_request(ETH_DHCP_PACKET * recv_dhcp)
{
	ETH_DHCP_PACKET * dhcp = (ETH_DHCP_PACKET *)syscall_umalloc(sizeof(ETH_DHCP_PACKET) + 256);
	memset((UINT8 *)dhcp, 0, sizeof(ETH_DHCP_PACKET) + 256);
	UINT8 * options = ((UINT8 *)dhcp) + sizeof(ETH_DHCP_PACKET);

	dhcp_populate_header(dhcp, &options, DHCP_OP_REQUEST, DHCP_REQUEST);	

	//parameter request list
	*options++ = 55;
	*options++ = 0x3;
	*options++ = 0x01;//Subnet mask
	*options++ = 0x03;//Router
	*options++ = 0x06;//DNS

	//request ip address
	*options++ = 50;
	*options++ = 0x04;
	*((UINT32 *)options) = recv_dhcp->yiaddr;
	options += 4;

	//DHCP server that sent offer
	*options++ = 54;
	*options++ = 0x04;
	dhcp_getoption(recv_dhcp, 54, options, 4);
	options+=4;

	dhcp_end_header(&options);

	UINT8 * dhcp_start = (UINT8 *)dhcp + sizeof(ETH_HEADER) + 
				sizeof(ETH_IP) + sizeof(ETH_UDP);
	SINT32 ret = dhcp_send(dhcp, dhcp_start, options);
	syscall_ufree(dhcp);
	return ret;
}

UINT8 dhcp_type(ETH_DHCP_PACKET * dhcp)
{
	UINT8 * options = ((UINT8 *)dhcp) + sizeof(ETH_DHCP_PACKET);
	while(*options != 53)
		options++;
	options += 2;
	return *options;
}

int wait_for_reply(TASK * task, TASK_MSG * msg)
{
	UINT32 origtick = syscall_timer_get_tick();
	ETH_DHCP_PACKET * buf_packet;
	int ret;
	while(TRUE)
	{
		ret = syscall_get_tskmsg(task,msg,TRUE);
		if(ret != 1)
		{
			syscall_idle_cpu();
			UINT32 curtick = syscall_timer_get_tick();
			if((curtick - origtick) > 3000)
			{
				syscall_monitor_write("wait for dhcp reply time out curtick:");
				syscall_monitor_write_dec(curtick);
				syscall_monitor_write(" origtick:");
				syscall_monitor_write_dec(origtick);
				return -1;
			}
			continue;
		}
		if(msg->type == MT_KEYBOARD &&
			msg->keyboard.type == MKT_ASCII &&
			msg->keyboard.ascii == 0x1B) // ESC
			break;
		if(msg->type != MT_NET_PACKET)
			continue;
		if(syscall_network_get_packet(task, msg->packet_idx, buf, 2048) == -1)
			continue;
		buf_packet = (ETH_DHCP_PACKET *)buf;
		buf_packet->eth.hdr_type = net_swap_word(buf_packet->eth.hdr_type);
		if(buf_packet->eth.hdr_type != 0x800)
			continue;
		if(buf_packet->ip.ip_p != NET_IP_PROTO_UDP)
			continue;
		buf_packet->udp.dst_port = net_swap_word(buf_packet->udp.dst_port);
		if(buf_packet->udp.dst_port != 68)
			continue;
		if(buf_packet->op != DHCP_OP_REPLY)
			continue;
		return 1;
	}
	return 0;
}

int main(TASK * task, int argc, char * argv[])
{
	UNUSED(argc);
	UNUSED(argv);

	ETH_DHCP_PACKET * dhcp = NULL;
	syscall_network_getinfo(&net_info);
	if(net_info.isInit == FALSE)
	{
		syscall_monitor_write("network is not init!");
		return -1;
	}
	syscall_network_set_focus_task(task);
	syscall_set_input_focus(task);
	if(dhcp_send_discover() == -1)
	{
		syscall_monitor_write("send dhcp discover time out");
		return -1;
	}
	else
		syscall_monitor_write("send dhcp discover and wait for reply\n");
	TASK_MSG msg;
	int ret = wait_for_reply(task, &msg);
	if(ret == 0 || ret == -1)
		return ret;
	UINT8 type = dhcp_type((ETH_DHCP_PACKET *)buf);
	if(type == DHCP_OFFER)
		syscall_monitor_write("received dhcp offer reply\n");
	else
	{
		syscall_monitor_write("received dhcp reply is not an OFFER, exit");
		return -1;
	}
	if(dhcp_send_request((ETH_DHCP_PACKET *)buf) == -1)
	{
		syscall_monitor_write("send dhcp request time out");
		return -1;
	}
	else
		syscall_monitor_write("send dhcp request and wait for reply\n");
	ret = wait_for_reply(task, &msg);
	if(ret == 0 || ret == -1)
		return ret;
	type = dhcp_type((ETH_DHCP_PACKET *)buf);
	if(type == DHCP_ACK)
	{
		dhcp = (ETH_DHCP_PACKET *)buf;
		net_info.ip_addr = dhcp->yiaddr;
		dhcp_getoption(dhcp, 1, (UINT8 *)&net_info.net_mask, 4);
		dhcp_getoption(dhcp, 3, (UINT8 *)&net_info.route_gateway, 4);
		syscall_monitor_write("received dhcp ack , config is follow:\n");
		syscall_monitor_write("ip addr: ");
		int i;
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
		syscall_network_setinfo(&net_info);
	}
	else
	{
		syscall_monitor_write("received dhcp reply is not an ACK, exit");
		return -1;
	}
	return 0;
}

