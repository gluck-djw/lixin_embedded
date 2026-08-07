/**
 ******************************************************************************
 * @file    bsp_key.h
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

#ifndef __BSP_KEY_H
#define __BSP_KEY_H

/* Includes ------------------------------------------------------------------*/
#include "main.h"

#include <stdint.h>
#include <stdio.h>

/*Defines ------------------------------------------------------------------*/
typedef enum
{
    keyOK = 0,
    keyError = -1,
    keyErrorTimeout = -2,
    keyErrorResource = -3,
    keyErrorParameter = -4,
    keyErrorNoMemory = -5,
    keyErrorISR = -6,
    keyStatusReserved = 0x7FFFFFFF
} key_status_t;

typedef enum
{
    KEY_PRESSED = 0,
    KEY_NO_PRESSED = 1
} key_press_status_t;

/*Declaring ------------------------------------------------------------------*/
key_press_status_t bsp_key_read(void);

#endif /* __BSP_KEY_H */
