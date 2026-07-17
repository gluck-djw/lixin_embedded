#include "proto.h"
#include "elog.h"

static FrameState g_state = FRAME_NOT_DETECTED;
static uint32_t g_xor = 0;

void frame_parse(ring_buffer_t *rb)
{
    uint8_t byte;

    while (ring_buffer_get(rb, &byte))
    {
        switch (g_state)
        {
        case FRAME_NOT_DETECTED:
            if (byte == FRAME_HEAD_FLAG)
            {
                g_state = FRAME_HEAD;
                g_xor = 0;
                log_i("Frame head detected");
            }
            break;

        case FRAME_HEAD:
            if (byte == FRAME_END_FLAG)
            {
                if (g_xor == 0)
                    log_i("Checksum ok");
                else
                    log_e("Checksum error");
                g_state = FRAME_END;
            }
            else
            {
                g_xor ^= byte;
                log_i("Frame data:%d", byte);
            }
            break;
        case FRAME_END:
            g_state = FRAME_NOT_DETECTED;
            log_i("Frame end detected");
            break;
        }
    }
}
