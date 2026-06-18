#ifndef __KEY_H
#define __KEY_H

#include "FreeRTOS.h"
#include "cmsis_os.h"
#include <stdint.h>

/* ---- 按键状态机 ---- */
typedef enum {
  KEY_STATE_IDLE = 0,
  KEY_STATE_PRESSED = 1,
} key_state_t;

/* ---- 按键结果（对外输出） ---- */
typedef enum {
  KEY_NONE_PRESSED = 0,
  KEY_SHORT_PRESSED = 1,
  KEY_LONG_PRESSED = 2,
} key_event_t;

/* ---- ISR → key_task 的消息 ---- */
typedef enum {
  KEY_EDGE_DOWN,
  KEY_EDGE_UP,
} key_edge_t;

typedef struct {
  key_edge_t edge;
  TickType_t tick;
} key_isr_msg_t;

/* ---- 对外接口 ---- */
void         key_init(void);
void         key_task(void *arg);
key_event_t  key_wait_event(void);
key_event_t  key_poll_event(void);

#endif /* __KEY_H */