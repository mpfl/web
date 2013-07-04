#ifndef __FILE_H__
#define __FILE_H__
#include "stdlib.h"
#include "string.h"
#include "wifimgr.h"

#define AF_INET 2
#define SOCK_STREAM 1
#define SOCK_DGRM 2 
#define IPPROTO_TCP 6
#define IPPROTO_UDP 17
#define SOL_SOCKET   1
#define INADDR_ANY   0
#define INADDR_BROADCAST 0xFFFFFFFF


#define u8 unsigned char
#define u16 unsigned short
#define u32 unsigned int


#ifndef ssize_t
#define ssize_t unsigned int
#endif

#ifndef size_t
#define size_t unsigned int
#endif

struct sockaddr_t {
   u16        s_type;
   u16        s_port;
   u32    	  s_ip;
   u16        s_spares[6];  /* unused in TCP realm */
} ;


struct timeval_t {
	unsigned long		tv_sec;		/* seconds */
	unsigned long		tv_usec;	/* microseconds */
};

#define NULL 0

typedef int socklen_t;

typedef enum {
	SO_REUSEADDR = 2,         /* Socket always support this option */
	SO_BROADCAST = 6,		/* Socket always support this option */
	SO_BLOCKMODE = 0x1000,  /* set socket as block/non-block mode, default is block mode */
 	SO_SNDTIMEO = 0x1005,	/* send timeout */
	SO_RCVTIMEO =0x1006		/* receive timeout */
} SOCK_OPT_VAL;

typedef struct _net_para {
	int iface;
	char mac[12]; // such as string "7E0000001111"
	char ip[16]; // such as string  "192.168.1.1"
	char netmask[16];
	char gateway[16];
	char dnsServer[16];
} net_para_t;

typedef struct _net_set_para {
	u8 iface;
	u8 dhcp;
	u32 ip;
	u32 mask;
	u32 gw;
} net_para_set_t;

enum {
    SCAN_SCURITY_NONE = 0,
    SCAN_SCURITY_WEP,
    SCAN_SCURITY_WPA,
    SCAN_SCURITY_WPA2,
};
 
typedef  struct  _ApList_str  
{  
    char ssid[32];  
    int rssi;  // sorted by rssi
    u8      bssid[6];
    u8      bss_type; // enum wlan_mode
    u8      security; // 0=NONE, 1=WEP, 2=WPA, 3=WPA2
    u8      channel;
}ApList_str; 


typedef  struct  _UwtPara_str  
{  
  char ApNum;       //AP number
  ApList_str * ApList; 
} UwtPara_str;  




typedef struct _uart_str
{
	char baudrate;     //The baud rate, 0:9600, 1:19200, 2:38400, 3:57600, 4:115200, 5:230400, 6:460800, 7:921600 
	char databits;      //0:8, 1:9
	char parity;       //The parity(default NONE)  0:none, 1:even parity, 2:odd parity
	char stopbits;       //The number of stop bits ,  0:1, 1:0.5, 2:2, 3:1.5
} uart_str; 

struct network_InitTypeDef
{
	u8 name[32];
	u8 wifi_ssid[32];
	u8 wifi_key[32];
	int wifi_keytype;
	u8 local_ip_addr[16];
	u8 net_mask[16];
	u8 gateway_ip_addr[16];
	u8 dnsServer_ip_addr[16];
	u8 dhcpMode;		// ADDR_TYPE_STATIC=0; ADDR_TYPE_DHCP=1
	u8 address_pool_start[16]; // dhcp server mode
	u8 address_pool_end[16];   // dhcp server mode
};

struct wifi_InitTypeDef
{
	u8 wifi_mode;		// adhoc mode(1), AP client mode(0), AP mode(2)
	u8 wifi_ssid[32];
	u8 wifi_key[32];
};

