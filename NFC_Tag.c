#include "stm32f2xx.h"
#include "mxchipWNET.h"

#include "NFC_Tag.h"

#define DATA_DEVICE_ADDR 0xA6
//#define DATA_DEVICE_ADDR 0xA0
#define CTRL_DEVICE_ADDR 0xAE

enum{
	CTRL,
	DATA
};

u16  sEEAddress = 0;   
u32  sEETimeout = 0x1000*10;   
u16* sEEDataReadPointer;   
u8*  sEEDataWritePointer;  
u8   sEEDataNum;

DMA_InitTypeDef    sEEDMA_InitStructure; 
NVIC_InitTypeDef   NVIC_InitStructure;

EE_Status Wait_For_OPT_Finish(u32 Set_Status);

void sEE_LowLevel_Init(void)
{
  GPIO_InitTypeDef  GPIO_InitStructure; 
   
  /*!< sEE_I2C Periph clock enable */
  RCC_APB1PeriphClockCmd(sEE_I2C_CLK, ENABLE);
  
  /*!< sEE_I2C_SCL_GPIO_CLK and sEE_I2C_SDA_GPIO_CLK Periph clock enable */
  RCC_AHB1PeriphClockCmd(sEE_I2C_SCL_GPIO_CLK | sEE_I2C_SDA_GPIO_CLK, ENABLE);

  RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);
  
  /* Reset sEE_I2C IP */
  RCC_APB1PeriphResetCmd(sEE_I2C_CLK, ENABLE);
  
  /* Release reset signal of sEE_I2C IP */
  RCC_APB1PeriphResetCmd(sEE_I2C_CLK, DISABLE);
    
  /*!< GPIO configuration */
  /* Connect PXx to I2C_SCL*/
  GPIO_PinAFConfig(sEE_I2C_SCL_GPIO_PORT, sEE_I2C_SCL_SOURCE, sEE_I2C_SCL_AF);
  /* Connect PXx to I2C_SDA*/
  GPIO_PinAFConfig(sEE_I2C_SDA_GPIO_PORT, sEE_I2C_SDA_SOURCE, sEE_I2C_SDA_AF);  
  
  /*!< Configure sEE_I2C pins: SCL */   
  GPIO_InitStructure.GPIO_Pin = sEE_I2C_SCL_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
  GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
  GPIO_Init(sEE_I2C_SCL_GPIO_PORT, &GPIO_InitStructure);
  /*!< Configure sEE_I2C pins: SDA */
  GPIO_InitStructure.GPIO_Pin = sEE_I2C_SDA_PIN;
  GPIO_Init(sEE_I2C_SDA_GPIO_PORT, &GPIO_InitStructure);
 
}

