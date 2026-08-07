/**
 * @file uart_proto.c
 * @brief UART Protocol Layer Implementation
 *
 * This file implements the core logic of the UART protocol layer, including:
 * - Protocol initialization (hardware/OS interface binding, resource allocation)
 * - Frame parsing thread management (RTOS-based asynchronous parsing)
 * - Subscription/unsubscription for function code callbacks
 * - Ring buffer data handling (UART DMA receive buffer adaptation)
 * - ISR notification processing and data queuing
 *
 * The layer uses abstracted UART and OS interfaces to ensure hardware/OS agnosticity,
 * supports function code-based callback subscription, and processes frames asynchronously
 * via RTOS queue to avoid blocking ISR context.
 */

#include "uart_proto.h"
#include <stdbool.h>
#include "t_list.h"

/**
 * @defgroup UART_PROTO_MACROS Helper Macros
 * @brief Abbreviated macros for accessing nested structure members
 * @note Improves code readability by reducing redundant member access chains
 * @{
 */
#define OS_INTERFACE(p)     (p)->uart_proto_input_arg->os_interface          /* OS abstraction interface */
#define UART_INTERFACE(p)   (p)->uart_proto_input_arg->uart_ops               /* UART hardware interface */
#define PARSE_INTERFACE(p)  (p)->uart_proto_input_arg->frame_parse_att        /* Frame parsing attributes */
#define PRIV_DATA(p)        (p)->uart_proto_priv_data                         /* Private internal data */

#define PARSE_ALGO(p)       PARSE_INTERFACE(p)->parse_algo->pf_parse_funcode  /* Frame parsing function */
#define RECV_BUF_ADDR(p)    PARSE_INTERFACE(p)->recv_buf->recv_buf            /* UART receive buffer base address */
#define RECV_BUF_SIZE(p)    PARSE_INTERFACE(p)->recv_buf->buffer_size         /* UART receive buffer total size */

#define NON_COPY_WHEN_NON_WRAP

/** @} */

/**
 * @defgroup UART_PROTO_PRIVATE_TYPES Private Data Types
 * @brief Internal data structures for protocol layer management
 * @{
 */

/**
 * @brief UART protocol private data structure (opaque type)
 *
 * Stores internal state, resource handles, and buffer management variables.
 * Hidden from user to encapsulate implementation details.
 */
typedef struct uart_proto_priv_data
{
    bool                if_inited;         /* Initialization flag (true = initialized) */
    volatile uint16_t   parse_fail_cnt;
    void                *queue_handle;     /* RTOS queue handle for parsed frame info */
    volatile uint16_t   data_counter;      /* DMA receive counter cache (remaining bytes) */
    volatile uint32_t   header;            /* Ring buffer read pointer (total bytes read) */
    volatile uint32_t   tail;              /* Ring buffer write pointer (total bytes parsed) */
    uint8_t             *parse_buf;        /* Temporary buffer for frame parsing (linearized ring data) */
    volatile uint8_t    payload_cnt;       /* Counter for pending frames in queue */
} uart_proto_priv_data_t;

/**
 * @brief Parsed frame information structure
 *
 * Stores extracted function code, payload data, and payload length.
 * Used for inter-thread communication via RTOS queue.
 */
typedef struct
{
    uint8_t     fun_code;         /* Function code of the parsed frame */
    uint8_t     *payload;         /* Pointer to the frame payload data */
    uint16_t    payload_len;      /* Length of the payload data (in bytes) */
} parse_info_t;

/**
 * @brief Function code callback node structure
 *
 * Linked list node for managing subscribed function code callbacks.
 * Stored in a sorted linked list for efficient lookup.
 */
typedef struct
{
    t_list_t            list;             /* Linked list node (for t_list library) */
    uint8_t             fun_code;         /* Function code to subscribe to */
    void                *arg;             /* User-defined argument for callback */
    pf_fun_code_cb_t    pf_fun_code_cb;   /* Callback function for the function code */
} funcode_node_t;
/** @} */

/**
 * @defgroup UART_PROTO_PRIVATE_GLOBALS Private Global Variables
 * @brief Internal global resources for the protocol layer
 * @{
 */
