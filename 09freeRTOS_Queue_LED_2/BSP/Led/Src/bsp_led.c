/**
 ******************************************************************************
 * @file    bsp_led.c
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

#include "bsp_led.h"

// control led
led_status_t bsp_led(led_operation_t led_operation)
{
    if (LED_ON == led_operation)
    {
        HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, LED_ON_LEVEL);
    }

    else if (LED_OFF == led_operation)
    {
        HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, LED_OFF_LEVEL);
    }

    else if (LED_TOGGLE == led_operation)
    {
        HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
    }
    else
    {
        return LED_ERROR;
    }
    return LED_OK;
}
