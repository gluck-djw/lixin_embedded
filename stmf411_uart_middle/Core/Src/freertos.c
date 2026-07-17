/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * File Name          : freertos.c
 * Description        : Code for freertos applications
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "usart.h"
#include "uart_proto.h"
#include "queue.h"
#include <stdbool.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
static int8_t my_parse_funcode(uint8_t *const p_data,
                        uint16_t data_len, frame_info_t *const frame_info);
void os_thread_create(
                    void (*task_code)(void*),
                    const char * const task_name,
                    const uint32_t stack_depth,
                    void * const parameters,
                    uint32_t priority,
                    void ** const thread_handle ); 
void os_thread_delete(void * const thread_handle);     
void os_queue_create(uint32_t const item_num, uint32_t const item_size, void ** const queue_handle);    
void os_queue_put_isr(void * const queue_handle, void * const item, long * const HigherPriorityTaskWoken);   
void os_queue_get(void * const queue_handle, void * const item, uint32_t const timeout) ;    
void os_enter_critical(void);              
void os_exit_critical(void);    
uint32_t  os_enter_critical_isr(void);
void os_exit_critical_isr(uint32_t primask);    
void* os_malloc(uint8_t data_size);  
void os_free(void* ptr) ;  

static void funcode_cb(void *arg, uint8_t *const payload, uint16_t payload_len);
static void funcode_cb07(void *arg, uint8_t *const payload, uint16_t payload_len);
static void funcode_cb08(void *arg, uint8_t *const payload, uint16_t payload_len);

static void funcode_cb0a(void *arg, uint8_t *const payload, uint16_t payload_len);
static void funcode_cb0b(void *arg, uint8_t *const payload, uint16_t payload_len);
static void funcode_cb0c(void *arg, uint8_t *const payload, uint16_t payload_len);
static void funcode_cb0d(void *arg, uint8_t *const payload, uint16_t payload_len);
static void funcode_cb0e(void *arg, uint8_t *const payload, uint16_t payload_len);
static void funcode_cb0f(void *arg, uint8_t *const payload, uint16_t payload_len);
static void funcode_cb10(void *arg, uint8_t *const payload, uint16_t payload_len);
static void funcode_cb11(void *arg, uint8_t *const payload, uint16_t payload_len);
static void funcode_cb12(void *arg, uint8_t *const payload, uint16_t payload_len);
static void funcode_cb13(void *arg, uint8_t *const payload, uint16_t payload_len);
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
// #define TASK_DEBUG_OUT(fmt, ...)    t_printf(fmt, ##__VA_ARGS__)
#define TASK_DEBUG_OUT(fmt, ...)        UP_DEBUG_OUT(fmt, ##__VA_ARGS__)
#define TASK_DEBUG_BLUE(fmt, ...)       UP_DEBUG_BLUE(fmt, ##__VA_ARGS__)
#define TASK_DEBUG_ISR(fmt, ...)        UP_DEBUG_ISR(fmt, ##__VA_ARGS__)
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */


static parse_algo_t parse_algo = 
{
    .pf_parse_funcode = my_parse_funcode
};
static frame_parse_att_t frame_parse_att =
{
  .parse_algo = &parse_algo,
  .recv_buf = &g_recv_buf
};

os_interface_t os_interface = 
{
    .pf_os_thread_create        = os_thread_create,
    .pf_os_thread_delete        = os_thread_delete,
    .pf_os_queue_create         = os_queue_create,
    .pf_os_queue_put_isr        = os_queue_put_isr,
    .pf_os_queue_get            = os_queue_get,
    .pf_os_enter_critical       = os_enter_critical,
    .pf_os_exit_critical        = os_exit_critical,
    .pf_os_enter_critical_isr   = os_enter_critical_isr,
    .pf_os_exit_critical_isr    = os_exit_critical_isr,
    .pf_os_malloc               = os_malloc,
    .pf_os_free                 = os_free
};


