#include "stm32f2xx.h"
#include "mxchipWNET.h"

#include "type.h"
#include "uart.h"

#include "command.h"

#define USART1_DR_Base ((uint32_t)USART1 + 0x04)

extern void set_uart_is_sending(int is_sending);
extern void goto_wps(void);
// head xxxxxx tail
u8 UartRxBuffer[UART_RX_BUF_SIZE];
u8 DMARxBuffer[UART_DMA_MAX_BUF_SIZE];
u8 DMATxBuffer[UART_RX_BUF_SIZE];
int DMARxBufferCount;
int UartRxLen;
int dmatxhead, dmatxtail;
int UartRxTail;
int UartRxHead;
int DmaCurBufSize;
int warning_size = 0; // if rxlen reach warning size, set RTS flag.
static u8 tx_start = 0;
u8 DMABufReadManually;	  // if it equals 1, it means DMA has not produce Half/Complete interrupt in one second, the data in buffer need to be read manually
u8 UartOverflow = 0;
static int use_CTS_RTS = 0;

void uart_tx_tick(void);
void uart_flush_rx_buffer_nolock(void);

void uart_gpio_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

    /* Enable GPIOA and USART1 clock */
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);

  	/* Connect P9 to USART1_Tx
	 * Connect P10 to USART1_Rx
	 * Connect P11 to USART1_CTS
	 * Connect P12 to USART1_RTS 
	 */
  	GPIO_PinAFConfig(GPIOA, GPIO_PinSource9, GPIO_AF_USART1);
  	GPIO_PinAFConfig(GPIOA, GPIO_PinSource10, GPIO_AF_USART1);
  	GPIO_PinAFConfig(GPIOA, GPIO_PinSource11, GPIO_AF_USART1);
  	GPIO_PinAFConfig(GPIOA, GPIO_PinSource12, GPIO_AF_USART1);
 
  	/* Configure USART Tx and Rx as alternate function push-pull */
  	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_12;
  	GPIO_Init(GPIOA, &GPIO_InitStructure);
}
void reset_uart_init(void)
{
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	
    uart_gpio_init();

	USART_InitStructure.USART_BaudRate = 115200;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx;
	USART_Init(USART1, &USART_InitStructure);

	UartRxLen = 0;
	UartRxTail = 0;
	UartRxHead = 0;

	/* Enable USART1 Receive interrupts */
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);

	/* Enable the USARTx */
	USART_Cmd(USART1, ENABLE);

}


void reset_uart_deinit(void)
{
	/* Disable USART1 Receive and Transmit interrupts */
	USART_ITConfig(USART1, USART_IT_RXNE, DISABLE);

	/* Disable the USARTx */
	USART_Cmd(USART1, DISABLE);
}

void reset_uart_handler(void)
{
	if(UartRxTail < UART_RX_BUF_SIZE)
		UartRxBuffer[UartRxTail++] = USART_ReceiveData(USART1);
}

u8 reset_uart_recv(u8 * buf, u8 len)
{
	if( UartRxTail >= len)
	{																										    
		memcpy(buf, UartRxBuffer, len);
		return len;
	}
	else
	{
		return UartRxTail;      
	}
}

void userResetConfig(void)
{
	u8 uartLen;
	u8 uartBuf[2];
				     
	uartLen = reset_uart_recv(uartBuf, 2);

	if(uartLen == 2 && uartBuf[0] == 0x20 && uartBuf[1] == 0x20)// space key
	{
		default_config();
		save_config();
	}
	else if(uartLen == 2 && uartBuf[0] == '1' && uartBuf[1] == '1')// goto configuration mode
	{
		set_config_mode();
	}
	else if(uartLen == 2 && uartBuf[0] == '2' && uartBuf[1] == '2')// goto WPS mode
	{
		SetTimer(5000,goto_wps,0);
	}
	else
	{
		return;
	}
}

void software_RTS_CTS_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);

	/* Configure GPIOA_12(RTS) as Output push-pull ----*/
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
       GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
       GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	/* Configure GPIOA_11(CTS) as Input push-pull ----*/
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;//GPIO_Mode_IN_FLOATING;
	GPIO_InitStructure.GPIO_PuPd =GPIO_PuPd_DOWN;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
}

