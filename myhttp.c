#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "mxchipWNET.h"
#include "command.h"
#include "tcpip.h"
#include "base64.h"
#include "script_data.h"

#define FALSE 0
#define TRUE 1

#define HTTP_DATA_MAX_LEN 2048

u8 scan_for_http = 0;
static char basic_ssid[32];
/****************************** http ***************************************/

static int httpfd = 0;
static int httpclientfd[MAX_TCP_CLIENT];

static  char *httpRequest;
static char *auth_str = NULL;

static void HandleHttpClient(int index);



const char authrized[] =
{
"HTTP/1.1 401 Authorization Required\r\n"
"Server: MySocket Server\r\n"
"WWW-Authenticate: Basic realm=\"MXCHIP 3280\"\r\n"
"Content-Type: text/html\r\n"
"Content-Length: 169\r\n\r\n"
"<HTML>\r\n<HEAD>\r\n<TITLE>Error</TITLE>\r\n"
"<META HTTP-EQUIV=\"Content-Type\" CONTENT=\"text/html; charset=ISO-8859-1\">\r\n"
"</HEAD>\r\n<BODY><H1>401 Unauthorized.</H1></BODY>\r\n</HTML>"
};

const char HTTPSaveResponse[] =
{
"HTTP/1.1 200 OK\r\n\
Server: Emlab Server\r\n\
Date: TEST\r\n\
Content-Type: text/html\r\n\
Content-Length: %d\r\n\
Accept-Ranges: bytes\r\n\
Connection: close\r\n\r\n\
%s"
};

const char basicResponseSucc[]=
{
"<html>\r\n\
<head>\r\n\
<title>MXCHIP Wi-Fi module</title>\r\n\
</head>\r\n\
<body>\r\n\
<p>Save Config Done!<a href=\"/basic.htm\">Return</a></p>\r\n\
</body>\r\n\
</html>"
};


const char basicResponseError[]=
{
"<html>\r\n\
<head>\r\n\
<title>MXCHIP Wi-Fi module</title>\r\n\
</head>\r\n\
<body>\r\n\
<p>Save Config Error, please retry<a href=\"/basic.htm\">Return</a></p>\r\n\
</body>\r\n\
</html>"
};


const char advanceResponseSucc[]=
{
"<html>\r\n\
<head>\r\n\
<title>MXCHIP Wi-Fi module</title>\r\n\
</head>\r\n\
<body>\r\n\
<p>Save Config Done!<a href=\"/advanced.htm\">Return</a></p>\r\n\
</body>\r\n\
</html>"
};

const char advanceResponseError[]=
{
"<html>\r\n\
<head>\r\n\
<title>MXCHIP Wi-Fi module</title>\r\n\
</head>\r\n\
<body>\r\n\
<p>Save Config Error, please retry<a href=\"/advanced.htm\">Return</a></p>\r\n\
</body>\r\n\
</html>"
};

const char systemResponseSucc[]=
{
"<html>\r\n\
<head>\r\n\
<title>MXCHIP Wi-Fi module</title>\r\n\
</head>\r\n\
<body>\r\n\
<p>Firmware update success, system reboot...please wait 5 seconds and refresh</p>\r\n\
</body>\r\n\
</html>"
};

const char systemResponseError[]=
{
"<html>\r\n\
<head>\r\n\
<title>MXCHIP Wi-Fi module</title>\r\n\
</head>\r\n\
<body>\r\n\
<p>Firmware update fail, system reboot...please wait 5 seconds and refresh</p>\r\n\
</body>\r\n\
</html>"
};

const char ResponseReset[]=
{
"<html>\r\n\
<head>\r\n\
<title>MXCHIP Wi-Fi module</title>\r\n\
</head>\r\n\
<body>\r\n\
<p>Reset system, please wait 5 seconds and refresh</p>\r\n\
</body>\r\n\
</html>"
};

const char headerPage[]=
{
"HTTP/1.1 200 OK\r\n\
Server: MySocket Server\r\n\
Date: TEST\r\n\
Content-Type: text/html\r\n\
Content-Length: %d\r\n\
Connection: close\r\n\
Accept-Ranges: bytes\r\n\r\n"
};

const char basicPage[] =
{
"<html><head><title>Basic Setting</title>\r\n\
</head>\r\n\
<body>\
Basic&nbsp;<a href=\"/advanced.htm\">Advance</a>\r\n\
<a href=\"/system.htm\">System</a>\r\n\
<br />Version:&nbsp;%s&nbsp;<br />MAC:&nbsp;%s&nbsp;\r\n\
<br /><br /><form action=\"basic.htm\" method=\"post\">\r\n\
<table id=\"displayme\" border=\"0\" width=\"500\" cellspacing=\"2\">\r\n\
<col align=\"right\" /> <col align=\"left\" />\r\n\
<tbody><tr><td>SSID:&nbsp;</td>\r\n\
<td><Input type=\"text\" name=\"SSID\" value = \"%s\"/></td></tr>\r\n\
<tr><td>&nbsp;</td> <td> <a href=\"scan.htm\">Find AP</a></td></tr>\r\n\
<tr><td>Key:&nbsp;</td> <td><Input type=\"text\" name=\"pass\" value= \"%s\"/></td></tr>\r\n\
</tbody></table><br />\r\n\
<INPUT type=\"submit\" name=\"CLICK\" value=\"  Save  \"><br />\
<INPUT type=\"submit\" value=\"  Reset  \" name=\"reset\"><br /></body></html>\r\n"
};

const char systemPage[] =
{
"<html><head><title>System Setting</title>\r\n\
</head>\r\n\
<body>\
<a href=\"/basic.htm\">Basic</a>\r\n\
<a href=\"/advanced.htm\">Advance</a>&nbsp;System\r\n\
<br />Version:&nbsp;%s&nbsp;<br /><br />\r\n\
<FORM ENCTYPE=\"multipart/form-data\" action=\"system.htm\" METHOD=POST>\
<label>Update firmware: <input type=\"file\" name=\"imagefile\" accept=\"bin\"></label>\
<input type=\"submit\" value=\"upload\">\
</FORM></body></html>\r\n"
};

const char scanPage_header[] =
{
"<html><head><title>Scan Results</title></head>\r\n\
<body><a href=\"basic.htm\">Return</a>\r\n\
<b><font color=\"green\">Available Wireless Network: </font></b>\r\n\
<table border=\"1\" width=\"320px\"> \
<TBODY><col align=left width=1%%> <col align=left width=30%%>\r\n\
<col align=middle width=29%%>\r\n\
<th align=left><font color=\"blue\">SSID</font></th>\r\n\
<th align=middle><font color=\"blue\">Signal</font></th>\r\n\
</tr>\r\n"
};

const char scanbody[] =
{
"<tr style=\"vertical-align:middle\">\r\n\
&nbsp;&nbsp;<td style=\"width:70%%\"><nobr><a href=\"/basic.htm/ssid=%s\">%s</a></nobr></td><td  style=\"width:30%%\" align=middle>%d%%</td></tr>\r\n"
};

