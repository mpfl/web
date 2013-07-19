/** 
  * command.c -- deal all module command
  */
#include "stm32f2xx.h"
#include "mxchipWNET.h"
#include "tcpip.h"
#include "command.h"
#include "emsp.h"

enum {
	NOT_SCAN = 0,
	EMSP_SCAN,
	EMSP_CMP_SCAN,
	HTTP_SCAN,
};

static sys_config_t running_config;
static sys_unconfig_t un_config;

static u8 respbuf[1500+12];
static u8 cmp_ssid[32];
static u8 user_wifi_started = 0;
static u8 user_sock_started = 0;
static u8 apnum;

extern u8 NFC_STARTED;

#define NETWORK_NAME "USER_NETWORK"

#define USER_SCAN_TIMEOUT 10*1000 // 10 seconds.
#define DEFAULTIP		"192.168.1.1"
#define DEFAULTREMOTEIP	"114.91.236.155"
#define DEFAULTNETMASK	"255.255.255.0"
#define DEFAULTDNS		"192.168.1.1"
#define DEFAULTGATEWAY	"192.168.1.1"
#define DEFAULTADHOCIP		DEFAULTIP
#define DEFAULT_PORT 		9999
#define UAP_KEY "11111111"
#define DNS_SERVER "test.solarinfobank.com"

#define MAGIC_FLAG		0x454D0380	// EM380
#define MAGIC2_FLAG		0x454D0382	// EM382
#define MAGIC_NUMBER    0x00000007	// configure version, if configure structure change, should add config transform function

#define BASE_CONFIG_ADDR     ((uint32_t)0x08006000) /* Base @ of Sector 1, 16 Kbytes */
#define UNCONFIG_ADDR		 ((uint32_t)0x08007000) /* Base @ of Sector 1, last 2 Kbytes */

static u8 emsp_scan = NOT_SCAN;
static u8 is_emsp_from_net = 0;

/* Lib used, return the system version str */
void system_version(char *str, int maxlen)
{
	snprintf(str, maxlen, "%04x%04x.%03x", HW_VERSION,
			 FW_VERSION, SUB_VERSION);
}

u8* http_scan(int *ap)
{
	u32 endtime;

	apnum = 0;
	user_scan();
	emsp_scan = HTTP_SCAN;
	memset(respbuf, 0, sizeof(respbuf));
	endtime = MS_TIMER+USER_SCAN_TIMEOUT;
	while(emsp_scan == HTTP_SCAN) {
		mxchipTick();
		if (endtime < MS_TIMER)
			break;
	}
	emsp_scan = NOT_SCAN;

	*ap = apnum;
	return &respbuf[sizeof(struct emsp_header)];
}

void mxchipScanCompleteHandler(UwtPara_str *pApList)
{
	int i;
	u8 tmprssi, ssid_len;
	u8 *data = &respbuf[sizeof(struct emsp_header)];
	u32 scanlen = 0, cmd = 0;
	
	if (emsp_scan == NOT_SCAN)
		return;

	if (emsp_scan == EMSP_SCAN)
		cmd = EMSP_CMD_SCAN_AP;
	else if (emsp_scan == EMSP_CMP_SCAN)
		cmd = EMSP_CMD_SCAN_CMP;
	
	apnum = pApList->ApNum;
	scanlen = 0;
	for (i=0;i<apnum;i++)
	{
		if (emsp_scan == EMSP_CMP_SCAN) {
			if ((strcmp(pApList->ApList[i].ssid, running_config.base.wifi_ssid) !=0) &&
				(strcmp(pApList->ApList[i].ssid, cmp_ssid) !=0)) {
				continue;
			}
		}
		tmprssi= (u8)pApList->ApList[i].rssi;
		ssid_len = strlen(pApList->ApList[i].ssid);
		strcpy(data, pApList->ApList[i].ssid);
		memcpy(data + ssid_len + 1, &tmprssi, 1);
		scanlen += (ssid_len + 1 + 2);
		if (scanlen > 1500)
			break;
		data+=(ssid_len + 1 + 2);
	}
	emsp_scan = NOT_SCAN;

	if (cmd != 0) {
	    if (is_emsp_from_net == 0)
            emsp_send_cmdpkt(cmd, apnum, &respbuf[sizeof(struct emsp_header)], scanlen, 0);
        else 
            netconfig_report(cmd, apnum, &respbuf[sizeof(struct emsp_header)], scanlen);
    }
}

