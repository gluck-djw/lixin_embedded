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
// #define buffer1 0
// #define buffer2 1

// uint8_t g_buffer1[1] = {0x00};
// uint8_t g_buffer2[1] = {0x00};
// uint8_t default_buffer = buffer1;
static ring_buffer_t g_ring_buffer;
#define RING_BUF_SIZE 64
static uint8_t ring_data[RING_BUF_SIZE] = {0};
#define DMA_BUF_SIZE 32
static uint8_t uart1_rx_buf[DMA_BUF_SIZE] = {0};

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
  ring_buffer_init(&g_ring_buffer, ring_data, RING_BUF_SIZE);
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
  for (;;)
  {
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
  HAL_UARTEx_ReceiveToIdle_DMA(&huart1, uart1_rx_buf, DMA_BUF_SIZE);

  for (;;)
  {
    /* ② 串口接收中断信号，通知环形缓冲区已有数据 */
    if (xQueueReceive(xrecvque, &irq_signal, portMAX_DELAY) == pdPASS)
    {
      if (IRQ_SEND_TO_THREAD == irq_signal)
      {
        /* ① buffer 是否已满 → 停下串口 */
        if (ring_buffer_is_full(&g_ring_buffer))
        {
          HAL_UART_AbortReceive(&huart1);
          log_i("Buffer full, UART stopped!");
        }

        /* ② 通知后端 */
        uint32_t notify = FRONT_SEND_TO_END;
        xQueueSend(xqueue_notify, &notify, 0);

        /* ③ 等 ISR 重启 DMA，给 IDLE 留时间 */
        osDelay(5);
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
        frame_parse(&g_ring_buffer);
      }
    }
  }
}

/* ─── ISR：IDLE 或 DMA 收满回调 ─── */
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
  if (huart->Instance == USART1)
  {
    log_i("HAL_UARTEx_RxEventCallback size:%d", Size);
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    /* ① 将实际收到的数据倒入环形缓冲区（Size = 实际收到几个） */
    ring_buffer_put_batch(&g_ring_buffer, uart1_rx_buf, Size);

    /* ② 通知前端 */
    uint32_t notify = IRQ_SEND_TO_THREAD;
    xQueueSendFromISR(xrecvque, &notify, &xHigherPriorityTaskWoken);

    /* ③ 继续接收 */
    HAL_UARTEx_ReceiveToIdle_DMA(&huart1, uart1_rx_buf, DMA_BUF_SIZE);

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
  }
}

/* USER CODE END Application */