const char scanPage_tail[]=
{
"</TBODY></table><br /> <input onclick=\"window.location.reload()\" type=\"button\" value=\"Refresh\">\r\n\
</html>"
};


char not_found[] =
{
"HTTP/1.1 200 OK\r\n\
Server: MySocket Server\r\n\
Content-Length: 145\r\n\
Connection: close\r\n\
Content-Type: text/html\r\n\r\n\
<html><head><title>404 Not Found</title></head><body>\r\n\
<h1>Not Found</h1>\r\n\
<p>The requested URL was not found on this server.</p>\r\n\
</body></html>"
};

extern u8* http_scan(int *ap);

/*******************************************************************************
* Function Name  :  HTTPParse.
* Description    :  HTTP context Parser.
* Input          :
* Return         :  None.
*******************************************************************************/
// static u8 HTTPParse(char* pStr, char** ppToken1, char** ppToken2)
// {
//  char* pch = strchr(pStr, ' ');
//  *ppToken1 = pStr;
//  if(pch)
//  {
//      *pch='\0';
//      pch++;
//      *ppToken2=pch;
//      return TRUE;
//  }
//  return FALSE;
// }

typedef struct
{
    char *pToken1;  //HTTP request
    char *pToken2;  //URL
    char *pToken3;  //URL function
//  char *pToken4;  //Next
} httpToken_struct;

static u8 HTTPParse(char* pStr, httpToken_struct *httpToken)
{
    char* pch = strchr(pStr, ' ');
    char* pch2 = strchr(pStr, '/');
    char* pch3;
    httpToken->pToken1 = pStr;

    if(pch)
    {
        *pch='\0';
        pch++;
        pch3 = strchr(pch, ' ');
        if(pch2&&pch2<pch3)
        {
            httpToken->pToken2=pch2;
            pch2++;
            pch2 = strchr(pch2, '/');
            if(pch2&&pch2<pch3)
            {
                pch2++;
                httpToken->pToken3=pch2;
            }
            else
                httpToken->pToken3=NULL;
        }
        else
            httpToken->pToken2=NULL;
//      *pch3='\0';
//      *ppToken4 = pch3 +1;
        return TRUE;
    }
    return FALSE;
}

void html_decode(char *p, int len)
{
    int i, j, val;
    char assic[4];

    for (i=0; i<len; i++)
    {
        if (p[i] == '+')
            p[i] = ' ';
        if (p[i] == '%')
        {
            if ((i+2) >= len)
                return;
            assic[2] = 0;
            assic[0] = p[i+1];
            assic[1] = p[i+2];
            val = strtol(assic,NULL,16);
            p[i] = val;
            for (j = i+1; j< len; j++)
                p[j] = p[j+2];
        }
    }
}

/*******************************************************************************
* Function Name  :  PostParse.
* Description    :  Post context Paser.
* Input          :
* Return         :  None.
*******************************************************************************/
static u8 PostParse(char** ppStr, const char* pFlag, char** ppValue)
{
    char* pch=strstr(*ppStr, pFlag);
    char* p;
    char* pch2=NULL;
    if(pch)
    {
        pch2=strchr(pch, '=');
        if(!pch2) return FALSE;
        pch2++;
        *ppValue=pch2;
        if(!*ppValue) return FALSE;
        pch=strchr(pch2, '&');
        if(pch)
        {
            *pch='\0';
            html_decode(pch2, strlen(pch2));
            *ppStr=pch+1;
            return TRUE;
        }
    }
    return FALSE;
}

static void send_http_data(int index, char *data, int len)
{
    int SendCount = 0, NumAtOnce;
    u32 end_time = MS_TIMER+5000; // max wait for 5 seconds.

    while(SendCount < len)
    {
        NumAtOnce = send(httpclientfd[index], data+SendCount, len-SendCount, 0);
        if(0 >= NumAtOnce)
        {
            return;
        }
        if (end_time<MS_TIMER) // timeout
            return;
        SendCount+=NumAtOnce;
        msleep(1);
    }
}

