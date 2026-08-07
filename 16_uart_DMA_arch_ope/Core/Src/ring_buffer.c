#include "ring_buffer.h"
#include <stdint.h>

void ring_buffer_init(ring_buffer_t *rb, ring_buffer_data_t *data, uint8_t size)
{
    rb->data = data;
    rb->head = 0;
    rb->tail = 0;
    rb->size = size;
}
/* 塞一个字节到缓冲区 */
void ring_buffer_put(ring_buffer_t *rb, ring_buffer_data_t data)
{
    rb->data[rb->head++] = data;
    if (rb->head == rb->size)
    {
        rb->head = 0;
    }
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
        if (++rb->head == rb->size)
        {
            rb->head = 0;
        }
    }
    return 1;
}

int ring_buffer_get(ring_buffer_t *rb, ring_buffer_data_t *data)
{
    if (!ring_buffer_is_empty(rb))
    {
        *data = rb->data[rb->tail++];
        if (rb->tail == rb->size)
        {
            rb->tail = 0;
        }
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