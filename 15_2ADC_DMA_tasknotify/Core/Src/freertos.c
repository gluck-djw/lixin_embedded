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
#include "adc.h"
#include "dma.h"
#include "elog.h"
#include "semphr.h"
#include "queue.h"
#include <stdlib.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

osThreadId_t TaskBHandle;
const osThreadAttr_t TaskB_attributes = {
    .name = "TaskB",
    .stack_size = 128 * 4,
    .priority = (osPriority_t)osPriorityNormal,
};

/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
    .name = "defaultTask",
    .stack_size = 128 * 4,
    .priority = (osPriority_t)osPriorityNormal,
};

extern DMA_HandleTypeDef hdma_adc1;

QueueHandle_t freeBufQueue;
QueueHandle_t processQueue;

#define BUF_COUNT 2 // buffer 数量
#define BUF_SIZE 1  // 每个 buffer 的 uint32_t 个数（当前是单次转换）

static uint32_t *g_dma_buf; // ← ISR 靠这个知道刚填满的是哪个 buffer
/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);
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
  freeBufQueue = xQueueCreate(BUF_COUNT, sizeof(uint32_t *));
  processQueue = xQueueCreate(BUF_COUNT, sizeof(uint32_t *));

  for (int i = 0; i < BUF_COUNT; i++)
  {
    uint32_t *pbuf = malloc(BUF_SIZE * sizeof(uint32_t));
    xQueueSend(freeBufQueue, &pbuf, 0);
  }
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  TaskBHandle = osThreadNew(TaskB, NULL, &TaskB_attributes);
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
/* ---- 生产者：乒乓 DMA（线程A） ---- */
void StartDefaultTask(void *argument)
{

  /* USER CODE BEGIN StartDefaultTask */
  for (;;)
  {
    uint32_t *buf;

    if (xQueueReceive(freeBufQueue, &buf, portMAX_DELAY) != pdTRUE)
      continue;
    g_dma_buf = buf;
    HAL_ADC_Start_DMA(&hadc1, buf, 1);
  }
  /* USER CODE END StartDefaultTask */
}

/* ---- 消费者：转换电压并输出（线程B） ---- */
void TaskB(void *argument)
{
  for (;;)
  {
    uint32_t *consum_buf;
    xQueueReceive(processQueue, &consum_buf, portMAX_DELAY);
    uint32_t mv = (3300U * (*consum_buf)) >> 12; /* ADC码值 -> mV (12bit, 3.3V) */
    elog_raw("B: raw=%u  %u.%03u V\r\n",
             (unsigned)(*consum_buf),
             (unsigned)(mv / 1000), (unsigned)(mv % 1000));
    elog_flush();
    xQueueSend(freeBufQueue, &consum_buf, portMAX_DELAY);
  }
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
  BaseType_t xHigher = pdFALSE;
  xQueueSendFromISR(processQueue, &g_dma_buf, &xHigher);
  portYIELD_FROM_ISR(xHigher);
}
/* USER CODE END Application */
