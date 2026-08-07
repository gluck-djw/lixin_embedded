#ifndef W25QXX_HANDLE_H
#define W25QXX_HANDLE_H

#include "w25qxx.h"

typedef struct
{
    uint8_t databuf[4096];
    uint32_t writeindex;
    uint16_t write_buf_index;
    uint8_t write_sector_index;
    uint32_t readindex;
    uint8_t read_sector_index;

} st_w25qxx_handle;

//extern st_w25qxx_handle w25qxx_handle;
void W25QXX_Init(void);
uint8_t W25QXX_EraseChip(void);
uint8_t W25QXX_read_sector_handler(uint8_t *data, uint16_t *size);
uint8_t W25QXX_writehandler(uint8_t *data, uint32_t addr, uint16_t size);
#endif /* W25QXX_HANDLE_H */