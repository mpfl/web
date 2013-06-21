#ifndef __UART_H_
#define __UART_H_

#define UART_RX_BUF_SIZE 2048
#define UART_DMA_MAX_BUF_SIZE 512



extern void	reset_uart_init(void);
extern void reset_uart_deinit(void);
extern void uart_init(void);
extern void uart_recv(void);
extern int uart_rx_data_length(void);
extern int uart_get_rx_buffer(u8* buf, u32 len);
extern void uart_flush_rx_buffer(void);
extern int uart_send_data(u8 *buf, u32 len);
extern void uart_send_is_done(void);

#endif