static uart_proto_input_arg_t uart_proto_input_arg =
{
  .frame_parse_att  = &frame_parse_att,
  .os_interface     = &os_interface,
  .uart_ops         = &g_uart_ops
};

uart_proto_t g_uart_proto;

/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
    /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
    /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
    /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
    /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
    /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
    /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
extern UART_HandleTypeDef huart1;
/**
 * @brief  Function implementing the defaultTask thread.
 * @param  argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN StartDefaultTask */
    TASK_DEBUG_OUT("task1 begin");    
    
    uart_proto_status_t ret; 
    ret = uart_proto_inst(&g_uart_proto, &uart_proto_input_arg);
    if(UART_PROTO_OK != ret)
        TASK_DEBUG_OUT("inst error");
    else
    {
        subscribe_para_t subscribe_para = 
        {
            .arg = NULL,
            .cb = funcode_cb07,
            .fun_code = 0x07
        };
        g_uart_proto.pf_subscribe(&g_uart_proto, &subscribe_para, NULL); 
        subscribe_para.cb = funcode_cb08;
        subscribe_para.fun_code = 0x08;
        g_uart_proto.pf_subscribe(&g_uart_proto, &subscribe_para, NULL); 

        subscribe_para.cb = funcode_cb0a;
        subscribe_para.fun_code = 0x0A;
        g_uart_proto.pf_subscribe(&g_uart_proto, &subscribe_para, NULL);   
        subscribe_para.cb = funcode_cb0c;
        subscribe_para.fun_code = 0x0C;
        g_uart_proto.pf_subscribe(&g_uart_proto, &subscribe_para, NULL);   
        subscribe_para.cb = funcode_cb0b;
        subscribe_para.fun_code = 0x0B;
        g_uart_proto.pf_subscribe(&g_uart_proto, &subscribe_para, NULL);   
        subscribe_para.cb = funcode_cb0f;
        subscribe_para.fun_code = 0x0F;
        g_uart_proto.pf_subscribe(&g_uart_proto, &subscribe_para, NULL);   
        subscribe_para.cb = funcode_cb0e;
        subscribe_para.fun_code = 0x0E;
        g_uart_proto.pf_subscribe(&g_uart_proto, &subscribe_para, NULL);   
        subscribe_para.cb = funcode_cb12;
        subscribe_para.fun_code = 0x12;
        g_uart_proto.pf_subscribe(&g_uart_proto, &subscribe_para, NULL);    
        subscribe_para.cb = funcode_cb13;
        subscribe_para.fun_code = 0x13;
        g_uart_proto.pf_subscribe(&g_uart_proto, &subscribe_para, NULL);   
        subscribe_para.cb = funcode_cb10;
        subscribe_para.fun_code = 0x10;
        g_uart_proto.pf_subscribe(&g_uart_proto, &subscribe_para, NULL);  
        subscribe_para.cb = funcode_cb11;
        subscribe_para.fun_code = 0x11;
        g_uart_proto.pf_subscribe(&g_uart_proto, &subscribe_para, NULL);   
        subscribe_para.cb = funcode_cb0d;
        subscribe_para.fun_code = 0x0D;
        g_uart_proto.pf_subscribe(&g_uart_proto, &subscribe_para, NULL);    

    }
    /* Infinite loop */
    for (;;)
    {
        osDelay(1);
    }
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

static void funcode_cb(void *arg, uint8_t *const payload, uint16_t payload_len)
{
    TASK_DEBUG_OUT("pay_len=%d, is:  ", payload_len);
    for(uint16_t i=0; i<payload_len; i++)
        TASK_DEBUG_OUT("%02x ", payload[i]);
    TASK_DEBUG_OUT("\r\n"); 
    // TASK_DEBUG_OUT("%02x\r\n", payload[payload_len-1]);   
}

static void funcode_cb07(void *arg, uint8_t *const payload, uint16_t payload_len)
{
    TASK_DEBUG_OUT(" 07 fun, ");
    funcode_cb(arg, payload, payload_len);    
}

