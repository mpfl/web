#ifndef __TCPIP_H_
#define __TCPIP_H_

enum {
	TCP_SERVER=0,
	TCP_CLIENT,
	UDP_UNICAST,
	UDP_BRDCAST,

	SOCK_DISABLE,
};


enum {
	SOCK_STATE_DISABLE = 0,
	SOCK_STATE_DISCONNECT,
	SOCK_STATE_CONNECT_NO_CHILD,
	SOCK_STATE_CONNECT,
};

#define MAX_SOCKET_NUM 2
#define MAX_TCP_CLIENT 8
#define MAX_RX_LEN 1024

int tcpip_init(void);
int sock_remove(int index);
int sock_add(u8 type, u16 lport, u16 rport, u8 *raddr);
void sock_tick(void);
int sock_output(u8 *data, int len,int socket_mask);
u8 * sock_recv_command(int *recvlen);

#endif
