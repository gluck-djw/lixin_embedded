/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * File Name          : freertos.c
 * Description        : Code for freertos applications
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2026 STMicroelectronics.
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
#include "elog.h"
#include "ring_buffer.h"
#include "proto.h"
#include "queue.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
extern UART_HandleTypeDef huart1;
static ring_buffer_t g_ring_buffer;
extern  app_subscribe_t app_subscribers[MAX_SUBSCRIBERS]; // 订阅者

QueueHandle_t xrecvque;      /* ISR → 前端 */
QueueHandle_t xqueue_notify; /* 前端 → 后端 */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
    .name = "defaultTask",
    .stack_size = 128 * 4,
    .priority = (osPriority_t)osPriorityNormal,
};

osThreadId_t TaskAHandle;
const osThreadAttr_t TaskA_attributes = {
    .name = "TaskA",
    .stack_size = 128 * 8,
    .priority = (osPriority_t)osPriorityNormal,
};

osThreadId_t TaskBHandle;
const osThreadAttr_t TaskB_attributes = {
    .name = "TaskB",
    .stack_size = 128 * 8,
    .priority = (osPriority_t)osPriorityNormal,
};
/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);
void TaskA(void *argument);
void TaskB(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
 * @brief  FreeRTOS initialization
 * @param  None
 * @retval None
 */
void MX_FREERTOS_Init(void)
{
  /* USER CODE BEGIN Init */
  // Initialize ring buffer
  ring_buffer_init(&g_ring_buffer);

  // Initialize queue
  xrecvque = xQueueCreate(1, sizeof(uint32_t));
  xqueue_notify = xQueueCreate(1, sizeof(uint32_t));
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
  TaskAHandle = osThreadNew(TaskA, NULL, &TaskA_attributes);
  TaskBHandle = osThreadNew(TaskB, NULL, &TaskB_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */
}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
 * @brief  Function implementing the defaultTask thread.
 * @param  argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN StartDefaultTask */
  /* Infinite loop */
  // app 订阅数据
  QueueHandle_t rx_queue1 = xQueueCreate(10, sizeof(app_msg_t));

  app_subscribe_t app1 = {.cmd = {0x01}, .rx_queue = rx_queue1};
  app_subscribers[0] = app1; // 注册到全局订阅表

  app_msg_t app1_msg = {0};
  for (;;)
  {
    if (xQueueReceive(rx_queue1, &app1_msg, portMAX_DELAY))
    {
      // 处理数据
      log_i("app1_msg_cnt: %d", app1_msg.data_cnt);
      for (int i = 0; i < app1_msg.data_cnt; i++)
      {
        log_i("app1_msg_data: %d", app1_msg.data[i]);
      }
    }
    osDelay(1);
  }
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* ─── 前端 Task：串口驱动层 ─── */
void TaskA(void *argument)
{
  log_i("TaskA started !!!");

  uint32_t irq_signal;
  /* ① 启动串口接收 */
  // 单字节接收
  //   HAL_UART_Receive_DMA(&huart1, uart1_rx_buf, 1);

  // 多字节接收
  HAL_UARTEx_ReceiveToIdle_DMA(&huart1, g_ring_buffer.data, RING_BUF_SIZE);

  for (;;)
  {
    if (xQueueReceive(xrecvque, &irq_signal, portMAX_DELAY) == pdPASS)
    {
      if (IRQ_SEND_TO_THREAD == irq_signal)
      {
        if (ring_buffer_is_full(&g_ring_buffer))
        {
          log_i("Buffer full, skip notify!");
          continue;
        }

        uint32_t notify = FRONT_SEND_TO_END;
        xQueueSend(xqueue_notify, &notify, 0);
      }
    }
  }
}

/* ─── 后端 Task：应用协议层 ─── */
void TaskB(void *argument)
{
  log_i("TaskB started !!!");

  uint32_t app_signal;
  for (;;)
  {
    if (xQueueReceive(xqueue_notify, &app_signal, portMAX_DELAY) == pdPASS)
    {
      if (FRONT_SEND_TO_END == app_signal)
      {
        /* 对数据进行解包：检测包头 → 输出 → 检测包尾 → 停止 */
        frame_parse_transfer(&g_ring_buffer);
      }
    }
  }
}

/* USER CODE END Application */

void dma_rx_half_irq_callback(UART_HandleTypeDef *huart, uint16_t Size)
{
  if (huart->Instance == USART1)
  {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    size_t head_pos = 0;
    uint8_t ret = 0xfe;

    ret = ring_buffer_get_head(&g_ring_buffer, &head_pos);
    if (ret != 0)
    {
      log_i("ring_buffer_get_head error: %d", ret);
    }

    uint32_t half_size = RING_BUF_SIZE / 2;
    uint32_t move_pos = half_size - (head_pos % half_size);

    ring_buffer_move_head(&g_ring_buffer, move_pos);

    /* ② 通知前端 */
    uint32_t notify = IRQ_SEND_TO_THREAD;
    xQueueSendFromISR(xrecvque, &notify, &xHigherPriorityTaskWoken);

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
  }
}

void dma_rx_complete_irq_callback(UART_HandleTypeDef *huart, uint16_t Size)
{
  if (huart->Instance == USART1)
  {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    log_i("dma_rx_irq_callback size:%d", Size);
    size_t head_pos = 0;
    uint8_t ret = 0xfe;

    ret = ring_buffer_get_head(&g_ring_buffer, &head_pos);
    if (ret != 0)
    {
      log_i("ring_buffer_get_head error: %d", ret);
    }
    uint32_t move_pos = RING_BUF_SIZE - (head_pos % RING_BUF_SIZE);
    log_i("move_pos:%d", move_pos);
    ring_buffer_move_head(&g_ring_buffer, move_pos);
    log_i("ring_buffer_move_head done, head_pos:%d", g_ring_buffer.head);

    /* ② 通知前端 */
    uint32_t notify = IRQ_SEND_TO_THREAD;
    xQueueSendFromISR(xrecvque, &notify, &xHigherPriorityTaskWoken);

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
  }
}

void uart_idle_irq_callback(UART_HandleTypeDef *huart, uint16_t Size)
{
  if (huart->Instance == USART1)
  {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    log_i("uart_idle_irq_callback, size:%d", Size);
    size_t head_pos = 0;
    uint8_t ret = 0xfe;
    uint32_t move_pos = 0;

    ret = ring_buffer_get_head(&g_ring_buffer, &head_pos);
    if (ret != 0)
    {
      log_i("ring_buffer_get_head error: %d", ret);
    }
    if (Size < head_pos)
    {
      move_pos = (Size + RING_BUF_SIZE) - (head_pos % RING_BUF_SIZE);
    }
    else
    {
      move_pos = Size - (head_pos % RING_BUF_SIZE);
    }

    log_i("move_pos:%d", move_pos);
    ring_buffer_move_head(&g_ring_buffer, move_pos);
    log_i("ring_buffer_move_head done, head_pos:%d", g_ring_buffer.head);

    /* ② 通知前端 */
    uint32_t notify = IRQ_SEND_TO_THREAD;
    xQueueSendFromISR(xrecvque, &notify, &xHigherPriorityTaskWoken);

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
  }
}
