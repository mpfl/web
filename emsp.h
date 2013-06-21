#ifndef __EMSP_H_
#define __EMSP_H_

//
// module (STM32 + GH320) work flow
// @ADHOC	RESET --> CONFIG_WIFI --> CONFIG_NETSTACK --> START --> RECV --> SEND --
//			  |														 |<------------|
//			  |<-------------------------------------------------------------|

#define EMSP_MIN_PACKET_SIZE		(sizeof(struct emsp_header) + sizeof(struct emsp_footer))
#define EMSP_MAX_PACKET_SIZE		1024
#define EMSP_MAX_PACKET_DATA_SIZE	(EMSP_MAX_PACKET_SIZE - sizeof(struct emsp_header) - sizeof(struct emsp_footer))
#define TIMEOUT						0xFFFF

__packed struct emsp_header {
	u16 cmdcode;	// Command code
	u16 size;		// size of the packet, including the header and footer
	u16 result;		// Result code, set by module. 
					// This field is used only the response packet. Set 0 in the request packet
	u16 checksum;	// check sum of the header
};

__packed struct emsp_footer {
	u16 checksum;	// check sum of the packet body( not include the header)
};

struct ap_list {
	u8 ssid[32];
	u32 rssi;
	u32 channel;
	u32 privacy;	// wep enabled ?
	u32 type;		// adhoc(0), ap client(1)
	u32 rate;		//
};


extern void emsp_init(void);
extern void emsp_tick(void);

#endif
