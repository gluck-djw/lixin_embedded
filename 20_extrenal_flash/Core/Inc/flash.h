#ifndef __FLASH_H__
#define __FLASH_H__

#include "stm32f4xx_flash.h"

/* base address of the Flash sectors*/
#define FLASH_SECTOR_0_startaddr ((uint32_t)0x08000000) /*!< Base @ of Sector 0, 16 Kbytes */
#define FLASH_SECTOR_1_startaddr ((uint32_t)0x08004000)
#define FLASH_SECTOR_2_startaddr ((uint32_t)0x08008000)
#define FLASH_SECTOR_3_startaddr ((uint32_t)0x0800C000)
#define FLASH_SECTOR_4_startaddr ((uint32_t)0x08010000)
#define FLASH_SECTOR_5_startaddr ((uint32_t)0x08020000)
#define FLASH_SECTOR_6_startaddr ((uint32_t)0x08040000)
#define FLASH_SECTOR_7_startaddr ((uint32_t)0x08060000)

static uint32_t GetSector(uint32_t Address);
uint8_t flash_erase(uint32_t Address, uint32_t size);
FLASH_Status Flash_Erase_sector(uint32_t Flash_sector);
void Flash_Write(uint32_t address, uint32_t data);
#endif /* __FLASH_H__ */