void set_RTS_status(int onoff)
{
#ifdef SOFT_CTS
	if(onoff)//RTS on
	{
		GPIO_WriteBit(GPIOA, GPIO_Pin_12, Bit_SET);	
	}
	else//RTS off
	{
		GPIO_WriteBit(GPIOA, GPIO_Pin_12, Bit_RESET);	
	}
#endif
}

u8 get_CTS_status(void)
{
	return GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_11);
}

static void DMA_RxConfiguration(u8 *BufferDST, u32 BufferSize)
{
  DMA_InitTypeDef DMA_InitStructure;

  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2,ENABLE);
  DMA_ClearFlag(DMA2_Stream2, DMA_FLAG_HTIF2 | DMA_FLAG_TCIF2);

  /* DMA2 Stream3  or Stream6 disable */
  DMA_Cmd(DMA2_Stream2, DISABLE);

  /* DMA2 Stream3 or Stream6 Config */
  DMA_DeInit(DMA2_Stream2);
#if 0
  DMA_InitStructure.DMA_Channel = DMA_Channel_4;
  DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)USART1_DR_Base;
  DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)BufferDST;
  DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;
  DMA_InitStructure.DMA_BufferSize = BufferSize;
  DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
  DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
  DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
  DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Enable;
  DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_Full;
  DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
  DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
#else
  DMA_InitStructure.DMA_PeripheralBaseAddr = USART1_DR_Base;
  DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
  DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
  DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;
  DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
  DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
  DMA_InitStructure.DMA_Priority = DMA_Priority_High;
  DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;         
  DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_HalfFull;
  DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
  DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
  DMA_InitStructure.DMA_Channel = DMA_Channel_4;
  DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)BufferDST;
  DMA_InitStructure.DMA_BufferSize = (uint16_t)BufferSize;
#endif
  DMA_Init(DMA2_Stream2, &DMA_InitStructure);

 }

static void uart_rx_period(void)
{
	uart_recv();
}

void uart_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	DMA_InitTypeDef DMA_InitStructure;
	sys_config_t *pconfig = get_running_config();
	base_config_t *pbase = &pconfig->base;
	
    uart_gpio_init();
	USART_DeInit(USART1);
	
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//see software_RTS_CTS_init() 
	/* USART1 default configured as follow:
	      - BaudRate = 115200 baud  
	      - Word Length = 8 Bits
	      - One Stop Bit
	      - No parity
	      - Hardware flow control disabled (RTS and CTS signals)
	      - Receive and transmit enabled
	*/
	/* Init RTS/CTS pin */
	if(pbase->use_CTS_RTS == 1)
	{
#ifdef SOFT_CTS
		software_RTS_CTS_init();
		set_RTS_status(0);
#else
		USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_RTS_CTS;
#endif
		use_CTS_RTS = 1;
	}
	switch(pbase->UART_buadrate)
	{
	case BaudRate_9600 :USART_InitStructure.USART_BaudRate = 9600;		break;
	case BaudRate_19200 :USART_InitStructure.USART_BaudRate = 19200;		break;
	case BaudRate_38400 :USART_InitStructure.USART_BaudRate = 38400;		break;
	case BaudRate_57600 :USART_InitStructure.USART_BaudRate = 57600;		break;
	case BaudRate_115200 :USART_InitStructure.USART_BaudRate = 115200;	break;
	case BaudRate_230400 :USART_InitStructure.USART_BaudRate = 230400;	break;
	case BaudRate_460800 :USART_InitStructure.USART_BaudRate = 460800;	break;
	case BaudRate_921600 :USART_InitStructure.USART_BaudRate = 921600;	break;
	case BaudRate_1843200 :USART_InitStructure.USART_BaudRate = 1843200;	break;
	case BaudRate_3686400 :USART_InitStructure.USART_BaudRate = 3686400;	break;
	case BaudRate_4800 :USART_InitStructure.USART_BaudRate = 4800;	break;
	case BaudRate_2400 :USART_InitStructure.USART_BaudRate = 2400;	break;
	case BaudRate_1200 :USART_InitStructure.USART_BaudRate = 1200;	break;

	default:USART_InitStructure.USART_BaudRate = 115200;	break;
	}
	switch(pbase->parity)
	{
	case 0 :
        USART_InitStructure.USART_Parity = USART_Parity_No;  
        USART_InitStructure.USART_WordLength = USART_WordLength_8b;
        break;
	case 1 :
        USART_InitStructure.USART_Parity = USART_Parity_Even;  
        USART_InitStructure.USART_WordLength = USART_WordLength_9b;
        break;
	case 2 :
        USART_InitStructure.USART_Parity = USART_Parity_Odd;  
        USART_InitStructure.USART_WordLength = USART_WordLength_9b;
        break;
	default:
        USART_InitStructure.USART_Parity = USART_Parity_No;  
        USART_InitStructure.USART_WordLength = USART_WordLength_8b;
        break;
	}
