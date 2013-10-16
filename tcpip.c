#include "mxchipWNET.h"
#include "command.h"
#include "tcpip.h"

typedef struct _socket_arg_ {
    int fd;
    u8 type;
    u16 lport;
    u16 rport;
    u32 rip;
    u8 raddr[64];
    u32 last_retry_time;
} socket_arg_t;

static socket_arg_t sock_arg[MAX_SOCKET_NUM];
static int sock_num;
static int tcpclientfd[2][MAX_TCP_CLIENT];
static u8 *p_buf;
static u8 UartIsWorking = 0;
static u8 blinkTimes = 0;

#define SOCKET_RETRY_INTERVAL 1000

int tcpip_init(void)
{
    u8 *buf;
    memset(sock_arg, 0, sizeof(sock_arg));
    sock_num = 0;
    buf = (u8*)malloc(MAX_RX_LEN+10);
    if (buf == NULL)
        while(1);

    p_buf = (u8*)(buf+8);// pbuf should keep 8 bytes to save emsp header.
    memset(tcpclientfd, 0, sizeof(tcpclientfd));
}

void sock_set_lport(u16 lport)
{
    int i;
    socket_arg_t *psock;

    for (i=0; i<sock_num; i++) {
        psock = &sock_arg[i];
        psock->lport = lport;
    }
}

/* return 1 if success, return 0 fail.*/
int sock_add(u8 type, u16 lport, u16 rport, u8 *raddr)
{
    socket_arg_t *psock;

    if (type == SOCK_DISABLE)
        return 0;

    if (sock_num == MAX_SOCKET_NUM)
        return 0;

    memset(&tcpclientfd[sock_num], 0, sizeof(tcpclientfd)/2);
    psock = &sock_arg[sock_num++];
    psock->type = type;
    psock->lport = lport;
    psock->rport = rport;
    psock->fd = 0;
    psock->rip = 0;
    if (raddr != NULL)
        strncpy(psock->raddr, raddr, 64);

    return 1;
}

/* return 1 if success, return 0 fail.*/
int sock_del(u8 type, u16 lport, u16 rport, u8 *raddr)
{
    socket_arg_t *psock;
    int i, j;

    for (i=0; i<sock_num; i++) {
        psock = &sock_arg[i];
        if ((psock->type == type) &&
                (psock->lport == lport) &&
                (psock->rport == rport) &&
                strcmp(psock->raddr, raddr)==0 ) {
            sock_num--;
            while(i<sock_num) {
                memcpy(&sock_arg[i], &sock_arg[i+1], sizeof(socket_arg_t));
                i++;
            }
            return 1;
        }
    }

    return 0;
}

int sock_close_all(void)
{
    int i;
    socket_arg_t *psock;

    for (i=0; i<sock_num; i++) {
        psock = &sock_arg[i];
        if (psock->fd == 0)
            continue;

        close(psock->fd);
        psock->fd = 0;
    }

    memset(tcpclientfd, 0, sizeof(tcpclientfd));
    return 1;
}

int sock_remove(int index)
{
    socket_arg_t *psock;
    if (index == 0)
        return sock_del_all();

    index--;
    if(sock_num<=1)
        return 0;
    psock = &sock_arg[index];
    if (psock->fd != 0)
        close(psock->fd);
    psock->fd = 0;
    memset(&tcpclientfd[index], 0, sizeof(tcpclientfd)/2);
    psock->type = SOCK_DISABLE;
    sock_num--;
    return 1;
}

int sock_del_all(void)
{
    int i;
    socket_arg_t *psock;

    sock_close_all();
    sock_num=0;
    return 1;
}