static t_list_t funcode_sentinel =        /* Sentinel node for function code callback linked list */
{
    .next = &funcode_sentinel,
    .pre = &funcode_sentinel
};
/** @} */

/**
 * @defgroup UART_PROTO_PRIVATE_FUNCS Private Helper Functions
 * @brief Internal helper functions (not exposed to user)
 * @{
 */

/**
 * @brief Initialize a function code callback node
 * @param[in] self            Pointer to the uart_proto handle
 * @param[in] subscribe_para  Pointer to subscription parameters (fun_code, cb, arg)
 * @retval funcode_node_t*    Pointer to initialized node (NULL = initialization failed)
 * @note Allocates memory for the node via OS interface; initializes linked list entry
 */
static funcode_node_t* funcode_node_init(uart_proto_t *const self,
                                         subscribe_para_t *const subscribe_para)
{
    if (!self)
        return NULL;
    if (false == PRIV_DATA(self)->if_inited)
        return NULL;

    /* Allocate memory for callback node via OS-managed malloc */
    funcode_node_t *p_node = OS_INTERFACE(self)->pf_os_malloc(sizeof(funcode_node_t));
    if (!p_node)
        return NULL;

    t_list_init(&p_node->list);                  /* Initialize linked list node */
    p_node->arg = subscribe_para->arg;           /* Assign user argument */
    p_node->fun_code = subscribe_para->fun_code; /* Assign target function code */
    p_node->pf_fun_code_cb = subscribe_para->cb; /* Assign callback function */

    return p_node;
}

/**
 * @brief Subscribe to a specific function code (implementation of pf_subscribe)
 * @param[in]  self            Pointer to the uart_proto handle
 * @param[in]  subscribe_para  Pointer to subscription parameters
 * @param[out] handle          Pointer to store subscription handle (for unsubscription)
 * @retval UART_PROTO_STATUS   Operation status (UART_PROTO_OK = success)
 * @note Inserts callback node into sorted linked list (sorted by fun_code) for efficient lookup
 * @note Uses critical section to ensure thread safety during list modification
 */
static uart_proto_status_t subscribe_funcode(uart_proto_t *const self,
                                             subscribe_para_t *const subscribe_para,
                                             void **const handle)
{
    if (!self || !subscribe_para)
        return UART_PROTO_ERR_PARAM_INVALID;
    if (false == PRIV_DATA(self)->if_inited)
        return UART_PROTO_ERR_HANDLER_NOT_READY;

    /* Initialize callback node */
    funcode_node_t *funcode = funcode_node_init(self, subscribe_para);
    if (!funcode)
        return UART_PROTO_ERR_OTHERS;

    /* Enter critical section to protect linked list modification */
    OS_INTERFACE(self)->pf_os_enter_critical();

    /* Traverse sorted linked list to find insertion position (ascending order by fun_code) */
    t_list_t *cur = &funcode_sentinel;
    while (cur->next != &funcode_sentinel)
    {
        funcode_node_t *next_funcode = T_LIST_ENTRY(cur->next, funcode_node_t, list);
        if (next_funcode->fun_code > subscribe_para->fun_code)
            break; /* Found insertion point (maintain sorted order) */
        cur = cur->next;
    }

    t_list_insert_after(cur, &funcode->list); /* Insert node into linked list */
    OS_INTERFACE(self)->pf_os_exit_critical(); /* Exit critical section */
    if(!handle)
    	*handle = funcode; /* Return subscription handle to user */
    return UART_PROTO_OK;
}

/**
 * @brief Unsubscribe from a function code (implementation of pf_unsubscribe)
 * @param[in] self             Pointer to the uart_proto handle
 * @param[in] handle           Subscription handle (obtained from pf_subscribe)
 * @retval UART_PROTO_STATUS   Operation status (UART_PROTO_OK = success)
 * @note Removes callback node from linked list; thread-safe via critical section
 * @note Does NOT free node memory (user responsible for resource management if needed)
 */