/* running configure goto default */
void default_config(void)
{
	u8 mac[6];
	base_config_t *pbase = &running_config.base;
	extra_config_t *pextra = &running_config.extra;
	
	wlan_get_mac_address(mac);
    memset(&running_config, 0, sizeof(running_config));
    pbase->wifi_mode = DUAL_MODE;
	
    memcpy(pbase->local_ip_addr, DEFAULTIP, sizeof(DEFAULTIP));
    memcpy(pbase->remote_ip_addr, DEFAULTREMOTEIP, sizeof(DEFAULTREMOTEIP));
    memcpy(pbase->net_mask, DEFAULTNETMASK, sizeof(DEFAULTNETMASK));
    memcpy(pbase->gateway_ip_addr, DEFAULTGATEWAY, sizeof(DEFAULTGATEWAY));
    pbase->portH = DEFAULT_PORT/256;
    pbase->portL = DEFAULT_PORT%256;
    pbase->connect_mode = 1;	// server(if udp, broadcast)
    pbase->use_dhcp = 1;		// disable DHCP
    pbase->use_udp = 0;			// use TCP
    pbase->UART_buadrate = 0;	// 115200
    pbase->DMA_buffersize = 0;	// 2
    pbase->use_CTS_RTS = 0;		// disable
    pbase->parity = 0;			// no parity
    pbase->data_length = 0;		// 8 bits
    pbase->stop_bits = 0;		// 1 bit
    pbase->device_num = 0;
    pbase->sec_mode = SEC_MODE_WPA_NONE;
		
		sprintf(pextra->uap_ssid, "SGxTL-M_%02X%02X%02X", mac[3], mac[4], mac[5]);
    pextra->uap_secmode = SEC_MODE_WPA_PSK;
//  pextra->uap_key = UAP_KEY;
    sprintf(pextra->uap_key, UAP_KEY, sizeof(UAP_KEY));
    pextra->is_remote_dns = 1;
    strcpy(pextra->remote_dns, DNS_SERVER);
    pextra->extra_sock_type = TCP_SERVER;
    pextra->extra_port = 502;

    sprintf(pextra->my_device_name, "SGxTL-M (%02X%02X%02X)", mac[3], mac[4], mac[5]);
	pextra->dataMode = UART_TIME_STAMP_100ms;
	pextra->flag1= MAGIC2_FLAG;
// 	pextra->extra_sock_type = SOCK_DISABLE;
	pextra->ps_unit = 0;
	pextra->ps_mtmo = 200;
	pextra->ps_utmo = 1000;
    pextra->tcp_keepalive_num = 4;
    pextra->tcp_keepalive_time = 120;
    strcpy(pextra->web_user, "admin");
    strcpy(pextra->web_pass, "admin");

//     pextra->main_lport = (pbase->portH<<8) | pbase->portL;
    pextra->extra_lport = pextra->extra_port;
}

/* Verify the configuration, remove invalid configuration. 
  *
  */
static void config_verify(void)
{
	base_config_t *pbase = &running_config.base;
	extra_config_t *pextra = &running_config.extra;

    pbase->data_length = 0; // haibo 2012-12-11, don't allow user change uart data length.
	pbase->wifi_ssid[31] = 0;
	pbase->wifi_wepkey[15] = 0;
	pbase->local_ip_addr[15] = 0;
	pbase->net_mask[15] = 0;
	pbase->gateway_ip_addr[15] = 0;
	pbase->wpa_psk[31] = 0;
	pextra->new_wpa_conf[0].ssid[31] = 0;
	pextra->new_wpa_conf[1].ssid[31] = 0;
	pextra->new_wpa_conf[2].ssid[31] = 0;
	pextra->new_wpa_conf[3].ssid[31] = 0;
	pextra->uap_ssid[31] = 0;
	pextra->uap_key[31] = 0;
	pextra->extra_addr[63] = 0;
}

void get_config(void)
{
    u32 *p1 = (u32 *)BASE_CONFIG_ADDR;
	base_config_t *pbase = &running_config.base;
	extra_config_t *pextra = &running_config.extra;
	
    if (*p1 == MAGIC_FLAG && *(p1+1) == MAGIC_NUMBER) {
        memcpy((u8 *)&running_config, (u8 *)(BASE_CONFIG_ADDR+8), sizeof(running_config));
    } else {
        default_config();
    }
	
	memcpy((u8 *)&un_config, (u8 *)(UNCONFIG_ADDR), sizeof(sys_unconfig_t));
	if (pextra->socks_conf.type == 0xff) {
		memset(&pextra->socks_conf, 0, sizeof(pextra->socks_conf));
	}

    if (pextra->web_user[0] == 0xff) {
        strcpy(pextra->web_user, "admin");
        strcpy(pextra->web_pass, "admin");
    }
    auth_init(pextra->web_user, pextra->web_pass);
    pbase->data_length = 0;
}

void saveNFCConfig(void)
{
	int i, j;
	config_verify();

	if(NFC_GetSsidPassword(&running_config.base.wifi_ssid, &running_config.base.wpa_psk)){
		NFC_STARTED = 0;     //NFC tast is pending end
		return;
	}
	
	if(running_config.base.wifi_mode == AP_SERVER_MODE)
	  running_config.base.wifi_mode = AP_CLIENT_MODE;
	
	for(i=0; i<32; i++){
		if(running_config.base.wifi_ssid[i]==0x20){
			running_config.base.wifi_ssid[i]=0;
		}
		if(running_config.base.wpa_psk[i]==0x20){
			running_config.base.wpa_psk[i]=0;
		}
	}

	running_config.base.sec_mode = SEC_MODE_AUTO;
	running_config.base.use_dhcp = 1;

	flash_init(); // after erase, unconfigurable part is removed.
	flash_erase();
    FLASH_ProgramWord(BASE_CONFIG_ADDR+4, MAGIC_NUMBER);	// program MAGIC_NUMBER
    FLASH_ProgramWord(BASE_CONFIG_ADDR, MAGIC_FLAG);

	flash_write_data(BASE_CONFIG_ADDR+8, &running_config, sizeof(running_config));
	FLASH_Lock();
	NFC_STARTED = 0;
	reload();
}

/* write configure to flash */
void save_config(void)	
{
	config_verify();
	flash_init(); // after erase, unconfigurable part is removed.
	flash_erase();
    FLASH_ProgramWord(BASE_CONFIG_ADDR+4, MAGIC_NUMBER);	// program MAGIC_NUMBER
    FLASH_ProgramWord(BASE_CONFIG_ADDR, MAGIC_FLAG);
	flash_write_data(BASE_CONFIG_ADDR+8, &running_config, sizeof(running_config));
	FLASH_Lock();
}

