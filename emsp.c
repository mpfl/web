#include "stm32f2xx.h"

#include "mxchipWNET.h"

#include "uart.h"
#include "emsp.h"
#include "command.h"
#include "myhttp.h"


#define UART_MTU 1460
#define IP_MTU 1460

static char *uart_buf = NULL;
static int uart_buffer_time;

u8 moduleStatus = 0, is_emsp = 0;

u32 countrycode;

/*
原config结构的ID用于设置这个功能。
0：正常的UART收发；〉〉〉没有设置过该参数时的默认值，PA15定义为带上拉的输入，但输入电平状态被或略。
1：PA15定义为带上拉的输入，低电平时，模块接收UART数据；高电平时，将接收到的数据通过网络转发。（前几个礼拜做的功能）
2：PA15定义为PP输出，UART接口数据不发送时，PA15脚为低电平；UART接口发送数据时，PA15脚为高电平（本次需求功能）
3:    UART SPC1 mode.
4:    UART SPC2 mode.
*/
static u8 uartmode = 0; // use config.device_num to indicate uart mode. 0: normal, 1: pingpong, 2:

static int tx_test_on = 0;
static int last_mode_status ;

void main_function_tick(void);

void emsp_init(void);
int emsp_process_pkt(int flag);
void emsp_send_cmdpkt(u16 cmdcode, u16 result, void *data, u16 datalen, int flag);
int check_sum(void *data, u32 len);
u16 calc_sum(void *data, u32 len);


/* Pingpong mode, use PA15 to get forward state. Can't use for SPI mode.*/
static void pingpong_gpio_init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
}

/* return 0: uart receive data. return 1: IP forward data*/
static u8 get_pingpong_gpio(void)
{
    return GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_15);
}

void emsp_init(void)
{
    sys_config_t* pconfig;

    uart_buf = malloc(UART_MTU);
    if (uart_buf == NULL)
        while(1);


    pconfig = get_running_config();

    uartmode = pconfig->base.device_num;

    switch(pconfig->extra.dataMode) {
    case UART_DATA_FLOW :
        uart_buffer_time = 0;
        break;
    case UART_TIME_STAMP_20ms :
        uart_buffer_time = 20;
        break;
    case UART_TIME_STAMP_50ms :
        uart_buffer_time = 50;
        break;
    case UART_TIME_STAMP_100ms :
        uart_buffer_time = 100;
        break;
    case UART_TIME_STAMP_150ms :
        uart_buffer_time = 150;
        break;
    case UART_TIME_STAMP_200ms:
        uart_buffer_time = 200;
        break;
    case UART_DATA_SPEC1:
        if (uartmode != 1) {
            uartmode = 3;
            pingpong_gpio_init();
        }
        break;
    case UART_DATA_SPEC2:
        if (uartmode != 1) {
            uartmode = 4;
            pingpong_gpio_init();
        }
        break;
    default :
        uart_buffer_time = 0;
        break;
    }

    if (uartmode == 2)
        uart_dir_init();

    if ( USER_CONFIG_MODE == moduleStatus ) {
        is_emsp = 1;
        set_sys_state(SYS_STATE_CONFIG);
        return;
    }

    module_status_init();
    last_mode_status = get_module_status();
    if (last_mode_status != 0) {
        user_wifi_start();
        user_sock_start();
        is_emsp = 0;
        set_sys_state(SYS_STATE_TRANS);
    } else {
        is_emsp = 1;
        set_sys_state(SYS_STATE_CONFIG);
    }

}

int uart_mode_get(void)
{
    return uartmode;
}

void emsp_tick(void)
{
    int recvlen, datalen;
    int timeout;
    struct emsp_header *hdr = (struct emsp_header *)uart_buf;
    u8 *data = &uart_buf[sizeof(struct emsp_header)];
    int i;

    recvlen = uart_rx_data_length();
    if (recvlen < sizeof(struct emsp_header))
        return;

    memset(uart_buf, 0, EMSP_MAX_PACKET_SIZE);
    // receive header
    recvlen = uart_get_rx_buffer((u8 *)hdr, sizeof(struct emsp_header));
// 	if (!check_sum(hdr, sizeof(struct emsp_header))) {	// check sum error
// 		// flush UART rx buffer
// 		uart_flush_rx_buffer();
// 		return;
// 	}
    if(hdr->size > EMSP_MAX_PACKET_SIZE) {	// haibo add. discard invalid cmd format.
        // flush UART rx buffer
        uart_flush_rx_buffer();
        return;
    }
    // receive data
    datalen = hdr->size - sizeof(struct emsp_header);  	// packet data length
    timeout = MS_TIMER + 2000;	// timeout 2s
    while ((uart_rx_data_length() < datalen) && timeout > MS_TIMER)
        mxchipTick();
    recvlen = uart_get_rx_buffer(data, datalen);
// 	if (!check_sum(data, datalen)) {	// check sum error
// 		// flush UART rx buffer
// 		uart_flush_rx_buffer();
// 		return;
// 	}

    emsp_process_pkt(0);
}