/*=============================================================================
brief  Reads a block of data from the EEPROM.
param  pBuffer : pointer to the buffer that receives the data read 
  *   from the EEPROM.
param  ReadAddr : EEPROM's internal address to read from.
param  NumByteToRead : number of bytes to read from the EEPROM.
retval None
=============================================================================*/
EE_Status nfcEE_ReadBuffer(u8 type, u8* pBuffer, u16 ReadAddr, u16 NumByteToRead)
{
	u32 l_Status;
	EE_Status OPT_Status;
	
  if(type==DATA)
    sEEAddress = DATA_DEVICE_ADDR;
  else
    sEEAddress = CTRL_DEVICE_ADDR;
	
	sEETimeout = sEE_LONG_TIMEOUT;

	do { 
		l_Status = I2C_Read_Flag_Status(sEE_I2C);
		if((sEETimeout--) == 0) 
			return ERR_No1;
	} while ((l_Status & I2C_FLAG_BUSY) !=0);				// While the bus is busy

	I2C_GenerateSTART(sEE_I2C, ENABLE);						// Send START condition
	OPT_Status = Wait_For_OPT_Finish(I2C_EVENT_MASTER_MODE_SELECT);		// Test on EV5 and clear it
	if (OPT_Status != 0)
		return ERR_No1;

	I2C_Send7bitAddress(sEE_I2C, sEEAddress, I2C_Direction_Transmitter);		// Send EEPROM address for write
	OPT_Status = Wait_For_OPT_Finish(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED);	// Test on EV6 and clear it
	if (OPT_Status != 0)
		return ERR_No2;

	I2C_SendData(sEE_I2C, (u8)((ReadAddr & 0xFF00) >> 8));			// Send the EEPROM's internal address to read from: MSB of the address first
	OPT_Status = Wait_For_OPT_Finish(I2C_EVENT_MASTER_BYTE_TRANSMITTED);	// Test on EV8 and clear it
	if (OPT_Status != 0)
		return ERR_No3;

	I2C_SendData(sEE_I2C, (u8)(ReadAddr & 0x00FF));					// Send the EEPROM's internal address to read from: LSB of the address

	OPT_Status = Wait_For_OPT_Finish(I2C_EVENT_MASTER_BYTE_TRANSMITTED);	// Test on EV8 and clear it
	if (OPT_Status != 0)
		return ERR_No3;

	I2C_GenerateSTART(sEE_I2C, ENABLE);									// Send STRAT condition a second time
	OPT_Status = Wait_For_OPT_Finish(I2C_EVENT_MASTER_MODE_SELECT);		// Test on EV5 and clear it
	if (OPT_Status != 0)
		return ERR_No4;

	I2C_Send7bitAddress(sEE_I2C, sEEAddress, I2C_Direction_Receiver);	// Send EEPROM internal address for read
	OPT_Status = Wait_For_OPT_Finish(I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED);	// Test on EV6 and clear it
	if (OPT_Status != 0)
		return ERR_No5;

	while(NumByteToRead)							// While there is data to be read
	{
		if(NumByteToRead == 1)
		{
			I2C_AcknowledgeConfig(sEE_I2C, DISABLE);		// Disable Acknowledgement
		}
		if(I2C_CheckEvent(sEE_I2C, I2C_EVENT_MASTER_BYTE_RECEIVED))	// Test on EV7 and clear it
		{
			*pBuffer = I2C_ReceiveData(sEE_I2C);		// Read a byte from the EEPROM
			pBuffer++;								// Point to the next location where the byte read will be saved
			NumByteToRead--;						// Decrement the read bytes counter
		}
	}

	I2C_GenerateSTOP(sEE_I2C, ENABLE);				// Send STOP Condition
	I2C_AcknowledgeConfig(sEE_I2C, ENABLE);			// Enable Acknowledgement to be ready for another reception

	return RIGHT;
}

/*=============================================================================
brief  Writes more than one byte to the EEPROM with a single WRITE cycle.
note   The number of byte can't exceed the EEPROM page size.
param  pBuffer : pointer to the buffer containing the data to be written to the EEPROM.
param  WriteAddr : EEPROM's internal address to write to.
param  NumByteToWrite : number of bytes to write to the EEPROM.
retval None
=============================================================================*/
EE_Status nfcEE_PageWrite(u8 type, u8* pBuffer, u16 WriteAddr, u8 NumByteToWrite)
{
	u32 l_Status;
	EE_Status OPT_Status;
	
	if(type==DATA)
    sEEAddress = DATA_DEVICE_ADDR;
  else
    sEEAddress = CTRL_DEVICE_ADDR;
	
		sEETimeout = sEE_LONG_TIMEOUT;

	do { 
		l_Status = I2C_Read_Flag_Status(sEE_I2C);
		if((sEETimeout--) == 0) 
			return ERR_No1;
	} while ((l_Status & I2C_FLAG_BUSY) !=0);				// While the bus is busy

	I2C_GenerateSTART(sEE_I2C, ENABLE);								// Send START condition
	OPT_Status = Wait_For_OPT_Finish(I2C_EVENT_MASTER_MODE_SELECT);	// Test on EV5 and clear it
	if (OPT_Status != 0)
		return ERR_No1;

	I2C_Send7bitAddress(sEE_I2C, sEEAddress, I2C_Direction_Transmitter);			// Send EEPROM address for write
	OPT_Status = Wait_For_OPT_Finish(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED);	// Test on EV6 and clear it
	if (OPT_Status != 0)
		return ERR_No2;

	I2C_SendData(sEE_I2C, (u8)((WriteAddr & 0xFF00) >> 8));		// Send the EEPROM's internal address to write to : MSB of the address first
	OPT_Status = Wait_For_OPT_Finish(I2C_EVENT_MASTER_BYTE_TRANSMITTED);// Test on EV8 and clear it
	if (OPT_Status != 0)
		return ERR_No3;

	I2C_SendData(sEE_I2C, (u8)(WriteAddr & 0x00FF));			// Send the EEPROM's internal address to write to : LSB of the address
	OPT_Status = Wait_For_OPT_Finish(I2C_EVENT_MASTER_BYTE_TRANSMITTED);// Test on EV8 and clear it
	if (OPT_Status != 0)
		return ERR_No3;

	while(NumByteToWrite--)					// While there is data to be written
	{
		I2C_SendData(sEE_I2C, *pBuffer);		// Send the current byte
		pBuffer++;							// Point to the next byte to be written
		OPT_Status = Wait_For_OPT_Finish(I2C_EVENT_MASTER_BYTE_TRANSMITTED);// Test on EV8 and clear it
		if (OPT_Status != 0)
			return ERR_No4;
	}
	I2C_GenerateSTOP(sEE_I2C, ENABLE);		// Send STOP condition

	return RIGHT;
}

