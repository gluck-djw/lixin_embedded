/**
 ******************************************************************************
 * @file    usart.c
 * @brief   This file provides code for the configuration
 *          of the USART instances.
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2026 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************/
#include "usart.h"

static void RCC_config(void)
{
    // 启用GPIOA时钟
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);

    // 启用USART1时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
}

static void GPIO_config(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    /* tx */
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* Rx */
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(GPIOA, &GPIO_InitStruct);

    // 配置引脚复用功能
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource9, GPIO_AF_USART1);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource10, GPIO_AF_USART1);
}
static void USART_config(void)
{
    USART_InitTypeDef uart_inst;
    uart_inst.USART_BaudRate = 115200;
    uart_inst.USART_WordLength = USART_WordLength_8b;
    uart_inst.USART_StopBits = USART_StopBits_1;
    uart_inst.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    uart_inst.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
    uart_inst.USART_Parity = USART_Parity_No;
    USART_Init(USART1, &uart_inst);
    USART_Cmd(USART1, ENABLE);
}

/* 串口发送，使用阻塞方式 */
void USART_send_byte(USART_TypeDef *USARTx, uint8_t byte)
{
    while (RESET == USART_GetFlagStatus(USARTx, USART_FLAG_TXE))
        ;
    USART_SendData(USARTx, byte);
}

/* 串口接收，使用阻塞方式 */
void USART_receive_byte(USART_TypeDef *USARTx, uint8_t *byte)
{
    while (RESET == USART_GetFlagStatus(USARTx, USART_FLAG_RXNE))
        ;
    *byte = (uint8_t)USART_ReceiveData(USARTx);
}

void USART_init(void)
{
    RCC_config();
    GPIO_config();
    USART_config();
}