// flag = 0: uart
// flag = 1: spi
int emsp_process_pkt(int flag)
{
    int datalen = 0;
    struct emsp_header *hdr = (struct emsp_header *)uart_buf;
    u8 *data = &uart_buf[sizeof(struct emsp_header)];
    u32 result = hdr->result;
    u32 retlen = 0;
    u8 *retdata;

    datalen = hdr->size - sizeof(struct emsp_header) - sizeof(struct emsp_footer);
    if ((hdr->cmdcode == EMSP_CMD_SCAN_AP) || (hdr->cmdcode == EMSP_CMD_SCAN_CMP) )
        emsp_cmd_do(hdr->cmdcode, datalen, data, &retlen, &result, 0);
    else if(hdr->cmdcode == EMSP_CMD_GET_COUNTRY) {
        countrycode = result;
    }
    else {
        retdata = cmd_do(hdr->cmdcode, datalen, data, &retlen, &result);

        // send response packet
        emsp_send_cmdpkt(hdr->cmdcode, result, retdata, retlen, flag);
    }
    return 0;
}

u32 get_countrycode(void) {
    return countrycode;
}

/* data must have space to save EMSP header */
void emsp_send_cmdpkt(u16 cmdcode, u16 result, void *data, u16 datalen, int flag)
{
    u8 *dt = (u8 *)data;
    struct emsp_header *hdr;
    struct emsp_footer *ft;
    u16 len = sizeof(struct emsp_header) + datalen + sizeof(struct emsp_footer);

    hdr = (struct emsp_header *)(dt - sizeof(struct emsp_header));
    ft = (struct emsp_footer *)(dt + datalen);

    hdr->cmdcode = cmdcode;
    hdr->size = len;
    hdr->result = result;
    hdr->checksum = calc_sum(hdr, sizeof(struct emsp_header) - 2);

    ft->checksum = calc_sum(dt, datalen);

    if (flag == 0) {	// uart
        uart_send_data((u8*)hdr, len);
    } else {	// spi
        //spi_slave_send_data(sendbuf, len);
    }

}

int check_sum(void *data, u32 len)
{
    u16 *sum;
    u8 *p = (u8 *)data;

    p += len - 2;

    sum = (u16 *)p;

    if (calc_sum(data, len - 2) != *sum) {	// check sum error
        return 0;
    }
    return 1;
}

u16 calc_sum(void *data, u32 len)
{
    u32	cksum=0;
    __packed u16 *p=data;

    while (len > 1)
    {
        cksum += *p++;
        len -=2;
    }
    if (len)
    {
        cksum += *(u8 *)p;
    }
    cksum = (cksum >> 16) + (cksum & 0xffff);
    cksum += (cksum >>16);

    return ~cksum;
}


/* receive data from uart forward to socket */
static void uart2ip(void)
{
    static int pend_len=0; //use static var
    static int pend_time=0;
    int recvlen, i, datalen;
    int sentlen;

    recvlen = uart_rx_data_length();
    if(pend_len == 0) // first uart packet, get a buffer to save uard data, then send out.
    {
        pend_time = MS_TIMER;
    }

    if(pend_len + recvlen >= UART_MTU) {
        datalen = UART_MTU - pend_len;
    } else {
        datalen = recvlen;
    }
    recvlen = uart_get_rx_buffer(&uart_buf[pend_len], datalen);
    pend_len += recvlen;

    if((pend_len < UART_MTU) &&
            ((MS_TIMER - pend_time) < uart_buffer_time)) // buffer and forward
    {
        return;
    }
    recvlen = pend_len;
    pend_len = 0; // clean buffer

    if (recvlen == 0) {
        return;
    }

    sentlen = sock_output(uart_buf, recvlen, 0);
    if (sentlen <= 0) {
        pend_len = recvlen;
        return;
    }

    if (sentlen < recvlen) {
        pend_len = recvlen-sentlen; // there are some data didn't send out.
        for(i=0; i<pend_len; i++) {
            uart_buf[i] = uart_buf[sentlen+i]; // move the unsent data at the begin.
        }
    }
}


