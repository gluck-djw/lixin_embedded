#include "FreeRTOS.h"
#include "key.h"
#include "bsp_key.h"
#include <stdio.h>

key_state_t key_state = KEY_STATE_IDLE;

TickType_t down_tick = 0;
key_event_t key_tick(key_isr_msg_t key_mg) {
  key_event_t key_op = KEY_NONE_PRESSED;
  switch (key_state) {
  case KEY_STATE_IDLE: {
    if (KEY_EDGE_DOWN == key_mg.edge) {
      down_tick = key_mg.tick;
      key_state = KEY_STATE_PRESSED;
    }

  } break;

  case KEY_STATE_PRESSED: {
    if (KEY_EDGE_UP == key_mg.edge) {
      if (down_tick) {
        TickType_t held = key_mg.tick - down_tick;

        if (held > pdMS_TO_TICKS(LONG_PRESS_TIME_MS)) {
          key_op = KEY_LONG_PRESSED;
        } else {
          key_op = KEY_SHORT_PRESSED;
        }
        down_tick = 0;
      }
      key_state = KEY_STATE_IDLE;
    }

    break;
  default:
    break;
  }
  }
  return key_op;
}
