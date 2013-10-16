#include "stdio.h"
#include "stm32f2xx.h"
#include "mxchipWNET.h"
#include "command.h"
#include "tcpip.h"

#define NEW_MODULE

extern u8 moduleStatus;
extern 	void main_function_tick(void);
extern 	void red_led_blink_tick(void);
extern void saveNFCConfig(void);

static sys_state_e system_state, last_work_state;
static u8 wifi_state = 0; // 0=disconnect, 1=connect
static u8 wps_ing = 0;

volatile u8 NFC_STARTED = 0;
volatile u8 NFC_STARTED2 = 0;

void set_sys_state(int state);

/* ========================================
	User provide callback functions 
    ======================================== */
void mxchipWifiStatusHandler(int event)
{
	switch (event) {
	case MXCHIP_WIFI_UP:
		wifi_state |= 1;
        mdns_begin(0);
		break;
	case MXCHIP_WIFI_DOWN:
		wifi_state &= ~1;
		break;
	case MXCHIP_UAP_ACTIVE:
        mdns_begin(1);
		wifi_state |= 2;
		break;
	case MXCHIP_UAP_IDLE:
		wifi_state &= ~2;
		break;
	default:
		break;
	}
}

void mxchipPMKHandler(struct wlan_network *net)
{
	sys_config_t *pconfig = get_running_config();
	base_config_t *pbase = &pconfig->base;
	extra_config_t *pextra = &pconfig->extra;
	sys_unconfig_t *punconfig = get_un_config();
	int i;
	
	if ((SEC_MODE_WPA_PSK == pextra->uap_secmode) &&
		(punconfig->pmk_invalid[0] != 0)){
		if ((strcmp(pextra->uap_ssid, net->ssid) == 0) &&
			(strcmp(pextra->uap_key	, net->security.psk) == 0)) {
			punconfig->pmk_invalid[0] = 0;
			memcpy(punconfig->pmk[0], net->security.pmk, WLAN_PMK_LENGTH);
			save_unconfig();
			return;
		}
	}
	if ((punconfig->pmk_invalid[1] != 0) && (pbase->sec_mode == SEC_MODE_WPA_PSK || 
		pbase->sec_mode == SEC_MODE_AUTO)) {
		if ((strcmp(pbase->wifi_ssid, net->ssid) == 0) &&
			(strcmp(pbase->wpa_psk, net->security.psk) == 0)) {
			punconfig->pmk_invalid[1] = 0;
			memcpy(punconfig->pmk[1], net->security.pmk, WLAN_PMK_LENGTH);
			save_unconfig();
			return;
		}
	}

	for (i=0; i<4; i++) {
		if ((punconfig->pmk_invalid[i+2] != 0) &&
			(pextra->new_wpa_conf[i].sec_mode == SEC_MODE_WPA_PSK || 
			pextra->new_wpa_conf[i].sec_mode == SEC_MODE_AUTO)) {
			if ((strcmp(pextra->new_wpa_conf[i].ssid, net->ssid) == 0) &&
				(strcmp(pextra->new_wpa_conf[i].key, net->security.psk) == 0)) {
				punconfig->pmk_invalid[i+2] = 0;
				memcpy(punconfig->pmk[i+2], net->security.pmk, WLAN_PMK_LENGTH);
				save_unconfig();
				return;
			}
		}
	}
}

void print_time(char *str)
{
}

void delay_reload(void)
{
	set_sys_state(SYS_STATE_DELAY_RELOAD);
}

void reload(void)
{
	NVIC_SystemReset();
}

#ifdef NEW_MODULE
/**
  * @brief  Configures EXTI Line0 (connected to PA0 pin) in interrupt mode
  * @param  None
  * @retval None
  */
