
#include "main.h"
#include "usart.h"
#include "gpio.h"
#include "ymodem.h"
#include "elog.h"
#include "common.h"
#include "flash.h"
#include "spi.h"
#include "aes.h"
#include "w25qxx.h"
#include "w25qxx_handle.h"
#include "stm32f4xx_rcc.h"

#include <stdio.h>

RCC_ClocksTypeDef RCC_Clocks;
uint8_t recv_data[1024];

typedef void (*pFunction)(void);
static pFunction JumptoApp;
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

// ① 全局变量
volatile uint32_t uwTick;

/* 硬件延时 */
// ③ 毫秒延迟
void Delay(uint32_t nTime)
{
	uint32_t tickstart = uwTick;
	while ((uwTick - tickstart) < nTime)
		;
}

 /* 软件延时 */
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
	// 关闭 SPI1
	SPI_Cmd(SPI1, DISABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, DISABLE);

//	// 关闭 USART
//	USART_Cmd(USART1, DISABLE);
//	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, DISABLE);

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
		for (int i = 0; i < 5; i++)
		{
			log_i("Bootloader running...\r\n");
		}
		JumptoApp = (pFunction)jumpAddr;
		SCB->VTOR = 0x8000000 | 0x10000;
		// set MSP
		__set_MSP(*(__IO uint32_t *)ApplicationAddress);
		JumptoApp();
	}
	else
	{
		log_e("No valid APP found at 0x%08X\r\n", ApplicationAddress);
	}
	return;
}

/**
 * @brief  从外部flash解密后拷贝固件到APP区（Flash -> Flash）
 *         注意：STM32 Flash 必须先擦除再写！
 * @param  dest  目标地址（ApplicationAddress: 0x08010000）
 * @param  size  固件大小（字节）
 * @retval 0=成功, 1=失败
 */
