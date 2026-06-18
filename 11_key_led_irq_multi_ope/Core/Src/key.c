#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "key.h"
#include "bsp_key.h"
#include <stdio.h>

#define ISR_QUEUE_SIZE    5
#define RESULT_QUEUE_SIZE 5
#define LONG_PRESS_TIME_MS 1000

/* ---- 模块内部状态 ---- */
static QueueHandle_t g_isr_queue;       /* ISR → key_task      */
static QueueHandle_t g_result_queue;    /* key_task → 消费者    */

static key_state_t  sm_state   = KEY_STATE_IDLE;
static TickType_t   down_tick  = 0;

/* ---- 内部：按键状态机 ---- */
static key_event_t key_tick(key_isr_msg_t msg) {
  key_event_t key_op = KEY_NONE_PRESSED;
  switch (sm_state) {
  case KEY_STATE_IDLE:
    if (msg.edge == KEY_EDGE_DOWN) {
      down_tick = msg.tick;
      sm_state  = KEY_STATE_PRESSED;
    }
    break;

  case KEY_STATE_PRESSED:
    if (msg.edge == KEY_EDGE_UP && down_tick) {
      TickType_t held = msg.tick - down_tick;
      key_op = (held > pdMS_TO_TICKS(LONG_PRESS_TIME_MS))
                 ? KEY_LONG_PRESSED : KEY_SHORT_PRESSED;
      down_tick = 0;
      sm_state  = KEY_STATE_IDLE;
    }
    break;
  }
  return key_op;
}

/* ---- 对外：模块初始化 ---- */
void key_init(void) {
  g_isr_queue    = xQueueCreate(ISR_QUEUE_SIZE,    sizeof(key_isr_msg_t));
  g_result_queue = xQueueCreate(RESULT_QUEUE_SIZE, sizeof(key_event_t));
}

/* ---- 对外：key 处理任务 ---- */
void key_task(void *arg) {
  key_isr_msg_t msg;
  for (;;) {
    if (xQueueReceive(g_isr_queue, &msg, portMAX_DELAY) == pdPASS) {
      key_event_t event = key_tick(msg);
      if (event != KEY_NONE_PRESSED) {
        xQueueSend(g_result_queue, &event, 0);
      }
    }
  }
}

/* ---- 对外：消费者阻塞等事件 ---- */
key_event_t key_wait_event(void) {
  key_event_t event = KEY_NONE_PRESSED;
  xQueueReceive(g_result_queue, &event, portMAX_DELAY);
  return event;
}

/* ---- 对外：非阻塞取事件（无事件返回 KEY_NONE_PRESSED） ---- */
key_event_t key_poll_event(void) {
  key_event_t event = KEY_NONE_PRESSED;
  xQueueReceive(g_result_queue, &event, 0);
  return event;
}

/* ---- EXTI 中断回调（覆盖 HAL 弱函数） ---- */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
  if (GPIO_Pin != KEY_Pin) return;

  BaseType_t xHigher = pdFALSE;
  key_isr_msg_t msg;
  msg.tick = xTaskGetTickCountFromISR();

  if (HAL_GPIO_ReadPin(KEY_GPIO_Port, KEY_Pin) == GPIO_PIN_RESET) {
    msg.edge = KEY_EDGE_DOWN;
    EXTI->FTSR &= ~KEY_Pin;
    EXTI->RTSR |=  KEY_Pin;
  } else {
    msg.edge = KEY_EDGE_UP;
    EXTI->RTSR &= ~KEY_Pin;
    EXTI->FTSR |=  KEY_Pin;
  }

  xQueueSendFromISR(g_isr_queue, &msg, &xHigher);
  portYIELD_FROM_ISR(xHigher);
}
