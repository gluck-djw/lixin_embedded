#ifndef __PROTO_H__
#define __PROTO_H__

#include "ring_buffer.h"

/* 帧定义 */
#define FRAME_HEAD_FLAG 0xFE
#define FRAME_END_FLAG 0xFF

/* 状态机 */
typedef enum
{
    FRAME_NOT_DETECTED = 0,
    FRAME_HEAD = 1,
    FRAME_END = 2,
} FrameState;

/* 通知信号 */
#define IRQ_SEND_TO_THREAD 0x01
#define FRONT_SEND_TO_END 0x02

void frame_parse(ring_buffer_t *rb);

#endif