/** enum : WPS events */
enum wps_event {
	/** WPS thread started */
	WPS_STARTED = 0,
	/** WPS PBC/PIN Session started */
	WPS_SESSION_STARTED,
	/** WPS PIN checksum failed */
	WPS_SESSION_PIN_CHKSUM_FAILED,
	/** WPS Session aborted */
	WPS_SESSION_ABORTED,
	/** WPS Session registration timeout */
	WPS_SESSION_TIMEOUT,
	/** WPS Session attempt successful */
	WPS_SESSION_SUCCESSFUL,
	/** WPS Session failed */
	WPS_SESSION_FAILED,
	/** WPS thread stopped */
	WPS_FINISHED
};

typedef enum {
	MXCHIP_SUCCESS,
	MXCHIP_FAILED,
	MXCHIP_8782_INIT_FAILED,
	MXCHIP_SYS_ILLEGAL,

	MXCHIP_WIFI_UP,
	MXCHIP_WIFI_DOWN,
	MXCHIP_WIFI_JOIN_FAILED,
	MXCHIP_UAP_ACTIVE,
	MXCHIP_UAP_IDLE,
} 	MxchipStatus;

/* Upgrade iamge should save this table to flash */
typedef struct _boot_table_t {
	u32 start_address; // the address of the bin saved on flash.
	u32 length; // file real length
	u8 version[8];
	u8 type; // B:bootloader, P:boot_table, A:application, D: 8782 driver
	u8 upgrade_type; //u:upgrade, 
	u8 reserved[6];
}boot_table_t;

typedef __packed struct {
    u32     remip;			// Peer's IP address	//fancpp add 2007.3.14 __packed, align byte
	u16     remport;		// Peer's port
	int     len;			// Length of following datagram
	u16		flags;			// Flags as follows:
	u8		iface;			// Interface on which received
	u8		hwa[6];			// Peer's hardware (ethernet) address, if applicable
} udp_datagram_info_t;

enum {
	SLEEP_UNIT_MS = 0,
	SLEEP_UNIT_BEACON = 1,
};

// upgraded image should saved in here
#define NEW_IMAGE_ADDR 0x08060000
#define BOOT_TABLE_ADDR 0x08004000

#define FD_USER_BEGIN   2
#define FD_SETSIZE      1024 // MAX 1024 fd
typedef unsigned long   fd_mask;

#define NBBY    8               /* number of bits in a byte */
#define NFDBITS (sizeof(fd_mask) * NBBY)        /* bits per mask */


#define howmany(x, y)   (((x) + ((y) - 1)) / (y))

typedef struct fd_set {
        fd_mask   fds_bits[howmany(FD_SETSIZE, NFDBITS)];
} fd_set;

#define _fdset_mask(n)   ((fd_mask)1 << ((n) % NFDBITS))
#define FD_SET(n, p)     ((p)->fds_bits[(n)/NFDBITS] |= _fdset_mask(n))
#define FD_CLR(n, p)     ((p)->fds_bits[(n)/NFDBITS] &= ~_fdset_mask(n))
#define FD_ISSET(n, p)  ((p)->fds_bits[(n)/NFDBITS] & _fdset_mask(n))
#define FD_ZERO(p)      memset(p, 0, sizeof(*(p)))

#define MSG_DONTWAIT    0x40    /* Nonblocking io  */

// global veriable
extern int need_macverify;
extern u32 MS_TIMER;


// global function
extern int socket(int domain, int type, int protocol);
extern int setsockopt(int sockfd, int level, int optname,const void *optval, socklen_t optlen);
extern int bind(int sockfd, const struct sockaddr_t *addr, socklen_t addrlen);
extern int connect(int sockfd, const struct sockaddr_t *addr, socklen_t addrlen);
extern int listen(int sockfd, int backlog);
extern int accept(int sockfd, struct sockaddr_t *addr, socklen_t *addrlen);
extern int select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval_t *timeout);
extern ssize_t send(int sockfd, const void *buf, size_t len, int flags);
extern ssize_t  sendto(int  sockfd,  const  void  *buf,  size_t  len,  int  flags,const  struct  sockaddr_t  *dest_addr, 
				socklen_t addrlen);
extern ssize_t recv(int sockfd, void *buf, size_t len, int flags);
extern ssize_t recvfrom(int  sockfd,  void  *buf,  size_t  len,  int  flags,struct  sockaddr_t  *src_addr,  socklen_t 
					*addrlen);
