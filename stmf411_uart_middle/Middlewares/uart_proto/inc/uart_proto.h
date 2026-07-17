/**
 * @file uart_proto.h
 * @brief UART Protocol Layer Header File
 *
 * This header defines the data structures, function interfaces, and configuration
 * macros for the UART communication protocol layer. It encapsulates frame parsing,
 * UART hardware operations, OS interface abstraction, and subscription-based
 * function code callback management. The layer is designed to be hardware-agnostic
 * and OS-compatible through abstracted operation interfaces.
 */
#ifndef __UART_PROTO_H__
#define __UART_PROTO_H__

#include <stdint.h>
#include <string.h>

/**
 * @defgroup UART_PROTO_CONFIG Configuration Macros
 * @brief Core configuration parameters for UART protocol layer (thread, parsing, OS behavior)
 * @{
 */
#define NUM_NOTIFY_ISR_CB_CALL       3           /* Number of concurrent/sequential calls to notify_isr_cb */
#define MAX_PARSE_NUM_ONCE_TRIGGER   10          /* Maximum number of frames parsed per ISR trigger */

#define PARSE_THREAD_PRIORITY        10          /* Priority level of the frame parsing thread */
#define PARSE_THREAD_STACK_DEPTH     0x200       /* Stack size allocated for the parsing thread */
#define OS_DELAY_MAX                 0xFFFFFFFF  /* Maximum OS delay value */
/** @} */

/**
 * @defgroup UART_PROTO_DEBUG Debug Macro
 * @brief Debug log output configuration (using SEGGER RTT)
 * @{
 */
#include "SEGGER_RTT.h"
extern int SEGGER_RTT_printf(unsigned BufferIndex, const char *sFormat, ...);
#define UP_DEBUG_OUT(fmt, ...)      SEGGER_RTT_printf(0, fmt, ##__VA_ARGS__)  /* Output log to RTT buffer 0 */
#define UP_DEBUG_RED(fmt, ...)      SEGGER_RTT_printf(0, RTT_CTRL_TEXT_BRIGHT_RED fmt RTT_CTRL_RESET, ##__VA_ARGS__)
#define UP_DEBUG_GREEN(fmt, ...)    SEGGER_RTT_printf(0, RTT_CTRL_TEXT_BRIGHT_GREEN fmt RTT_CTRL_RESET, ##__VA_ARGS__)
#define UP_DEBUG_YELLOW(fmt, ...)   SEGGER_RTT_printf(0, RTT_CTRL_TEXT_BRIGHT_YELLOW fmt RTT_CTRL_RESET, ##__VA_ARGS__)
#define UP_DEBUG_BLUE(fmt, ...)     SEGGER_RTT_printf(0, RTT_CTRL_TEXT_BRIGHT_CYAN fmt RTT_CTRL_RESET, ##__VA_ARGS__)
#define UP_DEBUG_ISR(fmt, ...)      //SEGGER_RTT_printf(0, fmt, ##__VA_ARGS__)  

#include "SEGGER_SYSVIEW_FreeRTOS.h"
#define UP_TRACE_ISR_ENTER()        //traceISR_ENTER()
#define UP_TRACE_ISR_EXTI()         //traceISR_EXIT()
/** @} */

/**
 * @defgroup UART_PROTO_S
 * TATUS Status Codes
 * @brief Return status codes for UART protocol operations
 * @{
 */
typedef enum
{
    UART_PROTO_OK = 0,                      /* Operation successful */
    UART_PROTO_ERR_PARAM_INVALID,           /* Invalid input parameter */
    UART_PROTO_ERR_HANDLER_NOT_READY,       /* Protocol handler not initialized */
    UART_PROTO_ERR_OTHERS                   /* Other unspecified errors */
} uart_proto_status_t;
/** @} */

/**
 * @brief Frame parsing algorithm status codes
 *
 * Return values for the frame parsing function, indicating parsing progress or error type.
 */
typedef enum
{
    ALGO_ING = 1,                     /* Parsing in progress (insufficient data for a complete frame) */
    ALGO_OK = 0,                      /* Parsing successful (complete valid frame extracted) */
    ALGO_ERR_LENGTH_INVALID = -1,     /* Error: Invalid input data length  */
    ALGO_ERR_CRC = -2,                /* Error: CRC check failed (frame data corrupted or invalid) */
    ALGO_ERR_NOICE = -3,              /* Error: noice detected */
    ALGO_ERR_OTHERS = -4              /* Error: Other parsing failures  */
} algo_status_t;

/**
 * @defgroup UART_PROTO_DATA_STRUCT Data Structures
 * @brief Core data structures for UART protocol layer
 * @{
 */

/**
 * @brief Receive buffer configuration structure
 *
 * Stores the base address and size of the UART receive buffer,
 * used for data storage before frame parsing.
 */
