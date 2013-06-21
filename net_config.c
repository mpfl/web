/*
 * net_config.c
 *
 * Copyright (C) Emlab http://www.emlab.net. All rights reserved.
 *
 */

#include "net_config.h"

extern char * device_name_get(void);


static int netconfig_handler(udp_datagram_info_t * udi, long data, int bytes)
{
	NET_CONFIG_T *p_cmd = (NET_CONFIG_T *)data;
	NET_SEARCH_T reply;
	net_para_t  netpara;
    int i,j;
    
	netpara.iface = udi->iface;
	GetNetPara(&netpara);
	if (bytes < 8)
		return 0;

	if (p_cmd->magic != NET_CONFIG_MAGIC)
		return 0;

	if (p_cmd->cmd == NET_CONFIG_SEARCH) {
		u32 version;
		u32 ip;
		u8 mac[6];
		
		memset(&reply, 0, sizeof(reply));
		reply.magic = NET_CONFIG_MAGIC;
		reply.cmd = NET_CONFIG_SEARCH;
		
		strcpy(reply.ip, netpara.ip);

        i = j = 0;
        while(1) {
            reply.mac[j++] = netpara.mac[i++];
            reply.mac[j++] = netpara.mac[i++];
            if (j < 17)
                reply.mac[j++] = ':';
            else
                break;
        }
        
        system_version(reply.ver, 13);
		sprintf(reply.device_name, device_name_get());
		udi->len = IPPORT_NETCONFIG;
		udp_write(NULL, paddrSS(&reply), sizeof(reply), 0, udi);
		return 1;
	}

	return 0;
}

void netconfig_init(void)
{
    udp_special_handler_register(IPPORT_NETCONFIG, netconfig_handler);

}

