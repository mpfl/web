#ifndef __COMMAND_H__
#define __COMMAND_H__

#define USE_0302

#define	HW_VERSION	0x0206	
#ifdef USE_0302
#define FW_VERSION	0xf303
#define SUB_VERSION	0x0001
#else
#define FW_VERSION	0xf301
#define SUB_VERSION	0x0018
#endif

#define MF_INFO	{'m'^0x03,'x'^0x03,'c'^0x03,'h'^0x03,'i'^0x03,'p'^0x03}

#define ENTER_CRITICAL		__disable_irq	//disable_IRQ
#define EXIT_CRITICAL		__enable_irq	//enable_IRQ

//for get_module_status()
#define UART_CONFIG_MODE	0x00
#define UART_TO_WIFE_MODE	0x01
#define USER_CONFIG_MODE   0x02 /* user use PC tool to set to config mode. */

typedef enum __SYSTEM_STATE__{
	SYS_STATE_INIT = 0,	
	SYS_STATE_CONFIG, // config mode, 
	SYS_STATE_TRANS, // transport mode, 
	SYS_STATE_GOTO_DEFAULT, // system goto default mode.
	SYS_STATE_RESET, // system reload
	SYS_STATE_ONLY_WIFI, // only wifi connect, no socket connection
	SYS_STATE_DELAY_RELOAD,

	SYS_STATE_MAX_WORK_STATE,
	// below are not work state.
	SYS_STATE_WPS, // WPS connect
	SYS_STATE_WPS_FAIL, // WPS Fail
	SYS_STATE_WPS_SUCCESS,
}sys_state_e;

#define REAL_CONFIGLEN	sizeof(base_config_t)
typedef enum {
	NORMAL_MODE = 0,
	PINGPONG_MODE,
	SEND_FLAG_MODE,
} UART_MODE;

enum {
	BaudRate_9600 = 0,
	BaudRate_19200,
	BaudRate_38400,
	BaudRate_57600,
	BaudRate_115200,
	BaudRate_230400,
	BaudRate_460800,
	BaudRate_921600,
	BaudRate_1843200,
	BaudRate_3686400,
	BaudRate_4800 = 10,
	BaudRate_2400,
	BaudRate_1200,
	BaudRate_600,
	BaudRate_300,
	BaudRate_110,
};

typedef enum {
	UART_DATA_FLOW = 0,
	UART_DATA_SPEC1,   // This mode is used for data transport mode with special uart foramt.
	UART_TIME_STAMP_20ms,
	UART_TIME_STAMP_50ms,
	UART_TIME_STAMP_100ms,    //default mode
	UART_TIME_STAMP_150ms,
	UART_TIME_STAMP_200ms,
	UART_DATA_SPEC2,
} DATA_MODE;


#define DEVICE_NAME_MAX_LEN 40
#define MAX_DNS_LEN 64