static void send_advace_page(int index, char* pToken3)
{
    int NumOfBytes;
    int remote_server;
    char *key;
    u8 ip_str[16], netmask_str[16], gateway_str[16], dns_str[16];
    int tx_max, tx_min, tx_cur;
    sys_config_t *pconfig = get_running_config();
    base_config_t *pbase = &pconfig->base;
    extra_config_t *pextra = &pconfig->extra;
    net_para_t netpara;
    u8 header_data[200];
	
	  char ssid[33];
		char *pch, *pch2;
	
		
	if(pToken3 == NULL) {
		memcpy(ssid, pbase->wifi_ssid, 32);
		switch(pbase->sec_mode) {
			case SEC_MODE_WPA_PSK:
			case SEC_MODE_AUTO:
			case SEC_MODE_WEP_HEX:
			case SEC_MODE_WPS_PIN:
				key = pbase->wpa_psk;
				break;
			case SEC_MODE_WEP:
				key = pbase->wifi_wepkey;
				break;
			default:
				key = "";
				break;
		}
	} else {
		memset(pbase->wifi_ssid, 0, 33);
		pch = strchr(pToken3, '=')+1;
		pch2 = strchr(pToken3, ' ');
		memcpy(pbase->wifi_ssid, pch, (int)pch2-(int)pch);
	}



#define APPEND_VAL_STR(_name, _val) sprintf(httpRequest, "%s addCfg(\"%s\", \"%s\");\r\n", httpRequest, _name, _val)
#define APPEND_VAL_INT(_name, _val) sprintf(httpRequest, "%s addCfg(\"%s\", \"%d\");\r\n", httpRequest, _name, _val)

    if (pbase->wifi_mode != AP_SERVER_MODE)
        netpara.iface = 0;
    else
        netpara.iface = 1;

    GetNetPara(&netpara);

    switch(pbase->sec_mode)
    {
        case SEC_MODE_WPA_PSK:
        case SEC_MODE_AUTO:
        case SEC_MODE_WEP_HEX:
        case SEC_MODE_WPS_PIN:
            key = pbase->wpa_psk;
            break;
        case SEC_MODE_WEP:
            key = pbase->wifi_wepkey;
            break;
        default:
            key = "";
            break;
    }

    strcpy(ip_str, netpara.ip) ;
    strcpy(gateway_str, netpara.gateway) ;
    strcpy(netmask_str, netpara.netmask) ;
    strcpy(dns_str, netpara.dnsServer) ;
    wlan_get_tx_power(&tx_min, &tx_max, &tx_cur);

    if (pbase->connect_mode == 0) // Server mode
        remote_server = 0;
    else if (pextra->is_remote_dns == 0)
        remote_server = 1; // Client mode, use IP
    else
        remote_server = 2; // Client mode, use DNS

    memset(httpRequest,0,HTTP_DATA_MAX_LEN);

    APPEND_VAL_INT("wifi_mode", pbase->wifi_mode);
    APPEND_VAL_STR("wifi_ssid", pbase->wifi_ssid);
    APPEND_VAL_INT("security_mode", pbase->sec_mode);
    APPEND_VAL_STR("wifi_key", key);

    APPEND_VAL_STR("wifi_ssid1", pextra->new_wpa_conf[0].ssid);
    APPEND_VAL_INT("security_mode1", pextra->new_wpa_conf[0].sec_mode);
    APPEND_VAL_STR("wifi_key1", pextra->new_wpa_conf[0].key);

    APPEND_VAL_STR("wifi_ssid2", pextra->new_wpa_conf[1].ssid);
    APPEND_VAL_INT("security_mode2", pextra->new_wpa_conf[1].sec_mode);
    APPEND_VAL_STR("wifi_key2", pextra->new_wpa_conf[1].key);

    APPEND_VAL_STR("wifi_ssid3", pextra->new_wpa_conf[2].ssid);
    APPEND_VAL_INT("security_mode3", pextra->new_wpa_conf[2].sec_mode);
    APPEND_VAL_STR("wifi_key3", pextra->new_wpa_conf[2].key);

    APPEND_VAL_STR("wifi_ssid4", pextra->new_wpa_conf[3].ssid);
    APPEND_VAL_INT("security_mode4", pextra->new_wpa_conf[3].sec_mode);
    APPEND_VAL_STR("wifi_key4", pextra->new_wpa_conf[3].key);

    APPEND_VAL_STR("uap_ssid", pextra->uap_ssid);
    APPEND_VAL_INT("uap_secmode", pextra->uap_secmode);
    APPEND_VAL_STR("uap_key", pextra->uap_key);

    APPEND_VAL_INT("socket_mode", pbase->connect_mode);
    APPEND_VAL_INT("dhcp_enalbe", pbase->use_dhcp);
    APPEND_VAL_STR("local_ip_addr", ip_str);
    APPEND_VAL_STR("netmask", netmask_str);
    APPEND_VAL_STR("gateway_ip_addr", gateway_str);
    APPEND_VAL_STR("dns_server", dns_str);

    APPEND_VAL_INT("remote_server_mode", pextra->is_remote_dns);
    if (pextra->is_remote_dns==1)
        APPEND_VAL_STR("remote_dns", pextra->remote_dns);
    else
        APPEND_VAL_STR("remote_dns", pbase->remote_ip_addr);

    APPEND_VAL_INT("rport", pbase->portH*256 + pbase->portL);
    APPEND_VAL_INT("lport", pextra->main_lport);
    APPEND_VAL_INT("udp_enalbe", pbase->use_udp);

    APPEND_VAL_INT("estype", pextra->extra_sock_type);
    APPEND_VAL_STR("esaddr", pextra->extra_addr);
    APPEND_VAL_INT("esrport", pextra->extra_port);
    APPEND_VAL_INT("eslport", pextra->extra_lport);

    APPEND_VAL_INT("baudrate", pbase->UART_buadrate);
    APPEND_VAL_INT("parity", pbase->parity);
    APPEND_VAL_INT("data_length", pbase->data_length);
    APPEND_VAL_INT("stop_bits", pbase->stop_bits);
    APPEND_VAL_INT("cts_rts_enalbe", pbase->use_CTS_RTS);
    APPEND_VAL_INT("dma_buffer_size", pbase->DMA_buffersize);
    APPEND_VAL_INT("uart_trans_mode", pextra->dataMode);
    APPEND_VAL_INT("device_num", pbase->device_num);

    APPEND_VAL_INT("ps_enalbe", pextra->wifi_ps_mode);
    APPEND_VAL_INT("ps_unit", pextra->ps_unit);
    APPEND_VAL_INT("ps_utmo", pextra->ps_utmo);
    APPEND_VAL_INT("ps_mtmo", pextra->ps_mtmo);

    APPEND_VAL_INT("tx_power", tx_cur);
    APPEND_VAL_INT("keepalive_num", pextra->tcp_keepalive_num);
    APPEND_VAL_INT("keepalive_time", pextra->tcp_keepalive_time);



    inet_ntoa(ip_str, pextra->socks_conf.addr);
    APPEND_VAL_INT("socks_type", pextra->socks_conf.type);
    APPEND_VAL_STR("socks_addr", ip_str);
    APPEND_VAL_INT("socks_port", pextra->socks_conf.port);
    APPEND_VAL_STR("socks_user", pextra->socks_conf.name);
    APPEND_VAL_STR("socks_pass", pextra->socks_conf.passwd);
    APPEND_VAL_INT("socks_1", pextra->socks_conf.socket_bitmask&1);
    APPEND_VAL_INT("socks_2", pextra->socks_conf.socket_bitmask&2);

    APPEND_VAL_STR("web_user", pextra->web_user);
    APPEND_VAL_STR("web_pass", pextra->web_pass);
    APPEND_VAL_STR("device_name", device_name_get());

    NumOfBytes = strlen(httpRequest);
    memset(header_data,0,sizeof(header_data));
    sprintf(header_data, headerPage, NumOfBytes+strlen(adv_page_hdr)+strlen(adv_page_body));
    send_http_data(index, header_data, strlen(header_data));
    send_http_data(index, (char*)adv_page_hdr, strlen(adv_page_hdr));
    send_http_data(index, httpRequest, strlen(httpRequest));
    send_http_data(index, (char*)adv_page_body, strlen(adv_page_body));

    return;
}

static void send_basic_page(int index, char* pToken3)
{
    sys_config_t *pconfig = get_running_config();
    base_config_t *pbase = &pconfig->base;
    extra_config_t *pextra = &pconfig->extra;
    char *key="";
    char mac[6], mac_str[20], ver[32];
    char ssid[33];
    u8 *body;
    u32 SendCount = 0, NumOfBytes, NumAtOnce;
    char *pch, *pch2;

    memset(ssid, 0, 33);
    if(pToken3 == NULL)
    {
        memcpy(ssid, pbase->wifi_ssid, 32);
        switch(pbase->sec_mode)
        {
            case SEC_MODE_WPA_PSK:
            case SEC_MODE_AUTO:
            case SEC_MODE_WEP_HEX:
            case SEC_MODE_WPS_PIN:
                key = pbase->wpa_psk;
                break;
            case SEC_MODE_WEP:
                key = pbase->wifi_wepkey;
                break;
            default:
                key = "";
                break;
        }
    }
    else
    {
        pch = strchr(pToken3, '=')+1;
        pch2 = strchr(pToken3, ' ');
        memcpy(ssid, pch, (int)pch2-(int)pch);
    }


    wlan_get_mac_address(mac);
    sprintf(mac_str, "%02X-%02X-%02X-%02X-%02X-%02X",
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    system_version(ver, sizeof(ver));
#define FORMAT_POST_STR sprintf(body, basicPage, \
            ssid, key)


    memset(httpRequest,0,HTTP_DATA_MAX_LEN);
    body = httpRequest;
    FORMAT_POST_STR;
    sprintf(httpRequest, headerPage, strlen(body));     // recalute the body length.
    body = httpRequest+strlen(httpRequest);
    FORMAT_POST_STR;
    NumOfBytes = strlen(httpRequest);

    send_http_data(index, httpRequest, NumOfBytes);
}

