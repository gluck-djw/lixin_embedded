/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file    usart.c
 * @brief   This file provides code for the configuration
 *          of the USART instances.
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
#include "usart.h"

/* USER CODE BEGIN 0 */
uart_ops_t uart_ops =
    {
        .pf_uart_recv = uart_recv,
        .pf_uart_trans = uart_transmit_data,
        .pf_get_counter = get_counter,
        .pf_set_counter = set_counter};
/* USER CODE END 0 */

UART_HandleTypeDef huart1;
DMA_HandleTypeDef hdma_usart1_rx;

/* USART1 init function */

void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */
}

void HAL_UART_MspInit(UART_HandleTypeDef *uartHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if (uartHandle->Instance == USART1)
  {
    /* USER CODE BEGIN USART1_MspInit 0 */

    /* USER CODE END USART1_MspInit 0 */
    /* USART1 clock enable */
    __HAL_RCC_USART1_CLK_ENABLE();

    __HAL_RCC_GPIOA_CLK_ENABLE();
    /**USART1 GPIO Configuration
    PA9     ------> USART1_TX
    PA10     ------> USART1_RX
    */
    GPIO_InitStruct.Pin = GPIO_PIN_9 | GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* USART1 DMA Init */
    /* USART1_RX Init */
    hdma_usart1_rx.Instance = DMA2_Stream2;
    hdma_usart1_rx.Init.Channel = DMA_CHANNEL_4;
    hdma_usart1_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_usart1_rx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_usart1_rx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_usart1_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_usart1_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_usart1_rx.Init.Mode = DMA_CIRCULAR;
    hdma_usart1_rx.Init.Priority = DMA_PRIORITY_LOW;
    hdma_usart1_rx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    if (HAL_DMA_Init(&hdma_usart1_rx) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(uartHandle, hdmarx, hdma_usart1_rx);

    /* USART1 interrupt Init */
    HAL_NVIC_SetPriority(USART1_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
    /* USER CODE BEGIN USART1_MspInit 1 */

    /* USER CODE END USART1_MspInit 1 */
  }
}

void HAL_UART_MspDeInit(UART_HandleTypeDef *uartHandle)
{

  if (uartHandle->Instance == USART1)
  {
    /* USER CODE BEGIN USART1_MspDeInit 0 */

    /* USER CODE END USART1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_USART1_CLK_DISABLE();

    /**USART1 GPIO Configuration
    PA9     ------> USART1_TX
    PA10     ------> USART1_RX
    */
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_9 | GPIO_PIN_10);

    /* USART1 DMA DeInit */
    HAL_DMA_DeInit(uartHandle->hdmarx);

    /* USART1 interrupt Deinit */
    HAL_NVIC_DisableIRQ(USART1_IRQn);
    /* USER CODE BEGIN USART1_MspDeInit 1 */

    /* USER CODE END USART1_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */
static ring_buffer_t recv_buffer;

static void uart_recv(void)
{

  // __HAL_UART_ENABLE_IT(&huart1, UART_IT_IDLE);
  // 固定长度
  //   HAL_UART_Receive_DMA(&huart1, uart1_rx_buf, 1);

  // 不定长度
  HAL_UARTEx_ReceiveToIdle_DMA(&huart1, recv_buffer.data, sizeof(recv_buffer.data));
}

static void uart_transmit_data(const uint8_t *pdata, uint16_t len)
{
  HAL_UART_Transmit_DMA(&huart1, (uint8_t *)pdata, len);
}

static uint16_t get_counter(void)
{
  return ((uint16_t)__HAL_DMA_GET_COUNTER(&hdma_usart1_rx));
}

static void set_counter(uint16_t counter)
{
  __HAL_DMA_SET_COUNTER(&hdma_usart1_rx, counter);
}

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

/* USER CODE END 1 * /