static uart_proto_status_t unsubscribe_funcode(struct uart_proto *const self, void *const handle)
{
    if (!self || !handle)
        return UART_PROTO_ERR_PARAM_INVALID;
    if (false == PRIV_DATA(self)->if_inited)
        return UART_PROTO_ERR_HANDLER_NOT_READY;

    /* Enter critical section to protect linked list modification */
    OS_INTERFACE(self)->pf_os_enter_critical();

    funcode_node_t *funcode = (funcode_node_t *)handle;
    t_list_remove(&funcode->list); /* Remove node from linked list */

    OS_INTERFACE(self)->pf_os_exit_critical(); /* Exit critical section */
    /* Free node memory */
    OS_INTERFACE(self)->pf_os_free(handle);
    return UART_PROTO_OK;
}

/**
 * @brief Frame parsing thread (RTOS task)
 * @param[in] arg              Pointer to uart_proto handle (passed as thread parameter)
 * @note Runs in infinite loop: waits for parsed frame info from queue, then invokes matching callbacks
 * @note Asynchronous processing to avoid blocking ISR context; thread priority configurable via macros
 */
static void parse_thread(void *arg)
{
    uart_proto_t *self = (uart_proto_t *)arg;
    if (!self || !PRIV_DATA(self)->if_inited)
        return;

    while (1)
    {
        parse_info_t parse_info;
        /* Wait indefinitely for frame info from queue (blocking call) */
        OS_INTERFACE(self)->pf_os_queue_get(PRIV_DATA(self)->queue_handle,
                                            &parse_info,
                                            OS_DELAY_MAX);
        UP_DEBUG_YELLOW("parse in\r\n");
        PRIV_DATA(self)->payload_cnt--; /* Decrement pending frame counter */

        /* Traverse sorted linked list to find matching function code callbacks */
        t_list_t *cur = &funcode_sentinel;
        while (cur->next != &funcode_sentinel)
        {
            funcode_node_t *funcode = T_LIST_ENTRY(cur->next, funcode_node_t, list);

            if (funcode->fun_code > parse_info.fun_code)
                break; /* No more matches (list is sorted) */

            /* Invoke callback if function code matches */
            if (funcode->fun_code == parse_info.fun_code)
            {
                if (funcode->pf_fun_code_cb)
                {
                    funcode->pf_fun_code_cb(funcode->arg,
                                            parse_info.payload,
                                            parse_info.payload_len);
                }
            }

            cur = cur->next;
        }
    }
}
/** @} */

/**
 * @defgroup UART_PROTO_PUBLIC_FUNCS Public API Implementations
 * @brief Implementations of user-facing API functions
 * @{
 */

/**
 * @brief Initialize the UART protocol layer
 * @param[in] self             Pointer to the uart_proto handle (to be initialized)
 * @param[in] p_input_args     Pointer to initialization input arguments (interfaces + attributes)
 * @retval UART_PROTO_STATUS   Operation status (UART_PROTO_OK = success)
 * @note Performs:
 *       1. Input parameter validation (all required interfaces must be provided)
 *       2. Allocation of private data and parsing buffer via OS interface
 *       3. UART hardware initialization via UART interface
 *       4. Creation of RTOS queue and parsing thread
 *       5. Binding of public API functions (pf_subscribe/pf_unsubscribe)
 */