/* write configure to flash and save the unconfigurable part. */
void save_unconfig(void)
{
	config_verify();
	flash_init();
    flash_erase();
    FLASH_ProgramWord(BASE_CONFIG_ADDR+4, MAGIC_NUMBER);	// program MAGIC_NUMBER
    FLASH_ProgramWord(BASE_CONFIG_ADDR, MAGIC_FLAG);
	flash_write_data(BASE_CONFIG_ADDR+8, &running_config, sizeof(running_config));
	flash_write_data(UNCONFIG_ADDR, &un_config, sizeof(un_config));
	FLASH_Lock();
}

sys_config_t *get_running_config(void)
{
	return &running_config;
}

sys_unconfig_t *get_un_config(void)
{
	return &un_config;
}

char * device_name_get(void)
{
	return running_config.extra.my_device_name;
}

int device_name_set(char *device_name)
{
	memcpy(running_config.extra.my_device_name, device_name, DEVICE_NAME_MAX_LEN);
	return 1;
}

/* Configure Wifi PowerSave mode.  */
void wifi_option_config(void)
{
	base_config_t *pbase = &running_config.base;
	extra_config_t *pextra = &running_config.extra;

	if (pextra->tcp_keepalive_num > 0) {
		set_tcp_keepalive(pextra->tcp_keepalive_num, 
						  pextra->tcp_keepalive_time);
	}
	
	if (pextra->tx_power != 0) // set rf tx power.
		wlan_set_tx_power(pextra->tx_power);

	if ((pextra->wifi_ps_mode == 1) && 
		(pbase->wifi_mode == AP_CLIENT_MODE))
		enable_ps_mode(pextra->ps_unit, pextra->ps_utmo, pextra->ps_mtmo);

	if (pextra->socks_conf.type == 4) {
		socks_init(4, pextra->socks_conf.addr, pextra->socks_conf.port, NULL, NULL);
	} else if (pextra->socks_conf.type == 5){
		socks_init(5, pextra->socks_conf.addr, pextra->socks_conf.port, 
				    pextra->socks_conf.name, pextra->socks_conf.passwd);
	}

    netconfig_init();
    tcp_config("HOSTNAME", pextra->my_device_name);
}

void emsp_cmd_do(u32 cmd, u32 len, u8 *indata, u32 *outlen, u32 *presult, int is_net)
{
	switch (cmd) {
	case EMSP_CMD_SCAN_AP:				// scan ap and return scan result to HOST
        user_scan();
		apnum = 0;
		emsp_scan = EMSP_SCAN;
		memset(respbuf, 0, sizeof(respbuf));
		break;
	case EMSP_CMD_SCAN_CMP:
		strncpy(cmp_ssid, indata, 32);
		user_scan();
		apnum = 0;
		emsp_scan = EMSP_CMP_SCAN;
		break;
	default:
		return;
	}
    is_emsp_from_net = is_net;
	return ;
}

/* Note: EMSP_CMD_SCAN_AP/EMSP_CMD_SCAN_CMP only accept from UART.
  * deal command, wait command response. 
  * INPUT argument
  * 		u32 cmd: the command nomber.
  *		u32 len: the byte length of the indata.
  * 		u8 *indata: the argument data of cmd.
  * OUTPUT argument
  *		return value: the response buffer
  *		u32*outlen: the byte length of response buffer
  *		u32*results: the command result.
  */
