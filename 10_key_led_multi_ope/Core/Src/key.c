#include "key.h"
#include "bsp_key.h"
#include <stdio.h>
#include <portmacro.h>

key_state_t key_state = KEY_STATE_IDLE;

TickType_t down_tick = 0;
key_event_t key_tick(void) {
  key_event_t key_op = KEY_NONE_PRESSED;
  key_level_t nowkey = bsp_key_read();  // 读取按键状态
  switch (key_state) {
  case KEY_STATE_IDLE: {
    if (KEY_PRESSED == nowkey) {
      osDelay(10);
      if (KEY_PRESSED == bsp_key_read()) {
        down_tick = xTaskGetTickCount();  // 获取当前时间
        key_state = KEY_STATE_PRESSED;
      }
    }
    break;
  }
  case KEY_STATE_PRESSED: {
    if (KEY_RELEASED == nowkey) {
      osDelay(10);
      if (KEY_RELEASED == bsp_key_read()) {
        if (down_tick) {
          TickType_t held = xTaskGetTickCount() - down_tick;

          if (held > pdMS_TO_TICKS(LONG_PRESS_TIME_MS)) {
            key_op = KEY_LONG_PRESSED;
          } else {
            key_op = KEY_SHORT_PRESSED;
          }
          down_tick = 0;
        }
        key_state = KEY_STATE_IDLE;
      }
    }
    break;
  default:
    break;
  }
  }
  return key_op;
}