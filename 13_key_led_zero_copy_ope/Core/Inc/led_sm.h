/**
 ******************************************************************************
 * @file    led_sm.h
 * @brief   LED 状态机 — 控制层
 *         依赖 bsp_led（HAL），向 Led_Task（调度层）暴露 feed/tick 接口
 ******************************************************************************
 */

#ifndef __LED_SM_H
#define __LED_SM_H

#include "FreeRTOS.h"
#include "cmsis_os.h"

/* ---- 命令枚举 ---- */
typedef enum {
  LED_CMD_ON = 0,
  LED_CMD_OFF = 1,
  LED_CMD_TOGGLE = 2,
  LED_CMD_BLINK_SHORT = 3,
  LED_CMD_BLINK_LONG  = 4
} led_cmd_t;

/* ---- 状态机接口 ---- */
void led_sm_feed(led_cmd_t cmd);       /* 喂入命令，立刻生效                 */
void led_sm_tim_callback(void);        /* TIM2 中断回调，每 100ms 调一次     */

#endif /* __LED_SM_H */
