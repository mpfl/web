
static const char adv_page_hdr[] = {
"<html>\r\n\
<head>\r\n\
<meta http-equiv=\"Content-Language\" content=\"zh-cn\">\r\n\
<meta http-equiv=\"Content-Type\" content=\"text/html; charset=gb2312\">\r\n\
<title>Advance Setting</title>\r\n\
<SCRIPT language=JavaScript>\r\n\
    function Cfg(n,v)\r\n\
    {\r\n\
        this.n=n;\r\n\
        this.v=this.o=v;\r\n\
    }\r\n\
    \r\n\
    var CA = new Array() ;\r\n\
    \r\n\
    function addCfg(n,v)\r\n\
    {\r\n\
        CA.length++;\r\n\
        CA[CA.length-1]= new Cfg(n,v);\r\n\
    }\r\n\
    \r\n\
    function idxOfCfg(kk)\r\n\
    {\r\n\
        for (var i=0; i< CA.length ;i++)\r\n\
        {\r\n\
    \r\n\
            if ( CA[i].n != 'undefined' && CA[i].n==kk )\r\n\
                return i;\r\n\
        }\r\n\
        return -1;\r\n\
    }\r\n\
    \r\n\
    function getCfg(n)\r\n\
    {\r\n\
        var idx=idxOfCfg(n)\r\n\
        if ( idx >=0)\r\n\
            return CA[idx].v ;\r\n\
        else\r\n\
            return \"\";\r\n\
    }\r\n\
    \r\n\
    function setCfg(n,v)\r\n\
    {\r\n\
        var idx=idxOfCfg(n)\r\n\
        if ( idx >=0)\r\n\
        {\r\n\
            CA[idx].v = v ;\r\n\
        }\r\n\
    }\r\n\
    \r\n\
    function cfg2Form(f)\r\n\
    {\r\n\
        for (var i=0;i<CA.length;i++)\r\n\
        {\r\n\
            var e=eval('f.'+CA[i].n);\r\n\
            if ( e )\r\n\
            {\r\n\
                if (e.name=='undefined') continue;\r\n\
                \r\n\
                if (e.type=='checkbox')\r\n\
                    e.checked=e.defaultChecked=Number(CA[i].v);\r\n\
                else if (e.type=='select-one')\r\n\
                {\r\n\
                    for (var j=0;j<e.options.length;j++)\r\n\
                    {    if (e.options[j].value==CA[i].v)\r\n\
                            e.options.selectedIndex = j;\r\n\
                    }\r\n\
                }\r\n\
                else\r\n\
                    e.value=getCfg(e.name);\r\n\
                if (e.defaultValue!='undefined')\r\n\
                    e.defaultValue=e.value;\r\n\
            }\r\n\
        }\r\n\
    }\r\n\
function init(){\r\n\
var f;\r\n"
};

