#include "w25qxx_handle.h"

static st_w25qxx_handle w25qxx_handle;

void W25QXX_Init(void)
{
    W25Qx_Init();
    w25qxx_handle.readindex = 0;
    w25qxx_handle.writeindex = 0;
    w25qxx_handle.read_sector_index = 0;
    w25qxx_handle.write_sector_index = 0;
    w25qxx_handle.write_buf_index = 0;
}
uint8_t W25QXX_EraseChip(void)
{
    if (W25Qx_OK == W25Qx_Erase_Chip())
    {
        w25qxx_handle.readindex = 0;
        w25qxx_handle.writeindex = 0;
        w25qxx_handle.read_sector_index = 0;
        w25qxx_handle.write_sector_index = 0;
        w25qxx_handle.write_buf_index = 0;
        return 0;
    }
    return 1;
}
uint8_t W25QXX_writehandler(uint8_t *data, uint32_t addr, uint16_t size)
{
    uint16_t index = 0;
    uint32_t address = addr;
    for (uint16_t i = 0; i < size; i++)
    {
        // 将数据写入缓冲区
        w25qxx_handle.databuf[w25qxx_handle.write_buf_index++] = data[i];

        // 当缓冲区满时，将缓冲区数据写入扇区
        if (w25qxx_handle.write_buf_index == W25QXXXX_SUBSECTOR_SIZE)
        {

            w25qxx_handle.write_buf_index = 0;

            // 擦除扇区
            address = w25qxx_handle.write_sector_index * W25QXXXX_SUBSECTOR_SIZE;
            W25Qx_Erase_Block(address);

            for (int j = 0; j < 16; j++) // 16个页
            {                            // 写入扇区（按页）
                address = w25qxx_handle.write_sector_index * W25QXXXX_SUBSECTOR_SIZE + j * W25QXXXX_PAGE_SIZE;
                index = j * W25QXXXX_PAGE_SIZE;
                W25Qx_Write(&w25qxx_handle.databuf[index], address, W25QXXXX_PAGE_SIZE);
            }
            w25qxx_handle.write_sector_index++;
            w25qxx_handle.writeindex += W25QXXXX_SUBSECTOR_SIZE;
        }
    }
    return 0;
}

uint8_t W25QXX_read_sector_handler(uint8_t *data, uint16_t *size)
{
    uint32_t address = 0;
    // 有可读区域
    if (w25qxx_handle.readindex < w25qxx_handle.writeindex)
    {
        // 可读数据>一个扇区
        if (w25qxx_handle.write_sector_index > w25qxx_handle.read_sector_index)
        {
            *size = W25QXXXX_SUBSECTOR_SIZE;
            address = w25qxx_handle.read_sector_index * W25QXXXX_SUBSECTOR_SIZE;
            // 读取一个扇区数据
            if (0 != W25Qx_Read(data, address, *size))
            {
                return 2;
            }
            w25qxx_handle.read_sector_index++;
            w25qxx_handle.readindex += W25QXXXX_SUBSECTOR_SIZE;
        }
        else // 不足一个扇区
        {
            address = w25qxx_handle.read_sector_index * W25QXXXX_SUBSECTOR_SIZE;
            *size = w25qxx_handle.writeindex - w25qxx_handle.readindex;
            if (0 != W25Qx_Read(data, address, *size))
            {
                return 2;
            }
            w25qxx_handle.readindex += *size;
            return 0;
        }
    }
    else // 无可读区域
    {
        return 1;
    }
}