// Command code
typedef enum {
	EMSP_CMD_RESET = 0x0001,	// reset module which is composed of STM32 and AW-GH320
	EMSP_CMD_GET_CONFIG,		// get wifi module configuration, ip address, tcp/udp, port etc.
	EMSP_CMD_SET_CONFIG,		// set wifi module configuration, ip address, tcp/udp, port etc.
	EMSP_CMD_SCAN_AP,		// scan ap
	EMSP_CMD_START ,			// start module
	EMSP_CMD_SEND_DATA ,		// send data to module, and the data will be send out use the WIFI
	EMSP_CMD_RECV_DATA,		// receive data from module
	EMSP_CMD_GET_STATUS,		// get module status
	EMSP_CMD_GET_VER,			// get module hardware & firmware version
	EMSP_CMD_GET_RF_POWER,	// get radio transmit power
	EMSP_CMD_SET_RF_POWER,	// set radio transmit power
	EMSP_CMD_GET_MAC_ADDR,	// get 88w8686's MAC address
	EMSP_CMD_SET_WPA,		// set wpa configuration
	EMSP_CMD_GET_WPA,  		// get wpa configuration
	EMSP_CMD_SET_FILE	,		// copy 4 files to flash
	EMSP_CMD_GET_FILE	,		// read 4 files from flash
	EMSP_CMD_SET_MAC,		// set MAC control
	EMSP_CMD_GET_MAC,		// get MAC control
	EMSP_CMD_SET_CHANNEL,	// set channel
	EMSP_CMD_GET_CHANNEL,	// get channel
	EMSP_CMD_SET_RATE,		// set data rate
	EMSP_CMD_GET_RATE,		// get data rate

	EMSP_CMD_GET_MF_INFO=0x0020,	// get manufacturer information

	// haibo added 2011-04-10, for VOIP
	EMSP_CMD_UDPOPEN = 0x0030, //Host->me, open an UDP port
	EMSP_CMD_UDPSEND, //Host->me, Host send a UDP buffer to me.
	EMSP_CMD_UDPRECV, // Me->Host, I send a UDP buffer to HOST.
	EMSP_CMD_UDPCLOSE, // Host->me, Host ask me close a UDP port.

       EMSP_CMD_SET_LOCALPORT,
	// haibo added 2011-05-02, for dynamic configuration change and get.
	EMSP_CMD_GET_IFCONFIG = 0x0040,
	EMSP_CMD_SET_IFCONFIG,
	EMSP_CMD_CONNECT_WIFI,
	EMSP_CMD_DISCONNECT_WIFI,
	EMSP_CMD_CLOSE_SOCKET,
	EMSP_CMD_OPEN_SOCKET,
	EMSP_CMD_GET_DEVICEID,
	EMSP_CMD_SET_DEVICEID,
	EMSP_CMD_SET_TXCONTROL,
	EMSP_CMD_GET_TXCONTROL,
	EMSP_CMD_WIFI_STOP,
	EMSP_CMD_WIFI_CONNECT,

	// haibo add for scan, get a specific ap's RSSI, and return current connected AP's RSSI.
	EMSP_CMD_SCAN_CMP = 0x0050,		// scan ap
	EMSP_CMD_SET_PINGPONG_MODE ,

	EMSP_CMD_SET_REMOTEDNS,		// remote is using DNS or not.
	EMSP_CMD_GET_REMOTEDNS,

	// reset system, delay x seconds boot
	EMSP_CMD_SAVE_CONFIG = 0x0060,

	EMSP_CMD_SET_UART_MODE = 0x0061,
	EMSP_CMD_GET_UART_MODE = 0x0062,

	EMSP_CMD_SET_PS_MODE = 0x0063, // set WIFI Power Save mode

	EMSP_CMD_SET_NEW_WPA = 0x0064,
	EMSP_CMD_GET_NEW_WPA = 0x0065,

	EMSP_CMD_GET_PS_MODE = 0x0066, // get WIFI Power Save mode

	EMSP_CMD_SET_DUAL_UAP,		// Set dual mode, uap ssid and security
	EMSP_CMD_GET_DUAL_UAP,		// Get dual mode, uap ssid and security

	EMSP_CMD_SET_EXTRA_SOCK,
	EMSP_CMD_GET_EXTRA_SOCK = 0x006A,

	EMSP_CMD_SET_KEEPALIVE = 0x006B,		// set and get tcp keepalive time and nums
	EMSP_CMD_GET_KEEPALIVE = 0x006C,

	EMSP_CMD_SET_SOCKS = 0x006D,		// set and get SOCKS proxy
	EMSP_CMD_GET_SOCKS = 0x006E,

	EMSP_CMD_GET_VER_STR, // Get version string format, 02060294.001

    EMSP_CMD_GET_RX_RSSI = 0x0070, // get wifi rx rssi
	
	EMSP_CMD_SET_COUNTRY = 0x0080, // set country code
	EMSP_CMD_GET_COUNTRY,
		
	EMSP_CMD_SYSTEM_BOOTUP = 0x00FF,


    
} EMSP_CMD;

typedef enum {
	SEC_MODE_WEP = 0,
	SEC_MODE_WPA_PSK,
	SEC_MODE_WPA_NONE,
	SEC_MODE_WEP_HEX,
	SEC_MODE_AUTO, // wep, none, wps psk auto.
	SEC_MODE_WPS_PBC,
	SEC_MODE_WPS_PIN,
}SEC_MODE;

enum {
	AD_HOC_MODE = 0,
	AP_CLIENT_MODE,
	AP_SERVER_MODE,
	DUAL_MODE, // AP Server + AP Client mode
};

