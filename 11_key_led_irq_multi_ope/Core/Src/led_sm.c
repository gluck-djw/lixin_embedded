/**
 ******************************************************************************
 * @file    led_sm.c
 * @brief   LED 状态机 — 控制层
 *         IDLE → 瞬时命令直接转发 HAL
 *         IDLE → LED_CMD_BLINK 进入 BLINKING，200ms 翻转一次，6 次后回 IDLE
 ******************************************************************************
 */

#include "led_sm.h"
#include "bsp_led.h"
#include <stdio.h>

/* ---- 调试日志开关 ---- */
#define LED_SM_DEBUG 1
#if LED_SM_DEBUG
#define LED_SM_LOG(...) printf(__VA_ARGS__)
#else
#define LED_SM_LOG(...)
#endif

/* ---- 状态定义 ---- */
typedef enum {
  LED_SM_IDLE,
  LED_SM_BLINKING,
} led_state_t;

static led_state_t sm_state = LED_SM_IDLE;
static uint8_t blink_cnt = 0;     /* 已完成翻转次数 (1~6)    */
static TickType_t blink_tick = 0; /* 上一次翻转的时刻         */

/* ---- feed：喂命令，立刻决策 ---- */
void led_sm_feed(led_cmd_t cmd) {
  if (cmd == LED_CMD_BLINK) {
    /* 启动/重启闪烁：立刻翻转第一次                 */
    led_toggle();
    blink_cnt = 1;
    blink_tick = xTaskGetTickCount();
    sm_state = LED_SM_BLINKING;
    LED_SM_LOG("[LED_SM] blink start  (1/6)\r\n");
  } else {
    /* 瞬时命令：直接执行，若正在闪烁则打断          */
    switch (cmd) {
    case LED_CMD_ON:
      led_on();
      break;
    case LED_CMD_OFF:
      led_off();
      break;
    case LED_CMD_TOGGLE:
      led_toggle();
      break;
    default:
      break;
    }
    blink_cnt = 0;
    sm_state = LED_SM_IDLE;
    LED_SM_LOG("[LED_SM] cmd %d → HAL, back to idle\r\n", cmd);
  }
}

/* ---- tick：推进节拍（调用方保证 ~1ms 周期） ---- */
void led_sm_tick(void) {
  if (sm_state != LED_SM_BLINKING)
    return; /* IDLE 时零开销     */

  if (xTaskGetTickCount() - blink_tick >= pdMS_TO_TICKS(200)) {
    led_toggle();
    blink_cnt++;
    blink_tick = xTaskGetTickCount();
    LED_SM_LOG("[LED_SM] toggle %d/6\r\n", blink_cnt);

    if (blink_cnt >= 6) {
      /* 3 次完整闪烁（6 次翻转）完成，关灯收工     */
      led_off();
      blink_cnt = 0;
      sm_state = LED_SM_IDLE;
      LED_SM_LOG("[LED_SM] blink done, idle\r\n");
    }
  }
}
