#include "ring_buffer.h"
#include <stdint.h>
#include <string.h>

void ring_buffer_init(ring_buffer_t *rb)
{
    memset(rb->data, 0, sizeof(rb->data));
    rb->head = 0;
    rb->tail = 0;
    rb->size = RING_BUF_SIZE;
}

/* 塞一个字节到缓冲区 */
void ring_buffer_put(ring_buffer_t *rb, ring_buffer_data_t data)
{
    rb->data[rb->head] = data;
    rb->head = (rb->head + 1) % rb->size;
}

/*塞一批字节到缓冲区*/
int ring_buffer_put_batch(ring_buffer_t *rb, ring_buffer_data_t *data, uint8_t size)
{
    for (size_t i = 0; i < size; i++)
    {
        if (ring_buffer_is_full(rb))
        {
            return 0;
        }
        rb->data[rb->head] = data[i];
        rb->head = (rb->head + 1) % rb->size;
    }
    return 1;
}

int ring_buffer_get(ring_buffer_t *rb, ring_buffer_data_t *data)
{
    if (!ring_buffer_is_empty(rb))
    {
        *data = rb->data[rb->tail];
        rb->tail = (rb->tail + 1) % rb->size;
        return 1;
    }
    return 0;
}

int ring_buffer_is_empty(ring_buffer_t *rb)
{
    return rb->head == rb->tail;
}

int ring_buffer_is_full(ring_buffer_t *rb)
{
    return (rb->head + 1) % rb->size == rb->tail;
}

/**
 * @brief get_the_head_pos.
 *
 *
 * @param[in] circular_buffer_t : Pointer to the target of handler.
 * @param[in] head : Pointer to the head storage varibale.
* @return      uint8_t :
                        0xff:error, the buffer pointer is NULL;
                        0xfe:error, the buffer is empty;
                        0x00:success
                        0x01:failed
 *
 * */
uint8_t ring_buffer_get_head(ring_buffer_t *rb, uint32_t *head)
{
    if (NULL == rb)
    {
        return 0xfe;
    }
    *head = rb->head;
    return 0x00;
}


uint8_t ring_buffer_move_head(ring_buffer_t *rb, uint32_t move_pos)
{
    if (NULL == rb)
    {
        return 0xfe;
    }
    rb->head = (rb->head + move_pos) % rb->size;
    return 0x00;
}

uint8_t ring_buffer_peek(ring_buffer_t *rb, uint32_t offset)
{
    if (NULL == rb)
        return 0;
    return rb->data[(rb->tail + offset) % rb->size];
}