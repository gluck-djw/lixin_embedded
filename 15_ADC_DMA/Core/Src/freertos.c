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
QueueHandle_t qIsrToA;
QueueHandle_t qAToB;

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

extern uint32_t *buf1;
extern uint32_t *buf2;
// static SemaphoreHandle_t g_xMutex;
extern DMA_HandleTypeDef hdma_adc1;
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
  buf1 = (uint32_t *)malloc(sizeof(uint32_t));
  buf2 = (uint32_t *)malloc(sizeof(uint32_t));
  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  // /* add mutexes, ... */
  // g_xMutex = xSemaphoreCreateMutex();
  // if (g_xMutex == NULL)
  // {
  //   elog_raw("create g_xMutex failed\r\n");
  // }
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  qIsrToA = xQueueCreate(2, sizeof(uint32_t)); /* 事件邮箱 */
  qAToB = xQueueCreate(2, sizeof(uint32_t *)); /* buffer 地址邮箱 */
  elog_raw("INIT: qIsrToA=%p qAToB=%p buf1=%p buf2=%p\r\n",
           qIsrToA, qAToB, buf1, buf2);
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  TaskBHandle = osThreadNew(TaskB, NULL, &TaskB_attributes);
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */

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
  uint32_t *cur = buf1; /* DMA 本次目标 buffer */
  uint32_t *done;       /* 本次已填满的 buffer */

  /* USER CODE BEGIN StartDefaultTask */
  volatile HAL_StatusTypeDef st = HAL_ADC_Start_DMA(&hadc1, cur, 1);
  log_d("start");
  for (;;)
  {
    uint32_t evt;
    if (xQueueReceive(qIsrToA, &evt, portMAX_DELAY) != pdTRUE)
      continue;
    done = cur; /* 刚填满的 buffer */

    /* 把已填满 buffer 地址通过邮箱发给线程B */
    xQueueSend(qAToB, &done, portMAX_DELAY);

    log_d("buf%d", (done == buf1) ? 1 : 2);
    elog_flush();
    cur = (cur == buf1) ? buf2 : buf1; /* 设置 DMA 下次目标 buffer */

    HAL_ADC_Start_DMA(&hadc1, cur, 1);

    /* 回到接收处阻塞 */
  }
  /* USER CODE END StartDefaultTask */
}

/* ---- 消费者：转换电压并输出（线程B） ---- */
void TaskB(void *argument)
{
  for (;;)
  {
    uint32_t *addr;
    if (xQueueReceive(qAToB, &addr, portMAX_DELAY) != pdTRUE)
      continue;

    uint32_t mv = (3300U * (*addr)) >> 12; /* ADC码值 -> mV (12bit, 3.3V) */
    elog_raw("B: buf%d raw=%u  %u.%03u V\r\n",
             (addr == buf1) ? 1 : 2,
             (unsigned)(*addr),
             (unsigned)(mv / 1000), (unsigned)(mv % 1000));
    elog_flush();
  }
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
  BaseType_t xHigher = pdFALSE;
  uint32_t evt = 1;
  /* ISR 第一件事：给线程A 发邮箱 */
  xQueueSendFromISR(qIsrToA, &evt, &xHigher);
  portYIELD_FROM_ISR(xHigher);
}
/* USER CODE END Application */