static void send_system_page(int index, int is_scan_ret)
{
    sys_config_t *pconfig = get_running_config();
    base_config_t *pbase = &pconfig->base;
    extra_config_t *pextra = &pconfig->extra;
    char *key="";
    char mac[6], mac_str[20], ver[32];
    u8 *ssid;
    u8 *body;
    u32 SendCount = 0, NumOfBytes, NumAtOnce;

    if(is_scan_ret == 0)
    {
        ssid = pbase->wifi_ssid;
        switch(pbase->sec_mode)
        {
            case SEC_MODE_WPA_PSK:
            case SEC_MODE_AUTO:
            case SEC_MODE_WEP_HEX:
            case SEC_MODE_WPS_PIN:
                key = pbase->wpa_psk;
                break;
            case SEC_MODE_WEP:
                key = pbase->wifi_wepkey;
                break;
            default:
                key = "";
                break;
        }
    }
    else
    {
        ssid = basic_ssid;
    }


    wlan_get_mac_address(mac);
    sprintf(mac_str, "%02X-%02X-%02X-%02X-%02X-%02X",
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    system_version(ver, sizeof(ver));
#define FORMAT_POST_STR sprintf(body, systemPage, \
            ver, mac_str, ssid, key)

    memset(httpRequest,0,HTTP_DATA_MAX_LEN);
    body = httpRequest;
    FORMAT_POST_STR;
    sprintf(httpRequest, headerPage, strlen(body));     // recalute the body length.
    body = httpRequest+strlen(httpRequest);
    FORMAT_POST_STR;
    NumOfBytes = strlen(httpRequest);

    send_http_data(index, httpRequest, NumOfBytes);
}

static u8 rssi_to_quality(u8 rssi)
{
    if (rssi <= 60)
        return 100;
    else if (rssi <= 70)
        return 80+2*(70-rssi);
    else if (rssi <= 80)
        return 60+2*(80-rssi);
    else if (rssi <= 90)
        return 40+2*(90-rssi);
    else if (rssi <= 100)
        return 20+2*(100-rssi);
    else
        return 10;

}

static int scan_page_size(int ap_num, u8 *results)
{
    int len, i, j, body_len = 0;
    u8 *p, signal;

    len = strlen((char*)scanPage_header) + strlen((char*)scanPage_tail);
    body_len = ap_num*(strlen(scanbody) - 6); // strlen("%s%d")
    p = results;
    for(i=0; i<ap_num; i++)
    {
        j = strlen(p);
        signal = rssi_to_quality(p[j+1]);
        p = p+j+3;
        j = j + j;
        if (signal >= 100)
            j+=3;
        else if (signal > 9)
            j+=2;
        else
            j+=1;
        body_len += j;
    }

    return body_len+len;
}

/* Scan and send results page */
static void send_scan_page(int index)
{
    int i, ap_num, len;
    u8 *scan_result, *p, signal;

    scan_result = http_scan(&ap_num);

    memset(httpRequest,0,HTTP_DATA_MAX_LEN);
    sprintf(httpRequest, headerPage, scan_page_size(ap_num, scan_result));
    strcat(httpRequest, scanPage_header);
    send_http_data(index, httpRequest, strlen(httpRequest));

    p = scan_result;

    for(i=0; i<ap_num; i++)
    {
        len = strlen(p);
        signal = rssi_to_quality(p[len+1]);
        sprintf(httpRequest, scanbody, p, p, signal);
        send_http_data(index, httpRequest, strlen(httpRequest));
        p = p + len + 3;
    }

    sprintf(httpRequest, scanPage_tail);
    send_http_data(index, httpRequest, strlen(httpRequest));
}

static void get_basic_post(int index, u8 *postdata)
{
    char* pToken1,*pToken2, *pValue;
    int bSucc = TRUE;
    sys_config_t *pconfig = get_running_config();
    base_config_t *pbase = &pconfig->base;
    extra_config_t *pextra = &pconfig->extra;

    pToken1  = postdata;

    // Set Wifi Mode to AP client mode or Dual mode.
    if (DUAL_MODE != pbase->wifi_mode)
        pbase->wifi_mode = AP_CLIENT_MODE;
    pbase->use_dhcp = 1;

    //Wifi SSID
    if(!(bSucc = PostParse(&pToken1,"SSID",&pValue)))
        goto Save_Out;
    strcpy(pbase->wifi_ssid, pValue);
    //Wifi sec mode
    pbase->sec_mode = SEC_MODE_AUTO;

    if(!(bSucc = PostParse(&pToken1,"pass",&pValue)))
        goto Save_Out;

    strcpy(pbase->wpa_psk, pValue);

    if(strstr(pToken1, "reset"))
    {
        bSucc = 2;
    }
Save_Out:

    if(bSucc == TRUE)
    {
        sprintf(httpRequest, HTTPSaveResponse,
                strlen(basicResponseSucc), basicResponseSucc);
        save_config();
    }
    else if(bSucc == FALSE)
    {
        sprintf(httpRequest, HTTPSaveResponse,
                strlen(basicResponseError), basicResponseError);

    }
    else if(bSucc == 2)
    {
        sprintf(httpRequest, HTTPSaveResponse,
                strlen(ResponseReset), ResponseReset);
        delay_reload();
    }

    send_http_data(index, httpRequest, strlen(httpRequest));
}

/* Find dst string from src string. return the first place */
char *memmem(char *src, int src_len, const char *dst, int dst_len)
{
    int i, j;

    for (i=0; i<src_len-dst_len; i++)
    {
        if (src[i] == dst[0])
        {
            for (j=1; j<dst_len; j++)
            {
                if (src[i+j] != dst[j])
                    break;
            }
            if (j == dst_len)
                return &src[i];
        }
    }

    return NULL;
}