#if 0    /*2012-12-11, Data length always 8 bit, set word length based on parity */
	switch(pbase->data_length)
	{
	case 0 :USART_InitStructure.USART_WordLength = USART_WordLength_8b;	break;
	case 1 :USART_InitStructure.USART_WordLength = USART_WordLength_9b;	break;
	default:USART_InitStructure.USART_WordLength = USART_WordLength_8b;	break;
	}
#endif
	switch(pbase->stop_bits)
	{
	case 0 :USART_InitStructure.USART_StopBits = USART_StopBits_1;		break;
	case 1 :USART_InitStructure.USART_StopBits = USART_StopBits_0_5;	break;
	case 2 :USART_InitStructure.USART_StopBits = USART_StopBits_2;		break;
	case 3 :USART_InitStructure.USART_StopBits = USART_StopBits_1_5;	break;
	default:USART_InitStructure.USART_StopBits = USART_StopBits_1;		break;
	}
	
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	/* Configure the USART1 */ 
	USART_Init(USART1, &USART_InitStructure);
	/* Enable USART1 Receive interrupts */
	//USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);

	/* DMA Channel4 (triggered by USART1 Rx event) Config */
	switch(pbase->DMA_buffersize)
	{
	case 0 :
		DmaCurBufSize = 2;	
		break;
	case 1 :
		DmaCurBufSize = 16;		
		break;
	case 2 :
		DmaCurBufSize = 32;		
		break;
	case 3 :
		DmaCurBufSize = 64;		
		break;
	case 4 :
		DmaCurBufSize = 128;	
		break;
	case 5 :
		DmaCurBufSize = 256;	
		break;
	case 6 :
		DmaCurBufSize = 512;	
		break;
	default:
		DmaCurBufSize = 2;		
		break;
	}

 #if 0
	DMA_DeInit(DMA2_Stream2);  
	DMA_InitStructure.DMA_PeripheralBaseAddr = USART1_DR_Base;
	DMA_InitStructure.DMA_Memory0BaseAddr = (u32)DMARxBuffer;
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;
	DMA_InitStructure.DMA_BufferSize = DmaCurBufSize;
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
	DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;
	//DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
	DMA_Init(DMA2_Stream2, &DMA_InitStructure);
#else    
    DMA_RxConfiguration(DMARxBuffer, DmaCurBufSize);
#endif   
	/* Enable USART1 DMA Rx request */
	USART_DMACmd(USART1, USART_DMAReq_Rx, ENABLE);
	/* Enable DMA Channel5 Transfer Half/Complete interrupt */
	DMA_ITConfig(DMA2_Stream2, DMA_IT_HT|DMA_IT_TC, ENABLE);
	/* Enable DMA Channel5 */
	DMA_Cmd(DMA2_Stream2, ENABLE);

	/* Enable the USARTx */
	USART_Cmd(USART1, ENABLE);

	UartRxLen = 0;
	UartRxTail = 0;
	UartRxHead = 0;
	DMARxBufferCount = 0;
	DMABufReadManually = 1;
	dmatxhead = dmatxtail = 0;
	warning_size = UART_RX_BUF_SIZE - DmaCurBufSize;
	tx_start = 0;

}// end uart_init()

/*
void uart_rx_handler(void)
{
	// Read one byte from the receive data register
	UartRxBuffer[UartRxTail++] = USART_ReceiveData(USART1);      
	if (UartRxTail == UART_RX_BUF_SIZE) {
		UartRxTail = 0;
	}
	UartRxLen ++;
	if (UartRxLen > UART_RX_BUF_SIZE) {	// RX buffer overflow, only the last UART_RX_BUF_SIZE bytes is available 
		UartRxLen = UART_RX_BUF_SIZE;
		UartRxHead ++;
		if (UartRxHead == UART_RX_BUF_SIZE) {
			UartRxHead = 0;
		}
	} 
}// end uart_rx_handler()
*/

