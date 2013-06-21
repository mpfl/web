/******************** (C) COPYRIGHT 2009 STMicroelectronics ********************
* File Name          : main.c
* Author             : MCD Application Team
* Version            : V3.1.0
* Date               : 10/30/2009
* Description        : Device Firmware Upgrade(DFU) demo main file
********************************************************************************
* THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
* AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
* CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "chip_id.h"
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
#define ILLEGAL_IMAGE_WARNING "Illegal Image! Please contact MXCHIP for details!"
/* Private variables ---------------------------------------------------------*/

/* Extern variables ----------------------------------------------------------*/
u32 Unique_ID_Mem_Raw [3];
u32 Unique_ID_Mem_encrypt [3];
extern vu32 MS_TIMER;
#ifdef BOOT_CHIPID
u32 Unique_ID_Flash [3] __attribute__((at(UNIQUE_CODE_ADDR))) = {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF};
#endif
/* Private function prototypes -----------------------------------------------*/
void 	Get_Unique_Device_ID(void);
void 	Generate_Encrypt_Code(void);
s32 Is_Fresh_DFU_Code(void);
s32 Program_Encrypt_Code(void);
s32 Is_Unique_Device_Code(void);
void Generate_UnEncrypt_Code(u32 encrypt_code[]);

#define TRUE 1
#define FALSE 0

/* Private functions ---------------------------------------------------------*/

void Verify_Program_Code(void)
{
  if(Is_Fresh_DFU_Code())
  {
	Get_Unique_Device_ID();
	Generate_Encrypt_Code();
	Program_Encrypt_Code();
  }
  else if(!Is_Unique_Device_Code())
  {
	while (1);
  }
}

void APP_Verify_Program_Code(void)
{
  if(Is_Fresh_DFU_Code()||!Is_Unique_Device_Code())
  {
	u8 on = TRUE;
  	mf_send_data(ILLEGAL_IMAGE_WARNING, strlen(ILLEGAL_IMAGE_WARNING));
	while (1) {
		
		if(MS_TIMER % 100 == 0)
		{
			led(on);
			on = !on ;
		}
	}
  }
}

void Get_Unique_Device_ID(void)
{	
	Unique_ID_Mem_Raw [0] = *(vu32*)(UNIQUE_ID_ADDR_0);
	Unique_ID_Mem_Raw [1] = *(vu32*)(UNIQUE_ID_ADDR_1);
	Unique_ID_Mem_Raw [2]= *(vu32*)(UNIQUE_ID_ADDR_2);	
}

void Generate_Encrypt_Code(void)
{
	Unique_ID_Mem_encrypt[0] = Unique_ID_Mem_Raw[0] ^ ENCRYPT_KEY;
	Unique_ID_Mem_encrypt[1] = Unique_ID_Mem_Raw[1] ^ ENCRYPT_KEY;
	Unique_ID_Mem_encrypt[2] = Unique_ID_Mem_Raw[2] ^ ENCRYPT_KEY;
}

void Generate_UnEncrypt_Code(u32 encrypt_code[])
{
	encrypt_code[0] = encrypt_code[0] ^ ENCRYPT_KEY;
	encrypt_code[1] = encrypt_code[1] ^ ENCRYPT_KEY;
	encrypt_code[2] = encrypt_code[2] ^ ENCRYPT_KEY;
}

s32 Program_Encrypt_Code(void)
{
	s32 status;
	FLASH_ProgramWord(UNIQUE_CODE_ADDR, Unique_ID_Mem_encrypt [0]);
	FLASH_ProgramWord(UNIQUE_CODE_ADDR + 4, Unique_ID_Mem_encrypt [1]);
	status = FLASH_ProgramWord(UNIQUE_CODE_ADDR + 8, Unique_ID_Mem_encrypt [2]);

	return 	status;
}

s32 Is_Fresh_DFU_Code(void)
{
	int result = 0;
	u32 code[3];

	code[0] = *((u32 *)UNIQUE_CODE_ADDR);
	code[1] = *((u32 *)UNIQUE_CODE_ADDR + 1);
	code[2] = *((u32 *)UNIQUE_CODE_ADDR + 2);

	if((code[0] == 0xFFFFFFFF) && (code[1] == 0xFFFFFFFF) && (code[2] == 0xFFFFFFFF))  result = 1;

	return result;
}

s32 Is_Unique_Device_Code(void)
{
	int result = 1;
	u32 code[3];

	Get_Unique_Device_ID();

	code[0] = *((u32 *)UNIQUE_CODE_ADDR);
	code[1] = *((u32 *)UNIQUE_CODE_ADDR + 1);
	code[2] = *((u32 *)UNIQUE_CODE_ADDR + 2);

	Generate_UnEncrypt_Code(code);

	if(code[0] != Unique_ID_Mem_Raw[0]) result = 0;
	if(code[1] != Unique_ID_Mem_Raw[1]) result = 0;
	if(code[2] != Unique_ID_Mem_Raw[2]) result = 0;

	return 	result;
}


/******************* (C) COPYRIGHT 2009 STMicroelectronics *****END OF FILE****/