/* receive data from uart forward to socket */
static void uart2ip_pingpong_mode(void)
{
    static int recvlen = 0, pre_gpio = 0;
    int i;

    if(get_pingpong_gpio() == 0) {
        if (pre_gpio == 1)
            uart_flush_rx_buffer();
        pre_gpio = 0;
        return;
    } else {
        if(pre_gpio == 0) {
            recvlen = uart_rx_data_length();
            if(recvlen > UART_MTU) {
                uart_flush_rx_buffer();
                return;
            }
            pre_gpio = 1;
            recvlen = uart_get_rx_buffer(uart_buf, recvlen);
        }
        else
            return;
    }


    if(recvlen == 0)
        return;

    sock_output(uart_buf, recvlen, 0);
}

/* UART packet format 7e len xxx CE, datalen = len, include CE.
 * socket forward for each packet. */
static void uart2ip_SPC1(void)
{
    int recvlen, i, datalen, sentlen;

    recvlen = uart_get_one_packet(uart_buf);

    if (recvlen == 0)
        return;

    sock_output(uart_buf, recvlen, 0);
}

/* UART packet format 7e len1 len2 xxx CE, datalen=len1<<8+len2, include CE.
 * Socket forward only data part, remove header 3bytes and tail 1byte.*/
static void uart2ip_SPC2(void)
{
    int recvlen, i, datalen, sentlen;

    recvlen = uart_get_one_packet2(uart_buf);

    if (recvlen == 0)
        return;

    sock_output(uart_buf, recvlen, 0);
}

void set_config_mode(void)
{
    moduleStatus =  USER_CONFIG_MODE;
    is_emsp = 1;
}

/* Main function. */
void main_function_tick(void)
{
    int cur_state = get_module_status();

    uart_tx_tick(); // UART DMA TX

    sock_tick(); // socket open, connect, accept ...
    sock_fwd_tick();	// forward socket data to uart
    if ( USER_CONFIG_MODE == moduleStatus ) {
        emsp_tick();
        return;
    }

    if (cur_state != last_mode_status) {
        // work mode changed, flush uart buffer.
        uart_flush_rx_buffer();
        uart_flush_tx_buffer();
        if (last_mode_status == 0) { // configure mode ==> data mode, enable wifi
            user_wifi_start();
            user_sock_start();
            set_sys_state(SYS_STATE_TRANS);
            is_emsp = 0;
        } else {
            set_sys_state(SYS_STATE_CONFIG);
            is_emsp = 1;
        }
    }

    last_mode_status = cur_state;

    if (cur_state == 0 )//configuration mode
    {
        emsp_tick();		// deal uart command
    } else {
        switch(uartmode) {
        case 1:
            uart2ip_pingpong_mode();
            break;
        case 3:
            uart2ip_SPC1();
            break;
        case 4:
            uart2ip_SPC2();
            break;
        default:
            uart2ip();
            break;
        }
    }

}

int ip2uart_out(u8 *data, int len, int socket_num)
{
    if (is_emsp == 1) {
        emsp_send_cmdpkt(EMSP_CMD_RECV_DATA, 1<<socket_num, data, len, 0);
    } else {
        return uart_send_data(data, len);
    }
}


void system_is_bootup(void)
{
    u8 *data = &uart_buf[sizeof(struct emsp_header)];
    int cur_state = get_module_status();

    if (( USER_CONFIG_MODE == moduleStatus ) || (cur_state == 0))
        emsp_send_cmdpkt(EMSP_CMD_SYSTEM_BOOTUP, 1, data, 0, 0);
}

void send_countrycode(int countrycode)
{
    u8 *data = &uart_buf[sizeof(struct emsp_header)];
    int cur_state = get_module_status();

    if (( USER_CONFIG_MODE == moduleStatus ) || (cur_state == 0))
        emsp_send_cmdpkt(EMSP_CMD_SET_COUNTRY, countrycode, data, 0, 0);
}

