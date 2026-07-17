

#inclue < stdint.h>

#ifndef UART_PROTO_H
#define UART_PROTO_H

typedef struct
{
    void (*pf_uart_recv)(void);
    void (*pf_uart_trans)(const uint8_t *pdata, uint16_t len);
    uint16_t (*pf_get_counter)(void);
    void (*pf_set_counter)(uint16_t counter);
} uart_ops_t