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
#include "queue.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "bsp_key.h"
#include "bsp_led.h"

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

/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
    .name = "defaultTask",
    .stack_size = 128 * 4,
    .priority = (osPriority_t)osPriorityNormal,
};

/*user task */
osThreadId_t key_TaskHandle;
QueueHandle_t keyQueue;
const osThreadAttr_t key_Task_attributes = {
    .name = "key_Task",
    .stack_size = 128 * 4,
    .priority = (osPriority_t)osPriorityNormal,
};

osThreadId_t led_TaskHandle;
QueueHandle_t LedQueue;
const osThreadAttr_t led_Task_attributes = {
    .name = "led_Task",
    .stack_size = 128 * 4,
    .priority = (osPriority_t)osPriorityNormal,
};
/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);
void Led_Task(void *argument);
void Key_Task(void *argument);

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
  keyQueue = xQueueCreate(1, sizeof(key_press_status_t));
  LedQueue = xQueueCreate(10, sizeof(led_operation_t));
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  key_TaskHandle = osThreadNew(Key_Task, NULL, &key_Task_attributes);
  led_TaskHandle = osThreadNew(Led_Task, NULL, &led_Task_attributes);
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
  key_press_status_t recv_data = 0;
  led_operation_t led_op = LED_ON;
  for (;;)
  {
    // printf("StartDefaultTask\n");
    if (keyQueue != NULL)
    {
      if (pdPASS == xQueueReceive(keyQueue, &recv_data, (TickType_t)100))
      {
        printf("recv_data = [%d]\r\n", recv_data);

        led_op = LED_TOGGLE;
        if (pdPASS == xQueueSend(LedQueue, &led_op, 0))
        {
          printf("Send to LedQueue\r\n");
        }
      }
    }

    osDelay(1);
  }
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
void Key_Task(void *argument)
{
  static key_press_status_t last = KEY_NO_PRESSED;
  key_press_status_t now;

  for (;;)
  {
    now = bsp_key_read();
    if (KEY_PRESSED == now && KEY_NO_PRESSED == last)
    {
      osDelay(20);
      if (KEY_PRESSED == bsp_key_read())
      {
        xQueueSend(keyQueue, &now, 0);
      }
    }
    last = now;
    osDelay(20);
  }
}

void Led_Task(void *argument)
{
  led_operation_t led_operation = LED_ON;

  for (;;)
  {
    if (LedQueue != NULL)
    {
      if (xQueueReceive(LedQueue, &led_operation, portMAX_DELAY) == pdPASS)
      {
        bsp_led(led_operation);
      }
      else
      {
        printf("led queue error\r\n");
      }
    }
  }
}
/* USER CODE END Application */
