/**
 ******************************************************************************
 * @file    bsp_led.h
 * @brief   LED 纯硬件抽象层 — 只做 GPIO，不带状态、不依赖 RTOS
 ******************************************************************************
 */

#ifndef __BSP_LED_H
#define __BSP_LED_H

#include "main.h"

#define LED_ON_LEVEL  GPIO_PIN_RESET
#define LED_OFF_LEVEL GPIO_PIN_SET

/* ---- 纯硬件操作，调用即生效 ---- */
void led_on(void);
void led_off(void);
void led_toggle(void);

#endif /* __BSP_LED_H */