static void funcode_cb08(void *arg, uint8_t *const payload, uint16_t payload_len)
{
    TASK_DEBUG_OUT(" 08 fun, ");
    funcode_cb(arg, payload, payload_len);    
}

static void funcode_cb0a(void *arg, uint8_t *const payload, uint16_t payload_len)
{
    static uint32_t cnt = 0;
    cnt ++;
    TASK_DEBUG_OUT(" 0A fun, cntA = %d. ", cnt);
    funcode_cb(arg, payload, payload_len);       
}
static void funcode_cb0b(void *arg, uint8_t *const payload, uint16_t payload_len)
{
    static uint32_t cnt = 0;
    cnt ++;
    TASK_DEBUG_OUT(" 0B fun, cntB = %d. ", cnt);
    funcode_cb(arg, payload, payload_len);       
}
static void funcode_cb0c(void *arg, uint8_t *const payload, uint16_t payload_len)
{
    static uint32_t cnt = 0;
    cnt ++;
    TASK_DEBUG_OUT(" 0C fun, cntC = %d. ", cnt);
    funcode_cb(arg, payload, payload_len);       
}
static void funcode_cb0d(void *arg, uint8_t *const payload, uint16_t payload_len)
{
    static uint32_t cnt = 0;
    cnt ++;
    TASK_DEBUG_OUT(" 0D fun, cntD = %d. ", cnt);
    funcode_cb(arg, payload, payload_len);       
}
static void funcode_cb0e(void *arg, uint8_t *const payload, uint16_t payload_len)
{
    static uint32_t cnt = 0;
    cnt ++;
    TASK_DEBUG_OUT(" 0E fun, cntE = %d. ", cnt);
    funcode_cb(arg, payload, payload_len);       
}
static void funcode_cb0f(void *arg, uint8_t *const payload, uint16_t payload_len)
{
    static uint32_t cnt = 0;
    cnt ++;
    TASK_DEBUG_OUT(" 0F fun, cntF = %d. ", cnt);
    funcode_cb(arg, payload, payload_len);       
}
static void funcode_cb10(void *arg, uint8_t *const payload, uint16_t payload_len)
{
    static uint32_t cnt = 0;
    cnt ++;
    TASK_DEBUG_OUT(" 10 fun, cnt10 = %d. ", cnt);
    funcode_cb(arg, payload, payload_len);       
}
static void funcode_cb11(void *arg, uint8_t *const payload, uint16_t payload_len)
{
    static uint32_t cnt = 0;
    cnt ++;
    TASK_DEBUG_OUT(" 11 fun, cnt11 = %d. ", cnt);
    funcode_cb(arg, payload, payload_len);       
}
static void funcode_cb12(void *arg, uint8_t *const payload, uint16_t payload_len)
{
    static uint32_t cnt = 0;
    cnt ++;
    TASK_DEBUG_OUT(" 12 fun, cnt12 = %d. ", cnt);
    funcode_cb(arg, payload, payload_len);       
}
static void funcode_cb13(void *arg, uint8_t *const payload, uint16_t payload_len)
{
    static uint32_t cnt = 0;
    cnt ++;
    TASK_DEBUG_OUT(" 13 fun, cnt13 = %d. ", cnt);
    funcode_cb(arg, payload, payload_len);       
}