u8 *cmd_do(u32 cmd, u32 len, u8 *indata, u32 *outlen, u32 *presult)
{
	u8 *retdata = indata;
	u32 result, retlen = 0, *pval=(u32*)indata, endtime;
	base_config_t *pbase = &running_config.base;
	extra_config_t *pextra = &running_config.extra;

	switch (cmd) {	
	case EMSP_CMD_RESET:				// reset GH320 and stack or we can reset hardware
		result = 1;
		set_sys_state(SYS_STATE_RESET);
		
		break;
	case EMSP_CMD_GET_CONFIG:			// get current system configuration and return them to HOST
		result = 1;
		retlen = REAL_CONFIGLEN;
        memcpy(retdata, &running_config.base, retlen);
		break;
	case EMSP_CMD_SET_CONFIG:			// set new configuration
		memcpy(&running_config.base, indata, len);
		result = 1;
		retlen = len;
        pextra->main_lport = (pbase->portH<<8) | pbase->portL;
		save_config();

		break;
	case EMSP_CMD_SAVE_CONFIG:
	    result = 1;
	    retlen = 0;
	    save_config();
        break;
		
	case EMSP_CMD_START:				// start module and handle wifi/tcpip
		result = 1;
		user_wifi_start();
		user_sock_start();
		break;
	case EMSP_CMD_WIFI_STOP:
		result = 1;
		user_wifi_stop();
		break;
	case EMSP_CMD_WIFI_CONNECT:
		result = 1;
		set_sys_state(SYS_STATE_ONLY_WIFI);
		user_wifi_start();
		break;
	case EMSP_CMD_SEND_DATA:
		sock_output(indata, len, *presult);
		result = 1;
        retlen = wait_sock_response(retdata);
		break;
	case EMSP_CMD_RECV_DATA:
		retdata = sock_recv_command(&retlen);
		result = retlen;
		break;
	case EMSP_CMD_GET_STATUS:			// get module current status
		result = 1;
		retdata[0] = get_sys_state();
		retdata[1] = get_wifi_state();
		retdata[2] = get_sock_state(0);
		retdata[3] = get_sock_state(1);
		retlen = 4;
		break;

	case EMSP_CMD_GET_VER:
		*pval = HW_VERSION << 16;
		*pval |= FW_VERSION;
		result = 1;
		retlen = sizeof(u32);
		break;
        
	case EMSP_CMD_GET_MAC_ADDR:
		result = 1;
		wlan_get_mac_address(retdata);
		retlen = 6;
		break;

#ifdef ANONYMOUS_UDP
	case EMSP_CMD_UDPOPEN:
		// TODO
		break;
	case EMSP_CMD_UDPSEND:
		// TODO
		break;
	case EMSP_CMD_UDPCLOSE:
		// TODO
		break;
#endif
	case EMSP_CMD_GET_IFCONFIG:
		{
			net_para_t  netpara;

			if (running_config.base.wifi_mode != AP_SERVER_MODE)
				netpara.iface = 0;
			else
				netpara.iface = 1;
			GetNetPara(&netpara);
			result = 1;
			retlen = 48;
			memcpy(retdata, netpara.ip, 16) ;
			memcpy(retdata + 16, netpara.gateway, 16) ;
			memcpy(retdata + 32, netpara.netmask, 16) ;
		}
		break;
	case EMSP_CMD_SET_IFCONFIG:
		{
			char local_ip[16], gateway[16], netmask[16];
			net_para_set_t netpara;
			
			strncpy(local_ip, indata, 16);
			strncpy(gateway, indata+16, 16);
			strncpy(netmask, indata+32, 16);
			netpara.ip = inet_addr(local_ip);
			netpara.mask = inet_addr(netmask);
			netpara.gw = inet_addr(gateway);
			netpara.dhcp = indata[48];
			netpara.iface = 0;
			SetNetPara(&netpara);

			result = 1;
			retlen = 0;
		}
		break;
	case EMSP_CMD_CLOSE_SOCKET:
		result = 1;
		retlen = 0;
		sock_del_all();
		break;
	case EMSP_CMD_OPEN_SOCKET:
		result = 1;
		retlen = 0;

		if (indata[0] == 1) { // udp
			if (indata[1] == 1) { // unicast
				sock_add(UDP_UNICAST, 0, indata[2]*256 + indata[3], indata+4);
			} else { //  broadcast
				sock_add(UDP_BRDCAST, 0, indata[2]*256 + indata[3], NULL);
			}
		} else { // tcp
			if (indata[1] == 0) { // server
				sock_add(TCP_SERVER, indata[2]*256 + indata[3], 0, NULL);
			} else { // client
				sock_add(TCP_CLIENT, 0, indata[2]*256 + indata[3], indata+4);
			}
		}
		
		break;

	case EMSP_CMD_GET_TXCONTROL:
		// TODO
		break;
	case EMSP_CMD_GET_DEVICEID:
		retlen = DEVICE_NAME_MAX_LEN;
        memcpy(retdata, running_config.extra.my_device_name, retlen);
		result = 1;
		break;
	case EMSP_CMD_SET_DEVICEID:
		result= device_name_set(indata);
		retlen = 0;
		break;

	case EMSP_CMD_GET_REMOTEDNS:
	{
		memcpy(retdata, &(pextra->dns_conf), sizeof(pextra->dns_conf));
		retlen = sizeof(pextra->dns_conf);
		result = 1;
	}
		break;
	case EMSP_CMD_SET_REMOTEDNS:
		memcpy(&(pextra->dns_conf), indata, sizeof(pextra->dns_conf));

		result= 1;
		retlen = 0;
		break;
    case EMSP_CMD_SET_LOCALPORT:
	{
		u16 lport = *((u16*)&indata[0]);
		    	
		sock_set_lport(lport);
		sock_close_all();
    }
        break;
	case EMSP_CMD_GET_UART_MODE:
		retdata[0] = pextra->dataMode;
		retlen = 1;
		result = 1;
		break;
	case EMSP_CMD_SET_UART_MODE:
		pextra->dataMode = indata[0];

		result= 1;
		retlen = 0;
		break;
	case EMSP_CMD_SET_PS_MODE:
		pextra->wifi_ps_mode = indata[0];
		pextra->ps_unit = indata[1];
		memcpy((unsigned char*)&pextra->ps_utmo, &indata[2], 4);
		memcpy((unsigned char*)&pextra->ps_mtmo, &indata[6], 4);

		result = 1;
		retlen = 0;
		break;

	case EMSP_CMD_GET_PS_MODE:
		retlen = 10;
		retdata[0] = pextra->wifi_ps_mode;
		retdata[1] = pextra->ps_unit;
		memcpy(&retdata[2], (unsigned char*)&pextra->ps_utmo, 4);
		memcpy(&retdata[6], (unsigned char*)&pextra->ps_mtmo, 4);
		result = 1;
		break;

	case EMSP_CMD_SET_NEW_WPA:
		memcpy(pextra->new_wpa_conf, indata, sizeof(pextra->new_wpa_conf));
		retlen = 0;
		result = 1;

		break;
		
	case EMSP_CMD_GET_NEW_WPA:
		retlen = sizeof(pextra->new_wpa_conf);
		memcpy(retdata, pextra->new_wpa_conf, retlen);
		result = 1;
		break;
	case EMSP_CMD_SET_DUAL_UAP:
		memcpy(pextra->uap_ssid, indata, sizeof(new_wpa_conf_t));
		retlen = 0;
		result = 1;

		break;
		
	case EMSP_CMD_GET_DUAL_UAP:
		retlen = sizeof(new_wpa_conf_t);
		memcpy(retdata, pextra->uap_ssid, retlen);
		result = 1;
		break;
	case EMSP_CMD_SET_EXTRA_SOCK:
		result = 1;
		retlen = 0;
		memcpy(&pextra->extra_sock_type, indata, 67);
        pextra->extra_lport = pextra->extra_port;
		break;
	case EMSP_CMD_GET_EXTRA_SOCK:
		result = 1;
		retlen = 67;
		memcpy(retdata, &pextra->extra_sock_type, 67);
		break;
	case EMSP_CMD_GET_VER_STR:
		memset(retdata, 0, 16);
		retlen = 16;
		system_version(retdata, 16);
		result = 1;
        break;
        
	case EMSP_CMD_SET_KEEPALIVE:
	{
		int keepalive_num, keepalive_time;
		
		result = 1;
		retlen = 0;
		memcpy(&keepalive_num, indata, 4);
		memcpy(&keepalive_time, &indata[4], 4);
		pextra->tcp_keepalive_num = keepalive_num;
		pextra->tcp_keepalive_time = keepalive_time;
		set_tcp_keepalive(keepalive_num, keepalive_time);
	}
		break;
	case EMSP_CMD_GET_KEEPALIVE:
	{
		int keepalive_num, keepalive_time;
		
		result = 1;
		retlen = 8;
		get_tcp_keepalive(&keepalive_num, &keepalive_time);
		memcpy(retdata, &keepalive_num, 4);
		memcpy(&retdata[4], &keepalive_time, 4);
	}
		break;
	case EMSP_CMD_SET_SOCKS:
		result = 1;
		retlen = 0;
		memcpy(&pextra->socks_conf, indata, sizeof(pextra->socks_conf));
		break;
	case EMSP_CMD_GET_SOCKS:
		result = 1;
		retlen = sizeof(pextra->socks_conf);
		memcpy(retdata, &pextra->socks_conf, retlen);
		break;
    case EMSP_CMD_GET_RX_RSSI:
        *pval = get_rx_rssi();
		result = 1;
		retdata = (u8 *)pval;
		retlen = sizeof(u32);
        break;
	default:
		break;
	}

	*presult = result;
	*outlen = retlen;
	return retdata;
}

