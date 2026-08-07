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

#ifndef __BSP_KEY_H__
#define __BSP_KEY_H__

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"

#include <stdint.h>
#include <stdio.h>


//******************************** Defines **********************************//

/*  function resturn status          */
typedef enum
{
  KEY_OK                = 0,           /* Operation completed successfully.  */
  KEY_ERROR             = 1,           /* Run-time error without case matched*/
  KEY_ERRORTIMEOUT      = 2,           /* Operation failed with timeout      */
  KEY_ERRORRESOURCE     = 3,           /* Resource not available.            */
  KEY_ERRORPARAMETER    = 4,           /* Parameter error.                   */
  KEY_ERRORNOMEMORY     = 5,           /* Out of memory.                     */
  KEY_ERRORISR          = 6,           /* Not allowed in ISR context         */
  KEY_RESERVED          = 0x7FFFFFFF   /* Reserved                           */
} key_status_t;

typedef enum
{
    KEY_PRESSED = 0,
    KEY_NO_PRESSED = 1
} key_press_status_t;
//******************************** Defines **********************************//

/*Declaring ------------------------------------------------------------------*/
key_press_status_t bsp_key_read(void);

#endif /* __BSP_KEY_H */
