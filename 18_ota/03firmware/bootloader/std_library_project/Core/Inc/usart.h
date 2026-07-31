/**
 ******************************************************************************
 * @file    usart.h
 * @brief   This file contains all the function prototypes for
 *          the usart.c file
 *******************************************************************************/

#ifndef __USART_H__
#define __USART_H__

#include "stm32f4xx.h"

void USART_init(void);
void USART_send_byte(USART_TypeDef *USARTx, uint8_t byte);
void USART_send_string(USART_TypeDef *USARTx, uint8_t *byte);
#endif /* __USART_H__ */