static uint16_t modbus_crc16(const uint8_t *p_data, uint16_t len) {
    uint16_t crc = 0xFFFF;
    uint8_t i, j;
    if (p_data == NULL || len == 0) {
        TASK_DEBUG_OUT("crc error");
        return crc;
    }
    for (i = 0; i < len; i++) {
        crc ^= p_data[i];  
        for (j = 0; j < 8; j++) {
            if (crc & 0x0001) {
                crc = (crc >> 1) ^ 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}
#if 1

static uint16_t find_possible_header_tail(const uint8_t *p_data,
                                          uint16_t data_len,
                                          const uint8_t *header,
                                          uint16_t header_len)
{
    /* search for possible partial header remains*/
    uint16_t possible_len = 0;
    if (0 == data_len) 
        return 0;

    for (uint16_t i = 1; i < header_len; i++)
    {
        if (0 == memcmp(&p_data[data_len - i], header, i))
        {
            possible_len = i;
        }
    }
    return possible_len;
}

static algo_status_t my_parse_funcode(uint8_t *const p_data,
                                      uint16_t data_len,
                                      frame_info_t *const frame_info)
{
    if (!p_data || !frame_info)
        return ALGO_ERR_OTHERS;

    const uint8_t header[] = {0xCC, 0xAA};
    const uint16_t min_frame_len = sizeof(header) + 1 /*fun_code*/ + 2 /*len*/ + 2 /*CRC*/ + 1 /*minimum payload*/;
    const uint16_t start_2_payload_len = sizeof(header) + 1 /*fun_code*/ + 2 /*len*/;
    const uint16_t payload_2_end_len = 2 /*CRC*/;

    if(data_len < min_frame_len)
        return ALGO_ING;    
    /* Search for frame header */
    uint16_t index = 0;
    while (index + sizeof(header) <= data_len) 
    {
        if (0 == memcmp(&p_data[index], header, sizeof(header))) 
        {
            /* Header found */
            if (data_len - index < min_frame_len) 
            {
                /* Not enough data for a full frame; keep the incomplete fragment for next time */
                frame_info->payload_len = 0;
                frame_info->pre_payload_len = index;  // Noise before header
                frame_info->post_payload_len = 0;  
                if(index)
                    return ALGO_ERR_NOICE;    
                return ALGO_ING;    
            }

            uint8_t fun_code = p_data[index + sizeof(header)];
            uint16_t payload_len = p_data[index + sizeof(header) + 1] |
                                   (p_data[index + sizeof(header) + 2] << 8);//LSB

            /* Check validity of payload length */
            if (payload_len >= g_recv_buf.buffer_size / 2 || 0 == payload_len) 
            {
                /* Invalid length, skip to next possible header */
                TASK_DEBUG_BLUE("data length illegal! len=0x%x\r\n", payload_len);
                uint16_t new_index = index + sizeof(header);
                bool found = false;
                while (new_index + sizeof(header) <= data_len) 
                {
                    if (0 == memcmp(&p_data[new_index], header, sizeof(header))) 
                    {
                        found = true;
                        break;
                    }
                    new_index++;
                }
                /* If a complete frame header is not found, then detect the possible partial frame header at the end. */
                if (found)
                {
                    frame_info->pre_payload_len = new_index;
                }
                else
                {
                    uint16_t possible_header_len = find_possible_header_tail(p_data, data_len, header, sizeof(header));
                    frame_info->pre_payload_len = data_len - possible_header_len;
                }
                frame_info->payload_len = 0;
                frame_info->post_payload_len = 0;
                return ALGO_ERR_LENGTH_INVALID;
            }

            /* Check if there is enough data for CRC */
            uint16_t total_frame_len = start_2_payload_len + payload_len + payload_2_end_len;
            if (data_len - index < total_frame_len) 
            {
                /* Data incomplete; wait for the rest of the frame */
                return ALGO_ING;
            }

            /* CRC verification */
            uint16_t crc_recv = *(uint16_t*)&p_data[index + start_2_payload_len + payload_len];//LSB
            uint16_t crc_calc = modbus_crc16(&p_data[index], start_2_payload_len + payload_len);

            /* Successfully parsed */
            if (crc_recv == crc_calc) 
            {
                frame_info->fun_code = fun_code;
                frame_info->payload_len = payload_len;
                frame_info->pre_payload_len = index + start_2_payload_len;
                frame_info->post_payload_len = payload_2_end_len;
                return ALGO_OK;
            }

            /* CRC error, try to find next header */
            TASK_DEBUG_BLUE("crc error, len=0x%x\r\n", payload_len);
            uint16_t search_start = index + 1; // Move forward one byte to avoid infinite loop
            uint16_t new_index = search_start;
            bool found = false;
            while (new_index + sizeof(header) <= data_len) 
            {
                if (0 == memcmp(&p_data[new_index], header, sizeof(header))) 
                {
                    found = true;
                    break;
                }
                new_index++;
            }
            if (found)
            {
                frame_info->pre_payload_len = new_index;
            }
            else
            {
                uint16_t possible_header_len = find_possible_header_tail(p_data, data_len, header, sizeof(header));
                frame_info->pre_payload_len = data_len - possible_header_len;
            }
            frame_info->payload_len = 0;
            frame_info->post_payload_len = 0;
            return ALGO_ERR_CRC;
            
        }
        index++;
    }

    /* No full header found, possible partial header remains */
    uint16_t possible_header_len = find_possible_header_tail(p_data, data_len, header, sizeof(header));
    frame_info->payload_len = 0;
    frame_info->pre_payload_len = data_len - possible_header_len;
    frame_info->post_payload_len = 0;

    if (frame_info->pre_payload_len)
        return ALGO_ERR_NOICE;

    return ALGO_ING;    
}
#endif

void os_thread_create(
                    void (*task_code)(void*),
                    const char * const task_name,
                    const uint32_t stack_depth,
                    void * const parameters,
                    uint32_t priority,
                    void ** const thread_handle
                                                ) 
{
    BaseType_t result = xTaskCreate(task_code, task_name, stack_depth, parameters, priority, (TaskHandle_t*)thread_handle);
    if (pdPASS != result) 
        TASK_DEBUG_BLUE("%xASSERTION FAILED: %s:%d\r\n",thread_handle, __FILE__, __LINE__);
}

void os_thread_delete(void * const thread_handle) {
    vTaskDelete((TaskHandle_t)thread_handle);    
}

void os_queue_create(uint32_t const item_num, uint32_t const item_size, void ** const queue_handle) {
    QueueHandle_t queue = xQueueCreate(item_num, item_size);
    if (queue == NULL) 
        TASK_DEBUG_BLUE("%xASSERTION FAILED: %s:%d\r\n",queue_handle, __FILE__, __LINE__);    
    *queue_handle = queue;    
}

void os_queue_put_isr(void * const queue_handle, void * const item, long * const HigherPriorityTaskWoken) {
    BaseType_t pxHigherPriorityTaskWoken = pdFALSE;
    BaseType_t result = xQueueSendFromISR((QueueHandle_t)queue_handle, item, &pxHigherPriorityTaskWoken);
    // TASK_DEBUG_BLUE("isr%d\r\n",result);
    if (HigherPriorityTaskWoken != NULL) {
        *HigherPriorityTaskWoken = pxHigherPriorityTaskWoken;
    }
    if (pdPASS != result) 
        TASK_DEBUG_BLUE("%xASSERTION FAILED: %s:%d\r\n",queue_handle, __FILE__, __LINE__);
}

void os_queue_get(void * const queue_handle, void * const item, uint32_t const timeout) {
    BaseType_t result = xQueueReceive((QueueHandle_t)queue_handle, item, timeout);
    if (pdPASS != result) 
        TASK_DEBUG_BLUE("%xASSERTION FAILED: %s:%d\r\n",queue_handle, __FILE__, __LINE__);
}

void os_enter_critical(void)
{
    taskENTER_CRITICAL();
}

void os_exit_critical(void)
{
    taskEXIT_CRITICAL();
}

uint32_t  os_enter_critical_isr(void)
{
    return taskENTER_CRITICAL_FROM_ISR(); 
}
void os_exit_critical_isr(uint32_t primask)
{
    taskEXIT_CRITICAL_FROM_ISR(primask);       
}

void* os_malloc(uint8_t data_size)
{
    return pvPortMalloc((size_t)data_size);
}

void os_free(void* ptr) 
{
    vPortFree(ptr);
}

/* USER CODE END Application */