/* Create socket, reconnect socket */
void sock_tick(void)
{
    int i, j, len;
    struct timeval_t t;
    socket_arg_t *psock;
    struct sockaddr_t addr;
    fd_set fds;
    int fd;
    int opt;
    u32 rip;

    t.tv_sec = 0;
    t.tv_usec = 1000;

    for (i=0; i<sock_num; i++) {
        psock = &sock_arg[i];
        if (psock->fd != 0) {
            if (TCP_CLIENT == psock->type) {
                FD_ZERO(&fds);
                FD_SET(psock->fd, &fds);
                select(1, NULL, NULL, &fds, &t);
                if (FD_ISSET(psock->fd, &fds)) {
                    close(psock->fd);
                    psock->fd = 0;
                }
            } else if (TCP_SERVER == psock->type) {
                FD_ZERO(&fds);
                FD_SET(psock->fd, &fds);
                select(1, &fds, NULL, NULL, &t);
                if (FD_ISSET(psock->fd, &fds)) {
                    int newfd;

                    newfd = accept(psock->fd, &addr, &len);
                    if (newfd > 0) {
                        for(j=0; j<MAX_TCP_CLIENT; j++) {
                            if (tcpclientfd[i][j] == 0) {
                                tcpclientfd[i][j] = newfd;
                                break;
                            }
                        }
                    }
                }

                FD_ZERO(&fds);
                for(j=0; j<MAX_TCP_CLIENT; j++) {
                    if (tcpclientfd[i][j] != 0)
                        FD_SET(tcpclientfd[i][j], &fds);
                }
                select(1, NULL, NULL, &fds, &t);
                for(j=0; j<MAX_TCP_CLIENT; j++) {
                    if (tcpclientfd[i][j] != 0) {
                        if (FD_ISSET(tcpclientfd[i][j], &fds)) {
                            close(tcpclientfd[i][j]);
                            tcpclientfd[i][j] = 0;
                        }
                    }
                }
            }
            continue;
        }

        if (psock->last_retry_time + SOCKET_RETRY_INTERVAL > MS_TIMER) {
            continue;
        }
        psock->last_retry_time = MS_TIMER;
        switch(psock->type) {
        case TCP_SERVER:
            psock->fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

            addr.s_port = psock->lport;
            bind(psock->fd, &addr, sizeof(addr));
            listen(psock->fd, 0);

            break;

        case TCP_CLIENT:
            if (psock->rip == 0) {
                rip = dns_request(psock->raddr);
                if (rip != 0xFFFFFFFF)
                    psock->rip = rip;
            }
            if (psock->rip != 0) {
                fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
                opt = 0;
                setsockopt(fd, 0, SO_BLOCKMODE, &opt, sizeof(opt));
                if (is_socks_needed(i, psock->rip)) {
                    if (socks_tcp_conn(fd, psock->rip,psock->rport) != 0) { // connect failed
                        close(fd);
                    } else
                        psock->fd = fd;
                } else {
                    addr.s_ip = psock->rip;
                    addr.s_port = psock->rport;
                    if (connect(fd, &addr, sizeof(addr)) != 0) { // connect failed
                        close(fd);
                    } else
                        psock->fd = fd;
                }
            }
            break;

        case UDP_UNICAST:
            if (psock->rip == 0) {
                rip = dns_request(psock->raddr);
                if (rip != 0xFFFFFFFF)
                    psock->rip = rip;
            }
            if (psock->rip != 0) {
                fd = socket(AF_INET, SOCK_DGRM, IPPROTO_UDP);
                addr.s_ip = 0;
                addr.s_port = psock->lport;
                bind(fd, &addr, sizeof(addr));
                addr.s_ip = psock->rip;
                addr.s_port = psock->rport;
                if (connect(fd, &addr, sizeof(addr)) != 0) { // connect failed
                    close(fd);
                } else
                    psock->fd = fd;
            }

            break;
        case UDP_BRDCAST:
            fd = socket(AF_INET, SOCK_DGRM, IPPROTO_UDP);
            addr.s_ip = 0;
            addr.s_port = psock->lport;
            bind(fd, &addr, sizeof(addr));
            addr.s_ip = 0xffffffff;
            addr.s_port = psock->rport;
            if (connect(fd, &addr, sizeof(addr)) != 0) { // connect failed
                close(fd);
            } else
                psock->fd = fd;
            break;
        default:
            break;
        }
    }

}

/* receive wifi data and forward to UART */
void sock_fwd_tick(void)
{
    int i, j, len, newfd;
    fd_set readfds;
    struct timeval_t t;
    socket_arg_t *psock;
    struct sockaddr_t addr;

    FD_ZERO(&readfds);
    t.tv_sec = 0;
    t.tv_usec = 1000;
    for (i=0; i<sock_num; i++) {
        psock = &sock_arg[i];
        if (psock->fd == 0)
            continue;

        switch(psock->type) {
        case TCP_SERVER:
            FD_SET(psock->fd, &readfds);
            for(j=0; j<MAX_TCP_CLIENT; j++) {
                if (tcpclientfd[i][j] != 0)
                    FD_SET(tcpclientfd[i][j], &readfds);
            }
            break;

        case TCP_CLIENT:
        case UDP_UNICAST:
        case UDP_BRDCAST:
            FD_SET(psock->fd, &readfds);
            break;
        default:
            break;
        }
    }

    select(1, &readfds, NULL, NULL, &t);

    for (i=0; i<sock_num; i++) {
        psock = &sock_arg[i];
        switch(psock->type) {
        case TCP_SERVER:
            if (FD_ISSET(psock->fd, &readfds)) {
                newfd = accept(psock->fd, &addr, &len);
                if (newfd > 0) {
                    for(j=0; j<MAX_TCP_CLIENT; j++) {
                        if (tcpclientfd[i][j] == 0) {
                            tcpclientfd[i][j] = newfd;
                            break;
                        }
                    }
                }
            }
            for(j=0; j<MAX_TCP_CLIENT; j++) {
                if (tcpclientfd[i][j] != 0) {
                    if (FD_ISSET(tcpclientfd[i][j], &readfds)) {
                        len = recv(tcpclientfd[i][j], p_buf, MAX_RX_LEN, 0);
                        if (len > 0) {
                            ip2uart_out(p_buf, len, i);
                            UartIsWorking = 1;
                        }
                        else {
                            close(tcpclientfd[i][j]);
                            tcpclientfd[i][j] = 0;
                        }
                    }
                }
            }
            break;

        case TCP_CLIENT:
        case UDP_UNICAST:
        case UDP_BRDCAST:
            if (FD_ISSET(psock->fd, &readfds)) {
                len = recv(psock->fd, p_buf, MAX_RX_LEN, 0);
                if (len > 0) {
                    ip2uart_out(p_buf, len, i);
                    UartIsWorking = 1;
                }
                else {
                    close(psock->fd);
                    psock->fd = 0;
                }
            }
            break;

        default:
            break;
        }
    }
}