uint8_t copy_firmware(uint32_t dest, uint32_t size)
{
	unsigned char key[32] = {0x31, 0x32, 0x31, 0x32, 0x31, 0x32, 0x31, 0x32, 0x31, 0x32, 0x31, 0x32, 0x31, 0x32, 0x31, 0x32, 0x31, 0x32, 0x31, 0x32, 0x31, 0x32, 0x31, 0x32, 0x31, 0x32, 0x31, 0x32, 0x31, 0x32, 0x31, 0x32};
	unsigned char IV[16] = {0x31, 0x32, 0x31, 0x32, 0x31, 0x32, 0x31, 0x32, 0x31, 0x32, 0x31, 0x32, 0x31, 0x32, 0x31, 0x32};

	static uint8_t mem_buf[4096];  // 改为 static，放到全局数据区，不占用栈空间
	uint32_t read_mem_index = 0;
	uint16_t read_mem_size = 0;

	volatile uint32_t custom_len = 0;	 // L1 自定义数据长度
	volatile uint32_t plaintext_len = 0; // L2 明文长度
	static uint8_t tmp[16];
	static uint8_t buf[16];			   // 存储明文数据
	volatile uint32_t appsize = 0; // 升级包大小

	uint16_t readcnt = 0;

	/*  4字节自定义数据长度+自定义数据+4字节明文长度+明文数据 */
	AES_key_expansion(key);
	uint8_t ret = W25QXX_read_sector_handler(mem_buf, &read_mem_size);
	if (2 == ret) // 读取密文数据
	{
		log_e("W25QXX_read_sector_handler error");
		return 1;
	}
	else if (1 == ret)
	{
		log_e("no date from W25QXX");
		return 1;
	}

	if (read_mem_size > 16)
	{
		memcpy(tmp, mem_buf, 16);
		AES_decrypt(tmp, buf, 16, IV);									 // 解密除明文长度外的16字节
		custom_len = buf[3] << 24 | buf[2] << 16 | buf[1] << 8 | buf[0]; // 自定义数据长度

		read_mem_index += 4 + custom_len + 4; // 跳过整个头部 → 指向数据
		appsize = (buf[15] << 24) + (buf[14] << 16) + (buf[13] << 8) + buf[12];
		log_i("Erasing APP area %d bytes!", appsize);
		readcnt = (appsize + 15) / 16;
	}
	else
	{
		log_e("Read flash data error!\r\n");
		return 1;
	}

	if (flash_erase(dest, appsize) != 0)
	{
		log_e("Erasing APP area error!\r\n");
		return 1;
	}
	/* to do：刷写失败需要回滚机制 */

	// ✅ IV 应该是头部的 16 字节密文块（CBC模式，前一个密文块作为下一个的IV）
	memcpy(IV, mem_buf, 16);

	for (int i = 0; i < readcnt; i++)
	{
		if (read_mem_index == read_mem_size) // 读取完
		{
			uint8_t ret = W25QXX_read_sector_handler(mem_buf, &read_mem_size);
			if (2 == ret) // 读取密文数据
			{
				log_e("W25QXX_read_sector_handler error");
				return 1;
			}
			else if (1 == ret) //
			{
				log_i("Read flash data complete!\r\n");
				break;
			}
			read_mem_index = 0;
		}
		// 解密

		memcpy(tmp, mem_buf + read_mem_index, 16);
		AES_decrypt(tmp, buf, 16, IV);
		/* to do：解密后二次校验 */

		// ✅ 立即更新 IV（在写入之前）
		memcpy(IV, tmp, 16);

		// 存入app区
		FLASH_Unlock();
		FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGPERR | FLASH_FLAG_WRPERR);

		for (int j = 0; j < 16; j += 4)
		{
			FLASH_Status status = FLASH_ProgramWord(dest, (*(__IO uint32_t *)(buf + j)));
			if (status != FLASH_COMPLETE)
			{
				log_e("Flash write error at 0x%08X, status=%d", dest, status);
				FLASH_Lock();
				return 1;
			}
			// 验证写入
			uint32_t readback = (*(__IO uint32_t *)dest);
			if (readback != (*(__IO uint32_t *)(buf + j)))
			{
				log_e("Verify failed! Addr:0x%08X, Write:0x%08X, Read:0x%08X",
				      dest, (*(__IO uint32_t *)(buf + j)), readback);
			}
			dest += 4;
		}
		FLASH_Lock();

		read_mem_index += 16;
	}

	// 验证写入结果
	log_i("Write complete! Verifying first 64 bytes at 0x%08X:", ApplicationAddress);
	for (int i = 0; i < 64; i += 16)
	{
		log_i("%08X: %08X %08X %08X %08X",
		      ApplicationAddress + i,
		      (*(__IO uint32_t *)(ApplicationAddress + i + 0)),
		      (*(__IO uint32_t *)(ApplicationAddress + i + 4)),
		      (*(__IO uint32_t *)(ApplicationAddress + i + 8)),
		      (*(__IO uint32_t *)(ApplicationAddress + i + 12)));
	}

	return 0;
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
	SPI1_Init();
	Easylogger_Configuration();
	uint8_t state = W25Qx_Init();
	if (state != W25Qx_OK)
	{
		log_e("W25Qx_Init error");
	}
	log_i("Bootloader started");

	while (1)
	{
		if (key_scan())
		{

			/* 等待接收新固件（通过 YMODEM 传输) */
			int32_t size = Ymodem_Receive(recv_data);
			/* to do： 没有经过CRC校验 */

			if (size > (0x18010 - 1) || size < 0) // App 运行区     96KB  (0x10000~0x27FFF)
			{
				log_e("Firmware receive failed or cancelled");
			}
			else
			{
				log_i("Firmware received: %d bytes", size);

				/* 从备份区拷贝到APP运行区 */
				copy_firmware(ApplicationAddress, size);
			}
			/* ★ 关键：等松键，防止回循环又进下载 */
			while (key_scan())
				;	  // 一直等到松开
			Delay(1); // 松键防抖
		}
		else
		{
			/* 跳转到应用程序 */
			log_i("Attempting to jump to application...");
			SCB->VTOR = 0x8000000;
			DisablePeripherals();
			JumpToApplication();

			/* 如果跳转失败，停在这里 */
			log_e("Failed to jump to application!");
			while (1)
				;
		}
	}
}
