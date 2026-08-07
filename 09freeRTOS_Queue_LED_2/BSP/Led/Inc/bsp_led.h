/**
 ******************************************************************************
 * @file    bsp_led.h
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

#ifndef __BSP_LED_H
#define __BSP_LED_H

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "FreeRTOS.h"
#include "cmsis_os.h"
#include <stdint.h>
#include <stdio.h>

// /*Thread defines -------------------------------------------------------------*/
// extern osThreadId_t led_TaskHandle;
// extern const osThreadAttr_t led_Task_attributes;
// /*Queue defines -------------------------------------------------------------*/
// extern QueueHandle_t LedQueue;

#define LED_ON_LEVEL GPIO_PIN_RESET
#define LED_OFF_LEVEL GPIO_PIN_SET

/*  function resturn status          */
typedef enum
{
  LED_OK = 0,               /* Operation completed successfully.  */
  LED_ERROR = 1,            /* Run-time error without case matched*/
  LED_ERRORTIMEOUT = 2,     /* Operation failed with timeout      */
  LED_ERRORRESOURCE = 3,    /* Resource not available.            */
  LED_ERRORPARAMETER = 4,   /* Parameter error.                   */
  LED_ERRORNOMEMORY = 5,    /* Out of memory.                     */
  LED_ERRORISR = 6,         /* Not allowed in ISR context         */
  LED_RESERVED = 0x7FFFFFFF /* Reserved                           */
} led_status_t;

typedef enum
{
  LED_ON = 0,
  LED_OFF = 1,
  LED_TOGGLE = 2
} led_operation_t;

led_status_t bsp_led(led_operation_t led_operation);


#endif /* __BSP_LED_H */
