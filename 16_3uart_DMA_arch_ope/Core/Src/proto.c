#include "proto.h"
#include "elog.h"

static FrameState g_state = FRAME_NOT_DETECTED;
static uint32_t g_xor = 0;
static uint8_t g_cmd = 0;
static uint8_t g_len = 0;
static uint8_t g_cnt = 0;
static uint8_t g_first = 0; // 1=下一个字节是len(跳过), 0=正常
static app_msg_t g_msg;

app_subscribe_t app_subscribers[MAX_SUBSCRIBERS];

void frame_parse_transfer(ring_buffer_t *rb)
{
    uint8_t byte;

    while (ring_buffer_get(rb, &byte))
    {
        switch (g_state)
        {
        case FRAME_NOT_DETECTED:
            if (byte == FRAME_HEAD_FLAG)
            {
                g_xor = 0xFE;
                g_cnt = 0;
                g_state = FRAME_HEAD;
                log_i("Frame head detected");
            }
            break;

        case FRAME_HEAD:
            g_cmd = byte;
            g_len = ring_buffer_peek(rb, 0);
            g_xor ^= g_cmd;
            g_cnt = 0;
            g_first = 1; // 标记下一个字节是len，跳过收集
            log_i("Frame cmd:%d, len:%d", g_cmd, g_len);
            g_state = FRAME_RECIVING;
            break;

        case FRAME_RECIVING:
            if (byte == FRAME_END_FLAG)
            {
                if (g_xor == 0)
                {
                    g_msg.data_cnt = g_cnt;
                    for (int i = 0; i < MAX_SUBSCRIBERS; i++)
                    {
                        for (int j = 0; j < MAX_SUBSCRIBERS_cmd; j++)
                        {
                            if (app_subscribers[i].cmd[j] == g_cmd)
                                xQueueSend(app_subscribers[i].rx_queue, &g_msg, 0);
                        }
                    }
                    log_i("Checksum ok");
                }
                else
                {
                    log_e("Checksum error");
                }
                g_state = FRAME_NOT_DETECTED; // 帧结束，直接回到等待状态
            }
            else
            {
                g_xor ^= byte;
                if (g_first)
                {
                    g_first = 0; // len字节，只XOR不收集
                }
                else if (g_cnt < g_len)
                {
                    g_msg.data[g_cnt++] = byte;
                    log_i("Frame data:%d", byte);
                }
                /* else: chk字节，只XOR不收集 */
            }
            break;
        }
    }
}