static const char adv_page_body[] = {
"f=document.APP_CONFIG;\r\n\
	cfg2Form(f);\r\n\
}\r\n\
\r\n\
</script>\r\n\
</HEAD>\r\n<body leftmargin=0 topmargin=0 onload=\"init()\">\r\n\
<a href=\"/basic.htm\">Basic</a>&nbsp;Advance\r\n\
<a href=\"/system.htm\">System</a>\r\n\
<form name=APP_CONFIG method=\"POST\" action=\"advanced.htm\">\r\n\
<TABLE BORDER=\"0\" CELLSPACING=\"2\" CELLPADDING=\"1\" align=\"left\"> \r\n\
<TR><TD>WiFi Mode:</TD><TD><select size=\"1\" name=\"wifi_mode\">\r\n\
<option value=1>AP Client</option>\r\n\
<option value=2>AP Server</option>\r\n\
<option value=3>Dual Mode</option></select></TD></TR>\r\n\
<TR><TD>Main AP: </TD><TD>SSID:<input type=\"text\" name=\"wifi_ssid\" size=\"32\" value=\"\">\r\n\
<select size=\"1\" name=\"security_mode\"> \r\n\
<option value=0>WEP</option> \r\n\
<option value=1>WPA</option> \r\n\
<option value=2>NONE</option>\r\n\
<option value=3>WEP-HEX</option>\r\n\
<option value=4>AUTO</option>\r\n\
<option value=5>WPS-PBC</option>\r\n\
<option value=6>WPS-PIN</option></select>\r\n\
Key:<input type=\"text\" size=\"32\" name=\"wifi_key\" value=\"\"></TD></TR>\r\n\
<TR><TD>Extra AP 1: </TD><TD>SSID:<input type=\"text\" name=\"wifi_ssid1\" size=\"32\" value=\"\">\r\n\
<select size=\"1\" name=\"security_mode1\"> \r\n\
<option value=0 >WEP</option> \r\n\
<option value=1 >WPA</option> \r\n\
<option value=2 >NONE</option>\r\n\
<option value=3 >WEP-HEX</option>\r\n\
<option value=4 >AUTO</option></select>\r\n\
Key:<input type=\"text\" size=\"32\" name=\"wifi_key1\" value=\"\"></TD></TR>\r\n\
<TR><TD>Extra AP 2: </TD><TD>SSID:<input type=\"text\" name=\"wifi_ssid2\" size=\"32\" value=\"\">\r\n\
<select size=\"1\" name=\"security_mode2\"> \r\n\
<option value=0 >WEP</option> \r\n\
<option value=1 >WPA</option> \r\n\
<option value=2 >NONE</option>\r\n\
<option value=3 >WEP-HEX</option>\r\n\
<option value=4 >AUTO</option></select>\r\n\
Key:<input type=\"text\" size=\"32\" name=\"wifi_key2\" value=\"\"></TD></TR>\r\n\
<TR><TD>Extra AP 3: </TD><TD>SSID:<input type=\"text\" name=\"wifi_ssid3\" size=\"32\" value=\"\">\r\n\
<select size=\"1\" name=\"security_mode3\"> \r\n\
<option value=0 >WEP</option> \r\n\
<option value=1 >WPA</option> \r\n\
<option value=2 >NONE</option> \r\n\
<option value=3 >WEP-HEX</option>\r\n\
<option value=4 >AUTO</option></select>\r\n\
Key:<input type=\"text\" size=\"32\" name=\"wifi_key3\" value=\"\"></TD></TR>\r\n\
<TR><TD>Extra AP 4: </TD><TD>SSID:<input type=\"text\" name=\"wifi_ssid4\" size=\"32\" value=\"\">\r\n\
<select size=\"1\" name=\"security_mode4\"> \r\n\
<option value=0 >WEP</option> \r\n\
<option value=1 >WPA</option> \r\n\
<option value=2 >NONE</option>\r\n\
<option value=3 >WEP-HEX</option>\r\n\
<option value=4 >AUTO</option></select>\r\n\
Key:<input type=\"text\" size=\"32\" name=\"wifi_key4\" value=\"\"></TD></TR>\r\n\
<TR><TD>AP Server: </TD><TD>SSID:<input type=\"text\" name=\"uap_ssid\" size=\"32\" value=\"\">\r\n\
<select size=\"1\" name=\"uap_secmode\"> \r\n\
<option value=1 >WPA</option> \r\n\
<option value=2 >NONE</option>\r\n\
Key:<input type=\"text\" size=\"32\" name=\"uap_key\" value=\"\"></TD></TR>\r\n\
<TR><TD>Socket Mode:</TD><TD><select size=\"1\" name=\"socket_mode\"> \r\n\
<option value=0 >Server</option> \r\n\
<option value=1 >Client</option> </select></TD></TR>\r\n\
<TR><TD>DHCP Select:</TD><TD><select size=\"1\" name=\"dhcp_enalbe\">\r\n\
<option value=0 >Disable</option>\r\n\
<option value=1 >Enable</option></select></TD></TR>\r\n\
<TR><TD>Local IP:</TD><TD><input type=\"text\" name=\"local_ip_addr\" size=\"16\" value=\"\"></TD></TR>\r\n\
<TR><TD>Netmask:</TD><TD><input type=\"text\" name=\"netmask\" size=\"16\" value=\"\"></TD></TR>\r\n\
<TR><TD>Gateway IP:</TD><TD><input type=\"text\" name=\"gateway_ip_addr\" size=\"16\" value=\"\"></TD></TR>\r\n\
<TR><TD>DNS Server:</TD><TD><input type=\"text\" name=\"dns_server\" size=\"16\" value=\"\"></TD></TR>\r\n\
<TR><TD>Remote Server Mode:</TD><TD><select size=\"1\" name=\"remote_server_mode\">\r\n\
<option value=1 >DNS</option> \r\n\
<option value=0 >IP</option> </select></TD></TR>\r\n\
<TR><TD>Remote Server:</TD><TD><input type=\"text\" name=\"remote_dns\" size=\"32\" value=\"\"></TD></TR>\r\n\
<TR><TD>Remote Port Number:</TD><TD><input type=\"text\" name=\"rport\" size=\"16\" value=\"\"></TD></TR>\r\n\
<TR><TD>Local Port Number:</TD><TD><input type=\"text\" name=\"lport\" size=\"16\" value=\"\"></TD></TR>\r\n\
<TR><TD>TCP/UDP Select:</TD><TD><select size=\"1\" name=\"udp_enalbe\">\r\n\
<option value=0 >TCP</option> \r\n\
<option value=1 >UDP</option> </select></TD></TR>\r\n\
<TR><TD>Extra Socket:</TD><TD><select size=\"1\" name=\"estype\">\r\n\
<option value=0 >TCP Server</option>\r\n\
<option value=1 >TCP Client</option>\r\n\
<option value=2 >UDP Unicast</option>\r\n\
<option value=3 >UDP Broadcast</option>\r\n\
<option value=4 >Disable</option></select></TD></TR>\r\n\
<TR><TD>Extra Remote Address:</TD><TD><input type=\"text\" name=\"esaddr\" size=\"32\" value=\"\"></TD></TR>\r\n\
<TR><TD>Extra Remote Port:</TD><TD><input type=\"text\" name=\"esrport\" size=\"16\" value=\"\"></TD></TR>\r\n\
<TR><TD>Extra Local Port:</TD><TD><input type=\"text\" name=\"eslport\" size=\"16\" value=\"\"></TD></TR>\r\n\
<TR><TD>UART BuadRate:</TD><TD><select size=\"1\" name=\"baudrate\">\r\n\
<option value=12 >1200</option>\r\n\
<option value=11 >2400</option>\r\n\
<option value=10 >4800</option>\r\n\
<option value=0 >9600</option>\r\n\
<option value=1 >19200</option>\r\n\
<option value=2 >38400</option>\r\n\
<option value=3 >57600</option>\r\n\
<option value=4 >115200</option>\r\n\
<option value=5 >230400</option>\r\n\
<option value=6 >460800</option>\r\n\
<option value=7 >921600</option>\r\n\
</select></TD></TR>\r\n\
<TR><TD>UART Parity:</TD><TD><select size=\"1\" name=\"parity\">\r\n\
<option value=0 >none</option>\r\n\
<option value=1 >even parity</option>\r\n\
<option value=2 >odd parity</option></TD></TR>\r\n\
<TR><TD>UART Data Length:</TD><TD><select size=\"1\" name=\"data_length\">\r\n\
<option value=0>8 bits</option>\r\n\
<option value=1>9 bits</option></TD></TR>\r\n\
<TR><TD>UART Stop Bits Length:</TD><TD><select size=\"1\" name=\"stop_bits\">\r\n\
<option value=0>1 bit</option>\r\n\
<option value=1>0.5 bit</option>\r\n\
<option value=2>2 bits</option>\r\n\
<option value=3>1.5 bits</option></TD></TR>\r\n\
<TR><TD>CTS/RTS Select:</TD><TD><select size=\"1\" name=\"cts_rts_enalbe\">\r\n\
<option value=0>Disable</option>\r\n\
<option value=1>Enable</option></select></TD></TR>\r\n\
<TR><TD>DMA Buffer Size:</TD><TD><select size=\"1\" name=\"dma_buffer_size\">\r\n\
<option value=0>2</option>\r\n\
<option value=1>16</option>\r\n\
<option value=2>32</option>\r\n\
<option value=3>64</option>\r\n\
<option value=4>128</option>\r\n\
<option value=5>256</option>\r\n\
<option value=6>512</option></select></TD></TR>\r\n\
<TR><TD>UART Data Tranceiver Mode:</TD><TD><select size=\"1\" name=\"uart_trans_mode\">\r\n\
<option value=0>Data Flow Mode</option>\r\n\
<option value=1>Package Mode</option>\r\n\
<option value=2>Time Stamp Mode: 20ms</option>\r\n\
<option value=3>Time Stamp Mode: 50ms</option>\r\n\
<option value=4>Time Stamp Mode: 100ms</option>\r\n\
<option value=5>Time Stamp Mode: 150ms</option>\r\n\
<option value=6>Time Stamp Mode: 200ms</option>\r\n\
<option value=7>Package Mode2</option></select></TD></TR>\r\n\
<TR><TD>IO1 Function:</TD><TD><select size=\"1\" name=\"device_num\">\r\n\
<option value=0>Not used</option>\r\n\
<option value=1>uart frame control(input)</option>\r\n\
<option value=2>485 TX control(output)</option></select></TD></TR>\r\n\
<TR><TD>WiFi PowerSave Mode:</TD><TD><select size=\"1\" name=\"ps_enalbe\">\r\n\
<option value=0>Disable</option> \r\n\
<option value=1>Enable</option> </select></TD></TR>\r\n\
<TR><TD>WiFi Sleep Unit:</TD><TD><select size=\"1\" name=\"ps_unit\">\r\n\
<option value=0>Millisecond</option> \r\n\
<option value=1>Beacon</option> </select></TD></TR>\r\n\
<TR><TD>Unicast Timeout:</TD><TD><input type=\"text\" name=\"ps_utmo\" size=\"40\" value=\"\"></TD></TR>\r\n\
<TR><TD>Multicast Timeout:</TD><TD><input type=\"text\" name=\"ps_mtmo\" size=\"40\" value=\"\"></TD></TR>\r\n\
<TR><TD>TxPower(10~18):</TD><TD><input type=\"text\" name=\"tx_power\" size=\"40\" value=\"\"></TD></TR>\r\n\
<TR><TD>TCP Keepalive Retry Num:</TD><TD><input type=\"text\" name=\"keepalive_num\" size=\"40\" value=\"\"></TD></TR>\r\n\
<TR><TD>TCP Keepalive Time(second):</TD><TD><input type=\"text\" name=\"keepalive_time\" size=\"40\" value=\"\"></TD></TR>\r\n\
<TR><TD>SOCKS Proxy Type</TD><TD><select size=\"1\" name=\"socks_type\">\r\n\
<option value=0>Disable</option>\r\n\
<option value=4>Version 4</option>\r\n\
<option value=5>Version 5</option></select></TD></TR>\r\n\
<TR><TD>Proxy Server Addr</TD><TD><input type=\"text\" name=\"socks_addr\" size=\"32\" value=\"\"></TD></TR>\r\n\
<TR><TD>Proxy Server Port</TD><TD><input type=\"text\" name=\"socks_port\" size=\"16\" value=\"\"></TD></TR>\r\n\
<TR><TD>Proxy Username</TD><TD><input type=\"text\" name=\"socks_user\" size=\"32\" value=\"\"></TD></TR>\r\n\
<TR><TD>Proxy Passwd</TD><TD><input type=\"text\" name=\"socks_pass\" size=\"32\" value=\"\"></TD></TR>\r\n\
<TR><TD>Main Socket</TD><TD><select size=\"1\" name=\"socks_1\">\r\n\
<option value=0>Don't Use SOCKS Proxy</option>\r\n\
<option value=1>Use SOCKS Proxy</option></select></TD></TR>\r\n\
<TR><TD>Extra Socket</TD><TD><select size=\"1\" name=\"socks_2\">\r\n\
<option value=0>Don't Use SOCKS Proxy</option>\r\n\
<option value=1>Use SOCKS Proxy</option></select></TD></TR>\r\n\
<TR><TD>Web Username:</TD><TD><input type=\"text\" name=\"web_user\" size=\"40\" value=\"\"></TD></TR>\r\n\
<TR><TD>Web Password:</TD><TD><input type=\"text\" name=\"web_pass\" size=\"40\" value=\"\"></TD></TR>\r\n\
<TR><TD>Device Name:</TD><TD><input type=\"text\" name=\"device_name\" size=\"40\" value=\"\"></TD></TR>\r\n\
<TR><TD><input type=\"submit\" value=\"Save\" name=\"save\"> </p>\r\n\
<TR><TD><input type=\"submit\" value=\"Reset\" name=\"reset\"> </p>\r\n\
</TABLE></form>\r\n\
</body>\r\n\
</html>\r\n"
};