/* ---------- GPIO(output) set uart direction  ------------- */
void uart_dir_init()
{
	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	GPIO_WriteBit(GPIOA, GPIO_Pin_15, Bit_RESET);	
}

// dir = 0, receive
// dir = 1, send
void set_uart_is_sending(int is_sending)
{
	if  (uart_mode_get() != 2) 
		return;

	if(is_sending == 1)
	{
		GPIO_WriteBit(GPIOA, GPIO_Pin_15, Bit_SET);
	} else {
		GPIO_WriteBit(GPIOA, GPIO_Pin_15, Bit_RESET);	
	}
}

/* -------------------- GPIO(input) Module Status : Configuration mode or Data Transfer mode ----------------------- */
void module_status_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
}

u8 get_module_status(void)
{
	return GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_9);
}

static int add_extra_network(int index, char *ssid, int secmode, char *key)
{
	struct wlan_network wlan_config;
	sys_config_t *pconfig = get_running_config();
	base_config_t *pbase = &pconfig->base;
	extra_config_t *pextra = &pconfig->extra;
	sys_unconfig_t *punconfig = get_un_config();
	
	memset(&wlan_config, 0, sizeof(wlan_config));
	/* Set profile name */
	snprintf(wlan_config.name, "extra_%s", ssid, 32);
	strcpy(wlan_config.ssid, ssid);
	wlan_config.channel = 0;
	wlan_config.mode = WLAN_MODE_INFRASTRUCTURE;

	switch(secmode) {
	case SEC_MODE_WEP:
		wlan_config.security.type = WLAN_SECURITY_WEP_OPEN;
		strcpy(wlan_config.security.psk, key);
		wlan_config.security.psk_len = strlen(key);
		break;
	case SEC_MODE_WEP_HEX:
		wlan_config.security.type = WLAN_SECURITY_WEP_OPEN;
		wlan_config.security.psk_len = str2hex(pextra->uap_key, 
				wlan_config.security.psk, 32);
		break;
	case SEC_MODE_WPA_NONE:
		wlan_config.security.type = WLAN_SECURITY_NONE;
		break;
	case SEC_MODE_WPA_PSK:
		wlan_config.security.type = WLAN_SECURITY_WPA12;
		strcpy(wlan_config.security.psk, key);
		wlan_config.security.psk_len = strlen(key);
		break;
    case SEC_MODE_AUTO:
		wlan_config.security.type = WLAN_SECURITY_AUTO;
		strcpy(wlan_config.security.psk, key);
		wlan_config.security.psk_len = strlen(key);
		break;
	default:
		return 0;
	}

	if (pbase->use_dhcp == 1)
		wlan_config.address.addr_type = ADDR_TYPE_DHCP;
	else {
		wlan_config.address.addr_type = ADDR_TYPE_STATIC;
		wlan_config.address.ip = inet_addr(pbase->local_ip_addr);
		wlan_config.address.gw = inet_addr(pbase->gateway_ip_addr);
		wlan_config.address.netmask = inet_addr(pbase->net_mask);
		wlan_config.address.dns1 = inet_addr(pextra->dns_server);
	}
	if (punconfig->pmk_invalid[index+2] == 0) {
		wlan_config.security.pmk_valid = 1;
		memcpy(wlan_config.security.pmk, punconfig->pmk[index+2], WLAN_PMK_LENGTH);
	}
	if (0 == wlan_add_network(&wlan_config))
		return 1;
	else
		return 0;
}