void uart_recv(void)
{
	u16 CurrentReceived;
	u16 i;

    if (DmaCurBufSize == 2)
        return;
	ENTER_CRITICAL();

	
	if(DMABufReadManually == 0) // DMABufReadManually can be reset at dma_rx_half_handler() or dam_rx_complete_handler()
	{
		DMABufReadManually = 1;
		EXIT_CRITICAL();
		return;
	}

	CurrentReceived = DmaCurBufSize - DMA_GetCurrDataCounter(DMA2_Stream2);
	while (DMARxBufferCount > CurrentReceived) { // should not happened
		if (DMARxBufferCount > DmaCurBufSize/2)
			DMARxBufferCount = DmaCurBufSize/2;
		else
			DMARxBufferCount = 0;
	}
	for(i = DMARxBufferCount; i < CurrentReceived; i++)
	{
		UartRxBuffer[UartRxTail++] = DMARxBuffer[i];
		if (UartRxTail == UART_RX_BUF_SIZE)
			UartRxTail = 0;
	}
	UartRxLen += (CurrentReceived - DMARxBufferCount);
	DMARxBufferCount = CurrentReceived;	
	DMABufReadManually = 1;
	
	EXIT_CRITICAL();
	return;
}

//DMA Transfer Half interrupt handler
void dma_rx_half_handler(void)
{
	u16 i;

	ENTER_CRITICAL();
	DMABufReadManually = 0;
	
	if (DMARxBufferCount > DmaCurBufSize/2)
		DMARxBufferCount = 0;
	// Read UART_DMA_BUF_SIZE/2 bytes from the DMA receive buffer
	for(i=DMARxBufferCount; i<DmaCurBufSize/2; i++)
	{
		UartRxBuffer[UartRxTail++] = DMARxBuffer[i];
		if (UartRxTail == UART_RX_BUF_SIZE)
			UartRxTail = 0;
	}
	UartRxLen += (DmaCurBufSize/2 - DMARxBufferCount);
	DMARxBufferCount = DmaCurBufSize/2;
	
	EXIT_CRITICAL();
}

//DMA Transfer Complete interrupt handler
void dma_rx_complete_handler(void)
{
	u16 i;

	ENTER_CRITICAL();
	DMABufReadManually = 0;
	if(UartRxLen > warning_size)
	{
		if(use_CTS_RTS == 1)
		{
			UartOverflow = 1;
			set_RTS_status(1);
			
			DMA_Cmd(DMA2_Stream2, DISABLE);
		}
		else
		{
			//discard data. move DMARxBufferCount to new position, but do not put data into UartRxBuffer
			DMARxBufferCount = 0;
			EXIT_CRITICAL();
			return;
		}
	}
	
	if (DMARxBufferCount > DmaCurBufSize)
		DMARxBufferCount = 0;
	// Read UART_DMA_BUF_SIZE/2 bytes from the DMA receive buffer
	for(i=DMARxBufferCount; i<DmaCurBufSize; i++)
	{
		UartRxBuffer[UartRxTail++] = DMARxBuffer[i];
		if (UartRxTail == UART_RX_BUF_SIZE)
			UartRxTail = 0;
	}
	UartRxLen += (DmaCurBufSize - DMARxBufferCount);
	DMARxBufferCount = 0;
	

	EXIT_CRITICAL();
}

int uart_rx_data_length(void)
{
	return UartRxLen;
}

int uart_get_rx_buffer(u8* buf, u32 len)
{
	u32 ret = 0;
	int i;

	ENTER_CRITICAL();
	if (len > UartRxLen) {
		goto done;			
	}
	for (i = 0; i < len; i++) {
		buf[i] = UartRxBuffer[UartRxHead++];
		if (UartRxHead == UART_RX_BUF_SIZE) {
			UartRxHead = 0;
		}
		UartRxLen--;
	}
	ret = len;
	if(use_CTS_RTS == 1)
	{
		if(UartOverflow == 1 && (UartRxLen + DmaCurBufSize) < UART_RX_BUF_SIZE)
		{
			UartOverflow = 0;
			DMA_Cmd(DMA2_Stream2, ENABLE); 
			set_RTS_status(0);
		}
	}
done:
	EXIT_CRITICAL();
	return ret;
}// end uart_get_rx_buffer


