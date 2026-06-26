/**
 ******************************************************************************
 * @file    bsp_led.c
 * @brief   LED 纯硬件抽象层 — GPIO 原子操作，不保存任何状态
 ******************************************************************************
 */

#include "bsp_led.h"

void led_on(void) {
  HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, LED_ON_LEVEL);
}

void led_off(void) {
  HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, LED_OFF_LEVEL);
}

void led_toggle(void) {
  HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
}
