#include "flash.h"
#include <stdint.h>

uint8_t flash_erase(uint32_t Address, uint32_t size)
{

    uint32_t erase_sector_start = GetSector(Address);
    uint32_t erase_sector_end = GetSector(Address + size);

    for (uint32_t i = erase_sector_start; i <= erase_sector_end; i++)
    {
        if (Flash_Erase_sector(i) != FLASH_COMPLETE)
            return 1;
    }

    return 0;
}

/**
 * @brief  Get FLASH Sector from address (for STM32F4)
 * @param  Address: Flash address
 * @retval FLASH_Sector_0 ~ FLASH_Sector_7
 */

static uint32_t GetSector(uint32_t Address)
{
    if (Address < FLASH_SECTOR_1_startaddr)
        return FLASH_Sector_0;
    else if (Address < FLASH_SECTOR_2_startaddr)
        return FLASH_Sector_1;
    else if (Address < FLASH_SECTOR_3_startaddr)
        return FLASH_Sector_2;
    else if (Address < FLASH_SECTOR_4_startaddr)
        return FLASH_Sector_3;
    else if (Address < FLASH_SECTOR_5_startaddr)

        return FLASH_Sector_4;
    else if (Address < FLASH_SECTOR_6_startaddr)
        return FLASH_Sector_5;
    else if (Address < FLASH_SECTOR_7_startaddr)
        return FLASH_Sector_6;
    else
        return FLASH_Sector_7;
}
void Flash_Unlock()
{
    FLASH_Unlock();
    while (FLASH_GetStatus() == FLASH_BUSY)
        ;
};

// 擦除APP区域的数据
// f4是按照扇区操作，计划将app放在扇区6 ,备份放在扇区7
FLASH_Status Flash_Erase_sector(uint32_t Flash_sector)
{
    Flash_Unlock();
    FLASH_Status staus = FLASH_EraseSector(Flash_sector, VoltageRange_3);
    FLASH_Lock();
    return staus;
}

void Flash_Write(uint32_t address, uint32_t data)
{
    // 解锁flash
    Flash_Unlock();
    // 清除所有标志位
    FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGPERR | FLASH_FLAG_WRPERR);

    if (FLASH_COMPLETE == FLASH_ProgramWord(address, data))
    {
        // 写入成功
    }
    else
    {
        // 写入失败
    }
    FLASH_Lock();
}