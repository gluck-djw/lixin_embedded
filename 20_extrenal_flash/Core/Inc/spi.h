#ifndef __SPI_H
#define __SPI_H

#include "stm32f4xx_spi.h"

#define F_CS_GPIO_Port GPIOA
#define F_CS_Pin GPIO_Pin_4

#define SPI_TIMEOUT 1
#define SPI_OK 0

void SPI1_Init(void);
uint8_t SPI_WR(uint8_t *pdata, uint16_t size, uint32_t timeout);
uint8_t SPI1_Writebyte(uint8_t *pdata, uint16_t size, uint32_t timeout);
uint8_t SPI1_readbyte(uint8_t *pdata, uint16_t size, uint32_t timeout);
#endif
