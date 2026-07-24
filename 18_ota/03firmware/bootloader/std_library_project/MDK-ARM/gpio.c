/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    gpio.c
  * @brief   This file provides code for the configuration
  *          of all used GPIO pins.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "gpio.h"


void key_init(void)
{
  GPIO_InitStruct->GPIO_Pin  = GPIO_Pin_0;
  GPIO_InitStruct->GPIO_Mode = GPIO_Mode_IN;
  GPIO_InitStruct->GPIO_OType = GPIO_OType_PP;
  GPIO_InitStruct->GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(GPIO_Pin_0, &GPIO_InitStruct);
}


void led_init(void)
{
  GPIO_InitStruct->GPIO_Pin  = GPIO_Pin_13;
  GPIO_InitStruct->GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStruct->GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_InitStruct->GPIO_OType = GPIO_OType_PP;
  GPIO_InitStruct->GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIO_Pin_13, &GPIO_InitStruct);
}


void key_scan(void)
{
	
}