uart_proto_status_t uart_proto_inst(uart_proto_t *const self,
                                    uart_proto_input_arg_t *const p_input_args)
{
    /* Validate input parameters (all critical pointers must be non-NULL) */
    if (!self || !p_input_args || !p_input_args->frame_parse_att ||
        !p_input_args->os_interface || !p_input_args->uart_ops)
    {
        return UART_PROTO_ERR_PARAM_INVALID;
    }

    self->uart_proto_input_arg = p_input_args; /* Bind input arguments to protocol handle */

    /* Validate required OS interface functions */
    if (!OS_INTERFACE(self)->pf_os_malloc || !OS_INTERFACE(self)->pf_os_free)
    {
        return UART_PROTO_ERR_PARAM_INVALID;
    }

    /* Validate required UART interface functions */
    if (!UART_INTERFACE(self)->pf_get_counter ||
        !UART_INTERFACE(self)->pf_uart_deinit ||
        !UART_INTERFACE(self)->pf_uart_init ||
        !UART_INTERFACE(self)->pf_uart_write)
    {
        return UART_PROTO_ERR_PARAM_INVALID;
    }

    /* Validate required frame parsing attributes */
    if (!PARSE_INTERFACE(self)->parse_algo || !PARSE_INTERFACE(self)->recv_buf)
    {
        return UART_PROTO_ERR_PARAM_INVALID;
    }

    /* Allocate private data structure via OS-managed malloc */
    self->uart_proto_priv_data = OS_INTERFACE(self)->pf_os_malloc(sizeof(uart_proto_priv_data_t));
    if (!PRIV_DATA(self))
    {
        return UART_PROTO_ERR_OTHERS;
    }

    /* Initialize private data variables */
    PRIV_DATA(self)->data_counter = 0;
    PRIV_DATA(self)->header = 0;
    PRIV_DATA(self)->tail = 0;
    PRIV_DATA(self)->parse_fail_cnt = 0;
    /* overflow test */
    // PRIV_DATA(self)->header = 0xFFFFFFFF/RECV_BUF_SIZE(self) * RECV_BUF_SIZE(self);
    // PRIV_DATA(self)->tail = 0xFFFFFFFF/RECV_BUF_SIZE(self) * RECV_BUF_SIZE(self);
    PRIV_DATA(self)->if_inited = false;
    PRIV_DATA(self)->payload_cnt = 0;

    /* Allocate linear parsing buffer (same size as receive buffer for full ring buffer linearization) */
    PRIV_DATA(self)->parse_buf = OS_INTERFACE(self)->pf_os_malloc(RECV_BUF_SIZE(self));

    /* Initialize UART hardware via abstracted interface */
    UART_INTERFACE(self)->pf_uart_init();

    /* Create RTOS queue for parsed frame info (buffers up to MAX_PARSE_NUM_ONCE_TRIGGER frames) */
    OS_INTERFACE(self)->pf_os_queue_create(MAX_PARSE_NUM_ONCE_TRIGGER,
                                           sizeof(parse_info_t),
                                           &PRIV_DATA(self)->queue_handle);

    /* Create frame parsing thread via OS interface */
    OS_INTERFACE(self)->pf_os_thread_create(parse_thread,
                                            "parse_thread",
                                            PARSE_THREAD_STACK_DEPTH,
                                            self,
                                            PARSE_THREAD_PRIORITY,
                                            NULL);

    /* Bind public API functions to protocol handle */
    self->pf_subscribe = subscribe_funcode;
    self->pf_unsubscribe = unsubscribe_funcode;

    PRIV_DATA(self)->if_inited = true; /* Mark protocol layer as initialized */
    return UART_PROTO_OK;
}

static void reset_uart_state(uart_proto_t *const self)
{
    PRIV_DATA(self)->header = PRIV_DATA(self)->tail = PRIV_DATA(self)->data_counter = 0;
    UART_INTERFACE(self)->pf_set_counter(RECV_BUF_SIZE(self));
}

/**
 * @brief ISR callback to notify protocol layer of UART receive events
 * @param[in] self             Pointer to the uart_proto handle
 * @note Invoked from UART/DMA interrupt context (e.g., IDLE, half-transfer, full-transfer)
 * @note Performs:
 *       1. Validation of initialization state and input handle
 *       2. Enter ISR-safe critical section (protects shared private data)
 *       3. Calculate ring buffer pointers and pending data length
 *       4. Linearization of ring buffer data into parse_buf (handles wrap-around)
 *       5. Frame parsing via user-provided parse algorithm (loop for multi-frame support)
 *       6. Queuing of valid parsed frames to RTOS queue (async processing)
 *       7. Handle parsing errors: discard invalid data segments to avoid blocking
 *       8. Exit ISR-safe critical section
 * @note Uses ISR-specific critical section API to ensure thread safety for shared data
 * @note Runs in ISR context: keeps logic concise to avoid blocking interrupts
 */