static int extra_ap_start(void)
{
	sys_config_t *pconfig = get_running_config();
	base_config_t *pbase = &pconfig->base;
	extra_config_t *pextra = &pconfig->extra;
	struct wlan_network wlan_config;
	new_wpa_conf_t *pextra_net;
	int i, ret = 0;
	
	pextra_net = pextra->new_wpa_conf;
	for (i=0; i<4; i++) {
		if (pextra_net[i].ssid[0] == 0)
			continue;

		ret += add_extra_network(i, pextra_net[i].ssid, 
								 pextra_net[i].sec_mode, pextra_net[i].key);
	}

	return ret;
}

void user_wifi_start(void)
{
	sys_config_t *pconfig = get_running_config();
	base_config_t *pbase = &pconfig->base;
	extra_config_t *pextra = &pconfig->extra;
	sys_unconfig_t *punconfig = get_un_config();
	struct wlan_network wlan_config;
	struct wps_start_command wpscmd;

	if (user_wifi_started == 1)
		return;
	
	user_wifi_started = 1;
	switch(pbase->wifi_mode) {
	case DUAL_MODE:
	case AP_CLIENT_MODE:
		memset(&wlan_config, 0, sizeof(wlan_config));
		/* Set profile name */
		strcpy(wlan_config.name, NETWORK_NAME);
		strcpy(wlan_config.ssid, pbase->wifi_ssid);
		wlan_config.channel = 0;
		wlan_config.mode = WLAN_MODE_INFRASTRUCTURE;

		switch(pbase->sec_mode) {
		case SEC_MODE_WEP:
			wlan_config.security.type = WLAN_SECURITY_WEP_OPEN;
			strcpy(wlan_config.security.psk, pbase->wifi_wepkey);
			wlan_config.security.psk_len = pbase->wifi_wepkeylen;
			break;
		case SEC_MODE_WEP_HEX:
			wlan_config.security.type = WLAN_SECURITY_WEP_OPEN;
			wlan_config.security.psk_len = str2hex(pextra->uap_key, 
					wlan_config.security.psk, 32);
			break;
		case SEC_MODE_WPA_NONE:
			wlan_config.security.type = WLAN_SECURITY_NONE;
			break;
		case SEC_MODE_WPA_PSK:
			wlan_config.security.type = WLAN_SECURITY_WPA12;
			strcpy(wlan_config.security.psk, pbase->wpa_psk);
			wlan_config.security.psk_len = strlen(pbase->wpa_psk);
			break;
		case SEC_MODE_WPS_PBC:
			memset(&wpscmd, 0, sizeof(wpscmd));
			strncpy(wpscmd.ssid, pbase->wifi_ssid, 32);
			wpscmd.command = WPS_CMD_PBC;
			sprintf(wpscmd.name, NETWORK_NAME);
			wlan_wps_start(&wpscmd);
			return;
		case SEC_MODE_WPS_PIN:
			memset(&wpscmd, 0, sizeof(wpscmd));
			strncpy(wpscmd.ssid, pbase->wifi_ssid, 32);
			wpscmd.command = WPS_CMD_PIN;
			wpscmd.wps_pin = strtol(pbase->wpa_psk,NULL,10);
			sprintf(wpscmd.name, NETWORK_NAME);
			wlan_wps_start(&wpscmd);
			return;
		case SEC_MODE_AUTO:
			wlan_config.security.type = WLAN_SECURITY_AUTO;
			strcpy(wlan_config.security.psk, pbase->wpa_psk);
			wlan_config.security.psk_len = strlen(pbase->wpa_psk);
			break;
		default:
			return;
		}

		if (pbase->use_dhcp == 1)
			wlan_config.address.addr_type = ADDR_TYPE_DHCP;
		else {
			wlan_config.address.addr_type = ADDR_TYPE_STATIC;
			wlan_config.address.ip = inet_addr(pbase->local_ip_addr);
			wlan_config.address.gw = inet_addr(pbase->gateway_ip_addr);
			wlan_config.address.netmask = inet_addr(pbase->net_mask);
			wlan_config.address.dns1 = inet_addr(pextra->dns_server);
		}

		if (punconfig->pmk_invalid[1] == 0) {
			wlan_config.security.pmk_valid = 1;
			memcpy(wlan_config.security.pmk, punconfig->pmk[1], WLAN_PMK_LENGTH);
		}
		wlan_add_network(&wlan_config);
		if (extra_ap_start() > 0)
			wlan_connect(NULL);
		else
			wlan_connect(wlan_config.name);
		break;

	case AP_SERVER_MODE:
		memset(&wlan_config, 0, sizeof(wlan_config));
		/* Set profile name */
		strcpy(wlan_config.name, NETWORK_NAME);
		strcpy(wlan_config.ssid, pbase->wifi_ssid);
		wlan_config.channel = 0;
		wlan_config.mode = WLAN_MODE_UAP;

		switch(pbase->sec_mode) {
		case SEC_MODE_WEP:
			wlan_config.security.type = WLAN_SECURITY_WEP_OPEN;
			strcpy(wlan_config.security.psk, pbase->wifi_wepkey);
			wlan_config.security.psk_len = pbase->wifi_wepkeylen;
			break;
		case SEC_MODE_WEP_HEX:
			wlan_config.security.type = WLAN_SECURITY_WEP_OPEN;
			wlan_config.security.psk_len = str2hex(pextra->uap_key, 
					wlan_config.security.psk, 32);
			break;
		case SEC_MODE_WPA_NONE:
			wlan_config.security.type = WLAN_SECURITY_NONE;
			break;
		case SEC_MODE_WPA_PSK:
		default:
			wlan_config.security.type = WLAN_SECURITY_WPA2;
			strcpy(wlan_config.security.psk, pbase->wpa_psk);
			wlan_config.security.psk_len = strlen(pbase->wpa_psk);
			break;
		}

		if (pbase->use_dhcp == 1) {
			wlan_config.address.addr_type = ADDR_TYPE_STATIC;
			wlan_config.address.ip = 0xc0a80101;
			wlan_config.address.gw = 0xc0a80102;
			wlan_config.address.netmask = 0xffffff00;
			wlan_config.address.dns1 = 0xc0a80101;
		} else {
			wlan_config.address.addr_type = ADDR_TYPE_STATIC;
			wlan_config.address.ip = inet_addr(pbase->local_ip_addr);
			wlan_config.address.gw = inet_addr(pbase->gateway_ip_addr);
			wlan_config.address.netmask = inet_addr(pbase->net_mask);
			wlan_config.address.dns1 = inet_addr(pextra->dns_server);
		}

		if (punconfig->pmk_invalid[1] == 0) {
			wlan_config.security.pmk_valid = 1;
			memcpy(wlan_config.security.pmk, punconfig->pmk[1], WLAN_PMK_LENGTH);
		}
		wlan_add_network(&wlan_config);
		wlan_start_network(wlan_config.name);
		break;

    case AD_HOC_MODE: // Because we have removed AD-Hoc mode, set AD-Hoc to uAP mode.
        memset(&wlan_config, 0, sizeof(wlan_config));
		/* Set profile name */
		strcpy(wlan_config.name, NETWORK_NAME);
		strcpy(wlan_config.ssid, pbase->wifi_ssid);
		wlan_config.channel = 0;
		wlan_config.mode = WLAN_MODE_UAP;

		switch(pbase->sec_mode) {
		case SEC_MODE_WEP:
			wlan_config.security.type = WLAN_SECURITY_WEP_OPEN;
			strcpy(wlan_config.security.psk, pbase->wifi_wepkey);
			wlan_config.security.psk_len = pbase->wifi_wepkeylen;
			break;

		default:
			wlan_config.security.type = WLAN_SECURITY_NONE;
			break;
		}

		if (pbase->use_dhcp == 1) {
			wlan_config.address.addr_type = ADDR_TYPE_STATIC;
			wlan_config.address.ip = 0xc0a80101;
			wlan_config.address.gw = 0xc0a80102;
			wlan_config.address.netmask = 0xffffff00;
			wlan_config.address.dns1 = 0xc0a80101;
		} else {
			wlan_config.address.addr_type = ADDR_TYPE_STATIC;
			wlan_config.address.ip = inet_addr(pbase->local_ip_addr);
			wlan_config.address.gw = inet_addr(pbase->gateway_ip_addr);
			wlan_config.address.netmask = inet_addr(pbase->net_mask);
			wlan_config.address.dns1 = inet_addr(pextra->dns_server);
		}

		wlan_add_network(&wlan_config);
		wlan_start_network(wlan_config.name);
        break;
    default:
		break;
	}
}

