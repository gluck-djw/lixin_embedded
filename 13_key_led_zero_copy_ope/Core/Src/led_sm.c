/**
 ******************************************************************************
 * @file    led_sm.c
 * @brief   LED 控制层 — TIM2 硬件定时器驱动闪烁（100ms 节拍）
 ******************************************************************************
 */

#include "led_sm.h"
#include "bsp_led.h"
#include "tim.h"
#include <stdio.h>

#define LED_SM_DEBUG 1
#if LED_SM_DEBUG
#define LED_SM_LOG(...) printf(__VA_ARGS__)
#else
#define LED_SM_LOG(...)
#endif

static uint8_t g_blink_cnt = 0;

void led_sm_feed(led_cmd_t cmd) {
  HAL_TIM_Base_Stop_IT(&htim2);       /* 打断旧闪烁 */

  switch (cmd) {
  case LED_CMD_BLINK_SHORT:
    g_blink_cnt = 2;
    HAL_TIM_Base_Start_IT(&htim2);
    LED_SM_LOG("[LED_SM] blink SHORT\r\n");
    break;

  case LED_CMD_BLINK_LONG:
    g_blink_cnt = 20;
    HAL_TIM_Base_Start_IT(&htim2);
    LED_SM_LOG("[LED_SM] blink LONG\r\n");
    break;

  case LED_CMD_ON:    led_on();     break;
  case LED_CMD_OFF:   led_off();    break;
  case LED_CMD_TOGGLE: led_toggle(); break;
  default: break;
  }
}

void led_sm_tim_callback(void) {
  led_toggle();
  if (--g_blink_cnt == 0) {
    HAL_TIM_Base_Stop_IT(&htim2);
    led_off();
    LED_SM_LOG("[LED_SM] blink done\r\n");
  }
}