/* For uart Package Mode, packet format: 7E len xx xx xx xx CE
  * 
  */
int uart_get_one_packet(u8* buf)
{
	u32 ret = 0;
	u8 len = 0;
	int i, j;

	ENTER_CRITICAL();

	if (UartRxLen <= 0) {
		uart_flush_rx_buffer_nolock();
		goto done;
	}

	for (i = 0, j = UartRxHead; i < UartRxLen; i++) {
		if (UartRxBuffer[j] != 0x7E) {
			j++;
			if (j == UART_RX_BUF_SIZE) {
				j = 0;
			}
		} else
			break;
	}
	if (j != UartRxHead) {
		UartRxLen -= i;
		UartRxHead = j;
	}
	if (UartRxLen <= 3)
		goto done;
	
	len = UartRxBuffer[(j+1)%UART_RX_BUF_SIZE];
	len += 2;
	if (len > UartRxLen) {
		len = 0; // don't have enough data.
		goto done;
	}

	if (UartRxBuffer[(j+len-1)%UART_RX_BUF_SIZE] != 0xCE) {
		len = 0; 
		uart_flush_rx_buffer_nolock();
		goto done;
	}
	
	for (i = 0; i < len; i++) {
		buf[i] = UartRxBuffer[j++];

		if (j == UART_RX_BUF_SIZE) {
			j = 0;
		}
	}

	UartRxLen -= len;
	UartRxHead = j;
done:
	EXIT_CRITICAL();
	return len;
}// end uart_get_rx_buffer



/* For uart Package Mode2, packet format: 7E lenH lenL xx xx xx xx CE
  * IP forward remove header: 7E + 2bytes len, and remove tail CE
  */
int uart_get_one_packet2(u8* buf)
{
	int len = 0;
	int i, j;

	ENTER_CRITICAL();

	if (UartRxLen <= 0) {
		uart_flush_rx_buffer_nolock();
		goto done;
	}
	
	for (i = 0, j = UartRxHead; i < UartRxLen; i++) {
		if (UartRxBuffer[j] != 0x7E) {
			j++;
			if (j == UART_RX_BUF_SIZE) {
				j = 0;
			}
		} else
			break;
	}
	if (j != UartRxHead) {
		UartRxLen -= i;
		UartRxHead = j;
	}
	if (UartRxLen <= 4)
		goto done;
	
	len = (UartRxBuffer[(j+1)%UART_RX_BUF_SIZE] << 8) |
		   UartRxBuffer[(j+2)%UART_RX_BUF_SIZE];

	if (len > UART_RX_BUF_SIZE) {
		len = 0; 
		uart_flush_rx_buffer_nolock();
		goto done;
	}
	
	if (len+3 > UartRxLen) {
		len = 0; // don't have enough data.
		goto done;
	}

	j=(j+3)%UART_RX_BUF_SIZE; // move to data field, 7e len1 len2.
	len--; // remove tail CE
	if (UartRxBuffer[(j+len)%UART_RX_BUF_SIZE] != 0xCE) {
		len = 0; 
		uart_flush_rx_buffer_nolock();
		goto done;
	}

	for (i = 0; i < len; i++) {
		buf[i] = UartRxBuffer[j++];

		if (j == UART_RX_BUF_SIZE) {
			j = 0;
		}
	}
	j=(j+1)%UART_RX_BUF_SIZE;
	UartRxLen = UartRxLen - len - 3;
	UartRxHead = j;

done:
	EXIT_CRITICAL();
	return len; // remove header 3bytes and tail 1byte.
}// end uart_get_rx_buffer

void uart_flush_rx_buffer(void)
{
	ENTER_CRITICAL();
  	UartRxLen = 0;
	UartRxTail = 0;
	UartRxHead = 0;
	EXIT_CRITICAL();
}

void uart_flush_rx_buffer_nolock(void)
{
	UartRxLen = 0;
	UartRxTail = 0;
	UartRxHead = 0;
}