static void get_system_post(int index, u8 *postdata, int len)
{
    static const char endline[] = {'\r', '\n', '\r', '\n'};
    static const char lengthstr[] = "Content-Length: ";
    static const char boundarystr[] = "boundary=";
    char *boundcp, *boundary, *p;
    char *read_buffer = postdata, *end_pos = NULL, *lengthpos;
    int read_buffer_size = len, time=5000;
    int bytes_received, read_len, content_len = 0, total_read;
    const char *resp;
    u32 addr = NEW_IMAGE_ADDR;


    setsockopt(httpclientfd[index],0, SO_RCVTIMEO, &time, 4);

    /* Get the content length & boundary & begin of content data */
    do
    {
        end_pos = (char*) memmem(read_buffer, read_buffer_size, endline, sizeof(endline));

        if ( ( lengthpos = (char*) memmem( read_buffer,  read_buffer_size, lengthstr, strlen( lengthstr )) ) != NULL )
        {
            content_len = atoi(lengthpos + sizeof( lengthstr)-1);
        }
        if (( boundary = (char*) memmem( read_buffer,  read_buffer_size, boundarystr, strlen(boundarystr) )) != NULL )
        {
            boundary += strlen(boundarystr);
            p = boundary;
            while(*p != 0x0d)
                p++;
            *p++ = 0;
            // now, we have found out the boundary, copy out.
            boundcp = (char*)malloc(strlen(boundary)+1);
            if (boundcp != NULL)
            {
                strcpy(boundcp, boundary);
            }

        }

        if (end_pos == NULL)
        {
            read_buffer = httpRequest;
            bytes_received = recv(httpclientfd[index], httpRequest, HTTP_DATA_MAX_LEN, 0 );
            if ( bytes_received <= 0 )
            {
                break;
            }
            else
            {
                total_read += bytes_received;
                read_buffer_size = bytes_received;
            }
        }

    }
    while ( end_pos == NULL );
    if (boundcp == NULL || content_len == 0)
    {
        resp = systemResponseError;
        goto EXIT;
    }

    end_pos += sizeof(endline);
    read_buffer_size = read_buffer_size - (end_pos-read_buffer);
    content_len -= read_buffer_size;
    read_buffer = end_pos;
    /* Get the begin of file data & write to flash */
    do
    {
        end_pos = (char*)memmem(read_buffer, read_buffer_size, endline, sizeof(endline));

        if (end_pos == NULL)
        {
            read_buffer = httpRequest;
            bytes_received = recv(httpclientfd[index], httpRequest, HTTP_DATA_MAX_LEN, 0 );
            if ( bytes_received <= 0 )
            {
                break;
            }
            else
            {
                content_len -= bytes_received;
                read_buffer_size = bytes_received;
                if (content_len <= 0)
                    break;
            }
        }

    }
    while ( end_pos == NULL );
    if (end_pos == NULL)
    {
        resp = systemResponseError;
        goto EXIT;
    }

    delay_reload(); // whether success or not, need reload system to use bootload erase NEW Image flash.
    flash_init();
    end_pos += sizeof(endline);
    read_buffer_size = read_buffer_size - (end_pos-read_buffer);
    if (read_buffer_size > 0)
    {
        flash_write_data(addr, end_pos, read_buffer_size);
        addr += read_buffer_size;
    }

    content_len -= strlen(boundcp) - 4; // last string is '--'+boudnary+'--'
    /* Recv file and write to flash, if it's last package, find the end of file to write */
    while(content_len > 0)
    {
        if (content_len > HTTP_DATA_MAX_LEN)
            read_len = HTTP_DATA_MAX_LEN;
        else
            read_len = content_len;

        bytes_received = recv(httpclientfd[index], httpRequest, read_len, 0 );
        if ( bytes_received <= 0 )
        {
            break;
        }

        flash_write_data(addr, httpRequest, bytes_received);
        addr += bytes_received;
        content_len -= bytes_received;
    }

    if (content_len == 0)
    {
        boot_table_t bt;

        memset(&bt, 0, sizeof(boot_table_t));
        bt.length = addr - NEW_IMAGE_ADDR;
        bt.start_address = NEW_IMAGE_ADDR;
        bt.type = 'A';
        bt.upgrade_type = 'U';
        save_config();
        FLASH_Lock();
        flash_init();
        flash_write_data(BOOT_TABLE_ADDR, &bt, sizeof(bt));
        FLASH_Lock();
        resp = systemResponseSucc;
    }
    else
        resp = systemResponseError;
EXIT:
    FLASH_Lock();
    sprintf(httpRequest, HTTPSaveResponse,
            strlen(resp), resp);
    send_http_data(index, httpRequest, strlen(httpRequest));
}

