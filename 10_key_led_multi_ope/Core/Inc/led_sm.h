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
  LED_CMD_ON     = 0,
  LED_CMD_OFF    = 1,
  LED_CMD_TOGGLE = 2,
  LED_CMD_BLINK  = 3,
} led_cmd_t;

/* ---- 状态机接口 ---- */
void led_sm_feed(led_cmd_t cmd);  /* 喂入一条命令，立刻响应        */
void led_sm_tick(void);           /* 推进节拍（每 1ms 调用一次）  */

#endif /* __LED_SM_H */