typedef struct
{
    uint8_t     *recv_buf;       /* Pointer to the receive buffer */
    uint16_t    buffer_size;     /* Total size of the receive buffer (in bytes) */
} recv_buf_t;

/**
 * @brief Frame information structure
 *
 * Parsed metadata of the UART frame, including function code and length
 * information of pre-payload, payload, and post-payload segments.
 */
typedef struct
{
    uint16_t    pre_payload_len;  /* Length of data before the payload (in bytes) */
    uint16_t    post_payload_len; /* Length of data after the payload (in bytes) */
    uint16_t    payload_len;      /* Length of the payload data (in bytes) */
    uint8_t     fun_code;         /* Function code of the frame */
} frame_info_t;

/**
 * @brief Frame parsing algorithm interface
 *
 * Abstract interface for frame parsing logic, enabling flexible replacement of parsing implementations
 * (e.g., adapting to different frame formats, protocols, or checksum mechanisms).
 * The interface decouples the protocol layer from specific parsing logic.
 */
typedef struct
{
    /**
     * @brief Parse function to extract frame metadata from raw received data
     * @param[in]  p_data     Pointer to the raw received data buffer (linearized ring buffer data)
     * @param[in]  data_len   Length of the raw data available for parsing (in bytes)
     * @param[out] frame_info Pointer to the frame_info_t structure to store parsed results
     *                        (populated with pre_payload_len, payload_len, fun_code, etc. on success)
     * @retval ALGO_OK        Parsing successful: a complete valid frame is extracted
     * @retval ALGO_ING       Parsing in progress: insufficient data to form a complete frame (need more data)
     * @retval ALGO_ERR_xxx   Parsing failed: specific error type (length invalid, CRC error, etc.)
     */
    algo_status_t (*pf_parse_funcode)(uint8_t *const p_data,
                                      uint16_t data_len,
                                      frame_info_t *const frame_info);
} parse_algo_t;

/**
 * @brief Frame parsing attribute structure
 *
 * Combines the receive buffer configuration and parsing algorithm,
 * providing necessary resources for frame parsing.
 */
typedef struct
{
    recv_buf_t              *recv_buf;    /* Pointer to receive buffer configuration */
    parse_algo_t            *parse_algo;  /* Pointer to frame parsing algorithm interface */
} frame_parse_att_t;

/**
 * @brief UART hardware operation interface
 *
 * Abstract interface for UART hardware operations, decoupling the protocol layer
 * from specific hardware implementations (e.g., STM32 HAL, custom drivers).
 * This layer does not handle UART hardware error processing.
 */
typedef struct
{
    void (*pf_uart_init)(void);                          /* Initialize UART hardware */
    void (*pf_uart_deinit)(void);                        /* Deinitialize UART hardware */
    void (*pf_uart_write)(uint8_t *const p_data, uint16_t len); /* Transmit data via UART (reserved, not use in fact)*/
    uint16_t (*pf_get_counter)(void);                    /* Get UART DMA receive counter (remaining bytes) */
    void (*pf_set_counter)(uint16_t counter);            /* Set UART DMA receive counter */
} uart_ops_t;

/**
 * @brief OS interface abstraction
 *
 * Abstract interface for OS-related operations, supporting integration with
 * different RTOS (e.g., FreeRTOS, RT-Thread).
 * This layer does not handle OS error processing.
 */
typedef struct
{
    /**
     * @brief Create an OS thread
     * @param[in]  task_code        Pointer to the thread entry function
     * @param[in]  task_name        Name of the thread (for debugging)
     * @param[in]  stack_depth      Stack size of the thread (in bytes/words, depends on OS)
     * @param[in]  parameters       Pointer to thread parameters
     * @param[in]  priority         Thread priority (OS-specific)
     * @param[out] thread_handle    Pointer to store the created thread handle
     */
    void (*pf_os_thread_create)(void (*task_code)(void*),
                                const char *const task_name,
                                const uint32_t stack_depth,
                                void *const parameters,
                                uint32_t priority,
                                void **const thread_handle);

    void (*pf_os_thread_delete)(void *const thread_handle); /* Delete an OS thread */

    /**
     * @brief Create an OS message queue
     * @param[in]  item_num         Maximum number of items the queue can hold
     * @param[in]  item_size        Size of each item in the queue (in bytes)
     * @param[out] queue_handle     Pointer to store the created queue handle
     */
    void (*pf_os_queue_create)(uint32_t const item_num,
                               uint32_t const item_size,
                               void **const queue_handle);

    /**
     * @brief Put an item into the queue from ISR context
     * @param[in]  queue_handle     Queue handle
     * @param[in]  item             Pointer to the item to be put into the queue
     * @param[out] HigherPriorityTaskWoken Flag indicating if a higher-priority task is woken (OS-specific)
     */
    void (*pf_os_queue_put_isr)(void *const queue_handle,
                                void *const item,
                                long *const HigherPriorityTaskWoken);

    /**
     * @brief Get an item from the queue (blocking)
     * @param[in]  queue_handle     Queue handle
     * @param[out] item             Pointer to store the received item
     * @param[in]  timeout          Maximum wait time (OS-specific units, e.g., ticks)
     */
    void (*pf_os_queue_get)(void *const queue_handle,
                            void *const item,
                            uint32_t const timeout);

    void (*pf_os_enter_critical)(void);  /* Enter critical section */
    void (*pf_os_exit_critical)(void);   /* Exit critical section */

    uint32_t (*pf_os_enter_critical_isr)(void);  /* Enter critical section */
    void (*pf_os_exit_critical_isr)(uint32_t primask);   /* Exit critical section */

    void* (*pf_os_malloc)(uint8_t data_size); /* Allocate memory (OS-managed) */
    void (*pf_os_free)(void *ptr);       /* Free allocated memory */
} os_interface_t;