void default_gpio_init(void)
{
  EXTI_InitTypeDef   EXTI_InitStructure;
  GPIO_InitTypeDef   GPIO_InitStructure;
  NVIC_InitTypeDef   NVIC_InitStructure;

  /* Enable GPIOA clock */
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA|RCC_AHB1Periph_GPIOB, ENABLE);
  /* Enable SYSCFG clock */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);
  
  /* Configure PA3 pin as input floating */
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	 /* Configure PA5 pin as input floating */
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
  GPIO_Init(GPIOB, &GPIO_InitStructure);

  /* Connect EXTI Line0 to PA0 pin */
  SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOA, EXTI_PinSource3);
	SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOB, EXTI_PinSource4);

  /* Configure EXTI Line0 */
  EXTI_InitStructure.EXTI_Line = EXTI_Line3|EXTI_Line4;
  EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
  EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising_Falling;  
  EXTI_InitStructure.EXTI_LineCmd = ENABLE;
  EXTI_Init(&EXTI_InitStructure);

  /* Enable and set EXTI Line0 Interrupt to the lowest priority */
  NVIC_InitStructure.NVIC_IRQChannel = EXTI3_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x0F;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x0F;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
	
	NVIC_InitStructure.NVIC_IRQChannel = EXTI4_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x0F;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x0F;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
}

#else
/**
  * @brief  Configures EXTI Line0 (connected to PA0 pin) in interrupt mode
  * @param  None
  * @retval None
  */
void default_gpio_init(void)
{
  EXTI_InitTypeDef   EXTI_InitStructure;
  GPIO_InitTypeDef   GPIO_InitStructure;
  NVIC_InitTypeDef   NVIC_InitStructure;

  /* Enable GPIOB clock */
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
  /* Enable SYSCFG clock */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);
  
  /* Configure PA3 pin as input floating */
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
  GPIO_Init(GPIOB, &GPIO_InitStructure);

  /* Connect EXTI Line0 to PA0 pin */
  SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOB, EXTI_PinSource4);

  /* Configure EXTI Line0 */
  EXTI_InitStructure.EXTI_Line = EXTI_Line4;
  EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
  EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising_Falling;  
  EXTI_InitStructure.EXTI_LineCmd = ENABLE;
  EXTI_Init(&EXTI_InitStructure);

  /* Enable and set EXTI Line0 Interrupt to the lowest priority */
  NVIC_InitStructure.NVIC_IRQChannel = EXTI4_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x0F;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x0F;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
}
#endif

static void wakeUpIOInit()
{
	GPIO_InitTypeDef GPIO_InitStructure;
	EXTI_InitTypeDef EXTI_InitStructure;

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
       GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
}

static void int2host_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
       GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
}

static void int2host(int level) 
{
	if (level) {	// enable
		GPIO_WriteBit(GPIOC, GPIO_Pin_11, Bit_SET);	
	} else {
		GPIO_WriteBit(GPIOC, GPIO_Pin_11, Bit_RESET);	
	}
}

/* Check whether goto standby
  * If PA0==0, goto standby mode. 
  * 
  */
static void check_gotostandby(void)
{
	wakeUpIOInit();
	if(GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0) == Bit_RESET) {
		//enable interrupt
		EXTI->IMR |= EXTI_Line0;
		
		//stm32 enter standby mode
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);
		PWR_WakeUpPinCmd(ENABLE);
		PWR_EnterSTANDBYMode();
		
		while(1);
	} else {
		//enable_watchdog(5);
	}
}

void set_sys_state(int state)
{
	if (system_state < SYS_STATE_MAX_WORK_STATE)
		last_work_state = system_state;

	system_state = state;
}

int get_sys_state(void)
{
	return (int)system_state;
}

int get_wifi_state(void)
{
	return (int)wifi_state;
}

int is_wps_state(void)
{
    return wps_ing;
}

void goto_default(void)
{
	set_sys_state(SYS_STATE_GOTO_DEFAULT);
}

void goto_wps(void)
{
	set_sys_state(SYS_STATE_WPS);
    wps_ing = 1;
}