static void get_advanced_post(int index, u8 *postdata)
{
    char* pToken1,*pToken2, *pValue;
    int bSucc = TRUE, val1, val2;
    sys_config_t *pconfig = get_running_config();
    base_config_t *pbase = &pconfig->base;
    extra_config_t *pextra = &pconfig->extra;

    pToken1  = postdata;

    //Wifi Mode
    if(!(bSucc = PostParse(&pToken1,"wifi_mode",&pValue)))
        goto Save_Out;
    pbase->wifi_mode = atoi(pValue);
    //Wifi SSID
    if(!(bSucc = PostParse(&pToken1,"wifi_ssid",&pValue)))
        goto Save_Out;
    strcpy(pbase->wifi_ssid, pValue);
    //Wifi sec mode
    if(!(bSucc = PostParse(&pToken1,"security_mode",&pValue)))
        goto Save_Out;
    pbase->sec_mode= atoi(pValue);

    if(!(bSucc = PostParse(&pToken1,"wifi_key",&pValue)))
        goto Save_Out;
    switch(pbase->sec_mode)
    {
        case SEC_MODE_WEP: // WEP
            strcpy(pbase->wifi_wepkey, pValue);
            pbase->wifi_wepkeylen = strlen(pbase->wifi_wepkey);
            break;
        case SEC_MODE_WPA_PSK: // WPA
        case SEC_MODE_AUTO: // auto
        case SEC_MODE_WPS_PIN:
            strcpy(pbase->wpa_psk, pValue);
            break;
        case SEC_MODE_WEP_HEX:
            pbase->wifi_wepkeylen = str2hex(pValue, pbase->wifi_wepkey, 16);
            strcpy(pbase->wpa_psk, pValue);// use wpa_psk to save wep_hex string.
            break;
        case SEC_MODE_WPA_NONE: // None
        default:
            pbase->wifi_wepkeylen = 0;
            memset(pbase->wpa_psk, 0, sizeof(pbase->wpa_psk));
            memset(pbase->wifi_wepkey, 0, sizeof(pbase->wifi_wepkey));
            break;

    }

    //Wifi SSID
    if(!(bSucc = PostParse(&pToken1,"wifi_ssid1",&pValue)))
        goto Save_Out;
    strcpy(pextra->new_wpa_conf[0].ssid, pValue);
    if(!(bSucc = PostParse(&pToken1,"security_mode1",&pValue)))
        goto Save_Out;
    pextra->new_wpa_conf[0].sec_mode= atoi(pValue);

    if(!(bSucc = PostParse(&pToken1,"wifi_key1",&pValue)))
        goto Save_Out;
    strcpy(pextra->new_wpa_conf[0].key, pValue);

    //Wifi SSID
    if(!(bSucc = PostParse(&pToken1,"wifi_ssid2",&pValue)))
        goto Save_Out;
    strcpy(pextra->new_wpa_conf[1].ssid, pValue);
    if(!(bSucc = PostParse(&pToken1,"security_mode2",&pValue)))
        goto Save_Out;
    pextra->new_wpa_conf[1].sec_mode= atoi(pValue);

    if(!(bSucc = PostParse(&pToken1,"wifi_key2",&pValue)))
        goto Save_Out;
    strcpy(pextra->new_wpa_conf[1].key, pValue);

    //Wifi SSID
    if(!(bSucc = PostParse(&pToken1,"wifi_ssid3",&pValue)))
        goto Save_Out;
    strcpy(pextra->new_wpa_conf[2].ssid, pValue);
    if(!(bSucc = PostParse(&pToken1,"security_mode3",&pValue)))
        goto Save_Out;
    pextra->new_wpa_conf[2].sec_mode= atoi(pValue);

    if(!(bSucc = PostParse(&pToken1,"wifi_key3",&pValue)))
        goto Save_Out;
    strcpy(pextra->new_wpa_conf[2].key, pValue);

    //Wifi SSID
    if(!(bSucc = PostParse(&pToken1,"wifi_ssid4",&pValue)))
        goto Save_Out;
    strcpy(pextra->new_wpa_conf[3].ssid, pValue);
    if(!(bSucc = PostParse(&pToken1,"security_mode4",&pValue)))
        goto Save_Out;
    pextra->new_wpa_conf[3].sec_mode= atoi(pValue);

    if(!(bSucc = PostParse(&pToken1,"wifi_key4",&pValue)))
        goto Save_Out;
    strcpy(pextra->new_wpa_conf[3].key, pValue);

    // uap
    if(!(bSucc = PostParse(&pToken1,"uap_ssid",&pValue)))
        goto Save_Out;
    strcpy(pextra->uap_ssid, pValue);
    if(!(bSucc = PostParse(&pToken1,"uap_secmode",&pValue)))
        goto Save_Out;
    pextra->uap_secmode = atoi(pValue);

    if(!(bSucc = PostParse(&pToken1,"uap_key",&pValue)))
        goto Save_Out;
    strcpy(pextra->uap_key, pValue);

    //Socket Mode
    if(!(bSucc = PostParse(&pToken1,"socket_mode",&pValue)))
        goto Save_Out;
    pbase->connect_mode = atoi(pValue);
    //DHCP select
    if(!(bSucc = PostParse(&pToken1,"dhcp_enalbe",&pValue)))
        goto Save_Out;
    pbase->use_dhcp = atoi(pValue);
    //Local IP
    if(!(bSucc = PostParse(&pToken1,"local_ip_addr",&pValue)))
        goto Save_Out;
    strcpy(pbase->local_ip_addr, pValue);
    // netmask
    if(!(bSucc = PostParse(&pToken1,"netmask",&pValue)))
        goto Save_Out;
    strcpy(pbase->net_mask, pValue);
    //Gateway IP
    if(!(bSucc = PostParse(&pToken1,"gateway_ip_addr",&pValue)))
        goto Save_Out;
    strcpy(pbase->gateway_ip_addr, pValue);

    // dns server IP address
    if(!(bSucc = PostParse(&pToken1,"dns_server",&pValue)))
        goto Save_Out;
    strcpy(pextra->dns_server, pValue);

    // remote server mode: 0=use DNS, 1=use IP address
    if(!(bSucc = PostParse(&pToken1,"remote_server_mode",&pValue)))
        goto Save_Out;
    pextra->is_remote_dns = atoi(pValue);
    // remote DNS
    if(!(bSucc = PostParse(&pToken1,"remote_dns",&pValue)))
        goto Save_Out;
    if (pextra->is_remote_dns == 1)
        strcpy(pextra->remote_dns, pValue);
    else
        strcpy(pbase->remote_ip_addr, pValue);

    //Socket Port Number
    if(!(bSucc = PostParse(&pToken1,"rport",&pValue)))
        goto Save_Out;
    pbase->portH = atoi(pValue)/256;
    pbase->portL = atoi(pValue)%256;
    if(!(bSucc = PostParse(&pToken1,"lport",&pValue)))
        goto Save_Out;
    pextra->main_lport = atoi(pValue);

    //TCP/UDP select
    if(!(bSucc = PostParse(&pToken1,"udp_enalbe",&pValue)))
        goto Save_Out;
    pbase->use_udp = atoi(pValue);

    // Extra Socket
    if(!(bSucc = PostParse(&pToken1,"estype",&pValue)))
        goto Save_Out;
    pextra->extra_sock_type = atoi(pValue);
    if(!(bSucc = PostParse(&pToken1,"esaddr",&pValue)))
        goto Save_Out;
    strncpy(pextra->extra_addr, pValue, 64);
    if(!(bSucc = PostParse(&pToken1,"esrport",&pValue)))
        goto Save_Out;
    pextra->extra_port= atoi(pValue);
    if(!(bSucc = PostParse(&pToken1,"eslport",&pValue)))
        goto Save_Out;
    pextra->extra_lport= atoi(pValue);
    //UART BuadRate
    if(!(bSucc = PostParse(&pToken1,"baudrate",&pValue)))
        goto Save_Out;
    pbase->UART_buadrate = atoi(pValue);
    //UART parity
    if(!(bSucc = PostParse(&pToken1,"parity",&pValue)))
        goto Save_Out;
    pbase->parity = atoi(pValue);
    //UART data length
    if(!(bSucc = PostParse(&pToken1,"data_length",&pValue)))
        goto Save_Out;
    pbase->data_length = atoi(pValue);
    //UART stop bits length
    if(!(bSucc = PostParse(&pToken1,"stop_bits",&pValue)))
        goto Save_Out;
    pbase->stop_bits = atoi(pValue);
    //CTS/TRS select
    if(!(bSucc = PostParse(&pToken1,"cts_rts_enalbe",&pValue)))
        goto Save_Out;
    pbase->use_CTS_RTS = atoi(pValue);
    //DMA buffer size
    if(!(bSucc = PostParse(&pToken1,"dma_buffer_size",&pValue)))
        goto Save_Out;
    pbase->DMA_buffersize = atoi(pValue);

    if(!(bSucc = PostParse(&pToken1,"uart_trans_mode",&pValue)))
        goto Save_Out;
    pextra->dataMode = atoi(pValue);

    //Device Number
    if(!(bSucc = PostParse(&pToken1,"device_num",&pValue)))
        goto Save_Out;
    pbase->device_num = atoi(pValue);

    // Power Save
    if(!(bSucc = PostParse(&pToken1,"ps_enalbe",&pValue)))
        goto Save_Out;

    pextra->wifi_ps_mode= atoi(pValue);
    if(!(bSucc = PostParse(&pToken1,"ps_unit",&pValue)))
        goto Save_Out;

    pextra->ps_unit = atoi(pValue);
    if(!(bSucc = PostParse(&pToken1,"ps_utmo",&pValue)))
        goto Save_Out;

    pextra->ps_utmo = atoi(pValue);
    if(!(bSucc = PostParse(&pToken1,"ps_mtmo",&pValue)))
        goto Save_Out;

    pextra->ps_mtmo = atoi(pValue);

    // tx_power
    if(!(bSucc = PostParse(&pToken1,"tx_power",&pValue)))
        goto Save_Out;

    pextra->tx_power = atoi(pValue);
    wlan_set_tx_power(pextra->tx_power);

    // TCP Keep Alive setting
    if(!(bSucc = PostParse(&pToken1,"keepalive_num",&pValue)))
        goto Save_Out;
    pextra->tcp_keepalive_num= atoi(pValue);
    if(!(bSucc = PostParse(&pToken1,"keepalive_time",&pValue)))
        goto Save_Out;
    pextra->tcp_keepalive_time= atoi(pValue);
    set_tcp_keepalive(pextra->tcp_keepalive_num,pextra->tcp_keepalive_time);

    // SOCKS Proxy setting
    if(!(bSucc = PostParse(&pToken1,"socks_type",&pValue)))
        goto Save_Out;
    pextra->socks_conf.type = atoi(pValue);
    if(!(bSucc = PostParse(&pToken1,"socks_addr",&pValue)))
        goto Save_Out;
    pextra->socks_conf.addr = inet_addr(pValue);
    if(!(bSucc = PostParse(&pToken1,"socks_port",&pValue)))
        goto Save_Out;
    pextra->socks_conf.port = atoi(pValue);
    if(!(bSucc = PostParse(&pToken1,"socks_user",&pValue)))
        goto Save_Out;
    strncpy(pextra->socks_conf.name, pValue, 32);
    if(!(bSucc = PostParse(&pToken1,"socks_pass",&pValue)))
        goto Save_Out;
    strncpy(pextra->socks_conf.passwd, pValue, 32);
    if(!(bSucc = PostParse(&pToken1,"socks_1",&pValue)))
        goto Save_Out;
    val1 = atoi(pValue) & 0x1;
    if(!(bSucc = PostParse(&pToken1,"socks_2",&pValue)))
        goto Save_Out;
    val2 = atoi(pValue) & 0x1;
    pextra->socks_conf.socket_bitmask = val1 | val2<<1;

    //Username:Password
    if(!(bSucc = PostParse(&pToken1,"web_user",&pValue)))
        goto Save_Out;
    strncpy(pextra->web_user, pValue, 32);
    if(!(bSucc = PostParse(&pToken1,"web_pass",&pValue)))
        goto Save_Out;
    strncpy(pextra->web_pass, pValue, 32);

    //Device name
    if(!(bSucc = PostParse(&pToken1,"device_name",&pValue)))
        goto Save_Out;
    device_name_set(pValue);
    if(strstr(pToken1, "reset"))
    {
        bSucc = 2;
    }
Save_Out:

    if(bSucc == TRUE)
    {
        sprintf(httpRequest, HTTPSaveResponse,
                strlen(advanceResponseSucc), advanceResponseSucc);
        save_config();

    }
    else if(bSucc == FALSE)
    {
        sprintf(httpRequest, HTTPSaveResponse,
                strlen(advanceResponseError), advanceResponseError);

    }
    else if(bSucc == 2)
    {
        sprintf(httpRequest, HTTPSaveResponse,
                strlen(ResponseReset), ResponseReset);
        delay_reload();
    }

    send_http_data(index, httpRequest, strlen(httpRequest));
}