void notify_isr_cb(uart_proto_t *const self)
{
    UP_TRACE_ISR_ENTER();
    /* Validate protocol handle and initialization state (early exit if invalid) */
    if (!self || !PRIV_DATA(self)->if_inited)
        return;
    
    /* Enter ISR-safe critical section: protects access to shared private data (header/tail/counter) */
    /* Preserves interrupt mask to restore later (compatible with ISR context) */
    uint32_t primask = OS_INTERFACE(self)->pf_os_enter_critical_isr();
    
    PRIV_DATA(self)->parse_fail_cnt ++;
    /* 
     * Calculate ring buffer pointers for pending data range:
     * - pre_parse_counter: Start index of unparsed data in receive buffer (tail mod buffer size)
     *   (tail tracks total parsed bytes; mod converts to 0~(buffer_size-1) index for wrap-around)
     * - end_counter: Current end index of received data in buffer (valid data up to this index-1)
     *   Calculation logic: DMA's pf_get_counter() returns "remaining untransferred bytes"
     *   So received data length = buffer_size - remaining_bytes ¡ú end index = received data length
     *   (applies to DMA Circular mode; ensures correct index even when buffer wraps around)
     */
    uint16_t pre_parse_counter = PRIV_DATA(self)->tail % RECV_BUF_SIZE(self);
    uint16_t end_counter = RECV_BUF_SIZE(self) - UART_INTERFACE(self)->pf_get_counter();        

    /* Calculate length of pending data to parse (ring buffer formula: handles wrap-around) */
    uint16_t parse_ring_len = (end_counter - pre_parse_counter + RECV_BUF_SIZE(self)) % RECV_BUF_SIZE(self);

    /* Update header (total bytes read from ring buffer: tracks all received data) */
    PRIV_DATA(self)->header += (end_counter - PRIV_DATA(self)->data_counter + RECV_BUF_SIZE(self)) % RECV_BUF_SIZE(self);

    /* Check for duplicate ISR triggers (header == tail means no new data to process) */
    if (PRIV_DATA(self)->header == PRIV_DATA(self)->tail)
    {
        // UP_DEBUG_BLUE("repeate enter\r\n");
        OS_INTERFACE(self)->pf_os_exit_critical_isr(primask); /* Restore critical section state */
        UP_TRACE_ISR_EXTI();
        return;
    }   

    /* Debug log: print buffer pointers, pending data length, and read/write counters */
    UP_DEBUG_ISR("start_point=%d, end_point=%d, ring_len=%d.", pre_parse_counter, end_counter, parse_ring_len);
    UP_DEBUG_ISR("header=%u, tail=%u.\r\n", PRIV_DATA(self)->header, PRIV_DATA(self)->tail);

    /* Check for data overflow: read pointer (header) exceeds write pointer (tail) by buffer size */
    /* Indicates received data speed exceeds parsing speed (data loss occurred) */
    if (PRIV_DATA(self)->header - PRIV_DATA(self)->tail >= RECV_BUF_SIZE(self))
    {
        UP_DEBUG_BLUE("fatal error: overflow\r\n");
        /* reset uart state */
        reset_uart_state(self);

        OS_INTERFACE(self)->pf_os_exit_critical_isr(primask); /* Restore critical section state */
        UP_TRACE_ISR_EXTI();
        return;
    }
    
    PRIV_DATA(self)->data_counter = end_counter; /* Update DMA counter cache (sync with current end index) */

    /* Linearize ring buffer data into parse_buf (converts ring data to linear for easy parsing) */
#ifdef NON_COPY_WHEN_NON_WRAP
    bool if_wrap = false;
#endif    
    if (parse_ring_len > 0)
    {
        if (pre_parse_counter <= end_counter || 0 == end_counter)
        {
#ifndef NON_COPY_WHEN_NON_WRAP
            /* Case 1: No buffer wrap-around - copy entire pending data in one segment */
            uint16_t copy_len = parse_ring_len;
            memcpy(PRIV_DATA(self)->parse_buf,
                   RECV_BUF_ADDR(self) + pre_parse_counter,
                   copy_len);
#endif  
        }
        else
        {
            /* Case 2: Buffer wrap-around - copy in two segments to cover full pending data */
            /* Segment 1: From pre_parse_counter to end of receive buffer */
            uint16_t len1 = RECV_BUF_SIZE(self) - pre_parse_counter;
            memcpy(PRIV_DATA(self)->parse_buf,
                   RECV_BUF_ADDR(self) + pre_parse_counter,
                   len1);

            /* Segment 2: From start of receive buffer to end_counter */
            uint16_t len2 = end_counter;
            memcpy(PRIV_DATA(self)->parse_buf + len1,
                   RECV_BUF_ADDR(self),
                   len2);
#ifdef NON_COPY_WHEN_NON_WRAP                   
            if_wrap = true;
#endif            
        }
    }

    /* Parse linearized data into frames (loop until parsing fails or no more data) */
    frame_info_t frame_info;
    uint16_t unpack_len = 0; /* Total length of successfully parsed valid frames */
    algo_status_t algo_status;        /* Storage for parsing algorithm return status */

    /* Loop: parse until algorithm returns non-ALGO_OK (no more valid frames or error) */
#ifdef NON_COPY_WHEN_NON_WRAP    
    uint8_t *parse_addr = if_wrap ? PRIV_DATA(self)->parse_buf : (RECV_BUF_ADDR(self) + pre_parse_counter);
    while ((ALGO_OK == (algo_status = PARSE_ALGO(self)(parse_addr + unpack_len,
#else
    while ((ALGO_OK == (algo_status = PARSE_ALGO(self)(PRIV_DATA(self)->parse_buf + unpack_len, 
#endif       
                                                       parse_ring_len - unpack_len,
                                                       &frame_info))) ||
           (ALGO_ERR_CRC == algo_status) ||
           (ALGO_ERR_LENGTH_INVALID == algo_status) ||
           (ALGO_ERR_NOICE == algo_status))                                         
    {

        /* Prepare parsed frame info for queuing to RTOS (pass to parse thread) */
        parse_info_t parse_info =
        {
            .fun_code = frame_info.fun_code,  /* Extracted frame function code */
#ifdef NON_COPY_WHEN_NON_WRAP 
            .payload = parse_addr + unpack_len + frame_info.pre_payload_len, /* Payload start address */
#else            
            .payload = PRIV_DATA(self)->parse_buf + unpack_len + frame_info.pre_payload_len, /* Payload start address */  
#endif                       
            .payload_len = frame_info.payload_len /* Payload length */
        };

        /* Update total parsed length (sum of pre-payload + payload + post-payload for this frame) */
        unpack_len += frame_info.pre_payload_len + frame_info.payload_len + frame_info.post_payload_len;
        UP_DEBUG_ISR("unpack_len=%d.\r\n", unpack_len); /* Debug log: current total parsed length */
               
        if(ALGO_OK == algo_status)
        {
            PRIV_DATA(self)->payload_cnt++; /* Increment counter for pending frames in RTOS queue */
            PRIV_DATA(self)->parse_fail_cnt = 0;
            /* Queue parsed frame info to RTOS queue (use ISR-safe API for inter-thread communication) */
            OS_INTERFACE(self)->pf_os_queue_put_isr(PRIV_DATA(self)->queue_handle,
                                                    &parse_info,
                                                    NULL);
        }            
    }
    if(PRIV_DATA(self)->parse_fail_cnt >= NUM_NOTIFY_ISR_CB_CALL)
    {
        UP_DEBUG_BLUE("fatal error: parse fail cnt not many\r\n");
        reset_uart_state(self);
    }
    /* Update tail pointer: advance to total bytes successfully parsed (valid frames) */
    PRIV_DATA(self)->tail += unpack_len;

    /* 
     * Handle specific parsing errors: discard invalid data segments to avoid blocking future parsing
     * Applicable errors:
     * - ALGO_ERR_LENGTH_INVALID: Invalid data length (e.g., incomplete frame header, too short)
     * - ALGO_ERR_CRC: CRC check failed (data corrupted or invalid frame)
     * Logic: Advance tail by the remaining unparsed data (parse_ring_len - unpack_len) to discard invalid bytes
     */
    if (ALGO_ERR_OTHERS == algo_status)
    {
        PRIV_DATA(self)->tail += (parse_ring_len - unpack_len);
    }

    /* Exit ISR-safe critical section: restore original interrupt mask */
    OS_INTERFACE(self)->pf_os_exit_critical_isr(primask);
    UP_TRACE_ISR_EXTI();
}

