#include "spi.h"
extern volatile uint32_t uwTick;
void SPI1_Init(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);

    GPIO_InitTypeDef GPIO_InitStruct;
    /**SPI1 GPIO Configuration
    PA4     ------> F_CS (Chip Select)
    PA5     ------> SPI1_SCK
    PA6     ------> SPI1_MISO
    PA7     ------> SPI1_MOSI
    */

    // 初始化 PA4 作为片选引脚（普通 GPIO 输出）
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_4;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_OUT;  // 输出模式
    GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_UP;   // 上拉，默认高电平（未选中）
    GPIO_InitStruct.GPIO_OType = GPIO_OType_PP; // 推挽
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStruct);
    GPIO_SetBits(GPIOA, GPIO_Pin_4);  // 默认拉高 CS，不选中芯片

    // 初始化 PA5/6/7 作为 SPI 复用功能引脚
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStruct.GPIO_OType = GPIO_OType_PP; // 推挽
    GPIO_Init(GPIOA, &GPIO_InitStruct);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource5, GPIO_AF_SPI1);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource6, GPIO_AF_SPI1);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource7, GPIO_AF_SPI1);

    /* 防止旧配置残留干扰新配置 */
    RCC_APB2PeriphResetCmd(RCC_APB2Periph_SPI1, ENABLE);  // ② 复位：清掉 SPI 寄存器
    RCC_APB2PeriphResetCmd(RCC_APB2Periph_SPI1, DISABLE); // ③ 停止复位：释放

    SPI_InitTypeDef hspi1;
    hspi1.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    hspi1.SPI_DataSize = SPI_DataSize_8b;
    hspi1.SPI_CPOL = SPI_CPOL_Low;
    hspi1.SPI_CPHA = SPI_CPHA_1Edge; // SCLK的第一个边沿采样（移入）
    hspi1.SPI_NSS = SPI_NSS_Soft;
    hspi1.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2;
    hspi1.SPI_FirstBit = SPI_FirstBit_MSB;
    hspi1.SPI_CRCPolynomial = 10;
    hspi1.SPI_Mode = SPI_Mode_Master;
    SPI_Init(SPI1, &hspi1);

    SPI_TIModeCmd(SPI1, DISABLE);

    SPI_Cmd(SPI1, ENABLE); // 开：SPI 开始工作

    if (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) != RESET)
    {
        SPI_I2S_SendData(SPI1, 0xff);
    }
}

uint8_t SPI_WR(uint8_t *pdata, uint16_t size, uint32_t timeout)
{
    uint16_t rxsize = size; // SPI Rx Counter
    uint16_t txsize = size; // SPI Tx Counter
    uint32_t txallow = 1u;
    const uint8_t *ptxbuffer = (const uint8_t *)pdata;
    uint8_t *prxbuffer = pdata; // SPI Rx Buffer
    uint32_t tickstart = uwTick;

    while (rxsize > 0 || txsize > 0)
    {
        if (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) != RESET && (txsize > 0) && txallow == 1)
        {
            SPI_I2S_SendData(SPI1, *(const uint8_t *)ptxbuffer);
            ptxbuffer += sizeof(uint8_t);
            txsize--;
            txallow = 0;
        }
        if (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) != RESET && (rxsize > 0))
        {
            *prxbuffer = SPI_I2S_ReceiveData(SPI1);
            prxbuffer += sizeof(uint8_t);
            rxsize--;
            txallow = 1;
        }
        if (uwTick - tickstart > timeout)
        {
            return SPI_TIMEOUT;
        }
    }
    return SPI_OK;
}

uint8_t SPI1_Writebyte(uint8_t *pdata, uint16_t size, uint32_t timeout)
{
    if (0 == SPI_WR(pdata, size, timeout))
    {
        SPI_I2S_ClearFlag(SPI1, SPI_I2S_FLAG_OVR); //  不读接收数据 → RXNE 一直满 → 继续发 → OVR（溢出）置位
        return SPI_OK;
    }
    return SPI_TIMEOUT;
}

uint8_t SPI1_readbyte(uint8_t *pdata, uint16_t size, uint32_t timeout)
{
    if (0 == SPI_WR(pdata, size, timeout))
    {
        return SPI_OK;
    }
    return SPI_TIMEOUT;
}