static void get_scan_post(int index, u8 *postdata)
{
    char* pToken1,*pToken2, *pValue;
    int bSucc = TRUE;

    pToken1  = postdata;

    PostParse(&pToken1,"SSIDSEL",&pValue);

    strncpy(basic_ssid, pValue, 32);
//  send_basic_page(index, 1,1);
}

/*******************************************************************************
* Function Name  :  HandleHttpClient.
* Description    :  handle http request.
* Input          :  ClientSocket:Http Client socket.
* Return         :  None.
*******************************************************************************/
static void HandleHttpClient(int index)
{
    int NumOfBytes;
    httpToken_struct httpToken = {0,0,0};
    char *p_auth;

    msleep(200); // sleep 200 ms, just incase http request fregment.
    NumOfBytes = recv(httpclientfd[index], httpRequest, HTTP_DATA_MAX_LEN, 0);
    if (NumOfBytes < 0)
    {
        return;
    }
    else if (NumOfBytes == 0)
    {
        return;
    }

    httpRequest[NumOfBytes] = '\0';
    if(!HTTPParse(httpRequest,&httpToken))
        goto EXIT;;//http request header error

    p_auth = strstr(httpToken.pToken2, auth_str);
    if (p_auth == NULL)   // un-authrized
    {
        send_http_data(index, (char*)authrized, strlen(authrized));
        goto EXIT;
    }
    else
    {
        p_auth += strlen(auth_str);
        if (*p_auth != 0x0d)
        {
            send_http_data(index, (char*)authrized, strlen(authrized));
            goto EXIT;
        }
    }


    if(!strcmp(httpToken.pToken1, "GET"))
    {
        if(!strncmp(httpToken.pToken2, "/h_000.htm", strlen("/h_000.htm")))
        {
           // send_scan_page(index);
            send_http_data(index, (unsigned char*)h_000_html,sizeof(h_000_html)-1);
        }
        else if(!strncmp(httpToken.pToken2, "/advanced.htm", strlen("/advanced.htm")))
        {
            send_advace_page(index, httpToken.pToken3);
        }
        else if(!strncmp(httpToken.pToken2, "/system.htm", strlen("/system.htm")))
        {
            send_system_page(index, 0);
        }
        else if(!strncmp(httpToken.pToken2, "/basic.htm", strlen("/basic.htm")))
        {
            send_basic_page(index, httpToken.pToken3);
        }
        else if(!strncmp(httpToken.pToken2, "/ ", 2))
        {
//             send_basic_page(index, httpToken.pToken3);
			send_http_data(index, (unsigned char*)h_000_html,sizeof(h_000_html)-1);
        }
		else if(!strncmp(httpToken.pToken2, "/h_001.htm", strlen("/h_001.htm")))
        {
//             send_basic_page(index, httpToken.pToken3);
			send_http_data(index, (unsigned char*)h_001_html,sizeof(h_001_html)-1);
        }
		else if(!strncmp(httpToken.pToken2, "/h_002.htm", strlen("/h_002.htm")))
        {
            send_http_data(index, (unsigned char*)h_002_html,sizeof(h_002_html)-1);
        }
		else if(!strncmp(httpToken.pToken2, "/h_003.htm", strlen("/h_003.htm")))
        {
            send_http_data(index, (unsigned char*)h_003_html,sizeof(h_003_html)-1);
        }
		else if(!strncmp(httpToken.pToken2, "/h_004.htm", strlen("/h_004.htm")))
        {
            send_http_data(index, (unsigned char*)h_004_html,sizeof(h_004_html)-1);
        }
		else if(!strncmp(httpToken.pToken2, "/h_005.htm", strlen("/h_005.htm")))
        {
            send_http_data(index, (unsigned char*)h_005_html,sizeof(h_005_html)-1);
        }
		else if(!strncmp(httpToken.pToken2, "/h_006.htm", strlen("/h_006.htm")))
        {
            send_http_data(index, (unsigned char*)h_006_html,sizeof(h_006_html)-1);
        }
		else if(!strncmp(httpToken.pToken2, "/h_007.htm", strlen("/h_007.htm")))
        {
            send_http_data(index, (unsigned char*)h_007_html,sizeof(h_007_html)-1);
        }
		else if(!strncmp(httpToken.pToken2, "/h_008.htm", strlen("/h_008.htm")))
        {
            send_http_data(index, (unsigned char*)h_008_html,sizeof(h_008_html)-1);
        }
		else if(!strncmp(httpToken.pToken2, "/h_009.htm", strlen("/h_009.htm")))
        {
            send_http_data(index, (unsigned char*)h_009_html,sizeof(h_009_html)-1);
        }
		else if(!strncmp(httpToken.pToken2, "/h_010.htm", strlen("/h_010.htm")))
        {
            send_http_data(index, (unsigned char*)h_010_html,sizeof(h_010_html)-1);
        }
		else if(!strncmp(httpToken.pToken2, "/h_011.htm", strlen("/h_011.htm")))
        {
            send_http_data(index, (unsigned char*)h_011_html,sizeof(h_011_html)-1);
        }
		else if(!strncmp(httpToken.pToken2, "/h_012.htm", strlen("/h_012.htm")))
        {
            send_http_data(index, (unsigned char*)h_012_html,sizeof(h_012_html)-1);
        }
		else if(!strncmp(httpToken.pToken2, "/h_013.htm", strlen("/h_013.htm")))
        {
            send_http_data(index, (unsigned char*)h_013_html,sizeof(h_013_html)-1);
        }
		else if(!strncmp(httpToken.pToken2, "/c_000.css", strlen("/c_000.css")))
        {
            send_http_data(index, (unsigned char*)c_000_css,sizeof(c_000_css)-1);
        }
		else if(!strncmp(httpToken.pToken2, "/i_000.png", strlen("/i_000.png")))
        {
            send_http_data(index, (unsigned char*)i_000_png,sizeof(i_000_png)-1);
        }			
		else if(!strncmp(httpToken.pToken2, "/i_001.png", strlen("/i_001.png")))
        {
            send_http_data(index, (unsigned char*)i_001_png,sizeof(i_001_png)-1);
        }	
		else if(!strncmp(httpToken.pToken2, "/i_002.png", strlen("/i_002.png")))
        {
            send_http_data(index, (unsigned char*)i_002_png,sizeof(i_002_png)-1);
        }	
		else if(!strncmp(httpToken.pToken2, "/i_003.png", strlen("/i_003.png")))
        {
            send_http_data(index, (unsigned char*)i_003_png,sizeof(i_003_png)-1);
        }	
		else if(!strncmp(httpToken.pToken2, "/i_004.png", strlen("/i_004.png")))
        {
            send_http_data(index, (unsigned char*)i_004_png,sizeof(i_004_png)-1);
        }	
		else if(!strncmp(httpToken.pToken2, "/i_005.gif", strlen("/i_005.gif")))
        {
            send_http_data(index, (unsigned char*)i_005_gif,sizeof(i_005_gif)-1);
        }	
    else
        {
            send_http_data(index, not_found, strlen(not_found));
        }
    }
    else if(!strcmp(httpToken.pToken1, "POST"))
    {
        if(!strncmp(httpToken.pToken2, "/h_000.htm", strlen("/h_000.htm")))
        {
            get_scan_post(index, httpToken.pToken2);
        }
        else if(!strncmp(httpToken.pToken2, "/advanced.htm", strlen("/advanced.htm")))
        {
            get_advanced_post(index, httpToken.pToken2);
        }
        else if(!strncmp(httpToken.pToken2, "/basic.htm", strlen("/basic.htm")))
        {
            get_basic_post(index, httpToken.pToken2);
        }
        else if(!strncmp(httpToken.pToken2, "/system.htm", strlen("/system.htm")))
        {
            get_system_post(index, httpToken.pToken2, NumOfBytes - (httpToken.pToken2-httpRequest));
        }

    }

EXIT:

    return;
}