void user_wifi_stop(void)
{
	sys_config_t *pconfig = get_running_config();
	base_config_t *pbase = &pconfig->base;
	extra_config_t *pextra = &pconfig->extra;

	if (user_wifi_started == 0)
		return;
	
	user_wifi_started = 0;
	wlan_remove_network(NETWORK_NAME);
}

void user_sock_start(void)
{
	sys_config_t *pconfig = get_running_config();
	base_config_t *pbase = &pconfig->base;
	extra_config_t *pextra = &pconfig->extra;
	u16 rport = (pbase->portH<<8) | pbase->portL;
    u16 lport = pextra->main_lport;

	if (user_sock_started == 1)
		return;
	
	user_sock_started = 1;
	if (pbase->use_udp == 0) {
		if (pbase->connect_mode == 0)
			sock_add(TCP_SERVER, lport, rport, NULL);
		else {
			if (pextra->is_remote_dns == 0)
				sock_add(TCP_CLIENT, lport, rport, pbase->remote_ip_addr);
			else
				sock_add(TCP_CLIENT, lport, rport, pextra->remote_dns);
		}
	} else {
		if (pbase->connect_mode == 0)
			sock_add(UDP_BRDCAST, rport, 0, NULL);
		else {
			if (pextra->is_remote_dns == 0)
				sock_add(UDP_UNICAST, lport, rport, pbase->remote_ip_addr);
			else
				sock_add(UDP_UNICAST, lport, rport, pextra->remote_dns);
		}
	}

	sock_add(pextra->extra_sock_type, pextra->extra_lport, pextra->extra_port, pextra->extra_addr);
}

/* Check whether need SOKCS Proxy. return 1=need,0=needn't
	
*/
int is_socks_needed(int socket_num, u32 rip)
{
	sys_config_t *pconfig = get_running_config();
	base_config_t *pbase = &pconfig->base;
	extra_config_t *pextra = &pconfig->extra;

	if (pextra->socks_conf.type == 0)
		return 0;

	if (!(pextra->socks_conf.socket_bitmask & (1<<socket_num)))
		return 0;

	if (is_all_nonlocal_subnet(rip))
		return 1;
	else
		return 0;
}

