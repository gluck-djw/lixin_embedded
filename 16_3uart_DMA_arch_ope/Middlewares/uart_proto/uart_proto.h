#ifndef __UART_PROTO_H__
#define __UART_PROTO_H__

#include <stdint.h>
#include <string.h>

#define PRIV(p) p->status;

#define INPUT_ARG(p) p->input_arg;
#define OS_INTERFACE(p) p->input_arg->os_interface;
#define UART_IINTERFACE(p) p->input_arg->uart_ops;
#define FRAME_INTERFACE(p) p->input_arg->input_arg->frame_parse_att;
#define RECV_BUF(p) FRAME_INTERFACE(p)->recv_buf;
#define RECV_BUF_SIZE(p) RECV_BUF(p)->buf_size;
#define PARSE_ALG(p) FRAME_INTERFACE(p)->parse_alg->pf_parse_funcode;

#define NON_COPY_WITH_NO_WRAP
typedef enum
{
    UART_PROTO_OK = 0,
    UART_PROTO_ERROR_PARAM_INVALID,
    UART_PROTO_ERR_HANDLER_NOT_READY,
    UART_PROTO_ERR_OTHERS
} uart_proto_status_t;

typedef enum
{
    algo_ing = 1,
    ALGO_OK = 0,
    ALGO_ERROR_LENGTH_INVALID = -1,
    ALGO_ERR_CRC = -2,
    ALGO_ERR_NOICE = -3,
    ALGO_ERR_OTHERS = -4
} alg_status_t;

typedef struct
{
    uint16_t pre_payload_len;
    uint16_t post_payload_len;
    uint16_t payload_len;
    uint8_t fun_code;
} frame_info_t;

/* parsed frame info */
typedef struct
{
    uint8_t funcode;
    uint8_t *payload;
    uint16_t payload_len;

} parse_info_t;
/**
 * @brief Frame parsing algorithm interface
 *
 * Abstract interface for frame parsing logic, enabling flexible replacement of parsing implementations
 * (e.g., adapting to different frame formats, protocols, or checksum mechanisms).
 * The interface decouples the protocol layer from specific parsing logic.
 */
typedef struct
{
    alg_status_t (*pf_parse_funcode)(const uint8_t *p_data,
                                     uint16_t data_len,
                                     frame_info_t *frame_info);
} parse_alg_t;

typedef struct
{
    uint8_t *buffer;
    uint16_t buf_size;
} recv_buf_t;

typedef struct
{
    recv_buf_t *recv_buf;
    parse_alg_t *parse_alg;
} frame_parse_att_t;

typedef struct
{
    frame_parse_att_t *frame_parse_att;
    uart_ops_t *uart_ops;
    os_interface_t *os_interface;

} uart_proto_input_arg_t;

typedef struct
{
    void (*pf_uart_recv)(void);
    void (*pf_uart_trans)(const uint8_t *pdata, uint16_t len);
    uint16_t (*pf_get_counter)(void);
    void (*pf_set_counter)(uint16_t counter);
} uart_ops_t;

/* OS interface abstraction
Abstract interface for OS-related operations, supporting integration with different RTOS
*/
typedef struct
{
    /* create an os thread
    task_func: function pointer to the thread entry function
    task_name: name of the thread
    stack_depth: stack size of the thread
    parameters: pointer to the parameters passed to the thread entry function
    */
    void (*pf_os_thread_create)(void *task_func(void *),
                                const char *const taks_name,
                                const uint32_t stack_depth,
                                void const *parameters,
                                uint32_t priority,
                                void **const thread_handle);

    void (*pf_os_thread_delete)(void const *thread_handle);

    /* create an os message queue */
    void (*pf_os_queue_create)(uint32_t const item_num,
                               uint32_t const item_size,
                               void **const queue_handle);

    /* put item into the queue  */
    void (*pf_os_queue_put_isr)(const void *queue_handle,
                                const void *pvitem,
                                const void *pxHigherPriorityTaskWoken);

    /* get item from the queue */
    void (*pf_os_queue_get)(const void *queue_handle,
                            void *const buffer,
                            const uint32_t timeout);

    void (*pf_os_enter_critical)(void);
    void (*pf_os_exit_critical)(void);

    uint32_t (*pf_os_enter_critical_from_isr)(void);
    void (*pf_os_exit_critical_from_isr)(uint32_t BASEPRI);

    void* (*pf_os_malloc)(uint8_t data_size);
    void (*pf_os_free)(void *pv);

} os_interface_t;

typedef struct
{
    uint8_t fun_code;
    void *arg; // argument for the callback function
    pf_fun_code_cb_t cb;
} subscribe_para_t;

typedef struct
{
    bool if_inited;
    volatile uint16_t parse_fail_cnt;
    void *queue_handle;                 // parse frame info
    uint32_t head;                      // total bytes read from the receive buffer
    uint32_t tail;                      // total bytes parsed
    uint8_t *parse_buf;                 // buffer for parsing frame
    volatile uint8_t pending_que_cnt; // pending frame count
    volatile uint32_t data_counter;     // DMA recive data counter

} uart_proto_priv_data_t;

typedef void (*pf_fun_code_cb_t)(const uint8_t *payload, uint16_t payload_len, void *cb_arg);

typedef struct uart_proto
{

    uart_proto_input_arg_t *input_arg;
    uart_proto_priv_data_t *status;

    uart_proto_status_t (*pf_subscribe)(const struct uart_proto *self,
                                        const subscribe_para_t *subscribe_para,
                                        void **const handle);
    uart_proto_status_t (*pf_unsubscribe)(const struct uart_proto *self,
                                          void **const handle);
} uart_proto_t;

/*
/**
 * @brief ISR callback to notify protocol layer of UART receive events
 *
 * Invoked from UART/DMA interrupt context (e.g., IDLE, half-transfer, full-transfer) to trigger frame parsing.
 * * @param[in] self             Pointer to the uart_proto handle
*/
void notify_isr_cb(const uart_proto_t *self);