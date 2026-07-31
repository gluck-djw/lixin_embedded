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
#include "main.h"
#include "gpio.h"

SysTick_Handler();
// ③ 毫秒延迟
void Delay(uint32_t nTime)
{
  uint32_t tickstart = uwTick;
  while ((uwTick - tickstart) < nTime)
    ;
}
void key_init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct;

  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
  GPIO_InitStruct.GPIO_Pin = GPIO_Pin_0;
  GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN;
  GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_Init(GPIOA, &GPIO_InitStruct);
}

void led_init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct;
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
  GPIO_InitStruct.GPIO_Pin = GPIO_Pin_13;
  GPIO_InitStruct.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStruct.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(GPIOC, &GPIO_InitStruct);
}

uint8_t key_scan(void)
{
  if (Bit_RESET == GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0))
  {
    Delay(10);
    if (Bit_RESET == GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0))
    {
      return 1;
    }
  }
  return 0;
}

void breathing_led(void)
{
  /* 呼吸灯测试 */
  static uint8_t pwmset;
  static uint8_t time;
  static uint8_t timeflag;
  static uint8_t timecount;

  if (timeflag == 0)
  {
    time++;
    if (time >= 1600)
      timeflag = 1;
  }
  else
  {
    time--;
    if (time == 0)
      timeflag = 0;
  }

  // 占空比设置
  pwmset = time / 80;

  if (timecount > 20)
    timecount = 0;
  else
    timecount++;

  if (timecount >= pwmset)
  {
    GPIO_SetBits(GPIOC, GPIO_Pin_13);
  }
  else
  {
    GPIO_ResetBits(GPIOC, GPIO_Pin_13);
  }
  Delay(20);
}