extern int read(int sockfd, void *buf, size_t len); 
extern int write(int sockfd, void *buf, size_t len); 
extern int close(int fd);

/* return the state of the socket. 
  * return value: @-1, can't find this socket.
  *                    @0, disonnect
  *                    @1, connect
  *
  */
extern int socket_conn_state(int fd);
extern void socks_init(int type, u32 ip, u16 port, u8*user, u8*passwd);
/* only support ipv4 tcp client socks */
extern int socks_tcp_conn(int fd, u32 ip, int port);
extern int socks_close(int fd);

extern MxchipStatus mxchipInit(void);
extern MxchipStatus mxchipJoinNetwork(struct network_InitTypeDef* mxconfig);
extern MxchipStatus mxchipReJoinWifi(struct wifi_InitTypeDef* wificonfig);

extern MxchipStatus  GetNetPara(net_para_t * pnetpara);
extern MxchipStatus  SetNetPara(net_para_set_t * pnetpara);

extern MxchipStatus gethostbyname(const u8 * name, u8 * ip_addr, u8 ipLength);
extern u32 resolve(char* name);
extern u32 dns_request(char *hostname);
/* call back function, return the IP address for user request DNS. */
extern void dns_ip_set(u8 *name, u32 ip);

extern void mxchipTick(void);

extern MxchipStatus mxchipStartScan(void);
extern MxchipStatus SetTimer(unsigned long ms, void (*psysTimerHandler)(void), u32 period);

extern void resetWatchDog(void);

extern void Flash_Init(void);
extern void Flash_UnInit(void);
extern MxchipStatus   FlashRW(char mode, int flashadd , int len , char *pbuf);
extern MxchipStatus newimage_write(int offset , int len , char *pbuf) ;
extern MxchipStatus  FlashErase(void);

extern int sleep(int seconds);
extern int msleep(int mseconds);

extern u16 ntohs(u16 n);
extern u16 htons(u16 n);
extern u32 ntohl(u32 n);
extern u32 htonl(u32 n);

/* Convert an ip address string in dotted decimal format to  its binary representation.*/
extern u32 inet_addr(char *s);

/* Convert a binary ip address to its dotted decimal format. 
PARAMETER1 's':  location to place the dotted decimal string.  This must be an array of at least 16 bytes.
PARAMETER2 'x':  ip address to convert.

RETURN VALUE:  returns 's' .
*/
extern char *inet_ntoa( char *s, u32 x );

extern void enable_ps_mode(int unit_type, int unitcast, int multicast);

extern void disable_ps_mode(void);

extern void system_reload(void);

/* This is function is used to caclute the md5 value for the input buffer
  * input: the source buffer;
  * len: the length of the source buffer;
  * output: point the output buffer, should be a 16 bytes buffer
  */
extern void md5_hex(u8 *input, u32 len, u8 *output);

/* This is function is used to caclute the md5 value for the input buffer and format the md5 value as string.
  * input: the source buffer;
  * len: the length of the source buffer;
  * output: point the output buffer, should be a 33 bytes buffer
  */
extern void md5(u8 *input, u32 len, u8 *output);

// user add function
extern void user_tick(void);

/* Control RED LED */
extern void set_conncetion_status(int on);
extern void red_led_toggle();

extern void set_tcp_keepalive(int num, int seconds);
extern void get_tcp_keepalive(int *num, int *seconds);

extern int wps_callback(enum wps_event event, void *data, u16 len);
extern int wps_stop(void);

/* transform src string to hex mode 
  * example: "aabbccddee" => 0xaabbccddee
  * each char in the string must 0~9 a~f A~F, otherwise return 0
  * return the real obuf length
  */
extern unsigned int str2hex(unsigned char *ibuf, unsigned char *obuf, unsigned int olen);

// Enable mDNS.
extern void init_mdns(char *instance_name, char *hostname, char *txt_att, u16 port, int iface);

/* Register UDP handler function. */
int udp_special_handler_register(u16 port, int (*handler)(udp_datagram_info_t * udi, long data, int bytes));

u32 get_rx_rssi(void); // get last rx rssi;

#endif

