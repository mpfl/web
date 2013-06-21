#ifndef _NET_CONFIG_H_
#define _NET_CONFIG_H_

#include "stm32f2xx.h"
#include "mxchipWNET.h"
#include "command.h"

/*
 * net_config.h
 *
 * Copyright (C) Emlab http://www.emlab.net. All rights reserved.
 * UDP broadcast search our module. Module reply IP, MAC, Device Name, Version.
 * UDP unicast send and receive confguration from module.
 */


#define IPPORT_NETCONFIG 8089

#define NET_CONFIG_MAGIC 0x454D0380

enum {
	NET_CONFIG_SEARCH = 1,
};

typedef __packed struct _net_config_ {
	u32 magic; // magic num for our protocol
	u32 cmd;	//
	u8 data[4]; // 
} NET_CONFIG_T;

typedef __packed struct _net_search_ {
	u32 magic; // magic num for our protocol
	u32 cmd;	//
	u8 ip[16];
	u8 mac[18];
	u8 ver[12];
	u8 device_name[DEVICE_NAME_MAX_LEN];
} NET_SEARCH_T;

#endif //_NET_CONFIG_H_

