/*
 * net_config.c
 *
 * Copyright (C) Emlab http://www.emlab.net. All rights reserved.
 *
 */

#include "net_config.h"
#include "command.h"
#include "emsp.h"

udp_datagram_info_t last_udi;

extern char * device_name_get(void);
void netconfig_reply(udp_datagram_info_t * udi, void *data, int len);


static int netconfig_handler(udp_datagram_info_t * udi, long data, int bytes)
{
    u32 result;
	u32 retlen = 0, datalen;
	u8 *retdata, *body, *p = (u8*)data;
    struct emsp_header *hdr = (struct emsp_header*)data;
    struct emsp_footer *ft;
    u16  cmd;

    result = hdr->result;
    cmd = hdr->cmdcode;
    body = &p[sizeof(struct emsp_header)];
    datalen = hdr->size - sizeof(struct emsp_header) - sizeof(struct emsp_footer); 
    if ((cmd == EMSP_CMD_SCAN_AP) || (cmd == EMSP_CMD_SCAN_CMP) ) {
        memcpy(&last_udi, udi, sizeof(last_udi));
	    emsp_cmd_do(cmd, datalen, body, &retlen, &result);
        return 0;
    }
    retdata = cmd_do(cmd, datalen, body, &retlen, &result);
    hdr = (struct emsp_header *)(retdata - sizeof(struct emsp_header));
	ft = (struct emsp_footer *)(retdata + retlen);
    hdr->result = result;
    hdr->size = retlen+sizeof(struct emsp_header)+sizeof(struct emsp_footer);
    hdr->cmdcode = cmd;
    hdr->checksum = calc_sum(hdr, sizeof(struct emsp_header) - 2);
    ft->checksum = calc_sum(retdata, retlen);
    
    netconfig_reply(udi, hdr, hdr->size);
    
	return 0;
}

void netconfig_init(void)
{
    udp_special_handler_register(IPPORT_NETCONFIG, netconfig_handler);

}

void netconfig_report(u16 cmd, u16 result, void *data, u16 datalen)
{
    struct emsp_header *hdr = (struct emsp_header*)data;
    struct emsp_footer *ft;

    hdr = (struct emsp_header *)((u8*)data - sizeof(struct emsp_header));
	ft = (struct emsp_footer *)((u8*)data + datalen);
    hdr->result = result;
    hdr->size = datalen+sizeof(struct emsp_header)+sizeof(struct emsp_footer);
    hdr->cmdcode = cmd;
    hdr->checksum = calc_sum(hdr, sizeof(struct emsp_header) - 2);
    ft->checksum = calc_sum(data, datalen);
    
    netconfig_reply(&last_udi, hdr, hdr->size);
}

void netconfig_reply(udp_datagram_info_t * udi, void *data, int len)
{
    udi->len = IPPORT_NETCONFIG;
	udp_write(NULL, data, len, 0, udi);
}