/****************************** my http ***************************************/

int auth_init(char *name, char *passwd)
{
    int len, outlen;
    char *src_str;
    len = strlen(name) + strlen(passwd) + 2;

    if (auth_str)
        free(auth_str);

    src_str = malloc(len);
    if (src_str == 0)
        return -1;

    sprintf(src_str, "%s:%s", name, passwd);
    auth_str = base64_encode(src_str, strlen(src_str), &outlen);
    len = strlen(auth_str);
    auth_str[len-1] = 0;
    free(src_str);
    return 0;
}

void http_init(void)
{
    memset(basic_ssid, 0, sizeof(basic_ssid));
    httpRequest = malloc(HTTP_DATA_MAX_LEN);
    httpfd = 0;
}

void http_tick(void)
{
    int i, j, len;
    fd_set readfds;
    struct timeval_t t;
    struct sockaddr_t addr;
    int optval = 1000;

    FD_ZERO(&readfds);
    t.tv_sec = 0;
    t.tv_usec = 1000;

    if (httpfd==0)
    {
        httpfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

        addr.s_port = 80;
        bind(httpfd, &addr, sizeof(addr));
        listen(httpfd, 0);

        memset(httpclientfd, 0, sizeof(httpclientfd));
    }
    else
    {
        FD_SET(httpfd, &readfds);
        for(i=0; i<MAX_TCP_CLIENT; i++)
        {
            if (httpclientfd[i] != 0)
                FD_SET(httpclientfd[i], &readfds);
        }
    }

    select(1, &readfds, NULL, NULL, &t);

    for(i=0; i<MAX_TCP_CLIENT; i++)
    {
        if (httpclientfd[i] != 0)
        {
            if (FD_ISSET(httpclientfd[i], &readfds))
            {
                HandleHttpClient(i);
                close(httpclientfd[i]);
                httpclientfd[i] = 0;
            }
        }
    }

    if (FD_ISSET(httpfd, &readfds))
    {
        j = accept(httpfd, &addr, &len);
        if (j > 0)
        {
            for(i=0; i<MAX_TCP_CLIENT; i++)
            {
                if (httpclientfd[i] == 0)
                {
                    httpclientfd[i] = j;
                    setsockopt(j, 0, SO_SNDTIMEO, &optval, 4);
                    break;
                }
            }
        }
    }
}

