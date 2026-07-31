#include "uart_proto.h"

#include <stdbool.h>

/* uart_proto_instance
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

uart_proto_status_t uart_proto_inst(uart_proto_t *self, uart_proto_input_arg_t const *p_input_arg)
{
    if (!self || !p_input_arg || !p_input_arg->uart_ops || !p_input_arg->os_interface || !p_input_arg->frame_parse_att)
        return UART_PROTO_ERROR_PARAM_INVALID;

    self->input_arg = p_input_arg;

    if(!OS_INTERFACE(self)->pf_os_malloc || !OS_INTERFACE(self)->pf_os_free)
    {
        return UART_PROTO_ERR_HANDLER_NOT_READY;
    }
}

void reset_uart_status(const uart_proto_t *self)
{
    PRIV(self)->head = PRIV(self)->tail = PRIV(self)->data_counter = 0;
    UART_IINTERFACE(p)->pf_set_counter(RECV_BUF_SIZE(self));
}

/*
Performs:
 *       1. Validation of initialization state and input handle
 *       2. Enter ISR-safe critical section (protects shared private data)
 *       3. Calculate ring buffer pointers and pending data length
 *       4. Linearization of ring buffer data into parse_buf (handles wrap-around)
 *       5. Frame parsing via user-provided parse algorithm (loop for multi-frame support)
 *       6. Queuing of valid parsed frames to RTOS queue (async processing)
 *       7. Handle parsing errors: discard invalid data segments to avoid blocking
 *       8. Exit ISR-safe critical section
 * @note Uses ISR-specific critical section API to ensure thread safety for shared data

*/
void notify_isr_cb(const uart_proto_t *self)
{
    // validate protocol handle and initialization state
    if (!self || !PRIV(self)->is_init)
        return;

    /* // enter ISR-safe critical section */
    uint32_t primask = OS_INTERFACE(self)->pf_os_enter_critical_from_isr();

    /* calculate ring buffer pointers and pending data length */

    uint16_t start_parse_pos = PRIV(self)->tail % RECV_BUF_SIZE(self); // start index of the unparsed data in receive buffer

    uint16_t recv_end_pos = RECV_BUF_SIZE(self) - UART_IINTERFACE(p)->pf_get_counter();
    // current end index of the receive buffer

    uint16_t parse_len = (recv_end_pos - start_parse_pos + RECV_BUF_SIZE(self)) % RECV_BUF_SIZE(self); // length of the unparsed data in receive buffer

    // IDLE repeat enter
    if (PRIV(self)->head == PRIV(self)->tail)
    {
        OS_INTERFACE(self)->pf_os_exit_critical_from_isr(primask);
        return;
    }

    // check overflow
    if (PRIV(self)->head - PRIV(self)->tail >= RECV_BUF_SIZE(self))
    {
        reset_uart_status();
        OS_INTERFACE(self)->pf_os_exit_critical_from_isr(primask);
        return;
    }

    PRIV(self)->data_counter = recv_end_pos;

/* Linearization of ring buffer data into parse_buf (handles wrap-around) */
#ifdef NON_COPY_WITH_NO_WRAP
    bool is_wrap = false;
#endif

    if (parse_len > 0)
    {
        if (start_parse_pos <= recv_end_pos || 0 == recv_end_pos)
        {
#ifndef NON_COPY_WITH_NO_WRAP
            memcpy(PRIV(self)->parse_buf, RECV_BUF(self)->buffer + start_parse_pos, parse_len);
#endif
        }
        else
        {
            // copy unparsed data to parse_buf
            uint16_t len1 = RECV_BUF_SIZE(self) - start_parse_pos;
            memcpy(PRIV(self)->parse_buf, RECV_BUF(self)->buffer + start_parse_pos, len1);

            memcpy(PRIV(self)->parse_buf + len1, RECV_BUF(self)->buffer, recv_end_pos);
#ifdef NON_COPY_WITH_NO_WRAP
            is_wrap = true;
#endif
        }
    }

    /* parse frame */
    uint8_t unpacked_len = 0;
    frame_info_t frame_info;
    algo_status_t algo_status;
    uint8_t parse_address = is_wrap ? PRIV(self)->parse_buf : RECV_BUF(self)->buffer + start_parse_pos;
    while (ALGO_OK == (algo_status == PARSE_ALG(self)(parse_address + unpacked_len, parse_len - unpacked_len, &frame_info)) || ALGO_ERR_CRC == algo_status || ALGO_ERR_NOICE == algo_status || ALGO_ERROR_LENGTH_INVALID == algo_status)
    {
        parse_info_t parse_info =
            {
                .funcode = frame_info.fun_code,
                .payload = parse_address + unpacked_len + frame_info.pre_payload_len,
                .payload_len = frame_info.payload_len}

        unpacked_len += frame_info.payload_len + frame_info.pre_payload_len + frame_info.post_payload_len;

        if (ALGO_OK == algo_status)
        {
            PRIV(self)->parse_fail_cnt = 0;
            PRIV(self)->pending_que_cnt++;
            OS_INTERFACE(self)->pf_os_queue_put_isr(PRIV(self)->queue_handle, &parse_info, NULL);
        }
    }

    PRIV(self)->tail += unpacked_len;

    /*  Handle parsing errors: discard invalid data segments to avoid blocking */
    if (ALGO_ERR_OTHERS == algo_status)
    {
        PRIV(self)->tail += parse_ring_len - parse_len;
    }

    OS_INTERFACE(self)->pf_os_exit_critical_isr(primask);
}