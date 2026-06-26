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
#include "bsp_key.h"
#include "led_sm.h"
#include "key.h"
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

/* USER CODE END Variables */

/* ---- 用户任务变量 ---- */
QueueHandle_t keyQueue = NULL;
QueueHandle_t ledQueue = NULL;

osThreadId_t key_TaskHandle;
const osThreadAttr_t key_Task_attributes = {
  .name = "key_Task",
  .stack_size = 128 * 4,
  .priority = (osPriority_t)osPriorityNormal,
};

osThreadId_t led_TaskHandle;
const osThreadAttr_t led_Task_attributes = {
  .name = "led_Task",
  .stack_size = 128 * 4,
  .priority = (osPriority_t)osPriorityNormal,
};
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t)osPriorityNormal,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void* argument);
void Key_Task(void* argument);
void Led_Task(void* argument);

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
  keyQueue = xQueueCreate(5, sizeof(key_isr_msg_t));
  ledQueue = xQueueCreate(5, sizeof(key_event_t));
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
void StartDefaultTask(void* argument) {
  /* USER CODE BEGIN StartDefaultTask */
  for (;;) {
    osDelay(1000);
  }
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
void Key_Task(void* argument) {
  key_isr_msg_t key_msg;

  for (;;) {
    if (xQueueReceive(keyQueue, &key_msg, portMAX_DELAY) == pdPASS) {
      key_event_t keyop = key_tick(key_msg);
      if (keyop != KEY_NONE_PRESSED) {
        // printf("keyop = [%d]\r\n", keyop);
        xQueueSend(ledQueue, &keyop, 0);
      }
    }

    osDelay(5);
  }
}
void Led_Task(void* argument) {
  key_event_t recv_data = KEY_NONE_PRESSED;
  led_cmd_t led_op = LED_CMD_TOGGLE;
  for (;;) {
    if (ledQueue == NULL) {
      osDelay(5);
      continue;
    }

    /* 非阻塞取命令，有就喂给状态机 */
    if (xQueueReceive(ledQueue, &recv_data, 0) == pdPASS) {
      printf("recv_data = [%d]\r\n", recv_data);
      switch (recv_data) {
      case KEY_SHORT_PRESSED:
        led_op = LED_CMD_TOGGLE;
        printf("KEY_SHORT_PRESSED → LED_CMD_TOGGLE\r\n");
        break;
      case KEY_LONG_PRESSED:
        led_op = LED_CMD_BLINK;
        printf("KEY_LONG_PRESSED → LED_CMD_BLINK\r\n");
        break;
      default:
        continue;
      }
      led_sm_feed(led_op);
    }

    /* 每次循环都推进状态机（翻转计时在这里） */
    led_sm_tick();

    osDelay(1);
  }
}

/* ---- EXTI 中断回调（覆盖 HAL 弱函数） ---- */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
  if (GPIO_Pin != KEY_Pin) return;

  BaseType_t xHigher = pdFALSE;
  key_isr_msg_t msg;
  msg.tick = xTaskGetTickCountFromISR();

  if (HAL_GPIO_ReadPin(KEY_GPIO_Port, KEY_Pin) == GPIO_PIN_RESET) {
    msg.edge = KEY_EDGE_DOWN;
    EXTI->FTSR &= ~KEY_Pin;
    EXTI->RTSR |=  KEY_Pin;
  } else {
    msg.edge = KEY_EDGE_UP;
    EXTI->RTSR &= ~KEY_Pin;
    EXTI->FTSR |=  KEY_Pin;
  }

  xQueueSendFromISR(keyQueue, &msg, &xHigher);
  portYIELD_FROM_ISR(xHigher);
}

/* USER CODE END Application */
