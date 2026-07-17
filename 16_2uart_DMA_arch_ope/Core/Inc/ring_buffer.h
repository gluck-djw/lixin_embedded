#ifndef __RING_BUFFER_H__
#define __RING_BUFFER_H__

#include <stdint.h>
#include <stddef.h>
#define RING_BUF_SIZE 64
typedef uint8_t ring_buffer_data_t;

typedef struct ring_buffer
{
    ring_buffer_data_t data[RING_BUF_SIZE];
    size_t head;
    size_t tail;
    size_t size;
} ring_buffer_t;

void ring_buffer_init(ring_buffer_t *rb);
void ring_buffer_put(ring_buffer_t *rb, ring_buffer_data_t data);
int ring_buffer_put_batch(ring_buffer_t *rb, ring_buffer_data_t *data, uint8_t size);

int ring_buffer_get(ring_buffer_t *rb, ring_buffer_data_t *data);

int ring_buffer_is_empty(ring_buffer_t *rb);

int ring_buffer_is_full(ring_buffer_t *rb);

uint8_t ring_buffer_get_head(ring_buffer_t *rb, size_t *head_pos);
uint8_t ring_buffer_move_head(ring_buffer_t *rb, uint32_t move_pos);

uint8_t ring_buffer_peek(ring_buffer_t *rb, uint32_t offset);
#endif /* __RING_BUFFER_H__ */