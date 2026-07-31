#include <stdio.h>
#include "main.h"
#include "usart.h"
#include "gpio.h"
#include "ymodem.h"
#include "elog.h"
#include "common.h"
#include "flash.h"

RCC_ClocksTypeDef RCC_Clocks;
uint8_t recv_data[1024];

typedef void (*pFunction)(void);
static pFunction JumptoApplication;
void Easylogger_Configuration(void)
{
	elog_init();
	elog_set_fmt(ELOG_LVL_ASSERT, ELOG_FMT_LVL);
	elog_set_fmt(ELOG_LVL_ERROR, ELOG_FMT_ALL);
	elog_set_fmt(ELOG_LVL_WARN, ELOG_FMT_LVL | ELOG_FMT_P_INFO);
	elog_set_fmt(ELOG_LVL_INFO, ELOG_FMT_ALL);
	elog_set_fmt(ELOG_LVL_DEBUG, ELOG_FMT_ALL);
	elog_set_fmt(ELOG_LVL_VERBOSE, ELOG_FMT_ALL);

	elog_start();
}
// 延时函数，依赖于HCLK频率
void delay(volatile uint32_t count)
{
	while (count--)
	{
		// 空循环，用于延时
	}
}

// 基于HCLK 100MHz 软件延时
void delay_seconds(uint32_t seconds)
{
	uint32_t count = 100000000;
	for (uint32_t i = 0; i < seconds; i++)
	{
		delay(count); // 软件延时
	}
}

void DisablePeripherals()
{
	RCC_RTCCLKCmd(DISABLE);
	__disable_irq();

	SysTick->CTRL = 0;
	SysTick->LOAD = 0;
	SysTick->VAL = 0;
}
void JumpToApplication(void)
{
	// pc pointer
	uint32_t jumpAddr = (*(__IO uint32_t *)(ApplicationAddress + 4));

	if (((*(__IO uint32_t *)ApplicationAddress) & 0x2FFE0000) == 0x20000000)
	{
		delay_seconds(2);
		for (int i = 0; i < 5; i++)
		{
			log_i("Bootloader running...\r\n");
		}
		delay_seconds(1);
		JumptoApplication = (pFunction)jumpAddr;
		SCB->VTOR = 0x8000000 | 0x3000;
		// set MSP
		__set_MSP(*(__IO uint32_t *)ApplicationAddress);
		JumptoApplication();
	}
	else
	{
		log_e("No valid APP found at 0x%08X\r\n", ApplicationAddress);
	}
	return;
}

/**
 * @brief  从备份区拷贝固件到APP区（Flash -> Flash）
 *         注意：STM32 Flash 必须先擦除再写！
 * @param  src   源地址（BackAppAddress: 0x0803D000）
 * @param  dest  目标地址（ApplicationAddress: 0x08010000）
 * @param  size  固件大小（字节）
 * @retval 0=成功, 1=失败
 */
uint8_t copy_firmware(uint32_t src, uint32_t dest, uint32_t size)
{

	if (flash_erase(dest, size) != 0)
	{
		log_e("Erasing APP area error!\r\n");
		return 1;
	}
	
	FLASH_Unlock();
	FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGPERR | FLASH_FLAG_WRPERR);

	for (int i = 0; i < size; i += 4)
	{
		if (FLASH_ProgramWord(dest, (*(__IO uint32_t *)(src + i))) != FLASH_COMPLETE)
		{
			log_e("Flash write error!\r\n");
			FLASH_Lock(); // ← 加上，不然 Flash 锁开着就走了
			return 1;
		}
		dest += 4;
	}
	FLASH_Lock();
}

// SystemInit();
int main(void)
{
	RCC_ClockSecuritySystemCmd(ENABLE);

	/* systick end of count event each 1ms */
	SystemCoreClockUpdate();
	RCC_GetClocksFreq(&RCC_Clocks);
	SysTick_Config(RCC_Clocks.HCLK_Frequency / 1000);

	key_init();
	led_init();
	USART_init();
	Easylogger_Configuration();

	log_i("Bootloader started");

	if (key_scan())
	{
		/* 等待接收新固件（通过 YMODEM 传输到 BackAppAddress：0x0803D000） */
		int32_t size = Ymodem_Receive(recv_data);

		if (size > 0)
		{
			log_i("Firmware received: %d bytes", size);

			/* 从备份区拷贝到APP运行区 */
			copy_firmware(BackAppAddress, ApplicationAddress, size);
		}
		else
		{
			log_e("Firmware receive failed or cancelled");
		}
	}
	else
	{
		/* 跳转到应用程序 */
		log_i("Attempting to jump to application...");
		SCB->VTOR = 0x8000000;
		DisablePeripherals();
		JumpToApplication();
	}

	/* 如果跳转失败，停在这里 */
	log_e("Failed to jump to application!");
	while (1)
		;
}