void wps_pbc_start(void)
{
	struct wps_start_command wpscmd;
	
	memset(&wpscmd, 0, sizeof(wpscmd));
	wpscmd.command = WPS_CMD_PBC;
	sprintf(wpscmd.name, NETWORK_NAME);
	wlan_wps_start(&wpscmd);
}

int wps_callback(enum wps_event event, void *data, uint16_t len)
{
	unsigned long msg;
	int err;
	struct wlan_network *network = (struct wlan_network *)data;
	base_config_t *pbase = &running_config.base;
	
	switch(event) {

	case WPS_SESSION_PIN_CHKSUM_FAILED:
	case WPS_SESSION_ABORTED:
	case WPS_SESSION_TIMEOUT:
	case WPS_SESSION_FAILED:
		set_sys_state(SYS_STATE_WPS_FAIL);
		break;

	case WPS_SESSION_SUCCESSFUL:
		pbase->wifi_mode = AP_CLIENT_MODE;
		pbase->use_dhcp = 1;
		strcpy(pbase->wifi_ssid, network->ssid);
		switch(network->security.type) {
		case WLAN_SECURITY_NONE:
			pbase->sec_mode = SEC_MODE_WPA_NONE;
			break;
			
		case WLAN_SECURITY_WEP_OPEN:
		case WLAN_SECURITY_WEP_SHARED:
			pbase->sec_mode = SEC_MODE_WEP;
			strcpy(pbase->wifi_wepkey, network->security.psk);
			pbase->wifi_wepkeylen = network->security.psk_len;
			break;

		default:
			pbase->sec_mode = SEC_MODE_WPA_PSK;
			strcpy(pbase->wpa_psk, network->security.psk);
			break;
		}
		set_sys_state(SYS_STATE_WPS_SUCCESS);
		break;

	default:
		break;
	}
	return 0;
}

static void trans_dot(char *dst, char *src) 
{
    while(*src!= 0) {
        if (*src == '.') {
            *dst++ = '/';
            *dst++ = '.';
            src++;
        } else {
            *dst++ = *src++;
        }
    }
    *dst = 0;
}
void mdns_begin(int iface)
{
    static int mlan_start = 0, uap_start = 0;
    base_config_t *pbase = &running_config.base;
    extra_config_t *pextra = &running_config.extra;
    char *txt_attr;
    u16 rport = (pbase->portH<<8) | pbase->portL;
    u16 lport = pextra->main_lport;
    char tmpstr[80];
    u8 mac[6];
    
    if (iface == 0 && mlan_start == 1)
        return;

    if (iface == 1 && uap_start == 1)
        return;

    if (iface == 0)
        mlan_start = 1;
    else
        uap_start = 1;

    txt_attr = malloc(500);
    system_version(txt_attr, 500);
    trans_dot(tmpstr, txt_attr);
    sprintf(txt_attr, "Vendor=SUNGROW.Version=%s.", tmpstr);
    if (pbase->use_udp == 1) {
        if (pbase->connect_mode == 0) {
            sprintf(txt_attr, "%sSocket1_Type=UDP broadcast.Socket1_LPort=%d.Socket1_RPort=%d.",
                    txt_attr, lport, rport); 
        } else {
            if (pextra->is_remote_dns == 0)
                trans_dot(tmpstr, pbase->remote_ip_addr);
            else
                trans_dot(tmpstr, pextra->remote_dns);
            sprintf(txt_attr, "%sSocket1_Type=UDP unicast.Socket1_LPort=%d.Socket1_RPort=%d." 
                    "Socket1_Raddr=%s.", txt_attr, lport, rport, tmpstr); 
        }
    } else {
        if (pbase->connect_mode == 0) {
            sprintf(txt_attr, "%sSocket1_Type=TCP Server.Socket1_Port=%d.", 
                    txt_attr, lport); 
        } else {
            if (pextra->is_remote_dns == 0)
                trans_dot(tmpstr, pbase->remote_ip_addr);
            else
                trans_dot(tmpstr, pextra->remote_dns);
            sprintf(txt_attr, "%sSocket1_Type=TCP Client.Socket1_Port=%d." 
                    "Socket1_Raddr=%s.", txt_attr, rport, tmpstr); 
        }
    }

    switch(pextra->extra_sock_type) {
    case TCP_SERVER:
        sprintf(txt_attr, "%sSocket2_Type=TCP Server.Socket2_Port=%d.", 
                txt_attr, pextra->extra_lport); 
        break;
    case TCP_CLIENT:
        trans_dot(tmpstr, pextra->extra_addr);
        sprintf(txt_attr, "%sSocket2_Type=TCP Client.Socket2_Port=%d." 
                "Socket2_Raddr=%s.", txt_attr, pextra->extra_port, tmpstr); 
        break;
    case UDP_UNICAST:
        trans_dot(tmpstr, pextra->extra_addr);
        sprintf(txt_attr, "%sSocket2_Type=UDP unicast.Socket2_LPort=%d.Socket2_RPort=%d." 
                "Socket2_Raddr=%s.", txt_attr, pextra->extra_lport, pextra->extra_port, tmpstr); 
        break;
    case UDP_BRDCAST:
        sprintf(txt_attr, "%sSocket2_Type=UDP broadcast.Socket2_LPort=%d.Socket2_RPort=%d.", 
                txt_attr, pextra->extra_lport, pextra->extra_port); 
        break;

    default:
        break;
    }

	wlan_get_mac_address(mac);
    sprintf(tmpstr, "SGxTL-M_%02X%02X%02X.local", mac[3], mac[4], mac[5]);
    init_mdns(pextra->my_device_name, tmpstr, txt_attr, 80, iface);

}