/* socket_mask used to choose which socket output data.
  * if socket_mask is 0, it means all socket.
  * Bit i is for socket i.
  */
int sock_output(u8 *data, int len, int socket_mask)
{
    int i, j, ret = 0, sent;
    socket_arg_t *psock;
    struct sockaddr_t addr;

    if (socket_mask == 0)
        socket_mask = 0xFF;
    if (len <= 0)
        return 0;

    for (i=0; i<sock_num; i++) {
        psock = &sock_arg[i];
        if (psock->fd == 0)
            continue;
        if (((i+1)&socket_mask) == 0)
            continue;
        switch(psock->type) {
        case TCP_SERVER:
            for(j=0; j<MAX_TCP_CLIENT; j++) {
                if (tcpclientfd[i][j] != 0) {
                    UartIsWorking = 1;
                    sent = send(tcpclientfd[i][j], data, len, 0);
                    if (sent > ret)
                        ret = sent;
                }
            }
            break;

        case TCP_CLIENT:
        case UDP_UNICAST:
            UartIsWorking = 1;
            sent = send(psock->fd, data, len, 0);
            if (sent > ret)
                ret = sent;
            break;
        case UDP_BRDCAST:
            UartIsWorking = 1;
            addr.s_ip = 0xffffffff;
            addr.s_port = psock->rport;
            sent = sendto(psock->fd, data, len, 0,
                          &addr, sizeof(addr));
            if (sent > ret)
                ret = sent;
            break;

        default:
            break;
        }
    }

    return ret;
}


/* receive wifi data */
u8 * sock_recv_command(int *recvlen)
{
    int i, j, newfd, len;
    fd_set readfds;
    struct timeval_t t;
    socket_arg_t *psock;
    struct sockaddr_t addr;

    FD_ZERO(&readfds);
    t.tv_sec = 0;
    t.tv_usec = 1000;
    for (i=0; i<sock_num; i++) {
        psock = &sock_arg[i];
        if (psock->fd == 0)
            continue;

        switch(psock->type) {
        case TCP_SERVER:
            FD_SET(psock->fd, &readfds);
            for(j=0; j<MAX_TCP_CLIENT; j++) {
                if (tcpclientfd[i][j] != 0)
                    FD_SET(tcpclientfd[i][j], &readfds);
            }
            break;

        case TCP_CLIENT:
        case UDP_UNICAST:
        case UDP_BRDCAST:
            FD_SET(psock->fd, &readfds);
            break;
        default:
            break;
        }
    }

    select(1, &readfds, NULL, NULL, &t);

    for (i=0; i<sock_num; i++) {
        psock = &sock_arg[i];
        switch(psock->type) {
        case TCP_SERVER:
            if (FD_ISSET(psock->fd, &readfds)) {
                newfd = accept(psock->fd, &addr, &len);
                if (newfd > 0) {
                    for(j=0; j<MAX_TCP_CLIENT; j++) {
                        if (tcpclientfd[i][j] == 0) {
                            tcpclientfd[i][j] = newfd;
                            break;
                        }
                    }
                }
            }
            for(j=0; j<MAX_TCP_CLIENT; j++) {
                if (tcpclientfd[i][j] != 0) {
                    if (FD_ISSET(tcpclientfd[i][j], &readfds)) {
                        len = recv(tcpclientfd[i][j], p_buf, MAX_RX_LEN, 0);
                        if (len > 0) {
                            *recvlen = len;
                            return p_buf;
                        }
                        else {
                            close(tcpclientfd[i][j]);
                            tcpclientfd[i][j] = 0;
                        }
                    }
                }
            }
            break;

        case TCP_CLIENT:
        case UDP_UNICAST:
        case UDP_BRDCAST:
            if (FD_ISSET(psock->fd, &readfds)) {
                len = recv(psock->fd, p_buf, MAX_RX_LEN, 0);
                if (len > 0) {
                    *recvlen = len;
                    return p_buf;
                }
                else {
                    close(psock->fd);
                    psock->fd = 0;
                }
            }
            break;

        default:
            break;
        }
    }

    *recvlen = 0;
    return NULL;
}