void uart_flush_tx_buffer(void)
{
	ENTER_CRITICAL();
	dmatxhead = dmatxtail = 0;
	tx_start = 0;
	EXIT_CRITICAL();
}

void uart_putc(char ch)  
{	 
	while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);	// TX FIFO NOT EMPTY
	USART_SendData(USART1, ch);
}// end uart_putc

int mf_send_data(u8 *buf, u32 len)
{
	int i;
	
	for (i = 0; i < len; i++)
	{
		uart_putc(buf[i]);
	}

	return len;
}

int uart_send_data(u8 *buf, u32 len)
{
	int i=len, timeout = 0;
	u32 endtime = 0;

	endtime = MS_TIMER+1000;
	while (dmatxtail+len > UART_RX_BUF_SIZE) {
		uart_tx_tick();
		tcp_tick(NULL);
	}

	memcpy(&DMATxBuffer[dmatxtail], buf, len);
	dmatxtail+=len;
	
	uart_tx_tick();

	return i;
}// uart_send_data

void mf_uart_init()
{
	USART_InitTypeDef USART_InitStructure;

	uart_gpio_init();

	USART_InitStructure.USART_BaudRate = 115200;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(USART1, &USART_InitStructure);

	UartRxLen = 0;
	UartRxTail = 0;
	UartRxHead = 0;

	/* Enable the USARTx */
	USART_Cmd(USART1, ENABLE);

}

void uart_dma_tx(void)
{
	DMA_InitTypeDef  DMA_InitStructure;

	DMA_InitStructure.DMA_PeripheralBaseAddr = USART1_DR_Base;
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
	DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;
	DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Enable;
	DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_Full;
	DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
	DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
	DMA_DeInit(DMA2_Stream7);
    DMA_InitStructure.DMA_Channel = DMA_Channel_4;
    DMA_InitStructure.DMA_DIR = DMA_DIR_MemoryToPeripheral;  
    
    /****************** USART will Transmit Specific Command ******************/ 
    /* Prepare the DMA to transfer the transaction command (2bytes) from the
       memory to the USART */  
    DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)DMATxBuffer;
    DMA_InitStructure.DMA_BufferSize = (uint16_t)dmatxtail;
    DMA_Init(DMA2_Stream7, &DMA_InitStructure); 
	/* Enable the USART DMA requests */
	USART_DMACmd(USART1, USART_DMAReq_Tx, ENABLE);

	/* Clear the TC bit in the SR register by writing 0 to it */
	USART_ClearFlag(USART1, USART_FLAG_TC);

	/* Enable the DMA TX Stream, USART will start sending the command code (2bytes) */
	DMA_Cmd(DMA2_Stream7, ENABLE);
	tx_start = 1;
	dmatxhead = dmatxtail;
}

void uart_tx_tick(void)
{
	int i, j;

	if (dmatxtail == dmatxhead) {// no new data need send
		if (DMA_GetFlagStatus(DMA2_Stream7, DMA_FLAG_TCIF7) == SET) {// DMA clear
			dmatxtail = dmatxhead = 0;
            set_uart_is_sending(0);
        }
		return;
	}
	
	if((tx_start==1) && 
		(DMA_GetFlagStatus(DMA2_Stream7, DMA_FLAG_TCIF7) == RESET))// DMA is sending data
		return;

    // DMA IDLE, use DMA send data
	tx_start = 0;
    set_uart_is_sending(1);
	if (dmatxhead>0) { // move data to the head
		for (i=dmatxhead, j=0; i<dmatxtail; i++, j++) {
			DMATxBuffer[j] = DMATxBuffer[i];
		}
		dmatxtail = j;
	}

	uart_dma_tx();
    while(USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET)
        ;
    dmatxtail = dmatxhead = 0;
    set_uart_is_sending(0);
}

/* Max wait 100ms */
void wait_uart_dma_clean(void)
{
    u32 end_time = MS_TIMER + 100;
    
    while((DMA_GetFlagStatus(DMA2_Stream7, DMA_FLAG_TCIF7) != SET)&& // DMA clean and UART tx clean
          (USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET)){
        if (end_time < MS_TIMER) {
            break;
        }
    }
    set_uart_is_sending(0);
    msleep(10);
}