void NFC_TAG_INIT(void)
{ 
  I2C_InitTypeDef  I2C_InitStructure;
	u8 data= 0;  
  sEE_LowLevel_Init();

  
  /*!< I2C configuration */
  /* sEE_I2C configuration */
  I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
  I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
  I2C_InitStructure.I2C_OwnAddress1 = 0xA0;
  I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
  I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
  I2C_InitStructure.I2C_ClockSpeed = I2C_SPEED;
  
  /* sEE_I2C Peripheral Enable */
  I2C_Cmd(sEE_I2C, ENABLE);
  /* Apply sEE_I2C configuration after enabling it */
  I2C_Init(sEE_I2C, &I2C_InitStructure);
  
	/* Config int pin triggle while RF writing */
	if(nfcEE_ReadBuffer(CTRL, &data, 0x910, 1))
		return;
	if((data&0x8)==0){
	  data |= 0x8;
	  nfcEE_PageWrite(CTRL, &data, 0x910, 1); 
  }
}

EE_Status NFC_GetSsidPassword(u8* ssid, u8* pass)
{
  u16 byte;
  int i,j=0;
	volatile char *cha;
  EE_Status OPT_Status;
	I2C_InitTypeDef  I2C_InitStructure;
	
  sEE_LowLevel_Init();
  /*!< I2C configuration */
  /* sEE_I2C configuration */
  I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
  I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
  I2C_InitStructure.I2C_OwnAddress1 = 0xA0;
  I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
  I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
  I2C_InitStructure.I2C_ClockSpeed = I2C_SPEED;
  
  /* sEE_I2C Peripheral Enable */
  I2C_Cmd(sEE_I2C, ENABLE);
  /* Apply sEE_I2C configuration after enabling it */
  I2C_Init(sEE_I2C, &I2C_InitStructure);
	
	OPT_Status = nfcEE_ReadBuffer(DATA, ssid, 0x20*4, 32);
	if(OPT_Status!=RIGHT)
		return OPT_Status;
	OPT_Status = nfcEE_ReadBuffer(DATA, pass, 0x30*4, 32);
	return OPT_Status;


}

/*=============================================================================
brief  check the I2C status for operation.
param  Set_Status : the status whcih would be checked in I2C register
retval 
  * - SUCCESS: Last event is equal to the I2C_EVENT
  * - ERROR: Last event is different from the I2C_EVENT
=============================================================================*/
EE_Status Wait_For_OPT_Finish(u32 Set_Status)
{
	volatile u32 I2C_Status;
	unsigned int Time1;

	for (Time1 = 2550; Time1; Time1--) {
		if (Time1 < 2000)	
		{
			I2C_Status = I2C_Read_Flag_Status(sEE_I2C);
			if (I2C_Status == Set_Status)
				return RIGHT;
		}
	}
	return ERR_No1;
}