int get_sock_state(int id)
{
    if (id >= sock_num)
        return SOCK_STATE_DISABLE;

    if (sock_arg[id].fd == 0)
        return SOCK_STATE_DISCONNECT;

    if (sock_arg[id].type == TCP_SERVER) {
        int j, childs = 0;
        for(j=0; j<MAX_TCP_CLIENT; j++) {
            if (tcpclientfd[id][j] != 0)
                childs++;
        }

        if (childs > 0)
            return SOCK_STATE_CONNECT;
        else
            return SOCK_STATE_CONNECT_NO_CHILD;
    } else if (socket_conn_state(sock_arg[id].fd) == 1) {
        return SOCK_STATE_CONNECT;
    }

    return SOCK_STATE_DISCONNECT;
}

void red_led_blink_tick(void)
{
    if (MS_TIMER % 100 != 0)
        return;

    uart_recv();

    if (is_wps_state()) {
        red_led_toggle();
        return ;
    }

    if(UartIsWorking == 0)
        return;

    if(blinkTimes < 6)
    {
        red_led_toggle();
        blinkTimes++;
    }
    else
    {
        blinkTimes = 0;
        UartIsWorking = 0;
    }
}


void dns_ip_set(u8 *name, u32 ip)
{
    socket_arg_t *psock;
    int i;

    for (i=0; i<sock_num; i++) {
        psock = &sock_arg[i];
        if (psock->rip == 0) {
            if (strcmp(name, psock->raddr) == 0)
                psock->rip = ip;
        }
    }
}


int wait_sock_response(char *outbuf)
{
    int i, j, len, newfd;
    fd_set readfds;
    struct timeval_t t;
    socket_arg_t *psock;
    struct sockaddr_t addr;

    FD_ZERO(&readfds);
    t.tv_sec = 2;
    t.tv_usec = 0;
    for (i=0; i<sock_num; i++) {
        psock = &sock_arg[i];
        if (psock->fd == 0)
            continue;

        switch(psock->type) {
        case TCP_SERVER:
            FD_SET(psock->fd, &readfds);
            for(j=0; j<MAX_TCP_CLIENT; j++) {
                if (tcpclientfd[i][j] != 0)
                    FD_SET(tcpclientfd[i][j], &readfds);
            }
            break;

        case TCP_CLIENT:
        case UDP_UNICAST:
        case UDP_BRDCAST:
            FD_SET(psock->fd, &readfds);
            break;
        default:
            break;
        }
    }

    select(1, &readfds, NULL, NULL, &t);

    for (i=0; i<sock_num; i++) {
        psock = &sock_arg[i];
        switch(psock->type) {
        case TCP_SERVER:
            if (FD_ISSET(psock->fd, &readfds)) {
                newfd = accept(psock->fd, &addr, &len);
                if (newfd > 0) {
                    for(j=0; j<MAX_TCP_CLIENT; j++) {
                        if (tcpclientfd[i][j] == 0) {
                            tcpclientfd[i][j] = newfd;
                            break;
                        }
                    }
                }
            }
            for(j=0; j<MAX_TCP_CLIENT; j++) {
                if (tcpclientfd[i][j] != 0) {
                    if (FD_ISSET(tcpclientfd[i][j], &readfds)) {
                        len = recv(tcpclientfd[i][j], outbuf, MAX_RX_LEN, 0);
                        if (len > 0) {
                            return len	;
                        }
                        else {
                            close(tcpclientfd[i][j]);
                            tcpclientfd[i][j] = 0;
                        }
                    }
                }
            }
            break;

        case TCP_CLIENT:
        case UDP_UNICAST:
        case UDP_BRDCAST:
            if (FD_ISSET(psock->fd, &readfds)) {
                len = recv(psock->fd, outbuf, MAX_RX_LEN, 0);
                if (len > 0) {
                    return len;
                }
                else {
                    close(psock->fd);
                    psock->fd = 0;
                }
            }
            break;

        default:
            break;
        }
    }

    return 0;
}