/**
 * @brief UART protocol initialization input arguments
 *
 * Aggregates all required configuration and interface pointers for
 * initializing the UART protocol layer.
 */
typedef struct
{
    frame_parse_att_t   *frame_parse_att;  /* Pointer to frame parsing attributes */
    uart_ops_t          *uart_ops;         /* Pointer to UART hardware operation interface */
    os_interface_t      *os_interface;     /* Pointer to OS abstraction interface */
} uart_proto_input_arg_t;

/**
 * @brief Function code callback type
 *
 * Callback function prototype for handling specific frame function codes.
 * Invoked when a frame with the subscribed function code is successfully parsed.
 * @param[in] arg         Pointer to user-defined argument (passed during subscription)
 * @param[in] payload     Pointer to the frame payload data
 * @param[in] payload_len Length of the payload data (in bytes)
 */
typedef void (*pf_fun_code_cb_t)(void *arg,
                                 uint8_t *const payload,
                                 uint16_t payload_len);

/**
 * @brief Subscription parameter structure
 *
 * Stores the function code, user argument, and callback function for
 * subscribing to specific frame processing.
 */
typedef struct
{
    uint8_t fun_code;                /* Function code to subscribe to */
    void *arg;                       /* User-defined argument passed to the callback */
    pf_fun_code_cb_t cb;             /* Callback function for the subscribed function code */
} subscribe_para_t;

/** Forward declaration of private data structure (hides internal implementation) */
typedef struct uart_proto_priv_data uart_proto_priv_data_t;

/**
 * @brief UART protocol main handle structure
 *
 * Maintains the state, configuration, and private data of the UART protocol layer.
 * Provides user-facing API functions for subscription management.
 */
typedef struct uart_proto
{
    uart_proto_input_arg_t  *uart_proto_input_arg;  /* Pointer to initialization input arguments */
    uart_proto_priv_data_t  *uart_proto_priv_data;  /* Pointer to private internal data (opaque type) */

    /**
     * @brief Subscribe to a specific function code
     * @param[in]  self            Pointer to the uart_proto handle
     * @param[in]  subscribe_para  Pointer to subscription parameters (fun_code, cb, arg)
     * @param[out] handle          Pointer to store the subscription handle (for unsubscription)
     * @retval UART_PROTO_OK       Subscription successful
     * @retval Non-zero            Subscription failed (status code)
     */
    uart_proto_status_t (*pf_subscribe)(struct uart_proto *const self,
                                        subscribe_para_t *const subscribe_para,
                                        void **const handle);

    /**
     * @brief Unsubscribe from a function code
     * @param[in] self             Pointer to the uart_proto handle
     * @param[in] handle           Subscription handle (obtained from pf_subscribe)
     * @retval UART_PROTO_OK       Unsubscription successful
     * @retval Non-zero            Unsubscription failed (status code)
     */
    uart_proto_status_t (*pf_unsubscribe)(struct uart_proto *const self,
                                          void *const handle);
} uart_proto_t;
/** @} */

/**
 * @defgroup UART_PROTO_PUBLIC_API Public API Functions
 * @brief Public interface functions for UART protocol layer
 * @{
 */

/**
 * @brief Initialize the UART protocol layer
 * @param[in] self             Pointer to the uart_proto handle (to be initialized)
 * @param[in] p_input_args     Pointer to initialization input arguments
 * @retval UART_PROTO_OK       Initialization successful
 * @retval Non-zero            Initialization failed (status code)
 */
uart_proto_status_t uart_proto_inst(uart_proto_t *const self,
                                    uart_proto_input_arg_t *const p_input_args);

/**
 * @brief ISR callback to notify protocol layer of UART receive events
 *
 * Invoked from UART/DMA interrupt context (e.g., IDLE, half-transfer, full-transfer)
 * to trigger frame parsing. Must be called with interrupts properly managed.
 * @param[in] self             Pointer to the uart_proto handle
 */
void notify_isr_cb(uart_proto_t *const self);

/** @} */

#endif // __UART_PROTO_H__
