#ifndef __PROTO_H__
#define __PROTO_H__

#include "ring_buffer.h"
#include "FreeRTOS.h"
#include "queue.h"

/* 帧定义 */
#define FRAME_HEAD_FLAG 0xFE
#define FRAME_END_FLAG 0xFF

#define PROTO_MAX_DATA_LEN 32
#define MAX_SUBSCRIBERS 3
#define MAX_SUBSCRIBERS_cmd 10

/* 通知信号 */
#define IRQ_SEND_TO_THREAD 0x01
#define FRONT_SEND_TO_END 0x02

/* 状态机 */
typedef enum
{
    FRAME_NOT_DETECTED = 0,
    FRAME_HEAD = 1,
    FRAME_END = 2,
    FRAME_RECIVING = 3,
} FrameState;

typedef struct
{
    uint8_t data_cnt;
    uint32_t data[PROTO_MAX_DATA_LEN]; // 数据
} app_msg_t;

// 定义订阅者
typedef struct
{
    uint8_t cmd[MAX_SUBSCRIBERS_cmd];
    QueueHandle_t rx_queue; // 接收的队列
} app_subscribe_t;

void frame_parse_transfer(ring_buffer_t *rb);

// app_subscribe_t app_subscribers[MAX_SUBSCRIBERS];
#endif
