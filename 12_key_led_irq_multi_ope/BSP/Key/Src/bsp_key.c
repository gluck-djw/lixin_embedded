/**
 ******************************************************************************
 * @file    bsp_key.c
 * @brief
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
 ******************************************************************************
 */

#include "bsp_key.h"


key_level_t bsp_key_read(void)
{

    if (GPIO_PIN_RESET == HAL_GPIO_ReadPin(KEY_GPIO_Port, KEY_Pin))
    {
      return KEY_PRESSED;
    }
  return KEY_RELEASED;
}


