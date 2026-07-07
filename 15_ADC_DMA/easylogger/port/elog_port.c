/*
 * This file is part of the EasyLogger Library.
 *
 * Copyright (c) 2015, Armink, <armink.ztl@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * 'Software'), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Function: Portable interface for each platform.
 * Created on: 2015-04-28
 */
 
#include <elog.h>
#include "SEGGER_RTT.h"

#ifdef ELOG_ASYNC_OUTPUT_ENABLE
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

static SemaphoreHandle_t async_sem = NULL;
static TaskHandle_t async_task_handle = NULL;
static bool async_task_running = false;

#define ASYNC_FLUSH_BUF_SIZE      (ELOG_LINE_BUF_SIZE)

extern size_t elog_async_get_line_log(char *log, size_t size);
extern void elog_port_output(const char *log, size_t size);

static void elog_async_flush_task(void *arg) {
    static char buf[ASYNC_FLUSH_BUF_SIZE];
    size_t len;

    while (async_task_running) {
        if (xSemaphoreTake(async_sem, portMAX_DELAY) == pdTRUE) {
            while (1) {
                len = elog_async_get_line_log(buf, sizeof(buf));
                if (len > 0) {
                    elog_port_output(buf, len);
                } else {
                    break;
                }
            }
        }
    }
    async_task_handle = NULL;
    vTaskDelete(NULL);
}

void elog_async_output_notice(void) {
    if (async_sem) {
        xSemaphoreGive(async_sem);
    }
}
#endif /* ELOG_ASYNC_OUTPUT_ENABLE */

/**
 * EasyLogger port initialize
 *
 * @return result
 */
ElogErrCode elog_port_init(void) {
    ElogErrCode result = ELOG_NO_ERR;

#ifdef ELOG_ASYNC_OUTPUT_ENABLE
    async_sem = xSemaphoreCreateBinary();
    async_task_running = true;
    xTaskCreate(elog_async_flush_task, "elogAsync", 512, NULL,
                tskIDLE_PRIORITY + 2, &async_task_handle);
#endif

    return result;
}

/**
 * EasyLogger port deinitialize
 *
 */
void elog_port_deinit(void) {
#ifdef ELOG_ASYNC_OUTPUT_ENABLE
    if (async_task_running) {
        async_task_running = false;
        if (async_sem) {
            xSemaphoreGive(async_sem);
        }
        while (async_task_handle != NULL) {
            vTaskDelay(1);
        }
        if (async_sem) {
            vSemaphoreDelete(async_sem);
            async_sem = NULL;
        }
    }
#endif
}

/**
 * output log port interface
 *
 * @param log output of log
 * @param size log size
 */
void elog_port_output(const char *log, size_t size) {
    SEGGER_RTT_Write(0, log, size);
}

/**
 * output lock
 */
void elog_port_output_lock(void) {
    __disable_irq();
}

/**
 * output unlock
 */
void elog_port_output_unlock(void) {
    __enable_irq();
}

/**
 * get current time interface
 *
 * @return current time
 */
const char *elog_port_get_time(void) {

    /* add your code here */
    return "current_time";
}

/**
 * get current process name interface
 *
 * @return current process name
 */
const char *elog_port_get_p_info(void) {

    /* add your code here */
    return "current_process_name";
}

/**
 * get current thread name interface
 *
 * @return current thread name
 */
const char *elog_port_get_t_info(void) {

    /* add your code here */
    return "current_thread_name";
}