typedef __packed struct _base_config_ {
	// WIFI
	u8 wifi_mode;		// Wlan802_11IBSS(0), Wlan802_11Infrastructure(1), Micro-AP(2), dual-mode(3)
	u8 wifi_ssid[32];	// 
	u8 wifi_wepkey[16];	// 40bit and 104 bit
	u8 wifi_wepkeylen;	// 5, 13

	// TCP/IP
	u8 local_ip_addr[16];		// if em380 is server, it is server's IP;	if em380 is client, it is em380's default IP(when DHCP is disable)
	u8 remote_ip_addr[16];		// if em380 is server, it is NOT used;		if em380 is client, it is server's IP
	u8 net_mask[16];			// 255.255.255.0
	u8 gateway_ip_addr[16];		// gateway ip address
	u8 portH;					// High Byte of 16 bit
	u8 portL;					// Low Byte of 16 bit
	u8 connect_mode;			// 0:server  1:client (if use_dhcp == 1, 0:udp broadcast 1:udp unicast)
	u8 use_dhcp;				// 0:disale, 1:enable
	u8 use_udp;					// 0:use TCP,1:use UDP

	// COM
	u8 UART_buadrate;	// 0:9600, 1:19200, 2:38400, 3:57600, 4:115200, 5:230400, 6:460800, 7:921600 
	u8 DMA_buffersize;	// 0:2, 1:16, 2:32, 3:64, 4:128, 5:256, 6:512
	u8 use_CTS_RTS;		// 0:disale, 1:enable
	u8 parity;			// 0:none, 1:even parity, 2:odd parity
	u8 data_length;		// 0:8, 1:9
	u8 stop_bits;		// 0:1, 1:0.5, 2:2, 3:1.5

	// DEVICE
	u8 device_num;		// 0 - 255

	u8 sec_mode; // 0 = wep, 1=wpa psk, 2=none, 3=wep_hex, 4=wps_pbc, 5=wps_pin
	u8 wpa_psk[32];
}base_config_t;

typedef __packed struct {
	char ssid[32];
	char key[32];
	u8 sec_mode; // 0 = wep, 1=wpa psk, 2=none, 3=wpa advanced
}new_wpa_conf_t;

typedef  __packed struct _extra_config_ {
	u32  flag1;
	u32  flag2;
	char my_device_name[DEVICE_NAME_MAX_LEN];
	__packed struct {
		int    enable;			// 1:use remote_dns as the rmote ip.
		char remote_name[MAX_DNS_LEN];
		char server[16];
	} dns_conf;

	DATA_MODE dataMode; // the time of uart data saved in buffer before forward.
	u8 wifi_ps_mode;   // 1=enable Power-Save mode, 0=disable
	u8 ps_unit; // 0=ms, 1=beacon interval
	int ps_utmo;  // timeout of unitcast
	int ps_mtmo;  // timeout of multicast
#define is_remote_dns dns_conf.enable
#define remote_dns	dns_conf.remote_name
#define dns_server dns_conf.server

	new_wpa_conf_t new_wpa_conf[4]; // support more wpa conf 

	int tx_power;
	u8 uap_ssid[32];
	u8 uap_key[32];
	u8 uap_secmode; // wep, none, and wpa

	u8 extra_sock_type;
	u16 extra_port;
	u8 extra_addr[64]; // ip or dns
	int tcp_keepalive_num;
	int tcp_keepalive_time;

	// SOCKS proxy. Now only support TCP client mode, other mode is disabled.
	__packed struct {
		u8 type; // 0=disable, 4=socks v4, 5=socks v5
		u8 socket_bitmask; // bit0:main socket, bit1:extra socket.
		u16 port; // socks server's port;
		u32 addr; // socks server's ip addr;
		u8 name[32]; // only used by socks v5
		u8 passwd[32]; // only used by socks v5
	} socks_conf;

    u8 web_user[32];
    u8 web_pass[32];

    u16 extra_lport;
    u16 main_lport;
}extra_config_t;

typedef __packed struct _sys_config_ {
	base_config_t base;
	extra_config_t extra;
}sys_config_t;

/* below is unconfigurable, system running changed
 * configuration is removed when save_config
 */
typedef __packed struct _sys_unconfigrable_ {
	// PMK, 0=uAP,1~5=sta group
	char pmk_invalid[6];
	char pmk[6][WLAN_PMK_LENGTH];// pmk[i] is valid when pmk_invalid[i]=0
}sys_unconfig_t;

extern sys_config_t *get_running_config(void);
extern sys_unconfig_t *get_un_config(void);
extern void get_config(void);
extern u8 *cmd_do(u32 cmd, u32 len, u8 *indata, u32 *outlen, u32 *presult);
extern void user_wifi_start(void);
extern void user_sock_start(void);
extern void user_wifi_stop(void);

#endif // __COMMAND_H__ END