/* DUAL mode, uap start at system initilize state. */
static void dual_uap_start(void)
{
	sys_config_t *pconfig = get_running_config();
	base_config_t *pbase = &pconfig->base;
	extra_config_t *pextra = &pconfig->extra;
	sys_unconfig_t *punconfig = get_un_config();
	struct wlan_network wlan_config;

	if (pbase->wifi_mode != DUAL_MODE)
		return;
	
	memset(&wlan_config, 0, sizeof(wlan_config));
	/* Set profile name */
	strcpy(wlan_config.name, "UAP");
	strcpy(wlan_config.ssid, pextra->uap_ssid);
	wlan_config.channel = 0;
	wlan_config.mode = WLAN_MODE_UAP;

	switch(pextra->uap_secmode) {
	case SEC_MODE_WEP:
	case SEC_MODE_WEP_HEX:
		wlan_config.security.type = WLAN_SECURITY_WEP_OPEN;
		break;
	case SEC_MODE_WPA_NONE:
		wlan_config.security.type = WLAN_SECURITY_NONE;
		break;
	case SEC_MODE_WPA_PSK:
		wlan_config.security.type = WLAN_SECURITY_WPA2;
		break;
	default:
		wlan_config.security.type = WLAN_SECURITY_WPA2;
		break;
	}

	if (SEC_MODE_WEP_HEX == pextra->uap_secmode) {
		wlan_config.security.psk_len = str2hex(pextra->uap_key, 
				wlan_config.security.psk, 32);
	} else {
		strcpy(wlan_config.security.psk, pextra->uap_key);
		wlan_config.security.psk_len = strlen(pextra->uap_key);
	}

	if (punconfig->pmk_invalid[0] == 0) {
		wlan_config.security.pmk_valid = 1;
		memcpy(wlan_config.security.pmk, punconfig->pmk[0], WLAN_PMK_LENGTH);
	}
	wlan_config.address.addr_type = ADDR_TYPE_STATIC;
	wlan_config.address.ip = 0x0a0a0a01;
	wlan_config.address.gw = 0x0a0a0a02;
	wlan_config.address.netmask = 0xffffff00;
	wlan_config.address.dns1 = 0x0a0a0a01;

	wlan_add_network(&wlan_config);
	wlan_start_network(wlan_config.name);
}
u8 ssid[4];
u8 pass[4];



int main(void)
{
	u32 endtime;
    
	system_state = SYS_STATE_INIT;
	wifi_state = 0;
	reset_uart_init();
	default_gpio_init();
	check_gotostandby();
	int2host_init();
	int2host(1);
	NFC_TAG_INIT();
	mxchipInit();
	userResetConfig();
	reset_uart_deinit();
	get_config();
	tcpip_init();
	emsp_init();
	uart_init();
	http_init();
#ifndef USE_0302
	APP_Verify_Program_Code();
#endif

	dual_uap_start();
	SetTimer(1, main_function_tick, 1);
	wifi_option_config();
	int2host(0);
	system_is_bootup(); //EMSP_CMD_SYSTEM_BOOTUP
	while(1) {
		if(GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0) != Bit_RESET) 
			IWDG_ReloadCounter();	// Reload IWDG counter
		else
			reload();
		
		if(NFC_STARTED){
			SetTimer(3000,saveNFCConfig,0);
			NFC_STARTED = 0;     //NFC task started
			NFC_STARTED2 = 1;    //NFC task pending
		}
		
		http_tick(); // HTTP Server
		mxchipTick(); // wifi, network function
		switch (system_state) {
			case SYS_STATE_RESET:
        wait_uart_dma_clean();
				NVIC_SystemReset();
				break;
			case SYS_STATE_GOTO_DEFAULT:
				default_config();
				save_config();
				NVIC_SystemReset();
				break;
			case SYS_STATE_DELAY_RELOAD:
				endtime = MS_TIMER+500;
				while(endtime>MS_TIMER)
					mxchipTick();
				
				NVIC_SystemReset();
				break;
				
			case SYS_STATE_WPS: // User pushed WPS PBC, try wps.
				user_wifi_stop();
				wps_pbc_start();
				set_sys_state(last_work_state);
				break;
			case SYS_STATE_WPS_FAIL:
                wps_ing = 0;
				user_wifi_start();
				set_sys_state(last_work_state);
                set_conncetion_status(0);
				break;
			case SYS_STATE_WPS_SUCCESS:
                wps_ing = 0;
				set_sys_state(last_work_state);
				save_config();
                set_conncetion_status(0);
				break;
			default:
				break;
		}
	}
}
