#ifndef __RING_BUFFER_H__
#define __RING_BUFFER_H__

#include <stdint.h>
#include <stddef.h>

typedef uint8_t ring_buffer_data_t;

typedef struct ring_buffer
{
    ring_buffer_data_t *data;
    size_t head;
    size_t tail;
    size_t size;
} ring_buffer_t;

void ring_buffer_init(ring_buffer_t *rb, ring_buffer_data_t *data, uint8_t size);
void ring_buffer_put(ring_buffer_t *rb, ring_buffer_data_t data);
int ring_buffer_put_batch(ring_buffer_t *rb, ring_buffer_data_t *data, uint8_t size);

int ring_buffer_get(ring_buffer_t *rb, ring_buffer_data_t *data);

int ring_buffer_is_empty(ring_buffer_t *rb);

int ring_buffer_is_full(ring_buffer_t *rb);

#endif /* __RING_BUFFER_H__ */